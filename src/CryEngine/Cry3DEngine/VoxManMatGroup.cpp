//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:voxman.cpp
//  voxel tecnology researh
//
//	History:
//	-:Created by Vladimir Kajalin
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "VoxMan.h"
#include "IndexedMesh.h"
#include "MeshCompiler/MeshCompiler.h"
#include "CGF/ReadOnlyChunkFile.h"

CVoxelMesh::CVoxelMesh(SSurfaceType ** ppSurfacesPalette)
{
	memset(this,0,sizeof(*this));

	for(int nLod=0; nLod<VOX_MAX_LODS_NUM; nLod++)
		for(int nSurf=0; nSurf<VOX_MAX_SURF_TYPES_NUM; nSurf++)
			m_arrSurfaceTypeInfo[nLod][nSurf].pSurfaceType = ppSurfacesPalette[nSurf];
}

CVoxelMesh::~CVoxelMesh()
{
	ResetRenderMeshs();
}

void CVoxelMesh::ResetRenderMeshs()
{
	for(int nLod=0; nLod<VOX_MAX_LODS_NUM; nLod++)
	{
		for(int l=VOX_MAX_SURF_TYPES_NUM-1; l>=0; l--)
			m_arrSurfaceTypeInfo[nLod][l].DeleteRenderMeshes(GetRenderer());

		GetRenderer()->DeleteRenderMesh(m_arrpRM_Ambient[nLod]);
		m_arrpRM_Ambient[nLod] = NULL;
	}
}

void CVoxelMesh::RenderAmbPass(int nLod, int nSort, int nAW, CRenderObject * pObj)
{
	// ambient pass
	if(m_arrpRM_Ambient[nLod])
		m_arrpRM_Ambient[nLod]->AddRenderElements(pObj,nSort,nAW,NULL);
}

void CVoxelMesh::RenderLightPasses(int nLod, int nDLMask, CVoxelObject * pVoxArea, bool bInShadow, float fDistance, const SRendParams & rParams)
{
	// detail light pass
	CRenderObject * pObj = NULL;

	for(int nSType=0; nSType<VOX_MAX_SURF_TYPES_NUM; nSType++)
		if(m_arrSurfaceTypeInfo[nLod][nSType].pSurfaceType)
			if(m_arrSurfaceTypeInfo[nLod][nSType].pSurfaceType->HasMaterial() && m_arrSurfaceTypeInfo[nLod][nSType].HasRM())
			{
        if(!pObj)
        {
          pObj = GetIdentityCRenderObject();
          if (!pObj)
            return;
          pObj->m_DynLMMask[m_nRenderThreadListID] = nDLMask;
          pObj->m_ObjFlags |= (rParams.dwFObjFlags & FOB_SELECTED) | (FOB_NO_FOG);
          //				pObj->m_pShadowCasters = (pShadowCasters && pShadowCasters->Count()) ? pShadowCasters : NULL;
          pObj->m_II.m_AmbColor = rParams.AmbientColor;
          pObj->m_II.m_Matrix = *rParams.pMatrix;
          pObj->m_ObjFlags |= FOB_TRANS_MASK;
          if (bInShadow)
            pObj->m_ObjFlags |= FOB_INSHADOW;
          pObj->m_fDistance = rParams.fDistance;
          pObj->m_nSort = fastround_positive(rParams.fDistance*2.0f);

          if(rParams.pTerrainTexInfo && GetTerrain()->IsAmbientOcclusionEnabled())
          { // provide terrain texture info
            pObj->m_nTextureID  = rParams.pTerrainTexInfo->nTex0;
            //pObj->m_nTextureID1 = rParams.pTerrainTexInfo->nTex1;
            SRenderObjData *pOD = GetRenderer()->EF_GetObjData(pObj, true);
            pOD->m_fTempVars[0] = rParams.pTerrainTexInfo->fTexOffsetX;
            pOD->m_fTempVars[1] = rParams.pTerrainTexInfo->fTexOffsetY;
            pOD->m_fTempVars[2] = rParams.pTerrainTexInfo->fTexScale;
            pOD->m_fTempVars[3] = rParams.pTerrainTexInfo->fTerrainMinZ;
            pOD->m_fTempVars[4] = rParams.pTerrainTexInfo->fTerrainMaxZ;
            pObj->m_ObjFlags |= FOB_AMBIENT_OCCLUSION;
          }
        }

        if(IMaterial * pMat = m_arrSurfaceTypeInfo[nLod][nSType].pSurfaceType->pLayerMat)
        {
					GetObjManager()->PrecacheMaterial(pMat,pObj->m_fDistance,NULL);
          Get3DEngine()->InitMaterialDefautMappingAxis(pMat);
        }

			  uint8 szProj[] = "XYZ";
			  for(int p=0; p<3; p++)
			  {
				  if(IMaterial * pSubMat = m_arrSurfaceTypeInfo[nLod][nSType].pSurfaceType->GetMaterialOfProjection(szProj[p]))
					  if(fDistance<m_arrSurfaceTypeInfo[nLod][nSType].pSurfaceType->GetMaxMaterialDistanceOfProjection(szProj[p]))
              if(IRenderMesh * pRM = m_arrSurfaceTypeInfo[nLod][nSType].arrpRM[p])
						  if(pRM->GetVertexContainer() == m_arrpRM_Ambient[nLod])
                if(pRM->GetIndicesCount())
							    pRM->AddRenderElements(pObj, EFSLIST_TERRAINLAYER, rParams.nAfterWater, pSubMat);
			  }
		}
}

