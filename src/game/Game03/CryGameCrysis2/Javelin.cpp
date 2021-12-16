/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 10:11:2009   11:10 : Created by Ben Parbury

*************************************************************************/
#include "StdAfx.h"
#include "Javelin.h"

#include "ItemParamReader.h"
#include "Player.h"
#include "PlayerInput.h"
#include "GameRules.h"
#include "SShootHelper.h"
#include "JavelinMissile.h"
#include "GameActions.h"

#include "HUD/UI/UISimpleBar.h"
#include "HUD/UI/UIButtonPromptRegion.h"

#define JAVELIN_ATTACHMENT_NAME "left_weapon"
#define JAVELIN_ACTION_SUFFIX "javelin"

//------------------------------------------------------------------------
void CJavelin::ResetVariables()
{
	m_state = EJS_Pickable;
	m_deployProgress = 0.0f;
	m_defuseProgress = 0.0f;
	m_defuserId = 0;
	m_targetId = 0;
	m_heldTimer = -1.0f;
	m_droppedLocation(0.0f, 0.0f, 0.0f);
	m_deployerId = 0;
	m_inLaunchArea = false;
	m_inputsLocked = false;
}

//--------------------------------------------------------------------
CJavelin::CJavelin()
{
	ResetVariables();
}

//------------------------------------------------------------------------
void CJavelin::OnReset()
{
	CItem::OnReset();

	ResetVariables();
}

//------------------------------------------------------------------------
void CJavelin::PostInit( IGameObject * pGameObject )
{
	EnableUpdate(true, eIUS_General);
	RequireUpdate(eIUS_General);

	EnableUpdate(false, eIUS_Zooming);
	EnableUpdate(false, eIUS_FireMode);
	EnableUpdate(false, eIUS_Scheduler);

	CItem::PostInit(pGameObject);
}

//-------------------------------------------------------------------------
CJavelin::~CJavelin()
{
	if (gEnv->bClient)
	{
		ClEnableInputFilter(false);
	}
}

//------------------------------------------------------------------------
bool CJavelin::ReadItemParams(const IItemParamsNode *root)
{
	if (!CItem::ReadItemParams(root))
		return false;

	const IItemParamsNode *missileNode = root->GetChild("missile");
	if (missileNode)
	{
		CItemParamReader reader(missileNode);
		reader.Read("name", m_missileParams.name);
		reader.Read("type", m_missileParams.type);
		reader.Read("damage", m_missileParams.damage);
		reader.Read("helper", m_missileParams.helper);
	}

	const IItemParamsNode *timingNode = root->GetChild("timing");
	if (timingNode)
	{
		CItemParamReader reader(timingNode);
		reader.Read("deployTime", m_timingParams.deployTime);
		reader.Read("defuseTime", m_timingParams.defuseTime);
		reader.Read("launchTime", m_timingParams.launchTime);
		reader.Read("tapToDropTime", m_timingParams.tapToDropTime);

		if(m_timingParams.deployTime == 0.0f)
		{
			m_timingParams.deployTime = 0.0001f;
		}
		if(m_timingParams.defuseTime == 0.0f)
		{
			m_timingParams.defuseTime = 0.0001f;
		}
	}

	const IItemParamsNode *targetNode = root->GetChild("target");
	if (targetNode)
	{
		CItemParamReader reader(targetNode);
		reader.Read("targetClass", m_targetParams.targetClass);
	}

	const IItemParamsNode *audioNode = root->GetChild("audio");
	if (audioNode)
	{
		CItemParamReader reader(audioNode);
		reader.Read("pickup", m_audioParams.pickup);
		reader.Read("defused", m_audioParams.defused);
	}

	return true;
}


bool CJavelin::IsPickable() const
{
	int javelinTeam = g_pGame->GetGameRules()->GetTeam(GetEntityId());
	int userTeam = g_pGame->GetGameRules()->GetTeam(gEnv->pGame->GetIGameFramework()->GetClientActorId());

	return m_state == EJS_Pickable && javelinTeam == userTeam;
}

