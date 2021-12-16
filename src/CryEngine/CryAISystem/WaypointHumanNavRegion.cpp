/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   WaypointHumanNavRegion.cpp
$Id$
Description: 

-------------------------------------------------------------------------
History:
- ?
- 4 May 2009  : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#include "StdAfx.h"
#include "WaypointHumanNavRegion.h"
#include "AICollision.h"
#include "CAISystem.h"
#include "ISerialize.h"
#include "WorldOctree.h"
#include "ISystem.h"
#include "IPhysics.h"
#include "I3DEngine.h"
#include "DebugDrawContext.h"	
#include "ITimer.h"
#include <limits>
#include "CalculationStopper.h"

static float zWaypointScale = 1.0f;

//====================================================================
// CWaypointHumanNavRegion
//====================================================================
CWaypointHumanNavRegion::CWaypointHumanNavRegion(CGraph* pGraph)
{
  AIAssert(pGraph);
  m_pGraph = pGraph;
  m_currentNodeIt = 0;
}

//====================================================================
// ~CWaypointHumanNavRegion
//====================================================================
CWaypointHumanNavRegion::~CWaypointHumanNavRegion()
{
  delete m_currentNodeIt;
  m_currentNodeIt = 0;
}

//===================================================================
// ResetUpdateStatus
//===================================================================
void CWaypointHumanNavRegion::ResetUpdateStatus()
{
  delete m_currentNodeIt;
  m_currentNodeIt = 0;
}

//====================================================================
// UglifyPath
//====================================================================
void CWaypointHumanNavRegion::UglifyPath(const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
                                         const Vec3& startPos, const Vec3& startDir, 
                                         const Vec3& endPos, const Vec3 & endDir)
{
  outPath.push_back(PathPointDescriptor(IAISystem::NAV_WAYPOINT_HUMAN, startPos));
  for(VectorConstNodeIndices::const_iterator itrCurNode=inPath.begin() ; itrCurNode != inPath.end() ; ++itrCurNode)
  {
		const GraphNode& curNode=*m_pGraph->GetNodeManager().GetNode(*itrCurNode);
    outPath.push_back(PathPointDescriptor(IAISystem::NAV_WAYPOINT_HUMAN, curNode.GetPos()));
  }
  outPath.push_back(PathPointDescriptor(IAISystem::NAV_WAYPOINT_HUMAN, endPos));
}

//====================================================================
// BeautifyPath
//====================================================================
void CWaypointHumanNavRegion::BeautifyPath(const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
                                           const Vec3& startPos, const Vec3& startDir, 
                                           const Vec3& endPos, const Vec3 & endDir,
                                           float radius,
                                           const AgentMovementAbility & movementAbility,
                                           const NavigationBlockers& navigationBlockers)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );
  if (inPath.empty())
    return;

  AIAssert(m_pGraph->GetNodeManager().GetNode(inPath.front())->navType == IAISystem::NAV_WAYPOINT_HUMAN);
  int nBuildingID = m_pGraph->GetNodeManager().GetNode(inPath.front())->GetWaypointNavData()->nBuildingID;
  if (nBuildingID < 0)
  {
    AIWarning("CWaypointHumanNavRegion::BeautifyPath Unable to get building ID from node %d", inPath.front());
    return;
  }
  const SpecialArea* sa = gAIEnv.pNavigation->GetSpecialArea(nBuildingID);
  if (!sa)
  {
    AIWarning("CWaypointHumanNavRegion::BeautifyPath Unable to get special area for building ID %d", nBuildingID);
    return;
  }
  const ListPositions	& buildingPolygon = sa->GetPolygon();

  bool allowStartSkip = !startPos.IsEquivalent(m_pGraph->GetNodeManager().GetNode(inPath.front())->GetPos(), 0.01f);
  bool allowEndSkip = !endPos.IsEquivalent(m_pGraph->GetNodeManager().GetNode(inPath.back())->GetPos(), 0.01f);

  if (inPath.size() == 1)
  {
    if (allowStartSkip && allowEndSkip && CheckWalkability(startPos, endPos, 0.0f, false, buildingPolygon, AICE_ALL))
    {
      // we can walk directly - don't need to go via the node.
      return;
    }
  }

  VectorConstNodeIndices inPathCopy(inPath);
  // see if we can skip the last one
  unsigned inPathSize = inPathCopy.size();
  if (inPathSize > 1 && allowEndSkip)
  {
    if (CheckWalkability(endPos, m_pGraph->GetNodeManager().GetNode(inPath[inPathSize-2])->GetPos(), 0.0f, false, buildingPolygon, AICE_ALL))
    {
      inPathCopy.pop_back();
      --inPathSize;
    }
  }
  // see if we can skip the first one
  if (inPathSize > 1 && allowStartSkip)
  {
    if (CheckWalkability(startPos, m_pGraph->GetNodeManager().GetNode(inPath[1])->GetPos(), 0.0f, false, buildingPolygon, AICE_ALL))
    {
      inPathCopy.erase(inPathCopy.begin());
      --inPathSize;
    }
  }
  else if (inPathSize == 1 && allowStartSkip)
  {
    if (CheckWalkability(startPos, endPos, 0.0f, false, buildingPolygon, AICE_ALL))
    {
      // we can walk directly - don't need to go via the node.
      return;
    }
  }

  // Just go through all the nodes
  for(VectorConstNodeIndices::const_iterator itrCurNode=inPathCopy.begin() ; itrCurNode != inPathCopy.end() ; ++itrCurNode)
  {
    const GraphNode& curNode=*m_pGraph->GetNodeManager().GetNode(*itrCurNode);
    AIAssert(curNode.navType == IAISystem::NAV_WAYPOINT_HUMAN);
    outPath.push_back(PathPointDescriptor(IAISystem::NAV_WAYPOINT_HUMAN, curNode.GetPos()));
  }
}