void CVoxelObject::SerializeMeshes(CVoxelObject * pVoxArea, CIndexedMesh * pIndexedMesh)
{
  ResetRenderMeshs();
  
  m_pVoxelVolume->SerializeRenderMeshs(pVoxArea, pIndexedMesh);
}

void CVoxelVolume::SerializeRenderMeshs(CVoxelObject * pVoxArea, CIndexedMesh * pIndexedMesh)
{



	FUNCTION_PROFILER_3DENGINE;

  pVoxArea->ReleaseMemBlocks();

	int nLodsNum = (pVoxArea->m_nFlags&IVOXELOBJECT_FLAG_GENERATE_LODS) ? min(1+GetCVars()->e_VoxelLodsNum,VOX_MAX_LODS_NUM) : 1;
	for(int nLod=0; pIndexedMesh->m_numFaces && nLod<nLodsNum; nLod++)
	{ // make RenderMeshs
		if(pVoxArea->m_nFlags&IVOXELOBJECT_FLAG_GENERATE_LODS || (m_pSrcArea->m_pDataProvider && GetCVars()->e_VoxelLodsNum))
			SimplifyIndexedMesh(pIndexedMesh, nLod);

    pIndexedMesh->InitStreamSize();
    pIndexedMesh->SetSubSetCount(1);
    pIndexedMesh->CalcBBox();

//    if(m_pSrcArea->m_pDataProvider)
  //    pVoxArea->m_pNodeTex = pIndexedMesh->GenerateTexture(GetCVars()->e_VoxTerTexRes, pVoxArea->m_pNodeTex, 
    //  DEF_VOX_VOLUME_SIZE*DEF_VOX_UNIT_SIZE, pVoxArea->GetPos(true), 0, 0);

    if(!m_bEditor)
      Warning( "CVoxelMesh::SerializeRenderMeshs: Stripifying geometry at loading time");

    CMesh mesh;
    mesh.Copy( *pIndexedMesh->GetMesh() );
    mesh_compiler::CMeshCompiler meshCompiler;
    meshCompiler.Compile( mesh, 0 );

    if(!mesh.GetIndexCount())
      break;

    if(m_pSrcArea->m_pDataProvider)
      CIndexedMesh::MarkSharpVertices(0.1f, &mesh);

    if(nLod==0 && GetCVars()->e_VoxelUpdatePhysics)
      pVoxArea->Physicalize(&mesh, NULL);

    mesh.SetFacesCount(0);

    pVoxArea->SerializeMesh( &mesh );
	}

}

