#include "stdafx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "../CharacterInstance.h"
#include "../Model.h"
#include "../ModelSkeleton.h"
#include "../CharacterManager.h"

#include "PoseModifierHelper.h"
#include "PoseBlenderAim.h"



//

//--------------------------------------------------------------------------------------
//--------                enables/disables aim IK and sets aim target			--------
//--------------------------------------------------------------------------------------
void CSkeletonPose::SetAimIK(bool useAimIK, const Vec3& aimIKTarget, uint32 nArms, float blendTime) 
{
	m_AimIK().m_Set_UseAimIK = useAimIK;
	m_AimIK().m_Set_nArms = nArms;

	if (blendTime != -1.0f)
	{
		m_AimIK().m_Set_AimIKBlend = blendTime;
	}

	if (useAimIK && !aimIKTarget.IsValid())
	{
		g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pInstance->GetFilePath(), 
			"CryAnimation: aimIKTarget Invalid (%f %f %f)", aimIKTarget.x, aimIKTarget.y, aimIKTarget.z);
#ifdef _DEBUG
		g_pISystem->debug_LogCallStack();
#endif
	}
	else
	{
		m_AimIK().m_Set_AimIKTarget = aimIKTarget;
	}
}

void CSkeletonPose::SetAimIKLayer(uint32 nLayer) 
{ 
	m_AimIK().m_Set_nLayer = nLayer;
	if (m_AimIK().m_Set_nLayer< 1)
		m_AimIK().m_Set_nLayer = 1;
};

void CSkeletonPose::SetPoseBlenderAimState(bool bEnable)
{
	m_AimIK().m_Set_UseAimIK = bEnable;
}

void CSkeletonPose::SetPoseBlenderAimLayer(uint32 layer)
{
	m_AimIK().m_Set_nLayer = layer;
	if (m_AimIK().m_Set_nLayer < 1)
		m_AimIK().m_Set_nLayer = 1;
}







void CPoseBlenderAim::UpdateParameters(const SAnimationPoseModiferParams& params)
{
	CCharInstance* m_pInstance				= PoseModifierHelper::GetCharInstance(params);
	CSkeletonPose* m_pSkeletonPose		= PoseModifierHelper::GetSkeletonPose(params);
	CModelSkeleton* m_pModelSkeleton	= PoseModifierHelper::GetModelSkeleton(params);

	QuatT* m_RelativePose = params.pPoseRelative;
	QuatT* m_AbsolutePose = params.pPoseAbsolute;

	CModelJoint* m_parrModelJoints = m_pSkeletonPose->m_parrModelJoints;

	const QuatTS& rAnimLocationNext = params.locationNextAnimation;

	//

	int32 nWeaponBone = m_pModelSkeleton->m_IdxArray[eIM_RWeaponBoneIdx];
	if (nWeaponBone>0)
	{
		Vec3 vAimIKTarget = m_Set_AimIKTarget;

		int32 neckidx	=	m_pModelSkeleton->m_IdxArray[eIM_NeckIdx];
		if (neckidx<0)
			neckidx=0; //we can take the root instead

		Vec3 NeckBoneWorld	= rAnimLocationNext*m_AbsolutePose[neckidx].t;
		Vec3 vDistance=vAimIKTarget-NeckBoneWorld;
		const f32 fMinDist=2.5f; 
		if (vDistance.GetLength()<fMinDist)
			vAimIKTarget = NeckBoneWorld+vDistance.GetNormalizedSafe(Vec3(0,1,0))*fMinDist; 

		SmoothCD(m_AimIKTargetSmooth, m_AimIKTargetRate, m_pInstance->m_fOriginalDeltaTime, vAimIKTarget, m_Set_AimIKTargetSmoothTime);

#if !defined(__SPU__)
		if (m_Set_UseAimIK)
		{
			if( Console::GetInst().ca_DrawAimPoses)
			{
				g_pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
				static Ang3 angle1(0,0,0); 
				angle1 += Ang3(0.01f,0.02f,0.08f);
				AABB aabb = AABB(Vec3(-0.09f,-0.09f,-0.09f),Vec3(+0.09f,+0.09f,+0.09f));
				OBB obb1=OBB::CreateOBBfromAABB( Matrix33::CreateRotationXYZ(angle1),aabb );
				g_pAuxGeom->DrawOBB(obb1, m_AimIKTargetSmooth,1,RGBA8(0x00,0x00,0xff,0xff), eBBD_Extremes_Color_Encoded);
			}
		}
#endif


		f32 fFrameTime = m_pInstance->m_fOriginalDeltaTime;
		if (fFrameTime<0.0f) fFrameTime=0.0f;
		if (m_AimIKFadeout || m_Set_UseAimIK==0) 
			m_Set_AimIKBlend-=m_Set_fAimIKFadeOutTime*fFrameTime;
		else
			m_Set_AimIKBlend+=m_Set_fAimIKFadeInTime*fFrameTime;

		Limit(m_Set_AimIKBlend, 0.0f, 1.0f);
	}
}



namespace 
{
	struct Tri
	{
		uint16 x, y, z;
		ColorB rgb;

		ILINE Tri(uint16 vx, uint16 vy, uint16 vz, ColorB c) { x=vx; y=vy; z=vz; rgb=c; };
	};
}

struct SAimDirection 
{
	Vec3 start;
	Vec3 end;
	bool evaluated;

	SAimDirection() : evaluated(false) { }
	SAimDirection(const Vec3& s, const Vec3& e, bool eval) : start(s), end(e), evaluated(eval) { }
};






