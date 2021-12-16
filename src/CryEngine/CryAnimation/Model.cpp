////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Crytek Character Animation source code
//	
//	History:
//	28/09/2004 - Created by Ivo Herzeg <ivo@crytek.de>
//
//  Contains:
//  stores the loaded model and animations
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <CryHeaders.h>
#include <I3DEngine.h>
#include "CryHeaders.h"
#include "Model.h"
#include "CharacterInstance.h"
#include "CharacterManager.h"
#include "StringUtils.h"
#include "FacialAnimation/FacialModel.h"

static int GetDefaultPhysMaterial()
{
	I3DEngine* pEngine = g_pI3DEngine;
	if (pEngine)
	{
		IPhysMaterialEnumerator *pMatEnum = pEngine->GetPhysMaterialEnumerator();
		if (pMatEnum)
			return pMatEnum->EnumPhysMaterial("mat_default");
	}
	return 0; // the default in case there's no physics yet
}

CCharacterModel::CCharacterModel (const string& strFileName, CharacterManager* pManager, uint32 type, const char* szName) : m_strFilePath (strFileName), m_name(szName)
{
	m_IsProcessed=0;
	m_pFacialModel = 0;
	m_ObjectType=type;
	m_AnimationSet.Init();
	m_AnimationSet.m_pModel=this;

	m_bHasPhysics2 = 0;
	m_pManager=pManager;
	m_vModelOffset = Vec3(0,0,0);
	m_nDefaultGameID = GetDefaultPhysMaterial();

	// the material physical game id that will be used as default for this character
	m_pMaterial = g_pI3DEngine->GetMaterialManager()->GetDefaultMaterial();
	 
	for (int i=0; i<g_nMaxGeomLodLevels; i++)
		m_pRenderMeshs[i] = 0;

	m_pKinCharInstance = 0;
	m_nRefCounter = 0;
	m_nBaseLOD = 0;
	m_arrAnimationLOD.reserve(8);
}

CCharacterModel::~CCharacterModel()
{
	SAFE_RELEASE(m_pFacialModel);

	if (!m_RefByInstances.empty())
	{
		g_pILog->LogToFile("*ERROR* ~CCharacterModel(%s): %u character instances still not deleted. Forcing deletion.", m_strFilePath.c_str(), m_RefByInstances.size());
		CleanupInstances();
	}

	m_pManager->UnregisterModel(this);

	//FIXME:
	//g_AnimationManager.RemoveLoaderDBA(m_strTracksDBFilePath);

	// delete render meshes
	for (int i=0; i<g_nMaxGeomLodLevels; i++)
	{
		if (m_pRenderMeshs[i])
			g_pIRenderer->DeleteRenderMesh(m_pRenderMeshs[i]);
	}

	//g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,GetFilePath(), "RELEASE MODEL: %s", GetFilePath().c_str() );
	
}

//////////////////////////////////////////////////////////////////////////
void CCharacterModel::RegisterInstance( CSkinInstance* pInstance )
{
	if (m_RefByInstances.empty())
		m_RefByInstances.reserve(16);

	stl::push_back_unique(m_RefByInstances,pInstance);
}

//////////////////////////////////////////////////////////////////////////
void CCharacterModel::UnregisterInstance( CSkinInstance* pInstance )
{
	stl::find_and_erase( m_RefByInstances,pInstance );
}

// destroys all characters
// THis may (and should) lead to destruction and self-deregistration of this body
void CCharacterModel::CleanupInstances()
{
	// since after each instance deletion the body may be destructed itself,
	// we'll lock it for this time
	uint32 numInstances = m_RefByInstances.size();
	if (!m_RefByInstances.empty())
		g_pISystem->Warning (VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, 0, "CCharacterModel.CleanupInstances", "Forcing deletion of %d instances for body %s. CRASH POSSIBLE because other subsystems may have stored dangling pointer(s).", NumRefs(), m_strFilePath.c_str());

	CCharacterModel_AutoPtr pLock = this; // don't remove this line, it's for locking the body in memory untill every instance is finished.

	std::vector<CSkinInstance*> instances = m_RefByInstances; // Cleaning up attachments may modify list of instances
	for (int i = 0; i < instances.size(); i++)
	{
		instances[i]->AddRef(); // Lock all instances, so that they are not deleted by ShutDown if cross referenced.
	}
	for (int i = 0; i < instances.size(); i++)
	{
		instances[i]->ShutDown();
	}
	for (int i = 0; i < instances.size(); i++)
	{
		instances[i]->DeleteThis(); // Delete all instances
	}
}

