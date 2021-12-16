#include "stdafx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "../CharacterInstance.h"
#include "../Model.h"
#include "../ModelSkeleton.h"
#include "../CharacterManager.h"

#include "../FacialAnimation/FacialInstance.h"

#include "PoseModifierHelper.h"
#include "LookAt.h"

CRYREGISTER_CLASS(CLookAt)

CLookAt::CLookAt()
{
}

CLookAt::~CLookAt()
{
}


// IAnimationPoseModifier

SPU_INDIRECT(CommandBufferExecute(ML))
bool CLookAt::Execute(const SAnimationPoseModiferParams& params)
{
	// transfer this to stack on SPU, to reduce lookups to member variables
	SpuStackValue<CLookAt,true,true> stackObj( *this );
	return stackObj->ExecuteLocal(params);
}

bool CLookAt::ExecuteLocal(const SAnimationPoseModiferParams& params)
{
	ICharacterModelSkeleton* pCharacterModelSkeleton = SPU_MAIN_PTR( params.GetICharacterModelSkeleton() );
	ISkeletonPose* pSkeletonPose = SPU_MAIN_PTR( PoseModifierHelper::GetSkeletonPose(params) );

	uint32 LIKFlag = m_UseLookIK;

//	const QuatT& rPhysLocationNext = params.locationNextPhysics;
	const QuatT& rPhysLocationNext = QuatT(params.locationNextAnimation);

	CCharInstance* m_pInstance = PoseModifierHelper::GetCharInstance(params);
	CSkeletonAnim* m_pSkeletonAnim = PoseModifierHelper::GetSkeletonAnim(params);
	CModelSkeleton* m_pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);

#define BINOUT (1.0f/0.6f)

	if ( (m_pSkeletonAnim->m_IsAnimPlaying&0x1)==0)
		return false;

	if (PoseModifierHelper::GetSkeletonPose(params)->m_bInstanceVisible==0)
		return false;

	// use local pointer so spare dereferencing
	int16* 		const __restrict pIdxArray 			= m_pModelSkeleton->m_IdxArray;
	
	QuatT* 		const	__restrict	pRelativePose = SPU_LOCAL_PTR( params.pPoseRelative );
	QuatT* 		const	__restrict	pAbsolutePose = SPU_LOCAL_PTR( params.pPoseAbsolute );	
	SLookIK 										&rLookIK			= *this;
	
	pAbsolutePose[0] = pRelativePose[0];
	uint32 numJoints	= params.jointCount;
	for (uint32 i=1; i<numJoints; i++)
		pAbsolutePose[i]	= pAbsolutePose[m_pModelSkeleton->GetJointParentIDByID(i)] * pRelativePose[i];



	Vec3 vLocalLookAtTarget = (rLookIK.m_LookIKTargetSmooth-rPhysLocationNext.t);

	if( Console::GetInst().ca_UseLookIK==0 || pSkeletonPose->StandingUp()>=0)
		return false;

	int32 Spine1Idx	=	pIdxArray[eIM_Spine1Idx];
	int32 Spine2Idx	=	pIdxArray[eIM_Spine2Idx];
	int32 Spine3Idx	=	pIdxArray[eIM_Spine3Idx];
	int32 NeckIdx		=	pIdxArray[eIM_NeckIdx];
	int32 NeckIdx1	=	pIdxArray[eIM_NeckIdx1];
	int32 HeadIdx		= pIdxArray[eIM_HeadIdx];

	int32 ridx			= pIdxArray[eIM_RightEyeIdx];
	int32 lidx			= pIdxArray[eIM_LeftEyeIdx];


	//	if (Spine1Idx<0)	return;
	//	if (Spine2Idx<0)	return;
	//	if (Spine3Idx<0)	return;
	if (NeckIdx<0)		return false;
	if (HeadIdx<0)		return false;
	if (ridx<0)				return false;
	if (lidx<0)				return false;

	QuatT* pRelativeQuatIK = SPU_LOCAL_PTR( (QuatT*)alloca( numJoints*sizeof(QuatT) ) );


