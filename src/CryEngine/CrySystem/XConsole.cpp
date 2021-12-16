// XConsole.cpp: implementation of the CXConsole class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "XConsole.h"
#include "XConsoleVariable.h"
#include "System.h"
#include "ConsoleBatchFile.h"
#include "StringUtils.h"

#include <IInput.h>
#include <ITimer.h>
#include <IScriptSystem.h>
#include <IInput.h>
#include <IRenderer.h>
#include <INetwork.h>     // EvenBalance - M.Quinn
#include <ISystem.h>
#include <ILog.h>
#include <IProcess.h>
#include <IHardwareMouse.h>
#include "ConsoleHelpGen.h"			// CConsoleHelpGen

// EvenBalance - M. Quinn




//#define CRYSIS_BETA

#ifdef CRYSIS_BETA
static inline bool AllowedForBeta(const string& cmd)
{
	if (gEnv->pSystem->IsDedicated())
		return true;
	static const char* commands[] = {"name", "connect", "quit"}; // a list of allowed beta commands; workaround ... but for beta test only
	static const size_t ncommands = sizeof(commands) / sizeof(char*);
	string temp = cmd; temp.MakeLower();
	for (size_t i = 0; i < ncommands; ++i) // slow ... but for beta test only
		if (temp == commands[i])
			return true;
	return false;
}
#endif

