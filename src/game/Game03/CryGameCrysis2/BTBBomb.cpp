/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 01:12:2009   : Created by Colin Gulliver

*************************************************************************/
#include "StdAfx.h"
#include "BTBBomb.h"

#include "ItemParamReader.h"
#include "Player.h"
#include "PlayerInput.h"
#include "GameRules.h"
#include "GameActions.h"
#include "IInteractor.h"
#include "IParticles.h"

#include "HUD/UI/UISimpleBar.h"
#include "HUD/UI/UIButtonPromptRegion.h"

#include "Utility/CryWatch.h"

#include "GameRulesModules/IGameRulesScoringModule.h"

#define BTBBOMB_ATTACHMENT_NAME		"left_weapon"
#define BTBBOMB_ACTION_SUFFIX			"flag"

#define BTBBOMB_REMOVE_TIME				3.f

#if NUM_ASPECTS > 8
	#define BTBBOMB_STATE_ASPECT		eEA_GameServerA
#else
	#define BTBBOMB_STATE_ASPECT		eEA_GameServerStatic
#endif

//--------------------------------------------------------------------
CBTBBomb::CBTBBomb()
{
	ResetVariables();
}

//-------------------------------------------------------------------------
CBTBBomb::~CBTBBomb()
{
	if (gEnv->bClient)
	{
		ClEnableInputFilter(false);
	}
}

//------------------------------------------------------------------------
void CBTBBomb::OnReset()
{
	CItem::OnReset();

	ResetVariables();
}

//------------------------------------------------------------------------
void CBTBBomb::ResetVariables()
{
	m_state = eBS_Dropped;
	m_droppedLocation(0.0f, 0.0f, 0.0f);
	m_deployerId = 0;
	m_deployerTeam = 0;
	m_removeTime = 0.f;
	m_deployedTime = 0.f;
	m_inputsLocked = false;
	m_sentRequest = false;
	m_targetState = eBS_Invalid;
	m_isUsing = false;
	m_dropTime = 0.f;
}

//------------------------------------------------------------------------
void CBTBBomb::PostInit( IGameObject * pGameObject )
{
	EnableUpdate(true, eIUS_General);
	RequireUpdate(eIUS_General);

	EnableUpdate(false, eIUS_Zooming);
	EnableUpdate(false, eIUS_FireMode);
	EnableUpdate(false, eIUS_Scheduler);

	CItem::PostInit(pGameObject);
}

//-------------------------------------------------------------------------
bool CBTBBomb::IsPickable() const
{
	return false;
}

//-------------------------------------------------------------------------
bool CBTBBomb::CanPickUp(EntityId userId) const
{
	return false;
}

//-------------------------------------------------------------------------
bool CBTBBomb::CanUse(EntityId userId) const
{
	int itemTeam = g_pGame->GetGameRules()->GetTeam(GetEntityId());
	int userTeam = g_pGame->GetGameRules()->GetTeam(userId);

	if (m_state == eBS_Dropped)
	{
		return true;
	}
	else if (m_state == eBS_Carried && userId == m_owner.GetId())
	{
		return true;
	}
	else if (m_state == eBS_Deployed && m_deployerTeam != userTeam)
	{
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------
void CBTBBomb::Use(EntityId userId)
{
	StartUse(userId);
}

//------------------------------------------------------------------------
void CBTBBomb::StartUse( EntityId userId )
{
	if(!m_sharedparams->pBTBParams)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "BOMB THE BASE PARAMS: Item of type CBTBBomb is missing it's bomb the base params!");
		return;
	}

	if (gEnv->bServer)
	{
		CActor *pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(userId));
		if (pActor)
		{
			if (pActor->IsMigrating())
			{
				// Force the bomb into the dropped state so it can be picked up
				m_state = eBS_Dropped;

				SSetStateParams params;
				params.m_interactorId = userId;
				params.m_state = eBS_Carried;
				SvReceivedStateRequest(params);
				return;
			}
			else
			{
				pActor->GetGameObject()->InvokeRMI(CActor::ClStartUse(), CActor::ItemIdParam(GetEntityId()), eRMI_ToClientChannel|eRMI_NoLocalCalls, pActor->GetChannelId());
			}
		}
	}
	if (gEnv->bClient && g_pGame->GetIGameFramework()->GetClientActorId() == userId)
	{
		m_interactorId = userId;
		m_interactionTime = 0.f;
		if (m_state == eBS_Dropped)
		{
			m_requiredInteractionTime = m_sharedparams->pBTBParams->pickupTime;
			m_targetState = eBS_Carried;
		}
		else if (m_state == eBS_Carried)
		{
			m_requiredInteractionTime = m_sharedparams->pBTBParams->deployTime;
			m_targetState = eBS_Deployed;
		}
		else if (m_state == eBS_Deployed)
		{
			m_requiredInteractionTime = m_sharedparams->pBTBParams->defuseTime;
			m_targetState = eBS_Defused;
		}
		m_sentRequest = false;

		m_isUsing = true;
		m_dropTime = m_sharedparams->pBTBParams->tapToDropTime + 1.f;
	}
}