//////////////////////////////////////////////////////////////////////////
void CCharacterModel::CleanupAttachments()
{
	std::vector<CSkinInstance*> instances = m_RefByInstances; // Cleaning up attachments may modify list of instances
	for (int i = 0; i < instances.size(); i++)
	{
		instances[i]->AddRef(); // Lock all instances.
	}
	for (int i = 0; i < instances.size(); i++)
	{
		instances[i]->m_AttachmentManager.RemoveAllAttachments();
	}
	for (int i = 0; i < instances.size(); i++)
	{
		instances[i]->Release();// Release all instances.
	}
}

//////////////////////////////////////////////////////////////////////////
CMorphTarget* CCharacterModel::getMorphSkin (unsigned nLOD, int nMorphTargetId)
{
	/*
	if (nLOD==0  &&  nMorphTargetId>=0  &&  nMorphTargetId<(int)GetModelMesh(0)->m_morphTargets.size() )
		return GetModelMesh(0)->m_morphTargets[nMorphTargetId];
	else
	{
		return 0;
	}
	*/

	if (nMorphTargetId>=0  &&  nMorphTargetId<(int)GetModelMesh(nLOD)->m_morphTargets.size() )
		return GetModelMesh(nLOD)->m_morphTargets[nMorphTargetId];
	else
	{
		return 0;
	}
}

const Vec3& CCharacterModel::getModelOffset()const
{
	return m_vModelOffset;
}



// sets the memory for the given vector, compatible with the STL vector by value_type, size() and &[0]
#define MEMSET_VECTOR(arr,value) memset (&((arr)[0]),value,sizeof(arr[0])*arr.size())


//////////////////////////////////////////////////////////////////////////
void CCharacterModel::PostLoad()
{
	CModelMesh *pModelMesh = GetModelMesh(0);
	if (pModelMesh)
	{
		if (pModelMesh->m_morphTargets.size() != 0)
		{
			m_pFacialModel = new CFacialModel( this );
			m_pFacialModel->AddRef();
		}
	}
}

//////////////////////////////////////////////////////////////////////////

template<class T> void _swap(T &op1, T &op2) { T tmp=op1; op1=op2; op2=tmp; }

int GetMeshApproxFlags(const char *str, int len)
{
	int flags=0;
	if (CryStringUtils::strnstr(str,"box",len))
		flags |= mesh_approx_box;
	else if (CryStringUtils::strnstr(str,"cylinder",len))
		flags |= mesh_approx_cylinder;
	else if (CryStringUtils::strnstr(str,"capsule", len))
		flags |= mesh_approx_capsule;
	else if (CryStringUtils::strnstr(str,"sphere", len))
		flags |= mesh_approx_sphere;	
	return flags;
}

