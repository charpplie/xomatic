/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------
History:
- 04:02:2009		Created by Ben Parbury
*************************************************************************/

#include "StdAfx.h"
#include "PersistantStats.h"

#include <IPlayerProfiles.h>

#include "IFlashPlayer.h"
#include "FrontEnd/FlashFrontEnd.h"
#include "Player.h"
#include "GameRules.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "GameRulesModules/GameRulesModulesManager.h"
#include "Menus/OptionsManager.h"
#include "HUD/HUD.h"
#include "Utility/CryWatch.h"
#include "Utility/CryHash.h"
#include "Utility/StringUtils.h"
#include "TypeInfo_impl.h"
#include <ILevelSystem.h>
#include "Endian.h"
#include "Network/Lobby/GameLobby.h"

CPersistantStats* CPersistantStats::s_persistantStats_instance = NULL;

#ifndef _RELEASE
static int s_debugPersistantStats = 0;
static int s_debugSessionStats = -1;
static float s_debugTimer = 0.0f;
#endif

static AUTOENUM_BUILDNAMEARRAY(s_intPersistantNames, IntPersistantStats);
static AUTOENUM_BUILDNAMEARRAY(s_floatPersistantNames, FloatPersistantStats);
static AUTOENUM_BUILDNAMEARRAY(s_streakIntPersistantNames, StreakIntPersistantStats);
static AUTOENUM_BUILDNAMEARRAY(s_streakFloatPersistantNames, StreakFloatPersistantStats);
static AUTOENUM_BUILDNAMEARRAY(s_mapPersistantNames, MapPersistantStats);
static AUTOENUM_BUILDNAMEARRAY(s_intDerivedPersistantNames, DerivedIntPersistantStats);
static AUTOENUM_BUILDNAMEARRAY(s_floatDerivedPersistantNames, DerivedFloatPersistantStats);
static AUTOENUM_BUILDNAMEARRAY(s_stringDerivedPersistantNames, DerivedStringPersistantStats);
static AUTOENUM_BUILDNAMEARRAY(s_intMapDerivedPersistantNames, DerivedIntMapPersistantStats);
static AUTOENUM_BUILDNAMEARRAY(s_floatMapDerivedPersistantNames, DerivedFloatMapPersistantStats);
static AUTOENUM_BUILDNAMEARRAY(s_stringMapDerivedPersistantNames, DerivedStringMapPersistantStats);

const static int k_ProfileVersionNumber = 1;

#define GET_PERSISTANT_FLAGS(a,b) b,

const static uint32 s_intStatsFlags[EIPS_Max] = 
{
	IntPersistantStats(GET_PERSISTANT_FLAGS)
};

const static uint32 s_floatStatsFlags[EFPS_Max] = 
{
	FloatPersistantStats(GET_PERSISTANT_FLAGS)
};

const static uint32 s_streakIntStatsFlags[ESIPS_Max] = 
{
	StreakIntPersistantStats(GET_PERSISTANT_FLAGS)
};

const static uint32 s_streakFloatStatsFlags[ESFPS_Max] = 
{
	StreakFloatPersistantStats(GET_PERSISTANT_FLAGS)
};

const static uint32 s_mapStatsFlags[EMPS_Max] = 
{
	MapPersistantStats(GET_PERSISTANT_FLAGS)
};

//static---------------------------------
CPersistantStats* CPersistantStats::GetInstance()
{
	CRY_ASSERT(s_persistantStats_instance);
	return s_persistantStats_instance;
}

//---------------------------------------
CPersistantStats::CPersistantStats()
{
	CRY_ASSERT(s_persistantStats_instance == NULL);
	s_persistantStats_instance = this;

	m_clientPersistantStats.Clear();
	m_sessionStats.clear();
	m_actorWeaponListener.clear();

#if defined(XENON)
	m_registeredLeaderboards = false;
	m_writingLeaderboardData = false;
#endif

#ifndef _RELEASE
	if (gEnv->pConsole)
	{
		REGISTER_CVAR(s_debugPersistantStats, 0, VF_NULL, "On screen debugging for stats");
		REGISTER_CVAR(s_debugSessionStats, -1, VF_NULL, "On screen debugging for session stats");
		gEnv->pConsole->AddCommand("ps_DumpTelemetryDescription", CmdDumpTelemetryDescription, VF_CHEAT, CVARHELP("Dumps an xml description of stats in telemetry"));
	}
#endif
}

//---------------------------------------
CPersistantStats::~CPersistantStats()
{
	ClearListeners();

	CRY_ASSERT(s_persistantStats_instance == this);
	s_persistantStats_instance = NULL;
#ifndef _RELEASE
	if (gEnv->pConsole)
	{
		gEnv->pConsole->UnregisterVariable("s_debugPersistantStats", true);
		gEnv->pConsole->UnregisterVariable("s_debugSessionStats", true);
	}
#endif
}

//---------------------------------------
void CPersistantStats::Init()
{
	CPlayerProgression::GetInstance()->AddEventListener(this);
	g_pGame->GetIGameFramework()->GetIItemSystem()->RegisterListener(this);

	Load();
}

//---------------------------------------
void CPersistantStats::Load()
{
	CRY_TODO(08, 02, 2010, "Loading and saving to different places");
	LoadFromProfile(GetClientPersistantStats());
}

//---------------------------------------
void CPersistantStats::Save()
{
	CRY_TODO(08, 02, 2010, "Loading and saving to different places");

	//update from current stats
	EntityId clientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	SSessionStats* pClientSessionStats = GetActorSessionStats(clientId);
	m_clientPersistantStats.Add(pClientSessionStats);
	pClientSessionStats->Clear();

	SaveToProfile(GetClientPersistantStats());

#if defined(XENON)
	WriteToLeaderboards();
#endif
}

//---------------------------------------
bool CPersistantStats::LoadFromProfile(SSessionStats *pSessionStats)
{
	IPlayerProfileManager *pProfileMan = g_pGame->GetOptions()->GetProfileManager();
	if ( pProfileMan )
	{
		const IPlayerProfile *pProfile = pProfileMan->GetCurrentProfile(pProfileMan->GetCurrentUser());
		if(pProfile)
		{
			int version = 0;
			pProfile->GetAttribute("MP/PersistantStats/Version", version);
			for(int i = 0; i < EIPS_Max; i++)
			{
				pProfile->GetAttribute(string().Format("MP/PersistantStats/%s", s_intPersistantNames[i]), pSessionStats->m_intStats[i]);
			}
			for(int i = 0; i < EFPS_Max; i++)
			{
				pProfile->GetAttribute(string().Format("MP/PersistantStats/%s", s_floatPersistantNames[i]), pSessionStats->m_floatStats[i]);
			}
			for(int i = 0; i < ESIPS_Max; i++)
			{
				pProfile->GetAttribute(string().Format("MP/PersistantStats/%s", s_streakIntPersistantNames[i]), pSessionStats->m_streakIntStats[i].m_maxVal);
			}
			for(int i = 0; i < ESFPS_Max; i++)
			{
				pProfile->GetAttribute(string().Format("MP/PersistantStats/%s", s_streakFloatPersistantNames[i]), pSessionStats->m_streakFloatStats[i].m_maxVal);
			}
			for(int i = 0; i < EMPS_Max; i++)
			{
				pSessionStats->m_mapStats[(EMapPersistantStats) i].Load(s_mapPersistantNames[i], pProfile);
			}
			return true;
		}
	}
	return false;
}

//---------------------------------------
bool CPersistantStats::SaveToProfile(const SSessionStats *pSessionStats)
{
	COptionsManager* pOptionsMgr = g_pGame->GetOptions();
	IPlayerProfileManager *pProfileMan = pOptionsMgr->GetProfileManager();
	if(pProfileMan)
	{
		IPlayerProfile *pProfile = pProfileMan->GetCurrentProfile(pProfileMan->GetCurrentUser());
		if(pProfile)
		{
			pProfile->SetAttribute("MP/PersistantStats/Version", k_ProfileVersionNumber);
			for(int i = 0; i < EIPS_Max; i++)
			{
				pProfile->SetAttribute(string().Format("MP/PersistantStats/%s", s_intPersistantNames[i]), pSessionStats->m_intStats[i]);
			}
			for(int i = 0; i < EFPS_Max; i++)
			{
				pProfile->SetAttribute(string().Format("MP/PersistantStats/%s", s_floatPersistantNames[i]), pSessionStats->m_floatStats[i]);
			}
			for(int i = 0; i < ESIPS_Max; i++)
			{
				pProfile->SetAttribute(string().Format("MP/PersistantStats/%s", s_streakIntPersistantNames[i]), pSessionStats->m_streakIntStats[i].m_maxVal);
			}
			for(int i = 0; i < ESFPS_Max; i++)
			{
				pProfile->SetAttribute(string().Format("MP/PersistantStats/%s", s_streakFloatPersistantNames[i]), pSessionStats->m_streakFloatStats[i].m_maxVal);
			}
			for(int i = 0; i < EMPS_Max; i++)
			{
				pSessionStats->m_mapStats[(EMapPersistantStats) i].Save(s_mapPersistantNames[i], pProfile);
			}
			pOptionsMgr->SaveProfile();
			return true;
		}
	}
	return false;
}

#if defined(XENON)
//XENON static---------------------------------------
int64 CPersistantStats::ConvertToLeaderboardScoreFromFloat(float value)
{
	return (int64) (value * 100.0f);
}

//XENON static---------------------------------------
float CPersistantStats::ConvertToFloatFromLeaderboardScore(int64 value)
{
	float fValue = (float) value;
	return (fValue / 100.0f);
}

//XENON static---------------------------------------
int64 CPersistantStats::GetLeaderboardScoreFromId(uint32 id)
{
	switch(id)
	{
	case eLB_XP:
		return CPlayerProgression::GetInstance()->GetData(EPP_XP);

	case eLB_Kills:
		return CPersistantStats::GetInstance()->GetDerivedStat(EDIPS_Kills);

	case eLB_KillDeathRatio:
		return ConvertToLeaderboardScoreFromFloat(CPersistantStats::GetInstance()->GetDerivedStat(EDFPS_KillDeathRatio));

	case eLB_Accuracy:
		return ConvertToLeaderboardScoreFromFloat(CPersistantStats::GetInstance()->GetDerivedStat(EDFPS_Accuracy));

	case eLB_KillStreak:
		return CPersistantStats::GetInstance()->GetStat(ESIPS_Kills);

	default:
			CRY_ASSERT_MESSAGE(false, "Unknown Leaderboard Id");
	}

	return 0;
}

