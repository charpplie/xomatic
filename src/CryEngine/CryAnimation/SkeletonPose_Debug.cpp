//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:SkeletonPose_Debug.cpp
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




//----------------------------------------------------------------------
//----        Draw Bounding Box                                     ----
//----------------------------------------------------------------------
void CSkeletonPose::DrawBBox( const Matrix34& rRenderMat34 )
{
	//check if this is a skin attachment! if yes, don't draw the box	
	g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
	CAttachment* pAttachment = m_pInstance->m_pSkinAttachment;
	if (pAttachment && (pAttachment->GetType()==CA_SKIN || pAttachment->GetType()==CA_PART)) 
		return;

	OBB obb=OBB::CreateOBBfromAABB( Matrix33(rRenderMat34), m_AABB );
	Vec3 wpos=rRenderMat34.GetTranslation();
	g_pAuxGeom->DrawOBB(obb,wpos,0,RGBA8(0xff,0x00,0x1f,0xff),eBBD_Extremes_Color_Encoded);
}




//----------------------------------------------------------------------
//----       draws the skeleton                                     ----
//----------------------------------------------------------------------
void CSkeletonPose::DrawSkeleton( const Matrix34& rRenderMat34, uint32 shift )
{
	//check if this is a skin attachment! if yes, don't draw the box	
	CAttachment* pAttachment = m_pInstance->m_pSkinAttachment;
	if (pAttachment && (pAttachment->GetType()==CA_SKIN || pAttachment->GetType()==CA_PART)) 
		return;

	g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags | e_DepthTestOff);

	static Ang3 ang_root(0,0,0); 
	ang_root += Ang3(0.01f,0.02f,0.03f);
	static Ang3 ang_rot(0,0,0); 
	ang_rot += Ang3(-0.01f,+0.02f,-0.03f);
	static Ang3 ang_pos(0,0,0); 
	ang_pos += Ang3(-0.03f,-0.02f,-0.01f);

	static AABB aabb_root	= AABB(Vec3(-0.019f,-0.019f,-0.019f),Vec3(+0.019f,+0.019f,+0.019f));
	Matrix33 m_root				=	Matrix33::CreateRotationXYZ(ang_root);
	OBB obb_root					=	OBB::CreateOBBfromAABB( m_root,aabb_root );

	static AABB aabb_rot	= AABB(Vec3(-0.011f,-0.011f,-0.011f),Vec3(+0.011f,+0.011f,+0.011f));
	Matrix33 m_rot				=	Matrix33::CreateRotationXYZ(ang_rot);
	OBB obb_rot						=	OBB::CreateOBBfromAABB( m_rot,aabb_rot );

	static AABB aabb_pos	= AABB(Vec3(-0.010f,-0.010f,-0.010f),Vec3(+0.010f,+0.010f,+0.010f));
	Matrix33 m_pos				=	Matrix33::CreateRotationXYZ(ang_pos);
	OBB obb_pos						=	OBB::CreateOBBfromAABB( m_pos,aabb_pos );

	QuatT WorldQuat		=	QuatT( rRenderMat34 );

	uint32 numJoints = GetPoseData().m_jointsAbsolute.size();
	for (uint32 i=0; i<numJoints; i++)
	{
		int32 p = m_parrModelJoints[i].m_idxParent;
		if (p>=0)
		{
			//this is a child
			QuatT WMatChild		=	WorldQuat * GetPoseData().m_jointsAbsolute[i];
			QuatT WMatParent	=	WorldQuat * GetPoseData().m_jointsAbsolute[p]; 
			g_pAuxGeom->DrawBone( WMatParent, WMatChild, RGBA8(0xff,0xff,0xff,0xc0) );

			const float dist = 0.03f;

			Vec3 xaxis = WMatChild.q * Vec3(dist,0,0);
			Vec3 yaxis = WMatChild.q * Vec3(0,dist,0);
			Vec3 zaxis = WMatChild.q * Vec3(0,0,dist);
			g_pAuxGeom->DrawLine(WMatChild.t,RGBA8(0xff>>shift,0x00>>shift,0x00>>shift,0xc0), WMatChild.t+xaxis, RGBA8(0xff>>shift,0x00>>shift,0x00>>shift,0xc0));
			g_pAuxGeom->DrawLine(WMatChild.t,RGBA8(0x00>>shift,0xff>>shift,0x00>>shift,0xc0), WMatChild.t+yaxis, RGBA8(0x00>>shift,0xff>>shift,0x00>>shift,0xc0));
			g_pAuxGeom->DrawLine(WMatChild.t,RGBA8(0x00>>shift,0x00>>shift,0xff>>shift,0xc0), WMatChild.t+zaxis, RGBA8(0x00>>shift,0x00>>shift,0xff>>shift,0xc0));


			if (GetPoseData().m_jointsStatus[i].o)
				g_pAuxGeom->DrawOBB(obb_rot,WMatChild.t,0,RGBA8(0xff,0x00,0x00,0xff),eBBD_Extremes_Color_Encoded);
			if (GetPoseData().m_jointsStatus[i].p)
				g_pAuxGeom->DrawOBB(obb_pos,WMatChild.t,0,RGBA8(0x00,0xff,0x00,0xff),eBBD_Extremes_Color_Encoded);

		} 
		else
		{
			//this is the root-bone (it can be a multiple-root system)
			QuatT WQuat		=	WorldQuat * GetPoseData().m_jointsAbsolute[i];
			g_pAuxGeom->DrawLine(WorldQuat.t,RGBA8(0xff>>shift,0x00>>shift,0x00>>shift,0xc0),  WQuat.t, RGBA8(0xff>>shift,0xff>>shift,0xff>>shift,0xc0));

			g_pAuxGeom->DrawOBB(obb_root,WQuat.t,1,RGBA8(0xff,0x00,0xff,0xff),eBBD_Extremes_Color_Encoded);

			//		float fColor[4] = {0,1,0,1};
			//		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"trans: %f %f %f",dd.t.x, dd.t.y, dd.t.z );	g_YLine+=16.0f;
			//		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"trans: %f %f %f",WQuat.t.x, WQuat.t.y, WQuat.t.z );	g_YLine+=16.0f;

		}
	}
}


