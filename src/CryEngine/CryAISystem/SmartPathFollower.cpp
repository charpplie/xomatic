/********************************************************************
CryGame Source File.
Copyright (C), StalTech Studios, 2001-2010.
---------------------------------------------------------------------
File name:   SmartPathFollower.cpp
Version:     v1.00
Description: PathFollower implementaion based on tracking the 
furthest reachable position on the path.

---------------------------------------------------------------------
History:
- 16 Feb 2010  : Will Wilson: Created.

*********************************************************************/

#include "StdAfx.h"

#include "SmartPathFollower.h"
#include "CAISystem.h"
#include "AILog.h"
#include "NavPath.h"
#include "NavRegion.h"
#include "DebugDrawContext.h"

// TODO: Can this live in Cry_Math.h? Already defined in the CryAction pch...
template<class T>
inline T Lerp(const T& a, const T& b, float s)
{
	return T(a + s * (b - a));
}

// Search for a segment index based on the distance along the path, startIndex is optional to allow search optimization
float InterpolatedPath::FindSegmentIndexAtDistance(float requestedDistance, size_t startIndex) const
{
	// Default to returning the last index (or -1 if empty)
	float result = static_cast<float>(m_points.size()) - 1.0f;

	// Limit to start
	if (requestedDistance < 0.0f)
		requestedDistance = 0.0f;

	if (!m_points.empty() && requestedDistance < m_totalDistance)
	{
		TPoints::const_iterator iter(m_points.begin());

		// Move index to start of search
		std::advance(iter, startIndex);

		float prevDistance = iter->distance;
		AIAssert(prevDistance <= requestedDistance);

		for (; iter != m_points.end(); ++iter)
		{
			const SPathControlPoint2& point = *iter;
			const float pointDistance = point.distance;

			// If waypoint at requested distance
			if (pointDistance >= requestedDistance)
			{
				float endIndex = static_cast<float>(std::distance(m_points.begin(), iter));
				float startIndex = (iter != m_points.begin()) ? endIndex - 1.0f : 0.0f;
				
				float distanceWithinSegment = requestedDistance - prevDistance;
				float segmentDistance = pointDistance - prevDistance;
				AIAssert(distanceWithinSegment <= segmentDistance);

				float delta = (segmentDistance > 0.001f) ? distanceWithinSegment / segmentDistance : 0.0f;

				AIAssert(delta >= 0.0f && delta <= 1.0f);

				result = Lerp(startIndex, endIndex, delta);
				break;
			}

			prevDistance = pointDistance;
		}
	}

	return result;
}

// Returns the segment index of the closest point on the path (fractional part provides exact position if required)
float InterpolatedPath::FindClosestSegmentIndex(const Vec3& testPoint, float startDistance, float endDistance) const
{
	AIAssert(startDistance <= endDistance);

	float bestIndex = -1.0f;

	if (!m_points.empty())
	{
		// FIXME: Does this handle case of 2 point paths?

		// If the point is this close or closer, consider it on the segment and stop searching
		static const float MIN_DISTANCE_TO_SEGMENT = 0.01f;

		float bestDistSq = std::numeric_limits<float>::max();

		float startIndex = FindSegmentIndexAtDistance(startDistance);
		size_t startIndexInt = static_cast<size_t>(startIndex);
		float endIndex = FindSegmentIndexAtDistance(endDistance, startIndexInt);

		Vec3 startPos(GetPositionAtSegmentIndex(startIndex));
		Vec3 endPos(GetPositionAtSegmentIndex(endIndex));

		// Move index to start of search
		TPoints::const_iterator iter(m_points.begin());
		std::advance(iter, startIndexInt);

		const TPoints::const_iterator finalIter(m_points.begin() + static_cast<size_t>(endIndex));

		float minIndex = startIndex;
		Vec3 prevPoint(startPos);
		//float prevDistance = startDistance;
		for (; iter != finalIter; ++iter)
		{
			const SPathControlPoint2& point = *iter;

			Lineseg segment(prevPoint, point.pos);

			const float maxIndex = static_cast<float>(std::distance(m_points.begin(), iter));
			
			float segmentDelta;
			float distToSegmentSq = Distance::Point_Lineseg2DSq(testPoint, segment, segmentDelta);

			// If this is a new best
			if (bestDistSq >= distToSegmentSq)
			{
				bestDistSq = distToSegmentSq;

				// Calculate index delta for segment (may not be 1 at start and end)
				float indexDelta = maxIndex - minIndex;
				// Calculate the segment index by incorporating the delta returned by the segment test
				bestIndex = minIndex + (segmentDelta * indexDelta);

				// Early out if point on line - it can't get any closer
				if (bestDistSq <= square(MIN_DISTANCE_TO_SEGMENT))
					break;
			}

			prevPoint = point.pos;
			minIndex = maxIndex;
		}

		// If no close match was found
		if (bestDistSq > square(MIN_DISTANCE_TO_SEGMENT))
		{
			// Test the final segment using the selected end position
			Lineseg segment(prevPoint, endPos);

			float segmentDelta;
			float distToSegmentSq = Distance::Point_Lineseg2DSq(testPoint, segment, segmentDelta);

			// If this is a new best
			if (bestDistSq >= distToSegmentSq)
			{
				bestDistSq = distToSegmentSq;

				const float maxIndex = static_cast<float>(m_points.size()) - 1.0f;

				// Calculate index delta for segment (may not be 1 at start and end)
				float indexDelta = maxIndex - minIndex;
				// Calculate the segment index by incorporating the delta returned by the segment test
				bestIndex = minIndex + (segmentDelta * indexDelta);
			}
		}
	}

	return bestIndex;
}

