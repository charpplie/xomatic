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

#ifndef __testprofiler_h__
#define __testprofiler_h__
#pragma once

#include "ITestModule.h"
#include "TestManager.h"

class CTestProfiler : public ITestModule, IFrameProfilePeakCallback
{
public:
	CTestProfiler( ISystem *pSystem ,const CTestManager* pTestManager);
	~CTestProfiler();
	
	virtual void StartSession();
	virtual void StopSession();
	virtual void PreUpdate();
	virtual void Update();
	virtual float RenderInfo(float y=0);
	virtual void Record( bool bEnable );
	virtual void Play( bool bEnable );
	virtual ETestModuleType GetType() const {return TM_TIMEDEMO;}
	virtual void ParseParams(XmlNodeRef node);
	virtual void SetVariable(const char* name,const char* szValue);
	virtual void SetVariable(const char* name,float value);
	virtual int GetNumberOfFrames() {return 0;}
	virtual int GetTotalPolysRecorded() {return 0;}

	bool IsRecording() const { return m_bRecording; };
	bool IsPlaying() const { return m_bPlaying; };

	//! Get number of frames in record.
	int GetNumFrames() const;
	float GetAverageFrameRate() const;

	void Save( const char *filename );
	bool Load(  const char *filename );
	
	//void StartChainDemo( const char *levelsListFilename );

	//////////////////////////////////////////////////////////////////////////
	// Implements IFrameProfilePeakCallback interface.
	//////////////////////////////////////////////////////////////////////////
	virtual void OnFrameProfilerPeak( CFrameProfiler *pProfiler,float fPeakTime );
	//////////////////////////////////////////////////////////////////////////


	virtual void StartRecording( IConsoleCmdArgs *pArgs );
	virtual void StopRecording( IConsoleCmdArgs *pArgs );
	virtual void PlayInit( IConsoleCmdArgs *pArgs ) {};
	virtual void Pause(bool paused) {m_bPaused = paused;}
	virtual void Stop( IConsoleCmdArgs *pArgs ) {};

	void GetMemoryStatistics(ICrySizer * s);
	virtual bool RecordFrame();
	virtual bool PlayFrame();
	virtual void Restart();
	virtual void EndLog();

private:


	CTimeValue GetTime();
	// Set Value of console variable.
	void SetConsoleVar( const char *sVarName,float value );
	// Get value of console variable.
	float GetConsoleVar( const char *sVarName );

	int ComputePolyCount();

	//void ResetSessionLoop();
	//void StartNextChainedLevel();

	void LogInfo( const char *format,... ) PRINTF_PARAMS(2, 3);


private:

	const CTestManager* m_pTestManager;

	bool m_bRecording;
	bool m_bPlaying;
	bool m_bPaused;
	bool m_bDemoFinished;

	//! Current play or record frame.
	int m_currentFrame;


	//std::vector<string> m_demoLevels;
	//int m_nCurrentDemoLevel;

	//////////////////////////////////////////////////////////////////////////
	// Old values of console vars.
	//////////////////////////////////////////////////////////////////////////
	float m_oldPeakTolerance;

	//! Timings.
	
	//CTimeValue m_recordStartTime;
	//CTimeValue m_recordEndTime;
	//CTimeValue m_recordLastFrameTime;
	CTimeValue m_lastFrameTime;
	//CTimeValue m_totalDemoTime;
	//CTimeValue m_recordedDemoTime;
	
	// How many polygons per frame where recorded.
	int m_nTotalPolysRecorded;
	// How many polygons per frame where played.
	int m_nTotalPolysPlayed;
	
	float m_lastPlayedTotalTime;
	float m_lastAveFrameRate;
	float m_minFPS;
	float m_maxFPS;
	float m_currFPS;
	float m_sumFPS ;
	static int m_minFPSCounter;
	int m_minFPS_Frame;
	int m_maxFPS_Frame;


	int m_nCurrPolys;
	int m_nMaxPolys;
	int m_nMinPolys;
	int m_nPolysPerSec;
	int m_nPolysCounter;
	
	// For calculating current last second fps.
	CTimeValue m_lastFpsTimeRecorded;
	int m_fpsCounter;

//	int m_numLoops;
//	int m_maxLoops;
	struct STimeDemoInfo *m_pTimeDemoInfo;

	bool m_bPlayingTimeDemo;

	bool m_bEnabledProfiling, m_bVisibleProfiling;

	string m_file;

	ISystem *m_pSystem;

	//cached console variables
	float m_demo_noinfo;
	float m_demoProfile;


};

#endif // __testprofiler_h__
