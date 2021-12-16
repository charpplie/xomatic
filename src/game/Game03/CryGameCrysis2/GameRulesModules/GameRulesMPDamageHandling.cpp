/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
	
	-------------------------------------------------------------------------
	History:
	- 07:09:2009  : Created by Ben Johnson
	- 08:09:2009  : Written by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesMPDamageHandling.h"
#include "IXml.h"
#include "GameRules.h"
#include "Actor.h"
#include "Player.h"
#include "PersistantDebug.h"
#include "IVehicleSystem.h"
#include "GameRulesModules/IGameRulesAssistScoringModule.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"
#include "BodyDamage.h"

#define ENTITY_COLLISION_LATENT_UPATE_TIMER 1.f // time (in seconds) to check through last entity collisions and remove them.
#define ENTITY_COLLISION_IGNORE_TIME_BETWEEN_COLLISIONS 0.3f // time (int seconds) to ignore subsequent collisions for between the same 2 entities.

//------------------------------------------------------------------------
CGameRulesMPDamageHandling::CGameRulesMPDamageHandling()
{
	CryLog("CGameRulesMPDamageHandling::CGameRulesMPDamageHandling()");

	m_entityLastDamageUpdateTimer = 0.f;

	m_pGameRules = 0;

	m_initialised = false;

	m_barbwireMaterialTypeId = 0;
	m_hitTypeId_melee = 0;
}

//------------------------------------------------------------------------
CGameRulesMPDamageHandling::~CGameRulesMPDamageHandling()
{
	CryLog("CGameRulesMPDamageHandling::~CGameRulesMPDamageHandling()");
}

//------------------------------------------------------------------------
void CGameRulesMPDamageHandling::Init( XmlNodeRef xml )
{
	m_pGameRules = g_pGame->GetGameRules();

	CRY_TODO(07, 09, 2009, "Move hit materials and types into xml");

	m_pGameRules->RegisterHitType("normal");	
	m_pGameRules->RegisterHitType("repair");
	m_pGameRules->RegisterHitType("bullet");
	m_pGameRules->RegisterHitType("gaussbullet");
	m_pGameRules->RegisterHitType("fire");
	m_hitTypeId_melee			= m_pGameRules->RegisterHitType("melee");
	m_pGameRules->RegisterHitType("silentMelee");
	m_pGameRules->RegisterHitType("tac");
	m_pGameRules->RegisterHitType("frag");
	m_pGameRules->RegisterHitType("fall");
	m_pGameRules->RegisterHitType("collision");
	m_pGameRules->RegisterHitType("event");
	m_pGameRules->RegisterHitType("punish");
	m_pGameRules->RegisterHitType("punishFall");
	m_pGameRules->RegisterHitType("avmine");
	m_pGameRules->RegisterHitType("moacbullet");
	m_pGameRules->RegisterHitType("scout_moac");
	m_pGameRules->RegisterHitType("aacannon");
	m_pGameRules->RegisterHitType("emp");
	m_pGameRules->RegisterHitType("pingerPing");
	m_pGameRules->RegisterHitType("vspike");
	m_pGameRules->RegisterHitType("vacid");
	m_pGameRules->RegisterHitType("kvolt");
	m_pGameRules->RegisterHitType("impulse_hit");
	m_pGameRules->RegisterHitType("heavyBullet");
	m_pGameRules->RegisterHitType("HMG");
	m_pGameRules->RegisterHitType("stamp");
	m_pGameRules->RegisterHitType("mike_burn");
	m_pGameRules->RegisterHitType("alienDropPodBounce");
	m_pGameRules->RegisterHitType("explosion");

	m_barbwireMaterialTypeId = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeIdByName("mat_metal_barbwire");
	

	m_scriptHitInfo.Create(gEnv->pScriptSystem);
}

void CGameRulesMPDamageHandling::PostInit()
{
	CryLog("CGameRulesMPDamageHandling::PostInit()");
}

