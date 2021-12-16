////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain_sector_render.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Create buffer, copy it into var memory, draw
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "terrain.h"
#include "terrain_sector.h"
#include "ObjMan.h"
#include "terrain_water.h"
#include "CryThread.h"
































// specialized container, which can manage main memory and a SPU local "ghost" memory
// to be used on SPU with an synchron writeback
template<typename Type>
class CTerrainTempDataStorage
{
public:
	CTerrainTempDataStorage();
	
	void Clear();
	void Free();

	void PreAllocate( int n );

	int Count() const {	return m_nCount; }
	Type* GetElements() const { return m_pMemory; }

	void Add( const Type &obj);
	SPU_NO_INLINE void AddFallback( const Type &obj );

	// SPU only interface
	int MemorySize() const { return m_nCap * sizeof(Type); }






private:
	SPU_DOMAIN_MAIN Type*		m_pMemory;		// continuous memory block to store objects in main memory
	int			m_nCount;			// number of objects in the container
	int			m_nCap;				// maximum number of objects fitting into the allocated memory

	// these are only used for SPU
	SPU_DOMAIN_LOCAL Type* m_pSPUMemory;		// "ghost" memory allocated on spu to operate there on it
	int m_nOffset;				// byte offset of already to main memory transfered data
	int m_nDmaId;					// DMA Id to use

	static const int m_nChunkSize = 2048; // start transfer each 2kb
};

template<typename Type>
CTerrainTempDataStorage<Type>::CTerrainTempDataStorage() :
	m_pMemory(NULL),
	m_nCount(0),
	m_nCap(0),
	m_pSPUMemory(NULL),
	m_nOffset(0),
	m_nDmaId(0)
{}

template<typename Type>
void CTerrainTempDataStorage<Type>::Clear()
{
	if( m_nOffset != 0 ){ snPause();}

	m_nCount = 0;
}

template<typename Type>
void CTerrainTempDataStorage<Type>::Free()
{
	if( m_nOffset != 0 ) { snPause();}

	if( m_pMemory )
		CryModuleMemalignFree( m_pMemory );

	m_pMemory = NULL;
	m_nCount = 0;
	m_nCap = 0;
}

template<typename Type>
void CTerrainTempDataStorage<Type>::PreAllocate( int n )
{
	Free();

	m_pMemory = (Type*)CryModuleMemalign( sizeof(Type)*n, 128 );
	m_nCap = n;
}

template<typename Type>
void CTerrainTempDataStorage<Type>::Add( const Type &obj)
{
	assert(m_nCount < m_nCap);

	// special case for when a reallocation is needed
	IF( m_nCount == m_nCap || m_pSPUMemory == NULL, false )
	{
		AddFallback( obj );
		return;
	}

	// add object
	Type* pMemory = SPU_PTR_SELECT(m_pMemory, m_pSPUMemory);
	pMemory[m_nCount] = obj;
	++m_nCount;













}

template<typename Type>
SPU_NO_INLINE void CTerrainTempDataStorage<Type>::AddFallback( const Type &obj )
{




	if( m_nCap == 0 )
	{
		// if no memory was there, allocate enough for 4 objects
		PreAllocate(4);
	}

	if(m_nCount == m_nCap)
	{
		// use manual realloc to prevent problems in the drivers memcpy$VMM
		Type* pTemp = (Type*)CryModuleMemalign( sizeof(Type)*m_nCap*2, 128 );
		memcpy( SPU_MAIN_PTR(pTemp), SPU_MAIN_PTR(m_pMemory), sizeof(Type)*m_nCount );
		CryModuleMemalignFree(m_pMemory);
		m_pMemory = pTemp;
		m_nCap = m_nCap*2;
	}

	// now store object
	m_pMemory[m_nCount] = obj;
	++m_nCount;
}

// SPU only interface
































struct CStripInfo {int begin,end;};

struct CStripsInfo 
{ 
	CTerrainTempDataStorage<unsigned short> idx_array; 
	int nNonBorderIndicesCount;

	inline void Clear() { idx_array.Clear(); nNonBorderIndicesCount=0; } 

	inline void AddIndex(int _x, int _y, int _step,  int nSectorSize)
	{
		unsigned short id = _x/_step*(nSectorSize/_step+1) + _y/_step;
		int nCount = idx_array.Count();
		idx_array.Add(id);
	}

	static uint16 GetIndex(int _x, int _y, int _step,  int nSectorSize)
	{
		unsigned short id = _x/_step*(nSectorSize/_step+1) + _y/_step;
		return id;
	}

};

class CUpdateTerrainTempData
{	
public:
	









	// align objects to 128 byte to provide each with an own cacheline since they are updated in parallel
	// for Update Indices
	CStripsInfo										m_StripsInfo _ALIGN(128);
	// for update vectices	
	CTerrainTempDataStorage<SVF_P3S_N4B_C4B_T2S> m_lstTmpVertArray _ALIGN(128);
};

// container to manager temp memory as well as running update jobs
class TerrainUpdateDispatcher
{
public:
	TerrainUpdateDispatcher()  : m_nPreAllocatedSlotsInUse(0) {}
	~TerrainUpdateDispatcher() { SyncAllJobs(); }

	CUpdateTerrainTempData* GetTempData();
	void DestroyTempData( CUpdateTerrainTempData *&pTempData );

	void AddJob( CTerrainNode *pNode );
	void SyncAllJobs();

private:
	static const int nPreAllocatedTempData = 4;

	PodArray<CTerrainNode*>		m_arrRunningJobs;
	CUpdateTerrainTempData		m_arrPreAllocatedTempData[nPreAllocatedTempData];
	int												m_nPreAllocatedSlotsInUse;

} gTerrainUpdateDispatcher;


CUpdateTerrainTempData* TerrainUpdateDispatcher::GetTempData()
{
	if( m_nPreAllocatedSlotsInUse < nPreAllocatedTempData )
	{
		return &m_arrPreAllocatedTempData[m_nPreAllocatedSlotsInUse++];		
	}

	return new CUpdateTerrainTempData();
}

void TerrainUpdateDispatcher::DestroyTempData( CUpdateTerrainTempData *&pTempData )
{
	if( pTempData >= &m_arrPreAllocatedTempData[0] && pTempData < &m_arrPreAllocatedTempData[nPreAllocatedTempData] )
	{
		pTempData->m_StripsInfo.Clear();
		pTempData->m_lstTmpVertArray.Clear();		
		
	} 
	else 
	{
		pTempData->m_StripsInfo.idx_array.Free();
		pTempData->m_lstTmpVertArray.Free();		
		delete pTempData;
	}

	pTempData = NULL;
}

void TerrainUpdateDispatcher::SyncAllJobs()
{
	for( int i = 0 ; i < m_arrRunningJobs.Count() ; ++i )
	{
		m_arrRunningJobs[i]->RenderSectorUpdate_Finish();
	}


	m_arrRunningJobs.Clear();
	m_nPreAllocatedSlotsInUse = 0;
}

void TerrainUpdateDispatcher::AddJob( CTerrainNode *pNode )
{
	m_arrRunningJobs.Add(pNode);
}

// pure function to be exposed and used to sync all jobs
void SyncAllTerrainNodes()
{
	gTerrainUpdateDispatcher.SyncAllJobs();
}


PodArray<uint16> CTerrainNode::m_arrIndices[MAX_SURFACE_TYPES_COUNT][4];

void CTerrainNode::SetupTexGenParams(SSurfaceType * pSurfaceType, float * pOutParams, uint8 ucProjAxis, bool bOutdoor, float fTexGenScale)
{
	assert(pSurfaceType);

  if(pSurfaceType->fCustomMaxDistance>0)
  {
    pSurfaceType->fMaxMatDistanceZ = pSurfaceType->fMaxMatDistanceXY = pSurfaceType->fCustomMaxDistance;
  }
  else
  {
	  pSurfaceType->fMaxMatDistanceZ	= float(GetCVars()->e_TerrainDetailMaterialsViewDistZ  *Get3DEngine()->m_fTerrainDetailMaterialsViewDistRatio);
	  pSurfaceType->fMaxMatDistanceXY = float(GetCVars()->e_TerrainDetailMaterialsViewDistXY *Get3DEngine()->m_fTerrainDetailMaterialsViewDistRatio);
  }

	// setup projection direction
	if(ucProjAxis == 'X')
	{
		pOutParams[0] = 0;
		pOutParams[1] = pSurfaceType->fScale*fTexGenScale;
		pOutParams[2] = 0;
		pOutParams[3] = pSurfaceType->fMaxMatDistanceXY;

		pOutParams[4] = 0;
		pOutParams[5] = 0;
		pOutParams[6] = -pSurfaceType->fScale*fTexGenScale;
		pOutParams[7] = 0;
	}
	else if(ucProjAxis =='Y')
	{
		pOutParams[0] = pSurfaceType->fScale*fTexGenScale;
		pOutParams[1] = 0;
		pOutParams[2] = 0;
		pOutParams[3] = pSurfaceType->fMaxMatDistanceXY;

		pOutParams[4] = 0;
		pOutParams[5] = 0;
		pOutParams[6] = -pSurfaceType->fScale*fTexGenScale;
		pOutParams[7] = 0;
	}
	else // Z
	{
		pOutParams[0] = pSurfaceType->fScale*fTexGenScale;
		pOutParams[1] = 0;
		pOutParams[2] = 0;
		pOutParams[3] = pSurfaceType->fMaxMatDistanceZ;

		pOutParams[4] = 0;
		pOutParams[5] = -pSurfaceType->fScale*fTexGenScale;
		pOutParams[6] = 0;
		pOutParams[7] = 0;
	}
}

