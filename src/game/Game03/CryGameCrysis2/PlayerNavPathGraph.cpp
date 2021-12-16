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
#include "PlayerNavPathGraph.h"


//////////////////////////////////////////////////////////////////////////


CPlayerNavPathGraph::CPlayerNavPathGraph()
{

}



CPlayerNavPathGraph::~CPlayerNavPathGraph()
{

}



bool CPlayerNavPathGraph::IsValid() const
{
	return m_cloud.size()>1;
}



int CPlayerNavPathGraph::GetBestNodeIndex( Vec3& referencePoint ) const
{
	float fDistanceCheck = FLT_MAX;
	int i_nearest = -1;
	const int size = m_cloud.size();
	for(int i=0; i<size; ++i)
	{
		const SNavTagPoint& point = m_cloud[i];
		const float distance = (point.m_pos - referencePoint).GetLengthSquared();
		if(fDistanceCheck > distance)
		{
			fDistanceCheck = distance;
			i_nearest = i;
		}
	}
	return i_nearest;
}



Vec3 CPlayerNavPathGraph::GetNodeReferencePoint( int nodeIndex ) const
{
	return m_cloud[nodeIndex].m_pos;
}



int CPlayerNavPathGraph::GetNumLinks( int nodeIndex ) const
{
	return m_cloud[nodeIndex].m_links.size();
}



int CPlayerNavPathGraph::GetNextNodeIndex( int graphNodeIndex, int linkIndex ) const
{
	return m_cloud[graphNodeIndex].m_links[linkIndex];
}



bool CPlayerNavPathGraph::AddEntityToCloudRecursive(const EntityId entityId, int& outIndex)
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityId);
	if(!pEntity)
		return false;

	const Vec3 pos = pEntity->GetPos();

	int index = -1;
	const int size = m_cloud.size();
	for(int i=0; i<size; ++i)
	{
		if(m_cloud[i].m_pos == pos)
		{
			index = i;
			break;
		}
	}

	if(index==-1)
	{
		index = m_cloud.size();
		m_cloud.push_back(SNavTagPoint(pos, pEntity->GetName()));
	}

	outIndex = index;

	IEntityLink* pLink = pEntity->GetEntityLinks();
	while (pLink)
	{
		int childIndex;

		if(AddEntityToCloudRecursive(pLink->entityId, childIndex))
		{
			LinkNavNodes(outIndex, childIndex);
		}
		pLink = pLink->next;
	}
	return true;
}



void CPlayerNavPathGraph::LinkNavNodes(const int parent, const int child)
{
	SNavTagPoint& parentPoint = m_cloud[parent];
	SNavTagPoint& childPoint = m_cloud[child];

	SNavTagPoint::TLinks::const_iterator it_link = parentPoint.m_links.begin();
	SNavTagPoint::TLinks::const_iterator end_link = parentPoint.m_links.end();
	
	bool add = true;
	for(; it_link!=end_link; ++it_link)
	{
		const int index = (*it_link);
		if(index == child)
			add = false;
	}

	if(add)
	{
		parentPoint.m_links.reserve(parentPoint.m_links.size()+1);
		parentPoint.m_links.push_back(child);
	}

	it_link = childPoint.m_links.begin();
	end_link = childPoint.m_links.end();

	add = true;
	for(; it_link!=end_link; ++it_link)
	{
		const int index = (*it_link);
		if(index == parent)
			add = false;
	}

	if(add)
	{
		childPoint.m_links.reserve(childPoint.m_links.size()+1);
		childPoint.m_links.push_back(parent);
	}
}



void CPlayerNavPathGraph::Recalculate( const EntityId startId /*= 0*/ )
{
	m_cloud.clear();
	int outId;
	AddEntityToCloudRecursive(startId, outId);
}



void CPlayerNavPathGraph::GetClosestNavNode( const Vec3& position, CPlayerNavPathGraph::SNavTagPoint& outPoint ) const
{
	Vec3 pos = position;
	const int best = GetBestNodeIndex(pos);
	outPoint.m_pos = m_cloud[best].m_pos;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


float CPlayerNavPathHeuristics::H( const CPlayerNavPathGraph* graph, int currentNodeIndex, int endNodeIndex ) const
{
	return ( graph->GetNodeReferencePoint(currentNodeIndex) - graph->GetNodeReferencePoint(endNodeIndex) ).GetLengthSquared();
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


float CPlayerNavPathCostCalculator::GetCost( const CPlayerNavPathGraph* graph, int currentNodeIndex, int nextNodeIndex ) const
{
	return ( graph->GetNodeReferencePoint(currentNodeIndex) - graph->GetNodeReferencePoint(nextNodeIndex) ).GetLengthSquared();
}


//////////////////////////////////////////////////////////////////////////