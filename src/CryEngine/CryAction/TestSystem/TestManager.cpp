////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   testmanager.cpp
//  Version:     v1.00
//  Created:     3/3/2003 by Luciano Morpurgo.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TestManager.h"
#include <CryFile.h>
#include <IActorSystem.h>
#include <IAgent.h>
#include <ILevelSystem.h>
#include <ITestSystem.h>
#include <IMovieSystem.h>
#include "IMovementController.h"

#include "TimeDemoRecorder.h"
#include "TestProfiler.h"

#if defined(WIN32)
#include <windows.h>
#endif


#define FIXED_TIME_STEP (30) // Assume runing at 30fps.

//#define SAVE_FILE_NAME PathUtil::Make( GetCurrentLevelPath(),"autotest_init","CRYSISJMSF" )
// loop only enabled test modules
#define TEST_MODULE_LOOP(doThisStuff) \
{\
	TestModuleMap::iterator theit = s_pTestManager->m_TestModules.begin(), theitEnd = s_pTestManager->m_TestModules.end();\
	for(; theit!=theitEnd;++theit) \
	{\
		ITestModule* pTM = theit->second; \
		if(pTM->IsEnabled()) \
			pTM->doThisStuff;\
	}\
}
// loop all modules
#define TEST_MODULE_LOOP_ALL(doThisStuff) \
	{\
		TestModuleMap::iterator it = s_pTestManager->m_TestModules.begin(), itEnd = s_pTestManager->m_TestModules.end();\
		for(; it!=itEnd;++it) \
		{\
			it->second->doThisStuff;\
		}\
	}


//////////////////////////////////////////////////////////////////////////

CTestManager* CTestManager::s_pTestManager(0);

char CTestManager::s_fileName[MAX_FILE_NAME];

//////////////////////////////////////////////////////////////////////////

CTestManager::CTestManager( ISystem *pSystem )
{
	if(s_pTestManager)
		SAFE_DELETE(s_pTestManager);
	s_pTestManager = this;

	//gEnv->pConsole->AddConsoleVarSink(this);

	m_pSystem = pSystem;
	CRY_ASSERT(m_pSystem);

	m_TestModules.clear();

	m_bRecording = false;
	m_bPlaying = false;
	m_bDemoFinished = false;

	//m_lastAveFrameRate = 0;
	//m_lastPlayedTotalTime = 0;

	m_bAIEnabled = false;
	//m_fixedTimeStep = 0;
	m_maxLoops = 2;
	m_demo_scroll_pause = 1;
	m_demo_noinfo = 1;
	m_demo_max_frames = 100000;
	m_demo_savestats = 0;
	m_demoProfile = 0;
	m_nCurrentDemoLevel = 0;
	m_countDownPlay = 0;

	m_bPaused = false;

	for(int t=TM_NONE; t<TM_LAST; t++)
		SetTestModule((ETestModuleType)t, false); // enable only these as default configuration

	// fill up test module types container
//	m_TestModuleTypes.insert(std::make_pair(TM_NONE,"global"));
	//m_TestModuleTypes.insert(std::make_pair(TM_TIMEDEMO,"timedemo"));

	IConsole *pConsole = gEnv->pConsole;

	// Register demo variables.
	// marked with VF_CHEAT because it offers a MP cheat (impression to run faster on client)
	
	REGISTER_COMMAND( "record",&CTestManager::cmd_StartRecordingTimeDemo,0,
		"Starts recording of a time demo.\n"
		"Usage: record demoname\n"
		"File 'demoname.tmd' will be created.");

	REGISTER_COMMAND("stoprecording",&CTestManager::cmd_Stop,0,
		"Stops recording of a time demo.\n"
		"Usage: stoprecording\n"
		"File 'demoname.?' will be saved.");

	REGISTER_COMMAND("demo",&CTestManager::cmd_Play,0,
		"Plays a time demo from file.\n"
		"Usage: demo demoname\n");

	REGISTER_COMMAND("stopdemo",&CTestManager::cmd_Stop,0,
		"Stop playing a time demo.\n" );

	REGISTER_COMMAND("demo_StartDemoChain",&CTestManager::cmd_StartDemoChain,0,"Load`s a file at 1st argument with the list of levels and play time demo on each\n" );
	
	REGISTER_CVAR2( "demo_profile",&m_demoProfile, 1,0,"Enable demo profiling" );
	REGISTER_CVAR2( "demo_num_runs",&m_maxLoops,1,0,"Number of times to loop timedemo" );
	REGISTER_CVAR2( "demo_scroll_pause",&m_demo_scroll_pause,1,0,"ScrollLock pauses demo play/record" );
	REGISTER_CVAR2( "demo_noinfo",&m_demo_noinfo,0,0,"Disable info display during demo playback" );
	REGISTER_CVAR2( "demo_quit",&m_demo_quit,0,0,"Quit game after demo runs finished" );
	REGISTER_CVAR2( "demo_screenshot_frame",&m_demo_screenshot_frame,0,0,"Make screenshot on specified frame during demo playback, If Negative then do screen shoot every N frame" );
	REGISTER_CVAR2( "demo_max_frames",&m_demo_max_frames,100000,0,"Max number of frames to save" );
	REGISTER_CVAR2( "demo_savestats",&m_demo_savestats,0,0,"Save level stats at the end of the loop" );
	REGISTER_CVAR2( "demo_ai",&m_demo_ai,0,0,"Enable/Disable AI during the demo" );
	REGISTER_CVAR2( "demo_restart_level",&m_demo_restart_level,1,0,"Restart level after each loop: 0 = Off; 1 = use quicksave on first playback; 2 = load level start" );
	REGISTER_CVAR2( "demo_panoramic",&m_demo_panoramic,0,0,"Panoramic view when playing back demo" );
	REGISTER_CVAR2( "demo_fixed_timestep",&m_demo_fixed_timestep,FIXED_TIME_STEP,0,"number of updates per second" );
	REGISTER_CVAR2( "demo_vtune",&m_demo_vtune,0,0,"Enables VTune profiling when running time demo" );
	REGISTER_CVAR2( "demo_time_of_day",&m_demo_time_of_day,-1,0,"Sets the time of day to override in game settings if not negative" );
}

