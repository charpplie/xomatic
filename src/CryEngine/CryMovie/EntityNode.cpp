////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   entitynode.cpp
//  Version:     v1.00
//  Created:     23/4/2002 by Timur.
//  Compilers:   Visual C++ 7.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "EntityNode.h"
#include "EventTrack.h"
#include "CharacterTrack.h"
#include "AnimSplineTrack.h"
#include "ExprTrack.h"
#include "BoolTrack.h"
#include "ISystem.h"
#include "SelectTrack.h"
#include "FaceSeqTrack.h"
#include "LookAtTrack.h"
#include "CompoundSplineTrack.h"
#include "Movie.h"
#include "PNoise3.h"

#include <ISound.h>
#include <ILipSync.h>
#include <ICryAnimation.h>
#include <CryCharMorphParams.h>
#include <Cry_Camera.h>

// AI
#include <IAgent.h>
#include "IAIObject.h"
#define HEAD_BONE_NAME "Bip01 Head"

#define s_nodeParamsInitialized s_nodeParamsInitializedEnt
#define s_nodeParams s_nodeParamsEnt
#define AddSupportedParam AddSupportedParamEnt

//////////////////////////////////////////////////////////////////////////
namespace
{
	bool s_nodeParamsInitialized = false;
	std::vector<IAnimNode::SParamInfo> s_nodeParams;

	void AddSupportedParam( std::vector<IAnimNode::SParamInfo> &nodeParams,const char *sName,int paramId,EAnimValue valueType,int flags=0 )
	{
		IAnimNode::SParamInfo param;
		param.name = sName;
		param.paramId = paramId;
		param.valueType = valueType;
		param.flags = flags;
		nodeParams.push_back( param );
	}
};

