#include "StdAfx.h"
#include "NetProfile.h"

#if NET_PROFILE_ENABLE || NET_MINI_PROFILE
SSocketBandwidth		g_socketBandwidth;
#endif

#if NET_PROFILE_ENABLE

#include "IConsole.h"
#include "IGameFramework.h"
#include "ITimer.h"
#include "TimeValue.h"
#include "CryFixedString.h"
#include "Protocol/PacketRateCalculator.h"
#include <IPerfHud.h>
#include <CryExtension/CryCreateClassInstance.h>

#define NET_PROFILE_MAX_ENTRIES 1024
#define NUM_TABS 11
#define TAB_SIZE 8
#define ROOT_INDEX 0
#define SOCKET_BUDGET 256.f

SNetProfileStackEntry g_netProfileNull;
SNetProfileStackEntry *g_netProfileCurrent = &g_netProfileNull;

static SNetProfileStackEntry	*s_netProfileRoot;
static SNetProfileStackEntry	s_netProfileEntries[NET_PROFILE_MAX_ENTRIES];
static int					s_netProfileEntryCount = 0;
static CTimeValue		s_timeValue;
static bool					s_initialised = false;
static int					s_netProfileLogging = 0;
static int					s_netProfileBudgetLogging = 0;
static bool					s_budgetExceeded = false;
static int					s_netProfileWorstNumChannels = 0;
static float				s_netProfileSocketBudget = SOCKET_BUDGET;
static int					s_netProfileShowSocketMeasurements = 0;
static int					s_netProfileEnable = 0;

void netProfileInitEntry(SNetProfileStackEntry* parent, SNetProfileStackEntry* child, const char* name, float budget, bool rmi)
{
	assert(parent && child && name);

	child->Clear();
	child->m_name = name;
	child->m_budget = budget;
	child->m_rmi = rmi;

	child->m_next = parent->m_child;	// link to our sibling
	parent->m_child = child;			// make new child the first link
	child->m_parent = parent;			// set parent
}

void netProfileStartProfile()
{
	s_netProfileRoot->Clear();

	s_netProfileRoot->m_name = "root";
	s_netProfileRoot->m_budget = NO_BUDGET; // we now check the socket budget as a more reliable measurement

	s_netProfileEntryCount = 1;
	g_netProfileCurrent = s_netProfileRoot;

	s_budgetExceeded = false;

	s_timeValue = gEnv->pTimer->GetAsyncCurTime();
}

void netProfileInitialise(bool isMultiplayer)
{
	if(!isMultiplayer)
		return;	// don't do this for single player

	if(!s_initialised)
	{
		s_netProfileEntryCount = 0;
		s_netProfileRoot = &s_netProfileEntries[ROOT_INDEX];
	
		g_netProfileNull.Clear();
		g_netProfileNull.m_parent = &g_netProfileNull;

		netProfileStartProfile();
		netProfileRegisterWithPerfHUD();

		
		s_initialised = true;

		g_socketBandwidth.totalNumPackets = 0;
		g_socketBandwidth.totalBandwidthRecvd = 0;
		g_socketBandwidth.totalBandwidthSent = 0;
	}
}

void netProfileRegisterWithPerfHUD()
{
	cryshared_ptr<ICryPerfHUD> pPerfHUD;
	CryCreateClassInstanceForInterface( cryiidof<ICryPerfHUD>(),pPerfHUD );
	cryshared_ptr<minigui::IMiniGUI> pGUI;
	CryCreateClassInstanceForInterface( cryiidof<minigui::IMiniGUI>(),pGUI );

	if(pPerfHUD&&pGUI)
	{
		minigui::IMiniCtrl *pNetworkMenu = pPerfHUD->CreateMenu("Networking");
		
		if(pNetworkMenu)
		{
			pPerfHUD->CreateInfoMenuItem(pNetworkMenu,"Network Profile",netProfileRenderStats,minigui::Rect(200,300,675,375));
		}
	}
}

void netProfileShutDown()
{
	s_initialised = false;
}

bool netProfileGetChildFromCurrent(const char* name, SNetProfileStackEntry** ioEntry, bool rmi)
{
	SNetProfileStackEntry *child = g_netProfileCurrent->m_child;	// get the first child of the current profile
	bool result = false;

	while(child)
	{
		if(child->m_name == name)
		{
			assert(child->m_rmi == rmi);
			*ioEntry = child;
			result = true;
			break;
		}
		child = child->m_next;
	}

	return result;
}

