/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 30:8:2005   12:52 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Item.h"
#include "Actor.h"
#include "Player.h"
#include "Alien.h"
#include "GameCVars.h"
#include <IViewSystem.h>
#include "ItemSharedParams.h"


//------------------------------------------------------------------------
void CItem::UpdateFPView(float frameTime)
{ 
	if (!m_stats.selected)
		return;

	CheckViewChange();

	if (!m_stats.fp && !m_stats.mounted)
		return;

	if (GetGameObject()->GetAspectProfile(eEA_Physics)!=eIPhys_NotPhysicalized)
		return;

	if (m_camerastats.animating)
	{
		if (m_camerastats.position)
			m_camerastats.pos=GetSlotHelperPos(eIGS_FirstPerson, m_camerastats.helper.c_str(), false, true);

		if (m_camerastats.rotation)
			m_camerastats.rot=Quat(GetSlotHelperRotation(eIGS_FirstPerson, m_camerastats.helper.c_str(), false, true)); //*Quat::CreateRotationZ(-gf_PI);
	}
	
	IEntity* pOwner = GetOwner();
	ICharacterInstance* pOwnerCharacter = pOwner ? pOwner->GetCharacter(0) : 0;
	
	if (pOwnerCharacter && !pOwnerCharacter->GetISkeletonAnim()->FindAnimInFIFO(0,ITEM_OWNER_ACTION_LAYER, true))
	{
		if (m_idleAnimation[eIGS_Owner].empty())
		{
			m_idleSoundId = PlayAction(m_idleAnimation[eIGS_Owner], 0, true);
			m_inIdleAction = true;
		}
		m_releaseCameraBone = false; //Make sure this one is cleared if not action playing
	}


	if (IItem *pSlave = GetDualWieldSlave())
		pSlave->UpdateFPView(frameTime);

}

//------------------------------------------------------------------------
bool CItem::FilterView(struct SViewParams &viewParams)
{
	if (m_camerastats.animating && m_camerastats.follow)
	{
		const Matrix34 tm = GetEntity()->GetSlotWorldTM(eIGS_FirstPerson);
		Vec3 offset(0.0f,0.0f,0.0f);

		offset += tm.GetColumn(0).GetNormalized()*m_camerastats.pos.x;
		offset += tm.GetColumn(1).GetNormalized()*m_camerastats.pos.y;
		offset += tm.GetColumn(2).GetNormalized()*m_camerastats.pos.z;

		viewParams.position+=offset;
		viewParams.rotation*=m_camerastats.rot;
		viewParams.blend=true;
		viewParams.viewID=5;
	}

	return m_camerastats.reorient;
}

//------------------------------------------------------------------------
void CItem::PostFilterView(struct SViewParams &viewParams)
{
	if (m_camerastats.animating && !m_camerastats.follow)
	{
		const Matrix34 tm = GetEntity()->GetSlotWorldTM(eIGS_FirstPerson);
		Vec3 offset(0.0f,0.0f,0.0f);

		offset += tm.GetColumn(0).GetNormalized()*m_camerastats.pos.x;
		offset += tm.GetColumn(1).GetNormalized()*m_camerastats.pos.y;
		offset += tm.GetColumn(2).GetNormalized()*m_camerastats.pos.z;

		viewParams.position+=offset;
		viewParams.rotation*=m_camerastats.rot;
		viewParams.blend=true;
		viewParams.viewID=5;
	}

	if (m_camerastats.animating && m_stats.mounted && !m_camerastats.helper.empty() && IsOwnerFP())
	{
		viewParams.position = GetSlotHelperPos(eIGS_FirstPerson, m_camerastats.helper, true);
		viewParams.rotation = Quat(GetSlotHelperRotation(eIGS_FirstPerson, m_camerastats.helper, true));    
		viewParams.blend = true;
		viewParams.viewID=5;

		viewParams.nearplane = 0.25f;
	}
}

//------------------------------------------------------------------------
bool CItem::IsOwnerFP()
{
	CActor *pOwner = GetOwnerActor();
	if (!pOwner)
		return false;

	if (m_pGameFramework->GetClientActor() != pOwner)
		return false;

	return !pOwner->IsThirdPerson();
}

//------------------------------------------------------------------------
bool CItem::IsCurrentItem()
{
	CActor *pOwner = GetOwnerActor();
	if (!pOwner)
		return false;

	if (pOwner->GetCurrentItem() == this)
		return true;

	return false;
}