//////////////////////////////////////////////////////////////////////////
CAnimEntityNode::CAnimEntityNode()
{
	m_EntityId = 0;
	m_entityGuid = 0;
	m_target=NULL;
	m_bWasTransRot = false;
	m_nPlayingAnimations = 0;

	m_pos(0,0,0);
	m_scale(1,1,1);
	m_rotate.SetIdentity();

	m_visible = true;

	m_time = 0.0f;

	m_lastEntityKey = -1;
	m_lastCharacterKey[0] = -1;
	m_lastCharacterKey[1] = -1;
	m_lastCharacterKey[2] = -1;
	m_bPlayingAnimationAtLayer[0] = false;
	m_bPlayingAnimationAtLayer[1] = false;
	m_bPlayingAnimationAtLayer[2] = false;

	m_lookAtTarget = "";
	m_lookAtEntityId = 0;
	m_allowAdditionalTransforms = true;
	m_boneSet = eLookAtKeyBoneSet_HeadEyes;

	m_entityGuidTarget = 0;
	m_EntityIdTarget = 0;
	m_entityGuidSource = 0;
	m_EntityIdSource = 0;

	if (!s_nodeParamsInitialized)
	{
		s_nodeParamsInitialized = true;
		AddSupportedParams( s_nodeParams );
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::AddSupportedParams( std::vector<IAnimNode::SParamInfo> &nodeParams )
{
	AddSupportedParam( nodeParams,"Position",APARAM_POS,AVALUE_VECTOR );
	AddSupportedParam( nodeParams,"Rotation",APARAM_ROT,AVALUE_QUAT );
	AddSupportedParam( nodeParams,"Scale",APARAM_SCL,AVALUE_VECTOR );
	AddSupportedParam( nodeParams,"Visibility",APARAM_VISIBLE,AVALUE_BOOL );
	AddSupportedParam( nodeParams,"Event",APARAM_EVENT,AVALUE_EVENT );
	AddSupportedParam( nodeParams,"Sound",APARAM_SOUND1,AVALUE_SOUND,PARAM_MULTIPLE_TRACKS );
	AddSupportedParam( nodeParams,"Animation",APARAM_CHARACTER1,AVALUE_CHARACTER,PARAM_MULTIPLE_TRACKS );
	AddSupportedParam( nodeParams,"Expression",APARAM_EXPRESSION1,AVALUE_EXPRESSION,PARAM_MULTIPLE_TRACKS );
	AddSupportedParam( nodeParams,"Facial Sequence",APARAM_FACE_SEQUENCE,AVALUE_FACESEQ );
	AddSupportedParam( nodeParams,"LookAt",APARAM_LOOKAT,AVALUE_LOOKAT );
	AddSupportedParam( nodeParams,"Noise",APARAM_NOISE,AVALUE_VECTOR4 );
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::CreateDefaultTracks()
{
	CreateTrack(APARAM_POS);
	CreateTrack(APARAM_ROT);
	CreateTrack(APARAM_EVENT);
};

//////////////////////////////////////////////////////////////////////////
CAnimEntityNode::~CAnimEntityNode()
{
	ReleaseSounds();
}

//////////////////////////////////////////////////////////////////////////
int CAnimEntityNode::GetParamCount() const
{
	return s_nodeParams.size();
}

//////////////////////////////////////////////////////////////////////////
bool CAnimEntityNode::GetParamInfo( int nIndex, SParamInfo &info ) const
{
	if (nIndex >= 0 && nIndex < (int)s_nodeParams.size())
	{
		info = s_nodeParams[nIndex];
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimEntityNode::GetParamInfoFromId( int paramId, SParamInfo &info ) const
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
IEntity* CAnimEntityNode::GetEntity()
{
	if (!m_EntityId)
	{
		m_EntityId = gEnv->pEntitySystem->FindEntityByGuid(m_entityGuid);
	}
	return gEnv->pEntitySystem->GetEntity(m_EntityId);
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SetEntityGuid( const EntityGUID &guid )
{
	m_entityGuid = guid;
	m_EntityId = gEnv->pEntitySystem->FindEntityByGuid(m_entityGuid);
}


//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SetEntityGuidTarget( const EntityGUID &guid )
{
	m_entityGuidTarget = guid;
	m_EntityIdTarget = gEnv->pEntitySystem->FindEntityByGuid(m_entityGuidTarget);
}


//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SetEntityGuidSource( const EntityGUID &guid )
{
	m_entityGuidSource = guid;
	m_EntityIdSource = gEnv->pEntitySystem->FindEntityByGuid(m_entityGuidSource);
}


//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::StillUpdate()
{
	IEntity *pEntity = GetEntity();
	if (!pEntity)
		return;

	int paramCount = NumTracks();
	for (int paramIndex = 0; paramIndex < paramCount; paramIndex++)
	{
		int trackType = m_tracks[paramIndex].paramId;
		IAnimTrack *pTrack = m_tracks[paramIndex].track;

		switch (trackType)
		{
		case APARAM_LOOKAT:
			{
				SAnimContext ec;
				ec.time = m_time;

				CLookAtTrack *pSelTrack = (CLookAtTrack*)pTrack;
				AnimateLookAt( pSelTrack,ec );
			}
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::Animate( SAnimContext &ec )
{
	IEntity *pEntity = GetEntity();
	if (!pEntity)
		return;

	Vec3 pos = m_pos;
	Quat rotate = m_rotate;
	Vec3 scale = m_scale;
	Vec4 noiseParam;

	bool bPosModified = pEntity->GetPos() == m_pos?false:true;
	bool bAnglesModified = pEntity->GetRotation() == m_rotate?false:true;
	bool bScaleModified = pEntity->GetScale() == m_scale?false:true;
	bool bApplyNoise = false;
	
	IAnimTrack *posTrack = NULL;
	IAnimTrack *rotTrack = NULL;
	IAnimTrack *sclTrack = NULL;

	if (!ec.bResetting)
	{
		StopExpressions();
	}

	int nAnimCharacterLayer = 0;

	int trackCount = NumTracks();
	for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
	{
		int trackType = m_tracks[paramIndex].paramId;
		IAnimTrack *pTrack = m_tracks[paramIndex].track;
		if(pTrack->GetNumKeys() == 0)
			continue;

		if(ec.trackMask && ((ec.trackMask & (1 << pTrack->GetType())) == 0))
			continue;

		switch (trackType)
		{
		case APARAM_POS:
			posTrack = pTrack;
			posTrack->GetValue( ec.time,pos );
			if (pos != m_pos)
			{
				bPosModified = true;
			}
			break;
		case APARAM_ROT:
			rotTrack = pTrack;
			rotTrack->GetValue( ec.time,rotate );
			if (rotate != m_rotate)
				bAnglesModified = true;
			break;
		case APARAM_NOISE:
			static_cast<CCompoundSplineTrack*>(pTrack)->GetValue( ec.time, noiseParam );
			m_posNoise.amp = noiseParam.x;
			m_posNoise.freq = noiseParam.y;
			m_rotNoise.amp = noiseParam.z;
			m_rotNoise.freq = noiseParam.w;
			bApplyNoise = true;
			break;
		case APARAM_SCL:
			sclTrack = pTrack;
			sclTrack->GetValue( ec.time,scale );
			// Check whether the scale value is valid.
			if (scale.x < 0.01 || scale.y < 0.01 || scale.z < 0.01)
			{
				CryWarning(VALIDATOR_MODULE_MOVIE, VALIDATOR_WARNING, 
							"An EntityNode <%s> gets an invalid scale (%f,%f,%f) from a TrackView track, so ignored.",
							(const char*)GetName(), scale.x, scale.y, scale.z);
				scale = m_scale;
			}
			if (scale != m_scale)
				bScaleModified = true;
			break;
		case APARAM_EVENT:
			if (!ec.bResetting)
			{
				CEventTrack *entityTrack = (CEventTrack*)pTrack;
				IEventKey key;
				int entityKey = entityTrack->GetActiveKey(ec.time,&key);
				// If key is different or if time is standing exactly on key time.
				//if ((entityKey != m_lastEntityKey || key.time == ec.time) && (!ec.bSingleFrame))
				if (entityKey != m_lastEntityKey || key.time == ec.time)
				{
					m_lastEntityKey = entityKey;
					if (entityKey >= 0)
					{
//						if (!ec.bSingleFrame || key.time == ec.time) // If Single frame update key time must match current time.
							ApplyEventKey( entityTrack,entityKey,key );
					}
				}
			}
			break;
		case APARAM_VISIBLE:
			if (!ec.bResetting)
			{
				IAnimTrack *visTrack = pTrack;
				bool visible = m_visible;
				visTrack->GetValue( ec.time,visible );
				pEntity->Hide(!visible);
				m_visible = visible;
			}
			break;

		//////////////////////////////////////////////////////////////////////////
		case APARAM_SOUND1:
			if (!ec.bResetting)
			{
				CSoundTrack *pSoundTrack = (CSoundTrack*)pTrack;
				ISoundKey key;
				int nSoundKey = pSoundTrack->GetActiveKey(ec.time, &key);

				if (!key.bLoop && key.time+key.fDuration < ec.time)
					nSoundKey = -1;

				int nSoundIndex = paramIndex;
				if (nSoundIndex >= m_SoundInfo.size())
					m_SoundInfo.resize(trackCount);

				if (nSoundKey!=m_SoundInfo[nSoundIndex].nLastKey || key.time==ec.time || nSoundKey==-1 || ec.bSingleFrame)
				{
					ApplySoundKey( pSoundTrack,nSoundKey,nSoundIndex, key, ec);
				}
				else
				{
					// update
					if (m_SoundInfo[nSoundIndex].pSound)
					{
						if (m_SoundInfo[nSoundIndex].pSound->GetFlags() & FLAG_SOUND_3D)
						{
							Vec3 SoundPos = pEntity->GetPos();
							Vec3 SoundDir = pEntity->GetWorldTM().TransformVector(FORWARD_DIRECTION);
							Vec3 vCharacterCenter(0);

							bool bHead = false;
							bool bCharacter = false;

							// 3D sound.

							// testing for head position
							ICharacterInstance * pCharacter = pEntity->GetCharacter(0);

							if (pCharacter)
							{
								bCharacter = true;
								vCharacterCenter = pCharacter->GetAABB().GetCenter();
								SoundPos = pEntity->GetWorldTM()*pCharacter->GetAABB().GetCenter();

								if (m_SoundInfo[nSoundIndex].pSound->GetFlags() & FLAG_SOUND_VOICE)
								{
									bHead = true;
								}
							}

							IEntitySoundProxy *pSoundProxy = (IEntitySoundProxy*)pEntity->CreateProxy(ENTITY_PROXY_SOUND);

							if (pSoundProxy)
							{
								pSoundProxy->UpdateSounds();

 								Vec3 vOffset(0);
 								if (!bHead && bCharacter)
 								{
 									vOffset = vCharacterCenter;
 								}

								pSoundProxy->SetSoundPos(m_SoundInfo[nSoundIndex].pSound->GetId(), vOffset);
							}
							else
							{
								// also set sound position, because entity might not move at all
								m_SoundInfo[nSoundIndex].pSound->SetPosition( SoundPos );
								m_SoundInfo[nSoundIndex].pSound->SetDirection( SoundDir );
							}
						}
					}
				}

				m_SoundInfo[nSoundIndex].nLastKey = nSoundKey;
			}
			break;
		
		//////////////////////////////////////////////////////////////////////////
		case APARAM_CHARACTER1:
			if (!ec.bResetting)
			{
				if (nAnimCharacterLayer < MAX_CHARACTER_TRACKS)
				{
					int index = nAnimCharacterLayer;
					CCharacterTrack *pCharTrack = (CCharacterTrack*)pTrack;
					AnimateCharacterTrack( pCharTrack,ec,index );
					nAnimCharacterLayer++;
				}
			}
			break;

		//////////////////////////////////////////////////////////////////////////
		case APARAM_EXPRESSION1:
			if (!ec.bResetting)
			{
				CExprTrack *pExpTrack = (CExprTrack*)pTrack;
				AnimateExpressionTrack( pExpTrack,ec );
			}
			break;

			//////////////////////////////////////////////////////////////////////////
		case APARAM_FACE_SEQUENCE:
			if (!ec.bResetting)
			{
				CFaceSeqTrack *pSelTrack = (CFaceSeqTrack*)pTrack;
				AnimateFacialSequence( pSelTrack,ec );
			}
			break;
		
		case APARAM_LOOKAT:
			if (!ec.bResetting)
			{
				CLookAtTrack *pSelTrack = (CLookAtTrack*)pTrack;
				AnimateLookAt( pSelTrack,ec );
			}
			break;
		};
	}
	
	if (bApplyNoise)
	{
		// Position noise
		if (m_posNoise.amp != 0)
		{
			pos += m_posNoise.Get(ec.time);
			if (pos != m_pos)
			{
				bPosModified = true;
			}
		}

		// Rotation noise
		if (m_rotNoise.amp != 0)
		{
			Ang3 angles = Ang3::GetAnglesXYZ(Matrix33(rotate))*180.0f/gf_PI;
			Vec3 noiseVec = m_rotNoise.Get(ec.time);
			angles.x += noiseVec.x;
			angles.y += noiseVec.y;
			angles.z += noiseVec.z;
			rotate.SetRotationXYZ(angles*gf_PI/180.0f);

			if (rotate != m_rotate)
			{
				bAnglesModified = true;
			}
		}
	}

	if (!ec.bResetting)
	{
		//////////////////////////////////////////////////////////////////////////
		// If not resetting animation sequence.
		//////////////////////////////////////////////////////////////////////////
		if (!ec.bSingleFrame)
		{
			IPhysicalEntity *pEntityPhysics = pEntity->GetPhysics();
			if (pEntityPhysics && !pEntity->IsHidden())
			{
				if (ec.time - m_time < 0.1f)
				{
					float timeStep = ec.time - m_time;

					float rtimeStep = timeStep>1E-5f ? 1.0f/timeStep : 0.0f;
					pe_action_set_velocity asv;
					asv.v = (pos - m_pos)*rtimeStep;
					asv.w = (rotate*!m_rotate).v*(rtimeStep*2);
					pEntityPhysics->Action(&asv);
				}
			}
		}
	} // not single frame

	m_time = ec.time;

	if (bPosModified || bAnglesModified || bScaleModified || (m_target!=NULL))
	{
		m_pos = pos;
		m_rotate = rotate;
		m_scale = scale;

		m_bIgnoreSetParam = true; // Prevents feedback change of track.
		if (m_pOwner)
		{
			m_pOwner->OnNodeAnimated(this);
		}
		else	// no callback specified, so lets move the entity directly
		{
			if (bPosModified)
				pEntity->SetPos(m_pos,ENTITY_XFORM_TRACKVIEW);

			if (bAnglesModified)
				pEntity->SetRotation(m_rotate,ENTITY_XFORM_TRACKVIEW);

			if (bScaleModified)
				pEntity->SetScale(m_scale,ENTITY_XFORM_TRACKVIEW);
		}
		m_bIgnoreSetParam = false; // Prevents feedback change of track.
	}

	if(m_entityGuidTarget)
	{
		IEntity * pEntityTarget = 0;
		if (!m_EntityIdTarget)
			m_EntityIdTarget = gEnv->pEntitySystem->FindEntityByGuid(m_entityGuidTarget);
		if(m_EntityIdTarget)
			pEntityTarget = gEnv->pEntitySystem->GetEntity(m_EntityIdTarget);
		if(pEntityTarget)
		{
			Vec3 worldPos = m_pos;
			if(pEntity->GetParent())
			{
				worldPos = pEntity->GetParent()->GetWorldTM().TransformPoint(worldPos);
			}
			Matrix34 tm = Matrix34( Matrix33::CreateRotationVDir((pEntityTarget->GetWorldPos()-worldPos).GetNormalized()), worldPos );
			pEntity->SetWorldTM(tm, ENTITY_XFORM_TRACKVIEW);
		}
	}

	if(m_entityGuidSource)
	{
		IEntity * pEntitySource = 0;
		if (!m_EntityIdSource)
			m_EntityIdSource = gEnv->pEntitySystem->FindEntityByGuid(m_entityGuidSource);
		if(m_EntityIdSource)
			pEntitySource = gEnv->pEntitySystem->GetEntity(m_EntityIdSource);
		if(pEntitySource)
		{
			Matrix34 wtm = pEntitySource->GetWorldTM();
			Vec3 worldPos = wtm.GetTranslation();
			Matrix34 tm = Matrix34( Matrix33::CreateRotationVDir((pEntity->GetWorldPos()-worldPos).GetNormalized()), worldPos );
			pEntitySource->SetWorldTM(tm, ENTITY_XFORM_TRACKVIEW);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::ReleaseSounds()
{
	// stop all sounds
	for (int i=0,num = m_SoundInfo.size();i<num;i++)
	{
		SSoundInfo &sndInfo = m_SoundInfo[i];

		if (sndInfo.pSound)
			sndInfo.pSound->Stop();

		sndInfo.pSound = NULL;
		sndInfo.nLastKey = -1;
		
		if (!sndInfo.sLastFilename.empty())
			sndInfo.sLastFilename = "";
	}

	// Stop sounds if this is a SoundEventSpot
	IEntity const* const pEntity = GetEntity();
	if(pEntity)
	{
		IEntityClass const* const pEntityClass = pEntity->GetClass();
		if(pEntityClass && !strcmp(pEntityClass->GetName(), "SoundEventSpot"))
		{
			IEntityScriptProxy* const pScriptProxy = (IEntityScriptProxy*)(pEntity->GetProxy(ENTITY_PROXY_SCRIPT));
			if(pScriptProxy)
				pScriptProxy->CallEvent("Stop", true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::Reset()
{
	m_EntityId = 0;
	m_lastEntityKey = -1;
	m_lastCharacterKey[0] = -1;
	m_lastCharacterKey[1] = -1;
	m_lastCharacterKey[2] = -1;
	m_lookAtTarget = "";
	m_lookAtEntityId = 0;
	m_allowAdditionalTransforms = true;
	m_boneSet = eLookAtKeyBoneSet_HeadEyes;
	ReleaseSounds();
	ReleaseAllAnims();
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::PrepareAnimations()
{
	// Update durations of all character animations.
	IEntity *pEntity = GetEntity();
	if (!pEntity)
		return;
	ICharacterInstance *pCharacter = pEntity->GetCharacter(0);
	if (!pCharacter)
		return;

	IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
	if (!pAnimations)
		return;

	int trackCount = NumTracks();
	for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
	{
		int trackType = m_tracks[paramIndex].paramId;
		IAnimTrack *pTrack = m_tracks[paramIndex].track;

		switch (trackType)
		{
		case APARAM_CHARACTER1:
		case APARAM_CHARACTER2:
		case APARAM_CHARACTER3:
			{
				int numKeys = pTrack->GetNumKeys();
				for (int i = 0; i < numKeys; i++)
				{
					ICharacterKey key;
					pTrack->GetKey( i,&key );
					int animId = pAnimations->GetAnimIDByName( key.m_animation );
					if (animId >= 0)
					{
						//float duration = pAnimations->GetLength(animId);
						float duration = pAnimations->GetDuration_sec(animId);
						if (duration != key.m_duration)
						{
							key.m_duration = duration;
							pTrack->SetKey( i,&key );
						}
					}
				}
			}
			break;
		case APARAM_FACE_SEQUENCE:
			{
				IFacialInstance *pFaceInstance = pCharacter->GetFacialInstance();
				if (pFaceInstance)
				{
					ISelectKey key;
					int numKeys = pTrack->GetNumKeys();
					for (int i = 0; i < numKeys; i++)
					{
						pTrack->GetKey( i,&key );
						_smart_ptr<IFacialAnimSequence> pSequence = LoadFacialSequence(key.szSelection);
						if (pSequence)
						{
							key.fDuration = pSequence->GetTimeRange().Length();
							pTrack->SetKey( i,&key );
						}
					}
				}
			}
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
IFacialAnimSequence* CAnimEntityNode::LoadFacialSequence(const char* sequenceName)
{
	FacialSequenceMap::iterator loadedSequencePosition = m_facialSequences.find(CONST_TEMP_STRING(sequenceName));
	if (loadedSequencePosition == m_facialSequences.end())
	{
		IEntity* pEntity = GetEntity();
		ICharacterInstance* pCharacter = (pEntity ? pEntity->GetCharacter(0) : 0);
		IFacialInstance* pFaceInstance = (pCharacter ? pCharacter->GetFacialInstance() : 0);
		IFacialAnimSequence* pSequence = (pFaceInstance ? pFaceInstance->LoadSequence( sequenceName, false ) : 0);

		// Add the sequence to the cache, even if the pointer is 0 - stop us from continually trying to load a missing sequence.
		loadedSequencePosition = m_facialSequences.insert(std::make_pair(sequenceName, pSequence)).first;
	}

	return (loadedSequencePosition != m_facialSequences.end() ? 
								(*loadedSequencePosition).second : 
								_smart_ptr<IFacialAnimSequence>(0));
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::ReleaseAllFacialSequences()
{
	IEntity* pEntity = GetEntity();
	ICharacterInstance* pCharacter = (pEntity ? pEntity->GetCharacter(0) : 0);
	IFacialInstance* pFaceInstance = (pCharacter ? pCharacter->GetFacialInstance() : 0);
	if (pFaceInstance)
		pFaceInstance->StopSequence(eFacialSequenceLayer_Trackview);

	// If commented out facial sequence will not be released for this level and will stay cached
	//m_facialSequences.clear();
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SelectLookIKBlends(ELookAtKeyBoneSet boneSet, float* blends)
{
	switch (boneSet)
	{
		case eLookAtKeyBoneSet_Eyes:
			blends[0] = 0.0f;
			blends[1] = 0.0f;
			blends[2] = 0.0f;
			blends[3] = 0.0f;
			blends[4] = 0.0f;
			break;

		case eLookAtKeyBoneSet_SpineHeadEyes:
			blends[0] = 0.04f;
			blends[1] = 0.06f;
			blends[2] = 0.08f;
			blends[3] = 0.15f;
			blends[4] = 0.60f;
			break;

		case eLookAtKeyBoneSet_HeadEyes:
		default:
			blends[0] = 0.0f;
			blends[1] = 0.0f;
			blends[2] = 0.0f;
			blends[3] = 0.0f;
			blends[4] = 0.60f;
			break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::Activate( bool bActivate )
{
	if (bActivate)
	{
		PrepareAnimations();
	}
	else
	{
		// reset.
		// Reset called explicitly by sequence.
	}
	m_lookAtTarget = "";
	m_lookAtEntityId = 0;
	m_allowAdditionalTransforms = true;
	m_boneSet = eLookAtKeyBoneSet_HeadEyes;
};

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::Pause()
{
	ReleaseSounds();
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SetPos( float time,const Vec3 &pos )
{
	bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (m_flags&ANODE_FLAG_SELECTED)); // Only selected nodes can be recorded

	{
		IAnimTrack *posTrack = GetTrackForParameter(APARAM_POS);
		if (posTrack)
		{
			posTrack->SetValue( time,pos,bDefault );
			if (bDefault)
			{
				// Offset all keys by move amount.
				Vec3 offset = pos - m_pos;
				posTrack->OffsetKeyPosition(offset);
			}
		}
	}

	if (!bDefault)
		GetCMovieSystem()->Callback( IMovieCallback::CBR_CHANGETRACK ,this );

	m_pos = pos;
}
	
//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SetRotate( float time,const Quat &quat )
{
	m_rotate = quat;

	bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (m_flags&ANODE_FLAG_SELECTED)); // Only selected nodes can be recorded

	IAnimTrack *rotTrack = GetTrackForParameter(APARAM_ROT);
	if (rotTrack)
		rotTrack->SetValue( time,m_rotate,bDefault );

	if (!bDefault)
		GetCMovieSystem()->Callback( IMovieCallback::CBR_CHANGETRACK ,this );
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::SetScale( float time,const Vec3 &scale )
{
	m_scale = scale;
	bool bDefault = !(gEnv->pMovieSystem->IsRecording() && (m_flags&ANODE_FLAG_SELECTED)); // Only selected nodes can be recorded

	IAnimTrack *sclTrack = GetTrackForParameter(APARAM_SCL);
	if (sclTrack)
		sclTrack->SetValue( time,scale,bDefault );

	if (!bDefault)
		GetCMovieSystem()->Callback( IMovieCallback::CBR_CHANGETRACK ,this );
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::ApplyEventKey( CEventTrack *track,int keyIndex,IEventKey &key )
{
	IEntity* pEntity = GetEntity();
	if (!pEntity)
		return;

	if (*key.animation) // if there is an animation
	{
		// Start playing animation.
		ICharacterInstance *pCharacter = pEntity->GetCharacter(0);
		if (pCharacter)
		{
			CryCharAnimationParams aparams;
			//aparams.fBlendInTime = 0.5f;
			//aparams.fBlendOutTime = 0.5f;
			aparams.m_fTransTime  = 0.15f;

			aparams.m_fTransTime    = 0.0f;
			aparams.m_nFlags = CA_TRACK_VIEW_EXCLUSIVE;

			ISkeletonAnim* pISkeletonAnim = pCharacter->GetISkeletonAnim();
			
			aparams.m_nLayerID = 0;
			pISkeletonAnim->StartAnimation( key.animation,  aparams );
		//	aparams.m_nLayerID = 1;
		//	pISkeleton->StartAnimation( key.animation,0, 0,0, aparams );
		//	aparams.m_nLayerID = 2;
		//	pISkeleton->StartAnimation( key.animation,0, 0,0, aparams );

			IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
			assert (pAnimations);
			//float duration = pAnimations->GetLength( key.animation );

			int animId = pAnimations->GetAnimIDByName( key.animation );
			if (animId >= 0)
			{
				float duration = pAnimations->GetDuration_sec(animId);
				if (duration != key.duration)
				{
					key.duration = duration;
					track->SetKey( keyIndex,&key );
				}
			}
		}
		//char str[1024];
		//sprintf( str,"StartAnim: %s",key.animation );
		//CryLog( str );
	}
	
	if (*key.event) // if there's an event
	{
		// Fire event on Entity.
		IEntityScriptProxy *pScriptProxy = (IEntityScriptProxy*)pEntity->GetProxy(ENTITY_PROXY_SCRIPT);
		if (pScriptProxy)
		{
			// Find event
			int type = -1;
			if (IEntityClass *pClass = pEntity->GetClass())
			{
				int count = pClass->GetEventCount();
				IEntityClass::SEventInfo info;
				for (int i = 0; i < count; ++i)
				{
					info = pClass->GetEventInfo(i);
					if (strcmp(key.event,info.name) == 0)
					{
						type = info.type; 
						break;
					}
				}
			}

			// Convert value to type
			switch (type)
			{
				case IEntityClass::EVT_INT:
				case IEntityClass::EVT_FLOAT:
				case IEntityClass::EVT_ENTITY:
					pScriptProxy->CallEvent(key.event, (float)atof(key.eventValue));
					break;
				case IEntityClass::EVT_BOOL:
					pScriptProxy->CallEvent(key.event, atoi(key.eventValue)!=0?true:false);
					break;
				case IEntityClass::EVT_STRING:
					pScriptProxy->CallEvent(key.event, key.eventValue);
					break;
				case IEntityClass::EVT_VECTOR:
					{
						Vec3 vTemp(0,0,0);
						float x=0,y=0,z=0;
						int res = sscanf(key.eventValue,"%f,%f,%f",&x,&y,&z);
						assert(res==3);
						vTemp(x,y,z);
						pScriptProxy->CallEvent(key.event, vTemp);
					}
					break;
				case -1:
				default:
					pScriptProxy->CallEvent(key.event);
			}
			
		}
	}
	//m_entity->Hide( key.hidden );
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::ApplySoundKey( IAnimTrack *pTrack,int nCurrKey,int nLayer, ISoundKey &key, SAnimContext &ec )
{
	IEntity *pEntity = GetEntity();
	if (!pEntity)
		return;

	SSoundInfo &sndInfo = m_SoundInfo[nLayer];

	// there is not audio key, so lets stop any sound
	if (nCurrKey == -1)
	{
		if (sndInfo.pSound)
		{
			sndInfo.pSound->Stop();
			sndInfo.pSound = NULL;
		}
		return;
	}

	if (sndInfo.nLastKey == nCurrKey)
		return;

	// stop any sound that is playing on that track
	if (sndInfo.pSound)
	{
		sndInfo.pSound->Stop();
		sndInfo.pSound = NULL;
	}

	int flags = 0;

	bool bNewSound = false;

	if (((strcmp(sndInfo.sLastFilename.c_str(), key.pszFilename) != 0) || (!sndInfo.pSound)))
	{
	
		//if (key.bStream)
			//flags |= FLAG_SOUND_STREAM;
		//else
			//flags |= FLAG_SOUND_LOAD_SYNCHRONOUSLY; // Always synchronously for now.

		if (key.bLoop)
			flags |= FLAG_SOUND_LOOP;

		if (key.bVoice)
			flags |= FLAG_SOUND_VOICE;

		if (key.b3DSound)
		{
			// 3D sound.
			flags |= FLAG_SOUND_DEFAULT_3D;
		}
		else
		{
			// 2D sound.
			flags |= FLAG_SOUND_2D|FLAG_SOUND_STEREO|FLAG_SOUND_16BITS;
		} 
		// we have a different sound now
		if (m_SoundInfo[nLayer].pSound)
			m_SoundInfo[nLayer].pSound->Stop();

		// only try to create a sound if a sound is referenced. Keys to stop loopings sounds are empty intentionally
		if (strlen(key.pszFilename) > 0)
		{
			m_SoundInfo[nLayer].pSound = gEnv->pSoundSystem->CreateSound(key.pszFilename, flags | FLAG_SOUND_UNSCALABLE | FLAG_SOUND_MOVIE );
			
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

				if (m_SoundInfo[nLayer].nLength > 0)
					key.fDuration = ((float)m_SoundInfo[nLayer].nLength+10) / 1000.0f;

				pTrack->SetKey( nCurrKey,&key ); // Update key duration.
			}
			else
			{
				if (!gEnv->pSoundSystem->GetInterfaceExtended()->IsNullImplementation())
					gEnv->pSoundSystem->Log(eSLT_Warning, "<Sound> In Trackview EntityNode <%s> sound <%s> can not be loaded.", GetName(), key.pszFilename);
			}

			m_SoundInfo[nLayer].sLastFilename = key.pszFilename;
			bNewSound = true;
		}
	}

	if (!sndInfo.pSound)
		return;
	
	if (sndInfo.pSound->GetFlags() & FLAG_SOUND_EVENT)
	{
		sndInfo.pSound->GetInterfaceExtended()->SetSoundPriority( MOVIE_SOUND_PRIORITY );
		sndInfo.pSound->GetInterfaceExtended()->SetVolume(key.fVolume);
	}

	Vec3 SoundPos = pEntity->GetPos();
	Vec3 SoundDir = pEntity->GetWorldTM().TransformVector(FORWARD_DIRECTION);
	Vec3 vCharacterCenter(0);

	bool bHead = false;
	bool bCharacter = false;

	if (sndInfo.pSound->GetFlags() & FLAG_SOUND_3D)
	{
		// 3D sound.
		// testing for head position
		ICharacterInstance * pCharacter = pEntity->GetCharacter(0);
		if (pCharacter)
		{
			bCharacter = true;
			vCharacterCenter = pCharacter->GetAABB().GetCenter();
			SoundPos = pEntity->GetWorldTM()*pCharacter->GetAABB().GetCenter();

			if (sndInfo.pSound->GetFlags() & FLAG_SOUND_VOICE)
			{
				bHead = true;
			}
		}

		sndInfo.pSound->SetPosition( SoundPos );
		sndInfo.pSound->SetDirection( SoundDir );

		if (key.inRadius > 0 && key.outRadius > 0)
			sndInfo.pSound->GetInterfaceDeprecated()->SetMinMaxDistance( key.inRadius,key.outRadius );
	}
	else
	{
		// 2D sound.
		sndInfo.pSound->GetInterfaceExtended()->SetPan(key.nPan);
	} 

	// start sound with offset to get timing right when QL happened in the middle of a sound key
	int nOffset = (int)((ec.time-key.time)*1000.0f);

	if (nOffset > 30 && nOffset < sndInfo.nLength) // small 30ms threshold to not skip start
	{
		sndInfo.pSound->GetInterfaceExtended()->SetCurrentSamplePos(nOffset, true);
	}

	if (sndInfo.nLength != 0 && nOffset > sndInfo.nLength && !key.bLoop)
	{
		// If time is outside of sound, do not start it.
		bNewSound = false;
	}

	if (bNewSound)
	{
		((CMovieSystem*)gEnv->pMovieSystem)->OnPlaySound( ec.sequence, sndInfo.pSound );
		if (!sndInfo.pSound->IsPlaying())
		{
			IEntitySoundProxy *pSoundProxy = (IEntitySoundProxy*)pEntity->CreateProxy(ENTITY_PROXY_SOUND);

			if (pSoundProxy)
			{
				Vec3 vOffset(0);

				if (!bHead && bCharacter)
				{
					vOffset = vCharacterCenter;
				}

				pSoundProxy->PlaySound(sndInfo.pSound, vOffset, FORWARD_DIRECTION, 1.0f, key.bLipSync);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::ReleaseAllAnims()
{
	IEntity *pEntity = GetEntity();
	if (!pEntity)
		return;

	ICharacterInstance *pCharacter = pEntity->GetCharacter(0);
	if (!pCharacter)
		return;
	IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
	assert(pAnimations);
	for (TStringSetIt It=m_setAnimationSinks.begin();It!=m_setAnimationSinks.end();++It)
	{
		const char *pszName=(*It).c_str();
		//pCharacter->RemoveAnimationEventSink(pszName, this);
		//pAnimations->UnloadAnimation(pszName);
	}
	m_setAnimationSinks.clear();

	if (m_nPlayingAnimations > 0)
	{
		m_bPlayingAnimationAtLayer[0]=m_bPlayingAnimationAtLayer[1]=m_bPlayingAnimationAtLayer[2] = 0;
		pCharacter->GetISkeletonAnim()->SetTrackViewExclusive(0);

		pCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();

		pCharacter->SetAnimationSpeed(1.0000f);
		pCharacter->GetISkeletonAnim()->SetAnimationDrivenMotion(m_bWasTransRot);
		m_nPlayingAnimations = 0;
	}

	ReleaseAllFacialSequences();
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::OnEndAnimation(const char *sAnimation)
{
	IEntity *pEntity = GetEntity();
	if (!pEntity)
		return;

	TStringSetIt It=m_setAnimationSinks.find(sAnimation);
	if (It==m_setAnimationSinks.end())
		return;	// this anim was not started by us...
	m_setAnimationSinks.erase(It);
	ICharacterInstance *pCharacter = pEntity->GetCharacter(0);
	if (!pCharacter)
		return;

	IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
	assert(pAnimations);
	//pCharacter->RemoveAnimationEventSink(sAnimation, this);
	//pAnimations->UnloadAnimation(sAnimation);
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::AnimateCharacterTrack( class CCharacterTrack* track,SAnimContext &ec,int layer )
{
	ISystem* pISystem = GetISystem();
	IRenderer* pIRenderer			= gEnv->pRenderer;
	IRenderAuxGeom* pAuxGeom	= pIRenderer->GetIRenderAuxGeom();



	IEntity *pEntity = GetEntity();
	if (!pEntity)
		return;

	ICharacterInstance *pCharacter = pEntity->GetCharacter(0);
	if (!pCharacter)
		return;

	ISkeletonAnim* pISkeletonAnim = pCharacter->GetISkeletonAnim();

	ICharacterKey key;
	int currKey = track->GetActiveKey(ec.time,&key);
	// If key is different or if time is standing exactly on key time.
	if (currKey != m_lastCharacterKey[layer] || key.time == ec.time || ec.time < m_time)
	{
		m_lastCharacterKey[layer] = currKey;
		float t = ec.time - key.time;
		t = key.m_startTime + t*key.m_speed;
		if (key.m_animation[0] && (t < key.m_duration || key.m_bLoop))
		{
			// retrieve the animation collection for the model
			IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
			assert (pAnimations);

			if (key.m_bUnload)
			{
				m_setAnimationSinks.insert(TStringSetIt::value_type(key.m_animation));
				//pCharacter->AddAnimationEventSink(key.animation, this);
			}

			if (pISkeletonAnim->GetAnimationDrivenMotion() && m_nPlayingAnimations == 0)
				m_bWasTransRot = true;
			
			if (key.m_bInPlace)
			{
				pISkeletonAnim->SetAnimationDrivenMotion(1);
			}
			else
			{
				//pISkeletonAnim->StopAnimationInLayer(0,0.0f);
				pISkeletonAnim->SetAnimationDrivenMotion(0);
			}

			pISkeletonAnim->SetTrackViewExclusive(1);

			m_bPlayingAnimationAtLayer[layer] = true;
			m_nPlayingAnimations++;

			// Start playing animation.
			CryCharAnimationParams aparams;
			aparams.m_fTransTime = key.m_blendTime;

			aparams.m_nFlags = CA_TRACK_VIEW_EXCLUSIVE;
			if (key.m_bLoop)
				aparams.m_nFlags |= CA_LOOP_ANIMATION;
	
			aparams.m_nLayerID = layer+0;
			pISkeletonAnim->StartAnimation( key.m_animation,  aparams );

			int animId = pAnimations->GetAnimIDByName( key.m_animation );
			if (animId >= 0)
			{
				//pCharacter->EnableTimeUpdate(false);
				//pCharacter->SetAnimationSpeed(layer, 0.0000f);
				//float duration = pAnimations->GetLength(animId);
				float duration = pAnimations->GetDuration_sec(animId);// + pAnimations->GetStart(animId);
				if (duration != key.m_duration)
				{
					key.m_duration = duration;
					track->SetKey( currKey,&key );
				}
				/*
				if (key.bLoop)
				{
				if (animId >= 0)
				pAnimations->SetLoop( animId,key.bLoop );
				}
				*/
			}
		}
	}

	bool bSomeKeyActive = false;

	// Animate.
	if (currKey >= 0)
	{
		float t = ec.time - key.time;
		t = key.m_startTime + t*key.m_speed;
		if ((t < key.m_duration || key.m_bLoop) && key.m_duration > 0.0f)
		{
			bSomeKeyActive = true;
			if (key.m_bLoop && key.m_duration > 0.0f)
			{
				t = fmod(t,key.m_duration);
			}
			pCharacter->SetAnimationSpeed(0.0000f);
			if (ec.dt > 0.0001f && key.m_duration > 0.0001f) // This should stop this call if the user is moving the time in the editor.
				pISkeletonAnim->ManualSeekAnimationInFIFO(0, 0, t/key.m_duration, true); // Make sure anim events get triggered.

			f32 fNormalizedTime = t/key.m_duration;
			assert(fNormalizedTime>=0.0f && fNormalizedTime<=1.0f);
			pISkeletonAnim->SetLayerTime(layer+0,fNormalizedTime);
			pISkeletonAnim->SetLayerTime(layer+1,fNormalizedTime);
			pISkeletonAnim->SetLayerTime(layer+2,fNormalizedTime);
		}
	}

	// Stop animation.
	if (!bSomeKeyActive && m_bPlayingAnimationAtLayer[layer])
	{
		m_bPlayingAnimationAtLayer[layer] = false;
		m_nPlayingAnimations--;

		if (m_nPlayingAnimations == 0)
		{
			// Not inside animation.
			pCharacter->GetISkeletonAnim()->SetTrackViewExclusive(0);
			
			pCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();

			pCharacter->SetAnimationSpeed(1.0000f);
			pCharacter->GetISkeletonAnim()->SetAnimationDrivenMotion(m_bWasTransRot);
		}
	}

}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::StopExpressions()
{
	if (m_setExpressions.empty())
		return;

	IEntity *pEntity = GetEntity();
	if (!pEntity)
		return;

	/*
	IEntityCharacter *pEntChar=m_entity->GetCharInterface();
	if (!pEntChar)
		return;
	ILipSync *pLipSync=pEntChar->GetLipSyncInterface();
	if (!pLipSync)
		return;
	for (TStringSetIt It=m_setExpressions.begin();It!=m_setExpressions.end();++It)
	{
		pLipSync->StopExpression((*It).c_str());
	}
	*/
	ICharacterInstance* pInst = pEntity->GetCharacter(0);
	if (pInst)
	{
   pInst->GetIMorphing()->StopAllMorphs();
	}

	m_setExpressions.clear();
}

void CAnimEntityNode::AnimateExpressionTrack(CExprTrack *pTrack, SAnimContext &ec)
{
	IEntity *pEntity = GetEntity();
	if (!pEntity)
		return;

	IExprKey Key;
	int nKeys=pTrack->GetNumKeys();
	// we go through all the keys, since expressions may overlap
	for (int nKey=0;nKey<nKeys;nKey++)
	{
		pTrack->GetKey(nKey, &Key);
		if ((!Key.pszName) || (!Key.pszName[0]))
			return;

		float fKeyLentgh=Key.fBlendIn+Key.fHold+Key.fBlendOut;
		float fOffset=ec.time-Key.time;

		CryCharMorphParams MorphParams;
		MorphParams.m_fAmplitude=Key.fAmp;
		MorphParams.m_fBlendIn=Key.fBlendIn;
		MorphParams.m_fBlendOut=Key.fBlendOut;
		MorphParams.m_fLength=Key.fHold;
		MorphParams.m_fStartTime=fOffset;
		//MorphParams.m_nFlags |= MorphParams.FLAGS_RECURSIVE;
		ICharacterInstance* pInst = pEntity->GetCharacter(0);
		if (pInst)
		{
	//		pInst->StartMorph (Key.pszName, MorphParams);

	//	bool CLipSync::DoExpression(const char* pszMorphTarget, CryCharMorphParams &MorphParams, bool bAnim)
	//	{
	//		if (!m_pCharInst)
	//			return false; // not initialized
	//		ICryCharModel *pModel=m_pCharInst->GetModel();
	//		ASSERT(pModel);
	//		IAnimationSet *pAnimSet=pModel->GetAnimationSet();
	//		ASSERT(pAnimSet);
	//		int nMorphTargetId=pAnimSet->FindMorphTarget(pszMorphTarget);
	//		if (nMorphTargetId==-1)
	//			return false;	// no such morph-target
			// try to set time first in case it is already playing
			//	if (!m_pCharInst->SetMorphTime(nMorphTargetId, MorphParams.fStartTime))

			if ((Key.time>ec.time) || (fOffset>=fKeyLentgh))
			{
				continue;
			}
			
			IMorphing* pIMorphing = pInst->GetIMorphing();
			pIMorphing->StopMorph(Key.pszName);
			pIMorphing->StartMorph(Key.pszName, MorphParams);
			pIMorphing->SetMorphSpeed(Key.pszName, 0.0f);
		}

		/*
		IEntityCharacter *pEntChar=m_entity->GetCharInterface();
		if (!pEntChar)
			return;
		ILipSync *pLipSync=pEntChar->GetLipSyncInterface();
		if (pLipSync)
		{
			float fKeyLentgh=Key.fBlendIn+Key.fHold+Key.fBlendOut;
			float fOffset=ec.time-Key.time;
			if ((Key.time>ec.time) || (fOffset>=fKeyLentgh))
			{
//				pLipSync->StopExpression(Key.pszName);
				continue;
			}
			CryCharMorphParams MorphParams;
			MorphParams.fAmplitude=Key.fAmp;
			MorphParams.fBlendIn=Key.fBlendIn;
			MorphParams.fBlendOut=Key.fBlendOut;
			MorphParams.fLength=Key.fHold;
			MorphParams.fStartTime=fOffset;
			if (pLipSync->DoExpression(Key.pszName, MorphParams, false))
				m_setExpressions.insert(Key.pszName);
		}
		*/
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::AnimateFacialSequence( CFaceSeqTrack *pTrack, SAnimContext &ec )
{
	IEntity *pEntity = GetEntity();
	if (!pEntity)
		return;

	ICharacterInstance *pCharacter = pEntity->GetCharacter(0);
	if (!pCharacter)
		return;

	IFacialInstance *pFaceInstance = pCharacter->GetFacialInstance();
	if (!pFaceInstance)
		return;

	IFaceSeqKey key;
	int nkey = pTrack->GetActiveKey(ec.time,&key);
	if (nkey >= 0 && nkey < pTrack->GetNumKeys())
	{
		_smart_ptr<IFacialAnimSequence> pSeq = LoadFacialSequence(key.szSelection);
		if (pSeq)
		{
			key.fDuration = pSeq->GetTimeRange().Length();
			pTrack->SetKey(nkey, &key);

			float t = ec.time - key.time;
			if (t <= key.fDuration)
			{
				if (!pFaceInstance->IsPlaySequence(pSeq, eFacialSequenceLayer_Trackview))
				{
					pFaceInstance->PlaySequence(pSeq, eFacialSequenceLayer_Trackview);
					pFaceInstance->PauseSequence(eFacialSequenceLayer_Trackview, true);
				}
				pFaceInstance->SeekSequence(eFacialSequenceLayer_Trackview,t);
			}
			else
			{
				if (pFaceInstance->IsPlaySequence(pSeq, eFacialSequenceLayer_Trackview))
					pFaceInstance->StopSequence(eFacialSequenceLayer_Trackview);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::AnimateLookAt( CLookAtTrack *pTrack, SAnimContext &ec )
{
	IEntity *pEntity = GetEntity();
	if (!pEntity)
		return;

	ICharacterInstance *pCharacter = 0;

	IPipeUser* pAIPipeUser = NULL;
	IAIObject* pAI = pEntity->GetAI();
	if (pAI)
	{
		pAIPipeUser = pAI->CastToIPipeUser();
	}
	if (!pAIPipeUser)
		pCharacter = pEntity->GetCharacter(0);

	EntityId lookAtEntityId = 0;
	bool allowAdditionalTransforms = 0;
	ELookAtKeyBoneSet boneSet = eLookAtKeyBoneSet_HeadEyes;

	ILookAtKey key;
	int nkey = pTrack->GetActiveKey(ec.time,&key);
	allowAdditionalTransforms = true;
	boneSet = eLookAtKeyBoneSet_HeadEyes;
	if (nkey >= 0)
	{
		allowAdditionalTransforms = key.bAllowAdditionalTransforms;
		boneSet = key.boneSet;
		if ((m_lookAtTarget[0] && !m_lookAtEntityId) || strcmp(key.szSelection,m_lookAtTarget) != 0)
		{
			m_lookAtTarget = key.szSelection;
			IEntity *pTargetEntity = gEnv->pEntitySystem->FindEntityByName(key.szSelection);
			if (pTargetEntity)
			{
				lookAtEntityId = pTargetEntity->GetId();
			}
			else
				lookAtEntityId = 0;
		}
		else
		{
			lookAtEntityId = m_lookAtEntityId;
		}
	}
	else
	{
		lookAtEntityId = 0;
		m_lookAtTarget = "";
		allowAdditionalTransforms = true;
		boneSet = eLookAtKeyBoneSet_HeadEyes;
	}

	if (m_lookAtEntityId != lookAtEntityId 
	|| m_allowAdditionalTransforms != allowAdditionalTransforms 
	|| m_boneSet != boneSet)
	{
		m_lookAtEntityId = lookAtEntityId;
		m_allowAdditionalTransforms = allowAdditionalTransforms;
		m_boneSet = boneSet;

		// We need to enable smoothing for the facial animations, since look ik can override them and cause snapping.
		IFacialInstance* pFacialInstance = (pCharacter ? pCharacter->GetFacialInstance() : 0);
		if (pFacialInstance)
			pFacialInstance->TemporarilyEnableBoneRotationSmoothing();
	}

	IEntity *pLookAtEntity = 0;
	if (m_lookAtEntityId)
		pLookAtEntity = gEnv->pEntitySystem->GetEntity(m_lookAtEntityId);

	if (pLookAtEntity)
	{
		Vec3 pos = pLookAtEntity->GetWorldPos();
		ICharacterInstance *pLookAtChar = pLookAtEntity->GetCharacter(0);
		if (pLookAtChar)
		{
			// Try look at head bone.
			int16 nHeadBoneId = pLookAtChar->GetISkeletonPose()->GetJointIDByName( HEAD_BONE_NAME );
			if (nHeadBoneId >= 0)
			{
				pos = pLookAtEntity->GetWorldTM().TransformPoint( pLookAtChar->GetISkeletonPose()->GetAbsJointByID(nHeadBoneId).t );
			//	gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere( pos,0.2f,ColorB(255,0,0,255) );
			}
		}
		float blends[5];
		SelectLookIKBlends(m_boneSet, blends);
		if (pCharacter)
			pCharacter->GetISkeletonPose()->SetLookIK( true,DEG2RAD(120),pos,blends,m_allowAdditionalTransforms );
		if (pAIPipeUser)
			pAIPipeUser->SetLookAtPointPos(pos);
	}
	else
	{
		if (pCharacter)
			pCharacter->GetISkeletonPose()->SetLookIK( false,false,Vec3(0,0,0) );
		if (pAIPipeUser)
			pAIPipeUser->ResetLookAt();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::Serialize( XmlNodeRef &xmlNode,bool bLoading,bool bLoadEmptyTracks )
{
	CAnimNode::Serialize( xmlNode,bLoading,bLoadEmptyTracks );
	if (bLoading)
	{
		xmlNode->getAttr( "Pos",m_pos );
		xmlNode->getAttr( "Rotate",m_rotate );
		xmlNode->getAttr( "Scale",m_scale );
		xmlNode->getAttr( "EntityGUID",m_entityGuid );
		
		xmlNode->getAttr( "EntityGUIDTarget",m_entityGuidTarget );
		xmlNode->getAttr( "EntityGUIDSource",m_entityGuidSource );
	}
	else
	{
		xmlNode->setAttr( "Pos",m_pos );
		xmlNode->setAttr( "Rotate",m_rotate );
		xmlNode->setAttr( "Scale",m_scale );

		xmlNode->setAttr( "EntityGUID",m_entityGuid );

		if(m_entityGuidTarget)
			xmlNode->setAttr( "EntityGUIDTarget",m_entityGuidTarget );
		if(m_entityGuidSource)
			xmlNode->setAttr( "EntityGUIDSource",m_entityGuidSource );
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimEntityNode::OffsetPosTrack( const Vec3 &offset )
{

}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CAnimEntityNode::SaveToColladaInFixedFPS(float fps) const
{
	XmlNodeRef colladaNode = CAnimNode::SaveToColladaInFixedFPS(fps);

	// <library_visual_scenes>
	XmlNodeRef libraryVisualScenesNode = colladaNode->newChild("library_visual_scenes");
		XmlNodeRef visualSceneNode = libraryVisualScenesNode->newChild("visual_scene");
		visualSceneNode->setAttr("id", "RootNode");
		visualSceneNode->setAttr("name", "RootNode");
			XmlNodeRef node = visualSceneNode->newChild("node");
			node->setAttr("id", m_name);
			node->setAttr("name", m_name);
				XmlNodeRef translateNode = node->newChild("translate");
				translateNode->setAttr("sid", "translate");
				translateNode->setContent("0 0 0");
				XmlNodeRef rotateNode = node->newChild("rotate");
				rotateNode->setAttr("sid", "rotateZ");
				rotateNode->setContent("0 0 1 0");
				rotateNode = node->newChild("rotate");
				rotateNode->setAttr("sid", "rotateY");
				rotateNode->setContent("0 1 0 0");
				rotateNode = node->newChild("rotate");
				rotateNode->setAttr("sid", "rotateX");
				rotateNode->setContent("1 0 0 0");

	// <scene>
	XmlNodeRef sceneNode = colladaNode->newChild("scene");
		sceneNode->newChild("instance_visual_scene")->setAttr("url", "#RootNode");
	
	return colladaNode;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimEntityNode::LoadFromCollada(XmlNodeRef colladaNode)
{
	if (CAnimNode::LoadFromCollada(colladaNode) == false)
		return false;

	// Further sanity checks
	XmlNodeRef libraryVisualScenesNode = colladaNode->findChild("library_visual_scenes");
	if (!libraryVisualScenesNode)
		return false;
	XmlNodeRef visualSceneNode = libraryVisualScenesNode->findChild("visual_scene");
	if (!visualSceneNode)
		return false;
	XmlString rootID;
	if (visualSceneNode->getAttr("id", rootID) == false)
		return false;
	XmlNodeRef sceneNode = colladaNode->findChild("scene");
	if (!sceneNode)
		return false;
	XmlString sceneUrl;
	if (sceneNode->findChild("instance_visual_scene")->getAttr("url", sceneUrl) == false)
		return false;
	rootID = ("#" + rootID).c_str();
	if (rootID != sceneUrl)
		return false;

	// Get the node name.
	XmlNodeRef node = visualSceneNode->findChild("node");
	if (!node)
		return false;
	XmlString nodeID;
	node->getAttr("id", nodeID);

	// Create pos, rot tracks if not made already.
	if (GetTrackForParameter(APARAM_POS) == NULL)
		CreateTrack(APARAM_POS);
	if (GetTrackForParameter(APARAM_ROT) == NULL)
		CreateTrack(APARAM_ROT);
	IAnimTrack *pPosTrack = GetTrackForParameter(APARAM_POS);
	IAnimTrack *pRotTrack = GetTrackForParameter(APARAM_ROT);
	// Only CompoundSplineTracks are supported currently.
	if (pPosTrack->GetSubTrackCount() != 3
	|| pRotTrack->GetSubTrackCount() != 3)
	 return false;

	// Find the animation data which are channeled to the node.
	XmlNodeRef libraryAnimationsNode = colladaNode->findChild("library_animations");
	if (!libraryAnimationsNode)
		return false;
	XmlNodeRef parentNode = libraryAnimationsNode;
	// It can be the case of animations right under the library tag,
	// or the case of animations under another parent animation tag inside the library tag.
	if (libraryAnimationsNode->getChildCount() == 1
	&& libraryAnimationsNode->getChild(0)->findChild("channel") == 0)
		parentNode = libraryAnimationsNode->getChild(0);
	nodeID += "/";
	for (int i=0; i<parentNode->getChildCount(); ++i)
	{
		XmlNodeRef animationNode = parentNode->getChild(i);
		XmlNodeRef channelNode = animationNode->findChild("channel");
		if (!channelNode)
			continue;
		XmlString channelTarget;
		if (channelNode->getAttr("target", channelTarget) == false)
			continue;
		if (channelTarget.substr(0, nodeID.length()) != nodeID)
			continue;
		// Now we've found a valid animation data for this node.
		XmlString whichChannel(channelTarget.substr(nodeID.length()));
		if (whichChannel == "translate.X")
		{
			pPosTrack->GetSubTrack(0)->LoadFromCollada(animationNode);
		}
		else if (whichChannel == "translate.Y")
		{
			pPosTrack->GetSubTrack(1)->LoadFromCollada(animationNode);
		}
		else if (whichChannel == "translate.Z")
		{
			pPosTrack->GetSubTrack(2)->LoadFromCollada(animationNode);
		}
		else if (whichChannel == "rotateX.ANGLE")
		{
			pRotTrack->GetSubTrack(0)->LoadFromCollada(animationNode);
		}
		else if (whichChannel == "rotateY.ANGLE")
		{
			pRotTrack->GetSubTrack(1)->LoadFromCollada(animationNode);
		}
		else if (whichChannel == "rotateZ.ANGLE")
		{
			pRotTrack->GetSubTrack(2)->LoadFromCollada(animationNode);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CAnimEntityNode::CreateTrack(int nParamType)
{
	IAnimTrack *pTrack = CAnimNode::CreateTrack(nParamType);
	
	if (nParamType == APARAM_NOISE)
	{
		if (pTrack)
		{
			pTrack->SetSubTrackName(0, "Pos Noise Amp");
			pTrack->SetSubTrackName(1, "Pos Noise Freq");
			pTrack->SetSubTrackName(2, "Rot Noise Amp");
			pTrack->SetSubTrackName(3, "Rot Noise Freq");
		}
	}
	return pTrack;
}

//////////////////////////////////////////////////////////////////////////
Vec3 CAnimEntityNode::Noise::Get(float time) const
{
	Vec3 noise;

	float phase = time*freq;

	Vec3 phase0 
		= Vec3(15.0f*freq, 55.1f*freq, 101.2f*freq);

	noise.x = gEnv->pSystem->GetNoiseGen()->Noise1D(phase + phase0.x)*amp;
	noise.y = gEnv->pSystem->GetNoiseGen()->Noise1D(phase + phase0.y)*amp;
	noise.z = gEnv->pSystem->GetNoiseGen()->Noise1D(phase + phase0.z)*amp;

	return noise;
}

#undef s_nodeParamsInitialized
#undef s_nodeParams
#undef AddSupportedParam

