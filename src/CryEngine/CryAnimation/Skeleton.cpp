#include "stdafx.h"
#include "Skeleton.h"

#include "ModelSkeleton.h"

namespace Skeleton {

/*
CPoseData
*/

bool CPoseData::Initialize(const CModelSkeleton& skeleton)
{
	uint32 jointCount = skeleton.GetJointCount();
	m_jointsRelative.resize(jointCount);
	m_jointsAbsolute.resize(jointCount);
	m_jointsStatus.resize(jointCount);

	assert(m_jointsRelative.capacity() == jointCount);
	assert(m_jointsAbsolute.capacity() == jointCount);
	assert(m_jointsStatus.capacity() == jointCount);

	for (uint32 i=0; i<jointCount; ++i)
	{
		m_jointsRelative[i] = skeleton.m_poseData.m_jointsRelative[i];
		m_jointsAbsolute[i] = skeleton.m_poseData.m_jointsAbsolute[i];
	}

	return true;
}

void CPoseData::Initialize(const CPoseData& poseData)
{
	uint32 jointCount = poseData.GetJointCount();
	m_jointsRelative.resize(jointCount);
	m_jointsAbsolute.resize(jointCount);
	m_jointsStatus.resize(jointCount);

	::memcpy(&m_jointsRelative[0], &poseData.m_jointsRelative[0], jointCount * sizeof(QuatT));
	::memcpy(&m_jointsAbsolute[0], &poseData.m_jointsAbsolute[0], jointCount * sizeof(QuatT));
	if (!poseData.m_jointsStatus.empty())
		::memcpy(&m_jointsStatus[0], &poseData.m_jointsStatus[0], jointCount * sizeof(Status4));
}

/*
CLocator
*/

CLocator::CLocator()
{
	Initialize();
}

CLocator::~CLocator()
{
}

//

bool CLocator::Initialize()
{
	m_speed = 0.0f;
	m_turn = 0.0f;
	m_slope = 0.0f;
	m_strafe = 0.0f;

	m_velocity = Vec3(ZERO);
	m_translationRelative = Vec3(ZERO);
	m_rotationRelative = 0.0f;

	return true;
}

} //namespace Skeleton
