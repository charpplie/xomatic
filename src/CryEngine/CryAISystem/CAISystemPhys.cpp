/********************************************************************
	StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2004
 -------------------------------------------------------------------------
  File name:   CAISystemPhys.cpp
	$Id$
  Description:	should contaioin all the methods of CAISystem which have to deal with Physics 
  
 -------------------------------------------------------------------------
  History:
	- 2007				: Created by Kirill Bulatsev
	

*********************************************************************/

#include "StdAfx.h"
#include "CAISystem.h"
#include "Puppet.h"


//===================================================================
// GetWaterOcclusionValue
//===================================================================
float CAISystem::GetWaterOcclusionValue(const Vec3& targetPos) const
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	float fResult = 0.0f;

	if (gAIEnv.CVars.WaterOcclusionEnable > 0)
	{
		IPhysicalEntity** pAreas;
		bool bFound = false;
		int nAreas = gEnv->pPhysicalWorld->GetEntitiesInBox(targetPos, targetPos, pAreas, ent_areas);
		for(int i(0); i<nAreas && !bFound; ++i)
		{
			pe_params_buoyancy pb; 
			// if this is a water area (there will be only one)
			if (pAreas[i]->GetParams(&pb) && !is_unused(pb.waterPlane.origin))
			{
				pe_status_contains_point scp; 
				scp.pt = targetPos;
				if (pAreas[i]->GetStatus(&scp))
				{
					const float waterLevel = targetPos.z-pb.waterPlane.n.z*(pb.waterPlane.n*(targetPos-pb.waterPlane.origin));
					const float inWaterDepth = waterLevel - targetPos.z;
					if (inWaterDepth > FLT_EPSILON)
					{
						IWaterVolumeRenderNode *pWaterRenderNode((IWaterVolumeRenderNode*)pAreas[i]->GetForeignData(PHYS_FOREIGN_ID_WATERVOLUME));
						if (pWaterRenderNode)
						{
							const float waterFogDensity = pWaterRenderNode->GetFogDensity();
							const float resValue = gAIEnv.CVars.WaterOcclusionScale * exp(-waterFogDensity*inWaterDepth);

							// make sure it's in 0-1 range
							fResult = 1.f - min(1.f, resValue);
						}
						else
						{
							// no render node if in the ocean
							// make sure it's in 0-1 range
							const float resValue = gAIEnv.CVars.WaterOcclusionScale * inWaterDepth/6.f;
							fResult = min(1.f, resValue);
						}
					}

					bFound = true;
				}
			}
		}
	}

	return fResult;
}




//
//-----------------------------------------------------------------------------------------------------------
bool CAISystem::CheckPointsVisibility(const Vec3& from, const Vec3& to, float rayLength, IPhysicalEntity* pSkipEnt, IPhysicalEntity* pSkipEntAux)
{
	Vec3 dir = to-from;
	if (rayLength && dir.GetLengthSquared() > rayLength*rayLength)
		dir *= rayLength / dir.GetLength();

	IPhysicalEntity* skipList[2] =
	{
		pSkipEnt,
		pSkipEntAux,
	};

	uint skipListSize = 0;
	if (!skipList[0])
	{
		skipList[0] = skipList[1];
		skipList[1] = 0;
	}

	skipListSize += (skipList[0] != 0) ? 1 : 0;
	skipListSize += (skipList[1] != 0) ? 1 : 0;

	return !gAIEnv.pRayCaster->Cast(RayCastRequest(from, dir, COVER_OBJECT_TYPES, HIT_COVER | HIT_SOFT_COVER, 
		skipList, skipListSize));
}

//
//-----------------------------------------------------------------------------------------------------------
bool CAISystem::CheckObjectsVisibility(const IAIObject *pObj1, const IAIObject *pObj2, float rayLength)
{
	Vec3 dir = pObj2->GetPos() - pObj1->GetPos();
	if (rayLength && dir.GetLengthSquared() > sqr(rayLength))
		dir *= rayLength / dir.GetLength();

	std::vector<IPhysicalEntity*> skipList;
	if (const CAIActor* pActor = pObj1->CastToCAIActor())
		pActor->GetPhysicsEntitiesToSkip(skipList);
	if (const CAIActor* pActor = pObj2->CastToCAIActor())
		pActor->GetPhysicsEntitiesToSkip(skipList);

	const RayCastResult& result = gAIEnv.pRayCaster->Cast(RayCastRequest(pObj1->GetPos(), dir, COVER_OBJECT_TYPES,
		HIT_COVER | HIT_SOFT_COVER,	skipList.empty() ? 0 : &skipList[0], skipList.size()));

	// Allow small fudge in th test just in case the point is exactly on ground.
	return !result || result[0].dist > (dir.GetLength() - 0.1f);
}


