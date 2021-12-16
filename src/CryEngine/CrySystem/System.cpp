//////////////////////////////////////////////////////////////////////
//
//	Crytek Source code
// 
//	File: System.cpp
//  Description: CryENGINE system core-handle all subsystems
// 
//	History:
//	-Jan 31,2001: Originally Created by Marco Corbetta
//	-: modified by all
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "System.h"
#include <time.h>
//#include "ini_vars.h"
#include "CryLibrary.h"





#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
	// If app hasn't choosen, set to work with Windows 98, Windows Me, Windows 2000, Windows XP and beyond
#include <windows.h>
#endif

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
#include <IReverbManager.h>
#include <IMusicSystem.h>
#include "NullImplementation/NULLSoundSystem.h"
#include <ICryAnimation.h>
#include <IScriptSystem.h>
#include <IProcess.h>
#include <IBudgetingSystem.h>
#include <IGame.h>
#include <IGameFramework.h>
#include <INotificationNetwork.h>
#include "TestSystemLegacy.h"							// CTestSystem
#include "VisRegTest.h"

#include "CryPak.h"
#include "XConsole.h"
#include "Log.h"
#include "CrySizerStats.h"
#include "CrySizerImpl.h"
#include "NotificationNetwork.h"
#include "ProfileLog.h"

#include "XML/xml.h"
#include "XML/ReadWriteXMLSink.h"
#include "DataProbe.h"

#include "StreamEngine.h"
#include "PhysRenderer.h"

#include "LocalizedStringManager.h"
#include "XML/XmlUtils.h"
#include "ThreadProfiler.h"
#include "IDiskProfiler.h"
#include "SystemEventDispatcher.h"
#include "HardwareMouse.h"
#include "ServerThrottle.h"
#include "ILocalMemoryUsage.h"
#include "ResourceManager.h"
#include "MemoryManager.h"

#include <crc32.h>
#include <PNoise3.h>

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
#include <IPhysicsGPU.h>
#endif





#include "CryWaterMark.h"
WATERMARKDATA(_m);

#ifdef WIN32
#define  PROFILE_WITH_VTUNE
#include <process.h>
#include <malloc.h>
#endif





// profilers api.
VTuneFunction VTResume = NULL;
VTuneFunction VTPause = NULL;

// Define global cvars.
SSystemCVars g_cvars;

#include "UnixConsole.h"
extern ITextModeConsole* g_pUnixConsole;

extern int CryMemoryGetAllocatedSize();

// these heaps are used by underlying System structures
// to allocate, accordingly, small (like elements of std::set<..*>) and big (like memory for reading files) objects
// hopefully someday we'll have standard MT-safe heap
//CMTSafeHeap g_pakHeap;
CMTSafeHeap* g_pPakHeap = 0;// = &g_pakHeap;





//////////////////////////////////////////////////////////////////////////
#include "Validator.h"















































/////////////////////////////////////////////////////////////////////////////////
// System Implementation.
//////////////////////////////////////////////////////////////////////////
CSystem::CSystem() 



{
	m_iHeight = 0;
	m_iWidth = 0;
	m_iColorBits = 0;
	// CRT ALLOCATION threshold

	m_pSystemEventDispatcher = new CSystemEventDispatcher(); // Must be first.

	//#ifndef _XBOX
#ifdef WIN32	
	m_hInst = NULL;
	m_hWnd = NULL;
//	int sbh = _set_sbh_threshold(1016); //Diesel cut
#endif

	//////////////////////////////////////////////////////////////////////////
	// Clear environment.
	//////////////////////////////////////////////////////////////////////////
	memset( &m_env,0,sizeof(m_env) );

	//////////////////////////////////////////////////////////////////////////
	// Reset handles.
	memset( &m_dll,0,sizeof(m_dll) );
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Initialize global environment interface pointers.
	m_env.pSystem = this;
	m_env.pTimer = &m_Time;
	m_env.pNameTable = &m_nameTable;
	m_env.pFrameProfileSystem = &m_FrameProfileSystem;
	m_env.bClient = false;
	m_env.bServer = false;
	m_env.bMultiplayer = false;
	m_env.bHostMigrating = false;
	m_env.bProfilerEnabled = false;
	m_env.callbackStartSection = 0;
	m_env.callbackEndSection = 0;
	m_env.bIgnoreAllAsserts = false;
	m_env.bTesting = false;
	//////////////////////////////////////////////////////////////////////////
	

	m_pStreamEngine = NULL;
	m_PhysThread = 0;

	m_pIFont = NULL;
	m_pTestSystem = NULL;
	m_pVisRegTest = NULL;
	m_rWidth=NULL;
	m_rHeight=NULL;
	m_rColorBits=NULL;
	m_rDepthBits=NULL;
	m_cvSSInfo=NULL;
	m_rStencilBits=NULL;
	m_rFullscreen=NULL;
	m_rDriver=NULL;
	m_sysNoUpdate=NULL;
	m_pMemoryManager = NULL;
	m_pProcess = NULL;
	
	m_pValidator = NULL;
	m_pCmdLine = NULL;
	m_pDefaultValidator = NULL;
	m_pIBudgetingSystem = NULL;
	m_pLocalizationManager = NULL;
	m_pNULLSoundSystem = NULL;
	m_sys_SaveCVars=0;
	m_sys_StreamCallbackTimeBudget=0;
	m_sys_physics_CPU=0;
	m_sys_min_step=0;
	m_sys_max_step=0;

	m_pNotificationNetwork = NULL;

	m_cvAIUpdate = NULL;

	m_pUserCallback = NULL;
	m_sys_memory_debug=NULL;
	m_sysWarnings = NULL;
	m_sys_profile = NULL;
	m_sys_profile_additionalsub = NULL;
	m_sys_profile_graphScale = NULL;
	m_sys_profile_pagefaultsgraph = NULL;
	m_sys_profile_graph = NULL;
	m_sys_profile_filter = NULL;
	m_sys_profile_filter_thread = NULL;
	m_sys_profile_allThreads = NULL;
	m_sys_profile_network = NULL;
	m_sys_profile_peak = NULL;
	m_sys_profile_peak_time = NULL;
	m_sys_profile_memory = NULL;
	m_sys_profile_sampler = NULL;
	m_sys_profile_sampler_max_samples = NULL;
  m_sys_mtrace = NULL;
	m_sys_spu_enable = NULL;
	m_sys_spu_profile = NULL;
	m_sys_spu_max = NULL;
	m_sys_spu_flipmode = NULL;
	m_sys_spu_debug = NULL;
	m_sys_spu_filter = NULL;
	m_sys_spu_dump_stats = NULL;
	m_sys_spu_streaming = NULL;
	m_sys_spec = NULL;
	m_sys_firstlaunch = NULL;
	m_sys_enable_budgetmonitoring = NULL;
	m_sys_preload = NULL;
	m_sys_crashtest = NULL;
	m_sys_writePixLog = NULL;
//	m_sys_filecache = NULL;
	m_gpu_particle_physics = NULL;
	m_pCpu = NULL;
	m_sys_game_folder = NULL;

	
	m_bQuit = false;
	m_bRelaunch = false;
	m_bRelaunched = false;
	m_iLoadingMode = 0;
	m_bTestMode = false;
	m_bEditor = false;
	m_bPreviewMode = false;
	m_bIgnoreUpdates = false;
	m_bNoCrashDialog = false;

	m_nStrangeRatio = 1000;
	// no mem stats at the moment
	m_pMemStats = NULL;
	m_pSizer = NULL;

	m_pCVarQuit = NULL;

	m_cvPakPriority = NULL;

	m_pDownloadManager = 0;
	// default game MOD is root
	memset(m_szGameMOD,0,256);
	m_pDataProbe = 0;
#if defined( _DATAPROBE )
	m_pDataProbe = new CDataProbe;
#endif
	m_bForceNonDevMode=false;
	m_bWasInDevMode = false;
	m_bInDevMode = false;
	m_bGameFolderWritable = false;

	m_nServerConfigSpec = CONFIG_VERYHIGH_SPEC;
	m_nMaxConfigSpec = CONFIG_PS3;

	//m_hPhysicsThread = INVALID_HANDLE_VALUE;
	//m_hPhysicsActive = INVALID_HANDLE_VALUE;
	//m_bStopPhysics = 0;
	//m_bPhysicsActive = 0;

	m_pProgressListener = 0;

	m_bPaused = false;
	m_nUpdateCounter = 0;
	m_iApplicationInstance = -1;

	m_pPhysRenderer = 0;

	m_pXMLUtils = new CXmlUtils(this);
	m_pTestSystem = new CTestSystemLegacy;
	m_pMemoryManager = CryGetIMemoryManager();
	m_pThreadTaskManager = new CThreadTaskManager;
	m_pResourceManager = new CResourceManager;
	m_pThreadProfiler = 0;
	m_pDiskProfiler = NULL;

	m_pMiniGUI = NULL;
	m_pPerfHUD = NULL;
	m_pCELogo = NULL;

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
	m_pGPUPhysics = NULL;
	pfnCreateGPUPhysics = NULL;
#endif

	g_pPakHeap = new CMTSafeHeap;

	m_bUIFrameworkMode = false;
















}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
CSystem::~CSystem()
{
	ShutDown();
	FreeLib(m_dll.hNetwork);
	FreeLib(m_dll.hAI);
	FreeLib(m_dll.hInput);
	FreeLib(m_dll.hScript);
	FreeLib(m_dll.hPhysics);
	FreeLib(m_dll.hEntitySystem);
	FreeLib(m_dll.hRenderer);
	FreeLib(m_dll.hFlash);
	FreeLib(m_dll.hFont);
	FreeLib(m_dll.hMovie);
	FreeLib(m_dll.hIndoor);
	FreeLib(m_dll.h3DEngine);
	FreeLib(m_dll.hAnimation);
	FreeLib(m_dll.hGame);
	FreeLib(m_dll.hSound);
#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
	FreeLib(m_dll.hPhysicsGPU);
#endif
	SAFE_DELETE(m_pThreadProfiler);
	SAFE_DELETE(m_pDiskProfiler);	
	SAFE_DELETE(m_pDataProbe);
	SAFE_DELETE(m_pXMLUtils);
	SAFE_DELETE(m_pThreadTaskManager);
	SAFE_DELETE(m_pResourceManager);
	SAFE_DELETE(m_pSystemEventDispatcher);
	SAFE_DELETE(m_pMemoryManager);

	SAFE_DELETE(g_pPakHeap);












}