void CVoxelMesh::MakeRenderMeshsFromMemBlocks(CVoxelObject * pVoxArea)
{
  FUNCTION_PROFILER_3DENGINE;

  ResetRenderMeshs();

//  Get3DEngine()->CheckMemoryHeap();

  uint32 nLodsNum = (pVoxArea->m_nFlags&IVOXELOBJECT_FLAG_GENERATE_LODS) ? min(1+GetCVars()->e_VoxelLodsNum,VOX_MAX_LODS_NUM) : 1;
  for(uint32 nLod=0; nLod<nLodsNum && nLod<pVoxArea->m_arrMeshesForSerialization.size(); nLod++)
  { // make RenderMeshs
    CMemoryBlock * pSerialized = pVoxArea->m_arrMeshesForSerialization[nLod];
    CMemoryBlock * pTmp = pSerialized;

    // check if was compressed
    if(strncmp((char *)pSerialized->GetData(),"CryTek",6)!=0)
    {
      pTmp = CMemoryBlock::DecompressFromMemBlock(pSerialized, GetSystem());
      if(!pTmp)
      {
        Get3DEngine()->CheckMemoryHeap();
        return Error("CVoxelMesh::MakeRenderMeshsFromMemBlocks: CMemoryBlock::DecompressFromMemBlock failed");
      }
      assert(strncmp((char *)pTmp->GetData(),"CryTek",6)==0);
    }

    std::vector<char> physData;
    CMesh * pLoaded = NULL;
    CReadOnlyChunkFile chunkFile(false);
    CContentCGF * pContent = pVoxArea->LoadFromMemBlock( &chunkFile, pTmp, pLoaded, nLod==0 ? &physData : NULL);

    if(nLod==0 && GetCVars()->e_VoxelUpdatePhysics)
      pVoxArea->Physicalize(pLoaded, &physData);

    CreateRenderMeshFromIndexedMesh(pLoaded, pVoxArea, nLod);

    if(!nLod && pVoxArea->m_pDataProvider)
    {
      if(!pVoxArea->m_pMesh)
        pVoxArea->m_pMesh = new CIndexedMesh();
      pVoxArea->m_pMesh->Copy(*pLoaded);
      pVoxArea->m_pMesh->SetFacesCount(0);
      pVoxArea->m_pMesh->CheckValid();
    }

    DeleteCContentCGF(pContent);

    if(pTmp != pSerialized)
      delete pTmp;
  }

//  Get3DEngine()->CheckMemoryHeap();

  if(m_arrpRM_Ambient[0])
    UpdateVertexHash(m_arrpRM_Ambient[0]);

  memset(m_arrCurrAoRadiusAndScale,0,sizeof(m_arrCurrAoRadiusAndScale));
}

void CVoxelMesh::SetupAmbPassMapping(int nElemCount, float * pHMTexOffsets, const SSectorTextureSet & texSet, bool bSetupSinglePassTexIds)
{
/*	float * pTexOffsets = m_arrLayerTexGenInfo_Ambient;

	// setup base texture texgen
	if(pHMTexOffsets)
		memcpy(pTexOffsets, pHMTexOffsets, sizeof(float)*8);
*/
	// setup detail layers
	/*for(int t=0; t<VOX_MAX_SINGLETPASS_MATS_NUM && t<m_lstOpenMaterials.Count(); t++)
	{
		float * pOutParams = pTexOffsets + 8 + t*8;

		if(&pOutParams[7] >= pTexOffsets+nElemCount)
			break;

		uint8 ucProjAxis = m_lstOpenMaterials[t].pOpenMat->m_ucDefautMappingAxis;
		float fScale = m_lstOpenMaterials[t].pOpenMat->m_fDefautMappingScale;
		float fMaxMatDistanceXY = m_lstOpenMaterials[t].pSurface->fMaxMatDistanceXY;
		float fMaxMatDistanceZ = m_lstOpenMaterials[t].pSurface->fMaxMatDistanceZ;

		// setup projection direction
		if(ucProjAxis == 'X')
		{
			pOutParams[0] = 0;
			pOutParams[1] = fScale;
			pOutParams[2] = 0;
			pOutParams[3] = fMaxMatDistanceXY;

			pOutParams[4] = 0;
			pOutParams[5] = 0;
			pOutParams[6] = fScale;
			pOutParams[7] = 0;
		}
		else if(ucProjAxis =='Y')
		{
			pOutParams[0] = fScale;
			pOutParams[1] = 0;
			pOutParams[2] = 0;
			pOutParams[3] = fMaxMatDistanceXY;

			pOutParams[4] = 0;
			pOutParams[5] = 0;
			pOutParams[6] = fScale;
			pOutParams[7] = 0;
		}
		else // Z
		{
			pOutParams[0] = fScale;
			pOutParams[1] = 0;
			pOutParams[2] = 0;
			pOutParams[3] = fMaxMatDistanceZ;

			pOutParams[4] = 0;
			pOutParams[5] = fScale;
			pOutParams[6] = 0;
			pOutParams[7] = 0;
		}
	}*/

	for(int nLod=0; nLod<VOX_MAX_LODS_NUM; nLod++)
	if(m_arrpRM_Ambient[nLod] && m_arrpRM_Ambient[nLod]->GetVerticesCount())
	{
		IRenderMesh * pRM = m_arrpRM_Ambient[nLod];

		if (pHMTexOffsets)
			pRM->SetREUserData(pHMTexOffsets);

		PodArray<CRenderChunk>& Mats = pRM->GetChunks();
		if (!Mats.Count())
			break;

//		for(int i=TERRAIN_BASE_TEXTURES_NUM; i<MAX_CUSTOM_TEX_BINDS_NUM; i++)
	//		pMats->GetAt(0).pRE->m_CustomTexBind[i] = 0;

    assert(TERRAIN_BASE_TEXTURES_NUM == 2);
		Mats[0].pRE->m_CustomTexBind[0] = texSet.nTex0;
		Mats[0].pRE->m_CustomTexBind[1] = texSet.nTex1;
/*
		if(texSet.pLightMap && texSet.pLightMap->GetTextureID())
			pMats->GetAt(0).pRE->m_CustomTexBind[1] = texSet.pLightMap->GetTextureID();
		else
			pMats->GetAt(0).pRE->m_CustomTexBind[1] = m_pTerrain->m_nBlackTexId;
*/
/*  No simple detail texturing for voxel
		if(GetCVars()->e_detail_texture && bSetupSinglePassTexIds)
		for(int m = 0; m<VOX_MAX_SINGLETPASS_MATS_NUM && m<MAX_CUSTOM_TEX_BINDS_NUM && m<m_lstOpenMaterials.Count(); m++)
		{
			if(IMaterial * pMat = m_lstOpenMaterials[m].pOpenMat)
			if(SRenderShaderResources * pShRes = m_lstOpenMaterials[m].pOpenMat->GetShaderItem().m_pShaderResources)
			if(SEfResTexture * pEfRes = pShRes->m_Textures[EFTT_DIFFUSE])
			if(CREMesh * pReMesh = pRM->GetChunks()->GetAt(0).pRE)
			{
				ITexture * pTex = ((ITexture *)(pEfRes->m_Sampler.m_pITex));
				if(pTex)
					pReMesh->m_CustomTexBind[m+TERRAIN_BASE_TEXTURES_NUM] = pTex->GetTextureID();
			}
		}*/
	}
}

