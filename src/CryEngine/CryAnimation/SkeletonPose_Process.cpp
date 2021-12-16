//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:Skeleton.cpp
//  Implementation of Skeleton class (Forward Kinematics)
//
//	History:
//	January 12, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "CharacterInstance.h"
#include "Model.h"
#include "ModelSkeleton.h"
#include "CharacterManager.h"
#include <float.h>
#include "FacialAnimation/FacialInstance.h"
#include "PoseModifier/PoseModifierHelper.h"

void CSkeletonPose::SkeletonPostProcess2( const QuatT& rPhysLocationNext, const QuatTS &__rAnimLocationNext, IAttachment* pIAttachment, uint32 OnRender )
{
	return;
}

void CSkeletonPose::SkeletonPostProcess( const QuatT& rPhysLocationNext, const QuatTS &rAnimLocationNext, IAttachment* pIAttachment, float fZoomAdjustedDistanceFromCamera, uint32 OnRender )
{

	DEFINE_PROFILER_FUNCTION();

	// cache often used pointers in local value (prevents using the this pointer everytime)
	CCharInstance* 					const	__restrict	pInstance 	= m_pInstance;
	CSkeletonAnim* const __restrict pSkeletonAnim 		= m_pSkeletonAnim;


	//TODO add code to transfer data to/from spu storage

	QuatT* 		const	__restrict	pRelativePose = &GetPoseDataWriteable()->m_jointsRelative[0];
	QuatT* 		const	__restrict	pAbsolutePose = &GetPoseDataWriteable()->m_jointsAbsolute[0];
	//SLookIK 										&rLookIK			= m_LookIK();




#ifdef VTUNE_PROFILE 
	g_pISystem->VTuneResume();
#endif

	/*
	const char* mname = pInstance->GetFilePath();
	if ( strcmp(mname,"objects/characters/human/us/nanosuit_v2/nanosuit_v2_nohead.cdf")==0 )
	{
	uint32 Inst=(uint32)pInstance;
	float fC1[4] = {1,0,0,1};
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fC1, false,"Post Update: Instance: %x OnRender: %x  Model: %s ",Inst,OnRender,mname );	
	g_YLine+=16.0f;
	} 
	*/


	/*	uint32 numJoints2 = GetJointCount();
	if (numJoints2==85)
	{
	float fColor[4] = {0,1,0,1};
	uint32 nInstance=(uint32)(pInstance);
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"Anim: (%f %f %f %f)    (%f %f %f)  OnRender: %x %x    m_nFallPlay: %x  instance: %x",rAnimLocationNext.q.w,rAnimLocationNext.q.v.x,rAnimLocationNext.q.v.y,rAnimLocationNext.q.v.z,  rAnimLocationNext.t.x,rAnimLocationNext.t.y,rAnimLocationNext.t.z, OnRender,m_bFullSkeletonUpdate,m_nFallPlay,nInstance );	
	g_YLine+=16.0f;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"Phys: (%f %f %f %f)    (%f %f %f)  OnRender: %x",rPhysLocationNext.q.w,rPhysLocationNext.q.v.x,rPhysLocationNext.q.v.y,rPhysLocationNext.q.v.z,  rPhysLocationNext.t.x,rPhysLocationNext.t.y,rPhysLocationNext.t.z, OnRender );	
	g_YLine+=16.0f;
	}*/
	if (pIAttachment) 
	{
		//It is an attachment, so it will need an update
		assert( pIAttachment->GetType()==CA_BONE || pIAttachment->GetType()==CA_FACE); 
		CCharInstance* pInstanceMaster=((CAttachment*)pIAttachment)->m_pAttachmentManager->m_pSkelInstance;
		m_bFullSkeletonUpdate = pInstanceMaster->m_SkeletonPose.m_bFullSkeletonUpdate;
		pSkeletonAnim->m_IsAnimPlaying = 0;
		pSkeletonAnim->ProcessAnimations(rPhysLocationNext,rAnimLocationNext,OnRender);		
		pSkeletonAnim->FinishAnimationComputations();
	}


	/*
	const char* mname = pInstance->GetModelFilePath();
	if ( strcmp(mname,"objects/weapons/attachments/tactical_attachment/tactical_attachment_fp.chr")==0 )
	{
	float fColor[4] = {1,1,0,1};
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"post update: %d  Attachment: %x  Model: %s ",OnRender, pIAttachment, mname );	g_YLine+=16.0f;
	}*/ 

	int nCurrentFrameID = g_pCharacterManager->m_nUpdateCounter; 
	if (pSkeletonAnim->GetCharEditMode()==0)
	{
		if ( pInstance->m_LastUpdateFrameID_Post==nCurrentFrameID )
		{
			const char* name = pInstance->m_pModel->GetModelFilePath();
			g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,name,	"several post-updates: FrameID: %x Old:%x New: %x",nCurrentFrameID, pInstance->m_LastUpdateFrameID_PostType, OnRender);
			//	assert(0);
			return;
		}
	}

	uint32 nObjectType =	pInstance->m_pModel->m_ObjectType;
	uint32 numJoints = GetJointCount();
	pInstance->m_PhysEntityLocation = rPhysLocationNext;
	pInstance->m_fOriginalDeltaTime	= g_pITimer->GetFrameTime();
	pInstance->m_fDeltaTime					=	pInstance->m_fOriginalDeltaTime*pInstance->m_fAnimSpeedScale;
	uint32 ObjectType =	pInstance->m_pModel->m_ObjectType;
	if (pInstance->FacialAnimationEnabled())
		pInstance->m_Morphing.UpdateMorphEffectors(pInstance->m_pFacialInstance, pInstance->m_fDeltaTime,rAnimLocationNext, fZoomAdjustedDistanceFromCamera);
	else
		pInstance->m_Morphing.UpdateMorphEffectors(0, pInstance->m_fDeltaTime,rAnimLocationNext, fZoomAdjustedDistanceFromCamera);

#ifdef _DEBUG
	if ( (pSkeletonAnim->m_IsAnimPlaying & 1 ) && pSkeletonAnim->m_AnimationDrivenMotion && nObjectType==CHR)
	{
		Quat q = pRelativePose[0].q;
		Vec3 t = pRelativePose[0].t;
		assert(q.IsEquivalent(IDENTITY,0.001f));
		assert(t.IsEquivalent(ZERO,0.001f));
	}
#endif



	if (pSkeletonAnim->m_IsAnimPlaying || m_bFullSkeletonUpdate)
	{

		if (pInstance->m_LastUpdateFrameID_Pre && pSkeletonAnim->m_IsAnimPlaying)
		{
			int32 nDifference = nCurrentFrameID-pInstance->m_LastUpdateFrameID_Pre;
			if ( nDifference) 
			{
				//const char* name = pInstance->m_pModel->GetModelFilePath();
				//g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,name,	"post-update not in synch with pre-update:  PreUpdate FrameID: %x   PostUpdate FrameID: %x",pInstance->m_LastUpdateFrameID_Pre, nCurrentFrameID);
				//return;
			}
		}


		/*		if (numJoints==92)
		{
		float fColor[4] = {0,1,0,1};
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"AABB: min(%f %f %f)   max(%f %f %f)",m_AABB.min.x,m_AABB.min.y,m_AABB.min.z,  m_AABB.max.x,m_AABB.max.y,m_AABB.max.z );	
		g_YLine+=16.0f;
		}*/


		KinematicPostProcess( rPhysLocationNext, rAnimLocationNext);
	}

	//------------------------------------------------------------------------------

#if !defined(__SPU__)	
	if (m_Superimposed)
	{

		extern std::vector< std::vector<DebugJoint> > g_arrSkeletons;
		extern int32 g_nAnimID;
		extern int32 g_nGlobalAnimID;
		DrawThinSkeletons( QuatT(IDENTITY,Vec3(2,0,0)), g_arrSkeletons, g_nAnimID, g_nGlobalAnimID );

	}
