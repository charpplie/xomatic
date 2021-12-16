// Heuristic.cpp: implementation of the CHeuristic class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "IAgent.h"
#include "Heuristic.h"
#include "Graph.h"
#include "AIObject.h"
#include "Cry_Math.h"
#include "AILog.h"
#include "CAISystem.h"
#include "VolumeNavRegion.h"
#include "Navigation/CustomNavRegion.h"
#include "LayeredNavMesh/LayeredNavMeshRegion.h"

#include <limits>

/// For debugging - over-ride all the complicated stuff and just use pure distance
static bool gUseSimpleHeuristic = false;

//====================================================================
// CStandardHeuristic
//====================================================================
CStandardHeuristic::CStandardHeuristic() : m_lnmHeuristic(0)
{
  SetProperties(m_properties);
  SetStartEndData(0, ZERO, 0, ZERO, PathfindingExtraConstraints());
}

//====================================================================
// SetStartEndData
//====================================================================
void CStandardHeuristic::SetStartEndData(const GraphNode *pStartNode, const Vec3 &startPos, const GraphNode *pEndNode, const Vec3 &endPos, const PathfindingExtraConstraints &extraConstraints)
{
  m_pStartNode = pStartNode;
  m_pEndNode = pEndNode;
  m_startPos = startPos;
  m_endPos = endPos;
	m_extraConstraints=extraConstraints;

	if (pStartNode && pStartNode->navType == IAISystem::NAV_LAYERED_NAV_MESH)
	{
		// NOTE Dec 4, 2009: <pvl> if any is LNM both should be - at least for the time being
		assert (pEndNode->navType == IAISystem::NAV_LAYERED_NAV_MESH);
		delete m_lnmHeuristic;
		m_lnmHeuristic = new CLayeredNavMeshHeuristic (
				gAIEnv.pGraph,
				gAIEnv.pNavigation->GetLayeredNavMeshRegion()->GetModifierByIndexAndType (
						pStartNode->GetLayeredMeshNavData()->navModifIndex,
						pStartNode->GetLayeredMeshNavData()->agentType
				),
				this
		);
	}
}


//====================================================================
// GetNodePos
//====================================================================
const Vec3 &CStandardHeuristic::GetNodePos(const GraphNode &node) const
{
  if (&node == m_pStartNode)
    return m_startPos;
  else if (&node == m_pEndNode)
    return m_endPos;
  else return node.GetPos();
}


//====================================================================
// SetRequesterProperties
//====================================================================
void CStandardHeuristic::SetProperties(const PathfindingHeuristicProperties &properties) 
{
  m_properties = properties;

  m_fMinExtraFactor = std::numeric_limits<float>::max();
  if ( (m_properties.agentproperties.navCapMask & IAISystem::NAV_TRIANGULAR) &&
    m_properties.agentproperties.triangularResistanceFactor < m_fMinExtraFactor)
    m_fMinExtraFactor = properties.agentproperties.triangularResistanceFactor;

  if ( (properties.agentproperties.navCapMask & IAISystem::NAV_WAYPOINT_HUMAN) &&
    properties.agentproperties.waypointResistanceFactor < m_fMinExtraFactor)
    m_fMinExtraFactor = properties.agentproperties.waypointResistanceFactor;

  if ( (properties.agentproperties.navCapMask & IAISystem::NAV_WAYPOINT_3DSURFACE) &&
    properties.agentproperties.waypointResistanceFactor < m_fMinExtraFactor)
    m_fMinExtraFactor = properties.agentproperties.waypointResistanceFactor;

  if ( (properties.agentproperties.navCapMask & IAISystem::NAV_FLIGHT) &&
    properties.agentproperties.flightResistanceFactor < m_fMinExtraFactor)
    m_fMinExtraFactor = properties.agentproperties.flightResistanceFactor;

  if ( (properties.agentproperties.navCapMask & IAISystem::NAV_VOLUME) &&
    properties.agentproperties.volumeResistanceFactor < m_fMinExtraFactor)
    m_fMinExtraFactor = properties.agentproperties.volumeResistanceFactor;

  if ( (properties.agentproperties.navCapMask & IAISystem::NAV_ROAD) &&
    properties.agentproperties.roadResistanceFactor < m_fMinExtraFactor)
    m_fMinExtraFactor = properties.agentproperties.roadResistanceFactor;

}

