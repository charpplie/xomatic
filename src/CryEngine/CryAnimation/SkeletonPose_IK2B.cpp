//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:Skeleton.cpp
//  Implementation of Skeleton class (Inverse Kinematics)
//
//	History:
//	March 18, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//	December 2007: Modified by Michelle Martin <michelle@crytek.de>
//	August 2008: Adjusted for PS3-SPU execution by Christopher Bolte<chrisb@crytek.de>
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "CharacterInstance.h"
#include "Model.h"
#include "ModelSkeleton.h"
#include "CharacterManager.h"

#include "PoseModifier/PoseModifierHelper.h"

uint32 CSkeletonPose::SetCustomArmIK(const Vec3& localgoal,int32 idx0,int32 idx1,int32 idx2)
{
	return 1;
}



//--------------------------------------------------------------------------------
//----                  Two bone IK for human arms and legs                         ----
//--------------------------------------------------------------------------------
uint32 CSkeletonPose::SetHumanLimbIK(const Vec3& vWorldPos, const char* strLimb)
{
	Vec3 vLocalPos=m_pInstance->m_PhysEntityLocation.GetInverted()*vWorldPos;
	return SetHumanLimbIK_Local(vLocalPos,strLimb);
}

uint32 CSkeletonPose::SetHumanLimbIK_Local(const Vec3& vLocalPos, const char* strLimb)
{
	if (m_bFullSkeletonUpdate == 0)
		return 0;
	if (m_pSkeletonAnim->m_IsAnimPlaying == 0)
		return 0;
	
	
	uint64 nHandle = *(uint64*)strLimb;
	CModelSkeleton* __restrict pModelSkeleton 	= m_pModelSkeleton;	
	int32 idxDefinition=pModelSkeleton->GetLimbDefinitionIdx(nHandle);
	if (idxDefinition<0)
		return 0;   //we didn't find the limb definition and the IK-Solver for this handle 

	QuatT* const __restrict	pRelativePose = SPU_PTR_SELECT( &GetPoseDataWriteable()->m_jointsRelative[0], gSkeletonRelativePose );
	QuatT* const __restrict	pAbsolutePose	= SPU_PTR_SELECT( &GetPoseDataWriteable()->m_jointsAbsolute[0], gSkeletonAbsolutePose );
	IKLimbType& rIKLimbType	= pModelSkeleton->m_IKLimbTypes[idxDefinition];
	if (rIKLimbType.m_nSolver == *(uint32*)SPU_MAIN_PTR("2BIK"))
		PoseModifierHelper::IK_Solver2Bones(vLocalPos,rIKLimbType,pRelativePose,pAbsolutePose);
  if (rIKLimbType.m_nSolver == *(uint32*)SPU_MAIN_PTR("3BIK"))
    PoseModifierHelper::IK_Solver3Bones(vLocalPos,rIKLimbType,pRelativePose,pAbsolutePose);
	if (rIKLimbType.m_nSolver == *(uint32*)SPU_MAIN_PTR("CCDX"))
		PoseModifierHelper::IK_SolverCCD(vLocalPos,rIKLimbType,pRelativePose,pAbsolutePose);

	CModelJoint * __restrict	parrModelJoints 	= m_parrModelJoints;
	uint32 numChilds = rIKLimbType.m_arrLimbChildren.size();
	int16* pJointIdx = &rIKLimbType.m_arrLimbChildren[0];
	for (uint32 i=0; i<numChilds; i++)
	{
		int32 c=pJointIdx[i];
		int32 p=parrModelJoints[c].m_idxParent;
		pAbsolutePose[c]	= pAbsolutePose[p] * pRelativePose[c];
	}

	return 1;

};




