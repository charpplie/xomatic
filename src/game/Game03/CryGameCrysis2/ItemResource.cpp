/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 30:8:2005   12:30 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Item.h"
#include "ItemSharedParams.h"
#include "IViewSystem.h"

#include <ICryAnimation.h>
#include <ISound.h>
#include <CryPath.h>
#include <IVehicleSystem.h>
#include "Actor.h"
#include "Game.h"
#include "GameCVars.h"
#include "Player.h"
#include "GameForceFeedback.h"
#include "StringUtils.h"
#include "RecordingSystem.h"

#if !defined(_RELEASE)
	#define DEBUG_ITEM_ACTIONS_ENABLED 1
#else
	#define DEBUG_ITEM_ACTIONS_ENABLED 0
#endif

//------------------------------------------------------------------------
struct CItem::SwitchHandAction
{
	SwitchHandAction(CItem *_item, int _hand): item(_item), hand(_hand) {};
	CItem *item; 
	int hand;

	void execute(CItem *_this)
	{
		item->AttachToHand(false);
		item->SetHand(hand);
		item->AttachToHand(true);
	}
};

//------------------------------------------------------------------------
void CItem::RemoveEntity(bool force)
{
	if (gEnv->IsEditor() && !force)
		Hide(true);
	else if (IsServer() || force)
		gEnv->pEntitySystem->RemoveEntity(GetEntityId());
}

//------------------------------------------------------------------------
bool CItem::CreateCharacterAttachment(int slot, const char *name, int type, const char *bone)
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (!pCharacter)
		return false;

	IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(name);

	if (pAttachment)
	{
//		GameWarning("Item '%s' trying to create attachment '%s' which already exists!", GetEntity()->GetName(), name);
		return false;
	}

	pAttachment = pAttachmentManager->CreateAttachment(name, type, bone);

	if (!pAttachment)
	{
		if (type == CA_BONE)
			GameWarning("Item '%s' failed to create attachment '%s' on bone '%s'!", GetEntity()->GetName(), name, bone);
		return false;
	}

	return true;
}

//------------------------------------------------------------------------
void CItem::DestroyCharacterAttachment(int slot, const char *name)
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (!pCharacter)
		return;

	IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
	pAttachmentManager->RemoveAttachmentByName(name);
}

ICharacterInstance* CItem::GetAppropriateCharacter(int slot, bool owner)
{
	ICharacterInstance* pCharacter = NULL;
	if(owner)
	{
		IEntity* pOwner = GetOwner();
		if(pOwner)
		{
			pCharacter = pOwner->GetCharacter(0);
		}
	}
	else
	{
		pCharacter = GetEntity()->GetCharacter(slot);
	}

	return pCharacter;
}

//------------------------------------------------------------------------
void CItem::ResetCharacterAttachment(int slot, const char *name, bool owner)
{
	if (ICharacterInstance *pCharacter = GetAppropriateCharacter(slot, owner))
	{
		IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
		IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(name);

		if (!pAttachment)
		{
			GameWarning("Item '%s' trying to reset attachment '%s' which does not exist!", GetEntity()->GetName(), name);
			return;
		}

		pAttachment->ClearBinding();
	}
}

//------------------------------------------------------------------------
const char *CItem::GetCharacterAttachmentBone(int slot, const char *name)
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (!pCharacter)
		return 0;

	IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(name);

	if (!pAttachment)
	{
		GameWarning("Item '%s' trying to get attachment bone on '%s' which does not exist!", GetEntity()->GetName(), name);
		return 0;
	}

	return pCharacter->GetISkeletonPose()->GetJointNameByID(pAttachment->GetBoneID());
}

//------------------------------------------------------------------------
void CItem::SetCharacterAttachment(int slot, const char *name, IEntity *pEntity, bool owner)
{
	if (ICharacterInstance *pCharacter = GetAppropriateCharacter(slot, owner))
	{
		IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
		IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(name);

		if (!pAttachment)
		{
			GameWarning("Item '%s' trying to attach entity on '%s' which does not exist!", GetEntity()->GetName(), name);
			return;
		}

		CEntityAttachment *pEntityAttachment = new CEntityAttachment();
		pEntityAttachment->SetEntityId(pEntity->GetId());

		pAttachment->AddBinding(pEntityAttachment);
		pAttachment->HideAttachment(0);
	}
}

//------------------------------------------------------------------------
void CItem::SetCharacterAttachment(int slot, const char *name, IStatObj *pObj, bool owner)
{
	if (ICharacterInstance *pCharacter = GetAppropriateCharacter(slot, owner))
	{
		IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
		IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(name);

		if (!pAttachment)
		{
			GameWarning("Item '%s' trying to attach static object on '%s' which does not exist!", GetEntity()->GetName(), name);
			return;
		}

		CCGFAttachment *pStatAttachment = new CCGFAttachment();
		pStatAttachment->pObj  = pObj;

		pAttachment->AddBinding(pStatAttachment);
	}
}

//------------------------------------------------------------------------
void CItem::SetCharacterAttachment(int slot, const char *name, ICharacterInstance *pAttachedCharacter, bool owner)
{
	if (ICharacterInstance *pCharacter = GetAppropriateCharacter(slot, owner))
	{
		IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
		IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(name);

		if (!pAttachment)
		{
			GameWarning("Item '%s' trying to attach character on '%s' which does not exist!", GetEntity()->GetName(), name);
			return;
		}

		CCHRAttachment *pCharacterAttachment = new CCHRAttachment();
		pCharacterAttachment->m_pCharInstance  = pAttachedCharacter;

		pAttachment->AddBinding(pCharacterAttachment);
	}
}

//------------------------------------------------------------------------
void CItem::SetCharacterAttachment(int slot, const char *name, CDLight &light, int flags)
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (!pCharacter)
		return;

	IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(name);

	if (!pAttachment)
	{
		GameWarning("Item '%s' trying to attach light on '%s' which does not exist!", GetEntity()->GetName(), name);
		return;
	}

	CLightAttachment *pLightAttachment = new CLightAttachment();
	pLightAttachment->LoadLight(light);

	pAttachment->AddBinding(pLightAttachment);
	pAttachment->HideAttachment(0);
}

//------------------------------------------------------------------------
void CItem::SetCharacterAttachment(int slot, const char *name, IEntity *pEntity, int objSlot, bool owner)
{
	SEntitySlotInfo info;
	if (!pEntity->GetSlotInfo(objSlot, info))
		return;

	if (info.pCharacter)
		SetCharacterAttachment(slot, name, info.pCharacter, owner);
	else if (info.pStatObj)
		SetCharacterAttachment(slot, name, info.pStatObj, owner);
}

//------------------------------------------------------------------------
void CItem::SetCharacterAttachmentLocalTM(int slot, const char *name, const Matrix34 &tm)
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (!pCharacter)
		return;

	IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(name);

	if (!pAttachment)
	{
		GameWarning("Item '%s' trying to set local TM on attachment '%s' which does not exist!", GetEntity()->GetName(), name);
		return;
	}

	pAttachment->SetAttRelativeDefault( QuatT(tm));
}

