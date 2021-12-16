#include "StdAfx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>

#include <ICryAnimation.h>

#include <CryExtension/Impl/ClassWeaver.h>
#include <CryExtension/CryCreateClassInstance.h>

#include "ITransformationPinning.h"
#include "TransformationPinning.h"

CRYREGISTER_CLASS(CTransformationPinning)

//

CTransformationPinning::CTransformationPinning() 
{	
	m_factor	= 0.0f;
	m_jointsInitialised = false;
	m_jointID = -1;
	m_numJoints = 0;
	m_jointTypes = NULL;
	m_source = NULL;
};

CTransformationPinning::~CTransformationPinning() 
{
	if (m_jointTypes)
	{
		delete [] m_jointTypes;
	}
}

//

void CTransformationPinning::SetJoint(uint32 jntID)
{
	m_jointID = jntID;
	m_jointsInitialised = false;
}

void CTransformationPinning::SetSource(ISkeletonPose *source)
{
	m_source = source;
	m_jointsInitialised = false;
}

void CTransformationPinning::Init(const SAnimationPoseModiferParams& params)
{
	m_jointsInitialised = true;
	int numJoints = m_source->GetJointCount();
	m_jointTypes = new char[numJoints];
	memset(m_jointTypes, TransformationPinJoint::Copy, sizeof(char) * numJoints);

	m_jointTypes[m_jointID] = TransformationPinJoint::Feather;
	for (int16 i=0; i<numJoints; i++)
	{
		int16 parent = m_source->GetParentIDByID(i);
		if ((parent >= 0) && (m_jointTypes[parent] != TransformationPinJoint::Copy))
		{
			m_jointTypes[i] = TransformationPinJoint::Inherit;
		}
	}

	m_numJoints = numJoints;
}

void CTransformationPinning::SetBlendWeight(float factor)
{
	m_factor = factor;
}

bool CTransformationPinning::Execute(const SAnimationPoseModiferParams& params)
{
	if (m_factor == 0.0f)
		return false;

	CRY_ASSERT(m_source && (m_jointID > 0.0f));

	if (!m_jointsInitialised)
	{
		Init(params);
	}

	ISkeletonPose* pSkeletonPose = params.pCharacterInstance->GetISkeletonPose();
	int sourceJoints = m_source->GetJointCount();
	CRY_ASSERT(sourceJoints == pSkeletonPose->GetJointCount());
	
	for (int i=0; i<sourceJoints; i++)
	{
		QuatT &joint			 = params.pPoseAbsolute[i];
		const QuatT &sourceJoint = m_source->GetAbsJointByID(i);

		QuatT &relJoint			 = params.pPoseRelative[i];
		const QuatT &relSourceJoint	 = m_source->GetRelJointByID(i);

		CRY_ASSERT(joint.IsValid());
		CRY_ASSERT(sourceJoint.IsValid());

		switch (m_jointTypes[i])
		{
		case TransformationPinJoint::Copy:
			if (m_factor < 1.0f)
			{
				QuatT diff = (joint.GetInverted() * sourceJoint);
				joint = joint * diff.GetScaled(m_factor);

				diff = (relJoint.GetInverted() * relSourceJoint);
				relJoint = relJoint * diff.GetScaled(m_factor);
			}
			else
			{
				joint = sourceJoint;
				relJoint = relSourceJoint;
			}
			break;
		case TransformationPinJoint::Feather:
			{
				int16 parent = m_source->GetParentIDByID(i);
				QuatT invParent = params.pPoseAbsolute[parent].GetInverted();
				relJoint = invParent * joint;
				QuatT thisJnt = params.pPoseAbsolute[parent] * relJoint;

				//relJoint = joint * invParent;
				//thisJnt = params.pPoseAbsolute[parent] * relJoint;

				CRY_ASSERT(relJoint.IsValid());
				CRY_ASSERT(thisJnt.IsValid());
			}
			break;
		case TransformationPinJoint::Inherit:
			break;
		}
	}

	return true;
}
