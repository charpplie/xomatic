////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Crytek Character Animation source code
//	
//	History:
//	10/9/2004 - Created by Ivo Herzeg <ivo@crytek.de>
//
//  Contains:
//  default-skeleton of the model   
/////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ModelSkeleton.h"
#include <StlUtils.h>
#include "StringUtils.h"
#include "crc32.h"





CModelJoint::~CModelJoint()
{
	IPhysicalWorld *pIPhysicalWorld = g_pIPhysicalWorld;
	IGeomManager* pPhysGeomManager = pIPhysicalWorld?pIPhysicalWorld->GetGeomManager():NULL;

	for(int nLod=0; nLod<2; nLod++)
	{
		if (m_PhysInfo[nLod].pPhysGeom && (INT_PTR)m_PhysInfo[nLod].pPhysGeom!=-1) 
		{
			if ((INT_PTR)m_PhysInfo[nLod].pPhysGeom<0x400) 
			{
				//CryWarning(VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,"Error: trying to release wrong bone phys geometry");
			} 
			else 
				if (pPhysGeomManager)
				{
					phys_geometry* pPhysGeom = m_PhysInfo[nLod].pPhysGeom;
					if (pPhysGeom->pForeignData)
						pPhysGeomManager->UnregisterGeometry((phys_geometry*)pPhysGeom->pForeignData);
					pPhysGeomManager->UnregisterGeometry(pPhysGeom);
				}
				else
					CryWarning(VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,"todo: delete bones phys");
		}
	}
}



// scales the bone with the given multiplier
void CModelJoint::scale (f32 fScale)
{
	assert(0);
}








//////////////////////////////////////////////////////////////////////////
// Updates the given lod level bone physics info from the bones found in
// the given chunk.
// THis is required to update the dead body physics info from the lod1 geometry
// without rebuilding the new bone structure. If the lod1 has another bone structure,
// then the bones are mapped to the lod0 ones using controller ids. Matching bones'
// physics info is updated
//////////////////////////////////////////////////////////////////////////
void CModelJoint::UpdateHierarchyPhysics( DynArray<BONE_ENTITY> arrBoneEntities, int nLod )
{


	UnsignedToCryBoneMap mapCtrlId;
	AddHierarchyToControllerIdMap(mapCtrlId);

	uint32 numBoneEntities = arrBoneEntities.size();

	// update physics for each bone entity
	for (uint32 i=0; i<numBoneEntities; i++)
	{
		CModelJoint* pBone = stl::find_in_map(mapCtrlId, arrBoneEntities[i].ControllerID, (CModelJoint*)NULL);
		if (pBone)
		{
			if (nLod==1)
				CopyPhysInfo(pBone->m_PhysInfo[nLod], arrBoneEntities[i].phys);

			int nFlags = 0;
			float mass = 0.0f;
			if (!arrBoneEntities[i].prop[0])
			{
				nFlags = joint_no_gravity|joint_isolated_accelerations;
			}
			else
			{
				if (!CryStringUtils::strnstr(arrBoneEntities[i].prop,"gravity", sizeof(arrBoneEntities[i].prop)))
					nFlags |= joint_no_gravity;
				if (!CryStringUtils::strnstr(arrBoneEntities[i].prop,"active_phys",sizeof(arrBoneEntities[i].prop)))
					nFlags |= joint_isolated_accelerations;
				if (const char *ptr = CryStringUtils::strnstr(arrBoneEntities[i].prop,"mass",sizeof(arrBoneEntities[i].prop)))
				{
					for(ptr+=4; *ptr && !(*ptr>='0' && *ptr<='9'); ptr++);
					if (*ptr)
						mass = (float)atof(ptr);
				}
			}
			assert(nLod<2);
			(pBone->m_PhysInfo[nLod].flags &= ~(joint_no_gravity|joint_isolated_accelerations)) |= nFlags;
			pBone->m_fMass = max(pBone->m_fMass, mass);
		}

	}

}