//----------------------------------------------------------------------
//----       draws the default skeleton                                     ----
//----------------------------------------------------------------------
void CSkeletonPose::DrawDefaultSkeleton( const Matrix34& rRenderMat34 )
{

	static Ang3 ang_root(0,0,0); 
	ang_root += Ang3(0.01f,0.02f,0.03f);
	static Ang3 ang_rot(0,0,0); 
	ang_rot += Ang3(-0.01f,+0.02f,-0.03f);
	static Ang3 ang_pos(0,0,0); 
	ang_pos += Ang3(-0.03f,-0.02f,-0.01f);

	static AABB aabb_root	= AABB(Vec3(-0.019f,-0.019f,-0.019f),Vec3(+0.019f,+0.019f,+0.019f));
	Matrix33 m_root				=	Matrix33::CreateRotationXYZ(ang_root);
	OBB obb_root					=	OBB::CreateOBBfromAABB( m_root,aabb_root );

	static AABB aabb_rot	= AABB(Vec3(-0.011f,-0.011f,-0.011f),Vec3(+0.011f,+0.011f,+0.011f));
	Matrix33 m_rot				=	Matrix33::CreateRotationXYZ(ang_rot);
	OBB obb_rot						=	OBB::CreateOBBfromAABB( m_rot,aabb_rot );

	static AABB aabb_pos	= AABB(Vec3(-0.010f,-0.010f,-0.010f),Vec3(+0.010f,+0.010f,+0.010f));
	Matrix33 m_pos				=	Matrix33::CreateRotationXYZ(ang_pos);
	OBB obb_pos						=	OBB::CreateOBBfromAABB( m_pos,aabb_pos );

	QuatTS WorldQuat		=	QuatTS( rRenderMat34 );

	uint32 numJoints = GetPoseData().m_jointsAbsolute.size();
	for (uint32 i=0; i<numJoints; i++)
	{
		int32 p = m_parrModelJoints[i].m_idxParent;
		if (p>=0)
		{
			//this is a child
			QuatT WMatChild		=	QuatT(WorldQuat * m_pPoseDataDefault->m_jointsAbsolute[i]);
			QuatT WMatParent	=	QuatT(WorldQuat * m_pPoseDataDefault->m_jointsAbsolute[p]); 
			g_pAuxGeom->DrawBone( WMatParent, WMatChild, RGBA8(0xff,0xff,0xff,0xc0) );

			if (GetPoseData().m_jointsStatus[i].o)
				g_pAuxGeom->DrawOBB(obb_rot,WMatChild.t,0,RGBA8(0xff,0x00,0x00,0xff),eBBD_Extremes_Color_Encoded);
			if (GetPoseData().m_jointsStatus[i].p)
				g_pAuxGeom->DrawOBB(obb_pos,WMatChild.t,0,RGBA8(0x00,0xff,0x00,0xff),eBBD_Extremes_Color_Encoded);

		} 
		else
		{
			//this is the root-bone (it can be a multiple-root system)
			QuatTS WQuat		=	WorldQuat * m_pPoseDataDefault->m_jointsAbsolute[i];
			g_pAuxGeom->DrawLine(WorldQuat.t,RGBA8(0xff,0x00,0x00,0xc0),  WQuat.t, RGBA8(0xff,0xff,0xff,0xc0));

			g_pAuxGeom->DrawOBB(obb_root,WQuat.t,1,RGBA8(0xff,0x00,0xff,0xff),eBBD_Extremes_Color_Encoded);

		}
	}
}