//====================================================================
// EstimateCost
//====================================================================
float CStandardHeuristic::EstimateCost(const AStarSearchNode & node0, const AStarSearchNode & node1) const
{
  if (node0.graphNode->navType == IAISystem::NAV_LAYERED_NAV_MESH && node1.graphNode->navType == IAISystem::NAV_LAYERED_NAV_MESH)
    return m_lnmHeuristic->EstimateCost (node0, node1);

  if (gUseSimpleHeuristic)
  {
		return Distance::Point_Point(GetNodePos(*node1.graphNode), GetNodePos(*node0.graphNode));
  }
  else
  {
		Vec3 delta = (GetNodePos(*node1.graphNode) - GetNodePos(*node0.graphNode));
    delta.z *= m_properties.agentproperties.zScale;
    return (1.0f + m_fMinExtraFactor) * delta.GetLength();
  }
}

//====================================================================
// CalculateCost
//====================================================================
float CStandardHeuristic::CalculateCost(const CGraph * graph,
										const AStarSearchNode & node0, unsigned linkIndex0,
										const AStarSearchNode & node1,
                                        const NavigationBlockers& navigationBlockers) const
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	if (node0.graphNode->navType != IAISystem::NAV_LAYERED_NAV_MESH && node1.graphNode->navType == IAISystem::NAV_LAYERED_NAV_MESH)
	{
		assert (node0.graphNode->navType == IAISystem::NAV_SMARTOBJECT);
		m_lnmHeuristic->AddEntryPoint (node0, node1, node1.graphNode->GetPos());
	}
	if (node0.graphNode->navType == IAISystem::NAV_LAYERED_NAV_MESH && node1.graphNode->navType == IAISystem::NAV_LAYERED_NAV_MESH)
		return m_lnmHeuristic->CalculateCost (graph, node0, linkIndex0, node1, navigationBlockers);

	static int maxBlockers = 0;
	if (navigationBlockers.size() > maxBlockers)
	{
		maxBlockers = navigationBlockers.size();
		if (maxBlockers > 10)
			AIWarning ("Too many (%d) navigation blockers passed to CalculateCost()", maxBlockers);
	}

  unsigned link = linkIndex0;
  if (graph->GetLinkManager().GetRadius(link) < m_properties.agentproperties.radius)
    return -1.0f;

	if (!(node1.graphNode->navType & m_properties.agentproperties.navCapMask))
    return -1.0f;

  if (gUseSimpleHeuristic)
		return (GetNodePos(*node1.graphNode) - GetNodePos(*node0.graphNode)).GetLength();

  if (graph->GetLinkManager().GetMaxWaterDepth(link) > m_properties.agentproperties.maxWaterDepth)
    return -1.0f;
  if (graph->GetLinkManager().GetMinWaterDepth(link) < m_properties.agentproperties.minWaterDepth)
    return -1.0f;

  for (int iExtraConstraint = 0 ; iExtraConstraint < m_extraConstraints.size() ; ++iExtraConstraint)
  {
    const PathfindingExtraConstraint &extraConstraint = m_extraConstraints[iExtraConstraint];
    switch (extraConstraint.type)
    {
    case PathfindingExtraConstraint::ECT_MAXCOST:
      {
        if (extraConstraint.constraint.maxCost.maxCost < node0.fCostFromStart)
          return -1.0f;
      }
      break;
    case PathfindingExtraConstraint::ECT_MINDISTFROMPOINT:
      {
        Vec3 point(extraConstraint.constraint.minDistFromPoint.px,
          extraConstraint.constraint.minDistFromPoint.py,
          extraConstraint.constraint.minDistFromPoint.pz);
				float distToPointSq = Distance::Point_PointSq(point, GetNodePos(*node0.graphNode));
        if (distToPointSq > extraConstraint.constraint.minDistFromPoint.minDistSq)
          return -1.0f;
      }
      break;
		case PathfindingExtraConstraint::ECT_AVOIDSPHERE:
			{
				Vec3 p(extraConstraint.constraint.avoidSphere.px,
					extraConstraint.constraint.avoidSphere.py,
					extraConstraint.constraint.avoidSphere.pz);
				Lineseg lineseg(GetNodePos(*node0.graphNode), GetNodePos(*node1.graphNode));
				float distToPointSq;
				float t;
				if (node0.graphNode->navType == IAISystem::NAV_TRIANGULAR)
					distToPointSq = Distance::Point_Lineseg2DSq(p, lineseg, t);
				else
					distToPointSq = Distance::Point_LinesegSq(p, lineseg, t);
				if (distToPointSq < extraConstraint.constraint.avoidSphere.minDistSq)
					return -1.0f;
			}
			break;
		case PathfindingExtraConstraint::ECT_AVOIDCAPSULE:
			{
				Vec3 p(extraConstraint.constraint.avoidCapsule.px,
					extraConstraint.constraint.avoidCapsule.py,
					extraConstraint.constraint.avoidCapsule.pz);
				Vec3 q(extraConstraint.constraint.avoidCapsule.qx,
					extraConstraint.constraint.avoidCapsule.qy,
					extraConstraint.constraint.avoidCapsule.qz);

				Lineseg lineseg(GetNodePos(*node0.graphNode), GetNodePos(*node1.graphNode));

				float distToLineSegSq;
				if (node0.graphNode->navType == IAISystem::NAV_TRIANGULAR)
				{
					p.z = 0;
					q.z = 0;
					lineseg.start.z = 0;
					lineseg.end.z = 0;
				}
				distToLineSegSq = Distance::Lineseg_LinesegSq(lineseg, Lineseg(p, q), (float*)0, (float*)0);
				if (distToLineSegSq < extraConstraint.constraint.avoidCapsule.minDistSq)
					return -1.0f;
			}
			break;
		default:
      AIWarning("Unhandled constraint type %d", extraConstraint.type);
      break;
    }
  }

	Vec3 delta = (GetNodePos(*node1.graphNode) - GetNodePos(*node0.graphNode));

  // TODO Danny should these really be 0?! It overrides the params in the lua files, and
  // also maybe invalidates the calculation in EstimateCost
  static float fResistanceFactorTriangularNoWater = 0.5f;
  static float fResistanceFactorTriangularWater = 1.0f;
  static float fResistanceFactorWaypoint = 0.5f;
  static float fResistanceFactorFlight = 0.0f;
  static float fResistanceFactorVolume = 0.0f;
  static float fResistanceFactorRoad = 0.0f;
  static float fResistanceFactorSmartObject = 0.0f;

	const GraphNode* nodes[2] = {node0.graphNode, node1.graphNode};
  float resistanceFactor[2] = {0.0f, 0.0f};
  for (unsigned i = 0 ; i < 2 ; ++i)
  {
    const GraphNode* node = nodes[i];
    switch (node->navType)
    {
    case IAISystem::NAV_TRIANGULAR: 
      resistanceFactor[i] = fResistanceFactorTriangularNoWater * m_properties.agentproperties.triangularResistanceFactor;
      if (graph->GetLinkManager().GetMaxWaterDepth(link) > 1.5f)
        resistanceFactor[i] += fResistanceFactorTriangularWater * (m_properties.agentproperties.triangularResistanceFactor + 2);
      else if (graph->GetLinkManager().GetMaxWaterDepth(link) > 0.0f)
        resistanceFactor[i] += fResistanceFactorTriangularWater * (m_properties.agentproperties.triangularResistanceFactor + 1);
      break;
    case IAISystem::NAV_WAYPOINT_HUMAN: 
      resistanceFactor[i] = fResistanceFactorWaypoint * m_properties.agentproperties.waypointResistanceFactor;
      break;
    case IAISystem::NAV_WAYPOINT_3DSURFACE: 
      resistanceFactor[i] = fResistanceFactorWaypoint * m_properties.agentproperties.waypointResistanceFactor;
      break;
    case IAISystem::NAV_FLIGHT: 
      resistanceFactor[i] = fResistanceFactorFlight * m_properties.agentproperties.flightResistanceFactor;
      break;
    case IAISystem::NAV_VOLUME: 
      resistanceFactor[i] = fResistanceFactorVolume * m_properties.agentproperties.volumeResistanceFactor;
      break;
    case IAISystem::NAV_ROAD: 
      resistanceFactor[i] = fResistanceFactorRoad * m_properties.agentproperties.roadResistanceFactor;
      break;
    case IAISystem::NAV_SMARTOBJECT: 
      resistanceFactor[i] = fResistanceFactorSmartObject * 1.0f;
      break;
    }
  }
  float resFactor = max(resistanceFactor[0], resistanceFactor[1]);

  int nBuildingID0 = nodes[0]->navType & (IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE) ? nodes[0]->GetWaypointNavData()->nBuildingID : -2;
  int nBuildingID1 = nodes[1]->navType & (IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE) ? nodes[1]->GetWaypointNavData()->nBuildingID : -2;

  static float deltaZ = 1.0f;
  float blockerCost = 0.0f;
  float blockerMult = 0.0f;

  if (nodes[0]->navType == IAISystem::NAV_SMARTOBJECT && nodes[1]->navType == IAISystem::NAV_SMARTOBJECT)
  {
		// TODO(marcio): Make this work with normal entities.
		// Perhaps with an ISmartObjectUser interface.

		// UGLY:
		CAIObject *pRequester=0;
		if (m_properties.pAgent)
			if (IEntity*pEntity=m_properties.pAgent->GetPathAgentEntity())
				if (IAIObject *pAIObject=pEntity->GetAI())
					pRequester=pAIObject->CastToCAIActor();

    resFactor = gAIEnv.pSmartObjectManager->GetSmartObjectLinkCostFactor(nodes, pRequester);
    if ( resFactor < 0 )
      return -1;
  }
	else if(nodes[0]->navType == IAISystem::NAV_CUSTOM_NAVIGATION && nodes[1]->navType == IAISystem::NAV_CUSTOM_NAVIGATION)
	{
		resFactor = gAIEnv.pNavigation->GetCustomNavRegion()->GetCustomLinkCostFactor(nodes, m_properties);
	}
  else if (nBuildingID0 >= 0 || nBuildingID1 >= 0)
  {
    // human or 3dsurface waypoint
		Lineseg lineseg(GetNodePos(*node0.graphNode), GetNodePos(*node1.graphNode));
    unsigned nBlockers = navigationBlockers.size();
    for (unsigned i = 0 ; i < nBlockers ; ++i)
    {
      const NavigationBlocker& navBlocker = navigationBlockers[i];
      bool useBlocker = false;
      if (navBlocker.restrictedLocation)
      {
        if (!(navBlocker.navType & (IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE)))
          continue;
        if (navBlocker.location.waypoint.nBuildingID == nBuildingID0 ||
          navBlocker.location.waypoint.nBuildingID == nBuildingID1)
          useBlocker = true;
      }
      else
      {
        useBlocker = true;
      }
      if (useBlocker)
      {
        float t = 0.0f;
        float d = Distance::Point_LinesegSq(navBlocker.sphere.center, lineseg, t);
        Vec3 pos = lineseg.GetPoint(t);
        if (pos.z > navBlocker.sphere.center.z - deltaZ && pos.z < navBlocker.sphere.center.z + deltaZ)
        {
          d = navBlocker.navType == IAISystem::NAV_WAYPOINT_HUMAN ? Distance::Point_Point2DSq(pos, navBlocker.sphere.center) : Distance::Point_PointSq(pos, navBlocker.sphere.center);
          if (d < square(navBlocker.sphere.radius))
          {
            if (navBlocker.costMultMod < 0.0f)
              return -1.0f;
            float scale = navBlocker.radialDecay ? square(square(1.0f - cry_sqrtf(d) / navBlocker.sphere.radius)) : 1.0f;
            if (navBlocker.directional)
            {
              /// reduce costs that go away from point
              Vec3 segDir = (lineseg.end - lineseg.start).GetNormalizedSafe();
              Vec3 dirToBlocker = (navBlocker.sphere.center - 0.5f * (lineseg.start + lineseg.end)).GetNormalizedSafe();
              float dirScale = 1.0f - abs(segDir.Dot(dirToBlocker));
              scale *= dirScale;
            }
            blockerCost += scale * navBlocker.costAddMod * (1.0f +  m_properties.agentproperties.waypointResistanceFactor);
            blockerMult += scale * navBlocker.costMultMod;
          }
        }
      }
    }
  }
  else if (nodes[0]->navType == IAISystem::NAV_VOLUME || nodes[1]->navType == IAISystem::NAV_VOLUME)
  {
#ifdef DYNAMIC_3D_NAVIGATION
    // do a collision test to handle dynamic/moving obstacles. Assume that static geometry has already been 
    // accounted for so we just need to check dynamic objects (cheaper). The question is whether it's better
    // to do this here or in background processing. If here, then we should really cache the result.
    bool intersect = 
      GetAISystem()->GetVolumeNavRegion()->PathSegmentWorldIntersection(node0.GetPos(), link.vEdgeCenter, m_properties.radius, false, true, AICE_DYNAMIC) ||
      GetAISystem()->GetVolumeNavRegion()->PathSegmentWorldIntersection(link.vEdgeCenter, node1.GetPos(), m_properties.radius, false, true, AICE_DYNAMIC);
    if (intersect)
      return -1;
#endif

		Lineseg lineseg(GetNodePos(*node0.graphNode), GetNodePos(*node1.graphNode));
    unsigned nBlockers = navigationBlockers.size();
    for (unsigned i = 0 ; i < nBlockers ; ++i)
    {
      const NavigationBlocker& navBlocker = navigationBlockers[i];
      bool useBlocker = false;
      if (navBlocker.restrictedLocation)
      {
        if (navBlocker.navType != IAISystem::NAV_VOLUME)
          continue;
      }
      else
      {
        useBlocker = true;
      }
      if (useBlocker)
      {
        float t = 0.0f;
        float d = Distance::Point_LinesegSq(navBlocker.sphere.center, lineseg, t);
        if (d < square(navBlocker.sphere.radius))
        {
          float scale = navBlocker.radialDecay ? square(square(1.0f - cry_sqrtf(d) / navBlocker.sphere.radius)) : 1.0f;
          if (navBlocker.directional)
          {
            /// reduce costs that go away from point
            Vec3 segDir = (lineseg.end - lineseg.start).GetNormalizedSafe();
            Vec3 dirToBlocker = (navBlocker.sphere.center - 0.5f * (lineseg.start + lineseg.end)).GetNormalizedSafe();
            float dirScale = 1.0f - abs(segDir.Dot(dirToBlocker));
            scale *= dirScale;
          }
          blockerCost += scale * navBlocker.costAddMod * (1.0f +  m_properties.agentproperties.volumeResistanceFactor);
          blockerMult += scale * navBlocker.costMultMod;
        }
      }
    }
  }
  else if (nodes[0]->navType == IAISystem::NAV_TRIANGULAR || nodes[1]->navType == IAISystem::NAV_TRIANGULAR)
  {
		Lineseg lineseg(GetNodePos(*node0.graphNode), GetNodePos(*node1.graphNode));
    unsigned nBlockers = navigationBlockers.size();
    for (unsigned i = 0 ; i < nBlockers ; ++i)
    {
      const NavigationBlocker& navBlocker = navigationBlockers[i];
      if (!navBlocker.restrictedLocation)
      {
        float t = 0.0f;
        float d = Distance::Point_LinesegSq(navBlocker.sphere.center, lineseg, t);
        if (d < square(navBlocker.sphere.radius))
        {
          if (navBlocker.costMultMod < 0.0f)
            return -1.0f;
          float scale = navBlocker.radialDecay ? square(square(1.0f - cry_sqrtf(d) / navBlocker.sphere.radius)) : 1.0f;
          if (navBlocker.directional)
          {
            /// reduce costs that go away from point
            Vec3 segDir = (lineseg.end - lineseg.start).GetNormalizedSafe();
            Vec3 dirToBlocker = (navBlocker.sphere.center - 0.5f * (lineseg.start + lineseg.end)).GetNormalizedSafe();
            float dirScale = 1.0f - abs(segDir.Dot(dirToBlocker));
            scale *= dirScale;
          }
          blockerCost += scale * navBlocker.costAddMod * (1.0f +  m_properties.agentproperties.triangularResistanceFactor);
          blockerMult += scale * navBlocker.costMultMod;
        }
      }
    }
  }

  // increase slightly the cost of paths that deviate from the direct line - really just to bias the result where
  // there are multiple routes that are equivalent before beautification
  Lineseg line(m_startPos, m_endPos);
  float junk;
	float offsetSq = Distance::Point_LinesegSq(GetNodePos(*node0.graphNode), line, junk);
  static float maxTweak = 0.1f;
  static float maxTweakDistSq = square(50.0f);
  Limit(offsetSq, 0.0f, maxTweakDistSq);
  float offsetFactor = maxTweak * offsetSq / maxTweakDistSq;

	// TODO/workaround! Ideally the following call would call GetNodePos(node0) instead of using the nodepos directly.
	// The change here is to solve some specific problems in one of the Crysis levels where extra link cost modifiers
	// were used too much. If the start location is inside an extra link cost modifier no path could be found.
	// This is especially problem with the volume navigation.
	// The volume nav GetEnclosing() is changed so that it tries to find the node outside nav cost modifier.
  float extraLinkFactor = 0;
	if (nodes[0]->navType == IAISystem::NAV_VOLUME || nodes[1]->navType == IAISystem::NAV_VOLUME)
		extraLinkFactor = gAIEnv.pNavigation->GetExtraLinkCost(node0.graphNode->GetPos(), node1.graphNode->GetPos());
	else
		extraLinkFactor = gAIEnv.pNavigation->GetExtraLinkCost(GetNodePos(*node0.graphNode), GetNodePos(*node1.graphNode));
	// End workaround.
  if (extraLinkFactor < 0.0f)
    return -1.0f;

  float exposureFactor = m_properties.agentproperties.exposureFactor * graph->GetLinkManager().GetExposure(link);
  delta.z *= m_properties.agentproperties.zScale;
  float length = delta.GetLength();
  float totalCost = blockerCost + (1.0f + resFactor + exposureFactor + extraLinkFactor + blockerMult + offsetFactor) * length;

  return totalCost;
}







