/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$

20:11:2009 - Benito G.R.
*************************************************************************/

#pragma once

#ifndef __AUTOAIM_MANAGER_H__
#define __AUTOAIM_MANAGER_H__

typedef struct SAutoaimTargetRegisterParams
{
	SAutoaimTargetRegisterParams()
		: fallbackOffset(0.0f, 0.0f, 1.2f)
	  , primaryBoneId(-1)
		, physicsBoneId(-1)
		, secondaryBoneId(-1)
		, innerRadius(0.4f)
		, outerRadius(0.8f)
		, snapRadius(2.0f)
		, secondaryRadius(0.2f)
	{

	}

	Vec3 fallbackOffset;
	float			innerRadius;
	float			outerRadius;
	float			snapRadius;
	float			secondaryRadius;
	int16 primaryBoneId;
	int16	physicsBoneId;
	int16 secondaryBoneId;

} SAutoaimTargetRegisterParams;

enum EAutoaimTargetFlags
{
	eAATF_None = 0,
	eAATF_AIHostile = BIT(1),
	eAATF_StealthKillable = BIT(2),
	eAATF_CanBeGrabbed = BIT(3),
};

typedef struct SAutoaimTarget
{
	SAutoaimTarget()
		: teamNum(-1)
		, entityId(0)
		, innerRadius(0.4f)
		, outerRadius(0.8f)
		, snapRadius(2.0f)
		, secondaryRadius(0.2f)
		, primaryBoneId(-1)
		, physicsBoneId(-1)
		, secondaryBoneId(-1)
		, flags(eAATF_None)
	{
		primaryAimPosition.zero();
		secondaryAimPosition.zero();
		fallbackOffset.zero();
	}

	ILINE bool HasFlagSet(EAutoaimTargetFlags _flag) const { return ((flags & _flag) != 0); }
	ILINE bool HasFlagsSet(int8 _flags) const { return ((flags & _flags) == _flags); } 
	ILINE void SetFlag(EAutoaimTargetFlags _flag) { flags |= _flag; }
	ILINE void RemoveFlag(EAutoaimTargetFlags _flag) { flags &= ~_flag; }

	Vec3			primaryAimPosition;
	Vec3			secondaryAimPosition;
	Vec3			fallbackOffset;
	EntityId	entityId;
	float			innerRadius;
	float			outerRadius;
	float			snapRadius;
	float			secondaryRadius;
	int16			primaryBoneId;
	int16			physicsBoneId;
	int16			secondaryBoneId;
	int8			flags;
	int8			teamNum;

} SAutoaimTarget;

typedef std::vector<SAutoaimTarget> TAutoaimTargets;

class CAutoAimManager
{
public:

	CAutoAimManager();
	~CAutoAimManager();

	bool RegisterAutoaimTarget(EntityId entityId, const SAutoaimTargetRegisterParams& registerParams);
	void UnregisterAutoaimTarget(EntityId entityId);

	void Update(float dt);

	ILINE void SetExternalSnapTarget(EntityId snapTargetId) { m_externalSnapTarget = snapTargetId; }
	ILINE EntityId GetExternalSnapTarget() const { return m_externalSnapTarget; }
	ILINE const TAutoaimTargets& GetAutoAimTargets() const { return m_autoaimTargets; };

	const SAutoaimTarget* GetTargetInfo(EntityId targetId) const;

private:

	static const int32 kMaxAutoaimTargets = 128;

	void RegisterCharacterTargetInfo(EntityId entityId, const SAutoaimTargetRegisterParams& registerParams);
	void UpdateCharacterTargetInfo(SAutoaimTarget& aaTarget);

	int		GetIndexFromEntity(EntityId entityId) const;
	bool	IsEntityRegistered(EntityId entityId) const;
	bool	IsSpaceAvailable() const;
	
	void  DebugDraw();
	void  DrawDisc(const Vec3& center, Vec3 axis, float innerRadius, float outerRadius, const ColorB& innerColor, const ColorB& outerColor);

	TAutoaimTargets	m_autoaimTargets;

	EntityId	m_externalSnapTarget;

	int				g_autoAimManagerDebug;
};

#endif //__AUTOAIM_MANAGER_H__