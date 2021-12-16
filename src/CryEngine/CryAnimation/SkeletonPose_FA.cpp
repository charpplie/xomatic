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

#include "CharacterManager.h"
#include "CharacterInstance.h"
#include "Model.h"
#include "ModelSkeleton.h"
#include <float.h>

#define SAFEANIM_IDX(a)	((a)!=0xffffffff ? (a) : 0)


//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
void CSkeletonPose::FootAnchoring( const SFootPlant& BFPlant, uint8 Foot, const QuatTS& rAnimCharLocationCurr )
{

	if (m_pSkeletonAnim->m_arrLayer_AFIFO[0].size()==0)
		return;
	if (m_pSkeletonAnim->m_AnimationDrivenMotion==0)
		return;

	// use local pointers to spare dereferencing
	int16* 					const	__restrict	pIdxArray 	= m_pModelSkeleton->m_IdxArray;
		
	// use spu local memory for often used arrays
	QuatT* 		const	__restrict	pRelativePose = SPU_PTR_SELECT( &GetPoseDataWriteable()->m_jointsRelative[0], gSkeletonRelativePose );
	QuatT* 		const	__restrict	pAbsolutePose = SPU_PTR_SELECT( &GetPoseDataWriteable()->m_jointsAbsolute[0], gSkeletonAbsolutePose );
	SFootAnchor 								&rFootAnchor 	= SPU_REF_SELECT( m_FootAnchor, *gSkeletonFootAnchor );

	uint32 FVE  = rFootAnchor.m_UseFootAnchoring;
	if (FVE==0 || m_timeStandingUp >= 0.0f) 
		return; 
	if (m_bPhysicsRelinquished==1)
		return; //its under control of physics

	uint8 FootPlant = Foot;

	f32 AnimTimeLayer0	= m_pSkeletonAnim->m_arrLayer_AFIFO[0][0].m_fAnimTime; //we assume LMGs are all in layer 0

	f32 tl = AnimTimeLayer0+0.5f; if (tl>1.0f) tl=tl-1.0f;
	f32 tr = AnimTimeLayer0;
	if ( tl>BFPlant.m_LHeelStart && tl<BFPlant.m_LHeelEnd ) FootPlant |= LHEEL;
	if ( tl>BFPlant.m_LToe0Start && tl<BFPlant.m_LToe0End ) FootPlant |= LTOE0;
	if ( tr>BFPlant.m_RHeelStart && tr<BFPlant.m_RHeelEnd ) FootPlant |= RHEEL;
	if ( tr>BFPlant.m_RToe0Start && tr<BFPlant.m_RToe0End ) FootPlant |= RTOE0;

	//float fColor[4] = {1,0,1,1};
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"FootAnchoring:  StandUp: %f  Relinquish: %d ",m_timeStandingUp,m_bPhysicsRelinquished );	
	//g_YLine+=10;

	//rFootAnchor.m_Toe0Slide = Vec3(ZERO);
	//rFootAnchor.m_HeelSlide = Vec3(ZERO);

	int32 PelvisIdx			= SAFEANIM_IDX(pIdxArray[eIM_PelvisIdx]);

	int32 lThighIdx			= SAFEANIM_IDX(pIdxArray[eIM_LThighIdx]);
	int32 lCalfIdx			= SAFEANIM_IDX(pIdxArray[eIM_LCalfIdx]);
	int32 lFootIdx			= SAFEANIM_IDX(pIdxArray[eIM_LFootIdx]);
	int32 lHeelIdx			= SAFEANIM_IDX(pIdxArray[eIM_LHeelIdx]);
	int32 lToe0Idx			= SAFEANIM_IDX(pIdxArray[eIM_LToe0Idx]);


	int32 rThighIdx			= SAFEANIM_IDX(pIdxArray[eIM_RThighIdx]);
	int32 rCalfIdx			= SAFEANIM_IDX(pIdxArray[eIM_RCalfIdx]);
	int32 rFootIdx			= SAFEANIM_IDX(pIdxArray[eIM_RFootIdx]);
	int32 rHeelIdx			= SAFEANIM_IDX(pIdxArray[eIM_RHeelIdx]);
	int32 rToe0Idx			= SAFEANIM_IDX(pIdxArray[eIM_RToe0Idx]);

	uint32 LeftPlant=0;
	uint32 RightPlant=0;
	if (FootPlant&LTOE0 && FootPlant&LHEEL)
	{
		LeftPlant=1; //DUBLY CONSTRAINT on the left foot
		rFootAnchor.m_LAnkleIntensity=1.0f;

		Vec2 xaxis0 = Vec2(rFootAnchor.m_AbsoluteLToe0.t-rFootAnchor.m_AbsoluteLHeel.t);	//xaxis0.z=0; xaxis0.Normalize();
		Vec2 xaxis1 = Vec2(rFootAnchor.m_LLastToe-rFootAnchor.m_LLastHeel);								//xaxis1.z=0; xaxis1.NormalizeSafe(Vec3(1,0,0));
		f32 yaw	= Ang3::CreateRadZ(xaxis0,xaxis1);
		if (yaw<-1.2f) yaw=-1.2f;
		if (yaw>+1.8f) yaw=+1.8f;
		rFootAnchor.m_fLAnkle=yaw;

		Quat LThigh = Quat::CreateRotationZ(rFootAnchor.m_fLAnkle*0.5f);
		rFootAnchor.m_AbsoluteLThigh.q	= LThigh*rFootAnchor.m_AbsoluteLThigh.q;
		pRelativePose[lThighIdx]				= rFootAnchor.m_AbsolutePelvis.GetInverted() * rFootAnchor.m_AbsoluteLThigh;
		rFootAnchor.m_AbsoluteLCalf		= rFootAnchor.m_AbsoluteLThigh * pRelativePose[lCalfIdx];
		rFootAnchor.m_AbsoluteLFoot		= rFootAnchor.m_AbsoluteLCalf  * pRelativePose[lFootIdx];
		rFootAnchor.m_AbsoluteLHeel		= rFootAnchor.m_AbsoluteLFoot  * pRelativePose[lHeelIdx];
		rFootAnchor.m_AbsoluteLToe0		= rFootAnchor.m_AbsoluteLFoot  * pRelativePose[lToe0Idx];

		xaxis0 = Vec2(rFootAnchor.m_AbsoluteLToe0.t-rFootAnchor.m_AbsoluteLHeel.t);
		xaxis1 = Vec2(rFootAnchor.m_LLastToe-rFootAnchor.m_LLastHeel);
		yaw	= Ang3::CreateRadZ(xaxis0,xaxis1);
		if (yaw<-1.2f) yaw=-1.2f;
		if (yaw>+1.8f) yaw=+1.8f;
		Quat LAnkle = Quat::CreateRotationZ(yaw);
		rFootAnchor.m_AbsoluteLFoot.q	= LAnkle*rFootAnchor.m_AbsoluteLFoot.q;
		pRelativePose[lFootIdx]				= rFootAnchor.m_AbsoluteLCalf.GetInverted() * rFootAnchor.m_AbsoluteLFoot;
		rFootAnchor.m_AbsoluteLHeel		= rFootAnchor.m_AbsoluteLFoot * pRelativePose[lHeelIdx];
		rFootAnchor.m_AbsoluteLToe0		= rFootAnchor.m_AbsoluteLFoot * pRelativePose[lToe0Idx];

#if !defined(__SPU__)
		if (Console::GetInst().ca_DebugFootPlants)
		{
			float fColor[4] = {0,1,0,1};
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"dubly constraint on the left foot: %f",yaw );	g_YLine+=16.0f;
		}