//////////////////////////////////////////////////////////////////////////
CTestManager::~CTestManager()
{

	ReleaseTestModules();

	//gEnv->pConsole->RemoveConsoleVarSink(this);

	IConsole* pConsole = gEnv->pConsole;

	pConsole->RemoveCommand("record");
	pConsole->RemoveCommand("stoprecording");
	pConsole->RemoveCommand("demo");
	pConsole->RemoveCommand("stopdemo");

	pConsole->UnregisterVariable( "demo_num_runs", true );
	pConsole->UnregisterVariable( "demo_scroll_pause", true );
	pConsole->UnregisterVariable( "demo_noinfo", true );
	pConsole->UnregisterVariable( "demo_quit", true );
	pConsole->UnregisterVariable( "demo_screenshot_frame", true );
	pConsole->UnregisterVariable( "demo_max_frames", true );
	pConsole->UnregisterVariable( "demo_savestats", true );
	pConsole->UnregisterVariable( "demo_ai", true );
	pConsole->UnregisterVariable( "demo_restart_level", true );
	pConsole->UnregisterVariable( "demo_panormaic", true );
	pConsole->UnregisterVariable( "demo_fixed_timestep", true );
	pConsole->UnregisterVariable( "demo_time_of_day", true );

	if(s_pTestManager == this)
		s_pTestManager = NULL;
}

//////////////////////////////////////////////////////////////////////////
const char* CTestManager::GetInitSaveFileName()
{
	sprintf(s_fileName,"autotest_init_%s.CRYSISJMSF",gEnv->pGame->GetIGameFramework()->GetLevelName());
	return s_fileName;
}

//////////////////////////////////////////////////////////////////////////
const char* CTestManager::GetCurrentLevelPath()
{
	static char str[256];

	const char* levelName = gEnv->pGame->GetIGameFramework()->GetLevelName();
	sprintf(str,"%s/levels/%s", PathUtil::GetGameFolder().c_str(), levelName? levelName:"");

	//CryLog("DEBUG Levelpath: %s",str);

	return str;
}

//////////////////////////////////////////////////////////////////////////
void CTestManager::Record( bool bEnable )
{
	if (bEnable == m_bRecording)
		return;

	m_bRecording = bEnable;
	m_bPlaying = false;
	m_lastFrameTime = GetTime();

	if (m_bRecording)
	{
		PreStartSession();
		StartSession();

		//m_recordStartTime = GetTime();
		//m_lastFrameTime = m_recordStartTime;

	}
	else
	{
		// Stop recording.
		//m_recordedDemoTime = m_totalDemoTime;

		StopSession();
	}


	TEST_MODULE_LOOP(Record(m_bRecording));

	m_currentFrame = 0;
	m_totalDemoTime.SetMilliSeconds(0);
}