void netProfileRegisterBeginCall(const char* name, SNetProfileStackEntry** ioEntry, float budget, bool rmi)
{
	ASSERT_PRIMARY_THREAD;
	assert(s_netProfileEntryCount < NET_PROFILE_MAX_ENTRIES);
	if(s_netProfileEntryCount >= NET_PROFILE_MAX_ENTRIES)
		return;	// sets as g_netProfileNULL

	SNetProfileStackEntry *entry = &s_netProfileEntries[s_netProfileEntryCount];
	netProfileInitEntry(g_netProfileCurrent, entry, name, budget, rmi);
	s_netProfileEntryCount++;

	*ioEntry = entry;

	assert(entry);
	assert(entry != g_netProfileCurrent);
}

void netProfileBeginFunction(SNetProfileStackEntry* entry, bool read)
{
	assert(entry != &g_netProfileNull);

	if(!read)
	{
		entry->m_calls++;
	}

	entry->m_parent = g_netProfileCurrent;
	g_netProfileCurrent = entry;
}

void netProfileEndFunction()
{
	assert(g_netProfileCurrent != &g_netProfileNull);
	assert(g_netProfileCurrent != s_netProfileRoot);

	g_netProfileCurrent = g_netProfileCurrent->m_parent;
}

void netProfileSumBits(SNetProfileStackEntry *entry)
{
	SNetProfileStackEntry *child = entry->m_child;
	entry->m_rmibits = entry->m_rmi ? entry->m_bits : 0.f;

	while(child)
	{
		netProfileSumBits(child);
		
		entry->m_rmibits += child->m_rmi ? child->m_bits : 0.f;
		entry->m_bits += child->m_bits;	
		child = child->m_next;
	}
}

void calculateEntryStats(SNetProfileStackEntry * entry, float parentWriteBits, int numChannels)
{
	float childBitsWritten = 0;
	SNetProfileStackEntry* child = NULL;

	for(child = entry->m_child; child; child = child->m_next)
	{
		calculateEntryStats(child, entry->m_bits, numChannels);
	}

	SNetProfileCount *counts = &entry->counts;
	float bitsTokbits = (entry->m_bits / 1024.f);
	float serialisedBits = entry->m_bits - entry->m_rmibits;
	
	counts->m_written = bitsTokbits; 
	counts->m_sent = entry->m_rmi ? bitsTokbits : ((serialisedBits * float(numChannels)) + entry->m_rmibits) / 1024.f;
	counts->m_worst = entry->m_rmi ? bitsTokbits : ((serialisedBits * float(s_netProfileWorstNumChannels)) + entry->m_rmibits) / 1024.f;
	counts->m_calls = entry->m_calls;
	counts->m_heir = parentWriteBits > 0.f ? (entry->m_bits / parentWriteBits) * 100.f : 0.f; 
	counts->m_self = s_netProfileRoot->m_bits > 0.f ? (entry->m_bits/s_netProfileRoot->m_bits) * 100.f : 0.f;
	counts->m_bits = entry->m_bits;
}

void netProfileEndProfile()
{
	netProfileSumBits(s_netProfileRoot);

	IGameFramework *pFramework = gEnv->pGame->GetIGameFramework();
	INetNub *pNub = NULL;
	int numChannels = 1; //local channel

	if(pFramework)
	{
		if(gEnv->bServer)
		{
			pNub = pFramework->GetServerNetNub();
		}
		else
		{
			pNub = pFramework->GetClientNetNub();
		}

		if(pNub)
		{
			numChannels = pNub->GetNumChannels();
		}
	}

	for(SNetProfileStackEntry *entry = s_netProfileRoot->m_child; entry; entry = entry->m_next)
	{
		calculateEntryStats(entry, s_netProfileRoot->m_bits, numChannels);
	}

	// fix up root stats
	SNetProfileStackEntry *child = s_netProfileRoot->m_child;
	SNetProfileCount *counts = &s_netProfileRoot->counts;

	counts->m_bits = s_netProfileRoot->m_bits;
	counts->m_calls = 1;
	counts->m_self = 100.f;
	counts->m_heir = 100.f;

	while(child)
	{
		SNetProfileCount *childCounts = &child->counts;
		counts->m_written += childCounts->m_written;
		counts->m_sent += childCounts->m_sent;
		counts->m_worst += childCounts->m_worst;
		child = child->m_next;
	}
}