//------------------------------------------------------------------------
void CItem::UpdateMounted(float frameTime)
{
	CActor *pActor = GetOwnerActor();
	if (!pActor)
		return;

	CheckViewChange();

  if (IsClient())
  {
	  ICharacterInstance *pCharacter = GetEntity()->GetCharacter(eIGS_FirstPerson);

	  if (pCharacter && !m_idleAnimation[eIGS_FirstPerson].empty() && pCharacter->GetISkeletonAnim()->GetNumAnimsInFIFO(0)<1)
		  PlayAction(m_idleAnimation[eIGS_FirstPerson], 0, true);
  }
	
	//adjust the orientation of the gun based on the aim-direction
  if (ICharacterInstance* pCharInstance = pActor->GetEntity()->GetCharacter(0))
	{
    if (ISkeletonAnim* pSkeletonAnim = pCharInstance->GetISkeletonAnim())
    {   
      OldBlendSpace ap;				
      if (GetAimBlending(ap))
      {
				pSkeletonAnim->SetBlendSpaceOverride(eMotionParamID_TurnSpeed, 0.5f + 0.5f * ap.m_turn, true);
      }        
		}        
	} 

	SMovementState info;
	IMovementController* pMC = pActor->GetMovementController();		
	pMC->GetMovementState(info);
	
	Matrix34 tm = Matrix33::CreateRotationVDir(info.aimDirection.GetNormalized());
	Vec3 vGunXAxis = tm.GetColumn0();

  UpdateIKMounted(pActor, vGunXAxis*0.1f);
   
	RequireUpdate(eIUS_General);
  
}

//------------------------------------------------------------------------
void CItem::UpdateIKMounted(IActor* pActor, const Vec3& vGunXAxis)
{
	if (SMountParams* pMountParams = m_sharedparams->pMountParams)
	{
		if (!pMountParams->left_hand_helper.empty() && !pMountParams->right_hand_helper.empty())
		{ 
			const Vec3 lhpos = GetSlotHelperPos(eIGS_FirstPerson, pMountParams->left_hand_helper.c_str(), true);
			const Vec3 rhpos = GetSlotHelperPos(eIGS_FirstPerson, pMountParams->right_hand_helper.c_str(), true);
			pActor->SetIKPos("leftArm",		lhpos - vGunXAxis, 1);
			pActor->SetIKPos("rightArm",	rhpos + vGunXAxis, 1);

			//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(lhpos, 0.075f, ColorB(255, 255, 255, 255));
			//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(rhpos, 0.075f, ColorB(128, 128, 128, 255));
		}
	}
}

//------------------------------------------------------------------------
bool CItem::GetAimBlending(OldBlendSpace& params)
{ 
  // unused here so far 
  return false;
}


//------------------------------------------------------------------------
void CItem::CheckViewChange()
{
  CActor *pOwner = GetOwnerActor();
  
  if (m_stats.mounted)
	{
    bool fp = pOwner?!pOwner->IsThirdPerson():false;

    if (fp!=m_stats.fp)
    {
      if (fp)
			OnEnterFirstPerson();
	  else
			OnEnterThirdPerson();
      //else if (!fp)
      //  AttachArms(false, false);
    }
    
    m_stats.fp = fp;
 
		return;
	}

  if (!pOwner)
    return;

	if (!pOwner->IsThirdPerson())
	{
		if (!m_stats.fp || !(m_stats.viewmode&eIVM_FirstPerson))
			OnEnterFirstPerson();
		m_stats.fp = true;
	}
	else
	{
		if (m_stats.fp || !(m_stats.viewmode&eIVM_ThirdPerson))
			OnEnterThirdPerson();
		m_stats.fp = false;
	}
}

//------------------------------------------------------------------------
void CItem::SetViewMode(int mode)
{
	m_stats.viewmode = mode;

	for (TAccessoryMap::iterator it = m_accessories.begin(); it != m_accessories.end(); it++)
	{
		IItem *pItem = m_pGameFramework->GetIItemSystem()->GetItem(it->second);
		if (pItem)
		{
			CItem *pCItem = static_cast<CItem *>(pItem);
			if (pCItem)
				pCItem->SetViewMode(mode);
		}
	}

	//--- Shadow character hack
	CActor *pOwner = GetOwnerActor();
	ICharacterInstance *pOwnerShadowCharacter = NULL;
	bool showShadowChar = g_pGameCVars->g_showShadowChar != 0;
	if (pOwner && pOwner->IsPlayer())
	{
		CPlayer *ownerPlayer = (CPlayer*)pOwner;
		pOwnerShadowCharacter = ownerPlayer->GetShadowCharacter();
	}

	if (mode & eIVM_FirstPerson)
	{
		SetHand(m_stats.hand);

		if (!m_parentId)
		{
			uint32 flags = GetEntity()->GetFlags();
			if (!m_stats.mounted)
				flags &= ~ENTITY_FLAG_CASTSHADOW;
			else
				flags |= ENTITY_FLAG_CASTSHADOW;

			//GetEntity()->SetFlags(flags|ENTITY_FLAG_RECVSHADOW);
			DrawSlot(eIGS_FirstPerson, true, !m_stats.mounted);

			if (pOwnerShadowCharacter)
			{
				GetEntity()->SetFlags(flags & ~ENTITY_FLAG_CASTSHADOW);
			}
		}
		else
			DrawSlot(eIGS_FirstPerson, false, false);

	}
	else
	{
		SetGeometry(eIGS_FirstPerson, 0);
	}


	if (mode & eIVM_ThirdPerson)
	{
		DrawSlot(eIGS_ThirdPerson, true);
		if (!m_stats.mounted)
			CopyRenderFlags(GetOwner());

		if (pOwnerShadowCharacter)
		{
			IAttachmentManager *pAttachmentManager = pOwnerShadowCharacter->GetIAttachmentManager();
			IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(m_sharedparams->params.attachment[m_stats.hand].c_str());
			if (pAttachment)
			{
				pAttachment->ClearBinding();
			}
		}
	}
	else
	{
		DrawSlot(eIGS_ThirdPerson, false);

		if (pOwnerShadowCharacter)
		{
			IAttachmentManager *pAttachmentManager = pOwnerShadowCharacter->GetIAttachmentManager();
			IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(m_sharedparams->params.attachment[m_stats.hand].c_str());
			ICharacterInstance *pTPChar=GetEntity()->GetCharacter(eIGS_ThirdPerson);

			//--- Attach the 3rd person version to the shadow character
			if (pTPChar && pAttachment)
			{
				//--- TSB First person hacking
				CCHRAttachment *pChrAttachment = new CCHRAttachment();
				pChrAttachment->m_pCharInstance = pTPChar;
				pAttachment->AddBinding(pChrAttachment);
				pAttachment->HideAttachment(!showShadowChar);
				pAttachment->HideInShadow(0);
			}
		}
	}
	SetViewModeForShadowCharacter(mode);

	if (!pOwner)
	{
		// Slight hack for kill cam replay, needed to get certain particle effects (e.g. muzzle flashes) to work properly
		// in first person mode. Normally this flag would get set in CheckViewChange() but this is never called during kill cam replay.
		m_stats.fp = mode & eIVM_FirstPerson;
	}
}