#endif
		
		rFootAnchor.m_LHeelSlide +=(rFootAnchor.m_AbsoluteLHeel.t-rFootAnchor.m_LLastHeel);
		rFootAnchor.m_LHeelSlide.z=0;
	} 



	//-----------------------------------------------------------------------------------------------------

	if (FootPlant&RTOE0 && FootPlant&RHEEL)
	{
		RightPlant=1;  //DUBLY CONSTRAINT on the right foot
		rFootAnchor.m_RAnkleIntensity=1.0f;


		Vec2 xaxis0 = Vec2(rFootAnchor.m_AbsoluteRToe0.t-rFootAnchor.m_AbsoluteRHeel.t);	//xaxis0.z=0; xaxis0.Normalize();
		Vec2 xaxis1 = Vec2(rFootAnchor.m_RLastToe-rFootAnchor.m_RLastHeel);								//xaxis1.z=0; xaxis1.NormalizeSafe(Vec3(1,0,0));
		f32 yaw	= Ang3::CreateRadZ(xaxis0,xaxis1);
		if (yaw<-1.8f) yaw=-1.8f;
		if (yaw>+1.2f) yaw=+1.2f;
		rFootAnchor.m_fRAnkle=yaw;

		Quat RThigh = Quat::CreateRotationZ(rFootAnchor.m_fRAnkle*0.5f);
		rFootAnchor.m_AbsoluteRThigh.q	= RThigh*rFootAnchor.m_AbsoluteRThigh.q;
		pRelativePose[rThighIdx]				= rFootAnchor.m_AbsolutePelvis.GetInverted() * rFootAnchor.m_AbsoluteRThigh;
		rFootAnchor.m_AbsoluteRCalf		= rFootAnchor.m_AbsoluteRThigh * pRelativePose[rCalfIdx];
		rFootAnchor.m_AbsoluteRFoot		= rFootAnchor.m_AbsoluteRCalf  * pRelativePose[rFootIdx];
		rFootAnchor.m_AbsoluteRHeel		= rFootAnchor.m_AbsoluteRFoot  * pRelativePose[rHeelIdx];
		rFootAnchor.m_AbsoluteRToe0		= rFootAnchor.m_AbsoluteRFoot  * pRelativePose[rToe0Idx];

		xaxis0 = Vec2(rFootAnchor.m_AbsoluteRToe0.t-rFootAnchor.m_AbsoluteRHeel.t);	//xaxis0.z=0; xaxis0.Normalize();
		xaxis1 = Vec2(rFootAnchor.m_RLastToe-rFootAnchor.m_RLastHeel);								//xaxis1.z=0; xaxis1.NormalizeSafe(Vec3(1,0,0));
		yaw	= Ang3::CreateRadZ(xaxis0,xaxis1);
		if (yaw<-1.8f) yaw=-1.8f;
		if (yaw>+1.2f) yaw=+1.2f;
		Quat RAnkle = Quat::CreateRotationZ(yaw);
		rFootAnchor.m_AbsoluteRFoot.q	= RAnkle*rFootAnchor.m_AbsoluteRFoot.q;
		pRelativePose[rFootIdx]				= rFootAnchor.m_AbsoluteRCalf.GetInverted() * rFootAnchor.m_AbsoluteRFoot;
		rFootAnchor.m_AbsoluteRHeel		= rFootAnchor.m_AbsoluteRFoot * pRelativePose[rHeelIdx];
		rFootAnchor.m_AbsoluteRToe0		= rFootAnchor.m_AbsoluteRFoot * pRelativePose[rToe0Idx];

#if !defined(__SPU__)
		if (Console::GetInst().ca_DebugFootPlants)
		{
			float fColor[4] = {0,1,0,1};
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"dubly constraint on the right foot: %f",yaw );	g_YLine+=16.0f;
		}
