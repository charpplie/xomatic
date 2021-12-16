
#include "StdAfx.h"

#include "Pathfinder.h"
#include "NavRegion.h"
#include "WaypointHumanNavRegion.h"
#include "PipeUser.h"
#include "ISystem.h"
#include "CalculationStopper.h"

static const char *GetRequesterName(PathfindRequest *pRequest)
{
	if (pRequest->type == PathfindRequest::TYPE_ACTOR)
			return pRequest->pRequester->GetPathAgentName();
	static char label[16];
	_snprintf(label, 16, "#%d", pRequest->id);
	return label;
}

static CStandardHeuristic sStandardHeuristic;

CPathfinder::CPathfinder (CAISystem * pAISystem) : m_pAStarSolver(0), m_pCurrentRequest(0),
		m_pathFindIdGen(1), m_fLastPathfindTimeStart(0.f)
{
	//m_pAStarSolver = new CAStarSolver(gAIEnv.pGraph->GetNodeManager());
	m_pAStarSolver = new CAStarSolver(gAIEnv.pGraph->GetNodeManager());
}

CPathfinder::~CPathfinder ()
{
	delete m_pAStarSolver;
	m_pAStarSolver = 0;
}

INavPath *CPathfinder::CreateEmptyPath() const
{
	INavPath *pNewNavPath=new CNavPath();
	return pNewNavPath;
}


IPathFollower *CPathfinder::CreatePathFollower(const PathFollowerParams &params) const
{
	IPathFollower *pNewPathFollower=new CPathFollower(params);
	return pNewPathFollower;
}


void CPathfinder::PopulateObjectTracker(CObjectTracker& objectTracker)
{
	objectTracker.AddObject(m_pAStarSolver, false);
	objectTracker.AddObject(&sStandardHeuristic, false);
	objectTracker.AddObject(&m_navPath, false);
}

bool CPathfinder::CheckForAndHandleShortPath(const PathfindRequest &request)
{
	AIAssert(request.endIndex && request.startIndex);
	if (request.startIndex == request.endIndex)
	{
		// backup the original since it might be being used (e.g. between beautifying and reporting found)
		CNavPath origNavPath = m_navPath;

		CAISystem * pAISystem = GetAISystem();
		CGraph * pGraph = gAIEnv.pGraph;
		GraphNode* pStart = pGraph->GetNodeManager().GetNode(request.startIndex);
		CNavRegion *pRegion = gAIEnv.pNavigation->GetNavRegion(pStart->navType, pGraph);
		std::vector<PathPointDescriptor> points;
		if (pRegion->GetSingleNodePath(pStart, request.startPos, request.endPos, request.pRequester->GetPathAgentPassRadius(), m_navigationBlockers, points, request.pRequester->GetPathAgentMovementAbility().pathfindingProperties.navCapMask) &&
			!points.empty())
		{
			if (gAIEnv.CVars.DebugPathFinding)
				AILogAlways("CAISystem::CheckForAndHandleShortPath %s Using single node path", request.pRequester->GetPathAgentName());

			m_navPath.Clear("CAISystem::CheckForAndHandleShortPath");
			int nPoints = (int) points.size();
			for (int i = 0 ; i < nPoints - 1 ; ++i)
				m_navPath.PushBack(points[i]);
			if (m_navPath.GetPath().size() < 2)
				m_navPath.PushBack(points[nPoints-1], true);
			else
				m_navPath.PushBack(points[nPoints-1], false);

			SAIEVENT event;
			event.bPathFound = true;
			event.vPosition = m_navPath.GetLastPathPos(ZERO);
			m_navPath.SetEndDir( request.endDir );
			m_navPath.SetParams(SNavPathParams(request.startPos, request.endPos, request.startDir, request.endDir,
				request.nForceTargetBuildingId, request.allowDangerousDestination, request.endDistance, false, request.isDirectional));
			request.pRequester->PathEvent(&event);
			m_navPath = origNavPath;
			return true;

		}
		else
		{
			if (gAIEnv.CVars.DebugPathFinding)
				AILogAlways("CAISystem::CheckForAndHandleShortPath %s Single node path but it is unpassable", request.pRequester->GetPathAgentName());
			SAIEVENT event;
			event.bPathFound = false;
			request.pRequester->PathEvent(&event);
			return true;
		}
	}
	return false;
}


//====================================================================
// TidyUpEndOfPath
// Shortens the "end" of the path as much as possible by checking to see
// if it's possible to reach (checking passability) the path end "early". If
// so subsequent path points are removed - though the pathEnd is added 
// to the end.
//====================================================================
static void TidyUpEndOfPath(TPathPoints& path, float radius, const Vec3 pathEnd, 
														float criticalDistForRemoval,
														IAISystem::tNavCapMask allowedTypeMask, const NavigationBlockers& navBlockers,
														IAIPathAgent *pPathAgent)
{
	float criticalDistZ = criticalDistForRemoval;
	if (path.size() <= 1)
		return;
	const PathPointDescriptor end = path.back();
	if (!(end.navType & allowedTypeMask))
		return;

	// walk back from end to find find the start of the last sequence of potential cutting points
	TPathPoints::iterator it = path.end();
	for (--it; it != path.begin(); --it)
	{
		if (!(it->navType & allowedTypeMask))
		{
			++it;
			break;
		}
	}

	for ( ; it != path.end() ; ++it)
	{
		TPathPoints::iterator itNext = it; ++itNext;
		if (itNext == path.end())
			return;

		const Vec3& pt = it->vPos;
		const Vec3& ptNext = itNext->vPos;
		float fT = 0.0f;
		float dist = Distance::Point_Lineseg2D(pathEnd, Lineseg(pt, ptNext), fT);
		if (dist < criticalDistForRemoval)
		{
			bool ok = true;
			if (it->navType == IAISystem::NAV_TRIANGULAR)
			{
				Lineseg seg(pathEnd, it->vPos);
				Vec3 closest;
				if (gAIEnv.pNavigation->IntersectsForbidden(pathEnd, pt, closest))
					ok = false;
			}
			if (ok)
			{
				bool threeD = (it->navType & (IAISystem::NAV_FLIGHT | IAISystem::NAV_WAYPOINT_3DSURFACE | IAISystem::NAV_VOLUME)) != 0;
				Vec3 segPos = pt + fT * (ptNext - pt);
				if ( threeD || fabsf(segPos.z - pathEnd.z) < criticalDistZ)
				{
					// TODO Danny commented this test out - it causes problems for vehicles where when the frame rate is low
					// the distance can be very large, so the walkability test fails even though it's OK. However, we need to check
					// walkability to avoid dynamic obstacles.
					//        if (!GetAISystem()->GetNavRegion(it->navType, gAIEnv.pGraph)->CheckPassability(pt, end.vPos, radius, navBlockers))
					//          return;
					if (it->navType == IAISystem::NAV_TRIANGULAR && pPathAgent->GetPathAgentType() == AIOBJECT_PUPPET)
					{
						if (!gAIEnv.pNavigation->GetNavRegion(it->navType, gAIEnv.pGraph)->CheckPassability(pt, end.vPos, radius, navBlockers, pPathAgent->GetPathAgentMovementAbility().pathfindingProperties.navCapMask))
							return;
					}

					// easier to erase right up to the end and then add the end back again
					path.erase(itNext, path.end());
					path.push_back(end);
					path.back().vPos = pathEnd;
					return;
				}
			}
		}
	}
}

static bool ValidateSONavData( const CNavPath& navPath )
{
	const TPathPoints& path = navPath.GetPath();
	if ( !path.size() )
		return true;

	if ( path.front().navType == IAISystem::NAV_SMARTOBJECT )
	{
		AIAssert( !"The nav. type of the first path point is NAV_SMARTOBJECT!" );
		return false;
	}

	int counter = 0;
	TPathPoints::const_iterator it, itEnd = path.end();
	for ( it = path.begin(); it != itEnd; ++it )
	{
		const PathPointDescriptor& point = *it;
		if ( point.navType == IAISystem::NAV_SMARTOBJECT )
		{
			switch ( ++counter )
			{
			case 1:
				if ( !point.pSONavData )
				{
					if ( &*it != &path.back() )
					{
						AIAssert( !"Path point of nav. type NAV_SMARTOBJECT in middle of path has no pSONavData!" );
						return false;
					}
				}
				break;
			case 2:
				if ( point.pSONavData )
				{
					AIAssert( !"The second path point of nav. type NAV_SMARTOBJECT has pSONavData!" );
					return false;
				}
				counter = 0;
				break;
			}
		}
		else if ( counter )
		{
			if ( counter == 1 )
			{
				AIAssert( !"Single path point of nav. type NAV_SMARTOBJECT in middle of path!" );
				return false;
			}
			counter = 0;
		}
	}
	return true;
}


void CPathfinder::BeautifyPath(const VectorConstNodeIndices & pathNodes)
{
	CAISystem * pAISystem = GetAISystem();
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	if (!m_pCurrentRequest->pRequester)
		return;

	Vec3  startPos = m_pCurrentRequest->pRequester->GetPathAgentPos();

	if (!m_pCurrentRequest->pRequester->GetForcedStartPos().IsZero())
		startPos = m_pCurrentRequest->pRequester->GetForcedStartPos();
	const Vec3& startDir = m_pCurrentRequest->startDir;
	const Vec3& endPos = m_pCurrentRequest->endPos;
	const Vec3& endDir = m_pCurrentRequest->endDir;

	bool bBeautify = gAIEnv.CVars.BeautifyPath != 0;

	m_navPath.Clear("CAISystem::BeautifyPath");
	if (pathNodes.empty())
		return;

	VectorConstNodeIndices::const_iterator nodeBeginIt = pathNodes.begin();
	VectorConstNodeIndices::const_iterator nodeEndIt = nodeBeginIt;

	CGraph * pGraph = gAIEnv.pGraph;

	TPathPoints totalPath;
	totalPath.push_back(PathPointDescriptor(pGraph->GetNodeManager().GetNode(pathNodes.front())->navType, startPos));

	while (nodeEndIt != pathNodes.end())
	{
		const GraphNode* nodeBegin = pGraph->GetNodeManager().GetNode(*nodeBeginIt);
		const GraphNode* nodeEnd = pGraph->GetNodeManager().GetNode(*nodeEndIt);

		VectorConstNodeIndices::const_iterator nodeNextIt = nodeEndIt;
		++nodeNextIt;

		bool beautify = false;
		if (nodeNextIt == pathNodes.end())
		{
			// last one
			beautify = true;
		}
		else
		{
			const GraphNode* nodeNext = pGraph->GetNodeManager().GetNode(*nodeNextIt);

			if (nodeNext->navType != nodeBegin->navType)
				beautify = true;
		}

		if (beautify)
		{
			TPathPoints segmentPath;
			VectorConstNodeIndices inPath(nodeBeginIt, nodeNextIt);

			Vec3 thisStartPos = nodeBegin->GetPos();
			if (!totalPath.empty())
				thisStartPos = totalPath.back().vPos;

			Vec3 thisEndPos = nodeEnd->GetPos();
			Vec3 thisStartDir(ZERO);
			Vec3 thisEndDir(ZERO);
			if (nodeBeginIt == pathNodes.begin())
			{
				thisStartPos = startPos;
				thisStartDir = startDir;
			}
			if (nodeNextIt == pathNodes.end())
			{
				thisEndPos = endPos;
				thisEndDir = endDir;
			}
			else if (pGraph->GetNodeManager().GetNode(*nodeNextIt)->navType == IAISystem::NAV_SMARTOBJECT)
			{
				thisEndPos = pGraph->GetNodeManager().GetNode(*nodeNextIt)->GetPos();
			}
			else if (pGraph->GetNodeManager().GetNode(*nodeNextIt)->navType == IAISystem::NAV_CUSTOM_NAVIGATION)
			{
				thisEndPos = pGraph->GetNodeManager().GetNode(*nodeNextIt)->GetPos();
			}
			else if (nodeBegin->navType == IAISystem::NAV_TRIANGULAR && 
				( pGraph->GetNodeManager().GetNode(*nodeNextIt)->navType & (IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE | IAISystem::NAV_ROAD) ) )
			{
				thisEndPos = pGraph->GetNodeManager().GetNode(*nodeNextIt)->GetPos();
			}

			CNavRegion* region = gAIEnv.pNavigation->GetNavRegion(nodeBegin->navType, pGraph);
			if (nodeBegin->navType == IAISystem::NAV_UNSET)
				region = gAIEnv.pNavigation->GetWaypointHumanNavRegion(); 

			if (region)
			{
				if (bBeautify)
				{
					region->BeautifyPath(inPath, segmentPath, 
						thisStartPos, thisStartDir, thisEndPos, thisEndDir, 
						m_pCurrentRequest->pRequester->GetPathAgentPassRadius(), 
						m_pCurrentRequest->pRequester->GetPathAgentMovementAbility(),
						m_navigationBlockers);
				}
				else
				{
					region->UglifyPath(inPath, segmentPath, 
						thisStartPos, thisStartDir, thisEndPos, thisEndDir);
				}
			}

			// splice the result in
			for (TPathPoints::iterator it = segmentPath.begin() ; it != segmentPath.end() ; ++it)
				totalPath.push_back(*it);

			nodeBeginIt = nodeEndIt;
			++nodeBeginIt;
		}
		++nodeEndIt;
	}

	totalPath.push_back(PathPointDescriptor(pGraph->GetNodeManager().GetNode(pathNodes.back())->navType, endPos));
	if ( totalPath.back().navType == IAISystem::NAV_SMARTOBJECT )
		totalPath.back().navType = IAISystem::NAV_UNSET;

	// For now disable this (Danny) so that the end direction is simply stored in the path
	// and has no effect on it.
	/*
	// Adjust for start/end directions
	if (!totalPath.empty())
	{
	if (startDir.GetLengthSquared() > 0.01f)
	{
	PathPointDescriptor newFirst = totalPath.front();
	newFirst.vPos -= startDir;
	totalPath.push_front(newFirst);
	}
	if (endDir.GetLengthSquared() > 0.01f)
	{
	PathPointDescriptor newLast = totalPath.back();
	newLast.vPos += endDir;
	totalPath.push_back(newLast);
	}
	}
	*/

	static bool doEnds = true;
	if (bBeautify && doEnds)
	{
		// avoid backtracking at the start of the path by looking for the last path segment that
		// passes close to the start position. Also do this for the end by temporarily reversing 
		// the path (actually, do end first since it's easier to traverse)
		if (totalPath.size() > 1)
		{
			IAISystem::ENavigationType endType = totalPath.back().navType;
			IAISystem::tNavCapMask allowedTypeMask = IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_TRIANGULAR | IAISystem::NAV_ROAD;

			if (!m_pCurrentRequest->pRequester->GetPathAgentMovementAbility().b3DMove)
				allowedTypeMask |= IAISystem::NAV_WAYPOINT_3DSURFACE;

			float criticalDistForRemoval = m_pCurrentRequest->pRequester->GetPathAgentMovementAbility().pathRadius;

			if (endType & allowedTypeMask)
			{
				TidyUpEndOfPath(totalPath, m_pCurrentRequest->pRequester->GetPathAgentPassRadius(), 
					totalPath.back().vPos, criticalDistForRemoval, allowedTypeMask, m_navigationBlockers, m_pCurrentRequest->pRequester);
			}

			if (totalPath.size() > 1)
			{
				std::reverse(totalPath.begin(), totalPath.end());
				IAISystem::ENavigationType startType = totalPath.back().navType;
				if (startType & allowedTypeMask)
				{
					TidyUpEndOfPath(totalPath, m_pCurrentRequest->pRequester->GetPathAgentPassRadius(), 
						startPos, criticalDistForRemoval, allowedTypeMask, m_navigationBlockers, m_pCurrentRequest->pRequester);
				}
				std::reverse(totalPath.begin(), totalPath.end());
			}
		}
	}

	// copy the result, and eliminate duplicates
	PathPointDescriptor last(IAISystem::NAV_UNSET, Vec3(-9999, 9999, -99999));
	for (TPathPoints::iterator it = totalPath.begin() ; it != totalPath.end() ; ++it)
	{
		const PathPointDescriptor &ppd = *it;
		if (!ppd.IsEquivalent(last))
			m_navPath.PushBack(ppd);
		last = *it;
	}

	if (gAIEnv.configuration.IsDevMode)
		ValidateSONavData( m_navPath );
}