void CTerrainNode::SetupTexGens()
{
	IRenderMesh * & pRenderMesh = GetLeafData()->m_pRenderMesh;

//	if(pRenderMesh->GetChunks()->Count())
	//	for(int i=TERRAIN_BASE_TEXTURES_NUM; i<MAX_CUSTOM_TEX_BINDS_NUM; i++)
		//	pRenderMesh->GetChunks()->GetAt(0).pRE->m_CustomTexBind[i] = 0;

	pRenderMesh->SetMaterial(GetTerrain()->m_pTerrainEf, m_nTexSet.nTex0);

	if (pRenderMesh->GetChunks().Count() && pRenderMesh->GetChunks()[0].pRE)
	{
		if(GetCVars()->e_TerrainTextureDebug == 0)
			pRenderMesh->GetChunks()[0].pRE->m_CustomTexBind[0] = m_nTexSet.nTex0;
		else if(GetCVars()->e_TerrainTextureDebug == 1)
			pRenderMesh->GetChunks()[0].pRE->m_CustomTexBind[0] = m_pTerrain->m_nWhiteTexId;
		else
			pRenderMesh->GetChunks()[0].pRE->m_CustomTexBind[0] = 0;

		pRenderMesh->GetChunks()[0].pRE->m_CustomTexBind[1] = m_nTexSet.nTex1;
	}
}

void CTerrainNode::DrawArray()
{
//	if(m_nSetupTexGensFrameId < GetMainFrameID())
	{
		SetupTexGens();
		//m_nSetupTexGensFrameId = GetMainFrameID();
	}

	IRenderMesh * & pRenderMesh = GetLeafData()->m_pRenderMesh;

	// make renderobject
	CRenderObject * pTerrainRenderObject = GetIdentityCRenderObject();
  if (!pTerrainRenderObject)
    return;
//	if(GetCVars()->e_HwOcclusionCullingObjects)
	//	pTerrainRenderObject->m_ObjFlags |= FOB_NO_Z_PASS;
	pTerrainRenderObject->m_pRenderNode = 0;
	pTerrainRenderObject->m_II.m_AmbColor = Get3DEngine()->GetSkyColor();
  pTerrainRenderObject->m_fDistance = m_arrfDistance[m_nRenderStackLevel];
/*	if(!m_nRenderStackLevel)
	{
		pTerrainRenderObject->m_nScissorX1 = GetCamera().m_ScissorInfo.x1;
		pTerrainRenderObject->m_nScissorY1 = GetCamera().m_ScissorInfo.y1;
		pTerrainRenderObject->m_nScissorX2 = GetCamera().m_ScissorInfo.x2;
		pTerrainRenderObject->m_nScissorY2 = GetCamera().m_ScissorInfo.y2;
	}*/

  pTerrainRenderObject->m_II.m_Matrix.SetIdentity(); 
  Vec3 vOrigin(m_nOriginX, m_nOriginY, 0);
  vOrigin += GetTerrain()->m_arrSegmentOrigns[m_nSID];
  pTerrainRenderObject->m_II.m_Matrix.SetTranslation(vOrigin);
  pTerrainRenderObject->m_ObjFlags |= FOB_TRANS_TRANSLATE;

  uint32 nSectorDLMask = 0;

  PodArray<CDLight*> * pAffLight = GetAffectingLights();

	// make light mask for terrain sector
	if(!IsRenderIntoShadowmap())
    nSectorDLMask = Get3DEngine()->BuildLightMask( GetBBox(), pAffLight, NULL, true );

  // pass only sun to base terrain pass
  if(pAffLight && pAffLight->Count() && (pAffLight->GetAt(0)->m_Flags & DLF_SUN))
    pTerrainRenderObject->m_DynLMMask[m_nRenderThreadListID] = 1;

	if(!IsRenderIntoShadowmap() && GetCVars()->e_Shadows)
		pTerrainRenderObject->m_ObjFlags |= FOB_INSHADOW;

	if(GetTerrain()->m_arrBaseTexInfos[m_nSID].m_hdrDiffTexInfo.dwFlags == TTFHF_AO_DATA_IS_VALID && GetCVars()->e_TerrainNormalMap)
	{ // provide normal map texture for terrain shader
		pTerrainRenderObject->m_nTextureID = m_nTexSet.nTex0;
		//pTerrainRenderObject->m_nTextureID1 = m_nTexSet.nTex1;
    SRenderObjData *pOD = GetRenderer()->EF_GetObjData(pTerrainRenderObject, true);
		pOD->m_fTempVars[0] = m_nTexSet.fTexOffsetX;
		pOD->m_fTempVars[1] = m_nTexSet.fTexOffsetY;
		pOD->m_fTempVars[2] = m_nTexSet.fTexScale;
		pOD->m_fTempVars[3] = m_nTexSet.fTerrainMinZ;
		pOD->m_fTempVars[4] = m_nTexSet.fTerrainMaxZ;
		pTerrainRenderObject->m_ObjFlags |= FOB_AMBIENT_OCCLUSION;
	}

	// render terrain base
  //if(m_arrfDistance[0]<64)
  //  pTerrainRenderObject->m_ObjFlags |= FOB_ONLY_Z_PASS;

 	pRenderMesh->AddRenderElements(pTerrainRenderObject, EFSLIST_GENERAL, 1, NULL);

	if(GetCVars()->e_TerrainDetailMaterials /*&& !m_nRenderStackLevel*/ && !IsRenderIntoShadowmap())
	{
		CRenderObject * pDetailObj = NULL;

		for(int i=0; i<m_lstSurfaceTypeInfo.Count(); i++)
		{
			assert(1 || m_lstSurfaceTypeInfo[i].pSurfaceType->HasMaterial() && m_lstSurfaceTypeInfo[i].HasRM());

      if(!m_lstSurfaceTypeInfo[i].pSurfaceType->HasMaterial() || !m_lstSurfaceTypeInfo[i].HasRM())
        continue;

			if(!pDetailObj)
			{ // make object on first attempt to draw detail layer
				pDetailObj = GetIdentityCRenderObject();
        if (!pDetailObj)
          return;
				pDetailObj->m_pRenderNode = 0;
        pDetailObj->m_ObjFlags |= (((!m_nRenderStackLevel)? FOB_NO_FOG : 0)); // enable fog on recursive rendering (per-vertex)
				pDetailObj->m_II.m_AmbColor = Get3DEngine()->GetSkyColor();
				pDetailObj->m_DynLMMask[m_nRenderThreadListID] = nSectorDLMask;
        pDetailObj->m_fDistance = m_arrfDistance[m_nRenderStackLevel];
        pDetailObj->m_II.m_Matrix = pTerrainRenderObject->m_II.m_Matrix;
        pDetailObj->m_ObjFlags |= FOB_TRANS_TRANSLATE;

				if(GetCVars()->e_Shadows && pDetailObj->m_DynLMMask)
        {
					  pDetailObj->m_ObjFlags |= FOB_INSHADOW;
        }

        if(GetTerrain()->m_arrBaseTexInfos[m_nSID].m_hdrDiffTexInfo.dwFlags == TTFHF_AO_DATA_IS_VALID && GetCVars()->e_TerrainNormalMap)
        { // provide normal map texture for terrain shader
          pDetailObj->m_nTextureID = m_nTexSet.nTex0;
          //pDetailObj->m_nTextureID1 = m_nTexSet.nTex1;
          SRenderObjData *pOD = GetRenderer()->EF_GetObjData(pDetailObj, true);
          pOD->m_fTempVars[0] = m_nTexSet.fTexOffsetX;
          pOD->m_fTempVars[1] = m_nTexSet.fTexOffsetY;
          pOD->m_fTempVars[2] = m_nTexSet.fTexScale;
          pOD->m_fTempVars[3] = m_nTexSet.fTerrainMinZ;
          pOD->m_fTempVars[4] = m_nTexSet.fTerrainMaxZ;
          pDetailObj->m_ObjFlags |= FOB_AMBIENT_OCCLUSION;
        }
			}

			// test for Mikko
//			bool bUseTerrainLayerList = (GetCVars()->e_DebugMask & 0x1)!=0;

			uint8 szProj[] = "XYZ";
			for(int p=0; p<3; p++)
			{
        if(SSurfaceType * pSurf = m_lstSurfaceTypeInfo[i].pSurfaceType)
				if(pSurf->GetMaterialOfProjection(szProj[p]))
        {
          pSurf->fMaxMatDistanceZ	  = float(GetCVars()->e_TerrainDetailMaterialsViewDistZ  *Get3DEngine()->m_fTerrainDetailMaterialsViewDistRatio);
          pSurf->fMaxMatDistanceXY  = float(GetCVars()->e_TerrainDetailMaterialsViewDistXY *Get3DEngine()->m_fTerrainDetailMaterialsViewDistRatio);

					if(m_arrfDistance[m_nRenderStackLevel]<pSurf->GetMaxMaterialDistanceOfProjection(szProj[p]))
            if(IRenderMesh * pMesh = m_lstSurfaceTypeInfo[i].arrpRM[p])
            {
              AABB aabb;
              pMesh->GetBBox(aabb.min, aabb.max);
              if(GetCamera().IsAABBVisible_F(aabb))
						    if(pMesh->GetVertexContainer() == pRenderMesh && pMesh->GetIndicesCount())
                {
                  if(GetCVars()->e_TerrainBBoxes==2)
                    GetRenderer()->GetIRenderAuxGeom()->DrawAABB(aabb,false,ColorB(255*((m_nTreeLevel&1)>0),255*((m_nTreeLevel&2)>0),0,255),eBBD_Faceted);

							    pMesh->AddRenderElements(pDetailObj, EFSLIST_TERRAINLAYER, 1);
                }
            }
        }
			}
    }
	}

	// if there are more than 3 detail texture layers - render second pass
	/*  float fZoomFactor = 0.1f+0.9f*(RAD2DEG(GetCamera().GetFov())/90.f);      
	if(bAllowSingePassZ && m_arrSSurfaceType[3] && m_fDistance*fZoomFactor < 90 && !m_nRenderStackLevel)
	pRenderMesh->AddRenderElements( pTerrainRenderObject, 0, 0, EFSLIST_GENERAL, GetTerrain()->m_pMatSecondPass );
	*/
	// terrain fog pass
/*	if( m_pFogVolume && m_pFogVolume->pMat && !nTechniqueID)
	{  
		if(!m_pFogVolume->bOcean || 
			( GetTerrain()->m_pOcean && GetTerrain()->m_pOcean->IsWaterVisible() &&
				GetObjManager()->m_dwRecursionDrawFlags[Cry3DEngineBase::m_nRenderStackLevel] & DLD_TERRAIN_WATER))
		{
			CRenderObject * pTerrainFogCRenderObject = GetRenderer()->EF_GetObject(true);
			pTerrainFogCRenderObject->m_ObjFlags |= FOB_FOGPASS;
			pTerrainFogCRenderObject->m_Matrix.SetIdentity();
			if(!m_nRenderStackLevel)
			{
				pTerrainFogCRenderObject->m_nScissorX1 = GetCamera().m_ScissorInfo.x1;
				pTerrainFogCRenderObject->m_nScissorY1 = GetCamera().m_ScissorInfo.y1;
				pTerrainFogCRenderObject->m_nScissorX2 = GetCamera().m_ScissorInfo.x2;
				pTerrainFogCRenderObject->m_nScissorY2 = GetCamera().m_ScissorInfo.y2;
			}
			pRenderMesh->AddRenderElements( pTerrainFogCRenderObject, 0, m_pFogVolume->nRendererVolumeID, EFSLIST_TRANSP | eS_TerrainFogPass );
		}
	}*/
/*
  if(GetCVars()->e_CoverageBufferTerrain && m_arrfDistance[m_nRenderStackLevel] < GetCVars()->e_CoverageBufferTerrain)
  {
    Matrix34 mat;
    mat.SetIdentity();
    Get3DEngine()->GetCoverageBuffer()->AddRenderMesh(pRenderMesh, &mat, pRenderMesh->GetMaterial(), true, false, false);
  }*/
}

