#include "StdAfx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "../CharacterInstance.h"
#include "../Model.h"
#include "../ModelSkeleton.h"
#include "../CharacterManager.h"

#include "../FacialAnimation/FacialInstance.h"

#include "PoseModifierHelper.h"
#include "PoseBlenderLook.h"
#include <CryExtension/CryCreateClassInstance.h>



CPoseBlenderLook& CSkeletonPose::GetPoseBlenderLook() 
{ 
	CPoseBlenderLook *pbLook = static_cast<CPoseBlenderLook*>(m_PoseBlenderLook.get());

	if (pbLook == NULL)
	{
		::CryCreateClassInstance<IAnimationPoseBlenderDir>("AnimationPoseModifier_PoseBlenderLook", m_PoseBlenderLook);
		pbLook = static_cast<CPoseBlenderLook*>(m_PoseBlenderLook.get());
		assert(pbLook);
		pbLook->m_blender.Init();
	}

	return *pbLook; 
}



CRYREGISTER_CLASS(CPoseBlenderLook)
CPoseBlenderLook::CPoseBlenderLook(): m_additionalRotationLeft(IDENTITY), m_additionalRotationRight(IDENTITY)
{
}

CPoseBlenderLook::~CPoseBlenderLook()
{
}



//
bool CPoseBlenderLook::Prepare(const SAnimationPoseModiferParams& params)
{
	//	float fTextColor[4] = {1,0,0,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"Prepare Look-IK" );	
	//	g_YLine+=16.0f;

	m_blender.m_UseDirIK							= m_blender.m_Set_UseDirIK;
	m_blender.m_nDirLayer							= m_blender.m_Set_nLayer;
	m_blender.m_vDirIKTarget						= m_blender.m_Set_vDirIKTarget;
	m_blender.m_fDirIKTargetSmoothTime = m_blender.m_Set_fDirIKTargetSmoothTime;
	m_blender.m_fDirIKBlend						= m_blender.m_Set_fDirIKBlend;
	m_blender.m_fDirIKFadeInTime			= m_blender.m_Set_fDirIKFadeInTime;
	m_blender.m_fDirIKFadeOutTime			= m_blender.m_Set_fDirIKFadeOutTime;
	m_blender.m_PolarCoordinates			= m_blender.m_Set_PolarCoordinates;

	IAttachmentManager* pIAttachmentManager = params.pCharacterInstance->GetIAttachmentManager();
	CModelSkeleton* pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);
	CCharInstance* pInstance = PoseModifierHelper::GetCharInstance(params);
	const char* pstrLEye=pModelSkeleton->m_strEyeAttachL.c_str();
	const char* pstrREye=pModelSkeleton->m_strEyeAttachR.c_str();
	int32 alidx = pIAttachmentManager->GetIndexByName(pstrLEye);
	if (alidx >= 0)
	{
		m_pAttachmentEyeLeft = SPU_MAIN_PTR( (CAttachment*)pIAttachmentManager->GetInterfaceByIndex(alidx) );
	}
	else
	{
#if !defined(__SPU__)		
		const char* pModelName = pInstance->m_pModel->GetFilePath();
		g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, pInstance->m_pModel->GetFilePath(), "CryAnimation: Left Eyeball attachments not defined or wrong name: %s",pstrLEye );
#endif		
		m_pAttachmentEyeLeft = NULL;
	}

	int32 aridx = pIAttachmentManager->GetIndexByName(pstrREye);
	if (aridx >= 0)
	{
		m_pAttachmentEyeRight = SPU_MAIN_PTR( (CAttachment*)pIAttachmentManager->GetInterfaceByIndex(aridx) );
	}
	else
	{
#if !defined(__SPU__)		
		const char* pModelName = pInstance->m_pModel->GetFilePath();
		g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, pInstance->m_pModel->GetFilePath(), "CryAnimation: Right Eyeball attachments not defined or wrong name: %s",pstrREye );
#endif		
		m_pAttachmentEyeRight = NULL;
	}

	return true;
}

