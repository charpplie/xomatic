#ifndef __Cover_h__
#define __Cover_h__


#pragma once


#include <ICoverSystem.h>


typedef enum ECoverUsageType
{
	eCU_Hide = 0,
	eCU_UnHide,
};


const float LowCoverMinHeight = 0.75f;
const float LowCoverMaxHeight = 1.1f;
const float HighCoverMaxHeight = 2.0f;
const float CoverOffsetUp = 0.075f;

const Vec3 CoverUp = Vec3(0.0f, 0.0f, 1.0f);


struct SCoverageInterval
{
	SCoverageInterval()
		: left(0.0f)
		, right(0.0f)
	{
	}

	SCoverageInterval(float l, float r)
		: left(l)
		, right(r)
	{
	}

	float left;
	float right;

	inline bool IsEmpty() const
	{
		return (right - left) <= 0.001f;
	}

	inline float Width() const
	{
		return right - left;
	}

	inline bool Intersect(const SCoverageInterval& other)
	{
		left = max(left, other.left);
		right = min(right, other.right);

		return !IsEmpty();
	}

	inline SCoverageInterval Intersection(const SCoverageInterval& other) const
	{
		return SCoverageInterval(max(left, other.left), min(right, other.right));
	}
};


#endif