#endif


	//-----------------------------------------------------------------------------------
	//--------                          update physics                          ---------
	//-----------------------------------------------------------------------------------
	if (1)
	{

		pe_params_flags pf,pf1;
		IPhysicalEntity* pCharPhys = GetCharacterPhysics();

		m_fPhysBlendTime += pInstance->m_fDeltaTime;
		if (ObjectType==CGA || pCharPhys && pCharPhys->GetType()==PE_ARTICULATED && pCharPhys->GetParams(&pf1) && pf1.flags & aef_recorded_physics)
		{

			if (m_bFullSkeletonUpdate && pSkeletonAnim->m_IsAnimPlaying || m_ppBonePhysics)
				//	if (m_InstanceVisible)
				pInstance->UpdatePhysicsCGA( 1, rAnimLocationNext );

			if (m_bFullSkeletonUpdate)
			{
				QuatT Offset = QuatT(rPhysLocationNext.GetInverted() * rAnimLocationNext);
				int32 numAttachments = pInstance->m_AttachmentManager.GetAttachmentCount()-1;

				for(int32 i=numAttachments-1; i>=0; i--) 
					pInstance->m_AttachmentManager.UpdatePhysicalizedAttachment(i,pCharPhys, Offset );

			}

#if (EXTREME_TEST==1)
			uint32 Valid = IsSkeletonValid();
			if (Valid==0)
			{
				assert(0);
				CryFatalError("CryAnimation: invalid skeleton after ProcessPhysics on CGA: %s",pInstance->GetFilePath());
			}
#endif

		}	
		else 
			if (ObjectType==CHR)
			{
				if (m_bFullSkeletonUpdate)
				{
					DEFINE_PROFILER_SECTION("CharacterInstance::Update.ProcessPhysics");
					if ( gEnv->pPhysicalWorld->GetPhysVars()->lastTimeStep > 0 || pSkeletonAnim->GetCharEditMode())
					{
						ProcessPhysics( rPhysLocationNext,pInstance->m_fOriginalDeltaTime,pSkeletonAnim->m_IsAnimPlaying,0);
						pf.flagsAND = ~pef_disabled;
						pInstance->SetWasVisible(1);
					}	else
						pInstance->SetWasVisible(0);
				} 
				else 
					if (m_pCharPhysics || m_nAuxPhys>1)
						pf.flagsOR = pef_disabled;

				pInstance->SetWasVisible(pInstance->GetWasVisible() & ( ((int)m_bFullSkeletonUpdate) > 0));
				SetAuxParams(&pf);

#if (EXTREME_TEST==1)
				for (uint32 i=0; i<numJoints; i++)
				{
					uint32 valid = pRelativePose[i].t.IsValid();
					if (valid==0)
						CryFatalError("CryAnimation: relative joint nr. %d is invalid after ProcessPhysics on CHR:  %s",i, pInstance->GetFilePath());
				}


				uint32 Valid = IsSkeletonValid();
				if (Valid==0)
				{
					assert(0);
					CryFatalError("CryAnimation: invalid skeleton after ProcessPhysics on CHR: %s",pInstance->GetFilePath());
				}
#endif
				//
				/*if( m_pCharPhysics )
				{
				QuatT offset(IDENTITY);
				if ( this->m_pSkeletonAnim->GetCharEditMode()==0 &&  m_pCharPhysics->GetType()==PE_ARTICULATED) 
				offset.t = -m_pPoseData->m_jointsAbsolute[getBonePhysChildIndex(0)].t;

				for(int i=m_pInstance->m_AttachmentManager.GetAttachmentCount()-1; i>=0; i--) 
				m_pInstance->m_AttachmentManager.UpdatePhysicalizedAttachment(i,m_pCharPhysics,offset);
				}*/
			}
	}
	//---------------------------------------------------------------------------
	//---------------------------------------------------------------------------
	//---------------------------------------------------------------------------
	if ( m_pInstance->IsCharacterVisible() || m_bFullSkeletonUpdate )
	{
		SetSkinningSkeletonMaster(rAnimLocationNext.s, (int)g_pIRenderer->EF_Query(EFQ_MainThreadList));
		pInstance->UpdateAttachedObjects(rPhysLocationNext,0,fZoomAdjustedDistanceFromCamera,OnRender);
		UpdateBBox();
	}
	else
	{
		pInstance->UpdateAttachedObjectsFast( rPhysLocationNext,fZoomAdjustedDistanceFromCamera,OnRender );
	}



#if !defined(__SPU__) 
	//	if (Console::GetInst().ca_DrawBodyMoveDir && pSkeletonAnim->m_AnimationDrivenMotion)
	if (Console::GetInst().ca_DrawLocator)
	{
		SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
		g_pAuxGeom->SetRenderFlags( renderFlags );

		const QuatT& offset2 = pSkeletonAnim->GetRelMovement().GetInverted();
		QuatT offset = offset2;
		if ( pSkeletonAnim->m_AnimationDrivenMotion)
			offset.t	=	Vec3(ZERO);

		QuatT BodyLocation = QuatT(rAnimLocationNext)*offset; 	
		BodyLocation.t += Vec3(0,0,0.01f);
		DrawArrow(BodyLocation,Vec3(0,1,0),1.0f,RGBA8(0x00,0xff,0x00,0x00) );

		//------------------------------

		Vec3 vMoveDirLocal	=	pSkeletonAnim->GetCurrentVelocity();
		f32 fLength					= pSkeletonAnim->GetCurrentVelocity().GetLength();
		QuatT TravelArrowLocation = QuatT(rAnimLocationNext)*offset;
		f32 yaw = -atan2f(vMoveDirLocal.x,vMoveDirLocal.y);
		TravelArrowLocation.t += Vec3(0.0f,0.0f,0.00f);
		TravelArrowLocation.q *= Quat::CreateRotationZ(yaw);

		DrawArrow(TravelArrowLocation,vMoveDirLocal,fLength,RGBA8(0xff,0xff,0x00,0x00) ); // Replaced by DrawArrowArc

		//DrawArrowArc(TravelArrowLocation, 
		//	pSkeletonAnim->controlParam(0, 1), // Turning angle
		//	pSkeletonAnim->controlParam(0, 0), // Speed
		//	pSkeletonAnim->controlParam(0, 2), // Slope
		//	RGBA8(0xff,0xff,0x00,0xff)
		//	);
		//	float fColor[4] = {0,1,0,1};
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"yaw: %f",yaw );	


		//	float fColor[4] = {0,1,0,1};
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"rAnimLocationNext: rot: %f (%f %f %f) pos: %f %f %f",rAnimLocationNext.q.w,rAnimLocationNext.q.v.x,rAnimLocationNext.q.v.y,rAnimLocationNext.q.v.z,  rAnimLocationNext.t.x,rAnimLocationNext.t.y,rAnimLocationNext.t.z );	
		//	g_YLine+=16.0f;
		//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"rPhysLocationNext: rot: %f (%f %f %f) pos: %f %f %f",rPhysLocationNext.q.w,rPhysLocationNext.q.v.x,rPhysLocationNext.q.v.y,rPhysLocationNext.q.v.z,  rPhysLocationNext.t.x,rPhysLocationNext.t.y,rPhysLocationNext.t.z );	
		//	g_YLine+=16.0f;


		/*
		const f32 scale = 1.0f;
		const f32 size=0.009f;
		AABB yaabb = AABB(Vec3(-size*scale, -0.0f*scale, -size*scale),Vec3(size*scale,	 fLength*scale, size*scale));

		Vec3 absAxisY	=	TravelArrowLocation.q.GetColumn1();
		OBB obb =	OBB::CreateOBBfromAABB( Matrix33(TravelArrowLocation.q), yaabb );
		g_pAuxGeom->DrawOBB(obb,TravelArrowLocation.t,1,RGBA8(0xff,0xff,0x00,0x00),eBBD_Extremes_Color_Encoded);
		g_pAuxGeom->DrawCone((absAxisY*fLength*scale)+TravelArrowLocation.t, absAxisY,0.03f,0.15f,RGBA8(0xff,0xff,0x00,0x00));
		*/
		/*
		AABB aabb;
		aabb.min=Vec3(-0.01f,-0.01f,-0.00f)+TravelArrowLocation.t;
		aabb.max=Vec3(+0.01f,+0.01f,+2.51f)+TravelArrowLocation.t;
		g_pAuxGeom->DrawAABB(aabb,1, RGBA8(0xff,0x00,0x00,0x00),eBBD_Extremes_Color_Encoded );
		g_pAuxGeom->DrawSphere(TravelArrowLocation.t,0.05f,ColorB(255,0,0));

		aabb.min=Vec3(-0.01f,-0.01f,-0.00f)+rPhysLocationNext.t;
		aabb.max=Vec3(+0.01f,+0.01f,+2.51f)+rPhysLocationNext.t;
		g_pAuxGeom->DrawAABB(aabb,1, RGBA8(0x00,0x00,0xff,0x00),eBBD_Extremes_Color_Encoded );
		g_pAuxGeom->DrawSphere(rPhysLocationNext.t,0.05f,ColorB(0,0,255));*/
	}
#endif


	pInstance->m_LastUpdateFrameID_PostType = OnRender;
	if (OnRender!=1)
		pInstance->m_LastUpdateFrameID_Post=nCurrentFrameID;

	if (pSkeletonAnim->m_AnimationDrivenMotion && nObjectType==CHR)
		pRelativePose[0].SetIdentity();

	if ( (m_nForceSkeletonUpdate&0x8000)==0)
	{
		m_nForceSkeletonUpdate--;
		if (m_nForceSkeletonUpdate<0)	m_nForceSkeletonUpdate=0;
	}

	//callback into the game with the latest skeleton pose
	if (m_pPostPhysicsCallback)		
		(*m_pPostPhysicsCallback)(pInstance,m_pPostPhysicsCallbackData);

#ifdef VTUNE_PROFILE 
	g_pISystem->VTunePause();
#endif


}



#if defined(_CPU_SSE) && !defined(_DEBUG) && !defined(PS3)
#include "fvec.h"
#endif