#include "LayeredNavMesh/LayeredNavMeshRegion.h"

CLayeredNavMeshHeuristic::CLayeredNavMeshHeuristic (CGraph * graph, CLayeredNavMeshRegion * region, const CStandardHeuristic * stdHeur) :
		m_lnmRegion (region), m_graph(graph), m_stdHeuristic (stdHeur)
{
	assert (m_stdHeuristic->m_pStartNode->navType == IAISystem::NAV_LAYERED_NAV_MESH);
	m_entryPts[m_stdHeuristic->m_pStartNode].push_back (EntryPt (0, m_stdHeuristic->m_startPos));
}

Vec3 CLayeredNavMeshHeuristic::GetEntryPoint (const AStarSearchNode & node0) const
{
	// NOTE Dec 16, 2008: <pvl> find an entry point to use for the current node.
	//
	// If the current node isn't in 'm_entryPts' it means that it is the starting
	// node.  For now, just use its position even though it'd probably be better to
	// the current agent position directly (which is not available to this function ATM).
	//
	// If there's exactly one entry point for the current polygon in 'm_entryPts'
	// just use that.
	//
	// If there's more than one then current polygon was reached from multiple
	// parents.  Check to see which parent is considered best by A* and use
	// entry point corresponding to it, eliminating the rest.
	//
	// UPDATE Dec 7, 2009: <pvl> actual start and end positions are available and used now

	Vec3 srcPt;
	EntryPts::iterator entryIt = m_entryPts.find (node0.graphNode);
	if (entryIt == m_entryPts.end ())
	{
		// NOTE Dec 9, 2009: <pvl> startPos gets inserted as startNode's entry point
		// this class' constructor so startNode always has to have an entry point
		assert (node0.graphNode != m_stdHeuristic->m_pStartNode);

		// NOTE Dec 9, 2009: <pvl> can happen with nodes whose predecessor is smart object?
		// At any rate, see who A* sees as our parent and use that.
		assert (node0.prevPathNodeIndex);
		srcPt = m_graph->GetNodeManager().GetNode (node0.prevPathNodeIndex->nodeIndex)->GetPos ();
		AddEntryPoint (*node0.prevPathNodeIndex, node0, srcPt);
	}
	else
	{
		PolygonEntryPts & entryPts = entryIt->second;
		if (entryPts.size () == 1)
		{
			// NOTE Dec 9, 2009: <pvl> make sure our idea of who our parent is matches A*'s
			if (node0.prevPathNodeIndex)
				assert (entryPts[0].m_parent == node0.prevPathNodeIndex->graphNode);

			srcPt = entryPts[0].m_pt;
		}
		else
		{
			PolygonEntryPts::iterator entryPtIt = entryPts.begin ();
			const PolygonEntryPts::iterator entryPtEnd = entryPts.end ();
			for ( ; entryPtIt != entryPtEnd; ++entryPtIt)
			{
				// NOTE Dec 10, 2009: <pvl> 'prevPathNodeIndex' will be 0 for start node
				const GraphNode * prevPathNode = node0.prevPathNodeIndex ? node0.prevPathNodeIndex->graphNode : 0;
				if (entryPtIt->m_parent == prevPathNode)
					break;
			}
			if (entryPtIt == entryPtEnd)
			{
				const GraphNode * prevGraphNode = m_graph->GetNodeManager().GetNode (node0.prevPathNodeIndex->nodeIndex);
				assert (prevGraphNode->navType == IAISystem::NAV_SMARTOBJECT);
				srcPt = prevGraphNode->GetPos();
				entryPts.resize (0);
				entryPts.push_back (EntryPt (node0.prevPathNodeIndex->graphNode, srcPt));
			}
			else
			{
				EntryPt e = *entryPtIt;
				srcPt = e.m_pt;
			}
		}
	}
	return srcPt;
}

