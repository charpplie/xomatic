/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description: Stealth mode implementation
-------------------------------------------------------------------------
History:
- 10-3-2008: Created by Benito G.R.
- 20-10-2009: Renamed to stealth mode

*************************************************************************/

#include "StdAfx.h"
#include "NanoSuit_v2.h"
#include "NanoModeStealth.h"
#include "Player.h"
#include "Audio/GameAudio.h"
#include "Audio/AudioSignalPlayer.h"
#include "WeaponSystem.h"
#include "Weapon.h"
#include "Game.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#include "IAIActor.h"

CNanoModeStealth::CNanoModeStealth(CNanoSuit& nanoSuit)
: m_cloakPulseTimer(0.0f)
, m_isFlickering(false)
, m_flickeringTimer(0.0f)
, m_flickeringTimeOut(-1.0f)
, m_cloakLayerActive(false)
, m_nanoSuit(nanoSuit)
, m_interference(false)
{

}

CNanoModeStealth::~CNanoModeStealth()
{
}

//-----------------------------------
void CNanoModeStealth::Release()
{
	delete this;
}

//-------------------------------------
void CNanoModeStealth::Activate(bool activate)
{
	if(activate)
	{
		CCCPOINT(Nanosuit_StealthMode_On);

		m_nanoSuit.ProcessSuitAction("Activate_Stealth");
		m_cloakPulseTimer=0.0f;
		ActivateSuitPower(m_nanoSuit.IsSuitPowerActive());
	}
	else
	{
		CCCPOINT(Nanosuit_StealthMode_Off);
		SetCloak(false);
	}

}

//----------------------------------
void CNanoModeStealth::Update(float frameTime)
{
	//Trigger malfunction when in critical energy zone (figure out better way to do it)
	const SNanoSuitGameParameters& suitParams = m_nanoSuit.GetGameParams();
	if(suitParams.IsSuitPowerActive())
	{
		if(suitParams.GetState() == eNanoSuitState_Critical || m_interference && !m_isFlickering)
		{
			ActivateCloakFlickering(true);
		}
		else if(suitParams.GetState() == eNanoSuitState_Normal && !m_interference && m_isFlickering && m_flickeringTimeOut < 0.0f)
		{
			ActivateCloakFlickering(false);
			if(!m_cloakLayerActive)
				SetCloakLayer(true);
			m_cloakPulseTimer= 0.0f;
		}

		//Force cloak if we changed state
		if(!suitParams.IsCloakEnabled())
			SetCloak(true);

		if (!m_isFlickering)
		{
			m_cloakPulseTimer-=frameTime;
			if (m_cloakPulseTimer<=0.0f)
			{
				m_cloakPulseTimer+=0.5f;

				if (IAIObject *pAIObject = m_nanoSuit.GetOwner().GetEntity()->GetAI())
					if (IAISignalExtraData* pData = gEnv->pAISystem->CreateSignalExtraData())
					{
						pData->point=pAIObject->GetPos();

						// Marcio: This signal filter uses the commrange entity property
						// Should create a new one that allows a specific range to be set
						gEnv->pAISystem->SendSignal(SIGNALFILTER_ANYONEINCOMM, 1, "OnCloakPulse", pAIObject, pData);
					}
			}
		}
		else
		{
			UpdateCloakFlickering(frameTime);
		}
	}
}