void CCharacterModel::UpdatePhysBonePrimitives(DynArray<BONE_ENTITY> arrBoneEntities, int iLOD)
{
	CModelJoint *pBone;
	mesh_data *pmesh;
	IGeometry *pMeshGeom;
	const char *pcloth;
	std::map<unsigned, CModelJoint*> mapCtrlId;
	IGeomManager *pGeoman = g_pIPhysicalWorld->GetGeomManager();
	m_ModelSkeleton.m_arrModelJoints[0].AddHierarchyToControllerIdMap(mapCtrlId);

	for (int i=arrBoneEntities.size()-1; i>=0; i--)
		if (arrBoneEntities[i].prop[0] && (pBone = stl::find_in_map(mapCtrlId, arrBoneEntities[i].ControllerID, 0)) && 
				pBone->m_PhysInfo[iLOD].pPhysGeom && pBone->m_PhysInfo[iLOD].pPhysGeom->pGeom->GetType()==GEOM_TRIMESH)
		{
			pmesh = (mesh_data*)(pMeshGeom=pBone->m_PhysInfo[iLOD].pPhysGeom->pGeom)->GetData();
			if (pmesh->nIslands==2 && (pcloth=CryStringUtils::strnstr(arrBoneEntities[i].prop,"cloth_proxy",sizeof(arrBoneEntities[i].prop))))
			{
				int itri,j,isle=isneg(pmesh->pIslands[1].V-pmesh->pIslands[0].V);
				for(itri=pmesh->pIslands[isle].itri,j=0; j<pmesh->pIslands[isle].nTris; itri=pmesh->pTri2Island[isle].inext,j++) {
					for(int ivtx=0;ivtx<3;ivtx++) _swap(pmesh->pIndices[itri*3+ivtx], pmesh->pIndices[j*3+2-ivtx]);
					_swap(pmesh->pMats[itri], pmesh->pMats[j]);
				}
				phys_geometry *pgeomMain,*pgeomCloth;
				int flags = GetMeshApproxFlags(arrBoneEntities[i].prop, pcloth-arrBoneEntities[i].prop);
				flags |= (flags || pmesh->pIslands[isle^1].nTris<20) ? mesh_SingleBB : mesh_OBB;
				pMeshGeom = pGeoman->CreateMesh(pmesh->pVertices, strided_pointer<uint16>((uint16*)(pmesh->pIndices+j)+eLittleEndian,sizeof(pmesh->pIndices[0])), 
					pmesh->pMats+j, 0,pmesh->pIslands[isle^1].nTris, flags,1.0f);
				pgeomMain = pGeoman->RegisterGeometry(pMeshGeom, pBone->m_PhysInfo[iLOD].pPhysGeom->surface_idx, 
					pBone->m_PhysInfo[iLOD].pPhysGeom->pMatMapping,pBone->m_PhysInfo[iLOD].pPhysGeom->nMats);

				flags = GetMeshApproxFlags(pcloth, sizeof(arrBoneEntities[i].prop)-(pcloth-arrBoneEntities[i].prop));
				flags |= (flags || pmesh->pIslands[isle].nTris<20) ? mesh_SingleBB : mesh_OBB;
				pMeshGeom = pGeoman->CreateMesh(pmesh->pVertices, strided_pointer<uint16>((uint16*)pmesh->pIndices+eLittleEndian,sizeof(pmesh->pIndices[0])), 
					pmesh->pMats, 0,j, flags,1.0f);
				pgeomCloth = pGeoman->RegisterGeometry(pMeshGeom, pBone->m_PhysInfo[iLOD].pPhysGeom->surface_idx, 
					pBone->m_PhysInfo[iLOD].pPhysGeom->pMatMapping,pBone->m_PhysInfo[iLOD].pPhysGeom->nMats);
				pgeomMain->pForeignData = pgeomCloth;

				pGeoman->UnregisterGeometry(pBone->m_PhysInfo[iLOD].pPhysGeom);
				pBone->m_PhysInfo[iLOD].pPhysGeom = pgeomMain;
				continue;
			}
			int flags = GetMeshApproxFlags(arrBoneEntities[i].prop, sizeof(arrBoneEntities[i].prop));
			if (!flags)
				continue;
			pBone->m_PhysInfo[iLOD].pPhysGeom->pGeom = pGeoman->CreateMesh(pmesh->pVertices,
				strided_pointer<uint16>((uint16*)pmesh->pIndices+eLittleEndian,sizeof(pmesh->pIndices[0])), pmesh->pMats, 0,pmesh->nTris,flags|mesh_SingleBB, 1.0f);
			pMeshGeom->Release();
		}
}

//////////////////////////////////////////////////////////////////////////
IMaterial* CCharacterModel::GetMaterial()
{
	if (this->pCGA_Object)
		return this->pCGA_Object->GetMaterial();

	return m_pMaterial; 
}

//////////////////////////////////////////////////////////////////////////
uint32 CCharacterModel::GetTextureMemoryUsage( ICrySizer *pSizer )
{
	uint32 nSize = 0;
	if (pSizer)
	{
		for (int i=0; i<g_nMaxGeomLodLevels; i++)
		{
			if (m_pRenderMeshs[i])
			{
				nSize += (uint32)m_pRenderMeshs[i]->GetTextureMemoryUsage( m_pMaterial,pSizer );
			}
		}
	}
	else
	{
		if (m_pRenderMeshs[0])
			nSize = (uint32)m_pRenderMeshs[0]->GetTextureMemoryUsage( m_pMaterial );
	}
	return nSize;
}