// splitted code out of update to help cyrcgs simulation times
SPU_NO_INLINE void UpdateAttachment( QuatT &rQuat, CAttachment *pAttachment, int32 &rEyeIdx )
{
	rQuat.SetIdentity();
	if (pAttachment)
	{
		pAttachment->m_additionalRotation.SetIdentity();
		rQuat = QuatT(pAttachment->CAttachment::GetAttRelativeDefault());
		rEyeIdx=pAttachment->CAttachment::GetBoneID();
		pAttachment->m_additionalRotation.SetIdentity();
	}
}

SPU_NO_INLINE bool CPoseBlenderLook::Update(const SAnimationPoseModiferParams& params)
{

	//	float fTextColor[4] = {1,0,0,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"Update Look-IK" );	
	//	g_YLine+=16.0f;

	if (Console::GetInst().ca_UseLookIK == 0)
		return false;
	
	CCharInstance* pInstance = PoseModifierHelper::GetCharInstance(params);
	if (pInstance->m_pModel->m_ObjectType==CGA)
		return false;  //error-check: we shouldn't execute the PM any more

	SmoothCD(m_blender.m_vDirIKTargetSmooth, m_blender.m_vDirIKTargetRate, params.timeDelta, m_blender.m_vDirIKTarget, m_blender.m_fDirIKTargetSmoothTime);

	uint32 useLookIK = (m_blender.m_Set_UseDirIK || m_blender.m_Set_fDirIKBlend);
	if (useLookIK==0)
		return false;

	CModelSkeleton* pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);
	uint32 numDirBlends = pModelSkeleton->m_LookDirBlends.size();
	if (numDirBlends==0)
		return false;  //error-check: we shouldn't execute the PM any more
	for (uint32 d=0; d<numDirBlends; d++)
	{
		if (pModelSkeleton->m_LookDirBlends[d].m_nParaJointIdx<0 || pModelSkeleton->m_LookDirBlends[d].m_nStartJointIdx<0 || pModelSkeleton->m_LookDirBlends[d].m_nReferenceJointIdx<0)
		{
#if !defined(__SPU__)				
			const char* pModelName = pInstance->m_pModel->GetFilePath();
			g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, pInstance->GetFilePath(), "CryAnimation: No look-bone specified for model: %s",pModelName );
#endif		
			return false;  //error-check: we shouldn't execute the PM any more
		}
	}

	uint32 numRotJoints = pModelSkeleton->m_LookIK_Rot.size();
	if (numRotJoints==0)
	{
#if !defined(__SPU__)		
		const char* pModelName = pInstance->m_pModel->GetFilePath();
		g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, pInstance->GetFilePath(), "CryAnimation: No LookIK-Setup for model: %s",pModelName );
#endif		
		return false;   //error-check: we shouldn't execute the PM any more
	}

	// Pose Blender Aim initialization
	// TODO: Move this as part to the PoseModifier prepare stage.
	CSkeletonPose* pSkeletonPose = PoseModifierHelper::GetSkeletonPose(params);
	CSkeletonAnim* pSkeletonAnim = PoseModifierHelper::GetSkeletonAnim(params);

	if (pSkeletonPose->m_timeStandingUp >= 0)
		return false;
	pSkeletonAnim->Commands_CreateAimPoseQueue(m_blender, 0);  //here we should call Prepare(), and then the rest can be inside
	if (m_blender.m_numActiveDirPoses==0)
		return false; // no animation in base-layer, no aim-pose


	if (pModelSkeleton->m_LookDirBlends.size())
	{
		int32 nWeaponBone = pModelSkeleton->m_LookDirBlends[0].m_nParaJointIdx;
		if (nWeaponBone>0)
		{
			if (m_blender.m_Set_UseDirIK && Console::GetInst().ca_DrawAimPoses)
			{
#if !defined(__SPU__)
				g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
				static Ang3 angle1(0,0,0); 
				angle1 += Ang3(0.01f,0.02f,0.08f);
				AABB aabb = AABB(Vec3(-0.09f,-0.09f,-0.09f),Vec3(+0.09f,+0.09f,+0.09f));
				OBB obb1=OBB::CreateOBBfromAABB( Matrix33::CreateRotationXYZ(angle1),aabb );
				g_pAuxGeom->DrawOBB(obb1, m_blender.m_vDirIKTargetSmooth,1,RGBA8(0x00,0x00,0xff,0xff), eBBD_Extremes_Color_Encoded);
#endif
			}

			f32 fFrameTime = params.timeDelta;
			if (fFrameTime<0.0f) fFrameTime=0.0f;
			if (m_blender.m_nDirIKFadeout || m_blender.m_Set_UseDirIK==0) 
				m_blender.m_Set_fDirIKBlend -= div_min(fFrameTime, m_blender.m_Set_fDirIKFadeOutTime, m_blender.m_Set_fDirIKBlend);
			else
				m_blender.m_Set_fDirIKBlend += div_min(fFrameTime, m_blender.m_Set_fDirIKFadeInTime, 1.0f - m_blender.m_Set_fDirIKBlend);
		}
	}
	m_blender.m_fDirIKInfluence = 0;
	m_blender.m_nDirIKFadeout = 0;
	if (m_blender.m_fDirIKBlend < 0.001f)
		return false;  //IK not visible: we shouldn't execute the PM any more

	UpdateAttachment( m_ql, m_pAttachmentEyeLeft, m_EyeIdxL );
	UpdateAttachment( m_qr, m_pAttachmentEyeRight, m_EyeIdxR );

	return true;
}