//-------------------------------------
void CNanoModeStealth::ProcessEvent(const SNanoSuitEvent& event)
{
	const SNanoSuitGameParameters& suitParams = m_nanoSuit.GetGameParams();

	switch(event.event)
	{
		case eNanoSuitEvent_SHOT:
		{
			if(suitParams.IsSuitPowerActive())
			{
				m_nanoSuit.ProcessSuitAction("Shot_Cloaked");
				//ActivateCloakFlickering(true, 1.0f);
			}
		}
		break;

		case eNanoSuitEvent_HIT:
			if(suitParams.IsSuitPowerActive())
			{
				CRY_ASSERT(event.sParam);

				bool isFallHit = (strcmp(event.sParam, "fall") == 0);
				m_nanoSuit.ProcessSuitAction(isFallHit ? "FallHit_Cloaked" : "Hit_Cloaked");

				//ActivateCloakFlickering(true, 1.0f);
			}
			break;

		case eNanoSuitEvent_MELEE:
			{
				if(suitParams.IsSuitPowerActive())
				{
					m_nanoSuit.ProcessSuitAction("Melee_Cloaked");
					//ActivateCloakFlickering(true, 1.0f);
				}
				else
				{
					m_nanoSuit.ProcessSuitAction("Melee_Stealth");
				}
			}
			break;

		case eNanoSuitEvent_MELEE_HIT:
			{
				m_nanoSuit.ProcessSuitAction("Melee_Hit");
			}
			break;

		case eNanoSuitEvent_CLOAK:
			SetCloak(true);
			break;
		case eNanoSuitEvent_UNCLOAK:
			SetCloak(false);
			break;

		case eNanoSuitEvent_ONAFTER_MODE_ACTIVATION:
			{
				m_nanoSuit.ProcessSuitAction("AfterActivate_Stealth");
			}			
			break;

		case eNanoSuitEvent_STEALTH_KILL:
			{
				if (suitParams.IsSuitPowerActive())
					m_nanoSuit.ProcessSuitAction("StealthKill_Cloaked");
			}
			break;

		case eNanoSuitEvent_THROW:
			{
				if (m_nanoSuit.IsSuitPowerActive())
				{
					m_nanoSuit.ProcessSuitAction("Throw_Cloaked");
				}
			}
			break;

		default:
			break;
	}
}

//-------------------------------------------
void CNanoModeStealth::OnStateChanged(ENanoSuitState newState)
{
	if (newState == eNanoSuitState_Disabled)
	{
		SetCloak(false);
	}
}

//---------------------------------------------
void CNanoModeStealth::ActivateSuitPower(bool activate)
{
	if (!activate)
	{
		m_nanoSuit.m_suitGameParams.GetEnergy().RestoreToDefaultModifiers();
		m_nanoSuit.m_suitGameParams.ApplyGameParametersMods(m_nanoSuit.s_nanoSuitParams.GetModePassiveProperties(GetType()));		
		SetCloak(false);
	}
	else
	{
		m_nanoSuit.m_suitGameParams.ApplyGameParametersMods(m_nanoSuit.s_nanoSuitParams.GetModeActiveProperties(GetType()));

		SNanoSuitEnergyParams energyParams = m_nanoSuit.s_nanoSuitParams.GetModeEnergyMod(GetType());

		CPlayer& player = m_nanoSuit.GetOwner();
		player.SendPerkEvent(EPE_OverrideStealthEnergyParams, &energyParams);
		m_nanoSuit.m_suitGameParams.GetEnergy().PatchModeModifiers(energyParams);
		SetCloak(true);
	}
}

//-----------------------------------
bool CNanoModeStealth::CanActivateSuitPower() const
{
	return true;
}

//----------------------------------------------------------
void CNanoModeStealth::ActivateCloakFlickering(bool activate, float timeOut /* = -1.0f */)
{
	m_nanoSuit.ActivateSuitState("Cloak_Malfunction", activate, 0.0f);

	m_isFlickering = activate;
	m_flickeringTimer = 4.0f;
	m_flickeringTimeOut = activate ? timeOut : -1.0f;

	if (IAIObject *pAIObject = m_nanoSuit.GetOwner().GetEntity()->GetAI())
	{
		if (IAIActor *pAIActor=pAIObject->CastToIAIActor())
		{
			AgentParameters ap(pAIActor->GetParameters());
			ap.m_fCloakScaleTarget=activate?0.5f:1.0f;
			pAIActor->SetParameters(ap);
		}

	}
}

//-----------------------------------------------------------------
void	CNanoModeStealth::UpdateCloakFlickering(float frameTime)
{
	//If there is a time out set...
	if(m_flickeringTimeOut > 0.0f)
	{
		m_flickeringTimeOut -= frameTime;
		if(m_flickeringTimeOut <= 0.0f)
		{
			ActivateCloakFlickering(false);
			SetCloakLayer(true); //Restore layer just in case
			return;
		}
	}

	//Switch cloak layer every half a second
	m_flickeringTimer += frameTime;
	const float switchDelay = m_cloakLayerActive ? Random(2.0f,4.0f) : 0.4f; 
	if(m_flickeringTimer > switchDelay)
	{
		m_flickeringTimer = 0.0f;
		SetCloakLayer(!m_cloakLayerActive);
	}

}