//XENON static---------------------------------------
int64 CPersistantStats::GetColumnScoreFromColumnId(uint32 columnId)
{
	switch(columnId)
	{
	case eXPCL_Armour:
		return CPlayerProgression::GetInstance()->GetSuitModeData(eNanoSuitMode_Armor, ePPS_XP);

	case eXPCL_Tactical:
		return CPlayerProgression::GetInstance()->GetSuitModeData(eNanoSuitMode_Tactical, ePPS_XP);

	case eXPCL_Power:
		return CPlayerProgression::GetInstance()->GetSuitModeData(eNanoSuitMode_Power, ePPS_XP);

	case eXPCL_Stealth:
		return CPlayerProgression::GetInstance()->GetSuitModeData(eNanoSuitMode_Stealth, ePPS_XP);

	default:
		CRY_ASSERT_MESSAGE(false, "Unknown column Id");
		return 0;
	}
}

//XENON static---------------------------------------
uint32 CPersistantStats::GetColumnDataIdFromColumnId(uint32 columnId)
{
	switch(columnId)
	{
	case eXPCL_Armour:
		return PROPERTY_XP_ARMOUR;

	case eXPCL_Tactical:
		return PROPERTY_XP_TACTICAL;

	case eXPCL_Power:
		return PROPERTY_XP_POWER;

	case eXPCL_Stealth:
		return PROPERTY_XP_STEALTH;

	default:
		CRY_ASSERT_MESSAGE(false, "Unknown column Id");
		return 0;
	}
}

//XENON static---------------------------------------
void CPersistantStats::RegisterLeaderboards()
{
	ICryStats* stats = gEnv->pNetwork->GetLobby()->GetStats();
	if (stats)
	{
		

		for(int i = 0; i < eLB_Num; i++)
		{
			uint32 leaderboardId = GetLeaderboardIdFromIndex(i);
			m_board[i].id = leaderboardId;
			m_board[i].data.score.id = PROPERTY_SCORE;
			m_board[i].data.score.score = GetLeaderboardScoreFromId(leaderboardId);

			if(leaderboardId != eLB_XP)
			{
				m_board[i].data.numColumns = 0;
				m_board[i].data.pColumns = NULL;
			}
			else
			{
				for(int columnIndex = 0; columnIndex < eXPCL_num; columnIndex++)
				{
					uint32 columnId = GetColumnIdFromIndex(columnIndex);
					m_column[columnIndex].columnID = columnId;
					m_column[columnIndex].data.m_id = GetColumnDataIdFromColumnId(columnId);
					m_column[columnIndex].data.m_type = eCLUDT_Int64;
					m_column[columnIndex].data.m_int64 = GetColumnScoreFromColumnId(columnId);
				}
				m_board[i].data.numColumns = eXPCL_num;
				m_board[i].data.pColumns = &m_column[0];
			}
		}

		ECryLobbyError error = stats->StatsRegisterLeaderBoards(&m_board[0], eLB_Num, NULL, RegisterLeaderboardsCallback, this);
		CRY_ASSERT(error == eCLE_Success);
	}
}

//XENON static---------------------------------------
void CPersistantStats::RegisterLeaderboardsCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg)
{
	if(error == eCLE_Success)
	{
		CPersistantStats* pStats = (CPersistantStats*) pArg;
		pStats->m_registeredLeaderboards = true;
	}
}

//XENON ---------------------------------------
void CPersistantStats::WriteToLeaderboards()
{
	ICryStats* stats = gEnv->pNetwork->GetLobby()->GetStats();
	CRY_ASSERT(g_pGame->GetGameLobby()->IsSessionRunning());
	if(stats && m_registeredLeaderboards && !m_writingLeaderboardData)
	{
		//Only writing for local client (as PS3 is limited to only write for your client)
		CryUserID id = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetUserID(0);
		SCryStatsLeaderBoardWrite* pBoards = &m_board[0];
		uint32 numBoards = eLB_Num;

		for (uint32 i = 0; i < eLB_Num; i++)
		{
			uint32 leaderboardId = GetLeaderboardIdFromIndex(i);
			m_board[i].id = leaderboardId;
			m_board[i].data.score.id = PROPERTY_SCORE;
			m_board[i].data.score.score = GetLeaderboardScoreFromId(leaderboardId);
			if(leaderboardId != eLB_XP)
			{
				m_board[i].data.numColumns = 0;
				m_board[i].data.pColumns = NULL;
			}
			else
			{
				for(int columnIndex = 0; columnIndex < eXPCL_num; columnIndex++)
				{
					uint32 columnId = GetColumnIdFromIndex(columnIndex);
					m_column[columnIndex].columnID = columnId;
					m_column[columnIndex].data.m_id = GetColumnDataIdFromColumnId(columnId);;
					m_column[columnIndex].data.m_type = eCLUDT_Int64;
					m_column[columnIndex].data.m_int64 = GetColumnScoreFromColumnId(columnId);
				}
				m_board[i].data.numColumns = eXPCL_num;
				m_board[i].data.pColumns = &m_column[0];
			}
		}
		
		ECryLobbyError error = stats->StatsWriteLeaderBoards(g_pGame->GetGameLobby()->GetSessionId(), &id, &pBoards, &numBoards, 1, NULL, WriteLeaderboardsCallback, this);
		if(error == eCLE_Success)
		{
			m_writingLeaderboardData = true;
		}
	}
}

//XENON static---------------------------------------
void CPersistantStats::WriteLeaderboardsCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg)
{
	CPersistantStats* pStats = (CPersistantStats*) pArg;
	pStats->m_writingLeaderboardData = false;

	if(error == eCLE_Success)
	{
		CryLogAlways("Successfully wrote to leaderboards");
	}
	else
	{
		CryLogAlways("Failed to write to leaderboards - %d", error);
	}
}
#endif

//---------------------------------------
bool CPersistantStats::SaveBinary(const char* filename, const SSessionStats* pSessionStats, uint32 flags, bool description)
{
	ICryPak *pak=gEnv->pCryPak;
	FILE *file=pak->FOpen(filename,"wb");

	if (file)
	{
		XmlNodeRef base = NULL;
		int pos = 0;
		if(description)
		{
			base = GetISystem()->CreateXmlNode("PersistantStatsDescriptions");
		}
#ifndef _RELEASE
#define DescriptionXML(description, base, pos, name, type, mapName, testValue) \
	if(description) \
	{ \
		CreateDescriptionNode(base, pos, name, sizeof(type), #type, mapName, testValue); \
	}
#else
#define DescriptionXML(description, base, pos, name, type, mapName, testValue)  (void)0
#endif
		//Version Info
		int version = GetBinaryVersionHash(flags);
		DescriptionXML(description, base, pos, "TelemetryVersion", int, 0, version);
		WriteBinData(pak, &version, sizeof(int), 1, file);
		
		//Int stats
		int intStatsCount = 0;
		int intData[EIPS_Max];
		for(int i = 0; i < EIPS_Max; i++)
		{
			if((s_intStatsFlags[i] & flags) == flags)
			{
				DescriptionXML(description, base, pos, s_intPersistantNames[i], int, 0, pSessionStats->m_intStats[i]);
				intData[intStatsCount] = pSessionStats->m_intStats[i];
				intStatsCount++;
			}
		}
		WriteBinData(pak, &intData[0], sizeof(int), intStatsCount, file);

		//float stats
		int floatDataCount = 0;
		float floatData[EFPS_Max];
		for(int i = 0; i < EFPS_Max; i++)
		{
			if((s_floatStatsFlags[i] & flags) == flags)
			{
				DescriptionXML(description, base, pos, s_floatPersistantNames[i], float, 0, pSessionStats->m_floatStats[i]);
				floatData[floatDataCount] = pSessionStats->m_floatStats[i];
				floatDataCount++;
			}
		}
		WriteBinData(pak, &floatData[0], sizeof(float), floatDataCount, file);

		//Streak stats
		int streakDataCount = 0;
		int streakData[ESIPS_Max];
		for(int i = 0; i < ESIPS_Max; i++)
		{
			if((s_streakIntStatsFlags[i] & flags) == flags)
			{
				DescriptionXML(description, base, pos, s_streakIntPersistantNames[i], int, 0, pSessionStats->m_streakIntStats[i].m_maxVal);
				streakData[streakDataCount] = pSessionStats->m_streakIntStats[i].m_maxVal;
				streakDataCount++;
			}
		}
		WriteBinData(pak, &streakData[0], sizeof(int), streakDataCount, file);

		//Streak float stats
		int streakFloatDataCount = 0;
		float streakFloatData[ESFPS_Max];
		for(int i = 0; i < ESFPS_Max; i++)
		{
			if((s_streakFloatStatsFlags[i] & flags) == flags)
			{
				DescriptionXML(description, base, pos, s_streakFloatPersistantNames[i], float, 0, pSessionStats->m_streakFloatStats[i].m_maxVal);
				streakFloatData[streakFloatDataCount] = pSessionStats->m_streakFloatStats[i].m_maxVal;
				streakFloatDataCount++;
			}
		}
		WriteBinData(pak, &streakFloatData[0], sizeof(float), streakFloatDataCount, file);

		//GameRules map stats
		IGameRulesModulesManager *pGameRulesModulesManager = CGameRulesModulesManager::GetInstance();
		int rulesDataCount = 0;
		const int rulesCount = pGameRulesModulesManager->GetRulesCount();
		const int gamemodeRange = EMPS_GamesLost + 1 - EMPS_Gamemodes;
		int rulesData[100];
		CRY_ASSERT(rulesCount < ARRAY_COUNT(rulesData));
		for(int i = 0; i < rulesCount; i++)
		{
			const char* name = pGameRulesModulesManager->GetRules(i);
			for(int j = EMPS_Gamemodes; j < EMPS_GamesLost + 1; j++)
			{
				if((s_mapStatsFlags[j] & flags) == flags)
				{
					DescriptionXML(description, base, pos, s_mapPersistantNames[j], int, name, pSessionStats->m_mapStats[j].GetStat(name));
					rulesData[rulesDataCount] = pSessionStats->m_mapStats[j].GetStat(name);
					rulesDataCount++;
				}
			}
		}
		WriteBinData(pak, &rulesData[0], sizeof(int), rulesDataCount, file);

		//level map stats
		ILevelSystem* pLevelSystem = g_pGame->GetIGameFramework()->GetILevelSystem();
		int levelDataCount = 0;
		const int levelCount = pLevelSystem->GetLevelCount();
		int levelsData[100];
		CRY_ASSERT(levelCount < ARRAY_COUNT(levelsData));
		for(int i = 0; i < levelCount; i++)
		{
			if((s_mapStatsFlags[EMPS_Levels] & flags) == flags)
			{
				const char* name = pLevelSystem->GetLevelInfo(i)->GetName();
				DescriptionXML(description, base, pos, s_mapPersistantNames[EMPS_Levels], int, name, pSessionStats->m_mapStats[EMPS_Levels].GetStat(name));
				levelsData[levelDataCount] = pSessionStats->m_mapStats[EMPS_Levels].GetStat(name);
				levelDataCount++;
			}
		}
		WriteBinData(pak, &levelsData[0], sizeof(int), levelDataCount, file);

		//Item map stats
		IItemSystem* pItemSystem = g_pGame->GetIGameFramework()->GetIItemSystem();
		int itemDataCount = 0;
		const int itemCount = pItemSystem->GetItemParamsCount();
		const int weaponRange = EMPS_WeaponUsage + 1 - EMPS_WeaponHits;
		int itemData[500];
		CRY_ASSERT(itemCount < 500);
		for(int i = 0; i < itemCount; i++)
		{
			const char* name = pItemSystem->GetItemParamName(i);
			for(int j = EMPS_WeaponHits; j < EMPS_WeaponUsage + 1; j++)
			{
				if((s_mapStatsFlags[j] & flags) == flags)
				{
					DescriptionXML(description, base, pos, s_mapPersistantNames[j], int, name, pSessionStats->m_mapStats[j].GetStat(name));
					itemData[itemDataCount] = pSessionStats->m_mapStats[j].GetStat(name);
					itemDataCount++;
				}
			}
		}
		WriteBinData(pak, &itemData[0], sizeof(int), itemDataCount, file);

		//Hit Types
		CGameRules* pGameRules = g_pGame->GetGameRules();
		const int hitTypeCount = pGameRules->GetHitTypesCount();
		int hitTypeDataCount = 0;
		int hitTypeData[100];
		CRY_ASSERT(hitTypeCount < ARRAY_COUNT(hitTypeData));
		for(int i = 1; i <= hitTypeCount; i++)	//hit types start from 1 for some reason
		{
			if((s_mapStatsFlags[EMPS_KillsByDamageType] & flags) == flags)
			{
				const char* name = pGameRules->GetHitType(i);
				DescriptionXML(description, base, pos, s_mapPersistantNames[EMPS_KillsByDamageType], int, name, pSessionStats->m_mapStats[EMPS_KillsByDamageType].GetStat(name));
				hitTypeData[hitTypeDataCount] = pSessionStats->m_mapStats[EMPS_KillsByDamageType].GetStat(name);
				hitTypeDataCount++;
			}
		}
		WriteBinData(pak, &hitTypeData[1], sizeof(int), hitTypeDataCount, file);

		pak->FClose(file);

#undef DescriptionXML

		if(description)
		{
			base->setAttr("Version", version);

			string descriptionFilename = filename;
			descriptionFilename.append(".description.xml");
			base->saveToFile(descriptionFilename.c_str());

			CRY_ASSERT(pos == gEnv->pCryPak->FGetSize(filename));	//checks description matches output
		}
	}
	else
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Failed File access in CPersistantStats::SaveTelemetry");
		return false;
	}

	return true;
}