#ifdef _DEBUG
	for (uint32 i=0; i<numJoints; i++)
	{
		QuatT qt = pAbsolutePose[i];
		assert(qt.IsValid());
	}
#endif

	//------------------------------------------------------------------------
	//---    use the attachment position of the eye for precise Look-IK    ---
	//------------------------------------------------------------------------

	IAttachmentManager* pIAttachmentManager = m_pInstance->CCharInstance::GetIAttachmentManager();   

	CAttachment* pAttachmentL = 0;
	CAttachment* pAttachmentR = 0;
	QuatT ql=QuatT(IDENTITY);
	QuatT qr=QuatT(IDENTITY);

	int32 alidx = pIAttachmentManager->GetIndexByName("eye_left");
	int32 aridx = pIAttachmentManager->GetIndexByName("eye_right");
	if (alidx>=0 && aridx>=0 )
	{
		pAttachmentL = SPU_MAIN_PTR( (CAttachment*)pIAttachmentManager->GetInterfaceByIndex(alidx) );
		pAttachmentR = SPU_MAIN_PTR( (CAttachment*)pIAttachmentManager->GetInterfaceByIndex(aridx) );
		pAttachmentL->m_additionalRotation.SetIdentity();
		pAttachmentR->m_additionalRotation.SetIdentity();
		ql = QuatT(pAttachmentL->CAttachment::GetAttRelativeDefault());
		qr = QuatT(pAttachmentR->CAttachment::GetAttRelativeDefault());
	}

	if (pAttachmentL==0)
		return false;
	if (pAttachmentR==0)
		return false;

	pAttachmentL->m_additionalRotation.SetIdentity();
	pAttachmentR->m_additionalRotation.SetIdentity();


	//-------------------------------------------------------------------------
#if !defined(__SPU__)
	//SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
	//g_pAuxGeom->SetRenderFlags( renderFlags );
	//const char* mname = m_pInstance->m_pModel->GetFilePathCStr();
	//if ( strcmp(mname,"objects/characters/human/us/officer/officer.chr")==0 )
//	{
//	float fColor[4] = {1,1,0,1};
//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"LookIK: %d   %12.8f    Target:(%12.8f %12.8f %12.8f) ",m_UseLookIK,m_LookIKFOR, LookIKTarget.x, LookIKTarget.y, LookIKTarget.z );	g_YLine+=16.0f;
//	} 
#endif

	Vec3 v1		=	pAbsolutePose[NeckIdx  ].q.GetColumn1();
	Vec3 v2		=	pAbsolutePose[HeadIdx  ].q.GetColumn1();
	Vec3 r0=(v1+v2).GetNormalized();
	Vec3 r1=r0;		r1.z=0; r1.Normalize(); 
	Vec3 PivotAxisX= rPhysLocationNext.q * (r0+4*r1).GetNormalized();


	Vec3 LocalLookAtTarget = rLookIK.m_LookIKTargetSmooth-rPhysLocationNext.t;
	Vec3 LeftEyePos		= (pAbsolutePose[lidx]*ql).t;
	Vec3 RightEyePos	= (pAbsolutePose[ridx]*qr).t;
	Vec3 MiddleEyePos	= (LeftEyePos+RightEyePos)*0.5f;
	
//	ColorF colorl( 1.0f, 0.0f, 0.0f, 1.0f );
//	g_pAuxGeom->DrawSphere(LeftEyePos, 0.003f, colorl);
//	ColorF colorr( 0.0f, 0.0f, 1.0f, 1.0f );
//	g_pAuxGeom->DrawSphere(RightEyePos, 0.003f, colorr);
//	ColorF colorg( 0.0f, 1.0f, 0.0f, 1.0f );
//	g_pAuxGeom->DrawSphere(MiddleEyePos, 0.003f, colorg);
//	return;

	Vec3 LookDir = (LocalLookAtTarget-MiddleEyePos).GetNormalized();

	f32 dot	=	PivotAxisX|LookDir;
	extern f32 g_YLine;
