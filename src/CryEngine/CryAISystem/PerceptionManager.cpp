/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   PerceptionManager.cpp
$Id$
Description: 

-------------------------------------------------------------------------
History:
- 2008				: Created by Mikko Mononen
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/

#include "StdAfx.h"
#include "PerceptionManager.h"
#include "CAISystem.h"
#include "Puppet.h"
#include "AIPlayer.h"
#include "AIVehicle.h"
#include "DebugDrawContext.h"

// AI Stimulus names for debugging
static const char* g_szAIStimulusType[AISTIM_LAST] =
{
	"SOUND",
	"COLLISION",
	"EXPLOSION",
	"BULLET_WHIZZ",
	"BULLET_HIT",
	"GRENADE"
};
static const char* g_szAISoundStimType[AISOUND_LAST] =
{
	" GENERIC",
	" COLLISION",
	" COLLISION_LOUD",
	" MOVEMENT",
	" MOVEMENT_LOUD",
	" WEAPON",
	" EXPLOSION"
};
static const char* g_szAIGrenadeStimType[AIGRENADE_LAST] =
{
	" THROWN",
	" COLLISION",
	" FLASH_BANG",
	" SMOKE"
};

CPerceptionManager* CPerceptionManager::Instance = NULL;

//===================================================================
// CPerceptionManager
//===================================================================
CPerceptionManager::CPerceptionManager() :
	m_visBroadPhaseDt(0),
	m_visCheckRaycastCount(0)
{
	for (unsigned i = 0; i < AI_MAX_STIMULI; ++i)
	{
		m_stimuli[i].reserve(64);
		memset(&m_stimulusTypes[i], 0, sizeof(SAIStimulusTypeDesc));
	}
	InitCommonTypeDescs();

	m_incomingStimuli.reserve(64);

	assert(!Instance);
	Instance = this;
	gEnv->pPhysicalWorld->AddEventClient(
		EventPhysRWIResult::id,
		CPerceptionManager::OnRwiResult,
		1
	);
}

//===================================================================
// CPerceptionManager
//===================================================================
CPerceptionManager::~CPerceptionManager()
{
	Instance = NULL;
	gEnv->pPhysicalWorld->RemoveEventClient(
		EventPhysRWIResult::id,
		CPerceptionManager::OnRwiResult,
		1
	);
}

//===================================================================
// InitCommonTypeDescs
//===================================================================
void CPerceptionManager::InitCommonTypeDescs()
{

	SAIStimulusTypeDesc desc;

	// Sound
	memset(&desc, 0, sizeof(SAIStimulusTypeDesc));
	desc.SetName("Sound");
	desc.processDelay = 0.15f;
	desc.duration[AISOUND_GENERIC] = 2.0f;
	desc.duration[AISOUND_COLLISION] = 4.0f;
	desc.duration[AISOUND_COLLISION_LOUD] = 4.0f;
	desc.duration[AISOUND_MOVEMENT] = 2.0f;
	desc.duration[AISOUND_MOVEMENT_LOUD] = 4.0f;
	desc.duration[AISOUND_WEAPON] = 4.0f;
	desc.duration[AISOUND_EXPLOSION] = 6.0f;
	desc.filterTypes = 0;
	desc.nFilters = 0;
	RegisterStimulusDesc(AISTIM_SOUND, desc);

	// Collision
	memset(&desc, 0, sizeof(SAIStimulusTypeDesc));
	desc.SetName("Collision");
	desc.processDelay = 0.15f;
	desc.duration[AICOL_SMALL] = 7.0f;
	desc.duration[AICOL_MEDIUM] = 7.0f;
	desc.duration[AICOL_LARGE] = 7.0f;
	desc.filterTypes = (1<<AISTIM_COLLISION) | (1<<AISTIM_EXPLOSION);
	desc.nFilters = 2;
	desc.filters[0].Set(AISTIM_COLLISION, 0, AISTIMFILTER_MERGE_AND_DISCARD, 0.9f); // Merge nearby collisions
	desc.filters[1].Set(AISTIM_EXPLOSION, 0, AISTIMFILTER_DISCARD, 2.5f); // Discard collision near explosions
	RegisterStimulusDesc(AISTIM_COLLISION, desc);

	// Explosion
	memset(&desc, 0, sizeof(SAIStimulusTypeDesc));
	desc.SetName("Explosion");
	desc.processDelay = 0.15f;
	desc.duration[0] = 7.0f;
	desc.filterTypes = (1<<AISTIM_EXPLOSION);
	desc.nFilters = 1;
	desc.filters[0].Set(AISTIM_EXPLOSION, 0, AISTIMFILTER_MERGE_AND_DISCARD, 0.5f); // Merge nearby explosions
	RegisterStimulusDesc(AISTIM_EXPLOSION, desc);

	// Bullet Whizz
	memset(&desc, 0, sizeof(SAIStimulusTypeDesc));
	desc.SetName("BulletWhizz");
	desc.processDelay = 0.01f;
	desc.duration[0] = 0.5f;
	desc.filterTypes = 0;
	desc.nFilters = 0;
	RegisterStimulusDesc(AISTIM_BULLET_WHIZZ, desc);

	// Bullet Hit
	memset(&desc, 0, sizeof(SAIStimulusTypeDesc));
	desc.SetName("BulletHit");
	desc.processDelay = 0.15f;
	desc.duration[0] = 0.5f;
	desc.filterTypes = (1<<AISTIM_BULLET_HIT);
	desc.nFilters = 1;
	desc.filters[0].Set(AISTIM_BULLET_HIT, 0, AISTIMFILTER_MERGE_AND_DISCARD, 0.5f); // Merge nearby hits
	RegisterStimulusDesc(AISTIM_BULLET_HIT, desc);

	// Grenade
	memset(&desc, 0, sizeof(SAIStimulusTypeDesc));
	desc.SetName("Grenade");
	desc.processDelay = 0.15f;
	desc.duration[AIGRENADE_THROWN] = 6.0f;
	desc.duration[AIGRENADE_COLLISION] = 6.0f;
	desc.duration[AIGRENADE_FLASH_BANG] = 6.0f;
	desc.duration[AIGRENADE_SMOKE] = 6.0f;
	desc.filterTypes = (1<<AISTIM_GRENADE);
	desc.nFilters = 1;
	desc.filters[0].Set(AISTIM_GRENADE, AIGRENADE_COLLISION, AISTIMFILTER_MERGE_AND_DISCARD, 1.0f); // Merge nearby collisions
	RegisterStimulusDesc(AISTIM_GRENADE, desc);

}

bool CPerceptionManager::RegisterStimulusDesc(EAIStimulusType type, const SAIStimulusTypeDesc& desc)
{
	memcpy(&m_stimulusTypes[type], &desc, sizeof(SAIStimulusTypeDesc));
	return true;
}


//===================================================================
// Reset
//===================================================================
void CPerceptionManager::Reset()
{
/*	m_visChecks = 0;
	m_visChecksMax = 0;
	m_visChecksRays = 0;
	m_visChecksRaysMax = 0;
	m_visChecksHistoryHead = 0;
	m_visChecksHistorySize = 0;
	m_nRaysThisUpdateFrame = 0;
	m_maxStimsPerUpdate = 0;*/

	m_visBroadPhaseDt = 0;

	m_incomingStimuli.clear();
	for (unsigned i = 0; i < AI_MAX_STIMULI; ++i)
		m_stimuli[i].clear();

	InitCommonTypeDescs();
}