template <class T>
void CPersistantStats::WriteBinData(ICryPak* pPak, T *data, size_t length, size_t elems, FILE *handle)
{
#if !defined(PS3) && !defined(XENON)
	SwapEndian(data, elems, true);		//swap to BigEndian on PC
#endif

	pPak->FWrite(data, length, elems, handle);

#if !defined(PS3) && !defined(XENON)
	SwapEndian(data, elems, true);	//swap back (don't trash any live data pointers)
#endif
}

#ifndef _RELEASE
//static//---------------------------------
void CPersistantStats::CmdDumpTelemetryDescription(IConsoleCmdArgs* pCmdArgs)
{
	CPersistantStats* pStats = CPersistantStats::GetInstance();
	pStats->SaveBinary("PersistantStatsTelemetry/Testing.bin", pStats->GetClientPersistantStats(), eSF_LocalClient, true);

	ActorSessionMap::const_iterator it = pStats->m_sessionStats.begin();
	ActorSessionMap::const_iterator end = pStats->m_sessionStats.end();
	for ( ; it!=end; it++)
	{
		IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(it->first);

		string filename;
		filename.Format("PersistantStatsTelemetry/%s.bin", pActor ? pActor->GetEntity()->GetName() : "Unknown");	//might need to store name of player in session stats

		pStats->SaveBinary(filename.c_str(), &it->second, eSF_RemoteClients, true);
	}
}
//static//---------------------------------
template <class T>
void CPersistantStats::CreateDescriptionNode(XmlNodeRef base, int &pos, const char* codeName, size_t size, const char* type, const char* mapName, T testValue)
{
	XmlNodeRef node = GetISystem()->CreateXmlNode("Stat");
	node->setAttr("Position", pos);
	node->setAttr("CodeName", codeName);
	if(mapName)
	{
		node->setAttr("MapName", mapName);
	}
	node->setAttr("Type", type);
	node->setAttr("Test value", testValue);
	base->addChild(node);
	pos += size;
}
#endif

//static//---------------------------------
int CPersistantStats::GetBinaryVersionHash(uint32 flags)
{
	//slow but robust hashing of names of everything!
	uint32 hash = 40503;

	for(int i = 0; i < EIPS_Max; i++)
	{
		hash = HashStringSeed(s_intPersistantNames[i], hash);
	}
	for(int i = 0; i < EFPS_Max; i++)
	{
		hash = HashStringSeed(s_floatPersistantNames[i], hash);
	}
	for(int i = 0; i < ESIPS_Max; i++)
	{
		hash = HashStringSeed(s_streakIntPersistantNames[i], hash);
	}
	for(int i = 0; i < ESFPS_Max; i++)
	{
		hash = HashStringSeed(s_streakFloatPersistantNames[i], hash);
	}
	for(int i = 0; i < EMPS_Max; i++)
	{
		hash = HashStringSeed(s_mapPersistantNames[i], hash);
	}

	ILevelSystem* pLevelSystem = g_pGame->GetIGameFramework()->GetILevelSystem();
	const int levelCount = pLevelSystem->GetLevelCount();
	for(int i = 0; i < levelCount; i++)
	{
		const char* name = pLevelSystem->GetLevelInfo(i)->GetName();
		hash = HashStringSeed(name, hash);
	}

	IItemSystem* pItemSystem = g_pGame->GetIGameFramework()->GetIItemSystem();
	const int itemCount = pItemSystem->GetItemParamsCount();
	for(int i = 0; i < itemCount; i++)
	{
		const char* name = pItemSystem->GetItemParamName(i);
		hash = HashStringSeed(name, hash);
	}

	IGameRulesModulesManager *pGameRulesModulesManager = CGameRulesModulesManager::GetInstance();
	const int rulesCount = pGameRulesModulesManager->GetRulesCount();
	for(int i = 0; i < rulesCount; i++)
	{
		const char* name = pGameRulesModulesManager->GetRules(i);
		hash = HashStringSeed(name, hash);
	}

	CGameRules* pGameRules = g_pGame->GetGameRules();
	const int hitTypeCount = pGameRules->GetHitTypesCount();
	for(int i = 1; i <= hitTypeCount; i++)
	{
		const char* name = pGameRules->GetHitType(i);
		hash = HashStringSeed(name, hash);
	}

	hash ^= flags;

	return (int) hash;
}

//---------------------------------------
int CPersistantStats::GetStat(EIntPersistantStats stat)
{
	SSessionStats *pSessionStats = GetClientPersistantStats();
	CRY_ASSERT(stat >= 0 && stat < ARRAY_COUNT(pSessionStats->m_intStats));
	return pSessionStats->m_intStats[stat];
}

//---------------------------------------
float CPersistantStats::GetStat(EFloatPersistantStats stat)
{
	SSessionStats *pSessionStats = GetClientPersistantStats();
	CRY_ASSERT(stat >= 0 && stat < ARRAY_COUNT(pSessionStats->m_floatStats));
	return pSessionStats->m_floatStats[stat];
}

//---------------------------------------
int CPersistantStats::GetStat(EStreakIntPersistantStats stat)
{
	SSessionStats *pSessionStats = GetClientPersistantStats();
	CRY_ASSERT(stat >= 0 && stat < ARRAY_COUNT(pSessionStats->m_streakIntStats));
	return pSessionStats->m_streakIntStats[stat].m_maxVal;
}

//---------------------------------------
float CPersistantStats::GetStat(EStreakFloatPersistantStats stat)
{
	SSessionStats *pSessionStats = GetClientPersistantStats();
	CRY_ASSERT(stat >= 0 && stat < ARRAY_COUNT(pSessionStats->m_streakFloatStats));
	return pSessionStats->m_streakFloatStats[stat].m_maxVal;
}

//---------------------------------------
int CPersistantStats::GetStat(const char* name, EMapPersistantStats stat)
{
	SSessionStats *pSessionStats = GetClientPersistantStats();
	CRY_ASSERT(stat >= 0 && stat < ARRAY_COUNT(pSessionStats->m_mapStats));
	return pSessionStats->m_mapStats[stat].GetStat(name);
}

//---------------------------------------
int CPersistantStats::GetDerivedStat(EDerivedIntPersistantStats stat)
{
	CRY_ASSERT(stat > EDIPS_Invalid && stat < EDIPS_Max);
	SSessionStats *pSessionStats = GetClientPersistantStats();
	switch(stat)
	{
	case EDIPS_GamesWon:
		return pSessionStats->m_mapStats[EMPS_GamesWon].GetTotal();

	case EDIPS_GamesDrawn:
		return pSessionStats->m_mapStats[EMPS_GamesDrawn].GetTotal();

	case EDIPS_GamesLost:
		return pSessionStats->m_mapStats[EMPS_GamesLost].GetTotal();

	case EDIPS_GamesPlayed:
		return GetDerivedStat(EDIPS_GamesWon) + GetDerivedStat(EDIPS_GamesDrawn) + GetDerivedStat(EDIPS_GamesLost);

	case EDIPS_Kills:
		return GetStat(EIPS_KillsSuitPower) + GetStat(EIPS_KillsSuitArmor) + GetStat(EIPS_KillsSuitStealth) + GetStat(EIPS_KillsSuitTactical) + GetStat(EIPS_KillsNoSuit);

	case EDIPS_Deaths:
		return GetStat(EIPS_DeathsSuitPower) + GetStat(EIPS_DeathsSuitArmor) + GetStat(EIPS_DeathsSuitStealth) + GetStat(EIPS_DeathsSuitTactical) + GetStat(EIPS_DeathsNoSuit) + GetStat(EIPS_Suicides);

	case EDIPS_Hits:
		return pSessionStats->m_mapStats[EMPS_WeaponHits].GetTotal();

	case EDIPS_Shots:
		return pSessionStats->m_mapStats[EMPS_WeaponShots].GetTotal();

	case EDIPS_Misses:
		return GetDerivedStat(EDIPS_Shots) - GetDerivedStat(EDIPS_Hits);

	case EDIPS_Headshots:
		return pSessionStats->m_mapStats[EMPS_WeaponHeadshots].GetTotal();

	default:
		CRY_ASSERT_MESSAGE(false, string().Format("Failed to find EDerivedIntPersistantStats %d", stat));
		return 0;
	}
}

