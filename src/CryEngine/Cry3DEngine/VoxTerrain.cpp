////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain_det_tex.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: terrain detail textures
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "3dEngine.h"
#include "VoxTerrain.h"
#include "IsoOctree/IsoOctree.h"
#include "IsoOctree/Geometry.h"
#include "IsoOctree/MAT.h"

float CVoxTerrain::m_fMapSize = 0;
const float fIsoTreeCurvature = .8f;
const float fPredictionDistance = 1.f;
bool CVoxTerrain::m_bAllowEditing=true;
int CVoxTerrain::m_nSelLayer = -1;
float CVoxTerrain::m_fSelLayerTime = -1;

bool CVoxTerrain::GetCompiledData(byte * pData, int nDataSize, bool bSaveMesh, EEndian eEndian, bool bSaveForEditing, AABB * pAreaBox, int nSID)
{
  PrintMessage("Serializing voxel terrain (%.2f MB) ...", 1.f/1024.f/1024.f*nDataSize);

  SIsoTreeChunkHeader * pTerrainChunkHeader = (SIsoTreeChunkHeader *)pData;
  pTerrainChunkHeader->nVersion = ISOTREE_CHUNK_VERSION;
  pTerrainChunkHeader->nChunkSize = nDataSize;
  pTerrainChunkHeader->TerrainInfo.fOceanWaterLevel = Get3DEngine()->GetWaterLevel();
  pTerrainChunkHeader->TerrainInfo.aabbTerrain = AABB(Vec3(0,0,0),Vec3(CVoxTerrain::m_fMapSize,CVoxTerrain::m_fMapSize,CVoxTerrain::m_fMapSize));
  pTerrainChunkHeader->TerrainInfo.nModId = IsoOctree_nModId;
  pTerrainChunkHeader->TerrainInfo.nSVoxValueSize = sizeof(SVoxValue);
  UPDATE_PTR_AND_SIZE(pData,nDataSize,sizeof(SIsoTreeChunkHeader));

  int nDataNodes = 0;
  if(CIsoOctreeThread::m_pIsoOctree)
    nDataNodes = CIsoOctreeThread::m_pIsoOctree->GetCompiledData(pData, nDataSize, eEndian, bSaveMesh, bSaveForEditing, pAreaBox);

  assert(nDataNodes && nDataSize==0);

  PrintMessagePlus(" ok");

  return (nDataNodes && nDataSize==0);
}

IMemoryBlock * CVoxTerrain::GetCompiledData(bool bSaveMesh, EEndian eEndian, bool bSaveForEditing, AABB * pAreaBox, int nSID)
{
  return CIsoOctreeThread::GetCompiledData(bSaveMesh, eEndian, this, bSaveForEditing, pAreaBox, nSID);
}

int CVoxTerrain::GetCompiledDataSize(bool bSaveMesh, bool bSaveForEditing, AABB * pAreaBox, int nSID)
{
  FUNCTION_PROFILER_3DENGINE;

  int nDataSize = 0;

  nDataSize += CIsoOctreeThread::m_pIsoOctree->GetCompiledDataSize(bSaveMesh, bSaveForEditing, pAreaBox);

  nDataSize += sizeof(SIsoTreeChunkHeader);

  return nDataSize;
}

template <class T> bool CVoxTerrain::Load_T(T * & f, int & nDataSize, SIsoTreeChunkHeader * pTerrainChunkHeader, bool bUpdateMesh, EEndian eEndian, AABB * pAreaBox, int nSID)
{
  if((pTerrainChunkHeader->nVersion != ISOTREE_CHUNK_VERSION_BC && pTerrainChunkHeader->nVersion != ISOTREE_CHUNK_VERSION) || !GetCVars()->e_VoxTer)
  {
    assert(!"Version error");
    Error("CVoxTerrain::Load_T: version of file is %d, expected version is %d", pTerrainChunkHeader->nVersion, (int)ISOTREE_CHUNK_VERSION);
    return 0;
  }

  if(pTerrainChunkHeader->nChunkSize != nDataSize+sizeof(SIsoTreeChunkHeader))
    return 0;

  if(!pAreaBox)
  {
    for(int i=0; i<ISO_THREADS_NUM; i++)
      SAFE_DELETE(m_pIsoOctreeThreads[i]);

    for(int i=0; i<ISO_THREADS_NUM; i++)
      m_pIsoOctreeThreads[i] = new CIsoOctreeThread(i);
  }

  int nDataNodesLoaded = 0;
  if(GetCVars()->e_VoxTer)
    nDataNodesLoaded = CIsoOctreeThread::m_pIsoOctree->Load_T(f, nDataSize, pTerrainChunkHeader, true, eEndian, pAreaBox, nSID);

  if(!nDataNodesLoaded || nDataSize)
    Error("CVoxTerrain::Load_T: loading failed");

  ImportUPC();

  return (nDataNodesLoaded && nDataSize==0);
}

bool CVoxTerrain::SetCompiledData(byte * pData, int nDataSize, bool bUpdateMesh, EEndian eEndian, AABB * pAreaBox, int nSID)
{
  if(!pAreaBox)
  {  
    for(int i=0; i<ISO_THREADS_NUM; i++)
      SAFE_DELETE(m_pIsoOctreeThreads[i]);

    for(int i=0; i<ISO_THREADS_NUM; i++)
      m_pIsoOctreeThreads[i] = new CIsoOctreeThread(i);
  }

  return CIsoOctreeThread::SetCompiledData(pData, nDataSize, bUpdateMesh, eEndian, this, pAreaBox, nSID);
}

bool CVoxTerrain::Load(FILE * f, int nDataSize, SIsoTreeChunkHeader * pTerrainChunkHeader, int nSID)
{
  return Load_T(f, nDataSize, pTerrainChunkHeader,true,GetPlatformEndian(), NULL, nSID);
}

CVoxTerrain::CVoxTerrain( const SVoxTerrainInfo & info )
{
  m_fMapSize = Get3DEngine()->GetTerrainSize();

  MarchingCubes::SetCaseTable();
  MarchingCubes::SetFullCaseTable();

  if(GetCVars()->e_VoxTerHeightmapEditingCustomLayerInfo)
  {
    int nDim = GetTerrain()->GetTerrainSize()/GetTerrain()->GetHeightMapUnitSize();
    int nDataSize = nDim*nDim*sizeof(SSurfTypeInfo);
    PrintMessage("Allocating layer data: %d x %d, Size = %d MB", nDim, nDim, nDataSize/1024/1024);
    m_arrSurfType2D.Allocate(nDim);
    for(int x=0; x<nDim; x++) for(int y=0; y<nDim; y++)
      m_arrSurfType2D[x][y] = C3DEngine::m_pGetLayerIdAtCallback->GetLayerIdAtPosition(x,y);
  }
  
  for(int i=0; i<ISO_THREADS_NUM; i++)
    m_pIsoOctreeThreads[i] = new CIsoOctreeThread(i);

  m_bAllowEditing = true;
  m_pEditorHelper = 0;
  m_bModified = false;
  m_pDataFile = 0;
  m_nMeshAmountStreamedIn = m_nReMeshTexStreamedIn = m_nMeshesStreamedIn = 0;
  memset(m_arrTargetTexId,0,sizeof(m_arrTargetTexId));
  memset(m_arrTimeStats,0,sizeof(m_arrTimeStats));
}

CVoxTerrain::~CVoxTerrain( )
{
  for(int i=0; i<ISO_THREADS_NUM; i++)
    SAFE_DELETE(m_pIsoOctreeThreads[i]);
}

void CVoxTerrain::Render()
{
  FUNCTION_PROFILER_3DENGINE;

  if(!CIsoOctreeThread::m_pIsoOctree)
    return;

  IsoOctree & isoTree = *CIsoOctreeThread::m_pIsoOctree;

  { // precache textures
    int nFlags = Get3DEngine()->IsShadersSyncLoad() ? FPR_SYNCRONOUS : 0;

    for(int nSType=0; nSType<16; nSType++)
    {
      if(Get3DEngine()->m_arrBaseTextureData.Count() > nSType && Get3DEngine()->m_arrBaseTextureData[nSType].szBaseTexName[0])
      {
        SImageInfo & rImgInfo = Get3DEngine()->m_arrBaseTextureData[nSType];

        for(int i=0; i<4; i++)
        {
          if(rImgInfo.arrTextureId[i])
            if(ITexture*pTex0 = GetRenderer()->EF_GetTextureByID(rImgInfo.arrTextureId[i]))
          {
            if(pTex0->IsTextureLoaded())
              GetRenderer()->EF_PrecacheResource(pTex0, 0.f, 0.f, nFlags, GetObjManager()->m_nUpdateStreamingPrioriryRoundId);
          }
        }
      }
    }
  }

  if(GetCVars()->e_VoxTerDebug)
  {
    RasterizeVolume(0);
  }
  else
  {
    if(GetCVars()->e_Voxel)
      isoTree.Render(AABB(Vec3(0,0,0),Vec3(CVoxTerrain::m_fMapSize,CVoxTerrain::m_fMapSize,CVoxTerrain::m_fMapSize)));
    CIsoOctreeThread::Update(Get3DEngine());
  }

  if(GetCVars()->e_VoxTer==3)
  {
    OctNode::NodeIndex nIdx;
    isoTree.DrawDebugValues(&CIsoOctreeThread::m_pIsoOctree->tree,nIdx);
  }

  if(GetCVars()->e_VoxTer==4)
    DrawStreamingActivity();

  // manage streaming
  if(!m_bEditor)
  {
    static bool bFirstTime = true;

    if(!(GetMainFrameID()&3) || Get3DEngine()->IsStatObjSyncLoad() || bFirstTime)
    {
      FRAME_PROFILER( "CVoxTerrain::Render_ActivateStreaming", GetSystem(), PROFILE_3DENGINE );

      OctNode::NodeIndex nIdx;
      AABB parentBoxWS = isoTree.GetNodeAABB(nIdx, true);
      parentBoxWS.min *= 2;
      parentBoxWS.max *= 2;
      isoTree.ActivateStreaming(&isoTree.renderTree, nIdx, parentBoxWS);
    }

    bFirstTime = false;

    if(PodArray<SIsoMesh*> * pObjects = SIsoMesh::GetCounter())
      if(pObjects->Count())
    { 
      FRAME_PROFILER( "CVoxTerrain::Render_UnloadOld", GetSystem(), PROFILE_3DENGINE );

      const int nMeshesToVisit = pObjects->Count()/10+1;

      for(int n=0; n<nMeshesToVisit; n++)
      {
        static int nCurrentMeshId = 0;
        static int nStreamedInMeshesNum = 0;
				static int nStreamedInTexAmount = 0;
        static int nStreamedInMeshAmount = 0;

        if(nCurrentMeshId >= pObjects->Count())
        {
          nCurrentMeshId = 0;
          m_nMeshesStreamedIn = nStreamedInMeshesNum;
          nStreamedInMeshesNum = 0;
					m_nReMeshTexStreamedIn = nStreamedInTexAmount;
					nStreamedInTexAmount = 0;
          m_nMeshAmountStreamedIn = nStreamedInMeshAmount;
          nStreamedInMeshAmount = 0;
        }

        SIsoMesh * pMesh = pObjects->GetAt(nCurrentMeshId);

        if( pMesh->m_eStreamingStatus == ecss_Ready && !pMesh->m_pPhysEnt && pMesh->m_fLastDrawTime < GetCurTimeSec()-3 )
        {
          pMesh->ResetRenderMeshs();
          pMesh->ReleaseTextures();
          pMesh->m_eStreamingStatus = ecss_NotLoaded;
        }

        if(pMesh->m_eStreamingStatus == ecss_Ready)
          nStreamedInMeshesNum++;

				if(pMesh->m_arrSrcTex[0])
				{
					nStreamedInTexAmount += GetRenderer()->EF_GetTextureByID(pMesh->m_arrSrcTex[0])->GetDataSize();
					nStreamedInTexAmount += GetRenderer()->EF_GetTextureByID(pMesh->m_arrSrcTex[1])->GetDataSize();
				}

        if(pMesh->m_pRenderMesh)
          nStreamedInMeshAmount += (pMesh->m_pRenderMesh->GetVerticesCount()*(sizeof(SVF_P3S_N4B_C4B_T2S)) + pMesh->m_pRenderMesh->GetIndicesCount()*sizeof(uint16));

        nCurrentMeshId ++;
      }
    }
  }

  if(PodArray<SIsoMesh*> * pObjects = SIsoMesh::GetCounter())
  {
    if(m_bEditor && pObjects->Count() && GetCVars()->e_VoxTerTexBuildOnCPU && GetCVars()->e_Voxel)
    { 
      FRAME_PROFILER( "CVoxTerrain::Render_UnloadOld", GetSystem(), PROFILE_3DENGINE );

      const int nMeshesToVisit = pObjects->Count()/20+1;

      for(int n=0; n<nMeshesToVisit; n++)
      {
        static int nCurrentMeshId = 0;
        static int nStreamedInMeshesNum = 0;
        static int nStreamedInTexAmount = 0;
        static int nStreamedInMeshAmount = 0;

        if(nCurrentMeshId >= pObjects->Count())
        {
          nCurrentMeshId = 0;
          m_nMeshesStreamedIn = nStreamedInMeshesNum;
          nStreamedInMeshesNum = 0;

          if(!GetCVars()->e_VoxTerHeightmapEditing)
            m_nReMeshTexStreamedIn = nStreamedInTexAmount;
          nStreamedInTexAmount = 0;
          m_nMeshAmountStreamedIn = nStreamedInMeshAmount;
          nStreamedInMeshAmount = 0;
        }

        SIsoMesh * pMesh = pObjects->GetAt(nCurrentMeshId);

        if( pMesh->m_arrSrcTex[0] && (pMesh->m_fLastDrawTime < GetCurTimeSec()-15.f) && (m_nReMeshTexStreamedIn > GetCVars()->e_VoxTerTexPoolSizeForEditor*1024*1024) )
        {
          pMesh->ReleaseTextures();
        }

				if(pMesh->m_arrSrcTex[0] && !GetCVars()->e_VoxTerHeightmapEditing)
				{
					nStreamedInTexAmount += GetRenderer()->EF_GetTextureByID(pMesh->m_arrSrcTex[0])->GetDataSize();
					nStreamedInTexAmount += GetRenderer()->EF_GetTextureByID(pMesh->m_arrSrcTex[1])->GetDataSize();
				}

        if(pMesh->m_pRenderMesh)
          nStreamedInMeshAmount += (pMesh->m_pRenderMesh->GetVerticesCount()*(sizeof(SVF_P3S_N4B_C4B_T2S)) + pMesh->m_pRenderMesh->GetIndicesCount()*sizeof(uint16));

        nCurrentMeshId ++;
      }
      if(GetCVars()->e_VoxTerHeightmapEditing)
        m_nReMeshTexStreamedIn = SIsoMesh::UnloadOldSrcTextures();
    }
  }

  if(CVoxTerrain::m_nSelLayer >= 0)
  {
    if(CVoxTerrain::m_fSelLayerTime < GetCurTimeSec() - 0.5f)
		{
      if(Get3DEngine()->m_pVoxTerrain)
        Get3DEngine()->m_pVoxTerrain->RequestTextureUpdate(AABB(Vec3(0,0,0),Vec3((float)Get3DEngine()->GetTerrainSize(), (float)Get3DEngine()->GetTerrainSize(), (float)Get3DEngine()->GetTerrainSize())));

      CVoxTerrain::m_nSelLayer = -1;
			CVoxTerrain::m_fSelLayerTime = -1;
		}
  }

	if(GetCVars()->e_VoxTerOnTheFlyIntegration)
		ProcessNodesForUpdate();
	m_arrNodesForUpdate.Clear();
}