//===================================================================
// UpdatePerception
//===================================================================
bool CPerceptionManager::UpdatePerception(CPuppet* pPuppet, std::vector<CAIObject*>& priorityTargets)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	m_stats.trackers[PERFTRACK_UPDATES].Inc();

	std::vector<IPhysicalEntity*> skipList;

	EPuppetUpdatePriority	pup = pPuppet->GetUpdatePriority();

	if (pPuppet->IsEnabled() && pPuppet->IsPerceptionEnabled() && pPuppet->GetParameters().m_nSpecies >= 0)
	{
		if (!priorityTargets.empty())
		{
			FRAME_PROFILER("AIPlayerVisibilityCheck", gEnv->pSystem, PROFILE_AI);

			// Priority targets.
			for (unsigned i = 0, ni = priorityTargets.size(); i < ni; ++i)
			{
				CAIObject* pTarget = priorityTargets[i];
				if (!pPuppet->IsHostile(pTarget))
					continue;
				if (pPuppet->IsDevalued(pTarget))
					continue;
				if (!pPuppet->CanAcquireTarget(pTarget))
					continue;

				IAIObject::EFieldOfViewResult viewResult = pPuppet->IsObjectInFOV(pTarget);
				if (IAIObject::eFOV_Outside == viewResult)
					continue;

				CAIActor* pTargetActor = pTarget->CastToCAIActor();
				if (pTargetActor && pTargetActor->IsInvisibleFrom(pPuppet->GetPos()))
					continue;

				int los = -1;

				bool lowPriorityTarget = pTarget->GetType() == AIOBJECT_ATTRIBUTE;

				for (unsigned j = 0, nj = pPuppet->m_priorityTargets.size(); j < nj; ++j)
				{
					CAIActor::SProbableTarget& pri = pPuppet->m_priorityTargets[j];
					if (pri.pTarget == pTarget)
					{
						if(
							((pup == AIPUP_LOW || lowPriorityTarget) && pri.updates <= 2)
							||((pup == AIPUP_HIGH || pup == AIPUP_MED) && pri.updates <= 1)
							||(pri.updates <= 1 && pri.lineOfSight == 1)
						)
						{
							los = pri.lineOfSight;
						}
						break;
					}
				}

				m_stats.trackers[PERFTRACK_VIS_CHECKS].Inc();

				if (los == -1)
				{
					bool queued = false;
					PuppetLOSQueries::iterator puppetLOSQueries_iter = m_PuppetLOSQueries.find(PuppetToObjectLOS(pPuppet, pTarget));
					if(puppetLOSQueries_iter != m_PuppetLOSQueries.end())
					{
						if(puppetLOSQueries_iter->Status != PuppetToObjectLOS::QUEUED)
						{
							los = (puppetLOSQueries_iter->Status == PuppetToObjectLOS::IN_LOS) ? 1 : 0;
							pPuppet->RefreshPriorityTarget(pTarget, los);

							m_PuppetLOSQueries.erase(puppetLOSQueries_iter);
						}
						else
						{
							queued = true;
						}
					}

					if (!queued && !(m_visCheckRaycastCount > gAIEnv.CVars.AIVisRaysPerFrame) )
					{
						// Check visibility if uncertain line of sight.
						skipList.clear();
						pPuppet->GetPhysicsEntitiesToSkip(skipList);
						if (pTargetActor)
							pTargetActor->GetPhysicsEntitiesToSkip(skipList);

						const Vec3& puppetPos = pPuppet->GetPos();
						const Vec3& targetPos = pTarget->GetPos();

						bool skipSoftCover = false;
						// See grenades through vegetation.
						if (pTarget->GetType() == AIOBJECT_GRENADE || pTarget->GetType() == AIOBJECT_RPG)
							skipSoftCover = true;

						// See live and memory target through vegetation.
						if ((pPuppet->GetAttentionTargetType() == AITARGET_VISUAL ||
							pPuppet->GetAttentionTargetType() == AITARGET_MEMORY) &&
							pPuppet->GetAttentionTargetThreat() == AITHREAT_AGGRESSIVE)
						{
							const float distSq = Distance::Point_PointSq(puppetPos, targetPos);
							skipSoftCover = (distSq < sqr(pPuppet->GetParameters().m_fMeleeRange * 1.2f));
						}

						int flags = HIT_COVER;
						if (!skipSoftCover)
							flags |= HIT_SOFT_COVER;

						// Occlusions planes
						int rwi = RayOcclusionPlaneIntersection(puppetPos, targetPos);

						// World
						if (!rwi)
						{
							//ray_hit hit;
							rwi = gEnv->pPhysicalWorld->RayWorldIntersection(
								targetPos
								,puppetPos - targetPos
								,COVER_OBJECT_TYPES
								,flags | rwi_queue
								,NULL
								,1
								,skipList.empty() ? 0 : &skipList[0]
								,skipList.size()
								,reinterpret_cast<void*>(pTarget)
								,reinterpret_cast<int>(pPuppet)
							);
							m_PuppetLOSQueries.insert(PuppetToObjectLOS(pPuppet, pTarget));

							m_stats.trackers[PERFTRACK_VIS_RAYS].Inc();
							m_visCheckRaycastCount++;
						}

						queued = true;
					}

					if (queued)
					{
						// Not allowed to raycast, reuse last result.
						for (unsigned j = 0, nj = pPuppet->m_priorityTargets.size(); j < nj; ++j)
						{
							CAIActor::SProbableTarget& pri = pPuppet->m_priorityTargets[j];
							if (pri.pTarget == pTarget)
							{
								los = pri.lineOfSight;
								break;
							}
						}
					}
				}

				if (los == 1)
				{
					if (pTarget)
					{
						// Notify visual perception.
						SAIEVENT event;
						event.sourceId = pTarget->GetEntityID();
						event.bFuzzySight = (viewResult == IAIObject::eFOV_Secondary);
						event.vPosition = pTarget->GetPos();
						pPuppet->Event(AIEVENT_ONVISUALSTIMULUS, &event);
					}
				}

				if (pTarget->IsEnabled() && ((pTarget->GetType()==AIOBJECT_PLAYER) || (pTarget->GetType()==AIOBJECT_PUPPET) ))
				{
					if (pPuppet->GetAttentionTarget() == pTarget)
					{
						const float distSq = Distance::Point_PointSq(pPuppet->GetPos(), pTarget->GetPos());
						if (distSq < sqr(pPuppet->GetParameters().m_fMeleeRange))
						{
							if (pPuppet->CloseContactEnabled())
							{
								pPuppet->SetSignal(1, "OnCloseContact", pTarget->GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnCloseContact);		
								pPuppet->SetCloseContact(true);
							}
						}
					}
				}
			}
			//
			pPuppet->UpdatePriorityTargets();
		}


		{
			FRAME_PROFILER("ProbableTargets", gEnv->pSystem, PROFILE_AI);
			// Probable targets.
			for (unsigned i = 0, ni = pPuppet->m_probableTargets.size(); i < ni; ++i)
			{
				CAIActor::SProbableTarget& prob = pPuppet->m_probableTargets[i];

				CPuppet* pTargetPuppet = prob.pTarget->CastToCPuppet();
				if (pTargetPuppet)
				{
					if (pTargetPuppet->m_Parameters.m_bInvisible || pPuppet->IsDevalued(prob.pTarget))
						continue;
				}

				if (!pPuppet->CanAcquireTarget(prob.pTarget))
					continue;
				if (!pPuppet->IsHostile(prob.pTarget))
					continue;

				IAIObject::EFieldOfViewResult viewResult = pPuppet->IsObjectInFOV(prob.pTarget);
				if (IAIObject::eFOV_Outside == viewResult)
					continue;

				int los = -1;
				if (prob.updates <= 3)
					los = prob.lineOfSight;
				if (pTargetPuppet && (los == -1))
					los = pTargetPuppet->HasLineOfSightTo(pPuppet);

				m_stats.trackers[PERFTRACK_VIS_CHECKS].Inc();

				if (los == -1)
				{
					bool queued = false;
					PuppetLOSQueries::iterator puppetLOSQueries_iter = m_PuppetLOSQueries.find(PuppetToObjectLOS(pPuppet, pTargetPuppet));
					if(puppetLOSQueries_iter != m_PuppetLOSQueries.end())
					{
						if(puppetLOSQueries_iter->Status != PuppetToObjectLOS::QUEUED)
						{
							prob.lineOfSight = (puppetLOSQueries_iter->Status == PuppetToObjectLOS::IN_LOS) ? 1 : 0;
							prob.updates = 0;
							los = prob.lineOfSight;

							m_PuppetLOSQueries.erase(puppetLOSQueries_iter);
						}
						else
						{
							queued = true;
						}
					}
					
					if (!queued && !(m_visCheckRaycastCount > gAIEnv.CVars.AIVisRaysPerFrame) )
					{
						// Check visibility if uncertain line of sight.

						const Vec3& puppetPos = pPuppet->GetPos();
						const Vec3& targetPos = prob.pTarget->GetPos();

						// Occlusions planes
						int rwi = RayOcclusionPlaneIntersection(puppetPos, targetPos);

						// World
						if (!rwi)
						{
							//ray_hit hit;

							skipList.clear();
							pPuppet->GetPhysicsEntitiesToSkip(skipList);

							if (pTargetPuppet)
								pTargetPuppet->GetPhysicsEntitiesToSkip(skipList);
							else
							{
								if (IPhysicalEntity* pPhysics = prob.pTarget->GetPhysics())
									skipList.push_back(pPhysics);
							}

							rwi = gEnv->pPhysicalWorld->RayWorldIntersection(
								puppetPos
								,targetPos - puppetPos
								,COVER_OBJECT_TYPES
								,HIT_COVER|HIT_SOFT_COVER|rwi_queue
								,NULL
								,1
								,skipList.empty() ? 0 : &skipList[0]
								,skipList.size()
								,reinterpret_cast<void*>( static_cast<CAIObject*>(pTargetPuppet) )
								,reinterpret_cast<int>(pPuppet)
							);
							m_PuppetLOSQueries.insert(PuppetToObjectLOS(pPuppet, pTargetPuppet));

							m_stats.trackers[PERFTRACK_VIS_RAYS].Inc();
							m_visCheckRaycastCount++;
						}
						queued = true;
					}

					if (queued)
					{
						los = prob.lineOfSight;

						if (pTargetPuppet)
						{
							for (uint32 j = 0, ni2 = pTargetPuppet->m_probableTargets.size(); j < ni2; ++j)
							{
								const CAIActor::SProbableTarget& p = pTargetPuppet->m_probableTargets[j];
								if (p.pTarget == pPuppet)
								{
									break;
									if (!prob.dirty && p.updates < prob.updates)
										los = p.lineOfSight;
								}
							}
						}
					}
				}

				if (los == 1)
				{
					if (prob.pTarget)
					{
						SAIEVENT event;
						event.sourceId = prob.pTarget->GetEntityID();
						event.bFuzzySight = (viewResult == IAIObject::eFOV_Secondary);
						event.fThreat = 1.f;
						event.vPosition = prob.pTarget->GetPos();
						pPuppet->Event(AIEVENT_ONVISUALSTIMULUS, &event);

						const float distSq = Distance::Point_PointSq(pPuppet->GetPos(), prob.pTarget->GetPos());
						pPuppet->CheckCloseContact(prob.pTarget, distSq);
					}
				}
			}

			pPuppet->UpdateProbableTargets();
		}
	}

	return false;
}

