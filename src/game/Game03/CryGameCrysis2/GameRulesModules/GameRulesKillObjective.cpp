/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of a kill objective (destroy an entity)

	-------------------------------------------------------------------------
	History:
	- 23:09:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesKillObjective.h"
#include "IXml.h"
#include "GameRules.h"
#include "Utility/CryWatch.h"
#include "Audio/AudioSignalPlayer.h"
#include "Utility/StringUtils.h"
#include "HUD/HUD.h" 
#include "Actor.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#if NUM_ASPECTS > 8
	#define KILL_OBJECTIVE_NET_ASPECT		eEA_GameServerA
#else
	#define KILL_OBJECTIVE_NET_ASPECT		eEA_GameServerStatic
#endif

//------------------------------------------------------------------------
void CGameRulesKillObjective::Init( XmlNodeRef xml )
{
	m_friendlyIcon = -1;
	m_hostileIcon = -1;
	m_moduleRMIIndex = -1;
	m_useIcons = false;
	m_maxDistanceSqr = 0.f;
	m_checkRange = false;
	m_lookAtEntityId = 0;

	if (xml->getAttr("maxDistance", m_maxDistanceSqr))
	{
		CryLog("CGameRulesKillObjective::Init, max distance=%f", m_maxDistanceSqr);
		m_maxDistanceSqr *= m_maxDistanceSqr;
		m_checkRange = true;
	}

	int numChildren = xml->getChildCount();
	for (int i = 0; i < numChildren; ++ i)
	{
		XmlNodeRef xmlChild = xml->getChild(i);
		if (!stricmp(xmlChild->getTag(), "Icons"))
		{
			CRY_ASSERT_MESSAGE(!m_useIcons, "We only support having one 'Icons' node");
			m_useIcons = true;
			xmlChild->getAttr("friendly", m_friendlyIcon);
			xmlChild->getAttr("hostile", m_hostileIcon);

			g_pGame->GetGameRules()->RegisterTeamChangedListener(this);
		}
		else if(strcmp(xmlChild->getTag(),"AudioSignal") == 0)
		{
			if(xmlChild->haveAttr("killed"))
			{
				cry_strncpy(m_killedSignal, xmlChild->getAttr("killed"), k_maxAudioSignalLength);
			}
		}
	}

	m_moduleRMIIndex = g_pGame->GetGameRules()->RegisterModuleRMIListener(this);

	memset(m_entities, 0, sizeof(SKillEntity) * KILL_OBJECTIVE_MAX_ENTITIES);
}