#endif
		
		rFootAnchor.m_RHeelSlide += (rFootAnchor.m_AbsoluteRHeel.t-rFootAnchor.m_RLastHeel);
		rFootAnchor.m_RHeelSlide.z=0;
	} 

	//---------------------------------------------------------------------------------------------------

	if (LeftPlant==0)
	{
		EaseOut_LAnkle();
		//check if we have at least a single constraint
		if (FootPlant&LHEEL && rFootAnchor.m_LAnkleIntensity>0.9999f)
			rFootAnchor.m_LHeelSlide += (rFootAnchor.m_AbsoluteLHeel.t-rFootAnchor.m_LLastHeel);
	}

	if (RightPlant==0)
	{
		EaseOut_RAnkle();
		//check if we have at least a single constraint
		if (FootPlant&RHEEL && rFootAnchor.m_LAnkleIntensity>0.9999f)
			rFootAnchor.m_RHeelSlide += (rFootAnchor.m_AbsoluteRHeel.t-rFootAnchor.m_RLastHeel);
	}


	const f32 MaxDeviation=0.30f;

	f32 lslide = rFootAnchor.m_LHeelSlide.GetLength();
	if (lslide>MaxDeviation)
	{ 
		rFootAnchor.m_LHeelSlide.Normalize();
		rFootAnchor.m_LHeelSlide*=MaxDeviation;
	}
	if (rFootAnchor.m_LAnkleIntensity<0.001f)
		rFootAnchor.m_LHeelSlide=Vec3(ZERO);


	f32 rslide = rFootAnchor.m_RHeelSlide.GetLength();
	if (rslide>MaxDeviation)
	{ 
		rFootAnchor.m_RHeelSlide.Normalize();
		rFootAnchor.m_RHeelSlide*=MaxDeviation;
	}
	if (rFootAnchor.m_RAnkleIntensity<0.001f)
		rFootAnchor.m_RHeelSlide = Vec3(ZERO);






	//-----------------------------------------------------------------------------------
	//-----------------------------------------------------------------------------------
	//-----------------------------------------------------------------------------------

	pAbsolutePose[0]						= pRelativePose[0];
	pAbsolutePose[PelvisIdx]		= pAbsolutePose[0] * pRelativePose[PelvisIdx];


	pAbsolutePose[lThighIdx]		= pAbsolutePose[PelvisIdx] * pRelativePose[lThighIdx];
	pAbsolutePose[lCalfIdx]		= pAbsolutePose[lThighIdx] * pRelativePose[lCalfIdx];
	pAbsolutePose[lFootIdx]		= pAbsolutePose[lCalfIdx]  * pRelativePose[lFootIdx];
	pAbsolutePose[lHeelIdx]		= pAbsolutePose[lFootIdx]  * pRelativePose[lHeelIdx];
	pAbsolutePose[lToe0Idx]		= pAbsolutePose[lFootIdx]  * pRelativePose[lToe0Idx];


	pAbsolutePose[rThighIdx]		= pAbsolutePose[PelvisIdx] * pRelativePose[rThighIdx];
	pAbsolutePose[rCalfIdx]		= pAbsolutePose[rThighIdx] * pRelativePose[rCalfIdx];
	pAbsolutePose[rFootIdx]		= pAbsolutePose[rCalfIdx]	* pRelativePose[rFootIdx];
	pAbsolutePose[rHeelIdx]		= pAbsolutePose[rFootIdx]	* pRelativePose[rHeelIdx];
	pAbsolutePose[rToe0Idx]		= pAbsolutePose[rFootIdx]	* pRelativePose[rToe0Idx];



	/*
	Vec3 wpos = m_pInstance->m_EntityQuatT.t;
	g_pAuxGeom->DrawLine(wpos,RGBA8(0x7f,0xff,0xff,0x00), wpos+footslide*100,RGBA8(0x7f,0xff,0xff,0x00) );

	g_YLine+=32.0f;		
	float fColor[4] = {1,0,1,1};
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"footslide: %f %f ",footslide.x,footslide.y );	
	g_YLine+=10;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"ToeSlide: %f %f ",Toe0Slide.x,Toe0Slide.y );	
	g_YLine+=10;
	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"HeelSlide: %f %f ",HeelSlide.x,HeelSlide.y );	
	g_YLine+=10;
	*/