//====================================================================
// RequestPathTo
//====================================================================
void CPathfinder::RequestPathTo(const Vec3 &start, const Vec3 &end, const Vec3 &endDir, IAIPathAgent *pRequester, 
															bool allowDangerousDestination, int forceTargetBuildingId, float endTol, float endDistance)
{
	CAISystem * pAISystem = GetAISystem();
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	if (gAIEnv.CVars.DebugPathFinding)
		AILogAlways("CAISystem::RequestPathTo %s from (%5.2f, %5.2f, %5.2f) to (%5.2f, %5.2f, %5.2f)",
		pRequester->GetPathAgentName(), start.x, start.y, start.z, end.x, end.y, end.z);

	// if this is some no pathfinding agent
	if(!pRequester->GetPathAgentMovementAbility().bUsePathfinder)
	{
		if (gAIEnv.CVars.DebugPathFinding)
			AILogAlways("CAISystem::RequestPathTo %s not using path finder", pRequester->GetPathAgentName());
		CNavPath origNavPath = m_navPath;
		SAIEVENT sai;
		sai.bPathFound = true;
		sai.vPosition = end;
		m_navPath.Clear("CAISystem::RequestPathTo");
		m_navPath.SetParams(SNavPathParams(start, end, ZERO, endDir, forceTargetBuildingId, allowDangerousDestination, endDistance));
		m_navPath.PushBack(PathPointDescriptor(IAISystem::NAV_UNSET, end));
		pRequester->PathEvent(&sai);
		m_navPath = origNavPath;
		return;
	}

	PathfindRequest pfr(PathfindRequest::TYPE_ACTOR);
	pfr.startPos = start;
	pfr.startDir = Vec3(0.0f, 0.0f, 0.0f);
	pfr.endPos = end;
	pfr.endDir = endDir;
	pfr.pRequester = pRequester;
	pfr.allowDangerousDestination = allowDangerousDestination;
	pfr.nForceTargetBuildingId = forceTargetBuildingId;
	pfr.bPathEndIsAsRequested = true;
	pfr.endTol = endTol;
	pfr.endDistance = endDistance;

	if (end.IsZero())
	{
		AIWarning("CAISystem::RequestPathTo passed zero position for %s returning no path", 
			pRequester->GetPathAgentName());
		SAIEVENT sai;
		sai.bPathFound = false;
		pRequester->PathEvent(&sai);
		return;
	}

	if (pfr.pRequester->GetPathAgentMovementAbility().pathFindPrediction > 0.0f)
	{
		// Predict the point slightly in future and use that in pathfinding to avoid traversing back when tracing the path.
		int nBuildingID;
		IVisArea *pAreaID;
		IAISystem::ENavigationType navType = gAIEnv.pNavigation->CheckNavigationType(pfr.startPos, nBuildingID, pAreaID, pRequester->GetPathAgentMovementAbility().pathfindingProperties.navCapMask & IAISystem::NAV_TRIANGULAR);
		if (navType == IAISystem::NAV_TRIANGULAR)
		{
			// todo Danny: clip against forbidden
		}

		EAICollisionEntities aice = pfr.pRequester->GetPathAgentType() == AIOBJECT_VEHICLE ? AICE_STATIC : AICE_ALL;
		float radius = min(1.0f, pfr.pRequester->GetPathAgentPassRadius());
		if (pfr.pRequester->GetPathAgentType() == AIOBJECT_VEHICLE)
			pfr.startPos.z += radius + 0.2f; // bit workaround but the vehicle position tends to be on the bottom
		Vec3	vel = pRequester->GetPathAgentVelocity();
		Vec3	newStartPos = pfr.startPos + vel * pfr.pRequester->GetPathAgentMovementAbility().pathFindPrediction;
		float	dist;
		bool	bSameStartPos((pfr.startPos-newStartPos).IsZero(.1f));
		if( (!bSameStartPos && !IntersectSweptSphere( 0, dist, Lineseg( pfr.startPos, newStartPos ), radius, aice )) ||
			(bSameStartPos && !OverlapSphere(pfr.startPos, radius, aice)) )
			pfr.startPos = newStartPos;
		pfr.startDir.Set(0, 0, 0);

	}

	// Now get the graph nodes for start/end
	// if a vehicle then avoid extra tests based on human walkability
	CGraph * pGraph = gAIEnv.pGraph;
	bool isVehicle = pRequester->GetPathAgentType()==AIOBJECT_VEHICLE;
	unsigned startNodeIndex = pGraph->GetEnclosing(pfr.startPos, pRequester->GetPathAgentMovementAbility().pathfindingProperties.navCapMask,
		pRequester->GetPathAgentPassRadius(), pRequester->GetPathAgentLastNavNode(), 5.0f, isVehicle ? 0 : &pfr.startPos, true, pRequester->GetPathAgentName());
	pfr.startIndex = startNodeIndex;
	if (forceTargetBuildingId >= 0)
	{
		pfr.bPathEndIsAsRequested = false;
		Vec3 newEndPos = pfr.endPos;
		unsigned endIndex = gAIEnv.pNavigation->GetWaypointHumanNavRegion()->GetEnclosing(pfr.endPos, pRequester->GetPathAgentPassRadius(), 
			pRequester->GetPathAgentLastNavNode(), 5.0f, isVehicle ? 0 : &newEndPos, endTol > 0.0f, pRequester->GetPathAgentName());
		pfr.endIndex = endIndex;
		const GraphNode* pEnd = pGraph->GetNodeManager().GetNode(pfr.endIndex);
		if (!pfr.endIndex || pEnd->navType != IAISystem::NAV_WAYPOINT_HUMAN || pEnd->GetWaypointNavData()->nBuildingID != forceTargetBuildingId)
		{
			// horrible workaround because the formation points are at eye height
			pfr.endIndex = gAIEnv.pNavigation->GetWaypointHumanNavRegion()->GetClosestNode(pfr.endPos - Vec3(0, 0, 1.3f), forceTargetBuildingId);
			if (pfr.endIndex)
				pfr.endPos = pGraph->GetNodeManager().GetNode(pfr.endIndex)->GetPos();
		}
	}
	else
	{
		unsigned endNodeIndex = pGraph->GetEnclosing(pfr.endPos, pRequester->GetPathAgentMovementAbility().pathfindingProperties.navCapMask,
			pRequester->GetPathAgentPassRadius(), pRequester->GetPathAgentLastNavNode(), 5.0f, isVehicle ? 0 : &pfr.endPos, endTol > 0.0f, pRequester->GetPathAgentName());
		pfr.endIndex = endNodeIndex;
	}

	if (pfr.startIndex && pfr.endIndex)
	{
		const GraphNode * startNode = pGraph->GetNodeManager().GetNode(pfr.startIndex);
		const GraphNode * endNode = pGraph->GetNodeManager().GetNode(pfr.endIndex);

		if (startNode->navType == IAISystem::NAV_LAYERED_NAV_MESH && endNode->navType == IAISystem::NAV_LAYERED_NAV_MESH)
		{
			if (startNode->GetLayeredMeshNavData()->navModifIndex != endNode->GetLayeredMeshNavData()->navModifIndex)
			{
				// NOTE Dec 8, 2009: <pvl> if start and end are in different nav
				// meshes there can be no path between them at this level
				// TODO Dec 3, 2009: <pvl> is this the correct way of returning
				// "path not found" right away?  Others seem to do it like this.
				SAIEVENT sai;
				sai.bPathFound = false;
				pRequester->PathEvent(&sai);
				return;
			}
			// NOTE Dec 3, 2009: <pvl> this should be ensured by the way LNM GetEnclosing() works
			assert (startNode->GetLayeredMeshNavData()->agentType == endNode->GetLayeredMeshNavData()->agentType);

			// NOTE Dec 3, 2009: <pvl> while we're here make sure start and end are set
			// properly for LNM - other parts of code might have messed with these positions
			pfr.startPos = start;
			pfr.endPos = end;
		}
	}

	// move the start out of forbidden if it's on the edge
	if (pfr.startIndex && pGraph->GetNodeManager().GetNode(pfr.startIndex)->navType == IAISystem::NAV_TRIANGULAR)
	{
		Vec3 origPos = pfr.startPos;
		float edgeTol = pRequester->GetPathAgentPassRadius();
		for (int nTries = 0 ; nTries < 2 ; ++nTries)
		{
			Vec3 newPos = gAIEnv.pNavigation->GetPointOutsideForbidden(pfr.startPos, edgeTol);
			float distSq = Distance::Point_Point2DSq(newPos, pfr.startPos);
			if (distSq <= 0.0f)
				break;
			else
				pfr.startPos = newPos;
		}
		// maybe have to re-evaluate start node
		float distSq = Distance::Point_Point(pfr.startPos, origPos);
		if (distSq > 0.1f)
		{
			unsigned startIndex = pGraph->GetEnclosing(pfr.startPos, pRequester->GetPathAgentMovementAbility().pathfindingProperties.navCapMask,
				pRequester->GetPathAgentPassRadius(), pRequester->GetPathAgentLastNavNode(), 5.0f, isVehicle ? 0 : &pfr.startPos, true, pRequester->GetPathAgentName());
			pfr.startIndex = startIndex;
		}
	}

	// move the end out of forbidden if it's on the edge
	if (pfr.endIndex && pGraph->GetNodeManager().GetNode(pfr.endIndex)->navType == IAISystem::NAV_TRIANGULAR)
	{
		Vec3 origPos = pfr.endPos;
		float edgeTol = pRequester->GetPathAgentPassRadius();
		for (int nTries = 0 ; nTries < 2 ; ++nTries)
		{
			Vec3 newPos = gAIEnv.pNavigation->GetPointOutsideForbidden(pfr.endPos, edgeTol);
			float distSq = Distance::Point_Point2DSq(newPos, pfr.endPos);
			if (distSq <= 0.0f)
				break;
			else
				pfr.endPos = newPos;
		}
		// maybe have to re-evaluate end node
		float distSq = Distance::Point_Point(pfr.endPos, origPos);
		if (distSq > 0.1f)
		{
			if (forceTargetBuildingId >= 0)
			{
				pfr.bPathEndIsAsRequested = false;
				Vec3 newEndPos = pfr.endPos;
				unsigned endIndex = gAIEnv.pNavigation->GetWaypointHumanNavRegion()->GetEnclosing(pfr.endPos, pRequester->GetPathAgentPassRadius(), 
					pRequester->GetPathAgentLastNavNode(), 5.0f, isVehicle ? 0 : &newEndPos, endTol > 0.0f, pRequester->GetPathAgentName());
				pfr.endIndex = endIndex;
				const GraphNode* pEnd = pGraph->GetNodeManager().GetNode(pfr.endIndex);
				if (!pfr.endIndex || pEnd->navType != IAISystem::NAV_WAYPOINT_HUMAN || pEnd->GetWaypointNavData()->nBuildingID != forceTargetBuildingId)
				{
					// horrible workaround because the formation points are at eye height
					pfr.endIndex = gAIEnv.pNavigation->GetWaypointHumanNavRegion()->GetClosestNode(pfr.endPos - Vec3(0, 0, 1.3f), forceTargetBuildingId);
					if (pfr.endIndex)
						pfr.endPos = pGraph->GetNodeManager().GetNode(pfr.endIndex)->GetPos();
				}
			}
			else
			{
				unsigned endNodeIndex = pGraph->GetEnclosing(pfr.endPos, pRequester->GetPathAgentMovementAbility().pathfindingProperties.navCapMask,
					pRequester->GetPathAgentPassRadius(), pRequester->GetPathAgentLastNavNode(), 5.0f, isVehicle ? 0 : &pfr.endPos, endTol > 0.0f, pRequester->GetPathAgentName());
				pfr.endIndex = endNodeIndex;
			}
		}
	}

	// never trace into a forbidden area - and in some cases move the destination outside of forbidden
	if (pfr.endIndex && pGraph->GetNodeManager().GetNode(pfr.endIndex)->navType == IAISystem::NAV_TRIANGULAR)
	{
		float edgeTol = pRequester->GetPathAgentPassRadius();
		if (!allowDangerousDestination)
			edgeTol += gAIEnv.CVars.ExtraForbiddenRadiusDuringBeautification;

		bool rejectPath = false;
		Vec3 origPos = pfr.endPos;

		if (endTol > 0.0f)
		{
			for (int nTries = 0 ; nTries < 2 ; ++nTries)
			{
				Vec3 newPos = gAIEnv.pNavigation->GetPointOutsideForbidden(pfr.endPos, edgeTol);
				float distSq = Distance::Point_PointSq(origPos, newPos);
				pfr.endPos = newPos;

				if (distSq < square(0.1f))
					break;

				if (distSq > square(endTol))
				{
					rejectPath = true;
					break;
				}
			}

			// maybe have to re-evaluate end node
			if (!rejectPath)
			{
				float distSq = Distance::Point_PointSq(pfr.endPos, origPos);
				if (distSq > 0.1f)
				{
					if (forceTargetBuildingId >= 0)
					{
						pfr.bPathEndIsAsRequested = false;
						Vec3 newEndPos = pfr.endPos;
						unsigned endIndex = gAIEnv.pNavigation->GetWaypointHumanNavRegion()->GetEnclosing(pfr.endPos, pRequester->GetPathAgentPassRadius(), 
							pRequester->GetPathAgentLastNavNode(), 5.0f, isVehicle ? 0 : &newEndPos, false, pRequester->GetPathAgentName());
						pfr.endIndex = endIndex;
						GraphNode* pEnd = pGraph->GetNodeManager().GetNode(pfr.endIndex);
						if (!pfr.endIndex || pEnd->navType != IAISystem::NAV_WAYPOINT_HUMAN || pEnd->GetWaypointNavData()->nBuildingID != forceTargetBuildingId)
						{
							// horrible workaround because the formation points are at eye height
							pfr.endIndex = gAIEnv.pNavigation->GetWaypointHumanNavRegion()->GetClosestNode(pfr.endPos - Vec3(0, 0, 1.3f), forceTargetBuildingId);
							if (pfr.endIndex)
								pfr.endPos = pGraph->GetNodeManager().GetNode(pfr.endIndex)->GetPos();
						}
					}
					else
					{
						unsigned endNodeIndex = pGraph->GetEnclosing(pfr.endPos, pRequester->GetPathAgentMovementAbility().pathfindingProperties.navCapMask,
							pRequester->GetPathAgentPassRadius(), pRequester->GetPathAgentLastNavNode(), 5.0f, isVehicle ? 0 : &pfr.endPos, false, pRequester->GetPathAgentName());
						pfr.endIndex = endNodeIndex;
					}
				}
			} // !rejectPath

			float distSq = Distance::Point_Point(origPos, pfr.endPos);
			if (distSq > square(endTol))
				rejectPath = true;
		} // endTol > 0
		else if (pGraph->GetNodeManager().GetNode(pfr.endIndex)->GetTriangularNavData()->isForbiddenDesigner || gAIEnv.pNavigation->IsPointOnForbiddenEdge(pfr.endPos, edgeTol, 0, 0, false))
		{
			rejectPath = true;
		}

		if (rejectPath)
		{
			AILogComment("CAISystem::RequestPathTo Path destination (%5.2f, %5.2f, %5.2f) is unreachable (tol was %5.2f) - returning no path",
				pfr.endPos.x, pfr.endPos.y, pfr.endPos.z, endTol);
			SAIEVENT sai;
			sai.bPathFound = false;
			pRequester->PathEvent(&sai);
			return;
		}
	}

	pRequester->SetPathAgentLastNavNode(pfr.startIndex);

	if( !pfr.startIndex || !pfr.endIndex )
	{
		if (!pfr.startIndex)
		{
			AIWarning("CAISystem::RequestPathTo pts not available for %s: %s %s (from (%5.2f %5.2f %5.2f) to (%5.2f %5.2f %5.2f) Trying straight path", 
				pRequester->GetPathAgentName(), 
				pfr.startIndex == 0 ? "start" : "", 
				pfr.endIndex == 0 ? "end" : "",
				pfr.startPos.x, pfr.startPos.y, pfr.startPos.z,
				pfr.endPos.x, pfr.endPos.y, pfr.endPos.z);
			CNavPath origNavPath = m_navPath;
			m_navPath.Clear("CAISystem::RequestPathTo no start node");
			// just try a straight line path - nothing to loose
			m_navPath.SetParams(SNavPathParams(start, end, ZERO, endDir, forceTargetBuildingId, allowDangerousDestination, endDistance));
			Vec3 startPos = pRequester->GetPathAgentPos();
			
			if (!pRequester->GetForcedStartPos().IsZero())
				startPos = pRequester->GetForcedStartPos();
			IAISystem::ENavigationType navType = pfr.endIndex ? pGraph->GetNodeManager().GetNode(pfr.endIndex)->navType : IAISystem::NAV_UNSET;
			m_navPath.PushBack(PathPointDescriptor(navType, startPos));
			m_navPath.PushBack(PathPointDescriptor(navType, end));
			m_navPath.SetPathEndIsAsRequested(false);
			SAIEVENT sai;
			sai.bPathFound = true;
			sai.vPosition = end;
			pRequester->PathEvent(&sai);
			m_navPath = origNavPath;
			return;
		}
		else
		{
			// not having an end is interesting, but shouldn't be a problem
			AILogComment("CAISystem::RequestPathTo points not available for %s: %s %s (from (%5.2f %5.2f %5.2f) to (%5.2f %5.2f %5.2f)", 
				pRequester->GetPathAgentName(), 
				pfr.startIndex == 0 ? "start" : "", 
				pfr.endIndex == 0 ? "end" : "",
				pfr.startPos.x, pfr.startPos.y, pfr.startPos.z,
				pfr.endPos.x, pfr.endPos.y, pfr.endPos.z);
			SAIEVENT sai;
			sai.bPathFound = false;
			pRequester->PathEvent(&sai);
			return;
		}
	}

	// To use the directional params then the start/end positions are adjusted so long as 
	// there is LoS between the original and adjusted positions. Then the extra points 
	// must be added at the end - this is done in CAISystem::BeautifyPath.
	// We do this here because we need the node pointer... and assume that the extra offset
	// doesn't take us significantly out of the node.
	/*	if (m_cvBeautifyPath != 0 &&
	pfr.pEnd->navType == IAISystem::NAV_VOLUME && 
	pfr.endDir.GetLengthSquared() > 0.01f)
	{
	// Danny TODO - for now just set the length to be an arbitrary amount
	pfr.endDir.SetLength(7.0f);
	if (m_pVolumeNavRegion->SweptSphereWorldIntersection(pfr.endPos, pfr.endPos - pfr.endDir, pfr.pRequester->GetPathAgentPassRadius(), true, true))
	{
	AILogComment("CAISystem::RequestPathTo Using endDir (%5.2f, %5.2f, %5.2f) from endPos (%5.2f, %5.2f, %5.2f)"
	"would generate bad path for %s", 
	pfr.endDir.x, pfr.endDir.y, pfr.endDir.z, 
	pfr.endPos.x, pfr.endPos.y, pfr.endPos.z,
	pfr.pRequester->GetPathAgentName());
	pfr.endDir.Set(0.0f, 0.0f, 0.0f);
	}
	else
	{
	// the real endPos will get added onto the path after it's been created, safe
	// in the knowledge that there's line of sight between the real and "fake" end
	// position.
	pfr.endPos -= pfr.endDir;
	}
	}
	else*/
	//  {
	//    pfr.endDir.Set(0, 0, 0);
	//  }

	if ( pfr.startIndex != pfr.endIndex && pAISystem->ExitNodeImpossible(pGraph->GetLinkManager(), pGraph->GetNodeManager().GetNode(pfr.startIndex),pRequester->GetPathAgentPassRadius()) )
	{
		CNavPath origNavPath = m_navPath;
		GraphNode* pStart = pGraph->GetNodeManager().GetNode(pfr.startIndex);
		const Vec3& pos = pStart->GetPos();
		AIWarning("CAISystem::RequestPathTo Node at position (%.3f,%.3f,%.3f) (type %d) rejected as path start due to pass radius (%5.2f) - trying straight path.",
			pos.x,pos.y,pos.z, pStart->navType, pRequester->GetPathAgentPassRadius());
		m_navPath.Clear("CAISystem::RequestPathTo no exit from start node");
		// just try a straight line path - nothing to loose
		m_navPath.SetParams(SNavPathParams(start, end, ZERO, endDir, forceTargetBuildingId, allowDangerousDestination, endDistance));
		Vec3 startPos = pRequester->GetPathAgentPos();

		// TODO(marcio): fix
		if (!pRequester->GetForcedStartPos().IsZero())
			startPos = pRequester->GetForcedStartPos();
		IAISystem::ENavigationType navType = pfr.endIndex ? pGraph->GetNodeManager().GetNode(pfr.endIndex)->navType : pStart->navType;
		m_navPath.PushBack(PathPointDescriptor(navType, startPos));
		if (pStart->navType == IAISystem::NAV_FLIGHT)
			m_navPath.PushBack(PathPointDescriptor(navType, end + Vec3(0.0f, 0.0f, (end - startPos).GetLength()))); // almost always OK if we just go up!
		else
			m_navPath.PushBack(PathPointDescriptor(navType, end));
		m_navPath.SetPathEndIsAsRequested(false);
		SAIEVENT sai;
		sai.bPathFound = true;
		sai.vPosition = end;

		pRequester->PathEvent(&sai);
		m_navPath = origNavPath;
		return;
	}

	if (pfr.startIndex != pfr.endIndex && pAISystem->EnterNodeImpossible(pGraph->GetNodeManager(), pGraph->GetLinkManager(), pGraph->GetNodeManager().GetNode(pfr.endIndex), pRequester->GetPathAgentPassRadius()))
	{
		const Vec3& pos = pGraph->GetNodeManager().GetNode(pfr.endIndex)->GetPos();
		if (pfr.endTol > 0.0f)
		{
			AILogComment("CAISystem::RequestPathTo Node at position (%.3f,%.3f,%.3f) rejected as path destination due to pass radius - calculating partial path.",pos.x,pos.y,pos.z);
		}
		else
		{
			AILogComment("CAISystem::RequestPathTo Node at position (%.3f,%.3f,%.3f) rejected as path destination due to pass radius - no path.",pos.x,pos.y,pos.z);
			SAIEVENT sai;
			sai.bPathFound = false;
			pRequester->PathEvent(&sai);
			return;
		}
	}

	// check for a very short path - fii.e. one that has start/end nodes either the same or adjacent
	if (CheckForAndHandleShortPath(pfr))
		return;

	QueuePathfindRequest(pfr);
}

