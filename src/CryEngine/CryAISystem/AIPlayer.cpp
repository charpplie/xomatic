/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   AIPlayer.cpp
$Id$
Description: 

-------------------------------------------------------------------------
History:
- 2 Mar 2009			 : Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/

#include "StdAfx.h"
#include "AIPlayer.h"
#include "CAISystem.h"
#include "DebugDrawContext.h"
#include "Puppet.h"

// The variables needs to be carefully tuned to possible the player action speeds.
static const float PLAYER_ACTION_SPRINT_RESET_TIME = 0.5f;
static const float PLAYER_ACTION_JUMP_RESET_TIME = 1.3f;
static const float PLAYER_ACTION_CLOAK_RESET_TIME = 1.5f;

static const float PLAYER_IGNORE_COVER_TIME = 6.0f;



CMissLocationSensor::CMissLocationSensor(const CAIActor* owner)
: m_state(Starting)
, m_owner(owner)
{
	AddDestroyableClass("DestroyableObject");
	AddDestroyableClass("BreakableObject");
	AddDestroyableClass("PressurizedObject");
}

void CMissLocationSensor::Update(float timeLimit)
{
	while(true)
	{
		switch (m_state)
		{
		case Starting:
			{
				m_updateCount = 0;
				m_state = Collecting;
			}
			break;
		case Collecting:
			{
				Collect(ent_static | ent_rigid | ent_sleeping_rigid | ent_independent);
				m_state = Filtering;
			}
			return;
		case Filtering:
			{
				if (Filter(timeLimit))
				{
					m_state = Finishing;
					break;
				}
			}
			return;
		case Finishing:
			{
				m_working.swap(m_locations);
				m_working.resize(0);
				m_state = Starting;
			}
			return;
		default:
			{
				assert(0);
				return;
			}
		}
	}

	++m_updateCount;
}




void CMissLocationSensor::Collect(int types)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	const float boxHalfSize = gAIEnv.CVars.CoolMissesBoxSize * 0.5f;
	const float boxHeight = gAIEnv.CVars.CoolMissesBoxHeight;

	const Vec3& feet = m_owner->GetPhysicsPos();
	const Vec3& dir = m_owner->GetViewDir();

	Vec3 pos = feet + Vec3(0.0f, 0.0f, -0.25f) + dir * boxHalfSize * 0.25f;
	Vec3 min(pos.x - boxHalfSize, pos.y - boxHalfSize, pos.z);
	Vec3 max(pos.x + boxHalfSize, pos.y + boxHalfSize, pos.z + boxHeight);

	types |= ent_allocate_list;
	m_entities.resize(MaxCollectedCount);

	IPhysicalEntity** entities = &m_entities.front();
	m_entities.resize(gEnv->pPhysicalWorld->GetEntitiesInBox(min, max, entities, types, MaxCollectedCount));
	assert(m_entities.size() <= MaxCollectedCount);

// /* //Diesel cut //bug phys 
	// Add refs to prevent physics from deleting the entity
	// TODO(márcio): Fix this once physics gets a ent_add_refs flag
	{
		MissEntities::iterator it = m_entities.begin();
		MissEntities::iterator end = m_entities.end();

		//for ( ; it != end; ++it) 
		//	(*it)->AddRef(); 
		//for ( ; it != end; ++it) //Diesel new
		//	(*it)->Release(); //Diesel new
	}
 //*/ //Diesel cut
}

bool CMissLocationSensor::Filter(float timeLimit)
{
/* //Diesel cut
	if (m_entities.empty())
		return true;

	CTimeValue now = gEnv->pTimer->GetAsyncTime();
	CTimeValue start = now;
	CTimeValue endTime = now + CTimeValue(timeLimit);

	do
	{
		IPhysicalEntity& entity = *m_entities.front();
		std::swap(m_entities.front(), m_entities.back());
		m_entities.pop_back();


		bool destroyable = false;	


		if (IEntity* ientity = gEnv->pEntitySystem->GetEntityFromPhysics(&entity))
		{
			for (uint c = 0; c < m_destroyableEntityClasses.size(); ++c)
			{
				if (ientity->GetClass() == m_destroyableEntityClasses[c])
				{
					destroyable = true;
					break;
				}
			}	
		}

		Matrix34 worldTM;

		// Check for idmatBreakable or geom_manually_breakable
		// If the entity was previous known to be destroyable add all it's physical parts
		{
			pe_params_part pp;
			pp.ipart = 0;
			pp.pMtx3x4 = &worldTM;

			uint32 partCount = 0;
			while (entity.GetParams(&pp)) //162
			{
				++partCount;

				if (!pp.pPhysGeom || !pp.pPhysGeom->pGeom)
					continue;

				primitives::box box;
				pp.pPhysGeom->pGeom->GetBBox(&box);

				Vec3 pos = worldTM.TransformPoint(box.center);

				if (!destroyable)
				{
					if (pp.flagsOR & geom_manually_breakable)
					{
						m_working.push_back(
							MissLocation(pos, MissLocation::ManuallyBreakable));
					}

					if (pp.idmatBreakable >= 0)
					{
						m_working.push_back(
							MissLocation(pos, MissLocation::MatBreakable));
					}

					if (pp.idSkeleton >= 0)
					{
						m_working.push_back(
							MissLocation(pos, MissLocation::Deformable));
					}
					else
					{
						m_working.push_back(
							MissLocation(pos, MissLocation::Destroyable));
					}
				}

				pp.ipart = partCount;
				MARK_UNUSED pp.partid;
			}
		}

		// Check if entity contains structural joints
		// Track which parts are connected by joints
		// Add those parts as good miss locations
		{
			uint64 jointConnectedParts = 0;
			pe_params_structural_joint sjp;
			sjp.idx = 0;

			uint32 jointCount = 0;
			while (entity.GetParams(&sjp))
			{
				++jointCount;
				if (sjp.bBreakable && !sjp.bBroken)
				{
					if ((sjp.partid[0] > -1) && (sjp.partid[0] < 64))
						jointConnectedParts |= 1ll << sjp.partid[0];

					if ((sjp.partid[1] > -1) && (sjp.partid[1] < 64))
						jointConnectedParts |= 1ll << sjp.partid[1];
				}

				sjp.idx = jointCount;
				MARK_UNUSED sjp.id;
			}

			if (jointConnectedParts)
			{
				pe_status_pos spos;
				spos.pMtx3x4 = &worldTM;

				for (uint32 p = 0; p < 64; ++p)
				{
					if (jointConnectedParts & (1ll << p))
					{
						spos.partid = p;
						MARK_UNUSED spos.ipart;

						if (!entity.GetStatus(&spos))
							continue;

						if (!spos.pGeom)
							continue;

						primitives::box box;
						spos.pGeom->GetBBox(&box);

						m_working.push_back(
							MissLocation(worldTM.TransformPoint(box.center), MissLocation::JointStructure));
					}
				}
			}
		}

		// Add rope vertices
		pe_status_rope srope;
		pe_params_rope prope;
		if (entity.GetStatus(&srope) && entity.GetParams(&prope))
		{
			if (prope.pEntTiedTo[0] && prope.pEntTiedTo[1])
			{
				uint32 pointCount = std::max(srope.nVtx, (srope.nSegments + 1)); // use the version with the most detail
				uint32 step = 1;

				if (pointCount > MaxRopeVertexCount)
					step = pointCount / MaxRopeVertexCount;

				m_vertices.resize(pointCount);

				if (srope.nVtx < srope.nSegments)
					srope.pPoints = &m_vertices.front();
				else
					srope.pVtx = &m_vertices.front();

				entity.GetStatus(&srope);

				for (uint i = 0; i < m_vertices.size(); i += step)
					m_working.push_back(MissLocation(m_vertices[i], MissLocation::Rope));
			}
		}

		now = gEnv->pTimer->GetAsyncTime();

		entity.Release();

		if (m_entities.empty())
			return true;

	} while (now < endTime);
*/ //Diesel cut
	return false;
}