void CGameRulesMPDamageHandling::Update(float frameTime)
{
	m_entityLastDamageUpdateTimer += frameTime;

	if (m_entityLastDamageUpdateTimer > ENTITY_COLLISION_LATENT_UPATE_TIMER)
	{
		m_entityLastDamageUpdateTimer = 0.f;
		if (m_entityLastDamage.size())
		{
			float curTime = gEnv->pTimer->GetCurrTime();

			TEntityLastDamageMap::iterator current = m_entityLastDamage.begin();
			while (current != m_entityLastDamage.end())
			{
				if ((current->second.time + ENTITY_COLLISION_IGNORE_TIME_BETWEEN_COLLISIONS) < curTime)
				{
					m_entityLastDamage.erase(current++);
				}
				else
				{
					++current;
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesMPDamageHandling::SvOnHit( const HitInfo &hitInfo )
{
	SDamageHandling damageHandling(&hitInfo, 1.0f);

	CActor *pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.targetId));
	//--- Fix to allow the damage handling to work for these entity classes in the same way as for Players
	static IEntityClass* sDamEntClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DamageTestEnt");
	static IEntityClass* sDummyPlayer = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DummyPlayer");
	bool isPlayer = pActor && (pActor->IsPlayer() || (pActor->GetEntity()->GetClass() == sDamEntClass) || (pActor->GetEntity()->GetClass() == sDummyPlayer));
	CPlayer* pPlayer = isPlayer ? static_cast<CPlayer*>(pActor) : NULL;

	if(pPlayer && hitInfo.partId >= 0 && hitInfo.type != m_hitTypeId_melee)
	{
		damageHandling.damageMultiplier = pPlayer->GetBodyDamage()->GetDamageMultiplier(*pPlayer, hitInfo);
	}

	if(pPlayer)
	{
		pPlayer->SendPerkEvent(EPE_DamageHandlingTarget, &damageHandling);
	}

	CActor *pShooter = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.shooterId));
	if(pShooter && pShooter->IsPlayer())
	{
		CPlayer* pPlayerShooter = static_cast<CPlayer*>(pShooter);
		pPlayerShooter->SendPerkEvent(EPE_DamageHandlingShooter, &damageHandling);
	}

	float damage = hitInfo.damage * damageHandling.damageMultiplier;

	// Check for friendly fire
	CGameRules *pGameRules = g_pGame->GetGameRules();
	if ((hitInfo.targetId != hitInfo.shooterId) && (pGameRules->GetTeamCount() > 1))
	{
		int shooterTeamId = pGameRules->GetTeam(hitInfo.shooterId);
		int targetTeamId = pGameRules->GetTeam(hitInfo.targetId);

		if (shooterTeamId && (shooterTeamId == targetTeamId))
		{
			damage *= g_pGameCVars->g_friendlyfireratio;
		}
	}

	IEntity *pTarget = gEnv->pEntitySystem->GetEntity(hitInfo.targetId);
	if (pTarget)
	{
		IScriptTable *pTargetScript = pTarget->GetScriptTable();
		if (pTargetScript)
		{
			HitInfo hitInfoWithDamage = hitInfo;
			hitInfoWithDamage.damage = damage;

			m_pGameRules->CreateScriptHitInfo(m_scriptHitInfo, hitInfoWithDamage);

			bool isDead = false;

			if (pActor)
			{
				if (pActor->GetHealth() <= 0)
				{
					isDead = true;

					// Target is already dead
					HSCRIPTFUNCTION pfnOnDeadHit = 0;
					if (pActor->GetActorStats()->isRagDoll && pTargetScript->GetValue("OnDeadHit", pfnOnDeadHit))
					{
						Script::CallMethod(pTargetScript, pfnOnDeadHit, m_scriptHitInfo);
						gEnv->pScriptSystem->ReleaseFunc(pfnOnDeadHit);
					}
				}
			}

			if (!isDead)
			{
				IGameRulesAssistScoringModule *assistScoringModule = m_pGameRules->GetAssistScoringModule();
				if (pActor && assistScoringModule)
				{
					assistScoringModule->SvOnPlayerHit(hitInfo);	// done here to ensure the killing hit is seen before calling SvOnPlayerKilled()
				}

				SmartScriptTable targetServerScript;
				if (pTargetScript->GetValue("Server", targetServerScript))
				{
					HSCRIPTFUNCTION pfnOnHit = 0;
					if (targetServerScript->GetValue("OnHit", pfnOnHit))
					{
						bool died = false;
						if (Script::CallReturn(gEnv->pScriptSystem, pfnOnHit, pTargetScript, m_scriptHitInfo, died))
						{
							if (died)
							{
								if (pActor)
								{
									ProcessDeath(hitInfoWithDamage);
								}
								else
								{
									m_pGameRules->OnEntityKilled(hitInfo);
								}
								if (g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(hitInfo.targetId))
								{
									ProcessVehicleDeath(hitInfoWithDamage);
								}
							}
						}
						gEnv->pScriptSystem->ReleaseFunc(pfnOnHit);
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRulesMPDamageHandling::ProcessDeath( const HitInfo &hitInfo )
{
	IActor *pTarget = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.targetId);
	if (pTarget)
	{
		m_pGameRules->KillPlayer(pTarget, true, true, hitInfo.shooterId, hitInfo.weaponId, hitInfo.damage, hitInfo.partId, hitInfo.type, hitInfo.dir, hitInfo.projectileId);
	}
}

//------------------------------------------------------------------------
void CGameRulesMPDamageHandling::ProcessVehicleDeath( const HitInfo &hitInfo )
{
	CRY_TODO(08, 09, 2009, "Implement ProcessVehicleDeath");
}

//------------------------------------------------------------------------
void CGameRulesMPDamageHandling::SvOnExplosion(const ExplosionInfo &explosionInfo, const CGameRules::TExplosionAffectedEntities& affectedEntities)
{
	// SinglePlayer.lua 1721

	// calculate damage for each entity
	CGameRules::TExplosionAffectedEntities::const_iterator it = affectedEntities.begin();
	for (; it != affectedEntities.end(); ++it)
	{
		bool success = false;
		IEntity* entity = it->first;
		float obstruction = 1.0f - it->second;

		bool incone=true;
		if (explosionInfo.angle > 0 && explosionInfo.angle < 2*3.1415f)
		{
			Vec3 explosionEntityPos = entity->GetWorldPos();
			Vec3 entitypos = explosionEntityPos;
			float ha = explosionInfo.angle * 0.5f;
			Vec3 edir = entitypos - explosionInfo.pos;
			edir.normalize();
			float dot = 1;

			if (edir != Vec3(ZERO))
			{
				dot = edir.dot(explosionInfo.dir);
			}

			float angle = abs(cry_acosf(dot));
			if (angle > ha)
			{
				incone = false;
			}
		}

		if (incone && gEnv->bServer)
		{
			float damage = explosionInfo.damage;

			damage = cry_floorf(0.5f + CalcExplosionDamage(entity, explosionInfo, obstruction));		

			bool dead = false;

			HitInfo explHit;

			Vec3 dir = entity->GetWorldPos() - explosionInfo.pos;

			explHit.pos = explosionInfo.pos;
			explHit.radius = explosionInfo.radius;
			explHit.partId = -1;

			dir.Normalize();

			explHit.targetId = entity->GetId();
			explHit.weaponId = explosionInfo.weaponId;
			explHit.shooterId = explosionInfo.shooterId;
			explHit.projectileId = explosionInfo.projectileId;
			explHit.material = 0;
			explHit.damage = damage;
			explHit.type = explosionInfo.type;

			explHit.dir = dir;
			explHit.normal = -dir; //set normal to reverse of  direction to avoid backface cull of damage

			if (explHit.damage > 0.0f || explosionInfo.damage == 0.f)
			{
				if(!success) 
				{
					IActorSystem* pActorSystem = g_pGame->GetIGameFramework()->GetIActorSystem();
					CActor* pActor = static_cast<CActor*>(pActorSystem->GetActor(entity->GetId()));

					if(pActor)
					{
						if(!pActor->IsFriendlyEntity(explHit.shooterId))
						{
							success = true;
						}
					}
					else
					{
						IVehicle* pVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(entity->GetId());

						if(pVehicle)
						{
							IActor* pIActor = pVehicle->GetDriver();
							int seatNum = 0;
							int numSeats = pVehicle->GetSeatCount();

							while(!pIActor && seatNum < numSeats)
							{
								IVehicleSeat* pSeat = pVehicle->GetSeatById(seatNum++);

								EntityId passengerId = pSeat ? pSeat->GetPassenger(true) : 0;
								pIActor = pActorSystem->GetActor(passengerId);
							}

							if(pIActor)
							{
								CActor* pActor = static_cast<CActor*>(pIActor);
								if(!pActor->IsFriendlyEntity(explHit.shooterId))
								{
									success = true;
								}
							}
						}
					}
				}
				
				m_pGameRules->ServerHit(explHit);
			}
		}
		
		if (success)
		{
			IGameRulesPlayerStatsModule*  pPlayStatsMod = m_pGameRules->GetPlayerStatsModule();
		
			if(pPlayStatsMod)
			{
				pPlayStatsMod->ProcessSuccessfulExplosion(explosionInfo.shooterId);
			}
		}
	}
}

//------------------------------------------------------------------------
float CGameRulesMPDamageHandling::CalcExplosionDamage(IEntity* entity, const ExplosionInfo &explosionInfo, float obstruction)
{
	// SinglePlayer.lua 228

	// impact explosions directly damage the impact target
	if (explosionInfo.impact && explosionInfo.impact_targetId && explosionInfo.impact_targetId==entity->GetId())
	{
		return explosionInfo.damage;
	}

	float effect = 1.0f;
	float distance;
	//	if (!entity.vehicle)
	{
		distance = (entity->GetWorldPos() - explosionInfo.pos).len();
		if (distance <= explosionInfo.minRadius || explosionInfo.minRadius==explosionInfo.radius)
		{
			effect = 1.0f;
		}
		else
		{
			distance = max(0.0f, min(distance, explosionInfo.radius));
		}
		float r = explosionInfo.radius - explosionInfo.minRadius;
		float d = distance - explosionInfo.minRadius;
		effect = (r-d)/r;
		effect = max(min(1.0f, effect*effect), 0.0f);
	}

	effect = effect * (1.0f - obstruction);

	return explosionInfo.damage * effect;
}

//------------------------------------------------------------------------
void CGameRulesMPDamageHandling::SvOnCollision(const IEntity *pEntity, const CGameRules::SCollisionHitInfo& colHitInfo)
{
	CRY_TODO(14, 09, 2009, "Collision code copied straight from lua. Could do with a little refactoring and optimizing.");
	// TODO: Things missing that were in lua
	// - advance door code, special case checks are missing.
	// - ai, a few ai checks and functions are not implemented here.
	// - still lua ties when calling functions on the entities. These might looking at to see if we can remove some.
	// - collision_type string isn't implemented, was used by player e.g. sliding, sprinting.
	// - last entity hit times are in a map - not ideal.

	IEntity *pTarget = gEnv->pEntitySystem->GetEntity(colHitInfo.targetId);

	IScriptTable *pEntityScript = pEntity ? pEntity->GetScriptTable() : NULL;
	IScriptTable *pTargetScript = pTarget ? pTarget->GetScriptTable() : NULL;
	
	if (pEntityScript)
	{
		SmartScriptTable entityServerScript;
		if (pEntityScript->GetValue("Server", entityServerScript))
		{
			HSCRIPTFUNCTION pfnOnHit = 0;
			if (!entityServerScript->GetValue("OnHit", pfnOnHit))
			{
				gEnv->pScriptSystem->ReleaseFunc(pfnOnHit);
				return;
			}
			gEnv->pScriptSystem->ReleaseFunc(pfnOnHit);
		}
		else
		{
			return;
		}

		HSCRIPTFUNCTION pfnIsDead = 0;
		if (pEntityScript->GetValue("IsDead", pfnIsDead))
		{
			bool result = false;
			if (Script::CallReturn(gEnv->pScriptSystem, pfnIsDead, pEntityScript, result))
			{
				if (result)
				{
					gEnv->pScriptSystem->ReleaseFunc(pfnIsDead);
					return;
				}
			}
		}
		gEnv->pScriptSystem->ReleaseFunc(pfnIsDead);
	}
	
	EntityId entityId = pEntity ? pEntity->GetId() : 0;

	const IEntity *	colInfo_collider = pTarget;
	EntityId	colInfo_colliderId = colHitInfo.targetId;
	float		colInfo_colliderMass = colHitInfo.target_mass;
	float		colInfo_contactVelocity = 0.f;
	float		colInfo_contactVelocitySq = 0.f;
	float		colInfo_contactMass = 0.f;
	float		colInfo_minVelocity = 0.f;
	float		colInfo_minVelocitySq = 0.f;
	float		colInfo_damage = 0.f;

	float curTime = gEnv->pTimer->GetCurrTime();
	TEntityLastDamageMap::iterator lastEntity = m_entityLastDamage.find(entityId);
	if (colInfo_collider && (lastEntity!=m_entityLastDamage.end()) && (lastEntity->second.entityId == colInfo_colliderId))
	{
		if ((lastEntity->second.time + ENTITY_COLLISION_IGNORE_TIME_BETWEEN_COLLISIONS) > curTime)
		{
			return;
		}
	}

	if (colInfo_collider || (colInfo_colliderMass > 0.f))	// -- collision with another entity
	{
		Vec3 result = colHitInfo.velocity.sub(colHitInfo.target_velocity);
		colInfo_contactVelocitySq = result.len2();
		colInfo_contactMass = colInfo_colliderMass;
		colInfo_minVelocity = GetCollisionMinVelocity(pEntity, colHitInfo);
	}
	else	// -- collision with world
	{
		colInfo_contactVelocitySq = colHitInfo.velocity.len2();

		float mass = 0.f;
		IPhysicalEntity *phys = pEntity->GetPhysics();
		if(phys)
		{
			pe_status_dynamics	dyn;
			phys->GetStatus(&dyn);
			mass = dyn.mass;
		}

		colInfo_contactMass = mass;
		colInfo_minVelocity = 7.5f;
	}

	// -- make sure we're colliding with something worthy
	if (colInfo_contactMass < 0.01f)
	{
			return;
	}

	// -- marcok: avoid fp exceptions, not nice but I don't want to mess up any damage calculations below at this stage
	colInfo_contactVelocitySq = MAX(0.01f, colInfo_contactVelocitySq);

	colInfo_minVelocitySq = sqr(colInfo_minVelocity);

	// -- This should handle falling trees/rocks (vehicles are more heavy usually)
	bool bigObject = false;
	if ((colInfo_contactMass > 200.f) && (colInfo_contactMass < 10000.f) && (colInfo_contactVelocitySq > 2.25f))
	{
		if (colHitInfo.target_velocity.len2() > (colInfo_contactVelocitySq * 0.3f))
		{
			bigObject = true;
			// -- vehicles and doors shouldn't be 'bigObject'-ified
			if (colInfo_collider)
			{
				IVehicle *vehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(colInfo_colliderId);
				if (vehicle)// or advancedDoor)
				{
					if (vehicle->GetStatus().beingFlipped)
					{
						return;	// Vehicles that are being flipped by the player don't cause damage
					}
					bigObject = false;
				}
			}
		}
	}

	// -- Check player collision against barbwire material
	bool collideBarbWire = false;
	CActor	*pEntityActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entityId));

	if (pEntityActor && (colHitInfo.materialId == m_barbwireMaterialTypeId))
	{
		collideBarbWire = true;
	}

	if (colInfo_contactVelocitySq >= colInfo_minVelocitySq || bigObject || collideBarbWire)
	{
// 		-- tell AIs about collision
// 		if(AI and entity and entity.AI and not entity.AI.Colliding) then 
// 			g_SignalData.id = hit.target_id;
// 			g_SignalData.fValue = collisionInfo.contactVelocitySq;
// 			AI.Signal(SIGNALFILTER_SENDER,1,"OnCollision",entity.id,g_SignalData);
// 			entity.AI.Colliding = true;
//			entity:SetTimer(COLLISION_TIMER,4000);
// 		end

		colInfo_contactVelocity = sqrt_tpl(colInfo_contactVelocitySq) - colInfo_minVelocity;
		if (colInfo_contactVelocity < 0.f)
		{
			colInfo_contactVelocitySq = colInfo_minVelocitySq;
			colInfo_contactVelocity = 0.f;
		}

		// -- damage
		IVehicle *pEntityVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(entityId);
		if (pEntityVehicle)
		{
			if (gEnv->bMultiplayer)
			{
 				colInfo_damage = 0.0002f * GetCollisionEnergy(pEntity, colHitInfo);	// keeping the original values for MP.
			}
			else
			{
 				colInfo_damage = 0.0005f * GetCollisionEnergy(pEntity, colHitInfo); // vehicles get less damage SINGLEPLAYER ONLY.
			}
		}
		else
		{
			colInfo_damage = 0.0025f * GetCollisionEnergy(pEntity, colHitInfo);
		}

		// -- apply damage multipliers 
		colInfo_damage *= GetCollisionDamageMult(pEntity, colInfo_collider, colHitInfo);

		EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();

		// -- Magic for player barb wire collision
		if (collideBarbWire && entityId == localActorId)	// SinglePlayer Only!?
		{
			colInfo_damage *= ((colInfo_contactMass * 0.15f) * (30.f / colInfo_contactVelocitySq));
		}

		// -- Magic for big objects (scales damage against AI)
		if (bigObject)
		{
			if (colInfo_damage > 0.5f)
			{
				colInfo_damage *= (colInfo_contactMass / colInfo_contactVelocitySq);
				if (entityId != localActorId)	// SinglePlayer Only!?
					colInfo_damage *= 3.f;
			}
			else
			{
				return;
			}
		}
		// -- subtract collision damage threshold, if available
		if (pEntityScript)
		{
			HSCRIPTFUNCTION pfnGetCollisionDamageThreshold = 0;
			if (pEntityScript->GetValue("GetCollisionDamageThreshold", pfnGetCollisionDamageThreshold))
			{
				float result = 0.f;
				if (Script::CallReturn(gEnv->pScriptSystem, pfnGetCollisionDamageThreshold, pEntityScript, result))
				{
					colInfo_damage = MAX(0, colInfo_damage - result);
				}
				gEnv->pScriptSystem->ReleaseFunc(pfnGetCollisionDamageThreshold);
			}
		}

		// -- Special damage handling for actors

		if (pEntityActor)
		{
			bool isPlayer = pEntityActor->IsPlayer();

			// --limit damage from player colliding 
			if (isPlayer)
			{
				colInfo_damage = AdjustPlayerCollisionDamage(pEntity, colInfo_collider, colHitInfo, colInfo_damage);
			}

			if (colInfo_collider)
			{
				IActor	*pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(colInfo_colliderId);
				// -- Collision from actor (non player) against world / object (non actor)	
				
				if (isPlayer && pActor)
				{
					colInfo_damage = ProcessPlayerToActorCollision(pEntity, colInfo_collider, colHitInfo, colInfo_damage);
				}
			}
		}

		if (colInfo_damage >= 0.5f)
		{
			if (!colInfo_collider && pEntity)
			{
				colInfo_collider = pEntity;
				colInfo_colliderId = entityId;
			}

// 			entity.lastCollDamagerId = collisionInfo.collider.id;
// 			entity.lastCollDamageTime = curtime;
// 			entity.lastCollDamage = collisionInfo.damage;
			TEntityLastDamageMap::iterator lastHitIt = m_entityLastDamage.find(entityId);
			if (lastHitIt!=m_entityLastDamage.end())
			{
				lastHitIt->second.entityId = colInfo_colliderId;
				lastHitIt->second.time = curTime;
			}
			else
			{
				m_entityLastDamage.insert(TEntityLastDamageMap::value_type(entityId, SEntityLastCollision(colInfo_colliderId, curTime)));
			}

			HitInfo hit;
			hit.pos = colHitInfo.pos;
			hit.dir = colHitInfo.dir;
			hit.radius = 0.f;
			hit.partId = -1;
			//hit.target
			hit.targetId = entityId;
			//hit.weapon
			hit.weaponId = colInfo_colliderId;
			//hit.shooter
			hit.shooterId = colInfo_colliderId;
			hit.material = 0;
			hit.damage = colInfo_damage;
			//hit.typeId
			hit.type = m_pGameRules->GetHitTypeId("collision");
			//hit.collision_type
			//hit.impulse

			IScriptTable *pColliderScript = colInfo_collider ? colInfo_collider->GetScriptTable() : NULL;
			if (pColliderScript)
			{
				IVehicle *vehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(colInfo_colliderId);
				if (vehicle)
				{
					if (IActor *pDriver = vehicle->GetDriver())
					{
						hit.shooterId = pDriver->GetEntityId();
					}
				}
			}

			bool deadly = false;

			if (pEntityScript)
			{
				SmartScriptTable targetServerScript;
				if (pEntityScript->GetValue("Server", targetServerScript))
				{
					HSCRIPTFUNCTION pfnOnHit = 0;
					if (targetServerScript->GetValue("OnHit", pfnOnHit))
					{
						bool died = false;
						m_pGameRules->CreateScriptHitInfo(m_scriptHitInfo, hit);
						if (Script::CallReturn(gEnv->pScriptSystem, pfnOnHit, pEntityScript, m_scriptHitInfo, died))
						{
							if (died)
							{
								if (g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hit.targetId))
								{
									ProcessDeath(hit);
								}
								if (g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(hit.targetId))
								{
									ProcessVehicleDeath(hit);
								}

								deadly = true;
								m_pGameRules->OnEntityKilled(hit);
							}
						}
						gEnv->pScriptSystem->ReleaseFunc(pfnOnHit);
					}
				}
			}

			if (g_pGameCVars->g_debugHits>0)
			{
				LogHit(hit, g_pGameCVars->g_debugHits>1, deadly);
			}
		}
	}
}

//------------------------------------------------------------------------
float CGameRulesMPDamageHandling::GetCollisionEnergy(const IEntity *pEntity, const CGameRules::SCollisionHitInfo& colHitInfo)
{
	float m1 = colHitInfo.target_mass;
	float m0 = 0.f;

	IPhysicalEntity *phys = pEntity->GetPhysics();
	if(phys)
	{
		pe_status_dynamics	dyn;
		phys->GetStatus(&dyn);
		m0 = dyn.mass;
	}

	IEntity *pTarget = gEnv->pEntitySystem->GetEntity(colHitInfo.targetId);
	bool bCollider = (pTarget || m1 > 0.001f);

	bool debugColl = (g_pGameCVars->g_debugCollisionDamage > 0);
	if (debugColl)
	{
		CryLog("GetCollisionEnergy %s (%.1f) <-> %s (%.1f)", pEntity?pEntity->GetName():"[no entity]", m0, pTarget?pTarget->GetName():"[no entity]", m1);
	}

	float v0Sq = 0.f, v1Sq = 0.f;

	if (bCollider)	// non-static
	{
		m0 = MIN(m0, m1);

		Vec3 v0normal, v1normal, vrel;
		Vec3 tempNormal = colHitInfo.normal;
		
		float v0dotN = colHitInfo.velocity.dot(colHitInfo.normal);
		v0normal = tempNormal.scale(v0dotN);

		float v1dotN = colHitInfo.target_velocity.dot(colHitInfo.normal);
		v1normal = tempNormal.scale(v1dotN);

		vrel = v0normal.sub(v1normal);
		float vrelSq = vrel.len2();

		v0Sq = MIN( sqr(v0dotN), vrelSq );
		v1Sq = MIN( sqr(v1dotN), vrelSq );

		if (debugColl)
		{
			IPersistantDebug* pPD = g_pGame->GetIGameFramework()->GetIPersistantDebug();

			pPD->Begin("CollDamage", false);
			pPD->AddSphere(colHitInfo.pos, 0.15f, Col_Red, 5.f);
			pPD->AddDirection(colHitInfo.pos, 1.5f, tempNormal.scale(sgn(v0dotN)), Col_Green, 5.f);
			pPD->AddDirection(colHitInfo.pos, 1.5f, tempNormal.scale(sgn(v1dotN)), Col_Red, 5.f);

			if ((v0Sq > 2*2) || (v1Sq > 2*2))
			{
				CryLog("normal velocities: rel %.1f, <%s> %.1f / <%s> %.1f", sqrt(vrelSq), pEntity?pEntity->GetName():"none", v0dotN, pTarget?pTarget->GetName():"none", v1dotN); 
				CryLog("target_type: %i, target_velocity: %.2f %.2f %.2f", colHitInfo.target_type, colHitInfo.target_velocity.x, colHitInfo.target_velocity.y, colHitInfo.target_velocity.z);
			}
		}
	}
	else
	{
		v0Sq = sqr(colHitInfo.velocity.dot(colHitInfo.normal));

		if (debugColl && v0Sq>5*5)
		{
			IPersistantDebug* pPD = g_pGame->GetIGameFramework()->GetIPersistantDebug();

			pPD->Begin("CollDamage", false);
			pPD->AddDirection(colHitInfo.pos, 1.5f, colHitInfo.normal, Col_Green, 5.f);
			string debugText;
			debugText.Format("z: %f", colHitInfo.velocity.z);
			pPD->Add2DText(debugText.c_str(), 1.5f, Col_White, 5.f);
		}
	}

	float colliderEnergyScale = 1.f;
	if (pEntity && pTarget)
	{
		IScriptTable *pEntityScript = pEntity->GetScriptTable();
		IScriptTable *pTargetScript = pTarget->GetScriptTable();
		if (pEntityScript)
		{
			HSCRIPTFUNCTION pfnGetColEnergyScale = 0;
			if (pEntityScript->GetValue("GetColliderEnergyScale", pfnGetColEnergyScale))
			{
				Script::CallReturn(gEnv->pScriptSystem, pfnGetColEnergyScale, pEntityScript, pTargetScript, colliderEnergyScale);
				gEnv->pScriptSystem->ReleaseFunc(pfnGetColEnergyScale);

				if (debugColl)
				{
					CryLog("colliderEnergyScale: %.1f", colliderEnergyScale);
				}
			}
		}
	}

	float energy0 = 0.5f * m0 * v0Sq;
	float energy1 = 0.5f * m1 * v1Sq * colliderEnergyScale;

	return energy0 + energy1;
}

//------------------------------------------------------------------------
float CGameRulesMPDamageHandling::GetCollisionMinVelocity(const IEntity *pEntity, const CGameRules::SCollisionHitInfo& colHitInfo)
{
	float minVel = 10.f;

	CActor	*pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId()));
	if ((pActor && !pActor->IsPlayer())/* || advancedDoor*/)
	{
		minVel = 1.f; // --Door or character hit	
	}

	IVehicle *vehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(colHitInfo.targetId);
	if(pActor && vehicle)
	{
		minVel = 6.f; // -- otherwise we don't get damage at slower speeds
	}

	if (colHitInfo.target_velocity.len2() == 0.f) // -- if collision target it not moving
	{
		minVel = minVel * 2.f;
	}

	return minVel;
}

//------------------------------------------------------------------------
float CGameRulesMPDamageHandling::GetCollisionDamageMult(const IEntity *pEntity, const IEntity *pCollider, const CGameRules::SCollisionHitInfo& colHitInfo)
{
	float result = 1.f;

	IScriptTable *pEntityScript = pEntity ? pEntity->GetScriptTable() : NULL;
	IScriptTable *pColliderScript = pCollider ? pCollider->GetScriptTable() : NULL;

	if (pColliderScript)
	{
		HSCRIPTFUNCTION pfnGetForeignColMult = 0;
		if (pColliderScript->GetValue("GetForeignCollisionMult", pfnGetForeignColMult))
		{
			float scriptResult = 1.f;
			SmartScriptTable collisionTable;
			collisionTable = gEnv->pScriptSystem->CreateTable();
			m_pGameRules->PrepCollisionForScript(colHitInfo, collisionTable);

			Script::CallReturn(gEnv->pScriptSystem, pfnGetForeignColMult, pColliderScript, pEntityScript, collisionTable, scriptResult);
			gEnv->pScriptSystem->ReleaseFunc(pfnGetForeignColMult);

			if (g_pGameCVars->g_debugCollisionDamage && scriptResult!=1.f)
			{
				CryLog("<%s>: collider <%s> has ForeignCollisionMult %.2f", pEntity?pEntity->GetName():"NULL", pCollider->GetName(), scriptResult);
			}

			result *= scriptResult;
		}
	}

	if (pEntityScript)
	{
		HSCRIPTFUNCTION pfnGetSelfColMult = 0;
		if (pEntityScript->GetValue("GetSelfCollisionMult", pfnGetSelfColMult))
		{
			float scriptResult = 1.f;
			SmartScriptTable collisionTable;
			collisionTable = gEnv->pScriptSystem->CreateTable();
			m_pGameRules->PrepCollisionForScript(colHitInfo, collisionTable);

			Script::CallReturn(gEnv->pScriptSystem, pfnGetSelfColMult, pEntityScript, pColliderScript, collisionTable, scriptResult);
			gEnv->pScriptSystem->ReleaseFunc(pfnGetSelfColMult);

			if (g_pGameCVars->g_debugCollisionDamage && scriptResult!=1.f)
			{
				CryLog("<%s>: returned SelfCollisionMult %.2f", pEntity?pEntity->GetName():"NULL", scriptResult);
			}

			result *= scriptResult;
		}
	}

	return result;
}

//------------------------------------------------------------------------
float CGameRulesMPDamageHandling::AdjustPlayerCollisionDamage(const IEntity *pEntity, const IEntity *pCollider, const CGameRules::SCollisionHitInfo& colHitInfo, float colInfo_damage)
{
	float result = colInfo_damage;
	bool isVehicle = pCollider ? g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(pCollider->GetId()) != NULL : false;

	if (isVehicle)
		return colInfo_damage;

	if (colHitInfo.target_velocity.len() == 0.f)
	{
		result = colInfo_damage * 0.2f;
	}

	CActor	*pEntityActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId()));
	if (pEntityActor)
	{
		float healthThreshold = (float)g_pGameCVars->pl_health.collision_health_threshold;
		float currentHealth = (float)pEntityActor->GetHealth();

		if (currentHealth <= healthThreshold)
		{
			result = 0.f;
		}
		else if ((currentHealth - result) < healthThreshold)
		{
			result = currentHealth - healthThreshold;
		}
	}

	return result;
}