//====================================================================
// FillGreedyMap
//====================================================================
void CWaypointHumanNavRegion::FillGreedyMap(unsigned nodeIndex, const Vec3 &pos, 
                                            IVisArea *pTargetArea, bool bStayInArea, CandidateIdMap& candidates)
{
	GraphNode* pNode = m_pGraph->GetNodeManager().GetNode(nodeIndex);
  m_pGraph->MarkNode(nodeIndex);

  if (!pNode->firstLinkIndex)
    return;
  for (unsigned link = pNode->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
  { 
		unsigned nowIndex = m_pGraph->GetLinkManager().GetNextNode(link);
    GraphNode *pNow = m_pGraph->GetNodeManager().GetNode(nowIndex);
    if ( pNow->mark || pNow->navType != IAISystem::NAV_WAYPOINT_HUMAN || pNow->GetWaypointNavData()->nBuildingID != pNode->GetWaypointNavData()->nBuildingID) 
      continue;
    if (bStayInArea && (pNow->GetWaypointNavData()->pArea != pTargetArea))
      continue;

    Vec3 delta = (pos - pNow->GetPos());
    delta.z *= zWaypointScale; // scale vertical to prefer waypoints on the same level
    float thisdist = delta.GetLengthSquared();
    candidates.insert(CandidateIdMap::iterator::value_type(thisdist,nowIndex));

    // this snippet will make sure we only check all points inside the target area - not the whole building
    if (pTargetArea && pNow->GetWaypointNavData()->pArea == pTargetArea)
      FillGreedyMap(nowIndex,pos,pTargetArea,true, candidates);
    else
      FillGreedyMap(nowIndex,pos,pTargetArea,false, candidates);
  }

}

struct SWrongBuildingRadius
{
  SWrongBuildingRadius(CGraphLinkManager& linkManager, CGraphNodeManager& nodeManager, int ID, float radius)
		: linkManager(linkManager), nodeManager(nodeManager), ID(ID), radius(radius) {}
  bool operator()(const std::pair<float, unsigned> &node) 
  {
    if (nodeManager.GetNode(node.second)->GetWaypointNavData()->nBuildingID != ID)
      return true;
    if (GetAISystem()->ExitNodeImpossible(linkManager, nodeManager.GetNode(node.second), radius))
      return true;
    return false;
  }
	CGraphLinkManager& linkManager;
	CGraphNodeManager& nodeManager;
  int ID;
  float radius;
};

//====================================================================
// GetEnclosing
//====================================================================
unsigned CWaypointHumanNavRegion::GetEnclosing(const Vec3 &pos, float passRadius, unsigned startIndex, 
                                                 float range, Vec3 * closestValid, bool returnSuspect, const char *requesterName)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	// A range value less than zero results in get search based on fNodeAutoConnectDistance.
	if (range < 0.0f)
		range = 0.0f;

  IVisArea *pGoalArea; 
  int	nBuildingID;
  gAIEnv.pNavigation->CheckNavigationType(pos, nBuildingID, pGoalArea, IAISystem::NAV_WAYPOINT_HUMAN);

  if (nBuildingID < 0)
  {
    AIWarning("CWaypointHumanNavRegion::GetEnclosing found bad building ID = %d (%s) for %s (%5.2f, %5.2f, %5.2f)", 
      nBuildingID, gAIEnv.pNavigation->GetNavigationShapeName(nBuildingID), requesterName, pos.x, pos.y, pos.z);
    return 0;
  }

  const SpecialArea *sa = gAIEnv.pNavigation->GetSpecialArea(nBuildingID);
  if (!sa)
  {
    AIWarning("CWaypointHumanNavRegion::GetEnclosing found no area for %s (%5.2f, %5.2f, %5.2f)", 
      requesterName, pos.x, pos.y, pos.z);
    return 0;
  }

  // get a floor position
  Vec3 floorPos;
  static float maxFloorDist = 15.0f; // big value because the target might be huge - e.g. hunter
  if (false == GetFloorPos(floorPos, pos, walkabilityFloorUpDist, maxFloorDist, walkabilityDownRadius, AICE_ALL))
  {
    AIWarning("CWaypointHumanNavRegion::GetEnclosing Could not find floor position from (%5.2f, %5.2f, %5.2f) %s",
      pos.x, pos.y, pos.z, requesterName);
    return 0;
  }

  if (sa->fNodeAutoConnectDistance < 0.0001f)
    AIWarning("CWaypointHumanNavRegion::GetEnclosing AI nav modifier %s has NodeAutoConnectDistance=0 - increase to a sensible value [design bug]", gAIEnv.pNavigation->GetNavigationShapeName(nBuildingID));

  float totalDist = 1.5f * sa->fNodeAutoConnectDistance + range;

  typedef std::vector< std::pair<float, unsigned> > TNodes;
  static TNodes nodes;
  nodes.resize(0);

  // Most times we'll find the desired node close by
  CAllNodesContainer &allNodes = m_pGraph->GetAllNodes();
  allNodes.GetAllNodesWithinRange(nodes, floorPos, totalDist, IAISystem::NAV_WAYPOINT_HUMAN);
  std::vector< std::pair<float, unsigned> >::iterator it = std::remove_if(nodes.begin(), nodes.end(), SWrongBuildingRadius(m_pGraph->GetLinkManager(), m_pGraph->GetNodeManager(), nBuildingID, passRadius));
  nodes.erase(it, nodes.end());

  if (nodes.empty())
  {
    AILogComment("Unable to find node close to %s (%5.2f, %5.2f, %5.2f) - checking all nodes from building id %d (%s)",
      requesterName, pos.x, pos.y, pos.z, nBuildingID, gAIEnv.pNavigation->GetNavigationShapeName(nBuildingID));

    CAllNodesContainer::Iterator nit(allNodes, IAISystem::NAV_WAYPOINT_HUMAN);
    while (unsigned currentIndex = nit.Increment())
    {
			GraphNode* pCurrent = m_pGraph->GetNodeManager().GetNode(currentIndex);
      if (pCurrent->GetWaypointNavData()->nBuildingID == nBuildingID && !GetAISystem()->ExitNodeImpossible(m_pGraph->GetLinkManager(), pCurrent, passRadius))
      {
        nodes.push_back(std::make_pair(0.0f, currentIndex)); // dist calculated later
      }
    }
  }

  if (nodes.empty())
  {
    AIWarning("CWaypointHumanNavRegion::GetEnclosing No nodes found for area containing %s (%5.2f, %5.2f, %5.2f) building id %d (%s) [design bug]", 
      requesterName, pos.x, pos.y, pos.z, nBuildingID, gAIEnv.pNavigation->GetNavigationShapeName(nBuildingID));
    return 0;
  }


  unsigned closestIndex = 0;
	if (returnSuspect)
	{
  float closestDist = std::numeric_limits<float>::max();

  for (unsigned i = 0 ; i < nodes.size() ; ++i)
  {
		Vec3 delta = m_pGraph->GetNodeManager().GetNode(nodes[i].second)->GetPos() - floorPos;
    delta.z *= 5;
    float distSq = delta.GetLengthSquared();
    nodes[i].first = distSq;
    if (distSq < closestDist)
    {
      closestDist = distSq;
      closestIndex = nodes[i].second;
    }
  }
	}

  // Assume that if the requester wants a full check then they've passed in a closest position
  // pointer - and if they haven't they're doing a quick test. The code gets called from 
  // COPTrace::Execute2D - Danny todo see if we can avoid calling this code from there, or call
  // a more specific function
//  if (!closestValid)
//    return pClosest;

  std::sort(nodes.begin(), nodes.end());

  const ListPositions	& buildingPolygon = sa->GetPolygon();

  static const uint32 MaxRejected = 10;
	uint32 rejected = 0;

  for (TNodes::iterator nit = nodes.begin() ; nit != nodes.end() && rejected < MaxRejected; ++nit )
  {
    uint32 nodeIndex = nit->second;

//    if (nRejected < 15)
 //   {
      if (CheckWalkabilitySimple(SWalkPosition (floorPos, true), m_pGraph->GetNodeManager().GetNode(nodeIndex)->GetPos(), 0.0f, AICE_ALL))
        return nodeIndex;
  /*  }
    else
    {
      if (CheckWalkability(SWalkPosition (floorPos, true), m_pGraph->GetNodeManager().GetNode(nodeIndex)->GetPos(), 0.0f, false, buildingPolygon, AICE_ALL))
        return nodeIndex;
    }*/
    ++rejected;
    }

	// NOTE Jun 16, 2007: <pvl> draw the requester pos and pos of nodes that had
	// their walkability from 'pos' checked and failed
	float timeVisible = gAIEnv.CVars.DrawGetEnclosingFailures;
	if (timeVisible > 0.0f)
	{
		TNodes::iterator nit = nodes.begin();
		TNodes::iterator nend = nodes.end();
		for (int i=0; nit != nend && i < MaxRejected; ++nit, ++i )
		{
			uint32 nodeIndex = nit->second;
			Vec3 nodePos = m_pGraph->GetNodeManager().GetNode(nodeIndex)->GetPos();
			GetAISystem()->AddDebugLine (pos, nodePos, 255, 255, 0, timeVisible);
			GetAISystem()->AddDebugSphere (nodePos, 0.15f, 255, 255, 0, timeVisible);
		}
	}

  uint32 nodeIndex = closestIndex;
  if (returnSuspect)
	{
    AIWarning("CWaypointHumanNavRegion::GetEnclosing %d nodes rejected for reaching %s (%5.2f, %5.2f, %5.2f): returning %d (in %d (%s))", 
		 rejected, requesterName, pos.x, pos.y, pos.z, nodeIndex, nBuildingID, gAIEnv.pNavigation->GetNavigationShapeName(nBuildingID));
	}

  return nodeIndex;
}

