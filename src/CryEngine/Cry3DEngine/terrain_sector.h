////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain_sector.h
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef SECINFO_H
#define SECINFO_H

#define MML_NOT_SET ((uint8)-1)

#define ARR_TEX_OFFSETS_SIZE 4
#define MAX_TERRAIN_SURFACE_TYPES 16
#define MAX_SURFACE_TYPES_COUNT 32

#include "BasicArea.h"
#include "Array2d.h"

#include <SpuUtils.h>
enum eTexureType
{
	ett_Diffuse,
	ett_LM
};

class CUpdateTerrainTempData;

struct SVoxSurfaceTypeInfo
{
  SVoxSurfaceTypeInfo() { nInUse=0; }
  byte nInUse;
};

struct SSurfaceTypeInfo
{
	SSurfaceTypeInfo() { memset(this, 0, sizeof(SSurfaceTypeInfo)); }
	struct SSurfaceType * pSurfaceType;
  IRenderMesh * arrpRM[3];
  int nInUse;

	bool HasRM() { return arrpRM[0] || arrpRM[1] || arrpRM[2]; }
	int GetIndexCount();
	void DeleteRenderMeshes(IRenderer * pRend);
};

struct SRangeInfo 
{
  SRangeInfo()
  {
    nModified = false;
    pHMData = NULL;
  }

	void UpdateBitShift(int nUnitToSectorBS)
	{
		const int nSecSize = 1<<nUnitToSectorBS;
		int n = nSize-1;
		nUnitBitShift = 0;
		while(n>0 && nSecSize>n) 
		{ 
			nUnitBitShift++; 
			n*=2; 
		}
		
		if(nSize>1)
			fInvStep = 1.f/(nSecSize/(nSize-1));	
		else
			fInvStep = 1.f;
	}

	inline uint16 GetDataLocal( int nX, int nY ) const
	{
		assert(nX>=0 && nX<(int)nSize);
		assert(nY>=0 && nY<(int)nSize);
		assert(pHMData);
		return pHMData[nX*nSize + nY];
	}

  inline uint32 GetDataLocalI( int nX, int nY ) const
  {
    assert(nX>=0 && nX<(int)nSize);
    assert(nY>=0 && nY<(int)nSize);
    assert(pHMData);
    return pHMData[nX*nSize + nY];
  }

  inline void SetDataLocal( int nX, int nY, uint16 usValue )
  {
    assert(nX>=0 && nX<(int)nSize);
    assert(nY>=0 && nY<(int)nSize);
    assert(pHMData);
    pHMData[nX*nSize + nY] = usValue;
  }

	inline uint16 GetDataUnits( int nX_units, int nY_units ) const
	{
		int nMask = nSize-2;
		int nX = nX_units >> nUnitBitShift;
		int nY = nY_units >> nUnitBitShift;
		return GetDataLocal(nX&nMask, nY&nMask);
	}

	float fOffset;
	float fRange;
	uint16 * pHMData;
	uint16 nSize;
	uint8 nUnitBitShift;
  uint8 nModified;
	float fInvStep;
};

template <class T> class TPool
{
public:

	TPool(int nPoolSize)
	{
    m_nPoolSize = nPoolSize;
		m_pPool = new T[nPoolSize];
		m_lstFree.PreAllocate(nPoolSize,0);
		m_lstUsed.PreAllocate(nPoolSize,0);
		for(int i=0; i<nPoolSize; i++)
			m_lstFree.Add(&m_pPool[i]);
	}

	~TPool()
	{
		delete [] m_pPool;
	}

	void ReleaseObject(T * pInst)
	{
		if(m_lstUsed.Delete(pInst))
			m_lstFree.Add(pInst);
	}

	int GetUsedInstancesCount(int & nAll)
	{
		nAll = m_nPoolSize;
		return m_lstUsed.Count();
	}

	T * GetObject() 
	{
		T * pInst = NULL;
		if(m_lstFree.Count())
		{
			pInst = m_lstFree.Last();
			m_lstFree.DeleteLast();
			m_lstUsed.Add(pInst);
		}
		else
			assert("TPool::GetObject: Out of free elements error");

		return pInst;
	}

	void GetMemoryUsage(class ICrySizer * pSizer)
	{
    pSizer->AddObject(this, sizeof(*this)+m_lstFree.GetDataSize()+m_lstUsed.GetDataSize());

    if(m_pPool)
      for(int i=0; i<m_nPoolSize; i++)
	  	  m_pPool[i].GetMemoryUsage(pSizer);
	}

	PodArray<T*> m_lstFree;
	PodArray<T*> m_lstUsed;
	T * m_pPool;
  int m_nPoolSize;
};

