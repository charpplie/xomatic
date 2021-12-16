#include "StdAfx.h"

#include "I3DEngine.h"
#include "IIndexedMesh.h"
#include "CGFContent.h"
#include "GeomQuery.h"
#include "RenderMesh2.h"














	#define MESSAGE_VIDEO_BUFFER_ACC_ATTEMPT


#ifndef __SPU__
//#define MEM_CHECK assert(_CrtCheckMemory());
#define MEM_CHECK 

CryCriticalSection CRenderMesh2::m_sResLock;

// General vertex stream stride
int32 CRenderMesh2::m_cSizeVF[eVF_Max] = 
{
  0,
  sizeof(SVF_P3F_C4B_T2F),
  sizeof(SVF_P3S_C4B_T2S),
  sizeof(SVF_P3S_N4B_C4B_T2S),
  sizeof(SVF_P3F_C4B_I4B_PS4F),
  sizeof(SVF_TP3F_C4B_T2F),
  sizeof(SVF_TP3F_T2F_T3F),
  sizeof(SVF_P3F_T3F),
  sizeof(SVF_P3F_T2F_T3F),
  sizeof(SVF_T2F),
  sizeof(SVF_W4B_I4B_P3F),
  sizeof(SVF_C4B_C4B),
  sizeof(SVF_P3F_P3F_I4B),
  sizeof(SVF_P3F)
};
// Additional streams stride
int32 CRenderMesh2::m_cSizeStream[VSF_NUM] = 
{
  -1,
  sizeof(SPipTangents),        // VSF_TANGENTS
  sizeof(SQTangents),          // VSF_QTANGENTS
  sizeof(SVF_W4B_I4B_P3F),     // VSF_HWSKIN_INFO
  sizeof(SVF_C4B_C4B),         // VSF_SH_INFO
  sizeof(SVF_P3F_P3F_I4B),     // VSF_HWSKIN_SHAPEDEFORM_INFO
  sizeof(SVF_P3F),             // VSF_HWSKIN_MORPHTARGET_INFO
};

SBufInfoTable CRenderMesh2::m_cBufInfoTable[eVF_Max] = 
{
  {
    0
  },
  {  //eVF_P3F_C4B_T2F
#define OOFS(x) (int)(INT_PTR)&(((SVF_P3F_C4B_T2F *)0)->x)  
    OOFS(st),
    OOFS(color.dcolor)
#undef OOFS
  },
  {  //eVF_P3S_C4B_T2S
#define OOFS(x) (int)(INT_PTR)&(((SVF_P3S_C4B_T2S *)0)->x)  
      OOFS(st),
      OOFS(color.dcolor)
#undef OOFS
  },
  {  //eVF_P3F_N4B_C4B_T2F
#define OOFS(x) (int)(INT_PTR)&(((SVF_P3S_N4B_C4B_T2S *)0)->x)  
    OOFS(st),
    OOFS(color.dcolor),
#undef OOFS
  },

  {  // eVF_P3F_C4B_I4B_PS4F
#define OOFS(x) (int)(INT_PTR)&(((SVF_P3F_C4B_I4B_PS4F *)0)->x)  
    0,
    OOFS(color.dcolor),
#undef OOFS
  },
  {  // eVF_TP3F_C4B_T2F
#define OOFS(x) (int)(INT_PTR)&(((SVF_TP3F_C4B_T2F *)0)->x)  
    OOFS(st[0]),
    OOFS(color.dcolor),
#undef OOFS
  },
  {  // eVF_TP3F_T2F_T3F
#define OOFS(x) (int)(INT_PTR)&(((SVF_TP3F_T2F_T3F *)0)->x)  
    OOFS(st0[0]),
#undef OOFS
  },
  {  // eVF_P3F_T3F
#define OOFS(x) (int)(INT_PTR)&(((SVF_P3F_T3F *)0)->x)  
    OOFS(st[0]),
#undef OOFS
  },
  {  // eVF_P3F_T2F_T3F
#define OOFS(x) (int)(INT_PTR)&(((SVF_P3F_T2F_T3F *)0)->x)  
    OOFS(st0[0]),
#undef OOFS
  }
};

CRenderMesh2 *CRenderMesh2::m_pLastTick;
CRenderMesh2  CRenderMesh2::m_Root;
CRenderMesh2  CRenderMesh2::m_RootRelease[4];

int CRenderMesh2::Release()
{
  if (--m_nRefCounter <= 0)
  {
    m_nFlags |= FRM_RELEASED;
    int nFrame = gRenDev->GetFrameID(false);
    CRenderMesh2& Root = CRenderMesh2::m_RootRelease[nFrame & 3];
    {
      AUTO_LOCK(m_sResLock);
      Unlink();
    }
    Link(&Root);
    return 0;
  }

  return m_nRefCounter;
}

CRenderMesh2::CRenderMesh2()
{
  if (!m_Root.m_Next)
  {
    m_Root.m_Next = &m_Root;
    m_Root.m_Prev = &m_Root;
  }
  if (!m_RootRelease[0].m_Next)
  {
    for (int i=0; i<4; i++)
    {
      m_RootRelease[i].m_Next = &m_RootRelease[i];
      m_RootRelease[i].m_Prev = &m_RootRelease[i];
    }
  }
}

CRenderMesh2::CRenderMesh2 (const char *szType, const char *szSourceName)
{
  m_Next = NULL;
  m_Prev = NULL;

  m_nRefCounter = 0;

  m_sType = szType;
  m_sSource = szSourceName;

  m_vBoxMin = m_vBoxMax = Vec3(0,0,0); //used for hw occlusion test
  m_nVerts = 0;
  m_nInds = 0;
  m_eVF = eVF_P3F_C4B_T2F;
  m_pVertexContainer = NULL;

  {
    AUTO_LOCK(m_sResLock);
    Link(&m_Root);
  }
  m_pCustomData = NULL;
  m_pChunksSkinned = NULL;
  m_nPrimetiveType = R_PRIMV_TRIANGLES;

  m_nFrameRender = 0;
  //m_nFrameUpdate = 0;
  m_arrVtxMap = NULL;
  m_nClientTextureBindID = 0;
#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT
  m_pTrisMap = NULL;
#endif

#ifdef KEEP_POSITIONS_INDICES
  m_pSysPosData = NULL;
#endif

#ifdef FP16_MESH
  m_pCachePos = NULL;        
  m_nFrameRequestCachePos = 0;
  m_nFlagsCachePos = 0;
#endif

  _SetRenderMeshType(eRMT_Static);

  m_nFlags = 0;

#if defined(ENABLE_GPU_TIMERS)
	for(int i=0; i<GpuTimerEvent::s_numBuffers; i++)
	{
		m_meshStat[i].nFrameID = 0;
		m_meshStat[i].nStartTime = 0;
		m_meshStat[i].nTotalTime = 0;
		m_meshStat[i].nBatchNumber = 0;
	}
#endif

  m_nLod = 0;
}

//////////////////////////////////////////////////////////////////////////
CRenderMesh2::~CRenderMesh2()
{
  if (this == m_pLastTick)
    m_pLastTick = NULL;

  {
    AUTO_LOCK(m_sResLock);
    Unlink();
  }

  delete m_pChunksSkinned;
  //ReleaseChunks(m_pMergedDepthOnlyChunks);

  FreeDeviceBuffers(false);
  FreeSystemBuffers();

  if (m_pVertexContainer)
    m_pVertexContainer->m_lstVertexContainerUsers.Delete(this);

  for(int i=0; i<m_lstVertexContainerUsers.Count(); i++)
  {
    if (m_lstVertexContainerUsers[i]->GetVertexContainer() == this)
      m_lstVertexContainerUsers[i]->m_pVertexContainer = NULL;
  }

#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT
  SAFE_DELETE(m_pTrisMap);
#endif
}
#endif//__SPU__

void *CRenderMesh2::LockVB(int nStream, uint32 nFlags, int nOffset, int nVerts, int *nStride)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMesh, 0, "%s", this->GetSourceName());
	MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", this->GetTypeName());

  assert(nVerts <= m_nVerts);
  if (nVerts > m_nVerts)
    nVerts = m_nVerts;
  if (nStride)
    *nStride = GetStreamStride(nStream);

  SMeshStream& MS = m_VBStream[nStream];
  byte *pD;
#if !defined(__SPU__) // SPUs only currently read, so no need for this code
  int nFrame = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;

  if (nFlags == FSL_SYSTEM_CREATE)
  {
    //ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)
    if (!MS.m_pUpdateData)
    {
      uint32 nSize = GetStreamSize(nStream);
      pD = new byte[nSize];
      MS.m_pUpdateData = pD;
    }
    else
      pD = (byte *)MS.m_pUpdateData;
    MS.m_nFrameRequest = nFrame;
    MS.m_nLockFlags = (nFlags | (MS.m_nLockFlags & FSL_LOCKED));
    return &pD[nOffset];
  }
  else
  if (nFlags == FSL_SYSTEM_UPDATE)
  {
#if !defined(XENON)
lSysUpd:
#endif
    if (!MS.m_pUpdateData)
    {
      MESSAGE_VIDEO_BUFFER_ACC_ATTEMPT
      CopyVBToSystemForUpdate(nStream);
    }
    assert(nStream || MS.m_pUpdateData);
    if (!MS.m_pUpdateData)
      return NULL;
    MS.m_nFrameRequest = nFrame;
    pD = (byte *)MS.m_pUpdateData;
    MS.m_nLockFlags = (nFlags | (MS.m_nLockFlags & FSL_LOCKED));
    return &pD[nOffset];
  }
  else
  if (nFlags == FSL_READ)
#endif//__SPU__
  {
    //ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)
    if (MS.m_pUpdateData)
    {
      pD = (byte *)MS.m_pUpdateData;
      return &pD[nOffset];
    }
    nFlags = FSL_READ | FSL_VIDEO;
  }
#if !defined(__SPU__) // SPUs only currently read, so no need for this code
  if (nFlags == (FSL_READ | FSL_VIDEO))
  {
/*#if defined (PS3) // Always use system buffer on PS3
    pD = (byte *)MS.m_pUpdateData;
    assert(pD);
    return &pD[nOffset];
#else*/
    // TODO: make smart caching mesh algorithm for consoles
#if !defined (XENON)
    if (gRenDev->m_pRT && gRenDev->m_pRT->IsMultithreaded())
    {
      // Always use system copy in MT mode
      goto lSysUpd;
    }
    else
#endif
    //assert(gRenDev->m_pRT->IsRenderThread());
    {
      int nVB = MS.m_nDevBuf;
      if (nVB < 0)
        return NULL;
      // Try to lock device buffer in single-threaded mode
      if (!MS.m_pLockedData)
      {
				MESSAGE_VIDEO_BUFFER_ACC_ATTEMPT
				MS.m_pLockedData = gRenDev->m_DevBufMan.LockVB(nVB, 0, 0, FSL_READ);
        MS.m_nLockFlags |= FSL_LOCKED;
      }
      if (MS.m_pLockedData)
      {
        pD = (byte *)MS.m_pLockedData;
        return &pD[nOffset];
      }
    }
  }
  assert(0);
#endif//__SPU__
  return NULL;
}

uint16 *CRenderMesh2::LockIB(uint32 nFlags, int nOffset, int nInds)
{
  byte *pD;
#ifndef __SPU__
  int nFrame = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;

  assert(nInds <= m_nInds);
  if (nFlags == FSL_SYSTEM_CREATE)
  {
    //ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)
    if (!m_IBStream.m_pUpdateData)
    {
      uint32 nSize = m_nInds * sizeof(uint16);
      pD = new byte[nSize];
      m_IBStream.m_pUpdateData = (uint16 *)pD;
    }
    else
      pD = (byte *)m_IBStream.m_pUpdateData;
    m_IBStream.m_nFrameRequest = nFrame;
    m_IBStream.m_nLockFlags = (nFlags | (m_IBStream.m_nLockFlags & FSL_LOCKED));
    return (uint16 *)&pD[nOffset];
  }
  else
  if (nFlags == FSL_SYSTEM_UPDATE)
  {
#if !defined (XENON)  // Always read from video buffer on x360
lSysUpd:
#endif
    if (!m_IBStream.m_pUpdateData)
    {
      MESSAGE_VIDEO_BUFFER_ACC_ATTEMPT
			CopyIBToSystemForUpdate();
    }
    assert(m_IBStream.m_pUpdateData);
    if (!m_IBStream.m_pUpdateData)
      return NULL;
    m_IBStream.m_nFrameRequest = nFrame;
    pD = (byte *)m_IBStream.m_pUpdateData;
    m_IBStream.m_nLockFlags = (nFlags | (m_IBStream.m_nLockFlags & FSL_LOCKED));
    return (uint16 *)&pD[nOffset];
  }
  else
  if (nFlags == FSL_READ)
#endif//__SPU__
  {
    //ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)
    if (m_IBStream.m_pUpdateData)
    {
      pD = (byte *)m_IBStream.m_pUpdateData;
      return (uint16 *)&pD[nOffset];
    }
    nFlags = FSL_READ | FSL_VIDEO;
  }
#if !defined(__SPU__) // SPUs only currently read, so no need for this code
  if (nFlags == (FSL_READ | FSL_VIDEO))
  {
/*#if defined (PS3) // Always use shadow buffer on PS3
    pD = (byte *)m_IBStream.m_pUpdateData[0];
    if (!pD)
      pD = (byte *)m_IBStream.m_pUpdateData[1];
    assert(pD);
    return (uint16 *)&pD[nOffset];
#else*/
    int nIB = m_IBStream.m_nDevBuf;
    if (nIB < 0)
      return NULL;
#if !defined(XENON) && !defined(__SPU__)  // Always read from video buffer on x360
    if (gRenDev->m_pRT && gRenDev->m_pRT->IsMultithreaded())
    {
      // Always use system copy in MT mode
      goto lSysUpd;
    }
    else
#endif
    //assert(gRenDev->m_pRT->IsRenderThread());
    {
      // TODO: make smart caching mesh algorithm for consoles
      if (!m_IBStream.m_pLockedData)
      {
				MESSAGE_VIDEO_BUFFER_ACC_ATTEMPT
				m_IBStream.m_pLockedData = gRenDev->m_DevBufMan.LockIB(nIB, 0, 0, FSL_READ);
        m_IBStream.m_nLockFlags |= FSL_LOCKED;
      }
      if (m_IBStream.m_pLockedData)
      {
        pD = (byte *)m_IBStream.m_pLockedData;
        return (uint16 *)&pD[nOffset];
      }
    }
//#endif
  }
  assert(0);
#endif//__SPU__
  return NULL;
}
#ifndef __SPU__
void CRenderMesh2::UnlockVB(int nStream)
{
  if (m_VBStream[nStream].m_nLockFlags & FSL_LOCKED)
  {
		AUTO_LOCK(m_sResLock);
    assert(!gRenDev->m_pRT->IsMultithreaded());
    m_VBStream[nStream].m_nLockFlags &= ~FSL_LOCKED;
    gRenDev->m_DevBufMan.UnlockVB(m_VBStream[nStream].m_nDevBuf);
  }
}
void CRenderMesh2::UnlockIB()
{
  if (m_IBStream.m_nLockFlags & FSL_LOCKED)
  {
		AUTO_LOCK(m_sResLock);
    assert(!gRenDev->m_pRT->IsMultithreaded());
    m_IBStream.m_nLockFlags &= ~FSL_LOCKED;
    gRenDev->m_DevBufMan.UnlockIB(m_IBStream.m_nDevBuf);
  }
}