//====================================================================
// CheckPassability
//====================================================================
bool CWaypointHumanNavRegion::CheckPassability(const Vec3& from, const Vec3& to, float radius, const NavigationBlockers& navigationBlockers, IAISystem::tNavCapMask ) const
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );
  ListPositions boundary;
  return CheckWalkability(from, to, 0.0f, false, boundary, AICE_ALL);
}

//====================================================================
// GetClosestNode
//====================================================================
unsigned CWaypointHumanNavRegion::GetClosestNode(const Vec3 &pos, int nBuildingID)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );
  std::vector<unsigned> entrances;
  if (!m_pGraph->GetEntrances(nBuildingID, pos, entrances))
  {
    AIWarning("CWaypointHumanNavRegion::GetClosestNode No entrance for navigation area nr %d (%s). The position is (%.3f,%.3f,%.3f)",
      nBuildingID, gAIEnv.pNavigation->GetNavigationShapeName(nBuildingID), pos.x, pos.y, pos.z);
    return 0;
  }

  IVisArea *pGoalArea = 0; 
  m_pGraph->ClearMarks();
  CandidateIdMap candidates;
  for (unsigned i = 0 ; i < entrances.size() ; ++i)
    FillGreedyMap(entrances[i], pos, pGoalArea, false, candidates);
  m_pGraph->ClearMarks();

  if (candidates.empty())
  {
    AIWarning("CWaypointHumanNavRegion::GetClosestNode No nodes found for this navigation modifier %d (%s) apart from entrance",
      nBuildingID, gAIEnv.pNavigation->GetNavigationShapeName(nBuildingID));
    return 0;
  }
  else
  {
    return candidates.begin()->second;
  }
}