bool CMissLocationSensor::GetLocation(const Vec3& shootPos, const Vec3& shootDir, float maxAngle, Vec3& pos)
{
	FUNCTION_PROFILER(gEnv->pSystem,PROFILE_AI);

	if (m_locations.empty())
		return false;

	m_goodies.resize(0);

	float maxAngleCos = cry_cosf(maxAngle);
	float angleIntervalInv = 1.0f / (1.0f - maxAngleCos);

	uint32 maxConsidered = min(static_cast<uint32>(MaxConsiderCount), m_locations.size());

	MissLocations::iterator it = m_locations.begin();
	MissLocations::iterator end = m_locations.end();

	for ( ; maxConsidered && (it != end); ++it)
	{
		MissLocation& location = *it;

		Vec3 dir(location.position - shootPos);
		dir.Normalize();

		float angleCos = dir.dot(shootDir);
		if (angleCos <= maxAngleCos)
			continue;

		float typeScore = 0.0f;
		switch(location.type)
		{
		case MissLocation::Destroyable:
			typeScore = 1.0f;
			break;
		case MissLocation::Rope:
			typeScore = 0.95f;
			break;
		case MissLocation::ManuallyBreakable:
			typeScore = 0.85f;
			break;
		case MissLocation::JointStructure:
			typeScore = 0.75f;
			break;
		case MissLocation::Deformable:
			typeScore = 0.65f;
			break;
		case MissLocation::MatBreakable:
			typeScore = 0.5f;
			break;
		case MissLocation::Unbreakable:
		default:
			break;
		}

		float angleScore = (angleCos - maxAngleCos) * angleIntervalInv;

		m_goodies.push_back(location);
		m_goodies.back().score = (angleScore * 0.4f) + (typeScore * 0.6f);
		--maxConsidered;
	}

	std::sort(m_goodies.begin(), m_goodies.end());

	// Ignore anything that would hit the player
	if (IPhysicalEntity* ownerPhysics = m_owner->GetPhysics())
	{
		pe_status_pos ppos;
		if (ownerPhysics->GetStatus(&ppos))
		{
			AABB player(ppos.BBox[0] - ppos.pos, ppos.BBox[1] + ppos.pos);
			player.Expand(Vec3(0.35f));
			
			Lineseg lineOfFire;
			lineOfFire.start = shootPos;

			uint32 locationCount = m_goodies.size();
			for (uint32 i = 0; (i < locationCount) && (i < MaxRandomPool); ++i)
			{
				lineOfFire.end = m_goodies[i].position;
				
				if (Overlap::Lineseg_AABB(lineOfFire, player))
				{
					std::swap(m_goodies[i], m_goodies.back());
					m_goodies.pop_back();
					--i;
					--locationCount;
				}
			}

			//GetAISystem()->AddDebugBox(ZERO, OBB::CreateOBBfromAABB(Quat(IDENTITY), player), 255, 255, 255, 0.33f);
		}
	}

	if (m_goodies.empty())
		return false;

	pos = m_goodies[Random(min(static_cast<uint32>(MaxRandomPool), m_goodies.size()))].position;

	return true;
}

void CMissLocationSensor::AddDestroyableClass(const char* className)
{
	if (IEntityClass* entityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(className))
		stl::push_back_unique(m_destroyableEntityClasses, entityClass);
}

void CMissLocationSensor::ResetDestroyableClasses()
{
	m_destroyableEntityClasses.clear();
}

//
//---------------------------------------------------------------------------------
CAIPlayer::CAIPlayer()
: m_FOV(0)
, m_playerStuntSprinting(-1.0f)
, m_playerStuntJumping(-1.0f)
, m_playerStuntCloaking(-1.0f)
, m_playerStuntUncloaking(-1.0f)
, m_stuntDir(0,0,0)
, m_mercyTimer(-1.0f)
, m_coverExposedTime(-1.0f)
#pragma warning(disable: 4355)
, m_missLocationSensor(this)
{
	_fastcast_CAIPlayer = true;
}

//
//---------------------------------------------------------------------------------
CAIPlayer::~CAIPlayer()
{
	if (m_exposedCoverState.rayID != 0)
		gAIEnv.pRayCaster->Cancel(m_exposedCoverState.rayID);

	ReleaseExposedCoverObjects();
}