void CRenderMesh2::UnlockStream(int nStream)
{
  UnlockVB(nStream);
  //if (m_VBStream[nStream].m_nLockFlags & FSL_WRITE)
  //  m_VBStream[nStream].m_nFrameUpdate = m_VBStream[nStream].m_nFrameRequest-1;
#ifdef FP16_MESH
  if (nStream == VSF_GENERAL && (m_nFlagsCachePos & FSL_WRITE) && m_pCachePos)
  {
    int i;
#ifdef KEEP_POSITIONS_INDICES
    if (!m_pSysPosData)
      m_pSysPosData = new Vec3f16[m_nVerts];
    for (i=0; i<m_nVerts; i++)
    {
      m_pSysPosData[i] = m_pCachePos[i];
    }
#endif
    int nStride;
    byte *pDst = (byte *)LockVB(nStream, m_nFlagsCachePos, 0, m_nVerts, &nStride);
    assert(pDst);
    if (pDst)
    {
      for (i=0; i<m_nVerts; i++)
      {
        Vec3f16 *pVDst = (Vec3f16 *)pDst;
        *pVDst = m_pCachePos[i];
        pDst += nStride;
      }
    }
    m_nFlagsCachePos = 0;
  }
#endif
  m_VBStream[nStream].m_nLockFlags &= ~(FSL_WRITE | FSL_READ | FSL_SYSTEM | FSL_VIDEO);
}
void CRenderMesh2::UnlockIndexStream()
{
  UnlockIB();
  //if (m_IBStream.m_nLockFlags & FSL_WRITE)
  //  m_IBStream.m_nFrameUpdate = m_IBStream.m_nFrameRequest-1;
  m_IBStream.m_nLockFlags &= ~(FSL_WRITE | FSL_READ | FSL_SYSTEM | FSL_VIDEO);
}

bool CRenderMesh2::CopyIBToSystemForUpdate()
{
  //ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)
  if (!m_IBStream.m_pUpdateData)
	{
		AUTO_LOCK(m_sResLock);

		uint32 nSize = m_nInds * sizeof(uint16);
		int nIB = m_IBStream.m_nDevBuf;
		if (nIB < 0)
			return false;
		byte *pD = new byte[nSize];
		m_IBStream.m_pUpdateData = pD;
		void *pSrc = m_IBStream.m_pLockedData;
		if (!pSrc)
		{
			pSrc = gRenDev->m_DevBufMan.LockIB(nIB);
			m_IBStream.m_nLockFlags |= FSL_LOCKED;
		}
		assert(pSrc);
		if (!pSrc)
			return false;
		cryMemcpy(m_IBStream.m_pUpdateData, pSrc, m_nInds * sizeof(uint16));
		if (m_IBStream.m_nLockFlags & FSL_LOCKED)
		{
			m_IBStream.m_nLockFlags &= ~FSL_LOCKED;
			gRenDev->m_DevBufMan.UnlockIB(nIB);
		}
		m_IBStream.m_pLockedData = pSrc;
	}
  return true;
}

bool CRenderMesh2::CopyVBToSystemForUpdate(int nStream)
{
  //ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)

  if (!m_VBStream[nStream].m_pUpdateData)
	{
	  AUTO_LOCK(m_sResLock);
		uint32 nSize = GetStreamSize(nStream);
		int nVB = m_VBStream[nStream].m_nDevBuf;
		if (nVB < 0)
			return false;
		byte *pD = new byte[nSize];
		m_VBStream[nStream].m_pUpdateData = (uint16 *)pD;
		void *pSrc = m_VBStream[nStream].m_pLockedData;
		if (!pSrc)
		{
			pSrc = gRenDev->m_DevBufMan.LockVB(nVB);
			m_VBStream[nStream].m_nLockFlags |= FSL_LOCKED;
		}
		assert(pSrc);
		if (!pSrc)
			return false;
		cryMemcpy(m_VBStream[nStream].m_pUpdateData, pSrc, nSize);
		if (m_VBStream[nStream].m_nLockFlags & FSL_LOCKED)
		{
			m_VBStream[nStream].m_nLockFlags &= ~FSL_LOCKED;
			gRenDev->m_DevBufMan.UnlockVB(nVB);
		}
		m_VBStream[nStream].m_pLockedData = pSrc;
	}
  return true;
}