// update data in video buffer
void CTerrainNode::UpdateRenderMesh(CStripsInfo * pArrayInfo, bool bUpdateVertices)
{
	FUNCTION_PROFILER_3DENGINE;

	IRenderMesh * & pRenderMesh = GetLeafData()->m_pRenderMesh;

	// if changing lod or there is no buffer allocated - reallocate
	if(!pRenderMesh || m_cCurrGeomMML != m_cNewGeomMML || bUpdateVertices)// || GetCVars()->e_TerrainDrawThisSectorOnly)
	{ 
		if(pRenderMesh)
			GetRenderer()->DeleteRenderMesh(pRenderMesh);  

		assert(m_pUpdateTerrainTempData->m_lstTmpVertArray.Count()<65536);

		static PodArray<SPipTangents> lstTangents; lstTangents.Clear();

		if(GetCVars()->e_DefaultMaterial && m_pUpdateTerrainTempData->m_lstTmpVertArray.Count())
		{
			lstTangents.PreAllocate(m_pUpdateTerrainTempData->m_lstTmpVertArray.Count(),m_pUpdateTerrainTempData->m_lstTmpVertArray.Count());

			Vec3 vBinorm(1,0,0); 
			vBinorm.Normalize();

			Vec3 vTang(0,1,0); 
			vTang.Normalize();

			Vec3 vNormal(0,0,1);

			vBinorm = -vNormal.Cross(vTang);
			vTang = vNormal.Cross(vBinorm);

			lstTangents[0].Binormal = Vec4sf(tPackF2B(vBinorm.x),tPackF2B(vBinorm.y),tPackF2B(vBinorm.z), tPackF2B(-1));
			lstTangents[0].Tangent  = Vec4sf(tPackF2B(vTang.x),tPackF2B(vTang.y),tPackF2B(vTang.z), tPackF2B(-1));

			for(int i=1; i<lstTangents.Count(); i++)
				lstTangents[i] = lstTangents[0];
		}

		pRenderMesh = GetRenderer()->CreateRenderMeshInitialized(
			m_pUpdateTerrainTempData->m_lstTmpVertArray.GetElements(), m_pUpdateTerrainTempData->m_lstTmpVertArray.Count(), eVF_P3S_N4B_C4B_T2S, NULL, 0,
			R_PRIMV_TRIANGLES, "TerrainSector", "TerrainSector", eRMT_KeepSystem, 1, 
			m_nTexSet.nTex0, NULL, NULL, false, true, lstTangents.Count() ? lstTangents.GetElements() : NULL);    
		AABB boxWS = GetBBox();
    pRenderMesh->SetBBox(boxWS.min, boxWS.max);

    if(GetCVars()->e_TerrainLog==1)
      PrintMessage("RenderMesh created %d", GetSecIndex());
	}

  if (!bUpdateVertices && !GetCVars()->e_TerrainDrawThisSectorOnly)
    assert(pArrayInfo->idx_array.Count()<=(int)pRenderMesh->GetIndicesCount());

	pRenderMesh->UpdateIndices(pArrayInfo->idx_array.GetElements(), pArrayInfo->idx_array.Count());

	pRenderMesh->SetChunk(GetTerrain()->m_pTerrainEf,0,pRenderMesh->GetVerticesCount(), 0, pArrayInfo->idx_array.Count(), 1.0f, 0, true);

	if(pRenderMesh->GetChunks().Count() && pRenderMesh->GetChunks()[0].pRE)
		pRenderMesh->GetChunks()[0].pRE->m_CustomData = GetLeafData()->m_arrTexGen[0];

	assert(pRenderMesh->GetChunks().Count() <= 256); // holes may cause more than 100 chunks
}

void CTerrainNode::UpdateRangeInfoShift()
{
  if(!m_arrChilds[0] || !m_nTreeLevel)
    return;

  m_rangeInfo.nUnitBitShift = GetTerrain()->m_nUnitsToSectorBitShift;

  for(int i=0; i<4; i++)
  {
    m_arrChilds[i]->UpdateRangeInfoShift();
    m_rangeInfo.nUnitBitShift = min(m_arrChilds[i]->m_rangeInfo.nUnitBitShift, m_rangeInfo.nUnitBitShift);
  }
}