#define FEET_ANCHOR 0
void CSkeletonPose::KinematicPostProcess(const QuatT &rPhysLocationNext, const QuatTS &rAnimLocationNext)
{
	SAnimationPoseModiferParams poseModifierParams;
	poseModifierParams.pCharacterInstance = m_pInstance;
	poseModifierParams.timeDelta = m_pInstance->m_fOriginalDeltaTime;
	poseModifierParams.locationNextPhysics = rPhysLocationNext;
	poseModifierParams.locationNextAnimation = rAnimLocationNext;
	poseModifierParams.pPoseRelative = &GetPoseDataWriteable()->m_jointsRelative[0];
	poseModifierParams.pPoseAbsolute = &GetPoseDataWriteable()->m_jointsAbsolute[0];
	poseModifierParams.jointCount = GetPoseDataWriteable()->GetJointCount();

	// use local pointer so spare dereferencing
	CSkeletonAnim* 	const __restrict pSkeletonAnim 	= m_pSkeletonAnim;
	int16* 					const __restrict pIdxArray 			= m_pModelSkeleton->m_IdxArray;

	// use spu local memory for often used arrays
	QuatT* const __restrict	pRelativePose = SPU_PTR_SELECT( &GetPoseDataWriteable()->m_jointsRelative[0], gSkeletonRelativePose );
	QuatT* const __restrict	pAbsolutePose	= SPU_PTR_SELECT( &GetPoseDataWriteable()->m_jointsAbsolute[0], gSkeletonAbsolutePose );
	SLookIK 										&rLookIK			= m_LookIK();
	SFootAnchor 								&rFootAnchor 	= m_FootAnchor;

#if !defined(__SPU__)
	if ( Console::GetInst().ca_DrawPositionPost & 1)
	{
		//draw the entity in blue
		Vec3 wpos = rPhysLocationNext.t+Vec3(0.0f,0.0f,0.0f);
		g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
		static Ang3 angle(0,0,0);		angle += Ang3(0.01f,+0.03f,-0.07f);
		AABB aabb = AABB(Vec3(-0.05f,-0.05f,-0.05f),Vec3(+0.05f,+0.05f,+0.05f));
		OBB obb=OBB::CreateOBBfromAABB( Matrix33::CreateRotationXYZ(angle), aabb );
		g_pAuxGeom->DrawOBB(obb,wpos,1,RGBA8(0x00,0x00,0xff,0x00), eBBD_Extremes_Color_Encoded);

		Vec3 axisX = rPhysLocationNext.q.GetColumn0();
		Vec3 axisY = rPhysLocationNext.q.GetColumn1();
		Vec3 axisZ = rPhysLocationNext.q.GetColumn2();
		g_pAuxGeom->DrawLine(wpos,RGBA8(0x7f,0x00,0x00,0x00), wpos+axisX,RGBA8(0x7f,0x00,0x00,0x00) );
		g_pAuxGeom->DrawLine(wpos,RGBA8(0x00,0x7f,0x00,0x00), wpos+axisY,RGBA8(0x00,0x7f,0x00,0x00) );
		g_pAuxGeom->DrawLine(wpos,RGBA8(0x00,0x00,0x7f,0x00), wpos+axisZ,RGBA8(0x00,0x00,0x7f,0x00) );

		Vec3 line = rPhysLocationNext.q.GetColumn2()*5;

		g_pAuxGeom->DrawLine(wpos,RGBA8(0x00,0x00,0xff,0x00), wpos+line,RGBA8(0x00,0x00,0xff,0x00) );
	}
	if ( Console::GetInst().ca_DrawPositionPost & 2) 
	{
		//draw the animated character in red
		Vec3 wpos = rAnimLocationNext.t+Vec3(0.0f,0.0f,0.0f);
		g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
		static Ang3 angle(0,0,0); 
		angle += Ang3(0.01f,-0.02f,-0.03f);
		AABB aabb = AABB(Vec3(-0.05f,-0.05f,-0.05f),Vec3(+0.05f,+0.05f,+0.05f));
		OBB obb=OBB::CreateOBBfromAABB( Matrix33::CreateRotationXYZ(angle), aabb );
		g_pAuxGeom->DrawOBB(obb,wpos,1,RGBA8(0xff,0x00,0x00,0x00), eBBD_Extremes_Color_Encoded);

		Vec3 axisX = rAnimLocationNext.q.GetColumn0();
		Vec3 axisY = rAnimLocationNext.q.GetColumn1();
		Vec3 axisZ = rAnimLocationNext.q.GetColumn2();
		g_pAuxGeom->DrawLine(wpos,RGBA8(0xff,0x00,0x00,0x00), wpos+axisX,RGBA8(0xff,0x00,0x00,0x00) );
		g_pAuxGeom->DrawLine(wpos,RGBA8(0x00,0xff,0x00,0x00), wpos+axisY,RGBA8(0x00,0xff,0x00,0x00) );
		g_pAuxGeom->DrawLine(wpos,RGBA8(0x00,0x00,0xff,0x00), wpos+axisZ,RGBA8(0x00,0x00,0xff,0x00) );

		Vec3 line = rAnimLocationNext.q.GetColumn2()*5;
		g_pAuxGeom->DrawLine(wpos,RGBA8(0xff,0x00,0x00,0x00), wpos+line,RGBA8(0xff,0x00,0x00,0x00) );
	}
	if (Console::GetInst().ca_DebugAnimUpdates)
	{
		//just for debugging
		string strModelPath = m_pInstance->m_pModel->GetModelFilePath();
		g_pCharacterManager->m_arrSkeletonUpdates.push_back(strModelPath);
		g_pCharacterManager->m_arrAnimPlaying.push_back(pSkeletonAnim->m_IsAnimPlaying);
		g_pCharacterManager->m_arrForceSkeletonUpdates.push_back(m_nForceSkeletonUpdate);
		g_pCharacterManager->m_arrVisible.push_back(m_bInstanceVisible);
	}
#endif




	//------------------------------------------------------------------------------------------------

	uint32 numJoints = GetJointCount();
	uint32 ObjectType =	m_pInstance->m_pModel->m_ObjectType;


	if (m_bFullSkeletonUpdate) 
	{
		//------------------------------------------------------------------------------
		//--------                        Aim-IK                                 -------
		//------------------------------------------------------------------------------
		/*if (m_AimIK().m_UseAimIK || m_AimIK().m_AimIKBlend)
		{
		m_AimIK().Execute(poseModifierParams);
		}*/


#if FEET_ANCHOR
		//------------------------------------------------------------------------------
		//--------                    Foot-Anchoring                              ------
		//------------------------------------------------------------------------------
		if ( Console::GetInst().ca_FootAnchoring && (pSkeletonAnim->m_IsAnimPlaying&1 && m_IsAnkleValid))
		{
			uint32 HasOldPosition=0;
			uint32 CurrentFrameID = g_pCharacterManager->m_nUpdateCounter; //g_pIRenderer->GetFrameID(false);
			uint32 DeltaID = (CurrentFrameID-rFootAnchor.m_LastLegUpdate);
			rFootAnchor.m_LastLegUpdate=CurrentFrameID;
			if (DeltaID==1 || DeltaID==2)
				HasOldPosition=1;

			if (m_pModelSkeleton->IsHuman())
			{
				AbsoluteLegPosition(rAnimLocationNext);

				if (HasOldPosition)
					FootAnchoring( pSkeletonAnim->m_BlendedRoot.m_FootPlant, pSkeletonAnim->m_BlendedRoot.m_nFootBits, rAnimLocationNext );

				rFootAnchor.m_LLastHeel=rFootAnchor.m_AbsoluteLHeel.t;	rFootAnchor.m_LLastToe=rFootAnchor.m_AbsoluteLToe0.t;
				rFootAnchor.m_RLastHeel=rFootAnchor.m_AbsoluteRHeel.t;	rFootAnchor.m_RLastToe=rFootAnchor.m_AbsoluteRToe0.t;
			}
		}
#endif

		int32 leye = pIdxArray[eIM_LeftEyeIdx];
		int32 reye = pIdxArray[eIM_RightEyeIdx];
		if (leye>0)
		{ 
			//pRelativePose[leye].q.SetRotationY(gf_PI/2);
			pRelativePose[leye].q.SetIdentity();
			if (m_arrPhysicsJoints.size())
				pRelativePose[leye].t=m_arrPhysicsJoints[leye].m_DefaultRelativeQuat.t;
		}
		if (reye>0)
		{
			//	pRelativePose[reye].q.SetRotationY(gf_PI/2);
			pRelativePose[reye].q.SetIdentity();
			if (m_arrPhysicsJoints.size())
				pRelativePose[reye].t=m_arrPhysicsJoints[reye].m_DefaultRelativeQuat.t;
		}

		//	m_parrJoints[0].pRelativePose.SetIdentity();
		//m_parrJoints[0].pRelativePose = m_RelativePelvis;


		if ((pSkeletonAnim->m_IsAnimPlaying&1) && pSkeletonAnim->m_AnimationDrivenMotion && ObjectType==CHR)
		{
			QuatT delta = rPhysLocationNext.GetInverted()*QuatT(rAnimLocationNext);

			delta.t.x = clamp(delta.t.x, -40.f, 40.f);
			delta.t.y = clamp(delta.t.y, -40.f, 40.f);
			delta.t.z = clamp(delta.t.z, -40.f, 40.f);
			pRelativePose[0] = delta;
			m_vRenderOffset = -delta.t;
		}
		else
		{
			/*	if (m_AnimationDrivenMotion==0 && ObjectType==CGA)
			{
			for (uint32 i=0; i<numJoints; i++)
			{
			QuatT delta = rPhysLocationNext.GetInverted()*QuatT(rAnimLocationNext);
			int32 p = m_parrModelJoints[i].m_idxParent;
			if (p<0)
			{
			m_parrJoints[i].pRelativePose = delta*m_parrModelJoints[i].m_DefaultRelQuat;
			}
			}
			}*/
		}


#if (EXTREME_TEST==1)
		QuatT delta = rPhysLocationNext.GetInverted()*QuatT(rAnimLocationNext);
		for (uint32 i=0; i<numJoints; i++)
		{
			QuatT rp = pRelativePose[i];
			f32 t=pRelativePose[i].t.GetLength();
			uint32 valid = pRelativePose[i].t.IsValid();
			if (valid==0)
			{
				assert(0);
				CryFatalError("CryAnimation: relative joint nr. %d is invalid in kinematic post process:  %s",i, m_pInstance->GetFilePath());
			}
		}
#endif



		if (ObjectType==CHR)
		{	
			CryPrefetch(&m_parrModelJoints[0].m_idxParent);

			//we assume that all CHRs are single root objects
			pAbsolutePose[0] = pRelativePose[0];
			for (uint32 i=1; i<numJoints; i++)
			{
#ifndef _DEBUG
				CryPrefetch(&m_parrModelJoints[i+1].m_idxParent);
				CryPrefetch(&pAbsolutePose[i+4]);
				CryPrefetch(&pRelativePose[i+4]);
#endif

				pAbsolutePose[i]	= pAbsolutePose[m_parrModelJoints[i].m_idxParent] * pRelativePose[i];
			}
		}
		else
		{
			CryPrefetch(&m_parrModelJoints[0].m_idxParent);

			for (uint32 i=0; i<numJoints; i++)
			{
#ifndef _DEBUG
				CryPrefetch(&m_parrModelJoints[i+1].m_idxParent);
				CryPrefetch(&pAbsolutePose[i+4]);
				CryPrefetch(&pRelativePose[i+4]);
#endif
				int32 p = m_parrModelJoints[i].m_idxParent;
				if (p>=0)
					pAbsolutePose[i]	= pAbsolutePose[p] * pRelativePose[i];
				else
					pAbsolutePose[i] = pRelativePose[i];
			}
		}


#if (EXTREME_TEST==1)
		uint32 Valid3 = IsSkeletonValid();
		if (Valid3==0)
		{
			assert(0);
			CryFatalError("CryAnimation: invalid skeleton after AbsolutePose: %s",m_pInstance->GetFilePath());
		}
#endif

		{

			// some reference to spare unnecessary ptr dereferencing
			CSkeletonPose* 	const __restrict 	pSkeletonPose 					= this;
			CModelSkeleton* const __restrict 	pModelSkeleton 					= pSkeletonPose->m_pModelSkeleton;
			int16* 					const __restrict 	pModelSkeletonIdxArray 	= pModelSkeleton->m_IdxArray;


		//	uint32 numJoints		= GetJointCount();

			uint32 numIKTargets = pModelSkeleton->m_ADIKTargets.size();
			if (m_pSkeletonAnim->m_IsAnimPlaying&1)
			{
				for (uint32 i=0; i<numIKTargets; i++)
				{
					uint64 nHandle = pModelSkeleton->m_ADIKTargets[i].m_nHandle;
					const char* pHandle = (const char*)(&pModelSkeleton->m_ADIKTargets[i].m_nHandle);
					const char* pName = pModelSkeleton->m_ADIKTargets[i].m_strTarget;

					int32 nIKTargetIdx   = pModelSkeleton->m_ADIKTargets[i].m_idxTarget;
					assert(nIKTargetIdx<256); //an index buffer then 256 is invalid
					if (nIKTargetIdx<0 )
						continue; //no Target-Joint

					int32 nIKWeightIdx    = pModelSkeleton->m_ADIKTargets[i].m_idxWeight;
					assert(nIKWeightIdx<256);  //an index buffer then 256 is invalid
					if (nIKWeightIdx<0)
						continue; //no Weight-Joint


					int32 idxDefinition=pModelSkeleton->GetLimbDefinitionIdx(nHandle);
					if (idxDefinition<0)
						continue;   //we didn't find the limb definition and the IK-Solver for this handle 

					IKLimbType& rIKLimbType	= pModelSkeleton->m_IKLimbTypes[idxDefinition];

					const QuatT& rAbsIKTarget	= GetPoseDataWriteable()->m_jointsAbsolute[nIKTargetIdx];
					f32 fIKTargetBlend		= GetPoseDataWriteable()->m_jointsRelative[nIKWeightIdx].t.x;
					fIKTargetBlend = clamp(fIKTargetBlend,0.0f, 1.0f);	
					if (fIKTargetBlend>0.01f)
					{
					//	float fColor[4] = {0,1,0,fIKTargetBlend};
					//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fColor, false,"LHand_IKTarget: name: %s  rot: (%f %f %f %f)  pos: (%f %f %f) blend: %f",pName, rAbsIKTarget.q.w,rAbsIKTarget.q.v.x,rAbsIKTarget.q.v.y,rAbsIKTarget.q.v.z,  rAbsIKTarget.t.x,rAbsIKTarget.t.y,rAbsIKTarget.t.z, m_pPoseData->m_jointsRelative[nIKWeightIdx].t.x);	
					//	g_YLine+=16.0f;

						uint32 numLinks=rIKLimbType.m_arrJointChain.size();
						int32 nEndEffector =	rIKLimbType.m_arrJointChain[numLinks-1].m_idxJoint; //EndEffector
						int32 nParentEndEffector =	m_parrModelJoints[nEndEffector].m_idxParent; //parent of EndEffector


						Vec3 vLocalTarget; vLocalTarget.SetLerp(GetPoseDataWriteable()->m_jointsAbsolute[nEndEffector].t,rAbsIKTarget.t,fIKTargetBlend);
						if (rIKLimbType.m_nSolver == *(uint32*)"2BIK")
							PoseModifierHelper::IK_Solver2Bones(vLocalTarget,rIKLimbType,pRelativePose,pAbsolutePose);
						if (rIKLimbType.m_nSolver == *(uint32*)"3BIK")
						  PoseModifierHelper::IK_Solver3Bones(vLocalTarget,rIKLimbType,pRelativePose,pAbsolutePose);
						if (rIKLimbType.m_nSolver == *(uint32*)"CCDX")
							PoseModifierHelper::IK_SolverCCD(vLocalTarget,rIKLimbType,pRelativePose,pAbsolutePose);

						//apply the orientation of the Target to the EndEffector
					//	m_pPoseData->m_jointsRelative[b2].q.SetNlerp( m_pPoseData->m_jointsRelative[b2].q, !m_pPoseData->m_jointsAbsolute[b1].q*rAbsIKTarget.q, fIKTargetBlend);

						uint32 numChilds = pModelSkeleton->m_IKLimbTypes[idxDefinition].m_arrLimbChildren.size();
						int16* pJointIdx = &pModelSkeleton->m_IKLimbTypes[idxDefinition].m_arrLimbChildren[0];
						for (uint32 u=0; u<numChilds; u++)
						{
							int32 c=pJointIdx[u];
							int32 p=m_parrModelJoints[c].m_idxParent;
							GetPoseDataWriteable()->m_jointsAbsolute[c]	= GetPoseDataWriteable()->m_jointsAbsolute[p] * GetPoseDataWriteable()->m_jointsRelative[c];
						}

						GetPoseDataWriteable()->m_jointsAbsolute[nEndEffector].q.SetNlerp( GetPoseDataWriteable()->m_jointsAbsolute[nEndEffector].q, rAbsIKTarget.q, fIKTargetBlend);
						GetPoseDataWriteable()->m_jointsRelative[nEndEffector].q = !GetPoseDataWriteable()->m_jointsAbsolute[nParentEndEffector].q*GetPoseDataWriteable()->m_jointsAbsolute[nEndEffector].q;
					}


#if !defined(__SPU__)
					if ( Console::GetInst().ca_DebugADIKTargets )
					{
						float fColor[4] = {0,1,0,1};
						g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"LHand_IKTarget: name: %s  rot: (%f %f %f %f)  pos: (%f %f %f) blend: %f",pName, rAbsIKTarget.q.w,rAbsIKTarget.q.v.x,rAbsIKTarget.q.v.y,rAbsIKTarget.q.v.z,  rAbsIKTarget.t.x,rAbsIKTarget.t.y,rAbsIKTarget.t.z, GetPoseDataWriteable()->m_jointsRelative[nIKWeightIdx].t.x);	
						g_YLine+=16.0f;

						static Ang3 angle(0,0,0); 	angle+=Ang3(0.01f,+0.02f,+0.03f);
						AABB aabb = AABB(Vec3(-0.015f,-0.015f,-0.015f),Vec3(+0.015f,+0.015f,+0.015f));

						Matrix33 m33=Matrix33::CreateRotationXYZ(angle);
						OBB obb=OBB::CreateOBBfromAABB( m33,aabb );
						g_pAuxGeom->DrawOBB(obb,rAbsIKTarget.t,1,RGBA8(0xff,0x00,0x1f,0xff),eBBD_Extremes_Color_Encoded);
					}
#endif

				}
			}
		}


		//------------------------------------------------------------------------------
		//--------                        Look-IK                                -------
		//------------------------------------------------------------------------------
		/*		if (rLookIK.m_UseLookIK || rLookIK.m_LookIKBlend)
		{
		SAnimationPoseModiferParams poseModifierParams;
		poseModifierParams.pCharacterInstance = m_pInstance;
		poseModifierParams.timeDelta = m_pInstance->m_fOriginalDeltaTime;
		poseModifierParams.locationNextPhysics = rPhysLocationNext;
		poseModifierParams.locationNextAnimation = rAnimLocationNext;
		poseModifierParams.pPoseRelative = pRelativePose;
		poseModifierParams.pPoseAbsolute = pAbsolutePose;
		poseModifierParams.jointCount = numJoints;
		m_LookAt->Execute(poseModifierParams);
		#if (EXTREME_TEST==1)
		uint32 Valid = IsSkeletonValid();
		if (Valid==0)
		g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,m_pInstance->GetFilePath(),	"CryAnimation: Invalid Skeleton after LookIK" );
		#endif
		}*/


		/*		else
		{
		int32 ridx			= pIdxArray[eIM_RightEyeIdx];
		int32 lidx			= pIdxArray[eIM_LeftEyeIdx];
		if (ridx>0 && lidx>0)
		{
		Vec3 LEyePos	= pAbsolutePose[lidx].t;
		Vec3 REyePos	= pAbsolutePose[ridx].t;
		Vec3 MEyePos	= rAnimLocationNext*((LEyePos+REyePos)*0.5f);
		Vec3 LEyeDir	= pAbsolutePose[lidx].q.GetColumn1();
		Vec3 REyeDir	= pAbsolutePose[ridx].q.GetColumn1();
		Vec3 MEyeDir	= rAnimLocationNext.q * (LEyeDir+REyeDir).GetNormalizedSafe();
		//g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
		//g_pAuxGeom->DrawLine(MEyePos,RGBA8(0x00,0x00,0x00,0x00), MEyePos+MEyeDir,RGBA8(0xff,0xff,0xff,0x00) );
		rLookIK.m_LookIKTarget = MEyePos+MEyeDir*100.0f;

		if ( (pSkeletonAnim->m_IsAnimPlaying & 0x1) && m_bInstanceVisible)
		{
		Vec3 vLocalLookAtTarget = rLookIK.m_LookIKTargetSmooth-rPhysLocationNext.t;
		CFacialInstance *pFacialInstance = (CFacialInstance *)(m_pInstance->GetFacialInstance());
		if (pFacialInstance)
		{
		Vec3 vp = vLocalLookAtTarget*rPhysLocationNext.q;
		pFacialInstance->ApplyProceduralFaceBehaviour( vp );	
		}
		}

		}


		} 
		*/


		if( Console::GetInst().ca_UseLookIK )
			LookIKHunter(rLookIK.m_UseLookIK, rPhysLocationNext, rAnimLocationNext);

#if (EXTREME_TEST==1)
		uint32 Valid8 = IsSkeletonValid();
		if (Valid8==0)
		{
			assert(0);
			CryFatalError("CryAnimation: invalid skeleton after LookIKHunter(): %s",m_pInstance->GetFilePath());
		}
#endif

		//-------------------------------------------------------------
		//update the new joint values we added in a post process
		//-------------------------------------------------------------

		//callback into the game with the latest skeleton pose
		if (m_pPostProcessCallback)		
			(*m_pPostProcessCallback)(m_pInstance,m_pPostProcessCallbackData);


#if FEET_ANCHOR
		if (m_bFullSkeletonUpdate && m_pModelSkeleton->IsHuman() )
		{
			//------------------------------------------------------------------------------
			//--------                    Foot-Anchoring                              ------
			//------------------------------------------------------------------------------
			if (Console::GetInst().ca_FootAnchoring && (pSkeletonAnim->m_IsAnimPlaying & 1))
			{
				/*	uint32 HasOldPosition=0;
				uint32 CurrentFrameID = g_pCharacterManager->m_nUpdateCounter; //g_pIRenderer->GetFrameID(false);
				uint32 DeltaID = (CurrentFrameID-rFootAnchor.m_LastLegUpdate);
				rFootAnchor.m_LastLegUpdate=CurrentFrameID;
				if (DeltaID==1 || DeltaID==2)
				HasOldPosition=1;

				if (m_pModelSkeleton->IsHuman())
				{
				AbsoluteLegPosition(rAnimLocationNext);
				if (HasOldPosition)
				FootAnchoring( pSkeletonAnim->m_BlendedRoot.m_FootPlant, pSkeletonAnim->m_BlendedRoot.m_nFootBits, rAnimLocationNext );
				rFootAnchor.m_LLastHeel=rFootAnchor.m_AbsoluteLHeel.t;	rFootAnchor.m_LLastToe=rFootAnchor.m_AbsoluteLToe0.t;
				rFootAnchor.m_RLastHeel=rFootAnchor.m_AbsoluteRHeel.t;	rFootAnchor.m_RLastToe=rFootAnchor.m_AbsoluteRToe0.t;
				}*/


				//new foot-anchoring
				int32 pelvis =	pIdxArray[eIM_PelvisIdx];

				Vec3 lhalf=(rFootAnchor.m_LHeelSlide*rPhysLocationNext.q*0.5f)*rFootAnchor.m_LAnkleIntensity;
				f32 ldot=lhalf|lhalf;
				if (ldot>0.00001f)
				{
					int32 lb0	=	pIdxArray[eIM_LThighIdx];
					int32 lb1	=	pIdxArray[eIM_LCalfIdx];
					int32 lb2	=	pIdxArray[eIM_LFootIdx];
					if (lb2>0)
					{
						const QuatT& lFoot =	pAbsolutePose[lb2];
						SetHumanLimbIK_Local(lFoot.t-lhalf,"LftLeg01");
						pAbsolutePose[lb0].t-=lhalf*0.3333f;
						pAbsolutePose[lb1].t-=lhalf*0.6666f;
						pAbsolutePose[lb2].t-=lhalf;

						pRelativePose[lb0]	= pAbsolutePose[pelvis].GetInverted() * pAbsolutePose[lb0];
						pRelativePose[lb0].q.Normalize();
						pRelativePose[lb1]	= pAbsolutePose[lb0].GetInverted() * pAbsolutePose[lb1];
						pRelativePose[lb1].q.Normalize();
						pRelativePose[lb2]	= pAbsolutePose[lb1].GetInverted() * pAbsolutePose[lb2];
						pRelativePose[lb2].q.Normalize();

						uint64 nLHandleLeg64 = *(uint64*)"LftLeg01";
						int32 idxLDefinitionLeg=m_pModelSkeleton->GetLimbDefinitionIdx(nLHandleLeg64);
						if (idxLDefinitionLeg<0)
							return;  //we didn't find the limb definition and the IK-Solver for this handle 
						uint32 numLChilds = m_pModelSkeleton->m_IKLimbTypes[idxLDefinitionLeg].m_arrLimbChildren.size();
						int16* pLJointIdx = &m_pModelSkeleton->m_IKLimbTypes[idxLDefinitionLeg].m_arrLimbChildren[0];
						for (uint32 i = 0; i <numLChilds; i++)
						{
							int32 lc1=pLJointIdx[i];
							uint32 lp1 = m_parrModelJoints[lc1].m_idxParent;
							GetPoseDataWriteable()->m_jointsAbsolute[lc1]	= GetPoseDataWriteable()->m_jointsAbsolute[lp1] * GetPoseDataWriteable()->m_jointsRelative[lc1];
						}
					}
				}



				Vec3 rhalf=(rFootAnchor.m_RHeelSlide*rPhysLocationNext.q*0.5f)*rFootAnchor.m_RAnkleIntensity;
				f32 rdot=rhalf|rhalf;
				if (rdot>0.00001f)
				{
					int32 rb0	=	pIdxArray[eIM_RThighIdx];
					int32 rb1	=	pIdxArray[eIM_RCalfIdx];
					int32 rb2	=	pIdxArray[eIM_RFootIdx];
					if (rb2>0)
					{
						const QuatT& rFoot =	pAbsolutePose[rb2];
						SetHumanLimbIK_Local(rFoot.t-rhalf,"RgtLeg01");
						pAbsolutePose[rb0].t-=rhalf*0.3333f;
						pAbsolutePose[rb1].t-=rhalf*0.6666f;
						pAbsolutePose[rb2].t-=rhalf;

						pRelativePose[rb0]	= pAbsolutePose[pelvis].GetInverted() * pAbsolutePose[rb0];
						pRelativePose[rb0].q.Normalize();
						pRelativePose[rb1]	= pAbsolutePose[rb0].GetInverted() * pAbsolutePose[rb1];
						pRelativePose[rb1].q.Normalize();
						pRelativePose[rb2]	= pAbsolutePose[rb1].GetInverted() * pAbsolutePose[rb2];
						pRelativePose[rb2].q.Normalize();

						uint64 nRHandleLeg64 = *(uint64*)"RgtLeg01";
						int32 idxRDefinitionLeg=m_pModelSkeleton->GetLimbDefinitionIdx(nRHandleLeg64);
						if (idxRDefinitionLeg<0)
							return;  //we didn't find the limb definition and the IK-Solver for this handle 
						uint32 numRChilds = m_pModelSkeleton->m_IKLimbTypes[idxRDefinitionLeg].m_arrLimbChildren.size();
						int16* pRJointIdx = &m_pModelSkeleton->m_IKLimbTypes[idxRDefinitionLeg].m_arrLimbChildren[0];
						for (uint32 i=0; i<numRChilds; i++)
						{
							int32 lc2=pRJointIdx[i];
							uint32 lp2 = m_parrModelJoints[lc2].m_idxParent;
							GetPoseDataWriteable()->m_jointsAbsolute[lc2]	= GetPoseDataWriteable()->m_jointsAbsolute[lp2] * GetPoseDataWriteable()->m_jointsRelative[lc2];
						}
					}
				}
			}
		}
#endif


		uint32 IsActive		= Console::GetInst().ca_GroundAlignment;
		uint32 isVisible	=	m_bInstanceVisible;
		uint32 IsPlayer		= pSkeletonAnim->m_IsAnimPlaying&1;
		uint32 IsReady		= m_bGroundAlignDataReady;
		if (m_bGroundAlignDataReady && Console::GetInst().ca_GroundAlignment && (pSkeletonAnim->m_IsAnimPlaying&1) && m_bInstanceVisible)
		{
			if (m_alignSkeletonVertical)
			{
				MoveSkeletonVertical(m_rootHeight);
			}

			SetFootGroundAlignmentCCD( "LftLeg01", m_planeLeft);
			SetFootGroundAlignmentCCD( "RgtLeg01", m_planeRight);
		} 

	}

	PoseModifiersExecute(rPhysLocationNext, rAnimLocationNext);

	if (m_bFullSkeletonUpdate)
	{

		uint32 numEntries = m_arrPostProcess.size();
		if (numEntries)
		{
			for (uint32 i=0; i<numEntries; i++)
			{
				int32 idx = m_arrPostProcess[i].m_JointIdx;
				QuatT qt  = m_arrPostProcess[i].m_RelativePose;
				if (!pRelativePose[idx].IsEquivalent(qt, 0.0001f))
				{
					pRelativePose[idx] = qt;
					m_bAllAbsoluteJointsValid = false;
					pSkeletonAnim->m_IsAnimPlaying |= 0x80000000;
				}
			}
		}

		//is an update necesarry?
		if (!m_bAllAbsoluteJointsValid)
		{
			//float fColor[4] = {0,1,0,1};
			//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"UpdateAbsoluteJointTransforms" );	
			//g_YLine+=16.0f;
			m_bAllAbsoluteJointsValid = true;
			//		uint32 numJoints = GetJointCount();
			if (numJoints)
			{
				for (uint32 i=0; i<numJoints; i++)
				{
					int32 p=m_parrModelJoints[i].m_idxParent;
					if (p<0)
						pAbsolutePose[i] = pRelativePose[i];  //this is a root
					else
						pAbsolutePose[i]	= pAbsolutePose[p] * pRelativePose[i];
				}
			}
		}
	}