//====================================================================
// Serialize
//====================================================================
void CWaypointHumanNavRegion::Serialize(TSerialize ser, CObjectTracker& objectTracker)
{
  ser.BeginGroup("WaypointNavRegion");

  ser.EndGroup();
}

//====================================================================
// Reset
//====================================================================
void CWaypointHumanNavRegion::Clear()
{
  delete m_currentNodeIt;
  m_currentNodeIt = 0;
}

//====================================================================
// CheckAndDrawBody
//====================================================================
bool CWaypointHumanNavRegion::CheckAndDrawBody(const Vec3& pos)
{
  bool result = false;
  bool gotFloor = false;
  Vec3 floorPos = pos;
  if (GetFloorPos(floorPos, pos, walkabilityFloorUpDist, walkabilityFloorDownDist, walkabilityDownRadius, AICE_ALL))
    gotFloor = true;

  Vec3 segStart = floorPos + walkabilityTorsoBaseOffset + Vec3(0, 0, walkabilityRadius);
  Vec3 segEnd = floorPos + Vec3(0, 0, walkabilityTotalHeight - walkabilityRadius); 
  Lineseg torsoSeg(segStart, segEnd);

  if (gotFloor && !OverlapCapsule(torsoSeg, walkabilityRadius, AICE_ALL))
    result = true;

  CDebugDrawContext dc;
  {
    ColorF col;
    if (result)
      col.Set(0, 1, 0);
    else
      col.Set(1, 0, 0);
    dc->DrawSphere(segStart, walkabilityRadius, col);
    dc->DrawSphere(segEnd, walkabilityRadius, col);
    dc->DrawSphere(segStart, walkabilityDownRadius, col);
    dc->DrawSphere(floorPos, walkabilityDownRadius, col);
    dc->DrawLine(floorPos, col, segStart, col);
  }

  return result;
}

std::pair<bool,bool> CWaypointHumanNavRegion::CheckIfLinkSimple(Vec3 from, Vec3 to,
										float paddingRadius, bool checkStart,
										const ListPositions& boundary, 
										EAICollisionEntities aiCollisionEntities, 
										SCheckWalkabilityState *pCheckWalkabilityState)
{
	std::vector<float> & heightAlongTheLink = pCheckWalkabilityState->heightAlongTheLink;
	if (pCheckWalkabilityState->state == SCheckWalkabilityState::CWS_NEW_QUERY)
	{
		pCheckWalkabilityState->computeHeightAlongTheLink = true;
		heightAlongTheLink.resize (0);
		heightAlongTheLink.reserve (50);
	} else {
		assert (pCheckWalkabilityState->computeHeightAlongTheLink);
	}

	bool walkable = CheckWalkability (from, to, paddingRadius, checkStart, boundary,
		aiCollisionEntities, 0, pCheckWalkabilityState);

	if (pCheckWalkabilityState->state != SCheckWalkabilityState::CWS_RESULT) {
		// NOTE Apr 27, 2007: <pvl> AFAIK 'walkable' should be 'false' here
		return std::make_pair (walkable,false);
	}

	if ( ! walkable)
		return std::make_pair (false,false);

	float avg = std::accumulate (
		heightAlongTheLink.begin (),
		heightAlongTheLink.end (),
		0.0f, std::plus<float> ()) / heightAlongTheLink.size ();
	std::vector<float>::const_iterator it = heightAlongTheLink.begin ();
	std::vector<float>::const_iterator end = heightAlongTheLink.end ();
	bool linkIsSimple = true;
	for ( ; it != end; ++it)
	{
		if ( fabs (*it - avg) > 0.10)
		{
			linkIsSimple = false;
			break;
		}
	}

	return std::make_pair (true, linkIsSimple);
}

/**
 * If simplified walkability computation isn't requested by a console variable,
 * this function just falls back to generic CheckWalkability() test.  If a link
 * is classified as "simple", its walkability is tested using
 * CheckWalkabilitySimple() which should be faster.  Conversely, if the link
 * fails the "simplicity" test, generic CheckWalkability() is invoked.
 *
 * If "simplicity" of the link hasn't been established yet, it is computed now.
 * The test involves gathering floor heights along the link and seeing if the
 * sequence is straight or "linear" enough.
 */
