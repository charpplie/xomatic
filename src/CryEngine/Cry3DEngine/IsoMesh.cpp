#include "StdAfx.h"

#include "VoxMan.h"

#include "IsoMesh.h"

#include "IndexedMesh.h"
#include "MeshCompiler/MeshCompiler.h"

#include "IChunkFile.h"
#include "CGFContent.h"
#include "CGF/ChunkFile.h"
#include "CGF/CGFSaver.h"
#include "CGF/CGFLoader.h"
#include "CGF/ReadOnlyChunkFile.h"

#include "MatMan.h"
#include "VoxTerrain.h"

int SIsoMesh::m_nThisFrameTextureMemoryUsage = 0;
int SIsoMesh::m_nThisFrameMeshMemoryUsage = 0;
int IsoOctree_nModId = 0;

float fPhysMeshSize = 16.f;

namespace
{
  CryCriticalSection g_cIsoMeshCounter;
  CryCriticalSection g_cMixMasksMap2D;
}

struct SMixMask2D
{
  SMixMask2D() { ZeroStruct(*this); }
  bool bTexUpdateNeeded;
  bool bTexDataUpdateNeeded;
  int arrNodeStreamedTextures[VOX_TEX_NUM_SRC];
  ColorB * arrNodeStreamedTexturesData[VOX_TEX_NUM_SRC];
  int nLastUseFrameId;
  PodArray<unsigned short> * pPixMats;
};

std::map<int,SMixMask2D> mixMasksMap2D;

void SIsoMesh::ResetRenderMeshs()
{
//  for(int l=0; l<MAX_SURFACE_TYPES_COUNT; l++)
  //  m_arrSurfaceTypeInfos[l].DeleteRenderMeshes(GetRenderer());

  GetRenderer()->DeleteRenderMesh(m_pRenderMesh);

  m_pRenderMesh = NULL;
}

void SIsoMesh::LoadMeshFromMemBlock( CMemoryBlock * pMemBlock, CMesh * pMesh )
{
  EEndian eEndian = eLittleEndian;

  assert(pMesh);

  byte * pData = (byte *)pMemBlock->GetData();

  assert(strncmp((char*)pData, "CryTek", 6)==0);
  pData+=6;

  SwapEndian((AABB*)pData, 1, eEndian);
  memcpy(&pMesh->m_bbox, pData, sizeof(AABB));
  pData+=sizeof(AABB);

  // Positions
  {
    SwapEndian((uint32*)pData, 1, eEndian);
    pMesh->m_streamSize[CMesh::POSITIONS] = *(int*)pData;

    pMesh->m_numVertices = *(int*)pData;
    pData+=sizeof(pMesh->m_numVertices);

    pMesh->m_pPositions = new Vec3[pMesh->m_numVertices];

    Vec3 vBoxSize = pMesh->m_bbox.GetSize();

    SwapEndian((uint16*)pData, pMesh->m_numVertices*3, eEndian);

    for(int i=0; i<pMesh->m_numVertices; i++)
    {
      Vec3 vPos = *(Vec3_tpl<uint16>*)pData;

      vPos *= 1.f / (256.f*256.f - 1.f);

      vPos.x = pMesh->m_bbox.min.x + vPos.x * vBoxSize.x;
      vPos.y = pMesh->m_bbox.min.y + vPos.y * vBoxSize.y;
      vPos.z = pMesh->m_bbox.min.z + vPos.z * vBoxSize.z;

      pMesh->m_pPositions[i] = vPos;

      pData += sizeof(Vec3_tpl<uint16>);
    }
  }

  // VertMats
  {
    SwapEndian((uint32*)pData, 1, eEndian);
    pMesh->m_streamSize[CMesh::VERT_MATS] = *(int*)pData;

    pMesh->m_numVertices = *(int*)pData;
    pData+=sizeof(pMesh->m_numVertices);

    pMesh->m_pVertMats = new int[pMesh->m_numVertices];

    for(int i=0; i<pMesh->m_numVertices; i++)
    {
      pMesh->m_pVertMats[i] = *(byte*)pData;

      pData += sizeof(byte);
    }
  }

  // Norms
  {
    SwapEndian((uint32*)pData, 1, eEndian);
    pMesh->m_streamSize[CMesh::NORMALS] = *(int*)pData;

    pMesh->m_numVertices = *(int*)pData;
    pData+=sizeof(pMesh->m_numVertices);

    pMesh->m_pNorms = new Vec3[pMesh->m_numVertices];

    for(int i=0; i<pMesh->m_numVertices; i++)
    {
      Vec3 vNorm = *(Vec3_tpl<byte>*)pData;

      pMesh->m_pNorms[i].x = (vNorm.x - 127.f) / 127.f;
      pMesh->m_pNorms[i].y = (vNorm.y - 127.f) / 127.f;
      pMesh->m_pNorms[i].z = (vNorm.z - 127.f) / 127.f;

      pData += sizeof(Vec3_tpl<byte>);
    }
  }

  // Color0
  {
    SwapEndian((uint32*)pData, 1, eEndian);
    pMesh->m_streamSize[CMesh::COLORS_0] = *(int*)pData;

    pMesh->m_numVertices = *(int*)pData;
    pData+=sizeof(pMesh->m_numVertices);

    pMesh->m_pColor0 = new SMeshColor[pMesh->m_numVertices];

    for(int i=0; i<pMesh->m_numVertices; i++)
    {
      pMesh->m_pColor0[i] = *(SMeshColor*)pData;

      pData += sizeof(SMeshColor);
    }
  }

  // Color1
  {
    SwapEndian((uint32*)pData, 1, eEndian);
    pMesh->m_streamSize[CMesh::COLORS_1] = *(int*)pData;

    pMesh->m_numVertices = *(int*)pData;
    pData+=sizeof(pMesh->m_numVertices);

    pMesh->m_pColor1 = new SMeshColor[pMesh->m_numVertices];

    for(int i=0; i<pMesh->m_numVertices; i++)
    {
      pMesh->m_pColor1[i] = *(SMeshColor*)pData;

      pData += sizeof(SMeshColor);
    }
  }

  // TC
  {
    SwapEndian((uint32*)pData, 1, eEndian);
    pMesh->m_streamSize[CMesh::TEXCOORDS] = *(int*)pData;

    pMesh->m_nCoorCount = *(int*)pData;
    pData+=sizeof(pMesh->m_nCoorCount);

    pMesh->m_pTexCoord = new SMeshTexCoord[pMesh->m_nCoorCount];

    SwapEndian((uint16*)pData, pMesh->m_numVertices*2, eEndian);

    for(int i=0; i<pMesh->m_nCoorCount; i++)
    {
      Vec2 vTC = *(Vec2_tpl<uint16>*)pData;

      vTC *= 1.f / (256.f*256.f - 1.f);

      pMesh->m_pTexCoord[i].s = vTC.x;
      pMesh->m_pTexCoord[i].t = vTC.y;

      pData += sizeof(Vec2_tpl<uint16>);
    }
  }

  // Indices
  {
    SwapEndian((uint32*)pData, 1, eEndian);
    pMesh->m_streamSize[CMesh::INDICES] = *(int*)pData;

    pMesh->m_nIndexCount = *(int*)pData;
    pData+=sizeof(pMesh->m_nIndexCount);

    pMesh->m_pIndices = new uint16[pMesh->m_nIndexCount];

    SwapEndian((uint16*)pData, pMesh->m_nIndexCount, eEndian);

    for(int i=0; i<pMesh->m_nIndexCount; i++)
    {
      uint16 nId = *(uint16*)pData;

      pMesh->m_pIndices[i] = nId;

      pData += sizeof(uint16);
    }
  }

  { // pix mats
    SwapEndian((uint32*)pData, 1, eEndian);
    int nPixMatsCount = *(int*)pData;
    pData+=sizeof(nPixMatsCount);

    if(nPixMatsCount)
    {
      pMesh->m_pPixMats = new PodArray<unsigned short>;
      SwapEndian((uint16*)pData, nPixMatsCount, eEndian);
      pMesh->m_pPixMats->AddList((uint16*)pData, nPixMatsCount);
      pData+=nPixMatsCount*sizeof(uint16);
    }
  }

  SMeshSubset sub;
  sub.nFirstIndexId = 0;
  sub.nNumIndices = pMesh->m_nIndexCount;
  sub.nFirstVertId = 0;
  sub.nNumVerts = pMesh->m_numVertices;
  pMesh->m_subsets.push_back(sub);

  if((int)(pData - (byte *)pMemBlock->GetData()) != pMemBlock->m_nSize)
    Error("SIsoMesh::LoadMeshFromMemBlock: pData - (byte *)pMemBlock->GetData() != pMemBlock->m_nSize");
}

void SIsoMesh::MakeRenderMesh()
{
  FUNCTION_PROFILER_3DENGINE;

  ResetRenderMeshs();

  if(m_pGeneratedMesh)
  { 
    CreateRenderMeshFromIndexedMesh(m_pGeneratedMesh->GetMesh());
  }
  else if(m_pMeshForSerialization)
  { 
    CMemoryBlock * pUnpacked = CMemoryBlock::DecompressFromMemBlock(m_pMeshForSerialization, GetSystem());

    if(!pUnpacked)
    {
      Get3DEngine()->CheckMemoryHeap();
      return Error("SIsoMesh::MakeRenderMesh: CMemoryBlock::DecompressFromMemBlock failed");
    }

    assert(strncmp((char *)pUnpacked->GetData(),"CryTek",6)==0);

    CMesh * pMesh = new CMesh;

    LoadMeshFromMemBlock( pUnpacked, pMesh );

    delete pUnpacked;

    CreateRenderMeshFromIndexedMesh(pMesh);

    delete pMesh;
  }

  if(!m_pRenderMesh)
    m_pRenderMesh=0;
}

IRenderMesh * SIsoMesh::CreateRenderMeshFromIndexedMesh(CMesh * pMatMesh)
{
  SAFE_RELEASE(m_pRenderMesh);

  // set nInUse flag
  if(pMatMesh->m_pPixMats)
  {
    for(int i=0; i<pMatMesh->m_pPixMats->Count(); i++)
    {
      unsigned short nSurfType = (*pMatMesh->m_pPixMats)[i];
      assert(nSurfType<MAX_SURFACE_TYPES_COUNT);
      m_arrSurfaceTypeInfos[nSurfType].nInUse = 1;
    }
  }
  else
  {
    for(int i=0; i<pMatMesh->m_numVertices; i++)
    {
      uint32 nSurfType = pMatMesh->m_pColor0 ? pMatMesh->m_pColor0[i].g : 0;
      assert(nSurfType<MAX_SURFACE_TYPES_COUNT);
      m_arrSurfaceTypeInfos[nSurfType].nInUse = 1;
    }
  }

  m_pRenderMesh = GetRenderer()->CreateRenderMesh("IsoMesh", "IsoMesh");
  const Vec3 & vOrigin = GetOrigin();
  m_pRenderMesh->SetMesh( *pMatMesh, 0, FSM_VOXELS, &vOrigin );
  m_pRenderMesh->SetBBox( pMatMesh->m_bbox.min-vOrigin, pMatMesh->m_bbox.max-vOrigin );
  m_pRenderMesh->UpdateBBoxFromMesh();
  m_pRenderMesh->SetMaterial(Get3DEngine()->m_pVoxTerrainMat);

  for(int s=0; s<MAX_SURFACE_TYPES_COUNT; s++)
  {
//    m_arrSurfaceTypeInfos[s].pSurfaceType = NULL;
/*
    SImageInfo * pImageInfo = Get3DEngine()->GetBaseTextureData(s);
    if(m_arrSurfaceTypeInfos[s].pSurfaceType = pImageInfo ? pImageInfo->pSurfType : NULL)
      if(IMaterial * pMat = m_arrSurfaceTypeInfos[s].pSurfaceType->pLayerMat)
    {
      Get3DEngine()->InitMaterialDefautMappingAxis(pMat);
      m_arrSurfaceTypeInfos[s].pSurfaceType->ucDefProjAxis = pMat->m_ucDefautMappingAxis = 'Z';
      m_arrSurfaceTypeInfos[s].pSurfaceType->fScale = pImageInfo->detailInfo.fTiling;
    }
*/
  }
/*
  if(!GetCVars()->e_TerrainDetailMaterials)
    return NULL;

  // build all indices
  short m_arrpNonBorderIdxNum[MAX_SURFACE_TYPES_COUNT][4];
  CTerrainNode::GenerateIndicesForAllSurfaces(m_pRenderMesh, 0, m_arrpNonBorderIdxNum, 0, &m_arrSurfaceTypeInfos[0]);

  static PodArray<SSurfaceType *> lstReadyTypes; lstReadyTypes.Clear(); // protection from duplications in palette of types

  for(int i=0; i<MAX_SURFACE_TYPES_COUNT; i++)
  {
    if(SSurfaceType * pSType = m_arrSurfaceTypeInfos[i].pSurfaceType)
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
            CTerrainNode::UpdateSurfaceRenderMeshes(m_pRenderMesh, pSType, m_arrSurfaceTypeInfos[i].arrpRM[p], p, lstIndices, "VoxelMaterialLayer", false, 0);
          }
        }
        lstReadyTypes.Add(pSType);
      }
    }
  }
*/
  if(pMatMesh->m_bbox.GetSize().x == fPhysMeshSize && !m_bEditor && pMatMesh->m_pVertMats)
  {
    PhysicalizeMesh(pMatMesh, NULL);
    EnablePhysEntity(true);
  }

  return NULL;
}

void SIsoMesh::EnablePhysEntity(bool bEnable)
{
  if(bEnable && !m_pPhysEnt && m_pPhysGeom)
  {
    assert(!m_pPhysEnt);
    m_pPhysEnt = GetPhysicalWorld()->CreatePhysicalEntity(PE_STATIC,NULL,NULL,PHYS_FOREIGN_ID_STATIC);

    pe_action_remove_all_parts remove_all;
    m_pPhysEnt->Action(&remove_all);

    pe_geomparams params;	  
    m_pPhysEnt->AddGeometry(m_pPhysGeom, &params);

    pe_params_flags par_flags;
    par_flags.flagsOR = pef_never_affect_triggers;
    m_pPhysEnt->SetParams(&par_flags);
    /*
    pe_params_pos par_pos;
    par_pos.pMtx3x4 = &m_Matrix;
    m_pPhysEnt->SetParams(&par_pos);
    */
    pe_params_foreign_data par_foreign_data;
    par_foreign_data.pForeignData = NULL;//(IRenderNode*)this;
    par_foreign_data.iForeignData = PHYS_FOREIGN_ID_STATIC;

    par_foreign_data.iForeignFlags |= PFF_EXCLUDE_FROM_STATIC;
    m_pPhysEnt->SetParams(&par_foreign_data);
  }
  else if(!bEnable && m_pPhysEnt)
  {
    m_pPhysEnt->RemoveGeometry(0);
    GetPhysicalWorld()->DestroyPhysicalEntity(m_pPhysEnt, 0, 1);
    m_pPhysEnt = NULL;
  }
}

