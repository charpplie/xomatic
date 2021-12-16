////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   3dengine.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Implementation of I3DEngine interface methods
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <ICryAnimation.h>
#include <IFaceGen.h>
#include <IGameFramework.h>

#include "3dEngine.h"
#include "terrain.h"
#include "VisAreas.h"
#include "ObjMan.h"
#include "terrain_water.h"

#include "DecalManager.h"
#include "Vegetation.h"
#include "IndexedMesh.h"
#include "WaterVolumes.h"

#include "MatMan.h"

#include "Brush.h"
#include "CullBuffer.h"
#include "CGF/CGFLoader.h"
#include "CGF/ReadOnlyChunkFile.h"

#include "CloudRenderNode.h"
#include "CloudsManager.h"
#include "SkyLightManager.h"
#include "FogVolumeRenderNode.h"
#include "RoadRenderNode.h"
#include "DecalRenderNode.h"
#include "TimeOfDay.h"
#include "VoxMan.h"
#include "LightEntity.h"
#include "FogVolumeRenderNode.h"
#include "ObjectsTree.h"
#include "WaterVolumeRenderNode.h"
#include "WaterWaveRenderNode.h"
#include "DistanceCloudRenderNode.h"
#include "VolumeObjectRenderNode.h"
#include "AutoCubeMapRenderNode.h"
#include "WaterWaveRenderNode.h"
#include "RopeRenderNode.h"
#include "RenderMeshMerger.h"
#include "PhysCallbacks.h"

#include "CGF/ChunkFile.h"
#include "CGF/CGFSaver.h"
#include "CGF/CGFLoader.h"

#include <crc32.h>

static int NUM_MAX_BONES_PER_GROUP=100;

template<typename TYPE>
struct DefaultValueVector
{
  static void MakeDefault(TYPE& value)
  {
  }
};

template<typename TYPE>
struct DefaultValueAtomic
{
  static void MakeDefault(TYPE& value)
  {
    value = 0;
  }
};

template<typename TYPE,int size,typename DefaultValue = DefaultValueVector<TYPE> >
struct SFixedArray
{
  SFixedArray()
  {
    for(int n=0;n<size;n++)
    {
      DefaultValue::MakeDefault(values[n]);
    }
  }
  TYPE& operator[](size_t n)
  {
    return values[n];
  }
  const TYPE& operator[](size_t n) const
  {
    return values[n];
  }
  TYPE values[size];
};


struct SMergedMtlInfo
{
  SMergedMtlInfo()
  {
  }
  _smart_ptr<IMaterial> mtlOrg;
  //
  std::vector<AABB> src_rects;
  SFixedArray< std::vector<int>,EFTT_MAX> src_x;
  SFixedArray< std::vector<int>,EFTT_MAX> src_y;
  SFixedArray< std::vector<int>,EFTT_MAX> dst_x;
  SFixedArray< std::vector<int>,EFTT_MAX> dst_y;
  SFixedArray< std::vector<int>,EFTT_MAX> w;
  SFixedArray< std::vector<int>,EFTT_MAX> h;
  SFixedArray< std::vector<float>,EFTT_MAX> uv_min_x;
  SFixedArray< std::vector<float>,EFTT_MAX> uv_min_y;
  SFixedArray< std::vector<float>,EFTT_MAX> uv_max_x;
  SFixedArray< std::vector<float>,EFTT_MAX> uv_max_y;
  SFixedArray< std::vector<int>,EFTT_MAX> border;
  //
  int src_w[EFTT_MAX];
  int src_h[EFTT_MAX];
  //
  string org_filename[EFTT_MAX];
  string dds_filename[EFTT_MAX];
};

struct SMergedSubset
{
  //
  SMergedSubset()
  {
  }
  IMaterial* mtlMerged;
  int tex_w[EFTT_MAX];// width of dest texture
  int tex_h[EFTT_MAX];
  std::vector<SMergedMtlInfo> originalMaterials;
};

struct SMergeMaterialsResult : IMergeMaterialsResult
{
  virtual void Release()
  {
    delete this;
  }
  std::vector<SMergedSubset> mergedMtls;
};


//////////////////////////////////////////////////////////////////////////
static void DeleteOldChunks( CContentCGF *pCGF,CChunkFile &chunkFile, bool bDelete_CompiledPhysicalProxies=true, bool bDeleteMorphs=true )
{


  for (int i = 0; i < pCGF->GetNodeCount(); i++)
  {
    CNodeCGF *pNode = pCGF->GetNode(i);
    if (pNode->type == CNodeCGF::NODE_MESH && pNode->nChunkId)
    {
      chunkFile.DeleteChunkId(pNode->nChunkId); // Delete chunk of node.
      if (pNode->nObjectChunkId)
        chunkFile.DeleteChunkId(pNode->nObjectChunkId); // Delete chunk of mesh.
    }
  }


  // Delete all mesh chunks.
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_Mesh);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }


  // Delete all mesh subsets.
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_MeshSubsets);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }

  // Delete all data streams.
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_DataStream);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }

  // Delete all precompiled physics data streams.
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_MeshPhysicsData);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }

  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_ExportFlags);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }

  //---------------------------------------------------
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_MeshMorphTarget);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }

  // Delete all bone-namelists.
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_BoneNameList);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }

  // Delete all bone-positions.
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_BoneInitialPos);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }

  // Delete all bones.
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_BoneAnim);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }

  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_BoneMesh);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }


  //-----------------------------------------------------------------------------------------------
  //delete all compiled chunks
  //-----------------------------------------------------------------------------------------------
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledBones);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledPhysicalBones);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }

  if( bDelete_CompiledPhysicalProxies )
  {
    while (true) {
      CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledPhysicalProxies);
      if (cd)
        chunkFile.DeleteChunkId( cd->hdr.ChunkID );
      else
        break;
    }
  }

  if( bDeleteMorphs )
  {
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledMorphTargets);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }
  }

  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledIntFaces);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledIntSkinVertices);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_CompiledExt2IntMap);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }
  while (true) {
    CChunkFile::ChunkDesc *cd = chunkFile.FindChunkByType(ChunkType_BonesBoxes);
    if (cd)
      chunkFile.DeleteChunkId( cd->hdr.ChunkID );
    else
      break;
  }

}

static int ApplyMorphs(CSkinningInfo* pSkinningInfo,CContentCGF *pCGF,const std::vector<std::pair<string,float> >& morphs,bool bSaveMorphs,std::vector<uint8>* cmt)
{
  //---------------------------------------------------------------------
  //--- create compiled internal morph-targets and write them to chnk ---
  //---------------------------------------------------------------------
  //
  if( cmt )
  {
    cmt->clear();
  }
  //
  int numResMorphTargets = 0;
  uint32 numMorphtargets = pSkinningInfo->m_arrMorphTargets.size();
  for(uint32 i=0; i<numMorphtargets; i++) 
  {
    bool bFound = false;
    const char* morphName = pSkinningInfo->m_arrMorphTargets[i]->m_strName.c_str();
    for(size_t m=0;m<morphs.size();m++)
    {
      if( strcmp(morphName,morphs[m].first.c_str())==0 )
      {
        bFound = true;
        //
        float weight = morphs[m].second;
        //
        // why 0 index is root mesh i don't know - i believe in this
        //
        CNodeCGF* pCGFNode = pCGF->GetNode(0);
        CMesh* pMesh = pCGFNode->pMesh;
        MorphTargets* pTargets =pSkinningInfo->m_arrMorphTargets[i];
        for(int v=0;v<pTargets->m_arrExtMorph.size();v++)
        {
          int posIndex = pTargets->m_arrExtMorph[v].nVertexId;
          pMesh->m_pPositions[posIndex]+=weight*pTargets->m_arrExtMorph[v].ptVertex;
        }
        break;
      }
    }
    if( !bFound && bSaveMorphs && cmt )
    {
      SMeshMorphTargetHeader header;
      //store the mesh ID
      header.MeshID = pSkinningInfo->m_arrMorphTargets[i]->MeshID;
      //store the name of morph-target
      header.NameLength = pSkinningInfo->m_arrMorphTargets[i]->m_strName.size()+1;
      //store the vertices&indices of morph-target
      header.numIntVertices = pSkinningInfo->m_arrMorphTargets[i]->m_arrIntMorph.size();
      //store the vertices&indices of morph-target
      header.numExtVertices = pSkinningInfo->m_arrMorphTargets[i]->m_arrExtMorph.size();
      //
      uint8* ptr=(uint8*)(&header);
      for(uint32 h=0; h<sizeof(SMeshMorphTargetHeader); h++) 
      {
        cmt->push_back(ptr[h]);
      }

      for(uint32 h=0; h<header.NameLength; h++) 
      {
        cmt->push_back(pSkinningInfo->m_arrMorphTargets[i]->m_strName[h]);
      }

      ptr=(uint8*)(&pSkinningInfo->m_arrMorphTargets[i]->m_arrIntMorph[0]);
      for(uint32 h=0; h<(sizeof(SMeshMorphTargetVertex)*header.numIntVertices); h++) 
      {
        cmt->push_back(ptr[h]);
      }

      ptr=(uint8*)(&pSkinningInfo->m_arrMorphTargets[i]->m_arrExtMorph[0]);
      for(uint32 h=0; h<(sizeof(SMeshMorphTargetVertex)*header.numExtVertices); h++) 
      {
        cmt->push_back(ptr[h]);
      }
      //
      numResMorphTargets++;
    }
  }
  return numResMorphTargets;
}

static void createAndSavePhysicalProxyMesh(CSkinningInfo* pSkinningInfo,CSaverCGF& cgfSaver)
{
  //---------------------------------------------------------------------
  //--- create compiled physical proxi mesh and write it to chunk     ---
  //---------------------------------------------------------------------
  std::vector<uint8> pmt;
  uint32 numPhysicalProxies = pSkinningInfo->m_arrPhyBoneMeshes.size();
  for(uint32 i=0; i<numPhysicalProxies; i++) 
  {
    SMeshPhysicalProxyHeader header;
    header.ChunkID			= pSkinningInfo->m_arrPhyBoneMeshes[i].ChunkID;
    header.numPoints		= pSkinningInfo->m_arrPhyBoneMeshes[i].m_arrPoints.size();
    header.numIndices		= pSkinningInfo->m_arrPhyBoneMeshes[i].m_arrIndices.size();
    header.numMaterials	= pSkinningInfo->m_arrPhyBoneMeshes[i].m_arrMaterials.size();
    uint8* ptr=(uint8*)(&header);
    for(uint32 h=0; h<sizeof(SMeshPhysicalProxyHeader); h++) 
      pmt.push_back(ptr[h]);

    ptr=(uint8*)(&pSkinningInfo->m_arrPhyBoneMeshes[i].m_arrPoints[0]);
    for(uint32 h=0; h<(sizeof(Vec3)*header.numPoints); h++) 
      pmt.push_back(ptr[h]);

    ptr=(uint8*)(&pSkinningInfo->m_arrPhyBoneMeshes[i].m_arrIndices[0]);
    for(uint32 h=0; h<(sizeof(uint16)*header.numIndices); h++) 
      pmt.push_back(ptr[h]);

    ptr=(uint8*)(&pSkinningInfo->m_arrPhyBoneMeshes[i].m_arrMaterials[0]);
    for(uint32 h=0; h<(sizeof(uint8)*header.numMaterials); h++) 
      pmt.push_back(ptr[h]);
  }
  cgfSaver.SaveCompiledPhysicalProxis( &pmt[0], pmt.size(), numPhysicalProxies );
}


bool C3DEngine::SaveCHR(const char* srcfilename,const char* filename,IMergeCHRMorphsPipe* pipe,bool bSaveMorps)
{
  CChunkFile chunkFile;
  chunkFile.Read( srcfilename );
  //
  CLoaderCGF cgfLoader;
  CContentCGF *pCGF = cgfLoader.LoadCGF( srcfilename,chunkFile,NULL );
  if (!pCGF)
  {
    Error( "Failed to load geometry file %s - %s",srcfilename,cgfLoader.GetLastError() );
    return false;
  }
  //
  DeleteOldChunks( pCGF,chunkFile );
//  reset
  //
  CSkinningInfo* pSkinningInfo = pCGF->GetSkinningInfo();
  //
//  CChunkFile chunkFile;
  CSaverCGF cgfSaver( filename,chunkFile );
  //---------------------------------------------------------------------
  //---  write compiled bones to chunk
  //---------------------------------------------------------------------
  CryBoneDescData* pCryBoneDescData = &pSkinningInfo->m_arrBonesDesc[0];
  uint32 numAnimBones = pSkinningInfo->m_arrBonesDesc.size();
  int q = cgfSaver.SaveCompiledBones( pCryBoneDescData, numAnimBones*sizeof(CryBoneDescData) );
  //---------------------------------------------------------------------
  //---  write compiled physical bones to chunk
  //---------------------------------------------------------------------
  BONE_ENTITY* pPhysicalBone = &pSkinningInfo->m_arrBoneEntities[0];
  uint32 numPhyBones = pSkinningInfo->m_arrBonesDesc.size();
  int n = cgfSaver.SaveCompiledPhysicalBones( pPhysicalBone, numPhyBones*sizeof(BONE_ENTITY) );
  //---------------------------------------------------------------------
  //--- create compiled physical proxi mesh and write it to chunk     ---
  //---------------------------------------------------------------------
  createAndSavePhysicalProxyMesh(pSkinningInfo,cgfSaver);
  //---------------------------------------------------------------------
  //--- create compiled internal morph-targets and write them to chnk ---
  //---------------------------------------------------------------------
  // mophs
  std::vector<std::pair<string,float> > morphs;
  for(uint32 i=0; i<pipe->getNumMorphs(); i++) 
  {
    const char* morphName = pipe->getMorphName(i);
    float weight = pipe->getMorphWeight(i);
    morphs.push_back(std::pair<string,float>(morphName,weight));
  }
  std::vector<uint8> cmt;
  int numResMorphTargets = ApplyMorphs(pSkinningInfo,pCGF,morphs,bSaveMorps,&cmt);
  if( numResMorphTargets )
  {
    cgfSaver.SaveCompiledMorphTargets( &cmt[0], cmt.size(), numResMorphTargets );
  }
  //---------------------------------------------------------------------
  //---  write internal skinning vertices to chunk
  //---------------------------------------------------------------------
  IntSkinVertex* pIntSkinVertex = &pSkinningInfo->m_arrIntVertices[0];
  uint32 numIntVertices = pSkinningInfo->m_arrIntVertices.size();
  int m = cgfSaver.SaveCompiledIntSkinVertices( pIntSkinVertex,numIntVertices*sizeof(IntSkinVertex) );
  //---------------------------------------------------------------------
  //---  write internal faces to chunk
  //---------------------------------------------------------------------
  uint32 numIntFaces = pSkinningInfo->m_arrIntFaces.size();
  cgfSaver.SaveCompiledIntFaces( &pSkinningInfo->m_arrIntFaces[0], numIntFaces*sizeof(TFace) );
  //---------------------------------------------------------------------
  //---  write Ext2IntMap to chunk
  //---------------------------------------------------------------------
  uint32 numExt2Int = pSkinningInfo->m_arrExt2IntMap.size();
  cgfSaver.SaveCompiledExt2IntMap( &pSkinningInfo->m_arrExt2IntMap[0], numExt2Int*sizeof(uint16) );
  //
  //  CContentCGF* pCContentCGF=cgfLoader.GetCContentCGF();
  //if (pCContentCGF==0) 
  //{
  //    std::vector<uint16> arrIRemapping;
  //std::vector<uint16> arrVRemapping;
  //pCContentCGF = MakeCompiledCGF( pCGF, &arrVRemapping, &arrIRemapping  );
  //}
  //
  //
  cgfSaver.SetContent(pCGF);
  cgfSaver.SaveNodes();
  // Force remove of the read only flag.
  CrySetFileAttributes( filename,FILE_ATTRIBUTE_NORMAL );
  chunkFile.Write( filename );
  //
  delete pCGF;
  delete cgfLoader.GetCContentCGF();
  //
  return true;
 }

static void MakeCustomHeadTextureFileName(string& path, const char* faceFile)
{
	string newPath=PathUtil::ReplaceExtension(faceFile,".dds");
  if( gEnv->pCryPak->IsFileExist(newPath) )
  {
    path = newPath;
  }
}


//-------------------------------------------------------------------------
struct CCMergePipe : IMergePipe
{
	CCMergePipe()
	{
	}
	VIRTUAL int GetModelsCount() const
	{
		return chrFiles.size();
	}
	VIRTUAL const char* GetModelFilename(int index) const
	{
		return chrFiles[index];
	}
	VIRTUAL IMaterial* GetModelMaterial(int index) const
	{
		return chrMtls[index];
	}
	VIRTUAL float GetModelLodMaxAngle(int chrindex) const
	{
		return 0.0f;
	}
	VIRTUAL IMergePipe::ELodFlags GetModelLodFlags(int chrindex) const
	{
		return IMergePipe::elfNothing;
	}
  VIRTUAL const char* GetModelLogPrefix(int chrindex) const
  {
    return "HEAD";
  }
	//
	// material merge
	VIRTUAL void SetMergedMaterial(IMaterial* pMergedMaterialIn)
	{
		pMergedMaterial = pMergedMaterialIn;
	}
	VIRTUAL const char* GetMaterialMergeTexmapName(int mtlIndex,int mtlSubsetIndex,int texmapIndex)
	{
		tmp_texmap.Format("%s_%02d_%02d_%02d%s.dds",chr_filename.c_str(),mtlIndex,mtlSubsetIndex,texmapIndex,texmapIndex==EFTT_BUMP ? "_ddn" : "");
		//
		tmp_texmap = "%USER%/ModelsCache/"+tmp_texmap;
		//
		char path[ICryPak::g_nMaxPath];
		path[sizeof(path) - 1] = 0;
		gEnv->pCryPak->AdjustFileName(tmp_texmap, path, ICryPak::FLAGS_PATH_REAL | ICryPak::FLAGS_FOR_WRITING);

		tmp_texmap = path;

		return tmp_texmap;
	}
	VIRTUAL IMaterialMergePipe::EMergeMaterialsFlags GetMaterialMergeFlags() const
	{
		return IMaterialMergePipe::mfDoNotMergeTextures;
	}
	//
	//! returns name of newly created node
	VIRTUAL const char* GetModelName() const
	{
		return "MergedHead";
	}
	VIRTUAL IMergePipe::SMergePhysicsBoneResult GetPhysicsBoneMergeInfo(int chrindex,const char* modelNameFull,const char* boneName,IMergePipeBackend* backend) const
	{
		SMergePhysicsBoneResult res;
		res.okToMerge = false;
  	return res;
	}
	//
	VIRTUAL float GetMorphTargetWeight(int chrIndex,const char* targetName) const
	{
		return 0.0f;
	}
	string chr_filename;
	std::vector<IMaterial*> chrMtls;
	std::vector<string> chrFiles;
	IMaterial* pMergedMaterial;
	string tmp_texmap;
};
























































































































































































































































































































static bool recalcSkinning(CMesh* pTgtMesh,int fromVertex,int toVertex,SMeshSubset& srcSubset,PodArray<uint16>& TgtArrGlobalBonesPerSubset,std::map<uint16,uint16>& boneMap,bool bUpdateVerticies)
{
  SMeshBoneMapping* pBoneMapping;
  int nBoneMappingElemSize;
  pTgtMesh->GetStreamInfo(CMesh::BONEMAPPING,(void*&)pBoneMapping,nBoneMappingElemSize);
  // check for good skin group
  TgtArrGlobalBonesPerSubset = pTgtMesh->m_subsets.back().m_arrGlobalBonesPerSubset;
  for(int v=fromVertex;v<toVertex;v++)
  {
    for(int w=0;w<4;w++)
    {
      f32 w0 = pBoneMapping[v].weights[w];
      if( w0!=0 )
      {
        uint8 boneID = pBoneMapping[v].boneIDs[w];
        uint16 globalBoneId = srcSubset.m_arrGlobalBonesPerSubset[boneID];
        std::map<uint16,uint16>::iterator it = boneMap.find(globalBoneId);
        if( it==boneMap.end() )
        {
          return false;
        }
        globalBoneId = it->second;
        // find in currently existent
        bool bFound = false;
        for(uint32 b=0;b<TgtArrGlobalBonesPerSubset.size();b++)
        {
          if( TgtArrGlobalBonesPerSubset[b]==globalBoneId )
          {
            boneID = b;
            bFound = true;
            if( bUpdateVerticies )
            {
              pBoneMapping[v].boneIDs[w] = boneID;
            }
            break;
          }
        }
        if( !bFound )
        {
          // add if not exist
          TgtArrGlobalBonesPerSubset.push_back(globalBoneId);
          if( bUpdateVerticies )
          {
            pBoneMapping[v].boneIDs[w] = TgtArrGlobalBonesPerSubset.size()-1;
          }
        }
      }
    }
  }
  return true;
}

struct MergePipeBackend : IMergePipeBackend
{
  MergePipeBackend(std::vector<CContentCGF*>*pcontents,IMergePipe* pipe)
  {
    contents = pcontents;
    m_pipe = pipe;
  }
  
  void GetFaceVerticies(int chrIndex,int face,bool bIncludeMorphs,Vec3* triangle) const
  {
    CSkinningInfo* info = contents->operator[](chrIndex)->GetSkinningInfo();
    //
    int ind[3] = 
    {
      info->m_arrIntFaces[face].i0,
      info->m_arrIntFaces[face].i1,
      info->m_arrIntFaces[face].i2,
    };

    for(int n=0;n<3;n++)
    {
      int i = ind[n];
      IntSkinVertex& vx = info->m_arrIntVertices[i];
      triangle[n] = info->m_arrIntVertices[i].wpos0;
      // morph
      for(int m=0;m<info->m_arrMorphTargets.size();m++)
      {
        float weight = m_pipe->GetMorphTargetWeight(chrIndex,info->m_arrMorphTargets[m]->m_strName);
        for(int v=0;v<info->m_arrMorphTargets[m]->m_arrIntMorph.size();v++)
        {
          if( i==info->m_arrMorphTargets[m]->m_arrIntMorph[v].nVertexId )
          {
            triangle[n]+=info->m_arrMorphTargets[m]->m_arrIntMorph[v].ptVertex*weight;
          }
        }
      }
      //
      //get indices for bones (always 4 indices per vertex)
      uint32 b0 = vx.boneIDs[0];
      uint32 b1 = vx.boneIDs[1];
      uint32 b2 = vx.boneIDs[2];
      uint32 b3 = vx.boneIDs[3];

      //get indices for vertices (always 4 weights per vertex)
      f32 w0 = vx.weights[0];
      f32 w1 = vx.weights[1];
      f32 w2 = vx.weights[2];
      f32 w3 = vx.weights[3];
      //assert(fabsf((w0+w1+w2+w3)-1.0f)<0.0001f);

      const Matrix34& m0 = info->m_arrBonesDesc[b0].m_DefaultB2W;
      const Matrix34& m1 = info->m_arrBonesDesc[b1].m_DefaultB2W;
      const Matrix34& m2 = info->m_arrBonesDesc[b2].m_DefaultB2W;
      const Matrix34& m3 = info->m_arrBonesDesc[b3].m_DefaultB2W;
      Vec3 v0=m0*triangle[n]*w0;
      Vec3 v1=m1*triangle[n]*w1;
      Vec3 v2=m2*triangle[n]*w2;
      Vec3 v3=m3*triangle[n]*w3;
      triangle[n] = v0+v1+v2+v3;
    }
  }
  std::vector<CContentCGF*>* contents;
  IMergePipe* m_pipe;
};

