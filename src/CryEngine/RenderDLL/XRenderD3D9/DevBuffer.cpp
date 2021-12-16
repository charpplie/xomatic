/*=============================================================================
  DevBuffer.cpp : Direct3D9 vertex/index buffers management.
  Copyright (c) 2001-2009 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"

//===============================================================================

SDevBuffer::~SDevBuffer()
{

}

void CDevBufferMan::Init()
{
#ifdef DIRECT3D10
  m_nMaxStagingBuffers = CD3D9Renderer::CV_d3d10_NumStagingBuffers;

  for (int j=0; j<m_nMaxStagingBuffers; j++)
  {
    m_pVBTemp[j] = new DynamicVB <byte>(gcpRendD3D->m_pd3dDevice, 0, gcpRendD3D->CV_d3d9_vbpoolsize, true);
    m_pIBTemp[j] = new DynamicIB <uint16>(gcpRendD3D->m_pd3dDevice, 128*1024, true);
  }
#endif
}

void CDevBufferMan::Invalidate()
{
}

int32 CDevBufferMan::CreateVB(int32 nSize)
{
  SDevBuffer DB;
  int32 nVB = -1;

  if (CD3D9Renderer::CV_d3d9_vbpools && nSize<POOL_SIZE_VB)
    nVB = CreateDevVBInPool(nSize, DB);
  else
    nVB = CreateDevVB(nSize, DB);

  ValidateVBPools();

  return nVB;
}

int32 CDevBufferMan::CreateIB(int32 nSize)
{
  SDevBuffer DB;
  int32 nIB = -1;

  if (CD3D9Renderer::CV_d3d9_ibpools && nSize<POOL_SIZE_IB)
    nIB = CreateDevIBInPool(nSize, DB);
  else
    nIB = CreateDevIB(nSize, DB);

  ValidateIBPools();

  return nIB;
}

bool CDevBufferMan::ReleaseVB(int32 nVB)
{
  assert(nVB < m_VBs.size());
  if (nVB >= m_VBs.size())
    return false;

  SDevBuffer &DB = m_VBs[nVB];
  assert(DB.m_nSize > 0);
  if (DB.m_pPoolItem)
    ReleaseDevVBInPool(DB);
  else
    ReleaseDevVB(DB);
  m_FreeVBSlots.push_back(nVB);
 
  DB.m_nSize = 0;

  ValidateVBPools();

  return true;
}

bool CDevBufferMan::ReleaseIB(int32 nIB)
{
  assert(nIB < m_IBs.size());
  if (nIB >= m_IBs.size())
    return false;

  SDevBuffer &DB = m_IBs[nIB];
  assert(DB.m_nSize > 0);
  if (DB.m_pPoolItem)
    ReleaseDevIBInPool(DB);
  else
    ReleaseDevIB(DB);
  m_FreeIBSlots.push_back(nIB);

  DB.m_nSize = 0;

  ValidateIBPools();

  return true;
}

SDevBuffer *CDevBufferMan::GetDevVB(int32 nVB)
{
  assert(nVB < m_VBs.size());
  if (nVB >= m_VBs.size())
    return NULL;

  SDevBuffer *pDB = &m_VBs[nVB];
  assert(pDB->m_nSize > 0);

  return pDB;
}
SDevBuffer *CDevBufferMan::GetDevIB(int32 nIB)
{
  assert(nIB < m_IBs.size());
  if (nIB >= m_IBs.size())
    return NULL;

  SDevBuffer *pDB = &m_IBs[nIB];
  assert(pDB->m_nSize > 0);

  return pDB;
}

D3DVertexBuffer *CDevBufferMan::GetD3DVB(int32 nVB, int32* nOffs)
{
  SDevBuffer *pDB = GetDevVB(nVB);
  assert(pDB);
  if (pDB)
  {
    if (pDB->m_pPoolItem)
      *nOffs = pDB->m_pPoolItem->m_nBufOffset;
    else
      *nOffs = 0;
    return pDB->m_D3DBuf.m_pVB;
  }
  return NULL;
}

D3DIndexBuffer  *CDevBufferMan::GetD3DIB(int32 nIB, int32* nOffs)
{
  SDevBuffer *pDB = GetDevIB(nIB);
  assert(pDB);
  if (pDB)
  {
    if (pDB->m_pPoolItem)
      *nOffs = pDB->m_pPoolItem->m_nBufOffset;
    else
      *nOffs = 0;
    return pDB->m_D3DBuf.m_pIB;
  }
  *nOffs = 0;
  return NULL;
}

void *CDevBufferMan::UpdateVB(int32 nVB, const void *pData, int32 nSize, int32 nOffset)
{
  assert(pData);

  byte *pDst = (byte *)LockVB(nVB, nOffset, nSize, FSL_WRITE);
  assert(pDst);
  if (!pDst)
    return NULL;

  cryMemcpy(pDst, pData, nSize, MC_CPU_TO_GPU);

  UnlockVB(nVB);

  return pDst;
}
void *CDevBufferMan::UpdateIB(int32 nIB, const void *pData, int32 nSize, int32 nOffset)
{
  assert(pData);

  byte *pDst = (byte *)LockIB(nIB, nOffset, nSize, FSL_WRITE);
  assert(pDst);
  if (!pDst)
    return NULL;

  cryMemcpy(pDst, pData, nSize, MC_CPU_TO_GPU);

  UnlockIB(nIB);

  return pDst;
}

void *CDevBufferMan::LockVB(D3DVertexBuffer *pVB, int32 nOffset, uint32 nFlags)
{
  byte *pDst = NULL;
  HRESULT hr = S_OK;
#if defined (DIRECT3D9)
  D3DVERTEXBUFFER_DESC Desc;
  pVB->GetDesc(&Desc);





  int nDevSize = Desc.Size - nOffset;
  hr = pVB->Lock(nOffset, nDevSize, (void **)&pDst, 0);

#elif defined (DIRECT3D10)
  D3D11_BUFFER_DESC Desc;
  pVB->GetDesc(&Desc);
  int nDevSize = Desc.ByteWidth - nOffset;
  // Get temporary vertex buffer
  DynamicVB<byte> *pTempBuf = static_cast<DynamicVB<byte> *>(m_pVBTemp[m_nCurStagedVB]);
  if (nDevSize > pTempBuf->GetBytesCount())
  {
    assert(0);
    return NULL;
  }
  else
  if (nDevSize+pTempBuf->GetBytesOffset() > pTempBuf->GetBytesCount())
  {
    m_nCurStagedVB++;
    if (m_nCurStagedVB > m_nMaxStagingBuffers-1)
      m_nCurStagedVB = 0;
    pTempBuf = static_cast<DynamicVB<byte> *>(m_pVBTemp[m_nCurStagedVB]);
  }
  uint32 nOffsTemp = 0;
  if (nFlags & FSL_READ)
  {
    int nOffs = pTempBuf->GetBytesOffset();
    if (nOffs + nDevSize > pTempBuf->GetBytesCount())
      nOffs = 0;
    D3D11_BOX box;
    ZeroStruct(box);
    box.left = nOffset;
    box.right = nOffset + nDevSize;
    box.bottom = 1;
    box.back = 1;
    gcpRendD3D->m_pd3dDeviceContext->CopySubresourceRegion(pTempBuf->m_pVB, 0, nOffs, 0, 0, pVB, 0, &box);
  }
  pDst = pTempBuf->Lock(nDevSize, nOffsTemp, (nFlags & FSL_WRITE) != 0);
#endif
  assert(hr == S_OK);
  return pDst;
}
void CDevBufferMan::UnlockVB(D3DVertexBuffer *pVB)
{
  byte *pDst = NULL;
  HRESULT hr = S_OK;
#if defined (DIRECT3D9)
  hr = pVB->Unlock();
#elif defined (DIRECT3D10)
  DynamicVB<byte> *pTempBuf = static_cast<DynamicVB<byte> *>(m_pVBTemp[m_nCurStagedVB]);
  pTempBuf->Unlock();
#endif
  assert(hr == S_OK);
}

void *CDevBufferMan::LockIB(D3DIndexBuffer *pIB, int32 nOffset, uint32 nFlags)
{
  uint16 *pDst = NULL;
  HRESULT hr = S_OK;
#if defined (DIRECT3D9)
  D3DINDEXBUFFER_DESC Desc;
  pIB->GetDesc(&Desc);





  int nDevSize = Desc.Size - (nOffset*2);
  hr = pIB->Lock(nOffset, nDevSize, (void **)&pDst, 0);

#elif defined (DIRECT3D10)
  D3D11_BUFFER_DESC Desc;
  pIB->GetDesc(&Desc);
  int nInds = (Desc.ByteWidth>>1) - nOffset;
  // Get temporary vertex buffer
  DynamicIB<uint16> *pTempBuf = static_cast<DynamicIB<uint16> *>(m_pIBTemp[m_nCurStagedIB]);
  if (nInds > pTempBuf->GetCount())
  {
    assert(0);
    return NULL;
  }
  else
  if (nInds+pTempBuf->GetOffset() > pTempBuf->GetCount())
  {
    m_nCurStagedIB++;
    if (m_nCurStagedIB > m_nMaxStagingBuffers-1)
      m_nCurStagedIB = 0;
    pTempBuf = static_cast<DynamicIB<uint16> *>(m_pIBTemp[m_nCurStagedIB]);
  }
  uint32 nOffsTemp = 0;
  if (nFlags & FSL_READ)
  {
    int nOffs = pTempBuf->GetOffset();
    if (nOffs + nInds > pTempBuf->GetCount())
      nOffs = 0;
    D3D11_BOX box;
    ZeroStruct(box);
    box.left = nOffset*2;
    box.right = (nOffset + nInds)*2;
    box.bottom = 1;
    box.back = 1;
    gcpRendD3D->m_pd3dDeviceContext->CopySubresourceRegion(pTempBuf->GetInterface(), 0, nOffs*2, 0, 0, pIB, 0, &box);
  }
  pDst = pTempBuf->Lock(nInds, nOffsTemp, (nFlags & FSL_WRITE) != 0);
#endif
  assert(hr == S_OK);
  return pDst;
}
void CDevBufferMan::UnlockIB(D3DIndexBuffer *pIB)
{
  byte *pDst = NULL;
  HRESULT hr = S_OK;
#if defined (DIRECT3D9)
  hr = pIB->Unlock();
#elif defined (DIRECT3D10)
  DynamicIB<uint16> *pTempBuf = static_cast<DynamicIB<uint16> *>(m_pIBTemp[m_nCurStagedIB]);
  pTempBuf->Unlock();
#endif
  assert(hr == S_OK);
}

void *CDevBufferMan::LockVB(int32 nVB, int32 nOffset, int32 nSize, uint32 nFlags)
{
  int32 nDevOffs = 0;
  D3DVertexBuffer *pVB = GetD3DVB(nVB, &nDevOffs);
  nDevOffs += nOffset;

  byte *pDst = NULL;
  HRESULT hr = S_OK;
#if defined (DIRECT3D9)





  SDevBuffer *pDB = GetDevVB(nVB);
  int32 nDevSize = pDB->m_nSize;
  if (nSize && nSize < nDevSize)
    nDevSize = nSize;
  hr = pVB->Lock(nDevOffs, nDevSize, (void **)&pDst, 0);

#elif defined (DIRECT3D10)
  SDevBuffer *pDB = GetDevVB(nVB);
  int32 nDevSize = pDB->m_nSize;
  if (nSize && nSize < nDevSize)
    nDevSize = nSize;
  // Get temporary vertex buffer
  DynamicVB<byte> *pTempBuf = static_cast<DynamicVB<byte> *>(m_pVBTemp[m_nCurStagedVB]);
  pDB->m_bTempBuf = false;
  if (nDevSize > pTempBuf->GetBytesCount())
  {
    pDB->m_bTempBuf = true;
    pTempBuf = new DynamicVB <byte>(gcpRendD3D->m_pd3dDevice, 0, nDevSize, true);
  }
  else
  if (nDevSize+pTempBuf->GetBytesOffset() > pTempBuf->GetBytesCount())
  {
    m_nCurStagedVB++;
    if (m_nCurStagedVB > m_nMaxStagingBuffers-1)
      m_nCurStagedVB = 0;
    pTempBuf = static_cast<DynamicVB<byte> *>(m_pVBTemp[m_nCurStagedVB]);
  }
  pDB->m_pStagedVB = pTempBuf;
  pDB->m_nLockFlags = nFlags;
  uint32 nOffsTemp = 0;
  if (nFlags & FSL_READ)
  {
    int nOffs = pTempBuf->GetBytesOffset();
    if (nOffs + nDevSize > pTempBuf->GetBytesCount())
      nOffs = 0;
    D3D11_BOX box;
    ZeroStruct(box);
    box.left = nDevOffs;
    box.right = nDevOffs + nDevSize;
    box.bottom = 1;
    box.back = 1;
    gcpRendD3D->m_pd3dDeviceContext->CopySubresourceRegion(pTempBuf->m_pVB, 0, nOffs, 0, 0, pVB, 0, &box);
  }
  pDst = pTempBuf->Lock(nDevSize, nOffsTemp);
  pDB->m_nStagedOffset = nOffsTemp;
  pDB->m_nDevOffset = nOffset + nDevOffs;
  pDB->m_nStagedSize = nDevSize;
#endif
  assert(hr == S_OK);
  return pDst;
}

void CDevBufferMan::UnlockVB(int32 nVB)
{
  int32 nDevOffs = 0;
  D3DVertexBuffer *pVB = GetD3DVB(nVB, &nDevOffs);
  assert(pVB);
  if (!pVB)
    return;

#if defined (DIRECT3D9)
  pVB->Unlock();
#elif defined (DIRECT3D10)
  SDevBuffer *pDB = GetDevVB(nVB);
  DynamicVB<byte> *pStagedVB = static_cast<DynamicVB<byte> *>(pDB->m_pStagedVB);
  assert(pStagedVB);
  if (!pStagedVB)
    return;
  pStagedVB->Unlock();
  if (pDB->m_nLockFlags & FSL_WRITE)
  {
    D3D11_BOX box;
    ZeroStruct(box);
    box.left = pDB->m_nStagedOffset;
    box.right = pDB->m_nStagedSize+box.left;
    box.bottom = 1;
    box.back = 1;
    gcpRendD3D->m_pd3dDeviceContext->CopySubresourceRegion(pVB, 0, pDB->m_nDevOffset, 0, 0, pStagedVB->m_pVB, 0, &box);
  }
  if (pDB->m_bTempBuf)
  {
    pDB->m_bTempBuf = false;
    SAFE_DELETE(pStagedVB);
  }
  pDB->m_pStagedVB = NULL;
#endif
}

void *CDevBufferMan::LockIB(int32 nIB, int32 nOffset, int32 nSize, uint32 nFlags)
{
  int32 nDevOffs = 0;
  D3DIndexBuffer *pIB = GetD3DIB(nIB, &nDevOffs);
  assert (pIB);
  if (!pIB)
    return NULL;

  nDevOffs += nOffset;

  byte *pDst = NULL;
  HRESULT hr = S_OK;
#if defined (DIRECT3D9)




  SDevBuffer *pDB = GetDevIB(nIB);
  int32 nDevSize = pDB->m_nSize;
  if (nSize && nSize < nDevSize)
    nDevSize = nSize;
  hr = pIB->Lock(nDevOffs, nDevSize, (void **)&pDst, 0);

#elif defined (DIRECT3D10)
  SDevBuffer *pDB = GetDevIB(nIB);
  int32 nDevSize = pDB->m_nSize;
  if (nSize && nSize < nDevSize)
    nDevSize = nSize;
  int nInds = nDevSize >> 1;
  // Get temporary index buffer
  DynamicIB<uint16> *pTempBuf = static_cast<DynamicIB<uint16> *>(m_pIBTemp[m_nCurStagedIB]);
  pDB->m_bTempBuf = false;
  if (nInds > pTempBuf->GetCount())
  {
    pDB->m_bTempBuf = true;
    pTempBuf = new DynamicIB <uint16>(gcpRendD3D->m_pd3dDevice, nInds, true);
  }
  else
  if (nInds+pTempBuf->GetOffset() > pTempBuf->GetCount())
  {
    m_nCurStagedIB++;
    if (m_nCurStagedIB > m_nMaxStagingBuffers-1)
      m_nCurStagedIB = 0;
    pTempBuf = static_cast<DynamicIB<uint16> *>(m_pIBTemp[m_nCurStagedIB]);
  }
  pDB->m_pStagedVB = pTempBuf;
  pDB->m_nLockFlags = nFlags;
  uint32 nOffsTemp = 0;
  if (nFlags & FSL_READ)
  {
    int nOffs = pTempBuf->GetOffset();
    if (nOffs + nInds > pTempBuf->GetCount())
      nOffs = 0;
    nOffs *= sizeof(uint16);
    D3D11_BOX box;
    ZeroStruct(box);
    box.left = nDevOffs;
    box.right = nDevOffs + nDevSize;
    box.bottom = 1;
    box.back = 1;
    gcpRendD3D->m_pd3dDeviceContext->CopySubresourceRegion(pTempBuf->GetInterface(), 0, nOffs, 0, 0, pIB, 0, &box);
  }
  pDst = (byte *)pTempBuf->Lock(nInds, nOffsTemp);
  pDB->m_nStagedOffset = nOffsTemp * sizeof(uint16);
  pDB->m_nDevOffset = nOffset + nDevOffs;
  pDB->m_nStagedSize = nDevSize;
#endif
  assert(hr == S_OK);
  return pDst;
}

void CDevBufferMan::UnlockIB(int32 nIB)
{
  int32 nDevOffs = 0;
  D3DIndexBuffer *pIB = GetD3DIB(nIB, &nDevOffs);
  assert(pIB);
  if (!pIB)
    return;

#if defined (DIRECT3D9)
  pIB->Unlock();
#elif defined (DIRECT3D10)
  SDevBuffer *pDB = GetDevIB(nIB);
  DynamicIB<uint16> *pStagedIB = static_cast<DynamicIB<uint16> *>(pDB->m_pStagedVB);
  assert(pStagedIB);
  if (!pStagedIB)
    return;
  pStagedIB->Unlock();
  if (pDB->m_nLockFlags & FSL_WRITE)
  {
    D3D11_BOX box;
    ZeroStruct(box);
    box.left = pDB->m_nStagedOffset;
    box.right = pDB->m_nStagedSize+box.left;
    box.bottom = 1;
    box.back = 1;
    gcpRendD3D->m_pd3dDeviceContext->CopySubresourceRegion(pIB, 0, pDB->m_nDevOffset, 0, 0, pStagedIB->GetInterface(), 0, &box);
  }
  if (pDB->m_bTempBuf)
  {
    pDB->m_bTempBuf = false;
    SAFE_DELETE(pStagedIB);
  }
  pDB->m_pStagedVB = NULL;
#endif
}

//===============================================================================================================

int32 CDevBufferMan::CreateDevVB(int32 nSizeBuf, SDevBuffer& DB)
{
  DB.m_nSize = nSizeBuf;
  CDeviceManager *pDM = &gRenDev->m_DevMan;
  HRESULT hr = pDM->CreateVertexBuffer(nSizeBuf, USAGE_WRITEONLY, D3DPOOL_MANAGED, &DB.m_D3DBuf.m_pVB);
  assert(hr == S_OK);

  if (hr == S_OK)
    return AllocateVBSlot(DB);
  return -1;
}
int32 CDevBufferMan::CreateDevVBInPool(int32 nSizeBuf, SDevBuffer& DB)
{
  DB.m_nSize = nSizeBuf;
  DB.m_pPoolItem = new SDevPoolItem;
  uint32 i;
  HRESULT hr = S_OK;
  CDeviceManager *pDM = &gRenDev->m_DevMan;
  int32 nVB = -1;

  for (i=0; i<m_VBPools.size(); i++)
  {
    SDevPool &Pool = m_VBPools[i];
    if (AllocateChunk(nSizeBuf, Pool, DB.m_pPoolItem))
    {
      assert(Pool.m_nPoolSize == POOL_SIZE_VB);
      if (!Pool.m_D3DBuf.m_pVB)
      {
        assert(Pool.m_nAllocatedSize == 0);
        hr = pDM->CreateVertexBuffer(POOL_SIZE_VB, USAGE_WRITEONLY, D3DPOOL_MANAGED, &Pool.m_D3DBuf.m_pVB);
      }
      Pool.m_nAllocatedSize += nSizeBuf;
      break;
    }
  }
  if (i == m_VBPools.size())
  {
    SDevPool Pool(POOL_SIZE_VB);
    Pool.m_nAllocatedSize = nSizeBuf;
    hr = pDM->CreateVertexBuffer(POOL_SIZE_VB, USAGE_WRITEONLY, D3DPOOL_MANAGED, &Pool.m_D3DBuf.m_pVB);
    m_VBPools.push_back(Pool);
    AllocateChunk(nSizeBuf, m_VBPools[i], DB.m_pPoolItem);
  }
  DB.m_pPoolItem->m_nPool = i;
  DB.m_D3DBuf.m_pVB = m_VBPools[i].m_D3DBuf.m_pVB;
  assert(hr == S_OK);
  if (hr == S_OK)
    nVB = AllocateVBSlot(DB);

  ValidateVBPools();

  return nVB;
}

int32 CDevBufferMan::CreateDevIB(int32 nSizeBuf, SDevBuffer& DB)
{
  DB.m_nSize = nSizeBuf;
  CDeviceManager *pDM = &gRenDev->m_DevMan;
  HRESULT hr = pDM->CreateIndexBuffer(nSizeBuf, USAGE_WRITEONLY, D3DPOOL_MANAGED, &DB.m_D3DBuf.m_pIB);
  assert(hr == S_OK);

  if (hr == S_OK)
    return AllocateIBSlot(DB);
  return -1;
}
int32 CDevBufferMan::CreateDevIBInPool(int32 nSizeBuf, SDevBuffer& DB)
{
  DB.m_nSize = nSizeBuf;
  DB.m_pPoolItem = new SDevPoolItem;
  uint32 i;
  HRESULT hr = S_OK;
  CDeviceManager *pDM = &gRenDev->m_DevMan;
  int nIB = -1;

  for (i=0; i<m_IBPools.size(); i++)
  {
    SDevPool &Pool = m_IBPools[i];
    if (AllocateChunk(nSizeBuf, Pool, DB.m_pPoolItem))
    {
      assert(Pool.m_nPoolSize == POOL_SIZE_IB);
      if (!Pool.m_D3DBuf.m_pIB)
      {
        assert(Pool.m_nAllocatedSize == 0);
        hr = pDM->CreateIndexBuffer(POOL_SIZE_IB, USAGE_WRITEONLY, D3DPOOL_MANAGED, &Pool.m_D3DBuf.m_pIB);
      }
      Pool.m_nAllocatedSize += nSizeBuf;
      break;
    }
  }
  if (i == m_IBPools.size())
  {
    SDevPool Pool(POOL_SIZE_IB);
    Pool.m_nAllocatedSize = nSizeBuf;
    hr = pDM->CreateIndexBuffer(POOL_SIZE_IB, USAGE_WRITEONLY, D3DPOOL_MANAGED, &Pool.m_D3DBuf.m_pIB);
    m_IBPools.push_back(Pool);
    AllocateChunk(nSizeBuf, m_IBPools[i], DB.m_pPoolItem);
  }
  DB.m_pPoolItem->m_nPool = i;
  DB.m_D3DBuf.m_pIB = m_IBPools[i].m_D3DBuf.m_pIB;
  assert(hr == S_OK);
  if (hr == S_OK)
    nIB = AllocateIBSlot(DB);

  return nIB;
}

bool CDevBufferMan::ReleaseDevVB(SDevBuffer& DB)
{
  SAFE_RELEASE(DB.m_D3DBuf.m_pVB);
  return true;
}
bool CDevBufferMan::ReleaseDevVBInPool(SDevBuffer& DB)
{
  SDevPoolItem *pItem = DB.m_pPoolItem;
  SDevPool& Pool = m_VBPools[pItem->m_nPool];
  Pool.m_nAllocatedSize -= DB.m_nSize;
  bool bRes = gRenDev->ReleaseChunk(pItem->m_nBufOffset, Pool.m_AllocInfo);
  if (!bRes)
    iLog->Log("Error: CDevBufferMan::ReleaseDevVBInPool: chunk not found");
  SAFE_DELETE(DB.m_pPoolItem);
  if (!Pool.m_nAllocatedSize)
  {
    assert(Pool.m_AllocInfo.size() == 0);
    SAFE_RELEASE(Pool.m_D3DBuf.m_pVB);
  }

  return true;
}

bool CDevBufferMan::ReleaseDevIB(SDevBuffer& DB)
{
  SAFE_RELEASE(DB.m_D3DBuf.m_pIB);
  return true;
}
bool CDevBufferMan::ReleaseDevIBInPool(SDevBuffer& DB)
{
  SDevPoolItem *pItem = DB.m_pPoolItem;
  SDevPool& Pool = m_IBPools[pItem->m_nPool];
  Pool.m_nAllocatedSize -= DB.m_nSize;
  bool bRes = gRenDev->ReleaseChunk(pItem->m_nBufOffset, Pool.m_AllocInfo);
  if (!bRes)
    iLog->Log("Error: CDevBufferMan::ReleaseDevIBInPool: chunk not found");
  SAFE_DELETE(DB.m_pPoolItem);
  if (!Pool.m_nAllocatedSize)
  {
    assert(Pool.m_AllocInfo.size() == 0);
    SAFE_RELEASE(Pool.m_D3DBuf.m_pIB);
  }

  return true;
}

void CDevBufferMan::_ValidateVBPools()
{
#ifdef _DEBUG
  uint32 i, j;

  for (i=0; i<m_VBPools.size(); i++)
  {
    SDevPool &Pool = m_VBPools[i];
    assert(Pool.m_nPoolSize == POOL_SIZE_VB);
    if (Pool.m_D3DBuf.m_pVB)
    {
      assert(Pool.m_nAllocatedSize != 0);
      int nSize = 0;
      for (j=0; j<m_VBs.size(); j++)
      {
        SDevBuffer &DB = m_VBs[j];
        if (DB.m_pPoolItem)
        {
          if (DB.m_pPoolItem->m_nPool == i)
            nSize += DB.m_nSize;
        }
        else
        {
          assert(DB.m_nSize == 0 || DB.m_nSize > POOL_SIZE_VB);
        }
      }
      assert (nSize == Pool.m_nAllocatedSize);
    }
    else
    {
      assert(Pool.m_nAllocatedSize == 0);
    }
  }
#endif
}

void CDevBufferMan::_ValidateIBPools()
{
#ifdef _DEBUG
  uint32 i, j;

  for (i=0; i<m_IBPools.size(); i++)
  {
    SDevPool &Pool = m_IBPools[i];
    assert(Pool.m_nPoolSize == POOL_SIZE_IB);
    if (Pool.m_D3DBuf.m_pIB)
    {
      assert(Pool.m_nAllocatedSize != 0);
      int nSize = 0;
      for (j=0; j<m_IBs.size(); j++)
      {
        SDevBuffer &DB = m_IBs[j];
        if (DB.m_pPoolItem)
        {
          if (DB.m_pPoolItem->m_nPool == i)
            nSize += DB.m_nSize;
        }
        else
        {
          assert(DB.m_nSize == 0 || DB.m_nSize > POOL_SIZE_IB);
        }
      }
      assert (nSize == Pool.m_nAllocatedSize);
    }
    else
    {
      assert(Pool.m_nAllocatedSize == 0);
    }
  }
#endif
}

bool CDevBufferMan::AllocateChunk(int32 nSizeBuf, SDevPool& Pool, SDevPoolItem *pPoolItem)
{
  assert(nSizeBuf);

  alloc_info_struct *pAI = gRenDev->GetFreeChunk(nSizeBuf, Pool.m_nPoolSize, Pool.m_AllocInfo, NULL);
  if (pAI)
  {
    pPoolItem->m_nBufOffset = pAI->ptr;
    return true;
  }

  return false;
}

int32 CDevBufferMan::AllocateVBSlot(SDevBuffer& DB)
{
  int32 nVB = -1;
  if (m_FreeVBSlots.size())
  {
    nVB = m_FreeVBSlots[m_FreeVBSlots.size()-1];
    m_VBs[nVB] = DB;
    m_FreeVBSlots.pop_back();
  }
  else
  {
    nVB = m_VBs.size();
    m_VBs.push_back(DB);
  }

  return nVB;
}

int32 CDevBufferMan::AllocateIBSlot(SDevBuffer& DB)
{
  int32 nIB = -1;
  if (m_FreeIBSlots.size())
  {
    nIB = m_FreeIBSlots[m_FreeIBSlots.size()-1];
    m_IBs[nIB] = DB;
    m_FreeIBSlots.pop_back();
  }
  else
  {
    nIB = m_IBs.size();
    m_IBs.push_back(DB);
  }

  return nIB;
}

//==================================================================================================

void CDevBufferMan::ReleaseVBuffer(CVertexBuffer* pVB)
{
  SAFE_DELETE(pVB);
}

void CDevBufferMan::ReleaseIBuffer(CIndexBuffer* pIB)
{
  SAFE_DELETE(pIB);
}

CVertexBuffer *CDevBufferMan::CreateVBuffer(int32 nVerts, EVertexFormat eVF, const char *szName)
{
  CVertexBuffer *pVB = new CVertexBuffer(NULL, eVF);
  pVB->m_nVerts = nVerts;
  pVB->m_VS.m_nDevID = CreateVB(nVerts * CRenderMesh2::m_cSizeVF[eVF]);

  return pVB;
}

CIndexBuffer *CDevBufferMan::CreateIBuffer(int32 nInds, const char *szName)
{
  CIndexBuffer *pIB = new CIndexBuffer(NULL);
  pIB->m_nInds = nInds;
  pIB->m_VS.m_nDevID = CreateIB(nInds * sizeof(uint16));

  return pIB;
}

void *CDevBufferMan::UpdateVBuffer(CVertexBuffer *pVB, void *pVerts, int32 nVerts)
{
  assert(pVB->m_VS.m_nDevID >= 0);
  void *pLocked = UpdateVB(pVB->m_VS.m_nDevID, pVerts, nVerts * CRenderMesh2::m_cSizeVF[pVB->m_eVF], 0);

  return pLocked;
}

void *CDevBufferMan::UpdateIBuffer(CIndexBuffer *pIB, void *pInds, int32 nInds)
{
  assert(pIB->m_VS.m_nDevID >= 0);
  void *pLocked = UpdateIB(pIB->m_VS.m_nDevID, pInds, nInds * sizeof(uint16), 0);

  return pLocked;
}

CVertexBuffer::~CVertexBuffer()
  {
  if (m_VS.m_nDevID >= 0)
  {
    gRenDev->m_DevBufMan.ReleaseVB(m_VS.m_nDevID);
    m_VS.m_nDevID = -1;
  }
}

CIndexBuffer::~CIndexBuffer()
{
  if (m_VS.m_nDevID >= 0)
  {
    gRenDev->m_DevBufMan.ReleaseIB(m_VS.m_nDevID);
    m_VS.m_nDevID = -1;
  }
}