//////////////////////////////////////////////////////////////////////////
void CSystem::Release()
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::FreeLib( WIN_HMODULE hLibModule)
{
	if (hLibModule) 
	{ 
		CryFreeLibrary(hLibModule);
		(hLibModule)=NULL; 
	} 
}

//////////////////////////////////////////////////////////////////////////
IStreamEngine* CSystem::GetStreamEngine()
{
	return m_pStreamEngine;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetForceNonDevMode( const bool bValue )
{
	m_bForceNonDevMode=bValue;
	if (bValue)
		SetDevMode(false);
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::GetForceNonDevMode() const
{
	return m_bForceNonDevMode;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetDevMode( bool bEnable )
{
	/*
	//////////////////////////////////////////////////////////////////////////
	// Timur
	//////////////////////////////////////////////////////////////////////////
	// For now always in dev mode.
	m_bWasInDevMode = true;
	m_bInDevMode = true; 
	return;
	*/
	
	if (!bEnable)
	{
		// set pak priority to files inside the pak to avoid
		// trying to open unnecessary files and to avoid cheating
		ICVar *cVar = m_env.pConsole != NULL ? m_env.pConsole->GetCVar("lua_debugger") : NULL;
		if (cVar) 
			cVar->ForceSet("0");
		if (m_cvPakPriority)
			m_cvPakPriority->ForceSet("1");
	}	
	else
	{
		if (m_cvPakPriority)
			m_cvPakPriority->ForceSet("0");
	}	
	if (bEnable)
		m_bWasInDevMode = true;
	m_bInDevMode = bEnable;
}


void LvlRes_export( IConsoleCmdArgs *pParams );

///////////////////////////////////////////////////
void CSystem::ShutDown()
{		
	CryLogAlways("System Shutdown");

	if (m_pUserCallback)
		m_pUserCallback->OnShutdown();

	KillPhysicsThread();
	
	if (m_env.pSoundSystem && m_env.pSoundSystem->GetIReverbManager())
	{
		// turn EAX off otherwise it affects all Windows sounds!
		m_env.pSoundSystem->GetIReverbManager()->SetListenerReverb(REVERB_PRESET_OFF);
	}

	m_FrameProfileSystem.Done();

	if (m_sys_firstlaunch)
		m_sys_firstlaunch->Set( "0" );

	if (m_bEditor)
	{
		// restore the old saved cvars
		if(m_env.pConsole->GetCVar("r_Width"))
			m_env.pConsole->GetCVar("r_Width")->Set(m_iWidth);
		if(m_env.pConsole->GetCVar("r_Height"))
			m_env.pConsole->GetCVar("r_Height")->Set(m_iHeight);
		if(m_env.pConsole->GetCVar("r_ColorBits"))
			m_env.pConsole->GetCVar("r_ColorBits")->Set(m_iColorBits);
	}

	if (m_bEditor && !m_bRelaunch)
	{
		SaveConfiguration();
	}

	//if (!m_bEditor && !bRelaunch)
#if !defined(XENON) && !defined(PS3)
	if (!m_bEditor)
	{	 		
		if (m_pCVarQuit && m_pCVarQuit->GetIVal())		
		{
			SaveConfiguration();
			m_env.pNetwork->FastShutdown();

			//@TODO: release game.
			//SAFE_RELEASE(m_env.pGame);
			//FreeLib(m_dll.hGame);

			SAFE_RELEASE(m_env.pRenderer);
			FreeLib(m_dll.hRenderer);
	
			SAFE_RELEASE(m_env.pProfileLogSystem);
			SAFE_RELEASE(m_env.pLog);		// creates log backup

			if (m_env.pGame)
			m_env.pGame->Shutdown();
	#if defined(LINUX) || defined(XENON)
			return; //safe clean return
	#else
			// Commit files changes to the disk.
			_flushall();
			_exit(EXIT_SUCCESS);
	#endif
		}
	}
#endif//XENON && PS3

	//////////////////////////////////////////////////////////////////////////
	// Release Game.
	//////////////////////////////////////////////////////////////////////////	
	if (m_env.pEntitySystem)
	  m_env.pEntitySystem->Reset();
	//@TODO: Release game.
	//SAFE_RELEASE(m_env.pGame); 
	if (m_env.pPhysicalWorld)
	{
		m_env.pPhysicalWorld->SetPhysicsStreamer(0);
		m_env.pPhysicalWorld->SetPhysicsEventClient(0);
	}

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
	// release GPU physics
	if ( m_pGPUPhysics )
	{
		//pfnDeleteGPUPhysics( m_pGPUPhysics );
		delete m_pGPUPhysics;
		m_pGPUPhysics = NULL;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Clear 3D Engine resources.
	if (m_env.p3DEngine)
		m_env.p3DEngine->ClearRenderResources();
	//////////////////////////////////////////////////////////////////////////

	// Shutdown resource manager.
	m_pResourceManager->Shutdown();

	SAFE_RELEASE(m_env.pHardwareMouse);
	SAFE_RELEASE(m_env.pMovieSystem);
	SAFE_RELEASE(m_env.pAISystem);
	SAFE_RELEASE(m_env.pEntitySystem);
	SAFE_RELEASE(m_env.pCryFont);
	SAFE_RELEASE(m_env.pMusicSystem);
	SAFE_RELEASE(m_env.pNetwork);
//	SAFE_RELEASE(m_env.pCharacterManager);
	SAFE_DELETE(m_pStreamEngine);
	SAFE_RELEASE(m_env.p3DEngine);
	SAFE_RELEASE(m_env.pPhysicalWorld);
  if (m_env.pConsole)
    ((CXConsole*)m_env.pConsole)->FreeRenderResources();
	SAFE_RELEASE(m_pIBudgetingSystem);
	SAFE_RELEASE(m_env.pRenderer);
	SAFE_RELEASE(m_env.pSoundSystem);
	SAFE_RELEASE(m_pNULLSoundSystem);

	if(m_env.pLog)
		m_env.pLog->UnregisterConsoleVariables();

	// Release console variables.
	
	SAFE_RELEASE(m_pCVarQuit);
	SAFE_RELEASE(m_rWidth);
	SAFE_RELEASE(m_rHeight);
	SAFE_RELEASE(m_rColorBits);
	SAFE_RELEASE(m_rDepthBits);
	SAFE_RELEASE(m_cvSSInfo);
	SAFE_RELEASE(m_rStencilBits);
	SAFE_RELEASE(m_rFullscreen);
	SAFE_RELEASE(m_rDriver);

	SAFE_RELEASE(m_sysWarnings);
	SAFE_RELEASE(m_sys_profile);
	SAFE_RELEASE(m_sys_profile_additionalsub);
	SAFE_RELEASE(m_sys_profile_graph);
	SAFE_RELEASE(m_sys_profile_pagefaultsgraph);
	SAFE_RELEASE(m_sys_profile_graphScale);
	SAFE_RELEASE(m_sys_profile_filter);
	SAFE_RELEASE(m_sys_profile_filter_thread);
	SAFE_RELEASE(m_sys_profile_allThreads);
	SAFE_RELEASE(m_sys_profile_network);
	SAFE_RELEASE(m_sys_profile_peak);
	SAFE_RELEASE(m_sys_profile_peak_time);
	SAFE_RELEASE(m_sys_profile_memory);
	SAFE_RELEASE(m_sys_profile_sampler);
	SAFE_RELEASE(m_sys_profile_sampler_max_samples);
	SAFE_RELEASE(m_sys_mtrace);
  SAFE_RELEASE(m_sys_mtrace_print);
	SAFE_RELEASE(m_sys_spu_enable);
	SAFE_RELEASE(m_sys_spu_profile);
	SAFE_RELEASE(m_sys_spu_max);
	SAFE_RELEASE(m_sys_spu_flipmode);
	SAFE_RELEASE(m_sys_spu_debug);
	SAFE_RELEASE(m_sys_spu_filter);
	SAFE_RELEASE(m_sys_spu_dump_stats);
	SAFE_RELEASE(m_sys_spu_streaming);
	SAFE_RELEASE(m_sys_spec);
	SAFE_RELEASE(m_sys_firstlaunch);
	SAFE_RELEASE(m_sys_enable_budgetmonitoring);
	SAFE_RELEASE(m_sys_SaveCVars);
	SAFE_RELEASE(m_sys_StreamCallbackTimeBudget);
	SAFE_RELEASE(m_sys_physics_CPU);
	SAFE_RELEASE(m_sys_min_step);
	SAFE_RELEASE(m_sys_max_step);

	if (m_env.pInput)
	{
		m_env.pInput->ShutDown();
		m_env.pInput = NULL;
	}

	SAFE_RELEASE(m_env.pConsole);
	SAFE_RELEASE(m_env.pScriptSystem);

	SAFE_RELEASE(m_pNotificationNetwork);

	SAFE_DELETE(m_pMemStats);
	SAFE_DELETE(m_pSizer);
	SAFE_DELETE(m_env.pCryPak);
	SAFE_DELETE(m_pDefaultValidator);

	SAFE_DELETE(m_pPhysRenderer);

	//SAFE_DELETE(m_pCELogo); //Diesel cut

#ifdef DOWNLOAD_MANAGER
	SAFE_RELEASE(m_pDownloadManager);
#endif //DOWNLOAD_MANAGER

	SAFE_DELETE(m_pLocalizationManager);

	//DebugStats(false, false);//true);
	//CryLogAlways("");
	//CryLogAlways("release mode memory manager stats:");
	//DumpMMStats(true);
	
	SAFE_DELETE(m_pCpu);

	delete m_pCmdLine;
	m_pCmdLine = 0;







	// Log must be last thing released.
	SAFE_RELEASE(m_env.pProfileLogSystem);
	SAFE_RELEASE(m_env.pLog);		// creates log backup
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
void CSystem::Quit()
{
	m_bQuit=true;

	if(gEnv->pCryPak->GetLvlResStatus())
		LvlRes_export(0);			// executable was started with -LvlRes so it should export lvlres file on quit

#ifdef WIN32
	// Fast Quit.

	if ((m_pCVarQuit && m_pCVarQuit->GetIVal() != 0) || m_bTestMode)
	{

		if (m_env.pNetwork)
			m_env.pNetwork->FastShutdown();

		// Dispatch quit event so other systems can do any last minute processing
		if (m_pSystemEventDispatcher)
			m_pSystemEventDispatcher->OnSystemEvent(ESYSTEM_EVENT_SHUTDOWN, 0, 0);

		// workaround! to save cvars on quit.
		SaveConfiguration();

		if(m_env.pGame)
			m_env.pGame->Shutdown();

		if(GetIRenderer())
		{
			GetIRenderer()->RestoreGamma();
			GetIRenderer()->ShutDownFast();
		}


		// Dispatch quit event so other systems can do any last minute processing
		if (m_pSystemEventDispatcher)
			m_pSystemEventDispatcher->OnSystemEvent(ESYSTEM_EVENT_SHUTDOWN, 0, 0);

		CryLogAlways( "System:Quit" );
		// Commit files changes to the disk.
		_flushall();

		//////////////////////////////////////////////////////////////////////////
		// Support relaunching for windows media center edition.
		//////////////////////////////////////////////////////////////////////////
#if defined(WIN32) && !defined(XENON)
		if (m_pCmdLine && strstr(m_pCmdLine->GetCommandLine(),"ReLaunchMediaCenter") != 0)
		{
			ReLaunchMediaCenter();
		}
#endif
		//////////////////////////////////////////////////////////////////////////
		// [marco] in test mode, kill the process and quit without performing full C libs cleanup
		// (for faster closing of application)
		_exit(0);
	}

	PostQuitMessage(0);
#endif
}
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
bool CSystem::IsQuitting()
{
	return (m_bQuit);
}
 
//////////////////////////////////////////////////////////////////////////
void CSystem::SetIProcess(IProcess *process)
{
	m_pProcess = process; 
	//if (m_pProcess)
		//m_pProcess->SetPMessage("");
}

//////////////////////////////////////////////////////////////////////////
// Physics thread task
//////////////////////////////////////////////////////////////////////////
class CPhysicsThreadTask : public IThreadTask
{
public:
	CPhysicsThreadTask() { m_bStopRequested=0; m_bIsActive=0; m_stepRequested=0; m_bProcessing=0; m_doZeroStep=0; }

	//////////////////////////////////////////////////////////////////////////
	// IThreadTask implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void OnUpdate()
	{
		Run();
		// At the end.. delete the task
		delete this;
	}
	virtual void Stop()
	{
		Cancel();
	}
	virtual SThreadTaskInfo* GetTaskInfo() { return &m_TaskInfo; }
	//////////////////////////////////////////////////////////////////////////

	virtual void Run()
	{
		m_bStopRequested = 0;
		m_bIsActive = 1;

		float step,timeTaken,kSlowdown=1.0f;
		int nSlowFrames=0;
		CTimeValue timeStart;
		MarkThisThreadForDebugging("Physics");

		while(true)
		{
			m_FrameEvent.Wait(); // Wait untill new frame

			if (m_bStopRequested)
			{
				UnmarkThisThreadFromDebugging();
				return;
			}	

///* //Diesel cut
			while ((step=m_stepRequested)>0 || m_doZeroStep)
			{

				m_stepRequested=0; m_bProcessing=1; m_doZeroStep=0;
				PhysicsVars *pVars = gEnv->pPhysicalWorld->GetPhysVars();
				pVars->bMultithreaded = 1;
				gEnv->pPhysicalWorld->TracePendingRays();
				
				if (kSlowdown!=1.0f) 
				{
					step = max(1,FtoI(step*kSlowdown*50-0.5f))*0.02f;
					pVars->timeScalePlayers = 1.0f/max(kSlowdown,0.2f);
				}	else
					pVars->timeScalePlayers = 1.0f;
				step = min(step, pVars->maxWorldStep);
				timeStart = gEnv->pTimer->GetAsyncTime();
				gEnv->pPhysicalWorld->TimeStep(step);
 //Diesel cut	
				timeTaken = (gEnv->pTimer->GetAsyncTime()-timeStart).GetSeconds();
				if (timeTaken>step*0.9f)
				{
					if (++nSlowFrames>5)
						kSlowdown = step*0.9f/timeTaken;
				} else 
 //Diesel cut
					kSlowdown=1.0f, nSlowFrames=0; //Diesel cut

			gEnv->pPhysicalWorld->TracePendingRays(2); //Diesel cut
				
			//m_bProcessing=0; //Diesel cut


				//int timeSleep = (int)((m_timeTarget-gEnv->pTimer->GetAsyncTime()).GetMilliSeconds()*0.9f);
				//Sleep(max(0,timeSleep));

				break; //Diesel new
			}
//*/ //Diesel cut
		}
	}
	virtual void Cancel()
	{
		Pause();
		m_bStopRequested = 1;
		m_FrameEvent.Set();
		Resume();
		m_bIsActive = 0;
	}

	int Pause()
	{
		if (m_bIsActive) 
		{
			gEnv->pPhysicalWorld->GetPhysVars()->lastTimeStep = 0;
			m_bIsActive = 0;
			return 1;
		}
		gEnv->pPhysicalWorld->GetPhysVars()->lastTimeStep = 0;
		return 0;
	}
	int Resume()
	{
		if (!m_bIsActive)
		{
			m_bIsActive = 1;
			return 1;
		}
		return 0;
	}
	int IsActive() { return m_bIsActive; }
	int RequestStep(float dt)
	{
		if (m_bIsActive)
		{
			m_stepRequested += dt; 
			if (dt<=0.0f)
				m_doZeroStep = 1;
			m_FrameEvent.Set();
		}

		return m_bProcessing; 
	}
	float GetRequestedStep() { return m_stepRequested; }

protected:
	volatile int m_bStopRequested;
	volatile int m_bIsActive; 
	volatile float m_stepRequested;
	volatile int m_bProcessing;
	volatile int m_doZeroStep;

	CryEvent m_FrameEvent;

	SThreadTaskInfo m_TaskInfo;
};

void CSystem::CreatePhysicsThread()
{
	if (!m_PhysThread)/* && (unsigned int)m_sys_physics_CPU->GetIVal()<m_pCpu->GetPhysCPUCount()
#if defined (XENON)
*2
#endif
		)*/
	{
		//////////////////////////////////////////////////////////////////////////
		SThreadTaskParams threadParams;
		threadParams.name = "Physics";
		threadParams.nFlags = THREAD_TASK_BLOCKING;

//Diesel cut
		//threadParams.nStackSizeKB = 128;
//Diesel new
		threadParams.nStackSizeKB = 512;







		m_PhysThread = new CPhysicsThreadTask;
		GetIThreadTaskManager()->RegisterTask( m_PhysThread,threadParams );

		/*
		m_PhysThread.Start(m_pCpu->GetPhysCPUAffinityMask(m_sys_physics_CPU->GetIVal()));
		//Timur, temporary workaround for GDC 2008
		#ifdef XENON
		XSetThreadProcessor(m_PhysThread.GetHandle(),2); // Run on core 2 hw thread 0
		#endif
		*/
	}

	if (g_cvars.sys_limit_phys_thread_count) 
	{
		PhysicsVars *pVars = gEnv->pPhysicalWorld->GetPhysVars();
		pVars->numThreads = max(1, min(pVars->numThreads, (int)m_pCpu->GetPhysCPUCount()-1));
	}
}

void CSystem::KillPhysicsThread()
{
	if (m_PhysThread)
	{
		GetIThreadTaskManager()->UnregisterTask( m_PhysThread );
		m_PhysThread = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
int CSystem::SetThreadState(ESubsystem subsys, bool bActive)
{
	switch (subsys)
	{
	case ESubsys_Physics: 
		{
			if (m_PhysThread)
			{
				return bActive ? ((CPhysicsThreadTask*)m_PhysThread)->Resume() : ((CPhysicsThreadTask*)m_PhysThread)->Pause();
			}

		}
		break;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SleepIfInactive()
{
	// ProcessSleep()
	if (m_bDedicatedServer || m_bEditor || gEnv->bMultiplayer)
		return;
	
#if defined(WIN32) && !defined(XENON)
	WIN_HWND hRendWnd = GetIRenderer()->GetHWND();
	if(!hRendWnd)
		return;

	// Loop here waiting for window to be activated.
	for (int nLoops = 0; nLoops < 5; nLoops++)
	{
		WIN_HWND hActiveWnd = ::GetActiveWindow();
		if (hActiveWnd == hRendWnd)
			break;

		if (m_hWnd && ::IsWindow((HWND)m_hWnd))
		{
			// Peek message.
			MSG msg;
			while (PeekMessage(&msg, (HWND)m_hWnd, 0, 0, PM_REMOVE))       
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		Sleep(20);
	}
#endif      
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SleepIfNeeded()
{ 
	ITimer *const pTimer = gEnv->pTimer;
	static CTimeValue prevTime = 0.f;
	static bool firstCall = true;

	typedef MiniQueue<CTimeValue, 32> PrevNow;
	static PrevNow prevNow;
	if (firstCall)
	{
		prevTime = pTimer->GetAsyncTime();
		prevNow.Push(prevTime);
		firstCall = false;
		return;
	}

	const float maxRate = m_svDedicatedMaxRate->GetFVal();
	const float minTime = 1.0f / maxRate;
	CTimeValue now = pTimer->GetAsyncTime();
	float elapsed = (now - prevTime).GetSeconds();

	if (prevNow.Full())
		prevNow.Pop();
	prevNow.Push(now);

	static bool allowStallCatchup = true;
	if (elapsed > minTime && allowStallCatchup)
	{
		allowStallCatchup = false;
		prevTime = pTimer->GetAsyncTime();
		return;
	}
	allowStallCatchup = true;

	float totalElapsed = (now - prevNow.Front()).GetSeconds();
	float wantSleepTime = CLAMP(minTime*(prevNow.Size()-1) - totalElapsed, 0, (minTime - elapsed)*0.9f);
	static float sleepTime = 0;
	sleepTime = (15*sleepTime + wantSleepTime)/16;
	int sleepMS = (int)(1000.0f*sleepTime + 0.5f);
	if (sleepMS > 0)
		Sleep(sleepMS);

	prevTime = pTimer->GetAsyncTime();
}

//////////////////////////////////////////////////////////////////////
#ifdef WIN32
HWND g_hBreakWnd;
WNDPROC g_prevWndProc;
LRESULT CALLBACK BreakWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg==WM_HOTKEY)
#ifdef WIN64
		DebugBreak();
#else
		__asm int 3;
#endif
	return CallWindowProc(g_prevWndProc,hWnd,msg,wParam,lParam);
}

HANDLE g_hBreakHotkeyThread = 0;
DWORD WINAPI BreakHotkeyThreadProc(void*)
{
	g_hBreakWnd = CreateWindowEx(0,"Message","",0,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,HWND_MESSAGE,0,GetModuleHandle(0),0);
	g_prevWndProc = (WNDPROC)SetWindowLongPtr(g_hBreakWnd, GWLP_WNDPROC, (LONG_PTR)BreakWndProc);
	RegisterHotKey(g_hBreakWnd,0,0,VK_PAUSE);
	MSG msg;
	while(GetMessage(&msg,g_hBreakWnd,0,0)) 
		DispatchMessage(&msg);
	UnregisterHotKey(g_hBreakWnd,0);
	return 0;
}
#endif

class BreakListener : public IInputEventListener {
	bool OnInputEvent(const SInputEvent &ie) {
		if (ie.deviceId==eDI_Keyboard && ie.keyId==eKI_Pause && ie.state & (eIS_Pressed|eIS_Down))
			CryDebugBreak();
		return true;
	}
} g_BreakListener;

volatile int g_lockInput = 0;

struct SBreakListenerTask : public IThreadTask 
{
	SBreakListenerTask() { m_bStop=0; m_nBreakIdle=0; }
	virtual void OnUpdate() {
		do {
			Sleep(200);
			if (++m_nBreakIdle>1)
			{ WriteLock lock(g_lockInput);
				gEnv->pInput->Update(true);
				m_nBreakIdle = 0;
			}
		} while(!m_bStop);
	}
	virtual void Stop() { m_bStop=1; }
	virtual SThreadTaskInfo* GetTaskInfo() { return &m_TaskInfo; }
	volatile int m_bStop;
	int m_nBreakIdle;
	SThreadTaskInfo m_TaskInfo;
};
SBreakListenerTask g_BreakListenerTask;
bool g_breakListenerOn = false;

extern DWORD g_idDebugThreads[];
extern int g_nDebugThreads;
int prev_sys_float_exceptions = -1;

//////////////////////////////////////////////////////////////////////
bool CSystem::Update( int updateFlags, int nPauseMode )
{
	// do the dedicated sleep earlier than the frame profiler to avoid having it counted
	if (IsDedicated())
	{
		SleepIfNeeded();
	}

  if(CryGetIMemoryManager() && CryGetIMemReplay())
	  CryGetIMemReplay()->MemStatAddFrameStart();

	m_pPlatformOS->Tick(m_Time.GetRealFrameTime());







	if (g_cvars.sys_keyboard_break && !g_breakListenerOn)
	{
#ifdef WIN32
		if (m_bEditor)
			g_hBreakHotkeyThread = CreateThread(0,1024,BreakHotkeyThreadProc,0,0,0);
#endif
		SThreadTaskParams ttp;
		ttp.name = "BreakListenerThread";
		ttp.nFlags = THREAD_TASK_BLOCKING;
		g_BreakListenerTask.m_bStop = 0;
		GetIThreadTaskManager()->RegisterTask( &g_BreakListenerTask, ttp );
		gEnv->pInput->AddEventListener(&g_BreakListener);
		g_breakListenerOn = true;
	} else if (!g_cvars.sys_keyboard_break && g_breakListenerOn) 
	{
#ifdef WIN32
		if (g_hBreakHotkeyThread)
			TerminateThread(g_hBreakHotkeyThread,0);
#endif
		gEnv->pInput->RemoveEventListener(&g_BreakListener);
		GetIThreadTaskManager()->UnregisterTask(&g_BreakListenerTask);
		g_breakListenerOn = false;
	}
#ifdef WIN32
	// enable/disable SSE fp exceptions (#nan and /0)
	// need to do it each frame since sometimes they are being reset
	_mm_setcsr(_mm_getcsr() & ~0x280 | (g_cvars.sys_float_exceptions>0 ? 0 : 0x280));
#endif

	FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_SYSTEM,g_bProfilerEnabled );

	m_nUpdateCounter++;

	if(!m_sDelayedScreeenshot.empty())
	{
		gEnv->pRenderer->ScreenShot(m_sDelayedScreeenshot.c_str());
		m_sDelayedScreeenshot.clear();
	}

	if (m_sys_crashtest->GetIVal() != 0)
	{
		if (m_sys_crashtest->GetIVal() == 2)
		{
			float a = 1.0f;
			memset(&a, 0, sizeof(a));
			float * b = &a;
			float c = 3;
			CryLog( "%f",(c / *b) );
		}
		else
		{
			int *p = 0;
			*p = 0xABCD;

		}
	}

	// Check if game needs to be sleeping when not active.
	SleepIfInactive();

	if (m_pUserCallback)
		m_pUserCallback->OnUpdate();

	CTimeValue updateStart = gEnv->pTimer->GetAsyncTime();

	//////////////////////////////////////////////////////////////////////////
	// Enable/Disable floating exceptions.
	//////////////////////////////////////////////////////////////////////////
	prev_sys_float_exceptions += 1+g_cvars.sys_float_exceptions & prev_sys_float_exceptions>>31;
	if (prev_sys_float_exceptions != g_cvars.sys_float_exceptions)
	{
		prev_sys_float_exceptions = g_cvars.sys_float_exceptions;

		EnableFloatExceptions( g_cvars.sys_float_exceptions );
		UpdateFPExceptionsMaskForThreads();
	}
	//////////////////////////////////////////////////////////////////////////

	if (m_env.pLog)
		m_env.pLog->Update();

	if (updateFlags&ESYSUPDATE_EDITOR && gEnv->pLocalMemoryUsage != NULL)
	{
		//gEnv->pLocalMemoryUsage->OnUpdate();
	}

	if(!IsEditor())
	{
		// if aspect ratio changes or is different from default we need to update camera
		float fCurrentProjRatio = GetViewCamera().GetProjRatio();
		float fNewProjRatio = fCurrentProjRatio;

		uint32 dwWidth = m_rWidth->GetIVal();
		uint32 dwHeight = m_rHeight->GetIVal();
		
		if(dwHeight)
			fNewProjRatio = (float)dwWidth / (float)dwHeight;

		if(fNewProjRatio!=fCurrentProjRatio)
			GetViewCamera().SetFrustum(m_rWidth->GetIVal(),m_rHeight->GetIVal(),GetViewCamera().GetFov(),GetViewCamera().GetNearPlane(),GetViewCamera().GetFarPlane());
	}

	if(m_pTestSystem)
		m_pTestSystem->Update();

	if (nPauseMode != 0)
		m_bPaused = true;
	else
		m_bPaused = false;

	if (m_env.pGame)
	{
		//bool bDevMode = m_env.pGame->GetModuleState( EGameDevMode );
		//if (bDevMode != m_bInDevMode)
			//SetDevMode(bDevMode);
	}
#ifdef PROFILE_WITH_VTUNE
	if (m_bInDevMode)
	{	
		if (VTPause != NULL && VTResume != NULL)
		{
			static bool bVtunePaused = true;
			
			bool bPaused = false;
			
			if (GetISystem()->GetIInput())
			{
				bPaused = !(GetKeyState(VK_SCROLL) & 1);
			}

			{
				if (bVtunePaused && !bPaused)
				{
					GetIProfilingSystem()->VTuneResume();
				}
				if (!bVtunePaused && bPaused)
				{
					GetIProfilingSystem()->VTunePause();
				}
				bVtunePaused = bPaused;
			}
		}
	}
#endif //PROFILE_WITH_VTUNE

#ifndef LINUX
	if (m_pStreamEngine)
	{
		FRAME_PROFILER( "IStreamEngine::Update()", this, PROFILE_SYSTEM );
		m_pStreamEngine->Update(0);
		m_pStreamEngine->SetCallbackTimeQuota( m_sys_StreamCallbackTimeBudget->GetIVal() );
	}
#endif	

	if (m_bIgnoreUpdates)
		return true;

	if (m_env.pCharacterManager)
		m_env.pCharacterManager->Update();

  //static bool sbPause = false; 
	//bool bPause = false;

	//check what is the current process 
	IProcess *pProcess=GetIProcess();
	if (!pProcess)
		return (true); //should never happen

	bool bNoUpdate = false;
	if (m_sysNoUpdate && m_sysNoUpdate->GetIVal())
	{
		bNoUpdate = true;
		updateFlags = ESYSUPDATE_IGNORE_AI | ESYSUPDATE_IGNORE_PHYSICS;
	}

	//if ((pProcess->GetFlags() & PROC_MENU) || (m_sysNoUpdate && m_sysNoUpdate->GetIVal()))
	//		bPause = true;

	//check if we are quitting from the game
	if (IsQuitting())
		return (false);
	

#ifndef _XBOX
#ifdef WIN32
  // process window messages
	{
		FRAME_PROFILER( "SysUpdate:PeekMessage",this,PROFILE_SYSTEM );

		if (m_hWnd && ::IsWindow((HWND)m_hWnd))
		{
			MSG msg;
			while (PeekMessage(&msg, (HWND)m_hWnd, 0, 0, PM_REMOVE))       
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
  }
#endif
#endif

	//////////////////////////////////////////////////////////////////////
	//update time subsystem	
	m_Time.UpdateOnFrameStart();

	if (m_env.p3DEngine)
		m_env.p3DEngine->OnFrameStart();

	//////////////////////////////////////////////////////////////////////
	// update rate limiter for dedicated server
	if (m_pServerThrottle.get())
		m_pServerThrottle->Update();

	static ICVar *pFixedTimeStep = gEnv->pConsole->GetCVar("t_FixedStep");
	float fMovieFrameTime;

	// The reason that we use this equation instead of the simpler 'GetFrameTime()'
	// is that we want the movie system affected only by the fixed time step & time scale settings.
	// (For instance, not affected by the game pause, etc.)
	if (pFixedTimeStep->GetFVal() == 0)
	{
		fMovieFrameTime = m_Time.GetRealFrameTime() * m_Time.GetTimeScale();
	}
	else
	{
		fMovieFrameTime = pFixedTimeStep->GetFVal();
	}

	//////////////////////////////////////////////////////////////////////
	// initial network update
	if (m_env.pNetwork)
	{
		m_env.pNetwork->SyncWithGame(eNGS_FrameStart);
	}

	//////////////////////////////////////////////////////////////////////////
	// Update script system.
	if (m_env.pScriptSystem)
	{
		FRAME_PROFILER( "SysUpdate:ScriptSystem",this,PROFILE_SYSTEM );
		m_env.pScriptSystem->Update();
	}
	
	if (m_env.pInput)
	{
		if (!(updateFlags&ESYSUPDATE_EDITOR))
		{
			//////////////////////////////////////////////////////////////////////
			//update input system
#ifndef WIN32
			m_env.pInput->Update(true);
#else
			bool bFocus = (GetFocus()==m_hWnd) || m_bEditor;
			{ WriteLock lock(g_lockInput);
				m_env.pInput->Update(bFocus);
				g_BreakListenerTask.m_nBreakIdle = 0;
			}
#endif
		}
	}

	//////////////////////////////////////////////////////////////////////
	//update console system
	if (m_env.pConsole)
	{
		FRAME_PROFILER( "SysUpdate:Console",this,PROFILE_SYSTEM );

		if (!(updateFlags&ESYSUPDATE_EDITOR))
			m_env.pConsole->Update();
	}

	//////////////////////////////////////////////////////////////////////
	//update notification network system
	if (m_pNotificationNetwork)
	{
		m_pNotificationNetwork->Update();
	}

	//////////////////////////////////////////////////////////////////////
	//update sound system Part 1 if in Editor / in Game Mode Viewsystem updates the Listeners
	if (updateFlags & ESYSUPDATE_EDITOR && !bNoUpdate && !m_env.bEditorGameMode)
	{
		// updating the Listener Position in a first separate step
		if ((nPauseMode!=1))
		{
			m_env.pSoundSystem->SetListener(LISTENERID_STANDARD, m_ViewCamera.GetMatrix(), Vec3(0,0,0), true, 1.0);
			m_env.pSoundSystem->Update(eSoundUpdateMode_Listeners);
		}
	}


	//////////////////////////////////////////////////////////////////////	
	// update entity system (a little bit) before physics
	if(nPauseMode!=1)
	{
		//////////////////////////////////////////////////////////////////////
		//update entity system	
		if (m_env.pEntitySystem && !bNoUpdate && g_cvars.sys_entitysystem)
			m_env.pEntitySystem->PrePhysicsUpdate();
	}

	//////////////////////////////////////////////////////////////////////////
	// Update Threads Task Manager.
	//////////////////////////////////////////////////////////////////////////
	m_pThreadTaskManager->OnUpdate();

	//////////////////////////////////////////////////////////////////////////
	// Update Resource Manager.
	//////////////////////////////////////////////////////////////////////////
	m_pResourceManager->Update();

	//////////////////////////////////////////////////////////////////////	
	// update physic system	
	//static float time_zero = 0;
	if(!m_bUIFrameworkMode)
	{

#if !defined(CRYSIS_BETA)
	if (m_sys_physics_CPU->GetIVal()>0 && !IsDedicated())
		CreatePhysicsThread();
	else
#endif
		KillPhysicsThread();

	static int g_iPausedPhys = 0;
	PhysicsVars *pVars = m_env.pPhysicalWorld->GetPhysVars();
	pVars->threadLag = 0;

	CPhysicsThreadTask* pPhysicsThreadTask = ((CPhysicsThreadTask*)m_PhysThread);
	if (!pPhysicsThreadTask)
	{
		FRAME_PROFILER( "SysUpdate:AllAIAndPhysics",this,PROFILE_SYSTEM );
		// intermingle physics/AI updates so that if we get a big timestep (frame rate glitch etc) the
		// AI gets to steer entities before they travel over cliffs etc.
		float maxTimeStep = 0.0f;
		if (m_env.pAISystem)
			maxTimeStep = m_env.pAISystem->GetUpdateInterval();
		else
			maxTimeStep = 0.25f;
		int maxSteps = 1;
		float fCurTime = m_Time.GetCurrTime();
		float fPrevTime = m_env.pPhysicalWorld->GetPhysicsTime();
		float timeToDo = m_Time.GetFrameTime();//fCurTime - fPrevTime;
		if (m_env.bMultiplayer)
			timeToDo = m_Time.GetRealFrameTime();
		m_env.pPhysicalWorld->TracePendingRays();
		while (timeToDo > 0.0001f && maxSteps-- > 0)
		{
			float thisStep = min(maxTimeStep, timeToDo);
			timeToDo -= thisStep;

			if ((nPauseMode!=1) && !(updateFlags&ESYSUPDATE_IGNORE_PHYSICS) && g_cvars.sys_physics && !bNoUpdate)
			{
				FRAME_PROFILER( "SysUpdate:physics",this,PROFILE_SYSTEM );

				int iPrevTime=m_env.pPhysicalWorld->GetiPhysicsTime();
				//float fPrevTime=m_env.pPhysicalWorld->GetPhysicsTime();
				pVars->bMultithreaded = 0;
				pVars->timeScalePlayers = 1.0f;
				if (!(updateFlags&ESYSUPDATE_MULTIPLAYER))
					m_env.pPhysicalWorld->TimeStep(thisStep);
				else
				{
					//@TODO: fixed step in game.
					/*
					if (m_env.pGame->UseFixedStep())
					{
						m_env.pPhysicalWorld->TimeStep(fCurTime-fPrevTime, 0);
						int iCurTime = m_env.pPhysicalWorld->GetiPhysicsTime();

						m_env.pPhysicalWorld->SetiPhysicsTime(m_env.pGame->SnapTime(iPrevTime));
						int i, iStep=m_env.pGame->GetiFixedStep();
						float fFixedStep = m_env.pGame->GetFixedStep();
						for(i=min(20*iStep,m_env.pGame->SnapTime(iCurTime)-m_pGame->SnapTime(iPrevTime)); i>0; i-=iStep)
						{
							m_env.pGame->ExecuteScheduledEvents();
							m_env.pPhysicalWorld->TimeStep(fFixedStep, ent_rigid|ent_skip_flagged);
						}

						m_env.pPhysicalWorld->SetiPhysicsTime(iPrevTime);
						m_env.pPhysicalWorld->TimeStep(fCurTime-fPrevTime, ent_rigid|ent_flagged_only);

						m_env.pPhysicalWorld->SetiPhysicsTime(iPrevTime);
						m_env.pPhysicalWorld->TimeStep(fCurTime-fPrevTime, ent_living|ent_independent|ent_deleted);
					}
					else
					*/
						m_env.pPhysicalWorld->TimeStep(thisStep);

				}
				g_iPausedPhys = 0;
			}	else if (!(g_iPausedPhys++ & 31))
				m_env.pPhysicalWorld->TimeStep(0);	// make sure objects get all notifications; flush deleted ents

			{ FRAME_PROFILER( "SysUpdate:PumpLoggedEvents",this,PROFILE_SYSTEM );

//Diesel cut			
				m_env.pPhysicalWorld->PumpLoggedEvents();

			

			}

			// now AI
			if ((nPauseMode==0) && !(updateFlags&ESYSUPDATE_IGNORE_AI) && g_cvars.sys_ai && !bNoUpdate)
			{
				FRAME_PROFILER( "SysUpdate:AI",this,PROFILE_SYSTEM );
				//////////////////////////////////////////////////////////////////////
				//update AI system - match physics
				if (m_env.pAISystem && !m_cvAIUpdate->GetIVal() && g_cvars.sys_ai)
					m_env.pAISystem->Update(gEnv->pTimer->GetFrameStartTime(), gEnv->pTimer->GetFrameTime());
			}
		}

		// Make sure we don't lag too far behind
		if ((nPauseMode!=1) && !(updateFlags&ESYSUPDATE_IGNORE_PHYSICS))
		{
			if (fabsf(m_env.pPhysicalWorld->GetPhysicsTime()-fCurTime)>0.01f)
			{
				//GetILog()->LogToConsole("Adjusting physical world clock by %.5f", fCurTime-m_env.pPhysicalWorld->GetPhysicsTime());
				m_env.pPhysicalWorld->SetPhysicsTime(fCurTime);
			}
		}
	}
	else
	{
		{ FRAME_PROFILER( "SysUpdate:PumpLoggedEvents",this,PROFILE_SYSTEM );
			m_env.pPhysicalWorld->PumpLoggedEvents();
		}

		if ((nPauseMode!=1) && !(updateFlags&ESYSUPDATE_IGNORE_PHYSICS))
		{
			pPhysicsThreadTask->Resume();
			float lag = pPhysicsThreadTask->GetRequestedStep();
			if (pPhysicsThreadTask->RequestStep(m_Time.GetFrameTime()))
			{
				pVars->threadLag = lag+m_Time.GetFrameTime();
				//GetILog()->Log("Physics thread lags behind; accum time %.3f", pVars->threadLag);
			}
		} else
		{
			pPhysicsThreadTask->Pause();
			m_env.pPhysicalWorld->TracePendingRays();
			m_env.pPhysicalWorld->TimeStep(0);
		}

		if ((nPauseMode==0) && !(updateFlags&ESYSUPDATE_IGNORE_AI) && g_cvars.sys_ai && !bNoUpdate)
		{
			FRAME_PROFILER( "SysUpdate:AI",this,PROFILE_SYSTEM );
			//////////////////////////////////////////////////////////////////////
			//update AI system
			if (m_env.pAISystem && !m_cvAIUpdate->GetIVal())
				m_env.pAISystem->Update(gEnv->pTimer->GetFrameStartTime(), gEnv->pTimer->GetFrameTime());
		}
	}
	pe_params_waterman pwm;
	pwm.posViewer = GetViewCamera().GetPosition();
	m_env.pPhysicalWorld->SetWaterManagerParams(&pwm);
	}
	//////////////////////////////////////////////////////////////////////////
	// fix to solve a flaw in the system
	// Update movie system (before entity update is better for having artifact free movie playback)
	//////////////////////////////////////////////////////////////////////////	
	if(m_cvEarlyMovieUpdate->GetIVal()!=0)
	{
		if(!bNoUpdate)
			UpdateMovieSystem(updateFlags,fMovieFrameTime);
	}

	if(nPauseMode!=1)
	{
		//////////////////////////////////////////////////////////////////////
		// reset volumetric fog modifiers before (fog) entities get updated
		m_env.p3DEngine->SetVolumetricFogModifiers(0, 0, true);

		//////////////////////////////////////////////////////////////////////
		//update entity system	
		if (m_env.pEntitySystem && !bNoUpdate && g_cvars.sys_entitysystem)
			m_env.pEntitySystem->Update();
	}

	//////////////////////////////////////////////////////////////////////////
	// Update movie system (Must be after updating EntitySystem and AI.
	//////////////////////////////////////////////////////////////////////////	
	// the movie system already disables AI physics etc.
	if(m_cvEarlyMovieUpdate->GetIVal()==0)
	{
		if(!bNoUpdate)
			UpdateMovieSystem(updateFlags,fMovieFrameTime);
	}

	//////////////////////////////////////////////////////////////////////
	//update process (3D engine)
	if (!(updateFlags&ESYSUPDATE_EDITOR) && !bNoUpdate)
	{
		if (ITimeOfDay * pTOD = m_env.p3DEngine->GetTimeOfDay())
			pTOD->Tick();

		if (m_pProcess && (m_pProcess->GetFlags() & PROC_3DENGINE))
		{
			if ((nPauseMode!=1))
			if (!IsEquivalent(m_ViewCamera.GetPosition(),Vec3(0,0,0),VEC_EPSILON))
			{			
				if (m_env.p3DEngine)
				{
//					m_env.p3DEngine->SetCamera(m_ViewCamera);
					m_pProcess->Update();

					//////////////////////////////////////////////////////////////////////////
					// Strange, !do not remove... ask Timur for the meaning of this.
					//////////////////////////////////////////////////////////////////////////
					if (m_nStrangeRatio > 32767)
					{
						gEnv->pScriptSystem->SetGCFrequency(-1); // lets get nasty.
					}
					//////////////////////////////////////////////////////////////////////////
					// Strange, !do not remove... ask Timur for the meaning of this.
					//////////////////////////////////////////////////////////////////////////
					if (m_nStrangeRatio > 1000)
					{
						if (m_pProcess && (m_pProcess->GetFlags() & PROC_3DENGINE))
							m_nStrangeRatio += (1 + (10*rand())/RAND_MAX);
					}
					//////////////////////////////////////////////////////////////////////////
				}
			}
		}
		else
		{
			if (m_pProcess)
				m_pProcess->Update();
		}
	}

	//////////////////////////////////////////////////////////////////////
	//update sound system part 2
  if (m_env.pSoundSystem && !bNoUpdate)
	{
		FRAME_PROFILER( "SysUpdate:Sound",this,PROFILE_SYSTEM );

		// Listener position was already updated above, now we just call the update function
		ESoundUpdateMode UpdateMode = (ESoundUpdateMode)(eSoundUpdateMode_All & ~eSoundUpdateMode_Listeners);
    m_env.pSoundSystem->Update(UpdateMode);
	}

	if (m_env.pMusicSystem && !bNoUpdate)
	{
		FRAME_PROFILER( "SysUpdate:Music",this,PROFILE_SYSTEM );

		m_env.pMusicSystem->Update();
	}

	//////////////////////////////////////////////////////////////////////
	// final network update
	if (m_env.pNetwork)
	{
		m_env.pNetwork->SyncWithGame(eNGS_FrameEnd);
	}

#ifdef DOWNLOAD_MANAGER
	if (m_pDownloadManager && !bNoUpdate)
	{
		m_pDownloadManager->Update();
	}
#endif

	if (m_sys_SimulateTask->GetIVal() > 0) {
#if !defined(PS3) && !defined(LINUX)
		// have a chance to win longest Pi calculation content
		int64 delay = m_sys_SimulateTask->GetIVal();
		int64 start = CryQueryPerformanceCounter();
		double a = 1.0, b = 1.0/sqrt(2.0), t = 1.0/4.0, p = 1.0, an, bn, tn, pn, Pi = 0.0;
		while (CryQueryPerformanceCounter() - start < delay) {
			// do something
			an = (a + b) / 2.0;
			bn = sqrt(a*b);
			tn = t - p*(a - an)*(a - an);
			pn = 2 * p;

			a = an;
			b = bn;
			t = tn;
			p = pn;

			Pi = (a+b)*(a+b)/4/t;
		}
		//CryLog("Task calculate PI = %f ", Pi); // Thats funny , but it works :-)
#endif
	}

// If MTracing is enabled on ps3, create a memory snapshot and flush 
// the buffer contents 















	//Now update frame statistics

	CTimeValue cur_time = gEnv->pTimer->GetAsyncTime();
	
	CTimeValue a_second(g_cvars.sys_update_profile_time);
	std::vector< std::pair<CTimeValue, float> >::iterator it = m_updateTimes.begin();
	for(std::vector< std::pair<CTimeValue, float> >::iterator eit = m_updateTimes.end(); it!=eit; ++it)
		if((cur_time - it->first) < a_second)
			break;
	
	if(it != m_updateTimes.begin())
		m_updateTimes.erase(m_updateTimes.begin(),it);
	
	float updateTime = (cur_time - updateStart).GetMilliSeconds();
	m_updateTimes.push_back(std::make_pair(cur_time, updateTime));
	
	return !m_bQuit;	
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetUpdateStats( SSystemUpdateStats& stats )
{
	if(m_updateTimes.empty())
	{
		stats = SSystemUpdateStats();
	}
	else
	{
		stats.avgUpdateTime = 0;
		stats.maxUpdateTime = -FLT_MAX;
		stats.minUpdateTime = +FLT_MAX;
		for(std::vector< std::pair<CTimeValue, float> >::const_iterator it = m_updateTimes.begin(), eit = m_updateTimes.end(); it!=eit; ++it)
		{
			const float t = it->second;
			stats.avgUpdateTime += t;
			stats.maxUpdateTime = max(stats.maxUpdateTime, t);
			stats.minUpdateTime = min(stats.minUpdateTime, t);
		}
		stats.avgUpdateTime /= m_updateTimes.size();
	}
}


//////////////////////////////////////////////////////////////////////////
void CSystem::UpdateMovieSystem( const int updateFlags, const float fFrameTime )
{
	if (m_env.pMovieSystem && !(updateFlags&ESYSUPDATE_EDITOR) && g_cvars.sys_trackview)
	{
		float fMovieFrameTime = fFrameTime;
		
		if (fMovieFrameTime > 0.1f) // Slow frame rate fix.
			fMovieFrameTime = 0.1f;

		m_env.pMovieSystem->Update(fMovieFrameTime);
	}
}

//////////////////////////////////////////////////////////////////////////
// XML stuff
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSystem::CreateXmlNode( const char *sNodeName )
{
	return new CXmlNode( sNodeName );
}

//////////////////////////////////////////////////////////////////////////
IXmlUtils* CSystem::GetXmlUtils()
{
	return m_pXMLUtils;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSystem::LoadXmlFile( const char *sFilename )
{
	LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);
	return m_pXMLUtils->LoadXmlFile(sFilename);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSystem::LoadXmlFromString( const char *sXmlString )
{
	return m_pXMLUtils->LoadXmlFromString(sXmlString);
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::CheckLogVerbosity( int verbosity )
{
	if (verbosity <= m_env.pLog->GetVerbosityLevel())
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Warning( EValidatorModule module,EValidatorSeverity severity,int flags,const char *file,const char *format,... )
{
	va_list args;
	va_start(args,format);
	WarningV( module,severity,flags,file,format,args );
	va_end(args);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::WarningV( EValidatorModule module,EValidatorSeverity severity,int flags,const char *file,const char *format,va_list args )
{
	// Fran: No logging in a testing environment
	if (m_env.pLog == 0)
	{
		return;
	}

	IMiniLog::ELogType ltype = ILog::eComment;
	switch(severity)
	{
	case		VALIDATOR_ERROR		: ltype = ILog::eError;		break;
	case		VALIDATOR_WARNING	: ltype = ILog::eWarning; break;
	case		VALIDATOR_COMMENT	: ltype = ILog::eComment; break;
	default										:													break;
	}
	char szBuffer[MAX_WARNING_LENGTH];
	vsnprintf_s(szBuffer, sizeof(szBuffer), sizeof(szBuffer) - 1, format, args);

	if (file && *file)
	{
		CryFixedStringT<MAX_WARNING_LENGTH> fmt = szBuffer;
		fmt += " [File=";
		fmt += file;
		fmt += "]";

		m_env.pLog->LogWithType( ltype, "%s", fmt.c_str());
	}
	else
		m_env.pLog->LogWithType( ltype, "%s", szBuffer);

	//if(file)
		//m_env.pLog->LogWithType( ltype, "  ... caused by file '%s'",file);

	if (m_pValidator)
	{
		SValidatorRecord record;
		record.file = file;
		record.text = szBuffer;
		record.module = module;
		record.severity = severity;
		record.flags = flags;
		m_pValidator->Report( record );
	}
}



//////////////////////////////////////////////////////////////////////////
void CSystem::Deltree(const char *szFolder, bool bRecurse)
{
	__finddata64_t fd;
	string filespec = szFolder;
	filespec += "*.*";

	intptr_t hfil = 0;
	if ((hfil = _findfirst64(filespec.c_str(), &fd)) == -1)
	{
		return;
	}

	do
	{
		if (fd.attrib & _A_SUBDIR)
		{
			string name = fd.name;

			if ((name != ".") && (name != ".."))
			{
				if (bRecurse)
				{
					name = szFolder;
					name += fd.name;
					name += "/";

					Deltree(name.c_str(), bRecurse);
				}
			}
		}
		else
		{
			string name = szFolder;
			
			name += fd.name;

			DeleteFile(name.c_str());
		}

	} while(!_findnext64(hfil, &fd));

	_findclose(hfil);

	RemoveDirectory(szFolder);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OpenBasicPaks()
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Open Pak Files");

	//////////////////////////////////////////////////////////////////////////
	// Open old packs
	//////////////////////////////////////////////////////////////////////////
	// [Timur]
	// Open all *.pak files in FCData folder.
	m_env.pCryPak->OpenPacks( PathUtil::GetGameFolder()+"/","FCData/*.pak" ); 
	//////////////////////////////////////////////////////////////////////////

	// [Timur]
	// Open all *.pak files in GameCrysis folder. (for backward compatability to Crysis1 assets)
	m_env.pCryPak->OpenPacks( PathUtil::GetGameFolder()+"/","GameCrysis/*.pak" ); 

	//////////////////////////////////////////////////////////////////////////
	// Open paks from Engine folder
	//////////////////////////////////////////////////////////////////////////
	m_env.pCryPak->OpenPacks( PathUtil::GetGameFolder()+"/","Engine/*.pak" );


	ICVar *pCVar = m_env.pConsole != 0 ? m_env.pConsole->GetCVar( "g_language" ) : 0;
	//////////////////////////////////////////////////////////////////////////
	// load language pak

	if (!pCVar)
	{
		// if the language value cannot be found, let's default to the english pak
		OpenLanguagePak("english"); 
	}
	else
	{ 
		OpenLanguagePak( pCVar->GetString() );
	}

	string paksFolder = string(PathUtil::GetGameFolder())+"/*.pak";

	// Open some specific paks with name as CRC32 flag on console.
#if defined(XENON) || defined(PS3)
	m_env.pCryPak->OpenPack( PathUtil::GetGameFolder(),"textures.pak",ICryPak::FLAGS_FILENAMES_AS_CRC32 ); 
	m_env.pCryPak->OpenPack( PathUtil::GetGameFolder(),"objects.pak",ICryPak::FLAGS_FILENAMES_AS_CRC32 );
#endif

	// Open all *.pak files in root folder.
	//m_env.pCryPak->OpenPacks( "*.pak" );
	m_env.pCryPak->OpenPacks( paksFolder.c_str() ); 

	if (const ICmdLineArg *pModArg = GetICmdLine()->FindArg(eCLAT_Pre,"MOD"))
	{
		if (IsMODValid(pModArg->GetValue()))
		{		
			string paksModFolder = string("Mods\\") + string(pModArg->GetValue())+ string("\\") + PathUtil::GetGameFolder()+"\\*.pak";
			GetIPak()->OpenPacks("", paksModFolder.c_str());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OpenConfigPak( const char *pakName )
{	
	if (pakName)
	{
		// Initialize languages.
		int nPakFlags = 0; 

#if defined(XENON) || defined(PS3)
		//nPakFlags |= ICryPak::FLAGS_FILENAMES_AS_CRC32;
#endif

		// load localized pak.
		string sLocalizedPath;
		string bindRoot = PathUtil::GetGameFolder();
		if (!m_env.pCryPak->OpenPack( bindRoot, pakName, nPakFlags ))
		{
			// make sure the localized language is found - not really necessary, for TC		
			CryLogAlways("Could not open config pak file %s", pakName);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
void CSystem::OpenLanguagePak( const char *sLanguage )
{	
	// Initialize languages.

	int nPakFlags = 0;

#if defined(XENON) || defined(PS3)
	//nPakFlags |= ICryPak::FLAGS_FILENAMES_AS_CRC32;
#endif

	// load localized pak.
	string sLocalizedPath;
	string bindRoot = PathUtil::GetGameFolder();
	GetLocalizedPath(sLanguage, sLocalizedPath);
	if (!m_env.pCryPak->OpenPacks( bindRoot, sLocalizedPath,nPakFlags ))
	{
		// make sure the localized language is found - not really necessary, for TC		
		CryLogAlways("Localized language content(%s) not available or modified from the original installation.",sLanguage );
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetLocalizedPath( const char *sLanguage, string& sLocalizedPath )
{	
	string bindRoot = PathUtil::GetGameFolder();
	sLocalizedPath = bindRoot + "/Localized/" + sLanguage + ".pak";
}
//////////////////////////////////////////////////////////////////////////
void CSystem::CloseLanguagePak( const char *sLanguage )
{	
	string sLocalizedPath;
	GetLocalizedPath(sLanguage, sLocalizedPath);
	m_env.pCryPak->ClosePacks(sLocalizedPath);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Strange()
{
	m_nStrangeRatio += (1 + (100*rand())/RAND_MAX);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Relaunch(bool bRelaunch)
{
	if (m_sys_firstlaunch)
		m_sys_firstlaunch->Set( "0" );

	m_bRelaunch = bRelaunch;
	SaveConfiguration();
}

//////////////////////////////////////////////////////////////////////////
ICrySizer* CSystem::CreateSizer()
{
	return new CrySizerImpl;
}

//////////////////////////////////////////////////////////////////////////
uint32 CSystem::GetUsedMemory()
{
	return CryMemoryGetAllocatedSize();
}

//////////////////////////////////////////////////////////////////////////
ILocalizationManager* CSystem::GetLocalizationManager()
{
	return m_pLocalizationManager;
}

//////////////////////////////////////////////////////////////////////////
IThreadTaskManager* CSystem::GetIThreadTaskManager()
{
	return m_pThreadTaskManager;
}

//////////////////////////////////////////////////////////////////////////
IResourceManager* CSystem::GetIResourceManager()
{
	return m_pResourceManager;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ApplicationTest( const char *szParam )
{
	assert(szParam);

	if(!m_pTestSystem)
		m_pTestSystem = new CTestSystemLegacy;

	m_pTestSystem->ApplicationTest(szParam);
}

void CSystem::ExecuteCommandLine()
{
	// should only be called once
	{
#ifndef PS3
		static bool bCalledAlready = false;
		assert(!bCalledAlready);
		bCalledAlready=true;
#endif
	}

	// 
	if(gEnv->pCryPak->GetRecordFileOpenList()==ICryPak::RFOM_EngineStartup)
		gEnv->pCryPak->RecordFileOpen(ICryPak::RFOM_Level);
	
	// auto detect system spec (overrides profile settings)
	if (m_pCmdLine->FindArg(eCLAT_Pre,"autodetect"))
		AutoDetectSpec();

	// execute command line arguments e.g. +g_gametype ASSAULT +map "testy"

	ICmdLine *pCmdLine = GetICmdLine();			assert(pCmdLine);

	const int iCnt = pCmdLine->GetArgCount();

	for(int i=0;i<iCnt;++i)
	{
		const ICmdLineArg *pCmd=pCmdLine->GetArg(i);

		if(pCmd->GetType()==eCLAT_Post)
		{
			string sLine = pCmd->GetName();

			if(pCmd->GetValue())
				sLine += string(" ") + pCmd->GetValue();

			GetILog()->Log("Executing command from command line: \n%s\n",sLine.c_str());  // - the actual command might be executed much later (e.g. level load pause)
			GetIConsole()->ExecuteString(sLine.c_str(), false, true);
		}
	}

	gEnv->pConsole->ExecuteString("sys_RestoreSpec test*"); // to get useful debugging information about current spec settings to the log file
}

void CSystem::DumpMemoryCoverage()
{
	m_MemoryFragmentationProfiler.DumpMemoryCoverage();
}

ITextModeConsole* CSystem::GetITextModeConsole()
{
	if (m_bDedicatedServer)
		return g_pUnixConsole;

	return 0;
}

//////////////////////////////////////////////////////////////////////////
ESystemConfigSpec CSystem::GetConfigSpec( bool bClient )
{
	if (bClient)
	{
		if (m_sys_spec)
			return (ESystemConfigSpec)m_sys_spec->GetIVal();
		return CONFIG_VERYHIGH_SPEC; // highest spec.
	}
	else
		return m_nServerConfigSpec;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetConfigSpec( ESystemConfigSpec spec,bool bClient )
{
	if (bClient)
	{
		if (m_sys_spec)
			m_sys_spec->Set( (int)spec );
	}
	else
	{
		m_nServerConfigSpec = spec;
	}
}

//////////////////////////////////////////////////////////////////////////
ESystemConfigSpec CSystem::GetMaxConfigSpec() const
{ 
	return m_nMaxConfigSpec; 
}

//////////////////////////////////////////////////////////////////////////
Crc32Gen* CSystem::GetCrc32Gen()
{
	static Crc32Gen m_crcGen;
	return &m_crcGen;
}

//////////////////////////////////////////////////////////////////////////
CPNoise3* CSystem::GetNoiseGen()
{
	static CPNoise3 m_pNoiseGen;	
	return &m_pNoiseGen;
}

//////////////////////////////////////////////////////////////////////////
void CProfilingSystem::VTuneResume()
{
#ifdef PROFILE_WITH_VTUNE
	if (VTResume)
	{
		CryLogAlways("VTune Resume");
		VTResume();
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
void CProfilingSystem::VTunePause()
{
#ifdef PROFILE_WITH_VTUNE
	if (VTPause)
	{
		VTPause();
		CryLogAlways("VTune Pause");
	}
#endif
}
//////////////////////////////////////////////////////////////////////////
void CProfilingSystem::StartProfilingX360(const char * fileName)
{
#ifdef XENON
	string newName(fileName);
	PathUtil::RemoveExtension(newName);
	newName += ".pix2";
	XTraceStartRecording(newName);
#endif
}

//////////////////////////////////////////////////////////////////////////
void CProfilingSystem::StopProfilingX360()
{
#ifdef XENON
	XTraceStopRecording();
#endif
}

#ifdef USING_UNIKEY_SECURITY

IUniKeyManager *CSystem::GetUniKeyManager()
{
	if (m_pUniKeyManager==NULL)
		m_pUniKeyManager = (IUniKeyManager*)new CUniKeyManager();
	return m_pUniKeyManager;
}

#endif // USING_UNIKEY_SECURITY

//////////////////////////////////////////////////////////////////////////

#ifdef USING_LICENSE_PROTECTION

IProtectionManager* CSystem::GetProtectionManager()
{
	if (m_pProtectionManager==NULL)
		m_pProtectionManager = (IProtectionManager*)new CProtectionManager();
	return m_pProtectionManager;
}

void CSystem::EncryptBuffer(char* dstBuf, const char* srcBuf, size_t bufLen, const char* key, size_t keyLen)
{
	CAesCryptography cryptObj;
	if (keyLen > 0)
		cryptObj.SetKeyValue((const uint8*)key, keyLen);
	cryptObj.EncryptBuffer((const uint8*)srcBuf, bufLen, (uint8*)dstBuf);
}

void CSystem::DecryptBuffer(char* dstBuf, const char* srcBuf, size_t bufLen, const char* key, size_t keyLen)
{
	CAesCryptography cryptObj;
	if (keyLen > 0)
		cryptObj.SetKeyValue((const uint8*)key, keyLen);
	cryptObj.DecryptBuffer((const uint8*)srcBuf, bufLen, (uint8*)dstBuf);
}

const char* GetLicenseInfoFilename()
{
	// encryption for "/license.launcher_data"
	const size_t FilenameSize = 32;
	static char encryptedFilename[FilenameSize] = { 
		0x0f,	0x4d,	0x4b,	0x40,	0x41,	0x4b,	0x55,	0x42,	0x06, 0x45,	0x4b,	
		0x5e,	0x42,	0x4e,	0x46,	0x4a,	0x42,	0x6e,	0x56,	0x52,	0x40,	0x54,	0x00 };
	size_t bufLen = strlen(encryptedFilename);
	static char filename[FilenameSize] = {0,};
	for(size_t i=0; i<bufLen; ++i)
		filename[i] = encryptedFilename[i]^32+i;
	return filename;
}

const char* GetLicenseInfoString()
{
	// encryption for "wuRok9nm3zxgfawBk8NTsfAjkhsff889sTaY"
	const size_t FilenameSize = 40;
	static char encryptedFilename[FilenameSize] = { 
		0x57,0x54,0x70,0x4c,0x4f,0x1c,0x48,0x4a,0x1b,0x53,0x52,0x4c,0x4a,0x4c,0x59,0x6d,0x5b,0x09,0x7c,
		0x67,0x47,0x53,0x77,0x5d,0x53,0x51,0x49,0x5d,0x5a,0x05,0x06,0x06,0x33,0x15,0x23,0x1a,0x00 };
	size_t bufLen = strlen(encryptedFilename);
	static char filename[FilenameSize] = {0,};
	for(size_t i=0; i<bufLen; ++i)
		filename[i] = encryptedFilename[i]^32+i;
	return filename;
}

bool CSystem::IsEncryptedLevel(const char* path)
{
	bool encrypted = false;
	CCryFile licenseInfoFileHandle;
	string licenseInfoFile = path + string(GetLicenseInfoFilename());
	if (licenseInfoFileHandle.Open(licenseInfoFile, "rb" ))
		encrypted = true;
	licenseInfoFileHandle.Close();
	return encrypted;
}

size_t CSystem::GenerateLicenseBuffer(char* dstBuff, const char* key, size_t keyLen, const char* additionalInfo, size_t infoSize)
{
	const char* licenseInfoString = GetLicenseInfoString();
	size_t licenseInfoStringLen = strlen(licenseInfoString);
	EncryptBuffer(dstBuff, licenseInfoString, licenseInfoStringLen, key, keyLen);
	//memcpy(dstBuff, key, keyLen);
	EncryptBuffer(&dstBuff[licenseInfoStringLen], additionalInfo, infoSize, licenseInfoString, licenseInfoStringLen);
	//memcpy(&dstBuff[licenseInfoStringLen], additionalInfo, infoSize);
	return licenseInfoStringLen+infoSize;
}

bool CSystem::IsValidLicenseBuffer(const char* path, const char* key, size_t keyLen)
{
	CCryFile licenseInfoFileHandle;
	string licenseInfoFile = path + string(GetLicenseInfoFilename());
	if (false == licenseInfoFileHandle.Open(licenseInfoFile, "rb" ))
		return false;

	const char* licenseInfoString = GetLicenseInfoString();
	size_t licenseInfoStringLen = strlen(licenseInfoString);

	bool result = true;
	char* readBuf = new char[licenseInfoStringLen];
	memset(readBuf, 0, licenseInfoStringLen);
	char* decryptedBuf = new char[licenseInfoStringLen];
	memset(decryptedBuf, 0, licenseInfoStringLen);
	size_t readSize = licenseInfoFileHandle.ReadRaw(readBuf, licenseInfoStringLen);
	do
	{
		if (readSize != licenseInfoStringLen)
		{
			result = false;
			break;
		}
		DecryptBuffer(decryptedBuf, readBuf, licenseInfoStringLen, key, keyLen);
		if (0 != strncmp(decryptedBuf, licenseInfoString, readSize))
		{
			result = false;
			break;
		}
	} while(false);

	delete [] readBuf;
	readBuf = NULL;
	delete [] decryptedBuf;
	decryptedBuf = NULL;
	licenseInfoFileHandle.Close();
	return result;
}

size_t GetFileSize(const char* filename)
{
	CCryFile cfile;
	if (false == cfile.Open(filename, "rb" ))
		return 0;
	size_t size = cfile.GetLength();
	cfile.Close();
	return size;
}

bool ReadFile2Buffer(const char* filename, char* buffer, size_t length)
{
	CCryFile cfile;
	if (false == cfile.Open(filename, "rb" ))
		return false;
	size_t readSize = cfile.ReadRaw(buffer, length);
	cfile.Close();
	buffer[readSize] = 0;
	return true;
}

XmlNodeRef CSystem::LoadEncryptedXmlFile(const char* filename, bool decryptIt, const char* key)
{
	if (false == decryptIt)
		return LoadXmlFile(filename);

	size_t fileSize = GetFileSize(filename);
	char* fileBuffer = new char[fileSize+1];
	if (false == ReadFile2Buffer(filename, fileBuffer, fileSize))
	{
		delete [] fileBuffer;
		fileBuffer = NULL;
		return NULL;
	}

	char* buffer4Decryption = new char[fileSize+1];
	memset(buffer4Decryption, 0, fileSize+1);
	DecryptBuffer(buffer4Decryption, fileBuffer, fileSize, key, strlen(key));
	buffer4Decryption[fileSize] = 0;
	XmlNodeRef rootNode = GetISystem()->LoadXmlFromString(buffer4Decryption);
	delete [] fileBuffer;
	fileBuffer = NULL;
	delete [] buffer4Decryption;
	buffer4Decryption = NULL;
	return rootNode;
}

#endif // USING_LICENSE_PROTECTION

#include UNIQUE_VIRTUAL_WRAPPER(IProfilingSystem)
//////////////////////////////////////////////////////////////////////
void CSystem::OnLanguageCVarChanged(ICVar * language)
{
	if(language && language->GetType() == CVAR_STRING)
	{
		CSystem* pSys = static_cast<CSystem*>(gEnv->pSystem);
		if(pSys && pSys->GetLocalizationManager())
		{
			const char* lang = language->GetString();

			pSys->CloseLanguagePak(pSys->GetLocalizationManager()->GetLanguage());
			pSys->OpenLanguagePak(lang);
			pSys->GetLocalizationManager()->SetLanguage(lang);
			pSys->GetLocalizationManager()->ReloadData();
		}
	}
}


#include UNIQUE_VIRTUAL_WRAPPER(ISystem)
#include UNIQUE_VIRTUAL_WRAPPER(INameTable)