void SIsoMesh::PhysicalizeMesh(CMesh * pMesh, std::vector<char> * physData)
{
  FUNCTION_PROFILER_3DENGINE;

  Dephysicalize();

  int flags = mesh_multicontact1 | mesh_AABB | mesh_no_vtx_merge | mesh_always_static;
/*
#if defined(XENON) || defined(PS3)
  flags |= mesh_shared_vtx;
#endif
*/
  int arrSurfaceTypesId[MAX_SURFACE_TYPES_COUNT];
  memset(arrSurfaceTypesId,0,sizeof(arrSurfaceTypesId));

  // make indices
  PodArray<char> lstMatIndices; lstMatIndices.Clear();
  for (int i=0; i<pMesh->m_nIndexCount; i+=3)
  {
    char nSurfType0 = pMesh->m_pVertMats[pMesh->m_pIndices[i+0]];
    char nSurfType1 = pMesh->m_pVertMats[pMesh->m_pIndices[i+1]];
    char nSurfType2 = pMesh->m_pVertMats[pMesh->m_pIndices[i+2]];

    char nSurfType = nSurfType0;
    if(nSurfType1 == nSurfType2 && nSurfType1 != nSurfType0)
      nSurfType = nSurfType1;

    if(nSurfType>=0 && nSurfType<MAX_SURFACE_TYPES_COUNT && nSurfType<Get3DEngine()->m_arrBaseTextureData.Count())
    {
      SImageInfo & rImgInfo = Get3DEngine()->m_arrBaseTextureData[nSurfType];
      arrSurfaceTypesId[nSurfType] = rImgInfo.nPhysSurfaceType;
    }

    lstMatIndices.Add(nSurfType);
  }

  if(!lstMatIndices.Count())
    return;

  if(physData && physData->size())
  {
    CMemStream stm( &physData->front(), physData->size(), true );
    m_pPhysGeom = GetPhysicalWorld()->GetGeomManager()->LoadPhysGeometry(stm,
      pMesh->m_pPositions,pMesh->m_pIndices,&lstMatIndices[0]);
  }
  else
  {/*
#if defined(XENON) || defined(PS3)
    int nStride=0;
    byte * pPos = m_pRenderMesh->GetStridedPosPtr(nStride);
    strided_pointer<Vec3> arrVerts((Vec3*)pPos, nStride);
#else*/
    strided_pointer<Vec3> arrVerts((Vec3*)pMesh->m_pPositions);
//#endif

    IGeomManager *pGeoman = GetPhysicalWorld()->GetGeomManager();
    IGeometry * pGeom = pGeoman->CreateMesh(arrVerts, pMesh->m_pIndices, 
      lstMatIndices.GetElements(), NULL, pMesh->m_nIndexCount/3, flags, 0.05f, 2, 31);
    m_pPhysGeom = pGeoman->RegisterGeometry(pGeom);
    pGeom->Release();
  }

  if(!m_pPhysGeom)
    return Error("IsoMesh physicalization failed");

  GetPhysicalWorld()->GetGeomManager()->SetGeomMatMapping( m_pPhysGeom, arrSurfaceTypesId, MAX_SURFACE_TYPES_COUNT );

//  EnablePhysEntity(true);
}

void SIsoMesh::Dephysicalize( bool bKeepIfReferenced )
{
  if(m_pPhysEnt)
  {
    m_pPhysEnt->RemoveGeometry(0);
    GetPhysicalWorld()->DestroyPhysicalEntity(m_pPhysEnt, 0, 1);
    m_pPhysEnt = NULL;
  }

  if(m_pPhysGeom)
  {
    GetPhysicalWorld()->GetGeomManager()->UnregisterGeometry(m_pPhysGeom);
    m_pPhysGeom = NULL;
  }
}

//int nReportIsoMeshDelete = 1;

SIsoMesh::~SIsoMesh() 
{ 
  if(m_pReadStream)
  {
    m_pReadStream->Wait();
    m_pReadStream=0;
  }

  /*
  if(nReportIsoMeshDelete)
  {
    assert(0);
    GetSystem()->FatalError( "SIsoMesh::~SIsoMesh" );
  }
*/
  Dephysicalize(); 
  ResetRenderMeshs();

  Get3DEngine()->UnRegisterEntity(this);
  Get3DEngine()->FreeRenderNodeState(this);

  ReleaseTextures();

  SAFE_DELETE(m_pMeshForSerialization);
  SAFE_DELETE(m_pGeneratedMesh);

  {
    AUTO_LOCK(g_cIsoMeshCounter);
    GetCounter()->Delete(this);
  }
  
  for(int i=0; i<VOX_TEX_NUM_SRC; i++)
    SAFE_DELETE(m_arrSrcTexData[i]);

  if(!GetCVars()->e_VoxTerHeightmapEditing)
    SAFE_DELETE(m_pZipData);
}

int SIsoMesh::GetCompiledMeshDataSize(bool bSaveForEditing, int nSectorId)
{
  int nSize = 0;

  // size
  nSize += sizeof(uint32);

  if(m_pGeneratedMesh)
    SerializeMesh(m_pGeneratedMesh);

  // data
  if(m_pMeshForSerialization)
    nSize += m_pMeshForSerialization->GetSize();

  // align
  while(nSize&3)
    nSize++;

  // texture header
  nSize += sizeof(uint32)*2;

  // compressed texture data size
  if(!bSaveForEditing)
  {
    if(m_nBuildTexturesFrameId != GetMainFrameID())
    {
      bool bReUseOld = !GetCVars()->e_VoxTerUpdateTextures;

      for(int t=0; t<2; t++)
      {
        m_arrTexturesZipSize[t] = 0;

        if(bReUseOld)
        {
          char szFileName[256]="";
          MakeTempTextureFileName(szFileName, sizeof(szFileName),t,"BIN",m_key);
          
          int nFSize = 0;
          if(FILE * f = fopen(szFileName, "rb"))
          {
            fseek(f, 0, SEEK_END); 
            nFSize = ftell(f);
            fclose(f);
          }

          if(nFSize>0)
            m_arrTexturesZipSize[t] = nFSize;
          else
            bReUseOld = false;
        }
      }

      if(!bReUseOld)
      {
        char szFileName[256]="";
        MakeTempTextureFileName(szFileName, sizeof(szFileName),0,"BIN",m_key);
        if(!(nSectorId%10))
        {
          const char * pFormatName = GetRenderer()->GetTextureFormatName((ETEX_Format)GetCVars()->e_VoxTerTexFormat);
          PrintMessage("Generating texture for segment %d [%s,%3dx%3d], %s", nSectorId, pFormatName, m_nTexDimX, m_nTexDimY, szFileName);
        }
        
        m_arrTexturesZipSize[0]=m_arrTexturesZipSize[1]=0;
        BuildTextures(true);

        int nTexDownScale = 1<<GetCVars()->e_VoxTerTexResExportDownscale;

        if(!(nSectorId%10) && nTexDownScale>1)
          PrintMessagePlus(" -> [%3dx%3d]", m_nTexDimX/nTexDownScale, m_nTexDimY/nTexDownScale);
      }

      m_nBuildTexturesFrameId = GetMainFrameID();
    }

    for(int t=0; t<2; t++)
    {
      nSize += m_arrTexturesZipSize[t];

      // align
      while(nSize&3)
        nSize++;
    }
  }

  // texture size X
  nSize += sizeof(uint32);

  // texture size Y
  nSize += sizeof(uint32);

  // aabb
  nSize += sizeof(m_WSBBox);

  // RGB range
  nSize += sizeof(m_arrTexRangeInfo);

  // align
  while(nSize&3)
    nSize++;

  m_nDiskUsage = nSize;

  return nSize;
}

byte * SIsoMesh::GetCompiledMeshData(int & nSizeOut, EEndian eEndian, bool bSaveForEditing, int nSectorId)
{
//  assert(IsHeapValid());

  int nSize = nSizeOut = GetCompiledMeshDataSize(bSaveForEditing, nSectorId);

  byte * pData = new byte[nSizeOut];

  byte * nPtr = pData;

  // data size
  int nMeshBytesCount = m_pMeshForSerialization ? m_pMeshForSerialization->GetSize() : 0;
  AddToPtr(nPtr,nSize,nMeshBytesCount,eEndian);

  // data
  if(nMeshBytesCount)
    AddToPtr(nPtr,nSize,(byte*)m_pMeshForSerialization->GetData(),m_pMeshForSerialization->GetSize(),eEndian,true);
  
  // compressed texture data

  if(!bSaveForEditing)
  {
    // sizes
    for(int t=0; t<2; t++)
    {
      uint32 nTexDataSize = m_arrTexturesZipSize[t];
      AddToPtr(nPtr,nSize,nTexDataSize,eEndian);
    }

    // data
    for(int t=0; t<2; t++)
    {
      if(m_arrTexturesZipSize[t])
      {
        byte * pZipData = new byte[m_arrTexturesZipSize[t]];

        char szFileName[256]="";
        MakeTempTextureFileName(szFileName, sizeof(szFileName),t,"BIN",m_key);
        if(FILE * f = fopen(szFileName,"rb"))
        {
          fread(pZipData, 1, m_arrTexturesZipSize[t], f);
          fclose(f);
        }

        AddToPtr(nPtr,nSize,pZipData,m_arrTexturesZipSize[t],eEndian,true);

        delete [] pZipData;
      }
    }
  }
  else
  {
    // sizes
    for(int t=0; t<2; t++)
    {
      uint32 nTexDataSize = 0;
      AddToPtr(nPtr,nSize,nTexDataSize,eEndian);
    }
  }

  int nTexDownScale = bSaveForEditing ? (1) : (1<<GetCVars()->e_VoxTerTexResExportDownscale);

  uint32 nTexDimX = m_nTexDimX / nTexDownScale;
  AddToPtr(nPtr,nSize,nTexDimX,eEndian);

  uint32 nTexDimY = m_nTexDimY / nTexDownScale;
  AddToPtr(nPtr,nSize,nTexDimY,eEndian);

  AddToPtr(nPtr,nSize,m_WSBBox,eEndian);

  // RGB range
  AddToPtr(nPtr, nSize, &m_arrTexRangeInfo[0][0], sizeof(m_arrTexRangeInfo)/sizeof(float), eEndian);

  assert(nSize == 0);

  return pData;
}

void SIsoMesh::SerializeMesh( CMesh * pMesh )
{
  FUNCTION_PROFILER_3DENGINE;

  SAFE_DELETE(m_pMeshForSerialization);

  if(pMesh)
  {
    if(pMesh->m_nCoorCount != pMesh->m_numVertices)
      Error("SIsoMesh::SerializeMeshL: pMesh->m_nCoorCount != pMesh->m_numVertices");

    PodArray<byte> arrUncomp;
    arrUncomp.AddList((byte*)"CryTek",6);

    arrUncomp.AddList((byte*)&m_WSBBox, sizeof(AABB));

    // Pos
    {
      Vec3_tpl<uint16> * pVerts = new Vec3_tpl<uint16>[pMesh->m_numVertices];
      for(int i=0; i<pMesh->m_numVertices; i++)
      {
        Vec3 vSize = m_WSBBox.GetSize();
        Vec3 vPosNorm = (pMesh->m_pPositions[i] - m_WSBBox.min);
        vPosNorm.x /= vSize.x;
        vPosNorm.y /= vSize.y;
        vPosNorm.z /= vSize.z;
        pVerts[i].x = (uint16)CLAMP(vPosNorm.x*(256.f*256.f-1.f), 0, (256.f*256.f-1.f));
        pVerts[i].y = (uint16)CLAMP(vPosNorm.y*(256.f*256.f-1.f), 0, (256.f*256.f-1.f));
        pVerts[i].z = (uint16)CLAMP(vPosNorm.z*(256.f*256.f-1.f), 0, (256.f*256.f-1.f));
      }
      arrUncomp.AddList((byte*)&pMesh->m_numVertices,sizeof(pMesh->m_numVertices));
      arrUncomp.AddList((byte*)pVerts,sizeof(pVerts[0])*pMesh->m_numVertices);
      delete [] pVerts;
    }

    // VertMat
    {
      byte * pVertMats = new byte[pMesh->m_numVertices];
      for(int i=0; i<pMesh->m_numVertices; i++)
      {
        pVertMats[i] = pMesh->m_pVertMats[i];
      }
      arrUncomp.AddList((byte*)&pMesh->m_numVertices,sizeof(pMesh->m_numVertices));
      arrUncomp.AddList((byte*)pVertMats,sizeof(pVertMats[0])*pMesh->m_numVertices);
      delete [] pVertMats;
    }

    // Norm
    {
      Vec3_tpl<byte> * pNorms = new Vec3_tpl<byte>[pMesh->m_numVertices];
      for(int i=0; i<pMesh->m_numVertices; i++)
      {
        Vec3 vN = pMesh->m_pNorms[i]*127.f;
        pNorms[i].x = (byte)SATURATEB(vN.x + 127.f);
        pNorms[i].y = (byte)SATURATEB(vN.y + 127.f);
        pNorms[i].z = (byte)SATURATEB(vN.z + 127.f);
      }
      arrUncomp.AddList((byte*)&pMesh->m_numVertices,sizeof(pMesh->m_numVertices));
      arrUncomp.AddList((byte*)pNorms,sizeof(pNorms[0])*pMesh->m_numVertices);
      delete [] pNorms;
    }

    // Color0
    {
      SMeshColor * pCol0 = new SMeshColor[pMesh->m_numVertices];
      for(int i=0; i<pMesh->m_numVertices; i++)
      {
        pCol0[i] = pMesh->m_pColor0[i];
      }
      arrUncomp.AddList((byte*)&pMesh->m_numVertices,sizeof(pMesh->m_numVertices));
      arrUncomp.AddList((byte*)pCol0,sizeof(pCol0[0])*pMesh->m_numVertices);
      delete [] pCol0;
    }

    // Color1
    {
      SMeshColor * pCol1 = new SMeshColor[pMesh->m_numVertices];
      for(int i=0; i<pMesh->m_numVertices; i++)
      {
        pCol1[i] = pMesh->m_pColor1[i];
      }
      arrUncomp.AddList((byte*)&pMesh->m_numVertices,sizeof(pMesh->m_numVertices));
      arrUncomp.AddList((byte*)pCol1,sizeof(pCol1[0])*pMesh->m_numVertices);
      delete [] pCol1;
    }

    // TC
    {
      Vec2_tpl<uint16> * pTC = new Vec2_tpl<uint16>[pMesh->m_nCoorCount];
      for(int i=0; i<pMesh->m_nCoorCount; i++)
      {
        Vec2 vTCNorm;
        vTCNorm.x = CLAMP(pMesh->m_pTexCoord[i].s*(256.f*256.f-1.f), 0, (256.f*256.f-1.f));
        vTCNorm.y = CLAMP(pMesh->m_pTexCoord[i].t*(256.f*256.f-1.f), 0, (256.f*256.f-1.f));
        pTC[i].x = (uint16)vTCNorm.x;
        pTC[i].y = (uint16)vTCNorm.y;
      }
      arrUncomp.AddList((byte*)&pMesh->m_nCoorCount,sizeof(pMesh->m_nCoorCount));
      arrUncomp.AddList((byte*)pTC,sizeof(pTC[0])*pMesh->m_nCoorCount);
      delete [] pTC;
    }

    {
      uint16 * pIndices = new uint16[pMesh->m_nIndexCount];
      for(int i=0; i<pMesh->m_nIndexCount; i++)
      {
        pIndices[i] = pMesh->m_pIndices[i];
      }
      arrUncomp.AddList((byte*)&pMesh->m_nIndexCount,sizeof(pMesh->m_nIndexCount));
      arrUncomp.AddList((byte*)pIndices,sizeof(pIndices[0])*pMesh->m_nIndexCount);
      delete [] pIndices;
    }

    if(pMesh->m_pPixMats)
    {
      int nCount = pMesh->m_pPixMats->Count();
      arrUncomp.AddList((byte*)&nCount,sizeof(nCount));
      arrUncomp.AddList((byte*)pMesh->m_pPixMats->GetElements(),pMesh->m_pPixMats->GetDataSize());
    }
    else
    {
      int nCount = 0;
      arrUncomp.AddList((byte*)&nCount,sizeof(nCount));
    }

    m_pMeshForSerialization = CMemoryBlock::CompressToMemBlock(arrUncomp.GetElements(), arrUncomp.GetDataSize(), GetSystem());
    assert(m_pMeshForSerialization);
  }
  else
    Error("SIsoMesh::SerializeMeshL: !pMesh");
}
/*
void SIsoMesh::SavePhysicalizeData( CNodeCGF *pNode )
{
  if (!m_pPhysGeom)
    return;

  CMemStream stm(false);
  IGeomManager *pGeoman = GetPhysicalWorld()->GetGeomManager();
  pGeoman->SavePhysGeometry( stm,m_pPhysGeom );

  // Add physicalized data to the node.
  pNode->physicalGeomData[0].resize(stm.GetUsedSize());
  memcpy( &pNode->physicalGeomData[0][0],stm.GetBuf(),stm.GetUsedSize() );
}
*/
/*bool SIsoMesh::SaveIndexedMeshToFile( CMesh * pMesh, const char *sFilename, IChunkFile** pOutChunkFile )
{
  CContentCGF *pCGF = new CContentCGF(sFilename);

  CChunkFile *pChunkFile = new CChunkFile;
  if (pOutChunkFile)
    *pOutChunkFile = pChunkFile;

  // Add single node for merged mesh.
  CNodeCGF *pNode = new CNodeCGF;
  pNode->type = CNodeCGF::NODE_MESH;
  pNode->name = "Merged";
  pNode->localTM.SetIdentity();
  pNode->worldTM.SetIdentity();
  pNode->pos.Set(0,0,0);
  pNode->rot.SetIdentity();
  pNode->scl.Set(1,1,1);
  pNode->bIdentityMatrix = true;
  pNode->pMesh = new CMesh;
  pNode->pMesh->Copy( *pMesh );
  pNode->pParent = 0;
  pNode->pMaterial = NULL;
  pNode->nPhysicalizeFlags = 0;
//  SavePhysicalizeData( pNode );

  // Add node to CGF contents.
  pCGF->AddNode( pNode );

  CSaverCGF cgfSaver( sFilename,*pChunkFile );
  cgfSaver.SaveContent( pCGF );

  bool bResult = true;
  if (!pOutChunkFile)
  {
    bResult = pChunkFile->Write( sFilename );
    pChunkFile->Release();
  }

  delete pCGF;

  return bResult;
}*/