//---------------------------------------
float CPersistantStats::GetDerivedStat(EDerivedFloatPersistantStats stat)
{
	CRY_ASSERT(stat > EDFPS_Invalid && stat < EDFPS_Max);

	switch(stat)
	{
	case EDFPS_WinLoseRatio:
		return GetRatio(GetDerivedStat(EDIPS_GamesWon), GetDerivedStat(EDIPS_GamesLost));

	case EDFPS_KillDeathRatio:
		return GetRatio(GetDerivedStat(EDIPS_Kills), GetDerivedStat(EDIPS_Deaths));

	case EDFPS_Accuracy:
		return GetRatio(GetDerivedStat(EDIPS_Hits), GetDerivedStat(EDIPS_Shots)) * 100.0f; //percentage

	case EDFPS_AliveTime:
		return GetStat(EFPS_SuitPowerTime) + GetStat(EFPS_SuitArmorTime) + GetStat(EFPS_SuitStealthTime) + GetStat(EFPS_SuitTacticalTime) + GetStat(EFPS_NoSuitTime);

	default:
		CRY_ASSERT_MESSAGE(false, string().Format("Failed to find EDerivedFloatPersistantStats %d", stat));
		return 0.0f;
	}
}

//---------------------------------------
int CPersistantStats::GetDerivedStat(const char* name, EDerivedIntMapPersistantStats stat)
{
	CRY_ASSERT(stat > EDIMPS_Invalid && stat < EDIMPS_Max);
	switch(stat)
	{
	case EDIMPS_GamesPlayed:
		return GetStat(name, EMPS_GamesWon) + GetStat(name, EMPS_GamesDrawn) + GetStat(name, EMPS_GamesLost);

	default:
		CRY_ASSERT_MESSAGE(false, string().Format("Failed to find EDerivedIntMapPersistantStats %d", stat));
		return 0;
	}
}

//---------------------------------------
float CPersistantStats::GetDerivedStat(const char* name, EDerivedFloatMapPersistantStats stat)
{
	CRY_ASSERT(stat > EDFMPS_Invalid && stat < EDFMPS_Max);
	switch(stat)
	{
	case EDFMPS_Accuracy:
		return GetRatio(GetStat(name, EMPS_WeaponHits), GetStat(name, EMPS_WeaponShots)) * 100.0f;	//percentage

	default:
		CRY_ASSERT_MESSAGE(false, string().Format("Failed to find EDerivedFloatMapPersistantStats %d", stat));
		return 0.0f;
	}
}

//---------------------------------------
const char * CPersistantStats::GetDerivedStat(EDerivedStringPersistantStats stat)
{
	CRY_ASSERT(stat > EDSPS_Invalid && stat < EDSPS_Max);
	switch(stat)
	{
	case EDSPS_FavouriteSuitMode:
		{
			float armor = GetStat(EFPS_SuitArmorTime);
			float power = GetStat(EFPS_SuitPowerTime);
			float stealth = GetStat(EFPS_SuitStealthTime);
			float tactical = GetStat(EFPS_SuitTacticalTime);

			CRY_TODO(2010, 03, 02, "Need to get localized suit mode names.");

			if ((armor >= power) && (armor >= stealth) && (armor >= tactical))
				return "Armor";
			else if ((power >= armor) && (power >= stealth) && (power >= tactical))
				return "Power";
			else if ((stealth >= armor) && (stealth >= power) && (stealth >= tactical))
				return "Stealth";
			else	
				return "Tactical";
		}
	default:
		CRY_ASSERT_MESSAGE(false, string().Format("Failed to find EDerivedStringPersistantStats %d", stat));
		return NULL;
	}
}

//---------------------------------------
const char * CPersistantStats::GetDerivedStat(EDerivedStringMapPersistantStats stat)
{
	CRY_ASSERT(stat > EDSMPS_Invalid && stat < EDSMPS_Max);
	static CryFixedStringT<32> returnString;
	switch(stat)
	{
	case EDSMPS_FavouriteGamemode:
		{
			int best = -1;
			int statValue = 0;
			const char * bestGamemode = "";

			IGameRulesModulesManager *pGameRulesModulesManager = CGameRulesModulesManager::GetInstance();
			const int rulesCount = pGameRulesModulesManager->GetRulesCount();
			for(int i = 0; i < rulesCount; i++)
			{
				const char* name = pGameRulesModulesManager->GetRules(i);
				statValue = GetDerivedStat(name, EDIMPS_GamesPlayed);	
				if (statValue > best)
					bestGamemode = name;
			}

			returnString.Format("@ui_rules_%s", bestGamemode);
			return returnString.c_str();
		}
	case EDSMPS_FavouritePrimaryWeapon:
		{
			CRY_TODO(02, 03, 2010, "Make sure all primary weapons are defined here. Perhaps find a way to make it less hardcoded.");
			
			const char *weaponName[] =		{ "SCAR",				"KVolt",			"Feline",				"Marshall",				"Jackal",				"Gauss",			"MK60",				"Grendel",			"LTag",				"Mike",				"DSG1"			};
			const char *uiStringName[] =	{ "@mp_eScar",	"@mp_eKVolt",	"@mp_eFeline",	"@mp_eMarshall",	"@mp_eJackal",	"@mp_eGauss",	"@mp_eMK60",	"@mp_eGrendel",	"@mp_eLTag",	"@mp_eMike",	"@mp_eDSG1"	};

			int best = -1;
			int bestValue = 0;

			int arraySize = ARRAY_COUNT(weaponName);
			for (int i=0; i<arraySize; ++i)
			{
				int statValue = GetStat(weaponName[i],EMPS_WeaponUsage);
				if (best<0 || statValue>bestValue)
				{
					best = i;
					bestValue = statValue;
				}
			}

			if (best>=0)
				return uiStringName[best];
			else
				return NULL;
		}
	default:
		CRY_ASSERT_MESSAGE(false, string().Format("Failed to find EDerivedStringMapPersistantStats %d", stat));
		return NULL;
	}
}

//---------------------------------------
CPersistantStats::SSessionStats::SSessionStats()
{
	Clear();
}

//---------------------------------------
void CPersistantStats::SSessionStats::Add(const SSessionStats* pSessionStats)
{
	for(int i = 0; i < EIPS_Max; i++)
	{
		m_intStats[i] += pSessionStats->m_intStats[i];
	}
	for(int i = 0; i < EFPS_Max; i++)
	{
		m_floatStats[i] += pSessionStats->m_floatStats[i];
	}
	for(int i = 0; i < ESIPS_Max; i++)
	{
		m_streakIntStats[i].m_maxVal = max(m_streakIntStats[i].m_maxVal, pSessionStats->m_streakIntStats[i].m_maxVal);
	}
	for(int i = 0; i < ESFPS_Max; i++)
	{
		m_streakFloatStats[i].m_maxVal = max(m_streakFloatStats[i].m_maxVal, pSessionStats->m_streakFloatStats[i].m_maxVal);
	}
	for(int i = 0; i < EMPS_Max; i++)
	{
		SMap::MapNameToCount::const_iterator it = pSessionStats->m_mapStats[i].m_map.begin();
		SMap::MapNameToCount::const_iterator end = pSessionStats->m_mapStats[i].m_map.end();
		for ( ; it!=end; it++)
		{
			m_mapStats[i].Update(it->first, it->second);
		}
	}
}

void CPersistantStats::SSessionStats::Clear()
{
	memset(&m_intStats, 0, sizeof(m_intStats));
	memset(&m_floatStats, 0, sizeof(m_floatStats));

	memset(&m_streakIntStats, 0, sizeof(m_streakIntStats));
	memset(&m_streakFloatStats, 0, sizeof(m_streakFloatStats));

	const int mapCount = ARRAY_COUNT(m_mapStats);
	for(int i = 0; i < mapCount; i++)
	{
		m_mapStats[i].Clear();
	}
}

//---------------------------------------
int CPersistantStats::SSessionStats::GetStat(EIntPersistantStats stat) const
{
	CRY_ASSERT(stat >= 0 && stat < ARRAY_COUNT(m_intStats));
	return m_intStats[stat];
}

//---------------------------------------
float CPersistantStats::SSessionStats::GetStat(EFloatPersistantStats stat) const
{
	CRY_ASSERT(stat >= 0 && stat < ARRAY_COUNT(m_floatStats));
	return m_floatStats[stat];
}

//---------------------------------------
int CPersistantStats::SSessionStats::GetStat(EStreakIntPersistantStats stat) const
{
	CRY_ASSERT(stat >= 0 && stat < ARRAY_COUNT(m_streakIntStats));
	return m_streakIntStats[stat].m_maxVal;
}

//---------------------------------------
int CPersistantStats::GetCurrentStat(EntityId actorId, EStreakIntPersistantStats stat)
{
	const SSessionStats* pActorStats = GetActorSessionStats(actorId);
	if(pActorStats)
	{
		CRY_ASSERT(stat >= 0 && stat < ARRAY_COUNT(pActorStats->m_streakIntStats));
		return pActorStats->m_streakIntStats[stat].m_curVal;
	}
	return 0;
}

//---------------------------------------
float CPersistantStats::SSessionStats::GetStat(EStreakFloatPersistantStats stat) const
{
	CRY_ASSERT(stat >= 0 && stat < ARRAY_COUNT(m_streakFloatStats));
	return m_streakFloatStats[stat].m_maxVal;
}

//---------------------------------------
int CPersistantStats::SSessionStats::GetStat(const char* name, EMapPersistantStats stat) const
{
	CRY_ASSERT(stat >= 0 && stat < ARRAY_COUNT(m_mapStats));
	return m_mapStats[stat].GetStat(name);
}

