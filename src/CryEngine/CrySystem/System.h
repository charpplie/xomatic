//////////////////////////////////////////////////////////////////////
//
//	Crytek CryENGINE Source code
// 
//	File: System.h	
// 
//	History:
//	-Jan 31,2001:Originally Created by Marco Corbetta
//	-: modified by all
//
//////////////////////////////////////////////////////////////////////

#ifndef SYSTEM_H
#define SYSTEM_H

#if _MSC_VER > 1000
# pragma once
#endif

#include <ISystem.h>
#include <IRenderer.h>
#include <IPhysics.h>

#include "Timer.h"
#include <CryVersion.h>
#include "CmdLine.h"
#include "CryName.h"

#include "FrameProfileSystem.h"
#include "MTSafeAllocator.h"
#include "CPUDetect.h"
#include "PakVars.h"
#include "MemoryFragmentationProfiler.h"	// CMemoryFragmentationProfiler
#include "ThreadTask.h"

struct ISoundSystem;
struct IConsoleCmdArgs;
class CServerThrottle;
class CNULLSoundSystem;
struct ICryFactoryRegistryImpl;
struct IGPUPhysicsManager;

struct ICryPerfHUD;
namespace minigui
{
	struct IMiniGUI;
}

//#if defined(LINUX) || defined(WIN32)
// The console _should_ work on AMD64, but is untested, so I'll restrict the
// console to Win32 for now.
#if defined(LINUX) || (defined(WIN32) && defined(_CPU_X86)) || defined(WIN64)
#define USE_UNIXCONSOLE 1
#endif

#if !defined(_XBOX) && !defined(LINUX) && !defined(PS3)
#define DOWNLOAD_MANAGER
#endif

#ifdef DOWNLOAD_MANAGER
#include "DownloadManager.h"
#endif //DOWNLOAD_MANAGER

#if defined(LINUX)
	#include "CryLibrary.h"
#endif

#ifdef WIN32
typedef void* WIN_HMODULE;
#else
typedef void* WIN_HMODULE;
#endif

//forward declarations
class CScriptSink;
class CLUADbg;
struct IMusicSystem;
struct SDefaultValidator;
struct IDataProbe;
class CPhysRenderer;
class CVisRegTest;

#define PHSYICS_OBJECT_ENTITY 0

typedef void (__cdecl *VTuneFunction)(void);
extern VTuneFunction VTResume;
extern VTuneFunction VTPause;

#define MAX_STREAMING_POOL_INDEX 6
#define MAX_THREAD_POOL_INDEX 6

struct SSystemCVars
{
	int sys_streaming_sleep;
	int sys_float_exceptions;
	int sys_no_crash_dialog;
	int sys_dump_aux_threads;
	int sys_keyboard_break;
	int sys_WER;
	int sys_ai;
	int sys_physics;
	int sys_entitysystem;
	int sys_trackview;
	int sys_vtune;
	float sys_update_profile_time;
	float sys_sound_fCacheSize;
	int sys_sound_nCacheStatic;
	float sys_sound_fCacheRSXSize;
	int sys_limit_phys_thread_count;




};
extern SSystemCVars g_cvars;

class CSystem;

struct SmallModuleInfo
{
	string name;
	CryModuleMemoryInfo memInfo;
};

struct SCryEngineStatsModuleInfo
{
	string name;
	CryModuleMemoryInfo memInfo;
	uint32 moduleStaticSize;
	uint32 usedInModule;
	uint32 SizeOfCode;
	uint32 SizeOfInitializedData;
	uint32 SizeOfUninitializedData;
};

struct SCryEngineStatsGlobalMemInfo
{
	int totalUsedInModules;
	int totalCodeAndStatic;
	int countedMemoryModules;
	uint64 totalAllocatedInModules;
	int totalNumAllocsInModules;
	std::vector<SCryEngineStatsModuleInfo> modules;
};

struct CProfilingSystem: public IProfilingSystem 
{
	//////////////////////////////////////////////////////////////////////////
	// VTune Profiling interface.

	// Summary:
	//	 Resumes vtune data collection.
	VIRTUAL void VTuneResume() ;
	// Summary:
	//	 Pauses vtune data collection.
	VIRTUAL void VTunePause();
	//////////////////////////////////////////////////////////////////////////