//	float fColor[4] = {1,1,0,1};
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"dot: %12.8f", dot );	g_YLine+=16.0f;

	f32 FOR = cosf(rLookIK.m_LookIKFOR);
	if (dot<FOR || rLookIK.m_LookIKFadeout)
	{
		//avoid Linda-Blair effect
		rLookIK.m_LookIKFadeout=1;
		rLookIK.m_LookIKBlend-=BINOUT*fabsf(m_pInstance->m_fOriginalDeltaTime);
		if (rLookIK.m_LookIKBlend<0.0f)
		{
			rLookIK.m_LookIKFadeout=0;
			rLookIK.m_LookIKBlend=0.0f;
			return false;
		}
	}
	else
	{
		if (LIKFlag)
		{
			rLookIK.m_LookIKBlend+=BINOUT*fabsf(m_pInstance->m_fOriginalDeltaTime);
			if (rLookIK.m_LookIKBlend>1.0f)	rLookIK.m_LookIKBlend=1.0f;
			assert(rLookIK.m_LookIKFadeout==0);
		}
		else
		{
			rLookIK.m_LookIKBlend-=BINOUT*fabsf(m_pInstance->m_fOriginalDeltaTime);
			if (rLookIK.m_LookIKBlend<0.0f)	rLookIK.m_LookIKBlend=0.0f;
			assert(rLookIK.m_LookIKFadeout==0);
		}
	}

/*	
	extern f32 g_YLine;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"m_LookIKBlend: %12.8f", m_LookIKBlend );	g_YLine+=16.0f;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"Model: %s ",m_pInstance->GetModelFilePath());	g_YLine+=16.0f;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"Target:(%12.8f %12.8f %12.8f) ",m_LookIKTarget.x, m_LookIKTarget.y, m_LookIKTarget.z );	g_YLine+=16.0f;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"LookIK_FOR: %12.8f    SmoothTarget:(%12.8f %12.8f %12.8f) ",m_LookIKFOR, m_LookIKTargetSmooth.x, m_LookIKTargetSmooth.y, m_LookIKTargetSmooth.z );	g_YLine+=16.0f;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"m_fOriginalDeltaTime: %12.8f",m_pInstance->m_fOriginalDeltaTime );	g_YLine+=16.0f;
*/
	
	for (uint32 i=0; i<numJoints; i++)
		pRelativeQuatIK[i] = pRelativePose[i];

//	Matrix34 rRenderMat34 = Matrix34(Matrix33(IDENTITY),rPhysLocationNext.t);
//	pSkeletonPose->DrawSkeleton( rRenderMat34, 0 );

	Vec3 CamPos	=  LocalLookAtTarget * rPhysLocationNext.q;
	Vec3 vLookAt	= (CamPos-MiddleEyePos).GetNormalized();


#if !defined(__SPU__)
	if (Console::GetInst().ca_DrawLookIK)
	{
		IRenderAuxGeom * pAux = gEnv->pRenderer->GetIRenderAuxGeom();
		ColorF color( 1.0f, 0.0f, 0.0f, rLookIK.m_LookIKBlend );
		pAux->DrawLine(rPhysLocationNext*MiddleEyePos, color, rLookIK.m_LookIKTargetSmooth, color);

		pAux->DrawSphere(rLookIK.m_LookIKTargetSmooth, 0.3f, color);
	//	pAux->DrawSphere(CamPos+rPhysLocationNext.t, 0.3f, color);

		gEnv->pRenderer->DrawLabel
			( 
			rLookIK.m_LookIKTargetSmooth + Vec3(0,0,0.4f), 1.0f, "t:(%.1f %.1f %.1f)\nm:(%.1f %.1f %.1f)\nl:(%.1f %.1f %.1f)",
			rPhysLocationNext.t.x,
			rPhysLocationNext.t.y,
			rPhysLocationNext.t.z,
			rLookIK.m_LookIKTargetSmooth.x,
			rLookIK.m_LookIKTargetSmooth.y,
			rLookIK.m_LookIKTargetSmooth.z,
			MiddleEyePos.x,
			MiddleEyePos.y,
			MiddleEyePos.z
			);
	}
