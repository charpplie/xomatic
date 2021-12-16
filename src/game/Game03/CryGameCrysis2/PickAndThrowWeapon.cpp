/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
*************************************************************************/

#include "StdAfx.h"
#include "PickAndThrowWeapon.h"

#include "Game.h"
#include "GameActions.h"
#include "Actor.h"
#include "Player.h"
#include "WeaponSharedParams.h"
#include "IAIActor.h"
#include "GameRules.h"
#include "AutoAimManager.h"
#include <ITargetTrackManager.h>
#include <IVehicleSystem.h>

TActionHandler<CPickAndThrowWeapon> CPickAndThrowWeapon::s_actionHandler;
int CPickAndThrowWeapon::m_objectSlot = 0; 


//////////////////////////////////////////////////////////////////////////
struct CPickAndThrowWeapon::FinishMelee
{
	FinishMelee(CPickAndThrowWeapon& pickAndThrowWeapon)
		: m_pickAndThrowWeapon(pickAndThrowWeapon)
	{

	}

	void execute(CItem *_this)
	{
		m_pickAndThrowWeapon.OnMeleeFinished();
	}

private:
	CPickAndThrowWeapon& m_pickAndThrowWeapon;

};
//////////////////////////////////////////////////////////////////////////
struct CPickAndThrowWeapon::DoMelee
{
	DoMelee(CPickAndThrowWeapon& pickAndThrowWeapon)
		: m_pickAndThrowWeapon(pickAndThrowWeapon)
	{

	}

	void execute(CItem *_this)
	{
		m_pickAndThrowWeapon.OnDoMelee();
	}

private:
	CPickAndThrowWeapon& m_pickAndThrowWeapon;

};
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
struct CPickAndThrowWeapon::DelayedImpulse
{
	DelayedImpulse(CPickAndThrowWeapon& pickAndThrow, EntityId collidedEntityId, const Vec3& impulsePos, const Vec3& impulse, int partId, int ipart)
		: m_pickAndThrow(pickAndThrow)
		, m_collidedEntityId(collidedEntityId)
		, m_impulsePos(impulsePos)
		, m_impulse(impulse)
		, m_partId(partId)
		, m_ipart(ipart)
	{

	};

	void execute(CItem *pItem)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_collidedEntityId);
		IPhysicalEntity* pPhysicalEntity = pEntity ? pEntity->GetPhysics() : NULL;

		if (pPhysicalEntity)
		{
			m_pickAndThrow.m_collisionHelper.Impulse(pPhysicalEntity, m_impulsePos, m_impulse, m_partId, m_ipart);
		}
	}

private:
	CPickAndThrowWeapon& m_pickAndThrow;
	EntityId m_collidedEntityId;
	Vec3 m_impulsePos;
	Vec3 m_impulse;
	int m_partId;
	int m_ipart;
};
//////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
CPickAndThrowWeapon::CPickAndThrowWeapon()
: m_constraintId(0)
, m_timePassed(0)
, m_state( eST_UNKNOWN )
, m_framesToCheckConstraint( 0 )
, m_objectCollisionsLimited( false )
, m_objectGrabbed( false )
, m_objectId( 0 )
, m_objectScale( 1, 1, 1 )
, m_grabType( ePTGrabType_MaxGrabTypes ) // invalid value
, m_currentMeleeAutoTargetId(0)
, m_objectMass(0.0f)
, m_animDuration(0.f)
{
	if(s_actionHandler.GetNumHandlers()==0)
	{
		s_actionHandler.AddHandler(g_pGame->Actions().use, &CPickAndThrowWeapon::OnActionUse);
		s_actionHandler.AddHandler(g_pGame->Actions().attack1_xi, &CPickAndThrowWeapon::OnActionAttack);
		s_actionHandler.AddHandler(g_pGame->Actions().attack2_xi, &CPickAndThrowWeapon::OnActionAttack);
		s_actionHandler.AddHandler(g_pGame->Actions().special, &CPickAndThrowWeapon::OnActionMelee);
	}
	m_attachmentOldRelativeLoc.SetIdentity();
	m_collisionHelper.SetUser(this);	
}


//---------------------------------------------------------------------------
CPickAndThrowWeapon::~CPickAndThrowWeapon()
{
}

//---------------------------------------------------------------------------
void CPickAndThrowWeapon::Reset()
{
	inherited::Reset();

	if (m_state != eST_UNKNOWN)
	{
		if (m_objectGrabbed)
			DropObject();

		if (m_objectCollisionsLimited)
			SetUpCollisionPropertiesAndMassInfo( false );

		DrawNearObject( false );

		m_state = eST_UNKNOWN;
		m_currentMeleeAutoTargetId = 0;
		m_objectMass = 0.0f;
	}
}

//---------------------------------------------------------------------------
bool CPickAndThrowWeapon::Init(IGameObject * pGameObject)
{
	if(!inherited::Init(pGameObject))
	{
		return false;
	}

	if(!m_weaponsharedparams->pPickAndThrowParams)
	{
		GameWarning("Uninitialised pick and throw params. Is the xml for item %s missing the 'PickAndThrowParams' node?", GetEntity()->GetName());

		return false;
	}

	return true;
}

//---------------------------------------------------------------------------
void CPickAndThrowWeapon::OnSelected(bool selected)
{
	CWeapon::OnSelected( selected );
	if (selected)
	{
		m_objectId = GetOwnerPlayer() ? GetOwnerPlayer()->GetPickAndThrowEntity() : 0;
		m_timePassed = 0;
		DecideGrabType();
		PlayAction( GetGrabTypeParams().grab_action );
		RequireUpdate( eIUS_General );
		m_state = eST_GOINGTOPICK;
		m_objectGrabbed = m_objectCollisionsLimited = false;  
	}
	else 
	{
		if (m_objectGrabbed)
			DropObject();
		
		if (m_objectCollisionsLimited)
			SetUpCollisionPropertiesAndMassInfo( false );
		
		DrawNearObject( false ); 
		m_state = eST_UNKNOWN;
	}
}


