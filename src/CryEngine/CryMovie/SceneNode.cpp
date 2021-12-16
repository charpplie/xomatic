////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   scenenode.cpp
//  Version:     v1.00
//  Created:     23/4/2002 by Lennert.
//  Compilers:   Visual C++ 7.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SceneNode.h"
#include "AnimTrack.h"
#include "SelectTrack.h"
#include "EventTrack.h"
#include "ConsoleTrack.h"
#include "MusicTrack.h"
#include "SequenceTrack.h"
#include "GotoTrack.h"
#include "CaptureTrack.h"
#include "ISystem.h"
#include "ITimer.h"
#include "AnimCameraNode.h"
#include "Movie.h"

#include <ISound.h>
#include <IMusicSystem.h>
#include <IConsole.h>

#define s_nodeParamsInitialized s_nodeParamsInitializedScene
#define s_nodeParams s_nodeParamsSene
#define AddSupportedParam AddSupportedParamScene

namespace {
	bool s_nodeParamsInitialized = false;
	std::vector<IAnimNode::SParamInfo> s_nodeParams;

	void AddSupportedParam( const char *sName,int paramId,EAnimValue valueType,int flags=0 )
	{
		IAnimNode::SParamInfo param;
		param.name = sName;
		param.paramId = paramId;
		param.valueType = valueType;
		param.flags = flags;
		s_nodeParams.push_back( param );
	}
}

//////////////////////////////////////////////////////////////////////////
CAnimSceneNode::CAnimSceneNode()
{
	for (int i=0;i<SCENE_SOUNDTRACKS;i++)
	{
		m_SoundInfo[i].nLastKey=-1;
		m_SoundInfo[i].pSound=NULL;
		m_SoundInfo[i].sLastFilename="";
	}
	m_bMusicMoodSet=false;
	m_lastCameraKey = -1;
	m_lastEventKey = -1;
	m_lastConsoleKey = -1;
	m_lastMusicKey = -1;
	m_lastSequenceKey = -1;
	m_nLastGotoKey=-1;
	m_lastCaptureKey = -1;
	m_bLastCapturingEnded = true;
	m_currentCameraEntityId = 0;
	SetName("Scene");

	if (!s_nodeParamsInitialized)
	{
		s_nodeParamsInitialized = true;
		AddSupportedParam( "Camera",APARAM_CAMERA,AVALUE_SELECT );
		AddSupportedParam( "Event",APARAM_EVENT,AVALUE_EVENT );
		AddSupportedParam( "Sound",APARAM_SOUND1,AVALUE_SOUND );
		AddSupportedParam( "Sequence",APARAM_SEQUENCE,AVALUE_SEQUENCE );
		AddSupportedParam( "Console",APARAM_CONSOLE,AVALUE_CONSOLE );
		AddSupportedParam( "Music",APARAM_MUSIC,AVALUE_MUSIC );
		AddSupportedParam( "GoTo",APARAM_FLOAT_1,AVALUE_DISCRETE_FLOAT );
		AddSupportedParam( "Capture",APARAM_CAPTURE,AVALUE_CAPTURE );
		AddSupportedParam( "Timewarp",APARAM_TIMEWARP,AVALUE_FLOAT );
	}
}

