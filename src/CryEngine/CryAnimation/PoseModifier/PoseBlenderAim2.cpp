#include "stdafx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "../CharacterInstance.h"
#include "../Model.h"
#include "../ModelSkeleton.h"
#include "../CharacterManager.h"

#include "PoseModifierHelper.h"
#include "PoseBlenderAim.h"


#if (NEWAIMSYSTEM==0)
struct DBW2
{
	uint32	m_nAnimTokenCRC32; 
	int32		m_nParaJointIdx; 
	int32		m_nStartJointIdx;
	f32			m_fWeight;
	DBW2() {
		m_nAnimTokenCRC32=0; 
		m_nParaJointIdx=-1; 
		m_nStartJointIdx=-1;
		m_fWeight=0;
	}
};
#endif

//-----------------------------------------------------------------------------------------------------
//-----                        compute pose for aim-ik                            ---------------------
//-----------------------------------------------------------------------------------------------------

#if (NEWAIMSYSTEM==1)
SPU_INDIRECT(CommandBufferExecute(LL))
bool CPoseBlenderAim::ExecuteAimIK_New(const SAnimationPoseModiferParams& params)
{
	float fColDebug[4] = {1,1,0,1};
	CCharInstance* pInstance = PoseModifierHelper::GetCharInstance(params);

	CSkeletonPose* pSkeletonPose		= PoseModifierHelper::GetSkeletonPose(params);
	CSkeletonAnim* pSkeletonAnim		= PoseModifierHelper::GetSkeletonAnim(params);
	CModelSkeleton* pModelSkeleton	= PoseModifierHelper::GetModelSkeleton(params);
	CModelJoint* parrModelJoints		= pSkeletonPose->m_parrModelJoints;
	CAnimationSet* pAnimationSet		=	&pInstance->m_pModel->m_AnimationSet;
	QuatT* const __restrict pRelPose = SPU_LOCAL_PTR(&params.pPoseRelative[0]);
	QuatT* const __restrict pAbsPose = SPU_LOCAL_PTR(&params.pPoseAbsolute[0]);

	if (pInstance->m_pModel->m_ObjectType==CGA)
		return 0;

	const QuatTS& rAnimLocationNext = SPU_LOCAL_REF(params.locationNextAnimation);
	int16 const * __restrict pIdxArray = pModelSkeleton->m_IdxArray;

	m_AimIKInfluence=0;

	Vec3 vAimIKTarget=(m_AimIKTarget-rAnimLocationNext.t)*rAnimLocationNext.q;


	uint32 numJoints = params.jointCount;
	pAbsPose[0] = pRelPose[0];
	for (uint32 i=1; i<numJoints; i++)
		pAbsPose[i]	= pAbsPose[pModelSkeleton->GetJointParentIDByID(i)] * pRelPose[i];

	m_AimIKFadeout = 0; // by default we are in fadeout mode
	if (Console::GetInst().ca_UseAimIK == 0)
		return false;

	uint32 numRotJoints = pModelSkeleton->m_AimIK_Rot.size();
	if (numRotJoints==0)
	{
#if !defined(__SPU__)		
		const char* pModelName = pInstance->m_pModel->GetFilePath();
		g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, pInstance->GetFilePath(), "CryAnimation: No AimIK-Setup for model: %s",pModelName );
#endif		
		return false;
	}

	struct DBW
	{
		uint32	m_nAnimTokenCRC32; 
		int32		m_nParaJointIdx; 
		int32		m_nStartJointIdx;
		f32			m_fWeight;
		DBW() {
			m_nAnimTokenCRC32=0; 
			m_nParaJointIdx=-1; 
			m_nStartJointIdx=-1;
			m_fWeight=0;
		}
	};

	DBW dbw[4];

	uint32 numDB = pModelSkeleton->m_DirectionalBlends.size();
	if (numDB==0)
	{
#if !defined(__SPU__)				
		const char* pModelName = pInstance->m_pModel->GetFilePath();
		g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, pInstance->GetFilePath(), "CryAnimation: No weapon-bone specified for model: %s",pModelName );
#endif		
		return false;
	}
	DirectionalBlends *pDirectionalBlends = &pModelSkeleton->m_DirectionalBlends[0];
	for (uint32 d=0; d<numDB; d++)
	{
		dbw[d].m_nAnimTokenCRC32	= pDirectionalBlends[d].m_AnimTokenCRC32;
		dbw[d].m_nParaJointIdx		=	pDirectionalBlends[d].m_nParaJointIdx;
		dbw[d].m_nStartJointIdx		=	pDirectionalBlends[d].m_nStartJointIdx;
	}

	for (uint32 d=0; d<numDB; d++)
	{
		if (dbw[d].m_nParaJointIdx<0 || dbw[d].m_nStartJointIdx<0)
		{
#if !defined(__SPU__)				
			const char* pModelName = pInstance->m_pModel->GetFilePath();
			g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, pInstance->GetFilePath(), "CryAnimation: No weapon-bone specified for model: %s",pModelName );
#endif		
			return false;
		}
	}

	if (pSkeletonPose->m_timeStandingUp >= 0)
		return false;
	if (m_numActiveAimPoses==0)
		return false; // no animation in base-layer, no aim-pose

	//-----------------------------------------------------------------------------------------------

	//some rules to disable aim-ik
	//target is too close to the weapon
	Vec3 nWeaponBoneWorld	= rAnimLocationNext*pAbsPose[dbw[0].m_nParaJointIdx].t;
	f32 fDistance = (vAimIKTarget-nWeaponBoneWorld).GetLength();

	if ( fDistance<1.0f )
		m_AimIKFadeout=1;
	if (m_AimIKBlend<0.001f)
		return false;

	//------------------------------------------------------------------------------------
	//---------         here we know that we can actually use AimIK          -------------
	//------------------------------------------------------------------------------------
	f32 fWeightOfAllAimPoses=0;
	for (uint32 i=0; i<m_numActiveAimPoses; i++)
		fWeightOfAllAimPoses += m_AimInfo[i].m_fWeight;



	QuatT arrRelPose[64];
	f32 fIKBlend = clamp(m_AimIKBlend,0.0f,1.0f)-0.5f;
	fIKBlend = fIKBlend/(0.5f+2.0f*fIKBlend*fIKBlend)+0.5f;
	f32 t0 = 1.0f-fIKBlend*fWeightOfAllAimPoses;
	assert(t0< 1.001f);
	assert(t0>-0.001f);