//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void CSkeletonPose::MoveSkeletonVertical( f32 vertical )
{
	if ( (m_pSkeletonAnim->m_IsAnimPlaying & 1) && m_pSkeletonAnim->m_AnimationDrivenMotion && m_bFullSkeletonUpdate)
	{
		uint32 numJoints = GetPoseDataWriteable()->m_jointsAbsolute.size();
		for (uint32 i=1; i<numJoints; i++)
			GetPoseDataWriteable()->m_jointsAbsolute[i].t.z += vertical;
		if (numJoints>2)
			GetPoseDataWriteable()->m_jointsRelative[1] = GetPoseDataWriteable()->m_jointsAbsolute[0].GetInverted()*GetPoseDataWriteable()->m_jointsAbsolute[1];
	}

}



//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
uint32 CSkeletonPose::SetFootGroundAlignmentCCD( const char* strLEG, const Plane& GroundPlane)
{
	//	float fColor[4] = {0,1,0,1};
	IRenderer* pIRenderer = g_pISystem->GetIRenderer();

	uint64 nHandle = *(uint64*)strLEG;

	f32* pFootGroundAlign = &m_LFootGroundAlign;
	//	int32 ThighIdx	=	m_pModelSkeleton->m_IdxArray[eIM_LThighIdx];
	int32 CalfIdx		=	m_pModelSkeleton->m_IdxArray[eIM_LCalfIdx];	
	int32 FootIdx		=	m_pModelSkeleton->m_IdxArray[eIM_LFootIdx];
	int32 HeelIdx		=	m_pModelSkeleton->m_IdxArray[eIM_LHeelIdx];
	int32 Toe0Idx		=	m_pModelSkeleton->m_IdxArray[eIM_LToe0Idx];

	if (nHandle==*(uint64*)("RgtLeg01"))
	{
		pFootGroundAlign = &m_RFootGroundAlign;
		CalfIdx		=m_pModelSkeleton->m_IdxArray[eIM_RCalfIdx];
		FootIdx		=m_pModelSkeleton->m_IdxArray[eIM_RFootIdx];
		HeelIdx		=m_pModelSkeleton->m_IdxArray[eIM_RHeelIdx];
		Toe0Idx		=m_pModelSkeleton->m_IdxArray[eIM_RToe0Idx];
	}

	uint32 IkProcess=0;
	for (uint32 i=0; i<8; i++)
	{
		Quat rel_b0		= GetPoseDataWriteable()->m_jointsRelative[FootIdx].q;
		QuatT abs_b0  = GetPoseDataWriteable()->m_jointsAbsolute[FootIdx];
		QuatT abs_b1  = GetPoseDataWriteable()->m_jointsAbsolute[HeelIdx];
		QuatT abs_b2  = GetPoseDataWriteable()->m_jointsAbsolute[Toe0Idx];

		SetFootGroundAlignment( GroundPlane.n, CalfIdx,FootIdx,HeelIdx,Toe0Idx );

		Vec3	FootPos	=	GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].t;
		Vec3	HeelPos	=	GetPoseDataWriteable()->m_jointsAbsolute[HeelIdx].t;
		Vec3	Toe0Pos	=	GetPoseDataWriteable()->m_jointsAbsolute[Toe0Idx].t;

		GetPoseDataWriteable()->m_jointsRelative[FootIdx].q	=	rel_b0;
		GetPoseDataWriteable()->m_jointsAbsolute[FootIdx]		=	abs_b0;
		GetPoseDataWriteable()->m_jointsAbsolute[HeelIdx]		=	abs_b1;
		GetPoseDataWriteable()->m_jointsAbsolute[Toe0Idx]		=	abs_b2;

		//--------------------------------------------------------------------

		Ray ray_heel( HeelPos+Vec3(0.0f,0.0f, 0.6f), Vec3(0,0,-1) );
		//	pIRenderer->Draw_Lineseg(g_AnimatedCharacter*ray_heel.origin,RGBA8(0xff,0x1f,0x1f,0x00),g_AnimatedCharacter*ray_heel.origin+ray_heel.direction*10,RGBA8(0xff,0x1f,0x1f,0x00) );

		f32 heel_dist=0;
		Vec3 output_heel(0,0,0);
		bool _heel = Intersect::Ray_Plane( ray_heel, GroundPlane, output_heel );
		if (_heel)
		{
			heel_dist = output_heel.z-HeelPos.z;
			//pIRenderer->D8Print("Heel: %x   HeelIntersect: %f %f %f   distance_heel: %f", _heel, output_heel.x,output_heel.y,output_heel.z,heel_dist );
			if (heel_dist<0.001f)
				heel_dist=0.0f;
		}

		//----------------------------------------------------------------

		Ray ray_toe0( Toe0Pos+Vec3(0.0f,0.0f, 0.6f), Vec3(0,0,-1) );
		//pIRenderer->Draw_Lineseg(g_AnimatedCharacter*ray_toe0.origin,RGBA8(0xff,0x1f,0x1f,0x00),g_AnimatedCharacter*ray_toe0.origin+ray_toe0.direction*10.0f,RGBA8(0xff,0x1f,0x1f,0x00) );

		f32 toe0_dist=0;
		Vec3 output_toe0(0,0,0);
		bool _toe0 = Intersect::Ray_Plane( ray_toe0, GroundPlane, output_toe0 );
		if (_toe0)
		{
			toe0_dist = output_toe0.z-Toe0Pos.z;
			//pIRenderer->D8Print("Toe0: %x   Toe0Intersect: %f %f %f   distance_toe: %f", _toe0, output_toe0.x,output_toe0.y,output_toe0.z,toe0_dist );
			if (toe0_dist<0.001f)
				toe0_dist=0.0f;
		}

		//do the ik
		f32 distance = max(toe0_dist,heel_dist);
		if (distance==0)
			break;

		FootPos.z += distance; 
		if (nHandle==*(uint64*)("LftLeg01"))
			SetHumanLimbIK_Local(FootPos,"LftLeg01");
		if (nHandle==*(uint64*)("RgtLeg01"))
			SetHumanLimbIK_Local(FootPos,"RgtLeg01");

		IkProcess++;
	}



	//--------------------------------------------------------------

	//	pIRenderer->D8Print("  ");
	//	pIRenderer->D8Print("  ");
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"IkProcess: %d",IkProcess );	g_YLine+=16.0f;



	Quat fk_foot		= GetPoseDataWriteable()->m_jointsRelative[FootIdx].q;
	SetFootGroundAlignment( GroundPlane.n, CalfIdx,FootIdx,HeelIdx,Toe0Idx );

	if (IkProcess)
	{
		*pFootGroundAlign += 3.0f*fabsf(m_pInstance->m_fDeltaTime*m_pSkeletonAnim->m_arrLayerSpeedMultiplier[0]);
		//	*pFootGroundAlign += 3.0f*fabsf(0.014f);
	}
	else
	{
		Vec3	FootPos	=	GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].t;
		Vec3	HeelPos	=	GetPoseDataWriteable()->m_jointsAbsolute[HeelIdx].t;
		Vec3	Toe0Pos	=	GetPoseDataWriteable()->m_jointsAbsolute[Toe0Idx].t;

		Ray ray_heel( HeelPos+Vec3(0.0f,0.0f, 0.6f), Vec3(0,0,-1) );
		f32 heel_dist=0;
		Vec3 output_heel(0,0,0);
		bool _heel = Intersect::Ray_Plane( ray_heel, GroundPlane, output_heel );
		if (_heel)
		{
			heel_dist = output_heel.z-HeelPos.z;
			//		pIRenderer->D8Print("Heel: %x   HeelIntersect: %f %f %f   distance_heel: %f", _heel, output_heel.x,output_heel.y,output_heel.z,heel_dist );
			if (heel_dist<0.00f)
				heel_dist=0.0f;
		}

		//----------------------------------------------------------------

		Ray ray_toe0( Toe0Pos+Vec3(0.0f,0.0f, 0.6f), Vec3(0,0,-1) );
		f32 toe0_dist=0;
		Vec3 output_toe0(0,0,0);
		bool _toe0 = Intersect::Ray_Plane( ray_toe0, GroundPlane, output_toe0 );
		if (_toe0)
		{
			toe0_dist = output_toe0.z-Toe0Pos.z;
			//	pIRenderer->D8Print("Toe0: %x   Toe0Intersect: %f %f %f   distance_toe: %f", _toe0, output_toe0.x,output_toe0.y,output_toe0.z,toe0_dist );
			if (toe0_dist<0.00f)
				toe0_dist=0.0f;
		}

		//do just alignment
		f32 distance = max(toe0_dist,heel_dist);
		if (distance>0)
		{
			*pFootGroundAlign += 3.0f*fabsf(m_pInstance->m_fDeltaTime*m_pSkeletonAnim->m_arrLayerSpeedMultiplier[0]);
			//*pFootGroundAlign += 3.0f*fabsf(0.014f);
			SetFootGroundAlignment( GroundPlane.n, CalfIdx,FootIdx,HeelIdx,Toe0Idx );
		}
		else
		{
			*pFootGroundAlign -= 3.0f*fabsf(m_pInstance->m_fDeltaTime*m_pSkeletonAnim->m_arrLayerSpeedMultiplier[0]);
			//	*pFootGroundAlign -= 3.0f*fabsf(0.014f);
		}
	}

	if (*pFootGroundAlign>1.0f)
		*pFootGroundAlign=1.0f;
	if (*pFootGroundAlign<0.0f)
		*pFootGroundAlign=0.0f;

	f32 a=*pFootGroundAlign;
	f32 t		= -(2*a*a*a - 3*a*a);
	f32 t0	= 1.0f-t;
	f32 t1	= t;
	GetPoseDataWriteable()->m_jointsRelative[FootIdx].q	= fk_foot*t0 + GetPoseDataWriteable()->m_jointsRelative[FootIdx].q*t1;
	GetPoseDataWriteable()->m_jointsRelative[FootIdx].q.Normalize();

	int32 pb1=m_parrModelJoints[FootIdx].m_idxParent;
	int32 pb2=m_parrModelJoints[HeelIdx].m_idxParent;
	int32 pb3=m_parrModelJoints[Toe0Idx].m_idxParent;
	GetPoseDataWriteable()->m_jointsAbsolute[FootIdx]	  = GetPoseDataWriteable()->m_jointsAbsolute[pb1]   * GetPoseDataWriteable()->m_jointsRelative[FootIdx];
	GetPoseDataWriteable()->m_jointsAbsolute[HeelIdx]	  = GetPoseDataWriteable()->m_jointsAbsolute[pb2]   * GetPoseDataWriteable()->m_jointsRelative[HeelIdx];
	GetPoseDataWriteable()->m_jointsAbsolute[Toe0Idx]	  = GetPoseDataWriteable()->m_jointsAbsolute[pb3]   * GetPoseDataWriteable()->m_jointsRelative[Toe0Idx];


	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"m_LFootGroundAlign: %f   t:%f",m_LFootGroundAlign,t );	g_YLine+=16.0f;
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"m_RFootGroundAlign: %f   t:%f",m_RFootGroundAlign,t );	g_YLine+=16.0f;

	Vec3	FootPos	=	GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].t;
	Quat	FootRot	=	GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].q;
	Vec3	HeelPos	=	GetPoseDataWriteable()->m_jointsAbsolute[HeelIdx].t;
	Vec3	Toe0Pos	=	GetPoseDataWriteable()->m_jointsAbsolute[Toe0Idx].t;
	//pIRenderer->Draw_Sphere( Sphere(g_AnimatedCharacter*HeelPos,0.01f),RGBA8(0xff,0x1f,0x1f,0x00) );
	//pIRenderer->Draw_Sphere( Sphere(g_AnimatedCharacter*Toe0Pos,0.01f),RGBA8(0xff,0x1f,0x1f,0x00) );

	//pIRenderer->Draw_Lineseg(g_AnimatedCharacter*FootPos,RGBA8(0xff,0x1f,0x1f,0x00),g_AnimatedCharacter*FootPos+FootRot.GetColumn0()*0.1f,RGBA8(0xff,0x1f,0x1f,0x00) );
	//pIRenderer->Draw_Lineseg(g_AnimatedCharacter*FootPos,RGBA8(0x1f,0xff,0x1f,0x00),g_AnimatedCharacter*FootPos+FootRot.GetColumn1()*0.1f,RGBA8(0x1f,0xff,0x1f,0x00) );
	//pIRenderer->Draw_Lineseg(g_AnimatedCharacter*FootPos,RGBA8(0x1f,0x1f,0xff,0x00),g_AnimatedCharacter*FootPos+FootRot.GetColumn2()*0.1f,RGBA8(0x1f,0x1f,0xff,0x00) );

	return 1;
};