/// True if the path section between the start and endIndex deviates from the line by no more than maxDeviation.
bool InterpolatedPath::IsParrallelTo(const Lineseg& line, float startIndex, float endIndex, float maxDeviation) const
{
	AIAssert(startIndex <= endIndex);
	
	// Move along path between start point (inclusive) and end point (exclusive) testing each control point
	for (float index = startIndex; index < endIndex; index = floorf(index) + 1.0f)
	{
		Vec3 testPos(GetPositionAtSegmentIndex(index));

		float delta;
		if (Distance::Point_Lineseg2DSq(testPos, line, delta) > maxDeviation)
			return false;
	}
	
	// Finally test the end position
	Vec3 endPos(GetPositionAtSegmentIndex(endIndex));

	float delta;
	return (Distance::Point_Lineseg2DSq(endPos, line, delta) <= maxDeviation);
}

Vec3 InterpolatedPath::GetPositionAtSegmentIndex(float index) const
{
	// Bound index to minimum
	index = max(index, 0.0f);

	float delta = fmodf(index, 1.0f);

	size_t startIndex = static_cast<size_t>(index);
	size_t endIndex = startIndex + 1;

	Vec3 startPos(m_points[startIndex].pos);
	Vec3 endPos((endIndex < m_points.size()) ? m_points[endIndex].pos : startPos);

	return Lerp(startPos, endPos, delta);
}

float InterpolatedPath::GetDistanceAtSegmentIndex(float index) const
{
	// Bound index to minimum
	index = max(index, 0.0f);

	float delta = fmodf(index, 1.0f);

	size_t startIndex = static_cast<size_t>(index);
	size_t endIndex = startIndex + 1;

	float startDist = m_points[startIndex].distance;
	float endDist = (endIndex < m_points.size()) ? m_points[endIndex].distance : startDist;

	return Lerp(startDist, endDist, delta);
}

void InterpolatedPath::GetLineSegment(float startIndex, float endIndex, Lineseg& segment) const
{
	segment.start = GetPositionAtSegmentIndex(startIndex);
	segment.end = GetPositionAtSegmentIndex(endIndex);
}

// Returns the index of the first navigation type transition after index or end of path
size_t InterpolatedPath::FindNextNavTypeSectionIndexAfter(size_t index) const
{
	// Default is to return the last valid index (or ~0 if empty)
	size_t changeIndex = m_points.size() - 1;

	// If index safely on path
	if (index < changeIndex)
	{
		TPoints::const_iterator iter(m_points.begin() + index);
		const IAISystem::ENavigationType prevNavType = iter->navType;

		while (++iter != m_points.end())
		{
			// If the nav type changes
			if (iter->navType != prevNavType)
			{
				changeIndex = std::distance<TPoints::const_iterator>(m_points.begin(), iter);
				break;
			}
		}
	}

	return changeIndex;
}

// Returns the next index on the path that deviates from a straight line by the deviation specified.
float InterpolatedPath::FindNextInflectionIndex(float startIndex, float maxDeviation) const
{
	// Start at start pos and generate ray based on next segment start point
	Lineseg testLine;
	testLine.start = GetPositionAtSegmentIndex(startIndex);
	testLine.end = testLine.start;

	const float maxDeviationSq = square(maxDeviation);

	float searchStartIndex = ceilf(startIndex);
	float searchStartDist = GetDistanceAtSegmentIndex(searchStartIndex);
	float searchEndDist = min(searchStartDist + 3.0f, m_totalDistance);

	const float distIncrement = 0.1f;
	float lastSafeIndex = searchStartIndex;

	for (float dist = searchStartDist; dist < searchEndDist; dist += distIncrement)
	{
		// Update line
		const float index = FindSegmentIndexAtDistance(dist, static_cast<size_t>(startIndex));
		testLine.end = GetPositionAtSegmentIndex(index);

		const size_t subIndexEnd = static_cast<size_t>(ceilf(index));

		// Test deviation against test range of the path
		for (size_t subIndex = static_cast<size_t>(searchStartIndex) + 1; subIndex < subIndexEnd; ++subIndex)
		{
			float delta;
			float deviationSq = Distance::Point_Lineseg2DSq(m_points[subIndex].pos, testLine, delta);
			if (deviationSq > maxDeviationSq)
			{
				// This point is no longer safe, use the last safe index
				return lastSafeIndex;
			}
		}

		lastSafeIndex = index;
	}

	return lastSafeIndex;
}