#if !defined(__SPU__)
	if (Console::GetInst().ca_DrawFootPlants)
	{
		g_YLine+=32.0f;		
		float fColor[4] = {1,0,1,1};
		const char* mname = m_pInstance->GetFilePath();
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.6f, fColor, false,"Model: %s ",mname );	
		g_YLine+=16.0f;
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"LHeelSlide: %f %f  mag:%f ",rFootAnchor.m_LHeelSlide.x*2,rFootAnchor.m_LHeelSlide.y*2, rFootAnchor.m_LHeelSlide.GetLength()*2 );
		g_YLine+=10;
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"RHeelSlide: %f %f  mag:%f ",rFootAnchor.m_RHeelSlide.x*2,rFootAnchor.m_RHeelSlide.y*2, rFootAnchor.m_RHeelSlide.GetLength()*2 );
		g_YLine+=10;
	}

	if (Console::GetInst().ca_DrawFootPlants)
	{
		g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
		static Ang3 angle1(0,0,0); 
		static Ang3 angle2(0,0,0); 
		static Ang3 angle3(0,0,0); 
		angle1 += Ang3(0.01f,0.02f,0.03f);
		angle2 += Ang3(0.01f,-0.12f,0.03f);
		angle3 += Ang3(0.01f,0.01f,-0.03f);
		AABB aabb = AABB(Vec3(-0.05f,-0.05f,-0.05f),Vec3(+0.05f,+0.05f,+0.05f));
		OBB obb1=OBB::CreateOBBfromAABB( Matrix33::CreateRotationXYZ(angle1),aabb );
		OBB obb2=OBB::CreateOBBfromAABB( Matrix33::CreateRotationXYZ(angle2),aabb );
		OBB obb3=OBB::CreateOBBfromAABB( Matrix33::CreateRotationXYZ(angle3),aabb );

		QuatT offset = m_pSkeletonAnim->GetRelMovement();
		if (m_pSkeletonAnim->m_CharEditMode==1)
			offset.t = Vec3(0,0,0);

		QuatT NewAC = QuatT(rAnimCharLocationCurr)*offset;
		if (FootPlant&LHEEL)
		{
			Vec3 vLHeel = NewAC*pAbsolutePose[lHeelIdx].t;
			g_pAuxGeom->DrawOBB(obb1,vLHeel,0,RGBA8(0x00,0x00,0xff,0xff), eBBD_Extremes_Color_Encoded);
			g_pAuxGeom->DrawOBB(obb2,vLHeel,0,RGBA8(0x00,0x00,0xff,0xff), eBBD_Extremes_Color_Encoded);
			g_pAuxGeom->DrawOBB(obb3,vLHeel,0,RGBA8(0x00,0x00,0xff,0xff), eBBD_Extremes_Color_Encoded);
		}
		if (FootPlant&RHEEL)
		{
			Vec3 vRHeel = NewAC*pAbsolutePose[rHeelIdx].t;
			g_pAuxGeom->DrawOBB(obb1,vRHeel,0,RGBA8(0x00,0x00,0xff,0xff), eBBD_Extremes_Color_Encoded);
			g_pAuxGeom->DrawOBB(obb2,vRHeel,0,RGBA8(0x00,0x00,0xff,0xff), eBBD_Extremes_Color_Encoded);
			g_pAuxGeom->DrawOBB(obb3,vRHeel,0,RGBA8(0x00,0x00,0xff,0xff), eBBD_Extremes_Color_Encoded);
		}

		if (FootPlant&LTOE0)
		{
			Vec3 vLToe0 = NewAC*pAbsolutePose[lToe0Idx].t;
			g_pAuxGeom->DrawOBB(obb1,vLToe0,0,RGBA8(0xff,0xff,0x00,0xff), eBBD_Extremes_Color_Encoded);
			g_pAuxGeom->DrawOBB(obb2,vLToe0,0,RGBA8(0xff,0xff,0x00,0xff), eBBD_Extremes_Color_Encoded);
			g_pAuxGeom->DrawOBB(obb3,vLToe0,0,RGBA8(0xff,0xff,0x00,0xff), eBBD_Extremes_Color_Encoded);
		}
		if (FootPlant&RTOE0)
		{
			Vec3 vRToe0 = NewAC*pAbsolutePose[rToe0Idx].t;
			g_pAuxGeom->DrawOBB(obb1,vRToe0,0,RGBA8(0xff,0xff,0x00,0xff), eBBD_Extremes_Color_Encoded);
			g_pAuxGeom->DrawOBB(obb2,vRToe0,0,RGBA8(0xff,0xff,0x00,0xff), eBBD_Extremes_Color_Encoded);
			g_pAuxGeom->DrawOBB(obb3,vRToe0,0,RGBA8(0xff,0xff,0x00,0xff), eBBD_Extremes_Color_Encoded);
		}
	}
