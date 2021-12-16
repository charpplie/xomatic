#ifndef __CoverSurface_h__
#define __CoverSurface_h__
#pragma once


#include "Cover.h"
#include "CoverPath.h"


class CoverSurface
{
	typedef ICoverSampler::Sample Sample;
public:
	CoverSurface();
	CoverSurface(const ICoverSystem::SurfaceInfo& surfaceInfo);

	struct Segment
	{
		Segment()
			: normal(ZERO)
			, leftIdx(-1)
			, rightIdx(-1)
		{
		}

		Segment(const Vec3& n, float len, uint16 left, uint16 right)
			: normal(n)
			, length(len)
			, leftIdx(left)
			, rightIdx(right)
		{
		}

		Vec3 normal;
		float length;

		uint16 leftIdx;
		uint16 rightIdx;
	};

	bool IsValid() const;
	void Clear();
	void Swap(CoverSurface& other);

	uint32 GetSampleCount() const;
	bool GetSurfaceInfo(ICoverSystem::SurfaceInfo* surfaceInfo) const;

	const AABB& GetAABB() const;

	void ReserveSamples(uint32 sampleCount);
	void AddSample(const Sample& sample);
	void Generate();

	bool IsPointInCover(const Vec3& eye, const Vec3& point) const;
	bool CalculatePathCoverage(const Vec3& eye, const CCoverPath& coverPath, SCoverageInterval* interval) const;

	bool GenerateCoverPath(float distanceToCover, CCoverPath* path, bool skipSimplify = false) const;

	void DebugDraw() const;
	
private:
	typedef std::list<Vec3> Points;
	void SimplifyCoverPath(Points& points) const;

	bool IntersectRayCoverPlane(const Ray& ray, const Plane& plane, float* t) const;
	void FindCoverPlanes(const Vec3& eye, Plane& left, Plane& right) const;

	typedef std::vector<Sample> Samples;
	Samples m_samples;

	typedef std::vector<Segment> Segments;
	Segments m_segments;

	AABB m_aabb;
};

#endif //__CoverSurface_h__