#include "StdAfx.h"
#include "EMPStrikePerk.h"

#include "SShootHelper.h"
#include "Player.h"
#include "SatelliteStrike.h"
#include "HUD/HUD_Impl.h"
#include "PerkIconData.h"
#include "Audio/AudioSignalPlayer.h"
#include "PerkDbgDisplay.h"
#include "Game.h"
#include "GameActions.h"
#include "PlayerProgression.h"
#include "HUD/HUD.h"

static const char* STRIKE_GEOMETRY = "Objects/effects/satellite_target/cw2_satellite_target.cgf";
static AUTOENUM_BUILDNAMEARRAY(s_strikeStateNames, StrikeStateList);

enum EPointerSlot
{
	EPS_geometry = 0,
	EPS_particle,
};

EMPStrikePerk::EMPStrikePerk() 
{ 
	m_mostRecentStrikePos.Set(0.0f, 0.0f, 0.0f);
	m_color = ColorF(0.0f, 0.0f, 1.0f, 0.25f); 
	m_pointerEntityId = 0;
	m_targetingRingsEffect = gEnv->pParticleManager->FindEffect("perk_fx.satalite_strike.targeting_rings");
	m_announcement = "EMPStrike";
	m_audioSignal = "UnlockEMPStrike";
	m_numTimesFired = 0;
	m_state = ESPS_uninit;
	m_everRead = false;
}

void EMPStrikePerk::Enter()
{
	CAnnouncer::GetInstance()->Announce(m_ownerPlayer->GetEntityId(), m_announcement);

	IPerk::Enter();

	if(m_ownerPlayer->IsClient())
	{
		CAudioSignalPlayer::JustPlay(m_audioSignal, m_ownerPlayer->GetEntityId());
		InitTargetingPointer();
	}
}

void EMPStrikePerk::InitTargetingPointer()
{
	const Vec3 & createPos = m_ownerPlayer->GetEntity()->GetPos();

	SEntitySpawnParams entitySpawnParams;
	entitySpawnParams.nFlags = ENTITY_FLAG_SPAWNED|ENTITY_FLAG_CLIENT_ONLY;
	entitySpawnParams.sName = "Orbital Strike Pointer";
	entitySpawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();
	entitySpawnParams.vPosition = createPos;

	IEntity * newEntity = gEnv->pEntitySystem->SpawnEntity(entitySpawnParams);
	PlayerPluginAssert(newEntity, "Unable to spawn orbital strike pointer entity");

	int slot = newEntity->LoadGeometry(EPS_geometry,STRIKE_GEOMETRY);
	PlayerPluginAssert(slot != -1, "Unable to load strike geometry %s", STRIKE_GEOMETRY);

	m_pointerEntityId = newEntity->GetId();
	
	if(m_targetingRingsEffect)
	{
		int slot = newEntity->LoadParticleEmitter(EPS_particle, m_targetingRingsEffect);
		PlayerPluginAssert(slot != -1, "Failed to load strike particle effect %s", m_targetingRingsEffect->GetName());
	}
	newEntity->Hide(true);
}

void EMPStrikePerk::Leave()
{
	IPerk::Leave();

	if(m_pointerEntityId)
	{
		gEnv->pEntitySystem->RemoveEntity(m_pointerEntityId);
		m_pointerEntityId = 0;
	}

	if (m_ownerPlayer->IsClient())
	{
		SetState(ESPS_uninit);
	}
}