void CPerceptionManager::RemovePuppetLOSQueries(CPuppet* pPuppet)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	assert(pPuppet);

	if (pPuppet)
	{
		PuppetLOSQueries::iterator itPuppetLOSQuery = m_PuppetLOSQueries.begin();
		while (itPuppetLOSQuery != m_PuppetLOSQueries.end())
		{
			const PuppetToObjectLOS &puppetToObjectLOS = *itPuppetLOSQuery;
			if (puppetToObjectLOS.puppet == pPuppet)
			{
				PuppetLOSQueries::iterator erased = itPuppetLOSQuery++;
				m_PuppetLOSQueries.erase(erased);
				continue;
			}

			++itPuppetLOSQuery;
		}
	}
}

bool CPerceptionManager::FilterStimulus(SAIStimulus* stim)
{
	const SAIStimulusTypeDesc* desc = &m_stimulusTypes[stim->type];
	const SAIStimulusFilter* filters[AI_MAX_FILTERS];

	// Merge and filter with current active stimuli.
	for (unsigned int i = 0; i < AI_MAX_STIMULI; ++i)
	{
		unsigned int mask = (1 << i);
		if ((mask & desc->filterTypes) == 0) continue;

		// Collect filters for this stimuli type
		unsigned int nFilters = 0;
		for (unsigned j = 0; j < desc->nFilters; ++j)
		{
			const SAIStimulusFilter* filter = &desc->filters[j];
			if ((unsigned int)filter->type != stim->type) continue;
			if (filter->subType && (unsigned int)filter->subType != stim->subType) continue;
			filters[nFilters++] = filter;
		}
		if (nFilters == 0)
			continue;

		std::vector<SStimulusRecord>& stimuli = m_stimuli[i];
		for (unsigned j = 0, nj = stimuli.size(); j < nj; ++j)
		{
			SStimulusRecord& s = stimuli[j];
			Vec3 diff = stim->pos - s.pos;
			float distSq = diff.GetLengthSquared();

			// Apply filters
			for (unsigned int k = 0; k < nFilters; ++k)
			{
				const SAIStimulusFilter* filter = filters[k];
				if (filter->subType && (filter->subType & (1<<s.subType))) continue;

				if (filter->merge == AISTIMFILTER_MERGE_AND_DISCARD)
				{
					// Merge stimuli
					// Allow to merge stimuli before they are processed.
					const float duration = desc->duration[stim->subType];
					if (distSq < sqr(s.radius * filter->scale))
					{
						if ((duration - s.t) < desc->processDelay)
						{
							float dist = sqrtf(distSq);
							// Merge spheres into a larger one.
							if (dist > 0.00001f)
							{
								diff *= 1.0f/dist;
								// Calc new radius
								float r = s.radius;
								s.radius = (dist + r + stim->radius)/2;
								// Calc new location
								s.pos += diff * (s.radius - r);
							}
							else
							{
								// Spheres are at same location, merge radii.
								s.radius = max(s.radius, stim->radius);
							}
						}
						return true;
					}
				}
				else
				{
					// Discard stimuli
					if (distSq < sqr(s.radius * filter->scale))
						return true;
				}
			}
		}

	}

	// Not filtered nor merged, should create new stimulus.
	return false;
}

//===================================================================
// Update
//===================================================================
void CPerceptionManager::Update(float dt)
{

	// Update stats about the incoming stimulus.
	m_stats.trackers[PERFTRACK_INCOMING_STIMS].Inc(m_incomingStimuli.size());

	// Process incoming stimuli
	bool previousDiscarded = false;
	for (unsigned i = 0, ni = m_incomingStimuli.size(); i < ni; ++i)
	{
		SAIStimulus& is = m_incomingStimuli[i];
		// Check if stimulus should be discarded because it is set linked to the discard rules
		// of the previous stimulus.
		bool discardWithPrevious = previousDiscarded && (is.flags & AISTIMPROC_FILTER_LINK_WITH_PREVIOUS);
		if (!discardWithPrevious && !FilterStimulus(&is))
		{
			const SAIStimulusTypeDesc* desc = &m_stimulusTypes[is.type];
			std::vector<SStimulusRecord>& stimuli = m_stimuli[is.type];

			// Create new Stimulus
			stimuli.resize(stimuli.size()+1);
			SStimulusRecord& stim = stimuli.back();
			stim.sourceId = is.sourceId;
			stim.targetId = is.targetId;
			stim.pos = is.pos;
			stim.dir = is.dir;
			stim.radius = is.radius;
			stim.t = desc->duration[is.subType];
			stim.type = is.type;
			stim.subType = is.subType;
			stim.flags = is.flags;
			stim.dispatched = 0;
			
			previousDiscarded = false;
		}
		else
		{
			previousDiscarded = true;
		}
	}
	m_incomingStimuli.clear();

	// Update stimuli
	// Merge and filter with current active stimuli.
	for (unsigned int i = 0; i < AI_MAX_STIMULI; ++i)
	{
		std::vector<SStimulusRecord>& stims = m_stimuli[i];
		for (unsigned j = 0; j < stims.size(); )
		{
			SStimulusRecord& stim = stims[j];
			const SAIStimulusTypeDesc* desc = &m_stimulusTypes[stim.type];

			// Dispatch
			if (!stim.dispatched && stim.t < (desc->duration[stim.subType] - desc->processDelay))
			{
				float threat = 1.0f;
				switch (stim.type)
				{
				case AISTIM_SOUND:
					HandleSound(stim);
					switch (stim.subType)
					{
					case AISOUND_GENERIC: threat = 0.3f; break;
					case AISOUND_COLLISION: threat = 0.3f; break;
					case AISOUND_COLLISION_LOUD: threat = 0.3f; break;
					case AISOUND_MOVEMENT: threat = 0.5f; break;
					case AISOUND_MOVEMENT_LOUD: threat = 0.5f; break;
					case AISOUND_WEAPON: threat = 1.0f; break;
					case AISOUND_EXPLOSION: threat = 1.0f; break;
					};
					break;
				case AISTIM_COLLISION:
					HandleCollision(stim);
					threat = 0.2f;
					break;
				case AISTIM_EXPLOSION:
					HandleExplosion(stim);
					threat = 1.0f;
					break;
				case AISTIM_BULLET_WHIZZ:
					HandleBulletWhizz(stim);
					threat = 0.7f;
					break;
				case AISTIM_BULLET_HIT:
					HandleBulletHit(stim);
					threat = 1.0f;
					break;
				case AISTIM_GRENADE:
					HandleGrenade(stim);
					threat = 0.7f;
					break;
				default:
					// Invalid type
					AIAssert(0);
					break;
				}

				NotifyAIEventListeners(stim, threat);

				stim.dispatched = 1;
			}

			// Update stimulus time.
			stim.t -= dt;
			if (stim.t < 0.0f)
			{
				// The stimuli has timed out, delete it.
				memcpy(&stims[j], &stims.back(), sizeof(SStimulusRecord));
				stims.pop_back();
			}
			else
			{
				// Advance
				++j;
			}
		}

		// Update stats about the stimulus.
		m_stats.trackers[PERFTRACK_STIMS].Inc(stims.size());

		// Update the ignores.
		if (!m_ignoreStimuliFrom[i].empty())
		{
			StimulusIgnoreMap::iterator it = m_ignoreStimuliFrom[i].begin();
			while (it != m_ignoreStimuliFrom[i].end())
			{
				it->second -= dt;
				if (it->second <= 0.0f)
				{
					StimulusIgnoreMap::iterator del = it;
					++it;
					m_ignoreStimuliFrom[i].erase(del);
				}
				else
					++it;
			}
		}
	}

	VisCheckBroadPhase(dt);

	// Update stats
	m_stats.Update();

	m_visCheckRaycastCount = 0;
}