#define MIDDLE (24)
Tri g_AimTriangles[] = 
{
	//first ring
	Tri( 16, 23, 24,RGBA8(0x00,0xff,0x00,0xff)),
	Tri( 16, 24, 17,RGBA8(0x00,0xff,0x00,0xff)),
	Tri( 24, 18, 17,RGBA8(0x00,0xff,0x00,0xff)),
	Tri( 24, 25, 18,RGBA8(0x00,0xff,0x00,0xff)),

	Tri( 24, 23, 30,RGBA8(0x00,0xff,0x00,0xff)),
	Tri( 24, 30, 31,RGBA8(0x00,0xff,0x00,0xff)),
	Tri( 24, 31, 32,RGBA8(0x00,0xff,0x00,0xff)),
	Tri( 24, 32, 25,RGBA8(0x00,0xff,0x00,0xff)),

	//second ring
	Tri( 8, 15, 16,RGBA8(0xff,0x00,0x00,0xff)),
	Tri( 8, 16,  9,RGBA8(0xff,0x00,0x00,0xff)),
	Tri( 9, 16, 17,RGBA8(0xff,0x00,0x00,0xff)),
	Tri( 9, 17, 10,RGBA8(0xff,0x00,0x00,0xff)),

	Tri(10, 17, 11,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(17, 18, 11,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(11, 18, 12,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(18, 19, 12,RGBA8(0xff,0x00,0x00,0xff)),

	Tri(18, 25, 19,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(25, 26, 19,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(25, 32, 33,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(25, 33, 26,RGBA8(0xff,0x00,0x00,0xff)),

	Tri(32, 39, 40,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(32, 40, 33,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(31, 38, 39,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(31, 39, 32,RGBA8(0xff,0x00,0x00,0xff)),

	Tri(30, 37, 31,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(37, 38, 31,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(29, 36, 30,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(36, 37, 30,RGBA8(0xff,0x00,0x00,0xff)),

	Tri(22, 29, 23,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(23, 29, 30,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(15, 22, 23,RGBA8(0xff,0x00,0x00,0xff)),
	Tri(15, 23, 16,RGBA8(0xff,0x00,0x00,0xff)),


	//extrapolation ring
	Tri( 0,  7,  8,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri( 0,  8,  1,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri( 1,  8,  9,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri( 1,  9,  2,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri( 2,  9, 10,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri( 2, 10,  3,RGBA8(0xff,0x0f,0xff,0xff)),

	Tri( 4,  3, 10,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri( 4, 10, 11,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri( 5,  4, 11,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri( 5, 11, 12,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri( 6,  5, 12,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri( 6, 12, 13,RGBA8(0xff,0x0f,0xff,0xff)),



	Tri(13, 12, 19,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(13, 19, 20,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(19, 26, 20,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(20, 26, 27,RGBA8(0xff,0x0f,0xff,0xff)),

	Tri(26, 33, 34,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(26, 34, 27,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(33, 40, 41,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(33, 41, 34,RGBA8(0xff,0x0f,0xff,0xff)),

	Tri(40, 47, 48,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(40, 48, 41,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(39, 46, 47,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(39, 47, 40,RGBA8(0xff,0x0f,0xff,0xff)),

	Tri(38, 45, 46,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(38, 46, 39,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(38, 44, 45,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(38, 37, 44,RGBA8(0xff,0x0f,0xff,0xff)),

	Tri(37, 43, 44,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(37, 36, 43,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(36, 42, 43,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(36, 35, 42,RGBA8(0xff,0x0f,0xff,0xff)),

	Tri(29, 35, 36,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(29, 28, 35,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(22, 28, 29,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(22, 21, 28,RGBA8(0xff,0x0f,0xff,0xff)),

	Tri(14, 21, 22,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri(14, 22, 15,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri( 7, 14, 15,RGBA8(0xff,0x0f,0xff,0xff)),
	Tri( 7, 15,  8,RGBA8(0xff,0x0f,0xff,0xff))
};


Tri g_AimTriangles4[] = 
{
	Tri(  8, 22, 24,RGBA8(0x00,0xff,0x00,0xff)),
	Tri(  8, 24, 10,RGBA8(0x00,0xff,0x00,0xff)),
	Tri( 10, 24, 12,RGBA8(0x00,0xff,0x00,0xff)),
	Tri( 24, 26, 12,RGBA8(0x00,0xff,0x00,0xff)),

	Tri( 22, 36, 24,RGBA8(0x00,0xff,0x00,0xff)),
	Tri( 36, 38, 24,RGBA8(0x00,0xff,0x00,0xff)),
	Tri( 24, 38, 40,RGBA8(0x00,0xff,0x00,0xff)),
	Tri( 24, 40, 26,RGBA8(0x00,0xff,0x00,0xff))
};


/*
Tri g_AimTriangles2[] = 
{
	Tri(  0,  1, 4,RGBA8(0x00,0xff,0x00,0xff)),
	Tri(  0,  4, 3,RGBA8(0x00,0xff,0x00,0xff)),
	Tri(  3,  4, 6,RGBA8(0x00,0xff,0x00,0xff)),
	Tri(  4,  7, 6,RGBA8(0x00,0xff,0x00,0xff)),

	Tri(  4,  8, 7,RGBA8(0x00,0xff,0x00,0xff)),
	Tri(  4,  5, 8,RGBA8(0x00,0xff,0x00,0xff)),
	Tri(  4,  2, 5,RGBA8(0x00,0xff,0x00,0xff)),
	Tri(  4,  1, 2,RGBA8(0x00,0xff,0x00,0xff))
};
*/

//#define XENON_DEBUG

#ifdef XENON_DEBUG
#include "xtl.h"
#include "tracerecording.h"
#pragma comment( lib, "tracerecording.lib" )
#pragma comment( lib, "xbdm.lib" )
#endif


//

CRYREGISTER_CLASS(CPoseBlenderAim)

CPoseBlenderAim::CPoseBlenderAim()
{
}

CPoseBlenderAim::~CPoseBlenderAim()
{
}






// util class to provide each SPU with its own temp memory for AimIKPoses
struct AimPoseContainer
{
	//TODO: get rid of this completly
	AimPoseContainer()
	{
		for( uint32 j = 0 ; j < NUM_BUCKETS ; ++j  )
		{
			for( uint32 i = 0 ; i < AIM_POSES ; ++i )
			{
				arrAimIKPosesCont[j][i].m_arrRotation.resize(64);
				arrAimIKPosesCont[j][i].m_arrPosition.resize(64);
			}
		}
	}

	AimIKPose* GetAimPoseContainer() 
	{ 
#if !defined(__SPU__)
		return arrAimIKPosesCont[0];
#else			
		uint32 index = __spu_get_current_id() - 1;
		return arrAimIKPosesCont[index]; 
#endif			
	}


#if !defined(PS3)		
	static const uint32 NUM_BUCKETS = 1;
#else		
	static const uint32 NUM_BUCKETS = 4;
#endif

	AimIKPose arrAimIKPosesCont[NUM_BUCKETS][AIM_POSES];

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		SIZER_COMPONENT_NAME(pSizer, "AimPoseTempData");
		for( uint32 j = 0 ; j < NUM_BUCKETS ; ++j  )
		{
			for( uint32 i = 0 ; i < AIM_POSES ; ++i )
			{
				pSizer->AddObject(arrAimIKPosesCont[j][i].m_arrRotation);
				pSizer->AddObject(arrAimIKPosesCont[j][i].m_arrPosition);
			}
		}
	}
} gAnimPoseContainer;

bool CPoseBlenderAim::ExecutePreProcess()
{
	m_UseAimIK = m_Set_UseAimIK;
	m_nArms = m_Set_nArms;
	m_nLayer = m_Set_nLayer;
	m_AimIKTarget = m_Set_AimIKTarget;
	m_AimIKTargetSmoothTime = m_Set_AimIKTargetSmoothTime;
	m_AimIKBlend = m_Set_AimIKBlend;
	m_AimIKFadeoutFlag = m_Set_AimIKFadeoutFlag;
	m_fAimIKFadeInTime = m_Set_fAimIKFadeInTime;
	m_fAimIKFadeOutTime = m_Set_fAimIKFadeOutTime;
	m_PolarCoordinates = m_Set_PolarCoordinates;

	return true;
}

SPU_INDIRECT(CommandBufferExecute(ML))
bool CPoseBlenderAim::Execute(const SAnimationPoseModiferParams& params) 
{

#if (NEWAIMSYSTEM)
	// for SPU copy object to stack to improve execution time
	SpuStackValue<CPoseBlenderAim,true,true> stackThis(*this);
	stackThis->ExecuteAimIK_New(params);
	return 0;
#endif

	CCharInstance* m_pInstance = PoseModifierHelper::GetCharInstance(params);
	CSkeletonPose* m_pSkeletonPose = PoseModifierHelper::GetSkeletonPose(params);
	CSkeletonAnim* m_pSkeletonAnim = PoseModifierHelper::GetSkeletonAnim(params);
	CModelSkeleton* m_pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);

	QuatT* m_RelativePose = SPU_LOCAL_PTR( params.pPoseRelative );
	QuatT* m_AbsolutePose = SPU_LOCAL_PTR( params.pPoseAbsolute );

	CModelJoint* m_parrModelJoints = m_pSkeletonPose->m_parrModelJoints;
	const QuatTS& rAnimLocationNext = SPU_LOCAL_REF(params.locationNextAnimation);

	m_AbsolutePose[0]		= m_RelativePose[0];
	for (uint32 i=1; i<params.jointCount; i++)
		m_AbsolutePose[i]	= m_AbsolutePose[m_pModelSkeleton->GetJointParentIDByID(i)] * m_RelativePose[i];

#ifdef XENON_DEBUG
	if (Console::GetInst().ca_UseAimIK == 2) 
	{
		XTraceStartRecording( "e:\\AimIK.pix2" );
		Console::GetInst().ca_UseAimIK = 0;
	}
#endif

	m_AimIKFadeout = 0; // by default we are in fadeout mode
	m_AimIKInfluence = 0.0f; // [artemk]: does this initialized in the right place ???

	if (Console::GetInst().ca_UseAimIK == 0)
		return false;

	uint32 numRot = m_pModelSkeleton->m_AimIK_Rot.size();
	if (numRot==0)
	{
#if !defined(__SPU__)		
		const char* pModelName = m_pInstance->m_pModel->GetFilePath();
		g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pInstance->GetFilePath(), "CryAnimation: No AimIK-Setup for model: %s",pModelName );
#endif		
		return false;
	}
	int16 const * __restrict pIdxArray = m_pModelSkeleton->m_IdxArray;
	int32 nWeaponBone = pIdxArray[eIM_RWeaponBoneIdx];
	if (nWeaponBone<0)
	{
#if !defined(__SPU__)				
		const char* pModelName = m_pInstance->m_pModel->GetFilePath();
		g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pInstance->GetFilePath(), "CryAnimation: No weapon-bone specified for model: %s",pModelName );
#endif		
		return false;
	}

	if (m_pSkeletonPose->m_timeStandingUp >= 0)
		return false;
	if (m_numActiveAimPoses==0)
		return false; // no animation in base-layer, no aim-pose


//-----------------------------------------------------------------------------------------------


	Vec3 vBodyDirectionWorld	= rAnimLocationNext.q.GetColumn1();  //this is the VIEW direction 
	Vec3 AimIKTargetProj			= m_AimIKTargetSmooth;
	Vec3 vAimDirectionWorld		= (AimIKTargetProj-rAnimLocationNext.t).GetNormalizedSafe( Vec3(0,1,0) );


	f32 cosine2=vAimDirectionWorld|vBodyDirectionWorld;
	if (cosine2<0)
	{
		Vec3 clamped = vAimDirectionWorld.Cross(Vec3(0,0,1)).Cross(vAimDirectionWorld).Cross(vBodyDirectionWorld).GetNormalizedSafe(vBodyDirectionWorld);
		if (vAimDirectionWorld.Dot(clamped) < 0)
			vAimDirectionWorld = -clamped;
		else
			vAimDirectionWorld = clamped;
		AimIKTargetProj =  rAnimLocationNext.t + 8.0f*vAimDirectionWorld;
	}
	vAimDirectionWorld		= (AimIKTargetProj-rAnimLocationNext.t).GetNormalizedSafe(Vec3(0,1,0));
	Vec3 localAimTarget	= AimIKTargetProj-rAnimLocationNext.t;
	m_AimDirection					= vAimDirectionWorld*rAnimLocationNext.q;



	QuatT* 	const	__restrict	pSkeletonRelativePose	= &m_RelativePose[0];
	QuatT* 	const	__restrict	pSkeletonAbsolutPose	= &m_AbsolutePose[0];

	//some rules to disable aim-ik
	//RULE#1: target is too close
	int32 nWeaponIdx2 = pIdxArray[eIM_RWeaponBoneIdx];
	Vec3 nWeaponBoneWorld2	= rAnimLocationNext*pSkeletonAbsolutPose[nWeaponIdx2].t;
	if ((m_AimIKTarget - nWeaponBoneWorld2).GetLength() < 0.5f)
		m_AimIKFadeout=1;

	//RULE#2: he is trying to aim sideways or behind him
	f32 cosine=vAimDirectionWorld|vBodyDirectionWorld;
	if (cosine<0.3f && (Console::GetInst().ca_AimIKFadeout & m_AimIKFadeoutFlag))
		m_AimIKFadeout=1;








	//---------------------------------------------------------------------------
	//----           make sure every base-animation has a valid aim-pose     ----
	//---------------------------------------------------------------------------
	uint32 numActiveAnims = m_numActiveAimPoses;
	for (uint32 c=0; c<numActiveAnims; c++)
	{
		for (uint32 i=1; i<numActiveAnims; i++)
		{
			if (m_AimInfo[i-1].m_nGlobalAimID0<0 && m_AimInfo[i].m_nGlobalAimID0>=0)
			{ 
				m_AimInfo[i-1].m_nGlobalAimID0=m_AimInfo[i].m_nGlobalAimID0;
				m_AimInfo[i-1].m_numAimPoses=1;
			}
		}
	}

	if (m_AimInfo[numActiveAnims-1].m_nGlobalAimID0<0)
		m_AimIKFadeout = 1;  //last anim has no aim-pose -> start fading out

	for (uint32 c=0; c<numActiveAnims; c++)
	{
		for (uint32 i=0; i<(numActiveAnims-1); i++)
		{
			if (m_AimInfo[i+1].m_nGlobalAimID0<0 && m_AimInfo[i].m_nGlobalAimID0>=0)
			{ 
				m_AimInfo[i+1].m_nGlobalAimID0=m_AimInfo[i].m_nGlobalAimID0;
				m_AimInfo[i+1].m_numAimPoses=1;
			}
		}
	}

	//do we have a single valid aim-pose
	if (m_AimInfo[0].m_nGlobalAimID0<0)
	{
		if (m_nLastValidAimPose<0)
			return false;
		for (uint32 i=0; i<numActiveAnims; i++)
		{
			m_AimInfo[i].m_nGlobalAimID0 =m_nLastValidAimPose;
			m_AimInfo[i].m_numAimPoses=1;
		}
	}
	m_nLastValidAimPose=m_AimInfo[numActiveAnims-1].m_nGlobalAimID0;
	if (m_AimInfo[0].m_nGlobalAimID0<0) 
		return false; //no aimpose at all

	if (m_AimIKBlend<0.001f)
		return false;

//	float fColDebug[4] = {1,1,0,1};
//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColDebug, false,"m_AimIKBlend: %f",m_AimIKBlend );	
//	g_YLine+=16.0f;

	//------------------------------------------------------------------------------------
	//---------         here we know that we can actually use AimIK          -------------
	//------------------------------------------------------------------------------------
	uint32 numJoints = params.jointCount;
	AimIKPose* arrAimIKPoses = gAnimPoseContainer.GetAimPoseContainer();
	BuildLocalAimPoseArray(params, arrAimIKPoses);

	f32 fIKBlend = clamp(m_AimIKInfluence,0.0f,1.0f);
	fIKBlend -= 0.5f;
	fIKBlend = 0.5f+fIKBlend/(0.5f+2.0f*fIKBlend*fIKBlend);

#if !defined(__SPU__)
	if( Console::GetInst().ca_DrawAimPoses==1)
	{
		IPol _ipol[AimTRIANGLES];
		SAimDirection ls[AIM_POSES]; //return values

		QuatT* _pRelativeQuatIK = (QuatT*)alloca( numJoints*sizeof(QuatT) );
		QuatT* _pAbsoluteQuatIK = (QuatT*)alloca( numJoints*sizeof(QuatT) );
		for (uint32 i=0; i<numJoints; i++)
		{
			_pRelativeQuatIK[i] = pSkeletonRelativePose[i];
			_pAbsoluteQuatIK[i] = pSkeletonAbsolutPose[i];
		}

		IPol _ipol2[AimTRIANGLES];
		SAimDirection ls2[AIM_POSES];
		for (uint32 c=0; c<AimTRIANGLES; c++)
		{
			uint32 k0	=g_AimTriangles[c].x;
			uint32 k1	=g_AimTriangles[c].y;
			uint32 k2	=g_AimTriangles[c].z;
			ColorB rgb=g_AimTriangles[c].rgb;

			if (ls[k0].evaluated==0) ls[k0]=EvaluatePose(params, k0, &arrAimIKPoses[0]);
			if (ls[k1].evaluated==0) ls[k1]=EvaluatePose(params, k1, &arrAimIKPoses[0]);
			if (ls[k2].evaluated==0) ls[k2]=EvaluatePose(params, k2, &arrAimIKPoses[0]);

			IPol ipol;
			ipol.k0=k0;		ipol.k1=k1;		ipol.k2=k2;
			Vec3 s0=ls[k0].start;	Vec3 s1=ls[k1].start;	Vec3 s2=ls[k2].start;
			Vec3 t0=ls[k0].end;		Vec3 t1=ls[k1].end;		Vec3 t2=ls[k2].end;

			Vec3 WPos = rAnimLocationNext.t;
			ColorB tricol = RGBA8(uint8(rgb.r*fIKBlend),uint8(rgb.g*fIKBlend),uint8(rgb.b*fIKBlend),0xff);
			g_pAuxGeom->DrawLine(WPos+s0,tricol, WPos+s1,tricol );
			g_pAuxGeom->DrawLine(WPos+s1,tricol, WPos+s2,tricol );
			g_pAuxGeom->DrawLine(WPos+s2,tricol, WPos+s0,tricol );
			g_pAuxGeom->DrawLine(WPos+t0,tricol, WPos+t1,tricol );
			g_pAuxGeom->DrawLine(WPos+t1,tricol, WPos+t2,tricol );
			g_pAuxGeom->DrawLine(WPos+t2,tricol, WPos+t0,tricol );

			g_pAuxGeom->DrawLine(	WPos+s0,RGBA8(0x7f,0x7f,0x7f,0xff), WPos+t0,RGBA8(0xff,0xff,0xff,0xff) );
			g_pAuxGeom->DrawLine(	WPos+s1,RGBA8(0x7f,0x7f,0x7f,0xff), WPos+t1,RGBA8(0xff,0xff,0xff,0xff) );
			g_pAuxGeom->DrawLine(	WPos+s2,RGBA8(0x7f,0x7f,0x7f,0xff), WPos+t2,RGBA8(0xff,0xff,0xff,0xff) );
		}
		for (uint32 i=0; i<numJoints; i++)
		{
			pSkeletonRelativePose[i]	=	_pRelativeQuatIK[i];
			pSkeletonAbsolutPose[i]	=	_pAbsoluteQuatIK[i];
		}
	}
#endif

	//--------------------------------------------------------------------------------------------------------------

	int32 c=0;
	uint32 InsideTriangle=0;
	IPol _ipol[AimTRIANGLES];
	SAimDirection ls[AIM_POSES]; //return values
	for ( ; c<AimTRIANGLES; c++)
	{
		uint32 i0=g_AimTriangles[c].x;
		uint32 i1=g_AimTriangles[c].y;
		uint32 i2=g_AimTriangles[c].z;
		ColorB rgb=g_AimTriangles[c].rgb;
		_ipol[c]=CheckTriangles(params, i0, i1, i2, rgb, ls, localAimTarget, &arrAimIKPoses[0], fIKBlend);
		if ( _ipol[c].inside )	
		{
			InsideTriangle++;
			break;
		}; 
	}

	f32 d = FLT_MAX;
	if (InsideTriangle==0)
	{
		c=-1;
		for (uint32 i=0; i<AimTRIANGLES; i++)
		{
			if (d>_ipol[i].distance) 
			{ 
				d=_ipol[i].distance;	c=i; 
			}
		}
	}
	//-------------------------------------------------------------

	IPol ipol = _ipol[0];
	if (c<0)
		m_AimIKBlend=0;
	else
		ipol=_ipol[c];

	f32 sum = fabsf(ipol.iepv.x)+fabsf(ipol.iepv.y)+fabsf(ipol.iepv.z);
	if (sum==0)
		return false;
	assert(sum);

#if !defined(__SPU__)
	if( Console::GetInst().ca_DrawAimPoses)
	{
		float fColDebug[4] = {1,1,0,1};
		Vec3 WPos = rAnimLocationNext.t;
		g_pAuxGeom->DrawSphere(	WPos+ipol.ieoutput,0.01f, RGBA8(0xff,0x00,0x00,0xff) );
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColDebug, false,"distance: %f  Tri:%d  Dist:%f",ipol.distance,c,d);	g_YLine+=16.0f;
		g_pAuxGeom->DrawSphere(	WPos+ipol.output,0.01f, RGBA8(0x00,0x00,0xff,0xff) );

		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColDebug, false,"t0: %f %f",ipol.ipv.x,ipol.iepv.x );	g_YLine+=16.0f;
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColDebug, false,"t1: %f %f",ipol.ipv.y,ipol.iepv.y );	g_YLine+=16.0f;
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColDebug, false,"t2: %f %f",ipol.ipv.z,ipol.iepv.z );	g_YLine+=16.0f;

		f32 sum0	=	ipol.ipv.x	+	ipol.ipv.y	+	ipol.ipv.z;
		f32 sum1	=	ipol.iepv.x	+	ipol.iepv.y	+	ipol.iepv.z;
		f32 sum2	=	ipol.iepv.x/5+ipol.iepv.y/5+ipol.iepv.z/5;
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColDebug, false,"sum: %f %f %f",sum0,sum1,sum2 );	g_YLine+=16.0f;
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColDebug, false,"AimIKBlend:%f   m_AimIKFadeout:%d", m_AimIKBlend, m_AimIKFadeout);	g_YLine+=16.0f;
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColDebug, false,"cosine:%f",cosine);	g_YLine+=16.0f;
	}
#endif

	if (m_AimIKBlend)
	{
		BlendAimPoses(params, ipol, arrAimIKPoses, fIKBlend);
	}

	//create the new absolute pose
	for (uint32 i=1; i<numJoints; i++)
	{
		int32 p = m_parrModelJoints[i].m_idxParent;
		pSkeletonAbsolutPose[i]	= pSkeletonAbsolutPose[p] * pSkeletonRelativePose[i];
		pSkeletonAbsolutPose[i].q.Normalize();
	}


#if !defined(__SPU__)
	//Draw aim-direction
	if( Console::GetInst().ca_DrawAimPoses)
	{
		int32 WeaponIdx = pIdxArray[eIM_RWeaponBoneIdx];
		Vec3 newdir	=	pSkeletonAbsolutPose[WeaponIdx].q.GetColumn1()*10;
		Vec3 RWBone	= rAnimLocationNext.q*pSkeletonAbsolutPose[WeaponIdx].t;
		Vec3 WStart = rAnimLocationNext.t+RWBone;
		Vec3 WEnd   = rAnimLocationNext.t+RWBone + rAnimLocationNext.q*newdir;
		g_pAuxGeom->DrawLine(	WStart,RGBA8(0x00,0x7f,0x7f,0xff), WEnd,RGBA8(0x00,0xff,0xff,0xff) );
	}
#endif

#ifdef XENON_DEBUG
	if (Console::GetInst().ca_UseAimIK == 2)
		XTraceStopRecording();
#endif

	return true;
}

//--------------------------------------------------------------------------
//-----              make a local array with all 49 aimposes            ---
//--------------------------------------------------------------------------

SPU_NO_INLINE uint32 CPoseBlenderAim::BuildLocalAimPoseArray(const SAnimationPoseModiferParams& params, AimIKPose* parrAimIKPoses)
{
	CModelSkeleton* m_pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);
	uint32 numRotJoints = m_pModelSkeleton->m_AimIK_Rot.size();
	uint32 numPosJoints = m_pModelSkeleton->m_AimIK_Pos.size();

	//reset all positions and rotations
	for (uint32 i=0; i<AIM_POSES; i++) 
	{
		AimIKPose &rDstIKPose = parrAimIKPoses[i];
		if (numRotJoints)
		{
			Quat* const __restrict pDstRotation = &rDstIKPose.m_arrRotation[0];
			for (uint32 j=0; j<numRotJoints; j++)
			{
				pDstRotation[j].w   = 0.0f;
				pDstRotation[j].v.x = 0.0f;
				pDstRotation[j].v.y = 0.0f;
				pDstRotation[j].v.z = 0.0f;
			}
		}
		if (numPosJoints)
		{
			Vec3* const __restrict pDstPosition = &rDstIKPose.m_arrPosition[0];
			for (uint32 j=0; j<numPosJoints; j++)
			{
				pDstPosition[j].x = 0.0f;
				pDstPosition[j].y = 0.0f;
				pDstPosition[j].z = 0.0f;
			}
		}			
	}




	assert(m_numActiveAimPoses);
	f32 fTotalWeight = 0.0f;
	for (int32 a=0; a<m_numActiveAimPoses; a++)
	{
		fTotalWeight += BuildLocalAimPoseArrayOtherLayer( parrAimIKPoses, m_AimInfo[a], numRotJoints, numPosJoints );
	}
	assert( fabsf(fTotalWeight-1.0f) < 0.001f );



	for (uint32 p=0; p<AIM_POSES; p++)
	{
		Quat * const __restrict pRotation = &parrAimIKPoses[p].m_arrRotation[0];
		for (uint32 j=0; j<numRotJoints; j++)
			pRotation[j].Normalize();
	}

	//--------------------------------------------------------------------------
	m_AimIKInfluence=m_AimIKBlend;

	return 1;
}



SPU_NO_INLINE float CPoseBlenderAim::BuildLocalAimPoseArrayOtherLayer(AimIKPose* parrAimIKPoses, SAimInfo& rAimInfo, uint32 numRotJoints, uint32 numPosJoints)
{
	uint32 numAimPoses = rAimInfo.m_numAimPoses;
	assert(numAimPoses==1 || numAimPoses==2);

	assert(rAimInfo.m_nGlobalAimID0>=0);
	GlobalAnimationHeaderAIM& rGAH0 = g_AnimationManager.m_arrGlobalAIM[rAimInfo.m_nGlobalAimID0];
	uint32 numPoses = rGAH0.m_arrAimIKPosesAIM.size();
	assert(numPoses == AIM_POSES);
	if (numPoses==0)
		return 0;

	f32 fWeight = rAimInfo.m_fWeight;
	if (numAimPoses==1)
	{
		for (uint32 i=0; i<AIM_POSES; i++) 
		{
			AimIKPose &rSrcIKPose = rGAH0.m_arrAimIKPosesAIM[i];
			AimIKPose &rDstIKPose = parrAimIKPoses[i];

			if (numRotJoints)
			{
				Quat* const __restrict pSrcRotation = &rSrcIKPose.m_arrRotation[0];
				Quat* const __restrict pDstRotation = &rDstIKPose.m_arrRotation[0];
				for (uint32 j=0; j<numRotJoints; j++)
					pDstRotation[j] += pSrcRotation[j]*fWeight;
			}

			if (numPosJoints)
			{
				Vec3* const __restrict pSrcPosition = &rSrcIKPose.m_arrPosition[0];
				Vec3* const __restrict pDstPosition = &rDstIKPose.m_arrPosition[0];
				for (uint32 j=0; j<numPosJoints; j++)
					pDstPosition[j] += pSrcPosition[j]*fWeight;
			}

		}
	}
	else
	{				
		assert(rAimInfo.m_nGlobalAimID1>=0);
		GlobalAnimationHeaderAIM& rGAH1 = g_AnimationManager.m_arrGlobalAIM[rAimInfo.m_nGlobalAimID1];
		assert(rGAH1.m_arrAimIKPosesAIM.size() == AIM_POSES);

		f32 t0 = 1.0f-rAimInfo.m_fAnimTime;
		f32 t1 = rAimInfo.m_fAnimTime;
		for (uint32 i=0; i<AIM_POSES; i++) 
		{
			AimIKPose &rSrc0IKPose = rGAH0.m_arrAimIKPosesAIM[i];
			AimIKPose &rSrc1IKPose = rGAH1.m_arrAimIKPosesAIM[i];
			AimIKPose &rDstIKPose  = parrAimIKPoses[i];

			if (numRotJoints)
			{
				Quat* const __restrict pSrc0Rotation = &rSrc0IKPose.m_arrRotation[0];
				Quat* const __restrict pSrc1Rotation = &rSrc1IKPose.m_arrRotation[0];
				Quat* const __restrict pDstRotation = &rDstIKPose.m_arrRotation[0];
				for (uint32 j=0; j<numRotJoints; j++)
				{
					pDstRotation[j].w   += (pSrc0Rotation[j].w  *t0 + pSrc1Rotation[j].w  *t1)*fWeight;
					pDstRotation[j].v.x += (pSrc0Rotation[j].v.x*t0 + pSrc1Rotation[j].v.x*t1)*fWeight;
					pDstRotation[j].v.y += (pSrc0Rotation[j].v.y*t0 + pSrc1Rotation[j].v.y*t1)*fWeight;
					pDstRotation[j].v.z += (pSrc0Rotation[j].v.z*t0 + pSrc1Rotation[j].v.z*t1)*fWeight;
				}
			}

			if (numPosJoints)
			{
				Vec3* const __restrict pSrc0Postion = &rSrc0IKPose.m_arrPosition[0];
				Vec3* const __restrict pSrc1Postion = &rSrc1IKPose.m_arrPosition[0];
				Vec3* const __restrict pDstPostion = &rDstIKPose.m_arrPosition[0];
				for (uint32 j=0; j<numPosJoints; j++)
				{
					pDstPostion[j].x += (pSrc0Postion[j].x*t0 + pSrc1Postion[j].z*t1)*fWeight;
					pDstPostion[j].y += (pSrc0Postion[j].y*t0 + pSrc1Postion[j].y*t1)*fWeight;
					pDstPostion[j].z += (pSrc0Postion[j].z*t0 + pSrc1Postion[j].z*t1)*fWeight;
				}
			}
		}
	}

	return fWeight;
}






//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
namespace MathHelper 
{
	ILINE Vec3 Barycentric(const Line& line, const Vec3& v0, const Vec3& v1, const Vec3& v2)
	{
		// find vectors for two edges sharing v0
		Vec3r dir = line.direction;
		Vec3r edge_1 = v0-v2;
		Vec3r edge_2 = v1-v2;
		// begin calculating determinant - also used to calculate U parameter
		Vec3r pvec = dir % edge_1;

		// if determinat is near zero, ray lies in plane of triangel
		real det = edge_2 | pvec;
		if (fabs(det) < (real)0.0001)
			return Vec3(0, 0, 0);

		// calculate distance from v0 to ray origin
		Vec3r tvec = line.pointonline - v2;
		// prepare to test V parameter
		Vec3r qvec = tvec % edge_2;


		//calculate U parameter and test bounds
		real u = (dir | qvec);
		//calculate V parameter and test bounds
		real v = tvec | pvec;

		//------------------------------------------------------
		Vec3r output;
		output.x = u;
		output.y = v;
		output.z = det - (u+v);

		real sum = output.x + output.y + output.z;
		assert(sum);

		Vec3 barycentric;
		barycentric.x = f32(output.x/sum);
		barycentric.y = f32(output.y/sum);
		barycentric.z = f32(output.z/sum);
		return barycentric;
	}

	ILINE Vec3 ClosestPointTriangle(Vec3 p, Vec3 a, Vec3 b, Vec3 c)
	{
		Vec3 ba = b-a, ab = a-b, ca = c-a;
		Vec3 cb = c-b, bc = b-c, ac = a-c;
		Vec3 pa = p-a, pb = p-b, pc = p-c;
		Vec3 ap = a-p, bp = b-p, cp = c-p;

		f32 snom = pa | ba;
		f32 unom = pb | cb;
		f32 tnom = pa | ca;

		f32 sdenom = pb | ab;
		f32 tdenom = pc | ac;
		f32 udenom = pc | bc;

		if (snom <= 0.0f && tnom <= 0.0f) return a; // Vertex region early out
		if (sdenom <= 0.0f && unom <= 0.0f) return b; // Vertex region early out
		if (tdenom <= 0.0f && udenom <= 0.0f) return c; // Vertex region early out

		// P is outside (or on) AB if the triple scalar product [N PA PB] <= 0
		Vec3 n = ba % ca;
		f32 vc = n | (ap % bp);
		// If P outside AB and within feature region of AB, return projection of P onto AB
		if (vc <= 0.0f && snom >= 0.0f && sdenom >= 0.0f)
			return a + snom / (snom + sdenom) * ba;

		// P is outside (or on) BC if the triple scalar product [N PB PC] <= 0
		f32 va = n | (bp % cp);
		// If P outside BC and within feature region of BC, return projection of P onto BC
		if (va <= 0.0f && unom >= 0.0f && udenom >= 0.0f)
			return b + unom / (unom + udenom) * cb;

		// P is outside (or on) CA if the triple scalar product [N PC PA] <= 0
		f32 vb = n | (cp % ap);
		// If P outside CA and within feature region of CA, return projection of P onto CA
		if (vb <= 0.0f && tnom >= 0.0f && tdenom >= 0.0f)
			return a + tnom / (tnom + tdenom) * ca;

		// P must project inside face region. Compute Q using barycentric coordinates
		f32 u = va / (va + vb + vc);
		f32 v = vb / (va + vb + vc);
		f32 w = 1.0f - u - v; // = vc / (va + vb + vc)
		return u*a + v*b + w*c;
	}

} // namespace MathHelper


SPU_NO_INLINE IPol CPoseBlenderAim::CheckTriangles(const SAnimationPoseModiferParams& params, uint32 k0,uint32 k1,uint32 k2,ColorB rgb, SAimDirection* ls,const Vec3& localAimTarget, AimIKPose* parrAimIKPoses, f32 fIKBlend )
{
	CSkeletonPose* m_pSkeletonPose = PoseModifierHelper::GetSkeletonPose(params);
	CModelSkeleton* m_pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);

	QuatT* m_RelativePose = SPU_LOCAL_PTR( params.pPoseRelative );
	QuatT* m_AbsolutePose = SPU_LOCAL_PTR( params.pPoseAbsolute );

	CModelJoint* m_parrModelJoints = m_pSkeletonPose->m_parrModelJoints;

	if (ls[k0].evaluated==0)
		ls[k0]=EvaluatePose(params, k0, parrAimIKPoses);
	if (ls[k1].evaluated==0)
		ls[k1]=EvaluatePose(params, k1, parrAimIKPoses);
	if (ls[k2].evaluated==0)
		ls[k2]=EvaluatePose(params, k2, parrAimIKPoses);

	IPol ipol;
	ipol.k0=k0;
	ipol.k1=k1;
	ipol.k2=k2;

	Vec3 s0=ls[k0].start;
	Vec3 s1=ls[k1].start;
	Vec3 s2=ls[k2].start;

	Vec3 t0=ls[k0].end;
	Vec3 t1=ls[k1].end;
	Vec3 t2=ls[k2].end;

	//float fColor[4] = {0,0,1,1};
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"s0: %f %f %f   t0: %f %f %f",s0.x,s0.y,s0.z, t0.x,t0.y,t0.z );	g_YLine+=16.0f;
#if !defined(__SPU__)
	if( Console::GetInst().ca_DrawAimPoses==2)
	{
		const QuatTS& rAnimLocationNext = SPU_LOCAL_REF(params.locationNextAnimation);
		Vec3 WPos = rAnimLocationNext.t;
		ColorB tricol = RGBA8(uint8(rgb.r*fIKBlend),uint8(rgb.g*fIKBlend),uint8(rgb.b*fIKBlend),0xff);
		g_pAuxGeom->DrawLine(WPos+s0,tricol, WPos+s1,tricol );
		g_pAuxGeom->DrawLine(WPos+s1,tricol, WPos+s2,tricol );
		g_pAuxGeom->DrawLine(WPos+s2,tricol, WPos+s0,tricol );

		g_pAuxGeom->DrawLine(WPos+t0,tricol, WPos+t1,tricol );
		g_pAuxGeom->DrawLine(WPos+t1,tricol, WPos+t2,tricol );
		g_pAuxGeom->DrawLine(WPos+t2,tricol, WPos+t0,tricol );
	}
#endif
	ipol.ipv=Vec3(0.33333f,0.33333f,0.33333f);

	Vec3 origin,direction;
	uint32 i=0;
	for (i=0; i<33; i++)
	{

		origin = (s0*ipol.ipv.x + s1*ipol.ipv.y + s2*ipol.ipv.z);
		assert( origin.IsValid() );
		direction = (localAimTarget-origin).GetNormalized();
		assert( direction.IsValid() );

		ipol.iepv			=	MathHelper::Barycentric(Line(origin,direction),t0,t1,t2);
		assert( ipol.iepv.IsValid() );
		f32 sum=ipol.iepv.x+ipol.iepv.y+ipol.iepv.z;
		if (sum==0)
			break;


		ipol.ieoutput	=	t0*ipol.iepv.x + t1*ipol.iepv.y + t2*ipol.iepv.z;
		assert( ipol.ieoutput.IsValid() );

		ipol.output		=	MathHelper::ClosestPointTriangle(ipol.ieoutput,t0,t1,t2);
		assert( ipol.output.IsValid() );

		ipol.ipv			=	MathHelper::Barycentric(Line(ipol.output,direction),t0,t1,t2);
		assert( ipol.ipv.IsValid() );

		if (ipol.ipv.x<0.0f) ipol.ipv.x=0.0f;
		if (ipol.ipv.y<0.0f) ipol.ipv.y=0.0f;
		if (ipol.ipv.z<0.0f) ipol.ipv.z=0.0f;
		if (ipol.ipv.x>1.0f) ipol.ipv.x=1.0f;
		if (ipol.ipv.y>1.0f) ipol.ipv.y=1.0f;
		if (ipol.ipv.z>1.0f) ipol.ipv.z=1.0f;
		if (i>3)
		{
			if (ipol.iepv.x<0.0f) break;
			if (ipol.iepv.y<0.0f) break;
			if (ipol.iepv.z<0.0f) break;
			if (ipol.iepv.x>1.0f) break;
			if (ipol.iepv.y>1.0f) break;
			if (ipol.iepv.z>1.0f) break;
		}
	}

	if (i==0)
	{
		ipol.distance=99999.0f;
		ipol.inside=0;
		return ipol;
	}

	ipol.distance = (ipol.ieoutput-ipol.output).GetLength();


	//	float fColor[4] = {0,1,0,1};
	//	extern float g_YLine;
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"planecheck: %f %f",distS,distE );	g_YLine+=16.0f;

	ipol.inside=1;
	if (ipol.iepv.x<0.0f) ipol.inside=0;
	if (ipol.iepv.y<0.0f) ipol.inside=0;
	if (ipol.iepv.z<0.0f) ipol.inside=0;
	if (ipol.iepv.x>1.0f) ipol.inside=0;
	if (ipol.iepv.y>1.0f) ipol.inside=0;
	if (ipol.iepv.z>1.0f) ipol.inside=0;

	/*
	if (ipol.iepv.x<-2.0f) ipol.distance=99999.0f;
	if (ipol.iepv.y<-2.0f) ipol.distance=99999.0f;
	if (ipol.iepv.z<-2.0f) ipol.distance=99999.0f;
	if (ipol.iepv.x> 3.0f) ipol.distance=99999.0f;
	if (ipol.iepv.y> 3.0f) ipol.distance=99999.0f;
	if (ipol.iepv.z> 3.0f) ipol.distance=99999.0f;
	*/

	f32 sum=ipol.iepv.x+ipol.iepv.y+ipol.iepv.z;
	if (sum==0)
		ipol.distance=99999.0f;

	Plane plane=Plane::CreatePlane(t0,t1,t2);
	f32 distS=plane|origin;
	f32 distE=plane|(origin+direction*100.0f);
	if (distS>0 && distE>0)
	{
		ipol.distance=99999.0f;
		ipol.inside=0;
	}
	if (distS<0 && distE<0)
	{
		ipol.distance=99999.0f;
		ipol.inside=0;
	}

	return ipol;
}


SAimDirection CPoseBlenderAim::EvaluatePose(const SAnimationPoseModiferParams& params, uint32 aimpose, AimIKPose* parrAimIKPoses )
{
	CSkeletonPose* m_pSkeletonPose = PoseModifierHelper::GetSkeletonPose(params);
	CSkeletonAnim* m_pSkeletonAnim = PoseModifierHelper::GetSkeletonAnim(params);
	CModelSkeleton* m_pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);

	QuatT* m_RelativePose = SPU_LOCAL_PTR( params.pPoseRelative );
	QuatT* m_AbsolutePose = SPU_LOCAL_PTR( params.pPoseAbsolute );

	CModelJoint* m_parrModelJoints = m_pSkeletonPose->m_parrModelJoints;

	const QuatTS& rAnimLocationNext = SPU_LOCAL_REF( params.locationNextAnimation );

	//

	QuatT qtemp; 
	int16 const* __restrict pIdxArray = m_pModelSkeleton->m_IdxArray;
	const Quat* pSrc = &parrAimIKPoses[MIDDLE].m_arrRotation[0];

	uint32 numRotJoints = m_pModelSkeleton->m_AimIK_Rot.size();
	for (uint32 b=0; b<numRotJoints; b++)
	{
		if (m_pModelSkeleton->m_AimIK_Rot[b].m_nPreEvaluate)
		{
			int32 j	= m_pModelSkeleton->m_AimIK_Rot[b].m_nJointIdx;
			int32 a	= m_pModelSkeleton->m_AimIK_Rot[b].m_nAdditive;
			if (a)
			{
				Quat additive = (!pSrc[b]*parrAimIKPoses[aimpose].m_arrRotation[b]);
				qtemp.q	= m_RelativePose[j].q*additive;     //additive blend
			}
			else
				qtemp.q = parrAimIKPoses[aimpose].m_arrRotation[b];

			qtemp.t = m_RelativePose[j].t;
			int32 p	= m_pModelSkeleton->m_AimIK_Rot[b].m_nPosIndex;
			if (p>-1)
				qtemp.t=parrAimIKPoses[aimpose].m_arrPosition[p];

			uint32 pidx = m_parrModelJoints[j].m_idxParent;
			m_AbsolutePose[j]	= m_AbsolutePose[pidx] * qtemp;
		}
	}

	f32 fAimDirScale=0.5f;
	int32 widx =	pIdxArray[eIM_RWeaponBoneIdx];
	Vec3 vAimDirection	=	(rAnimLocationNext.q*m_AbsolutePose[widx].q).GetColumn1();
	Vec3 RWBone0	= rAnimLocationNext.q*m_AbsolutePose[widx].t + vAimDirection*fAimDirScale;
	Vec3 RWBone1	= RWBone0+vAimDirection;

#if !defined(__SPU__)
	if( Console::GetInst().ca_DrawAimPoses)
	{
		/*
		Vec3 vPolarCoord;
		vPolarCoord.x = atan2f(-vAimDirection.x,vAimDirection.y);
		f32 s,c; sincos_tpl(vPolarCoord.x,&s,&c);
		vPolarCoord.y = atan2f(vAimDirection.z,-vAimDirection.x*s+vAimDirection.y*c);
		vPolarCoord.z=0;
		float fColor[4] = {1,0,0,1};
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"Polar: %f  %f",vPolarCoord.x,vPolarCoord.y);	
		g_YLine+=16.0f;

		SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
		g_pAuxGeom->SetRenderFlags( renderFlags );
		AABB aabbBoundBox = AABB(Vec3(-0.02f,-0.02f,-0.02f),Vec3(+0.002f,+0.002f,+0.002f));
		Matrix33 m33=Matrix33::CreateIdentity();
		OBB obb=OBB::CreateOBBfromAABB( m33,aabbBoundBox );
		g_pAuxGeom->DrawOBB(obb,vPolarCoord,1,RGBA8(0xff,0x10,0x1f,0x00),eBBD_Extremes_Color_Encoded);
		*/

		Vec3 WPos	=	rAnimLocationNext.t;
		g_pAuxGeom->DrawLine(	WPos+RWBone0,RGBA8(0x7f,0x7f,0x7f,0xff), WPos+RWBone1,RGBA8(0xff,0xff,0xff,0xff) );
	}
#endif

	SAimDirection tmp;
	tmp.start = RWBone0;
	tmp.end   = RWBone1;
	tmp.evaluated = true;
	return tmp;
}

//-------------------------------------------------------------------------------

SPU_NO_INLINE void CPoseBlenderAim::BlendAimPoses(const SAnimationPoseModiferParams& params, IPol &ipol, AimIKPose *arrAimIKPoses, f32 fIKBlend)
{
	CSkeletonPose* m_pSkeletonPose = PoseModifierHelper::GetSkeletonPose(params);
	CSkeletonAnim* m_pSkeletonAnim = PoseModifierHelper::GetSkeletonAnim(params);
	CModelSkeleton* m_pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);

	QuatT* m_RelativePose = SPU_LOCAL_PTR( params.pPoseRelative );
	QuatT* m_AbsolutePose = SPU_LOCAL_PTR( params.pPoseAbsolute );

	CModelJoint* m_parrModelJoints = m_pSkeletonPose->m_parrModelJoints;
	//

	QuatT* 	const	__restrict	pSkeletonRelativePose	= &m_RelativePose[0];
	//	int16 const * __restrict pIdxArray = m_pModelSkeleton->m_IdxArray;

	assert(m_numActiveAimPoses);
	assert(ipol.k0<AIM_POSES);
	assert(ipol.k1<AIM_POSES);
	assert(ipol.k2<AIM_POSES);

	uint32 numRotJoints = m_pModelSkeleton->m_AimIK_Rot.size();
	for (uint32 b=0; b<numRotJoints; b++)
	{
		const Quat& rSrc1 = arrAimIKPoses[ipol.k0].m_arrRotation[b];
		const Quat& rSrc2 = arrAimIKPoses[ipol.k1].m_arrRotation[b];
		const Quat& rSrc3 = arrAimIKPoses[ipol.k2].m_arrRotation[b];

		const Quat* pSrc4 = &arrAimIKPoses[MIDDLE].m_arrRotation[0];

		assert((fabs_tpl(1-(rSrc1|rSrc1)))<0.00001);   //check if unit-quaternion
		assert((fabs_tpl(1-(rSrc2|rSrc2)))<0.00001);   //check if unit-quaternion
		assert((fabs_tpl(1-(rSrc3|rSrc3)))<0.00001);   //check if unit-quaternion
		int32 j	= m_pModelSkeleton->m_AimIK_Rot[b].m_nJointIdx;
		int32 a	= m_pModelSkeleton->m_AimIK_Rot[b].m_nAdditive;
		if (a)
			BlendAimPoses_Additive(params, ipol,j, rSrc1, rSrc2, rSrc3, pSrc4[b], fIKBlend);
		else
			BlendAimPoses_Normal(params, ipol,j, rSrc1, rSrc2, rSrc3, fIKBlend);
	}

	uint32 numPosJoints = m_pModelSkeleton->m_AimIK_Pos.size();
	for (uint32 b=0; b<numPosJoints; b++)
	{
		const Vec3& rSrc1 = arrAimIKPoses[ipol.k0].m_arrPosition[b];
		const Vec3& rSrc2 = arrAimIKPoses[ipol.k1].m_arrPosition[b];
		const Vec3& rSrc3 = arrAimIKPoses[ipol.k2].m_arrPosition[b];
		int32 j	= m_pModelSkeleton->m_AimIK_Pos[b].m_nJointIdx;
		BlendAimPoses_Normal(params, ipol,j, rSrc1, rSrc2, rSrc3, fIKBlend);
	}

	//	int32 j=pIdxArray[eIM_LHand2Weapon_IKBlendIdx];   //optional IK target
	//	if (j>0)
	//		pSkeletonRelativePose[j].t = pSkeletonRelativePose[j].t*min(m_AimIKBlend,m_AimIKInfluence);

}



void CPoseBlenderAim::BlendAimPoses_Additive(const SAnimationPoseModiferParams& params, IPol &ipol, int32 j,const Quat& aim0,const Quat& aim1,const Quat& aim2,const Quat& rel, f32 fIKBlend)
{
	CSkeletonPose* m_pSkeletonPose = PoseModifierHelper::GetSkeletonPose(params);
	CModelSkeleton* m_pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);

	QuatT* m_RelativePose = SPU_LOCAL_PTR( params.pPoseRelative );
	QuatT* m_AbsolutePose = SPU_LOCAL_PTR( params.pPoseAbsolute );

	CModelJoint* m_parrModelJoints = m_pSkeletonPose->m_parrModelJoints;
	//extern float g_YLine;
	//float fColor[4] = {0,1,0,1};
	//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"pSrc: %f (%f %f %f)",rel.w, rel.v.x,rel.v.y,rel.v.z );	g_YLine+=16.0f;

	assert(j>0 && j<256); //an index is bad
	QuatT* 	const	__restrict	pSkeletonRelativePose	= &m_RelativePose[0];
	if (j>0 && j<256 ) 
	{
		assert((fabs_tpl(1-(aim0|aim0)))<0.00001);   //check if unit-quaternion
		assert((fabs_tpl(1-(aim1|aim1)))<0.00001);   //check if unit-quaternion
		assert((fabs_tpl(1-(aim2|aim2)))<0.00001);   //check if unit-quaternion
		//do additive blending
//		Quat aq0 = pSkeletonRelativePose[j].q*(1.0f-fIKBlend) + (pSkeletonRelativePose[j].q*(!rel*aim0))*fIKBlend;
//		Quat aq1 = pSkeletonRelativePose[j].q*(1.0f-fIKBlend) + (pSkeletonRelativePose[j].q*(!rel*aim1))*fIKBlend;
//		Quat aq2 = pSkeletonRelativePose[j].q*(1.0f-fIKBlend) + (pSkeletonRelativePose[j].q*(!rel*aim2))*fIKBlend;
		Quat aq0; aq0.SetNlerp(pSkeletonRelativePose[j].q,pSkeletonRelativePose[j].q*(!rel*aim0),fIKBlend);
		Quat aq1; aq1.SetNlerp(pSkeletonRelativePose[j].q,pSkeletonRelativePose[j].q*(!rel*aim1),fIKBlend);
		Quat aq2; aq2.SetNlerp(pSkeletonRelativePose[j].q,pSkeletonRelativePose[j].q*(!rel*aim2),fIKBlend);
/*
		if( (aq0|aq1) < 0 ) 
			aq1=-aq1;
		if( (aq0|aq2) < 0 ) 
			aq2=-aq2; 
*/

		pSkeletonRelativePose[j].q = (aq0*ipol.iepv.x) + (aq1*ipol.iepv.y) + (aq2*ipol.iepv.z);
		pSkeletonRelativePose[j].q.Normalize();
	}
}


void CPoseBlenderAim::BlendAimPoses_Normal(const SAnimationPoseModiferParams& params, IPol &ipol, int32 j,const Quat& aim0,const Quat& aim1,const Quat& aim2,f32 fIKBlend)
{
	CSkeletonPose* m_pSkeletonPose = PoseModifierHelper::GetSkeletonPose(params);
	CModelSkeleton* m_pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);

	QuatT* m_RelativePose = SPU_LOCAL_PTR( params.pPoseRelative );
	QuatT* m_AbsolutePose = SPU_LOCAL_PTR( params.pPoseAbsolute );

	CModelJoint* m_parrModelJoints = m_pSkeletonPose->m_parrModelJoints;

	//
	assert(j>0 && j<256); //an index is bad
	QuatT* 	const	__restrict	pSkeletonRelativePose	= &m_RelativePose[0];
	if (j>0 && j<256) 
	{
		assert((fabs_tpl(1-(aim0|aim0)))<0.00001);   //check if unit-quaternion
		assert((fabs_tpl(1-(aim1|aim1)))<0.00001);   //check if unit-quaternion
		assert((fabs_tpl(1-(aim2|aim2)))<0.00001);   //check if unit-quaternion
		//do normal blending

/*
		const char* pname = m_parrModelJoints[j].GetJointName();
		f32 dot = m_parrModelJoints[j].m_DefaultRelPose.q|pSkeletonRelativePose[j].q;
		assert(dot>=0);

		f32 dot0 = (m_parrModelJoints[j].m_DefaultRelPose.q|aim0);
		f32 dot1 = (m_parrModelJoints[j].m_DefaultRelPose.q|aim1);
		f32 dot2 = (m_parrModelJoints[j].m_DefaultRelPose.q|aim2);
		assert(dot0>=0);
		assert(dot1>=0);
		assert(dot2>=0);

		if (dot0<0 || dot1<0 || dot2<0 )
		{
			float fColor[4] = {1,0,1,1};
			extern f32 g_YLine;
			g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"JointName: %s    %f %f %f",pname,dot0,dot1,dot2); 
			g_YLine+=0x10;
		}
*/

//		f32 dot0 = (pSkeletonRelativePose[j].q|aim0);
//		f32 dot1 = (pSkeletonRelativePose[j].q|aim1);
//		f32 dot2 = (pSkeletonRelativePose[j].q|aim2);
//		assert(dot0>=0);
//		assert(dot1>=0);
//		assert(dot2>=0);
//		Quat aq0=pSkeletonRelativePose[j].q*(1.0f-fIKBlend) + aim0*fIKBlend;
//		Quat aq1=pSkeletonRelativePose[j].q*(1.0f-fIKBlend) + aim1*fIKBlend;
//		Quat aq2=pSkeletonRelativePose[j].q*(1.0f-fIKBlend) + aim2*fIKBlend;

		Quat aq0; aq0.SetNlerp(pSkeletonRelativePose[j].q,aim0,fIKBlend);
		Quat aq1; aq1.SetNlerp(pSkeletonRelativePose[j].q,aim1,fIKBlend);
		Quat aq2; aq2.SetNlerp(pSkeletonRelativePose[j].q,aim2,fIKBlend);
	
		/*
		if( (aq0|aq1) < 0 ) 
			aq1=-aq1;
		if( (aq0|aq2) < 0 ) 
			aq2=-aq2; 
*/
		pSkeletonRelativePose[j].q = (aq0*ipol.iepv.x) + (aq1*ipol.iepv.y) + (aq2*ipol.iepv.z);
		pSkeletonRelativePose[j].q.Normalize();
	}
}

void CPoseBlenderAim::BlendAimPoses_Normal(const SAnimationPoseModiferParams& params, IPol &ipol, int32 j,const Vec3& aim0,const Vec3& aim1,const Vec3& aim2,f32 fIKBlend)
{
	CSkeletonPose* m_pSkeletonPose = PoseModifierHelper::GetSkeletonPose(params);
	CModelSkeleton* m_pModelSkeleton = PoseModifierHelper::GetModelSkeleton(params);

	QuatT* m_RelativePose = SPU_LOCAL_PTR( params.pPoseRelative );
	QuatT* m_AbsolutePose = SPU_LOCAL_PTR( params.pPoseAbsolute );

	CModelJoint* m_parrModelJoints = m_pSkeletonPose->m_parrModelJoints;

	//
	assert(j>0 && j<256); //an index is bad
	QuatT* 	const	__restrict	pSkeletonRelativePose	= &m_RelativePose[0];
	if (j>0 && j<256) 
	{
		//do normal blending
		Vec3 aq0=pSkeletonRelativePose[j].t*(1.0f-fIKBlend) + aim0*fIKBlend;
		Vec3 aq1=pSkeletonRelativePose[j].t*(1.0f-fIKBlend) + aim1*fIKBlend;
		Vec3 aq2=pSkeletonRelativePose[j].t*(1.0f-fIKBlend) + aim2*fIKBlend;
		pSkeletonRelativePose[j].t = (aq0*ipol.iepv.x) + (aq1*ipol.iepv.y) + (aq2*ipol.iepv.z);
	}
}

// free function to get access to temp data for CrySizer
namespace AimPoseTempData
{
	void GetMemoryUsage( ICrySizer *pSizer )
	{
		gAnimPoseContainer.GetMemoryUsage(pSizer);
	}
}


#include UNIQUE_VIRTUAL_WRAPPER(IAnimationPoseBlenderAim)
