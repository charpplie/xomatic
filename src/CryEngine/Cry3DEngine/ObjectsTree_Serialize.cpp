#include "StdAfx.h"

#include "VoxMan.h"
#include "Brush.h"
#include "RoadRenderNode.h"
#include "DecalRenderNode.h"
#include "WaterVolumeRenderNode.h"
#include "DistanceCloudRenderNode.h"
#include "WaterWaveRenderNode.h"
#include "AutoCubeMapRenderNode.h"
#include "IrradianceVolumeRenderNode.h"
#include "VisAreas.h"
#include "Brush.h"




#pragma warning(disable: 4244)

CMemoryBlock COctreeNode::m_mbLoadingCache;

#pragma pack(push,4)

// common node data
struct SRenderNodeChunk
{
  SRenderNodeChunk() { m_nObjectTypeID = 0; }
	AABB          m_WSBBox;
	float         m_fWSMaxViewDist;
	uint32        m_dwRndFlags;
  uint16        m_nObjectTypeID;
	uint8         m_ucViewDistRatio;
  uint8         m_ucLodRatio;

	AUTO_STRUCT_INFO_LOCAL
};

struct SVegetationChunk : public SRenderNodeChunk
{
  Vec3  m_vPos;
  float m_fScale;
  uint8 m_ucBright;
  uint8 m_ucAngle;

  AUTO_STRUCT_INFO_LOCAL
};

struct SVegetationChunkEx : public SVegetationChunk
{
  uint8 m_normalX;
  uint8 m_normalY;
  uint32 m_HMAIndex;

  AUTO_STRUCT_INFO_LOCAL
};

struct SBrushChunk : public SRenderNodeChunk
{
  SBrushChunk() { m_nMergeGroupId=0; }
	Matrix34  m_Matrix;
	int32     m_nMergeGroupId;
  int32     m_nMaterialId;
  int32     m_nMaterialLayers;

	AUTO_STRUCT_INFO_LOCAL
};

struct SRoadChunk : public SRenderNodeChunk
{
	int32   m_nVertsNum;
  int32   m_nSortPriority;
  int32   m_nMaterialId;
	float   m_arrTexCoors[2];
	float   m_arrTexCoorsGlobal[2];

	AUTO_STRUCT_INFO_LOCAL
};

struct SVoxelObjectChunkVer3 : public SRenderNodeChunk
{
	SVoxelChunkVer3 m_VoxData;
	Matrix34 m_Matrix;

	AUTO_STRUCT_INFO_LOCAL
};

struct SVoxelObjectChunkVer4 : public SRenderNodeChunk
{
  SVoxelChunkVer4 m_VoxData;
  Matrix34 m_Matrix;

  AUTO_STRUCT_INFO_LOCAL
};

struct SDecalChunk : public SRenderNodeChunk
{
	int16     m_projectionType; 
  uint8     m_deferred; 
  uint8     m_notused; 
	Vec3      m_pos;
	Vec3      m_normal;
	Matrix33  m_explicitRightUpFront;
	f32       m_radius;
	int32     m_nMaterialId;
  int32     m_nSortPriority;

	AUTO_STRUCT_INFO_LOCAL
};

struct SWaterVolumeChunk : public SRenderNodeChunk
{
	// volume type and id
	int32 m_volumeTypeAndMiscBits;
	uint64 m_volumeID;

	// material
	int32 m_materialID;	

	// fog properties
	f32 m_fogDensity;
	Vec3 m_fogColor;
	Plane m_fogPlane;

	// render geometry 
	f32 m_uTexCoordBegin;
	f32 m_uTexCoordEnd;
	f32 m_surfUScale;
	f32 m_surfVScale;
	uint32 m_numVertices;

	// physics properties
	f32 m_volumeDepth;
	f32 m_streamSpeed;
	uint32 m_numVerticesPhysAreaContour;

	AUTO_STRUCT_INFO_LOCAL
};

struct SWaterVolumeVertex
{
	Vec3 m_xyz;
	
	AUTO_STRUCT_INFO_LOCAL
};

struct SDistanceCloudChunk : public SRenderNodeChunk
{
	Vec3 m_pos;
	f32 m_sizeX;
	f32 m_sizeY;
	f32 m_rotationZ;
	int32 m_materialID;

	AUTO_STRUCT_INFO_LOCAL
};

struct SWaterWaveChunk : public SRenderNodeChunk
{
	int32 m_nID;

	// Geometry properties
	Matrix34 m_pWorldTM;
	uint32 m_nVertexCount;

	f32 m_fUScale;
	f32 m_fVScale;

	// Material
	int32 m_nMaterialID;


	// Animation properties

	f32 m_fSpeed;
	f32 m_fSpeedVar;

	f32 m_fLifetime;
	f32 m_fLifetimeVar;  

	f32 m_fPosVar;

	f32 m_fHeight;
	f32 m_fHeightVar;

	f32 m_fCurrLifetime;  
	f32 m_fCurrFrameLifetime;
	f32 m_fCurrSpeed;
	f32 m_fCurrHeight;

	AUTO_STRUCT_INFO_LOCAL
};

struct SAutoCubeMapChunk : public SRenderNodeChunk
{
	uint32 m_flags;

	Quat m_oobRot;
	Vec3 m_oobH;
	Vec3 m_oobC;

	Vec3 m_refPos;
	
	AUTO_STRUCT_INFO_LOCAL
};

struct SIrradianceVolumeChunk : public SRenderNodeChunk
{
	Matrix34 m_WorldMatrix;
	float		 m_fDensity;
	uint32	 m_bIsSpecular;
	AUTO_STRUCT_INFO_LOCAL
};

#pragma pack(pop)

AUTO_TYPE_INFO(EERType)

#include "TypeInfo_impl.h"
#include "ObjectsTree_Serialize_info.h"

inline void CopyCommonData(SRenderNodeChunk * pChunk, IRenderNode * pObj)
{
  pChunk->m_WSBBox = pObj->GetBBox();
  COPY_MEMBER_SAVE(pChunk,pObj,m_fWSMaxViewDist);
  COPY_MEMBER_SAVE(pChunk,pObj,m_dwRndFlags); 
  COPY_MEMBER_SAVE(pChunk,pObj,m_ucViewDistRatio);
  COPY_MEMBER_SAVE(pChunk,pObj,m_ucLodRatio);
}