	// XBox360 Profiling interface
	// Summary:
	//	 Start data collection.
	// Argument: Filename for captured data. 
	VIRTUAL void StartProfilingX360(const char * fileName);
	// Summary:
	//	 Stop data collection and write to a hard drive.
	VIRTUAL void StopProfilingX360();

};

/*
===========================================
The System interface Class
===========================================
*/
class CXConsole;
//////////////////////////////////////////////////////////////////////
//!	ISystem implementation
class CSystem :public ISystem, public ILoadConfigurationEntrySink
{
public:
	CSystem();
	~CSystem();
	bool IsDedicated(){return m_bDedicatedServer;}
	bool IsEditor(){return m_bEditor;}
	bool IsEditorMode();
	bool IsUIFrameworkMode() { return m_bUIFrameworkMode; }

	static void OnLanguageCVarChanged(ICVar * language);

	// interface ILoadConfigurationEntrySink ----------------------------------

	virtual void OnLoadConfigurationEntry( const char *szKey, const char *szValue, const char *szGroup );

	///////////////////////////////////////////////////////////////////////////
	//! @name ISystem implementation
	//@{ 
	virtual bool Init(const SSystemInitParams &startupParams);
	virtual void Release();

	VIRTUAL SSystemGlobalEnvironment* GetGlobalEnvironment() { return &m_env; }

	const char* GetRootFolder() const { return m_root.c_str(); }

	// Release all resources.
	void	ShutDown();
	VIRTUAL bool Update( int updateFlags=0, int nPauseMode=0);

	//! Begin rendering frame.
	void	RenderBegin();
	//! Render subsystems.
	void	Render();
	//! End rendering frame and swap back buffer.
	void	RenderEnd( bool bRenderStats=true );

	//! Update screen during loading.
	void UpdateLoadingScreen();

	//! Renders the statistics; this is called from RenderEnd, but if the 
	//! Host application (Editor) doesn't employ the Render cycle in ISystem,
	//! it may call this method to render the essencial statistics
	void RenderStatistics ();

	uint32 GetUsedMemory();

	VIRTUAL void DumpMemoryUsageStatistics(bool bUseKB);
	VIRTUAL void DumpMemoryCoverage();
	void CollectMemInfo(SCryEngineStatsGlobalMemInfo&);


	void Relaunch( bool bRelaunch );
	bool IsRelaunch() const { return m_bRelaunch; };
	void SerializingFile( int mode ) { m_iLoadingMode = mode; }
	int  IsSerializingFile() const { return m_iLoadingMode; }
	void Quit();
	bool IsQuitting();
	void SetAffinity();
	VIRTUAL const char *GetUserName();
	VIRTUAL int GetApplicationInstance();
	