void CRenderMesh2::SetMesh (CMesh &mesh, int nSecColorsSetOffset, uint32 flags, const Vec3 *pPosOffset)	
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMesh, 0, "%s", this->GetSourceName());
	MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", this->GetTypeName());

	if (m_pChunksSkinned)
	{
		delete m_pChunksSkinned;
		m_pChunksSkinned = 0;
	}

  m_vBoxMin = mesh.m_bbox.min;
  m_vBoxMax = mesh.m_bbox.max;

  //////////////////////////////////////////////////////////////////////////
  // Initialize Render Chunks.
  //////////////////////////////////////////////////////////////////////////
  uint32 numSubsets = mesh.GetSubSetCount();
  for (uint32 i=0; i<numSubsets; i++)
  {
    CRenderChunk ChunkInfo;

    if (mesh.m_subsets[i].nNumIndices == 0)
      continue;

    if(mesh.m_subsets[i].nMatFlags & MTL_FLAG_NODRAW)
      continue;

    //add empty chunk, because PodArray is not working with STL-vectors
    m_Chunks.Add(ChunkInfo);

    uint32 num = m_Chunks.Count();
    CRenderChunk* pChunk = &m_Chunks[num-1];

    pChunk->nFirstIndexId = mesh.m_subsets[i].nFirstIndexId;
    pChunk->nNumIndices   = mesh.m_subsets[i].nNumIndices;
    pChunk->nFirstVertId  = mesh.m_subsets[i].nFirstVertId;
    pChunk->nNumVerts     = mesh.m_subsets[i].nNumVerts;
    pChunk->m_nMatID      = mesh.m_subsets[i].nMatID;
    pChunk->m_nMatFlags   = mesh.m_subsets[i].nMatFlags;
    if (mesh.m_subsets[i].nPhysicalizeType==PHYS_GEOM_TYPE_NONE)
      pChunk->m_nMatFlags |= MTL_FLAG_NOPHYSICALIZE;
    
    pChunk->m_texelAreaDensity = gRenDev->CalculateTexelAreaDensity(mesh,i, GetSourceName());

#define VALIDATE_CHUCKS
#if defined(_DEBUG) && defined(VALIDATE_CHUCKS)
    size_t indStart( pChunk->nFirstIndexId );
    size_t indEnd( pChunk->nFirstIndexId + pChunk->nNumIndices );
    for( size_t j( indStart ); j < indEnd; ++j )
    {
      size_t vtxStart( pChunk->nFirstVertId );
      size_t vtxEnd( pChunk->nFirstVertId + pChunk->nNumVerts );
      size_t curIndex0( mesh.m_pIndices[ j ] ); // absolute indexing
      size_t curIndex1( mesh.m_pIndices[ j ] + vtxStart ); // relative indexing using base vertex index
      assert( ( curIndex0 >= vtxStart && curIndex0 < vtxEnd ) || ( curIndex1 >= vtxStart && curIndex1 < vtxEnd ) ) ;
    }
#endif
    
    if (mesh.m_pBoneMapping)
    {
			pChunk->m_arrChunkBoneIDs  = mesh.m_subsets[i].m_arrGlobalBonesPerSubset;
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Create RenderElements.
  //////////////////////////////////////////////////////////////////////////
  const bool cbSetDecompressionMatrix = (mesh.m_pSHInfo && mesh.m_pSHInfo->nDecompressionCount == mesh.GetSubSetCount());
  int nCurChunk = 0;
  for (int i=0; i<mesh.GetSubSetCount(); i++)
  {
    SMeshSubset &subset = mesh.m_subsets[i];
    if (subset.nNumIndices == 0)
      continue;

    if(subset.nMatFlags & MTL_FLAG_NODRAW)
      continue;

    CRenderChunk *pRenderChunk = &m_Chunks[nCurChunk++];
    CREMesh *pRenderElement = (CREMesh*)gRenDev->EF_CreateRE(eDATA_Mesh);

    // Cross link render chunk with render element.
    pRenderChunk->pRE = pRenderElement;
    AssignChunk(pRenderChunk, pRenderElement);
    if (subset.nNumVerts <= 500 && !mesh.m_pBoneMapping && !mesh.m_pShapeDeformation && !(flags & FSM_VOXELS) && !(flags & FSM_NO_TANGENTS))
      pRenderElement->mfUpdateFlags(FCEF_MERGABLE);

    bool bTwoSided = (pRenderChunk->m_nMatFlags & MTL_FLAG_2SIDED) != 0;
  }

  uint32 nVerts = mesh.GetVertexCount();
  uint32 nInds = mesh.GetIndexCount();

  //////////////////////////////////////////////////////////////////////////
  // Create system vertex buffer in system memory.
  //////////////////////////////////////////////////////////////////////////
	m_nVerts = nVerts;
  m_nInds = 0;
#ifdef FP16_MESH
  if (!(flags & FSM_VOXELS) && nVerts < RM_MAXVERTS_FOR_INSTANCING && CRenderer::CV_r_geominstancing == 3)
    m_nFlags |= FRM_INSTANCED;
#endif

  SVF_P3S_C4B_T2S * pVBuff = NULL;
  SVF_P3S_N4B_C4B_T2S * pVBuffV = NULL;
  if (!(flags & FSM_VOXELS))
  {
    m_eVF = eVF_P3S_C4B_T2S;
    pVBuff = (SVF_P3S_C4B_T2S *)LockVB(VSF_GENERAL, FSL_SYSTEM_CREATE);
		
		// stop initializing if allocation failed
		if( pVBuff == NULL ) { m_nVerts = 0; return; }
		
  }
  else
  {
    m_eVF = eVF_P3S_N4B_C4B_T2S;
    pVBuffV = (SVF_P3S_N4B_C4B_T2S *)LockVB(VSF_GENERAL, FSL_SYSTEM_CREATE);

		// stop initializing if allocation failed
		if( pVBuffV == NULL ) { m_nVerts = 0; return; }	
  }			

  SPipTangents *pTBuff = NULL;
  SQTangents *pQTBuff = NULL;
  if (!(flags & FSM_NO_TANGENTS))
	{

		if (mesh.m_pQTangents)
			pQTBuff = (SQTangents *)LockVB(VSF_QTANGENTS, FSL_SYSTEM_CREATE);
		else
		    pTBuff = (SPipTangents *)LockVB(VSF_TANGENTS, FSL_SYSTEM_CREATE);
		
		// stop initializing if allocation failed
		if (pTBuff == NULL && pQTBuff == NULL)
      return;
	}

  //////////////////////////////////////////////////////////////////////////
  // Copy sh coefficient stream.
  //////////////////////////////////////////////////////////////////////////
  if (mesh.m_pSHInfo && mesh.m_pSHInfo->pSHCoeffs)
  {
    SVF_C4B_C4B *pSHBuf = (SVF_C4B_C4B *)LockVB(VSF_SH_INFO, FSL_SYSTEM_CREATE);

		// stop initializing if allocation failed
		if( pSHBuf == NULL ) {return; }
		
    for (uint32 i=0; i<nVerts; ++i)
    {
      pSHBuf[i].coef0 = *(UCol*)(&mesh.m_pSHInfo->pSHCoeffs[i].coeffs[0]);
      pSHBuf[i].coef1 = *(UCol*)(&mesh.m_pSHInfo->pSHCoeffs[i].coeffs[4]);
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Copy positions and normals stream.
  //////////////////////////////////////////////////////////////////////////
  if (pVBuff)
  {
    if(pPosOffset)
    {
      for (uint32 i=0; i<nVerts; ++i)
        pVBuff[i].xyz = mesh.m_pPositions[i] - *pPosOffset;
    }
    else
    {
      for (uint32 i=0; i<nVerts; ++i)
        pVBuff[i].xyz = mesh.m_pPositions[i];
    }
  }
  else
  {
    if(pPosOffset)
    {
      for (uint32 i=0; i<nVerts; ++i)
      {
        pVBuffV[i].xyz = mesh.m_pPositions[i] - *pPosOffset;
        pVBuffV[i].normal.bcolor[0] = (byte)(mesh.m_pNorms[i][0] * 127.5f + 128.0f);
        pVBuffV[i].normal.bcolor[1] = (byte)(mesh.m_pNorms[i][1] * 127.5f + 128.0f);
        pVBuffV[i].normal.bcolor[2] = (byte)(mesh.m_pNorms[i][2] * 127.5f + 128.0f);
        SwapEndian(pVBuffV[i].normal.dcolor);
      }
    }
    else
    {
      for (uint32 i=0; i<nVerts; ++i)
      {
        pVBuffV[i].xyz = mesh.m_pPositions[i];
        pVBuffV[i].normal.bcolor[0] = (byte)(mesh.m_pNorms[i][0] * 127.5f + 128.0f);
        pVBuffV[i].normal.bcolor[1] = (byte)(mesh.m_pNorms[i][1] * 127.5f + 128.0f);
        pVBuffV[i].normal.bcolor[2] = (byte)(mesh.m_pNorms[i][2] * 127.5f + 128.0f);
        SwapEndian(pVBuffV[i].normal.dcolor);
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Copy Texture coords stream.
  //////////////////////////////////////////////////////////////////////////
  if (pVBuff)
  {
    if (mesh.m_pTexCoord)
    {
      for (uint32 i=0; i<nVerts; ++i)
      {
        pVBuff[i].st = Vec2f16(mesh.m_pTexCoord[i].s, mesh.m_pTexCoord[i].t);
      }
    }
  }
  else
  if(pVBuffV)
  {
    if (mesh.m_pTexCoord)
    {
      for (uint32 i=0; i<nVerts; ++i)
      {
        pVBuffV[i].st = Vec2f16(mesh.m_pTexCoord[i].s, mesh.m_pTexCoord[i].t);
        //pVBuffV[i].st[0] = mesh.m_pTexCoord[i].s;
        //pVBuffV[i].st[1] = mesh.m_pTexCoord[i].t;
      }
    }
    else
    {
      // Value less than -1 tells vertex program to use usual (old) terrain tex gen (temporary)
      // Real coordinates from vertex are used only in case of 3d terrain research (CVoxTerrain)
      for (uint32 i=0; i<nVerts; ++i)
      {
        pVBuffV[i].st = Vec2f16(-100.f, -100.f);
        //pVBuffV[i].st[0] = -100.f;
        //pVBuffV[i].st[1] = -100.f;
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Copy color streams.
  //////////////////////////////////////////////////////////////////////////
  if (mesh.m_pColor0)
  {
    if (pVBuff)
    {
      for (uint32 i=0; i<nVerts; ++i)
      {
        pVBuff[i].color.bcolor[0] = mesh.m_pColor0[i].b;
        pVBuff[i].color.bcolor[1] = mesh.m_pColor0[i].g;
        pVBuff[i].color.bcolor[2] = mesh.m_pColor0[i].r;
        pVBuff[i].color.bcolor[3] = mesh.m_pColor0[i].a;
        SwapEndian(pVBuff[i].color.dcolor);
      }
    }
    else
    {
      for (uint32 i=0; i<nVerts; ++i)
      {
        pVBuffV[i].color.bcolor[0] = mesh.m_pColor0[i].b;
        pVBuffV[i].color.bcolor[1] = mesh.m_pColor0[i].g;
        pVBuffV[i].color.bcolor[2] = mesh.m_pColor0[i].r;
        pVBuffV[i].color.bcolor[3] = mesh.m_pColor0[i].a;
        SwapEndian(pVBuffV[i].color.dcolor);
      }
    }
  }
  else
  {
    if (pVBuff)
    {
      for (uint32 i=0; i<nVerts; ++i)
      {
        pVBuff[i].color.dcolor = ~0;
      }
    }
    else
    {
      assert(0);
    }
  }

  if (mesh.m_pColor1)
  {
    if (pVBuff)
    {
      assert(0);
    }
    else
    {
      for (uint32 i=0; i<nVerts; i++)
      {
#ifndef XENON
        pVBuffV[i].normal.bcolor[3] = mesh.m_pColor1[i].b;


#endif
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Copy tangent space stream.
  //////////////////////////////////////////////////////////////////////////
  if (mesh.m_pTangents && pTBuff)
  {
    for (uint32 i=0; i<nVerts; ++i)
    {
      pTBuff[i].Binormal = mesh.m_pTangents[i].Binormal;
      pTBuff[i].Tangent = mesh.m_pTangents[i].Tangent;
    }
  }

  if (mesh.m_pQTangents && pQTBuff)
  {
	  for (uint32 i=0; i<nVerts; ++i)
	  {
		  pQTBuff[i].Tangent = mesh.m_pQTangents[i].TangentBinormal;
	  }
  }

  //////////////////////////////////////////////////////////////////////////
  // Copy skin-streams.
  //////////////////////////////////////////////////////////////////////////
  if (mesh.m_pBoneMapping) 
    SetSkinningDataCharacter(mesh, mesh.m_pBoneMapping);

  //////////////////////////////////////////////////////////////////////////
  // Copy shape deformation data.
  //////////////////////////////////////////////////////////////////////////
  if (mesh.m_pShapeDeformation) 
  {
    SVF_P3F_P3F_I4B *pShapeDeformBuff = (SVF_P3F_P3F_I4B *)LockVB(VSF_HWSKIN_SHAPEDEFORM_INFO, FSL_SYSTEM_CREATE);
    for (uint32 i=0; i<nVerts; i++)
    {
      pShapeDeformBuff[i].thin            = mesh.m_pShapeDeformation[i].thin;
      pShapeDeformBuff[i].fat             = mesh.m_pShapeDeformation[i].fat;
      pShapeDeformBuff[i].index.bcolor[0] = mesh.m_pShapeDeformation[i].index.b;
      pShapeDeformBuff[i].index.bcolor[1] = mesh.m_pShapeDeformation[i].index.g;
      pShapeDeformBuff[i].index.bcolor[2] = mesh.m_pShapeDeformation[i].index.r;
      pShapeDeformBuff[i].index.bcolor[3] = mesh.m_pShapeDeformation[i].index.a;
      SwapEndian(pShapeDeformBuff[i].index.dcolor);
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // create buffer for morph-targets.
  //////////////////////////////////////////////////////////////////////////
  if (flags & FSM_MORPH_TARGETS)
  {
    SVF_P3F *pMorphTargets = (SVF_P3F *)LockVB(VSF_HWSKIN_MORPHTARGET_INFO, FSL_SYSTEM_CREATE);		

		// stop initializing if allocation failed
		if( pMorphTargets == NULL ) {return; }
		
    // Initialize morph targets buffer.
    memset(pMorphTargets, 0, GetStreamSize(VSF_HWSKIN_MORPHTARGET_INFO));
  }

  //////////////////////////////////////////////////////////////////////////
  // Copy indices.
  //////////////////////////////////////////////////////////////////////////
	m_nInds = nInds;
  uint16 *pInds = LockIB(FSL_SYSTEM_CREATE);

	// stop initializing if allocation failed
	if( m_nInds && pInds == NULL )
	{
		m_nInds = 0;
		return;
	}

  memcpy(pInds, &mesh.m_pIndices[0], m_nInds*sizeof(uint16));
	
  // Create device buffers immediately in non-multithreaded mode
  if (!gRenDev->m_pRT->IsMultithreaded())
  {
    if (flags & FSM_CREATE_DEVICE_MESH)
      CheckUpdate(m_eVF, VSM_MASK);
  }
}

void CRenderMesh2::SetSkinningDataVegetation(struct SMeshBoneMapping *pBoneMapping)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMesh, 0, "%s", this->GetSourceName());
	MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", this->GetTypeName());

  //  struct_VERTEX_FORMAT_WEIGHTS4UB_INDICES4UB *pSkinBuff = new struct_VERTEX_FORMAT_WEIGHTS4UB_INDICES4UB[nVerts];
  SVF_W4B_I4B_P3F *pSkinBuff = (SVF_W4B_I4B_P3F *)LockVB(VSF_HWSKIN_INFO, FSL_SYSTEM_CREATE);
	
	// stop initializing if allocation failed
	if( pSkinBuff == NULL ) return;
		 
  for (int32 i = 0; i < m_nVerts; i++ )
  {
    // get bone IDs
    uint16 b0 = pBoneMapping[i].boneIDs[0];
    uint16 b1 = pBoneMapping[i].boneIDs[1];
    uint16 b2 = pBoneMapping[i].boneIDs[2];
    uint16 b3 = pBoneMapping[i].boneIDs[3];

    // get weights
    f32 w0 = pBoneMapping[i].weights[0];
    f32 w1 = pBoneMapping[i].weights[1];
    f32 w2 = pBoneMapping[i].weights[2];
    f32 w3 = pBoneMapping[i].weights[3];

    // if weight is zero set bone ID to zero as the bone has no influence anyway,
    // this will fix some issue with incorrectly exported models (e.g. system freezes on ATI cards when access invalid bones)
    if (w0 == 0) b0 = 0;
    if (w1 == 0) b1 = 0;
    if (w2 == 0) b2 = 0;
    if (w3 == 0) b3 = 0;											








    pSkinBuff[i].indices.bcolor[0] = (uint8)b0;
    pSkinBuff[i].indices.bcolor[1] = (uint8)b1;
    pSkinBuff[i].indices.bcolor[2] = (uint8)b2;
    pSkinBuff[i].indices.bcolor[3] = (uint8)b3;

    // copy weights






    pSkinBuff[i].weights.bcolor[0] = (uint8)w0;
    pSkinBuff[i].weights.bcolor[1] = (uint8)w1;
    pSkinBuff[i].weights.bcolor[2] = (uint8)w2;
    pSkinBuff[i].weights.bcolor[3] = (uint8)w3;

  //  if (pBSStreamTemp)
  //    pSkinBuff[i].boneSpace  = pBSStreamTemp[i];
  }

}

void CRenderMesh2::SetSkinningDataCharacter(CMesh& mesh, struct SMeshBoneMapping *pBoneMapping)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMesh, 0, "%s", this->GetSourceName());
	MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", this->GetTypeName());

  SVF_W4B_I4B_P3F *pSkinBuff = (SVF_W4B_I4B_P3F *)LockVB(VSF_HWSKIN_INFO, FSL_SYSTEM_CREATE);

	// stop initializing if allocation failed
	if( pSkinBuff == NULL ) {return; }

  for (int32 i=0; i<m_nVerts; i++ )
  {
    // get bone IDs
    uint16 b0 = pBoneMapping[i].boneIDs[0];
    uint16 b1 = pBoneMapping[i].boneIDs[1];
    uint16 b2 = pBoneMapping[i].boneIDs[2];
    uint16 b3 = pBoneMapping[i].boneIDs[3];

    // get weights
    f32 w0 = pBoneMapping[i].weights[0];
    f32 w1 = pBoneMapping[i].weights[1];
    f32 w2 = pBoneMapping[i].weights[2];
    f32 w3 = pBoneMapping[i].weights[3];

    // if weight is zero set bone ID to zero as the bone has no influence anyway,
    // this will fix some issue with incorrectly exported models (e.g. system freezes on ATI cards when access invalid bones)
    if (w0 == 0) b0 = 0;
    if (w1 == 0) b1 = 0;
    if (w2 == 0) b2 = 0;
    if (w3 == 0) b3 = 0;											








    pSkinBuff[i].indices.bcolor[0] = (uint8)b0;
    pSkinBuff[i].indices.bcolor[1] = (uint8)b1;
    pSkinBuff[i].indices.bcolor[2] = (uint8)b2;
    pSkinBuff[i].indices.bcolor[3] = (uint8)b3;

    // copy weights






    pSkinBuff[i].weights.bcolor[0] = (uint8)w0;
    pSkinBuff[i].weights.bcolor[1] = (uint8)w1;
    pSkinBuff[i].weights.bcolor[2] = (uint8)w2;
    pSkinBuff[i].weights.bcolor[3] = (uint8)w3;

 //   if (pBSStreamTemp)
 //     pSkinBuff[i].boneSpace  = pBSStreamTemp[i];
  }
}

IIndexedMesh *CRenderMesh2::GetIndexedMesh(IIndexedMesh *pIdxMesh)
{
  if (!pIdxMesh)
    pIdxMesh = gEnv->p3DEngine->CreateIndexedMesh();

	// cach failed allocation of IndexedMesh
	if( pIdxMesh == NULL )
		return NULL;

  CMesh *pMesh = pIdxMesh->GetMesh();
  int i,j;

  pIdxMesh->SetVertexCount(m_nVerts);
  pIdxMesh->SetTexCoordsAndTangentsCount(m_nVerts);
  pIdxMesh->SetIndexCount(m_nInds);
  pIdxMesh->SetSubSetCount(m_Chunks.size());

  strided_pointer<Vec3> pVtx;
  strided_pointer<Vec4sf> pTang,pBinorm;
  strided_pointer<Vec2f16> pTex;
  pVtx.data = (Vec3*)GetPosPtr(pVtx.iStride, FSL_READ);
  //pNorm.data = (Vec3*)GetNormalPtr(pNorm.iStride);
  pTex.data = (Vec2f16*)GetUVPtr(pTex.iStride, FSL_READ);
  pTang.data = (Vec4sf*)GetTangentPtr(pTang.iStride, FSL_READ);
  pBinorm.data = (Vec4sf*)GetBinormalPtr(pBinorm.iStride, FSL_READ);

	// don't copy if some src, or dest buffer is NULL (can happen because of failed allocations)
	if(		pVtx.data == NULL			|| pMesh->m_pPositions == NULL ||
				pTex.data == NULL			|| pMesh->m_pTexCoord == NULL ||
				pTang.data == NULL		|| pMesh->m_pTangents  == NULL ||
				pBinorm.data == NULL )
	{		
		delete pIdxMesh;
		return NULL; 
	}

  for(i=0;i<m_nVerts;i++)
  {
    pMesh->m_pPositions[i] = pVtx[i];
    pMesh->m_pNorms[i] = Vec3(0,0,1); //pNorm[i];
    Vec2 st = pTex[i].ToVec2();
    pMesh->m_pTexCoord[i].s = st.x;
    pMesh->m_pTexCoord[i].t = st.y;
    pMesh->m_pTangents[i].Binormal = pBinorm[i];
    pMesh->m_pTangents[i].Tangent = pTang[i];
  }

  if (m_eVF==eVF_P3S_C4B_T2S || m_eVF==eVF_P3S_N4B_C4B_T2S)
  {
    strided_pointer<SMeshColor> pColors;
    pColors.data = (SMeshColor*)GetColorPtr(pColors.iStride, FSL_READ);
    pIdxMesh->SetColorsCount(m_nVerts);
    for(i=0; i<m_nVerts; i++) {






      pMesh->m_pColor0[i] = pColors[i];

    }
  }
  UnlockStream(VSF_GENERAL);

  uint16 *pInds = GetIndexPtr(FSL_READ);
  for(i=0;i<(int)m_nInds;i++)
    pMesh->m_pIndices[i] = pInds[i];

  SVF_W4B_I4B_P3F *pSkinBuff = (SVF_W4B_I4B_P3F*)LockVB(VSF_HWSKIN_INFO, FSL_READ);
  if (pSkinBuff)
  {
    pIdxMesh->AllocateBoneMapping();
    for(i=0;i<m_nVerts;i++) for(j=0;j<4;j++)
    {
      pMesh->m_pBoneMapping[i].boneIDs[j] = pSkinBuff[i].indices.bcolor[j];
      pMesh->m_pBoneMapping[i].weights[j] = pSkinBuff[i].weights.bcolor[j];
    }
  }

  SVF_C4B_C4B *pSHBuf = (SVF_C4B_C4B*)LockVB(VSF_SH_INFO, FSL_READ);
  if (pSHBuf) 
  {
    pIdxMesh->AllocateSHData();
    for(i=0;i<m_nVerts;i++) for(j=0;j<8;j++)
      pMesh->m_pSHInfo->pSHCoeffs[i].coeffs[j] = ((uint8*)&pSHBuf[i].coef0)[j];
  }

  for(i=0;i<(int)m_Chunks.size();i++)
  {
    SMeshSubset &mss = pIdxMesh->GetSubSet(i);
    mss.nFirstIndexId = m_Chunks[i].nFirstIndexId;
    mss.nNumIndices		= m_Chunks[i].nNumIndices;
    mss.nFirstVertId	= m_Chunks[i].nFirstVertId;
    mss.nNumVerts			= m_Chunks[i].nNumVerts;
    mss.nMatID				= m_Chunks[i].m_nMatID;
    mss.nMatFlags			= m_Chunks[i].m_nMatFlags;
    mss.nPhysicalizeType = (m_Chunks[i].m_nMatFlags & MTL_FLAG_NOPHYSICALIZE) ? PHYS_GEOM_TYPE_NONE :
      ((m_Chunks[i].m_nMatFlags & MTL_FLAG_NODRAW) ? PHYS_GEOM_TYPE_OBSTRUCT : PHYS_GEOM_TYPE_DEFAULT);
    pIdxMesh->SetSubsetBoneIds(i,m_Chunks[i].m_arrChunkBoneIDs);
    for(j=mss.nFirstIndexId,mss.vCenter.zero(); j<mss.nFirstIndexId+mss.nNumIndices; j++)
      mss.vCenter += pMesh->m_pPositions[pMesh->m_pIndices[j]];
    if (mss.nNumIndices)
      mss.vCenter /= (float)mss.nNumIndices;
    for(j=mss.nFirstIndexId,mss.fRadius=0; j<mss.nFirstIndexId+mss.nNumIndices; j++)
      mss.fRadius = max(mss.fRadius, (pMesh->m_pPositions[pMesh->m_pIndices[j]]-mss.vCenter).len2());
    mss.fRadius = sqrt_tpl(mss.fRadius);
  }

  return pIdxMesh;
}

void CRenderMesh2::CreateChunksSkinned()
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMesh, 0, "%s", this->GetSourceName());
	MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", this->GetTypeName());

  PodArray<CRenderChunk>& arrSrcMats = m_Chunks;
  PodArray<CRenderChunk>& arrNewMats = *(m_pChunksSkinned = new PodArray<CRenderChunk>);
  arrNewMats.resize (arrSrcMats.Size());
  for (uint32 i=0; i<arrSrcMats.size(); ++i)
  {
    CRenderChunk& rSrcMat = arrSrcMats[i]; 
    CRenderChunk& rNewMat = arrNewMats[i];
    rNewMat = rSrcMat;
    CREMesh *re = rSrcMat.pRE;
    if (re)
    {
      rNewMat.pRE = (CREMesh *)gRenDev->EF_CreateRE(eDATA_Mesh);
      CRendElement *pNext = rNewMat.pRE->m_NextGlobal;
      CRendElement *pPrev = rNewMat.pRE->m_PrevGlobal;
      *rNewMat.pRE = *re;
      if (rNewMat.pRE->m_pChunk) // affects the source mesh!! will only work correctly if the source is deleted after copying
        rNewMat.pRE->m_pChunk = &rNewMat;
      rNewMat.pRE->m_NextGlobal = pNext;
      rNewMat.pRE->m_PrevGlobal = pPrev;
      rNewMat.pRE->m_pRenderMesh = this;
      rNewMat.pRE->m_CustomData = NULL;
    }
  }
}

int CRenderMesh2::GetRenderChunksCount(IMaterial * pMaterial, int & nRenderTrisCount)
{
  int nCount = 0;
  nRenderTrisCount = 0;

  CRenderer *rd = gRenDev;
  const uint32 ni = (uint32)m_Chunks.Count();
  for (uint32 i=0; i<ni; i++)     
  {
    CRenderChunk *pChunk = &m_Chunks[i];
    CRendElementBase * pREMesh = pChunk->pRE;

    SShaderItem *pShaderItem = &pMaterial->GetShaderItem(pChunk->m_nMatID);

    SRenderShaderResources *pR = (SRenderShaderResources *)pShaderItem->m_pShaderResources;
    CShader *pS = (CShader *)pShaderItem->m_pShader;
    if (pREMesh && pS && pR)
    {
      if(pChunk->m_nMatFlags & MTL_FLAG_NODRAW)
        continue;

      if (pS->m_Flags2 & EF2_NODRAW)
        continue;

      if(rd->CV_r_VegetationAlphaTestOnly==1 && pS->GetShaderType() == eST_Vegetation)
        if(!pR->GetAlphaRef())
          continue;

      if(pChunk->nNumIndices)
      {
        nRenderTrisCount += pChunk->nNumIndices/3;
        nCount++;
      }
    }
  }

  return nCount;
}

void CRenderMesh2::CopyTo(IRenderMesh *_pDst, int nAppendVtx, bool bDynamic)
{
  CRenderMesh2 *pDst = (CRenderMesh2 *)_pDst;
  PodArray<CRenderChunk>& arrSrcMats = m_Chunks;
  PodArray<CRenderChunk>& arrNewMats = pDst->m_Chunks;
  //pDst->m_bMaterialsWasCreatedInRenderer  = true;
  arrNewMats.resize (arrSrcMats.Size());
  if (bDynamic)
    pDst->m_eType = eRMT_KeepSystem;

  uint32 i;
  for (i=0; i<arrSrcMats.size(); ++i)
  {
    CRenderChunk& rSrcMat = arrSrcMats[i]; 
    CRenderChunk& rNewMat = arrNewMats[i];
    rNewMat = rSrcMat;
    rNewMat.nNumVerts	+= ((m_nVerts-2-rNewMat.nNumVerts-rNewMat.nFirstVertId)>>31) & nAppendVtx;
    CREMesh *re = rSrcMat.pRE;
    if (re)
    {
      rNewMat.pRE = (CREMesh *)gRenDev->EF_CreateRE(eDATA_Mesh);
      CRendElement *pNext = rNewMat.pRE->m_NextGlobal;
      CRendElement *pPrev = rNewMat.pRE->m_PrevGlobal;
      *rNewMat.pRE = *re;
      if (rNewMat.pRE->m_pChunk) // affects the source mesh!! will only work correctly if the source is deleted after copying
      {
        rNewMat.pRE->m_pChunk = &rNewMat;
        rNewMat.pRE->m_pChunk->nNumVerts += ((m_nVerts-2-re->m_pChunk->nNumVerts-re->m_pChunk->nFirstVertId)>>31) & nAppendVtx;
      }
      rNewMat.pRE->m_NextGlobal = pNext;
      rNewMat.pRE->m_PrevGlobal = pPrev;
      rNewMat.pRE->m_pRenderMesh = pDst;
      //assert(rNewMat.pRE->m_CustomData);
      rNewMat.pRE->m_CustomData = NULL;
    }
  }

  pDst->m_nVerts = m_nVerts+nAppendVtx;
  pDst->m_eVF = m_eVF;
  for (i=0; i<VSF_NUM; i++)
  {
    void *pSrcD = LockVB(i, FSL_READ);
    if (pSrcD)
    {
      void *pDstD = pDst->LockVB(i, FSL_SYSTEM_CREATE);
      assert(pDstD);
      if (pDstD)
        cryMemcpy(pDstD, pSrcD, GetStreamSize(i));
    }
    UnlockVB(i);
  }

  pDst->m_nInds = m_nInds;
  void *pSrcD = LockIB(FSL_READ);
  if (pSrcD)
  {
    void *pDstD = pDst->LockIB(FSL_SYSTEM_CREATE);
    assert(pDstD);
    if (pDstD)
      cryMemcpy(pDstD, pSrcD, m_nInds*sizeof(uint16));
  }
  UnlockIB();
}

// set effector for all chunks
void CRenderMesh2::SetMaterial( IMaterial * pNewMat, int nCustomTID )
{
  m_pMaterial = pNewMat;

  if (m_Chunks.Count() && nCustomTID != 0)
  {
    for(int i=0; i<m_Chunks.Count(); i++)
    {
      CRenderChunk *pChunk = &m_Chunks[i];
      //    pChunk->shaderItem.m_pShader = pShader;
      if (pChunk->pRE)
        pChunk->pRE->m_CustomTexBind[0] = nCustomTID;
    }
  }
}

void CRenderMesh2::SetChunk(int nIndex, CRenderChunk &inChunk, bool bForceInitChunk)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMesh, 0, "%s", this->GetSourceName());
	MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", this->GetTypeName());

  if (!inChunk.nNumIndices || !inChunk.nNumVerts)
    return;

  CRenderChunk *pMat = NULL;

  if (nIndex < 0 || nIndex >= m_Chunks.Count())
  {
    // add new chunk
    CRenderChunk matinfo;
    m_Chunks.Add(matinfo);
    pMat = &m_Chunks.Last();

    if (m_Chunks.Count()>1 && !bForceInitChunk)
      pMat->pRE = 0;
    else
    {
      pMat->pRE = (CREMesh*)gRenDev->EF_CreateRE(eDATA_Mesh);
      pMat->pRE->m_CustomTexBind[0] = m_nClientTextureBindID;
    }
    nIndex = m_Chunks.Count()-1;
  }
  else
  {
    // use present chunk
    pMat = &m_Chunks[nIndex];
    if (!pMat)
      return;
  }

  pMat->m_nMatID = inChunk.m_nMatID;
  pMat->m_nMatFlags = inChunk.m_nMatFlags;

  pMat->nFirstIndexId	= inChunk.nFirstIndexId;
  pMat->nNumIndices		= max(inChunk.nNumIndices,0);
  pMat->nFirstVertId	= inChunk.nFirstVertId;
  pMat->nNumVerts			= max(inChunk.nNumVerts,0);

  pMat->m_texelAreaDensity = inChunk.m_texelAreaDensity;

  // update chunk RE
  if (pMat->pRE)
    AssignChunk(pMat, pMat->pRE);
  assert(!pMat->pRE || pMat->pRE->m_pChunk->nFirstIndexId<60000);
  assert(pMat->nFirstIndexId + pMat->nNumIndices <= m_nInds);
}

void CRenderMesh2::SetChunk(IMaterial *pNewMat, int nFirstVertId, int nVertCount, int nFirstIndexId, int nIndexCount, float texelAreaDensity, int nIndex, bool bForceInitChunk)
{
  CRenderChunk chunk;

  if (pNewMat)
    chunk.m_nMatFlags = pNewMat->GetFlags();

  if (nIndex < 0 || nIndex >= m_Chunks.Count())
    chunk.m_nMatID = m_Chunks.Count();
  else
    chunk.m_nMatID = nIndex;

  chunk.nFirstVertId = nFirstVertId;
  chunk.nNumVerts = nVertCount;

  chunk.nFirstIndexId = nFirstIndexId;
  chunk.nNumIndices = nIndexCount;

	chunk.m_texelAreaDensity = texelAreaDensity;

  SetChunk(nIndex, chunk, bForceInitChunk);
}
#endif//__SPU__
//================================================================================================================

#ifdef FP16_MESH
bool CRenderMesh2::CreateCachePos(byte *pSrc, uint32 nStrideSrc, uint nFlags)
{
  assert(gRenDev->m_pRT->IsMainThread());

  if (m_eVF != eVF_P3S_C4B_T2S && m_eVF != eVF_P3S_N4B_C4B_T2S)
    return false;

  m_nFlagsCachePos = nFlags;



  m_nFrameRequestCachePos = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;

  if ((nFlags & FSL_READ) && m_pCachePos)
    return true;
  if ((nFlags == FSL_SYSTEM_CREATE) && m_pCachePos)
    return true;
  if (!m_pCachePos)
    m_pCachePos = new Vec3 [m_nVerts];
  if (nFlags == FSL_SYSTEM_UPDATE || (nFlags & FSL_READ))
  {
    for (int i=0; i<m_nVerts; i++)
    {
      Vec3f16 *pVSrc = (Vec3f16 *)pSrc;
      m_pCachePos[i] = pVSrc->ToVec3();
      pSrc += nStrideSrc;
    }
  }
  return true;
}
#endif

byte *CRenderMesh2::GetPosPtr(int32& nStride, uint32 nFlags, int32 nOffset)
{
  int nStr = 0;
  byte *pData = NULL;
#ifdef KEEP_POSITIONS_INDICES
  if ((nFlags & FSL_READ) && m_pSysPosData)
  {
    pData = (byte *)m_pSysPosData;
#ifdef FP16_MESH
    if (CreateCachePos(pData, sizeof(Vec3f16), nFlags))
      pData = (byte *)m_pCachePos;
#endif
    if (pData)
    {
      nStride = sizeof(Vec3);
      return pData;
    }
  }
#endif
  pData = (byte *)LockVB(VSF_GENERAL, nFlags, nOffset, 0, &nStr);
  assert((m_nVerts == 0) || pData);
  if (!pData)
    return NULL;
#ifdef FP16_MESH
  if (!CreateCachePos(pData, nStr, nFlags))
  {
    nStride = nStr;
    return pData;
  }
  pData = (byte *)m_pCachePos;
  nStride = sizeof(Vec3);
  return pData;
#else
  nStride = nStr;
  return pData;
#endif
}

uint16 *CRenderMesh2::GetIndexPtr(uint32 nFlags, int32 nOffset)
{
  uint16 *pData = LockIB(nFlags, nOffset, 0);
  assert((m_nInds == 0) || pData);
  return pData;
}

#ifndef __SPU__
byte *CRenderMesh2::GetColorPtr(int32& nStride, uint32 nFlags, int32 nOffset)
{
  int nStr = 0;
  byte *pData = (byte *)LockVB(VSF_GENERAL, nFlags, nOffset, 0, &nStr);
  assert((m_nVerts == 0) || pData);
  if (!pData)
    return NULL;
  nStride = nStr;
  SBufInfoTable *pOffs = &m_cBufInfoTable[_GetVertexFormat()];
  if (pOffs->OffsColor)
    return &pData[pOffs->OffsColor];
  return NULL;
}
byte *CRenderMesh2::GetNormPtr(int32& nStride, uint32 nFlags, int32 nOffset)
{
  int nStr = 0;
  byte *pData = (byte *)LockVB(VSF_GENERAL, nFlags, nOffset, 0, &nStr);
  assert((m_nVerts == 0) || pData);
  if (!pData)
    return NULL;
  nStride = nStr;
  //assert(_GetVertexFormat() == eVF_P3S_N4B_C4B_T2S);
  if(_GetVertexFormat() == eVF_P3S_N4B_C4B_T2S)
    return pData + sizeof(Vec3f16);
  return NULL;
}
byte *CRenderMesh2::GetUVPtr(int32& nStride, uint32 nFlags, int32 nOffset)
{
  int nStr = 0;
  byte *pData = (byte *)LockVB(VSF_GENERAL, nFlags, nOffset, 0, &nStr);
  assert((m_nVerts == 0) || pData);
  if (!pData)
    return NULL;
  nStride = nStr;
  SBufInfoTable *pOffs = &m_cBufInfoTable[_GetVertexFormat()];
  if (pOffs->OffsTC)
    return &pData[pOffs->OffsTC];
  return NULL;
}
#endif
byte *CRenderMesh2::GetTangentPtr(int32& nStride, uint32 nFlags, int32 nOffset)
{
  int nStr = 0;
  byte *pData = (byte *)LockVB(VSF_TANGENTS, nFlags, nOffset, 0, &nStr);
  //assert((m_nVerts == 0) || pData);
  if (!pData)
    pData = (byte *)LockVB(VSF_QTANGENTS, nFlags, nOffset, 0, &nStr);
  if (!pData)
    return NULL;
  nStride = nStr;
  return pData;
}
byte *CRenderMesh2::GetBinormalPtr(int32& nStride, uint32 nFlags, int32 nOffset)
{
  int nStr = 0;
  byte *pData = (byte *)LockVB(VSF_TANGENTS, nFlags, nOffset, 0, &nStr);
  //assert((m_nVerts == 0) || pData);
  if (!pData)
    return NULL;
  nStride = nStr;
  return &pData[sizeof(Vec4sf)];
}

byte *CRenderMesh2::GetHWSkinPtr(int32& nStride, uint32 nFlags, int32 nOffset)
{
  int nStr = 0;
  byte *pData = (byte *)LockVB(VSF_HWSKIN_INFO, nFlags, nOffset, 0, &nStr);
  assert((m_nVerts == 0) || pData);
  if (!pData)
    return NULL;
  nStride = nStr;
  return pData;
}
#if !defined(__SPU__)
byte *CRenderMesh2::GetShapePtr(int32& nStride, uint32 nFlags, int32 nOffset)
{
  int nStr = 0;
  byte *pData = (byte *)LockVB(VSF_HWSKIN_SHAPEDEFORM_INFO, nFlags, nOffset, 0, &nStr);
  assert((m_nVerts == 0) || pData);
  if (!pData)
    return NULL;
  nStride = nStr;
  return pData;
}
byte *CRenderMesh2::GetMorphTargetPtr(int32& nStride, uint32 nFlags, int32 nOffset)
{
  int nStr = 0;
  byte *pData = (byte *)LockVB(VSF_HWSKIN_MORPHTARGET_INFO, nFlags, nOffset, 0, &nStr);
  assert((m_nVerts == 0) || pData);
  if (!pData)
    return NULL;
  nStride = nStr;
  return pData;
}

bool CRenderMesh2::IsEmpty()
{
	ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)
  return (!m_nVerts || (!_HasVBStream(VSF_GENERAL) && !m_VBStream[VSF_GENERAL].m_pUpdateData) || (!_HasIBStream() && !m_IBStream.m_pUpdateData));
}

//================================================================================================================

bool CRenderMesh2::CheckUpdate(EVertexFormat eVF, uint32 nStreamMask)
{
  CRenderMesh2 *pRM = _GetVertexContainer();
  return gRenDev->m_pRT->RC_CheckUpdate2(this, pRM, eVF, nStreamMask);
}
bool CRenderMesh2::RT_CheckUpdate(CRenderMesh2 *pVContainer, EVertexFormat eVF, uint32 nStreamMask)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMesh, 0, "%s", this->GetSourceName());
	MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", this->GetTypeName());

  assert (m_pVertexContainer || m_nVerts > 2);
  if (!m_pVertexContainer && m_nVerts < 2)
    return false;

  /*if (strstr(pVContainer->m_sSource, "building8"))
  {
    int nnn = 0;
  }*/

  CRenderer *rd = gRenDev;
  int nThreadID = rd->m_RP.m_nProcessThreadID;
  int nFrame = rd->m_RP.m_TI[nThreadID].m_nFrameUpdateID;
  if (pVContainer->m_VBStream[VSF_GENERAL].m_nFrameAccess != nFrame)
  {
    pVContainer->m_VBStream[VSF_GENERAL].m_nFrameAccess = nFrame;
    if (pVContainer->m_VBStream[VSF_GENERAL].m_nFrameRequest > pVContainer->m_VBStream[VSF_GENERAL].m_nFrameUpdate)
    {
      // Create the device buffer
      //pVContainer->m_nFrameUpdate = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameID;
      {
        PROFILE_FRAME(Mesh_CheckUpdateUpdateGBuf);
        assert(pVContainer->m_VBStream[VSF_GENERAL].m_pUpdateData);
        if (!pVContainer->UpdateVidVertices(VSF_GENERAL))
          return false;
        pVContainer->m_VBStream[VSF_GENERAL].m_nFrameUpdate = nFrame;
      }
    }
    else
    if (pVContainer->m_VBStream[VSF_GENERAL].m_nLockFlags & FSL_LOCKED)
      pVContainer->UnlockVB(VSF_GENERAL);
  }
  if (nStreamMask & VSM_TANGENTS)
    nStreamMask |= VSM_TANGENTS;

  // Additional streams updating
  if (nStreamMask & VSM_MASK)
  {
    int i;
    for (i=1; i<VSF_NUM; i++)
    {
      if (nStreamMask & (1<<i))
      {
        if (pVContainer->m_VBStream[i].m_nFrameAccess != nFrame)
        {
          pVContainer->m_VBStream[i].m_nFrameAccess = nFrame;
          if (pVContainer->m_VBStream[i].m_nFrameRequest > pVContainer->m_VBStream[i].m_nFrameUpdate)
          {
            // Update the device buffer
            assert(pVContainer->m_VBStream[i].m_pUpdateData);
            PROFILE_FRAME(Mesh_CheckUpdateUpdateGBuf);
            if (pVContainer->UpdateVidVertices(i))
              pVContainer->m_VBStream[i].m_nFrameUpdate = nFrame;
          }
          else
          if (pVContainer->m_VBStream[i].m_nLockFlags & FSL_LOCKED)
            pVContainer->UnlockVB(i);
        }
      }
    }
  }
  if (nStreamMask & VSM_TANGENTS)
  {
    if (pVContainer->_GetVBStream(VSF_QTANGENTS) > 0)
    {
      rd->m_RP.m_FlagsStreams_Stream &= ~VSM_TANGENTS;
      rd->m_RP.m_FlagsStreams_Decl &= ~VSM_TANGENTS;
      rd->m_RP.m_FlagsStreams_Stream |= (1<<VSF_QTANGENTS);
      rd->m_RP.m_FlagsStreams_Decl |= (1<<VSF_QTANGENTS);
    }
  }

  //if (m_IBStream.m_nFrameAccess != nFrame)
  {
    m_IBStream.m_nFrameAccess = nFrame;
    if (m_IBStream.m_nFrameRequest > m_IBStream.m_nFrameUpdate)
    {
      assert(m_IBStream.m_pUpdateData);
      PROFILE_FRAME(Mesh_CheckUpdate_UpdateInds);
      if (UpdateVidIndices())
        m_IBStream.m_nFrameUpdate = nFrame;
    }
    else
    if (m_IBStream.m_pLockedData)
      UnlockIB();
  }

  return true;
}

void CRenderMesh2::ReleaseVB(int nStream)
{
  UnlockVB(nStream);
  gRenDev->m_pRT->RC_ReleaseVB(m_VBStream[nStream].m_nDevBuf);
  m_VBStream[nStream].m_nDevBuf = -1;
  m_VBStream[nStream].m_nFrameUpdate = -1;
}

void CRenderMesh2::ReleaseIB()
{
  UnlockIB();
  gRenDev->m_pRT->RC_ReleaseIB(m_IBStream.m_nDevBuf);
  m_IBStream.m_nDevBuf = -1;
  m_IBStream.m_nFrameUpdate = -1;
}

bool CRenderMesh2::UpdateIndices(const uint16 *pNewInds, int nInds, int nOffsInd)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMesh, 0, "%s", this->GetSourceName());
	MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", this->GetTypeName());

	ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)

  AUTO_LOCK(m_sResLock);

  // Resize the index buffer
  if (m_nInds != nInds)
  {
    SAFE_DELETE_VOID_ARRAY(m_IBStream.m_pUpdateData);
    if (_HasIBStream())
      ReleaseIB();
    m_nInds = nInds;
  }
  if (!nInds)
  {
    assert(!m_IBStream.m_pUpdateData);
    return true;
  }

  uint16 *pDst = LockIB(FSL_SYSTEM_CREATE, 0, nInds);
  if (pDst && pNewInds)
    cryMemcpy(&pDst[nOffsInd], pNewInds, nInds*sizeof(uint16));
  else
    return false;

  return true;
}