bool SIsoMesh::InitMaterials(int nSID)
{
  FUNCTION_PROFILER_3DENGINE;

  if(!GetTerrain())
    return true;

  bool bMatsFound = false;

  SSurfaceType * pTypes = GetTerrain()->GetSurfaceTypes(nSID);

  // init surface default mapping
  for(int nMatId=0; nMatId<MAX_SURFACE_TYPES_COUNT; nMatId++)
  {
    if(pTypes[nMatId].pLayerMat)
    {
      Get3DEngine()->InitMaterialDefautMappingAxis(pTypes[nMatId].pLayerMat);
      pTypes[nMatId].pLayerMat->m_ucDefautMappingAxis = 'Z';
      bMatsFound = true;
    }
  }

  return bMatsFound;
}

void SIsoMesh::BildIndexedMeshForPhysics( std::vector<uint16> & indices, 
                                          const std::vector<Vec3> & vertices, 
                                          const std::vector<SSurfTypeInfo> & surf_types)
{
  FUNCTION_PROFILER_3DENGINE;

  Dephysicalize();

  if(!vertices.size() || !indices.size())
    return;

  int flags = mesh_multicontact1 | mesh_AABB | mesh_no_vtx_merge | mesh_always_static;
  int arrSurfaceTypesId[MAX_SURFACE_TYPES_COUNT];
  memset(arrSurfaceTypesId,0,sizeof(arrSurfaceTypesId));

  // make indices
  PodArray<char> lstMatIndices; lstMatIndices.Clear();
  for (uint32 i=0; i<indices.size(); i+=3)
  {
    char nSurfType0 = surf_types[indices[i+0]].GetDomSType();
    char nSurfType1 = surf_types[indices[i+1]].GetDomSType();
    char nSurfType2 = surf_types[indices[i+2]].GetDomSType();

    char nSurfType = nSurfType0;
    if(nSurfType1 == nSurfType2 && nSurfType1 != nSurfType0)
      nSurfType = nSurfType1;

    if(nSurfType>=0 && nSurfType<MAX_SURFACE_TYPES_COUNT && nSurfType<Get3DEngine()->m_arrBaseTextureData.Count())
    {
      SImageInfo & rImgInfo = Get3DEngine()->m_arrBaseTextureData[nSurfType];
      arrSurfaceTypesId[nSurfType] = rImgInfo.nPhysSurfaceType;
    }

    lstMatIndices.Add(nSurfType);
  }

  if(!lstMatIndices.Count())
    return;

  {
    IGeomManager *pGeoman = GetPhysicalWorld()->GetGeomManager();
    IGeometry * pGeom = pGeoman->CreateMesh((Vec3*)&vertices[0], &indices[0], 
      lstMatIndices.GetElements(), NULL, indices.size()/3, flags, 0.05f, 2, 31);
    m_pPhysGeom = pGeoman->RegisterGeometry(pGeom);
    pGeom->Release();
  }

  if(!m_pPhysGeom)
    return Error("IsoMesh physicalization failed");

  GetPhysicalWorld()->GetGeomManager()->SetGeomMatMapping( m_pPhysGeom, arrSurfaceTypesId, MAX_SURFACE_TYPES_COUNT );

  //  EnablePhysEntity(true);
}

void SIsoMesh::BildIndexedMeshForRendering(const std::vector<TriangleIndex> & triangles, 
                       const std::vector<Vec3> & vertices, 
                       const std::vector<SSurfTypeInfo> & surf_types, 
                       const std::vector<ColorB> & colors, 
                       float fPosScale, const AABB & WSBBox, 
                       bool bLog, bool bCopiedFromParent,
                       void * node, void * pIdx, char * szCurrentMessageIn)
{
  FUNCTION_PROFILER_3DENGINE;

  SAFE_DELETE(m_pGeneratedMesh);

  m_WSBBox = WSBBox;

  CIndexedMesh * pMesh = new CIndexedMesh();

  pMesh->ReallocStream(CMesh::FACES,      triangles.size());
  pMesh->ReallocStream(CMesh::TEXCOORDS,  vertices.size());
  pMesh->ReallocStream(CMesh::NORMALS,    vertices.size());
  pMesh->ReallocStream(CMesh::POSITIONS,  vertices.size());
  pMesh->ReallocStream(CMesh::COLORS_0,   vertices.size());
  pMesh->ReallocStream(CMesh::COLORS_1,   vertices.size());
  pMesh->ReallocStream(CMesh::VERT_MATS,  vertices.size());

  memset(pMesh->m_pNorms, 0, sizeof(pMesh->m_pNorms[0])*vertices.size());
  memset(pMesh->m_pVertMats, 0, sizeof(pMesh->m_pVertMats[0])*vertices.size());

  // fill mesh data
  for (uint32 t=0; t<triangles.size(); t++)
  {
    const TriangleIndex & triangle = triangles[t];

    pMesh->m_pFaces[t].nSubset = 0;
    pMesh->m_pFaces[t].dwFlags = 0;

    Vec3 vFaceNormal;
    {
      const Vec3 & v0 = vertices[triangle[0]];
      const Vec3 & v1 = vertices[triangle[1]];
      const Vec3 & v2 = vertices[triangle[2]];
      vFaceNormal = (v1-v0).Cross(v2-v0);
      vFaceNormal.Normalize();

      float fEps = 0.001f / fPosScale;
      if(v0.IsEquivalent(v1,fEps) || v1.IsEquivalent(v2,fEps) || v2.IsEquivalent(v0,fEps))
        pMesh->m_pFaces[t].dwFlags |= SFACE_FLAG_REMOVE;
    }

    for (int v=0; v<3; v++)
    {
      int id = triangle[v];

      pMesh->m_pFaces[t].v[v] = pMesh->m_pFaces[t].t[v] = id;

      {
/*        // accumulate pseudo normal (not always good for lighting)
        Vec3 v0 = vertices[triangle[(v+0)%3]];
        Vec3 v1 = vertices[triangle[(v+1)%3]];
        Vec3 v2 = vertices[triangle[(v+2)%3]];
        Vec3 a = (v1-v0).GetNormalized();
        Vec3 b = (v2-v0).GetNormalized();
        float dot = a.dot(b);
        if(dot<0.999f)
        {
          if(dot<-1.f)
            dot=-1.f;
          float angle = acos(dot);
          pMesh->m_pNorms[id] += vFaceNormal*angle;
        }*/

        if(!GetCVars()->e_VoxTerHeightmapEditing || vFaceNormal.z>0.01f)
          pMesh->m_pNorms[id] += vFaceNormal;
      }

      pMesh->m_pPositions[id] = vertices[id]*fPosScale;

      pMesh->m_pTexCoord[id].s = (pMesh->m_pPositions[id].x-WSBBox.min.x)/(WSBBox.max.x - WSBBox.min.x);
      pMesh->m_pTexCoord[id].t = (pMesh->m_pPositions[id].y-WSBBox.min.y)/(WSBBox.max.y - WSBBox.min.y);

      pMesh->m_pVertMats[id] = surf_types[id].GetDomSType();
      assert(pMesh->m_pVertMats[id]<MAX_SURFACE_TYPES_COUNT);

      pMesh->m_pColor0[id].r = 255;
      pMesh->m_pColor0[id].g = 255;
      pMesh->m_pColor0[id].b = 255;
      pMesh->m_pColor0[id].a = 255;
      pMesh->m_pColor1[id].r = 255;
      pMesh->m_pColor1[id].g = 255;
      pMesh->m_pColor1[id].b = 255;
      pMesh->m_pColor1[id].a = 255;

      pMesh->m_pColor0[id].g = 0; // nSurfIdGlobal | TAKE_BASE_COLOR_FROM_VERTEX
      
      if(!colors.empty())
      {
        pMesh->m_pColor0[id].r = colors[id].r; // R
        pMesh->m_pColor0[id].b = colors[id].g; // G
        pMesh->m_pColor1[id].b = colors[id].b; // B
      }
    }

    if(triangle.nCull)
      pMesh->m_pFaces[t].dwFlags |= SFACE_FLAG_REMOVE;
  }

  if(GetCVars()->e_VoxTerHeightmapEditing)
  {
    for(int i=0; i<pMesh->m_numVertices; i++)
      pMesh->m_pNorms[i] = GetTerrain()->GetTerrainSurfaceNormal(pMesh->m_pPositions[i],.5f);
  }
  else
  {
    for(int i=0; i<pMesh->m_numVertices; i++)
      pMesh->m_pNorms[i].NormalizeSafe();
  }

  // remove faces outside of sector
  for(int i=0; i<pMesh->m_numFaces; i++)
  {
    if(	pMesh->m_pFaces[i].dwFlags & SFACE_FLAG_REMOVE ||
        pMesh->m_pFaces[i].v[0] == pMesh->m_pFaces[i].v[1] || 
        pMesh->m_pFaces[i].v[1] == pMesh->m_pFaces[i].v[2] || 
        pMesh->m_pFaces[i].v[2] == pMesh->m_pFaces[i].v[0] )
    {
      if(i < pMesh->m_numFaces-1) // replace this with last
        memcpy(&pMesh->m_pFaces[i],&pMesh->m_pFaces[pMesh->m_numFaces-1],sizeof(pMesh->m_pFaces[i]));
      i--;
      pMesh->m_numFaces--;
    }
  }

  pMesh->SetSubSetCount(1);
  pMesh->InitStreamSize();
  pMesh->CalcBBox();
  pMesh->m_bbox = WSBBox;

  if(bLog)
    PrintMessagePlus(" %d tris after clean up ...", pMesh->m_numFaces);
  
  for(int i=0; i<pMesh->m_numVertices; i++)
  {
    // get global surf type id
    uint32 nSurfType = pMesh->m_pVertMats[i];
    assert(nSurfType<MAX_SURFACE_TYPES_COUNT);
    pMesh->m_pColor0[i].g = nSurfType;
  }

  pMesh->CheckValid();

  if(pMesh->m_numFaces)
  {
    PodArray<IRenderNode*> arrItems; arrItems.Clear();

    float fMeshSize = WSBBox.max.x-WSBBox.min.x;
    float fRayLen = min(fMeshSize*0.05f, GetCVars()->e_VoxTerPlanarProjection ? 0.05f : 1.f);

    if(GetCVars()->e_VoxTerTexBuildOnCPU)
    {
      AABB aabbWSEx = pMesh->GetBBox();
      aabbWSEx.min -= Vec3(fRayLen,fRayLen,fRayLen);
      aabbWSEx.max += Vec3(fRayLen,fRayLen,fRayLen);
      Get3DEngine()->GetObjectsByTypeGlobal(arrItems, eERType_Brush, &aabbWSEx, WSBBox.GetRadius()*2.f);
    }

    // generate and save

    strcpy(szCurrentMessageIn, "BuildTex");

    ColorB * pTexData[VOX_TEX_NUM_SRC];
    memset(pTexData,0,sizeof(pTexData));

    int nTexDimInt = GetTexDimInternal();

    bool bTexNeeded = false;
    
    bool b2DMixMask = GetCVars()->e_VoxTerHeightmapEditing && GetCVars()->e_VoxTerMixMask;

    if( b2DMixMask )
    {
      AUTO_LOCK(g_cMixMasksMap2D);

      int key = GetMeshKey2D();
      if(mixMasksMap2D.find(key) == mixMasksMap2D.end() || mixMasksMap2D[key].bTexDataUpdateNeeded)
      {
        mixMasksMap2D[key].nLastUseFrameId = GetMainFrameID();
        mixMasksMap2D[key].bTexDataUpdateNeeded = true;
        bTexNeeded = true;
      }
    }
    else
      bTexNeeded = true;

    pMesh->GenerateTextureCoordinates(nTexDimInt, fMeshSize, &m_nTexDimX, &m_nTexDimY, bLog, node, pIdx, &arrItems, 
      bTexNeeded ? pTexData : NULL, VOX_TEX_NUM_SRC, fRayLen);

    if(!bTexNeeded)
    {
      int key = GetMeshKey2D();
      if(mixMasksMap2D[key].pPixMats)
      {
        pMesh->m_pPixMats = new PodArray<unsigned short>;
        *pMesh->m_pPixMats = *mixMasksMap2D[key].pPixMats;
      }
    }

    {
      FRAME_PROFILER( "IsoMesh::Compile", GetSystem(), PROFILE_3DENGINE );

      pMesh->ShareVerticesTC(GetSystem(),WSBBox);

      pMesh->ReallocStream(CMesh::INDICES, pMesh->m_numFaces*3);

      int nIndexId=0;
      for(int i=0; i<pMesh->m_numFaces; i++)
      {
        assert(pMesh->m_pFaces[i].v[0] == pMesh->m_pFaces[i].t[0]);
        assert(pMesh->m_pFaces[i].v[1] == pMesh->m_pFaces[i].t[1]);
        assert(pMesh->m_pFaces[i].v[2] == pMesh->m_pFaces[i].t[2]);

        pMesh->m_pIndices[nIndexId++] = pMesh->m_pFaces[i].v[0];
        pMesh->m_pIndices[nIndexId++] = pMesh->m_pFaces[i].v[1];
        pMesh->m_pIndices[nIndexId++] = pMesh->m_pFaces[i].v[2];
      }

      pMesh->SetFacesCount(0);

      pMesh->InitStreamSize();
      pMesh->SetSubSetCount(1);
      SMeshSubset& subSet = pMesh->GetSubSet(0);
      subSet.nFirstIndexId=0;
      subSet.nNumIndices=pMesh->m_nIndexCount;
      subSet.nFirstVertId=0;
      subSet.nNumVerts=pMesh->m_numVertices;

      pMesh->CheckValid();
    }

    if(pMesh->GetVertexCount()>0xFFFF)
    {
      PrintMessagePlus(" ERROR: too many vertices (%d) in mesh", pMesh->GetVertexCount()); 
      delete pMesh;
      return;
    }

    if( GetCVars()->e_VoxTerTexBuildOnCPU && !b2DMixMask )
    {
      char szFileName[256]="";
      MakeTempTextureFileName(szFileName, sizeof(szFileName),0,"MTT",m_key, true);

#if !defined(XENON) && !defined(PS3)
      remove(szFileName);
#endif

      SerializeMesh( pMesh );
      int nSize = m_pMeshForSerialization->GetSize();

      if(FILE * f = fopen(szFileName,"wb"))
      {
        fwrite(&nSize, 1, sizeof(nSize), f);
        int nModId = IsoOctree_nModId;
        fwrite(&nModId, 1, sizeof(nModId), f);
        fwrite((byte*)m_pMeshForSerialization->GetData(), 1, nSize, f);

        if(!m_nTexDimX || !m_nTexDimY)
          Error(__FUNC__);

        fwrite(&m_nTexDimX, 1, sizeof(m_nTexDimX), f);
        fwrite(&m_nTexDimY, 1, sizeof(m_nTexDimY), f);

        if(pTexData[0])
        {
          CMemoryBlock mbTextures;
          mbTextures.Allocate(m_nTexDimX*m_nTexDimY*4*VOX_TEX_NUM_SRC);
          for(int t=0; t<VOX_TEX_NUM_SRC; t++)
            memcpy((byte*)mbTextures.GetData() + t*m_nTexDimX*m_nTexDimY*4, pTexData[t], m_nTexDimX*m_nTexDimY*4);
          assert(!m_pZipData);
          m_pZipData = CMemoryBlock::CompressToMemBlock(mbTextures.GetData(), mbTextures.GetSize(), GetSystem());
          int nZipSize = m_pZipData->GetSize();
          int writen_nZipSize = fwrite(&nZipSize, 1, sizeof(nZipSize), f);
          if(writen_nZipSize!=sizeof(nZipSize))
          {
            Error("%s, nZipSize data fwrite failed, %s", __FUNC__, szFileName);
            Error("writen_nZipSize=%d", writen_nZipSize);
          }

          int writen_pZipData = fwrite(m_pZipData->GetData(), 1, m_pZipData->GetSize(), f);
          if(writen_pZipData!=m_pZipData->GetSize())
          {
            Error("%s, nZipSize data fwrite failed, %s", __FUNC__, szFileName);
            Error("writen_pZipData=%d", writen_pZipData);
          }
        }
        else
        {
          int nZipSize = 0;
          fwrite(&nZipSize, 1, sizeof(nZipSize), f);
        }

        fclose(f);
      }

      SAFE_DELETE(m_pMeshForSerialization);

      for(int t=0; t<VOX_TEX_NUM_SRC; t++)
        SAFE_DELETE_ARRAY(pTexData[t]);
    }
  
    if(b2DMixMask)
    {
      if(pTexData[0])
      {
        AUTO_LOCK(g_cMixMasksMap2D);

        FRAME_PROFILER( "SIsoMesh::BildIndexedMeshForRendering_CompressToMemBlock", GetSystem(), PROFILE_3DENGINE );

        // if still needed
        int key = GetMeshKey2D();
        if(mixMasksMap2D.find(key) == mixMasksMap2D.end() || mixMasksMap2D[key].bTexDataUpdateNeeded)
        {
          for(int t=0; t<VOX_TEX_NUM_SRC; t++)
            SAFE_DELETE_ARRAY(mixMasksMap2D[key].arrNodeStreamedTexturesData[t]);

          for(int t=0; t<VOX_TEX_NUM_SRC; t++)
            mixMasksMap2D[key].arrNodeStreamedTexturesData[t] = pTexData[t];

          mixMasksMap2D[key].bTexUpdateNeeded = true;
          mixMasksMap2D[key].bTexDataUpdateNeeded = false;
          mixMasksMap2D[key].nLastUseFrameId = GetMainFrameID();
          if(pMesh->m_pPixMats)
          {
            mixMasksMap2D[key].pPixMats = new PodArray<unsigned short>;
            *mixMasksMap2D[key].pPixMats = *pMesh->m_pPixMats;
          }
        }
        else
        {
          mixMasksMap2D[key].nLastUseFrameId = GetMainFrameID();
          for(int t=0; t<VOX_TEX_NUM_SRC; t++)
            SAFE_DELETE_ARRAY(pTexData[t]);
        }
      }
    }
  }

  m_pGeneratedMesh = pMesh;
}

