//////////////////////////////////////////////////////////////////////
//
//	Crytek Source code (c) Crytek 2001-2004
//
//	File: SystemInit.cpp
//  Description: CryENGINE system core-handle all subsystems
//
//	History:
//	-Feb 09,2004: split from system.cpp
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "System.h"
#include "CryLibrary.h"
#include <StringUtils.h>
#include "NullImplementation/NULLSoundSystem.h"
#include "NullImplementation/NULLMusicSystem.h"
#include "NullImplementation/NullInput.h"

#include <IEngineModule.h>
#include <CryExtension/CryCreateClassInstance.h>

#include "CopyProtection.h"

#if defined(LINUX) && !defined(DEDICATED_SERVER)
#include <dlfcn.h>
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include <float.h>

#define  PROFILE_WITH_VTUNE

#endif //WIN32

#include <INetwork.h>
#include <I3DEngine.h>
#include <IAISystem.h>
#include <IRenderer.h>
#include <ICryPak.h>
#include <IMovieSystem.h>
#include <IEntitySystem.h>
#include <IInput.h>
#include <ILog.h>
#include <ISound.h>
#include <ICryAnimation.h>
#include <IScriptSystem.h>
#include <ICmdLine.h>
#include <IProcess.h>

#include "CryPak.h"
#include "XConsole.h"
#include "Log.h"
#include "XML/xml.h"
#include "DataProbe.h"
#include "StreamEngine.h"
#include "BudgetingSystem.h"
#include "PhysRenderer.h"
#include "LocalizedStringManager.h"
#include "SystemEventDispatcher.h"
#include "Statistics.h"
#include "Statistics/LocalMemoryUsage.h"
#include "ThreadProfiler.h"
#include "HardwareMouse.h"
#include "Scaleform/FlashPlayerInstance.h"				// CFlashPlayer::InitCVars
#include "Validator.h"
#include "ServerThrottle.h"
#include "SystemCFG.h"
#include "AutoDetectSpec.h"
#include "ResourceManager.h"
#include "LoadingProfiler.h"
#include "DiskProfiler.h"
#include "TestSystemLegacy.h"
#include "VisRegTest.h"
#include "MTSafeAllocator.h"
#include "NotificationNetwork.h"
#include "HotUpdate.h"
#include "ExtensionSystem/CryFactoryRegistryImpl.h"
#include "ExtensionSystem/TestCases/TestExtensions.h"
#include "ProfileLogSystem.h"

#include "IPlatformOS.h"
#include "PerfHUD.h"
#include "MiniGUI/MiniGUI.h"

#include <IGame.h>
#include <IGameFramework.h>














































#ifdef WIN32
#include "DebugCallStack.h"
#endif

#include "UnixConsole.h"
DLL_EXPORT ITextModeConsole* g_pUnixConsole = 0;

#ifdef WIN32
extern LONG WINAPI CryEngineExceptionFilterWER( struct _EXCEPTION_POINTERS * pExceptionPointers );
#endif

//////////////////////////////////////////////////////////////////////////
#define DEFAULT_LOG_FILENAME "Log.txt"

// Alias name used for per level cache
#define CRYENGINE_ENGINE_FOLDER "Engine"

//////////////////////////////////////////////////////////////////////////
#define CRYENGINE_DEFAULT_LANGUAGE_CONFIG_FILE "/Localized/Default.lng"

#define CRYENGINE_DEFAULT_CONFIG_PAK "GameData.pak"

//////////////////////////////////////////////////////////////////////////
#define CRYENGINE_GAME_FOLDER_KEY "sys_game_folder"
#define CRYENGINE_USER_FOLDER_KEY "sys_user_folder"

#if defined (SP_DEMO)
#define CRYENGINE_FOLDER_NAME_USER "UserFolderSPDEMO"
#elif defined (CRYSIS_BETA) 
#define CRYENGINE_FOLDER_NAME_USER "UserFolderMPDEMO"
#else
#define CRYENGINE_FOLDER_NAME_USER "UserFolder"
#endif

//////////////////////////////////////////////////////////////////////////
// Where possible, these are defaults used to initialize cvars
// System.cfg can then be used to override them
// This includes the Game DLL, although it is loaded elsewhere
#define DLL_SOUND				  "CrySoundSystem"
#define DLL_NETWORK			  "CryNetwork"
#define DLL_ENTITYSYSTEM	"CryEntitySystem"
#define DLL_SCRIPTSYSTEM	"CryScriptSystem"
#define DLL_INPUT				  "CryInput"
#define DLL_PHYSICS			  "CryPhysics"
#define DLL_MOVIE				  "CryMovie"
#define DLL_AI						"CryAISystem"
#define DLL_ANIMATION		  "CryAnimation"
#define DLL_FONT					"CryFont"
#define DLL_3DENGINE			"Cry3DEngine"
#define DLL_RENDERER_DX9  "CryRenderD3D9"
#define DLL_RENDERER_DX11 "CryRenderD3D11"
#define DLL_RENDERER_OGL  "CryRenderOGL"
#define DLL_RENDERER_NULL "CryRenderNULL"
#define DLL_GAME					"CryGame"
#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
	#define DLL_GPU_PHYSICS		"CryPhysicsGPU"
#endif

//////////////////////////////////////////////////////////////////////////
#if defined(WIN32) || defined(LINUX)
# define DLL_MODULE_INIT_ISYSTEM "ModuleInitISystem"
#	define DLL_INITFUNC_RENDERER "PackageRenderConstructor"
#	define DLL_INITFUNC_NETWORK "CreateNetwork"
#	define DLL_INITFUNC_ENTITY "CreateEntitySystem"
#	define DLL_INITFUNC_INPUT "CreateInput"
#	define DLL_INITFUNC_SOUND "CreateSoundSystem"
#	define DLL_INITFUNC_PHYSIC "CreatePhysicalWorld"
#	define DLL_INITFUNC_MOVIE "CreateMovieSystem"
#	define DLL_INITFUNC_AI "CreateAISystem"
#	define DLL_INITFUNC_SCRIPT "CreateScriptSystem"
#	define DLL_INITFUNC_FONT "CreateCryFontInterface"
#	define DLL_INITFUNC_3DENGINE "CreateCry3DEngine"
#	define DLL_INITFUNC_ANIMATION "CreateCharManager"
#	ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
#		define DLL_INITFUNC_GPU_PHYSICS	"CreateGPUPhysics"
#	endif
#else
# define DLL_MODULE_INIT_ISYSTEM (LPCSTR)2
#	define DLL_INITFUNC_RENDERER  (LPCSTR)1
#	define DLL_INITFUNC_RENDERER  (LPCSTR)1
#	define DLL_INITFUNC_NETWORK   (LPCSTR)1
#	define DLL_INITFUNC_ENTITY    (LPCSTR)1
#	define DLL_INITFUNC_INPUT     (LPCSTR)1
#	define DLL_INITFUNC_SOUND     (LPCSTR)1
#	define DLL_INITFUNC_PHYSIC    (LPCSTR)1
#	define DLL_INITFUNC_MOVIE     (LPCSTR)1
#	define DLL_INITFUNC_AI        (LPCSTR)1
#	define DLL_INITFUNC_SCRIPT    (LPCSTR)1
#	define DLL_INITFUNC_FONT      (LPCSTR)1
#	define DLL_INITFUNC_3DENGINE  (LPCSTR)1
#	define DLL_INITFUNC_ANIMATION (LPCSTR)1
#	ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
#		define DLL_INITFUNC_GPU_PHYSICS (LPCSTR)1
#	endif
#endif

//////////////////////////////////////////////////////////////////////////
// Extern declarations for static libraries.
//////////////////////////////////////////////////////////////////////////
#if defined(_LIB) || defined(PS3)
extern "C"
{
	IAISystem* CreateAISystem( ISystem *pSystem );
	IMovieSystem* CreateMovieSystem( ISystem *pSystem );
}
#endif //_LIB
//////////////////////////////////////////////////////////////////////////

extern CMTSafeHeap* g_pPakHeap;

#ifdef WIN32
extern HMODULE gDLLHandle;
#endif

//static int g_sysSpecChanged = false;

const char *g_szLvlResExt="_LvlRes.txt";

//////////////////////////////////////////////////////////////////////////
static void OnSysSpecChange( ICVar *pVar )
{
//	g_sysSpecChanged = true;
	static int no_recursive = false;
	if (no_recursive)
		return;
	no_recursive = true;
	// Called when sys_spec (client config spec) variable changes.
	int spec = pVar->GetIVal();

	if (spec > ((CSystem*)gEnv->pSystem)->GetMaxConfigSpec())
	{
		spec = ((CSystem*)gEnv->pSystem)->GetMaxConfigSpec();
		pVar->Set(spec);
	}

	CryLog("OnSysSpecChange(%d)",spec);

	switch (spec)
	{
	case CONFIG_LOW_SPEC:
		GetISystem()->LoadConfiguration( "LowSpec.cfg" );
		break;
	case CONFIG_MEDIUM_SPEC:
		GetISystem()->LoadConfiguration( "MedSpec.cfg" );
		break;
	case CONFIG_HIGH_SPEC:
		GetISystem()->LoadConfiguration( "HighSpec.cfg" );
		break;
	case CONFIG_VERYHIGH_SPEC:
		GetISystem()->LoadConfiguration( "VeryHighSpec.cfg" );
		break;
	case CONFIG_X360:
		GetISystem()->LoadConfiguration( "console.cfg" );
		GetISystem()->LoadConfiguration( "x360.cfg" );
		break;
	case CONFIG_PS3:
		GetISystem()->LoadConfiguration( "console.cfg" );
		GetISystem()->LoadConfiguration( "ps3.cfg" );
		break;

	default:
		// Do nothing.
		break;
	}

  // make sure editor specific settings are not changed
  if(gEnv->pSystem->IsEditorMode())
    GetISystem()->LoadConfiguration( "editor.cfg" );

	bool bChangeServerSpec = true;
	if (gEnv->pGame && gEnv->bMultiplayer)
		bChangeServerSpec = false;
	if (bChangeServerSpec)
		GetISystem()->SetConfigSpec( (ESystemConfigSpec)spec,false );

	no_recursive = false;
}

//////////////////////////////////////////////////////////////////////////
struct SCryEngineLanguageConfigLoader : public ILoadConfigurationEntrySink
{
	CSystem *m_pSystem;
	string m_language;
	string m_pakFile;

	SCryEngineLanguageConfigLoader( CSystem *pSystem ) { m_pSystem = pSystem; }
	void Load( const char *sCfgFilename )
	{
		CSystemConfiguration cfg(sCfgFilename,m_pSystem,this); // Parse folders config file.
	}
	virtual void OnLoadConfigurationEntry( const char *szKey, const char *szValue, const char *szGroup )
	{
		if (stricmp(szKey,"Language") == 0)
		{
			m_language = szValue;
		} else if (stricmp(szKey,"PAK") == 0)
		{
			m_pakFile = szValue;
		}
	}
	virtual void OnLoadConfigurationEntry_End() {}
};

//////////////////////////////////////////////////////////////////////////
struct SCryEngineFolderCfgLoader : public ILoadConfigurationEntrySink
{
public:
	CSystem *m_pSystem;

	SCryEngineFolderCfgLoader( CSystem *pSystem ) { m_pSystem = pSystem; }
	void Load( const char *sCfgFilename )
	{
		CSystemConfiguration cfg(sCfgFilename,m_pSystem,this); // Parse folders config file.
	}
	virtual void OnLoadConfigurationEntry( const char *szKey, const char *szValue, const char *szGroup )
	{
		if (stricmp(szKey,CRYENGINE_GAME_FOLDER_KEY) == 0)
		{
			m_pSystem->OnLoadConfigurationEntry(szKey,szValue,szGroup);
		}
		if (stricmp(szKey,CRYENGINE_USER_FOLDER_KEY) == 0)
		{
			m_pSystem->OnLoadConfigurationEntry(szKey,szValue,szGroup);
		}
	}
	virtual void OnLoadConfigurationEntry_End() {}
};

//////////////////////////////////////////////////////////////////////////
#if !defined(_LIB) && !defined(PS3)
WIN_HMODULE CSystem::LoadDynamiclibrary( const char *dllName ) const
{
	WIN_HMODULE handle = NULL;
#ifdef WIN32
	if (m_binariesDir.empty())
	{
		handle = CryLoadLibrary(dllName);
		if (!handle)
		{
			DWORD dwErrorCode = GetLastError();
			CryLogAlways( "DLL Failed to load, error code: %X",dwErrorCode );
		}
	}
	else
	{
		char currentDirectory[1024];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		SetCurrentDirectory(m_binariesDir.c_str());
		handle = CryLoadLibrary(dllName);
		SetCurrentDirectory(currentDirectory);
	}
#else
	handle = CryLoadLibrary(dllName);
#endif
	return handle;
}

WIN_HMODULE CSystem::LoadDLL( const char *dllName,bool bQuitIfNotFound)
{ 
	LOADING_TIME_PROFILE_SECTION(GetISystem());

	CryComment("Loading DLL: %s",dllName);

	WIN_HMODULE handle = LoadDynamiclibrary(dllName); 

	if (!handle)      
	{
#if defined(LINUX)
		printf ("Error loading DLL: %s, error :  %s\n", dllName, dlerror());
		if (bQuitIfNotFound)
			Quit();
		else
			return 0;
#else
		if (bQuitIfNotFound)
		{		
			CryFatalError( "Error loading DLL: %s, error code %d",dllName, GetLastError());
			Quit();
		}
		return 0;
#endif //LINUX
	}
#if defined(_DATAPROBE) && !defined(LINUX) && !defined(XENON) && !defined(PS3)
	IDataProbe::SModuleInfo module;
	module.filename = dllName;
	module.handle = gDLLHandle;
	m_pDataProbe->AddModule(module);
#endif

	//////////////////////////////////////////////////////////////////////////
	// After loading DLL initialize it by calling ModuleInitISystem
	//////////////////////////////////////////////////////////////////////////
	string moduleName = PathUtil::GetFileName(dllName);

	typedef void  *(*PtrFunc_ModuleInitISystem)(ISystem *pSystem,const char *moduleName);
	PtrFunc_ModuleInitISystem pfnModuleInitISystem = (PtrFunc_ModuleInitISystem) CryGetProcAddress(handle,DLL_MODULE_INIT_ISYSTEM);
	if (pfnModuleInitISystem)
	{
		pfnModuleInitISystem( this,moduleName.c_str() );
	}

	return handle;
}
#endif //#if !defined(_LIB) && !defined(LINUX) && !defined(PS3)

//////////////////////////////////////////////////////////////////////////
bool CSystem::LoadEngineDLLs()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::IntializeEngineModule( const char *dllName,const char *moduleClassName,const SSystemInitParams &initParams, bool bQuitIfNotFound )
{
	if (m_pUserCallback)
	{
		stack_string msg;
		msg = "Initializing ";
		msg += dllName;
		m_pUserCallback->OnInitProgress( msg.c_str() );
	}
	CryLog( "Initializing %s ...",dllName );

	IMemoryManager::SProcessMemInfo memStart,memEnd;
  if(GetIMemoryManager())
	  GetIMemoryManager()->GetProcessMemInfo(memStart);
  else
    ZeroStruct(memStart);

	stack_string dllfile = dllName;
	
#if defined(LINUX) || defined(PS3)
	dllfile = PathUtil::ReplaceExtension(dllfile,"so");
#ifndef LINUX
	dllfile.MakeLower();
#endif
#else
	dllfile = PathUtil::ReplaceExtension(dllfile,"dll");
#endif

#if !defined(_LIB)
	WIN_HMODULE hModule = LoadDLL(dllfile.c_str(), bQuitIfNotFound);
	if(!hModule)
		return (!bQuitIfNotFound);
	m_moduleDLLHandles.push_back(hModule);
#else



#endif //_LIB

	bool bResult = false;
	cryshared_ptr<IEngineModule> pModule;
	if (CryCreateClassInstance( moduleClassName,pModule ))
	{
		bResult = pModule->Initialize( m_env,initParams );
	}

  if(GetIMemoryManager())
  {
	  GetIMemoryManager()->GetProcessMemInfo(memEnd);

	  uint64 memUsed = memEnd.WorkingSetSize - memStart.WorkingSetSize;
	  CryLog( "Initializing %s done, MemUsage=%dKb",dllName,uint32(memUsed/1024) );
  }

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::OpenRenderLibrary(const char *t_rend, const SSystemInitParams &initParams )
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());





	if (IsDedicated())
		return OpenRenderLibrary(R_NULL_RENDERER,initParams);

	if (stricmp(t_rend, "OpenGL") == 0)
    return OpenRenderLibrary(R_GL_RENDERER,initParams);
  else
  if (stricmp(t_rend, "DX9") == 0)
	{
//		if (GetConfigSpec() > CONFIG_HIGH_SPEC)
//			SetConfigSpec(CONFIG_HIGH_SPEC, true);
    return OpenRenderLibrary(R_DX9_RENDERER,initParams);
	}
	else
		if (stricmp(t_rend, "DX10") == 0)
			return OpenRenderLibrary(R_DX11_RENDERER,initParams);
	else
  if (stricmp(t_rend, "DX11") == 0)
    return OpenRenderLibrary(R_DX11_RENDERER,initParams);
  else
  if (stricmp(t_rend, "NULL") == 0)
    return OpenRenderLibrary(R_NULL_RENDERER,initParams);

	CryFatalError ("Unknown renderer type: %s", t_rend);
	return false;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