//------------------------------------------------------------------------
float CGameRulesMPDamageHandling::ProcessPlayerToActorCollision(const IEntity *pEntity, const IEntity *pCollider, const CGameRules::SCollisionHitInfo& colHitInfo, float colInfo_damage)
{
	float ragdoll_to_player = 0.f; // -- Max damage from ragdoll collision

	CActor *pColliderActor = pCollider ? static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pCollider->GetId())) : NULL;
	if (pColliderActor)
	{
		uint8 profile=pColliderActor->GetGameObject()->GetAspectProfile(eEA_Physics);
		if ((profile == eAP_Sleep) || (profile == eAP_Ragdoll))
		{
			return MIN(colInfo_damage, ragdoll_to_player);
		}
	}

	return colInfo_damage;
}

// Debug output
//------------------------------------------------------------------------
void CGameRulesMPDamageHandling::LogHit(const HitInfo &hit, bool extended, bool dead)
{
	const IEntity *pShooter = gEnv->pEntitySystem->GetEntity(hit.shooterId);
	const IEntity *pTarget = gEnv->pEntitySystem->GetEntity(hit.targetId);
	const IEntity *pWeapon = gEnv->pEntitySystem->GetEntity(hit.weaponId);

	CryLog("'%s' hit '%s' for %f with '%s'... %s", pShooter?pShooter->GetName():"null", pTarget?pTarget->GetName():"null", hit.damage, pWeapon?pWeapon->GetName():"null",dead?"*DEADLY*":"");

	if (extended)
	{
		CryLog("  shooterId..: %d", hit.shooterId);
		CryLog("  targetId...: %d", hit.targetId);
		CryLog("  weaponId...: %d", hit.weaponId);
		CryLog("  type.......: %d", hit.type);
		CryLog("  material...: %d", hit.material);
		CryLog("  damage.....: %f", hit.damage);
		CryLog("  partId.....: %d", hit.partId);
		CryLog("  pos........: %f %f %f", hit.pos.x, hit.pos.y, hit.pos.z);
		CryLog("  dir........: %f %f %f", hit.dir.x, hit.dir.y, hit.dir.z);
		CryLog("  radius.....: %.3f", hit.radius);
		CryLog("  remote.....: %d", hit.remote);
	}
}