//===================================================================
// VisCheckBroadPhase
//===================================================================
void CPerceptionManager::VisCheckBroadPhase(float dt)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI );

	const float UPDATE_DELTA_TIME = 0.2f;

	m_visBroadPhaseDt += dt;
	if (m_visBroadPhaseDt < UPDATE_DELTA_TIME)
		return;
	m_visBroadPhaseDt -= UPDATE_DELTA_TIME;
	if (m_visBroadPhaseDt > UPDATE_DELTA_TIME) m_visBroadPhaseDt = 0.0f;

	const CAISystem::PuppetSet& enabledPuppetsSet = GetAISystem()->GetEnabledPuppetSet();

	if (enabledPuppetsSet.empty())
		return;

	// Find player vehicles (driver inside but disabled).
	static std::vector<CAIVehicle*>	playerVehicles;
	playerVehicles.resize(0);

	const CAISystem::AIObjectOwners::iterator vehicleBegin = GetAISystem()->m_Objects.find(AIOBJECT_VEHICLE);
	const CAISystem::AIObjectOwners::iterator aiEnd = GetAISystem()->m_Objects.end();
	for (CAISystem::AIObjectOwners::iterator ai = vehicleBegin; ai != aiEnd && ai->first == AIOBJECT_VEHICLE ; ++ai)
	{
		CAIVehicle* pVehicle = (CAIVehicle*)ai->second.GetAIObject();
		if (!pVehicle->IsEnabled() && pVehicle->IsDriverInside())
			playerVehicles.push_back(pVehicle);
	}

	// Find target entities
	static std::vector<CAIObject*>	targetEntities;
	targetEntities.resize(0);

	const CAISystem::AIObjectOwners::iterator targetsIt = GetAISystem()->m_Objects.find(AIOBJECT_TARGET);
	const CAISystem::AIObjectOwners::iterator targetsEnd = GetAISystem()->m_Objects.end();

	for (CAISystem::AIObjectOwners::iterator ai = targetsIt; ai != targetsEnd && ai->first == AIOBJECT_TARGET ; ++ai)
	{
		CAIObject* pTarget = ai->second.GetAIObject();
		if (pTarget->IsEnabled())
			targetEntities.push_back(pTarget);
	}

	// Clear target lists.
	for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
	{
		CPuppet *pPuppet = it->GetAIObject();
		if (pPuppet)
			pPuppet->ClearProbableTargets();
	}

	// Find potential targets.
	for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
	{
		CPuppet* pFirst = it->GetAIObject();
		if (!pFirst)
			continue;

		const Vec3& firstPos = pFirst->GetPos();

		// Check against other AIs
		CAISystem::PuppetSet::const_iterator it2 = it;
		++it2;
		for ( ; it2 != itend; ++it2)
		{
			CPuppet* pSecond = it2->GetAIObject();
			if (!pSecond)
				continue;

			// Skip non-hostile.
			if (!pFirst->IsHostile(pSecond))
				continue;

			const Vec3& secondPos = pSecond->GetPos();

			float distSq = Distance::Point_PointSq(firstPos, secondPos);
			if (distSq < sqr(pFirst->GetMaxTargetVisibleRange(pSecond)))
				pFirst->AddProbableTarget(pSecond);
			if (distSq < sqr(pSecond->GetMaxTargetVisibleRange(pFirst)))
				pSecond->AddProbableTarget(pFirst);
		}

		// Check against player vehicles.
		for (unsigned j = 0, nj = playerVehicles.size(); j < nj; ++j)
		{
			CAIVehicle* pSecond = playerVehicles[j];

			// Skip non-hostile.
			if (!pFirst->IsHostile(pSecond))
				continue;

			const Vec3& secondPos = pSecond->GetPos();

			float distSq = Distance::Point_PointSq(firstPos, secondPos);
			if (distSq < sqr(pFirst->GetMaxTargetVisibleRange(pSecond)))
				pFirst->AddProbableTarget(pSecond);
		}

		// Check against target entities.
		for (unsigned j = 0, nj = targetEntities.size(); j < nj; ++j)
		{
			CAIObject* pSecond = targetEntities[j];

			// Skip non-hostile.
			if (!pFirst->IsHostile(pSecond))
				continue;

			const Vec3& secondPos = pSecond->GetPos();

			float distSq = Distance::Point_PointSq(firstPos, secondPos);
			if (distSq < sqr(pFirst->GetMaxTargetVisibleRange(pSecond)))
				pFirst->AddProbableTarget(pSecond);
		}
	}
}


inline int bit(unsigned b, unsigned x)
{
	return (x >> b) & 1;
}

inline ColorB GetColorFromId(unsigned x)
{
	unsigned r = (bit(0,x)<<7) | (bit(4,x)<<5) | (bit(7,x)<<3);
	unsigned g = (bit(1,x)<<7) | (bit(5,x)<<5) | (bit(8,x)<<3);
	unsigned b = (bit(2,x)<<7) | (bit(6,x)<<5) | (bit(9,x)<<3);
	return ColorB(255-r, 255-g, 255-b);
}


//===================================================================
// DebugDraw
//===================================================================
void CPerceptionManager::DebugDraw(int mode)
{
	CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetAISystem()->GetPlayer());

	typedef std::map<unsigned, unsigned> OccMap;
	OccMap occupied;

	CDebugDrawContext dc;

	for (unsigned i = 0; i < AI_MAX_STIMULI; ++i)
	{
		switch(i)
		{
		case AISTIM_SOUND:
			if (gAIEnv.CVars.DrawSoundEvents == 0) continue;
			break;
		case AISTIM_COLLISION:
			if (gAIEnv.CVars.DrawCollisionEvents == 0) continue;
			break;
		case AISTIM_EXPLOSION:
			if (gAIEnv.CVars.DrawExplosions == 0) continue;
			break;
		case AISTIM_BULLET_WHIZZ:
		case AISTIM_BULLET_HIT:
			if (gAIEnv.CVars.DrawBulletEvents == 0) continue;
			break;
		case AISTIM_GRENADE:
			if (gAIEnv.CVars.DrawGrenadeEvents == 0) continue;
			break;
		};

		const SAIStimulusTypeDesc* desc = &m_stimulusTypes[i];
		std::vector<SStimulusRecord>& stimuli = m_stimuli[i];
		for (unsigned j = 0, nj = stimuli.size(); j < nj; ++j)
		{
			SStimulusRecord& s = stimuli[j];

			assert(s.subType < AI_MAX_SUBTYPES);
			float tmax = desc->duration[s.subType];
			float a = clamp((s.t-tmax/2)/(tmax/2), 0.0f, 1.0f);

			int row = 0;
			unsigned hash = HashFromVec3(s.pos, 0.1f);
			OccMap::iterator it = occupied.find(hash);
			if (it != occupied.end())
			{
				row = it->second;
				it->second++;
			}
			else
			{
				occupied[hash] = 1;
			}
			if (row > 5) row = 5;

			bool thrownByPlayer = pPlayer ? pPlayer->IsThrownByPlayer(s.sourceId) : false;

			ColorB color = thrownByPlayer ? ColorB(240, 16, 0) : GetColorFromId(i);
			ColorB colorTrans(color);
			color.a = 10 + (uint8)(240*a);
			colorTrans.a = 10 + (uint8)(128*a);

			char rowTxt[] = "\n\n\n\n\n\n\n";
			rowTxt[row] = '\0';

			char subTypeTxt[128] = "";
			switch(s.type)
			{
			case AISTIM_SOUND:
				switch(s.subType)
				{
				case AISOUND_GENERIC: _snprintf(subTypeTxt, 128, "GENERIC  R=%.1f", s.radius); break;
				case AISOUND_COLLISION: _snprintf(subTypeTxt, 128, "COLLISION  R=%.1f", s.radius); break;
				case AISOUND_COLLISION_LOUD: _snprintf(subTypeTxt, 128, "COLLISION LOUD  R=%.1f", s.radius); break;
				case AISOUND_MOVEMENT: _snprintf(subTypeTxt, 128, "MOVEMENT  R=%.1f", s.radius); break;
				case AISOUND_MOVEMENT_LOUD: _snprintf(subTypeTxt, 128, "MOVEMENT LOUD  R=%.1f", s.radius); break;
				case AISOUND_WEAPON: _snprintf(subTypeTxt, 128, "WEAPON\nR=%.1f", s.radius); break;
				case AISOUND_EXPLOSION: _snprintf(subTypeTxt, 128, "EXPLOSION  R=%.1f", s.radius); break;
				}
				break;
			case AISTIM_COLLISION:
				switch(s.subType)
				{
				case AICOL_SMALL: _snprintf(subTypeTxt, 128, "SMALL  R=%.1f", s.radius); break;
				case AICOL_MEDIUM: _snprintf(subTypeTxt, 128, "MEDIUM  R=%.1f", s.radius); break;
				case AICOL_LARGE:  _snprintf(subTypeTxt, 128, "LARGE  R=%.1f", s.radius); break;
				};
				break;
			case AISTIM_EXPLOSION:
				_snprintf(subTypeTxt, 128, "R=%.1f", s.radius);
				break;
			case AISTIM_BULLET_WHIZZ:
				_snprintf(subTypeTxt, 128, "R=%.1f", s.radius);
				break;
			case AISTIM_BULLET_HIT:
				_snprintf(subTypeTxt, 128, "R=%.1f", s.radius);
				break;
			case AISTIM_GRENADE:
				_snprintf(subTypeTxt, 128, "R=%.1f", s.radius);
				break;
			};

			Vec3 ext(0,0,s.radius);
			if (s.dir.GetLengthSquared() > 0.1f)
				ext = s.dir * s.radius;

			dc->DrawSphere(s.pos, 0.15f, colorTrans);
			dc->DrawLine(s.pos, color, s.pos + ext, color);
			dc->DrawWireSphere(s.pos, s.radius, color);
			dc->Draw3dLabel(s.pos, 1.1f, "%s%s  %s", rowTxt, desc->name, subTypeTxt);
		}
	}

}