// Shortens the path to the specified endIndex
void InterpolatedPath::ShortenToIndex(float endIndex)
{
	// If cut is actually on the path
	if (endIndex >= 0.0f && endIndex < m_points.size())
	{
		// Create the cut index based on the next integer index after the endIndex
		size_t cutIndex = static_cast<size_t>(endIndex) + 1;

		float delta = fmodf(endIndex, 1.0f);

		// Create a new point based on the original
		SPathControlPoint2 newEndPoint(m_points[static_cast<size_t>(endIndex)]);
		newEndPoint.pos = GetPositionAtSegmentIndex(endIndex);
		newEndPoint.distance = GetDistanceAtSegmentIndex(endIndex);
		// NOTE: New end point copies other values from the point immediately before it. Safe assumption?

		// Erase removed section of the path
		m_points.erase(m_points.begin() + cutIndex, m_points.end());

		m_points.push_back(newEndPoint);
	}
}



void InterpolatedPath::push_back(const SPathControlPoint2& point)
{
	float distance = 0.0f;

	if (!m_points.empty())
		distance = m_points.back().pos.GetDistance(point.pos);

	m_points.push_back(point);

	float cumulativeDistance = m_totalDistance + distance;
	m_points.back().distance = cumulativeDistance;

	m_totalDistance = cumulativeDistance;
}



//===================================================================
// CSmartPathFollower
//===================================================================
CSmartPathFollower::CSmartPathFollower(const PathFollowerParams &params, const CPathObstacles* pPathObstacles)
: m_params(params),
	m_pathVersion(-2),
	m_followTargetIndex(),
	m_inflectionIndex(),
	m_pNavPath(),
	m_pPathObstacles(pPathObstacles),
	m_curPos(ZERO)
// 	m_safePathOptimal(false),
// 	m_reachTestCount()
{
	Reset();
}

//===================================================================
// CSmartPathFollower
//===================================================================
CSmartPathFollower::~CSmartPathFollower()
{
}

// Attempts to find a reachable target between startIndex (exclusive) & endIndex (inclusive).
// Successful forward searches return the last reachable target found advancing up the path.
// Successful reverse searches return the first reachable target found reversing back down the path.
bool CSmartPathFollower::FindReachableTarget(float startIndex, float endIndex, float& reachableIndex) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	// Default fail value
	reachableIndex = -1.0f;

	float startDistance = m_path.GetDistanceAtSegmentIndex(startIndex);
	float endDistance = m_path.GetDistanceAtSegmentIndex(endIndex);

	// This can be negative if searching backwards along path
	float lengthToTest = endDistance - startDistance;
	float lengthToTestAbs = fabsf(lengthToTest);

	// Use this as the starting offset distance
	const float minTestDist = 0.4f; 			// TODO: Parameterize
	
	// Direction of search determines the termination logic
	const bool lookingAhead = (lengthToTest >= 0.0f);

	// This is used to ease calculation of path distance without worrying about direction of search
	const float dirMultiplier = (lookingAhead) ? 1.0f : -1.0f;
	
	float advanceDist = minTestDist;
	// Move along the path starting at minTestDist
	for (float testOffsetDist = minTestDist; testOffsetDist < lengthToTestAbs; testOffsetDist += advanceDist)
	{
		float testDist = startDistance + (testOffsetDist * dirMultiplier);
		float testIndex = m_path.FindSegmentIndexAtDistance(testDist);			// TODO: Optimize, needs range look-up support

		if (CanReachTarget(m_curPos, testIndex))
		{
			reachableIndex = testIndex;

			// If looking behind, return the first reachable index found
			if (!lookingAhead)
			{
				//m_safePathOptimal = true;
				return true;
			}
		}
		else	// Can't reach target at this index
		{
			// If looking ahead & there is a previous valid index
			if (lookingAhead && reachableIndex >= 0.0f)
			{
				// Indicate the safe path is now optimal (continue using it until)
				//m_safePathOptimal = true;
				return true;	
			}
			// Otherwise, keep looking (expensive but copes with weird situations better)
		}

		// Increase the advance distance
		advanceDist *= 1.5f;
	}

	// Finally, always test the endIndex (to support start/end of path)
	if (CanReachTarget(m_curPos, endIndex))
	{
		reachableIndex = endIndex;
	}

	return (reachableIndex >= 0.0f);
}

