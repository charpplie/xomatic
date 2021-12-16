/********************************************************************
CryGame Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   Graph.cpp
Version:     v1.00
Description: Implementation of the CGraph class.

-------------------------------------------------------------------------
History:
- ?
- 4 May 2009  : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#include "StdAfx.h"

#include <limits>
#include <algorithm>

#include <ISystem.h>
#include <ISerialize.h>
#include <IRenderAuxGeom.h>
#include <ILog.h>
#include <I3DEngine.h>
#include <ITimer.h>
#include <IConsole.h>
#include <IPhysics.h>
#include <CryFile.h>

#include "Graph.h"
#include "Heuristic.h"
#include "CAISystem.h"
#include "AILog.h"
#include "Cry_Math.h"
#include "ObjectTracker.h"
#include "AIObject.h"
#include "VertexList.h"
#include "TriangularNavRegion.h"
#include "WaypointHumanNavRegion.h"
#include "VolumeNavRegion.h"
#include "FlightNavRegion.h"
#include "RoadNavRegion.h"
#include "SmartObjects.h"
#include "LayeredNavMesh/LayeredNavMeshRegion.h"
#include "GraphLinkManager.h"
#include "GraphNodeManager.h"


#define BAI_TRI_FILE_VERSION 54

// identifier so links can be marked as impassable, then restored
//static const float RADIUS_FOR_BROKEN_LINKS = -121314.0f;

const float AStarSearchNode::fInvalidCost = 999999.0f;

Vec3 CObstacleRef::GetPos() const
{
	CCCPOINT(CObstacleRef_GetPos);

	if (m_refAnchor.IsValid())
	{
		return m_refAnchor.GetAIObject()->GetPos();
	}
	else if (m_pNode)
	{
		return m_pNode->GetPos();
	}
	else if (m_pSmartObject && m_pRule)
	{
		return m_pRule->pObjectHelper ? m_pSmartObject->GetHelperPos( m_pRule->pObjectHelper ) : m_pSmartObject->GetPos();
	}
	else
	{
		AIAssert(m_vertexIndex >= 0);
		return GetAISystem()->m_VertexList.GetVertex(m_vertexIndex).vPos;
	}
}

float CObstacleRef::GetApproxRadius() const
{
	if(m_vertexIndex>=0)
	{
		const ObstacleData& od = GetAISystem()->m_VertexList.GetVertex( m_vertexIndex);
		return od.fApproxRadius;
	}
	else
		return 0;
}

//====================================================================
// ValidateNode
//====================================================================
inline bool CGraph::ValidateNode(unsigned nodeIndex, bool fullCheck) const
{
	const GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);

	if (!nodeIndex)
		return false;
	if (!m_allNodes.DoesNodeExist(nodeIndex))
		return false;
	if (!fullCheck)
		return true;
	else 
		return ValidateNodeFullCheck(pNode);
}

//====================================================================
// ValidateNodeFullCheck
//====================================================================
bool CGraph::ValidateNodeFullCheck(const GraphNode* pNode) const
{
	bool result = true;
	AIAssert(pNode);
	int nNonRoadLinks = 0;
	unsigned nTriLinks = 0;
	for (unsigned linkId = pNode->firstLinkIndex; linkId; linkId = GetLinkManager().GetNextLink(linkId))
	{
		unsigned nextNodeIndex = GetLinkManager().GetNextNode(linkId);
		const GraphNode *next = GetNodeManager().GetNode(nextNodeIndex);
		if (!CGraph::ValidateNode(nextNodeIndex, false))
			result = false;
		if (next->navType != IAISystem::NAV_ROAD)
			++nNonRoadLinks;
		if (next->navType == IAISystem::NAV_TRIANGULAR)
			++nTriLinks;
	}
	if (nNonRoadLinks > 50)
	{
		AIWarning("Too many non-road links (%d) from node %p type %d at (%5.2f, %5.2f, %5.2f)", nNonRoadLinks, pNode, pNode->navType, 
			pNode->GetPos().x, pNode->GetPos().y, pNode->GetPos().z);
	}
	if (pNode->navType == IAISystem::NAV_TRIANGULAR && nTriLinks != 3)
	{
		unsigned numVertices = pNode->GetTriangularNavData()->vertices.size();
		if (numVertices != 3)
		{
			AIWarning("Triangular node at (%5.2f %5.2f %5.2f) does not have 3 vertices", pNode->GetPos().x, pNode->GetPos().y, pNode->GetPos().z);
		}
		else
		{
			Vec3 v0Pos = GetAISystem()->m_VertexList.GetVertex(pNode->GetTriangularNavData()->vertices[0]).vPos;
			Vec3 v1Pos = GetAISystem()->m_VertexList.GetVertex(pNode->GetTriangularNavData()->vertices[1]).vPos;
			Vec3 v2Pos = GetAISystem()->m_VertexList.GetVertex(pNode->GetTriangularNavData()->vertices[2]).vPos;
			int numOut = 0;
			numOut += InsideOfBBox(v0Pos) ? 0 : 1;
			numOut += InsideOfBBox(v1Pos) ? 0 : 1;
			numOut += InsideOfBBox(v2Pos) ? 0 : 1;
			if (numOut < 2)
			{
				AIWarning("Triangular node at (%5.2f %5.2f %5.2f) does not have 3 triangular links (has %d). Triangulate level", 
					pNode->GetPos().x, pNode->GetPos().y, pNode->GetPos().z, numOut);
			}
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//====================================================================
// CGraph
//====================================================================
CGraph::CGraph(CAISystem *pSystem)
:	m_pGraphLinkManager(new CGraphLinkManager()),
	m_pGraphNodeManager(new CGraphNodeManager()),
	m_allNodes(*m_pGraphNodeManager),
	m_triangularBBox(AABB::RESET)
{ 

	m_safeFirstIndex = CreateNewNode(IAISystem::NAV_UNSET, Vec3(ZERO), 0);
	m_firstIndex = m_safeFirstIndex;
	m_pFirst = GetNodeManager().GetNode(m_safeFirstIndex);
	m_pSafeFirst = m_pFirst;
	m_pCurrent = m_pFirst;
	m_currentIndex = m_firstIndex;
	m_pCurrent->firstLinkIndex = 0;
}

//====================================================================
// ~CGraph
//====================================================================
CGraph::~CGraph()
{
	Clear(IAISystem::NAVMASK_ALL);

	delete m_pGraphNodeManager;
	delete m_pGraphLinkManager;
}


//====================================================================
// Clear
//====================================================================
void CGraph::Clear(IAISystem::tNavCapMask navTypeMask)
{
	ClearMarks();
	DeleteGraph(navTypeMask);
	if (navTypeMask & (IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE))
	{
		EntranceMap::iterator next;
		for (EntranceMap::iterator it = m_mapEntrances.begin() ; it != m_mapEntrances.end() ; it = next )
		{
			next = it; ++next;
			GraphNode* pNode = GetNodeManager().GetNode(it->second);
			if (pNode->navType & navTypeMask)
				m_mapEntrances.erase(it);
		}
		for (EntranceMap::iterator it = m_mapExits.begin() ; it != m_mapExits.end() ; it = next)
		{
			next = it; ++next;
			GraphNode* pNode = GetNodeManager().GetNode(it->second);
			if (pNode->navType & navTypeMask)
				m_mapExits.erase(it);
		}
	}

	GetNodeManager().Clear(navTypeMask);

	if (m_pSafeFirst)
	{
		Disconnect(m_safeFirstIndex, false);
	}
	else
	{
		m_safeFirstIndex = CreateNewNode(IAISystem::NAV_UNSET, Vec3(ZERO), 0);
		m_pSafeFirst = GetNodeManager().GetNode(m_safeFirstIndex);
	}
	m_pFirst = m_pSafeFirst;
	m_firstIndex = m_safeFirstIndex;
	m_pCurrent = m_pFirst;
	m_currentIndex = m_firstIndex;
}

//====================================================================
// GetEnclosing
//====================================================================
unsigned int CGraph::GetEnclosing(const Vec3 &pos,IAISystem::tNavCapMask navCapMask, float passRadius, unsigned startIndex,
																float range, Vec3 * closestValid, bool returnSuspect, const char *requesterName)
{
	IVisArea *pGoalArea; 
	int	nGoalBuilding;

#ifdef _DEBUG
	// Sanity check for LNM usage
	if (gAIEnv.pNavigation->GetLayeredNavMeshRegion()->IsAnyNavMeshLoaded())
	{
		// If agent is LNM capable but has no LNM capabilities set
		if ((navCapMask & IAISystem::NAV_LAYERED_NAV_MESH) && (navCapMask.GetLnmCaps() == 0))
		{
			AIError("CGraph::GetEnclosing() called by %s using LNM but without any LNM capabilities set [Code bug]", requesterName);
		}
	}
#endif

	IAISystem::ENavigationType navType = gAIEnv.pNavigation->CheckNavigationType(pos,nGoalBuilding,pGoalArea,navCapMask);

	if (!(navType & navCapMask))
		return 0;

  unsigned nodeIndex = 0;

  // in some cases prefer roads over everything. In other cases only check roads at the end
  bool checkRoadsFirst = !(navCapMask & IAISystem::NAV_VOLUME);
  if (checkRoadsFirst && navCapMask & IAISystem::NAV_ROAD)
	{
		nodeIndex = gAIEnv.pNavigation->GetRoadNavRegion()->GetEnclosing(pos, passRadius, startIndex, range, closestValid, returnSuspect, requesterName);
		if (nodeIndex)
			return nodeIndex;
	}
	switch(navType)
	{
	case IAISystem::NAV_TRIANGULAR:
	case IAISystem::NAV_WAYPOINT_HUMAN:
	case IAISystem::NAV_WAYPOINT_3DSURFACE:
	case IAISystem::NAV_FLIGHT:
	case IAISystem::NAV_VOLUME:
	case IAISystem::NAV_ROAD:
	case IAISystem::NAV_FREE_2D:
		nodeIndex = gAIEnv.pNavigation->GetNavRegion(navType, this)->GetEnclosing(pos, passRadius, startIndex, range, closestValid, returnSuspect, requesterName);
    break;
	case IAISystem::NAV_LAYERED_NAV_MESH:
	{
		// NOTE Aug 14, 2009: <pvl> unfortunately, we need to downcast here.  I
		// don't see any good way of stuffing LNM behind NavRegion::GetEnclosing()
		// signature.  Crucially, LNM nav region needs to know the agent type.
		CCompositeLayeredNavMeshRegion * lnmRegion =
				static_cast <CCompositeLayeredNavMeshRegion*> (gAIEnv.pNavigation->GetNavRegion(navType, this));
		// NOTE Aug 14, 2009: <pvl> only casting to aid overload resolution
		nodeIndex = lnmRegion->GetEnclosing(pos, (int )navCapMask.GetLnmCaps (), range);
	    break;
	}
	case IAISystem::NAV_CUSTOM_NAVIGATION:
		nodeIndex = gAIEnv.pNavigation->GetNavRegion(IAISystem::NAV_CUSTOM_NAVIGATION, this)->GetEnclosing(pos, passRadius, startIndex, range, closestValid, returnSuspect, requesterName);
		break;
  default:
    AIError("CGraph::GetEnclosing Unhandled navigation type: %d %s [Code bug]", navType, requesterName);
    return 0;
	}
  if (!nodeIndex && !checkRoadsFirst && navCapMask & IAISystem::NAV_ROAD)
  {
    nodeIndex = gAIEnv.pNavigation->GetRoadNavRegion()->GetEnclosing(pos, passRadius, startIndex, range, closestValid, returnSuspect, requesterName);
  }
  return nodeIndex;
}



//====================================================================
// Connect
//====================================================================
void CGraph::Connect(unsigned oneNodeIndex, unsigned twoNodeIndex, 
										 float radiusOneToTwo, float radiusTwoToOne,
										 unsigned* pLinkOneTwo, unsigned* pLinkTwoOne)
{
	GraphNode* one = GetNodeManager().GetNode(oneNodeIndex);
	GraphNode* two = GetNodeManager().GetNode(twoNodeIndex);

	if (pLinkOneTwo)
		*pLinkOneTwo = 0;
	if (pLinkTwoOne)
		*pLinkTwoOne = 0;

	if (one==two)
		return;

	if (!one || !two)
		return;

	if (!CGraph::ValidateNode(oneNodeIndex, false) || !CGraph::ValidateNode(twoNodeIndex, false))
	{
		AIError("CGraph::Connect Attempt to connect nodes that aren't created [Code bug]");
		return;
	}

	if ((one == m_pSafeFirst || two == m_pSafeFirst) && m_pSafeFirst->firstLinkIndex)
	{
		AIWarning("Second link being made to safe/first node");
		return;
	}

#ifdef CRYAISYSTEM_DEBUG
	extern std::vector<const GraphNode *> g_DebugGraphNodesToDraw;
	g_DebugGraphNodesToDraw.clear();
#endif //CRYAISYSTEM_DEBUG

	// handle case where they're already connected
	unsigned linkIndexOne = one->GetLinkTo(GetNodeManager(), GetLinkManager(), two);
	unsigned linkIndexTwo = two->GetLinkTo(GetNodeManager(), GetLinkManager(), one);

	if ( (linkIndexOne == 0) != (linkIndexTwo == 0) )
	{
		AIWarning("Trying to connect links but one is already connected, other isn't");
		return;
	}

	// Check that if both links have bidirectional data, then it is the same.
	assert(linkIndexOne == 0 || linkIndexTwo == 0 || (linkIndexOne & ~1) == (linkIndexTwo & ~1));

	// Create new bidirectional data if necessary.
	unsigned linkIndex = linkIndexOne;
	if (!linkIndex && linkIndexTwo)
		linkIndex = linkIndexTwo ^ 1;
	if (!linkIndex)
		linkIndex = m_pGraphLinkManager->CreateLink();

	Vec3 midPos = 0.5f * (one->GetPos() + two->GetPos());

	if (!linkIndexOne)
	{
		// [1/3/2007 MichaelS] Should be possible to push link on front, but I don't want to run the risk
		// of breaking code that relies on the link order being preserved, so we add it to the end.
		//one->links.push_back(linkIndex);
		if (!one->firstLinkIndex)
			one->firstLinkIndex = linkIndex;
		else
		{
			unsigned lastLink, nextLink;
			for (lastLink = one->firstLinkIndex; nextLink = m_pGraphLinkManager->GetNextLink(lastLink); lastLink = nextLink);
			m_pGraphLinkManager->SetNextLink(lastLink, linkIndex);
		}

		linkIndexOne = linkIndex;
		GetLinkManager().SetNextNode(linkIndex, twoNodeIndex);
		GetLinkManager().SetRadius(linkIndex, two == m_pSafeFirst ? -1.0f : radiusOneToTwo);
		GetLinkManager().GetEdgeCenter(linkIndex) = midPos;
		two->AddRef();
	}
	else
	{
		if (radiusOneToTwo != 0.0f)
			GetLinkManager().ModifyRadius(linkIndexOne, radiusOneToTwo);
	}
	if (!linkIndexTwo)
	{
		// [1/3/2007 MichaelS] Should be possible to push link on front, but I don't want to run the risk
		// of breaking code that relies on the link order being preserved, so we add it to the end.
		//one->links.push_back(linkIndex);
		if (!two->firstLinkIndex)
			two->firstLinkIndex = linkIndex ^ 1;
		else
		{
			unsigned lastLink, nextLink;
			for (lastLink = two->firstLinkIndex; nextLink = m_pGraphLinkManager->GetNextLink(lastLink); lastLink = nextLink);
			m_pGraphLinkManager->SetNextLink(lastLink, linkIndex ^ 1);
		}

		linkIndexTwo = linkIndex ^ 1;
		GetLinkManager().SetNextNode(linkIndex ^ 1, oneNodeIndex);
		GetLinkManager().SetRadius(linkIndex ^ 1, one == m_pSafeFirst ? -1.0f : radiusTwoToOne);
		GetLinkManager().GetEdgeCenter(linkIndex ^ 1) = midPos;
		one->AddRef();
	}
	else
	{
		if (radiusTwoToOne != 0.0f)
			GetLinkManager().ModifyRadius(linkIndexTwo, radiusTwoToOne);
	}

	if (pLinkOneTwo && linkIndexOne)
		*pLinkOneTwo = linkIndexOne;
	if (pLinkTwoOne && linkIndexTwo)
		*pLinkTwoOne = linkIndexTwo;

	if (m_pSafeFirst->firstLinkIndex)
		return;

	if (one->navType == IAISystem::NAV_TRIANGULAR  || one->navType == IAISystem::NAV_LAYERED_NAV_MESH)
	{
		Connect(m_safeFirstIndex,oneNodeIndex, 100.0f, -1.0f);
		m_pFirst = one;
		m_firstIndex = oneNodeIndex;
	}
	else if (two->navType == IAISystem::NAV_TRIANGULAR || two->navType == IAISystem::NAV_LAYERED_NAV_MESH)
	{
		Connect(m_safeFirstIndex,twoNodeIndex, 100.0f, -1.0f);
		m_pFirst = two;
		m_firstIndex = twoNodeIndex;
	}
	// may have incurred a reallocation
	// [1/3/2007 MichaelS] Should no longer be necessary since we use indices instead of pointers, but leaving it here for now.
	if (pLinkOneTwo && linkIndexOne)
		*pLinkOneTwo = linkIndexOne;
	if (pLinkTwoOne && linkIndexTwo)
		*pLinkTwoOne = linkIndexTwo;
}

//====================================================================
// DeleteGraph
//====================================================================
void CGraph::DeleteGraph(IAISystem::tNavCapMask navTypeMask)
{
	std::vector<unsigned> nodesToDelete;
	CAllNodesContainer::Iterator it(m_allNodes, navTypeMask);
	const unsigned lnmData = navTypeMask.GetLnmCaps();
	while (unsigned nodeIndex = it.Increment())
	{
		GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);
		if (lnmData)
		{
			AIAssert (pNode->navType == IAISystem::NAV_LAYERED_NAV_MESH);
			if (pNode->GetLayeredMeshNavData()->navModifIndex != lnmData)
				continue;
		}
		else
		{
			AIAssert(pNode->navType & navTypeMask);
		}
		nodesToDelete.push_back(nodeIndex);
	}

	for (unsigned i = 0 ; i < nodesToDelete.size() ; ++i)
	{
		GraphNode* pNode = GetNodeManager().GetNode(nodesToDelete[i]);
		Disconnect(nodesToDelete[i]);
		if (pNode == m_pSafeFirst)
		{
			m_pSafeFirst = 0;
			m_safeFirstIndex = 0;
		}
	}
}

//====================================================================
// Disconnect
//====================================================================
void CGraph::Disconnect(unsigned nodeIndex, unsigned linkId)
{
	GraphNode* pNode = m_pGraphNodeManager->GetNode(nodeIndex);

	if (!CGraph::ValidateNode(nodeIndex, false))
	{
		AIError("CGraph::Disconnect Attempt to disconnect link from node that isn't created [Code bug]");
		return;
	}
	AIAssert(linkId);
	unsigned otherNodeIndex = GetLinkManager().GetNextNode(linkId);
	GraphNode* pOtherNode = GetNodeManager().GetNode(otherNodeIndex);
	if (!CGraph::ValidateNode(otherNodeIndex, false))
	{
		AIError("CGraph::Disconnect Attempt to disconnect link from other node that isn't created [Code bug]");
		return;
	}

	pNode->RemoveLinkTo(GetLinkManager(), otherNodeIndex);
	pNode->Release();

	pOtherNode->RemoveLinkTo(GetLinkManager(), nodeIndex);
	pOtherNode->Release();

	GetLinkManager().DestroyLink(linkId);
}

//====================================================================
// Disconnect
//====================================================================
void CGraph::Disconnect(unsigned nodeIndex, bool bDelete)
{
	GraphNode* pDisconnected = m_pGraphNodeManager->GetNode(nodeIndex);

	if (!CGraph::ValidateNode(nodeIndex, false))
	{
		AIError("CGraph::Disconnect Attempt to disconnect node that isn't created [Code bug]");
		return;
	}

#ifdef CRYAISYSTEM_DEBUG
	extern std::vector<const GraphNode *> g_DebugGraphNodesToDraw;
	g_DebugGraphNodesToDraw.clear();
#endif //CRYAISYSTEM_DEBUG

	// if the node we are disconnecting is the current node, move the current 
	// to one of his links, or the root if it has no links
	if (pDisconnected == m_pCurrent)
	{
		if (pDisconnected->firstLinkIndex)
		{
			m_currentIndex = GetLinkManager().GetNextNode(pDisconnected->firstLinkIndex);
			m_pCurrent = GetNodeManager().GetNode(m_currentIndex);
		}
		else
		{
			m_currentIndex = m_safeFirstIndex;
			m_pCurrent = m_pSafeFirst;
		}
	}

	// if its the root that is being disconnected, move it
	if (m_pFirst == pDisconnected)
	{
		if (pDisconnected->firstLinkIndex)
		{
			m_firstIndex = GetLinkManager().GetNextNode(pDisconnected->firstLinkIndex);
			m_pFirst = GetNodeManager().GetNode(m_firstIndex);
		}
		else
		{
			if (m_pFirst!=m_pSafeFirst)
			{
				m_pFirst = m_pSafeFirst;
				m_firstIndex = m_safeFirstIndex;
			}
			else
			{
				m_pFirst = 0;
				m_firstIndex = 0;
			}
		}
	}

	// now disconnect this node from its links
	for (unsigned link = pDisconnected->firstLinkIndex, nextLink; link; link = nextLink)
	{
		nextLink = GetLinkManager().GetNextLink(link);

		unsigned nextNodeIndex = GetLinkManager().GetNextNode(link);
		GraphNode *pNextNode = GetNodeManager().GetNode(nextNodeIndex);
		pNextNode->RemoveLinkTo(GetLinkManager(), nodeIndex);
		pNextNode->Release();
		pDisconnected->Release();
		GetLinkManager().DestroyLink(link);
	}

	pDisconnected->firstLinkIndex = 0;

	if (pDisconnected->nRefCount != 1)
		AIWarning("Node reference count is not 1 after disconnecting");

	if (bDelete)
		DeleteNode(nodeIndex);

	if (!m_pSafeFirst)
		return;

	if (pDisconnected != m_pSafeFirst && !m_pSafeFirst->firstLinkIndex)
	{
		unsigned firstIndex = m_firstIndex;
		// we have disconnected the link to the dummy safe node - relink it to any outdoor node of the graph
		if (firstIndex == m_safeFirstIndex)
		{
			if (m_currentIndex == m_safeFirstIndex)
			{
				// try any entrance
				if (!m_mapEntrances.empty())
					firstIndex = (m_mapEntrances.begin()->second);
				else
					return; // m_pSafeFirst links will stay empty

			}
			else
			{
				firstIndex = m_currentIndex;
			}
		}

		if (firstIndex)
		{
			GraphNode* pFirst = GetNodeManager().GetNode(firstIndex);
			if (pFirst->navType == IAISystem::NAV_TRIANGULAR)	
				Connect(m_safeFirstIndex,firstIndex, 100.0f, -1.0f);
			else if (pFirst->navType == IAISystem::NAV_WAYPOINT_HUMAN)	
			{
				GraphNode *pEntrance = GetEntrance(pFirst->GetWaypointNavData()->nBuildingID,Vec3(0,0,0));
				if (pEntrance)
				{
					for (unsigned link = pEntrance->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
					{
						unsigned nextNodeIndex = GetLinkManager().GetNextNode(link);
						GraphNode* pNextNode = GetNodeManager().GetNode(nextNodeIndex);
						if ( pNextNode->navType == IAISystem::NAV_WAYPOINT_HUMAN )
						{
							Connect(m_safeFirstIndex,nextNodeIndex, 100.0f, -1.0f);
							break;
						}
					}
				}
			}
		}
	}
}

bool operator<(const Vec3r &v1, const Vec3r &v2)
{
	if (v1.x < v2.x)
		return true;
	else if (v1.x > v2.y)
		return false;
	if (v1.y < v2.y)
		return true;
	else if (v1.y > v2.y)
		return false;
	if (v1.z < v2.z)
		return true;
	else if (v1.z > v2.z)
		return false;
	return false;
}

//====================================================================
// PointInTriangle
// Check whether a position is within a node's triangle
//====================================================================
bool PointInTriangle(const Vec3 & pos, GraphNode * pNode)
{
	AIAssert(pNode);
	if (!pNode || pNode->navType != IAISystem::NAV_TRIANGULAR || pNode->GetTriangularNavData()->vertices.empty())
		return false;
	unsigned numV = pNode->GetTriangularNavData()->vertices.size();

#define CROSS2D(vec1, vec2) (vec1.x * vec2.y - vec1.y * vec2.x)

	// don't know the winding of the triangle, so just require all the results to be 
	// the same. 0 Means there wasn't a prev result, or it was on the edge
	int prevResult = 0;

	// count points close to the boundary as inside the triangle
	const real tol = 0.00000000001;

	for (unsigned i = 0 ; i < numV ; ++i)
	{
		unsigned iNext = (i + 1) % numV;
		int index1 = pNode->GetTriangularNavData()->vertices[i];
		int index2 = pNode->GetTriangularNavData()->vertices[iNext];

		Vec3r v1 = GetAISystem()->m_VertexList.GetVertex(index1).vPos;
		Vec3r v2 = GetAISystem()->m_VertexList.GetVertex(index2).vPos;

		// ensure that the test is consistent for adjacent triangles
		bool swapped = false;
		if (v1 < v2)
		{
			std::swap(v1, v2);
			swapped = true;
		}

		Vec3r edge = v2 - v1;
		Vec3r dir = pos - v1;
		// inside if edge x dir is up... but only in twoD
		real cross = CROSS2D(edge, dir);

		if (swapped)
			cross = -cross;

		int result;
		if (cross > tol)
			result = 1;
		else if (cross < -tol)
			result = 2;
		else 
			result = 0;

		if (prevResult == 1 && result == 2)
			return false;
		else if (prevResult == 2 && result == 1)
			return false;

		if (result != 0)
			prevResult = result;
	}
	// if prevResult == 0 then it means all our results were 0 - bloomin wierd,
	// but we would still want to return true
	// AIAssert(prevResult);
	return true;
#undef CROSS2D
}

//#define VALIDATEINMARK

//====================================================================
// MarkNode
// uses mark for internal graph operation without disturbing the pathfinder
//====================================================================
void CGraph::MarkNode(unsigned nodeIndex) const
{
#ifdef VALIDATEINMARK
	if (!CGraph::ValidateNode(nodeIndex))
	{
		AIError("CGraph::MarkNode Unable to validate/mark node %p [Code bug]", pNode);
		return;
	}
#endif
	GetNodeManager().GetNode(nodeIndex)->mark = 1;
	m_markedNodes.push_back(nodeIndex);
}

//====================================================================
// ClearMarks
// clears the marked nodes
//====================================================================
void CGraph::ClearMarks() const
{
	while (!m_markedNodes.empty())
	{
#ifdef VALIDATEINMARK
		if (!CGraph::ValidateNode(m_markedNodes.back()))
			AIError("CGraph::ClearMarks Unable to validate/clear mark node %p [Code bug]", m_markedNodes.back());
		else
#endif
			GetNodeManager().GetNode(m_markedNodes.back())->mark = 0;
		m_markedNodes.pop_back();
	}
}

//====================================================================
// ReadFromFile
// Reads the AI graph from a specified file
//====================================================================
bool CGraph::ReadFromFile(const char * szName, int lnmModif)
{
	CCryFile file;;
	if (file.Open( szName,"rb"))
		return ReadNodes( file, lnmModif );
	else
		return false;
}

//====================================================================
// ReadNodes
// reads all the nodes in a map
// UPDATE Oct 27, 2009: <pvl> it now only reads all non-LNM nodes in
// a map.  What happens to LNM nodes depends on 'lnmModif':
// 0 - zero means LNM nodes will be ignored
// 0xffffffff - all f's mean all LNM nodes will be loaded
// positive integer in between - only LNM nodes belonging to that modifier
//     will be loaded
//====================================================================
bool CGraph::ReadNodes( CCryFile &file, int lnmModif )
{
	NodeDescBuffer nodeDescs;

	//AIAssert(_heapchk()==_HEAPOK);
	int iNumber;
	Vec3 mins,maxs;

	AILogLoading("Verifying BAI file version");
	file.ReadType(&iNumber);
	if (iNumber != BAI_TRI_FILE_VERSION)
	{
		AIError("CGraph::ReadNodes Wrong triangulation BAI file version - found %d expected %d: Regenerate triangulation in the editor [Design bug]", iNumber, BAI_TRI_FILE_VERSION);
		return false;
	}

	AILogLoading("Reading BBOX");
	file.ReadType( &mins );
	file.ReadType( &maxs );

	SetBBox(mins,maxs);

	AILogLoading("Reading node descriptors");

	file.ReadType( &iNumber );

	if (iNumber>0) 
	{
		nodeDescs.resize(iNumber);
		file.ReadType( &nodeDescs[0], iNumber );
	}

	AILogLoading("Creating graph nodes");

	typedef std::map< unsigned, unsigned > ReadNodesMap;
	ReadNodesMap mapReadNodes;

	I3DEngine *pEngine = gEnv->p3DEngine;
	NodeDescBuffer::iterator ni;
	int index=0;

	if (/*!gEnv->IsEditor() && */iNumber)
	{
		//int nodeCounts[IAISystem::NAV_TYPE_COUNT] = {0};

		NodeDescBuffer::iterator end = nodeDescs.end();
		//for (ni=nodeDescs.begin();ni != end;++ni,++index)
		//{
		//	NodeDescriptor desc = (*ni);
		//	IAISystem::ENavigationType type = (IAISystem::ENavigationType) desc.navType;

		//	int typeIndex = TypeIndexFromType(type);

		//	// in editor waypoint nodes get added by the editor
		//	// MichaelS - removed check to improve stats consistency.
		//	if (typeIndex < 0 ||
		//		(/*type != IAISystem::NAV_TRIANGULAR || */type != IAISystem::NAV_UNSET && gEnv->IsEditor()))
		//		continue;

		//	++nodeCounts[typeIndex];
		//}

		for (ni=nodeDescs.begin();ni != end;++ni,++index)
		{
			NodeDescriptor desc = (*ni);
			IAISystem::ENavigationType type = (IAISystem::ENavigationType) desc.navType;

			// NOTE Oct 20, 2009: <pvl> we can only create a LNM node once we know
			// its LNM-specific data (modifier index in particular)
			unsigned int nodeIndex = 0;
			GraphNode *pNode = 0;
			if ( ! (type & IAISystem::NAV_LAYERED_NAV_MESH) )
			{
				nodeIndex = CreateNewNode(type, desc.pos, desc.ID);
				pNode = GetNodeManager().GetNode(nodeIndex);
			}

			switch (type)
			{
			case IAISystem::NAV_UNSET:
				break;
			case IAISystem::NAV_TRIANGULAR:
				{
					ClearVectorMemory(pNode->GetTriangularNavData()->vertices);
					int nObstacles = 0;
					if (desc.obstacle[2] >= 0)
						nObstacles = 3;
					else if (desc.obstacle[1] >= 0)
						nObstacles = 2;
					else if (desc.obstacle[0] >= 0)
						nObstacles = 1;
					if (nObstacles)
					{
						pNode->GetTriangularNavData()->vertices.reserve(nObstacles);
						for (int i=0 ; i < nObstacles ; i++)
						{
							int nVertexIndex = desc.obstacle[i];
							pNode->GetTriangularNavData()->vertices.push_back(nVertexIndex);
              if (!GetAISystem()->m_VertexList.IsIndexValid(nVertexIndex))
                AIError("Invalid obstacle index %d on loading", nVertexIndex);
						}
					}
					pNode->GetTriangularNavData()->isForbidden = desc.bForbidden;
					pNode->GetTriangularNavData()->isForbiddenDesigner = desc.bForbiddenDesigner;
				}
				break;
			case IAISystem::NAV_WAYPOINT_3DSURFACE:
			case IAISystem::NAV_WAYPOINT_HUMAN:
				{
					pNode->GetWaypointNavData()->type = (EWaypointNodeType) desc.type;

					// building id isn't preserved
					pNode->GetWaypointNavData()->nBuildingID = -1;
					pNode->GetWaypointNavData()->pArea = 0;
					SpecialArea::EType areaType = pNode->navType == IAISystem::NAV_WAYPOINT_HUMAN ? SpecialArea::TYPE_WAYPOINT_HUMAN : SpecialArea::TYPE_WAYPOINT_3DSURFACE;
					const SpecialArea *sa = gAIEnv.pNavigation->GetSpecialArea(pNode->GetPos(), areaType);
					if (sa)
					{
						pNode->GetWaypointNavData()->nBuildingID = sa->nBuildingID;
						I3DEngine *p3dEngine=gEnv->p3DEngine;
						pNode->GetWaypointNavData()->pArea = p3dEngine->GetVisAreaFromPos(pNode->GetPos());
					}

					if (desc.type == WNT_ENTRYEXIT)
						m_mapEntrances.insert(EntranceMap::iterator::value_type(pNode->GetWaypointNavData()->nBuildingID, nodeIndex));

					if (desc.type == WNT_ENTRYEXIT || desc.type == WNT_EXITONLY)
						m_mapExits.insert(EntranceMap::iterator::value_type(pNode->GetWaypointNavData()->nBuildingID, nodeIndex));

					// NOTE Oct 15, 2009: <pvl> removable nodes have been unused for
					// a very long time now, removing support altogether
					assert ( ! desc.bRemovable);

					pNode->GetWaypointNavData()->dir = desc.dir;
					pNode->GetWaypointNavData()->up = desc.up;
				}
				break;
			case IAISystem::NAV_FLIGHT:
				AIAssert(!"flight nav should be loaded separately");
				pNode->GetFlightNavData()->nSpanIdx = desc.index;
				break;
			case IAISystem::NAV_VOLUME:
				AIAssert(!"volume nav should be loaded separately");
				pNode->GetVolumeNavData()->nVolimeIdx = desc.index;
				break;
			case IAISystem::NAV_ROAD:
				AIAssert(!"road nav should be loaded separately");
				pNode->GetRoadNavData()->fRoadWidth = desc.dir.x;
				/*pNode->GetRoadNavData()->fRoadOffset = desc.dir.y;*/
				break;
			case IAISystem::NAV_SMARTOBJECT:
				AIAssert(!"smart object nav should be loaded separately");
				break;
			case IAISystem::NAV_LAYERED_NAV_MESH:
			{
				// FIXME Oct 20, 2009: <pvl> a clumsy way of extracting modifier
				// index before the actual node is created
				SLayeredMeshNavData dummy;
				dummy.SetFromPacked (desc.index);

				// NOTE Oct 27, 2009: <pvl> load node only if it belongs to the
				// right modifier or if we're instructed to load all nodes
				if (lnmModif == ~0 || dummy.navModifIndex == lnmModif)
				{
					// FIXME Oct 20, 2009: <pvl> we're misusing here the "LNM bits" of
					// NavCapMask - they are supposed to carry agent type but now we're
					// storing modifier index in there
					IAISystem::tNavCapMask mask (type);
					// FIXME Oct 20, 2009: <pvl> this would be cleaner if there was
					// an explicit function along the lines of SetLnmBits()
					mask.SetLnmCaps (dummy.navModifIndex);

					nodeIndex = CreateNewNode(mask, desc.pos, desc.ID);
					pNode = GetNodeManager().GetNode(nodeIndex);

					pNode->GetLayeredMeshNavData()->SetFromPacked (desc.index);
				}
			}
				break;
			default:
				AIError("CGraph::ReadNodes Unhandled nav type %d", pNode->navType);
			}
			mapReadNodes.insert(ReadNodesMap::value_type(desc.ID, nodeIndex));

		}

	}

	AILogLoading("Reading links");

	LinkDescBuffer linkDescs;

	file.ReadType( &iNumber );
	if (iNumber>0) 
	{
		linkDescs.resize(iNumber);
		file.ReadType( &linkDescs[0], iNumber );
	}

	ReadNodesMap::iterator ei,link;

	ei = mapReadNodes.find(1);
	if (ei==mapReadNodes.end())
	{
		AIError("CGraph::ReadNodes First node not found - navigation loading failed [code bug]");
		gEnv->pLog->UpdateLoadingScreen(" ");
		return false;
	}
	else
	{
		if (m_pSafeFirst)
			Disconnect(m_safeFirstIndex);
		m_safeFirstIndex = (ei->second);
		m_pSafeFirst = GetNodeManager().GetNode(m_safeFirstIndex);
		m_pFirst = m_pSafeFirst;
		m_firstIndex = m_safeFirstIndex;
		m_pCurrent = m_pSafeFirst;
		m_currentIndex = m_safeFirstIndex;
	}


	float fStartTime = gEnv->pTimer->GetAsyncCurTime();
	AILogLoading("Reconnecting links");

	if (!linkDescs.empty())
	{
		LinkDescBuffer::iterator iend = linkDescs.end();
		for (LinkDescBuffer::iterator ldbi=linkDescs.begin();ldbi!=iend;++ldbi)
		{
			LinkDescriptor &ldb = (*ldbi);

			ei=mapReadNodes.find((unsigned)ldb.nSourceNode);
			if (ei == mapReadNodes.end())
			{
#if defined __GNUC__
				AIWarning( "NodeId %lld not found in node map",(long long)ldb.nSourceNode );
#else
				AIWarning( "NodeId %I64d not found in node map",(int64)ldb.nSourceNode );
#endif
			}
			else
			{
				unsigned nodeIndex = ei->second;
				GraphNode *pNode = GetNodeManager().GetNode(nodeIndex);
				link = mapReadNodes.find((unsigned int)ldb.nTargetNode);
				if (link==mapReadNodes.end() || ei==mapReadNodes.end())
				{
					AIError("CGraph::ReadNodes Read a link to a node which could not be found [Code bug]");
				}
				else
				{
					if (pNode != m_pSafeFirst && link->second != m_safeFirstIndex)
					{
						unsigned linkOneTwo = 0;
						unsigned linkTwoOne = 0;
						// incoming link gets set when the other way is read
						Connect(nodeIndex,link->second, ldb.fMaxPassRadius, 0.0f, &linkOneTwo, &linkTwoOne);
						if (linkOneTwo)
						{
							unsigned nextNodeIndex = link->second;
							GraphNode* pNextNode = GetNodeManager().GetNode(nextNodeIndex);
							AIAssert(GetLinkManager().GetNextNode(linkOneTwo) == nextNodeIndex);
							if ( GetLinkManager().GetNextNode(linkOneTwo) == link->second )
							{
								GetLinkManager().SetRadius(linkOneTwo, ldb.fMaxPassRadius);
								GetLinkManager().SetStartIndex(linkOneTwo, ldb.nStartIndex);
								GetLinkManager().SetEndIndex(linkOneTwo, ldb.nEndIndex);
								GetLinkManager().GetEdgeCenter(linkOneTwo) = ldb.vEdgeCenter; // TODO: Don't read shared value twice
								GetLinkManager().SetExposure(linkOneTwo, ldb.fExposure); // TODO: Don't read shared value twice
								GetLinkManager().SetMaxWaterDepth(linkOneTwo, ldb.fMaxWaterDepth);
								GetLinkManager().SetMinWaterDepth(linkOneTwo, ldb.fMinWaterDepth);
								GetLinkManager().SetSimple(linkOneTwo, ldb.bSimplePassabilityCheck);
							}
							if (linkTwoOne)
								GetLinkManager().RestoreLink(linkTwoOne);
						}
					}
				}
			}
		}
	}

	AILogLoading("Finished Reconnecting links in %6.3f sec",
		gEnv->pTimer->GetAsyncCurTime() - fStartTime);

  mapReadNodes.clear();
	//AIAssert(_heapchk()==_HEAPOK);

	return true;
}