#define MAX_PROC_OBJ_CHUNKS_NUM 128

struct SProcObjChunk : public Cry3DEngineBase
{
	CVegetation * m_pInstances;
	SProcObjChunk();
	~SProcObjChunk();
  void GetMemoryUsage(class ICrySizer * pSizer);
};

typedef TPool<SProcObjChunk> SProcObjChunkPool;

class CProcObjSector : public Cry3DEngineBase
{
public:
	CProcObjSector() { m_nProcVegetNum = 0; m_ProcVegetChunks.PreAllocate(32); }
	~CProcObjSector();
	CVegetation * AllocateProcObject();
	void ReleaseAllObjects();
	int GetUsedInstancesCount(int & nAll) { nAll = m_ProcVegetChunks.Count(); return m_nProcVegetNum; }
	void GetMemoryUsage(ICrySizer*pSizer);

protected:
	PodArray<SProcObjChunk*> m_ProcVegetChunks;
	int m_nProcVegetNum;
};

typedef TPool<CProcObjSector> CProcVegetPoolMan;


struct STerrainNodeLeafData
{
	STerrainNodeLeafData() { memset(this,0,sizeof(*this)); }
	~STerrainNodeLeafData();
	float m_arrTexGen[MAX_RECURSION_LEVELS][ARR_TEX_OFFSETS_SIZE];
	short m_arrpNonBorderIdxNum[MAX_SURFACE_TYPES_COUNT][4];
	PodArray<CTerrainNode*> m_lstNeighbourSectors;
	PodArray<int> m_lstNeighbourLods;
	IRenderMesh * m_pRenderMesh;
};

struct CTerrainNode : public Cry3DEngineBase, public IShadowCaster, public IStreamCallback
{
public:

	friend class CTerrain;

	virtual void Render(const SRendParams &RendParams);
  const AABB GetBBox() const;
  virtual const AABB GetBBoxVirtual() { return GetBBox(); }
  virtual struct ICharacterInstance* GetEntityCharacter( unsigned int nSlot, Matrix34A * pMatrix = NULL, bool bReturnOnlyVisible = false ) {return NULL;};
  virtual bool IsRenderNode() { return false; }

	// streaming
	virtual void StreamOnComplete (IReadStream* pStream, unsigned nError);	
	void StartSectorTexturesStreaming(bool bFinishNow);
	IReadStreamPtr m_pReadStream;
	EFileStreamingStatus m_eTexStreamingStatus;

	CTerrainNode(int x1, int y1, int nNodeSize, CTerrainNode * pParent, bool bBuildErrorsTable, int nSID);
	~CTerrainNode();
	bool CheckVis(bool bAllIN, bool bAllowRenderIntoCBuffer, const Vec3 & vSegmentOrigin);
	void SetupTexturing(bool bMakeUncompressedForEditing = false);
  void RequestTextures();
	void SetSectorTexture(unsigned int textureId);
	void CheckNodeGeomUnload();
	IRenderMesh * MakeSubAreaRenderMesh(const Vec3 & vPos, float fRadius, IRenderMesh * pPrevRenderMesh, IMaterial * pMaterial, bool bRecalIRenderMeshconst, const char * szLSourceName);
	void SetChildsLod(int nNewGeomLOD);
  int GetAreaLOD();
	void RenderNodeHeightmap();
	bool CheckUpdateProcObjects();
	void IntersectTerrainAABB(const AABB & aabbBox, PodArray<CTerrainNode*> & lstResult);
	void UpdateDetailLayersInfo(bool bRecursive);
	void RemoveProcObjects(bool bRecursive = false);
	void IntersectWithShadowFrustum(bool bAllIn, PodArray<CTerrainNode*> * plstResult, ShadowMapFrustum * pFrustum, const float fHalfGSMBoxSize);
	void IntersectWithBox(const AABB & aabbBox, PodArray<CTerrainNode*> * plstResult);
  void GetTerrainAOTextureNodesInBox(const AABB & aabbBox, PodArray<CTerrainNode*> * plstResult);
	CTerrainNode * FindMinNodeContainingBox(const AABB & aabbBox);
	void RenderSector();
	CTerrainNode * GetTexuringSourceNode(int nTexMML, eTexureType eTexType);
	CTerrainNode * GetReadyTexSourceNode(int nTexMML, eTexureType eTexType);
	int GetData(byte * & pData, int & nDataSize, EEndian eEndian, SHotUpdateInfo * pExportInfo);