// True if the test position can be reached by the agent.
bool CSmartPathFollower::CanReachTarget(const Vec3 startPos, float testIndex) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	// Assume safe until proved otherwise
	bool safeLineToTarget = false;

	const SPathControlPoint2& segStartPoint = m_path[static_cast<size_t>(testIndex)];
	Vec3 testPos(m_path.GetPositionAtSegmentIndex(testIndex));

	// Firstly attempt "cheap" path query to avoid expensive CheckPassability() test.
	// This assumes that the path was and still is passable.
	{
		const float MAX_DEVIATION_FROM_PATH = square(0.1f);		// TODO: Parameterize?
		
		// Get the closest index to the starting position
		// NOTE: Even if this provides a false "closest", it should still indicate a safe path.
		const float closestIndex = m_path.FindClosestSegmentIndex(startPos);

		const Lineseg testLine(startPos, testPos);
		const float startIndex = min(closestIndex, testIndex);
		const float endIndex = max(closestIndex, testIndex);

		// There is a safe line to target if it's parallel to the path
		safeLineToTarget = m_path.IsParrallelTo(testLine, startIndex, endIndex, MAX_DEVIATION_FROM_PATH);

// 		if (safeLineToTarget)
// 			++s_cheapTestSuccess;
	}

	if (!safeLineToTarget)
	{
		// Do the expensive test and query navigation (and possibly physics)
		const IAISystem::ENavigationType navType = segStartPoint.navType;

		// Terrain paths can ignore heights, so ridges between two path points can lead to 
		// the follow target going underground - therefore this calculation is required.
		if (segStartPoint.navType == IAISystem::NAV_TRIANGULAR)
		{
			Vec3 closestPoint;

			// TODO: Hopefully move this special-case to CTriangularNavRegion::CheckPassability() if CheckWalkabilitySimple() can be used.
			// Check forbidden areas and boundaries (or falls back on attempting to escape from one)
			if (gAIEnv.pNavigation->IntersectsForbidden(startPos, testPos, closestPoint) && !gAIEnv.pNavigation->IsPointForbidden(startPos, 0.0f))
			{
				// FIXME: Does not account for agent radius!
				// Early out, can't cross forbidden areas/boundaries
				return false;
			}

			testPos.z = gEnv->p3DEngine->GetTerrainElevation(testPos.x, testPos.y);
		}

		// UGLY FUDGE: Raise the test positions up a little to better cope with sloped surfaces
		const Vec3 raiseUp(0.0f, 0.0f, 0.2f);
		const Vec3 raisedStartPos = startPos + raiseUp;
		const Vec3 raisedTestPos = testPos + raiseUp;

		if (!m_pPathObstacles || false == m_pPathObstacles->IsPathIntersectingObstacles(raisedStartPos, raisedTestPos, m_params.passRadius))
		{
			// NOTE: CheckPassability() is too expensive currently as most NavRegions simply call the (very) expensive CheckWalkability().
			// TODO: Once LNM is widely used and it implements a cheap CheckPassability() it should be used instead of the old functions below.
			// TODO: It may be possible to change the contract of CheckPassibility() to use CheckWalkabilitySimple() instead for some NavTypes.

			//const CNavRegion *pRegion = gAIEnv.pNavigation->GetNavRegion(navType, gAIEnv.pGraph);
			//if (pRegion)
			//{
			// 	NavigationBlockers navBlockers;
			// 	safeLineToTarget = pRegion->CheckPassability(raisedStartPos, raisedTestPos, m_params.passRadius, navBlockers, m_params.navCapMask);
			//}
			//else	// NOTE: NAV_UNSET appears to be used by the C2 cover system for simple paths.
			{
				// Attempt a direct walk-ability test
				if (bool simpleTest = true)
				{
					safeLineToTarget = CheckWalkabilitySimple(SWalkPosition(raisedStartPos, true),	// Start (marked as floor as it should be the agent base)
																										SWalkPosition(raisedTestPos, true),		// End (marked as floor as it should be on the path)
																										0.0f,																	// No padding radius
																										AICE_ALL);														// Check against what?

				}
				else
				{
					ListPositions boundary;
					safeLineToTarget = CheckWalkability(SWalkPosition(raisedStartPos, true),	// Start (marked as floor as it should be the agent base)
																							SWalkPosition(raisedTestPos, true),		// End (marked as floor as it should be on the path)
																							0.0f,																	// No padding radius
																							false,																// Don't check starting position (should be valid for agent)
																							boundary,															// Empty boundary
																							AICE_ALL,															// Check against what?
																							NULL,																	// Can't use cached result
																							NULL);
				}
			}
			//++m_reachTestCount;
		}
	}

	return safeLineToTarget;
}

//===================================================================
// DistancePointPoint
//===================================================================
float CSmartPathFollower::DistancePointPoint(const Vec3 pt1, const Vec3 pt2) const
{
  return m_params.use2D ? Distance::Point_Point2D(pt1, pt2) : Distance::Point_Point(pt1, pt2);
}