//
//---------------------------------------------------------------------------------
void CAIPlayer::Reset(EObjectResetType type)
{
	CAIActor::Reset(type);
	m_fLastUpdateTargetTime = 0.f;

	m_deathCount = 0;
	m_lastThrownItems.clear();
	m_stuntTargets.clear();
	m_stuntDir.Set(0,0,0);
	m_mercyTimer = -1.0f;
	m_coverExposedTime = -1.0f;

	ReleaseExposedCoverObjects();
}

//
//---------------------------------------------------------------------------------
void CAIPlayer::ReleaseExposedCoverObjects()
{
	for (unsigned i = 0, ni = m_exposedCoverObjects.size(); i < ni; ++i)
		m_exposedCoverObjects[i].pPhysEnt->Release();
	m_exposedCoverObjects.clear();
}

//
//---------------------------------------------------------------------------------
void CAIPlayer::AddExposedCoverObject(IPhysicalEntity* pPhysEnt)
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	unsigned oldest = 0;
	float oldestTime = FLT_MAX; // Count down timers, find smallest value.
	for (unsigned i = 0, ni = m_exposedCoverObjects.size(); i < ni; ++i)
	{
		SExposedCoverObject& co = m_exposedCoverObjects[i];
		if (co.pPhysEnt == pPhysEnt)
		{
			co.t = PLAYER_IGNORE_COVER_TIME;
			return;
		}
		if (co.t < oldestTime)
		{
			oldest = i;
			oldestTime = co.t;
		}
	}

	// Limit the number of covers, override oldest one.
	if (m_exposedCoverObjects.size() >= 3)
	{
		// Release the previous entity
		m_exposedCoverObjects[oldest].pPhysEnt->Release();
		// Fill in new.
		pPhysEnt->AddRef();
		m_exposedCoverObjects[oldest].pPhysEnt = pPhysEnt;
		m_exposedCoverObjects[oldest].t = PLAYER_IGNORE_COVER_TIME;
	}
	else
	{
		// Add new
		pPhysEnt->AddRef();
		m_exposedCoverObjects.push_back(SExposedCoverObject(pPhysEnt, PLAYER_IGNORE_COVER_TIME));
	}
}

//
//---------------------------------------------------------------------------------
void CAIPlayer::CollectExposedCover()
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	if (m_coverExposedTime > 0.0f)
	{
		if (m_exposedCoverState.asyncState == AsyncReady)
		{
			m_exposedCoverState.asyncState = AsyncInProgress;

			// Find the object directly in front of the player
			const Vec3& pos = GetPos();
			const Vec3 dir = GetViewDir() * 3.0f;
			const int flags = rwi_colltype_any | (geom_colltype_obstruct << rwi_colltype_bit) | (VIEW_RAY_PIERCABILITY & rwi_pierceability_mask);

			m_exposedCoverState.rayID = gAIEnv.pRayCaster->Queue(
				RayCastRequest::MediumPriority,
				RayCastRequest(pos, dir, ent_static, flags),
				functor(*this, &CAIPlayer::CollectExposedCoverRayComplete));
		}
	}
}

void CAIPlayer::CollectExposedCoverRayComplete(const QueuedRayID& rayID, const RayCastResult& result)
{
	if (m_exposedCoverState.rayID == rayID)
	{
		m_exposedCoverState.rayID = 0;
		m_exposedCoverState.asyncState = AsyncReady;

		if (result && result[0].pCollider)
			AddExposedCoverObject(result[0].pCollider);
	}
}

//
//---------------------------------------------------------------------------------
void CAIPlayer::GetPhysicsEntitiesToSkip(std::vector<IPhysicalEntity*>& skips) const
{
	CAIActor::GetPhysicsEntitiesToSkip(skips);
	// Skip exposed covers
	for (unsigned i = 0, ni = m_exposedCoverObjects.size(); i < ni; ++i)
		skips.push_back(m_exposedCoverObjects[i].pPhysEnt);
}

//
//---------------------------------------------------------------------------------
void CAIPlayer::ParseParameters(const AIObjectParameters & params)
{
	CAIActor::ParseParameters( params );
	m_Parameters = params.m_sParamStruct;
}

//
//---------------------------------------------------------------------------------
IPhysicalEntity* CAIPlayer::GetPhysics(bool wantCharacterPhysics=false) const
{
	// temporary, should access a proxy instead of going directly to entity
	IEntity *pEntity = GetEntity();
	if(pEntity)
		return pEntity->GetPhysics();
	return NULL;
}

//
//---------------------------------------------------------------------------------
IAIObject::EFieldOfViewResult CAIPlayer::IsPointInFOV(const Vec3 &pos, float distanceScale) const
{
	EFieldOfViewResult eResult = eFOV_Outside;

	Vec3 vDirection = pos - GetPos();
	const float fDirectionLengthSq = vDirection.GetLengthSquared();
	vDirection.NormalizeSafe();

	// lets see if it is outside of its vision range
	if (fDirectionLengthSq > 0.1f && fDirectionLengthSq <= sqr(m_Parameters.m_PerceptionParams.sightRange * distanceScale))
	{
		const Vec3 vViewDir = GetViewDir().GetNormalizedSafe();
		const float fDot = vDirection.Dot(vViewDir);

		eResult = (fDot >= m_FOV ? eFOV_Primary : eFOV_Outside);
	}
	
	return eResult;
}