bool CWaypointHumanNavRegion::CheckPassability(unsigned linkIndex,
											Vec3 from, Vec3 to,
											float paddingRadius, bool checkStart,
											const ListPositions& boundary, 
											EAICollisionEntities aiCollisionEntities, 
											SCachedPassabilityResult *pCachedResult,
											SCheckWalkabilityState *pCheckWalkabilityState)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if ( ! gAIEnv.CVars.SimpleWayptPassability) {
		return CheckWalkability (from, to, paddingRadius, checkStart, boundary,
						aiCollisionEntities, pCachedResult, pCheckWalkabilityState);
	}

	if (m_pGraph->GetLinkManager().SimplicityComputed(linkIndex))
	{
		if (m_pGraph->GetLinkManager().IsSimple(linkIndex))
			return CheckWalkabilitySimple (from, to, paddingRadius,	aiCollisionEntities);
		else
			return CheckWalkability (from, to, paddingRadius, checkStart, boundary,
							aiCollisionEntities, pCachedResult, pCheckWalkabilityState);
	}

	std::pair<bool,bool> result = CheckIfLinkSimple (from, to, paddingRadius, checkStart,
							boundary, aiCollisionEntities, pCheckWalkabilityState);
	bool walkable = result.first;

	if (pCheckWalkabilityState->state == SCheckWalkabilityState::CWS_RESULT)
	{
		if (walkable)
		{
			bool simple = result.second;
			CGraphLinkManager& linkManager = ((CGraph*)gAIEnv.pGraph)->GetLinkManager();
			linkManager.SetSimple (linkIndex, simple);
		}

		// NOTE Apr 25, 2007: <pvl> we're done with this link, reset the flag
		pCheckWalkabilityState->computeHeightAlongTheLink = false;
	}

	return walkable;
}

//===================================================================
// ModifyNodeConnections
//===================================================================
void CWaypointHumanNavRegion::ModifyNodeConnections(unsigned nodeIndex1, EAICollisionEntities collisionEntities, unsigned link, bool adjustOriginalEditorLinks)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	GraphNode* pNode1 = m_pGraph->GetNodeManager().GetNode(nodeIndex1);
	unsigned nodeIndex2 = m_pGraph->GetLinkManager().GetNextNode(link);
  GraphNode *pNode2 = m_pGraph->GetNodeManager().GetNode(nodeIndex2);
  if (pNode2 < pNode1)
  {
    return;
  }
  else
  {
    float origRadius = m_pGraph->GetLinkManager().GetOrigRadius(link);
    EWaypointLinkType origType = SWaypointNavData::GetLinkTypeFromRadius(origRadius);
    float curRadius = m_pGraph->GetLinkManager().GetRadius(link);
    EWaypointLinkType curType = SWaypointNavData::GetLinkTypeFromRadius(curRadius);

    bool walkabilityFromTo;

    if (origType == WLT_EDITOR_IMPASS)
    {
      // don't over-ride editor impassability
      walkabilityFromTo = false;
    }
    else
    {
      const SpecialArea *sa = gAIEnv.pNavigation->GetSpecialArea(pNode1->GetWaypointNavData()->nBuildingID);
      if (!sa)
        return;
      const ListPositions	& buildingPolygon = sa->GetPolygon();

      Vec3 nodeOffset(0, 0, 0.5f);
      walkabilityFromTo = CheckWalkability(
        pNode1->GetPos() + nodeOffset, 
        pNode2->GetPos() + nodeOffset, 
        0.0f, true, buildingPolygon, collisionEntities,
				GetCachedPassabilityResult(link, true));
    }

    if ( (walkabilityFromTo && curRadius < 0.0f) || (!walkabilityFromTo && curRadius > 0.0f) )
    {
      float newRadius = 0.0f;

      if (curType == WLT_AUTO_IMPASS || curType == WLT_AUTO_PASS)
      {
        newRadius = walkabilityFromTo ? (float)WLT_AUTO_PASS : (float)WLT_AUTO_IMPASS;
        unsigned linkOneTwo = 0;
        unsigned linkTwoOne = 0;
        m_pGraph->Connect(nodeIndex1, nodeIndex2, newRadius, newRadius, &linkOneTwo, &linkTwoOne);
        if (origType == WLT_AUTO_PASS && adjustOriginalEditorLinks)
        {
          if (linkOneTwo)
            m_pGraph->GetLinkManager().SetRadius(linkOneTwo, newRadius);
          if (linkTwoOne)
            m_pGraph->GetLinkManager().SetRadius(linkTwoOne, newRadius);
        }
      }
      else if (curType == WLT_EDITOR_IMPASS || curType == WLT_EDITOR_PASS)
      {
        newRadius = walkabilityFromTo ? (float)WLT_EDITOR_PASS : (float)WLT_EDITOR_IMPASS;
        m_pGraph->Connect(nodeIndex1, nodeIndex2, newRadius, newRadius);
      }
    } // walkability has changed 
  } // link pointer
}

// NOTE Mai 6, 2007: <pvl> my dbg stuff - keeping it in for now, might be
// useful in future
/*
#include <cstdio>

class ModConnTimer {
		CTimeValue mStart;
		static float sAccumTime;
		static int sInvocations;
		static FILE * sFile;
public:
		ModConnTimer()
		{
			mStart = gEnv->pTimer->GetAsyncTime();
			++sInvocations;
			if (sFile == 0) sFile = fopen ("mrd", "w");
		}
		~ModConnTimer()
		{
			CTimeValue end = gEnv->pTimer->GetAsyncTime();
			sAccumTime += (end - mStart).GetMilliSeconds();

			fprintf (sFile, "modconn ran for %f ms\n", (end-mStart).GetMilliSeconds());
			fflush (sFile);
		}
		static void Reset ()
		{
			fprintf (sFile, "%f - %d\n", sAccumTime, sInvocations);
			fflush (sFile);
			sAccumTime = 0.0f;
			sInvocations = 0;
		}
};

float ModConnTimer::sAccumTime = 0.0f;
int ModConnTimer::sInvocations = 0;
FILE * ModConnTimer::sFile = 0;
*/