#endif
	
}



#define EASEOUT (6.0f)

void CSkeletonPose::EaseOut_LAnkle()
{

	// use local pointers to spare dereferencing
	int16* 					const	__restrict	pIdxArray 	= m_pModelSkeleton->m_IdxArray;
		
	// use spu local memory for often used arrays
	QuatT* 		const	__restrict	pRelativePose = SPU_PTR_SELECT( &GetPoseDataWriteable()->m_jointsRelative[0], gSkeletonRelativePose );
	SFootAnchor 								&rFootAnchor 	= SPU_REF_SELECT( m_FootAnchor, *gSkeletonFootAnchor );
		
	
	//	return;

	int32 lThighIdx		= SAFEANIM_IDX(pIdxArray[eIM_LThighIdx]);
	int32 lCalfIdx		= SAFEANIM_IDX(pIdxArray[eIM_LCalfIdx]);
	int32 lFootIdx		=	SAFEANIM_IDX(pIdxArray[eIM_LFootIdx]);
	int32 lHeelIdx		=	SAFEANIM_IDX(pIdxArray[eIM_LHeelIdx]);
	int32 lToe0Idx		=	SAFEANIM_IDX(pIdxArray[eIM_LToe0Idx]);
	assert(rFootAnchor.m_LAnkleIntensity<=1.0f);
	rFootAnchor.m_LAnkleIntensity -= EASEOUT*fabsf(m_pInstance->m_fDeltaTime*m_pSkeletonAnim->m_arrLayerSpeedMultiplier[0]);
	if (rFootAnchor.m_LAnkleIntensity<0) rFootAnchor.m_LAnkleIntensity=0;
	if (rFootAnchor.m_LAnkleIntensity)
	{
		f32 t = -(2*rFootAnchor.m_LAnkleIntensity*rFootAnchor.m_LAnkleIntensity*rFootAnchor.m_LAnkleIntensity - 3*rFootAnchor.m_LAnkleIntensity*rFootAnchor.m_LAnkleIntensity);
		assert(t<=1.0f);
		assert(t>=0.0f);

#if !defined(__SPU__)
		if (Console::GetInst().ca_DrawFootPlants)
		{
			float fColor[4] = {0,1,0,1};
			extern float g_YLine;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"EaseOut_LAnkle %f",t);	g_YLine+=16.0f;
		}
#endif

		Quat LAnkle = Quat::CreateRotationZ(rFootAnchor.m_fLAnkle*0.5f);
		rFootAnchor.m_AbsoluteLThigh.q		= Quat::CreateNlerp(IDENTITY,LAnkle,t)*rFootAnchor.m_AbsoluteLThigh.q;
		pRelativePose[lThighIdx]					= rFootAnchor.m_AbsolutePelvis.GetInverted() * rFootAnchor.m_AbsoluteLThigh;
		rFootAnchor.m_AbsoluteLFoot.q		= Quat::CreateNlerp(IDENTITY,LAnkle,t)*rFootAnchor.m_AbsoluteLFoot.q;
		pRelativePose[lFootIdx]					= rFootAnchor.m_AbsoluteLCalf.GetInverted() * rFootAnchor.m_AbsoluteLFoot;

		rFootAnchor.m_AbsoluteLCalf		= rFootAnchor.m_AbsoluteLThigh * pRelativePose[lCalfIdx];
		rFootAnchor.m_AbsoluteLFoot		= rFootAnchor.m_AbsoluteLCalf  * pRelativePose[lFootIdx];
		rFootAnchor.m_AbsoluteLHeel		= rFootAnchor.m_AbsoluteLFoot  * pRelativePose[lHeelIdx];
		rFootAnchor.m_AbsoluteLToe0		= rFootAnchor.m_AbsoluteLFoot  * pRelativePose[lToe0Idx];
	}
}

