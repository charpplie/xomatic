/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   CAISystemUpdate.cpp
$Id$
Description: all the update related functionality here

-------------------------------------------------------------------------
History:
- 2007				: Created by Kirill Bulatsev
- 2 Mar 2009	: Evgeny Adamenkov: Removed IRenderer ("gEnv->pRenderer")

*********************************************************************/





#include "StdAfx.h"

#include <stdio.h>

#include <limits>
#include <map>
#include <numeric>
#include <algorithm>

#include "IPhysics.h"
#include "IEntitySystem.h"
#include "IScriptSystem.h"
#include "I3DEngine.h"
#include "ILog.h"
#include "CryFile.h"
#include "Cry_Math.h"
#include "ISystem.h"
#include "IEntityRenderState.h"
#include "ITimer.h"
#include "IConsole.h"
#include "ISerialize.h"
#include "IAgent.h"
#include "VectorSet.h"
#include <AISystemListener.h>

#include "CAISystem.h"
#include "CryAISystem.h"
#include "AILog.h"
#include "CTriangulator.h"
#include "TriangularNavRegion.h"
#include "WaypointHumanNavRegion.h"
#include "Waypoint3DSurfaceNavRegion.h"
#include "VolumeNavRegion.h"
#include "FlightNavRegion.h"
#include "RoadNavRegion.h"
#include "SmartObjectNavRegion.h"
#include "Free2DNavRegion.h"
#include "Graph.h"
#include "AStarSolver.h"
#include "Puppet.h"
#include "AIVehicle.h"
#include "GoalOp.h"
#include "AIPlayer.h"
#include "PipeUser.h"
#include "Leader.h"
#include "ObjectTracker.h"
#include "SmartObjects.h"
#include "AIActions.h"
#include "AICollision.h"
#include "AIRadialOcclusion.h"
#include "GraphNodeManager.h"
#include "CentralInterestManager.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/ActivationConditions.h"
#include "BehaviorTree/ProfileDictionary.h"
#include "EmotionalSystem/EmotionalSystem.h"
#include "GroupSystem/GroupSystem.h"
#include "CodeCoverageManager.h"
#include "CodeCoverageGUI.h"
#include "StatsManager.h"
#include "TacticalPointSystem/TacticalPointSystem.h"
#include "CoordinationSystem/CoordinationManager.h"
#include "Communication/CommunicationManager.h"
#include "SelectionTree/SelectionTreeManager.h"

#include "DebugDrawContext.h"

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::TakeDetectionSnapshot()
{
	// Reset detection on player and squadmates
	CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetPlayer());
	if (pPlayer)
	{
		pPlayer->SnapshotDetectionLevels();
		pPlayer->ResetDetectionLevels();

		int groupid = pPlayer->GetGroupId();
		AIObjects::iterator itSquadMates = m_mapGroups.find(groupid);
		while (itSquadMates != m_mapGroups.end() && itSquadMates->first == groupid)
		{
			CAIObject* pObject = itSquadMates->second.GetAIObject();

			CPuppet* pPuppet = CastToCPuppetSafe(pObject);
			if (pPuppet)
			{
				pPuppet->SnapshotDetectionLevels();
				pPuppet->ResetDetectionLevels();
			}

			++itSquadMates;
		}
	}
}

//
//-----------------------------------------------------------------------------------------------------------
static bool IsPuppetOnScreen(CPuppet* pPuppet)
{
	IEntity* pEntity = pPuppet->GetEntity();
	if (!pEntity)
		return false;
	IEntityRenderProxy* pRenderProxy = (IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER);
	if (!pRenderProxy || !pRenderProxy->GetRenderNode())
		return false;
	int frameDiff = gEnv->pRenderer->GetFrameID() - pRenderProxy->GetRenderNode()->GetDrawFrame();
	if (frameDiff > 2)
		return false;
	return true;
}