//---------------------------------------------------------------------------
void CPickAndThrowWeapon::CalcObjectHelperInverseMat( Matrix34& objectHelperInvMat )
{
	IEntity* pEntity = GetEntityObject();
	
	objectHelperInvMat.SetIdentity();
	if (!pEntity)
		return;

	if (IsNPC())
	{
		SEntitySlotInfo info;
		if (pEntity->GetSlotInfo( m_objectSlot, info ))
		{
			if (info.pCharacter)
			{
				ICharacterInstance *pCharacter = info.pCharacter;
				IAttachment* pAttachment = NULL;
				pAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName( GetGrabTypeParams().helper.c_str() );
				if (pAttachment)
				{
					objectHelperInvMat = Matrix34( pAttachment->GetAttModelRelative() );
					objectHelperInvMat.OrthonormalizeFast();
				}
				objectHelperInvMat.InvertFast();
			}
		}
	}
	else
	{
		SEntitySlotInfo info;
		if (pEntity && pEntity->GetSlotInfo( m_objectSlot, info ))
		{
			if (info.pStatObj)
			{
				IStatObj::SSubObject* pSObjHelper = FindHelperObject( GetGrabTypeParams().helper.c_str(), m_objectId );
			
				if (pSObjHelper)
				{
					// this is a way of adjusting scaled objects so they dont block players view if scaled up, or they dont disappear if scaled down. 
					// in any case is not perfect, and designers are encouraged to not use scaled objects for pickandthrow, or at least check them before if they do so.
					AABB box;
					pEntity->GetLocalBounds( box );
					float height = box.GetSize().z;
					float displacementNeeded = ( height * m_objectScale.z ) - height;
					objectHelperInvMat = pSObjHelper->tm;
					objectHelperInvMat.m23 += displacementNeeded;
				}
				
				objectHelperInvMat.InvertFast();
			}
		}
	}
	objectHelperInvMat.Scale( m_objectScale );
}


//---------------------------------------------------------------------------
IStatObj::SSubObject* CPickAndThrowWeapon::FindHelperObject( const char* pHelperName, EntityId objectId ) const
{
	IStatObj::SSubObject* pSObjHelper = NULL;
	IEntity* pEntity = m_pEntitySystem->GetEntity( objectId );
	SEntitySlotInfo info;
	if (pEntity && pEntity->GetSlotInfo( m_objectSlot, info ))
	{
		if (info.pStatObj)
		{
			pSObjHelper = info.pStatObj->FindSubObject( pHelperName ); // first try an easy look in the current object
			if (!pSObjHelper) // if not success, look for the parent and start looking down from there
			{
				IStatObj* pObj = info.pStatObj;
				while (pObj->GetParentObject())   
					pObj = pObj->GetParentObject();
				pSObjHelper = FindHelperObject_RecursivePart( pObj, pHelperName );
			}

			//If after recursing we don't find the object, try with composed name based on geometry name (for destroyed pieces)
			if (!pSObjHelper)
			{
				CryFixedStringT<128> helperNameBuffer;
				helperNameBuffer.Format("%s_%s", info.pStatObj->GetGeoName(), pHelperName);

				pSObjHelper = info.pStatObj->FindSubObject( helperNameBuffer.c_str() ); 
				if (!pSObjHelper) 
				{
					IStatObj* pObj = info.pStatObj;
					while (pObj->GetParentObject())   
						pObj = pObj->GetParentObject();
					pSObjHelper = FindHelperObject_RecursivePart( pObj, pHelperName );
				}
			}
		}
	}
	return pSObjHelper;
}
//---------------------------------------------------------------------------
// just part of FindHelperObject()
IStatObj::SSubObject* CPickAndThrowWeapon::FindHelperObject_RecursivePart( IStatObj* pObj, const char* pHelperName ) const
{
	IStatObj::SSubObject* pSObjHelper = pObj->FindSubObject( pHelperName );
	if (!pSObjHelper)
	{
		uint32 numSubObjects = pObj->GetSubObjectCount();
		for (uint32 i=0; i<numSubObjects && !pSObjHelper; i++)
		{
			if (IStatObj* pNextSubObject = pObj->GetSubObject( i )->pStatObj)
			{
				pSObjHelper = FindHelperObject_RecursivePart( pNextSubObject, pHelperName );
			}
		}
	}

	return pSObjHelper;
}


//---------------------------------------------------------------------------
bool CPickAndThrowWeapon::UpdateAimAnims( SParams_WeaponFPAiming &aimAnimParams, bool &releaseCameraBone )
{
	const SAimAnimsBlock& aimAnims = GetGrabTypeParams().aimAnims;

	const SCachedItemAnimation check = GenerateAnimCacheID(aimAnims.anim[0].c_str());
	uint32 changeFlags = aimAnimParams.UpdateStatus(check);
	aimAnimParams.transitionTime = -1.0f;

	if (changeFlags != 0)
	{
		for (int i=0; i<WeaponAimAnim::Total; i++)
		{
			int animationId = FindCachedAnimationId(aimAnims.anim[i], aimAnimParams.characterInst);
			aimAnimParams.SetAnimation( i, animationId );
		}
	}

	releaseCameraBone = m_releaseCameraBone;
	 
	return true;
}


//---------------------------------------------------------------------------
void CPickAndThrowWeapon::OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if(!s_actionHandler.Dispatch(this,actorId,actionId,activationMode,value))
		CWeapon::OnAction(actorId, actionId, activationMode, value);
}


//---------------------------------------------------------------------------
bool CPickAndThrowWeapon::OnActionAttack(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	bool performThrow = (activationMode == eAAM_OnPress) && (m_state == eST_IDLE);

	if (performThrow)
	{
		m_timePassed = 0;
		PlayAction( GetGrabTypeParams().throw_action );
		m_state = eST_PUSHINGAWAY;  // first step of the throwing sequence
		if (CActor* pOwner = GetOwnerActor())
		{
			SNanoSuitEvent event;
			event.event = eNanoSuitEvent_THROW;
			pOwner->SendActorSuitEvent(event);
		}
	}
	
	return true;
}


//---------------------------------------------------------------------------
bool CPickAndThrowWeapon::OnActionUse(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	bool performDrop = (activationMode == eAAM_OnPress) && (m_state == eST_IDLE);
	if (performDrop)
	{
		m_timePassed = 0;
		PlayAction( GetGrabTypeParams().drop_action );
		m_state = eST_STARTDROPPING;
	}

	return true;
}