//////////////////////////////////////////////////////////////////////////
void CTestManager::Play( bool bEnable )
{
	if (bEnable == m_bPlaying)
		return;


	m_bPlaying = bEnable;

	if (m_bPlaying)
	{
		LogInfo( "AutoTest Play started");
		LogInfo( "==============================================================" );
		//LogInfo( "TimeDemo Play Started , (Total Frames: %d, Recorded Time: %.2fs)",(int)m_records.size(),m_recordedDemoTime.GetSeconds() );

		m_bDemoFinished = false;

		// Start demo playback.
		//m_lastPlayedTotalTime = 0;
		PreStartSession();
		StartSession();
	}
	else
	{
		LogInfo( "AutoTest Play Ended, (%d Runs Performed)",m_numLoops );
		LogInfo( "==============================================================" );

		// End demo playback.
		//m_lastPlayedTotalTime = m_totalDemoTime.GetSeconds();
		StopSession();
	}
	m_bRecording = false;
	m_currentFrame = 0;
	m_totalDemoTime.SetMilliSeconds(0);
	m_lastFrameTime = GetTime();
	m_numLoops = 0;
	TEST_MODULE_LOOP(Play(m_bPlaying));

}


//////////////////////////////////////////////////////////////////////////
void CTestManager::PlayInit( IConsoleCmdArgs *pArgs)
{
	PreStartSession();

	TEST_MODULE_LOOP(PlayInit(pArgs));
//	Play(true);
	m_countDownPlay = 2;

}


//////////////////////////////////////////////////////////////////////////
void CTestManager::PreUpdate()
{
	if(m_countDownPlay)
	{
		// to avoid playing demo before game is initialized (when running autotest)
		m_countDownPlay--;
		if(m_countDownPlay==0)
			Play(true);
	}

	if (m_bPlaying && !m_bPaused)
	{
		if (!gEnv->pConsole->GetStatus()) // Only when console closed.
		{
			// Reset random number generators seed.
			GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_RANDOM_SEED,0,0 );

			//override game time of day
			if( 0 <= m_demo_time_of_day )
			{
				//force update if time is significantly different from current time
				float fTime = gEnv->p3DEngine->GetTimeOfDay()->GetTime();
				const float cfSmallDeltaTime = 0.1f;

				if( ( fTime >  m_demo_time_of_day + cfSmallDeltaTime ) ||
					( fTime <  m_demo_time_of_day - cfSmallDeltaTime ) )
				{
					gEnv->p3DEngine->GetTimeOfDay()->SetTime( (float)m_demo_time_of_day, true );
					SetConsoleVar( "e_TimeOfDaySpeed", 0 );
				}
			}


			TEST_MODULE_LOOP(PreUpdate());

			bool bPlaying = true;

			TestModuleMap::iterator it = s_pTestManager->m_TestModules.begin(), itEnd = s_pTestManager->m_TestModules.end();
			for(; it!=itEnd;++it) 
			{
				ITestModule* pTM = it->second; 
				if(pTM->IsEnabled()) 
					bPlaying &= pTM->PlayFrame();
			}

			if ((m_numLoops == 0) &&
				((m_demo_screenshot_frame && m_currentFrame == m_demo_screenshot_frame) ||
				(m_demo_screenshot_frame < 0) && (m_currentFrame % abs(m_demo_screenshot_frame) == 0) ))
			{
				gEnv->pRenderer->ScreenShot();
			}

//			if (m_demo_noinfo <= 0)
	//			RenderInfo();

			m_currentFrame++;

			CTimeValue deltaFrameTime = (GetTime() - m_lastFrameTime);
			float frameTime = deltaFrameTime.GetSeconds();
			m_totalDemoTime += frameTime;
			m_lastFrameTime = GetTime();

			if(!bPlaying) // one test module has stopped playing
			{
				// Stop playing if max runs reached.
				m_numLoops++;

				EndLog();

				m_totalDemoTime.SetMilliSeconds(0);
				if (m_numLoops >= m_maxLoops)
				{
					LogInfo( "Save AutoTestFinished.log" );
					Play(false);
					m_bDemoFinished = true;











				}
				else
				{ //next loop
					TEST_MODULE_LOOP(Restart());
					ResetSessionLoop();
				}
			}
		}
	}
	else 	if (m_bRecording && !m_bPaused)
	{
		TEST_MODULE_LOOP(PreUpdate());
		m_currentFrame++;
	}

}