#endif


	CFacialInstance *pFacialInstance= SPU_MAIN_PTR( (CFacialInstance *)(m_pInstance->CCharInstance::GetFacialInstance()) );
	if (pFacialInstance)
		pFacialInstance->CFacialInstance::ApplyProceduralFaceBehaviour(CamPos);

	//VDirIdentity is the matrix that is not changing the rotation of the head
	Quat defaultAbsHeadJoint = pSkeletonPose->GetDefaultAbsJointByID(HeadIdx).q;
	Vec3 headX=defaultAbsHeadJoint.GetColumn0();
	Vec3 headY=defaultAbsHeadJoint.GetColumn1();
	Vec3 headZ=defaultAbsHeadJoint.GetColumn2();
	Matrix33 VDirIdentity = Matrix33::CreateFromVectors( -headZ, headY, headX ); 

	Quat AlignmentQuat	 =	 !Quat(VDirIdentity) * defaultAbsHeadJoint;  

	Quat absHeadQuat = Quat::CreateRotationVDir(vLookAt) * AlignmentQuat;
	Quat relHeadQuat = !pAbsolutePose[HeadIdx].q * absHeadQuat ;
	relHeadQuat.NormalizeSafe();

//	float fColor[4] = {1,1,0,1};
//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"relHeadQuat: (%12.8f %12.8f %12.8f %12.8f) ",relHeadQuat.w,relHeadQuat.v.x, relHeadQuat.v.y, relHeadQuat.v.z );	g_YLine+=16.0f;
//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"pAbsolutePose[0]:  rot (%12.8f %12.8f %12.8f %12.8f) pos (%12.8f %12.8f %12.8f) ",pAbsolutePose[0].q.w,pAbsolutePose[0].q.v.x, pAbsolutePose[0].q.v.y, pAbsolutePose[0].q.v.z,    pAbsolutePose[0].t.x,pAbsolutePose[0].t.y,pAbsolutePose[0].t.z );	g_YLine+=16.0f;
//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"rPhysLocationNext:  rot (%12.8f %12.8f %12.8f %12.8f) ",rPhysLocationNext.q.w,rPhysLocationNext.q.v.x, rPhysLocationNext.q.v.y, rPhysLocationNext.q.v.z );	g_YLine+=16.0f;


	ComputeNewSpineAndHeadOrientation(params, rLookIK, pRelativeQuatIK, relHeadQuat);

	CalculateEyeLookDirection(params, ql, qr, pAttachmentL, pAttachmentR, rLookIK, CamPos, pRelativeQuatIK);

	//-------------------------------------------------------------------------------
	//---     blend with the FK-pose and calculate the new absolute pose         ---
	//-------------------------------------------------------------------------------
	f32 IKBlend = clamp(rLookIK.m_LookIKBlend,0.0f,1.0f);
	IKBlend -= 0.5f;
	IKBlend = 0.5f + IKBlend/(0.5f+2.0f*IKBlend*IKBlend);

	for (uint32 i=1; i<numJoints; i++)
	{
		Quat quat0 = pRelativePose[i].q;
		Quat quat1 = pRelativeQuatIK[i].q;
		pRelativePose[i].q.SetNlerp(quat0,quat1,IKBlend);

		int16 p = pCharacterModelSkeleton->GetJointParentIDByID(i);
		pAbsolutePose[i]	= pAbsolutePose[p] * pRelativePose[i];
		assert(pAbsolutePose[i].IsValid());
	}

	SPU_MAIN_REF(pAttachmentL->m_additionalRotation).SetNlerp(IDENTITY,pAttachmentL->m_additionalRotation,IKBlend);
	SPU_MAIN_REF(pAttachmentR->m_additionalRotation).SetNlerp(IDENTITY,pAttachmentR->m_additionalRotation,IKBlend);

	if (rLookIK.m_LookIKFadeout==0)
	{
		if (Spine1Idx>0)
			rLookIK.m_oldSpine1	=	pRelativeQuatIK[Spine1Idx].q;
		if (Spine2Idx>0)
			rLookIK.m_oldSpine2	=	pRelativeQuatIK[Spine2Idx].q;
		if (Spine3Idx>0)
			rLookIK.m_oldSpine3	=	pRelativeQuatIK[Spine3Idx].q;
		if (NeckIdx>0)
			rLookIK.m_oldNeck		=	pRelativeQuatIK[NeckIdx].q;;
		if (NeckIdx1>0)
			rLookIK.m_oldNeck1		=	pRelativeQuatIK[NeckIdx1].q;
		if (HeadIdx>0)
			rLookIK.m_oldHead		=	pRelativeQuatIK[HeadIdx].q;

	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"rLookIK.m_oldHead: %f (%f %f %f)",rLookIK.m_oldHead.w,rLookIK.m_oldHead.v.x,rLookIK.m_oldHead.v.y,rLookIK.m_oldHead.v.z );
	//	g_YLine+=16.0f;
	}

	return true;
}