//====================================================================
// SetBBox
// defines bounding rectangle of this graph
//====================================================================
void CGraph::SetBBox(const Vec3 & min, const Vec3 & max)
{
	m_triangularBBox.min = min;
	m_triangularBBox.max = max;
}

//====================================================================
// InsideOfBBox
//====================================================================
bool CGraph::InsideOfBBox(const Vec3 & pos) const
{
	return pos.x > m_triangularBBox.min.x && pos.x < m_triangularBBox.max.x && pos.y > m_triangularBBox.min.y && pos.y < m_triangularBBox.max.y;
}

unsigned CGraph::CreateNewNode(IAISystem::tNavCapMask type, const Vec3 &pos, unsigned ID)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Navigation, 0, "Graph Node (%s)", StringFromTypeIndex(TypeIndexFromType(type)));
	//GraphNode *pNode = new GraphNode(type, pos, ID);
	//GraphNode *pNode = NodesPool.AddGraphNode(type, pos, ID); 
	unsigned nodeIndex = m_pGraphNodeManager->CreateNode(type, pos, ID);
	GraphNode* pNode = m_pGraphNodeManager->GetNode(nodeIndex);
	pNode->AddRef();
	m_allNodes.AddNode(nodeIndex);

	if(type != IAISystem::NAV_UNSET)
	{
		CNavRegion * pNavRegion = gAIEnv.pNavigation->GetNavRegion(pNode->navType, this);
		//?? can comment out? I think so.  if (pNavRegion)
			pNavRegion->NodeCreated(nodeIndex);
	}

	return nodeIndex;
}