bool CRenderMesh2::UpdateVertices(const void *pVertBuffer, int nVertCount, int nOffset, int nStream)
{
  ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)

  int nStride;

  AUTO_LOCK(m_sResLock);

  // Resize the vertex buffer
  if (m_nVerts != nVertCount)
  {
    for (int i=0; i<VSF_NUM; i++)
    {
      SAFE_DELETE_VOID_ARRAY(m_VBStream[i].m_pUpdateData);
      if (_HasVBStream(i))
        ReleaseVB(i);
    }
    m_nVerts = nVertCount;
  }
  if (!m_nVerts)
    return true;

  byte *pDstVB = (byte *)LockVB(nStream, FSL_SYSTEM_CREATE, 0, nVertCount, &nStride);
  assert((nVertCount == 0) || pDstVB);
  if (pDstVB && pVertBuffer)
    cryMemcpy(&pDstVB[nOffset], pVertBuffer, nStride*nVertCount);
  else
    return false;

  return true;
}

bool CRenderMesh2::UpdateVidIndices()
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMesh, 0, "%s", this->GetSourceName());
	MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", this->GetTypeName());

  assert(gRenDev->m_pRT->IsRenderThread());

  AUTO_LOCK(m_sResLock);

  MEM_CHECK
  assert(gRenDev->m_pRT->IsRenderThread());

  int nInds = m_nInds;
  if (m_nFlags & FRM_INSTANCED)
    nInds *= RM_INSTANCES;

	if (!nInds)
	{
		// 0 size index buffer creation crashes on 360
		assert( nInds );
		return false;
	}

  if (m_IBStream.m_nDevBuf < 0)
    m_IBStream.m_nDevBuf = gRenDev->m_DevBufMan.CreateIB(nInds*sizeof(uint16));
  if (m_IBStream.m_nDevBuf >= 0)
  {
    UnlockIndexStream();
    if (m_IBStream.m_pUpdateData)
    {
      bool bRes = true;
      int n = (m_nFlags & FRM_INSTANCED) ? RM_INSTANCES : 1;
      int nOffs = 0;
      uint16 *pTemp = NULL;
      for (int i=0; i<n; i++)
      {
        if (nOffs)
        {
          if (!pTemp)
            pTemp = new uint16[m_nInds];
          uint16 *pSrc = (uint16 *)m_IBStream.m_pUpdateData;
          int nV = m_nVerts*i;
          for (int j=0; j<m_nInds; j++)
          {
            pTemp[j] = pSrc[j] + nV;
          }
          if (!gRenDev->m_DevBufMan.UpdateIB(m_IBStream.m_nDevBuf, pTemp, m_nInds*sizeof(uint16), nOffs*sizeof(uint16)))
            bRes = false;
        }
        else
        {
          m_IBStream.m_pLockedData = gRenDev->m_DevBufMan.UpdateIB(m_IBStream.m_nDevBuf, m_IBStream.m_pUpdateData, m_nInds*sizeof(uint16), 0);
          if (!m_IBStream.m_pLockedData)
            bRes = false;
        }
        nOffs += m_nInds;
      }
      SAFE_DELETE_ARRAY(pTemp);
      return bRes;
    }
  }
  return false;

  MEM_CHECK
}