void CTerrainNode::ReleaseHeightMapGeometry(bool bRecursive, const AABB * pBox)
{
  if(pBox && !Overlap::AABB_AABB(*pBox, GetBBox()))
    return;

	for(int i=0; i<m_lstSurfaceTypeInfo.Count(); i++)
		m_lstSurfaceTypeInfo[i].DeleteRenderMeshes(GetRenderer());

	if(GetLeafData() && GetLeafData()->m_pRenderMesh)
	{
		IRenderMesh * & pRenderMesh = GetLeafData()->m_pRenderMesh;

		GetRenderer()->DeleteRenderMesh(pRenderMesh);  
		pRenderMesh = NULL;

		if (GetCVars()->e_TerrainLog==1)
			PrintMessage("RenderMesh unloaded %d", GetSecIndex());
	}

	delete m_pLeafData;
	m_pLeafData = NULL;

//  GetRenderer()->DeleteRenderMesh(m_pCBRenderMesh); m_pCBRenderMesh = NULL;

  if (bRecursive && m_arrChilds[0])
    for(int i=0; i<4; i++)
      m_arrChilds[i]->ReleaseHeightMapGeometry(bRecursive, pBox);
}

// fill vertex buffer
void CTerrainNode::BuildVertices(int nStep, bool bSafetyBorder)
{
	FUNCTION_PROFILER_3DENGINE;

	m_pUpdateTerrainTempData->m_lstTmpVertArray.Clear();

	int nSectorSize = CTerrain::GetSectorSize()<<m_nTreeLevel;

  int nSafetyBorder = bSafetyBorder ? nStep : 0;

	// keep often used variables on stack
	const uint16	nOriginX = m_nOriginX;
	const uint16	nOriginY = m_nOriginY;
	const int			nSID = m_nSID;
	const int			nTerrainSize = CTerrain::GetTerrainSize();
	CTerrain*			pTerrain = GetTerrain();

	for( int x=nOriginX-nSafetyBorder; x<=nOriginX+nSectorSize+nSafetyBorder; x+=nStep )
  {
		for( int y=nOriginY-nSafetyBorder; y<=nOriginY+nSectorSize+nSafetyBorder; y+=nStep )
		{
      int _x = CLAMP(x, nOriginX, nOriginX+nSectorSize);
      int _y = CLAMP(y, nOriginY, nOriginY+nSectorSize);
      float _z = pTerrain->GetZ(_x,_y, nSID);

      // close gaps on border of terrain for shadow mapping
//      if(	_x <= 0 || _x >= pTerrain->GetTerrainSize() ||
  //      _y <= 0 || _y >= pTerrain->GetTerrainSize())
    //    _z = 0;

      // safety borders are used for cbuffer, note: holes not supported here
//      if(	x != _x || y != _y )
  //      _z = max(_z-2.f*nStep, 0.f);

      SVF_P3S_N4B_C4B_T2S vert;
      vert.xyz = Vec3f16( (float)(_x-nOriginX),
                          (float)(_y-nOriginY),
                          _z
                         );
      //vert.xyz.x = (float)(_x-nOriginX);
			//vert.xyz.y = (float)(_y-nOriginY);
			//vert.xyz.z = (pTerrain->GetZ(_x,_y));    

			int iLookupRadius = nStep;

			if(x==nOriginX || x==nOriginX+nSectorSize || y==nOriginY || y==nOriginY+nSectorSize)
				iLookupRadius=1;

			// calculate surface normal
			float sx;
			if((x+iLookupRadius)<nTerrainSize && x>=iLookupRadius)
				sx = pTerrain->GetZ(x+iLookupRadius,y, nSID  ) - pTerrain->GetZ(x-iLookupRadius,y, nSID  );
			else
				sx = 0;

			float sy;
			if((y+iLookupRadius)<nTerrainSize && y>=iLookupRadius)
				sy = pTerrain->GetZ(x  ,y+iLookupRadius, nSID) - pTerrain->GetZ(x  ,y-iLookupRadius, nSID);
			else
				sy = 0;

			// z component of normal will be used as point brightness ( for burned terrain )
			Vec3 vNorm(-sx, -sy, iLookupRadius*2.0f);
			vNorm.Normalize();

			vert.normal.bcolor[0] = (byte)(vNorm[0] * 127.5f + 128.0f);
      vert.normal.bcolor[1] = (byte)(vNorm[1] * 127.5f + 128.0f);
      vert.normal.bcolor[2] = (byte)(vNorm[2] * 127.5f + 128.0f);
      vert.normal.bcolor[3] = 255;
      SwapEndian(vert.normal.dcolor, eLittleEndian);

			uint8 ucSurfaceTypeID = pTerrain->GetSurfaceTypeID(x,y, nSID);
			if(ucSurfaceTypeID == STYPE_HOLE)
			{ // in case of hole - try to find some valid surface type around
				for(int i=-nStep; i<=nStep && (ucSurfaceTypeID == STYPE_HOLE); i+=nStep)
					for(int j=-nStep; j<=nStep && (ucSurfaceTypeID == STYPE_HOLE); j+=nStep)
						ucSurfaceTypeID = pTerrain->GetSurfaceTypeID(x+i,y+j, nSID);
			}

      ucSurfaceTypeID = min(ucSurfaceTypeID, MAX_SURFACE_TYPES_COUNT-1);

			vert.color.bcolor[0] = 255;
			vert.color.bcolor[1] = ucSurfaceTypeID;
			vert.color.bcolor[2] = 255;
			vert.color.bcolor[3] = 255;
			SwapEndian(vert.color.dcolor, eLittleEndian);

      // generate texture coordinates
//      float * BaseTCMatrix = GetLeafData()->m_arrTexGen[0];
  //    vert.st.x = vert.xyz.y*BaseTCMatrix[2] + BaseTCMatrix[0];
    //  vert.st.y = vert.xyz.x*BaseTCMatrix[2] + BaseTCMatrix[1];
      
      // Less than -1 means usual (old) texgen will be used in vertex shader (temporary)
      // Coordinates from vertex are used only in case of CVoxTerrain
      vert.st = Vec2f16(-100.f, -100.f);
      //vert.st.x = vert.st.y = -100.f;

      //vert.xyz += pTerrain->m_arrSegmentOrigns[nSID]; // TODO: Please fix me, Vlad!

			m_pUpdateTerrainTempData->m_lstTmpVertArray.Add(vert);
		}
  }

	// include some of brushes into terrain mesh, for example for nice golf cup
	if(m_nTreeLevel == 0)
	{ 
    PodArray<IRenderNode*> lstBrushes; lstBrushes.Clear();
    AABB boxWS = GetBBox();
    if(Get3DEngine()->m_pObjectsTree[nSID])
      Get3DEngine()->m_pObjectsTree[nSID]->GetObjectsByType(lstBrushes, eERType_Brush, &boxWS, boxWS.GetRadius()*2.f);

    for(int i=0; i<lstBrushes.Count(); i++)
    {
			if(lstBrushes[i]->GetIntegrationType() != eIT_TerrainMesh && lstBrushes[i]->GetIntegrationType() != eIT_TerrainMeshAligned)
				continue;

      Matrix34A objMat;
      CStatObj * pStatObj = (CStatObj *)lstBrushes[i]->GetEntityStatObj(0,0,&objMat);

      if(IRenderMesh * pRM = pStatObj ? pStatObj->GetRenderMesh() : 0)
      {
        int nPosStride=0;
        const byte * pPos = pRM->GetPosPtr(nPosStride, FSL_READ);
        const int nVertCount = pRM->GetVerticesCount();

        int nTangStride=0;
        int nBNormStride=0;
        byte * pTang = 0;
        byte * pBNorm = 0;
        if (pRM->GetVertexFormat() != eVF_P3S_N4B_C4B_T2S)
        {
          pTang = pRM->GetTangentPtr(nTangStride, FSL_READ);
          pBNorm = pRM->GetBinormalPtr(nBNormStride, FSL_READ);
        }

        for(int v=0; v<nVertCount; v++)
        {
          Vec3 vPosOS; memcpy(&vPosOS, &pPos[nPosStride*v], sizeof(vPosOS));
          Vec3 vPos = objMat.TransformPoint(vPosOS);

					if(lstBrushes[i]->GetIntegrationType() == eIT_TerrainMeshAligned)
						vPos.z = pTerrain->GetZApr(vPos.x,vPos.y, false, nSID) + (vPos.z - objMat.GetTranslation().z);

          SVF_P3S_N4B_C4B_T2S vert;   
          vert.xyz = vPos - Vec3(nOriginX, nOriginY, 0);

          Vec3 vNorm(0, 0, 1.f);

          if(pTang && pBNorm)
          {
            Vec4sf Tangent; 
            memcpy(&Tangent, &pTang[nTangStride*v], sizeof(Tangent));
            Vec4sf Binormal; 
            memcpy(&Binormal, &pBNorm[nBNormStride*v], sizeof(Binormal));





            Vec3 tangent	= Vec3(tPackB2F(Tangent.x), tPackB2F(Tangent.y), tPackB2F(Tangent.z)); float tw = tPackB2F(Binormal.w);
            Vec3 binormal	= Vec3(tPackB2F(Binormal.x), tPackB2F(Binormal.y), tPackB2F(Binormal.z)); float bw = tPackB2F(Binormal.w);


            tangent.Normalize(); 
            binormal.Normalize(); 
            vNorm = tangent.Cross(binormal);
						vNorm*=tw;
          }

          vNorm.Normalize();

          vert.normal.bcolor[0] = (byte)(vNorm[0] * 127.5f + 128.0f);
          vert.normal.bcolor[1] = (byte)(vNorm[1] * 127.5f + 128.0f);
          vert.normal.bcolor[2] = (byte)(vNorm[2] * 127.5f + 128.0f);
          vert.normal.bcolor[3] = 255;
          SwapEndian(vert.normal.dcolor, eLittleEndian);

          uint8 ucSurfaceTypeID = pTerrain->GetSurfaceTypeID((int)vert.xyz.x, (int)vert.xyz.y, nSID);
          if(ucSurfaceTypeID == STYPE_HOLE)
          { // in case of hole - try to find some valid surface type around
            for(int ii=-nStep; ii<=nStep && (ucSurfaceTypeID == STYPE_HOLE); ii+=nStep)
              for(int j=-nStep; j<=nStep && (ucSurfaceTypeID == STYPE_HOLE); j+=nStep)
                ucSurfaceTypeID = pTerrain->GetSurfaceTypeID((int)vert.xyz.x+ii, (int)vert.xyz.y+j, nSID);
          }

          ucSurfaceTypeID = min(ucSurfaceTypeID, MAX_SURFACE_TYPES_COUNT-1);

          vert.color.bcolor[0] = 255;
          vert.color.bcolor[1] = ucSurfaceTypeID;
          vert.color.bcolor[2] = 255;
          vert.color.bcolor[3] = 255;
          SwapEndian(vert.color.dcolor, eLittleEndian);

          m_pUpdateTerrainTempData->m_lstTmpVertArray.Add(vert);
        }
      }
    }
  }
}