//====================================================================
// RequestPathTo
//====================================================================
void CPathfinder::RequestPathTo(uint32 startIndex, uint32 endIndex, const Vec3 &endDir, IAIPathAgent *pRequester, 
																bool allowDangerousDestination, int forceTargetBuildingId, float endTol, float endDistance)
{
	CAISystem * pAISystem = GetAISystem();
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	//if (gAIEnv.CVars.DebugPathFinding)
	//	AILogAlways("CAISystem::RequestPathTo %s from (%5.2f, %5.2f, %5.2f) to (%5.2f, %5.2f, %5.2f)",
	//	pRequester->GetPathAgentName(), start.x, start.y, start.z, end.x, end.y, end.z);

	if (startIndex && endIndex)
	{
		PathfindRequest pfr(PathfindRequest::TYPE_ACTOR);
		
		pfr.startDir = Vec3(0.0f, 0.0f, 0.0f);

		pfr.pRequester = pRequester;
		pfr.allowDangerousDestination = allowDangerousDestination;
		pfr.nForceTargetBuildingId = forceTargetBuildingId;
		pfr.bPathEndIsAsRequested = true;
		pfr.endTol = endTol;
		pfr.endDistance = endDistance;
		pfr.navCapMask = pRequester->GetPathAgentMovementAbility().pathfindingProperties.navCapMask;

		CGraph * pGraph = gAIEnv.pGraph;
		const GraphNode * startNode = pGraph->GetNodeManager().GetNode(startIndex);
		const GraphNode * endNode = pGraph->GetNodeManager().GetNode(endIndex);

		if (startNode && endNode)
		{
			pfr.startPos		= startNode->GetPos();
			pfr.endPos			= endNode->GetPos();
			pfr.startIndex	= startIndex;
			pfr.endIndex		= endIndex;

			pRequester->SetPathAgentLastNavNode(pfr.startIndex);

			QueuePathfindRequest(pfr);
		}
	}
}