//-------------------------------------------------------------------------
bool CJavelin::CanPickUp(EntityId userId) const
{
	int javelinTeam = g_pGame->GetGameRules()->GetTeam(GetEntityId());
	int userTeam = g_pGame->GetGameRules()->GetTeam(userId);

	return m_state == EJS_Pickable && javelinTeam == userTeam;
}

//-------------------------------------------------------------------------
void CJavelin::PickUp(EntityId pickerId, bool sound, bool select, bool keepHistory, const char* setup)
{
	CActor *pActor=GetActor(pickerId);
	if (!pActor)
		return;

	SetState(EJS_Carried, gEnv->bServer);

	CItem::PickUp(pickerId, sound, select, keepHistory, setup);

	IEntity *pActorEntity = pActor->GetEntity();
	ICharacterInstance *pCharacter = pActorEntity->GetCharacter(0);
	IAttachmentManager *pIAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment *pIAttachment = pIAttachmentManager->GetInterfaceByName(JAVELIN_ATTACHMENT_NAME);

	CRY_ASSERT_MESSAGE(pIAttachment, "Missing attachment "JAVELIN_ATTACHMENT_NAME" for javelin");
	if (pIAttachment)
	{
		CEntityAttachment *pEntityAttachment = new CEntityAttachment();
		pEntityAttachment->SetEntityId(GetEntityId());
		pIAttachment->AddBinding(pEntityAttachment);
		pIAttachment->HideAttachment(0);
	}

	pActor->SelectNextItem(1, true, "secondary");	//switch to revolver or razor 5

	CItem *pItem = static_cast<CItem*>(pActor->GetCurrentItem());
	if (pItem)
	{
		pItem->SetActionSuffix(JAVELIN_ACTION_SUFFIX, JAVELIN_ACTION_SUFFIX);
	}

	pActor->EnableSwitchingItems(false);
	pActor->EnableIronSights(false);
	pActor->EnablePickingUpItems(false);

	EnableUpdate(false, eIUS_Zooming);
	EnableUpdate(false, eIUS_FireMode);
	EnableUpdate(false, eIUS_Scheduler);

	if (gEnv->bClient && (pickerId == g_pGame->GetIGameFramework()->GetClientActorId()))
	{
		m_inLaunchArea = IsInsideDeploymentArea(pickerId);
		ClChangedInsideState();
	}

	IEntity* pEntity = GetEntity();
	pEntity->Hide(false);
	SetViewMode(eIVM_ThirdPerson);

	pEntity->SetSlotFlags(k_renderSlot, pEntity->GetSlotFlags(k_renderSlot)|ENTITY_SLOT_RENDER_NEAREST);
}

//-------------------------------------------------------------------------
bool CJavelin::CanUse(EntityId userId) const
{
	//if not friendly
	int javelinTeam = g_pGame->GetGameRules()->GetTeam(GetEntityId());
	int userTeam = g_pGame->GetGameRules()->GetTeam(userId);

	return (m_state == EJS_Deployed || m_state == EJS_Pickable) && javelinTeam != userTeam && !m_droppedLocation.IsZero();
}

//-------------------------------------------------------------------------
void CJavelin::Use(EntityId userId)
{
	CPlayer* pDefuser = static_cast<CPlayer*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(userId));
	if(pDefuser)
	{
		GetGameObject()->InvokeRMI(ClCanDefuse(), NoParams(), eRMI_ToClientChannel, pDefuser->GetChannelId());	
	}
}

//-------------------------------------------------------------------------
bool CJavelin::CanDrop() const
{
	return true;
}

