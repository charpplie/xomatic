////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   RenderMesh2.h
//  Version:     v1.00
//  Created:     01/07/2009 by Andrey Honich.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __RenderMesh2_h__
#define __RenderMesh2_h__
#pragma once





class RenderMesh_hash_int32
{
public:
  enum {	// parameters for hash table
    bucket_size = 4,	  // 0 < bucket_size



	min_buckets = 1024 // min_buckets = 2 ^^ N, 0 < N

  };

    ILINE size_t operator()( int32 key ) const
    {
      key = ~key + (key << 15);
      key = key ^ (key >> 12);
      key = key + (key << 2);
      key = key ^ (key >> 4);
      key = key * 2057;
      key = key ^ (key >> 16);
      return key;
    };
    ILINE bool operator()( const int32& key1,const int32& key2 ) const
    {
      return key1 < key2;
    }
};

struct SBufInfoTable
{
  int OffsTC;
  int OffsColor;
};

struct SMeshStream
{
  int m_nDevBuf;           // device buffer Id
  void *m_pUpdateData;     // system buffer for updating (used for async. mesh updates)
  void *m_pReadData;       // system temp data for reading only (can be created, accessed and deleted in main thread only)
  void *m_pLockedData;     // locked device buffer data (can be used in render thread only)
  int m_nFrameAccess;
  int m_nFrameRequest;
  int m_nFrameUpdate;
  int m_nFrameRead;
  uint32 m_nLockFlags;

  SMeshStream()
  {
    m_nDevBuf = -1;
    m_pUpdateData = NULL;
    m_pLockedData = NULL;
    m_pReadData = NULL;
    m_nFrameRequest = -2;
    m_nFrameUpdate = -1;
    m_nFrameAccess = -1;
    m_nFrameRead = 0;
    m_nLockFlags = 0;
  }
};

#define RM_INSTANCES 8
#define RM_MAXVERTS_FOR_INSTANCING 200

// CRenderMesh2::m_nFlags
#define FRM_RELEASED 1
#define FRM_INSTANCED 2

#define MAX_RELEASED_MESH_FRAMES 4

//typedef void (CRenderMesh2::*RenderMeshCallback)(DWORD context);

class CRenderMesh2 : public IRenderMesh
{
private:
  EVertexFormat m_eVF;          // Base stream vertex format (optional streams are hardcoded: VSF_)
  uint32 m_nVerts;
  uint32 m_nInds;

  SMeshStream m_VBStream[VSF_NUM];
  SMeshStream m_IBStream;
#ifdef KEEP_POSITIONS_INDICES
  Vec3f16 *m_pSysPosData;       // positions (PS3 only)
#endif
#ifdef FP16_MESH
  Vec3 *m_pCachePos;         // float positions (cached)
  uint32 m_nFrameRequestCachePos;
  uint32 m_nFlagsCachePos;
#endif

  CRenderMesh2 *m_pVertexContainer;
  PodArray<CRenderMesh2*>  m_lstVertexContainerUsers;

  uint32 m_nPrimetiveType;
  ERenderMeshType m_eType;
  uint32 m_nFlags;          // FRM_

#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT
  typedef stl::hash_map<int,PodArray< std::pair<int,int> >,RenderMesh_hash_int32> TrisMap;
  TrisMap * m_pTrisMap;
#endif

  uint32 *         m_arrVtxMap;                //!< [Anton] mapping table RenderMesh vertex idx->original vertex idx


private:
  bool UpdateVidIndices();

  bool CreateVidVertices(int nVerts=0, EVertexFormat eVF=eVF_Unknown, int nStream=VSF_GENERAL);
  bool UpdateVidVertices(int nStream);

  bool CopyVBToSystemForUpdate(int nStream);
  bool CopyIBToSystemForUpdate();

  void ReleaseVB(int nStream);
  void ReleaseIB();

  void InitTriHash();
#ifdef FP16_MESH
  bool CreateCachePos(byte *pSrc, uint32 nStrideSrc, uint32 nFlags);
#endif

public:
	//! constructor
	//! /param szSource this pointer is stored - make sure the memory stays
	CRenderMesh2(const char *szType, const char *szSourceName);
  CRenderMesh2();

	//! destructor
	~CRenderMesh2();

	virtual void AddRef() { m_nRefCounter++; }
	virtual int Release();
  void ReleaseForce()
  {
    while (true)
    {
      int nRef = Release();
      if (nRef <= 0)
        return;
    }
  }

  // ----------------------------------------------------------------
  // Helper functions

