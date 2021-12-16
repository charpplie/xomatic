////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   ICryAnimation2.h
//  Version:     v1.00
//  Created:     30/7/2009 by Ivo Frey
//  Compilers:   Visual Studio.NET
//  Description: CryAnimation interfaces
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include DEVIRTUALIZE_HEADER_FIX(ICryAnimation2.h)

#include <CryExtension/ICryUnknown.h>

#ifndef ICryAnimation2_h
#define ICryAnimation2_h

struct IAnimationLayer;
struct IAnimationPoseModifier;

//

struct IAnimationLayer :
	public ICryUnknown
{
};

struct SAnimationPoseModiferParams
{
	ICharacterInstance* pCharacterInstance;

	f32 timeDelta;

	QuatT locationNextPhysics;
	QuatTS locationNextAnimation;

	QuatT* pPoseRelative;
	QuatT* pPoseAbsolute;
	uint32 jointCount;

	ILINE ISkeletonPose* GetISkeletonPose() const { return pCharacterInstance->GetISkeletonPose(); }
	ILINE ISkeletonAnim* GetISkeletonAnim() const  { return pCharacterInstance->GetISkeletonAnim(); }

	ILINE ICharacterModel* GetICharacterModel() const { return pCharacterInstance->GetICharacterModel(); }
	ILINE ICharacterModelSkeleton* GetICharacterModelSkeleton() const { return pCharacterInstance->GetICharacterModel()->GetICharacterModelSkeleton(); }
};

struct IAnimationPoseModifier :
	public IAnimationLayer
{
	CRYINTERFACE_DECLARE(IAnimationPoseModifier, 0x22fe47755e42447f, 0xbab6274ed39af449)

	virtual bool Execute(const SAnimationPoseModiferParams& params) = 0;
	virtual void GetMemoryUsage(ICrySizer *pSizer) const = 0;
};

typedef cryshared_ptr<IAnimationPoseModifier> IAnimationPoseModifierPtr;

UNIQUE_IFACE struct IAnimationPoseBlenderAim :
	public IAnimationPoseModifier
{
	CRYINTERFACE_DECLARE(IAnimationPoseBlenderAim, 0x1725a49dbd684ff4, 0x852cd0d4b7f86c28)

	enum EArm
	{
		eArm_Left = 1<<0,
		eArm_Right = 1<<1,
	};

	virtual void SetTarget(const Vec3& target) = 0;
	virtual void SetArms(uint32 arms) = 0;
	virtual void SetBlendTime(f32 time) = 0;
//	virtual f32 GetBlendTime() = 0;
	virtual void SetFadeOut(uint32 a) = 0;
	virtual void SetFadeOutSpeed(f32 time) = 0;
//	virtual void SetFadeInSpeed(f32 time) = 0;
	virtual void SetTargetSmoothTime(f32 time) = 0;
	virtual void SetPolarCoordinates(const Vec2& pc) = 0;
//	virtual uint32 GetStatus() = 0;
	virtual f32 GetBlend() = 0;
	
	virtual void GetMemoryUsage(ICrySizer *pSizer) const = 0;
};

typedef cryshared_ptr<IAnimationPoseBlenderAim> IAnimationPoseBlenderAimPtr;

#endif // ICryAnimation2_h