void netProfileCheckBudgets(SNetProfileStackEntry *entry)
{
	SNetProfileStackEntry *child = entry->m_child;

	// don't check root, we cover this with the socket measurements
	if((entry != s_netProfileRoot) && (entry->m_budget > NO_BUDGET) && (entry->counts.m_worst > entry->m_budget))
	{
		NetQuickLog(true, 10.f, "%s has exceeded its budget of %.2f k/bits by %.2f k/bits", entry->m_name.c_str(), entry->m_budget, (entry->counts.m_worst - entry->m_budget));
		s_budgetExceeded = true;
	}

	while(child)
	{
		netProfileCheckBudgets(child);
		child = child->m_next;
	}
}

FILE* netProfileOpenLogFile(const char *name, int *firstTime)
{
	FILE *fout = NULL;

	if (*firstTime)
	{
		fout = fopen(name, "w+");
		*firstTime=0;
	}
	else
	{
		fout = fopen(name, "a+");
	}

	return fout;
}

static void netProfilePrintEntry(FILE* file, const SNetProfileStackEntry* entry, int depth)
{
	CryFixedStringT<1024> stringBufferTemp;
	CryFixedStringT<1024> stringBuffer;
	const SNetProfileCount *counts = &entry->counts;

	if(depth >= 0)
	{
		for(int i=0; i < depth; i++)
		{
			stringBuffer.append(".");
		}
	}

	stringBufferTemp.Format("%s", entry->m_name.c_str());
	stringBuffer.append(stringBufferTemp);

	int doTabs = NUM_TABS - (stringBuffer.length() / TAB_SIZE);
	for (int j = 0; j < doTabs; j++)
	{
		stringBuffer.append("\t"); 
	}

	stringBufferTemp.Format("%d\t%.0f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f", counts->m_calls, counts->m_bits, counts->m_written, counts->m_sent, counts->m_worst, counts->m_heir, counts->m_self);
	stringBuffer.append(stringBufferTemp);

	if(depth >= 0)
	{
		stringBufferTemp.Format("\t\t}}}%d\n", depth+1);
		stringBuffer.append(stringBufferTemp);
	}
	else
	{
		stringBuffer.append("\n");
	}

	fprintf(file, "%s", stringBuffer.c_str());
}

void netProfileFormatSocketView(FILE *file)
{
	CryFixedStringT<1024> stringBuffer;
	char tabs[NUM_TABS+1] = {'\0'};

	int doTabs = NUM_TABS - (strlen("socket") / TAB_SIZE);
	for(int i=0; i < doTabs; i++)
	{
		tabs[i] = '\t';
	}

	stringBuffer.Format("socket%s%.0f\t%.2f\t%.0f\t%.2f\t%d\t%d\t%.2f\n\n", tabs, g_socketBandwidth.sizeDisplayTx, g_socketBandwidth.sizeDisplayTx/1024.f, g_socketBandwidth.sizeDisplayRx, g_socketBandwidth.sizeDisplayRx/1024.f, g_socketBandwidth.numDisplayTx, g_socketBandwidth.numDisplayNetTicks, gEnv->pTimer->GetFrameRate());

	fprintf(file, "%s", stringBuffer.c_str());
}

void netProfileFormatFlatView(FILE* file)
{
	SNetProfileStackEntry *entry = NULL;
	
	netProfilePrintEntry(file, s_netProfileRoot, -1);

	for(entry = s_netProfileRoot->m_child; entry; entry = entry->m_next)
	{
		netProfilePrintEntry(file, entry, -1);
	}
}

void netProfileFormatHierarchyView(FILE* file, SNetProfileStackEntry* root, int depth)
{
	SNetProfileStackEntry *entry = NULL;

	netProfilePrintEntry(file, root, depth);

	for(entry = root->m_child; entry; entry = entry->m_next)
	{
		netProfileFormatHierarchyView(file, entry, depth+1);
	}
}

