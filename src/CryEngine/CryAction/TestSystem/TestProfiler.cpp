////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   testprofiler.h
//  Version:     v1.00
//  Created:     13/3/2008 by Luciano Morpurgo (refactoring test system).
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"
#include "TestProfiler.h"
//#include <CryFile.h>
//#include <IActorSystem.h>
#include <IAgent.h>
#include <ILevelSystem.h>
#include <ITestSystem.h>
#include <IMovieSystem.h>
//#include "IMovementController.h"

#if defined(WIN32)
#include <windows.h>
#endif

#if defined(WIN32) && !defined(WIN64)
//#include "Psapi.h"								// PSAPI is not supported on windows9x
//#pragma comment(lib,"Psapi.lib")	// PSAPI is not supported on windows9x
#endif


//#define FIXED_TIME_STEP (30) // Assume runing at 30fps.


int CTestProfiler::m_minFPSCounter = 60;

//////////////////////////////////////////////////////////////////////////

CTestProfiler::CTestProfiler( ISystem *pSystem, const CTestManager* pTestManager )
{
	m_pSystem = pSystem;
	m_pTestManager = pTestManager;

	CRY_ASSERT(m_pSystem);

	m_bRecording = false;
	m_bPlaying = false;
	m_bDemoFinished = false;
	m_bPlayingTimeDemo = false;

	m_pTimeDemoInfo = 0;
	m_demo_noinfo	= 0;
	//m_recordStartTime = 0;
	//m_recordEndTime = 0;
	m_lastFrameTime = GetTime();
	//m_totalDemoTime = 0;
	//m_recordedDemoTime = 0;

	m_lastAveFrameRate = 0;
	m_lastPlayedTotalTime = 0;

	m_currentFrame= 0;
	m_bPaused = false;


}

//////////////////////////////////////////////////////////////////////////
CTestProfiler::~CTestProfiler()
{

}


//////////////////////////////////////////////////////////////////////////
void CTestProfiler::StartRecording( IConsoleCmdArgs *pArgs )
{
	Enable(true);
	
}

//////////////////////////////////////////////////////////////////////////
void CTestProfiler::StopRecording( IConsoleCmdArgs *pArgs )
{
}


//////////////////////////////////////////////////////////////////////////
void CTestProfiler::Record( bool bEnable )
{

	m_bRecording = bEnable;
	m_bPlaying = false;
	if (m_bRecording)
	{
//		m_currentFrame = 0;
//		m_totalDemoTime.SetMilliSeconds(0);
		StartSession();
		//m_recordStartTime = GetTime();
		m_lastFrameTime = GetTime();
	}
	else
	{
		// Stop recording.
		//m_recordedDemoTime = m_pTestManager->GetTotalTime();//m_totalDemoTime;
		m_lastFrameTime = GetTime();
//		StopSession();
	}
	//m_currentFrame = 0;
	//m_totalDemoTime.SetMilliSeconds(0);
	m_bPlayingTimeDemo = false;
}

//////////////////////////////////////////////////////////////////////////
void CTestProfiler::Play( bool bEnable )
{
	if (bEnable == m_bPlaying)
		return;

	m_bPlaying = bEnable;
	m_bPlayingTimeDemo = bEnable && m_pTestManager->IsTimeDemo();

	if (m_bPlaying)
	{
		LogInfo( "TestProfiler Play Started");

		m_bDemoFinished = false;

		m_lastPlayedTotalTime = 0;
		StartSession();
		
		if (!m_pTimeDemoInfo && m_bPlayingTimeDemo)
		{
			m_pTimeDemoInfo = new STimeDemoInfo();
			m_pTimeDemoInfo->pFrames = 0;
		}

		int size = m_pTestManager->GetNumberOfFrames();

		if (m_bPlayingTimeDemo && m_pTimeDemoInfo && m_pTimeDemoInfo->nFrameCount != size)//(int)m_records.size())
		{
			delete []m_pTimeDemoInfo->pFrames;
			STimeDemoInfo *pTD = m_pTimeDemoInfo;
			pTD->nFrameCount = size;//(int)m_records.size();
			pTD->pFrames = new STimeDemoFrameInfo[pTD->nFrameCount];
		}

	}
	else
	{
		LogInfo( "TestProfiler Play Ended");//, (%d Runs Performed)",m_numLoops );
		//LogInfo( "==============================================================" );

		// End demo playback.
		m_lastPlayedTotalTime = m_pTestManager->GetTotalTime()/*m_totalDemoTime*/.GetSeconds();
//		StopSession();
	}
	m_bRecording = false;
	//m_currentFrame = 0;
	//m_totalDemoTime.SetMilliSeconds(0);
	//m_numLoops = 0;
	m_fpsCounter = 0;
	m_lastFpsTimeRecorded = GetTime();

	m_currFPS = 0;
	m_sumFPS = 0;
	m_minFPS = 10000;
	m_maxFPS = -10000;
	m_nMaxPolys = INT_MIN;
	m_nMinPolys = INT_MAX;
}