//////////////////////////////////////////////////////////////////////////
void CTestManager::Update()
{






	if (!m_bPlaying && m_bDemoFinished && m_demo_quit != 0)
	{
		if (!m_demoLevels.empty())
		{
			StartNextChainedLevel();
			return;
		}



#if defined(WIN32)
		gEnv->pRenderer->RestoreGamma();
		//gEnv->pRenderer->ShutDownFast();
		GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_SHARE_SHADER_COMBINATIONS,0,0 );
		CryLog( "TerminateProcess" );
		// Quit after demo was finished.
		TerminateProcess( GetCurrentProcess(),0 );
#else




		_exit(0);

#endif
	}


#ifdef WIN32
	if (!gEnv->pSystem->IsDedicated() && gEnv->pSystem->IsDevMode())		
	{
		// Check if special development keys where pressed.
		bool bAlt = ((CryGetAsyncKeyState(VK_LMENU) & (1<<15)) != 0) || (CryGetAsyncKeyState(VK_RMENU) & (1<<15)) != 0;
		bool bCtrl = (CryGetAsyncKeyState(VK_CONTROL) & (1<<15)) != 0;
		bool bShift = (CryGetAsyncKeyState(VK_SHIFT) & (1<<15)) != 0;

		bool bCancel = CryGetAsyncKeyState(VK_CANCEL) & 1;
		bool bTimeDemoKey = CryGetAsyncKeyState(VK_SNAPSHOT) & 1;

		if (bCancel)
		{
			if (IsRecording())
			{
				// stop all test modules
				Record(false);
				return;
			}
			// Toggle start/stop of demo recording.
			if (IsPlaying())
			{
				// Stop playing.
				//Play(false);
				// stop all test modules
				Play(false);
				return;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Time demo on/off
		//////////////////////////////////////////////////////////////////////////
		if ((bCtrl||bAlt) && bTimeDemoKey)
		{
			if (!IsRecording())
			{
				// Start record.
				Record(true);
			}
		}
		if (bShift && bTimeDemoKey)
		{
			if (!IsPlaying())
			{
				// Load and start playing.
				Play(true);
			}
		}
	}
#endif

	bool bPaused = false;
	if (m_bRecording || m_bPlaying)
	{
		bPaused =  gEnv->pConsole->IsOpened();

		if (!bPaused && m_demo_scroll_pause != 0 && gEnv->pInput)
		{
			static TKeyName scrollKey("scrolllock");
			bPaused = gEnv->pInput->InputState(scrollKey, eIS_Down);
		}

		if (bPaused != m_bPaused)
			Pause(bPaused);
	}

	if(!m_bPaused  && !gEnv->pConsole->GetStatus()) // Only when console closed.)
	{
		if (m_bRecording)
		{
			
			// Reset random number generators seed.

			GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_RANDOM_SEED,0,0 );

			TestModuleMap::iterator it = s_pTestManager->m_TestModules.begin(), itEnd = s_pTestManager->m_TestModules.end();
			for(; it!=itEnd;++it) 
			{
				ITestModule* pTM = it->second; 
				if(pTM->IsEnabled()) 
					m_bRecording &= pTM->RecordFrame();
			}
		}

		if(m_bRecording ||m_bPlaying)
			TEST_MODULE_LOOP(Update());
		
		//m_currentFrame++;
		
		if ((m_bPlaying || m_bRecording) && m_demo_noinfo <= 0)
			RenderInfo();

	}
}


//////////////////////////////////////////////////////////////////////////
void CTestManager::Pause(bool bPaused)
{
	m_bPaused = bPaused;
	TEST_MODULE_LOOP(Pause(bPaused));
}

//////////////////////////////////////////////////////////////////////////
CTimeValue CTestManager::GetTime()
{
	// Must be asynchronius time, used for profiling.
	return gEnv->pTimer->GetAsyncTime();
}