void netProfileWriteLogFiles()
{
	static int firstTimeLogging = 1;
	static int firstTimeBudgetLogging = 1;
	static ICVar *pLogging = gEnv->pConsole->GetCVar("net_profile_logging");
	static ICVar *pBudgetLogging = gEnv->pConsole->GetCVar("net_profile_budget_logging");
	const char *header = "calls\tbits\twritten\tsent\tworst\their%\tself%";
	char tabs[NUM_TABS+1] = {0};
	FILE *fout = NULL;

	for(int i=0; i<NUM_TABS; i++)
	{
		tabs[i] = '\t';
	}

	if(pLogging->GetIVal() != 0)
	{
		static ICVar *pName = gEnv->pConsole->GetCVar("net_profile_logname");
		fout = netProfileOpenLogFile(pName->GetString(), &firstTimeLogging);

		if(fout)
		{
			fprintf(fout, "%sbitsTx\tsent\tbitsRx\trecv\tnumPackets\n", tabs);
			netProfileFormatSocketView(fout);
			
			fprintf(fout, "%s%s\n", tabs, header);
			netProfileFormatFlatView(fout);

			fprintf(fout, "\n\n%s%s\n", tabs, header);
			netProfileFormatHierarchyView(fout, s_netProfileRoot, 0);
			fprintf(fout, "\n");
			
			fclose(fout);
			fout = NULL;
		}
	}

	if(s_budgetExceeded && pBudgetLogging->GetIVal() != 0)
	{
		static ICVar *pName = gEnv->pConsole->GetCVar("net_profile_budget_logname");
		fout = netProfileOpenLogFile(pName->GetString(), &firstTimeBudgetLogging);

		if(fout)
		{
			fprintf(fout, "%sbitsTx\tsent\tbitsRx\trecv\n", tabs);
			netProfileFormatSocketView(fout);
			
			fprintf(fout, "%s%s\n", tabs, header);
			netProfileFormatHierarchyView(fout, s_netProfileRoot, 0);
			fprintf(fout, "\n");

			fclose(fout);
			fout = NULL;
		}
	}
}

void netProfileAddBits(float bits, bool read)
{
	ASSERT_PRIMARY_THREAD;
	assert(g_netProfileCurrent != s_netProfileRoot);
	SNetProfileStackEntry* entry =  g_netProfileCurrent;

	if(!read)
	{
		entry->m_bits += bits;
	}
}

bool netProfileSocketMeasurementTick()
{
	bool shouldDumpLogs = false;
	CTimeValue when= gEnv->pTimer->GetAsyncTime();

#define		AVERAGE_PERIOD		(1.f) // 1 means don't average, and we probably want that when dumping to file
#define		UPDATE_PERIOD		(1.f / AVERAGE_PERIOD)

	g_socketBandwidth.numNetTicks++;

	if ((when - g_socketBandwidth.last) > UPDATE_PERIOD)
	{
		g_socketBandwidth.bandwidthUsedAmountTx.AddSample(g_socketBandwidth.sizeSent);
		g_socketBandwidth.avgValueTx = g_socketBandwidth.bandwidthUsedAmountTx.GetAverage() * AVERAGE_PERIOD;
		g_socketBandwidth.bandwidthUsedAmountRx.AddSample(g_socketBandwidth.sizeRecv);
		g_socketBandwidth.avgValueRx = g_socketBandwidth.bandwidthUsedAmountRx.GetAverage() * AVERAGE_PERIOD;
		g_socketBandwidth.sizeDisplayTx = g_socketBandwidth.sizeSent * AVERAGE_PERIOD;
		g_socketBandwidth.sizeDisplayRx = g_socketBandwidth.sizeRecv * AVERAGE_PERIOD;
		g_socketBandwidth.sizeSent = 0.f;
		g_socketBandwidth.sizeRecv = 0.f;
		g_socketBandwidth.numPacketsSent.AddSample(g_socketBandwidth.numSendsTx);
		g_socketBandwidth.numDisplayTx = g_socketBandwidth.numSendsTx;
		g_socketBandwidth.numSendsTx = 0;
		g_socketBandwidth.last = when;
		g_socketBandwidth.numDisplayNetTicks = g_socketBandwidth.numNetTicks;
		g_socketBandwidth.numNetTicks = 0;

		float kbits = g_socketBandwidth.sizeDisplayTx / 1024.f;
		if(kbits > s_netProfileSocketBudget)
		{
			NetQuickLog(true, 10.f, "%s has exceeded its budget of %.2f k/bits by %.2f k/bits", "root", s_netProfileSocketBudget, (kbits - s_netProfileSocketBudget));
			s_budgetExceeded = true;
		}

		shouldDumpLogs = TRUE;
	}

	static ICVar *pSocketMeasurements = gEnv->pConsole->GetCVar("net_profile_show_socket_measurements");
	if(pSocketMeasurements->GetIVal() != 0)
	{
		#define BANDWIDTH_DEBUG_RENDER_START_X		(70.f)
		#define BANDWIDTH_DEBUG_RENDER_START_Y		(500.f)

		netProfileRenderStats(BANDWIDTH_DEBUG_RENDER_START_X, BANDWIDTH_DEBUG_RENDER_START_Y);
	}

	return shouldDumpLogs;
}