IRenderMesh * CVoxelMesh::CreateRenderMeshFromIndexedMesh(CMesh * pMatMesh, CVoxelObject * pVoxArea, int nLod)
{
  m_arrpRM_Ambient[nLod] = GetRenderer()->CreateRenderMesh("Voxel", "Voxel");
  m_arrpRM_Ambient[nLod]->SetMesh( *pMatMesh, 0, (GetCVars()->e_DefaultMaterial ? 0 : FSM_NO_TANGENTS) | FSM_VOXELS);
	m_arrpRM_Ambient[nLod]->SetBBox( pMatMesh->m_bbox.min, pMatMesh->m_bbox.max);

  if(m_arrpRM_Ambient[nLod])
    PrintMessagePlus(" %d tris produced,", m_arrpRM_Ambient[nLod]->GetIndicesCount()/3);

  m_arrpRM_Ambient[nLod]->SetMaterial(Get3DEngine()->m_pTerrainMat);

  // build all indices
  short m_arrpNonBorderIdxNum[MAX_SURFACE_TYPES_COUNT][4];
  CTerrainNode::GenerateIndicesForAllSurfaces(m_arrpRM_Ambient[nLod], 0, m_arrpNonBorderIdxNum, 0, 0, pVoxArea->m_nSID);

  static PodArray<SSurfaceType *> lstReadyTypes; lstReadyTypes.Clear(); // protection from duplications in palette of types

  for(int i=0; i<VOX_MAX_SURF_TYPES_NUM; i++)
  {
    if(SSurfaceType * pSType = m_arrSurfaceTypeInfo[nLod][i].pSurfaceType)
    {
      if(lstReadyTypes.Find(pSType)<0)
      {
        uint8 szProj[] = "XYZ";
        for(int p=0; p<3; p++)
        {
          if(pSType->GetMaterialOfProjection(szProj[p]) || pSType->IsMaterial3D()) // todo: duplicate rendering?
          {
            int nProjId = pSType->IsMaterial3D() ? p : 3;
            PodArray<unsigned short> & lstIndices = CTerrainNode::m_arrIndices[pSType->ucThisSurfaceTypeId][nProjId];
            CTerrainNode::UpdateSurfaceRenderMeshes(m_arrpRM_Ambient[nLod], pSType, m_arrSurfaceTypeInfo[nLod][i].arrpRM[p], p, lstIndices, "VoxelMaterialLayer", false, 0);
          }
        }
        lstReadyTypes.Add(pSType);
      }
    }
  }

	return NULL;
}

