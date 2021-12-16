////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   timedemorecorder.h
//  Version:     v1.00
//  Created:     2/8/2003 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __timedemorecorder_h__
#define __timedemorecorder_h__
#pragma once

#include "ITestModule.h"
#include "TestManager.h"

struct SRecordedGameEvent;

struct SGameEvent
{
	string entityName;
	uint8 gameEventType;
	string description;
	string description2;
	float				value;
	int				extra;
 
	SGameEvent() {};
	SGameEvent(IEntity* pEntity, const GameplayEvent& event)
	{	
		if(pEntity)
			entityName = pEntity->GetName();
		gameEventType = event.event;
		value = event.value;
		description = event.description;
		description2 = (const char*)(event.extra);
	};

	SGameEvent(const SRecordedGameEvent& event);

};
typedef std::vector<SGameEvent> TGameEventRecords;

class CTimeDemoRecorder : public ITestModule, /*IFrameProfilePeakCallback,*/ IInputEventListener, IEntitySystemSink, IGameplayListener
{
public:
	CTimeDemoRecorder( ISystem *pSystem , CTestManager* pTestManager);
	~CTimeDemoRecorder();
	
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
	//virtual void OnFrameProfilerPeak( CFrameProfiler *pProfiler,float fPeakTime );
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Implements IInputEventListener interface.
	//////////////////////////////////////////////////////////////////////////
	virtual bool OnInputEvent( const SInputEvent &event );
	//////////////////////////////////////////////////////////////////////////
	// Implements IGameplayListener interface.
	//////////////////////////////////////////////////////////////////////////
	virtual void OnGameplayEvent(IEntity *pEntity, const GameplayEvent &event);

	//////////////////////////////////////////////////////////////////////////
	// IEntitySystemSink
	//////////////////////////////////////////////////////////////////////////
	virtual bool OnBeforeSpawn( SEntitySpawnParams &params );
	virtual void OnSpawn( IEntity *pEntity,SEntitySpawnParams &params );
	virtual bool OnRemove( IEntity *pEntity );
	virtual void OnEvent( IEntity *pEntity, SEntityEvent &event );
	//////////////////////////////////////////////////////////////////////////

	virtual void StartRecording( IConsoleCmdArgs *pArgs );
	virtual void StopRecording( IConsoleCmdArgs *pArgs );
//	virtual void Play( IConsoleCmdArgs *pArgs );
	virtual void PlayInit( IConsoleCmdArgs *pArgs );

	virtual void Pause(bool paused) {m_bPaused = paused;}
	virtual void Stop( IConsoleCmdArgs *pArgs );

	void GetMemoryStatistics(ICrySizer * s);
	virtual bool RecordFrame();
	virtual bool PlayFrame();
	virtual void Restart();
	virtual int GetNumberOfFrames();
	virtual int GetTotalPolysRecorded() {return m_nTotalPolysRecorded;}
	virtual void EndLog();

private:

	const char* GetDescriptionFromCache(const char* desc);

	struct EntityEventRecord
	{
		EntityId   entityId;      // What entity performed event.
		EntityGUID guid;          // What entity performed event.

		uint32 eventType;         // What event.
		uint64 nParam[4];         // event params.
		Vec3 pos;
		Quat q;

		enum Flags
		{
			HIDDEN = BIT(1),
		};

		// Special flags.
		uint32 flags;
	};


	static const char* GetCurrentLevelPath();

	CTimeValue GetTime();
	// Set Value of console variable.
	void SetConsoleVar( const char *sVarName,float value );
	// Get value of console variable.
	float GetConsoleVar( const char *sVarName );

	//void ResetSessionLoop();
	//void StartNextChainedLevel();

	void LogInfo( const char *format,... ) PRINTF_PARAMS(2, 3);

	void PlayBackEntityEvent( const EntityEventRecord &rec );
	void SaveAllEntitiesState();
	void RestoreAllEntitiesState();
private:
	// Input event list.
	typedef std::vector<SInputEvent> InputEventsList;
	typedef std::vector<EntityEventRecord> EntityEventRecords;

	//! This sructure saved for every frame of time demo.
	struct FrameRecord
	{
		Vec3 playerPos;
		Ang3 cameraAngles;
		Quat playerRotation;
		float frameTime; // Immidiate frame rate, for this frame.
		
		// Snapshot of current processing command.
		unsigned int nActionFlags[2];
		float fLeaning;
		int nPolygons; // Polys rendered in this frame.

		InputEventsList inputEventsList;
		EntityEventRecords entityEvents;
		TGameEventRecords gameEvents;

	};
	typedef std::vector<FrameRecord> FrameRecords;
	FrameRecords m_records;

	bool m_bRecording;
	bool m_bPlaying;
	bool m_bPaused;
	bool m_bDemoFinished;

	//! Current play or record frame.
	int m_currentFrame;
	FrameRecords::iterator m_currFrameIter;
	
	std::vector<SInputEvent> m_currentFrameInputEvents;
	EntityEventRecords m_currentFrameEntityEvents;
	EntityEventRecords m_firstFrameEntityState;


	TGameEventRecords m_CurrentFrameGameEvents;

	typedef std::vector<string> TDescriptionContainer;
	TDescriptionContainer m_Descriptions;

	//////////////////////////////////////////////////////////////////////////
	// Old values of console vars.
	//////////////////////////////////////////////////////////////////////////

	//! Timings.
	CTimeValue m_recordStartTime;
	CTimeValue m_recordEndTime;
	//CTimeValue m_recordLastFrameTime;
	CTimeValue m_lastFrameTime;
	CTimeValue m_totalDemoTime;
	CTimeValue m_recordedDemoTime;

	// How many polygons per frame where recorded.
	int m_nTotalPolysRecorded;
	// How many polygons per frame where played.
	//int m_nTotalPolysPlayed;
	
	//float m_lastPlayedTotalTime;
	//float m_lastAveFrameRate;
	//float m_minFPS;
	//float m_maxFPS;
	//float m_currFPS;
	

	//int m_minFPS_Frame;
	//int m_maxFPS_Frame;

/*
	int m_nCurrPolys;
	int m_nMaxPolys;
	int m_nMinPolys;
	int m_nPolysPerSec;
	int m_nPolysCounter;
*/
	// For calculating current last second fps.
	CTimeValue m_lastFpsTimeRecorded;
	//int m_fpsCounter;

//	int m_numLoops;
//	int m_maxLoops;
/*
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
*/
	int m_demo_gameState;
	int m_fileVersion;
	//bool m_bEnabledProfiling, m_bVisibleProfiling;

	string m_file;

	ISystem *m_pSystem;

	CTestManager* m_pTestManager;
//	IGameStateRecorder* m_pGameStateRecorder;

	//struct STimeDemoInfo *m_pTimeDemoInfo;

	static ICVar *s_timedemo_file;
	static CTimeDemoRecorder *s_pTimeDemoRecorder;
};

#endif // __timedemorecorder_h__