//////////////////////////////////////////////////////////////////////////
uint32 CCharacterModel::GetMeshMemoryUsage( ICrySizer *pSizer)
{
	uint32 nSize = 0;
	for (int i=0; i<g_nMaxGeomLodLevels; i++)
	{
		if (m_pRenderMeshs[i])
		{
			nSize += m_pRenderMeshs[i]->GetMemoryUsage(0, IRenderMesh::MEM_USAGE_ONLY_STREAMS);
		}
	}
	return nSize;
}

uint32 CCharacterModel::SizeOfModelData(ICrySizer *pSizer)
{
	uint32 nSize = sizeof(CCharacterModel) + m_RefByInstances.size() * sizeof(CSkinInstance*);

	{
		SIZER_COMPONENT_NAME(pSizer, "CModelMesh");
		uint32 numMeshes = m_arrModelMeshes.size();
		for(uint32 i=0; i<numMeshes; ++i)
			nSize += m_arrModelMeshes[i].SizeOfModelMesh(pSizer);

		pSizer->AddObject(&m_arrModelMeshes, nSize);
	}


	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "ModelSkeleton");
		nSize += m_ModelSkeleton.SizeOfSkeleton();
		pSizer->AddObject(&m_ModelSkeleton, m_ModelSkeleton.SizeOfSkeleton());
	}

	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "AnimationSets");
		nSize += m_AnimationSet.SizeOfAnimationSet();

		uint32 numDBFilePath = m_arrFilePathDBA.size();
		for (uint32 i=0; i<numDBFilePath; ++i)
			nSize += m_arrFilePathDBA[i].capacity();

		nSize += m_arrAnimationLOD.get_alloc_size();
		uint32 numAnimLOD = m_arrAnimationLOD.size();
		for (uint32 i=0; i<numAnimLOD; i++)
			nSize += m_arrAnimationLOD[i].get_alloc_size();

		pSizer->AddObject(&m_AnimationSet, m_AnimationSet.SizeOfAnimationSet());
	}

	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "FacialModel");
		if (m_pFacialModel)
		{
			nSize += m_pFacialModel->SizeOfThis();
			pSizer->AddObject(&m_pFacialModel, m_pFacialModel->SizeOfThis());
		}
	}

	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "CollisionInfo");
		uint32 coll=0;
		uint32 numCollisions = m_arrCollisions.size();
		for(uint32 i=0; i<numCollisions; i++)
		{
			coll += sizeof(MeshCollisionInfo);
			coll += m_arrCollisions[i].m_arrIndexes.get_alloc_size();
		}

		nSize += coll;
		pSizer->AddObject(&m_arrCollisions, coll);
	}



	return nSize;
}

void CCharacterModel::GetMemoryUsage(ICrySizer *pSizer) const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject(m_arrFilePathDBA);
	pSizer->AddObject(m_strAnimEventFilePath);
			
	{
		SIZER_COMPONENT_NAME(pSizer, "CModelMesh");
		pSizer->AddObject( m_arrModelMeshes );		
	}


	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "ModelSkeleton");
		pSizer->AddObject(m_ModelSkeleton);
	}

	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "AnimationSets");
		m_AnimationSet.GetMemoryUsage(pSizer);
		pSizer->AddObject( m_arrAnimationLOD );
	}

	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "FacialModel");
		pSizer->AddObject(m_pFacialModel);		
	}

	{
		SIZER_SUBCOMPONENT_NAME(pSizer, "CollisionInfo");
		pSizer->AddObject(m_arrCollisions);		
	}

}

//////////////////////////////////////////////////////////////////////////
IRenderMesh* CCharacterModel::GetRenderMesh( int nLod )
{
	if (nLod == -1)
		nLod = m_nBaseLOD;

	if (nLod < 0 || nLod >= g_nMaxGeomLodLevels)
		return 0;
	return m_pRenderMeshs[nLod];

}