//---------------------------------------------------------------------------
bool CPickAndThrowWeapon::OnActionMelee(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	bool performMelee = (activationMode == eAAM_OnPress) && (m_state == eST_IDLE) && !m_collisionHelper.IsBlocked();
	if (performMelee)
	{
		PlayAction( GetGrabTypeParams().melee_action, 0, false, eIPAF_Default | eIPAF_RestartAnimation );
		m_state = eST_MELEE;
		GetScheduler()->TimerAction((uint32)(GetGrabTypeParams().melee_delay * 1000.0f), CSchedulerAction<DoMelee>::Create(DoMelee(*this)), false);
		GetScheduler()->TimerAction(GetCurrentAnimationTime(eIGS_Owner), CSchedulerAction<FinishMelee>::Create(FinishMelee(*this)), false);
	}

	return true;
}

//---------------------------------------------------------------------------
bool CPickAndThrowWeapon::CanModify() const
{
	return false;
}



//---------------------------------------------------------------------------
void CPickAndThrowWeapon::SetUpCollisionPropertiesAndMassInfo( bool pickedUp )
{
	IEntity *pEntity = GetEntityObject();
	if (!pEntity)
		return;
	IPhysicalEntity *pPE = pEntity->GetPhysics();
	if (!pPE)
		return;

	if (pickedUp)
	{
		CActor *pActor = GetOwnerActor();
		if (pActor)
		{
			pe_action_add_constraint ic;
			ic.flags = constraint_inactive|constraint_ignore_buddy;
			ic.pBuddy = pActor->GetEntity()->GetPhysics();
			ic.pt[0].Set(0,0,0);
			m_constraintId = pPE->Action(&ic);
			m_framesToCheckConstraint = 3;  // need to wait a few frames before start checking for the constraint. apparently, some issues with sys_physics_cpu 1 
			m_objectCollisionsLimited = true;
		}

		pe_status_dynamics dynamics;
		if (pPE->GetStatus(&dynamics))
		{
			m_objectMass = dynamics.mass;
		}
	}
	else
	{
		pe_action_update_constraint up;
		up.bRemove = true;
		up.idConstraint = m_constraintId;
		m_constraintId = 0;
		pPE->Action(&up);
		m_objectCollisionsLimited = false;

		m_objectMass = 0.0f;
	} 
}


//---------------------------------------------------------------------------
void CPickAndThrowWeapon::DrawNearObject( bool drawNear )
{
	IEntity *pEntity = GetEntityObject();
	if (!pEntity)
		return;

	int nslots = pEntity->GetSlotCount();
	for (int i=0;i<nslots;i++)
	{
		if (pEntity->GetSlotFlags(i)&ENTITY_SLOT_RENDER)
		{
			if (drawNear)
			{
				pEntity->SetSlotFlags(i, pEntity->GetSlotFlags(i)|ENTITY_SLOT_RENDER_NEAREST);
				if(IEntityRenderProxy* pProxy = (IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER))
				{
					if(IRenderNode* pRenderNode = pProxy->GetRenderNode())
						pRenderNode->SetRndFlags(ERF_REGISTER_BY_POSITION,true);
				}
				
			}
			else
			{
				pEntity->SetSlotFlags(i, pEntity->GetSlotFlags(i)&(~ENTITY_SLOT_RENDER_NEAREST));
				if(IEntityRenderProxy* pProxy = (IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER))
				{
					if(IRenderNode* pRenderNode = pProxy->GetRenderNode())
						pRenderNode->SetRndFlags(ERF_REGISTER_BY_POSITION,false);
				}
			}
		}
	}
}


//---------------------------------------------------------------------------
void CPickAndThrowWeapon::Update(SEntityUpdateContext& ctx, int val )
{
	CWeapon::Update( ctx, val );
	m_timePassed += ctx.fFrameTime;
	
	switch (m_state)
	{
		case eST_GOINGTOPICK:
		{
			if (!GetEntityObject())
				GetOwnerPlayer()->ExitPickAndThrow();
			RequireUpdate( eIUS_General );
			if (m_timePassed >= GetGrabTypeParams().timeToStickAtPicking)
			{
				if (IsNPC())
					PrepareNPCForBeingGrabbed();
				AttachObject();
				m_state = eST_PICKING;
			}
			break;
		}
	
		case eST_PICKING:
		{
			RequireUpdate( eIUS_General );
			if (m_timePassed >= GetGrabTypeParams().timePicking)
			{
				m_state = eST_IDLE;
			}
			break;
		}
		
		case eST_PUSHINGAWAY:
		{
			RequireUpdate( eIUS_General );
			if (m_timePassed >= GetGrabTypeParams().timeToFreeAtThrowing)
			{
				if (IsNPC())
					PrepareNPCToBeThrown();
				ThrowObject();
				DrawNearObject( false );
				SetUpCollisionPropertiesAndMassInfo( false );
				m_state = eST_THROWING; 
			}
			break;
		}
		
		case eST_THROWING:
		{
			RequireUpdate( eIUS_General );
			if (m_timePassed >= GetGrabTypeParams().timeThrowing)
			{
				GetOwnerPlayer()->ExitPickAndThrow();
				m_state = eST_UNKNOWN;
			}
			break;
		}
		
		case eST_STARTDROPPING:
		{
			RequireUpdate( eIUS_General );
			if (IsNPC())
				PrepareNPCToBeDropped();
			DropObject();
			if (m_timePassed >= GetGrabTypeParams().timeToFreeAtDropping)  
			{
				m_state = eST_DROPPING;
				SetUpCollisionPropertiesAndMassInfo( false );
				if (IsNPC())
				{
					if (CActor* pActorNPC = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor( m_objectId )))
						pActorNPC->Fall();
				}
			}
			break;
		}

		case eST_DROPPING:
		{
			RequireUpdate( eIUS_General );
			if (m_timePassed >= GetGrabTypeParams().timeDropping)
			{
				GetOwnerPlayer()->ExitPickAndThrow();
				m_state = eST_UNKNOWN;
			}
			break;
		}
		
		case eST_MELEE:
		{
			if(m_currentMeleeAutoTargetId)
			{
				g_pGame->GetAutoAimManager().SetExternalSnapTarget(m_currentMeleeAutoTargetId);
				RequireUpdate( eIUS_General );
			}
			break;
		}
		
		case eST_KILLING_GRABBEDNPC:
		{
			RequireUpdate( eIUS_General );
			if (m_timePassed >= GetGrabTypeParams().timeToKillGrabbedNPC)
			{
				if (CActor *pActorNPC = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor( m_objectId )))
					SendMortalHitTo( pActorNPC );
				m_state = eST_KILLED_GRABBEDNPC;
			}
			break;
		}
		
		case eST_KILLED_GRABBEDNPC:
		{
			RequireUpdate( eIUS_General );
			if (m_timePassed >= m_animDuration)
			{
				m_timePassed = 0;
				PlayAction( GetGrabTypeParams().drop_action );
				m_state = eST_STARTDROPPING;
			}
			break;
		}
	}
}


