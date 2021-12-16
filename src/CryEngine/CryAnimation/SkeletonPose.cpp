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

#include <CryExtension/Impl/ClassWeaver.h>
#include <CryExtension/Impl/ICryFactoryRegistryImpl.h>
#include <CryExtension/CryCreateClassInstance.h>

#include <I3DEngine.h>
#include <IRenderAuxGeom.h>

#include "CharacterManager.h"
#include "CharacterInstance.h"
#include "Model.h"
#include "ModelSkeleton.h"

#include "Helper.h"
#include "ControllerOpt.h"

CSkeletonPose::CSkeletonPose()
{
	m_pPoseData = &m_poseData;
	m_pLocator = &m_locator;

	m_bGroundAlignDataReady = false;
	m_alignSkeletonVertical = false;
	m_rootHeight = 0.0f;

	m_poseModifiers.clear();
	m_poseModifiers.reserve(16);

	ICryFactoryRegistry* pRegistry = gEnv->pSystem->GetCryFactoryRegistry();

	::CryCreateClassInstance<IAnimationPoseBlenderAim>("AnimationPoseModifier_PoseBlenderAim", m_PoseBlenderAim);
	assert(m_PoseBlenderAim.get());

	::CryCreateClassInstance<IAnimationPoseModifier>("AnimationPoseModifier_LookAt", m_LookAt);
	assert(m_LookAt.get());

	memset(&m_auxPhys,0, sizeof(m_auxPhys));		
}

//////////////////////////////////////////////////////////////////////////
// initialize the moving skeleton 
//////////////////////////////////////////////////////////////////////////
void CSkeletonPose::InitSkeletonPose(CCharInstance* pInstance, CSkeletonAnim* pSkeletonAnim)
{
	m_pInstance = pInstance;
	m_pSkeletonAnim = pSkeletonAnim;
	m_pModelSkeleton = &pInstance->m_pModel->m_ModelSkeleton;
	m_parrModelJoints = &pInstance->m_pModel->m_ModelSkeleton.m_arrModelJoints[0];
	m_pPoseDataDefault = &pInstance->m_pModel->m_ModelSkeleton.m_poseData;

	m_poseData.Initialize(m_pInstance->m_pModel->m_ModelSkeleton);

	//usually not needed for skin-attachments
	m_LookIK().m_addHead.SetIdentity();
	m_LookIK().m_addLEye.SetIdentity();
	m_LookIK().m_addREye.SetIdentity();

	//------------------------------------------------------------
	m_nForceSkeletonUpdate=0;
	m_bAllAbsoluteJointsValid = false;
	m_LFootGroundAlign=0;
	m_RFootGroundAlign=0;
	m_enableFootGroundAlignment = (Console::GetInst().ca_GroundAlignment != 0);

	m_pPostProcessCallback = 0;	
	m_pPostProcessCallbackData = 0;
	m_pPostPhysicsCallback = 0;
	m_pPostPhysicsCallbackData = 0;


	//---------------------------------------------------------------------
	//---                   physics                                    ----
	//---------------------------------------------------------------------
	m_pCharPhysics = 0;
	m_bHasPhysics = 0;
	m_timeStandingUp = -1.0f;
	m_ppBonePhysics = 0;
	m_timeLying = -1.0f;
	m_timeNoColl = -1.0f;
	m_timeStandingUp = -1.0f;
	m_fScale = 0.01f;
	m_bPhysicsAwake = 0;
	m_bPhysicsWasAwake = 1;
	m_nAuxPhys = 0;
	m_fPhysBlendTime = 1E6f;
	m_fPhysBlendMaxTime = 1.0f;
	m_frPhysBlendMaxTime = 1.0f;
	m_bPhysicsRelinquished = 0;
	m_iFnPSet = 0;
	m_vOffset(0,0,0);
	m_velPivot.zero(); m_prevPosPivot.zero();



	m_vRenderOffset=Vec3(ZERO);
	m_AABB.min=Vec3(-2,-2,-2);	
	m_AABB.max=Vec3(+2,+2,+2);	
	m_Superimposed=0;

	SetDefaultPose();
}