//====================================================================
// ModifyConnections
//====================================================================
void CWaypointHumanNavRegion::ModifyConnections(const CCalculationStopper& stopper, EAICollisionEntities collisionEntities)
{
//  ModConnTimer c;

  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

  static int numIterPerWalkabilityCheck = 4;

//  static unsigned firstNode;
//  static bool firstNodeLeft = false;

  CAllNodesContainer& allNodes = m_pGraph->GetAllNodes();
  if (!m_currentNodeIt)
  {
    m_currentNodeIt = new CAllNodesContainer::Iterator(allNodes, IAISystem::NAV_WAYPOINT_HUMAN);
		m_currentLink = (m_currentNodeIt->GetNode() ? m_pGraph->GetNodeManager().GetNode(m_currentNodeIt->GetNode())->firstLinkIndex : 0);
    m_checkWalkabilityState.state = SCheckWalkabilityState::CWS_NEW_QUERY;
    m_checkWalkabilityState.numIterationsPerCheck = numIterPerWalkabilityCheck;

//    firstNode = m_currentNodeIt->GetNode();
  }

  unsigned origNodeIndex = 0;
  bool origNodeLeft = false;

  do
  {
/*
		if (m_currentNodeIt->GetNode() == firstNode && firstNodeLeft) {
			c.Reset();
			firstNodeLeft = false;
		}
*/

    unsigned nodeIndex1 = m_currentNodeIt->GetNode();
    if (!nodeIndex1)
    {
      m_checkWalkabilityState.state = SCheckWalkabilityState::CWS_NEW_QUERY;
      m_currentNodeIt->Reset();
      nodeIndex1 = m_currentNodeIt->GetNode();
			m_currentLink = (nodeIndex1 ? m_pGraph->GetNodeManager().GetNode(nodeIndex1)->firstLinkIndex : 0);
      if (!nodeIndex1)
        return;
    }

    // prevent sitting in a wasteful loop if there's no more to do
    if (!origNodeIndex)
      origNodeIndex = nodeIndex1;
    else if (origNodeIndex == nodeIndex1 && origNodeLeft)
      return;

		GraphNode* pNode1 = m_pGraph->GetNodeManager().GetNode(nodeIndex1);

    int nBuildingID1 = pNode1->GetWaypointNavData()->nBuildingID;
    const SpecialArea *sa1 = gAIEnv.pNavigation->GetSpecialArea(nBuildingID1);

    if (!m_currentLink || !sa1)
    {
      m_checkWalkabilityState.state = SCheckWalkabilityState::CWS_NEW_QUERY;
      (void) m_currentNodeIt->Increment(); // may return 0
			if (m_currentNodeIt->GetNode())
	      m_currentLink = m_pGraph->GetNodeManager().GetNode(m_currentNodeIt->GetNode())->firstLinkIndex;;
      origNodeLeft = true;
//			firstNodeLeft = true;
    }
    else
    {
      unsigned link = m_currentLink;
			unsigned nodeIndex2 = m_pGraph->GetLinkManager().GetNextNode(link);
      GraphNode *pNode2 = m_pGraph->GetNodeManager().GetNode(nodeIndex2);

      if (pNode2->navType != IAISystem::NAV_WAYPOINT_HUMAN)
      {
        m_currentLink = m_pGraph->GetLinkManager().GetNextLink(m_currentLink);
        continue;
      }

      int nBuildingID2 = pNode2->GetWaypointNavData()->nBuildingID;
      const SpecialArea *sa2 = gAIEnv.pNavigation->GetSpecialArea(nBuildingID1);
      if (!sa2)
      {
        m_currentLink = m_pGraph->GetLinkManager().GetNextLink(m_currentLink);
        continue;
      }

      bool auto1 = sa1->waypointConnections == WPCON_DESIGNER_PARTIAL || sa1->waypointConnections == WPCON_AUTO_PARTIAL;
      bool auto2 = sa2->waypointConnections == WPCON_DESIGNER_PARTIAL || sa2->waypointConnections == WPCON_AUTO_PARTIAL;

      // only check one way for each pair
      // also only check if at least one is auto
      if (pNode2 < pNode1 || (!auto1 && !auto2))
      {
        m_currentLink = m_pGraph->GetLinkManager().GetNextLink(m_currentLink); // only check one way for each pair
        continue;
      }
      else
      {
        float origRadius = m_pGraph->GetLinkManager().GetOrigRadius(link);
        EWaypointLinkType origType = SWaypointNavData::GetLinkTypeFromRadius(origRadius);
        float curRadius = m_pGraph->GetLinkManager().GetRadius(link);
        EWaypointLinkType curType = SWaypointNavData::GetLinkTypeFromRadius(curRadius);

        bool walkabilityFromTo;

        if (origType == WLT_EDITOR_IMPASS)
        {
          // don't over-ride editor impassability
          walkabilityFromTo = false;
        }
        else
        {
          float distToPlayerSq = 0.0f;
          if (GetAISystem()->GetPlayer())
          {
            Vec3 playerPos = GetAISystem()->GetPlayer()->GetPos();
            Vec3 delta1 = pNode1->GetPos() - playerPos;
            Vec3 delta2 = pNode2->GetPos() - playerPos;
            delta1.z *= 3.0f;
            delta2.z *= 3.0f;
            distToPlayerSq = min(delta1.GetLengthSquared(), delta2.GetLengthSquared());
          }
          static float criticalDistToPlayerSq = square(50.0f);
          if (distToPlayerSq > criticalDistToPlayerSq)
          {
            walkabilityFromTo = m_pGraph->GetLinkManager().GetRadius(link) > 0;
            m_checkWalkabilityState.state = SCheckWalkabilityState::CWS_NEW_QUERY;
          }
          else
          {
            const ListPositions	& buildingPolygon = sa1->GetPolygon();

            // the state should be OK from the last call
moreCalcs:
            Vec3 nodeOffset(0, 0, 0.5f);
            AIAssert(m_checkWalkabilityState.state != SCheckWalkabilityState::CWS_RESULT);
            walkabilityFromTo = CheckPassability(link,
              pNode1->GetPos() + nodeOffset, 
              pNode2->GetPos() + nodeOffset, 
              0.0f, true, buildingPolygon, collisionEntities,
              GetCachedPassabilityResult(link, true),
              &m_checkWalkabilityState);
            if (m_checkWalkabilityState.state == SCheckWalkabilityState::CWS_PROCESSING)
            {
              if (stopper.ShouldCalculationStop())
                return;
              goto moreCalcs;
            }
            m_checkWalkabilityState.state = SCheckWalkabilityState::CWS_NEW_QUERY;
          }
        }

        if ( (walkabilityFromTo && curRadius < 0.0f) || (!walkabilityFromTo && curRadius > 0.0f) )
        {
          float newRadius = 0.0f;

          if (curType == WLT_AUTO_IMPASS || curType == WLT_AUTO_PASS)
          {
            newRadius = walkabilityFromTo ? (float)WLT_AUTO_PASS : (float)WLT_AUTO_IMPASS;
            m_pGraph->Connect(nodeIndex1, nodeIndex2, newRadius, newRadius);
          }
          else if (curType == WLT_EDITOR_IMPASS || curType == WLT_EDITOR_PASS)
          {
            newRadius = walkabilityFromTo ? (float)WLT_EDITOR_PASS : (float)WLT_EDITOR_IMPASS;
            m_pGraph->Connect(nodeIndex1, nodeIndex2, newRadius, newRadius);
          }
        } // link needs changing
      } // check other node pointer
      m_currentLink = m_pGraph->GetLinkManager().GetNextLink(m_currentLink);
    } // link index is valid
  } while (!stopper.ShouldCalculationStop());
}































































































