  _inline void Unlink()
  {
    if (!m_Next || !m_Prev)
      return;
    m_Next->m_Prev = m_Prev;
    m_Prev->m_Next = m_Next;
    m_Next = m_Prev = NULL;
  }
  _inline void Link(CRenderMesh2* Before)
  {
    if (m_Next || m_Prev)
      return;
    m_Next = Before->m_Next;
    Before->m_Next->m_Prev = this;
    Before->m_Next = this;
    m_Prev = Before;
  }

  _inline int GetStreamStride(int nStream) const
  {
    if (nStream == VSF_GENERAL)
      return  m_cSizeVF[m_eVF];
    else
      return m_cSizeStream[nStream];
  }

  _inline uint32 _GetFlags() { return m_nFlags; }
  _inline int GetStreamSize(int nStream, int nVerts=0) const { return GetStreamStride(nStream) * (nVerts ? nVerts : m_nVerts); }
  _inline int _GetVBStream(int nStream) { return m_VBStream[nStream].m_nDevBuf; }
  _inline int _GetIBStream() { return m_IBStream.m_nDevBuf; }
  _inline bool _HasVBStream(int nStream) { return m_VBStream[nStream].m_nDevBuf>=0; }
  _inline bool _HasIBStream() { return m_IBStream.m_nDevBuf>=0; }
  _inline int _IsVBStreamLocked(int nStream) { return (m_VBStream[nStream].m_nLockFlags & FSL_LOCKED); }
  _inline int _IsIBStreamLocked() { return m_IBStream.m_nLockFlags & FSL_LOCKED; }
  _inline EVertexFormat _GetVertexFormat() { return m_eVF; }
  _inline void _SetVertexFormat(EVertexFormat eVF) { m_eVF = eVF; }
  _inline int _GetNumVerts() { return m_nVerts; }
  _inline void _SetNumVerts(int nVerts) { m_nVerts = nVerts; }
  _inline int _GetNumInds() { return m_nInds; }
  _inline void _SetNumInds(int nInds) { m_nInds = nInds; }
  _inline int _GetPrimitiveType() { return m_nPrimetiveType; }
  _inline void _SetPrimitiveType(int nPrimType) { m_nPrimetiveType = nPrimType; }
  _inline void _SetRenderMeshType(ERenderMeshType eType) { m_eType = eType; }
  _inline CRenderMesh2 *_GetVertexContainer()
  {
    if (m_pVertexContainer)
      return m_pVertexContainer;
    return this;
  }
  int Size(uint32 nFlags);
	void Size(uint32 nFlags, ICrySizer *pSizer ) const;

  void *LockVB(int nStream, uint32 nFlags, int nOffset=0, int nVerts=0, int *nStride=NULL);
  uint16 *LockIB(uint32 nFlags, int nOffset=0, int nInds=0);
  void UnlockVB(int nStream);
  void UnlockIB();

  bool RT_CheckUpdate(CRenderMesh2 *pVContainer, EVertexFormat eVF, uint32 nStreamMask);
  bool CheckUpdate(EVertexFormat eVF, uint32 nStreamMask);
  void AssignChunk(CRenderChunk *pChunk, CREMesh *pRE)
  {
    pRE->m_pChunk = pChunk;
    pRE->m_pRenderMesh = this;
    pRE->m_nPrimetiveType = m_nPrimetiveType;
    pRE->m_nFirstIndexId = pChunk->nFirstIndexId;
    pRE->m_nNumIndices = pChunk->nNumIndices;
    pRE->m_nFirstVertId = pChunk->nFirstVertId;
    pRE->m_nNumVerts = pChunk->nNumVerts;    
  }
  void FreeDeviceBuffers(bool bRestoreSys);
  void FreeSystemBuffers();

#ifdef ENABLE_GPU_TIMERS

  static void StartTimerCallback(DWORD ptrMeshStat)
  {
    SRenderMeshStat* pMeshStat = reinterpret_cast<SRenderMeshStat*>(ptrMeshStat);
    
		if(pMeshStat==NULL)
      return;

    pMeshStat->nStartTime = CryGetTicks();
  };

  static void EndTimerCallback(DWORD ptrMeshStat)
  {
    SRenderMeshStat* pMeshStat = reinterpret_cast<SRenderMeshStat*>(ptrMeshStat);

		if(pMeshStat==NULL)
      return;

		uint64 perfCount = CryGetTicks();

    pMeshStat->nTotalTime += (perfCount - pMeshStat->nStartTime);
    pMeshStat->nStartTime = 0;
    pMeshStat->nBatchNumber++; 
  };

#endif

  //===========================================================================================
  // IRenderMesh interface
  VIRTUAL const char* GetTypeName()   { return m_sType; }
  VIRTUAL const char* GetSourceName() const { return m_sSource; }

