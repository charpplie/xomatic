/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 23:5:2006   9:27 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "ScriptBind_GameRules.h"
#include "GameRules.h"
#include "Game.h"
#include "GameCVars.h"
#include "Actor.h"
#include "Player.h"

#include "IVehicleSystem.h"
#include "IItemSystem.h"
#include "IMaterialEffects.h"

#include "WeaponSystem.h"
#include "Audio/GameAudio.h"
#include "Audio/AudioSignalPlayer.h"
#include "IWorldQuery.h"

#include "Effects/GameEffects/ExplosionGameEffect.h"
#include "IMovieSystem.h"
#include "HUD/HUDMissionObjectiveSystem.h"
#include "HUD/HUD.h"
#include "HUD/HUD_Radar.h"
#include "Utility/CryWatch.h"

#include <StlUtils.h>
#include <Cry_Math.h>

#include "GameForceFeedback.h"

#include "GameRulesModules/IGameRulesPlayerSetupModule.h"
#include "GameRulesModules/IGameRulesDamageHandlingModule.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"
#include "GameRulesModules/IGameRulesVictoryConditionsModule.h"
#include "GameRulesModules/IGameRulesSpectatorModule.h"
#include "GameRulesModules/IGameRulesAssistScoringModule.h"
#include "GameRulesModules/IGameRulesTeamChangedListener.h"
#include "GameRulesModules/IGameRulesModuleRMIListener.h"
#include "GameRulesModules/IGameRulesClientConnectionListener.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"

#include "EquipmentLoadout.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#include "Projectile.h"

#include "Battlechatter.h"
#include "PlayerProgression.h"
#include "HitDeathReactions.h"
#include "PersistantStats.h"

#include "HUD/UI/UIButtonPromptRegion.h"

//------------------------------------------------------------------------
void CGameRules::ClientSimpleHit(const SimpleHitInfo &simpleHitInfo)
{
	if (!simpleHitInfo.remote)
	{
		if (!gEnv->bServer)
			GetGameObject()->InvokeRMI(SvRequestSimpleHit(), simpleHitInfo, eRMI_ToServer);
		else
			ServerSimpleHit(simpleHitInfo);
	}
}

//------------------------------------------------------------------------
void CGameRules::ClientHit(const HitInfo &hitInfo)
{
	// [*DavidR | 8/Dec/2009] WARNING: This function is called from a collision event in an ammo class
	// Unfortunately, collisions are not guaranteed to happen in both server and client side. So if we
	// are calling this ClientHit from the side that originated the hit we take it into account, other-
	// wise is dismissed (we don't "ServerProcess" the hit, but we still send this to the hit listeners)

	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
	IEntity *pTarget = m_pEntitySystem->GetEntity(hitInfo.targetId);
	IEntity *pShooter =	m_pEntitySystem->GetEntity(hitInfo.shooterId);
	IVehicle *pVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(hitInfo.targetId);
	IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.targetId);