//

namespace Util {

	SPU_NO_INLINE void GetJointID( EAnimationJointType type, int32 &rIndex, int32 &rParentIndex, ICharacterModelSkeleton* pCharacterModelSkeleton )
	{
		rIndex = pCharacterModelSkeleton->GetJointIDByType(type);		
		if (rIndex>0) 
			rParentIndex=pCharacterModelSkeleton->GetJointParentIDByID(rIndex);
	}

	SPU_NO_INLINE void Update( int32 nIndex, int32 nParentIndex, Quat &relHeadQuat, f32 fBlendSmooth, QuatT *pRelativePose, QuatT *pAbsolutePose )
	{
		if( nIndex < 0 )
			return;

		Quat lerp	= Quat::CreateNlerp( Quat(IDENTITY), relHeadQuat, fBlendSmooth );
		pRelativePose[nIndex].q *= lerp;
		pAbsolutePose[nIndex] = pAbsolutePose[nParentIndex] * pRelativePose[nIndex];			
	}

	SPU_NO_INLINE void FadeOut( int32 nIndex, int32 nParentIndex, Quat &rOldRotation, QuatT *pRelativePose, QuatT *pAbsolutePose)
	{
		if( nIndex < 0 )
			return;
		
		pRelativePose[nIndex].q	=	rOldRotation;
		pAbsolutePose[nIndex] = pAbsolutePose[nParentIndex] * pRelativePose[nIndex];			
	}
}