//
//---------------------------------------------------------------------------------
void CAIPlayer::UpdateAttentionTarget(CWeakRef<CAIObject> refTarget)
{
	bool bUpdateLeaderStats = false;

	bool bSameTarget = (m_refAttentionTarget==refTarget);
	if(!bUpdateLeaderStats || bSameTarget)
	{
		bUpdateLeaderStats = !bSameTarget;
		m_refAttentionTarget = refTarget;
		m_fLastUpdateTargetTime = GetAISystem()->GetFrameStartTime();
	}
	else // compare the new target with the current one
	{ 
		CCCPOINT(CAIPlayer_UpdateAttentionTarget);

		CAIObject *pAttentionTarget = m_refAttentionTarget.GetAIObject();
		CAIObject *pTarget = refTarget.GetAIObject();

		Vec3 direction = pAttentionTarget->GetPos() - GetPos();

		// lets see if it is outside of its vision range

		Vec3 myorievector = GetViewDir();
		float dist = direction.GetLength();
		
		if(dist>0)
			direction /=dist;
		Vec3 directionNew(pTarget->GetPos() - GetPos());

		float distNew = directionNew.GetLength();
		if(distNew>0)
			directionNew /=dist;
		
		float fdot = ((Vec3)direction).Dot((Vec3)myorievector);
		// check if new target is more interesting, by checking if old target is still visible
		// and comparing distances if it is
		if ( fdot <  0 || fdot<m_FOV || distNew < dist)
		{
			m_refAttentionTarget = refTarget;
			m_fLastUpdateTargetTime = GetAISystem()->GetFrameStartTime();
			bUpdateLeaderStats = true;
		}
	}
	if(bUpdateLeaderStats)
	{
		CAIGroup* pGroup = GetAISystem()->GetAIGroup(GetGroupId());
		if(pGroup)
			pGroup->OnUnitAttentionTargetChanged();
	}
}

//
//---------------------------------------------------------------------------------
float CAIPlayer::AdjustTargetVisibleRange(const CAIActor& observer, float fVisibleRange) const
{
	float fRangeScale = 1.0f;

	// Adjust using my light level if the observer is affected by light
	if (observer.GetParameters().m_PerceptionParams.isAffectedByLight)
	{
		const EAILightLevel targetLightLevel = GetLightLevel();
		switch (targetLightLevel)
		{
			//	case AILL_LIGHT: SOMSpeed
			case AILL_MEDIUM: fRangeScale *= gAIEnv.CVars.SightRangeMediumIllumMod; break;
			case AILL_DARK:	fRangeScale *= gAIEnv.CVars.SightRangeDarkIllumMod; break;
		}
	}

	// Scale down sight range when target is underwater based on distance
	const float fCachedWaterOcclusionValue = GetCachedWaterOcclusionValue();
	if (fCachedWaterOcclusionValue > FLT_EPSILON)
	{
		const Vec3& observerPos = observer.GetPos();
		const float fDistance = Distance::Point_Point(GetPos(), observerPos);
		const float fDistanceFactor = (fVisibleRange > FLT_EPSILON ? GetAISystem()->GetVisPerceptionDistScale(fDistance/fVisibleRange) : 0.0f);

		const float fWaterOcclusionEffect = 2.0f*fCachedWaterOcclusionValue + (1-fDistanceFactor)*0.5f;

		fRangeScale *= (fWaterOcclusionEffect > 1.0f ? 0.0f : 1.0f-fWaterOcclusionEffect);
	}

	// Return new range
	return fVisibleRange * fRangeScale;
}

//
//---------------------------------------------------------------------------------
void CAIPlayer::Update(EObjectUpdate type)
{
	if (m_refAttentionTarget.IsValid())
	{
		if ( !m_refAttentionTarget.GetAIObject()->IsEnabled() ||
				(GetAISystem()->GetFrameStartTime()-m_fLastUpdateTargetTime).GetSeconds() > 5.0f )
			m_refAttentionTarget.Reset();
	}

	// There should never be player without physics.
	if (!GetPhysics())
	{
		AIWarning("AIPlayer::Update Player %s does not have physics!", GetName());
		AIAssert(0);
		return;
	}

	if (gAIEnv.CVars.DebugDraw > 0)
	{
		if (!stricmp(gAIEnv.CVars.DrawPerceptionHandlerModifiers,GetName()))
			DebugDrawPerceptionHandlerModifiers();
	}

	CCCPOINT(CAIPlayer_Update);

	// make sure to update direction when entity is not moved
	SAIBodyInfo bodyInfo;
	GetProxy()->QueryBodyInfo( bodyInfo);
	SetPos( bodyInfo.vEyePos );
	SetBodyDir( bodyInfo.vBodyDir );
	SetMoveDir( bodyInfo.vMoveDir );
	SetViewDir( bodyInfo.vEyeDir );
	
	// Determine if position has changed
	if (type == AIUPDATE_FULL && !IsEquivalent(m_vLastFullUpdatePos, bodyInfo.vEyePos, 1.0f))
	{
		// Recalculate the water occlusion at the new point
		m_cachedWaterOcclusionValue = GetAISystem()->GetWaterOcclusionValue(bodyInfo.vEyePos);

		m_vLastFullUpdatePos = bodyInfo.vEyePos;
	}

	m_bUpdatedOnce = true;
	
	m_FOV = cosf(GetAISystem()->GetAIDebugRenderer()->GetCameraFOV());
	
	// (MATT) I'm assuming that AIActor should always have a proxy, or this could be bad for performance {2009/04/03}
	IAIActorProxy *pProxy = GetProxy();
	if (pProxy)
		pProxy->UpdateMind(&m_State);

	// CAIActor updates light level on full updates
	if (type != AIUPDATE_FULL)
		m_lightLevel = GetAISystem()->GetLightManager()->GetLightLevelAt(GetPos(), this, &m_usingCombatLight);
	else
		m_missLocationSensor.Update(0.1f);

	if (type == AIUPDATE_FULL)
	{
		// Health
		{
			RecorderEventData recorderEventData((float)GetProxy()->GetActorHealth());
			RecordEvent(IAIRecordable::E_HEALTH, &recorderEventData);
		}

		// Pos
		{
			RecorderEventData recorderEventData(GetPos());
			RecordEvent(IAIRecordable::E_AGENTPOS, &recorderEventData);
		}

		// Dir
		{
			RecorderEventData recorderEventData(GetViewDir());
			RecordEvent(IAIRecordable::E_AGENTDIR, &recorderEventData);
		}
	}

	const float dt = GetAISystem()->GetFrameDeltaTime();

	// Exposed (soft) covers. When the player fires the weapon, 
	// disable the cover where the player is hiding at or the
	// soft cover the player is shooting at.

	// Timeout the cover exposure timer.
	if (m_coverExposedTime > 0.0f)
		m_coverExposedTime -= dt;
	else
		m_coverExposedTime = -1.0f;

	if (GetProxy())
	{
		SAIWeaponInfo wi;
		GetProxy()->QueryWeaponInfo(wi);
		if (wi.isFiring)
			m_coverExposedTime = 1.0f;
	}

	// Timeout the exposed covers
	for (unsigned i = 0; i < m_exposedCoverObjects.size(); )
	{
		m_exposedCoverObjects[i].t -= dt;
		if (m_exposedCoverObjects[i].t < 0.0f)
		{
			m_exposedCoverObjects[i].pPhysEnt->Release();
			m_exposedCoverObjects[i] = m_exposedCoverObjects.back();
			m_exposedCoverObjects.pop_back();
		}
		else
			++i;
	}

	// Collect new covers.
	if (type == AIUPDATE_FULL)
		CollectExposedCover();


	if(gAIEnv.CVars.DebugDrawDamageControl > 0)
		UpdateHealthHistory();

	UpdatePlayerStuntActions();
	UpdateCloakScale();

	// Update low health mercy pause
	if (m_mercyTimer > 0.0f)
		m_mercyTimer -= GetAISystem()->GetFrameDeltaTime();
	else
		m_mercyTimer = -1.0f;

	UpdateDamageParts(m_damageParts);
}