	IGame						*GetIGame(){ return m_env.pGame; }
	INetwork				*GetINetwork(){ return m_env.pNetwork; }
	IRenderer				*GetIRenderer(){ return m_env.pRenderer; }
	IInput					*GetIInput(){ return m_env.pInput; }
	ITimer					*GetITimer(){ return m_env.pTimer; }
	ICryPak					*GetIPak() { return m_env.pCryPak; };
	IConsole				*GetIConsole() { return m_env.pConsole; };
	IScriptSystem		*GetIScriptSystem(){ return m_env.pScriptSystem; }
	I3DEngine				*GetI3DEngine(){ return m_env.p3DEngine; }
	ICharacterManager *GetIAnimationSystem() {return m_env.pCharacterManager;}
	ISoundSystem		*GetISoundSystem(){ return m_env.pSoundSystem; }
	IMusicSystem		*GetIMusicSystem(){ return m_env.pMusicSystem; }
  IPhysicalWorld	*GetIPhysicalWorld(){ return m_env.pPhysicalWorld;}
	IMovieSystem		*GetIMovieSystem() { return m_env.pMovieSystem; };
	IAISystem				*GetAISystem(){ return m_env.pAISystem;}
	IMemoryManager	*GetIMemoryManager(){ return m_pMemoryManager;}
	IEntitySystem		*GetIEntitySystem(){ return m_env.pEntitySystem;}
	ICryFont				*GetICryFont(){ return m_env.pCryFont; }
	ILog						*GetILog()
	{ 



		return m_env.pLog; 

	}
	ICmdLine				*GetICmdLine(){ return m_pCmdLine; }
	IStreamEngine   *GetStreamEngine();
	IValidator			*GetIValidator() { return m_pValidator; };
	IFrameProfileSystem* GetIProfileSystem() { return &m_FrameProfileSystem; }
	VIRTUAL IDiskProfiler *GetIDiskProfiler() { return m_pDiskProfiler; }
	const char			*GetGameMOD() { if (m_szGameMOD[0]) return (m_szGameMOD);return (NULL); }
	INameTable      *GetINameTable() { return m_env.pNameTable; };
	IBudgetingSystem* GetIBudgetingSystem()  { return( m_pIBudgetingSystem ); }
	IFlowSystem     *GetIFlowSystem() { return m_env.pFlowSystem; }
	IDialogSystem   *GetIDialogSystem() { return m_env.pDialogSystem; }
	IHardwareMouse	*GetIHardwareMouse() { return m_env.pHardwareMouse; }
	IAnimationGraphSystem *GetIAnimationGraphSystem() { return m_env.pAnimationGraphSystem; }
	ISystemEventDispatcher*GetISystemEventDispatcher() { return m_pSystemEventDispatcher; }
	ITestSystem			*GetITestSystem() { return m_pTestSystem; }
	IThreadTaskManager* GetIThreadTaskManager();
	IResourceManager *GetIResourceManager();
	ITextModeConsole *GetITextModeConsole();
	Crc32Gen				*GetCrc32Gen();
	IFileChangeMonitor	*GetIFileChangeMonitor() { return m_env.pFileChangeMonitor; }
	IVisualLog			*GetIVisualLog() { return m_env.pVisualLog; }
	INotificationNetwork *GetINotificationNetwork() { return m_pNotificationNetwork; }
	IProfilingSystem * GetIProfilingSystem() { return &m_ProfilingSystem;}
	IPlatformOS			*GetPlatformOS() { return m_pPlatformOS.get(); }
	ICryPerfHUD     *GetPerfHUD() { return m_pPerfHUD; }
	//////////////////////////////////////////////////////////////////////////
	// retrieves the perlin noise singleton instance
	CPNoise3* GetNoiseGen();
	VIRTUAL uint64 GetUpdateCounter() { return m_nUpdateCounter; };

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
	IGPUPhysicsManager	*GetIGPUPhysicsManager() { return (IGPUPhysicsManager *) m_pGPUPhysics; }
#endif

	VIRTUAL void SetLoadingProgressListener(ILoadingProgressListener *pLoadingProgressListener)
	{
		m_pProgressListener = pLoadingProgressListener;
	};

	VIRTUAL ILoadingProgressListener *GetLoadingProgressListener() const
	{
		return m_pProgressListener;
	};

	void		SetIGame(IGame* pGame) {m_env.pGame = pGame;}
	void    SetIFlowSystem(IFlowSystem* pFlowSystem) { m_env.pFlowSystem = pFlowSystem; }
	void    SetIAnimationGraphSystem(IAnimationGraphSystem * pAnimationGraphSystem) { m_env.pAnimationGraphSystem = pAnimationGraphSystem; }
	void    SetIDialogSystem(IDialogSystem * pDialogSystem) { m_env.pDialogSystem = pDialogSystem; }
	void    SetIMaterialEffects(IMaterialEffects* pMaterialEffects) { m_env.pMaterialEffects = pMaterialEffects; }
	void		SetIParticleManager(IParticleManager* pParticleManager) { m_env.pParticleManager = pParticleManager; }
	void    SetIFileChangeMonitor(IFileChangeMonitor * pFileChangeMonitor) { m_env.pFileChangeMonitor = pFileChangeMonitor; }
	void    SetIVisualLog(IVisualLog * pVisualLog) { m_env.pVisualLog = pVisualLog; }
	void    ChangeUserPath( const char *sUserPath );
	void		DetectGameFolderAccessRights();

	VIRTUAL void ExecuteCommandLine();

	VIRTUAL void GetUpdateStats(SSystemUpdateStats& stats);

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL XmlNodeRef CreateXmlNode( const char *sNodeName="" );
	VIRTUAL XmlNodeRef LoadXmlFile( const char *sFilename );
	VIRTUAL XmlNodeRef LoadXmlFromString( const char *sXmlString );
	VIRTUAL IXmlUtils* GetXmlUtils();
	//////////////////////////////////////////////////////////////////////////

	void SetViewCamera( CCamera &Camera){ m_ViewCamera = Camera; }
	CCamera& GetViewCamera() { return m_ViewCamera; }