//////////////////////////////////////////////////////////////////////////
uint32 CCharacterModel::GetNumLods()
{
	uint32 nLods = 0;
	for (int i=0; i<g_nMaxGeomLodLevels; i++)
	{
		if (m_pRenderMeshs[i])
			nLods++;
	}
	return nLods;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
phys_geometry* CCharacterModel::GetJointPhysGeom(uint32 jointIndex,int nLod)
{
  return m_ModelSkeleton.m_arrModelJoints[jointIndex].getPhysInfo(nLod).pPhysGeom;
}

DynArray<SJointProperty> CCharacterModel::GetJointPhysProperties(uint32 jointIndex, int nLod) 
{
	if (jointIndex>=(uint32)m_ModelSkeleton.m_arrModelJoints.size() || (uint32)nLod>=(uint32)m_arrModelMeshes.size())
		return DynArray<SJointProperty>();
	CModelJoint& joint = m_ModelSkeleton.m_arrModelJoints[jointIndex];
	if (!strnicmp(joint.GetJointName(),"rope",4) && 
			(!joint.getParent() || strnicmp(joint.getParent()->GetJointName(),"rope",4)))
	{
		const char *ptr;
		for(ptr=joint.GetJointName(); *ptr && *ptr!=' '; ptr++);
		int i, len=ptr-joint.GetJointName();
		for(i=m_ModelSkeleton.m_arrModelJoints.size()-1; i>=0; i--)	
		{
			const CModelJoint &joint1 = m_ModelSkeleton.m_arrModelJoints[i];
			if (i!=jointIndex && !strncmp(joint1.GetJointName(),joint.GetJointName(),len) && 
					(!joint1.getParent() || strnicmp(joint1.getParent()->GetJointName(),"rope",4)))
				break;
		}
		return GetPhysInfoProperties(joint.getPhysInfo(nLod), i>=0);
	}
	return DynArray<SJointProperty>();
}

DynArray<SJointProperty> CCharacterModel::GetPhysInfoProperties(CryBonePhysics &pi, int type)
{
	DynArray<SJointProperty> res;
	if (type==0) 
	{
		res.push_back(SJointProperty("Type", "Rope"));
		res.push_back(SJointProperty("Gravity",!(pi.flags & joint_no_gravity)));
		float t = pi.spring_tension[0];
		if (pi.min[0]!=0)
			t = RAD2DEG(fabs_tpl(pi.min[0]));
		res.push_back(SJointProperty("JointLimit",t));
		t = pi.spring_tension[1];
		if (t<=0 || t>=1)
			t = 0.02f;
		res.push_back(SJointProperty("MaxTimestep", t));
		res.push_back(SJointProperty("Stiffness", max(0.001f, RAD2DEG(pi.max[0]))));
		res.push_back(SJointProperty("StiffnessDecay", RAD2DEG(pi.max[1])));
		res.push_back(SJointProperty("Damping", RAD2DEG(pi.max[2])));
		res.push_back(SJointProperty("Friction", pi.spring_tension[2]));
		res.push_back(SJointProperty("SimpleBlending", !(pi.flags & 4)));
		res.push_back(SJointProperty("EnvCollisions", !(pi.flags & 1)));
		res.push_back(SJointProperty("BodyCollisions", !(pi.flags & 2)));
		res.push_back(SJointProperty("Mass", RAD2DEG(fabs_tpl(pi.min[1]))));
		res.push_back(SJointProperty("Thickness", RAD2DEG(fabs_tpl(pi.min[2]))));
	}	else if (type==1)
	{
		res.push_back(SJointProperty("Type", "Cloth"));
		res.push_back(SJointProperty("MaxTimestep", pi.damping[0]));
		res.push_back(SJointProperty("MaxStretch", pi.damping[1]));
		res.push_back(SJointProperty("Stiffness", RAD2DEG(pi.max[2])));
		res.push_back(SJointProperty("Thickness", pi.damping[2]));
		res.push_back(SJointProperty("Friction", pi.spring_tension[2]));
		res.push_back(SJointProperty("StiffnessNorm", RAD2DEG(pi.max[0])));
		res.push_back(SJointProperty("StiffnessTang", RAD2DEG(pi.max[1])));
		res.push_back(SJointProperty("Damping", pi.spring_tension[0]));
		res.push_back(SJointProperty("AirResistance", pi.spring_tension[1]));
	}
	return res;
}

bool CCharacterModel::SetJointPhysProperties(uint32 jointIndex, int nLod, const DynArray<SJointProperty> &props) 
{
	return ParsePhysInfoProperties(m_ModelSkeleton.m_arrModelJoints[jointIndex].getPhysInfo(nLod), props);
}

bool CCharacterModel::ParsePhysInfoProperties(CryBonePhysics &pi, const DynArray<SJointProperty> &props)
{
	if (props.size() && props[0].type>=2)
		if (!strcmp(props[0].strval,"Rope"))
		{
			*(int*)pi.spring_angle = 0x12345678;
			pi.flags &= ~joint_isolated_accelerations;
			for(int i=(int)props.size()-1; i>=0; i--) 
				if (!strcmp(props[i].name,"Gravity"))
					(pi.flags &= ~joint_no_gravity) |= (props[i].bval ? 0:joint_no_gravity);
				else if (!strcmp(props[i].name,"JointLimit"))
					pi.min[0] = -DEG2RAD(props[i].fval), pi.spring_tension[0]=0;
				else if (!strcmp(props[i].name,"MaxTimestep"))
					pi.spring_tension[1] = props[i].fval;
				else if (!strcmp(props[i].name,"Stiffness"))
					pi.max[0] = DEG2RAD(props[i].fval);
				else if (!strcmp(props[i].name,"StiffnessDecay"))
					pi.max[1] = DEG2RAD(props[i].fval);
				else if (!strcmp(props[i].name,"Damping"))
					pi.max[2] = DEG2RAD(props[i].fval);
				else if (!strcmp(props[i].name,"Friction"))
					pi.spring_tension[2] = props[i].fval;
				else if (!strcmp(props[i].name,"SimpleBlending"))
					(pi.flags &= ~4) |= (props[i].bval ? 0:4);
				else if (!strcmp(props[i].name,"EnvCollisions"))
					(pi.flags &= ~1) |= (props[i].bval ? 0:1);
				else if (!strcmp(props[i].name,"BodyCollisions"))
					(pi.flags &= ~2) |= (props[i].bval ? 0:2);
				else if (!strcmp(props[i].name,"Mass"))
					pi.min[1] = DEG2RAD(-props[i].fval);
				else if (!strcmp(props[i].name,"Thickness"))
					pi.min[2] = DEG2RAD(-props[i].fval);
			return true;
		} else if (!strcmp(props[0].strval,"Cloth"))
		{
			*(int*)pi.spring_angle = 0x12345678;
			for(int i=(int)props.size()-1; i>=0; i--) 
				if (!strcmp(props[i].name,"MaxTimestep"))
					pi.damping[0] = props[i].fval;
				else if (!strcmp(props[i].name,"MaxStretch"))
					pi.damping[1] = props[i].fval;
				else if (!strcmp(props[i].name,"Stiffness"))
					pi.max[2] = DEG2RAD(props[i].fval);
				else if (!strcmp(props[i].name,"Thickness"))
					pi.damping[2] = props[i].fval;
				else if (!strcmp(props[i].name,"Friction"))
					pi.spring_tension[2] = props[i].fval;
				else if (!strcmp(props[i].name,"StiffnessNorm"))
					pi.max[0] = DEG2RAD(props[i].fval);
				else if (!strcmp(props[i].name,"StiffnessTang"))
					pi.max[1] = DEG2RAD(props[i].fval);
				else if (!strcmp(props[i].name,"Damping"))
					pi.spring_tension[0] = props[i].fval;
				else if (!strcmp(props[i].name,"AirResistance"))
					pi.spring_tension[1] = props[i].fval;
			return true;
		}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CCharacterModel::AddModelTracksDatabase(const string& sAnimTracksDatabasePath) 
{
	string tmp(sAnimTracksDatabasePath);
	CryStringUtils::UnifyFilePath(tmp);
	uint32 numDBAs = m_arrFilePathDBA.size();
	for (int i=0; i<numDBAs; i++)
	{
		if (m_arrFilePathDBA[i].compareNoCase(tmp) == 0)
			return;
	}
	m_arrFilePathDBA.push_back(tmp);
}
CloseInfo CCharacterModel::FindClosestPointOnMesh( const Vec3& RMWPosition, bool bUseCenterPointDist ) 
{
  return m_arrModelMeshes[m_nBaseLOD].FindClosestPointOnMesh( RMWPosition, bUseCenterPointDist );
}

#include UNIQUE_VIRTUAL_WRAPPER(ICharacterModel)
