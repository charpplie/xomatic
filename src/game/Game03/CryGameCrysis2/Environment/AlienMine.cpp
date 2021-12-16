/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Crysis 2 Alien Mine reacting to player proximity

-------------------------------------------------------------------------
History:
- 10/08/09: Created by Jan Müller

*************************************************************************/

#include "StdAfx.h"
#include "AlienMine.h"
#include "Game.h"
#include "BattleDust.h"
#include "GameRules.h"
#include "Player.h"

static const float DISTANCE_CHECK_TIMER_SLEEPING = 1.0f;
static const float DISTANCE_CHECK_TIMER_ACTIVE = 0.1f;
static const float ACTIVATION_DISTANCE = 30.0f;
static const float ACTIVATION_DISTANCE_SQUARED = ACTIVATION_DISTANCE*ACTIVATION_DISTANCE;
static const float SPAWN_LOCATION_MIN_DISTANCE_SQ = 0.3f;

CAlienMine::CAlienMine() :
m_distanceTestTimer(0.0f),
m_health(0.0f),
m_maxDistanceToSpawnLocation(0.0f),
m_sleeping(false),
m_armed(false),
m_deactivated(false),
m_exploding(false),
m_returnToSpawnPos(false),
m_invincibleUntilArmed(false),
m_ignoreCloak(false),
m_spawnLocation(ZERO)
{

}

CAlienMine::~CAlienMine()
{
	Activate(false);
}

// IGameObjectExtension
bool CAlienMine::Init(IGameObject *pGameObject)
{
	if(!pGameObject)
		return false;

	SetGameObject(pGameObject);

	//load character
	GetEntity()->LoadCharacter(0,"Objects/weapons/alien/drone_mine/drone_mine.chr");

	return true;
}

void CAlienMine::PostInit(IGameObject *pGameObject)
{
	Activate(true);
}

void CAlienMine::ProcessEvent(SEntityEvent &event)
{
	if(event.event == ENTITY_EVENT_START_LEVEL || event.event == ENTITY_EVENT_RESET)
	{
		if(event.event == ENTITY_EVENT_RESET)
			Activate(true);

		//save spawn location
		m_spawnLocation = GetEntity()->GetWorldPos();

		Physicalize();

		UpdateAnimations();

		//read health from script
		GetScriptValue("fHealth", m_health);

		//read invincible option from script
		GetScriptValue("bInvincibleUntilArmed", m_invincibleUntilArmed);

		//read max distance to travel from script
		GetScriptValue("fMaxDistanceToSpawnLocation", m_maxDistanceToSpawnLocation);

		//read whether to ignore the cloak and attack cloaked players
		GetScriptValue("bIgnoreCloak", m_ignoreCloak);
	}
	else if(event.event == ENTITY_EVENT_PRE_SERIALIZE)
		Activate(false); //remove old listeners etc. before changing the game state
}

void CAlienMine::Activate(bool active)
{
	if(active)
	{
		GetGameObject()->EnablePhysicsEvent(true, eEPE_OnCollisionLogged);
		GetGameObject()->EnablePostUpdates(this);
		GetEntity()->Hide(false);
		GetEntity()->Activate(true);
		m_deactivated = false;
		m_armed = false;
		m_exploding = false;

		if(g_pGame->GetGameRules())
			g_pGame->GetGameRules()->AddHitListener(this);
	}
	else
	{
		GetGameObject()->EnablePhysicsEvent(false, eEPE_OnCollisionLogged);
		GetGameObject()->DisablePostUpdates(this);
		GetEntity()->Hide(true);
		GetEntity()->Activate(false);
		m_deactivated = true;

		if(g_pGame->GetGameRules())
			g_pGame->GetGameRules()->RemoveHitListener(this);
	}
}

void CAlienMine::PostUpdate(float frameTime )
{
	IActor *pTarget = g_pGame->GetIGameFramework()->GetClientActor();
	CRY_ASSERT(pTarget);

	if(!pTarget || m_deactivated || g_pGame->GetIGameFramework()->IsEditing())
		return;

	if(m_exploding)
	{
		Activate(false);
		return;
	}

	Vec3 targetPos = pTarget->GetEntity()->GetWorldPos();
	targetPos.z += 1.8f; //~head pos

	//remember old state to trigger animations
	bool wasArmed = m_armed;

	m_distanceTestTimer -= frameTime;
	if(m_distanceTestTimer <= 0)
	{
		const Vec3 &minePos = GetEntity()->GetWorldPos();
		bool inProximity = CheckDistanceToTarget(targetPos, minePos);

		if(inProximity)
		{
			m_distanceTestTimer = DISTANCE_CHECK_TIMER_ACTIVE;

			m_armed = HasLineOfSight(targetPos, minePos - targetPos, pTarget);

			if(m_armed)
				m_returnToSpawnPos = false;
			else
				GetScriptValue("bReturnToSpawnLocation", m_returnToSpawnPos);
		}
		else
		{
			m_distanceTestTimer = DISTANCE_CHECK_TIMER_SLEEPING;

			if(m_armed)
			{
				GetScriptValue("bReturnToSpawnLocation", m_returnToSpawnPos);
				m_armed = false;
			}
			else
			{
				float spawnDistSq = (GetEntity()->GetWorldPos() - m_spawnLocation).GetLengthSquared();
				if(spawnDistSq > SPAWN_LOCATION_MIN_DISTANCE_SQ)
					GetScriptValue("bReturnToSpawnLocation", m_returnToSpawnPos);
			}
		}
	}

	//change of state
	if(m_armed != wasArmed)
	{
		UpdateAnimations();
	}

	//armed to detonate
	if(m_armed)
	{
		SeekAndDestroy(targetPos, GetEntity()->GetWorldPos());
	}
	if(m_returnToSpawnPos)
	{
		ReturnToSpawnPos();
	}
}