//////////////////////////////////////////////////////////////////////////
// reset the bone to default position/orientation
// initializes the bones and IK limb pose
//////////////////////////////////////////////////////////////////////////
void CSkeletonPose::SetDefaultPose()
{
	SetDefaultPosePerInstance();	

	IAttachmentManager* pIAttachmentManager = m_pInstance->GetIAttachmentManager();
	uint32 numAttachments = pIAttachmentManager->GetAttachmentCount();
	for (uint32 i=0; i<numAttachments; i++)
	{
		CAttachment* pAttachment = (CAttachment*)pIAttachmentManager->GetInterfaceByIndex(i);
		IAttachmentObject* pIAttachmentObject = pAttachment->m_pIAttachmentObject;
		if (pIAttachmentObject)
		{
			if (pAttachment->GetType()==CA_SKIN || pAttachment->GetType()==CA_PART)
				continue;
			if (pIAttachmentObject->GetAttachmentType() != IAttachmentObject::eAttachment_Character)
				continue;

			CCharInstance* pCharacterInstance = (CCharInstance*)pAttachment->m_pIAttachmentObject->GetICharacterInstance();
			pCharacterInstance->m_SkeletonPose.SetDefaultPosePerInstance();	
		}
	}

	//ExportSkeleton();

}


void CSkeletonPose::SetDefaultPosePerInstance()
{
	m_pSkeletonAnim->StopAnimationsAllLayers();

	m_bInstanceVisible=0;
	m_bFullSkeletonUpdate=0;

	m_Recoil.Init();
	m_LookIK().Init();
	m_AimIK().Init();
	m_FootAnchor.Init();
	m_arrPostProcess.resize(0);

	uint32 numJoints = GetJointCount();
	if (numJoints)
	{
		if (GetPoseDataWriteable()) for (uint32 i=0; i<numJoints; i++)
		{
			GetPoseDataWriteable()->m_jointsRelative[i] = m_pModelSkeleton->m_poseData.m_jointsRelative[i];
			GetPoseDataWriteable()->m_jointsAbsolute[i] = m_pModelSkeleton->m_poseData.m_jointsAbsolute[i];
		}

		int32 leye = m_pModelSkeleton->m_IdxArray[eIM_LeftEyeIdx];
		int32 reye = m_pModelSkeleton->m_IdxArray[eIM_RightEyeIdx];
		if (GetPoseDataWriteable()) if (leye>0)
			GetPoseDataWriteable()->m_jointsRelative[leye].q.SetIdentity();
		if (GetPoseDataWriteable()) if (reye>0)
			GetPoseDataWriteable()->m_jointsRelative[reye].q.SetIdentity();
		m_LookIK().m_addHead.SetIdentity();
		m_LookIK().m_addLEye.SetIdentity();
		m_LookIK().m_addREye.SetIdentity();

		if (GetPoseDataWriteable()) for (uint32 i=1; i<numJoints; i++)
		{
			GetPoseDataWriteable()->m_jointsAbsolute[i] = GetPoseDataWriteable()->m_jointsRelative[i];
			int32 p = m_parrModelJoints[i].m_idxParent;
			if (p>=0)
				GetPoseDataWriteable()->m_jointsAbsolute[i]	= GetPoseDataWriteable()->m_jointsAbsolute[p] * GetPoseDataWriteable()->m_jointsRelative[i];
			GetPoseDataWriteable()->m_jointsAbsolute[i].q.Normalize();
		}
	}

	if ( m_pCharPhysics != NULL )
		if (m_pInstance->m_pModel->m_ObjectType==CGA)
			m_pInstance->UpdatePhysicsCGA( 1, QuatTS(IDENTITY) );
		else if (!m_bPhysicsRelinquished)
			SynchronizeWithPhysicalEntity( m_pCharPhysics, Vec3(ZERO), Quat(IDENTITY), QuatT(IDENTITY), 0 );
}