//------------------------------------------------------------------------
void CItem::SetCharacterAttachmentWorldTM(int slot, const char *name, const Matrix34 &tm)
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (!pCharacter)
		return;

	IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(name);

	if (!pAttachment)
	{
		GameWarning("Item '%s' trying to set world TM on attachment '%s' which does not exist!", GetEntity()->GetName(), name);
		return;
	}

//	Matrix34 boneWorldMatrix = GetEntity()->GetSlotWorldTM(slot) *	pCharacter->GetISkeleton()->GetAbsJMatrixByID(pAttachment->GetBoneID());
	Matrix34 boneWorldMatrix = GetEntity()->GetSlotWorldTM(slot) *	Matrix34(pCharacter->GetISkeletonPose()->GetAbsJointByID(pAttachment->GetBoneID()) );

	Matrix34 localAttachmentMatrix = (boneWorldMatrix.GetInverted()*tm);
	pAttachment->SetAttRelativeDefault(QuatT(localAttachmentMatrix));
}

//------------------------------------------------------------------------
Matrix34 CItem::GetCharacterAttachmentLocalTM(int slot, const char *name)
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (!pCharacter)
		return Matrix34::CreateIdentity();;

	IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(name);

	if (!pAttachment)
	{
		GameWarning("Item '%s' trying to get local TM on attachment '%s' which does not exist!", GetEntity()->GetName(), name);
		return Matrix34::CreateIdentity();
	}

	return Matrix34(pAttachment->GetAttRelativeDefault());
}

//------------------------------------------------------------------------
Matrix34 CItem::GetCharacterAttachmentWorldTM(int slot, const char *name)
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (!pCharacter)
		return Matrix34::CreateIdentity();

	IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(name);

	if (!pAttachment)
	{
		GameWarning("Item '%s' trying to get local TM on attachment '%s' which does not exist!", GetEntity()->GetName(), name);
		return Matrix34::CreateIdentity();
	}

	return Matrix34(pAttachment->GetAttWorldAbsolute());
}

//------------------------------------------------------------------------
void CItem::HideCharacterAttachment(int slot, const char *name, bool hide)
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (!pCharacter)
		return;

	IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(name);

	if (!pAttachment)
	{
		GameWarning("Item '%s' trying to hide attachment '%s' which does not exist!", GetEntity()->GetName(), name);
		return;
	}

	pAttachment->HideAttachment(hide?1:0);
}

//------------------------------------------------------------------------
void CItem::HideCharacterAttachmentMaster(int slot, const char *name, bool hide)
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (!pCharacter)
		return;

	pCharacter->HideMaster(hide?1:0);
}

//------------------------------------------------------------------------
void CItem::CreateAttachmentHelpers(int slot)
{
	for (THelperVector::const_iterator it = m_sharedparams->helpers.begin(); it != m_sharedparams->helpers.end(); ++it)
	{
		if (it->slot != slot)
			continue;

		CreateCharacterAttachment(slot, it->name.c_str(), CA_BONE, it->bone.c_str());
	}

	if (slot == eIGS_FirstPerson)
		CreateCharacterAttachment(slot, ITEM_ARMS_ATTACHMENT_NAME, CA_SKIN, 0);
}

//------------------------------------------------------------------------
void CItem::DestroyAttachmentHelpers(int slot)
{
	for (THelperVector::const_iterator it = m_sharedparams->helpers.begin(); it != m_sharedparams->helpers.end(); ++it)
	{
		if (it->slot != slot)
			continue;

		DestroyCharacterAttachment(slot, it->name.c_str());
	}
}

//------------------------------------------------------------------------
const THelperVector& CItem::GetAttachmentHelpers()
{
	return m_sharedparams->helpers;
}

//------------------------------------------------------------------------
bool CItem::SetGeometry(int slot, const ItemString& name, const Vec3& poffset, const Ang3& aoffset, float scale, bool forceReload)
{
	bool changedfp=false;
	switch(slot)
	{
	case eIGS_Owner:
		break;
	case eIGS_FirstPerson:
	case eIGS_ThirdPerson:
	default:
		{
			if (!name || forceReload)
			{
				GetEntity()->FreeSlot(slot);
#ifndef ITEM_USE_SHAREDSTRING
				m_geometry[slot].resize(0);
#else
				m_geometry[slot].reset();
#endif
			}
	
			DestroyAttachmentHelpers(slot);

			if (name && name[0])
			{
				if (m_geometry[slot] != name)
				{
					const char* ext = PathUtil::GetExt(name.c_str());
					if ((stricmp(ext, "chr") == 0) || (stricmp(ext, "cdf") == 0) || (stricmp(ext, "cga") == 0) )
						GetEntity()->LoadCharacter(slot, name, 0);
					else
						GetEntity()->LoadGeometry(slot, name, 0, 0);

					changedfp=slot==eIGS_FirstPerson;
				}
				
				CreateAttachmentHelpers(slot);

				SetDefaultIdleAnimation(slot, GetParams().idle);
			}

/*			if (slot == eIGS_FirstPerson)
			{
				ICharacterInstance *pCharacter = GetEntity()->GetCharacter(eIGS_FirstPerson);
				if (pCharacter)
				{
					pCharacter->SetFlags(pCharacter->GetFlags()&(~CS_FLAG_UPDATE));
				}
			}
      else */if (slot == eIGS_Destroyed)
        DrawSlot(eIGS_Destroyed, false);
		}
		break;
	}

	Matrix34 slotTM;
	slotTM = Matrix34::CreateRotationXYZ(aoffset);
	slotTM.Scale(Vec3(scale, scale, scale));
	slotTM.SetTranslation(poffset);
	GetEntity()->SetSlotLocalTM(slot, slotTM);

	if (changedfp && m_stats.mounted)
	{
		PlayAction(m_idleAnimation[eIGS_FirstPerson], 0, true);
		ForceSkinning(true);

		if (m_sharedparams->pMountParams && !m_sharedparams->pMountParams->pivot.empty())
		{
			Matrix34 tm=GetEntity()->GetSlotLocalTM(eIGS_FirstPerson, false);
			Vec3 pivot = GetSlotHelperPos(eIGS_FirstPerson, m_sharedparams->pMountParams->pivot.c_str(), false);
			tm.AddTranslation(pivot);

			GetEntity()->SetSlotLocalTM(eIGS_FirstPerson, tm);
		}

		GetEntity()->InvalidateTM();
	}

	m_geometry[slot] = name ? name : ItemString();

	ReAttachAccessories();

	return true;
}

//------------------------------------------------------------------------
void CItem::SetDefaultIdleAnimation(int slot, const ItemString& actionName)
{
	m_idleAnimation[slot] = actionName;
}