//====================================================================
// GetNode
//====================================================================
GraphNode* CGraph::GetNode(unsigned index)
{
	return GetNodeManager().GetNode(index);
}

//====================================================================
// MoveNode
//====================================================================
void CGraph::MoveNode(unsigned nodeIndex, const Vec3 &newPos)
{
	GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);
	if (pNode->GetPos().IsEquivalent(newPos))
		return;
	m_allNodes.RemoveNode(nodeIndex);
	pNode->SetPos(newPos);
	m_allNodes.AddNode(nodeIndex);

	CNavRegion * pNavRegion = gAIEnv.pNavigation->GetNavRegion(pNode->navType, this);
	if (pNavRegion)
		pNavRegion->NodeMoved(nodeIndex);
}

struct SNodeFinder
{
	SNodeFinder(unsigned nodeIndex) : nodeIndex(nodeIndex) {}
	bool operator()(const EntranceMap::value_type& val) const {return val.second == nodeIndex;}
	unsigned nodeIndex;
};

//====================================================================
// DeleteNode
//====================================================================
void CGraph::DeleteNode(unsigned nodeIndex)
{
	GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);

	if (!CGraph::ValidateNode(nodeIndex, false))
	{
		AIError("CGraph::DeleteNode Attempting to delete node that doesn't exist %p [Code bug]", pNode);
		return;
	}

	if (pNode->firstLinkIndex)
	{
		AIWarning("Deleting node %p but it is still connected - disconnecting", pNode);
		Disconnect(nodeIndex, false);
	}

	if (pNode->Release())
	{
		CNavRegion * pNavRegion = gAIEnv.pNavigation->GetNavRegion(pNode->navType, this);
		if (pNavRegion)
			pNavRegion->NodeAboutToBeDeleted(pNode);

		m_allNodes.RemoveNode(nodeIndex);

		VectorConstNodeIndices::iterator it;
		it = std::remove(m_taggedNodes.begin(), m_taggedNodes.end(), nodeIndex);
		m_taggedNodes.erase(it, m_taggedNodes.end());
		it = std::remove(m_markedNodes.begin(), m_markedNodes.end(), nodeIndex);
		m_markedNodes.erase(it, m_markedNodes.end());

		EntranceMap::iterator entranceIt;
		entranceIt = std::find_if(m_mapEntrances.begin(), m_mapEntrances.end(), SNodeFinder(nodeIndex));
		if (entranceIt != m_mapEntrances.end())
			m_mapEntrances.erase(entranceIt);
		entranceIt = std::find_if(m_mapExits.begin(), m_mapExits.end(), SNodeFinder(nodeIndex));
		if (entranceIt != m_mapExits.end())
			m_mapExits.erase(entranceIt);

		// no waypoint nav region when AI system being deleted
		if (gAIEnv.pNavigation->GetWaypointHumanNavRegion())
			gAIEnv.pNavigation->GetWaypointHumanNavRegion()->ResetUpdateStatus();

		m_pGraphNodeManager->DestroyNode(nodeIndex);
	}
}


