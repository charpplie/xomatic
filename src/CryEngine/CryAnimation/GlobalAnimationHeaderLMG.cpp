//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File: AnimationManager.cpp
//  Implementation of GlobalAnimationHeaderLMG.cpp
//
//	History:
//	Mai 25, 2010: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "LMG.h"
#include "SkeletonAnim.h"



bool GlobalAnimationHeaderLMG::Export2HTR(const char* szAnimationName, const char* savePath, const CCharacterModel* pModel, CSkeletonAnim* pSkeletonAnim)
{
	uint32 num=pSkeletonAnim->GetNumAnimsInFIFO(0);
	if (num==0)
		return 0; //fail

	const CAnimation& Animation = pSkeletonAnim->GetAnimFromFIFO(0,0);
	if (Animation.m_Parametric==0)
		return 0; //fail

	//-------------------------------------------------------------------

	std::vector<string> jointNameArray;
	std::vector<string> jointParentArray;

	const CModelSkeleton* pModelSkeleton = &pModel->m_ModelSkeleton;
	const QuatT* parrDefJoints = &pModelSkeleton->m_poseData.m_pJointsRelative[0];
	uint32 numJoints = pModelSkeleton->m_arrModelJoints.size();
	for(uint32 j=0; j<numJoints; j++)
	{
		const CModelJoint* pJoint	= &pModelSkeleton->m_arrModelJoints[j];
		assert(pJoint);
		jointNameArray.push_back( pJoint->GetJointName() );

		int16 parentID = pJoint->m_idxParent;
		if(parentID == -1)
			jointParentArray.push_back("INVALID");
		else
		{
			const CModelJoint* parentJoint	= &pModelSkeleton->m_arrModelJoints[parentID];
			if(parentJoint)
				jointParentArray.push_back(parentJoint->GetJointName());
		}		
	}

	//fetch and sum up all CAFs in this LMG
	const CAnimationSet* pAnimationSet = &pModel->m_AnimationSet;
	SParametric lmg = *Animation.m_Parametric;
	LMG::ComputeWeight(pAnimationSet, lmg);
	LMG::CheckBlendWeights(pAnimationSet, lmg);
	LMG::TW TW0 = LMG::GetTimewarpedDuration(pAnimationSet, lmg, 1.0f/30.0f);

	uint32 nFrames = uint32(1.0f/TW0.m_fDeltaTime+1.5f); 
	f32 timestep = 1.0f/f32(nFrames-1);

	std::vector< std::vector<QuatT> > arrAnimation;

	arrAnimation.resize(numJoints);
	for(uint32 j=0; j<numJoints; j++)
		arrAnimation[j].resize(nFrames);

	for(uint32 j=0; j<numJoints; j++)
	{
		for(uint32 k=0; k<nFrames; k++)
		{
			arrAnimation[j][k].q.w=0.0f;
			arrAnimation[j][k].q.v.x=0.0f;
			arrAnimation[j][k].q.v.y=0.0f;
			arrAnimation[j][k].q.v.z=0.0f;
			arrAnimation[j][k].t.x=0.0f;
			arrAnimation[j][k].t.y=0.0f;
			arrAnimation[j][k].t.z=0.0f;
		}
	}

	for (int32 a=0; a<lmg.m_numAnims; a++)
	{
		int nAnimID = lmg.m_nAnimID[a];
		f32 fWeight = lmg.m_fBlendWeight[a];
		if (fWeight==0.0f)
			continue;
		const ModelAnimationHeader* pAnim = pAnimationSet->GetModelAnimationHeader(nAnimID);
		assert(pAnim->m_nAssetType==CAF_File);
		GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[pAnim->m_nGlobalAnimId];


		const CModelJoint* pJointRoot	= &pModelSkeleton->m_arrModelJoints[0];	assert(pJointRoot);
		IController* pControllerRoot = rCAF.GetControllerByJointCRC32(pJointRoot->m_nJointCRC32);
		f32 t=timestep;	
		arrAnimation[0][0].q.SetIdentity();
		for (uint32 k=1; k<nFrames; k++)
		{
			QuatT qtold=parrDefJoints[0];
			QuatT qtnew=parrDefJoints[0];
			if(pControllerRoot)
			{
				pControllerRoot->GetOP( rCAF.NTime2KTime(t-timestep),qtold.q,qtold.t);
				qtold.q *= fsgnnz(parrDefJoints[0].q|qtold.q);  //this could be optimized at loading-time
				pControllerRoot->GetOP( rCAF.NTime2KTime(t),qtnew.q,qtnew.t);
				qtnew.q *= fsgnnz(parrDefJoints[0].q|qtnew.q);  //this could be optimized at loading-time
			}
			QuatT rel=qtold.GetInverted()*qtnew;
			arrAnimation[0][k].q+=fWeight*rel.q;
			arrAnimation[0][k].t+=fWeight*rel.t;
			t += timestep;
		}

		for(uint32 j=1; j<numJoints; j++)
		{
			const CModelJoint* pJoint	= &pModelSkeleton->m_arrModelJoints[j];	assert(pJoint);
			IController* pController = rCAF.GetControllerByJointCRC32(pJoint->m_nJointCRC32);
			t=0.0f;	
			for (uint32 k=0; k<nFrames; k++)
			{
				QuatT qt=parrDefJoints[j];
				if(pController)
				{
					pController->GetOP( rCAF.NTime2KTime(t),qt.q,qt.t);
					qt.q *= fsgnnz(parrDefJoints[j].q|qt.q);  //this could be optimized at loading-time
				}
				arrAnimation[j][k].q+=fWeight*qt.q;
				arrAnimation[j][k].t+=fWeight*qt.t;
				t += timestep;
			}
		}
	}


	for(uint32 j=0; j<numJoints; j++)
	{
		for(uint32 k=0; k<nFrames; k++)
			arrAnimation[j][k].q.Normalize();
	}

	for (uint32 k=1; k<nFrames; k++)
	{
		Vec3 abs = arrAnimation[0][k-1].t;
		Vec3 rel = arrAnimation[0][k].t;
		arrAnimation[0][k]=arrAnimation[0][k-1]*arrAnimation[0][k];
		Vec3 abs1 = arrAnimation[0][k].t;
		uint32 ddd=0;
	}

	bool htr= GlobalAnimationHeaderCAF::SaveHTR(szAnimationName, savePath, jointNameArray, jointParentArray, arrAnimation, parrDefJoints);
	bool caf= GlobalAnimationHeaderCAF::SaveCAF(szAnimationName, savePath, jointNameArray, arrAnimation);
	return true;
}

