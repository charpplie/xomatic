/*=============================================================================
DeviceManager.cpp : Device manager
Copyright (c) 2001-2009 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Khonich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "../DriverD3D.h"

CDeviceManager::CDeviceManager(D3DDevice *pDevice)
{
  m_pDevice = pDevice;
}

HRESULT CDeviceManager::Create2DTexture(uint32 nWidth, uint32 nHeight, uint32 nMips, uint32 nUsage, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI)
{
  HRESULT hr = S_OK;

  CDeviceTexture *pDeviceTexture = new CDeviceTexture();
  D3DTexture *pD3DTex = NULL;





























#if defined (DIRECT3D9)
  hr = m_pDevice->CreateTexture(nWidth, nHeight, nMips, nUsage & ~USAGE_CUSTOM, Format, Pool, &pD3DTex, NULL);
#elif defined (DIRECT3D10) || defined (PS3) 
  uint32 nBindFlags = D3D11_BIND_SHADER_RESOURCE;
  if (nUsage & USAGE_DEPTH_STENCIL)
    nBindFlags |= D3D11_BIND_DEPTH_STENCIL;
  else
  if (nUsage & USAGE_RENDER_TARGET)
    nBindFlags |= D3D11_BIND_RENDER_TARGET;
  D3D11_TEXTURE2D_DESC Desc;
  ZeroStruct(Desc);
  Desc.Width = nWidth;
  Desc.Height = nHeight;
  Desc.MipLevels = nMips;
  Desc.Format = Format;
  Desc.ArraySize = pTI ? pTI->m_nArraySize : 1;
  Desc.BindFlags = nBindFlags;
  Desc.CPUAccessFlags = (nUsage & USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
  Desc.Usage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
  Desc.SampleDesc.Count = pTI ? pTI->m_nMSAASamples : 1;
  Desc.SampleDesc.Quality = pTI ? pTI->m_nMSAAQuality : 0;
	// AntonK: supported only by DX11 feature set
	// needs to be enabled for pure DX11 context
	//if(nUsage & USAGE_STREAMING)
	//	Desc.MiscFlags = D3D11_RESOURCE_MISC_RESOURCE_CLAMP;
  D3D11_SUBRESOURCE_DATA *pSRD = NULL;
  D3D11_SUBRESOURCE_DATA SRD[20];
  if (pTI && pTI->m_pData)
  {
    pSRD = &SRD[0];
    for (int i=0; i<nMips; i++)
    {
      SRD[i].pSysMem = pTI->m_pData[i].pSysMem;
      SRD[i].SysMemPitch = pTI->m_pData[i].SysMemPitch;
      SRD[i].SysMemSlicePitch = pTI->m_pData[i].SysMemSlicePitch;
    }
  }
  hr = m_pDevice->CreateTexture2D(&Desc, pSRD, &pD3DTex);
#endif
	assert(SUCCEEDED(hr));

	pDeviceTexture->m_pD3DTexture = pD3DTex;

	assert(pDeviceTexture->m_pD3DTexture);

  *ppDevTexture = pDeviceTexture;

  return hr;
}

HRESULT CDeviceManager::CreateCubeTexture(uint32 nSize, uint32 nMips, uint32 nUsage, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI)
{
  HRESULT hr = S_OK;

  CDeviceTexture *pDeviceTexture = new CDeviceTexture();
  D3DCubeTexture *pD3DTex = NULL;





























#if defined (DIRECT3D9)
  hr = m_pDevice->CreateCubeTexture(nSize, nMips, nUsage & ~USAGE_CUSTOM, Format, Pool, &pD3DTex, NULL);
#elif defined (DIRECT3D10) || defined (PS3) 
  uint32 nBindFlags = D3D11_BIND_SHADER_RESOURCE;
  if (nUsage & USAGE_DEPTH_STENCIL)
    nBindFlags |= D3D11_BIND_DEPTH_STENCIL;
  else
  if (nUsage & USAGE_RENDER_TARGET)
    nBindFlags |= D3D11_BIND_RENDER_TARGET;
  D3D11_TEXTURE2D_DESC Desc;
  ZeroStruct(Desc);
  Desc.Width = nSize;
  Desc.Height = nSize;
  Desc.MipLevels = nMips;
  Desc.Format = Format;
  Desc.ArraySize = 6;
  Desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
  Desc.CPUAccessFlags = (nUsage & USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
  Desc.Usage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
  Desc.SampleDesc.Count = 1;
  Desc.SampleDesc.Quality = 0;
  Desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	// AntonK: supported only by DX11 feature set
	// needs to be enabled for pure DX11 context
	//if(nUsage & USAGE_STREAMING)
	//	Desc.MiscFlags |= D3D11_RESOURCE_MISC_RESOURCE_CLAMP;
	D3D11_SUBRESOURCE_DATA *pSRD = NULL;
  D3D11_SUBRESOURCE_DATA SRD[g_nD3D10MaxSupportedSubres];
  if (pTI && pTI->m_pData)
  {
    pSRD = &SRD[0];
    for (int j=0; j<6; j++)
    {
      for (int i=0; i<nMips; i++)
      {
        int nSubresInd = j * nMips + i;
        SRD[nSubresInd].pSysMem = pTI->m_pData[nSubresInd].pSysMem;
        SRD[nSubresInd].SysMemPitch = pTI->m_pData[nSubresInd].SysMemPitch;
        SRD[nSubresInd].SysMemSlicePitch = pTI->m_pData[nSubresInd].SysMemSlicePitch;
      }
    }
  }
  hr = m_pDevice->CreateTexture2D(&Desc, pSRD, &pD3DTex);
#endif
  pDeviceTexture->m_pD3DTexture = pD3DTex;

  *ppDevTexture = pDeviceTexture;

  return hr;
}

HRESULT CDeviceManager::CreateVolumeTexture(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nUsage, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI)
{
  HRESULT hr = S_OK;

  CDeviceTexture *pDeviceTexture = new CDeviceTexture();
  D3DVolumeTexture *pD3DTex = NULL;
#if defined (DIRECT3D9) || defined (XENON)
  hr = m_pDevice->CreateVolumeTexture(nWidth, nHeight, nDepth, nMips, nUsage & ~USAGE_CUSTOM, Format, Pool, &pD3DTex, NULL);
#elif defined (DIRECT3D10) || defined (PS3) 
  uint32 nBindFlags = D3D11_BIND_SHADER_RESOURCE;
  if (nUsage & USAGE_DEPTH_STENCIL)
    nBindFlags |= D3D11_BIND_DEPTH_STENCIL;
  else
  if (nUsage & USAGE_RENDER_TARGET)
    nBindFlags |= D3D11_BIND_RENDER_TARGET;
  D3D11_TEXTURE3D_DESC Desc;
  ZeroStruct(Desc);
  Desc.Width = nWidth;
  Desc.Height = nHeight;
  Desc.Depth = nDepth;
  Desc.MipLevels = nMips;
  Desc.Format = Format;
  Desc.BindFlags = nBindFlags;
  Desc.CPUAccessFlags = 0;
  Desc.Usage = (D3D11_USAGE)nUsage;
  Desc.MiscFlags = 0;
  D3D11_SUBRESOURCE_DATA *pSRD = NULL;
  D3D11_SUBRESOURCE_DATA SRD[20];
  if (pTI && pTI->m_pData)
  {
    pSRD = &SRD[0];
    for (int i=0; i<nMips; i++)
    {
      SRD[i].pSysMem = pTI->m_pData[i].pSysMem;
      SRD[i].SysMemPitch = pTI->m_pData[i].SysMemPitch;
      SRD[i].SysMemSlicePitch = pTI->m_pData[i].SysMemSlicePitch;
    }
  }
  hr = m_pDevice->CreateTexture3D(&Desc, pSRD, &pD3DTex);
#endif
  pDeviceTexture->m_pD3DTexture = pD3DTex;

  *ppDevTexture = pDeviceTexture;

  return hr;
}


HRESULT CDeviceManager::CreateVertexBuffer(uint32 nSize, uint32 nUsage, D3DPOOL Pool, D3DVertexBuffer** ppVB)
{
  HRESULT hr = S_OK;

#if defined (DIRECT3D9) || defined(XENON)



  hr = m_pDevice->CreateVertexBuffer(nSize, nUsage, 0, Pool, ppVB, NULL);
#elif defined (DIRECT3D10) || defined (PS3) 
  D3D11_BUFFER_DESC BufDesc;
  ZeroStruct(BufDesc);
  BufDesc.ByteWidth = nSize;
  BufDesc.Usage = D3D11_USAGE_DEFAULT;
  BufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  BufDesc.CPUAccessFlags = 0;
  BufDesc.MiscFlags = 0; 
  hr = m_pDevice->CreateBuffer(&BufDesc, NULL, ppVB);
#endif
  assert(hr == S_OK);

  return hr;
}

HRESULT CDeviceManager::CreateIndexBuffer(uint32 nSize, uint32 nUsage, D3DPOOL Pool, D3DIndexBuffer** ppIB)
{
  HRESULT hr = S_OK;

#if defined (DIRECT3D9) || defined(XENON)



  hr = m_pDevice->CreateIndexBuffer(nSize, nUsage, D3DFMT_INDEX16, Pool, ppIB, NULL);
#elif defined (DIRECT3D10) || defined (PS3) 
  D3D11_BUFFER_DESC BufDesc;
  ZeroStruct(BufDesc);
  BufDesc.ByteWidth = nSize;
  BufDesc.Usage = D3D11_USAGE_DEFAULT;
  BufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  BufDesc.CPUAccessFlags = 0;
  BufDesc.MiscFlags = 0; 
  hr = m_pDevice->CreateBuffer(&BufDesc, NULL, ppIB);
#endif
  assert(hr == S_OK);

  return hr;
}

//=============================================================================

int32 CDeviceTexture::Release()
{
  int32 nRef = -1;
  if (m_pD3DTexture)
  {
#ifndef PS3

















			nRef = m_pD3DTexture->Release();



#endif
    m_pD3DTexture = NULL;
  }
  if (nRef <= 0)
    delete this;

  return nRef;
}

HRESULT CDeviceTexture::UnlockRect(uint32 nLevel)
{
  if (!m_pD3DTexture)
    return E_FAIL;
  D3DTexture *pD3DTexture = Get2DTexture();
#if defined (DIRECT3D9) || defined (XENON)
  return pD3DTexture->UnlockRect(nLevel);
#elif defined (DIRECT3D10) || defined (PS3) 
  gcpRendD3D->m_pd3dDeviceContext->Unmap(pD3DTexture, nLevel);
#endif
  return S_OK;
}

HRESULT CDeviceTexture::UnlockRect(int32 nCubeSide, uint32 nLevel)
{
  if (!m_pD3DTexture)
    return E_FAIL;
	if(nCubeSide >= 0)
	{
		D3DCubeTexture *pD3DTexture = GetCubeTexture();
	#if defined (DIRECT3D9) || defined (XENON)
		return pD3DTexture->UnlockRect((D3DCUBEMAP_FACES)nCubeSide, nLevel);
	#elif defined (DIRECT3D10) || defined (PS3) 
		gcpRendD3D->m_pd3dDeviceContext->Unmap(pD3DTexture, D3D11CalcSubresource(nLevel, nCubeSide, 1));
	#endif
		return S_OK;
	}
	else
	{
		return UnlockRect(nLevel);
	}
}

HRESULT CDeviceTexture::UnlockBox(uint32 nLevel)
{
  if (!m_pD3DTexture)
    return E_FAIL;
  D3DVolumeTexture *pD3DTexture = GetVolumeTexture();
#if defined (DIRECT3D9) || defined (XENON)
  return pD3DTexture->UnlockBox(nLevel);


#elif defined (DIRECT3D10)
  gcpRendD3D->m_pd3dDeviceContext->Unmap(pD3DTexture, D3D11CalcSubresource(nLevel, 0, 1));
#endif
  return S_OK;
}

HRESULT CDeviceTexture::LockRect(uint32 nLevel, STexLock& Data, uint32 nFlags)
{
	assert(nFlags == 0 || IsPowerOfTwo(nFlags));
  if (!m_pD3DTexture)
    return E_FAIL;
  D3DTexture *pD3DTexture = Get2DTexture();
#if defined (DIRECT3D9) || defined (XENON)
  D3DLOCKED_RECT lrct;
  uint32 nLockFlags = 0;
  if (nFlags & LF_DISCARD)
    nLockFlags |= D3DLOCK_DISCARD;
	if (nFlags & LF_READ)
		nLockFlags |= D3DLOCK_READONLY;
  HRESULT hr = pD3DTexture->LockRect(nLevel, &lrct, NULL, nLockFlags);
  Data.pData = (uint8 *)lrct.pBits;
  Data.Pitch = lrct.Pitch;
  return hr;
#elif defined (DIRECT3D10) || defined (PS3) 
  D3D11_MAPPED_SUBRESOURCE lrct;
  uint32 nLockFlags = 0;
  if (nFlags & LF_READ)
    nLockFlags |= D3D11_MAP_READ;
  if (nFlags & LF_WRITE)
    nLockFlags |= D3D11_MAP_WRITE;
  if (nFlags & LF_DISCARD)
    nLockFlags |= D3D11_MAP_WRITE_DISCARD;
  HRESULT hr = gcpRendD3D->m_pd3dDeviceContext->Map(pD3DTexture, D3D11CalcSubresource(nLevel, 0, 1), (D3D11_MAP)nLockFlags, 0, &lrct);
  Data.pData = (uint8*)lrct.pData;
  Data.Pitch = lrct.RowPitch;
#endif
  return hr;
}

HRESULT CDeviceTexture::LockRect(int32 nCubeSide, uint32 nLevel, STexLock& Data, uint32 nFlags)
{
	assert(nFlags == 0 || IsPowerOfTwo(nFlags));
	if (!m_pD3DTexture)
    return E_FAIL;

	if(nCubeSide < 0)
		return LockRect(nLevel, Data, nFlags);

  D3DCubeTexture *pD3DTexture = GetCubeTexture();
#if defined (DIRECT3D9) || defined (XENON)
  D3DLOCKED_RECT lrct;
  uint32 nLockFlags = 0;
  if (nFlags & LF_DISCARD)
    nLockFlags |= D3DLOCK_DISCARD;
	if (nFlags & LF_READ)
		nLockFlags |= D3DLOCK_READONLY;
  HRESULT hr = pD3DTexture->LockRect((D3DCUBEMAP_FACES)nCubeSide, nLevel, &lrct, NULL, nLockFlags);
  Data.pData = (uint8 *)lrct.pBits;
  Data.Pitch = lrct.Pitch;
  return hr;
#elif defined (DIRECT3D10) || defined (PS3) 
  D3D11_MAPPED_SUBRESOURCE lrct;
  uint32 nLockFlags = 0;
  if (nFlags & LF_READ)
    nLockFlags |= D3D11_MAP_READ;
  if (nFlags & LF_WRITE)
    nLockFlags |= D3D11_MAP_WRITE;
  if (nFlags & LF_DISCARD)
    nLockFlags |= D3D11_MAP_WRITE_DISCARD;
	D3D11_TEXTURE2D_DESC Desc;
	((ID3D11Texture2D*)m_pD3DTexture)->GetDesc(&Desc);
	gcpRendD3D->m_pd3dDeviceContext->Map(pD3DTexture, D3D11CalcSubresource(nLevel, nCubeSide, Desc.MipLevels), (D3D11_MAP)nLockFlags, 0, &lrct);
  Data.pData = (uint8*)lrct.pData;
  Data.Pitch = lrct.RowPitch;
#endif
  return S_OK;
}

HRESULT CDeviceTexture::LockBox(uint32 nLevel, STexLockBox& Data, uint32 nFlags)
{
	assert(nFlags == 0 || IsPowerOfTwo(nFlags));
	if (!m_pD3DTexture)
    return E_FAIL;
  D3DVolumeTexture *pD3DTexture = GetVolumeTexture();
#if defined (DIRECT3D9) || defined (XENON)
  D3DLOCKED_BOX lrct;
  uint32 nLockFlags = 0;
  if (nFlags & LF_DISCARD)
    nLockFlags |= D3DLOCK_DISCARD;
	if (nFlags & LF_READ)
		nLockFlags |= D3DLOCK_READONLY;
  HRESULT hr = pD3DTexture->LockBox(nLevel, &lrct, NULL, nLockFlags);
  Data.pData = (uint8 *)lrct.pBits;
  Data.RowPitch = lrct.RowPitch;
  Data.SlicePitch = lrct.SlicePitch;
  return hr;


#elif defined (DIRECT3D10) 
  D3D11_MAPPED_SUBRESOURCE lrct;
  uint32 nLockFlags = 0;
  if (nFlags & LF_READ)
    nLockFlags |= D3D11_MAP_READ;
  if (nFlags & LF_WRITE)
    nLockFlags |= D3D11_MAP_WRITE;
  if (nFlags & LF_DISCARD)
    nLockFlags |= D3D11_MAP_WRITE_DISCARD;
  gcpRendD3D->m_pd3dDeviceContext->Map(pD3DTexture, D3D11CalcSubresource(nLevel, 0, 1), (D3D11_MAP)nLockFlags, 0, &lrct);
  Data.pData = (uint8*)lrct.pData;
  Data.RowPitch = lrct.RowPitch;
  Data.SlicePitch = lrct.DepthPitch;
#endif
  return S_OK;
}

CDeviceTexture::~CDeviceTexture()
{

















}

alloc_info_struct *CRenderer::GetFreeChunk(int bytes_count, int nBufSize, PodArray<alloc_info_struct>& alloc_info, const char *szSource)
{
  int best_i = -1;
  int min_size = 10000000;

  // find best chunk
  for(int i=0; i<alloc_info.Count(); i++)
  {
    if(!alloc_info[i].busy)
    {
      if(alloc_info[i].bytes_num >= bytes_count)
      {
        if(alloc_info[i].bytes_num < min_size)
        {
          best_i = i;
          min_size = alloc_info[i].bytes_num;
        }
      }
    }
  }

  if(best_i>=0)
  { // use best free chunk
    alloc_info[best_i].busy = true;
    alloc_info[best_i].szSource = szSource;

    int bytes_free = alloc_info[best_i].bytes_num - bytes_count;
    if(bytes_free>0)
    { 
      // modify reused shunk
      alloc_info[best_i].bytes_num = bytes_count;

      // insert another free shunk
      alloc_info_struct new_chunk;
      new_chunk.bytes_num = bytes_free;
      new_chunk.ptr = alloc_info[best_i].ptr + alloc_info[best_i].bytes_num;
      new_chunk.busy = false;

      if(best_i < alloc_info.Count()-1) // if not last
        alloc_info.InsertBefore(new_chunk, best_i+1);
      else
        alloc_info.Add(new_chunk);
    }

    return &alloc_info[best_i];
  }

  int res_ptr = 0;

  int piplevel = alloc_info.Count() ? (alloc_info.Last().ptr - alloc_info[0].ptr) + alloc_info.Last().bytes_num : 0;
  if(piplevel + bytes_count >= nBufSize)
    return NULL;
  else
    res_ptr = piplevel;

  // register new chunk
  alloc_info_struct ai;
  ai.ptr = res_ptr;
  ai.szSource = szSource;
  ai.bytes_num  = bytes_count;
  ai.busy = true;
  alloc_info.Add(ai);

  return &alloc_info[alloc_info.Count()-1];
}

bool CRenderer::ReleaseChunk(int p, PodArray<alloc_info_struct>& alloc_info)
{
  for(int i=0; i<alloc_info.Count(); i++)
  {
    if(alloc_info[i].ptr == p)
    {
      alloc_info[i].busy = false;

      // delete info about last unused chunks
      while(alloc_info.Count() && alloc_info.Last().busy == false)
        alloc_info.Delete(alloc_info.Count()-1);

      // merge unused chunks
      for(int s=0; s<alloc_info.Count()-1; s++)
      {
        assert(alloc_info[s].ptr < alloc_info[s+1].ptr);

        if(alloc_info[s].busy == false)
        {
          if(alloc_info[s+1].busy == false)
          {
            alloc_info[s].bytes_num += alloc_info[s+1].bytes_num;
            alloc_info.Delete(s+1);
            s--;
          }
        }
      }

      return true;
    }
  }

  return false;
}