#if defined(WIN32) || defined(WIN64)
wstring CSystem::GetErrorStringUnsupportedGPU(const char* gpuName, unsigned int gpuVendorId, unsigned int gpuDeviceId)
{
	ICVar* pCVarGLang(m_env.pConsole->GetCVar("g_language"));

	string lang;
	if (pCVarGLang)
		lang = pCVarGLang->GetString();

	wchar_t* unsupportedGPU(0);
	wchar_t* vendor(0);
	wchar_t* device(0);
	wchar_t* hint(0);

	wchar_t* pFmtMsgBuf(0);

	if (stricmp(lang.c_str(), "english") == 0)
	{
		unsupportedGPU = L"Unsupported video card!\n\n";
		vendor = L"vendor = ";
		device = L"device = ";
		hint = L"Continuing to run might lead to unexpected results or crashes.";
	}
	else
	{		
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, E_NOINTERFACE, 0, (wchar_t*) &pFmtMsgBuf, 0, 0);
		unsupportedGPU = L"";
		vendor = L"";
		device = L"";
		hint = pFmtMsgBuf;
	}

	wchar_t msg[1024]; msg[0] = L'\0'; msg[sizeof(msg) / sizeof(msg[0]) - 1] = L'\0';
	_snwprintf(msg, sizeof(msg) / sizeof(msg[0]) - 1, L"%s\"%S\" [%s0x%.4x, %s0x%.4x]\n\n%s", unsupportedGPU, gpuName, vendor, gpuVendorId, device, gpuDeviceId, hint);

	if (pFmtMsgBuf)
		LocalFree(pFmtMsgBuf);

	return msg;
}
#endif