//---------------------------------------
int CPersistantStats::SSessionStats::GetDerivedStat(EDerivedIntPersistantStats stat) const
{
	CRY_ASSERT(stat > EDIPS_Invalid && stat < EDIPS_Max);

	switch(stat)
	{
	case EDIPS_GamesWon:
		return m_mapStats[EMPS_GamesWon].GetTotal();

	case EDIPS_GamesDrawn:
		return m_mapStats[EMPS_GamesDrawn].GetTotal();

	case EDIPS_GamesLost:
		return m_mapStats[EMPS_GamesLost].GetTotal();

	case EDIPS_GamesPlayed:
		return GetDerivedStat(EDIPS_GamesWon) + GetDerivedStat(EDIPS_GamesDrawn) + GetDerivedStat(EDIPS_GamesLost);

	case EDIPS_Kills:
		return GetStat(EIPS_KillsSuitPower) + GetStat(EIPS_KillsSuitArmor) + GetStat(EIPS_KillsSuitStealth) + GetStat(EIPS_KillsSuitTactical) + GetStat(EIPS_KillsNoSuit);

	case EDIPS_Deaths:
		return GetStat(EIPS_DeathsSuitPower) + GetStat(EIPS_DeathsSuitArmor) + GetStat(EIPS_DeathsSuitStealth) + GetStat(EIPS_DeathsSuitTactical) + GetStat(EIPS_DeathsNoSuit) + GetStat(EIPS_Suicides);

	case EDIPS_Hits:
		return m_mapStats[EMPS_WeaponHits].GetTotal();

	case EDIPS_Shots:
		return m_mapStats[EMPS_WeaponShots].GetTotal();

	case EDIPS_Misses:
		return GetDerivedStat(EDIPS_Shots) - GetDerivedStat(EDIPS_Hits);

	case EDIPS_Headshots:
		return m_mapStats[EMPS_WeaponHeadshots].GetTotal();

	default:
		CRY_ASSERT_MESSAGE(false, string().Format("Failed to find EDerivedIntPersistantStats %d", stat));
		return 0;
	} }

//---------------------------------------
float CPersistantStats::SSessionStats::GetDerivedStat(EDerivedFloatPersistantStats stat) const
{
	CRY_ASSERT(stat > EDFPS_Invalid && stat < EDFPS_Max);

	switch(stat)
	{
	case EDFPS_WinLoseRatio:
		return GetRatio(GetDerivedStat(EDIPS_GamesWon), GetDerivedStat(EDIPS_GamesLost));

	case EDFPS_KillDeathRatio:
		return GetRatio(GetDerivedStat(EDIPS_Kills), GetDerivedStat(EDIPS_Deaths));

	case EDFPS_Accuracy:
		return GetRatio(GetDerivedStat(EDIPS_Hits), GetDerivedStat(EDIPS_Shots)) * 100.0f; //percentage

	case EDFPS_AliveTime:
		return GetStat(EFPS_SuitPowerTime) + GetStat(EFPS_SuitArmorTime) + GetStat(EFPS_SuitStealthTime) + GetStat(EFPS_SuitTacticalTime) + GetStat(EFPS_NoSuitTime);

	default:
		CRY_ASSERT_MESSAGE(false, string().Format("Failed to find EDerivedFloatPersistantStats %d", stat));
		return 0.0f;
	}
}

//---------------------------------------
CPersistantStats::SSessionStats* CPersistantStats::GetActorSessionStats(EntityId actorId)
{
	ActorSessionMap::iterator it = m_sessionStats.find(actorId);
	if(it != m_sessionStats.end())
	{
		return &it->second;
	}
	else
	{
		SSessionStats stats;
		ActorSessionMap::iterator inserted = m_sessionStats.insert(ActorSessionMap::value_type(actorId, stats)).first;
		return &inserted->second;
	}
}

//---------------------------------------
CPersistantStats::SSessionStats* CPersistantStats::GetClientPersistantStats()
{
	return &m_clientPersistantStats;
}

//---------------------------------------
void CPersistantStats::Update(const float dt)
{
	SSessionStats *pSessionStats = GetClientPersistantStats();
	pSessionStats->m_floatStats[EFPS_Overall] += dt;

	IActor* pClientActor =gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(pClientActor && pClientActor->GetHealth() > 0)
	{
		pSessionStats->m_floatStats[EFPS_SuitPowerTime + GetClientStatSuitIndex()] += dt;
		pSessionStats->m_streakFloatStats[ESFPS_TimeAlive].Increment(dt);

		CPlayer* pClientPlayer = static_cast<CPlayer*>(pClientActor);

		SActorStats* pStats = pClientPlayer->GetActorStats();
		float distance = pStats->velocity.len() * dt;

		if(pStats->inAir > 0.0f)
		{
			pSessionStats->m_floatStats[EFPS_DistanceAir]+= distance;
			pSessionStats->m_streakFloatStats[ESFPS_DistanceAir].Increment(distance);
		}
		else if(pStats->inWaterTimer > 0.0f)
		{
			pSessionStats->m_floatStats[EFPS_DistanceSwum]+= distance;
			pSessionStats->m_streakFloatStats[ESFPS_DistanceAir].Reset();
		}
		else
		{
			pSessionStats->m_floatStats[EFPS_DistanceRan]+= distance;
			pSessionStats->m_streakFloatStats[ESFPS_DistanceAir].Reset();
		}
	}
	else
	{
		pSessionStats->m_streakFloatStats[ESFPS_TimeAlive].Reset();
	}

#ifndef _RELEASE
	if(s_debugSessionStats >= 0 && s_debugSessionStats < m_sessionStats.size())
	{
		ActorSessionMap::const_iterator iter = m_sessionStats.begin();
		std::advance(iter, s_debugSessionStats);

		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(iter->first);
		CryWatch("Stats %d - %s", s_debugSessionStats, pEntity ? pEntity->GetName() : "Invalid Entity");
		debugSessionStats(&iter->second, dt);
	}
	else
	{
		debugSessionStats(GetClientPersistantStats(), dt);
	}
#endif
}

//---------------------------------------
void CPersistantStats::EnteredGame()
{
	m_sessionStats.clear();

	AddListeners();
	
	CGameRules* pGameRules = g_pGame->GetGameRules();
	IGameRulesStateModule *stateModule = pGameRules->GetStateModule();
	if (!stateModule || (stateModule->GetGameState() != IGameRulesStateModule::EGRS_PreGame && stateModule->GetGameState() != IGameRulesStateModule::EGRS_PostGame))
	{
		SSessionStats *pSessionStats = GetClientPersistantStats();
		const char* gamemodeName = pGameRules->GetEntity()->GetClass()->GetName();
		if(gamemodeName)
		{
			pSessionStats->m_mapStats[EMPS_Gamemodes].Update(gamemodeName);
		}

		const char* mapName = gEnv->pGame->GetIGameFramework()->GetLevelName();
		if(mapName)
		{
			pSessionStats->m_mapStats[EMPS_Levels].Update(mapName);
		}
	}
}

void CPersistantStats::ClientDisconnect( EntityId clientId )
{
	if(clientId == gEnv->pGame->GetIGameFramework()->GetClientActorId())
	{
		ClearListeners();
	}
}

//---------------------------------------
void CPersistantStats::GameOver(EGameOverType localWinner)
{
	SSessionStats *pSessionStats = GetClientPersistantStats();
	CGameRules* pGameRules = g_pGame->GetGameRules();
	const char* gamemodeName = pGameRules->GetEntity()->GetClass()->GetName();
	if(gamemodeName)
	{
		switch (localWinner)
		{
		case EGOT_Lose:
			pSessionStats->m_mapStats[EMPS_GamesLost].Update(gamemodeName);
			pSessionStats->m_streakIntStats[ESIPS_Win].Reset();
			pSessionStats->m_streakIntStats[ESIPS_Lose].Increment();
			break;
		case EGOT_Draw:
			pSessionStats->m_mapStats[EMPS_GamesDrawn].Update(gamemodeName);
			pSessionStats->m_streakIntStats[ESIPS_Win].Reset();
			pSessionStats->m_streakIntStats[ESIPS_Lose].Reset();
			break;
		case EGOT_Win:
			pSessionStats->m_mapStats[EMPS_GamesWon].Update(gamemodeName);
			pSessionStats->m_streakIntStats[ESIPS_Win].Increment();
			pSessionStats->m_streakIntStats[ESIPS_Lose].Reset();
			break;
		default:
			CRY_ASSERT_MESSAGE(false, "Unknown EGameOverType - stats won't track exactly");
		}
	}

	PostGame();
}

//---------------------------------------
void CPersistantStats::PostGame()
{
	ClearListeners();
	Save();
}

//---------------------------------------
void CPersistantStats::ClientScoreEvent(EGameRulesScoreType scoreType, int points)
{
	SSessionStats *pSessionStats = GetClientPersistantStats();
	switch(scoreType)
	{
	case EGRST_PlayerKillAssist:
		pSessionStats->m_intStats[EIPS_KillAssists]++;
		break;
	case EGRST_CTF_FlagCompleted:
		pSessionStats->m_intStats[EIPS_FlagCaptures]++;
		break;
	case EGRST_CaptureObjectiveCompleted:
		pSessionStats->m_intStats[EIPS_CaptureObjectives]++;
		break;
	case EGRST_CarryObjectiveCompleted:
		pSessionStats->m_intStats[EIPS_CarryObjectives]++;
		break;
	case EGRST_BombTheBaseCompleted:
		pSessionStats->m_intStats[EIPS_BombTheBase]++;
		break;
	}
}

//---------------------------------------
void CPersistantStats::OnEntityKilled(const HitInfo &hitInfo)
{
	if(hitInfo.shooterId != 0)
	{
		SSessionStats* pShooterStats = GetActorSessionStats(hitInfo.shooterId);
		if(hitInfo.shooterId == hitInfo.targetId)
		{
			pShooterStats->m_intStats[EIPS_Suicides]++;
			pShooterStats->m_streakIntStats[ESIPS_Kills].Reset();
			pShooterStats->m_streakIntStats[ESIPS_Deaths].Increment();
		}
		else
		{
			const char* weaponName = GetItemName(hitInfo.weaponId);
			if(weaponName)
			{
				pShooterStats->m_mapStats[EMPS_WeaponKills].Update(weaponName);
			}
			pShooterStats->m_intStats[EIPS_KillsSuitPower + GetActorStatSuitIndex(hitInfo.shooterId)]++;	//Pass in ActorId
			pShooterStats->m_streakIntStats[ESIPS_Kills].Increment();
			pShooterStats->m_streakIntStats[ESIPS_Deaths].Reset();

			CGameRules* pGameRules = g_pGame->GetGameRules();
			const char* name = pGameRules->GetHitType(hitInfo.type);
			if(name)
			{
				pShooterStats->m_mapStats[EMPS_KillsByDamageType].Update(name);
			}
		}
	}
	
	if(hitInfo.targetId != 0)
	{
		SSessionStats* pTargetStats = GetActorSessionStats(hitInfo.targetId);
		pTargetStats->m_intStats[EIPS_DeathsSuitPower + GetActorStatSuitIndex(hitInfo.targetId)]++;
		pTargetStats->m_streakIntStats[ESIPS_Kills].Reset();
		pTargetStats->m_streakIntStats[ESIPS_Deaths].Increment();
	}
}