//===================================================================
// DebugDrawPerformance
//===================================================================
void CPerceptionManager::DebugDrawPerformance(int mode)
{
	CDebugDrawContext dc;
	ColorB	white(255, 255, 255);
	
	static std::vector<Vec3> points;

	if (mode == 1)
	{
		// Draw visibility performance

		const int visChecks = m_stats.trackers[PERFTRACK_VIS_CHECKS].GetCount();
		const int visChecksMax = m_stats.trackers[PERFTRACK_VIS_CHECKS].GetCountMax();
		const int visRays = m_stats.trackers[PERFTRACK_VIS_RAYS].GetCount();
		const int visRaysMax = m_stats.trackers[PERFTRACK_VIS_RAYS].GetCountMax();
		const int updates = m_stats.trackers[PERFTRACK_UPDATES].GetCount();
		const int updatesMax = m_stats.trackers[PERFTRACK_UPDATES].GetCountMax();

		dc->Draw2dLabel(50, 200, 2, white, false, "Updates:");
		dc->Draw2dLabel(175, 200, 2, white, false, "%d", updates);
		dc->Draw2dLabel(215, 200+5, 1, white, false, "max:%d", updatesMax);

		dc->Draw2dLabel(50, 225, 2, white, false, "Vis checks:");
		dc->Draw2dLabel(175, 225, 2, white, false, "%d", visChecks);
		dc->Draw2dLabel(215, 225+5, 1, white, false, "max:%d", visChecksMax);

		dc->Draw2dLabel(50, 250, 2, white, false, "Rays:");
		dc->Draw2dLabel(175, 250, 2, white, false, "%d", visRays);
		dc->Draw2dLabel(215, 250+5, 1, white, false, "max:%d", visRaysMax);

		dc->Draw2dLabel(50, 275, 2, white, false, "Ratio:");
		if (visChecks)
			dc->Draw2dLabel(175, 275, 2, white, false, "%.1f", ((float)visRays / (float)visChecks) * 100.0f);
		if (visChecksMax)
			dc->Draw2dLabel(215, 275+5, 1, white, false, "max:%.1f", ((float)visRaysMax / (float)visChecksMax) * 100.0f);


		{
			dc->Init2DMode();
			dc->SetAlphaBlended(true);
			dc->SetBackFaceCulling(false);
			dc->SetDepthTest(false);

			float	rw = (float) dc->GetWidth();
			float	rh = (float) dc->GetHeight();
//		float	as = rh/rw;
		float scale = 1.0f / rh;

		// Divider lines every 10 units.
		for (unsigned i = 0; i <= 10; ++i)
		{
			int v = i * 10;
				dc->DrawLine(Vec3(0.1f, 0.9f - v * scale, 0), ColorB(255, 255, 255, 128), Vec3(0.9f, 0.9f - v * scale, 0), ColorB(255, 255, 255, 128));
				dc->Draw2dLabel(0.1f * rw - 20, rh * 0.9f - v * scale * rh - 6, 1.0f, white, false, "%d", i * 10);
		}

		// Max rays per frame
		{
				ColorB cyan(0, 192, 255);
			int v = gAIEnv.CVars.AIVisRaysPerFrame;
				dc->DrawLine(Vec3(0.1f, 0.9f - v * scale, 0), ColorB(0, 196, 255, 128), Vec3(0.9f, 0.9f - v * scale, 0), ColorB(0, 196, 255, 128));
				dc->Draw2dLabel(0.9f * rw + 2, rh * 0.9f - v * scale * rh - 7, 1.2f, cyan, false, "Max Rays %d", v);
		}

		int idx[3] = {PERFTRACK_UPDATES, PERFTRACK_VIS_RAYS, PERFTRACK_VIS_CHECKS};
		ColorB colors[3] = { ColorB(255,0,0), ColorB(0,196,255), ColorB(255,255,255) };

		for (int i = 0; i < 3; ++i)
		{
			const CValueHistory<int>& hist = m_stats.trackers[idx[i]].GetHist();
			unsigned n = hist.GetSampleCount();
			if (!n) continue;

			points.resize(n);

			for (unsigned j = 0; j < n; ++j)
			{
				float t = (float)j / (float)hist.GetMaxSampleCount();
				points[j].x = 0.1f + t*0.8f;
				points[j].y = 0.9f - hist.GetSample(j)*scale;
				points[j].z = 0;
			}
				dc->DrawPolyline(&points[0], n, false, colors[i]);
			}
		}

		const CAISystem::PuppetSet& enabledPuppetsSet = GetAISystem()->GetEnabledPuppetSet();
		for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
		{
			CPuppet* pPuppet = it->GetAIObject();
			if (!pPuppet->IsDryUpdate())
				dc->DrawSphere(pPuppet->GetPos(), 1.0f, ColorB(255, 0, 0));
		}
	}
	else if (mode == 2)
	{
		// Draw stims performance

		const int incoming = m_stats.trackers[PERFTRACK_INCOMING_STIMS].GetCount();
		const int incomingMax = m_stats.trackers[PERFTRACK_INCOMING_STIMS].GetCountMax();
		const int stims = m_stats.trackers[PERFTRACK_STIMS].GetCount();
		const int stimsMax = m_stats.trackers[PERFTRACK_STIMS].GetCountMax();

		dc->Draw2dLabel(50, 200, 2, white, false, "Incoming:");
		dc->Draw2dLabel(175, 200, 2, white, false, "%d", incoming);
		dc->Draw2dLabel(215, 200+5, 1, white, false, "max:%d", incomingMax);

		dc->Draw2dLabel(50, 225, 2, white, false, "Stims:");
		dc->Draw2dLabel(175, 225, 2, white, false, "%d", stims);
		dc->Draw2dLabel(215, 225+5, 1, white, false, "max:%d", stimsMax);

		dc->Init2DMode();
		dc->SetAlphaBlended(true);
		dc->SetBackFaceCulling(false);
		dc->SetDepthTest(false);

		float	rw = (float) dc->GetWidth();
		float	rh = (float) dc->GetHeight();
//		float	as = rh/rw;
		float scale = 1.0f / rh;

		// Divider lines every 10 units.
		for (unsigned i = 0; i <= 10; ++i)
		{
			int v = i * 10;
			dc->DrawLine(Vec3(0.1f, 0.9f - v * scale, 0), ColorB(255, 255, 255, 128), Vec3(0.9f, 0.9f - v * scale, 0), ColorB(255, 255, 255, 128));
			dc->Draw2dLabel(0.1f * rw - 20, rh * 0.9f - v * scale * rh - 6, 1.0f, white, false, "%d", i * 10);
		}

		int idx[2] = {PERFTRACK_INCOMING_STIMS, PERFTRACK_STIMS};
		ColorB colors[2] = { ColorB(0,196,255), ColorB(255,255,255) };

		for (int i = 0; i < 2; ++i)
		{
			const CValueHistory<int>& hist = m_stats.trackers[idx[i]].GetHist();
			unsigned n = hist.GetSampleCount();
			if (!n) continue;

			points.resize(n);

			for (unsigned j = 0; j < n; ++j)
			{
				float t = (float)j / (float)hist.GetMaxSampleCount();
				points[j].x = 0.1f + t*0.8f;
				points[j].y = 0.9f - hist.GetSample(j)*scale;
				points[j].z = 0;
			}
			dc->DrawPolyline(&points[0], n, false, colors[i]);
		}
	}
}


