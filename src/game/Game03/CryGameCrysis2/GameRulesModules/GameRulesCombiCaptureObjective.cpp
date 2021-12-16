/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of a "combination capture" objective.
		One team's goal is to be in proximity of one or more of multiple "capture"
		points for a specified combined duration of time.
		The other team must prevent them.

	-------------------------------------------------------------------------
	History:
	- 16:12:2009  : Created by Thomas Houghton
	- 03:03:2009  : Refactored to use HoldObjectiveBase by Colin Gulliver

*************************************************************************/

/*************************************************************************
	TODO
	----
	+ messages
	+ cache round number and attackingteam at round start, to get around messiness inbetween rounds when that information changes before everything else resets (can also get rid of the IsRestarting() clause in a couple of the asserts too)

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesCombiCaptureObjective.h"
#include "IXml.h"
#include "GameRules.h"
#include "Player.h"
#include "Utility/CryWatch.h"
#include "Utility/StringUtils.h"
#include "HUD/HUD.h"
#include "HUD/UI/UISimpleBar.h" // TODO : remove in favor of sending a HUDEvent
#include "GameRulesModules/IGameRulesScoringModule.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"
#include "GameRulesModules/IGameRulesRoundsModule.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#if NUM_ASPECTS > 8
	#define COMBICAPTURE_OBJECTIVE_STATE_ASPECT		eEA_GameServerB
#else
	#define COMBICAPTURE_OBJECTIVE_STATE_ASPECT		eEA_GameServerStatic
#endif

#define GET_COMBI_CAPTURE_ENTITY	\
	SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity *>(pDetails->m_pAdditionalData);	\
	if (!pCaptureEntity)	\
	{	\
		CRY_ASSERT(pCaptureEntity);	\
		CryLog("CGameRulesCombiCaptureObjective failed to find capture entity from hold entity"); \
		return;	\
	}

#define GET_COMBI_CAPTURE_ENTITY_RET(returnValue)	\
	SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity *>(pDetails->m_pAdditionalData);	\
	if (!pCaptureEntity)	\
	{	\
		CRY_ASSERT(pCaptureEntity);	\
		CryLog("CGameRulesCombiCaptureObjective failed to find capture entity from hold entity"); \
		return returnValue;	\
	}


//------------------------------------------------------------------------
CGameRulesCombiCaptureObjective::CGameRulesCombiCaptureObjective()
{
	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		m_additionalInfo[i].Reset();
	}

	m_ourCapturePoint = EGRMO_Unknown;
	m_theirCapturePoint = EGRMO_Unknown;
	m_usCapturingPoint = EGRMO_Unknown;
	m_themCapturingPoint = EGRMO_Unknown;

	m_iconPriority = 0;
	m_numActiveEntities = 0;

	m_attackingTeamId = 1;

	m_combiProgress = 0.f;
	m_goalCombiCaptureTime = 0.01f;

	m_contestable = false;
	m_scoringEnabled[0] = false;
	m_scoringEnabled[1] = false;

	m_useIcons = false;
	m_allowMultiPlayerCaptures = false;

	m_capturingAudio[0] = 0;
	m_capturedAudio[0] = 0;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	pGameRules->RegisterRoundsListener(this);
	pGameRules->AddGameRulesListener(this);
}

//------------------------------------------------------------------------
CGameRulesCombiCaptureObjective::~CGameRulesCombiCaptureObjective()
{
	if (gEnv->bServer)
	{
		CGameRules *pGameRules = g_pGame->GetGameRules();
		if (pGameRules)
		{
			pGameRules->UnRegisterRoundsListener(this);
			pGameRules->RemoveGameRulesListener(this);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::Init( XmlNodeRef xml )
{
	BaseType::Init(xml);

	if (xml->getAttr("goalCombiCaptureTime", m_goalCombiCaptureTime))
	{
		CryLog("CGameRulesCombiCaptureObjective::Init, goal combi capture time set to %f", m_goalCombiCaptureTime);
		m_goalCombiCaptureTime = MAX(0.01f, m_goalCombiCaptureTime);
	}

	if (xml->getAttr("contestable", m_contestable))
	{
		CryLog("CGameRulesCombiCaptureObjective::Init, contestable set to %d", m_contestable);
	}

	if (xml->getAttr("allowMultiPlayerCaptures", m_allowMultiPlayerCaptures))
	{
		CryLog("CGameRulesCombiCaptureObjective::Init, allowMultiPlayerCaptures set to %d", m_allowMultiPlayerCaptures);
	}

	int numChildren = xml->getChildCount();
	for (int childIdx = 0; childIdx < numChildren; ++ childIdx)
	{
		const XmlNodeRef&  xmlChild = xml->getChild(childIdx);
		const char*  tagName = xmlChild->getTag(); 
		if (!stricmp(tagName, "Icons"))
		{
			CRY_ASSERT_MESSAGE(!m_useIcons, "CombiCaptureObjective xml contains more than one 'Icons' node, we only support one");
			m_useIcons = true;

			xmlChild->getAttr("priority", m_iconPriority);

			// TODO: Read strings, turn strings => numerical IDs (enum/hash/whatever)
			m_ourCapturePoint = (EGameRulesMissionObjectives)atoi(xmlChild->getAttr("ourCapturePoint"));
			m_theirCapturePoint = (EGameRulesMissionObjectives)atoi(xmlChild->getAttr("theirCapturePoint"));
			m_usCapturingPoint = (EGameRulesMissionObjectives)atoi(xmlChild->getAttr("usCapturingPoint"));
			m_themCapturingPoint = (EGameRulesMissionObjectives)atoi(xmlChild->getAttr("themCapturingPoint"));

			m_shouldShowIconFunc.Format("%s", xmlChild->getAttr("checkFunc"));

			CryLog("CGameRulesCombiCaptureObjective::Init, using on-screen icons [%i %i %i %i]", m_ourCapturePoint, m_theirCapturePoint, m_usCapturingPoint, m_themCapturingPoint);

			g_pGame->GetGameRules()->RegisterTeamChangedListener(this);
		}
		else if(strcmp(tagName,"Audio") == 0)
		{
			if(xmlChild->haveAttr("capturing"))
			{
				cry_strncpy(m_capturingAudio, xmlChild->getAttr("capturing"), k_maxAudioLength);
			}
			if(xmlChild->haveAttr("capturedLoop"))
			{
				cry_strncpy(m_capturedAudio, xmlChild->getAttr("capturedLoop"), k_maxAudioLength);
			}
		}
		else if(strcmp(tagName,"AudioSignal") == 0)
		{
			ReadAudioSignal(xmlChild, "capturingTeam", &m_capturingSignalTeam);
			ReadAudioSignal(xmlChild, "capturingOpponent", &m_capturingSignalOpponent);
		}
		else
		{
			GameWarning("Unrecognised/unsupported node '%s' in XML.", tagName);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::ReadAudioSignal(XmlNodeRef node, const char* name, CAudioSignalPlayer* signalPlayer)
{
	if(node->haveAttr(name))
	{
		const static int k_maxAudioSignalLength = 32;
		char signalName[k_maxAudioSignalLength];
		cry_strncpy(signalName, node->getAttr(name), k_maxAudioSignalLength);
		signalPlayer->SetSignal(signalName);
	}
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::UpdateIcon(SHoldEntityDetails * pDetails, bool force)
{
	GET_COMBI_CAPTURE_ENTITY;

	EGameRulesMissionObjectives requestedIcon = GetIcon(pDetails);

	if (requestedIcon != pCaptureEntity->m_currentIcon || pCaptureEntity->m_needIconUpdate || force)
	{
		if (requestedIcon != -1)
		{
			CRY_TODO(23,02,2010, "[AS HUD] here's where the objective icons above the terminals are created/changed, and where the 'CAPTURE/DEFEND' text above the objective icons can be set accordingly");

			CCCPOINT(CombiCaptureObj_SetNewIcon);

			SHUDEvent newMissionObjective(eHUDEvent_OnNewObjective);
			newMissionObjective.ReserveData(4);
			newMissionObjective.AddData( static_cast<int>(pDetails->m_id) ); /*(EntityId)*/
			newMissionObjective.AddData( requestedIcon ); /*(EGameRulesMissionObjectives)*/ 
			newMissionObjective.AddData( 0.f );
			newMissionObjective.AddData( m_iconPriority );
			CHUD::CallEvent(newMissionObjective);

			CRY_TODO(09, 10, 2009, "Move to happen only once when CaptureEntities are created because the hud doesn't exist at that point currently");
			SHUDEvent hudevent(eHUDEvent_AddEntity);
			hudevent.AddData(SHUDEventData((int)pDetails->m_id));
			CHUD::CallEvent(hudevent);
		}
		else
		{
			SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
			newRemoveObjective.ReserveData(2);
			newRemoveObjective.AddData( static_cast<int>(pDetails->m_id) ); /*(EntityId)*/
			newRemoveObjective.AddData( m_iconPriority ); /*(EntityId)*/
			CHUD::CallEvent(newRemoveObjective);
		}
		pCaptureEntity->m_currentIcon = requestedIcon;
		pCaptureEntity->m_needIconUpdate = false;
	}
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::Update( float frameTime )
{
	BaseType::Update(frameTime);

	IGameRulesRoundsModule*  pRoundsModule = g_pGame->GetGameRules()->GetRoundsModule();
	CRY_ASSERT(pRoundsModule);
	if (!pRoundsModule)
		return;

	m_attackingTeamId = pRoundsModule->GetPrimaryTeam();
	CRY_ASSERT(m_attackingTeamId > 0);

#if CRY_WATCH_ENABLED
	if (g_pGameCVars->g_CombiCaptureObjective_watchLvl > 0)
	{
		CryWatch("[CGameRulesCombiCaptureObjective::Update()]");
		CryWatch(" Round num: %d (of %d)", pRoundsModule->GetRoundNumber(), (pRoundsModule->GetRoundNumber() + pRoundsModule->GetRoundsRemaining()));

		if (g_pGameCVars->g_CombiCaptureObjective_watchLvl > 1)
		{
			CryWatch(" Capture ents:");
		}
	}
#endif

	bool capturing = false;
	bool localPlayerCapturing = false;

	int currActiveIndex = -1;
	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SHoldEntityDetails *pDetails = &m_entities[i];
		if (pDetails->m_id)
		{
			GET_COMBI_CAPTURE_ENTITY;
			++ currActiveIndex;

#if CRY_WATCH_ENABLED
			if (g_pGameCVars->g_CombiCaptureObjective_watchLvl > 1)
			{
				IEntity *pEnt = gEnv->pEntitySystem->GetEntity(pDetails->m_id);
				CryWatch("  %d: '%s' eid %d controlledBy %d", i, (pEnt?pEnt->GetName():"!"), pDetails->m_id, pDetails->m_controllingTeamId);
			}
#endif

			if ((pDetails->m_controllingTeamId == m_attackingTeamId) || (!m_contestable && (pDetails->m_controllingTeamId == CONTESTED_TEAM_ID)))
			{
				UpdateCaptureProgress(pDetails, frameTime);
				capturing = true;
				localPlayerCapturing |= pDetails->m_localPlayerIsWithinRange;
				if (!pCaptureEntity->m_capturing)
				{
					ClSiteStartCapturing(pDetails);
					pCaptureEntity->m_capturing = true;
				}
			}
			else
			{
				pCaptureEntity->m_capturing = false;
			}

			// Do HUD stuff
			if (gEnv->bClient)
			{
				ClUpdateSiteHUD(pDetails, currActiveIndex);
			}
		}
	}

	if (gEnv->bClient)
	{
		ClUpdateMainHUD(capturing, localPlayerCapturing);
	}
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::ClUpdateMainHUD(const bool siteBeingCaptured, const bool localPlayerIsCapturing)
{
	CRY_TODO(23,02,2010, "[AS HUD] Replace with download progress bar in score element");
	if (CUISimpleBar* captureBar = CUISimpleBar::GetInstanceWithName("CaptureProgressBar"))
	{
		captureBar->Set(clamp(m_combiProgress, 0.f, 1.f));

		ColorF bg;
		ColorF fg;
		ColorF f1(0.0f,0.0f,0.5f,0.4f);
		ColorF f2(1.0f,0.0f,0.0f,0.6f);

		const int playerTeamId = g_pGame->GetGameRules()->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());

		if (((playerTeamId == m_attackingTeamId) && siteBeingCaptured) || ((playerTeamId != m_attackingTeamId) && !siteBeingCaptured))
		{
			// local player attacking and a site is being captured or local player is defending and no sites are being captured
			bg.set(0.0f,0.0f,0.5f,0.4f);
			fg.set(0.0f,0.0f,1.0f,1.0f);
		}
		else
		{
			bg.set(0.5f,0.0f,0.0f,0.4f);
			fg.set(1.0f,0.0f,0.0f,1.0f);
		}

		captureBar->SetColours(bg,fg,f1,f2);
	}

	if (localPlayerIsCapturing)
	{
		CRY_TODO(23,02,2010, "[AS HUD] Replace with downloading prompt");
		CryWatch("Downloading ...");
	}
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::ClUpdateSiteHUD(SHoldEntityDetails *pDetails, const int currActiveIndex)
{
	GET_COMBI_CAPTURE_ENTITY;

	UpdateIcon(pDetails, false);

	if (pDetails->m_controllingTeamId == m_attackingTeamId)
	{
		SHUDEvent siteIsBeingCaptured;
		siteIsBeingCaptured.eventType = eHUDEvent_OnSiteBeingCaptured;
		siteIsBeingCaptured.eventIntData = currActiveIndex;
		siteIsBeingCaptured.eventIntData2 = (int) pDetails->m_id;
		siteIsBeingCaptured.eventIntData3 = (pDetails->m_localPlayerIsWithinRange ? 1 : 0);
		siteIsBeingCaptured.eventFloatData = m_combiProgress;
		siteIsBeingCaptured.eventFloat2Data = m_combiProgress;
		CHUD::CallEvent(siteIsBeingCaptured);
	}
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::OnStartGame()
{
	BaseType::OnStartGame();
	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SHoldEntityDetails *pDetails = &m_entities[i];
		if (pDetails->m_id)
		{
			GET_COMBI_CAPTURE_ENTITY;
			pCaptureEntity->m_needIconUpdate = true;
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::OnInsideStateChanged(SHoldEntityDetails *pDetails)
{
	GET_COMBI_CAPTURE_ENTITY;

	pCaptureEntity->m_needIconUpdate = true;
}

//------------------------------------------------------------------------
bool CGameRulesCombiCaptureObjective::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	BaseType::NetSerialize(ser, aspect, profile, flags);
	if (aspect == COMBICAPTURE_OBJECTIVE_STATE_ASPECT)
	{
		float progress = m_combiProgress;
		ser.Value("combiProgress", progress, 'unit');
		if (ser.IsReading() && (m_combiProgress != progress))
		{
			m_combiProgress = progress;

			SHUDEvent progressUpdate(eHUDEvent_OnOverallCaptureProgressUpdate);
			progressUpdate.AddData(m_combiProgress);
			CHUD::CallEvent(progressUpdate);
		}
	}
	
	return true;
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::OnNewHoldEntity(SHoldEntityDetails *pDetails, int index)
{
	CRY_ASSERT(index < HOLD_OBJECTIVE_MAX_ENTITIES);

	SCaptureEntity *pCaptureEntity = &m_additionalInfo[index];

	pDetails->m_pAdditionalData = pCaptureEntity;

	m_numActiveEntities ++;
	if (gEnv->bClient)
	{
		SHUDEvent activeChanged;
		activeChanged.eventType = eHUDEvent_OnCaptureObjectiveNumChanged;
		activeChanged.AddData(SHUDEventData(m_numActiveEntities));
		activeChanged.AddData(SHUDEventData(m_numActiveEntities - 1));
		activeChanged.AddData(SHUDEventData(index));
		activeChanged.AddData(SHUDEventData((int)pDetails->m_id));
		CHUD::CallEvent(activeChanged);
	}

	OnInsideStateChanged(pDetails);
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::OnRemoveHoldEntity(SHoldEntityDetails *pDetails)
{
	GET_COMBI_CAPTURE_ENTITY;
	pCaptureEntity->Reset();

	m_numActiveEntities --;
	if (gEnv->bClient)
	{
		SHUDEvent activeChanged;
		activeChanged.eventType = eHUDEvent_OnCaptureObjectiveNumChanged;
		activeChanged.AddData(SHUDEventData(m_numActiveEntities));
		activeChanged.AddData(SHUDEventData(m_numActiveEntities + 1));
		CHUD::CallEvent(activeChanged);

		SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
		newRemoveObjective.ReserveData(2);
		newRemoveObjective.AddData( static_cast<int>(pDetails->m_id) ); /*(EntityId)*/
		newRemoveObjective.AddData( m_iconPriority ); /*(EntityId)*/
		CHUD::CallEvent(newRemoveObjective);
	}
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::EnableScoring( int teamId, bool enable )
{
	if (teamId == 1 || teamId == 2)
	{
		m_scoringEnabled[teamId - 1] = enable;
	}
}

//------------------------------------------------------------------------
bool CGameRulesCombiCaptureObjective::IsComplete( int teamId )
{
	CRY_ASSERT(gEnv->bServer);

	IGameRulesRoundsModule* pRoundsModule = g_pGame->GetGameRules()->GetRoundsModule();
	if (pRoundsModule && !pRoundsModule->IsInProgress())
		return false;

	CGameRules*  pGameRules = g_pGame->GetGameRules();

	bool  isComplete = false;

	if (teamId != m_attackingTeamId)
	{
		// TODO this could perhaps be improved by hooking into OnEntityKilled and keeping track of how many attacking team-members are alive there instead
		isComplete = true;
		IGameRulesPlayerStatsModule*  pPlayStatsMo = pGameRules->GetPlayerStatsModule();
		CRY_ASSERT(pPlayStatsMo);
		IGameRulesSpawningModule*  pSpawnMo = pGameRules->GetSpawningModule();
		CRY_ASSERT(pSpawnMo);
		const int  spawnNumLives = pSpawnMo->GetNumLives();
		CRY_ASSERT_MESSAGE(g_pGameCVars->g_timelimitextratime <= 0.f, "Sudden Death / Extra Time not supported by this module yet. (The for-loop below will have to change for that.)");
		int  numAttackersSpawned = 0;
		int  numStats = pPlayStatsMo->GetNumPlayerStats();
		for (int i=0; i<numStats; i++)
		{
			const SGameRulesPlayerStat*  s = pPlayStatsMo->GetNthPlayerStats(i);
			if (s->flags & SGameRulesPlayerStat::PLYSTATFL_HASSPAWNEDTHISROUND)
			{
				if (pGameRules->GetTeam(s->playerId) != teamId)
				{
					numAttackersSpawned++;
					if (s->deathsThisRound < spawnNumLives)
					{
						isComplete = false;
						break;
					}
				}
			}
		}
		CRY_FIXME(5,2,2010, "Not sure if this is going to be correct for the only attacker leaving the game - that should win it for the defenders");
		if (numAttackersSpawned == 0)
		{
			isComplete = false;
		}
		if (isComplete)
		{
			CCCPOINT(CombiCaptureObj_SvDefendersWonRound_KilledAll);
			if (m_scoringEnabled[teamId - 1])
			{
				if (IGameRulesScoringModule* pScoringModule=pGameRules->GetScoringModule())
				{
					pScoringModule->OnTeamScoringEvent(teamId, EGRST_CaptureObjectivesDefended);
				}
			}
		}
	}
	else
	{
		if (m_combiProgress >= 1.f)
		{
			CCCPOINT(CombiCaptureObj_SvAttackersWonRound);
			if (m_scoringEnabled[teamId - 1])
			{
				if (IGameRulesScoringModule* pScoringModule=pGameRules->GetScoringModule())
				{
					pScoringModule->OnTeamScoringEvent(m_attackingTeamId, EGRST_CaptureObjectiveTaken);
				}
			}
			isComplete = true;
		}
	}
	return isComplete;
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::UpdateCaptureProgress(SHoldEntityDetails *pDetails, float frameTime)
{
	IGameRulesRoundsModule* pRoundsModule = g_pGame->GetGameRules()->GetRoundsModule();
	if (pRoundsModule && !pRoundsModule->IsInProgress())
		return;

	GET_COMBI_CAPTURE_ENTITY;

	const float  timeToCapture = m_goalCombiCaptureTime;
	// [tlh] if we ever wanted per-entity capture multipliers, this is where they'd go (basically fudge timeToCapture)

	float multiplier = (1.f / timeToCapture);

	if (m_allowMultiPlayerCaptures)
	{
		int insideCount = pDetails->m_insideCount[m_attackingTeamId - 1];
		CRY_ASSERT(insideCount > 0);
		multiplier *= insideCount;
	}

	float progress = (multiplier * frameTime);

	m_combiProgress += progress;
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::ClSiteStartCapturing(SHoldEntityDetails *pDetails)
{
	CCCPOINT(CombiCaptureObj_ClSiteCaptureStarted);

	//Feedback
	CGameRules *pGameRules = g_pGame->GetGameRules();
	CRY_ASSERT(m_attackingTeamId > 0);
	int localTeam = pGameRules->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());
	if (localTeam == m_attackingTeamId)
	{
		CCCPOINT(CombiCaptureObj_SiteStartCapturingByFriendlyTeam);
		m_capturingSignalTeam.Play();
	}
	else
	{
		CCCPOINT(CombiCaptureObj_SiteStartCapturingByEnemyTeam);
		m_capturingSignalOpponent.Play();
	}
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::OnChangedTeam( EntityId entityId, int oldTeamId, int newTeamId )
{
	BaseType::OnChangedTeam(entityId, oldTeamId, newTeamId);
	if ((g_pGame->GetIGameFramework()->GetClientActorId() == entityId) && newTeamId)
	{
		// Local player has changed teams, reset icons
		int currActiveIndex = -1;
		for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
		{
			SHoldEntityDetails *pDetails = &m_entities[i];
			if (pDetails->m_id)
			{
				GET_COMBI_CAPTURE_ENTITY;
				pCaptureEntity->m_needIconUpdate = true;
			}
		}
	}
}

//------------------------------------------------------------------------
// IGameRulesRoundsListener
void CGameRulesCombiCaptureObjective::OnRoundStart()
{
	m_combiProgress = 0.f;
	if (gEnv->bServer)
	{
		g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(g_pGame->GetGameRules()->GetEntityId(), COMBICAPTURE_OBJECTIVE_STATE_ASPECT);
	}
}

//------------------------------------------------------------------------
EGameRulesMissionObjectives CGameRulesCombiCaptureObjective::GetIcon(SHoldEntityDetails *pDetails)
{
	GET_COMBI_CAPTURE_ENTITY_RET(EGRMO_Unknown);

	EGameRulesMissionObjectives requestedIcon = pCaptureEntity->m_currentIcon;	// Default to current icon

	bool iconAllowed = true;
	if (!m_shouldShowIconFunc.empty())
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(pDetails->m_id);
		if (pEntity)
		{
			IScriptTable *pEntityScript = pEntity->GetScriptTable();
			HSCRIPTFUNCTION iconCheckFunc;
			if (pEntityScript && pEntityScript->GetValue(m_shouldShowIconFunc.c_str(), iconCheckFunc))
			{
				IScriptSystem *pScriptSystem = gEnv->pScriptSystem;
				bool result = false;
				if (Script::CallReturn(pScriptSystem, iconCheckFunc, pEntityScript, result))
				{
					if (!result)
					{
						requestedIcon = EGRMO_Unknown;
						iconAllowed = false;
					}
				}
			}
		}
	}

	if (iconAllowed)
	{
		CGameRules *pGameRules = g_pGame->GetGameRules();
		int localTeamId = pGameRules->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());
		float serverTime = pGameRules->GetServerTime();

		if (pCaptureEntity->m_capturing)
		{
			requestedIcon = (localTeamId == m_attackingTeamId) ? m_usCapturingPoint : m_themCapturingPoint;
		}
		else
		{
			requestedIcon = (localTeamId == m_attackingTeamId) ? m_theirCapturePoint : m_ourCapturePoint;
		}
	}

	return requestedIcon;
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::OnOwnClientEnteredGame()
{
	BaseType::OnOwnClientEnteredGame();
	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SHoldEntityDetails *pDetails = &m_entities[i];
		if (pDetails->m_id)
		{
			GET_COMBI_CAPTURE_ENTITY;
			pCaptureEntity->m_needIconUpdate = true;
		}
	}
}

//------------------------------------------------------------------------
// SGameRulesListener (interface)
void CGameRulesCombiCaptureObjective::SvOnTimeLimitExpired()
{
	CryLog("[tlh] @ CGameRulesCombiCaptureObjective::SvOnTimeLimitExpired()");
	CRY_ASSERT(gEnv->bServer);

	CGameRules*  pGameRules = g_pGame->GetGameRules();

	CRY_ASSERT(m_attackingTeamId > 0);
	int  defendingTeamId = (3 - m_attackingTeamId);

	if (m_scoringEnabled[defendingTeamId - 1])
	{
		if (IGameRulesScoringModule* pScoringModule=pGameRules->GetScoringModule())
		{
			pScoringModule->OnTeamScoringEvent(defendingTeamId, EGRST_CaptureObjectivesDefended);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCombiCaptureObjective::OnClientConnect( int channelId, bool isReset, EntityId playerId )
{
	BaseType::OnClientConnect(channelId, isReset, playerId);
	// Need to mark aspect as changes so that the network collects the latest capture progress amounts
	g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(g_pGame->GetGameRules()->GetEntityId(), COMBICAPTURE_OBJECTIVE_STATE_ASPECT);
}