static inline void AssertName( const char *szName )
{
#ifdef _DEBUG
	assert(szName);

	// test for good console variable / command name
	const char *p = szName;
	bool bFirstChar=true;

	while(*p)
	{
		assert((*p>='a' && *p<='z') 
				|| (*p>='A' && *p<='Z') 
				|| (*p>='0' && *p<='9' && !bFirstChar) 
				|| *p=='_'
				|| *p=='.');

		++p;bFirstChar=false;
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
// user defined comparison - for nicer printout
inline int GetCharPrio( char x ) 
{
	if(x>='a' && x<='z')
		x+='A'-'a';					// make upper case

	if(x=='_')return 300;
		else return x;
}
// case sensitive
inline bool less_CVar( const char* left,const char* right )
{
	for(;;)
	{
		uint32 l=GetCharPrio(*left), r=GetCharPrio(*right);

		if(l<r)
			return true;
		if(l>r)
			return false;

		if(*left==0 || *right==0)
			break;
	
		++left;++right;
	}

	return false;
}

void Command_SetWaitSeconds(IConsoleCmdArgs* pCmd)
{
	CXConsole * pConsole = (CXConsole *)gEnv->pConsole;

	// This command is only functional in the deferred execution mode
	if (!pConsole->m_deferredExecution)
		return;

	if (pCmd->GetArgCount() > 1)
	{
		pConsole->m_waitSeconds.SetSeconds(atof(pCmd->GetArg(1)));
		pConsole->m_waitSeconds += gEnv->pTimer->GetFrameStartTime();
	}
}

void Command_SetWaitFrames(IConsoleCmdArgs* pCmd)
{
	CXConsole * pConsole = (CXConsole *)gEnv->pConsole;

	// This command is only functional in the deferred execution mode
	if (!pConsole->m_deferredExecution)
		return;

	if (pCmd->GetArgCount() > 1)
		pConsole->m_waitFrames = max(0, atoi(pCmd->GetArg(1)));
}

/*

  CNotificationNetworkConsole

*/

#include <INotificationNetwork.h>
class CNotificationNetworkConsole :
	public INotificationNetworkListener,
	public IOutputPrintSink
{
private:
	static const uint32 LENGTH_MAX = 256;

public:
	static CNotificationNetworkConsole &Instance()
	{
		static CNotificationNetworkConsole instance;
		return instance;
	}

private:
	CNotificationNetworkConsole()
	{
		m_pNotificationNetwork = NULL;
		m_pConsole = NULL;

		m_commandBuffer[0][0] = NULL;
		m_commandBuffer[1][0] = NULL;
		m_commandBufferIndex = 0;
		m_commandCriticalSection = ::CryCreateCriticalSection();
	}

	~CNotificationNetworkConsole()
	{
		if (m_commandCriticalSection)
			::CryDeleteCriticalSection(m_commandCriticalSection);
	}

public:
	void ProcessCommand()
	{
		if (!ValidateConsole())
			return;

		char *command = NULL;
		::CryEnterCriticalSection(m_commandCriticalSection);
		if (*m_commandBuffer[m_commandBufferIndex])
		{
			command = m_commandBuffer[m_commandBufferIndex];
		}
		++m_commandBufferIndex &= 1;
		::CryLeaveCriticalSection(m_commandCriticalSection);

		if (command)
		{
			m_pConsole->ExecuteString(command);
			*command = NULL;
		}
	}

private:
	bool ValidateConsole()
	{
		if (m_pConsole)
			return true;

		if (!gEnv->pConsole)
			return false;

		m_pConsole = gEnv->pConsole;
		m_pConsole->AddOutputPrintSink(this);
		return true;
	}

	// INotificationNetworkListener
public:
	void OnNotificationNetworkReceive(const void *pBuffer, size_t length)
	{
		if (!ValidateConsole())
			return;

		if (length > LENGTH_MAX)
			length = LENGTH_MAX;

		::CryEnterCriticalSection(m_commandCriticalSection);
		::memcpy(m_commandBuffer[m_commandBufferIndex], pBuffer, length);
		m_commandBuffer[m_commandBufferIndex][LENGTH_MAX-1] = '\0';
		::CryLeaveCriticalSection(m_commandCriticalSection);
	}

	// IOutputPrintSink
public:
	void Print(const char *text)
	{
		if (!m_pNotificationNetwork)
			m_pNotificationNetwork = gEnv->pSystem->GetINotificationNetwork();
		if (!m_pNotificationNetwork)
			return;
//		if (m_pNotificationNetwork)
//			m_pNotificationNetwork->Send("Log", text, ::strlen(text)+1);
	}

private:
	INotificationNetwork *m_pNotificationNetwork;
	IConsole *m_pConsole;

	char m_commandBuffer[2][LENGTH_MAX];
	size_t m_commandBufferIndex;
	void *m_commandCriticalSection;
};














































































































































































































































































void ConsoleShow( IConsoleCmdArgs* )
{
  gEnv->pConsole->ShowConsole(true);
}
void ConsoleHide( IConsoleCmdArgs* )
{
  gEnv->pConsole->ShowConsole(false);
}

void Bind( IConsoleCmdArgs* cmdArgs)
{
	int count = cmdArgs->GetArgCount();

	if (cmdArgs->GetArgCount() >= 3)
	{
		string arg;
		for (int i = 2; i < cmdArgs->GetArgCount(); ++i)
		{
			arg += cmdArgs->GetArg(i);
			arg += " ";
		}
		gEnv->pConsole->CreateKeyBind(cmdArgs->GetArg(1), arg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////
int CXConsole::con_display_last_messages = 0;
int CXConsole::con_line_buffer_size = 500;
int CXConsole::con_showonload = 0;
int CXConsole::con_debug = 0;
int CXConsole::con_restricted = 1;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CXConsole::CXConsole()
{
	m_fRepeatTimer=0;
	m_pSysDeactivateConsole=0;
	m_pFont=NULL;
	m_pRenderer=NULL;
	m_pNetwork=NULL; // EvenBalance - M. Quinn
	m_pInput=NULL;
	m_pImage=NULL;
	m_nCursorPos=0;
	m_nScrollPos=0;
	m_nScrollMax=300;
	m_nTempScrollMax=m_nScrollMax;
	m_nScrollLine=0;
	m_nHistoryPos=-1;
	m_nTabCount=0;
	m_bConsoleActive=false;
	m_bActivationKeyEnable=true;
	m_sdScrollDir = sdNONE;
	m_pSystem=NULL;
	m_bDrawCursor = true;
	m_fCursorBlinkTimer=0;

	m_bStaticBackground=false;
	m_nProgress = 0;
	m_nProgressRange = 0;
	m_nLoadingBackTexID = 0;
	m_nWhiteTexID = 0;

	m_deferredExecution = false;
	m_waitFrames = 0;
	m_waitSeconds = 0.0f;
	m_blockCounter = 0;

	if (INotificationNetwork *pNotificationNetwork =
		gEnv->pSystem->GetINotificationNetwork())
	{
		pNotificationNetwork->ListenerBind("Command",
			&CNotificationNetworkConsole::Instance());
	}







}


//////////////////////////////////////////////////////////////////////////
CXConsole::~CXConsole()
{
	if(!m_mapVariables.empty())
	{
		while (!m_mapVariables.empty())
			m_mapVariables.begin()->second->Release();

		m_mapVariables.clear();
	}
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::FreeRenderResources()
{
  if (m_pRenderer)
  {
		if (m_nLoadingBackTexID)
		{
      m_pRenderer->RemoveTexture(m_nLoadingBackTexID);
			m_nLoadingBackTexID = -1;
		}
		if (m_nWhiteTexID)
		{
      m_pRenderer->RemoveTexture(m_nWhiteTexID);
			m_nWhiteTexID = -1;
		}
    if (m_pImage)
      m_pImage->Release();
  }
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::Release()
{
	delete this;
}





#if !defined(XENON) && !defined(PS3) && !defined(LINUX)
void Command_DumpCommandsVars(IConsoleCmdArgs* Cmd)
{
	const char *arg = "";

	if(Cmd->GetArgCount()>1) 
		arg = Cmd->GetArg(1);

	CXConsole *pConsole = (CXConsole *)gEnv->pConsole;

	// txt
	pConsole->DumpCommandsVarsTxt(arg);

	// HTML
	{
		CConsoleHelpGen Generator(*pConsole);

		Generator.Work();
	}
}
#endif

//////////////////////////////////////////////////////////////////////////
void CXConsole::Init(CSystem *pSystem)
{
	m_pSystem=pSystem;
	if (pSystem->GetICryFont())
		m_pFont=pSystem->GetICryFont()->GetFont("console");
	m_pRenderer=pSystem->GetIRenderer();
	m_pNetwork=gEnv->pNetwork;  // EvenBalance - M. Quinn
	m_pInput=pSystem->GetIInput();   
	m_pTimer=pSystem->GetITimer();
	
	if (m_pInput)
	{
		// Assign this class as input listener.
		m_pInput->AddConsoleEventListener( this );
	}
	
	m_pSysDeactivateConsole = REGISTER_INT("sys_DeactivateConsole",0,0,
		"0: normal console behavior\n"
		"1: hide the console");

	REGISTER_CVAR(con_display_last_messages,0,VF_NULL,"");  // keep default at 1, needed for gameplay
	REGISTER_CVAR(con_line_buffer_size,1000,VF_NULL,"");
	REGISTER_CVAR(con_showonload,0,VF_NULL,"Show console on level loading");
	REGISTER_CVAR(con_debug,0,VF_CHEAT,"Log call stack on every GetCVar call");
	REGISTER_CVAR(con_restricted,con_restricted,VF_RESTRICTEDMODE,"0=normal mode / 1=restricted access to the console");				// later on VF_RESTRICTEDMODE should be removed (to 0)

	if(m_pSystem->IsDevMode()					// unrestricted console for -DEVMODE
	|| m_pSystem->IsDedicated())			// unrestricted console for dedicated server
		con_restricted=0;

	// test cases -----------------------------------------------
#ifdef CRYSIS_BETA
	// no test for BETA test
#else
	assert(GetCVar("con_debug")!=0);		// should be registered a few lines above
	assert(GetCVar("Con_Debug")==GetCVar("con_debug"));		// different case

	// editor
	assert(strcmp(AutoComplete("con_"),"con_debug")==0);
	assert(strcmp(AutoComplete("CON_"),"con_debug")==0);
	assert(strcmp(AutoComplete("con_debug"),"con_display_last_messages")==0);		// actually we should reconsider this behavior
	assert(strcmp(AutoComplete("Con_Debug"),"con_display_last_messages")==0);		// actually we should reconsider this behavior

	// game
	assert(strcmp(ProcessCompletion("con_"),"con_debug ")==0);
	ResetAutoCompletion();
	assert(strcmp(ProcessCompletion("CON_"),"con_debug ")==0);
	ResetAutoCompletion();
	assert(strcmp(ProcessCompletion("con_debug"),"con_debug ")==0);
	ResetAutoCompletion();
	assert(strcmp(ProcessCompletion("Con_Debug"),"con_debug ")==0);
	ResetAutoCompletion();
#endif

	// ----------------------------------------------------------

	if (m_pRenderer)
	{
		m_nWhiteTexID = -1;

		ITexture *pTex = 0;		

		// This texture is already loaded by the renderer. It's ref counted so there is no wasted space.
		pTex = pSystem->GetIRenderer()->EF_LoadTexture("Textures/Defaults/White.dds", FT_DONT_STREAM|FT_DONT_RELEASE, eTF_Unknown);
		if (pTex)
      m_nWhiteTexID = pTex->GetTextureID();
	}
	else
	{
		m_nLoadingBackTexID = -1;
		m_nWhiteTexID = -1;
	} 
	if(pSystem && pSystem->IsDedicated())
			m_bConsoleActive = true;

  REGISTER_COMMAND("ConsoleShow",&ConsoleShow,VF_NULL,"" );
  REGISTER_COMMAND("ConsoleHide",&ConsoleHide,VF_NULL,"" );

#if !defined(XENON) && !defined(PS3) && !defined(LINUX)
	REGISTER_COMMAND("DumpCommandsVars",&Command_DumpCommandsVars,VF_NULL,
		"This console command dumps all console variables and commands to disk\n"
		"DumpCommandsVars [prefix]");
#endif

	REGISTER_COMMAND("Bind", &Bind,VF_NULL,"");
	REGISTER_COMMAND("wait_seconds", &Command_SetWaitSeconds, VF_BLOCKFRAME,
		"Forces the console to wait for a given number of seconds before the next deferred command is processed\n"
		"Works only in deferred command mode");
	REGISTER_COMMAND("wait_frames", &Command_SetWaitFrames, VF_BLOCKFRAME,
		"Forces the console to wait for a given number of frames before the next deferred command is processed\n"
		"Works only in deferred command mode");

	CConsoleBatchFile::Init();
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::RegisterVar( ICVar *pCVar,ConsoleVarFunc pChangeFunc )
{
	// first register callback so setting the value from m_configVars
	// is calling pChangeFunc         (that would be more correct but to not introduce new problems this code was not changed)
//	if (pChangeFunc)
//		pCVar->SetOnChangeCallback(pChangeFunc);

	ConfigVars::iterator it = m_configVars.find( CONST_TEMP_STRING(pCVar->GetName()) );
	if (it != m_configVars.end())
	{
		pCVar->Set( it->second );
		pCVar->SetFlags( pCVar->GetFlags()|VF_WASINCONFIG );
		// Remove value.
		m_configVars.erase(it);
	}
	else
	{
		// Variable is not modified when just registered.
		pCVar->ClearFlags(VF_MODIFIED);
	}

	if (pChangeFunc)
		pCVar->SetOnChangeCallback(pChangeFunc);

	m_mapVariables.insert(ConsoleVariablesMapItor::value_type(pCVar->GetName(),pCVar));
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::LoadConfigVar( const char *sVariable,const char *sValue )
{
	ICVar *pCVar = GetCVar(sVariable);
	if (pCVar)
	{
		pCVar->Set( sValue );
		pCVar->SetFlags( pCVar->GetFlags()|VF_WASINCONFIG );
		return;
	}

	m_configVars[sVariable] = sValue;
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::EnableActivationKey(bool bEnable)
{
	m_bActivationKeyEnable = bEnable;
}

//////////////////////////////////////////////////////////////////////////
ICVar *CXConsole::Register(const char *sName, int *src, int iValue, int nFlags, const char *help,ConsoleVarFunc pChangeFunc )
{
	AssertName(sName);

	ICVar *pCVar = stl::find_in_map( m_mapVariables,sName,NULL );
	if (pCVar)
		return pCVar;

	pCVar=new CXConsoleVariableIntRef(this,sName,src,nFlags,help);
	*src = iValue;
	RegisterVar( pCVar,pChangeFunc );
	return pCVar;
}


//////////////////////////////////////////////////////////////////////////
ICVar *CXConsole::RegisterCVarGroup( const char *szName, const char *szFileName )
{
	AssertName(szName);
	assert(szFileName);

	// suppress cvars not starting with sys_spec_ as
	// cheaters might create cvars before we created ours
	if(strnicmp(szName,"sys_spec_",9)!=0)
		return 0;

	ICVar *pCVar = stl::find_in_map( m_mapVariables,szName,NULL );
	if (pCVar)
	{
		assert(0);			// groups should be only registered once
		return pCVar;
	}

	CXConsoleVariableCVarGroup *pCVarGroup=new CXConsoleVariableCVarGroup(this,szName,szFileName,VF_COPYNAME);

	pCVar = pCVarGroup;

	RegisterVar( pCVar,CXConsoleVariableCVarGroup::OnCVarChangeFunc);
/*
	// log to file - useful during development - might not be required for final shipping
	{
		string sInfo = pCVarGroup->GetDetailedInfo();

		gEnv->pLog->LogToFile("CVarGroup %s",sInfo.c_str());
		gEnv->pLog->LogToFile(" ");
	}
*/
	return pCVar;
}

//////////////////////////////////////////////////////////////////////////
ICVar *CXConsole::Register(const char *sName, float *src, float fValue, int nFlags, const char *help,ConsoleVarFunc pChangeFunc )
{
	AssertName(sName);

	ICVar *pCVar = stl::find_in_map( m_mapVariables,sName,NULL );
	if (pCVar)
		return pCVar;

	pCVar = new CXConsoleVariableFloatRef(this,sName,src,nFlags,help);
	*src = fValue;
	RegisterVar( pCVar,pChangeFunc );
	return pCVar;
}


//////////////////////////////////////////////////////////////////////////
ICVar *CXConsole::Register(const char *sName, const char** src, const char* defaultValue, int nFlags, const char *help,ConsoleVarFunc pChangeFunc )
{
	AssertName(sName);

	ICVar *pCVar = stl::find_in_map( m_mapVariables,sName,NULL );
	if (pCVar)
		return pCVar;

	pCVar = new CXConsoleVariableStringRef(this,sName,src,defaultValue,nFlags,help);
	RegisterVar( pCVar,pChangeFunc );
	return pCVar;
}


//////////////////////////////////////////////////////////////////////////
ICVar *CXConsole::RegisterString(const char *sName,const char *sValue,int nFlags, const char *help,ConsoleVarFunc pChangeFunc )
{
	AssertName(sName);

	ICVar *pCVar = stl::find_in_map( m_mapVariables,sName,NULL );
	if (pCVar)
		return pCVar;

	pCVar = new CXConsoleVariableString(this,sName,sValue,nFlags,help);
	RegisterVar( pCVar,pChangeFunc );
	return pCVar;
}

//////////////////////////////////////////////////////////////////////////
ICVar *CXConsole::RegisterFloat(const char *sName,float fValue,int nFlags, const char *help,ConsoleVarFunc pChangeFunc )
{
	AssertName(sName);

	ICVar *pCVar = stl::find_in_map( m_mapVariables,sName,NULL );
	if (pCVar)
		return pCVar;

	pCVar=new CXConsoleVariableFloat(this,sName,fValue,nFlags,help);
	RegisterVar( pCVar,pChangeFunc );
	return pCVar;
}

//////////////////////////////////////////////////////////////////////////
ICVar *CXConsole::RegisterInt(const char *sName,int iValue,int nFlags, const char *help,ConsoleVarFunc pChangeFunc )
{
	AssertName(sName);

	ICVar *pCVar = stl::find_in_map( m_mapVariables,sName,NULL );
	if (pCVar)
		return pCVar;
	
	pCVar=new CXConsoleVariableInt(this,sName,iValue,nFlags,help);
	RegisterVar( pCVar,pChangeFunc );
	return pCVar;
}



//////////////////////////////////////////////////////////////////////////
void CXConsole::UnregisterVariable(const char *sVarName,bool bDelete)
{
	ConsoleVariablesMapItor itor;
	itor=m_mapVariables.find(sVarName);

	if(itor==m_mapVariables.end())
		return;

	ICVar *pCVar=itor->second;

	m_mapVariables.erase(sVarName);

	if(bDelete)
		pCVar->Release();
}


//////////////////////////////////////////////////////////////////////////
void CXConsole::SetScrollMax(int value)
{
	m_nScrollMax=value;
	m_nTempScrollMax=m_nScrollMax;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CXConsole::SetImage(ITexture *pImage,bool bDeleteCurrent)
{
	if (bDeleteCurrent)	
	{
    pImage->Release();
	}
		
	m_pImage=pImage;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void	CXConsole::ShowConsole(bool show, const int iRequestScrollMax )
{
	if(m_pSysDeactivateConsole->GetIVal())
		show=false;

	if(show && !m_bConsoleActive)
	{
		if(gEnv->pHardwareMouse)
		{
			gEnv->pHardwareMouse->IncrementCounter();
		}
	}
	else if(!show && m_bConsoleActive)
	{
		if(gEnv->pHardwareMouse)
		{
			gEnv->pHardwareMouse->DecrementCounter();	
		}
	}

	SetStatus(show);

	if(iRequestScrollMax>0)
		m_nTempScrollMax=iRequestScrollMax;			// temporary user request
	 else
		m_nTempScrollMax=m_nScrollMax;					// reset

	if(m_bConsoleActive) 
    m_sdScrollDir=sdDOWN;		
	 else 
    m_sdScrollDir=sdUP;	
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::CreateKeyBind(const char *sCmd,const char *sRes)
{
	m_mapBinds.insert(ConsoleBindsMapItor::value_type(sCmd, sRes));
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::DumpKeyBinds( IKeyBindDumpSink *pCallback )
{
	for (ConsoleBindsMap::iterator it = m_mapBinds.begin(); it != m_mapBinds.end(); ++it)
	{
		pCallback->OnKeyBindFound( it->first.c_str(),it->second.c_str() );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* CXConsole::FindKeyBind( const char *sCmd ) const
{
	ConsoleBindsMap::const_iterator it = m_mapBinds.find(CONST_TEMP_STRING(sCmd));

	if (it != m_mapBinds.end())
		return it->second.c_str();

	return 0;
}


//////////////////////////////////////////////////////////////////////////
void CXConsole::DumpCVars(ICVarDumpSink *pCallback,unsigned int nFlagsFilter)
{
	ConsoleVariablesMapItor It=m_mapVariables.begin();

	while (It!=m_mapVariables.end())
	{
		if((nFlagsFilter==0) || ((nFlagsFilter!=0) && (It->second->GetFlags()&nFlagsFilter) ))
			pCallback->OnElementFound(It->second);
		++It;
	}
}

//////////////////////////////////////////////////////////////////////////
ICVar *CXConsole::GetCVar( const char *sName )
{
	assert(this);
	assert(sName);

	if (con_debug)
	{
		// Log call stack on get cvar.
		CryLog( "GetCVar(\"%s\") called",sName );
		m_pSystem->debug_LogCallStack();
	}

	// Fast map lookup for case-sensitive match.
	ConsoleVariablesMapItor it;
	
	it=m_mapVariables.find(sName);
	if(it!=m_mapVariables.end())
		return it->second;

/*
	if(!bCaseSensitive)
	{
		// Much slower but allows names with wrong case (use only where performance doesn't matter).
		for(it=m_mapVariables.begin(); it!=m_mapVariables.end(); ++it)
		{
			if(stricmp(it->first,sName)==0)
				return it->second;
		}
	}
	test else
	{
		for(it=m_mapVariables.begin(); it!=m_mapVariables.end(); ++it)
		{
			if(stricmp(it->first,sName)==0)
			{
				CryFatalError("Error: Wrong case for '%s','%s'",it->first,sName);
			}
		}
	}
*/

	return NULL;		// haven't found this name
}

//////////////////////////////////////////////////////////////////////////
char *CXConsole::GetVariable( const char * szVarName, const char * szFileName, const char * def_val )
{
	assert( m_pSystem );
	return 0;
}

//////////////////////////////////////////////////////////////////////////
float CXConsole::GetVariable( const char * szVarName, const char * szFileName, float def_val )
{
	assert( m_pSystem );
	return 0;
}


//////////////////////////////////////////////////////////////////////////
bool CXConsole::GetStatus()
{
	return m_bConsoleActive;
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::Clear()
{
	m_dqConsoleBuffer.clear();
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::Update()
{
	// Repeat GetIRenderer (For Editor).
	if (!m_pSystem)
		return;

	// Execute the deferred commands
	ExecuteDeferredCommands();

	m_pRenderer = m_pSystem->GetIRenderer();

	if(!m_bConsoleActive)
		m_nRepeatEvent.keyId = eKI_Unknown;

	// Process Key press repeat (backspace and cursor on PC, all keys on PS3)
	if(m_nRepeatEvent.keyId != eKI_Unknown)
	{
		const float fRepeatDelay = 1.0f/40.0f;			// in sec (similar to Windows default but might differ from actual setting)

		m_fRepeatTimer -= gEnv->pTimer->GetRealFrameTime();											// works even when time is manipulated
//		m_fRepeatTimer -= gEnv->pTimer->GetFrameTime(ITimer::ETIMER_UI);			// can be used once ETIMER_UI works even with t_FixedTime

		if(m_fRepeatTimer<=0.0f)
		{
			if(m_fRepeatTimer<-fRepeatDelay)
			{
				// bad framerate or hitch
				m_nRepeatEvent.keyId = eKI_Unknown;
			}
			else
			{
				ProcessInput(m_nRepeatEvent);
				m_fRepeatTimer=fRepeatDelay;			// next repeat even in .. sec
			}
		}
	}

	CNotificationNetworkConsole::Instance().ProcessCommand();








}

//////////////////////////////////////////////////////////////////////////
bool CXConsole::OnInputEvent( const SInputEvent &event )
{
	// Process input event
	ConsoleBindsMapItor itorBind;

	if (event.state == eIS_Released && m_bConsoleActive)
		m_nRepeatEvent.keyId = eKI_Unknown;

	if (event.state != eIS_Pressed)
		return false;

	// restart cursor blinking
	m_fCursorBlinkTimer=0.0f;
	m_bDrawCursor=true;

	// key repeat
	const float fStartRepeatDelay = 0.5f;						// in sec (similar to Windows default but might differ from actual setting)
	m_nRepeatEvent = event;
	m_fRepeatTimer=fStartRepeatDelay;

	//execute Binds
	if(!m_bConsoleActive)
	{
		const char* cmd=0;

		if(event.modifiers==0)
		{
			// fast
			cmd = FindKeyBind(event.keyName);
		}
		else
		{
			// slower
			char szCombinedName[40];
			int iLen=0;

			if(event.modifiers&eMM_Ctrl)
				{ strcpy(szCombinedName,				"ctrl ");	iLen+=5; }
			if(event.modifiers&eMM_Shift)
				{ strcpy(&szCombinedName[iLen],	"shift ");iLen+=6; }
			if(event.modifiers&eMM_Alt)
				{ strcpy(&szCombinedName[iLen],	"alt ");	iLen+=4; }
			if(event.modifiers&eMM_Win)
				{ strcpy(&szCombinedName[iLen],	"win ");	iLen+=4; }

			strcpy(&szCombinedName[iLen],event.keyName);

			cmd = FindKeyBind(szCombinedName);
		}

		if(cmd)
		{
			SetInputLine("");
			ExecuteStringInternal(cmd,true);		// keybinds are treated as they would come from console
		}
	}
	else
	{
		if (event.keyId != eKI_Tab)
			ResetAutoCompletion();

		if (event.keyId == eKI_V && (event.modifiers & eMM_Ctrl) != 0) 
		{
			Paste();
			return false;
		}

		if (event.keyId == eKI_C && (event.modifiers & eMM_Ctrl) != 0) 
		{
			Copy();
			return false;
		}
	}

	if (event.keyId == eKI_Tilde)
	{
		if(m_bActivationKeyEnable)
		{
			m_sInputBuffer="";
      m_nCursorPos=0;			
      m_pInput->ClearKeyState();	
			ShowConsole(!GetStatus());
      m_nRepeatEvent.keyId = eKI_Unknown;
			return true;
		}		
	}
	if (event.keyId == eKI_Escape)
	{
		//switch process or page or other things
		m_sInputBuffer="";
		m_nCursorPos=0;
		if (m_pSystem)
		{
			if (!m_bConsoleActive)
			{
				//m_pSystem->GetIGame()->SendMessage("Switch");
			}
			ShowConsole(false);

			ISystemUserCallback *pCallback = ((CSystem*)m_pSystem)->GetUserCallback();
			if (pCallback)
				pCallback->OnProcessSwitch();
		}
		return false;
	}

	return ProcessInput(event);
}

//////////////////////////////////////////////////////////////////////////
bool CXConsole::OnInputEventUI( const SInputEvent &event )
{
	if (!m_bConsoleActive)
		return false;

	AddInputChar( event.keyName[0] );

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CXConsole::ProcessInput(const SInputEvent& event)
{
	if (!m_bConsoleActive)
		return false;

	// this is not so super-nice as the XKEY's ... but a small price to pay
	// if speed is a problem (which would be laughable for this) the CCryName
	// can be cached in a static var
	if (event.keyId == eKI_Enter || event.keyId == eKI_NP_Enter)
	{
		ExecuteInputBuffer();
		m_nScrollLine=0; 
		return true;
	}
	else if (event.keyId == eKI_Backspace)
	{
		RemoveInputChar(true);
		return true;
	}
	else if (event.keyId == eKI_Left)
	{
		if(m_nCursorPos)
			m_nCursorPos--;
		return true;
	}
	else if (event.keyId == eKI_Right)
	{
		if(m_nCursorPos<(int)(m_sInputBuffer.length()))
			m_nCursorPos++;
		return true;
	}
	else if (event.keyId == eKI_Up)
	{
		const char *szHistoryLine=GetHistoryElement(true);		// true=UP

		if(szHistoryLine)
		{
			m_sInputBuffer=szHistoryLine;
			m_nCursorPos=(int)m_sInputBuffer.size();
		}
		return true;
	}
	else if (event.keyId == eKI_Down)
	{
		const char *szHistoryLine=GetHistoryElement(false);		// false=DOWN

		if(szHistoryLine)
		{
			m_sInputBuffer=szHistoryLine;
			m_nCursorPos=(int)m_sInputBuffer.size();
		}
		return true;
	}
	else if (event.keyId == eKI_Tab)
	{
		if (!(event.modifiers & eMM_Alt))
		{
			m_sInputBuffer = ProcessCompletion(m_sInputBuffer.c_str());
			m_nCursorPos = m_sInputBuffer.size();
		}
		return true;
	}
	else if (event.keyId == eKI_PgUp || event.keyId == eKI_MouseWheelUp)
	{
		if(event.modifiers&eMM_Ctrl)
		{
			m_nScrollLine = min((int)(m_dqConsoleBuffer.size()-1), m_nScrollLine + 21);
		}
		else
		{
			m_nScrollLine = min((int)(m_dqConsoleBuffer.size()-1), m_nScrollLine + 1);
		}
		return true;
	}
	else if (event.keyId == eKI_PgDn || event.keyId == eKI_MouseWheelDown)
	{
		if(event.modifiers&eMM_Ctrl)
		{
			m_nScrollLine = max(0, m_nScrollLine - 21);
		}
		else
		{
			m_nScrollLine = max(0, m_nScrollLine - 1);
		}
		return true;
	}
	else if (event.keyId == eKI_Home)
	{
		if(event.modifiers&eMM_Ctrl)
		{
			m_nScrollLine = m_dqConsoleBuffer.size()-1;
		}
		else
		{
			m_nCursorPos = 0;
		}
		return true;
	}
	else if (event.keyId == eKI_End)
	{
		if(event.modifiers&eMM_Ctrl)
		{
			m_nScrollLine = 0;
		}
		else
		{
			m_nCursorPos=(int)m_sInputBuffer.length();
		}
		return true;
	}
	else if (event.keyId == eKI_Delete)
	{
		RemoveInputChar(false);
		return true;
	}
	else 
	{
	#if !defined(PS3)
		if (gEnv->pSystem->IsEditor())
	#endif
		{
			const char*sKeyName = m_pInput->GetKeyName(event, 1);

			if (sKeyName)
			{
				if(strlen(sKeyName)!=1)
					return true;
				AddInputChar( sKeyName[0] );
				return true;
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* CXConsole::GetHistoryElement( const bool bUpOrDown )
{
	if(bUpOrDown)
	{
		if(!m_dqHistory.empty())
		{
			if(m_nHistoryPos<(int)(m_dqHistory.size()-1))
			{
				m_nHistoryPos++;
				m_sReturnString = m_dqHistory[m_nHistoryPos];
				return m_sReturnString.c_str();
			}
		}
	}
	else
	{		
		if(m_nHistoryPos>0)
		{
			m_nHistoryPos--;
			m_sReturnString = m_dqHistory[m_nHistoryPos];
			return m_sReturnString.c_str();
		}
	}

	return 0;
}



//////////////////////////////////////////////////////////////////////////
void CXConsole::Draw()
{
  //ShowConsole(true);
	if(!m_pSystem || !m_nTempScrollMax)
		return;

	if(!m_pRenderer)
	{
		// For Editor.
		m_pRenderer = m_pSystem->GetIRenderer();
	}

	if(!m_pRenderer)
		return;

	if (!m_pFont)
	{
		// For Editor.
		ICryFont *pICryFont=m_pSystem->GetICryFont();

		if(pICryFont)
			m_pFont= m_pSystem->GetICryFont()->GetFont("default");
	}

	ScrollConsole();


















	if (m_nScrollPos<=0)
	{
//#ifdef _DEBUG
		DrawBuffer(70, "console");
//#endif
		return;
	} 

	// cursor blinking
	{
		m_fCursorBlinkTimer += gEnv->pTimer->GetRealFrameTime();											// works even when time is manipulated
//	m_fCursorBlinkTimer += gEnv->pTimer->GetFrameTime(ITimer::ETIMER_UI);					// can be used once ETIMER_UI works even with t_FixedTime

		const float fCursorBlinkDelay = 0.5f;						// in sec (similar to Windows default but might differ from actual setting)

		if(m_fCursorBlinkTimer>fCursorBlinkDelay)
		{
			m_bDrawCursor=!m_bDrawCursor;
			m_fCursorBlinkTimer=0.0f;
		}
	}

  int nPrevMode = m_pRenderer->SetWireframeMode(R_SOLID_MODE);

	if (!m_nProgressRange)
	{
		if (m_bStaticBackground)
		{
      m_pRenderer->SetState(GS_NODEPTHTEST);
			m_pRenderer->Draw2dImage(0,0,800,600,m_pImage?m_pImage->GetTextureID():m_nWhiteTexID, 0.0f, 1.0f, 1.0f, 0.0f);
		}
		else
		{
			m_pRenderer->Set2DMode(true,m_pRenderer->GetWidth(),m_pRenderer->GetHeight());

			float fReferenceSize = 600.0f;

			float fSizeX = (float)m_pRenderer->GetWidth();
			float fSizeY = m_nTempScrollMax * m_pRenderer->GetHeight() / fReferenceSize;

			m_pRenderer->SetState(GS_NODEPTHTEST|GS_BLSRC_SRCALPHA|GS_BLDST_ONEMINUSSRCALPHA);
			m_pRenderer->DrawImage(0,0,fSizeX,fSizeY,m_nWhiteTexID,0.0f,0.0f,1.0f,1.0f,0.0f,0.0f,0.0f,0.7f);
			m_pRenderer->DrawImage(0,fSizeY,fSizeX,2.0f*m_pRenderer->GetHeight()/fReferenceSize,m_nWhiteTexID,0,0,0,0,0.0f,0.0f,0.0f,1.0f);

			m_pRenderer->Set2DMode(false,0,0);
		}
	}
/*#else
		if(m_bStaticBackground)
			m_pRenderer->Draw2dImage(0,(float)(m_nScrollPos-m_nTempScrollMax),800,(float)m_nTempScrollMax,m_pImage->GetTextureID(),4.0f,2.0f);	
		else
			m_pRenderer->Draw2dImage(0,0,800,600,m_pImage->GetTextureID());
#endif		*/


  // draw progress bar
  if(m_nProgressRange)
  {
    m_pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
		m_pRenderer->Draw2dImage(0.0, 0.0, 800.0f, 600.0f, m_nLoadingBackTexID, 0.0f, 1.0f, 1.0f, 0.0f);
  }

  //if (!m_bStaticBackground)
	DrawBuffer(m_nScrollPos, "console");
  m_pRenderer->SetWireframeMode(nPrevMode);
}

void CXConsole::DrawBuffer(int nScrollPos, const char *szEffect)
{
	if(!m_bConsoleActive && con_display_last_messages == 0)
		return;
	
	if (m_pFont && m_pRenderer)
  {
	  m_pFont->Reset();
	  m_pFont->SetEffect(szEffect);
	  m_pFont->SetProportional(false);
		m_pFont->SetCharWidthScale(0.5f);
	  m_pFont->SetSize(vector2f(14, 14));
	  m_pFont->SetColor(ColorF(1,1,1,1));

		m_pFont->SetSizeIn800x600(false);
		float csize = 0.8f * m_pFont->GetCharHeight();
		m_pFont->SetSizeIn800x600(true);

		float yPos = nScrollPos-csize-3.0f;

		//calculate overscan border for title safe area
		Vec2 overscanBorders = *(Vec2*)gEnv->pRenderer->EF_Query(EFQ_OverscanBorders);
		float xDelta = /*((float)gEnv->pRenderer->GetWidth())*/800.0f * overscanBorders.x;
		//x position considering overscan borders
		float xPos = LINE_BORDER + xDelta;

		float fCharWidth=(m_pFont->GetCharWidth() * m_pFont->GetCharWidthScale());
		
		//int ypos=nScrollPos-csize-3;	

		//Draw the input line
		if(m_bConsoleActive && !m_nProgressRange)
    {
      /*m_pRenderer->DrawString(xPos-nCharWidth, yPos, false, ">");
      m_pRenderer->DrawString(xPos, yPos, false, m_sInputBuffer.c_str());
		  if(m_bDrawCursor)
			  m_pRenderer->DrawString(xPos+nCharWidth*m_nCursorPos, yPos, false, "_");*/

      m_pFont->DrawString((float)(xPos-fCharWidth), (float)yPos, ">");
			m_pFont->DrawString((float)xPos, (float)yPos, m_sInputBuffer.c_str(),false);
		  
			if(m_bDrawCursor)
			{
				string szCursorLeft(m_sInputBuffer.c_str(), m_sInputBuffer.c_str() + m_nCursorPos);
				int n = m_pFont->GetTextLength(szCursorLeft.c_str(), false);

			  m_pFont->DrawString((float)(xPos+(fCharWidth * n)), (float)yPos, "_");
			}
		}
		
		yPos-=csize;
		
		ConsoleBufferRItor ritor;
		ritor=m_dqConsoleBuffer.rbegin();
		int nScroll=0;
		while(ritor!=m_dqConsoleBuffer.rend() && yPos>=0)  
		{
			if(nScroll>=m_nScrollLine)
			{
				const char *buf=ritor->c_str();// GetBuf(k);
				
				if(*buf>0 && *buf<32) buf++;		// to jump over verbosity level character

				if (yPos+csize>0) 
  			  m_pFont->DrawString((float)xPos, (float)yPos, buf,false);
				yPos-=csize;
			}
			nScroll++;
			
			++ritor;
		} //k		
  }
}


//////////////////////////////////////////////////////////////////////////
bool CXConsole::GetLineNo( const int indwLineNo, char *outszBuffer, const int indwBufferSize ) const
{
	assert(outszBuffer);
	assert(indwBufferSize>0);

	outszBuffer[0]=0;

	ConsoleBuffer::const_reverse_iterator ritor = m_dqConsoleBuffer.rbegin();
  
	ritor+=indwLineNo;

	if(indwLineNo>=(int)m_dqConsoleBuffer.size())
		return false;

	const char *buf=ritor->c_str();// GetBuf(k);
	
	if(*buf>0 && *buf<32) buf++;		// to jump over verbosity level character

	strncpy(outszBuffer,buf,indwBufferSize-1);
	outszBuffer[indwBufferSize-1]=0;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////
int CXConsole::GetLineCount() const
{
	return m_dqConsoleBuffer.size();
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::ScrollConsole()
{
	if(!m_pRenderer)
		return;
  
  int nCurrHeight=m_pRenderer->GetHeight();
  
  switch (m_sdScrollDir)
  {
/////////////////////////////////
		case sdDOWN: // The console is scrolling down
      
      // Vlads note: console should go down immediately, otherwise it can look very bad on startup
      //m_nScrollPos+=nCurrHeight/2;			
      m_nScrollPos = m_nTempScrollMax;
      
      if (m_nScrollPos>m_nTempScrollMax)
      {
        m_nScrollPos = m_nTempScrollMax;
        m_sdScrollDir = sdNONE;
      }
      break;
/////////////////////////////////      
    case sdUP: // The console is scrolling up
      
      m_nScrollPos-=nCurrHeight;//2;			
      
      if (m_nScrollPos<0)
      {
        m_nScrollPos = 0;
        m_sdScrollDir = sdNONE;				
      }
      break;
/////////////////////////////////      
    case sdNONE: 
			break;
/////////////////////////////////
  }
}


//////////////////////////////////////////////////////////////////////////
void CXConsole::AddCommand( const char *sCommand, ConsoleCommandFunc func,int nFlags, const char *sHelp )
{
	AssertName(sCommand);

	CConsoleCommand cmd;
	cmd.m_sName = sCommand;
	cmd.m_func = func;
	if (sHelp)
		cmd.m_sHelp = sHelp;
	cmd.m_nFlags = nFlags;
	m_mapCommands.insert(std::make_pair(cmd.m_sName, cmd));
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::AddCommand( const char *sCommand, const char *sScriptFunc,int nFlags, const char *sHelp )
{
	AssertName(sCommand);

	CConsoleCommand cmd;
	cmd.m_sName = sCommand;
	cmd.m_sCommand = sScriptFunc;
	if (sHelp)
		cmd.m_sHelp = sHelp;
	cmd.m_nFlags = nFlags;
	m_mapCommands.insert(std::make_pair(cmd.m_sName, cmd));
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::RemoveCommand(const char *sName)
{
	ConsoleCommandsMap::iterator ite = m_mapCommands.find(sName);
	if (ite != m_mapCommands.end())
		m_mapCommands.erase(ite);
}

//////////////////////////////////////////////////////////////////////////
inline bool hasprefix(const char *s, const char *prefix)
{
	while(*prefix) if(*prefix++!=*s++) return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////
const char* CXConsole::GetFlagsString( const uint32 dwFlags )
{
	static char sFlags[256];

// hiding this makes it a bit more difficult for cheaters
//	if(dwFlags&VF_CHEAT)									sFlags+="CHEAT, ";

	strcpy( sFlags,"" );

//	if(0 == (dwFlags&VF_NOT_NET_SYNCED_INTERNAL))	sFlags+="REQUIRE_NET_SYNC, ";
	if(dwFlags&VF_SAVEGAME)								strcat( sFlags,"SAVEGAME, ");
	if(dwFlags&VF_READONLY)								strcat( sFlags,"READONLY, ");
	if(dwFlags&VF_DUMPTODISK)							strcat( sFlags,"DUMPTODISK, ");
	if(dwFlags&VF_REQUIRE_LEVEL_RELOAD)		strcat( sFlags,"REQUIRE_LEVEL_RELOAD, ");
	if(dwFlags&VF_REQUIRE_APP_RESTART)		strcat( sFlags,"REQUIRE_APP_RESTART, ");
	if(dwFlags&VF_RESTRICTEDMODE)					strcat( sFlags,"RESTRICTEDMODE, ");

	if(sFlags[0] != 0)
		sFlags[strlen(sFlags)-2] = 0; // remove ending chars

	return sFlags;
}


//////////////////////////////////////////////////////////////////////////
#if !defined(PS3) && !defined(XENON)
void CXConsole::DumpCommandsVarsTxt( const char *prefix )
{
	FILE *f0 = fopen("consolecommandsandvars.txt", "w");

	if(!f0) 
		return;

	ConsoleCommandsMapItor itrCmd, itrCmdEnd=m_mapCommands.end();
	ConsoleVariablesMapItor itrVar, itrVarEnd=m_mapVariables.end();

	fprintf(f0," CHEAT: stays in the default value if cheats are not disabled\n");
	fprintf(f0," REQUIRE_NET_SYNC: cannot be changed on client and when connecting it´s sent to the client\n");
	fprintf(f0," SAVEGAME: stored when saving a savegame\n");
	fprintf(f0," READONLY: can not be changed by the user\n");
	fprintf(f0,"-------------------------\n");
	fprintf(f0,"\n");

	for(itrCmd=m_mapCommands.begin(); itrCmd!=itrCmdEnd; ++itrCmd)
	{
		CConsoleCommand &cmd = itrCmd->second;

		if(hasprefix(cmd.m_sName.c_str(), prefix))
		{
			const char* sFlags = GetFlagsString(cmd.m_nFlags);

			fprintf(f0, "Command: %s %s\nscript: %s\nhelp: %s\n\n", cmd.m_sName.c_str(), sFlags, cmd.m_sCommand.c_str(), cmd.m_sHelp.c_str());
		}
	}

	for(itrVar=m_mapVariables.begin(); itrVar!=itrVarEnd; ++itrVar)
	{
		ICVar *var = itrVar->second;
		const char *types[] = { "?", "int", "float", "string", "?" };

		var->GetRealIVal();			// assert inside checks consistency for all cvars

		if(hasprefix(var->GetName(), prefix)) 
		{
			const char *sFlags = GetFlagsString(var->GetFlags());

			fprintf(f0, "variable: %s %s\ntype: %s\ncurrent: %s\nhelp: %s\n\n", var->GetName(), sFlags, types[var->GetType()], var->GetString(), var->GetHelp());
		}
	}

	fclose(f0);

	ConsoleLogInputResponse( "successfully wrote consolecommandsandvars.txt");	
}
#endif // !defined(PS3) && !defined(XENON)

//////////////////////////////////////////////////////////////////////////
void CXConsole::DisplayHelp( const char *help, const char *name)
{
	if(help==0 || *help==0)
	{
		ConsoleLogInputResponse("No help available for $3%s", name);
	}
	else
	{
		char *start, *pos;
		for(pos = (char *)help, start = (char *)help; pos = strstr(pos, "\n"); start = ++pos)
		{
			string s = start;
			s.resize(pos-start);
			ConsoleLogInputResponse("    $3%s", s.c_str());
		}
		ConsoleLogInputResponse("    $3%s", start);
	}
}


void CXConsole::ExecuteString( const char *command, const bool bSilentMode, const bool bDeferExecution )
{
	if (!m_deferredExecution && !bDeferExecution)
	{
		// This is a regular mode
		ExecuteStringInternal(command, false, bSilentMode);		// not from console

		return;
	}

	// Store the string commands into a list and defer the execution for later.
	// The commands will be processed in CXConsole::Update()
	string str(command);
	str.TrimLeft();

	// Unroll the exec command
	bool unroll = (0 == str.Left(strlen("exec")).compareNoCase("exec"));
	
	if (unroll)
	{
		bool oldDeferredExecution = m_deferredExecution;

		// Make sure that the unrolled commands are processed with deferred mode on
		m_deferredExecution = true;
		ExecuteStringInternal(str.c_str(), false, bSilentMode);

		// Restore to the previous setting
		m_deferredExecution = oldDeferredExecution;
	}
	else
		m_deferredCommands.push_back( SDeferredCommand(str, bSilentMode) );
}

void CXConsole::SplitCommands(const char* line, std::list<string>& split)
{
	const char *start = line;
	string working;

	while(true)
	{
		char ch = *line++;
		switch(ch)
		{
		case '\'':
		case '\"':		
			while((*line++ != ch) && *line);
			break;
		case '\n':
		case '\r':
		case ';':
		case '\0':
			{
				working.assign(start, line-1);
				working.Trim();

				if (!working.empty())
					split.push_back(working);
				start = line;

				if (ch == '\0')
					return;
			}
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::ExecuteStringInternal(const char *command, const bool bFromConsole, const bool bSilentMode )
{
	assert(command);
	assert(command[0]!='\\');			// caller should remove leading "\\"

	///////////////////////////
	//Execute as string
	if(command[0]=='#' || command[0]=='@')
	{
		if(!con_restricted || !bFromConsole)			// in restricted mode we allow only VF_RESTRICTEDMODE CVars&CCmd
		{
			AddLine(command);

			if (m_pSystem->IsDevMode())
			{
				if (m_pSystem->GetIScriptSystem())
					m_pSystem->GetIScriptSystem()->ExecuteBuffer(command+1,strlen(command)-1);
				m_bDrawCursor = 0;
			}
			else
			{
				// Warning.
				// No Cheat warnings. ConsoleWarning("Console execution is cheat protected");
			}
			return;
		}
	}

	ConsoleCommandsMapItor itrCmd;
	ConsoleVariablesMapItor itrVar;

	std::list<string> lineCommands;
	SplitCommands(command, lineCommands);

	string sTemp;
	string sCommand;

	while (!lineCommands.empty())
	{
		sTemp = lineCommands.front();
		sCommand = lineCommands.front();
		lineCommands.pop_front();







			
		if(!bSilentMode)
		if(GetStatus())
			AddLine(sTemp);

		string::size_type nPos = sTemp.find_first_of('=');

		if (nPos != string::npos)
			sCommand=sTemp.substr(0,nPos);
		else if((nPos=sTemp.find_first_of(' ')) != string::npos)
			sCommand=sTemp.substr(0,nPos);
		else
			sCommand=sTemp;

		sCommand.Trim();

		//////////////////////////////////////////
		// Search for CVars
		if( sCommand.length() > 1 && sCommand[0] == '?' )
		{
			sTemp = sCommand.substr( 1 );
			itrVar = m_mapVariables.begin();
			while( itrVar != m_mapVariables.end() )
			{
				if( (itrVar->second->GetFlags()&VF_RESTRICTEDMODE) || !con_restricted || !bFromConsole )
				{
					if( CryStringUtils::stristr( itrVar->first, sTemp.c_str() ) != NULL )
						CryLog( itrVar->first );
				}
				++itrVar;
			}
			continue;
		}

		//////////////////////////////////////////
		//Check if is a command
		itrCmd=m_mapCommands.find(sCommand);
		if(itrCmd!=m_mapCommands.end())
		{
			if(((itrCmd->second).m_nFlags&VF_RESTRICTEDMODE) || !con_restricted || !bFromConsole)			// in restricted mode we allow only VF_RESTRICTEDMODE CVars&CCmd
			{
				if (itrCmd->second.m_nFlags&VF_BLOCKFRAME)
					m_blockCounter++;

				sTemp=command;
				ExecuteCommand((itrCmd->second), sTemp);

				continue;
			}
		}

		//////////////////////////////////////////
		//Check  if is a variable
		itrVar=m_mapVariables.find(sCommand);
		if(itrVar!=m_mapVariables.end())
		{
			ICVar *pCVar = itrVar->second;

			if((pCVar->GetFlags()&VF_RESTRICTEDMODE) || !con_restricted || !bFromConsole)			// in restricted mode we allow only VF_RESTRICTEDMODE CVars&CCmd
			{
				if (pCVar->GetFlags()&VF_BLOCKFRAME)
					m_blockCounter++;

				if(nPos!=string::npos)
				{
					sTemp=sTemp.substr(nPos+1);		// remove the command from sTemp
					sTemp.Trim(" \t\r\n\"\'");

					if(sTemp=="?")
					{
						ICVar *v = itrVar->second;
						DisplayHelp( v->GetHelp(), sCommand.c_str());
						return;
					}

					if(!sTemp.empty() || (pCVar->GetType() == CVAR_STRING))
						pCVar->Set(sTemp.c_str());
				}

				// the following line calls AddLine() indirectly
				DisplayVarValue( pCVar );
				//ConsoleLogInputResponse("%s=%s",pCVar->GetName(),pCVar->GetString());
				continue;
			}
		}

		if(!bSilentMode)
			ConsoleWarning("Unknown command: %s", sCommand.c_str());	
	}
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::ExecuteDeferredCommands()
{
	TDeferredCommandList::iterator it;

// 	const float fontHeight = 10;
// 	ColorF col = Col_Yellow;
// 
// 	float curX = 10;
// 	float curY = 10;

	IRenderer * pRenderer = gEnv->pRenderer;

	// Print the deferred messages
// 	it = m_deferredCommands.begin();
// 	if (it != m_deferredCommands.end())
// 	{
// 		pRenderer->Draw2dLabel( curX, curY += fontHeight, 1.2f, &col.r, false
// 		, "Pending deferred commands = %d", m_deferredCommands.size() );
// 	}
// 
// 	for (
// 			; it != m_deferredCommands.end()
// 			; ++it
// 			)
// 	{
// 		pRenderer->Draw2dLabel( curX + fontHeight * 2.0f, curY += fontHeight, 1.2f, &col.r, false
// 			, "Cmd: %s", it->command.c_str() );
// 	}

	if (m_waitFrames)
	{
// 		pRenderer->Draw2dLabel( curX, curY += fontHeight, 1.2f, &col.r, false
// 			, "Waiting frames = %d", m_waitFrames );

		--m_waitFrames;
		return;
	}

	if (m_waitSeconds.GetValue())
	{
		if (m_waitSeconds > gEnv->pTimer->GetFrameStartTime())
		{
// 			pRenderer->Draw2dLabel( curX, curY += fontHeight, 1.2f, &col.r, false
// 				, "Waiting seconds = %f"
// 				, m_waitSeconds.GetSeconds() - gEnv->pTimer->GetFrameStartTime().GetSeconds() );

			return;
		}

		// Help to avoid overflow problems
		m_waitSeconds.SetValue(0);
	}

	const int blockCounter = m_blockCounter;

	// Signal the console that we executing a deferred command
	//m_deferredExecution = true;

	while (! m_deferredCommands.empty())
	{
		it = m_deferredCommands.begin();
		ExecuteStringInternal(it->command.c_str(), false, it->silentMode);
		m_deferredCommands.pop_front();

		// A blocker command was executed
		if (m_blockCounter != blockCounter)
			break;
	}

	//m_deferredExecution = false;
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::ExecuteCommand(CConsoleCommand &cmd,string &str,bool bIgnoreDevMode)
{
    std::vector<string> args;

		size_t t=1;

		const char* start = str.c_str();
		const char* commandLine = start;
		while(char ch = *commandLine++)
		{
			switch(ch)
			{
			case '\'':
			case '\"':
				{
					while((*commandLine++ != ch) && *commandLine);
					args.push_back(string(start+1, commandLine-1));
					start = commandLine;
					break;
				}
			case ' ':
				start = commandLine;
				break;
			default:
				{
					if ((*commandLine == ' ') || !*commandLine)
					{
						args.push_back(string(start, commandLine));
						start = commandLine + 1;
					}
				}
				break;
			}
		}

    if(args.size()>=2 && args[1]=="?")
    {
			DisplayHelp( cmd.m_sHelp, cmd.m_sName.c_str() );
			return;
    }

		if((cmd.m_nFlags&VF_CHEAT)!=0 && !m_pSystem->IsDevMode() && !bIgnoreDevMode)
		{
			// No Log. ConsoleWarning("Command %s is cheat protected.", cmd.m_sName.c_str());
			return;
		}

		if (cmd.m_func)
		{
			// This is function command, execute it with a list of parameters.
			CConsoleCommandArgs cmdArgs(str, args);
			cmd.m_func( &cmdArgs );
			return;
		}

		// only do this for commands with script implementation
		for(;;) 
		{
			t = str.find_first_of("\\",t);
			if (t==string::npos)break;
			str.replace(t, 1, "\\\\", 2);
			t+=2;
		} 

		for(t=1;;) 
		{
			t = str.find_first_of("\"",t);
			if (t==string::npos)break;
			str.replace(t, 1, "\\\"", 2);
			t+=2;
		} 
    
    string buf = cmd.m_sCommand;

    size_t pp = buf.find("%%");
    if(pp!=string::npos)
    {
      string list = "";
      for(unsigned int i = 1; i<args.size(); i++)
      {
          list += "\""+args[i]+"\"";
          if(i<args.size()-1) list += ",";
      }
			buf.replace(pp, 2, list);          
		}
		else if((pp = buf.find("%line"))!=string::npos)
		{
			string tmp="\""+str.substr(str.find(" ")+1)+"\"";
			if(args.size()>1)
			{
				buf.replace(pp, 5, tmp);
			}
			else
			{
				buf.replace(pp, 5, "");
			}
		}
    else
    {
      for(unsigned int i = 1; i<=args.size(); i++)
      {
        char pat[10];
        sprintf(pat, "%%%d", i); 
        size_t pos = buf.find(pat);
        if(pos==string::npos)
        {
          if(i!=args.size())
          {
            ConsoleWarning("Too many arguments for: %s", cmd.m_sName.c_str());
            return;
          }
        }
        else
        {
          if(i==args.size())
          {
              ConsoleWarning("Not enough arguments for: %s", cmd.m_sName.c_str());
              return;
          }
          string arg = "\""+args[i]+"\"";
					buf.replace(pos, strlen(pat), arg);          
        }
			}
    }

	if (m_pSystem->GetIScriptSystem())
		m_pSystem->GetIScriptSystem()->ExecuteBuffer(buf.c_str(), buf.length());
	m_bDrawCursor = 0;
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::Exit(const char * szExitComments, ...)
{
  char sResultMessageText[1024]="";
  
	if(szExitComments) 
  { 
		// make result string
    va_list		arglist;    
    va_start(arglist, szExitComments);
    vsprintf_s(sResultMessageText, szExitComments, arglist);
    va_end(arglist);
  }
  else strcpy(sResultMessageText, "No comments from application");

	CryFatalError( "%s",sResultMessageText );
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::RegisterAutoComplete( const char *sVarOrCommand,IConsoleArgumentAutoComplete *pArgAutoComplete )
{
	m_mapArgumentAutoComplete[sVarOrCommand] = pArgAutoComplete;
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::ResetAutoCompletion()
{
	m_nTabCount = 0;
	m_sPrevTab = "";
}

//////////////////////////////////////////////////////////////////////////
const char *CXConsole::ProcessCompletion(const char *szInputBuffer)
{
	m_sInputBuffer = szInputBuffer;

	int offset = (szInputBuffer[0] == '\\' ? 1 : 0);		// legacy support

	if ((m_sPrevTab.size() > strlen(szInputBuffer + offset)) || strnicmp(m_sPrevTab.c_str(), (szInputBuffer + offset), m_sPrevTab.size()))
	{
		m_nTabCount = 0;
		m_sPrevTab = "";
	}

	if(m_sInputBuffer.empty())
		return (char *)m_sInputBuffer.c_str();

	int nMatch = 0;
	ConsoleCommandsMapItor itrCmds;
	ConsoleVariablesMapItor itrVars;
	bool showlist = !m_nTabCount && m_sPrevTab=="";

	if (m_nTabCount==0)
	{
		if(m_sInputBuffer.size()>0)
			if(m_sInputBuffer[0]=='\\')
				m_sPrevTab=&m_sInputBuffer.c_str()[1];		// legacy support
			else
			{
				m_sPrevTab=m_sInputBuffer;
			}

		else
			m_sPrevTab="";
	}
	//try to search in command list

	bool bArgumentAutoComplete = false;
	std::vector<string> matches;

	if (m_sPrevTab.find(' ') != string::npos)
	{
		bool bProcessAutoCompl=true;

		// Find command.
		string sVar = m_sPrevTab.substr(0,m_sPrevTab.find(' '));
		ICVar *pCVar = GetCVar(sVar);
		if (pCVar)
		{
			if(!(pCVar->GetFlags()&VF_RESTRICTEDMODE) && con_restricted)			// in restricted mode we allow only VF_RESTRICTEDMODE CVars&CCmd
				bProcessAutoCompl=false;
		}
		
		ConsoleCommandsMap::iterator it = m_mapCommands.find(sVar);
		if (it != m_mapCommands.end())
		{
			CConsoleCommand &ccmd = it->second;

			if(!(ccmd.m_nFlags&VF_RESTRICTEDMODE) && con_restricted)			// in restricted mode we allow only VF_RESTRICTEDMODE CVars&CCmd
				bProcessAutoCompl=false;
		}

		if(bProcessAutoCompl)
		{
			IConsoleArgumentAutoComplete *pArgumentAutoComplete = stl::find_in_map(m_mapArgumentAutoComplete,sVar,0);
			if (pArgumentAutoComplete)
			{
				int nMatches = pArgumentAutoComplete->GetCount();
				for (int i = 0; i < nMatches; i++)
				{
					string cmd = string(sVar) + " " + pArgumentAutoComplete->GetValue(i);
					if(strnicmp(m_sPrevTab.c_str(),cmd.c_str(),m_sPrevTab.length())==0)
					{
						bArgumentAutoComplete = true;
						matches.push_back( cmd );
					}
				}
			}
		}
	}

	if (!bArgumentAutoComplete)
	{
		itrCmds=m_mapCommands.begin();
		while(itrCmds!=m_mapCommands.end())
		{
			CConsoleCommand &cmd = itrCmds->second;

			if((cmd.m_nFlags&VF_RESTRICTEDMODE) || !con_restricted)			// in restricted mode we allow only VF_RESTRICTEDMODE CVars&CCmd
#ifdef SP_DEMO
			if((cmd.m_nFlags&(VF_CHEAT|VF_READONLY))==0 || (m_pSystem && m_pSystem->IsDevMode()))
			{
#endif
			if(strnicmp(m_sPrevTab.c_str(),itrCmds->first.c_str(),m_sPrevTab.length())==0)
#ifdef CRYSIS_BETA
				if ( AllowedForBeta(itrCmds->first) )
#endif
				matches.push_back((char *const)itrCmds->first.c_str());
#ifdef SP_DEMO
			}
#endif
			++itrCmds;
		}

		// try to search in console variables

		itrVars=m_mapVariables.begin();
		while(itrVars!=m_mapVariables.end()) 
		{
			ICVar *pVar = itrVars->second;

			if((pVar->GetFlags()&VF_RESTRICTEDMODE) || !con_restricted)			// in restricted mode we allow only VF_RESTRICTEDMODE CVars&CCmd
#ifdef SP_DEMO
			if((pVar->GetFlags()&(VF_CHEAT|VF_READONLY))==0 || (m_pSystem && m_pSystem->IsDevMode()))
			{
#endif
			//if(itrVars->first.compare(0,m_sPrevTab.length(),m_sPrevTab)==0)
			if(strnicmp(m_sPrevTab.c_str(),itrVars->first,m_sPrevTab.length())==0)
#ifdef CRYSIS_BETA
				if ( AllowedForBeta(itrVars->first) )
#endif
				matches.push_back((char *const)itrVars->first);
#ifdef SP_DEMO
			}
#endif
			++itrVars;
		}
	}























	if(!matches.empty())
		std::sort( matches.begin(),matches.end(),less_CVar );		// to sort commands with variables

	if (showlist && !matches.empty()) 
	{
		ConsoleLogInput(" ");		// empty line before auto completion

		for(std::vector<string>::iterator i = matches.begin(); i!=matches.end(); ++i)
		{
			// List matching variables
			const char *sVar = *i;
			ICVar *pVar = GetCVar( sVar );

			if (pVar)
				DisplayVarValue( pVar );
			 else
				ConsoleLogInputResponse( "    $3%s $6(Command)", sVar );
		}
	}

	for(std::vector<string>::iterator i = matches.begin(); i!=matches.end(); ++i)
	{
		if(m_nTabCount<=nMatch)
		{
			m_sInputBuffer = *i;
			m_sInputBuffer += " ";
			m_nTabCount=nMatch+1;
			return (char *)m_sInputBuffer.c_str();
		}
		nMatch++;
	}

	if (m_nTabCount>0)
	{
		m_nTabCount=0;
		m_sInputBuffer = m_sPrevTab;
		m_sInputBuffer = ProcessCompletion(m_sInputBuffer.c_str());
	}

	return (char *)m_sInputBuffer.c_str();
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::DisplayVarValue( ICVar *pVar  )
{
	if (!pVar)
		return;

	const char * sFlagsString=GetFlagsString(pVar->GetFlags());

	string sValue = pVar->GetString();
	string sVar = pVar->GetName();

	char szRealState[40]="";

	if(pVar->GetType()==CVAR_INT)
	{
		int iRealState = pVar->GetRealIVal();

		if(iRealState!=pVar->GetIVal())
		{
			if(iRealState==-1) 
				strcpy(szRealState," RealState=Custom");
			else
				sprintf_s(szRealState," RealState=%d",iRealState);
		}
	}

	if (gEnv->IsEditor())
		ConsoleLogInputResponse( "%s=%s [ %s ]%s",sVar.c_str(),sValue.c_str(),sFlagsString,szRealState);
	 else
		ConsoleLogInputResponse( "    $3%s = $6%s $5[%s]$4%s", sVar.c_str(),sValue.c_str(),sFlagsString,szRealState);
}

//////////////////////////////////////////////////////////////////////////
bool CXConsole::IsOpened()
{
	return m_nScrollPos == m_nTempScrollMax;
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::PrintLine(const char *s)
{
	AddLine(s);
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::PrintLinePlus(const char *s)
{
	AddLinePlus(s);
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::AddLine(string str)
{
	// strip trailing \n or \r.
	if (!str.empty() && (str[str.size()-1]=='\n' || str[str.size()-1]=='\r'))
		str.resize(str.size()-1);

	string::size_type nPos;
	while ((nPos=str.find('\n'))!=string::npos)
	{
		str.replace(nPos,1,1,' ');
	}

	while ((nPos=str.find('\r'))!=string::npos)
	{
		str.replace(nPos,1,1,' ');
	}

	m_dqConsoleBuffer.push_back(str);

	int nBufferSize = con_line_buffer_size;

	while(((int)(m_dqConsoleBuffer.size()))>nBufferSize)
	{
		m_dqConsoleBuffer.pop_front();
	}

	// tell everyone who is interested (e.g. dedicated server printout)
	{
		std::vector<IOutputPrintSink *>::iterator it;

		for(it=m_OutputSinks.begin();it!=m_OutputSinks.end();++it)
			(*it)->Print(str.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::ResetProgressBar(int nProgressBarRange)
{
	m_nProgressRange = nProgressBarRange;
	m_nProgress = 0;

	if (nProgressBarRange < 0)
		nProgressBarRange = 0;

	if (!m_nProgressRange)
	{
		if (m_nLoadingBackTexID)
		{
			if (m_pRenderer)
				m_pRenderer->RemoveTexture(m_nLoadingBackTexID);
			m_nLoadingBackTexID = -1;
		}
	}

	static ICVar *log_Verbosity = GetCVar("log_Verbosity");

	if (log_Verbosity && (!log_Verbosity->GetIVal()))
		Clear();
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::TickProgressBar()
{	
	if (m_nProgressRange != 0 && m_nProgressRange > m_nProgress)
	{
		m_nProgress++;
		m_pSystem->UpdateLoadingScreen();
	}

	//if(m_pSystem->GetIGame())
		//m_pSystem->GetIGame()->UpdateDuringLoading();		// network update
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::SetLoadingImage(const char *szFilename )
{
	ITexture *pTex = 0;		

	pTex = m_pSystem->GetIRenderer()->EF_LoadTexture(szFilename, FT_DONT_STREAM | FT_DONT_RESIZE | FT_NOMIPS, eTT_2D);

	if (!pTex || (pTex->GetFlags() & FT_FAILED))
	{
    SAFE_RELEASE(pTex);
		pTex = m_pSystem->GetIRenderer()->EF_LoadTexture("Textures/Console/loadscreen_default.dds", FT_DONT_STREAM | FT_DONT_RESIZE | FT_NOMIPS, eTT_2D);
	}

	if (pTex)
	{
		m_nLoadingBackTexID = pTex->GetTextureID();
	}
	else
	{
		m_nLoadingBackTexID = -1;
	}
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::AddOutputPrintSink( IOutputPrintSink *inpSink )
{
	assert(inpSink);

	m_OutputSinks.push_back(inpSink);
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::RemoveOutputPrintSink( IOutputPrintSink *inpSink )
{
	assert(inpSink);

	int nCount=m_OutputSinks.size();

	for(int i=0;i<nCount;i++)
	{
		if(m_OutputSinks[i]==inpSink)
		{
			if(nCount<=1)	m_OutputSinks.clear();
			else
			{
				m_OutputSinks[i]=m_OutputSinks.back();
				m_OutputSinks.pop_back();
			}
			return;
		}
	}

	assert(0);
}


//////////////////////////////////////////////////////////////////////////
void CXConsole::AddLinePlus(string str)
{
	if(!m_dqConsoleBuffer.size())
   return;

	// strip trailing \n or \r.
	if (!str.empty() && (str[str.size()-1]=='\n' || str[str.size()-1]=='\r'))
		str.resize(str.size()-1);

	string::size_type nPos;
	while ((nPos=str.find('\n'))!=string::npos)
		str.replace(nPos,1,1,' ');

	while ((nPos=str.find('\r'))!=string::npos)
		str.replace(nPos,1,1,' ');

	string tmpStr = m_dqConsoleBuffer.back();// += str;

	m_dqConsoleBuffer.pop_back();

  if(tmpStr.size()<256)
    m_dqConsoleBuffer.push_back(tmpStr + str);
  else 
    m_dqConsoleBuffer.push_back(tmpStr);

	// tell everyone who is interested (e.g. dedicated server printout)
	{
		std::vector<IOutputPrintSink *>::iterator it;

		for(it=m_OutputSinks.begin();it!=m_OutputSinks.end();++it)
			(*it)->Print((tmpStr + str).c_str());
	}
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::AddInputChar(const char c)
{
	if(m_nCursorPos<(int)(m_sInputBuffer.length()))
		m_sInputBuffer.insert(m_nCursorPos,1,c);
	else
		m_sInputBuffer=m_sInputBuffer+c;
	m_nCursorPos++;
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::ExecuteInputBuffer()
{
	string sTemp=m_sInputBuffer;
	if(m_sInputBuffer.empty())
		return;
	m_sInputBuffer="";
#ifdef CRYSIS_BETA
	int i = 0;
	string cmd = sTemp.Tokenize("# =", i);
	if ( AllowedForBeta(cmd) )
		goto L_execute;
	CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "You are not allowed to execute '%s' during BETA test!", sTemp.c_str());
	m_nCursorPos = 0;
	return;
L_execute:
#endif
	AddCommandToHistory(sTemp.c_str());

	ExecuteStringInternal(sTemp.c_str(),true);			// from console
	
	m_nCursorPos=0;
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::RemoveInputChar(bool bBackSpace)
{
	if(m_sInputBuffer.empty())
		return;

	if(bBackSpace)
	{
		if(m_nCursorPos>0)
		{
			m_sInputBuffer.erase(m_nCursorPos-1,1);
			m_nCursorPos--;
		}
	}
	else
	{
		if(m_nCursorPos<(int)(m_sInputBuffer.length()))
			m_sInputBuffer.erase(m_nCursorPos,1);
	}
}


//////////////////////////////////////////////////////////////////////////
void CXConsole::AddCommandToHistory( const char *szCommand )
{
	assert(szCommand);

	m_nHistoryPos=-1;

	if(!m_dqHistory.empty())
	{
		// add only if the command is != than the last
		if(m_dqHistory.front()!=szCommand)
			m_dqHistory.push_front(szCommand);
	}
	else
		m_dqHistory.push_front(szCommand);

	while(m_dqHistory.size()>MAX_HISTORY_ENTRIES)
		m_dqHistory.pop_back();
}


//////////////////////////////////////////////////////////////////////////
void CXConsole::Copy()
{
#ifdef WIN32
	if (m_sInputBuffer.empty())
		return;

	if (!OpenClipboard(NULL))
		return;

	size_t cbLength = m_sInputBuffer.length();

	HGLOBAL hGlobal;
	LPVOID  pGlobal;

	hGlobal = GlobalAlloc(GHND, cbLength + 1);
	pGlobal = GlobalLock (hGlobal);

	strcpy((char *)pGlobal, m_sInputBuffer.c_str());

	GlobalUnlock(hGlobal);

	EmptyClipboard();
	SetClipboardData(CF_TEXT, hGlobal);
	CloseClipboard();

	return;
#endif //WIN32
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::Paste()
{
#ifdef WIN32
	//TRACE("Paste\n");
	
	static char sTemp[256];
	HGLOBAL hGlobal;
  void*  pGlobal;
	
	
	if (!IsClipboardFormatAvailable(CF_TEXT)) 
		return; 
	if (!OpenClipboard(NULL))
		return;
	
	hGlobal = GetClipboardData(CF_TEXT);
	if(!hGlobal)
		return;

	pGlobal = GlobalLock(hGlobal);
	size_t cbLength = min(sizeof(sTemp)-1, strlen((char *)pGlobal));
	
	strncpy(sTemp, (char *)pGlobal, cbLength);
	sTemp[cbLength] = '\0';
	GlobalUnlock(hGlobal);
	CloseClipboard();
	
	m_sInputBuffer.insert(m_nCursorPos,sTemp,strlen(sTemp));
	m_nCursorPos+=(int)strlen(sTemp);
#endif //WIN32
}


//////////////////////////////////////////////////////////////////////////
int CXConsole::GetNumVars()
{
	return (int)m_mapVariables.size();
}

//////////////////////////////////////////////////////////////////////////
size_t CXConsole::GetSortedVars( const char **pszArray, size_t numItems, const char *szPrefix )
{
	size_t i = 0;
	size_t iPrefixLen = szPrefix ? strlen(szPrefix) : 0;

	// variables
	{
		ConsoleVariablesMap::const_iterator it, end=m_mapVariables.end();
		for(it=m_mapVariables.begin(); it!=end; ++it)
		{
			if(pszArray && i>=numItems)
				break;

			if(szPrefix)
			if(strnicmp(it->first,szPrefix,iPrefixLen)!=0)
				continue;

			if (it->second->GetFlags() & VF_INVISIBLE)
				continue;

			if(pszArray)
				pszArray[i] = it->first;

			i++;
		}
	}

	// commands
	{
		ConsoleCommandsMap::iterator it, end=m_mapCommands.end();
		for(it=m_mapCommands.begin(); it!=end; ++it)
		{
			if(pszArray && i>=numItems)
				break;

			if(szPrefix)
				if(strnicmp(it->first.c_str(),szPrefix,iPrefixLen)!=0)
					continue;

			if (it->second.m_nFlags & VF_INVISIBLE)
				continue;

			if(pszArray)
				pszArray[i] = it->first.c_str();

			i++;
		}
	}

	if(i!=0 && pszArray)
		std::sort( pszArray,pszArray+i,less_CVar );

	return i;
}

//////////////////////////////////////////////////////////////////////////
const char* CXConsole::AutoComplete( const char* substr )
{
	// following code can be optimized

	std::vector<const char*> cmds;
	cmds.resize( GetNumVars()+m_mapCommands.size() );
	size_t cmdCount = GetSortedVars( &cmds[0],cmds.size() );

	size_t substrLen = strlen(substr);
	
	// If substring is empty return first command.
	if (substrLen==0 && cmdCount>0)
		return cmds[0];

	// find next
	for (size_t i = 0; i < cmdCount; i++)
	{
		const char *szCmd = cmds[i];
		size_t cmdlen = strlen(szCmd);
		if (cmdlen >= substrLen && memcmp(szCmd,substr,substrLen) == 0)
		{
			if (substrLen == cmdlen)
			{
				i++;
				if (i < cmdCount)
					return cmds[i];
				return cmds[i-1];
			}
			return cmds[i];
		}
	}

	// then first matching case insensitive
	for (size_t i = 0; i < cmdCount; i++)
	{
		const char *szCmd = cmds[i];

		size_t cmdlen = strlen(szCmd);
		if (cmdlen >= substrLen && memicmp(szCmd,substr,substrLen) == 0)
		{
			if (substrLen == cmdlen)
			{
				i++;
				if (i < cmdCount)
					return cmds[i];
				return cmds[i-1];
			}
			return cmds[i];
		}
	}

	// Not found.
	return "";
}
	
void CXConsole::SetInputLine( const char *szLine )
{
	assert(szLine);

	m_sInputBuffer = szLine;
	m_nCursorPos=m_sInputBuffer.size();
}



//////////////////////////////////////////////////////////////////////////
const char* CXConsole::AutoCompletePrev( const char* substr )
{
	std::vector<const char*> cmds;
	cmds.resize( GetNumVars()+m_mapCommands.size() );
	size_t cmdCount = GetSortedVars( &cmds[0],cmds.size() );

	// If substring is empty return last command.
	if (strlen(substr)==0 && cmds.size()>0)
		return cmds[cmdCount-1];

	for (unsigned int i = 0; i < cmdCount; i++)
	{
		if (stricmp(substr,cmds[i])==0)
		{
			if (i > 0) 
				return cmds[i-1];
			else
				return cmds[0];
		}
	}
	return AutoComplete( substr );
}

//////////////////////////////////////////////////////////////////////////
inline size_t sizeOf (const string& str)
{
	return str.capacity()+1;
}

//////////////////////////////////////////////////////////////////////////
inline size_t sizeOf (const char* sz)
{
	return sz? strlen(sz)+1:0;
}

//////////////////////////////////////////////////////////////////////////
inline size_t sizeOf (const CXConsole::ConsoleBuffer& Buffer)
{
	size_t nSize = 0;
	CXConsole::ConsoleBuffer::const_iterator it = Buffer.begin(), itEnd = Buffer.end();
	for (; it != itEnd; ++it)
	{
		nSize += sizeof(*it) + sizeOf (*it);
	}
	return nSize;
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::GetMemoryUsage (class ICrySizer* pSizer)
{
	pSizer->AddObject (this, sizeof(*this)
		+ sizeOf (m_dqConsoleBuffer)
		+ sizeOf (m_dqHistory)
		+ sizeOf (m_sInputBuffer)
		+ sizeOf (m_sPrevTab)
		);

	{
		ConsoleCommandsMap::const_iterator it = m_mapCommands.begin(), itEnd = m_mapCommands.end();
		for (;it != itEnd; ++it)
			pSizer->AddObject (&(*it), sizeof(*it) + sizeOf(it->first) + it->second.sizeofThis());
	}

	{
		ConsoleBindsMap::const_iterator it = m_mapBinds.begin(), itEnd = m_mapBinds.end();
		for (; it != itEnd; ++it)
			pSizer->AddObject (&(*it), sizeof(*it) + sizeOf (it->first) + sizeOf (it->second));
	}

	{
		ConsoleVariablesMap::const_iterator it = m_mapVariables.begin(), itEnd = m_mapVariables.end();
		for (; it != itEnd; ++it)
		{
			pSizer->AddObject (&*it, sizeof(*it) + sizeOf(it->first));
			it->second->GetMemoryUsage (pSizer);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::ConsoleLogInputResponse( const char *format,... )
{
	va_list args;
	va_start(args,format);
	gEnv->pLog->LogV( ILog::eInputResponse,format,args );
	va_end(args);
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::ConsoleLogInput( const char *format,... )
{
	va_list args;
	va_start(args,format);
	gEnv->pLog->LogV( ILog::eInput,format,args );
	va_end(args);
}


//////////////////////////////////////////////////////////////////////////
void CXConsole::ConsoleWarning( const char *format,... )
{
	va_list args;
	va_start(args,format);
	gEnv->pLog->LogV( ILog::eWarningAlways,format,args );
	va_end(args);
}

//////////////////////////////////////////////////////////////////////////
bool CXConsole::OnBeforeVarChange( ICVar *pVar,const char *sNewValue )
{
	if (((pVar->GetFlags()&VF_CHEAT)!=0) && !gEnv->pSystem->IsDevMode())
		return (false);

	if (!m_consoleVarSinks.empty())
	{
		ConsoleVarSinks::iterator it,next;
		for (it = m_consoleVarSinks.begin(); it != m_consoleVarSinks.end(); it = next)
		{
			next = it; next++;
			if (!(*it)->OnBeforeVarChange(pVar,sNewValue))
				return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::OnAfterVarChange( ICVar *pVar )
{
	if (!m_consoleVarSinks.empty())
	{
		ConsoleVarSinks::iterator it,next;
		for (it = m_consoleVarSinks.begin(); it != m_consoleVarSinks.end(); it = next)
		{
			next = it; next++;
			(*it)->OnAfterVarChange(pVar);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::AddConsoleVarSink( IConsoleVarSink *pSink )
{
	m_consoleVarSinks.push_back(pSink);
}

//////////////////////////////////////////////////////////////////////////
void CXConsole::RemoveConsoleVarSink( IConsoleVarSink *pSink )
{
	m_consoleVarSinks.remove(pSink);
}


#include UNIQUE_VIRTUAL_WRAPPER(IConsoleCmdArgs)
#include UNIQUE_VIRTUAL_WRAPPER(IConsole)
