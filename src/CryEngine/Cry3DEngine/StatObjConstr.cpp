////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   statobjconstr.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: creation
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "StatObj.h"

#if !defined(__SPU__) // ONly scan the stuff needed for UpdateParticles

#include "IndexedMesh.h"
#include "../RenderDll/Common/Shadow_Renderer.h"
#include <IRenderer.h>
#include <CrySizer.h>
#include "ObjMan.h"
#include "RenderMeshMerger.h"


#define MAX_VERTICES_MERGABLE 15000
#define MAX_TRIS_IN_LOD_0 512
#define TRIS_IN_LOD_WARNING_RAIO (1.5f)
// Minimal ratio of Lod(n-1)/Lod(n) polygons to consider LOD for sub-object merging.
#define MIN_TRIS_IN_MERGED_LOD_RAIO (1.5f)

//////////////////////////////////////////////////////////////////////////
void CStatObj::FreeIndexedMesh()
{
	delete m_pIndexedMesh;
	m_pIndexedMesh=0;
}

void CStatObj::CalcRadiuses()
{
  if(!m_vBoxMin.IsValid() || !m_vBoxMax.IsValid())
  {
    Error("CStatObj::CalcRadiuses: Invalid bbox, File name: %s", m_szFileName.c_str());
    m_vBoxMin.zero();
    m_vBoxMax.zero();
  }

  m_fObjectRadius = m_vBoxMin.GetDistance(m_vBoxMax)*0.5f;
  float dxh = (float)max( fabs(GetBoxMax().x), fabs(GetBoxMin().x));
  float dyh = (float)max( fabs(GetBoxMax().y), fabs(GetBoxMin().y));
  m_fRadiusHors = (float)cry_sqrtf(dxh*dxh+dyh*dyh);
  m_fRadiusVert = (GetBoxMax().z - 0)*0.5f;// never change this
  m_vVegCenter = (m_vBoxMax+m_vBoxMin)*0.5f;
  m_vVegCenter.z = m_fRadiusVert;
}

CStatObj::CStatObj( ) 
{ 
  m_pData=0;
  m_nDataSize=0;
  m_pSubObjectsForRendering = 0;
  m_nMergedMemoryUsage = 0;
  m_nUsers = 0; // reference counter
	m_nChildUsers = 0;
  m_nLastDrawMainFrameId=0;
	m_nFlags = 0;
	m_nStreamingFinishedMainFrameId = 0;
  //m_nStreamingRequestMainFrameId = 0;
	m_fOcclusionAmount = -1;
  m_pHeightmap = NULL;
  m_nHeightmapSize = 0;
  memset(m_arrpLowLODs,0,sizeof(m_arrpLowLODs));
//  m_fMinViewDistance=1000000.f;
  m_nFileNodeId=-1;
  m_FileNodeName[0]=0;
  m_lastBooleanOpScale = 1.f;
  
	Init();

  GetCounter()->Add(this);

#ifdef TRACE_CGF_LEAKS
	m_sLoadingCallstack = GetSystem()->GetLoadingProfilerCallstack();
#endif
}

void CStatObj::Init() 
{
	m_pIndexedMesh = 0;
	m_nRenderTrisCount = m_nLoadedTrisCount = m_nLoadedVertexCount = 0;
	m_nRenderMatIds = 0;
  m_fObjectRadius = 0;
	m_fRadiusHors = 0;
	m_fRadiusVert = 0;
	m_pParentObject = 0;
	m_pClonedSourceObject = 0;
	m_bVehicleOnlyPhysics = 0;
	m_bBreakableByGame = 0;
	MARK_UNUSED m_idmatBreakable;

	m_nLoadedLodsNum = 1;
	m_nMinUsableLod0 = 0;
	m_nMaxUsableLod = 0;
	m_pLod0 = 0;
	
	m_aiVegetationRadius = -1.0f;
	m_phys_mass = -1.0f;
	m_phys_density = -1.0f;

	m_nMaterialLayersMask = 0;

	m_vBoxMin.Set(0,0,0); 
	m_vBoxMax.Set(0,0,0); 
	m_vVegCenter.Set(0,0,0);

//  m_pSMLSource = 0;

  m_pRenderMesh = 0;
	m_pRenderMeshOcclusion = 0;

	m_bDefaultObject=false;

	m_pMergedObject = 0;
  for(int i=0; i<MAX_STATOBJ_LODS_NUM; i++)
    if(m_arrpLowLODs[i])
      m_arrpLowLODs[i]->Init();

	m_pReadStream=0;
	m_nSubObjectMeshCount = 0;
  m_nRenderMeshMemoryUsage = 0;
  m_arrRenderMeshesPotentialMemoryUsage[0] = m_arrRenderMeshesPotentialMemoryUsage[1] = -1;

	m_bUseStreaming  = false;
  m_bLodsLoaded = false;
	m_bDefaultObject = false;
	m_bOpenEdgesTested = false;
	m_bSubObject = false;
	m_bSharesChildren = false;
	m_bHasDeformationMorphs = false;
	m_bTmpIndexedMesh = false;
	m_bMerged = false;
	m_bUnmergable = false;
	m_bLowSpecLod0Set = false;
	m_bForInternalUse = false;
	m_bHaveOcclusionProxy = false;
	m_bCheckGarbage = false;
  m_bLodsAreLoadedFromSeparateFile = false;
	m_bNoHitRefinement = false;
	m_bDontOccludeExplosions = false;

	// Assign default material originally.
	m_pMaterial = GetMatMan()->GetDefaultMaterial();

	m_pLattice = 0;
	m_pDeformableMeshData = 0;
	m_pSpines = 0; m_nSpines = 0; m_pBoneMapping = 0;
	m_nPhysFoliages = 0;
	m_pSrcPhysMesh = 0;
	m_pLastBooleanOp = 0;
	m_pMapFaceToFace0 = 0;
	m_pClothTangentsData = 0;
	m_pSkinInfo = 0;
	m_pDelayedSkinParams = 0;
	m_arrPhysGeomInfo.m_array.clear();
}

CStatObj::~CStatObj()
{ 
	if (m_bCheckGarbage)
		GetObjManager()->CheckForGarbage(this,false);

  GetCounter()->Delete(this);

	ShutDown();

	//scalar deleting destructor //Bugs
	
	
}