struct CMeshLodGenerator
{
  CMeshLodGenerator()
  {
  }
  //
  typedef std::pair<string,std::vector<Vec3> > morphChannel;
  // non collapsable:
  //   --- is in use by any open edge
  struct Vertex 
  {
    struct SplitData
    {
      Vec2 uv;
      SMeshTangents tangent;
      ColorB color;
      Vec3 normal;
      int vb_index;
      SplitData()
      {
        vb_index=-1;
      }
    };
    Vertex()
    {
      numSplits = 0;
      bCollapsable = true;
      allNormalsEqual= true;
    }
    bool isEqual(const Vec3& p_other,const SMeshBoneMapping& bm_other)
    {
      if( this->p!=p_other)
        return false;
      if( bm.boneIDs!=bm_other.boneIDs ||
          bm.weights!=bm_other.weights )
        return false;
      return true;
    }
    int addSplit(const SplitData& split,bool bAllowTexSplits)
    {
      bool bFound = false;
      int nn=0;
      for(nn=0;nn<numSplits;nn++)
      {
        if( 
          splits[nn].normal!=split.normal ||
          splits[nn].uv!=split.uv ||
          splits[nn].color!=split.color ||
          splits[nn].tangent.Tangent[0]!=split.tangent.Tangent[0] || 
          splits[nn].tangent.Tangent[1]!=split.tangent.Tangent[1] || 
          splits[nn].tangent.Tangent[2]!=split.tangent.Tangent[2] ||
          splits[nn].tangent.Tangent[3]!=split.tangent.Tangent[3] || 
          splits[nn].tangent.Binormal[0]!=split.tangent.Binormal[0] ||
          splits[nn].tangent.Binormal[1]!=split.tangent.Binormal[1] ||
          splits[nn].tangent.Binormal[2]!=split.tangent.Binormal[2] ||
          splits[nn].tangent.Binormal[3]!=split.tangent.Binormal[3] )
        {
          continue;
        }
        bFound = true;
        break;
      }
      if( !bFound )
      {
        //
        if( numSplits>=1 && !bAllowTexSplits )
          return -1;
        //
        if( numSplits+1<MAX_SPLITS )
        {
          splits[numSplits++] = split;
          nn=(numSplits-1);
        }
        else 
        {
          nn=-1;
        }
      }
      //
      if( nn!=-1 )
      {
        // update allNormalsEqual variable
        if( allNormalsEqual && !bFound )
        {
          for(int n=1;n<numSplits;n++)
          {
            if( splits[0].normal!=splits[n].normal )
            {
              allNormalsEqual = false;
              break;
            }
          }
        }
      }
      //
      return nn;
    }
    Vec4sf Slerp(const Vec4sf& v0,const Vec4sf& v1,float t) 
    {
      Vec3 _v0 = Vec3(tPackB2F(v0.x),tPackB2F(v0.y),tPackB2F(v0.z));
      _v0.Normalize();
      Vec3 _v1 = Vec3(tPackB2F(v1.x),tPackB2F(v1.y),tPackB2F(v1.z));
      _v1.Normalize();
      //
      //
      _v0.SetSlerp(_v0,_v1,t);
      return Vec4sf(tPackF2B(_v0.x), tPackF2B(_v0.y), tPackF2B(_v0.z), tPackF2B(1));
    }
    ColorB Lerp(const ColorB& v0,const ColorB& v1,float t) 
    {
      Vec4 _v0 = Vec4(v0.r/255.0f,v0.g/255.0f,v0.b/255.0f,v0.a/255.0f);
      Vec4 _v1 = Vec4(v1.r/255.0f,v1.g/255.0f,v1.b/255.0f,v1.a/255.0f);
      //
      _v0 = _v0*(1.0f-t) + _v1*t;
      return ColorB((uint8)(_v0.x*255.0f), (uint8)(_v0.y*255.0f), (uint8)(_v0.z*255.0f), (uint8)(_v0.w*255.0f));
    }
    void Lerp(const Vertex& vx,float t,std::vector< morphChannel >& morphs,int index1,int index2)
    {
      assert(!bCollapsable);
      //
      bool bLinear = !allNormalsEqual || !vx.allNormalsEqual;
      Vec3 orgp = p;
      p = GetCollapsePos(splits[0].normal,vx.splits[0].normal,p,vx.p,t,bLinear);
      // do it for morphs also
      if( morphs.size() )
      {
        for(uint32 n=0;n<morphs.size();n++)
        {
          Vec3 pMorph = GetCollapsePos(splits[0].normal,vx.splits[0].normal,orgp+morphs[n].second[index1],vx.p+morphs[n].second[index2],t,bLinear);
          morphs[n].second[index1] = pMorph-p;
        }
      }
      if( bm.boneIDs!=vx.bm.boneIDs )
      {
        // merge blend weights
        int numBones = 0;
        byte bones[8] = {0,0,0,0,0,0,0,0};
        float weights[8] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
        // create 8 weights 
        for( int w=0;w<4;w++ )
        {
          if( bm.weights[w]!=0 )
          {
            bones[numBones]=bm.boneIDs[w];
            weights[numBones] = (float)bm.weights[w]/float(255.0f);
            numBones++;
          }
        }
        for( int w=0;w<4;w++ )
        {
          bool bFound = false;
          for( int w1=0;w1<numBones;w1++ )
          {
            if( vx.bm.boneIDs[w]==bones[w1] )
            {
              bFound = true;
              weights[w1]+=(float)vx.bm.weights[w]/float(255.0f);
              break;
            }
          }
          if( !bFound )
          {
            bones[numBones]=vx.bm.boneIDs[w];
            weights[numBones] = (float)vx.bm.weights[w]/float(255.0f);
            numBones++;
          }
        }
        // buble sort
        bool bSorted;
        do
        {
          bSorted = true;
          for(int n=0;n<numBones-1;n++)
          {
            if( weights[n]>weights[n+1] )
            {
              std::swap(weights[n],weights[n+1]);
              std::swap(bones[n],bones[n+1]);
              bSorted = false;
            }
          }
        } while(!bSorted);
        // normalize first max(numBones,4);
        float summ = weights[0]+weights[1]+weights[2]+weights[3];
        weights[0] /= summ;
        weights[1] /= summ;
        weights[2] /= summ;
        weights[3] /= summ;
        //
        bm.boneIDs = ColorB(bones[0],bones[1],bones[2],bones[3]);
        bm.weights = ColorB((uint8)(weights[0]*255.0f),(uint8)(weights[1]*255.0f),(uint8)(weights[2]*255.0f),(uint8)(weights[3]*255.0f));
      }
      else
      {
        bm.weights = Lerp(bm.weights,vx.bm.weights,t);
      }
      int sum = bm.weights[0]+bm.weights[1]+bm.weights[2]+bm.weights[3];
      if( sum<255 )
      {
        bm.weights[0]+=255-sum;
      }
      else if( sum>255 )
      {
        int delta = 255-sum;
        if( bm.weights[0]+delta>=0 )
        {
          bm.weights[0]+=delta;
        }
        else if( bm.weights[1]+delta>=0 )
        {
          bm.weights[1]+=delta;
        }
        else if( bm.weights[2]+delta>=0 )
        {
          bm.weights[2]+=delta;
        }
        else if( bm.weights[3]+delta>=0 )
        {
          bm.weights[3]+=delta;
        }
      }
      //
      for(int n=0;n<numSplits;n++)
      {
        splits[n].normal = Vec3::CreateSlerp(splits[n].normal,vx.splits[n].normal,t);
        splits[n].tangent.Tangent = Slerp(splits[n].tangent.Tangent,vx.splits[n].tangent.Tangent,t);
        splits[n].tangent.Binormal = Slerp(splits[n].tangent.Binormal,vx.splits[n].tangent.Binormal,t);
        //
        Vec3 tangent = Vec3(tPackB2F(splits[n].tangent.Tangent.x),tPackB2F(splits[n].tangent.Tangent.y),tPackB2F(splits[n].tangent.Tangent.z));
        Vec3 binormal = Vec3(tPackB2F(splits[n].tangent.Binormal.x),tPackB2F(splits[n].tangent.Binormal.y),tPackB2F(splits[n].tangent.Binormal.z));
        Vec3 cross = (tangent%binormal);
        if( cross.Dot(splits[n].normal)<0 )
        {
            splits[n].tangent.Tangent.w = tPackF2B(-1);
            splits[n].tangent.Binormal.w = tPackF2B(-1);
        }
        splits[n].uv.SetLerp(splits[n].uv,vx.splits[n].uv,t);
        splits[n].color = Lerp(splits[n].color,vx.splits[n].color,t);
      }
    }
    static Vec3 GetCollapsePos(const Vec3& n0,const Vec3& n1,const Vec3& v0,const Vec3& v1,const float& t,bool bLinear)
    {
      if( bLinear )
      {
        Vec3 res;
        res.SetLerp(v0,v1,t);
        return res;
      }
      //return (v0+v1)*0.5f;
      //
      float lenEdge = (v0-v1).GetLength();
      //
      Vec3 v0t = (n0%((v1-v0)%n0)).GetNormalized();
      Vec3 v1t = (n1%((v0-v1)%n1)).GetNormalized();
      //
      v0t=v0t*lenEdge*1.0f/3.0f+v0;
      v1t=v1t*lenEdge*1.0f/3.0f+v1;
      //
      // bezier interpolation
      float invt = (1.0f-t);
      float invt2 = invt*invt;
      float invt3 = invt2*invt;
      float t2 = t*t;
      float t3 = t2*t;
      return invt3*v0+3.0f*invt2*t*v0t+3.0f*invt*t2*v1t+t3*v1;
    }
    //
    Vec3 p;
    SMeshBoneMapping bm;
    bool allNormalsEqual;
    int numSplits;
    static const int MAX_SPLITS = 16;
    SplitData splits[MAX_SPLITS];
    bool bCollapsable;
  };
  struct Edge;
  struct Face
  {
    Face()
    {
      processID = INT_MIN;
      ajacency.Set(-1,-1,-1);
      edges.Set(-1,-1,-1);
    }
    bool HasVertex(int v) const
    {
      if( indicies[0]==v || indicies[1]==v || indicies[2]==v )
        return true;
      return false;
    }
    void GetEdges(int &e2,int &e3,int &f3,int &f4,int thisFace,int ignoreFace,int ignoreEdge,const Edge* edgeList,int v0,int v1);
    bool ReplaceVertex(int vOld,int vNew)
    {
      if( vOld==vNew )
        return false;
      if( indicies[0]==vOld )
      {
        indicies[0] = vNew;
      }
      else if( indicies[1]==vOld )
      {
        indicies[1] = vNew;
      }
      else if( indicies[2]==vOld )
      {
        indicies[2] = vNew;
      }
      else
      {
        return false;
      }
      //
      if( isDegenerate() )
      {
        //CryLog("CCLOD:Face:ReplaceVertex error");
      }
      return true;
    }
    bool isDegenerate() const
    {
      if( indicies[0]==indicies[1] || indicies[0]==indicies[2] || indicies[1]==indicies[2]  )
        return true;
      return false;
    }
    void ReplaceAjacency(int fOld,int fNew)
    {
/*
      // for open face is might be valid
      if( fNew==-1)
      {
        CryLog("CCLOD:Face:ReplaceAjacency error new face is -1");
        return;
      }
*/
      for(int n=0;n<3;n++)
      {
        if( ajacency[n]==fOld )
        {
          ajacency[n]=fNew;
          break;
        }
      }
      if( ajacency[0]==ajacency[1] || ajacency[0]==ajacency[2] || ajacency[1]==ajacency[2] )
      {
        CryLog("CCLOD:Face:ReplaceAjacency error");
      }
    }
    void ReplaceAjacencyEdge(int eOld,int eNew)
    {
/*
      // for open face is might be valid
      if( eNew==-1)
      {
        CryLog("CCLOD:Face:ReplaceAjacencyEdge new edge is -1");
        return;
      }
*/
      for(int n=0;n<3;n++)
      {
        if( edges[n]==eOld )
        {
          edges[n]=eNew;
          break;
        }
      }
      if( edges[0]==edges[1] || edges[0]==edges[2] || edges[1]==edges[2] )
      {
        CryLog("CCLOD:Face:ReplaceAjacencyEdge error");
      }
    }
    void Remove() 
    {
      edges[0]=edges[1]=edges[2]=-1;
      indicies[0]=indicies[1]=indicies[2]=-1;
      ajacency[0]=ajacency[1]=ajacency[2]=-1;
    }
    bool isRemoved() const
    {
      if( indicies[0]==-1 || indicies[1]==-1 || indicies[2]==-1 )
        return true;
      return false;
    }
    bool HasEdge(int e) const
    {
      if( edges[0]==e || edges[1]==e || edges[2]==e )
        return true;
      return false;
    }
    //
    Vec3 faceNormal;
    Vec3i indicies;
    Vec3i indicies_split;
    //
    Vec3i ajacency;
    Vec3i edges;
    int processID;
  };
  struct Edge
  {
    Edge()
    {
      faces=Vec2i(-1,-1);
      verts=Vec2i(-1,-1);
      edgeIndex = -1;
      metric = 0.0f;
    }
    bool operator<(const Edge& other) const
    {
      if( verts[0]<other.verts[0] )
        return true;
      if( verts[0]>other.verts[0] )
        return false;
      if( verts[1]<other.verts[1] )
        return true;
      return false;
    }
    bool isEqual(int a,int b) const
    {
      if( verts[0]==a && verts[1]==b )
        return true;
      if( verts[0]==b && verts[1]==a )
        return true;
      return false;
    }
    bool isBetweenFaces(int a,int b) const
    {
      if( faces[0]==a && faces[1]==b )
        return true;
      if( faces[0]==b && faces[1]==a )
        return true;
      return false;
    }
    bool IsRemoved()  const
    {
      if( verts[0]==-1 || verts[1]==-1 )
        return true;
      return false;
    }
    bool HasVertex(int v) const
    {
      if( verts[0]==v || verts[1]==v )
        return true;
      return false;
    }
    void Remove()
    {
      metric = FLT_MAX;
      verts[0]=verts[1]=-1;
      faces[0]=faces[1]=-1;
    }
    void ReplaceFace(int fOld,int fNew)
    {
      if( faces[0]==fOld )
      {
        faces[0] = fNew;
      }
      else if( faces[1]==fOld )
      {
        faces[1] = fNew;
      }
      //
      if( faces[1]==faces[0] )
      {
        CryLog("CCLOD:Edge:ReplaceFace error");
      }
    }
    bool ReplaceVertex(int vOld,int vNew)
    {
      if( vOld==vNew )
      {
        return false;
      }
      if( verts[0]==vOld )
      {
        verts[0] = vNew;
      }
      else if( verts[1]==vOld )
      {
        verts[1] = vNew;
      }
      else
      {
        return false;
      }
/*
      if( verts[0]==verts[1] )
      {
        CryLog("CCLOD:Edge:ReplaceVector error");
      }
*/
      return true;
    }
    //
    void calcMetric(int v0,int v1,const Vec3& vNew,int faceCur,const std::vector<Vertex>& vertexList,std::vector<Face>& faceList, int faceStop1,int faceStop2, float& minDot,float& minValue,int processID)
    {
      /*
      for(int f=0;f<faceList.size();f++)
      {
        if( faceList[f].isRemoved())
          continue;
        if( f==faceStop1 )
          continue;
        if( f==faceStop2 )
          continue;
        Face& face = faceList[f];
        //
        if( !face.HasVertex(v0) && !face.HasVertex(v1) )
        {
          continue;
        }
        //
        int _v1 = face.indicies[0];
        int _v2 = face.indicies[1];
        int _v3 = face.indicies[2];
        //
        Vec3 pv1 = vertexList[_v1].p;
        Vec3 pv2 = vertexList[_v2].p;
        Vec3 pv3 = vertexList[_v3].p;
        if( _v1==v0 || _v1==v1 )
        {
          pv1 = vNew;
        }
        if( _v2==v0 || _v2==v1 )
        {
          pv2 = vNew;
        }
        if( _v3==v0 || _v3==v1 )
        {
          pv3 = vNew;
        }
        if( !faceList[f].isDegenerate() )
        {
          Vec3 newFaceNormal = ((pv2-pv1)%(pv3-pv1)).GetNormalized();	//vector cross-product
          float dot = newFaceNormal.Dot(face.faceNormal);
          minDot = min(minDot,dot);
        }
      }
      */
      ///*
      if( faceCur==faceStop1 )
        return;
      if( faceCur==faceStop2 )
        return;
      //
      Face& face = faceList[faceCur];
      //
      if( face.processID==processID )
        return;
      //
      if( !face.HasVertex(v0) && !face.HasVertex(v1) )
      {
        return;
      }
      //
      face.processID = processID;
      //
      int _v1 = face.indicies[0];
      int _v2 = face.indicies[1];
      int _v3 = face.indicies[2];
      //
      Vec3 pv1 = vertexList[_v1].p;
      Vec3 pv2 = vertexList[_v2].p;
      Vec3 pv3 = vertexList[_v3].p;
      //
      /*
      float minAngleBefore1 = (pv2-pv1).GetNormalized().Dot((pv3-pv1).GetNormalized());
      float minAngleBefore2 = (pv3-pv2).GetNormalized().Dot((pv1-pv2).GetNormalized());
      float minAngleBefore3 = (pv2-pv3).GetNormalized().Dot((pv1-pv3).GetNormalized());
      float minAngleBefore = min(minAngleBefore1,min(minAngleBefore2,minAngleBefore3));
      */
      //
      if( _v1==v0 || _v1==v1 )
      {
        pv1 = vNew;
      }
      if( _v2==v0 || _v2==v1 )
      {
        pv2 = vNew;
      }
      if( _v3==v0 || _v3==v1 )
      {
        pv3 = vNew;
      }
      //
      Vec3 newFaceNormal = ((pv2-pv1)%(pv3-pv1)).GetNormalized();	//vector cross-product
      float dot = newFaceNormal.Dot(face.faceNormal);
      minDot = min(minDot,dot);
      //
      /*
      float _minAngleBefore1 = (pv2-pv1).GetNormalized().Dot((pv3-pv1).GetNormalized());
      float _minAngleBefore2 = (pv3-pv2).GetNormalized().Dot((pv1-pv2).GetNormalized());
      float _minAngleBefore3 = (pv2-pv3).GetNormalized().Dot((pv1-pv3).GetNormalized());
      float _minAngleBefore = min(_minAngleBefore1,min(_minAngleBefore2,_minAngleBefore3));
      //
      minValue = min(minValue,minDot-(_minAngleBefore-minAngleBefore));
      */
      minValue = min(minValue,minDot);
      //
      for(int a=0;a<3;a++)
      {
        if( face.ajacency[a]!=-1 )
        {
          calcMetric(v0,v1,vNew,face.ajacency[a],vertexList,faceList,faceStop1,faceStop2,minDot,minValue,processID);
        }
      }
      //*/
    }
    //
    void UpdateMetric(const std::vector<Vertex>& vertexList,std::vector<Face>& faceList,float maxAngle,int& processId,bool bAllowBlendSkin)
    {
      processId++;
      //
      if( metric==FLT_MAX )
        return;
      if( faces[0]!=-1 && faces[1]!=-1 )
      {
        int v1 = verts[0];
        int v2 = verts[1];
        const Vertex& vx1 = vertexList[v1];
        const Vertex& vx2 = vertexList[v2];
        //
        if( !bAllowBlendSkin && vx1.bm.boneIDs!=vx2.bm.boneIDs )
        {
          metric = FLT_MAX;
        }
        else
        {
          float minDot = FLT_MAX;
          float minValue = FLT_MAX;
          float t = 0.5f;
          Vec3 newPos = Vertex::GetCollapsePos(vx1.splits[0].normal,vx2.splits[0].normal,vx1.p,vx2.p,t,!vx1.allNormalsEqual || !vx2.allNormalsEqual );
          for(int f=0;f<2;f++)
          {
            if( faces[f]!=-1 )
            {
              for(int a=0;a<3;a++)
              {
                if( faceList[faces[f]].ajacency[a]!=-1 )
                {
                  calcMetric(v1,v2,newPos,faceList[faces[f]].ajacency[a],vertexList,faceList,faces[0],faces[1],minDot,minValue,processId);
                }
              }
            }
          }
          if( minDot<cry_cosf(maxAngle) || minDot==FLT_MAX )
          {
            metric = FLT_MAX;
          }
          else
          {
            metric = 1.0f-minValue;
          }
        }
      }
      else
      {
        metric = FLT_MAX;
      }
    }
    //
    int edgeIndex;
    float metric;
    Vec2i verts;
    Vec2i faces;
  };
  struct Subset
  {
    Subset()
    {
      m_processId = 0;
    }
    static bool Ugreater(const Vec2_tpl<byte>& v1,const Vec2_tpl<byte>& v2)
    {
      return v1.x>v2.x;
    }
    static bool UgreaterWeights(const Vec2i& v1,const Vec2i& v2)
    {
      return v1.y>v2.y;
    }
    int AddVertex(const Vertex& vx, const Vertex::SplitData& vx_split,int& split_index,bool bAllowTexSplits,bool& addedNew )
    {
      addedNew = false;
      for(uint32 v=0;v<m_vertexList.size();v++)
      {
        if( m_vertexList[v].isEqual(vx.p,vx.bm))
        {
          split_index = m_vertexList[v].addSplit(vx_split,bAllowTexSplits);
          if( split_index!=-1 )
            return v;
        }
      }
      addedNew = true;
      m_vertexList.push_back(vx);
      split_index = m_vertexList.back().addSplit(vx_split,bAllowTexSplits);
      return m_vertexList.size()-1;
    }
    bool AddFace(int n, CMesh* mesh,const SMeshSubset& subset,bool bAllowTexSplits,DynArray<MorphTargetsPtr>* arrMorphTargets)
    {
      Face face;
      Vertex vx[3];
//      int splits[3];
      Vertex::SplitData vx_split[3];
      for(int i=0;i<3;i++)
      {
        int index = mesh->m_pIndices[subset.nFirstIndexId+n*3+i];
        //
        // general part
        vx[i].p = mesh->m_pPositions[index];
        vx[i].bm = mesh->m_pBoneMapping[index];
        // assign 0 for unsed
        Vec2_tpl<byte> bones[4];
        for(int b=0;b<4;b++)
        {
          bones[b].x = vx[i].bm.boneIDs[b];
          bones[b].y = vx[i].bm.weights[b];
        }
        std::sort(&bones[0],&bones[4],Ugreater);
        vx[i].bm.boneIDs.Set(0,0,0,0);
        vx[i].bm.weights.Set(0,0,0,0);
        int ibone = 0;
        for(int b=0;b<4;b++)
        {
          //if( bones[b].y!=0 )
          {
            vx[i].bm.boneIDs[ibone] = bones[b].x;
            vx[i].bm.weights[ibone] = bones[b].y;
            ibone++;
          }
        }
        // split part
        //
        vx_split[i].normal = mesh->m_pNorms[index];
        vx_split[i].uv = Vec2(mesh->m_pTexCoord[index].s,mesh->m_pTexCoord[index].t);
        vx_split[i].tangent = mesh->m_pTangents[index];
        if( mesh->m_pColor0 )
          vx_split[i].color = (ColorB&)mesh->m_pColor0[index];
        else
          vx_split[i].color.Set(255,255,255,0);
        //
        bool baddedNew;
        face.indicies[i] = AddVertex(vx[i],vx_split[i],face.indicies_split[i],bAllowTexSplits,baddedNew);
//        splits[i] = face.indicies_split[i];
        // morph part
        if( arrMorphTargets )
        {
          if( baddedNew )
          {
            for(int m=0;m<arrMorphTargets->size();m++)
            {
              MorphTargets& target = *((*arrMorphTargets)[m]);
              m_morphs[m].second.push_back(Vec3(0));
              for(int v=0;v<target.m_arrExtMorph.size();v++)
              {
                if( target.m_arrExtMorph[v].nVertexId==index )
                {
                    m_morphs[m].second.back() = target.m_arrExtMorph[v].ptVertex;
                }
              }
            }
          }
        }
      }
      face.faceNormal = ((vx[1].p-vx[0].p)%(vx[2].p-vx[0].p)).GetNormalized();	//vector cross-product
      m_faceList.push_back(face);
      return true;
    }
    void Init(CMesh* mesh,const SMeshSubset& subset,bool bAllowTexSplits,DynArray<MorphTargetsPtr>* arrMorphTargets)
    {
      int numFaces = subset.nNumIndices/3;
      m_faceList.reserve(numFaces);
      m_vertexList.reserve(subset.nNumVerts);
      // morph part
      if( arrMorphTargets )
      {
        for(int m=0;m<arrMorphTargets->size();m++)
        {
          MorphTargets& target = *((*arrMorphTargets)[m]);
          m_morphs.push_back( morphChannel(target.m_strName,std::vector<Vec3>()) );
        }
      }
      //
      for(int n=0;n<numFaces;n++)
      {
        AddFace(n,mesh,subset,bAllowTexSplits,arrMorphTargets);
      }
      /*
      m_vertexList.resize(10);
      m_vertexList[0].p.Set(1,4,0);
      m_vertexList[1].p.Set(0,3,0);
      m_vertexList[2].p.Set(1,3,0);
      m_vertexList[3].p.Set(0,1,0);
      m_vertexList[4].p.Set(1,1,0);
      m_vertexList[5].p.Set(1,0,0);
      m_vertexList[6].p.Set(2,2,0);
      m_vertexList[7].p.Set(3,2,0);
      m_vertexList[8].p.Set(0,5,0);
      m_vertexList[9].p.Set(1,5,0);
      //
      m_faceList.resize(10);
      m_faceList[0].indicies.Set(1,0,2);
      m_faceList[1].indicies.Set(2,0,7);
      m_faceList[2].indicies.Set(4,1,2);
      m_faceList[3].indicies.Set(3,1,4);
      m_faceList[4].indicies.Set(4,2,6);
      m_faceList[5].indicies.Set(6,2,7);
      m_faceList[6].indicies.Set(4,6,7);
      m_faceList[7].indicies.Set(5,4,7);
      m_faceList[8].indicies.Set(5,3,4);
      m_faceList[9].indicies.Set(0,8,9);
      */
      /*
      m_vertexList.resize(6);
      m_vertexList[0].p.Set(0,3,0);
      m_vertexList[1].p.Set(2,1,0);
      m_vertexList[2].p.Set(1,2,0);
      m_vertexList[3].p.Set(1,1,0);
      m_vertexList[4].p.Set(0,0,0);
      m_vertexList[5].p.Set(2,0,0);
      //
      m_faceList.resize(6);
      m_faceList[0].indicies.Set(2,0,1);
      m_faceList[1].indicies.Set(3,0,2);
      m_faceList[2].indicies.Set(1,3,2);
      m_faceList[3].indicies.Set(4,0,3);
      m_faceList[4].indicies.Set(5,3,1);
      m_faceList[5].indicies.Set(4,3,5);
      */
      /*
      m_faceList[0].indicies.Set(2,0,1);
      m_faceList[1].indicies.Set(4,0,2);
      m_faceList[2].indicies.Set(5,2,1);
      m_faceList[3].indicies.Set(4,2,3);
      m_faceList[4].indicies.Set(5,3,2);
      m_faceList[5].indicies.Set(4,3,5);
      */
      /*
      m_vertexList.resize(5);
      m_vertexList[0].p.Set(1,3,0);
      m_vertexList[1].p.Set(1,2,0);
      m_vertexList[2].p.Set(1,1,0);
      m_vertexList[3].p.Set(0,0,0);
      m_vertexList[4].p.Set(2,0,0);
      //
      m_faceList.resize(5);
      m_faceList[0].indicies.Set(3,0,1);
      m_faceList[1].indicies.Set(4,1,0);
      m_faceList[2].indicies.Set(3,1,2);
      m_faceList[3].indicies.Set(4,2,1);
      m_faceList[4].indicies.Set(3,2,4);
      */
      //
      // calculate edges
      BuildEdges();
      m_numEdgesToUpdateMetric = 0;
      m_edgeListUpdated.resize(m_edgeList.size(),-1);
      // fixup equal 2 faces for 1 edge
      for(uint32 e=0;e<m_edgeList.size();e++)
      {
        if( m_edgeList[e].faces[0]==m_edgeList[e].faces[1] )
        {
          m_edgeList[e].faces[1] = -1;
        }
      }
      // generate adjacencies
      for(uint32 e=0;e<m_edgeList.size();e++)
      {
        // edges
        for(int f=0;f<2;f++)
        {
          int face = m_edgeList[e].faces[f];
          if( face==-1 )
            continue;
          //
          if( m_faceList[face].edges[0]!=e && m_faceList[face].edges[1]!=e && m_faceList[face].edges[2]!=e )
          {
            bool bFound = false;
            for(int a=0;a<3;a++)
            {
              if( m_faceList[face].edges[a]==-1 )
              {
                m_faceList[face].edges[a] = e;
                bFound = true;
                break;
              }
            }
            if( !bFound )
            {
              CryLog("CCLOD:Degenerated Face found");
            }
          }
        }
        // face ajacency
        if( m_edgeList[e].faces[0]==-1 || m_edgeList[e].faces[1]==-1)
          continue;
        //
        for(int f=0;f<2;f++)
        {
          int face = m_edgeList[e].faces[f];
          int faceAjac = m_edgeList[e].faces[(f+1)%2];
          //
          if( m_faceList[face].ajacency[0]!=faceAjac && m_faceList[face].ajacency[1]!=faceAjac && m_faceList[face].ajacency[2]!=faceAjac )
          {
            bool bFound = false;
            for(int a=0;a<3;a++)
            {
              if( m_faceList[face].ajacency[a]==-1 )
              {
                m_faceList[face].ajacency[a] = faceAjac;
                bFound = true;
                break;
              }
            }
            if( !bFound )
            {
              CryLog("CCLOD:Degenerated Face found");
            }
          }
        }
      }
    } 
    //          v0 (new)
    //         /|\
    //  f3    / | \    f5
    //       /  |  \
    //   e2 /   |   \ e4
    //     /    |    \
    //    /     |     \
    //   /   f1 | f2   \
    //   \      |      /
    //    \     e     /
    //     \    |    /
    //   e3 \   |   / e5
    //       \  |  /
    //  f4    \ | /    f6
    //         \|/
    //          v1 (old)