void CVoxTerrain::PolygonToTriangleMeshForPhysics(const std::vector<Vec3>& vertices, const std::vector<SSurfTypeInfo>& surf_types, const std::vector< std::vector<int> > & polygons, std::vector<uint16> & indices)
{
  FUNCTION_PROFILER_3DENGINE;

  MinimalAreaTriangulation mat;
  for(size_t i=0;i<polygons.size();i++)
  {
    std::vector<Vec3> loop; loop.clear();
    std::vector<int> loop_st; loop_st.clear();
    std::vector<int> vertexMap; vertexMap.clear();
    std::vector<TriangleIndex> tgl; tgl.clear();
    loop.resize(polygons[i].size());
    loop_st.resize(polygons[i].size());
    vertexMap.resize(polygons[i].size());
    for(size_t j=0;j<polygons[i].size();j++)
    {
      loop[j]=vertices[polygons[i][j]];
      loop_st[j]=surf_types[polygons[i][j]].GetDomSType();
      vertexMap[j]=polygons[i][j];
    }

    mat.GetTriangulation(loop,loop_st,tgl);

    int nPrevSize = indices.size();
    indices.resize(nPrevSize+tgl.size()*3);
    for(size_t j=0;j<tgl.size();j++)
    {
      for(int k=0;k<3;k++)
        indices[nPrevSize+j*3+k]=vertexMap[tgl[j].idx[k]];
    }
  }
}

void CVoxTerrain::PolygonToTriangleMeshForRendering(const std::vector<Vec3>& vertices, const std::vector<SSurfTypeInfo>& surf_types, const std::vector< std::vector<int> > & polygons, std::vector<TriangleIndex> & triangles, int nIn)
{
  FUNCTION_PROFILER_3DENGINE;

  MinimalAreaTriangulation mat;

  for(size_t i=0;i<polygons.size();i++)
  {
    std::vector<Vec3> loop; loop.clear();
    std::vector<int> loop_st; loop_st.clear();
    std::vector<int> vertexMap; vertexMap.clear();
    std::vector<TriangleIndex> tgl; tgl.clear();
    loop.resize(polygons[i].size());
    loop_st.resize(polygons[i].size());
    vertexMap.resize(polygons[i].size());
    for(size_t j=0;j<polygons[i].size();j++)
    {
      loop[j]=vertices[polygons[i][j]];
      loop_st[j]=surf_types[polygons[i][j]].GetDomSType();
      vertexMap[j]=polygons[i][j];
    }

    mat.GetTriangulation(loop,loop_st,tgl);

    size_t tSize=triangles.size();
    triangles.resize(tSize+tgl.size());
    for(size_t j=0;j<tgl.size();j++)
    {
      for(int k=0;k<3;k++)
        triangles[tSize+j][k]=vertexMap[tgl[j].idx[k]];
      triangles[tSize+j].nCull = !nIn;
    }
  }
}

SBrushAABB * GetBrushAABB(const Sphere & sp, EVoxelEditOperation eOperation, float fMapSize, float fEps)
{
  static SBrushAABB br;

  br.fEps = fEps;
  
  br.spEps = sp;

/*  if(eOperation == eveoPaintHeightPos)
    br.spEps.center.z -= sp.radius/2;
  else if(eOperation == eveoPaintHeightNeg)
    br.spEps.center.z += sp.radius/2;*/

  br.spEps.radius += fEps;

  Vec3 vRadiusEps(br.spEps.radius, br.spEps.radius, br.spEps.radius);
  br.boxEps = AABB(sp.center-vRadiusEps, sp.center+vRadiusEps);

/*  if(eOperation == eveoPaintHeightPos || eOperation == eveoPaintHeightNeg)
  {
    Vec3 vCenter = br.boxEps.GetCenter();
    br.boxEps.min.z = (vCenter.z + br.boxEps.min.z) / 2;
    br.boxEps.max.z = (vCenter.z + br.boxEps.max.z) / 2;
  }*/

  return &br;
}