void CStatObj::ShutDown() 
{
  SAFE_DELETE(m_pSubObjectsForRendering);

  if(m_pReadStream)
  {
    m_pReadStream->Wait();
    assert(m_pReadStream==0);
    m_pReadStream=0;
  }

	m_pMergedObject = 0;

//	assert (IsHeapValid());

  SAFE_DELETE(m_pIndexedMesh);
//	assert (IsHeapValid());

	for(int n=0; n<m_arrPhysGeomInfo.GetGeomCount(); n++)
  if(m_arrPhysGeomInfo[n])
	{
		m_arrPhysGeomInfo[n]->pGeom->SetForeignData(0,0);
    GetPhysicalWorld()->GetGeomManager()->UnregisterGeometry(m_arrPhysGeomInfo[n]);
	}
	m_arrPhysGeomInfo.m_array.clear();

//	assert (IsHeapValid());

//  delete m_pSMLSource;

	SetRenderMesh(0);

	if (m_pRenderMeshOcclusion)
	{
		m_pRenderMeshOcclusion = 0;
	}

//	assert (IsHeapValid());

/* // SDynTexture is not accessable for 3dengine

	for(int i=0; i<FAR_TEX_COUNT; i++)
		if(m_arrSpriteTexPtr[i])
			m_arrSpriteTexPtr[i]->ReleaseDynamicRT(true);

	for(int i=0; i<FAR_TEX_COUNT_60; i++)
		if(m_arrSpriteTexPtr_60[i])
			m_arrSpriteTexPtr_60[i]->ReleaseDynamicRT(true);
*/

	SAFE_RELEASE(m_pLattice);

	for(int i=0; i<MAX_STATOBJ_LODS_NUM; i++)
		if(m_arrpLowLODs[i])
		{
      if(m_arrpLowLODs[i]->m_pParentObject)
        GetObjManager()->UnregisterForStreaming(m_arrpLowLODs[i]->m_pParentObject);
      else
        GetObjManager()->UnregisterForStreaming(m_arrpLowLODs[i]);

      // Sub objects do not own the LODs, so they should not delete them.
			m_arrpLowLODs[i] = 0;
		}

	//////////////////////////////////////////////////////////////////////////
	// Handle sub-objects and parents.
	//////////////////////////////////////////////////////////////////////////
	for (int i=0; i<m_subObjects.size(); i++)
	{
		CStatObj *pChildObj = (CStatObj*)m_subObjects[i].pStatObj;
		if (pChildObj)
		{
			if (!m_bSharesChildren)
				pChildObj->m_pParentObject = NULL;
      GetObjManager()->UnregisterForStreaming(pChildObj);
			pChildObj->Release();
		}
	}
	m_subObjects.clear();

	if (m_pParentObject && !m_pParentObject->m_subObjects.empty())
	{
		// Remove this StatObject from sub-objects of the parent.
		SSubObject *pSubObjects = &m_pParentObject->m_subObjects[0];
		for (int i = 0,num = m_pParentObject->m_subObjects.size(); i < num; i++)
		{
			if (pSubObjects[i].pStatObj == this)
			{
				m_pParentObject->m_subObjects.erase( m_pParentObject->m_subObjects.begin()+i );
				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////

	if (m_pDeformableMeshData)
	{
		if (m_pDeformableMeshData->pInternalGeom) m_pDeformableMeshData->pInternalGeom->Release();
		if (m_pDeformableMeshData->pVtxMap) delete[] m_pDeformableMeshData->pVtxMap;
		if (m_pDeformableMeshData->pUsedVtx) delete[] m_pDeformableMeshData->pUsedVtx;
		if (m_pDeformableMeshData->pVtxTri) delete[] m_pDeformableMeshData->pVtxTri;
		if (m_pDeformableMeshData->pVtxTriBuf) delete[] m_pDeformableMeshData->pVtxTriBuf;
		if (m_pDeformableMeshData->pPrevVtx) delete[] m_pDeformableMeshData->pPrevVtx;
		if (m_pDeformableMeshData->prVtxValency) delete[] m_pDeformableMeshData->prVtxValency;
		delete m_pDeformableMeshData;
	}
	m_pDeformableMeshData = 0;

  FreeFoliageData();

	if (m_pMapFaceToFace0)
		delete[] m_pMapFaceToFace0;
	m_pMapFaceToFace0 = 0;
	if (m_pClothTangentsData && !m_pClonedSourceObject)
		delete[] m_pClothTangentsData;
	m_pClothTangentsData = 0;
	if (m_pSkinInfo && !m_pClonedSourceObject)
		delete[] m_pSkinInfo;
	m_pSkinInfo = 0;
	if (m_pDelayedSkinParams)
		delete m_pDelayedSkinParams;

	SAFE_RELEASE(m_pClonedSourceObject);

  delete [] m_pHeightmap;

	if (m_pSrcPhysMesh)
	{
		m_pSrcPhysMesh->SetForeignData(0,0);
		m_pSrcPhysMesh->Release();
	}

  GetObjManager()->UnregisterForStreaming(this);
  m_arrStatObjForRenderMeshDelete.Delete(this);
}


/*
void CStatObj::BuildOcTree()
{
  if(!m_pIndexedMesh->m_nFaceCount || m_pIndexedMesh->m_nFaceCount<=2)
    return;

  CBox parent_box( m_pIndexedMesh->m_vBoxMin, m_pIndexedMesh->m_vBoxMax );
  parent_box.max += 0.01f;
  parent_box.min +=-0.01f;

  SFace ** allFaces = new SFace *[m_pIndexedMesh->m_nFaceCount];

  for(int f=0; f<m_pIndexedMesh->m_nFaceCount; f++)
  {
    m_pIndexedMesh->m_pFaces[f].m_vCenter = 
     (Vec3(&m_pIndexedMesh->m_pVerts[m_pIndexedMesh->m_pFaces[f].v[0]].x) + 
      Vec3(&m_pIndexedMesh->m_pVerts[m_pIndexedMesh->m_pFaces[f].v[1]].x) + 
      Vec3(&m_pIndexedMesh->m_pVerts[m_pIndexedMesh->m_pFaces[f].v[2]].x))/3.f;

    allFaces[f] = &m_pIndexedMesh->m_pFaces[f];
  }

  const int max_tris_in_leaf = 2000;
  const float leaf_min_size  = stricmp(m_szGeomName,"sector_0") ? 16.f : 32.f;

  text_to_log("  Generating octree ... ");
  m_pOcTree = new octree_node( &parent_box, allFaces, IndexedMesh->m_nFaceCount, IndexedMesh, leaf_min_size, max_tris_in_leaf, 2);
  text_to_log_plus("%d leafs created", octree_node::static_current_leaf_id);
  m_pOcTree->update_bbox(IndexedMesh);

  delete [] allFaces;
}*/

//float M akeBuffersTime = 0;

void CStatObj::MakeRenderMesh()
{
	if(GetSystem()->IsDedicated())
		return;

  FUNCTION_PROFILER_3DENGINE;

	SetRenderMesh(0);
	
	if (m_pIndexedMesh && m_pIndexedMesh->GetSubSetCount() == 0)
		return;

	CMesh *pMesh = m_pIndexedMesh->GetMesh();

	m_nRenderTrisCount = 0;
	//////////////////////////////////////////////////////////////////////////
	// Initialize Mesh subset material flags.
	//////////////////////////////////////////////////////////////////////////
	for (int i = 0; i < pMesh->GetSubSetCount(); i++)
	{
		SMeshSubset &subset = pMesh->m_subsets[i];
		if (!(subset.nMatFlags&MTL_FLAG_NODRAW))
		{
			m_nRenderTrisCount += subset.nNumIndices/3;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	if (!m_nRenderTrisCount)
		return;

	m_pRenderMesh = GetRenderer()->CreateRenderMesh("StatObj_Dynamic", GetFilePath());
	m_pRenderMesh->KeepSysMesh((m_nFlags & STATIC_OBJECT_DYNAMIC)!=0);
	m_pRenderMesh->SetMaterial( m_pMaterial );
	SMeshBoneMapping *pBoneMap = pMesh->m_pBoneMapping;
	pMesh->m_pBoneMapping = 0;
	m_pRenderMesh->SetMesh( *pMesh );
	pMesh->m_pBoneMapping = pBoneMap;
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::SetMaterial( IMaterial *pMaterial )
{
	m_pMaterial = pMaterial;
	if (m_pRenderMesh)
		m_pRenderMesh->SetMaterial( m_pMaterial );
}

///////////////////////////////////////////////////////////////////////////////////////    
Vec3 CStatObj::GetHelperPos(const char * szHelperName)
{
	SSubObject* pSubObj = FindSubObject(szHelperName);
	if (!pSubObj)
		return Vec3(0,0,0);

	return Vec3(pSubObj->tm.m03, pSubObj->tm.m13, pSubObj->tm.m23);
}

///////////////////////////////////////////////////////////////////////////////////////   
const Matrix34& CStatObj::GetHelperTM(const char * szHelperName)
{
  SSubObject* pSubObj = FindSubObject(szHelperName);  
  
  if (!pSubObj)
  {    
    static Matrix34 identity(IDENTITY);
    return identity;
  }

  return pSubObj->tm;
}



/*
bool CStatObj::GetHelper(int id, char * szHelperName, int nMaxHelperNameSize, Vec3 * pPos, Vec3 * pRot)
{
  if(id<0 || id>=IndexedMesh->m_Helpers.Count())
    return false;

  strncpy(szHelperName, IndexedMesh->m_Helpers[id].name, nMaxHelperNameSize);
  *pPos = IndexedMesh->m_Helpers[id].pos;
  *pRot = IndexedMesh->m_Helpers[id].rot;
  return true;
} */

/*
const char * CStatObj::GetPhysMaterialName(int nMatID)
{
  if(m_pRenderMesh && m_pRenderMesh->GetChunks() && nMatID < m_pRenderMesh->GetChunks()->Count())
    return m_pRenderMesh->GetChunks()->Get(nMatID)->szPhysMat;

  return 0;
}

bool CStatObj::SetPhysMaterialName(int nMatID, const char * szPhysMatName)
{
  if(m_pRenderMesh && m_pRenderMesh->GetChunks() && nMatID < m_pRenderMesh->GetChunks()->Count())
  {
    strncpy(m_pRenderMesh->GetChunks()->Get(nMatID)->szPhysMat, szPhysMatName, sizeof(m_pRenderMesh->GetChunks()->Get(nMatID)->szPhysMat));
    return true;
  }

  return false;
}
*/
int CStatObj::Release()
{
	m_nUsers--;
	if (m_nUsers <= 1 && m_pParentObject && !m_pParentObject->m_bCheckGarbage && m_pParentObject->m_nUsers <= 0)
	{
		m_pParentObject->m_bCheckGarbage = true;
		GetObjManager()->CheckForGarbage( m_pParentObject,true );
	}
	int res = m_nUsers;
	if(m_nUsers<=0)
	{
		GetObjManager()->InternalDeleteObject(this);
	}
	//return res; //Diesel cut
	return false; //Diesel new
}

bool CStatObj::IsSameObject(const char * szFileName, const char * szGeomName)
{
	// cmp object names
	if (szGeomName)
	{
		// [Anton] - always use new cgf for objects used for cloth simulation
		//if (*szGeomName && stricmp(szGeomName,"cloth") == 0)
		//	return false;

		if(stricmp(szGeomName,m_szGeomName)!=0)
			return false;
	}

  // Normilize file name
	char szFileNameNorm[MAX_PATH_LENGTH]="";
	char *pszDest = szFileNameNorm;
	const char *pszSource = szFileName;
	while (*pszSource)
	{
		if (*pszSource=='\\')
			*pszDest++='/';
		else 
			*pszDest++=*pszSource;
		pszSource++;
	}
	*pszDest=0;

	// cmp file names
	if(stricmp(szFileNameNorm,m_szFileName)!=0)
		return false;

	return true;
}

void CStatObj::GetMemoryUsage(ICrySizer* pSizer)
{
  int nSize = sizeof(CStatObj);

  nSize += m_subObjects.size() * sizeof(SSubObject);

  nSize += m_szFileName.capacity();
  nSize += m_szGeomName.capacity();
  nSize += m_szProperties.capacity();

	pSizer->AddObject(this,nSize);

	for (int i = 0; i < m_subObjects.size(); i++)
	{
		if (m_subObjects[i].pStatObj)
			m_subObjects[i].pStatObj->GetMemoryUsage(pSizer);
	}

	for(int i=1; i<MAX_STATOBJ_LODS_NUM; i++)
  {
		if(m_arrpLowLODs[i])
			m_arrpLowLODs[i]->GetMemoryUsage(pSizer);
  }

	if(m_pIndexedMesh)
	{
		SIZER_COMPONENT_NAME(pSizer, "Mesh");
		pSizer->AddObject(m_pIndexedMesh->GetMesh(), m_pIndexedMesh->GetMemoryUsage());
	}

	if (m_pMergedObject)
		m_pMergedObject->GetMemoryUsage(pSizer);

	int nVtx=0,nIdx=0;
	if (m_pIndexedMesh)
		nVtx=m_pIndexedMesh->GetVertexCount(), m_pIndexedMesh->GetIndexCount();
	else if (m_pRenderMesh)
		nVtx=m_pRenderMesh->GetVerticesCount(), m_pRenderMesh->GetIndicesCount();

	if (m_pSpines)
	{
		SIZER_COMPONENT_NAME(pSizer, "StatObj Foliage Data");
		pSizer->AddObject(m_pSpines, sizeof(m_pSpines[0]), m_nSpines);
		if (m_pBoneMapping)
			pSizer->AddObject(m_pBoneMapping, sizeof(m_pBoneMapping[0]), nVtx);
		for(int i=0; i<m_nSpines; i++)
			pSizer->AddObject(m_pSpines[i].pVtx, sizeof(Vec3)*2+sizeof(Vec4), m_pSpines[i].nVtx);
	}

	if (m_pSkinInfo && (!m_pClonedSourceObject || m_pClonedSourceObject==this))
	{
		SIZER_COMPONENT_NAME(pSizer, "Deformable StatObj SkinData");
		pSizer->AddObject(m_pSkinInfo, (nVtx+1)*sizeof(m_pSkinInfo[0]));
	}

	if (m_pDeformableMeshData)
		pSizer->AddObject(m_pDeformableMeshData, sizeof(SDeformableMeshData) + 
											(sizeof(int)*2+sizeof(float)+sizeof(Vec3))*nVtx + (nVtx>>5) + sizeof(int)*nIdx);
	if (m_pMapFaceToFace0)
		pSizer->AddObject(m_pMapFaceToFace0, sizeof(m_pMapFaceToFace0[0])*max(m_nLoadedTrisCount,m_nRenderTrisCount));
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::SetLodObject( int nLod,CStatObj *pLod )
{
	assert( nLod > 0 && nLod < MAX_STATOBJ_LODS_NUM );
	if (nLod <= 0 || nLod >= MAX_STATOBJ_LODS_NUM)
		return;

	if (strstr(m_szProperties,"lowspeclod0") != 0)
		m_bLowSpecLod0Set = true;

	if (pLod)
	{
		// Check if low lod decrease ammount of used materials.
		int nPrevLodMatIds = m_nRenderMatIds;
		int nPrevLodTris = m_nRenderTrisCount;
		if (nLod > 1 && m_arrpLowLODs[nLod-1])
		{
			nPrevLodMatIds = m_arrpLowLODs[nLod-1]->m_nRenderMatIds;
			nPrevLodTris = m_arrpLowLODs[nLod-1]->m_nRenderTrisCount;
		}

		if(GetCVars()->e_LodsForceUse)
		{
			if((int)m_nMaxUsableLod<nLod)
        m_nMaxUsableLod = nLod;
		}
		else
		{
			int min_tris = GetCVars()->e_LodMinTtris;
			if (((pLod->m_nRenderTrisCount >= min_tris || nPrevLodTris >= (3*min_tris)/2)
				|| pLod->m_nRenderMatIds < nPrevLodMatIds) && nLod > (int)m_nMaxUsableLod)
			{
				m_nMaxUsableLod = nLod;
			}
		}

		pLod->m_pLod0 = this;
		pLod->m_pMaterial = m_pMaterial; // Lod must use same material as parent.
		if ((m_nRenderTrisCount && pLod != 0) && (pLod->m_nRenderTrisCount > m_nRenderTrisCount / TRIS_IN_LOD_WARNING_RAIO) &&
			(m_nRenderMatIds == pLod->m_nRenderMatIds))
		{
			FileWarning(0, GetFilePath(),	"LOD%d of geometry %s contains too many polygons comparing to the LOD0 [%d against %d]",nLod,m_szGeomName.c_str(),
				pLod->m_nRenderTrisCount, m_nRenderTrisCount);
		}
		if (pLod->m_nRenderTrisCount > MAX_TRIS_IN_LOD_0)
		{
			if (strstr(pLod->GetProperties(),"lowspeclod0") != 0 && !m_bLowSpecLod0Set)
			{
				m_bLowSpecLod0Set = true;
				m_nMinUsableLod0 = nLod;
			}
			if (!m_bLowSpecLod0Set)
				m_nMinUsableLod0 = nLod;
		}
		if (nLod+1 > (int)m_nLoadedLodsNum)
			m_nLoadedLodsNum = nLod+1;

		/*  Should depend from prev LOD
		if(m_arrpLowLODs[nLodLevel] != 0 && m_arrpLowLODs[nLodLevel]->m_nRenderTrisCount < MAX_TRIS_IN_LOD && m_arrpLowLODs[nLodLevel]->m_nRenderTrisCount != 0)
		{
		FileWarning(0, sLodFileName,	"Low LOD geometry %s contains too few polygons [%d], there is no sense to use it. Minimal recommended value is %d",
		sLodFileName,m_arrpLowLODs[nLodLevel]->m_nRenderTrisCount, MAX_TRIS_IN_LOD);
		}
		*/

		// When assigning lod to child object.
		if (m_pParentObject)
		{
			if (nLod+1 > (int)m_pParentObject->m_nLoadedLodsNum)
				m_pParentObject->m_nLoadedLodsNum = nLod+1;
		}
	}
	m_arrpLowLODs[nLod] = pLod;
}

//////////////////////////////////////////////////////////////////////////
IStatObj * CStatObj::GetLodObject(int nLodLevel,bool bReturnNearest)
{
	if(nLodLevel<1)
  {
    if (!bReturnNearest || m_pRenderMesh != 0)
      return this;
		
		nLodLevel = 1;
  }

	CStatObj *pLod = 0;
	if(nLodLevel < MAX_STATOBJ_LODS_NUM)
	{
		pLod = m_arrpLowLODs[nLodLevel];
		
		// Look up
		if (bReturnNearest && !pLod)
		{
			// Find highest existing lod looking up.
			int lod = nLodLevel;
			while (lod > 0 && m_arrpLowLODs[lod] == 0) lod--;
			if (lod > 0)
				pLod = m_arrpLowLODs[lod];
			else if (m_pRenderMesh)
			{
				pLod = this;
			}
		}
		// Look down
		if (bReturnNearest && !pLod)
		{
			// Find highest existing lod looking down.
			for (int lod = nLodLevel+1; lod < MAX_STATOBJ_LODS_NUM; lod++)
			{
				if (m_arrpLowLODs[lod])
				{
					pLod = m_arrpLowLODs[lod];
					break;
				}
			}
		}
	}

	return pLod;
}

bool CStatObj::IsPhysicsExist()
{
	return m_arrPhysGeomInfo.GetGeomCount()>0;
}

bool CStatObj::IsSphereOverlap(const Sphere& sSphere)
{
	if(m_pRenderMesh && Overlap::Sphere_AABB(sSphere,AABB(m_vBoxMin,m_vBoxMax)))
	{ // if inside bbox
		int nPosStride=0;
    int nInds = m_pRenderMesh->GetIndicesCount();
    uint16 *pInds = m_pRenderMesh->GetIndexPtr(FSL_READ);
		const byte * pPos = m_pRenderMesh->GetPosPtr(nPosStride, FSL_READ);
		
		if(pInds && pPos)
		for(int i=0; (i+2)<nInds; i+=3)
		{	// test all triangles of water surface strip
			Vec3 v0 = (*(Vec3*)&pPos[nPosStride*pInds[i+0]]);
			Vec3 v1 = (*(Vec3*)&pPos[nPosStride*pInds[i+1]]);
			Vec3 v2 = (*(Vec3*)&pPos[nPosStride*pInds[i+2]]);
			Vec3 vBoxMin = v0; vBoxMin.CheckMin(v1); vBoxMin.CheckMin(v2);
			Vec3 vBoxMax = v0; vBoxMax.CheckMax(v1); vBoxMax.CheckMax(v2);

			if(	Overlap::Sphere_AABB(sSphere,AABB(vBoxMin,vBoxMax)))
				return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::Invalidate( bool bPhysics, float tolerance )
{
	if (m_pIndexedMesh)
	{
		m_pIndexedMesh->Invalidate();

		m_vBoxMin = m_pIndexedMesh->m_bbox.min;
		m_vBoxMax = m_pIndexedMesh->m_bbox.max;

		MakeRenderMesh();
		m_pIndexedMesh->m_bInvalidated = false;
		m_nLoadedVertexCount = m_pIndexedMesh->GetVertexCount();
		m_nLoadedTrisCount = m_pIndexedMesh->GetFacesCount();
		if (!m_nLoadedTrisCount)
		{
			m_nLoadedTrisCount = m_pIndexedMesh->GetIndexCount()/3;
		}
		CalcRadiuses();

		if (bPhysics)
		{
			PhysicalizeGeomType( PHYS_GEOM_TYPE_DEFAULT,*m_pIndexedMesh->GetMesh(), tolerance, 0 );
		}

		ReleaseIndexedMesh(true);
	}

	// if this is compound multi-sub object.
	if (m_bMerged)
		UnMergeSubObjectsRenderMeshes();

	m_nSubObjectMeshCount = 0;
	for(int i=0; i<m_subObjects.size(); i++)
		m_nSubObjectMeshCount += m_subObjects[i].pStatObj && m_subObjects[i].nType==STATIC_SUB_OBJECT_MESH && !m_subObjects[i].bHidden;
}

//////////////////////////////////////////////////////////////////////////
IStatObj* CStatObj::Clone(bool bCloneChildren, bool bDynamic)
{
	if (m_bDefaultObject)
		return this;

	CStatObj *pNewObj = new CStatObj;
	pNewObj->m_bForInternalUse = m_bForInternalUse;

	pNewObj->m_pClonedSourceObject = m_pClonedSourceObject ? m_pClonedSourceObject:this;
	pNewObj->m_pClonedSourceObject->AddRef(); // Cloned object will keep a reference to this.

	pNewObj->m_nLoadedTrisCount = m_nLoadedTrisCount;
	pNewObj->m_nLoadedVertexCount = m_nLoadedVertexCount;
	pNewObj->m_nRenderTrisCount = m_nRenderTrisCount;

	if (m_pIndexedMesh && !m_bMerged)
	{
		pNewObj->m_pIndexedMesh = new CIndexedMesh;
		pNewObj->m_pIndexedMesh->Copy( *m_pIndexedMesh->GetMesh() );
	}

	if (m_pRenderMesh && !m_bMerged) 
	{
		IRenderMesh *pRenderMesh = GetRenderer()->CreateRenderMesh("StatObj_Cloned", pNewObj->GetFilePath());
		m_pRenderMesh->CopyTo(pRenderMesh, 0, bDynamic);
		if (m_nFlags & STATIC_OBJECT_DYNAMIC)
			pRenderMesh->KeepSysMesh(true);
		pNewObj->SetRenderMesh(pRenderMesh);
	}

	//////////////////////////////////////////////////////////////////////////
	pNewObj->m_pRenderMeshOcclusion = m_pRenderMeshOcclusion;
	//////////////////////////////////////////////////////////////////////////

	//pNewObj->->m_szFolderName = m_szFolderName;
	//pNewObj->->m_szFileName = m_szFileName;
	//pNewObj->->m_szGeomName = m_szGeomName;

	// Default material.
	pNewObj->m_pMaterial = m_pMaterial;
	pNewObj->m_nMaterialLayersMask = m_nMaterialLayersMask;

	//*pNewObj->m_arrSpriteTexID = *m_arrSpriteTexID;

	int i;
	for (i = 0; i < m_arrPhysGeomInfo.GetGeomCount(); i++)
	{
		pNewObj->m_arrPhysGeomInfo.SetPhysGeom(m_arrPhysGeomInfo[i], i, m_arrPhysGeomInfo.GetGeomType(i));
		if (pNewObj->m_arrPhysGeomInfo[i])
			GetPhysicalWorld()->GetGeomManager()->AddRefGeometry( pNewObj->m_arrPhysGeomInfo[i] );
	}
	pNewObj->m_vBoxMin = m_vBoxMin;
	pNewObj->m_vBoxMax = m_vBoxMax;
	pNewObj->m_vVegCenter = m_vVegCenter;

	pNewObj->m_fObjectRadius = m_fObjectRadius;
	pNewObj->m_fRadiusHors = m_fRadiusHors;
	pNewObj->m_fRadiusVert = m_fRadiusVert;

	// Clone Lods?
	/*
	for (i = 0; i < MAX_STATOBJ_LODS_NUM; i++)
	{
		if (m_arrpLowLODs[i])
			pNewObj->m_arrpLowLODs[i] = m_arrpLowLODs[i]->Clone();
	}
	pNewObj->m_nLoadedLodsNum = m_nLoadedLodsNum;
	*/

	pNewObj->m_nFlags = m_nFlags | STATIC_OBJECT_CLONE;

	//////////////////////////////////////////////////////////////////////////
	// Internal Flags.
	//////////////////////////////////////////////////////////////////////////
	pNewObj->m_bUseStreaming = false;
	pNewObj->m_bDefaultObject = m_bDefaultObject;
	pNewObj->m_bOpenEdgesTested = m_bOpenEdgesTested;
	pNewObj->m_bSubObject = false; // [anton] since parent is not copied anyway
	pNewObj->m_bVehicleOnlyPhysics  = m_bVehicleOnlyPhysics;
	pNewObj->m_idmatBreakable  = m_idmatBreakable;
	pNewObj->m_bBreakableByGame = m_bBreakableByGame;
	pNewObj->m_bHasDeformationMorphs = m_bHasDeformationMorphs;
	pNewObj->m_bTmpIndexedMesh = m_bTmpIndexedMesh;
	pNewObj->m_bHaveOcclusionProxy = m_bHaveOcclusionProxy;
	//////////////////////////////////////////////////////////////////////////

	int nNumSubObj = (int)m_subObjects.size();
	pNewObj->m_subObjects.resize( nNumSubObj );
	for (i = 0; i < nNumSubObj; i++)
	{
		pNewObj->m_subObjects[i] = m_subObjects[i];
		if (m_subObjects[i].pStatObj)
		{
			if (bCloneChildren)
			{
				pNewObj->m_subObjects[i].pStatObj = m_subObjects[i].pStatObj->Clone();
				pNewObj->m_subObjects[i].pStatObj->AddRef();
				((CStatObj*)(pNewObj->m_subObjects[i].pStatObj))->m_pParentObject = this;
			}
			else
      {
        m_subObjects[i].pStatObj->AddRef();
        ((CStatObj*)m_subObjects[i].pStatObj)->m_nFlags |= STATIC_OBJECT_MULTIPLE_PARENTS;
      }
		}
	}
	pNewObj->m_nSubObjectMeshCount = m_nSubObjectMeshCount;
	if (!bCloneChildren)
		pNewObj->m_bSharesChildren = true;

	if (m_pDeformableMeshData && pNewObj->GetIndexedMesh(true))
	{
		int nVtx;
		CMesh *pMesh = pNewObj->GetIndexedMesh()->GetMesh();
		pNewObj->m_pDeformableMeshData = new SDeformableMeshData;
		nVtx = pMesh->m_numVertices;
		pMesh->ReallocStream(CMesh::POSITIONS, (nVtx-1&~31)+32);
		pMesh->ReallocStream(CMesh::NORMALS, (nVtx-1&~31)+32);
		pMesh->ReallocStream(CMesh::TEXCOORDS, (nVtx-1&~31)+32);
		pMesh->ReallocStream(CMesh::TANGENTS, (nVtx-1&~31)+32);
		pNewObj->m_pDeformableMeshData->pPrevVtx = new Vec3[pMesh->m_numVertices];
		memcpy(pNewObj->m_pDeformableMeshData->pVtxMap=new int[pMesh->m_numVertices], 
			m_pDeformableMeshData->pVtxMap, pMesh->m_numVertices*sizeof(int));
		memcpy(pNewObj->m_pDeformableMeshData->pUsedVtx = new unsigned int[pMesh->m_numVertices>>5], 
			m_pDeformableMeshData->pUsedVtx, (pMesh->m_numVertices>>5)*sizeof(int));
		memcpy(pNewObj->m_pDeformableMeshData->pVtxTri = new int[pMesh->m_numVertices+1], 
			m_pDeformableMeshData->pVtxTri, (pMesh->m_numVertices+1)*sizeof(int));
		memcpy(pNewObj->m_pDeformableMeshData->pVtxTriBuf = new int[pMesh->m_nIndexCount],
			m_pDeformableMeshData->pVtxTriBuf, pMesh->m_nIndexCount*sizeof(int));
		memcpy(pNewObj->m_pDeformableMeshData->prVtxValency = new float[pMesh->m_numVertices],
			m_pDeformableMeshData->prVtxValency, pMesh->m_numVertices*sizeof(float));
		pNewObj->m_pDeformableMeshData->kViscosity = m_pDeformableMeshData->kViscosity;
		if (pNewObj->m_pDeformableMeshData->pInternalGeom = m_pDeformableMeshData->pInternalGeom)
			pNewObj->m_pDeformableMeshData->pInternalGeom->AddRef();
		pMesh->m_numVertices = pMesh->m_nCoorCount = 
		pMesh->m_streamSize[CMesh::POSITIONS] = pMesh->m_streamSize[CMesh::NORMALS]=
		pMesh->m_streamSize[CMesh::TEXCOORDS] = pMesh->m_streamSize[CMesh::TANGENTS] = nVtx;
	}

	pNewObj->m_pClothTangentsData = m_pClothTangentsData;
	pNewObj->m_pSkinInfo = m_pSkinInfo;

	return pNewObj;
}

//////////////////////////////////////////////////////////////////////////
IStatObj* CStatObj::Clone( bool bCloneGeometry,bool bCloneChildren,bool bMeshesOnly )
{
	if (m_bDefaultObject)
		return this;

	CStatObj *pNewObj = new CStatObj;
	pNewObj->m_bForInternalUse = m_bForInternalUse;

	pNewObj->m_pClonedSourceObject = m_pClonedSourceObject ? m_pClonedSourceObject:this;
	pNewObj->m_pClonedSourceObject->AddRef(); // Cloned object will keep a reference to this.

	pNewObj->m_nLoadedTrisCount = m_nLoadedTrisCount;
	pNewObj->m_nLoadedVertexCount = m_nLoadedVertexCount;
	pNewObj->m_nRenderTrisCount = m_nRenderTrisCount;

	if (bCloneGeometry)
	{
		if (m_pIndexedMesh && !m_bMerged)
		{
			pNewObj->m_pIndexedMesh = new CIndexedMesh;
			pNewObj->m_pIndexedMesh->Copy( *m_pIndexedMesh->GetMesh() );
		}
		if (m_pRenderMesh && !m_bMerged) 
		{
			IRenderMesh *pRenderMesh = GetRenderer()->CreateRenderMesh("StatObj_Cloned", pNewObj->GetFilePath());
			m_pRenderMesh->CopyTo(pRenderMesh);
			pNewObj->SetRenderMesh(pRenderMesh);
		}
	}
	else
	{
		if (m_pRenderMesh && !m_bMerged)
		{
			pNewObj->SetRenderMesh(m_pRenderMesh);
			m_pRenderMesh->AddRef();
		}
	}

	pNewObj->m_pRenderMeshOcclusion = m_pRenderMeshOcclusion;

	pNewObj->m_szFileName = m_szFileName;
	pNewObj->m_szGeomName = m_szGeomName;

	// Default material.
	pNewObj->m_pMaterial = m_pMaterial;

	//*pNewObj->m_arrSpriteTexID = *m_arrSpriteTexID;

	pNewObj->m_fObjectRadius = m_fObjectRadius;

	int i;
	for (i = 0; i < m_arrPhysGeomInfo.GetGeomCount(); i++)
	{
		pNewObj->m_arrPhysGeomInfo.SetPhysGeom(m_arrPhysGeomInfo[i], i, m_arrPhysGeomInfo.GetGeomType(i));
		if (pNewObj->m_arrPhysGeomInfo[i])
			GetPhysicalWorld()->GetGeomManager()->AddRefGeometry( pNewObj->m_arrPhysGeomInfo[i] );
	}
	pNewObj->m_vBoxMin = m_vBoxMin;
	pNewObj->m_vBoxMax = m_vBoxMax;
	pNewObj->m_vVegCenter = m_vVegCenter;

	pNewObj->m_fRadiusHors = m_fRadiusHors;
	pNewObj->m_fRadiusVert = m_fRadiusVert;

	// Clone Lods?
	/*
	for (i = 0; i < MAX_STATOBJ_LODS_NUM; i++)
	{
	if (m_arrpLowLODs[i])
	pNewObj->m_arrpLowLODs[i] = m_arrpLowLODs[i]->Clone();
	}
	pNewObj->m_nLoadedLodsNum = m_nLoadedLodsNum;
	*/

	pNewObj->m_nFlags = m_nFlags | STATIC_OBJECT_CLONE;

	//////////////////////////////////////////////////////////////////////////
	// Internal Flags.
	//////////////////////////////////////////////////////////////////////////
	pNewObj->m_bUseStreaming = false;
	pNewObj->m_bDefaultObject = m_bDefaultObject;
	pNewObj->m_bOpenEdgesTested = m_bOpenEdgesTested;
	pNewObj->m_bSubObject = false; // [anton] since parent is not copied anyway
	pNewObj->m_bVehicleOnlyPhysics = m_bVehicleOnlyPhysics;
	pNewObj->m_idmatBreakable = m_idmatBreakable;
	pNewObj->m_bBreakableByGame = m_bBreakableByGame;
	pNewObj->m_bHasDeformationMorphs = m_bHasDeformationMorphs;
	pNewObj->m_bTmpIndexedMesh = m_bTmpIndexedMesh;
	pNewObj->m_bHaveOcclusionProxy = m_bHaveOcclusionProxy;
	//////////////////////////////////////////////////////////////////////////

	int numSubObj = (int)m_subObjects.size();
	if (bMeshesOnly) 
	{
		numSubObj = 0;
		for(i=0;i<m_subObjects.size();i++)
		{
			if (m_subObjects[i].nType==STATIC_SUB_OBJECT_MESH)
				numSubObj++;
			else
				break;
		}
	}
	pNewObj->m_subObjects.resize(numSubObj);
	for (i=0; i < numSubObj; i++)
	{
		pNewObj->m_subObjects[i] = m_subObjects[i];
		if (m_subObjects[i].pStatObj)
		{
			if (bCloneChildren)
			{
				pNewObj->m_subObjects[i].pStatObj = m_subObjects[i].pStatObj->Clone();
				pNewObj->m_subObjects[i].pStatObj->AddRef();
				((CStatObj*)(pNewObj->m_subObjects[i].pStatObj))->m_pParentObject = this;
			}
			else
      {
        m_subObjects[i].pStatObj->AddRef();
        ((CStatObj*)(m_subObjects[i].pStatObj))->m_nFlags |= STATIC_OBJECT_MULTIPLE_PARENTS;
      }
		}
	}
	pNewObj->m_nSubObjectMeshCount = m_nSubObjectMeshCount;
	if (!bCloneChildren)
		pNewObj->m_bSharesChildren = true;

	if (bCloneGeometry && m_pDeformableMeshData && pNewObj->GetIndexedMesh(true))
	{
		int nVtx;
		CMesh *pMesh = pNewObj->GetIndexedMesh()->GetMesh();
		pNewObj->m_pDeformableMeshData = new SDeformableMeshData;
		nVtx = pMesh->m_numVertices;
		pMesh->ReallocStream(CMesh::POSITIONS, (nVtx-1&~31)+32);
		pMesh->ReallocStream(CMesh::NORMALS, (nVtx-1&~31)+32);
		pMesh->ReallocStream(CMesh::TEXCOORDS, (nVtx-1&~31)+32);
		pMesh->ReallocStream(CMesh::TANGENTS, (nVtx-1&~31)+32);
		pNewObj->m_pDeformableMeshData->pPrevVtx = new Vec3[pMesh->m_numVertices];
		memcpy(pNewObj->m_pDeformableMeshData->pVtxMap=new int[pMesh->m_numVertices], 
			m_pDeformableMeshData->pVtxMap, pMesh->m_numVertices*sizeof(int));
		memcpy(pNewObj->m_pDeformableMeshData->pUsedVtx = new unsigned int[pMesh->m_numVertices>>5], 
			m_pDeformableMeshData->pUsedVtx, (pMesh->m_numVertices>>5)*sizeof(int));
		memcpy(pNewObj->m_pDeformableMeshData->pVtxTri = new int[pMesh->m_numVertices+1], 
			m_pDeformableMeshData->pVtxTri, (pMesh->m_numVertices+1)*sizeof(int));
		memcpy(pNewObj->m_pDeformableMeshData->pVtxTriBuf = new int[pMesh->m_nIndexCount],
			m_pDeformableMeshData->pVtxTriBuf, pMesh->m_nIndexCount*sizeof(int));
		memcpy(pNewObj->m_pDeformableMeshData->prVtxValency = new float[pMesh->m_numVertices],
			m_pDeformableMeshData->prVtxValency, pMesh->m_numVertices*sizeof(float));
		pNewObj->m_pDeformableMeshData->kViscosity = m_pDeformableMeshData->kViscosity;
		if (pNewObj->m_pDeformableMeshData->pInternalGeom = m_pDeformableMeshData->pInternalGeom)
			pNewObj->m_pDeformableMeshData->pInternalGeom->AddRef();
		pMesh->m_numVertices = pMesh->m_nCoorCount = 
			pMesh->m_streamSize[CMesh::POSITIONS] = pMesh->m_streamSize[CMesh::NORMALS]=
			pMesh->m_streamSize[CMesh::TEXCOORDS] = pMesh->m_streamSize[CMesh::TANGENTS] = nVtx;
	}

	return pNewObj;
}
//////////////////////////////////////////////////////////////////////////

IIndexedMesh *CStatObj::GetIndexedMesh(bool bCreateIfNone)
{ 
	if (m_pIndexedMesh)
		return m_pIndexedMesh;
	else if (m_pRenderMesh && bCreateIfNone)
	{
		
		if( m_pRenderMesh->GetIndexedMesh( m_pIndexedMesh = new CIndexedMesh ) == NULL )
		{
			// GetIndexMesh will free the IndexedMesh object if an allocation failed
			m_pIndexedMesh = NULL;
			return NULL;
		}
		m_pRenderMesh->UnKeepSysMesh();
		CMesh *pMesh = m_pIndexedMesh->GetMesh();
		if (!pMesh || pMesh->m_subsets.size()<=0)
		{
			m_pIndexedMesh->Release(); m_pIndexedMesh=0; 
			return 0;
		}
		m_bTmpIndexedMesh = true;

		int i,j,i0=pMesh->m_subsets[0].nFirstVertId+pMesh->m_subsets[0].nNumVerts;
		for(i=j=1;i<pMesh->m_subsets.size();i++) if (pMesh->m_subsets[i].nFirstVertId-i0<pMesh->m_subsets[j].nFirstVertId-i0)
			j = i;
		if (j<pMesh->m_subsets.size() && pMesh->m_subsets[0].nPhysicalizeType==PHYS_GEOM_TYPE_DEFAULT && 
				pMesh->m_subsets[j].nPhysicalizeType!=PHYS_GEOM_TYPE_DEFAULT && pMesh->m_subsets[j].nFirstVertId>i0)
		{
			pMesh->m_subsets[j].nNumVerts += pMesh->m_subsets[j].nFirstVertId-i0;
			pMesh->m_subsets[j].nFirstVertId = i0;
		}
		return m_pIndexedMesh;
	}
	return 0;
}

void CStatObj::ReleaseIndexedMesh(bool bRenderMeshUpdated)
{ 
	if (m_bTmpIndexedMesh && m_pIndexedMesh) 
	{ 
		int istart,iend;
		CMesh *pMesh = m_pIndexedMesh->GetMesh();
		if (m_pRenderMesh && !bRenderMeshUpdated)
		{
			PodArray<CRenderChunk>& Chunks = m_pRenderMesh->GetChunks();
			for(int i=0;i<pMesh->m_subsets.size();i++)
				Chunks[i].m_nMatFlags |= pMesh->m_subsets[i].nMatFlags & 1<<30;
		}
		if (bRenderMeshUpdated && m_pBoneMapping)
		{
			for(int i=iend=0;i<pMesh->m_subsets.size();i++)
				if ((pMesh->m_subsets[i].nMatFlags & (MTL_FLAG_NOPHYSICALIZE|MTL_FLAG_NODRAW)) == MTL_FLAG_NOPHYSICALIZE)	
				{
					for(istart=iend++; iend<m_chunkBoneIds.size() && m_chunkBoneIds[iend]; iend++);	
					if (!pMesh->m_subsets[i].nNumIndices) 
					{
						m_chunkBoneIds.erase(m_chunkBoneIds.begin()+istart,m_chunkBoneIds.begin()+iend);
						iend = istart;
					}
				}
		}
		m_pIndexedMesh->Release(); m_pIndexedMesh=0; 
		m_bTmpIndexedMesh = false; 
	}
}

//////////////////////////////////////////////////////////////////////////
bool CStatObj::MergeSubObjectsRenderMeshes()
{
  if (m_bUnmergable || m_nSubObjectMeshCount <= 1)
    return false;

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Merged StatObj" );

  FUNCTION_PROFILER_3DENGINE;

  LOADING_TIME_PROFILE_SECTION(GetSystem());

	m_bMerged = false;
	m_pMergedObject = 0;

	for (uint32 i = 0; i < m_nLoadedLodsNum; i++)
	{
		if (!MergeSubObjectsRenderMeshes(i))
			break;
	}
	if (!m_bMerged)
		m_bUnmergable = true;

	return m_bMerged;
}

//////////////////////////////////////////////////////////////////////////
bool CStatObj::MergeSubObjectsRenderMeshes( int nLod )
{
	if(GetSystem()->IsDedicated())
		return false;
	
	int nSubObjCount = (int)m_subObjects.size();

	for(int s=0; s < nSubObjCount; s++)
	{
		CStatObj::SSubObject *pSubObj = &m_subObjects[s];
		if (pSubObj->pStatObj && pSubObj->nType == STATIC_SUB_OBJECT_MESH && !pSubObj->bHidden)
		{
			CStatObj *pStatObj = (CStatObj*)pSubObj->pStatObj;
			if (pStatObj->m_pMaterial != m_pMaterial) // All materials must be same.
			{
				return false;
			}
		}
	}

	PodArray<SRenderMeshInfoOutput> resultMeshes;
	PodArray<SRenderMeshInfoInput> lstRMI;

	SRenderMeshInfoInput rmi;

	rmi.pMat = m_pMaterial;
	rmi.mat.SetIdentity();
	rmi.pMesh = 0;
	rmi.pSrcRndNode = 0;
	int nTotalVertexCount = 0;
	int nTotalTriCount = 0; 
  int nTotalRenderChunksCount = 0; 
	
	for(int s=0; s < nSubObjCount; s++)
	{
		CStatObj::SSubObject *pSubObj = &m_subObjects[s];
		if (pSubObj->pStatObj && pSubObj->nType == STATIC_SUB_OBJECT_MESH && !pSubObj->bHidden)
		{
			CStatObj *pStatObj = (CStatObj*)pSubObj->pStatObj->GetLodObject(nLod,true); // Get lod, if not exist get lowest existing.
			if (pStatObj && pStatObj->m_pRenderMesh)
			{
				rmi.pMesh = pStatObj->m_pRenderMesh;
				rmi.mat = pSubObj->tm;
				lstRMI.Add(rmi);
				nTotalVertexCount += rmi.pMesh->GetVerticesCount();

        int nTris = 0;
        nTotalRenderChunksCount += rmi.pMesh->GetRenderChunksCount(rmi.pMat, nTris);
        nTotalTriCount += nTris;
			}
		}
	}
	if (nTotalVertexCount > MAX_VERTICES_MERGABLE || lstRMI.size() <= 1 || nTotalRenderChunksCount<=1)
	{
		return false;
	}
  if(!nLod && (nTotalTriCount/nTotalRenderChunksCount) > GetCVars()->e_StatObjMergeMaxTrisPerDrawCall)
  {
    return false; // tris to draw calls ratio is already not so bad
  }
  // Check if merged LOD is almost the same poly count as previous lod
	if (nLod > 0 && m_pMergedObject)
	{
		CStatObj *pPrevMergedLod = (CStatObj*)m_pMergedObject->GetLodObject(nLod-1);
		if (pPrevMergedLod)
		{
			// Get LOD0 triangle count.
			int nLod0TriCount = pPrevMergedLod->m_nRenderTrisCount;
			if (nTotalTriCount*MIN_TRIS_IN_MERGED_LOD_RAIO > nLod0TriCount)
			{
				return false;
			}
		}
	}

	if (!lstRMI.empty())
	{
		resultMeshes.clear();

		CRenderMeshMerger::SMergeInfo info;
		info.sMeshName = GetFilePath();
		info.sMeshType = "StatObj_Merged";
		info.bMergeToOneRenderMesh = true;
		info.pUseMaterial = m_pMaterial;
		GetSharedRenderMeshMerger()->MergeRenderMeshes( &lstRMI[0],lstRMI.size(),resultMeshes, info);
	}

	if (resultMeshes.empty())
	{
		// Nothing to merge.
		return false;
	}

  int nMergedChunks = 0;
  int nMergedTris = 0;
  for(int i=0; i<resultMeshes.Count(); i++)
  {
    int nTris=0;
    nMergedChunks += resultMeshes[i].pMesh->GetRenderChunksCount(resultMeshes[i].pMesh->GetMaterial(), nTris);
    nMergedTris += nTris;
  }

  if(!nLod && nMergedChunks >= nTotalRenderChunksCount)
  {
    // no reduction of draw call number

    for(int i=0; i<resultMeshes.Count(); i++)
    {
      GetRenderer()->DeleteRenderMesh(resultMeshes[i].pMesh);
      resultMeshes[i].pMesh = NULL;
    }

    return false;
  }

	// Make merged stat object.
	_smart_ptr<CStatObj> pMergedStatObj = new CStatObj;
	pMergedStatObj->m_bForInternalUse = true;
	pMergedStatObj->m_szFileName = m_szFileName;

	IRenderMesh *pMergedMesh = resultMeshes[0].pMesh;
	pMergedMesh->SetMaterial( m_pMaterial );
	pMergedStatObj->SetRenderMesh(pMergedMesh);
	pMergedStatObj->m_bMerged = true;
	pMergedStatObj->SetMaterial( m_pMaterial );
	pMergedStatObj->m_vBoxMin = m_vBoxMin;
	pMergedStatObj->m_vBoxMax = m_vBoxMax;
	pMergedStatObj->CalcRadiuses();

	if (nLod == 0)
	{
		m_pMergedObject = pMergedStatObj;
	}
	else if (m_pMergedObject)
	{
		m_pMergedObject->SetLodObject(nLod,pMergedStatObj);
	}

  m_nMaxUsableLod = m_pMergedObject->m_nMaxUsableLod;

	m_bMerged = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::UnMergeSubObjectsRenderMeshes()
{
	if (m_bMerged)
	{
		if (m_pLod0)
		{
			// If called on LOD.
			m_pLod0->UnMergeSubObjectsRenderMeshes();
		}
		else
		{
			m_bMerged = false;
			m_pMergedObject = 0;
			SetRenderMesh(0);
			for (uint32 i = 1; i < m_nLoadedLodsNum; i++)
			{
				CStatObj *pStatObj = (CStatObj*)GetLodObject(i);
				if (pStatObj)
				{
					pStatObj->m_bMerged = false;
          pStatObj->m_pMergedObject = 0;
					pStatObj->SetRenderMesh(0);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
// This function is recovered from previous FindSubObject function which was then
// changed and creating many CGA model issues (some joints never move).
CStatObj::SSubObject* CStatObj::FindSubObject_CGA( const char *sNodeName )
{
	uint32 numSubObjects = m_subObjects.size();
	for (uint32 i=0; i<numSubObjects; i++)
	{
		if (stricmp(m_subObjects[i].name.c_str(),sNodeName) == 0)
		{
			return &m_subObjects[i];
		}
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
CStatObj::SSubObject* CStatObj::FindSubObject( const char *sNodeName )
{
	uint32 numSubObjects = m_subObjects.size();
	int len;
	for(len=0; sNodeName[len]>' ' && sNodeName[len]!=',' && sNodeName[len]!=';'; len++);
	for (uint32 i=0; i<numSubObjects; i++)
	{
		if (strnicmp(m_subObjects[i].name.c_str(),sNodeName,len)==0 && m_subObjects[i].name[len]==0)
		{
			return &m_subObjects[i];
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
CStatObj::SSubObject* CStatObj::FindSubObject_StrStr( const char *sNodeName )
{
  uint32 numSubObjects = m_subObjects.size();
  for (uint32 i=0; i<numSubObjects; i++)
  {
    if (stristr(m_subObjects[i].name.c_str(),sNodeName))
    {
      return &m_subObjects[i];
    }
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
IStatObj::SSubObject& CStatObj::AddSubObject( IStatObj *pSubObj )
{
	assert( pSubObj );
	SetSubObjectCount( m_subObjects.size()+1 );
	InitializeSubObject( m_subObjects[m_subObjects.size()-1] );
	m_subObjects[m_subObjects.size()-1].pStatObj = pSubObj;
	pSubObj->AddRef();
	return m_subObjects[m_subObjects.size()-1];
}

//////////////////////////////////////////////////////////////////////////
bool CStatObj::RemoveSubObject( int nIndex )
{
	if ( nIndex >= 0 && nIndex < (int)m_subObjects.size() )
	{
		SSubObject *pSubObj = &m_subObjects[nIndex];
		CStatObj *pChildObj = (CStatObj*)pSubObj->pStatObj;
		if (pChildObj)
		{
			if (!m_bSharesChildren)
				pChildObj->m_pParentObject = NULL;
			pChildObj->Release();
		}
		m_subObjects.erase( m_subObjects.begin()+nIndex );
		Invalidate(true);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::SetSubObjectCount( int nCount )
{
	// remove sub objects.
	while (m_subObjects.size() > nCount)
	{
		RemoveSubObject(m_subObjects.size()-1);
	}
	
	int i = m_subObjects.size();
	m_subObjects.resize( nCount );
	for(; i<nCount; i++) 
	{
		InitializeSubObject( m_subObjects[i] );
	}
	if (nCount > 0)
	{
		m_nFlags |= STATIC_OBJECT_COMPOUND;
	}
	Invalidate(true);
}

//////////////////////////////////////////////////////////////////////////
bool CStatObj::CopySubObject( int nToIndex,IStatObj *pFromObj,int nFromIndex )
{
	SSubObject *pSrcSubObj = pFromObj->GetSubObject(nFromIndex);
	if (!pSrcSubObj)
		return false;

	if (nToIndex >= (int)m_subObjects.size())
	{
		SetSubObjectCount(nToIndex+1);
		if (pFromObj==this)
			pSrcSubObj = pFromObj->GetSubObject(nFromIndex);
	}
	
	m_subObjects[nToIndex] = *pSrcSubObj;
	if (pSrcSubObj->pStatObj)
		pSrcSubObj->pStatObj->AddRef();

	Invalidate(true);

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CStatObj::GetPhysicalProperties( float &mass,float &density )
{
	mass = m_phys_mass;
	density = m_phys_density;
	if (mass < 0 && density < 0)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CStatObj::RayIntersection( SRayHitInfo &hitInfo,IMaterial *pCustomMtl )
{
	Vec3 vOut;
	// First check if ray intersect objects bounding box.
	if (!Intersect::Ray_AABB(hitInfo.inRay,AABB(m_vBoxMin,m_vBoxMax),vOut))
		return false;

	// Sometimes, object has no base lod mesh. So need to hit test with low level mesh.
	// If the distance from camera is larger then a base lod distance, then base lod mesh is not loaded yet.
	IRenderMesh* pRenderMesh = (m_nMaxUsableLod == 0 ? m_pRenderMesh : m_arrpLowLODs[m_nMaxUsableLod]->GetRenderMesh());

	if (pRenderMesh)
	{
		bool bRes = CRenderMeshUtils::RayIntersection(pRenderMesh,hitInfo,pCustomMtl);
		if (bRes)
		{
		  hitInfo.pStatObj = this;
		  hitInfo.pRenderMesh = pRenderMesh;
		}
		return bRes;
	}
	else
	{
		SRayHitInfo hitOut;
		bool bAnyHit = false;
		float fMinDistance = FLT_MAX;
		for (int i = 0,num = m_subObjects.size(); i < num; i++)
		{
			if (m_subObjects[i].pStatObj && m_subObjects[i].nType == STATIC_SUB_OBJECT_MESH && !m_subObjects[i].bHidden)
			{
				Matrix34 invertedTM = m_subObjects[i].tm.GetInverted();

				SRayHitInfo hit = hitInfo;
				
				// Transform ray into sub-object local space.
				hit.inReferencePoint = invertedTM.TransformPoint(hit.inReferencePoint);
				hit.inRay.origin = invertedTM.TransformPoint(hit.inRay.origin);
				hit.inRay.direction = invertedTM.TransformVector(hit.inRay.direction);

				if (((CStatObj*)m_subObjects[i].pStatObj)->RayIntersection( hit,pCustomMtl ))
				{
					if (hit.fDistance < fMinDistance)
					{
						hitInfo.pStatObj = m_subObjects[i].pStatObj;
						bAnyHit = true;
						hitOut = hit;
					}
				}
			}
		}
		if (bAnyHit)
		{
			// Restore input ray/reference point.
			hitOut.inReferencePoint = hitInfo.inReferencePoint;
			hitOut.inRay = hitInfo.inRay;

			hitInfo = hitOut;
			return true;
		}
	}
	return false;
}

float CStatObj::GetOcclusionAmount()
{
	if(m_fOcclusionAmount<0)
	{
		PrintMessage("Computing occlusion value for: %s ... ", GetFilePath());

		float fHits=0;
		float fTests=0;

		Matrix34 objMatrix; objMatrix.SetIdentity();

		Vec3 vStep = Vec3(0.5f,0.5f,0.5f);

		Vec3 vClosestHitPoint;
		float fClosestHitDistance=1000;

		// y-rays
		for(float x=m_vBoxMin.x; x<=m_vBoxMax.x; x+=vStep.x)
		{
			for(float z=m_vBoxMin.z; z<=m_vBoxMax.z; z+=vStep.z)
			{
				Vec3 vStart	(x,m_vBoxMin.y,z);
				Vec3 vEnd		(x,m_vBoxMax.y,z);
				if(CObjManager::RayStatObjIntersection(this, objMatrix, GetMaterial(), vStart, vEnd, vClosestHitPoint, fClosestHitDistance, true))
					fHits++;				
				fTests++;
			}
		}

		// x-rays
		for(float y=m_vBoxMin.y; y<=m_vBoxMax.y; y+=vStep.y)
		{
			for(float z=m_vBoxMin.z; z<=m_vBoxMax.z; z+=vStep.z)
			{
				Vec3 vStart	(m_vBoxMin.x,y,z);
				Vec3 vEnd		(m_vBoxMax.x,y,z);
				if(CObjManager::RayStatObjIntersection(this, objMatrix, GetMaterial(), vStart, vEnd, vClosestHitPoint, fClosestHitDistance, true))
					fHits++;
				fTests++;
			}
		}

		// z-rays
		for(float y=m_vBoxMin.y; y<=m_vBoxMax.y; y+=vStep.y)
		{
			for(float x=m_vBoxMin.x; x<=m_vBoxMax.x; x+=vStep.x)
			{
				Vec3 vStart	(x,y,m_vBoxMax.z);
				Vec3 vEnd		(x,y,m_vBoxMin.z);
				if(CObjManager::RayStatObjIntersection(this, objMatrix, GetMaterial(), vStart, vEnd, vClosestHitPoint, fClosestHitDistance, true))
					fHits++;
				fTests++;
			}
		}

		m_fOcclusionAmount = fTests ? (fHits/fTests) : 0;

		PrintMessagePlus("[%.2f]", m_fOcclusionAmount);
	}

	return m_fOcclusionAmount;
}

void CStatObj::SetRenderMesh( IRenderMesh *pRM ) 
{ 

	LOADING_TIME_PROFILE_SECTION(GetSystem());

	if (pRM == m_pRenderMesh)
		return;

	if (m_pRenderMesh)
	{
		GetRenderer()->DeleteRenderMesh(m_pRenderMesh);
    m_arrStatObjForRenderMeshDelete.Delete(this);
	}
	m_pRenderMesh = 0;

  m_pRenderMesh = pRM; 

  if(m_pRenderMesh)
  {
    m_nRenderTrisCount = 0;
    m_nRenderMatIds = 0;

    PodArray<CRenderChunk>& Chunks = m_pRenderMesh->GetChunks();
    for(int nChunkId=0; nChunkId<Chunks.Count(); nChunkId++)
    {
      CRenderChunk *pChunk = &Chunks[nChunkId];
      if(pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
        continue;

			if (pChunk->nNumIndices > 0)
			{
				m_nRenderMatIds++;
				m_nRenderTrisCount += pChunk->nNumIndices/3;
			}
    }
		m_nLoadedTrisCount = pRM->GetIndicesCount()/3;
		m_nLoadedVertexCount = pRM->GetVerticesCount();
  }

	// this will prepare all deformable object during loading instead when needed.
	// Thus is will prevent runtime stalls (300ms when shooting a taxi with 6000 vertices)
	// but will incrase memory since every deformable information needs to be loaded(500kb for the taxi)
	// So this is more a workaround, the real solution would precompute the data in the RC and load
	// the data only when needed into memory
	if(GetCVars()->e_PrepareDeformableObjectsAtLoadTime && m_pRenderMesh && m_pDelayedSkinParams)
	{
		PrepareSkinData(m_pDelayedSkinParams->mtxSkelToMesh, m_pDelayedSkinParams->pPhysSkel, m_pDelayedSkinParams->r);
	}
}

void CStatObj::CheckUpdateObjectHeightmap()
{
  if(m_pHeightmap)
    return;

  PrintMessage("Computing object heightmap for: %s ... ", GetFilePath());

  m_nHeightmapSize = (int)(CLAMP(max(m_vBoxMax.x-m_vBoxMin.x,m_vBoxMax.y-m_vBoxMin.y)*16, 8, 256));
  m_pHeightmap = new float[m_nHeightmapSize*m_nHeightmapSize];
  memset(m_pHeightmap,0,sizeof(float)*m_nHeightmapSize*m_nHeightmapSize);

  float dx = max(0.001f, (m_vBoxMax.x-m_vBoxMin.x)/m_nHeightmapSize);
  float dy = max(0.001f, (m_vBoxMax.y-m_vBoxMin.y)/m_nHeightmapSize);

  Matrix34 objMatrix; objMatrix.SetIdentity();

  for(float x=m_vBoxMin.x+dx; x<m_vBoxMax.x-dx; x+=dx)
  {
    for(float y=m_vBoxMin.y+dy; y<m_vBoxMax.y-dy; y+=dy)
    {
      Vec3 vStart (x, y, m_vBoxMax.z);
      Vec3 vEnd   (x, y, m_vBoxMin.z);

      Vec3 vClosestHitPoint(0,0,0);
      float fClosestHitDistance = 1000000;

      if(CObjManager::RayStatObjIntersection(this, objMatrix, GetMaterial(),
        vStart, vEnd, vClosestHitPoint, fClosestHitDistance, false))
      {
        int nX = CLAMP(int((x-m_vBoxMin.x)/dx), 0, m_nHeightmapSize-1);
        int nY = CLAMP(int((y-m_vBoxMin.y)/dy), 0, m_nHeightmapSize-1);
        m_pHeightmap[nX*m_nHeightmapSize + nY] = vClosestHitPoint.z;
      }
    }
  }

  PrintMessagePlus("[%dx%d] done", m_nHeightmapSize, m_nHeightmapSize);
}


float CStatObj::GetObjectHeight(float x, float y)
{
  CheckUpdateObjectHeightmap();

  float dx = max(0.001f, (m_vBoxMax.x-m_vBoxMin.x)/m_nHeightmapSize);
  float dy = max(0.001f, (m_vBoxMax.y-m_vBoxMin.y)/m_nHeightmapSize);

  int nX = CLAMP(int((x-m_vBoxMin.x)/dx), 0, m_nHeightmapSize-1);
  int nY = CLAMP(int((y-m_vBoxMin.y)/dy), 0, m_nHeightmapSize-1);

  return m_pHeightmap[nX*m_nHeightmapSize + nY];
}

//////////////////////////////////////////////////////////////////////////
int CStatObj::CountChildReferences()
{
	// Check if it must be released.
	int nChildRefs = 0;
	for (int k = 0,numChilds = m_subObjects.size(); k < numChilds; k++)
	{
		IStatObj::SSubObject &so = m_subObjects[k];
		if (so.pStatObj)
			nChildRefs += ((CStatObj*)so.pStatObj)->m_nUsers-1; // 1 reference from parent should be ignored.
	}
	return nChildRefs;
}

IStatObj* CStatObj::GetLowestLod() 
{ 
  if(int nLowestLod = CStatObj::GetMinUsableLod())
    return m_arrpLowLODs[CStatObj::GetMinUsableLod()];
  return this;
}
#endif // !__SPU__

SPU_INDIRECT(UpdateParticles(M))
int CStatObj::AddRef()
{






	m_nUsers++;

	return m_nUsers;
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::GetStatisticsNonRecursive( SStatistics &si )
{
	CStatObj *pStatObj = this;

	for (int lod = 0; lod < MAX_STATOBJ_LODS_NUM; lod++)
	{
		CStatObj *pLod = (CStatObj*)pStatObj->GetLodObject(lod);
		if (pLod)
		{
			//if (!pLod0 && pLod->GetRenderMesh())
				//pLod0 = pLod;

			if (lod > 0 && lod+1 > si.nLods) // Assign last existing lod.
				si.nLods = lod+1;

			si.nIndicesPerLod[lod] += pLod->m_nLoadedTrisCount*3;
			si.nMeshSize += pLod->m_nRenderMeshMemoryUsage;

			IRenderMesh *pRenderMesh = pLod->GetRenderMesh();
			if (pRenderMesh)
			{
				si.nMeshSizeLoaded += pRenderMesh->GetMemoryUsage(0,IRenderMesh::MEM_USAGE_ONLY_STREAMS);
				//if (si.pTextureSizer)
					//pRenderMesh->GetTextureMemoryUsage(0,si.pTextureSizer);
				//if (si.pTextureSizer2)
					//pRenderMesh->GetTextureMemoryUsage(0,si.pTextureSizer2);
			}
		}
	}

	si.nIndices += m_nLoadedTrisCount*3;
	si.nVertices += m_nLoadedVertexCount;

	for(int j = 0; j < MAX_PHYS_GEOMS_TYPES; j++)
	{
		if (pStatObj->GetPhysGeom(j))
		{
			ICrySizer *pPhysSizer = GetISystem()->CreateSizer();
			pStatObj->GetPhysGeom(j)->pGeom->GetMemoryStatistics(pPhysSizer);
			si.nPhysProxySize += pPhysSizer->GetTotalSize(); 
			si.nPhysPrimitives += pStatObj->GetPhysGeom(j)->pGeom->GetPrimitiveCount();
			pPhysSizer->Release();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::GetStatistics( SStatistics &si )
{
	si.bSplitLods = m_bLodsAreLoadedFromSeparateFile;

	bool bMultiSubObj = (GetFlags() & STATIC_OBJECT_COMPOUND) != 0;
	if (!bMultiSubObj)
	{
		GetStatisticsNonRecursive( si );
		si.nSubMeshCount = 0;
		si.nNumRefs = GetNumRefs();
	}
	else
	{
		si.nNumRefs = GetNumRefs();

		std::vector<void*> addedObjects;
		for (int k = 0; k < GetSubObjectCount(); k++)
		{
			if (!GetSubObject(k))
				continue;
			CStatObj *pSubObj = (CStatObj*)GetSubObject(k)->pStatObj;

			int nSubObjectType = GetSubObject(k)->nType;
			if (nSubObjectType != STATIC_SUB_OBJECT_MESH && nSubObjectType != STATIC_SUB_OBJECT_HELPER_MESH)
				continue;

			if (stl::find( addedObjects,pSubObj ))
				continue;
			addedObjects.push_back(pSubObj);

			if(pSubObj)
			{
				pSubObj->GetStatisticsNonRecursive( si );
				si.nSubMeshCount++;

				if (pSubObj->GetNumRefs() > si.nNumRefs)
					si.nNumRefs = pSubObj->GetNumRefs();
			}
		}
	}

	// Only do rough estimation based on the material
	// This is more consistent when streaming is enabled and render mesh may no exist
	if (m_pMaterial)
	{
		if (si.pTextureSizer)
			m_pMaterial->GetTextureMemoryUsage(si.pTextureSizer);
		if (si.pTextureSizer2)
			m_pMaterial->GetTextureMemoryUsage(si.pTextureSizer2);
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(IStatObj)
