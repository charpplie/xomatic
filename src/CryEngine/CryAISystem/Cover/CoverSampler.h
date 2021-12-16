#ifndef __CoverSampler_h__
#define __CoverSampler_h__
#pragma once


#include "Cover.h"
#include "CoverSurface.h"
#include "CoverPath.h"


class CoverSampler
	: public ICoverSampler
{
public:
	CoverSampler();
	virtual void Release();

	virtual ESamplerState StartSampling(const ICoverSampler::Params& params);
	virtual ESamplerState Update(float timeLimit);
	virtual ESamplerState GetState() const;

	virtual uint32 GetSampleCount() const;
	virtual const ICoverSampler::Sample* GetSamples() const;
	virtual void DebugDraw() const;

private:
	virtual ~CoverSampler();

	struct IntersectionResult
	{
		Vec3 position;
		Vec3 normal;

		float distance;
	};
	
	enum EInternalState
	{
		SamplerReady = 0,
		SamplerStarting,
		SamplerLeft,
		SamplerLeftEdge,
		SamplerRight,
		SamplerRightEdge,
		SamplerSimplifySurface,
		SamplerFinished,
		SamplerError,
	};

	enum EDirection
	{
		Left = 0,
		Right,
	};

	bool OverlapCylinder(const Vec3& center, const Vec3& dir, float radius, uint32 collisionFlags) const;
	bool IntersectSweptSphere(const Vec3& center, const Vec3& dir, float radius, uint32 collisionFlags,
		IntersectionResult* result) const;

	float SampleHeight(const Vec3& position, float radius, float heightInterval, float maxHeight, float* depth);
	bool SampleFloor(const Vec3& position, float searchHeight, float searchRadius,
		Vec3* floor, Vec3* normal);
	bool SampleCover(EDirection direction, float widthInterval, float gapTolerance, float maxHeight);
	void Simplify();
	
	ESamplerState m_externalState;
	Params m_params;

	typedef std::list<ICoverSampler::Sample> TempSamples;
	TempSamples m_tempSamples;

	typedef std::vector<ICoverSampler::Sample> Samples;
	Samples m_samples;

	struct SamplingState
	{
		SamplingState()
			: origin(ZERO)
			, right(ZERO)
			, direction(ZERO)
			, internalState(SamplerReady)
			, currOffset(0.0f)
			, edgeHeight(0.0f)
			, floorHeight(0.0f)
			, lastSurfaceOffset(0.0f)
			, lastSurfaceHeight(0.0f)
			, totalTime(0.0f)
			, originalSurfaceSamples(0)
			, updateCount(0)
			, pwiCount(0)
		{
		}

		Vec3 origin;
		Vec3 right;
		Vec3 direction;

		EInternalState internalState;

		float currOffset;
		float edgeHeight;
		float floorHeight;

		float lastSurfaceOffset;
		float lastSurfaceHeight;

		CTimeValue totalTime;
		uint32 originalSurfaceSamples;
		uint32 updateCount;
		mutable uint32 pwiCount;
	};

	SamplingState m_state;
};

#endif //__CoverSampler_h__