void CPerceptionManager::RegisterStimulus(const SAIStimulus& stim)
{
	// Check if we should ignore stimulus from the source.
	StimulusIgnoreMap& ignore = m_ignoreStimuliFrom[stim.type];
	if (ignore.find(stim.sourceId) != ignore.end())
		return;

	if (stim.sourceId == 0)
	{
		gEnv->pLog->LogWarning("%s(%d) - CPerceptionManager::RegisterStimulus: Source ID = 0", __FILE__, __LINE__);
		return;
	}

//	const SAIStimulusTypeDesc* desc = &m_stimulusTypes[stim.type];
	m_incomingStimuli.resize(m_incomingStimuli.size()+1);
	SAIStimulus& is = m_incomingStimuli.back();
	memcpy(&is, &stim, sizeof(SAIStimulus));

//	m_maxIncomingPerUpdate = max(m_maxIncomingPerUpdate, (int)m_incomingStimuli.size());
}

//===================================================================
// IgnoreStimulusFrom
//===================================================================
void CPerceptionManager::IgnoreStimulusFrom(EntityId sourceId, EAIStimulusType type, float time)
{
	StimulusIgnoreMap& ignore = m_ignoreStimuliFrom[(int)type];
	StimulusIgnoreMap::iterator it = ignore.find(sourceId);
	if (it != ignore.end())
		it->second = max(it->second, time);
	else
		ignore[sourceId] = time;
}


//===================================================================
// HandleSound
//===================================================================
void CPerceptionManager::HandleSound(const SStimulusRecord& stim)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	if (gAIEnv.CVars.SoundPerception && !gAIEnv.CVars.SoundPerception)
		return;	// sound perception of enemies is off

	IEntity* pSenderEnt = gEnv->pEntitySystem->GetEntity(stim.sourceId);
	CAIActor* pSenderActor = 0;
	if (pSenderEnt)
		 pSenderActor = CastToCAIActorSafe(pSenderEnt->GetAI());

	// perform suppression of the sound from any nearby suppressors
	float rad = SupressSound(stim.pos, stim.radius);

	// If the sound events comes from an object thrown by the player, increase the radius.
	if (CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetAISystem()->GetPlayer()))
	{
		if (pPlayer->IsThrownByPlayer(stim.sourceId))
			rad *= 1.5f;
	}

	const CAISystem::PuppetSet& enabledPuppetsSet = GetAISystem()->GetEnabledPuppetSet();
	for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
	{ 
		CPuppet* pReceiverPuppet = it->GetAIObject();
		if (!pReceiverPuppet)
			continue;

		if (!pReceiverPuppet->IsPerceptionEnabled())
			continue;

		// Do not report sounds to the sound source.
		if (pReceiverPuppet->GetEntityID() == stim.sourceId)
			continue;

		// No explosion sounds for same group
		if (stim.subType == AISOUND_EXPLOSION)
		{
			if (pSenderActor && !pSenderActor->IsHostile(pReceiverPuppet) &&
					pReceiverPuppet->GetGroupId() == pSenderActor->GetGroupId())
			continue;
		}

		// No vehicle sounds for same species - destructs convoys
		if (pSenderActor && !pSenderActor->IsHostile(pReceiverPuppet) && pSenderActor->GetType() == AIOBJECT_VEHICLE)
			continue;

		// The sound is occluded because of the buildings.
		if (IsSoundOccluded(pReceiverPuppet, stim.pos))
			continue;

		// Send event.
		SAIEVENT event;
		event.vPosition = stim.pos;
		event.fThreat = rad;
		event.nType = (int)stim.subType;
		event.nFlags = stim.flags;
		event.sourceId = stim.sourceId;
		pReceiverPuppet->Event(AIEVENT_ONSOUNDEVENT, &event);

		RecordStimulusEvent(stim, event.fThreat, *pReceiverPuppet);
	}
}

//===================================================================
// HandleCollisionEvent
//===================================================================
void CPerceptionManager::HandleCollision(const SStimulusRecord& stim)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	IEntity* pCollider = gEnv->pEntitySystem->GetEntity(stim.sourceId);
	IEntity* pTarget = gEnv->pEntitySystem->GetEntity(stim.targetId);

	CAIActor* pColliderAI = pCollider ? CastToCAIActorSafe(pCollider->GetAI()) : 0;
	CAIActor* pTargetAI = pTarget ? CastToCAIActorSafe(pTarget->GetAI()) : 0;

	// Do not report AI collisions
	if (pColliderAI || pTargetAI)
		return;

	CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetAISystem()->GetPlayer());
	bool thrownByPlayer = false;
	if (pCollider && pPlayer)
		thrownByPlayer = pPlayer->IsThrownByPlayer(pCollider->GetId());

	// Allow to react to larger objects which collide nearby.
	const CAISystem::PuppetSet& enabledPuppetsSet = GetAISystem()->GetEnabledPuppetSet();
	for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
	{
		CPuppet* pReceiverPuppet = it->GetAIObject();
		if (!pReceiverPuppet)
			continue;

		if (!pReceiverPuppet->IsPerceptionEnabled())
			continue;

		EntityId id = pReceiverPuppet->GetEntity()->GetId();

		const float scale = pReceiverPuppet->GetParameters().m_PerceptionParams.collisionReactionScale;
		const float rangeSq = sqr(stim.radius * scale);

		float distSq = Distance::Point_PointSq(stim.pos, pReceiverPuppet->GetPos());
		if (distSq > rangeSq) continue;

		IAISignalExtraData* pData = GetAISystem()->CreateSignalExtraData();
		pData->iValue = thrownByPlayer ? 1 : 0;
		pData->fValue = sqrtf(distSq);
		pData->point = stim.pos;
		pReceiverPuppet->SetSignal(0, "OnCloseCollision", 0, pData);

		if (thrownByPlayer)
			pReceiverPuppet->SetAlarmed();

		RecordStimulusEvent(stim, 0.0f, *pReceiverPuppet);
	}
}

//===================================================================
// HandleExplosion
//===================================================================
void CPerceptionManager::HandleExplosion(const SStimulusRecord& stim)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	// React to explosions.
	const CAISystem::PuppetSet& enabledPuppetsSet = GetAISystem()->GetEnabledPuppetSet();
	for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
	{
		CPuppet* pReceiverPuppet = it->GetAIObject();
		if (!pReceiverPuppet)
			continue;

		if (!pReceiverPuppet->IsPerceptionEnabled())
			continue;

		const float scale = pReceiverPuppet->GetParameters().m_PerceptionParams.collisionReactionScale;
		const float rangeSq = sqr(stim.radius * scale);

		float distSq = Distance::Point_PointSq(stim.pos, pReceiverPuppet->GetPos());
		if (distSq > rangeSq) continue;

		IAISignalExtraData* pData = GetAISystem()->CreateSignalExtraData();
		pData->point = stim.pos;
		pReceiverPuppet->SetSignal(0, "OnExposedToExplosion", 0, pData);
		pReceiverPuppet->SetAlarmed();

		RecordStimulusEvent(stim, 0.0f, *pReceiverPuppet);
	}
}