#if !defined(__SPU__)
	//TODO this adds 20 seconds of computation time for CryCG, find out why, or better
	// move memory handling away from the SPU
	m_arrPostProcess.resize(0);
#endif

	if (m_pModelSkeleton->IsHuman() && (pSkeletonAnim->m_IsAnimPlaying & 1) && m_bInstanceVisible)
		PlayRecoilAnimation(rPhysLocationNext);
}




///////////////////////////////////////////////////////////////////////////////
void CSkeletonPose::SetSkinningSkeletonMaster(f32 fScaling, int nList) 
{

#ifdef VTUNE_PROFILE 
	g_pISystem->VTuneResume();
#endif

	// cache often used pointers in local value (prevents using the this pointer everytime)
	CCharInstance* const __restrict pInstance = m_pInstance;

	const char* name = pInstance->m_pModel->GetModelFilePath();

	int nCurrentFrameID = g_pCharacterManager->m_nUpdateCounter;//g_pIRenderer->GetFrameID(false);
	if (pInstance->m_LastUpdateFrameID_Post == nCurrentFrameID)
		return;
//	g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,name,	"several updates per frame: FrameID: %x",nCurrentFrameID);

/*
	pInstance->m_iBackFrame = pInstance->m_iActiveFrame;

	if( pInstance->m_bUpdateMotionBlurSkinnning )
		++pInstance->m_iActiveFrame;

//	if (pInstance->m_iActiveFrame >= MAX_MOTIONBLUR_FRAMES)
		pInstance->m_iActiveFrame = 0;
	int nMT = (int)g_pIRenderer->EF_Query(EFQ_RenderMultithreaded);
	if (nMT)
	{
		pInstance->m_iActiveFrame = 0;
		pInstance->m_iBackFrame = 0;
	}

	uint32 iActiveFrame = pInstance->m_iActiveFrame;
*/
	const QuatT* pJointsAbsolute = GetPoseData().GetJointsAbsolute();
	const QuatT* pJointsAbsoluteDefault = GetPoseDataDefault().GetJointsAbsolute();

	QuatTS* arrSkinQuat = &pInstance->m_arrNewSkinQuat[nList][0];

	//----------------------------------------------------------
	//--  its a master skeleton. do the full initialization   --
	//----------------------------------------------------------
	uint32 numJoints = GetJointCount();
	if (Console::GetInst().ca_SphericalSkinning) 
	{
		
	//	float fColor[4] = {0,1,0,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"Skinning");	
	//	g_YLine+=16.0f;

		//--- Deliberate querying off the end of arrays for speed, prefetching off the end of an array is safe & avoids extra branches
		const QuatT* absPose_PreFetchOnly = &pJointsAbsolute[0];
#ifndef _DEBUG
		CryPrefetch(&pJointsAbsoluteDefault[0]);
		CryPrefetch(&absPose_PreFetchOnly[0]);
		CryPrefetch(&arrSkinQuat[0]);
		CryPrefetch(&pJointsAbsoluteDefault[1]);
		CryPrefetch(&absPose_PreFetchOnly[4]);
		CryPrefetch(&arrSkinQuat[4]);
#endif

		{
			QuatT DevInvPose = pJointsAbsoluteDefault[0].GetInverted();
			DevInvPose.t *= fScaling;
			QuatD qd(pJointsAbsolute[0] * DevInvPose);
			QuatTS &rSkinQuat0 = arrSkinQuat[0];
			rSkinQuat0.q = qd.nq*fScaling;
			rSkinQuat0.t.x = qd.dq.v.x;
			rSkinQuat0.t.y = qd.dq.v.y;
			rSkinQuat0.t.z = qd.dq.v.z;
			rSkinQuat0.s = qd.dq.w;
		}	

		for (uint32 i=1; i<numJoints; i++)
		{
#ifndef _DEBUG
			CryPrefetch(&pJointsAbsoluteDefault[i+1]);
			CryPrefetch(&absPose_PreFetchOnly[i+4]);
			CryPrefetch(&arrSkinQuat[i+4]);
			CryPrefetch(&m_parrModelJoints[i+1].m_idxParent);
#endif

			QuatT DevInvPose = pJointsAbsoluteDefault[i].GetInverted();
			DevInvPose.t *= fScaling;

			QuatD qd(pJointsAbsolute[i] * DevInvPose);

			int32 p = m_parrModelJoints[i].m_idxParent; 
			if (p>-1)
			{
				f32 cosine = qd.nq | arrSkinQuat[p].q;
				f32 mul = (f32)__fsel(cosine, 1.0f, -1.0f);
				qd.nq *= mul;
				qd.dq *= mul;
			}

			QuatTS &rCurSkinQuat0 = arrSkinQuat[i];

			rCurSkinQuat0.q = qd.nq*fScaling;
			rCurSkinQuat0.t.x = qd.dq.v.x;
			rCurSkinQuat0.t.y = qd.dq.v.y;
			rCurSkinQuat0.t.z = qd.dq.v.z;
			rCurSkinQuat0.s = qd.dq.w;
		}
	}
	else
	{
		QuatTS q;
		for (uint32 i=0; i<numJoints; i++)
		{ 
			if( m_pInstance->m_bCustomScale[i] )
			{
				QuatT DevInvPose = pJointsAbsoluteDefault[i].GetInverted();
				DevInvPose.t *= m_pInstance->m_fCustomScale[i];
				arrSkinQuat[i] = pJointsAbsolute[i] * DevInvPose;
				arrSkinQuat[i].s = m_pInstance->m_fCustomScale[i];
			}
			else if( m_pInstance->m_IgnoreScale[i] )
			{
				arrSkinQuat[i] = pJointsAbsolute[i]*pJointsAbsoluteDefault[i].GetInverted();//.t/=fScaling;
				arrSkinQuat[i].s = 1.0f;
			}
			else
			{
				QuatT DevInvPose = pJointsAbsoluteDefault[i].GetInverted();
				DevInvPose.t *= fScaling;
				arrSkinQuat[i] = pJointsAbsolute[i] * DevInvPose;
				arrSkinQuat[i].s = fScaling;
			}
		}

		pInstance->m_bUpdateMotionBlurSkinnning = true;
	}


#ifdef VTUNE_PROFILE 
	g_pISystem->VTunePause();
#endif

} 


