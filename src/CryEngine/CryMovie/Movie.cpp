////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   movie.cpp
//  Version:     v1.00
//  Created:     23/4/2002 by Timur.
//  Compilers:   Visual C++ 7.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Movie.h"
#include "AnimSplineTrack.h"
#include "AnimSequence.h"
#include "SequenceIt.h"
#include "EntityNode.h"
#include "CVarNode.h"
#include "ScriptVarNode.h"
#include "AnimCameraNode.h"
#include "SceneNode.h"
#include "MaterialNode.h"
#include "EventNode.h"

#include <StlUtils.h>

#include <ISystem.h>
#include <ILog.h>
#include <IConsole.h>
#include <ITimer.h>
#include <IRenderer.h>

int CMovieSystem::m_mov_NoCutscenes = 0;
int CMovieSystem::m_mov_DebugEvents = 0;

//////////////////////////////////////////////////////////////////////////
CMovieSystem::CMovieSystem( ISystem *system )
{
	m_system = system;
	m_bRecording = false;
	m_pCallback=NULL;
	m_pUser=NULL;
	m_bPaused = false;
	m_bCutscenesPausedInEditor = true;
	m_bLastFrameAnimateOnStop = true;
	m_nProceduralAnimation=1; 
	m_lastGenId = 1;
	m_sequenceStopBehavior = ONSTOP_GOTO_END_TIME;
	m_lastUpdateTime.SetValue(0);
	m_bStartCapture = false;
	m_bEndCapture = false;
	m_fixedTimeStepBackUp = 0;
	m_cvar_capture_file_format = NULL;
	m_cvar_capture_frame_once = NULL;
	m_cvar_capture_folder = NULL;
	m_cvar_t_FixedStep = NULL;
	m_cvar_capture_frames = NULL;


	REGISTER_CVAR2( "mov_NoCutscenes",&m_mov_NoCutscenes,0,0,"Disable playing of Cut-Scenes" );
	system->GetIConsole()->Register( "mov_debugEvents",&m_mov_DebugEvents,0,0,"Enable debug output for Cinematic events" );
	REGISTER_COMMAND("mov_goToFrame", (ConsoleCommandFunc)GoToFrameCmd, 0, "Set specified sequence to given frame time.");
	m_mov_overrideCam = REGISTER_STRING("mov_overrideCam", "", VF_NULL, "Set the camera used for the sequence which overrides the camera track info in the sequence.");
	REGISTER_COMMAND("mov_useCam", (ConsoleCommandFunc)GoToFrameCmd, 0, "Set specified sequence to given frame time.");
}