void netProfileRenderStats(float x, float y)
{
	const float lineHeight = 22.f;
	IRenderer *pRenderer = gEnv->pRenderer;
	float col2[] = {1.f, 1.f, 1.f, 1.f};
	
	x+=10.f;
	y+=15.f;

	pRenderer->Draw2dLabel(x, y, 2, col2, false, "Num Packets Sent %d  %.2fk  Avg(10s) %.2fk", g_socketBandwidth.numDisplayTx, g_socketBandwidth.sizeDisplayTx / 1024.f, g_socketBandwidth.avgValueTx / 1024.f);
	y += lineHeight;
	pRenderer->Draw2dLabel(x, y, 2, col2, false, "Received %.2fk  Avg(10s) %.2fk", g_socketBandwidth.sizeDisplayRx / 1024.f, g_socketBandwidth.avgValueRx / 1024.f);
}

void netProfileTick()
{
	ASSERT_PRIMARY_THREAD;
	assert(g_netProfileCurrent == s_netProfileRoot);

	static ICVar *pLogProfiles = gEnv->pConsole->GetCVar("net_profile_logging");
	static ICVar *pLogBudget = gEnv->pConsole->GetCVar("net_profile_budget_logging");
	CTimeValue timeValue = gEnv->pTimer->GetAsyncCurTime();

	bool dumpLogs = netProfileSocketMeasurementTick();

	if(dumpLogs)
	{
		netProfileEndProfile();
		netProfileCheckBudgets(s_netProfileRoot);
		netProfileWriteLogFiles();
		netProfileStartProfile();	
	}
}

bool netProfileIsInitialised()
{
	assert(gEnv->bMultiplayer && s_initialised || !gEnv->bMultiplayer && !s_initialised);
	return (s_netProfileEnable) ? s_initialised : false;
}

SNetProfileStackEntry* netProfileGetNullProfile()
{
	return &g_netProfileNull;
}

void netProfileRegisterCVars()
{
	REGISTER_STRING("net_profile_logname","profile_net.log",VF_CHEAT, "log name for net profiler");
	REGISTER_CVAR2("net_profile_logging", &s_netProfileLogging, 0, VF_CHEAT, "enable/disable logging");
	REGISTER_STRING("net_profile_budget_logname","profile_net_budget.log",VF_CHEAT, "budget log name for net profiler");
	REGISTER_CVAR2("net_profile_budget_logging", &s_netProfileBudgetLogging, 0, VF_CHEAT, "enable/disable budget logging");
	REGISTER_CVAR2("net_profile_worst_num_channels", &s_netProfileWorstNumChannels, 15, VF_CHEAT, "maximum number of channels expected to connect");
	REGISTER_CVAR2("net_profile_socket_budget", &s_netProfileSocketBudget, SOCKET_BUDGET, VF_CHEAT, "maximum bandwidth budget for root node");
	REGISTER_CVAR2("net_profile_show_socket_measurements", &s_netProfileShowSocketMeasurements, 0, VF_CHEAT, "show bandwidth socket measurements on screen");
	REGISTER_CVAR2("net_profile_enable", &s_netProfileEnable, 0, VF_CHEAT, "enable/disable net profile feature");
}

#endif
