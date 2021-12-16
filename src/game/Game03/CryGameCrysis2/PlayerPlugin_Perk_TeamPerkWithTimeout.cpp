#include "StdAfx.h"
#include "PlayerPlugin_Perk_TeamPerkWithTimeout.h"
#include "PlayerPlugin_ScoreRewards.h"
#include "Player.h"
#include "PerkIconData.h"
#include "PerkDbgDisplay.h"
#include "GameRules.h"
#include "Utility/StringUtils.h"
#include "Audio/AudioSignalPlayer.h"
#include "PlayerProgression.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

CPlayerPlugin_Perk_TeamPerkWithTimeout::CPlayerPlugin_Perk_TeamPerkWithTimeout(ETeamPerks teamPerkID, float duration, const char* startSignal, const char* stopSignal, const char* loopSignal) :
	m_teamPerkID(teamPerkID),
	m_duration(duration),
	m_activatedPerkForTeam(0),
	m_active(false)
{

	m_startSignalID = startSignal ? g_pGame->GetGameAudio()->GetSignalID(startSignal) : INVALID_AUDIOSIGNAL_ID;
	m_stopSignalID = stopSignal ? g_pGame->GetGameAudio()->GetSignalID(stopSignal) : INVALID_AUDIOSIGNAL_ID;
	if(loopSignal)
	{
		m_loopSignal.SetSignal(loopSignal);
	}
}

void CPlayerPlugin_Perk_TeamPerkWithTimeout::Activate()
{
	if(!m_active)
	{
		m_active = true;

		CGameRules * rules = g_pGame->GetGameRules();
		PlayerPluginLog ("On team %d, Starting now!", rules->GetTeam(m_ownerPlayer->GetEntityId()));
		EntityId entityId = m_ownerPlayer->GetEntityId();
		ActivatePerkForTeam(rules->GetTeam(entityId), entityId);

		CCCPOINT_IF(m_perkId == ePerk_TeamSuitBoost, Perk_MaximumSuit_Enter);

		if(m_ownerPlayer->IsClient())
		{
			if(m_perkId == ePerk_TeamSuitBoost)
			{
				CPlayerProgression::GetInstance()->Event(EPP_SuitBoost);
			}
			else if(m_perkId == ePerk_TeamSuperRadar)
			{
				CPlayerProgression::GetInstance()->Event(EPP_TeamRadar);
			}
		}

		if(m_ownerPlayer->IsClient())
		{
			CAudioSignalPlayer::JustPlay(m_startSignalID, entityId);
			m_loopSignal.Play(entityId);
		}

		CPerkIconData * iconData = CPerkIconData::GetForEntity(entityId);
		if (iconData)
		{
			CControllerInputRenderInfo * myPrompt = iconData->GetIconInputPrompt(m_perkId);
			if(myPrompt)
			{
				myPrompt->Clear();
			}
		}
	}
}

void CPlayerPlugin_Perk_TeamPerkWithTimeout::Enter()
{
	IPerk::Enter();

	EntityId entityId = m_ownerPlayer->GetEntityId();
	CPerkIconData * iconData = CPerkIconData::GetForEntity(entityId);
	if (iconData)
	{
		CControllerInputRenderInfo * myPrompt = iconData->GetIconInputPrompt(m_perkId);
		if(myPrompt)
		{
			myPrompt->CreateForInput("crysis2_common", "binoculars");
		}
	}
}

void CPlayerPlugin_Perk_TeamPerkWithTimeout::Leave()
{
	CCCPOINT_IF(m_perkId == ePerk_TeamSuitBoost, Perk_MaximumSuit_Exit);
	ActivatePerkForTeam(0, m_ownerPlayer->GetEntityId());

	IPerk::Leave();
	if(m_ownerPlayer->IsClient())
	{
		EntityId entityId = m_ownerPlayer->GetEntityId();
		CAudioSignalPlayer::JustPlay(m_stopSignalID, entityId);
		m_loopSignal.Stop();
	}

	m_active = false;
}