//////////////////////////////////////////////////////////////////////////
CMovieSystem::~CMovieSystem()
{
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::Load(const char *pszFile, const char *pszMission)
{
	LOADING_TIME_PROFILE_SECTION(GetISystem());

	XmlNodeRef rootNode = m_system->LoadXmlFile(pszFile);
	if (!rootNode)
		return false;
	XmlNodeRef Node=NULL;
	for (int i=0;i<rootNode->getChildCount();i++)
	{
		XmlNodeRef missionNode=rootNode->getChild(i);
		XmlString sName;
		if (!(sName = missionNode->getAttr("Name")))
			continue;
		if (stricmp(sName.c_str(), pszMission))
			continue;
		Node=missionNode;
		break;
	}
	if (!Node)
		return false;
	Serialize(Node, true, true, false);
	return true;
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CMovieSystem::CreateTrack( EAnimTrackType type )
{
	switch (type)
	{
	case ATRACK_TCB_FLOAT:
		return new CTcbFloatTrack;
	case ATRACK_TCB_VECTOR:
		return new CTcbVectorTrack;
	case ATRACK_TCB_QUAT:
		return new CTcbQuatTrack;
	};
	//ATRACK_TCB_FLOAT,
	//ATRACK_TCB_VECTOR,
	//ATRACK_TCB_QUAT,
	//ATRACK_BOOL,
	// Unknown type of track.
//	CLogFile::WriteLine( "Error: Requesting unknown type of animation track!" );
	assert(0);
	return 0;
}

//////////////////////////////////////////////////////////////////////////
IAnimSequence* CMovieSystem::CreateSequence( const char *sequenceName )
{
	IAnimSequence *seq = new CAnimSequence( this );
	seq->SetName( sequenceName );
	m_sequences.push_back( seq );
	return seq;
}

//////////////////////////////////////////////////////////////////////////
IAnimSequence* CMovieSystem::LoadSequence( const char *pszFilePath )
{
	XmlNodeRef sequenceNode = m_system->LoadXmlFile( pszFilePath );
	if (sequenceNode)
	{
		return LoadSequence( sequenceNode );
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
IAnimSequence* CMovieSystem::LoadSequence( XmlNodeRef &xmlNode, bool bLoadEmpty )
{
	IAnimSequence *seq = new CAnimSequence( this );
	seq->Serialize( xmlNode,true,bLoadEmpty );
	// Delete previous sequence with the same name.
	IAnimSequence *pPrevSeq = FindSequence( seq->GetName() );
	if (pPrevSeq)
		RemoveSequence( pPrevSeq );
	m_sequences.push_back( seq );
	return seq;
}

//////////////////////////////////////////////////////////////////////////
IAnimSequence* CMovieSystem::FindSequence( const char *sequence )
{
	for (Sequences::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
	{
		IAnimSequence *seq = *it;
		if (stricmp(seq->GetName(),sequence) == 0)
		{
			return seq;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
ISequenceIt* CMovieSystem::GetSequences(bool bPlayingOnly/* =false */, bool bCutscenesOnly/* =false */)
{
	CSequenceIt *It=new CSequenceIt();
	if (bPlayingOnly == false)
	{
		for (Sequences::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
		{
			IAnimSequence* pSeq = *it;
			if (!bCutscenesOnly || (pSeq->GetFlags() & IAnimSequence::CUT_SCENE))
				It->add( pSeq );
		}
	}
	else
	{
		for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
		{
			IAnimSequence* pSeq = it->sequence;
			if (!bCutscenesOnly || (pSeq->GetFlags() & IAnimSequence::CUT_SCENE))
				It->add( pSeq );
		}
	}
	return It;
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::RemoveSequence( IAnimSequence *seq )
{
	assert( seq != 0 );
	if (seq)
	{
		IMovieCallback *pCallback=GetCallback();
		SetCallback(NULL);
		StopSequence(seq);

		for (Sequences::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
		{
			if (seq == *it)
			{
				m_movieListenerMap.erase(seq);
				m_sequences.erase(it);
				break;
			}
		}
		SetCallback(pCallback);
	}
}

//////////////////////////////////////////////////////////////////////////
int CMovieSystem::OnSequenceRenamed( const char *before, const char *after )
{
	int count = 0;
	// For every sequence,
	for (Sequences::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
	{
		// Find a director node, if any.
		for (int k=0; k<(*it)->GetNodeCount(); ++k)
		{
			IAnimNode *node = (*it)->GetNode(k);
			if (node->GetType() != ANODE_SCENE)
				continue;

			// If there is a director node, check whether it has a sequence track.
			IAnimTrack *track = node->GetTrackForParameter(APARAM_SEQUENCE);
			if (track)
			{
				for (int m=0; m<track->GetNumKeys(); ++m)
				{
					ISequenceKey seqKey;
					track->GetKey(m, &seqKey);
					// For each key that refers the sequence, update the name.
					if (stricmp(seqKey.szSelection, before) == 0)
					{
						strcpy_s(seqKey.szSelection, after);
						track->SetKey(m, &seqKey);
						++count;
					}
				}
			}
			break;
		}
	}
	
	return count;
}

//////////////////////////////////////////////////////////////////////////
int CMovieSystem::OnCameraRenamed( const char *before, const char *after )
{
	int count = 0;
	// For every sequence,
	for (Sequences::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
	{
		// Find a director node, if any.
		for (int k=0; k<(*it)->GetNodeCount(); ++k)
		{
			IAnimNode *node = (*it)->GetNode(k);

			if (node->GetType() != ANODE_SCENE)
				continue;

			// If there is a director node, check whether it has a camera track.
			IAnimTrack *track = node->GetTrackForParameter(APARAM_CAMERA);
			if (track)
			{
				for (int m=0; m<track->GetNumKeys(); ++m)
				{
					ISelectKey selKey;
					track->GetKey(m, &selKey);
					// For each key that refers the camera, update the name.
					if (stricmp(selKey.szSelection, before) == 0)
					{
						strcpy_s(selKey.szSelection, after);
						track->SetKey(m, &selKey);
						++count;
					}
				}
			}
			break;
		}
	}
	
	// For every sequence,
	for (Sequences::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
	{
		// Find camera nodes.
		for (int k=0; k<(*it)->GetNodeCount(); ++k)
		{
			IAnimNode *node = (*it)->GetNode(k);

			if (node->GetType() != ANODE_CAMERA)
				continue;

			// Update its name, if it's a corresponding one.
			if (stricmp(node->GetName(), before) == 0)
			{
				node->SetName(after);
			}
		}
	}

	return count;
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::RemoveAllSequences()
{
	m_bLastFrameAnimateOnStop = false;
	IMovieCallback *pCallback=GetCallback();
	SetCallback(NULL);
	StopAllSequences();
	m_sequences.clear();
	m_movieListenerMap.clear();
	SetCallback(pCallback);
	m_bLastFrameAnimateOnStop = true;
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::PlaySequence( const char *sequenceName,IAnimSequence *parentSeq,
																bool bResetFx,bool bTrackedSequence,float startTime,float endTime )
{
	IAnimSequence *seq = FindSequence(sequenceName);
	if (seq)
	{ 
		PlaySequence(seq,parentSeq,bResetFx,bTrackedSequence,startTime,endTime);
	}
	else
		gEnv->pLog->Log ("CMovieSystem::PlaySequence: Error: Sequence \"%s\" not found", sequenceName);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::PlaySequence( IAnimSequence *seq,IAnimSequence *parentSeq,
																bool bResetFx,bool bTrackedSequence,float startTime,float endTime )
{
	assert( seq != 0 );
	if (!seq || IsPlaying(seq))
		return;

	// disable procedural animations during cutscene as they are supposed
	// to be created by animators, store current status
	ICVar *pVar=gEnv->pConsole->GetCVar("ca_eyes_procedural");
	if (pVar)
	{
		m_nProceduralAnimation=pVar->GetIVal();
		pVar->Set((int)(0));
	}

	if ((seq->GetFlags() & IAnimSequence::CUT_SCENE) || (seq->GetFlags() & IAnimSequence::NO_HUD))
	{
		// Dont play cut-scene if this console variable set.
		if (m_mov_NoCutscenes != 0)
			return;
	}

	//gEnv->pLog->Log ("TEST: Playing Sequence (%s)", seq->GetName());

	// If this sequence is cut scene disable player.
	if (seq->GetFlags() & IAnimSequence::CUT_SCENE)
	{
		seq->SetParentSequence(parentSeq);

		if (!gEnv->pSystem->IsEditorMode() || !m_bCutscenesPausedInEditor)
		{
			if (m_pUser)
			{
				m_pUser->BeginCutScene(seq, seq->GetCutSceneFlags(),bResetFx);
			}
		}
	}

	seq->Activate();
	PlayingSequence ps;
	ps.sequence = seq;
	ps.startTime = startTime == -FLT_MAX ? seq->GetTimeRange().start : startTime;
	ps.currentTime = startTime == -FLT_MAX ? seq->GetTimeRange().start : startTime;
	ps.endTime = endTime == -FLT_MAX ? seq->GetTimeRange().end : endTime;
	ps.trackedSequence = bTrackedSequence;
	ps.bSingleFrame = false;
	m_playingSequences.push_back(ps);

	// tell all interested listeners
	NotifyListeners(seq,IMovieListener::MOVIE_EVENT_START);
}

void CMovieSystem::NotifyListeners(IAnimSequence* pSequence, IMovieListener::EMovieEvent event)
{
	TMovieListenerMap::iterator found (m_movieListenerMap.find(pSequence));
	if (found != m_movieListenerMap.end())
	{
		TMovieListenerVec::iterator iter ((*found).second.begin());
		while (iter != (*found).second.end()) {
			(*iter)->OnMovieEvent(event, pSequence);
			++iter;
		}
	}
	// 'NULL' ones are listeners interested in every sequence.
	TMovieListenerMap::iterator found2 (m_movieListenerMap.find((IAnimSequence*)0));
	if (found2 != m_movieListenerMap.end())
	{
		TMovieListenerVec::iterator iter ((*found2).second.begin());
		while (iter != (*found2).second.end()) {
			(*iter)->OnMovieEvent(event, pSequence);
			++iter;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::StopSequence( const char *sequenceName )
{
	IAnimSequence *seq = FindSequence(sequenceName);
	if (seq)
		return StopSequence(seq);
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::StopSequence( IAnimSequence *seq)
{
	return InternalStopSequence(seq, false);
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::InternalStopSequence( IAnimSequence *seq, bool bIsAbort /* = false */ )
{
	assert( seq != 0 );
	bool bRet = false;
	for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
	{
		if (it->sequence == seq)
		{
			m_playingSequences.erase( it );

			if (m_bLastFrameAnimateOnStop)
			{
				if (m_sequenceStopBehavior == ONSTOP_GOTO_END_TIME)
				{
					SAnimContext ac;
					ac.bSingleFrame = true;
					ac.time = seq->GetTimeRange().end;
					seq->Animate(ac);
				}
				else if (m_sequenceStopBehavior == ONSTOP_GOTO_START_TIME)
				{
					SAnimContext ac;
					ac.bSingleFrame = true;
					ac.time = seq->GetTimeRange().start;
					seq->Animate(ac);
				}
				seq->Deactivate();
			}
			
			// If this sequence is cut scene end it.
			if (seq->GetFlags() & IAnimSequence::CUT_SCENE)
			{
				/* AlexL: disabled until clarified with Timur
				// if it's a cutscene and it should not be animated to the end, deactivate it anyway
				if (m_bLastFrameAnimateOnStop == false)
				{
					seq->Deactivate();
				}
				*/

				if (!gEnv->pSystem->IsEditorMode() || !m_bCutscenesPausedInEditor)
				{
					if (m_pUser)
					{
						m_pUser->EndCutScene(seq, seq->GetCutSceneFlags(true));
					}
				}

				seq->SetParentSequence(NULL);
			}

			// tell all interested listeners
			NotifyListeners(seq, bIsAbort ? IMovieListener::MOVIE_EVENT_ABORTED : IMovieListener::MOVIE_EVENT_STOP);

			bRet = true;
			break;
		}
	}

	// restore procedural animations after cutscene is done	
	ICVar *pVar=gEnv->pConsole->GetCVar("ca_eyes_procedural");
	if (pVar)
		pVar->Set((int)(1));

	return bRet;
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::AbortSequence(IAnimSequence *seq, bool bLeaveTime/* =false  */)
{
	assert( seq != 0 );

	// check if it can be aborted
	if (seq->GetCutSceneFlags() & IAnimSequence::NO_ABORT)
		return false;

	m_bLastFrameAnimateOnStop = !bLeaveTime;
	bool bAborted = InternalStopSequence(seq, true);
	m_bLastFrameAnimateOnStop = true;
	return bAborted;
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::StopAllSequences()
{
	while (!m_playingSequences.empty())
	{
		StopSequence( m_playingSequences.begin()->sequence );
	}
	m_playingSequences.clear();
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::StopAllCutScenes()
{
	bool bAnyStoped;
	PlayingSequences::iterator next;
	do {
		bAnyStoped = false;
		for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); it = next)
		{
			next = it; ++next;
			IAnimSequence *seq = it->sequence;
			if (seq->GetFlags() & IAnimSequence::CUT_SCENE)
			{
				bAnyStoped = true;
				StopSequence( seq );
				break;
			}
		}
	} while (bAnyStoped);
}

//////////////////////////////////////////////////////////////////////////
bool CMovieSystem::IsPlaying( IAnimSequence *seq ) const
{
	for (PlayingSequences::const_iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
	{
		if (it->sequence == seq)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::Reset( bool bPlayOnReset,bool bSeekToStart )
{
	m_bLastFrameAnimateOnStop = false;
	StopAllSequences();
	m_bLastFrameAnimateOnStop = true;

	// Reset all sequences.
	for (Sequences::iterator sit = m_sequences.begin(); sit != m_sequences.end(); ++sit)
	{
		IAnimSequence *seq = *sit;
		NotifyListeners(seq,IMovieListener::MOVIE_EVENT_START);
		seq->Reset(bSeekToStart);
		NotifyListeners(seq,IMovieListener::MOVIE_EVENT_STOP);
	}

	// Force end Cut-Scene on the reset.
/*	if (m_pUser)	// lennert why is this here ??? if there was a cutscene playing it will be stopped above...
	{
		m_pUser->EndCutScene();
	}*/

	if (bPlayOnReset)
	{
		for (Sequences::iterator sit = m_sequences.begin(); sit != m_sequences.end(); ++sit)
		{
			IAnimSequence *seq = *sit;
			if (seq->GetFlags() & IAnimSequence::PLAY_ONRESET)
				PlaySequence(seq);
		}
	}

	// unpause the moviesystem
	m_bPaused = false;

	// Reset camera.
	SCameraParams CamParams=GetCameraParams();
	CamParams.cameraEntityId = 0;
	CamParams.fFOV=0;
	CamParams.justActivated = true;
	SetCameraParams(CamParams);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::PlayOnLoadSequences()
{
	for (Sequences::iterator sit = m_sequences.begin(); sit != m_sequences.end(); ++sit)
	{
		IAnimSequence *seq = *sit;
		if (seq->GetFlags() & IAnimSequence::PLAY_ONRESET)
			PlaySequence(seq);
	}

	// Reset camera.
	SCameraParams CamParams=GetCameraParams();
	CamParams.cameraEntityId = 0;
	CamParams.fFOV=0;
	CamParams.justActivated = true;
	SetCameraParams(CamParams);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::StillUpdate()
{
	if (!gEnv->bEditor)
		return;

	for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
	{
		PlayingSequence &ps = *it;

		ps.sequence->StillUpdate();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::ShowPlayedSequencesDebug()
{
	f32 green[4] = {0,1,0,1};
	f32 purple[4] = {1,0,1,1};
	f32 white[4] = {1,1,1,1};
	float y = 10.0f;
	std::vector<const char *> names;

	for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
	{
		PlayingSequence &ps = *it;
		
		if(ps.sequence == NULL)
			continue;

		gEnv->pRenderer->Draw2dLabel(1.0f, y, 1.3f, green, false, "Sequence %s : %f", ps.sequence->GetName(), ps.currentTime);
		y += 16.0f;
		for (int i=0; i<ps.sequence->GetNodeCount(); ++i)
		{
			// Checks nodes which happen to be in several sequences.
			// Those can be a bug, since several sequences may try to control the same entity. 
			const char *name = ps.sequence->GetNode(i)->GetName();
			bool alreadyThere = false;
			for (int k=0; k<names.size(); ++k)
			{
				if(strcmp(names[k], name) == 0)
				{
					alreadyThere = true;
					break;
				}
			}
			if(alreadyThere == false)
				names.push_back(name);
			gEnv->pRenderer->Draw2dLabel((21.0f+100.0f*i), ((i%2)?(y+8.0f):y), 1.0f, alreadyThere?white:purple, false, "%s", name);
		}
		y += 32.0f;
	}
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::Update( float dt )
{
	if (m_bPaused)
		return;

	// don't update more than once if dt==0.0
	CTimeValue curTime = gEnv->pTimer->GetFrameStartTime();
	if (dt == 0.0f && curTime == m_lastUpdateTime && !gEnv->IsEditor())
		return;
	m_lastUpdateTime = curTime;

	SAnimContext ac;
	float fps = 60;

	std::vector<IAnimSequence*> stopSequences;

	// cap delta time.
	//dt = MAX( 0,MIN(2.0f,dt) );

	for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
	{
		PlayingSequence &ps = *it;

		int nSeqFlags = ps.sequence->GetFlags();
		if ((nSeqFlags & IAnimSequence::CUT_SCENE) && m_mov_NoCutscenes != 0)
		{
			// Don't play cut-scene if no cutscenes console variable set.
			stopSequences.push_back(ps.sequence);
			break;
		}

		ac.time = ps.currentTime;
		ac.sequence = ps.sequence;
		ac.dt = dt;
		ac.fps = fps;

		// Increase play time.
		ps.currentTime += dt;

		// Check time out of range.
		if (ps.currentTime > ps.endTime)
		{
			int seqFlags = ps.sequence->GetFlags();
			if (seqFlags & IAnimSequence::ORT_LOOP)
			{
				// Time wrap's back to the start of the time range.
				ps.currentTime = ps.startTime;
			}
			else if (seqFlags & IAnimSequence::ORT_CONSTANT)
			{
				// Time just continues normally past the end of time range.
			}
			else
			{
				// If no out-of-range type specified sequence stopped when time reaches end of range.
				// Que sequence for stopping.
				if (ps.trackedSequence == false)
				{
					stopSequences.push_back(ps.sequence);
				}
				continue;
			}
		}

		ac.bSingleFrame = ps.bSingleFrame;
		if(ps.bSingleFrame)
			ps.bSingleFrame = false;
		// Animate sequence. (Can invalidate iterator)
		ps.sequence->Animate( ac );
	}

	if(m_mov_DebugEvents)
	{
		ShowPlayedSequencesDebug();		
	}

	// Stop queued sequences.
	for (int i = 0; i < (int)stopSequences.size(); i++)
	{
		StopSequence( stopSequences[i] );
	}
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::Callback( IMovieCallback::ECallbackReason Reason,IAnimNode *pNode )
{
	if (m_pCallback)
		m_pCallback->OnMovieCallback( Reason,pNode );
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::Serialize( XmlNodeRef &xmlNode,bool bLoading,bool bRemoveOldNodes,bool bLoadEmpty )
{
	if (bLoading)
	{
		//////////////////////////////////////////////////////////////////////////
		// Load sequences from XML.
		//////////////////////////////////////////////////////////////////////////
		XmlNodeRef seqNode=xmlNode->findChild("SequenceData");
		if (seqNode)
		{
			RemoveAllSequences();

			for (int i=0;i<seqNode->getChildCount();i++)
			{
				XmlNodeRef childNode = seqNode->getChild(i);
				if (!LoadSequence(childNode, bLoadEmpty))
					return;
			}
		}
	}else
	{
		XmlNodeRef sequencesNode=xmlNode->newChild("SequenceData");
		ISequenceIt *It=GetSequences();
		IAnimSequence *seq=It->first();;
		while (seq)
		{
			XmlNodeRef sequenceNode=sequencesNode->newChild("Sequence");
			seq->Serialize(sequenceNode, false);
			seq=It->next();
		}
		It->Release();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::SetCameraParams( const SCameraParams &Params )
{
	m_ActiveCameraParams = Params;
	if (m_pUser)
		m_pUser->SetActiveCamera(m_ActiveCameraParams);
	if (m_pCallback)
		m_pCallback->OnSetCamera( m_ActiveCameraParams );
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::SendGlobalEvent( const char *pszEvent )
{
	if (m_pUser)
		m_pUser->SendGlobalEvent(pszEvent);
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::Pause()
{
	if (m_bPaused)
		return;
	m_bPaused = true;

	/*
	PlayingSequences::iterator next;
	for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); it = next)
	{
		next = it; ++next;
		PlayingSequence &ps = *it;
		ps.sequence->Pause();
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::Resume()
{
	if (!m_bPaused)
		return;

	m_bPaused = false;

	/*
	PlayingSequences::iterator next;
	for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); it = next)
	{
		next = it; ++next;
		PlayingSequence &ps = *it;
		ps.sequence->Resume();
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::PauseCutScenes()
{
	m_bCutscenesPausedInEditor = true;

	if (m_pUser != NULL)
	{
		for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
		{
			if (it->sequence->GetFlags() & IAnimSequence::CUT_SCENE)
			{
				m_pUser->EndCutScene(it->sequence,it->sequence->GetCutSceneFlags(true));
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::ResumeCutScenes()
{
	if (m_mov_NoCutscenes != 0)
		return;

	m_bCutscenesPausedInEditor = false;

	if (m_pUser != NULL)
	{
		for (PlayingSequences::iterator it = m_playingSequences.begin(); it != m_playingSequences.end(); ++it)
		{
			if (it->sequence->GetFlags() & IAnimSequence::CUT_SCENE)
			{
				m_pUser->BeginCutScene(it->sequence,it->sequence->GetCutSceneFlags(),true);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CMovieSystem::OnPlaySound( IAnimSequence* pSeq, ISound *pSound )
{
	if (pSeq->GetFlags() & IAnimSequence::CUT_SCENE)
	{
		if (m_pUser)
			m_pUser->PlaySubtitles( pSeq, pSound );
	}
}

float CMovieSystem::GetPlayingTime(IAnimSequence * pSeq)
{
	if (!pSeq)
		return -1;

	if (!IsPlaying(pSeq))
		return -1;

	PlayingSequences::const_iterator itend = m_playingSequences.end();
	for (PlayingSequences::const_iterator it = m_playingSequences.begin(); it != itend; ++it)
	{
		if (it->sequence == pSeq)
			return it->currentTime;
	}

	return -1;
}

bool CMovieSystem::SetPlayingTime(IAnimSequence * pSeq, float fTime)
{
	if (!pSeq)
		return false;

	if (!IsPlaying(pSeq))
		return false;

	PlayingSequences::iterator itend = m_playingSequences.end();
	for (PlayingSequences::iterator it = m_playingSequences.begin(); it != itend; ++it)
	{
		if (it->sequence == pSeq && !(pSeq->GetFlags() & IAnimSequence::NO_SEEK))
		{
			it->currentTime = fTime;
			it->bSingleFrame = true;
		}
	}


	return false;
}

bool CMovieSystem::GetStartEndTime(IAnimSequence *pSeq,float &fStartTime,float &fEndTime)
{
	fStartTime = 0.0f;
	fEndTime = 0.0f;

	if (!pSeq)
		return false;

	if (!IsPlaying(pSeq))
		return false;

	PlayingSequences::iterator itend = m_playingSequences.end();
	for (PlayingSequences::iterator it = m_playingSequences.begin(); it != itend; ++it)
	{
		if (it->sequence == pSeq)
		{
			fStartTime = it->startTime;
			fEndTime = it->endTime;
		}
	}

	return false;
}

bool CMovieSystem::SetStartEndTime(IAnimSequence *pSeq,const float fStartTime,const float fEndTime)
{
	if (!pSeq)
		return false;

	if (!IsPlaying(pSeq))
		return false;

	PlayingSequences::iterator itend = m_playingSequences.end();
	for (PlayingSequences::iterator it = m_playingSequences.begin(); it != itend; ++it)
	{
		if (it->sequence == pSeq)
		{
			it->startTime = fStartTime;
			it->endTime = fEndTime;
		}
	}

	return false;
}

void CMovieSystem::SetSequenceStopBehavior( ESequenceStopBehavior behavior )
{
	m_sequenceStopBehavior = behavior;
}

IMovieSystem::ESequenceStopBehavior CMovieSystem::GetSequenceStopBehavior()
{
  return m_sequenceStopBehavior;
}


bool CMovieSystem::AddMovieListener(IAnimSequence* pSequence, IMovieListener* pListener)
{
	assert (pListener != 0);
	if (pSequence != NULL
	&& std::find(m_sequences.begin(), m_sequences.end(), pSequence) == m_sequences.end())
	{
		gEnv->pLog->Log ("CMovieSystem::AddMovieListener: Sequence %p unknown to CMovieSystem", pSequence);
		return false;
	}
	return stl::push_back_unique(m_movieListenerMap[pSequence], pListener);
}

bool CMovieSystem::RemoveMovieListener(IAnimSequence* pSequence, IMovieListener* pListener)
{
	assert (pListener != 0);
	if (pSequence != NULL
	&& std::find(m_sequences.begin(), m_sequences.end(), pSequence) == m_sequences.end())
	{
		gEnv->pLog->Log ("CMovieSystem::AddMovieListener: Sequence %p unknown to CMovieSystem", pSequence);
		return false;
	}
	return stl::find_and_erase(m_movieListenerMap[pSequence], pListener);
}

void CMovieSystem::GoToFrameCmd(IConsoleCmdArgs *pArgs)
{
	if(pArgs->GetArgCount() < 3)
	{
		gEnv->pLog->LogError("GoToFrame failed! You should provide two arguments of 'sequence name' & 'frame time'.");
		return;
	}

	const char *seqName = pArgs->GetArg(1);
	float targetFrame = (float)atof(pArgs->GetArg(2));

	((CMovieSystem*)gEnv->pMovieSystem)->GoToFrame(seqName, targetFrame);

}

void CMovieSystem::GoToFrame( const char * seqName, float targetFrame )
{
	assert(seqName != NULL);

	if(gEnv->IsEditor() && gEnv->bEditorGameMode == false)
	{
		string editorCmd;
		editorCmd.Format("mov_goToFrameEditor %s %f", seqName , targetFrame);
		gEnv->pConsole->ExecuteString(editorCmd.c_str());
		return;
	}

	for (PlayingSequences::iterator it = m_playingSequences.begin();
		it != m_playingSequences.end(); ++it)
	{
		PlayingSequence &ps = *it;

		if(strcmp(ps.sequence->GetName(), seqName) == 0)
		{
			assert(ps.sequence->GetTimeRange().start <= targetFrame && targetFrame <= ps.sequence->GetTimeRange().end);
			ps.currentTime = targetFrame;
			ps.bSingleFrame = true;
			break;
		}
	}
}

void CMovieSystem::StartCapture(const ICaptureKey& key)
{
	m_bStartCapture = true;
	m_captureKey = key;
}

void CMovieSystem::EndCapture()
{
	m_bEndCapture = true;
}

void CMovieSystem::ControlCapture()
{
	bool bothStartAndEnd = m_bStartCapture && m_bEndCapture;
	assert(bothStartAndEnd == false);

	bool allCVarsReady
		= m_cvar_capture_file_format && m_cvar_capture_frame_once 
			&& m_cvar_capture_folder && m_cvar_t_FixedStep && m_cvar_capture_frames;
	if (allCVarsReady == false)
	{
		m_cvar_capture_file_format = gEnv->pConsole->GetCVar("capture_file_format");
		m_cvar_capture_frame_once = gEnv->pConsole->GetCVar("capture_frame_once");
		m_cvar_capture_folder = gEnv->pConsole->GetCVar("capture_folder");
		m_cvar_t_FixedStep = gEnv->pConsole->GetCVar("t_FixedStep");
		m_cvar_capture_frames = gEnv->pConsole->GetCVar("capture_frames");
		m_cvar_capture_file_prefix = gEnv->pConsole->GetCVar("capture_file_prefix");
		m_cvar_capture_misc_render_buffers = gEnv->pConsole->GetCVar("capture_misc_render_buffers");
	}

	allCVarsReady
		= m_cvar_capture_file_format && m_cvar_capture_frame_once 
		&& m_cvar_capture_folder && m_cvar_t_FixedStep && m_cvar_capture_frames
		&& m_cvar_capture_file_prefix && m_cvar_capture_misc_render_buffers;
	assert(allCVarsReady);
	
	if (allCVarsReady == false)
	{
		m_bStartCapture = m_bEndCapture = false;
		return;
	}

	if(m_bStartCapture)
	{
		m_cvar_capture_file_format->Set(m_captureKey.GetFormat());
		m_cvar_capture_frame_once->Set(m_captureKey.once?1:0);
		m_cvar_capture_folder->Set(m_captureKey.folder);
		m_cvar_capture_file_prefix->Set(m_captureKey.prefix);
		m_cvar_capture_misc_render_buffers->Set(m_captureKey.bufferToCapture);

		m_fixedTimeStepBackUp = m_cvar_t_FixedStep->GetFVal();
		m_cvar_t_FixedStep->Set(m_captureKey.timeStep);
		m_cvar_capture_frames->Set(1);

		m_bStartCapture = false;
	}

	if(m_bEndCapture)
	{
		m_cvar_capture_frames->Set(0);
		m_cvar_t_FixedStep->Set(m_fixedTimeStepBackUp);

		m_bEndCapture = false;
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(IMovieSystem)