//---------------------------------------------------------------------------
void CPickAndThrowWeapon::UpdateFPView( float frameTime )
{
	CWeapon::UpdateFPView( frameTime );

	switch (m_state)
	{
		case eST_PICKING:
		case eST_IDLE:
		case eST_PUSHINGAWAY:
		case eST_STARTDROPPING:
		case eST_MELEE:
		case eST_KILLING_GRABBEDNPC:
		case eST_KILLED_GRABBEDNPC:
		{
			if (IsNPC())
				UpdateHeldNPC();
			else
			if (!CheckObjectIsStillThere())
			{
				SetUpCollisionPropertiesAndMassInfo( false );
				m_state = eST_UNKNOWN;
				GetOwnerPlayer()->ExitPickAndThrow();
			}
			break;
		}
	}
}


//---------------------------------------------------------------------------
bool CPickAndThrowWeapon::CheckObjectIsStillThere()
{
	if (IsNPC())
		return true;
		
	// check to allow physics some time to create the constraint
	if (m_framesToCheckConstraint>0)
	{
		m_framesToCheckConstraint--;
		return true;
	}
	
	IEntity *pEntity = GetEntityObject();
	if (pEntity)
	{
		IPhysicalEntity* pPE = pEntity->GetPhysics();
		if(pPE) 
		{
			pe_status_constraint state;
			state.id = m_constraintId;

			return pPE->GetStatus( &state )!=0; // when this returns 0, is because the object was destroyed
		} 
	}
	
	return false;
}


//---------------------------------------------------------------------------
void CPickAndThrowWeapon::ThrowObject()
{
	UnAttachObject();

	IEntity *pEntity = GetEntityObject();
	if (!pEntity)
		return;

	IPhysicalEntity *pPE = pEntity->GetPhysics();
	if (!pPE)
		return;
		
	if (CPlayer *pPlayer = GetOwnerPlayer())
	{
		Vec3 dir = pPlayer->GetViewRotation().GetColumn1();
		
		{
			float speed = GetGrabTypeParams().throwSpeed;
			speed *= pPlayer->GetActorSuitGameParameters().GetProps().throwStrengthScale;

			pe_params_pos ppos;
			ppos.pos = pEntity->GetWorldPos();
			pPE->SetParams(&ppos);

			pe_action_set_velocity asv;
			asv.v = (dir*speed);
			AABB box;
			pEntity->GetWorldBounds(box);
			Vec3 finalW = -gEnv->pSystem->GetViewCamera().GetMatrix().GetColumn0()*(8.0f/max(0.1f,box.GetRadius()));
			finalW.x *= Random(0.5f,1.3f);
			finalW.y *= Random(0.5f,1.3f);
			finalW.z *= Random(0.5f,1.3f);
			if (!IsNPC())
				asv.w = finalW;
			pPE->Action(&asv);
		}
	}
}


//---------------------------------------------------------------------------
void CPickAndThrowWeapon::DropObject()
{
	UnAttachObject();
	
	IEntity *pEntity = GetEntityObject();
	if (!pEntity)
		return;

	IPhysicalEntity *pPE = pEntity->GetPhysics();
	if (!pPE)
		return;
		
	pe_params_pos ppos;
	ppos.pos = pEntity->GetWorldPos();
	pPE->SetParams(&ppos);

	pe_action_set_velocity asv;
	asv.v = Vec3(0,0,-0.1f);
	
	if (GetGrabTypeParams().dropSpeedForward>0) // extra forward impulse
	{
		Vec3 actorDir2D = GetOwnerActor()->GetViewRotation().GetColumn1();
		actorDir2D.z = 0;
		actorDir2D.normalize();
		
		Vec3 impulse = GetOwnerActor()->GetActorStats()->velocity + actorDir2D * GetGrabTypeParams().dropSpeedForward;
		asv.v.x += impulse.x;
		asv.v.y += impulse.y;
	}
	pPE->Action(&asv);
}



//---------------------------------------------------------------------------
IAttachment* CPickAndThrowWeapon::GetOwnerAttachment()
{
	IAttachment* pAttachment = NULL;
	
	IEntity* pEntity = GetOwnerActor()->GetEntity();
	SEntitySlotInfo info;
	if (pEntity->GetSlotInfo(m_objectSlot, info))
	{
		if (info.pCharacter)
		{
			ICharacterInstance *pCharacter = info.pCharacter;
			if (pCharacter)
				pAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName( GetGrabTypeParams().attachment.c_str() );
		}
	}

	return pAttachment;
}			



//---------------------------------------------------------------------------
void CPickAndThrowWeapon::UnAttachObject()
{
	IAttachment* pAttachment = GetOwnerAttachment();
	IEntity* pEntityObject = GetEntityObject();
	if (pAttachment && pEntityObject && !IsNPC())
	{
		pAttachment->ClearBinding();
		pAttachment->SetAttRelativeDefault(m_attachmentOldRelativeLoc);
		m_objectGrabbed = false;
	}
	if (IsNPC() && pEntityObject && pEntityObject->GetAI())
		pEntityObject->GetAI()->Event(AIEVENT_ENABLE,0);
}