void CAIPlayer::OnObjectRemoved(CAIObject* pObject)
{
	// (MATT) Moved here from CAISystems call {2009/02/05}
	CPuppet* pRemovedPuppet = pObject->CastToCPuppet();
	if (!pRemovedPuppet)
		return;

	for (unsigned i = 0; i < m_stuntTargets.size(); )
	{
		if (m_stuntTargets[i].pPuppet == pRemovedPuppet)
		{
			m_stuntTargets[i] = m_stuntTargets.back();
			m_stuntTargets.pop_back();
		}
		else
			++i;
	}
}

void CAIPlayer::UpdatePlayerStuntActions()
{
	const float dt = GetAISystem()->GetFrameDeltaTime();

	// Update thrown entities
	for (unsigned i = 0; i < m_lastThrownItems.size(); )
	{
		IEntity* pEnt = gEnv->pEntitySystem->GetEntity(m_lastThrownItems[i].id);
		if (pEnt)
		{
			if (IPhysicalEntity* pPhysEnt = pEnt->GetPhysics())
			{
				pe_status_dynamics statDyn;
				pPhysEnt->GetStatus(&statDyn);
				m_lastThrownItems[i].pos = pEnt->GetWorldPos();
				m_lastThrownItems[i].vel = statDyn.v;
				if (statDyn.v.GetLengthSquared() > sqr(3.0f))
					m_lastThrownItems[i].time = 0;
			}
		}

		m_lastThrownItems[i].time += dt;
		if (!pEnt || m_lastThrownItems[i].time > 1.0f)
		{
			m_lastThrownItems[i] = m_lastThrownItems.back();
			m_lastThrownItems.pop_back();
		}
		else
			++i;
	}

	Vec3 vel = GetVelocity();
	float speed = vel.GetLength();

	m_stuntDir = vel;
	if (m_stuntDir.GetLength() < 0.001f)
		m_stuntDir = GetBodyDir();
	m_stuntDir.z = 0;
	m_stuntDir.NormalizeSafe();

	if (m_playerStuntSprinting > 0.0f)
	{
		if (speed > 10.0f)
			m_playerStuntSprinting = PLAYER_ACTION_SPRINT_RESET_TIME;
		m_playerStuntSprinting -= dt;
	}

	if (m_playerStuntJumping > 0.0f)
	{
		if (speed > 7.0f)
			m_playerStuntJumping = PLAYER_ACTION_JUMP_RESET_TIME;
		m_playerStuntJumping -= dt;
	}

	if (m_playerStuntCloaking > 0.0f)
		m_playerStuntCloaking -= dt;

	if (m_playerStuntUncloaking > 0.0f)
		m_playerStuntUncloaking -= dt;


	bool checkMovement = m_playerStuntSprinting > 0.0f || m_playerStuntJumping > 0.0f;
	bool checkItems = !m_lastThrownItems.empty();

	if (checkMovement || checkItems)
	{
		float movementScale = 0.7f;
//		if (m_playerStuntJumping > 0.0f)
//			movementScale *= 2.0f;

		Lineseg	playerMovement(GetPos(), GetPos() + vel*movementScale);
		SAIBodyInfo bi;
		GetProxy()->QueryBodyInfo(bi);
		float playerRad = bi.stanceSize.GetRadius();

		// Update stunt effect on puppets
		AutoAIObjectIter it(GetAISystem()->GetFirstAIObject(IAISystem::OBJFILTER_TYPE, AIOBJECT_PUPPET));
		for (; it->GetObject(); it->Next())
		{
			CPuppet* pPuppet = it->GetObject()->CastToCPuppet();
			if (!pPuppet) continue;
			if (!pPuppet->IsEnabled()) continue;
			if (!IsHostile(pPuppet)) continue;

			const float scale = pPuppet->GetParameters().m_PerceptionParams.collisionReactionScale;

			bool hit = false;

			float t, distSq;
			Vec3 threatPos;

			if (checkMovement)
			{
				// Player movement
				distSq = Distance::Point_Lineseg2DSq(pPuppet->GetPos(), playerMovement, t);
				if (distSq < sqr(playerRad * scale))
				{
					threatPos = GetPos();
					hit = true;
				}
			}

			if (checkItems)
			{
				// Thrown items
				for (unsigned i = 0, ni = m_lastThrownItems.size(); i < ni; ++i)
				{
					Lineseg	itemMovement(m_lastThrownItems[i].pos, m_lastThrownItems[i].pos + m_lastThrownItems[i].vel*2);
					distSq = Distance::Point_LinesegSq(pPuppet->GetPos(), itemMovement, t);
					if (distSq < sqr(m_lastThrownItems[i].r * 2.0f * scale))
					{
						threatPos = m_lastThrownItems[i].pos;
						hit = true;
					}
				}
			}

			if (hit)
			{
				bool found = false;
				for (unsigned i = 0, ni = m_stuntTargets.size(); i < ni; ++i)
				{
					if (m_stuntTargets[i].pPuppet == pPuppet)
					{
						m_stuntTargets[i].threatPos = threatPos;
						m_stuntTargets[i].exposed += dt;
						m_stuntTargets[i].t = 0;
						found = true;
						break;
					}
				}
				if (!found)
				{
					m_stuntTargets.push_back(SStuntTargetPuppet(pPuppet, threatPos));
					m_stuntTargets.back().exposed += dt;
				}
			}
		}
	}

	for (unsigned i = 0; i < m_stuntTargets.size(); )
	{
		m_stuntTargets[i].t += dt;
		if (!m_stuntTargets[i].signalled 
				&& m_stuntTargets[i].exposed > 0.15f 
				&& m_stuntTargets[i].pPuppet->GetAttentionTarget() 
				&& m_stuntTargets[i].pPuppet->GetAttentionTarget()->GetEntityID() == GetEntityID())
		{
			IAISignalExtraData* pData = GetAISystem()->CreateSignalExtraData();
			pData->iValue = 1;
			pData->fValue = Distance::Point_Point(m_stuntTargets[i].pPuppet->GetPos(), m_stuntTargets[i].threatPos);
			pData->point = m_stuntTargets[i].threatPos;
			m_stuntTargets[i].pPuppet->SetSignal(1, "OnCloseCollision", 0, pData);
			m_stuntTargets[i].pPuppet->SetAlarmed();

			m_stuntTargets[i].signalled = true;
		}
		if (m_stuntTargets[i].t > 2.0f)
		{
			m_stuntTargets[i] = m_stuntTargets.back();
			m_stuntTargets.pop_back();
		}
		else
			++i;
	}

}