SPU_NO_INLINE void CTerrainNode::AddIndexAliased(int _x, int _y, int _step, int nSectorSize, PodArray<CTerrainNode*> * plstNeighbourSectors, CStripsInfo * pArrayInfo)
{
	int nAliasingX=1, nAliasingY=1;
	int nShiftX=0, nShiftY=0;

	IF(_x && _x<nSectorSize && plstNeighbourSectors, true)
	{
		IF(_y==0, false)
		{
			if(CTerrainNode * pNode = GetTerrain()->GetSecInfo(m_nOriginX+_x, m_nOriginY+_y-_step, m_nSID))
			{
        int nAreaMML = pNode->GetAreaLOD();
				if(nAreaMML != MML_NOT_SET)
				{
					nAliasingX = CTerrain::GetHeightMapUnitSize()*(1<<nAreaMML);
					nShiftX = nAliasingX/4;
				}
				if(plstNeighbourSectors->Find(pNode)<0) // todo: cache this list, it is always the same
					plstNeighbourSectors->Add(pNode);
			}
		}
		else IF(_y==nSectorSize, false)
		{
			if(CTerrainNode * pNode = GetTerrain()->GetSecInfo(m_nOriginX+_x, m_nOriginY+_y+_step, m_nSID))
			{
        int nAreaMML = pNode->GetAreaLOD();
				if(nAreaMML != MML_NOT_SET)
				{
					nAliasingX = CTerrain::GetHeightMapUnitSize()*(1<<nAreaMML);
					nShiftX = nAliasingX/2;
				}
				if(plstNeighbourSectors->Find(pNode)<0)
					plstNeighbourSectors->Add(pNode);
			}
		}
	}

	IF(_y && _y<nSectorSize && plstNeighbourSectors, true)
	{
		IF(_x==0 ,false)
		{
			if(CTerrainNode * pNode = GetTerrain()->GetSecInfo(m_nOriginX+_x-_step, m_nOriginY+_y, m_nSID))
			{
        int nAreaMML = pNode->GetAreaLOD();
				if(nAreaMML != MML_NOT_SET)
				{
					nAliasingY = CTerrain::GetHeightMapUnitSize()*(1<<nAreaMML);
					nShiftY = nAliasingY/4;
				}
				if(plstNeighbourSectors->Find(pNode)<0)
					plstNeighbourSectors->Add(pNode);
			}
		}
		else IF(_x==nSectorSize,false)
		{
			if(CTerrainNode * pNode = GetTerrain()->GetSecInfo(m_nOriginX+_x+_step, m_nOriginY+_y, m_nSID))
			{
        int nAreaMML = pNode->GetAreaLOD();
				if(nAreaMML != MML_NOT_SET)
				{
					nAliasingY = CTerrain::GetHeightMapUnitSize()*(1<<nAreaMML);
					nShiftY = nAliasingY/2;
				}
				if(plstNeighbourSectors->Find(pNode)<0)
					plstNeighbourSectors->Add(pNode);
			}
		}
	}

	int XA = nAliasingX ? (_x+nShiftX)/nAliasingX*nAliasingX : _x;
	int YA = nAliasingY ? (_y+nShiftY)/nAliasingY*nAliasingY : _y;

	assert(XA>=0 && XA<=nSectorSize);
	assert(YA>=0 && YA<=nSectorSize);

	pArrayInfo->AddIndex(XA,YA,_step,nSectorSize);
}


void CTerrainNode::BuildIndices(CStripsInfo & si, PodArray<CTerrainNode*> * pNeighbourSectors, bool bSafetyBorder)
{
  FUNCTION_PROFILER_3DENGINE;

  int nStep = (1<<m_cNewGeomMML)*CTerrain::GetHeightMapUnitSize();
  int nSectorSize = CTerrain::GetSectorSize()<<m_nTreeLevel;
  STerrainNodeLeafData * pLeafData = GetLeafData();

  int nSafetyBorder = bSafetyBorder ? nStep : 0;

  int nSectorSizeSB = nSectorSize+nSafetyBorder*2;

  si.Clear();

  // add non borders
  for( int x=0; x<nSectorSizeSB; x+=nStep )
  {   
    for( int y=0; y<nSectorSizeSB; y+=nStep )
    {
      bool bBorder = x<=0 || y<=0 || x>=(nSectorSizeSB-nStep) || y>=(nSectorSizeSB-nStep);
      if(bBorder)
        continue;

      if(!m_bHasHoles || m_cNewGeomMML || !GetTerrain()->GetHole(m_nOriginX+x,m_nOriginY+y, m_nSID))
      {
        AddIndexAliased(x       ,y        ,nStep,nSectorSizeSB,pNeighbourSectors,&si);
        AddIndexAliased(x+nStep ,y        ,nStep,nSectorSizeSB,pNeighbourSectors,&si);
        AddIndexAliased(x       ,y+nStep  ,nStep,nSectorSizeSB,pNeighbourSectors,&si);

        AddIndexAliased(x+nStep ,y        ,nStep,nSectorSizeSB,pNeighbourSectors,&si);
        AddIndexAliased(x+nStep ,y+nStep  ,nStep,nSectorSizeSB,pNeighbourSectors,&si);
        AddIndexAliased(x       ,y+nStep  ,nStep,nSectorSizeSB,pNeighbourSectors,&si);
      }
    }
  }

  si.nNonBorderIndicesCount = si.idx_array.Count();

  // add borders
  for( int x=0; x<nSectorSizeSB; x+=nStep )
  {   
    for( int y=0; y<nSectorSizeSB; y+=nStep )
    {
      bool bBorder = x<=0 || y<=0 || x>=(nSectorSizeSB-nStep) || y>=(nSectorSizeSB-nStep);
      if(!bBorder)
        continue;

      if(!m_bHasHoles || m_cNewGeomMML || !GetTerrain()->GetHole(m_nOriginX+x,m_nOriginY+y, m_nSID))
      {
        AddIndexAliased(x       ,y        ,nStep,nSectorSizeSB,pNeighbourSectors,&si);
        AddIndexAliased(x+nStep ,y        ,nStep,nSectorSizeSB,pNeighbourSectors,&si);
        AddIndexAliased(x       ,y+nStep  ,nStep,nSectorSizeSB,pNeighbourSectors,&si);

        AddIndexAliased(x+nStep ,y        ,nStep,nSectorSizeSB,pNeighbourSectors,&si);
        AddIndexAliased(x+nStep ,y+nStep  ,nStep,nSectorSizeSB,pNeighbourSectors,&si);
        AddIndexAliased(x       ,y+nStep  ,nStep,nSectorSizeSB,pNeighbourSectors,&si);
      }
    }
  }

	// include some of brushes into terrain mesh, for example for nice golf cup
	if(m_nTreeLevel == 0)
  { 
    PodArray<IRenderNode*> lstBrushes; lstBrushes.Clear();
    AABB boxWS = GetBBox();

    if(Get3DEngine()->m_pObjectsTree[m_nSID])
      Get3DEngine()->m_pObjectsTree[m_nSID]->GetObjectsByType(lstBrushes, eERType_Brush, &boxWS, boxWS.GetRadius()*2.f);

    int nSectorSizeUnits = (CTerrain::GetSectorSize()<<m_nTreeLevel) / nStep;

    int nVertOffset = (nSectorSizeUnits+1)*(nSectorSizeUnits+1);

    for(int i=0; i<lstBrushes.Count(); i++)
    {
      if(lstBrushes[i]->GetIntegrationType() != eIT_TerrainMesh && lstBrushes[i]->GetIntegrationType() != eIT_TerrainMeshAligned)
        continue;

      Matrix34A objMat;
      CStatObj * pStatObj = (CStatObj *)lstBrushes[i]->GetEntityStatObj(0,0,&objMat);

      if(IRenderMesh * pRM = pStatObj ? pStatObj->GetRenderMesh() : 0)
      {
        int nIndCount = pRM->GetIndicesCount();
        const uint16 * pIndices = pRM->GetIndexPtr(FSL_READ);
        const int nVertCount = pRM->GetVerticesCount();

        for(int n=0; n<nIndCount; n++)
        {
          int id = pIndices[n] + nVertOffset;
          m_pUpdateTerrainTempData->m_StripsInfo.idx_array.Add(id);
        }

        nVertOffset += nVertCount;
      }
    }
  }
}