//===================================================================
// DistancePointPointSq
//===================================================================
float CSmartPathFollower::DistancePointPointSq(const Vec3 pt1, const Vec3 pt2) const
{
  return m_params.use2D ? Distance::Point_Point2DSq(pt1, pt2) : Distance::Point_PointSq(pt1, pt2);
}


void CSmartPathFollower::Reset()
{
	// NOTE: m_params is left unaltered
	m_pathVersion=-2;

	m_validatedStartPos.zero();
	m_followTargetIndex = 0.0f;
	m_inflectionIndex = 0.0f;

	m_path.clear();
}

//===================================================================
// AttachToPath
//===================================================================
void CSmartPathFollower::AttachToPath(INavPath *pNavPath)
{
	m_pNavPath = pNavPath;
}

//===================================================================
// ProcessPath
//===================================================================
void CSmartPathFollower::ProcessPath()
{
  AIAssert(m_pNavPath);
  m_pathVersion = m_pNavPath->GetVersion();

	// Reset cached data
  m_path.clear();
	m_followTargetIndex = 0.0f;
	m_inflectionIndex = 0.0f;
	//m_safePathOptimal = false;

	const CNavPath* pNavPath=static_cast<CNavPath *>(m_pNavPath);
  const TPathPoints& pathPts = pNavPath->GetPath();

  if (pathPts.size() < 2)
    return;

  const TPathPoints::const_iterator itEnd = pathPts.end();

	SPathControlPoint2 pathPoint;
	pathPoint.distance			= 0.0f;

	// For each existing path segment
	for (TPathPoints::const_iterator it = pathPts.begin() ; it != itEnd ; ++it)
	{
		// Distance used to look ahead and behind (magic number)
		const PathPointDescriptor &ppd = *it;

		pathPoint.navType				= ppd.navType;
		pathPoint.pos						= ppd.vPos;

		m_path.push_back(pathPoint);
	}

	// Ensure end point is at ground level (very often it isn't)
	{
 		SPathControlPoint2& endPoint = m_path.back();
		
		// FIXME: GetFloorPos() is failing in some cases
// 		Vec3 floorPos;
// 		if (GetFloorPos(floorPos,							// Found floor position (if successful)
// 										endPoint.pos + Vec3(0.0f, 0.0f, 0.5f),					// Existing end position
// 										0.0f,									// Up distance (already included)
// 										2.0f,									// End below floor position
// 										m_params.passRadius,	// Test radius (avoid falling through cracks)
// 										AICE_STATIC))					// Only consider static obstacles
// 		{
// 			endPoint.pos = floorPos;
// 		}
// 		else
// 		{
// 			// Can't find the floor!!
// 			IPersistantDebug* pDebug = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();
// 			pDebug->AddSphere(endPoint.pos, 0.6f, ColorF(1.0f, 0, 0), 5.0f);
// 			//AIAssert(false);
// 		}

		// Snap to previous point height - assuming it is safe
		if (m_path.size() > 1)
		{
			const SPathControlPoint2& prevPoint = m_path[m_path.size() - 2];
			endPoint.pos.z = min(prevPoint.pos.z + 0.1f, endPoint.pos.z);
		}
	}

	// Now cut the path short if we should stop before the end
	if (m_params.endDistance > 0.0f)
	{
		float newDistance = m_path.TotalDistance() - m_params.endDistance;
		float endIndex = m_path.FindSegmentIndexAtDistance(newDistance);
		m_path.ShortenToIndex(endIndex);
	}

  AIAssert(m_path.size() >= 2);
}


