/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Game rules module for dealing with modes that have objectives that 
		change (completing objective 1 starts objective 2)
	-------------------------------------------------------------------------
	History:
	- 21:09:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesProgressiveObjectives.h"
#include "IXml.h"
#include "GameRules.h"
#include "GameRulesModules/GameRulesSimpleEntityBasedObjective.h"
#include "GameRulesModules/GameRulesObjectiveHelper_Spawn.h"
#include "GameRulesModules/GameRulesObjectiveHelper_NeutralSpawn.h"
#include "GameRulesModules/GameRulesObjectiveHelper_Carry.h"
#include "GameRulesModules/GameRulesObjectiveHelper_Icon.h"

#if NUM_ASPECTS > 8
	#define PROGRESSIVE_OBJECTIVES_ASPECT			eEA_GameServerB
#else
	#define PROGRESSIVE_OBJECTIVES_ASPECT			eEA_GameServerStatic
#endif

//-------------------------------------------------------------------------
CGameRulesProgressiveObjectives::CGameRulesProgressiveObjectives()
{
	m_teamBased = false;

	m_teamStages.insert(TTeamStageMap::value_type(1, -1));
	m_teamStages.insert(TTeamStageMap::value_type(2, -1));

	m_finished[0] = false;
	m_finished[1] = false;
	m_isResetting = false;
}

//-------------------------------------------------------------------------
CGameRulesProgressiveObjectives::~CGameRulesProgressiveObjectives()
{
	int numStages = m_stages.size();

	for (int i = 0; i < numStages; ++ i)
	{
		SObjectivesStage *stage = &m_stages[i];
		int numObjectives = stage->m_objectives.size();

		for (int j = 0; j < numObjectives; ++ j)
		{
			IGameRulesObjective *pObjective = stage->m_objectives[j];
			delete pObjective;
		}

		stage->m_objectives.clear();
	}

	g_pGame->GetIGameFramework()->UnregisterListener(this);
}

//-------------------------------------------------------------------------
void CGameRulesProgressiveObjectives::Init( XmlNodeRef xml )
{
	int teamBased = 0;
	if (xml->getAttr("teamBased", teamBased))
	{
		m_teamBased = (teamBased != 0);
	}

	// Read stages
	int numStages = xml->getChildCount();
	for (int stageIdx = 0; stageIdx < numStages; ++ stageIdx)
	{
		XmlNodeRef stageXml = xml->getChild(stageIdx);
		if (!stricmp(stageXml->getTag(), "Stage"))
		{
			m_stages.push_back(SObjectivesStage());
			SObjectivesStage *currentStage = &m_stages[m_stages.size() - 1];

			bool exitConditionFound = false;

			int numChildren = stageXml->getChildCount();
			for (int childIdx = 0; childIdx < numChildren; ++ childIdx)
			{
				XmlNodeRef childXml = stageXml->getChild(childIdx);
				const char *childTag = childXml->getTag();

				if (!stricmp(childTag, "Objective"))
				{
					IGameRulesObjective *pObjective = ReadObjective(childXml);
					if (pObjective)
					{
						currentStage->m_objectives.push_back(pObjective);
					}
				}
				else if (!stricmp(childTag, "Exit"))
				{
					CRY_ASSERT_MESSAGE(!exitConditionFound, "Stage has multiple exit conditions");
					exitConditionFound = ReadExitType(childXml, currentStage->m_stageExitConditionType, currentStage->m_stageExitCondition);
				}
				else
				{
					CRY_ASSERT(!"Syntax error: Expected 'Objective' or 'Exit' node");
				}
			}
			if (!exitConditionFound)
			{
				currentStage->m_stageExitConditionType = ESCT_None;
			}
		}
		else
		{
			CRY_ASSERT(!"Syntax error: Expected 'Stage' node");
		}
	}
	g_pGame->GetIGameFramework()->RegisterListener(this, "progressiveObjectives", FRAMEWORKLISTENERPRIORITY_GAME);
}