//-------------------------------------------------------------------------
bool CBTBBomb::CanDrop() const
{
	return true;
}

//-------------------------------------------------------------------------
void CBTBBomb::Drop(float impulseScale, bool selectNext, bool byDeath)
{
	CryLog("CBTBBomb::Drop()");

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

	m_state = eBS_Dropped;

	CRY_TODO(23, 11, 2009, "Look at why it's not switching back to previous weapon");
	pActor->SelectNextItem(1, true, "primary");	//switch to primary...

	//still want to update even when dropped
	EnableUpdate(true, eIUS_General);
	RequireUpdate(eIUS_General);

	ICharacterInstance *pCharacter = pActor->GetEntity()->GetCharacter(0);
	IAttachmentManager *pIAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment *pIAttachment = pIAttachmentManager->GetInterfaceByName(BTBBOMB_ATTACHMENT_NAME);

	CRY_ASSERT_MESSAGE(pIAttachment, "Missing attachment "BTBBOMB_ATTACHMENT_NAME" for BTBBomb");
	if (pIAttachment)
	{
		pIAttachment->ClearBinding();
	}

	Physicalize(true, false);
	
	IEntity* pEntity = GetEntity();
	pEntity->SetSlotFlags(k_renderSlot, pEntity->GetSlotFlags(k_renderSlot)&~ENTITY_SLOT_RENDER_NEAREST);
}