  VIRTUAL int GetCPUFlags()
  {
    int Flags = 0;
    if (!m_pCpu)
      return Flags;
    if (m_pCpu->hasMMX())
      Flags |= CPUF_MMX;
    if (m_pCpu->hasSSE())
      Flags |= CPUF_SSE;
    if (m_pCpu->hasSSE2())
      Flags |= CPUF_SSE;
    if (m_pCpu->has3DNow())
      Flags |= CPUF_3DNOW;

    return Flags;
  }
	VIRTUAL int GetLogicalCPUCount()
	{
		if(m_pCpu)
			return m_pCpu->GetLogicalCPUCount();
		return 0;
	}
  VIRTUAL double GetSecondsPerCycle()
  {
    if (!m_pCpu)
      return 0;
    else
      return m_pCpu->m_Cpu[0].m_SecondsPerCycle;
  }

	void IgnoreUpdates( bool bIgnore ) { m_bIgnoreUpdates = bIgnore; };
	void SetGCFrequency( const float fRate );

	void SetIProcess(IProcess *process);
	IProcess* GetIProcess(){ return m_pProcess; }

	bool IsTestMode() const { return m_bTestMode; }
	//@}

	void SleepIfNeeded();
	void SleepIfInactive();

	VIRTUAL void FatalError( const char *format,... ) PRINTF_PARAMS(2, 3);
	// Validator Warning.
	void WarningV( EValidatorModule module,EValidatorSeverity severity,int flags,const char *file,const char *format,va_list args );
	void Warning( EValidatorModule module,EValidatorSeverity severity,int flags,const char *file,const char *format,... );
	bool CheckLogVerbosity( int verbosity );
		
	VIRTUAL void DebugStats(bool checkpoint, bool leaks);
	void DumpWinHeaps();
	
	// tries to log the call stack . for DEBUG purposes only
	void LogCallStack();

	VIRTUAL int DumpMMStats(bool log);

	//! Return pointer to user defined callback.
	ISystemUserCallback* GetUserCallback() const { return m_pUserCallback; };

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void SaveConfiguration();
	VIRTUAL void LoadConfiguration( const char *sFilename, ILoadConfigurationEntrySink *pSink=0 );
	VIRTUAL ESystemConfigSpec GetConfigSpec( bool bClient=true );
	VIRTUAL void SetConfigSpec( ESystemConfigSpec spec,bool bClient );
	VIRTUAL ESystemConfigSpec GetMaxConfigSpec() const;
	//////////////////////////////////////////////////////////////////////////

	VIRTUAL int SetThreadState(ESubsystem subsys, bool bActive);
	VIRTUAL ICrySizer* CreateSizer();
	VIRTUAL bool IsPaused() const { return m_bPaused; };
	VIRTUAL IFlashPlayer* CreateFlashPlayerInstance() const;
	VIRTUAL void SetFlashLoadMovieHandler(IFlashLoadMovieHandler* pHandler) const;

	//////////////////////////////////////////////////////////////////////////
	// Creates an instance of the AVI Reader class.
	VIRTUAL IAVI_Reader *CreateAVIReader();
	// Release the AVI reader
	VIRTUAL void ReleaseAVIReader(IAVI_Reader *pAVIReader);

	VIRTUAL ILocalizationManager* GetLocalizationManager();
	VIRTUAL void debug_GetCallStack( const char **pFunctions,int &nCount );
	VIRTUAL void debug_LogCallStack( int nMaxFuncs=32,int nFlags=0 );
	VIRTUAL void ApplicationTest( const char *szParam );

	VIRTUAL ICryFactoryRegistry* GetCryFactoryRegistry() const;















public:
		// this enumeration describes the purpose for which the statistics is gathered.
	// if it's gathered to be dumped, then some different rules may be applied
	enum MemStatsPurposeEnum {nMSP_ForDisplay, nMSP_ForDump, nMSP_ForCrashLog, nMSP_ForBudget};
	// collects the whole memory statistics into the given sizer object
	void CollectMemStats (class CrySizerImpl* pSizer, MemStatsPurposeEnum nPurpose = nMSP_ForDisplay, std::vector<SmallModuleInfo> * pStats = 0);
	void GetExeSizes (ICrySizer* pSizer, MemStatsPurposeEnum nPurpose = nMSP_ForDisplay);
	//! refreshes the m_pMemStats if necessary; creates it if it's not created
	void TickMemStats(MemStatsPurposeEnum nPurpose = nMSP_ForDisplay, IResourceCollector *pResourceCollector=0 );

private:
	bool LoadEngineDLLs();

