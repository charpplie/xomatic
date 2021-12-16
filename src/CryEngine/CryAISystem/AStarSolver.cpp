#include "StdAfx.h"
#include "AStarSolver.h"
#include "AILog.h"
#include "Graph.h"
#include "ISerialize.h"
#include "ObjectTracker.h"
#include "FrameProfiler.h"
#include "CAISystem.h"
#include "CalculationStopper.h"

static float sCostOffset = 0.001f;

//====================================================================
// CAStarSolver
//====================================================================
CAStarSolver::CAStarSolver(CGraphNodeManager& nodeManager)
:	m_AStarNodeManager(nodeManager), m_bDebugDrawOpenList(false)
{
  // should really be the size of the graph?
	m_AStarNodeManager.ReserveMemory(10000);
  AbortAStar();
}

//====================================================================
// AbortAStar
//====================================================================
void CAStarSolver::AbortAStar()
{
  m_request.m_pGraph = 0;
  m_request.m_startIndex = m_request.m_endIndex = 0;
  m_request.m_pHeuristic = 0;
	m_pathfinderCurrent = 0;
	m_AStarNodeManager.Clear();
  m_pathNodes.resize(0);
}

//====================================================================
// SetupAStar
//====================================================================
EAStarResult CAStarSolver::SetupAStar(CCalculationStopper& stopper,
                                      const CGraph* pGraph, 
                                      const CHeuristic* pHeuristic,
                                      unsigned startIndex, unsigned endIndex,
                                      const NavigationBlockers& navigationBlockers,
																			bool bDebugDrawOpenList)
{
  // Check params
  if (!startIndex || !endIndex || !pGraph || !pHeuristic) 
    return ASTAR_NOPATH;

  // Tidy up
  m_pathNodes.resize(0);
	m_AStarNodeManager.Clear();
  m_navigationBlockers = navigationBlockers;

  // store state ready for timeslicing
  m_bWalkingBack = false;
  m_request.m_pGraph = pGraph;
  m_request.m_startIndex = startIndex;
  m_request.m_endIndex = endIndex;
  m_request.m_pHeuristic = pHeuristic;

	m_bDebugDrawOpenList = bDebugDrawOpenList;

  // let's start
	m_pathfinderCurrent = m_AStarNodeManager.GetAStarNode(startIndex);
	m_pathfinderCurrent->fCostFromStart = 0.0f;

	AStarSearchNode* pEnd = m_AStarNodeManager.GetAStarNode(endIndex);

	m_pathfinderCurrent->fEstimatedCostToGoal = pHeuristic->EstimateCost(*m_pathfinderCurrent, *pEnd);
	m_AStarNodeManager.AddNodeToOpenList(m_pathfinderCurrent);

  return ASTAR_STILLSOLVING;
}

//====================================================================
// ContinueAStar
//====================================================================
EAStarResult CAStarSolver::ContinueAStar(CCalculationStopper& stopper)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );
  if (!m_request.m_pGraph)
  {
    AIWarning("CAStarSolver::ContinueAStar called but no graph");
    return ASTAR_NOPATH;
  }

  if (m_bWalkingBack)
    return WalkBack(stopper);

  while (!stopper.ShouldCalculationStop() && 
		!m_AStarNodeManager.IsEmpty() &&
		m_pathfinderCurrent->nodeIndex != m_request.m_endIndex &&
		m_pathfinderCurrent->nodeIndex)
  {
    ASTARStep();
  }

	if (m_pathfinderCurrent->nodeIndex == m_request.m_endIndex)
    m_bWalkingBack = true;
	else if (m_AStarNodeManager.IsEmpty())
		m_pathfinderCurrent = 0;

	if (!m_pathfinderCurrent)
    return ASTAR_NOPATH;

  return ASTAR_STILLSOLVING;
}