void CAlienMine::FullSerialize( TSerialize ser )
{
	ser.Value("activationStatus", m_deactivated);
	Activate(!m_deactivated);

	ser.Value("m_health", m_health);
	ser.Value("m_exploding", m_exploding);
	ser.Value("m_returnToSpawnPos", m_returnToSpawnPos);
}
//~IGameObjectExtension

//IHitListener
void CAlienMine::OnHit(const HitInfo& hit)
{
	 if(hit.targetId == GetEntityId() && hit.damage >= 1.0f)
	 {
		if(!m_invincibleUntilArmed || m_armed)
			m_health -= hit.damage;

		if(m_health <= 0.0f)
			Explode(GetEntity()->GetWorldPos(), Vec3Constants<float>::fVec3_OneY);
	 }
}

void CAlienMine::OnExplosion(const ExplosionInfo &explInfo)
{

}
//~IHitListener

bool CAlienMine::CheckDistanceToTarget(const Vec3 &targetPos, const Vec3 &minePos)
{
	Vec3 targetVec = targetPos - minePos;
	float distanceSq = targetVec.GetLengthSquared();

	if(distanceSq < ACTIVATION_DISTANCE_SQUARED)
	{
		//check distance against activation radius
		float activationRadius = 0.0f;
		if(GetScriptValue("fAttackRadius", activationRadius))
		{
			if(activationRadius > sqrtf(distanceSq))
				return true;
		}

		return false;
	}

	return false;
}

bool CAlienMine::HasLineOfSight(const Vec3 &targetPos, const Vec3 &dir, IActor *pTarget)
{
	if(pTarget->IsPlayer())
	{
		if(!m_ignoreCloak)
		{
			//can't see the player when cloaked
			CPlayer *pPlayer = static_cast<CPlayer*>(pTarget);
			if(pPlayer->GetNanoSuit() && pPlayer->GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Stealth)
				return false;
		}
	}

	//check whether the player is in cover
	IPhysicalEntity *pTargetPhysics = pTarget->GetEntity()->GetPhysics();

	//ray_hit hit;
	//gEnv->pPhysicalWorld->RayWorldIntersection(targetPos, dir, ent_all, rwi_stop_at_pierceable|rwi_ignore_back_faces, &hit, 1, &pTargetPhysics, 1);
	m_raycastHelper->CastRay(targetPos, dir, ent_all, rwi_stop_at_pierceable|rwi_ignore_back_faces, &pTargetPhysics, 1);
	const ray_hit *pHit = m_raycastHelper->GetRayHit();

	if(pHit->dist <= 0.0f || pHit->pCollider == GetEntity()->GetPhysics())
		return true;

	return false;
}

void CAlienMine::SeekAndDestroy(const Vec3 &targetPos, const Vec3 &minePos)
{
	Vec3 approachVector = targetPos - minePos;
	float distanceSq = approachVector.GetLengthSquared();

	float explosionDistance = 0.0f;
	GetScriptValue("fExplosionDistance", explosionDistance);

	if(distanceSq < explosionDistance*explosionDistance)
	{
		Explode(minePos, approachVector);
	}
	else	//approach
	{
		float travelDistance = (m_spawnLocation - targetPos).len();
		if(travelDistance > m_maxDistanceToSpawnLocation)
		{
			//don't travel further away
			GetScriptValue("bReturnToSpawnLocation", m_returnToSpawnPos);
			if(!m_returnToSpawnPos)
			{
				pe_action_set_velocity velocity;
				velocity.v = Vec3Constants<float>::fVec3_Zero;
				GetEntity()->GetPhysics()->Action(&velocity);
			}

			return;
		}
		else
		{
			//get speed from script
			float approachSpeed = 0.0f;
			GetScriptValue("fApproachSpeed", approachSpeed);

			//move object
			pe_action_set_velocity velocity;
			velocity.v = approachVector.GetNormalized() * approachSpeed;
			GetEntity()->GetPhysics()->Action(&velocity);
		}
	}
}

