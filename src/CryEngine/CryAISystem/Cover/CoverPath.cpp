#include "StdAfx.h"
#include "CoverPath.h"

#include "DebugDrawContext.h"


const CCoverPath::Points& CCoverPath::GetPoints() const
{
	return m_points;
}


void CCoverPath::Clear()
{
	m_points.clear();
}


void CCoverPath::Reserve(uint32 pointCount)
{
	m_points.reserve(pointCount);
}


void CCoverPath::AddPoint(const Vec3& point)
{
	if (!m_points.empty())
	{
		float distance = m_points.back().distance + (m_points.back().position - point).len();
		m_points.push_back(Point(point, distance));
	}
	else
		m_points.push_back(Point(point, 0.0f));
}


Vec3 CCoverPath::GetPointAt(float distance) const
{
	uint32 pointCount = m_points.size();
	uint32 current = 0;

	while (current < pointCount-1)
	{
		const Point& left = m_points[current];
		const Point& right = m_points[current + 1];

		if ((distance >= left.distance) && (distance < right.distance))
			break;

		++current;
	}

	if (current != pointCount)
	{
		const Point& left = m_points[current];
		const Point& right = m_points[current + 1];

		float frac = (distance - left.distance) / (right.distance - left.distance);
		
		return left.position + (right.position - left.position) * frac;
	}

	return Vec3(ZERO);
}


Vec3 CCoverPath::GetClosestPoint(const Vec3& point, float* distanceToPath, float* distanceOnPath) const
{
	uint32 pointCount = m_points.size();
	
	float shortestSq = FLT_MAX;
	float shortestT = FLT_MAX;
	uint32 shortestLeft = -1;
	
	for (uint32 i = 0; i < pointCount - 1; ++i)
	{
		const Point& left = m_points[i];
		const Point& right = m_points[i + 1];

		Lineseg	pathSegment(left.position, right.position);

		float t;
		float	distToSegmentSq = Distance::Point_Lineseg2DSq(point, pathSegment, t);

		if (distToSegmentSq <= shortestSq)
		{
			shortestSq = distToSegmentSq;
			shortestT = t;
			shortestLeft = i;
		}
	}

	const Point& left = m_points[shortestLeft];
	const Point& right = m_points[shortestLeft + 1];

	if (distanceToPath)
		*distanceToPath = cry_sqrtf(shortestSq);
	
	if (distanceOnPath)
		*distanceOnPath = left.distance + shortestT * (right.distance - left.distance);

	return left.position + (right.position - left.position) * shortestT;
}


float CCoverPath::GetDistanceAt(const Vec3& point, float tolerance) const
{
	uint32 pointCount = m_points.size();

	for (uint32 i = 0; i < pointCount - 1; ++i)
	{
		const Point& left = m_points[i];
		const Point& right = m_points[i + 1];

		Lineseg	pathSegment(left.position, right.position);
		
		float t;
		float	distToSegmentSq = Distance::Point_Lineseg2DSq(point, pathSegment, t);

		if (distToSegmentSq <= sqr(tolerance))
			return left.distance + t * (right.distance - left.distance);
	}
	
	return -1.0f;
}


bool CCoverPath::Intersect(const Vec3& origin, const Vec3& dir, float* distance, Vec3* point) const
{
	uint32 pointCount = m_points.size();

	Lineseg ray(origin, origin + dir);

	int closestLeftIdx = -1;
	float closestA = FLT_MAX;
	float closestB = FLT_MAX;

	for (uint32 i = 0; i < pointCount - 1; ++i)
	{
		const Point& left = m_points[i];
		const Point& right = m_points[i + 1];

		Lineseg	pathSegment(left.position, right.position);

		float a, b;

		Vec3 delta = ray.start - pathSegment.start;
		Vec3 segmentDir = pathSegment.end - pathSegment.start;

		float crossD = segmentDir.x * dir.y - segmentDir.y * dir.x;
		float crossDelta1 = delta.x * dir.y - delta.y * dir.x;
		float crossDelta2 = delta.x * segmentDir.y - delta.y * segmentDir.x;

		if (cry_fabsf(crossD) > 0.0001f)
		{
			a = crossDelta1 / crossD;
			b = crossDelta2 / crossD;

			if (a > 1.0f || a < 0.0f || b > 1.0f || b < 0.0f)
				continue;

			if (b < closestB)
			{
				closestA = a;
				closestB = b;
				closestLeftIdx = i;
			}
		}
	}

	if (closestLeftIdx != -1)
	{
		const Point& left = m_points[closestLeftIdx];
		const Point& right = m_points[closestLeftIdx + 1];

		if (distance)
			*distance = left.distance + closestA * (right.distance - left.distance);

		if (point)
			*point = left.position +  closestA * (right.position - left.position);

		return true;
	}

	return false;
}


void CCoverPath::DebugDraw()
{
	uint32 pointCount = m_points.size();

	if (pointCount < 2)
		return;

	CDebugDrawContext dc;

	Points::const_iterator rightIt = m_points.begin();
	Points::const_iterator leftIt = rightIt++;
	
	for (uint32 i = 0; i < pointCount-1; ++i)
	{
		const Vec3& left = (CoverUp * 0.015f) + (*leftIt++).position;
		const Vec3& right = (CoverUp * 0.015f) + (*rightIt++).position;

		dc->DrawLine(left, Col_White, right, Col_White, 7.5f);
	}

	Points::const_iterator pointIt = m_points.begin();
	for (uint32 i = 0; i < pointCount; ++i)
		dc->DrawSphere((CoverUp * 0.015f) + (*pointIt++).position, 0.035f, ColorB(Col_Black), true);
}