//
//-----------------------------------------------------------------------------------------------------------
EPuppetUpdatePriority CAISystem::CalcPuppetUpdatePriority(CPuppet* pPuppet) const
{
	float fMinDistSq = std::numeric_limits<float>::max();
	bool bOnScreen = false;
	const Vec3 pos = pPuppet->GetPos();

	if(gAIEnv.configuration.eCompatibilityMode != ECCM_CRYSIS && gAIEnv.configuration.eCompatibilityMode != ECCM_CRYSIS2)
	{
		// find closest player distance (better than using the camera pos in coop / dedicated server)
		//	and check visibility against all players

		AIObjectOwners::const_iterator ai=m_Objects.find(AIOBJECT_PLAYER);
		for (;ai!=m_Objects.end() && ai->first==AIOBJECT_PLAYER; ++ai)
		{
			CAIPlayer* pPlayer = CastToCAIPlayerSafe(ai->second.GetAIObject());
			if (pPlayer)
			{
				fMinDistSq = min(fMinDistSq, (pos - pPlayer->GetPos()).GetLengthSquared());
				
				if(!bOnScreen)
					bOnScreen = (IAIObject::eFOV_Outside != pPlayer->IsPointInFOV(pos, 2.0f));	// double range for this check, real sight range is used below.
			}
		}
	}
	else
	{
		// previous behavior retained for Crysis compatibility
		Vec3 camPos = gEnv->pSystem->GetViewCamera().GetPosition();
		fMinDistSq = Distance::Point_PointSq(camPos, pos);
		bOnScreen = IsPuppetOnScreen(pPuppet);
	}

	// Calculate the update priority of the puppet.
	const float fSightRangeSq = sqr(pPuppet->GetParameters().m_PerceptionParams.sightRange);
	const bool bInSightRange = (fMinDistSq < fSightRangeSq);
	if (bOnScreen)
	{
		return (bInSightRange ? AIPUP_VERY_HIGH : AIPUP_HIGH);
	}
	else
	{
		return (bInSightRange ? AIPUP_MED : AIPUP_LOW);
	}
}

