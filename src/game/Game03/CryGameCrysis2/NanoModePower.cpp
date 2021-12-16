/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description: Power mode implementation
-------------------------------------------------------------------------
History:
- 4-3-2008: Created by Benito G.R.
- 20-10-2009: Renamed to power mode

*************************************************************************/

#include "StdAfx.h"
#include "NanoSuit_v2.h"
#include "NanoModePower.h"
#include "Player.h"
#include "ScreenEffects.h"
#include "GameCVars.h"
#include "Game.h"
#include "Audio/AudioSignalPlayer.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

CNanoModePower::CNanoModePower(CNanoSuit& nanoSuit)
	: m_sprintingActive(false)
	, m_sprintWasActivatedSinceModeChange( false )
	, m_nanoSuit(nanoSuit)
{

}

CNanoModePower::~CNanoModePower()
{
}


//-----------------------------------
void CNanoModePower::Release()
{
	delete this;
}

//-------------------------------------
void CNanoModePower::Activate(bool activate)
{
	//This is currently being called for all clients on all clients in a network game
	if(activate)
	{
		CCCPOINT(Nanosuit_PowerMode_On);

		m_nanoSuit.ProcessSuitAction("Activate_Power");
		m_sprintWasActivatedSinceModeChange = false;

		ActivateSuitPower(m_nanoSuit.IsSuitPowerActive());
	}
	else
	{
		CCCPOINT(Nanosuit_PowerMode_Off);

		ActivateSprinting(false);

/*
		if(m_nanoSuit.GetOwner().GetEntityId()==gEnv->pGame->GetIGameFramework()->GetClientActorId())
		{
			//Prevent everyones screens from distorting when one player switches from maneuvre->any other mode
			g_pGame->GetScreenEffects()->ProcessSprintingFX(false, false);
		}*/

		m_nanoSuit.ActivateSuitState("PowerModeON", false);
	}
}

//-----------------------------------------------------------------
void CNanoModePower::Update(float frameTime)
{
	const SPlayerStats* pStats = static_cast<const SPlayerStats*>(m_nanoSuit.GetOwner().GetActorStats());

	if(pStats->bSprinting && !m_sprintingActive)
	{
		ActivateSprinting(true);
	}
	else if(!pStats->bSprinting && m_sprintingActive)
	{
		ActivateSprinting(false);
	}
}

//-------------------------------------
void CNanoModePower::ProcessEvent(const SNanoSuitEvent& event)
{
	switch(event.event)
	{
		case eNanoSuitEvent_POWERJUMP:
			{
				if(event.bParam)
				{
					m_nanoSuit.ProcessSuitAction("PowerJumpPerk");
				}
				else
				{
					m_nanoSuit.ProcessSuitAction("PowerJump");
				}
			}
			break;

		case eNanoSuitEvent_KVOLT_CHARGE:
		case eNanoSuitEvent_DISABLE_SUIT:
			{
				ActivateSprinting(false);
			}

		case eNanoSuitEvent_MELEE:
			{
				bool powerActive = m_nanoSuit.IsSuitPowerActive();
				m_nanoSuit.ProcessSuitAction(powerActive ? "Melee_SuperPower" : "Melee_Power");
			}
			break;
		case eNanoSuitEvent_MELEE_HIT:
			{
				bool powerActive = m_nanoSuit.IsSuitPowerActive();
				m_nanoSuit.ProcessSuitAction(powerActive ? "Melee_Hit_SuperPower" : "Melee_Hit");
			}
			break;
			
		case eNanoSuitEvent_ONAFTER_MODE_ACTIVATION:
			{
				m_nanoSuit.ProcessSuitAction("AfterActivate_Power");
			}			
			break;

		case eNanoSuitEvent_THROW:
			{
				if (m_nanoSuit.IsSuitPowerActive())
				{
					m_nanoSuit.ProcessSuitAction("Throw_Power");
				}
			}
			break;

		default:
			break;
	}
}

//-------------------------------------------
void CNanoModePower::OnStateChanged(ENanoSuitState newState)
{

}

//---------------------------------------------
void CNanoModePower::ActivateSuitPower(bool activate)
{
	if (!activate)
	{
		m_nanoSuit.m_suitGameParams.GetEnergy().RestoreToDefaultModifiers();
		m_nanoSuit.m_suitGameParams.ApplyGameParametersMods(m_nanoSuit.s_nanoSuitParams.GetModePassiveProperties(GetType()));
		if (m_sprintingActive)
			ActivateSprinting( false );

		if (m_nanoSuit.GetOwner().IsClient() && m_nanoSuit.m_suitShapeDeformation.IsAnyTransitionEnabled())
		{
			m_nanoSuit.m_suitShapeDeformation.ExecuteShapeTransition(CNanoSuitShapeDeformation::eST_SuitModeDeactivate, 0.5f);
		}
	}
	else
	{
		m_nanoSuit.m_suitGameParams.ApplyGameParametersMods(m_nanoSuit.s_nanoSuitParams.GetModeActiveProperties(GetType()));

		SNanoSuitEnergyParams energyParams = m_nanoSuit.s_nanoSuitParams.GetModeEnergyMod(GetType());

		CPlayer& player = m_nanoSuit.GetOwner();
		if(player.IsPerkSet(ePerk_ManeuverMKII))
		{
			const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
			energyParams.consumptionRateMin *= perkVars->perk_ManeuverMKII_scaleEnergyConsumption;
			energyParams.consumptionRateMax *= perkVars->perk_ManeuverMKII_scaleEnergyConsumption;

			CAudioSignalPlayer::JustPlay("Perk_Maneuver2", player.GetEntityId());
		}

		m_nanoSuit.m_suitGameParams.GetEnergy().PatchModeModifiers(energyParams);

		if (m_nanoSuit.GetOwner().IsClient())
		{
			m_nanoSuit.m_suitShapeDeformation.ExecuteShapeTransition(CNanoSuitShapeDeformation::eST_SuitModeActivatePower, 0.5f);
		}
	}

	m_nanoSuit.ActivateSuitState("PowerModeON", activate);
}

//-----------------------------------
bool CNanoModePower::CanActivateSuitPower() const
{
	return true;
}

//-------------------------------------
void CNanoModePower::ActivateSprinting(bool activate)
{
	CPlayer& suitOwner = m_nanoSuit.GetOwner();

	if (activate && !m_sprintWasActivatedSinceModeChange)
	{
		m_nanoSuit.ProcessSuitAction( "OnActivate_Sprint_Sound" );
	}

	m_nanoSuit.ActivateSuitState("PowerSprint", activate);

	m_sprintWasActivatedSinceModeChange = true;

	m_sprintingActive = activate;

	/*
	if (suitOwner.IsClient())
		{
			g_pGame->GetScreenEffects()->ProcessSprintingFX(activate, false);
		}*/
	
}

//-------------------------------------
ENanoSuitMode CNanoModePower::GetType() const
{
	return eNanoSuitMode_Power;
}

//--------------------------------------
void CNanoModePower::GetMemoryUsage(ICrySizer* s) const
{
	s->Add(*this);
}