//CryLogAlways ("ClientHit: %s damaged %s", pShooter ? pShooter->GetName() : "NULL", pTarget ? pTarget->GetName() : "NULL");
//CryWatch3DAdd(GetHitType(hitInfo.type), hitInfo.pos, 2.f, NULL, -1.f);
	CPlayer *pPlayer = (pActor && pActor->IsPlayer()) ? static_cast<CPlayer*>(pActor) : NULL;

	int pActorHealth = 0;
	bool dead = false;
	if(pActor)
	{
		pActorHealth = pActor->GetHealth();
		dead = (pActorHealth <= 0);
	}

	SanityCheckHitInfo(hitInfo, "CGameRules::ClientHit");

	if((pClientActor && pClientActor->GetEntity()==pShooter) && pTarget && (pVehicle || pActor) && !dead)
	{
		SHUDEvent hudEvent;
		hudEvent.eventType = eHUDEvent_OnHitTarget;
		hudEvent.eventIntData2 = eHUDEventHT_Bullet;

		if (pVehicle)
		{
			hudEvent.eventIntData = EGRTT_Neutral;
		}
		else
		{
			assert (pClientActor->IsPlayer());
			hudEvent.eventIntData = ((CPlayer*)(pClientActor))->IsFriendlyEntity(hitInfo.targetId) ? EGRTT_Friendly : EGRTT_Hostile;
		}

		CHUD::CallEvent(hudEvent);

		if(pActor && gEnv->bMultiplayer && hitInfo.targetId != hitInfo.shooterId)
		{
			if(pPlayer && pPlayer->IsHeadShot(hitInfo))
			{
				CAudioSignalPlayer::JustPlay("HitTargetHeadshot", pTarget->GetId());
			}
			else
			{
				CAudioSignalPlayer::JustPlay("HitTarget", pTarget->GetId());
			}
			
		}
	}

	if(pPlayer)
	{
		pPlayer->SendPerkEvent(EPE_DamageHandlingFeedback, (void *) &hitInfo);

		if(pActor->IsClient())
		{
			const char* hit = GetHitType(hitInfo.type);
			if(hit)
			{
				string hitSound;
				hitSound.Format("ClientDamage%s", hit);
				CAudioSignalPlayer::JustPlay(hitSound.c_str(), pActor->GetEntityId());
			}
		}
	}

	IActor *pShooterActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.shooterId);
	if(pShooterActor && pShooterActor->IsPlayer())
	{
		static_cast<CPlayer*>(pShooterActor)->SendPerkEvent(EPE_DamageHandlingFeedback, (void *) &hitInfo);
	}

	if(pShooter && pActor)
	{
		if(!dead && (pActorHealth < (pActor->GetMaxHealth()/2)))
		{
			BATTLECHATTER(BC_LowHealth, hitInfo.targetId);
		}
	}


	if(!gEnv->pSystem->IsDedicated())
	{
		CreateScriptHitInfo(m_scriptHitInfo, hitInfo);
		CallScript(m_clientStateScript, "OnHit", m_scriptHitInfo);
	}

	bool backface = hitInfo.dir.Dot(hitInfo.normal)>0;
	if (!hitInfo.remote && hitInfo.targetId && !backface)
	{
		HitInfo hitToSend(hitInfo);

		if (hitToSend.projectileId)
		{
			uint16 projectileClass = ~uint16(0);
			CProjectile *pProjectile = g_pGame->GetWeaponSystem()->GetProjectile(hitToSend.projectileId);
			if (pProjectile)
			{
				g_pGame->GetIGameFramework()->GetNetworkSafeClassId(projectileClass, pProjectile->GetEntity()->GetClass()->GetName());
			}
			hitToSend.projectileClassId = projectileClass;
		}

		CPersistantStats::GetInstance()->ClientHit(pPlayer, hitInfo);

		if (!gEnv->bServer)
		{
			ProcessLocalHit(hitToSend);
			GetGameObject()->InvokeRMI(SvRequestHit(), hitToSend, eRMI_ToServer);
		}
		else
		{
			ServerHit(hitToSend);
			CRY_TODO(18, 12, 2009, "The code below is commented because we don't want too many RMI calls (293450).");
			//GetGameObject()->InvokeRMI(ClRequestHit(), hitToSend,  eRMI_ToRemoteClients);
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::ServerSimpleHit(const SimpleHitInfo &simpleHitInfo)
{
	switch (simpleHitInfo.type)
	{
	case 0: // tag
		{
			if (!simpleHitInfo.targetId)
				return;

			// tagged entities are temporary in MP, not in SP.
			bool temp = gEnv->bMultiplayer;

			ERadarTagReason reason = eRTR_General;
			if (!stricmp(GetEntity()->GetClass()->GetName(), "Assault")) // Need a better way to get current gamemode than a strcmp
				reason = eRTR_AssaultBinoculars;

			AddTaggedEntity(simpleHitInfo.shooterId, simpleHitInfo.targetId, temp, 15.f, reason);
		}
		break;
	case 1: // tac
		{
/*
			// NOTE: This is redundant, since the implementation of CanSleep always return false.

			if (!simpleHitInfo.targetId)
				return;

			CActor *pActor = (CActor *)gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(simpleHitInfo.targetId);

			IActorComponent_OldActor* pOldActorCmp = pActor ? pActor->GetComponent<IActorComponent_OldActor>() : NULL;
			if (pOldActorCmp && pOldActorCmp->CanSleep())
				pOldActorCmp->Fall(Vec3(0.0f,0.0f,0.0f),false,simpleHitInfo.value);
*/
		}
		break;
	case 2:	// fall and play
		{
			KnockActorDown( simpleHitInfo.targetId );
		}
		break;

	default:
		assert(!"Unknown Simple Hit type!");
	}
}


//------------------------------------------------------------------------
void CGameRules::KnockActorDown( EntityId actorEntityId )
{
	// Forbid fall and play if the actor is playing a hit/death reaction
	CActor* pHitActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor( actorEntityId ));
	if (pHitActor)
	{
		if (pHitActor->GetActorClass() == CPlayer::GetActorClassType())
		{
			// Don't trigger Fall and Play if the actor is playing a hit reaction
			CPlayer* pHitPlayer = static_cast<CPlayer*>(pHitActor);
			CHitDeathReactionsConstPtr pHitDeathReactions = pHitPlayer->GetHitDeathReactions();
			if (!pHitDeathReactions || !pHitDeathReactions->IsInReaction())
				pHitActor->Fall();
		}
		else
			pHitActor->Fall();
	}
}


//------------------------------------------------------------------------
void CGameRules::ServerHit(const HitInfo& hitInfo)
{
	// This may be called from other places (Vehicle code) and not directly from ClientHit or RMI.
	//	In such cases, we go through the same process of queuing up the hit, minus any specific handling
	//	cases as defined from Client Hit. This shouldn't be virtual and outside Gamerules should have
	//	a more generic way of requesting hits! (Kevin)
	if (m_processingHit)
	{
		m_queuedHits.push(hitInfo);
		return;
	}

	++m_processingHit;

	ProcessServerHit(hitInfo);

	while (!m_queuedHits.empty())
	{
		HitInfo info(m_queuedHits.front());
		ProcessServerHit(info);
		m_queuedHits.pop();
	}

	--m_processingHit;
}

//------------------------------------------------------------------------
void CGameRules::ProcessServerHit(const HitInfo& hitInfo)
{
	bool bHandleRequest = true;

	if (hitInfo.targetId)
	{
		IActor *pTarget = GetActorByEntityId(hitInfo.targetId);
		if (pTarget && pTarget->GetSpectatorMode())
			bHandleRequest = false;
	}

	if (bHandleRequest)
	{
		if (g_pGameCVars->g_hitDeathReactions_disable_ai)
		{
			if (gEnv->pAISystem && hitInfo.targetId && hitInfo.shooterId)
			{
				ISurfaceType *pSurfaceType = GetHitMaterial(hitInfo.material);
				const ISurfaceType::SSurfaceTypeAIParams* pParams = pSurfaceType ? pSurfaceType->GetAIParams() : 0;
				const float fHitRadius = pParams ? pParams->fImpactRadius : 2.5f;
				/*const float fSoundRadius = pParams ? pParams->fImpactSoundRadius : 20.0f;*/

				SAIStimulus stim(AISTIM_BULLET_HIT, 0, hitInfo.shooterId, hitInfo.targetId, hitInfo.pos, hitInfo.dir, fHitRadius);
				gEnv->pAISystem->RegisterStimulus(stim);

				/*SAIStimulus stimSound(AISTIM_SOUND, AISTIM_BULLET_HIT, m_ownerId, 0, pCollision->pt, ZERO, fSoundRadius, AISTIMPROC_NO_UPDATE_MEMORY);
				gEnv->pAISystem->RegisterStimulus(stimSound);*/
			}
			//=========================================~ Notify AI ===============================
		}


		IActor *pTarget = GetActorByEntityId(hitInfo.targetId);

		if(pTarget)
		{
			static_cast<CActor*>(pTarget)->GetDamageEffectController()->OnHit(&hitInfo);
		}

		if (GetDamageHandlingModule())
		{
			GetDamageHandlingModule()->SvOnHit(hitInfo);
		}
		else
		{
			CreateScriptHitInfo(m_scriptHitInfo, hitInfo);
			CallScript(m_serverStateScript, "OnHit", m_scriptHitInfo);
		}

		ProcessLocalHit(hitInfo);

		// call hit listeners if any
		if (m_hitListeners.empty() == false)
		{
			THitListenerVec::iterator iter = m_hitListeners.begin();
			THitListenerVec::iterator end = m_hitListeners.end();
			for(; iter != end; ++iter)
				(*iter)->OnHit(hitInfo);
		}

		IActor *pShooter = GetActorByEntityId(hitInfo.shooterId);
		if (pShooter && hitInfo.shooterId!=hitInfo.targetId && hitInfo.weaponId!=hitInfo.shooterId && hitInfo.weaponId!=hitInfo.targetId && hitInfo.damage>=0)
		{
			EntityId params[2];
			params[0] = hitInfo.weaponId;
			params[1] = hitInfo.targetId;
			m_pGameplayRecorder->Event(pShooter->GetEntity(), GameplayEvent(eGE_WeaponHit, 0, 0, (void *)params));
		}

		if (pShooter)
			m_pGameplayRecorder->Event(pShooter->GetEntity(), GameplayEvent(eGE_Hit, 0, 0, (void *)hitInfo.weaponId));

		if (pShooter)
			m_pGameplayRecorder->Event(pShooter->GetEntity(), GameplayEvent(eGE_Damage, 0, hitInfo.damage, (void *)hitInfo.weaponId));
			
		// knockdown is delayed because when the actor is ragdolized, all pending physic events on that actor are flushed. If the hit was part of a barrage, the other hits would be lost
		if (hitInfo.knocksDown && !IsDead(hitInfo.targetId))
			m_pendingActorsToBeKnockedDown.push_back( hitInfo.targetId ); 
	}
}

//------------------------------------------------------------------------
void CGameRules::ProcessLocalHit(const HitInfo& hitInfo)
{
	// [*DavidR | 8/Dec/2009]: Place the code that should be invoked in both server and client sides here
	IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.targetId);
	if (pActor && (pActor->GetActorClass() == CPlayer::GetActorClassType()))
	{
		CHitDeathReactionsPtr pHitDeathReactions = static_cast<CPlayer*>(pActor)->GetHitDeathReactions();
		if (pHitDeathReactions && pHitDeathReactions->OnHit(hitInfo))
			return;
	}

	// If HitDeathReactions hasn't processed this local hit, add a physic impulse
	AddLocalHitImpulse(hitInfo);
}