  template<class T> 
  int Load_T(T * & f, int & nDataSize, EEndian eEndian, SHotUpdateInfo * pExportInfo);
  int Load(uint8 * & f, int & nDataSize, EEndian eEndian, SHotUpdateInfo * pExportInfo);
  int Load(FILE * & f, int & nDataSize, EEndian eEndian, SHotUpdateInfo * pExportInfo);
  int ReloadModifiedHMData(FILE * f);

	void UnloadNodeTexture(bool bRecursive);
	float GetSurfaceTypeAmount(Vec3 vPos, int nSurfType);
	void GetMemoryUsage(ICrySizer*pSizer);
	void GetResourceMemoryUsage(ICrySizer*pSizer,const AABB& cstAABB);
//	void GetProcVegetMemoryUsage(ICrySizer*pSizer);

	void SetLOD();
	uint8 GetTextureLOD(float fDistance);

//	void SetHeightmapAABBAndHoleFlag();

	void ReleaseHeightMapGeometry(bool bRecursive = false, const AABB * pBox = NULL);
	int GetSecIndex();

	void DrawArray();
	void UpdateRenderMesh(struct CStripsInfo * pArrayInfo, bool bUpdateVertices);
	void BuildVertices(int step, bool bSafetyBorder);  
	
	int  GetMML(int dist, int mmMin, int mmMax);

	PodArray<CDLight*> * GetAffectingLights();
	void AddLightSource(CDLight * pSource);
  void CheckInitAffectingLights();

//	void GenerateIndicesForQuad(IRenderMesh * pRM, Vec3 vBoxMin, Vec3 vBoxMax, PodArray<uint16> & dstIndices);

	uint32 GetLastTimeUsed() { return m_nLastTimeUsed; }

	void AddIndexAliased(int _x, int _y, int _step,  int nSectorSize, PodArray<CTerrainNode*> * plstNeighbourSectors, CStripsInfo * pArrayInfo);
	static void GenerateIndicesForAllSurfaces(IRenderMesh * pRM, bool bOnlyBorder, short arrpNonBorderIdxNum[MAX_SURFACE_TYPES_COUNT][4], int nBorderStartIndex, SSurfaceTypeInfo * pSurfaceTypeInfos, int nSID);
	void BuildIndices(CStripsInfo & si, PodArray<CTerrainNode*> * pNeighbourSectors, bool bSafetyBorder);
	
	// entry and sync functions for SPU asynchrony RenderSectorUpdate
	void BuildIndices_Wrapper();
	void BuildVertices_Wrapper();  
	void RenderSectorUpdate_Finish();

	static void UpdateSurfaceRenderMeshes(IRenderMesh * pSrcRM, struct SSurfaceType * pSurface, IRenderMesh * & pMatRM, int nProjectionId, PodArray<unsigned short> & lstIndices, const char * szComment, bool bUpdateOnlyBorders, int nNonBorderIndicesCount);
	void SetupTexGens();
	static void SetupTexGenParams(SSurfaceType * pLayer, float * pOutParams, uint8 ucProjAxis, bool bOutdoor, float fTexGenScale = 1.f);

	int CreateSectorTexturesFromBuffer();
		