//------------------------------------------------------------------------
void CBTBBomb::Update( SEntityUpdateContext& ctx, int slot)
{
	if (eIUS_General == slot)
	{
		if(!m_sharedparams->pBTBParams)
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "BOMB THE BASE PARAMS: Item of type CBTBBomb is missing it's bomb the base params!");
			return;
		}

		if (gEnv->bServer)
		{
			if (m_state == eBS_Deployed)
			{
				float currentTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
				if ((currentTime - m_deployedTime) > m_sharedparams->pBTBParams->explodeTime)
				{
					SSetStateParams params;
					params.m_interactorId = m_deployerId;
					params.m_state = eBS_Finished;
					SvReceivedStateRequest(params);
				}
			}
			else if (m_state == eBS_Defused || m_state == eBS_Finished)
			{
				float oldTime = m_removeTime;
				m_removeTime += ctx.fFrameTime;
				if (m_removeTime > BTBBOMB_REMOVE_TIME)
				{
					if (oldTime <= BTBBOMB_REMOVE_TIME)
					{
						gEnv->pEntitySystem->RemoveEntity(GetEntityId());
					}
					return;
				}
			}
		}

		if (gEnv->bClient)
		{
			if (m_state == eBS_Deployed)
			{
				float timeToExplode = m_sharedparams->pBTBParams->explodeTime - (gEnv->pTimer->GetFrameStartTime().GetSeconds() - m_deployedTime);
				CryWatch("Exploding in %f", timeToExplode);
			}

			bool canUse = false;
			if ((m_state == eBS_Dropped || m_state == eBS_Deployed) && m_interactorId)
			{
				CPlayer* pPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_interactorId));
				if (pPlayer && pPlayer->GetPlayerInput() && pPlayer->IsClient())
				{
					uint32 actions = pPlayer->GetPlayerInput()->GetActions();
					if((actions & ACTION_USE) == ACTION_USE)
					{
						IInteractor *pInteractor = pPlayer->GetInteractor();
						if (pInteractor)
						{
							if (GetEntityId() == (EntityId) pInteractor->GetOverEntityId())
							{
								canUse = true;
							}
						}
					}
				}
			}
			else if ((m_state == eBS_Carried) && (m_owner.GetId() == g_pGame->GetIGameFramework()->GetClientActorId()))
			{
				CPlayer* pPlayer = static_cast<CPlayer*>(m_owner.GetActorRef().Get());
				if (pPlayer && pPlayer->GetPlayerInput())
				{
					uint32 actions = pPlayer->GetPlayerInput()->GetActions();
					if((actions & ACTION_USE) == ACTION_USE)
					{
						if (!m_isUsing)
						{
							m_isUsing = true;
							m_dropTime = 0.f;
						}
						m_dropTime += ctx.fFrameTime;
						if (ClCanDeploy())
						{
							canUse = true;
							if (m_targetState != eBS_Deployed)
							{
								m_targetState = eBS_Deployed;
								m_requiredInteractionTime = m_sharedparams->pBTBParams->deployTime;
								m_sentRequest = false;
							}
						}
					}
					else
					{
						if (m_targetState == eBS_Deployed)
						{
							m_targetState = eBS_Invalid;
						}
						if (m_isUsing)
						{
							m_isUsing = false;
							if (m_dropTime < m_sharedparams->pBTBParams->tapToDropTime)
							{
								IInteractor *pInteractor = pPlayer->GetInteractor();
								if (pInteractor)
								{
									EntityId overEntityId = (EntityId) pInteractor->GetOverEntityId();
									if (!overEntityId || overEntityId == GetEntityId())
									{
										pPlayer->DropItem(GetEntityId());
										m_state = eBS_Dropped;
									}
								}
								return;
							}
						}
					}
				}
			}

			if (canUse && (m_targetState != eBS_Invalid))
			{
				m_interactionTime += ctx.fFrameTime;

				CRY_TODO(18, 11, 2009, "Text should be localised and should use own hud element");
				if (m_targetState == eBS_Carried)
				{
					CUIButtonPromptRegion::SetOnScreenMessageText("JavelinBarText", "Picking up...", NULL, 0.5f);
				}
				else if (m_targetState == eBS_Deployed)
				{
					CUIButtonPromptRegion::SetOnScreenMessageText("JavelinBarText", "Planting...", NULL, 0.5f);
				}
				else if (m_targetState == eBS_Defused)
				{
					CUIButtonPromptRegion::SetOnScreenMessageText("JavelinBarText", "Defusing...", NULL, 0.5f);
				}

				CUISimpleBar * deployBar = CUISimpleBar::GetInstanceWithName("JavelinBar");
				if (deployBar)
				{
					float percCompleted = m_interactionTime / m_requiredInteractionTime;
					deployBar->Set(clamp(percCompleted, 0.f, 1.f));
				}

				if (m_interactionTime > m_requiredInteractionTime && !m_sentRequest)
				{
					SSetStateParams params;
					params.m_interactorId = g_pGame->GetIGameFramework()->GetClientActorId();
					params.m_state = (int)m_targetState;

					if (!gEnv->bServer)
					{
						GetGameObject()->InvokeRMI(SvRequestState(), params, eRMI_ToServer);
					}
					else
					{
						SvReceivedStateRequest(params);
					}
					m_sentRequest = true;
				}
			}
			else
			{
				m_interactorId = 0;
				m_interactionTime = 0.f;
			}
		}
	}

	CItem::Update(ctx, slot);
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CBTBBomb, SvRequestState)
{
	SvReceivedStateRequest(params);
	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CBTBBomb, ClSetState)
{
	ClReceivedStateChange(params);

	return true;
}