// 	g_YLine+=16.0f;
//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fColDebug, false,"m_numActiveAimPoses: %d  fWeightOfAllAimPoses: %f  fIKBlend: %f   t0: %f",m_numActiveAimPoses,fWeightOfAllAimPoses,fIKBlend,t0 );	
//	g_YLine+=16.0f;


	f32 t1 = fIKBlend;
	SJointsAimIK_Rot *pAimIK_Rot = &pModelSkeleton->m_AimIK_Rot[0];
	for (uint32 r=0; r<numRotJoints; r++)
	{
		int32 j	=pAimIK_Rot[r].m_nJointIdx;
		arrRelPose[r].q.w	 =pRelPose[j].q.w;		pRelPose[j].q.w		*= t0;
		arrRelPose[r].q.v.x=pRelPose[j].q.v.x;	pRelPose[j].q.v.x *= t0;
		arrRelPose[r].q.v.y=pRelPose[j].q.v.y;	pRelPose[j].q.v.y *= t0;
		arrRelPose[r].q.v.z=pRelPose[j].q.v.z;	pRelPose[j].q.v.z *= t0;
		//	int32 p	= pModelSkeleton->m_AimIK_Rot[r].m_nPosIndex;
		//	if (p>-1)
		{
			arrRelPose[r].t.x = pRelPose[j].t.x;	pRelPose[j].t.x	*= t0;
			arrRelPose[r].t.y = pRelPose[j].t.y;	pRelPose[j].t.y	*= t0;
			arrRelPose[r].t.z = pRelPose[j].t.z;	pRelPose[j].t.z	*= t0;
		}
	}



	assert(m_numActiveAimPoses);

	f32 fTotalWeight = t0;
	GlobalAnimationHeaderAIM *parrGlobalAIM = &g_AnimationManager.m_arrGlobalAIM[0];

	for (int32 a=0; a<m_numActiveAimPoses; a++)
	{
		uint32 numAimPosesPerAnim = m_AimInfo[a].m_numAimPoses;
		assert(numAimPosesPerAnim==1 || numAimPosesPerAnim==2);
		if (numAimPosesPerAnim==1)
		{	
			assert(m_AimInfo[a].m_nGlobalAimID0>=0);
			GlobalAnimationHeaderAIM& rGAH = parrGlobalAIM[m_AimInfo[a].m_nGlobalAimID0];
			uint32 nAnimTokenCRC32 = rGAH.m_AnimTokenCRC32;
			f32 fAimPoseWeight = m_AimInfo[a].m_fWeight;
			for (uint32 d=0; d<numDB; d++)
			{
				if (dbw[d].m_nAnimTokenCRC32==rGAH.m_AnimTokenCRC32)
				{
					dbw[d].m_fWeight += t1*fAimPoseWeight;
					AccumulateAimPoses( pInstance,rGAH,arrRelPose,t1*fAimPoseWeight,vAimIKTarget,dbw[d].m_nParaJointIdx);
					fTotalWeight += t1*fAimPoseWeight;
#ifndef PS3		
					if (Console::GetInst().ca_DrawAimIKVEGrid) DebugVEGrid( pInstance, m_AimInfo[a], fIKBlend,dbw[d].m_nParaJointIdx);
#endif
					break;
				}
			}
			continue;
		}

		if (numAimPosesPerAnim==2)
		{	
			assert(m_AimInfo[a].m_nGlobalAimID0>=0);
			GlobalAnimationHeaderAIM& rGAH0 = parrGlobalAIM[m_AimInfo[a].m_nGlobalAimID0];
			f32 a0 = 1.0f-m_AimInfo[a].m_fAnimTime;
			f32 fAimPoseWeight0 = m_AimInfo[a].m_fWeight*a0;
			for (uint32 d=0; d<numDB; d++)
			{
				if (dbw[d].m_nAnimTokenCRC32==rGAH0.m_AnimTokenCRC32)
				{
					dbw[d].m_fWeight += t1*fAimPoseWeight0;
					AccumulateAimPoses( pInstance,rGAH0,arrRelPose,t1*fAimPoseWeight0,vAimIKTarget,dbw[d].m_nParaJointIdx);
					fTotalWeight += t1*fAimPoseWeight0;
					break;
				}
			}

			assert(m_AimInfo[a].m_nGlobalAimID1>=0);
			GlobalAnimationHeaderAIM& rGAH1 = parrGlobalAIM[m_AimInfo[a].m_nGlobalAimID1];
			f32 a1 = m_AimInfo[a].m_fAnimTime;
			f32 fAimPoseWeight1 = m_AimInfo[a].m_fWeight*a1;
			for (uint32 d=0; d<numDB; d++)
			{
				if (dbw[d].m_nAnimTokenCRC32==rGAH1.m_AnimTokenCRC32)
				{
					dbw[d].m_fWeight += t1*fAimPoseWeight1;
					AccumulateAimPoses( pInstance,rGAH1,arrRelPose,t1*fAimPoseWeight1,vAimIKTarget,dbw[d].m_nParaJointIdx);
					fTotalWeight += t1*fAimPoseWeight1;
					break;
				}
			}
			continue;
		}
		assert(0);
	}
	assert( fabsf(fTotalWeight-1.0f) < 0.001f );


	for (uint32 r=0; r<numRotJoints; r++)
	{
		int32 j	= pAimIK_Rot[r].m_nJointIdx;
		int32 p = parrModelJoints[j].m_idxParent;
		pRelPose[j].q.Normalize();
		pAbsPose[j]	= pAbsPose[p] * pRelPose[j];
	}


	for (uint32 g=0; g<numDB; g++)
	{
		int32 nDirJointIdx = dbw[g].m_nParaJointIdx;
		f32 wR = dbw[g].m_fWeight;
		if (nDirJointIdx>0)
		{
			int32 nSpine2 = pIdxArray[eIM_Spine2Idx];
			int32 nSpine3 = pIdxArray[eIM_Spine3Idx];
			if (nSpine2>0 && nSpine3>0)
			{
				Quat rel=pRelPose[nSpine3].q;
				for (uint32 d=0; d<4; d++)
				{
					Vec3 realdir	=	pAbsPose[nDirJointIdx].q.GetColumn1();
					Vec3 wantdir	=	(vAimIKTarget-pAbsPose[nDirJointIdx].t).GetNormalized();
					f32 angle = acos_tpl( realdir|wantdir );

					f32 blend = 1-MIN(fabsf(angle),1.0f);
					blend = blend*blend*blend*blend;

					//		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColDebug, false,"angle: %f   blend: %f",angle,blend );	
					//		g_YLine+=16.0f;

					Quat rel; rel.SetRotationV0V1(realdir,wantdir);
					rel.SetNlerp(IDENTITY,rel,blend*0.4f);

					pAbsPose[nSpine3]	  = rel*pAbsPose[nSpine3];
					pRelPose[nSpine3].q = !pAbsPose[nSpine2].q*pAbsPose[nSpine3].q;
					for (uint32 r=0; r<numRotJoints; r++)
					{
						int32 j	= pAimIK_Rot[r].m_nJointIdx;
						int32 p = parrModelJoints[j].m_idxParent;
						pAbsPose[j]	= pAbsPose[p] * pRelPose[j];
					}
				}
				pRelPose[nSpine3].q.SetNlerp(rel,pRelPose[nSpine3].q,wR);
				for (uint32 r=0; r<numRotJoints; r++)
				{
					int32 j	= pAimIK_Rot[r].m_nJointIdx;
					int32 p = parrModelJoints[j].m_idxParent;
					pAbsPose[j]	= pAbsPose[p] * pRelPose[j];
				}
			}
		}

		m_AimIKInfluence=fWeightOfAllAimPoses;

#if !defined(__SPU__)
		float fTextColor[4] = {1,0,0,1};
		if( Console::GetInst().ca_DrawAimPoses)
		{
			Vec3 RWBone		= rAnimLocationNext.q*pAbsPose[nDirJointIdx].t;
			Vec3 WStart		= rAnimLocationNext.t+RWBone;
			Vec3 realdir	=	pAbsPose[nDirJointIdx].q.GetColumn1();
			Vec3 WEnd   = WStart + rAnimLocationNext.q*realdir*10;
			//	m_AimDirection	= (WEnd-WStart).GetNormalized();

			g_pAuxGeom->DrawLine(	WStart,RGBA8(0x00,0x7f,0x7f,0xff), WEnd,RGBA8(0x00,0xff,0xff,0xff) );
			Vec2 vPolarCoord = Vec2( pAnimationSet->PolarCoordinate(pAbsPose[nDirJointIdx].q) );
			g_YLine+=16.0f;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 2.4f, fTextColor, false,"final polar: %f %f",vPolarCoord.x,vPolarCoord.y );	
			g_YLine+=16.0f;
		}
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"Aim-IK Influence: %f %f %f",m_AimIKInfluence,fTotalWeight,fIKBlend );	
	//	g_YLine+=16.0f;