void CSkeletonPose::EaseOut_RAnkle()
{
	// use local pointers to spare dereferencing
	int16* 					const	__restrict	pIdxArray 	= m_pModelSkeleton->m_IdxArray;

	// use spu local memory for often used arrays
	QuatT* 		const	__restrict	pRelativePose = SPU_PTR_SELECT( &GetPoseDataWriteable()->m_jointsRelative[0], gSkeletonRelativePose );
	SFootAnchor 								&rFootAnchor 	= SPU_REF_SELECT( m_FootAnchor, *gSkeletonFootAnchor );
	
	
	//	return;
	int32 rThighIdx		= SAFEANIM_IDX(pIdxArray[eIM_RThighIdx]);
	int32 rCalfIdx		= SAFEANIM_IDX(pIdxArray[eIM_RCalfIdx]);
	int32 rFootIdx		=	SAFEANIM_IDX(pIdxArray[eIM_RFootIdx]);
	int32 rHeelIdx		=	SAFEANIM_IDX(pIdxArray[eIM_RHeelIdx]);
	int32 rToe0Idx		=	SAFEANIM_IDX(pIdxArray[eIM_RToe0Idx]);
	assert(rFootAnchor.m_RAnkleIntensity<=1.0f);
	rFootAnchor.m_RAnkleIntensity -= EASEOUT*fabsf(m_pInstance->m_fDeltaTime*m_pSkeletonAnim->m_arrLayerSpeedMultiplier[0]);
	if (rFootAnchor.m_RAnkleIntensity<0) rFootAnchor.m_RAnkleIntensity=0;
	if (rFootAnchor.m_RAnkleIntensity)
	{
		f32 t = -(2*rFootAnchor.m_RAnkleIntensity*rFootAnchor.m_RAnkleIntensity*rFootAnchor.m_RAnkleIntensity - 3*rFootAnchor.m_RAnkleIntensity*rFootAnchor.m_RAnkleIntensity);
		assert(t<=1.0f);
		assert(t>=0.0f);
#if !defined(__SPU__)
		if (Console::GetInst().ca_DrawFootPlants)
		{
			float fColor[4] = {0,1,0,1};
			extern float g_YLine;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"EaseOut_RAnkle %f",t);	g_YLine+=16.0f;
		}		
#endif

		Quat RAnkle = Quat::CreateRotationZ(rFootAnchor.m_fRAnkle*0.5f);
		rFootAnchor.m_AbsoluteRThigh.q				= Quat::CreateNlerp(IDENTITY,RAnkle,t)*rFootAnchor.m_AbsoluteRThigh.q;
		pRelativePose[rThighIdx]	= rFootAnchor.m_AbsolutePelvis.GetInverted() * rFootAnchor.m_AbsoluteRThigh;
		rFootAnchor.m_AbsoluteRFoot.q				= Quat::CreateNlerp(IDENTITY,RAnkle,t)*rFootAnchor.m_AbsoluteRFoot.q;
		pRelativePose[rFootIdx]	= rFootAnchor.m_AbsoluteRCalf.GetInverted() * rFootAnchor.m_AbsoluteRFoot;

		rFootAnchor.m_AbsoluteRCalf		= rFootAnchor.m_AbsoluteRThigh * pRelativePose[rCalfIdx];
		rFootAnchor.m_AbsoluteRFoot		= rFootAnchor.m_AbsoluteRCalf  * pRelativePose[rFootIdx];
		rFootAnchor.m_AbsoluteRHeel		= rFootAnchor.m_AbsoluteRFoot  * pRelativePose[rHeelIdx];
		rFootAnchor.m_AbsoluteRToe0		= rFootAnchor.m_AbsoluteRFoot  * pRelativePose[rToe0Idx];
	/*	if (rKneeIdx>0 && rKneeEndIdx>0)
		{
			GetPoseDataWriteable()->m_jointsAbsolute[rKneeIdx]		= GetPoseDataWriteable()->m_jointsAbsolute[rThighIdx]	* pRelativePose[rKneeIdx];
			GetPoseDataWriteable()->m_jointsAbsolute[rKneeEndIdx]	= GetPoseDataWriteable()->m_jointsAbsolute[rKneeIdx]	* pRelativePose[rKneeEndIdx];
		}*/
	}
}