// Attempts to advance the follow target along the path as far as possible while ensuring the follow 
// target remains reachable. Returns true if the follow target is reachable, false otherwise.
bool CSmartPathFollower::Update(PathFollowResult &result, const Vec3 &curPos, const Vec3 &curVel, float dt)
{
  FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	bool targetReachable = true;
	//m_reachTestCount = 0;
	
	CAISystem* pAISystem = GetAISystem();

	// If path has changed
	if (m_pathVersion != m_pNavPath->GetVersion())
	{
		// Duplicate, fix end height and optionally shorten path
		ProcessPath();
	}

	// Set result defaults (ensure no undefined data is passed back to the caller)
	{
		// This is used to vaguely indicate if the FT has reached path end and so has the agent
		result.reachedEnd = false;

		// Don't generate predicted states (obsolete)
		if (result.predictedStates)
			result.predictedStates->resize(0);

		result.followTargetPos = curPos;
		result.inflectionPoint = curPos;

		result.velocityOut.zero();
	}

	// Cope with empty path (should not occur but best to be safe)
	if (m_path.empty())
	{
		// Indicate we've reached the "end" of the empty path
		result.reachedEnd = true;
		return true;
	}

	m_curPos = curPos;

	Vec3 followTargetPos = m_path.GetPositionAtSegmentIndex(m_followTargetIndex);
	Vec3 inflectionPoint = m_path.GetPositionAtSegmentIndex(m_inflectionIndex);

	// TODO: Optimize case where very little deviation from path (check max distances of intervening points to line)

	bool recalculateTarget = false;
	bool onSafeLine = false;

	// If safe path not optimal or progress has been made along the path
	if (/*!m_safePathOptimal ||*/ m_followTargetIndex > 0.0f)
	{
		// Generate the safe line previously calculated
		Lineseg safeLine(m_validatedStartPos, followTargetPos);

		float delta;
		const float distToSafeLineSq = Distance::Point_LinesegSq(curPos, safeLine, delta);

		onSafeLine = distToSafeLineSq < square(0.25f);		// TODO: Parameterize & perhaps scale by proximity to target?

		// Are we still effectively on the safe line?
		if (onSafeLine)
		{
			// Have we moved significantly along safe line?
			if (delta > 0.5f)
			{
				// Is the more path left to advance the FT?
				if (m_followTargetIndex < m_path.size() - 1.001f)
				{
					// Are we close enough to the follow target to warrant recalculating the path?
					bool atEndOfSafePath = curPos.GetSquaredDistance2D(followTargetPos) < square(2.0f);		// TODO: Parameterize?

					recalculateTarget = atEndOfSafePath;
				}
			}
		}
		else	// Deviated too far from safe line
		{
			recalculateTarget = true;
		}
	}
	else	// No target yet calculated (or attempting to get to start)
	{
		recalculateTarget = true;
	}

	if (recalculateTarget)
	{
		// Generate a look-ahead range based on the current FT index.
		const float originalTargetDist = m_path.GetDistanceAtSegmentIndex(m_followTargetIndex);
		const float lookAheadIndex = m_path.FindSegmentIndexAtDistance(originalTargetDist + 10.0f);		// TODO: Parameterize?
		float newTargetIndex = -1;

		// 1. Search forward from previous FT - start small and increase until a fixed look-ahead limit.
		if (!FindReachableTarget(m_followTargetIndex, lookAheadIndex, newTargetIndex))
		{
			// 2. Try original target.
			if (onSafeLine || CanReachTarget(m_curPos, m_followTargetIndex))
			{
				newTargetIndex = m_followTargetIndex;
			}
			else	// Old target inaccessible
			{
				// 3. Find closest point on path before previous FT - use it to bound look-behind search
				// NOTE: Should be safe even if path loops and gives inaccessible closest as search is done backwards.
				float closestIndex = m_path.FindClosestSegmentIndex(curPos, 0.0f, m_path.GetDistanceAtSegmentIndex(m_followTargetIndex));		// Optimal using dist?
				float closestPathDist = m_path.GetDistanceAtSegmentIndex(closestIndex) - 5.0f;
				float lookBehindIndex = m_path.FindSegmentIndexAtDistance(closestPathDist);
				
				// 4. Search backwards from previous FT - start small and increase until beyond closet point on path.
				if (!FindReachableTarget(m_followTargetIndex, lookBehindIndex, newTargetIndex))
				{
					// 5. Admit defeat and inform caller. The caller probably needs to regenerate path.
					targetReachable = false;

#ifndef _RELEASE
					if (gAIEnv.CVars.DrawPathFollower == 1)
					{
						CDebugDrawContext dc;
						dc->Draw3dLabel(m_curPos, 1.6f, "Failed PathFollower!");
					}
#endif
				}
			}
		}

		// If valid target was found
		if (newTargetIndex >= 0.0f)
		{
			m_validatedStartPos = curPos;

			// If the index has actually changed (avoids recalculating inflection point)
			if (m_followTargetIndex != newTargetIndex)
			{
				// Update the target
				m_followTargetIndex = newTargetIndex;
				followTargetPos = m_path.GetPositionAtSegmentIndex(newTargetIndex);

				// Update inflection point
				m_inflectionIndex = m_path.FindNextInflectionIndex(newTargetIndex, m_params.pathRadius * 0.5f);		// TODO: Parameterize max deviation
				inflectionPoint = m_path.GetPositionAtSegmentIndex(m_inflectionIndex);
			}
		}
	}

	// Generate results
	{
		// Pass the absolute data (will eventually replace the old velocity based data below)
		result.followTargetPos = followTargetPos;
		result.inflectionPoint = inflectionPoint;

		// TODO: The following is deprecated. Passing motion requests using velocity only is imprecise,
		// unstable with frame time (due to interactions between animation and physics) and requires 
		// workarounds and magic numbers to make it work semi-reliably.
		if (bool allowMovement = true)
		{
			Vec3 velocity = followTargetPos - curPos;
			if (m_params.use2D)
				velocity.z = 0.0f;
			float distToEnd = velocity.NormalizeSafe();
			float speed = m_params.normalSpeed;

			// If target has reached end of path
			if (m_followTargetIndex >= m_path.size() - 1.001f)
			{
				// This is used to vaguely indicate if the FT has reached path end and so has the agent
				result.reachedEnd = distToEnd < max(0.2f, speed * dt);

				if (m_params.stopAtEnd)
				{
					float slowDownDist = 1.2f;
					// Scale speed down when within slowDownDist
					speed *= result.reachedEnd ? 0.0f : max((min(distToEnd, slowDownDist) / slowDownDist), 0.1f);
				}
			}

			// TODO: Stability might be improved by detecting and reducing velocities pointing backwards along the path.

			// Integrate speed
			velocity *= speed;

			result.velocityOut = velocity;
		}
	}

	AIAssert(result.velocityOut.IsValid());

#ifndef _RELEASE
	if (gAIEnv.CVars.DrawPathFollower == 1)
	{
		// Draw path
		Draw();

		Vec3 up(0.0f, 0.0f, 0.2f);

		// Draw the safe line & follow target
		CDebugDrawContext dc;
		ColorB debugColor = recalculateTarget ? ColorB(255, 0, 0) : ColorB(0, 255, 0);

		if (result.reachedEnd)
		{
			debugColor.g = result.velocityOut.IsZeroFast() ? 128 : 0;
			debugColor.b = 255;
		}

		dc->DrawSphere(followTargetPos + up, 0.2f, debugColor);
		dc->DrawCapsuleOutline(m_validatedStartPos + up, followTargetPos + up, m_params.passRadius, debugColor);

		// Draw inflection point
		dc->DrawSphere(inflectionPoint + up, 0.2f, ColorB(0,0,255));
		dc->DrawCapsuleOutline(followTargetPos + up, inflectionPoint + up, m_params.passRadius, ColorB(0,0,255));

		dc->DrawArrow(m_curPos + up, result.velocityOut, 0.2f, ColorB(230,200,180));

		//s_passibilityCheckMaxCount = max(s_passibilityCheckMaxCount, m_reachTestCount);
		//dc->Draw3dLabel(m_curPos + up, 1.5f, "%d/%d (%d)", m_reachTestCount, s_passibilityCheckMaxCount, s_cheapTestSuccess);
	}
#endif		// !defined _RELEASE

	return targetReachable;
}