//------------------------------------------------------------------------
void CItem::ForceSkinning(bool always)
{
	for (int slot=0; slot<eIGS_Last; slot++)
	{
		ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
		if (pCharacter)
		{
			Matrix34 m34=GetEntity()->GetSlotWorldTM(slot);
			QuatT renderLocation = QuatT(m34);

			pCharacter->GetISkeletonPose()->SetForceSkeletonUpdate(7);
			pCharacter->SkeletonPreProcess(renderLocation, renderLocation, GetISystem()->GetViewCamera(),0x55 );
			pCharacter->SetPostProcessParameter(renderLocation, renderLocation, 0, 0.0f, 0x55 );
			if (!always)
				pCharacter->GetISkeletonPose()->SetForceSkeletonUpdate(0);
		}
	}
}

//------------------------------------------------------------------------
void CItem::ForceFPSkeletonUpdate()
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(eIGS_FirstPerson);
	if (pCharacter)
	{
		Matrix34 m34=GetEntity()->GetSlotWorldTM(eIGS_FirstPerson);
		QuatT renderLocation = QuatT(m34);

		pCharacter->GetISkeletonPose()->SetForceSkeletonUpdate(7);
		pCharacter->SkeletonPreProcess(renderLocation, renderLocation, GetISystem()->GetViewCamera(),0x55 );
		pCharacter->GetISkeletonPose()->SetForceSkeletonUpdate(0);
	}
}

//------------------------------------------------------------------------
void CItem::EnableHiddenSkinning(bool enable)
{
	/*
	for (int slot=0; slot<eIGS_Last; slot++)
	{
		ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
		if (pCharacter)
		{
			if (enable)
				pCharacter->SetFlags(pCharacter->GetFlags()|CS_FLAG_UPDATE_ALWAYS);
			else
				pCharacter->SetFlags(pCharacter->GetFlags()&(~CS_FLAG_UPDATE_ALWAYS));
		}
	}
	*/
}

const SCachedItemAnimation CItem::GenerateAnimCacheID(const char *parameterisedAnimName) const
{
	CActor* pOwnerActor						= GetOwnerActor();
	const uint32 checkHash				= CryStringUtils::CalculateHash(parameterisedAnimName);
	const uint32 suffixHash				= CryStringUtils::CalculateHash(m_actionSuffix.c_str());
	ENanoSuitMode checkMode	= (pOwnerActor && pOwnerActor->HasNanoSuit()) ? pOwnerActor->GetActorSuitGameParameters().GetMode() : eNanoSuitMode_Tactical;

	if (checkMode == eNanoSuitMode_Stealth && !m_isUserStealthy)
		checkMode = eNanoSuitMode_Tactical;

	return SCachedItemAnimation(checkHash, (int8)m_stats.hand, (int8)checkMode, suffixHash);
}

//-----------------------------------------------------------------------
int CItem::FindCachedAnimationId(const ItemString& inName, ICharacterInstance* pCharacter)
{
	if (pCharacter)
	{
#ifdef DEBUG_ITEM_ACTIONS_ENABLED
		static uint32 totalEntries = 0;
		static uint32 totalMemory = 0;
#endif

		SCachedItemAnimation checkCachedId = GenerateAnimCacheID(inName.c_str());

		TAnimationIdsCache::const_iterator begin = m_sharedparams->m_cachedAnimationIDs.begin();
		TAnimationIdsCache::const_iterator end = m_sharedparams->m_cachedAnimationIDs.end();
		TAnimationIdsCache::const_iterator found = std::find(begin, end, checkCachedId);

		if(found != end)
		{
			return found->GetAnimationId();
		}

		TempResourceName fixedName;

		// the whole thing of fixing is not nice, but at least we don't allocate too often
		fixedName.assign(inName.c_str(), inName.length());

		const char* hand = (m_stats.hand == eIH_Left) ? "left" : "right";
		if (fixedName.find("[hand]") != TempResourceName::npos)
		{
			fixedName.replace("[hand]", hand);
			checkCachedId.SetVariableFlag(SCachedItemAnimation::eCVF_Hand);
		}

		if (fixedName.find("[suffix]") != TempResourceName::npos)
		{
			fixedName.replace("[suffix]", m_actionSuffix.c_str());
			checkCachedId.SetVariableFlag(SCachedItemAnimation::eCVF_Suffix);
		}

		CActor* pOwnerActor = GetOwnerActor();
		const char* suitMode = (pOwnerActor && pOwnerActor->HasNanoSuit()) ? pOwnerActor->GetActorSuitGameParameters().ModeToStringParam() : "tac"; 

		if (fixedName.find("[suitmode]") != TempResourceName::npos)
		{
			fixedName.replace("[suitmode]", suitMode);
			checkCachedId.SetVariableFlag(SCachedItemAnimation::eCVF_SuitMode);
		}

		checkCachedId.SetAnimationId(pCharacter->GetIAnimationSet()->GetAnimIDByName(fixedName.c_str()));

		m_sharedparams->m_cachedAnimationIDs.push_back(checkCachedId);

#ifdef DEBUG_ITEM_ACTIONS_ENABLED
		totalEntries++;
		totalMemory+= sizeof(checkCachedId);

		CryComment("CItem::FindCachedAnimationId: Num Entries: %d, Memory: %d", totalEntries, totalMemory);

		if (g_pGameCVars->g_animatorDebug && (checkCachedId.GetAnimationId() < 0))
		{
			static const ColorF col (1.0f, 0.0f, 0.0f, 1.0f);
			CryFixedStringT<256> errorMsg;
			errorMsg.Format("Missing %s", fixedName.c_str());
			g_pGame->GetIGameFramework()->GetIPersistantDebug()->Add2DText(errorMsg.c_str(), 1.0f, col, 10.0f);
			GameWarning("CItem::FindCachedAnimationId : %s", errorMsg.c_str());
		}
#endif

		return checkCachedId.GetAnimationId();

	}

	return -1;
}

//----------------------------------------------------------------------------------------
void CItem::FixSoundResourceName(const ItemString& inName, TempResourceName& fixedName)
{
	fixedName.assign(inName.c_str(), inName.length());

	IEntity* pOwnerEntity = GetOwner();
	if (!pOwnerEntity)
		return;

	IEntitySoundProxy *pSoundProxy = (IEntitySoundProxy *)pOwnerEntity->GetProxy(ENTITY_PROXY_SOUND);

	if (!pSoundProxy)
		pSoundProxy = (IEntitySoundProxy *)pOwnerEntity->CreateProxy(ENTITY_PROXY_SOUND);

	if (pSoundProxy)
	{
		// check for a roof 10m above the Owner
		// recalculate visibility when owner move more than 2 meters
		pSoundProxy->CheckVisibilityForTailName(10.0f, 2.0f);
		fixedName.replace("[env]", pSoundProxy->GetTailName());
	}
}