bool CSystem::OpenRenderLibrary(int type,const SSystemInitParams &initParams)
{
#if defined(WIN32) || defined(WIN64)
	if (type == R_DX9_RENDERER && !m_env.IsEditor())
	{
		unsigned int gpuVendorId(0), gpuDeviceId(0), totVidMem(0);
		bool supportsSM20orAbove(false);
		char gpuName[256];
		Win32SysInspect::GetGPUInfo(gpuName, sizeof(gpuName), gpuVendorId, gpuDeviceId, totVidMem, supportsSM20orAbove);
		int gpuRating(Win32SysInspect::GetGPURating(gpuVendorId, gpuDeviceId));
		if (gpuRating < 0 || !supportsSM20orAbove)
		{
			int mbRes(MessageBoxW(0, GetErrorStringUnsupportedGPU(gpuName, gpuVendorId, gpuDeviceId).c_str(), L"CryEngine", MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON2));
			const char logMsgFmt[]("Unsupported video card!\n- %s (vendor = 0x%.4x, device = 0x%.4x)\n- Video memory: %d MB\n- Minimum SM 2.0 support: %s\n- Rating: %d\n%s");
			CryLogAlways(logMsgFmt, gpuName, gpuVendorId, gpuDeviceId, totVidMem >> 20, (supportsSM20orAbove == true) ? "yes" : "no", gpuRating, mbRes == IDCANCEL ? "User chose to quit!" : "User chose to continue!");
			if (mbRes == IDCANCEL)
				return false;
		}
	}
#endif

#if defined(_XBOX) || (defined(LINUX) && !defined(DEDICATED_SERVER))
  type = R_DX9_RENDERER;
#endif

#if defined(DEDICATED_SERVER)
	type = R_NULL_RENDERER;
#else
	if (IsDedicated())
		type = R_NULL_RENDERER;
#endif

	
	
	const char* libname = "";
	if (type == R_GL_RENDERER)
		libname = DLL_RENDERER_OGL;
	else if (type == R_DX9_RENDERER)
		libname = DLL_RENDERER_DX9;
	else if (type == R_DX11_RENDERER)
		libname = DLL_RENDERER_DX11;
	else if (type == R_NULL_RENDERER)
		libname = DLL_RENDERER_NULL;
	else
	{
		CryFatalError("No renderer specified!");
		return false;
	}

	if (!IntializeEngineModule( libname,"EngineModule_CryRenderer",initParams ))
	{
		return false;
	}

	if (!m_env.pRenderer)
	{
		CryFatalError("Error creating Render System!");
		return false;
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitNetwork( const SSystemInitParams &initParams )
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());





	if (!IntializeEngineModule( DLL_NETWORK,"EngineModule_CryNetwork",initParams ))
		return false;


	if(m_env.pNetwork==NULL)
	{
		CryFatalError( "Error creating Network System!" );
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitEntitySystem( const SSystemInitParams &initParams )
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

	if (!IntializeEngineModule( DLL_ENTITYSYSTEM,"EngineModule_CryEntitySystem",initParams ))
		return false;

	if (!m_env.pEntitySystem)
	{
		CryFatalError( "Error creating Entity System!");
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::InitInput( const SSystemInitParams &initParams )
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

	if (initParams.bSkipInput)
	{
		m_env.pInput = new CNullInput();
		return true;
	}

	if (!IntializeEngineModule( DLL_INPUT,"EngineModule_CryInput",initParams ))
	{
#ifdef WIN32
			MessageBox( NULL,"CryInput.dll could not be loaded. This is likely due to not having XInput support installed.\nPlease install the most recent version of the DirectX runtime." ,"ERROR: CryInput.dll could not be loaded!",MB_OK|MB_ICONERROR );
#endif
		return false;
	}

	if (!m_env.pInput)
	{
		CryFatalError( "Error creating Input System!" );
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitConsole()
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

//	m_Console->Init(this);
	// Ignore when run in Editor.
	if (m_bEditor && !m_env.pRenderer)
		return true;

	// Ignore for dedicated server.
	if (IsDedicated())
		return true;

/*	char *filename = "Textures/Defaults/DefaultConsole.dds"; // this path is set by Harry
	if (filename)
	{
		ITexture * conimage = m_env.pRenderer->EF_LoadTexture(filename, FT_DONT_STREAM|FT_DONT_RESIZE, eTT_2D);
		if (conimage)
  		m_env.pConsole->SetImage(conimage,false);
	}
	else
	{
		CryFatalError("Error: Cannot open %s", filename);
	}*/

	return (true);
}

//////////////////////////////////////////////////////////////////////////
// attaches the given variable to the given container;
// recreates the variable if necessary
ICVar* CSystem::attachVariable (const char* szVarName, int* pContainer, const char*szComment,int dwFlags)
{
	IConsole* pConsole = GetIConsole();

	ICVar* pOldVar = pConsole->GetCVar (szVarName);
	int nDefault;
	if (pOldVar)
	{
		nDefault = pOldVar->GetIVal();
		pConsole->UnregisterVariable(szVarName, true);
	}

	// NOTE: maybe we should preserve the actual value of the variable across the registration,
	// because of the strange architecture of IConsole that converts int->float->int

	REGISTER_CVAR2(szVarName, pContainer, *pContainer, dwFlags, szComment);

	ICVar* pVar = pConsole->GetCVar(szVarName);

#ifdef _DEBUG
	// test if the variable really has this container
	assert (*pContainer == pVar->GetIVal());
	++*pContainer;
	assert (*pContainer == pVar->GetIVal());
	--*pContainer;
#endif

	if (pOldVar)
	{
		// carry on the default value from the old variable anyway
		pVar->Set(nDefault);
	}
	return pVar;
}

/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitRenderer(WIN_HINSTANCE hinst, WIN_HWND hwnd, const SSystemInitParams &initParams )
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

	if (m_pUserCallback)
		m_pUserCallback->OnInitProgress( "Initializing Renderer..." );

	if (m_bEditor)
	{
		m_env.pConsole->GetCVar("r_Width");

		// save current screen width/height/bpp, so they can be restored on shutdown
		m_iWidth = m_env.pConsole->GetCVar("r_Width")->GetIVal();
		m_iHeight = m_env.pConsole->GetCVar("r_Height")->GetIVal();
		m_iColorBits = m_env.pConsole->GetCVar("r_ColorBits")->GetIVal();
	}

	if (!OpenRenderLibrary(m_rDriver->GetString(),initParams))
		return false;

#ifdef WIN32

	if(!m_bDedicatedServer)
	{
		// [marco] If a previous instance is running, activate
		// the old one and terminate the new one, depending
		// on command line devmode status - is done here for
		// smart people that double-click 20 times on the game
		// icon even before the previous detection on the main
		// executable has time to be executed - so now the game will quit
		HWND hwndPrev;
//Diesel cut
		//static char szWndClass[] = "CryENGINE";
//Diesel new
		static char szWndClass[] = "StEgine 3206";
		
		// in devmode we don't care, we allow to run multiple instances
		// for mp debugging
		if (!m_bInDevMode)
		{
			hwndPrev = FindWindow (szWndClass, NULL);
			// not in devmode and we found another window - see if the
			// system is relaunching, in this case is fine 'cos the application
			// will be closed immediately after
			if (hwndPrev && (hwndPrev!=m_hWnd) && !m_bRelaunched)
			{
				SetForegroundWindow (hwndPrev);
				MessageBox( NULL,"You cannot start multiple instances of Crysis!\nThe program will now quit.\n" ,"ERROR: STARTING MULTIPLE INSTANCES OF CRYSIS ",MB_OK|MB_ICONERROR );
				Quit();
			}
		}
	}
#endif

#ifdef WIN32
	if (m_env.pRenderer)
	{
		if(m_env.pHardwareMouse)
			m_env.pHardwareMouse->OnPreInitRenderer();
		
		SCustomRenderInitArgs args;
		args.appStartedFromMediaCenter = strstr(initParams.szSystemCmdLine, "ReLaunchMediaCenter") != 0;

		m_hWnd = m_env.pRenderer->Init(0, 0, m_rWidth->GetIVal(), m_rHeight->GetIVal(), m_rColorBits->GetIVal(), m_rDepthBits->GetIVal(), m_rStencilBits->GetIVal(), m_rFullscreen->GetIVal() ? true : false, hinst, hwnd, false, &args);		
		//Timur, Not very clean code, we need to push new hwnd value to the system init params, so other modules can used when initializing.
		(const_cast<SSystemInitParams*>(&initParams))->hWnd = m_hWnd;

		InitPhysicsRenderer(initParams);

		return m_hWnd != 0;
	}
#else
	if (m_env.pRenderer)
	{
		WIN_HWND h = m_env.pRenderer->Init(0, 0, m_rWidth->GetIVal(), m_rHeight->GetIVal(), m_rColorBits->GetIVal(), m_rDepthBits->GetIVal(), m_rStencilBits->GetIVal(), m_rFullscreen->GetIVal() ? true : false, hinst, hwnd);
		InitPhysicsRenderer(initParams);

#if defined(_XBOX) || defined(XENON) || defined(PS3) || defined(LINUX)
		return true;
#else
		if (h)
			return true;
		return (false);
#endif 
	}
#endif
	return true;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitSound( const SSystemInitParams &initParams )
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

	if (!IntializeEngineModule( DLL_SOUND,"EngineModule_CrySoundSystem",initParams, false ))
		return false;

	if (!m_env.pSoundSystem)
	{
		// assign SoundSystemNULL
		m_env.pSoundSystem = m_pNULLSoundSystem;
		
		CryWarning(VALIDATOR_MODULE_SOUNDSYSTEM, VALIDATOR_ERROR, "<Sound> Couldnt create the sound system interface");
		CryLogAlways("<Sound> Using DummySoundSystem now!");
		return false;
	}

	m_env.pMusicSystem = m_env.pSoundSystem->GetInterfaceExtended()->CreateMusicSystem();

	if (!m_env.pMusicSystem)
	{
		m_env.pMusicSystem = m_pNULLSoundSystem->CreateMusicSystem();
		CryWarning(VALIDATOR_MODULE_SOUNDSYSTEM, VALIDATOR_ERROR, "<Sound> Couldnt create the music system interface");
		return false; 
	}
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////////

char *PhysHelpersToStr(int iHelpers, char *strHelpers)
{
	char *ptr = strHelpers;
	if (iHelpers & 128) *ptr++ = 't';
	if (iHelpers & 256) *ptr++ = 's';
	if (iHelpers & 512) *ptr++ = 'r';
	if (iHelpers & 1024) *ptr++ = 'R';
	if (iHelpers & 2048) *ptr++ = 'l';
	if (iHelpers & 4096) *ptr++ = 'i';
	if (iHelpers & 8192) *ptr++ = 'e';
	if (iHelpers & 16384) *ptr++ = 'g';
	if (iHelpers & 32768) *ptr++ = 'w';
	if (iHelpers & 32) *ptr++ = 'a';
	if (iHelpers & 64) *ptr++ = 'y';
	*ptr++ = iHelpers ? '_' : '0';
	if (iHelpers & 1) *ptr++ = 'c';
	if (iHelpers & 2) *ptr++ = 'g';
	if (iHelpers & 4) *ptr++ = 'b';
	if (iHelpers & 8) *ptr++ = 'l';
	if (iHelpers & 16) *ptr++ = 'j';
	if (iHelpers>>16) 
		if (!(iHelpers & 1<<27))
			ptr += sprintf(ptr,"t(%d)",iHelpers>>16);
		else for(int i=0;i<16;i++) if (i!=11 && iHelpers & 1<<(16+i))
			ptr += sprintf(ptr,"f(%d)",i);
	*ptr++ = 0;
	return strHelpers;
}

int StrToPhysHelpers(const char *strHelpers)
{
	const char *ptr;
	int iHelpers=0,level;
	if (*strHelpers=='1')
		return 7970;
	if (*strHelpers=='2')
		return 7970 | 1<<31 | 1<<27;
	for(ptr=strHelpers; *ptr && *ptr!='_'; ptr++) switch(*ptr) 
	{
		case 't': iHelpers |= 128; break;
		case 's': iHelpers |= 256; break;
		case 'r': iHelpers |= 512; break;
		case 'R': iHelpers |= 1024; break;
		case 'l': iHelpers |= 2048; break;
		case 'i': iHelpers |= 4096; break;
		case 'e': iHelpers |= 8192; break;
		case 'g': iHelpers |= 16384; break;
		case 'w': iHelpers |= 32768; break;
		case 'a': iHelpers |= 32; break;
		case 'y': iHelpers |= 64; break;
	}
	if (*ptr=='_') ptr++;
	for(; *ptr; ptr++) switch(*ptr)
	{
		case 'c': iHelpers |= 1; break;
		case 'g': iHelpers |= 2; break;
		case 'b': iHelpers |= 4; break;
		case 'l': iHelpers |= 8; break;
		case 'j': iHelpers |= 16; break;
		case 'f': 
			if (*++ptr && *++ptr) for(level=0; *(ptr+1) && *ptr!=')'; ptr++) level = level*10+*ptr-'0'; 
			iHelpers |= 1<<(16+level) | 1<<27; break;
		case 't': 
			if (*++ptr && *++ptr) for(level=0; *(ptr+1) && *ptr!=')'; ptr++) level = level*10+*ptr-'0'; 
			iHelpers |= level<<16 | 2; 
	}
	return iHelpers;
}

void OnDrawHelpersStrChange(ICVar *pVar)
{
	gEnv->pPhysicalWorld->GetPhysVars()->iDrawHelpers = StrToPhysHelpers(pVar->GetString());
}

bool CSystem::InitPhysics( const SSystemInitParams &initParams )
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Physics, 0, "Init Physics");




	if (!IntializeEngineModule( DLL_PHYSICS,"EngineModule_CryPhysics",initParams ))
		return false;


	if(!m_env.pPhysicalWorld)
	{
		CryFatalError( "Error creating Physics System!" );
		return false;
	}
	//m_env.pPhysicalWorld->Init();	// don't need a second Init, the world is created initialized

	// Register physics console variables.
	IConsole *pConsole = GetIConsole();

  PhysicsVars *pVars = m_env.pPhysicalWorld->GetPhysVars();

  REGISTER_CVAR2("p_fly_mode", &pVars->bFlyMode, pVars->bFlyMode,VF_CHEAT,
		"Toggles fly mode.\n"
		"Usage: p_fly_mode [0/1]");
  REGISTER_CVAR2("p_collision_mode", &pVars->iCollisionMode, pVars->iCollisionMode,VF_CHEAT,
		"This variable is obsolete.");
  REGISTER_CVAR2("p_single_step_mode", &pVars->bSingleStepMode, pVars->bSingleStepMode,VF_CHEAT,
		"Toggles physics system 'single step' mode."
		"Usage: p_single_step_mode [0/1]\n"
		"Default is 0 (off). Set to 1 to switch physics system (except\n"
		"players) to single step mode. Each step must be explicitly\n"
		"requested with a 'p_do_step' instruction.");
  REGISTER_CVAR2("p_do_step", &pVars->bDoStep, pVars->bDoStep,VF_CHEAT,
		"Steps physics system forward when in single step mode.\n"
		"Usage: p_do_step 1\n"
		"Default is 0 (off). Each 'p_do_step 1' instruction allows\n"
		"the physics system to advance a single step.");
  REGISTER_CVAR2("p_fixed_timestep", &pVars->fixedTimestep, pVars->fixedTimestep,VF_CHEAT,
		"Toggles fixed time step mode."
		"Usage: p_fixed_timestep [0/1]\n"
		"Forces fixed time step when set to 1. When set to 0, the\n"
		"time step is variable, based on the frame rate.");
  REGISTER_CVAR2("p_draw_helpers_num", &pVars->iDrawHelpers, pVars->iDrawHelpers,VF_CHEAT,
		"Toggles display of various physical helpers. The value is a bitmask:\n"
		"bit 0  - show contact points\n"
		"bit 1  - show physical geometry\n"
		"bit 8  - show helpers for static objects\n"
		"bit 9  - show helpers for sleeping physicalized objects (rigid bodies, ragdolls)\n"
		"bit 10 - show helpers for active physicalized objects\n"
		"bit 11 - show helpers for players\n"
		"bit 12 - show helpers for independent entities (alive physical skeletons,particles,ropes)\n"
		"bits 16-31 - level of bounding volume trees to display (if 0, it just shows geometry)\n"
		"Examples: show static objects - 258, show active rigid bodies - 1026, show players - 2050");
  REGISTER_CVAR2("p_max_contact_gap", &pVars->maxContactGap, pVars->maxContactGap, 0,
		"Sets the gap, enforced whenever possible, between\n"
		"contacting physical objects."
		"Usage: p_max_contact_gap 0.01\n"
		"This variable is used for internal tweaking only.");
  REGISTER_CVAR2("p_max_contact_gap_player", &pVars->maxContactGapPlayer, pVars->maxContactGapPlayer, 0,
		"Sets the safe contact gap for player collisions with\n"
		"the physical environment."
		"Usage: p_max_contact_gap_player 0.01\n"
		"This variable is used for internal tweaking only.");
  REGISTER_CVAR2("p_gravity_z", &pVars->gravity.z, pVars->gravity.z, 0,"");
  REGISTER_CVAR2("p_max_substeps", &pVars->nMaxSubsteps, pVars->nMaxSubsteps, 0,
		"Limits the number of substeps allowed in variable time step mode.\n"
		"Usage: p_max_substeps 5\n"
		"Objects that are not allowed to perform time steps\n"
		"beyond some value make several substeps.");
	REGISTER_CVAR2("p_prohibit_unprojection", &pVars->bProhibitUnprojection, pVars->bProhibitUnprojection, 0,
		"This variable is obsolete.");
	REGISTER_CVAR2("p_enforce_contacts", &pVars->bEnforceContacts, pVars->bEnforceContacts, 0,
		"This variable is obsolete.");
	REGISTER_CVAR2("p_damping_group_size", &pVars->nGroupDamping, pVars->nGroupDamping, 0,
		"Sets contacting objects group size\n"
		"before group damping is used."
		"Usage: p_damping_group_size 3\n"
		"Used for internal tweaking only.");
	REGISTER_CVAR2("p_group_damping", &pVars->groupDamping, pVars->groupDamping, 0,
		"Toggles damping for object groups.\n"
		"Usage: p_group_damping [0/1]\n"
		"Default is 1 (on). Used for internal tweaking only.");
	REGISTER_CVAR2("p_max_substeps_large_group", &pVars->nMaxSubstepsLargeGroup, pVars->nMaxSubstepsLargeGroup, 0,
		"Limits the number of substeps large groups of objects can make");
	REGISTER_CVAR2("p_num_bodies_large_group", &pVars->nBodiesLargeGroup, pVars->nBodiesLargeGroup, 0,
		"Group size to be used with p_max_substeps_large_group, in bodies");
	REGISTER_CVAR2("p_break_on_validation", &pVars->bBreakOnValidation, pVars->bBreakOnValidation, 0,
		"Toggles break on validation error.\n"
		"Usage: p_break_on_validation [0/1]\n"
		"Default is 0 (off). Issues DebugBreak() call in case of\n"
		"a physics parameter validation error.");
	REGISTER_CVAR2("p_time_granularity", &pVars->timeGranularity, pVars->timeGranularity, 0,
		"Sets physical time step granularity.\n"
		"Usage: p_time_granularity [0..0.1]\n"
		"Used for internal tweaking only.");
	REGISTER_CVAR2("p_list_active_objects", &pVars->bLogActiveObjects, pVars->bLogActiveObjects,VF_NULL,"");
	REGISTER_CVAR2("p_profile_entities", &pVars->bProfileEntities, pVars->bProfileEntities, 0,
		"Enables per-entity time step profiling");
	REGISTER_CVAR2("p_profile_functions", &pVars->bProfileFunx, pVars->bProfileFunx, 0,
		"Enables detailed profiling of physical environment-sampling functions");
	REGISTER_CVAR2("p_GEB_max_cells", &pVars->nGEBMaxCells, pVars->nGEBMaxCells, 0,
		"Specifies the cell number threshold after which GetEntitiesInBox issues a warning");
	REGISTER_CVAR2("p_max_velocity", &pVars->maxVel, pVars->maxVel, 0,
		"Clamps physicalized objects' velocities to this value");
	REGISTER_CVAR2("p_max_player_velocity", &pVars->maxVelPlayers, pVars->maxVelPlayers, 0,
		"Clamps players' velocities to this value");

	REGISTER_CVAR2("p_max_MC_iters", &pVars->nMaxMCiters, pVars->nMaxMCiters, 0,
		"Specifies the maximum number of microcontact solver iterations *per contact*");
	REGISTER_CVAR2("p_min_MC_iters", &pVars->nMinMCiters, pVars->nMinMCiters, 0,
		"Specifies the minmum number of microcontact solver iterations *per contact set* (this has precedence over p_max_mc_iters)");
	REGISTER_CVAR2("p_accuracy_MC", &pVars->accuracyMC, pVars->accuracyMC, 0,
		"Desired accuracy of microcontact solver (velocity-related, m/s)");
	REGISTER_CVAR2("p_accuracy_LCPCG", &pVars->accuracyLCPCG, pVars->accuracyLCPCG, 0,
		"Desired accuracy of LCP CG solver (velocity-related, m/s)");
	REGISTER_CVAR2("p_max_contacts", &pVars->nMaxContacts, pVars->nMaxContacts, 0,
		"Maximum contact number, after which contact reduction mode is activated");
	REGISTER_CVAR2("p_max_plane_contacts", &pVars->nMaxPlaneContacts, pVars->nMaxPlaneContacts, 0,
		"Maximum number of contacts lying in one plane between two rigid bodies\n"
		"(the system tries to remove the least important contacts to get to this value)");
	REGISTER_CVAR2("p_max_plane_contacts_distress", &pVars->nMaxPlaneContactsDistress, pVars->nMaxPlaneContactsDistress, 0,
		"Same as p_max_plane_contacts, but is effective if total number of contacts is above p_max_contacts");
	REGISTER_CVAR2("p_max_LCPCG_subiters", &pVars->nMaxLCPCGsubiters, pVars->nMaxLCPCGsubiters, 0,
		"Limits the number of LCP CG solver inner iterations (should be of the order of the number of contacts)");
	REGISTER_CVAR2("p_max_LCPCG_subiters_final", &pVars->nMaxLCPCGsubitersFinal, pVars->nMaxLCPCGsubitersFinal, 0,
		"Limits the number of LCP CG solver inner iterations during the final iteration (should be of the order of the number of contacts)");
	REGISTER_CVAR2("p_max_LCPCG_microiters", &pVars->nMaxLCPCGmicroiters, pVars->nMaxLCPCGmicroiters, 0,
		"Limits the total number of per-contact iterations during one LCP CG iteration\n"
		"(number of microiters = number of subiters * number of contacts)");
	REGISTER_CVAR2("p_max_LCPCG_microiters_final", &pVars->nMaxLCPCGmicroitersFinal, pVars->nMaxLCPCGmicroitersFinal, 0,
		"Same as p_max_LCPCG_microiters, but for the final LCP CG iteration");
	REGISTER_CVAR2("p_max_LCPCG_iters", &pVars->nMaxLCPCGiters, pVars->nMaxLCPCGiters, 0,
		"Maximum number of LCP CG iterations");
	REGISTER_CVAR2("p_min_LCPCG_improvement", &pVars->minLCPCGimprovement, pVars->minLCPCGimprovement, 0,
		"Defines a required residual squared length improvement, in fractions of 1");
	REGISTER_CVAR2("p_max_LCPCG_fruitless_iters", &pVars->nMaxLCPCGFruitlessIters, pVars->nMaxLCPCGFruitlessIters, 0,
		"Maximum number of LCP CG iterations w/o improvement (defined by p_min_LCPCGimprovement)");
	REGISTER_CVAR2("p_accuracy_LCPCG_no_improvement", &pVars->accuracyLCPCGnoimprovement, pVars->accuracyLCPCGnoimprovement, 0,
		"Required LCP CG accuracy that allows to stop if there was no improvement after p_max_LCPCG_fruitless_iters");
	REGISTER_CVAR2("p_min_separation_speed", &pVars->minSeparationSpeed, pVars->minSeparationSpeed, 0,
		"Used a threshold in some places (namely, to determine when a particle\n"
		"goes to rest, and a sliding condition in microcontact solver)");
	REGISTER_CVAR2("p_use_distance_contacts", &pVars->bUseDistanceContacts, pVars->bUseDistanceContacts, 0,
		"Allows to use distance-based contacts (is forced off in multiplayer)");
	REGISTER_CVAR2("p_unproj_vel_scale", &pVars->unprojVelScale, pVars->unprojVelScale, 0,
		"Requested unprojection velocity is set equal to penetration depth multiplied by this number");
	REGISTER_CVAR2("p_max_unproj_vel", &pVars->maxUnprojVel, pVars->maxUnprojVel, 0,
		"Limits the maximum unprojection velocity request");
	REGISTER_CVAR2("p_penalty_scale", &pVars->penaltyScale, pVars->penaltyScale, 0, 
		"Scales the penalty impulse for objects that use the simple solver");
	REGISTER_CVAR2("p_max_contact_gap_simple", &pVars->maxContactGapSimple, pVars->maxContactGapSimple, 0, 
		"Specifies the maximum contact gap for objects that use the simple solver");
	REGISTER_CVAR2("p_skip_redundant_colldet", &pVars->bSkipRedundantColldet, pVars->bSkipRedundantColldet, 0, 
		"Specifies whether to skip furher collision checks between two convex objects using the simple solver\n"
		"when they have enough contacts between them");
	REGISTER_CVAR2("p_limit_simple_solver_energy", &pVars->bLimitSimpleSolverEnergy, pVars->bLimitSimpleSolverEnergy, 0, 
		"Specifies whether the energy added by the simple solver is limited (0 or 1)");
	REGISTER_CVAR2("p_max_world_step", &pVars->maxWorldStep, pVars->maxWorldStep, 0, 
		"Specifies the maximum step physical world can make (larger steps will be truncated)");
	REGISTER_CVAR2("p_use_unproj_vel", &pVars->bCGUnprojVel, pVars->bCGUnprojVel, 0, "internal solver tweak");
	REGISTER_CVAR2("p_tick_breakable", &pVars->tickBreakable, pVars->tickBreakable, 0, 
		"Sets the breakable objects structure update interval");
	REGISTER_CVAR2("p_log_lattice_tension", &pVars->bLogLatticeTension, pVars->bLogLatticeTension, 0, 
		"If set, breakable objects will log tensions at the weakest spots");
	REGISTER_CVAR2("p_debug_joints", &pVars->bLogLatticeTension, pVars->bLogLatticeTension, 0, 
		"If set, breakable objects will log tensions at the weakest spots");
	REGISTER_CVAR2("p_lattice_max_iters", &pVars->nMaxLatticeIters, pVars->nMaxLatticeIters, 0, 
		"Limits the number of iterations of lattice tension solver");
	REGISTER_CVAR2("p_max_entity_cells", &pVars->nMaxEntityCells, pVars->nMaxEntityCells, 0,
		"Limits the number of entity grid cells an entity can occupy");
	REGISTER_CVAR2("p_max_MC_mass_ratio", &pVars->maxMCMassRatio, pVars->maxMCMassRatio, 0, 
		"Maximum mass ratio between objects in an island that MC solver is considered safe to handle");
	REGISTER_CVAR2("p_max_MC_vel", &pVars->maxMCVel, pVars->maxMCVel, 0, 
		"Maximum object velocity in an island that MC solver is considered safe to handle");
	REGISTER_CVAR2("p_max_LCPCG_contacts", &pVars->maxLCPCGContacts, pVars->maxLCPCGContacts, 0, 
		"Maximum number of contacts that LCPCG solver is allowed to handle");
	REGISTER_CVAR2("p_approx_caps_len", &pVars->approxCapsLen, pVars->approxCapsLen, 0, 
		"Breakable trees are approximated with capsules of this length (0 disables approximation)");
	REGISTER_CVAR2("p_max_approx_caps", &pVars->nMaxApproxCaps, pVars->nMaxApproxCaps, 0, 
		"Maximum number of capsule approximation levels for breakable trees");
	REGISTER_CVAR2("p_players_can_break", &pVars->bPlayersCanBreak, pVars->bPlayersCanBreak, 0, 
		"Whether living entities are allowed to break static objects with breakable joints");
	REGISTER_CVAR2("p_max_debris_mass", &pVars->massLimitDebris, 10.0f, 0, 
		"Broken pieces with mass<=this limit use debris collision settings");
	REGISTER_CVAR2("p_max_object_splashes", &pVars->maxSplashesPerObj, pVars->maxSplashesPerObj, 0, 
		"Specifies how many splash events one entity is allowed to generate");
	REGISTER_CVAR2("p_splash_dist0", &pVars->splashDist0, pVars->splashDist0, 0, 
		"Range start for splash event distance culling");
	REGISTER_CVAR2("p_splash_force0", &pVars->minSplashForce0, pVars->minSplashForce0, 0, 
		"Minimum water hit force to generate splash events at p_splash_dist0");
	REGISTER_CVAR2("p_splash_vel0", &pVars->minSplashVel0, pVars->minSplashVel0, 0, 
		"Minimum water hit velocity to generate splash events at p_splash_dist0");
	REGISTER_CVAR2("p_splash_dist1", &pVars->splashDist1, pVars->splashDist1, 0, 
		"Range end for splash event distance culling");
	REGISTER_CVAR2("p_splash_force1", &pVars->minSplashForce1, pVars->minSplashForce1, 0, 
		"Minimum water hit force to generate splash events at p_splash_dist1");
	REGISTER_CVAR2("p_splash_vel1", &pVars->minSplashVel1, pVars->minSplashVel1, 0, 
		"Minimum water hit velocity to generate splash events at p_splash_dist1");
	REGISTER_CVAR2("p_joint_gravity_step", &pVars->jointGravityStep, pVars->jointGravityStep, 0, 
		"Time step used for gravity in breakable joints (larger = stronger gravity effects)");
	REGISTER_CVAR2("p_debug_explosions", &pVars->bDebugExplosions, pVars->bDebugExplosions, 0, 
		"Turns on explosions debug mode");
	REGISTER_CVAR2("p_num_threads", &pVars->numThreads, pVars->numThreads, 0,
		"The number of internal physics threads");
	REGISTER_CVAR2("p_num_jobs", &pVars->numJobs, pVars->numJobs, 0,
		"Specifies the number of jobs used for job based phys.sim.(used by SPUs)");

	REGISTER_CVAR2("p_net_minsnapdist", &pVars->netMinSnapDist, pVars->netMinSnapDist, 0,
		"Minimum distance between server position and client position at which to start snapping");
	REGISTER_CVAR2("p_net_velsnapmul", &pVars->netVelSnapMul, pVars->netVelSnapMul, 0,
		"Multiplier to expand the p_net_minsnapdist based on the objects velocity");
	REGISTER_CVAR2("p_net_minsnapdot", &pVars->netMinSnapDot, pVars->netMinSnapDot, 0,
		"Minimum quat dot product between server orientation and client orientation at which to start snapping");
	REGISTER_CVAR2("p_net_angsnapmul", &pVars->netAngSnapMul, pVars->netAngSnapMul, 0,
		"Multiplier to expand the p_net_minsnapdot based on the objects angular velocity");
	REGISTER_CVAR2("p_net_smoothtime", &pVars->netSmoothTime, pVars->netSmoothTime, 0,
		"How much time should non-snapped positions take to synchronize completely?");

	pVars->flagsColliderDebris = geom_colltype_debris;
	pVars->flagsANDDebris = ~(geom_colltype_vehicle|geom_colltype6);
#ifdef USE_FRAME_PROFILER
	pVars->ticksPerSecond = (int64)(1.0f/CFrameProfilerTimer::TicksToMilliseconds(1))*1000;
#endif // USE_FRAME_PROFILER

#ifndef EXCLUDE_SCALEFORM_SDK
	CFlashPlayer::InitCVars();
#endif

	if (m_bEditor)
	{
		// Setup physical grid for Editor.
		int nCellSize = 16;
		m_env.pPhysicalWorld->SetupEntityGrid(2,Vec3(0,0,0), (2048)/nCellSize,(2048)/nCellSize, (float)nCellSize,(float)nCellSize);
		pConsole->CreateKeyBind("comma", "#System.SetCVar(\"p_single_step_mode\",1-System.GetCVar(\"p_single_step_mode\"));");
		pConsole->CreateKeyBind("period", "p_do_step 1");
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::InitPhysicsRenderer( const SSystemInitParams &initParams )
{
	//////////////////////////////////////////////////////////////////////////
	// Physics Renderer (for debug helpers)
	//////////////////////////////////////////////////////////////////////////
	if (!initParams.bSkipRenderer && !m_bUIFrameworkMode)
	{
		m_pPhysRenderer = new CPhysRenderer;
		m_pPhysRenderer->Init(); // needs to be created after physics and renderer
		m_pPhysRenderer->SetCamera(&m_PhysRendererCamera);
		m_p_draw_helpers_str = REGISTER_STRING("p_draw_helpers","0",VF_CHEAT,
			"Same as p_draw_helpers_num, but encoded in letters\n"
			"Usage [Entity_Types]_[Helper_Types] - [t|s|r|R|l|i|g|a|y|e]_[g|c|b|l|t(#)]\n"
			"Entity Types:\n"
			"t - show terrain\n"
			"s - show static entities\n"
			"r - show sleeping rigid bodies\n"
			"R - show active rigid bodies\n"
			"l - show living entities\n"
			"i - show independent entities\n"
			"g - show triggers\n"
			"a - show areas\n"
			"y - show rays in RayWorldIntersection\n"
			"e - show explosion occlusion maps\n"
			"Helper Types\n"
			"g - show geometry\n"
			"c - show contact points\n"
			"b - show bounding boxes\n"
			"l - show tetrahedra lattices for breakable objects\n"
			"j - show structural joints (will force translucency on the main geometry)\n"
			"t(#) - show bounding volume trees up to the level #\n"
			"f(#) - only show geometries with this bit flag set (multiple f\'s stack)\n"
			"Example: p_draw_helpers larRis_g - show geometry for static, sleeping, active, independent entities and areas");
		
		assert(m_p_draw_helpers_str);
		m_p_draw_helpers_str->SetOnChangeCallback(OnDrawHelpersStrChange);

		REGISTER_CVAR2("p_cull_distance", &m_pPhysRenderer->m_cullDist, m_pPhysRenderer->m_cullDist, 0,
			"Culling distance for physics helpers rendering");
		REGISTER_CVAR2("p_wireframe_distance", &m_pPhysRenderer->m_wireframeDist, m_pPhysRenderer->m_wireframeDist, 0,
			"Maximum distance at which wireframe is drawn on physics helpers");
		REGISTER_CVAR2("p_ray_fadein", &m_pPhysRenderer->m_timeRayFadein, m_pPhysRenderer->m_timeRayFadein, 0,
			"Fade-in time for ray physics helpers");
		REGISTER_CVAR2("p_ray_peak_time", &m_pPhysRenderer->m_rayPeakTime, m_pPhysRenderer->m_rayPeakTime, 0,
			"Rays that take longer then this (in ms) will use different color");
		REGISTER_CVAR2("p_proxy_highlight_threshold", &m_pPhysRenderer->m_maxTris, m_pPhysRenderer->m_maxTris, 0,
			"Physics proxies with triangle counts large than this will be highlighted");
		REGISTER_CVAR2("p_proxy_highlight_range", &m_pPhysRenderer->m_maxTrisRange, m_pPhysRenderer->m_maxTrisRange, 0,
			"Physics proxies with triangle counts >= p_proxy_highlight_threshold+p_proxy_highlight_range will get the maximum highlight");
		REGISTER_CVAR2("p_jump_to_profile_ent", &(m_iJumpToPhysProfileEnt=0), 0, 0,
			"Move the local player next to the corresponding entity in the p_profile_entities list");
		GetIConsole()->CreateKeyBind("alt 1", "p_jump_to_profile_ent 1");
		GetIConsole()->CreateKeyBind("alt 2", "p_jump_to_profile_ent 2");
		GetIConsole()->CreateKeyBind("alt 3", "p_jump_to_profile_ent 3");
		GetIConsole()->CreateKeyBind("alt 4", "p_jump_to_profile_ent 4");
		GetIConsole()->CreateKeyBind("alt 5", "p_jump_to_profile_ent 5");
	}
	return true;
}

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
bool CSystem::InitGPUPhysics()
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

	if (m_pUserCallback)
		m_pUserCallback->OnInitProgress( "Initializing GPU Physics..." );

#if !defined(_LIB) && !defined(LINUX) && !defined(PS3)
	//m_dll.hPhysicsGPU = LoadDLL(DLL_GPU_PHYSICS, false);
	m_dll.hPhysicsGPU = LoadDLL( DLL_GPU_PHYSICS, false );

	if (m_dll.hPhysicsGPU)
	{
		pfnCreateGPUPhysics = (CreateGPUPhysicsProc) CryGetProcAddress(m_dll.hPhysicsGPU, "CreateGPUPhysics" );

		if( ( pfnCreateGPUPhysics != NULL ) )
		{
			m_pGPUPhysics = pfnCreateGPUPhysics( this );
		} 
		else 
		{
			m_pGPUPhysics = NULL;
			pfnCreateGPUPhysics	= NULL;

			CryWarning(VALIDATOR_MODULE_PHYSICS, VALIDATOR_WARNING, "Couldn't load all required functions from %s" DLL_GPU_PHYSICS );
		}
	} 
	else 
	{
		CryWarning(VALIDATOR_MODULE_PHYSICS, VALIDATOR_WARNING, "Couldn't load %s" DLL_GPU_PHYSICS);
		return false;
	}

#else //_LIB
	m_pGPUPhysics = CreateGPUPhysics(this);
#endif //_LIB

	return true;

}
#endif

/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitMovieSystem( const SSystemInitParams &initParams )
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

//#if defined(_LIB) && defined(XENON)
//	m_env.pMovieSystem = CreateMovieSystem(this);
//#else
	if (!IntializeEngineModule( DLL_MOVIE,"EngineModule_CryMovie",initParams ))
		return false;
//#endif

	if (!m_env.pMovieSystem)
	{
		CryFatalError("Error creating Movie System!");
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitAISystem( const SSystemInitParams &initParams )
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());
	
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Init AISystem ");

//#if defined(_LIB) && defined(XENON)
//	m_env.pAISystem = CreateAISystem(this);
//#else
	const char * sDLLName = m_sys_dll_ai->GetString();
	if (!IntializeEngineModule( sDLLName,"EngineModule_CryAISystem",initParams,false ))
		return false;
//#endif


	if (!m_env.pAISystem)
		CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"Cannot create AI System!" );

	return true;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitScriptSystem( const SSystemInitParams &initParams )
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_LUA, 0, "Init ScriptSystem");