void CVoxTerrain::DoVoxelShape(Vec3 vWSPos, float fRadius, int nSurfaceTypeId, ColorB color, EVoxelEditOperation eOperation, EVoxelBrushShape eShape, PodArray<IRenderNode*> * pBrushes, float fMinVoxelSize, AABB * pUpdateArea)
{
  m_bModified = true;

  // prevent too many editing requests
  static float fLastEditTime = -1;
  if( (eOperation != eveoCopyTerrainPos && eOperation != eveoCopyTerrainNeg) && (eOperation != eveoIntegrateMeshPos) && (eOperation != eveoIntegrateMeshNeg) && ((GetCurTimeSec() - fLastEditTime) < 0.1f) )
    return;
  fLastEditTime = GetCurTimeSec();

  m_bAllowEditing = true;

  int nDetailLevel = 0;
  float fTerrainSize = Get3DEngine()->GetTerrainSize();
  while(fTerrainSize>fMinVoxelSize)
  {
    fTerrainSize/=2;
    nDetailLevel++;
  }

  if(nDetailLevel > IsoOctree::nMeshDepth + GetCVars()->e_VoxTerMaxMeshBuildDepth)
    nDetailLevel = IsoOctree::nMeshDepth + GetCVars()->e_VoxTerMaxMeshBuildDepth;

  if(nDetailLevel > IsoOctree::maxDepth)
    nDetailLevel = IsoOctree::maxDepth;

  if(CIsoOctreeThread::m_pIsoOctree)
    CIsoOctreeThread::m_pIsoOctree->m_lastEditingTime = GetCurTimeSec();

  if(eOperation == eveoIntegrateMeshPos || eOperation == eveoIntegrateMeshNeg)
  {
    PrintMessage("Preparing brushes for integration ...");

    PodArray<IRenderNode*> lstBrushes; lstBrushes.Clear();

    if(pBrushes)
      lstBrushes = *pBrushes;
    else
    {
      AABB areaBoxWS(vWSPos-Vec3(fRadius,fRadius,fRadius), vWSPos+Vec3(fRadius,fRadius,fRadius));
      Get3DEngine()->GetObjectsByTypeGlobal(lstBrushes, eERType_Brush, &areaBoxWS, 0);
    }

    int nBrushesFound = 0;
    int nGroupsFound = 0;

    if(GetCVars()->e_LodMin>0)
      Error("e_LodMin is not 0");

    for(int nBrush=0; nBrush<lstBrushes.Count(); nBrush++)
    {
      IRenderNode * pObj = lstBrushes[nBrush];

      IMaterial * pObjMaterial = pObj->GetMaterial();

      if(pObj->GetRndFlags() & ERF_HIDDEN)
        continue;

      if(pObj->GetIntegrationType() == eIT_VoxelMesh)
      {
        SEditThreadMessage * pEditTask = new SEditThreadMessage;

        pEditTask->nProgressCur = nBrush+1;
        pEditTask->nProgressAll = lstBrushes.Count();
        pEditTask->bAllowRequestTreeUpdate = pEditTask->nProgressCur>=pEditTask->nProgressAll;

        pEditTask->vWSPos = pObj->GetBBox().GetCenter();
				pEditTask->fRadius = pObj->GetBBox().GetRadius();
        pEditTask->nSurfaceTypeId = nSurfaceTypeId;
        pEditTask->color = color;
        pEditTask->eOperation = eveoForceDepth;
        pEditTask->eShape = eShape;
        pEditTask->nDetailLevel = IsoOctree::nMeshDepth;
        pEditTask->meshBox = pObj->GetBBox();
        pEditTask->editingTime = CIsoOctreeThread::m_pIsoOctree->m_lastEditingTime;
				if(pUpdateArea)
				{
//					pEditTask->updateArea = *pUpdateArea;
//					pEditTask->vWSPos = pEditTask->updateArea.GetCenter();
	//				pEditTask->fRadius = pEditTask->updateArea.GetRadius();
				}

        CIsoOctreeThread::AddTaskIn(pEditTask);
        continue;
      }

      Matrix34A mat;

      CStatObj * pParentStatObj = (CStatObj *)pObj->GetEntityStatObj(0, 0, &mat);
      if(!pParentStatObj || pParentStatObj->m_nFlags&STATIC_OBJECT_HIDDEN)
        continue;

      PodArray<CStatObj*> arrStatObj;

      if(pParentStatObj->m_nFlags & STATIC_OBJECT_COMPOUND)
      {
        for (int i = 0; i < pParentStatObj->GetSubObjectCount(); i++)
        {
          IStatObj::SSubObject * pSubObject = pParentStatObj->GetSubObject(i);

          if(!pSubObject || pSubObject->nType != STATIC_SUB_OBJECT_MESH)
            break;

          if (pSubObject->pStatObj && !pSubObject->bHidden)
            arrStatObj.Add((CStatObj*)pSubObject->pStatObj);
        }
      }
      else
      {
        arrStatObj.Add(pParentStatObj);
      }

      for(int nStatObj=0; nStatObj<arrStatObj.Count(); nStatObj++)
      {
        CStatObj * pStatObj = arrStatObj[nStatObj];

        if(!pStatObj || !pStatObj->GetRenderMesh())
          continue;

        IMaterial * pStatObjMaterial = pObjMaterial ? pObjMaterial : pStatObj->GetMaterial();

        if(!pStatObjMaterial)
          continue;

        // prepare indices, vertices and bbox, all in WS
        std::vector<Vec3> * pVertices = new std::vector<Vec3>;
        std::vector<int> * pIndices = new std::vector<int>;

        IRenderMesh * pRenderMesh = pStatObj->GetRenderMesh();

        AABB meshBox; meshBox.Reset();

        // get positions
        int nPosStride = 0;
        byte * pPos  = pRenderMesh->GetPosPtr(nPosStride, FSL_READ);
        pVertices->resize(pRenderMesh->GetVerticesCount());
        for(int i=0; i<pRenderMesh->GetVerticesCount(); i++)
        {
          Vec3 v0 = (*(Vec3*)&pPos[nPosStride*i]);
          v0 = mat.TransformPoint(v0);
          meshBox.Add(v0);
          (*pVertices)[i] = v0;
        }

        // get indices
        int nIndCount = pRenderMesh->GetIndicesCount();
        const uint16 * pVidIndices = pRenderMesh->GetIndexPtr(FSL_READ);
        pIndices->reserve(nIndCount);

        {
          PodArray<CRenderChunk>&	Chunks = pRenderMesh->GetChunks();
          const uint32 VertexCount = pRenderMesh->GetVerticesCount();
          for(int nChunkId=0; nChunkId<Chunks.Count(); nChunkId++)
          {
            CRenderChunk *pChunk = &Chunks[nChunkId];
            if(pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
              continue;

            // skip transparent and alpha test
            const SShaderItem &shaderItem = pStatObjMaterial->GetShaderItem(pChunk->m_nMatID);
            if (!shaderItem.IsZWrite())
              continue;

            if(shaderItem.m_pShader && shaderItem.m_pShader->GetFlags()&EF_DECAL)
              continue;

            if(!shaderItem.m_pShaderResources || shaderItem.m_pShaderResources->GetAlphaRef()>0 || shaderItem.m_pShaderResources->GetGlow()>0)
              continue;

            int nLastIndexId = pChunk->nFirstIndexId + pChunk->nNumIndices;
            int nIndexStep = 3;
            for(int i=pChunk->nFirstIndexId; i<nLastIndexId-2; i+=nIndexStep)
            {
              int32 I0	=	pVidIndices[i+0];
              int32 I1	=	pVidIndices[i+1];
              int32 I2	=	pVidIndices[i+2];

              (*pIndices).push_back(I0);
              (*pIndices).push_back(I1);
              (*pIndices).push_back(I2);
            }
          }
        }

        mesh_compiler::CMeshCompiler meshCompiler;
        int nVertNum = pVertices->size();
        meshBox.min -= Vec3(0.1f,0.1f,0.1f);
        meshBox.max += Vec3(0.1f,0.1f,0.1f);
        meshCompiler.WeldPositions( &((*pVertices)[0]), nVertNum, *pIndices, 0.025f, meshBox );
        pVertices->resize(nVertNum);

        {
          // generate connectivity info

          std::vector<Vec3> & vertices = *(pVertices);
          std::vector<int> & indices = *(pIndices);

          int numVertices = vertices.size();
          int numFaces = indices.size()/3;

          PodArray<VertInfo> arrVertInfo; arrVertInfo.PreAllocate(numVertices, numVertices);
          PodArray<int> arrFacesProjId; arrFacesProjId.PreAllocate(numFaces, numFaces);
          PodArray<VertInfo> arrFaceConnections; arrFaceConnections.PreAllocate(numFaces, numFaces);

          for (int i=0; i<numFaces; i++) 
          {
            // build list of faces for every vertex
            for (int v=0; v<3; v++)
              if(arrVertInfo[indices[i*3+v]].lstFacesIds.Find(i)<0)
                arrVertInfo[indices[i*3+v]].lstFacesIds.Add(i);

            // store face projection id
            Vec3 v0 = vertices[indices[i*3+0]];
            Vec3 v1 = vertices[indices[i*3+1]];
            Vec3 v2 = vertices[indices[i*3+2]];

            Vec3 vNormal = (v1-v0).Cross(v2-v0).GetNormalized();
            int nProjId = 1;//GetVecProjId(vNormal, 0.05f);

            arrFacesProjId[i] = nProjId;
          }

          // build for every face list of connected faces with same projection id
          for (int nMainFaceId=0; nMainFaceId<numFaces; nMainFaceId++) 
          {
            // build list of all faces used by all 3 vertices of the face (with duplicates)
            PodArray<int> allNeibFaces; allNeibFaces.Clear();
            for (int v=0; v<3; v++)
              allNeibFaces.AddList(arrVertInfo[indices[nMainFaceId*3+v]].lstFacesIds);

            PodArray<int> & connFaces = arrFaceConnections[nMainFaceId].lstFacesIds;

            // add faces duplicated more than once in allNeibFaces into connFaces
            for(int n=0; n<allNeibFaces.Count(); n++)
            {
              int nNeibFaceId = allNeibFaces[n];

              if(nMainFaceId != nNeibFaceId)
                if(connFaces.Find(nNeibFaceId) < 0)
                  if(arrFacesProjId[nNeibFaceId] == arrFacesProjId[nMainFaceId])
                  {
                    int nFaceUsageNum=0;
                    for(int c=0; c<allNeibFaces.Count(); c++)
                      if(allNeibFaces[c] == nNeibFaceId)
                        nFaceUsageNum++;

                    if(nFaceUsageNum>1)
                      connFaces.Add(nNeibFaceId);
                  }
            }

            int nRes = connFaces.Count();
          }

          PodArray<SGroupInfo> arrGroups;

          int nStartFaceId=0;

          while(1)
          {
            // find next starting face
            for (; nStartFaceId<numFaces; nStartFaceId++) 
              if(arrFacesProjId[nStartFaceId]>=0)
                break;

            if(nStartFaceId==numFaces)
              break; // all faces processed

            arrGroups.Add(SGroupInfo());
            SGroupInfo & newGroup = arrGroups.Last();

            PodArray<uint32> arrFacesToProcess;
            arrFacesToProcess.Add(nStartFaceId);
            assert(arrFacesProjId[nStartFaceId]>=0);
            arrFacesProjId[nStartFaceId] -= 100;

            while(arrFacesToProcess.Count()) 
            {
              uint32 nMainFaceId = arrFacesToProcess[0];

              arrFacesToProcess.Delete(0);

              PodArray<int> & connFaces = arrFaceConnections[nMainFaceId].lstFacesIds;

              if(newGroup.arrTris.Find(nMainFaceId)>=0)
                continue;

              newGroup.arrTris.Add(nMainFaceId);

              for(int i=0; i<connFaces.Count(); i++)
              {
                if(arrFacesProjId[connFaces[i]] >= 0)
                {
                  arrFacesProjId[connFaces[i]] -= 100;
                  arrFacesToProcess.Add(connFaces[i]);
                }
              }
            }
          }

          int nRes = arrGroups.Count();
      
          for(int g=0; g<arrGroups.Count(); g++)
          {
            PodArray<uint32> & arrTris = arrGroups[g].arrTris;
            if(arrTris.Count()<3)
              continue;

            SEditThreadMessage * pEditTask = new SEditThreadMessage;

            pEditTask->nProgressCur = nBrush+1;
            pEditTask->nProgressAll = lstBrushes.Count();
            pEditTask->bAllowRequestTreeUpdate = pEditTask->nProgressCur>=pEditTask->nProgressAll;

            pEditTask->vWSPos = pObj->GetBBox().GetCenter();
						pEditTask->fRadius = pObj->GetBBox().GetRadius();
            pEditTask->nSurfaceTypeId = nSurfaceTypeId;
            pEditTask->color = color;
            pEditTask->eOperation = eOperation;
            pEditTask->eShape = eShape;
            pEditTask->nDetailLevel = nDetailLevel;
            pEditTask->vertices = new std::vector<Vec3>;
            (*pEditTask->vertices) = *pVertices;
            pEditTask->indices = new std::vector<int>;
            pEditTask->meshBox.Reset();
            pEditTask->editingTime = CIsoOctreeThread::m_pIsoOctree->m_lastEditingTime;
						if(pUpdateArea)
						{
//							pEditTask->updateArea = *pUpdateArea;
//							pEditTask->vWSPos = pEditTask->updateArea.GetCenter();
	//						pEditTask->fRadius = pEditTask->updateArea.GetRadius();
						}

            for(int t=0; t<arrTris.Count(); t++)
            {
              int nFaceId = arrTris[t];
              
              Vec3 v0 = (*pVertices)[(*pIndices)[nFaceId*3+0]];
              Vec3 v1 = (*pVertices)[(*pIndices)[nFaceId*3+1]];
              Vec3 v2 = (*pVertices)[(*pIndices)[nFaceId*3+2]];

              Triangle tr(v0,v1,v2);

              if(tr.GetArea()<0.001f)
                continue;

							if(pUpdateArea && !Overlap::AABB_Triangle(*pUpdateArea, v0, v1, v2))
                continue;

              pEditTask->indices->push_back((*pIndices)[nFaceId*3+0]);
              pEditTask->indices->push_back((*pIndices)[nFaceId*3+1]);
              pEditTask->indices->push_back((*pIndices)[nFaceId*3+2]);

              pEditTask->meshBox.Add(v0);
              pEditTask->meshBox.Add(v1);
              pEditTask->meshBox.Add(v2);
            }
            
						if(pEditTask->indices->size()>9)
						{
							pEditTask->vWSPos = pEditTask->meshBox.GetCenter();
							pEditTask->fRadius = pEditTask->meshBox.GetRadius();
							CIsoOctreeThread::AddTaskIn(pEditTask);
							nGroupsFound++;
						}
						else
						{
							delete pEditTask->vertices;
							delete pEditTask->indices;
							delete pEditTask;
						}
          }
        }

        delete pIndices;
        delete pVertices;
      }

      nBrushesFound++;
    }

    PrintMessagePlus(" %d brushes found, %d mesh groups produced", nBrushesFound, nGroupsFound);

    if(nGroupsFound)
      PrintMessagePlus(", press Ctrl-Break to abort ...");

    if(IsContinue())
      CIsoOctreeThread::MSGI_PrintText("Finished integrating brushes (%.1f)");
  }
  else
  {
    SEditThreadMessage * pEditTask = new SEditThreadMessage;

    pEditTask->vWSPos = vWSPos;
    pEditTask->fRadius = fRadius;
    pEditTask->nSurfaceTypeId = nSurfaceTypeId;
    pEditTask->color = color;
    pEditTask->eOperation = eOperation;
    pEditTask->eShape = eShape;
    pEditTask->nDetailLevel = nDetailLevel;
    pEditTask->vertices = 0;
    pEditTask->indices = 0;

    CIsoOctreeThread::AddTaskIn(pEditTask);
  }  

  // do not overflow octree thread and give it some time so it can update at least some of meshes
  CrySleep(20);
}

/*int CVoxNode__Cmp_CVoxNode(const void* v1, const void* v2)
{
  CVoxMeshNode *p1 = *(CVoxMeshNode**)v1;
  CVoxMeshNode *p2 = *(CVoxMeshNode**)v2;

  float f1 = p1->GetNodeSize();
  float f2 = p2->GetNodeSize();

  if(f1 > f2)
    return 1;
  else if(f1 < f2)
    return -1;

  f1 = (float)p1->GetRebuildFrameId();
  f2 = (float)p2->GetRebuildFrameId();

  if(f1 > f2)
    return 1;
  else if(f1 < f2)
    return -1;

  return 0;
}*/

/*void CVoxTerrain::UpdateTreeIncrementally()
{
  if(!m_rebuildLine.Count())
    return;

  qsort(m_rebuildLine.GetElements(), m_rebuildLine.Count(), sizeof(m_rebuildLine[0]), CVoxNode__Cmp_CVoxNode);
  m_rebuildLine[0]->UpdateMesh();
  m_rebuildLine.Delete(0);
}*/

void CVoxTerrain::FillShadowCastersList(CDLight * pLight, ShadowMapFrustum * pFr, PodArray<SPlaneObject> * pShadowHull, bool bUseFrustumTest, int nMaxReqursion)
{
  OctNode::NodeIndex nIdx;
  if(CIsoOctreeThread::m_pIsoOctree)
    CIsoOctreeThread::m_pIsoOctree->FillShadowCastersList(&CIsoOctreeThread::m_pIsoOctree->renderTree, nIdx, false, pLight, pFr, pShadowHull, bUseFrustumTest);
}

float CVoxTerrain::GetElevation3D(Vec3 vPos, Vec3 & vNorm)
{
  vNorm = Vec3(0.f,0.f,1.f);

  if (!m_bEditor || !CIsoOctreeThread::m_pIsoOctree)
    return 0;

  if(m_nElevCacheLastFrameId != GetMainFrameID() || m_arrElevCache.Count()>16)
  {
    if(m_arrElevCache.Count())
      m_arrElevCache.Delete(0);
    m_nElevCacheLastFrameId = GetMainFrameID();
  }

  SElevCacheItem item;
  item.vPos = vPos;

  float fMaxZ = 0;

  int nFind = -1;

  for(int i=0; i<m_arrElevCache.Count(); i++)
  {
    if(m_arrElevCache[i].vPos.x == item.vPos.x && m_arrElevCache[i].vPos.y == item.vPos.y)
    {
      nFind = i;
      break;
    }
  }
  
  if(nFind<0)
  {
    Vec3 vMaxZNorm(0,0,1);

    Ray r(vPos / CVoxTerrain::m_fMapSize, Vec3(0,0,-1.f));
    r.origin.z = 1.f;

    Vec3 vHitPoint[3];

    if(Get3DEngine()->m_pVoxTerrain->RayIntersection(r, vHitPoint[0]))
    {
      fMaxZ = vHitPoint[0].z*CVoxTerrain::m_fMapSize;

      r.origin.x += .5f / CVoxTerrain::m_fMapSize;
      if(Get3DEngine()->m_pVoxTerrain->RayIntersection(r, vHitPoint[1]))
      {
        r.origin.x -= .5f / CVoxTerrain::m_fMapSize;
        r.origin.y += .5f / CVoxTerrain::m_fMapSize;
        if(Get3DEngine()->m_pVoxTerrain->RayIntersection(r, vHitPoint[2]))
        {
          vMaxZNorm = (vHitPoint[0]-vHitPoint[1]).GetNormalized().Cross((vHitPoint[0]-vHitPoint[2]).GetNormalized());
        }
      }
    }

    item.vPos.z = fMaxZ;
    vNorm = item.vNor = vMaxZNorm;
    m_arrElevCache.Add(item);
  }
  else
  {
    fMaxZ = m_arrElevCache[nFind].vPos.z;
    vNorm = m_arrElevCache[nFind].vNor;
  }

  return fMaxZ;
}

void CVoxTerrain::GetVoxelColorInterpolated(class IsoOctree * isoTree, Vec3 vPos, ColorB & resColor, void* node, void* pIdx)
{
  FUNCTION_PROFILER_3DENGINE;

  //  IsoOctree & isoTree = *m_pIsoOctree;
  /*
  if(node && pIdx)
  {
  isoTree->GetMaterialByPosition((OctNode*)node, *(OctNode::NodeIndex*)pIdx, vPos/fMapSize, ppMats, ppWeights, pMatsNum);
  }
  else*/
  {
    OctNode::NodeIndex nIdx;
    isoTree->GetColorByPosition(&isoTree->tree, nIdx, vPos/CVoxTerrain::m_fMapSize, resColor);
  }
}

void CVoxTerrain::GetVoxelValueInterpolated(IsoOctree * _isoTree, Vec3 vPos, ColorF & resColor, void* node, void* pIdx, SSurfTypeInfo & surf_type, Vec3 & resNormal, SVoxValueCache * pVVC)
{
  //FUNCTION_PROFILER_3DENGINE;

  IsoOctree & isoTree = *(CIsoOctreeThread::m_pIsoOctree);

/*  if(node && pIdx)
  {
    isoTree.GetMaterialByPosition((OctNode*)node, *(OctNode::NodeIndex*)pIdx, vPos/CVoxTerrain::m_fMapSize, resColor, surf_type);
  }
  else*/
  {
    OctNode::NodeIndex nIdx;
    isoTree.GetMaterialByPosition(&isoTree.tree, nIdx, vPos/CVoxTerrain::m_fMapSize, resColor, surf_type, resNormal, pVVC);

    if(GetCVars()->e_VoxTerHeightmapEditing)
      resNormal = GetTerrain()->GetTerrainSurfaceNormal(vPos,1.f,false);
  }
}

/*
uint16 CVoxTerrain::GetCachedVoxelValueInterpolated(Vec3 vPos, float fGridSize, int ** ppMats, float ** ppWeights, int * pMatsNum, IsoOctree * pVoxNode, const AABB & nodeBox)
{
  FUNCTION_PROFILER_3DENGINE;

#define VOX_EPS VEC_EPSILON

  float x=vPos.x;
  float y=vPos.y;
  float z=vPos.z;

  float nScale = 2.f;

  x /= nScale;
  y /= nScale;
  z /= nScale;

  int nX0 = (int)floor(x); int nX1 = nX0+1;
  float fDX = x - nX0;
  assert(fDX>=-VOX_EPS && fDX<=1.f);

  int nY0 = (int)floor(y); int nY1 = nY0+1;
  float fDY = y - nY0;
  assert(fDY>=-VOX_EPS && fDY<=1.f);

  int nZ0 = (int)floor(z); int nZ1 = nZ0+1;
  float fDZ = z - nZ0;
  assert(fDZ>=-VOX_EPS && fDZ<=1.f);

  float arrfValues1D[2];

  int arrMats[2][2][2];

  for(int nX=0; nX<2; nX++)
  {
    float fValue0 = 
      (1.f - fDZ)*GetCachedVoxelValue(Vec3i(1,1,1)+Vec3i((int)(nScale*(nX0+nX)), (int)(nScale*nY0), (int)(nScale*nZ0)), fGridSize, &arrMats[nX][0][0], pVoxNode, nodeBox) +
      (			 fDZ)*GetCachedVoxelValue(Vec3i(1,1,1)+Vec3i((int)(nScale*(nX0+nX)), (int)(nScale*nY0), (int)(nScale*nZ1)), fGridSize, &arrMats[nX][0][1], pVoxNode, nodeBox);

    float fValue1 = 
      (1.f - fDZ)*GetCachedVoxelValue(Vec3i(1,1,1)+Vec3i((int)(nScale*(nX0+nX)), (int)(nScale*(nY0+1)), (int)(nScale*nZ0)), fGridSize, &arrMats[nX][1][0], pVoxNode, nodeBox) +
      (			 fDZ)*GetCachedVoxelValue(Vec3i(1,1,1)+Vec3i((int)(nScale*(nX0+nX)), (int)(nScale*(nY0+1)), (int)(nScale*nZ1)), fGridSize, &arrMats[nX][1][1], pVoxNode, nodeBox);

    arrfValues1D[nX] = (1.f - fDY)*fValue0 + (fDY)*fValue1;
  }

  float fIsoValue = (1.f - fDX)*arrfValues1D[0] +(fDX)*arrfValues1D[1];

  static PodArray<int> arrMatsNoDup; arrMatsNoDup.Clear();
  static PodArray<float> arrWeightsNoDup; arrWeightsNoDup.Clear();

  { // materials
    for(int n=0; n<8; n++)
      if(((int*)arrMats)[n]>=0)
        if(arrMatsNoDup.Find(((int*)arrMats)[n])<0)
          arrMatsNoDup.Add(((int*)arrMats)[n]);

    arrWeightsNoDup.PreAllocate(arrMatsNoDup.Count(),arrMatsNoDup.Count());

    for(int i=0; i<arrMatsNoDup.Count(); i++)
    {
      int nMatId = arrMatsNoDup[i];

      for(int nX=0; nX<2; nX++)
      {
        float fValue0 = (1.f - fDZ)*(nMatId == arrMats[nX][0][0]) + (fDZ)*(nMatId == arrMats[nX][0][1]);
        float fValue1 = (1.f - fDZ)*(nMatId == arrMats[nX][1][0]) + (fDZ)*(nMatId == arrMats[nX][1][1]);
        arrfValues1D[nX] = (1.f - fDY)*fValue0 + (fDY)*fValue1;
      }

      arrWeightsNoDup[i] = (1.f - fDX)*arrfValues1D[0] +(fDX)*arrfValues1D[1];
    }

    if(ppMats)
    {
      float fSumW = 0;
      for(int i=0; i<arrWeightsNoDup.Count(); i++)
        fSumW += arrWeightsNoDup[i];
      for(int i=0; i<arrWeightsNoDup.Count(); i++)
        arrWeightsNoDup[i] /= fSumW;

      *ppMats = arrMatsNoDup.GetElements();
      *ppWeights = arrWeightsNoDup.GetElements();
      *pMatsNum = arrMatsNoDup.Count();
    }
  }

  return (uint16)fIsoValue;
}

uint16 CVoxTerrain::GetCachedVoxelValue(Vec3i vPos, float fGridSize, int * pMatId, IsoOctree * pVoxNode, const AABB & nodeBox)
{
  Vec3i vPosS;
  assert(fGridSize == 2.f);
  vPosS.x = vPos.x>>1;
  vPosS.y = vPos.y>>1;
  vPosS.z = vPos.z>>1;
  SCachedVoxVal & rCache = m_arrCacheVoxVal[vPosS.x&(nVoxCacheSize-1)][vPosS.y&(nVoxCacheSize-1)][vPosS.z&(nVoxCacheSize-1)];
  if(rCache.vPos != vPos)
  {
    rCache.vPos = vPos;
    float fIsoVal = pVoxNode->GetVoxelValueF(rCache.vPos, fGridSize, &rCache.nMatId, false, nodeBox);
    rCache.usVal = uint16(fIsoVal*VOX_MAX_USHORT) & ~VOX_MAT_MASK;
  }

  *pMatId = rCache.nMatId;

  return rCache.usVal;
}
*/
void CVoxTerrain::ResetHeightMapCache()
{
  m_arrElevCache.Clear();
//  memset(m_arrCacheVoxVal,0,sizeof(m_arrCacheVoxVal));
  //assert(sizeof(m_arrCacheVoxVal[0][0]) == 8);
}
/*
void BlurImage8(ColorB * pImage, int nSize, int nPassesNum)
{
#define DATA_TMP(_x,_y) (pTemp [(_x&nMask)+nSize*(_y&nMask)])
#define DATA_IMG(_x,_y) (pImage[(_x&nMask)+nSize*(_y&nMask)])

  ColorB * pTemp = new ColorB [nSize*nSize];

  for(int nPass=0; nPass<nPassesNum; nPass++)
  {
    cryMemcpy(pTemp,pImage,nSize*nSize);

    int nMask = nSize-1;
    for(int x=0; x<nSize; x++)
    {
      for(int y=0; y<nSize; y++)
      {
        ColorF colSumm(0,0,0,0);
        float fCount=0;
        for(int _x = x-1; _x <= x+1; _x++)
        {
          for(int _y = y-1; _y <= y+1; _y++)
          {
            colSumm.r += 1.f/255.f*DATA_TMP(_x,_y).r;
            colSumm.g += 1.f/255.f*DATA_TMP(_x,_y).g;
            colSumm.b += 1.f/255.f*DATA_TMP(_x,_y).b;
            colSumm.a += 1.f/255.f*DATA_TMP(_x,_y).a;
            fCount ++;
          }
        }

        colSumm /= fCount;

        pImage[x*nSize+y] = colSumm;
      }
    }
  }

  delete [] pTemp;

#undef DATA_IMG
#undef DATA_TMP
}
*/

void CVoxTerrain::GetTrianglesInAABB(const AABB & aabb, PodArray<Vec3> & lstVerts, PodArray<Vec3> & lstNorms, PodArray<uint16> & lstIndices)
{
  OctNode::NodeIndex nIdx;
  if(CIsoOctreeThread::m_pIsoOctree)
    CIsoOctreeThread::m_pIsoOctree->GetTrianglesInAABB(&CIsoOctreeThread::m_pIsoOctree->renderTree, nIdx, aabb, lstVerts, lstNorms, lstIndices);
}

void CVoxTerrain::GetTrianglesInSphere(const Sphere & sp,PodArray<Vec3> & lstVerts, PodArray<Vec3> & lstNorms, PodArray<uint16> & lstIndices)
{
  OctNode::NodeIndex nIdx;
  if(CIsoOctreeThread::m_pIsoOctree)
    CIsoOctreeThread::m_pIsoOctree->GetTrianglesInSphere(&CIsoOctreeThread::m_pIsoOctree->renderTree, nIdx, sp, lstVerts, lstNorms, lstIndices);
}

#include UNIQUE_VIRTUAL_WRAPPER(IVoxTerrain)

Vec3 CVoxTerrain::GetTerrainSurfaceNormal3D(Vec3 vPos, float fRange)
{ 
  Vec3 vNorm;
  GetElevation3D(vPos, vNorm);
  return vNorm;
}

char * CVoxTerrain::GetStatusString(int nLine)
{
  if(!CIsoOctreeThread::m_pIsoOctree)
    return 0;

  IsoOctree * pIsoOctree = CIsoOctreeThread::m_pIsoOctree;

  // current thread job
  string szCurrentMessageIn;
  for(int i=0; i<ISO_THREADS_NUM && i<GetCVars()->e_VoxTerThreads; i++)
  {
    if(i)
      szCurrentMessageIn += "+";
    szCurrentMessageIn += m_pIsoOctreeThreads[i]->szCurrentMessageIn;
    szCurrentMessageIn.resize(8 + (8+1)*i);
  }

  // measure time spend until all meshes are ready
  static float fStartTime = GetCurTimeSec();
  static float fLastTimeTestResult = -1;
  if(pIsoOctree->GetProgress() == pIsoOctree->m_nodesForUpdate.Count())
  { // finished
    if(!fLastTimeTestResult)
      fLastTimeTestResult = GetCurTimeSec() - fStartTime;
  }
  else
  { // in progress
    if(fLastTimeTestResult)
      fStartTime = GetCurTimeSec();
    fLastTimeTestResult = 0;
  }

  const int nMaxLines = 3;
  static char szInfo[nMaxLines][256]={"","",""};
  if(m_bEditor)
  {
    sprintf(szInfo[0], "VoxMesh: P%4d A%4d T%4d R%4d TE%4d RE%4d %.1fs", 
      pIsoOctree->GetProgress(), pIsoOctree->m_nodesForUpdate.Count(), 
      pIsoOctree->threadMeshes.size(), CIsoOctreeThread::pRenderMeshes->size(), 
      pIsoOctree->threadEmptyMeshes.size(), CIsoOctreeThread::pRenderEmptyMeshes->size(), 
      fLastTimeTestResult);

    sprintf(szInfo[1], "VoxMem: FrameTex%2dm SrcTex%3dm FrameMesh%2d AllMesh%2dm",
      SIsoMesh::m_nThisFrameTextureMemoryUsage/1024/1024, m_nReMeshTexStreamedIn/1024/1024,
      SIsoMesh::m_nThisFrameMeshMemoryUsage/1024/1024, m_nMeshAmountStreamedIn/1024/1024);

    sprintf(szInfo[2], "VoxTasks: In%4d [%8s] Out%2d",
			CIsoOctreeThread::m_nMessagesIn, szCurrentMessageIn.c_str(), CIsoOctreeThread::m_nMessagesOut);
  }
  else
  {
    sprintf(szInfo[0], "Vox: R%4d E%4d L%3d T%4d Tex=%2d/%2dm Mesh=%2d/%2dm", 
      CIsoOctreeThread::pRenderMeshes->size(), 
      CIsoOctreeThread::pRenderEmptyMeshes->size(), 
      m_nMeshesStreamedIn, SIsoMesh::GetCounter()->Count(),
      SIsoMesh::m_nThisFrameTextureMemoryUsage/1024/1024, m_nReMeshTexStreamedIn/1024/1024,
      SIsoMesh::m_nThisFrameMeshMemoryUsage/1024/1024, m_nMeshAmountStreamedIn/1024/1024);
  }

  return (nLine<nMaxLines && szInfo[nLine][0]) ? &szInfo[nLine][0] : NULL;
}

void CVoxTerrain::GetMemoryUsage(ICrySizer * pSizer)
{
  SIZER_COMPONENT_NAME(pSizer, "VoxTerrain");

  m_bAllowEditing = false;
  AUTO_LOCK(g_cIsoTreeAccess);

  if(CIsoOctreeThread::m_pIsoOctree)
    CIsoOctreeThread::m_pIsoOctree->GetMemoryUsage(pSizer);

  {
    SIZER_COMPONENT_NAME(pSizer, "IsoOctree_renderMeshesMap");
    pSizer->AddObject(&CIsoOctreeThread::pRenderMeshes,stl::size_of_map(*CIsoOctreeThread::pRenderMeshes));
  }

  {
    SIZER_COMPONENT_NAME(pSizer, "IsoOctree_renderEmptyMeshesMap");
    pSizer->AddObject(&CIsoOctreeThread::pRenderEmptyMeshes,stl::size_of_map(*CIsoOctreeThread::pRenderEmptyMeshes));
  }

  {
    if(PodArray<SIsoMesh*> * pObjects = SIsoMesh::GetCounter())
    {
      for(int i=0; i<pObjects->Count(); i++)
      {
        SIsoMesh * pMesh = pObjects->GetAt(i);
        pMesh->GetMemoryUsage(pSizer);
      }
    }
  }

  {
    SIZER_COMPONENT_NAME(pSizer, "VoxTerrainDebugTargets");
    
    for(int i=0; i<INDEX_TARGETS_NUM; i++)
      pSizer->AddObject(m_arrTargetIndex[i].arrData.GetData(), m_arrTargetIndex[i].arrData.GetDataSize());
    pSizer->AddObject(m_arrTargetCounter.GetData(), m_arrTargetCounter.GetDataSize());
    pSizer->AddObject(IsoOctree_visNodes.GetElements(), IsoOctree_visNodes.GetDataSize());
  }
}

template <class T, bool bABS> 
int TCompare(const void* v1, const void* v2)
{
  T*p1 = (T*)v1;
  T*p2 = (T*)v2;

  if(bABS)
  {
    if(abs(*p1)>abs(*p2))
      return -1;
    else if(abs(*p1)<abs(*p2))
      return 1;
  }
  else
  {
    if((*p1)>(*p2))
      return -1;
    else if((*p1)<(*p2))
      return 1;
  }

  return 0;
}

/*void CVoxTerrain::VoxelizeMesh(CStatObj * pStatObj, const Matrix34A & objMat, int nSurfaceTypeId, ColorB color, float fWSStep, float _fRadius)
{
  int nPaintPoins = 0;
 
  AABB OSBBox = pStatObj->GetAABB();
  float fSize = max(max(OSBBox.GetSize().x, OSBBox.GetSize().y),OSBBox.GetSize().z);

  IMaterial * m_pMaterial = pStatObj->GetMaterial();

  float fStep = fWSStep / 2;

  float fTime = Get3DEngine()->GetCurAsyncTimeSec();

  Get3DEngine()->PrintMessage("Voxelizing mesh: fStep = %.2f, fSize = %.2f", fStep, fSize);

  int nXStepsNum = (int)((OSBBox.max.x-OSBBox.min.x) / fStep);
  int nProgressStepsNum = nXStepsNum / 80;
  int nCurStepId = 0;

  OctNode::NodeIndex nIdx;
  IsoOctree & isoTree = *Get3DEngine()->m_pVoxTerrain->CIsoOctreeThread::m_pIsoOctree;

  Sphere sp(objMat.TransformPoint(OSBBox.GetCenter())/CVoxTerrain::m_fMapSize, OSBBox.GetRadius()/CVoxTerrain::m_fMapSize);
  SBrushAABB * pBrushAABB = GetBrushAABB(sp, eveoCreate, 1.f, sp.radius);

  Get3DEngine()->PrintMessage("Forcing requested depth ..." );

  isoTree.ForceDepth(&isoTree.tree, nIdx, sp, pBrushAABB, eveoCreate);

  isoTree.PrintMessagePlus(" ok" );
  isoTree.PrintMessage("Working (press ESC to abort) ." );

  for(float x=OSBBox.min.x; x<=OSBBox.max.x; x+=fStep)
  {
    for(float y=OSBBox.min.y; y<=OSBBox.max.y; y+=fStep)
    {
      if(!Get3DEngine()->m_pVoxTerrain->IsContinue())
        return;

      static PodArray<float> arrSurfaceZ; arrSurfaceZ.Clear();

      {
        Vec3 vStart = (Vec3(x,y,OSBBox.max.z));
        Vec3 vEnd = (Vec3(x,y,OSBBox.min.z));
        Vec3 vStartCur = vStart;

        bool bHit = true;
        while(bHit) 
        {
          SRayHitInfo hitInfo;
          hitInfo.inRay = Ray(vStartCur, (vEnd-vStart));
          hitInfo.inReferencePoint = vStartCur;

          if(bHit = pStatObj->RayIntersection(hitInfo,m_pMaterial))
          {
            vStartCur.z = hitInfo.vHitPos.z - 0.0001f;
            Vec3 vCurWS = (vStartCur);
            arrSurfaceZ.Add((vCurWS.z+1000.f));
          }
        }
      }

      {
        Vec3 vStart = (Vec3(x,y,OSBBox.min.z));
        Vec3 vEnd = (Vec3(x,y,OSBBox.max.z));
        Vec3 vStartCur = vStart;

        bool bHit = true;
        while(bHit) 
        {
          SRayHitInfo hitInfo;
          hitInfo.bOnlyZWrite = true;
          hitInfo.inRay = Ray(vStartCur, (vEnd-vStart));
          hitInfo.inReferencePoint = vStartCur;

          if(bHit = pStatObj->RayIntersection(hitInfo,m_pMaterial))
          {
            vStartCur.z = hitInfo.vHitPos.z + 0.0001f;
            Vec3 vCurWS = (vStartCur);
            arrSurfaceZ.Add(-(vCurWS.z+1000.f));
          }
        }
      }

      if(arrSurfaceZ.Count()==1 && arrSurfaceZ[0]>0)
        arrSurfaceZ.Add(-(OSBBox.min.z+1000.f));

      if(arrSurfaceZ.Count()>=2)
      {
        qsort(arrSurfaceZ.GetElements(), arrSurfaceZ.Count(), sizeof(arrSurfaceZ[0]), TCompare<float,true>);

        for(int i=0; (i+1)<arrSurfaceZ.Count();)
        {
          while(arrSurfaceZ.Count()>i && arrSurfaceZ[i]<0)
            i++;
          if(i>=arrSurfaceZ.Count())
            break;

          float zTop = fabs(arrSurfaceZ[i])-1000.f;

          while(arrSurfaceZ.Count()>i && arrSurfaceZ[i]>0)
            i++;
          if(i>=arrSurfaceZ.Count())
            break;

          float zBot = fabs(arrSurfaceZ[i])-1000.f;

          for(float z=zTop-fStep/4; z>zBot; z-=fStep)
          {
            Vec3 vPos(x,y,z);

            vPos = objMat.TransformPoint(vPos);

            Sphere sp(vPos/CVoxTerrain::m_fMapSize, fStep/CVoxTerrain::m_fMapSize*3);

            SBrushAABB * pBrushAABB = GetBrushAABB(sp, eveoCreate, 1.f, sp.radius);

            isoTree.setFromSphere(sp, IsoOctree::maxDepth, 0, fIsoTreeCurvature, eveoCreate, 0!=0, nSurfaceTypeId, color, pBrushAABB, NULL);

            nPaintPoins++;
          }
        }
      }
    }
  
    if(!nProgressStepsNum || !(nCurStepId % nProgressStepsNum))
      isoTree.PrintMessagePlus(" .");

    nCurStepId++;
  }

  isoTree.PrintMessagePlus(" done (%d pp, %.1f sec)", nPaintPoins, isoTree.GetCurAsyncTimeSec() - fTime);
}*/

void CVoxTerrain::RegisterLightSource(CDLight * pLight)
{
  FUNCTION_PROFILER_3DENGINE;

  OctNode::NodeIndex nIdx;

  CIsoOctreeThread::m_pIsoOctree->RegisterLightSource(&CIsoOctreeThread::m_pIsoOctree->renderTree, nIdx, pLight);
}

void CVoxTerrain::RequestTextureUpdate(AABB areaBox)
{
  OctNode::NodeIndex nIdx;
  areaBox.min -= Vec3(1.f,1.f,1.f);
  areaBox.max += Vec3(1.f,1.f,1.f);
  if(m_bEditor && CIsoOctreeThread::m_pIsoOctree && CIsoOctreeThread::m_pIsoOctree->m_nodesForUpdate.Count())
    CIsoOctreeThread::m_pIsoOctree->RequestTextureUpdate(&CIsoOctreeThread::m_pIsoOctree->renderTree, nIdx, areaBox);
}

void CVoxTerrain::PhysicalizeContentOfBBox(const AABB & areaBox)
{  
#ifdef _DEBUG
  Vec3 vCamPos = GetCamera().GetPosition();
  Vec3 vRange(16,16,16);
  AABB camBox(vCamPos-vRange,vCamPos+vRange); 
  if(!Overlap::AABB_AABB(camBox,areaBox))
    return;
#endif // _DEBUG

  if(m_bEditor && CIsoOctreeThread::m_pIsoOctree && !GetCVars()->e_VoxTerHeightmapEditing)
    CIsoOctreeThread::MSGI_PhysicalizeContentOfBBox(areaBox, true);
}

void CVoxTerrain::DephysicalizeContentOfBBox(const AABB & areaBox)
{
  if(CIsoOctreeThread::m_pIsoOctree && !GetCVars()->e_VoxTerHeightmapEditing)
    CIsoOctreeThread::MSGI_PhysicalizeContentOfBBox(areaBox, false);
}

void CVoxTerrain::DrawEditingHelper(const Sphere & sp, EVoxelEditOperation eOperation, IMaterial * pHelperMat)
{
  if(sp.radius<0.01f || sp.radius>100.f || !pHelperMat)
    return;

  ColorB col(65,154,235,25);
  GetRenderer()->GetIRenderAuxGeom()->DrawSphere(sp.center,sp.radius,col);

  static PodArray<Vec3> arrVerts; arrVerts.Clear();
  static PodArray<Vec3> arrNorms; arrNorms.Clear();
  static PodArray<uint16> arrInds; arrInds.Clear();

  GetTrianglesInSphere(sp,arrVerts,arrNorms,arrInds);

  if(arrVerts.Count()>0xFFFF || !arrVerts.Count() || !arrInds.Count())
    return;

  for(int i=0; i<arrInds.Count(); i+=3)
  {
    Triangle t;

    t.v0 = arrVerts[arrInds[i+0]];
    t.v1 = arrVerts[arrInds[i+1]];
    t.v2 = arrVerts[arrInds[i+2]];

    float fArea = t.GetArea();
    if(fArea<0.001f)
      fArea=fArea;

    if( fArea<0.0001f || t.v0.IsEquivalent(t.v1,0.01f) || t.v1.IsEquivalent(t.v2,0.01f) || t.v2.IsEquivalent(t.v0,0.01f) )
    {
      arrInds.Delete(i,3);
      i-=3;
    }
  }

/*
  for(int i=0; i<arrInds.Count(); i+=3)
  {
    Vec3 v[3];
    v[0] = arrVerts[arrInds[i+0]];
    v[1] = arrVerts[arrInds[i+1]];
    v[2] = arrVerts[arrInds[i+2]];

    Vec3 n[3];
    n[0] = arrNorms[arrInds[i+0]];
    n[1] = arrNorms[arrInds[i+1]];
    n[2] = arrNorms[arrInds[i+2]];

    SAuxGeomRenderFlags flags;
    flags.SetFillMode( e_FillModeWireframe );
    GetRenderer()->GetIRenderAuxGeom()->SetRenderFlags(flags);
    GetRenderer()->GetIRenderAuxGeom()->DrawTriangle(v[0],col,v[1],col,v[2],col);
  }
  //  for(int i=0; i<arrVerts.Count(); i++)
  //  GetRenderer()->GetIRenderAuxGeom()->DrawLine(arrVerts[i],col,arrVerts[i]+arrNorms[i],col);

*/

  PodArray<SVF_P3F_C4B_T2F> arrVertBuffer;
  arrVertBuffer.PreAllocate(arrVerts.Count(),arrVerts.Count());
  for(int i=0; i<arrVerts.Count(); i++)
  {
    SVF_P3F_C4B_T2F tmp;
    tmp.xyz = arrVerts[i];
    tmp.color.dcolor = ~0;

    Matrix33 mat;
    if( arrNorms[i].dot(Vec3(0,0,1)) > .9f )
      mat.SetIdentity();
    else
      mat = Matrix33::CreateRotationV0V1( arrNorms[i], Vec3(0,0,1) );

    Vec3 v0t = mat.TransformVector(arrVerts[i] - sp.center);

    tmp.st.x = v0t.x*0.5f/sp.radius;// + 0.5f;
    tmp.st.y = v0t.y*0.5f/sp.radius;// + 0.5f;

    arrVertBuffer[i] = tmp;
  }

  if(!m_pEditorHelper)
  {
    GetRenderer()->DeleteRenderMesh(m_pEditorHelper);
    m_pEditorHelper = GetRenderer()->CreateRenderMeshInitialized(
      arrVertBuffer.GetElements(),arrVertBuffer.Count(), eVF_P3F_C4B_T2F,
      arrInds.GetElements(),arrInds.Count(), R_PRIMV_TRIANGLES, 
      "Helper","Helper", eRMT_Static, 1, 0, NULL, NULL, false, true );        
  }
  else
  {
    m_pEditorHelper->UpdateVertices(arrVertBuffer.GetElements(), arrVertBuffer.Count(), 0, VSF_GENERAL);
    m_pEditorHelper->UpdateIndices(arrInds.GetElements(), arrInds.Count());
  }

	float texelAreaDensity = GetRenderer()->CalculateTexelAreaDensity(arrVertBuffer,arrInds, "VoxelTerrain");

  m_pEditorHelper->SetChunk(pHelperMat,0,arrVertBuffer.Count(),0,arrInds.Count(),texelAreaDensity);

  Vec3 r(sp.radius,sp.radius,sp.radius);
  AABB box(sp.center-r, sp.center+r);
  m_pEditorHelper->SetBBox(box.min, box.max);

  CRenderObject * pObj = GetIdentityCRenderObject();
  pObj->m_II.m_Matrix.SetIdentity();
  pObj->m_ObjFlags |= (FOB_INSHADOW | FOB_NO_FOG | FOB_TRANS_TRANSLATE);
  GetObjManager()->PrecacheMaterial(pHelperMat,pObj->m_fDistance,NULL);
  m_pEditorHelper->SetMaterial(pHelperMat);
  m_pEditorHelper->Render(pObj);
}

bool CVoxTerrain::RayIntersection(const Ray & r, Vec3 & vHitPoint)
{
  if(CIsoOctreeThread::m_pIsoOctree)
  {
    AUTO_LOCK(g_cIsoTreeAccess);
    OctNode::NodeIndex nIdx;
    return CIsoOctreeThread::m_pIsoOctree->RayIntersection(&CIsoOctreeThread::m_pIsoOctree->tree,nIdx,r,vHitPoint);
  }

  return 0;
}

void CVoxTerrain::RequestPhysicsUpdate()
{
  OctNode::NodeIndex nIdx;
  if(CIsoOctreeThread::m_pIsoOctree && !GetCVars()->e_VoxTerHeightmapEditing)
  {
    AUTO_LOCK(g_cIsoTreeAccess);
    CIsoOctreeThread::m_pIsoOctree->RegisterBBoxInPODGrid(&CIsoOctreeThread::m_pIsoOctree->tree, nIdx);
  }
}

Vec3 CVoxTerrain::ProjectFromScreen(const float & x, const float & y, const float & z, int nRes)
{
  Vec3 vRes;
  CCamera tmpCam = m_pRenderer->GetCamera();
  m_pRenderer->SetCamera(m_Camera);
  m_pRenderer->UnProjectFromScreen( 
    x*m_pRenderer->GetWidth()/nRes, 
    y*m_pRenderer->GetHeight()/nRes, 
    z, &vRes.x, &vRes.y, &vRes.z );
  m_pRenderer->SetCamera(tmpCam);
  return vRes;
}

extern CVoxTerrain::STargetMipData * IsoOctree_pTargets;
extern Array2d<uint8> * IsoOctree_pTargetsCounter;
extern float * IsoOctree_pMatCombined;
extern CCamera IsoOctree_camNorm;
extern Vec3 IsoOctree_camNormPos;
extern PodArray<SOcNodeInfo> IsoOctree_visNodes;

void CVoxTerrain_ScaleMatrix(float * m, float fScale)
{
#define M(row,col)  m[col*4+row]
  M(0, 0) *= fScale ; M(0, 1) *= fScale ; M(0, 2) *= fScale ;
  M(1, 0) *= fScale ; M(1, 1) *= fScale ; M(1, 2) *= fScale ;
  M(2, 0) *= fScale ; M(2, 1) *= fScale ; M(2, 2) *= fScale ;
  M(3, 0) *= fScale ; M(3, 1) *= fScale ; M(3, 2) *= fScale ;
#undef M
}

void CVoxTerrain::RasterizeVolume(const int _nRes)
{ // project buffer to the screen
  AUTO_LOCK(g_cIsoTreeAccess);

  FUNCTION_PROFILER_3DENGINE;

  if(GetCVars()->e_VoxTerDebug == 2)
    return;

  {
    FRAME_PROFILER( "CVoxTerrain::RasterizeVolume_ResetImages", GetSystem(), PROFILE_3DENGINE );

    for(int i=0; i<INDEX_TARGETS_NUM; i++)
    {
      m_arrTargetIndex[i].arrData.Allocate(nResIndex);
      memset(m_arrTargetIndex[i].arrData.GetData(),0,m_arrTargetIndex[i].arrData.GetDataSize());
    }

    m_arrTargetCounter.Allocate(nResIndex);
    memset(m_arrTargetCounter.GetData(),0,m_arrTargetCounter.GetDataSize());
  }

  // make matrices
  float matModel[16]; memset(matModel, 0, sizeof(matModel));
  float matProj[16]; memset(matProj, 0, sizeof(matProj));
  CCamera tmpCam = m_pRenderer->GetCamera();
  m_pRenderer->SetCamera(m_Camera);
  m_pRenderer->GetModelViewMatrix(matModel);
  m_pRenderer->GetProjectionMatrix(matProj);
  float matCombined[16];
  CCoverageBuffer::MatMul4(matCombined,matProj,matModel);
  m_pRenderer->SetCamera(tmpCam);
  CVoxTerrain_ScaleMatrix(&matCombined[0], m_fMapSize);

  // render into target
  OctNode::NodeIndex nIdx;
  int nFirstNodeSlotId = CIsoOctreeThread::m_pIsoOctree->m_nCurrentNodeSlotId;
  {
    FRAME_PROFILER( "CVoxTerrain::RasterizeVolume_RasterizeVolume", GetSystem(), PROFILE_3DENGINE );
    
    IsoOctree_camNorm = GetCamera();
    IsoOctree_camNormPos = GetCamera().GetPosition()/m_fMapSize;
    for(int p=0; p<6; p++)
    {
      Plane plane = *GetCamera().GetFrustumPlane(p);
      plane.d /= m_fMapSize;
      IsoOctree_camNorm.SetFrustumPlane(p,plane);
    }

    IsoOctree_pTargets = &m_arrTargetIndex[0];
    IsoOctree_pTargetsCounter = &m_arrTargetCounter;
    IsoOctree_pMatCombined = &matCombined[0];

    int nMaxElements = (nResData*nResData)/nNodeDataSize;
    IsoOctree_visNodes.PreAllocate(nMaxElements,nMaxElements);

#ifdef VOX_DVR
    CIsoOctreeThread::m_pIsoOctree->RasterizeVolume(&CIsoOctreeThread::m_pIsoOctree->tree, nIdx, true, false);
#endif
//    if(IsoOctree_visNodes.Count()>=65535)
  //    PrintMessage("vis nodes = %d", IsoOctree_visNodes.Count());
  }
  int nLastNodeSlotId = CIsoOctreeThread::m_pIsoOctree->m_nCurrentNodeSlotId;

  {
    int nId=0;

    for(; nId<INDEX_TARGETS_NUM; nId++)
    {
      FRAME_PROFILER( "CVoxTerrain::RasterizeVolume_DownLoadToVideoMemory_Index", GetSystem(), PROFILE_3DENGINE );
      char szName[256];
      sprintf(szName, "VoxDebugIndex_%d", nId);
      m_arrTargetTexId[nId] = GetRenderer()->DownLoadToVideoMemory((uint8*)m_arrTargetIndex[nId].arrData.GetData(), 
        m_arrTargetIndex[nId].arrData.GetSize(), m_arrTargetIndex[nId].arrData.GetSize(), 
        eTF_A16B16G16R16, eTF_A16B16G16R16, 0, false, FILTER_POINT, m_arrTargetTexId[nId], szName);
    }

    {
      assert(sizeof(SOcNodeInfo)/sizeof(Vec4_tpl<uint16>) == nNodeDataSize);
      FRAME_PROFILER( "CVoxTerrain::RasterizeVolume_DownLoadToVideoMemory_Node", GetSystem(), PROFILE_3DENGINE );
      char szName[256];
      sprintf(szName, "VoxDebugData_%d", nId);

      if(m_arrTargetTexId[nId])
      { // update
        if(nFirstNodeSlotId != nLastNodeSlotId)
        {
          RectI updateRegion;
          updateRegion.x = 0;
          updateRegion.w = nResData;

          if(nFirstNodeSlotId < nLastNodeSlotId)
          { // single update
            updateRegion.y = nFirstNodeSlotId*nNodeDataSize/nResData;
            updateRegion.h = min(nLastNodeSlotId*nNodeDataSize/nResData - updateRegion.y + 1, nResData-updateRegion.y);

            m_arrTargetTexId[nId] = GetRenderer()->DownLoadToVideoMemory((uint8*)(IsoOctree_visNodes.GetElements() + (updateRegion.y*nResData/nNodeDataSize)), 
              nResData, nResData, eTF_A16B16G16R16, eTF_A16B16G16R16, 0, false, FILTER_POINT, m_arrTargetTexId[nId], szName, 0, GetPlatformEndian(), &updateRegion);
          }
          else if(nFirstNodeSlotId > nLastNodeSlotId)
          { // double update
            // first
            updateRegion.y = nFirstNodeSlotId*nNodeDataSize/nResData;
            updateRegion.h = nResData - updateRegion.y;

            m_arrTargetTexId[nId] = GetRenderer()->DownLoadToVideoMemory((uint8*)(IsoOctree_visNodes.GetElements() + (updateRegion.y*nResData/nNodeDataSize)), 
              nResData, nResData, eTF_A16B16G16R16, eTF_A16B16G16R16, 0, false, FILTER_POINT, m_arrTargetTexId[nId], szName, 0, GetPlatformEndian(), &updateRegion);

            // second
            updateRegion.y = 0;
            updateRegion.h = min(nLastNodeSlotId*nNodeDataSize/nResData+1, nResData);

            m_arrTargetTexId[nId] = GetRenderer()->DownLoadToVideoMemory((uint8*)(IsoOctree_visNodes.GetElements() + (updateRegion.y*nResData/nNodeDataSize)), 
              nResData, nResData, eTF_A16B16G16R16, eTF_A16B16G16R16, 0, false, FILTER_POINT, m_arrTargetTexId[nId], szName, 0, GetPlatformEndian(), &updateRegion);
          }
        }
      }
      else
      { // create
        m_arrTargetTexId[nId] = GetRenderer()->DownLoadToVideoMemory((uint8*)IsoOctree_visNodes.GetElements(), 
          nResData, nResData, 
          eTF_A16B16G16R16, eTF_A16B16G16R16, 0, false, FILTER_POINT, m_arrTargetTexId[nId], szName, 0, GetPlatformEndian());
      }
    }
  }

  GetRenderer()->ProcessVoxTerrainDebugTarget(&m_arrTargetTexId[0], INDEX_TARGETS_NUM+1);

  /*if(GetCVars()->e_VoxTerDebug == 2)
  {
    Vec3 vSize(.4f,.4f,.4f);

    int nRes = m_arrTarget[TARGET_MIPS_NUM-1].arrData.GetSize();

    m_pRenderer->Set2DMode(true,nRes,nRes);

    // show target
    for(int x=0; x<nRes; x++)
    {
      for(int y=0; y<nRes; y++)
      {
        if(int nVal = m_arrTarget[TARGET_MIPS_NUM-1].arrData.GetData()[y*nRes+x].w)
        {
          float fBr = 1.f/255.f/255.f*nVal;
          ColorB col((uint8)(fBr*255), (uint8)(fBr*255), (uint8)(fBr*255), (uint8)255);
          Vec3 vPos((float)x,(float)y,0);
          GetRenderer()->GetIRenderAuxGeom()->DrawAABB(AABB(vPos-vSize, vPos+vSize), true, col, eBBD_Faceted);
        }
      }
    }

    m_pRenderer->Set2DMode(false,nRes,nRes);
  }*/
}

void CVoxTerrain::DrawDebug(int nRes)
{ // project buffer to the screen
  AUTO_LOCK(g_cIsoTreeAccess);

  FUNCTION_PROFILER_3DENGINE;

  m_pRenderer->Set2DMode(true,nRes,nRes);

  Vec3 vSize(.4f,.4f,.4f);

  Vec3 vWSPos0 = GetCamera().GetPosition();

  for(int x=0; x<nRes; x++)
  {
    for(int y=0; y<nRes; y++)
    {
      Vec3 vPos((float)x,(float)y,0);

      Vec3 vWSPos1 = ProjectFromScreen(vPos.x, nRes-vPos.y, 1.f, nRes);

      Ray r(vWSPos0/m_fMapSize,(vWSPos1-vWSPos0)/m_fMapSize);
      r.direction.NormalizeFast();

      OctNode::NodeIndex nIdx;
      Vec3 vHitPoint;
      bool bHit = CIsoOctreeThread::m_pIsoOctree->RayIntersectionNew(r,vHitPoint);

      float fBr = 0;
      if(bHit)
        fBr = 1;//SATURATE(r.origin.GetDistance(vHitPoint)*m_fMapSize / GetCamera().GetFarPlane());

      ColorB col((uint8)(fBr*255), (uint8)(fBr*255), /*(m_farrBuffer[x][y]>0) ? */(uint8)(fBr*255)/* : 255*/, (uint8)255);
      
      GetRenderer()->GetIRenderAuxGeom()->DrawAABB(AABB(vPos-vSize, vPos+vSize), true, col, eBBD_Faceted);
    }
  }

  m_pRenderer->Set2DMode(false,nRes,nRes);
}

void CVoxTerrain::OnMouse(bool bUp)
{
  if(CIsoOctreeThread::m_pIsoOctree && !CIsoOctreeThread::m_pIsoOctree->m_aabbPhysDirty.IsReset() && !GetCVars()->e_VoxTerHeightmapEditing)
  {
    gEnv->pPhysicalWorld->RegisterBBoxInPODGrid(&(CIsoOctreeThread::m_pIsoOctree->m_aabbPhysDirty.min));
    CIsoOctreeThread::m_pIsoOctree->m_aabbPhysDirty.Reset();
  }
}

int CVoxTerrain::nSeekMax = 0;
int32 CVoxTerrain::arrActLog[CVoxTerrain::nActLogRes];

void CVoxTerrain::DrawStreamingActivity()
{ // project buffer to the screen
  AUTO_LOCK(g_cIsoTreeAccess);

  FUNCTION_PROFILER_3DENGINE;

  m_pRenderer->Set2DMode(true,nActLogRes,nActLogRes);

  int nBarHeight = 8;

  for(int x=0; x<nActLogRes; x++)
  {
    Vec3 vPos(x, nActLogRes-nBarHeight, 0);
    float fBr = (float)((uint32)arrActLog[x] > GetMainFrameID()-100);
    ColorB colBk = Col_Black;
    ColorB colSignal = Col_White;//((uint8)(fBr*255), (uint8)(fBr*255), /*(m_farrBuffer[x][y]>0) ? */(uint8)(fBr*255)/* : 255*/, (uint8)255);

    Vec3 vSizeMax(.4f, nBarHeight, 0);
    GetRenderer()->GetIRenderAuxGeom()->DrawAABB(AABB(vPos-vSizeMax, vPos+vSizeMax), true, colBk, eBBD_Faceted);

    Vec3 vSize(.4f, nBarHeight * SATURATE(1.f - float(GetMainFrameID()-arrActLog[x]) / 100.f), 0);
    GetRenderer()->GetIRenderAuxGeom()->DrawAABB(AABB(vPos-vSize, vPos+vSize), true, colSignal, eBBD_Faceted);
  }

  m_pRenderer->Set2DMode(false,nActLogRes,nActLogRes);
}

void CVoxTerrain::LogStreamingActivity(int nSeek)
{
  nSeek = int((float)nSeek / (float)nSeekMax * (float)nActLogRes);
  nSeek = CLAMP(nSeek,0,nActLogRes-1);
  arrActLog[nSeek] = GetMainFrameID();
}

void CVoxTerrain::SetStreamingActivityMax(int _nSeekMax)
{
  nSeekMax = _nSeekMax;
  memset(arrActLog,0,sizeof(arrActLog));
}

void CVoxTerrain::ProcessNodesForUpdate()
{
	int nSurfType = 0;

	for(int id=0; id<Get3DEngine()->m_arrBaseTextureData.Count(); id++)
	{
		SImageInfo & rImgInfo = Get3DEngine()->m_arrBaseTextureData[id];
		if(rImgInfo.fUseRemeshing > 0)
		{
			nSurfType = id;
			break;
		}
	}

	const float fMinVoxelSize = 0.0625; 

	Vec3 vBaseColor(15.f/255.f,15.f/255.f,15.f/255.f);


	if(m_arrNodesForUpdate.Count()>2)
		return;

	for(int m=0; m<m_arrNodesForUpdate.Count(); m++)
	{
		IRenderNode * pNode = m_arrNodesForUpdate[m];
		AABB updateArea = pNode->GetBBox();
		Sphere sp(Vec3(0,0,0), 0.0001f);
		updateArea.min -= Vec3(.125f,.125f,.125f);
		updateArea.max += Vec3(.125f,.125f,.125f);
		sp.center = updateArea.GetCenter();
		sp.radius = updateArea.GetRadius();

		// subtract if selected
		if(pNode->GetRndFlags()&ERF_SELECTED)
		{
			PodArray<IRenderNode*> arrNodes;
			arrNodes.Add(pNode);
			Get3DEngine()->Voxel_Paint(sp.center, sp.radius, nSurfType, vBaseColor, eveoIntegrateMeshNeg, evbsSphere, evetVoxelObjects, &arrNodes, fMinVoxelSize*4.f);
		}

		// re-integrate terrain
		if( GetCVars()->e_VoxTerOnTheFlyIntegration==2 )
		{
			Sphere sph(Vec3(0,0,0), 0.0001f);
			sph.center = updateArea.GetCenter();
			sph.radius = updateArea.GetRadius()*.6f;
			sph.center.z = Get3DEngine()->GetTerrainElevation(sph.center.x,sph.center.y);
			gEnv->p3DEngine->Voxel_Paint(sph.center, sph.radius, -1, vBaseColor, eveoCopyTerrainPos, evbsSphere, evetVoxelObjects, NULL, 2.f);
		}

		// re-add affected
		{
			PodArray<IRenderNode*> arrNodes;

			if(pNode->GetRndFlags()&ERF_SELECTED)
			{
				PodArray<IRenderNode*> arrNodesAround;
				Get3DEngine()->GetObjectsByTypeGlobal(arrNodesAround, eERType_Brush, &updateArea, 1.f);
				for(int i=0; i<arrNodesAround.Count(); i++)
				{
					if(!(arrNodesAround[i]->GetRndFlags()&ERF_SELECTED))
						arrNodes.Add(arrNodesAround[i]);
				}
			}
			else
			{
				arrNodes.Add(pNode);
			}

			if(arrNodes.Count())	
				DoVoxelShape(sp.center, sp.radius, nSurfType, vBaseColor, eveoIntegrateMeshPos, evbsSphere, &arrNodes, fMinVoxelSize*4.f, &updateArea);
		}
	}

	m_arrNodesForUpdate.Clear();
	
}

#if !defined(PS3) && !defined(LINUX)
#pragma pack(push,1)

typedef struct
{
  char signature[4]; // Signature of the file format (always UPCf)
  UCHAR versionMajor; // The major version number for the file
  UCHAR versionMinor; // The minor version number for the file
  USHORT headerSize; // Size of the header block
  unsigned __int64 numOfPts; // The number of points within the file
  double xScale; // The scale used in the x-coord
  double yScale; // The scale used in the y-coord
  double zScale; // The scale used in the z-coord
  double xOffset; // The offset used in the x-coord
  double yOffset; // The offset used in the y-coord
  double zOffset; // The offset used in the z-coord
} UPC_HEADER;

typedef struct
{
  double gpsTimeOfWeek; // The GPS time of week of the point
  char sensorNum[2]; // The laser sensor number used for the point
  char julianDay[3]; // The day the point was collected
  char flightLine[3]; // The flight line number of the point
  int x; // The recorded x-coord of the point
  int y; // The recorded y-coord of the point
  int z; // The recorded z-coord of the point
  UCHAR intensity; // The intensity of the point
  USHORT red; // The red component of the point
  USHORT green; // The green component of the point
  USHORT blue; // The blue component of the point
  char returnNum; // The return number of the point
} UPC_DATA_10;

#pragma pack(pop)
#endif

void CVoxTerrain::ImportUPC()
{
#if !defined(PS3)	&& !defined(LINUX) 	// disabled for PS3 because we dont have UCHAR and USHORT as types  //[K01]: disabled for LINUX as well
																			// and i didn't wanted to mess with these since they are loaded directly.
																			// valdimir, please take a look at these and fix the code for PS3
  float IMU_Laser_X[] = {-0.088f, -0.271f, 0.045f, -0.185f, -0.041f };
  float IMU_Laser_Y[] = { 0.239f, 0.198f, 0.198f, -0.257f, -0.261f };
  float IMU_Laser_Z[] = { 0.04f, 0.072f, -0.072f, -0.115f, 0.126f };

  FILE * f = 0;//fopen("PointCloudData/000007.upc","rb");
  if(!f)
    return;

  UPC_HEADER header;
  fread(&header, 1, sizeof(header), f);        

  // find min/max
  AABB cloudBox;
  cloudBox.Reset();
  for(int i=0; i<header.numOfPts; i++)
  {
    UPC_DATA_10 pointData;
    fread(&pointData, 1, sizeof(pointData), f);
    Vec3 vPos(pointData.x,pointData.y,pointData.z);
    cloudBox.Add(vPos);
  }

  fseek(f,sizeof(header),SEEK_SET);

  int nDetailLevel = 0;

  float fTerrainSize = Get3DEngine()->GetTerrainSize();
  float fVoxSize = .5f;
  while(fTerrainSize > fVoxSize)
  {
    fTerrainSize/=2;
    nDetailLevel++;
  }

  Vec3 vSize = cloudBox.GetSize();

  GetCVars()->e_VoxTerLog=0;
  m_bAllowEditing = true;

  float fTimeStart = GetCurAsyncTimeSec();

  for(int i=0; i<header.numOfPts; i++)
  {
    UPC_DATA_10 pointData;

    fread(&pointData, 1, sizeof(pointData), f);

    Vec3 vPos(pointData.x,pointData.y,pointData.z);
    vPos -= cloudBox.min;
    vPos.x /= vSize.x;
    vPos.y /= vSize.y;
    vPos.z /= vSize.z*2;

    ColorB col(SATURATEB(pointData.red),SATURATEB(pointData.green),SATURATEB(pointData.blue));

    //if(vPos.GetDistance(Vec3(.5f,.5f,.5f)) < .5f)
    {
      AABB meshBox; meshBox.Reset();
      CIsoOctreeThread::m_pIsoOctree->DoVoxelShapeTask( vPos*m_fMapSize, fVoxSize, 0, col, 
        eveoCreate, evbsSphere, nDetailLevel, NULL, 
        NULL, NULL, meshBox, 0, 1, fTimeStart, false);
    }

    if((i&(256*32-1)) == 0)
    {
      PrintMessage("Integrating point %.1f / %dK / %dK", 
        float(i)*100.f/float(header.numOfPts), 
        i/1000, 
        header.numOfPts/1000);
    }
  }

  fclose(f);

  AABB meshBox(Vec3(0,0,0),Vec3(1,1,1));
  OctNode::NodeIndex nIdxGlobal;
  CIsoOctreeThread::m_pIsoOctree->RequestMeshUpdate(&CIsoOctreeThread::m_pIsoOctree->tree, nIdxGlobal, meshBox, meshBox);

  m_bAllowEditing = false;
  GetCVars()->e_VoxTerLog=1;
#endif
}

void CVoxTerrain::RequestMeshUpdate(AABB aabb)
{
  AABB meshBox(aabb.min / m_fMapSize, aabb.max / m_fMapSize);
  OctNode::NodeIndex nIdxGlobal;
  if(CIsoOctreeThread::m_pIsoOctree)
    CIsoOctreeThread::m_pIsoOctree->RequestMeshUpdate(&CIsoOctreeThread::m_pIsoOctree->tree, nIdxGlobal, meshBox, meshBox);
}

bool CVoxTerrain::IsEditing()
{
  return IsoOctree::m_lastEditingTime > Get3DEngine()->GetCurTimeSec() - gEditingRenderModeDelay;
}

SSurfTypeInfo CVoxTerrain::GetLayerAtPosition( const int x, const int y )
{
  assert(GetCVars()->e_VoxTerHeightmapEditingCustomLayerInfo);
  int nWrap = m_arrSurfType2D.GetSize()-1;
  return m_arrSurfType2D[x&nWrap][y&nWrap];
}

void CVoxTerrain::PaintLayerId( const float fpx, const float fpy, const float radius, const float hardness, const uint32 dwLayerId )
{
  if(!GetCVars()->e_VoxTerHeightmapEditing || !m_arrSurfType2D.GetSize() || !GetCVars()->e_VoxTerHeightmapEditingCustomLayerInfo)
    return;

  float fTS = (float)Get3DEngine()->GetTerrainSize();																			// in m
  int nWrap = m_arrSurfType2D.GetSize()-1;

  float X = fpx*fTS;
  float Y = fpy*fTS;
  float R = radius*fTS;

  SSurfTypeInfo newType = dwLayerId;

  int x1 = (int)floor(X-R);
  int x2 = (int)ceil (X+R);
  int y1 = (int)floor(Y-R);
  int y2 = (int)ceil (Y+R);

  for(int x=x1; x<=x2; x++) for(int y=y1; y<=y2; y++)
  {
    float fDist = sqrt(sqr(X-(float)x)+sqr(Y-(float)y));
    float t = SATURATE(1.f-fDist/R) * hardness;
    m_arrSurfType2D[x&nWrap][y&nWrap].Lerp(1.f-t, newType, m_arrSurfType2D[x&nWrap][y&nWrap]);
  }
}

void CVoxTerrain::SetLayerData(void * pData, int nDataSize)
{
  assert(GetCVars()->e_VoxTerHeightmapEditingCustomLayerInfo);
  if(!GetCVars()->e_VoxTerHeightmapEditingCustomLayerInfo)
    return;

  int nDim = (int)sqrt((float)nDataSize/sizeof(SSurfTypeInfo));
  PrintMessage("Loading layer data: %d x %d, Size = %d MB", nDim, nDim, nDataSize/1024/1024);
  m_arrSurfType2D.SetData((SSurfTypeInfo*)pData, nDim);
}

void CVoxTerrain::GetLayerData(void ** pData, int & nDataSize)
{
  nDataSize = m_arrSurfType2D.GetDataSize();
  int nDim = (int)sqrt((float)nDataSize/sizeof(SSurfTypeInfo));
  if(pData)
  {
    PrintMessage("Exporting layer data: %d x %d, Size = %d MB", nDim, nDim, nDataSize/1024/1024);
    *pData = m_arrSurfType2D.GetData();
  }
}