#if !defined(PROFILE) && !defined(_RELEASE)
static ItemString s_previousAction;
#endif

//------------------------------------------------------------------------
tSoundID CItem::PlayAction(const ItemString& actionName, int layer, bool loop, uint32 flags, float speedOverride, float animWeigth, float ffeedbackWeight)
{
	CRY_TODO(26, 3, 2010, "Benito: Re-visit this function, and clean/re-factor!");

#if DEBUG_ITEM_ACTIONS_ENABLED && !defined(PROFILE)
	if(g_pGameCVars->i_debug_weaponActions == 2 || (g_pGameCVars->i_debug_weaponActions == 1 && strcmpi(s_previousAction.c_str(), actionName.c_str())))
	{
		s_previousAction = actionName;
		static const ColorF col (0.0f, 1.0f, 1.0f, 1.0f);
		g_pGame->GetIGameFramework()->GetIPersistantDebug()->Add2DText(string().Format("%s - %s", GetEntity()->GetName(), actionName.c_str()).c_str(), 1.0f, col, 10.0f);
	}
#endif

	if (m_inIdleAction)
	{
		StopSound(m_idleSoundId);
		m_idleSoundId = INVALID_SOUNDID;
	}
	m_inIdleAction = false;

	CActor *pOwnerActor = GetOwnerActor();
	EntityId ownerId = m_owner.GetId();

	bool concentratedFire = false;
	if(flags&eIPAF_Shoot && pOwnerActor && pOwnerActor->IsPlayer())
	{
		CPlayer* pPlayer = static_cast<CPlayer*>(pOwnerActor);
		if(pPlayer->IsPerkActive(ePerk_ConcentratedFire))
		{
			concentratedFire = true;
		}
	}

	TActionMap::const_iterator it = m_sharedparams->actions.find(CONST_TEMPITEM_STRING(actionName));
	if (it == m_sharedparams->actions.end())
	{
		memset(m_animationTime, 0, sizeof(m_animationTime));
		m_fpOverlayOverride = 1.0f;
		m_releaseCameraBone = false;
		return 0;
	}

	bool fp = m_stats.fp;
	
	CRY_TODO(26, 3, 2010, "Benito: This probably is not needed, find out, and remove if it's the case");
	if (m_parentId)
	{
		CItem *pParent = static_cast<CItem *>(m_pItemSystem->GetItem(m_parentId));
		if (pParent)
			fp = pParent->GetStats().fp;
	}
	
	int sid = fp ? eIGS_FirstPerson : eIGS_ThirdPerson;
	const SItemAction &action = it->second;

	tSoundID result = INVALID_SOUNDID;
	if ((flags&eIPAF_Sound) && !action.sound[sid].name.empty() && IsSoundEnabled() && g_pGameCVars->i_soundeffects)
	{
		int nSoundFlags = FLAG_SOUND_DEFAULT_3D;
		nSoundFlags |= flags&eIPAF_SoundStartPaused?FLAG_SOUND_START_PAUSED:0;
		IEntitySoundProxy *pSoundProxy = GetSoundProxy(true);

		//GetSound proxy from dualwield master if neccesary
		if(IsDualWieldSlave())
		{
			CItem* pMaster = static_cast<CItem*>(GetDualWieldMaster());
			if(pMaster)
			{
				pSoundProxy = pMaster->GetSoundProxy(true);
			}
		}

		EntityId pSkipEnts[3];
		int nSkipEnts = 0;

		// TODO for Marcio :)
		// check code changes

		// Skip the Item
		pSkipEnts[nSkipEnts] = GetEntity()->GetId();
		++nSkipEnts;

		// Skip the Owner
		pSkipEnts[nSkipEnts] = ownerId;
		++nSkipEnts;

		if (pSoundProxy)
		{
			
			TempResourceName name;
			FixSoundResourceName(action.sound[sid].name, name);

			if (!g_pGameCVars->i_staticfiresounds)
			{
				result = pSoundProxy->PlaySoundEx(name, ZERO, FORWARD_DIRECTION, nSoundFlags, 1.0f, 0, 0, eSoundSemantic_Weapon, pSkipEnts, nSkipEnts);
				ISound *pSound = pSoundProxy->GetSound(result);

				if (pSound && action.sound[sid].sphere>0.0f)
					pSound->SetSphereSpec(action.sound[sid].sphere);
			}
			else
			{
				SInstanceAudio *pInstanceAudio = 0;

				if (action.sound[sid].isstatic)
				{
					TInstanceActionMap::iterator iit = m_instanceActions.find(CONST_TEMPITEM_STRING(actionName));
					if (iit == m_instanceActions.end())
					{
						std::pair<TInstanceActionMap::iterator, bool> insertion=m_instanceActions.insert(TInstanceActionMap::value_type(actionName, SInstanceAction()));
						pInstanceAudio = &insertion.first->second.sound[sid];
					}
					else
					{
						pInstanceAudio = &iit->second.sound[sid];
					}
				}

				if (pInstanceAudio && (pInstanceAudio->id != INVALID_SOUNDID) && (name != pInstanceAudio->static_name))
				{
					ReleaseStaticSound(pInstanceAudio);
				}

				if (!pInstanceAudio || pInstanceAudio->id == INVALID_SOUNDID)
				{
          result = pSoundProxy->PlaySoundEx(name, ZERO, FORWARD_DIRECTION, nSoundFlags, 1.0f, 0, 0, eSoundSemantic_Weapon, pSkipEnts, nSkipEnts);
					ISound *pSound = pSoundProxy->GetSound(result);
					if(pSound)
					{
						if(concentratedFire)
						{
							pSound->SetParam("concentration", 1.0f);
						}

						if (action.sound[sid].sphere > 0.0f)
						{
							pSound->SetSphereSpec(action.sound[sid].sphere);
						}
					}
					
				}

				if (action.sound[sid].isstatic)
				{
					if (pInstanceAudio->id == INVALID_SOUNDID)
					{
						if(pSoundProxy->SetStaticSound(result, true))
						{
							pInstanceAudio->id = result;
							pInstanceAudio->static_name = name;
							pInstanceAudio->synch = action.sound[sid].issynched;
						}
					}
					else
					{
						ISound *pSound = pSoundProxy->GetSound(pInstanceAudio->id);
						if (pSound)
						{
							if(pSound->IsPlaying())
							{
								pSound->Stop();
							}
							pSound->Play(1.0, true, true, pSoundProxy);
							result = pSound->GetId();
						}
					}
				}
			}
			
			if (gEnv->pAISystem && action.sound[sid].airadius > 0.0f)
			{
				// associate sound event with vehicle if the shooter is in a vehicle (tank cannon shot, etc)
				IVehicle* pOwnerVehicle = pOwnerActor ? pOwnerActor->GetLinkedVehicle() : NULL;
				if (pOwnerVehicle && pOwnerVehicle->GetEntityId())
				{
						ownerId = pOwnerVehicle->GetEntityId();
				}
				SAIStimulus stim(AISTIM_SOUND, AISOUND_WEAPON, ownerId, 0,
					GetEntity()->GetWorldPos(), ZERO, action.sound[sid].airadius);
				gEnv->pAISystem->RegisterStimulus(stim);
			}
		}
	}


	if (flags&eIPAF_Animation)
	{
		TempResourceName name;
		// generate random number only once per call to allow animations to
		// match across geometry slots (like first person and third person)
		float randomNumber = Random();
		for (int i=0; i<eIGS_LastAnimSlot; i++)
		{
			if (!(flags&(1<<i)))
				continue;

			int nanimations=action.animation[i].size();
			if (nanimations <= 0)
			{
				if((flags & eIPAF_EndCurrentWeaponAnim) && (i == eIGS_FirstPerson || i == eIGS_ThirdPerson))
				{
					ICharacterInstance* pCharacter = GetEntity()->GetCharacter(i);
					if(pCharacter)
					{
						pCharacter->GetISkeletonAnim()->SetLayerTime(0, 1.f);
					}
				}
				continue;
			}
		
			int anim = int( randomNumber * float(nanimations) );

			const SAnimation& animation = action.animation[i][anim];

			if (animation.name.empty())
				continue;

			if (i == eIGS_Owner)
			{
				if (!pOwnerActor)
					continue;

				m_fpOverlayOverride					= action.fpOverlayOverride;
				m_fpOverlayOverrideEndTime	= loop ? std::numeric_limits<float>::max() : gEnv->pTimer->GetCurrTime() + ((float)GetCurrentAnimationTime(eIGS_Owner)/1000.0f) - action.fpOverlayBlendTime;

				//Benito - Backwards compatibility with upperbody graph for AI actors
				//       - If there is some animation graph data or a procedural recoil for the action, then skip playing animations directly
				if (!pOwnerActor->IsPlayer())
				{
					int animGraphEntries = action.animation[eIGS_OwnerAnimGraph].size();
					int animGraphLoopEntries = action.animation[eIGS_OwnerAnimGraphLooped].size();
					bool useProceduralRecoil = (0 != (flags & CItem::eIPAF_ProceduralRecoil));

					if ((animGraphEntries > 0) || (animGraphLoopEntries > 0) || useProceduralRecoil)
						continue;
				}
			}

			if ((i == eIGS_OwnerAnimGraph) || (i == eIGS_OwnerAnimGraphLooped))
			{
				if (!pOwnerActor || pOwnerActor->IsPlayer())
					continue;

				if (!animation.name.empty())
				{
					bool looping = (eIGS_OwnerAnimGraphLooped==i);

					if (IsDualWield() && !m_sharedparams->params.dual_wield_pose.empty())
						pOwnerActor->PlayAction(name, m_sharedparams->params.dual_wield_pose.c_str(), looping);
					else
						pOwnerActor->PlayAction(name, m_sharedparams->params.pose.c_str(), looping);
				}
				continue;
			}

			if (!animation.name.empty())
			{
				float localSpeed = (float)__fsel(-speedOverride, animation.speed, speedOverride);
				float blend = (flags&eIPAF_NoBlend) ? 0.0f : animation.blend;

				if(concentratedFire)
				{
					localSpeed *= (float)__fres(CPerk::GetInstance()->GetVars()->perk_ConcentratedFire_rateScale);
				}

				PlayAnimationEx(animation.name, i, layer, loop, blend, localSpeed, flags, action.timeOverride, animWeigth);

				if(i == eIGS_Owner)
				{
					m_releaseCameraBone = animation.releaseCameraBone;

					const SItemAction::SHandSwitch* handSwitchParams = NULL;
					ENanoSuitMode suitMode = eNanoSuitMode_Tactical;
					
					if(pOwnerActor && pOwnerActor->HasNanoSuit())
					{
						suitMode = pOwnerActor->GetActorSuitGameParameters().GetMode();
					}

					int num = action.handSwitch.size();
					
					for(int i = 0; i < num; i++)
					{
						if(action.handSwitch[i].associatedSuitMode == suitMode)
						{
							handSwitchParams = &action.handSwitch[i];
							break;
						}
					}

					if(handSwitchParams && handSwitchParams->switchToHand != eIH_Last && handSwitchParams->switchToHand != m_stats.hand)
					{
						uint32 animLength = GetCurrentAnimationTime(eIGS_Owner);

						GetScheduler()->TimerAction((uint32)(animLength * handSwitchParams->switchToHandAnimFraction), CSchedulerAction<SwitchHandAction>::Create(SwitchHandAction(this, handSwitchParams->switchToHand)), false);

						if(handSwitchParams->resetHandAnimFraction > handSwitchParams->switchToHandAnimFraction)
						{
							GetScheduler()->TimerAction((uint32)(animLength * handSwitchParams->resetHandAnimFraction), CSchedulerAction<SwitchHandAction>::Create(SwitchHandAction(this, m_stats.hand)), false);
						}
					}
				}
			}

			if ((m_stats.fp || m_stats.viewmode&eIVM_FirstPerson) && i==eIGS_FirstPerson && !animation.camera_helper.empty())
			{
				m_camerastats.animating=true;
				m_camerastats.helper=animation.camera_helper;
				m_camerastats.position=animation.camera_pos;
				m_camerastats.rotation=animation.camera_rot;
				m_camerastats.follow=animation.camera_follow;
				m_camerastats.reorient=animation.camera_reorient;
			}
			else if (m_camerastats.animating)
				m_camerastats=SCameraAnimationStats();
		}
	}

	if (action.children)
	{
		for (TAccessoryMap::iterator ait=m_accessories.begin(); ait!=m_accessories.end(); ++ait)
		{
			EntityId aId=(EntityId)ait->second;
			CItem *pAccessory=static_cast<CItem *>(m_pItemSystem->GetItem(aId));
			if (pAccessory)
				pAccessory->PlayAction(actionName, layer, loop, flags, speedOverride);
		}
	}

	// play force feedback for client
	bool isClient = pOwnerActor ? pOwnerActor->IsClient() : false;
	if(isClient) 
	{
		const SForceFeedback& forceFeedback = it->second.forceFeedback;
		if ((forceFeedback.amplifierA + forceFeedback.amplifierB) > 0)
		{
			ENanoSuitMode suitMode = pOwnerActor->GetActorSuitGameParameters().GetMode();
			const float mult = (suitMode == eNanoSuitMode_Power) ? forceFeedback.combatModeMultiplier : 1.0f;

			const float time = forceFeedback.time * mult;
			const float ampA = forceFeedback.amplifierA * mult * ffeedbackWeight;
			const float ampB = forceFeedback.amplifierB * mult * ffeedbackWeight;
			GameForceFeedback_Always(	SFFOutputEvent(	eDI_XI, eFF_Rumble_Basic,	time, ampA, ampB));
		}
	}

	return result;
}