//////////////////////////////////////////////////////////////////////////
void CTestProfiler::PreUpdate()
{
	m_currentFrame = m_pTestManager->GetCurrentFrame();
}

//////////////////////////////////////////////////////////////////////////
void CTestProfiler::Update()
{
}

//////////////////////////////////////////////////////////////////////////
bool CTestProfiler::RecordFrame()
{
	int nPolygons = ComputePolyCount();

	m_nTotalPolysRecorded += nPolygons; 
	m_nTotalPolysPlayed += nPolygons;
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CTestProfiler::PlayFrame()
{
	// If the current frame is equals or greater than the total number of frames...
	// ...as this is a zero based index, it means we already played all frames...
  // ... and thus we should not play anymore frames...
	if (m_currentFrame>=m_pTimeDemoInfo->nFrameCount)
	{
		return false;
	}
	
	CTimeValue time = GetTime();
	
	CTimeValue deltaFrameTime = (time - m_lastFrameTime);
	float frameTime = deltaFrameTime.GetSeconds();

	if (m_bPaused)
	{
	//	m_lastFrameTime = time;		
		return true;
	}

	//////////////////////////////////////////////////////////////////////////


	//	m_totalDemoTime += deltaFrameTime;
	

	int nPolygons = ComputePolyCount();

//	m_nTotalPolysRecorded += nPolygons; //m_pTestManager->GetTotalPolysRecorded();
	m_nTotalPolysPlayed += nPolygons;

	//////////////////////////////////////////////////////////////////////////
	// Calculate Frame Rates.
	//////////////////////////////////////////////////////////////////////////
	// Skip some frames before calculating frame rates.
	float timeElapsed = (time - m_lastFpsTimeRecorded).GetSeconds();
	if (timeElapsed > 1 && m_fpsCounter>0)
	{
		// Skip some frames before recording frame rates.
		//if (m_currentFrame > 60)
		m_nPolysPerSec = (int)(float(m_nPolysCounter)  / timeElapsed);
		m_nPolysCounter = 0;

		m_fpsCounter = 0;
		m_lastFpsTimeRecorded = time;
	}
	else
	{
		m_fpsCounter++;
	}

	if (m_currentFrame > m_minFPSCounter)
	{
		//m_currFPS = (float)m_fpsCounter / timeElapsed;

		m_currFPS = (float)(1.0 / deltaFrameTime.GetSeconds());
		m_sumFPS += m_currFPS;

		if (m_currFPS > m_maxFPS)
		{
			m_maxFPS_Frame = m_currentFrame;
			m_maxFPS = m_currFPS;
		}
		if (m_currFPS < m_minFPS)
		{
			m_minFPS_Frame = m_currentFrame;
			m_minFPS = m_currFPS;
		}

	}
	
	//////////////////////////////////////////////////////////////////////////
	// Fill Time Demo Info structure.
	//////////////////////////////////////////////////////////////////////////
	if(m_pTimeDemoInfo && m_bPlayingTimeDemo)
	{
		m_pTimeDemoInfo->pFrames[m_currentFrame].fFrameRate = (float)(1.0 / deltaFrameTime.GetSeconds());
		m_pTimeDemoInfo->pFrames[m_currentFrame].nPolysRendered = nPolygons;
		m_pTimeDemoInfo->pFrames[m_currentFrame].nDrawCalls = gEnv->pRenderer->GetCurrentNumberOfDrawCalls();
	}
	//////////////////////////////////////////////////////////////////////////

	//m_currentFrame++;

	/*
	if (m_currentFrame >= m_demo_max_frames)
	{
		m_lastPlayedTotalTime = m_totalDemoTime.GetSeconds();
		m_lastAveFrameRate = GetAverageFrameRate();

		// Log info to file.
		LogRun();


		//ResetSessionLoop();
		m_lastFrameTime = GetTime();
		return false;
	}
	*/
	m_lastFrameTime = GetTime();	
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTestProfiler::Restart()
{
//	m_currentFrame = 0;
	m_nTotalPolysPlayed = 0;
	m_nTotalPolysRecorded = 0;
	m_sumFPS = 0;
	m_currFPS = 0;
}

//////////////////////////////////////////////////////////////////////////
CTimeValue CTestProfiler::GetTime()
{
	// Must be asynchronius time, used for profiling.
	return gEnv->pTimer->GetAsyncTime();
}

/*
//////////////////////////////////////////////////////////////////////////
int CTestProfiler::GetNumFrames() const
{
	//TO DO: check if it's right
	return m_currentFrame;//m_records.size();
}
*/
//////////////////////////////////////////////////////////////////////////
float CTestProfiler::GetAverageFrameRate() const
{
	//if(m_currentFrame)
	//{
//		float aveFrameTime = m_totalDemoTime.GetSeconds() / m_currentFrame;
		float aveFrameTime = m_pTestManager->GetTotalTime().GetSeconds() / (m_currentFrame+1);
		float aveFrameRate = 1.0f / aveFrameTime;
		return aveFrameRate;
//	}
//	return 0.0f;
}

//////////////////////////////////////////////////////////////////////////
float CTestProfiler::RenderInfo(float y)
{
	float retY = 0;

	if (m_demo_noinfo != 0)
		return retY ;

	const char *sInfo = m_bPaused ? " (Paused)" : "";
	
	IRenderer *pRenderer = gEnv->pRenderer;
	
	if (m_bRecording)
	{
		float fColor[4] = {1,0,0,1};
		pRenderer->Draw2dLabel( 1,y+retY, 1.3f, fColor,false,"TestProfiler%s on",sInfo );
		retY +=15;
	}
	else if (m_bPlaying)
	{
		//float aveFrameRate = GetAverageFrameRate();
		float aveFrameRate = m_currentFrame > m_minFPSCounter ? m_sumFPS/(m_currentFrame - m_minFPSCounter) : 0;
		//float aveFrameRate = m_sumFPS/(m_currentFrame +1);
		float polyRatio = m_nTotalPolysPlayed ? (float)m_pTestManager->GetTotalPolysRecorded()/m_nTotalPolysPlayed : 0.0f;
		//int numFrames = GetNumFrames();
		float fColor[4] = {0,1,0,1};
		pRenderer->Draw2dLabel( 1,y+retY, 1.3f, fColor,false,"TestProfiler%s, Frame %d",sInfo,m_currentFrame);
		retY +=15;
		pRenderer->Draw2dLabel( 1,y+retY, 1.3f, fColor,false," Last Played Length: %.2fs, FPS: %.2f",m_lastPlayedTotalTime,m_lastAveFrameRate );
		retY +=15;
		pRenderer->Draw2dLabel( 1,y+retY, 1.3f, fColor,false," Average FPS: %.2f, FPS: %.2f, Polys/Frame: %d",aveFrameRate,m_currFPS,m_nCurrPolys );
		retY +=15;

		pRenderer->Draw2dLabel( 1,y+retY, 1.3f, fColor,false," Polys Rec/Play Ratio: %.2f",polyRatio );
		retY +=15;
	}
	return retY;
}

//////////////////////////////////////////////////////////////////////////
void CTestProfiler::SetConsoleVar( const char *sVarName,float value )
{
	ICVar *pVar = gEnv->pConsole->GetCVar( sVarName );
	if (pVar)
		pVar->Set( value );
}

//////////////////////////////////////////////////////////////////////////
float CTestProfiler::GetConsoleVar( const char *sVarName )
{
	ICVar *pVar = gEnv->pConsole->GetCVar( sVarName );
	if (pVar)
		return pVar->GetFVal();
	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CTestProfiler::StartSession()
{
\
#ifdef SP_DEMO
	//allow devmode in SP_DEMO only during demo playback
	CCryAction::GetCryAction()->CreateDevMode();
#endif

	m_nTotalPolysRecorded = 0;
	m_nTotalPolysPlayed = 0;
	m_lastAveFrameRate = 0;
	m_lastPlayedTotalTime = 0;

	//cache CVars
	// beware: this means that changing these cvars during test has no effect
	//m_demo_noinfo = GetConsoleVar("demo_noinfo");
	//m_demoProfile = GetConsoleVar("demo_profile");

	// Register to frame profiler.

	// remember old profiling settings
	m_bEnabledProfiling = gEnv->pFrameProfileSystem->IsEnabled();
	m_bVisibleProfiling = gEnv->pFrameProfileSystem->IsVisible();

	//if (m_demoProfile)
	{
		gEnv->pFrameProfileSystem->Enable(true, gEnv->pFrameProfileSystem->IsVisible());
	}

	gEnv->pFrameProfileSystem->AddPeaksListener( this );

	// Profile
	m_oldPeakTolerance = GetConsoleVar( "profile_peak" );
	SetConsoleVar( "profile_peak",50 );

	//ResetSessionLoop();

	m_lastFrameTime = GetTime();
}

//////////////////////////////////////////////////////////////////////////
void CTestProfiler::StopSession()
{
#ifdef SP_DEMO
	CCryAction::GetCryAction()->RemoveDevMode();
#endif

	// Profile.
	SetConsoleVar( "profile_peak",m_oldPeakTolerance );
	gEnv->pFrameProfileSystem->RemovePeaksListener( this );

	if (m_demoProfile)
	{
		gEnv->pFrameProfileSystem->Enable(m_bEnabledProfiling, m_bVisibleProfiling);
	}
	//m_lastPlayedTotalTime = m_totalDemoTime.GetSeconds();
	m_lastPlayedTotalTime = m_pTestManager->GetTotalTime().GetSeconds();

}

/*
//////////////////////////////////////////////////////////////////////////
void CTestProfiler::ResetSessionLoop()
{

	if (m_demo_restart_level != 0 && m_bPlaying)
	{
		switch( m_demo_restart_level )
		{
		case 1:
			// Quick load at the begining of the playback, so we restore initial state as good as possible.
			CCryAction::GetCryAction()->LoadGame( PathUtil::Make( "",s_timedemo_file->GetString(),".qsv"),true );
			break;

		case 2:
		default:	
			//load save made at start of level
			if( gEnv->pGame->GetIGameFramework() )
			{
				string levelstart( gEnv->pGame->GetIGameFramework()->GetLevelName() );
				if(const char* visibleName = gEnv->pGame->GetMappedLevelName(levelstart))
				{
					levelstart = visibleName;
				}
				levelstart.append("_crysis.crysisjmsf");	//TODO: refactor to remove crysis
				gEnv->pGame->GetIGameFramework()->LoadGame(levelstart.c_str(), true, true);
			}
			break;

		}


	}
	// Reset random number generators seed.
	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_RANDOM_SEED,0,0 );
	
}
*/

//////////////////////////////////////////////////////////////////////////
void CTestProfiler::LogInfo( const char *format,... )
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
void CTestProfiler::OnFrameProfilerPeak( CFrameProfiler *pProfiler,float fPeakTime )
{
	if (m_bPlaying && !m_bPaused)
	{
		LogInfo( "    -Peak at Frame %d, %.2fms : %s (count: %d)",m_currentFrame,fPeakTime,pProfiler->m_name,pProfiler->m_count );
	}
}


//////////////////////////////////////////////////////////////////////////
void CTestProfiler::EndLog()
{
	//m_lastPlayedTotalTime = m_totalDemoTime.GetSeconds();
	m_lastPlayedTotalTime = m_pTestManager->GetTotalTime().GetSeconds();
	m_lastAveFrameRate = GetAverageFrameRate();

	if(m_pTimeDemoInfo && m_bPlayingTimeDemo)
	{
		STimeDemoInfo *pTD = m_pTimeDemoInfo;
		pTD->lastPlayedTotalTime = m_lastPlayedTotalTime;
		pTD->lastAveFrameRate = m_lastAveFrameRate;
		pTD->minFPS = m_minFPS;
		pTD->maxFPS = m_maxFPS;
		pTD->minFPS_Frame = m_minFPS_Frame;
		pTD->maxFPS_Frame = m_maxFPS_Frame;
		m_nTotalPolysRecorded = m_pTestManager->GetTotalPolysRecorded();
		pTD->nTotalPolysRecorded = m_nTotalPolysRecorded;
		pTD->nTotalPolysPlayed = m_nTotalPolysPlayed;
		GetISystem()->GetITestSystem()->SetTimeDemoInfo( m_pTimeDemoInfo );
	}

	int numFrames = m_pTestManager->GetNumberOfFrames();//m_records.size();
	LogInfo( "!TestProfiler Run Finished.");
	LogInfo( "    Play Time: %.2fs, Average FPS: %.2f",m_lastPlayedTotalTime,m_lastAveFrameRate );
	LogInfo( "    Min FPS: %.2f at frame %d, Max FPS: %.2f at frame %d",m_minFPS,m_minFPS_Frame,m_maxFPS,m_maxFPS_Frame );
	if (m_lastPlayedTotalTime * (float)numFrames > 0.f)
		LogInfo( "    Average Tri/Sec: %d, Tri/Frame: %d",(int)(m_nTotalPolysPlayed/m_lastPlayedTotalTime),m_nTotalPolysPlayed/numFrames );
	if (m_nTotalPolysPlayed)
		LogInfo( "    Recorded/Played Tris ratio: %.2f",(float)m_nTotalPolysRecorded/m_nTotalPolysPlayed );


#if defined(WIN32) && !defined(WIN64)

	// PSAPI is not supported on window9x
	// so, don't use it

	//PROCESS_MEMORY_COUNTERS pc;
	//HANDLE hProcess = GetCurrentProcess();
	//pc.cb = sizeof(pc);
	//GetProcessMemoryInfo( hProcess,&pc,sizeof(pc) );
	//int MB = 1024*1024;
	//LogInfo( "    Memory Usage: WorkingSet=%dMb, PageFile=%dMb, PageFaults=%d",pc.WorkingSetSize/MB,pc.PagefileUsage/MB,pc.PageFaultCount );

#endif

}

void CTestProfiler::GetMemoryStatistics(ICrySizer * s)
{
	SIZER_SUBCOMPONENT_NAME(s, "TestProfiler");
	s->Add(*this);
}


int CTestProfiler::ComputePolyCount()
{
	int nPolygons,nShadowVolPolys;
	gEnv->pRenderer->GetPolyCount(nPolygons,nShadowVolPolys);
	m_nPolysCounter += nPolygons;
	m_nCurrPolys = nPolygons;
	if (nPolygons > m_nMaxPolys)
	m_nMaxPolys = nPolygons;
	if (nPolygons < m_nMinPolys)
	m_nMinPolys = nPolygons;
	return nPolygons;
}

///////////////////////////////////////////////////////////////////////////////
void CTestProfiler::ParseParams(XmlNodeRef baseNode) 
{
	
}

///////////////////////////////////////////////////////////////////////////////
void CTestProfiler::SetVariable(const char* name,const char* szValue)
{

}

///////////////////////////////////////////////////////////////////////////////
void CTestProfiler::SetVariable(const char* name,float value)
{

}