//====================================================================
// ASTARStep
//====================================================================
void CAStarSolver::ASTARStep()
{
	if (m_AStarNodeManager.IsEmpty())
  {
    AIWarning("Path not found");
		m_pathfinderCurrent = 0;
    return;
  }

	m_pathfinderCurrent = m_AStarNodeManager.PopBestNodeFromOpenList();

	if (m_request.m_endIndex == m_pathfinderCurrent->nodeIndex) 
  {
    // path found
		m_pathfinderCurrent = m_AStarNodeManager.GetAStarNode(m_request.m_endIndex);
    return;
  }

	const AStarSearchNode* pPathfinderCurrent = m_pathfinderCurrent;

  // now walk through all the nodes connected to this one
	for (unsigned linkId = pPathfinderCurrent->graphNode->firstLinkIndex ; linkId ; linkId = m_request.m_pGraph->GetLinkManager().GetNextLink(linkId))
  {
		unsigned nextNodeIndex = m_request.m_pGraph->GetLinkManager().GetNextNode(linkId);
		AStarSearchNode* nextNode = m_AStarNodeManager.GetAStarNode(nextNodeIndex);

		//AIAssert(nextNode);//TODO what to replace this with?
    float linkCost = m_request.m_pHeuristic->CalculateCost(m_request.m_pGraph, 
			*m_pathfinderCurrent, linkId, *nextNode, m_navigationBlockers);
    // Checking of node type etc against the requester is done in the heuristic - it should return < 0 if
    // the link is completely impassable
    if (linkCost >= 0.0f)
    {
      // update this next node
      // Add a tiny offset to avoid cycles during walkback - in case two nodes had zero cost from one to the next
			float nextCostFromStart = sCostOffset + m_pathfinderCurrent->fCostFromStart + linkCost;
			const AStarSearchNode* pEnd = m_AStarNodeManager.GetAStarNode(m_request.m_endIndex);
      float nextEstimatedCostToGoal = m_request.m_pHeuristic->EstimateCost(*nextNode, *pEnd);
      float totalCost = nextCostFromStart + nextEstimatedCostToGoal;

      // only valid if next is tagged...
      float origTotalCost = nextNode->fCostFromStart + nextNode->fEstimatedCostToGoal;
			if (nextNode->IsTagged() && origTotalCost <= totalCost)
      {
        // old version was better	
      }
      else
      {
				m_AStarNodeManager.UpdateNode(nextNodeIndex, nextCostFromStart, nextEstimatedCostToGoal);
				nextNode->prevPathNodeIndex = m_pathfinderCurrent;
      }

			if (!nextNode->IsTagged())
      {
				if (m_bDebugDrawOpenList)
				{
					Vec3 p0 = nextNode->graphNode->GetPos();
					Vec3 p1 = pPathfinderCurrent->graphNode->GetPos();
					p0.z += 0.2f;
					p1.z += 0.2f;
					GetAISystem()->AddDebugSphere(p0, 0.1f, 255,255,255, 10.0f);
					GetAISystem()->AddDebugLine(p1, p0, 255,255,255, 10.0f);
				}

				m_AStarNodeManager.AddNodeToOpenList(nextNode);
      }
    } // check for link passability
  } // loop over all nodes connected to m_pPathfinderCurrent

  return;
}