//------------------------------------------------------------------------
void CItem::PlayAnimation(const char* animationName, int layer, bool loop, uint32 flags)
{
	for (int i=0; i<eIGS_Last; i++)
	{
		if (!(flags&1<<i))
			continue;

		PlayAnimationEx(animationName, i, layer, loop, 0.175f, 1.0f, flags);
	}
}

//------------------------------------------------------------------------
void CItem::PlayAnimationEx(const ItemString& animationName, int slot, int layer, bool loop, float blend, float speed, uint32 flags, float overrideTime, float weigth)
{
	// Owner / player animation, goes through animation proxy 
	if ((slot == eIGS_Owner) && animationName)
	{
		if (CActor * pOwnerActor = GetOwnerActor())
		{
			ICharacterInstance* pOwnerCharacter = pOwnerActor->GetEntity()->GetCharacter(0);
			IAnimatedCharacter *pAnimCharacter = pOwnerActor->GetAnimatedCharacter();
			CAnimationPlayerProxy *pAnimPlayerProxy = pAnimCharacter ? pAnimCharacter->GetAnimationPlayerProxy(eAnimationGraphLayer_UpperBody) : NULL;

			if (pOwnerCharacter && pAnimPlayerProxy)
			{
				layer += ITEM_OWNER_ACTION_LAYER;

				int animationId = FindCachedAnimationId(animationName, pOwnerCharacter);
				float duration  = (animationId>=0) ? pOwnerCharacter->GetIAnimationSet()->GetDuration_sec(animationId) : 0.0f;

#if DEBUG_ITEM_ACTIONS_ENABLED
				if (g_pGameCVars->g_animatorDebug && (animationId < 0))
				{
					static const ColorF col (1.0f, 0.0f, 0.0f, 1.0f);
					g_pGame->GetIGameFramework()->GetIPersistantDebug()->Add2DText((string("Missing ")+animationName.c_str()).c_str(), 1.0f, col, 10.0f);
				}
#endif

				if ((duration > 0.0f) && (overrideTime > 0.0f))
				{
					speed = (duration / overrideTime);
					CRY_ASSERT((speed > 0.0f) && (speed < 99999.0f));
				}

				if (flags&eIPAF_CleanBlending)
				{
					pAnimPlayerProxy->StopAnimationInLayer(GetOwnerActor()->GetEntity(), layer, 0.0f);
				}

				blend = (flags&eIPAF_NoBlend) ? 0.0f : blend;

				CryCharAnimationParams params;
				params.m_fTransTime = blend;
				params.m_nLayerID = layer;
				params.m_nFlags = 0;
				params.m_nFlags |= loop ? CA_LOOP_ANIMATION : 0;
				params.m_nFlags |= (flags & eIPAF_RestartAnimation) ? CA_ALLOW_ANIM_RESTART : 0;
				params.m_nFlags |= (flags & eIPAF_RepeatLastFrame) ? CA_REPEAT_LAST_KEY : 0;
				pAnimPlayerProxy->StartAnimationById(pOwnerActor->GetEntity(), animationId, params, speed);
				pOwnerCharacter->GetISkeletonAnim()->SetAdditiveWeight(layer, weigth);

				m_animationTime[slot] = (uint32)(duration*1000.0f/speed);
			}			
		}
		return;
	}

	//Other anims (weapon usually)
	bool start = true;

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);

	if (!pCharacter)
	{
		if ((slot == eIGS_FirstPerson) && ((m_stats.viewmode&eIVM_FirstPerson) == 0))
		{
			start = false;

			int idx = 0;
			if (m_stats.hand == eIH_Right)
				idx = 1;
			else if (m_stats.hand == eIH_Left)
				idx = 2;
			if (m_fpgeometry[idx].name.empty())
				idx = 0;

			pCharacter = m_pItemSystem->GetCachedCharacter(m_fpgeometry[idx].name.c_str());
		}
	}

	if (pCharacter && animationName)
	{
		ISkeletonAnim* pSkeletonAnim = pCharacter->GetISkeletonAnim();
		int animationId = FindCachedAnimationId(animationName, pCharacter);
		float duration  = (animationId>=0) ? pCharacter->GetIAnimationSet()->GetDuration_sec(animationId) : 0.0f;

		if (flags&eIPAF_CleanBlending)
		{
			while(pSkeletonAnim->GetNumAnimsInFIFO(layer)>1)
			{
				if (!pSkeletonAnim->RemoveAnimFromFIFO(layer, pSkeletonAnim->GetNumAnimsInFIFO(layer)-1))
					break;
			}
		}

		if (flags&eIPAF_NoBlend)
			blend = 0.0f;

#ifdef DEBUG_ITEM_ACTIONS_ENABLED
		if (g_pGameCVars->g_animatorDebug && (animationId < 0))
		{
			static const ColorF col (1.0f, 0.0f, 0.0f, 1.0f);
			g_pGame->GetIGameFramework()->GetIPersistantDebug()->Add2DText((string("Missing ")+animationName.c_str()).c_str(), 1.0f, col, 10.0f);
		}
#endif

		if ((duration > 0.0f) && (overrideTime > 0.0f))
		{
			speed = (duration / overrideTime);
			CRY_ASSERT((speed > 0.0f) && (speed < 99999.0f));
		}

		if (start)
		{
			CryCharAnimationParams params;
			params.m_fTransTime = blend;
			params.m_nLayerID = layer;
			params.m_nFlags = (loop?CA_LOOP_ANIMATION:0)|(flags&eIPAF_RestartAnimation?CA_ALLOW_ANIM_RESTART:0)|(flags&eIPAF_RepeatLastFrame?CA_REPEAT_LAST_KEY:0);
			params.m_nUserToken = animationId;
			bool success = pSkeletonAnim->StartAnimationById(animationId, params);
			pSkeletonAnim->SetLayerUpdateMultiplier(layer, speed);
			pSkeletonAnim->SetAdditiveWeight(layer, weigth);

			//pCharacter->GetISkeleton()->SetDebugging( true );
			CRecordingSystem* pRecordingSystem = g_pGame->GetRecordingSystem();
			if (pRecordingSystem && success)
			{
				SRecording_Animation itemAnimation;
				itemAnimation.type = eTPP_WeaponAnim;
				itemAnimation.eid = GetOwnerId();
				itemAnimation.speedMultiplier = speed;
				itemAnimation.animID = animationId;
				itemAnimation.animparams = params;
				pRecordingSystem->AddPacket(itemAnimation);
			}
		}
		
		m_animationTime[slot] = (uint32)(duration*1000.0f/speed);
	}
}