//////////////////////////////////////////////////////////////////////////
// Updates the given lod level bone physics info from the bones found in
// the given chunk.
// THis is required to update the dead body physics info from the lod1 geometry
// without rebuilding the new bone structure. If the lod1 has another bone structure,
// then the bones are mapped to the lod0 ones using controller ids. Matching bones'
// physics info is updated
void CModelJoint::UpdateHierarchyPhysics (const BONEANIM_CHUNK_DESC* pChunk, unsigned nChunkSize, int nLodLevel)
{
	UnsignedToCryBoneMap mapCtrlId;
	AddHierarchyToControllerIdMap(mapCtrlId);

	// the first bone entity
	const BONE_ENTITY* pBoneEntity = (const BONE_ENTITY*)(pChunk+1);
	// the actual end of the chunk
	const BONE_ENTITY* pBoneEntityEnd = (const BONE_ENTITY*)(((const char*)pChunk)+nChunkSize);

	// if you get this assert, it means the lod 1 file is corrupted
	if (pBoneEntity + pChunk->nBones > pBoneEntityEnd)
	{
		assert (0);
		return;
	}

	// update physics for each bone entity
	for (; pBoneEntity < pBoneEntityEnd; ++pBoneEntity)
	{
		CModelJoint* pBone = stl::find_in_map(mapCtrlId, pBoneEntity->ControllerID, (CModelJoint*)NULL);
		if (pBone)
		{
			//pBone->UpdatePhysics(*pBoneEntity, nLodLevel);

			//		assert (nLod >= 0 && nLod < SIZEOF_ARRAY(m_PhysInfo));
			CopyPhysInfo(pBone->m_PhysInfo[nLodLevel], pBoneEntity->phys);

			int nFlags = 0;
			if (!pBoneEntity->prop[0])
			{
				nFlags = joint_no_gravity|joint_isolated_accelerations;
			}
			else
			{
				if (!CryStringUtils::strnstr(pBoneEntity->prop,"gravity", sizeof(pBoneEntity->prop)))
					nFlags |= joint_no_gravity;

				if (!CryStringUtils::strnstr(pBoneEntity->prop,"active_phys",sizeof(pBoneEntity->prop)))
					nFlags |= joint_isolated_accelerations;
			}

			(pBone->m_PhysInfo[nLodLevel].flags &= ~(joint_no_gravity|joint_isolated_accelerations)) |= nFlags;


		}
	}
}

//////////////////////////////////////////////////////////////////////////
// adds this bone and all its children to the given map controller id-> bone ptr
//////////////////////////////////////////////////////////////////////////
void CModelJoint::AddHierarchyToControllerIdMap (UnsignedToCryBoneMap& mapControllerIdToCryBone)
{
	mapControllerIdToCryBone.insert (UnsignedToCryBoneMap::value_type(m_nJointCRC32, this));

	uint32 numChilds = numChildren();
	for (uint32 i=0; i<numChilds; i++)
		getChild(i)->AddHierarchyToControllerIdMap(mapControllerIdToCryBone);

}

//! Performs post-initialization. This step is requred to initialize the pPhysGeom of the bones
//! After the bone has been loaded but before it is first used. When the bone is first loaded, pPhysGeom
//!   is set to the value equal to the chunk id in the file where the physical geometry (BoneMesh) chunk is kept.
//! After those chunks are loaded, and chunk ids are mapped to the registered physical geometry objects,
//!   call this function to replace pPhysGeom chunk ids with the actual physical geometry object pointers.
//! RETURNS:
//    true if the corresponding physical geometry object has been found
//!	NOTE:
//!	The entries of the map that were used are deleted
bool CModelJoint::PostInitPhysGeom (ChunkIdToPhysGeomMap& mapChunkIdToPhysGeom, int nLodLevel, bool bAllowRopePhys)
{
	phys_geometry*& pPhysGeom = m_PhysInfo[nLodLevel].pPhysGeom;
	ChunkIdToPhysGeomMap::iterator it = mapChunkIdToPhysGeom.find ((INT_PTR)pPhysGeom);
	pPhysGeom = NULL;
	*(int*)(m_PhysInfo[nLodLevel].spring_angle+1) = 0;
	if (it != mapChunkIdToPhysGeom.end()) 
	{
		int id = it->second->pGeom->GetPrimitiveId(0,0);
		if (id<0)
			id = it->second->surface_idx;
		if ((unsigned int)id < (unsigned int)it->second->nMats)
			id = it->second->pMatMapping[id];
		*(int*)(m_PhysInfo[nLodLevel].spring_angle+1) = id;
		if (!bAllowRopePhys && !strnicmp(GetJointName(),"rope",4))
			return false;
		// remap the chunk id to the actual pointer to the geometry
		pPhysGeom = it->second;
		mapChunkIdToPhysGeom.erase (it);
		return true;
	} 
	return false;
}

