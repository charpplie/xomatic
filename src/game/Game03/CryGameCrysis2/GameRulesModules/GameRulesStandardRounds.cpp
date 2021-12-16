/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 26:10:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesStandardRounds.h"
#include "IXml.h"
#include "GameRules.h"
#include "GameRulesModules/IGameRulesObjectivesModule.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"
#include "GameRulesModules/IGameRulesPlayerSetupModule.h"
#include "GameRulesModules/IGameRulesScoringModule.h"
#include "GameRulesModules/IGameRulesVictoryConditionsModule.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "HUD/HUD.h"
#include "HUD/UI/UIButtonPromptRegion.h"

#define GAMERULES_STANDARD_ROUNDS_DELAY_TIMER_LENGTH	5000.f

static TKeyValuePair<EGameOverReason,const char*>
g_gameOverReasons[] = {
	{EGOR_Unknown,							"Unknown"},
	{EGOR_TimeLimitReached,			"TimeLimitReached"},
	{EGOR_ObjectivesCompleted,	"ObjectivesCompleted"},
	{EGOR_NoLivesRemaining,			"NoLivesRemaining"},
};

#if NUM_ASPECTS > 8
	#define STANDARD_ROUNDS_ASPECT				eEA_GameServerC
#else
	#define STANDARD_ROUNDS_ASPECT				eEA_GameServerStatic
#endif

//-------------------------------------------------------------------------
CGameRulesStandardRounds::CGameRulesStandardRounds()
{
	CryLog("CGameRulesStandardRounds::CGameRulesStandardRounds()");

	m_roundNumber = 0;
	m_resetScores = false;
	m_previousRoundWinnerTeamId = 0;
	m_previousRoundWinnerEntityId = 0;
	m_previousRoundWinReason = EGOR_Unknown;
	m_newRoundStartTime = -1.f;
	m_roundState = eGRRS_InProgress;

	m_endOfRoundStringCount = 0;
}