//===================================================================
// GetDistToEnd
//===================================================================
float CSmartPathFollower::GetDistToEnd(const Vec3 *pCurPos) const
{
	float distanceToEnd = 0.0f;

	if (!m_path.empty())
	{
		distanceToEnd = m_path.TotalDistance() - m_path.GetDistanceAtSegmentIndex(m_followTargetIndex);
		if (pCurPos)
		{
			Vec3 followTargetPos(m_path.GetPositionAtSegmentIndex(m_followTargetIndex));
			distanceToEnd += DistancePointPoint(*pCurPos, followTargetPos);
		}
	}

  return distanceToEnd;
}

//===================================================================
// Serialize
//===================================================================
void CSmartPathFollower::Serialize(TSerialize ser, class CObjectTracker& objectTracker)
{
  ser.Value("m_params", m_params);

	ser.Value("m_followTargetIndex", m_followTargetIndex);
	ser.Value("m_inflectionIndex", m_inflectionIndex);
	ser.Value("m_validatedStartPos", m_validatedStartPos);

  if (ser.IsReading())
	{
		// NOTE: It's assumed the path will be rebuilt as we don't serialize the path version.
		// If we're reading, we need to rebuild the cached path by creating an "invalid" path version.
		m_pathVersion = -2;
	}
}

//===================================================================
// Draw
//===================================================================
void CSmartPathFollower::Draw(const Vec3& drawOffset) const
{
	CDebugDrawContext dc;
	size_t pathSize = m_path.size();
	for (size_t i = 0; i < pathSize; ++i)
	{
		Vec3 prevControlPoint(m_path.GetPositionAtSegmentIndex(static_cast<float>(i) - 1.0f));
		Vec3 thisControlPoint(m_path.GetPositionAtSegmentIndex(static_cast<float>(i)));

		dc->DrawLine(prevControlPoint, ColorB(0, 0, 0), thisControlPoint, ColorB(0, 0, 0));
		dc->DrawSphere(thisControlPoint, 0.05f, ColorB(0, 0, 0));
		dc->Draw3dLabel(thisControlPoint, 1.5f, "%d", i);
	}
}

//===================================================================
// GetDistToSmartObject
//===================================================================
float CSmartPathFollower::GetDistToSmartObject() const
{
	return GetDistToNavType(IAISystem::NAV_SMARTOBJECT);
}