bool CAIPlayer::IsDoingStuntActionRelatedTo(const Vec3& pos, float nearDistance)
{
	if (m_playerStuntCloaking > 0.0f)
		return true;

	if (m_playerStuntSprinting <= 0.0f && m_playerStuntJumping <= 0.0f)
		return false;

	// If the stunt is not done at really close range,
	// do not consider the stunt unless it is towards the specified position.
	Vec3 diff = pos - GetPos();
	diff.z = 0;
	float dist = diff.NormalizeSafe();
	const float thr = cosf(DEG2RAD(75.0f));
	if (dist > nearDistance && m_stuntDir.Dot(diff) < thr)
		return false;

	return true;
}

bool CAIPlayer::IsThrownByPlayer(EntityId id) const
{
	if (m_lastThrownItems.empty()) return false;
	for (unsigned i = 0, ni = m_lastThrownItems.size(); i < ni; ++i)
		if (m_lastThrownItems[i].id == id)
			return true;
	return false;
}

bool CAIPlayer::IsPlayerStuntAffectingTheDeathOf(CAIActor* pDeadActor) const
{
	if (!pDeadActor || !pDeadActor->GetEntity())
		return false;

	// If the actor is thrown/punched by the player.
	if (IsThrownByPlayer(pDeadActor->GetEntityID()))
		return true;

	// If any of the objects the player has thrown is close to the dead body.
	AABB deadBounds;
	pDeadActor->GetEntity()->GetWorldBounds(deadBounds);
	Vec3 deadPos = deadBounds.GetCenter();
	float deadRadius = deadBounds.GetRadius();

	for (unsigned i = 0, ni = m_lastThrownItems.size(); i < ni; ++i)
	{
		if (Distance::Point_PointSq(deadPos, m_lastThrownItems[i].pos) < sqr(deadRadius + m_lastThrownItems[i].r))
			return true;
	}

	return false;
}

EntityId CAIPlayer::GetNearestThrownEntity(const Vec3& pos)
{
	EntityId nearest = 0;
	float nearestDist = FLT_MAX;
	for (unsigned i = 0, ni = m_lastThrownItems.size(); i < ni; ++i)
	{
		float d = Distance::Point_Point(pos, m_lastThrownItems[i].pos) - m_lastThrownItems[i].r;
		if (d < nearestDist)
		{
			nearestDist = d;
			nearest = m_lastThrownItems[i].id;
		}
	}
	return nearest;
}

void CAIPlayer::AddThrownEntity(EntityId id)
{
	float oldestTime = 0.0f;
	unsigned oldestId = 0;
	for (unsigned i = 0, ni = m_lastThrownItems.size(); i < ni; ++i)
	{
		if (m_lastThrownItems[i].id == id)
		{
			m_lastThrownItems[i].time = 0;
			return;
		}
		if (m_lastThrownItems[i].time > oldestTime)
		{
			oldestTime = m_lastThrownItems[i].time;
			oldestId = i;
		}
	}

	IEntity* pEnt = gEnv->pEntitySystem->GetEntity(id);
	if (!pEnt)
		return;

	// The entity does not exists yet, add it to the list of entities to watch.
	m_lastThrownItems.push_back(SThrownItem(id));

	// Skip the nearest thrown entity, since it is potentially blocking the view to the corpse.
	IEntity* pThrownEnt = id ? gEnv->pEntitySystem->GetEntity(id) : 0;
	CAIActor* pThrownActor = CastToCAIActorSafe(pThrownEnt->GetAI());
	if (pThrownActor)
	{
		short gid = (short)pThrownActor->GetGroupId();
		CAISystem::AIObjects::iterator ai = GetAISystem()->m_mapGroups.find(gid);
		CAISystem::AIObjects::iterator end = GetAISystem()->m_mapGroups.end();
		for ( ; ai != end && ai->first == gid; ++ai)
		{
			CPuppet* pPuppet = CastToCPuppetSafe( ai->second.GetAIObject() );
			if (!pPuppet) continue;
			if (pPuppet->GetEntityID() == pThrownActor->GetEntityID()) continue;
			float dist = FLT_MAX;
			if (!GetAISystem()->CheckVisibilityToBody(pPuppet, pThrownActor, dist))
				continue;
			pPuppet->SetSignal(1, "OnGroupMemberMutilated", pThrownActor->GetEntity(), 0);
			pPuppet->SetAlarmed();
		}
	}

	// Set initial position, radius and velocity.
	m_lastThrownItems.back().pos = pEnt->GetWorldPos();
	AABB bounds;
	pEnt->GetWorldBounds(bounds);
	m_lastThrownItems.back().r = bounds.GetRadius();

	if (IPhysicalEntity* pPhysEnt = pEnt->GetPhysics())
	{
		pe_status_dynamics statDyn;
		pPhysEnt->GetStatus(&statDyn);
		m_lastThrownItems.back().vel = statDyn.v;
	}

	// Limit the number of hot entities.
	if (m_lastThrownItems.size() > 4)
	{
		m_lastThrownItems[oldestId] = m_lastThrownItems.back();
		m_lastThrownItems.pop_back();
	}
}