struct SVolumeHideSpotFinder
{
	SVolumeHideSpotFinder(ListObstacles& obs) : obs(obs) {}

	void operator()(const SVolumeHideSpot& hs, float) 
	{
		obs.push_back(ObstacleData(hs.pos, hs.dir));
	}

private:
	ListObstacles& obs;
};


//===================================================================
// FindNodesWithinRange
//===================================================================
void CGraph::FindNodesWithinRange(MapConstNodesDistance &result, float curDistT, float maxDist, 
																	const GraphNode *pStartNode, float passRadius, const class CAIObject *pRequester) const
{
	bool checkWaterDepth = false;
	float minWaterDepth = 0.0f;
	float maxWaterDepth = 0.0f;

	if (pRequester && pRequester->CastToCAIActor())
	{
		const CAIActor* pActor = pRequester->CastToCAIActor();
		if (pActor)
		{
			minWaterDepth = pActor->m_movementAbility.pathfindingProperties.minWaterDepth;
			maxWaterDepth = pActor->m_movementAbility.pathfindingProperties.maxWaterDepth;
			checkWaterDepth = true;
		}
	}

	const CGraphLinkManager& linkManager = GetLinkManager();
	const CGraphNodeManager& nodeManager = GetNodeManager();

	typedef stl::STLPoolAllocator< std::pair<float, const GraphNode*> > FloatNodeMapAllocator;
	typedef std::multimap<float, const GraphNode*, std::less<float>, FloatNodeMapAllocator> OpenListMap;

	typedef stl::STLPoolAllocator< const GraphNode* > NodeSetAllocator;

	static OpenListMap openList;
	openList.clear();
	pStartNode->mark = 1;
	openList.insert(std::make_pair(0.0f, pStartNode));
//	openList[0.0f] = pStartNode;

	result[pStartNode] = 0.0f;

	while (!openList.empty())
	{
		OpenListMap::iterator front = openList.begin();
		const GraphNode *pNode = front->second;
		float curDist = front->first;
		openList.erase(front);
		pNode->mark = 0;

		for (unsigned link = pNode->firstLinkIndex; link; link = linkManager.GetNextLink(link))
		{
			unsigned int nextNodeIndex = linkManager.GetNextNode(link);
			const GraphNode* pNext = nodeManager.GetNode(nextNodeIndex);

			if (!(pNext->navType & (IAISystem::NAV_SMARTOBJECT | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE | IAISystem::NAV_TRIANGULAR | IAISystem::NAV_VOLUME)))
				continue;

			if (pRequester && pNode->navType == IAISystem::NAV_SMARTOBJECT && pNext->navType == IAISystem::NAV_SMARTOBJECT)
			{
				const GraphNode* nodes[2] = {pNode, pNext};
				float resFactor = gAIEnv.pSmartObjectManager->GetSmartObjectLinkCostFactor(nodes, pRequester);
				if ( resFactor < 0 )
					continue;
			}
			else if (linkManager.GetRadius(link) < passRadius)
			{
				continue;
			}
			else if (checkWaterDepth)
			{
				float wd = linkManager.GetMaxWaterDepth(link);
				if (wd > maxWaterDepth)
					continue;
				if (wd < minWaterDepth)
					continue;
			}

			float linkLen = 0.01f + Distance::Point_Point(pNode->GetPos(), pNext->GetPos()); // bias to prevent loops

			float totalDist = curDist + linkLen;
			if (totalDist <= maxDist)
			{
				// If we've already processed pNext and had a lower range value before then don't continue
				MapConstNodesDistance::iterator it = result.find(pNext);
				if (it != result.end())
				{
					if (totalDist >= it->second)
						continue;
					// Smaller distance, update value.
					it->second = totalDist;
				}
				else
				{
					result[pNext] = totalDist;
				}

				// Update open list.
				OpenListMap::iterator found = openList.end();
				if (pNext->mark == 1)
				{
					for (OpenListMap::iterator it2 = openList.begin(), end = openList.end(); it2 != end; ++it2)
					{
						if (it2->second == pNext)
						{
							found = it2;
							break;
						}
					}
				}

				if (found != openList.end())
				{
					// Replace value in open list.
					if (totalDist < found->first)
					{
						openList.erase(found);
						openList.insert(std::make_pair(totalDist, pNext));
					}
				}
				else
				{
					// Add to the open list
					pNext->mark = 1;
					openList.insert(std::make_pair(totalDist, pNext));
				}
			}
		}
	}
}