//====================================================================
// RequestPathInDirection
//====================================================================
void CPathfinder::RequestPathInDirection(const Vec3 &start, const Vec3 &pos, float maxDist,
																			 IAIPathAgent *pRequester, float endDistance)
{
	CAISystem * pAISystem = GetAISystem();
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	if (gAIEnv.CVars.DebugPathFinding)
		AILogAlways("CPathfinder::RequestPathInDirection %s from (%5.2f, %5.2f, %5.2f) towards pos (%5.2f, %5.2f, %5.2f) up to dist %5.2f",
		pRequester->GetPathAgentName(), start.x, start.y, start.z, pos.x, pos.y, pos.z, maxDist);

	PathfindRequest pfr(PathfindRequest::TYPE_ACTOR);
	pfr.startPos = start;
	pfr.startDir = ZERO;
	pfr.endPos = pos;
	pfr.endDir = ZERO;
	pfr.pRequester = pRequester;
	pfr.startIndex = pfr.endIndex = 0;
	pfr.allowDangerousDestination = false;
	pfr.nForceTargetBuildingId = false;
	pfr.bPathEndIsAsRequested = true;
	pfr.isDirectional = true;
	pfr.endDistance = endDistance;

	CGraph * pGraph = gAIEnv.pGraph;
	// Now get the graph nodes for start/end
	unsigned startNodeIndex = pGraph->GetEnclosing(pfr.startPos, pRequester->GetPathAgentMovementAbility().pathfindingProperties.navCapMask,
		pRequester->GetPathAgentPassRadius(), pRequester->GetPathAgentLastNavNode(), 5.0f, &pfr.startPos, true, pRequester->GetPathAgentName());
	pfr.startIndex = startNodeIndex;

	// TODO Jan 23, 2008: <pvl> consider moving ExitNodeImpossible() out of CAISystem as well
	if (pfr.startIndex && pAISystem->ExitNodeImpossible(pGraph->GetLinkManager(), pGraph->GetNodeManager().GetNode(pfr.startIndex),pRequester->GetPathAgentPassRadius()) )
	{
		Vec3 vStartPos = pGraph->GetNodeManager().GetNode(pfr.startIndex)->GetPos();
		AILogComment("CAISystem::RequestPathInDirection %s Start node (%5.2f, %5.2f, %5.2f) has no useable links", 
			pRequester->GetPathAgentName(), vStartPos.x, vStartPos.y, vStartPos.z);
		m_navPath.Clear("CAISystem::RequestPathInDirection");
		SAIEVENT sai;
		sai.bPathFound = false;
		pRequester->PathEvent(&sai);
		return;
	}

	unsigned endNodeIndex = pGraph->GetEnclosing(pfr.endPos, pRequester->GetPathAgentMovementAbility().pathfindingProperties.navCapMask,
		pRequester->GetPathAgentPassRadius(), pRequester->GetPathAgentLastNavNode(), 5.0f, &pfr.endPos, false, pRequester->GetPathAgentName());
	pfr.endIndex = endNodeIndex;
	//	pfr.endIndex = 0;

	if (!pfr.startIndex)
	{
		AIWarning("CPathfinder::RequestPathInDirection pts not available for %s: %s %s (from (%5.2f %5.2f %5.2f) to (%5.2f %5.2f %5.2f)", 
			pRequester->GetPathAgentName(), 
			pfr.startIndex == 0 ? "start" : "", 
			pfr.endIndex == 0 ? "end" : "",
			pfr.startPos.x, pfr.startPos.y, pfr.startPos.z,
			pfr.endPos.x, pfr.endPos.y, pfr.endPos.z);
		SAIEVENT sai;
		sai.bPathFound = false;
		pRequester->PathEvent(&sai);
		return;
	}

	pRequester->SetPathAgentLastNavNode(pfr.startIndex);

	if (!pfr.endIndex)
	{
		// if there's no end node then use a fake node - it has no inward connections so we'll 
		// never reach it but that doesn't matter
		pGraph->MoveNode(pGraph->m_safeFirstIndex, pfr.endPos);
		pfr.endIndex = pGraph->m_safeFirstIndex;
	}

	// Stop searching when far enough from the starting position (euclidean distance).
	pfr.extraConstraints.push_back(PathfindingExtraConstraint());
	pfr.extraConstraints.back().type = PathfindingExtraConstraint::ECT_MINDISTFROMPOINT;
	pfr.extraConstraints.back().constraint.minDistFromPoint.minDistSq = sqr(maxDist);
	pfr.extraConstraints.back().constraint.minDistFromPoint.px = pfr.startPos.x;
	pfr.extraConstraints.back().constraint.minDistFromPoint.py = pfr.startPos.y;
	pfr.extraConstraints.back().constraint.minDistFromPoint.pz = pfr.startPos.z;

	// Stop searching when far enough from the starting position (distance along the nodes).
	//	pfr.extraConstraints.push_back(CStandardHeuristic::SExtraConstraint());
	//	pfr.extraConstraints.back().type = CStandardHeuristic::SExtraConstraint::ECT_MAXCOST;
	//	pfr.extraConstraints.back().constraint.maxCost.maxCost = maxDist * (1.0f + pRequester->GetPathAgentMovementAbility().pathfindingProperties.triangularResistanceFactor);

	QueuePathfindRequest(pfr);
}


Vec3 CPathfinder::GetBestPosition(const PathfindingHeuristicProperties &heuristic, float maxCost, const Vec3 &startPos, const Vec3 &endPos, unsigned startHintIndex, IAISystem::tNavCapMask navCapMask)
{
	CStandardHeuristic h;
	h.SetProperties(heuristic);

	return GetBestPosition(h, maxCost, startPos, endPos, startHintIndex, navCapMask);
}


//====================================================================
// GetBestPosition
//====================================================================
// NOTE Jan 25, 2008: <pvl> moved from CGraph since it invokes the pathfinder
// meaning that a lower-level class called a higher-level one.  The semantics
// of this function are more pathfinding-like anyway.
Vec3 CPathfinder::GetBestPosition(const CHeuristic& heuristic, float maxCost, 
														 const Vec3& startPos, const Vec3& endPos, 
														 unsigned startHintIndex,
														 IAISystem::tNavCapMask navCapMask)
{
	CGraph * pGraph = gAIEnv.pGraph;

	unsigned startNodeIndex = pGraph->GetEnclosing(startPos, navCapMask, 0.0f, startHintIndex);
	if (!startNodeIndex)
		return startPos;
	unsigned endNodeIndex = pGraph->GetEnclosing(endPos, navCapMask, 0.0f, startNodeIndex);

	AStarSearchNode* startNode = m_pAStarSolver->GetAStarNode(startNodeIndex);
	AStarSearchNode* endNode = m_pAStarSolver->GetAStarNode(endNodeIndex);
	if (endNodeIndex && endNode->graphNode->navType == IAISystem::NAV_TRIANGULAR && startNode->graphNode->navType == IAISystem::NAV_TRIANGULAR)
	{
		// outside - if possible just try a straight path. If this fails then hopefully there's enough
		// triangulation around to make the fallback code work OK.
		TPathPoints straightPath;
		PathfindRequest pfr(PathfindRequest::TYPE_ACTOR);
		pfr.startPos = startPos;
		pfr.endPos = endPos;
		pfr.startIndex = startNodeIndex;
		pfr.endIndex = endNodeIndex;
		float cost = AttemptStraightPath(straightPath, pfr, startPos, &heuristic, 
			maxCost, NavigationBlockers(), false);
		if (cost > 0.0f && !straightPath.empty())
			return straightPath.back().vPos;
	}

	// first of the pair is the cost from the start node.
	typedef std::pair<float, AStarSearchNode*> CostNodePair;
	std::map<AStarSearchNode*, float> checkedNodes;
	std::vector<CostNodePair> pendingNodes;
	pendingNodes.push_back(std::make_pair(0.0f, startNode));

	// First we get a list of all nodes that are < maxCost from the start. Then
	// we'll walk through each node and pick the one that the heuristic says
	// is closest to the target
	while (!pendingNodes.empty())
	{
		CostNodePair currentPair = pendingNodes.back();
		pendingNodes.pop_back();

		float cost = currentPair.first;
		AStarSearchNode* currentNode = currentPair.second;

		bool needToCheckThisNode = false;
		std::map<AStarSearchNode*, float>::iterator it = checkedNodes.find(currentNode);
		if (it == checkedNodes.end() || it->second > cost)
		{
			needToCheckThisNode = true;
			checkedNodes[currentNode] = cost;
		}

		if (needToCheckThisNode)
		{
			for (unsigned link = currentNode->graphNode->firstLinkIndex; link; link = pGraph->GetLinkManager().GetNextLink(link))
			{
				unsigned nodeIndex = pGraph->GetLinkManager().GetNextNode(link);
				AStarSearchNode* pNext = m_pAStarSolver->GetAStarNode(nodeIndex);

				if (!(pNext->graphNode->navType & navCapMask))
					continue;

				float costToNext = heuristic.CalculateCost(pGraph, *currentNode, link, *pNext, NavigationBlockers());
				if (costToNext <= 0.0f)
					continue;

				// Even if the cost to next is large return it, since in most cases(?) 
				// this is just because it's a big triangle, or something... Anyway
				// the node actually containing the destination is really preferred.
				if (pNext == endNode)
					return endPos;

				float totalCostToNext = cost + costToNext;
				if (totalCostToNext >= maxCost)
					continue;

				pendingNodes.push_back(std::make_pair(totalCostToNext, pNext));
			}
		}
	}

	// now find the best
	AStarSearchNode* bestNode = 0;
	float bestCostToGoal = std::numeric_limits<float>::max();
	// FIXME Jan 25, 2008: <pvl> this function was previously in CGraph where
	// it accessed GraphNode constructor directly.  This is no longer possible
	// here so we have to go through all the heavy-weight stuff that's only
	// useful for real nodes.
	unsigned int fakeIndex = pGraph->CreateNewNode( IAISystem::NAV_UNSET, endPos, 1);
	AStarSearchNode * fakeNode = m_pAStarSolver->GetAStarNode(fakeIndex);

	for (std::map<AStarSearchNode*, float>::iterator it = checkedNodes.begin() ; it != checkedNodes.end() ; ++it)
	{
		AStarSearchNode* pNode = it->first;
		float costToGoal = heuristic.EstimateCost(*pNode, *fakeNode);
		if (costToGoal < bestCostToGoal)
		{
			bestCostToGoal = costToGoal;
			bestNode = pNode;
		}
	}
	pGraph->Disconnect(fakeIndex);
	return bestNode ? bestNode->graphNode->GetPos() : startPos;
}

//====================================================================
// PathFind
//====================================================================
void CPathfinder::QueuePathfindRequest(const PathfindRequest &request)
{
	// all previous paths for this requester should be cancelled
	if (request.type == PathfindRequest::TYPE_ACTOR)
		CancelAnyPathsFor(request.pRequester);

	PathfindRequest *pNewRequest = new PathfindRequest(request);
	pNewRequest->bSuccess = false;
	m_lstPathQueue.push_back(pNewRequest);
}

//====================================================================
// CancelAnyPathsFor
//====================================================================
void CPathfinder::CancelAnyPathsFor(IAIPathAgent* pRequester, bool actorRemoved)
{
	AILogComment("Cancelling paths for %s", pRequester->GetPathAgentName());
	if (m_pCurrentRequest)
	{
		if ((actorRemoved || m_pCurrentRequest->type == PathfindRequest::TYPE_ACTOR) &&
			m_pCurrentRequest->pRequester == pRequester)
		{
			if (m_pCurrentRequest->id)
				NotifyPathFinderListeners(m_pCurrentRequest->id, 0);
			delete m_pCurrentRequest;
			m_pCurrentRequest = 0;
			m_pAStarSolver->AbortAStar();
		}
	}

	PathQueue::iterator pi;
	for (pi=m_lstPathQueue.begin();pi!=m_lstPathQueue.end();)
	{
		PathfindRequest* req = *pi;
		if ((actorRemoved || req->type == PathfindRequest::TYPE_ACTOR) && req->pRequester == pRequester)
		{
			if (req->id)
				NotifyPathFinderListeners(req->id, 0);
			delete *pi;
			pi = m_lstPathQueue.erase(pi);
		}
		else
		{
			++pi;
		}
	}
}