#if defined(_RELEASE)
#define BBOX_ERROR_CHECKING 0
#else
#define BBOX_ERROR_CHECKING 1
#endif


//////////////////////////////////////////////////////////////////////////
// calculates the bounding box for this instance using the moving skeleton
//////////////////////////////////////////////////////////////////////////
void CSkeletonPose::UpdateBBox(uint32 update)
{

	const QuatT* const __restrict pAbsolutePose = GetPoseData().GetJointsAbsolute();

	// cache m_AABB to stack on SPU
	SpuStackValue<AABB, true, true> stackAabb(m_AABB);
	AABB &rAABB = stackAabb;

	uint32 nErrorCode=0;
	const f32 fMaxBBox=13000.0f;
	rAABB.min.Set(+99999.0f,+99999.0f,+99999.0f);
	rAABB.max.Set(-99999.0f,-99999.0f,-99999.0f);


	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"Update BBox:  NumBones:%d   Model: %s", numJoints, m_pInstance->GetModelFilePath() );	
	//g_YLine+=16.0f;

	const uint32 numCGAJoints = m_arrCGAJoints.size();
	const uint32 numJoints = GetJointCount();
	if (numCGAJoints)
	{
		//if we have a CGA, then us this code
		for (uint32 i=0; i<numCGAJoints; ++i) 
		{
			if (m_arrCGAJoints[i].m_CGAObjectInstance)
			{
				QuatT bone = pAbsolutePose[i];
				AABB aabb = AABB::CreateTransformedAABB(Matrix34(bone),m_arrCGAJoints[i].m_CGAObjectInstance->GetAABB());
				rAABB.Add(aabb);
			}
		}
	}
	else
	{
		//if we have a CHR, then us this code
		for (uint32 i=1; i<numJoints; i++)
		{
			rAABB.Add( pAbsolutePose[i].t );
		}
	}

	if (numCGAJoints==0 && numJoints<=1)
	{
		rAABB.min=Vec3(-2,-2,-2);
		rAABB.max=Vec3( 2, 2, 2);
	}

	//----------------------------------------------------------------------------------------------------------
