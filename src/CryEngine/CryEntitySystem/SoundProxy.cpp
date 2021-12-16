////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   SoundProxy.cpp
//  Version:     v1.00
//  Created:     26/9/2004 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <ICryAnimation.h>
#include <IPhysics.h>
#include "SoundProxy.h"
#include "Entity.h"
#include "ISerialize.h"
#include "ISound.h"
#include "IReverbManager.h"
#include <IFacialAnimation.h>
#include "IRenderer.h"
#include "Area.h"
#include <Cry_Camera.h>

#define TOO_MANY_SOUNDS 20

#ifdef WIN64
	#pragma warning ( disable : 4244 )
#endif

//////////////////////////////////////////////////////////////////////////
CSoundProxy::CSoundProxy( CEntity *pEntity )
{
	m_bHide = pEntity->IsHidden();
	m_nFlags = 0;
	m_SoundsAttached.clear();
	m_pEntity = pEntity;
	m_fEffectRadius = 0.0f;
	m_currentLipSyncId = INVALID_SOUNDID;
	m_vLastVisibilityPos = Vec3(0);
	m_sTailname = SOUND_OUTDOOR_TAILNAME;
	m_Headtm = m_pEntity->GetWorldTM();
	m_pSkeletonPose = NULL;
	m_nBoneHead = -1;
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::Release()
{
	StopAllSounds();
	m_pEntity = NULL;
	delete this;
}
void CSoundProxy::UpdateSounds()
{
	OnMove();
}

void CSoundProxy::PrecacheHeadBone()
{
	// checking for and caching HeadBone
	if (m_nBoneHead == -1)
	{
		ICharacterInstance * pCharacter = m_pEntity->GetCharacter(0);
		if (pCharacter)
		{
			m_pSkeletonPose = pCharacter->GetISkeletonPose();

			if (m_pSkeletonPose)
				m_nBoneHead = m_pSkeletonPose->GetJointIDByName("Bip01 Head");
		}
	}
}

void CSoundProxy::SetSoundPosition(ISound *pSound, const Vec3 *vOffset, const Vec3 *vDirection)
{
	const Matrix34 &tm = m_pEntity->GetWorldTM();

	// Only set position on non-self-moving sounds
	if (!(pSound->GetFlags() & FLAG_SOUND_SELFMOVING))
	{
		if (vOffset && vDirection)
		{
			Matrix34 tmOffs;
			tmOffs.SetIdentity();

			if (pSound->GetFlags() & FLAG_SOUND_VOICE)
			{
				// re-query SkeletonPose to prevent crash on removed Character
				ICharacterInstance * pCharacter = m_pEntity->GetCharacter(0);

				if (pCharacter)
					m_pSkeletonPose = pCharacter->GetISkeletonPose();
				else
					m_pSkeletonPose = NULL;

				if (m_nBoneHead != -1 && m_pSkeletonPose)
					m_Headtm = tm * Matrix34(m_pSkeletonPose->GetAbsJointByID(m_nBoneHead));
				else
					m_Headtm = tm;

				tmOffs = Matrix33::CreateRotationVDir(*vDirection);
				tmOffs.SetTranslation(*vOffset);
				tmOffs = m_Headtm * tmOffs;
				//tmOffs = m_Headtm + vDirection;
			}
			else
			{
				tmOffs.SetTranslation(*vOffset);
				tmOffs = tm * tmOffs;
			}

			pSound->SetPosition( tmOffs.GetTranslation() );
			pSound->SetMatrix(tmOffs);
		}
		else
		{
			if (pSound->GetFlags() & FLAG_SOUND_VOICE)
			{
				// re-query SkeletonPose to prevent crash on removed Character
				ICharacterInstance * pCharacter = m_pEntity->GetCharacter(0);
				
				if (pCharacter)
					m_pSkeletonPose = pCharacter->GetISkeletonPose();
				else
					m_pSkeletonPose = NULL;

				if (m_nBoneHead != -1 && m_pSkeletonPose)
					m_Headtm = tm * Matrix34(m_pSkeletonPose->GetAbsJointByID(m_nBoneHead));
				else
					m_Headtm = tm;
				
				pSound->SetPosition( m_Headtm.GetTranslation() );
				pSound->SetMatrix(m_Headtm);
			}
			else
			{
				pSound->SetPosition( tm.GetTranslation() );
				pSound->SetMatrix(tm);
			}

			
		}
	}

}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::OnMove()
{
	const Matrix34 &tm = m_pEntity->GetWorldTM();

	// put some debug output here:
	//IRenderer *pRenderer = gEnv->pRenderer;
	//float colorwhite[4]={1,1,1,1};
	//string sOutput= m_SoundsAttached.size() + " Proxy Sounds: \n";
#ifdef USER_TOMAS	
	// this code is to detect lost sounds on Sound Proxies
	// enable for debugging
	std::vector<tSoundID> ToBeDeletedSoundID;
#endif

	//////////////////////////////////////////////////////////////////////////
	AttachedSoundsVec::iterator itEnd = m_SoundsAttached.end();
	for (AttachedSoundsVec::iterator it = m_SoundsAttached.begin(); it != itEnd; ++it)
	{
		const SAttachedSound &slot = (*it);

		SetSoundPosition(slot.pSound, slot.bOffset?&slot.vOffset:0, &slot.vDirection);

#ifdef USER_TOMAS	
		// this code is to detect lost sounds on Sound Proxies
		// enable for debugging
		if (slot.pSound && !slot.bStatic)
		{
			if (!slot.pSound->IsPlaying())
			{
				// found a sound which is supposed to be removed already
				ToBeDeletedSoundID.push_back(slot.nSoundID);
			}
		}
#endif

		//sOutput += slot.pSound->GetName() + "\n";
	}

#ifdef USER_TOMAS	
	// this code is to detect lost sounds on Sound Proxies
	// enable for debugging
	std::vector<tSoundID>::const_iterator IDItEnd = ToBeDeletedSoundID.end();
	for (std::vector<tSoundID>::const_iterator IDIt = ToBeDeletedSoundID.begin(); IDIt != IDItEnd; ++IDIt)
	{
		RemoveAttachedSound((*IDIt));
	}
#endif
	

	//if (pRenderer)
	//{
	//	pRenderer->DrawLabelEx(tm.GetTranslation(), 1.3f, colorwhite, true, false, sOutput.c_str());
	//}
}


//////////////////////////////////////////////////////////////////////////
void CSoundProxy::OnLeaveNear(IEntity *pWho, IEntity *pArea)
{
	// not needed
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::OnMoveInside(IEntity *pWho, IEntity *pArea)
{
	IListener *pListener = gEnv->pSoundSystem->GetListener(LISTENERID_STANDARD);

	if (pListener && pListener->GetEntityID() == pWho->GetId())
		m_pEntity->SetPos(pListener->GetPosition());
	else
		m_pEntity->SetPos(pWho->GetWorldPos());

	//////////////////////////////////////////////////////////////////////////
	AttachedSoundsVec::iterator itEnd = m_SoundsAttached.end();
	for (AttachedSoundsVec::iterator it = m_SoundsAttached.begin(); it != itEnd; ++it)
	{
		const SAttachedSound &slot = (*it);
		slot.pSound->GetInterfaceExtended()->SetVolume(1.0f);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::OnExclusiveMoveInside(IEntity *pWho, IEntity *pAreaLow, IEntity *pAreaHigh)
{
	IEntityAreaProxy *pAreaProxyLow		= (IEntityAreaProxy*)pAreaLow->GetProxy(ENTITY_PROXY_AREA);
	IEntityAreaProxy *pAreaProxyHigh	= (IEntityAreaProxy*)pAreaHigh->GetProxy(ENTITY_PROXY_AREA);
	IListener *pListener = gEnv->pSoundSystem->GetListener(LISTENERID_STANDARD);
	Vec3 OnHighHull3d;

	if (!pAreaProxyLow || !pAreaProxyHigh)
		return;

	if (pListener && pListener->GetEntityID() == pWho->GetId())
	{
		bool bInsideLow = pAreaProxyLow->CalcPointWithin(pListener->GetPosition());
		if (bInsideLow)
		{
			pAreaProxyHigh->ClosestPointOnHullDistSq(pListener->GetPosition(), OnHighHull3d);
			m_pEntity->SetPos(OnHighHull3d);
		}
	}
	else
	{
		bool bInsideLow = pAreaProxyLow->CalcPointWithin(pWho->GetWorldPos());
		if (bInsideLow)
		{
			pAreaProxyHigh->ClosestPointOnHullDistSq(pWho->GetWorldPos(), OnHighHull3d);
			m_pEntity->SetPos(OnHighHull3d);
		}
		
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::OnEnter(IEntity *pWho, IEntity *pArea)
{
	IListener *pListener = gEnv->pSoundSystem->GetListener(LISTENERID_STANDARD);

	if (pListener && pListener->GetEntityID() == pWho->GetId())
		m_pEntity->SetPos(pListener->GetPosition());
	else
		m_pEntity->SetPos(pWho->GetWorldPos());

	//////////////////////////////////////////////////////////////////////////
	AttachedSoundsVec::iterator itEnd = m_SoundsAttached.end();
	for (AttachedSoundsVec::iterator it = m_SoundsAttached.begin(); it != itEnd; ++it)
	{
 		const SAttachedSound &slot = (*it);

		//slot.pSound->SetFlags(slot.pSound->GetFlags() | FLAG_SOUND_RELATIVE);
		
		// update spread
		if (slot.pSound->GetFlags() & FLAG_SOUND_PARAM_SPREAD)
			slot.pSound->SetParam("spread", 0.0f, false);

		//slot.pSound->SetVolume(1.0f);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::OnLeave(IEntity *pWho, IEntity *pArea)
{
	// not needed
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::OnMoveNear(IEntity *pWho, IEntity *pArea, float fFade, float fDistanceSq)
{
	IEntityAreaProxy *pAreaProxy = (IEntityAreaProxy*)pArea->GetProxy(ENTITY_PROXY_AREA);
	Vec3 OnHull3d=Vec3(ZERO);
	float fDistsq = 0;
	IListener *pListener = gEnv->pSoundSystem->GetListener(LISTENERID_STANDARD);

	if (!pAreaProxy)
		return;

	// TODO: Check use of cached data
	if (pListener && pListener->GetEntityID() == pWho->GetId())
		fDistsq = pAreaProxy->CalcPointNearDistSq(pListener->GetPosition(), OnHull3d);
	else
		fDistsq = pAreaProxy->CalcPointNearDistSq(pWho->GetWorldPos(), OnHull3d);
	(void) fDistsq;

	m_pEntity->SetPos(OnHull3d);
	//////////////////////////////////////////////////////////////////////////
	AttachedSoundsVec::iterator itEnd = m_SoundsAttached.end();
	for (AttachedSoundsVec::iterator it = m_SoundsAttached.begin(); it != itEnd; ++it)
	{
		const SAttachedSound &slot = (*it);

		float fVolume = 0.0f;

		if (m_fEffectRadius > 0.0f)
			fVolume = (1.0f - (cry_sqrtf(fDistanceSq) * __fres(m_fEffectRadius) ) );
		else
			fVolume = 1.0f;

		// update spread
		if (slot.pSound->GetFlags() & FLAG_SOUND_PARAM_SPREAD)
			slot.pSound->SetParam("spread", 1.0f - fVolume, false);

		fVolume = clamp(fVolume*fabsf(fFade), 0.0f, 1.0f);
		slot.pSound->GetInterfaceExtended()->SetVolume(fVolume);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::OnHide( bool bHide )
{
	if (m_bHide != bHide)
	{
		m_bHide = bHide;

		//////////////////////////////////////////////////////////////////////////
		AttachedSoundsVec::reverse_iterator itEndR = m_SoundsAttached.rend();
		for (AttachedSoundsVec::reverse_iterator itR = m_SoundsAttached.rbegin(); itR != itEndR; ++itR)
		{
			// going through backwards to prevent double deletion
			//gEnv->pLog->Log(">>>>>> %p %p %d\n",this,&m_SoundsAttached[0],m_SoundsAttached.size());

			const SAttachedSound &slot = (*itR);
			assert (NULL != slot.pSound);
			gEnv->pSoundSystem->GetInterfaceDeprecated()->SetSoundActiveState(slot.pSound, bHide?eSoundState_None:eSoundState_Active);
		}
		//OnEntityXForm();

	}
}
//////////////////////////////////////////////////////////////////////////
void CSoundProxy::ProcessEvent( SEntityEvent &event )
{

	switch(event.event) {
	case ENTITY_EVENT_XFORM:
		{
			OnMove();
			break;
		}
	case ENTITY_EVENT_ENTERAREA:
		{
			if (!(m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND))
				break;

			EntityId who = event.nParam[0]; // player

			IListener *pListener = gEnv->pSoundSystem->GetListener(LISTENERID_STANDARD);
			IEntity *pWho = gEnv->pEntitySystem->GetEntity(who);
			bool bIsCamera = (pWho && pWho->GetClass() && strcmp(pWho->GetClass()->GetName(),"CameraSource") == 0 );

			if (pListener && pListener->GetEntityID() == who || bIsCamera)
			{
				// the player is a registered listener, so essentially its the local actor
				EntityId area = event.nParam[2]; // AreaEntityID
				IEntity *pArea = gEnv->pEntitySystem->GetEntity(area);

				if (pWho && pArea)
					OnEnter(pWho, pArea);
			}
			break;
		}
	case ENTITY_EVENT_LEAVEAREA:
		{
			break;
		}
	case ENTITY_EVENT_ENTERNEARAREA:
		{
			if (!(m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND))
				break;

			EntityId who = event.nParam[0]; // player

			IListener *pListener = gEnv->pSoundSystem->GetListener(LISTENERID_STANDARD);
			IEntity *pWho = gEnv->pEntitySystem->GetEntity(who);
			bool bIsCamera = (pWho && pWho->GetClass() && strcmp(pWho->GetClass()->GetName(),"CameraSource") == 0 );

			if (pListener && pListener->GetEntityID() == who || bIsCamera)
			{
				// the player is a registered listener, so essentially its the local actor
				EntityId area = event.nParam[2]; // AreaEntityID
				IEntity *pArea = gEnv->pEntitySystem->GetEntity(area);

				if (pWho && pArea)
					OnMoveNear(pWho, pArea, event.fParam[0], event.fParam[1]);
			}
			break;
		}
	case ENTITY_EVENT_LEAVENEARAREA:
		{
			break;
		}
	case ENTITY_EVENT_MOVEINSIDEAREA:
		{
			if (!(m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND))
				break;

			EntityId who = event.nParam[0]; // player

			IListener *pListener = gEnv->pSoundSystem->GetListener(LISTENERID_STANDARD);
			IEntity *pWho = gEnv->pEntitySystem->GetEntity(who);
			bool bIsCamera = (pWho && pWho->GetClass() && strcmp(pWho->GetClass()->GetName(),"CameraSource") == 0 );

			if (pListener && pListener->GetEntityID() == who || bIsCamera)
			{
				// the player is a registered listener, so essentially its the local actor
				EntityId AreaID1 = event.nParam[2]; // AreaEntityID (low)
				EntityId AreaID2 = event.nParam[3]; // AreaEntityID (high)

				IEntity *pArea1 = gEnv->pEntitySystem->GetEntity(AreaID1);
				IEntity *pArea2 = gEnv->pEntitySystem->GetEntity(AreaID2);

				if (pWho && pArea1)
				{
					if(pArea2)
						OnExclusiveMoveInside(pWho, pArea1, pArea2);
					else
						OnMoveInside(pWho, pArea1);
				}
			}
			break;
		}
	//case ENTITY_EVENT_EXCLUSIVEMOVEINSIDEAREA:
	//	{
	//		if (!(m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND))
	//			break;

	//		EntityId who = event.nParam[0]; // player

//			SListener *pListener = gEnv->pSoundSystem->GetListener(LISTENERID_STANDARD);
//			IEntity *pWho = gEnv->pEntitySystem->GetEntity(who);
//			bool bIsCamera = (pWho && pWho->GetClass() && strcmp(pWho->GetClass()->GetName(),"CameraSource") == 0 );

//			if (pListener && pListener->nEntityID==who || bIsCamera)
//			{
//				// the player is a registered listener, so essentially its the local actor
//				EntityId AreaHigh = event.nParam[2];	// AreaEntityID of higher area
//				EntityId AreaLow = event.nParam[3];		// AreaEntityID of lower area
//				
//				IEntity *pAreaHigh = gEnv->pEntitySystem->GetEntity(AreaHigh);
//				IEntity *pAreaLow = gEnv->pEntitySystem->GetEntity(AreaLow);

//				if (pWho && pAreaHigh && pAreaLow)
//					OnExclusiveMoveInside(pWho, pAreaHigh, pAreaLow);
//			}
	//		break;
	//	}
	case ENTITY_EVENT_MOVENEARAREA:
		{
			if (!(m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND))
				break;

			EntityId who = event.nParam[0]; // player

			IListener *pListener = gEnv->pSoundSystem->GetListener(LISTENERID_STANDARD);
			IEntity *pWho = gEnv->pEntitySystem->GetEntity(who);
			bool bIsCamera = (pWho && pWho->GetClass() && strcmp(pWho->GetClass()->GetName(),"CameraSource") == 0 );

			if (pListener && pListener->GetEntityID() == who || bIsCamera)
			{
				// the player is a registered listener, so essentially its the local actor
				EntityId area = event.nParam[2]; // AreaEntityID
				IEntity *pArea = gEnv->pEntitySystem->GetEntity(area);

				if (pWho && pArea)
					OnMoveNear(pWho, pArea, event.fParam[0], event.fParam[1]);
			}
			break;
		}
	case ENTITY_EVENT_ANIM_EVENT:
		{
			if (!IsSoundAnimEventsHandledExternally())
			{
				const AnimEventInstance* pAnimEvent = reinterpret_cast<const AnimEventInstance*>(event.nParam[0]);
				ICharacterInstance* pCharacter = reinterpret_cast<ICharacterInstance*>(event.nParam[1]);
				const char* eventName = (pAnimEvent ? pAnimEvent->m_EventName : 0);
				if (eventName && stricmp(eventName, "sound") == 0)
				{
					Vec3 offset(ZERO);
					if (pAnimEvent->m_BonePathName && pAnimEvent->m_BonePathName[0])
					{
						ISkeletonPose* pSkeletonPose = (pCharacter ? pCharacter->GetISkeletonPose() : 0);

						int id = (pSkeletonPose ? pSkeletonPose->GetJointIDByName(pAnimEvent->m_BonePathName) : -1);
						if (pSkeletonPose && id >= 0)
						{
							QuatT boneQuat(pSkeletonPose->GetAbsJointByID(id));
							offset = boneQuat.t;
						}
					}

					int flags = FLAG_SOUND_DEFAULT_3D;
					if (strchr(pAnimEvent->m_CustomParameter, ':') == NULL)
						flags |= FLAG_SOUND_VOICE;
					PlaySound(pAnimEvent->m_CustomParameter, offset, FORWARD_DIRECTION, flags, eSoundSemantic_Animation, 0, 0);
				}
			}
		}
		break;
	case ENTITY_EVENT_HIDE:
		OnHide(true);
		break;
	case ENTITY_EVENT_UNHIDE:
		OnHide(false);
		break;
	case ENTITY_EVENT_INVISIBLE:
		break;
	case ENTITY_EVENT_VISIBLE:
		break;
	case ENTITY_EVENT_INIT:
	case ENTITY_EVENT_DONE:
	case ENTITY_EVENT_PRE_SERIALIZE:
		StopAllSounds();
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::Serialize( TSerialize ser )
{
#ifdef USER_TOMAS
	//this is a good test to check if SoundProxies cleaned up their sounds correctly
	// activate for debug
	int nRefCount = 0;
	AttachedSoundsVec::iterator ItEnd = m_SoundsAttached.end();
	for (AttachedSoundsVec::iterator It = m_SoundsAttached.begin(); It != ItEnd; ++It)
	{
		SAttachedSound &slot = (*It);
		if (slot.pSound)
		{
			nRefCount = slot.pSound->AddRef() -1;
			slot.pSound->Release();
			assert (nRefCount > 1 || slot.pSound->IsPlaying());
		}
	}
#endif

}

//////////////////////////////////////////////////////////////////////////
ISound* CSoundProxy::GetSound( tSoundID nSoundId )
{
  if (nSoundId == INVALID_SOUNDID)
    return 0;

	SAttachedSound *pSoundSlot = FindAttachedSound(nSoundId);
	if (pSoundSlot)
		return pSoundSlot->pSound;
	return 0;
}

//////////////////////////////////////////////////////////////////////////
ISound* CSoundProxy::GetSoundByIndex(uint32 nIndex) // will return NULL on invalid index
{
	if (nIndex < m_SoundsAttached.size())
		return m_SoundsAttached[nIndex].pSound;
	
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
CSoundProxy::SAttachedSound* CSoundProxy::FindAttachedSound( tSoundID nSoundId )
{
	AttachedSoundsVec::iterator ItEnd = m_SoundsAttached.end();
	for (AttachedSoundsVec::iterator It = m_SoundsAttached.begin(); It != ItEnd; ++It)
	{
		SAttachedSound &slot = (*It);
		if (slot.nSoundID == nSoundId)
			return &slot;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
bool CSoundProxy::RemoveAttachedSound( tSoundID nSoundId )
{
	AttachedSoundsVec::iterator ItEnd = m_SoundsAttached.end();
	for (AttachedSoundsVec::iterator It = m_SoundsAttached.begin(); It != ItEnd; ++It)
	{
		SAttachedSound &slot = (*It);
		if (slot.nSoundID == nSoundId)
		{
			slot.pSound->RemoveEventListener(this);
			m_SoundsAttached.erase(It);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CSoundProxy::AddAttachedSound( ISound * pSound, const bool bOffset, const Vec3 &vOffset, const Vec3 &vDirection )
{
	assert(pSound != NULL);

	if (FindAttachedSound(pSound->GetId()))
		return false;

	SAttachedSound slot;
	slot.bOffset = bOffset;
	slot.vOffset = vOffset;
	slot.vDirection = vDirection;
	slot.pSound = pSound;
	slot.nSoundID = pSound->GetId();
	slot.bStatic = false;

	m_SoundsAttached.push_back(slot);
	pSound->AddEventListener(this, m_pEntity->GetName());

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CSoundProxy::PlaySound( ISound *pSound, const Vec3 &vOffset,const Vec3 &vDirection,float fSoundScale,bool bLipSync )
{
	bool bOffset = !vOffset.IsZero() || !IsEquivalent(vDirection,FORWARD_DIRECTION);

	if (m_SoundsAttached.size() > TOO_MANY_SOUNDS)
	{
		gEnv->pSoundSystem->Log(eSLT_Warning, "<Sound> Trying to attach too many sounds to the entity %s (20 already attached)",m_pEntity->GetEntityTextDescription() );
		return false;
	}

	PrecacheHeadBone();
	SetSoundPosition(pSound, bOffset?&vOffset:0, &vDirection);

	Matrix34 tm = m_pEntity->GetWorldTM();
	
	if (tm.GetTranslation() == Vec3Constants<float>::fVec3_Zero)
	{
		gEnv->pSoundSystem->Log(eSLT_Warning, "<Sound> Trying to play a sound at (0,0,0) position in the entity %s. Entity may not be initialized correctly! ",m_pEntity->GetEntityTextDescription() );
	}

	if (!m_bHide || (m_pEntity->GetFlags() & ENTITY_FLAG_UPDATE_HIDDEN))
	{
		AddAttachedSound(pSound, bOffset, vOffset, vDirection);
		if (bLipSync &&	(pSound->GetFlags() & FLAG_SOUND_VOICE))
		{
			// If voice is playing inform entity character (if present) about it to apply lip-sync.
			// actually facial sequence is triggered in OnSoundEvent SOUND_EVENT_ON_PLAYBACK_STARTED
			ICharacterInstance *pChar = m_pEntity->GetCharacter(0);
			if (pChar)
			{
				// when PLAYBACK is started, it will start facial sequence as well
				m_currentLipSyncId = pSound->GetId();
			}
		}
		pSound->Play(fSoundScale, true, true, this);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
tSoundID CSoundProxy::PlaySound( const char *sGroupAndSoundName, const Vec3 &vOffset,const Vec3 &vDirection, uint32 nSoundFlags, ESoundSemantic eSemantic)
{
	EntityId SkipEntIDs[1];
	SkipEntIDs[0] = m_pEntity->GetId();
	return PlaySound(sGroupAndSoundName, vOffset, vDirection, nSoundFlags, eSemantic, SkipEntIDs, 1);
}

//////////////////////////////////////////////////////////////////////////
tSoundID CSoundProxy::PlaySound( const char *sGroupAndSoundName, const Vec3 &vOffset,const Vec3 &vDirection, uint32 nSoundFlags, ESoundSemantic eSemantic, EntityId *pSkipEnts,int nSkipEnts)
{
	tSoundID ID = INVALID_SOUNDID;

	ISound *pSound = gEnv->pSoundSystem->CreateSound(sGroupAndSoundName, nSoundFlags);

	if (pSound)
	{
		pSound->SetSemantic(eSemantic);
		pSound->SetPhysicsToBeSkipObstruction(pSkipEnts, nSkipEnts);
		
		if (PlaySound(pSound, vOffset, vDirection, 1.0f))
			ID = pSound->GetId();
	}
	else
	{
		// sound failed to load, but still send the onDone event to Script or FG
		ESoundCallbackEvent myEvent = SOUND_EVENT_ON_STOP;
		OnSoundEvent( myEvent,NULL );
	}

	return ID;
}

//////////////////////////////////////////////////////////////////////////
tSoundID CSoundProxy::PlaySoundEx( const char *sGroupAndSoundName, const Vec3 &vOffset,const Vec3 &vDirection, uint32 nSoundFlags, float fVolume, float fMinRadius, float fMaxRadius, ESoundSemantic eSemantic)
{
	EntityId SkipEntIDs[1];
	SkipEntIDs[0] = m_pEntity->GetId();
	return PlaySoundEx(sGroupAndSoundName, vOffset, vDirection, nSoundFlags, fVolume, fMinRadius, fMaxRadius, eSemantic, SkipEntIDs, 1);
}

//////////////////////////////////////////////////////////////////////////
tSoundID CSoundProxy::PlaySoundEx( const char *sGroupAndSoundName, const Vec3 &vOffset,const Vec3 &vDirection, uint32 nSoundFlags, float fVolume, float fMinRadius, float fMaxRadius, ESoundSemantic eSemantic, EntityId *pSkipEnts, int nSkipEnts)
{
	tSoundID ID = INVALID_SOUNDID;

	_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound(sGroupAndSoundName, nSoundFlags);

	if (pSound)
	{
		pSound->SetSemantic(eSemantic);
		pSound->GetInterfaceExtended()->SetVolume(fVolume);
		
		if (fMinRadius>0 && fMaxRadius>0)
			pSound->GetInterfaceDeprecated()->SetMinMaxDistance(fMinRadius, fMaxRadius);
		
		pSound->SetPhysicsToBeSkipObstruction(pSkipEnts, nSkipEnts);		

		if (PlaySound(pSound, vOffset, vDirection, 1.0f))
			ID = pSound->GetId();
	}
	else
	{
		// sound failed to load, but still send the onDone event to Script or FG
		ESoundCallbackEvent myEvent = SOUND_EVENT_ON_STOP;
		OnSoundEvent( myEvent,NULL );
	}

	return ID;
}

//////////////////////////////////////////////////////////////////////////
bool CSoundProxy::SetStaticSound( tSoundID nSoundId, bool bStatic)
{
	SAttachedSound *pSoundSlot = FindAttachedSound(nSoundId);
	if (pSoundSlot)
	{
		pSoundSlot->bStatic = bStatic;
        return true;
	}
    return false;        
}

//////////////////////////////////////////////////////////////////////////
bool CSoundProxy::GetStaticSound( const tSoundID nSoundId)
{
	bool bResult = false;
	SAttachedSound *pSoundSlot = FindAttachedSound(nSoundId);
	if (pSoundSlot)
	{
		bResult = pSoundSlot->bStatic;
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::StopSound( tSoundID nSoundId, ESoundStopMode eStopMode )
{
  if (nSoundId == INVALID_SOUNDID)
    return;

	SAttachedSound *pSoundSlot = FindAttachedSound(nSoundId);
	if (pSoundSlot && pSoundSlot->pSound)
	{
		pSoundSlot->pSound->Stop(eStopMode);
		if (!pSoundSlot->bStatic)
			RemoveAttachedSound(nSoundId);
	}
	else
	{
		// this should not happen, but lets try to stop the sound even if its not known to the proxy
		_smart_ptr<ISound> pSound = gEnv->pSoundSystem->GetSound(nSoundId);
		
		if (pSound)
			pSound->Stop(eStopMode);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::PauseSound( tSoundID nSoundId, bool bPause )
{
	if (nSoundId == INVALID_SOUNDID)
		return;

	SAttachedSound *pSoundSlot = FindAttachedSound(nSoundId);
	if (pSoundSlot && pSoundSlot->pSound)
		pSoundSlot->pSound->SetPaused( bPause );
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::StopAllSounds()
{
	//////////////////////////////////////////////////////////////////////////
	AttachedSoundsVec::reverse_iterator itEndR = m_SoundsAttached.rend();
	for (AttachedSoundsVec::reverse_iterator itR = m_SoundsAttached.rbegin(); itR != itEndR; ++itR)
	{
		SAttachedSound &slot = (*itR);
		slot.bStatic = false;
		assert (NULL!=slot.pSound);
		slot.pSound->RemoveEventListener(this);
		slot.pSound->Stop(ESoundStopMode_AtOnce);
	}

	m_SoundsAttached.clear();
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::SetSoundPos( tSoundID nSoundId,const Vec3 &vOffset )
{
	SAttachedSound *pSoundSlot = FindAttachedSound(nSoundId);
	if (pSoundSlot)
	{
		pSoundSlot->vOffset = vOffset;
		pSoundSlot->bOffset = !pSoundSlot->vOffset.IsZero() || !IsEquivalent(pSoundSlot->vDirection,FORWARD_DIRECTION);
	}
}

//////////////////////////////////////////////////////////////////////////
Vec3 CSoundProxy::GetSoundPos( tSoundID nSoundId )
{
	SAttachedSound *pSoundSlot = FindAttachedSound(nSoundId);
	if (pSoundSlot)
	{
		return pSoundSlot->vOffset;
	}
	return Vec3(0,0,0);
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::SetSoundDirection( tSoundID nSoundId,const Vec3 &dir )
{
	SAttachedSound *pSoundSlot = FindAttachedSound(nSoundId);
	if (pSoundSlot)
	{
		pSoundSlot->vDirection = dir;
		pSoundSlot->bOffset = !pSoundSlot->vOffset.IsZero() || !IsEquivalent(pSoundSlot->vDirection,FORWARD_DIRECTION);
	}
}

//////////////////////////////////////////////////////////////////////////
Vec3 CSoundProxy::GetSoundDirection( tSoundID nSoundId )
{
	SAttachedSound *pSoundSlot = FindAttachedSound(nSoundId);
	if (pSoundSlot)
	{
		return pSoundSlot->vDirection;
	}
	return FORWARD_DIRECTION;
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::OnSoundEvent( ESoundCallbackEvent event,ISound *pSound )
{
	switch (event) {
	case SOUND_EVENT_ON_PLAYBACK_STARTED:
		{
			if (pSound && pSound->GetId() == m_currentLipSyncId)
			{
				ICharacterInstance *pChar = m_pEntity ? m_pEntity->GetCharacter(0) : 0;
				if (pChar)
				{
					pChar->LipSyncWithSound(m_currentLipSyncId);
				}
			}
		}
		break;

	case SOUND_EVENT_ON_STOP:
		{
			if (pSound)
			{
				if (pSound->GetId() == m_currentLipSyncId)
				{
					ICharacterInstance *pChar = m_pEntity ? m_pEntity->GetCharacter(0) : 0;
					if (pChar)
					{
						pChar->LipSyncWithSound(m_currentLipSyncId, true);
					}
					m_currentLipSyncId = INVALID_SOUNDID;
				}				
				
				// maybe NULL when soundsystem is disabled or sound not found
				SAttachedSound *pSoundSlot = FindAttachedSound(pSound->GetId());

				assert(pSoundSlot);

				if (pSoundSlot)
				{
					if (!pSoundSlot->bStatic)
					{
						pSound->RemoveEventListener(this);
						RemoveAttachedSound(pSound->GetId());
						// pSound may be invalid from now on
					}
				}
				else
					event = event;
			}

			// send event to entity (lua or FG node)
			if (m_pEntity)
			{
				SEntityEvent DoneEvent;
				DoneEvent.event = ENTITY_EVENT_SOUND_DONE;
				DoneEvent.nParam[0] = m_pEntity->GetId();
				m_pEntity->SendEvent( DoneEvent );
			}

			break;
		}

	case SOUND_EVENT_ON_LOAD_FAILED:
		// Remove sound from set.
		pSound->RemoveEventListener(this);
		if (pSound->GetId() != INVALID_SOUNDID)
			RemoveAttachedSound(pSound->GetId());
		else
			event = event;

		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundProxy::CheckVisibilityForTailName(const float fLength, const float fDistanceToRecalculate)
{
	if (m_pEntity)
	{
		if (m_pEntity->GetWorldPos().GetSquaredDistance(m_vLastVisibilityPos) > fDistanceToRecalculate*fDistanceToRecalculate)
		{
			m_vLastVisibilityPos = m_pEntity->GetWorldPos();

			Quat const qDirection = m_pEntity->GetWorldRotation().GetNormalized();
			Vec3 const vDirection = qDirection * Vec3(0,0,fLength);

			IPhysicalEntity* pSkipEnts;
			int nSkipEnts = 0;

			IPhysicalEntity* const pPhysicalEntity = m_pEntity->GetPhysics();
			if (pPhysicalEntity)
			{
				pSkipEnts = pPhysicalEntity;
				++nSkipEnts;
			}

			EntityId const nEntityID = m_pEntity->GetId();
			gEnv->pPhysicalWorld->RayWorldIntersection(m_vLastVisibilityPos, vDirection, ent_static|ent_terrain, rwi_pierceability0|rwi_queue, NULL, 5, &pSkipEnts, nSkipEnts, (void*)nEntityID, PHYS_FOREIGN_ID_SOUND_PROXY_OBSTRUCTION);
		}

		if (CVar::pDrawSoundProxyZRay->GetIVal() != 0)
		{
			IRenderAuxGeom* pAuxGeom( gEnv->pRenderer->GetIRenderAuxGeom() );
			pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );

			ColorB red(255,0,0,255);
			ColorB green(0,255,0,255);
			if (m_sTailname == SOUND_INDOOR_TAILNAME)
				pAuxGeom->DrawLine(m_vLastVisibilityPos, red, m_vLastVisibilityPos+Vec3(0,0,fLength), red, 3.0f);
			else
				pAuxGeom->DrawLine(m_vLastVisibilityPos, green, m_vLastVisibilityPos+Vec3(0,0,fLength), green, 3.0f);
		}
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(IEntitySoundProxy)