bool CRenderMesh2::CreateVidVertices(int nVerts, EVertexFormat eVF, int nStream)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMesh, 0, "%s", this->GetSourceName());
	MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", this->GetTypeName());

  assert(gRenDev->m_pRT->IsRenderThread());

  AUTO_LOCK(m_sResLock);

  MEM_CHECK
  assert(gRenDev->m_pRT->IsRenderThread());
  if (gRenDev->CheckDeviceLost())
    return false;
  if (!nVerts && eVF==eVF_Unknown)
  {
    nVerts = m_nVerts;
    eVF = m_eVF;
  }
  if (m_nFlags & FRM_INSTANCED)
    nVerts *= RM_INSTANCES;

  int nSize = GetStreamSize(nStream, nVerts);
  m_VBStream[nStream].m_nDevBuf = gRenDev->m_DevBufMan.CreateVB(nSize);

  return (m_VBStream[nStream].m_nDevBuf >= 0);

  MEM_CHECK
}

bool CRenderMesh2::UpdateVidVertices(int nStream)
{
  assert(gRenDev->m_pRT->IsRenderThread());

  AUTO_LOCK(m_sResLock);

  MEM_CHECK
  assert(nStream < VSF_NUM);

  if (m_VBStream[nStream].m_nDevBuf<0)
  {
    if (!CreateVidVertices(m_nVerts, m_eVF, nStream))
      return false;
  }

  MEM_CHECK
  if (m_VBStream[nStream].m_nDevBuf>=0)
  {
    UnlockStream(nStream);
    if (m_VBStream[nStream].m_pUpdateData)
    {
      int nSStride = GetStreamStride(nStream);
      int nSSize = GetStreamSize(nStream);

      bool bRes = true;
      int n = (m_nFlags & FRM_INSTANCED) ? RM_INSTANCES : 1;
      int nOffs = 0;
      byte *pTemp = NULL;
      for (int i=0; i<n; i++)
      {
#ifdef FP16_MESH
        if (n > 1)
        {
          if (!pTemp)
            pTemp = new byte[nSSize];
          byte *pSrc = (byte *)m_VBStream[nStream].m_pUpdateData;
          memcpy(pTemp, pSrc, nSSize);
          if (nStream == VSF_GENERAL)
          {
            byte *pDst = pTemp;
            CryHalf hInd = CryConvertFloatToHalf((float)i + 0.1f);
            for (int j=0; j<m_nVerts; j++)
            {
              Vec3f16 *pVDst = (Vec3f16 *)pDst;
              pVDst->w = hInd;
              pDst += nSStride;
            }
          }
          void *pLocked = gRenDev->m_DevBufMan.UpdateVB(m_VBStream[nStream].m_nDevBuf, pTemp, nSSize, nOffs*nSStride);
          if (!pLocked)
            bRes = false;
          else
          if (!nOffs)
            m_VBStream[nStream].m_pLockedData = pLocked;
        }
        else
#endif
        {
          m_VBStream[nStream].m_pLockedData = gRenDev->m_DevBufMan.UpdateVB(m_VBStream[nStream].m_nDevBuf, m_VBStream[nStream].m_pUpdateData, nSSize, nOffs*nSStride);
          if (!m_VBStream[nStream].m_pLockedData)
            bRes = false;
        }
        nOffs += m_nVerts;
      }
      SAFE_DELETE_ARRAY(pTemp);
      return bRes;
    }
  }

  return false;

  MEM_CHECK
}