void CPathfinder::CancelCurrentRequest ()
{
	if (m_pCurrentRequest)
		delete m_pCurrentRequest;
	m_pCurrentRequest = 0;
}

//====================================================================
// RegisterPathFinderListener
//====================================================================
void CPathfinder::RegisterPathFinderListener(IAIPathFinderListerner* pListener)
{
	m_pathFindListeners.insert(pListener);
}

//====================================================================
// UnregisterPathFinderListener
//====================================================================
void CPathfinder::UnregisterPathFinderListener(IAIPathFinderListerner* pListener)
{
	m_pathFindListeners.erase(pListener);
}

//====================================================================
// NotifyPathFinderListeners
//====================================================================
void CPathfinder::NotifyPathFinderListeners(int id, const std::vector<unsigned>* pathNodes)
{
	for (unsigned i = 0, ni = m_pathFindListeners.size(); i < ni; ++i)
	{
		if (m_pathFindListeners[i])
			m_pathFindListeners[i]->OnPathResult(id, pathNodes);
	}
}

//====================================================================
// RequestRawPathTo
//====================================================================
int CPathfinder::RequestRawPathTo(const Vec3 &start, const Vec3 &end,
																float passRadius, IAISystem::tNavCapMask navCapMask, unsigned& lastNavNode,
																bool allowDangerousDestination, float endTol, const PathfindingExtraConstraints &constraints,
																IAIPathAgent *pReference)
{
	CAISystem * pAISystem = GetAISystem();
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	if (gAIEnv.CVars.DebugPathFinding)
		AILogAlways("CAISystem::RequestRawPathTo from (%5.2f, %5.2f, %5.2f) to (%5.2f, %5.2f, %5.2f)",
		start.x, start.y, start.z, end.x, end.y, end.z);

	PathfindRequest pfr(PathfindRequest::TYPE_RAW);
	pfr.startPos = start;
	pfr.startDir.zero();
	pfr.endPos = end;
	pfr.endDir.zero();
	pfr.pRequester = pReference;	// used for checking smart objects
	pfr.allowDangerousDestination = allowDangerousDestination;
	pfr.nForceTargetBuildingId = -1;
	pfr.bPathEndIsAsRequested = true;
	pfr.endTol = endTol;
	pfr.endDistance = 0.0f;
	pfr.id = m_pathFindIdGen++;
	pfr.navCapMask = navCapMask;
	pfr.passRadius = passRadius;
	pfr.extraConstraints = constraints;

	if (end.IsZero())
	{
		AIWarning("CAISystem::RequestRawPathTo passed zero position returning no path");
		return 0;
	}

	CGraph * pGraph = gAIEnv.pGraph;

	// Now get the graph nodes for start/end
	// if a vehicle then avoid extra tests based on human walkability
	unsigned startNodeIndex = pGraph->GetEnclosing(pfr.startPos, pfr.navCapMask, passRadius, lastNavNode, 5.0f, &pfr.startPos, true, "Raw Path");
	pfr.startIndex = startNodeIndex;

	unsigned endNodeIndex = pGraph->GetEnclosing(pfr.endPos, pfr.navCapMask, passRadius, lastNavNode, 5.0f, &pfr.endPos, endTol > 0.0f, "Raw Path");
	pfr.endIndex = endNodeIndex;

	// move the start out of forbidden if it's on the edge
	if (pfr.startIndex && pGraph->GetNodeManager().GetNode(pfr.startIndex)->navType == IAISystem::NAV_TRIANGULAR)
	{
		Vec3 origPos = pfr.startPos;
		float edgeTol = passRadius;
		for (int nTries = 0 ; nTries < 2 ; ++nTries)
		{
			Vec3 newPos = gAIEnv.pNavigation->GetPointOutsideForbidden(pfr.startPos, edgeTol);
			float distSq = Distance::Point_Point2DSq(newPos, pfr.startPos);
			if (distSq <= 0.0f)
				break;
			else
				pfr.startPos = newPos;
		}
		// maybe have to re-evaluate start node
		float distSq = Distance::Point_Point(pfr.startPos, origPos);
		if (distSq > 0.1f)
		{
			unsigned startIndex = pGraph->GetEnclosing(pfr.startPos, navCapMask, passRadius, lastNavNode, 5.0f, &pfr.startPos, true, "Raw Path");
			pfr.startIndex = startIndex;
		}
	}


	// never trace into a forbidden area - and in some cases move the destination outside of forbidden
	if (pfr.endIndex && pGraph->GetNodeManager().GetNode(pfr.endIndex)->navType == IAISystem::NAV_TRIANGULAR)
	{
		float edgeTol = passRadius;
		if (!allowDangerousDestination)
			edgeTol += gAIEnv.CVars.ExtraForbiddenRadiusDuringBeautification;

		bool rejectPath = false;
		Vec3 origPos = pfr.endPos;

		if (endTol > 0.0f)
		{
			for (int nTries = 0 ; nTries < 2 ; ++nTries)
			{
				Vec3 newPos = gAIEnv.pNavigation->GetPointOutsideForbidden(pfr.endPos, edgeTol);
				float distSq = Distance::Point_PointSq(origPos, newPos);
				pfr.endPos = newPos;

				if (distSq < square(0.1f))
					break;

				if (distSq > square(endTol))
				{
					rejectPath = true;
					break;
				}
			}

			// maybe have to re-evaluate end node
			if (!rejectPath)
			{
				float distSq = Distance::Point_PointSq(pfr.endPos, origPos);
				if (distSq > 0.1f)
				{
					unsigned endIndex = pGraph->GetEnclosing(pfr.endPos, navCapMask, passRadius, lastNavNode, 5.0f, &pfr.endPos, false, "Raw Path");
					pfr.endIndex = endIndex;
				}
			} // !rejectPath

			float distSq = Distance::Point_Point(origPos, pfr.endPos);
			if (distSq > square(endTol))
				rejectPath = true;
		} // endTol > 0
		else if (pGraph->GetNodeManager().GetNode(pfr.endIndex)->GetTriangularNavData()->isForbiddenDesigner || gAIEnv.pNavigation->IsPointOnForbiddenEdge(pfr.endPos, edgeTol, 0, 0, false))
		{
			rejectPath = true;
		}

		if (rejectPath)
		{
			AILogComment("CAISystem::RequestRawPathTo Path destination (%5.2f, %5.2f, %5.2f) is unreachable (tol was %5.2f) - returning no path",
				pfr.endPos.x, pfr.endPos.y, pfr.endPos.z, endTol);
			return -1;
		}
	}

	lastNavNode = pfr.startIndex;

	if (!pfr.startIndex || !pfr.endIndex)
		return -1;

	if (pfr.startIndex != pfr.endIndex && pAISystem->ExitNodeImpossible(pGraph->GetLinkManager(), pGraph->GetNodeManager().GetNode(pfr.startIndex), passRadius))
	{
		CNavPath origNavPath = m_navPath;
		GraphNode* pStart = pGraph->GetNodeManager().GetNode(pfr.startIndex);
		const Vec3& pos = pStart->GetPos();
		AIWarning("CAISystem::RequestRawPathTo Node at position (%.3f,%.3f,%.3f) (type %d) rejected as path start due to pass radius (%5.2f).",
			pos.x, pos.y, pos.z, pStart->navType, passRadius);
		return -1;
	}

	if (pfr.startIndex != pfr.endIndex && pAISystem->EnterNodeImpossible(pGraph->GetNodeManager(), pGraph->GetLinkManager(), pGraph->GetNodeManager().GetNode(pfr.endIndex), passRadius))
	{
		const Vec3& pos = pGraph->GetNodeManager().GetNode(pfr.endIndex)->GetPos();
		if (pfr.endTol > 0.0f)
		{
			AILogComment("CAISystem::RequestRawPathTo Node at position (%.3f,%.3f,%.3f) rejected as path destination due to pass radius - calculating partial path.",pos.x,pos.y,pos.z);
		}
		else
		{
			AILogComment("CAISystem::RequestRawPathTo Node at position (%.3f,%.3f,%.3f) rejected as path destination due to pass radius - no path.",pos.x,pos.y,pos.z);
			return -1;
		}
	}

	QueuePathfindRequest(pfr);

	return pfr.id;
}