    void CollapseEdge(int e,float maxAngle,int& processId,bool bAllowBlendSkin,std::vector< morphChannel >& morphs)
    {
      // check for valid edge
      assert(!m_edgeList[e].IsRemoved());
      if( m_edgeList[e].IsRemoved() )
        return;
      //
      //if( m_vertexList[m_edgeList[e].verts[0]].numSplits!=m_vertexList[m_edgeList[e].verts[1]].numSplits )
      //{
      //  int ggg=0;
      //}
      //
      //
      int v0 = m_edgeList[e].verts[0];
      int v1 = m_edgeList[e].verts[1];
      int f1a = m_edgeList[e].faces[0];
      int f2a = m_edgeList[e].faces[1];
      //
      int e2a[2],e3a[2],f3a[2],f4a[2];
        m_faceList[f1a].GetEdges(e2a[0],e3a[0],f3a[0],f4a[0],f1a,f2a,e,&m_edgeList[0],v0,v1);
        m_faceList[f2a].GetEdges(e2a[1],e3a[1],f3a[1],f4a[1],f2a,f1a,e,&m_edgeList[0],v0,v1);
      // 
      if( f3a[0]==f3a[1] && f4a[0]==f4a[1] )
      {
        m_edgeList[e].metric = FLT_MAX;
        return;
      }
      //
      for(int f=0;f<2;f++)
      {
        if( f3a[f]!=-1 && f4a[f]!=-1)
        {
          // at min one common edge
          if( m_faceList[f3a[f]].HasEdge(m_faceList[f4a[f]].edges[0]) || 
            m_faceList[f3a[f]].HasEdge(m_faceList[f4a[f]].edges[1]) ||
            m_faceList[f3a[f]].HasEdge(m_faceList[f4a[f]].edges[2]) )
          {
            m_edgeList[e].metric = FLT_MAX;
            return;
          }
        }
      }
      //
      RebindVertex(v1,v0,f1a!=-1 ? f1a : f2a);
      //
/*
      for(int n=0;n<m_faceList.size();n++)
      {
        if( m_faceList[n].HasVertex(v1) )
        {
          int gg=0;
        }
      }
*/
      //       for(int n=0;n<m_edgeList.size();n++)
      //       {
      //         if( m_edgeList[n].HasVertex(v1) )
      //         {
      //           int gg=0;
      //         }
      //       }
      // update edges and adjacencies
      for(int f=0;f<2;f++)
      {
        int f1 = m_edgeList[e].faces[f];
        int f2 = m_edgeList[e].faces[(f+1)%2];
        if( f1==-1 )
          continue;
        //
        //
        int e2 = e2a[f];
        int e3 = e3a[f];
        int f3 = f3a[f];
        int f4 = f4a[f];
        //m_faceList[f1].GetEdges(e2,e3,f3,f4,f1,f2,e,m_edgeList,v0);
        //
        if( f3==-1 && f4==-1 )
        {
          m_edgeList[e2].Remove();
          m_edgeList[e3].Remove();
        }
        else if( f4==-1 )
        {
          m_edgeList[e3].Remove();
          m_edgeList[e2].ReplaceFace(f1,-1);
          m_faceList[f3].ReplaceAjacency(f1,-1);
        }
        else if( f3==-1 )
        {
          m_edgeList[e2].Remove();
          m_edgeList[e3].ReplaceFace(f1,-1);
          m_faceList[f4].ReplaceAjacency(f1,-1);
        }
        else
        {
          assert(f1!=-1);
          assert(f2!=-1);
          assert(e2!=-1);
          assert(e3!=-1);
          //
          m_edgeList[e2].ReplaceFace(f1,f4);
#if defined(_DEBUG) && defined(WIN32)
          checkEdge(e2);
#endif
          m_faceList[f3].ReplaceAjacency(f1,f4);
          //
          m_edgeList[e3].Remove();//ReplaceFace(f1,f3);
          m_faceList[f4].ReplaceAjacency(f1,f3);
          m_faceList[f4].ReplaceAjacencyEdge(e3,e2);
#if defined(_DEBUG) && defined(WIN32)
          checkFace(f4);
          checkEdgeNoUsed(e3,f1,f4);
#endif
        }
      }
      // remove faces
      if( f1a!=-1 )
        m_faceList[f1a].Remove();
      if( f2a!=-1 )
        m_faceList[f2a].Remove();
      m_edgeList[e].Remove();
      //
      m_vertexList[v0].Lerp(m_vertexList[v1],0.5f,morphs,v0,v1);
      //
      /*
      std::vector<int> ggg;
      for(int e=0;e<m_edgeList.size();e++)
      {
        if( m_edgeList[e].HasVertex(v0) )
        {
          m_edgeList[e].UpdateMetric(m_vertexList,m_faceList,maxAngle,processId);
          ggg.push_back(e);
        }
      }
      */
      //
      m_numEdgesToUpdateMetric=0;
      FindChangedEdges(f3a[0],f3a[1],f4a[0],f4a[1],v0,processId);
      //
      /*
      if( m_numEdgesToUpdateMetric!=ggg.size() )
      {
        int hhh=0;
      }
      */
      for(int n=0;n<m_numEdgesToUpdateMetric;n++)
      {
        m_edgeList[m_edgeListUpdated[n]].UpdateMetric(m_vertexList,m_faceList,maxAngle,processId,bAllowBlendSkin);
      }
      m_numEdgesToUpdateMetric=0;
#if defined(_DEBUG) && defined(WIN32)
      //
      checkEdgeNoUsed(e,f1a,f2a);
      //
      Check();
      //
#endif
    }
    void checkEdge(int n)
    {
      for(int f=0;f<2;f++)
      {
        int face = m_edgeList[n].faces[f];
        if( face==-1 )
          continue;
        if( m_faceList[face].isRemoved() )
        {
          CryLog("checkEdge failed");
        }
      }
    }
    void checkEdgeNoUsed(int e,int f1,int f2)
    {
      for(uint32 n=0;n<m_faceList.size();n++)
      {
        if( n!=f1 && n!=f2 )
        {
          for(int a=0;a<3;a++)
          {
            int aedge = m_faceList[n].edges[a];
            if( aedge==e )
            {
              CryLog("checkEdgeNoUsed failed");
            }
          }
        }
      }
    }
    void checkFace(int f)
    {
      Face& face = m_faceList[f];
      if( face.edges[0]==face.edges[1] || face.edges[0]==face.edges[2] || face.edges[1]==face.edges[2] )
      {
        CryLog("checkFace failed");
      }
    }
    bool Check() const
    {
      for(uint32 n=0;n<m_edgeList.size();n++)
      {
        for(int f=0;f<2;f++)
        {
          int face = m_edgeList[n].faces[f];
          if( face==-1 )
            continue;
          if( m_faceList[face].isRemoved() )
          {
            CryLog("check failed 1");
          }
        }
      }
      for(uint32 n=0;n<m_faceList.size();n++)
      {
        for(int a=0;a<3;a++)
        {
          int aface = m_faceList[n].ajacency[a];
          if( aface!=-1 )
          {
            if( m_faceList[aface].isRemoved() )
            {
              CryLog("check failed 2");
            }
          }
        }
      }
      for(uint32 n=0;n<m_faceList.size();n++)
      {
        for(int32 a=0;a<3;a++)
        {
          int aedge = m_faceList[n].edges[a];
          if( aedge!=-1 )
          {
            if( m_edgeList[aedge].IsRemoved() )
            {
              CryLog("check failed 3");
            }
          }
        }
      }
      return true;
    }
    //
    void RebindVertex(int vOld, int vNew,int face)
    {
      if( vOld==vNew )
      {
        return;
      }
      bool bFound = m_faceList[face].ReplaceVertex(vOld,vNew);
      for(int v=0;v<3;v++)
      {
        if( m_edgeList[m_faceList[face].edges[v]].ReplaceVertex(vOld,vNew) )
        {
          m_edgeListUpdated[m_numEdgesToUpdateMetric++]=m_faceList[face].edges[v];
          bFound = true;
        }
      }
      if( bFound )
      {
/*
        Vec3 normal = (m_vertexList[m_faceList[face].indicies[1]].p-m_vertexList[m_faceList[face].indicies[0]].p)%(m_vertexList[m_faceList[face].indicies[2]].p-m_vertexList[m_faceList[face].indicies[0]].p);
        m_faceList[face].faceNormal = normal.GetNormalized();	
*/
        // recurse for all adjacencies
        for(int a=0;a<3;a++)
        {
          int aface = m_faceList[face].ajacency[a];
          if( aface!=-1 )
          {
            RebindVertex(vOld,vNew,aface);
          }
        }
      }
    }
    void FindChangedEdges(int f1,int f2,int f3,int f4,int v,int& processId)
    {
      processId++;
      if( f1!=-1 )
      {
        FindChangedEdges(v,f1,processId);
      }
      if( f2!=-1 )
      {
        FindChangedEdges(v,f2,processId);
      }
      if( f3!=-1 )
      {
        FindChangedEdges(v,f3,processId);
      }
      if( f4!=-1 )
      {
        FindChangedEdges(v,f4,processId);
      }
    }
    void FindChangedEdges(int v0,int face,int& processId)
    {
      // recursion check
      if( m_faceList[face].processID==processId )
        return;
      //
      m_faceList[face].processID = processId;
      //
      bool bFound = false;
      for(int v=0;v<3;v++)
      {
        int e = m_faceList[face].edges[v];
        if( e!=-1 && m_edgeList[e].HasVertex(v0) )
        {
          m_edgeListUpdated[m_numEdgesToUpdateMetric++]=e;
          bFound = true;
        }
      }
      if( bFound )
      {
        // recurse for all adjacencies
        for(int a=0;a<3;a++)
        {
          int aface = m_faceList[face].ajacency[a];
          if( aface!=-1 )
          {
            FindChangedEdges(v0,aface,processId);
          }
        }
      }
    }
    //
    void ReBuildMetric(float maxAngle,int& processID,bool bAllowBlendSkin)
    { 
      for(uint32 n=0;n<m_edgeList.size();n++)
      {
        m_edgeList[n].UpdateMetric(m_vertexList,m_faceList,maxAngle,processID,bAllowBlendSkin);
      }
    }
    // Return whether first element is greater than the second
    static bool less_cmp ( Edge* elem1, Edge* elem2 )
    {
      return elem1->metric < elem2->metric;
    }


    void Process(float maxAngle,bool bAllowBlendSkin)
    { 
      ReBuildMetric(maxAngle,m_processId,bAllowBlendSkin);
      std::vector<Edge*> edges;
      // mark non collapsable verticies and edges
      for(uint32 n=0;n<m_edgeList.size();n++)
      {
        if( m_edgeList[n].faces[0]==-1 || m_edgeList[n].faces[1]==-1 )
        {
          m_vertexList[m_edgeList[n].verts[0]].bCollapsable = false;
          m_vertexList[m_edgeList[n].verts[1]].bCollapsable = false;
          m_edgeList[n].metric = FLT_MAX;
        }
        else if( m_vertexList[m_edgeList[n].verts[0]].numSplits!=m_vertexList[m_edgeList[n].verts[1]].numSplits )
        {
          //m_vertexList[m_edgeList[n].verts[0]].bCollapsable = false;
          //m_vertexList[m_edgeList[n].verts[1]].bCollapsable = false;
          m_edgeList[n].metric = FLT_MAX;
        }
        //else if( m_vertexList[m_edgeList[n].verts[0]].numSplits>1 && m_vertexList[m_edgeList[n].verts[1]].numSplits>1 )
        //{
        //  m_edgeList[n].metric = FLT_MAX;
        //}
      }
      // mark non collapsable verticies and edges
      for(uint32 n=0;n<m_edgeList.size();n++)
      {
        if( m_edgeList[n].metric!=FLT_MAX )
        {
          if( m_vertexList[m_edgeList[n].verts[0]].numSplits==1 || m_vertexList[m_edgeList[n].verts[1]].numSplits==1 )
            continue;
          //if face has different indicies for split for the same edge - swap it
          for(int f=0;f<2;f++)
          {
            Face& face = m_faceList[m_edgeList[n].faces[f]];
            int i0=0,i1=0;
            if( face.indicies[0]==m_edgeList[n].verts[0] )
            {
              i0 = 0;
            }
            else if( face.indicies[1]==m_edgeList[n].verts[0] )
            {
              i0 = 1;
            }
            else if( face.indicies[2]==m_edgeList[n].verts[0] )
            {
              i0 = 2;
            }
            //
            if( face.indicies[0]==m_edgeList[n].verts[1] )
            {
              i1 = 0;
            }
            else if( face.indicies[1]==m_edgeList[n].verts[1] )
            {
              i1 = 1;
            }
            else if( face.indicies[2]==m_edgeList[n].verts[1] )
            {
              i1 = 2;
            }
            //
            if( face.indicies_split[i0]!=face.indicies_split[i1] )
            {
              m_edgeList[n].metric = FLT_MAX;
              break;
            }
          }
        }
      }
      //
      for(uint32 n=0;n<m_edgeList.size();n++)
      {
        if( !m_vertexList[m_edgeList[n].verts[0]].bCollapsable || !m_vertexList[m_edgeList[n].verts[1]].bCollapsable )
        {
          m_edgeList[n].metric = FLT_MAX;
        }
      }
      for(uint32 n=0;n<m_edgeList.size();n++)
      {
        m_edgeList[n].edgeIndex = n;
        if( m_edgeList[n].metric!=FLT_MAX )
        {
          // if edge where one of face has open edge is not allowed
          edges.push_back(&m_edgeList[n]);
        }
      }
      //
      ReBuildMetric(maxAngle,m_processId,bAllowBlendSkin);
      std::sort(edges.begin(),edges.end(),less_cmp);
      for(uint32 n=0;n<edges.size() && edges[0]->metric!=FLT_MAX;n++)
      {
        CollapseEdge(edges[0]->edgeIndex,maxAngle,m_processId,bAllowBlendSkin,m_morphs);
        //ReBuildMetric();
        std::sort(edges.begin(),edges.end(),less_cmp);
      }
    }
    //
    void BuildEdges()
    {
      //      /*
      std::set<Edge> edgeSet;
      for(uint32 f=0;f<m_faceList.size();f++)
      {
        for(int32 n=0;n<3;n++)
        {
          int v0 = m_faceList[f].indicies[(n+0)%3];
          int v1 = m_faceList[f].indicies[(n+1)%3];
          Edge edge;
          if( v0<v1 )
          {
            edge.verts[0] = v0;
            edge.verts[1] = v1;
          }
          else
          {
            edge.verts[0] = v1;
            edge.verts[1] = v0;
          }
          
          for(int fi=0;fi<2;fi++)
          {
					  if( edge.faces[fi]==f )
              break;
            if( edge.faces[fi]==-1 )
            {
              edge.faces[fi] = f;
              break;
            }
          }
					edgeSet.insert(edge);
        }
      }
      //
      //
      m_edgeList.resize(edgeSet.size());
      int e=0;
      for(std::set<Edge>::iterator it = edgeSet.begin();it!=edgeSet.end();it++,e++)
      {
//        if( it->verts[0]==it->verts[1])
//        {
//          int jjj=0;
//        }
        m_edgeList[e] = *it;
      }
    }
    std::vector<Vertex> m_vertexList;
    std::vector<Edge> m_edgeList;
    std::vector<Face> m_faceList;
    int m_numEdgesToUpdateMetric;
    std::vector<int> m_edgeListUpdated;
    std::vector< morphChannel > m_morphs;
    int m_processId;
  };
  //
  CMesh* GenerateLod(CMesh* mesh,float maxAngle,bool bAllowTexSplits,bool bAllowBlendSkin,DynArray<MorphTargetsPtr>* arrMorphTargets)
  {
    std::vector<Subset> subsets(mesh->m_subsets.size());
    int nVerts=0;
    int realFaceCount = 0;
    for(int s=0;s<mesh->m_subsets.size();s++)
    {
      SMeshSubset& subset = mesh->m_subsets[s];
      subset.FixRanges(mesh->m_pIndices);
      subsets[s].Init(mesh,subset,bAllowTexSplits,arrMorphTargets);
      // 
      // build lod here
      subsets[s].Process(DEG2RAD(maxAngle),bAllowBlendSkin);
      //
/*
      int numVertsSubset = 0;
      for(int v=0;v<subsets[s].m_vertexList.size();v++)
      {
        numVertsSubset+=subsets[s].m_vertexList[v].numSplits;
      }
*/
      //
      int numSubsetVBVerts=0;
      for(uint32 f=0;f<subsets[s].m_faceList.size();f++)
      {
        if( subsets[s].m_faceList[f].isRemoved() )
          continue;
        //
        for(int i=0;i<3;i++)
        {
          int v = subsets[s].m_faceList[f].indicies[i];
          int v_split = subsets[s].m_faceList[f].indicies_split[i];
          if( subsets[s].m_vertexList[v].splits[v_split].vb_index==-1 )
          {
            subsets[s].m_vertexList[v].splits[v_split].vb_index = numSubsetVBVerts;
            numSubsetVBVerts++;
          }
        }
        realFaceCount++;
      }
      nVerts+=numSubsetVBVerts;
    }
    CMesh* lodmesh = new CMesh();
    lodmesh->SetVertexCount(nVerts);
    lodmesh->SetTexCoordsCount(nVerts);
    lodmesh->SetIndexCount(realFaceCount*3);
    lodmesh->ReallocStream(CMesh::BONEMAPPING,nVerts);
    lodmesh->ReallocStream(CMesh::SHAPEDEFORMATION,nVerts);
    lodmesh->ReallocStream(CMesh::COLORS_0,nVerts);
    lodmesh->ReallocStream(CMesh::TANGENTS,nVerts);
    int curStartVertex = 0;
    int curStartIndex = 0;
    AABB meshbox(AABB::RESET);
    for(uint32 s=0;s<subsets.size();s++)
    {
      AABB box(AABB::RESET);
      Subset& subset = subsets[s];
      // remove unused verts
      int numVerts = 0;
      //
      for(uint32 v=0;v<subset.m_vertexList.size();v++)
      {
        for(int split=0;split<subset.m_vertexList[v].numSplits;split++)
        {
          int newV = subset.m_vertexList[v].splits[split].vb_index;
          if( newV!=-1 )
          {
            Vec3& pos = subset.m_vertexList[v].p;
            box.Add(pos);
            lodmesh->m_pPositions[newV+curStartVertex] = pos;
            //
            Vec2_tpl<byte> bones[4];
            for(int b=0;b<4;b++)
            {
              bones[b].x = subset.m_vertexList[v].bm.boneIDs[b];
              bones[b].y = subset.m_vertexList[v].bm.weights[b];
            }
            std::sort(&bones[0],&bones[4],Subset::UgreaterWeights);
            subset.m_vertexList[v].bm.boneIDs.Set(0,0,0,0);
            subset.m_vertexList[v].bm.weights.Set(0,0,0,0);
            int ibone = 0;
            for(int b=0;b<4;b++)
            {
              if( bones[b].y!=0 )
              {
                subset.m_vertexList[v].bm.boneIDs[ibone] = bones[b].x;
                subset.m_vertexList[v].bm.weights[ibone] = bones[b].y;
                ibone++;
              }
            }
            //
            lodmesh->m_pBoneMapping[newV+curStartVertex] = subset.m_vertexList[v].bm;
            // split data
            lodmesh->m_pNorms[newV+curStartVertex] = subset.m_vertexList[v].splits[split].normal.GetNormalized();
            lodmesh->m_pTexCoord[newV+curStartVertex].s = subset.m_vertexList[v].splits[split].uv[0];
            lodmesh->m_pTexCoord[newV+curStartVertex].t = subset.m_vertexList[v].splits[split].uv[1];
            lodmesh->m_pTangents[newV+curStartVertex] = subset.m_vertexList[v].splits[split].tangent;
            lodmesh->m_pColor0[newV+curStartVertex] = (SMeshColor&)subset.m_vertexList[v].splits[split].color;
            lodmesh->m_pShapeDeformation[newV+curStartVertex].thin = pos;
            lodmesh->m_pShapeDeformation[newV+curStartVertex].fat = pos;
            lodmesh->m_pShapeDeformation[newV+curStartVertex].index.Set(0,0,0,0);
            numVerts++;
          }
        }
      }
      //
      int face = 0;
      for(uint32 f=0;f<subset.m_faceList.size();f++)
      {
        if( subset.m_faceList[f].isRemoved() )
          continue;
        //
        {
          for(int i=0;i<3;i++)
          {
            int v = subset.m_faceList[f].indicies[i];
            int v_split = subset.m_faceList[f].indicies_split[i];
            int newV = subset.m_vertexList[v].splits[v_split].vb_index;
            lodmesh->m_pIndices[face*3+curStartIndex+i] = newV+curStartVertex;
          }
          face++;
        }
      }
      //
      SMeshSubset subsetOut = mesh->m_subsets[s];// taken from 'in' mesh
      //
      subsetOut.vCenter = box.GetCenter();
      subsetOut.fRadius = box.GetRadius();
      //
      subsetOut.nFirstVertId = curStartVertex;
      subsetOut.nNumVerts = numVerts;
      //
      subsetOut.nFirstIndexId = curStartIndex;
      subsetOut.nNumIndices = face*3;
      //
      lodmesh->m_subsets.push_back(subsetOut);
      //
      curStartVertex+=numVerts;
      curStartIndex+=face*3;
      //
      meshbox.Add(box);
    }
    lodmesh->m_bbox = meshbox;
    //
    return lodmesh;
  }
};