void CSkeletonPose::InitPhysicsSkeleton()
{

	assert(!m_arrPhysicsJoints.size());
	uint32 numJoints	= m_pInstance->m_pModel->m_ModelSkeleton.m_arrModelJoints.size();

	m_arrPhysicsJoints.resize(numJoints);
	for (uint32 i=0; i<numJoints; i++)
	{
		m_arrPhysicsJoints[i].m_DefaultRelativeQuat	=	m_pModelSkeleton->m_poseData.m_jointsRelative[i];
		m_arrPhysicsJoints[i].m_qRelPhysParent[0] = m_parrModelJoints[i].m_qDefaultRelPhysParent[0];
		m_arrPhysicsJoints[i].m_qRelPhysParent[1] = m_parrModelJoints[i].m_qDefaultRelPhysParent[1];
		
#if (EXTREME_TEST==1)
			uint32 valid1 = m_parrModelJoints[i].m_DefaultRelPose.IsValid();
			if (valid1==0)
			{
				assert(0);
				CryFatalError("CryAnimation: m_DefaultRelQuat nr. %d is invalid in InitPhysicsSkeleton: %s",i, m_pInstance->GetFilePath());
			}
			uint32 valid2 = m_parrModelJoints[i].m_qDefaultRelPhysParent[0].IsValid();
			if (valid2==0)
			{
				assert(0);
				CryFatalError("CryAnimation: m_qRelPhysParent[0] nr. %d is invalid in InitPhysicsSkeleton: %s",i, m_pInstance->GetFilePath());
			}
			uint32 valid3 = m_parrModelJoints[i].m_qDefaultRelPhysParent[1].IsValid();
			if (valid3==0)
			{
				assert(0);
				CryFatalError("CryAnimation: m_qRelPhysParent[1] nr. %d is invalid in InitPhysicsSkeleton: %s",i, m_pInstance->GetFilePath());
			}
#endif

	}
}

void CSkeletonPose::InitCGASkeleton()
{
	uint32 numJoints	= m_pInstance->m_pModel->m_ModelSkeleton.m_arrModelJoints.size();

	m_arrCGAJoints.resize(numJoints);

	for (uint32 i=0; i<numJoints; i++)
	{
		m_arrCGAJoints[i].m_CGAObjectInstance		= m_parrModelJoints[i].m_CGAObject;
	}
}



uint32 CSkeletonPose::GetChildrenCount(int parentID) const
{
	assert(parentID >= 0 && parentID < (int)m_pPoseData->m_jointsAbsolute.size());
	return m_parrModelJoints[parentID].m_numChildren;
}

int16 CSkeletonPose::GetChildID(int32 parentID, int32 childIndex) const
{
	assert(parentID >= 0 && parentID < (int)m_pPoseData->m_jointsAbsolute.size());
	assert(childIndex >= 0 && childIndex <  (int32)m_parrModelJoints[parentID].m_numChildren);
	return m_parrModelJoints[parentID].getChild(childIndex)->m_idx;
}

uint32 CSkeletonPose::GetJointCRC32 (int32 nJointID) const
{
	int32 numJoints = GetPoseDataDefault().GetJointCount();
	if(nJointID>=0 && nJointID<numJoints)
		return m_parrModelJoints[nJointID].m_nJointCRC32;
	assert(0);
	return ~0;
}



IStatObj* CSkeletonPose::GetStatObjOnJoint(int32 nId)
{
	if (nId < 0 || nId >= (int)GetPoseDataDefault().GetJointCount())
	{
		assert(0);
		return NULL;
	}
	if (m_arrCGAJoints.size())
	{
		const CCGAJoint& joint = m_arrCGAJoints[nId];
		return joint.m_CGAObjectInstance;
	}

	return 0;
}

void CSkeletonPose::SetStatObjOnJoint(int32 nId, IStatObj* pStatObj)
{
	if (nId < 0 || nId >= (int)GetPoseDataDefault().GetJointCount())
	{
		assert(0);
		return;
	}

	assert(m_arrCGAJoints.size());
	// do not handle physicalization in here, use IEntity->SetStatObj instead
	CCGAJoint& joint = m_arrCGAJoints[nId];  
	joint.m_CGAObjectInstance = pStatObj;  

	if(joint.m_pRNTmpData)
		gEnv->p3DEngine->FreeRNTmpData(&joint.m_pRNTmpData);
	assert(!joint.m_pRNTmpData);
}