void CLayeredNavMeshHeuristic::AddEntryPoint (const AStarSearchNode & srcNode, const AStarSearchNode & dstNode, const Vec3 & entryPt) const
{
	EntryPts::iterator entryIt = m_entryPts.find (dstNode.graphNode);
	if (entryIt != m_entryPts.end())
	{	
		PolygonEntryPts & entryPts = entryIt->second;

		PolygonEntryPts::const_iterator entryPtIt = entryPts.begin ();
		const PolygonEntryPts::const_iterator entryPtEnd = entryPts.end ();
		for ( ; entryPtIt != entryPtEnd; ++entryPtIt)
		{
			if (entryPtIt->m_parent == srcNode.graphNode)
				return;
		}
	}

	m_entryPts[dstNode.graphNode].push_back (EntryPt (srcNode.graphNode, entryPt));
}

float CLayeredNavMeshHeuristic::EstimateCost(const AStarSearchNode & node0, const AStarSearchNode & node1) const
{
	if (node0.graphNode->navType == IAISystem::NAV_UNSET)
		return -1.0f;

	assert (node0.graphNode->navType == IAISystem::NAV_LAYERED_NAV_MESH);

	Vec3 srcPt (GetEntryPoint (node0));

	return (srcPt - m_stdHeuristic->m_endPos).len ();
}