//====================================================================
// UpdatePathFinder
//====================================================================
void CPathfinder::UpdatePathFinder()
{
	CAISystem * pAISystem = GetAISystem();
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	CTimeValue frameStartTime = pAISystem->GetFrameStartTime();

	if (!m_pCurrentRequest)
		m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;

	if (m_pCurrentRequest)
	{
		if (m_timeConsumption > gAIEnv.CVars.AllowedTimeForPathfinding)
		{
			AIWarning( "Pathfinder time out %f s (max: %f s) by %s.", m_timeConsumption,
				gAIEnv.CVars.AllowedTimeForPathfinding, GetRequesterName(m_pCurrentRequest) );
			m_nPathfinderResult = PATHFINDER_ABORT;
		}
	}

	CGraph * pGraph = gAIEnv.pGraph;

	// Beware that outputting the timeout warning above can use up all
	// our time if we start the stopper at the beginning of this function!
	CCalculationStopper stopper("Pathfinder", gAIEnv.CVars.PathfinderUpdateTime, 70000);

	while (!stopper.ShouldCalculationStop())
	{
		switch (m_nPathfinderResult)
		{
		case PATHFINDER_STILLFINDING:
			switch (m_pAStarSolver->ContinueAStar(stopper))
			{
			case ASTAR_NOPATH: 
				if (gAIEnv.CVars.DebugPathFinding)
					AILogAlways("A* ContinueAStar returned no path for %s", GetRequesterName(m_pCurrentRequest));
				m_nPathfinderResult = PATHFINDER_NOPATH; 
				break;
			case ASTAR_STILLSOLVING: 
				m_nPathfinderResult = PATHFINDER_STILLFINDING; 
				break;
			case ASTAR_PATHFOUND: 
				if (gAIEnv.CVars.DebugPathFinding)
				{
					AILogAlways("A* ContinueAStar returned path for %s", GetRequesterName(m_pCurrentRequest));
				}
				if (m_pCurrentRequest->type == PathfindRequest::TYPE_RAW)
				{
					// Raw path, return result.
					m_nPathfinderResult = PATHFINDER_PATHFOUND;
				}
				else
				{
					// Agent path, beautify.
					m_nPathfinderResult = PATHFINDER_BEAUTIFYINGPATH; 
				}
				break;
			}
			break;

		case PATHFINDER_BEAUTIFYINGPATH:
			{
				if (m_pCurrentRequest->type == PathfindRequest::TYPE_RAW)
				{
					// Should not happen, just in case.
					m_nPathfinderResult = PATHFINDER_NOPATH;
					AIWarning("Cannot beautify path for %s", GetRequesterName(m_pCurrentRequest));
					break;
				}
				if (gAIEnv.CVars.DebugPathFinding)
					AILogAlways("Beautifying path for %s", GetRequesterName(m_pCurrentRequest));
				m_navPath.Clear("CAISystem::UpdatePathFinder - PATHFINDER_BEAUTIFYINGPATH");
				const CAStarSolver::tPathNodes & pathNodes = m_pAStarSolver->GetPathNodes();
				if (pathNodes.empty())
				{
					AIWarning("No path nodes before beautifying path for %s", GetRequesterName(m_pCurrentRequest));
					m_nPathfinderResult = PATHFINDER_NOPATH;
				}
				else if (gAIEnv.CVars.BeautifyPath != 0)
				{
					// See if a straight (well, almost straight because we need to handle the pass-radius)
					// path would actually be cheaper
					if (gAIEnv.CVars.AttemptStraightPath != 0)
					{
						Vec3  startPos = m_pCurrentRequest->pRequester->GetPathAgentPos();
						if (!m_pCurrentRequest->pRequester->GetForcedStartPos().IsZero())
							startPos = m_pCurrentRequest->pRequester->GetForcedStartPos();
						TPathPoints straightPath;
						float straightCost;
						if (m_pCurrentRequest->isDirectional)
						{
							Vec3 dir = (m_pCurrentRequest->endPos - startPos);
							float maxDist = dir.NormalizeSafe();
							sStandardHeuristic.ClearConstraints();
							straightCost = AttemptPathInDir(straightPath, startPos, dir, *m_pCurrentRequest, 
								m_pAStarSolver->GetHeuristic(), maxDist, m_navigationBlockers);
						}
						else
						{
							straightCost = AttemptStraightPath(straightPath, *m_pCurrentRequest, startPos,
								m_pAStarSolver->GetHeuristic(), std::numeric_limits<float>::max(), m_navigationBlockers, false);
						}
						if (!straightPath.empty() && straightCost > 0.0f && straightCost < m_pAStarSolver->GetAStarNode(m_pCurrentRequest->endIndex)->fCostFromStart)
						{
							m_navPath.Clear("CAISystem::UpdatePathFinder - PATHFINDER_BEAUTIFYINGPATH straight path");
							m_navPath.SetPathEndIsAsRequested(m_pCurrentRequest->bPathEndIsAsRequested);
							// just convert the straight path
							for (TPathPoints::iterator it = straightPath.begin() ; it != straightPath.end() ; ++it)
								m_navPath.PushBack(*it);
						}
						else
						{
							// Use the A* path after beautifying it
							BeautifyPath(pathNodes);
						}
					}
					else
					{
						// Use the A* path after beautifying it
						BeautifyPath(pathNodes);
					}
					m_nPathfinderResult = m_navPath.GetPath().empty() ? PATHFINDER_NOPATH : PATHFINDER_PATHFOUND;
				}
				else
				{
					BeautifyPath(pathNodes);
					m_nPathfinderResult = m_navPath.GetPath().empty() ? PATHFINDER_NOPATH : PATHFINDER_PATHFOUND;
				}
			}
			break;

		case PATHFINDER_PATHFOUND:
			{
				if (m_pCurrentRequest->type == PathfindRequest::TYPE_RAW)
				{
					// Raw path was requested.
					NotifyPathFinderListeners(m_pCurrentRequest->id, &m_pAStarSolver->GetPathNodes());
					AILogComment("Pathfinding for #%d took %5.2f seconds", m_pCurrentRequest->id, (frameStartTime - m_fLastPathfindTimeStart).GetSeconds());
					delete m_pCurrentRequest;
					m_pCurrentRequest = 0;
					m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;
					m_navPath.Clear("CAISystem::UpdatePathFinder - RawPath");
				}
				else
				{
					// danny bug report crysis-735 says it hit this assert (when it was AIAssert). It shouldn't be possible, 
					// according to the logic here, unless m_navPath got cleared in between one update and the next. So, 
					// convert it to assert (so if you hit it please tell me), and handle it.
					assert(!m_navPath.GetPath().empty());
					SAIEVENT event;
					if (m_navPath.GetPath().empty())
					{
						event.bPathFound = false;
					}
					else
					{
						event.bPathFound = true;
						m_navPath.SetEndDir( m_pCurrentRequest->endDir );
						if (m_navPath.GetPath().size() == 1)
							m_navPath.PushFront(PathPointDescriptor(IAISystem::NAV_UNSET, m_pCurrentRequest->pRequester->GetPathAgentPos()), true);
					}
					AILogComment("Pathfinding for %s took %5.2f seconds", GetRequesterName(m_pCurrentRequest), (frameStartTime - m_fLastPathfindTimeStart).GetSeconds());
					m_pCurrentRequest->pRequester->PathEvent(&event);
					delete m_pCurrentRequest;
					m_pCurrentRequest = 0;
					m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;
				}
			}

			break;

		case PATHFINDER_NOPATH:
			{
				if (m_pCurrentRequest->type == PathfindRequest::TYPE_RAW)
				{
					// Raw path was requested.
					const CAStarSolver::tPathNodes & pathNodes = m_pAStarSolver->CalculateAndGetPartialPath();
					if (!pathNodes.empty())
					{
						AILogComment("Using partial path for #%d", m_pCurrentRequest->id);
						NotifyPathFinderListeners(m_pCurrentRequest->id, &pathNodes);
					}
					else
					{
						AILogComment("Path failed for #%d", m_pCurrentRequest->id);
						NotifyPathFinderListeners(m_pCurrentRequest->id, 0);
					}
					AILogComment("Pathfinding for #%d took %5.2f seconds", m_pCurrentRequest->id, (frameStartTime - m_fLastPathfindTimeStart).GetSeconds());
					delete m_pCurrentRequest;
					m_pCurrentRequest = 0;
					m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;
					m_navPath.Clear("CAISystem::UpdatePathFinder - RawPath");
				}
				else
				{
					AILogComment("Path failed for %s", GetRequesterName(m_pCurrentRequest));
					// if the reason for no path was that the destination was a little bit too tight - well
					// just find the closest
					if (m_pCurrentRequest->endTol > 0.0f)
					{
						AILogComment("Attempting partial path for %s", GetRequesterName(m_pCurrentRequest));
						m_navPath.Clear("CAISystem::UpdatePathFinder - PATHFINDER_NOPATH before partial");
						m_navPath.SetPathEndIsAsRequested(false);
						// could use either the partial path, or straight/directional path
						const CAStarSolver::tPathNodes & pathNodes = m_pAStarSolver->CalculateAndGetPartialPath();
						if (!pathNodes.empty())
						{
							// bear in mind that the beautifier adds the final destination - since we don't want that
							// then force it not to
							Vec3 origEndPos = m_pCurrentRequest->endPos;
							m_pCurrentRequest->endPos = pGraph->GetNodeManager().GetNode(pathNodes.back())->GetPos();
							m_pCurrentRequest->endDir.Set(0, 0, 0);
							BeautifyPath(pathNodes);
							m_pCurrentRequest->endPos = origEndPos;
						}

						if (gAIEnv.CVars.AttemptStraightPath != 0)
						{
							Vec3  startPos = m_pCurrentRequest->pRequester->GetPathAgentPos();
							if (!m_pCurrentRequest->pRequester->GetForcedStartPos().IsZero())
								startPos = m_pCurrentRequest->pRequester->GetForcedStartPos();
							TPathPoints straightPath;
							float straightCost;
							if (m_pCurrentRequest->isDirectional)
							{
								Vec3 dir = (m_pCurrentRequest->endPos - startPos);
								float maxDist = dir.NormalizeSafe();
								sStandardHeuristic.ClearConstraints();
								straightCost = AttemptPathInDir(straightPath, startPos, dir, *m_pCurrentRequest, 
									m_pAStarSolver->GetHeuristic(), maxDist, m_navigationBlockers);
							}
							else
							{
								straightCost = AttemptStraightPath(straightPath, *m_pCurrentRequest, startPos,
									m_pAStarSolver->GetHeuristic(), std::numeric_limits<float>::max(), m_navigationBlockers, true);
							}

							if (!straightPath.empty() && straightCost >= 0.0f)
							{
								float straightDist = Distance::Point_Point(straightPath.back().vPos, m_pCurrentRequest->endPos);
								float pathDist = Distance::Point_Point(m_navPath.GetLastPathPos(), m_pCurrentRequest->endPos);
								if (straightDist < pathDist)
								{
									AILogComment("CAISystem::UpdatePathFinder - PATHFINDER_NOPATH using straight path instead of partial for %s", GetRequesterName(m_pCurrentRequest));
									m_navPath.Clear("CAISystem::UpdatePathFinder - PATHFINDER_NOPATH using straight path instead of partial");
									// just convert the straight path
									for (TPathPoints::iterator it = straightPath.begin() ; it != straightPath.end() ; ++it)
										m_navPath.PushBack(*it);
									m_nPathfinderResult = m_navPath.GetPath().empty() ? PATHFINDER_NOPATH : PATHFINDER_PATHFOUND;
									break;
								}
								else
								{
									AILogComment("CAISystem::UpdatePathFinder - PATHFINDER_NOPATH using partial path instead of straight for %s", GetRequesterName(m_pCurrentRequest));
								}
							}
						}
						if (!m_navPath.GetPath().empty())
						{
							AILogComment("Using partial path for %s", GetRequesterName(m_pCurrentRequest));
							m_nPathfinderResult = PATHFINDER_PATHFOUND;
							break; // finish the switch case
						}
						SAIEVENT event;
						event.bPathFound = false;
						m_navPath.Clear("CAISystem::UpdatePathFinder - PATHFINDER_NOPATH");
						AILogComment("Pathfinding for %s took %5.2f seconds", GetRequesterName(m_pCurrentRequest), (frameStartTime - m_fLastPathfindTimeStart).GetSeconds());
						m_pCurrentRequest->pRequester->PathEvent(&event);
						delete m_pCurrentRequest;
						m_pCurrentRequest = 0;
						m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;
						break; // finish the switch case
					}
					else
					{
						SAIEVENT event;
						event.bPathFound = false;
						m_navPath.Clear("CAISystem::UpdatePathFinder - PATHFINDER_NOPATH no partial");

						AILogComment("Pathfinding for %s took %5.2f seconds", GetRequesterName(m_pCurrentRequest), (frameStartTime - m_fLastPathfindTimeStart).GetSeconds());
						m_pCurrentRequest->pRequester->PathEvent(&event);
						delete m_pCurrentRequest;
						m_pCurrentRequest = 0;
						m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;
					}
				}
			}
			break;
		case PATHFINDER_ABORT:
			{
				if (m_pCurrentRequest->type == PathfindRequest::TYPE_RAW)
				{
					// Raw path was requested.
					const CAStarSolver::tPathNodes & pathNodes = m_pAStarSolver->CalculateAndGetPartialPath();
					if (!pathNodes.empty())
					{
						AILogComment("Pathfinding aborted but using partial path for #%d", m_pCurrentRequest->id);
						NotifyPathFinderListeners(m_pCurrentRequest->id, &pathNodes);
					}
					else
					{
						AILogComment("Pathfinding aborted #%d", m_pCurrentRequest->id);
						NotifyPathFinderListeners(m_pCurrentRequest->id, 0);
					}
					delete m_pCurrentRequest;
					m_pCurrentRequest = 0;
					m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;
					m_navPath.Clear("CAISystem::UpdatePathFinder - RawPath");
				}
				else
				{
					if (gAIEnv.CVars.CrowdControlInPathfind != 0 && 
						m_pCurrentRequest->pRequester->GetPathAgentMovementAbility().pathRegenIntervalDuringTrace > 0.0f)
					{
						AILogComment("Attempting partial path for %s", GetRequesterName(m_pCurrentRequest));
						m_navPath.Clear("CAISystem::UpdatePathFinder - PATHFINDER_ABORT before partial");
						m_navPath.SetPathEndIsAsRequested(false);
						// could use either the partial path, or straight/directional path
						const CAStarSolver::tPathNodes & pathNodes = m_pAStarSolver->CalculateAndGetPartialPath();
						if (!pathNodes.empty())
						{
							// bear in mind that the beautifier adds the final destination, which is what we want (even though the
							// final path segment will be bad... just hope we regen before we get there)
							BeautifyPath(pathNodes);
						}

						if (!m_navPath.GetPath().empty())
						{
							AILogComment("Pathfinding aborted but using partial path for %s", GetRequesterName(m_pCurrentRequest));
							m_nPathfinderResult = PATHFINDER_PATHFOUND;
							m_fLastPathfindTimeStart = frameStartTime;
							break; // finish the switch case
						}
					}

					SAIEVENT event;
					event.bPathFound = false;
					m_navPath.Clear("CAISystem::UpdatePathFinder - PATHFINDER_ABORT");
					AILogComment("Pathfinding (aborted, no partial path) for %s took %5.2f seconds", GetRequesterName(m_pCurrentRequest), (frameStartTime - m_fLastPathfindTimeStart).GetSeconds());
					m_pCurrentRequest->pRequester->PathEvent(&event);
					delete m_pCurrentRequest;
					m_pCurrentRequest = 0;
					m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;
				}
			}
			break;
		case PATHFINDER_POPNEWREQUEST:
			{
				if (!m_pCurrentRequest)
				{
					// get request if any waiting in queue
					if (m_lstPathQueue.empty()) 
						return;
					m_pCurrentRequest = (*m_lstPathQueue.begin());
					m_lstPathQueue.pop_front();

					m_timeConsumption = 0;

					if (gAIEnv.CVars.DebugPathFinding)
						AILogAlways("Processing path request for %s", GetRequesterName(m_pCurrentRequest));

					// set the params here since various things can get tweaked subsequently. However, this does
					// mean that m_navPath must not get "corrupted" between now and when the path is found (e.g.
					// by a trivial path)
					m_navPath.SetParams(SNavPathParams(
						m_pCurrentRequest->startPos, m_pCurrentRequest->endPos, 
						m_pCurrentRequest->startDir, m_pCurrentRequest->endDir,
						m_pCurrentRequest->nForceTargetBuildingId, m_pCurrentRequest->allowDangerousDestination,
						m_pCurrentRequest->endDistance, false, m_pCurrentRequest->isDirectional));

					// do A*
					m_nPathfinderResult = PATHFINDER_NOPATH;
					m_fLastPathfindTimeStart = frameStartTime;

					sStandardHeuristic.SetStartEndData(
						pGraph->GetNodeManager().GetNode(m_pCurrentRequest->startIndex),
						m_pCurrentRequest->startPos,
						pGraph->GetNodeManager().GetNode(m_pCurrentRequest->endIndex),
						m_pCurrentRequest->endPos,
						m_pCurrentRequest->extraConstraints);

					if (m_pCurrentRequest->type == PathfindRequest::TYPE_ACTOR)
					{
						PathfindingHeuristicProperties props;
						props.agentproperties = m_pCurrentRequest->pRequester->GetPathAgentMovementAbility().pathfindingProperties;
						props.agentproperties.radius = m_pCurrentRequest->pRequester->GetPathAgentPassRadius();
						props.pAgent = m_pCurrentRequest->pRequester;
						sStandardHeuristic.SetProperties(props);
						GetNavigationBlockers(m_navigationBlockers, m_pCurrentRequest->pRequester, m_pCurrentRequest);
					}
					else
					{
						PathfindingHeuristicProperties props;
						props.agentproperties.SetToDefaults();
						props.agentproperties.navCapMask = m_pCurrentRequest->navCapMask;
						props.agentproperties.radius = m_pCurrentRequest->passRadius;
						props.pAgent = m_pCurrentRequest->pRequester;
						sStandardHeuristic.SetProperties(props);
						m_navigationBlockers.clear();
					}

					bool bDebugDrawOpenList = false;
					if (gAIEnv.CVars.DebugPathFinding > 0)
					{
						const char* szName = gAIEnv.CVars.DebugDrawAStarOpenList;
						if (szName && strcmp(szName, "0") != 0)
						{
							if (strcmp(GetRequesterName(m_pCurrentRequest), szName) == 0)
								bDebugDrawOpenList = true;
						}
					}

					switch(m_pAStarSolver->SetupAStar(stopper, pGraph, &sStandardHeuristic,
						m_pCurrentRequest->startIndex, m_pCurrentRequest->endIndex,
						m_navigationBlockers, bDebugDrawOpenList))
					{
					case ASTAR_NOPATH: 
						if (gAIEnv.CVars.DebugPathFinding)
							AILogAlways("A* SetupAStar returned no path for %s", GetRequesterName(m_pCurrentRequest));
						m_nPathfinderResult = PATHFINDER_NOPATH; 
						break;
					case ASTAR_STILLSOLVING: 
						m_nPathfinderResult = PATHFINDER_STILLFINDING; 
						break;
					case ASTAR_PATHFOUND: 
						if (gAIEnv.CVars.DebugPathFinding)
							AILogAlways("Got immediate ASTAR_PATHFOUND for %s", GetRequesterName(m_pCurrentRequest));
						if (m_pCurrentRequest->type == PathfindRequest::TYPE_RAW)
							m_nPathfinderResult = PATHFINDER_PATHFOUND; 
						else
							m_nPathfinderResult = PATHFINDER_BEAUTIFYINGPATH; 
						break;
					}
				}
			}
			break;
		}//end switch
	}// end while
	
	if (m_nPathfinderResult == PATHFINDER_STILLFINDING)
		m_timeConsumption += gAIEnv.CVars.PathfinderUpdateTime - 0.001f * stopper.GetMilliSecondsRemaining();
	else
		m_timeConsumption = 0;
}