//===================================================================
// HandleBulletHit
//===================================================================
void CPerceptionManager::HandleBulletHit(const SStimulusRecord& stim)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	IEntity* pShooterEnt = gEnv->pEntitySystem->GetEntity(stim.sourceId);
	CAIActor* pShooterActor = NULL;
	if (pShooterEnt)
		 pShooterActor = CastToCAIActorSafe(pShooterEnt->GetAI());

	// Send bullet events
	const CAISystem::PuppetSet& enabledPuppetsSet = GetAISystem()->GetEnabledPuppetSet();
	for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
	{
		CPuppet* pReceiverPuppet = it->GetAIObject();
		if (!pReceiverPuppet)
			continue;

		if (!pReceiverPuppet->IsPerceptionEnabled())
			continue;

		// Skip own fire.
		if (stim.sourceId == pReceiverPuppet->GetEntityID()) continue;
		// Skip non-hostile bullets.
		if (!pReceiverPuppet->IsHostile(pShooterActor)) continue;

		AABB bounds;
		pReceiverPuppet->GetEntity()->GetWorldBounds(bounds);
		const float d = Distance::Point_AABBSq(stim.pos, bounds);
		const float r = max(stim.radius, pReceiverPuppet->m_Parameters.m_PerceptionParams.bulletHitRadius);

		if (d < sqr(r))
		{
			// Send event.
			SAIEVENT event;
			event.sourceId = pShooterEnt ? pShooterEnt->GetId() : 0;
			event.vPosition = pShooterEnt ? pShooterEnt->GetWorldPos() : stim.pos;
			event.nFlags = stim.flags;
			pReceiverPuppet->Event(AIEVENT_ONBULLETRAIN, &event);

			RecordStimulusEvent(stim, 0.0f, *pReceiverPuppet);
		}
	}
}

//===================================================================
// HandleBulletWhizz
//===================================================================
void CPerceptionManager::HandleBulletWhizz(const SStimulusRecord& stim)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	IEntity* pShooterEnt = gEnv->pEntitySystem->GetEntity(stim.sourceId);
	IAIObject* pShooterAI = pShooterEnt ? pShooterEnt->GetAI() : 0;

	Lineseg lof(stim.pos, stim.pos + stim.dir*stim.radius);
	float t;

	const CAISystem::PuppetSet& enabledPuppetsSet = GetAISystem()->GetEnabledPuppetSet();
	for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
	{
		CPuppet* pReceiverPuppet = it->GetAIObject();
		if (!pReceiverPuppet)
			continue;

		if (!pReceiverPuppet->IsPerceptionEnabled())
			continue;

		// Skip own fire.
		if (pReceiverPuppet->GetEntityID() == stim.sourceId) continue;
		// Skip non-hostile bullets.
		if (!pReceiverPuppet->IsHostile(pShooterAI)) continue;

		float d = Distance::Point_LinesegSq(pReceiverPuppet->GetPhysicsPos(), lof, t);
		const float r = 5.0f;
		if (d < sqr(r))
		{
			const Vec3 hitPos = lof.GetPoint(t);
			SAIStimulus hitStim(AISTIM_BULLET_HIT, 0, stim.sourceId, 0, hitPos, ZERO, r);
			RegisterStimulus(hitStim);
		}

		RecordStimulusEvent(stim, 0.0f, *pReceiverPuppet);
	}

}

//===================================================================
// HandleGrenade
//===================================================================
void CPerceptionManager::HandleGrenade(const SStimulusRecord& stim)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	EntityId shooterId = stim.sourceId;

	IEntity* pShooter = gEnv->pEntitySystem->GetEntity(shooterId);
	if (!pShooter)
		return;

	CAIActor* pShooterActor = 0;
	if (pShooter)
		pShooterActor = CastToCAIActorSafe(pShooter->GetAI());

	const CAISystem::PuppetSet& enabledPuppetsSet = GetAISystem()->GetEnabledPuppetSet();

	if (stim.subType == AIGRENADE_THROWN)
	{
		float radSq = sqr(stim.radius);
		Vec3 throwPos = pShooterActor ? pShooterActor->GetFirePos() : stim.pos; // Grenade position
		// Inform the AI that sees the throw
		for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
		{
			CPuppet* pPuppet = it->GetAIObject();
			if (!pPuppet || pPuppet->GetEntityID() == shooterId)
				continue;

			if (!pPuppet->IsPerceptionEnabled())
				continue;

			// If the puppet is not close to the predicted position, skip.
			if (Distance::Point_PointSq(stim.pos, pPuppet->GetPos()) > radSq)
				continue;

			// Inform enemies only.
			if (pShooterActor && !pShooterActor->IsHostile(pPuppet))
				continue;

			// Only sense grenades that are on front of the AI and visible when thrown.
			// Another signal is sent when the grenade hits the ground.
			Vec3 delta = throwPos - pPuppet->GetPos();	// grenade to AI
			float dist = delta.NormalizeSafe();
			const float thr = cosf(DEG2RAD(160.0f));
			if (delta.Dot(pPuppet->GetViewDir()) > thr)
			{
				static const int objTypes = ent_static | ent_terrain | ent_rigid | ent_sleeping_rigid;
				static const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;
				const RayCastResult& result = gAIEnv.pRayCaster->Cast(RayCastRequest(pPuppet->GetPos(), delta*dist, objTypes, flags));

				if (result)
					throwPos = result[0].pt;

				GetAISystem()->AddDebugLine(pPuppet->GetPos(), throwPos, 255,0,0, 15.0f);

				if (!result || result[0].dist > dist*0.9f)
				{
					IAISignalExtraData* pEData = GetAISystem()->CreateSignalExtraData();	// no leak - this will be deleted inside SendAnonymousSignal
					pEData->point = stim.pos; // avoid predicted pos
					pEData->nID = shooterId;
					pEData->iValue = 1;
					GetAISystem()->SendSignal(SIGNALFILTER_SENDER, 1, "OnGrenadeDanger", pPuppet, pEData);

					RecordStimulusEvent(stim, 0.0f, *pPuppet);
				}
			}
		}
	}
	else if (stim.subType == AIGRENADE_COLLISION)
	{
		float radSq = sqr(stim.radius);

		for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
		{
			CPuppet* pPuppet = it->GetAIObject();

			// If the puppet is not close to the grenade, skip.
			if (!pPuppet || Distance::Point_PointSq(stim.pos, pPuppet->GetPos()) > radSq)
				continue;
			IAISignalExtraData* pEData = GetAISystem()->CreateSignalExtraData();	// no leak - this will be deleted inside SendAnonymousSignal
			pEData->point = stim.pos;
			pEData->nID = shooterId;
			pEData->iValue = 2;
			GetAISystem()->SendSignal(SIGNALFILTER_SENDER, 1, "OnGrenadeDanger", pPuppet, pEData);

			RecordStimulusEvent(stim, 0.0f, *pPuppet);
		}
	}
	else if (stim.subType == AIGRENADE_FLASH_BANG)
	{
		float radSq = sqr(stim.radius);
		for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
		{
			CPuppet* pPuppet = it->GetAIObject();
			if (!pPuppet || pPuppet->GetEntityID() == shooterId) continue;

			// If the puppet is not close to the flash, skip.
			if (Distance::Point_PointSq(stim.pos, pPuppet->GetPos()) > radSq)
				continue;

			// Only sense grenades that are on front of the AI and visible when thrown.
			// Another signal is sent when the grenade hits the ground.
			Vec3 delta = stim.pos - pPuppet->GetPos();	// grenade to AI
			float dist = delta.NormalizeSafe();
			const float thr = cosf(DEG2RAD(160.0f));
			if (delta.Dot(pPuppet->GetViewDir()) > thr)
			{
				static const int objTypes = ent_static | ent_terrain | ent_rigid | ent_sleeping_rigid;
				static const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;

				if (!gAIEnv.pRayCaster->Cast(RayCastRequest(pPuppet->GetPos(), delta*dist, objTypes, flags)))
				{
					IAISignalExtraData* pExtraData = GetAISystem()->CreateSignalExtraData();
					pExtraData->iValue = 0;
					GetAISystem()->SendSignal(SIGNALFILTER_SENDER, 1, "OnExposedToFlashBang", pPuppet, pExtraData);

					RecordStimulusEvent(stim, 0.0f, *pPuppet);
				}
			}
		}
	}
	else if (stim.subType == AIGRENADE_SMOKE)
	{
		float radSq = sqr(stim.radius);
		for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
		{
			CPuppet* pPuppet = it->GetAIObject();

			// If the puppet is not close to the smoke, skip.
			if (!pPuppet || Distance::Point_PointSq(stim.pos, pPuppet->GetPos()) > radSq)
				continue;

			GetAISystem()->SendSignal(SIGNALFILTER_SENDER, 1, "OnExposedToSmoke", pPuppet);

			RecordStimulusEvent(stim, 0.0f, *pPuppet);
		}
	}
}

