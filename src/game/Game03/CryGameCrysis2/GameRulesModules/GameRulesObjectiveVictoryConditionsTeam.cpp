/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Game rules module to handle victory conditions for team games that
		use are won by completing an objective
	-------------------------------------------------------------------------
	History:
	- 24:09:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesObjectiveVictoryConditionsTeam.h"
#include "GameRules.h"
#include "IGameRulesStateModule.h"
#include "IGameRulesObjectivesModule.h"

//-------------------------------------------------------------------------
void CGameRulesObjectiveVictoryConditionsTeam::Update( float frameTime )
{
	if (!gEnv->bServer)
		return;

	IGameRulesStateModule *pStateModule = m_pGameRules->GetStateModule();
	if (pStateModule && pStateModule->GetGameState() != IGameRulesStateModule::EGRS_InGame)
		return;

	CheckTimeLimit();
	CheckObjectives();
}

//-------------------------------------------------------------------------
void CGameRulesObjectiveVictoryConditionsTeam::CheckObjectives()
{
	IGameRulesObjectivesModule *pObjectivesModule = m_pGameRules->GetObjectivesModule();
	if (pObjectivesModule)
	{
		bool team1HasCompleted = pObjectivesModule->HasCompleted(1);
		bool team2HasCompleted = pObjectivesModule->HasCompleted(2);

		if (team1HasCompleted && team2HasCompleted)
		{
			OnEndGame(0, EGOR_ObjectivesCompleted);
		}
		else if (team1HasCompleted)
		{
			OnEndGame(1, EGOR_ObjectivesCompleted);
		}
		else if (team2HasCompleted)
		{
			OnEndGame(2, EGOR_ObjectivesCompleted);
		}
	}
}