//---------------------------------------------------------------------------
// attachs the "going to be grabbed" object to the player
void CPickAndThrowWeapon::AttachObject()
{
	IEntity* pEntity = GetEntityObject();
	if (pEntity) // cant really be NULL...
		m_objectScale = pEntity->GetScale();
	
	SetUpCollisionPropertiesAndMassInfo( true );
	DrawNearObject( true );
	
	IAttachment* pAttachment = GetOwnerAttachment();
	IEntity* pEntityObject = GetEntityObject();
	if (pAttachment && pEntityObject /*&& !IsNPC()*/)
	{
		if (IsNPC())
		{
/*
			CCHRAttachment* pCHRAttachment = new CCHRAttachment();
			pCHRAttachment->m_pCharInstance = pEntityObject->GetCharacter(0);
			IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pCHRAttachment;
			pAttachment->AddBinding(pIAttachmentObject);
			pAttachment->HideAttachment(0);
//			pEntityObject->SetUpdatePolicy( ENTITY_UPDATE_NEVER ); 
//			pEntityObject->Invisible(true);
//			pEntityObject->GetCharacter(0)->HideMaster(1);
//			pEntityObject->GetCharacter(0)->GetISkeletonPose()->SetForceSkeletonUpdate(0);
//			pEntityObject->Hide( true );
//			pEntityObject->SetFlags( pEntityObject->GetFlags() | ENTITY_FLAG_UPDATE_HIDDEN );

			m_attachmentOldRelativeLoc = pAttachment->GetAttRelativeDefault();
			Matrix34 objectHelperInvMat;
			CalcObjectHelperInverseMat( objectHelperInvMat );
			QuatT requiredRelativeLocation = m_attachmentOldRelativeLoc * QuatT(objectHelperInvMat.GetTranslation(), Quat(objectHelperInvMat).GetNormalized());
			requiredRelativeLocation.q.Normalize();

			pAttachment->SetAttRelativeDefault(requiredRelativeLocation);
*/			
			
		}
		else
		{
			CEntityAttachment *pEntityAttachment = new CEntityAttachment();
			pEntityAttachment->SetEntityId( pEntityObject->GetId() );

			pAttachment->AddBinding( pEntityAttachment );
			pAttachment->HideAttachment(0);
			
			m_attachmentOldRelativeLoc = pAttachment->GetAttRelativeDefault();
			Matrix34 objectHelperInvMat;
			CalcObjectHelperInverseMat( objectHelperInvMat );
			QuatT requiredRelativeLocation = m_attachmentOldRelativeLoc * QuatT(objectHelperInvMat.GetTranslation(), Quat(objectHelperInvMat).GetNormalized());
			requiredRelativeLocation.q.Normalize();

			pAttachment->SetAttRelativeDefault(requiredRelativeLocation);

			m_objectGrabbed = true;
		}
	}
}



//---------------------------------------------------------------------------
void CPickAndThrowWeapon::GetNanosuitHelperMatrix( Matrix34& helperMat )
{
	IEntity* pEntity = GetOwnerActor()->GetEntity();
	bool isOk = false;
	
	SEntitySlotInfo info;
	if (pEntity->GetSlotInfo(m_objectSlot, info))
	{
		if (info.pCharacter)
		{
			ICharacterInstance *pCharacter = info.pCharacter;
			if (pCharacter)
			{
				IAttachment* pAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName( GetGrabTypeParams().attachment.c_str() );
				if (pAttachment)
				{
					helperMat = Matrix34( pAttachment->GetAttWorldAbsolute() );
					isOk = true;
				}
			}
		}
	}
	
	if (!isOk)
		helperMat.SetIdentity();
}


//---------------------------------------------------------------------------
IEntity* CPickAndThrowWeapon::GetEntityObject()
{
	IEntity *pEntity = m_pEntitySystem->GetEntity( m_objectId );
	return pEntity;
}

//---------------------------------------------------------------------------
const SPickAndThrowParams::SGrabTypeParams& CPickAndThrowWeapon::GetGrabTypeParams()
{
	if (m_grabType<ePTGrabType_MaxGrabTypes)
		return m_weaponsharedparams->pPickAndThrowParams->grabTypesParams[m_grabType];
	else
		return m_weaponsharedparams->pPickAndThrowParams->grabTypesParams[ePTGrabType_1H];
}

//---------------------------------------------------------------------------
void CPickAndThrowWeapon::DecideGrabType()
{
	m_grabType = ePTGrabType_1H; //Default one

	if (gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor( m_objectId ))
		m_grabType = ePTGrabType_NPCFront;
	else
	{
		for (int i = 0; i < ePTGrabType_MaxGrabTypes; ++i)
		{
			if (FindHelperObject( m_weaponsharedparams->pPickAndThrowParams->grabTypesParams[i].helper.c_str(), m_objectId ))
			{
				m_grabType = (EPickAndThrowGrabTypes)(i);
				return;
			}
		}
	}
}