  VIRTUAL int  GetIndicesCount()  { return m_nInds; }
  VIRTUAL int  GetVerticesCount() { return m_nVerts; }

  VIRTUAL EVertexFormat GetVertexFormat() { return m_eVF; }
  VIRTUAL ERenderMeshType GetMeshType() { return m_eType; }

	VIRTUAL const IRenderMesh::SRenderMeshStat* GetRenderStats() const;

  VIRTUAL void CreateChunksSkinned();
  VIRTUAL void CopyTo(IRenderMesh *pDst, int nAppendVtx=0, bool bDynamic=false);
  VIRTUAL void SetSkinningDataVegetation(struct SMeshBoneMapping *pBoneMapping);
  VIRTUAL void SetSkinningDataCharacter(CMesh& mesh, struct SMeshBoneMapping *pBoneMapping);
  // Creates an indexed mesh from this render mesh (accepts an optional pointer to an IIndexedMesh object that should be used)
  VIRTUAL IIndexedMesh* GetIndexedMesh(IIndexedMesh *pIdxMesh=0);
  VIRTUAL int GetRenderChunksCount(IMaterial *pMat, int& nRenderTrisCount);

  VIRTUAL IRenderMesh *GenerateMorphWeights() { return NULL; }
  VIRTUAL IRenderMesh *GetMorphBuddy() { return NULL; }
  VIRTUAL void SetMorphBuddy(IRenderMesh *pMorph) {}


  // Create render buffers from render mesh.
  VIRTUAL void SetMesh( CMesh &mesh, int nSecColorsSetOffset=0, uint32 flags=0,  const Vec3 *pPosOffset=0);	

  // Update system vertices buffer
  VIRTUAL bool UpdateVertices(const void *pVertBuffer, int nVertCount, int nOffset, int nStream=VSF_GENERAL);
  // Update system indices buffer
  VIRTUAL bool UpdateIndices(const uint16 *pNewInds, int nInds, int nOffsInd=0);

  VIRTUAL IMaterial* GetMaterial() { return m_pMaterial; }
  VIRTUAL void SetMaterial( IMaterial * pNewMat, int nCustomTID=0 );
  VIRTUAL void SetChunk(int nIndex, CRenderChunk &chunk, bool bForceInitChunk=false);
  VIRTUAL void SetChunk(IMaterial *pNewMat, int nFirstVertId, int nVertCount, int nFirstIndexId, int nIndexCount, float texelAreaDensity, int nMatID = 0,bool bForceInitChunk=false);
  VIRTUAL PodArray<CRenderChunk>& GetChunks() { return m_Chunks; }
  VIRTUAL PodArray<CRenderChunk>* GetChunksSkinned() { return m_pChunksSkinned; }
  VIRTUAL IRenderMesh *GetVertexContainer() { return _GetVertexContainer(); }
  VIRTUAL void SetVertexContainer(IRenderMesh *pBuf)
  {
		MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMesh, 0, "%s", this->GetSourceName());
		MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", this->GetTypeName());

    if (m_pVertexContainer)
      ((CRenderMesh2 *)m_pVertexContainer)->m_lstVertexContainerUsers.Delete(this);

    m_pVertexContainer = (CRenderMesh2 *)pBuf;