template <class T>
bool CAlienMine::GetScriptValue(const char *name, T &propValue)
{
	CRY_ASSERT(name);

	SmartScriptTable props;
	IScriptTable* pScriptTable = GetEntity()?GetEntity()->GetScriptTable():NULL;
	CRY_ASSERT(pScriptTable);

	if(pScriptTable && pScriptTable->GetValue("Properties", props))
	{
		CRY_ASSERT(props);
		props->GetValue(name, propValue);
		return true;
	}

	return false;
}

void CAlienMine::Explode(const Vec3 &pos, const Vec3 &dir)
{
	CGameRules *pGameRules = g_pGame->GetGameRules();
	CRY_ASSERT(pGameRules);

	//CreateExplosion
	if (gEnv->bServer)
	{
		EntityId id = GetEntityId();

		float damage = 0.0f;
		GetScriptValue("fDamage", damage);

		float explosionRadius = 0.0f;
		GetScriptValue("fExplosionRadius", explosionRadius);

		const char *effect = "";
		GetScriptValue("ExplosionEffect", effect);

		//explode
		ExplosionInfo explosionInfo(id, id, 0, damage, pos, dir, 0.1f, explosionRadius, 0.1f, explosionRadius, 0.0f, 1000.0f, 1.0f, pGameRules->GetHitTypeId("frag"));
		explosionInfo.SetEffect(effect, 1.0f, 2.0f);
		pGameRules->QueueExplosion(explosionInfo);

		// add battle dust as well
		CBattleDust* pBD = pGameRules->GetBattleDust();
		if(pBD)
			pBD->RecordEvent(eBDET_Explosion, GetEntity()->GetWorldPos(), GetEntity()->GetClass());
	}

	m_exploding = true;
}

void CAlienMine::ReturnToSpawnPos()
{
	Vec3 deltaVec = m_spawnLocation - GetEntity()->GetWorldPos();
	float distSq = deltaVec.GetLengthSquared();

	if(distSq < SPAWN_LOCATION_MIN_DISTANCE_SQ)
	{
		//stop moving
		pe_action_set_velocity velocity;
		velocity.v = Vec3Constants<float>::fVec3_Zero;
		GetEntity()->GetPhysics()->Action(&velocity);
		m_returnToSpawnPos = false;
		return;
	}

	float dist = deltaVec.GetLength();

	//get speed from script
	float approachSpeed = 0.0f;
	GetScriptValue("fApproachSpeed", approachSpeed);

	//move object
	pe_action_set_velocity velocity;
	velocity.v = (deltaVec/dist) * min(approachSpeed, dist);
	GetEntity()->GetPhysics()->Action(&velocity);
}

void CAlienMine::Physicalize()
{
	float mass = 0.f;
	GetScriptValue("fMass", mass);

	//physicalize
	SEntityPhysicalizeParams physicsParams;
	physicsParams.mass = mass;
	physicsParams.type = PE_ARTICULATED;
	physicsParams.nFlagsOR &= pef_log_collisions;
	physicsParams.nSlot = 0;
	GetEntity()->Physicalize(physicsParams);

	IPhysicalEntity *pPhysics = GetEntity()->GetPhysics();
	CRY_ASSERT(pPhysics);
	if(pPhysics)
	{
		//deactivate gravity 
		pe_simulation_params sp;
		sp.gravity = Vec3Constants<float>::fVec3_Zero;
		pe_params_flags pf;
		pf.flagsOR = pef_ignore_areas;
		pPhysics->SetParams(&sp);
		pPhysics->SetParams(&pf);

		//deactivate rotations from collisions
		pe_action_add_constraint constr_params;
		constr_params.flags = constraint_no_rotation | constraint_free_position;
		constr_params.pt[0] = Vec3Constants<float>::fVec3_Zero;
		constr_params.pBuddy = WORLD_ENTITY;
		pPhysics->Action(&constr_params);
	}
}

void CAlienMine::UpdateAnimations()
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
	if(pCharacter && pCharacter->GetISkeletonAnim())
	{
		CryCharAnimationParams animParams;
		animParams.m_nFlags = CA_LOOP_ANIMATION;
		animParams.m_nLayerID = 0;
		pCharacter->GetISkeletonAnim()->StartAnimation("idle_01", animParams);

		animParams.m_fTransTime = 0.5f;
		animParams.m_nLayerID = 1;
		animParams.m_nFlags = CA_TRANSITION_TIMEWARPING | CA_REPEAT_LAST_KEY;
		if(m_armed)
			pCharacter->GetISkeletonAnim()->StartAnimation("open_01", animParams);
		else
			pCharacter->GetISkeletonAnim()->StartAnimation("closed_01", animParams);
	}
}