inline void LoadCommonData(SRenderNodeChunk * pChunk, IRenderNode * pObj)
{
  COPY_MEMBER_LOAD(pObj,pChunk,m_fWSMaxViewDist);
  COPY_MEMBER_LOAD(pObj,pChunk,m_dwRndFlags); 
  COPY_MEMBER_LOAD(pObj,pChunk,m_ucViewDistRatio);
  COPY_MEMBER_LOAD(pObj,pChunk,m_ucLodRatio);
  pObj->m_dwRndFlags &= ~(ERF_HIDDEN|ERF_SELECTED);
}

//////////////////////////////////////////////////////////////////////////
int COctreeNode::SaveObjects(CMemoryBlock * pMemBlock, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, const SHotUpdateInfo * pExportInfo)
{
  uint32 nObjTypeMask = pExportInfo ? pExportInfo->nObjTypeMask : (uint32)~0;

  int nBlockSize=0;

//  FreeAreaBrushes();

  for(int l=0; l<eRNListType_ListsNum; l++)
  for(IRenderNode * pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
  {
    IRenderNode * pRenderNode = pObj;
    EERType eType = pRenderNode->GetRenderNodeType();

    if(!(nObjTypeMask & (1<<eType)))
      continue;

    if(eType == eERType_Vegetation && !(pRenderNode->GetRndFlags()&ERF_PROCEDURAL) && GetCVars()->e_VoxTerVegetationEx)
    {
      nBlockSize += sizeof(eType);
      nBlockSize += sizeof(SVegetationChunkEx);
    }
    else if(eType == eERType_Vegetation && !(pRenderNode->GetRndFlags()&ERF_PROCEDURAL))
    {
      nBlockSize += sizeof(eType);
      nBlockSize += sizeof(SVegetationChunk);
    }
    else if(eType == eERType_Brush && !(pRenderNode->GetRndFlags()&ERF_MERGE_RESULT) && !(GetCVars()->e_VoxTer && pRenderNode->GetIntegrationType()))
    {
      nBlockSize += sizeof(eType);
      nBlockSize += sizeof(SBrushChunk);
    }
    else if(eType == eERType_VoxelObject && ((CVoxelObject*)pRenderNode)->GetRenderMesh(0) && ((CVoxelObject*)pRenderNode)->GetRenderMesh(0)->GetIndicesCount())
    {
      nBlockSize += sizeof(eType);
      nBlockSize += sizeof(SVoxelObjectChunkVer4);
      nBlockSize += ((CVoxelObject*)pRenderNode)->GetCompiledMeshDataSize();
    }
    else if(eType == eERType_Decal)
    {
      nBlockSize += sizeof(eType);
      nBlockSize += sizeof(SDecalChunk);
    }
    else if(eType == eERType_WaterVolume)
    {
      CWaterVolumeRenderNode* pWVRN( static_cast< CWaterVolumeRenderNode* >( pRenderNode ) );
      const SWaterVolumeSerialize* pSerParams( pWVRN->GetSerializationParams() );
      if( pSerParams )
      {
        nBlockSize += sizeof( eType );
        nBlockSize += sizeof( SWaterVolumeChunk );
        nBlockSize += ( pSerParams->m_vertices.size() + pSerParams->m_physicsAreaContour.size() ) * sizeof( SWaterVolumeVertex );
      }
    }
    else if(eType == eERType_Road)
    {
      nBlockSize += sizeof(eType);
      nBlockSize += sizeof(SRoadChunk);
      nBlockSize += ((CRoadRenderNode*)pRenderNode)->m_arrVerts.GetDataSize();
    }
    else if(eType == eERType_DistanceCloud)
    {
      nBlockSize += sizeof(eType);
      nBlockSize += sizeof(SDistanceCloudChunk);
    }
    else if(eType == eERType_WaterWave)
    {
      CWaterWaveRenderNode* pWaveRenderNode( static_cast< CWaterWaveRenderNode* >( pRenderNode ) );
      const SWaterWaveSerialize* pSerParams( pWaveRenderNode->GetSerializationParams() );

      nBlockSize += sizeof( eType );
      nBlockSize += sizeof( SWaterWaveChunk );

      nBlockSize += ( pSerParams->m_pVertices.size() ) * sizeof( Vec3 );
    }
    else if(eType == eERType_AutoCubeMap)
    {
      nBlockSize += sizeof(eType);
      nBlockSize += sizeof(SAutoCubeMapChunk);
    }
		else if(eType == eERType_IrradianceVolume)
		{
			CIrradianceVolumeRenderNode* pIrradianceVolumeNode((CIrradianceVolumeRenderNode*)pRenderNode);
			if(pIrradianceVolumeNode && pIrradianceVolumeNode->m_pRE
				&& !(pIrradianceVolumeNode->m_pRE->GetFlags() & CREIrradianceVolume::efGIVolume))
			{
				nBlockSize += sizeof(eType);
				nBlockSize += sizeof(SIrradianceVolumeChunk);
			}
		}

    // align to 4
    while(uint32(nBlockSize)&3)
      nBlockSize++;
  }

  if(!pMemBlock)
    return nBlockSize;

  pMemBlock->Allocate(nBlockSize);
  byte * pPtr = (byte *)pMemBlock->GetData();

  int nDatanSize = nBlockSize;

  for(int l=0; l<eRNListType_ListsNum; l++)
  for(IRenderNode * pRenderNode = m_arrObjects[l].m_pFirstNode; pRenderNode; pRenderNode = pRenderNode->m_pNext)
  {
    IRenderNode * pEnt = pRenderNode;
    EERType eType = pEnt->GetRenderNodeType();

    if(!(nObjTypeMask & (1<<eType)))
      continue;

    if(eType == eERType_Brush && !(pEnt->GetRndFlags()&ERF_MERGE_RESULT) && !(GetCVars()->e_VoxTer && pEnt->GetIntegrationType()))
    {
      AddToPtr(pPtr, nDatanSize, eType, eEndian);

      CBrush * pObj = (CBrush*)pEnt;

      SBrushChunk chunk;

      CopyCommonData(&chunk, pObj);

      COPY_MEMBER_SAVE(&chunk,pObj,m_nMaterialLayers);          

      // brush data
      COPY_MEMBER_SAVE(&chunk,pObj,m_Matrix);

      chunk.m_nMaterialId = CObjManager::GetItemId(pMatTable, pObj->GetMaterial());

      COPY_MEMBER_SAVE(&chunk,pObj,m_nMergeGroupId);

      { // find StatObj Id
        int nItemId = CObjManager::GetItemId<IStatObj>(pStatObjTable, pObj->GetStatObj());
        if(nItemId<0)
          assert(!"StatObj not found in BrushTable on exporting");
        chunk.m_nObjectTypeID = nItemId;
      }

      AddToPtr(pPtr, nDatanSize,chunk,eEndian);
    }
    else if(eType == eERType_Vegetation  && !(pEnt->GetRndFlags()&ERF_PROCEDURAL) && GetCVars()->e_VoxTerVegetationEx)
    {
      AddToPtr(pPtr, nDatanSize,eType,eEndian);

      CVegetation * pObj = (CVegetation*)pEnt;

      SVegetationChunkEx chunk;

      CopyCommonData(&chunk, pObj);

      // vegetation data
      COPY_MEMBER_SAVE(&chunk,pObj,m_vPos);
      chunk.m_fScale = pObj->GetScale();
      COPY_MEMBER_SAVE(&chunk,pObj,m_nObjectTypeID);
      COPY_MEMBER_SAVE(&chunk,pObj,m_ucAngle);

      if(!GetCVars()->e_VoxTer || m_bEditor)
        pObj->CalcTerrainAdaption();

      chunk.m_normalX = pObj->m_alignNormal[0];
      chunk.m_normalY = pObj->m_alignNormal[1];
      chunk.m_HMAIndex = pObj->m_HMAIndex;

      AddToPtr(pPtr, nDatanSize,chunk,eEndian);
    }
    else if(eType == eERType_Vegetation  && !(pEnt->GetRndFlags()&ERF_PROCEDURAL))
    {
      AddToPtr(pPtr, nDatanSize,eType,eEndian);

      CVegetation * pObj = (CVegetation*)pEnt;

      SVegetationChunk chunk;

      CopyCommonData(&chunk, pObj);

      // vegetation data
      COPY_MEMBER_SAVE(&chunk,pObj,m_vPos);
      chunk.m_fScale = pObj->GetScale();
      COPY_MEMBER_SAVE(&chunk,pObj,m_nObjectTypeID);
      COPY_MEMBER_SAVE(&chunk,pObj,m_ucAngle);

      AddToPtr(pPtr, nDatanSize,chunk,eEndian);
    }
    else if(eType == eERType_VoxelObject && ((CVoxelObject*)pEnt)->GetRenderMesh(0) && ((CVoxelObject*)pEnt)->GetRenderMesh(0)->GetIndicesCount())
    {
      AddToPtr(pPtr, nDatanSize,eType,eEndian);

      CVoxelObject * pObj = (CVoxelObject*)pEnt;

      PrintMessage("Exporting compiled voxel object (%d,%d,%d) ...", 
        (int)pObj->GetPos(true).x, (int)pObj->GetPos(true).y, (int)pObj->GetPos(true).z);

      SVoxelObjectChunkVer4 * pChunk = new SVoxelObjectChunkVer4;

      CopyCommonData(pChunk, pObj);

      // voxel data
      COPY_MEMBER_SAVE(pChunk,pObj,m_Matrix);
      SVoxelChunkVer3* pDataChunk = new SVoxelChunkVer3;
      pObj->GetData(*pDataChunk,Vec3(0,0,0));

      pChunk->m_VoxData.nChunkVersion = 4;
      pChunk->m_VoxData.vOrigin = pDataChunk->vOrigin;
      pChunk->m_VoxData.vSize = pDataChunk->vSize;
      pChunk->m_VoxData.nFlags = pDataChunk->nFlags;
      memcpy(pChunk->m_VoxData.m_arrSurfaceNames,pDataChunk->m_arrSurfaceNames,sizeof(pChunk->m_VoxData.m_arrSurfaceNames));

      if(pObj->GetCompiledMeshDataSize())
        pChunk->m_VoxData.nFlags |= IVOXELOBJECT_FLAG_COMPILED;
      pChunk->m_VoxData.nFlags |= IVOXELOBJECT_FLAG_DATA_IS_ALLIGNED;

      AddToPtr(pPtr, nDatanSize,*pChunk,eEndian);

      // compiled mesh
      if(pChunk->m_VoxData.nFlags & IVOXELOBJECT_FLAG_COMPILED)
      {
        int nMeshDataSize=0;
        byte * pMesh = pObj->GetCompiledMeshData(nMeshDataSize, eEndian);
        memcpy(pPtr,pMesh,nMeshDataSize); 
        pPtr += nMeshDataSize; 
        nDatanSize -= nMeshDataSize;
        delete[] pMesh;
      }

      SAFE_DELETE(pChunk);
      SAFE_DELETE(pDataChunk);

      PrintMessagePlus(" ok");
    }
    else if(eType == eERType_Road)
    {
      AddToPtr(pPtr, nDatanSize,eType,eEndian);

      CRoadRenderNode * pObj = (CRoadRenderNode*)pEnt;

      SRoadChunk chunk;

      CopyCommonData(&chunk, pObj);

      // road data
      chunk.m_nMaterialId = CObjManager::GetItemId(pMatTable,pObj->GetMaterial());
      chunk.m_nSortPriority = pObj->m_sortPrio;

      chunk.m_nVertsNum = pObj->m_arrVerts.Count();

      for(int i=0; i<2; i++)
        COPY_MEMBER_SAVE(&chunk,pObj,m_arrTexCoors[i]);

      for(int i=0; i<2; i++)
        COPY_MEMBER_SAVE(&chunk,pObj,m_arrTexCoorsGlobal[i]);

      AddToPtr(pPtr, nDatanSize,chunk,eEndian);

      AddToPtr(pPtr, nDatanSize,pObj->m_arrVerts.GetElements(),pObj->m_arrVerts.Count(),eEndian);
    }
    else if( eERType_Decal == eType )
    {					
      AddToPtr(pPtr, nDatanSize,eType,eEndian);

      CDecalRenderNode* pObj( (CDecalRenderNode*) pEnt );

      SDecalChunk chunk;

      CopyCommonData(&chunk, pObj);

      // decal properties
      const SDecalProperties* pDecalProperties( pObj->GetDecalProperties() );
      assert( pDecalProperties );
      chunk.m_projectionType = pDecalProperties->m_projectionType;
      chunk.m_deferred = pDecalProperties->m_deferred;
      chunk.m_pos = pDecalProperties->m_pos;
      chunk.m_normal = pDecalProperties->m_normal;
      chunk.m_explicitRightUpFront = pDecalProperties->m_explicitRightUpFront;
      chunk.m_radius = pDecalProperties->m_radius;
      //chunk.m_materialID = pObj->GetMaterialID();

      chunk.m_nMaterialId = CObjManager::GetItemId(pMatTable,pObj->GetMaterial());
      chunk.m_nSortPriority = pDecalProperties->m_sortPrio;

      AddToPtr(pPtr, nDatanSize,chunk,eEndian);
    }
    else if( eERType_WaterVolume == eType )
    {
      CWaterVolumeRenderNode* pObj( (CWaterVolumeRenderNode*) pEnt );

      // get access to serialization parameters
      const SWaterVolumeSerialize* pSerData( pObj->GetSerializationParams() );
      assert( pSerData ); // trying to save level outside the editor?

      // save type
      AddToPtr(pPtr, nDatanSize,eType,eEndian);

      // save node data
      SWaterVolumeChunk chunk;

      CopyCommonData(&chunk, pObj);

			chunk.m_volumeTypeAndMiscBits = (pSerData->m_volumeType & 0xFFFF) | ((pSerData->m_capFogAtVolumeDepth ? 1 : 0) << 16);
      chunk.m_volumeID = pSerData->m_volumeID;

      chunk.m_materialID = CObjManager::GetItemId(pMatTable,pSerData->m_pMaterial);

      chunk.m_fogDensity = pSerData->m_fogDensity;
      chunk.m_fogColor = pSerData->m_fogColor;
      chunk.m_fogPlane = pSerData->m_fogPlane;

      chunk.m_uTexCoordBegin = pSerData->m_uTexCoordBegin;
      chunk.m_uTexCoordEnd = pSerData->m_uTexCoordEnd;
      chunk.m_surfUScale = pSerData->m_surfUScale;
      chunk.m_surfVScale = pSerData->m_surfVScale;
      chunk.m_numVertices = pSerData->m_vertices.size();

      chunk.m_volumeDepth = pSerData->m_volumeDepth;
      chunk.m_streamSpeed = pSerData->m_streamSpeed;
      chunk.m_numVerticesPhysAreaContour = pSerData->m_physicsAreaContour.size();

      AddToPtr(pPtr, nDatanSize,chunk,eEndian);

      // save vertices
      for( size_t i( 0 ); i < pSerData->m_vertices.size(); ++i )
      {
        SWaterVolumeVertex v;
        v.m_xyz = pSerData->m_vertices[i];

        AddToPtr(pPtr, nDatanSize, v, eEndian);
      }

      // save physics area contour vertices
      for( size_t i( 0 ); i < pSerData->m_physicsAreaContour.size(); ++i )
      {
        SWaterVolumeVertex v;
        v.m_xyz = pSerData->m_physicsAreaContour[i];

        AddToPtr(pPtr, nDatanSize, v, eEndian);
      }
    }
    else if (eERType_DistanceCloud == eType)
    {					
      AddToPtr(pPtr, nDatanSize, eType, eEndian);

      CDistanceCloudRenderNode* pObj((CDistanceCloudRenderNode*)pEnt);

      SDistanceCloudChunk chunk;

      CopyCommonData(&chunk, pObj);

      // distance cloud properties
      SDistanceCloudProperties properties(pObj->GetProperties());
      chunk.m_pos = properties.m_pos;
      chunk.m_sizeX = properties.m_sizeX;
      chunk.m_sizeY = properties.m_sizeY;
      chunk.m_rotationZ = properties.m_rotationZ;
      chunk.m_materialID = CObjManager::GetItemId(pMatTable,pObj->GetMaterial());

      AddToPtr(pPtr, nDatanSize, chunk, eEndian);
    }
    else if( eERType_WaterWave == eType )
    {
      CWaterWaveRenderNode* pObj( (CWaterWaveRenderNode*) pEnt );

      // get access to serialization parameters
      const SWaterWaveSerialize* pSerData( pObj->GetSerializationParams() );
      assert( pSerData ); // trying to save level outside the editor?

      // save type
      AddToPtr(pPtr, nDatanSize,eType,eEndian);

      SWaterWaveChunk chunk;

      CopyCommonData(&chunk, pObj);

      chunk.m_nID = (int32)pSerData->m_nID;
      chunk.m_nMaterialID = CObjManager::GetItemId(pMatTable, pSerData->m_pMaterial);

      chunk.m_fUScale = pSerData->m_fUScale;
      chunk.m_fVScale = pSerData->m_fVScale;

      chunk.m_nVertexCount = pSerData->m_nVertexCount;

      chunk.m_pWorldTM = pSerData->m_pWorldTM;

      chunk.m_fSpeed = pSerData->m_pParams.m_fSpeed;
      chunk.m_fSpeedVar = pSerData->m_pParams.m_fSpeedVar;
      chunk.m_fLifetime = pSerData->m_pParams.m_fLifetime;
      chunk.m_fLifetimeVar = pSerData->m_pParams.m_fLifetimeVar;
      chunk.m_fPosVar = pSerData->m_pParams.m_fPosVar;
      chunk.m_fHeight = pSerData->m_pParams.m_fHeight;
      chunk.m_fHeightVar = pSerData->m_pParams.m_fHeightVar;

      chunk.m_fCurrLifetime = pSerData->m_pParams.m_fCurrLifetime;          
      chunk.m_fCurrFrameLifetime = pSerData->m_pParams.m_fCurrFrameLifetime;
      chunk.m_fCurrSpeed = pSerData->m_pParams.m_fCurrSpeed;
      chunk.m_fCurrHeight = pSerData->m_pParams.m_fCurrHeight;

      AddToPtr(pPtr, nDatanSize, chunk, eEndian);

      // save vertices
      for( size_t i( 0 ); i < pSerData->m_nVertexCount; ++i )
      {
        Vec3 vPos( pSerData->m_pVertices[i] );
        AddToPtr(pPtr, nDatanSize, vPos, eEndian);
      }
    }
    else if (eERType_AutoCubeMap == eType)
    {					
      AddToPtr(pPtr, nDatanSize, eType, eEndian);

      CAutoCubeMapRenderNode* pObj((CAutoCubeMapRenderNode*)pEnt);

      SAutoCubeMapChunk chunk;

      CopyCommonData(&chunk, pObj);

      // auto cube map properties
      SAutoCubeMapProperties properties(pObj->GetProperties());
      chunk.m_flags = properties.m_flags;
      chunk.m_oobRot = Quat(properties.m_obb.m33);
      chunk.m_oobH = properties.m_obb.h;
      chunk.m_oobC = properties.m_obb.c;
      chunk.m_refPos = properties.m_refPos;

      AddToPtr(pPtr, nDatanSize, chunk, eEndian);
    }
		else if(eERType_IrradianceVolume == eType)
		{
			CIrradianceVolumeRenderNode* pObj((CIrradianceVolumeRenderNode*)pEnt);
			if(pObj && pObj->m_pRE && !(pObj->m_pRE->GetFlags() & CREIrradianceVolume::efGIVolume))	// check if it's not a GI special volume
			{
				AddToPtr(pPtr, nDatanSize, eType, eEndian);

				SIrradianceVolumeChunk chunk;
				CopyCommonData(&chunk, pObj);
				pObj->GetMatrix(chunk.m_WorldMatrix);
				chunk.m_fDensity = pObj->GetDensity();
				chunk.m_bIsSpecular = pObj->IsSpecularEnabled() ? 1 : 0;
				AddToPtr(pPtr, nDatanSize, chunk, eEndian);
			}
		}

    // align to 4
    while(uint32(pPtr)&3)
    {
      byte emptyByte = 222;
      memcpy(pPtr,&emptyByte,sizeof(emptyByte)); 
      pPtr += sizeof(emptyByte);
      nDatanSize -= sizeof(emptyByte);
    }
  }

  assert(pPtr == (byte *)pMemBlock->GetData()+nBlockSize);
  assert(nDatanSize == 0);

  return nBlockSize;
}

//////////////////////////////////////////////////////////////////////////
int COctreeNode::LoadObjects(byte * pPtr, byte * pEndPtr, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian)
{
	int nBlockSize=0;

	while(pPtr < pEndPtr)
	{
		EERType eType = *StepData<EERType>(pPtr,eEndian);

		// For these structures, our Endian swapping is built in to the member copy.
		if(eType == eERType_Brush) 
		{
			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, 0, "Brush object");

			SBrushChunk * pChunk = StepData<SBrushChunk>(pPtr,eEndian);

			if (!CheckRenderFlagsMinSpec(pChunk->m_dwRndFlags))
				continue;

			CBrush * pObj = new CBrush();

			// common node data
			COPY_MEMBER_LOAD(pObj,pChunk,m_WSBBox);
      LoadCommonData(pChunk, pObj);

			// brush data
      COPY_MEMBER_LOAD(pObj,pChunk,m_nMaterialLayers);
			COPY_MEMBER_LOAD(pObj,pChunk,m_Matrix);
			COPY_MEMBER_LOAD(pObj,pChunk,m_nMergeGroupId);

      pObj->SetStatObj(CObjManager::GetItemPtr(pStatObjTable, (int)pChunk->m_nObjectTypeID));

			// set material
      pObj->SetMaterial(CObjManager::GetItemPtr(pMatTable, pChunk->m_nMaterialId));

			//to get correct indirect lighting the registration must be done before checking if this object is inside a VisArea
			Get3DEngine()->RegisterEntity(pObj);

			pObj->Physicalize();
		}
    else if(eType == eERType_Vegetation && GetCVars()->e_VoxTerVegetationEx)
    {
      MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, 0, "Vegetation object");
      SVegetationChunkEx * pChunk = StepData<SVegetationChunkEx>(pPtr,eEndian);

      if (!CheckRenderFlagsMinSpec(GetObjManager()->m_lstStaticTypes[pChunk->m_nObjectTypeID].m_dwRndFlags))
        continue;

      StatInstGroup * pGroup = &GetObjManager()->m_lstStaticTypes[pChunk->m_nObjectTypeID];
      if(!pGroup->GetStatObj() || (pGroup->GetStatObj()->GetRadius()*pChunk->m_fScale < GetCVars()->e_VegetationMinSize))
        continue; // skip creation of very small objects

      CVegetation * pObj = new CVegetation();

      // common node data
      LoadCommonData(pChunk, pObj);

      // vegetation data
      COPY_MEMBER_LOAD(pObj,pChunk,m_vPos);

      pObj->SetScale(pChunk->m_fScale);

      COPY_MEMBER_LOAD(pObj,pChunk,m_nObjectTypeID);
      COPY_MEMBER_LOAD(pObj,pChunk,m_ucAngle);

      pObj->SetBBox(pChunk->m_WSBBox);

      pObj->m_alignNormal[0] = pChunk->m_normalX;
      pObj->m_alignNormal[1] = pChunk->m_normalY;
      pObj->m_HMAIndex = pChunk->m_HMAIndex;

      pObj->Physicalize();
      Get3DEngine()->RegisterEntity(pObj);

      //pObj->CalcTerrainAdaption();

      assert(!(pObj->GetRndFlags()&ERF_PROCEDURAL));
    }
    else if(eType == eERType_Vegetation)
    {
      MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, 0, "Vegetation object");

      SVegetationChunk * pChunk = StepData<SVegetationChunk>(pPtr,eEndian);

      if (!CheckRenderFlagsMinSpec(GetObjManager()->m_lstStaticTypes[pChunk->m_nObjectTypeID].m_dwRndFlags))
        continue;

      StatInstGroup * pGroup = &GetObjManager()->m_lstStaticTypes[pChunk->m_nObjectTypeID];
      if(!pGroup->GetStatObj() || (pGroup->GetStatObj()->GetRadius()*pChunk->m_fScale < GetCVars()->e_VegetationMinSize))
        continue; // skip creation of very small objects

      CVegetation * pObj = new CVegetation();

      // common node data
      LoadCommonData(pChunk, pObj);

      // vegetation data
      COPY_MEMBER_LOAD(pObj,pChunk,m_vPos);

      pObj->SetScale(pChunk->m_fScale);

      COPY_MEMBER_LOAD(pObj,pChunk,m_nObjectTypeID);
      COPY_MEMBER_LOAD(pObj,pChunk,m_ucAngle);

      pObj->SetBBox(pChunk->m_WSBBox);

      pObj->Physicalize();
      Get3DEngine()->RegisterEntity(pObj);

      //pObj->CalcTerrainAdaption();

      assert(!(pObj->GetRndFlags()&ERF_PROCEDURAL));
    }
		else if(eType == eERType_VoxelObject)
		{
			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, 0, "Voxel object");

			int nVersion = ((SVoxelObjectChunkVer3*)pPtr)->m_VoxData.nChunkVersion;
			SwapEndian(nVersion, eEndian);

			SVoxelObjectChunkVer3 * pChunkVer3 = new SVoxelObjectChunkVer3();
      SVoxelObjectChunkVer3 & chunkVer3 = *pChunkVer3;
      if(nVersion == 4)
      {
        SVoxelObjectChunkVer4 * pChunk = StepData<SVoxelObjectChunkVer4>(pPtr,eEndian);
        SVoxelObjectChunkVer4 & chunk = *pChunk;

        chunkVer3.m_WSBBox = chunk.m_WSBBox;
        chunkVer3.m_fWSMaxViewDist = chunk.m_fWSMaxViewDist;
        chunkVer3.m_dwRndFlags = chunk.m_dwRndFlags; 
        chunkVer3.m_ucViewDistRatio = chunk.m_ucViewDistRatio;
        chunkVer3.m_ucLodRatio = chunk.m_ucLodRatio;
        chunkVer3.m_Matrix = chunk.m_Matrix;

        memcpy(chunkVer3.m_VoxData.m_arrSurfaceNames,chunk.m_VoxData.m_arrSurfaceNames,sizeof(chunkVer3.m_VoxData.m_arrSurfaceNames));

        chunkVer3.m_VoxData.nChunkVersion = 3;
        chunkVer3.m_VoxData.nFlags = chunk.m_VoxData.nFlags;
        chunkVer3.m_VoxData.vOrigin = chunk.m_VoxData.vOrigin;
        chunkVer3.m_VoxData.vSize = chunk.m_VoxData.vSize;
      }
			else if(nVersion == 3)
			{
				SVoxelObjectChunkVer3 * pChunk = StepData<SVoxelObjectChunkVer3>(pPtr,eEndian);
				memcpy(&chunkVer3,pChunk,sizeof(chunkVer3));
			}
			else
			{
				assert(!"nVersion unknown");
				break; // error
			}