void CModelJoint::PostInitialize(const Skeleton::CPoseData& poseData)
{
	m_qDefaultRelPhysParent[0].SetIdentity();
	m_qDefaultRelPhysParent[1].SetIdentity();

	if (getParent())
	{
		CModelJoint *pParent,*pPhysParent;
		for(int nLod=0; nLod<2; nLod++)
		{
			for(pParent=pPhysParent=getParent(); pPhysParent && !pPhysParent->m_PhysInfo[nLod].pPhysGeom; pPhysParent=pPhysParent->getParent());
			{
				if (pPhysParent) 
				{
					m_qDefaultRelPhysParent[nLod] =
						!poseData.m_jointsAbsolute[pParent->m_idx].q *
						poseData.m_jointsAbsolute[pPhysParent->m_idx].q;
				}
				else 
				{
					m_qDefaultRelPhysParent[nLod].SetIdentity();
				}
			}
		}
	}

#if (EXTREME_TEST==1)
	uint32 valid0 = m_qDefaultRelPhysParent[0].IsValid();
	if (valid0==0)
		CryFatalError("CryAnimation: m_qRelPhysParent[0] is invalid in PostInitialize" );

	uint32 valid1 = m_qDefaultRelPhysParent[1].IsValid();
	if (valid1==0)
		CryFatalError("CryAnimation: m_qRelPhysParent[1] is invalid in PostInitialize" );
#endif

}


