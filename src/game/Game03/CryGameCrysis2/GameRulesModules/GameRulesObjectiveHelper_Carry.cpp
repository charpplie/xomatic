/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Helper objective, tracks entities that can be carried
		i.e. Javelin missile, Extraction gun, BombTheBase bomb

	-------------------------------------------------------------------------
	History:
	- 26:11:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesObjectiveHelper_Carry.h"
#include "IXml.h"
#include "GameRules.h"
#include "GameRulesModules/IGameRulesScoringModule.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"
#include "Utility/CryWatch.h"
#include "Item.h"
#include "Actor.h"
#include "HUD/HUD.h"

//------------------------------------------------------------------------
CGameRulesObjectiveHelper_Carry::CGameRulesObjectiveHelper_Carry()
{
	m_pEntityClass = NULL;

	m_pickupType = ePT_Proximity;

	m_hostilePickAc = ePA_Pickup;
	m_friendlyPickAc = ePA_Pickup;

	m_hostileCanPickAtBase = true;
	m_friendlyCanPickAtBase = true;

	m_droppedRemoveTime = 0.f;

	m_baseDefenceRadius = 3.0f;
	m_baseDefenceRadiusHeight = 2.5f;

	m_teamEnabled[0] = false;
	m_teamEnabled[1] = false;

	m_removeOnDropped = false;
	m_useSpawnPOIs = false;

	m_moduleRMIIndex = -1;
	m_droppedIconNeutral = int(EGRMO_Unknown);
	m_droppedIconFriendly = int(EGRMO_Unknown);
	m_droppedIconHostile = int(EGRMO_Unknown);
	m_carriedIconFriendly = int(EGRMO_Unknown);
	m_carriedIconHostile = int(EGRMO_Unknown);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::Init( XmlNodeRef xml )
{
	m_teamActiveAlarmSound[0] = m_teamActiveAlarmSound[1] = 0;

	int  iValue = 0;
	float  fValue = 0.f;
	const char*  pCharTmp = 0;

	const char *pClassName = 0;
	if (xml->getAttr("class", &pClassName))
	{
		m_pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(pClassName);
	}
	CRY_ASSERT_MESSAGE(m_pEntityClass, "Failed to find entity class");

	if (xml->getAttr("useSpawnPOIs", iValue))
	{
		m_useSpawnPOIs = (iValue != 0);
	}

	if (xml->getAttr("baseDefenceRadius", fValue))
	{
		m_baseDefenceRadius = fValue;
	}
	if (xml->getAttr("baseDefenceRadiusHeight", fValue))
	{
		m_baseDefenceRadiusHeight = fValue;
	}

	int numChildren = xml->getChildCount();
	for (int i = 0; i < numChildren; ++ i)
	{
		XmlNodeRef xmlChild = xml->getChild(i);
		if (!stricmp(xmlChild->getTag(), "Pickup"))
		{
			if (xmlChild->getAttr("type", &pCharTmp))
			{
				if (!stricmp(pCharTmp, "Proximity"))
				{
					m_pickupType = ePT_Proximity;
				}
				else if (!stricmp(pCharTmp, "Item"))
				{
					m_pickupType = ePT_Item;
				}
			}

			if (xmlChild->getAttr("hostileAction", &pCharTmp))
			{
				if (!stricmp(pCharTmp, "Pickup"))
				{
					m_hostilePickAc = ePA_Pickup;
				}
				else if (!stricmp(pCharTmp, "Return"))
				{
					m_hostilePickAc = ePA_Remove;
				}
				else if (!stricmp(pCharTmp, "Nothing"))
				{
					m_hostilePickAc = ePA_Nothing;
				}
			}

			if (xmlChild->getAttr("friendlyAction", &pCharTmp))
			{
				if (!stricmp(pCharTmp, "Pickup"))
				{
					m_friendlyPickAc = ePA_Pickup;
				}
				else if (!stricmp(pCharTmp, "Remove"))
				{
					m_friendlyPickAc = ePA_Remove;
				}
				else if (!stricmp(pCharTmp, "Nothing"))
				{
					m_friendlyPickAc = ePA_Nothing;
				}
			}

			if (xmlChild->getAttr("hostileCanAtBase", iValue))
			{
				m_hostileCanPickAtBase = (iValue != 0);
			}

			if (xmlChild->getAttr("friendlyCanAtBase", iValue))
			{
				m_friendlyCanPickAtBase = (iValue != 0);
			}
		}
		else if (!stricmp(xmlChild->getTag(), "Remove"))
		{
			if (xmlChild->getAttr("onDropped", iValue))
			{
				m_removeOnDropped = (iValue != 0);
				xmlChild->getAttr("droppedTimerLength", m_droppedRemoveTime);
			}
		}
		else if (!stricmp(xmlChild->getTag(), "DroppedIcons"))
		{
			xmlChild->getAttr("neutral", m_droppedIconNeutral);
			xmlChild->getAttr("friendly", m_droppedIconFriendly);
			xmlChild->getAttr("hostile", m_droppedIconHostile);
		}
		else if (!stricmp(xmlChild->getTag(), "CarriedIcons"))
		{
			xmlChild->getAttr("friendly", m_carriedIconFriendly);
			xmlChild->getAttr("hostile", m_carriedIconHostile);
		}
		else if (!stricmp(xmlChild->getTag(), "Strings"))
		{
			const char *pString = 0;
			if (xmlChild->getAttr("friendlyPickedUp", &pString))
			{
				m_friendlyPickedUpString.Format("@%s", pString);
			}
			if (xmlChild->getAttr("hostilePickedUp", &pString))
			{
				m_hostilePickedUpString.Format("@%s", pString);
			}
			if (xmlChild->getAttr("friendlyDropped", &pString))
			{
				m_friendlyDroppedString.Format("@%s", pString);
			}
			if (xmlChild->getAttr("hostileDropped", &pString))
			{
				m_hostileDroppedString.Format("@%s", pString);
			}
		}
		else if (!stricmp(xmlChild->getTag(), "Sounds"))
		{
			if (xmlChild->getAttr("baseAlarm", &pCharTmp))
			{
				//m_baseAlarmSound.Format("%s", pString);
				m_baseAlarmSound = pCharTmp;
			}
			if (xmlChild->getAttr("enemyPickupFriendly", &pCharTmp))
			{
				m_enemyPickupFriendlySound = pCharTmp;
			}
			if (xmlChild->getAttr("friendlyPickupEnemy", &pCharTmp))
			{
				m_friendlyPickupEnemySound = pCharTmp;
			}
			if (xmlChild->getAttr("enemyPickupEnemy", &pCharTmp))
			{
				m_enemyPickupEnemySound = pCharTmp;
			}
			if (xmlChild->getAttr("friendlyPickupFriendly", &pCharTmp))
			{
				m_friendlyPickupFriendlySound = pCharTmp;
			}
			if (xmlChild->getAttr("enemyComplete", &pCharTmp))
			{
				m_enemyCompleteSound = pCharTmp;
			}
			if (xmlChild->getAttr("friendlyComplete", &pCharTmp))
			{
				m_friendlyCompleteSound = pCharTmp;
			}
		}

	}

	CGameRules *pGameRules = g_pGame->GetGameRules();
	pGameRules->RegisterPickupListener(this);
	pGameRules->RegisterClientConnectionListener(this);
	pGameRules->RegisterKillListener(this);
	pGameRules->RegisterTeamChangedListener(this);
	m_moduleRMIIndex = pGameRules->RegisterModuleRMIListener(this);
	gEnv->pEntitySystem->AddSink(this);
}

//------------------------------------------------------------------------
CGameRulesObjectiveHelper_Carry::~CGameRulesObjectiveHelper_Carry()
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->UnRegisterPickupListener(this);
		pGameRules->UnRegisterClientConnectionListener(this);
		pGameRules->UnRegisterKillListener(this);
		pGameRules->UnRegisterTeamChangedListener(this);
		pGameRules->UnRegisterModuleRMIListener(m_moduleRMIIndex);
	}
	gEnv->pEntitySystem->RemoveSink(this);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::Update( float frameTime )
{
	if (gEnv->bServer)
	{
		if (m_removeOnDropped && (m_droppedRemoveTime > 0.f))
		{
			int  numEntities = m_entities.size();
			for (int i=0; i<numEntities; i++)
			{
				SEntityDetails*  pDetails = &m_entities[i];
				if ((pDetails->m_id) && (pDetails->m_state == eES_Dropped))
				{
					if ((g_pGame->GetGameRules()->GetServerTime() - pDetails->m_droppedTime) > (m_droppedRemoveTime * 1000.f))
					{
						gEnv->pEntitySystem->RemoveEntity(pDetails->m_id);
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnStartGame()
{
	m_teamEnabled[0] = false;
	m_teamEnabled[1] = false;
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnSpawn( IEntity *pEntity,SEntitySpawnParams &params )
{
	if (pEntity->GetClass() == m_pEntityClass)
	{
		int numEntities = m_entities.size();
		m_entities.resize(numEntities + 1);

		SEntityDetails *pEntityDetails = &m_entities[numEntities];
		pEntityDetails->m_id = pEntity->GetId();
		pEntityDetails->m_carrierId = 0;
		pEntityDetails->m_droppedTime = 0.f;
		pEntityDetails->m_state = eES_AtBase;
		pEntityDetails->m_currentIcon = int(EGRMO_Unknown);
		pEntityDetails->m_iconEntityId = 0;

		pEntityDetails->m_baseId = 0;  // will get lazily evaluated by calls to GetBaseEntityIdForCarryEnt()

		UpdateIcon(pEntityDetails);

		if (gEnv->bServer)
		{
			if (m_useSpawnPOIs)
			{
				if (IGameRulesSpawningModule* spawningModule=g_pGame->GetGameRules()->GetSpawningModule())
				{
					spawningModule->AddAvoidPOI(pEntityDetails->m_id, g_pGameCVars->g_spawn_ctfFlagAvoidDist, false);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
bool CGameRulesObjectiveHelper_Carry::OnRemove( IEntity *pEntity )
{
	if (pEntity->GetClass() == m_pEntityClass)
	{
		int numEntities = m_entities.size();
		for (int i = 0; i < numEntities; ++ i)
		{
			SEntityDetails *pEntityDetails = &m_entities[i];
			if (pEntityDetails->m_id == pEntity->GetId())
			{
				int  entTeam = g_pGame->GetGameRules()->GetTeam(pEntity->GetId());

				if ((m_pickupType == ePT_Proximity) && (pEntityDetails->m_state == eES_Carried))
				{
					g_pGame->GetGameRules()->OnPickupEntityDetached(pEntityDetails->m_id, pEntityDetails->m_carrierId, true);
				}

				if (entTeam)
				{
					tSoundID*  pTeamActiveAlarmSound = &m_teamActiveAlarmSound[entTeam - 1];
					if (*pTeamActiveAlarmSound)
					{
						if (ISound* pSound=gEnv->pSoundSystem->GetSound(*pTeamActiveAlarmSound))
							pSound->Stop();
						(*pTeamActiveAlarmSound) = 0;
					}
				}

				if (pEntityDetails->m_iconEntityId)
				{
					SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
					newRemoveObjective.ReserveData(1);
					newRemoveObjective.AddData( static_cast<int>(pEntityDetails->m_iconEntityId) ); /*(EntityId)*/
					CHUD::CallEvent(newRemoveObjective);
				}

				if (gEnv->bServer)
				{
					if (m_useSpawnPOIs)
					{
						if (IGameRulesSpawningModule* spawningModule=g_pGame->GetGameRules()->GetSpawningModule())
						{
							spawningModule->RemovePOI(pEntityDetails->m_id);
						}
					}
				}

				TEntityDetailsVec::iterator it = (m_entities.begin() + i);
				m_entities.erase(it);
				break;
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------
CGameRulesObjectiveHelper_Carry::SEntityDetails * CGameRulesObjectiveHelper_Carry::FindEntityDetails( EntityId entId )
{
	SEntityDetails *pResult = NULL;
	int numEntities = m_entities.size();
	for (int i = 0; i < numEntities; ++ i)
	{
		if (m_entities[i].m_id == entId)
		{
			pResult = &m_entities[i];
			break;
		}
	}
	return pResult;
}

//------------------------------------------------------------------------
bool CGameRulesObjectiveHelper_Carry::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	return true;
}

//------------------------------------------------------------------------
bool CGameRulesObjectiveHelper_Carry::IsComplete( int teamId )
{
	return true;
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::Enable( int teamId, bool enable )
{
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnItemPickedUp( EntityId itemId, EntityId actorId )
{
	OnItemOrEntityPickup(itemId, actorId);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnPickupEntityAttached( EntityId entityId, EntityId actorId )
{
	OnItemOrEntityPickup(entityId, actorId);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnItemOrEntityPickup( EntityId entityId, EntityId actorId )
{
	SEntityDetails *pDetails = FindEntityDetails(entityId);
	if (pDetails)
	{
		int  oldCarrier = pDetails->m_carrierId;

		pDetails->m_carrierId = actorId;
		if (gEnv->bClient)
		{
			ClItemPickedUp(pDetails, oldCarrier);
		}

		if (gEnv->bServer)
		{
			if (m_useSpawnPOIs)
			{
				if (IGameRulesSpawningModule* spawningModule=g_pGame->GetGameRules()->GetSpawningModule())
				{
					spawningModule->EnablePOI(entityId);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnItemDropped( EntityId itemId, EntityId actorId )
{
	OnItemOrEntityDrop(itemId, actorId, false);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnPickupEntityDetached( EntityId entityId, EntityId actorId, bool isOnRemove )
{
	OnItemOrEntityDrop(entityId, actorId, isOnRemove);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnItemOrEntityDrop( EntityId entityId, EntityId actorId, bool isOnRemove )
{
	SEntityDetails *pDetails = FindEntityDetails(entityId);
	if (pDetails)
	{
		pDetails->m_carrierId = 0;
		pDetails->m_droppedTime = g_pGame->GetGameRules()->GetServerTime();

		if (gEnv->bServer)
		{
			if (m_useSpawnPOIs)
			{
				if (IGameRulesSpawningModule* spawningModule=g_pGame->GetGameRules()->GetSpawningModule())
				{
					spawningModule->DisablePOI(entityId);
				}
			}
		}

		bool  remove = (m_removeOnDropped && (m_droppedRemoveTime <= 0.f));

		if (remove && !isOnRemove)
		{
			gEnv->pEntitySystem->RemoveEntity(entityId);
		}

		if (gEnv->bClient)
		{
			ClItemDropped(pDetails, actorId, (/*remove ||*/ isOnRemove));
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnEvent( IEntity *pEntity, SEntityEvent &event )
{
	if (gEnv->bServer && m_pickupType == ePT_Proximity && pEntity->GetClass() == m_pEntityClass)
	{
		if (event.event == ENTITY_EVENT_ENTERAREA)
		{
			EntityId insideId = (EntityId) event.nParam[0];

			CActor *pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(insideId));
			if (pActor && pActor->IsPlayer() && pActor->GetHealth() > 0 && pActor->GetSpectatorState() == CActor::eASS_Ingame && pActor->GetSpectatorMode() == CActor::eASM_None)
			{
				SEntityDetails *pDetails = FindEntityDetails(pEntity->GetId());
				if (pDetails && (pDetails->m_state != eES_Carried))
				{
					int  entTeam = g_pGame->GetGameRules()->GetTeam(pEntity->GetId());
					int  plyrTeam = g_pGame->GetGameRules()->GetTeam(insideId);
					bool  sameTeam = (entTeam == plyrTeam);

					if ((entTeam == 0) || (pDetails->m_state != eES_AtBase) || ((sameTeam && m_friendlyCanPickAtBase) || (!sameTeam && m_hostileCanPickAtBase)))
					{
						if ((entTeam == 0) || (sameTeam && (m_friendlyPickAc == ePA_Pickup) || (!sameTeam && (m_hostilePickAc == ePA_Pickup))))
						{
							// Pickup entity
							pDetails->m_state = eES_Carried;

							g_pGame->GetGameRules()->OnPickupEntityAttached(pDetails->m_id, insideId);

							CGameRules::SModuleRMITwoEntityParams params;
							params.m_listenerIndex = m_moduleRMIIndex;
							params.m_entityId1 = pDetails->m_id;
							params.m_entityId2 = insideId;
							params.m_data = 0;
							g_pGame->GetGameRules()->GetGameObject()->InvokeRMI(CGameRules::ClModuleRMIDoubleEntity(), params, eRMI_ToRemoteClients);

							//if (m_scoringEnabled[pTargetEntity->m_objectiveTeamId - 1])  // TODO? need the equiv of this here?
							{
								if (IGameRulesScoringModule* pScoringModule=g_pGame->GetGameRules()->GetScoringModule())
								{
									pScoringModule->OnPlayerScoringEvent(insideId, EGRST_CarryObjectiveTaken);
								}
							}
						}
						else if (sameTeam && (m_friendlyPickAc == ePA_Remove) || (!sameTeam && (m_hostilePickAc == ePA_Remove)))
						{
							// Remove/return entity

							// Providing <Objective type='Helper_Spawn'><Reset onRemoved='1'/></Objective> is set then this has the effect of Returning stolen pickups to base
							gEnv->pEntitySystem->RemoveEntity(pEntity->GetId());

							//if (m_scoringEnabled[pTargetEntity->m_objectiveTeamId - 1])  // TODO? need the equiv of this here?
							{
								if (IGameRulesScoringModule* pScoringModule=g_pGame->GetGameRules()->GetScoringModule())
								{
									pScoringModule->OnPlayerScoringEvent(insideId, EGRST_CarryObjectiveRetrieved);

									// bonus score if retrieve carry objective whilst carrying another
									int  numEntities = m_entities.size();
									for (int i=0; i<numEntities; i++)
									{
										if (m_entities[i].m_carrierId == insideId)
										{
											pScoringModule->OnPlayerScoringEvent(insideId, EGRST_CarryObjectiveRetrieved);
											break;
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
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnEntityKilled( const HitInfo &hitInfo )
{
	bool  dropped = CheckForAndDropItem(hitInfo.targetId);

	//if (m_scoringEnabled[pTargetEntity->m_objectiveTeamId - 1])  // TODO? need the equiv of this here?
	{
		if (IGameRulesScoringModule* pScoringModule=g_pGame->GetGameRules()->GetScoringModule())
		{
			CActor*  pShooter = static_cast< CActor* >( g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.shooterId));
			if (pShooter && pShooter->IsPlayer())
			{
				if (dropped)
				{
					// bonus points for killing an enemy objective carrier
					if (!pShooter->IsFriendlyEntity(hitInfo.targetId))
					{
						pScoringModule->OnPlayerScoringEvent(hitInfo.shooterId, EGRST_CarryObjectiveCarrierKilled);
					}
				}
				else
				{
					if (IEntity* pTarget=gEnv->pEntitySystem->GetEntity(hitInfo.targetId))
					{
						// bonus points for killing an enemy near own base while carry objective is there
						int  numEntities = m_entities.size();
						for (int i=0; i<numEntities; i++)
						{
							SEntityDetails*  ed = &m_entities[i];
							if (ed->m_state == eES_AtBase)
							{
								if (IEntity* pEntity=gEnv->pEntitySystem->GetEntity(ed->m_id))
								{
									float  heightdiff = cry_fabsf(pEntity->GetWorldPos().z - pTarget->GetWorldPos().z);
									if (heightdiff < m_baseDefenceRadiusHeight)
									{
										float  dist2dsq = (pEntity->GetWorldPos() - pTarget->GetWorldPos()).GetLengthSquared2D();
										if (dist2dsq < (m_baseDefenceRadius * m_baseDefenceRadius))
										{
											pScoringModule->OnPlayerScoringEvent(hitInfo.shooterId, EGRST_CarryObjectiveDefended);
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
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnClientDisconnect( int channelId, EntityId playerId )
{
	CheckForAndDropItem(playerId);
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnChangedTeam( EntityId entityId, int oldTeamId, int newTeamId )
{
	int numEntities = m_entities.size();

	if (entityId == g_pGame->GetIGameFramework()->GetClientActorId())
	{
		for (int i = 0; i < numEntities; ++ i)
		{
			SEntityDetails *pDetails = &m_entities[i];
			UpdateIcon(pDetails);
		}
	}
	else
	{
		for (int i = 0; i < numEntities; ++ i)
		{
			SEntityDetails *pDetails = &m_entities[i];
			if (pDetails->m_id == entityId)
			{
				UpdateIcon(pDetails);
				break;
			}
		}
	}
}

//------------------------------------------------------------------------
bool CGameRulesObjectiveHelper_Carry::CheckForAndDropItem( EntityId playerId )
{
	bool dropped = false;
	int numEntities = m_entities.size();
	for (int i = 0; i < numEntities; ++ i)
	{
		SEntityDetails *pDetails = &m_entities[i];
		if (pDetails->m_carrierId == playerId)
		{
			// Drop entity
			if (m_pickupType == ePT_Proximity)
			{
				pDetails->m_state = eES_Dropped;

				g_pGame->GetGameRules()->OnPickupEntityDetached(pDetails->m_id, playerId, false);

				CGameRules::SModuleRMIEntityParams params;
				params.m_listenerIndex = m_moduleRMIIndex;
				params.m_entityId = pDetails->m_id;
				params.m_data = 0;
				g_pGame->GetGameRules()->GetGameObject()->InvokeRMI(CGameRules::ClModuleRMISingleEntity(), params, eRMI_ToRemoteClients);
				dropped = true;
			}
			else
			{
				IItem *pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(pDetails->m_id);
				if (pItem && g_pGame->GetIGameFramework()->GetNetContext())		// Don't bother dropping it if we're the one disconnecting
				{
					pItem->Drop();
					dropped = true;
				}
				if (gEnv->bClient)
				{
					ClItemDropped(pDetails, playerId, false);
				}
			}
		}
	}
	return dropped;
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnOwnClientEnteredGame()
{
	int numEntities = m_entities.size();
	for (int i = 0; i < numEntities; ++ i)
	{
		SEntityDetails *pDetails = &m_entities[i];
		UpdateIcon(pDetails);
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnClientEnteredGame( int channelId, bool isReset, EntityId playerId )
{
	if (m_pickupType == ePT_Proximity)
	{
		int numEntities = m_entities.size();
		for (int i = 0; i < numEntities; ++ i)
		{
			SEntityDetails *pDetails = &m_entities[i];
			if (pDetails->m_state == eES_Carried)
			{
				if (pDetails->m_carrierId == playerId)
				{
					// Entity is being carried by a migrating player
					CGameRules::SModuleRMITwoEntityParams params;
					params.m_listenerIndex = m_moduleRMIIndex;
					params.m_entityId1 = pDetails->m_id;
					params.m_entityId2 = pDetails->m_carrierId;
					params.m_data = 0;
					g_pGame->GetGameRules()->GetGameObject()->InvokeRMIWithDependentObject(CGameRules::ClModuleRMIDoubleEntity(), params, eRMI_ToRemoteClients, pDetails->m_id);
				}
				else
				{
					CGameRules::SModuleRMITwoEntityParams params;
					params.m_listenerIndex = m_moduleRMIIndex;
					params.m_entityId1 = pDetails->m_id;
					params.m_entityId2 = pDetails->m_carrierId;
					params.m_data = 0;
					g_pGame->GetGameRules()->GetGameObject()->InvokeRMIWithDependentObject(CGameRules::ClModuleRMIDoubleEntity(), params, eRMI_ToClientChannel|eRMI_NoLocalCalls, pDetails->m_id, channelId);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::CallScriptFunction( IEntity *pEntity, const char *pFuncName, EntityId otherEnt )
{
	IScriptTable *pScript = pEntity->GetScriptTable();
	if (pScript && pScript->GetValueType(pFuncName) == svtFunction)
	{
		IScriptSystem *pScriptSystem = gEnv->pScriptSystem;
		pScriptSystem->BeginCall(pScript, pFuncName);
		pScriptSystem->PushFuncParam(pScript);

		if (otherEnt)
		{
			IEntity *pOtherEnt = gEnv->pEntitySystem->GetEntity(otherEnt);
			if (pOtherEnt && pOtherEnt->GetScriptTable())
			{
				pScriptSystem->PushFuncParam(pOtherEnt->GetScriptTable());
			}
		}
		pScriptSystem->EndCall();
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnSingleEntityRMI( CGameRules::SModuleRMIEntityParams params )
{
	// SingleEntityRMI means an item has been dropped
	SEntityDetails *pDetails = FindEntityDetails(params.m_entityId);
	if (pDetails)
	{
		EntityId previousCarrierId = pDetails->m_carrierId;
		pDetails->m_state = eES_Dropped;

		if (m_pickupType == ePT_Proximity)
		{
			g_pGame->GetGameRules()->OnPickupEntityDetached(pDetails->m_id, previousCarrierId, false);
		}
		else
		{
			pDetails->m_carrierId = 0;
			if (IEntity* pEntity=gEnv->pEntitySystem->GetEntity(pDetails->m_id))
				CallScriptFunction(pEntity, "AttachTo", 0);
			ClItemDropped(pDetails, previousCarrierId, false);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::OnDoubleEntityRMI( CGameRules::SModuleRMITwoEntityParams params )
{
	// DoubleEntityRMI means an item has been picked up
	SEntityDetails *pDetails = FindEntityDetails(params.m_entityId1);
	if (pDetails)
	{
		pDetails->m_state = eES_Carried;

		if (m_pickupType == ePT_Proximity)
		{
			g_pGame->GetGameRules()->OnPickupEntityAttached(params.m_entityId1, params.m_entityId2);
		}
		else
		{
			int  oldCarrier = pDetails->m_carrierId;
			pDetails->m_carrierId = params.m_entityId2;
			if (IEntity* pEntity=gEnv->pEntitySystem->GetEntity(params.m_entityId1))
				CallScriptFunction(pEntity, "AttachTo", params.m_entityId2);
			ClItemPickedUp(pDetails, oldCarrier);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::UpdateIcon( SEntityDetails *pEntityDetails )
{
	if (pEntityDetails->m_id)
	{
		CGameRules *pGameRules = g_pGame->GetGameRules();
		EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
		int localTeam = pGameRules->GetTeam(localActorId);

		int requiredIcon = -1;
		int iconEntity = 0;

		if (pEntityDetails->m_carrierId)
		{
			if (pEntityDetails->m_carrierId != localActorId)
			{
				iconEntity = pEntityDetails->m_carrierId;
				int carrierTeamId = pGameRules->GetTeam(pEntityDetails->m_carrierId);

				if (carrierTeamId == localTeam)
				{
					requiredIcon = m_carriedIconFriendly;
				}
				else
				{
					requiredIcon = m_carriedIconHostile;
				}
			}
		}
		else
		{
			iconEntity = pEntityDetails->m_id;

			int entityTeamId = pGameRules->GetTeam(pEntityDetails->m_id);

			if (entityTeamId == 0)
			{
				requiredIcon = m_droppedIconNeutral;
			}
			else if (entityTeamId == localTeam)
			{
				requiredIcon = m_droppedIconFriendly;
			}
			else
			{
				requiredIcon = m_droppedIconHostile;
			}
		}

		if (requiredIcon != pEntityDetails->m_currentIcon || iconEntity != pEntityDetails->m_iconEntityId)
		{
			// Remove old one
			if (pEntityDetails->m_currentIcon != int(EGRMO_Unknown))
			{
				SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
				newRemoveObjective.ReserveData(1);
				newRemoveObjective.AddData( static_cast<int>(pEntityDetails->m_iconEntityId) ); /*(EntityId)*/
				CHUD::CallEvent(newRemoveObjective);
			}

			pEntityDetails->m_iconEntityId = iconEntity;
			pEntityDetails->m_currentIcon = requiredIcon;

			if (pEntityDetails->m_currentIcon != int(EGRMO_Unknown))
			{
				SHUDEvent newMissionObjective(eHUDEvent_OnNewObjective);
				newMissionObjective.ReserveData(2);
				newMissionObjective.AddData( static_cast<int>(pEntityDetails->m_iconEntityId) ); /*(EntityId)*/
				newMissionObjective.AddData( pEntityDetails->m_currentIcon ); /*(EGameRulesMissionObjectives)*/ 
				CHUD::CallEvent(newMissionObjective);
			}
		}

		// inform the HUD of carry object/flag status
		{
			int  entTeam = g_pGame->GetGameRules()->GetTeam(pEntityDetails->m_id);
			SHUDEvent  e (eHUDEvent_OnStartCarryingFlag);
			e.eventIntData = entTeam;
			e.eventIntData2 = pEntityDetails->m_carrierId;
			CHUD::CallEvent(e);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::ProcessSoundForCarryStateChange( SEntityDetails* pEntityDetails, const int oldCarrierId, const bool isOnRemove )
{
	const int  newCarrierId = pEntityDetails->m_carrierId;
	//if (newCarrierId != oldCarrierId)
	{
		CGameRules*  pGameRules = g_pGame->GetGameRules();
		if (int entTeam=g_pGame->GetGameRules()->GetTeam(pEntityDetails->m_id))
		{
			const char*  eventSnd = NULL;
			const char*  alarmSnd = NULL;
			Vec3  eventPos;
			Vec3  alarmPos;
			IEntity*  pCarrEnt = gEnv->pEntitySystem->GetEntity(pEntityDetails->m_id);
			CRY_ASSERT(pCarrEnt);
			if (pCarrEnt)
			{
				EntityId  localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
				int  localTeam = pGameRules->GetTeam(localActorId);
				if (newCarrierId)
				{
					int  carrTeam = pGameRules->GetTeam(newCarrierId);
					if (carrTeam != entTeam)
					{
						if (carrTeam != localTeam)
							eventSnd = m_enemyPickupFriendlySound;
						else if (carrTeam == localTeam)
							eventSnd = m_friendlyPickupEnemySound;
						if (eventSnd)
							eventPos = pCarrEnt->GetWorldPos();
						//if (entTeam == localTeam)  // FIXME!!!!!!!! TMP!!!!!!!!!!!!!!!!!1
						{
							if (IEntity* pBaseEnt=gEnv->pEntitySystem->GetEntity(GetBaseEntityIdForCarryEnt(pEntityDetails)))
							{
								alarmSnd = m_baseAlarmSound;
								if (alarmSnd)
								{
									alarmPos = pBaseEnt->GetWorldPos();
								}
							}
						}
					}
				}
				else
				{
					if (pEntityDetails->m_state == eES_Dropped)
					{
						/*
						int  oldCarrTeam = pGameRules->GetTeam(newCarrierId);
						if (oldCarrTeam != entTeam)
						{
							if (oldCarrTeam != localTeam)
								eventSnd = m_enemyDroppedFriendlySound;
							else if (oldCarrTeam == localTeam)
							{
								eventSnd = m_friendlyDroppedEnemySound;
							}
						}
						else
						{
							if (oldCarrTeam != localTeam)
								eventSnd = m_enemyDroppedEnemySound;
							else if (oldCarrTeam == localTeam)
							{
								eventSnd = m_friendlyDroppedFriendlySound;
							}
						}
						if (eventSnd)
						{
							eventPos = pCarrEnt->GetWorldPos();
						}
						*/
						if (entTeam == localTeam)
						{
							if (IEntity* pBaseEnt=gEnv->pEntitySystem->GetEntity(GetBaseEntityIdForCarryEnt(pEntityDetails)))
							{
								alarmSnd = m_baseAlarmSound;
								if (alarmSnd)
								{
									alarmPos = pBaseEnt->GetWorldPos();
								}
							}
						}
					}
				}
			}
			if (eventSnd)
			{
				CAudioSignalPlayer::JustPlay(eventSnd, eventPos);
			}
			UpdateTeamAlarmSound(entTeam, alarmSnd, &alarmPos);
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::UpdateTeamAlarmSound(const int alarmTeam, const char newSnd[], const Vec3* newPos)
{
	CRY_ASSERT(alarmTeam > 0);
	tSoundID*  pTeamActiveAlarmSound = &m_teamActiveAlarmSound[alarmTeam - 1];
	if (newSnd && newPos)
	{
		if (ISound* pSound=gEnv->pSoundSystem->CreateSound(newSnd, FLAG_SOUND_DEFAULT_3D))
		{
			CRY_ASSERT(!stricmp(newSnd,pSound->GetName()));
			if (*pTeamActiveAlarmSound)
			{
				if (ISound* pCurSound=gEnv->pSoundSystem->GetSound(*pTeamActiveAlarmSound))
				{
					pCurSound->Stop();
				}
				(*pTeamActiveAlarmSound) = 0;
			}
			pSound->SetPosition(*newPos);
			pSound->SetSemantic(eSoundSemantic_Mechanic_Entity);
			pSound->Play();
			(*pTeamActiveAlarmSound) = pSound->GetId();
			CRY_ASSERT(*pTeamActiveAlarmSound);
		}
	}
	else
	{
		if (*pTeamActiveAlarmSound)
		{
			if (ISound* pSound=gEnv->pSoundSystem->GetSound(*pTeamActiveAlarmSound))
				pSound->Stop();
			(*pTeamActiveAlarmSound) = 0;
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::ClItemPickedUp( SEntityDetails *pEntityDetails, const int oldCarrierId )
{
	UpdateIcon(pEntityDetails);

	ProcessSoundForCarryStateChange(pEntityDetails, oldCarrierId, false);
	
	IEntity *pCarrier = gEnv->pEntitySystem->GetEntity(pEntityDetails->m_carrierId);
	if (pCarrier)
	{
		CGameRules *pGameRules = g_pGame->GetGameRules();

		int carrierTeam = pGameRules->GetTeam(pEntityDetails->m_carrierId);
		int localTeam = pGameRules->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());

		if (carrierTeam == localTeam && !m_friendlyPickedUpString.empty())
		{
			pGameRules->OnTextMessage(eTextMessageAnnouncement, m_friendlyPickedUpString.c_str(), pCarrier->GetName());
		}
		else if (carrierTeam != localTeam && !m_hostilePickedUpString.empty())
		{
			pGameRules->OnTextMessage(eTextMessageAnnouncement, m_hostilePickedUpString.c_str(), pCarrier->GetName());
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesObjectiveHelper_Carry::ClItemDropped( SEntityDetails *pEntityDetails, EntityId previousCarrierId, bool isOnRemove )
{
	if (!m_removeOnDropped || (m_droppedRemoveTime > 0.f))
	{
		UpdateIcon(pEntityDetails);
	}

	ProcessSoundForCarryStateChange(pEntityDetails, previousCarrierId, isOnRemove);

	if (!isOnRemove)
	{
		IEntity *pCarrier = gEnv->pEntitySystem->GetEntity(previousCarrierId);
		if (pCarrier)
		{
			CGameRules *pGameRules = g_pGame->GetGameRules();

			int carrierTeam = pGameRules->GetTeam(previousCarrierId);
			int localTeam = pGameRules->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId());

			if (carrierTeam == localTeam && !m_friendlyDroppedString.empty())
			{
				pGameRules->OnTextMessage(eTextMessageAnnouncement, m_friendlyDroppedString.c_str(), pCarrier->GetName());
			}
			else if (carrierTeam != localTeam && !m_hostileDroppedString.empty())
			{
				pGameRules->OnTextMessage(eTextMessageAnnouncement, m_hostileDroppedString.c_str(), pCarrier->GetName());
			}
		}
	}
}

//------------------------------------------------------------------------
// static
EntityId CGameRulesObjectiveHelper_Carry::S_GetBaseEntityIdForCarryEnt( const EntityId carryEid, EntityId* baseEid )
{
	CRY_ASSERT(baseEid);
	if (!(*baseEid))
	{
		IEntity*  pEnt = gEnv->pEntitySystem->GetEntity(carryEid);
		CRY_ASSERT(pEnt);
		const char*  entName = pEnt->GetName();
		const char*  ptr = strstr(entName, "__");
		CRY_ASSERT_MESSAGE(ptr, "The 'Helper_Spawn' objective needs \"prefixSpawnAtName='1'\" set (in the Gamemode XML).");
		if (ptr)
		{
			const int  baseNameSz = 32;
			int  baseNameLen = (int) (ptr - entName);
			CRY_ASSERT(baseNameLen < baseNameSz);
			if (baseNameLen < baseNameSz)
			{
				char  baseName[baseNameSz];
				memcpy(baseName, entName, baseNameLen);
				baseName[baseNameLen] = '\0';
				IEntity*  pBase = gEnv->pEntitySystem->FindEntityByName(baseName);
				CRY_ASSERT(pBase);
				(*baseEid) = pBase->GetId();
			}
		}
	}
	CRY_ASSERT(*baseEid);
	return (*baseEid);
}

//------------------------------------------------------------------------
EntityId CGameRulesObjectiveHelper_Carry::GetBaseEntityIdForCarryEnt( SEntityDetails* pEntityDetails )
{
	return S_GetBaseEntityIdForCarryEnt(pEntityDetails->m_id, &pEntityDetails->m_baseId);
}