//====================================================================
// WalkBack
// When we start walking back m_pPathfinderCurrent must be = m_request.m_pEnd
// since we use a std::vector we push the reverse path onto the back - and
// then reverse at the end
//====================================================================
EAStarResult CAStarSolver::WalkBack(const CCalculationStopper& stopper)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

  if (!m_request.m_pGraph)
    return ASTAR_NOPATH;

  std::set<const GraphNode *> nodesToProcess;

	while (!stopper.ShouldCalculationStop() && m_pathfinderCurrent->nodeIndex && m_pathfinderCurrent->nodeIndex != m_request.m_startIndex)
  {
		m_pathNodes.push_back(m_pathfinderCurrent->nodeIndex);

		unsigned prevNodeIndex = m_pathfinderCurrent->prevPathNodeIndex != 0 ? m_pathfinderCurrent->prevPathNodeIndex->nodeIndex : 0;
    if (prevNodeIndex == 0)
    {
			const GraphNode* pStart = m_request.m_pGraph->GetNodeManager().GetNode(m_request.m_startIndex);
			const GraphNode* pEnd = m_request.m_pGraph->GetNodeManager().GetNode(m_request.m_endIndex);
      AIWarning("AStar: Unable to walk back - path nodes from (%5.2f, %5.2f, %5.2f) to (%5.2f, %5.2f, %5.2f) - managed %d nodes!",
        pStart->GetPos().x, pStart->GetPos().y, pStart->GetPos().z,
        pEnd->GetPos().x, pEnd->GetPos().y, pEnd->GetPos().z,
        m_pathNodes.size());
      m_pathNodes.clear();
      return ASTAR_NOPATH;
    }
		m_pathfinderCurrent = m_AStarNodeManager.GetAStarNode(prevNodeIndex);
  }

	if (m_pathfinderCurrent->nodeIndex == m_request.m_startIndex)
  {
		m_pathfinderCurrent = 0;
    if (std::find(m_pathNodes.begin(), m_pathNodes.end(), m_request.m_startIndex) == m_pathNodes.end())
    {
      m_pathNodes.push_back(m_request.m_startIndex);
    }
    std::reverse(m_pathNodes.begin(), m_pathNodes.end());
    return ASTAR_PATHFOUND;
  }
  else
  {
    return ASTAR_STILLSOLVING;
  }
}

//====================================================================
// CalculateAndGetPartialPath
// Just find the best tagged node, then walk back from there
//====================================================================
const CAStarSolver::tPathNodes& CAStarSolver::CalculateAndGetPartialPath()
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );
  m_pathNodes.resize(0);
  if (!m_request.m_pGraph)
  {
    AILogEvent("Partial path requested but graph is 0 (due to path request timeout?)");
    return m_pathNodes;
  }
	const AStarSearchNodeVector& taggedNodes = m_AStarNodeManager.GetTaggedNodesVector();
  if (taggedNodes.empty())
    return m_pathNodes;

	const AStarSearchNode* pTaggedNode0 = taggedNodes[0];
  float bestEstimate = pTaggedNode0->fEstimatedCostToGoal;
	m_pathfinderCurrent = taggedNodes[0];
  for (uint32 i = 1 ; i < taggedNodes.size() ; ++i)
  {
		const AStarSearchNode* pTaggedNode = taggedNodes[i];
    if (pTaggedNode->fEstimatedCostToGoal < bestEstimate)
    {
      bestEstimate = pTaggedNode->fEstimatedCostToGoal;
			m_pathfinderCurrent = taggedNodes[i];
    }
  }

  m_bWalkingBack = true;

  EAStarResult result = WalkBack(CCalculationStopper("A*Walkback", 1.0f, 10000000));
  if (result != ASTAR_PATHFOUND)
    m_pathNodes.resize(0);

  return m_pathNodes;
}

//====================================================================
// Serialize
//====================================================================
void CAStarSolver::Serialize(TSerialize ser, CObjectTracker& objectTracker)
{
  if (ser.IsReading())
    AbortAStar();

  ser.BeginGroup("AStarSolver");

  // shouldn't need to store the path since normally the requester will copy it
  // from us immediately it's generated.

  bool gotRequest = (m_request.m_pGraph != 0);
  ser.Value("gotRequest", gotRequest);

  if (gotRequest)
  {
    objectTracker.SerializeObjectPointer(ser, "graph", m_request.m_pGraph, false);
    objectTracker.SerializeObjectPointer(ser, "heuristic", m_request.m_pHeuristic, false);

    if (m_request.m_pGraph)
    {
      m_request.m_pGraph->SerializeNodePointer(ser, "startID", m_request.m_startIndex);
      m_request.m_pGraph->SerializeNodePointer(ser, "endID", m_request.m_endIndex);

      if (!m_request.m_startIndex || !m_request.m_endIndex)
      {
        AIWarning("Unable to retrieve start/end nodes in A* serialisation");
        m_request.m_pGraph = 0;
      }
    }
  }
  ser.EndGroup();
}
