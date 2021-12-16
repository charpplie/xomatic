#include "StdAfx.h"
#include "SelfDestructPerk.h"

#include "GameCVars.h"
#include "SShootHelper.h"
#include "Player.h"
#include "PlayerInput.h"
#include "PerkIconData.h"
#include "HUD/UI/UIButtonPromptRegion.h"
#include "Audio/AudioSignalPlayer.h"
#include "GameActions.h"
#include "GameRules.h"
#include "GameRulesModules/IGameRulesAssistScoringModule.h"

SelfDestructPerk::SelfDestructPerk()
{
	m_minigame = false;
	m_explode = false;
	m_timer = 0.0f;
	m_power = 0.0f;
	m_warningSignal.SetSignal("SelfDestructWarning");
	m_killerId = 0;
}

void SelfDestructPerk::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
		case EPE_Die:
		{
			EnableInputs(false);
			break;
		}
		case EPE_Spawn:
		{
			m_explode = false;
			
			m_timer = 0.0f;
			m_power = 0.0f;
			if(gEnv->bServer)
			{
				m_minigame = false;
				CHANGED_NETWORK_STATE(m_ownerPlayer, CPlayer::ASPECT_PERK_SELFDESTRUCT_SERVER);
			}
			break;
		}
		case EPE_OverrideHealth:
		{
			int* pHealth = (int*) data;
			int health = *pHealth;
			const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
			//can only be done on server and client doesn't have quiet the same health (can be different by 1000 :o
			if(health <=0 && health > -perkVars->perk_selfDestruct_StartHealth && !m_minigame && InRequiredSuitMode() && gEnv->bServer)
			{
				*pHealth = perkVars->perk_selfDestruct_StartHealth;
				m_minigame = true;
				StartMinigame();
				CGameRules *pGameRules = g_pGame->GetGameRules();
				IGameRulesAssistScoringModule *assistScoringModule = pGameRules ? pGameRules->GetAssistScoringModule() : NULL;
				m_killerId = assistScoringModule->SvGetMostRecentAttacker(m_ownerPlayer->GetEntityId());
				CHANGED_NETWORK_STATE(m_ownerPlayer, CPlayer::ASPECT_PERK_SELFDESTRUCT_SERVER);
			}
			break;
		}
		case EPE_SelfDestructExplode:
		{
			if(m_minigame && !m_explode && m_power > 0.0f && m_ownerPlayer->GetHealth() > 0)
			{
				SelfDestructExplode();
				CHANGED_NETWORK_STATE(m_ownerPlayer, CPlayer::ASPECT_PERK_SELFDESTRUCT_CLIENT);
			}
			break;
		}
		default:
		{
			IPerk::HandleEvent (perkEvent, data);
			break;
		}
	}
}

const void* SelfDestructPerk::GetData(EPlayerPlugInData dataType)
{
	switch(dataType)
	{
	case EPD_SelfDestructMinigame:
		{
			return &m_minigame;
		}
	case EPD_SelfDestructKiller:
		{
			return &m_killerId;
		}
	}

	return NULL;
}

void SelfDestructPerk::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if(aspect == CPlayer::ASPECT_PERK_SELFDESTRUCT_SERVER)
	{
		bool previousMinigame = m_minigame;
		ser.Value("minigame", m_minigame, 'bool');
		ser.Value("killerId", m_killerId, 'eid');
		if(ser.IsReading() && !previousMinigame && m_minigame)
		{
			StartMinigame();
		}
	}
	if(aspect == CPlayer::ASPECT_PERK_SELFDESTRUCT_CLIENT)
	{
		ser.Value("finalCountdownPower", m_power);
		bool previousExplode = m_explode;
		ser.Value("finalCountdownExplode", m_explode, 'bool');
		if(ser.IsReading() && !previousExplode && m_explode && !m_ownerPlayer->IsClient())
		{
			SelfDestructExplode();
		}
	}
}