void CMeshLodGenerator::Face::GetEdges(int &e2,int &e3,int &f3,int &f4,int thisFace,int ignoreFace,int ignoreEdge,const Edge* edgeList,int v0,int v1)
{
  for(int n=0;n<3;n++)
  {
    if( ajacency[n]==ignoreFace )
      break;
  }
  // find f2 (edge 
  for(int n=0;n<3;n++)
  {
    int edge = edges[n];
    if( edge!=ignoreEdge && edgeList[edge].HasVertex(v0) )
    {
      e2 = edge;
      //v2 = edgeList[e2].verts[0]==v0 ? edgeList[e2].verts[1] : edgeList[e2].verts[0];
      f3 = edgeList[e2].faces[0]==thisFace ? edgeList[e2].faces[1] : edgeList[e2].faces[0];
    }
    if( edge!=ignoreEdge && edgeList[edge].HasVertex(v1) )
    {
      e3 = edges[n];
      f4 = edgeList[e3].faces[0]==thisFace ? edgeList[e3].faces[1] : edgeList[e3].faces[0];
    }
  }
}

inline float RenormalizeTexCoord(float v,bool* bOutside01=NULL)
{
  if( v<0.0f || v>1.0f )
  {
    if( bOutside01 )
      *bOutside01 = true;
    if( v>1.0f )
    {
      v = v-floorf(v);
    }
    else if( v<0.0f )
    {
      v = 1.0f-(fabsf(v)-floorf(fabsf(v)));
    }
  }
  return v;
}

//-------------------------------------------------------------------------
struct CCMaterialMergePipe : IMaterialMergePipe
{
  IMergePipe* m_pMergePipe;
  CCMaterialMergePipe(IMergePipe* pMergePipe,const std::vector<CContentCGF*>& contents)
  {
    m_pMergePipe = pMergePipe;
    // fill materials for mat pipe
    for(int m=0;m<m_pMergePipe->GetModelsCount();m++)
    {
      IMaterial* pMtl = m_pMergePipe->GetModelMaterial(m);
      if( pMtl==NULL )
      {
        pMtl = gEnv->p3DEngine->GetMaterialManager()->GetDefaultMaterial();
      }
      //
      // this part of mesh is interesting for us
      CSkinningInfo* pSrcSkinningInfo = contents[m]->GetSkinningInfo();
      CNodeCGF* pSrcNode = contents[m]->GetNode(0);
      meshMtlList.push_back(pSrcNode->pMesh);
      chrMtlList.push_back(pMtl);
      //
      if( pMtl->GetSubMtlCount() )
      {
        for(int s=0;s<pMtl->GetSubMtlCount();s++)
        {
          if( pMtl->GetSubMtl(s) )
          {
            mtList.push_back(pMtl->GetSubMtl(s));
          }
        }
      }
      else
      {
        mtList.push_back(pMtl);
      }
    }
  }
  VIRTUAL int GetMaterialsCount() const
  {
    return mtList.size();
  }
  VIRTUAL IMaterial* GetMaterial(int index) const
  {
    return mtList[index];
  }
  VIRTUAL const char* GetTexmapName(int mtlIndex,int mtlSubsetIndex,int texmapIndex)
  {
    return m_pMergePipe->GetMaterialMergeTexmapName(mtlIndex,mtlSubsetIndex,texmapIndex);
  }
  VIRTUAL size_t GetTextureCoordinatesSubsets(IMaterial* mtl)
  {
    int numSubsets = 0;
    for(uint32 c=0;c<chrMtlList.size();c++)
    {
      if( chrMtlList[c]==mtl)
      {
        numSubsets++;
      }
      else
      {
        for(int s=0;s<chrMtlList[c]->GetSubMtlCount();s++)
        {
          if( chrMtlList[c]->GetSubMtl(s)==mtl)
          {
            // find subset c
            for(int subset=0;subset<meshMtlList[c]->GetSubSetCount();subset++)
            {
              if( chrMtlList[c]->GetSubMtl(meshMtlList[c]->m_subsets[subset].nMatID%chrMtlList[c]->GetSubMtlCount())==mtl )
              {
                numSubsets++;
              }
            }
          }
        }
      }
    }
    return numSubsets;
  }
  //
  const uint16* GetIndicies(int subset,IMaterial* mtl,int& stride,int& firstIndex,int& count)
  {
    stride = sizeof(uint16);
    //
    int numSubsets = 0;
    for(uint32 c=0;c<chrMtlList.size();c++)
    {
      if( chrMtlList[c]==mtl)
      {
        if( numSubsets==subset )
        {
          firstIndex = 0;
          count = meshMtlList[c]->GetIndexCount();
          return (const uint16*)meshMtlList[c]->m_pIndices;
        }
        numSubsets++;
      }
      else
      {
        for(int s=0;s<chrMtlList[c]->GetSubMtlCount();s++)
        {
          if( chrMtlList[c]->GetSubMtl(s)==mtl)
          {
            // find subset c
            for(int ss=0;ss<meshMtlList[c]->GetSubSetCount();ss++)
            {
              if( chrMtlList[c]->GetSubMtl(meshMtlList[c]->m_subsets[ss].nMatID%chrMtlList[c]->GetSubMtlCount())==mtl )
              {
                if( numSubsets==subset )
                {
                  meshMtlList[c]->m_subsets[ss].FixRanges(meshMtlList[c]->m_pIndices);
                  count = meshMtlList[c]->m_subsets[ss].nNumIndices;
                  firstIndex = meshMtlList[c]->m_subsets[ss].nFirstIndexId;
                  return (const uint16*)meshMtlList[c]->m_pIndices;
                }
                numSubsets++;
              }
            }
          }
        }
      }
    }
    return NULL;
  }

  const Vec2* GetTextureCoordinates(int subset,IMaterial* mtl,int& stride,int& firstVertex,int& count)
  {
    stride = sizeof(SMeshTexCoord);
    //
    int numSubsets = 0;
    for(uint32 c=0;c<chrMtlList.size();c++)
    {
      if( chrMtlList[c]==mtl)
      {
        if( numSubsets==subset )
        {
          count = meshMtlList[c]->GetVertexCount();
          return (const Vec2*)meshMtlList[c]->m_pTexCoord;
        }
        numSubsets++;
      }
      else
      {
        for(int s=0;s<chrMtlList[c]->GetSubMtlCount();s++)
        {
          if( chrMtlList[c]->GetSubMtl(s)==mtl)
          {
            // find subset c
            for(int ss=0;ss<meshMtlList[c]->GetSubSetCount();ss++)
            {
              if( chrMtlList[c]->GetSubMtl(meshMtlList[c]->m_subsets[ss].nMatID%chrMtlList[c]->GetSubMtlCount())==mtl )
              {
                if( numSubsets==subset )
                {
                  meshMtlList[c]->m_subsets[ss].FixRanges(meshMtlList[c]->m_pIndices);
                  firstVertex = meshMtlList[c]->m_subsets[ss].nFirstVertId;
                  count = meshMtlList[c]->m_subsets[ss].nNumVerts;
                  return (const Vec2*)meshMtlList[c]->m_pTexCoord;
                }
                numSubsets++;
              }
            }
          }
        }
      }
    }
    return NULL;
  }
  std::vector<IMaterial*> mtList;
  std::vector<IMaterial*> chrMtlList;
  std::vector<CMesh*> meshMtlList;
};