//
//-----------------------------------------------------------------------------------------------------------
#ifdef CRYAISYSTEM_DEBUG
void CAISystem::UpdateDebugStuff()
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );
	// Delete the debug lines if the debug draw is not on.
	if((gAIEnv.CVars.DebugDraw==0))
	{
		m_vecDebugLines.clear();
		m_vecDebugBoxes.clear();
	}

	bool drawCover = (gAIEnv.CVars.DebugDraw != 0) && (gAIEnv.CVars.DebugDrawCover != 0);
	
	const char* debugHideSpotName = gAIEnv.CVars.DebugHideSpotName;
	drawCover &= (debugHideSpotName && debugHideSpotName[0] && strcmp(debugHideSpotName, "0"));

	if (drawCover)
	{
		uint32 anchorTypes[2] =
		{
			AIANCHOR_COMBAT_HIDESPOT,
			AIANCHOR_COMBAT_HIDESPOT_SECONDARY
		};

		uint32 anchorTypeCount = sizeof(anchorTypes)/sizeof(anchorTypes[0]);

		bool all = !stricmp(debugHideSpotName, "all");
		if (!all && (m_DebugHideObjects.size() > 1))
			m_DebugHideObjects.clear();

		const float maxDistanceSq = sqr(75.0f);
		CCamera camera = gEnv->pSystem->GetViewCamera();

		bool found = false;
		for (int at = 0 ; at < anchorTypeCount; ++at)
		{
			const AIObjectOwners::const_iterator itEnd = m_Objects.end();
			for (AIObjectOwners::const_iterator it = m_Objects.find(anchorTypes[at]); it != itEnd ; ++it)
			{
				CAIObject* pObject = it->second.GetAIObject();
				if (pObject->GetType() != anchorTypes[at])
					break;

				if (!pObject->IsEnabled())
					continue;

				const GraphNode *pNode = pObject->GetAnchorNavNode();
				if (!pNode)
					continue;

				if (all)
				{
					if ((pObject->GetPos() - camera.GetPosition()).GetLengthSquared() > maxDistanceSq)
					{
						DebugHideObjectMap::iterator delIt = m_DebugHideObjects.find(pObject->GetAIObjectID());
						if (delIt != m_DebugHideObjects.end())
							m_DebugHideObjects.erase(delIt);

						continue;
					}

					// Marcio: Assume max radius for now!
					Sphere sphere(pObject->GetPos(), 12.0f);

					if (!camera.IsSphereVisible_F(sphere))
					{
						DebugHideObjectMap::iterator delIt = m_DebugHideObjects.find(pObject->GetAIObjectID());
						if (delIt != m_DebugHideObjects.end())
							m_DebugHideObjects.erase(delIt);

						continue;
					}
				}
				else
				{
					if (stricmp(pObject->GetName(), debugHideSpotName))
						continue;
				}

				SHideSpot hideSpot(SHideSpotInfo::eHST_ANCHOR, pObject->GetPos(), pObject->GetMoveDir());
				hideSpot.pNavNode = pNode;
				hideSpot.pAnchorObject = pObject;
 
				std::pair<DebugHideObjectMap::iterator, bool> result = m_DebugHideObjects.insert(
					DebugHideObjectMap::value_type(pObject->GetAIObjectID(), CAIHideObject()));		
				
				CAIHideObject& hideObject = result.first->second;
				hideObject.Set(&hideSpot, pObject->GetPos(), pObject->GetMoveDir());

				if (!all)
				{
					found = true;
					break;
				}
			}

			if (!all && found)
				break;
		}

		// clean up removed objects
		{
			DebugHideObjectMap::iterator it = m_DebugHideObjects.begin();
			DebugHideObjectMap::iterator itEnd = m_DebugHideObjects.end();

			while (it != itEnd)
			{
				CAIObject* pAIObject = static_cast<CAIObject*>(GetAISystem()->GetAIObject(it->first));

				bool ok = false;
				if (pAIObject && pAIObject->IsEnabled())
				{
					for (uint32 at = 0; at < anchorTypeCount; ++at)
					{
						if (pAIObject->GetType() == anchorTypes[at])
						{
							ok = true;
							break;
						}
					}
				}

				DebugHideObjectMap::iterator toErase = it++;
				if (!ok)
					m_DebugHideObjects.erase(toErase);
			}
		}

		// update and draw them
		DebugHideObjectMap::iterator it = m_DebugHideObjects.begin();
		DebugHideObjectMap::iterator itEnd = m_DebugHideObjects.end();

		for ( ; it != itEnd; ++it)
		{
			CAIHideObject& debugHideObject = it->second;

			if (debugHideObject.IsValid())
			{
				debugHideObject.HurryUpCoverPathGen();
				while (!debugHideObject.IsCoverPathComplete())
					debugHideObject.Update(0);
				debugHideObject.DebugDraw();
			}
		}
	}
	else
		m_DebugHideObjects.clear();

	// Update fake tracers
	if(gAIEnv.CVars.DrawFakeTracers > 0)
	{
		for(size_t i = 0; i < m_DEBUG_fakeTracers.size();)
		{
			m_DEBUG_fakeTracers[i].t -= m_fFrameDeltaTime;
			if(m_DEBUG_fakeTracers[i].t < 0.0f)
			{
				m_DEBUG_fakeTracers[i] = m_DEBUG_fakeTracers.back();
				m_DEBUG_fakeTracers.pop_back();
			}
			else
			{
				++i;
			}
		}
	}
	else
	{
		m_DEBUG_fakeTracers.clear();
	}

	// Update fake hit effects
	if(gAIEnv.CVars.DrawFakeHitEffects > 0)
	{
		for(size_t i = 0; i < m_DEBUG_fakeHitEffect.size();)
		{
			m_DEBUG_fakeHitEffect[i].t -= m_fFrameDeltaTime;
			if(m_DEBUG_fakeHitEffect[i].t < 0.0f)
			{
				m_DEBUG_fakeHitEffect[i] = m_DEBUG_fakeHitEffect.back();
				m_DEBUG_fakeHitEffect.pop_back();
			}
			else
			{
				++i;
			}
		}
	}
	else
	{
		m_DEBUG_fakeHitEffect.clear();
	}

	// Update fake damage indicators
	if(gAIEnv.CVars.DrawFakeDamageInd > 0)
	{
		for(unsigned i = 0; i < m_DEBUG_fakeDamageInd.size();)
		{
			m_DEBUG_fakeDamageInd[i].t -= m_fFrameDeltaTime;
			if(m_DEBUG_fakeDamageInd[i].t < 0)
			{
				m_DEBUG_fakeDamageInd[i] = m_DEBUG_fakeDamageInd.back();
				m_DEBUG_fakeDamageInd.pop_back();
			}
			else
				++i;
		}
		m_DEBUG_screenFlash = max(0.0f, m_DEBUG_screenFlash - m_fFrameDeltaTime);
	}
	else
	{
		m_DEBUG_fakeDamageInd.clear();
		m_DEBUG_screenFlash = 0.0f;
	}
}
#endif //CRYAISYSTEM_DEBUG