//====================================================================
// GetNodesInRange
//====================================================================
MapConstNodesDistance &CGraph::GetNodesInRange(MapConstNodesDistance &result, const Vec3 &startPos, 
																							 float maxDist, IAISystem::tNavCapMask navCapMask, float passRadius,
																							 unsigned startNodeIndex, const class CAIObject *pRequester) 
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI );
	result.clear();

	GraphNode* pStart = GetNodeManager().GetNode(startNodeIndex);

	const CAllNodesContainer &allNodes = GetAllNodes(); 
	const CAllNodesContainer::Iterator it(allNodes, IAISystem::NAV_TRIANGULAR | IAISystem::NAV_VOLUME); 
	if (!it.GetNode())
		return result; // no navigation

	unsigned nodeIndex = 0;
	if (pStart && !allNodes.DoesNodeExist(startNodeIndex))
	{
		startNodeIndex = 0;
		pStart = 0;
	}
	if (pStart && (pStart->navType & navCapMask) && startPos.IsEquivalent(pStart->GetPos(), 0.01f))
		nodeIndex = startNodeIndex;
	if (!nodeIndex)
	{
		nodeIndex = GetEnclosing(startPos, navCapMask, 0.0f, startNodeIndex);
		if (!nodeIndex)
			return result;
	}

	// don't add the distance to the current node since that 
	float curDist = 0.0f; //Distance::Point_Point(startPos, pNode->GetPos());
	FindNodesWithinRange(result, curDist, maxDist, GetNodeManager().GetNode(nodeIndex), passRadius, pRequester);
	return result;
}