//-------------------------------------------------------------------------
void CJavelin::Drop(float impulseScale, bool selectNext, bool byDeath)
{
	CActor *pActor=GetOwnerActor();
	if (!pActor)
		return;

	CItem *pItem = static_cast<CItem*>(pActor->GetCurrentItem());
	if (pItem)
	{
		pItem->ResetActionSuffix();
		if(pActor->IsClient())
		{
			pItem->GetEntity()->SetSlotFlags(0, pItem->GetEntity()->GetSlotFlags(0) &~ ENTITY_SLOT_RENDER_NEAREST); 
		}
	}

	pActor->EnableSwitchingItems(true);
	pActor->EnableIronSights(true);
	pActor->EnablePickingUpItems(true);

	CItem::Drop(0.0f, selectNext, byDeath);

	CRY_TODO(23, 11, 2009, "Look at why it's not switching back to previous weapon");
	pActor->SelectNextItem(1, true, "primary");	//switch to primary...

	//still want to update even when dropped
	EnableUpdate(true, eIUS_General);
	RequireUpdate(eIUS_General);

	m_heldTimer = -1.0f;

	if(gEnv->bServer)
	{
		//server syncs location (as physics doesn't)
		const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;
		ray_hit hitResult;

		if(gEnv->pPhysicalWorld->RayWorldIntersection(GetEntity()->GetWorldPos() + pActor->GetEntity()->GetForwardDir(), Vec3(0.0f, 0.0f, -2.0f) , ent_all, flags, &hitResult, 1))
		{
			m_droppedLocation = hitResult.pt;
		}
		else
		{
			CRY_ASSERT_MESSAGE(false, "Unable to find ground where deploying javelin");
			m_droppedLocation = GetEntity()->GetWorldPos() + pActor->GetEntity()->GetForwardDir();
		}
		if(m_state == EJS_Carried)	//Dropped when carried put it's back to pickable
		{
			SetState(EJS_Pickable, gEnv->bServer);
		}
	}

	ICharacterInstance *pCharacter = pActor->GetEntity()->GetCharacter(0);
	IAttachmentManager *pIAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment *pIAttachment = pIAttachmentManager->GetInterfaceByName(JAVELIN_ATTACHMENT_NAME);

	CRY_ASSERT_MESSAGE(pIAttachment, "Missing attachment "JAVELIN_ATTACHMENT_NAME" for javelin");
	if (pIAttachment)
	{
		pIAttachment->ClearBinding();
	}

	Physicalize(true, false);
	
	IEntity* pEntity = GetEntity();
	pEntity->SetSlotFlags(k_renderSlot, pEntity->GetSlotFlags(k_renderSlot)&~ENTITY_SLOT_RENDER_NEAREST);
}

//------------------------------------------------------------------------
void CJavelin::Update( SEntityUpdateContext& ctx, int slot)
{
	assert(eIUS_General == slot);

	if(m_state == EJS_Pickable)
	{
		if(!m_droppedLocation.IsZero(0.05f))
		{
			SetDroppedPositionAndRotation();

			CheckDefuse(ctx.fFrameTime);
		}
	}
	else if(m_state == EJS_Carried)
	{
		CPlayer* pOwnerPlayer = static_cast<CPlayer*>(GetOwnerActor());
		if(pOwnerPlayer && pOwnerPlayer->GetPlayerInput())
		{
			if (pOwnerPlayer->IsClient())
			{
				bool isInLaunchArea = IsInsideDeploymentArea(pOwnerPlayer->GetEntityId());
				if (m_inLaunchArea != isInLaunchArea)
				{
					m_inLaunchArea = isInLaunchArea;
					ClChangedInsideState();
				}
			}

			uint32 actions = pOwnerPlayer->GetPlayerInput()->GetActions();
			if((actions & ACTION_USE) == ACTION_USE)
			{
				if(m_heldTimer >= 0.0f)	//this is to stop you dropping the javelin when you first pick it up
				{
					m_heldTimer += ctx.fFrameTime;
				}

				if(IsInsideDeploymentArea(pOwnerPlayer->GetEntityId()))
				{
					SetState(EJS_Deploying, gEnv->bServer);
				}
			}
			else
			{
				if(m_heldTimer < (m_timingParams.tapToDropTime) && m_heldTimer > 0.0f)
				{
					SetState(EJS_Pickable, gEnv->bServer);
				}
				else
				{
					m_heldTimer = 0.0f;
				}
			}

		}
	}
	else if(m_state == EJS_Deploying)
	{
		SetDroppedPositionAndRotation();

		EntityId clientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
		if(m_deployerId == clientId)
		{
			CPlayer *pDeployerPlayer = static_cast<CPlayer *>(g_pGame->GetIGameFramework()->GetClientActor());
			if(pDeployerPlayer && pDeployerPlayer->GetPlayerInput())
			{
				uint32 actions = pDeployerPlayer->GetPlayerInput()->GetActions();
				if(pDeployerPlayer->GetHealth() > 0 && (actions & ACTION_USE) == ACTION_USE)
				{
					m_deployProgress += ctx.fFrameTime/m_timingParams.deployTime;
					ClEnableInputFilter(true);
				}
				else
				{
					SetState(EJS_Pickable, gEnv->bServer);
					m_deployProgress = 0.0f;
					ClEnableInputFilter(false);
				}

				m_deployProgress = clamp(m_deployProgress, 0.0f, 1.0f);

				if(m_deployProgress == 1.0f)
				{
					SetState(EJS_Deployed, gEnv->bServer);
				}
			}
		}
	}
	else if(m_state == EJS_Deployed)
	{
		SetDroppedPositionAndRotation();

		CheckDefuse(ctx.fFrameTime);

		if(m_deployProgress > 0.0f)
		{
			m_deployProgress -= ctx.fFrameTime;
		}
		else
		{
			SetState(EJS_Finished, gEnv->bServer);
		}
	}
	if(gEnv->bClient)
		UpdateStateFeedback((EJS_JavelinState) m_state);
}