//====================================================================
// GetNavigationBlockers
//====================================================================
void CPathfinder::GetNavigationBlockers(NavigationBlockers& navigationBlockers, IAIPathAgent *pRequester, const PathfindRequest *pfr)
{
	CAISystem * pAISystem = GetAISystem();

	const Vec3 requesterPos = pRequester->GetPathAgentPos();
	const Vec3 requesterVel = pRequester->GetPathAgentVelocity();

	navigationBlockers.resize(0);
	if (gAIEnv.CVars.CrowdControlInPathfind == 0)
		return;

	// Danny: this is/looks expensive and could result in a lot of blockers (also expensive in the pathfinder)
	// So - maybe it is an OK compromise that triangular is disabled from GetNavigationBlocker.
	static bool doEveryone = false;
	if (doEveryone && m_pCurrentRequest)
	{
		const CAISystem::AIObjectOwners::iterator aiEnd = GetAISystem()->m_Objects.end();
		for (CAISystem::AIObjectOwners::iterator ai = GetAISystem()->m_Objects.find(AIOBJECT_PUPPET); 
			ai != aiEnd && ai->first == AIOBJECT_PUPPET ; ++ai)
		{
			CAIActor* pAI = (CAIActor*) ai->second.GetAIObject();
			if (pAI != m_pCurrentRequest->pRequester)
				// FIXME - GetNavigationBlocker should accept CAIObjects, not CAIActors?
				GetNavigationBlocker(navigationBlockers, requesterPos, requesterVel, pAI->GetPhysicsPos(), pAI->GetVelocity(), pAI);
		}
	}

	if (pAISystem->GetPlayer())
		GetNavigationBlocker(navigationBlockers, requesterPos, requesterVel, pAISystem->GetPlayer()->GetPhysicsPos(), pAISystem->GetPlayer()->GetVelocity(), 0);

	pRequester->GetPathAgentNavigationBlockers(navigationBlockers, pfr);

#ifdef _DEBUG
	IEntity* ent = gEnv->pEntitySystem->FindEntityByName("TestNavBlocker");
	if (ent)
	{
		if (pfr)
		{
			static float radius = 50.0f;
			static float cost = 1000.0f;
			static bool radialDecay = true;
			static bool directional = true;
			static float extra = 1.5f;
			float d1 = extra * Distance::Point_Point(ent->GetPos(), pfr->startPos);
			float d2 = extra * Distance::Point_Point(ent->GetPos(), pfr->endPos);
			float r = min(min(d1, d2), radius);
			navigationBlockers.push_back(NavigationBlocker(ent->GetPos(), r, 0.0f, cost, radialDecay, directional));
		}
		else
		{
			static float radius = 70.0f;
			static float cost = 20.0f;
			static bool radialDecay = true;
			static bool directional = true;
			navigationBlockers.push_back(NavigationBlocker(ent->GetPos(), radius, 0.0f, cost, radialDecay, directional));
		}
	}
#endif
}

//====================================================================
// GetNavigationBlocker
//====================================================================
void CPathfinder::GetNavigationBlocker(NavigationBlockers& navigationBlockers, 
																		 const Vec3 &requesterPos, const Vec3 &requesterVel,
																		 Vec3 blockerPos, const Vec3& blockerVel, CAIActor* pAIActor)
{
	CAISystem * pAISystem = GetAISystem();

	int nBuildingID = -1;
	IVisArea* pArea = 0;

	// Don't consider triangular - generally there's too many, and the path beautification will 
	// make it irrelevant anyway.
	IAISystem::ENavigationType navType = gAIEnv.pNavigation->CheckNavigationType(
		blockerPos, nBuildingID, pArea, pAIActor ? pAIActor->GetMovementAbility().pathfindingProperties.navCapMask : 
		/*IAISystem::NAV_TRIANGULAR | */ IAISystem::tNavCapMask (IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_VOLUME));

	// if dist to blocker < criticalDist then the blocker position is predicted. Otherwise the blocker
	// is only considered if he's pretty well stationary (vel < criticalVel)
	static float criticalDistSq = square(20.0f);
	static float criticalSpeedSq = square(0.5f);
	static float predictionTime = 2.0f;

	float blockerSpeedSq = blockerVel.GetLengthSquared();
	float requesterSpeedSq = requesterVel.GetLengthSquared();

	/// This represents the extra distance that a path follower would be prepared to divert
	/// in order to avoid the blocker
	static float costMod2D = 10.0f;
	static float costMod3D = 20.0f;

	switch (navType)
	{
	case IAISystem::NAV_TRIANGULAR:
		{
			bool gotFloor = GetFloorPos(blockerPos, blockerPos, 0.05f, 2.0f, 0.1f, AICE_ALL);
			if (!gotFloor)
				return;

			float distSq = Distance::Point_PointSq(requesterPos, blockerPos);
			if (distSq > criticalDistSq && blockerSpeedSq > criticalSpeedSq)
				return;
			else if (distSq < criticalDistSq)
				blockerPos += predictionTime * blockerVel;

			float radius = 1.0f;

			navigationBlockers.push_back(NavigationBlocker(blockerPos, radius, costMod2D, 0.0f, false, false));
			navigationBlockers.back().navType = navType;
			navigationBlockers.back().restrictedLocation = true;
		}
		break;
	case IAISystem::NAV_WAYPOINT_HUMAN:
		{
			bool gotFloor = GetFloorPos(blockerPos, blockerPos, 0.05f, 2.0f, 0.1f, AICE_ALL);
			if (!gotFloor)
				return;

			float distSq = Distance::Point_PointSq(requesterPos, blockerPos);
			if (distSq > criticalDistSq && blockerSpeedSq > criticalSpeedSq)
				return;
			else if (distSq < criticalDistSq)
				blockerPos += predictionTime * blockerVel;

			float radius = 1.0f;

			navigationBlockers.push_back(NavigationBlocker(blockerPos, radius, costMod2D, 0.0f, false, false));
			navigationBlockers.back().navType = navType;
			navigationBlockers.back().location.waypoint.nBuildingID = nBuildingID;
			navigationBlockers.back().restrictedLocation = true;
		}
		break;
	case IAISystem::NAV_WAYPOINT_3DSURFACE:
		{
			float distSq = Distance::Point_PointSq(requesterPos, blockerPos);
			if (distSq > criticalDistSq && blockerSpeedSq > criticalSpeedSq)
				return;
			else if (distSq < criticalDistSq)
				blockerPos += predictionTime * blockerVel;

			float radius = 1.0f;

			navigationBlockers.push_back(NavigationBlocker(blockerPos, radius, costMod2D, 0.0f, false, false));
			navigationBlockers.back().navType = navType;
			navigationBlockers.back().location.waypoint.nBuildingID = nBuildingID;
			navigationBlockers.back().restrictedLocation = true;
		}
		break;
	case IAISystem::NAV_FLIGHT:
		{
		}
		break;
	case IAISystem::NAV_VOLUME:
		{
			if (!pAIActor)
			{
				float distSq = Distance::Point_PointSq(requesterPos, blockerPos);
				if (distSq > criticalDistSq && blockerSpeedSq > criticalSpeedSq)
					return;
				else if (distSq < criticalDistSq)
					blockerPos += predictionTime * blockerVel;

				float radius = 2.5f;
				navigationBlockers.push_back(NavigationBlocker(blockerPos, radius, costMod3D, 0.0f, false, false));
				navigationBlockers.back().navType = navType;
				navigationBlockers.back().restrictedLocation = true;
			}
		}
		break;
	case IAISystem::NAV_ROAD:
		{
			// don't steer off a road...
		}
		break;
	default:
		break;
	}
}