//---------------------------------------
void CPersistantStats::OnSetActorItem(IActor *pActor, IItem *pItem )
{
	if(pItem && pActor)
	{
		IWeapon* pWeapon=pItem->GetIWeapon();
		if(pWeapon)
		{
			EntityId actorId = pActor->GetEntityId();
			EntityId weaponId = pItem->GetEntityId();
			SetNewWeaponListener(pWeapon, weaponId, actorId);
			const char* weaponName = GetItemName(weaponId);
			if(weaponName)
			{
				GetActorSessionStats(actorId)->m_mapStats[EMPS_WeaponUsage].Update(weaponName);
			}
		}
	}
}

//---------------------------------------
void CPersistantStats::SetNewWeaponListener(IWeapon* pWeapon, EntityId weaponId, EntityId actorId)
{
	ActorWeaponListenerMap::iterator it = m_actorWeaponListener.find(actorId);
	if(it != m_actorWeaponListener.end())
	{
		//remove previous weapon listener for actor
		RemoveWeaponListener(it->second);
		//update with new weapon
		it->second = weaponId;
	}
	else
	{
		//aren't listener so add actor and weapon
		m_actorWeaponListener.insert(ActorWeaponListenerMap::value_type(actorId, weaponId));
	}
	
	pWeapon->AddEventListener(this, "CPersistantStats");
	
}

//---------------------------------------
void CPersistantStats::RemoveWeaponListener(EntityId weaponId)
{
	IItem* pItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(weaponId);
	if(pItem)
	{
		IWeapon *pWeapon = pItem->GetIWeapon();
		if(pWeapon)
		{
			pWeapon->RemoveEventListener(this);
		}
	}
}

//---------------------------------------
void CPersistantStats::RemoveAllWeaponListeners()
{
	ActorWeaponListenerMap::const_iterator it = m_actorWeaponListener.begin();
	ActorWeaponListenerMap::const_iterator end = m_actorWeaponListener.end();
	for ( ; it!=end; it++)
	{
		RemoveWeaponListener(it->second);
	}

	m_actorWeaponListener.clear();
}

//---------------------------------------
void CPersistantStats::OnShoot(IWeapon *pWeapon, EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3 &vel)
{
	if(shooterId != 0)
	{
		const char* weaponName = GetActorItemName(shooterId);
		if(weaponName)
		{
			GetActorSessionStats(shooterId)->m_mapStats[EMPS_WeaponShots].Update(weaponName);
		}
	}
}

//---------------------------------------
void CPersistantStats::ClientHit(CPlayer *pPlayer, const HitInfo& hitInfo)
{
	if(hitInfo.targetId != 0 && hitInfo.damage > 0)
	{
		const char* weaponName = GetItemName(hitInfo.weaponId);
		if(weaponName)
		{
			SSessionStats *pSessionStats = GetClientPersistantStats();
			pSessionStats->m_mapStats[EMPS_WeaponHits].Update(weaponName);

			CGameRules* pGameRules = g_pGame->GetGameRules();
			if(pPlayer && pPlayer->IsHeadShot(hitInfo))
			{
				pSessionStats->m_mapStats[EMPS_WeaponHeadshots].Update(weaponName);
			}
		}
	}
}

//---------------------------------------
void CPersistantStats::AddListeners()
{
	CGameRules* pGameRules = g_pGame->GetGameRules();
	pGameRules->RegisterKillListener(this);
	pGameRules->RegisterClientScoreListener(this);
}

//---------------------------------------
void CPersistantStats::ClearListeners()
{
	CGameRules* pGameRules = g_pGame->GetGameRules();
	if(pGameRules)
	{
		pGameRules->UnRegisterKillListener(this);
		pGameRules->UnRegisterClientScoreListener(this);
	}
	RemoveAllWeaponListeners();
}

//---------------------------------------
void CPersistantStats::OnEvent(EPPType type, bool skillKill)
{
	SSessionStats *pSessionStats = GetClientPersistantStats();
	switch(type)
	{
		case EPP_TeamRadar:
			pSessionStats->m_intStats[EIPS_TeamRadar]++;
			break;
		case EPP_EMPStrike:
			pSessionStats->m_intStats[EIPS_EMPStrike]++;
			break;
		case EPP_SatStrike:
			pSessionStats->m_intStats[EIPS_SatStrike]++;
			break;
		case EPP_SuitBoost:
			pSessionStats->m_intStats[EIPS_SuitBoost]++;
			break;
		case EPP_DogtagCollection:
			pSessionStats->m_intStats[EIPS_DogtagsCollected]++;
			break;
		case EPP_FirstBlood:
			pSessionStats->m_intStats[EIPS_FirstBlood]++;
			break;
	}

	if(skillKill)
	{
		pSessionStats->m_intStats[EIPS_SkillKills]++;
	}
}

//---------------------------------------
int CPersistantStats::GetClientStatSuitIndex()
{
	IActor* pClientActor =gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(pClientActor)
	{
		return GetActorStatSuitIndex(pClientActor);
	}

	return eNanoSuitMode_Last;
}

//---------------------------------------
int CPersistantStats::GetActorStatSuitIndex(EntityId actorId)
{
	IActor* pActor =gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(actorId);
	if(pActor)
	{
		return GetActorStatSuitIndex(pActor);
	}

	return eNanoSuitMode_Last;	
}

//---------------------------------------
int CPersistantStats::GetActorStatSuitIndex(IActor* pActor)
{
	CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
	if(pPlayer->HasNanoSuit())
	{
		ENanoSuitMode mode = pPlayer->GetActorSuitGameParameters().GetMode();
		CRY_ASSERT_MESSAGE(mode >= 0 && mode < eNanoSuitMode_Last, "Invalid suitmode from client actor");
		return mode;
	}

	return eNanoSuitMode_Last;
}

//---------------------------------------
const char* CPersistantStats::GetActorItemName(EntityId actorId)
{
	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(actorId);
	if(pActor)
	{
		IItem *pItem = pActor->GetCurrentItem();
		if(pItem)
		{
			return GetItemName(pItem);
		}
	}
	return NULL;
}

//---------------------------------------
const char* CPersistantStats::GetItemName(EntityId weaponId)
{
	IItem *pItem=gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(weaponId);
	if(pItem)
	{
		return GetItemName(pItem);
	}
	return NULL;
}

//---------------------------------------
const char* CPersistantStats::GetItemName(IItem* pItem)
{
	return pItem->GetEntity()->GetClass()->GetName();
}

//static---------------------------------------
float CPersistantStats::GetRatio(int a, int b)
{
	float ratio = 0.0f;
	if(b > 0)
	{
		ratio = a / (float) b;
	}
	return ratio;
}

//---------------------------------------
#ifndef _RELEASE
void CPersistantStats::debugSessionStats(const SSessionStats *pSession, const float dt)
{
	//Just a way of cycling through the stats every 4 seconds
	s_debugTimer += (dt * 0.25f);
	int debugStats = s_debugPersistantStats;
	if(debugStats == 9)
	{
		debugStats = (((int)s_debugTimer) % 6) + 1;
	}

	if(debugStats > 0)
	{
		CryWatch("DebugStats %d", debugStats);
	}

	if(debugStats == 1)
	{
		for(int i = 0; i < EIPS_Max; i++)
		{
			CryWatch("%s - %d", s_intPersistantNames[i], pSession->GetStat((EIntPersistantStats) i));
		}
	}
	else if(debugStats == 2)
	{
		for(int i = 0; i < EFPS_Max; i++)
		{
			CryWatch("%s - %.2f", s_floatPersistantNames[i], pSession->GetStat((EFloatPersistantStats) i));
		}
	}
	else if(debugStats == 3)
	{
		for(int i = 0; i < EDIPS_Max; i++)
		{
			CryWatch("%s - %d", s_intDerivedPersistantNames[i], pSession->GetDerivedStat((EDerivedIntPersistantStats) i));
		}
	}
	else if(debugStats == 4)
	{
		for(int i = 0; i < EDFPS_Max; i++)
		{
			CryWatch("%s - %.2f", s_floatDerivedPersistantNames[i], pSession->GetDerivedStat((EDerivedFloatPersistantStats) i));
		}
	}
	else if(debugStats == 5)
	{
		for(int i = 0; i < ESIPS_Max; i++)
		{
			CryWatch("%s - %d (%d)", s_streakIntPersistantNames[i], pSession->m_streakIntStats[i].m_maxVal, pSession->m_streakIntStats[i].m_curVal);
		}
	}
	else if(debugStats == 6)
	{
		for(int i = 0; i < ESFPS_Max; i++)
		{
			CryWatch("%s - %.2f (%.2f)", s_streakFloatPersistantNames[i], pSession->m_streakFloatStats[i].m_maxVal, pSession->m_streakFloatStats[i].m_curVal);
		}
	}
	else if(debugStats == 7)
	{
		for(int i = 0; i < EMPS_Max; i++)
		{
			CryWatch("%s", s_mapPersistantNames[i]);
			pSession->m_mapStats[(EMapPersistantStats) i].watch();
		}
	}
}


void CPersistantStats::SMap::watch() const
{
	MapNameToCount::const_iterator it = m_map.begin();
	MapNameToCount::const_iterator end = m_map.end();
	for ( ; it!=end; it++)
	{
		CryWatch("\t%s - %d", it->first.c_str(), it->second);
	}
}
#endif

//----------------------------------------------------------
template <class T>
CPersistantStats::SStreak<T>::SStreak()
{
	m_curVal = 0;
	m_maxVal = 0;
}

//----------------------------------------------------------
template <class T>
void CPersistantStats::SStreak<T>::Increment(T inc)
{
	T newVal = m_curVal + inc;
	m_maxVal = max(m_maxVal,newVal);
	m_curVal = newVal;
}

//----------------------------------------------------------
template<class T>
void CPersistantStats::SStreak<T>::Increment()
{
	T newVal = m_curVal + 1;
	m_maxVal = max(m_maxVal,newVal);
	m_curVal = newVal;
}

//----------------------------------------------------------
template <class T>
void CPersistantStats::SStreak<T>::Reset()
{
	m_curVal = 0;
}

//----------------------------------------------------------
CPersistantStats::SMap::SMap()
{
	Clear();
}