//-----------------------------------------------------------------------
void CItem::SetViewModeForShadowCharacter( int mode )
{
	if (m_stats.mounted)
		return;

	//--- Shadow character for FP mode
	CActor *pOwner = GetOwnerActor();
	ICharacterInstance *pOwnerShadowCharacter = NULL;
	
	if (pOwner && pOwner->IsPlayer())
	{
		CPlayer *ownerPlayer = (CPlayer*)pOwner;
		pOwnerShadowCharacter = ownerPlayer->GetShadowCharacter();
	}

	if (pOwnerShadowCharacter == NULL)
		return;

	bool showShadowChar = g_pGameCVars->g_showShadowChar != 0;

	if (mode & eIVM_FirstPerson)
	{
		GetEntity()->SetFlags(GetEntity()->GetFlags() & ~ENTITY_FLAG_CASTSHADOW);


		IAttachment *pAttachment = pOwnerShadowCharacter->GetIAttachmentManager()->GetInterfaceByName(m_sharedparams->params.attachment[m_stats.hand].c_str());

		if (pAttachment == NULL)
			return;

		if (ICharacterInstance *pTPChar = GetEntity()->GetCharacter(eIGS_ThirdPerson))
		{
			CCHRAttachment *pChrAttachment = new CCHRAttachment();
			pChrAttachment->m_pCharInstance = pTPChar;
			pAttachment->AddBinding(pChrAttachment);
			pAttachment->HideAttachment(!showShadowChar);
			pAttachment->HideInShadow(0);
		}
		else if(IStatObj* pTPObject = GetEntity()->GetStatObj(eIGS_ThirdPerson))
		{
			CCGFAttachment* pCGFAttachment = new CCGFAttachment();
			pCGFAttachment->pObj = pTPObject;
			pAttachment->AddBinding(pCGFAttachment);
			pAttachment->HideAttachment(!showShadowChar);
			pAttachment->HideInShadow(0);
		}
	}
	else
	{
		IAttachment *pAttachment = pOwnerShadowCharacter->GetIAttachmentManager()->GetInterfaceByName(m_sharedparams->params.attachment[m_stats.hand].c_str());
		if (pAttachment == NULL)
			return;

		pAttachment->ClearBinding();
	}
}

//------------------------------------------------------------------------
void CItem::CopyRenderFlags(IEntity *pOwner)
{
	if (!pOwner || !GetRenderProxy())
		return;

	IRenderNode *pRenderNode = GetRenderProxy()->GetRenderNode();
	if (pRenderNode)
	{
		IEntityRenderProxy *pOwnerRenderProxy = (IEntityRenderProxy *)pOwner->GetProxy(ENTITY_PROXY_RENDER);
		IRenderNode *pOwnerRenderNode = pOwnerRenderProxy?pOwnerRenderProxy->GetRenderNode():NULL;
		if (pOwnerRenderNode)
		{
			pRenderNode->SetViewDistRatio(pOwnerRenderNode->GetViewDistRatio());
			pRenderNode->SetLodRatio(pOwnerRenderNode->GetLodRatio());

			uint32 flags = pOwner->GetFlags()&(ENTITY_FLAG_CASTSHADOW);
			uint32 mflags = GetEntity()->GetFlags()&(~(ENTITY_FLAG_CASTSHADOW));
			GetEntity()->SetFlags(mflags|flags);
		}
	}
}


