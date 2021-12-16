#include "StdAfx.h"
#include "CoverSurface.h"

#include "DebugDrawContext.h"


CoverSurface::CoverSurface()
{
}

CoverSurface::CoverSurface(const ICoverSystem::SurfaceInfo& surfaceInfo)
{
	m_samples.assign(surfaceInfo.samples, surfaceInfo.samples + surfaceInfo.sampleCount);
	
	Generate();
}

void CoverSurface::Swap(CoverSurface& other)
{
	m_samples.swap(other.m_samples);
	m_segments.swap(other.m_segments);
	
	std::swap(m_aabb, other.m_aabb);
}

bool CoverSurface::IsValid() const
{
	return m_samples.size() > 1;
}

void CoverSurface::Clear()
{
	m_samples.clear();
	m_segments.clear();
	m_aabb.Reset();
}

uint32 CoverSurface::GetSampleCount() const
{
	return m_samples.size();
}

bool CoverSurface::GetSurfaceInfo(ICoverSystem::SurfaceInfo* surfaceInfo) const
{
	if (surfaceInfo)
	{
		surfaceInfo->sampleCount = m_samples.size();
		if (surfaceInfo->sampleCount)
		{
			surfaceInfo->samples = &m_samples.front();
		}
	}

	return true;
}

const AABB& CoverSurface::GetAABB() const
{
	return m_aabb;
}

void CoverSurface::ReserveSamples(uint32 sampleCount)
{
	m_samples.reserve(sampleCount);
}

void CoverSurface::AddSample(const Sample& sample)
{
	m_samples.push_back(sample);
}