#endif


	}
	
	m_AimIKInfluence=fWeightOfAllAimPoses;

	return true;
}


SPU_NO_INLINE void CPoseBlenderAim::AccumulateAimPoses(CCharInstance* pInstance,GlobalAnimationHeaderAIM& rGAH,QuatT* arrRelPose,f32 fIKBlend,const Vec3& vAimIKTarget,int32 widx)
{
	Matrix33 m33; m33.SetRotationX(-gf_PI/2);
	m33.m00=-m33.m00;	
	m33.m01=-m33.m01; 
	m33.m02=-m33.m02;

	uint32 numAimPosesCAF = rGAH.m_arrAimIKPosesAIM.size();	
	assert(numAimPosesCAF==9 || numAimPosesCAF==15 || numAimPosesCAF==27);
	if (numAimPosesCAF==0)
		return;

	uint32 AimPoseMid = uint32(numAimPosesCAF/2);

	CModelSkeleton* pModelSkeleton	=	&pInstance->m_pModel->m_ModelSkeleton;
	CModelJoint* parrModelJoints		= &pModelSkeleton->m_arrModelJoints[0];

	////////////////////////////////////////////////////////////////////////////

	float fTextColor[4] = {1,0,0,1};


	QuatT qtemp;
	const Quat*  const __restrict pSrcRot = &rGAH.m_arrAimIKPosesAIM[AimPoseMid].m_arrRotation[0];
	const Vec3*  const __restrict pSrcPos = &rGAH.m_arrAimIKPosesAIM[AimPoseMid].m_arrPosition[0];
	QuatT* const __restrict jointsRelative = SPU_PTR_SELECT(&pInstance->m_SkeletonPose.GetPoseDataWriteable()->m_jointsRelative[0], gSkeletonRelativePose);	
	QuatT* const __restrict jointsAbsolute = SPU_PTR_SELECT(&pInstance->m_SkeletonPose.GetPoseDataWriteable()->m_jointsAbsolute[0], gSkeletonAbsolutePose);	
	uint32 numRotJoints = pModelSkeleton->m_AimIK_Rot.size();
	SJointsAimIK_Rot *pAimIK_Rot = &pModelSkeleton->m_AimIK_Rot[0];

	for (uint32 r=0; r<numRotJoints; r++)
	{
		if (pAimIK_Rot[r].m_nPreEvaluate)
		{
			int32 j	= pAimIK_Rot[r].m_nJointIdx;
			int32 pidx = parrModelJoints[j].m_idxParent;
			qtemp = arrRelPose[r];
			if (rGAH.m_nExist & uint64(1<<r))
			{
				int32 a	= pAimIK_Rot[r].m_nAdditive;
				qtemp.q = pSrcRot[r];
				assert(qtemp.q.IsValid());
				assert(pSrcRot[r].IsValid());
				if (a) qtemp.q	= arrRelPose[r].q*(!pSrcRot[r]*qtemp.q);     //additive blend
				//	qtemp.t = m_pPoseData->m_jointsRelative[j].t;
				int32 p	= pAimIK_Rot[r].m_nPosIndex;
				if (p>-1)
					qtemp.t = pSrcPos[p];
			}
			jointsAbsolute[j]	= jointsAbsolute[pidx] * qtemp;
		}
	}
	Quat qMiddle = jointsAbsolute[widx].q*rGAH.m_MiddleAimPoseRot;


	/*
	eIM_RClavicleIdx,
	eIM_RUpperArmIdx,
	eIM_RForeArmIdx,
	eIM_RHandIdx,

	eIM_LClavicleIdx,
	eIM_LUpperArmIdx,
	eIM_LForeArmIdx,
	*/

	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"vAimIKTarget: %f %f %f",vAimIKTarget.x,vAimIKTarget.y,vAimIKTarget.z );	
	//	g_YLine+=16.0f;


	int16 const * __restrict pIdxArray = pModelSkeleton->m_IdxArray;

	Vec3 vRefPose=Vec3(0,0,1.6f);
	int32 nCenter= pIdxArray[eIM_RUpperArmIdx];
	if (nCenter>-1)
		vRefPose=jointsAbsolute[nCenter].t;

	Vec3 vAimVec = (vAimIKTarget-vRefPose).GetNormalized();
	Vec2 p;
	p.x = atan2f(-vAimVec.x,vAimVec.y);
	f32 s1,c1; sincos_tpl(p.x,&s1,&c1);
	p.y = -atan2f(vAimVec.z,-vAimVec.x*s1+vAimVec.y*c1);


	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"p: %f %f",p.x,p.y );	
	//g_YLine+=16.0f;

	Vec3 vAimIKTargetNormalized = vAimIKTarget.GetNormalized();
	if (vAimIKTargetNormalized.y<-0.707f)
	{
		if (vAimIKTargetNormalized.x<0 && p.x>+3.0f)
			p.x=+gf_PI;
		if (vAimIKTargetNormalized.x>0 && p.x<-3.0f)
			p.x=-gf_PI;
		if (vAimIKTargetNormalized.x>0 && p.x>3.0f)
			p.x=-p.x;
	}

	//SmoothCD(m_PolarCoordinatesSmooth, m_PolarCoordinatesRate, g_AverageFrameTime, m_PolarCoordinates, 0.11f);
	f32 st = (1-vAimIKTargetNormalized.y)*0.10f+0.02f;
	SmoothCD(m_PolarCoordinatesSmooth, m_PolarCoordinatesRate, g_AverageFrameTime, p, st);
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"st: %f",st);	
	//	g_YLine+=16.0f;


	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"p: %f %f",p.x,p.y );	
	//g_YLine+=16.0f;
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"PolarCoordinates: %f %f   vAimIKTarget_y  %f",m_PolarCoordinates.x,m_PolarCoordinates.y, vAimIKTarget.y );	
	//g_YLine+=16.0f;
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"m_PolarCoordinatesSmooth: %f %f",m_PolarCoordinatesSmooth.x,m_PolarCoordinatesSmooth.y );	
	//g_YLine+=16.0f;
	f32 sz,cz; sincos_tpl(m_PolarCoordinatesSmooth.x,&sz,&cz); //YAW
	f32 sx,cx; sincos_tpl(m_PolarCoordinatesSmooth.y,&sx,&cx); //PITCH

	Vec3 vdir1 = Vec3(-sz*cx,cz*cx,-sx); //calculate the view-direction
	Vec3 vdir2 = vdir1*qMiddle; //calculate the view-direction

	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"vdir1: %f %f %f",vdir1.x,vdir1.y,vdir1.z );	
	//	g_YLine+=16.0f;
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"vdir2: %f %f %f",vdir2.x,vdir2.y,vdir2.z );	
	//	g_YLine+=16.0f;

	Vec2 InputPolar;
	InputPolar.x = atan2f(-vdir2.x,vdir2.y);
	f32 s,c; sincos_tpl(InputPolar.x,&s,&c);
	InputPolar.y = -atan2f(vdir2.z,-vdir2.x*s+vdir2.y*c);
	if (m_PolarCoordinatesSmooth.x<-1.57f && InputPolar.x>0)
		InputPolar.x=-3.14f;
	if (m_PolarCoordinatesSmooth.x>+1.57f && InputPolar.x<0)
		InputPolar.x=+3.14f;

	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"InputPolar: %f %f",InputPolar.x,InputPolar.y );	
	//g_YLine+=16.0f;
	//if (InputPolar.x<-2.55f) InputPolar.x=-2.55f;
	//if (InputPolar.x> 2.55f) InputPolar.x= 2.55f;
	if (InputPolar.y<-1.40f) InputPolar.y=-1.35f;
	if (InputPolar.y> 1.40f) InputPolar.y= 1.35f;

