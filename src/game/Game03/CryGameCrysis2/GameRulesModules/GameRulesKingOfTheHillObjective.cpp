/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of a king of the hill objective (take and hold)

	-------------------------------------------------------------------------
	History:
	- 15:02:2010  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesKingOfTheHillObjective.h"
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
#include "Utility/CryWatch.h"

#if NUM_ASPECTS > 8
	#define KING_OF_THE_HILL_OBJECTIVE_STATE_ASPECT		eEA_GameServerA
#else
	#define KING_OF_THE_HILL_OBJECTIVE_STATE_ASPECT		eEA_GameServerStatic
#endif

//------------------------------------------------------------------------
CGameRulesKingOfTheHillObjective::CGameRulesKingOfTheHillObjective()
{
	m_scoringEnabled[0] = false;
	m_scoringEnabled[1] = false;

	m_iconPriority = 0;
	m_scoreTimerMaxLength = 0.f;
	m_scoreTimerAdditionalPlayerMultiplier = 0.f;
	m_useIcons = false;
	m_neutralIcon = EGRMO_Unknown;
	m_friendlyIcon = EGRMO_Unknown;
	m_hostileIcon = EGRMO_Unknown;

	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		m_additionalInfo[i].Reset();
	}
}

//------------------------------------------------------------------------
CGameRulesKingOfTheHillObjective::~CGameRulesKingOfTheHillObjective()
{
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::Init( XmlNodeRef xml )
{
	BaseType::Init(xml);

	if (xml->getAttr("scoreTime", m_scoreTimerMaxLength))
	{
		CryLog("CGameRulesKingOfTheHillObjective::Init, using score timer, length=%f", m_scoreTimerMaxLength);
	}
	xml->getAttr("additionalPlayerTimerMultiplier", m_scoreTimerAdditionalPlayerMultiplier);
	if (m_scoreTimerAdditionalPlayerMultiplier == 0.f)
	{
		m_scoreTimerAdditionalPlayerMultiplier = 1.f;
	}
	CryLog("CGameRulesKingOfTheHillObjective::Init, multiplier for additional players=%f", m_scoreTimerAdditionalPlayerMultiplier);

	int numChildren = xml->getChildCount();
	for (int childIdx = 0; childIdx < numChildren; ++ childIdx)
	{
		XmlNodeRef xmlChild = xml->getChild(childIdx);
		const char *pTag = xmlChild->getTag();
		if (!stricmp(pTag, "Icons"))
		{
			CRY_ASSERT_MESSAGE(!m_useIcons, "KingOfTheHillObjective xml contains more than one 'Icons' node, we only support one");
			m_useIcons = true;

			xmlChild->getAttr("priority", m_iconPriority);

			m_neutralIcon		= (EGameRulesMissionObjectives)atoi(xmlChild->getAttr("neutral"));
			m_friendlyIcon	= (EGameRulesMissionObjectives)atoi(xmlChild->getAttr("friendly"));
			m_hostileIcon		= (EGameRulesMissionObjectives)atoi(xmlChild->getAttr("hostile"));

			m_shouldShowIconFunc.Format("%s", xmlChild->getAttr("checkFunc"));

			CryLog("CGameRulesKingOfTheHillObjective::Init, using on-screen icons [%i %i %i]", m_neutralIcon, m_friendlyIcon, m_hostileIcon);
		}
		else if(strcmp(pTag,"Audio") == 0)
		{
			if(xmlChild->haveAttr("capturedLoop"))
			{
				cry_strncpy(m_capturedAudio, xmlChild->getAttr("capturedLoop"), k_maxAudioLength);
			}
		}
		else if(strcmp(pTag,"AudioSignal") == 0)
		{
			ReadAudioSignal(xmlChild, "captured", &m_captureSignal);
			ReadAudioSignal(xmlChild, "capturedTeam", &m_capturedSignalTeam);
			ReadAudioSignal(xmlChild, "capturedOpponent", &m_capturedSignalOpponent);
		}
		else if (!stricmp(pTag, "Strings"))
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
			if (xmlChild->getAttr("friendlyLost", &pString))
			{
				m_friendlyLostString.Format("@%s", pString);
			}
			if (xmlChild->getAttr("enemyLost", &pString))
			{
				m_enemyLostString.Format("@%s", pString);
			}
			if (xmlChild->getAttr("newEntity", &pString))
			{
				m_newEntityString.Format("@%s", pString);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::ReadAudioSignal(const XmlNodeRef& node, const char* name, CAudioSignalPlayer* signalPlayer)
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
void CGameRulesKingOfTheHillObjective::UpdateIcon(SHoldEntityDetails * pDetails, bool force)
{
	SKotHEntity *pKotHEntity = static_cast<SKotHEntity *>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pKotHEntity);

	EGameRulesMissionObjectives requestedIcon = GetIcon(pDetails);

	if ((requestedIcon != pKotHEntity->m_currentIcon) || force)
	{
		if (requestedIcon != EGRMO_Unknown)
		{
			CCCPOINT_IF(requestedIcon != pKotHEntity->m_currentIcon, KingOfTheHillObjective_SetNewIcon);

			SHUDEvent newMissionObjective(eHUDEvent_OnNewObjective);
			newMissionObjective.ReserveData(4);
			newMissionObjective.AddData( static_cast<int>(pDetails->m_id) ); /*(EntityId)*/
			newMissionObjective.AddData( requestedIcon ); /*(EGameRulesMissionObjectives)*/ 
			newMissionObjective.AddData( 0.0f ); /*(float)*/ 
			newMissionObjective.AddData( m_iconPriority ); /*(int)*/ 
			CHUD::CallEvent(newMissionObjective);

			pKotHEntity->m_needsIconUpdate = false;

			if(!pKotHEntity->m_isOnRadar)
			{
				SHUDEvent hudevent(eHUDEvent_AddEntity);
				hudevent.AddData(SHUDEventData((int)pDetails->m_id));
				CHUD::CallEvent(hudevent);
				pKotHEntity->m_isOnRadar = true;
			}
		}
		else
		{
			CCCPOINT(KingOfTheHillObjective_RemoveIcon);

			SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
			newRemoveObjective.ReserveData(2);
			newRemoveObjective.AddData( static_cast<int>(pDetails->m_id) ); /*(EntityId)*/
			newRemoveObjective.AddData( m_iconPriority );
			CHUD::CallEvent(newRemoveObjective);
		}
		pKotHEntity->m_currentIcon = requestedIcon;
	}
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::Update( float frameTime )
{
	BaseType::Update(frameTime);

	IGameRulesScoringModule *pScoringModule = g_pGame->GetGameRules()->GetScoringModule();

	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SHoldEntityDetails *pDetails = &m_entities[i];
		if (!pDetails->m_id)
		{
			continue;
		}

		SKotHEntity *pKotHEntity = static_cast<SKotHEntity *>(pDetails->m_pAdditionalData);
		CRY_ASSERT(pKotHEntity);

		if (gEnv->bServer && pScoringModule)
		{
#ifndef _RELEASE
			if (g_pGameCVars->g_KingOfTheHillObjective_watchLvl)
			{
				IEntity *pEntity = gEnv->pEntitySystem->GetEntity(pDetails->m_id);
				const char *pEntName = pEntity ? pEntity->GetName() : "<NULL>";
				if (pDetails->m_controllingTeamId == CONTESTED_TEAM_ID)
				{
					CryWatch("KotH entity '%s' is contested", pEntName);
				}
				else if (pDetails->m_controllingTeamId == 0)
				{
					CryWatch("KotH entity '%s' has no players nearby", pEntName);
				}
				else
				{
					CryWatch("KotH entity '%s' controlled by team %i, scoreTimerLength='%.2f', timeSinceLastScore='%.2f'", 
										pEntName, pDetails->m_controllingTeamId, pKotHEntity->m_scoreTimerLength, pKotHEntity->m_timeSinceLastScore);
				}
			}
#endif

			if (pKotHEntity->m_scoringTeamId)
			{
				pKotHEntity->m_timeSinceLastScore += frameTime;
				if (pKotHEntity->m_timeSinceLastScore >= pKotHEntity->m_scoreTimerLength)
				{
					const int teamIndex = pKotHEntity->m_scoringTeamId - 1;
					pScoringModule->OnTeamScoringEvent(teamIndex + 1, EGRST_KingOfTheHillObjectiveHeld);
					pKotHEntity->m_timeSinceLastScore = 0.f;
					AwardPlayerPoints(&pDetails->m_insideEntities[teamIndex], EGRST_KingOfTheHillObjectiveHeld);
					CCCPOINT_IF((pKotHEntity->m_scoringTeamId == 1), KingOfTheHillObjective_TeamTanScored);
					CCCPOINT_IF((pKotHEntity->m_scoringTeamId == 2), KingOfTheHillObjective_TeamBlackScored);
				}
			}
		}
		if (gEnv->bClient)
		{
			if (m_useIcons && pKotHEntity->m_needsIconUpdate)
			{
				UpdateIcon(pDetails, false);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::SvSiteChangedOwner( SHoldEntityDetails *pDetails )
{
	SKotHEntity *pKotHEntity = static_cast<SKotHEntity*>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pKotHEntity);

	// Set the team
	g_pGame->GetGameRules()->SetTeam(MAX(0, pDetails->m_controllingTeamId), pDetails->m_id);
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::ClUpdateHUD( SHoldEntityDetails *pDetails )
{
	SKotHEntity *pKotHEntity = static_cast<SKotHEntity *>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pKotHEntity);

	pKotHEntity->m_needsIconUpdate = true;

	// TODO: Local player inside area
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::OnStartGame()
{
	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		if (m_entities[i].m_id)
		{
			SKotHEntity *pKotHEntity = static_cast<SKotHEntity*>(m_entities[i].m_pAdditionalData);
			CRY_ASSERT(pKotHEntity);
			pKotHEntity->m_needsIconUpdate = true;
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::OnInsideStateChanged( SHoldEntityDetails *pDetails )
{
	SKotHEntity *pKotHEntity = static_cast<SKotHEntity*>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pKotHEntity);

	if (pKotHEntity->m_scoringTeamId != pDetails->m_controllingTeamId)
	{
		int oldTeamId = pKotHEntity->m_scoringTeamId;

		pKotHEntity->m_scoringTeamId = MAX(pDetails->m_controllingTeamId, 0);
		pKotHEntity->m_timeSinceLastScore = 0.f;

		if (gEnv->bServer)
		{
			SvSiteChangedOwner(pDetails);
		}
		if (gEnv->bClient)
		{
			ClSiteChangedOwner(pDetails, oldTeamId);
			UpdateEntityAudio(pDetails);
		}
	}

	if (pKotHEntity->m_scoringTeamId)
	{
		pKotHEntity->m_scoreTimerLength = CalculateScoreTimer(pDetails->m_insideCount[pKotHEntity->m_scoringTeamId - 1]);
	}

	if (gEnv->bClient)
	{
		ClUpdateHUD( pDetails );
	}
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::OnNewHoldEntity(SHoldEntityDetails *pDetails, int index)
{
	CRY_ASSERT(index < HOLD_OBJECTIVE_MAX_ENTITIES);

	SKotHEntity *pKotHEntity = &m_additionalInfo[index];
	pKotHEntity->Reset();

	pDetails->m_pAdditionalData = pKotHEntity;

	if (gEnv->bServer)
	{
		g_pGame->GetGameRules()->SetTeam(0, pDetails->m_id);
	}

	if (gEnv->bClient)
	{
		pKotHEntity->m_needsIconUpdate = true;		// Can't set icon straight away since the lua 'checkFunc' will give an incorrect result
		InitEntityAudio(pDetails);
	}

	OnInsideStateChanged(pDetails);
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::EnableScoring( int teamId, bool enable )
{
	if (teamId == 1 || teamId == 2)
	{
		m_scoringEnabled[teamId - 1] = enable;
	}
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::ClSiteChangedOwner( SHoldEntityDetails *pDetails, int oldTeamId )
{
	// Site has been captured
	CGameRules *pGameRules = g_pGame->GetGameRules();
	int localTeam = pGameRules->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());
	m_captureSignal.Play(pDetails->m_id);

	SKotHEntity *pKotHEntity = static_cast<SKotHEntity*>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pKotHEntity);

	const int ownerTeamId = pDetails->m_controllingTeamId;
	if (ownerTeamId > 0)
	{
		if (localTeam == ownerTeamId)
		{
			CCCPOINT(KingOfTheHillObjective_SiteCapturedByFriendlyTeam);
			if (!m_friendlyCaptureString.empty())
			{
				pGameRules->OnTextMessage(eTextMessageAnnouncement, m_friendlyCaptureString.c_str());
			}
			m_capturedSignalTeam.Play();
		}
		else
		{
			CCCPOINT(KingOfTheHillObjective_SiteCapturedByEnemyTeam);
			if (!m_enemyCaptureString.empty())
			{
				pGameRules->OnTextMessage(eTextMessageAnnouncement, m_enemyCaptureString.c_str());
			}
			m_capturedSignalOpponent.Play();
		}
	}
	else if (oldTeamId > 0)
	{
		if (localTeam == oldTeamId)
		{
			CCCPOINT(KingOfTheHillObjective_SiteLostByFriendlyTeam);
			if (!m_friendlyLostString.empty())
			{
				pGameRules->OnTextMessage(eTextMessageAnnouncement, m_friendlyLostString.c_str());
			}
		}
		else
		{
			CCCPOINT(KingOfTheHillObjective_SiteLostByEnemyTeam);
			if (!m_enemyLostString.empty())
			{
				pGameRules->OnTextMessage(eTextMessageAnnouncement, m_enemyLostString.c_str());
			}
		}
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
		siteIsCaptured.AddData(SHUDEventData(ownerTeamId));
		CHUD::CallEvent(siteIsCaptured);
	}
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::OnChangedTeam( EntityId entityId, int oldTeamId, int newTeamId )
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
				SKotHEntity *pKotHEntity = static_cast<SKotHEntity *>(pDetails->m_pAdditionalData);
				CRY_ASSERT(pKotHEntity);

				pKotHEntity->m_needsIconUpdate = true;

				++currActiveIndex;
				if (pDetails->m_controllingTeamId == 1 || pDetails->m_controllingTeamId == 2)
				{
					SHUDEvent siteIsCaptured;
					siteIsCaptured.eventType = eHUDEvent_OnSiteCaptured;
					siteIsCaptured.eventIntData = currActiveIndex;
					siteIsCaptured.eventIntData2 = pDetails->m_controllingTeamId;
					CHUD::CallEvent(siteIsCaptured);
				}

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
EGameRulesMissionObjectives CGameRulesKingOfTheHillObjective::GetIcon( SHoldEntityDetails *pDetails )
{
	SKotHEntity *pKotHEntity = static_cast<SKotHEntity*>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pKotHEntity);

	EGameRulesMissionObjectives requestedIcon = pKotHEntity->m_currentIcon;	// Default to current icon

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

			if ((pDetails->m_controllingTeamId == CONTESTED_TEAM_ID) || (pDetails->m_controllingTeamId == 0))
			{
				requestedIcon = m_neutralIcon;
			}
			else if (pDetails->m_controllingTeamId == localTeamId)
			{
				requestedIcon = m_friendlyIcon;
			}
			else
			{
				requestedIcon = m_hostileIcon;
			}
		}
	}

	return requestedIcon;
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::InitEntityAudio( SHoldEntityDetails *pDetails )
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(pDetails->m_id);
	if(pEntity)
	{
		pEntity->CreateProxy(ENTITY_PROXY_SOUND);
	}
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::UpdateEntityAudio( SHoldEntityDetails *pDetails )
{
	SKotHEntity *pKotHEntity = static_cast<SKotHEntity*>(pDetails->m_pAdditionalData);
	CRY_ASSERT(pKotHEntity);

	_smart_ptr<ISound> pCapturedAudio = NULL;
	IEntitySoundProxy *soundProxy = NULL;
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(pDetails->m_id);
	if(pEntity)
	{
		soundProxy = (IEntitySoundProxy*)(pEntity->GetProxy(ENTITY_PROXY_SOUND));
		if (soundProxy)
		{
			if (pKotHEntity->m_scoringTeamId && (pKotHEntity->m_capturedAudio == INVALID_SOUNDID))
			{
				const Vec3 zero(ZERO);
				pKotHEntity->m_capturedAudio = soundProxy->PlaySound(m_capturedAudio, zero, zero, FLAG_SOUND_EVENT, eSoundSemantic_Mechanic_Entity);
			}
			else if (!pKotHEntity->m_scoringTeamId && (pKotHEntity->m_capturedAudio != INVALID_SOUNDID))
			{
				soundProxy->StopSound(pKotHEntity->m_capturedAudio);
				pKotHEntity->m_capturedAudio = INVALID_SOUNDID;
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::ClearEntityAudio( SHoldEntityDetails *pDetails )
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
void CGameRulesKingOfTheHillObjective::OnRemoveHoldEntity( SHoldEntityDetails *pDetails )
{
	ClearEntityAudio(pDetails);

	SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
	newRemoveObjective.ReserveData(2);
	newRemoveObjective.AddData( static_cast<int>(pDetails->m_id) ); /*(EntityId)*/
	newRemoveObjective.AddData( m_iconPriority );
	CHUD::CallEvent(newRemoveObjective);

	SHUDEvent hudevent(eHUDEvent_RemoveEntity);
	hudevent.AddData(SHUDEventData((int)pDetails->m_id));
	CHUD::CallEvent(hudevent);
}

//------------------------------------------------------------------------
void CGameRulesKingOfTheHillObjective::OnOwnClientEnteredGame()
{
	BaseType::OnOwnClientEnteredGame();

	for (int i = 0; i < HOLD_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SHoldEntityDetails *pDetails = &m_entities[i];
		if (pDetails->m_id)
		{
			SKotHEntity *pKotHEntity = static_cast<SKotHEntity*>(pDetails->m_pAdditionalData);
			CRY_ASSERT(pKotHEntity);

			pKotHEntity->m_needsIconUpdate = true;
		}
	}
}

//------------------------------------------------------------------------
float CGameRulesKingOfTheHillObjective::CalculateScoreTimer( int playerCount )
{
	float timerLength = m_scoreTimerMaxLength;
	timerLength *= pow(m_scoreTimerAdditionalPlayerMultiplier, playerCount - 1);
	return timerLength;
}
