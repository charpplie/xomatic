#ifndef __CoverPath_h__
#define __CoverPath_h__
#pragma once


#include "Cover.h"


class CCoverPath
{
public:

	typedef struct Point
	{
		Point()
			: position(ZERO)
			, distance(0.0f)
		{
		}
		Point(const Vec3& pos, float dist)
			: position(pos)
			, distance(dist)
		{
		}

		Vec3 position;
		float distance;
	};

	typedef std::vector<Point> Points;
	const Points& GetPoints() const;

	void Clear();
	void Reserve(uint32 pointCount);
	void AddPoint(const Vec3& point);

	Vec3 GetPointAt(float distance) const;
	Vec3 GetClosestPoint(const Vec3& point, float* distanceToPath, float* distanceOnPath = 0) const;
	float GetDistanceAt(const Vec3& point, float tolerance = 0.01f) const;

	bool Intersect(const Vec3& origin, const Vec3& dir, float* distance, Vec3* point) const;

	void DebugDraw();

private:
	Points m_points;
};


#endif //__CoverPath_h__