	bool CheckUpdateDiffuseMap();
	bool AssignTextureFileOffset(int16 * &pIndices, int16 & nElementsNum);
	static CProcVegetPoolMan * GetProcObjPoolMan() { return m_pProcObjPoolMan; }
	static SProcObjChunkPool * GetProcObjChunkPool() { return m_pProcObjChunkPool; }
	static void SetProcObjPoolMan(CProcVegetPoolMan * pProcObjPoolMan) { m_pProcObjPoolMan = pProcObjPoolMan; }
	static void SetProcObjChunkPool(SProcObjChunkPool * pProcObjChunkPool) { m_pProcObjChunkPool = pProcObjChunkPool; }
	void UpdateDistance();
	const float GetDistance();
	bool IsProcObjectsReady() { return m_bProcObjectsReady!=0; }
  void UpdateRangeInfoShift();
  
	// member variables
	CTerrainNode * m_arrChilds[4];

	// flags
	uint8 m_bProcObjectsReady : 1;
	uint8 m_bMergeNotAllowed : 1;
	uint8 m_bHasHoles : 1; // sector has holes in the ground
	uint8 m_bHasLinkedVoxel: 1; // for editor
	uint8 m_bNoOcclusion : 1; // sector has visareas under terrain surface
	uint8 // LOD's
		m_cNewGeomMML, m_cCurrGeomMML, 
		m_cNewGeomMML_Min, m_cNewGeomMML_Max,
		m_cNodeNewTexMML, m_cNodeNewTexMML_Min; 

	uint32 m_nEditorDiffuseTex; // for editor

  uint16 m_nOriginX, m_nOriginY; // sector origin
  int m_nLastTimeUsed; // basically last time rendered
  int m_nSetLodFrameId;
	OcclusionTestClient m_OcclusionTestClient;


  float * m_pGeomErrors; // errors for each lod level

protected:

	// temp data for terrain generation
	CUpdateTerrainTempData *m_pUpdateTerrainTempData;

public:

  PodArray<CDLight*> m_lstAffectingLights; uint32 m_nLightMaskFrameId;

	PodArray<SSurfaceTypeInfo> m_lstSurfaceTypeInfo;

	SRangeInfo m_rangeInfo;

//  IRenderMesh * m_pCBRenderMesh;

	STerrainNodeLeafData* m_pLeafData;

	CProcObjSector * m_pProcObjPoolPtr;

	void CheckLeafData()
	{ 
		if(!m_pLeafData)
			m_pLeafData = new STerrainNodeLeafData; 
	}

	inline STerrainNodeLeafData * GetLeafData()
	{ 
		return m_pLeafData;
	}

  int GetSectorSizeInHeightmapUnits();

  int m_nTreeLevel;
  SSectorTextureSet m_nNodeTexSet, m_nTexSet; // texture id's

	struct STerrainLightInfo 
	{ 
		uint8 r,g,b,a; 
	};

  int m_nNodeTextureLastUsedSec;
	//int m_nNodeRenderLastFrameId;
	AABB m_boxHeigtmapLocal;
	struct CTerrainNode * m_pParent;
	//int m_nSetupTexGensFrameId;
  int m_nGSMFrameId;

	float m_arrfDistance[MAX_RECURSION_LEVELS];
	int m_nNodeTextureOffset;
  int m_nNodeHMDataOffset;
  int FTell(uint8 * & f);
  int FTell(FILE * & f);

	static PodArray<uint16> m_arrIndices[MAX_SURFACE_TYPES_COUNT][4];
	static CProcVegetPoolMan * m_pProcObjPoolMan;
	static SProcObjChunkPool * m_pProcObjChunkPool;

  int m_nSID;
	bool m_bUpdateOnlyBorders;		// remeber if only the border were updated
};

#pragma pack(push,4)

struct STerrainNodeChunk
{
	int16	nChunkVersion;
  int16 bHasHoles;
	AABB	boxHeightmap;
	float fOffset;
	float fRange;
	int		nSize;
	int		nSurfaceTypesNum;

	AUTO_STRUCT_INFO
};

#pragma pack(pop)

#endif
