/*************************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 4:3:2009   11:38 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "VisionMap.h"

#include "IRenderAuxGeom.h"

#include <CAISystem.h>


static const uint32 VISIBILITY_MAP_PHYS_FOREIGN_ID = PHYS_FOREIGN_ID_USER+1337;


CVisionMap::CVisionMap()
: m_observablesSpace(Vec3(20.0f, 20.0f, 20.0f))
, m_rayCastCount(0)
, m_rayResultCount(0)
, m_pvsUpdateCount(0)
, m_visUpdateCount(0)
, m_maxLatency(0.0f)
, m_maxLatencyQueueSize(0)
, m_genId(0)
{
}

bool CVisionMap::OnBeforeSpawn(SEntitySpawnParams& params)
{
	return true;
}

void CVisionMap::OnSpawn(IEntity* pEntity,SEntitySpawnParams& params)
{
}

bool CVisionMap::OnRemove(IEntity* pEntity)
{
	return true;
}

void CVisionMap::OnEvent(IEntity* pEntity, SEntityEvent& event)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (event.event == ENTITY_EVENT_XFORM)
	{
		IPhysicalEntity* pPhysics = pEntity->GetPhysics();
		if (!pPhysics)
			return;

		pe_status_dynamics dynamics;
		if (pPhysics->GetStatus(&dynamics))
		{
			if (dynamics.v.len2() > 25.0f*25.0f)
				return;
		}

		pe_params_part part;
		part.partid = 0;
		if (pPhysics->GetParams(&part))
		{
			if (part.pPhysGeom && part.pPhysGeom->V <= cube(0.25f))
				return;
			else if (!part.pPhysGeom && part.pPhysGeomProxy && part.pPhysGeomProxy->V <= cube(0.25f))
				return;
		}

		Vec3 pos = pEntity->GetWorldPos();

		Observers::iterator obsIt = m_observers.begin();
		Observers::iterator end = m_observers.end();

		for ( ; obsIt != end; ++obsIt)
		{
			// PVS is getting updated anyway so ignore
			if (obsIt->second.dirtyPVS || obsIt->second.dirtyVis)
				continue;

			ObserverInfo& observerInfo = obsIt->second;
			ObserverParams& observerParams = observerInfo.params;

			const Vec3& eyePos = observerParams.eyePos;

			float rangeSq = observerParams.sightRange * observerParams.sightRange;
			float distanceSq = (pos - eyePos).len2();

			if (distanceSq > rangeSq)
				continue;

			if (!IsPointInFront(eyePos, observerParams.eyeDir, pos))
				continue;

			observerInfo.dirtyVis = true;
		}
	}
}

void CVisionMap::Init()
{
	gEnv->pPhysicalWorld->AddEventClient(EventPhysRWIResult::id, RWIResult, 1);
	gEnv->pEntitySystem->AddSink(this);

	m_priorityClasses.push_back(PriorityClassInfo(1.0f/10.0f, 1.0f,		100.0f, 0.5f));
	m_priorityClasses.push_back(PriorityClassInfo(1.0f/10.0f, 10.0f,	10.0f,	0.4f));
	m_priorityClasses.push_back(PriorityClassInfo(1.0f/25.0f, 25.0f,	4.0f,		0.3f));
	m_priorityClasses.push_back(PriorityClassInfo(1.0f/25.0f, 50.0f,	2.5f,		0.25f));
}

void CVisionMap::Destroy()
{
	gEnv->pEntitySystem->RemoveSink(this);
	gEnv->pPhysicalWorld->RemoveEventClient(EventPhysRWIResult::id, RWIResult, 1);

	m_observablesSpace.Clear();
}

void CVisionMap::Reset()
{
	Observers::iterator it = m_observers.begin();
	Observers::iterator end = m_observers.end();

	for ( ; it != end; ++it)
		DeletePendingRays(it->second.pvs);

	m_rayCastCount = 0;
	m_rayResultCount = 0;
	m_pvsUpdateCount = 0;
	m_visUpdateCount = 0;
	m_maxLatency = 0.0f;
	m_maxLatencyQueueSize = 0;

	m_observers.clear();

	m_observablesSpace.Clear();
	m_observables.clear();
	m_pendingRays.clear();
}

VisionID CVisionMap::CreateVisionID(const char* name)
{
	if (m_genId == 0xffffffff)	// handle wrapping
		++m_genId;

	return VisionID(++m_genId, name);
}

void CVisionMap::RegisterObserver(const ObserverID& observerID, const ObserverParams& params)
{
	assert(observerID);
	if (!observerID)
		return;

	m_observers.insert(Observers::value_type(observerID, ObserverInfo()));

	ObserverChanged(observerID, params, eChangedAll);
}

void CVisionMap::UnregisterObserver(const ObserverID& observerID)
{
	assert(observerID);
	if (!observerID)
		return;

	Observers::iterator it = m_observers.find(observerID);
	if (it == m_observers.end())
		return;

	DeletePendingRays(it->second.pvs);

	m_observers.erase(it);
}

void CVisionMap::RegisterObservable(const ObservableID& observableID, const ObservableParams& params)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	assert(observableID);

	if (!observableID)
		return;

	assert(params.posCount > 0);
	assert(params.posCount <= ObservableParams::MaxPositionCount);

	std::pair<Observables::iterator, bool> result = m_observables.insert(Observables::value_type(observableID, ObservableInfo(observableID, params)));
	ObservableInfo& info = result.first->second;

	info.params.pos[0].zero(); // trigger a position change in ObservableChanged

	m_observablesSpace.RegisterObject(&info);
	
	ObservableChanged(observableID, params, eChangedAll);
}

void CVisionMap::UnregisterObservable(const ObservableID& observableID)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	assert(observableID);

	if (!observableID)
		return;

	Observables::iterator obsIt = m_observables.find(observableID);
	if (obsIt == m_observables.end())
		return;

	Observers::iterator it = m_observers.begin();
	Observers::iterator end = m_observers.end();

	for ( ; it != end; ++it)
	{
		if (it->first == observableID)
			continue;

		ObserverInfo& observerInfo = it->second;

		PVS::iterator pvsIt = observerInfo.pvs.find(observableID);
		if (pvsIt == observerInfo.pvs.end())
			continue;
			
		PVSEntry& entry = pvsIt->second;
		if (entry.visible && observerInfo.params.callback)
			observerInfo.params.callback(it->first, observerInfo.params, 
				observableID, entry.observableInfo->params, false);

		DeletePendingRay(pvsIt->second);

		observerInfo.pvs.erase(pvsIt);
		observerInfo.dirtyPVS = true;
	}

	m_observablesSpace.UnregisterObject(&obsIt->second);
	m_observables.erase(obsIt);
}

void CVisionMap::ObserverChanged(const ObserverID& observerID, const ObserverParams& params, uint32 hint)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	assert(observerID);

	Observers::iterator it = m_observers.find(observerID);
	assert(it != m_observers.end());
	if (it == m_observers.end())
		return;

	bool needsUpdate = false;
	ObserverInfo& observerInfo = it->second;
	ObserverParams& myparams = observerInfo.params;

	// if only priority changed
	if (hint & eChangedPriority)
	{
		myparams.priority = params.priority;
	}
	
	if (hint & eChangedFaction)
	{
		myparams.factionMask = params.factionMask;
		needsUpdate = true;
	}

	if (hint & eChangedSight)
	{
		myparams.sightRange = params.sightRange;
		myparams.primaryFoVCos = params.primaryFoVCos;
		myparams.peripheralFoVCos = params.peripheralFoVCos;
		needsUpdate = true;
	}

	if (hint & eChangedPosition)
	{
		if (!IsEquivalent(myparams.eyePos, params.eyePos, 0.05f))
		{
			myparams.eyePos = params.eyePos;
			needsUpdate = true;
		}
	}

	if (hint & eChangedOrientation)
	{
		if (!IsEquivalent(myparams.eyeDir, params.eyeDir, 0.05f))
		{
			myparams.eyeDir = params.eyeDir;
			needsUpdate = true;
		}
	}

	if (hint & eChangedSkipList)
	{
		assert(params.skipListSize <= ObserverParams::MaxSkipListSize);

		myparams.skipListSize = MIN(params.skipListSize, ObserverParams::MaxSkipListSize);
		for (int i = 0; i < myparams.skipListSize; ++i)
			myparams.skipList[i] = params.skipList[i];
	}

	if (hint & eChangedCallback)
	{
		myparams.callback = params.callback;
	}

	if (hint & eChangedUserData)
	{
		myparams.userData = params.userData;
	}

	if (hint & eChangedEntityId)
	{
		myparams.entityID = params.entityID;
	}

	if (needsUpdate)
		observerInfo.dirtyPVS = true;
}

void CVisionMap::ObservableChanged(const ObservableID& observableID, const ObservableParams& params, uint32 hint)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	assert(observableID);

	Observables::iterator observableIt = m_observables.find(observableID);
	assert(observableIt != m_observables.end());
	if (observableIt == m_observables.end())
		return;

	ObservableInfo& observableInfo = observableIt->second;
	ObservableParams& observableParams = observableInfo.params;

	bool visibilityChanged = false;
	if (hint & eChangedPosition)
	{
		assert(params.posCount > 0);
		assert(params.posCount <= ObservableParams::MaxPositionCount);
		assert(params.pos[0].IsValid());

		Vec3 oldpos = observableParams.pos[0];

		if (!IsEquivalent(oldpos, params.pos[0], 0.05f))
		{
			FRAME_PROFILER("ObservableChanged_UpdateHashedSpace", GetISystem(), PROFILE_AI);
		
			observableParams.pos[0] = params.pos[0];
			m_observablesSpace.ObjectMoved(&observableInfo, oldpos);
			visibilityChanged = true;
			
			observableParams.posCount = params.posCount;

			for (int i = 1; i < observableParams.posCount; ++i)
			{
				assert(params.pos[i].IsValid());
				observableParams.pos[i] = params.pos[i];
			}
		}
	}

	if (hint & eChangedUserData)
	{
		observableParams.userData = params.userData;
	}

	if (hint & eChangedCallback)
	{
		observableParams.callback = params.callback;
	}

	if (hint & eChangedCamouflage)
	{
		observableParams.camouflage = params.camouflage;
		visibilityChanged = true;
	}

	if (hint & eChangedSkipList)
	{
		assert(params.skipListSize <= ObserverParams::MaxSkipListSize);

		observableParams.skipListSize = MIN(params.skipListSize, ObservableParams::MaxSkipListSize);
		for (int i = 0; i < observableParams.skipListSize; ++i)
			observableParams.skipList[i] = params.skipList[i];
	}

	if (hint & eChangedFaction)
	{
		observableParams.faction = params.faction;
		visibilityChanged = true;
	}

	Observers::iterator obsIt = m_observers.begin();
	Observers::iterator end = m_observers.end();

	for ( ; obsIt != end; ++obsIt)
	{
		if (obsIt->first == observableID)
			continue;

		// PVS is getting updated anyway so ignore
		if (obsIt->second.dirtyPVS)
			continue;

		ObserverInfo& observerInfo = obsIt->second;
		ObserverParams& observerParams = observerInfo.params;

		if (visibilityChanged)
		{
			const Vec3& eyePos = observerParams.eyePos;

			float rangeSq = sqr(observerParams.sightRange) * sqr(1.0f - observableParams.camouflage);
			float distanceSq = (observableParams.pos[0] - eyePos).len2();

			PVS::iterator pvsIt = observerInfo.pvs.find(observableID);
			bool inPVS = (pvsIt != observerInfo.pvs.end());
			bool visible = inPVS && pvsIt->second.visible;

			bool factionMatch = (observerParams.factionMask & (1 << observableParams.faction)) != 0;
			bool typeMatch = (observerParams.typeMask & (1 << observableParams.type)) != 0;

			float angleCos;
			if (!factionMatch || 
				!typeMatch || 
				(distanceSq > rangeSq) || 
				!IsPointInFoV(eyePos, observerParams.eyeDir, observableParams.pos[0], observerParams.peripheralFoVCos, distanceSq, &angleCos))
			{
				if (inPVS)
				{
					FRAME_PROFILER("ObservableChanged_PVSErase1", GetISystem(), PROFILE_AI);

					if (visible)
					{
						if (observerParams.callback)
							observerParams.callback(obsIt->first, observerParams, observableID, observableInfo.params, false);
						if (observableInfo.params.callback)
							observableInfo.params.callback(obsIt->first, observerParams, observableID, observableInfo.params, false);
					}

					DeletePendingRay(pvsIt->second);

					observerInfo.pvs.erase(pvsIt);
					observerInfo.dirtyVis = true;
				}

				continue;
			}

			float priorityScale = GetPriorityScale(observerParams.primaryFoVCos, observerParams.peripheralFoVCos, angleCos);

			if (!inPVS)
			{
				FRAME_PROFILER("ObservableChanged_PVSInsert", GetISystem(), PROFILE_AI);

				std::pair<PVS::iterator, bool> result = observerInfo.pvs.insert(PVS::value_type(observableID, PVSEntry()));
				result.first->second.priorityScale = priorityScale;
				result.first->second.observableInfo = &observableInfo;
			}
			else
				pvsIt->second.priorityScale = priorityScale;

			// at this point contents of this pvs have changed for sure
			observerInfo.dirtyVis = true;
		}
	}
}

bool CVisionMap::IsVisible(const ObserverID& observerID, const ObservableID& observableID) const
{
	Observers::const_iterator obsIt = m_observers.find(observerID);
	if (obsIt == m_observers.end())
		return false;

	const ObserverInfo& observerInfo = obsIt->second;
	PVS::const_iterator pvsIt = observerInfo.pvs.find(observableID);
	if (pvsIt == observerInfo.pvs.end())
		return false;

	return pvsIt->second.visible;
}

const ObserverParams* CVisionMap::GetObserverParams(const ObserverID& observerID) const
{
	Observers::const_iterator obsIt = m_observers.find(observerID);
	if (obsIt == m_observers.end())
		return 0;

	const ObserverInfo& observerInfo = obsIt->second;

	return &observerInfo.params;
}

const ObservableParams* CVisionMap::GetObservableParams(const ObservableID& observableID) const
{
	Observables::const_iterator obsIt = m_observables.find(observableID);
	if (obsIt == m_observables.end())
		return 0;

	const ObservableInfo& observableInfo = obsIt->second;

	return &observableInfo.params;
}

void CVisionMap::Update(float frameTime)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CTimeValue now = gEnv->pTimer->GetFrameStartTime();

	Observers::iterator it = m_observers.begin();
	Observers::iterator end = m_observers.end();

	for ( ; it != end; ++it)
	{
		ObserverInfo& observerInfo = it->second;
		ObserverParams& observerParams = observerInfo.params;

		float period = m_priorityClasses[observerParams.priority].updateInterval;

		if (observerInfo.dirtyPVS && (now-observerInfo.pvsUpdated).GetSeconds() >= period)
		{
			UpdatePVS(it->first, observerInfo, now);

			observerInfo.dirtyPVS = false;
			observerInfo.dirtyVis = true;
			observerInfo.pvsUpdated = now;

			++m_pvsUpdateCount;
		}

		if (observerInfo.dirtyVis && (now-observerInfo.visUpdated).GetSeconds() >= period)
		{
			UpdateVisibility(it->first, observerInfo, now);

			observerInfo.dirtyVis = false;

			++m_visUpdateCount;
		}
	}

	UpdateRayPriorities(frameTime);
	ProcessRayQueue();

	if (gAIEnv.CVars.DebugDrawVisionMap != 0)
		DebugDraw();

	m_rayCastCount = 0;
	m_rayResultCount = 0;
	m_pvsUpdateCount = 0;
	m_visUpdateCount = 0;
	m_maxLatency = 0.0f;
	m_maxLatencyQueueSize = 0;
}

void CVisionMap::UpdatePVS(const ObserverID& observerID, ObserverInfo& observerInfo, const CTimeValue& now)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	ObserverParams& params = observerInfo.params;

	const Vec3 eyePos = params.eyePos;
	const Vec3 eyeDir = params.eyeDir;

	float baseSightRangeSq = sqr(params.sightRange);
	PVS& pvs = observerInfo.pvs;

	assert(eyeDir.IsUnit());
	assert(eyePos.IsValid());

	// TODO: Think of a better way to do the following:
	// (It's needed because we want to keep the state of the visibles for the remaining of this frame, until new results come in)

	// Step1:
	//	-Make sure everything in the PVS is in supposed to be there
	//		-Delete what it's not

	// Step2:
	//	-Go through all objects in range
	//		-If object is already in the PVS skip it
	//		-Otherwise check if it should be added and add it

	{
		FRAME_PROFILER("UpdatePVS_Step1", GetISystem(), PROFILE_AI);

		PVS::iterator pvsIt = pvs.begin();
		PVS::iterator end = pvs.end();

		for ( ; pvsIt != end; )
		{
			const ObservableParams& observableParams = pvsIt->second.observableInfo->params;
			const Vec3& pos = observableParams.pos[0];
			bool factionMatch = (params.factionMask & (1 << observableParams.faction)) != 0;
			bool typeMatch = (params.typeMask & (1 << observableParams.type)) != 0;

			float sightRangeSq = baseSightRangeSq * sqr(1.0f - observableParams.camouflage);

			float angleCos;
			float distanceSq = (pos - eyePos).len2();

			if (!typeMatch ||
				!factionMatch ||
				(distanceSq > sightRangeSq) ||
				!IsPointInFoV(eyePos, eyeDir, pos, params.peripheralFoVCos, distanceSq, &angleCos))
			{
				PVS::iterator next = pvsIt;
				++next;

				if (pvsIt->second.visible)
				{
					if (params.callback)
						params.callback(observerID, params, pvsIt->first, pvsIt->second.observableInfo->params, false);

					if (pvsIt->second.observableInfo->params.callback)
						pvsIt->second.observableInfo->params.callback(observerID, params, pvsIt->first, pvsIt->second.observableInfo->params, false);
				}

				DeletePendingRay(pvsIt->second);

				pvs.erase(pvsIt);
				pvsIt = next;

				continue;
			}
			else
				pvsIt->second.priorityScale = GetPriorityScale(params.primaryFoVCos, params.peripheralFoVCos, angleCos);

			++pvsIt;
		}
	}

	{
		FRAME_PROFILER("UpdatePVS_Step2", GetISystem(), PROFILE_AI);

		ObservablesSpace::radial_iterator it = m_observablesSpace.BeginRadial(eyePos, params.sightRange);
		ObservableInfo *observableInfo;

		for ( ; observableInfo = it; ++it)
		{
			if (it->id == observerID)
				continue;

			PVS::iterator pvsIt = pvs.find(observableInfo->id);
			if (pvsIt != pvs.end())
				continue;

			const ObservableParams& observableParams = observableInfo->params;
			bool factionMatch = (params.factionMask & (1 << observableParams.faction)) != 0;
			bool typeMatch = (params.factionMask & (1 << observableParams.faction)) != 0;

			if (!factionMatch || !typeMatch)
					continue;

			float distanceSq = it.GetDistSq();
			float sightRangeSq = baseSightRangeSq * sqr(1.0f - observableParams.camouflage);
			float angleCos;

			if ((distanceSq <= sightRangeSq) &&
				IsPointInFoV(eyePos, eyeDir, observableInfo->params.pos[0], params.peripheralFoVCos, distanceSq, &angleCos))
			{
				assert(m_observables.find(observableInfo->id) != m_observables.end());	// Consistency check

				std::pair<PVS::iterator, bool> result = pvs.insert(PVS::value_type(observableInfo->id, PVSEntry()));
				result.first->second.priorityScale = GetPriorityScale(params.primaryFoVCos, params.peripheralFoVCos, angleCos);
				result.first->second.observableInfo = observableInfo;
			}
		}
	}
}

void CVisionMap::UpdateVisibility(const ObserverID& observerID, ObserverInfo& observerInfo, const CTimeValue& now)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	PVS& pvs = observerInfo.pvs;

	PVS::iterator pvsIt = pvs.begin();
	PVS::iterator end = pvs.end();

	for ( ; pvsIt != end; ++pvsIt)
	{
		assert(pvsIt->first != observerID);

		PVSEntry& entry = pvsIt->second;
		QueueRay(observerID, observerInfo, entry);
	}
}

CVisionMap::PendingRayInfo* CVisionMap::QueueRay(const ObserverID& observerID, ObserverInfo& observerInfo, PVSEntry& entry)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	uint8 priorityClass = observerInfo.params.priority;

	if (!entry.rayPending)
	{
		PendingRayInfo *ray;
		{
			FRAME_PROFILER("QueueRayAllocation", GetISystem(), PROFILE_AI);

			ray = new (m_pendingRayInfoAlloc.Allocate()) PendingRayInfo();
			ray->owner = this;
			ray->entry = &entry;

			ray->observerID = observerID;
			ray->observerInfo = &observerInfo;

			ray->observableID = entry.observableInfo->id;
			ray->observableInfo = entry.observableInfo;

			ray->priorityClass = priorityClass;
			ray->priorityScale = entry.priorityScale;
			ray->age = 0.0f;

			assert(entry.priorityScale > 0.0f);
		}

		m_pendingRays.push_front(ray);
		entry.pendingRay = m_pendingRays.begin();
		entry.rayPending = true;

		return ray;
	}
	else
	{
		PendingRayInfo *pendingRay = *entry.pendingRay;

		if (pendingRay->priorityClass < priorityClass) // bump priorityClass if needed
			pendingRay->priorityClass = priorityClass;

		if (pendingRay->priorityScale < entry.priorityScale)
			pendingRay->priorityScale = entry.priorityScale;

		return pendingRay;
	}
}

void CVisionMap::DeletePendingRay(PVSEntry& entry)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (entry.rayPending)
	{
		PendingRayInfo* ray = *entry.pendingRay;
		m_pendingRays.erase(entry.pendingRay);

		ray->~PendingRayInfo();
		m_pendingRayInfoAlloc.Deallocate(ray);

		entry.rayPending = false;
	}
}

void CVisionMap::DeletePendingRays(PVS& pvs)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	PVS::iterator pvsIt = pvs.begin();
	PVS::iterator end = pvs.end();

	for ( ; pvsIt != end; ++pvsIt)
		DeletePendingRay(pvsIt->second);
}

bool CVisionMap::ComparePriority::operator ()(const PendingRayInfo* lhs, const PendingRayInfo* rhs) const
{
	return lhs->priority > rhs->priority;
}

void CVisionMap::UpdateRayPriorities(float frameTime)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	PendingRays::iterator rayIt = m_pendingRays.begin();
	PendingRays::iterator end = m_pendingRays.end();

	{
		FRAME_PROFILER("UpdateRayPriorities_UpdatePrio", GetISystem(), PROFILE_AI);

		for ( ; rayIt != end; ++rayIt)
		{
			PendingRayInfo *ray = *rayIt;
			PriorityClassInfo& priorityClass = m_priorityClasses[ray->priorityClass];

			ray->priority = priorityClass.basePriority*ray->priorityScale;
			ray->priority *= cry_powf(priorityClass.growthFactor, ray->age/priorityClass.growthTime);

			if (ray->age > m_maxLatency)
			{
				m_maxLatency = ray->age;
				m_maxLatencyQueueSize = m_pendingRays.size();
			}

			ray->age += frameTime;
		}
	}

	{
		FRAME_PROFILER("UpdateRayPriorities_Sort", GetISystem(), PROFILE_AI);

		m_pendingRays.sort(ComparePriority());
	}
}

void CVisionMap::ProcessRayQueue()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	static uint32 rayCountPerFrame = 4;	// TODO: make cvar

	while (!m_pendingRays.empty())
	{
		PendingRayInfo *ray = *m_pendingRays.begin();
		m_pendingRays.pop_front();

		PVSEntry& entry = *ray->entry;

		entry.rayPending = false;

		ObserverParams& observerParams = ray->observerInfo->params;
		ObservableParams& observableParams = ray->observableInfo->params;
		const Vec3& pos = observerParams.eyePos;

		Vec3 dir(observableParams.pos[entry.currentPos] - pos);
		Vec3 dirNorm = dir.normalized();

		IPhysicalEntity* pSkipList[2 * ObserverParams::MaxSkipListSize];
		memcpy(pSkipList, observerParams.skipList, observerParams.skipListSize * sizeof(IPhysicalEntity*));
		memcpy(pSkipList + observerParams.skipListSize, observableParams.skipList, observableParams.skipListSize * sizeof(IPhysicalEntity*));

		uint32 skipListSize = observerParams.skipListSize + observableParams.skipListSize;

		uint32 flags = HIT_COVER;	// TODO: Move this to the params struct
		{
			FRAME_PROFILER("ProcessRayQueue_CastRay", GetISystem(), PROFILE_AI);

			gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir, COVER_OBJECT_TYPES, flags|rwi_queue, 0, 1, pSkipList, skipListSize,
				(void *)ray, VISIBILITY_MAP_PHYS_FOREIGN_ID);
		}	

		if (++m_rayCastCount >= rayCountPerFrame)
			break;
	}
}


int CVisionMap::RWIResult(const EventPhys *pEvent)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const EventPhysRWIResult* pResult = static_cast<const EventPhysRWIResult*>(pEvent);
	if (pResult->iForeignData != VISIBILITY_MAP_PHYS_FOREIGN_ID)
		return 0;

	PendingRayInfo* ray = static_cast<PendingRayInfo*>(pResult->pForeignData);

	ObserverID observerID = ray->observerID;
	ObservableID observableID = ray->observableID;
	CVisionMap* owner = ray->owner;

	++owner->m_rayResultCount;

	{
		FRAME_PROFILER("RWIResultDeallocation", GetISystem(), PROFILE_AI);

		ray->~PendingRayInfo();
		owner->m_pendingRayInfoAlloc.Deallocate(ray);
	}

	Observers::iterator obsIt = owner->m_observers.find(observerID);
	if (obsIt == owner->m_observers.end())
		return 1; // Observer was removed after ray was cast

	PVS& pvs = obsIt->second.pvs;

	PVS::iterator pvsIt = pvs.find(observableID);
	if (pvsIt == pvs.end())
		return 1; // Observable was removed from pvs after ray was cast

	assert(owner->m_observables.find(observableID) != owner->m_observables.end());	// Consistency check

	PVSEntry& entry = pvsIt->second;

	// This might be setting an invalid result i.e. the object has changed since this ray was cast and a new ray is cast
	// but it should be ok, since most likely this result was valid in the previous frame
	bool visible = pResult->nHits == 0;

	ObserverParams& observerParams = obsIt->second.params;
	ObservableParams& observableParams = pvsIt->second.observableInfo->params;

	if (!visible)
	{
		if (++entry.currentPos < observableParams.posCount)
		{
			owner->QueueRay(observerID, obsIt->second, entry);

			return 1;
		}
	}

	entry.currentPos = 0;

	if (pvsIt->second.visible != visible)
	{
		if (observerParams.callback)
			observerParams.callback(observerID, observerParams, observableID, observableParams, visible);
		if (observableParams.callback)
			observableParams.callback(observerID, observerParams, observableID, observableParams, visible);
	}

	entry.visible = visible;

	return 1;
}


void CVisionMap::DebugDraw()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (gAIEnv.CVars.DebugDrawVisionMap > 1)
	{
		Observers::iterator it = m_observers.begin();
		Observers::iterator end = m_observers.end();

		for ( ; it != end; ++it)
		{
			ObserverID observerID = it->first;
			ObserverInfo& observerInfo = it->second;

			DebugDraw_ObserverPVS(observerID, observerInfo);
			DebugDraw_ObserverStats(observerID, observerInfo);
		}
	}

	DebugDrawStats();
}

void CVisionMap::DebugDrawStats()
{
	static float color[]={1.0f, 1.0f, 1.0f, 1.0f};
	
	float y=600.0f;

	gEnv->pRenderer->Draw2dLabel(750.0f, y,				1.1f, color, false, "ray queue size: %d", m_pendingRays.size());
	gEnv->pRenderer->Draw2dLabel(750.0f, y+=8.0f,	1.1f, color, false, "ray cast count: %d", m_rayCastCount);
	gEnv->pRenderer->Draw2dLabel(750.0f, y+=8.0f, 1.1f, color, false, "ray result count: %d", m_rayResultCount);
	gEnv->pRenderer->Draw2dLabel(750.0f, y+=8.0f,	1.1f, color, false, "pvs update count: %d", m_pvsUpdateCount);
	gEnv->pRenderer->Draw2dLabel(750.0f, y+=8.0f, 1.1f, color, false, "vis update count: %d", m_visUpdateCount);
	gEnv->pRenderer->Draw2dLabel(750.0f, y+=8.0f, 1.1f, color, false, "max latency: %.3f / %d", m_maxLatency, m_maxLatencyQueueSize);


	ObservablesSpace::Stats stats;

	m_observablesSpace.GetStats(&stats);

	gEnv->pRenderer->Draw2dLabel(750.0f, y+=10.0f,1.1f, color, false, "HashedSpace object count: %d", stats.objectCount);
	gEnv->pRenderer->Draw2dLabel(750.0f, y+=8.0f, 1.1f, color, false, "HashedSpace move count: %d", stats.moveCount);
	gEnv->pRenderer->Draw2dLabel(750.0f, y+=8.0f, 1.1f, color, false, "HashedSpace empty buckets: %d/%d", stats.emptyBuckets, stats.bucketCount);
	gEnv->pRenderer->Draw2dLabel(750.0f, y+=8.0f, 1.1f, color, false, "HashedSpace occupancy min/max: %d/%d", stats.minOccupancy, stats.maxOccupancy);
	gEnv->pRenderer->Draw2dLabel(750.0f, y+=8.0f, 1.1f, color, false, "HashedSpace average occupancy: %.1f", stats.averageOccupancy);
}

void CVisionMap::DebugDraw_ObserverPVS(const ObserverID& observerID, const ObserverInfo& observerInfo)
{
	Vec3 pos = observerInfo.params.eyePos;
	Vec3 dir;

	SAuxGeomRenderFlags flags;

	flags.SetAlphaBlendMode(e_AlphaBlended);
	flags.SetCullMode(e_CullModeNone);

	gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(flags);

	PVS::const_iterator it = observerInfo.pvs.begin();
	PVS::const_iterator end = observerInfo.pvs.end();
	for ( ; it != end; ++it)
	{
		dir = it->second.observableInfo->params.pos[it->second.currentPos] - pos;
		
		if (it->second.visible)
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(pos, ColorB(0xff00ff00u), pos+dir*0.5f, ColorB(0x7f00ff00u));
		else
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(pos, ColorB(0xff0000ffu), pos+dir*0.5f, ColorB(0x7f0000ffu));
	}
}

void CVisionMap::DebugDraw_ObserverStats(const ObserverID& observerID, const ObserverInfo& observerInfo)
{
	static float color[]={1.0f,1.0f,1.0f,1.0f};
	static float red[]={1.0f,0.0f,0.0f,1.0f};
	static float yellow[]={0.9f,0.9f,0.0f,1.0f};
	static float orange[]={1.0f,0.5f,0.0f,1.0f};
	static float green[]={0.0f,1.0f,0.0f,1.0f};

	Vec3 pos = observerInfo.params.eyePos;
	pos.z += 0.2f;

	PVS::const_iterator pvsIt = observerInfo.pvs.begin();
	PVS::const_iterator end = observerInfo.pvs.end();

	int visible = 0;
	for ( ; pvsIt != end; ++pvsIt)
	{
		if (pvsIt->second.visible)
			++visible;
	}

	const char *priority=0;
	float *prioColor;
	switch (observerInfo.params.priority)
	{
	case eLowPriority:
		priority="low";
		prioColor=green;
		break;
	case eMediumPriority:
		priority="medium";
		prioColor=yellow;
		break;
	case eHighPriority:
		priority="high";
		prioColor=orange;
		break;
	case eVeryHighPriority:
		priority="very high";
		prioColor=red;
		break;
	default:
		assert(false);
	}

	gEnv->pRenderer->DrawLabelEx(pos, 1.0f, color, true, true, "pvs visible: %d/%d", visible, observerInfo.pvs.size());
	pos.z+=0.35f;
	gEnv->pRenderer->DrawLabelEx(pos, 1.0f, prioColor, true, true, "priority: %s", priority);
	pos.z+=0.35f;
}