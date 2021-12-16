#ifndef __ICoverSystem_h__
#define __ICoverSystem_h__

#pragma once


typedef uint32 CoverSurfaceID; // only 24bits will be used - static level surfaceIDs are always < 2^16
const uint32 CoverSurfaceIDDynamicMask = 0xfff00000;


struct ICoverSampler
{
	enum ESamplerState
	{
		None = 0,
		InProgress,
		Finished,
		Error,
	};

	struct Sample
	{
		static const uint8 FractionBitCount = 12;

		enum ESampleFlags
		{
			Edge = 1 << 0,
		};

		Sample()
		{
		}

		Sample(const Vec3& _position, float _height, uint16 _flags = 0)
			: position(_position)
			, height((uint16)(_height * (1 << FractionBitCount)))
			, flags(_flags)
		{
		}

		Vec3 position;

		uint16 height;	// unsigned fixed point 4:12
		uint16 flags;

		inline void SetHeight(float _height)
		{
			height = (uint16)(_height * (1 << FractionBitCount));
		}

		inline float GetHeight() const
		{
			return height * (1.0f / (float)(1 << FractionBitCount));
		}
	};

	struct Params
	{
		Params()
			: position(ZERO)
			, direction(ZERO)
			, limitDepth(1.85f)
			, limitHeight(1.25f)
			, limitLeft(10.0f)
			, limitRight(10.0f)
			, heightSamplerInterval(0.2f)
			, heightGapTolerance(0.2f)
			, widthSamplerInterval(0.25f)
			, widthGapTolerance(0.25f)
			, floorSearchHeight(2.0f)
			, floorSearchRadius(0.45f)
			, minHeight(0.75f)
			, minClearance(0.4f)
			, maxClearance(0.8f)
			, simplifyMaxDistance(2.0f)
			, simplifyMinAngle(gf_PI / 18.0f) // 10º
			, refineEdges(true)
			, collisionFlags(AICE_ALL)
			, referenceLocation(IDENTITY)
		{
		}

		Vec3 position;
		Vec3 direction;

		float limitDepth;
		float limitHeight;
		float limitLeft;
		float limitRight;
		//float maxWidth;

		float heightSamplerInterval;
		float heightGapTolerance;

		float widthSamplerInterval;
		float widthGapTolerance;

		float floorSearchHeight;
		float floorSearchRadius;

		float minHeight;

		float minClearance;
		float maxClearance;

		float simplifyMaxDistance;
		float simplifyMinAngle;

		bool refineEdges;

		uint32 collisionFlags;
		QuatT referenceLocation;
	};

	virtual void Release() = 0;
	virtual ESamplerState StartSampling(const Params& params) = 0;
	virtual ESamplerState Update(float maxMilliseconds = 0.1f) = 0;
	virtual ESamplerState GetState() const = 0;

	virtual uint32 GetSampleCount() const = 0;
	virtual const struct Sample* GetSamples() const = 0;
	virtual void DebugDraw() const = 0;
};


struct ICoverSystem
{
	struct SurfaceInfo
	{
		SurfaceInfo()
			: samples(0)
			, sampleCount(0)
		{
		}

		const ICoverSampler::Sample* samples;
		uint32 sampleCount;
	};

	virtual ICoverSampler* CreateCoverSampler(const char* samplerName = "default") = 0;

	virtual void Clear() = 0;
	virtual bool ReadSurfacesFromFile(const char* fileName) = 0;

	virtual CoverSurfaceID AddSurface(const SurfaceInfo& surfaceInfo) = 0;
	virtual void RemoveSurface(const CoverSurfaceID& surfaceID) = 0;
	virtual void UpdateSurface(const CoverSurfaceID& surfaceID, const SurfaceInfo& surfaceInfo) = 0;
	
	virtual uint32 GetSurfaceCount() const = 0;
	virtual bool GetSurfaceInfo(const CoverSurfaceID& surfaceID, SurfaceInfo* surfaceInfo) const = 0;

	virtual void DrawSurface(const CoverSurfaceID& surfaceID) = 0;
};



#endif // __ICoverSystem_h__