//---------------------------------------------------------------------------
bool CPickAndThrowWeapon::HasObjectGrabHelper(EntityId entityId) const
{
	const SPickAndThrowParams& pickAndThrowParams = *(m_weaponsharedparams->pPickAndThrowParams);

	for (int i = 0; i < ePTGrabType_MaxGrabTypes; ++i)
	{
		if (FindHelperObject(pickAndThrowParams.grabTypesParams[i].helper.c_str(), entityId))
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------
void CPickAndThrowWeapon::OnMeleeFinished()
{
	m_state = eST_IDLE;
	m_currentMeleeAutoTargetId = 0;
}

//---------------------------------------------------------------------------
void CPickAndThrowWeapon::OnDoMelee()
{
	if (CActor *pActor = GetOwnerActor())
	{
		if (IMovementController * pMC = pActor->GetMovementController())
		{
			if (IsNPC()) // kill the grabbed NPC
			{
				m_state = eST_KILLING_GRABBEDNPC;
				m_timePassed = 0;
				PlayAction( GetGrabTypeParams().melee_action );
				m_animDuration = GetCurrentAnimationTime( eIGS_Owner ) / 1000.f;
			}
			else  // melee with the object in hand
			{
				SMovementState info;
				pMC->GetMovementState(info);

				float range = 1.5f;
				if (IEntity* pObjectEntity = GetEntityObject())
				{
					AABB objectBBox;
					pObjectEntity->GetWorldBounds(objectBBox);

					range = (!objectBBox.IsEmpty()) ? max(objectBBox.GetRadius() + range, range) : range;
				}

				m_collisionHelper.DoCollisionTest(SCollisionTestParams(info.eyePosition, info.eyeDirection, range, pActor->GetEntityId(), m_objectId, false));
				m_currentMeleeAutoTargetId = m_collisionHelper.GetBestAutoAimTargetForUser(GetOwnerId(), info.eyePosition, info.eyeDirection, range, cos_tpl(DEG2RAD(70.0f)));
			}
		}
	}
}

//---------------------------------------------------------------------------
void CPickAndThrowWeapon::OnSuccesfulHit( ray_hit& hitResult )
{
	IEntity *pTarget = gEnv->pEntitySystem->GetEntityFromPhysics(hitResult.pCollider);
	EntityId targetId = pTarget ? pTarget->GetId() : 0;

	DoMeleeHit(hitResult, targetId);

	DoMeleeImpulse(hitResult, targetId);

	PlayAction( GetGrabTypeParams().melee_hit_action );

	m_currentMeleeAutoTargetId = 0;
}

//---------------------------------------------------------------------------
void CPickAndThrowWeapon::OnFailedHit()
{
	const SCollisionTestParams& collisionParams = m_collisionHelper.GetCollisionTestParams();

	if (m_currentMeleeAutoTargetId != 0)
	{
		m_collisionHelper.PerformMeleeOnAutoTarget(m_currentMeleeAutoTargetId);
	}

	m_currentMeleeAutoTargetId = 0;
}

//---------------------------------------------------------------------------
void CPickAndThrowWeapon::DoMeleeHit( ray_hit& hitResult, EntityId collidedEntityId )
{
	// generate the damage
	CActor *pOwnerActor = GetOwnerActor();

	if (pOwnerActor)
	{
		const SCollisionTestParams& collisionParams = m_collisionHelper.GetCollisionTestParams();

		IEntity *pTarget = gEnv->pEntitySystem->GetEntity(collidedEntityId);
		IEntity* pOwnerEntity = pOwnerActor->GetEntity();

		bool silentHit = pOwnerActor->GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Stealth;
		bool isFriendlyHit = false;

		if (IAIObject* pOwnerAI = pOwnerEntity->GetAI())
		{
			SAIEVENT AIevent;
			AIevent.targetId = pTarget ? pTarget->GetId() : 0;
			pOwnerAI->Event(AIEVENT_PLAYER_STUNT_PUNCH, &AIevent);

			IAIObject *pTargetAI = pTarget ? pTarget->GetAI() : NULL;
			isFriendlyHit = (pTargetAI) ? !pOwnerAI->IsHostile(pTargetAI, false) : false;

			// Send target stimuli
			IAISystem *pAISystem = gEnv->pAISystem;
			ITargetTrackManager *pTargetTrackManager = pAISystem ? pAISystem->GetTargetTrackManager() : NULL;
			if (pTargetTrackManager && pTargetAI)
			{
				const tAIObjectID aiOwnerId = pOwnerAI->GetAIObjectID();
				const tAIObjectID aiTargetId = pTargetAI->GetAIObjectID();

				pTargetTrackManager->HandleStimulusEvent(aiTargetId, aiOwnerId, "MeleeHit", hitResult.pt);
				pTargetTrackManager->HandleStimulusEvent(aiOwnerId, "MeleeHitNear", hitResult.pt, 5.0f);
			}
		}

		if(!isFriendlyHit)
		{
			float damage = pOwnerActor->GetActorSuitGameParameters().GetProps().meleeDamageScale * m_objectMass * GetGrabTypeParams().melee_damage_scale;

			//Generate Hit
			if(pTarget)
			{
				CGameRules *pGameRules = g_pGame->GetGameRules();

				HitInfo info(GetOwnerId(), pTarget->GetId(), GetEntityId(),
					damage, 0.0f, pGameRules->GetHitMaterialIdFromSurfaceId(hitResult.surface_idx), hitResult.partid,
					pGameRules->GetHitTypeId(silentHit ? "silentMelee" : "melee"), hitResult.pt, collisionParams.m_dir, hitResult.n);

				info.knocksDown = (Random(20.0f, 50.0f) < m_objectMass);
				info.remote = collisionParams.m_remote;

				pGameRules->ClientHit(info);
			}

			//Play Material FX
			IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();

			TMFXEffectId effectId = pMaterialEffects->GetEffectId(GetGrabTypeParams().melee_mfxLibrary.c_str(), hitResult.surface_idx);
			if (effectId != InvalidEffectId)
			{
				SMFXRunTimeEffectParams params;
				params.pos = hitResult.pt;
				params.playflags = MFX_PLAY_ALL | MFX_DISABLE_DELAY;
				params.soundSemantic = eSoundSemantic_Player_Foley;
				pMaterialEffects->ExecuteEffect(effectId, params);
			}
		}
	}
}

//---------------------------------------------------------------------------
void CPickAndThrowWeapon::DoMeleeImpulse( ray_hit& hitResult, EntityId collidedEntityId )
{
	SNanoSuitGameParameters defaultSuitParams;

	CActor* pWeaponOwner = GetOwnerActor();
	const SNanoSuitGameParameters& suitParams = pWeaponOwner ? pWeaponOwner->GetActorSuitGameParameters() : defaultSuitParams;

	float impulseScale = 1.0f;
	bool delayImpulse = false;

	IEntity* pCollidedEntity = gEnv->pEntitySystem->GetEntity(collidedEntityId);
	if (pCollidedEntity)
	{
		IGameFramework* pGameFrameWork = g_pGame->GetIGameFramework();
			
		CActor* pColliderActor = static_cast<CActor*>(pGameFrameWork->GetIActorSystem()->GetActor(collidedEntityId));
		if (pColliderActor)
		{
			SActorParams* pActorParams = pColliderActor->GetActorParams();
			if (pActorParams && pColliderActor->IsDead())
			{
				delayImpulse = true;
				impulseScale = pActorParams->meeleHitRagdollImpulseScale * suitParams.GetProps().meleeStrengthScale; 
			}
			else
			{
				impulseScale = suitParams.GetProps().meleeStrengthScale;
			}
		}
		else if (pGameFrameWork->GetIVehicleSystem()->GetVehicle(collidedEntityId))
		{
			impulseScale = suitParams.GetProps().meleeStrengthVehicleScale;
		}
	}

	const SCollisionTestParams& collisionParams = m_collisionHelper.GetCollisionTestParams();
	const Vec3 impulse = collisionParams.m_dir * m_objectMass * GetGrabTypeParams().melee_impulse_scale * impulseScale; //Scale up a bit, maybe expose this in xml as well.

	if (!delayImpulse)
	{
		m_collisionHelper.Impulse(hitResult.pCollider, hitResult.pt, impulse, hitResult.partid, hitResult.ipart);
	}
	else
	{
		GetScheduler()->TimerAction(75, CSchedulerAction<DelayedImpulse>::Create(DelayedImpulse(*this, collidedEntityId, hitResult.pt, impulse, hitResult.partid, hitResult.ipart)), true);
	}

	const Vec3 speed = collisionParams.m_dir * cry_sqrtf(2000.0f/(80.0f * 0.5f)); // 80.0f is the mass of the player
	m_collisionHelper.GenerateArtificialCollision(GetOwner(), hitResult.pCollider, hitResult.pt, hitResult.n, speed, hitResult.partid, hitResult.ipart, hitResult.surface_idx);
}

//---------------------------------------------------------------------------
bool CPickAndThrowWeapon::PrepareNPCForBeingGrabbed()
{
	CActor *pPlayer = GetOwnerActor();
	CActor *pActorNPC = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor( m_objectId ));

	if(!pActorNPC || !pPlayer)
		return false;

	IEntity *pEntityNPC = pActorNPC->GetEntity();
	if(!pEntityNPC->GetCharacter(0))
		return false;


	//The NPC holster his weapon
	bool mounted = false;
	CItem *currentItem = static_cast<CItem*>(pActorNPC->GetCurrentItem());
	if(currentItem)
	{
		if(currentItem->IsMounted() && currentItem->IsUsed())
		{
			currentItem->StopUse(pActorNPC->GetEntityId());
			mounted = true;
		}
	}

	if(!mounted)
	{
		pActorNPC->HolsterItem(false); //AI sometimes has holstered a weapon and selected a different one...
		pActorNPC->HolsterItem(true);
	}

	// TODO: OnGrabbedByByPlayer does not really exists, so it does nothing for now. enabling / disabling the AI is just a temporary hack
	pEntityNPC->GetAI()->Event(AIEVENT_DISABLE,0);

	if ( IAnimationGraphState* pAGState = pActorNPC->GetAnimationGraphState() )
	{
		IAnimationGraphState::InputID actionInputID = pAGState->GetInputId( "Action" );
		pAGState->SetInput( actionInputID, "grabNPC" );
	}

	IAnimationGraphState* pAGState = pActorNPC->GetAnimationGraphState();

	if (IAISystem *pAISystem=gEnv->pAISystem)
	{
		// Marcio: Left this here for compatibility, new event is sent below, OnGrabbedByPlayer
		IAIActor* pAIActor = CastToIAIActorSafe(pEntityNPC->GetAI());
		if(pAIActor)
		{
			IAISignalExtraData *pSData = pAISystem->CreateSignalExtraData();	
			pSData->point = Vec3(0,0,0);
			pAIActor->SetSignal(1,"OnGrabbedByPlayer",pPlayer->GetEntity(),pSData);
		}
	}

//	if (pActor->GetActorSpecies() == eGCT_HUMAN)
	{
		SActorStats *stats = pActorNPC->GetActorStats();
		stats->isGrabbed = true; //Disable IK
	} 

	pActorNPC->GetAnimatedCharacter()->SetNoMovementOverride(true);

	return true;
}


//---------------------------------------------------------------------------
void CPickAndThrowWeapon::PrepareNPCToBeDropped()
{
	CActor *pActorNPC = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor( m_objectId ));

	if(!pActorNPC)
		return;

	IEntity *pEntityNPC = pActorNPC->GetEntity();

	SActorStats *stats = pActorNPC->GetActorStats();
	stats->isGrabbed = false;
	
	// TODO: enabling / disabling the AI is just a temporary hack, need some actual AI signalling
	if (pEntityNPC->GetAI())
		pEntityNPC->GetAI()->Event(AIEVENT_ENABLE,0);
	
	pActorNPC->SetAnimationInput( "Action", "idle" );
	
	pActorNPC->SelectLastItem(true);
}
	