//----------------------------------------------------------
void CPersistantStats::SMap::Clear()
{
	m_map.clear();
}

//----------------------------------------------------------
void CPersistantStats::SMap::Update(const char* name, int amount)
{
	MapNameToCount::iterator it = FindOrInsert(name);
	it->second += amount;
}

//----------------------------------------------------------
CPersistantStats::SMap::MapNameToCount::iterator CPersistantStats::SMap::FindOrInsert(const char* name)
{
	MapNameToCount::iterator it = m_map.find(name);
	if (it == m_map.end())
	{
		MapNameToCount::iterator inserted = m_map.insert(MapNameToCount::value_type(name, 0)).first;
		return inserted;
	}
	else
	{
		return it;
	}
}

//----------------------------------------------------------
int CPersistantStats::SMap::GetStat(const char* name) const
{
	MapNameToCount::const_iterator it = m_map.find(name);
	if(it != m_map.end())
	{
		return it->second;
	}
	return 0;
}

int CPersistantStats::SMap::GetTotal() const
{
	int total = 0;
	MapNameToCount::const_iterator it = m_map.begin();
	MapNameToCount::const_iterator end = m_map.end();
	for ( ; it!=end; it++)
	{
		total += it->second;
	}
	return total;
}

//----------------------------------------------------------
void CPersistantStats::SMap::Save(const char* name, IPlayerProfile* pProfile) const
{
	string data = "";
	MapNameToCount::const_iterator it = m_map.begin();
	MapNameToCount::const_iterator end = m_map.end();
	for ( ; it!=end; it++)
	{
		data.append(string().Format("%s,%d,", it->first.c_str(), it->second));
	}
	pProfile->SetAttribute(string().Format("MP/PersistantStats/%s", name), data);
}

//----------------------------------------------------------
void CPersistantStats::SMap::Load(const char* name, const IPlayerProfile* pProfile)
{
	string data = "";
	pProfile->GetAttribute(string().Format("MP/PersistantStats/%s", name), data);
	int start = 0, middle = 0, end = 0;
	while((middle = data.find(',', start)) != -1 && (end = data.find(',', middle + 1)) != -1)
	{
		string substr = data.substr(start, middle - start);
		string subnum = data.substr(middle + 1, end - (middle + 1));

		int num = atoi(subnum);
		m_map.insert(MapNameToCount::value_type(substr.c_str(), num));

		start = end + 1;
	}
}

//----------------------------------------------------------
void CPersistantStats::SendStatsToFlash(IFlashPlayer *pFlashPlayer)
{
	CRY_ASSERT(pFlashPlayer);

	// ADD_STATS_STRINGS define requires these structures
	TPersistantStatsStringsArray fixedArrayStats;	// Max num stats sent in 1 group set here
	SPersistantStatsStrings statsStrings;

	TStringsPushArray pushArray;
	pushArray.reserve(fixedArrayStats.max_size());

	//-----------------------
	// Defines

#define ADD_STATS_STRINGS(stat) \
		GetStatStrings(stat, &statsStrings); \
		fixedArrayStats.push_back(statsStrings)
	
#define ADD_MAP_STATS_STRINGS(stat, name, paramString) \
	GetStatStrings(name, stat, paramString, &statsStrings); \
	fixedArrayStats.push_back(statsStrings)

#define ADD_GAMEMODE_STATS_STRINGS(name) \
	ADD_MAP_STATS_STRINGS(EDIMPS_GamesPlayed, name, "@ui_rules_" name); \
	ADD_MAP_STATS_STRINGS(EMPS_GamesWon, name, "@ui_rules_" name); \
	ADD_MAP_STATS_STRINGS(EMPS_GamesLost, name, "@ui_rules_" name); \
	ADD_MAP_STATS_STRINGS(EMPS_GamesDrawn, name, "@ui_rules_" name);

#define ADD_WEAPON_STATS_STRINGS(name, uiString) \
	ADD_MAP_STATS_STRINGS(EMPS_WeaponKills, name, uiString); \
	ADD_MAP_STATS_STRINGS(EDFMPS_Accuracy, name, uiString); \
	ADD_MAP_STATS_STRINGS(EMPS_WeaponHeadshots, name, uiString);
	
	//-----------------------

	// General -----------------------
	SetupSendStatsGroupToFlash(pFlashPlayer, fixedArrayStats, "@ui_menu_stats_group_general");
		ADD_STATS_STRINGS(EFPS_Overall);
		ADD_STATS_STRINGS(EDFPS_AliveTime); // Not needed
		// Rank
		// XP earned
		ADD_STATS_STRINGS(EDIPS_GamesPlayed);
		ADD_STATS_STRINGS(EDIPS_GamesWon);
		ADD_STATS_STRINGS(EDIPS_GamesLost);
		ADD_STATS_STRINGS(EDIPS_GamesDrawn);
		ADD_STATS_STRINGS(ESIPS_Win);
		ADD_STATS_STRINGS(ESIPS_Lose);
		ADD_STATS_STRINGS(EDFPS_WinLoseRatio); // Not local?
		ADD_STATS_STRINGS(EDIPS_Kills);	// Not local?
		ADD_STATS_STRINGS(EDIPS_Deaths);
		ADD_STATS_STRINGS(EDFPS_KillDeathRatio); // Not local?
		ADD_STATS_STRINGS(EIPS_KillsSuitPower);
		ADD_STATS_STRINGS(EIPS_KillsSuitArmor);
		ADD_STATS_STRINGS(EIPS_KillsSuitStealth);
		ADD_STATS_STRINGS(EIPS_KillsSuitTactical);
		ADD_STATS_STRINGS(EIPS_KillsNoSuit);	// Not needed?
		ADD_STATS_STRINGS(EIPS_DeathsSuitPower);
		ADD_STATS_STRINGS(EIPS_DeathsSuitArmor);
		ADD_STATS_STRINGS(EIPS_DeathsSuitStealth);
		ADD_STATS_STRINGS(EIPS_DeathsSuitTactical);
		ADD_STATS_STRINGS(EIPS_DeathsNoSuit);	// Not needed?
		ADD_STATS_STRINGS(EIPS_KillAssists);
		// Enemies blinded
		ADD_STATS_STRINGS(EDIPS_Headshots);
		ADD_STATS_STRINGS(EIPS_SkillKills);
		// Melee kills
		// Grenade kills
		// Explosive charge kills
		ADD_STATS_STRINGS(EIPS_DogtagsCollected);
		// Dog tags surrendered
		// Medals earned
		// Average life expectancy
		ADD_STATS_STRINGS(EDIPS_Shots);
		ADD_STATS_STRINGS(EDIPS_Hits);
		ADD_STATS_STRINGS(EDIPS_Misses);
		ADD_STATS_STRINGS(EDFPS_Accuracy); // Not local?
		ADD_STATS_STRINGS(ESIPS_Kills); // Not local
		ADD_STATS_STRINGS(ESFPS_TimeAlive);
		ADD_STATS_STRINGS(ESIPS_Deaths);
		// Shortest alive time
		// Players tagged on CCTV
		// Times tagged on CCTV
		ADD_STATS_STRINGS(EFPS_DistanceRan);
		ADD_STATS_STRINGS(EFPS_DistanceSwum);
		ADD_STATS_STRINGS(EFPS_DistanceAir); // Not needed?
		ADD_STATS_STRINGS(EIPS_Suicides);
		// Highest score in single round
		// Mosts kills in one round
		// Most deaths in one round
		ADD_STATS_STRINGS(ESFPS_DistanceAir); // Longest fall height?
		// Favourite map
		// Least favourite map
		ADD_STATS_STRINGS(EIPS_FirstBlood);
		// Final blood
		// Times reincarnated
	SendStatsGroupToFlash(pFlashPlayer, fixedArrayStats, pushArray);
	
	// Team -----------------------
	SetupSendStatsGroupToFlash(pFlashPlayer, fixedArrayStats, "@ui_menu_stats_group_team");
		ADD_STATS_STRINGS(EIPS_TeamRadar);
		ADD_STATS_STRINGS(EIPS_EMPStrike);
		// Suits stressed
		ADD_STATS_STRINGS(EIPS_SatStrike);
		// Air strike kills
		ADD_STATS_STRINGS(EIPS_SuitBoost);
		// Fatality bonus acquired
	SendStatsGroupToFlash(pFlashPlayer, fixedArrayStats, pushArray);

	// Suit Usage -----------------------
	SetupSendStatsGroupToFlash(pFlashPlayer, fixedArrayStats, "@ui_menu_stats_group_suitusage");
		// Favourite suit mode
		ADD_STATS_STRINGS(EDSPS_FavouriteSuitMode);
		ADD_STATS_STRINGS(EFPS_SuitPowerTime); // Active / Passive?
		ADD_STATS_STRINGS(EFPS_SuitArmorTime); // Active / Passive?
		ADD_STATS_STRINGS(EFPS_SuitStealthTime); // Active / Passive?
		ADD_STATS_STRINGS(EFPS_SuitTacticalTime); // Active / Passive?
		ADD_STATS_STRINGS(EFPS_NoSuitTime); // Not needed?
		// Favourite suit module
	SendStatsGroupToFlash(pFlashPlayer, fixedArrayStats, pushArray);

	// Vehicle -----------------------
	SetupSendStatsGroupToFlash(pFlashPlayer, fixedArrayStats, "@ui_menu_stats_group_vehicle");
		// Vehicle stats - some DLC
	SendStatsGroupToFlash(pFlashPlayer, fixedArrayStats, pushArray);

	// Weapons -----------------------
	SetupSendStatsGroupToFlash(pFlashPlayer, fixedArrayStats, "@ui_menu_stats_group_weapon");
		CRY_TODO(2010, 03, 02, "Make sure all weapons that want stats are accounted for. Make sure strings exist and match too.");
		ADD_STATS_STRINGS(EDSMPS_FavouritePrimaryWeapon);		// Make sure primary weapons defined in GetDerivedStat() are accurate.
		ADD_WEAPON_STATS_STRINGS("Feline", "@mp_eFeline");
		ADD_WEAPON_STATS_STRINGS("Marshall", "@mp_eMarshall");
		ADD_WEAPON_STATS_STRINGS("Jackal", "@mp_eJackal");
		ADD_WEAPON_STATS_STRINGS("SCAR", "@mp_eSCAR");
		ADD_WEAPON_STATS_STRINGS("SCARAB", "@mp_eSCARAB");
		ADD_WEAPON_STATS_STRINGS("Grendel", "@mp_eGrendel");
		ADD_WEAPON_STATS_STRINGS("Gauss", "@mp_eGauss");
		ADD_WEAPON_STATS_STRINGS("MK60", "@mp_eMK60");
		ADD_WEAPON_STATS_STRINGS("LTag", "@mp_eLTag");
		ADD_WEAPON_STATS_STRINGS("MIKE", "@mp_eMIKE");
		ADD_WEAPON_STATS_STRINGS("K-Volt", "@mp_eKVolt");
		ADD_WEAPON_STATS_STRINGS("DSG1", "@mp_eDSG1");
		ADD_WEAPON_STATS_STRINGS("Nova", "@mp_eNova");
		ADD_WEAPON_STATS_STRINGS("RazorFive", "@mp_eRazor");
		ADD_WEAPON_STATS_STRINGS("Hammer", "@mp_eHammer");
		ADD_WEAPON_STATS_STRINGS("Majecstic", "@mp_eMajestic");
		ADD_WEAPON_STATS_STRINGS("FragGrenades", "@mp_eFragGrenade");
		ADD_WEAPON_STATS_STRINGS("C4", "@mp_eC4");
		ADD_WEAPON_STATS_STRINGS("HMG", "@mp_eHMG");
		ADD_WEAPON_STATS_STRINGS("VolatileSpike", "@mp_eVolatileSpike");
	SendStatsGroupToFlash(pFlashPlayer, fixedArrayStats, pushArray);

	// Gamemodes -----------------------
	SetupSendStatsGroupToFlash(pFlashPlayer, fixedArrayStats, "@ui_menu_stats_group_gamemode");
		ADD_STATS_STRINGS(EDSMPS_FavouriteGamemode);
		ADD_GAMEMODE_STATS_STRINGS("InstantAction");
		ADD_GAMEMODE_STATS_STRINGS("TeamInstantAction");
		ADD_GAMEMODE_STATS_STRINGS("CaptureTheFlag");
		ADD_STATS_STRINGS(EIPS_FlagCaptures);
		ADD_GAMEMODE_STATS_STRINGS("CrashSite");
		ADD_GAMEMODE_STATS_STRINGS("Assault");
		ADD_GAMEMODE_STATS_STRINGS("Extraction");
		//ADD_GAMEMODE_STATS_STRINGS("BombTheBase");		// DLC
		//ADD_STATS_STRINGS(EIPS_BombTheBase);					// DLC
		//ADD_GAMEMODE_STATS_STRINGS("AllOrNothing");		// DLC
		//ADD_GAMEMODE_STATS_STRINGS("PowerStruggle");	// DLC
	SendStatsGroupToFlash(pFlashPlayer, fixedArrayStats, pushArray);

#undef ADD_MAP_STATS_STRINGS
#undef ADD_STATS_STRINGS
}

