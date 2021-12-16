/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of a capture objective (take and hold)

	-------------------------------------------------------------------------
	History:
	- 21:09:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesCaptureObjective.h"
#include "IXml.h"
#include "GameRules.h"
#include "Player.h"
#include "GameRulesModules/IGameRulesScoringModule.h"
#include "Utility/CryWatch.h"
#include "Utility/StringUtils.h"
#include "HUD/HUD.h"
#include "HUD/UI/UISimpleBar.h" // TODO : remove in favor of sending a HUDEvent
#include "GameRulesModules/IGameRulesSpawningModule.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#if NUM_ASPECTS > 8
	#define CAPTURE_OBJECTIVE_STATE_ASPECT		eEA_GameServerA
#else
	#define CAPTURE_OBJECTIVE_STATE_ASPECT		eEA_GameServerStatic
#endif

//------------------------------------------------------------------------
CGameRulesCaptureObjective::CGameRulesCaptureObjective()
{
	m_capturingEnabled[0] = false;
	m_capturingEnabled[1] = false;
	m_scoringEnabled[0] = false;
	m_scoringEnabled[1] = false;

	m_iconPriority = 0;
	m_maxScoresPerEntity = 0;
	m_bleedRate = 0.f;
	m_scoreTimerLength = 0.f;
	m_useScoreTimer = false;
	m_useBleedRate = false;
	m_useIcons = false;
	m_neutralIcon = EGRMO_Unknown;
	m_stealBlueToRedIcon = EGRMO_Unknown;
	m_stealRedToBlueIcon = EGRMO_Unknown;
	m_initialCaptureBlueIcon = EGRMO_Unknown;
	m_initialCaptureRedIcon = EGRMO_Unknown;
	m_maxCapturesPerEntity = 0;

	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		m_additionalInfo[i].Reset();
	}

#if CAPTURE_OBJECTIVE_ADDITIONAL_DEBUG_CODE
	m_overrideCaptureTime = -1.f;

	IConsole * console = gEnv->pConsole;

	if (console)
	{
		console->Register("gameRules_captureObjective_overrideCaptureTime", & m_overrideCaptureTime, m_overrideCaptureTime);
	}
#endif
}