// entry point
void CTerrainNode::RenderSector()
{
	FUNCTION_PROFILER_3DENGINE;

	//m_nNodeRenderLastFrameId = GetMainFrameID();

	assert(m_nRenderStackLevel || m_cNewGeomMML<GetTerrain()->m_nUnitsToSectorBitShift);

	STerrainNodeLeafData * pLeafData = GetLeafData();

	// detect if any neighbors switched lod since previous frame
	bool bNeighbourChanged = false;

	if(!IsRenderIntoShadowmap())
  {
	  for(int i=0; i<pLeafData->m_lstNeighbourSectors.Count(); i++)
	  {
      int nNeighbourNewMML = pLeafData->m_lstNeighbourSectors[i]->GetAreaLOD();
		  if( nNeighbourNewMML != pLeafData->m_lstNeighbourLods[i] && (nNeighbourNewMML>m_cNewGeomMML || pLeafData->m_lstNeighbourLods[i]>m_cNewGeomMML) )
		  {
			  bNeighbourChanged = true;
			  break;
		  }
	  }
  }

	IRenderMesh * & pRenderMesh = GetLeafData()->m_pRenderMesh;

	bool bDetailLayersReady = IsRenderIntoShadowmap() ||
		!m_lstSurfaceTypeInfo.Count() ||
		m_lstSurfaceTypeInfo[0].HasRM() || 
		!GetCVars()->e_TerrainDetailMaterials ||
		!m_lstSurfaceTypeInfo[0].pSurfaceType ||
		!m_lstSurfaceTypeInfo[0].pSurfaceType->HasMaterial();

	// just draw if everything is up to date already
	if(pRenderMesh && GetCVars()->e_TerrainDrawThisSectorOnly<2 && bDetailLayersReady)
	{
		if( m_nRenderStackLevel || ( m_cCurrGeomMML == m_cNewGeomMML && !bNeighbourChanged ))
		{ 
			DrawArray(); 
			return; 
		}
	}

	if(m_nRenderStackLevel)
		if(pRenderMesh)
			return;

	m_pUpdateTerrainTempData = gTerrainUpdateDispatcher.GetTempData();

	// Preallocate enough temp memory to prevent reallocations
	int nStep = (1<<m_cNewGeomMML)*CTerrain::GetHeightMapUnitSize();
	int nSectorSize = CTerrain::GetSectorSize()<<m_nTreeLevel;  

	int	nNumVerts = ((nSectorSize/nStep)+1)*((nSectorSize/nStep)+1);		
	int nNumIdx = (nSectorSize/nStep)*(nSectorSize/nStep) * 6;

	m_pUpdateTerrainTempData->m_StripsInfo.idx_array.PreAllocate(nNumIdx);
	m_pUpdateTerrainTempData->m_lstTmpVertArray.PreAllocate(nNumVerts);
















	{
		BuildIndices_Wrapper();
		BuildVertices_Wrapper();
	}

	// make sure the Update only runs asynchron if called from DrawVisibleSector(where m_nRenderStackLevel is 0)
	// or else we could end up with an unsynced TerrainNode, leading to duplicates and a crash later
	if( m_nRenderStackLevel == 0)
	{
		gTerrainUpdateDispatcher.AddJob(this);
	}
	else
	{
		RenderSectorUpdate_Finish();
	}

}
void CTerrainNode::RenderSectorUpdate_Finish()
{
	assert( m_pUpdateTerrainTempData != NULL );












	// if an allocation failed, destroy the temp data and return
	if( m_pUpdateTerrainTempData->m_StripsInfo.idx_array.MemorySize() == 0 || 
			m_pUpdateTerrainTempData->m_lstTmpVertArray.MemorySize() == 0 )
	{
		gTerrainUpdateDispatcher.DestroyTempData( m_pUpdateTerrainTempData );
		return;
	}

	IRenderMesh * & pRenderMesh = GetLeafData()->m_pRenderMesh;
	STerrainNodeLeafData * pLeafData = GetLeafData();

	UpdateRenderMesh(&m_pUpdateTerrainTempData->m_StripsInfo, !m_bUpdateOnlyBorders);

	m_cCurrGeomMML = m_cNewGeomMML;

	// update detail layers indices
	if(GetCVars()->e_TerrainDetailMaterials ) //&& (!m_nRenderStackLevel || GetCVars()->e_TerrainDetailMaterials))
	{
		// build all indices
    GenerateIndicesForAllSurfaces(pRenderMesh, m_bUpdateOnlyBorders, pLeafData->m_arrpNonBorderIdxNum, m_pUpdateTerrainTempData->m_StripsInfo.nNonBorderIndicesCount, 0, m_nSID);

    static PodArray<SSurfaceType *> lstReadyTypes; lstReadyTypes.Clear(); // protection from duplications in palette of types

		uint8 szProj[] = "XYZ";
		for(int i=0; i<m_lstSurfaceTypeInfo.Count(); i++)
		{
      if(SSurfaceType * pSurfaceType = m_lstSurfaceTypeInfo[i].pSurfaceType)
      {
        if(lstReadyTypes.Find(pSurfaceType)<0)
        {
          bool b3D = pSurfaceType->IsMaterial3D();
          for(int p=0; p<3; p++)
          {
            if(b3D || pSurfaceType->GetMaterialOfProjection(szProj[p]))
            {
              int nProjId = b3D ? p : 3;
              PodArray<unsigned short> & lstIndices = m_arrIndices[pSurfaceType->ucThisSurfaceTypeId][nProjId];

							if(m_lstSurfaceTypeInfo[i].arrpRM[p] && (lstIndices.Count() != m_lstSurfaceTypeInfo[i].arrpRM[p]->GetIndicesCount()))
							{
								GetRenderer()->DeleteRenderMesh(m_lstSurfaceTypeInfo[i].arrpRM[p]);
								m_lstSurfaceTypeInfo[i].arrpRM[p] = NULL;
							}

							if(lstIndices.Count())
								UpdateSurfaceRenderMeshes(pRenderMesh, pSurfaceType, m_lstSurfaceTypeInfo[i].arrpRM[p], p, lstIndices, "TerrainMaterialLayer", m_bUpdateOnlyBorders, pLeafData->m_arrpNonBorderIdxNum[pSurfaceType->ucThisSurfaceTypeId][nProjId]);             

              if(!b3D)
                break;
            }
          }
          lstReadyTypes.Add(pSurfaceType);
        }
      }
		}
	}

	// free temp memory from indice and vertice generation
	gTerrainUpdateDispatcher.DestroyTempData( m_pUpdateTerrainTempData );

	DrawArray();
}

// function used as SPU entry point for BuildIndices
#if !defined(CRCYG_CM)
SPU_ENTRY(Terrain_BuildIndices)
#endif
void CTerrainNode::BuildIndices_Wrapper()
{
	// don't try to create terrain data if an allocation failed
	if( m_pUpdateTerrainTempData->m_StripsInfo.idx_array.MemorySize() == 0 ) return;






	STerrainNodeLeafData * pLeafData = GetLeafData();

	pLeafData->m_lstNeighbourSectors.Clear();
	pLeafData->m_lstNeighbourLods.Clear();

	// build indices and fill neighbor info
	BuildIndices(m_pUpdateTerrainTempData->m_StripsInfo, &pLeafData->m_lstNeighbourSectors, false);

	// remember neighbor LOD's
	for(int i=0; i<pLeafData->m_lstNeighbourSectors.Count(); i++)
		pLeafData->m_lstNeighbourLods.Add(pLeafData->m_lstNeighbourSectors[i]->GetAreaLOD());






}