//------------------------------------------------------------------------
bool CJavelin::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if(aspect == ASPECT_JAVELIN)
	{
		bool reading = ser.IsReading();
		int state = m_state;
		ser.Value("state", state, 'ui2');
		ser.Value("location", m_droppedLocation, 'wrl2');
		if(reading)
		{
			SetState((EJS_JavelinState) state, true);
		}
	}

	return CItem::NetSerialize(ser, aspect, profile, flags);
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CJavelin, SvRequestJavelinState)
{
	//currently blindly accepting
	assert(gEnv->bServer);
	SetState((EJS_JavelinState) params.m_state, true);

	return true;
}

//-------------------------------------------------------------------------
IMPLEMENT_RMI(CJavelin, ClCanDefuse)
{
	m_defuserId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	ClEnableInputFilter(true);

	return true;
}

//------------------------------------------------------------------------
void CJavelin::SetState(EJS_JavelinState newState, bool serverCommand)
{
	if(m_state != newState)
	{
		if(!gEnv->bServer && !serverCommand && (newState != EJS_Finished))
		{
			GetGameObject()->InvokeRMI(SvRequestJavelinState(), RequestJavelinState(newState), eRMI_ToServer);
		}

		if(serverCommand)
		{
			m_state = newState;
			if(gEnv->bServer)
			{
				CHANGED_NETWORK_STATE(this, ASPECT_JAVELIN);
			}

			if(newState == EJS_Pickable)
			{
				m_deployProgress = 0.0f;
				if(gEnv->bServer)
					Drop();
			}
			else if(newState == EJS_Deployed)
			{
				OpenAnimation();
				m_deployProgress = m_timingParams.launchTime;
			}
			else if(newState == EJS_Deploying)
			{
				if(gEnv->bServer)
				{
					m_deployerId = m_owner.GetId();
					Drop();
				}
			}
			else if(newState == EJS_Defused)
			{
				if(gEnv->bServer)
					RemoveEntity();
			}
			else if (newState == EJS_Finished)
			{
				FireJavelinMissile();
			}

			if(gEnv->bClient)
			{
				ChangedStateFeedback(newState);
				ClEnableInputFilter(false);
			}
		}
	}
}

//------------------------------------------------------------------------
EntityId CJavelin::GetTargetEntityId()
{
	if(m_targetId == 0)
	{
		int javelinTeam = g_pGame->GetGameRules()->GetTeam(GetEntityId());
		//This is a bite slow and ugly, but there'll only ever be at the end of a PSL game
		IEntityClass* pTargetClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(m_targetParams.targetClass);
		CRY_ASSERT_MESSAGE(pTargetClass, ("Unable to find Target class '%s'", m_targetParams.targetClass));
		if(pTargetClass)
		{
			IEntityIt* it = gEnv->pEntitySystem->GetEntityIterator();
			while ( !it->IsEnd() )
			{
				IEntity* pEntity = it->Next();
				if(pEntity->GetClass() == pTargetClass)
				{
					EntityId targetId = pEntity->GetId();
					int targetTeam = g_pGame->GetGameRules()->GetTeam(targetId);
					if(javelinTeam != targetTeam)
					{
						m_targetId = targetId;
						break;
					}
				}
			}
			it->Release();
		}
	}

	return m_targetId;
}