//===================================================================
// GetUpdateAllAlways
//===================================================================
bool CAISystem::GetUpdateAllAlways() const
{
	bool updateAllAlways = gAIEnv.CVars.UpdateAllAlways != 0;
	return updateAllAlways;
}

struct SSortedPuppetB
{
	SSortedPuppetB(CPuppet* o, float w, float dot, float d) : obj(o), weight(w), dot(dot), dist(d) {}
	bool	operator<(const SSortedPuppetB& rhs) const { return weight < rhs.weight;	}
	bool	operator>(const SSortedPuppetB& rhs) const	{ return weight > rhs.weight;	}

	float			weight, dot, dist;
	CPuppet*	obj;
};

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::UpdateAmbientFire()
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	if (gAIEnv.CVars.AmbientFireEnable == 0)
		return;

	float	dt = (GetFrameStartTime() - m_lastAmbientFireUpdateTime).GetSeconds();
	if(dt < gAIEnv.CVars.AmbientFireUpdateInterval)
		return;

	// Marcio: Update ambient fire towards all players.
	for (AIObjectOwners::const_iterator ai = m_Objects.find(AIOBJECT_PUPPET), end =  m_Objects.end(); ai != end && ai->first == AIOBJECT_PUPPET; ++ai)
	{
		CAIObject* obj = ai->second.GetAIObject();
		if(!obj->IsEnabled())
			continue;
		CPuppet* pPuppet = obj->CastToCPuppet();
		if(!pPuppet)
			continue;

		// By default make every AI in ambient fire, only the ones that are allowed to shoot right are set explicitly.
		pPuppet->SetAllowedToHitTarget(false);
	}

	for (AIObjectOwners::const_iterator ai = m_Objects.find(AIOBJECT_VEHICLE), end =  m_Objects.end(); ai != end && ai->first == AIOBJECT_VEHICLE; ++ai)
	{
		CAIVehicle* obj = CastToCAIVehicleSafe(ai->second.GetAIObject());
		if(!obj || !obj->IsDriverInside())
			continue;
		CPuppet* pPuppet = obj->CastToCPuppet();
		if(!pPuppet)
			continue;

		// By default make every AI in ambient fire, only the ones that are allowed to shoot right are set explicitly.
		pPuppet->SetAllowedToHitTarget(false);
	}

	AIObjectOwners::const_iterator plit=m_Objects.find(AIOBJECT_PLAYER);
	for (;plit!=m_Objects.end() && plit->first==AIOBJECT_PLAYER; ++plit)
	{
		CAIPlayer* pPlayer = CastToCAIPlayerSafe(plit->second.GetAIObject());
		if (!pPlayer)
			return;

		m_lastAmbientFireUpdateTime = GetFrameStartTime();

		const Vec3&	playerPos = pPlayer->GetPos();
		const Vec3& playerDir = pPlayer->GetMoveDir();

		typedef std::list<SSortedPuppetB, stl::STLPoolAllocator<SSortedPuppetB> > TShooters;
		TShooters shooters;

		float maxDist = 0.0f;

		// Update
		for (AIObjectOwners::const_iterator ai = m_Objects.find(AIOBJECT_PUPPET), end =  m_Objects.end(); ai != end && ai->first == AIOBJECT_PUPPET; ++ai)
		{
			CAIObject* obj = ai->second.GetAIObject();
			if(!obj->IsEnabled())
				continue;
			CPuppet* pPuppet = obj->CastToCPuppet();
			if(!pPuppet)
				continue;

			CAIObject* pTarget = (CAIObject*)pPuppet->GetAttentionTarget();

			if(pTarget && pTarget->IsAgent())
			{
				if(pTarget == pPlayer)
				{
					Vec3	dirPlayerToPuppet = pPuppet->GetPos() - playerPos;
					float	dist = dirPlayerToPuppet.NormalizeSafe();
					if(dist > 0.01f && dist < pPuppet->GetParameters().m_fAttackRange)
					{
						maxDist = max(maxDist, dist);
						float dot = playerDir.Dot(dirPlayerToPuppet);
						shooters.push_back(SSortedPuppetB(pPuppet, 1.0f - dot, dot, dist));
					}
					continue;
				}
			}

			// Shooting something else than player, allow to hit.
			pPuppet->SetAllowedToHitTarget(true);
		}

		// Update
		for (AIObjectOwners::const_iterator ai = m_Objects.find(AIOBJECT_VEHICLE), end =  m_Objects.end(); ai != end && ai->first == AIOBJECT_VEHICLE; ++ai)
		{
			CAIVehicle* obj = ai->second.GetAIObject()->CastToCAIVehicle();
			if(!obj->IsDriverInside())
				continue;
			CPuppet* pPuppet = obj->CastToCPuppet();
			if(!pPuppet)
				continue;

			CAIObject* pTarget = (CAIObject*)pPuppet->GetAttentionTarget();

			if(pTarget && pTarget->IsAgent())
			{
				if(pTarget == pPlayer)
				{
					Vec3	dirPlayerToPuppet = pPuppet->GetPos() - playerPos;
					float	dist = dirPlayerToPuppet.NormalizeSafe();

					if ((dist > 0.01f) && (dist < pPuppet->GetParameters().m_fAttackRange) && pPuppet->AllowedToFire())
					{
						maxDist = max(maxDist, dist);
	
						float dot = playerDir.Dot(dirPlayerToPuppet);
						shooters.push_back(SSortedPuppetB(pPuppet, 1.0f - dot, dot, dist));
					}

					continue;
				}
			}
			// Shooting something else than player, allow to hit.
			pPuppet->SetAllowedToHitTarget(true);
		}

		if(!shooters.empty() && maxDist > 0.01f)
		{
			// Find nearest shooter
			for(TShooters::iterator it = shooters.begin(); it != shooters.end(); ++it)
				it->weight = sqr((1.0f - it->dot) / 2) * (0.3f + 0.7f*it->dist/maxDist);

			shooters.sort();

			Vec3 dirToNearest = shooters.front().obj->GetPos() - playerPos;
			dirToNearest.NormalizeSafe();

			for(TShooters::iterator it = shooters.begin(); it != shooters.end(); ++it)
			{
				Vec3	dirPlayerToPuppet = it->obj->GetPos() - playerPos;
				float	dist = dirPlayerToPuppet.NormalizeSafe();
				float dot = dirToNearest.Dot(dirPlayerToPuppet);
				it->weight = sqr((1.0f - dot) / 2) * (dist/maxDist);
			}

			shooters.sort();

			uint32 i = 0;
			uint32 quota = gAIEnv.CVars.AmbientFireQuota;

			for (TShooters::iterator it = shooters.begin(); it != shooters.end(); ++it)
			{
				it->obj->SetAllowedToHitTarget(true);
				if ((++i >= quota) && (it->dist > 7.5f)) // Always allow to hit if in 2.5 meter radius
					break;
			}
		}
	}
}