// function used as SPU entry point for BuildVertices
#if !defined(CRCYG_CM)
SPU_ENTRY(Terrain_BuildVertices)
#endif
void CTerrainNode::BuildVertices_Wrapper()
{
	// don't try to create terrain data if an allocation failed
	if( m_pUpdateTerrainTempData->m_lstTmpVertArray.MemorySize() == 0 )
		return;






	IRenderMesh * & pRenderMesh = GetLeafData()->m_pRenderMesh;

	int nOldSrcCount = -1;
	if (pRenderMesh)
		nOldSrcCount = pRenderMesh->GetIndicesCount();

	int nStep = (1<<m_cNewGeomMML)*CTerrain::GetHeightMapUnitSize();
	int nSectorSize = CTerrain::GetSectorSize()<<m_nTreeLevel;
	assert(nStep && nStep<=nSectorSize);
	if(nStep > nSectorSize)
		nStep = nSectorSize;
	
	// update vertices if needed
	int nVertsNumRequired = (nSectorSize/nStep)*(nSectorSize/nStep)+(nSectorSize/nStep)+(nSectorSize/nStep)+1;
	if(!pRenderMesh || m_cCurrGeomMML != m_cNewGeomMML || nVertsNumRequired != pRenderMesh->GetVerticesCount() || m_bEditor || !m_cNewGeomMML)
	{



		if(m_pUpdateTerrainTempData->m_StripsInfo.idx_array.Count())
			BuildVertices(nStep, false);
		else
			m_pUpdateTerrainTempData->m_lstTmpVertArray.Clear();   		


		m_bUpdateOnlyBorders = false;
	}
	else
	{
		m_bUpdateOnlyBorders = true;
	}






}

/*void CTerrainNode::GenerateIndicesForQuad(IRenderMesh * pRM, Vec3 vBoxMin, Vec3 vBoxMax, PodArray<uint16> & dstIndices)
{
	dstIndices.Clear();

	int nSrcCount=0;
	uint16 * pSrcInds = pRM->GetIndices(&nSrcCount);

	int nPosStride=0;
	byte * pPos = pRM->GetStridedPosPtr(nPosStride);

	for(int i=0; i<(*pRM->GetChunks()).Count(); i++)
	{
		if (!(*pRM->GetChunks())[i].pRE)
			continue;

		CRenderChunk * pChunk = &(*pRM->GetChunks())[i];

		for (int j=pChunk->nFirstIndexId; (j+2)<pChunk->nNumIndices+pChunk->nFirstIndexId; j+=3)
		{
			int nIndex0 = pSrcInds[j+0];
			int nIndex1 = pSrcInds[j+1];
			int nIndex2 = pSrcInds[j+2];

			assert(nIndex0>=0 && nIndex0<pRM->GetVertCount());
			assert(nIndex1>=0 && nIndex1<pRM->GetVertCount());
			assert(nIndex2>=0 && nIndex2<pRM->GetVertCount());

			Vec3 & vPos0 = *(Vec3*)&pPos[nIndex0*nPosStride];
			Vec3 & vPos1 = *(Vec3*)&pPos[nIndex1*nPosStride];
			Vec3 & vPos2 = *(Vec3*)&pPos[nIndex2*nPosStride];

			if(nIndex0 != nIndex1 && nIndex1 != nIndex2 && nIndex2 != nIndex0)
				if(Overlap::AABB_Triangle(AABB(vBoxMin,vBoxMax),vPos0,vPos1,vPos2))
			{
				if(j&1)
				{
					dstIndices.Add(nIndex0);
					dstIndices.Add(nIndex2);
					dstIndices.Add(nIndex1);
				}
				else
				{
					dstIndices.Add(nIndex0);
					dstIndices.Add(nIndex1);
					dstIndices.Add(nIndex2);
				}
			}
		}
	}
}*/

int GetVecProjectId(const Vec3 & vNorm)
{
	Vec3 vNormAbs = vNorm.abs();

	int nOpenId=0;

	if(vNormAbs.x>=vNormAbs.y && vNormAbs.x>=vNormAbs.z)
		nOpenId = 0;
	else if(vNormAbs.y>=vNormAbs.x && vNormAbs.y>=vNormAbs.z)
		nOpenId = 1;
	else 
		nOpenId = 2;

	return nOpenId;
}

void CTerrainNode::GenerateIndicesForAllSurfaces(IRenderMesh * pRM, bool bOnlyBorder, short arrpNonBorderIdxNum[MAX_SURFACE_TYPES_COUNT][4], int nBorderStartIndex, SSurfaceTypeInfo * pSurfaceTypeInfos, int nSID)
{
	FUNCTION_PROFILER_3DENGINE;

	byte arrMat3DFlag[MAX_SURFACE_TYPES_COUNT];

	for(int s=0; s<MAX_SURFACE_TYPES_COUNT; s++)
	{
		for(int p=0; p<4; p++)
    {
      m_arrIndices[s][p].Clear();

      if(!bOnlyBorder)
        arrpNonBorderIdxNum[s][p] = -1;
    }

    if(pSurfaceTypeInfos)
    {
      if(pSurfaceTypeInfos[s].pSurfaceType)
        arrMat3DFlag[s] = pSurfaceTypeInfos[s].pSurfaceType->IsMaterial3D();
    }
    else
      arrMat3DFlag[s] = GetTerrain()->GetSurfaceTypes(nSID)[s].IsMaterial3D();
	}

  // Restore system buffer
	int nSrcCount = pRM->GetIndicesCount();
  if (!nSrcCount)
    return;
	uint16 *pSrcInds = pRM->GetIndexPtr(FSL_READ);

	int nColorStride=0;
	byte *pColor = pRM->GetColorPtr(nColorStride, FSL_READ);

	int nNormStride=0;
	byte *pNormB = pRM->GetNormPtr(nNormStride, FSL_READ);

	assert(pColor);

  if (!pColor || !pNormB)
    return;

//	for(int i=0; pColor && i<pRM->GetChunks()->Count(); i++)
	{
	//	CRenderChunk * pChunk = pRM->GetChunks()->Get(i);

    int nVertCount = pRM->GetVerticesCount();

    for (int j=(bOnlyBorder ? nBorderStartIndex : 0); j<nSrcCount; j+=3)
		{
      uint16 arrTriangle[3] = { pSrcInds[j+0], pSrcInds[j+1], pSrcInds[j+2] };

			// ignore invalid triangles
			if(arrTriangle[0] == arrTriangle[1] || arrTriangle[1] == arrTriangle[2] || arrTriangle[2] == arrTriangle[0])
				continue;

      assert(arrTriangle[0]<nVertCount && arrTriangle[1]<nVertCount && arrTriangle[2]<nVertCount);

      if(arrTriangle[0] >= nVertCount || arrTriangle[1] >= nVertCount || arrTriangle[2] >= nVertCount)
        continue;

			UCol & Color0 = *(UCol*)&pColor[arrTriangle[0]*nColorStride];
			UCol & Color1 = *(UCol*)&pColor[arrTriangle[1]*nColorStride];
			UCol & Color2 = *(UCol*)&pColor[arrTriangle[2]*nColorStride];




  #define BYTE_ID(_val) (_val)


			uint32 nVertSurfId0 = Color0.bcolor[BYTE_ID(1)]&127;
			uint32 nVertSurfId1 = Color1.bcolor[BYTE_ID(1)]&127;
			uint32 nVertSurfId2 = Color2.bcolor[BYTE_ID(1)]&127;
/*
			assert(nVertSurfId0<MAX_SURFACE_TYPES_COUNT);
			assert(nVertSurfId1<MAX_SURFACE_TYPES_COUNT);
			assert(nVertSurfId2<MAX_SURFACE_TYPES_COUNT);
*/
      nVertSurfId0 = CLAMP(nVertSurfId0, 0, MAX_SURFACE_TYPES_COUNT-1);
      nVertSurfId1 = CLAMP(nVertSurfId1, 0, MAX_SURFACE_TYPES_COUNT-1);
      nVertSurfId2 = CLAMP(nVertSurfId2, 0, MAX_SURFACE_TYPES_COUNT-1);

			static PodArray<int> lstSurfTypes; lstSurfTypes.Clear();
			lstSurfTypes.Add(nVertSurfId0);
			if(nVertSurfId1 != nVertSurfId0)
				lstSurfTypes.Add(nVertSurfId1);
			if(nVertSurfId2 != nVertSurfId0 && nVertSurfId2 != nVertSurfId1)
				lstSurfTypes.Add(nVertSurfId2);

			static PodArray<int> lstProjIds; lstProjIds.Clear();

      byte *pNorm;
      Vec3 vNorm;

			// if there is 3d materials - analyze normals
			if(arrMat3DFlag[nVertSurfId0])
			{
        pNorm = &pNormB[arrTriangle[0]*nNormStride];
				vNorm.Set(((float)pNorm[BYTE_ID(0)]-127.5f), ((float)pNorm[BYTE_ID(1)]-127.5f), ((float)pNorm[BYTE_ID(2)]-127.5f));
				int nProjId0 = GetVecProjectId(vNorm);
				if(lstProjIds.Find(nProjId0)<0)
					lstProjIds.Add(nProjId0);
			}

			if(arrMat3DFlag[nVertSurfId1])
			{
        pNorm = &pNormB[arrTriangle[1]*nNormStride];
				vNorm.Set(((float)pNorm[BYTE_ID(0)]-127.5f), ((float)pNorm[BYTE_ID(1)]-127.5f), ((float)pNorm[BYTE_ID(2)]-127.5f));
				int nProjId1 = GetVecProjectId(vNorm);
				if(lstProjIds.Find(nProjId1)<0)
					lstProjIds.Add(nProjId1);
			}

			if(arrMat3DFlag[nVertSurfId2])
			{
        pNorm = &pNormB[arrTriangle[2]*nNormStride];
        vNorm.Set(((float)pNorm[BYTE_ID(0)]-127.5f), ((float)pNorm[BYTE_ID(1)]-127.5f), ((float)pNorm[BYTE_ID(2)]-127.5f));
				int nProjId2 = GetVecProjectId(vNorm);
				if(lstProjIds.Find(nProjId2)<0)
					lstProjIds.Add(nProjId2);
			}

			// if not 3d materials found
			if(!arrMat3DFlag[nVertSurfId0] || !arrMat3DFlag[nVertSurfId1] || !arrMat3DFlag[nVertSurfId2])
				lstProjIds.Add(3);

			for(int s=0; s<lstSurfTypes.Count(); s++)
			{
				for(int p=0; p<lstProjIds.Count(); p++)
				{
          assert(lstSurfTypes[s]>=0 && lstSurfTypes[s]<MAX_SURFACE_TYPES_COUNT);
          assert(lstProjIds[p]>=0 && lstProjIds[p]<4);
					PodArray<uint16> & rList = m_arrIndices[lstSurfTypes[s]][lstProjIds[p]];

          if(!bOnlyBorder && j>=nBorderStartIndex && arrpNonBorderIdxNum[lstSurfTypes[s]][lstProjIds[p]]<0)
            arrpNonBorderIdxNum[lstSurfTypes[s]][lstProjIds[p]] = rList.Count();

          rList.AddList(arrTriangle, 3);
        }
			}
		}
	}
  pRM->UnlockStream(VSF_GENERAL);
}