void CRenderMesh2::Render(const struct SRendParams& rParams, CRenderObject* pObj, IMaterial *pMaterial, bool bSkinned)
{
  FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

  int nList = rParams.nRenderList;
  int nAW = rParams.nAfterWater;
  CRenderer *rd = gRenDev;

  if (!pMaterial)
    pMaterial = m_pMaterial;

  if (rd->m_pDefaultMaterial && pMaterial)
    pMaterial = rd->m_pDefaultMaterial;

  assert(pMaterial);

  if(!pMaterial)
    return;

  pObj->m_pRenderNode = rParams.pRenderNode;
  pObj->m_pCurrMaterial = pMaterial;
	SRenderObjData *pOD = NULL;
  if (rParams.pWeights)
  {
    pOD = rd->EF_GetObjData(pObj, true);
    pOD->m_pWeights = rParams.pWeights;
  } 

	if( (uint64) *rParams.pLayerEffectParams )
	{
		if( !pOD )
			pOD = rd->EF_GetObjData(pObj, true);
		pOD->m_pLayerEffectParams[0] = rParams.pLayerEffectParams[0];
		pOD->m_pLayerEffectParams[1] = rParams.pLayerEffectParams[1];
	}

  assert(!(pObj->m_ObjFlags & FOB_BENDED) || pObj->m_pBending);
  rd->CheckRenderObjVisArea(pObj);

  PodArray<CRenderChunk> *pChunks = bSkinned ? m_pChunksSkinned : &m_Chunks;

  //if (!(rParams.dwFObjFlags & FOB_RENDER_INTO_SHADOWMAP))
  {
    const uint32 ni = (uint32)pChunks->Count();
    for (uint32 i=0; i<ni; i++)     
    {
      CRenderChunk * pChunk = pChunks->Get(i);
      CRendElementBase * pREMesh = pChunk->pRE;

      SShaderItem *pShaderItem = &pMaterial->GetShaderItem(pChunk->m_nMatID);

      SRenderShaderResources *pR = (SRenderShaderResources *)pShaderItem->m_pShaderResources;
      CShader *pS = (CShader *)pShaderItem->m_pShader;
      if (pREMesh && pS && pR)
      {
        if (pS->m_Flags2 & EF2_NODRAW)
          continue;

        if ((rd->m_RP.m_TI[rd->m_RP.m_nFillThreadID].m_PersFlags & RBPF_SHADOWGEN) && (pR->m_ResFlags & MTL_FLAG_NOSHADOW))
          continue;

        rd->EF_AddEf_NotVirtual(pREMesh, *pShaderItem, pObj, nList, nAW);  
      }
    } 
  }

}

static CRenderObject *sDuplicateRO(CRenderObject *pObj, const SRenderObjectModifier * pROII)
{
  CRenderer *rd = gRenDev;
  CRenderObject * pObjNew = rd->EF_GetObject(true);
  pObj->CopyTo(pObjNew);

  SRenderObjData *pOD = NULL;
  if (pROII->nMatricesInUse)
  {
    pObjNew->m_II.m_Matrix = pROII->mat;
    if (pObjNew->m_ObjFlags & FOB_HAS_PREVMATRIX)
    {
      pOD = rd->EF_GetObjData(pObjNew, true);
      pOD->m_prevMatrix = pROII->prev_mat;
    }

    if( !pROII->mat.m01 && !pROII->mat.m02 && !pROII->mat.m10 && !pROII->mat.m12 && !pROII->mat.m20 && !pROII->mat.m21 )
      pObjNew->m_ObjFlags &= ~FOB_TRANS_ROTATE;
    else
      pObjNew->m_ObjFlags |= FOB_TRANS_ROTATE;
  }

  if (pROII->nStatesInUse)
  {
    if (pROII->pWeights)
    {
      if (!pOD)
        pOD = rd->EF_GetObjData(pObjNew, true);
      pOD->m_pWeights = pROII->pWeights;
    }
  }

  return pObjNew;
}

void CRenderMesh2::Render(CRenderObject *pObj, bool bSkinned, const SRenderObjectModifier * pROII)
{
  FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

  // duplicate render object
  // TODO: handle RenderObjectModifier by renderer natively 
  CRenderer *rd = gRenDev;
  assert(!(pObj->m_ObjFlags & FOB_BENDED) || pObj->m_pBending);
  //assert (!pObj->GetInstanceInfo(0));
  if (pROII)
    pObj = sDuplicateRO(pObj, pROII);

	int nList = EFSLIST_GENERAL;

	if(pObj->m_ObjFlags & FOB_RENDER_AFTER_POSTPROCESSING)
	{
		nList = EFSLIST_AFTER_POSTPROCESS;
	}
	
  int nAW = pROII ? pROII->nAfterWater : 1;

  IMaterial *pMaterial = pObj->m_pCurrMaterial;

  if (!pMaterial)
    pMaterial = m_pMaterial;

  if (rd->m_pDefaultMaterial && pMaterial)
    pMaterial = rd->m_pDefaultMaterial;

  assert(pMaterial);

  if (!pMaterial)
    return;

  //////////////
  gRenDev->CheckRenderObjVisArea( pObj );

  bool bAllowDepthChunksCreation = m_Chunks.Count()>1 && CRenderer::CV_r_MergeRenderChunksForDepth && (pObj->m_ObjFlags & FOB_VEGETATION);

  // update depth chunks
  //if (bAllowDepthChunksCreation && !m_pMergedDepthOnlyChunks)
  //  CreateDepthChunks();
  //if (m_pMergedDepthOnlyChunks && !CRenderer::CV_r_MergeRenderChunksForDepth)
  //  ReleaseChunks(m_pMergedDepthOnlyChunks);

  // check if makes sense to use
  bool bUseDepthChunks = false; //bAllowDepthChunksCreation && !bSkinned && m_pMergedDepthOnlyChunks && m_pMergedDepthOnlyChunks->Count();// && (pMaterial == m_pMaterial);

  // if valid depth chunks are present - use it for depth passes
  /*if (bUseDepthChunks)
  {
    for (uint32 i=0; i<(uint32)m_pMergedDepthOnlyChunks->Count(); i++)     
    {
      CRenderChunk * pChunk = m_pMergedDepthOnlyChunks->Get(i);
      CRendElementBase * pREMesh = pChunk->pRE;

      SShaderItem *pShaderItem = &pMaterial->GetShaderItem(pChunk->m_nMatID);

      SRenderShaderResources *pR = (SRenderShaderResources *)pShaderItem->m_pShaderResources;
      CShader *pS = (CShader *)pShaderItem->m_pShader;
      if (pREMesh && pS && pR)
      {
        if (pS->m_Flags2 & EF2_NODRAW)
          continue;

        if ((rd->m_RP.m_TI[rd->m_RP.m_nFillThreadID].m_PersFlags & RBPF_SHADOWGEN) && (pR->m_ResFlags & MTL_FLAG_NOSHADOW))
          continue;

        if (pROII && pROII->nTechniqueID > 0)
        {
          SShaderItem shaderItem;
          shaderItem = *pShaderItem;
          shaderItem.m_nTechnique = pS->GetTechniqueID(shaderItem.m_nTechnique, pROII->nTechniqueID);
          pShaderItem = &shaderItem;
          rd->EF_AddEf_NotVirtual(pREMesh, *pShaderItem, pObj, nList, nAW);  
        }
        else
          rd->EF_AddEf_NotVirtual(pREMesh, *pShaderItem, pObj, nList, nAW);  
      }
    } 

    return;
  }*/

  /////////////

  PodArray<CRenderChunk>* pChunks = bSkinned ? m_pChunksSkinned : &m_Chunks;

  const uint32 ni = (uint32)pChunks->Count();
  for (uint32 i=0; i<ni; i++)     
  {
    CRenderChunk * pChunk = pChunks->Get(i);
    CRendElementBase * pREMesh = pChunk->pRE;

    SShaderItem *pShaderItem = &pMaterial->GetShaderItem(pChunk->m_nMatID);

    SRenderShaderResources *pR = (SRenderShaderResources *)pShaderItem->m_pShaderResources;
    CShader *pS = (CShader *)pShaderItem->m_pShader;
    if (pREMesh && pS && pR)
    {
      if (pS->m_Flags2 & EF2_NODRAW)
        continue;

      if(rd->CV_r_VegetationAlphaTestOnly==1 && pS->GetShaderType() == eST_Vegetation)
        if(!pR->GetAlphaRef())
          continue;

      if ((rd->m_RP.m_TI[rd->m_RP.m_nFillThreadID].m_PersFlags & RBPF_SHADOWGEN) && (pR->m_ResFlags & MTL_FLAG_NOSHADOW))
        continue;

/*      if (pROII && pROII->nTechniqueID > 0)
      {
        SShaderItem shaderItem;
        shaderItem = *pShaderItem;
        shaderItem.m_nTechnique = pS->GetTechniqueID(shaderItem.m_nTechnique, pROII->nTechniqueID);
        pShaderItem = &shaderItem;
        rd->EF_AddEf_NotVirtual(pREMesh, *pShaderItem, pObj, nList, nAW);  
      }
      else*/
        rd->EF_AddEf_NotVirtual(pREMesh, *pShaderItem, pObj, nList, nAW);  
    }
  } 
}

void CRenderMesh2::SetREUserData(float *pfCustomData, float fFogScale, float fAlpha)
{
  for (int i=0; i<m_Chunks.Count(); i++)
  {
    if(m_Chunks[i].pRE)
    {
      m_Chunks[i].pRE->m_CustomData = pfCustomData;
    }
  }
}

void CRenderMesh2::AddRenderElements(CRenderObject *pObj, int nList, int nAW, IMaterial *pIMatInfo)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_RenderMesh, 0, "%s", this->GetSourceName());
	MEMSTAT_CONTEXT_NAMED_FMT(Type, EMemStatContextTypes::MSC_RenderMeshType, 0, "%s", this->GetTypeName());

  if (!pIMatInfo)
    pIMatInfo = m_pMaterial;

  assert(!(pObj->m_ObjFlags & FOB_BENDED) || pObj->m_pBending);
  //assert (!pObj->GetInstanceInfo(0));

  if(gRenDev->m_pDefaultMaterial && gRenDev->m_pTerrainDefaultMaterial)
  {
    if(nList == EFSLIST_TERRAINLAYER && pObj->GetMatrix().GetTranslation().GetLength()>1)
    {
      if (gRenDev->m_pTerrainDefaultMaterial && pIMatInfo)
        pIMatInfo = gRenDev->m_pTerrainDefaultMaterial;
    }
    else
    {
      if (gRenDev->m_pDefaultMaterial && pIMatInfo)
        pIMatInfo = gRenDev->m_pDefaultMaterial;
    }
  }

  //assert(pIMatInfo);
  if (!_GetVertexContainer()->m_nVerts || !m_Chunks.Count() || !pIMatInfo)
    return;

  for (int i=0; i<m_Chunks.Count(); i++)
  {
    CRenderChunk * pChunk = &m_Chunks[i];
    CREMesh * pOrigRE = pChunk->pRE;

    // get material

    SShaderItem shaderItem = pIMatInfo->GetShaderItem(pChunk->m_nMatID);

//    if (nTechniqueID > 0)
  //    shaderItem.m_nTechnique = shaderItem.m_pShader->GetTechniqueID(shaderItem.m_nTechnique, nTechniqueID);

    if (shaderItem.m_pShader && pOrigRE)// && pMat->nNumIndices)
    {
      TArray<CRendElementBase *> *pREs = shaderItem.m_pShader->GetREs(shaderItem.m_nTechnique);

      assert(pOrigRE->m_pChunk->nFirstIndexId<60000);

      if (!pREs || !pREs->Num())
        gRenDev->EF_AddEf_NotVirtual(pOrigRE, shaderItem, pObj, nList, nAW);
      else
        gRenDev->EF_AddEf_NotVirtual(pREs->Get(0), shaderItem, pObj, nList, nAW);

      if (m_nClientTextureBindID && (_GetPrimitiveType() == R_PRIMV_MULTI_STRIPS))
        break;
    }
  } //i
}

void CRenderMesh2::AddRE(CRenderObject *obj, IShader *ef, int nList, int nAW)
{
  if (!m_nVerts || !m_Chunks.Count())
    return;

  assert(!(obj->m_ObjFlags & FOB_BENDED) || obj->m_pBending);

  for(int i=0; i<m_Chunks.Count(); i++)
  {
    if (!m_Chunks[i].pRE)
      continue;

    SShaderItem SH = m_pMaterial->GetShaderItem();
    if (ef)
      SH.m_pShader = ef;
    if (SH.m_pShader)
    {
      assert(m_Chunks[i].pRE->m_pChunk->nFirstIndexId<60000);

      TArray<CRendElementBase *> *pRE = SH.m_pShader->GetREs(SH.m_nTechnique);
      if (!pRE || !pRE->Num())
        gRenDev->EF_AddEf_NotVirtual(m_Chunks[i].pRE, SH, obj, nList, nAW);
      else
        gRenDev->EF_AddEf_NotVirtual(SH.m_pShader->GetREs(SH.m_nTechnique)->Get(0), SH, obj, nList, nAW);
    }
  }
}