void CoverSurface::Generate()
{
	uint32 sampleCount = m_samples.size();

	if (sampleCount < 2)
		return;

	m_segments.clear();
	m_segments.reserve(sampleCount - 1);

	Vec3 mins(FLT_MAX, FLT_MAX, FLT_MAX);
	Vec3 maxs(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (uint32 i = 0; i < sampleCount-1; ++i)
	{
		const Sample& left = m_samples[i];
		const Sample& right = m_samples[i + 1];

		Vec3 rightToLeft = left.position - right.position;

		m_segments.push_back(
			Segment(CoverUp.Cross(rightToLeft).GetNormalized(), rightToLeft.GetLength(),	i, i + 1));

		mins.CheckMin(left.position);
		maxs.CheckMax(left.position);
		mins.CheckMin(right.position);
		maxs.CheckMax(right.position);
	}

	m_aabb = AABB(mins, maxs);
}

bool CoverSurface::IsPointInCover(const Vec3& eye, const Vec3& point) const
{
	uint32 segmentCount = m_segments.size();

	if (segmentCount < 1)
		return false;

	Vec3 eyeDir = (point - eye).GetNormalized();
	Ray ray(eye, eyeDir);
	Vec3 hit;

	for (uint32 i = 0; i < segmentCount; ++i)
	{
		const Segment& segment = m_segments[i];

		const Sample& left = m_samples[segment.leftIdx];
		const Sample& right = m_samples[segment.rightIdx];

		Vec3 leftTop = left.position + CoverUp * left.GetHeight();
		if (Intersect::Ray_Triangle(ray, right.position, leftTop, left.position, hit))
			return true;

		Vec3 rightTop = right.position + CoverUp * right.GetHeight();
		if (Intersect::Ray_Triangle(ray, right.position, rightTop, leftTop, hit))
			return true;
	}

	return false;
}

bool CoverSurface::CalculatePathCoverage(const Vec3& eye, const CCoverPath& coverPath, 
																					SCoverageInterval* interval) const
{
	const CCoverPath::Points& pathPoints = coverPath.GetPoints();

	const uint32 pathPointCount = pathPoints.size();
	const uint32 coverSampleCount = m_samples.size();
	
	if ((pathPointCount < 2) || (coverSampleCount < 2))
		return false;

	const float pathLength = pathPoints.back().distance;

	Plane leftPlane;
	Plane rightPlane;

	FindCoverPlanes(eye, leftPlane, rightPlane);

	uint32 leftIdx = UINT_MAX;
	float leftCoverage = FLT_MAX;

	uint32 rightIdx = UINT_MAX;
	float rightCoverage = -FLT_MAX;

	for (uint32 i = 0; i < pathPointCount; ++i)
	{
		const CCoverPath::Point& point = pathPoints[i];

		if (leftPlane.DistFromPlane(point.position) < 0.0f)
		{
			leftIdx = i;
			continue;
		}

		if (rightPlane.DistFromPlane(point.position) > 0.0f)
		{
			rightIdx = i;
			break;
		}
	}

	if (leftIdx < (pathPointCount - 1))
	{
		const CCoverPath::Point& leftPoint = pathPoints[leftIdx];
		const CCoverPath::Point& rightPoint = pathPoints[leftIdx + 1];

		Ray ray(leftPoint.position, (rightPoint.position - leftPoint.position).GetNormalized());

		float t;
		bool result = IntersectRayCoverPlane(ray, leftPlane, &t);
		assert(result);

		leftCoverage = leftPoint.distance + t;
	}
	else if (leftIdx == UINT_MAX)
		leftCoverage = 0.0f;

	if ((rightIdx > 0) && (rightIdx < pathPointCount))
	{
		const CCoverPath::Point& leftPoint = pathPoints[rightIdx - 1];
		const CCoverPath::Point& rightPoint = pathPoints[rightIdx];

		Ray ray(leftPoint.position, (rightPoint.position - leftPoint.position).GetNormalized());

		float t;
		bool result = IntersectRayCoverPlane(ray, rightPlane, &t);
		assert(result);

		rightCoverage = leftPoint.distance + t;
	}
	else if (rightIdx > 0)
		rightCoverage = pathLength;

	SCoverageInterval result(leftCoverage, rightCoverage);
	if (result.IsEmpty())
		return false;

	*interval = result;

	return true;
}

bool CoverSurface::GenerateCoverPath(float distanceToCover, CCoverPath* path, bool skipSimplify) const
{
	uint32 segmentCount = m_segments.size();
	
	if (segmentCount < 1)
		return false;

	if (segmentCount == 1)
	{
		const Segment& segment = m_segments[0];

		Vec3 extrusion = segment.normal * distanceToCover;

		Vec3 left = m_samples[segment.leftIdx].position + extrusion;
		Vec3 right = m_samples[segment.rightIdx].position + extrusion;

		path->AddPoint(left);
		path->AddPoint(right);

		return true;
	}

	Points points;

	const float extraPointMinAngleCos = cry_cosf(DEG2RAD(25.0f));

	const Segment& firstSegment = m_segments[0];
	Vec3 extrusionLeft = firstSegment.normal * distanceToCover;
	points.push_back(m_samples[firstSegment.leftIdx].position + extrusionLeft);
	
	for (uint32 i = 1; i < segmentCount; ++i)
	{
		const Segment& prevSegment = m_segments[i - 1];
		const Segment& segment = m_segments[i];

		if (prevSegment.normal.Cross(segment.normal).z > 0.0f)
		{
			Vec3 prevExtrusion = prevSegment.normal * distanceToCover;
			points.push_back(m_samples[prevSegment.rightIdx].position + prevExtrusion);

			Vec3 currentExtrusion = segment.normal * distanceToCover;

			float angleCos = prevSegment.normal.Dot(segment.normal);
			if ((angleCos <= extraPointMinAngleCos) && ((prevExtrusion - currentExtrusion).len2() > sqr(0.35f)))
			{
				Vec3 normalAvg = ((prevSegment.normal + segment.normal) * 0.5f).GetNormalized();
				points.push_back(m_samples[segment.leftIdx].position + (normalAvg * distanceToCover));
			}
		
			points.push_back(m_samples[segment.leftIdx].position + currentExtrusion);
		}
		else if (false)
		{
			Vec3 normalAvg = ((prevSegment.normal + segment.normal) * 0.5f).GetNormalized();
			points.push_back(m_samples[segment.leftIdx].position + (normalAvg * distanceToCover));
		}
	}

	const Segment& lastSegment = m_segments[segmentCount - 1];
	Vec3 extrusionRight = lastSegment.normal * distanceToCover;
	points.push_back(m_samples[lastSegment.rightIdx].position + extrusionRight);

	static bool testIt = true;
	static bool notTestIt = true;
	if ((!skipSimplify && !notTestIt) || testIt)
		SimplifyCoverPath(points);

	Points::const_iterator it = points.begin();
	Points::const_iterator itEnd = points.end();

	path->Reserve(points.size());

	for ( ; it != itEnd; ++it)
		path->AddPoint(*it);

	return true;
}

void CoverSurface::DebugDraw() const
{
	uint32 coverSampleCount = m_samples.size();
	uint32 segmentCount = m_segments.size();
	
	if (coverSampleCount < 2)
		return;

	std::vector<Vec3>	coverSurface;
	coverSurface.resize(2 * coverSampleCount * 12);

	const float offset = -0.0175f;
	const float floorOffset = 0.015f;

	// Low Cover
	uint32 k = 0;
	for (uint32 i = 0; i < segmentCount; ++i)
	{
		const Segment& segment = m_segments[i];
		const Sample& left = m_samples[segment.leftIdx];
		const Sample& right = m_samples[segment.rightIdx];

		Vec3 leftNormal = (i > 0) ? 
			(0.5f * (segment.normal + m_segments[i - 1].normal)) : segment.normal;
		Vec3 rightNormal = (i < segmentCount - 1) ? 
			(0.5f * (segment.normal + m_segments[i + 1].normal)) : segment.normal;
		
		Vec3 bottomLeft(left.position - (leftNormal * offset));
		Vec3 bottomRight(right.position - (rightNormal * offset));

		coverSurface[k * 12 + 0] = bottomLeft;
		coverSurface[k * 12 + 1] = bottomRight + Vec3(0.0f, 0.0f, min(right.GetHeight(), LowCoverMaxHeight));
		coverSurface[k * 12 + 2] = bottomRight;
		coverSurface[k * 12 + 3] = bottomLeft;
		coverSurface[k * 12 + 4] = bottomLeft + Vec3(0.0f, 0.0f, min(left.GetHeight(), LowCoverMaxHeight));
		coverSurface[k * 12 + 5] = bottomRight + Vec3(0.0f, 0.0f, min(right.GetHeight(), LowCoverMaxHeight));

		coverSurface[k * 12 + 6 + 0] = bottomLeft;
		coverSurface[k * 12 + 6 + 2] = bottomRight + Vec3(0.0f, 0.0f, min(right.GetHeight(), LowCoverMaxHeight));
		coverSurface[k * 12 + 6 + 1] = bottomRight;
		coverSurface[k * 12 + 6 + 3] = bottomLeft;
		coverSurface[k * 12 + 6 + 5] = bottomLeft + Vec3(0.0f, 0.0f, min(left.GetHeight(), LowCoverMaxHeight));
		coverSurface[k * 12 + 6 + 4] = bottomRight + Vec3(0.0f, 0.0f, min(right.GetHeight(), LowCoverMaxHeight));

		++k;
	}

	ColorB color(Col_DarkGreen);
	color.a = 128;

	CDebugDrawContext dc;
	dc->DrawTriangles(&coverSurface[0], coverSampleCount * 12, color);

	// High Cover
	k = coverSampleCount;
	for (uint32 i = 0; i < segmentCount; ++i)
	{
		const Segment& segment = m_segments[i];
		const Sample& left = m_samples[segment.leftIdx];
		const Sample& right = m_samples[segment.rightIdx];

		Vec3 leftNormal = (i > 0) ? 
			(0.5f * (segment.normal + m_segments[i - 1].normal)) : segment.normal;
		Vec3 rightNormal = (i < segmentCount - 1) ? 
			(0.5f * (segment.normal + m_segments[i + 1].normal)) : segment.normal;

		if ((left.GetHeight() < LowCoverMaxHeight) && (right.GetHeight() < LowCoverMaxHeight))
			continue;

		Vec3 bottomLeft(left.position - (leftNormal * offset) + Vec3(0.0f, 0.0f, min(left.GetHeight(), LowCoverMaxHeight)));
		Vec3 bottomRight(right.position - (leftNormal * offset) + Vec3(0.0f, 0.0f, min(right.GetHeight(), LowCoverMaxHeight)));

		float leftHeight = max(0.0f, (left.GetHeight() - min(left.GetHeight(), LowCoverMaxHeight)));
		float rightHeight = max(0.0f, (right.GetHeight() - min(right.GetHeight(), LowCoverMaxHeight)));

		coverSurface[k * 12 + 0] = bottomLeft;
		coverSurface[k * 12 + 1] = bottomRight + Vec3(0.0f, 0.0f, rightHeight);
		coverSurface[k * 12 + 2] = bottomRight;
		coverSurface[k * 12 + 3] = bottomLeft;
		coverSurface[k * 12 + 4] = bottomLeft + Vec3(0.0f, 0.0f, leftHeight);
		coverSurface[k * 12 + 5] = bottomRight + Vec3(0.0f, 0.0f, rightHeight);

		coverSurface[k * 12 + 6 + 0] = bottomLeft;
		coverSurface[k * 12 + 6 + 2] = bottomRight + Vec3(0.0f, 0.0f, rightHeight);
		coverSurface[k * 12 + 6 + 1] = bottomRight;
		coverSurface[k * 12 + 6 + 3] = bottomLeft;
		coverSurface[k * 12 + 6 + 5] = bottomLeft + Vec3(0.0f, 0.0f, leftHeight);
		coverSurface[k * 12 + 6 + 4] = bottomRight + Vec3(0.0f, 0.0f, rightHeight);

		++k;
	}

	if (k > coverSampleCount)
	{
		ColorB color(Col_MediumSeaGreen);
		color.a = 128;

		dc->DrawTriangles(&coverSurface[coverSampleCount * 12], (k-coverSampleCount) * 12, color);
	}

	// Segments
	for (uint32 i = 0; i < segmentCount; ++i)
	{
		const Segment& segment = m_segments[i];
	
		Vec3 leftNormal = (i > 0) ? 
			(0.5f * (segment.normal + m_segments[i - 1].normal)) : segment.normal;
		Vec3 rightNormal = (i < segmentCount - 1) ? 
			(0.5f * (segment.normal + m_segments[i + 1].normal)) : segment.normal;

		const Vec3& left = m_samples[segment.leftIdx].position - (leftNormal * offset) + (CoverUp * floorOffset);
		const Vec3& right = m_samples[segment.rightIdx].position - (rightNormal * offset) + (CoverUp * floorOffset);

		Vec3 segmentCenter = (left + right) * 0.5f;

		dc->DrawLine(left, Col_DarkWood, right, Col_DarkWood, 7.5f);
		dc->DrawLine(segmentCenter, Col_Goldenrod, segmentCenter + segment.normal * 0.15f, Col_Goldenrod);
		dc->DrawCone(segmentCenter + segment.normal * 0.15f, segment.normal, 0.02f, 0.05f, Col_Goldenrod);
	}

	// Samples
	{
		for (uint32 i = 0; i < segmentCount; ++i)
		{
			const Segment& segment = m_segments[i];

			Vec3 leftNormal = (i > 0) ?
				(0.5f * (segment.normal + m_segments[i - 1].normal)) : segment.normal;

			dc->DrawSphere(m_samples[segment.leftIdx].position - (leftNormal * offset) + (CoverUp * floorOffset),
			0.02f, ColorB(Col_FireBrick), true);
		}

		// Last Sample
		const Segment& segment = m_segments[segmentCount - 1];
		Vec3 rightNormal = segment.normal;
		dc->DrawSphere(m_samples[segment.rightIdx].position - (rightNormal * offset) + (CoverUp * floorOffset),
			0.02f, ColorB(Col_FireBrick), true);
	}
}

void CoverSurface::SimplifyCoverPath(Points& points) const
{
	uint32 pointCount = points.size();

	if (pointCount < 3)
		return;

	Points::iterator rightIt = points.begin();
	Points::iterator leftIt = rightIt++;
	Points::iterator midIt = rightIt++;

	float minAngleCos = cry_cosf(gf_PI / 18.0f);

	while (rightIt != points.end())
	{
		float dot = 0.0f;;
		const Vec3& left = *leftIt;
		const Vec3& mid = *midIt;
		const Vec3& right = *rightIt;

		Vec3 leftToMid = mid - left;
		Vec3 midToRight = right - mid;

		float leftToMidLenSq = leftToMid.len2();
		float midToRightLenSq = midToRight.len2();

		if ((leftToMidLenSq > sqr(2.0f)) || (midToRightLenSq > sqr(2.0f)))
			goto SkipToNext;

		if (leftToMidLenSq <= sqr(0.1f))
			goto PerformErase;

		leftToMid.Normalize();
		midToRight.Normalize();

		dot = leftToMid.Dot(midToRight);

		if ((dot >= minAngleCos) || (dot <= 0.0f))
		{
PerformErase:
			Points::iterator erased = midIt;
			midIt = rightIt++;
			points.erase(erased);

			continue;
		}

SkipToNext:
		++leftIt;
		++midIt;
		++rightIt;
	}
}

bool CoverSurface::IntersectRayCoverPlane(const Ray& ray, const Plane& plane, float* t) const
{
	float cosine = plane.n | ray.direction;
	if (cosine <= 0.0f)
		return false;					

	float numerator = plane.DistFromPlane(ray.origin);
	float length = -numerator / cosine;

	if (length < 0.0f)
		return false;

	if (t)
		*t = length;

	return true;
}

void CoverSurface::FindCoverPlanes(const Vec3& eye, Plane& left, Plane& right) const
{
	const uint32 coverSampleCount = m_samples.size();

	if (coverSampleCount < 2)
		return;

	Vec3 leftToEye = eye - m_samples.front().position;
	Vec3 leftNormal = leftToEye.Cross(CoverUp).GetNormalized();
	left.SetPlane(leftNormal, eye);

	Vec3 rightToEye = eye - m_samples.back().position;
	Vec3 rightNormal = rightToEye.Cross(CoverUp).GetNormalized();
	right.SetPlane(rightNormal, eye);

	for (uint32 i = 1; i < coverSampleCount - 1; ++i)
	{
		const Sample& sample = m_samples[i];
		
		if (left.DistFromPlane(sample.position) < 0.0f)
		{
			leftToEye = eye - sample.position;
			leftNormal = leftToEye.Cross(CoverUp).GetNormalized();
			left.SetPlane(leftNormal, eye);
		}
		else if (right.DistFromPlane(sample.position) > 0.0f)
		{
			rightToEye = eye - sample.position;
			rightNormal = rightToEye.Cross(CoverUp).GetNormalized();
			right.SetPlane(rightNormal, eye);
		}
	}
}