//-------------------------------------------------------------------------
void CGameRulesProgressiveObjectives::Update( float frameTime )
{
	// TODO: Only update active objectives
	int numStages = m_stages.size();

	for (int i = 0; i < numStages; ++ i)
	{
		SObjectivesStage *stage = &m_stages[i];
		int numObjectives = stage->m_objectives.size();

		for (int j = 0; j < numObjectives; ++ j)
		{
			IGameRulesObjective *pObjective = stage->m_objectives[j];
			pObjective->Update(frameTime);
		}
	}

	if (gEnv->bServer)
	{
		TTeamStageMap::iterator it = m_teamStages.begin();
		for (; it != m_teamStages.end(); ++ it)
		{
			int teamId = it->first;
			bool completed = false;

			int stageIdx = it->second;
			if (stageIdx != -1)
			{
				SObjectivesStage *pStage = &m_stages[stageIdx];
				switch (pStage->m_stageExitConditionType)
				{
				case ESCT_TeamScore:
					{
						int teamScore = g_pGame->GetGameRules()->GetTeamsScore(teamId);
						if (teamScore >= pStage->m_stageExitCondition.m_score.m_requiredScore)
						{
							completed = true;
						}
					}
					break;
				case ESCT_Completed:
					{
						completed = true;
						int numObjectives = pStage->m_objectives.size();
						for (int i = 0; i < numObjectives; ++ i)
						{
							IGameRulesObjective *pObjective = pStage->m_objectives[i];
							completed &= pObjective->IsComplete(teamId);
						}
					}
					break;
				}

				if (completed)
				{
					CryLog("CGameRulesProgressiveObjectives::Update() team %i has completed stage %i", teamId, it->second);
					int numStages = m_stages.size();
					if (it->second < numStages - 1)
					{
						CryLog("Entering next stage");
						ChangeStage(teamId, it->second + 1);
					}
					else
					{
						CryLog("Finished all stages");
						ChangeStage(teamId, -1);
						m_finished[teamId - 1] = true;
					}
				}
			}
		}
	}
}

//-------------------------------------------------------------------------
IGameRulesObjective *CGameRulesProgressiveObjectives::ReadObjective(XmlNodeRef xml)
{
	IGameRulesObjective *pObjective = 0;

	const char *objectiveType = 0;
	if (xml->getAttr("type", &objectiveType))
	{
		// Check what objective type we are and create the appropriate handler
		if (!stricmp(objectiveType, "SimpleEntityBased"))
		{
			pObjective = new CGameRulesSimpleEntityBasedObjective();
		}
		else if (!stricmp(objectiveType, "Helper_Spawn"))
		{
			pObjective = new CGameRulesObjectiveHelper_Spawn();
		}
		else if (!stricmp(objectiveType, "Helper_NeutralSpawn"))
		{
			pObjective = new CGameRulesObjectiveHelper_NeutralSpawn();
		}
		else if (!stricmp(objectiveType, "Helper_Carry"))
		{
			pObjective = new CGameRulesObjectiveHelper_Carry();
		}
		else if (!stricmp(objectiveType, "Helper_Icon"))
		{
			pObjective = new CGameRulesObjectiveHelper_Icon();
		}
		else
		{
			CRY_ASSERT(!"Syntax error: Unknown objective type");
		}
	}
	else
	{
		CRY_ASSERT(!"Syntax error: Expected 'type' attribute");
	}

	if (pObjective)
	{
		pObjective->Init(xml);
	}

	return pObjective;
}

//-------------------------------------------------------------------------
bool CGameRulesProgressiveObjectives::ReadExitType( XmlNodeRef xml, EStageConditionType &outType, UStageExitCondition &outCondition )
{
	const char *exitType = 0;
	if (xml->getAttr("type", &exitType))
	{
		if (!stricmp(exitType, "TeamScore"))
		{
			outType = ESCT_TeamScore;
			if (!xml->getAttr("score", outCondition.m_score.m_requiredScore))
			{
				outCondition.m_score.m_requiredScore = g_pGame->GetGameRules()->GetScoreLimit();
			}
		}
		else if (!stricmp(exitType, "Completed"))
		{
			outType = ESCT_Completed;
		}
	}

	return true;
}

//-------------------------------------------------------------------------
void CGameRulesProgressiveObjectives::OnStartGame()
{
	DoStartGame(false);
}

//-------------------------------------------------------------------------
void CGameRulesProgressiveObjectives::DoStartGame(bool isReset)
{
	if (m_isResetting && !isReset)
	{
		return;
	}

	int numStages = m_stages.size();

	for (int i = 0; i < numStages; ++ i)
	{
		SObjectivesStage *stage = &m_stages[i];
		int numObjectives = stage->m_objectives.size();

		for (int j = 0; j < numObjectives; ++ j)
		{
			IGameRulesObjective *pObjective = stage->m_objectives[j];
			pObjective->OnStartGame();
		}
	}

	if (numStages && gEnv->bServer)
	{
		CRY_TODO(23, 09, 09, "Support modes without 2 teams");
		// Set both teams to the first stage
		for (int i = 1; i < 3; ++ i)
		{
			TTeamStageMap::iterator it = m_teamStages.find(i);
			if (it != m_teamStages.end())
			{
				ChangeStage(i, 0);
			}
		}
	}
	m_finished[0] = false;
	m_finished[1] = false;
}