//====================================================================
// Update
//====================================================================
void CWaypointHumanNavRegion::Update(CCalculationStopper& stopper)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

  ModifyConnections(stopper, /*AICE_ALL_EXCEPT_TERRAIN_AND_STATIC*/AICE_ALL);
}

//====================================================================
// Reset
//====================================================================
void CWaypointHumanNavRegion::Reset(IAISystem::EResetReason reason)
{
	PassabilityCache cache;
	m_passabilityCache.swap(cache);
}

//===================================================================
// GetTeleportPosition
//===================================================================
bool CWaypointHumanNavRegion::GetTeleportPosition(const Vec3 &pos, Vec3 &teleportPos, const char *requesterName)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

  teleportPos.zero();

  IVisArea *pGoalArea; 
  int	nBuildingID;
  gAIEnv.pNavigation->CheckNavigationType(pos, nBuildingID, pGoalArea, IAISystem::NAV_WAYPOINT_HUMAN);

  if (nBuildingID < 0)
  {
    AIWarning("CWaypointHumanNavRegion::GetTeleportPosition found bad building ID = %d (%s) for %s (%5.2f, %5.2f, %5.2f)", 
      nBuildingID, gAIEnv.pNavigation->GetNavigationShapeName(nBuildingID), requesterName, pos.x, pos.y, pos.z);
    return false;
  }

  const SpecialArea *sa = gAIEnv.pNavigation->GetSpecialArea(nBuildingID);
  if (!sa)
  {
    AIWarning("CWaypointHumanNavRegion::GetTeleportPosition found no area for %s (%5.2f, %5.2f, %5.2f)", 
      requesterName, pos.x, pos.y, pos.z);
    return false;
  }

  if (sa->fNodeAutoConnectDistance < 0.0001f)
    AIWarning("CWaypointHumanNavRegion::GetTeleportPosition AI nav modifier %s has NodeAutoConnectDistance=0 - increase to a sensible value [design bug]", gAIEnv.pNavigation->GetNavigationShapeName(nBuildingID));

  float totalDist = 5 * sa->fNodeAutoConnectDistance + 5.0f;

  typedef std::vector< std::pair<float, unsigned> > TNodes;
  static TNodes nodes;
  nodes.resize(0);

  // Most times we'll find the desired node close by
  CAllNodesContainer &allNodes = m_pGraph->GetAllNodes();
  allNodes.GetAllNodesWithinRange(nodes, pos, totalDist, IAISystem::NAV_WAYPOINT_HUMAN);
  std::vector< std::pair<float, unsigned> >::iterator it = std::remove_if(nodes.begin(), nodes.end(), SWrongBuildingRadius(m_pGraph->GetLinkManager(), m_pGraph->GetNodeManager(), nBuildingID, 0.4f));
  nodes.erase(it, nodes.end());

  if (nodes.empty())
  {
    AILogComment("CWaypointHumanNavRegion::GetTeleportPosition Unable to find node close to %s (%5.2f, %5.2f, %5.2f) - checking all nodes from building id %d (%s)",
      requesterName, pos.x, pos.y, pos.z, nBuildingID, gAIEnv.pNavigation->GetNavigationShapeName(nBuildingID));

    CAllNodesContainer::Iterator nit(allNodes, IAISystem::NAV_WAYPOINT_HUMAN);
    while (unsigned currentIndex = nit.Increment())
    {
			GraphNode* pCurrent = m_pGraph->GetNodeManager().GetNode(currentIndex);
			if (pCurrent->GetWaypointNavData()->nBuildingID == nBuildingID && !GetAISystem()->ExitNodeImpossible(m_pGraph->GetLinkManager(), pCurrent, 0.4f))
      {
        float dist = Distance::Point_Point(pos, pCurrent->GetPos());
        nodes.push_back(std::make_pair(dist, currentIndex)); // dist calculated later
      }
    }
  }

  if (nodes.empty())
  {
    AIWarning("CWaypointHumanNavRegion::GetTeleportPosition No nodes found for waypoint area containing %s (%5.2f, %5.2f, %5.2f) building id %d (%s) [design bug]", 
      requesterName, pos.x, pos.y, pos.z, nBuildingID, gAIEnv.pNavigation->GetNavigationShapeName(nBuildingID));
    return false;
  }

  std::sort(nodes.begin(), nodes.end());

  for (TNodes::const_iterator it2 = nodes.begin() ; it2 != nodes.end() ; ++it2)
  {
		unsigned nodeIndex = it2->second;
    const GraphNode *pNode = m_pGraph->GetNodeManager().GetNode(nodeIndex);
    Vec3 nodePos = pNode->GetPos();
    if (!CheckBodyPos(nodePos, AICE_ALL))
      continue;
    if (GetAISystem()->WouldHumanBeVisible(nodePos, false))
      continue;
    teleportPos = nodePos;
    return true;
  }
  return false;
}