//---------------------------------------------------------------------------
void CPickAndThrowWeapon::PrepareNPCToBeThrown()
{
	CActor  *pActor  = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor( m_objectId ));

	if(!pActor)
		return;

	IEntity *pEntity = pActor->GetEntity();

	SActorStats *stats = pActor->GetActorStats();
	stats->isGrabbed = false;

	//Un-Hide attachments on the back
//	if(CWeaponAttachmentManager* pWAM = pActor->GetWeaponAttachmentManager())
//		pWAM->HideAllAttachments(false);

/*	CPlayer *pPlayer = static_cast<CPlayer*>(GetOwnerActor());

	if(kill)
	{
		UpdateGrabbedNPCWorldPos(pEntity,NULL);
		pActor->HolsterItem(false);
		IItem *currentItem = pActor->GetCurrentItem();

		{
			int prevHealth = pActor->GetHealth();
			int health = prevHealth-100;

			//In strenght mode, always kill
			if (health<=0)
			{
				pActor->SetHealth(0);
				if(currentItem)
					pActor->DropItem(currentItem->GetEntityId(),0.5f,false, true);

				//Don't kill if it was already dead
				if(!stats->isRagDoll || prevHealth>0)
				{
					pActor->SetAnimationInput( "Action", "idle" );
					pActor->CreateScriptEvent("kill",0);
				}
			}
			else
			{
				if(pEntity->GetAI())
				{
					pActor->SetHealth(health);
					pActor->SetAnimationInput( "Action", "idle" );
					pActor->Fall();
					PlaySound(eOHSound_Kill, true);
				}
			}

			if (!m_constraintId)
				IgnoreCollisions(true,m_heldEntityId);
		}

	}
	else */
	{
		pActor->SetAnimationInput( "Action", "idle" );
	}

	// TODO: OnThrownByPlayer does not really exists, so it does nothing for now. enabling / disabling the AI is just a temporary hack
	pEntity->GetAI()->Event(AIEVENT_ENABLE,0);

	if(IAISystem *pAISystem=gEnv->pAISystem)
	{
		IAIActor* pAIActor = CastToIAIActorSafe( pEntity->GetAI() );
		if (pAIActor)
		{
			IAISignalExtraData *pSData = pAISystem->CreateSignalExtraData();	
			pSData->point = Vec3(0,0,0);
			pAIActor->SetSignal(1,"OnThrownByPlayer", GetOwnerActor()->GetEntity(), pSData);
		}
	}

	pActor->RagDollize( false );
	pActor->GetAnimatedCharacter()->RequestPhysicalColliderMode( eColliderMode_Spectator, eColliderModeLayer_Game );

	SendMortalHitTo( pActor );
}