#if !defined(PS3)		
	if (Console::GetInst().ca_DrawAimIKVEGrid)
	{
		static Ang3 ang3 = Ang3(0,0,0);
		ang3+=Ang3(0.01f,0.02f,0.13f);
		AABB aabb=AABB( Vec3(-0.02f,-0.02f,-0.02f), Vec3(+0.02f,+0.02f,+0.02f));
		OBB obb; obb.SetOBBfromAABB(Matrix33::CreateRotationXYZ(ang3),aabb);
		g_pAuxGeom->DrawOBB(obb,m33*Vec3(InputPolar),1,RGBA8(0xff,0x3f,0x3f,0x00),eBBD_Extremes_Color_Encoded);
	}
#endif


	uint8 idx,i0,i1,i2,i3;
	f32 fx=(InputPolar.x+gf_PI  )/(gf_PI/8);
	f32 fy=(InputPolar.y+gf_PI/2)/(gf_PI/8);
	int32 ix=int32(fx);
	int32 iy=int32(fy);
	f32 px=fx-f32(ix);
	f32 py=fy-f32(iy);

	if (ix<0)	 ix=0;
	if (ix>15) ix=15;
	if (iy<0)	 iy=0;
	if (iy>7)	 iy=7;

	f32 weights[256];
	for (uint32 i=0; i<numAimPosesCAF; i++) weights[i]=0;

	CHUNK_GAHAIM_INFO::VirtualExample *pPolarGrid = &rGAH.m_PolarGrid[0];
	CHUNK_GAHAIM_INFO::VirtualExample example;

	f32 ew0=(1-px)*(1-py); 
	idx=(iy+0)*CHUNK_GAHAIM_INFO::XGRID+(ix+0);
	example = pPolarGrid[idx];
	i0		= example.i0;	if (i0==0xff) return;
	i1		= example.i1;	if (i1==0xff) return;
	i2		= example.i2;	if (i2==0xff) return;
	i3		= example.i3;	if (i3==0xff) return;
	weights[i0]+=f32(example.v0)/f32(0x2000)*ew0;	
	weights[i1]+=f32(example.v1)/f32(0x2000)*ew0;	
	weights[i2]+=f32(example.v2)/f32(0x2000)*ew0;	
	weights[i3]+=f32(example.v3)/f32(0x2000)*ew0;	

		
	f32 ew1=px*(1-py);
	idx=(iy+0)*CHUNK_GAHAIM_INFO::XGRID+(ix+1);
	example = pPolarGrid[idx];
	i0		= example.i0;	if (i0==0xff) return;
	i1		= example.i1;	if (i1==0xff) return;
	i2		= example.i2;	if (i2==0xff) return;
	i3		= example.i3;	if (i3==0xff) return;
	weights[i0]+=f32(example.v0)/f32(0x2000)*ew1;
	weights[i1]+=f32(example.v1)/f32(0x2000)*ew1;
	weights[i2]+=f32(example.v2)/f32(0x2000)*ew1;
	weights[i3]+=f32(example.v3)/f32(0x2000)*ew1;

	f32 ew3=(1-px)*py;		 
	idx=(iy+1)*CHUNK_GAHAIM_INFO::XGRID+(ix+0);
	example = pPolarGrid[idx];
	i0		= example.i0;	if (i0==0xff) return;
	i1		= example.i1;	if (i1==0xff) return;
	i2		= example.i2;	if (i2==0xff) return;
	i3		= example.i3;	if (i3==0xff) return;
	weights[i0]+= f32(example.v0)/f32(0x2000)*ew3;	
	weights[i1]+= f32(example.v1)/f32(0x2000)*ew3;	
	weights[i2]+= f32(example.v2)/f32(0x2000)*ew3;	
	weights[i3]+= f32(example.v3)/f32(0x2000)*ew3;	

	f32 ew2=px*py;
	idx=(iy+1)*CHUNK_GAHAIM_INFO::XGRID+(ix+1);
	example = pPolarGrid[idx];
	i0		= example.i0;	if (i0==0xff) return;
	i1		= example.i1;	if (i1==0xff) return;
	i2		= example.i2;	if (i2==0xff) return;
	i3		= example.i3;	if (i3==0xff) return;
	weights[i0]+= f32(example.v0)/f32(0x2000)*ew2;
	weights[i1]+= f32(example.v1)/f32(0x2000)*ew2;
	weights[i2]+= f32(example.v2)/f32(0x2000)*ew2;
	weights[i3]+= f32(example.v3)/f32(0x2000)*ew2;

	f32 sum=0.0f;	for(uint32 i=0; i<numAimPosesCAF; i++) sum+=weights[i];
	assert( fabs(sum-1.0f)<0.001 );
	
	for (uint32 i=0; i<numAimPosesCAF; i++)
	{
		const Quat *arrRotations = &rGAH.m_arrAimIKPosesAIM[i].m_arrRotation[0];
		const Vec3 *arrPostions = &rGAH.m_arrAimIKPosesAIM[i].m_arrPosition[0];
		f32 w=weights[i]*fIKBlend;
		if (w)
		{
			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"Index: %d  weights: %f",i,weights[i] );	
			//	g_YLine+=16.0f;
			for (uint32 r=0; r<numRotJoints; r++)
			{
				int32 j	= pAimIK_Rot[r].m_nJointIdx;
				qtemp = arrRelPose[r];
				/*
				if (i==0)
				{
				g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"exist: %d  joint-name: %s",(rGAH.m_nExist&uint64(1<<r))!=0,pAimIK_Rot[r].m_strJointName );	
				g_YLine+=16.0f;
				}*/

				if (rGAH.m_nExist&uint64(1<<r))
				{
					int32 a	= pAimIK_Rot[r].m_nAdditive;
					qtemp.q = arrRotations[r];
					if (a) qtemp.q = arrRelPose[r].q*(! pSrcRot[r]*qtemp.q);     //additive blend
					int32 p	= pAimIK_Rot[r].m_nPosIndex;
					if (p>-1)
						qtemp.t = arrPostions[p];
				}
				jointsRelative[j].q += qtemp.q*w;
				jointsRelative[j].t += qtemp.t*w;
			}
		}
	}

}