//------------------------------------------------------------------------
void CBTBBomb::ClReceivedStateChange(const SSetStateParams &params)
{
	m_state = params.m_state;
	// TODO: HUD announcement depending on state?

	m_interactorId = 0;
	m_interactionTime = 0.f;
	m_targetState = eBS_Invalid;

	CGameRules *pGameRules = g_pGame->GetGameRules();

	int localTeamId = pGameRules->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());
	int interactorTeamId = pGameRules->GetTeam(params.m_interactorId);

	if (params.m_state == eBS_Deployed)
	{
		m_deployerId = params.m_interactorId;
		m_deployerTeam = interactorTeamId;
		if (localTeamId == interactorTeamId)
		{
			pGameRules->OnTextMessage(eTextMessageAnnouncement, "$2Bomb deployed");
		}
		else
		{
			pGameRules->OnTextMessage(eTextMessageAnnouncement, "$4Bomb deployed");
		}
		if (!gEnv->bServer && (m_deployedTime == 0.f))
		{
			m_deployedTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		}
	}
	else if (params.m_state == eBS_Defused)
	{
		if (localTeamId == interactorTeamId)
		{
			pGameRules->OnTextMessage(eTextMessageAnnouncement, "$2Bomb defused");
		}
		else
		{
			pGameRules->OnTextMessage(eTextMessageAnnouncement, "$4Bomb defused");
		}
	}
	else if (params.m_state == eBS_Finished)
	{
		if (m_sharedparams->pBTBParams && m_sharedparams->pBTBParams->pExplosionEffect)
		{
			m_sharedparams->pBTBParams->pExplosionEffect->Spawn(true, IParticleEffect::ParticleLoc(GetEntity()->GetWorldPos()));
		}
	}
}

//------------------------------------------------------------------------
void CBTBBomb::SvReceivedStateRequest(const SSetStateParams &params)
{
	bool tellClients = false;
	if (params.m_state == eBS_Carried)		// Requesting to pick up
	{
		if (m_state == eBS_Dropped)
		{
			CryLog("CBTBBomb::SvReceivedStateRequest, Picking up");
			PickUp(params.m_interactorId, true, true, true, NULL);
		}
		else
		{
			CryLog("CBTBBomb::SvReceivedStateRequest, Received request to pick up but we're not dropped, current state='%i'", m_state);
		}
	}
	else if (params.m_state == eBS_Deployed)	// Requesting to deploy
	{
		if (m_state == eBS_Carried)
		{
			CGameRules *pGameRules = g_pGame->GetGameRules();

			CryLog("CBTBBomb::SvReceivedStateRequest, Deploying");
			m_state = (int)eBS_Deployed;
			m_deployedTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();

			tellClients = true;

			m_deployerTeam = pGameRules->GetTeam(params.m_interactorId);
			pGameRules->SetTeam(m_deployerTeam, GetEntityId());

			Drop();
		}
		else
		{
			CryLog("CBTBBomb::SvReceivedStateRequest, Received request to deploy we're not being carried, current state='%i'", m_state);
		}
	}
	else if (params.m_state == eBS_Defused)		// Requesting to defuse
	{
		if (m_state == eBS_Deployed)
		{
			CryLog("CBTBBomb::SvReceivedStateRequest, Defusing");

			tellClients = true;

			m_removeTime = 0.f;
		}
		else
		{
			CryLog("CBTBBomb::SvReceivedStateRequest, Received request to defuse but we're not deployed, current state='%i'", m_state);
		}
	}
	else if (params.m_state == eBS_Finished)
	{
		if (m_state == eBS_Deployed)
		{
			CryLog("CBTBBomb::SvReceivedStateRequest, Finished");

			tellClients = true;

			m_removeTime = 0.f;

			// Score points
			IGameRulesScoringModule *pScoringModule = g_pGame->GetGameRules()->GetScoringModule();
			if (pScoringModule)
			{
				pScoringModule->OnTeamScoringEvent(m_deployerTeam, EGRST_BombTheBaseCompleted);
			}
		}
		else
		{
			CryLog("CBTBBomb::SvReceivedStateRequest, Received request to finish but we're not deployed, current state='%i'", m_state);
		}
	}

	if (tellClients)
	{
		GetGameObject()->InvokeRMI(ClSetState(), params, eRMI_ToRemoteClients);
		if (gEnv->bClient)
		{
			ClReceivedStateChange(params);
		}
	}
}