float CSmartPathFollower::GetDistToNavType(IAISystem::ENavigationType navType) const
{
	// NOTE: This function is used by the Trace Op to detect movement through straight SO's (those that define no actions).
	// It's used to preclude path regeneration once the SO is within 1m (magic number).
	// This whole thing is poorly formed & ideally needs to be replaced by FT waiting to ensure accurate positioning.

	// TODO: Replace this *workaround* with wait system.

	if (!m_path.empty())
	{
		// FIXME: Stupid and expensive (but the original behavior)
		const float closestIndex = m_path.FindClosestSegmentIndex(m_curPos);
		const size_t closetIndexInt = static_cast<size_t>(closestIndex);

		size_t nPts = m_path.size();

		// If at end of path
		if (m_followTargetIndex + 1 >= nPts)
			return std::numeric_limits<float>::max();
		
		// Work out segment index for agent position
		float curDist = DistancePointPoint(m_curPos, m_path[closetIndexInt].pos);
		float totalDist = 0.0f;
		if (closetIndexInt + 1 < nPts)
		{
			totalDist = DistancePointPoint(m_path[closetIndexInt].pos, m_path[closetIndexInt+1].pos);
		}

		float curFraction = (totalDist > 0.0f) ? (curDist / totalDist) : 0.0f;

		// If current segment is of the selected nav type and equal customID.
		if ((m_path[closetIndexInt].navType == navType) &&
			(m_path[closetIndexInt+1].navType == navType) &&
			(m_path[closetIndexInt].customId == m_path[closetIndexInt+1].customId))
		{
			// If over half-way through segment - we're there! Otherwise not...
			return (curFraction < 0.5f) ? 0.0f : std::numeric_limits<float>::max();
		}

		// Go through all segments looking for navType
		float dist = 0.0f;
		Vec3 lastPos = m_curPos;
		for (int i = closetIndexInt+1 ; i < nPts ; ++i)
		{
			dist += DistancePointPoint(lastPos, m_path[i].pos);
			if(i + 1 < nPts)
			{
				if ((m_path[i].navType == navType) &&
					(m_path[i+1].navType == navType) &&
					(m_path[i].customId == m_path[i+1].customId))
					return dist;
			}
			else
			{
				if (m_path[i].navType == navType)
					return dist;
			}
			lastPos = m_path[i].pos;
		}
	}

	// Not found
	return std::numeric_limits<float>::max();
}

//===================================================================
// GetDistToCustomNav
//===================================================================
float CSmartPathFollower::GetDistToCustomNav(const InterpolatedPath &controlPoints, uint32 curLASegmentIndex, const Vec3 &curLAPos) const
{
	size_t nPts = controlPoints.size();

	float dist = std::numeric_limits<float>::max();

	if (curLASegmentIndex + 1 < nPts)
	{

		dist = 0.0f;

		if (controlPoints[curLASegmentIndex].navType != IAISystem::NAV_CUSTOM_NAVIGATION ||
			controlPoints[curLASegmentIndex+1].navType != IAISystem::NAV_CUSTOM_NAVIGATION)
		{
			Vec3 lastPos = curLAPos;

			for (int i = curLASegmentIndex+1 ; i < nPts ; ++i)
			{
				dist += DistancePointPoint(lastPos, m_path[i].pos);
				if(i + 1 < nPts)
				{
					if (controlPoints[i].navType == IAISystem::NAV_CUSTOM_NAVIGATION &&
						controlPoints[i+1].navType == IAISystem::NAV_CUSTOM_NAVIGATION)
					{
						break;
					}
				}
				else
				{
					if (controlPoints[i].navType == IAISystem::NAV_CUSTOM_NAVIGATION)
					{
						break;
					}
				}

				lastPos = controlPoints[i].pos;
			}
		}
	}

	return dist;
}


//===================================================================
// GetPathPointAhead
//===================================================================
Vec3 CSmartPathFollower::GetPathPointAhead(float requestedDist, float &actualDist) const
{
	Vec3 followTargetPos(m_path.GetPositionAtSegmentIndex(m_followTargetIndex));

	float posDist = m_curPos.GetDistance(followTargetPos);
	if (requestedDist <= posDist)
	{
		actualDist = requestedDist;
		return Lerp(m_curPos, followTargetPos, (posDist > 0.0f) ? (requestedDist / posDist) : 1.0f);
	}

	// Remove the safe line distance
	requestedDist -= posDist;

	float ftDist = m_path.GetDistanceAtSegmentIndex(m_followTargetIndex);

	// Find the end distance along the path
	float endDist = ftDist + requestedDist;
	if (endDist > m_path.TotalDistance())
		endDist = m_path.TotalDistance();

	// Actual distance ahead = (distance along line) + distance from FT to agent position
	actualDist = (endDist - ftDist) + posDist;
	
	float endIndex = m_path.FindSegmentIndexAtDistance(endDist);

	return m_path.GetPositionAtSegmentIndex(endIndex);
}