void CItem::EndItemAnimation(int slot)
{
	ICharacterInstance* pCharacter = GetEntity()->GetCharacter(slot);
	
	if(pCharacter)
	{
		pCharacter->GetISkeletonAnim()->SetLayerTime(0, 1.f);
	}
}

void CItem::EndActionAnimations(float blendOut)
{
	ResetOwnerAnimationsInLayer(ITEM_OWNER_ACTION_LAYER, blendOut);
	EndItemAnimation(eIGS_FirstPerson);
	EndItemAnimation(eIGS_ThirdPerson);
}

void CItem::StopActionAnimations(float blendOut)
{
	ResetOwnerAnimationsInLayer(ITEM_OWNER_ACTION_LAYER, blendOut);
	ResetAllSlotAnimations(eIGS_FirstPerson);
	ResetAllSlotAnimations(eIGS_ThirdPerson);
}

//------------------------------------------------------------------------
void CItem::PlayLayer(const ItemString& layerName, int flags, bool record)
{
	TLayerMap::const_iterator it = m_sharedparams->layers.find(CONST_TEMPITEM_STRING(layerName));
	if (it == m_sharedparams->layers.end())
		return;

	for (int i=0; i<eIGS_LastLayerSlot; i++)
	{
		if (!(flags&1<<i))
			continue;

		const SLayer &layer = it->second;

		if (!layer.name[i].empty())
		{
			ICharacterInstance *pCharacter = GetEntity()->GetCharacter(i);
			if (pCharacter)
			{
				CryCharAnimationParams params;
				params.m_fTransTime = (flags&eIPAF_NoBlend) ? 0.0f : 0.125f;;
				params.m_nLayerID = layer.id[i];
				params.m_nFlags = CA_LOOP_ANIMATION;
				
				int animationId = FindCachedAnimationId(layer.name[i], pCharacter);

				ISkeletonAnim* pSkeletonAnim = pCharacter->GetISkeletonAnim();
				pSkeletonAnim->StartAnimationById(animationId, params);

				if (layer.bones.empty())
				{
					pSkeletonAnim->SetLayerMask(layer.id[i], 1);
				}
				else
				{
					pSkeletonAnim->SetLayerMask(layer.id[i], 0);
					for (std::vector<ItemString>::const_iterator bit = layer.bones.begin(); bit != layer.bones.end(); ++bit)
					{
						pSkeletonAnim->SetJointMask(bit->c_str(), layer.id[i], 1);
					}
				}
			}
		}
	}

	if (record)
	{
		TActiveLayerMap::iterator ait = m_activelayers.find(CONST_TEMPITEM_STRING(layerName));
		if (ait == m_activelayers.end())
		{
			m_activelayers.insert(TActiveLayerMap::value_type(layerName, flags));
		}
	}
}

