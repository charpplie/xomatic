#include "StdAfx.h"
#include <CryHeaders.h>

#include "Model.h"
#include "CharacterInstance.h"
#include "CharacterManager.h"

// Physics

// given the bone index, (INDEX, NOT ID), returns this bone's parent index
uint32 CSkeletonPose::getBoneParentIndex(uint32 nBoneIndex)
{
	assert(nBoneIndex < GetJointCount());

	CModelJoint* pSkel = &m_pInstance->m_pModel->m_ModelSkeleton.m_arrModelJoints[nBoneIndex];
	int32 pidx = nBoneIndex + pSkel->getParentIndexOffset();

	int32 pidx2=m_parrModelJoints[nBoneIndex].m_idxParent;
	if (pidx2<0)	
		pidx2=0;
	assert(pidx2==pidx);

	return pidx;
}

void CSkeletonPose::DoSetRagdollDefaultPose(Skeleton::CPoseData& poseData)
{
	m_bSetRagdollDefaultPoseRequested = 0;

	const QuatT* const __restrict pJointRelative = SPU_PTR_SELECT(&poseData.m_pJointsRelative[0], gSkeletonRelativePose);
	const QuatT* const __restrict pJointAbsolute = SPU_PTR_SELECT(&poseData.m_pJointsAbsolute[0], gSkeletonAbsolutePose);
	uint32 numJoints = poseData.GetJointCount();

	float rscale = m_fScale>0.001f ? 1.0f/m_fScale : 1.0f;
	for (uint32 i=0; i<numJoints; ++i)
	{
		int32 pidx = m_parrModelJoints[i].m_idxParent;
		if (pidx >= 0)
		{
			int32 ParentIdx = m_parrModelJoints[i].m_idxParent;
			m_arrPhysicsJoints[i].m_DefaultRelativeQuat = pJointAbsolute[i];
			if (ParentIdx>=0)
				m_arrPhysicsJoints[i].m_DefaultRelativeQuat = pJointAbsolute[ParentIdx].GetInverted() * pJointAbsolute[i];
			m_arrPhysicsJoints[i].m_DefaultRelativeQuat.t *= rscale;

			CModelJoint *pParent,*pPhysParent;
			for (int nLod=0; nLod<2; nLod++)
			{
				for (pParent = pPhysParent = m_parrModelJoints[i].getParent();
					pPhysParent && !pPhysParent->m_PhysInfo[nLod].pPhysGeom;
					pPhysParent = pPhysParent->getParent());

				if (pPhysParent) 
				{
					m_arrPhysicsJoints[i].m_qRelPhysParent[nLod] = !pJointAbsolute[pParent-&m_parrModelJoints[0]].q * pJointAbsolute[pPhysParent-&m_parrModelJoints[0]].q;
				}
				else 
				{
					m_arrPhysicsJoints[i].m_qRelPhysParent[nLod].SetIdentity();
				}

			}
		}
	}
}