//
//-----------------------------------------------------------------------------------------------------------
// go trough all the disabled (dead) puppets
void CAISystem::CheckVisibilityBodies()
{
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );
	const int bodiesPerUpdateLimit(2);
	static int lastChecked(0);
	
	if (lastChecked >= (int)m_Objects.size())
		lastChecked = 0;

	AIObjectOwners::iterator ai = m_Objects.find(AIOBJECT_PUPPET);
	AIObjectOwners::iterator end = m_Objects.end();
	std::advance(ai, lastChecked);
	int counter = 0;
	for ( ; ai != end && ai->first == AIOBJECT_PUPPET && counter < bodiesPerUpdateLimit; ++ai, ++counter)
	{
		CPuppet *pBody = (CPuppet *) ai->second.GetAIObject();
		if (!pBody->m_bUncheckedBody)
			continue;

		CPuppet* pClosestPuppet = 0;
		float	closestDistSqr = FLT_MAX;

		for (unsigned j = 0, nj = m_enabledPuppetsSet.size(); j < nj; ++j)
		{
			CPuppet* pOtherPuppet = m_enabledPuppetsSet[j].GetAIObject();
			if (!pOtherPuppet || !pOtherPuppet->IsEnabled() || pOtherPuppet == pBody || pBody->m_Parameters.m_nSpecies != pOtherPuppet->m_Parameters.m_nSpecies)
				continue;
			if (!CheckVisibilityToBody(pOtherPuppet, pBody, closestDistSqr))
				continue;
			pClosestPuppet = pOtherPuppet;
		}

		if (pClosestPuppet)
		{
			if (CAIGroup* pGroup = GetAIGroup(pBody->GetGroupId()))
				pGroup->NotifyBodyChecked(pBody);
			pClosestPuppet->SetSignal(1, "OnGroupMemberDiedNearest", pBody->GetEntity(), 0, gAIEnv.SignalCRCs.m_nOnGroupMemberDiedNearest);
			pClosestPuppet->SetAlarmed();
			pBody->m_bUncheckedBody = false;
		}
	}
	lastChecked += counter;
}


//
//-----------------------------------------------------------------------------------------------------------
// 
bool CAISystem::CheckVisibilityToBody(CPuppet* pObserver, CAIActor* pBody, float& closestDistSq, IPhysicalEntity* pSkipEnt)
{
	int newFlags = HIT_COVER;
	newFlags |= HIT_SOFT_COVER;

	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	Vec3 bodyPos(pBody->GetPos());

	const Vec3& puppetPos = pObserver->GetPos();
	Vec3 posDiff = bodyPos - puppetPos;
	float	distSq = posDiff.GetLengthSquared();
	if (distSq > closestDistSq)
		return false;

	if (IAIObject::eFOV_Outside == pObserver->IsObjectInFOV(pBody, pObserver->GetParameters().m_PerceptionParams.perceptionScale.visual * 0.75f))
		return false;
/* //Diesel cut bug phisic tank
	//--------------- ACCURATE MEASURING
	float dist = sqrtf(distSq);

	std::vector<IPhysicalEntity*> skipList;
	pObserver->GetPhysicsEntitiesToSkip(skipList);
	pBody->GetPhysicsEntitiesToSkip(skipList);
	if (pSkipEnt)
		skipList.push_back(pSkipEnt);

	bool isVisible = false;
	const RayCastResult& result = gAIEnv.pRayCaster->Cast(RayCastRequest(puppetPos, posDiff, COVER_OBJECT_TYPES, newFlags,
		skipList.empty() ? 0 : &skipList[0], skipList.size()));

	isVisible = !result;

	// check if collider is the body itself
	if (!isVisible)
	{
		const ray_hit& hit = result[0];
		isVisible = hit.pCollider == pBody->GetProxy()->GetPhysics() || hit.pCollider == pBody->GetProxy()->GetPhysics(true);
	}

	if (!isVisible)
		return false;

	closestDistSq = distSq;
*/ //Diesel cut
	return true;
}