/*
//////////////////////////////////////////////////////////////////////////
float CTestManager::GetAverageFrameRate() const
{
	if(m_currentFrame)
	{
		float aveFrameTime = m_totalDemoTime.GetSeconds() / m_currentFrame;
		float aveFrameRate = 1.0f / aveFrameTime;
		return aveFrameRate;
	}
	return 0.0f;
}
*/
//////////////////////////////////////////////////////////////////////////
void CTestManager::RenderInfo()
{
	if (m_demo_noinfo != 0)
		return;

	const char *sInfo = m_bPaused ? " (Paused)" : "";
	
	float y=1;

	IRenderer *pRenderer = gEnv->pRenderer;
	if (m_bRecording)
	{
		// TO DO 
		float fColor[4] = {0.7f,0,0,1};
		pRenderer->Draw2dLabel( 1,y, 1.3f, fColor,false,"Recording AutoTest%s", sInfo);
	}
	else if (m_bPlaying)
	{
		float fColor[4] = {0,0.7f,0,1};
		pRenderer->Draw2dLabel( 1, y, 1.3f, fColor,false,"Playing AutoTest%s - Loop %d of %d", sInfo, m_numLoops+1, m_maxLoops);

	}
	
	y+=15;

	TestModuleMap::iterator it = m_TestModules.begin(),itEnd = m_TestModules.end();
	for(;it!=itEnd;++it)
	{
		y += it->second->RenderInfo(y);
	}

}

//////////////////////////////////////////////////////////////////////////
template<class T> void CTestManager::SetConsoleVar( const char *sVarName,T value )
{
	ICVar *pVar = gEnv->pConsole->GetCVar( sVarName );
	if (pVar)
		pVar->Set( value );
}

//////////////////////////////////////////////////////////////////////////
float CTestManager::GetConsoleVar( const char *sVarName )
{
	ICVar *pVar = gEnv->pConsole->GetCVar( sVarName );
	if (pVar)
		return pVar->GetFVal();
	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CTestManager::StartSession()
{

#ifdef SP_DEMO
	//allow devmode in SP_DEMO only during demo playback
	CCryAction::GetCryAction()->CreateDevMode();
#endif

	// cache console vars - changing them runtime has no effect
	m_bAIEnabled = m_demo_ai>0;
	
	Pause(false);

	//////////////////////////////////////////////////////////////////////////


	bool bCurrentlyRecording = m_bRecording;
	m_bRecording = false;
	if (m_demo_restart_level == 1 && bCurrentlyRecording)
	{
		//QS does not work when playing/recording a timedemo, so pretend we're not...
		// Quick save at the begining of the recording, so we restore initial state as good as possible.
		CCryAction::GetCryAction()->SaveGame( GetInitSaveFileName(), true,true,eSGR_QuickSave,true );
	}

	ResetSessionLoop();
	m_bRecording = bCurrentlyRecording;

	// test modules' StartSession() are called by their respective Record()/Play()
	//TEST_MODULE_LOOP(StartSession());

	//////////////////////////////////////////////////////////////////////////
	// Force player out of the vehicle on start.
	//////////////////////////////////////////////////////////////////////////
	// Luc TO DO: remove this
	gEnv->pConsole->ExecuteString( "v_exit_player" );

	if (m_demo_vtune)
		GetISystem()->GetIProfilingSystem()->VTuneResume();

	m_lastFrameTime = GetTime();

	m_fixedTimeStep = GetConsoleVar( "t_FixedStep" );
	if(m_demo_fixed_timestep>0)
		SetConsoleVar( "t_FixedStep",1.0f/(float)m_demo_fixed_timestep );

}

//////////////////////////////////////////////////////////////////////////
void CTestManager::StopSession()
{
#ifdef SP_DEMO
	CCryAction::GetCryAction()->RemoveDevMode();
#endif
	if (m_demo_vtune)
		GetISystem()->GetIProfilingSystem()->VTunePause();

	// Set old time step.
	SetConsoleVar( "t_FixedStep",m_fixedTimeStep );
	
	if (m_demo_ai == 0)
	{
		SetConsoleVar( "ai_SystemUpdate",1 );
		SetConsoleVar( "ai_IgnorePlayer",0 );
		SetConsoleVar( "ai_SoundPerception",1 );
	}
	else
	{
		SetConsoleVar( "ai_UseCalculationStopperCounter",0 ); // To make AI async time independent.
	}
	SetConsoleVar( "mov_NoCutscenes",0 );
	//SetConsoleVar("g_godMode",m_prevGodMode);

	
	TEST_MODULE_LOOP(StopSession()); 
}

//////////////////////////////////////////////////////////////////////////
void CTestManager::ResetSessionLoop()
{
	if (m_demo_restart_level != 0 && m_bPlaying)
	{
		switch( m_demo_restart_level )
		{
		case 1:
			// Quick load at the begining of the playback, so we restore initial state as good as possible.
			if( gEnv->pGame->GetIGameFramework() )
				gEnv->pGame->GetIGameFramework()->LoadGame(GetInitSaveFileName());//, true, true);
			
			break;

		case 2:
		default:	
			//load save made at start of level
			CCryAction::GetCryAction()->LoadGame(CCryAction::GetCryAction()->GetStartLevelSaveGameName());//, true);
			break;
		}


	}
	// Reset random number generators seed.
	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_RANDOM_SEED,0,0 );
	m_currentFrame = 0;
	m_totalDemoTime.SetMilliSeconds(0);
}