SPU_INDIRECT(CommandBufferExecute(ML))
bool CPoseBlenderLook::Execute(const SAnimationPoseModiferParams& params) 
{
	//	float fTextColor[4] = {1,0,0,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"Execute Look-IK" );	
	//	g_YLine+=16.0f;

	CModelSkeleton* pModelSkeleton	= PoseModifierHelper::GetModelSkeleton(params);
	uint32 numDB				=	pModelSkeleton->m_LookDirBlends.size();
	uint32 numRotJoints = pModelSkeleton->m_LookIK_Rot.size();
	uint32 numPosJoints = pModelSkeleton->m_LookIK_Pos.size();
	SpuStackValue<SDirectionalBlender,true,true> rBlender(m_blender);

	SJointsAimIK_Pos* pPos=0;
	if (numPosJoints)
		pPos=&pModelSkeleton->m_LookIK_Pos[0];
	rBlender->ExecuteDirectionalIK(params, &pModelSkeleton->m_LookDirBlends[0],numDB, &pModelSkeleton->m_LookIK_Rot[0],numRotJoints, pPos,numPosJoints,RGBA8(0xff,0x00,0x00,0x00));

	//procedural adjustments
	{
		CModelJoint* parrModelJoints = &pModelSkeleton->m_arrModelJoints[0];
		QuatT* const __restrict pRelPose = SPU_LOCAL_PTR(&params.pPoseRelative[0]);
		QuatT* const __restrict pAbsPose = SPU_LOCAL_PTR(&params.pPoseAbsolute[0]);

		uint32 numJoints = pModelSkeleton->GetJointCount();
		pAbsPose[0]	= pRelPose[0];
		for (uint32 j=1; j<numJoints; j++)
		{
			int32 p = parrModelJoints[j].m_idxParent;
			pAbsPose[j]	= pAbsPose[p] * pRelPose[j];
			pAbsPose[j].q.Normalize();
		}


		const QuatTS& rAnimLocationNext = SPU_LOCAL_REF(params.locationNextAnimation);
		Vec3 vLocalAimIKTarget=(rBlender->m_vDirIKTargetSmooth-rAnimLocationNext.t)*rAnimLocationNext.q;


		// NOTE: Quat::CreateRotationVDir() will assume an implicit up vector along
		// the Z axis. This will cause the eyes to rotate along their forward Y
		// axis whenever the PoseModifier is enabled.
		// In order to avoid this we rotate the Y axis back minus 90 degrees.
		// TODO: Better solution would be to provide a way to create a rotation out
		// of a direction and an arbitrary up axis.
		Quat rotYMinus90 = Quat::CreateRotationY(-gf_PI*0.5f);

		//		static f32 angle=0.0f;
		//		Quat rotYMinus45 = Quat::CreateRotationY(angle);
		//		angle+=0.05f;

		if (m_pAttachmentEyeLeft)
		{
			QuatT LAbsolutePose = pAbsPose[m_EyeIdxL] * m_ql;
			Vec3 CamPosL = (vLocalAimIKTarget-LAbsolutePose.t).GetNormalizedSafe(Vec3(0,1,0));
			Quat absLEye = Quat::CreateRotationVDir(CamPosL) * rotYMinus90;
			m_additionalRotationLeft = !(pAbsPose[m_EyeIdxL].q*m_ql.q)*absLEye;

#if !(defined(XENON) || defined(PS3))
			if (Console::GetInst().ca_DrawLookIK == 2)
			{
				g_pAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags | e_DepthTestOff);
				QuatT w = QuatT(rAnimLocationNext);
				g_pAuxGeom->DrawLine(w * LAbsolutePose.t, RGBA8(0xff, 0x80, 0x80, 0xff), w * (LAbsolutePose.t + absLEye.GetColumn0() * 0.04f), RGBA8(0xff, 0x00, 0x00, 0xff));
				g_pAuxGeom->DrawLine(w * LAbsolutePose.t, RGBA8(0x80, 0xff, 0x80, 0xff), w * (LAbsolutePose.t + absLEye.GetColumn1() * 0.04f), RGBA8(0x00, 0xff, 0x00, 0xff));
				g_pAuxGeom->DrawLine(w * LAbsolutePose.t, RGBA8(0x80, 0x80, 0xff, 0xff), w * (LAbsolutePose.t + absLEye.GetColumn2() * 0.04f), RGBA8(0x00, 0x00, 0xff, 0xff));
				Quat q=m_additionalRotationLeft;
				float fTextColor[4] = {1,0,0,1};
				g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"EyeLeft: %f %f %f %f  m_IKBlend: %f",q.w, q.v.x, q.v.y, q.v.z, m_blender.m_fDirIKInfluence );	
				g_YLine+=16.0f;
			}
#endif
		}

		if (m_pAttachmentEyeRight)
		{
			QuatT RAbsolutePose = pAbsPose[m_EyeIdxR] * m_qr;
			Vec3 CamPosR = (vLocalAimIKTarget-RAbsolutePose.t).GetNormalizedSafe(Vec3(0,1,0));
			Quat absREye = Quat::CreateRotationVDir(CamPosR) * rotYMinus90;
			m_additionalRotationRight = !(pAbsPose[m_EyeIdxR].q*m_qr.q)*absREye;

#if !(defined(XENON) || defined(PS3))
			if (Console::GetInst().ca_DrawLookIK == 2)
			{
				g_pAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags | e_DepthTestOff);
				QuatT w = QuatT(rAnimLocationNext);
				g_pAuxGeom->DrawLine(w * RAbsolutePose.t, RGBA8(0xff, 0x00, 0x00, 0xff), w * (RAbsolutePose.t + absREye.GetColumn0() * 0.04f), RGBA8(0xff, 0x00, 0x00, 0xff));
				g_pAuxGeom->DrawLine(w * RAbsolutePose.t, RGBA8(0x00, 0xff, 0x00, 0xff), w * (RAbsolutePose.t + absREye.GetColumn1() * 0.04f), RGBA8(0x00, 0xff, 0x00, 0xff));
				g_pAuxGeom->DrawLine(w * RAbsolutePose.t, RGBA8(0x00, 0x00, 0xff, 0xff), w * (RAbsolutePose.t + absREye.GetColumn2() * 0.04f), RGBA8(0x00, 0x00, 0xff, 0xff));
				Quat q=m_additionalRotationRight;
				float fTextColor[4] = {1,0,0,1};
				g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fTextColor, false,"EyeRight: %f %f %f %f  m_IKBlend: %f",q.w, q.v.x, q.v.y, q.v.z, m_blender.m_fDirIKInfluence  );	
				g_YLine+=16.0f;
			}
#endif
		}


	}


	return true;
}


void CPoseBlenderLook::Synchronize()
{
	if (m_pAttachmentEyeLeft)
	{
		m_pAttachmentEyeLeft->m_additionalRotation.SetNlerp(IDENTITY, m_additionalRotationLeft, m_blender.m_fDirIKInfluence);
		m_pAttachmentEyeLeft = NULL;
	}
	if (m_pAttachmentEyeRight)
	{
		m_pAttachmentEyeRight->m_additionalRotation.SetNlerp(IDENTITY, m_additionalRotationRight, m_blender.m_fDirIKInfluence);
		m_pAttachmentEyeRight = NULL;
	}
}

void CSkeletonPose::SetPoseBlenderLookLayer(uint32 layer)
{
}