//---------------------------------------------------------------------------
void CPickAndThrowWeapon::SendMortalHitTo( CActor* pActor )
{
	// TODO: find a more proper way of killing the NPC
	assert( pActor );
	HitInfo hitInfo;
	hitInfo.shooterId = pActor->GetEntityId();
	hitInfo.targetId = pActor->GetEntityId();
	hitInfo.damage = 999999999.0f; // CERTAIN_DEATH
	hitInfo.dir = pActor->GetEntity()->GetForwardDir();
	hitInfo.normal = -hitInfo.dir; // this has to be in an opposite direction from the hitInfo.dir or the hit is ignored as a 'backface' hit
	hitInfo.type = g_pGame->GetGameRules()->GetHitTypeId("silentMelee");
	g_pGame->GetGameRules()->ClientHit(hitInfo);
}


//---------------------------------------------------------------------------
#ifndef _RELEASE
void CPickAndThrowWeapon::DebugDraw()
{
	if (!GetOwnerPlayer())
		return;
		
	EntityId objectId = GetOwnerPlayer()->GetCurrentInteractionInfo().interactiveEntityId;
	IEntity* pObject = m_pEntitySystem->GetEntity( objectId );
	if (pObject)
	{
		const float boxSize = 0.1f;
		const float axisSize = 1.f;
		ColorB colorBox( 255, 0, 0 );
		const ColorB red(255,0,0);
		const ColorB blue(0,0,255);
		const ColorB green(0,255,0);
		Matrix34 mat = pObject->GetWorldTM();
		const char* pHelperName = "No Helper";
		float textColor[] = {1.0f,0.0f,0.0f,1.0f};

		IStatObj::SSubObject* pSObjHelper = NULL;
		for (int i = 0; i < ePTGrabType_MaxGrabTypes && pSObjHelper==NULL; ++i)
		{
			pSObjHelper = FindHelperObject( m_weaponsharedparams->pPickAndThrowParams->grabTypesParams[i].helper.c_str(), objectId );
		}

		if (pSObjHelper)
		{
			mat = pObject->GetWorldTM() * pSObjHelper->tm;
			colorBox = ColorB(0,255,255);
			pHelperName = pSObjHelper->name.c_str();
			textColor[0] = 0; textColor[1] = 1.f; textColor[2] = 1.f;
		}

		Vec3 wposCenter = mat.GetColumn3();
		Vec3 wposX = mat * Vec3(axisSize,0,0);
		Vec3 wposY = mat * Vec3(0,axisSize,0);
		Vec3 wposZ = mat * Vec3(0,0,axisSize);

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine( wposCenter, red, wposX, red );
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine( wposCenter, green, wposY, green );
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine( wposCenter, blue, wposZ, blue );
		
		AABB box( Vec3(-boxSize, -boxSize, -boxSize), Vec3( boxSize, boxSize, boxSize ) );
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB( box, mat, false, colorBox, eBBD_Faceted );
		
		gEnv->pRenderer->DrawLabelEx( wposCenter, 2, textColor, true, true, pHelperName );
	}
}
#endif

/*
//---------------------------------------------------------------------------
void CPickAndThrowWeapon::UpdateHeldNPC()
{
	// need to update this constantly, because the enemy animation does move its helper, and that position change is not automatically managed by the attachment
	IAttachment* pAttachment = GetOwnerAttachment();
	IEntity* pEntityObject = GetEntityObject();
	Matrix34 objectHelperInvMat;
	CalcObjectHelperInverseMat( objectHelperInvMat );
	QuatT requiredRelativeLocation = m_attachmentOldRelativeLoc * QuatT(objectHelperInvMat.GetTranslation(), Quat(objectHelperInvMat).GetNormalized());
	requiredRelativeLocation.q.Normalize();
	pAttachment->SetAttRelativeDefault(requiredRelativeLocation);

return;
	// also need to update enemy position even when using attachments, because CharAttachment does not update it, unlike the EntityAttachment	used for inert objects
	IEntity *pEntity = GetEntityObject();
	if (pEntity)
	{
//		Matrix34 helperMat;
//		GetNanosuitHelperMatrix( helperMat );
//		pEntity->SetWorldTM( helperMat, ENTITY_XFORM_USER);  // uses directly the nanosuit helper matrix because that helper is already repositioned at the origin of the grabbed enemy
	} 
}

*/

//---------------------------------------------------------------------------
void CPickAndThrowWeapon::UpdateHeldNPC()
{
	IEntity *pEntity = GetEntityObject();
	if (pEntity)
	{
		Matrix34 objectHelperInvMat;
		CalcObjectHelperInverseMat( objectHelperInvMat );
		
		Matrix34 helperMat;
		GetNanosuitHelperMatrix( helperMat );
		Matrix34 finalMat = helperMat * objectHelperInvMat;
		 	 
		pEntity->SetWorldTM( finalMat, ENTITY_XFORM_USER);
	}
}
