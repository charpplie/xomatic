/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Skeleton-aligned particle creator class (for 'Sonar Vision' and 'Cloak Awareness' perks)
-------------------------------------------------------------------------
History:
- 22:5:2009: Created by Tim Furnish

*************************************************************************/

#include "StdAfx.h"
#include "SkeletonAlignedParticleCreator.h"
#include "GameCVars.h"
#include "Player.h"

const Vec3* GetAbsJointPosByIdSafe(CPlayer& player, ISkeletonPose& pose, int ID)
{
	int boneID = player.GetBoneID(BONE_FOOT_L);
	if (boneID >= 0)
	{
		return &pose.GetAbsJointByID(boneID).t;
	}

	return NULL;
}

CSkeletonAlignedParticleCreator::SBoneCache CSkeletonAlignedParticleCreator::s_boneCache;

CSkeletonAlignedParticleCreator::CSkeletonAlignedParticleCreator()
{
}

void CSkeletonAlignedParticleCreator::Initialise(CPlayer* pPlayer)
{
	if((s_boneCache.isInitialised == false) && (pPlayer))
	{
		const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
		if(perkVars)
		{
			IEntity* pEntity = pPlayer->GetEntity();
			if(pEntity)
			{
				ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
				if(pCharacter)
				{
					ISkeletonPose *pPose = pCharacter->GetISkeletonPose();
					if(pPose)
					{
						const int numJoints = pPose->GetJointCount();
						float boneEmitterSpawnRadius = perkVars->perk_particleBoneEmitterSpawnRadius;

						CryFixedArray<const Vec3*,eMAX_BoneLocationCache-1> boneEmitterPos;
						boneEmitterPos.push_back(GetAbsJointPosByIdSafe(*pPlayer, *pPose, BONE_FOOT_L));
						boneEmitterPos.push_back(GetAbsJointPosByIdSafe(*pPlayer, *pPose, BONE_FOOT_R));
						boneEmitterPos.push_back(GetAbsJointPosByIdSafe(*pPlayer, *pPose, BONE_WEAPON));

						float boneChoice = cry_frand()*perkVars->perk_particleBoneChoiceScale;
						SBoneCache::SBoneCacheData boneData;

						for(int id = 0; id < numJoints; id++)
						{
							int parentId = pPose->GetParentIDByID(id);

							if(parentId>0)
							{
								QuatT boneQuat=pPose->GetAbsJointByID(id);
								QuatT parentBoneQuat=pPose->GetAbsJointByID(parentId);

								// Store bones which are close to the bone emitter
								for(int l=0; l<(eMAX_BoneLocationCache-1); l++)
								{
									if (boneEmitterPos[l])
									{
										float distFromEmitter = (boneQuat.t - *boneEmitterPos[l]).GetLength();
										float parentDistFromEmitter = (parentBoneQuat.t - *boneEmitterPos[l]).GetLength();

										if((distFromEmitter < boneEmitterSpawnRadius) && (parentDistFromEmitter < boneEmitterSpawnRadius))
										{
											boneData.id = id;
											boneData.distFromEmitter = 1.0f - distFromEmitter;
											boneData.distFromEmitterParentBone = 1.0f - parentDistFromEmitter;
											s_boneCache.boneArray[l].push_back(boneData);
										}
									}
								}

								// Now store some random body bones for the skeleton emitter
								boneChoice -= (parentBoneQuat.t-boneQuat.t).len();
								if(boneChoice <= 0.0f)
								{
									boneData.id = id;
									boneData.distFromEmitter = 0.0f;
									boneData.distFromEmitterParentBone = 0.0f;
									s_boneCache.boneArray[eBoneLocationCache_Skeleton].push_back(boneData);
									boneChoice = cry_frand()*perkVars->perk_particleBoneChoiceScale;
								}
							}
						}
					}
				}
			}
		}
		
		s_boneCache.isInitialised = true;
	}
}

void CSkeletonAlignedParticleCreator::GetBoneCacheArray(CryFixedArray<PodArray<SBoneCache::SBoneCacheData>*,SKELETON_MAX_EMITTER_CACHE>* boneCacheArray, int emitterId)
{
	switch(emitterId)
	{
		case eBoneEmitter_LeftFoot:
		{
			boneCacheArray->push_back(&s_boneCache.boneArray[eBoneLocationCache_LeftFoot]);
			break;
		}
		case eBoneEmitter_RightFoot:
		{
			boneCacheArray->push_back(&s_boneCache.boneArray[eBoneLocationCache_RightFoot]);
			break;
		}
		case eBoneEmitter_Feet:
		{
			boneCacheArray->push_back(&s_boneCache.boneArray[eBoneLocationCache_RightFoot]);
			boneCacheArray->push_back(&s_boneCache.boneArray[eBoneLocationCache_LeftFoot]);
			break;
		}
		case eBoneEmitter_Weapon:
		{
			boneCacheArray->push_back(&s_boneCache.boneArray[eBoneLocationCache_Weapon]);
			break;
		}
		case eBoneEmitter_Skeleton:
		{
			boneCacheArray->push_back(&s_boneCache.boneArray[eBoneLocationCache_Skeleton]);
			break;
		}
	}
}