//#if defined(_LIB) && defined(XENON)
//	m_env.pScriptSystem = CreateScriptSystem(this, true);
//#else
	if (!IntializeEngineModule( DLL_SCRIPTSYSTEM,"EngineModule_CryScriptSystem",initParams ))
		return false;
//#endif


	if(m_env.pScriptSystem==NULL)
	{
		CryFatalError( "Error creating Script System!" );
		return (false);
	}

	m_env.pScriptSystem->PostInit();

	// Load script surface types.	
	if (m_env.pScriptSystem)
		m_env.pScriptSystem->LoadScriptedSurfaceTypes( "Scripts/Materials",false );

	return (true);
}

/////////////////////////////////////////////////////////////////////////////////




bool CSystem::InitFileSystem()
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

	if (m_pUserCallback)
		m_pUserCallback->OnInitProgress( "Initializing File System..." );

	const ICmdLineArg *pArg = m_pCmdLine->FindArg(eCLAT_Pre,"LvlRes");			// -LvlRes command line option

	bool bLvlRes=false;								// true: all assets since executable start are recorded, false otherwise

	if(pArg)
		bLvlRes = true;														


	CCryPak *pCryPak = new CCryPak(m_env.pLog,&m_PakVar,bLvlRes);
	m_env.pCryPak = pCryPak;

	pCryPak->SetGameFolderWritable(m_bGameFolderWritable);
	
	if (m_bEditor || bLvlRes)
		m_env.pCryPak->RecordFileOpen(ICryPak::RFOM_EngineStartup);

	bool bRes=m_env.pCryPak->Init("");

	if (bRes)
	{	
		const ICmdLineArg *pakalias = m_pCmdLine->FindArg(eCLAT_Pre,"pakalias");
		if (pakalias && strlen(pakalias->GetValue())>0)
			m_env.pCryPak->ParseAliases(pakalias->GetValue());
	}

	// Create Engine folder mod mapping only for Engine assets
	pCryPak->AddMod( CRYENGINE_ENGINE_FOLDER );

	return (bRes);
}

/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitFileSystem_LoadEngineFolders()
{
	// Load value of sys_game_folder from system.cfg into the sys_game_folder console variable
	{
		SCryEngineFolderCfgLoader cfgGameFolderLoader(this);
		cfgGameFolderLoader.Load( "system.cfg" );
	}

	// We set now the correct "game" folder to use in Pak File
	m_env.pCryPak->SetGameFolder(m_sys_game_folder->GetString());

	// Load engine folders.
	ChangeUserPath( m_sys_user_folder->GetString() );

	if (const ICmdLineArg *pModArg = GetICmdLine()->FindArg(eCLAT_Pre,"MOD"))
	{
		if (IsMODValid(pModArg->GetValue()))
		{	
			string modPath;
			modPath.append("Mods\\");
			modPath.append(pModArg->GetValue());
			modPath.append("\\");

			m_env.pCryPak->AddMod(modPath.c_str());
		}
	}

	return (true);
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::InitStreamEngine()
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

	if (m_pUserCallback)
		m_pUserCallback->OnInitProgress( "Initializing Stream Engine..." );

	m_pStreamEngine = new CStreamEngine((CCryPak*)m_env.pCryPak, m_env.pLog, this);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitFont( const SSystemInitParams &initParams )
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Init FontSystem");

	if (!IntializeEngineModule( DLL_FONT,"EngineModule_CryFont",initParams ))
		return false;

	if(!m_env.pCryFont)
	{
		CryFatalError( "Error creating Font System!" );
		return false;
	}

	if (IsDedicated())
		return true;

	// Load the default font
	IFFont *pConsoleFont = m_env.pCryFont->NewFont("console");
	m_pIFont = m_env.pCryFont->NewFont("default");
	if(!m_pIFont || !pConsoleFont)
	{
		CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_ERROR,"Error creating the default fonts" );
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	string szFontPath = "Fonts/default.xml";

	if(!m_pIFont->Load(szFontPath.c_str()))
	{
		string szError = "Error loading the default font from ";
		szError += szFontPath;
		szError += ". You're probably running the executable from the wrong working folder.";
		CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_ERROR,szError.c_str());

		//return false;
	}

	int n = szFontPath.find("default.xml");
	assert(n != string::npos);

	szFontPath.replace(n, strlen("default.xml"), "console.xml");

	if(!pConsoleFont->Load(szFontPath.c_str()))
	{
		string szError = "Error loading the console font from ";
		szError += szFontPath;
		szError += ". You're probably running the executable from the wrong working folder.";
		CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_ERROR,szError.c_str() );

		//return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::Init3DEngine( const SSystemInitParams &initParams )
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Init 3D Engine");

	if (!IntializeEngineModule( DLL_3DENGINE,"EngineModule_Cry3DEngine",initParams ))
		return false;

  if (!m_env.p3DEngine )
	{
    CryFatalError( "Error creating 3D Engine!" );
		return false;
	}

	if (!m_env.p3DEngine->Init())
	{
		CryFatalError( "Error initializing 3D Engine!" );
		return false;
	}
	m_pProcess = m_env.p3DEngine;
	m_pProcess->SetFlags(PROC_3DENGINE);

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::InitAnimationSystem( const SSystemInitParams &initParams )
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Init AnimationSystem");

	if (!IntializeEngineModule( DLL_ANIMATION,"EngineModule_CryAnimation",initParams ))
		return false;

	return m_env.pCharacterManager != NULL;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::InitVTuneProfiler()
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

#ifdef PROFILE_WITH_VTUNE
	HMODULE hModule = CryLoadLibrary( "VTuneApi.dll" );
	if (hModule)
	{
		VTPause = (VTuneFunction) CryGetProcAddress( hModule, "VTPause");
		VTResume = (VTuneFunction) CryGetProcAddress( hModule, "VTResume");
		if (!VTPause || !VTResume)
			CryFatalError( "Failed to find VTPause" );
		else
			CryLogAlways( "VTune API Initialized" );
	}
	else
	{
		CryFatalError( "Failed to load VTuneAPI.dll" );
	}
#endif //PROFILE_WITH_VTUNE
}

//////////////////////////////////////////////////////////////////////////
void CSystem::InitLocalization()
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

	SCryEngineLanguageConfigLoader defaultLanguageLoader(this);
	defaultLanguageLoader.Load( PathUtil::GetGameFolder() + CRYENGINE_DEFAULT_LANGUAGE_CONFIG_FILE );
	string language = defaultLanguageLoader.m_language;
	if (language.empty())
	{
		language = "english";
	}

	if (m_pLocalizationManager == NULL)
		m_pLocalizationManager = new CLocalizedStringsManager(this);

	ICVar *pCVar = m_env.pConsole != 0 ? m_env.pConsole->GetCVar( "g_language" ) : 0;
	if (pCVar)
	{
		if (strlen(pCVar->GetString()) == 0)
		{
			pCVar->Set( language );
		}
		else
		{
			language = pCVar->GetString();
		}
	}
	GetLocalizationManager()->SetLanguage( language );
}

/*
static void ClampSysSpecVars()
{
	int iMaxSysSpec = ((CSystem*)gEnv->pSystem)->GetMaxConfigSpec();

	const char *szPrefix = "sys_spec_";

	std::vector<const char*> cmds;

	cmds.resize(gEnv->pConsole->GetSortedVars(0,0,szPrefix));
	gEnv->pConsole->GetSortedVars(&cmds[0],cmds.size(),szPrefix);

	std::vector<const char*>::const_iterator it, end=cmds.end();

	for(it=cmds.begin();it!=end;++it)
	{
		const char *szName = *it;

		if(stricmp(szName,"sys_spec_Full")==0)			// clamp it on a sys_spec_* level, not at top level (sys_spec)
			continue;

		ICVar *pCVar = gEnv->pConsole->GetCVar(szName);			assert(pCVar);

		if(!pCVar)
			continue;

		int iSpec = pCVar->GetIVal();

		// intended behavior: you still can change all controlled cvars,
		// we only clamp the convenient way changing them through sys_spec_ cvars and menu UI
		if(iSpec>iMaxSysSpec)
		{
//			CryLog("DEBUG ClampSysSpecVars %s was: %d",pCVar->GetName(),iSpec);			// TODO: remove this line
			pCVar->Set(iMaxSysSpec);
		}
	}
}
*/




























string GetUniqueLogFileName(string logFileName)
{
#if defined(WIN32) || (defined (XENON) && !defined(_RELEASE))
	int instance = gEnv->pSystem->GetApplicationInstance();
	if(instance != 0)
	{
		string logFileExtension;
		size_t extensionIndex = logFileName.find_last_of('.');
		string logFileNamePrefix = logFileName;
		if(extensionIndex != string::npos)
		{
			logFileExtension = logFileName.substr(extensionIndex, logFileName.length() - extensionIndex);
			logFileNamePrefix = logFileName.substr(0, extensionIndex);
		}
		logFileName.Format("%s(%d)%s", logFileNamePrefix.c_str(), instance, logFileExtension.c_str());
	}
#endif

	return logFileName;
}



void OnLevelLoadingDump( ICVar *pArgs )
{
	gEnv->pSystem->OutputLoadingTimeStats();
}

// System initialization
/////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////
bool CSystem::Init(const SSystemInitParams &startupParams)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "System Initialization");

//#ifdef XENON
//	XSetFileCacheSize(4*1024*1024);
//#endif

	m_env.bTesting = startupParams.bTesting;
	m_env.bIgnoreAllAsserts = startupParams.bTesting;
  assert(IsHeapValid());

#ifdef EXTENSION_SYSTEM_INCLUDE_TESTCASES
	TestExtensions(&CCryFactoryRegistryImpl::Access());
#endif

	//_controlfp(0, _EM_INVALID|_EM_ZERODIVIDE | _PC_64 );

#if defined(WIN32) || defined(WIN64)
	// check OS version - we only want to run on XP or higher - talk to Martin Mittring if you want to change this
	{
		OSVERSIONINFO osvi;

		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

		GetVersionExA(&osvi);

		bool bIsWindowsXPorLater = osvi.dwMajorVersion>5 || ( osvi.dwMajorVersion==5 && osvi.dwMinorVersion>=1 );

		if(!bIsWindowsXPorLater)
		{
			CryFatalError("Windows XP or later is required");
			return false;
		}
	}
#endif

	m_pResourceManager->Init();

	// Get file version information.
	QueryVersionInfo();
	DetectGameFolderAccessRights();

	m_hInst = (WIN_HINSTANCE)startupParams.hInstance;
	m_hWnd = (WIN_HWND)startupParams.hWnd;

	m_bEditor = startupParams.bEditor;
	m_bPreviewMode = startupParams.bPreview;
	m_bUIFrameworkMode = startupParams.bUIFramework;
	m_bTestMode = startupParams.bTestMode;
	m_pUserCallback = startupParams.pUserCallback;
	m_bDedicatedServer = startupParams.bDedicatedServer;
	m_pCmdLine = new CCmdLine(startupParams.szSystemCmdLine);
	m_bNoCrashDialog = false;
	memcpy( gEnv->pProtectedFunctions,startupParams.pProtectedFunctions,sizeof(startupParams.pProtectedFunctions) );

	m_env.bEditor = m_bEditor;
	m_env.bEditorGameMode = false;

	m_env.bIsOutOfMemory = false;

	if (m_bEditor)
		m_bInDevMode = true;

	const ICmdLineArg * crashdialog = m_pCmdLine->FindArg(eCLAT_Post,"sys_no_crash_dialog");
	if (crashdialog)
	{
		m_bNoCrashDialog = true;
	}

	if (!startupParams.pValidator)
	{
		m_pDefaultValidator = new SDefaultValidator(this);
		m_pValidator = m_pDefaultValidator;
	}
	else
	{
		m_pValidator = startupParams.pValidator;
	}

	if (!m_bDedicatedServer)
	{
		const ICmdLineArg *dedicated = m_pCmdLine->FindArg(eCLAT_Pre,"dedicated");
		if (dedicated)
			m_bDedicatedServer = true;
	}

#if !defined(PS3) && !defined(XENON)
	bool isDaemonMode = (m_pCmdLine->FindArg(eCLAT_Pre, "daemon") != 0);
#if !defined(USE_UNIXCONSOLE)
	CNULLConsole* pConsole = new CNULLConsole(isDaemonMode);
	g_pUnixConsole = pConsole;

	if (m_pUserCallback == NULL && m_bDedicatedServer)
		m_pUserCallback = pConsole;
#else
	bool isSimpleConsole = (m_pCmdLine->FindArg(eCLAT_Pre, "simple_console") != 0);
	if (isDaemonMode || isSimpleConsole)
	{
		CNULLConsole* pConsole = new CNULLConsole(isDaemonMode);
		g_pUnixConsole = pConsole;

		if (m_pUserCallback == NULL && m_bDedicatedServer)
			m_pUserCallback = pConsole;
	}
	else
	{
		CUNIXConsole* pConsole = new CUNIXConsole();
		g_pUnixConsole = pConsole;

		if (m_pUserCallback == NULL && m_bDedicatedServer)
		{
			char headerString[128];
		m_pUserCallback = pConsole;
		pConsole->SetRequireDedicatedServer(true);
			strcpy(
					headerString,
				"CryEngine - "
#if defined(LINUX)
					"Linux "
#endif
					"Dedicated Server"
					" - Version ");
			GetProductVersion().ToString(headerString + strlen(headerString));
		pConsole->SetHeader(headerString);
		}
	}
