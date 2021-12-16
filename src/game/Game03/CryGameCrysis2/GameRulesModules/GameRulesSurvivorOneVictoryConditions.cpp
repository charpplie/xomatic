/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Game rules module to handle victory upon being the last player left
	-------------------------------------------------------------------------
	History:
	- 06:11:2009  : Created by Thomas Houghton

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesSurvivorOneVictoryConditions.h"

#include "IXml.h"
#include "GameRules.h"
#include "IGameRulesPlayerStatsModule.h"
#include "IGameRulesStateModule.h"
#include "HUD/HUD.h"
#include "Audio/Announcer.h"
#include "Audio/AudioSignalPlayer.h"
#include "GameRulesModules/IGameRulesRoundsModule.h"
#include "GameRulesModules/IGameRulesScoringModule.h"
#include "Utility/CryWatch.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#if CRY_WATCH_ENABLED
#define WATCH_LV1  g_pGameCVars->g_SurvivorOneVictoryConditions_watchLvl<1 ? (NULL) : CryWatch
#define WATCH_LV2  g_pGameCVars->g_SurvivorOneVictoryConditions_watchLvl<2 ? (NULL) : CryWatch
#else
#define WATCH_LV1
#define WATCH_LV2
#endif


#define WATCH_LATEST_SURVIVOR_COUNT  (1 && CRY_WATCH_ENABLED)

//-------------------------------------------------------------------------
CGameRulesSurvivorOneVictoryConditions::CGameRulesSurvivorOneVictoryConditions()
{
	if (gEnv->bServer)
	{
		m_svLatestSurvCount = -123;
		memset(m_svLatestSurvList, 0, (sizeof(m_svLatestSurvList) / sizeof(m_svLatestSurvList[0])));
	}
}

//-------------------------------------------------------------------------
CGameRulesSurvivorOneVictoryConditions::~CGameRulesSurvivorOneVictoryConditions()
{
	if (gEnv->bServer)
	{
		CGameRules*  pGameRules = g_pGame->GetGameRules();
		if (pGameRules)
		{
			pGameRules->UnRegisterSurvivorCountListener(this);
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesSurvivorOneVictoryConditions::Init( XmlNodeRef xml )
{
	inherited::Init(xml);

	int  dbgWatchLvl;
	if (xml->getAttr("dbgWatchLvl", dbgWatchLvl))
	{
		g_pGameCVars->g_SurvivorOneVictoryConditions_watchLvl = dbgWatchLvl;
	}

	if (gEnv->bServer)
	{
		CGameRules*  pGameRules = g_pGame->GetGameRules();
		pGameRules->RegisterSurvivorCountListener(this);

		m_svLatestSurvCount = -123;
		memset(m_svLatestSurvList, 0, (sizeof(m_svLatestSurvList) / sizeof(m_svLatestSurvList[0])));
	}
}

//-------------------------------------------------------------------------
void CGameRulesSurvivorOneVictoryConditions::Update( float frameTime )
{
	inherited::Update(frameTime);

	if (!gEnv->bServer)
		return;

	WATCH_LV2("[CGameRulesSurvivorOneVictoryConditions]");

#if WATCH_LATEST_SURVIVOR_COUNT
	WATCH_LV2("Survivors (%d) :", m_svLatestSurvCount);
	for (int i=0; i<m_svLatestSurvCount; i++)
	{
		IEntity*  e = gEnv->pEntitySystem->GetEntity(m_svLatestSurvList[i]);
		WATCH_LV2(" %s", (e?e->GetName():"!"));
	}
#endif

	/*
	IGameRulesStateModule*  pStateModule = m_pGameRules->GetStateModule();
	if (pStateModule && pStateModule->GetGameState() != IGameRulesStateModule::EGRS_InGame)
		return;
	*/
}

//-------------------------------------------------------------------------
void CGameRulesSurvivorOneVictoryConditions::ClVictoryPlayer( int playerId, EGameOverReason reason )
{
	inherited::ClVictoryPlayer(playerId, reason);
}

//-------------------------------------------------------------------------
void CGameRulesSurvivorOneVictoryConditions::TimeLimitExpired()
{
	if (gEnv->bServer)  // this might always be true anyway, dunno
	{
		IGameRulesRoundsModule*  pRoundsModule = m_pGameRules->GetRoundsModule();
		if (!pRoundsModule || !pRoundsModule->IsRestarting())
		{
			assert(m_svLatestSurvCount > 1);
			if (IGameRulesScoringModule* pScoringModule=g_pGame->GetGameRules()->GetScoringModule())
			{
				for (int i=0; i<m_svLatestSurvCount; i++)
				{
					CCCPOINT(SurvivorOneVictoryConditions_SvDrawScoreEvent);
					pScoringModule->OnPlayerScoringEvent(m_svLatestSurvList[i], EGRST_AON_Draw);
				}
			}
			CCCPOINT(SurvivorOneVictoryConditions_SvTimeLimitReached);
			OnEndGamePlayer(0, EGOR_TimeLimitReached);
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesSurvivorOneVictoryConditions::OnEndGamePlayer( EntityId playerId, EGameOverReason reason )
{
	inherited::OnEndGamePlayer(playerId, reason);
}

//-------------------------------------------------------------------------
// IGameRulesSurvivorCountListener
void CGameRulesSurvivorOneVictoryConditions::SvSurvivorCountRefresh(int count, const EntityId survivors[], int numKills)
{
	CRY_ASSERT(gEnv->bServer);

	int  sz = (count * sizeof(survivors[0]));
	CRY_ASSERT_MESSAGE(sz <= sizeof(m_svLatestSurvList), "Memory overwrite!");
	memcpy(m_svLatestSurvList, survivors, sz);
	m_svLatestSurvCount = count;

	IGameRulesStateModule*  pStateModule = m_pGameRules->GetStateModule();
	if (pStateModule && (pStateModule->GetGameState() == IGameRulesStateModule::EGRS_InGame))
	{
		if (count == 1)  // TODO could also pass a count delta into this function (the amount of change since this function was last called) to check here whether or not the count has actually gone *down* to 1
		{
			if (numKills > 0)
			{
				EntityId  winner = survivors[0];
				CRY_ASSERT(winner);
				if (IGameRulesScoringModule* pScoringModule=g_pGame->GetGameRules()->GetScoringModule())
				{
					CCCPOINT(SurvivorOneVictoryConditions_SvSurvivorOneScoreEvent);
					pScoringModule->OnPlayerScoringEvent(winner, EGRST_AON_Win);
				}
				CCCPOINT(SurvivorOneVictoryConditions_SvSurvivorOneEndGame);
				OnEndGamePlayer(winner, EGOR_ObjectivesCompleted);  // TODO do we need our own EGOR_ value here?
			}
		}
	}
}


#undef WATCH_LV1
#undef WATCH_LV2