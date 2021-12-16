#ifndef _DevBuffer_H_
#define _DevBuffer_H_

struct alloc_info_struct
{
  int ptr; int bytes_num; bool busy; const char *szSource;
  unsigned Size() { return sizeof(*this); }

	void GetMemoryUsage(ICrySizer *pSizer ) const {}
};

#define POOL_SIZE_VB 512*1024   // size of VB pool in bytes
#define POOL_SIZE_IB 256*1024   // size of VB pool in bytes

union UD3DBuffer
{
#ifndef NULL_RENDERER
  D3DVertexBuffer *m_pVB;
  D3DIndexBuffer *m_pIB;
#else
  void *m_pVB;
  void *m_pIB;
#endif
};

struct SDevPool
{
  uint32 m_nPoolSize;
  uint32 m_nAllocatedSize;
  UD3DBuffer m_D3DBuf;
  PodArray<alloc_info_struct> m_AllocInfo;

  SDevPool(uint32 nSize)
  {
    m_nPoolSize = nSize;
    m_nAllocatedSize = 0;
    m_D3DBuf.m_pVB = NULL;
  }

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_AllocInfo);
	}
};

struct SDevPoolItem
{
  int32  m_nPool;
  int32 m_nBufOffset;

  SDevPoolItem()
  {
    m_nPool = -1;
    m_nBufOffset = 0;
  }
};

#define MAX_STAGING_BUFFERS 512

struct SDevBuffer
{
  uint32 m_nSize;
  SDevPoolItem *m_pPoolItem;    // In case of pool only
  UD3DBuffer m_D3DBuf;  // In case of pool-less
#ifdef DIRECT3D10
  bool m_bTempBuf;
  int m_nStagedOffset;
  int m_nStagedSize;
  int m_nDevOffset;
  void *m_pStagedVB;
  uint32 m_nLockFlags;
#endif
  SDevBuffer()
  {
    m_pPoolItem = NULL;
#ifdef DIRECT3D10
    m_bTempBuf = false;
    m_nStagedOffset = 0;
    m_nStagedSize = 0;
    m_nDevOffset = 0;
    m_nLockFlags = 0;
    m_pStagedVB = NULL;
#endif
  }
  ~SDevBuffer();

	void GetMemoryUsage(ICrySizer *pSizer) const{}
};

class CDevBufferMan
{
  friend class CD3D9Renderer;

  std::vector<SDevBuffer> m_VBs;
  std::vector<SDevBuffer> m_IBs;

  std::vector<uint32> m_FreeVBSlots;
  std::vector<uint32> m_FreeIBSlots;

  std::vector<SDevPool> m_VBPools;
  std::vector<SDevPool> m_IBPools;

#ifdef DIRECT3D10
  int m_nMaxStagingBuffers;
  int m_nCurStagedVB;
  int m_nCurStagedIB;
  void *m_pVBTemp[MAX_STAGING_BUFFERS];
  void *m_pIBTemp[MAX_STAGING_BUFFERS];
#endif

private:
  int32 CreateDevVB(int32 nSizeBuf, SDevBuffer& DB);
  int32 CreateDevVBInPool(int32 nSizeBuf, SDevBuffer& DB);
  int32 CreateDevIB(int32 nSizeBuf, SDevBuffer& DB);
  int32 CreateDevIBInPool(int32 nSizeBuf, SDevBuffer& DB);
  bool  ReleaseDevVB(SDevBuffer& DB);
  bool  ReleaseDevVBInPool(SDevBuffer& DB);
  bool  ReleaseDevIB(SDevBuffer& DB);
  bool  ReleaseDevIBInPool(SDevBuffer& DB);
  int32 AllocateVBSlot(SDevBuffer& DB);
  int32 AllocateIBSlot(SDevBuffer& DB);
  bool  AllocateChunk(int32 nSizeBuf, SDevPool& Pool, SDevPoolItem *pPoolItem);
  SDevBuffer *GetDevVB(int32 nVB);
  SDevBuffer *GetDevIB(int32 nIB);
  void _ValidateIBPools();
  _inline void ValidateIBPools()
  {
#ifdef _DEBUG
    //_ValidateIBPools();
#endif
  }
  void _ValidateVBPools();
  _inline void ValidateVBPools()
  {
#ifdef _DEBUG
    //_ValidateVBPools();
#endif
  }

public:
  int32 CreateVB(int32 nSize);
  int32 CreateIB(int32 nSize);
  bool  ReleaseVB(int32 nVB);
  bool  ReleaseIB(int32 nIB);

  void *LockVB(int32 nVB, int32 nOffset=0, int32 nSize=0, uint32 nFlags=0);
  void  UnlockVB(int32 nVB);

  void *LockIB(int32 nIB, int32 nOffset=0, int32 nSize=0, uint32 nFlags=0);
  void  UnlockIB(int32 nIB);

  void *UpdateVB(int32 nVB, const void *pData, int32 nSize, int32 nOffs=0);
  void *UpdateIB(int32 nIB, const void *pData, int32 nSize, int32 nOffs=0);

  CVertexBuffer *CreateVBuffer(int32 nVerts, EVertexFormat eVF, const char *szName);
  CIndexBuffer *CreateIBuffer(int32 nInds, const char *szName);
  void* UpdateVBuffer(CVertexBuffer *pVB, void *pVerts, int32 nVerts);
  void* UpdateIBuffer(CIndexBuffer *pIB, void *pInds, int32 nInds);
  void  ReleaseVBuffer(CVertexBuffer* pVB);
  void  ReleaseIBuffer(CIndexBuffer* pIB);

  void Init();
  void Invalidate();

#ifndef NULL_RENDERER
  void *LockVB(D3DVertexBuffer *pVB, int32 nOffset, uint32 nFlags);
  void  UnlockVB(D3DVertexBuffer *pVB);
  void *LockIB(D3DIndexBuffer *pIB, int32 nOffset, uint32 nFlags);
  void  UnlockIB(D3DIndexBuffer *pIB);

  D3DVertexBuffer *GetD3DVB(int32 nVB, int32* nOffs);
  D3DIndexBuffer  *GetD3DIB(int32 nIB, int32* nOffs);
#endif

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_VBs);
		pSizer->AddObject(m_IBs);
		pSizer->AddObject(m_FreeVBSlots);
		pSizer->AddObject(m_FreeIBSlots);
		pSizer->AddObject(m_VBPools);		
		pSizer->AddObject(m_IBPools);		
	}
};


#endif  _D3DBuffer_H_