//////////////////////////////////////////////////////////////////////////
void CTestManager::LogInfo( const char *format,... )
{
	if (m_demo_noinfo != 0)
		return;

	va_list	ArgList;
	char szBuffer[1024];

	va_start(ArgList, format);
	vsprintf_s(szBuffer, format, ArgList);
	va_end(ArgList);

	char path_buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	gEnv->pLog->Log( szBuffer  );

	_splitpath( m_file.c_str(), drive, dir, fname, ext );
	_makepath( path_buffer, drive, dir,fname,"log" );
	FILE *hFile = fopen( path_buffer,"at" );
	if (hFile)
	{
		// Write the string to the file and close it
		fprintf(hFile, "%s\n",szBuffer );
		fclose(hFile);
	}
}



//////////////////////////////////////////////////////////////////////////
void CTestManager::EndLog()
{
	LogInfo( "\n!----------------AutoTest Run %d Finished.------------------",m_numLoops );

	TEST_MODULE_LOOP(EndLog());
	// Save level stats, after each run.
	if (m_demo_savestats != 0)
	{
		// Save stats after last run only.
		if (/*m_demo_quit &&*/ m_numLoops >= m_maxLoops)
		{
			LogInfo( "Execute SaveLevelStats" );
			gEnv->pConsole->ExecuteString("SaveLevelStats");
			LogInfo( "SaveLevelStats done" );
		}
	}
}

void CTestManager::GetMemoryStatistics(ICrySizer * s)
{
	SIZER_SUBCOMPONENT_NAME(s, "TestManager");
	s->Add(*this);
	// add the test methods
	TEST_MODULE_LOOP_ALL(GetMemoryStatistics(s));
}

//////////////////////////////////////////////////////////////////////////
void CTestManager::StartChainDemo( const char *levelsListFilename )
{
	m_demoLevels.clear();
	if (levelsListFilename && *levelsListFilename)
	{
		// Open file with list of levels for autotest.
		// This is used only for development so doesn`t need to use CryPak!
		FILE *f = fopen( levelsListFilename,"rt" );
		if (f)
		{
			while (!feof(f))
			{
				char str[512];
				fgets( str,sizeof(str),f );
				if (*str)
				{
					string level = str;
					level.Trim();
					if(level.size())
						m_demoLevels.push_back(level);
				}
			}
			fclose(f);
		}
	}
	
	if(IsRecording())
		Record(false);
	else if(IsPlaying())
		Play(false);

	m_nCurrentDemoLevel = 0;
	StartNextChainedLevel();
}

//////////////////////////////////////////////////////////////////////////
void CTestManager::StartNextChainedLevel()
{
	if (!m_demoLevels.empty())
	{
		// This support loading level/playing time demo, then loading next level, etc...
		if (m_nCurrentDemoLevel < m_demoLevels.size())
		{
			string mapCmd = string("map ") + m_demoLevels[m_nCurrentDemoLevel];
			gEnv->pConsole->ExecuteString( mapCmd );
			Play(true);
			m_nCurrentDemoLevel++;
			return;
		}
	}
	if (m_demo_quit)
	{
		gEnv->pRenderer->RestoreGamma();
		gEnv->pRenderer->ShutDownFast();
		CryLogAlways( "Testing Successfully Finished, Exiting..." );

		// If No more chained levels. quit.
		gEnv->pConsole->ExecuteString("quit");
	}
}

ETestModuleType GetTestModuleType(const char* name)
{
	if(stricmp(name,"global")==0)
		return TM_GLOBAL;
	else if(stricmp(name,"timedemo")==0)
		return TM_TIMEDEMO;

	return TM_NONE;
}


//////////////////////////////////////////////////////////////////////////
ITestModule* CTestManager::SetTestModule(ETestModuleType tmType, bool enable)
{
	ITestModule* pModule = 0;
	TestModuleMap::iterator it = m_TestModules.find(tmType);
	if(it != m_TestModules.end())
	{
		//only one module per type, return the existing found module
		pModule = it->second;
	}
	else
	{
		// create a new test module
		switch(tmType)
		{
		case TM_TIMEDEMO:
			{
				pModule = new CTimeDemoRecorder(m_pSystem, this);

			}
			break;
		case TM_PROFILER:
			{
				pModule = new CTestProfiler(m_pSystem,this);

			}
			break;
		default:
			break;
		}

		if(pModule)
		{
//				pNewModule->ParseParams(pNode);
			m_TestModules.insert(std::make_pair(tmType,pModule));
		}
	}

	if(pModule)
		pModule->Enable(enable);

	return pModule;
}