//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
void CSkeletonPose::DrawThinSkeletons( const QuatT& m34, const std::vector< std::vector<DebugJoint> >& arrSkeletons, int32 nAnimID,int32 nGlobalAnimID )
{
	if (nGlobalAnimID<0)
		return;

	static Ang3 angle(0,0,0); 
	angle+=Ang3(0.01f,0.02f,0.03f);

	GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID];

	uint32 numSkeletons = arrSkeletons.size();
	for (uint32 s=0; s<numSkeletons; s++) 
		DrawThinSkeleton( m34, arrSkeletons[s], rGlobalAnimHeader.m_FootPlantBits[s], RGBA8(0xff,0xff,0xff,0xff) );
}



// draws the skeleton
void CSkeletonPose::DrawThinSkeleton( const QuatT& m34, const std::vector<DebugJoint>& arrSkeleton, uint8 FootPlant, ColorB col )
{
	static Ang3 angle(0,0,0); 
	angle+=Ang3(0.0001f,0.0002f,0.0003f);

	QuatT Pelvis	= arrSkeleton[0].m_AbsoluteQuat;
	QuatT WMat		= m34 * Pelvis;
	Vec3 WPos			=	WMat.t;


	Vec3 xaxis = WMat.GetColumn0()*0.15f;
	Vec3 yaxis = WMat.GetColumn1()*0.15f;
	Vec3 zaxis = WMat.GetColumn2()*0.15f;
	g_pAuxGeom->DrawLine(WPos, RGBA8(0xff,0x00,0x00,0), WPos+xaxis,RGBA8(0xff,0xff,0xff,0));
	g_pAuxGeom->DrawLine(WPos, RGBA8(0x00,0xff,0x00,0), WPos+yaxis,RGBA8(0xff,0xff,0xff,0));
	g_pAuxGeom->DrawLine(WPos, RGBA8(0x00,0x00,0xff,0), WPos+zaxis,RGBA8(0xff,0xff,0xff,0));



	{
		AABB aabb			= AABB(Vec3(-0.009f,-0.009f,-0.009f),Vec3(+0.009f,+0.009f,+0.009f));
		Matrix33 m33	=	Matrix33::CreateRotationXYZ(angle);
		OBB obb				=	OBB::CreateOBBfromAABB( m33,aabb );
		g_pAuxGeom->DrawOBB(obb,WPos,0,RGBA8(0xff,0x00,0x1f,0xff),eBBD_Extremes_Color_Encoded);
	}

	{
		AABB aabb			= AABB(Vec3(-0.002f,-0.002f,-0.002f),Vec3(+0.002f,+0.002f,+0.002f));
		Matrix33 m33	=	Matrix33::CreateRotationXYZ(angle);
		OBB obb				=	OBB::CreateOBBfromAABB( m33,aabb );

		int32 lHidx=m_pModelSkeleton->m_IdxArray[eIM_LHeelIdx];
		int32 rHidx=m_pModelSkeleton->m_IdxArray[eIM_RHeelIdx];
		if (lHidx>0 && rHidx>0)
		{
			if (FootPlant&LHEEL)
			{
				QuatT LWMat34	=	m34 * arrSkeleton[lHidx].m_AbsoluteQuat;
				g_pAuxGeom->DrawOBB(obb,LWMat34.t,0,RGBA8(0x00,0x00,0xff,0xff),eBBD_Extremes_Color_Encoded);
			}
			if (FootPlant&RHEEL)
			{
				QuatT RWMat34	=	m34 * arrSkeleton[rHidx].m_AbsoluteQuat;
				g_pAuxGeom->DrawOBB(obb,RWMat34.t,0,RGBA8(0x00,0x00,0xff,0xff),eBBD_Extremes_Color_Encoded);
			}
		}

		uint32 lTidx=m_pModelSkeleton->m_IdxArray[eIM_LToe0Idx];
		uint32 rTidx=m_pModelSkeleton->m_IdxArray[eIM_RToe0Idx];
		if (lTidx!=0xffff && rTidx!=0xffff)
		{
			if (FootPlant&LTOE0)
			{
				QuatT LWMat34	=	m34 * arrSkeleton[lTidx].m_AbsoluteQuat;
				g_pAuxGeom->DrawOBB(obb,LWMat34.t,0,RGBA8(0xff,0x00,0x00,0xff),eBBD_Extremes_Color_Encoded);
			}
			if (FootPlant&RTOE0)
			{
				QuatT RWMat34	=	m34 * arrSkeleton[rTidx].m_AbsoluteQuat;
				g_pAuxGeom->DrawOBB(obb,RWMat34.t,0,RGBA8(0xff,0x00,0x00,0xff),eBBD_Extremes_Color_Encoded);
			}
		}

	}


	uint32 numJoints = arrSkeleton.size();
	for (uint32 i=0; i<numJoints; i++)
	{

		int16 idxParent = arrSkeleton[i].m_idxParent;
		if (idxParent != -1)
		{

			QuatT WMatChild		=	m34 * arrSkeleton[i].m_AbsoluteQuat;
			QuatT WMatParent	=	m34 * arrSkeleton[idxParent].m_AbsoluteQuat;
			Vec3 JChild	  =	WMatChild.t;
			Vec3 JParent	=	WMatParent.t;
			f32 distance	= (JChild-JParent).GetLength();
			g_pAuxGeom->DrawLine( JParent,col, JChild,RGBA8(0x1f,0x1f,0x1f,0xff) );

		} 

	}
}