//-------------------------------------------------------------------------
void CBTBBomb::PickUp(EntityId pickerId, bool sound, bool select, bool keepHistory, const char* setup)
{
	CActor *pActor=GetActor(pickerId);
	if (!pActor)
		return;

	m_state = eBS_Carried;
	m_interactorId = 0;
	m_interactionTime = 0.f;
	m_targetState = eBS_Invalid;

	CItem::PickUp(pickerId, sound, select, keepHistory, setup);

	IEntity *pActorEntity = pActor->GetEntity();
	ICharacterInstance *pCharacter = pActorEntity->GetCharacter(0);
	IAttachmentManager *pIAttachmentManager = pCharacter->GetIAttachmentManager();
	IAttachment *pIAttachment = pIAttachmentManager->GetInterfaceByName(BTBBOMB_ATTACHMENT_NAME);

	CRY_ASSERT_MESSAGE(pIAttachment, "Missing attachment "BTBBOMB_ATTACHMENT_NAME" for BTBBomb");
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
		pItem->SetActionSuffix(BTBBOMB_ACTION_SUFFIX, BTBBOMB_ACTION_SUFFIX);
	}

	pActor->EnableSwitchingItems(false);
	pActor->EnableIronSights(false);
	pActor->EnablePickingUpItems(false);

	EnableUpdate(false, eIUS_Zooming);
	EnableUpdate(false, eIUS_FireMode);
	EnableUpdate(false, eIUS_Scheduler);

	IEntity* pEntity = GetEntity();
	pEntity->Hide(false);
	SetViewMode(eIVM_ThirdPerson);

	pEntity->SetSlotFlags(k_renderSlot, pEntity->GetSlotFlags(k_renderSlot)|ENTITY_SLOT_RENDER_NEAREST);
}

//------------------------------------------------------------------------
void CBTBBomb::ClEnableInputFilter( bool enable )
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

//------------------------------------------------------------------------
bool CBTBBomb::ClCanDeploy()
{
	if(!m_sharedparams->pBTBParams)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "BOMB THE BASE PARAMS: Item of type CBTBBomb is missing it's bomb the base params!");
		return false;
	}

	CActor *pActor=GetOwnerActor();
	if (pActor)
	{
		Vec3 currentPos = pActor->GetEntity()->GetWorldPos();

		SEntityProximityQuery query;
		query.box = AABB( currentPos - Vec3(1.0f, 1.0f, 1.0f), currentPos + Vec3(1.0f, 1.0f, 2.0f) );
		gEnv->pEntitySystem->QueryProximity(query);

		for (int i = 0; i < query.nCount; ++i)
		{
			IEntity* pEntity = query.pEntities[i];
			if (pEntity && (pEntity->GetClass() == m_sharedparams->pBTBParams->pTargetClass))
			{
				// Check team
				CGameRules *pGameRules = g_pGame->GetGameRules();
				int carrierTeamId = pGameRules->GetTeam(pActor->GetEntityId());
				int targetTeamId = pGameRules->GetTeam(pEntity->GetId());

				if (carrierTeamId != targetTeamId)
				{
					return true;
				}
			}
		}
	}
	return false;
}

//------------------------------------------------------------------------
void CBTBBomb::InitClient( int channelId )
{
	CItem::InitClient(channelId);

	SSetStateParams params;
	params.m_state = m_state;
	if (m_state == eBS_Deployed)
	{
		params.m_interactorId = m_deployerId;
	}
	else
	{
		params.m_interactorId = m_interactorId;
	}

	if (params.m_interactorId)
	{
		GetGameObject()->InvokeRMIWithDependentObject(ClSetState(), params, eRMI_ToClientChannel, params.m_interactorId, channelId);
	}
	else
	{
		GetGameObject()->InvokeRMI(ClSetState(), params, eRMI_ToClientChannel, channelId);
	}
	if (m_state == eBS_Deployed)
	{
		GetGameObject()->ChangedNetworkState(BTBBOMB_STATE_ASPECT);
	}
}

//------------------------------------------------------------------------
bool CBTBBomb::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if(!m_sharedparams->pBTBParams)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "BOMB THE BASE PARAMS: Item of type CBTBBomb is missing it's bomb the base params!");
		return false;
	}

	bool result = CItem::NetSerialize(ser, aspect, profile, flags);

	if (aspect == BTBBOMB_STATE_ASPECT)
	{
		float timeToExplode = 0.f;

		if (ser.IsWriting() && (m_state == eBS_Deployed))
		{
			timeToExplode = m_sharedparams->pBTBParams->explodeTime - (gEnv->pTimer->GetFrameStartTime().GetSeconds() - m_deployedTime);
		}

		ser.Value("timeToExplode", timeToExplode, 'fsec');
		
		if (ser.IsReading() && (timeToExplode != 0.f))
		{
			m_deployedTime = (gEnv->pTimer->GetFrameStartTime().GetSeconds() + timeToExplode) - m_sharedparams->pBTBParams->explodeTime;
		}
	}

	return result;
}