//------------------------------------------------------------------------
CGameRulesCaptureObjective::~CGameRulesCaptureObjective()
{
#if CAPTURE_OBJECTIVE_ADDITIONAL_DEBUG_CODE
	IConsole * console = gEnv->pConsole;

	if (console)
	{
		console->UnregisterVariable("gameRules_captureObjective_overrideCaptureTime", true);
	}
#endif
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::Init( XmlNodeRef xml )
{
	BaseType::Init(xml);

	if (xml->getAttr("scoreTime", m_scoreTimerLength))
	{
		CryLog("CGameRulesCaptureObjective::Init, using score timer, length=%f", m_scoreTimerLength);
		m_useScoreTimer = true;
	}
	if (xml->getAttr("bleedRate", m_bleedRate))
	{
		CryLog("CGameRulesCaptureObjective::Init, bleed rate set to %f", m_bleedRate);
		m_useBleedRate = true;
	}
	if (xml->getAttr("maxCapturesPerEntity", m_maxCapturesPerEntity))
	{
		CryLog("CGameRulesCaptureObjective::Init, max captures set to %i", m_maxCapturesPerEntity);
	}
	if (xml->getAttr("maxScoresPerEntity", m_maxScoresPerEntity))
	{
		CryLog("CGameRulesCaptureObjective::Init, max scores set to %i", m_maxScoresPerEntity);
	}

	int numChildren = xml->getChildCount();
	for (int childIdx = 0; childIdx < numChildren; ++ childIdx)
	{
		XmlNodeRef xmlChild = xml->getChild(childIdx);
		if (!stricmp(xmlChild->getTag(), "CaptureLength"))
		{
			int requiredPlayers = 0;
			float timeToCapture = 0.f;
			int requiredCaptures = 0;
			xmlChild->getAttr("requiredPlayers", requiredPlayers);
			xmlChild->getAttr("timeToCapture", timeToCapture);
			xmlChild->getAttr("requiredCaptures", requiredCaptures);
			CryLog("CGameRulesCaptureObjective::Init, 'CaptureLength' node found, requiredPlayers=%i, timeToCapture=%f, requiredCaptures=%i", requiredPlayers, timeToCapture, requiredCaptures);
			m_captureLengths.push_back(SCaptureLength(requiredPlayers, timeToCapture, requiredCaptures));
		}
		else if (!stricmp(xmlChild->getTag(), "Icons"))
		{
			CRY_ASSERT_MESSAGE(!m_useIcons, "CaptureObjective xml contains more than one 'Icons' node, we only support one");
			m_useIcons = true;

			xmlChild->getAttr("priority", m_iconPriority);

			// TODO: Read strings, turn strings => numerical IDs (enum/hash/whatever)
			m_neutralIcon            = (EGameRulesMissionObjectives)atoi(xmlChild->getAttr("neutral"));
			m_initialCaptureBlueIcon = (EGameRulesMissionObjectives)atoi(xmlChild->getAttr("initialCaptureBlue"));
			m_initialCaptureRedIcon  = (EGameRulesMissionObjectives)atoi(xmlChild->getAttr("initialCaptureRed"));
			m_stealRedToBlueIcon     = (EGameRulesMissionObjectives)atoi(xmlChild->getAttr("stealRedToBlue"));
			m_stealBlueToRedIcon     = (EGameRulesMissionObjectives)atoi(xmlChild->getAttr("stealBlueToRed"));

			m_shouldShowIconFunc.Format("%s", xmlChild->getAttr("checkFunc"));

			CryLog("CGameRulesCaptureObjective::Init, using on-screen icons [%i %i %i %i %i]", m_neutralIcon, m_initialCaptureBlueIcon, m_initialCaptureRedIcon, m_stealRedToBlueIcon, m_stealBlueToRedIcon);
		}
		else if(strcmp(xmlChild->getTag(),"Audio") == 0)
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
		else if(strcmp(xmlChild->getTag(),"AudioSignal") == 0)
		{
			ReadAudioSignal(xmlChild, "captured", &m_captureSignal);
			ReadAudioSignal(xmlChild, "capturedTeam", &m_capturedSignalTeam);
			ReadAudioSignal(xmlChild, "capturedOpponent", &m_capturedSignalOpponent);
			ReadAudioSignal(xmlChild, "capturingTeam", &m_capturingSignalTeam);
			ReadAudioSignal(xmlChild, "capturingOpponent", &m_capturingSignalOpponent);
		}
		else if (!stricmp(xmlChild->getTag(), "Strings"))
		{
			const char *pString = 0;
			if (xmlChild->getAttr("friendlyCapture", &pString))
			{
				m_friendlyCaptureString.Format("@%s", pString);
			}
			if (xmlChild->getAttr("enemyCapture", &pString))
			{
				m_enemyCaptureString.Format("@%s", pString);
			}
			if (xmlChild->getAttr("newEntity", &pString))
			{
				m_newEntityString.Format("@%s", pString);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::ReadAudioSignal(const XmlNodeRef& node, const char* name, CAudioSignalPlayer* signalPlayer)
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
void CGameRulesCaptureObjective::UpdateIcon(SHoldEntityDetails * pDetails, bool force)
{
	SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity *>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pCaptureEntity);

	EGameRulesMissionObjectives requestedIcon = GetIcon(pDetails);

	if (requestedIcon != pCaptureEntity->m_currentIcon || pCaptureEntity->m_needIconUpdate || force)
	{
		if (requestedIcon != -1)
		{
			CCCPOINT_IF(requestedIcon != pCaptureEntity->m_currentIcon, CaptureObjective_SetNewIcon);

			float captureProgress = pCaptureEntity->m_captureProgress.m_teamId ? pCaptureEntity->m_captureProgress.m_progress : 0.f;

			SHUDEvent newMissionObjective(eHUDEvent_OnNewObjective);
			newMissionObjective.ReserveData(4);
			newMissionObjective.AddData( static_cast<int>(pDetails->m_id) ); /*(EntityId)*/
			newMissionObjective.AddData( static_cast<int>(requestedIcon) ); /*(EGameRulesMissionObjectives)*/ 
			newMissionObjective.AddData( captureProgress ); /*float*/
			newMissionObjective.AddData( m_iconPriority ); /*int*/
			CHUD::CallEvent(newMissionObjective);

			pCaptureEntity->m_needIconUpdate = (pCaptureEntity->m_captureProgress.m_teamId != 0);

			CRY_TODO(09, 10, 2009, "Move to happen only once when CaptureEntities are created because the hud doesn't exist at that point currently");

			if(!pCaptureEntity->m_isOnRadar)
			{
				SHUDEvent hudevent(eHUDEvent_AddEntity);
				hudevent.AddData(SHUDEventData((int)pDetails->m_id));
				CHUD::CallEvent(hudevent);
				pCaptureEntity->m_isOnRadar = true;
			}
		}
		else
		{
			CCCPOINT(CaptureObjective_RemoveIcon);

			SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
			newRemoveObjective.ReserveData(2);
			newRemoveObjective.AddData( static_cast<int>(pDetails->m_id) ); /*(EntityId)*/
			newRemoveObjective.AddData( m_iconPriority ); /*(int)*/
			CHUD::CallEvent(newRemoveObjective);
		}
		pCaptureEntity->m_currentIcon = requestedIcon;
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::Update( float frameTime )
{
	BaseType::Update(frameTime);

	bool aspectHasChanged = false;			// Remember if we need to mark the aspect has changed since multiple
																			// changes can happen and we only want to mark as dirty once

	bool doScoring = false;
	IGameRulesScoringModule *pScoringModule = 0;
	int team1Count = 0;
	int team2Count = 0;

	if (gEnv->bServer && m_useScoreTimer)
	{
		m_timeToScore -= frameTime;
		if (m_timeToScore < 0.f)
		{
			pScoringModule = g_pGame->GetGameRules()->GetScoringModule();
			doScoring = (pScoringModule != NULL);

			m_timeToScore = m_scoreTimerLength;
		}
	}

	int currActiveIndex = -1;
	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SHoldEntityDetails *pDetails = &m_entities[i];
		if (!pDetails->m_id)
		{
			continue;
		}
		SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity *>(pDetails->m_pAdditionalData);
		CRY_ASSERT(pCaptureEntity);

		currActiveIndex++;

		if (pCaptureEntity->m_capturedTeamId && !pCaptureEntity->m_captureProgress.m_teamId)
		{
			if (gEnv->bServer)
			{
				SvSiteHeld(pCaptureEntity, team1Count, team2Count);
				if (m_maxScoresPerEntity)
				{
					aspectHasChanged = true;
				}
			}
		}
		else if (pCaptureEntity->m_captureProgress.m_teamId && pCaptureEntity->m_captureProgress.m_teamId == pDetails->m_controllingTeamId)
		{
			// Update cap progress, if server check for finished
			UpdateCaptureProgress(pCaptureEntity, frameTime);
			if (gEnv->bServer && (pCaptureEntity->m_captureProgress.m_progress >= 1.f))
			{
				// Player points, set to captured etc
				SvSiteCaptured(pDetails);
				aspectHasChanged = true;
			}
		}
		else if (!pDetails->m_controllingTeamId && pCaptureEntity->m_captureProgress.m_teamId)
		{
			// Bleed, if progress < 0, stop
			if (m_useBleedRate)
			{
				pCaptureEntity->m_captureProgress.m_progress -= (frameTime * m_bleedRate);
				if (pCaptureEntity->m_captureProgress.m_progress < 0.f)
				{
					pCaptureEntity->m_captureProgress.Reset();
				}
			}
		}

		// Do HUD stuff
		CRY_TODO(23, 09, 2009, "Switch this with real hud stuff once we can");
		if (gEnv->bClient)
		{
			ClUpdateHUD(pDetails, currActiveIndex);
			UpdateCaptureEntityAudio(pDetails);
		}
	}

	if (gEnv->bServer)
	{
		if (doScoring)
		{
			if (m_scoringEnabled[0])
			{
				for (int i = 0; i < team1Count; ++ i)
				{
					pScoringModule->OnTeamScoringEvent(1, EGRST_CaptureObjectiveHeld);
				}
			}
			if (m_scoringEnabled[1])
			{
				for (int i = 0; i < team2Count; ++ i)
				{
					pScoringModule->OnTeamScoringEvent(2, EGRST_CaptureObjectiveHeld);
				}
			}
		}

		if (aspectHasChanged)
		{
			g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(g_pGame->GetGameRules()->GetEntityId(), CAPTURE_OBJECTIVE_STATE_ASPECT);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::SvSiteCaptured( SHoldEntityDetails *pDetails )
{
	SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity*>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pCaptureEntity);

	CryLog("Team %i has captured the entity", pDetails->m_controllingTeamId);

	// Finish the capture
	pCaptureEntity->m_capturedTeamId = pDetails->m_controllingTeamId;
	pCaptureEntity->m_captureProgress.Reset();
	++ pCaptureEntity->m_captures;
	g_pGame->GetGameRules()->SetTeam(pCaptureEntity->m_capturedTeamId, pDetails->m_id);

	// Scoring
	IGameRulesScoringModule *pScoringModule = g_pGame->GetGameRules()->GetScoringModule();
	if (pScoringModule && m_scoringEnabled[pCaptureEntity->m_capturedTeamId - 1])
	{								
		AwardPlayerPoints(&pDetails->m_insideEntities[pCaptureEntity->m_capturedTeamId - 1], EGRST_CaptureObjectiveCompleted);
		pScoringModule->OnTeamScoringEvent(pCaptureEntity->m_capturedTeamId, EGRST_CaptureObjectiveTaken);
	}

	// Check if the entity is finished (can have a limit on number of allowed captures
	if (m_maxCapturesPerEntity && pCaptureEntity->m_captures >= m_maxCapturesPerEntity)
	{
		pCaptureEntity->m_isFinished = true;
	}

	// Tell client part of server
	if (gEnv->bClient)
	{
		ClSiteCaptured(pDetails);
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::SvSiteHeld( SCaptureEntity *pCaptureEntity, int &team1Count, int &team2Count )
{
	if (pCaptureEntity->m_capturedTeamId == 1)
	{
		++ team1Count;
	}
	else
	{
		++ team2Count;
	}
	++ pCaptureEntity->m_scores;
	if (m_maxScoresPerEntity && pCaptureEntity->m_scores >= m_maxScoresPerEntity)
	{
		pCaptureEntity->m_isFinished = true;
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::ClUpdateHUD( SHoldEntityDetails *pDetails, int &currentActiveIndex )
{
	SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity *>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pCaptureEntity);

	UpdateIcon(pDetails, false);

	if (pCaptureEntity->m_captureProgress.m_teamId)
	{
		SHUDEvent siteIsBeingCaptured;
		siteIsBeingCaptured.eventType = eHUDEvent_OnSiteBeingCaptured;
		siteIsBeingCaptured.eventIntData = currentActiveIndex;
		siteIsBeingCaptured.eventIntData2 = pCaptureEntity->m_capturedTeamId;
		siteIsBeingCaptured.eventIntData3 = pCaptureEntity->m_captureProgress.m_teamId;
		siteIsBeingCaptured.eventFloatData = pCaptureEntity->m_captureProgress.m_progress;
		CHUD::CallEvent(siteIsBeingCaptured);

		if (pDetails->m_localPlayerIsWithinRange && (pDetails->m_insideCount[pCaptureEntity->m_captureProgress.m_teamId - 1]))
		{
			CUISimpleBar * captureBar = CUISimpleBar::GetInstanceWithName("CaptureProgressBar");
			if (captureBar)
			{
				captureBar->Set(clamp(pCaptureEntity->m_captureProgress.m_progress, 0.f, 1.f));

				ColorF bg;
				ColorF fg;
				ColorF f1(0.0f,0.0f,0.5f,0.4f);
				ColorF f2(1.0f,0.0f,0.0f,0.6f);
				
				int playerTeamId = g_pGame->GetGameRules()->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());

				// BG
				if (pCaptureEntity->m_capturedTeamId == 0)									// Not owned by either team
				{
					bg.set(0.3f, 0.3f, 0.3f, 0.6f);
				}
				else if (pCaptureEntity->m_capturedTeamId == playerTeamId)	// Owned by friendly team
				{
					bg.set(0.0f, 0.0f, 0.5f, 0.4f);
				}
				else																												// Owned by enemy team
				{
					bg.set(1.0f, 0.0f, 0.0f, 0.6f);
				}

				// FG
				if (pCaptureEntity->m_captureProgress.m_teamId == playerTeamId)	// Being captured by friendly team
				{
					fg.set(0.0f, 0.0f, 1.0f, 1.0f);
				}
				else																														// Being captured by enemy team
				{
					fg.set(1.0f, 0.0f, 0.0f, 1.0f);
				}

				captureBar->SetColours(bg, fg, f1, f2);

				if (pDetails->m_controllingTeamId == CONTESTED_TEAM_ID)				// Site is contested
				{
					captureBar->TriggerWarningFlash();
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::OnStartGame()
{
	m_timeToScore = 0.f;

	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		if (m_entities[i].m_id)
		{
			UpdateIcon(&m_entities[i], true);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::OnInsideStateChanged( SHoldEntityDetails *pDetails )
{
	SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity*>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pCaptureEntity);

	if (pDetails->m_controllingTeamId == 1 || pDetails->m_controllingTeamId == 2)
	{
		if (pCaptureEntity->m_capturedTeamId != pDetails->m_controllingTeamId && (pCaptureEntity->m_captureProgress.m_teamId != pDetails->m_controllingTeamId))
		{
			SiteStartCapturing(pDetails, pDetails->m_controllingTeamId);
		}
		else if (pCaptureEntity->m_capturedTeamId && (pCaptureEntity->m_capturedTeamId == pDetails->m_controllingTeamId))
		{
			pCaptureEntity->m_captureProgress.Reset();
		}
		pCaptureEntity->m_capturelengthIdx = DetermineCaptureLengthIndex(pDetails->m_insideCount[pDetails->m_controllingTeamId - 1], pCaptureEntity->m_captures);
	}
	else if (pDetails->m_controllingTeamId == CONTESTED_TEAM_ID && pCaptureEntity->m_capturedTeamId)		// Owned by one team but contested, start capture for other team
	{
		SiteStartCapturing(pDetails, 3 - pCaptureEntity->m_capturedTeamId);
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::SiteStartCapturing(SHoldEntityDetails *pDetails, int teamId)
{
	SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity*>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pCaptureEntity);
	CRY_ASSERT(teamId == 1 || teamId == 2);

	int playerCount = pDetails->m_insideCount[teamId - 1];
	pCaptureEntity->m_capturelengthIdx = DetermineCaptureLengthIndex(playerCount, pCaptureEntity->m_captures);

	pCaptureEntity->m_captureProgress.StartCapture(teamId);

	if (gEnv->bClient)
	{
		//Feedback
		CGameRules *pGameRules = g_pGame->GetGameRules();
		int localTeam = pGameRules->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());
		if (localTeam == teamId)
		{
			CCCPOINT(CaptureObjective_SiteStartCapturingByFriendlyTeam);
			m_capturingSignalTeam.Play();
		}
		else
		{
			CCCPOINT(CaptureObjective_SiteStartCapturingByEnemyTeam);
			m_capturingSignalOpponent.Play();
		}
		pCaptureEntity->m_needIconUpdate = true;
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::OnNetSerializeHoldEntity(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags, SHoldEntityDetails *pDetails, int index)
{
	if (aspect == CAPTURE_OBJECTIVE_STATE_ASPECT)
	{
		SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity*>(pDetails->m_pAdditionalData);
		if (!pCaptureEntity)
		{
			// Have to serialise into somewhere
			CRY_ASSERT((index >= 0) && (index < HOLD_OBJECTIVE_MAX_ENTITIES));
			pCaptureEntity = &m_additionalInfo[index];		// This entity hasn't been added yet due to ordering issue but we need the capture progress data
		}

		int oldCapturedTeam = pCaptureEntity->m_capturedTeamId;

		ser.Value("capturedTeam", pCaptureEntity->m_capturedTeamId, 'team');
		ser.Value("captures", pCaptureEntity->m_captures, 'ui5');
		if (m_maxScoresPerEntity)
		{
			ser.Value("scoresCount", pCaptureEntity->m_scores, 'ui8');
		}
		int capturingTeamId = pCaptureEntity->m_captureProgress.m_teamId;
		ser.Value("capturingTeam", capturingTeamId, 'team');
		ser.Value("progress", pCaptureEntity->m_captureProgress.m_progress, 'unit');

		if (ser.IsReading())
		{
			if (pDetails->m_pAdditionalData)
			{
				if (pCaptureEntity->m_capturedTeamId && (oldCapturedTeam != pCaptureEntity->m_capturedTeamId))
				{
					CryLog("CGameRulesCaptureObjective::OnNetSerializeHoldEntity, Site captured by team %i", pCaptureEntity->m_capturedTeamId);
					ClSiteCaptured(pDetails);
				}

			}
			else
			{
				// Only need to set capturing teamId if we can't work it out ourselves
				pCaptureEntity->m_captureProgress.m_teamId = capturingTeamId;
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::OnNewHoldEntity(SHoldEntityDetails *pDetails, int index)
{
	CRY_ASSERT(index < HOLD_OBJECTIVE_MAX_ENTITIES);

	SCaptureEntity *pCaptureEntity = &m_additionalInfo[index];

	pDetails->m_pAdditionalData = pCaptureEntity;

	if (gEnv->bServer)
	{
		g_pGame->GetGameRules()->SetTeam(0, pDetails->m_id);
	}

	pCaptureEntity->m_currentIcon = EGRMO_Unknown;
	pCaptureEntity->m_needIconUpdate = true;
	pCaptureEntity->m_isOnRadar = false;

	if (gEnv->bClient)
	{
		UpdateIcon(pDetails, true);
		InitCaptureEntityAudio(pDetails);
	}

	OnInsideStateChanged(pDetails);
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::EnableCompletion( int teamId, bool enable )
{
	if (teamId == 1 || teamId == 2)
	{
		m_capturingEnabled[teamId - 1] = enable;
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::EnableScoring( int teamId, bool enable )
{
	if (teamId == 1 || teamId == 2)
	{
		m_scoringEnabled[teamId - 1] = enable;
	}
}

//------------------------------------------------------------------------
bool CGameRulesCaptureObjective::IsComplete( int teamId )
{
	return false;
}

//------------------------------------------------------------------------
int CGameRulesCaptureObjective::DetermineCaptureLengthIndex( int numPlayers, int numCaptures )
{
	int numLengths = m_captureLengths.size();
	for (int i = 0; i < numLengths; ++ i)
	{
		SCaptureLength *pCaptureLength = &m_captureLengths[i];
		if ((numPlayers >= pCaptureLength->m_requiredPlayers) && (numCaptures >= pCaptureLength->m_requiredCaptures))
		{
			return i;
		}
	}
	return -1;
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::UpdateCaptureProgress(SCaptureEntity *pCaptureEntity, float frameTime)
{
	if (pCaptureEntity->m_capturelengthIdx >= 0)
	{
		float captureLength = m_captureLengths[pCaptureEntity->m_capturelengthIdx].m_timeToCapture;

#if CAPTURE_OBJECTIVE_ADDITIONAL_DEBUG_CODE
		if (m_overrideCaptureTime >= 0.f)
		{
			captureLength = m_overrideCaptureTime;
		}
#endif

		if (captureLength > 0.f)
		{
			pCaptureEntity->m_captureProgress.m_progress += (frameTime / captureLength);
		}
		else
		{
			pCaptureEntity->m_captureProgress.m_progress = 1.1f;
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::ClSiteCaptured( SHoldEntityDetails *pDetails )
{
	// Site has been captured
	CGameRules *pGameRules = g_pGame->GetGameRules();
	int localTeam = pGameRules->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());
	m_captureSignal.Play(pDetails->m_id);

	SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity*>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pCaptureEntity);

	pCaptureEntity->m_captureProgress.Reset();
	if (localTeam == pCaptureEntity->m_capturedTeamId)
	{
		CCCPOINT(CaptureObjective_SiteCapturedByFriendlyTeam);
		if (!m_friendlyCaptureString.empty())
		{
			pGameRules->OnTextMessage(eTextMessageAnnouncement, m_friendlyCaptureString.c_str());
		}
		m_capturedSignalTeam.Play();
	}
	else
	{
		CCCPOINT(CaptureObjective_SiteCapturedByEnemyTeam);
		if (!m_enemyCaptureString.empty())
		{
			pGameRules->OnTextMessage(eTextMessageAnnouncement, m_enemyCaptureString.c_str());
		}
		m_capturedSignalOpponent.Play();
	}

	int currActiveIndex = -1;
	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; i ++)
	{
		if( !m_entities[i].m_id)
		{
			continue;
		}
		++currActiveIndex;

		if( &m_entities[i] != pDetails )
		{
			continue;
		}

		SHUDEvent siteIsCaptured;
		siteIsCaptured.eventType = eHUDEvent_OnSiteCaptured;
		siteIsCaptured.AddData(SHUDEventData(currActiveIndex));
		siteIsCaptured.AddData(SHUDEventData(pCaptureEntity->m_capturedTeamId));
		CHUD::CallEvent(siteIsCaptured);
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::OnChangedTeam( EntityId entityId, int oldTeamId, int newTeamId )
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
				const SCaptureEntity *pCaptureEntity = static_cast<const SCaptureEntity *>(pDetails->m_pAdditionalData);
				CRY_ASSERT(pCaptureEntity);

				UpdateIcon(pDetails, true);

				++currActiveIndex;
				SHUDEvent siteIsCaptured;
				siteIsCaptured.eventType = eHUDEvent_OnSiteCaptured;
				siteIsCaptured.eventIntData = currActiveIndex;
				siteIsCaptured.eventIntData2 = pCaptureEntity->m_capturedTeamId;
				CHUD::CallEvent(siteIsCaptured);

				IEntity *pEntity = gEnv->pEntitySystem->GetEntity(pDetails->m_id);
				if (pEntity)
				{
					IScriptTable *pScript = pEntity->GetScriptTable();
					if (pScript && pScript->GetValueType("LocalPlayerChangedTeam") == svtFunction)
					{
						IScriptSystem *pScriptSystem = gEnv->pScriptSystem;
						pScriptSystem->BeginCall(pScript, "LocalPlayerChangedTeam");
						pScriptSystem->PushFuncParam(pScript);
						pScriptSystem->EndCall();
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------
EGameRulesMissionObjectives CGameRulesCaptureObjective::GetIcon( SHoldEntityDetails *pDetails )
{
	SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity*>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pCaptureEntity);

	EGameRulesMissionObjectives requestedIcon = pCaptureEntity->m_currentIcon;	// Default to current icon

	if (!pDetails->m_id)
	{
		requestedIcon = EGRMO_Unknown;
	}
	else
	{
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

			if (pCaptureEntity->m_captureProgress.m_teamId)
			{
				if (pCaptureEntity->m_capturedTeamId)
				{
					requestedIcon = (pCaptureEntity->m_capturedTeamId == localTeamId) ? m_stealBlueToRedIcon : m_stealRedToBlueIcon;
				}
				else
				{
					requestedIcon = (pCaptureEntity->m_captureProgress.m_teamId == localTeamId) ? m_initialCaptureBlueIcon : m_initialCaptureRedIcon;
				}
			}
			else if (pCaptureEntity->m_capturedTeamId)
			{
				// Objective is captured, use friendly/hostile icons depending on who has captured it
				requestedIcon = (pCaptureEntity->m_capturedTeamId == localTeamId) ? m_stealBlueToRedIcon : m_stealRedToBlueIcon;
			}
			else
			{
				requestedIcon = m_neutralIcon;
			}
		}
	}

	return requestedIcon;
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::InitCaptureEntityAudio( SHoldEntityDetails *pDetails )
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(pDetails->m_id);
	if(pEntity && pEntity->CreateProxy(ENTITY_PROXY_SOUND))
	{
		SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity*>(pDetails->m_pAdditionalData);
		CRY_ASSERT(pCaptureEntity);

		IEntitySoundProxy *soundProxy = (IEntitySoundProxy*) pEntity->GetProxy(ENTITY_PROXY_SOUND);
		Vec3 zero = Vec3(0.0f, 0.0f, 0.0f);
		pCaptureEntity->m_captureAudio = soundProxy->PlaySound(m_capturingAudio, zero, zero, FLAG_SOUND_EVENT, eSoundSemantic_Mechanic_Entity);
		if(pCaptureEntity->m_captureAudio != INVALID_SOUNDID)
		{
			ISound* pSound = soundProxy->GetSound(pCaptureEntity->m_captureAudio);
			if(pSound)
			{
				pSound->SetParam("power", 0.0f);
				pSound->SetParam("opponentpower", 0.0f);
				pSound->SetParam("energy", 0.0f);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::UpdateCaptureEntityAudio( SHoldEntityDetails *pDetails )
{
	SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity*>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pCaptureEntity);

	if(pCaptureEntity->m_captureAudio != INVALID_SOUNDID)
	{
		_smart_ptr<ISound> pCaptureAudio = NULL;
		_smart_ptr<ISound> pCapturedAudio = NULL;
		IEntitySoundProxy *soundProxy = NULL;
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(pDetails->m_id);
		if(pEntity)
		{
			soundProxy = (IEntitySoundProxy*)(pEntity->GetProxy(ENTITY_PROXY_SOUND));
			if (soundProxy)
			{
				pCaptureAudio = soundProxy->GetSound(pCaptureEntity->m_captureAudio);
				pCapturedAudio = soundProxy->GetSound(pCaptureEntity->m_capturedAudio);
			}
		}

		CGameRules *pGameRules = g_pGame->GetGameRules();
		int localTeam = pGameRules->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());
		if (pCaptureEntity->m_captureProgress.m_teamId)
		{
			if(pCapturedAudio)
			{
				pCapturedAudio->Stop();
			}
			float power = 0.0f;
			float opponentPower = 0.0f;
			if (localTeam == pCaptureEntity->m_captureProgress.m_teamId )
			{
				power = pCaptureEntity->m_captureProgress.m_progress;
			}
			else
			{
				opponentPower = pCaptureEntity->m_captureProgress.m_progress;
			}
			if (pCaptureAudio)
			{
				pCaptureAudio->SetParam("energy", pCaptureEntity->m_captureProgress.m_progress);
				pCaptureAudio->SetParam("power", power);
				pCaptureAudio->SetParam("opponentpower", opponentPower);
			}
		}
		else if(pCaptureEntity->m_capturedTeamId && soundProxy)
		{
			if(pCaptureAudio && !pCaptureAudio->IsPlaying())
			{
				pCaptureAudio->Play();
			}

			if(soundProxy && pCaptureEntity->m_capturedAudio == INVALID_SOUNDID)
			{
				Vec3 zero = Vec3(0.0f, 0.0f, 0.0f);
				pCaptureEntity->m_capturedAudio = soundProxy->PlaySound(m_capturedAudio, zero, zero, FLAG_SOUND_EVENT, eSoundSemantic_Mechanic_Entity);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::ClearCaptureEntityAudio( SHoldEntityDetails *pDetails )
{
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(pDetails->m_id);
	if(pEntity)
	{
		IEntitySoundProxy *soundProxy = (IEntitySoundProxy*) pEntity->GetProxy(ENTITY_PROXY_SOUND);
		if(soundProxy)
		{
			soundProxy->StopAllSounds();
		}
	}

	m_captureSignal.Stop(pDetails->m_id);
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::OnRemoveHoldEntity( SHoldEntityDetails *pDetails )
{
	SCaptureEntity *pCaptureEntity = static_cast<SCaptureEntity *>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pCaptureEntity);
	pCaptureEntity->Reset();

	SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
	newRemoveObjective.ReserveData(2);
	newRemoveObjective.AddData( static_cast<int>(pDetails->m_id) ); /*(EntityId)*/
	newRemoveObjective.AddData( m_iconPriority );
	CHUD::CallEvent(newRemoveObjective);

	ClearCaptureEntityAudio(pDetails);

	SHUDEvent hudevent(eHUDEvent_RemoveEntity);
	hudevent.AddData(SHUDEventData((int)pDetails->m_id));
	CHUD::CallEvent(hudevent);
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::OnOwnClientEnteredGame()
{
	BaseType::OnOwnClientEnteredGame();
	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SHoldEntityDetails *pDetails = &m_entities[i];
		if (pDetails->m_id)
		{
			UpdateIcon(pDetails, true);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesCaptureObjective::OnClientConnect( int channelId, bool isReset, EntityId playerId )
{
	// Need to mark aspect as changes so that the network collects the latest capture progress amounts
	g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(g_pGame->GetGameRules()->GetEntityId(), CAPTURE_OBJECTIVE_STATE_ASPECT);
}

//------------------------------------------------------------------------
bool CGameRulesCaptureObjective::IsEntityFinished(int type, int index)
{
	CRY_ASSERT(index < HOLD_OBJECTIVE_MAX_ENTITIES);
	const SHoldEntityDetails *pDetails = &m_entities[index];
	const SCaptureEntity *pCaptureEntity = static_cast<const SCaptureEntity *>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pCaptureEntity);
	return pCaptureEntity->m_isFinished;
}

//------------------------------------------------------------------------
bool CGameRulesCaptureObjective::CanRemoveEntity(int type, int index)
{
	CRY_ASSERT(index < HOLD_OBJECTIVE_MAX_ENTITIES);
	const SHoldEntityDetails *pDetails = &m_entities[index];
	const SCaptureEntity *pCaptureEntity = static_cast<const SCaptureEntity *>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pCaptureEntity);
	return !pCaptureEntity->m_captureProgress.m_teamId;		// Don't remove entities that are half captured
}