void CSkeletonPose::SetPhysEntOnJoint(int32 nId, IPhysicalEntity *pPhysEnt)
{
	if (nId < 0 || nId >= (int)GetPoseDataDefault().GetJointCount())
	{
		assert(0);
		return;
	}
	if (!m_ppBonePhysics)
		memset(m_ppBonePhysics = new IPhysicalEntity*[GetPoseDataDefault().GetJointCount()], 0, GetPoseDataDefault().GetJointCount()*sizeof(IPhysicalEntity*));
	if (m_ppBonePhysics[nId])
		if (m_ppBonePhysics[nId]->Release()<=0)
			gEnv->pPhysicalWorld->DestroyPhysicalEntity(m_ppBonePhysics[nId]);
	m_ppBonePhysics[nId] = pPhysEnt;
	if (pPhysEnt)
		pPhysEnt->AddRef();
}

int CSkeletonPose::GetPhysIdOnJoint(int32 nId)
{
	if (nId < 0 || nId >= (int)GetPoseDataDefault().GetJointCount())
	{
		assert(0);
		return -1;
	}

	if (m_arrCGAJoints.size())
	{
		CCGAJoint& joint = m_arrCGAJoints[nId];

		if (joint.m_qqqhasPhysics>=0)
			return joint.m_qqqhasPhysics;
	}
	
	return -1;
}

void CSkeletonPose::DestroyPhysics()
{
	int i;
	if (m_pCharPhysics)
		g_pIPhysicalWorld->DestroyPhysicalEntity(m_pCharPhysics);
	m_pCharPhysics = 0;
	if (m_ppBonePhysics)
	{
		for(i=0;i<(int)GetPoseDataDefault().GetJointCount();i++)	if (m_ppBonePhysics[i])
			m_ppBonePhysics[i]->Release(), g_pIPhysicalWorld->DestroyPhysicalEntity(m_ppBonePhysics[i]);
		delete[] m_ppBonePhysics;
		m_ppBonePhysics = 0;
	}
	
	for(i=0;i<m_nAuxPhys;i++) {
		g_pIPhysicalWorld->DestroyPhysicalEntity(m_auxPhys[i].pPhysEnt);
		delete[] m_auxPhys[i].pauxBoneInfo;
		delete[] m_auxPhys[i].pVtx;
	}
	m_nAuxPhys = 0;
	m_bPhysicsRelinquished=m_bAliveRagdoll = 0;	m_timeStandingUp = -1;
}

void CSkeletonPose::SetAuxParams(pe_params* pf)
{
	for(int32 i=0; i<m_nAuxPhys; i++)
		m_auxPhys[i].pPhysEnt->SetParams(pf);
}



//--------------------------------------------------------------------
//---              hex-dump the skeleton                           ---
//--------------------------------------------------------------------