//------------------------------------------------------------------------
void CItem::StopLayer(const ItemString& layerName, int flags, bool record)
{
	TLayerMap::const_iterator it = m_sharedparams->layers.find(CONST_TEMPITEM_STRING(layerName));
	if (it == m_sharedparams->layers.end())
		return;

	for (int i=0; i<eIGS_LastLayerSlot; i++)
	{
		if (!(flags&1<<i))
			continue;
		
		if (it->second.name[i].empty())
			continue;

		ICharacterInstance *pCharacter = GetEntity()->GetCharacter(i);
		if (pCharacter)
			pCharacter->GetISkeletonAnim()->StopAnimationInLayer(it->second.id[i],0.0f);
	}

	if (record)
	{
		stl::member_find_and_erase(m_activelayers, CONST_TEMPITEM_STRING(layerName));
	}
}

//------------------------------------------------------------------------
void CItem::RestoreLayers()
{
	for (TActiveLayerMap::iterator it = m_activelayers.begin(); it != m_activelayers.end(); ++it)
		PlayLayer(it->first, it->second, false);

	for (TLayerMap::const_iterator lit = m_sharedparams->layers.begin(); lit != m_sharedparams->layers.end(); ++lit)
	{
		if (lit->second.isstatic)
			PlayLayer(lit->first, eIPAF_Default, false);
	}
}

//------------------------------------------------------------------------
void CItem::ResetAllAnimations()
{
	for (int i=0; i<eIGS_Last; i++)
	{
		if ((i == eIGS_OwnerAnimGraph) || (i == eIGS_OwnerAnimGraphLooped))
			continue;

		ICharacterInstance *pCharacter = GetEntity()->GetCharacter(i);
		if (pCharacter)
			pCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();
		m_animationTime[i] = 0;
	}
}

//------------------------------------------------------------------------
void CItem::ResetAllSlotAnimations(eGeometrySlot slot)
{
	if ((slot == eIGS_OwnerAnimGraph) || (slot == eIGS_OwnerAnimGraphLooped))
		return;

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter((int)slot);
	if (pCharacter)
		pCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();

	m_animationTime[slot] = 0;
}

//-----------------------------------------------------------------------
void CItem::ResetOwnerAnimationsInLayer(int layer, float blendOut)
{
	if (CActor* pOwner = GetOwnerActor())
	{
		CAnimationPlayerProxy* pAnimationProxy = pOwner->GetAnimatedCharacter()->GetAnimationPlayerProxy(eAnimationGraphLayer_UpperBody);
		assert(pAnimationProxy);

		pAnimationProxy->StopAnimationInLayer(pOwner->GetEntity(), layer , blendOut);
	}
}

//------------------------------------------------------------------------
uint32 CItem::GetCurrentAnimationTime(int slot)
{
	return m_animationTime[slot];
}

//------------------------------------------------------------------------
void CItem::DrawSlot(int slot, bool bDraw, bool bNear)
{
	uint32 flags = GetEntity()->GetSlotFlags(slot);
	if (bDraw)
		flags |= ENTITY_SLOT_RENDER;
	else
		flags &= ~ENTITY_SLOT_RENDER;

	if (bNear)
		flags |= ENTITY_SLOT_RENDER_NEAREST;
	else
		flags &= ~ENTITY_SLOT_RENDER_NEAREST;
	
	GetEntity()->SetSlotFlags(slot, flags);
}

//------------------------------------------------------------------------
Vec3 CItem::GetSlotHelperPos(int slot, const char *helper, bool worldSpace, bool relative) const
{
	Vec3 position(0,0,0);

	SEntitySlotInfo info;
	if (GetEntity()->GetSlotInfo(slot, info))
	{
		if (info.pStatObj)
		{
			IStatObj *pStatsObj = info.pStatObj;
			position = pStatsObj->GetHelperPos(helper);
			position = GetEntity()->GetSlotLocalTM(slot, false).TransformPoint(position);
		}
		else if (info.pCharacter)
		{
			ICharacterInstance *pCharacter = info.pCharacter;
			IAttachment* pAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName(helper);
			if (pAttachment)
			{
				position = pAttachment->GetAttModelRelative().t;
			}
			else
			{
				ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
				int16 id = pSkeletonPose->GetJointIDByName(helper);
				if (id > -1)
				{
					position = relative ? pSkeletonPose->GetRelJointByID(id).t : pSkeletonPose->GetAbsJointByID(id).t;
				}
			}

			if (!relative)
			{
				position = GetEntity()->GetSlotLocalTM(slot, false).TransformPoint(position);
			}
		}
	}

	if (worldSpace)
	{
		return GetEntity()->GetWorldTM().TransformPoint(position);
	}

	return position;
}

