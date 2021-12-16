////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   TestManager.h
//  Version:     v1.00
//  Created:     3/3/2008 by Luciano Morpurgo.
//  Compilers:   Visual Studio.NET
//  Description: The container and manager class for all Test Methods
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __testmanager_h__
#define __testmanager_h__
#pragma once

#include "ITestModule.h"

#define MAX_FILE_NAME 256

class CTestManager 
{
public:
	CTestManager( ISystem *pSystem );
	~CTestManager();
	
	virtual void PreUpdate();
	virtual void Update();
	//virtual void OnGameplayEvent(IEntity *pEntity, const GameplayEvent &event);

	void Record( bool bEnable );
	void Play( bool bEnable );
	void PlayInit( IConsoleCmdArgs *pArgs);
	void Pause(bool bPaused);

	bool IsRecording() const { return m_bRecording; };
	bool IsPlaying() const { return m_bPlaying; };
	virtual void GetMemoryStatistics(ICrySizer * s);
	void StartChainDemo( const char *levelsListFilename );
	bool ParseXmlFile(const char* szFileName);

	// console commands
	static void cmd_StartRecordingTimeDemo( IConsoleCmdArgs *pArgs );
	static void cmd_Play( IConsoleCmdArgs *pArgs );
	static void cmd_StartDemoChain( IConsoleCmdArgs *pArgs );
	static void cmd_Stop( IConsoleCmdArgs *pArgs );
	void PreStartSession();
	void StartSession();
	void StopSession();
	ITestModule* SetTestModule(ETestModuleType tmType, bool enable = true);
	ITestModule* GetTestModule(ETestModuleType tmType) const;
	int GetNumberOfFrames() const ;
	int GetTotalPolysRecorded() const;
	virtual int GetCurrentFrame() const {return m_currentFrame;}
	CTimeValue GetTotalTime() const {return m_totalDemoTime;}

	inline bool IsAIEnabled() const {return m_bAIEnabled;}
	inline bool IsInfoEnabled() const {return m_demo_noinfo <=0;}
	bool IsTimeDemo() const;
	void SetDemoFixedTimeStep(uint16 value);
	inline uint16 GetDemoFixedTimeStep() {return m_demo_fixed_timestep;}
	// interface IConsoleVarSink 
	//virtual bool OnBeforeVarChange( ICVar *pVar,const char *sNewValue );
	//virtual void OnAfterVarChange( ICVar *pVar ) {};

private:
	static const char* GetCurrentLevelPath();
	static const char* GetInitSaveFileName();

	float GetAverageFrameRate() const;

	CTimeValue GetTime();
	// Set Value of console variable.
	template<class T> void SetConsoleVar( const char *sVarName,T value );
	// Get value of console variable.
	float GetConsoleVar( const char *sVarName );

	void ResetSessionLoop();
	void StartNextChainedLevel();
	void RenderInfo();
	void SetVariable(const char* name,const char* szValue);
	void SetVariable(const char* name,float value);

//	ITestModule* SetTestModule(XmlNodeRef pNode);

	
	void ParseParams(XmlNodeRef pNode);

	void ReleaseTestModules();

	void EndLog();
	void LogInfo( const char *format,... ) PRINTF_PARAMS(2, 3);

private:

	static CTestManager* s_pTestManager;
	static char s_fileName[MAX_FILE_NAME];
	bool m_bRecording;
	bool m_bPlaying;
	bool m_bPaused;
	bool m_bDemoFinished;

	//! Current play or record frame.
	int m_currentFrame;

	std::vector<string> m_demoLevels;
	int m_nCurrentDemoLevel;

	//typedef std::map<ETestModuleType,string> TestModuleTypesMap;
	//TestModuleTypesMap m_TestModuleTypes;

	// Test Methods container
	typedef std::map<ETestModuleType,ITestModule*> TestModuleMap;
	TestModuleMap m_TestModules;

	//////////////////////////////////////////////////////////////////////////
	// Old values of console vars.
	//////////////////////////////////////////////////////////////////////////
	float m_fixedTimeStep;
	//float m_oldPeakTolerance;
	int m_prevGodMode;
	int m_countDownPlay;

	//! Timings.
	//CTimeValue m_recordStartTime;
	//CTimeValue m_recordEndTime;
	//CTimeValue m_recordLastFrameTime;
	CTimeValue m_lastFrameTime;
	CTimeValue m_totalDemoTime;
	//CTimeValue m_recordedDemoTime;

	//float m_lastAveFrameRate;
	//float m_lastPlayedTotalTime;

	int m_numLoops;
	int m_maxLoops;

	int m_demo_scroll_pause;
	int m_demo_noinfo;
	int m_demo_quit;
	int m_demo_screenshot_frame;
	int m_demo_max_frames;
	int m_demo_savestats;
	int m_demo_ai;
	int m_demo_restart_level;
	int m_demo_panoramic;
	int m_demo_fixed_timestep;
	int m_demo_vtune;
	int m_demoProfile;
	int m_demo_time_of_day;

	//bool m_bEnabledProfiling, m_bVisibleProfiling;
	bool m_bAIEnabled;

	string m_file;

	ISystem *m_pSystem;
};

#endif // __testmanager_h__
