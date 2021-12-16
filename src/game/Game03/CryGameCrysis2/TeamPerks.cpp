#include "StdAfx.h"
#include "TeamPerks.h"

#include "Player.h"
#include "GameCVars.h"
#include "GameRules.h"
#include "PerkDbgDisplay.h"
#include "HUD/UI/UIButtonPromptRegion.h"
#include "Audio/AudioSignalPlayer.h"
#include "HUD/HUD.h"

STeamPerkData::STeamPerkData(const char * name, const char * announcement)
{
	m_name = name;
	m_announcement = announcement;
};

const STeamPerkData CTeamPerks::s_teamPerkList[] =
{
	STeamPerkData ("Radar",			"TeamRadar"),
	STeamPerkData ("SuitBoost",	"SuitBoost"),
};

CTeamPerks::CTeamPerks(void)
{
	memset (m_teamPerkActivationInfo, 0, sizeof(m_teamPerkActivationInfo));
}

void CTeamPerks::Update(float fFrameTime)
{
	for (int i=0; i<eTeamPerk_Last; i++)
	{
		if (m_teamPerkActivationInfo[i].m_numReasonsToBeActive)
		{
			PerkDbgDisplay("TEAM %p: %d team-members are enabling '%s' team perk", this, m_teamPerkActivationInfo[i].m_numReasonsToBeActive, s_teamPerkList[i].m_name);
		}
	}
}

void CTeamPerks::UpdateTeamPerkCount(int teamNum, ETeamPerks teamPerkID, EntityId activatorId, int change)
{
	CRY_ASSERT_MESSAGE(teamPerkID >= 0 && teamPerkID < eTeamPerk_Last, string().Format("Invalid team perk ID %d! About to access data outside of array bounds!", teamPerkID));
	STeamPerkActivationInfo * info = & m_teamPerkActivationInfo[teamPerkID];

	bool wasActive = (info->m_numReasonsToBeActive > 0);
	info->m_numReasonsToBeActive += change;
	CRY_ASSERT_MESSAGE(info->m_numReasonsToBeActive >= 0, string().Format("Number of reasons for team perk %d to be active has dropped to %d! This should not happen!", teamPerkID, info->m_numReasonsToBeActive));
	bool nowActive = info->m_numReasonsToBeActive > 0;
	
	CryLog ("[TEAMPERK] Team %d team-perk %d was %s, is now %s; reasons to be active = %d (just changed by %d)", teamNum, teamPerkID, wasActive ? "on" : "off", nowActive ? "on" : "off", info->m_numReasonsToBeActive, change);

	if (nowActive != wasActive)
	{
		TeamPerkStateHasChanged(teamPerkID, teamNum, activatorId, nowActive);
	}
}

//===============================================================================================
// This function should ONLY give feedback about a team perk being activated or deactivated!
//===============================================================================================
void CTeamPerks::TeamPerkStateHasChanged(ETeamPerks teamPerk, int teamNum, EntityId activatorId, bool isNowOn)
{
	EntityId localActorId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	
	if (localActorId == 0)
	{
		CryLogAlways ("[TEAMPERK] A team perk is getting %s while there's no local player! Skipping all feedback!", isNowOn ? "enabled" : "disabled");
	}
	else
	{
		bool localTeam = teamNum == g_pGame->GetGameRules()->GetTeam(localActorId);

		if(isNowOn && s_teamPerkList[teamPerk].m_announcement[0] != '\0')
		{
			CAnnouncer::GetInstance()->Announce(activatorId, s_teamPerkList[teamPerk].m_announcement);
		}

		switch (teamPerk)
		{
		case eTeamPerk_Radar:
			{
				IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(localActorId);
				if(pActor && pActor->IsPlayer())
				{
					if(isNowOn && !localTeam)
					{
						CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
						if(pPlayer->IsPerkActive(ePerk_RadarJammer))
						{
							CAudioSignalPlayer::JustPlay("Perk_RadarJammer", localActorId);
						}
					}
					if(localTeam)
					{
						SHUDEvent eventTeamRadar(eHUDEvent_OnTeamRadarChanged);
						eventTeamRadar.AddData(SHUDEventData(isNowOn));
						CHUD::CallEvent(eventTeamRadar);
					}
				}
			}
			break;

		case eTeamPerk_SuitBoost:
			if (localTeam)
			{
				CUIButtonPromptRegion::SetOnScreenMessageText("suitBoostMsg", "ENHANCED NANOSUIT EFFICIENCY", NULL, NULL, NULL, isNowOn ? 1000.f : 0.f);
			}
			IActorIteratorPtr pIter = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
			while (IActor *pActor = pIter->Next())
			{
				bool isOnSuitBoostTeam = g_pGame->GetGameRules()->GetTeam(pActor->GetEntityId()) == teamNum;
				if(isOnSuitBoostTeam)
				{
					CPlayer *pPlayer = static_cast<CPlayer*>(pActor);
					pPlayer->GetNanoSuit()->SetMaximumSuitMaterial(isNowOn);
				}
			}
			break;
		}
	}
}