//------------------------------------------------------------------------
const Matrix33 &CItem::GetSlotHelperRotation(int slot, const char *helper, bool worldSpace, bool relative)
{
	static Matrix33 rotation;
	rotation.SetIdentity();

	IEntity* pEntity = GetEntity();
	if(!pEntity)
		return rotation;

	SEntitySlotInfo info;
	if (pEntity->GetSlotInfo(slot, info))
	{
    if (info.pStatObj)
    {
      IStatObj *pStatObj = info.pStatObj;
      rotation = Matrix33(pStatObj->GetHelperTM(helper));
      rotation.OrthonormalizeFast();
      rotation = Matrix33(GetEntity()->GetSlotLocalTM(slot, false))*rotation;        
    }
		else if (info.pCharacter)
		{
			ICharacterInstance *pCharacter = info.pCharacter;
			if(!pCharacter)
				return rotation;

			IAttachment* pAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName(helper);
			if(pAttachment)
			{
				rotation = Matrix33(pAttachment->GetAttModelRelative().q);
			}
			else
			{
				ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
				int16 id = pSkeletonPose->GetJointIDByName(helper);
				if (id > -1)
				{
					rotation = relative ? Matrix33(pSkeletonPose->GetRelJointByID(id).q) : Matrix33(pSkeletonPose->GetAbsJointByID(id).q);
				}
			}

			if (!relative)
			{
				rotation = Matrix33(pEntity->GetSlotLocalTM(slot, false)) * rotation;
			}
		}    
	}

	if (worldSpace)
	{
		rotation = Matrix33(pEntity->GetWorldTM()) * rotation;
	}

	return rotation;
}

//------------------------------------------------------------------------
void CItem::StopSound(tSoundID id)
{
  if (id == INVALID_SOUNDID)
    return;

	bool synchSound = false;
	IEntitySoundProxy *pSoundProxy = GetSoundProxy(false);
	if (pSoundProxy)
	{
		for (TInstanceActionMap::iterator it = m_instanceActions.begin(); it != m_instanceActions.end(); ++it)
		{
			SInstanceAction &action = it->second;
			for (int i=0;i<2;i++)
			{
				if (action.sound[i].id == id)
				{
					pSoundProxy->SetStaticSound(id, false);
					action.sound[i].id = INVALID_SOUNDID;
					synchSound = action.sound[i].synch;
					break;
				}
			}
		}
		if(synchSound)
			pSoundProxy->StopSound(id, ESoundStopMode_OnSyncPoint);
		else
			pSoundProxy->StopSound(id);
	}
}

//------------------------------------------------------------------------
void CItem::Quiet()
{
	IEntitySoundProxy *pSoundProxy = GetSoundProxy(false);
	if (pSoundProxy)
	{
		for (TInstanceActionMap::iterator it = m_instanceActions.begin(); it != m_instanceActions.end(); ++it)
		{
			SInstanceAction &action = it->second;
			for (int i=0;i<2;i++)
			{
				if (action.sound[i].id != INVALID_SOUNDID)
				{
					pSoundProxy->SetStaticSound(action.sound[i].id, false);
					action.sound[i].id = INVALID_SOUNDID;
				}
			}
		}

		pSoundProxy->StopAllSounds();
	}
}

//------------------------------------------------------------------------
ISound *CItem::GetISound(tSoundID id)
{
	IEntitySoundProxy *pSoundProxy = GetSoundProxy(false);
	if (pSoundProxy)
		return pSoundProxy->GetSound(id);

	return 0;
}

//------------------------------------------------------------------------
void CItem::ReleaseStaticSound(SInstanceAudio *sound)
{
	if (sound->id != INVALID_SOUNDID)
	{
		IEntitySoundProxy *pSoundProxy = GetSoundProxy(false);
		if (pSoundProxy)
		{
			pSoundProxy->SetStaticSound(sound->id, false);
			if(sound->synch)
				pSoundProxy->StopSound(sound->id,ESoundStopMode_OnSyncPoint);
			else
				pSoundProxy->StopSound(sound->id);
			sound->id = INVALID_SOUNDID;
#ifndef ITEM_USE_SHAREDSTRING
			sound->static_name.resize(0);
#else
			sound->static_name.reset();
#endif
		}
	}
}

//------------------------------------------------------------------------
void CItem::ReleaseStaticSounds()
{
	for (TInstanceActionMap::iterator it = m_instanceActions.begin(); it != m_instanceActions.end(); ++it)
	{
		ReleaseStaticSound(&it->second.sound[0]);
		ReleaseStaticSound(&it->second.sound[1]);
	}
}

//------------------------------------------------------------------------
IEntitySoundProxy *CItem::GetSoundProxy(bool create)
{
	IEntitySoundProxy *pSoundProxy = (IEntitySoundProxy *)GetEntity()->GetProxy(ENTITY_PROXY_SOUND);
	if (!pSoundProxy && create)
		pSoundProxy = (IEntitySoundProxy *)GetEntity()->CreateProxy(ENTITY_PROXY_SOUND);

	return pSoundProxy;
}

//------------------------------------------------------------------------
IEntityRenderProxy *CItem::GetRenderProxy(bool create)
{
	IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy *)GetEntity()->GetProxy(ENTITY_PROXY_RENDER);
	if (!pRenderProxy && create)
		pRenderProxy = (IEntityRenderProxy *)GetEntity()->CreateProxy(ENTITY_PROXY_RENDER);

	return pRenderProxy;
}

//------------------------------------------------------------------------
IEntityPhysicalProxy *CItem::GetPhysicalProxy(bool create)
{
	IEntityPhysicalProxy *pPhysicalProxy = (IEntityPhysicalProxy *)GetEntity()->GetProxy(ENTITY_PROXY_PHYSICS);
	if (!pPhysicalProxy && create)
		pPhysicalProxy = (IEntityPhysicalProxy *)GetEntity()->CreateProxy(ENTITY_PROXY_PHYSICS);

	return pPhysicalProxy;
}

//------------------------------------------------------------------------
void CItem::DestroyedGeometry(bool use)
{
  if (!m_geometry[eIGS_Destroyed].empty())
  {    
    DrawSlot(eIGS_Destroyed, use);
		if (m_stats.viewmode&eIVM_FirstPerson)
			DrawSlot(eIGS_FirstPerson, !use);
		else
			DrawSlot(eIGS_ThirdPerson, !use);

    if (use)
      GetEntity()->SetSlotLocalTM(eIGS_Destroyed, GetEntity()->GetSlotLocalTM(eIGS_ThirdPerson, false));
  }  
}