void CSkeletonAlignedParticleCreator::GenerateParticlesOnSkeleton(const SSkeletonEmitterParams& params)
{
	if(params.pEmitter1 && params.pPlayer)
	{
		IEntity* pEntity = params.pPlayer->GetEntity();
		if(pEntity)
		{
			ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
			ISkeletonPose *pPose = pCharacter ? pCharacter->GetISkeletonPose() : NULL;
			if(pPose)
			{
				float boneEmitterSpawnRadius = params.boneEmitterSpawnRadius;
				float boneEmitterSpawnRadiusSq = boneEmitterSpawnRadius*boneEmitterSpawnRadius;

				// Set velocity
				Vec3 velocity(0.0f, 0.0f, 0.0f);
				if(params.useEntityVel)
				{
					IPhysicalEntity* pPhysEnt = pEntity->GetPhysics();
					pe_status_dynamics pDyn;
					if(pPhysEnt && pPhysEnt->GetStatus(&pDyn) != 0)
					{
						velocity = pDyn.v;
						velocity.z = 0.0f;
					}
				}

				// Calc bounding box centre
				AABB bbox;
				pEntity->GetLocalBounds(bbox);
				Vec3 boundingBoxCentre = bbox.GetCenter();

				PodArray<SBoneCache::SBoneCacheData>* boneCache;
				int numParticles = 0;

				// Loop through each emitter and spawn particles if needed
				for(int emitterId=0; emitterId<eMAX_BoneEmitters; emitterId++)
				{
					numParticles = params.boneEmitterNumParticles[emitterId];
					if(numParticles > 0)
					{
						CryFixedArray<PodArray<SBoneCache::SBoneCacheData>*,SKELETON_MAX_EMITTER_CACHE> boneCacheArray;

						GetBoneCacheArray(&boneCacheArray,emitterId);

						if(boneCacheArray.size() > 0)
						{
							for(int i=0; i<numParticles; i++)
							{
								const int numJoints = pPose->GetJointCount();
								boneCache = boneCacheArray[Random(boneCacheArray.size())];
								const int boneCacheSize = boneCache->size();
								if (boneCacheSize > 0)
								{
									int arrayIndex = Random(boneCacheSize);
									SBoneCache::SBoneCacheData* boneCacheData = &(*boneCache)[arrayIndex];
									int id = boneCacheData->id;
									int parentId = pPose->GetParentIDByID(id);

									if(parentId>0)
									{
										QuatT boneQuat = pPose->GetAbsJointByID(id);
										QuatT parentBoneQuat= pPose->GetAbsJointByID(parentId);
										Vec3 bonePos = pEntity->GetWorldTM().TransformPoint(boneQuat.t);
										Vec3 parentBonePos = pEntity->GetWorldTM().TransformPoint(parentBoneQuat.t);

										float lerpScale = cry_frand();

										QuatTS loc;
										loc.q.SetIdentity();
										loc.t = LERP(bonePos,parentBonePos,lerpScale);
										loc.s = 1.0f;

										if(params.randomOffsetScale > 0.0f)
										{
											Vec3 randOffset;
											randOffset.SetRandomDirection();
											randOffset *= params.randomOffsetScale;
											loc.t += randOffset;
										}

										float distScale = 1.0f;

										if(emitterId != eBoneEmitter_Skeleton)
										{
											distScale = LERP(boneCacheData->distFromEmitter,boneCacheData->distFromEmitterParentBone,lerpScale);
										}
										else
										{
											if(params.randomiseScaleForAllBoneEmitter)
											{
												distScale = Random(params.maxScaleForAllBoneEmitter);
											}
											else
											{
												distScale = params.maxScaleForAllBoneEmitter;
											}
										}

										Vec3 vel = loc.t - pEntity->GetWorldTM().TransformPoint(boundingBoxCentre);
										vel *= params.scaleVelocityMin + cry_frand() * (params.scaleVelocityMax - params.scaleVelocityMin);
										vel += velocity;
										vel.z += params.velocityUpMin + cry_frand() * (params.velocityUpMax - params.velocityUpMin);

										IParticleEmitter* emitter = params.pEmitter1;
										if((params.pEmitter2) && (Random(params.emitterSpawnRatio)==0))
										{
											emitter = params.pEmitter2;
										}

										SpawnParams spawnParams;
										spawnParams.fCountScale = 0.0f;
										spawnParams.fSizeScale = 1.0f;
										spawnParams.fStrength = params.scale * distScale;

										emitter->SetSpawnParams (spawnParams);
										emitter->EmitParticle(0, 0, &loc, &vel);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