//////////////////////////////////////////////////////////////////////////
CAnimSceneNode::~CAnimSceneNode()
{
	ReleaseSounds();
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::CreateDefaultTracks()
{
	CreateTrack(APARAM_CAMERA);
};

//////////////////////////////////////////////////////////////////////////
int CAnimSceneNode::GetParamCount() const
{
	return s_nodeParams.size();
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSceneNode::GetParamInfo( int nIndex, SParamInfo &info ) const
{
	if (nIndex >= 0 && nIndex < (int)s_nodeParams.size())
	{
		info = s_nodeParams[nIndex];
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSceneNode::GetParamInfoFromId( int paramId, SParamInfo &info ) const
{
	for (int i = 0; i < (int)s_nodeParams.size(); i++)
	{
		if (s_nodeParams[i].paramId == paramId)
		{
			info = s_nodeParams[i];
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::Activate( bool bActivate )
{
	if (bActivate)
	{
		{
			int trackCount = NumTracks();
			for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
			{
				int paramId = m_tracks[paramIndex].paramId;
				IAnimTrack *pTrack = m_tracks[paramIndex].track;
	
				if (paramId != APARAM_SEQUENCE)
				{
					continue;
				}

				CSequenceTrack *pSequenceTrack = (CSequenceTrack *)pTrack;

				for (int currKey = 0; currKey < pSequenceTrack->GetNumKeys(); currKey++)
				{
					ISequenceKey key;

					pSequenceTrack->GetKey(currKey,&key);

					IAnimSequence *pSequence = GetMovieSystem()->FindSequence(key.szSelection);
					if (pSequence)
					{
						if (key.bOverrideTimes)
						{
							key.fDuration = key.fEndTime-key.fStartTime > 0.0f ? key.fEndTime-key.fStartTime : 0.0f;
						}
						else
						{
							key.fDuration = pSequence->GetTimeRange().Length();
						}
						pTrack->SetKey(currKey,&key);
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::Animate( SAnimContext &ec )
{
	if (ec.bResetting)
		return;

	CSelectTrack			*cameraTrack = NULL;
	CEventTrack				*pEventTrack = NULL;
	CSequenceTrack		*pSequenceTrack = NULL;
	CConsoleTrack			*pConsoleTrack = NULL;
	CMusicTrack				*pMusicTrack = NULL;
	CGototTrack				*pGotoTrack = NULL;
	CCaptureTrack			*pCaptureTrack = NULL;
	/*
	bool bTimeJump = false;
	if (ec.time < m_time)
		bTimeJump = true;
	*/

	int nCurrentSoundTrackIndex = 0;

	int trackCount = NumTracks();
	for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
	{
		int paramId = m_tracks[paramIndex].paramId;
		IAnimTrack *pTrack = m_tracks[paramIndex].track;

		if(ec.trackMask && ((ec.trackMask & (1 << pTrack->GetType())) == 0))
			continue;

		switch (paramId)
		{
		case APARAM_CAMERA: cameraTrack = (CSelectTrack*)pTrack; break;
		case APARAM_EVENT: pEventTrack = (CEventTrack*)pTrack; break;
		case APARAM_SEQUENCE: pSequenceTrack = (CSequenceTrack*)pTrack; break;
		case APARAM_CONSOLE: pConsoleTrack = (CConsoleTrack*)pTrack; break;
		case APARAM_MUSIC: pMusicTrack = (CMusicTrack*)pTrack; break;
		case APARAM_CAPTURE: pCaptureTrack = (CCaptureTrack*)pTrack; break;

		case APARAM_FLOAT_1:
			{
				if (pTrack->GetType()==ATRACK_GOTO)
				{
					pGotoTrack=(CGototTrack*)pTrack;
				}
			}
		break;

		case APARAM_SOUND1:
			if (nCurrentSoundTrackIndex < SCENE_SOUNDTRACKS && pTrack->GetType() == ATRACK_SOUND)
			{
				CSoundTrack *pSoundTrack = static_cast<CSoundTrack*>(pTrack);
				ISoundKey key;
				int nSoundKey = pSoundTrack->GetActiveKey(ec.time, &key);
				if (nSoundKey!=m_SoundInfo[nCurrentSoundTrackIndex].nLastKey 
				|| key.time==ec.time || nSoundKey==-1 || ec.bSingleFrame)
				{
					//if (!ec.bSingleFrame || key.time == ec.time) // If Single frame update key time must match current time.
					{
						m_SoundInfo[nCurrentSoundTrackIndex].nLastKey=nSoundKey;
						ApplySoundKey( pSoundTrack,nSoundKey,nCurrentSoundTrackIndex, key, ec);
					}
				}
				nCurrentSoundTrackIndex++;
			}
			break;
		case APARAM_TIMEWARP:
			{
			float timeScale;
			pTrack->GetValue(ec.time, timeScale);
			if (timeScale < 0)
				timeScale = 0;
			gEnv->pTimer->SetTimeScale(timeScale);
			}
			break;
		}
	}

	// Check if a camera override is set.
	const char *overrideCamName = gEnv->pMovieSystem->GetOverrideCamName();
	IEntity *overrideCamEntity = NULL;
	EntityId overrideCamId = 0;
	if (overrideCamName && strlen(overrideCamName) > 0)
	{
		overrideCamEntity = gEnv->pEntitySystem->FindEntityByName(overrideCamName);
		if (overrideCamEntity)
			overrideCamId = overrideCamEntity->GetId();
	}
	
	if (overrideCamEntity)		// There is a valid overridden camera.
	{
		if (overrideCamId != gEnv->pMovieSystem->GetCameraParams().cameraEntityId)
		{
			ISelectKey key;
			strcpy_s(key.szSelection, overrideCamName);
			ApplyCameraKey( key,ec );
		}
	}
	else if (cameraTrack)			// No camera override. Just follow the standard procedure.
	{
		ISelectKey key;
		int cameraKey = cameraTrack->GetActiveKey(ec.time,&key);
		if (cameraKey != m_lastCameraKey/* && cameraKey > m_lastCameraKey*/)
		{
//			if (!ec.bSingleFrame || key.time == ec.time) // If Single frame update key time must match current time.
				ApplyCameraKey( key,ec );
		}
		m_lastCameraKey = cameraKey;
	}

	if (pEventTrack)
	{
		IEventKey key;
		int nEventKey = pEventTrack->GetActiveKey(ec.time,&key);
		if (nEventKey != m_lastEventKey && nEventKey >= 0)
		{
//			if (!ec.bSingleFrame || key.time == ec.time) // If Single frame update key time must match current time.
				ApplyEventKey(key, ec);
		}
		m_lastEventKey = nEventKey;
	}

	if (pConsoleTrack)
	{
		IConsoleKey key;
		int nConsoleKey = pConsoleTrack->GetActiveKey(ec.time,&key);
		if (nConsoleKey != m_lastConsoleKey && nConsoleKey >= 0)
		{
			if (!ec.bSingleFrame || key.time == ec.time) // If Single frame update key time must match current time.
				ApplyConsoleKey(key, ec);
		}
		m_lastConsoleKey = nConsoleKey;
	}

	if (pMusicTrack)
	{
		IMusicKey key;
		int nMusicKey = pMusicTrack->GetActiveKey(ec.time,&key);
		if (nMusicKey != m_lastMusicKey && nMusicKey >= 0)
		{
			if (!ec.bSingleFrame || key.time == ec.time) // If Single frame update key time must match current time.
				ApplyMusicKey(key, ec);
		}
		m_lastMusicKey = nMusicKey;
	}

	if (pSequenceTrack)
	{
		ISequenceKey key;
		int nSequenceKey = pSequenceTrack->GetActiveKey(ec.time,&key);
		IAnimSequence *pSequence = GetMovieSystem()->FindSequence(key.szSelection);

		if ((nSequenceKey != m_lastSequenceKey || !GetMovieSystem()->IsPlaying(pSequence)))
		{
//			if (!ec.bSingleFrame || key.time == ec.time) // If Single frame update key time must match current time.
				ApplySequenceKey(pSequenceTrack,m_lastSequenceKey,nSequenceKey,key, ec);
		}
		m_lastSequenceKey = nSequenceKey;

		if (gEnv->pSystem->IsEditorMode() && key.szSelection[0])
		{
			if (pSequence)
			{
				SAnimContext ac;
				ac.dt = ec.dt;
				ac.fps = ec.fps;
				ac.time = ec.time-key.time+(key.bOverrideTimes ? key.fStartTime : pSequence->GetTimeRange().start);
				ac.bSingleFrame = ec.bSingleFrame;

				float endTime = key.bOverrideTimes ? key.fEndTime : pSequence->GetTimeRange().end;

				if (ac.time > endTime)
				{
					ac.time = endTime;
				}

				if (ec.bSingleFrame || fabs(GetMovieSystem()->GetPlayingTime(pSequence)-ac.time) >= 0.1f)
				{
					pSequence->Animate(ac);
					GetMovieSystem()->SetPlayingTime(pSequence,ac.time);
				}
			}
		}
	}

	if (pGotoTrack)
	{
		ApplyGotoKey(pGotoTrack,ec);
	}

	if (pCaptureTrack)
	{
		ICaptureKey key;
		int nCaptureKey = pCaptureTrack->GetActiveKey(ec.time,&key);
		bool justEnded = false;
		if (!m_bLastCapturingEnded && key.time + key.duration < ec.time)
		{
			justEnded = true;
		}
		if (!ec.bSingleFrame 
		&& !(gEnv->IsEditor() && gEnv->bEditorGameMode == false))
		{
			if (nCaptureKey != m_lastCaptureKey && nCaptureKey >= 0)
			{
				if (m_bLastCapturingEnded == false)
				{
					assert(0);
					gEnv->pMovieSystem->EndCapture();
					m_bLastCapturingEnded = true;
				}
				gEnv->pMovieSystem->StartCapture(key);
				if (key.once == false)
					m_bLastCapturingEnded = false;
				m_lastCaptureKey = nCaptureKey;
			}
			else if (justEnded)
			{
				gEnv->pMovieSystem->EndCapture();
				m_bLastCapturingEnded = true;
			}
		}
	}

	m_time = ec.time;
	if (m_pOwner)
	{
    m_pOwner->OnNodeAnimated(this);
	}else
	{
	}
}

void CAnimSceneNode::ReleaseSounds()
{
	// stop all sounds
	for (int i=0;i<SCENE_SOUNDTRACKS;i++)
	{
		if (m_SoundInfo[i].pSound)
			m_SoundInfo[i].pSound->Stop();
		m_SoundInfo[i].nLastKey=-1;
		m_SoundInfo[i].sLastFilename="";
		m_SoundInfo[i].pSound=NULL;
	}
	// enable music-event processing
	if (m_bMusicMoodSet)
	{
		gEnv->pMusicSystem->EnableEventProcessing(true);
		m_bMusicMoodSet = false;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::Reset()
{
	// If camera from this sequence still active, remove it.
	// reset camera
	SCameraParams CamParams = gEnv->pMovieSystem->GetCameraParams();
	if (CamParams.cameraEntityId != 0 && CamParams.cameraEntityId == m_currentCameraEntityId)
	{
		CamParams.cameraEntityId = 0;
		CamParams.fFOV = 0;
		CamParams.justActivated = true;
		gEnv->pMovieSystem->SetCameraParams(CamParams);
	}

	if (m_lastSequenceKey >= 0)
	{
		{
			int trackCount = NumTracks();
			for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
			{
				int paramId = m_tracks[paramIndex].paramId;
				IAnimTrack *pTrack = m_tracks[paramIndex].track;

				if (paramId != APARAM_SEQUENCE)
				{
					continue;
				}

				CSequenceTrack *pSequenceTrack = (CSequenceTrack *)pTrack;
				ISequenceKey prevKey;

				pSequenceTrack->GetKey(m_lastSequenceKey,&prevKey);
				GetMovieSystem()->StopSequence(prevKey.szSelection);
			}
		}
	}

	// If the last capturing hasn't finished properly, end it here.
	if (m_bLastCapturingEnded == false)
	{
		GetMovieSystem()->EndCapture();
		m_bLastCapturingEnded = true;
	}

	m_lastCameraKey = -1;
	m_lastEventKey = -1;
	m_lastConsoleKey = -1;
	m_lastMusicKey = -1;
	m_lastSequenceKey = -1;
	m_nLastGotoKey = -1;
	m_lastCaptureKey = -1;
	m_bLastCapturingEnded = true;
	m_currentCameraEntityId = 0;

	ReleaseSounds();

	if (GetTrackForParameter(APARAM_TIMEWARP))
		gEnv->pTimer->SetTimeScale(1.0f);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::Pause()
{
	ReleaseSounds();
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::ApplyCameraKey( ISelectKey &key,SAnimContext &ec )
{
	IAnimNode *cameraNode = m_pSequence->FindNodeByName(key.szSelection);

	SCameraParams CamParams;
	CamParams.cameraEntityId = 0;
	CamParams.fFOV = 0;
	CamParams.justActivated = true;

	IEntity *pEntity = gEnv->pEntitySystem->FindEntityByName(key.szSelection);
	if (pEntity)
		CamParams.cameraEntityId = pEntity->GetId();

	if (cameraNode && cameraNode->GetType() == ANODE_CAMERA)
	{
		float fov = 60.0f;
		cameraNode->GetParamValue( ec.time,APARAM_FOV,fov );
		CamParams.fFOV = DEG2RAD(fov);
	}
	else
	{
		/*
		if (strlen(key.szSelection) > 0)
		{
			gEnv->pMovieSystem->GetSystem()->Warning( VALIDATOR_MODULE_MOVIE,VALIDATOR_WARNING,0,0,
				"[CryMovie] Camera entity %s not found",(const char*)key.szSelection );
		}
		*/
	}
	m_currentCameraEntityId = CamParams.cameraEntityId;
	gEnv->pMovieSystem->SetCameraParams(CamParams);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::ApplyEventKey(IEventKey &key, SAnimContext &ec)
{
	char funcName[1024];
	strcpy_s(funcName, "Event_");
	strcat_s(funcName, key.event);
	gEnv->pMovieSystem->SendGlobalEvent(funcName);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::ApplySoundKey( IAnimTrack *pTrack,int nCurrKey,int nLayer, ISoundKey &key, SAnimContext &ec)
{
	if (m_SoundInfo[nLayer].nLastKey==-1)
	{
		if (m_SoundInfo[nLayer].pSound)
			m_SoundInfo[nLayer].pSound->Stop();
		m_SoundInfo[nLayer].pSound=NULL;
		return;
	}
	bool bNewSound = false;
	if (((strcmp(m_SoundInfo[nLayer].sLastFilename.c_str(), key.pszFilename) != 0) || (!m_SoundInfo[nLayer].pSound)))
	{
		int flags = 0;
		//if (key.bStream)
			//flags |= FLAG_SOUND_STREAM;
		//else
			//flags |= FLAG_SOUND_LOAD_SYNCHRONOUSLY; // Always synchronously for now.

		if (key.bLoop)
			flags |= FLAG_SOUND_LOOP;
		if (key.bVoice)
			flags |= FLAG_SOUND_VOICE;
		//if (key.b3DSound)
		{
			// Always 2D sound.
			flags |= FLAG_SOUND_2D|FLAG_SOUND_STEREO|FLAG_SOUND_16BITS;
		}

		// we have a different sound now
		if (m_SoundInfo[nLayer].pSound)
			m_SoundInfo[nLayer].pSound->Stop();
		
		// only try to create a sound if a sound is referenced. Keys to stop loopings sounds are empty intentionally
		if (strlen(key.pszFilename) > 0)
		{
			m_SoundInfo[nLayer].pSound = gEnv->pSoundSystem->CreateSound(key.pszFilename, flags|FLAG_SOUND_UNSCALABLE|FLAG_SOUND_MOVIE);

			if (m_SoundInfo[nLayer].pSound)
			{
				if (key.bVoice)
				{
					const char *sDialogKey = gEnv->pSoundSystem->GetInterfaceExtended()->MakeDialogKey(key.pszFilename);
					strncpy(key.pszFilename, sDialogKey, sizeof(key.pszFilename));
					m_SoundInfo[nLayer].pSound->SetSemantic((ESoundSemantic)(eSoundSemantic_TrackView|eSoundSemantic_Dialog));
				}
				else
					m_SoundInfo[nLayer].pSound->SetSemantic(eSoundSemantic_TrackView);

				m_SoundInfo[nLayer].nLength = m_SoundInfo[nLayer].pSound->GetLengthMs();
				key.fDuration = ((float)m_SoundInfo[nLayer].nLength) / 1000.0f;
				pTrack->SetKey( nCurrKey,&key ); // Update key duration.
			}
			else
			{
				if (!gEnv->pSoundSystem->GetInterfaceExtended()->IsNullImplementation())
					gEnv->pSoundSystem->Log(eSLT_Warning, "<Sound> In Trackview SceneNode <%s> sound <%s> can not be loaded.", GetName(), key.pszFilename);
			}

			m_SoundInfo[nLayer].sLastFilename = key.pszFilename;
			bNewSound = true;
		}
	}
	if (!m_SoundInfo[nLayer].pSound)
		return;
	
	if (bNewSound && !(m_SoundInfo[nLayer].pSound->GetFlags() & FLAG_SOUND_EVENT))
	{
		m_SoundInfo[nLayer].pSound->GetInterfaceExtended()->SetSoundPriority( MOVIE_SOUND_PRIORITY );
		m_SoundInfo[nLayer].pSound->GetInterfaceExtended()->SetVolume(key.fVolume);
		m_SoundInfo[nLayer].pSound->GetInterfaceExtended()->SetPan(key.nPan);
	}

	int nOffset=(int)((ec.time-key.time)*1000.0f);
	if (nOffset < m_SoundInfo[nLayer].nLength)
	{
		//return;
		m_SoundInfo[nLayer].pSound->GetInterfaceExtended()->SetCurrentSamplePos(nOffset, true);
	}
	if (m_SoundInfo[nLayer].nLength != 0 && nOffset > m_SoundInfo[nLayer].nLength)
	{
		// If time is outside of sound, do not start it.
		bNewSound = false;
	}
	
	if (bNewSound)
	{
		((CMovieSystem*)gEnv->pMovieSystem)->OnPlaySound( ec.sequence, m_SoundInfo[nLayer].pSound );
		if (!m_SoundInfo[nLayer].pSound->IsPlaying())
			m_SoundInfo[nLayer].pSound->Play();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::ApplySequenceKey(  IAnimTrack *pTrack,int nPrevKey,int nCurrKey,ISequenceKey &key,SAnimContext &ec )
{
	if( !key.bDoNotStop )
	{
		if (nPrevKey >= 0)
		{
			ISequenceKey prevKey;
			pTrack->GetKey(nPrevKey,&prevKey);
			GetMovieSystem()->StopSequence( prevKey.szSelection );
		}
	}

	if (nCurrKey >= 0 && key.szSelection[0])
	{
		IAnimSequence *pSequence = GetMovieSystem()->FindSequence(key.szSelection);
		float startTime = -FLT_MAX;
		float endTime = -FLT_MAX;
		if (pSequence)
		{
			if (key.bOverrideTimes)
			{
				key.fDuration = key.fEndTime-key.fStartTime > 0.0f ? key.fEndTime-key.fStartTime : 0.0f;
				startTime = key.fStartTime;
				endTime = key.fEndTime;
			}
			else
			{
				key.fDuration = pSequence->GetTimeRange().Length();
			}
			pTrack->SetKey( nCurrKey,&key );
		}
		GetMovieSystem()->PlaySequence(key.szSelection,m_pSequence,true,true,startTime,endTime);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::ApplyConsoleKey(IConsoleKey &key, SAnimContext &ec)
{
	if (key.command[0])
	{
		gEnv->pConsole->ExecuteString( key.command );
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSceneNode::ApplyMusicKey(IMusicKey &key, SAnimContext &ec)
{
	IMusicSystem *pMusicSystem = gEnv->pMusicSystem;
	
	switch (key.eType)
	{
		case eMusicKeyType_SetMood:
			m_bMusicMoodSet=true;
			pMusicSystem->EnableEventProcessing(false);
			pMusicSystem->SetMood(key.szMood);
			break;
		case eMusicKeyType_VolumeRamp:
			break;
	}
}

void CAnimSceneNode::ApplyGotoKey(CGototTrack*	poGotoTrack,SAnimContext &ec)
{
	IDiscreteFloatKey			stDiscreteFloadKey;
	int										nCurrentActiveKeyIndex(-1);

	nCurrentActiveKeyIndex=poGotoTrack->GetActiveKey(ec.time,&stDiscreteFloadKey);
	if (nCurrentActiveKeyIndex != m_nLastGotoKey && nCurrentActiveKeyIndex >= 0)
	{
		if (!ec.bSingleFrame)
		{
			if (stDiscreteFloadKey.m_fValue>=0)
			{
				GetMovieSystem()->GoToFrame(m_pSequence->GetName(), stDiscreteFloadKey.m_fValue);
			}			
		}
	}
	
	m_nLastGotoKey=nCurrentActiveKeyIndex;
}

#undef s_nodeParamsInitialized
#undef s_nodeParams
#undef AddSupportedParam