#if BBOX_ERROR_CHECKING
	uint32 minValid = rAABB.min.IsValid();
	if (minValid==0) nErrorCode|=0x4000;
	uint32 maxValid = rAABB.max.IsValid();
	if (maxValid==0) nErrorCode|=0x4000;
#endif
	//----------------------------------------------------------------------------------------------------------

	IAttachmentManager* pIAttachmentManager = m_pInstance->GetIAttachmentManager();
	if (pIAttachmentManager)
	{

		const uint32 numAttachments = pIAttachmentManager->GetAttachmentCount();
		for (uint32 i=0; i<numAttachments; i++)
		{ 

			CAttachment* pAttachment =  (CAttachment*)pIAttachmentManager->GetInterfaceByIndex(i);
			if (pAttachment)
			{

				if ( (pAttachment->m_Type==CA_BONE || pAttachment->m_Type==CA_FACE) && (pAttachment->CAttachment::IsAttachmentHidden()==0) )
				{

					IAttachmentObject* pIAttachmentObject = pAttachment->CAttachment::GetIAttachmentObject();
					if (pIAttachmentObject)
					{
						IAttachmentObject::EType eAttachmentType = pIAttachmentObject->GetAttachmentType();
						uint32 e = ( eAttachmentType == IAttachmentObject::eAttachment_Entity);
						uint32 c = ( eAttachmentType == IAttachmentObject::eAttachment_Character);
						uint32 s = ( eAttachmentType == IAttachmentObject::eAttachment_StatObj);

						if( e | c | s)
						{	
							AABB aabb = pIAttachmentObject->GetAABB();

#if BBOX_ERROR_CHECKING
							minValid =	aabb.min.IsValid();
							maxValid =	aabb.max.IsValid();
							if (minValid==0 || maxValid==0)
							{
								if( e ) AnimWarning("CryAnimation: Invalid BBox of eAttachment_Entity" );
								if( c ) AnimWarning("CryAnimation: Invalid BBox of eAttachment_Character" );
								if( s ) AnimWarning("CryAnimation: Invalid BBox of eAttachment_StatObj" );
								assert(0);
							}

							nErrorCode=0;
							if (aabb.max.x < aabb.min.x) nErrorCode|=0x0001;
							if (aabb.max.y < aabb.min.y) nErrorCode|=0x0001;
							if (aabb.max.z < aabb.min.z) nErrorCode|=0x0001;
							if (aabb.min.x < -fMaxBBox) nErrorCode|=0x0002;
							if (aabb.min.y < -fMaxBBox) nErrorCode|=0x0004;
							if (aabb.min.z < -fMaxBBox) nErrorCode|=0x0008;
							if (aabb.max.x > +fMaxBBox) nErrorCode|=0x0010;
							if (aabb.max.y > +fMaxBBox) nErrorCode|=0x0020;
							if (aabb.max.z > +fMaxBBox) nErrorCode|=0x0040;

							assert(nErrorCode==0);
							if (nErrorCode)
							{
								if( e )
								{
									IStatObj* pIStatObj =  pIAttachmentObject->GetIStatObj();
									if (pIStatObj)
									{
										const char* strFolderName = pIStatObj->GetFilePath();
										AnimWarning("CryAnimation: Invalid BBox of eAttachment_Entity (%.3f,%.3f,%.3f)-(%.3f,%.3f,%.3f). ModenPath: '%s' ObjPathName '%s'", 	aabb.min.x, aabb.min.y, aabb.min.z, aabb.max.x, aabb.max.y, aabb.max.z, 	m_pInstance->m_pModel->GetModelFilePath(), strFolderName);
									}
									else
									{
										AnimWarning("CryAnimation: Invalid BBox of eAttachment_Entity (%.3f,%.3f,%.3f)-(%.3f,%.3f,%.3f). ModenPath: '%s' ", 	aabb.min.x, aabb.min.y, aabb.min.z, aabb.max.x, aabb.max.y, aabb.max.z, 	m_pInstance->m_pModel->GetModelFilePath());
									}
									assert(0);
								}

								if( c )	AnimWarning("CryAnimation: Invalid BBox of eAttachment_Character (%.3f,%.3f,%.3f)-(%.3f,%.3f,%.3f). ModenPath: '%s'  ErrorCode: %08x", 	aabb.min.x, aabb.min.y, aabb.min.z, aabb.max.x, aabb.max.y, aabb.max.z, 	m_pInstance->m_pModel->GetModelFilePath(), nErrorCode);
								if( s ) AnimWarning("CryAnimation: Invalid BBox of eAttachment_StatObj (%.3f,%.3f,%.3f)-(%.3f,%.3f,%.3f). ModenPath: '%s'  ErrorCode: %08x", 	aabb.min.x, aabb.min.y, aabb.min.z, aabb.max.x, aabb.max.y, aabb.max.z, 	m_pInstance->m_pModel->GetModelFilePath(), nErrorCode);
							}
#endif

							assert(aabb.min.IsValid());
							assert(aabb.max.IsValid());

							AABB taabb; taabb.SetTransformedAABB(pAttachment->CAttachment::GetAttModelRelative(), aabb);

							assert(taabb.min.IsValid());
							rAABB.Add(taabb.min);

							assert(taabb.max.IsValid());
							rAABB.Add(taabb.max);
						}
					}
				}
			}
		}
	}