//-------------------------------------------------------------------------
bool CGameRulesProgressiveObjectives::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (aspect == PROGRESSIVE_OBJECTIVES_ASPECT)
	{
		for (int teamId = 1; teamId < 3; ++ teamId)
		{
			TTeamStageMap::iterator it = m_teamStages.find(teamId);
			if (it != m_teamStages.end())
			{
				int stageIdx = it->second;
				ser.Value("Stage", stageIdx, 'i8');
				if (ser.IsReading())
				{
					if (it->second != stageIdx)
					{
						ChangeStage(teamId, stageIdx);
						if (stageIdx == -1)
						{
							CryLog("CGameRulesProgressiveObjectives::NetSerialize(), team %i has finished all their objectives", teamId);
						}
					}
				}
			}
			else
			{
				int stage = 0;
				ser.Value("Stage", stage, 'i8');
			}
		}
	}

	int numStages = m_stages.size();

	for (int i = 0; i < numStages; ++ i)
	{
		SObjectivesStage *stage = &m_stages[i];
		int numObjectives = stage->m_objectives.size();

		for (int j = 0; j < numObjectives; ++ j)
		{
			IGameRulesObjective *pObjective = stage->m_objectives[j];
			pObjective->NetSerialize(ser, aspect, profile, flags);
		}
	}
	return true;
}

//-------------------------------------------------------------------------
void CGameRulesProgressiveObjectives::SetObjectivesEnabled( TObjectivesVector &pObjectivesVec, int teamId, bool enabled )
{
	int numObjectives = pObjectivesVec.size();
	for (int i = 0; i < numObjectives; ++ i)
	{
		IGameRulesObjective *pObjective = pObjectivesVec[i];
		pObjective->Enable(teamId, enabled);
	}
}

//-------------------------------------------------------------------------
void CGameRulesProgressiveObjectives::OnGameReset()
{
	// Reset causes us to start again
	DoStartGame(true);
	m_isResetting = false;
}

//-------------------------------------------------------------------------
void CGameRulesProgressiveObjectives::ChangeStage( int teamId, int newStageIdx )
{
	TTeamStageMap::iterator it = m_teamStages.find(teamId);
	if (it != m_teamStages.end())
	{
		int previousStage = it->second;
		if (previousStage != -1)
		{
			SetObjectivesEnabled(m_stages[previousStage].m_objectives, teamId, false);
		}
		it->second = newStageIdx;
		if (newStageIdx != -1)
		{
			SetObjectivesEnabled(m_stages[newStageIdx].m_objectives, teamId, true);
		}
		if (gEnv->bServer)
		{
			g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(g_pGame->GetGameRules()->GetEntityId(), PROGRESSIVE_OBJECTIVES_ASPECT);
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesProgressiveObjectives::OnHostMigration(bool becomeServer)
{
	if (!becomeServer)
	{
		// We'll get told about everything again when we reconnect to the server so just do a reset!
		OnStartGame();
	}
	else
	{
		// We're the new server, tell all objectives so they can setup server data (if applicable)
		int numStages = m_stages.size();
		for (int stageIdx = 0; stageIdx < numStages; ++ stageIdx)
		{
			SObjectivesStage *pStage = &m_stages[stageIdx];
			int numObjectives = pStage->m_objectives.size();
			for (int objectiveIdx = 0; objectiveIdx < numObjectives; ++ objectiveIdx)
			{
				IGameRulesObjective *pObjective = pStage->m_objectives[objectiveIdx];
				pObjective->OnHostMigration(becomeServer);
			}
		}
	}
}

//-------------------------------------------------------------------------
bool CGameRulesProgressiveObjectives::HasCompleted( int teamId )
{
	if (teamId == 1 || teamId == 2)
	{
		return m_finished[teamId - 1];
	}
	return false;
}

//-------------------------------------------------------------------------
void CGameRulesProgressiveObjectives::OnActionEvent( const SActionEvent& event )
{
	switch(event.m_event)
	{
	case eAE_resetBegin:
		m_isResetting = true;
		break;
	}
}