	//! @name Initialization routines
	//@{ 
	bool IntializeEngineModule( const char *dllName,const char *moduleClassName,const SSystemInitParams &initParams, bool bQuitIfNotFound = true );

	bool InitNetwork( const SSystemInitParams &startupParams );
	bool InitInput( const SSystemInitParams &startupParams );

	bool InitConsole();
	bool InitRenderer(WIN_HINSTANCE hinst,WIN_HWND hwnd,const SSystemInitParams &initParams );
	bool InitSound( const SSystemInitParams &initParams );
	bool InitPhysics( const SSystemInitParams &initParams );
	bool InitPhysicsRenderer( const SSystemInitParams &initParams );

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
	bool InitGPUPhysics();
#endif

	bool InitFont( const SSystemInitParams &initParams );
	bool InitFlash();
	bool InitAISystem( const SSystemInitParams &initParams );
	bool InitScriptSystem( const SSystemInitParams &initParams );
	bool InitFileSystem();
	bool InitFileSystem_LoadEngineFolders();
	bool InitStreamEngine();
	bool Init3DEngine( const SSystemInitParams &initParams );
	bool InitAnimationSystem( const SSystemInitParams &initParams );
	bool InitMovieSystem( const SSystemInitParams &initParams );
	bool InitEntitySystem( const SSystemInitParams &initParams );
	bool OpenRenderLibrary(int type, const SSystemInitParams &initParams );
	int AutoDetectRenderer(char *Vendor, char *Device);
  bool OpenRenderLibrary(const char *t_rend, const SSystemInitParams &initParams );
  bool CloseRenderLibrary();
	//@}
	void Strange();
	bool ParseSystemConfig(string &sFileName);

	//////////////////////////////////////////////////////////////////////////
	// Helper functions.
	//////////////////////////////////////////////////////////////////////////
	void CreateRendererVars(const SSystemInitParams &startupParams);
	void CreateSystemVars();
	void RenderLogos();
	void RenderStats();
	void RenderPS3Stats( bool bRenderStats );
	void RenderMemStats();
	void RenderFlashInfo();
	void RenderPhysicsHelpers();
	void GetFlashMemoryUsage(ICrySizer* pSizer);
	WIN_HMODULE LoadDLL( const char *dllName, bool bQuitIfNotFound=true);
	void FreeLib( WIN_HMODULE hLibModule);
	void QueryVersionInfo();
	void LogVersion();
	void LogBuildInfo();
	void SetDevMode( bool bEnable );
	void InitScriptDebugger();

	void CreatePhysicsThread();
	void KillPhysicsThread();

	bool ReLaunchMediaCenter();
	void LogSystemInfo();

	// recursive
	// Arguments:
	//   sPath - e.g. "Game/Config/CVarGroups"
	void AddCVarGroupDirectory( const string &sPath );

	wstring GetErrorStringUnsupportedGPU(const char* gpuName, unsigned int gpuVendorId, unsigned int gpuDeviceId);

	WIN_HMODULE LoadDynamiclibrary( const char *dllName ) const;

public:
	void EnableFloatExceptions( int type );

	// interface ISystem -------------------------------------------
	VIRTUAL IDataProbe* GetIDataProbe() { return m_pDataProbe; };
	VIRTUAL void SetForceNonDevMode( const bool bValue );
	VIRTUAL bool GetForceNonDevMode() const;
	VIRTUAL bool WasInDevMode() const { return m_bWasInDevMode; };
	VIRTUAL bool IsDevMode() const { return m_bInDevMode && !GetForceNonDevMode(); }
	VIRTUAL bool IsMODValid(const char *szMODName) const 
	{ 
		if (!szMODName || strstr(szMODName,".") || strstr(szMODName,"\\") || stricmp(szMODName,PathUtil::GetGameFolder().c_str())==0) 
			return (false);
		return (true);
	}
	VIRTUAL void AutoDetectSpec();

	// -------------------------------------------------------------

	//! attaches the given variable to the given container;
	//! recreates the variable if necessary
	ICVar* attachVariable (const char* szVarName, int* pContainer, const char *szComment,int dwFlags=0 );

	CCpuFeatures* GetCPUFeatures() { return m_pCpu; };