size_t CRenderMesh2::GetMemoryUsage(ICrySizer *pSizer,EMemoryUsageArgument nType)
{
	size_t nSize = 0;
	switch (nType)
	{
	case MEM_USAGE_COMBINED:
		nSize = Size(0) + Size(3);
		break;
	case MEM_USAGE_ONLY_SYSTEM:
		nSize = Size(0);
		break;
	case MEM_USAGE_ONLY_VIDEO:
		nSize = Size(0x3);
		return nSize;
		break;
	case MEM_USAGE_ONLY_STREAMS:
		nSize = Size(0) + Size(3);

		if (pSizer)
		{
			SIZER_COMPONENT_NAME(pSizer, "STREAM MESH");
			pSizer->AddObject((void *)this, nSize);
		}

		// Not add overhead allocations.
		return nSize;
		break;
	}

	{
		nSize += sizeof(*this);
		for (int i=0; i<(int)m_Chunks.capacity(); i++)
		{
			if (i < m_Chunks.Count())
				nSize += m_Chunks[i].Size();
			else
				nSize += sizeof(CRenderChunk);
		}
		if (m_pChunksSkinned)
		{
			for (int i=0; i<(int)m_pChunksSkinned->capacity(); i++)
			{
				if (i < m_pChunksSkinned->Count())
					nSize += m_pChunksSkinned->Get(i)->Size();
				else
					nSize += sizeof(CRenderChunk);
			}
		}
	}

	if (pSizer)
	{
		pSizer->AddObject((void *)this, nSize);

#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT
		if(m_pTrisMap)
		{
			SIZER_COMPONENT_NAME(pSizer, "Hash map");
			nSize += stl::size_of_map(*m_pTrisMap);
		}
#endif
	}

	return nSize;
}

void CRenderMesh2::GetMemoryUsage(ICrySizer *pSizer ) const
{
	
	pSizer->AddObject( this, sizeof(*this) );
	Size(0, pSizer); 
	pSizer->AddObject( m_Chunks );	
	pSizer->AddObject( m_pChunksSkinned );
	  
#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT
	{
		SIZER_COMPONENT_NAME(pSizer, "Hash map");
		pSizer->AddObject( m_pTrisMap );			
	}
#endif
}

int CRenderMesh2::GetAllocatedBytes(bool bVideoMem)
{
  if (!bVideoMem)
    return Size(0);
  else
    return Size(0x3);
}

int CRenderMesh2::GetTextureMemoryUsage(IMaterial *pMaterial, ICrySizer *pSizer)
{
  // If no input material use internal render mesh material.
  if (!pMaterial)
    pMaterial = m_pMaterial;
  if (!pMaterial)
    return 0;

  int textureSize = 0;
  std::set<CTexture*> used;
  for (int a=0; a<m_Chunks.Count(); a++)
  {
    CRenderChunk * pChunk = &m_Chunks[a];

    // Override default material
    SShaderItem shaderItem = pMaterial->GetShaderItem(pChunk->m_nMatID);
    if (!shaderItem.m_pShaderResources)
      continue;

    SRenderShaderResources *pRes = (SRenderShaderResources *)shaderItem.m_pShaderResources;

    for (int i=0; i<EFTT_MAX; i++)
    {
      if (!pRes->m_Textures[i])
        continue;

      CTexture *pTexture = pRes->m_Textures[i]->m_Sampler.m_pTex;
      if (!pTexture)
        continue;

      if (used.find(pTexture) != used.end()) // Already used in size calculation.
        continue;
      used.insert(pTexture);

      int nTexSize = pTexture->GetDataSize();
      textureSize += nTexSize;

      if (pSizer)
        pSizer->AddObject(pTexture, nTexSize);
    }
  }

  return textureSize;
}

float CRenderMesh2::GetAverageTrisNumPerChunk(IMaterial * pMat)
{
  float fTrisNum = 0;
  float fChunksNum = 0;

  for(int m=0; m<GetChunks().Count(); m++)
  {
    CRenderChunk& chunk = GetChunks()[m];
    if ((chunk.m_nMatFlags & MTL_FLAG_NODRAW) || !chunk.pRE)
      continue;

    IMaterial *pCustMat;        
    if (pMat && chunk.m_nMatID>=0 && chunk.m_nMatID < pMat->GetSubMtlCount())
      pCustMat = pMat->GetSubMtl(chunk.m_nMatID);
    else
      pCustMat = pMat;

    if(!pCustMat)
      continue;

    IShader * pShader = pCustMat->GetShaderItem().m_pShader;

    if(!pShader)
      continue;

    if (pShader->GetFlags2() & EF2_NODRAW)
      continue;

    fTrisNum += chunk.nNumIndices/3;
    fChunksNum++;
  }

  return fChunksNum ? (fTrisNum/fChunksNum) : 0;
}