//------------------------------------------------------------------------
void CGameRules::AddLocalHitImpulse(const HitInfo& hitInfo)
{
	// play an impulse using hit data (similar to old code on SinglePlayer.lua:ProcessActorDamage)
	if (!gEnv->bMultiplayer)
	{
		IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.targetId);
		IEntityPhysicalProxy* pPhysicsProxy = pActor ? static_cast<IEntityPhysicalProxy*>(pActor->GetEntity()->GetProxy(ENTITY_PROXY_PHYSICS)) : NULL;
		if (pPhysicsProxy && !pActor->IsPlayer() && !pActor->IsDead() && !pActor->GetLinkedVehicle() && (hitInfo.type != GetHitTypeId("melee")))
		{
			pPhysicsProxy->AddImpulse(hitInfo.partId, hitInfo.pos, hitInfo.dir * min(200.0f, hitInfo.damage), true, 1.0f);
		}
	}
}

//------------------------------------------------------------------------
int CGameRules::GetFreeExplosionIndex()
{
	int index = -1;
	for(int i = 0; i < MAX_CONCURRENT_EXPLOSIONS; i++)
	{
		if(!m_explosionValidities[i])
		{
			index = i;
			break;
		}
	}

	return index;
}

//------------------------------------------------------------------------

void CGameRules::QueueExplosion(const ExplosionInfo &explosionInfo)
{
	CRY_ASSERT_TRACE (explosionInfo.type, ("Queueing an explosion with an invalid hit type! (effect='%s'/'%s')", explosionInfo.effect_name.c_str(), explosionInfo.effect_class.c_str()));

	int index = GetFreeExplosionIndex();

	if(index != -1)
	{
		m_explosions[index] = explosionInfo;
		m_explosionValidities[index] = true;
		m_explosions[index].raycastHelper.SetReceiver(&m_explosions[index]);

		m_queuedExplosions.push(&m_explosions[index]);
	}
	else
	{
		assert(!"Out of explosion indicies to use!");
	}
}

//------------------------------------------------------------------------
void CGameRules::ProcessServerExplosion(ExplosionInfo &explosionInfo)
{
	if (!gEnv->bServer )
	{
		assert(0);
		return;
	}

  //CryLog("[ProcessServerExplosion] (frame %i) shooter %i, damage %.0f, radius %.1f", gEnv->pRenderer->GetFrameID(), explosionInfo.shooterId, explosionInfo.damage, explosionInfo.radius);

  GetGameObject()->InvokeRMI(ClExplosion(), explosionInfo, eRMI_ToRemoteClients);
  ClientExplosion(explosionInfo);  
}

//------------------------------------------------------------------------
void CGameRules::ProcessQueuedExplosions()
{
  const static uint8 nMaxExp = 1;

	if(gEnv->bServer)
	{
		for (uint8 exp=0; !m_queuedExplosions.empty() && exp<nMaxExp; ++exp)
		{ 
			ExplosionInfo& info = *m_queuedExplosions.front();
			ProcessServerExplosion(info);

			if(info.deferredRaycastState == eDeferredRaycastState_Dispatched)
			{
				m_queuedExplosionsAwaitingLinetests.push(&info);
			}

			m_queuedExplosions.pop();
		}
	}
	else
	{
		for (uint8 exp=0; !m_queuedExplosions.empty() && exp<nMaxExp; ++exp)
		{ 
			ExplosionInfo& info = *m_queuedExplosions.front();
			ClientExplosion(info);

			if(info.deferredRaycastState == eDeferredRaycastState_Dispatched)
			{
				m_queuedExplosionsAwaitingLinetests.push(&info);
			}

			m_queuedExplosions.pop();
		}
	}

	ProcessDeferredMaterialEffects();
}

//------------------------------------------------------------------------
void CGameRules::ProcessDeferredMaterialEffects()
{
	if(m_pExplosionGameEffect)
	{
		const uint32 numExplosionsAwaitingLinetests = m_queuedExplosionsAwaitingLinetests.size();
		for (uint32 i = 0; i < numExplosionsAwaitingLinetests; i++)
		{
			ExplosionInfo * pInfo = m_queuedExplosionsAwaitingLinetests.front();
			if(pInfo->deferredRaycastState >= eDeferredRaycastState_ResultImpact)
			{
				ExplosionInfo& info = *pInfo;
				m_pExplosionGameEffect->SpawnMaterialEffect(info);
				m_queuedExplosionsAwaitingLinetests.pop();
				
				int index = (int)(pInfo - m_explosions);
				m_explosionValidities[index]	= false;
				m_explosions[index].raycastHelper.CancelPendingRays();
			}
		}
	}
}