	string& GetDelayedScreeenshot() {return m_sDelayedScreeenshot;}

private: // ------------------------------------------------------

	// System environment.




	SSystemGlobalEnvironment m_env;


	CTimer								m_Time;								//!<
	CCamera								m_ViewCamera;					//!<
	bool									m_bQuit;							//!< if is true the system is quitting
	bool									m_bRelaunch;					//!< relaunching the app or not (true beforerelaunch)
	bool									m_bRelaunched;				//!< Application was started with the -RELAUNCH option (true after relaunch)
	int										m_iLoadingMode;				//!< Game is loading w/o changing context (0 not, 1 quickloading, 2 full loading)
	bool									m_bTestMode;					//!< If running in testing mode.
	bool									m_bEditor;						//!< If running in Editor.
	bool									m_bNoCrashDialog;
	bool                  m_bPreviewMode;       //!< If running in Preview mode.
	bool									m_bUIFrameworkMode;
	bool									m_bDedicatedServer;		//!< If running as Dedicated server.
	bool									m_bIgnoreUpdates;			//!< When set to true will ignore Update and Render calls,
	IValidator *					m_pValidator;					//!< Pointer to validator interface.
	bool									m_bForceNonDevMode;		//!< true when running on a cheat protected server or a client that is connected to it (not used in singlplayer)
	bool									m_bWasInDevMode;			//!< Set to true if was in dev mode.
	bool									m_bInDevMode;					//!< Set to true if was in dev mode.
	bool                  m_bGameFolderWritable;//!< True when verified that current game folder have write access.
	SDefaultValidator *		m_pDefaultValidator;	//!<
	int										m_nStrangeRatio;			//!<
	string								m_sDelayedScreeenshot;//!< to delay a screenshot call for a frame
  CCpuFeatures *				m_pCpu;								//!< CPU features

	int										m_iTraceAllocations;
	int										m_iUseFileCaching;
	//! DLLs handles.
	struct SDllHandles
	{
		WIN_HMODULE hRenderer;
		WIN_HMODULE hInput;
    WIN_HMODULE hFlash;
		WIN_HMODULE hSound;
		WIN_HMODULE hEntitySystem;
		WIN_HMODULE hNetwork;
		WIN_HMODULE hAI;
		WIN_HMODULE	hMovie;
		WIN_HMODULE	hPhysics;
		WIN_HMODULE	hFont;
		WIN_HMODULE hScript;
		WIN_HMODULE h3DEngine;
		WIN_HMODULE hAnimation;
		WIN_HMODULE hIndoor;
		WIN_HMODULE hGame;
#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
		WIN_HMODULE	hPhysicsGPU;
#endif
	};
	SDllHandles m_dll;

	std::vector<WIN_HMODULE> m_moduleDLLHandles;

	//! THe streaming engine
	class CRefStreamEngine* m_pStreamEngine;
	
	//! current active process
	IProcess *m_pProcess;

	IMemoryManager *m_pMemoryManager;

	CPhysRenderer *m_pPhysRenderer;
	CCamera m_PhysRendererCamera;
	ICVar *m_p_draw_helpers_str;
	int m_iJumpToPhysProfileEnt;

	//! system event dispatcher
	ISystemEventDispatcher * m_pSystemEventDispatcher;

	//! The default font
	IFFont*	m_pIFont;

	//! System to monitor given budget.
	IBudgetingSystem* m_pIBudgetingSystem;

	//! NULL Implementations
	CNULLSoundSystem* m_pNULLSoundSystem;

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
	// Function pointers from the dll
	CreateGPUPhysicsProc pfnCreateGPUPhysics;
	//	DeleteGPUPhysicsProc pfnDeleteGPUPhysics;
	//	SceneSetupGPUPhysicsProc pfnSceneSetupGPUPhysics;
	//	RenderGPUPhysicsProc pfnRenderGPUPhysics;
	//	RenderGPUPhysicsMessagesProc pfnRenderGPUPhysicsMessages;
	// GPU physics system
	IGPUPhysicsManager *m_pGPUPhysics;
#endif









	// XML Utils interface.
	class CXmlUtils *m_pXMLUtils;

	//! game path folder
	char	m_szGameMOD[256];

	//! global root folder
	string m_root;
	int m_iApplicationInstance;

	//! to hold the values stored in system.cfg
	//! because editor uses it's own values,
	//! and then saves them to file, overwriting the user's resolution.
	int m_iHeight;
	int m_iWidth;
	int m_iColorBits;
	