//===================================================================
// RegisterAIEventListener
//===================================================================
void CPerceptionManager::RegisterAIEventListener(IAIEventListener* pListener, const Vec3& pos, float rad, int flags)
{
	if (!pListener)
		return;

	// Check if the listener exists
	for (unsigned i = 0, ni = m_eventListeners.size(); i < ni; ++i)
	{
		SAIEventListener& listener = m_eventListeners[i];
		if (listener.pListener == pListener)
		{
			listener.pos = pos;
			listener.radius = rad;
			listener.flags = flags;
			return;
		}
	}

	m_eventListeners.resize(m_eventListeners.size() + 1);
	SAIEventListener& listener = m_eventListeners.back();
	listener.pListener = pListener;
	listener.pos = pos;
	listener.radius = rad;
	listener.flags = flags;
}

//===================================================================
// UnregisterAIEventListener
//===================================================================
void CPerceptionManager::UnregisterAIEventListener(IAIEventListener* pListener)
{
	if (!pListener)
		return;

	// Check if the listener exists
	for (unsigned i = 0, ni = m_eventListeners.size(); i < ni; ++i)
	{
		SAIEventListener& listener = m_eventListeners[i];
		if (listener.pListener == pListener)
		{
			m_eventListeners[i] = m_eventListeners.back();
			m_eventListeners.pop_back();
			return;
		}
	}
}

//===================================================================
// IsPointInRadiusOfStimulus
//===================================================================
bool CPerceptionManager::IsPointInRadiusOfStimulus(EAIStimulusType type, const Vec3& pos) const
{
	const std::vector<SStimulusRecord>& stims = m_stimuli[(int)type];
	for (unsigned i = 0, ni = stims.size(); i < ni; ++i)
	{
		const SStimulusRecord& s = stims[i];
		if (Distance::Point_PointSq(s.pos, pos) < sqr(s.radius))
			return true;
	}
	return false;
}

//===================================================================
// NotifyAIEventListeners
//===================================================================
void CPerceptionManager::NotifyAIEventListeners(const SStimulusRecord& stim, float threat)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	int flags = 1 << (int)stim.type;

	for (unsigned i = 0, ni = m_eventListeners.size(); i < ni; ++i)
	{
		SAIEventListener& listener = m_eventListeners[i];
		if ((listener.flags & flags) != 0 && Distance::Point_PointSq(stim.pos, listener.pos) < sqr(listener.radius + stim.radius))
			listener.pListener->OnAIEvent((EAIStimulusType)stim.type, stim.pos, stim.radius, threat, stim.sourceId);
	}
}

//===================================================================
// SupressSound
//===================================================================
float CPerceptionManager::SupressSound(const Vec3& pos, float radius)
{
	float minScale = 1.0f;
	CAISystem::AIObjectOwners::iterator ai = GetAISystem()->m_Objects.find(AIOBJECT_SNDSUPRESSOR), aiend = GetAISystem()->m_Objects.end();
	for (;ai != aiend; ++ai)
	{
		if (ai->first != AIOBJECT_SNDSUPRESSOR)
			break;
		CAIObject* obj = ai->second.GetAIObject();
		if (!obj->IsEnabled())
			continue;
		const float r = obj->GetRadius();
		const float silenceRadius = r * 0.3f;
		const float distSqr = Distance::Point_PointSq(pos, obj->GetPos());
		if (distSqr > sqr(r))
			continue;
		if (distSqr < sqr(silenceRadius))
			return 0.0f;

		const float dist = sqrtf(distSqr);
		const float scale = (dist - silenceRadius) / (r - silenceRadius);
		minScale = min(minScale, scale);
	}

	return radius * minScale;
}	

//===================================================================
// IsSoundOccluded
//===================================================================
bool CPerceptionManager::IsSoundOccluded(CPuppet *pPuppet, const Vec3& soundPos)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);

	// TODO: Check if this function is necessary at all!

	int nBuildingIDPuppet = -1, nBuildingIDSound = -1;
	IVisArea* pAreaPuppet = 0;
	IVisArea* pAreaSound = 0;
	gAIEnv.pNavigation->CheckNavigationType(pPuppet->GetPos(), nBuildingIDPuppet, pAreaPuppet, pPuppet->m_movementAbility.pathfindingProperties.navCapMask);
	gAIEnv.pNavigation->CheckNavigationType(soundPos, nBuildingIDSound, pAreaSound, pPuppet->m_movementAbility.pathfindingProperties.navCapMask);

	// Sound and puppet both are outdoor, not occluded.
	if (nBuildingIDPuppet < 0 && nBuildingIDSound < 0)
		return false;

	if (nBuildingIDPuppet != nBuildingIDSound)
	{
		if (nBuildingIDPuppet < 0)
		{
			// Puppet is outdoors, sound indoors
			if (pAreaSound)
			{
				// Occluded if the area is not connected to outdoors.
				return !pAreaSound->IsConnectedToOutdoor();
			}
		}
		else if (nBuildingIDSound < 0)
		{
			// Sound is outdoors, puppet indoors
			if (pAreaPuppet)
			{
				// Occluded if the area is not connected to outdoors.
				return !pAreaPuppet->IsConnectedToOutdoor();
			}
		}
		else
		{
			// If in two different buildings we cannot hear the sound for sure.
			// Require one building to have visarea, though.
			if (pAreaPuppet || pAreaSound)
				return true;
		}
	}

	// Not occluded
	return false;
}

//===================================================================
// RayOcclusionPlaneIntersection
//===================================================================
int CPerceptionManager::RayOcclusionPlaneIntersection(const Vec3 &start,const Vec3 &end)
{
	const ShapeMap& occPlanes = GetAISystem()->GetOcclusionPlanes();

	if (occPlanes.empty())
		return 0;

	ShapeMap::const_iterator di = occPlanes.begin(),diend = occPlanes.end();
	for (;di!=diend;++di)
	{
		if (!di->second.shape.empty())
		{
			float fShapeHeight = ((di->second.shape).front()).z;
			if ( (start.z<fShapeHeight) && (end.z<fShapeHeight))
				continue;
			if ( (start.z>fShapeHeight) && (end.z>fShapeHeight))
				continue;

			// find out where ray hits horizontal plane fShapeHeigh (with a nasty workaround)
			Vec3 vIntersection;
			float t = (start.z-fShapeHeight)/(start.z-end.z);
			vIntersection = start+t*(end-start);


			// is it inside the polygon?
			if (Overlap::Point_Polygon2D(vIntersection, di->second.shape, &di->second.aabb))
				return 1;
		}
	}

	return 0;
}

//===================================================================
// Serialize
//===================================================================
void CPerceptionManager::Serialize(TSerialize ser)
{
	char name[64];
	ser.BeginGroup("PerceptionManager");
	for (unsigned i = 0; i < AI_MAX_STIMULI; ++i)
	{
		_snprintf(name, 64, "m_stimuli%02d", i);
		ser.Value(name, m_stimuli[i]);
		_snprintf(name, 64, "m_ignoreStimuliFrom%02d", i);
		ser.Value(name, m_ignoreStimuliFrom[i]);
	}
	ser.EndGroup();
}

//===================================================================
// OnObjectRemoved
//===================================================================
void CPerceptionManager::OnObjectRemoved(CAIObject *pObject)
{
	CPuppet *pPuppet = CastToCPuppetSafe(pObject);
	if (pPuppet)
		RemovePuppetLOSQueries(pPuppet);
}

//===================================================================
// RecordStimulusEvent
//===================================================================
void CPerceptionManager::RecordStimulusEvent(const SStimulusRecord& stim, float fRadius, IAIObject &receiver) const
{
	if (!gEnv->pAISystem->IsRecording(&receiver, IAIRecordable::E_REGISTERSTIMULUS))
		return;

	string sType = g_szAIStimulusType[stim.type];
	if (stim.type == AISTIM_SOUND)
		sType += g_szAISoundStimType[stim.subType];
	else if (stim.type == AISTIM_GRENADE)
		sType += g_szAIGrenadeStimType[stim.subType];

	IEntity *pSource = gEnv->pEntitySystem->GetEntity(stim.sourceId);
	IEntity *pTarget = gEnv->pEntitySystem->GetEntity(stim.targetId);
	string sDebugLine;
	sDebugLine.Format("%s from %s to %s", sType.c_str(), pSource?pSource->GetName():"Unknown", pTarget?pTarget->GetName():"All");

	// Record event
	gEnv->pAISystem->Record(&receiver, IAIRecordable::E_REGISTERSTIMULUS, sDebugLine.c_str());
	IAIRecordable::RecorderEventData recorderEventData(sDebugLine.c_str());
	receiver.RecordEvent(IAIRecordable::E_REGISTERSTIMULUS, &recorderEventData);
}

CPerceptionManager* CPerceptionManager::GetInstance()
{
	return (Instance ? Instance : Instance = new CPerceptionManager());
}

void CPerceptionManager::DestroyInstance()
{
	if(Instance){delete Instance; Instance=NULL;}
}