SPU_NO_INLINE void CLookAt::ComputeNewSpineAndHeadOrientation(const SAnimationPoseModiferParams& params, SLookIK &rLookIK, QuatT* pRelativeQuatIK, Quat &relHeadQuat)
{
	ICharacterModelSkeleton* pCharacterModelSkeleton = params.GetICharacterModelSkeleton();

	// use local pointer so spare dereferencing
	//	int16* 	const __restrict pIdxArray 			= m_pModelSkeleton->m_IdxArray;

	QuatT* 	const	__restrict pRelativePose = SPU_LOCAL_PTR( params.pPoseRelative );
	QuatT* 	const	__restrict pAbsolutePose = SPU_LOCAL_PTR( params.pPoseAbsolute );

	//	int32 Spine0Idx	=	pIdxArray[eIM_Spine0Idx];
	//	int32 pSpine0Idx=-1;	
	//	if (Spine0Idx>0) pSpine0Idx=m_parrModelJoints[Spine0Idx].m_idxParent;

	int32 Spine1Idx, pSpine1Idx=-1;	
	int32 Spine2Idx, pSpine2Idx=-1;
	int32 Spine3Idx, pSpine3Idx=-1;
	int32 NeckIdx, pNeckIdx=-1;
	int32 NeckIdx1, pNeckIdx1=-1;
	int32 HeadIdx, pHeadIdx=-1;

	Util::GetJointID( eIM_Spine1Idx, Spine1Idx, pSpine1Idx, pCharacterModelSkeleton );
	Util::GetJointID( eIM_Spine2Idx, Spine2Idx, pSpine2Idx, pCharacterModelSkeleton );
	Util::GetJointID( eIM_Spine3Idx, Spine3Idx, pSpine3Idx, pCharacterModelSkeleton );
	Util::GetJointID( eIM_NeckIdx, NeckIdx, pNeckIdx, pCharacterModelSkeleton );
	Util::GetJointID( eIM_NeckIdx1, NeckIdx1, pNeckIdx1, pCharacterModelSkeleton );
	Util::GetJointID( eIM_HeadIdx, HeadIdx, pHeadIdx, pCharacterModelSkeleton );								
	
			
	/*
	float fColor[4] = {1,1,0,1};
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"relHeadQuat: %12.8f - %12.8f %12.8f %12.8f ",relHeadQuat.w,relHeadQuat.v.x,relHeadQuat.v.y,relHeadQuat.v.z );	g_YLine+=16.0f;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"rLookIK.m_lookIKBlendsSmooth[0]: %12.8f",rLookIK.m_lookIKBlendsSmooth[0] );	g_YLine+=16.0f;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"rLookIK.m_lookIKBlendsSmooth[1]: %12.8f",rLookIK.m_lookIKBlendsSmooth[1] );	g_YLine+=16.0f;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"rLookIK.m_lookIKBlendsSmooth[2]: %12.8f",rLookIK.m_lookIKBlendsSmooth[2] );	g_YLine+=16.0f;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"rLookIK.m_lookIKBlendsSmooth[3]: %12.8f",rLookIK.m_lookIKBlendsSmooth[3] );	g_YLine+=16.0f;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"rLookIK.m_lookIKBlendsSmooth[4]: %12.8f",rLookIK.m_lookIKBlendsSmooth[4] );	g_YLine+=16.0f;
	*/

	//-----------------------------------------------------------------------
	//---           new Spline1 orientation      -------------------------------
	//-----------------------------------------------------------------------
	Util::Update( Spine1Idx, pSpine1Idx, relHeadQuat, rLookIK.m_lookIKBlendsSmooth[0], pRelativeQuatIK, pAbsolutePose );	

	//-----------------------------------------------------------------------
	//---           new Spline2 orientation      -------------------------------
	//-----------------------------------------------------------------------
	Util::Update( Spine2Idx, pSpine2Idx, relHeadQuat, rLookIK.m_lookIKBlendsSmooth[1], pRelativeQuatIK, pAbsolutePose );	

	//-----------------------------------------------------------------------
	//---           new Spline3 orientation      ----------------------------
	//-----------------------------------------------------------------------
	Util::Update( Spine3Idx, pSpine3Idx, relHeadQuat, rLookIK.m_lookIKBlendsSmooth[2], pRelativeQuatIK, pAbsolutePose );	
	
	//-----------------------------------------------------------------------
	//---           new Neck orientation      -------------------------------
	//-----------------------------------------------------------------------
	Util::Update( NeckIdx, pNeckIdx, relHeadQuat, rLookIK.m_lookIKBlendsSmooth[3], pRelativeQuatIK, pAbsolutePose );	

	//-----------------------------------------------------------------------
	//---           new Neck1 orientation      -------------------------------
	//-----------------------------------------------------------------------
	Util::Update( NeckIdx1, pNeckIdx1, relHeadQuat, rLookIK.m_lookIKBlendsSmooth[3] * 0.7f, pRelativeQuatIK, pAbsolutePose );	

	//apply additional relative head orientation (this is comming from the facial-anim system)
	pRelativePose[HeadIdx].q		= pRelativePose[HeadIdx].q *m_addHead;
	pRelativeQuatIK[HeadIdx].q	= pRelativeQuatIK[HeadIdx].q*m_addHead;

	//-----------------------------------------------------------------------
	//---           new Head orientation      -------------------------------
	//-----------------------------------------------------------------------
	Quat lerpHead		= Quat::CreateNlerp( Quat(IDENTITY), relHeadQuat, rLookIK.m_lookIKBlendsSmooth[4]/1.8f);
	pRelativeQuatIK[HeadIdx].q *= lerpHead;
	pAbsolutePose[HeadIdx] = pAbsolutePose[pHeadIdx] * pRelativeQuatIK[HeadIdx];
	pAbsolutePose[HeadIdx].q.NormalizeSafe();

	if (rLookIK.m_LookIKFadeout)
	{
		//we are in FadeOut-Mode. Ignore all Look-IK caculation we did for this frame and just take the last valid values
		Util::FadeOut( Spine1Idx, pSpine1Idx, rLookIK.m_oldSpine1, pRelativeQuatIK, pAbsolutePose );
		Util::FadeOut( Spine2Idx, pSpine2Idx, rLookIK.m_oldSpine2, pRelativeQuatIK, pAbsolutePose );
		Util::FadeOut( Spine3Idx, pSpine3Idx, rLookIK.m_oldSpine3, pRelativeQuatIK, pAbsolutePose );
		Util::FadeOut( NeckIdx, pNeckIdx, rLookIK.m_oldNeck, pRelativeQuatIK, pAbsolutePose );
		Util::FadeOut( NeckIdx1, pNeckIdx1, rLookIK.m_oldNeck1, pRelativeQuatIK, pAbsolutePose );
		Util::FadeOut( HeadIdx, pHeadIdx, rLookIK.m_oldHead, pRelativeQuatIK, pAbsolutePose );
	}

}