uint32 CSkeletonPose::SetFootGroundAlignment(const Vec3& normal2, int32 CalfIdx,int32 FootIdx,int32 HeelIdx,int32 Toe0Idx )
{
	Vec3 normal = Vec3(normal2.x,normal2.y,normal2.z).GetNormalized();
	assert((fabs_tpl(1-(normal|normal)))<0.001); //check if unit-vector

	/*	
	normal = (normal+normal2).GetNormalized();
	if (normal.z<0.707f)
	normal.z = 0.707f;
	normal.Normalize();
	*/

	int32 pb1=m_parrModelJoints[FootIdx].m_idxParent;
	int32 pb2=m_parrModelJoints[HeelIdx].m_idxParent;
	int32 pb3=m_parrModelJoints[Toe0Idx].m_idxParent;

	Vec3 AxisX = -GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].GetColumn0();

	GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].q = Quat::CreateRotationV0V1(AxisX,normal)*GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].q;
	GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].q.Normalize();
	GetPoseDataWriteable()->m_jointsRelative[FootIdx].q	= !GetPoseDataWriteable()->m_jointsAbsolute[pb1].q * GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].q;
	GetPoseDataWriteable()->m_jointsRelative[FootIdx].q.Normalize();

	Vec3 v0	=	(GetPoseDataWriteable()->m_jointsAbsolute[CalfIdx].t-GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].t).GetNormalized();
	Vec3 v1 =  GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].GetColumn1();
	f32 angle	= acos_tpl( v0|v1 ); 

	f32 up_limit=0.90f;
	f32 down_limit=1.70f;
	f32 rad=0;

	if (angle<up_limit)
		rad = angle-up_limit;
	if (angle>down_limit)
		rad = angle-down_limit;

	Vec3 AxisZ = GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].GetColumn2();
	Quat qrad=Quat::CreateRotationAA(rad,AxisZ);

	GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].q	= qrad*GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].q;
	GetPoseDataWriteable()->m_jointsRelative[FootIdx].q	=	!GetPoseDataWriteable()->m_jointsAbsolute[pb1].q * GetPoseDataWriteable()->m_jointsAbsolute[FootIdx].q;
	GetPoseDataWriteable()->m_jointsAbsolute[HeelIdx]	  = GetPoseDataWriteable()->m_jointsAbsolute[pb2]   * GetPoseDataWriteable()->m_jointsRelative[HeelIdx];
	GetPoseDataWriteable()->m_jointsAbsolute[Toe0Idx]	  = GetPoseDataWriteable()->m_jointsAbsolute[pb3]   * GetPoseDataWriteable()->m_jointsRelative[Toe0Idx];

	//	IRenderer* pIRenderer = g_pISystem->GetIRenderer();
	//	pIRenderer->D8Print("angle: %f   rad: %f",angle,rad );

	return 1;
};


void CSkeletonPose::EnableFootGroundAlignment(bool enable)
{
	m_enableFootGroundAlignment = enable;
}