void CSkeletonPose::AbsoluteLegPosition( const QuatTS& rAnimCharLocationOld )
{

	// use local pointers to spare dereferencing
	int16* 					const	__restrict	pIdxArray 	= m_pModelSkeleton->m_IdxArray;

	// use spu local memory for often used arrays
	QuatT* 		const	__restrict	pRelativePose = SPU_PTR_SELECT( &GetPoseDataWriteable()->m_jointsRelative[0], gSkeletonRelativePose );
	SFootAnchor 								&rFootAnchor 	= SPU_REF_SELECT( m_FootAnchor, *gSkeletonFootAnchor );	
	
	uint32 mPidx=SAFEANIM_IDX(pIdxArray[eIM_PelvisIdx]);

	uint32 lThighIdx	=	SAFEANIM_IDX(pIdxArray[eIM_LThighIdx]);
	uint32 lCalfIdx		=	SAFEANIM_IDX(pIdxArray[eIM_LCalfIdx]);
	uint32 lFootIdx		=	SAFEANIM_IDX(pIdxArray[eIM_LFootIdx]);
	uint32 lHeelIdx		=	SAFEANIM_IDX(pIdxArray[eIM_LHeelIdx]);
	uint32 lToe0Idx		=	SAFEANIM_IDX(pIdxArray[eIM_LToe0Idx]);

	uint32 rThighIdx	=	SAFEANIM_IDX(pIdxArray[eIM_RThighIdx]);
	uint32 rCalfIdx		=	SAFEANIM_IDX(pIdxArray[eIM_RCalfIdx]);
	uint32 rFootIdx		=	SAFEANIM_IDX(pIdxArray[eIM_RFootIdx]);
	uint32 rHeelIdx		=	SAFEANIM_IDX(pIdxArray[eIM_RHeelIdx]);
	uint32 rToe0Idx		=	SAFEANIM_IDX(pIdxArray[eIM_RToe0Idx]);


	Vec3 trans = rAnimCharLocationOld.q * GetLocator().m_translationRelative;
	rFootAnchor.m_absSkeletonInWorld -= trans;


//	QuatT Pelvis = pRelativePose[mPidx];

	rFootAnchor.m_AbsoluteRoot.q			= pRelativePose[0].q * rAnimCharLocationOld.q;

	rFootAnchor.m_AbsoluteRoot.t.x		=	rFootAnchor.m_absSkeletonInWorld.x;
	rFootAnchor.m_AbsoluteRoot.t.y		=	rFootAnchor.m_absSkeletonInWorld.y;
	rFootAnchor.m_AbsoluteRoot.t.z		=	pRelativePose[0].t.z;

	rFootAnchor.m_AbsolutePelvis		= rFootAnchor.m_AbsoluteRoot   * pRelativePose[mPidx]; //m_RelativePelvis;

	rFootAnchor.m_AbsoluteLThigh		= rFootAnchor.m_AbsolutePelvis * pRelativePose[lThighIdx];
	rFootAnchor.m_AbsoluteLCalf		= rFootAnchor.m_AbsoluteLThigh * pRelativePose[lCalfIdx];
	rFootAnchor.m_AbsoluteLFoot		= rFootAnchor.m_AbsoluteLCalf  * pRelativePose[lFootIdx];
	rFootAnchor.m_AbsoluteLHeel		= rFootAnchor.m_AbsoluteLFoot  * pRelativePose[lHeelIdx];
	rFootAnchor.m_AbsoluteLToe0		= rFootAnchor.m_AbsoluteLFoot  * pRelativePose[lToe0Idx];

	rFootAnchor.m_AbsoluteRThigh		= rFootAnchor.m_AbsolutePelvis * pRelativePose[rThighIdx];
	rFootAnchor.m_AbsoluteRCalf		= rFootAnchor.m_AbsoluteRThigh * pRelativePose[rCalfIdx];
	rFootAnchor.m_AbsoluteRFoot		= rFootAnchor.m_AbsoluteRCalf  * pRelativePose[rFootIdx];
	rFootAnchor.m_AbsoluteRHeel		= rFootAnchor.m_AbsoluteRFoot  * pRelativePose[rHeelIdx];
	rFootAnchor.m_AbsoluteRToe0		= rFootAnchor.m_AbsoluteRFoot  * pRelativePose[rToe0Idx];

	rFootAnchor.m_AbsoluteRoot.q.Normalize();
	rFootAnchor.m_AbsolutePelvis.q.Normalize();

	rFootAnchor.m_AbsoluteLThigh.q.Normalize();
	rFootAnchor.m_AbsoluteLCalf.q.Normalize();
	rFootAnchor.m_AbsoluteLFoot.q.Normalize();
	rFootAnchor.m_AbsoluteLHeel.q.Normalize();
	rFootAnchor.m_AbsoluteLToe0.q.Normalize();

	rFootAnchor.m_AbsoluteRThigh.q.Normalize();
	rFootAnchor.m_AbsoluteRCalf.q.Normalize();
	rFootAnchor.m_AbsoluteRFoot.q.Normalize();
	rFootAnchor.m_AbsoluteRHeel.q.Normalize();
	rFootAnchor.m_AbsoluteRToe0.q.Normalize();


	/*
	g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
	g_pAuxGeom->DrawLine(rFootAnchor.m_AbsoluteLThigh.t,RGBA8(0xff,0x00,0x00,0),	rFootAnchor.m_AbsolutePelvis.t,RGBA8(0xff,0xff,0xff,0));
	g_pAuxGeom->DrawLine(rFootAnchor.m_AbsoluteLCalf.t, RGBA8(0xff,0x00,0x00,0),	rFootAnchor.m_AbsoluteLThigh.t,RGBA8(0xff,0xff,0xff,0));
	g_pAuxGeom->DrawLine(rFootAnchor.m_AbsoluteLFoot.t, RGBA8(0xff,0x00,0x00,0),	rFootAnchor.m_AbsoluteLCalf.t, RGBA8(0xff,0xff,0xff,0));
	g_pAuxGeom->DrawLine(rFootAnchor.m_AbsoluteLHeel.t, RGBA8(0xff,0x00,0x00,0),	rFootAnchor.m_AbsoluteLFoot.t, RGBA8(0xff,0xff,0xff,0));
	g_pAuxGeom->DrawLine(rFootAnchor.m_AbsoluteLToe0.t, RGBA8(0xff,0x00,0x00,0),	rFootAnchor.m_AbsoluteLFoot.t, RGBA8(0xff,0xff,0xff,0));
	g_pAuxGeom->DrawLine(rFootAnchor.m_AbsoluteLNub0.t, RGBA8(0xff,0x00,0x00,0),	rFootAnchor.m_AbsoluteLToe0.t, RGBA8(0xff,0xff,0xff,0));

	g_pAuxGeom->DrawLine(rFootAnchor.m_AbsoluteRThigh.t,RGBA8(0xff,0x00,0x00,0),	rFootAnchor.m_AbsolutePelvis.t,RGBA8(0xff,0xff,0xff,0));
	g_pAuxGeom->DrawLine(rFootAnchor.m_AbsoluteRCalf.t, RGBA8(0xff,0x00,0x00,0), rFootAnchor.m_AbsoluteRThigh.t,RGBA8(0xff,0xff,0xff,0));
	g_pAuxGeom->DrawLine(rFootAnchor.m_AbsoluteRFoot.t, RGBA8(0xff,0x00,0x00,0), rFootAnchor.m_AbsoluteRCalf.t, RGBA8(0xff,0xff,0xff,0));
	g_pAuxGeom->DrawLine(rFootAnchor.m_AbsoluteRHeel.t, RGBA8(0xff,0x00,0x00,0), rFootAnchor.m_AbsoluteRFoot.t, RGBA8(0xff,0xff,0xff,0));
	g_pAuxGeom->DrawLine(rFootAnchor.m_AbsoluteRToe0.t, RGBA8(0xff,0x00,0x00,0), rFootAnchor.m_AbsoluteRFoot.t, RGBA8(0xff,0xff,0xff,0));
	g_pAuxGeom->DrawLine(rFootAnchor.m_AbsoluteRNub0.t, RGBA8(0xff,0x00,0x00,0), rFootAnchor.m_AbsoluteRToe0.t, RGBA8(0xff,0xff,0xff,0));
*/	
}