inline bool PuppetFloatSorter(const std::pair<CPuppet*, float>& lhs, const std::pair<CPuppet*, float>& rhs)
{
	return lhs.second < rhs.second;
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::UpdateExpensiveAccessoryQuota()
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	for (unsigned i = 0; i < m_delayedExpAccessoryUpdates.size(); )
	{
		SAIDelayedExpAccessoryUpdate& update = m_delayedExpAccessoryUpdates[i];
		update.time -= m_fFrameDeltaTime;

		if (update.time < 0.0f)
		{
			update.pPuppet->SetAllowedToUseExpensiveAccessory(update.state);
			m_delayedExpAccessoryUpdates[i] = m_delayedExpAccessoryUpdates.back();
			m_delayedExpAccessoryUpdates.pop_back();
		}
		else
			++i;
	}

	const float updateTime = 3.0f;

	float	dt = (GetFrameStartTime() - m_lastExpensiveAccessoryUpdateTime).GetSeconds();
	if (dt < updateTime)
		return;

	m_lastExpensiveAccessoryUpdateTime = GetFrameStartTime();

	m_delayedExpAccessoryUpdates.clear();

	Vec3 interestPos = gEnv->pSystem->GetViewCamera().GetPosition();

	std::vector<std::pair<CPuppet*, float> > puppets;
	VectorSet<CPuppet*>	stateRemoved;

	// Choose the best of each group, then best of the best.
	for (AIGroupMap::iterator it = m_mapAIGroups.begin(), itend = m_mapAIGroups.end(); it != itend; ++it)
	{
		CAIGroup* pGroup = it->second;

		CPuppet* pBestUnit = 0;
		float bestVal = FLT_MAX;

		for (TUnitList::iterator itu = pGroup->GetUnits().begin(), ituend = pGroup->GetUnits().end(); itu != ituend; ++itu)
		{
			CPuppet* pPuppet = CastToCPuppetSafe(itu->m_refUnit.GetAIObject());
			if (!pPuppet)
				continue;
			if (!pPuppet->IsEnabled())
				continue;

			if (pPuppet->IsAllowedToUseExpensiveAccessory())
				stateRemoved.insert(pPuppet);

			const int accessories = pPuppet->GetParameters().m_weaponAccessories;
			if ((accessories & (AIWEPA_COMBAT_LIGHT|AIWEPA_PATROL_LIGHT)) == 0)
				continue;

			if (pPuppet->GetProxy())
			{
				SAIWeaponInfo wi;
				pPuppet->GetProxy()->QueryWeaponInfo(wi);
				if (!wi.hasLightAccessory)
					continue;
			}

			float val = Distance::Point_Point(interestPos, pPuppet->GetPos());

			if (pPuppet->GetAttentionTargetThreat() == AITHREAT_AGGRESSIVE)
				val *= 0.5f;
			else if (pPuppet->GetAttentionTargetThreat() >= AITHREAT_INTERESTING)
				val *= 0.8f;

			if (val < bestVal)
			{
				bestVal = val;
				pBestUnit = pPuppet;
			}
		}

		if (pBestUnit)
		{
			CCCPOINT(UpdateExpensiveAccessoryQuota);
			puppets.push_back(std::make_pair(pBestUnit, bestVal));
	}

	}

	std::sort(puppets.begin(), puppets.end(), PuppetFloatSorter);

	unsigned maxExpensiveAccessories = 3;
	for (unsigned i = 0, ni = puppets.size(); i < ni && i < maxExpensiveAccessories; ++i)
	{
		stateRemoved.erase(puppets[i].first);

		if (!puppets[i].first->IsAllowedToUseExpensiveAccessory())
		{
			//		puppets[i].first->SetAllowedToUseExpensiveAccessory(true);

			float t = updateTime*0.5f + ai_frand()*updateTime*0.4f;
			m_delayedExpAccessoryUpdates.push_back(SAIDelayedExpAccessoryUpdate(puppets[i].first, t, true));
		}
	}

	for (unsigned i = 0, ni = stateRemoved.size(); i < ni; ++i)
	{
		float t = ai_frand()*updateTime*0.4f;
		m_delayedExpAccessoryUpdates.push_back(SAIDelayedExpAccessoryUpdate(stateRemoved[i], t, false));
	}

}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::SingleDryUpdate(CPuppet * pObject)
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );
	if (pObject->IsEnabled())
		pObject->Update(AIUPDATE_DRY);
	else
		pObject->UpdateDisabled(AIUPDATE_DRY);
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::UpdateAuxSignalsMap()
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	if (!m_mapAuxSignalsFired.empty())
	{
		MapSignalStrings::iterator mss = m_mapAuxSignalsFired.begin();
		while (mss!=m_mapAuxSignalsFired.end())
		{
			(mss->second).fTimeout-= m_fFrameDeltaTime;
			if ((mss->second).fTimeout < 0)
			{
				MapSignalStrings::iterator mss_to_erase = mss;
				++mss;
				m_mapAuxSignalsFired.erase(mss_to_erase);
			}
			else
				++mss;
		}
	}
}