void SIsoMesh::CheckInitAffectingLights()
{
  if(m_nLightMaskFrameId != GetFrameID())
  {
    m_lstAffectingLights.Clear();
    PodArray<CDLight*> * pSceneLights = Get3DEngine()->GetDynamicLightSources();
    if(pSceneLights->Count() && (pSceneLights->GetAt(0)->m_Flags & DLF_SUN))
      m_lstAffectingLights.Add(pSceneLights->GetAt(0));

    m_nLightMaskFrameId = GetFrameID();
  }
}

PodArray<CDLight*> * SIsoMesh::GetAffectingLights()
{ 
  CheckInitAffectingLights();

  return &m_lstAffectingLights; 
}

void SIsoMesh::AddLightSource(CDLight * pSource)
{ 
  CheckInitAffectingLights();

  m_lstAffectingLights.Add(pSource); 
}

void SIsoMesh::Render(const SRendParams &rendParams)
{
  FUNCTION_PROFILER_3DENGINE;

  m_bRenderSuccess = true;

  m_nLastDrawFrameId = GetMainFrameID();
	m_fLastDrawTime = GetCurTimeSec();

  if(GetCVars()->e_VoxTer == 6)
    if(m_nRebuildStartedFrameId > (int)GetMainFrameID()-100)
      DrawBBox(m_WSBBox, Col_Blue);

  if (!m_pRenderMesh || !m_pRenderMesh->GetIndicesCount())
    return;

  if(GetCVars()->e_TerrainDrawThisSectorOnly)
  {
    AABB meshBox;
    m_pRenderMesh->GetBBox(meshBox.min,meshBox.max);
    const Vec3 & vOrigin = GetOrigin();
    meshBox.min += vOrigin;
    meshBox.max += vOrigin;
    meshBox.min.z = 0;
    meshBox.max.z = 10000;
    if(!meshBox.IsContainPoint(GetCamera().GetPosition()))
      return;
  }

  int nSectorDLMask = 0;

  if(!Get3DEngine()->GetRenderIntoShadowmap())
  {
    AABB meshBox;
    m_pRenderMesh->GetBBox(meshBox.min,meshBox.max);
    const Vec3 & vOrigin = GetOrigin();
    meshBox.min += vOrigin;
    meshBox.max += vOrigin;

    if(!GetCamera().IsAABBVisible_F(meshBox))
      return;

    if(m_bEditor || !GetCVars()->e_VoxTerInGameTextureStreaming)
    {
      static ICVar * p_wireframe = GetConsole()->GetCVar("r_wireframe");
      if(!p_wireframe || !p_wireframe->GetFVal())
//        if( !GetCVars()->e_HwOcclusionCullingObjects || 
  //        rendParams.fDistance < 2.f ||
    //      !GetObjManager()->IsBoxOccluded( meshBox, rendParams.fDistance, &m_pRNTmpData->userData.m_OcclState, true, eoot_OBJECT))
        {
          if(!BuildTextures(false))
          {
            m_bRenderSuccess = false;
            return;
          }
        }
    }

    PodArray<CDLight*> * pAffLight = GetAffectingLights();
    nSectorDLMask = Get3DEngine()->BuildLightMask( meshBox, pAffLight, NULL, true );

    // pass offset in the atlas
    for(int nTexSlot=0; nTexSlot<2; nTexSlot++)
    {
      if(m_arrDstDynTex[nTexSlot] && m_arrDstDynTex[nTexSlot]->IsValid())
      {
        uint32 nX, nY, nW, nH;
        m_arrDstDynTex[nTexSlot]->GetSubImageRect(nX, nY, nW, nH);

        uint32 nX1, nY1, nW1, nH1;
        m_arrDstDynTex[nTexSlot]->GetImageRect(nX1, nY1, nW1, nH1);

        m_arrTexInfo[nTexSlot][0] = float(nX)/float(nW1);
        m_arrTexInfo[nTexSlot][1] = float(nY)/float(nH1);
        m_arrTexInfo[nTexSlot][2] = float(nW)/float(nW1);
        m_arrTexInfo[nTexSlot][3] = float(nH)/float(nH1);

        float fAtlasW = (float)m_arrDstDynTex[nTexSlot]->GetTexture()->GetWidth();
        float fAtlasH = (float)m_arrDstDynTex[nTexSlot]->GetTexture()->GetHeight();

        m_arrTexInfo[nTexSlot][0] += 0.5f/fAtlasW;
        m_arrTexInfo[nTexSlot][1] += 0.5f/fAtlasH;
      }
      else
      {
        m_arrTexInfo[nTexSlot][0] = 0;
        m_arrTexInfo[nTexSlot][1] = 0;
        m_arrTexInfo[nTexSlot][2] = 1;
        m_arrTexInfo[nTexSlot][3] = 1;
      }
    }

    if(m_bEditor)
    {
      // min
      m_arrTexInfo[2][0] = 0;
      m_arrTexInfo[2][1] = 0;
      m_arrTexInfo[2][2] = 0;
      m_arrTexInfo[2][3] = 1.f / GetCVars()->e_VoxTerTexRangeScale;
      // range
      m_arrTexInfo[3][0] = 1.f;
      m_arrTexInfo[3][1] = 1.f;
      m_arrTexInfo[3][2] = 1.f;
      m_arrTexInfo[3][3] = 0;

      // min
      m_arrTexInfo[4][0] = 0;
      m_arrTexInfo[4][1] = 0;
      m_arrTexInfo[4][2] = 0;
      m_arrTexInfo[4][3] = 1.f / GetCVars()->e_VoxTerTexRangeScale;
      // range
      m_arrTexInfo[5][0] = 1.f;
      m_arrTexInfo[5][1] = 1.f;
      m_arrTexInfo[5][2] = 1.f;
      m_arrTexInfo[5][3] = 0;
    }
    else
    {
      // min
      m_arrTexInfo[2][0] = m_arrTexRangeInfo[0][0];
      m_arrTexInfo[2][1] = m_arrTexRangeInfo[0][1];
      m_arrTexInfo[2][2] = m_arrTexRangeInfo[0][2];
      m_arrTexInfo[2][3] = 1.f / GetCVars()->e_VoxTerTexRangeScale;
      // range
      m_arrTexInfo[3][0] = m_arrTexRangeInfo[1][0];
      m_arrTexInfo[3][1] = m_arrTexRangeInfo[1][1];
      m_arrTexInfo[3][2] = m_arrTexRangeInfo[1][2];
      m_arrTexInfo[3][3] = 0;

      // min
      m_arrTexInfo[4][0] = m_arrTexRangeInfo[2][0];
      m_arrTexInfo[4][1] = m_arrTexRangeInfo[2][1];
      m_arrTexInfo[4][2] = m_arrTexRangeInfo[2][2];
      m_arrTexInfo[4][3] = 1.f / GetCVars()->e_VoxTerTexRangeScale;
      // range
      m_arrTexInfo[5][0] = m_arrTexRangeInfo[3][0];
      m_arrTexInfo[5][1] = m_arrTexRangeInfo[3][1];
      m_arrTexInfo[5][2] = m_arrTexRangeInfo[3][2];
      m_arrTexInfo[5][3] = 0;
    }

    if(CREMesh *pRE = m_pRenderMesh->GetChunks()[0].pRE)
      pRE->m_CustomData = m_arrTexInfo[0];

    if((GetCVars()->e_VoxTer==2 || GetCVars()->e_VoxTer==4) && !Get3DEngine()->GetRenderIntoShadowmap())
    {
      float fBoxSize = m_WSBBox.GetSize().x;
      float fRefSize = (float)GetCVars()->e_VoxTerMeshSize;
      int nMip=0;
      while(fRefSize<fBoxSize)
      {
        nMip++;
        fRefSize*=2.f;
      }

      ColorB col = Col_White;
      col.r = (nMip&1) ? 255 : 0;
      col.g = (nMip&2) ? 255 : 0;
      col.b = (nMip&3) ? 255 : 0;

      if(GetCVars()->e_VoxTer != 4 || m_bMeshWasCopiedFromParent)
      {
        AABB WSBBoxRed = m_WSBBox;
        WSBBoxRed.min += Vec3(.1f,.1f,.1f);
        WSBBoxRed.max -= Vec3(.1f,.1f,.1f);
        DrawBBox(WSBBoxRed, col);
      }
    }
  }

  CRenderObject * pBasePassObj = GetIdentityCRenderObject();
  if (!pBasePassObj)
    return; // false;
  pBasePassObj->m_II.m_Matrix.SetIdentity();

  const Vec3 & vOrigin = GetOrigin();
	pBasePassObj->m_II.m_Matrix.SetTranslation(vOrigin);
	pBasePassObj->m_ObjFlags |= FOB_TRANS_TRANSLATE;

  pBasePassObj->m_ObjFlags |= rendParams.dwFObjFlags;
  pBasePassObj->m_fDistance = rendParams.fDistance;

  pBasePassObj->m_ObjFlags |= FOB_INSHADOW;

  pBasePassObj->m_DynLMMask[m_nRenderThreadListID] = nSectorDLMask;//rendParams.nDLightMask;

  pBasePassObj->m_II.m_AmbColor = Get3DEngine()->GetSkyColor();

  if(!Get3DEngine()->GetRenderIntoShadowmap())
  {
    SSectorTextureSet texSet = SSectorTextureSet(GetTerrain()->m_nWhiteTexId, GetTerrain()->m_nBlackTexId);

    for(int i=0; i<2; i++)
    {
      if(IDynTexture * pDynTex = m_arrDstDynTex[i])
      {
        if(pDynTex && GetCVars()->e_VoxTerTexDebug!=1 && pDynTex->IsValid())
        {
          ((unsigned short *)&texSet.nTex0)[i] = pDynTex->GetTextureID();

          m_nThisFrameTextureMemoryUsage += GetRenderer()->GetTextureFormatDataSize(
            pDynTex->GetWidth(), pDynTex->GetHeight(), 1, 1, pDynTex->GetTexture()->GetTextureDstFormat());
        }
        else
          ((unsigned short *)&texSet.nTex0)[i] = Get3DEngine()->GetBlackTexID();

        m_pRenderMesh->GetChunks()[0].pRE->m_CustomTexBind[i] = ((unsigned short *)&texSet.nTex0)[i];
      }
      else if(m_arrSrcTex[i])
      {
        ((unsigned short *)&texSet.nTex0)[i] = m_arrSrcTex[i];

        if(ITexture * pTex = GetRenderer()->EF_GetTextureByID(m_arrSrcTex[i]))
          m_nThisFrameTextureMemoryUsage += GetRenderer()->GetTextureFormatDataSize(
            pTex->GetWidth(), pTex->GetHeight(), 1, 1, pTex->GetTextureDstFormat());

        m_pRenderMesh->GetChunks()[0].pRE->m_CustomTexBind[i] = ((unsigned short *)&texSet.nTex0)[i];
      }
      else
        m_pRenderMesh->GetChunks()[0].pRE->m_CustomTexBind[i] = ((unsigned short *)&texSet.nTex0)[i] = Get3DEngine()->GetBlackTexID();
    }

    m_nThisFrameMeshMemoryUsage += (m_pRenderMesh->GetVerticesCount()*(sizeof(SVF_P3S_N4B_C4B_T2S)) + m_pRenderMesh->GetIndicesCount()*sizeof(uint16));

    m_pRenderMesh->SetMaterial(Get3DEngine()->m_pVoxTerrainMat);
  }

  ETEX_Format eIsoTexFormat = (ETEX_Format)GetCVars()->e_VoxTerTexFormat;

  if(GetCVars()->e_VoxTerMemoryUsageDebug)
  {
    int nTexMemUsage = GetRenderer()->GetTextureFormatDataSize(m_nTexDimX, m_nTexDimY, 1, 1, eIsoTexFormat)/1024;
    int nMemshMemUsage = (m_pRenderMesh->GetVerticesCount()*(sizeof(SVF_P3S_N4B_C4B_T2S)) + m_pRenderMesh->GetIndicesCount()*sizeof(uint16))/1024;
    if((nTexMemUsage+nMemshMemUsage) < GetCVars()->e_VoxTerMemoryUsageDebug)
      return;
    else
    {
      Matrix34 mt; mt.SetIdentity();
      ColorB col = ((nTexMemUsage+nMemshMemUsage) < GetCVars()->e_VoxTerMemoryUsageDebug*2) ? Col_Blue : Col_Red;
      DrawBBoxLabeled(m_WSBBox,mt,col,"T%dkb+M%dkb=%dkb", nTexMemUsage, nMemshMemUsage, nTexMemUsage+nMemshMemUsage);
    }
  }

  if(GetCVars()->e_VoxTerDiskUsageDebug)
  {
    if(m_nDiskUsage < GetCVars()->e_VoxTerDiskUsageDebug)
      return;
    else
    {
      Matrix34 mt; mt.SetIdentity();
      ColorB col = ((m_nDiskUsage) < GetCVars()->e_VoxTerDiskUsageDebug*2) ? Col_Blue : Col_Red;
      DrawBBoxLabeled(m_WSBBox,mt,col,"%dkb", m_nDiskUsage/1024);
    }
  }

  if(GetCVars()->e_VoxTerTexUsageDebug)
  {
    int nMemUsage = GetRenderer()->GetTextureFormatDataSize(m_nTexDimX, m_nTexDimY, 1, 1, eIsoTexFormat)/1024;
    if(nMemUsage < GetCVars()->e_VoxTerTexUsageDebug)
      return;
    else
    {
      Matrix34 mt; mt.SetIdentity();
      ColorB col = (nMemUsage < GetCVars()->e_VoxTerTexUsageDebug*2) ? Col_Blue : Col_Red;
      DrawBBoxLabeled(m_WSBBox,mt,col,"%dx%d=%dkb", m_nTexDimX, m_nTexDimY, nMemUsage);
    }
  }

  if(GetCVars()->e_VoxTerMeshUsageDebug && m_pRenderMesh)
  {
    int nMemUsage = (m_pRenderMesh->GetVerticesCount()*(sizeof(SVF_P3S_N4B_C4B_T2S)) + m_pRenderMesh->GetIndicesCount()*sizeof(uint16))/1024;
    if(nMemUsage < GetCVars()->e_VoxTerMeshUsageDebug)
      return;
    else
    {
      Matrix34 mt; mt.SetIdentity();
      ColorB col = (nMemUsage < GetCVars()->e_VoxTerMeshUsageDebug*2) ? Col_Blue : Col_Red;
      DrawBBoxLabeled(m_WSBBox,mt,col,"v=%d i=%d mem=%dkb", m_pRenderMesh->GetVerticesCount(), m_pRenderMesh->GetIndicesCount(), nMemUsage);
    }
  }

  m_pRenderMesh->AddRenderElements(pBasePassObj, EFSLIST_GENERAL, 0, NULL);

  /*
  // detail pass
  CRenderObject * pObj = NULL;

  if(rendParams.nTechniqueID<=0 && GetCVars()->e_TerrainDetailMaterials && 0)
  {
    for(int nSType=0; nSType<MAX_SURFACE_TYPES_COUNT && nSType<Get3DEngine()->m_arrBaseTextureData.Count(); nSType++) 
    {
      SSurfaceTypeInfo & typeInfo = m_arrSurfaceTypeInfos[nSType];
      SImageInfo & rImgInfo = Get3DEngine()->m_arrBaseTextureData[nSType];

      if(typeInfo.pSurfaceType && !rImgInfo.detailInfo.fAmount && typeInfo.pSurfaceType->pLayerMat)
      {

        { // check-update max view distance
          if(rImgInfo.baseInfo.fAmount)
            typeInfo.pSurfaceType->fCustomMaxDistance = 8.f;
          else
            typeInfo.pSurfaceType->fCustomMaxDistance = -1.f;

          uint8 szProj[] = "XYZ";
          for(int nProjectionId = 0; nProjectionId < 3; nProjectionId ++)
          {
            float * pParams = typeInfo.pSurfaceType->arrRECustomData[nProjectionId];
            CTerrainNode::SetupTexGenParams(typeInfo.pSurfaceType,pParams,szProj[nProjectionId],true);
          }
        }

        if(typeInfo.pSurfaceType->HasMaterial() && typeInfo.HasRM())
        {
          if(!pObj)
          {
            pObj = GetIdentityCRenderObject();
            if (!pObj)
              return;
            pObj->m_DynLMMask = 1;
            pObj->m_ObjFlags |= (FOB_DETAILPASS | FOB_NO_FOG);
            pObj->m_II.m_AmbColor = Get3DEngine()->GetSkyColor();
            pObj->m_II.m_Matrix.SetIdentity();
            //pObj->m_ObjFlags |= FOB_TRANS_MASK;
            pObj->m_ObjFlags |= FOB_INSHADOW;
            pObj->m_fDistance = 0;
            pObj->m_nRAEId = 0;
          }

          uint8 szProj[] = "XYZ";
          for(int p=0; p<3; p++)
          {
            if(IMaterial * pSubMat = typeInfo.pSurfaceType->GetMaterialOfProjection(szProj[p]))
              if(rendParams.fDistance<typeInfo.pSurfaceType->GetMaxMaterialDistanceOfProjection(szProj[p]))
                if(IRenderMesh * pRM = typeInfo.arrpRM[p])
                  if(pRM->GetVertexContainer() == m_pRenderMesh)
                    if(pRM->GetSysIndicesCount())
                      pRM->AddRenderElements(pObj, EFSLIST_TERRAINLAYER, 0, pSubMat);
          }
        }
      }
    }
  }*/
}