int CVoxelMesh::GetMemoryUsage()
{
  int nSize = sizeof(*this);

  // clear hash
  for(int i=0; i<VOX_HASH_SIZE; i++)
    for(int j=0; j<VOX_HASH_SIZE; j++)
      nSize += m_arrVertHash[i][j].capacity()*sizeof(uint16);

	return nSize;
}

void CVoxelMesh::UpdateVertexHash(IRenderMesh * pRM)
{
	// get offsets
	int nPosStride=0, nColorStride=0;
	byte  *pPos      = pRM->GetPosPtr(nPosStride, FSL_READ);
	uint8 *pColor    = pRM->GetColorPtr(nColorStride, FSL_READ);

	// clear hash
	for(int i=0; i<VOX_HASH_SIZE; i++)
		for(int j=0; j<VOX_HASH_SIZE; j++)
			m_arrVertHash[i][j].Clear();

	const int nShift = VOX_HASH_SIZE/4;
	const int nMask = VOX_HASH_SIZE-1;

  static uint16 arrVertHashElemCount[VOX_HASH_SIZE][VOX_HASH_SIZE];
  memset(arrVertHashElemCount,0,sizeof(arrVertHashElemCount));

  // count hash elements
  for(int i=0, nVertCount = pRM->GetVerticesCount(); i<nVertCount; i++)
  {
    Vec3 & vPos = *((Vec3*)&(pPos[i*nPosStride]));
    int x = uint8(vPos.x*VOX_HASH_SCALE+nShift)&nMask;
    int y = uint8(vPos.y*VOX_HASH_SCALE+nShift)&nMask;
    arrVertHashElemCount[x][y]++;
  }

  // preallocate hash arrays
  for(int x=0; x<VOX_HASH_SIZE; x++)
    for(int y=0; y<VOX_HASH_SIZE; y++)
      m_arrVertHash[x][y].PreAllocate(arrVertHashElemCount[x][y]);

	// register verts in hash
	for(int i=0, nVertCount = pRM->GetVerticesCount(); i<nVertCount; i++)
	{
    Vec3 & vPos = *((Vec3*)&(pPos[i*nPosStride]));
    int x = uint8(vPos.x*VOX_HASH_SCALE+nShift)&nMask;
    int y = uint8(vPos.y*VOX_HASH_SCALE+nShift)&nMask;
		PodArray<uint16> & rHash = m_arrVertHash[x][y];
    assert(rHash.capacity()>rHash.size());
    rHash.Add(i);
	}
}

int CVoxelMesh::UpdateAmbientOcclusion(CVoxelObject ** pNeighbours, CVoxelObject * pThisArea, int nLod)
{
	FUNCTION_PROFILER_3DENGINE;

	IRenderMesh * pRM = m_arrpRM_Ambient[nLod];
	if(!pRM)
		return 0;

	// get offsets
	int nPosStride=0, nColorStride=0, nNormStride=0;
	byte *pPos      = pRM->GetPosPtr(nPosStride, FSL_READ);
	uint8*pColor    = pRM->GetColorPtr(nColorStride, FSL_SYSTEM_UPDATE);
	byte *pNormB    = pRM->GetNormPtr(nNormStride, FSL_SYSTEM_UPDATE);

	float fVox2WorldRatio = pThisArea->m_Matrix.GetColumn(0).GetLength() * DEF_VOX_UNIT_SIZE; // *pThisArea->m_fUnitSize;
	const float fTestRadiusWS = GetCVars()->e_VoxelAoRadius*fVox2WorldRatio;
	Vec3 vTestRadiusWS(fTestRadiusWS,fTestRadiusWS,fTestRadiusWS);

	bool bSnapToTerrainZ = pThisArea->IsSnappedToTerrainSectors() || (pThisArea->m_nFlags & IVOXELOBJECT_FLAG_SNAP_TO_TERRAIN);

	// calculate occlusion
	for(int i=0, nVertCount = pRM->GetVerticesCount(); i<nVertCount; i++)
	{
    Vec3 vPos; memcpy(&vPos, &pPos[nPosStride*i], sizeof(vPos));

		const byte * bNorm = &pNormB[nNormStride*i];
		const Vec3 vNorm = Vec3((bNorm[0]-128.0f)/127.5f, (bNorm[1]-128.0f)/127.5f, (bNorm[2]-128.0f)/127.5f);

		SMeshColor & uColor = *((SMeshColor*)&pColor[nColorStride*i]);

    Vec3 vPosWS = pThisArea->m_Matrix.TransformPoint(vPos);

		if(bSnapToTerrainZ)
		{
			float fTerrainZDiff = GetTerrain()->GetZApr(vPosWS.x, vPosWS.y, false, pThisArea->m_nSID) - vPosWS.z;
			if(fabs(fTerrainZDiff)<0.5f)
			{
				uColor.a = 255;
				continue;
			}
		}

		float fAmbSumm = 1.f;
		int nSamplesNum = 1;
		
		AABB boxVertexArea;
		boxVertexArea.min = vPosWS - vTestRadiusWS;
		boxVertexArea.max = vPosWS + vTestRadiusWS;

		if(pNeighbours && !pThisArea->GetBBox().ContainsBox(boxVertexArea))
		{
			for(int x=0; x<3; x++)
			{
				for(int y=0; y<3; y++)
				{
					for(int z=0; z<3; z++)
					{
						if(CVoxelObject * pNeibArea = pNeighbours[x*9+y*3+z])
						{
							const AABB & boxNeibAreaBox = pNeibArea->GetBBox();
							if(Overlap::AABB_AABB(boxVertexArea, boxNeibAreaBox))
								nSamplesNum += pNeibArea->GetAmbientOcclusionForPoint(vPosWS, vNorm, fAmbSumm);
						}
					}
				}
			}
		}
		else
			nSamplesNum += GetAmbientOcclusionForPoint(vPos, vNorm, fAmbSumm, pThisArea);

		assert(nSamplesNum);
		if(nSamplesNum)
			fAmbSumm /= nSamplesNum;
		fAmbSumm = 1.f - (1.f - fAmbSumm) * GetCVars()->e_VoxelAoScale;
		
		uColor.a = uint8(255*min(1.f,max(0.f,fAmbSumm)));
	}

	return 0;
}