#if BBOX_ERROR_CHECKING
	if (rAABB.max.x < rAABB.min.x) nErrorCode=0x0001;
	if (rAABB.max.y < rAABB.min.y) nErrorCode=0x0001;
	if (rAABB.max.z < rAABB.min.z) nErrorCode=0x0001;

	minValid = rAABB.min.IsValid();
	if (minValid==0) nErrorCode|=0x8000;
	maxValid = rAABB.max.IsValid();
	if (maxValid==0) nErrorCode|=0x8000;


	if (rAABB.min.x < -fMaxBBox) nErrorCode|=0x0002;
	if (rAABB.min.y < -fMaxBBox) nErrorCode|=0x0004;
	if (rAABB.min.z < -fMaxBBox) nErrorCode|=0x0008;
	if (rAABB.max.x > +fMaxBBox) nErrorCode|=0x0010;
	if (rAABB.max.y > +fMaxBBox) nErrorCode|=0x0020;
	if (rAABB.max.z > +fMaxBBox) nErrorCode|=0x0040;
	assert(nErrorCode==0);
#endif

	const float fSelectX = (rAABB.max.x - rAABB.min.x) - 0.4f;
	const float fSelectY = (rAABB.max.y - rAABB.min.y) - 0.4f;
	const float fSelectZ = (rAABB.max.z - rAABB.min.z) - 0.4f;

	rAABB.max.x = (float)__fsel(fSelectX, rAABB.max.x, rAABB.max.x + 0.2f);
	rAABB.min.x = (float)__fsel(fSelectX, rAABB.min.x, rAABB.min.x - 0.2f);
	rAABB.max.y = (float)__fsel(fSelectY, rAABB.max.y, rAABB.max.y + 0.2f);
	rAABB.min.y = (float)__fsel(fSelectY, rAABB.min.y, rAABB.min.y - 0.2f);
	rAABB.max.z = (float)__fsel(fSelectZ, rAABB.max.z, rAABB.max.z + 0.2f);
	rAABB.min.z = (float)__fsel(fSelectZ, rAABB.min.z, rAABB.min.z - 0.2f);