//------------------------------------------------------------------------
void CJavelin::SetDroppedPositionAndRotation()
{
	assert(!m_droppedLocation.IsZero());
	GetEntity()->SetPos(m_droppedLocation);

	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(GetTargetEntityId());
	if(pEntity)
	{
		AABB box;
		pEntity->GetWorldBounds(box);
		Vec3 target = box.GetCenter();
		Vec3 dir = m_droppedLocation - target;
		dir.z = 0.0f;
		dir.normalize();

		Quat rotation;
		rotation.SetIdentity();
		rotation.SetRotationVDir(dir, 0.0f);
		GetEntity()->SetRotation(rotation);
	}
}

//------------------------------------------------------------------------
bool CJavelin::IsInsideDeploymentArea(EntityId userId)
{
	IEntity *pTarget = gEnv->pEntitySystem->GetEntity(GetTargetEntityId());
	if(pTarget)
	{
		IScriptTable *pEntityScript = pTarget->GetScriptTable();
		HSCRIPTFUNCTION pFnIsInside = 0;
		if (pEntityScript && pEntityScript->GetValue("IsInside", pFnIsInside))
		{
			bool result = false;
			if (Script::CallReturn(gEnv->pScriptSystem, pFnIsInside, pEntityScript, ScriptHandle(userId), result))
			{
				return result;
			}
		}

		CRY_ASSERT_MESSAGE(false, "Unable to find IsInside function on target entity");
	}
	return true;
}

//------------------------------------------------------------------------
void CJavelin::FireJavelinMissile()
{
	IEntity* pEntity = GetEntity();
	Vec3 firePos = GetSlotHelperPos(eIGS_ThirdPerson, m_missileParams.helper, true);
	Matrix33 rot = GetSlotHelperRotation(eIGS_ThirdPerson, m_missileParams.helper, true);
	Vec3 dir(0.0f, 0.0f, 1.0f);
	dir = rot * dir;
	CProjectile* pMissile = SShootHelper::Shoot(m_deployerId, m_missileParams.name, m_missileParams.type, firePos, dir, m_missileParams.damage);
	if(pMissile)
	{
		CJavelinMissile* pJavelinMissile = static_cast<CJavelinMissile*>(pMissile);
		pJavelinMissile->SetDestination(GetTargetEntityId());
	}
}

//------------------------------------------------------------------------
void CJavelin::UpdateStateFeedback(EJS_JavelinState state)
{
	if(m_state == EJS_Deploying)
	{
		if(m_deployProgress > 0.0f)
		{
			string msg;
			msg.Format("Deploying...");
			CUIButtonPromptRegion::SetOnScreenMessageText("JavelinBarText", msg.c_str(), NULL, 0.5f);

			CUISimpleBar * deployBar = CUISimpleBar::GetInstanceWithName("JavelinBar");
			if (deployBar)
			{
				deployBar->Set(m_deployProgress);
			}
		}
	}
	else if(state == EJS_Pickable)
	{
		DefuseFeedback();
	}
	else if(state == EJS_Deployed)
	{
		DefuseFeedback();

		if(m_deployProgress > 0.0f)
		{
			CGameRules *pGameRules = g_pGame->GetGameRules();
			int localTeam = pGameRules->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());
			int javelinTeam = g_pGame->GetGameRules()->GetTeam(GetEntityId());

			if (localTeam == javelinTeam)
			{
				string msg;
				msg.Format("Our Javelin launches in %d", (int) m_deployProgress);
				CUIButtonPromptRegion::SetOnScreenMessageText("JavelinCountdownFriendly", msg.c_str(), NULL, 0.5f);
			}
			else
			{
				string msg;
				msg.Format("Enemy Javelin launch in %d", (int) m_deployProgress);
				CUIButtonPromptRegion::SetOnScreenMessageText("JavelinCountdownEnemy", msg.c_str(), NULL, 0.5f);
			}
		}
	}
}