void CAIPlayer::HandleCloaking(bool cloak)
{
	// All puppets that have the player
	// Update stunt effect on puppets
	AutoAIObjectIter it(GetAISystem()->GetFirstAIObject(IAISystem::OBJFILTER_TYPE, AIOBJECT_PUPPET));
	for (; it->GetObject(); it->Next())
	{
		CPuppet* pPuppet = it->GetObject()->CastToCPuppet();
		if (!pPuppet) continue;
		if (!pPuppet->IsEnabled()) continue;
		if (!pPuppet->GetAttentionTarget()) continue;
		if (pPuppet->GetAttentionTarget()->GetEntityID() == GetEntityID())
		{
/*			IAISignalExtraData* pData = GetAISystem()->CreateSignalExtraData();
			pData->fValue = Distance::Point_Point(m_stuntTargets[i].pPuppet->GetPos(), m_stuntTargets[i].threatPos);
			pData->point = m_stuntTargets[i].threatPos;*/
			pPuppet->SetSignal(1, cloak ? "OnTargetCloaked" : "OnTargetUncloaked", 0, 0);
		}
	}
}

//-----------------------------------------------------------

void CAIPlayer::Event(unsigned short eType, SAIEVENT *pEvent)
{
	switch (eType)
	{
	case AIEVENT_AGENTDIED:
		// make sure everybody knows I have died
		GetAISystem()->NotifyTargetDead(this);
		m_bEnabled = false;
		GetAISystem()->RemoveFromGroup(GetGroupId(), this);

		GetAISystem()->ReleaseFormationPoint(this);
		ReleaseFormation();

		m_State.vSignals.clear();

		if(GetProxy())
			GetProxy()->Reset(AIOBJRESET_SHUTDOWN);
		break;
	case AIEVENT_PLAYER_STUNT_SPRINT:
		m_playerStuntSprinting = PLAYER_ACTION_SPRINT_RESET_TIME;
		m_playerStuntJumping = -1.0f;
		break;
	case AIEVENT_PLAYER_STUNT_JUMP:
		m_playerStuntJumping = PLAYER_ACTION_JUMP_RESET_TIME;
		m_playerStuntSprinting = -1.0f;
		break;
	case AIEVENT_PLAYER_STUNT_PUNCH:
		if (pEvent)
			AddThrownEntity(pEvent->targetId);
		break;
	case AIEVENT_PLAYER_STUNT_THROW:
		if (pEvent)
			AddThrownEntity(pEvent->targetId);
		break;
	case AIEVENT_PLAYER_STUNT_THROW_NPC:
		if (pEvent)
			AddThrownEntity(pEvent->targetId);
		break;
	case AIEVENT_PLAYER_THROW:
		if (pEvent)
			AddThrownEntity(pEvent->targetId);
		break;
	case AIEVENT_PLAYER_STUNT_CLOAK:
		m_playerStuntCloaking = PLAYER_ACTION_CLOAK_RESET_TIME;
		HandleCloaking(true);
		break;
	case AIEVENT_PLAYER_STUNT_UNCLOAK:
		m_playerStuntUncloaking = PLAYER_ACTION_CLOAK_RESET_TIME;
		HandleCloaking(false);
		break;
	case AIEVENT_LOWHEALTH:
		m_mercyTimer = gAIEnv.CVars.RODLowHealthMercyTime;
		break;

	default:
		CAIObject::Event(eType, pEvent);
		break;
	}
}

//
//---------------------------------------------------------------------------------
DamagePartVector* CAIPlayer::GetDamageParts()
{
	return &m_damageParts;
}

//
//----------------------------------------------------------------------------------------------
void	CAIPlayer::RecordSnapshot()
{
	// Currently not used
}

//
//----------------------------------------------------------------------------------------------
void	CAIPlayer::RecordEvent(IAIRecordable::e_AIDbgEvent event, const IAIRecordable::RecorderEventData* pEventData)
{
#ifdef CRYAISYSTEM_DEBUG
	CRecorderUnit *pRecord = (CRecorderUnit*)GetAIDebugRecord();
	if(pRecord!=NULL)
	{
		pRecord->RecordEvent(event, pEventData);
	}
#endif //CRYAISYSTEM_DEBUG
}

bool CAIPlayer::GetMissLocation(const Vec3& shootPos, const Vec3& shootDir, float maxAngle, Vec3& pos)
{
	return m_missLocationSensor.GetLocation(shootPos, shootDir, maxAngle, pos);
}