//------------------------------------------------------------------------
void CGameRules::CullEntitiesInExplosion(const ExplosionInfo &explosionInfo)
{
	if (!g_pGameCVars->g_ec_enable || explosionInfo.damage <= 0.1f)
		return;

	IPhysicalEntity **pents;
	float radiusScale = g_pGameCVars->g_ec_radiusScale;
	float minVolume = g_pGameCVars->g_ec_volume;
	float minExtent = g_pGameCVars->g_ec_extent;
	int   removeThreshold = max(1, g_pGameCVars->g_ec_removeThreshold);

	IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();

	Vec3 radiusVec(radiusScale * explosionInfo.physRadius);
	int i = gEnv->pPhysicalWorld->GetEntitiesInBox(explosionInfo.pos-radiusVec,explosionInfo.pos+radiusVec,pents, ent_rigid|ent_sleeping_rigid);
	int removedCount = 0;

	static IEntityClass* s_pInteractiveEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("InteractiveEntity");
	static IEntityClass* s_pDeadBodyClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DeadBody");

	if (i > removeThreshold)
	{
		int entitiesToRemove = i - removeThreshold;
		for(--i;i>=0;i--)
		{
			if(removedCount>=entitiesToRemove)
				break;

			IEntity * pEntity = (IEntity*) pents[i]->GetForeignData(PHYS_FOREIGN_ID_ENTITY);
			if (pEntity)
			{
				// don't remove items/pickups
				if (g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(pEntity->GetId()))
				{
					continue;
				}
				// don't remove enemies/ragdolls
				if (g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId()))
				{
					continue;
				}

				// if there is a flowgraph attached, never remove!
				if (pEntity->GetProxy(ENTITY_PROXY_FLOWGRAPH) != 0)
					continue;

				IEntityClass* pClass = pEntity->GetClass();
				if (pClass == s_pInteractiveEntityClass || pClass == s_pDeadBodyClass)
					continue;

				// get bounding box
				if (IEntityPhysicalProxy* pPhysProxy = (IEntityPhysicalProxy*)pEntity->GetProxy(ENTITY_PROXY_PHYSICS))
				{
					AABB aabb;
					pPhysProxy->GetWorldBounds(aabb);

					// don't remove objects which are larger than a predefined minimum volume
					if (aabb.GetVolume() > minVolume)
						continue;

					// don't remove objects which are larger than a predefined minimum volume
					Vec3 size(aabb.GetSize().abs());
					if (size.x > minExtent || size.y > minExtent || size.z > minExtent)
						continue;
				}

				// marcok: somehow editor doesn't handle deleting non-dynamic entities very well
				// but craig says, hiding is not synchronized for DX10 breakable MP, so we remove entities only when playing pure game
				// alexl: in SinglePlayer, we also currently only hide the object because it could be part of flowgraph logic
				//        which would break if Entity was removed and could not propagate events anymore
				if (gEnv->bMultiplayer == false || gEnv->IsEditor())
				{
					pEntity->Hide(true);
				}
				else
				{
					gEnv->pEntitySystem->RemoveEntity(pEntity->GetId());
				}
				removedCount++;
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::ClientExplosion(ExplosionInfo &explosionInfo)
{
	// let 3D engine know about explosion (will create holes and remove vegetation)
	if (explosionInfo.hole_size > 1.0f && gEnv->p3DEngine->GetIVoxTerrain())
	{
		gEnv->p3DEngine->OnExplosion(explosionInfo.pos, explosionInfo.hole_size, true);
	}

	TExplosionAffectedEntities affectedEntities;

	if (gEnv->bServer)
  {
		CullEntitiesInExplosion(explosionInfo);
		pe_explosion explosion;
		explosion.epicenter = explosionInfo.pos;
		explosion.rmin = explosionInfo.minRadius;
		explosion.rmax = explosionInfo.radius;
		if (explosion.rmax==0)
			explosion.rmax=0.0001f;
		explosion.r = explosion.rmin;
		explosion.impulsivePressureAtR = explosionInfo.pressure;
		explosion.epicenterImp = explosionInfo.pos;
		explosion.explDir = explosionInfo.dir;
		explosion.nGrow = 2;
		explosion.rminOcc = 0.07f;

		// Might skip shooter (for pinger, we might need multiple and more "general")
		IPhysicalEntity* pSkipEntity = GetEntityToSkipByExplosion(explosionInfo);

		// we separate the calls to SimulateExplosion so that we can define different radii for AI and physics bodies
		explosion.holeSize = 0.0f;
		explosion.nOccRes = explosion.rmax>50.0f ? 0:16;
		gEnv->pPhysicalWorld->SimulateExplosion( &explosion, &pSkipEntity , pSkipEntity? 1 : 0, ent_living);

		CreateScriptExplosionInfo(m_scriptExplosionInfo, explosionInfo);
		UpdateAffectedEntitiesSet(affectedEntities, &explosion);

		// check vehicles
		IVehicleSystem *pVehicleSystem = g_pGame->GetIGameFramework()->GetIVehicleSystem();
		uint32 vcount = pVehicleSystem->GetVehicleCount();
		if (vcount > 0)
		{
			IVehicleIteratorPtr iter = g_pGame->GetIGameFramework()->GetIVehicleSystem()->CreateVehicleIterator();
			while (IVehicle* pVehicle = iter->Next())
			{
				if(IEntity *pEntity = pVehicle->GetEntity())
				{
					AABB aabb;
					pEntity->GetWorldBounds(aabb);
					IPhysicalEntity* pEnt = pEntity->GetPhysics();
					if (pEnt && aabb.GetDistanceSqr(explosionInfo.pos) <= explosionInfo.radius*explosionInfo.radius)
					{
						float affected = gEnv->pPhysicalWorld->CalculateExplosionExposure(&explosion, pEnt);
						AddOrUpdateAffectedEntity(affectedEntities, pEntity, affected);
					}
				}
			}
		}
	
		explosion.rmin = explosionInfo.minPhysRadius;
		explosion.rmax = explosionInfo.physRadius;
		if (explosion.rmax==0)
			explosion.rmax=0.0001f;
		explosion.r = explosion.rmin;
		explosion.holeSize = explosionInfo.hole_size;
		explosion.nOccRes = -1;	// makes second call re-use occlusion info
		gEnv->pPhysicalWorld->SimulateExplosion( &explosion, &pSkipEntity, pSkipEntity ? 1 : 0, ent_rigid | ent_sleeping_rigid | ent_independent|ent_static | ent_delayed_deformations);

		UpdateAffectedEntitiesSet(affectedEntities, &explosion);
		RemoveFriendlyAffectedEntities(explosionInfo, affectedEntities);

		if (!GetDamageHandlingModule())
		{
			CommitAffectedEntitiesSet(m_scriptExplosionInfo, affectedEntities);

			CallScript(m_serverStateScript, "OnExplosion", m_scriptExplosionInfo);    
		}
		else
		{
			GetDamageHandlingModule()->SvOnExplosion(explosionInfo, affectedEntities);
		}

		// call hit listeners if any
		if (m_hitListeners.empty() == false)
		{
			THitListenerVec::iterator iter = m_hitListeners.begin();
			while (iter != m_hitListeners.end())
			{
				(*iter)->OnServerExplosion(explosionInfo);
				++iter;
			}
		}
  }

	if (gEnv->bClient)
	{
		if (!gEnv->bServer)
		{
			CreateScriptExplosionInfo(m_scriptExplosionInfo, explosionInfo);
		}
		else
		{
			affectedEntities.clear();
			CommitAffectedEntitiesSet(m_scriptExplosionInfo, affectedEntities);
		}

		CallScript(m_clientStateScript, "OnExplosion", m_scriptExplosionInfo);

		// call hit listeners if any
		if (m_hitListeners.empty() == false)
		{
			THitListenerVec::iterator iter = m_hitListeners.begin();
			while (iter != m_hitListeners.end())
			{
				(*iter)->OnExplosion(explosionInfo);
				++iter;
			}
		}

		//Pinger prototype
		NotifyPingExplosion(explosionInfo);

		IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
		if (pClientActor)
		{
			assert (pClientActor->IsPlayer());
			CPlayer* pPlayer = static_cast<CPlayer*>(pClientActor);

			Vec3 playerPos = pClientActor->GetEntity()->GetWorldPos();
			float distSq = (playerPos - explosionInfo.pos).len2();

			if(distSq < explosionInfo.radius * explosionInfo.radius)
			{
				HitInfo tempHitInfo(explosionInfo.shooterId, pClientActor->GetEntityId(), explosionInfo.weaponId, 0.f, explosionInfo.radius, 0, 0, explosionInfo.type);
				pPlayer->SendPerkEvent(EPE_DamageHandlingFeedback, & tempHitInfo);
				if(!gEnv->bMultiplayer)
				{
					// this will work as long as the values defined for this mood in the .xml are 0 for fadein, and some value >0 for the fadeout
					CAudioSignalPlayer::JustPlay("Player_Explosion", pPlayer->GetEntityId());
					CAudioSignalPlayer::JustPlay("Player_StopExplosionMood", pPlayer->GetEntityId()); 
				}
			}
		}
	}

	if(m_pExplosionGameEffect)
	{
		m_pExplosionGameEffect->Explode(explosionInfo);
	}

	if (gEnv->pAISystem)
	{
		// Associate event with vehicle if the shooter is in a vehicle (tank cannon shot, etc)
		EntityId ownerId = explosionInfo.shooterId;
		IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(ownerId);
		if (pActor && pActor->GetLinkedVehicle() && pActor->GetLinkedVehicle()->GetEntityId())
			ownerId = pActor->GetLinkedVehicle()->GetEntityId();

		if (ownerId != 0)
		{
			SAIStimulus stim(AISTIM_EXPLOSION, 0, ownerId, 0,
				explosionInfo.pos, ZERO, explosionInfo.radius);
			gEnv->pAISystem->RegisterStimulus(stim);

			float fSoundRadius = explosionInfo.soundRadius;
			if (fSoundRadius <= FLT_EPSILON)
			{
				fSoundRadius = explosionInfo.radius * 3.0f;
			}

			SAIStimulus stimSound(AISTIM_SOUND, AISOUND_EXPLOSION, ownerId, 0,
				explosionInfo.pos, ZERO, fSoundRadius, AISTIMPROC_FILTER_LINK_WITH_PREVIOUS);
			gEnv->pAISystem->RegisterStimulus(stimSound);
		}
	}
}

//-------------------------------------------
void CGameRules::SuccessfulFlashBang(const ExplosionInfo &explosionInfo)
{
	GetGameObject()->InvokeRMI(SvSuccessfulFlashBang(), EntityParams(explosionInfo.shooterId), eRMI_ToServer);
}

//-----------------------------------------------------
IPhysicalEntity* CGameRules::GetEntityToSkipByExplosion(const ExplosionInfo& explosionInfo) const
{
	// Beni - Explosion for the pinger (perhaps later add a new field/flag to explosionInfo for more general usage, it could return more than one)
	if(explosionInfo.type == GetHitTypeId("pingerPing"))
	{
		if(IEntity* pEntity = gEnv->pEntitySystem->GetEntity(explosionInfo.shooterId))
		{
			return pEntity->GetPhysics();
		}
	}
	else if(explosionInfo.type == GetHitTypeId("alienDropPodBounce"))
	{
		// Weapon Id should be the Id of the drop pod. We ignore the drop pod so we don't alter its flight/bounce path.
		if(IEntity* pEntity = gEnv->pEntitySystem->GetEntity(explosionInfo.weaponId))
		{
			return pEntity->GetPhysics();
		}
	}

	return NULL;
}

//-----------------------------------------------------
void CGameRules::NotifyPingExplosion(const ExplosionInfo& explosionInfo)
{
	//This will trigger a flow-graph node output, if the pinger has one assigned
	if(explosionInfo.type == GetHitTypeId("pingerPing"))
	{
		if(IEntity* pEntity = gEnv->pEntitySystem->GetEntity(explosionInfo.shooterId))
		{
			SEntityEvent flowEvent;
			flowEvent.event = ENTITY_EVENT_ACTIVE_FLOW_NODE_OUTPUT;
			const char* flowNodeOutput = "Ping_Now";
			flowEvent.nParam[0] = (INT_PTR)flowNodeOutput;

			//if(!strcmp(functionCall, "OnNanoToolActivated"))
			{
				bool vTrue = true;		
				flowEvent.nParam[1] = IEntityClass::EVT_BOOL;
				flowEvent.nParam[2] = (INT_PTR)&vTrue;
				pEntity->SendEvent(flowEvent);
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------
// RMI
//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvRequestRename)
{
	IActor *pActor = GetActorByEntityId(params.entityId);
	if (!pActor)
		return true;

	RenamePlayer(pActor, params.name.c_str());

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClRenameEntity)
{
	IEntity *pEntity=gEnv->pEntitySystem->GetEntity(params.entityId);
	if (pEntity)
	{
		string old=pEntity->GetName();
		pEntity->SetName(params.name.c_str());

		CryLogAlways("$8%s$o renamed to $8%s", old.c_str(), params.name.c_str());
		CCCPOINT(GameRules_ClRenameEntity);

		/* // Integrate with New FF HUD, may be needed. /FH
		// if this was a remote player, check we're not spectating them.
		//	If we are, we need to trigger a spectator hud update for the new name
		EntityId clientId = g_pGame->GetIGameFramework()->GetClientActorId();
		if(gEnv->bMultiplayer && params.entityId != clientId)
		{
			CActor* pClientActor = static_cast<CActor *>(g_pGame->GetIGameFramework()->GetClientActor());
			if(pClientActor && pClientActor->GetSpectatorMode() == CActor::eASM_Follow && pClientActor->GetSpectatorTarget() == params.entityId && g_pGame->GetHUD())
			{
				g_pGame->GetHUD()->RefreshSpectatorHUDText();
			}
		}
		*/
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvRequestChatMessage)
{
	SendChatMessage((EChatMessageType)params.type, params.sourceId, params.targetId, params.msg.c_str());

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClChatMessage)
{
	OnChatMessage((EChatMessageType)params.type, params.sourceId, params.targetId, params.msg.c_str(), params.onlyTeam);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvRequestChangeTeam)
{
	IActor *pActor = GetActorByEntityId(params.entityId);
	if (!pActor)
		return true;

	ChangeTeam(pActor, params.teamId);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvRequestSpectatorMode)
{
	IActor *pActor = GetActorByEntityId(params.entityId);
	if (!pActor)
		return true;

	IGameRulesSpectatorModule*  specmod = GetSpectatorModule();
	if (!specmod)
		return true;

	//revived since last died
	IGameRulesSpawningModule* spawnMod = GetSpawningModule();
	if(spawnMod)
	{
		const IGameRulesSpawningModule::TPlayerDataMap* playerData = spawnMod->GetPlayerValuesMap();
		
		IGameRulesSpawningModule::TPlayerDataMap::const_iterator  it = playerData->find(params.entityId);
		if (it != playerData->end())
		{
			if(it->second.lastRevivedTime >= it->second.deathTime)
			{
				return true;
			}
		}
	}
	
	specmod->ChangeSpectatorMode(pActor, params.mode, params.targetId, params.resetAll);
	

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetTeam)
{
	ClDoSetTeam(params.teamId, params.entityId);

	return true;
}

//------------------------------------------------------------------------
void CGameRules::ClDoSetTeam(int teamId, EntityId entityId)
{
	if (!entityId) // ignore these for now
		return;

	int oldTeam = GetTeam(entityId);
	if (oldTeam==teamId)
		return;

	stl::member_find_and_erase(m_entityteams, entityId);

	IActor *pActor=m_pActorSystem->GetActor(entityId);
	bool isplayer = (pActor != 0) && (pActor->IsPlayer());
	if (isplayer && oldTeam)
	{
		TPlayerTeamIdMap::iterator pit=m_playerteams.find(oldTeam);
		assert(pit!=m_playerteams.end());
		stl::find_and_erase(pit->second, entityId);
	}

	if (teamId)
	{
		m_entityteams.insert(TEntityTeamIdMap::value_type(entityId, teamId));
		if (isplayer)
		{
			TPlayerTeamIdMap::iterator pit=m_playerteams.find(teamId);
			assert(pit!=m_playerteams.end());
			pit->second.push_back(entityId);
		}
	}

	IEntity * entity = gEnv->pEntitySystem->GetEntity(entityId);
	if (!entity)
	{
		CryLog("CGameRules::ClDoSetTeam, tried to set team on NULL entity, id=%i", entityId);
		return;
	}

	CryLogAlways("[RS] CLIENT: Entity '%s' has been set to be on team number %d", entity->GetName(), teamId);

	// Doing nothing so commented out for now
//	if(IActor *pClient = g_pGame->GetIGameFramework()->GetClientActor())
//	{
//		if(GetTeam(pClient->GetEntityId()) == params.teamId)
//		{
//			if(params.entityId == pClient->GetGameObject()->GetWorldQuery()->GetLookAtEntityId())
//			{
//				//CHUD_Interactive
//				// Integrate - TODO : Check why this was forcing a cross hair update in this situation (?) and remove /FH.
//			}
//		}
//	}

	if(isplayer)
	{
#ifndef OLD_VOICE_SYSTEM_DEPRECATED
		ReconfigureVoiceGroups(entityId,oldTeam,teamId);
#endif
	}

	ScriptHandle handle(entityId);
	CallScript(m_clientStateScript, "OnSetTeam", handle, teamId);

	IScriptTable *pEntityScript = entity->GetScriptTable();
	if (pEntityScript)
	{
		if (pEntityScript->GetValueType("OnSetTeam") == svtFunction)
		{
			m_pScriptSystem->BeginCall(pEntityScript, "OnSetTeam");
			m_pScriptSystem->PushFuncParam(pEntityScript);
			m_pScriptSystem->PushFuncParam(teamId);
			m_pScriptSystem->EndCall();
		}
	}

	int numListeners = m_teamChangedListeners.size();
	for (int i = 0; i < numListeners; ++ i)
	{
		m_teamChangedListeners[i]->OnChangedTeam(entityId, oldTeam, teamId);
	}

	IGameObject* pGameObject = m_pGameFramework->GetGameObject(entityId);

	if(pGameObject)
	{
		STeamChangeInfo teamChangeInfo = {oldTeam, teamId};
		pGameObject->SendEvent(SGameObjectEvent(eCGE_SetTeam, eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, & teamChangeInfo));
	}

	if (teamId && !oldTeam)
	{
		// Entity is being put onto a team for the first time, add an event listener
		gEnv->pEntitySystem->AddEntityEventListener(entityId, ENTITY_EVENT_DONE, this);
	}
	else if (!teamId && oldTeam)
	{
		// Entity is being removed from a team, don't need an event listener anymore
		gEnv->pEntitySystem->RemoveEntityEventListener(entityId, ENTITY_EVENT_DONE, this);
	}
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClTextMessage)
{
	OnTextMessage((ETextMessageType)params.type, params.msg.c_str(), 
		params.params[0].empty()?0:params.params[0].c_str(),
		params.params[1].empty()?0:params.params[1].c_str(),
		params.params[2].empty()?0:params.params[2].c_str(),
		params.params[3].empty()?0:params.params[3].c_str()
		);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClNetConsoleCommand)
{
	CRY_TODO(05, 02, 2010, "This needs to be completely #if'd out for final release.");
#if !defined(_RELEASE)
	gEnv->pConsole->ExecuteString(params.m_commandString.c_str());
#endif

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvRequestSimpleHit)
{
	ServerSimpleHit(params);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvRequestHit)
{
	HitInfo info(params);
	info.remote=true;

	ServerHit(info);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClExplosion)
{
	QueueExplosion(params);

	return true;
}

//-----------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClPostInit)
{
	m_gameStartedTime = params.gameStartedTime;
	m_roundEndTime = params.roundEndTime;
	m_preRoundEndTime = params.preRoundEndTime;
	m_reviveCycleEndTime = params.reviveCycleEndTime;

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetGameStartedTime)
{
	m_gameStartedTime = params.time;

	return true;
}
//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetRoundTime)
{
	m_roundEndTime = params.time;

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetPreRoundTime)
{
	m_preRoundEndTime = params.time;

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetReviveCycleTime)
{
	m_reviveCycleEndTime = params.time;

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetGameStartTimer)
{
	m_gameStartTime = params.time;

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClTaggedEntity)
{
	if (!params.entityId)
		return true;

	//SAFE_HUD_FUNC(GetRadar()->AddTaggedEntity(params.entityId)); //we have no tagging anymore, just temp and non-temp adding
	SAFE_HUD_FUNC(GetRadar()->AddEntityToRadar(params.entityId));

	SEntityEvent scriptEvent( ENTITY_EVENT_SCRIPT_EVENT );
	scriptEvent.nParam[0] = (INT_PTR)"OnGPSTagged";
	scriptEvent.nParam[1] = IEntityClass::EVT_BOOL;
	bool bValue = true;
	scriptEvent.nParam[2] = (INT_PTR)&bValue;

	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(params.entityId);
	if (pEntity)
		pEntity->SendEvent( scriptEvent );

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClTempRadarEntity)
{
	CryLog("[tlh] @ (CGameRules RMI) ClTempRadarEntity()");

	SHUDEvent  eventTempAddToRadar(eHUDEvent_TemporarilyTrackEntity);
	eventTempAddToRadar.AddData(SHUDEventData((int)params.targetId));
	eventTempAddToRadar.AddData(SHUDEventData(15.0f));
	CHUD::CallEvent(eventTempAddToRadar);

	// TODO this needs re-implementing with the above event system too
#if 0
	if (params.m_reason == eRTR_AssaultBinoculars)
		SAFE_HUD_FUNC(GetRadar()->AddBinocularEntityTemporarily(params.targetId, 15.0f));
#endif

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvRequestTempRadarEntity)
{
	CryLog("[tlh] @ (CGameRules RMI) SvRequestTempRadarEntity()");

	assert(gEnv->bServer);
	RequestAddTempRadarEntity(params.shooterId, params.targetId, params.m_time);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClInformTaggedByCCTV)
{
	CryLog("[tlh] @ (CGameRules RMI) ClInformTaggedByCCTV()");

	IActor  *meAct = m_pGameFramework->GetClientActor();
	if (meAct)
	{
		IActor  *taggerAct = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(params.taggerId);

		if (taggerAct && taggerAct->GetEntity())
			OnTextMessage(/*eTextMessageTopRed*/eTextMessageAnnouncement, "@ui_hud_spec_beencctvtagged", taggerAct->GetEntity()->GetName());
		else
			OnTextMessage(/*eTextMessageTopRed*/eTextMessageAnnouncement, "@ui_msg_tagged_by_cctv");

		// [Tomas] TODO please avoid hardcoded sound references, use Game Audio Signal System instead
		_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound("Sounds/crysiswars2:interface:multiplayer/mp_cctv_spotted_fp", FLAG_SOUND_EVENT);  // [tlh] TODO this sound needs to be specified in data somewhere
		if (pSound)
		{
			pSound->SetSemantic(eSoundSemantic_HUD);
			pSound->Play();
		}
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetObjective)
{
	CHUDMissionObjective *pObjective = SAFE_HUD_FUNC_RET(GetMissionObjectiveSystem().GetMissionObjective(params.name.c_str()));
	if(pObjective)
	{
		pObjective->SetStatus((CHUDMissionObjective::HUDMissionStatus)params.status);
		pObjective->SetTrackedEntity(params.entityId);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetObjectiveStatus)
{
	CHUDMissionObjective *pObjective = SAFE_HUD_FUNC_RET(GetMissionObjectiveSystem().GetMissionObjective(params.name.c_str()));
	if(pObjective)
		pObjective->SetStatus((CHUDMissionObjective::HUDMissionStatus)params.status);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetObjectiveEntity)
{
	CHUDMissionObjective *pObjective = SAFE_HUD_FUNC_RET(GetMissionObjectiveSystem().GetMissionObjective(params.name.c_str()));
	if(pObjective)
		pObjective->SetTrackedEntity(params.entityId);

	return true;
}

//------------------------------------------------------------------------
// NB : Also see ActorDamageEffectController.h
IMPLEMENT_RMI(CGameRules, ClDamageIndicator)
{
	//Vec3 dir(ZERO);
	//bool vehicle=false;

	//IEntity *pShootoerEntity=gEnv->pEntitySystem->GetEntity(params.shooterId);
	IActor *pClientActor = m_pGameFramework->GetClientActor();
	if(pClientActor)
	{
		pClientActor = pClientActor->GetHealth() > 0 ? pClientActor : NULL; // Is alive.
	}

	/*if (pShootoerEntity)
	{
		if (pClientActor)
		{
			dir=(pClientActor->GetEntity()->GetWorldPos()-pShootoerEntity->GetWorldPos());
			dir.NormalizeSafe();

			vehicle = (pClientActor->GetLinkedVehicle() !=0 );
		}
	}*/

	{
		const Vec3& vecToUse = params.dir;

		SHUDEvent hitEvent(eHUDEvent_OnShowHitIndicator);
		hitEvent.ReserveData(3);
		hitEvent.AddData(vecToUse.x);
		hitEvent.AddData(vecToUse.y);
		hitEvent.AddData(vecToUse.z);
		CHUD::CallEvent(hitEvent);
	}

	CActor *pActor = static_cast<CActor *>(pClientActor);
	if (pActor)
	{
		IEntityClass *pProjectileClass = NULL;
		if(params.projectileClassId != 0xFFFF)
		{
			char projectileClassName[129]={0};
			if (g_pGame->GetIGameFramework()->GetNetworkSafeClassName(projectileClassName, 128, params.projectileClassId))
			{
				pProjectileClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(projectileClassName);
			}
		}

		const char * damageTypeName = GetHitType(params.hitTypeId);

		CRY_ASSERT_TRACE (damageTypeName, ("%s '%s' is taking damage of an unknown type %u - not informing entity it's been hit!", pActor->GetEntity()->GetClass()->GetName(), pActor->GetEntity()->GetName(), params.hitTypeId));

		if (damageTypeName)
		{
			pActor->DamageInfo(params.shooterId, params.weaponId, pProjectileClass, params.damage, damageTypeName, params.dir);
		}
	}

	//SAFE_HUD_FUNC(ShowTargettingAI(params.shooterId));
	return true;
}

//------------------------------------------------------------------------

IMPLEMENT_RMI(CGameRules, SvVote)
{
	IActor* pActor = GetActorByChannelId(m_pGameFramework->GetGameChannelId(pNetChannel));
	if(pActor)
		Vote(pActor, true);
	return true;
}

IMPLEMENT_RMI(CGameRules, SvVoteNo)
{
	IActor* pActor = GetActorByChannelId(m_pGameFramework->GetGameChannelId(pNetChannel));
	if(pActor)
		Vote(pActor, false);
	return true;
}

IMPLEMENT_RMI(CGameRules, SvStartVoting)
{
  IActor* pActor = GetActorByChannelId(m_pGameFramework->GetGameChannelId(pNetChannel));
  if(pActor)
    StartVoting(pActor,params.vote_type,params.entityId,params.param);
  return true;
}

IMPLEMENT_RMI(CGameRules, ClVotingStatus)
{
	SAFE_HUD_FUNC(SetVotingState(params.state,params.timeout,params.entityId,params.description));
  return true;
}


IMPLEMENT_RMI(CGameRules, ClEnteredGame)
{
	if(!gEnv->bServer && m_pGameFramework->GetClientActor())
	{
		UpdateCoopPlayersIds();
		IActor* pActor = GetActorByChannelId(m_pGameFramework->GetClientActor()->GetChannelId());
		if(pActor)
		{
			int status[2];
			status[0] = GetTeam(pActor->GetEntityId());
			status[1] = pActor->GetSpectatorMode();
			m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Connected, 0, 0, (void*)status));
		}
	}
	CRY_TODO(21, 10, 2009, "Move this call into above listener");
	if (GetPlayerSetupModule())
	{
		GetPlayerSetupModule()->SetupPlayer(m_pGameFramework->GetClientActorId());
	}
	
	EnteredGame();

	return true;
}

//------------------------------------------------------------------------
void CGameRules::OnHostMigrationGotLocalPlayer(CPlayer *pPlayer)
{
	if (m_pHostMigrationParams)
	{
		pPlayer->SetMigrating(true);

		// Request various bits
		GetGameObject()->InvokeRMI(SvHostMigrationRequestSetup(), *m_pHostMigrationParams, eRMI_ToServer);

		pPlayer->GetEntity()->SetPos(m_pHostMigrationClientParams->m_position);
		pPlayer->SetViewRotation(m_pHostMigrationClientParams->m_viewQuat);

		SAFE_DELETE(m_pHostMigrationParams);

		m_pHostMigrationClientParams->m_doneEnteredGame = true;
		if (m_pHostMigrationClientParams->IsDone())
		{
			SAFE_DELETE(m_pHostMigrationClientParams);
		}

		FreezeInput(true);
	}
}


// This will only ever be called on the machine on which the scoring player is the client!
IMPLEMENT_RMI(CGameRules, ClAddPoints)
{
	CryLogAlways("ClAddPoints: Local player has scored %d points", params.m_changeToScore);
	CCCPOINT(GameRules_ClModifyScore);

	SGameRulesScoreInfo scoreInfo( (EGameRulesScoreType)params.m_type, (TGameRulesScoreInt)params.m_changeToScore );
	scoreInfo.AttachVictim( (EntityId)params.m_killedEntityId );

	SHUDEvent newScoreEvent(eHUDEvent_OnNewScore);
	newScoreEvent.AddData( (void*)(&scoreInfo) );
	CHUD::CallEvent(newScoreEvent);

	if(!gEnv->bServer)
	{
		ClientScoreEvent(params.m_type, params.m_changeToScore);
	}

	return true;
}

IMPLEMENT_RMI(CGameRules, SvRequestRevive)
{
	CryLogAlways("CGameRules:SvRequestRevive");
	IGameRulesSpawningModule *pSpawningModule = GetSpawningModule();
	if (pSpawningModule)
	{
		pSpawningModule->SvRequestRevive(params.entityId);
	}
	else
	{
		CRY_ASSERT_MESSAGE(0, "CGameRules::SvRequestRevive() RMI - failed to find our spawning module, this is required");
	}

#if 0
	CActor *pActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(params.entityId));

	if (pActor)
	{
		CryLogAlways("RMI from player (%s)", pActor->GetEntity()->GetName());
		// Check if the player can revive
		if (pActor->GetHealth() <= 0)
		{
			if (GetSpawningModule())
			{
				CRY_TODO(09, 09, 2009, "This should be inside the spawning module");
				float zOffset = 0.f;
				EntityId spawnId = GetSpawningModule()->GetSpawnLocation(pActor->GetEntityId(), zOffset);
				IEntity *pSpawnPoint = gEnv->pEntitySystem->GetEntity(spawnId);
				if (pSpawnPoint)
				{
					CryLog("CGameRulesMPActorAction::OnActorAction(), reviving player '%s'", pActor->GetEntity()->GetName());
					RevivePlayer(pActor, pSpawnPoint->GetWorldPos(), pSpawnPoint->GetWorldAngles());
				}
			}
		}
		else
		{
			CryLogAlways("Player is not dead");
		}
	}
#endif

	return true;
}
//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClVictoryTeam)
{
	EGameOverReason reason = (EGameOverReason)params.reason;
	if (m_victoryConditionsModule)
		m_victoryConditionsModule->ClVictoryTeam(params.winningTeamId, reason);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClVictoryPlayer)
{
	EGameOverReason reason = (EGameOverReason)params.reason;
	if (m_victoryConditionsModule)
		m_victoryConditionsModule->ClVictoryPlayer(params.playerId, reason);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClModuleRMISingleEntity)
{
	assert(m_moduleRMIListenersVec.size() >	params.m_listenerIndex);

	IGameRulesModuleRMIListener *pListener = m_moduleRMIListenersVec[params.m_listenerIndex];
	if (pListener)		// pListener could be NULL if we get a load of lag and end up receiving an RMI after we've removed the listener
	{
		pListener->OnSingleEntityRMI(params);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClModuleRMIDoubleEntity)
{
	assert(m_moduleRMIListenersVec.size() >	params.m_listenerIndex);

	IGameRulesModuleRMIListener *pListener = m_moduleRMIListenersVec[params.m_listenerIndex];
	if (pListener)		// pListener could be NULL if we get a load of lag and end up receiving an RMI after we've removed the listener
	{
		pListener->OnDoubleEntityRMI(params);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvSetEquipmentLoadout)
{
	if (m_pEquipmentLoadout && m_pEquipmentLoadout->SvCanSetClientLoadout())
	{
		m_pEquipmentLoadout->SvSetClientLoadout(m_pGameFramework->GetGameChannelId(pNetChannel), params);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClSetServerOverrideLoadout)
{
	if (m_pEquipmentLoadout)
	{
		m_pEquipmentLoadout->ClSetServerOverrideLoadout(params);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvAutoReviveOptOut)
{
	IActor *pActor = GetActorByEntityId(params.entityId);
	if (!pActor)
		return true;

	if (m_spawningModule)
	{
		m_spawningModule->SvRecordReviveOptOut(params.entityId);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvSuccessfulFlashBang)
{
	if(IGameRulesPlayerStatsModule *statsModule = GetPlayerStatsModule())
	{
		statsModule->ProcessSuccessfulExplosion(params.entityId);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, SvHostMigrationRequestSetup)
{
	uint16 channelId = m_pGameFramework->GetGameChannelId(pNetChannel);

	IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActorByChannelId(channelId);
	CRY_ASSERT(pActor);

	CryLog("SvHostMigrationRequestSetup, unpacking setup for player '%s', channel=%i", pActor->GetEntity()->GetName(), channelId);

	m_pEquipmentLoadout->SvSetClientLoadout(channelId, params.m_loadoutParams);

	if (pActor->IsDead() && (params.m_timeToAutoRevive > 0.f))
	{
		IGameRulesSpawningModule *pSpawningModule = GetSpawningModule();
		pSpawningModule->HostMigrationInsertIntoReviveQueue(pActor->GetEntityId(), params.m_timeToAutoRevive * 1000.f);
	}

	EntityId playerId = pActor->GetEntityId();
	IItemSystem *pItemSystem = g_pGame->GetIGameFramework()->GetIItemSystem();
	SHostMigrationItemInfo *pCurrentItemInfo = NULL;

	for (int itemIndex = 0; itemIndex < m_hostMigrationItemMaxCount; ++ itemIndex)
	{
		SHostMigrationItemInfo *pItemInfo = &m_pHostMigrationItemInfo[itemIndex];
		if (pItemInfo->m_inUse)
		{
			if (pItemInfo->m_ownerId == playerId)
			{
				if (pItemInfo->m_isSelected)
				{
					// Have to give the current item last so that it gets selected
					pCurrentItemInfo = pItemInfo;
				}
				else
				{
					GiveItemToMigratingPlayer(pItemSystem, pItemInfo);
				}
			}
		}
		else
		{
			break;
		}
	}
	if (pCurrentItemInfo)
	{
		GiveItemToMigratingPlayer(pItemSystem, pCurrentItemInfo);
	}

	return true;
}

//------------------------------------------------------------------------
void CGameRules::GiveItemToMigratingPlayer(IItemSystem *pItemSystem, SHostMigrationItemInfo *pItemInfo)
{
	CItem *pItem = static_cast<CItem*>(pItemSystem->GetItem(pItemInfo->m_itemId));
	if (pItem)
	{
		CryLog("[CG]   GiveItemToMigratingPlayer() Giving %s (isUsed=%i)", pItem->GetEntity()->GetName(), (pItemInfo->m_isUsed ? 1 : 0));
		if (!pItemInfo->m_isUsed)
		{
			pItem->PickUp(pItemInfo->m_ownerId, false);
		}
		else
		{
			pItem->SetOwnerId(0);
			pItem->Use(pItemInfo->m_ownerId);
		}
	}
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClHostMigrationFinished)
{
	m_hostMigrationState = eHMS_Resuming;
	m_hostMigrationTimeStateChanged = gEnv->pTimer->GetAsyncCurTime();

	SHUDEvent hideHostMigration(eHUDEvent_HideHostMigrationScreen);
	CHUD::CallEvent(hideHostMigration);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClPredictionFailed)
{
#if !defined(_RELEASE)
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(params.predictionHandle);
	INetContext *pNetContext = g_pGame->GetIGameFramework()->GetNetContext();
	assert(pEntity && !pNetContext->IsBound(params.predictionHandle));

	CryLog("ClPredictionFailed name %s id %d", pEntity ? pEntity->GetName() : "<null entity>", params.predictionHandle);
#endif

	gEnv->pEntitySystem->RemoveEntity(params.predictionHandle);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CGameRules, ClMidMigrationJoin)
{
	CryLog("CGameRules::ClMidMigrationJoin() state=%i, timeSinceChange=%f", params.m_state, params.m_timeSinceStateChanged);
	m_hostMigrationState = EHostMigrationState(params.m_state);
	m_hostMigrationTimeStateChanged = gEnv->pTimer->GetAsyncCurTime() - params.m_timeSinceStateChanged;

	gEnv->pEntitySystem->PauseTimers(true, false);
	gEnv->pTimer->PauseTimer(ITimer::ETIMER_GAME, true);

	return true;
}