void CTerrainNode::UpdateSurfaceRenderMeshes(IRenderMesh * pSrcRM, SSurfaceType * pSurface, IRenderMesh * & pMatRM, int nProjectionId, 
                                             PodArray<unsigned short> & lstIndices, const char * szComment, bool bUpdateOnlyBorders, int nNonBorderIndicesCount)
{
	FUNCTION_PROFILER_3DENGINE;

//	static PodArray<unsigned short> lstIndices; lstIndices.Clear();
	//GenerateIndicesForSurface(pSrcRM, pSurface->ucThisSurfaceTypeId, lstIndices, pSurface->IsMaterial3D() ? nProjectionId : -1);

	if(!pMatRM)
	{

		pMatRM = GetRenderer()->CreateRenderMeshInitialized(
			NULL, 0, eVF_P3S_N4B_C4B_T2S, NULL, 0,
			R_PRIMV_TRIANGLES, szComment, szComment, eRMT_Static, 1, 0, NULL, NULL, false, false);
	}

	uint8 szProj[] = "XYZ";

	pMatRM->SetVertexContainer(pSrcRM);

  assert(1||nNonBorderIndicesCount>=0);

  if(bUpdateOnlyBorders)
  {
    int nOldIndexCount = pMatRM->GetIndicesCount();
    uint16 *pIndicesOld = pMatRM->GetIndexPtr(FSL_READ);

    if (nNonBorderIndicesCount<0 || !nOldIndexCount)
      return; // no borders involved

    int nIndexCountNew = nNonBorderIndicesCount + lstIndices.Count();

    assert(true || nIndexCountNew>=lstIndices.Count());

    uint16 * pIndicesNew = new uint16[nIndexCountNew];

    memcpy(pIndicesNew, pIndicesOld, sizeof(uint16)*nNonBorderIndicesCount);

    memcpy(pIndicesNew+nNonBorderIndicesCount, lstIndices.GetElements(), sizeof(uint16)*lstIndices.Count());

    pMatRM->UpdateIndices(pIndicesNew, nIndexCountNew);
    pMatRM->SetChunk(pSurface->GetMaterialOfProjection(szProj[nProjectionId]), 0, pSrcRM->GetVerticesCount(), 0, nIndexCountNew, 1.0f);

    delete [] pIndicesNew;
  }
  else
  {
    pMatRM->UpdateIndices(lstIndices.GetElements(), lstIndices.Count());
	  pMatRM->SetChunk(pSurface->GetMaterialOfProjection(szProj[nProjectionId]), 0, pSrcRM->GetVerticesCount(), 0, lstIndices.Count(), 1.0f);
  }

	//pMatRM->SetUpdateFrame(pSrcRM->GetUpdateFrame());

	assert(nProjectionId>=0 && nProjectionId<3);
	float * pParams = pSurface->arrRECustomData[nProjectionId];
	SetupTexGenParams(pSurface,pParams,szProj[nProjectionId],true);
	pMatRM->SetMaterial(pSurface->GetMaterialOfProjection(szProj[nProjectionId]));

	// set surface type
	if(pSurface->IsMaterial3D())
	{
		pParams[ 8] = (nProjectionId == 0);
		pParams[ 9] = (nProjectionId == 1);
		pParams[10] = (nProjectionId == 2);
	}
	else
	{
		pParams[ 8] = 1.f;
		pParams[ 9] = 1.f;
		pParams[10] = 1.f;
	}

	pParams[11] = pSurface->ucThisSurfaceTypeId;

	// set texgen offset
	Vec3 vCamPos = GetCamera().GetPosition();

	pParams[12] = pParams[13] = pParams[14] = pParams[15] = 0;

	// for diffuse
	if(IMaterial * pMat = pSurface->GetMaterialOfProjection(szProj[nProjectionId]))
		if(pMat->GetShaderItem().m_pShaderResources)
	{
		if(	SEfResTexture * pTex = pMat->GetShaderItem().m_pShaderResources->GetTexture(EFTT_DIFFUSE))
		{
			float fScaleX = pTex->m_bUTile ? pTex->m_TexModificator.m_Tiling[0]*pSurface->fScale : 1.f;
			float fScaleY = pTex->m_bVTile ? pTex->m_TexModificator.m_Tiling[1]*pSurface->fScale : 1.f;

			pParams[12] = int(vCamPos.x*fScaleX)/fScaleX;
			pParams[13] = int(vCamPos.y*fScaleY)/fScaleY;
		}

		if(	SEfResTexture * pTex = pMat->GetShaderItem().m_pShaderResources->GetTexture(EFTT_BUMP))
		{
			float fScaleX = pTex->m_bUTile ? pTex->m_TexModificator.m_Tiling[0]*pSurface->fScale : 1.f;
			float fScaleY = pTex->m_bVTile ? pTex->m_TexModificator.m_Tiling[1]*pSurface->fScale : 1.f;

			pParams[14] = int(vCamPos.x*fScaleX)/fScaleX;
			pParams[15] = int(vCamPos.y*fScaleY)/fScaleY;
		}
	}

	assert(8+8<=ARR_TEX_OFFSETS_SIZE_DET_MAT);

	if (pMatRM->GetChunks().Count() && pMatRM->GetChunks()[0].pRE)
		pMatRM->GetChunks()[0].pRE->m_CustomData = pParams;

	Vec3 vMin, vMax;
	pSrcRM->GetBBox(vMin, vMax);
	pMatRM->SetBBox(vMin, vMax);

//  if( pMatRM->GetSysIndicesCount() && pMatRM->GetSysIndicesCount() < pSrcRM->GetSysIndicesCount() / 8 )
  //  pMatRM->UpdateBBoxFromMesh();
}

STerrainNodeLeafData::~STerrainNodeLeafData()
{
	gEnv->pRenderer->DeleteRenderMesh(m_pRenderMesh);
}

/*void CTerrainNode::OnLightMapDeleted(IDynTexture * pLightMap)
{
	if(m_nTexSet.pLightMap == pLightMap)
		m_nTexSet.pLightMap = NULL;

	if(m_arrChilds[0])
		for(int i=0; i<4; i++)
			m_arrChilds[i]->OnLightMapDeleted(pLightMap);
}*/

#undef USE_SPU