//-------------------------------------------------------------------------
CGameRulesStandardRounds::~CGameRulesStandardRounds()
{
	CryLog("CGameRulesStandardRounds::~CGameRulesStandardRounds()");

	if (gEnv->pConsole)
	{
		gEnv->pConsole->RemoveCommand("g_nextRound");
	}

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->UnRegisterTeamChangedListener(this);
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::Init( XmlNodeRef xml )
{
	CryLog("CGameRulesStandardRounds::Init()");

	int resetScores = 0;
	if (xml->getAttr("resetScores", resetScores))
	{
		m_resetScores = (resetScores != 0);
	}

	int numChildren = xml->getChildCount();
	for (int i = 0; i < numChildren; ++ i)
	{
		XmlNodeRef xmlChild = xml->getChild(i);
		if (!stricmp(xmlChild->getTag(), "PrimaryTeam"))
		{
			ReadEntityClasses(m_primaryTeamEntities, xmlChild, m_primaryStartRoundString);
		}
		else if (!stricmp(xmlChild->getTag(), "SecondaryTeam"))
		{
			ReadEntityClasses(m_secondaryTeamEntities, xmlChild, m_secondaryStartRoundString);
		}
		else if (!stricmp(xmlChild->getTag(), "EndStrings"))
		{
			const char *pString = 0;

			// Defaults
			if (xmlChild->getAttr("winMessage", &pString))
			{
				m_endOfRoundStringsDefault.m_roundWinMessage.Format("@%s", pString);
			}
			if (xmlChild->getAttr("loseMessage", &pString))
			{
				m_endOfRoundStringsDefault.m_roundLoseMessage.Format("@%s", pString);
			}
			if (xmlChild->getAttr("drawMessage", &pString))
			{
				m_endOfRoundStringsDefault.m_roundDrawMessage.Format("@%s", pString);
			}

			// Custom end of round strings
			int numEORChildren = xmlChild->getChildCount();
			for (int j = 0; j < numEORChildren; ++ j)
			{
				XmlNodeRef xmlEORChild = xmlChild->getChild(j);
				if (!stricmp(xmlEORChild->getTag(), "Strings"))
				{
					if (xmlEORChild->getAttr("reason", &pString))
					{
						EGameOverReason reason = EGOR_Unknown;
						reason = KEY_BY_VALUE(pString, g_gameOverReasons);

						if (m_endOfRoundStringCount < MAX_END_OF_ROUND_STRINGS)
						{
							SOnEndRoundStrings &strings = m_endOfRoundStrings[m_endOfRoundStringCount];
							bool found = false;
							for (int k = 0; (k < MAX_END_OF_ROUND_STRINGS) && (k < m_endOfRoundStringCount); ++k )
							{
								if (reason == m_endOfRoundStrings[k].m_reason)
								{
									CryLog("CGameRulesStandardRounds::Init() EndOfRound strings already exist for reason:%s", pString);
									strings = m_endOfRoundStrings[k];
									found = true;
									break;
								}
							}

							if (!found)
							{
								CryLog("CGameRulesStandardRounds::Init() Added EndOfRound strings for reason:%s", pString);
								strings.m_reason = reason;
								m_endOfRoundStringCount++;
							}

							if (xmlEORChild->getAttr("winMessage", &pString))
							{
								strings.m_roundWinMessage.Format("@%s", pString);
							}
							if (xmlEORChild->getAttr("loseMessage", &pString))
							{
								strings.m_roundLoseMessage.Format("@%s", pString);
							}
							if (xmlEORChild->getAttr("drawMessage", &pString))
							{
								strings.m_roundDrawMessage.Format("@%s", pString);
							}
						}
						else
						{
							CRY_ASSERT_MESSAGE(0, "CGameRulesStandardRounds::Init() Reached max number of EndOfRound strings. Increase MAX_END_OF_ROUND_STRINGS if more are required.");
						}
					}
				}
			}
		}
		else if (!stricmp(xmlChild->getTag(), "StartStrings"))
		{
			const char *pString = 0;
			if (xmlChild->getAttr("startNewRound", &pString))
			{
				m_primaryStartRoundString.Format("@%s", pString);
				m_secondaryStartRoundString.Format("@%s", pString);
			}
		}
	}

	g_pGame->GetGameRules()->RegisterTeamChangedListener(this);

	REGISTER_COMMAND("g_nextRound", CmdNextRound, VF_RESTRICTEDMODE, "Starts the next game round.");
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::ReadEntityClasses( TEntityDetailsVec &classesVec, XmlNodeRef xml, TFixedString &startRoundString)
{
	int numChildren = xml->getChildCount();
	for (int i = 0; i < numChildren; ++ i)
	{
		XmlNodeRef xmlChild = xml->getChild(i);
		if (!stricmp(xmlChild->getTag(), "Entity"))
		{
			SEntityDetails entityDetails;
			bool valid = false;

			const char *pClassName = 0;
			if (xmlChild->getAttr("class", &pClassName))
			{
				const IEntityClass *pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(pClassName);
				if (pClass)
				{
					entityDetails.m_pEntityClass = pClass;
					valid = true;
				}
			}
			const char *pFuncName = 0;
			if (xmlChild->getAttr("luaActivateFunc", &pFuncName))
			{
				entityDetails.m_activateFunc.Format("%s", pFuncName);
			}
			if (xmlChild->getAttr("luaDeactivateFunc", &pFuncName))
			{
				entityDetails.m_deactivateFunc.Format("%s", pFuncName);
			}

			entityDetails.m_doRandomSelection = false;
			entityDetails.m_selectCount = 1;

			const char *pSelectType = 0;
			if (xmlChild->getAttr("select", &pSelectType))
			{
				if (!stricmp(pSelectType, "Random"))
				{
					entityDetails.m_doRandomSelection = true;
					xmlChild->getAttr("count", entityDetails.m_selectCount);
				}
			}

			if (valid)
			{
				classesVec.push_back(entityDetails);
			}
		}
		else if (!stricmp(xmlChild->getTag(), "StartStrings"))
		{
			const char *pString = 0;
			if (xmlChild->getAttr("startNewRound", &pString))
			{
				startRoundString.Format("@%s", pString);
			}
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::PostInit()
{
	CryLog("CGameRulesStandardRounds::PostInit()");
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::OnStartGame()
{
	if (gEnv->bServer)
	{
		StartNewRound(true);
	}
}

//-------------------------------------------------------------------------
bool CGameRulesStandardRounds::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (aspect == STANDARD_ROUNDS_ASPECT)
	{
		ser.Value("roundNum", m_roundNumber, 'ui8');
		ser.Value("previousTeamId", m_previousRoundWinnerTeamId, 'team');
		ser.Value("previousEntityId", m_previousRoundWinnerEntityId, 'eid');

		int winReason = (int)m_previousRoundWinReason;
		ser.Value("previousWinReason", winReason, 'ui8');
		if (ser.IsReading())
		{
			m_previousRoundWinReason = EGameOverReason(winReason);
		}

		int state = int(m_roundState);
		ser.Value("state", state, 'ui2');

		if (ser.IsReading())
		{
			ERoundState oldState = m_roundState;
			ERoundState newState = ERoundState(state);

			if (m_roundState != newState)
			{
				CRY_ASSERT(!gEnv->bServer);
				CRY_ASSERT(gEnv->bClient);

				if (newState == eGRRS_Restarting)
				{
					ClDisplayEndOfRoundMessage();
					m_newRoundStartTime = g_pGame->GetGameRules()->GetServerTime() + GAMERULES_STANDARD_ROUNDS_DELAY_TIMER_LENGTH;
				}

				if (newState == eGRRS_InProgress)
				{
					EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
					int localTeam = g_pGame->GetGameRules()->GetTeam(localActorId);
					int primaryTeam = GetPrimaryTeam();

					if (!localTeam || (primaryTeam == localTeam))
					{
						if (!m_primaryStartRoundString.empty())
							CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", m_primaryStartRoundString.c_str(), NULL, MIN(2.f, ((GAMERULES_STANDARD_ROUNDS_DELAY_TIMER_LENGTH / 1000.f) - 1.f)));
					}
					else
					{
						if (!m_secondaryStartRoundString.empty())
							CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", m_secondaryStartRoundString.c_str(), NULL, MIN(2.f, ((GAMERULES_STANDARD_ROUNDS_DELAY_TIMER_LENGTH / 1000.f) - 1.f)));
					}
				}

				CRY_FIXME(25,2,2010, "This code doesn't happen when the 1st round starts (ie. first going from pregame to ingame) - this should be deprecated and most (if not all) usages should be changed over to the OnRoundStart_NotifyListeners func (called below) instead");
				g_pGame->GetGameRules()->ClRoundsNetSerializeReadState_NotifyListeners(newState, m_roundState);

				m_roundState = newState;
			}

			IGameRulesStateModule*  pStateMo = g_pGame->GetGameRules()->GetStateModule();

			if (((oldState == eGRRS_Restarting) && (m_roundState == eGRRS_InProgress)) ||
				((m_roundState == eGRRS_InProgress) && (pStateMo && (pStateMo->GetGameState() == IGameRulesStateModule::EGRS_Reset))))
			{
				// note: the second line in the if() above is to catch the case of starting the 1st round (ie. from pregame to ingame)

				g_pGame->GetGameRules()->OnRoundStart_NotifyListeners();  // for clients

				SHUDEvent  e;
				e.eventType = eHUDEvent_OnRoundStart;
				CHUDEventDispatcher::CallEvent(e);  // for clients
			}
		}
	}
	return true;
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::ClDisplayEndOfRoundMessage()
{
	CRY_TODO(4,2,2010, "End-of-round HUD messages need to be localised and displayed properly.");

	if (!GetRoundsRemaining())
		return;	// End of game message in victory conditions instead.

	EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
	int localTeam = g_pGame->GetGameRules()->GetTeam(localActorId);
	int primaryTeam = GetPrimaryTeam();

	SOnEndRoundStrings &strings = m_endOfRoundStringsDefault;

	for (int i = 0; i < m_endOfRoundStringCount; ++ i)
	{
		if (m_previousRoundWinReason == m_endOfRoundStrings[i].m_reason)
		{
			strings = m_endOfRoundStrings[i];
			break;
		}
	}

	CHUD* pHUD = g_pGame->GetHUD();
	const char* localizedText = "";

	if (m_previousRoundWinnerTeamId)
	{
		static CryFixedStringT<16>  strWinTeamName;
		strWinTeamName.Format("@ui_hud_team_%d", m_previousRoundWinnerTeamId);

		if (localTeam == m_previousRoundWinnerTeamId)
		{
			// Local team won
			CryLog("CGameRulesStandardRounds::ClDisplayEndOfRoundMessage(), local team won");

			if (!strings.m_roundWinMessage.empty())
			{
				localizedText = pHUD->LocalizeString(strings.m_roundWinMessage.c_str(), strWinTeamName.c_str(), NULL);
				CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", localizedText, NULL, MIN(2.f, ((GAMERULES_STANDARD_ROUNDS_DELAY_TIMER_LENGTH / 1000.f) - 1.f)));
			}
		}
		else
		{
			// Local team lost
			CryLog("CGameRulesStandardRounds::ClDisplayEndOfRoundMessage(), local team lost");
			
			if (!strings.m_roundLoseMessage.empty())
			{
				localizedText = pHUD->LocalizeString(strings.m_roundLoseMessage.c_str(), strWinTeamName.c_str(), NULL);
				CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", localizedText, NULL, MIN(2.f, ((GAMERULES_STANDARD_ROUNDS_DELAY_TIMER_LENGTH / 1000.f) - 1.f)));
			}
		}
	}
	else if (m_previousRoundWinnerEntityId)
	{
		IEntity *pPlayer = gEnv->pEntitySystem->GetEntity(m_previousRoundWinnerEntityId);
		if (m_previousRoundWinnerEntityId == localActorId)
		{
			// Local player won
			CryLog("CGameRulesStandardRounds::ClDisplayEndOfRoundMessage(), local player won");
			
			if (!strings.m_roundWinMessage.empty())
			{
				localizedText = pHUD->LocalizeString( strings.m_roundWinMessage.c_str(), pPlayer ? pPlayer->GetName() : "" );
				CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", localizedText, NULL, MIN(2.f, ((GAMERULES_STANDARD_ROUNDS_DELAY_TIMER_LENGTH / 1000.f) - 1.f)));
			}
		}
		else
		{
			// Local player lost
			CryLog("CGameRulesStandardRounds::ClDisplayEndOfRoundMessage(), local player lost");
			
			if (!strings.m_roundLoseMessage.empty())
			{
				localizedText = pHUD->LocalizeString( strings.m_roundLoseMessage.c_str(), pPlayer ? pPlayer->GetName() : "" );
				CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", localizedText, NULL, MIN(2.f, ((GAMERULES_STANDARD_ROUNDS_DELAY_TIMER_LENGTH / 1000.f) - 1.f)));
			}
		}
	}
	else
	{
		// Draw
		CryLog("CGameRulesStandardRounds::ClDisplayEndOfRoundMessage(), draw");
		
		if (!strings.m_roundDrawMessage.empty())
			CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", strings.m_roundDrawMessage.c_str(), NULL, MIN(2.f, ((GAMERULES_STANDARD_ROUNDS_DELAY_TIMER_LENGTH / 1000.f) - 1.f)));
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::OnEndGame(int teamId, EntityId playerId, EGameOverReason reason)
{
	if (gEnv->bServer && (m_roundState == eGRRS_InProgress))
	{
		m_previousRoundWinnerTeamId = teamId;
		m_previousRoundWinnerEntityId = playerId;
		m_previousRoundWinReason = reason;

		CGameRules *pGameRules = g_pGame->GetGameRules();
		int newTeamRoundScore = pGameRules->GetTeamRoundScore(teamId);
		pGameRules->SetTeamRoundScore(teamId, ++newTeamRoundScore);

		EndRound();

		if (gEnv->bClient)
		{
			ClDisplayEndOfRoundMessage();
		}
	}
}

//-------------------------------------------------------------------------
int CGameRulesStandardRounds::GetRoundNumber() // Starts at 0
{
	return m_roundNumber;
}

//-------------------------------------------------------------------------
int CGameRulesStandardRounds::GetRoundsRemaining()
{
	return (g_pGame->GetGameRules()->GetRoundLimit() - m_roundNumber);
}

//-------------------------------------------------------------------------
int CGameRulesStandardRounds::GetPrimaryTeam()
{
	return (m_roundNumber % 2) + 1;		// Primary team is 1 for even round numbers, 2 for odd round numbers
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::StartNewRound( bool isReset )
{
	CGameRules *pGameRules = g_pGame->GetGameRules();

	if (isReset)
	{
		m_roundNumber = 0;
	}

	pGameRules->SvCacheRoundStartTeamScores();

	if (m_resetScores)
	{
		int startTeamScore = 0;
		if (pGameRules->GetScoringModule())
		{
			startTeamScore = pGameRules->GetScoringModule()->GetStartTeamScore();
		}

		pGameRules->SetTeamsScore(1, startTeamScore);
		pGameRules->SetTeamsScore(2, startTeamScore);
	}

	SetupEntities();

	if (!isReset)
	{
		pGameRules->ResetGameTime();

		if (pGameRules->GetObjectivesModule())
		{
			pGameRules->GetObjectivesModule()->OnGameReset();
		}

		if (IGameRulesPlayerStatsModule* pPlayStatsMod=pGameRules->GetPlayerStatsModule())
		{
			pPlayStatsMod->OnStartNewRound();
		}

		// Reset entities
		IEntityItPtr pIt = gEnv->pEntitySystem->GetEntityIterator();
		IEntity *pEntity = 0;

		pIt->MoveFirst();
		while(pEntity = pIt->Next())
		{
			EntityId entityId = pEntity->GetId();
			const CGameRules::SEntityRespawnData *pRespawnData = pGameRules->GetEntityRespawnData(entityId);
			if (pRespawnData)
			{
				// Check if the entity has been modified (currently only check position and rotation)
				if (!(pEntity->GetWorldPos().IsEquivalent(pRespawnData->position) && pEntity->GetWorldRotation().IsEquivalent(pRespawnData->rotation)))
				{
					IItem *pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(entityId);
					if (pItem && pItem->GetOwnerId())
					{
						if (pItem->IsUsed())
						{
							pItem->StopUse(pItem->GetOwnerId());
						}
						else
						{
							pItem->Drop();
						}
					}
					pGameRules->ScheduleEntityRemoval(entityId, 0.f, false);
					pGameRules->ScheduleEntityRespawn(entityId, true, 0.f);
				}
			}
		}

		pGameRules->FlushEntitySchedules();
	}

	if (IGameRulesSpawningModule* pSpawningModule=pGameRules->GetSpawningModule())
	{
		pSpawningModule->ReviveAllPlayers(isReset);
	}

	if (IGameRulesPlayerSetupModule* pPlaySetupMod=pGameRules->GetPlayerSetupModule())
	{
		pPlaySetupMod->SvOnStartNewRound(isReset);
	}

	IGameRulesStateModule*  pStateMo = pGameRules->GetStateModule();
	bool  onRoundStart = (!pStateMo || (pStateMo->GetGameState() != IGameRulesStateModule::EGRS_PreGame));

	if (gEnv->bClient)
	{
		int primaryTeam = GetPrimaryTeam();
		EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
		int localTeam = g_pGame->GetGameRules()->GetTeam(localActorId);

		if (!localTeam || (primaryTeam == localTeam))
		{
			if (!m_primaryStartRoundString.empty())
				CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", m_primaryStartRoundString.c_str(), NULL, MIN(2.f, ((GAMERULES_STANDARD_ROUNDS_DELAY_TIMER_LENGTH / 1000.f) - 1.f)));
		}
		else
		{
			if (!m_secondaryStartRoundString.empty())
				CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", m_secondaryStartRoundString.c_str(), NULL, MIN(2.f, ((GAMERULES_STANDARD_ROUNDS_DELAY_TIMER_LENGTH / 1000.f) - 1.f)));
		}

		if (onRoundStart)
		{
			CHUDEventDispatcher::CallEvent(SHUDEvent(eHUDEvent_OnRoundStart));  // for server (but not dedicated)
		}
	}

	if (onRoundStart)
	{
		pGameRules->OnRoundStart_NotifyListeners();  // for server (including dedicated)
	}

	m_roundState = eGRRS_InProgress;

	g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(g_pGame->GetGameRules()->GetEntityId(), STANDARD_ROUNDS_ASPECT);
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::EndRound()
{
	CGameRules *pGameRules = g_pGame->GetGameRules();

	++ m_roundNumber;
	if (GetRoundsRemaining())
	{
		if (m_roundState != eGRRS_Restarting)
		{
			m_newRoundStartTime = pGameRules->GetServerTime() + GAMERULES_STANDARD_ROUNDS_DELAY_TIMER_LENGTH;
			m_roundState = eGRRS_Restarting;
		}
	}
	else
	{
		m_roundState = eGRRS_Finished;
	}
	g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(g_pGame->GetGameRules()->GetEntityId(), STANDARD_ROUNDS_ASPECT);
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::SetupEntities()
{
	int primaryTeamId = GetPrimaryTeam();
	int secondaryTeamId = 3 - primaryTeamId;
	CryLog("CGameRulesStandardRounds::SetupEntities(), primaryTeamId=%i", primaryTeamId);

	SetTeamEntities(m_primaryTeamEntities, primaryTeamId);
	SetTeamEntities(m_secondaryTeamEntities, secondaryTeamId);
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::SetTeamEntities( TEntityDetailsVec &classesVec, int teamId)
{
	CGameRules *pGameRules = g_pGame->GetGameRules();

	IEntityItPtr pIt = gEnv->pEntitySystem->GetEntityIterator();
	IEntity *pEntity = 0;

	int numClasses = classesVec.size();
	for (int i = 0; i < numClasses; ++ i)
	{
		SEntityDetails *pEntityDetails = &classesVec[i];

		TEntityIdVec newRoundEntities;

		if ((m_roundNumber % 2) == 0)
		{
			// If this is an even numbered round then we need to select new entities, otherwise we use the same as last time but with different teams

			pIt->MoveFirst();
			while(pEntity = pIt->Next())
			{
				if (pEntityDetails->m_pEntityClass == pEntity->GetClass())
				{
					newRoundEntities.push_back(pEntity->GetId());
				}
			}

			if (pEntityDetails->m_doRandomSelection)
			{
				// If we're doing random selection then we probably don't need all the entities found in the loop above, remove excess ones
				int totalEntities = newRoundEntities.size();
				while (totalEntities > pEntityDetails->m_selectCount)
				{
					int index = cry_rand() % totalEntities;
					TEntityIdVec::iterator it = newRoundEntities.begin() + index;
					newRoundEntities.erase(it);
					-- totalEntities;
				}
			}
		}
		else
		{
			newRoundEntities = pEntityDetails->m_currentEntities;
		}

		// Deactivate all the entities used in previous rounds (even if we're going to reactivate it in a second)
		if (pEntityDetails->m_currentEntities.size())
		{
			DeactivateEntities(pEntityDetails->m_deactivateFunc.c_str(), pEntityDetails->m_currentEntities);
		}

		int totalEntities = newRoundEntities.size();
		for (int i = 0; i < totalEntities; ++ i)
		{
			EntityId entityId = newRoundEntities[i];
			ActivateEntity(entityId, teamId, pEntityDetails->m_activateFunc.c_str(), pEntityDetails->m_currentEntities);
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::ActivateEntity( EntityId entityId, int teamId, const char *pActivateFunc, TEntityIdVec &pEntitiesVec )
{
	g_pGame->GetGameRules()->SetTeam(teamId, entityId);
	if (pActivateFunc && *pActivateFunc)
	{
		CallScript(entityId, pActivateFunc);
	}
	pEntitiesVec.push_back(entityId);
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::DeactivateEntities( const char *pDeactivateFunc, TEntityIdVec &entitiesVec)
{
	CGameRules *pGameRules = g_pGame->GetGameRules();

	int numEntities = entitiesVec.size();
	for (int i = 0; i < numEntities; ++ i)
	{
		EntityId entityId = entitiesVec[i];
		pGameRules->SetTeam(0, entityId);
		if (pDeactivateFunc && *pDeactivateFunc)
		{
			CallScript(entityId, pDeactivateFunc);
		}
	}
	entitiesVec.clear();
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::CallScript( EntityId entityId, const char *pFunction )
{
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(entityId);
	if (pEntity)
	{
		IScriptTable *pScriptTable = pEntity->GetScriptTable();
		if (pScriptTable)
		{
			if (pScriptTable->GetValueType(pFunction) == svtFunction)
			{
				IScriptSystem *pScriptSystem = gEnv->pScriptSystem;
				pScriptSystem->BeginCall(pScriptTable, pFunction);
				pScriptSystem->PushFuncParam(pScriptTable);
				pScriptSystem->EndCall();
			}
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::CmdNextRound( IConsoleCmdArgs *pArgs )
{
	CGameRulesStandardRounds *pRoundsModule = static_cast<CGameRulesStandardRounds*>(g_pGame->GetGameRules()->GetRoundsModule());
	if (pRoundsModule)
	{
		pRoundsModule->EndRound();
		pRoundsModule->StartNewRound(false);
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::OnChangedTeam( EntityId entityId, int oldTeamId, int newTeamId )
{
	if (!gEnv->bServer)		// This stuff has already been done on the server
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(entityId);

		CheckForTeamEntity(pEntity, newTeamId, m_primaryTeamEntities);
		CheckForTeamEntity(pEntity, newTeamId, m_secondaryTeamEntities);
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::CheckForTeamEntity( IEntity *pEntity, int newTeamId, TEntityDetailsVec &entitiesVec )
{
	CryLog("CGameRulesStandardRounds::CheckForTeamEntity, entity '%s' has joined team '%i'", pEntity->GetName(), newTeamId);
	const IEntityClass *pEntityClass = pEntity->GetClass();

	int numEntityTypes = entitiesVec.size();
	for (int i = 0; i < numEntityTypes; ++ i)
	{
		SEntityDetails *pEntityDetails = &entitiesVec[i];
		if (pEntityDetails->m_pEntityClass == pEntityClass)
		{
			if (newTeamId)
			{
				// Activate the entity
				if (!pEntityDetails->m_activateFunc.empty())
				{
					CallScript(pEntity->GetId(), pEntityDetails->m_activateFunc.c_str());
					CryLog("Activating");
				}
			}
			else
			{
				// Deactivate the entity
				if (!pEntityDetails->m_deactivateFunc.empty())
				{
					CallScript(pEntity->GetId(), pEntityDetails->m_deactivateFunc.c_str());
					CryLog("Deactivating");
				}
			}
			break;
		}
	}
}

//-------------------------------------------------------------------------
void CGameRulesStandardRounds::Update( float frameTime )
{
	if (gEnv->bServer)
	{
		if ((m_roundState == eGRRS_Restarting) && (g_pGame->GetGameRules()->GetServerTime() > m_newRoundStartTime))
		{
			StartNewRound(false);
		}
	}
}