#endif // USE_UNIXCONSOLE
#endif // !defined(PS3) && !defined(XENON)

	{
		LOADING_TIME_PROFILE_SECTION(this);
	//////////////////////////////////////////////////////////////////////////
	// File system, must be very early
	//////////////////////////////////////////////////////////////////////////
	InitFileSystem();
	//////////////////////////////////////////////////////////////////////////

	CLoadingProfilerSystem::Init();

	const ICmdLineArg* root = m_pCmdLine->FindArg(eCLAT_Pre,"root");
	if (root)
	{
		string temp = PathUtil::ToUnixPath(PathUtil::AddSlash(root->GetValue()));
		if ( gEnv->pCryPak->MakeDir(temp.c_str()) )
			m_root = temp;
	}

	//////////////////////////////////////////////////////////////////////////
	// Logging is only available after file system initialization.
	//////////////////////////////////////////////////////////////////////////
	if (!startupParams.pLog)
	{
		m_env.pLog = new CLog(this);
		if (startupParams.pLogCallback)
			m_env.pLog->AddCallback(startupParams.pLogCallback);

		const ICmdLineArg *logfile = m_pCmdLine->FindArg(eCLAT_Pre,"logfile");
		if (logfile && strlen(logfile->GetValue()) > 0)
			if ( m_env.pLog->SetFileName(logfile->GetValue()) )
				goto L_done;
		if (startupParams.sLogFileName)
		{
			const string sUniqueLogFileName = GetUniqueLogFileName(startupParams.sLogFileName);
			if ( m_env.pLog->SetFileName(sUniqueLogFileName.c_str()) )
				goto L_done;
		}
		m_env.pLog->SetFileName(DEFAULT_LOG_FILENAME);
L_done:;
	}
	else
	{
		m_env.pLog = startupParams.pLog;
	}

	LogVersion();

	m_env.pLog->Log("Executable Command Line: %s\n",startupParams.szSystemCmdLine);

	((CCryPak*)m_env.pCryPak)->SetLog( m_env.pLog );

	m_env.pProfileLogSystem = new CProfileLogSystem();
	//////////////////////////////////////////////////////////////////////////
	// CREATE NOTIFICATION NETWORK
	//////////////////////////////////////////////////////////////////////////
#ifndef LINUX
	if (!startupParams.bMinimal)
	{
		if (m_pNotificationNetwork = CNotificationNetwork::Create())
		{
			m_pNotificationNetwork->ListenerBind("HotUpdate", &CHotUpdateNotification::Instance());
		}	
	}
#endif
	//////////////////////////////////////////////////////////////////////////
	// CREATE CONSOLE
	//////////////////////////////////////////////////////////////////////////
	if (!startupParams.bSkipConsole)
	{
		m_env.pConsole = new CXConsole;
		((CTestSystemLegacy*)m_pTestSystem)->Init(m_env.pConsole);
	}
	

	if (m_pUserCallback)
		m_pUserCallback->OnInit(this);

	m_env.pLog->RegisterConsoleVariables();

	if (!startupParams.bSkipConsole)
	{
		// Register system console variables.
		CreateSystemVars();
		
		if (*startupParams.szUserPath)
			m_sys_user_folder->Set( startupParams.szUserPath );
	}

	/*
	if(m_bEditor || m_pCmdLine->FindArg(eCLAT_Pre,"devmode"))
		SetDevMode(true);											// In Dev mode.
	else
		SetDevMode(true);										// Dev mode currently enabled by default!!!.
	*/

	SetDevMode(true);										// Dev mode currently enabled by default!!!.

	InitFileSystem_LoadEngineFolders();

	// CPU features detection.
	m_pCpu = new CCpuFeatures;
	m_pCpu->Detect();
	m_env.pi.numCoresAvailableToProcess = m_pCpu->GetCPUCount();

	if (!startupParams.bSkipConsole)
	{
		LogSystemInfo();
	}

	//////////////////////////////////////////////////////////////////////////

	AddCVarGroupDirectory(string(m_env.pCryPak->GetGameFolder()) + "/Config/CVarGroups");

	//////////////////////////////////////////////////////////////////////////
	// Stream Engine
	//////////////////////////////////////////////////////////////////////////
	CryLogAlways("Stream Engine Initialization");
	InitStreamEngine();

	//////////////////////////////////////////////////////////////////////////
	//Load config files
	//////////////////////////////////////////////////////////////////////////
#if !defined(PS3)	
	// load the game.cfg unless we're told to reset the profile
	if (m_pCmdLine->FindArg(eCLAT_Pre, "ResetProfile") == 0)
		LoadConfiguration("%USER%/game.cfg");
#endif









		

	LoadConfiguration("system.cfg"); // We have to load this file again since first time we did it without devmode
	LoadConfiguration("user.cfg");
//	if (!g_sysSpecChanged)
//		OnSysSpecChange( m_sys_spec );

	{
		if (m_pCmdLine->FindArg(eCLAT_Pre,"DX9"))
			m_env.pConsole->LoadConfigVar("r_Driver","DX9");
		else
			if (m_pCmdLine->FindArg(eCLAT_Pre,"DX10"))
				m_env.pConsole->LoadConfigVar("r_Driver","DX11");
		else
		if (m_pCmdLine->FindArg(eCLAT_Pre,"DX11"))
			m_env.pConsole->LoadConfigVar("r_Driver","DX11");
	}

	LogBuildInfo();























	if (!startupParams.bSkipRenderer)
	{
		m_FrameProfileSystem.Init( this, m_sys_profile_allThreads->GetIVal() );
		CreateRendererVars(startupParams);
	}

	if(m_bDedicatedServer && m_rDriver)
	{
		m_sSavedRDriver=m_rDriver->GetString();
		m_rDriver->Set("NULL");
	}


#if defined(WIN32) || defined(WIN64)
	if (!startupParams.bSkipRenderer)
	{
		if (stricmp(m_rDriver->GetString(), "Auto") == 0)
		{
			//if (Win32SysInspect::IsDX11Supported())
			//	m_rDriver->Set("DX11");
			// else
				m_rDriver->Set("DX9");
		}
	}
#endif

#ifdef WIN32
	if (g_cvars.sys_WER) SetUnhandledExceptionFilter(CryEngineExceptionFilterWER);
#endif

	if (!startupParams.bMinimal)
	{
		InitLocalization();
	}
	m_binariesDir = startupParams.szBinariesDir;

	//////////////////////////////////////////////////////////////////////////
	// Open basic pak files.
	//////////////////////////////////////////////////////////////////////////
	if (!startupParams.bMinimal)
	{
		OpenBasicPaks();
	}

	//////////////////////////////////////////////////////////////////////////
	// NETWORK
	//////////////////////////////////////////////////////////////////////////
	if (!startupParams.bPreview && !m_bUIFrameworkMode)
	{
		CryLogAlways("Network initialization");
		InitNetwork( startupParams );

		if (IsDedicated())
			m_pServerThrottle.reset( new CServerThrottle(this, m_pCpu->GetCPUCount()) );
	}
	//////////////////////////////////////////////////////////////////////////
	// PHYSICS
	//////////////////////////////////////////////////////////////////////////
	//if (!params.bPreview)
	if(!m_bUIFrameworkMode)
	{
		CryLogAlways("Physics initialization");
		if (!InitPhysics( startupParams ))
			return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// MOVIE
	//////////////////////////////////////////////////////////////////////////
	if(!m_bUIFrameworkMode)
	{
		CryLogAlways("MovieSystem initialization");
		if (!InitMovieSystem(startupParams))
			return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// FONT
	//////////////////////////////////////////////////////////////////////////
	if (!startupParams.bSkipFont)
	{
		CryLogAlways("Font initialization");
		if (!InitFont(startupParams))
			return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// RENDERER
	//////////////////////////////////////////////////////////////////////////
	if (!startupParams.bSkipRenderer)
	{
		CryLogAlways("Renderer initialization");
		if (!InitRenderer(m_hInst, (startupParams.bEditor)? (WIN_HWND)1 : m_hWnd, startupParams))
			return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// Hardware mouse
	//////////////////////////////////////////////////////////////////////////
	// - Dedicated server is in console mode by default (Hardware Mouse is always shown when console is)
	// - Mouse is always visible by default in Editor (we never start directly in Game Mode)
	// - Mouse has to be enabled manually by the Game (this is typically done in the main menu)
#ifdef LINUX
	m_env.pHardwareMouse = NULL;
#else
	m_env.pHardwareMouse = new CHardwareMouse(true);
#endif

	//////////////////////////////////////////////////////////////////////////
	// CONSOLE
	//////////////////////////////////////////////////////////////////////////
	if (!startupParams.bSkipConsole)
	{
		CryLogAlways("Console initialization");
		if (!InitConsole())
			return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// THREAD PROFILER
	//////////////////////////////////////////////////////////////////////////
	m_pThreadProfiler = new CThreadProfiler;

	//////////////////////////////////////////////////////////////////////////
	// DISK PROFILER
	//////////////////////////////////////////////////////////////////////////
	m_pDiskProfiler = new CDiskProfiler(this);

  // TIME
  //////////////////////////////////////////////////////////////////////////
  CryLogAlways("Time initialization");
  if (!m_Time.Init(this))
    return (false);
  m_Time.ResetTimer();

	//////////////////////////////////////////////////////////////////////////
	// INPUT
	//////////////////////////////////////////////////////////////////////////
	if (!startupParams.bPreview && !IsDedicated())
	{
		CryLogAlways("Input initialization");
		if (!InitInput(startupParams)) // !!! TODO: FIX ME !!!
			return false;

		if(m_env.pHardwareMouse)
			m_env.pHardwareMouse->OnPostInitInput();
	}
	
	//////////////////////////////////////////////////////////////////////////
	// Create PlatformOS
	//////////////////////////////////////////////////////////////////////////
	m_pPlatformOS.reset(IPlatformOS::Create());

	//////////////////////////////////////////////////////////////////////////
	// Create MiniGUI
	//////////////////////////////////////////////////////////////////////////
	cryshared_ptr<minigui::IMiniGUI> pMiniGUI;
	if (CryCreateClassInstanceForInterface( cryiidof<minigui::IMiniGUI>(),pMiniGUI ))
	{
		m_pMiniGUI = pMiniGUI.get();
		m_pMiniGUI->Init();
	}

	//////////////////////////////////////////////////////////////////////////
	// SOUND
	//////////////////////////////////////////////////////////////////////////
	// NULL IMPLEMENTATIONS
	m_pNULLSoundSystem = new CNULLSoundSystem();
	// assign SoundSystemNULL
	m_env.pSoundSystem = m_pNULLSoundSystem;
	m_env.pMusicSystem = m_pNULLSoundSystem->CreateMusicSystem();

	//////////////////////////////////////////////////////////////////////////
	if (!startupParams.bPreview && !m_bDedicatedServer && !m_bUIFrameworkMode)
	{
		CryLogAlways("Sound initialization");
		bool bResult = InitSound( startupParams );
		//if (!bResult)
			//return false;
	}

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
	/////////////////////////////////////////////////////////////////	
	// GPU PHYSICS
	// note GPU physics needs to be instanciated after the renderer!
	/////////////////////////////////////////////////////////////////	//if (!params.bPreview)
	if(!m_bUIFrameworkMode)
	{
		if (m_gpu_particle_physics->GetIVal())
		{
			CryLogAlways("GPUPhysics initialization");
			bool bResult = InitGPUPhysics();
			if (!bResult)
			{
				CryLogAlways("GPUPhysics initialization failed!");
				m_gpu_particle_physics->Set(0);
			}

			//GPU physics is optional, so no need to fail if load doesn't happen.
			//if (!bResult)
			//return false;
		}
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// AI
	//////////////////////////////////////////////////////////////////////////
	if (!startupParams.bPreview && !m_bUIFrameworkMode)
	{
		if (IsDedicated() && m_svAISystem && !m_svAISystem->GetIVal())
			;
		else
		{
			CryLogAlways("AI initialization");
			if (!InitAISystem( startupParams ))
				return false;
		}
	}

	if (m_env.pConsole != 0)
		((CXConsole*)m_env.pConsole)->Init(this);

	//////////////////////////////////////////////////////////////////////////
	// Init Animation system
	//////////////////////////////////////////////////////////////////////////
	CryLogAlways("Initializing Animation System");
	if(!m_bUIFrameworkMode)
		if (!InitAnimationSystem( startupParams ))
			return false;
	//////////////////////////////////////////////////////////////////////////
	// Init 3d engine
	//////////////////////////////////////////////////////////////////////////
	if (!startupParams.bSkipRenderer)
	{
		CryLogAlways("Initializing 3D Engine");
		if (!Init3DEngine(startupParams))
			return false;
	}

#ifdef DOWNLOAD_MANAGER
	m_pDownloadManager = new CDownloadManager;
	m_pDownloadManager->Create(this);
#endif //DOWNLOAD_MANAGER

//#ifndef MEM_STD
//  REGISTER_COMMAND("MemStats",::DumpAllocs,"");
//#endif

	//////////////////////////////////////////////////////////////////////////
	// SCRIPT SYSTEM
	//////////////////////////////////////////////////////////////////////////
	// We need script materials for now 

	// if (!startupParams.bPreview)
	if (!startupParams.bSkipRenderer)
	{
		CryLogAlways("Script System Initialization");
		if (!InitScriptSystem( startupParams ))
			return false;
	}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// ENTITY SYSTEM
	//////////////////////////////////////////////////////////////////////////
	if (!startupParams.bPreview)
	{
		CryLogAlways("Entity system initialization");
		if (!InitEntitySystem(startupParams))
			return false;
	}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// AI SYSTEM INITIALIZATION
	//////////////////////////////////////////////////////////////////////////
	// AI System needs to be initialized after entity system
	if (!startupParams.bPreview && !m_bUIFrameworkMode && m_env.pAISystem)
	{
		if (m_pUserCallback)
			m_pUserCallback->OnInitProgress( "Initializing AI System..." );
		CryLogAlways("Initializing AI System");
		m_env.pAISystem->Init();
	}

	/*
#ifdef SECUROM_32
	{
		CDataProbe probe;
		if (!probe.CheckPaul())	
		{
			Strange();
		}
	}
#endif
	*/

	//////////////////////////////////////////////////////////////////////////
	// BUDGETING SYSTEM
	m_pIBudgetingSystem = new CBudgetingSystem();


	//////////////////////////////////////////////////////////////////////////
	// Create PerfHUD
	//////////////////////////////////////////////////////////////////////////

	if (!gEnv->bTesting)
	{
		//Create late in Init so that associated CVars have already been created
		cryshared_ptr<ICryPerfHUD> pPerfHUD;
		if (CryCreateClassInstanceForInterface( cryiidof<ICryPerfHUD>(),pPerfHUD ))
		{
			m_pPerfHUD = pPerfHUD.get();
			m_pPerfHUD->Init();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Visual Regression Tests
	//////////////////////////////////////////////////////////////////////////
	if( m_pCmdLine->FindArg( eCLAT_Pre, "visregtest" ) )
	{
		m_pVisRegTest = new CVisRegTest();
		m_pVisRegTest->Init();
	}

	//////////////////////////////////////////////////////////////////////////
	// Check loader.
	//////////////////////////////////////////////////////////////////////////
#if defined(_DATAPROBE) && defined(_CHECKLOADER) && !defined(LINUX)
	CDataProbe probe;
	if (!startupParams.pCheckFunc || !probe.CheckLoader( startupParams.pCheckFunc ))
	{
		Strange();
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Initialize task threads.
	//////////////////////////////////////////////////////////////////////////
	if (!startupParams.bSkipRenderer)
	{
		m_pThreadTaskManager->InitThreads();

		SetAffinity();
		assert(IsHeapValid());


		if (strstr(startupParams.szSystemCmdLine,"-VTUNE") != 0 || g_cvars.sys_vtune != 0)
			InitVTuneProfiler();


		RegisterEngineStatistics();
	}

#if !defined(PS3) && !defined(XENON)
	m_env.pLocalMemoryUsage = new CLocalMemoryUsage();


#endif 

#if defined(WIN32)
#	if !defined(WIN64)
	_controlfp(_PC_64,_MCW_PC); // not supported on WIN64
#	endif
#endif

	if ( g_cvars.sys_float_exceptions > 0 )
			CryLogAlways("enabled float exceptions(sys_float_exceptions %d) this makes the performance slower.",g_cvars.sys_float_exceptions);
	EnableFloatExceptions( g_cvars.sys_float_exceptions );












	MarkThisThreadForDebugging("Main");
















	}
	CLoadingProfilerSystem::SaveTimeContainersToFile("EngineStart.xml");

  return (true);
}


static void LoadConfigurationCmd( IConsoleCmdArgs *pParams )
{
	assert(pParams);

	if(pParams->GetArgCount()!=2)
	{
		gEnv->pLog->LogError("LoadConfiguration failed, one parameter needed");
		return;
	}

	GetISystem()->LoadConfiguration(PathUtil::GetGameFolder() + "/Config/" + pParams->GetArg(1));
}


// --------------------------------------------------------------------------------------------------------------------------



static void _LvlRes_export_IResourceList( FILE *hFile, const ICryPak::ERecordFileOpenList eList )
{
	IResourceList *pResList = gEnv->pCryPak->GetRecorderdResourceList(eList);

	for (const char *filename = pResList->GetFirst(); filename; filename = pResList->GetNext())
	{
		enum {nMaxPath = 0x800};
		char szAbsPathBuf[nMaxPath];

		const char *szAbsPath = gEnv->pCryPak->AdjustFileName(filename,szAbsPathBuf,0);

		gEnv->pCryPak->FPrintf(hFile,"%s\n",szAbsPath);
	}
}

void LvlRes_export( IConsoleCmdArgs *pParams )
{
	// * this assumes the level was already loaded in the editor (resources have been recorded)
	// * it could be easily changed to run the launcher, start recording, load a level and quit (useful to autoexport many levels)

	const char *szLevelName = gEnv->pGame->GetIGameFramework()->GetLevelName();
	char szAbsLevelPathBuf[512];
	const char *szAbsLevelPath = gEnv->pGame->GetIGameFramework()->GetAbsLevelPath(szAbsLevelPathBuf, sizeof(szAbsLevelPathBuf));

	if(!szAbsLevelPath || !szLevelName)
	{
		gEnv->pLog->LogError("Error: LvlRes_export no level loaded?");
		return;
	}

	string sPureLevelName = PathUtil::GetFile(szLevelName);		// level name without path

	// record all assets that might be loaded after level loading
	if(gEnv->pGame)
	if(gEnv->pGame->GetIGameFramework())
		gEnv->pGame->GetIGameFramework()->PrefetchLevelAssets(true);

	enum {nMaxPath = 0x800};
	char szAbsPathBuf[nMaxPath];

	sprintf(szAbsPathBuf,"%s/%s%s",szAbsLevelPath,sPureLevelName.c_str(),g_szLvlResExt);

	// Write resource list to file.
	FILE *hFile = gEnv->pCryPak->FOpen(szAbsPathBuf,"wt");

	if(!hFile)
	{
		gEnv->pLog->LogError("Error: LvlRes_export file open failed");
		return;
	}

	gEnv->pCryPak->FPrintf(hFile,"; this file can be safely deleted - it's only purpose is to produce a striped build (without unused assets)\n\n");

	char rootpath[_MAX_PATH];
	CryGetCurrentDirectory(sizeof(rootpath),rootpath);

	gEnv->pCryPak->FPrintf(hFile,"; EngineStartup\n");
	_LvlRes_export_IResourceList(hFile,ICryPak::RFOM_EngineStartup);
	gEnv->pCryPak->FPrintf(hFile,"; Level '%s'\n",szAbsLevelPath);
	_LvlRes_export_IResourceList(hFile,ICryPak::RFOM_Level);

	gEnv->pCryPak->FClose(hFile);
}


// create all directories needed to represent the path, \ and / are handled
// currently no error checking
// (should be moved to PathUtil)
// Arguments:
//   szPath - e.g. "c:\temp/foldername1/foldername2"
static void CreateDirectoryPath( const char *szPath )
{
	const char *p = szPath;

	string sFolder;

	for(;;)
	{
		if(*p=='/' || *p=='\\' || *p==0)
		{
			CryCreateDirectory(sFolder.c_str(),0);

			if(*p==0)
				break;

			sFolder+='\\';++p;
		}
		else
			sFolder += *p++;
	}
}

static string ConcatPath( const char *szPart1, const char *szPart2 )
{
	if(szPart1[0]==0)
		return szPart2;

	string ret;

	ret.reserve(strlen(szPart1)+1+strlen(szPart2));

	ret=szPart1;
	ret+="/";
	ret+=szPart2;

	return ret;
}



class CLvlRes_base
{
public:

	// destructor
	virtual ~CLvlRes_base()
	{
	}

	void RegisterAllLevelPaks( const string &sPath )
	{
		_finddata_t fd;

		string sPathPattern = ConcatPath(sPath,"*.*");

		intptr_t handle = gEnv->pCryPak->FindFirst(sPathPattern.c_str(), &fd );

		if(handle<0)
		{
			gEnv->pLog->LogError("ERROR: CLvlRes_base failed '%s'",sPathPattern.c_str());
			return;
		}

		do 
		{
			if(fd.attrib&_A_SUBDIR)
			{
				if(strcmp(fd.name,".")!=0 && strcmp(fd.name,"..")!=0)
					RegisterAllLevelPaks(ConcatPath(sPath,fd.name));
			}
			else if(HasRightExtension(fd.name))			// open only the level paks if there is a LvlRes.txt, opening all would be too slow 
			{
				OnPakEntry(sPath,fd.name);
			}

		} while (gEnv->pCryPak->FindNext( handle, &fd ) >= 0);

		gEnv->pCryPak->FindClose(handle);
	}



	void Process( const string &sPath )
	{
		_finddata_t fd;

		string sPathPattern = ConcatPath(sPath,"*.*");

		intptr_t handle = gEnv->pCryPak->FindFirst(sPathPattern.c_str(), &fd );

		if(handle<0)
		{
			gEnv->pLog->LogError("ERROR: LvlRes_finalstep failed '%s'",sPathPattern.c_str());
			return;
		}

		do 
		{
			if(fd.attrib&_A_SUBDIR)
			{
				if(strcmp(fd.name,".")!=0 && strcmp(fd.name,"..")!=0)
					Process(ConcatPath(sPath,fd.name));
			}
			else if(HasRightExtension(fd.name))
			{
				string sFilePath = ConcatPath(sPath,fd.name);

				gEnv->pLog->Log("CLvlRes_base processing '%s' ...",sFilePath.c_str());

				FILE *hFile = gEnv->pCryPak->FOpen(sFilePath.c_str(),"rb");

				if(hFile)
				{
					std::vector<char> vBuffer;		

					size_t len = gEnv->pCryPak->FGetSize(hFile);
					vBuffer.resize(len+1);

					if(len)
					{
						if(gEnv->pCryPak->FReadRaw(&vBuffer[0],len,1,hFile)==1)
						{
							vBuffer[len]=0;															// end terminator

							char *p = &vBuffer[0];

							while(*p)
							{
								while(*p!=0 && *p<=' ')										// jump over whitespace
									++p;

								char *pLineStart = p;

								while(*p!=0 && *p!=10 && *p!=13)					// goto end of line
									++p;

								char *pLineEnd = p;

								while(*p!=0 && (*p==10 || *p==13))				// goto next line with data
									++p;

								if(*pLineStart!=';')											// if it's not a commented line
								{
									*pLineEnd=0;
									OnFileEntry(pLineStart);				// add line
								}
							}
						}
						else gEnv->pLog->LogError("Error: LvlRes_finalstep file open '%s' failed",sFilePath.c_str());
					}

					gEnv->pCryPak->FClose(hFile);
				}
				else gEnv->pLog->LogError("Error: LvlRes_finalstep file open '%s' failed",sFilePath.c_str());
			}
		} while (gEnv->pCryPak->FindNext( handle, &fd ) >= 0);

		gEnv->pCryPak->FindClose(handle);
	}

	bool IsFileKnown( const char *szFilePath )
	{
		string sFilePath = szFilePath;
		
		return m_UniqueFileList.find(sFilePath)!=m_UniqueFileList.end();
	}

protected: // -------------------------------------------------------------------------

	static bool HasRightExtension( const char *szFileName )
	{
		const char *szLvlResExt=szFileName;			

		size_t lenName = strlen(szLvlResExt);
		static size_t lenLvlExt = strlen(g_szLvlResExt);

		if(lenName>=lenLvlExt)
			szLvlResExt+=lenName-lenLvlExt;			// "test_LvlRes.txt" -> "_LvlRes.txt"

		return stricmp(szLvlResExt,g_szLvlResExt)==0;
	}

	// Arguments
	//   sFilePath - e.g. "game/object/vehices/car01.dds"
	void OnFileEntry( const char *szFilePath )
	{
		string sFilePath = szFilePath;
		if(m_UniqueFileList.find(sFilePath)==m_UniqueFileList.end())		// to to file processing only once per file
		{
			m_UniqueFileList.insert(sFilePath);

			ProcessFile(sFilePath);

			gEnv->pLog->UpdateLoadingScreen(0);
		}
	}

	virtual void ProcessFile( const string &sFilePath )=0;

	virtual void OnPakEntry( const string &sPath, const char *szPak ) {}

// -----------------------------------------------------------------

	std::set<string>				m_UniqueFileList;				// to removed duplicate files
};





class CLvlRes_finalstep :public CLvlRes_base
{
public:

	// constructor
	CLvlRes_finalstep( const char *szPath ) :m_sPath(szPath)
	{
		assert(szPath);
	}

	// destructor
	virtual ~CLvlRes_finalstep()
	{
		// free registered paks
		std::set<string>::iterator it, end=m_RegisteredPakFiles.end();

		for(it=m_RegisteredPakFiles.begin();it!=end;++it)
		{
			string sName = *it;

			gEnv->pCryPak->ClosePack(sName.c_str());
		}
	}

	// register a pak file so all files within do not become file entries but the pak file becomes
	void RegisterPak( const string &sPath, const char *szFile )
	{
		string sPak = ConcatPath(sPath,szFile);

		gEnv->pCryPak->ClosePack(sPak.c_str());			// so we don't get error for paks that were already opened

		if(!gEnv->pCryPak->OpenPack(sPak.c_str()))
		{
			CryLog("RegisterPak '%s' failed - file not present?", sPak.c_str());
			return;
		}

		enum {nMaxPath = 0x800};
		char szAbsPathBuf[nMaxPath];

		const char *szAbsPath = gEnv->pCryPak->AdjustFileName(sPak,szAbsPathBuf,0);

//		string sAbsPath = PathUtil::RemoveSlash(PathUtil::GetPath(szAbsPath));

		// debug
		CryLog("RegisterPak '%s'", szAbsPath);

		m_RegisteredPakFiles.insert(string(szAbsPath));

		OnFileEntry(sPak);		// include pak as file entry
	}

	// finds a specific file
	static bool FindFile( const char *szFilePath, const char *szFile, _finddata_t &fd )
	{
		intptr_t handle = gEnv->pCryPak->FindFirst(szFilePath, &fd );

		if(handle<0)
			return false;

		do
		{
			if(stricmp(fd.name,szFile)==0)
			{
				gEnv->pCryPak->FindClose(handle);
				return true;
			}
		} while(gEnv->pCryPak->FindNext(handle,&fd));

		gEnv->pCryPak->FindClose(handle);
		return false;
	}

	// slow but safe (to correct path and file name upper/lower case to the existing files)
	// some code might rely on the case (e.g. CVarGroup creation) so it's better to correct the case
	static void CorrectCaseInPlace( char *szFilePath )
	{
		// required for FindFirst, TODO: investigate as this seems wrong behavior
		{
			// jump over "Game"
			while(*szFilePath!='/' && *szFilePath!='\\' && *szFilePath!=0)
				++szFilePath;
			// jump over "/"
			if(*szFilePath!=0)
				++szFilePath;
		}

		char *szFile=szFilePath, *p=szFilePath;

		for(;;)
		{
			if(*p=='/' || *p=='\\' || *p==0)
			{
				char cOldChar = *p;		*p=0;			// create zero termination
				_finddata_t fd;

				bool bOk = FindFile(szFilePath,szFile,fd);

				if(bOk)
					assert(strlen(szFile)==strlen(fd.name));
				
				*p = cOldChar;									// get back the old separator

				if(!bOk)
					return;

				
				memcpy((void *)szFile,fd.name,strlen(fd.name));		// set

				if(*p==0)
					break;

				++p;szFile=p;
			}
			else ++p;
		}
	}

	virtual void ProcessFile( const string &_sFilePath )
	{
		string sFilePath=_sFilePath;

		CorrectCaseInPlace((char *)&sFilePath[0]);

		gEnv->pLog->LogWithType(ILog::eAlways,"LvlRes: %s",sFilePath.c_str());

		CCryFile file;
		std::vector<char> data;		

		if(!file.Open( sFilePath.c_str(),"rb" ))
		{
			OutputDebugString(">>>>> failed to open '");
			OutputDebugString(sFilePath.c_str());
			OutputDebugString("'\n");
			//			gEnv->pLog->LogError("ERROR: failed to open '%s'",sFilePath.c_str());			// pak not opened ?
			//			assert(0);
			return;
		}

		if(IsInRegisteredPak(file.GetHandle()))
			return;					// then don't process as we include the pak

		// Save this file in target folder.
		string trgFilename = PathUtil::Make( m_sPath,sFilePath );
		int fsize = file.GetLength();

		size_t len = file.GetLength();

		if (fsize > (int)data.size())
			data.resize(fsize + 16);

		// Read data.
		file.ReadRaw( &data[0],fsize );

		// Save this data to target file.
		string trgFileDir = PathUtil::ToDosPath(PathUtil::RemoveSlash(PathUtil::GetPath(trgFilename)));	

		CreateDirectoryPath( trgFileDir );		// ensure path exists

		// Create target file
		FILE *trgFile = fopen( trgFilename,"wb" );

		if (trgFile)
		{
			fwrite(&data[0],fsize,1,trgFile);
			fclose(trgFile);
		}
		else
		{
			gEnv->pLog->LogError("ERROR: failed to write '%s' (write protected/disk full/rights)",trgFilename.c_str());
			assert(0);
		}
	}

	bool IsInRegisteredPak( FILE *hFile )
	{
		const char *szPak = gEnv->pCryPak->GetFileArchivePath(hFile);

		if(!szPak)
			return false;			// outside pak

		bool bInsideRegisteredPak = m_RegisteredPakFiles.find(szPak)!=m_RegisteredPakFiles.end();

		return bInsideRegisteredPak;
	}

	virtual void OnPakEntry( const string &sPath, const char *szPak )
	{
		RegisterPak(sPath,"level.pak");
		RegisterPak(sPath,"levellm.pak");
	}

	// -------------------------------------------------------------------------------

	string									m_sPath;								// directory path to store the assets e.g. "c:\temp\Out"
	std::set<string>				m_RegisteredPakFiles;		// abs path to pak files we registered e.g. "c:\MasterCD\game\GameData.pak", to avoid processing files inside these pak files - the ones we anyway want to include
};


class CLvlRes_findunused :public CLvlRes_base
{
public:

	virtual void ProcessFile( const string &sFilePath )
	{
	}
};






static void LvlRes_finalstep( IConsoleCmdArgs *pParams )
{
	assert(pParams);

	uint32 dwCnt = pParams->GetArgCount();

	if(dwCnt!=2)
	{
		gEnv->pLog->LogWithType(ILog::eError,"ERROR: sys_LvlRes_finalstep requires destination path as parameter");
		return;
	}

	const char *szPath = pParams->GetArg(1);			assert(szPath);

	gEnv->pLog->LogWithType(ILog::eInputResponse,"sys_LvlRes_finalstep %s ...",szPath);

	// open console
	gEnv->pConsole->ShowConsole(true);

	CLvlRes_finalstep sink(szPath);

	sink.RegisterPak(PathUtil::GetGameFolder(), "GameData.pak");
	sink.RegisterPak(PathUtil::GetGameFolder(), "Shaders.pak");

	sink.RegisterAllLevelPaks(PathUtil::GetGameFolder() + "/levels");
	sink.Process(PathUtil::GetGameFolder() + "/levels");
}




static void _LvlRes_findunused_recursive( CLvlRes_findunused &sink, const string &sPath,
	uint32 &dwUnused, uint32 &dwAll )
{
	_finddata_t fd;

	string sPathPattern = ConcatPath(sPath,"*.*");

	// ignore some directories
	if(stricmp(sPath.c_str(),"Shaders")==0
	|| stricmp(sPath.c_str(),"ScreenShots")==0
	|| stricmp(sPath.c_str(),"Scripts")==0
	|| stricmp(sPath.c_str(),"Config")==0
	|| stricmp(sPath.c_str(),"LowSpec")==0)
		return;

//	gEnv->pLog->Log("_LvlRes_findunused_recursive '%s'",sPath.c_str());

	intptr_t handle = gEnv->pCryPak->FindFirst(sPathPattern.c_str(), &fd );

	if(handle<0)
	{
		gEnv->pLog->LogError("ERROR: _LvlRes_findunused_recursive failed '%s'",sPathPattern.c_str());
		return;
	}

	do 
	{
		if(fd.attrib&_A_SUBDIR)
		{
			if(strcmp(fd.name,".")!=0 && strcmp(fd.name,"..")!=0)
				_LvlRes_findunused_recursive(sink,ConcatPath(sPath,fd.name),dwUnused,dwAll);
		}
		else
		{
			/*
			// ignore some extensions
			if(stricmp(PathUtil::GetExt(fd.name),"cry")==0)
				continue;

			// ignore some files
			if(stricmp(fd.name,"TerrainTexture.pak")==0)
				continue;
			*/

			string sFilePath = CryStringUtils::toLower(ConcatPath(sPath,fd.name));
			enum {nMaxPath = 0x800};
			char szAbsPathBuf[nMaxPath];

			gEnv->pCryPak->AdjustFileName(sFilePath.c_str(),szAbsPathBuf,0);

			if(!sink.IsFileKnown(szAbsPathBuf))
			{
				gEnv->pLog->LogWithType(IMiniLog::eAlways,"%d, %s",(uint32)fd.size,szAbsPathBuf);
				++dwUnused;
			}
			++dwAll;
		}
	} while (gEnv->pCryPak->FindNext( handle, &fd ) >= 0);

	gEnv->pCryPak->FindClose(handle);
}


static void LvlRes_findunused( IConsoleCmdArgs *pParams )
{
	assert(pParams);

	gEnv->pLog->LogWithType(ILog::eInputResponse,"sys_LvlRes_findunused ...");

	// open console
	gEnv->pConsole->ShowConsole(true);

	CLvlRes_findunused sink;

	sink.RegisterAllLevelPaks(PathUtil::GetGameFolder() + "/levels");
	sink.Process(PathUtil::GetGameFolder() + "/levels");

	gEnv->pLog->LogWithType(ILog::eInputResponse," ");
	gEnv->pLog->LogWithType(ILog::eInputResponse,"Assets not used by the existing LvlRes data:");
	gEnv->pLog->LogWithType(ILog::eInputResponse," ");
	
	char rootpath[_MAX_PATH];
	CryGetCurrentDirectory(sizeof(rootpath),rootpath);

	gEnv->pLog->LogWithType(ILog::eInputResponse,"Folder: %s",rootpath);

	uint32 dwUnused=0, dwAll=0;

	string unused;
	_LvlRes_findunused_recursive(sink,unused,dwUnused,dwAll);

	gEnv->pLog->LogWithType(ILog::eInputResponse," ");
	gEnv->pLog->LogWithType(ILog::eInputResponse,"Unused assets: %d/%d",dwUnused,dwAll);
	gEnv->pLog->LogWithType(ILog::eInputResponse," ");
}

void CryResetStats(void);

static void DumpAllocs( IConsoleCmdArgs *pParams )
{
	CryGetIMemReplay()->DumpStats();
}

static void ReplayDumpSymbols(IConsoleCmdArgs* pParams)
{
	CryGetIMemReplay()->ReplayDumpSymbols();
}

static void ReplayStop(IConsoleCmdArgs* pParams)
{
	CryGetIMemReplay()->ReplayStop();
}

static void ReplayPause(IConsoleCmdArgs* pParams)
{
	CryGetIMemReplay()->ReplayStart(true);
}

static void ReplayResume(IConsoleCmdArgs* pParams)
{
	CryGetIMemReplay()->ReplayStart(false);
}

static void ResetAllocs( IConsoleCmdArgs *pParams )
{
	CryResetStats();
}

static void AddReplayLabel( IConsoleCmdArgs* pParams )
{
	if (pParams->GetArgCount() < 2)
		CryLog("Not enough arguments");
	else
		CryGetIMemReplay()->MemStatAddLabel(pParams->GetArg(1));
}

static void ReplayInfo(IConsoleCmdArgs* pParams)
{
	CryReplayInfo info;
	CryGetIMemReplay()->ReplayGetInfo(info);

	CryLog("Uncompressed length: %llu", info.uncompressedLength);
	CryLog("Tracking overhead: %u", info.trackingSize);
	CryLog("Output filename: %s", info.filename);
}

// --------------------------------------------------------------------------------------------------------------------------


static void ScreenshotCmd( IConsoleCmdArgs *pParams )
{
	assert(pParams);

	uint32 dwCnt = pParams->GetArgCount();

	if(dwCnt<=1)
	{
		if(!gEnv->pSystem->IsEditorMode())
		{
			// open console one line only
			
			//line should lie within title safe area, so calculate overscan border
			Vec2 overscanBorders = *(Vec2*)gEnv->pRenderer->EF_Query(EFQ_OverscanBorders);
			float yDelta = /*((float)gEnv->pRenderer->GetHeight())*/600.0f * overscanBorders.y;

			//set console height depending on top/bottom overscan border
			gEnv->pConsole->ShowConsole(true,(int)(16 + yDelta));
			gEnv->pConsole->SetInputLine("Screenshot ");
		}
		else
		{
			gEnv->pLog->LogWithType(ILog::eInputResponse,"Screenshot <annotation> missing - no screenshot was done");
		}
	}
	else
	{
		static int iScreenshotNumber=-1;

		const char *szPrefix="Screenshot";
		uint32 dwPrefixSize=strlen(szPrefix);

		char path[ICryPak::g_nMaxPath];
		path[sizeof(path) - 1] = 0;
		gEnv->pCryPak->AdjustFileName("%USER%/ScreenShots", path, ICryPak::FLAGS_PATH_REAL|ICryPak::FLAGS_FOR_WRITING);

		if(iScreenshotNumber==-1)		// first time - find max number to start
		{
			ICryPak *pCryPak = gEnv->pCryPak;
			_finddata_t fd;

			intptr_t handle = pCryPak->FindFirst((string(path)+"/*.*").c_str(),&fd );		// mastercd folder
			if (handle != -1)
			{
				int res = 0;
				do
				{
					int iCurScreenshotNumber;

					if(strnicmp(fd.name,szPrefix,dwPrefixSize)==0)
					{
						int iCnt = sscanf(fd.name+10,"%d",&iCurScreenshotNumber);

						if(iCnt)
							iScreenshotNumber=max(iCurScreenshotNumber,iScreenshotNumber);
					}

					res = pCryPak->FindNext( handle,&fd );
				} while (res >= 0);
				pCryPak->FindClose(handle);
			}
		}

		++iScreenshotNumber;

		char szNumber[16];
		sprintf(szNumber,"%.4d ",iScreenshotNumber);

		string sScreenshotName = string(szPrefix)+szNumber;

		for(uint32 dwI=1;dwI<dwCnt;++dwI)
		{
			if(dwI>1)
				sScreenshotName += "_";

			sScreenshotName += pParams->GetArg(dwI);
		}

		sScreenshotName.replace("\\","_");
		sScreenshotName.replace("/","_");
		sScreenshotName.replace(":","_");
		sScreenshotName.replace(".","_");

		gEnv->pConsole->ShowConsole(false);

		CSystem *pCSystem = (CSystem *)(gEnv->pSystem);
		pCSystem->GetDelayedScreeenshot() = string(path)+"/"+sScreenshotName;// to delay a screenshot call for a frame
	}
}




//////////////////////////////////////////////////////////////////////////
bool CSystem::IsEditorMode()
{
	if (!gEnv->pGame || !gEnv->pGame->GetIGameFramework())
		return IsEditor();
	else
		return IsEditor() && gEnv->pGame->GetIGameFramework()->IsEditing();
}




static void SysRestoreSpecCmd( IConsoleCmdArgs *pParams )
{
	assert(pParams);

	if(pParams->GetArgCount()==2)
	{
		const char *szArg = pParams->GetArg(1);

		ICVar *pCVar = gEnv->pConsole->GetCVar("sys_spec_Full");			

		if(!pCVar)
		{
			gEnv->pLog->LogWithType(ILog::eInputResponse,"sys_RestoreSpec: no action");		// e.g. running Editor in shder compile mode 
			return;
		}

		ICVar::EConsoleLogMode mode=ICVar::eCLM_Off;

		if(stricmp(szArg,"test")==0)
			mode= ICVar::eCLM_ConsoleAndFile;
		 else if(stricmp(szArg,"test*")==0)
			mode= ICVar::eCLM_FileOnly;
		 else if(stricmp(szArg,"info")==0)
			mode= ICVar::eCLM_FullInfo;

		if(mode!=ICVar::eCLM_Off)
		{
			bool bFileOrConsole = (mode==ICVar::eCLM_FileOnly || mode==ICVar::eCLM_FullInfo);

			if(bFileOrConsole)
				gEnv->pLog->LogToFile(" ");
			 else
				CryLog(" ");

			int iSysSpec = pCVar->GetRealIVal();

			if(iSysSpec==-1)
			{
				iSysSpec = ((CSystem*)gEnv->pSystem)->GetMaxConfigSpec();

				if(bFileOrConsole)
					gEnv->pLog->LogToFile("   sys_spec = Custom (assuming %d)",iSysSpec);
				 else
					gEnv->pLog->LogWithType(ILog::eInputResponse,"   $3sys_spec = $6Custom (assuming %d)",iSysSpec);
			}
			else
			{
				if(bFileOrConsole)
					gEnv->pLog->LogToFile("   sys_spec = %d",iSysSpec);
				 else
					gEnv->pLog->LogWithType(ILog::eInputResponse,"   $3sys_spec = $6%d",iSysSpec);
			}

			pCVar->DebugLog(iSysSpec,mode);

			if(bFileOrConsole)
				gEnv->pLog->LogToFile(" ");
			 else
				gEnv->pLog->LogWithType(ILog::eInputResponse," ");

			return;
		}
		else if(strcmp(szArg,"apply")==0)
		{
			const char *szPrefix = "sys_spec_";

			std::vector<const char*> cmds;

			cmds.resize(gEnv->pConsole->GetSortedVars(0,0,szPrefix));
			gEnv->pConsole->GetSortedVars(&cmds[0],cmds.size(),szPrefix);

			gEnv->pLog->LogWithType(IMiniLog::eInputResponse," ");

			std::vector<const char*>::const_iterator it, end=cmds.end();

			for(it=cmds.begin();it!=end;++it)
			{
				const char *szName = *it;

				if(stricmp(szName,"sys_spec_Full")==0)
					continue;

				pCVar = gEnv->pConsole->GetCVar(szName);			assert(pCVar);

				if(!pCVar)
					continue;

				bool bNeeded = pCVar->GetIVal()!=pCVar->GetRealIVal();

				gEnv->pLog->LogWithType(IMiniLog::eInputResponse," $3%s = $6%d ... %s",
					szName,pCVar->GetIVal(),
					bNeeded?"$4restored":"valid");

				if(bNeeded)
					pCVar->Set(pCVar->GetIVal());
			}

			gEnv->pLog->LogWithType(IMiniLog::eInputResponse," ");
			return;
		}
	}

	gEnv->pLog->LogWithType(ILog::eInputResponse,"ERROR: sys_RestoreSpec invalid arguments");
}

void ChangeLogAllocations( ICVar* pVal )
{
	g_iTraceAllocations = pVal->GetIVal();
#ifdef WIN32
	((DebugCallStack*)IDebugCallStack::instance())->SetMemLogFile(g_iTraceAllocations==2, "memallocfile.txt");
#endif
}



void ChangeFileCaching( ICVar* pVal )
{
	gEnv->pCryPak->SetCachingState((ICryPak::eCacheState)pVal->GetIVal());
}

//////////////////////////////////////////////////////////////////////////
void CSystem::CreateSystemVars()
{
	// Register DLL names as cvars before we load them
	// 
	EVarFlags dllFlags = (EVarFlags)0;
	m_sys_dll_ai = REGISTER_STRING("sys_dll_ai", DLL_AI, dllFlags,										"Specifies the DLL to load for the AI system");
	m_sys_dll_game = REGISTER_STRING("sys_dll_game", DLL_GAME, dllFlags,							"Specifies the game DLL to load");
	m_sys_game_folder = REGISTER_STRING("sys_game_folder", "game", 0,    "Specifies the game folder to read all data from");

	m_sys_user_folder = REGISTER_STRING("sys_user_folder", "USER", 0,    "Specifies the user folder where output data is stored");
//Diesel cut
	//m_cvGameName = REGISTER_STRING("cvGameName", "CryENGINE" , VF_DUMPTODISK ,"");
//Diesel new
	m_cvGameName = REGISTER_STRING("cvGameName", "StEngine", VF_DUMPTODISK, "");


	m_pCVarQuit = REGISTER_INT("ExitOnQuit",1,VF_NULL,"");

	m_cvAIUpdate = REGISTER_INT("ai_NoUpdate",0,VF_CHEAT,"Disables AI system update when 1");

	REGISTER_INT("r_AllowLiveMoCap", 0, VF_CHEAT, "Offers the LiveCreate MoCap Editor on Editor Startup when 1");

	m_iTraceAllocations = g_iTraceAllocations;
	ICVar* pCVar_sys_logallactions = REGISTER_CVAR2( "sys_logallocations", &m_iTraceAllocations, m_iTraceAllocations, VF_DUMPTODISK, "Save allocation call stack");
	if (pCVar_sys_logallactions)
		pCVar_sys_logallactions->SetOnChangeCallback(ChangeLogAllocations);

	m_iUseFileCaching = 0;
	ICVar* pCVar_sys_enablefilecaching = REGISTER_CVAR2( "sys_filecaching", &m_iUseFileCaching, m_iUseFileCaching, VF_DUMPTODISK | VF_ALWAYSONCHANGE, "Disable(0) or enable(1) autocaching on xenon");
	if (pCVar_sys_enablefilecaching)
		pCVar_sys_enablefilecaching->SetOnChangeCallback(ChangeFileCaching);

	m_cvMemStats = REGISTER_INT("MemStats", 0, 0,
		"0/x=refresh rate in milliseconds\n"
		"Use 1000 to switch on and 0 to switch off\n"
		"Usage: MemStats [0..]");
	m_cvMemStatsThreshold = REGISTER_INT ("MemStatsThreshold", 32000,VF_NULL,"");
	m_cvMemStatsMaxDepth = REGISTER_INT("MemStatsMaxDepth", 4,VF_NULL,"");

	m_sys_SaveCVars =  REGISTER_INT("sys_SaveCVars", 0, 0,
		"1 to activate saving of console variables, 0 to deactivate\n"
		"The variables are stored in 'system.cfg' on quit, only marked variables are saved (0)\n"
		"Usage: sys_SaveCVars [0/1]\n"
		"Default is 0");
	m_sys_StreamCallbackTimeBudget = REGISTER_INT("sys_StreamCallbackTimeBudget", 20000, 0,
		"Time budget, in microseconds, to be spent every frame in StreamEngine callbacks.\n"
		"Additive with cap: if more time is spent, the next frame gets less budget, and\n"
		"there's never more than this value per frame.");
	
	m_PakVar.nPriority  = 0;
	m_PakVar.nReadSlice = 0;
	m_PakVar.nLogMissingFiles = 0;
	m_cvPakPriority = attachVariable("sys_PakPriority", &m_PakVar.nPriority,"If set to 1, tells CryPak to try to open the file in pak first, then go to file system",VF_READONLY|VF_CHEAT);
	m_cvPakReadSlice = attachVariable("sys_PakReadSlice", &m_PakVar.nReadSlice,"If non-0, means number of kilobytes to use to read files in portions. Should only be used on Win9x kernels");
	m_cvPakLogMissingFiles = attachVariable("sys_PakLogMissingFiles",&m_PakVar.nLogMissingFiles, 
		"If non-0, missing file names go to mastercd/MissingFilesX.log.\n"
		"1) only resulting report\n"
		"2) run-time report is ON, one entry per file\n"
		"3) full run-time report");

	// fix to solve a flaw in the system
	// 0 (default) needed for game, 1 better for having artifact free movie playback (camera pos is valid during entity update)
	m_cvEarlyMovieUpdate = REGISTER_INT("sys_EarlyMovieUpdate",0,0,
		"0 needed for game, 1 better for having artifact free movie playback\n"
		"Usage: sys_EarlyMovieUpdate [0/1]\n"
		"Default is 0");

	m_sysNoUpdate = REGISTER_INT("sys_noupdate",0,VF_CHEAT,
		"Toggles updating of system with sys_script_debugger.\n"
		"Usage: sys_noupdate [0/1]\n"
		"Default is 0 (system updates during debug).");

	m_sysWarnings = REGISTER_INT("sys_warnings",0,0,
		"Toggles printing system warnings.\n"
		"Usage: sys_warnings [0/1]\n"
		"Default is 0 (off).");

	m_svDedicatedMaxRate = REGISTER_FLOAT("sv_DedicatedMaxRate",30.0f,0,
		"Sets the maximum update rate when running as a dedicated server.\n"
		"Usage: sv_DedicatedMaxRate [5..500]\n"
		"Default is 50.");
	REGISTER_FLOAT("sv_DedicatedCPUPercent",0.0f,0,
		"Sets the target CPU usage when running as a dedicated server, or disable this feature if it's zero.\n"
		"Usage: sv_DedicatedCPUPercent [0..100]\n"
		"Default is 0 (disabled).");
	REGISTER_FLOAT("sv_DedicatedCPUVariance",10.0f,0,
		"Sets how much the CPU can vary from sv_DedicateCPU (up or down) without adjusting the framerate.\n"
		"Usage: sv_DedicatedCPUVariance [5..50]\n"
		"Default is 10.");
	m_svAISystem = REGISTER_INT("sv_AISystem", 1, VF_REQUIRE_APP_RESTART, "Load and use the AI system on the server");

	m_cvSSInfo =  REGISTER_INT("sys_SSInfo",0,0,
		"Show SourceSafe information (Name,Comment,Date) for file errors."
		"Usage: sys_SSInfo [0/1]\n"
		"Default is 0 (off)");

	m_cvEntitySuppressionLevel = REGISTER_INT("e_EntitySuppressionLevel",0,0,
		"Defines the level at which entities are spawned.\n"
		"Entities marked with lower level will not be spawned - 0 means no level.\n"
		"Usage: e_EntitySuppressionLevel [0-infinity]\n"
		"Default is 0 (off)");

	m_sys_profile = REGISTER_INT("profile",0,0,"Allows CPU profiling\n"
		"Usage: profile #\n"
		"Where # sets the profiling to:\n"
		"	0: Profiling off\n"
		"	1: Self Time\n"
		"	2: Hierarchical Time\n"
		"	3: Extended Self Time\n"
		"	4: Extended Hierarchical Time\n"
		"	5: Peaks Time\n"
		"	6: Subsystem Info\n"
		"	7: Calls Numbers\n"
		"	8: Standard Deviation\n"
		"	9: Memory Allocation\n"
		"	10: Memory Allocation (Bytes)\n"
		"	11: Stalls\n"
		"	-1: Profiling enabled, but not displayed\n"
		"Default is 0 (off)");


	m_sys_profile_additionalsub = REGISTER_INT("profile_additionalsub", 0, 0, "Enable displaying additional sub-system profiling.\n"
		"Usage: profile_additionalsub #\n"
		"Where where # may be:\n"
		"	0: no additional subsystem information\n"
		"	1: display additional subsystem information\n"
		"Default is 0 (off)");

	m_sys_profile_filter = REGISTER_STRING("profile_filter","",0,
		"Profiles a specified subsystem.\n"
		"Usage: profile_filter subsystem\n"
		"Where 'subsystem' may be:\n"
		"Renderer\n"
		"3DEngine\n"
		"Animation\n"
		"AI\n"
		"Entity\n"
		"Physics\n"
		"Sound\n"
		"System\n"
		"Game\n"
		"Editor\n"
		"Script\n"
		"Network");
	m_sys_profile_filter_thread = REGISTER_STRING("profile_filter_thread","",0,
		"Profiles a specified thread only.\n"
		"Usage: profile_filter threadName\n"
		"Where 'threadName' may be:\n"
		"Main\n"
		"Renderer\n"
		"Streaming\n"
		"etc...");
	m_sys_profile_graph = REGISTER_INT("profile_graph",0,0,
		"Enable drawing of profiling graph.");
	m_sys_profile_graphScale = REGISTER_FLOAT("profile_graphScale",100.0f,0,
		"Sets the scale of profiling histograms.\n"
		"Usage: profileGraphScale 100");
	m_sys_profile_pagefaultsgraph = REGISTER_INT("profile_pagefaults",0,0,
		"Enable drawing of page faults graph.");
	m_sys_profile_allThreads = REGISTER_INT("profile_allthreads",0,0,
		"Enables profiling of non-main threads.\n" );
	m_sys_profile_network = REGISTER_INT("profile_network",0,0,
		"Enables network profiling" );
	m_sys_profile_peak = REGISTER_FLOAT("profile_peak",10.0f,0,
		"Profiler Peaks Tolerance in Milliseconds" );
	m_sys_profile_peak_time = REGISTER_FLOAT("profile_peak_display", 8.0f, 0, 
		"hot to cold time for peak display");
	m_sys_profile_memory = REGISTER_INT("MemInfo",0,0,"Display memory information by modules\n1=on, 0=off" );

	m_sys_profile_sampler = REGISTER_FLOAT("profile_sampler", 0, 0, 
		"Set to 1 to start sampling profiling");
	m_sys_profile_sampler_max_samples = REGISTER_FLOAT("profile_sampler_max_samples", 2000, 0, 
		"Number of samples to collect for sampling profiler");
	m_sys_mtrace = REGISTER_INT("enable_memory_trace",1,0, 
    "enable tracing of memory operations via a mtrace server: 1 to enable, 0 to disable");
	m_sys_mtrace_print = REGISTER_INT("print_memory_trace",1,0, 
    "dump memory traces to log: 1 to enable, 0 to disable");
	m_sys_spu_enable = REGISTER_INT("spu_enable",1,0,	
		"Enable SPU Jobs: 1 to activate, 0 to deactivate");
	m_sys_spu_profile = REGISTER_INT("spu_profile_mode",0,0,	
		"SPU Profile Mode: 1 to display functions, 0 to display jobs");
	m_sys_spu_max = REGISTER_INT("spu_max",5,0,	
		"Specifies how many raw SPUs are to be used: 2..5");
	m_sys_spu_flipmode = REGISTER_INT("spu_flipmode",0,0,	
		"Specifies how the flip is done on SPU, 0 inlined and 1 via interrupt");
	m_sys_spu_debug	= REGISTER_STRING("spu_debug","",0,
		"Enables debugging of a SPU Job.\n"
		"Usage: spu_debug name\n"
		"Where 'name' refers to the exact name of the SPU job, automatically deactivated afterwards");
	m_sys_spu_filter	= REGISTER_STRING("spu_filter","",0,
		"Filters a SPU Job.\n"
		"Usage: spu_filter name\n"
		"Where 'name' refers to the exact name of the SPU job, 0 disables it");
	m_sys_spu_dump_stats = REGISTER_INT("spu_dump_stats",0,0,	
		"Set to 1 to dump SPU job stats for current frame.\n"
		"Gets automatically deactivated after current frame has finished.");
	m_sys_spu_streaming = REGISTER_INT("spu_streaming",1,0,	
		"Use SPU for stream decompression: 1 to activate, 0 to deactivate");
	m_sys_spec=REGISTER_INT("sys_spec",CONFIG_CUSTOM,VF_ALWAYSONCHANGE,		// starts with CONFIG_CUSTOM so callback is called when setting initial value
		"Tells the system cfg spec. (0=custom, 1=low, 2=med, 3=high, 4=very high, 5=XBox360, 6=Playstation3)");
	m_sys_spec->SetOnChangeCallback(OnSysSpecChange );

	m_sys_SimulateTask = REGISTER_INT("sys_SimulateTask", 0, 0, 
		"Simulate a task in System:Update which takes X ms");

	m_sys_firstlaunch = REGISTER_INT( "sys_firstlaunch", 0, 0,
		"Indicates that the game was run for the first time." );

	m_sys_main_CPU = REGISTER_INT("m_sys_main_CPU", 0, 0, 
		"Specifies the physical CPU index main will run on");

	m_sys_streaming_CPU = REGISTER_INT("m_sys_streaming_CPU", 1, 0, 
		"Specifies the physical CPU index streaming will run on");


	m_sys_streamingpool_CPU[0] = REGISTER_INT("m_sys_streamingpool0_CPU", 5, 0, 
		"Specifies the physical CPU index streaming pool 0 will run on");

	m_sys_streamingpool_CPU[1] = REGISTER_INT("m_sys_streamingpool1_CPU", 3, 0, 
		"Specifies the physical CPU index streaming pool 1 will run on");

	m_sys_streamingpool_CPU[2] = REGISTER_INT("m_sys_streamingpool2_CPU", 2, 0, 
		"Specifies the physical CPU index streaming pool 2 will run on");

	m_sys_streamingpool_CPU[3] = REGISTER_INT("m_sys_streamingpool3_CPU", 4, 0, 
		"Specifies the physical CPU index streaming pool 3 will run on");

	m_sys_streamingpool_CPU[4] = REGISTER_INT("m_sys_streamingpool4_CPU", 1, 0, 
		"Specifies the physical CPU index streaming pool 4 will run on");

	m_sys_streamingpool_CPU[5] = REGISTER_INT("m_sys_streamingpool5_CPU", 0, 0, 
		"Specifies the physical CPU index streaming pool 5 will run on");

	m_sys_TaskThread_CPU[0] = REGISTER_INT("m_sys_TaskThread0_CPU", 3, 0, 
		"Specifies the physical CPU index taskthread0 will run on");

	m_sys_TaskThread_CPU[1] = REGISTER_INT("m_sys_TaskThread1_CPU", 5, 0, 
		"Specifies the physical CPU index taskthread1 will run on");

	m_sys_TaskThread_CPU[2] = REGISTER_INT("m_sys_TaskThread1_CPU", 4, 0, 
		"Specifies the physical CPU index taskthread2 will run on");

	m_sys_TaskThread_CPU[3] = REGISTER_INT("m_sys_TaskThread1_CPU", 3, 0, 
		"Specifies the physical CPU index taskthread3 will run on");

	m_sys_TaskThread_CPU[4] = REGISTER_INT("m_sys_TaskThread1_CPU", 2, 0, 
		"Specifies the physical CPU index taskthread4 will run on");

	m_sys_TaskThread_CPU[5] = REGISTER_INT("m_sys_TaskThread1_CPU", 1, 0, 
		"Specifies the physical CPU index taskthread5 will run on");

	//if physics thread is excluded all locks inside are mapped to NO_LOCK
	//var must be not visible to accidentally get enabled
#ifdef EXCLUDE_PHYSICS_THREAD
	m_sys_physics_CPU = REGISTER_INT("sys_physics_CPU_disabled", 0, 0, 
    "Specifies the physical CPU index physics will run on");
#else
	m_sys_physics_CPU = REGISTER_INT("sys_physics_CPU", 1, 0, 
    "Specifies the physical CPU index physics will run on");
#endif
	m_sys_min_step = REGISTER_FLOAT("sys_min_step", 0.01f, 0, 
		"Specifies the minimum physics step in a separate thread");
	m_sys_max_step = REGISTER_FLOAT("sys_max_step", 0.05f, 0, 
		"Specifies the maximum physics step in a separate thread");

	m_sys_enable_budgetmonitoring = REGISTER_INT( "sys_enable_budgetmonitoring", 0, 0,
		"Enables budget monitoring. Use #System.SetBudget( sysMemLimitInMB, videoMemLimitInMB,\n"
		"frameTimeLimitInMS, soundChannelsPlaying ) or sys_budget_sysmem, sys_budget_videomem\n"
		"or sys_budget_fps to set budget limits.");

	// used in define MEMORY_DEBUG_POINT()
	m_sys_memory_debug=REGISTER_INT("sys_memory_debug",0,VF_CHEAT,
		"Enables to activate low memory situation is specific places in the code (argument defines which place), 0=off");

	REGISTER_CVAR2( "sys_vtune",&g_cvars.sys_vtune,0,VF_NULL,"" );
	REGISTER_CVAR2( "sys_streaming_sleep",&g_cvars.sys_streaming_sleep,0,VF_NULL,"" );
	
#ifdef WIN32
	REGISTER_CVAR2( "sys_float_exceptions",&g_cvars.sys_float_exceptions,1,0,"Use or not use floating point exceptions." );
#else // Float exceptions by default disabled for console builds.
	REGISTER_CVAR2( "sys_float_exceptions",&g_cvars.sys_float_exceptions,0,0,"Use or not use floating point exceptions." );
#endif
	
	REGISTER_CVAR2( "sys_update_profile_time", &g_cvars.sys_update_profile_time, 1.0f,0,"Time to keep updates timings history for.");
	REGISTER_CVAR2( "sys_no_crash_dialog",&g_cvars.sys_no_crash_dialog,m_bNoCrashDialog,VF_NULL,"" );
	REGISTER_CVAR2( "sys_WER",&g_cvars.sys_WER,0,0,"Enables Windows Error Reporting" );
	REGISTER_CVAR2( "sys_dump_aux_threads",&g_cvars.sys_dump_aux_threads,1,VF_NULL,"Dumps callstacks of other threads in case of a crash" );
	REGISTER_CVAR2( "sys_keyboard_break",&g_cvars.sys_keyboard_break,0,VF_NULL,"Enables keyboard break handler" );

	REGISTER_CVAR2( "sys_limit_phys_thread_count",&g_cvars.sys_limit_phys_thread_count,1,VF_NULL,"Limits p_num_threads to physical CPU count - 1" );

	m_sys_preload = REGISTER_INT("sys_preload",0,0,"Preload Game Resources" );
	m_sys_crashtest = REGISTER_INT("sys_crashtest",0,VF_NULL,"" );
	m_sys_writePixLog= REGISTER_INT("sys_writepixlog",0,VF_NULL,"" );

	REGISTER_INT( "capture_frames", 0, 0, "Enables capturing of frames. 0=off, 1=on" );
	REGISTER_STRING( "capture_folder", "CaptureOutput", 0, "Specifies sub folder to write captured frames." );
	REGISTER_STRING( "capture_file_format", "jpg", 0, "Specifies file format of captured files (jpg, bmp, tga, hdr)." );
	REGISTER_INT( "capture_frame_once", 0, 0, "Makes capture single frame only" );
	REGISTER_INT( "capture_use_scale", 0, 0, "Specifies that the image scale cvar should be used" );
	REGISTER_FLOAT( "capture_image_scale", 0.25f, 0, "Image size. [0-1] = scale value. >1 = actual pixels for image width" );
	REGISTER_STRING( "capture_file_name", "", 0, "If set, specifies the path and name to use for the captured frame" );

	m_gpu_particle_physics = REGISTER_INT("gpu_particle_physics", 0, VF_REQUIRE_APP_RESTART, "Enable GPU physics if available (0=off / 1=enabled).");
	assert(m_gpu_particle_physics);

	REGISTER_COMMAND("LoadConfig",&LoadConfigurationCmd,0,
		"Load .cfg file from disk (from the {Game}/Config directory)\n"
		"e.g. LoadConfig lowspec.cfg\n"
		"Usage: LoadConfig <filename>");

	REGISTER_CVAR(sys_ProfileLevelLoading, 0, VF_CHEAT,
		"Output level loading stats into log\n"
		"0 = Off\n"
		"1 = Output basic info about loading time per function\n"
		"2 = Output full statistics including loading time and memory allocations with call stack info");

	{
		ICVar *pSys_ProfileLevelLoadingDump = REGISTER_CVAR(sys_ProfileLevelLoadingDump, 0, VF_CHEAT,	"Output level loading dump stats into log\n");
		assert(pSys_ProfileLevelLoadingDump);
		pSys_ProfileLevelLoadingDump->SetOnChangeCallback(OnLevelLoadingDump);
	}


	assert(m_env.pConsole);
	m_env.pConsole->CreateKeyBind("alt f12", "Screenshot");

	// screenshot functionality in system as console 	
	REGISTER_COMMAND("Screenshot",&ScreenshotCmd,VF_BLOCKFRAME,
		"Create a screenshot with annotation\n"
		"e.g. Screenshot beach scene with shark\n"
		"Usage: Screenshot <annotation text>");

	REGISTER_COMMAND("sys_LvlRes_finalstep",&LvlRes_finalstep,0,"to combine all recorded level resources and create final stripped build (pass directory name as parameter)");
	REGISTER_COMMAND("sys_LvlRes_findunused",&LvlRes_findunused,0,"find unused level resources");
/*
	// experimental feature? - needs to be created very early
	m_sys_filecache = REGISTER_INT("sys_FileCache",0,0,
		"To speed up loading from non HD media\n"
		"0=off / 1=enabled");
*/
	REGISTER_CVAR2( "sys_AI",&g_cvars.sys_ai,1,0,"Enables AI Update" );
	REGISTER_CVAR2( "sys_physics",&g_cvars.sys_physics,1,0,"Enables Physics Update" );
	REGISTER_CVAR2( "sys_entities",&g_cvars.sys_entitysystem,1,0,"Enables Entities Update" );
	REGISTER_CVAR2( "sys_trackview",&g_cvars.sys_trackview,1,0,"Enables TrackView Update" );

	if (gEnv->pConsole)
		gEnv->pConsole->RegisterString("g_language", "", VF_NULL, "", CSystem::OnLanguageCVarChanged);

	REGISTER_COMMAND("sys_RestoreSpec",&SysRestoreSpecCmd,0,
		"Restore or test the cvar settings of game specific spec settings,\n"
		"'test*' and 'info' log to the log file only\n"
		"Usage: sys_RestoreSpec [test|test*|apply|info]");

	REGISTER_STRING("sys_root", m_root.c_str(),VF_READONLY,"");


	REGISTER_COMMAND("memDumpAllocs",&DumpAllocs,0,"print allocs with stack traces");
	REGISTER_COMMAND("memReplayDumpSymbols",&ReplayDumpSymbols,0,"dump symbol info to mem replay log");
	REGISTER_COMMAND("memReplayStop", &ReplayStop, 0, "stop logging to mem replay");
	REGISTER_COMMAND("memReplayPause", &ReplayPause, 0, "Pause collection of mem replay data");
	REGISTER_COMMAND("memReplayResume", &ReplayResume, 0, "Resume collection of mem replay data (use with -memReplayPaused cmdline)");
	REGISTER_COMMAND("memResetAllocs",&ResetAllocs,0,"clears memHierarchy tree");
	REGISTER_COMMAND("memReplayLabel", &AddReplayLabel, 0, "record a label in the mem replay log");
	REGISTER_COMMAND("memReplayInfo", &ReplayInfo, 0, "output some info about the replay log");













	REGISTER_CVAR2("s_CacheStatic", &g_cvars.sys_sound_nCacheStatic, 0, VF_REQUIRE_APP_RESTART,
		"Sets if the sound memory cache is a static memory block.\n"
		"Usage: s_CacheStatic [0/1]\n" 
		"0:			<auto>, PC:0, PS3:, X360:0\n"
		"Default is 0 <auto>.");

	REGISTER_CVAR2("s_CacheSize", &g_cvars.sys_sound_fCacheSize, 0.0f, VF_REQUIRE_APP_RESTART,
		"Sets the size of the sound memory cache in MB.\n"
		"Usage: s_CacheSize [0..]\n" 
		"0:			<auto>, PC:80, PS3:16, X360:32\n"
		"Default is 0 <auto>.");







//Diesel cut
	//m_displayLogo = REGISTER_INT( "displayLogo", 0, 0, "Display CryENGINE logo (0: disabled, 1: enabled)" );
//Diesel new
	m_displayLogo = REGISTER_INT("displayLogo", 0, 0, "Display StEngine logo (0: disabled, 1: enabled)");



}




/////////////////////////////////////////////////////////////////////
void CSystem::AddCVarGroupDirectory( const string &sPath )
{
	CryLog("creating CVarGroups from directory '%s' ...",sPath.c_str());

	_finddata_t fd;

	intptr_t handle = gEnv->pCryPak->FindFirst(ConcatPath(sPath,"*.cfg"), &fd );

	if(handle<0)
		return;

	do 
	{
		if(fd.attrib&_A_SUBDIR)
		{
			if(strcmp(fd.name,".")!=0 && strcmp(fd.name,"..")!=0)
				AddCVarGroupDirectory(ConcatPath(sPath,fd.name));
		}
		else
		{
			string sFilePath = ConcatPath(sPath,fd.name);

			string sCVarName = sFilePath;
			PathUtil::RemoveExtension(sCVarName);

			if (m_env.pConsole != 0)
			{
				((CXConsole*)m_env.pConsole)->RegisterCVarGroup(PathUtil::GetFile(sCVarName),sFilePath);
			}
		}
	} while (gEnv->pCryPak->FindNext( handle, &fd ) >= 0);

	gEnv->pCryPak->FindClose(handle);
}

void CSystem::OutputLoadingTimeStats()
{
  if(GetIConsole())
    if(ICVar * pVar = GetIConsole()->GetCVar("e_ProfileLevelLoading"))
      CLoadingProfilerSystem::OutputLoadingTimeStats(GetILog(), pVar->GetIVal());
}

SLoadingTimeContainer * CSystem::StartLoadingSectionProfiling(CLoadingTimeProfiler * pProfiler, const char * szFuncName)
{
  return CLoadingProfilerSystem::StartLoadingSectionProfiling(pProfiler, szFuncName);
}

void CSystem::EndLoadingSectionProfiling(CLoadingTimeProfiler * pProfiler)
{
  CLoadingProfilerSystem::EndLoadingSectionProfiling(pProfiler);
}

const char* CSystem::GetLoadingProfilerCallstack()
{
	return CLoadingProfilerSystem::GetLoadingProfilerCallstack();
}

void CSystem::RegisterErrorObserver(IErrorObserver* errorObserver)
{
    m_errorObservers.push_back(errorObserver);
}

void CSystem::OnAssert(const char* condition, const char* message, const char* fileName, unsigned int fileLineNumber)
{
    std::vector<IErrorObserver*>::const_iterator end = m_errorObservers.end();
    for (std::vector<IErrorObserver*>::const_iterator it = m_errorObservers.begin(); it != end; ++it)
    {
        (*it)->OnAssert(condition, message, fileName, fileLineNumber);
    }
}

void CSystem::OnFatalError(const char* message)
{
    std::vector<IErrorObserver*>::const_iterator end = m_errorObservers.end();
    for (std::vector<IErrorObserver*>::const_iterator it = m_errorObservers.begin(); it != end; ++it)
    {
        (*it)->OnFatalError(message);
    }
}