void CRenderMesh2::InitTriHash()
{
#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT

  SAFE_DELETE(m_pTrisMap);
  m_pTrisMap = new TrisMap;

  int nPosStride=0;
  int nIndCount = m_nInds;
  const byte * pPositions = GetPosPtr(nPosStride, FSL_READ);
  const uint16 * pIndices = GetIndexPtr(FSL_READ);

  iLog->Log("CRenderMesh::InitTriHash: Tris=%d, Verts=%d, Name=%s ...", nIndCount/3, GetVerticesCount(), GetSourceName() ? GetSourceName() : "Null");

  if(pIndices && pPositions && m_Chunks.Count() && nIndCount && GetVerticesCount())
  {
    for (uint32 ii=0; ii<(uint32)m_Chunks.Count(); ii++)     
    {
      CRenderChunk *pChunk = &m_Chunks[ii];

      if (pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
        continue;

      // skip transparent and alpha test
      const SShaderItem &shaderItem = m_pMaterial->GetShaderItem(pChunk->m_nMatID);
      if (!shaderItem.IsZWrite() || !shaderItem.m_pShaderResources || shaderItem.m_pShaderResources->GetAlphaRef())
        continue;

      if(shaderItem.m_pShader && shaderItem.m_pShader->GetFlags()&EF_DECAL)
        continue;

      uint32 nFirstIndex = pChunk->nFirstIndexId;
      uint32 nLastIndex = pChunk->nFirstIndexId + pChunk->nNumIndices;

      for (uint32 i = nFirstIndex; i < nLastIndex; i+=3)
      {
        int32 I0	=	pIndices[i+0];
        int32 I1	=	pIndices[i+1];
        int32 I2	=	pIndices[i+2];

        Vec3 v0 = *(Vec3*)&pPositions[nPosStride*I0];
        Vec3 v1 = *(Vec3*)&pPositions[nPosStride*I1];
        Vec3 v2 = *(Vec3*)&pPositions[nPosStride*I2];

        AABB triBox;
        triBox.min = triBox.max = v0;
        triBox.Add(v1);
        triBox.Add(v2);

				float fRayLen = CRenderer::CV_r_RenderMeshHashGridUnitSize/2;
        triBox.min -= Vec3(fRayLen,fRayLen,fRayLen);
        triBox.max += Vec3(fRayLen,fRayLen,fRayLen);

        AABB aabbCell;

        aabbCell.min = triBox.min / CRenderer::CV_r_RenderMeshHashGridUnitSize;
        aabbCell.min.x = floor(aabbCell.min.x);
        aabbCell.min.y = floor(aabbCell.min.y);
        aabbCell.min.z = floor(aabbCell.min.z);

        aabbCell.max = triBox.max / CRenderer::CV_r_RenderMeshHashGridUnitSize;
        aabbCell.max.x = ceil(aabbCell.max.x);
        aabbCell.max.y = ceil(aabbCell.max.y);
        aabbCell.max.z = ceil(aabbCell.max.z);

        for(float x=aabbCell.min.x; x<aabbCell.max.x; x++)
        {
          for(float y=aabbCell.min.y; y<aabbCell.max.y; y++)
          {
            for(float z=aabbCell.min.z; z<aabbCell.max.z; z++)
            {
              AABB cellBox;
              cellBox.min = Vec3(x,y,z)*CRenderer::CV_r_RenderMeshHashGridUnitSize;
              cellBox.max = cellBox.min + Vec3(CRenderer::CV_r_RenderMeshHashGridUnitSize,CRenderer::CV_r_RenderMeshHashGridUnitSize,CRenderer::CV_r_RenderMeshHashGridUnitSize);
              cellBox.min -= Vec3(fRayLen,fRayLen,fRayLen);
              cellBox.max += Vec3(fRayLen,fRayLen,fRayLen);
              if(!Overlap::AABB_Triangle(cellBox, v0, v1, v2))
                continue;

              int key = (int)(x*256.f*256.f + y*256.f + z);
              PodArray<std::pair<int,int> > * pTris = &(*m_pTrisMap)[key];
              std::pair<int,int> t(i,pChunk->m_nMatID);
              if(pTris->Find(t)<0)
                pTris->Add(t);
            }
          }
        }
      }
    }
  }

  iLog->LogPlus(" ok (%d)", m_pTrisMap->size());

#endif
}


const PodArray<std::pair<int,int> > * CRenderMesh2::GetTrisForPosition(const Vec3 & vPos)
{
#ifdef RENDER_MESH_TRIANGLE_HASH_MAP_SUPPORT

  if(!m_pTrisMap)
	{
		AUTO_LOCK(m_getTrisForPositionLock);
		if(!m_pTrisMap)
	    InitTriHash();
	}

  Vec3 vCellMin = vPos / CRenderer::CV_r_RenderMeshHashGridUnitSize;
  vCellMin.x = floor(vCellMin.x);
  vCellMin.y = floor(vCellMin.y);
  vCellMin.z = floor(vCellMin.z);

  int key = (int)(vCellMin.x*256.f*256.f + vCellMin.y*256.f + vCellMin.z);

  const TrisMap::iterator & iter = (*m_pTrisMap).find(key);
  if(iter != (*m_pTrisMap).end())
    return &iter->second;

#endif

  return 0;
}

void CRenderMesh2::UpdateBBoxFromMesh()
{
  PROFILE_FRAME(UpdateBBoxFromMesh);

  AABB aabb; 
  aabb.Reset();

  int nVertCount = _GetVertexContainer()->GetVerticesCount();
  int nPosStride=0;
  int nIndCount = GetIndicesCount();
  const byte * pPositions = GetPosPtr(nPosStride, FSL_READ);
  const uint16 * pIndices = GetIndexPtr(FSL_READ);

  if(!pIndices || !pPositions)
  {
    //assert(!"Mesh is not ready");
    return;
  }

  for (int32 a=0; a<m_Chunks.Count(); a++)     
  {
    CRenderChunk * pChunk = &m_Chunks[a];

    if(pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
      continue;

    uint32 nFirstIndex = pChunk->nFirstIndexId;
    uint32 nLastIndex = pChunk->nFirstIndexId + pChunk->nNumIndices;

    for (uint32 i = nFirstIndex; i < nLastIndex; i++)
    {
      int32 I0	=	pIndices[i];
      if(I0 < nVertCount)
      {
        Vec3 v0 = *(Vec3*)&pPositions[nPosStride*I0];
        aabb.Add(v0);
      }
      else
        assert(!"Index is out of range");
    }
  }

  if (!aabb.IsReset())
  {
    m_vBoxMax = aabb.max;
    m_vBoxMin = aabb.min;
  }
}
#endif//__SPU__
void CRenderMesh2::GetRandomPos(RandomPos& ran, GeomQuery& geo, EGeomForm eForm)
{
  int nPStride;
  byte* pPos = GetPosPtr(nPStride, FSL_READ);

  if (eForm == GeomForm_Vertices)
  {
    int nVert = Random(m_nVerts);
    ran.vPos = *(Vec3*)(pPos+nVert*nPStride);
    ran.vNorm = Vec3(0,0,1);
    return;
  }

  geo.GetExtent(this, eForm);
  int nTri = geo.GetRandomPart(this, eForm);

  int nIndex = nTri*3;

  // Generate interpolators for verts. Volume gen not supported (use surface).
  float t[3];
  TriRandomPoint(t, eForm);

  uint16 *pInds = GetIndexPtr(FSL_READ);

  ran.vPos.Set(0,0,0);
  for(int v=0; v<3; v++)
  {
    Vec3 vPos = *((Vec3*)(pPos+pInds[nIndex+v]*nPStride));
    ran.vPos += vPos * t[v];
  }

  ran.vNorm = Vec3(0,0,1);
  ran.vNorm.Normalize();
}
#ifndef __SPU__
float CRenderMesh2::ComputeExtent(GeomQuery& geo, EGeomForm eForm)
{
  if (eForm == GeomForm_Vertices)
    return (float)m_nVerts;
  else
  {
    uint16 *pInds = GetIndexPtr(FSL_READ);
    int nTris = GetIndicesCount()/3;
    geo.SetNumParts(nTris);
    int nPStride;
    byte* pPos = GetPosPtr(nPStride, FSL_READ);
    for (int i=0; i<nTris; i++)
    {
      Vec3 v0, v1, v2;
      v0 = *(Vec3*)(pPos+pInds[i*3+0]*nPStride);
      v1 = *(Vec3*)(pPos+pInds[i*3+1]*nPStride);
      v2 = *(Vec3*)(pPos+pInds[i*3+2]*nPStride);
      geo.SetPartExtent(i, TriExtent(eForm, v0, v1, v2));
    }
    return geo.GetExtent();
  }
}

int CRenderChunk::Size()
{
  int nSize = sizeof(*this);
  return nSize;
}

void CRenderMesh2::Size(uint32 nFlags, ICrySizer *pSizer ) const
{
	uint32 i;
	if (!nFlags)  // System size
	{
		for (i=0; i<VSF_NUM; i++)
		{
			if (m_VBStream[i].m_pUpdateData)
				pSizer->AddObject(m_VBStream[i].m_pUpdateData,GetStreamSize(i) );
			if (m_VBStream[i].m_pReadData)
				pSizer->AddObject(m_VBStream[i].m_pReadData,GetStreamSize(i) );
		}
		if (m_IBStream.m_pUpdateData)
			pSizer->AddObject( m_IBStream.m_pUpdateData, m_nInds * sizeof(uint16) );
		if (m_IBStream.m_pReadData)
			pSizer->AddObject( m_IBStream.m_pReadData, m_nInds * sizeof(uint16) );
#ifdef FP16_MESH
		if (m_pCachePos)
			pSizer->AddObject(m_pCachePos, m_nVerts * sizeof(Vec3) );
#endif
#ifdef KEEP_POSITIONS_INDICES
		if (m_pSysPosData)
			pSizer->AddObject(m_pSysPosData, m_nVerts * sizeof(Vec3f16) );
#endif
	}

}

int CRenderMesh2::Size(uint32 nFlags)
{
  int nSize = 0;
  uint32 i;
  if (!nFlags)  // System size
  {
    for (i=0; i<VSF_NUM; i++)
    {
      if (m_VBStream[i].m_pUpdateData)
        nSize += GetStreamSize(i);
      if (m_VBStream[i].m_pReadData)
        nSize += GetStreamSize(i);
    }
    if (m_IBStream.m_pUpdateData)
      nSize += m_nInds * sizeof(uint16);
    if (m_IBStream.m_pReadData)
      nSize += m_nInds * sizeof(uint16);
#ifdef FP16_MESH
    if (m_pCachePos)
      nSize += m_nVerts * sizeof(Vec3);
#endif
#ifdef KEEP_POSITIONS_INDICES
    if (m_pSysPosData)
      nSize += m_nVerts * sizeof(Vec3f16);
#endif
  }
  if (nFlags & 1) // VB size
  {
    for (i=0; i<VSF_NUM; i++)
    {
      if (_HasVBStream(i))
        nSize += GetStreamSize(i);
    }
  }
  if (nFlags & 2) // IB size
  {
    if (_HasIBStream())
      nSize += m_nInds * sizeof(uint16);
  }

  return nSize;
}

void CRenderMesh2::FreeDeviceBuffers(bool bRestoreSys)
{
  uint32 i;

  for (i=0; i<VSF_NUM; i++)
  {
    if (_HasVBStream(i))
    {
      if (bRestoreSys)
      {
        void *pSrc = LockVB(i, FSL_READ | FSL_VIDEO);
        void *pDst = LockVB(i, FSL_SYSTEM_CREATE);
        cryMemcpy(pDst, pSrc, GetStreamSize(i));
      }
      ReleaseVB(i);
    }
  }

  if (_HasIBStream())
  {
    if (bRestoreSys)
    {
      void *pSrc = LockIB(FSL_READ | FSL_VIDEO);
      void *pDst = LockIB(FSL_SYSTEM_CREATE);
      cryMemcpy(pDst, pSrc, m_nInds * sizeof(uint16));
    }
    ReleaseIB();
  }
}

void CRenderMesh2::FreeSystemBuffers()
{
  uint32 i;

  for (i=0; i<VSF_NUM; i++)
  {
    SAFE_DELETE_VOID_ARRAY (m_VBStream[i].m_pUpdateData);
    SAFE_DELETE_VOID_ARRAY (m_VBStream[i].m_pReadData);
  }
  SAFE_DELETE_VOID_ARRAY (m_IBStream.m_pUpdateData);
  SAFE_DELETE_VOID_ARRAY (m_IBStream.m_pReadData);
#ifdef FP16_MESH
  SAFE_DELETE_ARRAY(m_pCachePos);
#endif
#ifdef KEEP_POSITIONS_INDICES
  SAFE_DELETE_VOID_ARRAY(m_pSysPosData);
#endif
}


//////////////////////////////////////////////////////////////////////////
void CRenderMesh2::DebugDraw( const struct SGeometryDebugDrawInfo &info,uint32 nVisibleChunksMask )
{
  IRenderAuxGeom *pRenderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();

  const Matrix34 &mat = info.tm;

  bool bNoCull = info.bNoCull;
  bool bNoLines = info.bNoLines;
  bool bExtrude = info.bExtrude;
  float fExtrdueScale = 0.01f;

  SAuxGeomRenderFlags prevRenderFlags = pRenderAuxGeom->GetRenderFlags();
  SAuxGeomRenderFlags renderFlags = prevRenderFlags;
  renderFlags.SetAlphaBlendMode( e_AlphaBlended );

  if (bNoCull)
  {
    renderFlags.SetCullMode(e_CullModeNone);
  }
  pRenderAuxGeom->SetRenderFlags(renderFlags);

  ColorB lineColor = info.lineColor;
  ColorB c = info.color;

  for (int32 a=0; a<m_Chunks.Count(); a++)     
  {
    CRenderChunk *pChunk = &m_Chunks[a];

    if(pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
      continue;

    if (!((1<<a) & nVisibleChunksMask))
      continue;

    const int nVertCount = GetVerticesCount();
    int nPosStride=0;
    int nIndCount = GetIndicesCount();
    const uint16 * pIndices = GetIndexPtr(FSL_READ);
    const byte * pPositions = GetPosPtr(nPosStride, FSL_READ);

    uint32 nFirstIndex = pChunk->nFirstIndexId;
    uint32 nLastIndex = pChunk->nFirstIndexId + pChunk->nNumIndices;
    int nIndexStep = (_GetPrimitiveType() == R_PRIMV_MULTI_STRIPS) ? 1 : 3;
    for (uint32 i = nFirstIndex; i < nLastIndex-2; i+=nIndexStep)
    {
      int32 I0	=	pIndices[i];
      int32 I1	=	pIndices[i+1];
      int32 I2	=	pIndices[i+2];
      if(nIndexStep==1 && (i&1))
      {
        I1	=	pIndices[i+2];
        I2	=	pIndices[i+1];
      }
      assert(I0<GetVerticesCount());
      assert(I1<GetVerticesCount());
      assert(I2<GetVerticesCount());

      Vec3 v0, v1, v2;
      v0 = *(Vec3*)&pPositions[nPosStride*I0];
      v1 = *(Vec3*)&pPositions[nPosStride*I1];
      v2 = *(Vec3*)&pPositions[nPosStride*I2];
      if (bExtrude)
      {
        // Extrude vertices along the face normal for some scale
        Vec3 normal = (v1-v0).Cross(v2-v0);
        normal.NormalizeSafe();
        v0 += normal*fExtrdueScale;
        v1 += normal*fExtrdueScale;
        v2 += normal*fExtrdueScale;
      }

      v0 = mat.TransformPoint( v0 );
      v1 = mat.TransformPoint( v1 );
      v2 = mat.TransformPoint( v2 );

      pRenderAuxGeom->DrawTriangle( v0,c,v1,c,v2,c );

      if (!bNoLines)
      {
        pRenderAuxGeom->DrawLine( v0,lineColor,v1,lineColor );
        pRenderAuxGeom->DrawLine( v1,lineColor,v2,lineColor );
        pRenderAuxGeom->DrawLine( v2,lineColor,v0,lineColor );
      }
    }
  }
  pRenderAuxGeom->SetRenderFlags(prevRenderFlags);
}

//===========================================================================================================

CRenderMesh2 *gRM;

// Mesh garbage collector
void CRenderMesh2::Tick()
{
  ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT)
  int nFrameID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;
  int nFrame = nFrameID - 3;
  CRenderMesh2& Root = CRenderMesh2::m_RootRelease[nFrame & 3];
  CRenderMesh2 *pRMNext = NULL;
  CRenderMesh2 *pRM;

  // Delayed meshes deleting
  for (pRM=Root.m_Next; pRM!=&Root; pRM=pRMNext)
  {
    pRMNext = pRM->m_Next;
    SAFE_DELETE(pRM);
    m_pLastTick = NULL;
  }

	//keep track if some garbage has been collected last run
	static const uint32 sChangedThreadHold = 4;
	static uint32 changedLastIter[2] = {sChangedThreadHold,sChangedThreadHold};//initial frames should garbage collect
	static uint32 changedLastRunIndex = 0;

  // Incrementally cleanup meshes (outdated temp. data)
  int nWeight = 0;
  pRM = m_pLastTick;
  if (!pRM)
    pRM = m_Root.m_Next;
  for (; pRM!=&m_Root; pRM=pRM->m_Next)
  {
    gRM = pRM;
    bool bKeepSystem = false;

    if (pRM->m_eType == eRMT_KeepSystem || pRM->m_eType == eRMT_KeepSystemPS3)
      bKeepSystem = true;

#ifdef FP16_MESH
    if (pRM->m_pCachePos && nFrameID-pRM->m_nFrameRequestCachePos > 10)
    {
      SAFE_DELETE(pRM->m_pCachePos);
			changedLastIter[changedLastRunIndex] += 1;
      nWeight += 10;
    }
#endif

    for (int i=0; i<VSF_NUM; i++)
    {
#if !defined(DIRECT3D10)
      // In DX11 we cannot lock device buffers, so we have to keep system copy
      if (!bKeepSystem && pRM->m_VBStream[i].m_pUpdateData && pRM->m_VBStream[i].m_nFrameUpdate >= pRM->m_VBStream[i].m_nFrameRequest && nFrameID-pRM->m_VBStream[i].m_nFrameUpdate > 1)
      {
        assert(pRM->_HasVBStream(i));
				changedLastIter[changedLastRunIndex] += 1;
        nWeight += 10;
        SAFE_DELETE_VOID_ARRAY(pRM->m_VBStream[i].m_pUpdateData);
      }
#elif defined (KEEP_POSITIONS_INDICES)
      if (!bKeepSystem && pRM->m_VBStream[i].m_pUpdateData && pRM->m_VBStream[i].m_nFrameUpdate >= pRM->m_VBStream[i].m_nFrameRequest && nFrameID-pRM->m_VBStream[i].m_nFrameUpdate > 1)
      {
        // Keep positions on PS3 always (but not for terrain)
        if (i == VSF_GENERAL && (pRM->m_eVF == eVF_P3S_C4B_T2S || pRM->m_eVF == eVF_P3F_C4B_T2F))
        {
          SAFE_DELETE_VOID_ARRAY(pRM->m_pSysPosData);
          Vec3f16 *pTemp = new Vec3f16[pRM->m_nVerts];
          SVF_P3S_C4B_T2S *pSrc = (SVF_P3S_C4B_T2S *)pRM->m_VBStream[i].m_pUpdateData;
          for (int j=0; j<pRM->m_nVerts; j++)
          {
            pTemp[j] = pSrc[j].xyz;
          }
          pRM->m_pSysPosData = pTemp;
          SAFE_DELETE_VOID_ARRAY(pRM->m_VBStream[i].m_pUpdateData);
          nWeight += 20;
        }
        else
        {
          assert(pRM->_HasVBStream(i));
          nWeight += 10;
          SAFE_DELETE_VOID_ARRAY(pRM->m_VBStream[i].m_pUpdateData);
        }
				changedLastIter[changedLastRunIndex] += 1;
      }
#endif
      if (pRM->m_VBStream[i].m_pReadData)
      {
        nWeight += 10;
				changedLastIter[changedLastRunIndex] += 1;
        SAFE_DELETE_VOID_ARRAY(pRM->m_VBStream[i].m_pReadData);
      }
    }
#if !defined(DIRECT3D10)
    if (!bKeepSystem && pRM->m_IBStream.m_pUpdateData && pRM->m_IBStream.m_nFrameUpdate >= pRM->m_IBStream.m_nFrameRequest && nFrameID-pRM->m_IBStream.m_nFrameUpdate > 1)
    {
      assert(pRM->_HasIBStream());
      nWeight += 10;
      SAFE_DELETE_VOID_ARRAY(pRM->m_IBStream.m_pUpdateData);
			changedLastIter[changedLastRunIndex] += 1;
    }
#endif
    if (pRM->m_IBStream.m_pReadData)
    {
      nWeight += 10;
      SAFE_DELETE_VOID_ARRAY(pRM->m_IBStream.m_pReadData);
			changedLastIter[changedLastRunIndex] += 1;
    }

    nWeight++;

		//avoids peak allocations if not much has been garbage collected last and current iteration
		//nWeight can be a low value since otherwise changedLastIter are > sChangedThreadHold



		if(nWeight > 150)

      break;











  }
  m_pLastTick = pRM;
  if (m_pLastTick == &m_Root)
	{
    m_pLastTick = NULL;
		changedLastRunIndex ^= 1;
		changedLastIter[changedLastRunIndex] = 0;
	}
}

void CRenderMesh2::ShutDown()
{
  if (CRenderer::CV_r_releaseallresourcesonexit)
  {
    CRenderMesh2 *pRM = CRenderMesh2::m_Root.m_Next;
    CRenderMesh2 *pNext;
    for (pRM=m_Root.m_Next; pRM != &CRenderMesh2::m_Root; pRM=pNext)
    {
      pNext = pRM->m_Next;
      if (CRenderer::CV_r_printmemoryleaks)
      {
        float fSize = pRM->Size(0)/1024.0f/1024.0f;
        iLog->Log("Warning: CRenderMesh::ShutDown: RenderMesh leak %s: %0.3fMb", pRM->m_sSource, fSize);
      }
      if (pRM != &m_Root)
      {
        SAFE_RELEASE_FORCE(pRM);
      }
    }
  }
  CRenderMesh2::m_Root.m_Next = &CRenderMesh2::m_Root;
  CRenderMesh2::m_Root.m_Prev = &CRenderMesh2::m_Root;
}

void CRenderMesh2::KeepSysMesh(bool keep)
{
	if (keep)
		m_eType = eRMT_KeepSystem;
}

void CRenderMesh2::UnKeepSysMesh()
{
  m_eType = eRMT_Static;
}

const IRenderMesh::SRenderMeshStat* CRenderMesh2::GetRenderStats() const
{
#if defined(ENABLE_GPU_TIMERS)
  return &m_meshStat[GpuTimerEvent::s_readIdx];
#else
  return NULL;
#endif
}

//////////////////////////////////////////////////////////////////////////
void CRenderMesh2::DeleteDelayedMeshes()
{
	for (int i = 0; i < MAX_RELEASED_MESH_FRAMES; i++)
	{
		CRenderMesh2& Root = CRenderMesh2::m_RootRelease[i];
		CRenderMesh2 *pRMNext = NULL;
		CRenderMesh2 *pRM;

		// Delayed meshes deleting
		for (pRM=Root.m_Next; pRM!=&Root; pRM=pRMNext)
		{
			pRMNext = pRM->m_Next;
			SAFE_DELETE(pRM);
			m_pLastTick = NULL;
		}
	}
}
#endif//__SPU__

#include UNIQUE_VIRTUAL_WRAPPER(IRenderMesh)
