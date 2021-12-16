#ifndef __FATALITYBONUS_H__
#define __FATALITYBONUS_H__

#include "IPerk.h"
#include "GameRulesModules/IGameRulesKillListener.h"

class FatalityBonus:
	public IPerk, public IGameRulesKillListener
{
public:
	SET_PLAYER_PLUGIN_NAME(FatalityBonus);

	FatalityBonus();
	virtual ~FatalityBonus() {}

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);

	virtual void Enter();
	virtual void Leave();

	//IGameRulesKillListener
	virtual void OnEntityKilled(const HitInfo &hitInfo);
	//~IGameRulesKillListener

protected:
	EPerks CalculateRewardPerk(const HitInfo &hitInfo);
	void BonusFeedback(const EPerks perk);
	void RemoveRewardedPerk();
	void RewardNextPerk();

	int m_killStreak;
	int m_deathStreak;
	EPerks m_nextRewardPerk;
	EPerks m_rewardedPerk;

	int m_meleeType;
	int m_fragType;
	int m_bulletType;
	int m_HMGType;
};

#endif