bool C3DEngine::MergeCHRs(const char* basechr,IMergePipe* pipe,const char* result_filename,IMergeMaterialsResult** mergedMtlsResult,IMergePipe::EMergeFlags merge_flags)
{
  std::auto_ptr<SMergeMaterialsResult> mergeMtlAuto;
  SMergeMaterialsResult* mergeMtl = NULL;
  if( mergedMtlsResult && *mergedMtlsResult )
  {
    mergeMtl = static_cast<SMergeMaterialsResult*>(*mergedMtlsResult);
  }
  //
  bool bBakeMorphs = (merge_flags & IMergePipe::mfMergeMorphs) ? true : false;
  CChunkFile tgtChunkFile;
  tgtChunkFile.Read( basechr );
  //
  CLoaderCGF cgfTgtLoader;
  CContentCGF *pTgtCGF = cgfTgtLoader.LoadCGF( basechr,tgtChunkFile,NULL );
  if (!pTgtCGF)
  {
    Error( "Failed to load geometry file %s - %s",basechr,cgfTgtLoader.GetLastError() );
    return false;
  }
  // clear chr nodes
  if( pTgtCGF->GetNodeCount()!=1 )
  {
    Error( "More than 1 mesh node in file %s",basechr );
    return false;
  }
  //
  DeleteOldChunks( pTgtCGF,tgtChunkFile, true);
  //
  CSkinningInfo* pTgtSkinningInfo = pTgtCGF->GetSkinningInfo();
  if( merge_flags & IMergePipe::mfSkin )
  {
    // cleanup
    pTgtSkinningInfo->m_arrMorphTargets.resize(0);
    pTgtSkinningInfo->m_arrIntFaces.resize(0);
    pTgtSkinningInfo->m_arrIntVertices.resize(0);
    pTgtSkinningInfo->m_arrExt2IntMap.resize(0);
  }
  //
  CNodeCGF *pDstNodeCGF = pTgtCGF->GetNode(0);
  strcpy_s(pDstNodeCGF->name,pipe->GetModelName());
  CMesh* pTgtMesh = pDstNodeCGF->pMesh;
  //
  if( merge_flags & IMergePipe::mfSkin )
  {
    pTgtMesh->SetVertexCount(0);
    pTgtMesh->SetIndexCount(0);
    pTgtMesh->SetTexCoordsAndTangentsCount(0);
    pTgtMesh->ReallocStream(CMesh::SHAPEDEFORMATION,0);
    pTgtMesh->ReallocStream(CMesh::BONEMAPPING,0);
    pTgtMesh->m_bbox.Reset();
    pTgtMesh->m_subsets.resize(0);
  }
  //
  //
  //
  int numChrs = pipe->GetModelsCount();
  std::vector<CLoaderCGF> cgfs(numChrs);
  std::vector<CContentCGF*> contents(numChrs);
  std::vector<CChunkFile> chunks(numChrs);
  for(int n=0;n<numChrs;n++)
  {
    //
    if( !chunks[n].Read( pipe->GetModelFilename(n) ) )
    {
      Error( "Failed to load file chunks %s",pipe->GetModelFilename(n));
      return false;
    }
    //
    CLoaderCGF cgfLoader2;
    contents[n] = cgfs[n].LoadCGF( pipe->GetModelFilename(n),chunks[n],NULL );
    if (!contents[n])
    {
      Error( "Failed to load cgf date from chunked file%s Error:%s",pipe->GetModelFilename(n),cgfs[n].GetLastError() );
      return false;
    }
    // only one chr node is allowed
    if( contents[n]->GetNodeCount()==0 )
    {
      CryLog("0 count nodes");
      continue;
    }
    if( contents[n]->GetNodeCount()>1 )
    {
      Error( "More than 1 mesh node in file %s",basechr );
      return false;
    }
    CNodeCGF* pSrcNode = contents[n]->GetNode(0);
    if( pSrcNode->type!=CNodeCGF::NODE_MESH )
    {
      Error( "More than 1 mesh node in file %s",basechr );
      return false;
    }
  }
  //
  if( merge_flags & IMergePipe::mfMergeMaterials )
  {
    CCMaterialMergePipe matPipe(pipe,contents);
    //
    //matPipe.chr_filename = PathUtil::GetFileName(result_filename);
    mergeMtl = static_cast<SMergeMaterialsResult*>(gEnv->p3DEngine->MergeMaterials(&matPipe,pipe->GetMaterialMergeFlags()));
    if( mergeMtl )
    {
      if( mergedMtlsResult==NULL )
      {
        std::auto_ptr<SMergeMaterialsResult> tmp(mergeMtl);
        mergeMtlAuto = tmp; 
      }
      std::vector<SMergedSubset>& mergedMtls = static_cast<SMergeMaterialsResult*>(mergeMtl)->mergedMtls;
      // create material 
      string mtlName = PathUtil::ReplaceExtension(result_filename,".mtl");
      IMaterial* pMergedMaterial = gEnv->p3DEngine->GetMaterialManager()->CreateMaterial( mtlName,MTL_FLAG_MULTI_SUBMTL );
      pMergedMaterial->SetSubMtlCount(mergedMtls.size());
      for(uint32 s=0;s<mergedMtls.size();s++)
      {
        mergedMtls[s].mtlMerged->SetName(string().Format("subset%d",s));
        mergedMtls[s].mtlMerged->SetFlags(MTL_FLAG_PURE_CHILD);
        pMergedMaterial->SetSubMtl(s,mergedMtls[s].mtlMerged);
      }
      // save
      XmlNodeRef xeRoot = gEnv->pSystem->CreateXmlNode("Material");
      gEnv->p3DEngine->GetMaterialManager()->SaveMaterial(xeRoot,pMergedMaterial);
      if( xeRoot->saveToFile(mtlName) )
      {
        pMergedMaterial->Release();
        pMergedMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial( mtlName );
        pipe->SetMergedMaterial(pMergedMaterial);
      }
      else
      {
	      CryWarning(VALIDATOR_MODULE_3DENGINE,VALIDATOR_WARNING,"Material save failed: %s.",mtlName.c_str() );
        return false;
      }
    }
    else
    {
      //CryLog("Material save failed:%s",mtlName);
      CryWarning(VALIDATOR_MODULE_3DENGINE,VALIDATOR_WARNING,"Material merge failed.");
      return false;
    }
    if( mergedMtlsResult && *mergedMtlsResult )
    {
      (*mergedMtlsResult)->Release();
    }
    if( mergedMtlsResult )
    {
      *mergedMtlsResult = mergeMtl;
    }
  }
  //
  MergePipeBackend backend(&contents,pipe);
  //
  // Merge skinning info first
  //
  int sum_numVertsBefore = 0;
  int sum_numFacesBefore = 0;
  int sum_numVertsAfter = 0;
  int sum_numFacesAfter = 0;
  std::vector<int> skinStartIntVertices(numChrs,0);
  std::vector<int> skinStartInt2ExtVertices(numChrs,0);
  std::vector<std::map<uint16,uint16> > boneMaps(numChrs);;
  for(int c=0;c<numChrs;c++)
  {
    // this part of mesh is interesting for us
    CSkinningInfo* pSrcSkinningInfo = contents[c]->GetSkinningInfo();
    CNodeCGF* pSrcNode = contents[c]->GetNode(0);
    //
    if( merge_flags & IMergePipe::mfSkin )
    {
      //
      // bake morphs
      //
      std::vector<std::pair<string,float> > morphs;
      uint32 numMorphtargets = pSrcSkinningInfo->m_arrMorphTargets.size();
      for(uint32 i=0; i<numMorphtargets; i++) 
      {
        const char* morphName = pSrcSkinningInfo->m_arrMorphTargets[i]->m_strName.c_str();
        float weight = pipe->GetMorphTargetWeight(c,morphName);
        morphs.push_back(std::pair<string,float>(morphName,weight));
      }
			//
      ApplyMorphs(pSrcSkinningInfo,contents[c],morphs,false,NULL);
      //
      //
      float maxAngle = pipe->GetModelLodMaxAngle(c);
      if( fabsf(maxAngle)>0.001f )
      {
        if( pSrcNode->type!=CNodeCGF::NODE_MESH )
        {
          continue;
        }
        CMesh* pMesh = pSrcNode->pMesh;
        //
        //
        CMeshLodGenerator lodGen;
        IMergePipe::ELodFlags lod_flags = pipe->GetModelLodFlags(c);
        int numVertsBefore = pMesh->GetVertexCount();
        int numFacesBefore = pMesh->GetIndexCount()/3;
        //
        sum_numVertsBefore+=numVertsBefore;
        sum_numFacesBefore+=numFacesBefore;
        //
        CMesh* lodmesh = lodGen.GenerateLod(pMesh,maxAngle,(lod_flags & IMergePipe::elfHandleSplits) ? true : false,(lod_flags & IMergePipe::elfHandleSkinning) ? true : false, bBakeMorphs ? &pSrcSkinningInfo->m_arrMorphTargets : NULL  );
        int numVertsAfter = lodmesh->GetVertexCount();
        int numFacesAfter = lodmesh->GetIndexCount()/3;
        //
        sum_numVertsAfter+=numVertsAfter;
        sum_numFacesAfter+=numFacesAfter;
        //
        if( (merge_flags & IMergePipe::mfVerbose_LOD) )
        {
          CryLog("LODGEN: %10s: ORG( v: %5d f: %5d ) LOD( v: %5d f: %5d ) REL( v: %5.2f%% f: %5.2f%%)",pipe->GetModelLogPrefix(c),numVertsBefore,numFacesBefore,numVertsAfter,numFacesAfter,(float)numVertsAfter/(float)numVertsBefore*100.0f,(float)numFacesAfter/(float)numFacesBefore*100.0f);
        }
        //
        //
        SAFE_DELETE(pSrcNode->pMesh);
        pSrcNode->pMesh = lodmesh;
        //
        pSrcSkinningInfo->m_arrExt2IntMap.resize(lodmesh->GetVertexCount());
        pSrcSkinningInfo->m_arrIntVertices.resize(lodmesh->GetVertexCount());
        for(int n=0;n<lodmesh->GetVertexCount();n++)
        {
          pSrcSkinningInfo->m_arrExt2IntMap[n] = n;
          pSrcSkinningInfo->m_arrIntVertices[n].wpos0 = lodmesh->m_pPositions[n];
          pSrcSkinningInfo->m_arrIntVertices[n].wpos1 = lodmesh->m_pPositions[n];
          pSrcSkinningInfo->m_arrIntVertices[n].wpos2 = lodmesh->m_pPositions[n];
          if( lodmesh->m_pColor0 )
            pSrcSkinningInfo->m_arrIntVertices[n].color = ColorB(lodmesh->m_pColor0[n].r,lodmesh->m_pColor0[n].g,lodmesh->m_pColor0[n].b,lodmesh->m_pColor0[n].a);
          else
            pSrcSkinningInfo->m_arrIntVertices[n].color.Set(255,255,255,0);
        }
        pSrcSkinningInfo->m_arrIntFaces.resize(lodmesh->GetIndexCount()/3);
        for(int n=0;n<lodmesh->GetIndexCount()/3;n++)
        {
          pSrcSkinningInfo->m_arrIntFaces[n].i0 = lodmesh->m_pIndices[n*3+0];
          pSrcSkinningInfo->m_arrIntFaces[n].i1 = lodmesh->m_pIndices[n*3+1];
          pSrcSkinningInfo->m_arrIntFaces[n].i2 = lodmesh->m_pIndices[n*3+2];
        }
        for(int s=0;s<lodmesh->m_subsets.size();s++)
        {
          SMeshSubset& subset = lodmesh->m_subsets[s];
          for(int n=0;n<subset.nNumVerts;n++)
          {
            for(int i=0;i<4;i++)
            {
              uint8 boneId = lodmesh->m_pBoneMapping[n+subset.nFirstVertId].boneIDs[i];
              uint16 globalBoneId = subset.m_arrGlobalBonesPerSubset[boneId];
              pSrcSkinningInfo->m_arrIntVertices[n+subset.nFirstVertId].boneIDs[i] = globalBoneId;
              pSrcSkinningInfo->m_arrIntVertices[n+subset.nFirstVertId].weights[i] = (float)lodmesh->m_pBoneMapping[n+subset.nFirstVertId].weights[i]/(float)255.0f;
            }
          }
        }
      }
      //
      int tgtExt2IntMapSize = pTgtSkinningInfo->m_arrExt2IntMap.size();
      int maxVertex = pSrcSkinningInfo->m_arrExt2IntMap.size();
      //
      int skin1BonesCount = pTgtSkinningInfo->m_arrBonesDesc.size();
      int skin2BonesCount = pSrcSkinningInfo->m_arrBonesDesc.size();
      //
      for(int n1=0;n1<skin1BonesCount;n1++)
      {
        unsigned int bone1 = pTgtSkinningInfo->m_arrBonesDesc[n1].m_nControllerID;// pTgtCGF->GetNode(n1)->name;
        for(int n2=0;n2<skin2BonesCount;n2++)
        {
          unsigned int bone2 = pSrcSkinningInfo->m_arrBonesDesc[n2].m_nControllerID;
          if( bone1==bone2 )
          {
            //boneMap[n2] = n1;
            boneMaps[c][n2] = n1;
          }
        }
      }
      //
      int skinTgtIntVertexCount = pTgtSkinningInfo->m_arrIntVertices.size();
      int skinSrcIntVertexCount = pSrcSkinningInfo->m_arrIntVertices.size();
      if( skinTgtIntVertexCount+skinSrcIntVertexCount>=(1 << 16) )
      {
        CryWarning(VALIDATOR_MODULE_3DENGINE,VALIDATOR_WARNING,"Too many total vertices to merge.");
        return false;
      }
      //
      skinStartIntVertices[c] = skinTgtIntVertexCount;
      // internal verticies
      pTgtSkinningInfo->m_arrIntVertices.resize(skinTgtIntVertexCount+skinSrcIntVertexCount);
      for(int n=0;n<skinSrcIntVertexCount;n++)
      {
        // copy from src
        int tgtIndex = skinTgtIntVertexCount+n;
        pTgtSkinningInfo->m_arrIntVertices[tgtIndex] = pSrcSkinningInfo->m_arrIntVertices[n];
        // remap bones
        for(int i=0;i<4;i++)
        {
          if( pSrcSkinningInfo->m_arrIntVertices[n].weights[i]>0.0f )
          {
            uint16 oldBoneIndex = pSrcSkinningInfo->m_arrIntVertices[n].boneIDs[i];
            pTgtSkinningInfo->m_arrIntVertices[tgtIndex].boneIDs[i] = boneMaps[c][oldBoneIndex];
          }
        }
      }
      // ext map - as i understand it is a vertex indicies for rendering
      skinStartInt2ExtVertices[c] = tgtExt2IntMapSize;
      int srcExt2IntMapSize = maxVertex;
      pTgtSkinningInfo->m_arrExt2IntMap.resize(tgtExt2IntMapSize+srcExt2IntMapSize);
      //
      for(int n=0;n<srcExt2IntMapSize;n++)
      {
        // copy from 2
        pTgtSkinningInfo->m_arrExt2IntMap[tgtExt2IntMapSize+n] = pSrcSkinningInfo->m_arrExt2IntMap[n];
      }
      // add faces
      int tgtFaceSize = pTgtSkinningInfo->m_arrIntFaces.size();
      pTgtSkinningInfo->m_arrIntFaces.resize(tgtFaceSize+pSrcSkinningInfo->m_arrIntFaces.size());
      for(int n=0;n<pSrcSkinningInfo->m_arrIntFaces.size();n++)
      {
        // copy from 2
        pTgtSkinningInfo->m_arrIntFaces[tgtFaceSize+n] = pSrcSkinningInfo->m_arrIntFaces[n];
        pTgtSkinningInfo->m_arrIntFaces[tgtFaceSize+n].i0 += skinTgtIntVertexCount;
        pTgtSkinningInfo->m_arrIntFaces[tgtFaceSize+n].i1 += skinTgtIntVertexCount;
        pTgtSkinningInfo->m_arrIntFaces[tgtFaceSize+n].i2 += skinTgtIntVertexCount;
      }
    }
    //
    if( merge_flags & IMergePipe::mfPhysics)
    {
      int skinSrcBonesCount = pSrcSkinningInfo->m_arrBoneEntities.size();
      //
      for(int nSrc=0;nSrc<skinSrcBonesCount;nSrc++)
      {
        int boneDescIndex = -1;
        const char* name = NULL;
        for(int t=0;t<pSrcSkinningInfo->m_arrBonesDesc.size();t++)
        {
          if( pSrcSkinningInfo->m_arrBonesDesc[t].m_nControllerID==pSrcSkinningInfo->m_arrBoneEntities[nSrc].ControllerID )
          {
            name = pSrcSkinningInfo->m_arrBonesDesc[t].m_arrBoneName;
            boneDescIndex = t;
            break;
          }
        }
        if( name )
        {
          IMergePipe::SMergePhysicsBoneResult res = pipe->GetPhysicsBoneMergeInfo(c,pSrcNode->name,name,&backend);
          if( res.okToMerge )
          {
            // step 1: find this bone in target
            bool bFound = false;
            int skinTgtBonesCount = pTgtSkinningInfo->m_arrBoneEntities.size();
            for(int nTgt=0;nTgt<skinTgtBonesCount;nTgt++)
            {
              if( pTgtSkinningInfo->m_arrBoneEntities[nTgt].ControllerID==pSrcSkinningInfo->m_arrBoneEntities[nSrc].ControllerID )
              {
                // copy prop data
                memcpy(pTgtSkinningInfo->m_arrBoneEntities[nTgt].prop,pSrcSkinningInfo->m_arrBoneEntities[nSrc].prop,sizeof(pSrcSkinningInfo->m_arrBoneEntities[nSrc].prop));
                // copy update bone mesh
                int srcIdx = pSrcSkinningInfo->m_arrBoneEntities[nSrc].phys.nPhysGeom;
                int tgtIdx = pTgtSkinningInfo->m_arrBoneEntities[nTgt].phys.nPhysGeom;
                if( tgtIdx>=0 )
                {
                  pTgtSkinningInfo->m_arrPhyBoneMeshes[tgtIdx].m_arrIndices = pSrcSkinningInfo->m_arrPhyBoneMeshes[srcIdx].m_arrIndices;
                  pTgtSkinningInfo->m_arrPhyBoneMeshes[tgtIdx].m_arrMaterials = pSrcSkinningInfo->m_arrPhyBoneMeshes[srcIdx].m_arrMaterials;
                  pTgtSkinningInfo->m_arrPhyBoneMeshes[tgtIdx].m_arrPoints = pSrcSkinningInfo->m_arrPhyBoneMeshes[srcIdx].m_arrPoints;
                }
                // update matricies
                for(int t=0;t<pTgtSkinningInfo->m_arrBonesDesc.size();t++)
                {
                  for(int s=0;s<pSrcSkinningInfo->m_arrBonesDesc.size();s++)
                  {
                    if( pTgtSkinningInfo->m_arrBonesDesc[t].m_nControllerID==pSrcSkinningInfo->m_arrBonesDesc[s].m_nControllerID )
                    {
                      if( res.bOverrideB2W )
                      {
                        pTgtSkinningInfo->m_arrBonesDesc[t].m_DefaultB2W = res.m_B2W;
                        pTgtSkinningInfo->m_arrBonesDesc[t].m_DefaultW2B = res.m_B2W.GetInverted();
                      }
                      else
                      {
                        pTgtSkinningInfo->m_arrBonesDesc[t].m_DefaultB2W = pSrcSkinningInfo->m_arrBonesDesc[s].m_DefaultB2W;
                        pTgtSkinningInfo->m_arrBonesDesc[t].m_DefaultW2B = pSrcSkinningInfo->m_arrBonesDesc[s].m_DefaultW2B;
                      }
                      break;
                    }
                  }
                }
                bFound = true;
              }
            }
            if(!bFound)
            {
              // anim bones
              int srcParentID = pSrcSkinningInfo->m_arrBoneEntities[nSrc].ParentID;
              int srcParentCtrlID = pSrcSkinningInfo->m_arrBoneEntities[srcParentID].ControllerID;
              int parentTgtID = -1;
              for(int t=0;t<pTgtSkinningInfo->m_arrBonesDesc.size();t++)
              {
                if( pTgtSkinningInfo->m_arrBonesDesc[t].m_nControllerID==srcParentCtrlID )
                {
                  parentTgtID = t;
                  break;
                }
              }
              CryBoneDescData* srcBoneDesc = NULL;
              for(int t=0;t<pSrcSkinningInfo->m_arrBonesDesc.size();t++)
              {
                if( pSrcSkinningInfo->m_arrBonesDesc[t].m_nControllerID==pSrcSkinningInfo->m_arrBoneEntities[nSrc].ControllerID )
                {
                  srcBoneDesc = &pSrcSkinningInfo->m_arrBonesDesc[t];
                  break;
                }
              }
              // find next free chunkid
              uint32 physMeshBoneId = 0;
              for(int t=0;t<pTgtSkinningInfo->m_arrPhyBoneMeshes.size();t++)
              {
                physMeshBoneId = max(physMeshBoneId,pTgtSkinningInfo->m_arrPhyBoneMeshes[t].ChunkID);
              }
              physMeshBoneId++;
              //
              CryBoneDescData boneData;
              strcpy(boneData.m_arrBoneName,res.new_name);
              boneData.m_nControllerID = gEnv->pSystem->GetCrc32Gen()->GetCRC32(res.new_name);
              boneData.m_PhysInfo[0] = srcBoneDesc->m_PhysInfo[0];
              if( (int)(boneData.m_PhysInfo[0].pPhysGeom)!=-1 )
              {
                boneData.m_PhysInfo[0].pPhysGeom = (phys_geometry*)physMeshBoneId;// later i will add geom
              }
              boneData.m_PhysInfo[1] = srcBoneDesc->m_PhysInfo[1];
              boneData.m_fMass = srcBoneDesc->m_fMass;
              //
              CryBoneDescData* pSrcParentNode = &pSrcSkinningInfo->m_arrBonesDesc[boneDescIndex+pSrcSkinningInfo->m_arrBonesDesc[boneDescIndex].m_nOffsetParent];
              CryBoneDescData* pTgtParentNode = &pTgtSkinningInfo->m_arrBonesDesc[parentTgtID];

              QuatT srcParentTM(pSrcParentNode->m_DefaultB2W);
              QuatT srcNodeTM(srcBoneDesc->m_DefaultB2W);
              QuatT srcRelTM = srcParentTM.GetInverted() * srcNodeTM;
              Matrix34 relSrcRelMatrix1(srcRelTM);

              QuatT tgtParentTM(pTgtParentNode->m_DefaultB2W);
              QuatT tgtNodeTM = tgtParentTM*srcRelTM;

              if( res.bOverrideB2W )
              {
                boneData.m_DefaultB2W = res.m_B2W;
                boneData.m_DefaultW2B = res.m_B2W.GetInverted();
              }
              else
              {
                boneData.m_DefaultB2W = Matrix34(tgtNodeTM);
                boneData.m_DefaultW2B = boneData.m_DefaultB2W.GetInverted();
              }
              //
              boneData.m_nLimbId = srcBoneDesc->m_nLimbId;// ???
              boneData.m_numChildren = 0;// ???
              boneData.m_nOffsetChildren = 0;
              int parentOffsetChildren = pTgtSkinningInfo->m_arrBonesDesc[parentTgtID].m_nOffsetChildren;
              int parentNumChildren = pTgtSkinningInfo->m_arrBonesDesc[parentTgtID].m_numChildren;
              int indexToInsert = parentTgtID+parentOffsetChildren+parentNumChildren;
              boneData.m_nOffsetParent = parentTgtID-indexToInsert;// ???
              pTgtSkinningInfo->m_arrBonesDesc.insert(pTgtSkinningInfo->m_arrBonesDesc.begin()+indexToInsert,boneData);
              pTgtSkinningInfo->m_arrBonesDesc[parentTgtID].m_numChildren++;
              //
              // update indexes in anim bones
              for(int t=0;t<pTgtSkinningInfo->m_arrBonesDesc.size();t++)
              {
                if( t==indexToInsert )
                  continue;
                //
                if(pTgtSkinningInfo->m_arrBonesDesc[t].m_numChildren!=0 )
                {
                  // offset children
                  if( t<indexToInsert && t+pTgtSkinningInfo->m_arrBonesDesc[t].m_nOffsetChildren>=indexToInsert )
                  {
                    pTgtSkinningInfo->m_arrBonesDesc[t].m_nOffsetChildren++;
                  }
                }
                // offset parent
                if( t>indexToInsert )
                {
                  if( t+pTgtSkinningInfo->m_arrBonesDesc[t].m_nOffsetParent<=indexToInsert )
                  {
                    pTgtSkinningInfo->m_arrBonesDesc[t].m_nOffsetParent--;
                  }
                }
              }
              // add new physical bone
              pTgtSkinningInfo->m_arrBoneEntities.push_back(pSrcSkinningInfo->m_arrBoneEntities[nSrc]);
              //
              // update bone proxy mesh
              //
              //
              pTgtSkinningInfo->m_arrBoneEntities.back().phys.nPhysGeom = -1;
              //
              int srcPhysID = pSrcSkinningInfo->m_arrBoneEntities[nSrc].phys.nPhysGeom;
              if( srcPhysID!=-1 )
              {
                int srcPhysIndex = -1;
                // find src proxy chunk index
                for(int t=0;t<pSrcSkinningInfo->m_arrPhyBoneMeshes.size();t++)
                {
                  if( pSrcSkinningInfo->m_arrPhyBoneMeshes[t].ChunkID==srcPhysID )
                  {
                    srcPhysIndex = t;
                    break;
                  }
                }
                if( srcPhysIndex==-1 )
                {
                  // it is helper
                  //pTgtSkinningInfo->m_arrPhyBoneMeshes.push_back(pSrcSkinningInfo->m_arrPhyBoneMeshes[srcPhysIndex]);
                  pTgtSkinningInfo->m_arrPhyBoneMeshes.back().ChunkID = ~0;
                  pTgtSkinningInfo->m_arrBoneEntities.back().phys.nPhysGeom = -1;
                  //CryLog("ProxyMesh with chunkid:%d not found",srcPhysID);
                  //return false;
                }
                else
                {
                  pTgtSkinningInfo->m_arrPhyBoneMeshes.push_back(pSrcSkinningInfo->m_arrPhyBoneMeshes[srcPhysIndex]);
                  pTgtSkinningInfo->m_arrPhyBoneMeshes.back().ChunkID = physMeshBoneId;
                  pTgtSkinningInfo->m_arrBoneEntities.back().phys.nPhysGeom = physMeshBoneId;
                }
              }
              //
              BONE_ENTITY& boneEnt = pTgtSkinningInfo->m_arrBoneEntities.back();
              // update controller
              boneEnt.ControllerID = boneData.m_nControllerID;
              // bone id
              boneEnt.BoneID = pTgtSkinningInfo->m_arrBoneEntities.size()-1;
              boneEnt.nChildren = 0;
              int parentIDtoSearch = boneEnt.ParentID;
              boneEnt.ParentID = -1;
              // update parent
              for(int b=0;b<pSrcSkinningInfo->m_arrBoneEntities.size();b++)
              {
                if( pSrcSkinningInfo->m_arrBoneEntities[b].BoneID==parentIDtoSearch )
                {
                  int foundIndex=-1;
                  int parentCtrlID = pSrcSkinningInfo->m_arrBoneEntities[b].ControllerID;
                  // find in target mesh
                  for(int nTgt=0;nTgt<pTgtSkinningInfo->m_arrBoneEntities.size()-1;nTgt++)// except last(current)
                  {
                    if( pTgtSkinningInfo->m_arrBoneEntities[nTgt].ControllerID==parentCtrlID )
                    {
                      foundIndex = pTgtSkinningInfo->m_arrBoneEntities[nTgt].BoneID;
                      break;
                    }
                  }
                  if( foundIndex!=-1 )
                  {
                    boneEnt.ParentID = foundIndex;
                    pTgtSkinningInfo->m_arrBoneEntities[foundIndex].nChildren++;
                  }
                  else
                  {
                    CryLog("Parent ctrl id:%08X not found",parentCtrlID);
                    return false;
                  }
                  break;
                }
              }
              // update boneid for skin verticies
              for(int v=0;v<pTgtSkinningInfo->m_arrIntVertices.size();v++)
              {
                for(int w=0;w<4;w++)
                {
                  if( pTgtSkinningInfo->m_arrIntVertices[v].boneIDs[w]>=indexToInsert )
                  {
                    pTgtSkinningInfo->m_arrIntVertices[v].boneIDs[w]++;
                  }
                }
              }
              // updates bones for subsets
              for(int n=0;n<pTgtCGF->GetNodeCount();n++)
              {
                CNodeCGF* pNode = pTgtCGF->GetNode(n);
                if( pNode->type == CNodeCGF::NODE_MESH )
                {
                  CMesh* pMesh = pNode->pMesh;
                  for(int s=0;s<pMesh->m_subsets.size();s++)
                  {
                    for(uint32 b=0;b<pMesh->m_subsets[s].m_arrGlobalBonesPerSubset.size();b++)
                    {
                      if( pMesh->m_subsets[s].m_arrGlobalBonesPerSubset[b]>=indexToInsert )
                      {
                        pMesh->m_subsets[s].m_arrGlobalBonesPerSubset[b]++;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  //
  if( merge_flags & IMergePipe::mfSkin )
  {
    //
    bool bColor0 = false;
    for(int i=0;i<numChrs;i++)
    {
      CNodeCGF* pSrcNode = contents[i]->GetNode(0);
      CMesh* pSrcMesh = pSrcNode->pMesh;
      if( pSrcMesh->m_pColor0!=NULL )
      {
        bColor0 = true;
      }
/*
      if( bColor0 && pSrcMesh->m_pColor0==NULL )
      {
        int ee = 0;
      }
*/
    }
    if( pTgtMesh->m_pColor0==NULL && bColor0 )
    {
      // workaround
      pTgtMesh->ReallocStream(CMesh::COLORS_0,1);
      pTgtMesh->ReallocStream(CMesh::COLORS_0,0);
    }
    //
    int tgtMtlID = 0;
    // move data to result, arrange by materials
    for(uint32 ss=0;ss<mergeMtl->mergedMtls.size();ss++,tgtMtlID++)
    {
      const SMergedSubset& merged_subset = mergeMtl->mergedMtls[ss];
      // add all geometries that uses original materials
      for(uint32 m=0;m<mergeMtl->mergedMtls[ss].originalMaterials.size();m++)
      {
        const SMergedMtlInfo& mtlInfo = mergeMtl->mergedMtls[ss].originalMaterials[m];
        IMaterial* orgMtl = mergeMtl->mergedMtls[ss].originalMaterials[m].mtlOrg;
        for(int i=0;i<numChrs;i++)
        {
          IMaterial* inMtl = pipe->GetModelMaterial(i);
          int mtlId = -1;
          int mtlCount = -1;
          bool bChrHasMtl = false;
          if( inMtl==orgMtl )
          {
            bChrHasMtl = true;
            mtlId=0;
            mtlCount=1;
          }
          else
          {
            // check submaterials
            if( inMtl->GetSubMtlCount() )
            {
              for(int sm=0;sm<inMtl->GetSubMtlCount();sm++)
              {
                if( inMtl->GetSubMtl(sm)==orgMtl )
                {
                  bChrHasMtl = true;
                  mtlId = sm;
                  mtlCount = inMtl->GetSubMtlCount();
                  break;
                }
              }
            }
          }
          //
          if( !bChrHasMtl)
            continue;
          CNodeCGF* pSrcNode = contents[i]->GetNode(0);
          //
          CMesh* pSrcMesh = pSrcNode->pMesh;
          pTgtMesh->m_bbox.Add(pSrcMesh->m_bbox);
          // find required material subset
          for(int rs=0;rs<pSrcMesh->m_subsets.size();rs++)
          {
            SMeshSubset& srcSubset = pSrcMesh->m_subsets[rs];
            if( (srcSubset.nMatID%mtlCount)==mtlId )
            {
              // this part of mesh is interesting for us
              CSkinningInfo* pSrcSkinningInfo = contents[i]->GetSkinningInfo();
              //
              srcSubset.FixRanges(pSrcMesh->m_pIndices);
              //
              int startVertexToMerge = srcSubset.nFirstVertId;
              int numVerticesToMerge = srcSubset.nNumVerts;
              int startIndexToMerge = srcSubset.nFirstIndexId;
              int numIndiciesToMerge = srcSubset.nNumIndices;
              //
              int numIndicied1 = pTgtMesh->GetIndexCount();
              int numVertices1 = pTgtMesh->GetVertexCount();
              int numFaces1 = pTgtMesh->GetFacesCount();
              // create color0 if not exists
              pTgtMesh->Append(*pSrcMesh,startVertexToMerge,numVerticesToMerge,startIndexToMerge/3,numIndiciesToMerge/3);
              if( bBakeMorphs )
              {
                // bake src morph to target morphs
                for(int ns=0;ns<pSrcSkinningInfo->m_arrMorphTargets.size();ns++)
                {
                  MorphTargets* pSrcTarget = pSrcSkinningInfo->m_arrMorphTargets[ns];
                  MorphTargets* pTgtTarget = NULL;
                  // find morphs in targed or create it
                  for(int ds=0;ds<pTgtSkinningInfo->m_arrMorphTargets.size();ds++)
                  {
                    if( pTgtSkinningInfo->m_arrMorphTargets[ds]->m_strName==pSrcTarget->m_strName )
                    {
                      pTgtTarget = pTgtSkinningInfo->m_arrMorphTargets[ds];
                      break;
                    }
                  }
                  if( !pTgtTarget )
                  {
                    pTgtSkinningInfo->m_arrMorphTargets.push_back(new MorphTargets());
                    pTgtTarget = pTgtSkinningInfo->m_arrMorphTargets.back();
                    pTgtTarget->MeshID = 0;
                    pTgtTarget->m_strName = pSrcTarget->m_strName;
                  }
                  // now 
                  for(int v=0;v<pSrcTarget->m_arrExtMorph.size();v++)
                  {
                    if( (int)pSrcTarget->m_arrExtMorph[v].nVertexId>=startVertexToMerge &&
                        (int)pSrcTarget->m_arrExtMorph[v].nVertexId<=startVertexToMerge+numVerticesToMerge)
                    {
                      SMeshMorphTargetVertex morph_vertex;
                      morph_vertex.nVertexId = (pSrcTarget->m_arrExtMorph[v].nVertexId-startVertexToMerge)+numVertices1;
                      morph_vertex.ptVertex = pSrcTarget->m_arrExtMorph[v].ptVertex;
                      pTgtTarget->m_arrIntMorph.push_back(morph_vertex);
                      pTgtTarget->m_arrExtMorph.push_back(morph_vertex);
                    }
                  }
                }
              }
              // color0
              if( bColor0 && pSrcMesh->m_pColor0==NULL )
              {
                // not filled at all
                pTgtMesh->ReallocStream(CMesh::COLORS_0,numVertices1+numVerticesToMerge);
                for(int n=0;n<numVerticesToMerge;n++)
                {
                  pTgtMesh->m_pColor0[numVertices1+n].r = 255;
                  pTgtMesh->m_pColor0[numVertices1+n].g = 255;
                  pTgtMesh->m_pColor0[numVertices1+n].b = 255;
                  pTgtMesh->m_pColor0[numVertices1+n].a = 0;
                }
              }
/*
              // update m_arrExt2IntMap for this subset
              int baseIndex = skinStartInt2ExtVertices[i];
              for(int n=0;n<numVerticesToMerge;n++)
              {
                pTgtSkinningInfo->m_arrExt2IntMap[baseIndex+n+startVertexToMerge] += skinStartIntVertices[i];
                //if( pTgtSkinningInfo->m_arrExt2IntMap[baseIndex+n+startVertexToMerge]>=pTgtSkinningInfo->m_arrIntVertices.size() )
                //{
                //  int hh=0;
                //}
              }
*/
              //
              // update texture coordinates
              //
              SMeshTexCoord* pTexCoord;
              int nTexCoordElemSize;
              pTgtMesh->GetStreamInfo(CMesh::TEXCOORDS,(void*&)pTexCoord,nTexCoordElemSize);
              // find first tex occurency
              bool bOutside01 = false;
              for(int t=0;t<EFTT_MAX;t++)
              {
                if( mtlInfo.dds_filename[t].size()!=0 )
                {
                    //
                    for(int v=numVertices1;v<pTgtMesh->GetTexCoordsCount();v++)
                    {
                      //
                      float tu = RenormalizeTexCoord(pTexCoord[v].s,&bOutside01);
                      float tv = RenormalizeTexCoord(pTexCoord[v].t,&bOutside01);
                      //
                      for(uint32 s=0;s<mtlInfo.src_rects.size();s++)
                      {
                        if( tu>=mtlInfo.src_rects[s].min.x && tu<=mtlInfo.src_rects[s].max.x && 
                          tv>=mtlInfo.src_rects[s].min.y && tv<=mtlInfo.src_rects[s].max.y )
                        {
                          //
                          //float offsetx = (float)mtlInfo.src_x[t][s]/(float)mtlInfo.src_w[t];
                          //float offsety = (float)mtlInfo.src_y[t][s]/(float)mtlInfo.src_h[t];
                          float offsetx = mtlInfo.uv_min_x[t][s];
                          float offsety = mtlInfo.uv_min_y[t][s];
                          //
                          float dx = (float)mtlInfo.dst_x[t][s]/(float)merged_subset.tex_w[t];
                          float dy = (float)mtlInfo.dst_y[t][s]/(float)merged_subset.tex_h[t];
                          float dw = (float)mtlInfo.w[t][s]/(float)merged_subset.tex_w[t];
                          float dh = (float)mtlInfo.h[t][s]/(float)merged_subset.tex_h[t];
                          //
                          float sw = (float)mtlInfo.w[t][s]/(float)mtlInfo.src_w[t];
                          float sh = (float)mtlInfo.h[t][s]/(float)mtlInfo.src_h[t];
                          //
                          float sx = dw/sw;
                          float sy = dh/sh;
                          //
                          tu = (tu-offsetx)*sx+dx;
                          tv = (tv-offsety)*sy+dy;
                          //tu-=offsetx;
                          //tv-=offsety;
                          pTexCoord[v].s = tu;
                          pTexCoord[v].t = tv;
                          break;
                        }
                    }
                  }
                    //
                    if( bOutside01 )
                    {
                      CryWarning(VALIDATOR_MODULE_3DENGINE,VALIDATOR_WARNING,"Texture coordinates for %s model in outside [0..1]",pipe->GetModelFilename(i));
                    }
                  //
                  break;
                }
              }
              // ??? looks like not used
              if( pSrcNode->mapFaceToFace0.size() )
              {
                for(int n=0;n<numIndiciesToMerge/3;n++)
                {
                  pDstNodeCGF->mapFaceToFace0.push_back(pSrcNode->mapFaceToFace0[n+startIndexToMerge/3]+numFaces1);
                }
              }
              // update subset
              if( pTgtMesh->m_subsets.size()==0 || pTgtMesh->m_subsets.back().nMatID!=tgtMtlID )
              {
                pTgtMesh->m_subsets.push_back(SMeshSubset());
                pTgtMesh->m_subsets.back().nFirstIndexId = 0;
                pTgtMesh->m_subsets.back().nFirstVertId = 0;
                if( pTgtMesh->m_subsets.back().nMatID!=tgtMtlID )
                {
                  pTgtMesh->m_subsets.back().nFirstIndexId = numIndicied1;
                  pTgtMesh->m_subsets.back().nFirstVertId = numVertices1;
                }
                pTgtMesh->m_subsets.back().nNumIndices = 0;
                pTgtMesh->m_subsets.back().nNumVerts = 0;
                pTgtMesh->m_subsets.back().nMatID = tgtMtlID;
                pTgtMesh->m_subsets.back().nMatFlags = srcSubset.nMatFlags;
                pTgtMesh->m_subsets.back().nPhysicalizeType = srcSubset.nPhysicalizeType;
              }
              // bone map update
              PodArray<uint16> TgtArrGlobalBonesPerSubset;
              if( !recalcSkinning(pTgtMesh,numVertices1,pTgtMesh->GetVertexCount(),srcSubset,TgtArrGlobalBonesPerSubset,boneMaps[i],false) )
              {
                CryLog( "Base chr file does not contain bones for attached parts Base: %s, Part:%s",basechr,pipe->GetModelFilename(i) );
                return false;
              }
              // 
              if( (int)TgtArrGlobalBonesPerSubset.size()>=NUM_MAX_BONES_PER_GROUP )
              {
                // add to new subset
                pTgtMesh->m_subsets.push_back(SMeshSubset());
                pTgtMesh->m_subsets.back().nFirstIndexId = 0;
                pTgtMesh->m_subsets.back().nFirstVertId = 0;
                if( pTgtMesh->m_subsets.back().nMatID!=tgtMtlID )
                {
                  pTgtMesh->m_subsets.back().nFirstIndexId = numIndicied1;
                  pTgtMesh->m_subsets.back().nFirstVertId = numVertices1;
                }
                pTgtMesh->m_subsets.back().nNumIndices = 0;
                pTgtMesh->m_subsets.back().nNumVerts = 0;
                pTgtMesh->m_subsets.back().nMatID = tgtMtlID;
                pTgtMesh->m_subsets.back().nMatFlags = srcSubset.nMatFlags;
                pTgtMesh->m_subsets.back().nPhysicalizeType = srcSubset.nPhysicalizeType;
                recalcSkinning(pTgtMesh,numVertices1,pTgtMesh->GetVertexCount(),srcSubset,TgtArrGlobalBonesPerSubset,boneMaps[i],true);
                pTgtMesh->m_subsets.back().m_arrGlobalBonesPerSubset = srcSubset.m_arrGlobalBonesPerSubset;
              }
              else
              {
                recalcSkinning(pTgtMesh,numVertices1,pTgtMesh->GetVertexCount(),srcSubset,TgtArrGlobalBonesPerSubset,boneMaps[i],true);
                pTgtMesh->m_subsets.back().m_arrGlobalBonesPerSubset = TgtArrGlobalBonesPerSubset;
              }
              pTgtMesh->m_subsets.back().nNumIndices += numIndiciesToMerge;
              pTgtMesh->m_subsets.back().nNumVerts += numVerticesToMerge;
              pTgtMesh->m_subsets.back().vCenter = pTgtMesh->m_bbox.GetCenter();
              pTgtMesh->m_subsets.back().fRadius = pTgtMesh->m_bbox.GetRadius();
            } //end of subset
          }
        }
      }
    }
    // update skinning info
    int numFaces = 0; 
    int numVerts = 0; 
    for(int s=0;s<pTgtMesh->m_subsets.size();s++)
    {
      SMeshSubset& subset = pTgtMesh->m_subsets[s];
      numFaces+=subset.nNumIndices/3;
      numVerts+=subset.nNumVerts;
    }
    pTgtSkinningInfo->m_arrExt2IntMap.resize(numVerts);
    for(int n=0;n<numVerts;n++)
    {
      pTgtSkinningInfo->m_arrExt2IntMap[n]=n;
    }
    //
    pTgtSkinningInfo->m_arrIntVertices.resize(numVerts);
    pTgtSkinningInfo->m_arrIntFaces.resize(numFaces);
    int nFace = 0;
    int nVert = 0;
    for(int s=0;s<pTgtMesh->m_subsets.size();s++)
    {
      SMeshSubset& subset = pTgtMesh->m_subsets[s];
      for(int n=0;n<subset.nNumVerts;n++)
      {
        pTgtSkinningInfo->m_arrIntVertices[nVert].wpos0 = pTgtMesh->m_pPositions[subset.nFirstVertId+n];
        pTgtSkinningInfo->m_arrIntVertices[nVert].wpos1 = pTgtMesh->m_pPositions[subset.nFirstVertId+n];
        pTgtSkinningInfo->m_arrIntVertices[nVert].wpos2 = pTgtMesh->m_pPositions[subset.nFirstVertId+n];
        if( pTgtMesh->m_pColor0 )
          pTgtSkinningInfo->m_arrIntVertices[nVert].color = ColorB(pTgtMesh->m_pColor0[subset.nFirstVertId+n].r,pTgtMesh->m_pColor0[subset.nFirstVertId+n].g,pTgtMesh->m_pColor0[subset.nFirstVertId+n].b,pTgtMesh->m_pColor0[subset.nFirstVertId+n].a);
        else
          pTgtSkinningInfo->m_arrIntVertices[nVert].color.Set(255,255,255,0);
        //
        for(int i=0;i<4;i++)
        {
          uint8 boneId = pTgtMesh->m_pBoneMapping[subset.nFirstVertId+n].boneIDs[i];
          uint16 globalBoneId = subset.m_arrGlobalBonesPerSubset[boneId];
          pTgtSkinningInfo->m_arrIntVertices[nVert].boneIDs[i] = globalBoneId;
          pTgtSkinningInfo->m_arrIntVertices[nVert].weights[i] = (float)pTgtMesh->m_pBoneMapping[subset.nFirstVertId+n].weights[i]/(float)255.0f;
        }
        nVert++;
      }
      for(int n=0;n<subset.nNumIndices/3;n++)
      {
        pTgtSkinningInfo->m_arrIntFaces[nFace].i0 = pTgtMesh->m_pIndices[subset.nFirstIndexId+n*3+0];
        pTgtSkinningInfo->m_arrIntFaces[nFace].i1 = pTgtMesh->m_pIndices[subset.nFirstIndexId+n*3+1];
        pTgtSkinningInfo->m_arrIntFaces[nFace].i2 = pTgtMesh->m_pIndices[subset.nFirstIndexId+n*3+2];
        nFace++;
      }
    }
  }
  if( (merge_flags & IMergePipe::mfVerbose_LOD) )
  {
    CryLog("LODGEN: %10s: ORG( v: %5d f: %5d ) LOD( v: %5d f: %5d ) REL( v: %5.2f%% f: %5.2f%%)","TOTAL",sum_numVertsBefore,sum_numFacesBefore,sum_numVertsAfter,sum_numFacesAfter,(float)sum_numVertsAfter/(float)sum_numVertsBefore*100.0f,(float)sum_numFacesAfter/(float)sum_numFacesBefore*100.0f);
  }
  //
  // Save
  //
  CSaverCGF cgfSaver( result_filename,tgtChunkFile );
  //---------------------------------------------------------------------
  //---  write compiled bones to chunk
  //---------------------------------------------------------------------
  CryBoneDescData* pCryBoneDescData = &pTgtSkinningInfo->m_arrBonesDesc[0];
  uint32 numAnimBones = pTgtSkinningInfo->m_arrBonesDesc.size();
  int q = cgfSaver.SaveCompiledBones( pCryBoneDescData, numAnimBones*sizeof(CryBoneDescData) );
  //---------------------------------------------------------------------
  //---  write compiled physical bones to chunk
  //---------------------------------------------------------------------
  BONE_ENTITY* pPhysicalBone = &pTgtSkinningInfo->m_arrBoneEntities[0];
  uint32 numPhyBones = pTgtSkinningInfo->m_arrBonesDesc.size();
  int nn = cgfSaver.SaveCompiledPhysicalBones( pPhysicalBone, numPhyBones*sizeof(BONE_ENTITY) );
  //---------------------------------------------------------------------
  //--- create compiled physical proxi mesh and write it to chunk     ---
  //---------------------------------------------------------------------
  createAndSavePhysicalProxyMesh(pTgtSkinningInfo,cgfSaver);

  //---------------------------------------------------------------------
  //--- create compiled internal morph-targets and write them to chunk ---
  //---------------------------------------------------------------------
  // cleared above
  if( bBakeMorphs )
  {
    int nWrongModelFormat = 1;
    Matrix33 z180(IDENTITY);

    if (nWrongModelFormat)
      z180.SetRotationZ(gf_PI);

    //---------------------------------------------------------------------
    //--- create compiled internal morph-targets and write them to chnk ---
    //---------------------------------------------------------------------
    std::vector<uint8> cmt;
    uint32 numMorphtargets = pTgtSkinningInfo->m_arrMorphTargets.size();
    for(uint32 i=0; i<numMorphtargets; i++) 
    {
      SMeshMorphTargetHeader header;
      //store the mesh ID
      header.MeshID = 0;//pTgtSkinningInfo->m_arrMorphTargets[i]->MeshID;
      //store the name of morph-target
      header.NameLength = pTgtSkinningInfo->m_arrMorphTargets[i]->m_strName.size()+1;
      //store the vertices&indices of morph-target
      header.numIntVertices = pTgtSkinningInfo->m_arrMorphTargets[i]->m_arrIntMorph.size();
      //store the vertices&indices of morph-target
      header.numExtVertices = pTgtSkinningInfo->m_arrMorphTargets[i]->m_arrExtMorph.size();
      //
      uint8* ptr=(uint8*)(&header);
      for(uint32 h=0; h<sizeof(SMeshMorphTargetHeader); h++) 
        cmt.push_back(ptr[h]);
      //
      for(uint32 h=0; h<header.NameLength; h++) 
        cmt.push_back(pTgtSkinningInfo->m_arrMorphTargets[i]->m_strName[h]);
      //
      if (nWrongModelFormat) {
        for(uint32 h=0; h<header.numIntVertices; h++) {
          pTgtSkinningInfo->m_arrMorphTargets[i]->m_arrIntMorph[h].ptVertex = z180 * pTgtSkinningInfo->m_arrMorphTargets[i]->m_arrIntMorph[h].ptVertex;
        }
      }
      ptr=(uint8*)(&pTgtSkinningInfo->m_arrMorphTargets[i]->m_arrIntMorph[0]);
      for(uint32 h=0; h<(sizeof(SMeshMorphTargetVertex)*header.numIntVertices); h++) 
        cmt.push_back(ptr[h]);
      //
      if (nWrongModelFormat) {
        for(uint32 h=0; h<header.numExtVertices; h++) {
          pTgtSkinningInfo->m_arrMorphTargets[i]->m_arrExtMorph[h].ptVertex = z180 * pTgtSkinningInfo->m_arrMorphTargets[i]->m_arrExtMorph[h].ptVertex;
        }
      }
      ptr=(uint8*)(&pTgtSkinningInfo->m_arrMorphTargets[i]->m_arrExtMorph[0]);
      for(uint32 h=0; h<(sizeof(SMeshMorphTargetVertex)*header.numExtVertices); h++) {
        cmt.push_back(ptr[h]);
      }
    }
    cgfSaver.SaveCompiledMorphTargets( &cmt[0], cmt.size(), numMorphtargets );
  }
  //---------------------------------------------------------------------
  //---  write internal skinning vertices to chunk
  //---------------------------------------------------------------------
  IntSkinVertex* pIntSkinVertex = &pTgtSkinningInfo->m_arrIntVertices[0];
  uint32 numIntVertices = pTgtSkinningInfo->m_arrIntVertices.size();
  int m = cgfSaver.SaveCompiledIntSkinVertices( pIntSkinVertex,numIntVertices*sizeof(IntSkinVertex) );
  //---------------------------------------------------------------------
  //---  write internal faces to chunk
  //---------------------------------------------------------------------
  uint32 numIntFaces = pTgtSkinningInfo->m_arrIntFaces.size();
  cgfSaver.SaveCompiledIntFaces( &pTgtSkinningInfo->m_arrIntFaces[0], numIntFaces*sizeof(TFace) );
  //---------------------------------------------------------------------
  //---  write Ext2IntMap to chunk
  //---------------------------------------------------------------------
  uint32 numExt2Int = pTgtSkinningInfo->m_arrExt2IntMap.size();
  cgfSaver.SaveCompiledExt2IntMap( &pTgtSkinningInfo->m_arrExt2IntMap[0], numExt2Int*sizeof(uint16) );
  //
  //
  //
  cgfSaver.SetContent(pTgtCGF);
  cgfSaver.SaveNodes();
  // Force remove of the read only flag.
  CrySetFileAttributes( result_filename,FILE_ATTRIBUTE_NORMAL );
  tgtChunkFile.Write( result_filename );
  //
  delete pTgtCGF;
  delete cgfTgtLoader.GetCContentCGF();
  for(int n=0;n<numChrs;n++)
  {
    delete contents[n];
    delete cgfs[n].GetCContentCGF();
  }
  return true;
}

static bool CompareThreshold(const ColorF& a,const ColorF& b)
{
  ColorF diff = a-b;
  if( cry_fabsf(diff.r)>0.0001f )
    return false;
  if( cry_fabsf(diff.g)>0.0001f )
    return false;
  if( cry_fabsf(diff.b)>0.0001f )
    return false;
  if( cry_fabsf(diff.a)>0.0001f )
    return false;
  return true;
}

bool C3DEngine::CanBeMerged(IMaterial* mtl1,IMaterial* mtl2,bool bVerbose,IMaterialMergePipe::EMergeMaterialsFlags flags)
{
  //
  if( bVerbose )
  {
    //CryLog("---------------- MergeInfo Issues ----------------------");
  }
  //
  bool bAllEqual = true;
  //
  SShaderItem& si1 = mtl1->GetShaderItem(0);
  SShaderItem& si2 = mtl2->GetShaderItem(0);
  if( strcmp(si1.m_pShader->GetName(),si2.m_pShader->GetName())!=0 )
  {
    bAllEqual = false;
    if( bVerbose )
    {
      CryLog("Issue: ShaderName:%s %s Mtl1:%s Mtl2:%s ",si1.m_pShader->GetName(),si2.m_pShader->GetName(),mtl1->GetName(),mtl2->GetName());
    }
  }
  //      DynArray<SShaderParam>& si1Params = si1.m_pShader->GetPublicParams();
  //    DynArray<SShaderParam>& si2Params = si2.m_pShader->GetPublicParams();
  DynArray<SShaderParam> si1Params = si1.m_pShaderResources->GetParameters();
  DynArray<SShaderParam> si2Params = si2.m_pShaderResources->GetParameters();
  //
  if( si1Params.size()!=si2Params.size() )
  {
    bAllEqual = false;
    if( bVerbose )
    {
      CryLog("Issue: params count is different Mtl1:%s Mtl2:%s ",mtl1->GetName(),mtl2->GetName());
    }
  }
  else
  {
    for( int p=0;p<si1Params.size();p++ )
    {
      bool bFound = false;
      SShaderParam& param = si1Params[p];
      for( int p1=0;p1<si1Params.size();p1++ )
      {
        SShaderParam& param2 = si2Params[p1];
        UParamVal& val1 = param.m_Value;
        UParamVal& val2 = param2.m_Value;
        if( strcmp(param.m_Name,param2.m_Name)==0 )
        {
          bool bEqualParams = true;
          if( param.m_Type==eType_UNKNOWN )
          {
            //continue;
          }
          else if( param.m_Type!=param2.m_Type )
          {
            bEqualParams = false;
          }
          else if( param.m_Type==eType_BYTE )
          {
            if( val1.m_Byte!=val2.m_Byte )
            {
              bEqualParams = false;
            }
          }
          else if( param.m_Type==eType_SHORT )
          {
            if( val1.m_Short!=val2.m_Short )
            {
              bEqualParams = false;
            }
          }
          else if( param.m_Type==eType_INT || param.m_Type==eType_TEXTURE_HANDLE)
          {
            if( val1.m_Int!=val2.m_Int )
            {
              bEqualParams = false;
            }
          }
          else if( param.m_Type==eType_FLOAT )
          {
            if( !CompareThreshold(val1.m_Float,val2.m_Float) )
            {
              bEqualParams = false;
            }
          }
          else if( param.m_Type==eType_STRING )
          {
            if( strcmp(val1.m_String,val2.m_String)!=0 )
            {
              bEqualParams = false;
            }
          }
          else if( param.m_Type==eType_FCOLOR )
          {
            if( !CompareThreshold(val1.m_Color[0],val2.m_Color[0]) ||
                !CompareThreshold(val1.m_Color[1],val2.m_Color[1]) ||
                !CompareThreshold(val1.m_Color[2],val2.m_Color[2]) ||
                !CompareThreshold(val1.m_Color[3],val2.m_Color[3]) )
            {
              bEqualParams = false;
            }
          }
          else if( param.m_Type==eType_VECTOR )
          {
            if( !CompareThreshold(val1.m_Vector[0],val2.m_Vector[0]) ||
              !CompareThreshold(val1.m_Vector[1],val2.m_Vector[1]) ||
              !CompareThreshold(val1.m_Vector[2],val2.m_Vector[2]) )
            {
              bEqualParams = false;
            }
          }
          else if( param.m_Type==eType_CAMERA )
          {
            if( val1.m_pCamera!=val2.m_pCamera )
            {
              bEqualParams = false;
            }
          }
          //
          if( !bEqualParams )
          {
            bAllEqual = false;
            if( bVerbose )
            {
              CryLog("Issue: Param:%s Mtl1:%s Mtl2:%s ",param.m_Name,mtl1->GetName(),mtl2->GetName());
            }
          }
          bFound = true;
          break;
        }
      }
      if( !bFound || bAllEqual==false)
      {
        bAllEqual = false;
      }
    }
  }
  if( bAllEqual )
  {
    if( !CompareThreshold(si1.m_pShaderResources->GetDiffuseColor(),si2.m_pShaderResources->GetDiffuseColor()) )
    {
      if( bVerbose )
      {
        CryLog("Issue: Diffuse Color: Mtl1:%s Mtl2:%s ",mtl1->GetName(),mtl2->GetName());
      }
      bAllEqual = false;
    }
    if( !CompareThreshold(si1.m_pShaderResources->GetSpecularColor(),si2.m_pShaderResources->GetSpecularColor()) )
    {
      if( bVerbose )
      {
        CryLog("Issue: Specular Color: Mtl1:%s Mtl2:%s ",mtl1->GetName(),mtl2->GetName());
      }
      bAllEqual = false;
    }
    if( !CompareThreshold(si1.m_pShaderResources->GetEmissiveColor(),si2.m_pShaderResources->GetEmissiveColor()) )
    {
      if( bVerbose )
      {
        CryLog("Issue: Emmisive Color: Mtl1:%s Mtl2:%s ",mtl1->GetName(),mtl2->GetName());
      }
      bAllEqual = false;
    }
    if( si1.m_pShaderResources->GetSpecularShininess()!=si2.m_pShaderResources->GetSpecularShininess() )
    {
      if( bVerbose )
      {
        CryLog("Issue: Specular Shininess: Mtl1:%s Mtl2:%s ",mtl1->GetName(),mtl2->GetName());
      }
      bAllEqual = false;
    }
    int flag1 = si1.m_pShaderResources->GetResFlags() & (~MTL_FLAG_PURE_CHILD);
    int flag2 = si2.m_pShaderResources->GetResFlags() & (~MTL_FLAG_PURE_CHILD);
    if( flag1!=flag2 )
    {
      if( bVerbose )
      {
        CryLog("Issue: ResFlags: Mtl1:%s %X Mtl2:%s %X",mtl1->GetName(),flag1,mtl2->GetName(),flag2);
      }
      bAllEqual = false;
    }
    if( si1.m_pShaderResources->GetMtlLayerNoDrawFlags()!=si2.m_pShaderResources->GetMtlLayerNoDrawFlags() )
    {
      if( bVerbose )
      {
        CryLog("Issue: NoDraw Flags: Mtl1:%s Mtl2:%s ",mtl1->GetName(),mtl2->GetName());
      }
      bAllEqual = false;
    }
    if( si1.m_pShaderResources->GetGlow()!=si2.m_pShaderResources->GetGlow() )
    {
      if( bVerbose )
      {
        CryLog("Issue: Glow: Mtl1:%s Mtl2:%s ",mtl1->GetName(),mtl2->GetName());
      }
      bAllEqual = false;
    }
    if( si1.m_pShaderResources->GetAlphaRef()!=si2.m_pShaderResources->GetAlphaRef() )
    {
      if( bVerbose )
      {
        CryLog("Issue: AlphaRef: Mtl1:%s Mtl2:%s ",mtl1->GetName(),mtl2->GetName());
      }
      bAllEqual = false;
    }
    if( si1.m_pShaderResources->GetOpacity()!=si2.m_pShaderResources->GetOpacity() )
    {
      if( bVerbose )
      {
        CryLog("Issue: Opacity: Mtl1:%s Mtl2:%s ",mtl1->GetName(),mtl2->GetName());
      }
      bAllEqual = false;
    }
    static char* texnames[] = 
    {
      "EFTT_DIFFUSE",
      "EFTT_BUMP",
      "EFTT_GLOSS",
      "EFTT_ENV",
      "EFTT_DETAIL_OVERLAY",
      "EFTT_BUMP_DIFFUSE",
      "EFTT_PHONG",
      "EFTT_BUMP_HEIGHT",
      "EFTT_SPECULAR",
      "EFTT_DECAL_OVERLAY",
      "EFTT_NORMALMAP",
      "EFTT_SUBSURFACE",
      "EFTT_CUSTOM",
      "EFTT_CUSTOM_SECONDARY",
      "EFTT_OPACITY",
      "EFTT_LIGHTMAP",
      "EFTT_LIGHTMAP_HDR",
      "EFTT_LIGHTMAP_DIR",
      "EFTT_RAE",
      "EFTT_OCCLUSION",
    };
    for(int n=0;n<EFTT_MAX;n++)
    {
      // logical xor
      //
      bool bTex1 = si1.m_pShaderResources->GetTexture(n)==NULL ? false : true;
      if( bTex1 && si1.m_pShaderResources->GetTexture(n)->m_Name.empty() )
      {
        bTex1 = false;
      }
      bool bTex2 = si2.m_pShaderResources->GetTexture(n)==NULL ? false : true;
      if( bTex2 && si2.m_pShaderResources->GetTexture(n)->m_Name.empty() )
      {
        bTex2 = false;
      }
      if( bTex1==false && bTex2==false )
      {
        continue;
      }
      if( bTex1 != bTex2 )
      {
        bAllEqual = false;
        if( bVerbose )
        {
          CryLog("Issue: TextureMap:%s Mtl1:%s Mtl2:%s ",texnames[n],mtl1->GetName(),mtl2->GetName());
        }
        break;
      }
      else
      {
        if( flags & IMaterialMergePipe::mfDoNotMergeTextures )
        {
          // textures should be equal
          if( si1.m_pShaderResources->GetTexture(n) )
          {
            if( si1.m_pShaderResources->GetTexture(n)->m_Name!=si2.m_pShaderResources->GetTexture(n)->m_Name )
            {
              bAllEqual = false;
              if( bVerbose )
              {
                CryLog("Issue: Textures Mtl1:%s Mtl2:%s is not equal",/*texnames[n],*/mtl1->GetName(),mtl2->GetName());
              }
            }
          }
        }
        if( si1.m_pShaderResources->GetTexture(n)!=NULL )
        {
          if(si1.m_pShaderResources->GetTexture(n)->m_TexFlags!=si2.m_pShaderResources->GetTexture(n)->m_TexFlags)
          {
            if( bVerbose )
            {
              CryLog("Issue: TexFlags: Texture:%s Mtl1:%s Mtl2:%s ",texnames[n],mtl1->GetName(),mtl2->GetName());
            }
            bAllEqual = false;
          }
          if(si1.m_pShaderResources->GetTexture(n)->m_Amount!=si2.m_pShaderResources->GetTexture(n)->m_Amount)
          {
            if( bVerbose )
            {
              CryLog("Issue: Amount: Texture:%s Mtl1:%s Mtl2:%s ",texnames[n],mtl1->GetName(),mtl2->GetName());
            }
            bAllEqual = false;
          }
          if(si1.m_pShaderResources->GetTexture(n)->m_bUTile!=si2.m_pShaderResources->GetTexture(n)->m_bUTile)
          {
            if( bVerbose )
            {
              CryLog("Issue: bUTile: Texture:%s Mtl1:%s Mtl2:%s ",texnames[n],mtl1->GetName(),mtl2->GetName());
            }
            bAllEqual = false;
          }
          if(si1.m_pShaderResources->GetTexture(n)->m_bVTile!=si2.m_pShaderResources->GetTexture(n)->m_bVTile)
          {
            if( bVerbose )
            {
              CryLog("Issue: bVTile: Texture:%s Mtl1:%s Mtl2:%s ",texnames[n],mtl1->GetName(),mtl2->GetName());
            }
            bAllEqual = false;
          }
          if(si1.m_pShaderResources->GetTexture(n)->m_Filter!=si2.m_pShaderResources->GetTexture(n)->m_Filter)
          {
            if( bVerbose )
            {
              CryLog("Issue: Filter: Texture:%s Mtl1:%s Mtl2:%s ",texnames[n],mtl1->GetName(),mtl2->GetName());
            }
            bAllEqual = false;
          }
          if(si1.m_pShaderResources->GetTexture(n)->m_TexModificator!=si2.m_pShaderResources->GetTexture(n)->m_TexModificator)
          {
            if( bVerbose )
            {
              CryLog("Issue: TexMod: Texture:%s Mtl1:%s Mtl2:%s ",texnames[n],mtl1->GetName(),mtl2->GetName());
            }
            bAllEqual = false;
          }
          if(si1.m_pShaderResources->GetTexture(n)->m_Sampler!=si2.m_pShaderResources->GetTexture(n)->m_Sampler)
          {
            if( bVerbose )
            {
              CryLog("Issue: Sampler: Texture:%s Mtl1:%s Mtl2:%s ",texnames[n],mtl1->GetName(),mtl2->GetName());
            }
            bAllEqual = false;
          }
        }
      }
    }
  }
  return bAllEqual;
}

static const float alignVal = 64.0f;// move to console var 
inline int AlignToRight(float value,float valAlign=alignVal)
{
  int iv = (int)(value/valAlign);
  iv*=(int)valAlign;
  if( value>0 )
  {
    float mod = fmod(value,valAlign);
    if( mod>0.0f )
      iv+=(int)valAlign;
  }
  return iv;
}

inline int AlignToLeft(float value,float valAlign=alignVal)
{
  int iv = (int)(value/valAlign);
  iv*=(int)valAlign;
  if( value<0 )
  {
    float mod = fmod(value,valAlign);
    if( mod>0.0f )
      iv-=(int)valAlign;
    return iv;
  }
  return iv;
}

class CTexAtlas
{
public:

  CTexAtlas()
  {
    m_nLMWidth = m_nLMHeight = 0;
  }

  CTexAtlas(int nSize)
  {
    Init(nSize);
  }

  ~CTexAtlas()
  {
  }

  void Init(int nSize)
  {
    m_nLMWidth = m_nLMHeight = nSize;
    m_pAllocatedTexels.resize(m_nLMHeight,0);
    memset(&m_pAllocatedTexels[0],0,m_nLMHeight*sizeof(int));
  }

  bool AllocateGroup(int32* pOffsetX, int32* pOffsetY, int nSizeX, int nSizeY,bool bAllocate=true,int nBorder=4,int* usedWidth=NULL,int* usedHeight=NULL )
  {
    ///*
    int32	nBestX = m_nLMWidth;
    int32	nBestY = m_nLMHeight;
    int32	nBestWaste = m_nLMWidth * m_nLMHeight;

    nSizeX += nBorder*2;
    nSizeY += nBorder*2;

    for(int32 Y = 0; Y < m_nLMHeight; Y++)
    {
      if( Y+nSizeY>m_nLMHeight )
        break;

      int32	nMaxX = 0;
      int32	nWaste = 0;

      for(int32 Row = 0; Row < nSizeY; Row++)
        nMaxX = AlignToRight((float)max(m_pAllocatedTexels[Y + Row],nMaxX));

      for(int32 Row = 0;Row < nSizeY;Row++)
        nWaste += nMaxX - m_pAllocatedTexels[Y + Row];

      //
      if( nMaxX+nSizeX <= m_nLMWidth && nWaste < nBestWaste )
      {
        nBestWaste = nWaste;
        nBestX = nMaxX;
        nBestY = Y;
      }
    }

    if(nBestX <= m_nLMWidth - nSizeX && nBestY <= m_nLMHeight - nSizeY)
    {
      *pOffsetX = nBestX + nBorder;
      *pOffsetY = nBestY + nBorder;

      if( usedWidth && usedHeight )
      {
        GetUsedCaps(*usedWidth,*usedHeight);
        for(int32 Row = 0;Row < nSizeY;Row++)
        {
          int used = nBestX + nSizeX;
          *usedWidth = max(*usedWidth,used);
        }
        *usedHeight = max(*usedHeight,nBestY + nSizeY);
      }

      if( bAllocate )
      {
        for(int32 Row = 0;Row < nSizeY;Row++)
          m_pAllocatedTexels[nBestY + Row] = nBestX + nSizeX;
      }
      return true;
    }

    return false;
    //*/
    /*
    int32	nBestX = m_nLMWidth;
    int32	nBestY = m_nLMHeight;
    int32	nBestWaste = m_nLMWidth * m_nLMHeight;

    nSizeX += nBorder*2;
    nSizeY += nBorder*2;

    for(int32 Y = 0; Y < (m_nLMHeight - nSizeY); Y++)
    {
      int32	nMaxX = 0;
      int32	nWaste = 0;

      for(int32 Row = 0; Row < nSizeY; Row++)
        nMaxX = AlignTo4Right(max(m_pAllocatedTexels[Y + Row],nMaxX));

      for(int32 Row = 0;Row < nSizeY;Row++)
        nWaste += nMaxX - m_pAllocatedTexels[Y + Row];

      if(nMaxX <= m_nLMWidth - nSizeX && nWaste < nBestWaste)
      {
        nBestX = nMaxX;
        nBestY = Y;
        nBestWaste = nWaste;
      }
    }

    if(nBestX <= m_nLMWidth - nSizeX && nBestY <= m_nLMHeight - nSizeY)
    {
      *pOffsetX = nBestX + nBorder;
      *pOffsetY = nBestY + nBorder;

      if( usedWidth && usedHeight )
      {
        GetUsedCaps(*usedWidth,*usedHeight);
        for(int32 Row = 0;Row < nSizeY;Row++)
        {
          int used = nBestX + nSizeX;
          *usedWidth = max(*usedWidth,used);
        }
        *usedHeight = max(*usedHeight,nBestY + nSizeY);
      }

      if( bAllocate )
      {
        for(int32 Row = 0;Row < nSizeY;Row++)
          m_pAllocatedTexels[nBestY + Row] = nBestX + nSizeX;
      }
      return true;
    }

    return false;
    */
  }
  
  void GetUsedCaps(int& width,int& height)
  {
    width = 0;
    height = 0;
    for(int32 Row = 0; Row < m_nLMHeight; Row++)
    {
      if( m_pAllocatedTexels[Row]>0 )
      {
        height = Row+1;
      }
      width = max(m_pAllocatedTexels[Row],width);
    }
  }

  int m_nLMWidth;
  int m_nLMHeight;
  std::vector<int> m_pAllocatedTexels;
};

static IMaterial* CloneMaterial(IMaterial* org)
{
  IMaterialManager* pMatMan = gEnv->p3DEngine->GetMaterialManager();
  IMaterial* clonedMtl = pMatMan->CloneMaterial(org);
  return clonedMtl;
  /*
  // check for slot EFTT_NORMALMAP
  SShaderItem& si = org->GetShaderItem(0);
  //
  f( si.m_pShaderResources->GetTexture(EFTT_NORMALMAP) )
  {
    SShaderItem& siCloned = clonedMtl->GetShaderItem(0);
  f( !siCloned.m_pShaderResources->GetTexture(EFTT_NORMALMAP) )
    {
      siCloned.m_pShaderResources->SetTexture(EFTT_NORMALMAP,si.m_pShaderResources->GetTexture(EFTT_NORMALMAP));
    }
  }
  return clonedMtl;
  */
}

static int MaxMergedTexureSize = 2048;//*2;//12048*4;
struct ResultSubset
{
  ResultSubset()
  {
    for(int n=0;n<EFTT_MAX;n++)
    {
      atlas[n].Init(MaxMergedTexureSize);
    }
  }

  ResultSubset(const ResultSubset& res)
  {
    *this = res;
  }

  ResultSubset& operator=(const ResultSubset& res)
  {
    mtlMerged = res.mtlMerged;
    for(int n=0;n<EFTT_MAX;n++)
    {
      atlas[n] = res.atlas[n];
    }
    mtls = res.mtls;
    return *this;
  }
  //
  IMaterial* mtlMerged;
  CTexAtlas atlas[EFTT_MAX];
  std::vector<SMergedMtlInfo> mtls;
};

struct MergedDataSubSet
{
  IMaterial* mtlSubSet;
  std::vector<SMergedMtlInfo> info;
  //
  std::vector<ResultSubset> result;
};

struct MergedData
{
  std::vector<MergedDataSubSet> subsets;
  bool Solve(bool UseOneAtlassForAllChannels);
  bool Dispatch(SMergedMtlInfo& SMergedMtlInfoToDispatch,ResultSubset& result_subset,int border,bool UseOneAtlassForAllChannels);
};

bool MergedData::Dispatch(SMergedMtlInfo& SMergedMtlInfoToDispatch,ResultSubset& result_subset,int border,bool UseOneAtlassForAllChannels)
{
  SShaderItem& si = SMergedMtlInfoToDispatch.mtlOrg->GetShaderItem(0);
  bool bManaged = true;
  bool bWasAdded[EFTT_MAX];
  bool bAvailable[EFTT_MAX];
  bool bAllTextures = true;
  SFixedArray< std::vector<int>,EFTT_MAX> x,y,w,h;
  int usedw[EFTT_MAX],usedh[EFTT_MAX];
  CTexAtlas atlases_before[EFTT_MAX];
  for(int n=0;n<EFTT_MAX;n++)
  {
    atlases_before[n] = result_subset.atlas[n];
  }
  // try to atlas all textures for this res subset
  SMergedMtlInfo info;
  info.mtlOrg = SMergedMtlInfoToDispatch.mtlOrg;
  //
  for(uint32 s=0;s<SMergedMtlInfoToDispatch.src_rects.size();s++)
  {
    for(int n=0;n<EFTT_MAX;n++)
    {
      bAvailable[n] = false;
      //
      if( !si.m_pShaderResources->GetTexture(n) )
        continue;
      //
      if( si.m_pShaderResources->GetTexture(n)->m_Name.empty() )
        continue;
      //
      bAvailable[n] = true;
      //
      SEfResTexture* pTexture = si.m_pShaderResources->GetTexture(n);
//      STexSampler& sampler = pTexture->m_Sampler;
      // before atlas texture check for already added
      /*
      bWasAdded[n] = false;
      for(int a=0;a<result_subset.mtls.size();a++)
      {
      if( strcmp(result_subset.mtls[a].org_filename[n],pTexture->m_Name)==0 )
      {
      //if( contains our rect )
      {
      bWasAdded[n] = true;
      for(int s=0;s<result_subset.mtls[a].src_rects[n].size();s++)
      {
      x[n].push_back(result_subset.mtls[a].src_x[n][s]);
      y[n].push_back(result_subset.mtls[a].src_y[n][s]);
      w[n].push_back(result_subset.mtls[a].w[n][s]);
      h[n].push_back(result_subset.mtls[a].h[n][s]);
      }
      break;
      }
      }
      }
      if( !bWasAdded[n] )
      */
      {
        int out_x=0,out_y=0;
        int in_w = SMergedMtlInfoToDispatch.w[n][s];
        int in_h = SMergedMtlInfoToDispatch.h[n][s];
        if( !result_subset.atlas[n].AllocateGroup(&out_x,&out_y,in_w,in_h,false,border,&usedw[n],&usedh[n]) )
        {
          bAllTextures = false;
          break;
        }
        else
        {
          x[n].push_back(out_x);
          y[n].push_back(out_y);
          w[n].push_back(in_w);
          h[n].push_back(in_h);
        }
      }
    }
    // check for all chanels - all chanels should be placed to the same relative places
    if( bAllTextures && !UseOneAtlassForAllChannels )
    {
      std::vector<bool> bInitialized(SMergedMtlInfoToDispatch.src_rects.size(),false);
      std::vector<float> ox(SMergedMtlInfoToDispatch.src_rects.size(),0.0f);
      std::vector<float> oy(SMergedMtlInfoToDispatch.src_rects.size(),0.0f);
      std::vector<float> ow(SMergedMtlInfoToDispatch.src_rects.size(),0.0f);
      std::vector<float> oh(SMergedMtlInfoToDispatch.src_rects.size(),0.0f);
      for(int n=0;n<EFTT_MAX;n++)
      {
        //
        if( !bAvailable[n] )
          continue;
        //
        float xx = (float)x[n][s];
        float yy = (float)y[n][s];
        float ww = (float)w[n][s];
        float hh = (float)h[n][s];
        float rel_x = xx/(float)usedw[n];
        float rel_y = yy/(float)usedh[n];
        float rel_w = ww/(float)usedw[n];
        float rel_h = hh/(float)usedh[n];

        if( bInitialized[s] )
        {
          if( ox[s]!=rel_x )
          {
            bAllTextures = false;
            break;
          }
          if( oy[s]!=rel_y )
          {
            bAllTextures = false;
            break;
          }
          if( ow[s]!=rel_w )
          {
            bAllTextures = false;
            break;
          }
          if( oh[s]!=rel_h )
          {
            bAllTextures = false;
            break;
          }
        }
        else
        {
          ox[s] = rel_x;
          oy[s] = rel_y;
          ow[s] = rel_w;
          oh[s] = rel_h;
          bInitialized[s] = true;
        }
      }
    }
    //
    if( bAllTextures )
    {
      // commit atlases
      for(int n=0;n<EFTT_MAX;n++)
      {
        //
        if( !bAvailable[n] )
          continue;
        //
        info.src_rects = SMergedMtlInfoToDispatch.src_rects;
        //
        info.org_filename[n] = SMergedMtlInfoToDispatch.org_filename[n];
        info.dds_filename[n] = SMergedMtlInfoToDispatch.dds_filename[n];
        //
        info.src_x[n].push_back(SMergedMtlInfoToDispatch.src_x[n][s]);
        info.src_y[n].push_back(SMergedMtlInfoToDispatch.src_y[n][s]);
        //
        info.uv_min_x[n].push_back(SMergedMtlInfoToDispatch.uv_min_x[n][s]);
        info.uv_min_y[n].push_back(SMergedMtlInfoToDispatch.uv_min_y[n][s]);
        //
        info.uv_max_x[n].push_back(SMergedMtlInfoToDispatch.uv_max_x[n][s]);
        info.uv_max_y[n].push_back(SMergedMtlInfoToDispatch.uv_max_y[n][s]);
        //
        info.src_w[n] = SMergedMtlInfoToDispatch.src_w[n];
        info.src_h[n] = SMergedMtlInfoToDispatch.src_h[n];
        //
        info.dst_x[n].push_back(x[n][s]);
        info.dst_y[n].push_back(y[n][s]);
        info.w[n].push_back(w[n][s]);
        info.h[n].push_back(h[n][s]);
        //
        info.border[n].push_back(border);
        //
        if( bWasAdded[n] )
          continue;
        //
        int32 _x,_y;
        result_subset.atlas[n].AllocateGroup(&_x,&_y,w[n][s],h[n][s],true,border);
      }
    }
    else
    {
      bManaged = false;
    }
  }
  if( bManaged )
  {
    result_subset.mtls.push_back(info);
  }
  // restore atlases
  if( !bManaged )
  {
    for(int n=0;n<EFTT_MAX;n++)
    {
      result_subset.atlas[n] = atlases_before[n];
    }
  }
  return bManaged;
}

// Return whether first element is greater than the second
static int BiggerInFront( const void* a, const void* b)
{
	const SMergedMtlInfo& elem1 = *(const SMergedMtlInfo*)a;
	const SMergedMtlInfo& elem2 = *(const SMergedMtlInfo*)b;
  for(int n=0;n<EFTT_MAX;n++)
  {
    if( !elem1.dds_filename[n].empty() )
    {
      float sz1 = 0;
      for(uint32 s=0;s<elem1.src_rects.size();s++)
      {
        sz1 = max(sz1,(float)elem1.h[n][s]);
      }
      float sz2 = 0;
      for(uint32 s=0;s<elem2.src_rects.size();s++)
      {
        sz2 = max(sz2,(float)elem2.h[n][s]);
      }
      if( sz1 != sz2 )
			{
        if( sz1 > sz2 )
					return 1;
				else
					return -1;
			}
			return 0;
    }
  }
	return 0;
}

// Return whether first element is greater than the second
static bool BiggerInFrontSubsets( const AABB& elem1, const AABB& elem2 )
{
    float sz1 = (elem1.max.x-elem1.min.x) * (elem1.max.y-elem1.min.y) ;
    float sz2 = (elem2.max.x-elem2.min.x) * (elem2.max.y-elem2.min.y) ;
    return (sz1 > sz2);
}

static int defBorder = 0;
bool MergedData::Solve(bool UseOneAtlassForAllChannels)
{
  for(uint32 s=0;s<subsets.size();s++)
  {
    // sort by textures (ie bigger sizes first)
		if(!subsets[s].info.empty())
			qsort(&subsets[s].info[0], subsets[s].info.size(), sizeof(subsets[s].info[0]), BiggerInFront);
    //
    for(uint32 m=0;m<subsets[s].info.size();m++)
    {
      SMergedMtlInfo& SMergedMtlInfoToDispatch = subsets[s].info[m];
      bool bManaged = false;
      // find good result subspace
      for(uint32 r=0;r<subsets[s].result.size();r++)
      {
        if (Dispatch(SMergedMtlInfoToDispatch,subsets[s].result[r],defBorder,UseOneAtlassForAllChannels) )
        {
          bManaged = true;
          break;
        }
      }
      if( !bManaged )
      {
        // create new res subset
        subsets[s].result.push_back(ResultSubset());
        subsets[s].result.back().mtlMerged = CloneMaterial(subsets[s].mtlSubSet);
        bool bRes = Dispatch(SMergedMtlInfoToDispatch,subsets[s].result.back(),defBorder,UseOneAtlassForAllChannels);
        if( !bRes )
        {
          subsets[s].result.pop_back();
          //
          subsets[s].result.push_back(ResultSubset());
          subsets[s].result.back().mtlMerged = CloneMaterial(subsets[s].mtlSubSet);
          //
          SMergedMtlInfoToDispatch.src_rects.resize(1);
          SMergedMtlInfoToDispatch.src_rects[0] = AABB(Vec3(0,0,0),Vec3(1,1,0));
          for(int n=0;n<EFTT_MAX;n++)
          {
            if( SMergedMtlInfoToDispatch.src_rects.size()!=0 )
            {
              SMergedMtlInfoToDispatch.src_x[n].resize(1);
              SMergedMtlInfoToDispatch.src_x[n][0] = 0;
              SMergedMtlInfoToDispatch.src_y[n].resize(1);
              SMergedMtlInfoToDispatch.src_y[n][0] = 0;
              SMergedMtlInfoToDispatch.w[n].resize(1);
              SMergedMtlInfoToDispatch.w[n][0] = SMergedMtlInfoToDispatch.src_w[n];
              SMergedMtlInfoToDispatch.h[n].resize(1);
              SMergedMtlInfoToDispatch.h[n][0] = SMergedMtlInfoToDispatch.src_h[n];
              SMergedMtlInfoToDispatch.uv_min_x[n].resize(1);
              SMergedMtlInfoToDispatch.uv_min_x[n][0] = 0;
              SMergedMtlInfoToDispatch.uv_min_y[n].resize(1);
              SMergedMtlInfoToDispatch.uv_min_y[n][0] = 0;
              SMergedMtlInfoToDispatch.uv_max_x[n].resize(1);
              SMergedMtlInfoToDispatch.uv_max_x[n][0] = 1;
              SMergedMtlInfoToDispatch.uv_max_y[n].resize(1);
              SMergedMtlInfoToDispatch.uv_max_y[n][0] = 1;
              SMergedMtlInfoToDispatch.border[n].resize(1);
              SMergedMtlInfoToDispatch.border[n][0] = 0;
            }
          }
          bRes = Dispatch(SMergedMtlInfoToDispatch,subsets[s].result.back(),0,UseOneAtlassForAllChannels);
          if( !bRes )
          {
						CryWarning( VALIDATOR_MODULE_3DENGINE,VALIDATOR_ERROR,"MaterialMerge: Cannot add texture to atlas, should never happens" );
            return false;
          }
        }
      }
    }
  }
  //
  return true;
}

inline int LogBaseTwo(int iNum)
{
  int i, n;
  for(i = iNum-1, n = 0; i > 0; i >>= 1, n++ );
  return n;
}


struct SUVRectsFinder
{
  struct Edge
  {
    Edge()
    {
      v[0] = v[1] = -1;
      f[0] = f[1] = -1;
    };
    bool operator<(const Edge& other) const
    {
      if( v[0]<other.v[0] )
        return true;
      if( v[0]>other.v[0] )
        return false;
      if( v[1]<other.v[1] )
        return true;
      return false;
    }
    Vec2i v;
    Vec2i f;
  };
  struct Face
  {
    Face()
    {
      v[0] = v[1] = v[2] = -1;
      e[0] = e[1] = e[2] = -1;
      a[0] = a[1] = a[2] = -1;
      bProcessed = false;
    };
    bool bProcessed;
    Vec3i v;
    Vec3i a;
    Vec3i e;
  };
  void AddGeometry(const uint16* indicies,int fromIndex,int numIndicies)
  {
    size_t numFacesBefore = m_faces.size();
    m_faces.resize(numFacesBefore+numIndicies/3);
    for(int f=0;f<numIndicies/3;f++)
    {
      for(int i=0;i<3;i++)
      {
        m_faces[f].v[i] = indicies[fromIndex+f*3+i];
      }
    }
  }
public:
  bool Build(const Vec2* coords)
  {
    BuildAjacency();

    /*
    minVec = Vec2(FLT_MAX,FLT_MAX);
    maxVec = Vec2(FLT_MIN,FLT_MIN);

    for(int f=0;f<m_faces.size();f++)
    {
      for(int i=0;i<3;i++)
      {
        //
        float u = RenormalizeTexCoord(coords[m_faces[f].v[i]][0]);
        float v = RenormalizeTexCoord(coords[m_faces[f].v[i]][1]);
        // min
        minVec.x = min(minVec.x,u);
        minVec.y = min(minVec.y,v);
        // max
        maxVec.x = max(maxVec.x,u);
        maxVec.y = max(maxVec.y,v);
      }
    }
    */
    // iterate over 
    bool bProcessed = true;
    do 
    {
      bProcessed = true;
      //
      int n=0;
      for(;n<(int)m_faces.size();n++)
      {
        if( !m_faces[n].bProcessed )
        {
          bProcessed = false;
          break;
        }
      }
      if( !bProcessed )
      {
        // process face 
        AABB result(AABB::RESET);
        RecursiveProcess(coords,n,result,0);
        //
        m_subsets.push_back(result);
        collapseToNonOverlaping(m_subsets);
        //
      }
    } while(!bProcessed);
    //
    return true;
  }
  // check for overlapping and collapse them
  static void collapseToNonOverlaping(std::vector<AABB>& in)
  {
    // first pass - add overlapping only
    for(int s=0;s<(int)in.size()-1;s++)
    {
      for(int s1=s+1;s1<(int)in.size();s1++)
      {
        if( in[s].IsIntersectBox(in[s1]) )
        {
          in[s].Add(in[s1]);
          in.erase(in.begin()+s1);
          s = 0;
          break;
        }
      }
    }
  }
protected:
  void RecursiveProcess(const Vec2* coords,int f,AABB& result,int recLevel)
  {
    if( m_faces[f].bProcessed || recLevel>500 )
    {
      return;
    }
    //
    for(int i=0;i<3;i++)
    {
      result.Add(Vec3(RenormalizeTexCoord(coords[m_faces[f].v[i]][0]),RenormalizeTexCoord(coords[m_faces[f].v[i]][1]),0.0f));
    }
    m_faces[f].bProcessed = true;
    for(int i=0;i<3;i++)
    {
      int aj = m_faces[f].a[i];
      if( aj!=-1 )
      {
        RecursiveProcess(coords,aj,result,recLevel+1);
      }
    }
  }
  void BuildAjacency()
  {
    // build edges
    for(uint32 f=0;f<m_faces.size();f++)
    {
      for(int n=0;n<3;n++)
      {
        int v0 = m_faces[f].v[(n+0)%3];
        int v1 = m_faces[f].v[(n+1)%3];
        Edge edge;
        if( v0<v1 )
        {
          edge.v[0] = v0;
          edge.v[1] = v1;
        }
        else
        {
          edge.v[0] = v1;
          edge.v[1] = v0;
        }
        for(int fi=0;fi<2;fi++)
        {
          if( edge.f[fi]==f )
            break;
          if( edge.f[fi]==-1 )
          {
            edge.f[fi] = f;
            break;
          }
        }
				m_edges.insert(edge);
      }
    }
    // generate adjacencies
    int e=0;
    for(std::set<Edge>::iterator it = m_edges.begin();it!=m_edges.end();it++,e++)
    {
      // edges
      for(int f=0;f<2;f++)
      {
        int face = it->f[f];
        if( face==-1 )
          continue;
        //
        if( m_faces[face].e[0]!=e && m_faces[face].e[1]!=e && m_faces[face].e[2]!=e )
        {
          bool bFound = false;
          for(int a=0;a<3;a++)
          {
            if( m_faces[face].e[a]==-1 )
            {
              m_faces[face].e[a] = e;
              bFound = true;
              break;
            }
          }
          if( !bFound )
          {
            CryLog("SUVRectsFinder:Degenerated Face found");
          }
        }
      }
      // face ajacency
      if( it->f[0]==-1 || it->f[1]==-1)
        continue;
      //
      for(int f=0;f<2;f++)
      {
        int face = it->f[f];
        int faceAjac = it->f[(f+1)%2];
        //
        if( m_faces[face].a[0]!=faceAjac && m_faces[face].a[1]!=faceAjac && m_faces[face].a[2]!=faceAjac )
        {
          bool bFound = false;
          for(int a=0;a<3;a++)
          {
            if( m_faces[face].a[a]==-1 )
            {
              m_faces[face].a[a] = faceAjac;
              bFound = true;
              break;
            }
          }
          if( !bFound )
          {
            CryLog("SUVRectsFinder:Degenerated Face found");
          }
        }
      }
    }
  }
  int numVerts;
  std::set<Edge> m_edges;
  std::vector<Face> m_faces;
public:
  std::vector<AABB> m_subsets;
  Vec2 minVec;
  Vec2 maxVec;
};

inline int Clamp(int val,int min_val,int max_val)
{
  if( val<min_val )
    return min_val;
  else if( val>max_val )
    return max_val;
  return val;
}

IMergeMaterialsResult* C3DEngine::MergeMaterials(IMaterialMergePipe* pipe,IMaterialMergePipe::EMergeMaterialsFlags flags)
{
  std::auto_ptr<SMergeMaterialsResult> resultResult = std::auto_ptr<SMergeMaterialsResult>(new SMergeMaterialsResult());
  std::vector<SMergedSubset>& result=resultResult->mergedMtls;
  MergedData mergedData1;
  MergedData* mergedData = &mergedData1;
  // 1 step: create good subsets
  for(int n=0;n<pipe->GetMaterialsCount();n++)
  {
    bool bManaged = false;
    for(uint32 s=0;s<mergedData->subsets.size();s++)
    {
      if( CanBeMerged(mergedData->subsets[s].mtlSubSet,pipe->GetMaterial(n),(flags & IMaterialMergePipe::mfVerbose) ? true : false,flags) )
      {
        bManaged = true;
        break;
      }
    }
    if( !bManaged )
    {
      MergedDataSubSet subset;
      //
      subset.mtlSubSet = CloneMaterial(pipe->GetMaterial(n));
      //
      mergedData->subsets.push_back(subset);
    }
  }
  // 1a step: create good subsets
  for(int nn=0;nn<pipe->GetMaterialsCount();nn++)
  {
    bool bManaged = false;
    for(uint32 ss=0;ss<mergedData->subsets.size();ss++)
    {
      if( CanBeMerged(mergedData->subsets[ss].mtlSubSet,pipe->GetMaterial(nn),false,flags) )
      {
        // check for already existent
        bool bExistInThisSubset = false;
        for( uint32 i=0;i<mergedData->subsets[ss].info.size();i++)
        {
          if( mergedData->subsets[ss].info[i].mtlOrg==pipe->GetMaterial(nn) )
          {
            bExistInThisSubset = true;
            bManaged = true;
            break;
          }
        }
        if( !bExistInThisSubset )
        {
          SMergedMtlInfo info;
          info.mtlOrg = pipe->GetMaterial(nn);
          SShaderItem& si = info.mtlOrg->GetShaderItem(0);
          //
          // calculate source rect/s
          std::vector<AABB> subsets;
          if( flags & IMaterialMergePipe::mfOnlyUsedSpace )
          {
            int stride_index=0;
            int count_indicies = 0;
            int first_indicies = 0;
            int stride=0;
            int first_vertex = 0;
            int count = 0;
            int numSets = pipe->GetTextureCoordinatesSubsets(info.mtlOrg);
            if( numSets )
            {
              //
              //
              for(int s=0;s<numSets;s++)
              {
                SUVRectsFinder finder;
                const Vec2* coords = pipe->GetTextureCoordinates(s,info.mtlOrg,stride,first_vertex,count);
                const uint16* indicies = pipe->GetIndicies(s,info.mtlOrg,stride_index,first_indicies,count_indicies);
                finder.AddGeometry(indicies,first_indicies,count_indicies);
                finder.Build(coords);
                //minVec = finder.minVec;
                //maxVec = finder.maxVec;
                //
                subsets.insert(subsets.end(),finder.m_subsets.begin(),finder.m_subsets.end());
              }
              //
              SUVRectsFinder::collapseToNonOverlaping(subsets);
            }
          }
          else
          {
            subsets.push_back(AABB(Vec3(0,0,0),Vec3(1,1,0)));
          }
          bool bWasAdded = false;
          std::sort(subsets.begin(),subsets.end(),BiggerInFrontSubsets);
          info.src_rects = subsets;
          //
          for(int n=0;n<EFTT_MAX;n++)
          {
            //
            if( !si.m_pShaderResources->GetTexture(n) )
              continue;
            //
            if( si.m_pShaderResources->GetTexture(n)->m_Name.empty() )
              continue;
            //
            SEfResTexture* pTexture = si.m_pShaderResources->GetTexture(n);
            info.org_filename[n] = pTexture->m_Name;
            //
            STexSampler& sampler = pTexture->m_Sampler;
            ITexture* pTex = sampler.m_pITex;
            info.dds_filename[n] = pTex->GetName();
            // if some rect takes more then 80% use full texture
            bool bUseFullTexture = false;
            AABB aab(AABB::RESET);
            for(uint32 sr=0;sr<info.src_rects.size();sr++)
            {
              aab.Add(info.src_rects[sr]);
            }
            float area = aab.GetSize().x*aab.GetSize().y;
            if( area>0.8 )
              bUseFullTexture = true;
						if( (flags & IMaterialMergePipe::mfDoNotMergeTextures) || bUseFullTexture )
						{
							info.src_rects.resize(1);
							info.src_rects[0] = AABB(Vec3(0,0,0),Vec3(1,1,0));
							//
							info.uv_min_x[n].push_back(0.0f);
							info.uv_min_y[n].push_back(0.0f);
							info.uv_max_x[n].push_back(1.0f);
							info.uv_max_y[n].push_back(1.0f);
							//
							info.src_x[n].push_back(0);
							info.src_y[n].push_back(0);
              //
              int Width=0 ,Height=0;
              bool bRes = gEnv->pRenderer->GetImageCaps(info.dds_filename[n], Width ,Height);
              if( !bRes )
              {
                // for non dx9 and dx10 render this should fail
                CryLog("pRenderer->GetImageCaps returns false");
                return NULL;
              }
              //
              info.src_w[n] = Width;
              info.src_h[n] = Height;
              //
							info.w[n].push_back(Width);
							info.h[n].push_back(Height);
							bWasAdded = true;
						}
						else 
						{
							int Width=0 ,Height=0;
							bool bRes = gEnv->pRenderer->GetImageCaps(info.dds_filename[n], Width ,Height);
							if( !bRes )
							{
								// for non dx9 and dx10 render this should fail
								CryLog("pRenderer->GetImageCaps returns false");
								return NULL;
							}
							//
							info.src_w[n] = Width;
							info.src_h[n] = Height;
							//
							bWasAdded = true;
							//
							for(uint32 s=0;s<subsets.size();s++)
							{
								//
								int x1 = Clamp(AlignToLeft(subsets[s].min.x*(float)Width-3),0,Width);
								int y1 = Clamp(AlignToLeft(subsets[s].min.y*(float)Height-3),0,Height);
								int x2 = Clamp(AlignToRight(subsets[s].max.x*(float)Width+3),0,Width);
								int y2 = Clamp(AlignToRight(subsets[s].max.y*(float)Height+3),0,Height);
								//
								info.uv_min_x[n].push_back((float)x1/(float)Width);
								info.uv_min_y[n].push_back((float)y1/(float)Height);
								info.uv_max_x[n].push_back((float)x2/(float)Width);
								info.uv_max_y[n].push_back((float)y2/(float)Height);
								//
								info.src_x[n].push_back(x1);
								info.src_y[n].push_back(y1);
								info.w[n].push_back(x2-x1);
								info.h[n].push_back(y2-y1);
							}
						}
          }
          if( !bWasAdded )
          {
            info.src_rects.resize(0);
          }
          if( (flags & IMaterialMergePipe::mfDoNotMergeTextures) )
          {
            if( mergedData->subsets[ss].info.size()==0 )
              mergedData->subsets[ss].info.push_back(info);
          }
          else
          {
            mergedData->subsets[ss].info.push_back(info);
          }
          bManaged = true;
          break;
        }
      }
    }
    if( !bManaged )
    {
      CryLog("Should never happens");
      return NULL;
    }
  }
  // step2: collapse subsets textures
  bool bSolved = mergedData->Solve((flags & IMaterialMergePipe::mfUseDiffuseAtlas) ? true : false);
  //
  CryLog("Solver:%s Num Subsets:%d",bSolved ? "Solved" : "Failed", mergedData->subsets.size());
  //
  if( bSolved )
  {
    for(uint32 ss=0;ss<mergedData->subsets.size();ss++)
    {
      for(uint32 r=0;r<mergedData->subsets[ss].result.size();r++)
      {
        result.push_back(SMergedSubset());
        //
        int atlasToUse = 0;
        for(int n=0;n<EFTT_MAX;n++)
        {
          string out_filename = pipe->GetTexmapName(ss,r,n);//string().Format("c:\\MergeResult\\S!%03d_R!%03d_T!%03d%s.dds",s,r,n,n==EFTT_BUMP ? "_ddn" : "");
          //
          if( out_filename.size() )
          {
            atlasToUse = n;
            break;
          }
        }
        string out_filenames[EFTT_MAX];
        ResultSubset& resultSubset = mergedData->subsets[ss].result[r];
        //
        std::vector<const char*> filenames[EFTT_MAX];
        std::vector<int> offsetx[EFTT_MAX];
        std::vector<int> offsety[EFTT_MAX];
        std::vector<int> rect_width[EFTT_MAX];
        std::vector<int> rect_height[EFTT_MAX];
        std::vector<int> src_rect_width[EFTT_MAX];
        std::vector<int> src_rect_height[EFTT_MAX];
        std::vector<int> src_offsetx[EFTT_MAX];
        std::vector<int> src_offsety[EFTT_MAX];
        std::vector<int> src_tex_width[EFTT_MAX];
        std::vector<int> src_tex_height[EFTT_MAX];
        //
        for(int n=0;n<EFTT_MAX;n++)
        {
					string out_filename = pipe->GetTexmapName(ss,r,n);//string().Format("c:\\MergeResult\\S!%03d_R!%03d_T!%03d%s.dds",s,r,n,n==EFTT_BUMP ? "_ddn" : "");
					//
					int out_width = 0;
					int out_height = 0;
					//
					resultSubset.atlas[n].GetUsedCaps(out_width,out_height);
					out_width = 1<<(LogBaseTwo((uint16)out_width));
					out_height = 1<<(LogBaseTwo((uint16)out_height));
					//
					for(uint32 m=0;m<resultSubset.mtls.size();m++)
					{
						if( !resultSubset.mtls[m].dds_filename[n].empty() )
						{
							for(uint32 s=0;s<resultSubset.mtls[m].src_rects.size();s++)
							{
								filenames[n].push_back(resultSubset.mtls[m].dds_filename[n]);
								offsetx[n].push_back(resultSubset.mtls[m].dst_x[n][s]);
								offsety[n].push_back(resultSubset.mtls[m].dst_y[n][s]);
								rect_width[n].push_back(resultSubset.mtls[m].w[n][s]);
								rect_height[n].push_back(resultSubset.mtls[m].h[n][s]);
                src_rect_width[n].push_back(resultSubset.mtls[m].w[n][s]);
                src_rect_height[n].push_back(resultSubset.mtls[m].h[n][s]);
								src_offsetx[n].push_back(resultSubset.mtls[m].src_x[n][s]);
								src_offsety[n].push_back(resultSubset.mtls[m].src_y[n][s]);
                src_tex_width[n].push_back(resultSubset.mtls[m].src_w[n]);
                src_tex_height[n].push_back(resultSubset.mtls[m].src_h[n]);
							}
						}
					}
					if( filenames[n].size() )
					{
            std::set<string> uniq_files;
            for(uint32 f=0;f<filenames[n].size();f++)
            {
              uniq_files.insert(filenames[n][f]);
            }
						if( uniq_files.size()==1 )
						{
							// leave as it
							out_filenames[n] = filenames[n].front();
						}
						else
						{
              if( (flags & IMaterialMergePipe::mfUseDiffuseAtlas) )
              {
                if( n!=atlasToUse )
                {
                  int index = 0;
                  for(uint32 m=0;m<resultSubset.mtls.size();m++)
                  {
                    for(uint32 s=0;s<resultSubset.mtls[m].src_rects.size();s++)
                    {
                      resultSubset.atlas[atlasToUse].GetUsedCaps(out_width,out_height);
                      out_width = 1<<(LogBaseTwo((uint16)out_width));
                      out_height = 1<<(LogBaseTwo((uint16)out_height));
                      //
                      offsetx[n][index] = offsetx[atlasToUse][index];
                      offsety[n][index] = offsety[atlasToUse][index];
                      rect_width[n][index] = rect_width[atlasToUse][index];
                      rect_height[n][index] = rect_height[atlasToUse][index];
                      float ratiox = (float)src_tex_width[n][index]/(float)src_tex_width[atlasToUse][index];
                      float ratioy = (float)src_tex_height[n][index]/(float)src_tex_height[atlasToUse][index];
                      src_offsetx[n][index] = (int)((float)src_offsetx[atlasToUse][index]*ratiox);
                      src_offsety[n][index] = (int)((float)src_offsety[atlasToUse][index]*ratioy);
                      //
                      src_rect_width[n][index] = (int)((float)src_rect_width[atlasToUse][index]*ratiox);
                      src_rect_height[n][index] = (int)((float)src_rect_height[atlasToUse][index]*ratioy);
                      //
                      //src_tex_width[n][index] = (float)src_tex_width[atlasToUse][index]*ratiox;
                      //src_tex_height[n][index] = (float)src_tex_height[atlasToUse][index]*ratioy;
                      index++;
                    }
                  }
                }
              }
							// OPTIMIZE ME: collapse equal textures
							if( gEnv->pRenderer->MergeImages(out_filename,out_width,out_height,&filenames[n][0],&offsetx[n][0],&offsety[n][0],&rect_width[n][0],&rect_height[n][0],&src_offsetx[n][0],&src_offsety[n][0],&src_rect_width[n][0],&src_rect_height[n][0],filenames[n].size()) )
							{
								out_filenames[n] = out_filename;
							}
							else
							{
								return NULL;
							}
						}
						result.back().tex_w[n] = out_width;
						result.back().tex_h[n] = out_height;
					}
				}
        // reassign textures
        SShaderItem& si = resultSubset.mtlMerged->GetShaderItem(0);
        for(int n=0;n<EFTT_MAX;n++)
        {
          SEfResTexture* pTexture = si.m_pShaderResources->GetTexture(n);
          if( pTexture && !out_filenames[n].empty())
          {
            pTexture->m_Name = out_filenames[n];
          }
        }
        result.back().mtlMerged = resultSubset.mtlMerged;
        result.back().originalMaterials = resultSubset.mtls;
      }
    }
  }
  //
  return resultResult.release();
}

#include UNIQUE_VIRTUAL_WRAPPER(IMergePipeBackend)
#include UNIQUE_VIRTUAL_WRAPPER(IMaterialMergePipe)
#include UNIQUE_VIRTUAL_WRAPPER(IMergePipe)