//--------------------------------------------------------------------
void CNanoModeStealth::SetCloak(bool set)
{
	if ((m_nanoSuit.m_suitGameParams.IsCloakEnabled() == set) && !m_nanoSuit.IsReseting())
		return;

	m_nanoSuit.m_suitGameParams.SetCloaked(set);

	CPlayer& suitOwner = m_nanoSuit.GetOwner();

	if (set)
	{
		m_nanoSuit.ActivateSuitState("Cloak", true, 0.0f);

		suitOwner.SendMusicLogicEvent(eMUSICLOGICEVENT_CLOAKMODE_ENTER);
		IActor *pClient=gEnv->pGame->GetIGameFramework()->GetClientActor();
		if(pClient)
		{
			CPlayer *pPlayer = static_cast<CPlayer*>(pClient);
			EntityId ownerId = suitOwner.GetEntityId();
			pPlayer->SendPerkEvent(EPE_PlayerCloaking, &ownerId);
		}
	}
	else
	{
		m_nanoSuit.ActivateSuitState("Cloak", false, 0.0f);

		suitOwner.SendMusicLogicEvent(eMUSICLOGICEVENT_CLOAKMODE_LEAVE);

		ActivateCloakFlickering(false);
	}

	SetCloakLayer(set);
	
	suitOwner.CreateScriptEvent("cloaking", set ? 1.0f : 0.0f);

	// player's squadmates mimicking nanosuit modifications
	IEntity *pOwner = m_nanoSuit.GetOwner().GetEntity();
	assert(pOwner);
	if (pOwner->GetAI())
	{
		gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER,1, (set?"OnNanoSuitCloak":"OnNanoSuitUnCloak"),pOwner->GetAI());

		if(set)
			pOwner->GetAI()->Event(AIEVENT_PLAYER_STUNT_CLOAK, 0);
		else
			pOwner->GetAI()->Event(AIEVENT_PLAYER_STUNT_UNCLOAK, 0);
	}

	bool enable = set;
	suitOwner.SendPerkEvent(EPE_SetCloak, &set);
}

//-----------------------------------
void CNanoModeStealth::SetCloakLayer(bool set)
{
	m_cloakLayerActive = set;

	IEntity *pOwnerEntity = m_nanoSuit.GetOwner().GetEntity();

	// new cloak effect
	IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy*)pOwnerEntity->GetProxy(ENTITY_PROXY_RENDER);
	uint8 mask = pRenderProxy->GetMaterialLayersMask();
	uint32 blend = pRenderProxy->GetMaterialLayersBlend();
	pRenderProxy->SetMaterialLayersMask(set?mask|MTL_LAYER_CLOAK:mask&~MTL_LAYER_CLOAK);
	pRenderProxy->SetMaterialLayersBlend(blend&0xffffff00);


	if (CItem* pItem = static_cast<CItem*>(m_nanoSuit.GetOwner().GetCurrentItem(true)))
		pItem->CloakSync(true);

	// take care of the attachments on the back
	if (ICharacterInstance *pOwnerCharacter = pOwnerEntity->GetCharacter(0))
	{
		if (IAttachmentManager *pAttachmentManager = pOwnerCharacter->GetIAttachmentManager())
		{
			int32 count = pAttachmentManager->GetAttachmentCount();
			for (uint32 i=0; i<count; ++i)
			{
				if (IAttachment* pAttachment = pAttachmentManager->GetInterfaceByIndex(i))
				{
					if(IAttachmentObject *pAO = pAttachment->GetIAttachmentObject())
					{
						if(pAO->GetAttachmentType()==IAttachmentObject::eAttachment_Entity)
						{
							CEntityAttachment* pEA = static_cast<CEntityAttachment*>(pAO);
							if (CItem* pItem = static_cast<CItem*>(gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(pEA->GetEntityId())))
								pItem->CloakSync(true);
						}
					}
				}
			}
		}
	}
}

//-------------------------------------
ENanoSuitMode CNanoModeStealth::GetType() const
{
	return eNanoSuitMode_Stealth;
}

//-------------------------------------
void CNanoModeStealth::GetMemoryUsage(ICrySizer* s) const
{
	s->Add(*this);
}
