/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Player nav path tag point cloud implementation

-------------------------------------------------------------------------
History:
- 26:01:2010: Created by Jan Neugebauer

*************************************************************************/

#include "StdAfx.h"
#include "PlayerNavPathTagPointCloud.h"

#include "IAISystem.h"

//////////////////////////////////////////////////////////////////////////


CPlayerNavPathTagPointCloud::CPlayerNavPathTagPointCloud()
{

}



CPlayerNavPathTagPointCloud::~CPlayerNavPathTagPointCloud()
{

}



bool CPlayerNavPathTagPointCloud::IsValid() const
{
	return m_cloud.size()>1;
}



void CPlayerNavPathTagPointCloud::Recalculate(const EntityId startId)
{
	m_startEntity = startId;
	uint32 outId;
	AddEntityToCloudRecursive(startId, outId);
}



void CPlayerNavPathTagPointCloud::Clear()
{
	IAISystem* pSystem = gEnv->pAISystem;

	TCloud::const_iterator it = m_cloud.begin();
	TCloud::const_iterator end = m_cloud.end();
	for(; it!=end; ++it)
	{
//		pSystem->RemoveCustomNavNode((*it).m_navNodeId);
	}
	m_cloud.clear();
}



bool CPlayerNavPathTagPointCloud::AddEntityToCloudRecursive(const EntityId entityId, uint32& outNavNodeId)
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityId);
	if(!pEntity)
		return false;

	const Vec3 pos = pEntity->GetPos();
	const uint32 navNodeId = 0;//gEnv->pAISystem->CreateCustomNavNode(pos, NULL, 0, NULL, false);
	outNavNodeId = navNodeId;

	if(!IsAlreadyRegistered(entityId))
		m_cloud.push_back(SNavTagPoint(entityId, pos, navNodeId));

	IEntityLink* pLink = pEntity->GetEntityLinks();
	while (pLink)
	{
		uint32 childNavNodeId;

		if(AddEntityToCloudRecursive(pLink->entityId, childNavNodeId))
		{
			LinkNavNodes(navNodeId, childNavNodeId);
		}
		pLink = pLink->next;
	}
	return true;
}



void CPlayerNavPathTagPointCloud::LinkNavNodes(const uint32 parent, const uint32 child)
{
	TCloud::iterator it_cloud = m_cloud.begin();
	TCloud::const_iterator end_cloud = m_cloud.end();
	for(; it_cloud!=end_cloud; ++it_cloud)
	{
		SNavTagPoint& point = (*it_cloud);
		if(point.m_navNodeId == parent)
		{
			SNavTagPoint::TLinks::const_iterator it_link = point.m_links.begin();
			SNavTagPoint::TLinks::const_iterator end_link = point.m_links.end();
			for(; it_link!=end_link; ++it_link)
			{
				const uint32 id = (*it_link);
				if(id == child)
					return;
			}
			point.m_links.reserve(point.m_links.size()+1);
			point.m_links.push_back(child);
			//gEnv->pAISystem->LinkCustomNavNodes(parent, child, 100.0f, 100.0f);
			return;
		}
	}
}



bool CPlayerNavPathTagPointCloud::IsAlreadyRegistered(const EntityId entityId) const
{
	TCloud::const_iterator it = m_cloud.begin();
	TCloud::const_iterator end = m_cloud.end();
	for(; it!=end; ++it)
	{
		if((*it).m_entityId == entityId)
			return true;
	}
	return false;
}



void CPlayerNavPathTagPointCloud::GetClosestNavNode(const Vec3& position, CPlayerNavPathTagPointCloud::SNavTagPoint& outPoint) const
{
	float fDistanceCheck = FLT_MAX;
	TCloud::const_iterator it = m_cloud.begin();
	TCloud::const_iterator end = m_cloud.end();
	TCloud::const_iterator it_nearest = end;
	for(; it!=end; ++it)
	{
		const SNavTagPoint& point = (*it);
		const float distance = (point.m_pos - position).GetLengthSquared();
		if(fDistanceCheck > distance)
		{
			fDistanceCheck = distance;
			it_nearest = it;
		}
	}
	if(it_nearest != end)
	{
		outPoint.m_entityId = (*it_nearest).m_entityId;
		outPoint.m_pos = (*it_nearest).m_pos;
		outPoint.m_navNodeId = (*it_nearest).m_navNodeId;
	}
}


/*
float CPlayerNavPathTagPointCloud::GetCosts(const void *node1Data, const void *node2Data, const PathfindingHeuristicProperties &pathFindProperties)
{
	
}
*/


//////////////////////////////////////////////////////////////////////////