//------------------------------------------------------------------------
void CJavelin::DefuseFeedback()
{
	EntityId clientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	if(m_defuserId == clientId)
	{
		string msg;
		msg.Format("Defusing...");
		CUIButtonPromptRegion::SetOnScreenMessageText("JavelinBarText", msg.c_str(), NULL, 0.5f);

		CUISimpleBar * deployBar = CUISimpleBar::GetInstanceWithName("JavelinBar");
		if (deployBar)
		{
			deployBar->Set(m_defuseProgress);
		}
	}
}

//------------------------------------------------------------------------
void CJavelin::CheckDefuse(float fFrameTime)
{
	EntityId clientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	if(m_defuserId == clientId)
	{
		CPlayer* pDefuser = static_cast<CPlayer*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_defuserId));
		if(pDefuser)
		{
			uint32 actions = pDefuser->GetPlayerInput()->GetActions();
			if(pDefuser->GetHealth() > 0 && (actions & ACTION_USE) == ACTION_USE)
			{
				m_defuseProgress += fFrameTime/m_timingParams.defuseTime;
			}
			else
			{
				m_defuserId = 0;
				ClEnableInputFilter(false);
			}

			if(m_defuseProgress > 1.0f)
			{
				SetState(EJS_Defused, gEnv->bServer);
			}
		}
	}
};

//------------------------------------------------------------------------
void CJavelin::ChangedStateFeedback(EJS_JavelinState newState)
{
	if(newState == EJS_Carried)
	{
		CAudioSignalPlayer::JustPlay(m_audioParams.pickup, m_droppedLocation);
	}
	else if(newState == EJS_Defused)
	{
		CAudioSignalPlayer::JustPlay(m_audioParams.defused, m_droppedLocation);

		CGameRules *pGameRules = g_pGame->GetGameRules();
		int localTeam = pGameRules->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());
		int javelinTeam = g_pGame->GetGameRules()->GetTeam(GetEntityId());

		if (localTeam == javelinTeam)
		{
			CUIButtonPromptRegion::SetOnScreenMessageText("JavelinCountdownFriendly", "Our Javelin has been defused", NULL, 3.5f);
		}
		else
		{
			CUIButtonPromptRegion::SetOnScreenMessageText("JavelinCountdownEnemy", "Enemy Javelin has been defused", NULL, 3.5f);
		}
	}
}

//------------------------------------------------------------------------
void CJavelin::ClChangedInsideState()
{
	if (m_inLaunchArea)
	{
		g_pGame->GetGameRules()->OnTextMessage(eTextMessageAnnouncement, "@ui_msg_psl_launch_missile");
	}
	else
	{
		g_pGame->GetGameRules()->OnTextMessage(eTextMessageAnnouncement, "@ui_msg_psl_get_to_launch_site");
	}
}

//------------------------------------------------------------------------
void CJavelin::ClEnableInputFilter( bool enable )
{
	if (enable && !m_inputsLocked)
	{
		g_pGameActions->FilterUseKeyOnly()->Enable(true);
		m_inputsLocked = true;

		// If the player is already moving then we need to clear the inputs
		CPlayer *pPlayer = static_cast<CPlayer *>(g_pGame->GetIGameFramework()->GetClientActor());
		IPlayerInput* pInput = pPlayer ? pPlayer->GetPlayerInput() : NULL;
		if(pInput && (pInput->GetType() == IPlayerInput::PLAYER_INPUT))
		{
			CPlayerInput *pPlayerInput = static_cast<CPlayerInput *>(pInput);
			pPlayerInput->ClearAllExceptAction(ACTION_USE);
		}
	}
	else if (!enable && m_inputsLocked)
	{
		g_pGameActions->FilterUseKeyOnly()->Enable(false);
		m_inputsLocked = false;
	}
}

void CJavelin::OpenAnimation()
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(1);
	if (!pCharacter)
		return;

	ISkeletonAnim* pISkeletonAnim=pCharacter->GetISkeletonAnim();
	if(!pISkeletonAnim)
		return;

	CryCharAnimationParams params;
	bool result = pISkeletonAnim->StartAnimation("javelin_open", params);
	CRY_ASSERT_MESSAGE(result, "Failed to play animation on Javelin");
}