int CVoxelMesh::GetAmbientOcclusionForPoint(Vec3 vPos, Vec3 vNorm, float & fResult, CVoxelObject * pThisArea)
{
	FUNCTION_PROFILER_3DENGINE;

	IRenderMesh * pRM = m_arrpRM_Ambient[0];
	if(!pRM)
		return 0;

	// get offsets
	int nPosStride=0, nNormStride=0;
	byte *pPos      = pRM->GetPosPtr(nPosStride, FSL_READ);
  byte *pNormB    = pRM->GetNormPtr(nNormStride, FSL_SYSTEM_UPDATE);

	const float fTestRadiusOS = GetCVars()->e_VoxelAoRadius*DEF_VOX_UNIT_SIZE;//pThisArea->m_fUnitSize;

	const int nShift = VOX_HASH_SIZE/4;
	const int nMask = VOX_HASH_SIZE-1;

	// calculate occlusion for point
	// ambient occlusion
	int nSamplesNum = 0;

	int hx = (int)(vPos.x*VOX_HASH_SCALE+nShift);
	int hy = (int)(vPos.y*VOX_HASH_SCALE+nShift);

	int nHashArea = (int)max(1.f,fTestRadiusOS*VOX_HASH_SCALE);
	for(int x=hx-nHashArea; x<=hx+nHashArea; x++)
	{
		for(int y=hy-nHashArea; y<=hy+nHashArea; y++)
		{
			PodArray<uint16> * pList = &m_arrVertHash[uint8(x)&nMask][uint8(y)&nMask];
			for(int n=0; n<pList->Count(); n++)
			{
				int idx = pList->GetAt(n);
				Vec3 vPos0; memcpy(&vPos0, &pPos[nPosStride*idx], sizeof(vPos0));

				const byte * bNorm0 = &pNormB[nNormStride*idx];
				const Vec3 vNorm0 = Vec3((bNorm0[0]-128.0f)/127.5f, (bNorm0[1]-128.0f)/127.5f, (bNorm0[2]-128.0f)/127.5f);

				float fDistV = vPos0.GetDistance(vPos);
				if(fDistV<fTestRadiusOS && fDistV>0.1f)
				{
					float fDistN = (vPos0+vNorm0*fDistV*0.5f).GetDistance(vPos+vNorm*fDistV*0.5f);
					fDistV = max(fDistV,0.001f);
					float t = fDistV/fTestRadiusOS;
					float fRatioFull = min(1.f, fDistN/fDistV);
					float fRatio = (1.f-t)*fRatioFull + t;
					fResult += fRatio;
					nSamplesNum++;
				}
			}
		}
	}

	return nSamplesNum;
}
