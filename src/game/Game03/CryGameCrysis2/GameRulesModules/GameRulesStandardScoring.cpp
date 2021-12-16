/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Game rules module to handle scoring points values
	-------------------------------------------------------------------------
	History:
	- 03:09:2009  : Created by Ben Johnson

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesStandardScoring.h"
#include "IXml.h"
#include "GameRules.h"
#include "IGameRulesPlayerStatsModule.h"
#include "IGameRulesStateModule.h"
#include "GameRulesTypes.h"
#include "Utility/DesignerWarning.h"

static AUTOENUM_BUILDNAMEARRAY(s_gamerulesScoreType, EGRSTList);

//-------------------------------------------------------------------------
CGameRulesStandardScoring::CGameRulesStandardScoring()
{
	m_maxTeamScore = 0;
	m_startTeamScore = 0;
	m_useScoreAsTime = false;

	for(int i = 0; i < EGRST_Num; i++)
	{
		m_playerScorePoints[i] = 0;
		m_teamScorePoints[i] = 0;
	}
}

//-------------------------------------------------------------------------
CGameRulesStandardScoring::~CGameRulesStandardScoring()
{
}

//-------------------------------------------------------------------------
void CGameRulesStandardScoring::Init( XmlNodeRef xml )
{
	int numScoreCategories = xml->getChildCount();
	for (int i = 0; i < numScoreCategories; ++ i)
	{
		XmlNodeRef categoryXml = xml->getChild(i);
		const char *categoryTag = categoryXml->getTag();

		if (!stricmp(categoryTag, "Player"))
		{
			InitScoreData(categoryXml, &m_playerScorePoints[0]);
		}
		else if (!stricmp(categoryTag, "Team"))
		{
			InitScoreData(categoryXml, &m_teamScorePoints[0]);
			categoryXml->getAttr("maxScore", m_maxTeamScore);
			categoryXml->getAttr("startTeamScore", m_startTeamScore);
			categoryXml->getAttr("useScoreAsTime", m_useScoreAsTime);
		}
	}
}

void CGameRulesStandardScoring::InitScoreData(XmlNodeRef categoryXml, TGameRulesScoreInt *scoringData)
{
	CRY_ASSERT(scoringData);

	int numScoreValues = categoryXml->getChildCount();
	for (int j = 0; j < numScoreValues; ++ j)
	{
		XmlNodeRef childXml = categoryXml->getChild(j);
		const char *scoringTag = childXml->getTag();

		if (!stricmp(scoringTag, "Event"))
		{
			int points = 0;
			if (childXml->getAttr("points", points))
			{
				int type = EGRST_Unknown;
				const char*  pChar = NULL;
				if (childXml->getAttr("type", &pChar))
				{
					bool  typeOk = AutoEnum_GetEnumValFromString(pChar, s_gamerulesScoreType, EGRST_Num, &type);
					CRY_ASSERT(typeOk);

					DesignerWarning( points < SGameRulesScoreInfo::SCORE_MAX && points > SGameRulesScoreInfo::SCORE_MIN, "Adding score for player which is out of net-serialize bounds (%d is not within [%d .. %d])", points, SGameRulesScoreInfo::SCORE_MIN, SGameRulesScoreInfo::SCORE_MAX );
					scoringData[type] = static_cast<TGameRulesScoreInt>(points);
				}
				else
				{
					CryLogAlways("GameRulesStandardScoring::Init() : Scoring Event has no type declared.");
				}
			}
			else
			{
				CryLogAlways("GameRulesStandardScoring::Init() : Scoring Event has no points declared.");
			}
		}
	}
}

//-------------------------------------------------------------------------
TGameRulesScoreInt CGameRulesStandardScoring::GetPlayerPointsByType(EGRST pointsType) const
{
	return GetPointsByType(&m_playerScorePoints[0], pointsType);
}

//-------------------------------------------------------------------------
TGameRulesScoreInt CGameRulesStandardScoring::GetTeamPointsByType(EGRST pointsType) const
{
	return GetPointsByType(&m_teamScorePoints[0], pointsType);
}

//-------------------------------------------------------------------------
TGameRulesScoreInt CGameRulesStandardScoring::GetPointsByType(const TGameRulesScoreInt *scoringData, EGRST pointsType) const
{
	CRY_ASSERT_MESSAGE(pointsType > EGRST_Unknown && pointsType < EGRST_Num, "Out of range parameter passed into CGameRulesStandardScoring::GetPointsByType");
	const TGameRulesScoreInt &scoreData = scoringData[pointsType];

	if(scoreData == 0)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_COMMENT, "Scoring not setup for %s in gamemode %s", s_gamerulesScoreType[pointsType], gEnv->pConsole->GetCVar("sv_gamerules")->GetString());
	}

	return scoreData;
}