//====================================================================
// Reset
//====================================================================
void CGraph::Reset()
{
	ClearMarks();
	RestoreAllNavigation( );
}

//====================================================================
// GetEntrance
//====================================================================
GraphNode * CGraph::GetEntrance(int nBuildingID,const Vec3 &pos)
{
	GraphNode *pEntrance=0;
	float mindist = 1000000;
	EntranceMap::iterator ei = m_mapEntrances.find(nBuildingID);
	if (ei != m_mapEntrances.end())
	{
		pEntrance=GetNodeManager().GetNode(ei->second);
		if (m_mapEntrances.count(nBuildingID) > 1)
		{
			mindist = (pEntrance->GetPos()-pos).GetLengthSquared();
			for (;ei!=m_mapEntrances.end();ei++)
			{
				if (ei->first != nBuildingID)
					break;
				float curr_dist = (GetNodeManager().GetNode(ei->second)->GetPos()-pos).GetLengthSquared();
				if (curr_dist<=mindist)
				{
					pEntrance = GetNodeManager().GetNode(ei->second);
					mindist = curr_dist;
				}
			}
		}
	}

	ei = m_mapExits.find(nBuildingID);
	if (ei != m_mapExits.end())
	{
		pEntrance=GetNodeManager().GetNode(ei->second);
		if (m_mapExits.count(nBuildingID) > 1)
		{
			mindist = (pEntrance->GetPos()-pos).GetLengthSquared();
			for (;ei!=m_mapExits.end();ei++)
			{
				if (ei->first != nBuildingID)
					break;
				float curr_dist = (GetNodeManager().GetNode(ei->second)->GetPos()-pos).GetLengthSquared();
				if (curr_dist<=mindist)
				{
					pEntrance = GetNodeManager().GetNode(ei->second);
					mindist = curr_dist;
				}
			}
		}
	}


	return pEntrance;
}