void SelfDestructPerk::Update(const float dt)
{
	CPerk * perkInstance = CPerk::GetInstance();
	const CPerk::SPerkVars * perkVars = perkInstance->GetVars();

	int health = m_ownerPlayer->GetHealth();
	bool client = m_ownerPlayer->IsClient();

	if(health > 0)
	{
		if (m_minigame && client)
		{
			m_timer += dt;
			//if we have exploded or ran out of time kill the player
			if(gEnv->bServer && m_timer >= perkVars->perk_SelfDestruct_maxTime)
			{
				m_ownerPlayer->PerkRequestDamage(ePerk_SelfDestruct, 100.0f);
			}
		}
	}
	else
	{
		m_warningSignal.Stop(m_ownerPlayer->GetEntityId());
	}

	CPerkIconData * localPerkIcons = CPerkIconData::GetForEntity(m_ownerPlayer->GetEntityId());

	if (localPerkIcons)
	{
		if (health > 0)
		{
			if(m_minigame)
			{
				if(m_timer > perkVars->perk_SelfDestruct_minTime)
				{
					//random mini game to scale the explosion size
					if(!m_explode)
					{
						if(m_timer < perkVars->perk_SelfDestruct_maxTime)
						{
							m_power = perkVars->perk_SelfDestruct_minPower +
								(clamp(((1.0f + cosf(m_timer * perkVars->perk_SelfDestruct_barSpeed)) * 0.5f), 0.0f, 1.0f)
								* (1.0f - perkVars->perk_SelfDestruct_minPower));

							CControllerInputRenderInfo promptInfo;
							promptInfo.CreateForInput("crysis2_common", "use");
							CUIButtonPromptRegion::SetOnScreenMessageText("buttonPrompts", "@ui_prompt_selfDestruct", & promptInfo);
						}

						DrawDebugHUD();
					}
				}
				else
				{
					CryFixedStringT<64> nameInColour;
					nameInColour.Format("$8%s$o", perkInstance->GetPerkData(m_perkId)->GetShowOnScreenName().c_str());
					CUIButtonPromptRegion::SetOnScreenMessageText("buttonPrompts", "@ui_prompt_waitForSelfDestructToActivate", nameInColour, NULL, NULL);
				}

				// Minigame is active, set drain amount to fraction of time left
				localPerkIcons->SetIconDrainAmount(m_perkId, min(1.f, m_timer / perkVars->perk_SelfDestruct_maxTime));
			}
			else
			{
				// Minigame is not active but player is alive, set not-at-all drained
				localPerkIcons->SetIconDrainAmount(m_perkId, 0.f);
			}
		}
		else
		{
			// Player is dead, set icon to completely drained
			localPerkIcons->SetIconDrainAmount(m_perkId, 1.f);
		}
	}
}

void SelfDestructPerk::DrawDebugHUD()
{
	IRenderAuxGeom* rag = gEnv->pRenderer->GetIRenderAuxGeom();
	SAuxGeomRenderFlags flags = rag->GetRenderFlags();
	flags.SetMode2D3DFlag( e_Mode2D );
	rag->SetRenderFlags(flags);

	float w = 0.3f;
	float x1 = (1.0f - w) * 0.5f;
	float y1 = 0.65f, y2 = 0.68f;

	for(float f = 0.0f; f <= m_power + 0.05f; f+= 0.1f)
	{
		float lw = f * w;
		float lh = (f * f) * 0.15f;
		ColorB color((uint8) (255.0f * f), 0, 0, 255);
		rag->DrawLine( Vec3(x1+lw, y1-lh, 0), color, Vec3(x1+lw, y2, 0), color, 4.0f );
	}
}

void SelfDestructPerk::SelfDestructExplode()
{
	m_explode = true;

	EnableInputs(false);

	Vec3 explosionPos = m_ownerPlayer->GetEntity()->GetPos();
	explosionPos.z += 0.5f;	//need to raise up the position of the explosion so it isn't obstructed by the physics
	SShootHelper::Explode(m_ownerPlayer->GetEntityId(), "SelfDestructPerk", explosionPos, m_ownerPlayer->GetEntity()->GetForwardDir(), (int) (m_power * CPerk::GetInstance()->GetVars()->perk_SelfDestruct_damage));
	m_ownerPlayer->PerkRequestDamage(ePerk_SelfDestruct, 100.0f);

	m_warningSignal.Stop(m_ownerPlayer->GetEntityId());
}

void SelfDestructPerk::StartMinigame()
{
	if(gEnv->bServer)
	{
		m_ownerPlayer->DropItem(m_ownerPlayer->GetCurrentItemId());
	}

	m_ownerPlayer->GetAnimationGraphState()->SetInput("Action", "selfDestruct");

	m_ownerPlayer->HolsterItem(true);
	m_ownerPlayer->SetActorState(CActor::kActorState_incapacitated);

	m_warningSignal.Play(m_ownerPlayer->GetEntityId());

	EnableInputs(true);
}

void SelfDestructPerk::EnableInputs(bool enable)
{
	if(m_ownerPlayer->IsClient())
	{
		g_pGameActions->FilterUseKeyOnly()->Enable(enable);
		if(enable)
		{
			// If the player is already moving then we need to clear the inputs
			CPlayer *pPlayer = static_cast<CPlayer *>(g_pGame->GetIGameFramework()->GetClientActor());
			if(pPlayer)
			{
				IPlayerInput* pInput = pPlayer->GetPlayerInput();
				if(pInput && (pInput->GetType() == IPlayerInput::PLAYER_INPUT))
				{
					CPlayerInput *pPlayerInput = static_cast<CPlayerInput *>(pInput);
					pPlayerInput->ClearAllExceptAction(0);
				}
			}
		}
	}
}