//---------------------------------------------------------------------------------
//---      for certain IK-operation we need special bone-configurations      ------
//---------------------------------------------------------------------------------
void CModelSkeleton::SetIKJointID()
{

	uint32 numJoints	= m_arrModelJoints.size();
	for (uint32 i=0; i<numJoints; i++)
	{
		const char * BoneName =  m_arrModelJoints[i].GetJointName();
		if (0 == stricmp(BoneName,"Bip01"))
			m_IdxArray[eIM_BipIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 Pelvis"))
			m_IdxArray[eIM_PelvisIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 Spine"))
			m_IdxArray[eIM_Spine0Idx] = i;  
		if (0 == stricmp(BoneName,"Bip01 Spine1"))
			m_IdxArray[eIM_Spine1Idx] = i; 	
		if (0 == stricmp(BoneName,"Bip01 Spine2"))
			m_IdxArray[eIM_Spine2Idx] = i;
		if (0 == stricmp(BoneName,"Bip01 Spine3"))
			m_IdxArray[eIM_Spine3Idx] = i; 	
		if (0 == stricmp(BoneName,"Bip01 Neck"))
			m_IdxArray[eIM_NeckIdx] = i; 
		if (0 == stricmp(BoneName,"Bip01 Neck1"))
			m_IdxArray[eIM_NeckIdx1] = i; 
		if (0 == stricmp(BoneName,"Bip01 Head"))
			m_IdxArray[eIM_HeadIdx] = i;
		if (0 == stricmp(BoneName,"eye_left_bone"))
			m_IdxArray[eIM_LeftEyeIdx] = i;
		if (0 == stricmp(BoneName,"eye_right_bone"))
			m_IdxArray[eIM_RightEyeIdx] = i;


		if (0 == stricmp(BoneName,"Bip01 R Clavicle"))
			m_IdxArray[eIM_RClavicleIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 R UpperArm"))
			m_IdxArray[eIM_RUpperArmIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 R ForeArm"))
			m_IdxArray[eIM_RForeArmIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 R Hand"))
			m_IdxArray[eIM_RHandIdx] = i;


		if (0 == stricmp(BoneName,"Bip01 L Clavicle"))
			m_IdxArray[eIM_LClavicleIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 L UpperArm"))
			m_IdxArray[eIM_LUpperArmIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 L ForeArm"))
			m_IdxArray[eIM_LForeArmIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 L Hand"))
			m_IdxArray[eIM_LHandIdx] = i;



		if (0 == stricmp(BoneName,"Bip01 L Thigh"))
			m_IdxArray[eIM_LThighIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 L Calf"))
			m_IdxArray[eIM_LCalfIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 L Foot"))
			m_IdxArray[eIM_LFootIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 L Heel"))
			m_IdxArray[eIM_LHeelIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 L Toe0"))
			m_IdxArray[eIM_LToe0Idx] = i;

		if (0 == stricmp(BoneName,"Bip01 R Thigh"))
			m_IdxArray[eIM_RThighIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 R Calf"))
			m_IdxArray[eIM_RCalfIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 R Foot"))
			m_IdxArray[eIM_RFootIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 R Heel"))
			m_IdxArray[eIM_RHeelIdx] = i;
		if (0 == stricmp(BoneName,"Bip01 R Toe0"))
			m_IdxArray[eIM_RToe0Idx] = i;




		//if both bone-names are in the skeleton, then we assume this is the Hunter
		if (0 == stricmp(BoneName,"root"))
			m_IdxArray[eIM_HunterRootIdx] = i;

		if (0 == stricmp(BoneName,"frontLegLeft01"))
			m_IdxArray[eIM_HunterFrontLegL01Idx] = i;
		if (0 == stricmp(BoneName,"frontLegLeft12"))
			m_IdxArray[eIM_HunterFrontLegL12Idx] = i;

		if (0 == stricmp(BoneName,"frontLegRight01"))
			m_IdxArray[eIM_HunterFrontLegR01Idx] = i;
		if (0 == stricmp(BoneName,"frontLegRight12"))
			m_IdxArray[eIM_HunterFrontLegR12Idx] = i;

		if (0 == stricmp(BoneName,"backLegLeft02"))
			m_IdxArray[eIM_HunterBackLegL01Idx] = i;
		if (0 == stricmp(BoneName,"backLegLeft13"))
			m_IdxArray[eIM_HunterBackLegL13Idx] = i;

		if (0 == stricmp(BoneName,"backLegRight02"))
			m_IdxArray[eIM_HunterBackLegR01Idx] = i;
		if (0 == stricmp(BoneName,"backLegRight13"))
			m_IdxArray[eIM_HunterBackLegR13Idx] =i;
	}


	//------------------------------------------------------------------------
	//------------------------------------------------------------------------
	//------------------------------------------------------------------------

	m_arrMirrorJoints.resize(numJoints);
	for (uint32 i=0; i<numJoints; i++)
		m_arrMirrorJoints[i]=-1;

	int32 i=-1;
	i=m_IdxArray[eIM_PelvisIdx];
	if (i>0) m_arrMirrorJoints[i]=i;
	i=m_IdxArray[eIM_Spine0Idx];
	if (i>0) m_arrMirrorJoints[i]=i;
	i=m_IdxArray[eIM_Spine1Idx];
	if (i>0) m_arrMirrorJoints[i]=i;
	i=m_IdxArray[eIM_Spine2Idx];
	if (i>0) m_arrMirrorJoints[i]=i;
	i=m_IdxArray[eIM_Spine3Idx];
	if (i>0) m_arrMirrorJoints[i]=i;
	i=m_IdxArray[eIM_NeckIdx];
	if (i>0) m_arrMirrorJoints[i]=i;
	i=m_IdxArray[eIM_HeadIdx];
	if (i>0) m_arrMirrorJoints[i]=i;


	i=m_IdxArray[eIM_RClavicleIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_LClavicleIdx];
	i=m_IdxArray[eIM_RUpperArmIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_LUpperArmIdx];
	i=m_IdxArray[eIM_RForeArmIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_LForeArmIdx];
	i=m_IdxArray[eIM_RHandIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_LHandIdx];

	i=m_IdxArray[eIM_LClavicleIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_RClavicleIdx];
	i=m_IdxArray[eIM_LUpperArmIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_RUpperArmIdx];
	i=m_IdxArray[eIM_LForeArmIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_RForeArmIdx];
	i=m_IdxArray[eIM_LHandIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_RHandIdx];


	i=m_IdxArray[eIM_RThighIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_LThighIdx];
	i=m_IdxArray[eIM_RCalfIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_LCalfIdx];
	i=m_IdxArray[eIM_RFootIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_LFootIdx];
	i=m_IdxArray[eIM_RToe0Idx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_LToe0Idx];

	i=m_IdxArray[eIM_LThighIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_RThighIdx];
	i=m_IdxArray[eIM_LCalfIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_RCalfIdx];
	i=m_IdxArray[eIM_LFootIdx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_RFootIdx];
	i=m_IdxArray[eIM_LToe0Idx];
	if (i>0) m_arrMirrorJoints[i]=m_IdxArray[eIM_RToe0Idx];


}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
int32 CModelSkeleton::GetJointIDByName(const char* strJointName) const
{
	uint32 crc32 = g_pCrc32Gen->GetCRC32Lowercase(strJointName);
	uint32 numJoints	= m_arrModelJoints.size();
	for (uint32 i=0; i<numJoints; i++)
	{
		if (m_arrModelJoints[i].m_nJointCRC32Lower==crc32)
			return i;
	}
	return -1;
}


/*
//needed for debugging
int32 CModelSkeleton::GetJointIDByName(const char* strJointName) const
{
	uint32 crc32 = g_pCrc32Gen->GetCRC32Lowercase(strJointName);
	uint32 numJoints	= m_arrModelJoints.size();


	int32 crc_i = -1;
	for (uint32 i=0; i<numJoints; i++)
	{
		uint32 nCRC32 = m_arrModelJoints[i].m_nJointCRC32Lower;
		const char* strName =  m_arrModelJoints[i].GetJointName();
		if (m_arrModelJoints[i].m_nJointCRC32Lower==crc32)
		{
			crc_i=i;
			break;
		}
	}

	for (uint32 i=0; i<numJoints; i++)
	{
		uint32 nCRC32 = m_arrModelJoints[i].m_nJointCRC32Lower;
		const char* strName =  m_arrModelJoints[i].GetJointName();
		if (0 == stricmp(strName,strJointName))
		{
			assert(i==crc_i);
			return i;
		}
	}
	return -1;
}*/


// Return name of bone from bone table, return zero id nId is out of range
const char* CModelSkeleton::GetJointNameByID(int32 nJointID) const
{
	int32 numJoints = m_arrModelJoints.size();
	if(nJointID>=0 && nJointID<numJoints)
		return m_arrModelJoints[nJointID].GetJointName();
	assert("GetJointNameByID - Index out of range!");
	return ""; // invalid bone id
}

/*
SJointsAimIK_Rot::SJointsAimIK_Rot(class CModelSkeleton* pModelSkeleton, const char* pJointname, int8 nPreEvaluate, int8 nAdditive )
{
	m_strJointName	=	pJointname;
	m_nJointIdx			=	pModelSkeleton->GetJointIDByName(pJointname);
	m_nPreEvaluate	=	nPreEvaluate;
	m_nAdditive			=	nAdditive;
	m_nPosIndex			=	-1;
	CRY_ASSERT_MESSAGE(m_nJointIdx>-1, m_strJointName);
}

SJointsAimIK_Pos::SJointsAimIK_Pos(class CModelSkeleton* pModelSkeleton, const char* pJointname )
{
	m_strJointName	=	pJointname;
	m_nJointIdx			=	pModelSkeleton->GetJointIDByName(pJointname);
	CRY_ASSERT_MESSAGE(m_nJointIdx>-1, m_strJointName);
}*/


#include UNIQUE_VIRTUAL_WRAPPER(ICharacterModelSkeleton)