SPU_NO_INLINE void CLookAt::CalculateEyeLookDirection(const SAnimationPoseModiferParams& params, QuatT &ql, QuatT &qr, CAttachment* pAttachmentL, CAttachment* pAttachmentR, SLookIK &rLookIK, Vec3 &CamPos, QuatT* pRelativeQuatIK)
{
	ICharacterModelSkeleton* pCharacterModelSkeleton =	params.GetICharacterModelSkeleton();

	QuatT* const __restrict pRelativePose = SPU_PTR_SELECT(params.pPoseRelative, gSkeletonRelativePose);
	QuatT* const __restrict pAbsolutePose = SPU_PTR_SELECT(params.pPoseAbsolute, gSkeletonAbsolutePose);

	int32 HeadIdx = pCharacterModelSkeleton->GetJointIDByType(eIM_HeadIdx);
	int32 ridx = pCharacterModelSkeleton->GetJointIDByType(eIM_RightEyeIdx);
	int32 lidx = pCharacterModelSkeleton->GetJointIDByType(eIM_LeftEyeIdx);

	// NOTE: Quat::CreateRotationVDir() will assume an implicit up vector along
	// the Z axis. This will cause the eyes to rotate along their forward Y
	// axis whenever the PoseModifier is enabled.
	// In order to avoid this we rotate the Y axis back minus 45 degrees.
	// TODO: Better solution would be to provide a way to create a rotation out
	// of a direction and an arbitrary up axis.
	Quat rotYMinus45 = Quat::CreateRotationY(-gf_PI*0.5f);

	QuatT LAbsolutePose = (pAbsolutePose[HeadIdx] * pRelativePose[lidx]) * ql;
	rLookIK.m_oldEyeLeft = m_addLEye;
	Vec3 CamPosL = CamPos-LAbsolutePose.t;
	Quat absLEye = Quat::CreateRotationVDir(CamPosL.GetNormalized()) * rotYMinus45 * m_addLEye;
	pAttachmentL->m_additionalRotation = !pAbsolutePose[HeadIdx].q * absLEye;

	QuatT RAbsolutePose = (pAbsolutePose[HeadIdx] * pRelativePose[ridx]) * qr;
	rLookIK.m_oldEyeRight = m_addREye;
	Vec3 CamPosR = CamPos-RAbsolutePose.t;
	Quat absREye = Quat::CreateRotationVDir(CamPosR.GetNormalized()) * rotYMinus45 * m_addREye;
	pAttachmentR->m_additionalRotation = !pAbsolutePose[HeadIdx].q * absREye;	

}