#if !defined(PS3)		
void CPoseBlenderAim::DebugVEGrid(CCharInstance* pInstance, SAimInfo& rAimInfo,f32 fIKBlend,int32 widx)
{
	Matrix33 m33; m33.SetRotationX(-gf_PI/2);
	m33.m00=-m33.m00;	
	m33.m01=-m33.m01; 
	m33.m02=-m33.m02;


	for (f32 x=-gf_PI; x<=gf_PI+0.001f; x+=gf_PI/8 )
		g_pAuxGeom->DrawLine( m33*Vec3(x,-gf_PI/2,-0.001f),RGBA8(0x1f,0x0f,0x7f,0xff), m33*Vec3(x,gf_PI/2,-0.001f), RGBA8(0x1f,0x0f,0x7f,0xff) );
	for (f32 y=-gf_PI/2; y<=gf_PI/2+0.001f; y+=gf_PI/8 )
		g_pAuxGeom->DrawLine( m33*Vec3(-gf_PI,  y,-0.001f),RGBA8(0x1f,0x0f,0x7f,0xff), m33*Vec3(gf_PI,  y,-0.001f), RGBA8(0x1f,0x0f,0x7f,0xff) );

	CModelSkeleton* pModelSkeleton =	&pInstance->m_pModel->m_ModelSkeleton;
	uint32 numJoints = pModelSkeleton->m_poseData.GetJointCount();
	const CModelJoint* parrModelJoints = &pModelSkeleton->m_arrModelJoints[0];
	CAnimationSet* pAnimationSet =		&pInstance->m_pModel->m_AnimationSet;

	GlobalAnimationHeaderAIM& rGAH0 = g_AnimationManager.m_arrGlobalAIM[rAimInfo.m_nGlobalAimID0];

	QuadIndices arrQuat[255];
	uint32 numAimPoses=uint32(rGAH0.m_fTotalDuration/SECONDS_PER_TICK+1.1f);
	uint32 numQuats = pAnimationSet->AnnotateExamples(numAimPoses, arrQuat);


	//--------------------------------------------------------------------------------
	//----    debug stuff
	//--------------------------------------------------------------------------------
	static Ang3 ang3 = Ang3(0,0,0);
	ang3+=Ang3(0.01f,0.02f,0.13f);
	AABB aabb1=AABB( Vec3(-0.01f,-0.01f,-0.01f), Vec3(+0.01f,+0.01f,+0.01f));
	AABB aabb2=AABB( Vec3(-0.02f,-0.02f,-0.02f), Vec3(+0.02f,+0.02f,+0.02f));
	OBB obb1; obb1.SetOBBfromAABB(Matrix33::CreateRotationXYZ(ang3),aabb1);
	OBB obb2; obb2.SetOBBfromAABB(Matrix33::CreateRotationXYZ(ang3),aabb2);

	QuatT arrRelPose0[256];
	QuatT arrAbsPose0[256];
	cryMemcpy( arrRelPose0, &pModelSkeleton->m_poseData.m_jointsRelative[0], numJoints*sizeof(QuatT) );
	for (uint32 i=0; i<numJoints; i++) arrAbsPose0[i]=pModelSkeleton->m_poseData.m_jointsAbsolute[i];

	QuatT arrRelPose1[256];
	QuatT arrAbsPose1[256];
	cryMemcpy( arrRelPose1, &pModelSkeleton->m_poseData.m_jointsRelative[0], numJoints*sizeof(QuatT) );
	for (uint32 i=0; i<numJoints; i++) arrAbsPose1[i]=pModelSkeleton->m_poseData.m_jointsAbsolute[i];

	QuatT arrRelPose2[256];
	QuatT arrAbsPose2[256];
	cryMemcpy( arrRelPose2, &pModelSkeleton->m_poseData.m_jointsRelative[0], numJoints*sizeof(QuatT) );
	for (uint32 i=0; i<numJoints; i++) arrAbsPose2[i]=pModelSkeleton->m_poseData.m_jointsAbsolute[i];

	QuatT arrRelPose3[256];
	QuatT arrAbsPose3[256];
	cryMemcpy( arrRelPose3, &pModelSkeleton->m_poseData.m_jointsRelative[0], numJoints*sizeof(QuatT) );
	for (uint32 i=0; i<numJoints; i++) arrAbsPose3[i]=pModelSkeleton->m_poseData.m_jointsAbsolute[i];

	QuatT arrRelPose[256];
	QuatT arrAbsPose[256];
	cryMemcpy( arrRelPose, &pModelSkeleton->m_poseData.m_jointsRelative[0], numJoints*sizeof(QuatT) );
	for (uint32 i=0; i<numJoints; i++) arrAbsPose[i]=pModelSkeleton->m_poseData.m_jointsAbsolute[i];

	static Ang3 ang = Ang3(0,0,0);
	ang+=Ang3(0.1f,0.2f,0.03f);
	AABB aabb=AABB( Vec3(-0.01f,-0.01f,-0.01f), Vec3(+0.01f,+0.01f,+0.01f));
	OBB obb; obb.SetOBBfromAABB(Matrix33::CreateRotationXYZ(ang),aabb);

	ColorB tricol = RGBA8(0x00,0xff,0x00,0xff);

	float fTextColor[4] = {1,0,0,1};
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fTextColor, false,"rGAH0.m_MiddleAimPose: %f %f %f %f",rGAH0.m_MiddleAimPose.w,  rGAH0.m_MiddleAimPose.v.x,rGAH0.m_MiddleAimPose.v.y,rGAH0.m_MiddleAimPose.v.z );	
	g_YLine+=14.0f;

	for (uint32 aq=0; aq<numQuats; aq++)
	{
		int8 i0=arrQuat[aq].i0;
		int8 i1=arrQuat[aq].i1;
		int8 i2=arrQuat[aq].i2;
		int8 i3=arrQuat[aq].i3;

		pAnimationSet->Blend4AimPose( rGAH0, pModelSkeleton, i0,i1,i2,i3, arrQuat[aq].w0,  arrRelPose0,arrAbsPose0 );
		Quat qt0=rGAH0.m_MiddleAimPose*arrAbsPose0[widx].q;
		Vec2 polar0=Vec2(pAnimationSet->PolarCoordinate(qt0));
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fTextColor, false,"polar0: %f %f",polar0.x,polar0.y );	
		g_YLine+=14.0f;


		pAnimationSet->Blend4AimPose( rGAH0, pModelSkeleton, i0,i1,i2,i3, arrQuat[aq].w1,  arrRelPose1,arrAbsPose1  );
		Quat qt1=rGAH0.m_MiddleAimPose*arrAbsPose1[widx].q;
		Vec2 polar1=Vec2(pAnimationSet->PolarCoordinate(qt1));
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fTextColor, false,"polar1: %f %f",polar1.x,polar1.y );	
		g_YLine+=14.0f;

		pAnimationSet->Blend4AimPose( rGAH0, pModelSkeleton, i0,i1,i2,i3, arrQuat[aq].w2,  arrRelPose2,arrAbsPose2 );
		Quat qt2=rGAH0.m_MiddleAimPose*arrAbsPose2[widx].q;
		Vec2 polar2=Vec2(pAnimationSet->PolarCoordinate(qt2));
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fTextColor, false,"polar2: %f %f",polar2.x,polar2.y );	
		g_YLine+=14.0f;

		pAnimationSet->Blend4AimPose( rGAH0, pModelSkeleton, i0,i1,i2,i3, arrQuat[aq].w3,  arrRelPose3,arrAbsPose3  );
		Quat qt3=rGAH0.m_MiddleAimPose*arrAbsPose3[widx].q;
		Vec2 polar3=Vec2(pAnimationSet->PolarCoordinate(qt3));
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.2f, fTextColor, false,"polar3: %f %f",polar3.x,polar3.y );	
		g_YLine+=24.0f;

		f32 t=1.0;
		f32 angle0	=	acos_tpl(qt0|qt1);
		f32 step0		=	1.0f/(angle0*30.0f);
		Vec2 p0 = polar0;
		g_pAuxGeom->DrawOBB(obb,m33*p0,1,RGBA8(0x00,0xff,0x00,0x00),eBBD_Extremes_Color_Encoded);
		for (f32 i=step0; i<3.0f; i+=step0)
		{
			t=i;	if (i>0.999) t=1.0;
			pAnimationSet->NLerp2AimPose( pModelSkeleton, arrRelPose0,arrRelPose1,t,arrAbsPose);
			Quat qt = rGAH0.m_MiddleAimPose*arrAbsPose[widx].q;
			Vec2 p1 = Vec2(pAnimationSet->PolarCoordinate(qt));
			g_pAuxGeom->DrawLine( m33*(p0+arrQuat[aq].height),arrQuat[aq].col, m33*(p1+arrQuat[aq].height),arrQuat[aq].col );
			p0=p1;
			if (t==1.0)
				break;
		}

		f32 angle1	=	acos_tpl(qt1|qt2);
		f32 step1		=	1.0f/(angle1*30.0f);
		g_pAuxGeom->DrawOBB(obb,m33*p0,1,RGBA8(0x00,0xff,0x00,0x00),eBBD_Extremes_Color_Encoded);
		for (f32 i=step1; i<3.0f; i+=step1)
		{
			t=i;	if (i>0.999) t=1.0;
			pAnimationSet->NLerp2AimPose( pModelSkeleton, arrRelPose1,arrRelPose2,t,arrAbsPose);
			Quat qt = rGAH0.m_MiddleAimPose*arrAbsPose[widx].q;
			Vec2 p1 = Vec2( pAnimationSet->PolarCoordinate(qt) );
			g_pAuxGeom->DrawLine( m33*(p0+arrQuat[aq].height),arrQuat[aq].col, m33*(p1+arrQuat[aq].height),arrQuat[aq].col );
			p0=p1;
			if (t==1.0)
				break;
		}

		f32 angle2	=	acos_tpl(qt2|qt3);
		f32 step2		=	1.0f/(angle2*30.0f);
		g_pAuxGeom->DrawOBB(obb,m33*p0,1,RGBA8(0x00,0xff,0x00,0x00),eBBD_Extremes_Color_Encoded);
		for (f32 i=step2; i<3.0f; i+=step2)
		{
			t=i; if (i>0.999) t=1.0;
			pAnimationSet->NLerp2AimPose( pModelSkeleton, arrRelPose2,arrRelPose3,t,arrAbsPose);
			Quat qt = rGAH0.m_MiddleAimPose*arrAbsPose[widx].q;
			Vec2 p1 = Vec2( pAnimationSet->PolarCoordinate(qt) );
			g_pAuxGeom->DrawLine( m33*(p0+arrQuat[aq].height),arrQuat[aq].col, m33*(p1+arrQuat[aq].height),arrQuat[aq].col );
			p0=p1;
			if (t==1.0)
				break;
		}


		f32 angle3	=	acos_tpl(qt3|qt0);
		f32 step3		=	1.0f/(angle3*30.0f);
		g_pAuxGeom->DrawOBB(obb,m33*p0,1,RGBA8(0x00,0xff,0x00,0x00),eBBD_Extremes_Color_Encoded);
		for (f32 i=step3; i<3.0f; i+=step3)
		{
			t=i;	if (i>0.999) t=1.0;
			pAnimationSet->NLerp2AimPose( pModelSkeleton, arrRelPose3,arrRelPose0,t,arrAbsPose);
			Quat qt = rGAH0.m_MiddleAimPose*arrAbsPose[widx].q;
			Vec2 p1 = Vec2( pAnimationSet->PolarCoordinate(qt) );
			g_pAuxGeom->DrawLine( m33*(p0+arrQuat[aq].height),arrQuat[aq].col, m33*(p1+arrQuat[aq].height),arrQuat[aq].col );
			p0=p1;
			if (t==1.0)
				break;
		}
	}

	for (int32 y=0; y<CHUNK_GAHAIM_INFO::YGRID; y++)
	{
		for (int32 x=0; x<CHUNK_GAHAIM_INFO::XGRID; x++)
		{
			//	if (rGAH0.m_PolarGrid[y*XGRID+x].m_fSmalest<0.2f)
			{
				int i0 = rGAH0.m_PolarGrid[y*CHUNK_GAHAIM_INFO::XGRID+x].i0;
				int i1 = rGAH0.m_PolarGrid[y*CHUNK_GAHAIM_INFO::XGRID+x].i1;
				int i2 = rGAH0.m_PolarGrid[y*CHUNK_GAHAIM_INFO::XGRID+x].i2;
				int i3 = rGAH0.m_PolarGrid[y*CHUNK_GAHAIM_INFO::XGRID+x].i3;
				Vec4 w;
				w.x = f32(rGAH0.m_PolarGrid[y*CHUNK_GAHAIM_INFO::XGRID+x].v0)/f32(0x2000);
				w.y = f32(rGAH0.m_PolarGrid[y*CHUNK_GAHAIM_INFO::XGRID+x].v1)/f32(0x2000);
				w.z = f32(rGAH0.m_PolarGrid[y*CHUNK_GAHAIM_INFO::XGRID+x].v2)/f32(0x2000);
				w.w = f32(rGAH0.m_PolarGrid[y*CHUNK_GAHAIM_INFO::XGRID+x].v3)/f32(0x2000);
				pAnimationSet->Blend4AimPose( rGAH0, pModelSkeleton, i0,i1,i2,i3, w, arrRelPose,arrAbsPose );
				Vec3 vPolarCoord=Vec2(pAnimationSet->PolarCoordinate(rGAH0.m_MiddleAimPose*arrAbsPose[widx].q));
				g_pAuxGeom->DrawOBB(obb1,m33*vPolarCoord,1,RGBA8(0xff,0xff,0xff,0x00),eBBD_Extremes_Color_Encoded);
			}
		}
	}

	size_t nHowManyVE = rGAH0.VE2.size();
	for (uint32 i=0; i<nHowManyVE; i++)
	{
		int32 i0	=rGAH0.VE2[i].i0;
		int32 i1	=rGAH0.VE2[i].i1;
		int32 i2	=rGAH0.VE2[i].i2;
		int32 i3	=rGAH0.VE2[i].i3;
		Vec2 polcor;
		polcor.x=rGAH0.VE2[i].polar.x;
		polcor.y=rGAH0.VE2[i].polar.y;

		Vec4 w;
		w.x		=rGAH0.VE2[i].w0;
		w.y		=rGAH0.VE2[i].w1;
		w.z		=rGAH0.VE2[i].w2;
		w.w		=rGAH0.VE2[i].w3;
		pAnimationSet->Blend4AimPose( rGAH0, pModelSkeleton, i0,i1,i2,i3, w, arrRelPose,arrAbsPose );
		Vec3 vPolarCoord=Vec2(pAnimationSet->PolarCoordinate(rGAH0.m_MiddleAimPose*arrAbsPose[widx].q));

		assert(fabs(polcor.x-polcor.x)<0.00001);
		assert(fabs(polcor.y-polcor.y)<0.00001);
		g_pAuxGeom->DrawOBB(obb1,m33*vPolarCoord,1,RGBA8(0x00,0xff,0xff,0x00),eBBD_Extremes_Color_Encoded);
	}
}
#endif

#endif



#if (NEWAIMSYSTEM==0)
struct DBW3
{
	uint32	m_nAnimTokenCRC32; 
	int32		m_nParaJointIdx; 
	int32		m_nStartJointIdx;
	f32			m_fWeight;
	DBW3() {
		m_nAnimTokenCRC32=0; 
		m_nParaJointIdx=-1; 
		m_nStartJointIdx=-1;
		m_fWeight=0;
	}
};
#endif