/*
void CWaypointHumanNavRegion::CheckLinkLengths() const
{
	LinkIterator linkIt = CreateLinkIterator();
	for ( ; linkIt; ++linkIt)
	{
		unsigned int linkIndex = *linkIt;
		unsigned nodeIndex0 = m_pGraph->GetLinkManager().GetPrevNode(linkIndex);
		unsigned nodeIndex1 = m_pGraph->GetLinkManager().GetNextNode(linkIndex);
		GraphNode * node0 = m_pGraph->GetNodeManager().GetNode(nodeIndex0);
		GraphNode * node1 = m_pGraph->GetNodeManager().GetNode(nodeIndex1);
		AIWarning ("Link length %f\n", (node0->GetPos() - node1->GetPos()).len());
	}
}
*/

CWaypointHumanNavRegion::LinkIterator::LinkIterator (const CWaypointHumanNavRegion * wayptRegion) :
		m_pGraph (wayptRegion->m_pGraph),
		m_currNodeIt(new CAllNodesContainer::Iterator(m_pGraph->GetAllNodes(), IAISystem::NAV_WAYPOINT_HUMAN)),
		m_currLinkIndex (m_pGraph->GetNodeManager().GetNode(m_currNodeIt->GetNode())->firstLinkIndex)
{
	FindNextWaypointLink();
}

CWaypointHumanNavRegion::LinkIterator::LinkIterator (const LinkIterator & rhs) :
		m_pGraph (rhs.m_pGraph),
		m_currNodeIt (new CAllNodesContainer::Iterator(*rhs.m_currNodeIt)),
		m_currLinkIndex (rhs.m_currLinkIndex)
{
}

CWaypointHumanNavRegion::LinkIterator & CWaypointHumanNavRegion::LinkIterator::operator++ ()
{
	m_currLinkIndex = m_pGraph->GetLinkManager().GetNextLink(m_currLinkIndex);
	FindNextWaypointLink();
	return *this;
}

/**
 * This function finds the next waypoint link, starting with and including the
 * current value of m_currLinkIndex.  m_currLinkIndex should denote
 * a valid link upon the call even though not necessarily a waypoint one.
 * Note that if m_currLinkIndex does point to a waypoint link, this function
 * does nothing.
 */
void CWaypointHumanNavRegion::LinkIterator::FindNextWaypointLink ()
{
	unsigned int currNodeIndex = m_currNodeIt->GetNode();
	bool gotSuitableLink = false;

	while ( ! gotSuitableLink)
	{
		while (m_currLinkIndex == 0)
		{
			m_currNodeIt->Increment();
			currNodeIndex = m_currNodeIt->GetNode();
			if (0 == currNodeIndex)
				return;		// reached the final node, the iteration is over
			m_currLinkIndex = m_pGraph->GetNodeManager().GetNode(currNodeIndex)->firstLinkIndex;
		}

		unsigned nextNodeIndex = m_pGraph->GetLinkManager().GetNextNode(m_currLinkIndex);

		const GraphNode * currNode = m_pGraph->GetNodeManager().GetNode(currNodeIndex);
		const GraphNode * nextNode = m_pGraph->GetNodeManager().GetNode(nextNodeIndex);

		if (nextNode->navType != IAISystem::NAV_WAYPOINT_HUMAN || currNode > nextNode /*check in one direction only*/)
		{
			m_currLinkIndex = m_pGraph->GetLinkManager().GetNextLink(m_currLinkIndex);
			continue;
		}

		gotSuitableLink = true;
	}
}

//====================================================================
// GetCachedPassabilityResult
//====================================================================
SCachedPassabilityResult* CWaypointHumanNavRegion::GetCachedPassabilityResult(unsigned graphLink, bool bCreate)
{
	PassabilityCache::iterator it = m_passabilityCache.find(graphLink);
	if (it == m_passabilityCache.end() && bCreate)
		it = m_passabilityCache.insert(std::make_pair(graphLink, SCachedPassabilityResult())).first;

	return (it != m_passabilityCache.end() ? &(*it).second : 0);
}
