/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
Description:
Rewards the player for scoring certain numbers of points without dying
**************************************************************************/

#ifndef __PLAYERPLUGIN_SCOREREWARDS_H__
#define __PLAYERPLUGIN_SCOREREWARDS_H__

#include "PlayerPlugin.h"
#include "Perk.h"
#include "FrontEnd/ILoadingMessageProvider.h"

//--------------------------------------------------------------------------------------------
// CScoreRewardTable: singleton class containing list of scores and rewards
//--------------------------------------------------------------------------------------------

struct SScoreRewardTableEntry
{
	int m_numberOfPointsWithoutDying;
	EPerks m_givePerk;
};

class CScoreRewardTable : public ILoadingMessageProvider
{
	public:
		CScoreRewardTable();
		~CScoreRewardTable();
		void Init(const char* xmlFilename);
		void CheckPassedThreshold(CPlayer * player, int & nextThreshold, int pointsWithoutDying) const;

		static ILINE const CScoreRewardTable * GetInstance()
		{
			ASSERT_IS_NOT_NULL (s_instance);
			return s_instance;
		}

	private:
		static const int k_maxEntries = 4;
		static CScoreRewardTable * s_instance;

		CLoadingMessageProviderListNode m_messageProviderListNode;

		SScoreRewardTableEntry m_entries[k_maxEntries];
		int m_numEntries;

		int GetNumMessagesProvided() const;
		string GetMessageNum(int n) const;
};

//--------------------------------------------------------------------------------------------
// CPlayerPlugin_ScoreReward: class keeping track of the current score of a single player
//--------------------------------------------------------------------------------------------

class CPlayerPlugin_ScoreRewards : public CPlayerPlugin
{
	public:
		SET_PLAYER_PLUGIN_NAME(CPlayerPlugin_ScoreRewards);

		CPlayerPlugin_ScoreRewards();

	private:
		virtual void Update(const float dt);
		virtual void HandleEvent(EPlayerPlugInEvent theEvent, void * data);

		// NB: It would use less memory, but be horribly unpleasant, to store these in the singleton
		// CScoreRewardTable class instead as they're only updated for the local player. But which is
		// more important, using less memory or not being horribly unpleasant? Let's keep 'em here,
		// if only for now. [TF]

		int m_scoreWhenLastDied;
		int m_currentScore;
		int m_nextScoreThresholdToHit;
};

#endif __PLAYERPLUGIN_SCOREREWARDS_H__