//////////////////////////////////////////////////////////////////////////
// calculates the bounding box for this instance using the moving skeleton
//////////////////////////////////////////////////////////////////////////
f32 CSkeletonPose::SecurityCheck()
{
	f32 fRadius = 0.0f;
	uint32 numJoints = GetPoseData().m_jointsAbsolute.size();
	for (uint32 i=0; i<numJoints; i++)
	{
		f32 t=GetPoseData().m_jointsAbsolute[i].t.GetLength();
		if (fRadius<t)
			fRadius=t;
	}
	return fRadius; 
}

//////////////////////////////////////////////////////////////////////////
// calculates the bounding box for this instance using the moving skeleton
//////////////////////////////////////////////////////////////////////////
uint32 CSkeletonPose::IsSkeletonValid()
{
	uint32 numJoints = GetPoseData().m_jointsAbsolute.size();
	for (uint32 i=0; i<numJoints; i++)
	{
		uint32 valid = GetPoseData().m_jointsAbsolute[i].t.IsValid();
		if (valid==0)
			return 0;
		if ( fabsf(GetPoseData().m_jointsAbsolute[i].t.x)>20000.0f )
			return 0;
		if ( fabsf(GetPoseData().m_jointsAbsolute[i].t.y)>20000.0f )
			return 0;
		if ( fabsf(GetPoseData().m_jointsAbsolute[i].t.z)>20000.0f )
			return 0;
	}
	return 1;
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
size_t CSkeletonPose::SizeOfThis (ICrySizer * pSizer)
{
	size_t TotalSize = 0;

	TotalSize += GetPoseData().m_jointsStatus.get_alloc_size();
	TotalSize += GetPoseData().m_jointsRelative.get_alloc_size();
	TotalSize += GetPoseData().m_jointsAbsolute.get_alloc_size();
	TotalSize += m_FaceAnimPosSmooth.get_alloc_size(); 
	TotalSize += m_FaceAnimPosSmoothRate.get_alloc_size(); 
	//TotalSize += m_InitialRelativePose.get_alloc_size(); 
	TotalSize += m_arrCGAJoints.get_alloc_size();
	TotalSize += m_arrPhysicsJoints.get_alloc_size();

	TotalSize += m_arrPostProcess.get_alloc_size();
	TotalSize += m_arrCCDChain.get_alloc_size();
	TotalSize += m_poseModifiers.get_alloc_size();

	if (m_ppBonePhysics)
		TotalSize += GetPoseData().m_jointsAbsolute.capacity() * sizeof(IPhysicalEntity*);

	return TotalSize;
}

void CSkeletonPose::GetMemoryUsage(ICrySizer * pSizer) const
{
	pSizer->AddObject( m_pPoseData );
	//pSizer->AddObject( m_pCharPhysics );
	pSizer->AddObject( m_LookAt );
	pSizer->AddObject( m_PoseBlenderAim );
	for( int i = 0 ; i < 12 ; ++i )
		pSizer->AddObject(m_auxPhys[i]);
	pSizer->AddObject( m_FaceAnimPosSmooth );
	pSizer->AddObject( m_FaceAnimPosSmoothRate );
	pSizer->AddObject( m_arrCGAJoints );
	pSizer->AddObject( m_arrPhysicsJoints );
	pSizer->AddObject( m_arrPostProcess );
	pSizer->AddObject( m_arrCCDChain );
	pSizer->AddObject( m_poseModifiers );	
}