    if (m_pVertexContainer && ((CRenderMesh2 *)m_pVertexContainer)->m_lstVertexContainerUsers.Find(this)<0)
      ((CRenderMesh2 *)m_pVertexContainer)->m_lstVertexContainerUsers.Add(this);
  }

  VIRTUAL void Render(const struct SRendParams& rParams, CRenderObject* pObj, IMaterial *pMaterial, bool bSkinned=false);
  VIRTUAL void Render(CRenderObject * pObj, bool bSkinned=false, const SRenderObjectModifier * pROII=NULL);
  VIRTUAL void AddRenderElements(CRenderObject * pObj=0, int nSortId=EFSLIST_GENERAL, int nAW=1, IMaterial * pIMatInfo=NULL);
  VIRTUAL void SetREUserData(float * pfCustomData, float fFogScale=0, float fAlpha=1);
  VIRTUAL void AddRE(CRenderObject * pObj, IShader * pEf, int nList=EFSLIST_GENERAL, int nAW=1);

  VIRTUAL void DrawImmediately();

  VIRTUAL byte *GetPosPtr(int32& nStride, uint32 nFlags, int32 nOffset=0);
  VIRTUAL byte *GetNormPtr(int32& nStride, uint32 nFlags, int32 nOffset=0);
  VIRTUAL byte *GetColorPtr(int32& nStride, uint32 nFlags, int32 nOffset=0);
  VIRTUAL byte *GetUVPtr(int32& nStride, uint32 nFlags, int32 nOffset=0);

  VIRTUAL byte *GetTangentPtr(int32& nStride, uint32 nFlags, int32 nOffset=0);
  VIRTUAL byte *GetBinormalPtr(int32& nStride, uint32 nFlags, int32 nOffset=0);

  VIRTUAL byte *GetHWSkinPtr(int32& nStride, uint32 nFlags, int32 nOffset=0);
  VIRTUAL byte *GetShapePtr(int32& nStride, uint32 nFlags, int32 nOffset=0);
  VIRTUAL byte *GetMorphTargetPtr(int32& nStride, uint32 nFlags, int32 nOffset=0);

  VIRTUAL void UnlockStream(int nStream);
  VIRTUAL void UnlockIndexStream();

  VIRTUAL uint16 *GetIndexPtr(uint32 nFlags, int32 nOffset=0);
  VIRTUAL const PodArray<std::pair<int,int> > * GetTrisForPosition(const Vec3 & vPos);
  VIRTUAL void GetRandomPos(RandomPos& ran, GeomQuery& geo, EGeomForm eForm);
  VIRTUAL float ComputeExtent(GeomQuery& geo, EGeomForm eForm);
  VIRTUAL uint32 *GetPhysVertexMap() { return m_arrVtxMap; }
  VIRTUAL void SetPhysVertexMap(uint32 * pVtxMap) { m_arrVtxMap = pVtxMap; }
  VIRTUAL bool IsEmpty();

  VIRTUAL size_t GetMemoryUsage( ICrySizer *pSizer,EMemoryUsageArgument nType );
	VIRTUAL void GetMemoryUsage( ICrySizer *pSizer) const;
  VIRTUAL float GetAverageTrisNumPerChunk(IMaterial * pMat);
  VIRTUAL int GetTextureMemoryUsage(IMaterial *pMaterial, ICrySizer *pSizer=NULL);
  // Get allocated only in video memory or only in system memory.
  VIRTUAL int GetAllocatedBytes(bool bVideoMem);

  VIRTUAL void SetBBox(const Vec3& vBoxMin, const Vec3& vBoxMax) { m_vBoxMin = vBoxMin; m_vBoxMax = vBoxMax; }
  VIRTUAL void GetBBox(Vec3& vBoxMin, Vec3& vBoxMax) { vBoxMin = m_vBoxMin; vBoxMax = m_vBoxMax; };
  VIRTUAL void UpdateBBoxFromMesh();

  // Debug draw this render mesh.
  VIRTUAL void DebugDraw(const struct SGeometryDebugDrawInfo &info, uint32 nVisibleChunksMask=~0);
  VIRTUAL void KeepSysMesh(bool keep);  // workaround: temp workaround for GDC-888
	VIRTUAL void UnKeepSysMesh();
  VIRTUAL void SetMeshLod( int nLod ) { m_nLod = nLod; }

	// --------------------------------------------------------------
  // Members

#ifdef ENABLE_GPU_TIMERS
  SRenderMeshStat m_meshStat[3]; //GpuTimerEvent::s_numBuffers
#endif

  static int32 m_cSizeVF[eVF_Max];
  static int32 m_cSizeStream[VSF_NUM];
  static SBufInfoTable m_cBufInfoTable[eVF_Max];

  static CryCriticalSection m_sResLock;
	static CRenderMesh2     m_Root;
  static CRenderMesh2     m_RootRelease[MAX_RELEASED_MESH_FRAMES];
  static CRenderMesh2    *m_pLastTick;

	CRenderMesh2 *          m_Next;           //!<
	CRenderMesh2 *          m_Prev;           //!<

  PodArray<CRenderChunk>  m_Chunks;
  PodArray<CRenderChunk>* m_pChunksSkinned;

  int                     m_nRefCounter;
	const char*             m_sType;          //!< pointer to the type name in the constructor call
	const char*             m_sSource;        //!< pointer to the source  name in the constructor call
	_smart_ptr<IMaterial>   m_pMaterial;      // Default material for this render mesh.
  uint32                  m_nFrameRender;

  int                     m_nClientTextureBindID;
  void                   *m_pCustomData;
  Vec3                    m_vBoxMin;
  Vec3                    m_vBoxMax;

  int                     m_nLod; // used for LOD debug visualization

#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT
  CryCriticalSection m_getTrisForPositionLock;
#endif

  static void ShutDown();
  static void Tick();
	static void DeleteDelayedMeshes();
};

#endif // __RenderMesh2_h__
