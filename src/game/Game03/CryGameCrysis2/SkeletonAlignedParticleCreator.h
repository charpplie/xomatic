/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Skeleton-aligned particle creator class (for 'Sonar Vision' and 'Cloak Awareness' perks)
-------------------------------------------------------------------------
History:
- 22:5:2009: Created by Tim Furnish
- 12:1:2009: Developed by James Chilvers

*************************************************************************/

#ifndef __SKELETON_ALIGNED_PARTICLE_CREATOR_H__
#define __SKELETON_ALIGNED_PARTICLE_CREATOR_H__

#include "Utility/CryFixedArray.h"

#define SKELETON_MAX_EMITTER_CACHE	2

struct IParticleEmitter;
class CPlayer;

enum EBoneEmitterLocation
{
	eBoneEmitter_Skeleton = 0,
	eBoneEmitter_LeftFoot,
	eBoneEmitter_RightFoot,
	eBoneEmitter_Feet,
	eBoneEmitter_Weapon,
	eMAX_BoneEmitters
};

struct SSkeletonEmitterParams
{
	SSkeletonEmitterParams()
	{
		pEmitter1 = NULL; 
		pEmitter2 = NULL;
		pPlayer = NULL;
		scaleVelocityMin = 0.0f; 
		scaleVelocityMax = 0.0f; 
		velocityUpMin = 0.0f; 
		velocityUpMax = 0.0f; 
		boneEmitterSpawnRadius = 0.0f;
		scale  = 1.0f;
		maxScaleForAllBoneEmitter = 1.0f;
		randomOffsetScale = 0.0f;
		emitterSpawnRatio = 0;
		randomiseScaleForAllBoneEmitter = false;

		useEntityVel = false;
		for(int i=0; i<boneEmitterNumParticles.max_size(); i++)
		{
			boneEmitterNumParticles.push_back(0);
		}
	}

	CryFixedArray<int,eMAX_BoneEmitters> boneEmitterNumParticles; 
	IParticleEmitter* pEmitter1; 
	IParticleEmitter* pEmitter2;  
	CPlayer* pPlayer;
	float scaleVelocityMin; 
	float scaleVelocityMax; 
	float velocityUpMin; 
	float velocityUpMax; 
	float boneEmitterSpawnRadius;
	float scale;
	float maxScaleForAllBoneEmitter;
	float randomOffsetScale;
	int emitterSpawnRatio;
	bool randomiseScaleForAllBoneEmitter;
	bool useEntityVel;
};

class CSkeletonAlignedParticleCreator
{
public:
	CSkeletonAlignedParticleCreator ();
	void Initialise(CPlayer* pPlayer);
	void GenerateParticlesOnSkeleton(const SSkeletonEmitterParams& skeletonEmitterParams);

private:
	
	enum EBoneLocationCache
	{
		eBoneLocationCache_LeftFoot,
		eBoneLocationCache_RightFoot,
		eBoneLocationCache_Weapon,
		eBoneLocationCache_Skeleton,
		eMAX_BoneLocationCache
	};

	struct SBoneCache
	{
		SBoneCache()
		{
			isInitialised = false;
		}

		struct SBoneCacheData
		{
			SBoneCacheData()
			{
				id = 0;
				distFromEmitter = 0.0f;
				distFromEmitterParentBone = 0.0f;
			}

			int id;
			float distFromEmitter;
			float distFromEmitterParentBone;
		};

		PodArray<SBoneCacheData>	boneArray[eMAX_BoneLocationCache];
		bool isInitialised;
	};

	void GetBoneCacheArray(CryFixedArray<PodArray<SBoneCache::SBoneCacheData>*,SKELETON_MAX_EMITTER_CACHE>* boneCacheArray, int emitterId);

	static SBoneCache s_boneCache;

};

#endif // __SKELETON_ALIGNED_PARTICLE_CREATOR_H__