void CGameRulesStandardScoring::DoScoringForDeath(IActor *pTargetActor, EntityId shooterId, const char *weaponClassName, int damage, int material, int hit_type)
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	IActor *pShooterActor =  g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(shooterId);

	if (pGameRules && pTargetActor && pShooterActor)
	{
		// No scoring at game end
		IGameRulesStateModule *pStateModule = pGameRules->GetStateModule();
		if (pStateModule && pStateModule->GetGameState() == IGameRulesStateModule::EGRS_PostGame)
		{
			return;
		}

		EntityId targetId = pTargetActor->GetEntityId();
		bool bTeamGame = (pGameRules->GetTeamCount() > 1);
		int targetTeam = pGameRules->GetTeam(targetId);
		int shooterTeam = pGameRules->GetTeam(shooterId);
		
		int playerPoints = 0, teamPoints = 0;
		EGameRulesScoreType scoreType;

		if (pTargetActor == pShooterActor)
		{
			playerPoints = GetPlayerPointsByType(EGRST_Suicide);
			scoreType = EGRST_Suicide;
		}
		else if (bTeamGame && (targetTeam == shooterTeam))
		{
			playerPoints = GetPlayerPointsByType(EGRST_PlayerTeamKill);
			scoreType = EGRST_PlayerTeamKill;
		}
		else
		{
			playerPoints = GetPlayerPointsByType(EGRST_PlayerKill);
			scoreType = EGRST_PlayerKill;

			if (bTeamGame)
				teamPoints = GetTeamPointsByType(EGRST_PlayerKill);
		}

		SGameRulesScoreInfo scoreInfo(scoreType, playerPoints);
		scoreInfo.AttachVictim(targetId);
		CryLogAlways ("About to call pGameRules->IncreasePoints, pGameRules=%p", pGameRules);
		pGameRules->IncreasePoints(shooterId, scoreInfo);

		if (bTeamGame && teamPoints)
		{
			int teamScore = pGameRules->GetTeamsScore(shooterTeam) + teamPoints;
			pGameRules->SetTeamsScore(shooterTeam, teamScore);
		}
	}
}

bool CGameRulesStandardScoring::ShouldScore(CGameRules *pGameRules) const
{
	if (!pGameRules)
		return false;

	// No scoring at game end
	IGameRulesStateModule *pStateModule = pGameRules->GetStateModule();
	if (pStateModule && pStateModule->GetGameState() == IGameRulesStateModule::EGRS_PostGame)
	{
		return false;
	}

	return true;
}

void CGameRulesStandardScoring::OnPlayerScoringEvent( EntityId playerId, EGRST pointsType)
{
	CRY_ASSERT(pointsType != EGRST_Unknown);

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if(ShouldScore(pGameRules))
	{
		int playerPoints = GetPlayerPointsByType(pointsType);

		if(playerPoints != 0)
		{
			SGameRulesScoreInfo  scoreInfo(pointsType, playerPoints);
			pGameRules->IncreasePoints(playerId, scoreInfo);
		}
	}

}

void CGameRulesStandardScoring::OnTeamScoringEvent( int teamId, EGRST pointsType)
{
	CRY_ASSERT(pointsType != EGRST_Unknown);

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if(ShouldScore(pGameRules))
	{
		bool bTeamGame = (pGameRules->GetTeamCount() > 1);
		if (bTeamGame)
		{
			int teamPoints = GetTeamPointsByType(pointsType);
			if (teamPoints)
			{
				int teamScore = pGameRules->GetTeamsScore(teamId) + teamPoints;

				if (m_maxTeamScore)
				{
					if (m_useScoreAsTime)
					{
						float gameTime = pGameRules->GetCurrentGameTime();
						int maxActualScore = (int)floor(gameTime + m_maxTeamScore);
						if (teamScore > maxActualScore)
						{
							teamScore = maxActualScore;
						}
					}
					else
					{
						if (teamScore > m_maxTeamScore)
						{
							teamScore = m_maxTeamScore;
						}
					}
				}
				pGameRules->SetTeamsScore(teamId, teamScore);
			}
		}
	}
}