SPU_NO_INLINE void CSkeletonPose::ResetNonphysicalBoneRotations(Skeleton::CPoseData& poseData, int nLod, float fBlend)
{
	QuatT* const __restrict pJointRelative = SPU_PTR_SELECT(&poseData.m_pJointsRelative[0], gSkeletonRelativePose);

	// set non-physical bones to their default position wrt parent for LODs>0
	// do it for every bone except the root, parents first
	uint32 numBones = m_arrPhysicsJoints.size();
	//	assert(numBones); having 0 physical bones is a legal case
	for (uint32 nBone=1; nBone<numBones; ++nBone)
	{
		const CModelJoint& joint = m_parrModelJoints[nBone];

//		if (m_pSkeletonAnim->m_IsAnimPlaying || m_bPhysicsRelinquished)
		{
			if (joint.getPhysInfo(nLod).pPhysGeom)
				continue;
		}

		const CPhysicsJoint& physicsJoint = m_arrPhysicsJoints[nBone];
		QuatT qDef(physicsJoint.m_DefaultRelativeQuat.q, physicsJoint.m_DefaultRelativeQuat.t * m_fScale);
		if (fBlend >= 1.0f)
		{
			 pJointRelative[nBone] = qDef;
		}
		else 
		{
			QuatT g(physicsJoint.m_qRelFallPlay, physicsJoint.m_DefaultRelativeQuat.t * m_fScale);
			pJointRelative[nBone].SetNLerp(g, qDef, fBlend);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Multiplies each bone global matrix with the parent global matrix,
// and calculates the relative-to-default-pos matrix. This is essentially
// the process opposite to conversion of global matrices to relative form
// performed by ConvertBoneGlobalToRelativeMatrices()
// NOTE:
//   The root matrix is relative to the world, which is its parent, so it's
//   obviously both the global and relative (they coincide), so it doesn't change
//
// PARAMETERS:
//   bNonphysicalOnly - if set to true, only those bones that have no physical geometry are affected
//
// ASSUMES: in each m_matRelativeToParent matrix, upon entry, there's a matrix relative to the parent
// RETURNS: in each bone global matrix, there's the actual global matrix
SPU_NO_INLINE void CSkeletonPose::UnconvertBoneGlobalFromRelativeForm(Skeleton::CPoseData& poseData, bool bNonphysicalOnlyArg, int nLod, bool bRopeTipsOnly)
{
	uint32 jointCount = poseData.GetJointCount();
	QuatT* const __restrict pJointRelative = SPU_PTR_SELECT(&poseData.m_pJointsRelative[0], gSkeletonRelativePose);
	QuatT* const __restrict pJointAbsolute = SPU_PTR_SELECT(&poseData.m_pJointsAbsolute[0], gSkeletonAbsolutePose);

	uint32 nRopeLevel = 0;
	bool bNonphysicalOnly = true;

	const CModelJoint* pModelJoints = &m_parrModelJoints[0];

	// start from 1, since we don't affect the root, which is #0
	for (uint32 i=1; i<jointCount; ++i)
	{
		const CModelJoint& modelJoint = pModelJoints[i];

		bool bPhysical =
			modelJoint.getPhysInfo(nLod).pPhysGeom ||
			modelJoint.getLimbId()>=100;

		if ((modelJoint.getPhysInfo(0).flags & 0xFFFF0000) == 0x30000)
		{
			nRopeLevel = modelJoint.m_numLevels;
		}
		else
		{
			if (modelJoint.m_numLevels<nRopeLevel)
				nRopeLevel = 0;

			// -2 is for rope bones, never force their positions
			if ((!bNonphysicalOnly || !bPhysical) && (!bRopeTipsOnly || nRopeLevel))
			{
				//	assert (pBoneInfo->hasParent());
				// CRAIG - gross hack - must be fixed properly
				if (!modelJoint.qhasParent())
					continue;
				// END GROSS HACK

				pJointAbsolute[i] = pJointAbsolute[getBoneParentIndex(i)] * pJointRelative[i];
			}
		}

		if (bPhysical) // don't update the 1st physical bone (pelvis) even if bNonphysicalOnlyArg is false
			bNonphysicalOnly = bNonphysicalOnlyArg;
	}
}

// finds the first physicalized parent of the given bone (bone given by index)
int CSkeletonPose::getBonePhysParentIndex(int iBoneIndex, int nLod)
{
	int iPrevBoneIndex;
	do 
	{
		iBoneIndex = getBoneParentIndex(iPrevBoneIndex = iBoneIndex);
	} 
	while (iBoneIndex!=iPrevBoneIndex && !GetModelJointPointer(iBoneIndex)->m_PhysInfo[nLod].pPhysGeom);

	return iBoneIndex==iPrevBoneIndex ? -1 : iBoneIndex;
}

// Entity

void CSkeletonPose::SPU_Physics_SynchronizeFromEntityPrepare(Memory::CPool& memoryPool, IPhysicalEntity* pent)
{
	DEFINE_PROFILER_FUNCTION();

	m_bPhysicsSynchronizeFromEntity = false;

	if (!m_bPhysicsRelinquished)
		return;

	if (!pent && !m_nAuxPhys) 
		return;

	// Query for physical entity awakeness

	if (pent)
	{
		pe_status_awake statusTmp2; statusTmp2.lag=2;
		m_bPhysicsAwake = (pent->GetStatus(&statusTmp2)!=0);
	}

	for (int j=0; j<m_nAuxPhys; j++)
	{
		pe_status_awake statusTmp; 
		m_bPhysicsAwake |= (m_auxPhys[j].pPhysEnt->GetStatus(&statusTmp) !=0);
	}

	if (!m_bPhysicsAwake && !m_bPhysicsWasAwake)
		return;

	if (!pent)
		return;

	m_bPhysicsSynchronizeFromEntity = true;

	pe_status_joint sj;
	pe_status_pos partpos;
	partpos.flags = status_local;

	uint32 jointCount = GetJointCount();
	
	int nLod = (m_pCharPhysics != pent || m_bPhysicsRelinquished) ? GetPhysicsLod() : 0;

	m_pPhysBuffer = (PhysData*)memoryPool.Allocate(jointCount * sizeof(PhysData));
	m_bPhysBufferFilled = false;

	CModelJoint* pModelJoints = &m_parrModelJoints[0];
	for (uint32 i=0; i<jointCount; ++i) 
	{
		m_pPhysBuffer[i].bSet = false;

		if (!pModelJoints[i].m_PhysInfo[nLod].pPhysGeom) 
			continue;

		partpos.partid = i;
		int32 status = pent->GetStatus(&partpos);
		if (!status)
			continue;

		m_pPhysBuffer[i].bSet = true;
		m_pPhysBuffer[i].location.q = partpos.q;
		m_pPhysBuffer[i].location.t = partpos.pos;
		if (m_fPhysBlendTime < m_fPhysBlendMaxTime)
			break; // if in blending stage, do it only for the root
	}

	for (uint32 i=0; i<jointCount; ++i) 
	{
		m_pPhysBuffer[i].bSet2 = false;

		if (!pModelJoints[i].m_PhysInfo[nLod].pPhysGeom) 
			continue;

		sj.idChildBody = i;
		int32 status = pent->GetStatus(&sj);
		if (!status)
			continue;

		m_pPhysBuffer[i].rotation = sj.quat0;
		m_pPhysBuffer[i].angles = sj.q + sj.qext;

		m_pPhysBuffer[i].bSet2 = true;
	}

	pe_status_pos sp;
	int status2 = pent->GetStatus(&sp);
	if (status2==0)
	{
		g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pInstance->m_pModel->GetModelFilePath(), "GetStatus() returned 0" );
		assert(0);
	}
}

void CSkeletonPose::SPU_Physics_SynchronizeFromEntity(Skeleton::CPoseData& poseData, IPhysicalEntity* pent, const Vec3& posMaster, const Quat& qMaster, QuatT offset)
{
	DEFINE_PROFILER_FUNCTION();

	if (!m_bPhysicsSynchronizeFromEntity)
		return;

	if (!m_pPhysBuffer)
		return;

	//

	if (!m_bPhysicsAwake)
		m_fPhysBlendTime = m_fPhysBlendMaxTime + 0.1f;

	m_bPhysicsAwake = false;

	int nLod = (m_pCharPhysics!=pent || m_bPhysicsRelinquished) ? GetPhysicsLod() : 0;
	ResetNonphysicalBoneRotations(poseData, nLod, m_fPhysBlendTime * m_frPhysBlendMaxTime);	

	QuatT roffs(!offset.q, -offset.t * offset.q);

	uint32 jointCount = poseData.GetJointCount();








	QuatT* const __restrict pJointRelative = SPU_PTR_SELECT(&poseData.m_pJointsRelative[0], gSkeletonRelativePose);
	QuatT* const __restrict pJointAbsolute = SPU_PTR_SELECT(&poseData.m_pJointsAbsolute[0], gSkeletonAbsolutePose);
	PhysData * const __restrict pPhysBuffer = SPU_PTR_SELECT(&m_pPhysBuffer[0], spuPhysBuffer);
	CPhysicsJoint * const __restrict pPhysJoints = SPU_PTR_SELECT(&m_arrPhysicsJoints[0], spuPhysJoints);

	if (pent)
	{
		CModelJoint* pModelJoints = &m_parrModelJoints[0];
		for (uint32 i=0; i<jointCount; ++i) 
		{
			if (!pModelJoints[i].m_PhysInfo[nLod].pPhysGeom) 
				continue;

			if (!pPhysBuffer[i].bSet)
				continue;

			pJointAbsolute[i] = roffs * pPhysBuffer[i].location;
			if (m_fPhysBlendTime < m_fPhysBlendMaxTime)
				break; // if in blending stage, do it only for the root
		}

		for (uint32 i=0; i<jointCount; ++i) 
		{
			if (!m_pPhysBuffer[i].bSet2)
				continue;

			pJointRelative[i].q = pPhysJoints[i].m_qRelPhysParent[nLod] * pPhysBuffer[i].rotation * Quat::CreateRotationXYZ(pPhysBuffer[i].angles);
			pJointRelative[i].t = pPhysJoints[i].m_DefaultRelativeQuat.t * m_fScale;
		}
	}

	UnconvertBoneGlobalFromRelativeForm(poseData, m_fPhysBlendTime >= m_fPhysBlendMaxTime, nLod);
	poseData.ComputeRelativePose(m_parrModelJoints);

	m_bPhysicsWasAwake = m_bPhysicsAwake;
}

void CSkeletonPose::SPU_Physics_SynchronizeFromEntityArticulated(Skeleton::CPoseData& poseData, float fDeltaTimePhys, int nNeff)
{
	if (m_bPhysicsRelinquished)
	{
		m_bPhysicsWasAwake = true;

		SPU_Physics_SynchronizeFromEntity(poseData, m_pCharPhysics, Vec3(0,0,0), Quat(1,0,0,0), QuatT(IDENTITY));

		// [*DavidR | 1/Dec/2009] Make sure CharacterInstance it's using the same root orientation
//		m_pInstance->m_PhysEntityLocation.q.SetIdentity();

		m_fPhysBlendTime += fDeltaTimePhys;
	}
}

void CSkeletonPose::SPU_Physics_SynchronizeFrom(Skeleton::CPoseData& poseData, const QuatT& rPhysEntityLocation, float fDeltaTimePhys, int nNeff, SCharUpdateFeedback *pCharFeedback)
{
	if (!m_pSkeletonAnim->m_IsAnimPlaying && !m_bPhysicsRelinquished && !m_nAuxPhys)
		return;

	SPU_Physics_SynchronizeFromEntityArticulated(poseData, fDeltaTimePhys, nNeff);
	SPU_Physics_SynchronizeFromAux(poseData, Matrix34(rPhysEntityLocation));
}

// Aux

void CSkeletonPose::SPU_Physics_SynchronizeFromAuxPrepare(Memory::CPool& memoryPool, int nNeff)
{
	m_bPhysicsSynchronizeAux = false;
	m_pPhysAuxBuffer = NULL;

	if (!m_nAuxPhys)
		return;

	pe_params_articulated_body pab;
	if (m_pCharPhysics)
		m_pCharPhysics->GetParams(&pab);

	m_pPhysAuxBuffer = (PhysAuxData*)memoryPool.Allocate(m_nAuxPhys * sizeof(PhysAuxData));
	if (!m_pPhysBuffer)
	{
		uint32 jointCount = GetJointCount();
		m_pPhysBuffer = (PhysData*)memoryPool.Allocate(jointCount * sizeof(PhysData));
		m_bPhysBufferFilled = false;
	}

	int bAwake = 0;
	for (int j=0; j<m_nAuxPhys; ++j)
	{
		m_pPhysAuxBuffer[j].bSet = false;
		m_pPhysAuxBuffer[j].matrix = Matrix34(IDENTITY);
		m_pPhysAuxBuffer[j].bPosHostPivot = is_unused(pab.posHostPivot);

		const pe_type peType = m_auxPhys[j].pPhysEnt->GetType();
		if (peType == PE_SOFT)
		{
			pe_status_softvtx ssv;
			if (m_auxPhys[j].pPhysEnt->GetStatus(&ssv))
			{
				m_pPhysAuxBuffer[j].bSet = true;
				m_pPhysAuxBuffer[j].pPoints = m_auxPhys[j].pVtx;
				for(int i=0; i<=m_auxPhys[j].nBones; i++)
					m_pPhysAuxBuffer[j].pPoints[i] = ssv.pVtx[i];
				if (!m_pPhysAuxBuffer[j].bPosHostPivot)
					m_pPhysAuxBuffer[j].matrix = Matrix34(Vec3(1.0f), ssv.qHost, ssv.posHost);
			}
		}
		else
		{
			pe_status_rope sr;
			sr.pPoints = NULL;
			sr.pVtx = NULL;

			if (!m_auxPhys[j].pPhysEnt->GetStatus(&sr))
				continue;

			if (!m_pPhysAuxBuffer[j].bPosHostPivot)
				m_pPhysAuxBuffer[j].matrix = Matrix34(Vec3(1.0f), sr.qHost, sr.posHost);

			sr.pPoints = m_auxPhys[j].pVtx;
			m_auxPhys[j].pPhysEnt->GetStatus(&sr);
			m_pPhysAuxBuffer[j].bSet = !(sr.bTargetPoseActive==1 && sr.stiffnessAnim==0);

			m_pPhysAuxBuffer[j].pPoints = sr.pPoints;
		}

		pe_status_awake statusTmp;
		bAwake |= m_auxPhys[j].pPhysEnt->GetStatus(&statusTmp);
	}

	if (m_bPhysicsRelinquished && !bAwake)
		return;

	m_bPhysicsSynchronizeAux = true;
}

void CSkeletonPose::SPU_Physics_SynchronizeFromAux(Skeleton::CPoseData& poseData, const Matrix34& locationPhysics)
{
	if (!m_bPhysicsSynchronizeAux)
		return;

	if (!m_pPhysAuxBuffer)
		return;

	int nLod = m_bPhysicsRelinquished ? GetPhysicsLod() : 0;

	if (m_bPhysicsSynchronizeFromEntity &&
		!m_pSkeletonAnim->m_IsAnimPlaying && !m_bPhysicsRelinquished)
	{
		ResetNonphysicalBoneRotations(poseData, nLod, 2.0f);
		poseData.ComputeAbsolutePose(m_parrModelJoints);
	}

	uint32 jointCount = poseData.GetJointCount();

	QuatT* const __restrict pJointRelative = SPU_PTR_SELECT(&poseData.m_pJointsRelative[0], gSkeletonRelativePose);
	QuatT* const __restrict pJointAbsolute = SPU_PTR_SELECT(&poseData.m_pJointsAbsolute[0], gSkeletonAbsolutePose);

	// IZF: We have to buffer this back because we need the locations of the
	// joints before they are modified to blend whatever animation was playing
	// with the physics rope simulation.
	if (m_pPhysBuffer)
	{
		for (uint32 i=0; i<jointCount; ++i)
			m_pPhysBuffer[i].location = pJointAbsolute[i];
		m_bPhysBufferFilled = true;
	}





	PhysAuxData* const __restrict pPhysAuxBuffer = SPU_PTR_SELECT(&m_pPhysAuxBuffer[0], spuPhysAuxBuffer);

	Matrix33 orientationPhysics(locationPhysics);
	for (int j=0; j<m_nAuxPhys; ++j)
	{
		if (!pPhysAuxBuffer[j].bSet)
			continue;

		aux_phys_data &r_cur_aux_phys_data = m_auxPhys[j];

		Matrix34 root(IDENTITY);
		if (r_cur_aux_phys_data.bTied0)
			root = Matrix34(pJointAbsolute[getBonePhysParentIndex(r_cur_aux_phys_data.iBoneTiedTo[0], nLod)]);

		Matrix34 mtxBone;
		if  (pPhysAuxBuffer[j].bPosHostPivot)
			mtxBone = root * (locationPhysics * root).GetInvertedFast();
		else
			mtxBone = root * pPhysAuxBuffer[j].matrix.GetInvertedFast();

		int nAuxBones = r_cur_aux_phys_data.nBones;
		for (int i=0; i<nAuxBones; ++i) 
		{
			SpuStackValue<aux_bone_info, false, true> rAuxBoneInfo(r_cur_aux_phys_data.pauxBoneInfo[i]);

			Quat qParent(IDENTITY);// = pJointAbsolute[getBoneParentIndex(rAuxBoneInfo->iBone)].q;
			Vec3 dir = !qParent*((pPhysAuxBuffer[j].pPoints[i+1] - pPhysAuxBuffer[j].pPoints[i]) * orientationPhysics);
			float len = dir.GetLength();
			Vec3 v0 = rAuxBoneInfo->dir0;
			Vec3 v1 = len>1E-5f ? dir/len : v0;
			Quat q0 = rAuxBoneInfo->quat0;

			QuatT& matBoneGlobal34 = pJointAbsolute[rAuxBoneInfo->iBone];
			matBoneGlobal34.q = qParent * (Quat::CreateRotationV0V1(v0, v1) * q0).GetNormalized();
			matBoneGlobal34.t = mtxBone * pPhysAuxBuffer[j].pPoints[i];
		}

		int iParent = r_cur_aux_phys_data.pauxBoneInfo[nAuxBones-1].iBone;
		if (GetModelJointPointer(iParent)->numChildren() > 0)
		{
			int i = GetModelJointChildIndex(iParent, 0);

			Vec3 vCurAuxBone = pPhysAuxBuffer[j].pPoints[nAuxBones];
			pJointAbsolute[i].t = mtxBone * vCurAuxBone;
			pJointRelative[i].t =
				pJointAbsolute[iParent].GetInverted() *
				pJointAbsolute[i].t;
		}
	}

	UnconvertBoneGlobalFromRelativeForm(poseData, true, nLod, true);
	poseData.ComputeRelativePose(m_parrModelJoints);
}

//

void CSkeletonPose::SPU_SynchronizeWithPhysics(Skeleton::CPoseData& poseData, const QuatT& locationPhysics, const QuatTS& locationAnimation)
{
	DEFINE_PROFILER_FUNCTION();

	if (m_bSetRagdollDefaultPoseRequested)
		DoSetRagdollDefaultPose(poseData);

	SPU_Physics_SynchronizeFrom(poseData, locationPhysics, m_pInstance->m_fOriginalDeltaTime, m_pSkeletonAnim->m_IsAnimPlaying, 0);
}

void CSkeletonPose::SPU_SynchronizeWithPhysicsPrepare(Memory::CPool& memoryPool)
{
	m_bPhysicsSynchronize = false;
	m_bPhysicsSynchronizeFromEntity = false;
	m_ePhysicsEntityType = PE_NONE;

	m_bPhysicsSynchronizeAux = false;

	m_pPhysBuffer = NULL;
	m_pPhysAuxBuffer = NULL;

	if (m_pCharPhysics)
		m_ePhysicsEntityType = m_pCharPhysics->GetType();

	m_fPhysBlendTime += m_pInstance->m_fDeltaTime;

	// Determine the need for a physics sync.
	bool bSyncPhysics =
		m_bFullSkeletonUpdate &&
		m_pInstance->m_pModel->m_ObjectType == CHR &&
		(gEnv->pPhysicalWorld->GetPhysVars()->lastTimeStep > 0 || m_pSkeletonAnim->GetCharEditMode()) &&
		Console::GetInst().ca_UsePhysics;

	if (!bSyncPhysics)
		return;

	m_bPhysicsSynchronize = true;

	SPU_Physics_SynchronizeFromEntityPrepare(memoryPool, m_pCharPhysics);
	SPU_Physics_SynchronizeFromAuxPrepare(memoryPool, m_pSkeletonAnim->m_IsAnimPlaying);
}