//////////////////////////////////////////////////////////////////////////
ITestModule* CTestManager::GetTestModule(ETestModuleType tmType)  const
{
	TestModuleMap::const_iterator it = m_TestModules.find(tmType);
	if(it != m_TestModules.end())
		return it->second;

	return NULL;
}


//////////////////////////////////////////////////////////////////////////
void CTestManager::ReleaseTestModules()
{
	while(!m_TestModules.empty())
	{
		TestModuleMap::iterator it = m_TestModules.begin();
		ITestModule* pTM = it->second;
		SAFE_DELETE(pTM);
		m_TestModules.erase(it);
	}
}



//////////////////////////////////////////////////////////////////////////

bool CTestManager::ParseXmlFile(const char* szFileName)
{
	typedef std::map<const char*,float> TMapFloatVar;
	TMapFloatVar MapFloatVar;

	typedef std::map<const char*,const char*> TMapStringVar;
	TMapStringVar MapStringVar;

	XmlNodeRef root = GetISystem()->LoadXmlFile(szFileName);
	if (!root)
		return false;

	XmlNodeRef nodeWorksheet = root->findChild("Worksheet");
	if (!nodeWorksheet)
		return false;

	XmlNodeRef nodeTable = root->findChild("Table");
	if (!nodeTable)
		return false;

	int childCount = nodeTable->getChildCount();
	int rowCount = 0;

	ITestModule* pCurrentTestModule = 0;

	for(int i=0; i < childCount; i++)
	{
		XmlNodeRef nodeRow = nodeTable->getChild(i);
		if (!nodeRow || !nodeRow->isTag("Row"))
			continue;
		
		++rowCount;		

		int rowChildCount = nodeRow->getChildCount();
		const char* varName=0;
		int cellCount=0;
		for(int j=0; j < rowChildCount; j++)
		{
			XmlNodeRef nodeCell = nodeRow->getChild(j);
			if(!nodeCell || !nodeCell->isTag("Cell"))
				continue;
			XmlNodeRef nodeCellData = nodeCell->findChild("Data");
			if (!nodeCellData)
				continue;
			
			cellCount++;

			if(nodeCellData->haveAttr("ss:Type"))
			{
				const char* szType=nodeCellData->getAttr("ss:Type");
				if(cellCount==1 && strcmp(szType,"String")==0)
				{
					const char* name = nodeCellData->getContent();
					if(name )
					{
						ETestModuleType tmType = GetTestModuleType(name);
						if(tmType != TM_NONE)
						{
							// new test module/global section
							pCurrentTestModule = tmType != TM_GLOBAL ? SetTestModule(tmType): 0;
						}
						else // variable name in current section
							varName = name;
					}
				}
				else if(cellCount==2 && varName)
				{
					if(strcmp(szType,"Number")==0)
					{
						const char* content = nodeCellData->getContent();
						float value = 0;
						if (sscanf(content,"%f",&value) != 1) CRY_ASSERT(0);
						if(pCurrentTestModule)
							pCurrentTestModule->SetVariable(varName,value);
						else
							SetVariable(varName,value);
						//MapFloatVar.insert(std::make_pair(varName,value));
					}
					else if(strcmp(szType,"String")==0)
					{
						const char* content = nodeCellData->getContent();
//						MapStringVar.insert(std::make_pair(varName,content));
						if(pCurrentTestModule)
							pCurrentTestModule->SetVariable(varName,content);
						else
							SetVariable(varName,content);
					}
				}
				varName = 0;
			}
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////
// Executed as soon as the play session command is 
/// separate time from StartSession() since game maybe not yet initialized at this time
void CTestManager::PreStartSession()
{

	for(int t=TM_NONE; t<TM_LAST; t++)
		SetTestModule((ETestModuleType)t, (t == TM_TIMEDEMO || (t == TM_PROFILER && m_demoProfile>0))); // enable only these as default configuration

	if (gEnv->pMovieSystem)
		gEnv->pMovieSystem->StopAllCutScenes();

	if (m_demo_ai == 0)
	{
#ifdef SP_DEMO
		// force these for sp demo during demo recording because there is no devmode available
		ICVar *pVar = gEnv->pConsole->GetCVar( "ai_systemupdate" );
		if (pVar) pVar->ForceSet( "0" );
		pVar = gEnv->pConsole->GetCVar( "ai_ignoreplayer" );
		if (pVar) pVar->ForceSet( "1" );
		pVar = gEnv->pConsole->GetCVar( "ai_soundperception" );
		if (pVar) pVar->ForceSet( "0" );
#else
		SetConsoleVar( "ai_SystemUpdate",0 );
		SetConsoleVar( "ai_IgnorePlayer",1 );
		SetConsoleVar( "ai_SoundPerception",0 );
#endif
	}
	else
	{
		SetConsoleVar( "ai_UseCalculationStopperCounter",1 ); // To make AI async time independent.
	}

	// No wait for key-press on level load.
	SetConsoleVar( "hud_startPaused",0 );
	// Not cut-scenes 
	SetConsoleVar( "mov_NoCutscenes",1 );

	m_prevGodMode = (int)GetConsoleVar("g_godMode");
	//SetConsoleVar("g_godMode",1);
	//if (m_demo_panoramic != 0)
	//SetConsoleVar( "e_Panorama",1 );


}

//////////////////////////////////////////////////////////////////////////
void CTestManager::cmd_StartRecordingTimeDemo( IConsoleCmdArgs *pArgs )
{
	if(s_pTestManager)
	{
		if(s_pTestManager->IsRecording())
			return;

		s_pTestManager->Record(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTestManager::cmd_Play( IConsoleCmdArgs *pArgs )
{
	if(s_pTestManager)
		s_pTestManager->PlayInit(pArgs);
}

//////////////////////////////////////////////////////////////////////////
void CTestManager::cmd_Stop( IConsoleCmdArgs *pArgs )
{
	if(s_pTestManager)
		s_pTestManager->Record(false);
	if(s_pTestManager)
		s_pTestManager->Play(false);
}

//////////////////////////////////////////////////////////////////////////
void CTestManager::cmd_StartDemoChain( IConsoleCmdArgs *pArgs )
{
	if (pArgs->GetArgCount() > 1)
	{
		const char *sLevelsFile = pArgs->GetArg(1);
		s_pTestManager->StartChainDemo( sLevelsFile );
	}
	else
	{
		CryLogAlways( "Expect filename with the list of the levels" );
	}
	
}

//////////////////////////////////////////////////////////////////////////
int CTestManager::GetNumberOfFrames() const
{
	// When recording return max frames that can be recorded.
	if (m_bRecording)
		return m_demo_max_frames;

	int maxFrames= 0;
	TestModuleMap::iterator it = s_pTestManager->m_TestModules.begin(), itEnd = s_pTestManager->m_TestModules.end();
	for(; it!=itEnd;++it) 
	{
		int numFrames = it->second->GetNumberOfFrames();
		if(maxFrames < numFrames)
			maxFrames = numFrames;
	}

	if(maxFrames ==0 )
		maxFrames = m_demo_max_frames;
	
	return maxFrames;
}

//////////////////////////////////////////////////////////////////////////
int CTestManager::GetTotalPolysRecorded() const
{
	ITestModule* pTimeDemo = GetTestModule(TM_TIMEDEMO);
	return pTimeDemo ? pTimeDemo->GetTotalPolysRecorded() : 0;
}

//////////////////////////////////////////////////////////////////////////
bool CTestManager::IsTimeDemo() const
{
	ITestModule* pTimeDemo = GetTestModule(TM_TIMEDEMO);
	return pTimeDemo && pTimeDemo->IsEnabled();
}


//////////////////////////////////////////////////////////////////////////
void CTestManager::SetVariable(const char* name,const char* szValue)
{
	// TO DO: set non-console variables, leave the console variable setting as default in the end
	SetConsoleVar( name, szValue );
}

//////////////////////////////////////////////////////////////////////////
void CTestManager::SetVariable(const char* name,float value)
{
	// TO DO: set non-console variables, leave the console variable setting as default in the end
	SetConsoleVar( name, value );
}

//////////////////////////////////////////////////////////////////////////
void CTestManager::SetDemoFixedTimeStep(uint16 value)
{
	m_demo_fixed_timestep = value;
	if(m_demo_fixed_timestep>0)
		SetConsoleVar("t_FixedStep",1/(float)m_demo_fixed_timestep);
}