#if BBOX_ERROR_CHECKING
	if (nErrorCode)
	{
		AnimWarning("CryAnimation: Invalid BBox (%.3f,%.3f,%.3f)-(%.3f,%.3f,%.3f). ModenPath: '%s'  ErrorCode: %08x", 	rAABB.min.x, rAABB.min.y, rAABB.min.z, rAABB.max.x, rAABB.max.y, rAABB.max.z, 	m_pInstance->m_pModel->GetModelFilePath(), nErrorCode);
		assert(!"Invalid BBox");
		rAABB.min=Vec3(-2,-2,-2);
		rAABB.max=Vec3( 2, 2, 2);
	}
#endif
}


void CSkeletonPose::DrawArrowArc( const QuatT& location, const f32 turnAngle, const f32 length, const f32 slope, const ColorB& col )
{
	f32 mSign = turnAngle >.0f? 1.0f : -1.0f;

	SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
	g_pAuxGeom->SetRenderFlags( renderFlags );

	Vec3 uprightVec(.0f, 0.0f, 1.0f);
	Matrix33 ma = Matrix33(location.q);//*m;
	Vec3 Va = ma * Vec3(.0f, 1.0f, .0f);

	//------------------------------------------------------------------------------
	// zero turning angles, straight-forward
	if(fabs(turnAngle) < 1e-10)
	{
		g_pAuxGeom->DrawLine(location.t, col, location.t + Va*length, col, 3.0f); //DrawOBB(obb,O,1,col,eBBD_Extremes_Color_Encoded);
		g_pAuxGeom->DrawCone(location.t + Va*length, Va, 0.03f,0.15f,col);
		return;
	}
	//------------------------------------------------------------------------------

	f32 r = fabs(length / turnAngle); // radius

	Matrix33 mb = Matrix33::CreateRotationAA(mSign* gf_PI/2.0f, Vec3(.0f, .0f, 1.0f));
	Vec3 Vt = mb*Va;
	Vt *= r;

	//g_pAuxGeom->DrawLine(location.t, RGBA8(0x00, 0xff, 0x00, 0xff), location.t + Vt, RGBA8(0x00, 0xff, 0x00, 0xff), 10.0f); //DrawOBB(obb,O,1,col,eBBD_Extremes_Color_Encoded);

	Vec3 O = location.t + Vt;
	Vec3 Vd = - Vt;

	static f32 arcSegNum = 30.0f;
	f32 stepSize = fabs(turnAngle) / arcSegNum;
	f32 offsetZ = 0.009f;
	for(f32 s=0.0f; s<fabs(turnAngle)-stepSize; s += stepSize)
	{
		Matrix33 m1 = Matrix33::CreateRotationAA(mSign* s, uprightVec);
		Matrix33 m2 = Matrix33::CreateRotationAA(mSign*(s+stepSize), uprightVec);
		Matrix33 mSlope = Matrix33::CreateRotationAA(mSign*slope, Vt.GetNormalized());

		Vec3 a = m1 * Vd;
		Vec3 b = m2 * Vd;

		g_pAuxGeom->DrawLine(mSlope*a + O + Vec3(.0f, .0f, offsetZ), col, mSlope*b + O + Vec3(.0f, .0f, offsetZ), col, 10.0f); //DrawOBB(obb,O,1,col,eBBD_Extremes_Color_Encoded);

		if( !( (s + stepSize) < (fabs(turnAngle)-stepSize) ) )
			g_pAuxGeom->DrawCone(mSlope*b+O, mSlope*(b-a), 0.05f,0.2f,col);
	}
}



//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
void CSkeletonPose::DrawArrow( const QuatT& location, const Vec3& vTravelDir, f32 length, ColorB col )
{
	SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
	g_pAuxGeom->SetRenderFlags( renderFlags );

	Vec3 absAxisY	=	location.q.GetColumn1();

	Vec3 vdir=vTravelDir.GetNormalized();
	Matrix33 m;
	m.m00=1;	m.m01=0;	m.m02=0;
	m.m10=0;	m.m11=0;	m.m12=-1;
	m.m20=0;	m.m21=1;	m.m22=0;
	f64 l = sqrt(vdir.x*vdir.x + vdir.y*vdir.y);
	if (l>0.0001)	
	{
		f32 rad			= f32( atan2(-vdir.z*(vdir.y/l),l) );
		m.SetRotationX(-rad);
	}

	//----------------------------------------------------------------------

	const f32 scale = 1.0f;
	const f32 size=0.009f;
	AABB yaabb = AABB(Vec3(-size*scale, -0.0f*scale, -size*scale),Vec3(size*scale,	 length*scale, size*scale));

	Matrix33 m2=Matrix33(location.q)*m;
	OBB obb =	OBB::CreateOBBfromAABB( Matrix33(location.q)*m, yaabb );
	g_pAuxGeom->DrawOBB(obb,location.t,1,col,eBBD_Extremes_Color_Encoded);

	if (l>0.0001)	
	{
		f64 xl=-vdir.x/l; f64 yl=vdir.y/l;
		m.m00=f32(yl);	m.m01=f32(vdir.x);		m.m02=f32(xl*vdir.z);
		m.m10=f32(xl);	m.m11=f32(vdir.y);		m.m12=f32(-vdir.z*yl);
		m.m20=0;				m.m21=f32(vdir.z);		m.m22=f32(l);
	}
	g_pAuxGeom->DrawCone(m2*(Vec3(0,1,0)*length*scale)+location.t, m2*Vec3(0,1,0),0.03f,0.15f,col);


}

void CSkeletonPose::AdjustFeetHelper( QuatT &rAbsoluteCalf, QuatT &rRelativeCalf, QuatT &rAbsoluteTight, QuatT &rAbsoluteFoot,QuatT &rRelativeFoot, const Vec3 &rCross, const Vec3 &rKneeDir, uint32 iteration, float knee_factor )
{
	for (uint32 i=0; i<iteration; i++)
	{
		Vec3 ulleg = rAbsoluteCalf.t - rAbsoluteTight.t;
		Vec3 llleg = rAbsoluteFoot.t - rAbsoluteCalf.t;

		Vec3 lcross = ulleg%llleg;

		if ( (lcross|lcross)>0.0001f && (rCross|lcross)>0)
			break;

		rAbsoluteCalf.t += rKneeDir*knee_factor;
	}

	rRelativeCalf = rAbsoluteTight.GetInverted() 	* rAbsoluteCalf;
	rRelativeFoot = rAbsoluteCalf.GetInverted() 	* rAbsoluteFoot;
}

void CSkeletonPose::PoseModifiersExecute(const QuatT& locationNextPhysics, const QuatTS& locationNextAnimation)
{
	SAnimationPoseModiferParams params;
	params.pCharacterInstance = m_pInstance;
	params.timeDelta = m_pInstance->m_fOriginalDeltaTime;
	params.locationNextPhysics = locationNextPhysics;
	params.locationNextAnimation = locationNextAnimation;
	params.pPoseRelative = &GetPoseDataWriteable()->m_jointsRelative[0];
	params.pPoseAbsolute = &GetPoseDataWriteable()->m_jointsAbsolute[0];
	params.jointCount = GetJointCount();

	const uint32 poseModifierCount = uint32(m_poseModifiers.size());
	for (uint32 i=0; i<poseModifierCount; ++i)
	{
		if (!m_poseModifiers[i]->Execute(params))
			m_bAllAbsoluteJointsValid = true;
	}

	m_poseModifiers.clear();
}