void EMPStrikePerk::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
		case EPE_Reset:
		case EPE_Spawn:
		case EPE_Die:
		if (m_ownerPlayer->IsClient())
		{
			PlayerPluginAssert (perkEvent != EPE_Reset || m_state == ESPS_uninit, "Received EPE_Reset but state isn't ESPS_uninit it's %d", m_state);
			SetState(ESPS_none);
		}
		break;

	case EPE_Pointer:
		{
			PlayerPluginAssert(m_ownerPlayer->IsClient(), "EPE_Pointer should only be received by client!");
			if(m_ownerPlayer->GetHealth() > 0)
			{
				SOnActionData* actionData = static_cast<SOnActionData*>(data);
				if(actionData->activationMode == eAAM_OnPress)
				{
					// Toggle between 'pointer' and 'none' states
					SetState((m_state == ESPS_none) ? ESPS_pointer : ESPS_none);
				}
			}
			break;
		}
	case EPE_OrbitalStrikeFromMap:
		{
			PlayerPluginAssert(m_ownerPlayer->IsClient(), "EPE_OrbitalStrikeFromMap should only be received by client!");
			Vec3* pos = static_cast<Vec3*>(data);
			ActivateStrike(pos);

			SHUDEvent event;
			event.eventType = eHUDEvent_HideSelectMenu;
			CHUD::CallEvent(event);
			break;
		}
	case EPE_OrbitalStrike:
		{
			PlayerPluginAssert(m_ownerPlayer->IsClient(), "EPE_OrbitalStrike should only be received by client!");
			SOnActionData* actionData = static_cast<SOnActionData*>(data);
			if(actionData->activationMode == eAAM_OnRelease && m_state == ESPS_pointer)
			{
				ActivateStrikeFromPointer();
				actionData->handled = true;
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

void EMPStrikePerk::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if(aspect == CPlayer::ASPECT_PERK_STRIKES_CLIENT)
	{
		uint8 newNumTimesFired = m_numTimesFired;

		ser.Value("strikePos", m_mostRecentStrikePos);
		ser.Value("numTimesFired", newNumTimesFired, 'ui2');

		if (ser.IsReading())
		{
			if (m_everRead == false)
			{
				PlayerPluginLog ("Initial num-times-struck value is %d", newNumTimesFired);
				m_numTimesFired = newNumTimesFired;
				m_everRead = true;
			}
			else
			{
				if (m_numTimesFired != newNumTimesFired)
				{
					PlayerPluginLog ("Struck %d times, net serialize says %d times... firing now!", m_numTimesFired, newNumTimesFired);
					DoStrike(m_mostRecentStrikePos);
					m_numTimesFired = newNumTimesFired;
				}
			}
		}
	}
}

void EMPStrikePerk::SetState(ESP_State newState)
{
	bool isSet = m_ownerPlayer->IsPerkSet(m_perkId);
	PlayerPluginAssert(m_ownerPlayer->IsClient(), "Function should only be called for client!");
	PlayerPluginAssert(isSet == (newState != ESPS_uninit), "Shouldn't change state to %s while perk %s set!", s_strikeStateNames[newState], isSet ? "is" : "is not");

	if (m_state != newState)
	{
		PlayerPluginLog("Changing from %s to %s", s_strikeStateNames[m_state], s_strikeStateNames[newState]);

		CPerkIconData * iconData = CPerkIconData::GetForEntity(m_ownerPlayer->GetEntityId());
		assert (iconData);

		if (isSet)
		{
			CControllerInputRenderInfo * myPrompt = iconData->GetIconInputPrompt(m_perkId);
			if (newState == ESPS_none)
			{
				myPrompt->CreateForInput("crysis2_common", "binoculars");
			}
			else if (m_state == ESPS_none)
			{
				myPrompt->Clear();
			}
		}

		if(newState == ESPS_pointer)
		{
			IActionFilter* strikeFilter = g_pGame->Actions().FilterStrikePointer();

			PlayerPluginAssert(! strikeFilter->Enabled(), "Input filter is already activated!");
			strikeFilter->Enable(true);
			PlayerPluginAssert(strikeFilter->Enabled(), "Input filter failed to activate!");
		}
		else if(m_state == ESPS_pointer)
		{
			IActionFilter* strikeFilter = g_pGame->Actions().FilterStrikePointer();

			PlayerPluginAssert(strikeFilter->Enabled(), "Input filter is already deactivated!");
			strikeFilter->Enable(false);
			PlayerPluginAssert(! strikeFilter->Enabled(), "Input filter failed to deactivate!");
		}

		m_state = newState;
	}
}

void EMPStrikePerk::Update(const float dt)
{
	PlayerPluginWatch("State %s, counter = %d, ever read = %d", s_strikeStateNames[m_state], m_numTimesFired, m_everRead);

	if (m_ownerPlayer->IsClient())
	{
		switch (m_state)
		{
			case ESPS_pointer:
			{
				UpdatePointer();
				break;
			}
			default:
			{
				// If not aiming, then hide targeting pointer
				IEntity *pointerEntity = gEnv->pEntitySystem->GetEntity(m_pointerEntityId);
				if(pointerEntity)
				{
					pointerEntity->Hide(true);
				}
				break;
			}
		}
	}
}

void EMPStrikePerk::UpdatePointer()
{
	PlayerPluginAssert(m_ownerPlayer->IsClient(), "Function should only be called for client!");

	const float k_ScreenFractionX = 0.5f;
	const float k_ScreenFractionY = 0.65f;
	float fontColor[4] = { 1.0f, 1.0f, 1.0f, 0.85f };

	Vec3 hitPos;

	IEntity *pointerEntity = gEnv->pEntitySystem->GetEntity(m_pointerEntityId);
	assert (pointerEntity);

	if (pointerEntity)
	{
		if(GetStrikePointerPos(hitPos))
		{
			// Show targeting pointer
			pointerEntity->SetPos(hitPos);
			pointerEntity->Hide(false);
		}
		else
		{
			// Hide pointer if not able to fire
			pointerEntity->Hide(true);

			// Inform player about unable to fire pointer 
			gEnv->pRenderer->Draw2dLabel(gEnv->pRenderer->GetWidth()*k_ScreenFractionX, gEnv->pRenderer->GetHeight()*k_ScreenFractionY, 2, fontColor, true, "Failing to acquire target - Press Y to cancel deployment");
		}
	}
}

bool EMPStrikePerk::GetStrikePointerPos(Vec3 &outPos)
{
	PlayerPluginAssert(m_ownerPlayer->IsClient(), "Function should only be called for client!");

	SMovementState ms;
	m_ownerPlayer->GetMovementController()->GetMovementState(ms);
	Vec3 origin = ms.eyePosition;
	Vec3 dir = ms.eyeDirection;

	ray_hit hit;
	gEnv->pPhysicalWorld->RayWorldIntersection(origin, dir * 500.0f, ent_terrain|ent_static|ent_sleeping_rigid|ent_rigid, rwi_stop_at_pierceable|rwi_colltype_any, &hit, 1);

	outPos = hit.pt;
	return hit.dist > 0.0f;
}

void EMPStrikePerk::ActivateStrikeFromPointer()
{
	PlayerPluginAssert(m_ownerPlayer->IsClient(), "Function should only be called for client!");

	Vec3 targetPos;
	if(GetStrikePointerPos(targetPos))
	{
		ActivateStrike(&targetPos);
	}
}

void EMPStrikePerk::ActivateStrike(Vec3 *pos)
{
	PlayerPluginAssert(m_ownerPlayer->IsClient(), "Function should only be called for client!");

	pos->z = CPerk::GetInstance()->GetVars()->perk_empSatelliteStrikeHeight;
	
	DoStrike(*pos);

	m_mostRecentStrikePos.Set(pos->x, pos->y, pos->z);
	SetState(ESPS_strike);
	CHANGED_NETWORK_STATE(m_ownerPlayer, CPlayer::ASPECT_PERK_STRIKES_CLIENT);

	// NB: This will remove the perk icon!
	m_ownerPlayer->SetPerkActive(m_perkId, false);
}

void EMPStrikePerk::DoStrike(Vec3 firePos)
{
	FireStrike(firePos);

	// Increase m_numTimesFired by 1: m_numTimesFired is net serialized
	// NB: This number wraps around to 0 quite quickly (after only a few strikes) to reduce bandwidth
	m_numTimesFired ++;
	m_numTimesFired &= 3;
}

// Overloaded in SatStrikePerk.h
void EMPStrikePerk::FireStrike(Vec3 firePos)
{
	Vec3 dir = Vec3(0.0f, 0.0f, -1.0f);
	SShootHelper::Shoot(m_ownerPlayer->GetEntityId(), "empsatstrike", "emp", firePos, dir, 0);
	if(m_ownerPlayer->IsClient())
	{
		CPlayerProgression::GetInstance()->Event(EPP_EMPStrike);
	}
}