	// System console variables.
	//////////////////////////////////////////////////////////////////////////

	// DLL names
	ICVar *m_sys_dll_ai;
	ICVar *m_sys_dll_game;
	ICVar *m_sys_game_folder;
	ICVar *m_sys_user_folder;

	ICVar *m_cvAIUpdate;
	ICVar *m_rWidth;
	ICVar *m_rHeight;
	ICVar *m_rColorBits;
	ICVar *m_rDepthBits;
	ICVar *m_rStencilBits;
	ICVar *m_rFullscreen;
	ICVar *m_rDriver;
	ICVar *m_cvGameName;
	ICVar *m_rDisplayInfo;
	ICVar *m_sysNoUpdate;
	ICVar *m_cvEntitySuppressionLevel;
	ICVar *m_pCVarQuit;
	ICVar *m_cvMemStats;
	ICVar *m_cvMemStatsThreshold;
	ICVar *m_cvMemStatsMaxDepth;
	ICVar *m_sysWarnings;										//!< might be 0, "sys_warnings" - Treat warning as errors.
	ICVar *m_cvSSInfo;											//!< might be 0, "sys_SSInfo" 0/1 - get file sourcesafe info
	ICVar *m_svDedicatedMaxRate;
	ICVar *m_svAISystem;
	ICVar *m_sys_profile;
	ICVar *m_sys_profile_additionalsub;
	ICVar *m_sys_profile_graph;
	ICVar *m_sys_profile_graphScale;
	ICVar *m_sys_profile_pagefaultsgraph;
	ICVar *m_sys_profile_filter;
	ICVar *m_sys_profile_filter_thread;
	ICVar *m_sys_profile_allThreads;
	ICVar *m_sys_profile_network;
	ICVar *m_sys_profile_peak;
	ICVar *m_sys_profile_peak_time;
	ICVar *m_sys_profile_memory;
	ICVar *m_sys_profile_sampler;
	ICVar *m_sys_profile_sampler_max_samples;
	ICVar *m_sys_mtrace;
	ICVar *m_sys_mtrace_print;
	ICVar *m_sys_spu_enable;
	ICVar *m_sys_spu_profile;
	ICVar *m_sys_spu_max;
	ICVar *m_sys_spu_flipmode;
	ICVar *m_sys_spu_debug;
	ICVar *m_sys_spu_filter;
	ICVar *m_sys_spu_dump_stats;
	ICVar *m_sys_spu_streaming;
	ICVar *m_sys_spec;
	ICVar *m_sys_firstlaunch;
	ICVar *m_sys_StreamCallbackTimeBudget;
	ICVar *m_sys_SaveCVars;

	ICVar *m_sys_physics_CPU;

	ICVar * m_sys_SimulateTask;

	ICVar *m_displayLogo;

	ICVar *m_sys_min_step;
	ICVar *m_sys_max_step;
	ICVar *m_sys_enable_budgetmonitoring;
	ICVar *m_sys_memory_debug;
	ICVar *m_sys_preload;
	ICVar *m_sys_crashtest;
	ICVar *m_sys_writePixLog;
//	ICVar *m_sys_filecache;
	ICVar *m_gpu_particle_physics;

	string	m_sSavedRDriver;								//!< to restore the driver when quitting the dedicated server

	ICVar* m_cvPakPriority;
	ICVar* m_cvPakReadSlice;
	ICVar* m_cvPakLogMissingFiles;
	// the contents of the pak priority file
	PakVars m_PakVar;
	ICVar *m_cvEarlyMovieUpdate;						//!< 0 (default) needed for game, 1 better for having artifact free movie playback (camera pos is valid during entity update)

	//////////////////////////////////////////////////////////////////////////
	//! User define callback for system events.
	ISystemUserCallback *m_pUserCallback;

	WIN_HWND		m_hWnd;
	WIN_HINSTANCE	m_hInst;

	// this is the memory statistics that is retained in memory between frames
	// in which it's not gathered
	class CrySizerStats* m_pMemStats;
	class CrySizerImpl* m_pSizer;

	CFrameProfileSystem m_FrameProfileSystem;
	class CThreadProfiler *m_pThreadProfiler;
	class IDiskProfiler*	m_pDiskProfiler;
	
	std::auto_ptr<IPlatformOS> m_pPlatformOS;
	ICryPerfHUD        *m_pPerfHUD;
	minigui::IMiniGUI  *m_pMiniGUI;
	ITexture           *m_pCELogo;