void SIsoMesh::GetTrianglesInSphere(const Sphere & spWS, PodArray<Vec3> & lstVerts, PodArray<Vec3> & lstNorms, PodArray<uint16> & lstIndices)
{
  if(!m_pRenderMesh)
    return;

  Sphere spOS = spWS;
  spOS.center -= GetOrigin();

  const int nVertCount = m_pRenderMesh->GetVerticesCount();
  int nStridePos=0, nStrideCol=0;
  int nIndCount = m_pRenderMesh->GetIndicesCount();
  const uint16 * pIndices = m_pRenderMesh->GetIndexPtr(FSL_READ);
  const byte * pPos = m_pRenderMesh->GetPosPtr(nStridePos, FSL_READ);
  const byte * pCol = m_pRenderMesh->GetColorPtr(nStrideCol, FSL_READ);

  if(!pIndices || !pPos)
    return;

  int nInitVertsCount = lstVerts.Count();

  // render tris
  PodArray<CRenderChunk>& Chunks = m_pRenderMesh->GetChunks();
  const uint32 VertexCount = m_pRenderMesh->GetVerticesCount();
  for(int nChunkId=0; nChunkId<Chunks.Count(); nChunkId++)
  {
    CRenderChunk * pChunk = &Chunks[nChunkId];
    if(pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
      continue;

    int nLastIndexId = pChunk->nFirstIndexId + pChunk->nNumIndices;
    for(int i=pChunk->nFirstIndexId; i<nLastIndexId; i+=3)
    {
      int32 I0	=	pIndices[i+0];
      int32 I1	=	pIndices[i+1];
      int32 I2	=	pIndices[i+2];

      assert(I0<(int)VertexCount);
      assert(I1<(int)VertexCount);
      assert(I2<(int)VertexCount);

      Triangle t;
      t.v0	=	(*((Vec3*)&pPos[nStridePos*I0]));
      t.v1	=	(*((Vec3*)&pPos[nStridePos*I1]));
      t.v2	=	(*((Vec3*)&pPos[nStridePos*I2]));

      float fArea = t.GetArea();
      if( fArea<0.001f || t.v0.IsEquivalent(t.v1,0.001f) || t.v1.IsEquivalent(t.v2,0.001f) || t.v2.IsEquivalent(t.v0,0.001f) )
        continue;

      if(!Overlap::Sphere_Triangle(spOS, t))
        continue;

      lstIndices.Add(I0+nInitVertsCount);
      lstIndices.Add(I1+nInitVertsCount);
      lstIndices.Add(I2+nInitVertsCount);
    }
  }

  for(int i=0; i<nVertCount; i++)
  {
    lstVerts.Add(*((Vec3*)&pPos[nStridePos*i]));
    lstVerts.Last() += GetOrigin();

    ColorB col0; memcpy(&col0, &pCol[nStrideCol*i], sizeof(col0));
    ColorB norm; memcpy(&norm, &pCol[nStrideCol*i]-sizeof(col0), sizeof(norm));






    ColorB col;
    col.g = col0.r;
    col.r = col0.b;
    col.b = norm.a;
    col.a = 255;

    Vec3 vNorm((norm[0]-128.f), (norm[1]-128.f), (norm[2]-128.f));

    lstNorms.Add(vNorm.GetNormalized());
  }
}

void SIsoMesh::GetTrianglesInAABB(const AABB & aabb, PodArray<Vec3> & lstVerts, PodArray<Vec3> & lstNorms, PodArray<uint16> & lstIndices)
{
  if(!m_pRenderMesh)
    return;

  const int nVertCount = m_pRenderMesh->GetVerticesCount();
  int nIndCount = 0, nPosStride=0;
  nIndCount = m_pRenderMesh->GetIndicesCount();
  const uint16 * pIndices = m_pRenderMesh->GetIndexPtr(FSL_READ);
  const byte * pPos = m_pRenderMesh->GetPosPtr(nPosStride, FSL_READ);
  const byte * pCol = m_pRenderMesh->GetColorPtr(nPosStride, FSL_READ);

  if(!pIndices || !pPos)
    return;

  int nInitVertsCount = lstVerts.Count();

  // render tris
  PodArray<CRenderChunk>& Chunks = m_pRenderMesh->GetChunks();
  const uint32 VertexCount = m_pRenderMesh->GetVerticesCount();
  for(int nChunkId=0; nChunkId<Chunks.Count(); nChunkId++)
  {
    CRenderChunk * pChunk = &Chunks[nChunkId];
    if(pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
      continue;

    int nLastIndexId = pChunk->nFirstIndexId + pChunk->nNumIndices;
    for(int i=pChunk->nFirstIndexId; i<nLastIndexId; i+=3)
    {
      int32 I0	=	pIndices[i+0];
      int32 I1	=	pIndices[i+1];
      int32 I2	=	pIndices[i+2];

      assert(I0<(int)VertexCount);
      assert(I1<(int)VertexCount);
      assert(I2<(int)VertexCount);

      Vec3 & v0	=	(*((Vec3*)&pPos[nPosStride*I0]));
      Vec3 & v1	=	(*((Vec3*)&pPos[nPosStride*I1]));
      Vec3 & v2	=	(*((Vec3*)&pPos[nPosStride*I2]));

      if(!Overlap::AABB_Triangle(aabb,v0,v1,v2))
        continue;

//      Vec3 vNorm = (v1-v0).Cross(v2-v0);
//      if(vNorm.z<0)
  //      continue;

      lstIndices.Add(I0+nInitVertsCount);
      lstIndices.Add(I1+nInitVertsCount);
      lstIndices.Add(I2+nInitVertsCount);
    }
  }

  for(int i=0; i<nVertCount; i++)
  {
    lstVerts.Add(*((Vec3*)&pPos[nPosStride*i]));

    ColorB col0; memcpy(&col0, &pCol[nPosStride*i], sizeof(col0));
    ColorB norm; memcpy(&norm, &pCol[nPosStride*i]-sizeof(col0), sizeof(norm));






    ColorB col;
    col.g = col0.r;
    col.r = col0.b;
    col.b = norm.a;
    col.a = 255;

    Vec3 vNorm((norm[0]-128.f), (norm[1]-128.f), (norm[2]-128.f));

    lstNorms.Add(vNorm.GetNormalized());
  }
}

SIsoMesh::SIsoMesh(ISO_KEY key) 
{
  m_nDiskUsage=0;
  m_pZipData=0;
  m_eStreamingStatus = ecss_NotLoaded;
  m_bUseStreaming = false;
  m_nMeshFileOffset=0;
  m_nMeshFileDataSize=0;
  m_key = key;
  m_bMeshWasCopiedFromParent = false;
  m_pMeshForSerialization = 0; m_pRenderMesh = 0; ; 
  ZeroStruct(m_arrTexturesZipSize);
  ZeroStruct(m_arrDstDynTex); 
  ZeroStruct(m_arrSrcTex);
  ZeroStruct(m_arrSrcTexData);
  m_pPhysEnt = 0; m_pPhysGeom = 0; m_WSBBox.Reset(); 
  m_bInProgress=0; m_nRebuildStartedFrameId=m_nRebuildRequestedFrameId=0; m_nTexDimX=0; m_nTexDimY=0; //m_nTexDimScale=0;
  m_nLastDrawFrameId = GetMainFrameID();
	m_fLastDrawTime = GetCurTimeSec();
  m_pGeneratedMesh = NULL;
  m_nBuildTexturesFrameId = 0;

  {
    AUTO_LOCK(g_cIsoMeshCounter);
    GetCounter()->Add(this);
  }
}

void SIsoMesh::GetMemoryUsage(ICrySizer * pSizer)
{
  SIZER_COMPONENT_NAME(pSizer, "IsoMesh");
  pSizer->AddObject(this, sizeof(*this));
  if(m_pMeshForSerialization)
  {
    SIZER_COMPONENT_NAME(pSizer, "MeshForSerialization");
    pSizer->AddObject(m_pMeshForSerialization, m_pMeshForSerialization->GetSize());
  }
  
  if(m_pGeneratedMesh)
  {
    SIZER_COMPONENT_NAME(pSizer, "GeneratedMesh");
    pSizer->AddObject(m_pGeneratedMesh, m_pGeneratedMesh->GetMemoryUsage());
  }

  if(m_pZipData)
  {
    SIZER_COMPONENT_NAME(pSizer, "ZipData");
    pSizer->AddObject(m_pZipData, m_pZipData->GetSize());
  }
}


int Cmp_BuildItem(const void* v1, const void* v2)
{
  SBuildTerrainTextureParams::SBuildItem * p[2] = { (SBuildTerrainTextureParams::SBuildItem*)v1, (SBuildTerrainTextureParams::SBuildItem*)v2 };

  int arrTypeSort[2] = { 0, 0 };
  int arrUserSort[2] = { 0, 0 };
  float arrSize[2] = { 1000000.f, 1000000.f };
//  Vec3 arrPos[2] = { Vec3(0,0,0), Vec3(0,0,0) };

  for(int i=0; i<2; i++)
  {
    if(!p[i]->pRenderNode && !p[i]->nRemeshTexId[0])
      arrTypeSort[i] = 0; // terrain
    else if(p[i]->pRenderNode && p[i]->pRenderNode->GetRenderNodeType() == eERType_Road)
      arrTypeSort[i] = 1; // road
    else if(!p[i]->pRenderNode && p[i]->nRemeshTexId[0])
      arrTypeSort[i] = 1; // integrated meshes texture
    else if(p[i]->pRenderNode && p[i]->pRenderNode->GetRenderNodeType() == eERType_Decal)
      arrTypeSort[i] = 1; // decal
    else
      assert(!"Undefined");
  
    if(p[i]->pRenderNode)
      arrUserSort[i] = p[i]->pRenderNode->GetSortPriority();
    else
    {
      assert(p[i]->nTerrainLayerId>=0 && p[i]->nTerrainLayerId<32);
      SImageInfo & rImgInfo = ((C3DEngine*)gEnv->p3DEngine)->m_arrBaseTextureData[p[i]->nTerrainLayerId];
      arrUserSort[i] = rImgInfo.baseInfo.nSortOrder;
    }

    arrSize[i] = p[i]->pRenderNode ? p[i]->pRenderNode->GetBBox().GetRadius() : 1000000;

//    arrPos[i] = p[i]->pRenderNode ? p[i]->pRenderNode->GetPos() : Vec3((float)p[i]->nTerrainLayerId,0,0);
  }
/*
  if(((C3DEngine*)gEnv->p3DEngine)->GetCVars()->e_VoxTerHeightmapEditing)
  {
    if(arrTypeSort[0] < arrTypeSort[1])
      return -1;
    else if(arrTypeSort[0] > arrTypeSort[1])
      return 1;
  }
*/
  if(arrUserSort[0] < arrUserSort[1])
    return -1;
  else if(arrUserSort[0] > arrUserSort[1])
    return 1;

//  if(!((C3DEngine*)gEnv->p3DEngine)->GetCVars()->e_VoxTerHeightmapEditing)
  {
    if(arrTypeSort[0] < arrTypeSort[1])
      return -1;
    else if(arrTypeSort[0] > arrTypeSort[1])
      return 1;
  }

  if(arrSize[0] < arrSize[1])
    return 1;
  else if(arrSize[0] > arrSize[1])
    return -1;

  Vec3 vPos0 = p[0]->pRenderNode ? p[0]->pRenderNode->GetPos() : Vec3(0,0,0);
  Vec3 vPos1 = p[1]->pRenderNode ? p[1]->pRenderNode->GetPos() : Vec3(0,0,0);

  if(vPos0.x < vPos1.x)
    return -1;
  else if(vPos0.x > vPos1.x)
    return 1;

  return 0;
}


void SIsoMesh::ReleaseTextures(bool bReleaseOnlyFinalTexture)
{
  for(int i=0; i<VOX_TEX_NUM_DST; i++)
  {
    CVoxTerrain::AddTextureForRelease(m_arrDstDynTex[i]);
    m_arrDstDynTex[i] = NULL;
  }

  bool b2DMixMask = GetCVars()->e_VoxTerHeightmapEditing && GetCVars()->e_VoxTerMixMask;

  if(!bReleaseOnlyFinalTexture && !(b2DMixMask && m_bEditor))
  {
    for(int i=0; i<VOX_TEX_NUM_SRC; i++)
    {
      GetRenderer()->RemoveTexture(m_arrSrcTex[i]);
      m_arrSrcTex[i]=0;
    }
  }
}

byte *SaveCompessedMipmapLevel_data=0;
int   SaveCompessedMipmapLevel_allocated_size=0;
int   SaveCompessedMipmapLevel_size=0;

static void SaveCompessedMipmapLevel(const void *data, size_t size, void * userData)
{
  SaveCompessedMipmapLevel_size = size+sizeof(SVoxTexHeader);

  if(SaveCompessedMipmapLevel_size > SaveCompessedMipmapLevel_allocated_size)
  {
    SAFE_DELETE_ARRAY(SaveCompessedMipmapLevel_data);
    SaveCompessedMipmapLevel_allocated_size = max((int)1024*1024*4 + (int)sizeof(SVoxTexHeader),SaveCompessedMipmapLevel_size);
    SaveCompessedMipmapLevel_data = new byte[SaveCompessedMipmapLevel_allocated_size];
  }

  memcpy(SaveCompessedMipmapLevel_data+sizeof(SVoxTexHeader), data, size);
}

bool SIsoMesh::BuildTextures(bool bSerializeTextures)
{
#if !defined(PS3) && !defined(XENON)

  if(GetCVars()->e_VoxTerTexBuildOnCPU == 2)
    return true;

  FUNCTION_PROFILER_3DENGINE;

  if(bSerializeTextures)
    if( !m_pRenderMesh && (m_pGeneratedMesh || m_pMeshForSerialization) )
      MakeRenderMesh();

  if( !m_pRenderMesh || !m_pRenderMesh->GetIndicesCount() || !m_nTexDimX )
    return true;

  if(!(GetCVars()->e_VoxTerTexRebuildEveryFrame || CVoxTerrain::m_nSelLayer >= 0) && !bSerializeTextures)
    if(m_arrDstDynTex[0] && m_arrDstDynTex[0]->IsValid())
      if(m_arrDstDynTex[1] && m_arrDstDynTex[1]->IsValid())
        return true;

  bool b2DMixMask = GetCVars()->e_VoxTerHeightmapEditing && GetCVars()->e_VoxTerMixMask;

  if( GetCVars()->e_VoxTerTexBuildOnCPU && !m_arrSrcTex[0] )
  {
    static int nTexCounter = 0;
    static int nLastFrameId = 0;
    if(!bSerializeTextures && nLastFrameId == GetMainFrameID() && !CVoxTerrain::IsEditing())
    {
      nTexCounter++;
      if(nTexCounter>GetCVars()->e_VoxTerTexBuildsPerFrame)
        return false;
    }
    else
    {
      nLastFrameId = GetMainFrameID();
      nTexCounter=1;
    }

    if(!b2DMixMask && m_pZipData)
    {
      FRAME_PROFILER( "SIsoMesh::BuildTextures_ReadMTT", GetSystem(), PROFILE_3DENGINE );

      if(CMemoryBlock * pUnpacked = CMemoryBlock::DecompressFromMemBlock(m_pZipData, GetSystem()))
      {
        for(int t=0; t<VOX_TEX_NUM_SRC; t++)
        {
          byte * pImg = (byte *)pUnpacked->GetData() + t*m_nTexDimX*m_nTexDimY*4;
          assert((t+1)*m_nTexDimX*m_nTexDimY*4 <= pUnpacked->GetSize());
          m_arrSrcTex[t] = GetRenderer()->DownLoadToVideoMemory( pImg, 
            m_nTexDimX, m_nTexDimY, eTF_A8R8G8B8, eTF_A8R8G8B8, 
            0, false, FILTER_BILINEAR, 0, 0, FT_TEX_WAS_NOT_PRE_TILED, eLittleEndian);
        }

        delete pUnpacked;
      }
    }    
    else if(b2DMixMask)
    {
      AUTO_LOCK(g_cMixMasksMap2D);

      int key = GetMeshKey2D();
      if(mixMasksMap2D.find(key) != mixMasksMap2D.end())
      {
        if(!mixMasksMap2D[key].arrNodeStreamedTextures[0] || mixMasksMap2D[key].bTexUpdateNeeded)
        {
          for(int t=0; t<VOX_TEX_NUM_SRC; t++)
            GetRenderer()->RemoveTexture(mixMasksMap2D[key].arrNodeStreamedTextures[t]);          

          mixMasksMap2D[key].bTexUpdateNeeded = false;

          int nTexDimInt = GetTexDimInternal();

          for(int t=0; t<VOX_TEX_NUM_SRC; t++)
          {
            byte * pImg = (byte *)mixMasksMap2D[key].arrNodeStreamedTexturesData[t];

            mixMasksMap2D[key].arrNodeStreamedTextures[t] = GetRenderer()->DownLoadToVideoMemory( pImg, 
              nTexDimInt, nTexDimInt, eTF_A8R8G8B8, eTF_A8R8G8B8, 
              0, false, FILTER_BILINEAR, 0, 0, FT_TEX_WAS_NOT_PRE_TILED, eLittleEndian);

            SAFE_DELETE_ARRAY(mixMasksMap2D[key].arrNodeStreamedTexturesData[t]);
          }
        }

        for(int t=0; t<VOX_TEX_NUM_SRC; t++)
          m_arrSrcTex[t] = mixMasksMap2D[key].arrNodeStreamedTextures[t];

        mixMasksMap2D[key].nLastUseFrameId = GetMainFrameID();
      }
      else
      {
        assert(0);
        return false;
      }
    }
  }

  SBuildTerrainTextureParams params;
  params.nDstTexDimX = m_nTexDimX*GetCVars()->e_VoxTerTexResFinal/GetTexDimInternal();
  params.nDstTexDimY = m_nTexDimY*GetCVars()->e_VoxTerTexResFinal/GetTexDimInternal();

  if(b2DMixMask)
  {
    params.nSrcTexDimX = GetTexDimInternal();
    params.nSrcTexDimY = GetTexDimInternal();
  }
  else
  {
    params.nSrcTexDimX = m_nTexDimX;
    params.nSrcTexDimY = m_nTexDimY;
  }

/*
#ifdef XENON
  params.nDstTexDimX = max((uint16)256,params.nDstTexDimX);
  params.nDstTexDimY = max((uint16)256,params.nDstTexDimY);
#endif
*/

  params.pDstTex_Diff = m_arrDstDynTex[0];
  params.pDstTex_Bump = m_arrDstDynTex[1];
  params.fMeshNodeSize = m_WSBBox.GetSize().x;
  params.vMeshWSPos = GetOrigin();
  params.nBumpDownScale = GetCVars()->e_VoxTerTexResBumpDownscale;
  params.bHeightMapMode = GetCVars()->e_VoxTerHeightmapEditing!=0;
  params.bOverlayBlend = GetCVars()->e_VoxTerOverlayBlend!=0;
  params.bMixMask = GetCVars()->e_VoxTerMixMask!=0;
  params.vBoxMin = m_WSBBox.min;
  params.fLodDistance = params.fMeshNodeSize / 2 / GetCVars()->e_VoxTerViewDistRatio;
  params.pRM = m_pRenderMesh;
  params.fTexRangeScale = GetCVars()->e_VoxTerTexRangeScale;

  for(int nSType=0; nSType<MAX_SURFACE_TYPES_COUNT; nSType++)
  {
    int nInUse = m_arrSurfaceTypeInfos[nSType].nInUse;

    if(nInUse && Get3DEngine()->m_arrBaseTextureData.Count() > nSType && GetCVars()->e_VoxTerTexDebug!=2)
    {
      SImageInfo & rImgInfo = Get3DEngine()->m_arrBaseTextureData[nSType];

      if(!rImgInfo.arrTextureId[0])
      {
        ITexture *pTex0=0, *pTex1=0, *pTex2=0;
        
        pTex0 = GetRenderer()->EF_LoadTexture(rImgInfo.szBaseTexName,FT_DONT_STREAM,eTT_2D);
        
        if(pTex0->IsTextureLoaded())
        {
          char szNameBump[256]="";
          strcpy(szNameBump, rImgInfo.szBaseTexName);
          szNameBump[strlen(szNameBump)-4]=0;        
          if(strstr(szNameBump,"_color"))
            szNameBump[strlen(szNameBump)-6]=0;
          if(strstr(szNameBump,"_diff"))
            szNameBump[strlen(szNameBump)-5]=0;
          if(strstr(szNameBump,"_dif"))
            szNameBump[strlen(szNameBump)-4]=0;

          strcat(szNameBump, "_ddn.dds");
          pTex1 = GetPak()->IsFileExist(szNameBump) ? GetRenderer()->EF_LoadTexture(szNameBump,FT_DONT_STREAM,eTT_2D) : NULL;
          szNameBump[strlen(szNameBump)-8]=0;

          if(!pTex1 || !pTex1->IsTextureLoaded())
          {
            strcat(szNameBump, "_nm.dds");
            pTex1 = GetPak()->IsFileExist(szNameBump) ? GetRenderer()->EF_LoadTexture(szNameBump,FT_DONT_STREAM,eTT_2D) : NULL;
            szNameBump[strlen(szNameBump)-7]=0;
          }

          if(!pTex1 || !pTex1->IsTextureLoaded())
          {
            strcat(szNameBump, "-nm.dds");
            pTex1 = GetPak()->IsFileExist(szNameBump) ? GetRenderer()->EF_LoadTexture(szNameBump,FT_DONT_STREAM,eTT_2D) : NULL;
            szNameBump[strlen(szNameBump)-7]=0;
          }

          strcat(szNameBump, "_spec.dds");
          pTex2 = GetPak()->IsFileExist(szNameBump) ? GetRenderer()->EF_LoadTexture(szNameBump,FT_DONT_STREAM,eTT_2D) : NULL;
        }

        rImgInfo.arrTextureId[0] = (pTex0 && pTex0->IsTextureLoaded()) ? pTex0->GetTextureID() : GetTerrain()->m_nWhiteTexId;
        rImgInfo.arrTextureId[1] = (pTex1 && pTex1->IsTextureLoaded()) ? pTex1->GetTextureID() : 0;
        rImgInfo.arrTextureId[2] = (pTex2 && pTex2->IsTextureLoaded()) ? pTex2->GetTextureID() : 0;
      }

      params.arrSrcTextures[nSType].nTexId_Diff = rImgInfo.arrTextureId[0];
      params.arrSrcTextures[nSType].nTexId_Bump = rImgInfo.arrTextureId[1];
      params.arrSrcTextures[nSType].nTexId_Spec = rImgInfo.arrTextureId[2];

      float fBrPlus = 1.f;
      if(CVoxTerrain::m_nSelLayer >= 0 && CVoxTerrain::m_nSelLayer == nSType)
        fBrPlus = (((int)(GetCurTimeSec()*8))&1) ? 1.5f : 0.66f;

      params.arrSrcTextures[nSType].filterColor = fBrPlus;
      if(!GetCVars()->e_VoxTerHeightmapEditing)
        params.arrSrcTextures[nSType].filterColor = rImgInfo.layerFilterColor * rImgInfo.fBr;

      params.arrSrcTextures[nSType].nSortOrder = rImgInfo.baseInfo.nSortOrder;
      params.arrSrcTextures[nSType].fSpecularAmount = rImgInfo.baseInfo.fSpecularAmount;
      params.arrSrcTextures[nSType].fDetailAmount = rImgInfo.detailInfo.fAmount;
      params.arrSrcTextures[nSType].fUseRemeshing = rImgInfo.fUseRemeshing || GetCVars()->e_VoxTerMixMask;
      
      if(ITexture*pTex0 = GetRenderer()->EF_GetTextureByID(rImgInfo.arrTextureId[0]))
        params.arrSrcTextures[nSType].fTiling = rImgInfo.baseInfo.fTiling / pTex0->GetWidth() * GetCVars()->e_VoxTerTexTiling;
    
      params.arrSrcTextures[nSType].pMatTerrainLayer = rImgInfo.szDetMatName[0] ? Get3DEngine()->GetMatMan()->LoadMaterial(rImgInfo.szDetMatName) : NULL;
    }
    else
    {
      params.arrSrcTextures[nSType].nTexId_Diff = 0;
      params.arrSrcTextures[nSType].nTexId_Bump = 0;
      params.arrSrcTextures[nSType].nTexId_Spec = 0;
    }
  }

  static PodArray<SBuildTerrainTextureParams::SBuildItem> arrItems; arrItems.Clear();
  
  {
    AABB meshBox;
    m_pRenderMesh->GetBBox(meshBox.min,meshBox.max);
    const Vec3 & vOrigin = GetOrigin();
    meshBox.min += vOrigin;
    meshBox.max += vOrigin;
    for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
      if(Get3DEngine()->m_pObjectsTree[nSID])
        Get3DEngine()->m_pObjectsTree[nSID]->GetObjectsForIntegrationIntoTexture(arrItems, &meshBox, meshBox.GetRadius()*.5f);
  }

  for(int i=0; i<32; i++)
  {
    if(params.arrSrcTextures[i].nTexId_Diff)
    {
      SBuildTerrainTextureParams::SBuildItem item;
      
      item.nTerrainLayerId = i;

      if(GetCVars()->e_VoxTerTexBuildOnCPU && params.arrSrcTextures[i].fUseRemeshing && m_arrSrcTex[0])
      {
        for(int t=0; t<VOX_TEX_NUM_SRC; t++)
          item.nRemeshTexId[t] = m_arrSrcTex[t];
      }

      arrItems.Add(item);
    }
  }

  qsort(arrItems.GetElements(), arrItems.Count(), sizeof(SBuildTerrainTextureParams::SBuildItem), Cmp_BuildItem);

  params.pDecalsAndRoadsAndTerrainLayers = &arrItems;

  params.nSyncTextures = Get3DEngine()->IsStatObjSyncLoad() || Get3DEngine()->IsShadersSyncLoad() || bSerializeTextures;

  GetRenderer()->RequestTerrainTexture(params);

  m_arrDstDynTex[0] = params.pDstTex_Diff;
  m_arrDstDynTex[1] = params.pDstTex_Bump;

  // save textures to disk
  if( bSerializeTextures )
  {
    static uint32 arrHG[3][256];

    for(int t=0; t<VOX_TEX_NUM_DST; t++)
    {
      // clear old data
      char szFileName[256]="";
      MakeTempTextureFileName(szFileName, sizeof(szFileName),t,"BIN",m_key, true);
      
#if !defined(XENON) && !defined(PS3)
      remove(szFileName);
#endif

      m_arrTexturesZipSize[t]=0;

      // get input image from the texture

      int nDataSizeIn = m_arrDstDynTex[t]->GetWidth()*m_arrDstDynTex[t]->GetHeight()*4;
      assert(nDataSizeIn);				
			PodArray<byte> arrImgDataIn;
			arrImgDataIn.PreAllocate(nDataSizeIn,nDataSizeIn);
      m_arrDstDynTex[t]->GetImageData32(arrImgDataIn.GetElements(), nDataSizeIn);

      //if(t==0)
      {
        // lossy range compression

        ZeroStruct(arrHG);

        for(int i=0; i<nDataSizeIn; i+=4)
        {
          arrHG[0][arrImgDataIn[i+0]]++;
          arrHG[1][arrImgDataIn[i+1]]++;
          arrHG[2][arrImgDataIn[i+2]]++;
        }

        uint32 arrMin[3]={0,0,0};

        int nClearColor = (int)(0.1f*255.f);

        int nIgnorePercentage = 2;

        // find good min
        for(int c=0; c<3; c++)
        {
          int nPixTooDark=0;

          for(int i=0; i<255; i++)
          {
            if(i == nClearColor)
              continue;

            nPixTooDark += arrHG[c][i];

            if(nPixTooDark >= nDataSizeIn / 4 * nIgnorePercentage / 100)
            {
              arrMin[c] = i;
              break;
            }
          }
        }

        uint32 arrMax[3]={0,0,0};

        // find good max
        for(int c=0; c<3; c++)
        {
          int nPixTooBright=0;

          for(uint i=255-1; i>=arrMin[c]; i--)
          {
            if(i == nClearColor)
              continue;

            nPixTooBright += arrHG[c][i];

            if(nPixTooBright >= nDataSizeIn / 4 * nIgnorePercentage / 100)
            {
              arrMax[c] = i;
              break;
            }
          }
        }

        ColorB colMin(arrMin[0],arrMin[1],arrMin[2],255);
        ColorB colMax(arrMax[0],arrMax[1],arrMax[2],255);


        Vec3 vRange(
          max(1.f,(float)colMax.r-(float)colMin.r),
          max(1.f,(float)colMax.g-(float)colMin.g),
          max(1.f,(float)colMax.b-(float)colMin.b));
        vRange *= 1.f/255.f;

        for(int i=0; i<nDataSizeIn; i+=4)
        {
          arrImgDataIn[i+0] = (byte)SATURATEB((float)(arrImgDataIn[i+0]-colMin[0])/vRange[0]);
          arrImgDataIn[i+1] = (byte)SATURATEB((float)(arrImgDataIn[i+1]-colMin[1])/vRange[1]);
          arrImgDataIn[i+2] = (byte)SATURATEB((float)(arrImgDataIn[i+2]-colMin[2])/vRange[2]);
          arrImgDataIn[i+3] = 255;
        }

        m_arrTexRangeInfo[2*t+0].x = 1.f/255.f*(float)colMin.r;
        m_arrTexRangeInfo[2*t+0].y = 1.f/255.f*(float)colMin.g;
        m_arrTexRangeInfo[2*t+0].z = 1.f/255.f*(float)colMin.b;
        m_arrTexRangeInfo[2*t+1] = vRange;
      }

      ETEX_Format eIsoTexFormat = (ETEX_Format)GetCVars()->e_VoxTerTexFormat;

      int nTexDownScale = 1<<GetCVars()->e_VoxTerTexResExportDownscale;

      int nResW = m_arrDstDynTex[t]->GetWidth() / nTexDownScale;
      int nResH = m_arrDstDynTex[t]->GetHeight() / nTexDownScale;

      PodArray<byte> arrImgDataOut;
      arrImgDataOut.PreAllocate(nResW*nResH*4,nResW*nResH*4);

      if(nTexDownScale<=1)
      {
        arrImgDataOut = arrImgDataIn;
      }
      else
      {
        for(int x=0; x<nResW; x++)
        {
          for(int y=0; y<nResH; y++)
          {
            ColorF colRes(0,0,0,0);

            for(int _x=0; _x<nTexDownScale; _x++)
            {
              for(int _y=0; _y<nTexDownScale; _y++)
              {
                for(int c=0; c<4; c++)
                {
                  colRes[c] += arrImgDataIn[((nTexDownScale*x+_x)*nResH*nTexDownScale + (nTexDownScale*y+_y))*4+c];
                }
              }
            }

            colRes *= 1.f/(float)(nTexDownScale*nTexDownScale);

            for(int c=0; c<4; c++)
            {
              arrImgDataOut[((x)*nResH + (y))*4+c] = (byte)SATURATEB(colRes[c]);
            }
          }
        }
      }

      if(eIsoTexFormat >= eTF_DXT1 && eIsoTexFormat <= eTF_DXT5)
      {
        int nClamp = 255;

compress_again:

        try
        {
          GetRenderer()->DXTCompress( arrImgDataOut.GetElements(),
            nResW, nResH,
            eIsoTexFormat, true, 0, 4, Vec3(0,0,0), SaveCompessedMipmapLevel);
        }
        catch( ... ) 
        {
          nClamp -= 5;

          Error("SIsoMesh::BuildTextures: DXTCompress failed: %s, will clamp to %d", szFileName, nClamp);

          if(nClamp>=0)
          {
            int nDataSizeOut = nResW*nResH*4;
            for(int i=0; i<nDataSizeOut; i++)
              arrImgDataOut[i] = min(arrImgDataOut[i],nClamp);
            goto compress_again;
          }
          else
            CryFatalError("SIsoMesh::BuildTextures: DXTCompress failed");
        }        
      }
      else if(eIsoTexFormat == eTF_R5G6B5)
      {
        SAFE_DELETE_ARRAY(SaveCompessedMipmapLevel_data);

        int nPixNum = nResW*nResH;
        int nNewDataSize = nPixNum*2;
        SaveCompessedMipmapLevel_size = nNewDataSize+sizeof(SVoxTexHeader);
        SaveCompessedMipmapLevel_data = new byte[nNewDataSize];

        #define GFC_RGB32_TO_RGB16(color)       (uint16)((((color)&0xF8)>>3)|(((color)&0xFC00)>>5)|(((color)&0xF80000)>>8))

        for(int i=0; i<nPixNum; i++)
        {
          uint32 col32 = *(uint32*)(arrImgDataOut.GetElements()+i*4);
          *(uint16*)&(SaveCompessedMipmapLevel_data[sizeof(SVoxTexHeader)+i*2]) = GFC_RGB32_TO_RGB16(col32);
        }
      }
      else if(eIsoTexFormat == eTF_A4R4G4B4)
      {
        SAFE_DELETE_ARRAY(SaveCompessedMipmapLevel_data);

        int nPixNum = nResW*nResH;
        int nNewDataSize = nPixNum*2;
        SaveCompessedMipmapLevel_size = nNewDataSize+sizeof(SVoxTexHeader);
        SaveCompessedMipmapLevel_data = new byte[nNewDataSize];

        for(int i=0; i<nPixNum; i++)
        {
          ColorB col32_InB = *(ColorB*)(arrImgDataOut.GetElements()+i*4);
          
          ColorF col32_In;
          col32_In.r = 1.f/255.f*(float)col32_InB.r;
          col32_In.g = 1.f/255.f*(float)col32_InB.g;
          col32_In.b = 1.f/255.f*(float)col32_InB.b;
          col32_In.a = 1.f;

          {
            float br = ((float)col32_In.r+(float)col32_In.g+(float)col32_In.b)/3.f;
            for(int c=0; c<3; c++)
              col32_In[c] = br + ((float)col32_In[c] - br)/2.f;
          }

          ColorF nonSRGB = col32_In;
          col32_In.rgb2srgb();
          col32_In = (nonSRGB+col32_In)*.5f;

          ColorB col32_Out;

          {
            col32_Out.b = 255;
            col32_Out.a = (uint8)SATURATEB(col32_In.b * 255.f);
            col32_Out.r = (uint8)SATURATEB(col32_In.g * 255.f);
            col32_Out.g = (uint8)SATURATEB(col32_In.r * 255.f);
          }

          unsigned short col16 = col32_Out.pack_argb4444();
          *(uint16*)&(SaveCompessedMipmapLevel_data[sizeof(SVoxTexHeader)+i*2]) = col16;
        }
      }

      assert(SaveCompessedMipmapLevel_size);

      SVoxTexHeader header;
      header.w = nResW;
      header.h = nResH;
      header.eFormat = eIsoTexFormat;
      header.SetDataSize(SaveCompessedMipmapLevel_size-sizeof(header));

//        if(t==0)
//        PrintMessage("SIsoMesh::BuildTextures: %d x %d F %d S %d", header.w, header.h, header.eFormat, header.GetDataSize());

      memcpy(SaveCompessedMipmapLevel_data, &header, sizeof(header));

      CMemoryBlock * pZipData = CMemoryBlock::CompressToMemBlock(SaveCompessedMipmapLevel_data, SaveCompessedMipmapLevel_size, GetSystem());

      m_arrTexturesZipSize[t] = pZipData ? pZipData->GetSize() : 0;

      if(m_arrTexturesZipSize[t])
      if(FILE * f = fopen(szFileName,"wb"))
      {
        fwrite(pZipData->GetData(), 1, pZipData->GetSize(), f);
        fclose(f);
      }

      delete pZipData;
    }
  }

  if(bSerializeTextures)
  {
    for(int t=0; t<VOX_TEX_NUM_DST; t++)
      SAFE_RELEASE(m_arrDstDynTex[t]);

    ReleaseTextures();
  }

#endif

  return true;
}

void SIsoMesh::StreamAsyncOnComplete(IReadStream* pStream, unsigned nError)
{
  FUNCTION_PROFILER_3DENGINE;

  pStream->SetUserData((DWORD_PTR)NULL);

  m_pReadStream = 0;

  if(pStream->IsError())
    return Error("SIsoMesh::StreamAsyncOnComplete: Error loading IsoMesh: %s Error# %d", "", nError);

  assert(!m_pMeshForSerialization && !m_pGeneratedMesh && m_nMeshFileDataSize);

  byte * f = (byte *)pStream->GetBuffer();
  int nDataSize = pStream->GetSize();
  EEndian eEndian = eLittleEndian;

  // load mesh
  {
    int nMeshDataSize=0;
    if(!CTerrain::LoadDataFromFile(&nMeshDataSize, 1, f, nDataSize, eEndian))
      return Error("SIsoMesh::StreamAsyncOnComplete: Error loading IsoMesh: %s Error# %d", "", nError);
    CMemoryBlock * pCompMeshData = new CMemoryBlock(f, nMeshDataSize);
    if(!CTerrain::LoadDataFromFile((byte*)pCompMeshData->GetData(), nMeshDataSize, f, nDataSize, eEndian))
      return Error("SIsoMesh::StreamAsyncOnComplete: Error loading IsoMesh: %s Error# %d", "", nError);
    CTerrain::LoadDataFromFile_FixAllignemt(f,nDataSize);
    assert(strncmp((char *)pCompMeshData->GetData(),"CryTek",6)!=0);
    CMemoryBlock * pUnpacked = CMemoryBlock::DecompressFromMemBlock(pCompMeshData, GetSystem());
    delete pCompMeshData;
    if(!pUnpacked)
      return Error("SIsoMesh::StreamAsyncOnComplete: CMemoryBlock::DecompressFromMemBlock failed");
    assert(strncmp((char *)pUnpacked->GetData(),"CryTek",6)==0);

    CIndexedMesh * pGeneratedMesh = new CIndexedMesh();
    LoadMeshFromMemBlock( pUnpacked, pGeneratedMesh->GetMesh() );
    delete pUnpacked;
    pStream->SetUserData((DWORD_PTR)pGeneratedMesh);
  }

  // load texture

  if(GetCVars()->e_VoxTerInGameTextureStreaming && nDataSize>0)
  {
    uint32 arrSize[VOX_TEX_NUM_DST]={0,0};

    if(!CTerrain::LoadDataFromFile(arrSize, VOX_TEX_NUM_DST, f, nDataSize, eEndian))
      return Error("SIsoMesh::StreamAsyncOnComplete: Error loading IsoMesh: %s Error# %d", "", nError);

    for(int t=0; t<VOX_TEX_NUM_DST; t++)
    {
      if(arrSize[t])
      {
        CMemoryBlock * pCompTexData = new CMemoryBlock(f, arrSize[t]);

        if(!CTerrain::LoadDataFromFile((byte*)pCompTexData->GetData(), arrSize[t], f, nDataSize, eEndian))
          return Error("SIsoMesh::StreamAsyncOnComplete: Error loading IsoMesh: %s Error# %d", "", nError);
        CTerrain::LoadDataFromFile_FixAllignemt(f,nDataSize);
        assert(strncmp((char *)pCompTexData->GetData(),"CryTek",6)!=0);
        CMemoryBlock * pUnpacked = CMemoryBlock::DecompressFromMemBlock(pCompTexData, GetSystem());
        delete pCompTexData;
        if(!pUnpacked)
          return Error("SIsoMesh::StreamAsyncOnComplete: CMemoryBlock::DecompressFromMemBlock failed");
        assert(1||strncmp((char *)pUnpacked->GetData(),"CryTek",6)==0);

        SVoxTexHeader & header = *(SVoxTexHeader *)pUnpacked->GetData();
        SwapEndian(header, eEndian);

        if( pUnpacked->GetSize()>sizeof(SVoxTexHeader) && header.w && header.h )
        {
          if(GetCVars()->e_VoxTerInGameTextureStreaming==2)
            PrintMessage("SIsoMesh::StreamAsyncOnComplete: %d x %d, %d, %d", header.w, header.h, header.eFormat, header.GetDataSize());




















          m_arrSrcTexData[t] = pUnpacked;
        }
        else
        {
          delete pUnpacked;
          m_arrSrcTexData[t] = 0;
        }
      }
    }
  }
}

void SIsoMesh::StreamOnComplete(IReadStream* pStream, unsigned nError)
{
  FUNCTION_PROFILER_3DENGINE;

  SAFE_DELETE(m_pGeneratedMesh);

  m_pGeneratedMesh = (CIndexedMesh *)pStream->GetParams().dwUserData;

  if(pStream->IsError())
    return Error("SIsoMesh::StreamOnComplete: Error loading CGF: %s Error# %d", "", nError);

  MakeRenderMesh();

  SAFE_DELETE(m_pGeneratedMesh);

  ETEX_Format eIsoTexFormat = (ETEX_Format)GetCVars()->e_VoxTerTexFormat;

  for(int t=0; t<VOX_TEX_NUM_DST; t++)
  {
    if(m_arrSrcTexData[t])
    {
      SVoxTexHeader * pHeader = (SVoxTexHeader *)m_arrSrcTexData[t]->GetData();

      static char szTexName[256];
      static int nTexGenID=0;
      if(t==0)
        sprintf(szTexName, "$VoxTerrainDiff_%d", nTexGenID++);
      else
        sprintf(szTexName, "$VoxTerrainBump_%d", nTexGenID++);

      m_arrSrcTex[t] = GetRenderer()->DownLoadToVideoMemory((byte*)m_arrSrcTexData[t]->GetData() + sizeof(SVoxTexHeader), 
        pHeader->w, pHeader->h, eIsoTexFormat, eIsoTexFormat, 
        0, false, FILTER_BILINEAR, m_arrSrcTex[t], szTexName, FT_TEX_WAS_NOT_PRE_TILED, GetPlatformEndian(), NULL, true);

      SAFE_DELETE(m_arrSrcTexData[t]);
    }
  }

  m_eStreamingStatus = ecss_Ready;
}

void SIsoMesh::StartStreaming( bool bFinishNow, float fTime )
{
  assert(m_eStreamingStatus == ecss_NotLoaded);

  if(m_eStreamingStatus != ecss_NotLoaded)
    return;
/*
  if(m_nTexDimX && m_nTexDimY)
  {
    for(int t=0; t<VOX_TEX_NUM_DST; t++)
      m_arrSrcTex[t] = GetRenderer()->DownLoadToVideoMemory(NULL, m_nTexDimX/(t+1), m_nTexDimY/(t+1), 
      eIsoTexFormat, eIsoTexFormat, 0, false, FILTER_BILINEAR, 0, 0, FT_TEX_WAS_NOT_PRE_TILED, eLittleEndian);
  }
*/
  StreamReadParams params;
  params.nSize = m_nMeshFileDataSize;
  params.nOffset = m_nMeshFileOffset;
  params.nLoadTime = (uint32)(fTime*1000.f);

  CVoxTerrain::LogStreamingActivity(params.nOffset);

  m_pReadStream = GetSystem()->GetStreamEngine()->StartRead(eStreamTaskTypeGeometry, 
    Get3DEngine()->GetLevelFilePath(COMPILED_VOX_MAP_FILE_NAME), this, &params);

  m_eStreamingStatus = ecss_InProgress;

  if(bFinishNow)
    m_pReadStream->Wait();
}

bool SIsoMesh::IsMeshCompiled()
{
  return m_pPhysGeom || m_pMeshForSerialization || m_bMeshWasCopiedFromParent || m_pGeneratedMesh || 
    (m_pRenderMesh && m_pRenderMesh->GetIndicesCount()) || m_eStreamingStatus == ecss_Ready;
}

char * SIsoMesh::MakeTempTextureFileName(char * szFileName, int nMaxFileNameLen, int nTexSlotId, const char * szExt, ISO_KEY key, bool bMakeFolder)
{
  char szLevelName[256];
  strcpy(szLevelName, Get3DEngine()->GetLevelFolder());
  szLevelName[strlen(szLevelName)-1]=0;
  while(szLevelName[0] && strstr(szLevelName,"/"))
    memmove(szLevelName,szLevelName+1,strlen(szLevelName));

  if(bMakeFolder)
  {
    sprintf_s(szFileName, nMaxFileNameLen, "USER");
    CryCreateDirectory(szFileName,0);
    sprintf_s(szFileName, nMaxFileNameLen, "USER/VoxTempCache");
    CryCreateDirectory(szFileName,0);
    sprintf_s(szFileName, nMaxFileNameLen, "USER/VoxTempCache/Cache_%s", szLevelName);
    CryCreateDirectory(szFileName,0);
    sprintf_s(szFileName, nMaxFileNameLen, "USER/VoxTempCache/Cache_%s/%s", szLevelName, szExt);
    CryCreateDirectory(szFileName,0);
  }

  ETEX_Format eIsoTexFormat = (ETEX_Format)GetCVars()->e_VoxTerTexFormat;

  if(key)
    sprintf_s(szFileName, nMaxFileNameLen, "USER/VoxTempCache/Cache_%s/%s/%X_%X_%d_F%d_x%d.%s", 
    szLevelName, szExt, (uint32)(key&0xFFFFFFFF), (uint32)(key>>(sizeof(ISO_KEY)*4)), nTexSlotId, (int)eIsoTexFormat, (int)GetCVars()->e_VoxTerTexRangeScale, szExt);
  else
    sprintf_s(szFileName, nMaxFileNameLen, "USER/VoxTempCache/Cache_%s/%s", szLevelName, szExt);

  return szFileName;
}

const Vec3 SIsoMesh::GetOrigin() const 
{ 
  Vec3 o;
  const float fSnap = 64.f;
  o.x = floor((m_WSBBox.min.x+m_WSBBox.max.x)*.5f/fSnap)*fSnap;
  o.y = floor((m_WSBBox.min.y+m_WSBBox.max.y)*.5f/fSnap)*fSnap;
  o.z = floor((m_WSBBox.min.z+m_WSBBox.max.z)*.5f/fSnap)*fSnap;
  return o;
}

int SIsoMesh::GetTexDimInternal()
{
  float fFullResPixSize = 2.f / GetCVars()->e_VoxTerTexResInternal;

  int nResultTexRes = GetCVars()->e_VoxTerTexResInternal;

  float fMeshSize = m_WSBBox.max.x-m_WSBBox.min.x;
  
  while ((fMeshSize / nResultTexRes) > fFullResPixSize)
  {
    nResultTexRes *= 2;
  }

  return min(nResultTexRes, GetCVars()->e_VoxTerTexResFinal);
}

void SIsoMesh::RequestTextureUpdate()
{
  AUTO_LOCK(g_cMixMasksMap2D);
  int key = GetMeshKey2D();
  std::map<int,SMixMask2D>::iterator iter = mixMasksMap2D.find(key);
  if(iter != mixMasksMap2D.end())
  {
    iter->second.bTexDataUpdateNeeded = true;
    iter->second.nLastUseFrameId = GetMainFrameID();
  }
}

int SIsoMesh::GetMeshKey2D()
{
  Vec3 vCenter = m_WSBBox.GetCenter();
  return int(vCenter.x)*int(CVoxTerrain::m_fMapSize) + int(vCenter.y);
}

int SIsoMesh::UnloadOldSrcTextures()
{
  FUNCTION_PROFILER_3DENGINE;

  AUTO_LOCK(g_cMixMasksMap2D);

  int nMemUsage = 0;

  std::map<int,SMixMask2D>::iterator next;
  for(std::map<int,SMixMask2D>::iterator iter = mixMasksMap2D.begin(); iter != mixMasksMap2D.end(); iter = next)
  {
    next = iter;
    next++;

    SMixMask2D & rItem = iter->second;

    if(rItem.nLastUseFrameId < (int)GetMainFrameID()-500)
    {
      //Get3DEngine()->PrintMessage("SIsoMesh::UnloadOldSrcTextures: Unloading, key = %d", iter->first);

      for(int t=0; t<VOX_TEX_NUM_SRC; t++)
        SAFE_DELETE_ARRAY(rItem.arrNodeStreamedTexturesData[t]);

      for(int t=0; t<VOX_TEX_NUM_SRC; t++)
        GetRenderer()->RemoveTexture(rItem.arrNodeStreamedTextures[t]);          

      mixMasksMap2D.erase(iter);
    }
    else
    {
      for(int t=0; t<VOX_TEX_NUM_SRC; t++)
        if(rItem.arrNodeStreamedTextures[t])
          if(ITexture * pTex = GetRenderer()->EF_GetTextureByID(rItem.arrNodeStreamedTextures[t]))
            nMemUsage += pTex->GetDataSize();
    }
  }

  return nMemUsage;
}

void SIsoMesh::MarkMixMasksMap2DAsUsed()
{
  AUTO_LOCK(g_cMixMasksMap2D);
  int key = GetMeshKey2D();
  if(mixMasksMap2D.find(key) != mixMasksMap2D.end())
    mixMasksMap2D[key].nLastUseFrameId = GetMainFrameID();
}