void CSkeletonPose::ExportSkeleton()
{
	return;
/*
	struct SBipedJoint
	{
		int32 m_idxParent;
		const char* m_strJointName;
		QuatT	m_DefRelativeQuat;			
		ILINE SBipedJoint()	{	m_idxParent=-1;	m_strJointName=0;	m_DefRelativeQuat.SetIdentity();};
		ILINE SBipedJoint(const char* jname) { m_strJointName=jname; };
	};

	//------------------------------------------------------------
	//---   initialize the skeleton with the primary joints    ---
	//------------------------------------------------------------
	SBipedJoint BipedSkeleton[] = 
	{
		//--------------> BoneNr.0000
		SBipedJoint("Bip01"),
		//--------------> BoneNr.0001
		SBipedJoint("Bip01 Pelvis"),
		//--------------> BoneNr.0002
		SBipedJoint("Bip01 Spine"),
		//--------------> BoneNr.0003
		SBipedJoint("Bip01 Spine1"),
		//--------------> BoneNr.0004
		SBipedJoint("Bip01 Spine2"),
		//--------------> BoneNr.0005
		SBipedJoint("Bip01 Spine3"),
		//--------------> BoneNr.0006
		SBipedJoint("Bip01 Neck"),
		//--------------> BoneNr.0007
		SBipedJoint("Bip01 Neck1"),
		//--------------> BoneNr.0008
		SBipedJoint("Bip01 Head"),



		//--------------> BoneNr.0009
		SBipedJoint("Bip01 L Clavicle"),
		//--------------> BoneNr.0010
		SBipedJoint("Bip01 L UpperArm"),
		//--------------> BoneNr.0011
		SBipedJoint("Bip01 L Forearm"),
		//--------------> BoneNr.0012
		SBipedJoint("Bip01 L Hand"),



		//--------------> BoneNr.0013
		SBipedJoint("Bip01 R Clavicle"),
		//--------------> BoneNr.0014
		SBipedJoint("Bip01 R UpperArm"),
		//--------------> BoneNr.0015
		SBipedJoint("Bip01 R Forearm"),
		//--------------> BoneNr.0016
		SBipedJoint("Bip01 R Hand"),


		//--------------> BoneNr.0017
		SBipedJoint("Bip01 L Thigh"),
		//--------------> BoneNr.0018
		SBipedJoint("Bip01 L Calf"),
		//--------------> BoneNr.0019
		SBipedJoint("Bip01 L Foot"),
		//--------------> BoneNr.0020
		SBipedJoint("Bip01 L Toe0"),


		//--------------> BoneNr.0021
		SBipedJoint("Bip01 R Thigh"),
		//--------------> BoneNr.0022
		SBipedJoint("Bip01 R Calf"),
		//--------------> BoneNr.0023
		SBipedJoint( "Bip01 R Foot"),
		//--------------> BoneNr.0024
		SBipedJoint( "Bip01 R Toe0")


	};

	uint32 numPrimaryJoints = sizeof(BipedSkeleton)/sizeof(SBipedJoint);

	struct TW
	{
		static ILINE int32 GetIDbyName(SBipedJoint* pBipedSkeleton, uint32 numPrimaryJoints, const char* strJName)
		{
			int32 idx=-1; 
			for(uint32 i=0; i<numPrimaryJoints; i++)
			{
				uint32 res = stricmp(pBipedSkeleton[i].m_strJointName,strJName);
				if (res==0)
				{
					idx=i; 
					break;
				}
			}
			return idx;
		}
	};


	//------------------------------------------------------------------
	//----                     dump all joints                      ----
	//------------------------------------------------------------------
	FILE* vstream = fopen( "c:\\Skeleton.txt", "w+b" );

	fprintf(vstream, "SBipedJoint BipedSkeleton[] = \r\n");  
	fprintf(vstream, "{\r\n");  

	uint32 numJoints	= m_ModelSkeleton.m_arrModelJoints.size();
	for (uint32 i=0; i<numPrimaryJoints; i++)
	{
		const char* pChildName	=  BipedSkeleton[i].m_strJointName;
		int32 cidx	=	GetJointIDByName(pChildName);
		int32 pidx	=	GetJointParentIDByID(cidx);
		const char* pParentName=0;
		if (pidx>-1)
		{
			pParentName	= m_ModelSkeleton.m_arrModelJoints[pidx].GetJointName();
			pidx=TW::GetIDbyName(BipedSkeleton, numPrimaryJoints, pParentName);
			assert(pidx>-1);
		}

		QuatT rel; rel.SetIdentity();
		if (cidx>-1)
			rel	= m_ModelSkeleton.m_arrModelJoints[cidx].m_DefaultRelPose;
			
		fprintf(vstream, "//--------------> BoneNr.%d\r\n",i);  
		fprintf(vstream, "SBipedJoint(%d,%01d,QuatT(Quat(%ff,%ff,%ff,%ff),Vec3(%ff,%ff,%ff)),\"%s\"),\r\n",(cidx>-1),pidx, rel.q.w,rel.q.v.x,rel.q.v.y,rel.q.v.z, rel.t.x,rel.t.y,rel.t.z,  pChildName);  
	}

	fprintf(vstream, "};\r\n");  
	fclose( vstream );

*/
}


#include UNIQUE_VIRTUAL_WRAPPER(ISkeletonPose)






