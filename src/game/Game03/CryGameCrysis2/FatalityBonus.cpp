#include "StdAfx.h"
#include "FatalityBonus.h"

#include "GameRules.h"
#include "Player.h"

FatalityBonus::FatalityBonus()
{
	m_killStreak = 0;
	m_deathStreak = 0;
	m_rewardedPerk = ePerk_Null;
	m_nextRewardPerk = ePerk_Null;

	CGameRules* pGameRules = g_pGame->GetGameRules();
	m_meleeType = pGameRules->GetHitTypeId("melee");
	m_fragType = pGameRules->GetHitTypeId("frag");
	m_bulletType = pGameRules->GetHitTypeId("bullet");
	m_HMGType = pGameRules->GetHitTypeId("HMG");
}

void FatalityBonus::OnEntityKilled(const HitInfo &hitInfo)
{
	const EntityId clientId = m_ownerPlayer->GetEntityId();
	if(hitInfo.targetId == clientId)
	{
		m_killStreak = 0;
		m_deathStreak++;
		if(m_deathStreak >= CPerk::GetInstance()->GetVars()->perk_FatalityBonus_deathStreakRequired)
		{
			m_nextRewardPerk = CalculateRewardPerk(hitInfo);
		}
	}
	else if(hitInfo.shooterId == clientId)
	{
		m_deathStreak = 0;
		m_killStreak++;
		if(m_killStreak >= CPerk::GetInstance()->GetVars()->perk_FatalityBonus_killStreakToStop)
		{
			RemoveRewardedPerk();
		}
	}
}

EPerks FatalityBonus::CalculateRewardPerk(const HitInfo &hitInfo)
{

	EPerks rewardPerk = ePerk_Deflection;

	if(hitInfo.type == m_meleeType)
	{
		rewardPerk = ePerk_MeleeDefense;
	}
	else if(hitInfo.type == m_fragType)
	{
		rewardPerk = ePerk_FragResistant;
	}
	else if(hitInfo.type == m_bulletType || hitInfo.type == m_HMGType)
	{
		IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.shooterId);
		if(pActor)
		{
			const float dot = m_ownerPlayer->GetEntity()->GetForwardDir().dot(pActor->GetEntity()->GetForwardDir());
			if(dot > 0.0f)
			{
				rewardPerk = ePerk_RearDeflection;
			}
		}
	}

	return rewardPerk;
}

void FatalityBonus::BonusFeedback(const EPerks perk)
{
	CRY_TODO(24, 02, 2010, "HUD feedback - waiting for flash version of hud messages");

	CAudioSignalPlayer::JustPlay("FatalityBonus");
}

void FatalityBonus::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
	case EPE_Spawn:
		{
			RemoveRewardedPerk();
			RewardNextPerk();
			break;
		}
	default:
		{	
			IPerk::HandleEvent (perkEvent, data);
			break;
		}
	}
}

void FatalityBonus::RemoveRewardedPerk()
{
	if(m_rewardedPerk != ePerk_Null)
	{
		m_ownerPlayer->SetPerkActive(m_rewardedPerk, false);
		m_rewardedPerk = ePerk_Null;
	}
}

void FatalityBonus::RewardNextPerk()
{
	if(m_nextRewardPerk != ePerk_Null)
	{
		m_ownerPlayer->SetPerkActive(m_nextRewardPerk);
		m_rewardedPerk = m_nextRewardPerk;
		BonusFeedback(m_nextRewardPerk);
		m_nextRewardPerk = ePerk_Null;
	}

}

void FatalityBonus::Enter()
{
	IPerk::Enter();

	CGameRules* pGameRules = g_pGame->GetGameRules();
	pGameRules->RegisterKillListener(this);

}

void FatalityBonus::Leave()
{
	CGameRules* pGameRules = g_pGame->GetGameRules();
	pGameRules->UnRegisterKillListener(this);
	
	IPerk::Leave();
}