//				if (!CheckRenderFlagsMinSpec(chunkVer3.m_dwRndFlags))
//				continue;

      CVoxelObject * pObj = new CVoxelObject(chunkVer3.m_Matrix.GetTranslation(), DEF_VOX_UNIT_SIZE, 
				DEF_VOX_VOLUME_SIZE, DEF_VOX_VOLUME_SIZE, DEF_VOX_VOLUME_SIZE);

			// common node data
			COPY_MEMBER_LOAD(pObj,&chunkVer3,m_WSBBox);
      LoadCommonData(&chunkVer3, pObj);

			// voxel data
			COPY_MEMBER_LOAD(pObj,&chunkVer3,m_Matrix);

      if(chunkVer3.m_VoxData.nFlags & IVOXELOBJECT_FLAG_COMPILED)
      {
        PrintMessage("Loading compiled voxel object (%d,%d,%d) ...", 
          (int)chunkVer3.m_Matrix.GetTranslation().x, 
          (int)chunkVer3.m_Matrix.GetTranslation().y, 
          (int)chunkVer3.m_Matrix.GetTranslation().z);

        pObj->SetSurfacesInfo(&chunkVer3.m_VoxData);

        int nSize=0;
        pObj->SetCompiledMeshData(pPtr, nSize, eEndian, (chunkVer3.m_VoxData.nFlags & IVOXELOBJECT_FLAG_DATA_IS_ALLIGNED)!=0);
        nSize = pObj->GetCompiledMeshDataSize();
        pPtr += nSize;

        pObj->ReleaseMemBlocks();

        if(pObj->GetRenderMesh(0) && pObj->GetRenderMesh(0)->GetIndicesCount())
        {
          Get3DEngine()->RegisterEntity(pObj);
          PrintMessagePlus(" (%d KB) Ok", nSize/1024);
        }
        else
        {
          PrintMessagePlus(" (%d KB) Empty object skipped", nSize/1024);
          delete pObj;
        }
      }
      else
      {
        assert(nVersion == 3);
        pObj->SetData(&chunkVer3.m_VoxData, 0);
        Get3DEngine()->RegisterEntity(pObj);
      }

      delete pChunkVer3;
		}
		else if(eType == eERType_Road) 
		{
			SRoadChunk * pChunk = StepData<SRoadChunk>(pPtr,eEndian);
			if (!CheckRenderFlagsMinSpec(pChunk->m_dwRndFlags) && !GetCVars()->e_VoxTer)
      {
        for( int j=0; j < pChunk->m_nVertsNum; j++ )
        {
          Vec3 * pVert = StepData<Vec3>( pPtr,eEndian );
        }

				continue;
      }

			CRoadRenderNode * pObj = new CRoadRenderNode();

			// common node data
			COPY_MEMBER_LOAD(pObj,pChunk,m_WSBBox);
      LoadCommonData(pChunk, pObj);

			// road data
      if(!GetCVars()->e_VoxTer)
			  pObj->SetMaterial(CObjManager::GetItemPtr(pMatTable,pChunk->m_nMaterialId));
			pObj->m_sortPrio = pChunk->m_nSortPriority;

			for(int i=0; i<2; i++)
				COPY_MEMBER_LOAD(pObj,pChunk,m_arrTexCoors[i]);

			for(int i=0; i<2; i++)
				COPY_MEMBER_LOAD(pObj,pChunk,m_arrTexCoorsGlobal[i]);

			pObj->m_arrVerts.PreAllocate(pChunk->m_nVertsNum);
			for( int j=0; j < pChunk->m_nVertsNum; j++ )
			{
				Vec3 * pVert = StepData<Vec3>( pPtr,eEndian );
				pObj->m_arrVerts.Add( *pVert );
			}

			pObj->SetVertices(pObj->m_arrVerts.GetElements(), pObj->m_arrVerts.Count(), 
				pObj->m_arrTexCoors[0], pObj->m_arrTexCoors[1], 
				pObj->m_arrTexCoorsGlobal[0], pObj->m_arrTexCoorsGlobal[1]);

			Get3DEngine()->RegisterEntity(pObj);
		}
		else if( eERType_Decal == eType )
		{
			SDecalChunk* pChunk( StepData<SDecalChunk>( pPtr,eEndian ) );
			if (!CheckRenderFlagsMinSpec(pChunk->m_dwRndFlags))
				continue;

			CDecalRenderNode* pObj( new CDecalRenderNode() );

			// common node data
			pObj->SetBBox(pChunk->m_WSBBox);
      LoadCommonData(pChunk, pObj);

			// decal properties
			SDecalProperties properties;

			switch( pChunk->m_projectionType )
			{
			case SDecalProperties::eProjectOnTerrainAndStaticObjects:
				{
					properties.m_projectionType = SDecalProperties::eProjectOnTerrainAndStaticObjects;
					break;
				}
			case SDecalProperties::eProjectOnStaticObjects:
				{
					properties.m_projectionType = SDecalProperties::eProjectOnStaticObjects;
					break;
				}
			case SDecalProperties::eProjectOnTerrain:
				{
					properties.m_projectionType = SDecalProperties::eProjectOnTerrain;
					break;
				}
			case SDecalProperties::ePlanar:
			default:
				{
					properties.m_projectionType = SDecalProperties::ePlanar;
					break;
				}
			}
      memcpy(&properties.m_pos, &pChunk->m_pos, sizeof (pChunk->m_pos));
      memcpy(&properties.m_normal, &pChunk->m_normal, sizeof (pChunk->m_normal));
      memcpy(&properties.m_explicitRightUpFront, &pChunk->m_explicitRightUpFront, sizeof (pChunk->m_explicitRightUpFront));
      memcpy(&properties.m_radius, &pChunk->m_radius, sizeof(pChunk->m_radius));

			IMaterial* pMaterial( CObjManager::GetItemPtr(pMatTable,pChunk->m_nMaterialId) );
			assert(pMaterial);
      properties.m_pMaterialName = pMaterial ? pMaterial->GetName() : "";
			properties.m_sortPrio = pChunk->m_nSortPriority;
      properties.m_deferred = pChunk->m_deferred;

			pObj->SetDecalProperties( properties );

			//if( pObj->GetMaterialID() != pChunk->m_materialID )
			if( pObj->GetMaterial() != pMaterial )
			{
				const char* pMatName( "_can't_resolve_material_name_" );										
				int32 matID( pChunk->m_nMaterialId );
				if( matID >= 0 && matID < (int)pMatTable->size() )					
          pMatName = pMaterial ? pMaterial->GetName() : "";
				Warning( "Warning: Removed placement decal at (%4.2f, %4.2f, %4.2f) with invalid material \"%s\"!\n", pChunk->m_pos.x, pChunk->m_pos.y, pChunk->m_pos.z, pMatName );
				pObj->ReleaseNode();
			} 
			else
			{
				Get3DEngine()->RegisterEntity( pObj );
				GetObjManager()->m_decalsToPrecreate.push_back( pObj );
			}
		}
		else if( eERType_WaterVolume == eType )
		{
			// read common info
			SWaterVolumeChunk* pChunk( StepData<SWaterVolumeChunk>( pPtr,eEndian ) );
			if (!CheckRenderFlagsMinSpec(pChunk->m_dwRndFlags))
      {
        for( uint32 j( 0 ); j < pChunk->m_numVertices; ++j )
        {
          SWaterVolumeVertex* pVertex( StepData<SWaterVolumeVertex>( pPtr,eEndian ) );
        }

        for( uint32 j( 0 ); j < pChunk->m_numVerticesPhysAreaContour; ++j )
        {
          SWaterVolumeVertex* pVertex( StepData<SWaterVolumeVertex>( pPtr,eEndian ) );
        }

				continue;
      }

			CWaterVolumeRenderNode* pObj( new CWaterVolumeRenderNode() );

			// read common node data
      pObj->SetBBox(pChunk->m_WSBBox);
      LoadCommonData(pChunk, pObj);

			// read vertices
			std::vector<Vec3> vertices;
			vertices.reserve( pChunk->m_numVertices );
			for( uint32 j( 0 ); j < pChunk->m_numVertices; ++j )
			{
				SWaterVolumeVertex* pVertex( StepData<SWaterVolumeVertex>( pPtr,eEndian ) );
				vertices.push_back( pVertex->m_xyz );
			}

			// read physics area contour vertices
			std::vector<Vec3> physicsAreaContour;
			physicsAreaContour.reserve( pChunk->m_numVerticesPhysAreaContour );
			for( uint32 j( 0 ); j < pChunk->m_numVerticesPhysAreaContour; ++j )
			{
				SWaterVolumeVertex* pVertex( StepData<SWaterVolumeVertex>( pPtr,eEndian ) );
				physicsAreaContour.push_back( pVertex->m_xyz );
			}

			const int volumeType = pChunk->m_volumeTypeAndMiscBits & 0xFFFF;
			const bool capFogAtVolumeDepth = 	((pChunk->m_volumeTypeAndMiscBits & 0x10000) != 0) ? true : false;

			// create volume 
			switch(volumeType)
			{
			case IWaterVolumeRenderNode::eWVT_River:
				{
					pObj->CreateRiver( pChunk->m_volumeID, &vertices[0], pChunk->m_numVertices, pChunk->m_uTexCoordBegin, pChunk->m_uTexCoordEnd, Vec2( pChunk->m_surfUScale, pChunk->m_surfVScale ), pChunk->m_fogPlane );				
					break;
				}
			case IWaterVolumeRenderNode::eWVT_Area:
				{
					pObj->CreateArea( pChunk->m_volumeID, &vertices[0], pChunk->m_numVertices, Vec2( pChunk->m_surfUScale, pChunk->m_surfVScale ),  pChunk->m_fogPlane );
					break;
				}
			case IWaterVolumeRenderNode::eWVT_Ocean:
				{
					assert( !"COctreeNode::SerializeObjects( ... ) -- Water volume of type \"Ocean\" not supported yet!" );
					break;
				}
			default:
				{
					assert( !"COctreeNode::SerializeObjects( ... ) -- Invalid water volume type!" );
					break;
				}
			}

			// set material
			IMaterial* pMaterial( CObjManager::GetItemPtr(pMatTable, pChunk->m_materialID) );


			// set properties
			pObj->SetFogDensity( pChunk->m_fogDensity );
			pObj->SetFogColor( pChunk->m_fogColor );

			pObj->SetVolumeDepth(pChunk->m_volumeDepth);
			pObj->SetStreamSpeed(pChunk->m_streamSpeed);
			pObj->SetCapFogAtVolumeDepth(capFogAtVolumeDepth);

			// set physics
			if( !physicsAreaContour.empty() )
			{
				switch(volumeType)
				{
				case IWaterVolumeRenderNode::eWVT_River:
					{
						pObj->SetRiverPhysicsArea( &physicsAreaContour[0], physicsAreaContour.size() );
						break;
					}
				case IWaterVolumeRenderNode::eWVT_Area:
					{
						pObj->SetAreaPhysicsArea( &physicsAreaContour[0], physicsAreaContour.size() );
						break;
					}
				case IWaterVolumeRenderNode::eWVT_Ocean:
					{
						assert( !"COctreeNode::SerializeObjects( ... ) -- Water volume of type \"Ocean\" not supported yet!" );
						break;
					}
				default:
					{
						assert( !"COctreeNode::SerializeObjects( ... ) -- Invalid water volume type!" );
						break;
					}
				}

				pObj->Physicalize();
			}

			// set material
			pObj->SetMaterial( pMaterial );

			Get3DEngine()->RegisterEntity( pObj );					
		}
		else if(eERType_DistanceCloud == eType)
		{
			SDistanceCloudChunk* pChunk(StepData<SDistanceCloudChunk>(pPtr,eEndian));
			if (!CheckRenderFlagsMinSpec(pChunk->m_dwRndFlags))
				continue;

			CDistanceCloudRenderNode* pObj(new CDistanceCloudRenderNode());

			// common node data
			AABB box;
			memcpy(&box,&pChunk->m_WSBBox, sizeof(AABB));
      pObj->SetBBox(box);
      LoadCommonData(pChunk, pObj);

			// distance cloud properties
			SDistanceCloudProperties properties;

			properties.m_pos = pChunk->m_pos;
			properties.m_sizeX = pChunk->m_sizeX;
			properties.m_sizeY = pChunk->m_sizeY;
			properties.m_rotationZ = pChunk->m_rotationZ;

			IMaterial* pMaterial( CObjManager::GetItemPtr(pMatTable, pChunk->m_materialID) );
			assert(pMaterial);
      properties.m_pMaterialName = pMaterial ? pMaterial->GetName() : "";

			pObj->SetProperties(properties);

			if (pObj->GetMaterial() != pMaterial)
			{
				const char* pMatName("_can't_resolve_material_name_");										
				int32 matID(pChunk->m_materialID);
				if(matID >= 0 && matID < (int32)(*pMatTable).size())					
          pMatName = pMaterial ? pMaterial->GetName() : "";
				Warning("Warning: Removed distance cloud at (%4.2f, %4.2f, %4.2f) with invalid material \"%s\"!\n", pChunk->m_pos.x, pChunk->m_pos.y, pChunk->m_pos.z, pMatName);
				pObj->ReleaseNode();
			} 
			else
			{
				Get3DEngine()->RegisterEntity( pObj );
			}
		}
		else if( eERType_WaterWave == eType )
    {      
      // read common info
      SWaterWaveChunk* pChunk( StepData<SWaterWaveChunk>( pPtr,eEndian ) );
			if (!CheckRenderFlagsMinSpec(pChunk->m_dwRndFlags))
      {
        for( uint32 j( 0 ); j < pChunk->m_nVertexCount; ++j )
        {
          Vec3* pVertex( StepData<Vec3>( pPtr,eEndian ) );
        }

				continue;
      }

			CWaterWaveRenderNode* pObj( new CWaterWaveRenderNode() );

      // read common node data
      pObj->SetBBox(pChunk->m_WSBBox);
      LoadCommonData(pChunk, pObj);

      // read vertices
      std::vector<Vec3> pVertices;
      pVertices.reserve( pChunk->m_nVertexCount );
      for( uint32 j( 0 ); j < pChunk->m_nVertexCount; ++j )
      {
        Vec3* pVertex( StepData<Vec3>( pPtr,eEndian ) );
        pVertices.push_back( *pVertex );
      }

      // set material
      IMaterial* pMaterial( CObjManager::GetItemPtr(pMatTable, pChunk->m_nMaterialID) );
      assert( pMaterial );

      // set material
      pObj->SetMaterial( pMaterial );

      SWaterWaveParams pParams;
      pParams.m_fSpeed = pChunk->m_fSpeed;
      pParams.m_fSpeedVar = pChunk->m_fSpeedVar;
      pParams.m_fLifetime = pChunk->m_fLifetime;
      pParams.m_fLifetimeVar = pChunk->m_fLifetimeVar;
      pParams.m_fPosVar = pChunk->m_fPosVar;
      pParams.m_fHeight = pChunk->m_fHeight;
      pParams.m_fHeightVar = pChunk->m_fHeightVar;
      pParams.m_fCurrLifetime = pChunk->m_fCurrLifetime;          
      pParams.m_fCurrFrameLifetime = pChunk->m_fCurrFrameLifetime;
      pParams.m_fCurrSpeed = pChunk->m_fCurrSpeed;
      pParams.m_fCurrHeight = pChunk->m_fCurrHeight;

      pObj->SetParams(pParams);

      Vec2 vScaleUV( pChunk->m_fUScale, pChunk->m_fVScale );
      pObj->Create( pChunk->m_nID, &pVertices[0], pChunk->m_nVertexCount, vScaleUV, pChunk->m_pWorldTM);

      //Get3DEngine()->RegisterEntity( pObj );					
    }    
		else if(eERType_AutoCubeMap == eType)
		{
			SAutoCubeMapChunk* pChunk(StepData<SAutoCubeMapChunk>(pPtr,eEndian));
			if (!CheckRenderFlagsMinSpec(pChunk->m_dwRndFlags))
				continue;

			CAutoCubeMapRenderNode* pObj(new CAutoCubeMapRenderNode());

			// common node data
      pObj->SetBBox(pChunk->m_WSBBox);
      LoadCommonData(pChunk, pObj);

			// auto cube map properties
			SAutoCubeMapProperties properties;
			properties.m_flags = pChunk->m_flags;
			properties.m_obb.SetOBB(Matrix33(pChunk->m_oobRot), pChunk->m_oobH, pChunk->m_oobC);
			
			pObj->SetProperties(properties);

			Get3DEngine()->RegisterEntity( pObj );
		}
		else if(eERType_IrradianceVolume == eType)
		{
			SIrradianceVolumeChunk* pChunk(StepData<SIrradianceVolumeChunk>(pPtr,eEndian));
			if (!CheckRenderFlagsMinSpec(pChunk->m_dwRndFlags))
				continue;

			CIrradianceVolumeRenderNode* pObj(new CIrradianceVolumeRenderNode());

			// common node data
			LoadCommonData(pChunk, pObj);
			pObj->EnableSpecular(pChunk->m_bIsSpecular != 0);
			pObj->SetDensity(pChunk->m_fDensity);
			pObj->SetMatrix(pChunk->m_WorldMatrix);

			Get3DEngine()->RegisterEntity( pObj );
		}
		else
			assert(!"Unsupported object type");

    // align to 4
    while(uint32(pPtr)&3)
    {
      assert(*pPtr == 222);
      pPtr++;
    }
  }

	return nBlockSize;
}