//====================================================================
// GetEntrances
//====================================================================
bool CGraph::GetEntrances(int nBuildingID, const Vec3& pos, std::vector<unsigned>& nodes)
{
	EntranceMap::iterator it1 = m_mapEntrances.lower_bound(nBuildingID);
	EntranceMap::iterator it2 = m_mapEntrances.upper_bound(nBuildingID);
	for (EntranceMap::iterator it = it1 ; it != it2 ; ++it)
		nodes.push_back(it->second);
	it1 = m_mapExits.lower_bound(nBuildingID);
	it2 = m_mapExits.upper_bound(nBuildingID);
	for (EntranceMap::iterator it = it1 ; it != it2 ; ++it)
		nodes.push_back(it->second);
	return !nodes.empty();
}

//====================================================================
// RestoreAllNavigation
// for all removable nodes restore links passibility etc
//====================================================================
void CGraph::RestoreAllNavigation( )
{
	CAllNodesContainer::Iterator it(m_allNodes, IAISystem::NAVMASK_ALL);
	while (GraphNode* node = GetNodeManager().GetNode(it.Increment()))
	{
		for (unsigned link = node->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
		{
			GetLinkManager().RestoreLink(link);
		}
	}
}

//====================================================================
// CheckForEmpty
//====================================================================
bool CGraph::CheckForEmpty(IAISystem::tNavCapMask navTypeMask) const
{
	unsigned count = 0;
	CAllNodesContainer::Iterator it(m_allNodes, navTypeMask);
	while (const GraphNode* node = GetNodeManager().GetNode(it.Increment()))
	{
		++count;
		AILogEvent("Unexpected Node %p, type = %d, pos = (%5.2f %5.2f %5.2f)", node, node->navType, 
			node->GetPos().x, node->GetPos().y, node->GetPos().z);
	}
	if (count)
		AIWarning("Detected %d unexpected nodes whilst checking types %u", count, (unsigned )navTypeMask);
	return (count == 0);
}

//====================================================================
// Validate
//====================================================================
bool CGraph::Validate(const char * msg, bool checkPassable) const
{
#ifdef _DEBUG
	return true;
#endif

	if (!AIGetWarningErrorsEnabled())
	{
		AILogEvent("CGraph::Validate Skipping: %s", msg); 
		return true;
	}

	AILogProgress("CGraph::Validate Starting: %s", msg);

	// Danny todo tweak this so that if !checkPassable then we only clear
	// errors that are not to do with the passable checks...
	if (checkPassable)
		mBadGraphData.clear();

	if (!m_pFirst)
		return false;

	if (!m_pSafeFirst->firstLinkIndex)
		return true;

	// the first safe node is different - expect only one link
	if (GetLinkManager().GetNextLink(m_pSafeFirst->firstLinkIndex) != 0)
	{
		AIWarning("CGraph::Validate Expect only 1 link to the first safe node");
		return false;
	}

	int badOutdoorNodes = 0;
	int badIndoorNodes = 0;
	int badLinks = 0;
	int badNumOutsideLinks = 0;
	int badLinkPassability = 0;

  static int maxBadImassabilityWarnings = 10;

	// nodes containing vertices with identical indices
	int indexDegenerates = 0;
	// nodes containing vertices that have different indices, but essentially
	// the same position.
	int posDegenerates = 0;

	CAllNodesContainer::Iterator it(m_allNodes, IAISystem::NAVMASK_ALL);
	while (unsigned nodeIndex = it.Increment())
	{
		const GraphNode* node = GetNodeManager().GetNode(nodeIndex);
		if (!CGraph::ValidateNode(nodeIndex, true))
			AIWarning("CGraph::Validate Node validation failed: %p", node);

		if (node->navType == IAISystem::NAV_WAYPOINT_HUMAN)
		{
			if (node->GetWaypointNavData()->nBuildingID == -1)
			{
				AIWarning("Node at (%5.2f, %5.2f, %5.2f) is not in a human waypoint nav modifier [design bug]", 
					node->GetPos().x, node->GetPos().y, node->GetPos().z);
				++badIndoorNodes;
			}
		}
		else if (node->navType == IAISystem::NAV_WAYPOINT_3DSURFACE)
		{
			if (node->GetWaypointNavData()->nBuildingID == -1)
			{
				AIWarning("Node at (%5.2f, %5.2f, %5.2f) is not in a 3d surface nav modifier [design bug]", 
					node->GetPos().x, node->GetPos().y, node->GetPos().z);
				++badIndoorNodes;
			}
		}
		else if (node->navType == IAISystem::NAV_TRIANGULAR)
		{
			unsigned numVertices = node->GetTriangularNavData()->vertices.size();
			if (numVertices != 3)
				++badOutdoorNodes;

			// check for degenerates
			for (unsigned iV = 0 ; iV < numVertices ; ++iV)
			{
				unsigned iNext = iV+1;
				if (iNext == numVertices)
					iNext = 0;

				if (node->GetTriangularNavData()->vertices[iV] == node->GetTriangularNavData()->vertices[iNext])
				{
					const Vec3 & v = GetAISystem()->m_VertexList.GetVertex(node->GetTriangularNavData()->vertices[iV]).vPos;
					AIWarning("Degenerate vertex at (%5.2f, %5.2f, %5.2f)", v.x, v.y, v.z);
					++indexDegenerates;
				}
				else
				{
					const Vec3 & v0 = GetAISystem()->m_VertexList.GetVertex(node->GetTriangularNavData()->vertices[iV]).vPos;
					const Vec3 & v1 = GetAISystem()->m_VertexList.GetVertex(node->GetTriangularNavData()->vertices[iNext]).vPos;
					float distTol = 0.001f;
					if ( (v0 - v1).GetLengthSquared() < distTol*distTol )
					{
						++posDegenerates;
					}
				}
			}
			// store if we're in forbidden so we can check boundaries
			bool nodeInForbidden = node->GetTriangularNavData()->isForbidden;

			for (unsigned link = node->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
			{
				if (GetLinkManager().GetStartIndex(link) != GetLinkManager().GetEndIndex(link) && GetLinkManager().GetEdgeCenter(link).GetLength() == 0.0f)
					++badLinks;

				const GraphNode * next = GetNodeManager().GetNode(GetLinkManager().GetNextNode(link));
				AIAssert(next);

				// check that there's a link back...
				unsigned nextLink;
				for (nextLink = next->firstLinkIndex; nextLink; nextLink = GetLinkManager().GetNextLink(nextLink))
				{
					if (GetLinkManager().GetNextNode(nextLink) == nodeIndex)
						break;
				}
				if (!nextLink)
					++badNumOutsideLinks;

				// check if it's impassable:
				if (checkPassable && 
					next != m_pSafeFirst &&
					(GetLinkManager().GetStartIndex(link) != GetLinkManager().GetEndIndex(link) && !GetLinkManager().GetEdgeCenter(link).IsZero()))
				{
					Vec3 vStart = GetAISystem()->m_VertexList.GetVertex(node->GetTriangularNavData()->vertices[GetLinkManager().GetStartIndex(link)]).vPos;
					Vec3 vEnd = GetAISystem()->m_VertexList.GetVertex(node->GetTriangularNavData()->vertices[GetLinkManager().GetEndIndex(link)]).vPos;

					bool nextInForbidden = next->GetTriangularNavData()->isForbidden;
					if (GetLinkManager().GetRadius(link) > 0.0f && nextInForbidden && !nodeInForbidden)
					{
						mBadGraphData.push_back(SBadGraphData());
						mBadGraphData.back().mType = SBadGraphData::BAD_IMPASSABLE;
						mBadGraphData.back().mPos1 = vStart;
						mBadGraphData.back().mPos2 = vEnd;
						++badLinkPassability;
            if (badLinkPassability <= maxBadImassabilityWarnings)
              AIWarning("Bad link passability at (%5.2f %5.2f %5.2f), linkID %#x - see if tweaking the object positions there helps",
              0.5f*(vStart.x+vEnd.x), 0.5f*(vStart.y+vEnd.y), 0.5f*(vStart.z+vEnd.z), LinkId (link));
					}
					else if (GetLinkManager().GetRadius(link) == -1.0f && !nextInForbidden && nodeInForbidden)
					{
						mBadGraphData.push_back(SBadGraphData());
						mBadGraphData.back().mType = SBadGraphData::BAD_PASSABLE;
						mBadGraphData.back().mPos1 = vStart;
						mBadGraphData.back().mPos2 = vEnd;
						++badLinkPassability;
            if (badLinkPassability <= maxBadImassabilityWarnings)
              AIWarning("Bad link passability at (%5.2f %5.2f %5.2f), linkID %#x - see if tweaking the object positions there helps",
              0.5f*(vStart.x+vEnd.x), 0.5f*(vStart.y+vEnd.y), 0.5f*(vStart.z+vEnd.z), LinkId (link));
					}
				}
			}
		}
	}

	if (badOutdoorNodes + badIndoorNodes + badNumOutsideLinks + indexDegenerates + posDegenerates + badLinkPassability > 0)
	{
		AIError("CGraph::Validate AI graph contains invalid: %d outdoor, %d indoor, %d outdoor links, %d degenerate index, %d degenerate pos, %d passable: %s: Regenerate triangulation in editor [Design bug]", 
			badOutdoorNodes, badIndoorNodes, badNumOutsideLinks, indexDegenerates, posDegenerates, badLinkPassability, msg);
	}

	AILogProgress("CGraph::Validate Finished graph validation: %s", msg);
	return (0 == badOutdoorNodes + badIndoorNodes);
}

//====================================================================
// GetIDFromNode
//====================================================================
unsigned CGraph::GetIDFromNode(unsigned nodeIndex, TGraphNodeIDs *graphNodeIDs) const
{
	AIAssert(nodeIndex != ~0);
  unsigned result = 0;
	if (nodeIndex != 0 && nodeIndex != ~0)
		result = GetNodeManager().GetNode(nodeIndex)->ID;
  if (graphNodeIDs)
    (*graphNodeIDs)[result] = nodeIndex;
  return result;
}

//====================================================================
// GetNodeFromID
//====================================================================
unsigned CGraph::GetNodeFromID(unsigned ID, TGraphNodeIDs *graphNodeIDs) const
{
	if (ID == 0)
		return 0;

  if (graphNodeIDs)
  {
    const TGraphNodeIDs::iterator it = graphNodeIDs->find(ID);
    if (it != graphNodeIDs->end())
      return it->second;
    AIWarning("CGraph::GetNodeFromID: Cannot find node with ID %d in graphNodeIDs", ID);
  }

	CAllNodesContainer::Iterator it(m_allNodes, IAISystem::NAVMASK_ALL | IAISystem::NAV_UNSET);
	while (unsigned nodeIndex = it.Increment())
	{
		const GraphNode* node = GetNodeManager().GetNode(nodeIndex);
		if (node->ID == ID)
			return nodeIndex;
	}
	AIWarning("CGraph::GetNodeFromID: Cannot find node with ID %d", ID);
	return 0;
}

//====================================================================
// PopulateObjectTracker
//====================================================================
void CGraph::PopulateObjectTracker(CObjectTracker& objectTracker)
{
}

// modified links
struct SLinkModifier
{
	unsigned nodeIndex;
	unsigned iLink;
	float newRadius;

	SLinkModifier() : nodeIndex(0), iLink(0), newRadius(0.0f)
	{}

	void Serialize(TSerialize ser, const CGraph* pGraph)
	{
		ser.BeginGroup("LinkMod");
		pGraph->SerializeNodePointer(ser, "node", nodeIndex);
		ser.Value("iLink", iLink);
		ser.Value("newRadius", newRadius);
		ser.EndGroup();
	}
};

//===================================================================
// ProcessGraphNodeIDs
//===================================================================
void CGraph::SerializeGraphNodeIDs(TSerialize ser, CGraph::TGraphNodeIDs &graphNodeIDs)
{
  ser.BeginGroup("GraphNodeIDs");
  {
    std::vector<unsigned> IDs;
    if (ser.IsWriting())
    {
      for (TGraphNodeIDs::iterator it = graphNodeIDs.begin() ; it != graphNodeIDs.end() ; ++it)
        IDs.push_back(it->first);
      ser.Value("IDs", IDs);
    }
    else
    {
      ser.Value("IDs", IDs);
      std::sort(IDs.begin(), IDs.end());
      CAllNodesContainer::Iterator it(m_allNodes, IAISystem::NAVMASK_ALL | IAISystem::NAV_UNSET);
      while (unsigned nodeIndex = it.Increment())
      {
				unsigned id = GetNodeManager().GetNode(nodeIndex)->ID;
				if (std::binary_search(IDs.begin(), IDs.end(), id))
          graphNodeIDs[id] = nodeIndex;
      }
    }
  }
  ser.EndGroup();
}

//====================================================================
// Serialize
//====================================================================
void CGraph::Serialize( TSerialize ser, CObjectTracker& objectTracker, const char* graphName )
{
	if (ser.IsReading())
	{
		Reset();
	}

  ser.BeginGroup( graphName );
  {
    TGraphNodeIDs graphNodeIDs;
    if (ser.IsReading())
      SerializeGraphNodeIDs(ser, graphNodeIDs);

    ser.BeginGroup("GraphPointers");
    {
      SerializeNodePointer(ser, "m_pCurrent", m_currentIndex, &graphNodeIDs);
      SerializeNodePointer(ser, "m_pFirst", m_firstIndex, &graphNodeIDs);
      SerializeNodePointerContainer(ser, "m_markedNodes", m_markedNodes, &graphNodeIDs);
      SerializeNodePointerContainer(ser, "m_taggedNodes", m_taggedNodes, &graphNodeIDs);
    }
    ser.EndGroup();

    if (ser.IsWriting())
      SerializeGraphNodeIDs(ser, graphNodeIDs);

    ser.BeginGroup("Link_Modifiers");
    {
      if (ser.IsReading())
      {
        unsigned numModifiers = 0;
        ser.Value("numModifiers", numModifiers);
        for (unsigned i = 0 ; i < numModifiers ; ++i)
        {
          SLinkModifier mod;
          mod.Serialize(ser, this);
          if (mod.nodeIndex)
          {
						GraphNode* modNode = GetNodeManager().GetNode(mod.nodeIndex);

						unsigned link, index;
						for (link = modNode->firstLinkIndex, index = 0; link && index < mod.iLink; link = GetLinkManager().GetNextLink(link), ++index);
						if (link)
						{
              GetLinkManager().ModifyRadius(link, mod.newRadius);
						}
						else
						{
              AIWarning("link modifier for node %p has link index = %d - only %d links on this node", 
                modNode, mod.iLink, index);
						}
          }
          else
          {
            AIWarning("link modifier generated 0 node pointer");
          }
        }
      }
      else
      {
        std::vector<SLinkModifier> linkModifiers;
        CAllNodesContainer::Iterator it(m_allNodes, IAISystem::NAVMASK_ALL);
        while (unsigned nodeIndex = it.Increment())
        {
					GraphNode* node = GetNodeManager().GetNode(nodeIndex);
					int linkIndex = 0;
          for (unsigned link = node->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
          {
            if (GetLinkManager().IsLinkModified(link))
            {
              SLinkModifier mod;
              mod.iLink = linkIndex;
              mod.newRadius = GetLinkManager().GetRadius(link);
              mod.nodeIndex = nodeIndex;
              linkModifiers.push_back(mod);
            }
						++linkIndex;
          }
        }
        unsigned numModifiers = linkModifiers.size();
        ser.Value("numModifiers", numModifiers);
        for (std::vector<SLinkModifier>::iterator it2 = linkModifiers.begin() ; it2 != linkModifiers.end() ; ++it2)
        {
          SLinkModifier& mod = *it2;
          mod.Serialize(ser, this);
        }
      }
    }
    ser.EndGroup(); // link modifiers

  }
  ser.EndGroup(); // graphName
}

//====================================================================
// SerializeNodePointer
//====================================================================
void CGraph::SerializeNodePointer(TSerialize ser, const char* name, unsigned& pNode, TGraphNodeIDs *graphNodeIDs) const
{
	if (ser.IsReading())
	{
		unsigned ID;
		ser.Value(name, ID);
		pNode = GetNodeFromID(ID, graphNodeIDs);
		// if pointer = 0 the ID is 0
	}
	else
	{
		unsigned ID = GetIDFromNode(pNode, graphNodeIDs);
		ser.Value(name, ID);
	}
}


//====================================================================
// Serialize
//====================================================================
void CObstacleRef::Serialize( TSerialize ser, CObjectTracker& objectTracker )
{
	m_refAnchor.Serialize(ser, "m_refAnchor");
	ser.Value("m_vertexIndex", m_vertexIndex);
	CGraph* pGraph = gAIEnv.pGraph;
	if (pGraph)
		pGraph->SerializeNodePointer(ser, "m_pNode", m_nodeIndex);
}

//===================================================================
// ResetIDs
//===================================================================
void GraphNode::ResetIDs(CGraphNodeManager& nodeManager, class CAllNodesContainer &allNodes, GraphNode *pNodeForID1)
{
	CAllNodesContainer::Iterator itAll(allNodes, 0xffffffff);

	maxID = 1;
	while (GraphNode* pCurrent = nodeManager.GetNode(itAll.Increment()))
	{
		if (pCurrent == pNodeForID1)
			pCurrent->ID = 1;
		else
			pCurrent->ID = ++maxID;
	}

	freeIDs.clear();
}
