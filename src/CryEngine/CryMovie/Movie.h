////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   movie.h
//  Version:     v1.00
//  Created:     23/4/2002 by Timur.
//  Compilers:   Visual C++ 7.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __movie_h__
#define __movie_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "IMovieSystem.h"
#include <CrySizer.h>

/**	This is descirption of currently playing sequence.
*/
struct PlayingSequence
{
	//! Sequence playnig.
	_smart_ptr<IAnimSequence> sequence;
	//! Start/End/Current playing time for this sequence.
	float startTime;
	float endTime;
	float currentTime;
	//! Sequence from other sequence's sequence track
	bool trackedSequence;
	bool bSingleFrame;

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(sequence);
	}

};

struct IConsoleCmdArgs;

class CMovieSystem : public IMovieSystem
{
public:
	CMovieSystem( ISystem *system );
	~CMovieSystem();

	void Release() { delete this; };

	void SetUser(IMovieUser *pUser) { m_pUser=pUser; }
	IMovieUser* GetUser() { return m_pUser; }

	bool Load(const char *pszFile, const char *pszMission);

	ISystem* GetSystem() { return m_system; }

	IAnimTrack* CreateTrack( EAnimTrackType type );

	IAnimSequence* CreateSequence( const char *sequence );
	IAnimSequence* LoadSequence( const char *pszFilePath );
	IAnimSequence* LoadSequence( XmlNodeRef &xmlNode, bool bLoadEmpty=true );

	void RemoveSequence( IAnimSequence *seq );
	IAnimSequence* FindSequence( const char *sequence );
	ISequenceIt* GetSequences(bool bPlayingOnly=false, bool bCutscenesOnly=false);

	int OnSequenceRenamed( const char *before, const char *after );
	int OnCameraRenamed( const char *before, const char *after );

	bool AddMovieListener(IAnimSequence* pSequence, IMovieListener* pListener);
	bool RemoveMovieListener(IAnimSequence* pSequence, IMovieListener* pListener);

	void RemoveAllSequences();

	//////////////////////////////////////////////////////////////////////////
	// Sequence playback.
	//////////////////////////////////////////////////////////////////////////
	void PlaySequence( const char *sequence,IAnimSequence *parentSeq=NULL,bool bResetFX=true,
											bool bTrackedSequence=false,float startTime = -FLT_MAX,float endTime = -FLT_MAX );
	void PlaySequence( IAnimSequence *seq,IAnimSequence *parentSeq=NULL,bool bResetFX=true,
											bool bTrackedSequence=false,float startTime = -FLT_MAX,float endTime = -FLT_MAX );
	void PlayOnLoadSequences();

	bool StopSequence( const char *sequence );
	bool StopSequence( IAnimSequence *seq);
	bool AbortSequence(IAnimSequence *seq, bool bLeaveTime=false);

	void StopAllSequences();
	void StopAllCutScenes();
	void Pause( bool bPause );

	void Reset( bool bPlayOnReset,bool bSeekAllToStart );
	void StillUpdate();
	void Update( float dt );

	void StartCapture(const ICaptureKey& key);
	void EndCapture();
	void ControlCapture();

	bool IsPlaying( IAnimSequence *seq ) const;

	void Pause();
	void Resume();

	VIRTUAL void PauseCutScenes();
	VIRTUAL void ResumeCutScenes();

	void SetRecording( bool recording ) { m_bRecording = recording; };
	bool IsRecording() const { return m_bRecording; };

	void SetCallback( IMovieCallback *pCallback ) { m_pCallback=pCallback; }
	IMovieCallback* GetCallback() { return m_pCallback; }
	void Callback( IMovieCallback::ECallbackReason Reason,IAnimNode *pNode );

	void Serialize( XmlNodeRef &xmlNode,bool bLoading, bool bRemoveOldNodes=false, bool bLoadEmpty=true );

	const SCameraParams& GetCameraParams() const { return m_ActiveCameraParams; }
	void SetCameraParams( const SCameraParams &Params );

	void SendGlobalEvent( const char *pszEvent );
	void OnPlaySound( IAnimSequence* pSeq, ISound *pSound );
	void SetSequenceStopBehavior( ESequenceStopBehavior behavior );
	IMovieSystem::ESequenceStopBehavior GetSequenceStopBehavior();

	float GetPlayingTime(IAnimSequence * pSeq);
	bool SetPlayingTime(IAnimSequence * pSeq, float fTime);

	bool GetStartEndTime(IAnimSequence *pSeq,float &fStartTime,float &fEndTime);
	bool SetStartEndTime(IAnimSequence *pSeq,const float fStartTime,const float fEndTime);

	void GoToFrame( const char * seqName, float targetFrame );

	const char * GetOverrideCamName() const 
	{ return m_mov_overrideCam->GetString(); }

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this,sizeof(*this));
		pSizer->AddObject(m_sequences);
		pSizer->AddObject(m_playingSequences);
		pSizer->AddObject(m_movieListenerMap);
	}
protected:
	void NotifyListeners(IAnimSequence* pSequence, IMovieListener::EMovieEvent event);
	bool InternalStopSequence( IAnimSequence *seq, bool bIsAbort=false);

	static void GoToFrameCmd(IConsoleCmdArgs *pArgs);
private:
	ISystem*	m_system;

	IMovieUser *m_pUser;
	IMovieCallback *m_pCallback;

	CTimeValue m_lastUpdateTime;
	int m_lastGenId;

	typedef std::vector<_smart_ptr<IAnimSequence> > Sequences;
	Sequences m_sequences;

	typedef std::list<PlayingSequence> PlayingSequences;
	PlayingSequences m_playingSequences;

	typedef std::vector<IMovieListener*> TMovieListenerVec;
	typedef std::map<IAnimSequence*, TMovieListenerVec> TMovieListenerMap;

	// a container which maps sequences to all interested listeners
	// listeners is a vector (could be a set in case we have a lot of listeners, stl::push_back_unique!)
	TMovieListenerMap m_movieListenerMap;

	bool	m_bRecording;
	bool	m_bPaused;
	bool	m_bCutscenesPausedInEditor;
	int		m_nProceduralAnimation;

	bool m_bLastFrameAnimateOnStop;

	SCameraParams m_ActiveCameraParams;

	ESequenceStopBehavior m_sequenceStopBehavior;

	bool m_bStartCapture;
	bool m_bEndCapture;
	ICaptureKey m_captureKey;
	float m_fixedTimeStepBackUp;
	ICVar *m_cvar_capture_file_format;
	ICVar *m_cvar_capture_frame_once;
	ICVar *m_cvar_capture_folder;
	ICVar *m_cvar_t_FixedStep;
	ICVar *m_cvar_capture_frames;
	ICVar *m_cvar_capture_file_prefix;
	ICVar *m_cvar_capture_misc_render_buffers;

	static int m_mov_NoCutscenes;
	ICVar *m_mov_overrideCam;

	void ShowPlayedSequencesDebug();
public:
	static int m_mov_DebugEvents;
};

#endif // __movie_h__