//
//---------------------------------------------------------------------------------
void CAIPlayer::DebugDraw()
{
	CDebugDrawContext dc;

	// Draw items associated with player actions.
	for (unsigned i = 0, ni = m_lastThrownItems.size(); i < ni; ++i)
	{
//		IEntity* pEnt = gEnv->pEntitySystem->GetEntity(m_lastThrownItems[i].id);
//		if (pEnt)
		{
/*			AABB bounds;
			pEnt->GetWorldBounds(bounds);
			dc->DrawAABB(bounds, false, ColorB(255, 0, 0), eBBD_Faceted);
			bounds.Move(m_lastThrownItems[i].vel);
			dc->DrawAABB(bounds, false, ColorB(255, 0, 0, 128), eBBD_Faceted);*/

			AABB bounds(AABB::RESET);
			bounds.Add(m_lastThrownItems[i].pos, m_lastThrownItems[i].r);
			dc->DrawAABB(bounds, false, ColorB(255, 0, 0), eBBD_Faceted);
			bounds.Move(m_lastThrownItems[i].vel);
			dc->DrawLine(m_lastThrownItems[i].pos, ColorB(255, 0, 0), m_lastThrownItems[i].pos + m_lastThrownItems[i].vel, ColorB(255, 0, 0, 128));
			dc->DrawAABB(bounds, false, ColorB(255, 0, 0, 128), eBBD_Faceted);

//			Vec3 dir = m_lastThrownItems[i].vel;
//			float speed = dir.NormalizeSafe(Vec3(1,0,0));

/*			float speed = m_lastThrownItems[i].vel.GetLength();
			if (speed > 0.0f)
			{
			}
			else
			{
			}*/
		}
	}

	for (unsigned i = 0, ni = m_stuntTargets.size(); i < ni; ++i)
	{
		SAIBodyInfo	bodyInfo;
		m_stuntTargets[i].pPuppet->GetProxy()->QueryBodyInfo(bodyInfo);
		Vec3	pos = m_stuntTargets[i].pPuppet->GetPhysicsPos();
		AABB	aabb(bodyInfo.stanceSize);
		aabb.Move(pos);
		dc->DrawAABB(aabb, true, ColorB(255, 255, 255, m_stuntTargets[i].signalled ? 128 : 48), eBBD_Faceted);
	}

	ColorB color(255, 255, 255);

	// Draw special player actions
	if (m_playerStuntSprinting > 0.0f)
	{
		Vec3 pos = GetPos();
		Vec3 vel = GetVelocity();
		SAIBodyInfo bi;
		GetProxy()->QueryBodyInfo(bi);
		float r = bi.stanceSize.GetRadius();
		AABB bounds(AABB::RESET);
		bounds.Add(pos, r);
		dc->DrawAABB(bounds, false, ColorB(255, 0, 0), eBBD_Faceted);
		bounds.Move(vel);
		dc->DrawLine(pos, ColorB(255, 0, 0), pos + vel, ColorB(255, 0, 0, 128));
		dc->DrawAABB(bounds, false, ColorB(255, 0, 0, 128), eBBD_Faceted);

		// [2/27/2009 evgeny] Here and below in this method,
		// first argument for Draw2dLabel was 10, not 100, and the text was hardly visible
		dc->Draw2dLabel(100, 10, 2.5f, color, true, "SPRINTING");
	}
	if (m_playerStuntJumping > 0.0f)
	{
		Vec3 pos = GetPos();
		Vec3 vel = GetVelocity();
		SAIBodyInfo bi;
		GetProxy()->QueryBodyInfo(bi);
		float r = bi.stanceSize.GetRadius();
		AABB bounds(AABB::RESET);
		bounds.Add(pos, r);
		dc->DrawAABB(bounds, false, ColorB(255, 0, 0), eBBD_Faceted);
		bounds.Move(vel);
		dc->DrawLine(pos, ColorB(255, 0, 0), pos + vel, ColorB(255, 0, 0, 128));
		dc->DrawAABB(bounds, false, ColorB(255, 0, 0, 128), eBBD_Faceted);

		dc->Draw2dLabel(100, 40, 2.5f, color, true, "JUMPING");
	}
	if (m_playerStuntCloaking > 0.0f)
	{
		dc->Draw2dLabel(100, 70, 2.5f, color, true, "CLOAKING");
	}
	if (m_playerStuntUncloaking > 0.0f)
	{
		dc->Draw2dLabel(100, 110, 2.5f, color, true, "UNCLOAKING");
	}
	if (!m_lastThrownItems.empty())
	{
		dc->Draw2dLabel(100, 150, 2.5f, color, true, "THROWING");
	}

	if (IsLowHealthPauseActive())
		dc->Draw2dLabel(100, 190, 2.0f, color, true, "Mercy t=%.2fs/%.2f", m_mercyTimer, gAIEnv.CVars.RODLowHealthMercyTime);

	for (unsigned i = 0, ni = m_exposedCoverObjects.size(); i < ni; ++i)
	{
		pe_status_pos statusPos;
		m_exposedCoverObjects[i].pPhysEnt->GetStatus(&statusPos);
		AABB bounds(AABB::RESET);
		bounds.Add(statusPos.BBox[0] + statusPos.pos);
		bounds.Add(statusPos.BBox[1] + statusPos.pos);
		dc->DrawAABB(bounds, false, ColorB(255, 0, 0), eBBD_Faceted);
		dc->Draw3dLabel(bounds.GetCenter(), 1.1f, "IGNORED %.1fs", m_exposedCoverObjects[i].t);
	}
}

//
//---------------------------------------------------------------------------------
bool CAIPlayer::IsLowHealthPauseActive() const
{
	if (m_mercyTimer > 0.0f)
		return true;
	return false;
}

//
//------------------------------------------------------------------------------------------------------------------------
IEntity* CAIPlayer::GetGrabbedEntity() const
{
	if (!GetProxy())
		return NULL;
	return GetProxy()->GetGrabbedEntity();
}



//
//---------------------------------------------------------------------------------
bool CAIPlayer::Serialize(TSerialize ser, class CObjectTracker& objectTracker )
{
	ser.BeginGroup("AIPlayer");

	const bool bNeedsPostSerialize = CAIActor::Serialize(ser,objectTracker);

	m_refAttentionTarget.Serialize(ser, "m_refAttentionTarget");

	ser.Value("m_fLastUpdateTargetTime",m_fLastUpdateTargetTime);
	ser.Value("m_FOV",m_FOV);

	ser.Value("m_playerStuntSprinting", m_playerStuntSprinting);
	ser.Value("m_playerStuntJumping", m_playerStuntJumping);
	ser.Value("m_playerStuntCloaking", m_playerStuntCloaking);
	ser.Value("m_playerStuntUncloaking", m_playerStuntUncloaking);
	ser.Value("m_stuntDir", m_stuntDir);
	ser.ValueWithDefault("m_mercyTimer", m_mercyTimer, -1.0f);

	ser.EndGroup();

	return bNeedsPostSerialize;
}