void CPlayerPlugin_Perk_TeamPerkWithTimeout::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	IPerk::HandleEvent (perkEvent, data);

	switch(perkEvent)
	{
		case EPE_Reset:
		m_timeAlive = 0.f;
		break;

		case EPE_Die:
		PlayerPluginLog ("player on team %d has died!", g_pGame->GetGameRules()->GetTeam(m_ownerPlayer->GetEntityId()));
		m_timeAlive = m_duration;
		break;

		case EPE_SetTeam:
		{
			const STeamChangeInfo * teamChangeInfo = (const STeamChangeInfo *)data;
			CGameRules * rules = g_pGame->GetGameRules();

			EntityId entityId = m_ownerPlayer->GetEntityId();
			PlayerPluginAssert(rules->GetTeam(entityId) == teamChangeInfo->newTeamNum, "New team numbers should match");
			PlayerPluginAssert(m_activatedPerkForTeam == teamChangeInfo->oldTeamNum, "Old team numbers should match");

			PlayerPluginLog("Changed team from %d to %d", teamChangeInfo->oldTeamNum, teamChangeInfo->newTeamNum);
			ActivatePerkForTeam(teamChangeInfo->newTeamNum, entityId);
		}
		case EPE_ActivateTeamPerk:
			{
				Activate();
				break;
			}
		break;
	}
}

void CPlayerPlugin_Perk_TeamPerkWithTimeout::Update(const float dt)
{
	if(m_active)
	{
		m_timeAlive += dt;
	}
	
	EntityId entityId = m_ownerPlayer->GetEntityId();
	CPerkIconData * iconData = CPerkIconData::GetForEntity(entityId);

	PlayerPluginWatch("Time alive = %.1f / %.1f", m_timeAlive, m_duration);

	if (iconData)
	{
		if (m_timeAlive > m_duration)
		{
			m_ownerPlayer->SetPerkActive(m_perkId, false);
		}
		else
		{
			iconData->SetIconDrainAmount(m_perkId, m_timeAlive / m_duration);
		}
	}

	if(m_ownerPlayer->IsClient())
	{
		m_loopSignal.SetParam(entityId, "boost", 1.0f - (m_timeAlive/m_duration));
	}
}

void CPlayerPlugin_Perk_TeamPerkWithTimeout::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if(aspect == CPlayer::ASPECT_PERK_TEAMPERK_CLIENT)
	{
		bool previousActive = m_active;
		ser.Value("active", m_active, 'bool');
		if(m_active != previousActive)	//must be reading and changed
		{
			CGameRules * rules = g_pGame->GetGameRules();
			EntityId entityId = m_ownerPlayer->GetEntityId();
			ActivatePerkForTeam(m_active ? rules->GetTeam(entityId) : 0, entityId);
		}
	}
}

void CPlayerPlugin_Perk_TeamPerkWithTimeout::ActivatePerkForTeam(int team, EntityId activatorId)
{
	if (m_activatedPerkForTeam != team)
	{
		CGameRules * rules = g_pGame->GetGameRules();
		if (rules)
		{
			rules->UpdateTeamPerkCount(m_activatedPerkForTeam, m_teamPerkID, activatorId, -1);
			rules->UpdateTeamPerkCount(team, m_teamPerkID, activatorId, 1);
			m_activatedPerkForTeam = team;

			if(m_ownerPlayer->IsClient())
				CHANGED_NETWORK_STATE(m_ownerPlayer, CPlayer::ASPECT_PERK_TEAMPERK_CLIENT);
		}
		else
		{
			// The only time having no rules is OK is when we're leaving because the game's over (i.e. when setting team number to 0)
			PlayerPluginAssert(team == 0, "Failed to find game rules while setting team number in team perk plug-in class");
		}
	}
}