//----------------------------------------------------------
void CPersistantStats::SetupSendStatsGroupToFlash(IFlashPlayer *pFlashPlayer, TPersistantStatsStringsArray &fixedStringsArray, const char *name)
{
	fixedStringsArray.clear();
	pFlashPlayer->Invoke1(FRONTEND_SUBSCREEN_PATH_SET("UpdateStatsGroup"),SFlashVarValue(name));
}

//----------------------------------------------------------
void CPersistantStats::SendStatsGroupToFlash(IFlashPlayer *pFlashPlayer, TPersistantStatsStringsArray &fixedStringsArray, TStringsPushArray &pushArray)
{
	pushArray.clear();

	int size = fixedStringsArray.size();
	if(size)
	{
		for (int i(0); i<size; ++i)
		{
			pushArray.push_back(fixedStringsArray[i].m_title.c_str());
			pushArray.push_back(fixedStringsArray[i].m_value.c_str());
		}

		pFlashPlayer->SetVariableArray(FVAT_ConstStrPtr, FRONTEND_SUBSCREEN_PATH_SET("m_allValues"), 0, &pushArray[0], pushArray.size());
	}
	pFlashPlayer->Invoke0(FRONTEND_SUBSCREEN_PATH_SET("UpdateStatsGroupDone"));
}


//----------------------------------------------------------
void CPersistantStats::GetStatStrings(EIntPersistantStats stat, SPersistantStatsStrings *statsStrings)
{
	statsStrings->m_title.Format("@ui_menu_stats_%s", s_intPersistantNames[(int)stat]);
	
	// Add a switch statement when specific strings are required e.g. 3 "miles"
	statsStrings->m_value.Format("%d", GetStat(stat));
}

//----------------------------------------------------------
void CPersistantStats::GetStatStrings(EFloatPersistantStats stat, SPersistantStatsStrings *statsStrings)
{
	statsStrings->m_title.Format("@ui_menu_stats_%s", s_floatPersistantNames[(int)stat]);

	float statValue = GetStat(stat);

	switch (stat)
	{
		case EFPS_DistanceAir:	// deliberate fall throughs
		case EFPS_DistanceRan:
		case EFPS_DistanceSwum:
				statsStrings->m_value.Format("%.2f m", statValue);
			break;
		case EFPS_Overall:
		case EFPS_SuitPowerTime:	// deliberate fall throughs
		case EFPS_SuitArmorTime:
		case EFPS_SuitStealthTime:
		case EFPS_SuitTacticalTime:
		case EFPS_NoSuitTime:
				statsStrings->m_value.Format("%s", GetTimeString(statValue));
			break;
		default:
				statsStrings->m_value.Format("%.2f", statValue);
			break;
	};
}

//----------------------------------------------------------
void CPersistantStats::GetStatStrings(EStreakIntPersistantStats stat, SPersistantStatsStrings *statsStrings)
{
	statsStrings->m_title.Format("@ui_menu_stats_%s", s_streakIntPersistantNames[(int)stat]);
	statsStrings->m_value.Format("%d", GetStat(stat));
}

//----------------------------------------------------------
void CPersistantStats::GetStatStrings(EStreakFloatPersistantStats stat, SPersistantStatsStrings *statsStrings)
{
	statsStrings->m_title.Format("@ui_menu_stats_%s", s_streakFloatPersistantNames[(int)stat]);

	float statValue = GetStat(stat);

	switch (stat)
	{
		case ESFPS_TimeAlive:	// deliberate fall throughs
				statsStrings->m_value.Format("%s", GetTimeString(statValue));
			break;
		case ESFPS_DistanceAir:
				statsStrings->m_value.Format("%.2f m", statValue);
			break;
		default:
				statsStrings->m_value.Format("%.2f", statValue);
			break;
	};
}

//----------------------------------------------------------
void CPersistantStats::GetStatStrings(EDerivedIntPersistantStats stat, SPersistantStatsStrings *statsStrings)
{
	statsStrings->m_title.Format("@ui_menu_stats_%s", s_intDerivedPersistantNames[(int)stat]);
	statsStrings->m_value.Format("%d", GetDerivedStat(stat));
}

//----------------------------------------------------------
void CPersistantStats::GetStatStrings(EDerivedFloatPersistantStats stat, SPersistantStatsStrings *statsStrings)
{
	statsStrings->m_title.Format("@ui_menu_stats_%s", s_floatDerivedPersistantNames[(int)stat]);

	float statValue = GetDerivedStat(stat);

	switch (stat)
	{
		case EDFPS_Accuracy:
				statsStrings->m_value.Format("%.2f %%", MIN(statValue,100.f));
			break;
		case EDFPS_AliveTime:
				statsStrings->m_value.Format("%s", GetTimeString(statValue));
			break;
		default:
				statsStrings->m_value.Format("%.2f", statValue);
			break;
	};
}

//----------------------------------------------------------
void CPersistantStats::GetStatStrings(EDerivedStringPersistantStats stat, SPersistantStatsStrings *statsStrings)
{
	statsStrings->m_title.Format("@ui_menu_stats_%s", s_stringDerivedPersistantNames[(int)stat]);
	statsStrings->m_value = GetDerivedStat(stat);
}

//----------------------------------------------------------
void CPersistantStats::GetStatStrings(EDerivedStringMapPersistantStats stat, SPersistantStatsStrings *statsStrings)
{
	statsStrings->m_title.Format("@ui_menu_stats_%s", s_stringMapDerivedPersistantNames[(int)stat]);
	statsStrings->m_value = GetDerivedStat(stat);
}

//----------------------------------------------------------
void CPersistantStats::GetStatStrings(const char* name, EDerivedIntMapPersistantStats stat, const char *paramString, SPersistantStatsStrings *statsStrings)
{
	CHUD* pHUD = g_pGame->GetHUD();

	CryFixedStringT<32> localizedString;
	localizedString.Format("@ui_menu_stats_%s", s_intMapDerivedPersistantNames[(int)stat]);

	if (pHUD)
	{
		CryFixedStringT<32> localizedName;
		localizedName = pHUD->LocalizeString(paramString);
		statsStrings->m_title = pHUD->LocalizeString(localizedString.c_str(), localizedName.c_str());
	}

	int statValue = GetDerivedStat(name, stat);

	statsStrings->m_value.Format("%d", statValue);
}

//----------------------------------------------------------
void CPersistantStats::GetStatStrings(const char* name, EDerivedFloatMapPersistantStats stat, const char *paramString, SPersistantStatsStrings *statsStrings)
{
	CHUD* pHUD = g_pGame->GetHUD();

	CryFixedStringT<32> localizedString;
	localizedString.Format("@ui_menu_stats_%s", s_floatMapDerivedPersistantNames[(int)stat]);

	if (pHUD)
	{
		CryFixedStringT<32> localizedName;
		localizedName = pHUD->LocalizeString(paramString);
		statsStrings->m_title = pHUD->LocalizeString(localizedString.c_str(), localizedName.c_str());
	}

	float statValue = GetDerivedStat(name, stat);

	switch (stat)
	{
		case EDFMPS_Accuracy:
				statsStrings->m_value.Format("%.2f %%", statValue);
			break;
		default:
				statsStrings->m_value.Format("%.2f", statValue);
			break;
	};
}

//----------------------------------------------------------
void CPersistantStats::GetStatStrings(const char* name, EMapPersistantStats stat, const char *paramString, SPersistantStatsStrings *statsStrings)
{
	CHUD* pHUD = g_pGame->GetHUD();

	CryFixedStringT<32> localizedString;
	localizedString.Format("@ui_menu_stats_%s", s_mapPersistantNames[(int)stat]);

	if (pHUD)
	{
		CryFixedStringT<32> localizedName;
		localizedName = pHUD->LocalizeString(paramString);
		statsStrings->m_title = pHUD->LocalizeString(localizedString.c_str(), localizedName.c_str());
	}

	int statValue = GetStat(name, stat);

	statsStrings->m_value.Format("%d", statValue);
}