//------------------------------------------------------------------------
void CGameRulesKillObjective::Update( float frameTime )
{
	if (gEnv->bClient)
	{
		IActor *pLocalActor = g_pGame->GetIGameFramework()->GetClientActor();
		if (pLocalActor)
		{
			int localTeamId = g_pGame->GetGameRules()->GetTeam(pLocalActor->GetEntityId());
			for (int i = 0; i < 2; ++ i)
			{
				int teamId = i + 1;
				if (m_completingEnabled[i])
				{
					if (teamId == localTeamId)
					{
						// Unfortunately the WorldQuery method of getting the current 'LookAt' target has a max range of 300m which isn't enough for us here
						EntityId lookAtId = 0;

						static const int obj_types = ent_all; // ent_terrain|ent_static|ent_rigid|ent_sleeping_rigid|ent_living;
						static const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;

						CActor *pPlayer = static_cast<CActor*>(pLocalActor);
						IPhysicalEntity *pPhysEnt = pLocalActor->GetEntity()->GetPhysics();

						Vec3 dir(1.f, 0.f, 0.f);
						Vec3 worldPosition(0.f, 0.f, 0.f);

						IMovementController * pMC = pLocalActor->GetMovementController();
						if (pMC)
						{
							SMovementState s;
							pMC->GetMovementState(s);
							worldPosition = s.eyePosition;
							dir = s.eyeDirection;

							ray_hit rayHit;
							bool rayHitAny = (0 != gEnv->pPhysicalWorld->RayWorldIntersection( worldPosition, 3000.0f * dir, obj_types, flags, &rayHit, 1, pPhysEnt ));
							if(rayHitAny)
							{
								IEntity *pLookAt = gEnv->pEntitySystem->GetEntityFromPhysics(rayHit.pCollider);
								if (pLookAt)
								{
									lookAtId = pLookAt->GetId();
								}
							}
						}

						if (lookAtId != m_lookAtEntityId)
						{
							m_lookAtEntityId = lookAtId;
							int targetTeamId = 3 - localTeamId;

							for (int i = 0; i < KILL_OBJECTIVE_MAX_ENTITIES; ++ i)
							{
								SKillEntity *pKillEntity = &m_entities[i];
								if (pKillEntity->m_isActive && (pKillEntity->m_teamId == targetTeamId) && (pKillEntity->m_targetId == m_lookAtEntityId))
								{
									CryLog("Looking at a kill objective");
									// Check range
									if (!CheckRange(pLocalActor->GetEntityId(), pKillEntity->m_targetId))
									{
										g_pGame->GetGameRules()->OnTextMessage(eTextMessageAnnouncement, "@ui_msg_out_of_range");
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesKillObjective::OnStartGame()
{
	for (int i = 0; i < KILL_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SKillEntity *pKillEntity = &m_entities[i];
		SetIcon(pKillEntity);
	}
}

//------------------------------------------------------------------------
void CGameRulesKillObjective::AddEntityId(int type, EntityId entityId, int index, bool isNewEntity)
{
	SKillEntity *pKillEntity = &m_entities[index];
	if (pKillEntity->m_isActive)
	{
		// We're overwriting another entity, disable the old one
		CleanUpEntity(pKillEntity);
	}
	if (gEnv->bServer)
	{
		memset(pKillEntity, 0, sizeof(SKillEntity));
	}

	pKillEntity->m_targetId = entityId;
	pKillEntity->m_teamId = g_pGame->GetGameRules()->GetTeam(entityId);
	pKillEntity->m_currentIcon = -1;

	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(pKillEntity->m_targetId);
	if (pEntity)
	{
		IScriptTable *pEntityScript = pEntity->GetScriptTable();
		if (gEnv->bServer)
		{
			// Get the entity team
			if (!pKillEntity->m_teamId)
			{
				if (pEntityScript)
				{
					// Entity doesn't have a team set yet, see if there is one set in lua
					if (pEntityScript->GetValueType("Properties") == svtObject)
					{
						SmartScriptTable pPropertiesScript;
						if (pEntityScript->GetValue("Properties", pPropertiesScript))
						{
							const char *teamName;
							if (pPropertiesScript->GetValue("teamName", teamName))
							{
								pKillEntity->m_teamId = g_pGame->GetGameRules()->GetTeamId(teamName);
								g_pGame->GetGameRules()->SetTeam(pKillEntity->m_teamId, pKillEntity->m_targetId);
							}
						}
					}
				}
			}
			if (pKillEntity->m_teamId)
			{
				int teamIndex = (3 - pKillEntity->m_teamId) - 1;	// If the entity is on team 2, it is a target for team 1 (which is index 0 in the list)
				++ m_targetsRemainingForTeams[teamIndex];
			}
			g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(g_pGame->GetGameRules()->GetEntityId(), KILL_OBJECTIVE_NET_ASPECT);
		}

		if (pKillEntity->m_teamId)
		{
			pKillEntity->m_isActive = m_completingEnabled[(3 - pKillEntity->m_teamId) - 1];
		}

		IScriptSystem *pScriptSystem = gEnv->pScriptSystem;
		if (pEntityScript)
		{
			// Have to tell the entity to refresh its links due to horrible loading order specific stuff in the editor
			HSCRIPTFUNCTION resetLinksFunc;
			if (pEntityScript->GetValue("ResetLinks", resetLinksFunc))
			{
				Script::Call(pScriptSystem, resetLinksFunc, pEntityScript);
			}
		}

		if (gEnv->bClient && m_useIcons)
		{
			// Check if the entity has proxies for displaying the icon
			if (pEntityScript)
			{
				HSCRIPTFUNCTION countFunc;
				HSCRIPTFUNCTION getIdFunc;
				if (pEntityScript->GetValue("GetProxyIconEntityCount", countFunc) && pEntityScript->GetValue("GetProxyIconEntityId", getIdFunc))
				{
					int proxyCount = 0;
					if (Script::CallReturn(pScriptSystem, countFunc, pEntityScript, proxyCount))
					{
						if (proxyCount >= SKillEntity::MAX_PROXY_ENTITIES)
						{
							proxyCount = SKillEntity::MAX_PROXY_ENTITIES - 1;
						}
						for (int i = 0; i < proxyCount; ++ i)
						{
							ScriptHandle entityHandle;
							EntityId proxyId = 0;
							if (Script::CallReturn(pScriptSystem, getIdFunc, pEntityScript, i, entityHandle))
							{
								proxyId = (EntityId)entityHandle.n;
							}
							pKillEntity->m_proxyEntityIds[i] = proxyId;
						}
						pKillEntity->m_proxyEntityIdCount = proxyCount;
					}
				}
			}

			if (pKillEntity->m_teamId && !pKillEntity->m_isDead)
			{
				int teamIndex = pKillEntity->m_teamId - 1;
				if (!gEnv->bServer)
				{
					++ m_targetsRemainingForTeams[teamIndex];
				}
				SetIcon(pKillEntity);
			}
		}
	}
}

//------------------------------------------------------------------------
bool CGameRulesKillObjective::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (aspect == KILL_OBJECTIVE_NET_ASPECT)
	{
		if (ser.IsReading())
		{
			m_targetsRemainingForTeams[0] = 0;
			m_targetsRemainingForTeams[1] = 0;
		}

		for (int i = 0; i < KILL_OBJECTIVE_MAX_ENTITIES; ++ i)
		{
			SKillEntity *pKillEntity = &m_entities[i];

			bool wasDead = pKillEntity->m_isDead;

			ser.Value("isDead", pKillEntity->m_isDead, 'bool');

			if (ser.IsReading())
			{
				if (!pKillEntity->m_isDead)
				{
					if (pKillEntity->m_teamId)
					{
						++ m_targetsRemainingForTeams[pKillEntity->m_teamId - 1];
					}
				}
				if (wasDead != pKillEntity->m_isDead)
				{
					SetIcon(pKillEntity);
				}
			}
		}
	}
	
	return true;
}

//------------------------------------------------------------------------
CGameRulesKillObjective::CGameRulesKillObjective()
{
	g_pGame->GetGameRules()->RegisterKillListener(this);

	m_targetsRemainingForTeams[0] = 0;
	m_targetsRemainingForTeams[1] = 0;
	m_completingEnabled[0] = false;
	m_completingEnabled[1] = false;
}

//------------------------------------------------------------------------
CGameRulesKillObjective::~CGameRulesKillObjective()
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->UnRegisterKillListener(this);
		pGameRules->UnRegisterModuleRMIListener(m_moduleRMIIndex);

		if (m_useIcons)
		{
			pGameRules->UnRegisterTeamChangedListener(this);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesKillObjective::OnEntityKilled( const HitInfo &hitInfo )
{
	if (!gEnv->bServer)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();

	for (int i = 0; i < KILL_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SKillEntity *pKillEntity = &m_entities[i];

		if (pKillEntity->m_isActive && pKillEntity->m_targetId == hitInfo.targetId)
		{
			int teamId = pGameRules->GetTeam(pKillEntity->m_targetId);
			int playerTeamId = pGameRules->GetTeam(hitInfo.shooterId);
			if (teamId && playerTeamId && teamId != playerTeamId)
			{
				int teamIndex = (3 - teamId) - 1;	// If the entity is on team 1, it is a target for team 2 (which is index 1 in the list)
				if (m_completingEnabled[teamIndex] && !pKillEntity->m_isDead)
				{
					if (CheckRange(hitInfo.shooterId, hitInfo.targetId))
					{
						pKillEntity->m_isDead = true;
						g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(g_pGame->GetGameRules()->GetEntityId(), KILL_OBJECTIVE_NET_ASPECT);

						-- m_targetsRemainingForTeams[teamIndex];

						if (!m_targetsRemainingForTeams[teamIndex])
						{
							CryLog("CGameRulesKillObjective::OnEntityKilled, team %i has killed all its targets", (teamIndex + 1));
							CRY_TODO(23, 09, 2009, "Complete the objective for team 'teamIndex'");
						}

						if (gEnv->bClient)
						{
							SetIcon(pKillEntity);
							EntityKilled(hitInfo.targetId, hitInfo.shooterId);
						}
						CGameRules::SModuleRMITwoEntityParams params(m_moduleRMIIndex, hitInfo.targetId, hitInfo.shooterId, 0);
						pGameRules->GetGameObject()->InvokeRMI(CGameRules::ClModuleRMIDoubleEntity(), params, eRMI_ToRemoteClients);
					}
				}
			}

			break;
		}
	}
}

bool CGameRulesKillObjective::CheckRange(EntityId shooterId, EntityId targetId)
{
	bool targetInRange = true;
	if (m_checkRange)
	{
		// Check distance from target
		IEntity *pShooter = gEnv->pEntitySystem->GetEntity(shooterId);
		IEntity *pTarget = gEnv->pEntitySystem->GetEntity(targetId);
		if (pShooter && pTarget)
		{
			float distanceSqr = pTarget->GetPos().GetSquaredDistance(pShooter->GetPos());
			if (distanceSqr > m_maxDistanceSqr)
			{
				targetInRange = false;
			}
		}
	}
	return targetInRange;
}

//------------------------------------------------------------------------
bool CGameRulesKillObjective::IsComplete( int teamId )
{
	bool complete = false;
	if (teamId == 1 || teamId == 2)
	{
		int targetsRemaining = m_targetsRemainingForTeams[teamId - 1];
		complete = (targetsRemaining == 0);
	}
	return complete;
}

//------------------------------------------------------------------------
void CGameRulesKillObjective::EnableCompletion( int teamId, bool enable )
{
	if (teamId == 1 || teamId == 2)
	{
		m_completingEnabled[teamId - 1] = enable;

		if (gEnv->bClient)
		{
			for (int i = 0; i < KILL_OBJECTIVE_MAX_ENTITIES; ++ i)
			{
				SKillEntity *pKillEntity = &m_entities[i];
				if (pKillEntity->m_teamId == (3 - teamId))
				{
					pKillEntity->m_isActive = enable;
				}
				SetIcon(pKillEntity);
			}
		}

		if (enable && gEnv->bClient)
		{
			ShowHUDMessage(teamId, "@ui_msg_psl_base_vulnerable_0", "@ui_msg_psl_base_vulnerable_1", 0, 0);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesKillObjective::OnChangedTeam( EntityId entityId, int oldTeamId, int newTeamId )
{
	if (entityId == g_pGame->GetIGameFramework()->GetClientActorId())
	{
		for (int i = 0; i < KILL_OBJECTIVE_MAX_ENTITIES; ++ i)
		{
			SKillEntity *pKillEntity = &m_entities[i];
			if (pKillEntity->m_isActive && gEnv->bClient)
			{
				SetIcon(pKillEntity);
			}
		}
	}
	else
	{
		for (int i = 0; i < KILL_OBJECTIVE_MAX_ENTITIES; ++ i)
		{
			SKillEntity *pKillEntity = &m_entities[i];
			if (pKillEntity->m_targetId == entityId)
			{
				if (newTeamId)
				{
					pKillEntity->m_isActive  = m_completingEnabled[(3 - newTeamId) - 1];
				}
				pKillEntity->m_teamId = newTeamId;
				if (gEnv->bClient)
				{
					SetIcon(pKillEntity);
					break;
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesKillObjective::SetIcon( SKillEntity *pKillEntity )
{
	CGameRules *pGameRules = g_pGame->GetGameRules();

	int localTeamId = pGameRules->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());
	int requestedIcon = -1;

	if (pKillEntity->m_isActive && pKillEntity->m_teamId && !pKillEntity->m_isDead)
	{
		int teamIndex = pKillEntity->m_teamId - 1;
		if (m_completingEnabled[1 - teamIndex])	// Entity is a target for the opposing team
		{
			if (pKillEntity->m_teamId == localTeamId)
			{
				// Entity is on the local team, put friendly icon on it
				requestedIcon = m_friendlyIcon;
			}
			else
			{
				// Entity is on the enemy team
				requestedIcon = m_hostileIcon;
			}
		}
	}

	if (requestedIcon != pKillEntity->m_currentIcon)
	{
		CCCPOINT(KillObjective_SetNewIcon);
	
		if (!pKillEntity->m_proxyEntityIdCount)
		{
			if (requestedIcon != -1)
			{
				SHUDEvent newMissionObjective(eHUDEvent_OnNewObjective);
				newMissionObjective.ReserveData(2);
				newMissionObjective.AddData( static_cast<int>(pKillEntity->m_targetId) ); /*(EntityId)*/
				newMissionObjective.AddData( requestedIcon ); /*(EGameRulesMissionObjectives)*/ 
				CHUD::CallEvent(newMissionObjective);
			}
			else
			{
				SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
				newRemoveObjective.ReserveData(1);
				newRemoveObjective.AddData( static_cast<int>(pKillEntity->m_targetId) ); /*(EntityId)*/
				CHUD::CallEvent(newRemoveObjective);
			}
		}
		else
		{
			for (int i = 0; i < pKillEntity->m_proxyEntityIdCount; ++ i)
			{
				EntityId proxyId = pKillEntity->m_proxyEntityIds[i];
				if (requestedIcon != -1)
				{
					SHUDEvent newMissionObjective(eHUDEvent_OnNewObjective);
					newMissionObjective.ReserveData(2);
					newMissionObjective.AddData( static_cast<int>(proxyId) ); /*(EntityId)*/
					newMissionObjective.AddData( requestedIcon ); /*(EGameRulesMissionObjectives)*/ 
					CHUD::CallEvent(newMissionObjective);
				}
				else
				{
					SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
					newRemoveObjective.ReserveData(1);
					newRemoveObjective.AddData( static_cast<int>(proxyId) ); /*(EntityId)*/
					CHUD::CallEvent(newRemoveObjective);
				}
			}
		}
		pKillEntity->m_currentIcon = requestedIcon;
	}
}

//------------------------------------------------------------------------
void CGameRulesKillObjective::ShowHUDMessage( int teamId, const char *pFriendlyMessage, const char *pHostileMessage, EntityId aboutPlayerId, const char *pOwnMessage )
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	int localActorId = g_pGame->GetIGameFramework()->GetClientActorId();

	const char *pArg1 = 0;
	if (aboutPlayerId)
	{
		if (localActorId == aboutPlayerId)
		{
			pGameRules->OnTextMessage(eTextMessageAnnouncement, pOwnMessage);
			return;
		}
		else
		{
			IEntity *pEntity = gEnv->pEntitySystem->GetEntity(aboutPlayerId);
			if (pEntity)
			{
				pArg1 = pEntity->GetName();
			}
		}
	}

	int localTeamId = pGameRules->GetTeam(localActorId);
	if (localTeamId == teamId)
	{
		pGameRules->OnTextMessage(eTextMessageAnnouncement, pFriendlyMessage, pArg1);
	}
	else
	{
		pGameRules->OnTextMessage(eTextMessageAnnouncement, pHostileMessage, pArg1);
	}
}

//------------------------------------------------------------------------
void CGameRulesKillObjective::OnDoubleEntityRMI(CGameRules::SModuleRMITwoEntityParams params)
{
	EntityKilled(params.m_entityId1, params.m_entityId2);
}

//------------------------------------------------------------------------
void CGameRulesKillObjective::EntityKilled(EntityId targetId, EntityId shooterId)
{
	int teamId = 3 - g_pGame->GetGameRules()->GetTeam(targetId);

	ShowHUDMessage(teamId, "@ui_msg_psl_base_killed_0", "@ui_msg_psl_base_killed_1", shooterId, "@ui_msg_psl_base_killed_2");

	CCCPOINT(KillObjective_EntityKilled);
	if(m_killedSignal[0] != '\0')
	{
		CAudioSignalPlayer::JustPlay(m_killedSignal, targetId);
	}
}

//------------------------------------------------------------------------
void CGameRulesKillObjective::RemoveEntityId( int type, EntityId entityId )
{
	for (int i = 0; i < KILL_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SKillEntity *pKillEntity = &m_entities[i];
		if (pKillEntity->m_isActive && pKillEntity->m_targetId == entityId)
		{
			CleanUpEntity(pKillEntity);
			break;
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesKillObjective::ClearEntities(int type )
{
	for (int i = 0; i < KILL_OBJECTIVE_MAX_ENTITIES; ++ i)
	{
		SKillEntity *pKillEntity = &m_entities[i];
		CleanUpEntity(pKillEntity);
	}
}

//------------------------------------------------------------------------
void CGameRulesKillObjective::CleanUpEntity( SKillEntity *pKillEntity )
{
	pKillEntity->m_isActive = false;
	if (pKillEntity->m_teamId && !pKillEntity->m_isDead)
	{
		-- m_targetsRemainingForTeams[(3 - pKillEntity->m_teamId) - 1];
	}
	SetIcon(pKillEntity);
}

//------------------------------------------------------------------------
bool CGameRulesKillObjective::IsEntityFinished( int type, int index )
{
	CRY_ASSERT(index < KILL_OBJECTIVE_MAX_ENTITIES);
	const SKillEntity *pKillEntity = &m_entities[index];
	return pKillEntity->m_isDead;
}