	//int m_nCurrentLogVerbosity;

	SFileVersion m_fileVersion;
	SFileVersion m_productVersion;
	IDataProbe *m_pDataProbe;

	class CLocalizedStringsManager *m_pLocalizationManager;

	// Name table.
	CNameTable m_nameTable;

	IThreadTask *m_PhysThread;

	ESystemConfigSpec m_nServerConfigSpec;
	ESystemConfigSpec m_nMaxConfigSpec;

	std::auto_ptr<CServerThrottle> m_pServerThrottle;

	CProfilingSystem m_ProfilingSystem;

	// Pause mode.
	bool m_bPaused;

	uint64 m_nUpdateCounter;

	int sys_ProfileLevelLoading, sys_ProfileLevelLoadingDump;

public:
	//! Pointer to the download manager
	class CDownloadManager	*m_pDownloadManager;

	ICVar *m_sys_main_CPU;
	ICVar *m_sys_streaming_CPU;
	ICVar *m_sys_streamingpool_CPU[MAX_STREAMING_POOL_INDEX];
	ICVar *m_sys_TaskThread_CPU[MAX_THREAD_POOL_INDEX];



#ifdef USE_FRAME_PROFILER
	void SetFrameProfiler(bool on, bool display, char *prefix) { m_FrameProfileSystem.SetProfiling(on, display, prefix, this); };
#else
	void SetFrameProfiler(bool on, bool display, char *prefix) {};
#endif

	//////////////////////////////////////////////////////////////////////////
	// File version.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL const SFileVersion& GetFileVersion();
	VIRTUAL const SFileVersion& GetProductVersion();

	bool WriteCompressedFile(const char *filename, void *data, unsigned int bitlen);
	unsigned int ReadCompressedFile(const char *filename, void *data, unsigned int maxbitlen);
	unsigned int GetCompressedFileSize(const char *filename);
	bool CompressDataBlock(const void * input, size_t inputSize, void * output, size_t& outputSize, int level);
	bool DecompressDataBlock( const void * input, size_t inputSize, void * output, size_t& outputSize );
	void InitVTuneProfiler();

	void OpenBasicPaks();
	void OpenLanguagePak( const char *sLanguage );
	void OpenConfigPak( const char *pakName = 0);
	void GetLocalizedPath( const char *sLanguage, string& sLocalizedPath );
	void CloseLanguagePak( const char *sLanguage );

	void	Deltree(const char *szFolder, bool bRecurse);
	void UpdateMovieSystem( const int updateFlags, const float fFrameTime );

  // level loading profiling
  VIRTUAL void OutputLoadingTimeStats();
  VIRTUAL struct SLoadingTimeContainer * StartLoadingSectionProfiling(CLoadingTimeProfiler * pProfiler, const char * szFuncName);
  VIRTUAL void EndLoadingSectionProfiling(CLoadingTimeProfiler * pProfiler);
	VIRTUAL const char* GetLoadingProfilerCallstack();

  VIRTUAL void RegisterErrorObserver(IErrorObserver* errorObserver);
  VIRTUAL void OnAssert(const char* condition, const char* message, const char* fileName, unsigned int fileLineNumber);
  void OnFatalError(const char* message);

private:
    std::vector<IErrorObserver*> m_errorObservers;

public:
	void InitLocalization();

protected: // -------------------------------------------------------------

	ILoadingProgressListener *		m_pProgressListener;
  CCmdLine *										m_pCmdLine;
	ITestSystem	*									m_pTestSystem;				// needed for external test application (0 if not activated yet)
	CVisRegTest *                 m_pVisRegTest;
	CThreadTaskManager*           m_pThreadTaskManager;
	class CResourceManager*       m_pResourceManager;
	INotificationNetwork *m_pNotificationNetwork;
	
	string	m_binariesDir;

	std::vector< std::pair<CTimeValue, float> > m_updateTimes;

	CMemoryFragmentationProfiler	m_MemoryFragmentationProfiler;

	friend struct SDefaultValidator;
	friend struct SCryEngineFoldersLoader;
//	friend void ScreenshotCmd( IConsoleCmdArgs *pParams );
};

/*extern static */bool QueryModuleMemoryInfo( SCryEngineStatsModuleInfo &moduleInfo,int index );

#endif // SYSTEM_H