float CLayeredNavMeshHeuristic::CalculateCost(const CGraph * graph,
											  const AStarSearchNode & node0, unsigned linkIndex0,
											  const AStarSearchNode & node1,
														const NavigationBlockers& navigationBlockers) const
{
	if (node0.graphNode->navType == IAISystem::NAV_UNSET || node1.graphNode->navType == IAISystem::NAV_UNSET)
		return -1.0f;

	assert (node0.graphNode->navType == IAISystem::NAV_LAYERED_NAV_MESH);
	assert (node1.graphNode->navType == IAISystem::NAV_LAYERED_NAV_MESH);

	const unsigned polyIndex0 = node0.graphNode->GetLayeredMeshNavData()->polygonIndex;
	const unsigned polyIndex1 = node1.graphNode->GetLayeredMeshNavData()->polygonIndex;

	Vec3 srcPt (GetEntryPoint (node0));

	// NOTE Dec 16, 2008: <pvl> compute the entry point into the next polygon
	Lineseg sharedEdge = m_lnmRegion->GetSharedEdge (polyIndex0, polyIndex1);

	float t;
	Distance::Point_Lineseg (srcPt, sharedEdge, t);
	const Vec3 entryPt (sharedEdge.GetPoint (t));
	AddEntryPoint (node0, node1, entryPt);

	// NOTE Dec 16, 2009: <pvl> if the points only differ by less than
	// 1 millimetre (essentially arbitrary) ignore the difference - it's
	// extremely likely to be the same point really, computed from
	// different data (I know that's bad by itself but it's also hard
	// to avoid at this point).
	const float dist = (srcPt-entryPt).len ();
	return  dist > 0.001f ? dist : 0.0f;
}