//===================================================================
// AttemptPathInDir
//===================================================================
float CPathfinder::AttemptPathInDir(TPathPoints &path, const Vec3 &curPos, const Vec3 &dir, const PathfindRequest &request, 
																	const CHeuristic* pHeuristic, float maxCost, const NavigationBlockers& navigationBlockers)
{
	CAISystem * pAISystem = GetAISystem();

	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );
	// potential path seg Dot with dir must be > criticalDot to continue;
	static float criticalDot = 0.7f;

	float inputDirLenSq = dir.GetLengthSquared();
	AIAssert(inputDirLenSq > 0.8f && inputDirLenSq < 1.2f);

	unsigned currentNodeIndex = request.startIndex;
	if (!currentNodeIndex)
		return -1.0f;

	if (!pHeuristic)
		return -1.0f;

	const PathfindingHeuristicProperties *reqProperties = pHeuristic->GetProperties();
	if (!reqProperties)
		return -1.0f;

	CGraph * pGraph = gAIEnv.pGraph;
	IAISystem::ENavigationType navType = pGraph->GetNodeManager().GetNode(request.startIndex)->navType;
	if (navType != IAISystem::NAV_TRIANGULAR)
		return -1.0f;

	float extraR = gAIEnv.CVars.ExtraRadiusDuringBeautification;
	float extraFR = gAIEnv.CVars.ExtraForbiddenRadiusDuringBeautification;

	float pathCost = 0.0f;

	float heuristicZScale = 1.0f;
	//  if (pHeuristic->GetRequesterProperties())
	//    heuristicZScale *= pHeuristic->GetRequesterProperties()->properties.zScale;
	Vec3 heuristicZScaleVec(1, 1, heuristicZScale);

	SMarkClearer markClearer(pGraph);

	path.clear();
	path.push_back(PathPointDescriptor(IAISystem::NAV_TRIANGULAR, request.startPos));
	while (pathCost < maxCost)
	{
		const AStarSearchNode* currentNode = m_pAStarSolver->GetAStarNode(currentNodeIndex);

		pGraph->MarkNode(currentNodeIndex);
		const Vec3 &currentPos = path.back().vPos;

		float bestDot = -std::numeric_limits<float>::max();
		Vec3 bestEdgePos(ZERO);
		unsigned bestNextNodeIndex = 0;
		Vec3 bestPathSegment(ZERO);
		float bestHeuristicCost = 0.0f;
		float bestHeuristicDist = 0.0f;

		for (unsigned linkId = currentNode->graphNode->firstLinkIndex ; linkId ; linkId = pGraph->GetLinkManager().GetNextLink(linkId))
		{
			unsigned nextNodeIndex = pGraph->GetLinkManager().GetNextNode(linkId);
			const AStarSearchNode *nextNode = m_pAStarSolver->GetAStarNode(nextNodeIndex);

			if (nextNode->graphNode->mark)
				continue;
			if (nextNode->graphNode->navType != IAISystem::NAV_TRIANGULAR)
				continue;
			float heuristicCost = pHeuristic->CalculateCost(pGraph, *currentNode, linkId, *nextNode, navigationBlockers);
			if (heuristicCost < 0.0f)
				continue;
			float heuristicDist = (nextNode->graphNode->GetPos() - currentNode->graphNode->GetPos()).CompMul(heuristicZScaleVec).GetLength();
			if (heuristicDist < 0.01f)
				continue;

			// Now find the point on the edge containing the link that intersects the straight path
			// in 2D.
			unsigned vertexStartIndex = pGraph->GetLinkManager().GetStartIndex(linkId);
			unsigned vertexEndIndex = pGraph->GetLinkManager().GetEndIndex(linkId);
			if (vertexStartIndex == vertexEndIndex)
				continue;
			const ObstacleData &startObst = GetAISystem()->m_VertexList.GetVertex(currentNode->graphNode->GetTriangularNavData()->vertices[vertexStartIndex]);
			const ObstacleData &endObst = GetAISystem()->m_VertexList.GetVertex(currentNode->graphNode->GetTriangularNavData()->vertices[vertexEndIndex]);
			Vec3 edgeStart = startObst.vPos;
			Vec3 edgeEnd = endObst.vPos;
			Vec3 edgeDir = (edgeEnd - edgeStart).GetNormalizedSafe(Vec3(1, 0, 0));
			Lineseg edgeSegment(edgeStart, edgeEnd);
			Lineseg segmentToEnd(currentPos, currentPos + dir * 100.0f);
			float tA, tB;
			// ignore the return value - use as a line-line test
			Intersect::Lineseg_Lineseg2D(segmentToEnd, edgeSegment, tA, tB);
			{
				Vec3 edgePt = edgeSegment.start + tB * (edgeSegment.end - edgeSegment.start);
				// constrain this edge point to be in a valid region
				float workingRadius = reqProperties->agentproperties.radius + extraR;
				if (!startObst.IsCollidable() && !endObst.IsCollidable())
				{
					workingRadius = 0.0f;
				}
				// TODO Jan 24, 2008: <pvl> look into whether IsPointOnForbiddenEdge()
				// should be moved here, too
				else if (gAIEnv.pNavigation->IsPointOnForbiddenEdge(edgeStart, 0.0001f, 0, 0, false) || 
					gAIEnv.pNavigation->IsPointOnForbiddenEdge(edgeEnd, 0.0001f, 0, 0, false))
				{
					workingRadius += extraFR;
				}
				if (workingRadius > pGraph->GetLinkManager().GetRadius(linkId))
					workingRadius = pGraph->GetLinkManager().GetRadius(linkId);
				Vec3 edgeStartOK = pGraph->GetLinkManager().GetEdgeCenter(linkId) - edgeDir * (pGraph->GetLinkManager().GetRadius(linkId) - workingRadius);
				Vec3 edgeEndOK = pGraph->GetLinkManager().GetEdgeCenter(linkId) + edgeDir * (pGraph->GetLinkManager().GetRadius(linkId) - workingRadius);

				if ( ( (edgePt - edgeStartOK) | edgeDir ) < 0.0f)
					edgePt = edgeStartOK;
				else if ( ( (edgePt - edgeEndOK) | edgeDir ) > 0.0f)
					edgePt = edgeEndOK;

				// Estimate the cost of getting there
				Vec3 pathSegment = edgePt - currentPos;
				Vec3 pathSegmentDir = pathSegment.GetNormalizedSafe();

				float dirDot = pathSegmentDir.Dot(dir);

				if (dirDot <= bestDot)
					continue;
				if (dirDot < criticalDot)
					continue;

				bestDot = dirDot;
				bestEdgePos = edgePt;
				bestNextNodeIndex = nextNodeIndex;
				bestPathSegment = pathSegment;
				bestHeuristicCost = heuristicCost;
				bestHeuristicDist = heuristicDist;
			}
		}
		if (bestDot < criticalDot)
		{
			// no exit from this triangle - just go as far as we can.
			static std::vector<Vec3> poly(3);
			const STriangularNavData *pTriNavData = currentNode->graphNode->GetTriangularNavData();
			const ObstacleIndexVector::const_iterator oiEnd = pTriNavData->vertices.end();
			unsigned iVert = 0;
			for (ObstacleIndexVector::const_iterator oi = pTriNavData->vertices.begin(); oi != oiEnd ; ++oi)
				poly[iVert++] = pAISystem->m_VertexList.GetVertex(*oi).vPos;
			Vec3 startPos = currentPos;
			float segLen = maxCost - pathCost;
			segLen += extraR + extraFR; // gets subtracted afterwards

			// Assumes that the startPos is valid and can be outside the triangle.
			// Find the furthest point on the target triangle and clamp the movement there.
			// Since the lineseg poly intersection always returns the nearest hit,
			// check in both directions.

			Lineseg lineseg0(startPos, startPos + dir * segLen);
			Lineseg lineseg1(lineseg0.end, lineseg0.start);

			Vec3 intPt, tmp;
			float distSq = -1.0f;

			if (Intersect::Lineseg_Polygon2D(lineseg0, poly, tmp))
			{
				distSq = Distance::Point_PointSq(startPos, tmp);
				intPt = tmp;
			}
			if (Intersect::Lineseg_Polygon2D(lineseg1, poly, tmp))
			{
				float d = Distance::Point_PointSq(startPos, tmp);
				if (d > distSq)
				{
					distSq = d;
					intPt = tmp;
				}
			}
			if (distSq > sqr(extraR + extraFR))
			{
				intPt -= (extraR + extraFR) * dir;
				pathCost += (intPt - currentPos).GetLength();
				path.push_back(PathPointDescriptor(IAISystem::NAV_TRIANGULAR, intPt));
			}

			return pathCost;
		}

		float pathSegmentDist = bestPathSegment.CompMul(heuristicZScaleVec).GetLength();
		float pathSegmentCost = bestHeuristicCost * (pathSegmentDist / bestHeuristicDist);

		float newCost = pathCost + pathSegmentCost;
		if (newCost < maxCost)
		{
			path.push_back(PathPointDescriptor(IAISystem::NAV_TRIANGULAR, bestEdgePos));
			pathCost = newCost;
		}
		else 
		{
			float frac = (maxCost - pathCost) / (newCost - pathCost);
			Vec3 finalPos = frac * bestEdgePos + (1.0f - frac) * currentPos;
			path.push_back(PathPointDescriptor(IAISystem::NAV_TRIANGULAR, finalPos));
			return maxCost;
		}
		currentNodeIndex = bestNextNodeIndex;
	}

	path.push_back(PathPointDescriptor(IAISystem::NAV_TRIANGULAR, request.startPos + dir * maxCost));

	return maxCost;
}

//====================================================================
// AttemptStraightPath
//====================================================================
float CPathfinder::AttemptStraightPath(TPathPoints& straightPath, 
																		 const PathfindRequest &request,
																		 const Vec3 &curPos,
																		 const CHeuristic* pHeuristic,
																		 float maxCost,
																		 const NavigationBlockers& navigationBlockers, 
																		 bool returnPartialPath)
{
	CAISystem * pAISystem = GetAISystem();
	FUNCTION_PROFILER( gEnv->pSystem,PROFILE_AI );

	CGraph * pGraph = gAIEnv.pGraph;
	GraphNode* pStart = pGraph->GetNodeManager().GetNode(request.startIndex);
	GraphNode* pEnd = pGraph->GetNodeManager().GetNode(request.endIndex);
	IAISystem::ENavigationType navType = pStart->navType;

	if (navType != pEnd->navType)
		return -1.0f;

	float cost = -1.0f;
	CNavRegion *pRegion = gAIEnv.pNavigation->GetNavRegion(navType, pGraph);
	if (pRegion)
		cost = pRegion->AttemptStraightPath(straightPath, m_pAStarSolver, pHeuristic, request.startPos, request.endPos, 
		request.startIndex, maxCost, navigationBlockers, returnPartialPath);

	if (straightPath.size() < 2 || cost < 0.0f)
		return -1.0f;

	Vec3 deltaToEnd = request.endPos - curPos;

	// tidy up the start of the path here
	bool cutOne = false;
	while (straightPath.size() > 1)
	{
		const PathPointDescriptor& ppd = straightPath.front();
		const Vec3& pt = ppd.vPos;
		if ( ( (curPos - pt) | deltaToEnd) > 0.0f)
		{
			straightPath.erase(straightPath.begin());
			cutOne = true;
		}
		else
		{
			break;
		}
	}
	if (cutOne)
		straightPath.push_front(PathPointDescriptor(navType, curPos));

	return cost;
}

//====================================================================
// RescheduleCurrentPathfindRequest
//====================================================================
void CPathfinder::RescheduleCurrentPathfindRequest()
{
	if (m_pCurrentRequest)
	{
		m_pAStarSolver->AbortAStar();
		m_lstPathQueue.push_back(m_pCurrentRequest);
		m_pCurrentRequest = 0;
	}
	m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;
}

//====================================================================
// CancelPath
//====================================================================
void CPathfinder::CancelPath(int id)
{
	if (id <= 0)
		return;

	AILogComment("Cancelling path #%d", id);
	if (m_pCurrentRequest)
	{
		if (m_pCurrentRequest->id == id)
		{
			delete m_pCurrentRequest;
			m_pCurrentRequest = 0;
			m_pAStarSolver->AbortAStar();
		}
	}

	PathQueue::iterator pi;
	for (pi=m_lstPathQueue.begin();pi!=m_lstPathQueue.end();)
	{
		if ((*pi)->id == id)
		{
			delete *pi;
			pi = m_lstPathQueue.erase(pi);
		}
		else
		{
			++pi;
		}
	}
}

//====================================================================
// IsFindingPathFor
//====================================================================
bool CPathfinder::IsFindingPathFor(const IAIPathAgent *pRequester) const
{
	if (!pRequester)
		return false;

	if (m_pCurrentRequest && m_pCurrentRequest->type == PathfindRequest::TYPE_ACTOR &&
		m_pCurrentRequest->pRequester == pRequester)
		return true;

	PathQueue::const_iterator pi;
	for (pi=m_lstPathQueue.begin();pi!=m_lstPathQueue.end();++pi)
	{
		if ((*pi)->type == PathfindRequest::TYPE_ACTOR && (*pi)->pRequester == pRequester)
			return true;
	}
	return false;
}

void CPathfinder::Reset(IAISystem::EResetReason reason)
{
	// TODO Jan 24, 2008: <pvl> look into just calling FlushPathQueue() here
	while (!m_lstPathQueue.empty())
	{
		PathfindRequest *pReq = m_lstPathQueue.front();
		m_lstPathQueue.pop_front();
		delete pReq;
	}

	if (m_pCurrentRequest)
	{
		delete m_pCurrentRequest;
		m_pCurrentRequest = 0;
	}

	m_nPathfinderResult = PATHFINDER_POPNEWREQUEST;
}

void CPathfinder::FlushPathQueue ()
{
	// TODO Jan 24, 2008: <pvl> look into using IsPathQueueEmpty() here
	while (!m_lstPathQueue.empty())
	{
		PathfindRequest *pRequest = m_lstPathQueue.front();
		delete pRequest;
		m_lstPathQueue.pop_front();
	}
}

bool CPathfinder::IsPathQueueEmpty () const
{
	return m_lstPathQueue.empty ();
}

void CPathfinder::Serialize( TSerialize ser, CObjectTracker& objectTracker )
{
	ser.BeginGroup("CPathfinder");
	{
		ser.Value("m_pathFindIdGen", m_pathFindIdGen);

		ser.BeginGroup("PathRequestQueue");
		{
			unsigned pathQueueSize = m_lstPathQueue.size();
			ser.Value("pathQueueSize", pathQueueSize);

			if (ser.IsReading())
			{
				AIAssert(m_lstPathQueue.empty());
				for (unsigned i = 0 ; i < pathQueueSize ; ++i)
				{
					PathfindRequest* request = new PathfindRequest(PathfindRequest::TYPE_ACTOR);
					request->Serialize(ser, objectTracker);
				}
			}
			else
			{
				for (PathQueue::const_iterator it = m_lstPathQueue.begin() ; it != m_lstPathQueue.end() ; ++it)
				{
					PathfindRequest* request = *it;
					AIAssert(request);
					request->Serialize(ser, objectTracker);
				}
			}
		}
		ser.EndGroup();

		if(ser.IsWriting())
		{
			if(ser.BeginOptionalGroup("m_pCurrentRequest", m_pCurrentRequest!=NULL))
			{
				assert(m_pCurrentRequest);
				m_pCurrentRequest->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
		}
		else
		{
			SAFE_DELETE(m_pCurrentRequest);
			if(ser.BeginOptionalGroup("m_pCurrentRequest", true))
			{
				m_pCurrentRequest = new PathfindRequest(PathfindRequest::TYPE_ACTOR);
				m_pCurrentRequest->Serialize(ser, objectTracker);
				ser.EndGroup();
			}
		}

		ser.EnumValue("m_nPathfinderResult", m_nPathfinderResult, PATHFINDER_STILLFINDING, PATHFINDER_MAXVALUE);

		ser.Value("m_fLastPathfindTimeStart",m_fLastPathfindTimeStart);

		// TODO Jan 24, 2008: <pvl> there was a whole bunch of stuff between
		// previous and following code in the original CAISystem::Serialize()
		// function.  Make sure somehow that this implicit reordering doesn't
		// break anything.
		m_pAStarSolver->Serialize(ser, objectTracker);

		m_navPath.Serialize(ser, objectTracker);
	}
	ser.EndGroup();
}

void CPathfinder::SerializePointers( TSerialize ser, CObjectTracker& objectTracker )
{
	objectTracker.SerializeObjectPointer(ser, "m_pAStarSolver", m_pAStarSolver, true);
	objectTracker.SerializeObjectReference(ser, "sStandardHeuristic", sStandardHeuristic);
	objectTracker.SerializeObjectReference(ser, "m_navPath", m_navPath);
}

void CPathfinder::GetMemoryStatistics(ICrySizer *pSizer)
{
	size_t size = m_lstPathQueue.size()*(sizeof(PathfindRequest*)+sizeof(PathfindRequest));

	pSizer->AddObject( this, size); 

	{
		SIZER_SUBCOMPONENT_NAME(pSizer,"AStar");
		if(m_pAStarSolver)
			pSizer->AddObject( m_pAStarSolver, m_pAStarSolver->MemStats());
	}
}

const AStarSearchNodeVector& CPathfinder::GetTaggedNodesVector()
{
	return m_pAStarSolver->GetTaggedNodesVector();
}
