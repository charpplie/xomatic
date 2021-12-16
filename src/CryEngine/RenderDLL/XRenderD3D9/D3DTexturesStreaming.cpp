/*=============================================================================
  D3DTexturesStreaming.cpp : Direct3D9 specific texture streaming technology.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
										* Created by Honitch Andrey
- 19:8:2008   12:14 : Refactored by Kaplanyan Anton

=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"

//===============================================================================

#define STREAMED_TEXTURE_USAGE 0

// checks for MT-safety of called functions
#define CHK_RENDTH assert(gcpRendD3D->m_pRT->IsRenderThread())
#define CHK_MAINTH assert(gcpRendD3D->m_pRT->IsMainThread())
#define CHK_MAINORRENDTH assert(gRenDev->m_pRT->IsMainThread() || gRenDev->m_pRT->IsRenderThread())

bool STexPoolItem::IsStillUsedByGPU()
{
	CDeviceTexture* pDeviceTexture = m_pDevTexture;
	if(pDeviceTexture)
	{
    CHK_MAINORRENDTH;











	}
	return false;
}

STexPoolItem::~STexPoolItem()
{
	Unlink();
	UnlinkFree();
	if(m_pDevTexture)
	{
		assert(CTexture::s_nStatsCurManagedStreamedTexMem > m_pOwner->m_Size);
		CryInterlockedAdd(CTexture::s_nStatsCurManagedStreamedTexMem.Addr(), -m_pOwner->m_Size);
	}
	assert(!IsStillUsedByGPU());
#if !defined (XENON)
  SAFE_RELEASE(m_pDevTexture);











#endif	
}

bool CTexture::IsStillUsedByGPU()
{
	CDeviceTexture* pDeviceTexture = m_pDevTexture;
	if (pDeviceTexture)
	{
		CHK_RENDTH;
		D3DBaseTexture *pD3DTex = pDeviceTexture->GetBaseTexture();









	}
	return false;
}








































































































#if defined (DIRECT3D9) || defined(OPENGL) || defined(XENON) || defined(PS3)
// streaming thread
void CTextureStreamLodCallback::StreamAsyncOnComplete(IReadStream *pStream, unsigned nError)
{
  STexStreamLodInfo *pTexLodInfo = (STexStreamLodInfo *)pStream->GetUserData();
  assert(pTexLodInfo->m_pTexture);

  CTexture *tp = pTexLodInfo->m_pTexture;
  if (!tp || !tp->m_pFileTexMips || !tp->IsStreamed())
    return;
	if(pStream->IsError())
		return;




  int n = 0;
  STexCacheFileHeader *fh = &tp->m_CacheFileHeader;
  STexCacheMipHeader *mh = tp->m_pFileTexMips;
  int nMips = tp->GetNumMipsNonVirtual();
  int nStartMip = pTexLodInfo->m_nStartLoadMip;
  int nEndMip = nMips-1;

	const int nOldMipOffset = pTexLodInfo->m_nStartLoadMip - tp->m_nMinMipVidUploaded;
	const int nNumMips = tp->GetNumMipsNonVirtual() - pTexLodInfo->m_nStartLoadMip;
	CTexture::StreamCopyMipsFast(tp->m_pPoolItem, 0 + nOldMipOffset, tp->m_pNewPoolItem, 0, nNumMips);
}

// main thread
void CTextureStreamLodCallback::StreamOnComplete (IReadStream* pStream, unsigned nError)
{
	CHK_MAINORRENDTH;
	int nRefs = pStream->AddRef();
	assert(nRefs > 1);

	if(pStream->GetError() == ERROR_USER_ABORT)
	{
		STexStreamLodInfo *pTexLodInfo = (STexStreamLodInfo *)pStream->GetUserData();
		CTexture *tp = pTexLodInfo->m_pTexture;
		CTexture::StreamRemoveTask(tp, IReadStreamPtr(pStream));
	}

  gRenDev->m_pRT->RC_TexStreamComplete(pStream);
}

// consoles-only, main thread
void CTexture::StreamSetLodNonManagedAsync(int nStartMip)
{
	CHK_RENDTH;
	int nEndMip = m_nMips-1;

  IStreamEngine *pSE = iSystem->GetStreamEngine();
#ifdef DO_RENDERLOG
  if (gRenDev->m_LogFileStr)
    gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Async Start SetLod '%s', Lods: [%d-%d], Time: %.3f\n", m_SrcName.c_str(), nStartMip, nEndMip, iTimer->GetAsyncCurTime());
#endif
  STexStreamLodInfo *pTexCacheFileInfo = new STexStreamLodInfo;
  pTexCacheFileInfo->m_pTexture = this;
  pTexCacheFileInfo->m_nStartLoadMip = nStartMip;
  pTexCacheFileInfo->m_nEndLoadMip = nEndMip;
  pTexCacheFileInfo->m_fStartTime = iTimer->GetCurrTime();
  StreamReadParams StrParams;
  StrParams.nOffset = -1;
  StrParams.dwUserData = (DWORD_PTR)pTexCacheFileInfo;
  StrParams.nLoadTime = 1;
  StrParams.nMaxLoadTime = 3;
	StrParams.nPriority = (int)(1000000.f / max(.1f, m_fLastMipFactor)) + 10 * 1024;	// distance + 10K
  StrParams.pBuffer = NULL;
  StrParams.nSize = 0;
  STexPoolItem *pIT = StreamGetPoolItem(nStartMip, nEndMip-nStartMip+1);
  if (pIT)
  {
    assert(!m_pNewPoolItem);
		m_pNewPoolItem = NULL;
		if(pIT != m_pPoolItem)
			m_pNewPoolItem = pIT;
		else
			assert(0);
  }
	else
		s_bPoolOverflow = TRUE;
	if(m_pNewPoolItem)
		CTexture::StreamAddTask(this, NULL, &pTexCacheFileInfo->m_Callback, &StrParams);
}

int CTexture::StreamSetLod(int nToMip, bool bUnload)
{
	CHK_RENDTH;
	if (bUnload && m_bWasUnloaded)
    return 0;
  if (!IsStreamed() || m_bStreamingInProgress)
    return 0;
  if (m_nMinMipVidUploaded == nToMip)
    return 0;

	// clamp mip level 
	nToMip = min(nToMip, m_nMips-1);

  const int nPrevMinMip = m_nMinMipVidUploaded;

	m_nStreamingSwitchFrameID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
  // Make sure we don't have to request any async loading at this stage
  assert(!m_bStreamingInProgress/*m_TextureStreamTasks.empty()*/);
	int nFreeSize = m_pFileTexMips[nPrevMinMip].m_SideSizeWithMips - m_pFileTexMips[nToMip].m_SideSizeWithMips;
	if(m_eTT == eTT_Cube) nFreeSize *= 6;





	m_nMinMipVidUploaded = nToMip;

	// Release video-memory
	D3DBaseTexture *pBaseTexture = m_pDevTexture->GetBaseTexture();
	assert(pBaseTexture->GetLevelCount() > nToMip);
	pBaseTexture->SetLOD(nToMip);

	CryInterlockedAdd(&m_nDeviceSize, -nFreeSize);
	CryInterlockedAdd(CTexture::s_nStatsCurManagedStreamedTexMem.Addr(), -nFreeSize);

	CTexture::StreamValidateTexSize();

	return nFreeSize;

}

#elif defined (DIRECT3D10)
int CTexture::StreamSetLod(int nToMip, bool bUnload)
{
  if (bUnload && m_bWasUnloaded)
    return 0;
  if (!IsStreamed())
    return 0;
  if (m_nMinMipVidUploaded == nToMip)
    return 0;

	// clamp mip level 
	nToMip = min(nToMip, m_nMips-1);

  int nPrevMinMip = m_nMinMipVidUploaded;

  int nSides = (m_eTT == eTT_Cube) ? 6 : 1;

	// Make sure we don't have to request any async loading at this stage
	if (bUnload && m_bStreamingInProgress)
	{
		AbortStreamingTasks(this);
		assert(!m_bStreamingInProgress);
	}

  STexCacheMipHeader *mh = m_pFileTexMips;

  m_nMinMipVidUploaded = nToMip;

  int nFreeSize = bUnload ? m_pFileTexMips[nPrevMinMip].m_SideSizeWithMips - m_pFileTexMips[nToMip].m_SideSizeWithMips : 0;
	nFreeSize *= nSides;

  CD3D9Renderer *r = gcpRendD3D;
  D3DDevice *dv = r->GetD3DDevice();
  HRESULT hr = S_OK;
  D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
  ZeroStruct(SRVDesc);
  SRVDesc.Format = DeviceFormatFromTexFormat(GetDstFormat());
	if (m_bIsSRGB && gRenDev->IsLinearSpaceShadingEnabled() != 0)
		SRVDesc.Format = ConvertToSRGBFmt(SRVDesc.Format);
  ID3D11ShaderResourceView *pSRV = (ID3D11ShaderResourceView *)m_pDeviceShaderResource;
  SAFE_RELEASE(pSRV);

  // Release video-memory

  if (m_eTT != eTT_Cube)
  {
		SRVDesc.Texture2D.MipLevels = m_nMips - nToMip;
		SRVDesc.Texture2D.MostDetailedMip = nToMip;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    hr = dv->CreateShaderResourceView(m_pDevTexture->Get2DTexture(), &SRVDesc, &pSRV);
  }
  else
  {
    nFreeSize *= 6;
		SRVDesc.TextureCube.MipLevels = m_nMips - nToMip;
		SRVDesc.TextureCube.MostDetailedMip = nToMip;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    hr = dv->CreateShaderResourceView(m_pDevTexture->GetCubeTexture(), &SRVDesc, &pSRV);
  }
  m_pDeviceShaderResource = pSRV;
  assert(hr == S_OK);

	CryInterlockedAdd(&m_nDeviceSize, -nFreeSize);
	assert(CTexture::s_nStatsCurManagedStreamedTexMem >= nFreeSize);
	CryInterlockedAdd(CTexture::s_nStatsCurManagedStreamedTexMem.Addr(), -nFreeSize);
  CTexture::StreamValidateTexSize();

  return nFreeSize;
}
#endif

#if defined (DIRECT3D9) || defined(OPENGL) || defined(XENON)
void CTexture::StreamCopyMips(int nStartMip, int nEndMip, bool bToDevice)
{
	PROFILE_FRAME(Texture_StreamUpload);

	CDeviceManager *pDevMan = &gcpRendD3D->m_DevMan;
	HRESULT h;
	D3DLOCKED_RECT d3dlr;
	STexCacheMipHeader *mh = m_pFileTexMips;

#ifdef _DEBUG
	int nOldMinMipVidUploaded = m_nMinMipVidUploaded;
#endif

	if(bToDevice && !m_pNewPoolItem)
		m_nMinMipVidUploaded = nStartMip;





	D3DFormat d3dFmt = DeviceFormatFromTexFormat(GetDstFormat());
	if ( m_bIsSRGB && gRenDev->IsLinearSpaceShadingEnabled() )
		d3dFmt = CTexture::ConvertToSRGBFmt(d3dFmt);

	if (m_eTT != eTT_Cube)
	{
		CDeviceTexture *pDevTexture = NULL;
		if (bToDevice && m_pNewPoolItem)
		{
			pDevTexture = m_pNewPoolItem->m_pDevTexture;
			assert(pDevTexture);
		}
		else
			pDevTexture = m_pDevTexture;
		if (!pDevTexture && bToDevice)
		{




			PROFILE_FRAME(Texture_StreamCreate);
			if (FAILED(h = pDevMan->Create2DTexture(m_nWidth, m_nHeight, m_nMips, STREAMED_TEXTURE_USAGE, d3dFmt, TEXPOOL, &pDevTexture)))
			{
				assert(false);
				return;
			}
			m_pDevTexture = pDevTexture;

		}
    D3DTexture *pID3DTexture = pDevTexture->Get2DTexture();

		int nMipOffset = m_nMips - pID3DTexture->GetLevelCount();
#if !defined(XENON)
		assert(nMipOffset == 0);
#endif

		if (CRenderer::CV_r_texturesstreamingnoupload && bToDevice)
			return;

		int SizeToLoad = 0;
		if (m_pPoolItem)
		{
			assert(pID3DTexture->GetLevelCount() >= (nEndMip - nStartMip + 1));
			for (int i=nStartMip; i<=nEndMip; i++)
			{
				int nLod = i - nMipOffset;
				SMipData *mp = &mh[i].m_Mips[0];

				if (bToDevice)
					CryInterlockedAdd(&CTexture::s_nTexturesDataBytesUploaded, mh[i].m_SideSize);
#if !defined(XENON)
				D3DTexture* pSysTexture = m_pPoolItem->m_pOwner->m_pSysTexture->Get2DTexture();
				h = pSysTexture->LockRect(nLod, &d3dlr, NULL, 0);


#endif
				assert(h == S_OK);
				SizeToLoad += mh[i].m_SideSize;
#ifndef XENON
				bool bLoaded = false;
				if (mh[i].m_USize < 4 && !IsDXTCompressed(m_eTFDst))
				{
					int nD3DSize = d3dlr.Pitch * mh[i].m_VSize;
					if (nD3DSize != mh[nLod].m_SideSize)
					{
						bLoaded = true;
						byte *pDst = (byte *)d3dlr.pBits;
						byte *pSrc = &mp->DataArray[0];
						int nPitchSrc = CTexture::TextureDataSize(mh[i].m_USize, 1, 1, 1, m_eTFDst);
						if (bToDevice)
						{
							for (int j=0; j<mh[i].m_VSize; j++)
							{
								memcpy(pDst, pSrc, nPitchSrc);
								pSrc += nPitchSrc;
								pDst += d3dlr.Pitch;
							}
						}
						else
						{
							for (int j=0; j<mh[i].m_VSize; j++)
							{
								memcpy(pSrc, pDst, nPitchSrc);
								pSrc += nPitchSrc;
								pDst += d3dlr.Pitch;
							}
						}
					}
				}
				if (!bLoaded)
				{
					// Copy data to system texture 
					if (bToDevice)
					{
						assert(mp->DataArray.GetMemoryUsage());
						memcpy((byte *)d3dlr.pBits, &mp->DataArray[0], mh[i].m_SideSize);
					}
					else
					{
						if(!mp->DataArray.GetMemoryUsage())
							mp->Init(mh[i].m_SideSize, mh[i].m_USize, mh[i].m_VSize);
						memcpy(&mp->DataArray[0], (byte *)d3dlr.pBits, mh[i].m_SideSize);
					}
				}


























































#endif

#if !defined(XENON)
				// Unlock the system texture
				pSysTexture->UnlockRect(nLod);
				D3DSurface* pSrcSurf = NULL;
				D3DSurface* pDestSurf = NULL;
				pID3DTexture->GetSurfaceLevel(nLod, &pDestSurf);
				pSysTexture->GetSurfaceLevel(nLod, &pSrcSurf);
				if (bToDevice)
					h = gcpRendD3D->m_pd3dDevice->UpdateSurface(pSrcSurf, 0, pDestSurf, 0);
				else
					D3DXLoadSurfaceFromSurface(pSrcSurf, NULL, NULL, pDestSurf, NULL, NULL, D3DX_FILTER_NONE, 0);
				SAFE_RELEASE(pDestSurf);
				SAFE_RELEASE(pSrcSurf);


#endif

				if(s_bStreamDontKeepSystem && bToDevice)
					mp->Free();
			}
		}
		else
		{
			for (int i=nStartMip; i<=nEndMip; i++)
			{
				int nLod = i - nMipOffset;
				assert(nLod >= 0);
				SMipData *mp = &mh[i].m_Mips[0];

				if(bToDevice && mp->DataArray.empty() && s_bStreamDontKeepSystem)	// we have this mip already loaded
				{
					continue;
				}

				if (bToDevice)
				{
#if !defined(XENON)
					CryInterlockedAdd(&m_nDeviceSize, mh[i].m_SideSize);
					CryInterlockedAdd(CTexture::s_nStatsCurManagedStreamedTexMem.Addr(), mh[i].m_SideSize);
					CTexture::StreamValidateTexSize();
#endif
					CryInterlockedAdd(&CTexture::s_nTexturesDataBytesUploaded, mh[i].m_SideSize);
				}

				h = pID3DTexture->LockRect(nLod, &d3dlr, NULL, bToDevice?0:D3DLOCK_READONLY);
				assert(h == S_OK);

				bool bLoaded = false;
				SizeToLoad += m_pFileTexMips[i].m_SideSize;
				if (mh[i].m_USize < 4 && !IsDXTCompressed(m_eTFDst))
				{
					int nD3DSize = d3dlr.Pitch * mh[i].m_VSize;
					if (nD3DSize != m_pFileTexMips[i].m_SideSize)
					{
						bLoaded = true;
						byte *pDst = (byte *)d3dlr.pBits;
						byte *pSrc = &mp->DataArray[0];

						int nPitchSrc = CTexture::TextureDataSize(mh[i].m_USize, 1, 1, 1, m_eTFDst);
						if (bToDevice)
						{
							for (int j=0; j<mh[i].m_VSize; j++)
							{
								memcpy(pDst, pSrc, nPitchSrc);
								pSrc += nPitchSrc;
								pDst += d3dlr.Pitch;
							}
						}
						else
						{
							for (int j=0; j<mh[i].m_VSize; j++)
							{
								memcpy(pSrc, pDst, nPitchSrc);
								pSrc += nPitchSrc;
								pDst += d3dlr.Pitch;
							}
						}
					}
				}
				if (!bLoaded)
				{
					if(!mp->DataArray.empty())
					{
						// Copy data to/from video texture 
						int nPitch = CTexture::TextureDataSize(mh[i].m_USize, 1, 1, 1, m_eTFDst);
						assert(mh[i].m_SideSize % nPitch == 0);
						if (bToDevice)
							memcpy((byte *)d3dlr.pBits, &mp->DataArray[0], mh[i].m_SideSize);
						else
							memcpy(&mp->DataArray[0], (byte *)d3dlr.pBits, mh[i].m_SideSize);
					}
					else
						assert(0);
				}
				// Unlock the video texture
				h = pID3DTexture->UnlockRect(nLod);
				assert(h == S_OK);

				if(s_bStreamDontKeepSystem && bToDevice)
					mp->Free();
			}
#if !defined(XENON)
			assert(m_nMinMipVidUploaded < pID3DTexture->GetLevelCount());
			int nOldMip = pID3DTexture->SetLOD(m_nMinMipVidUploaded);
			assert(nOldMinMipVidUploaded == nOldMip || (nOldMinMipVidUploaded == MAX_MIP_LEVELS && nOldMip == 0));
#endif
		}
#ifdef DO_RENDERLOG
		if (gRenDev->m_LogFileStr)
			gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Uploading mips '%s'. (Side: %d, %d-%d[%d]), Size: %d, Time: %.3f\n", m_SrcName.c_str(), -1, nStartMip, nEndMip, m_nMips, SizeToLoad, iTimer->GetAsyncCurTime());
#endif
	}
	else
	{
		CDeviceTexture *pDevTexture = NULL;
		if (bToDevice && m_pNewPoolItem)
		{
			pDevTexture = m_pNewPoolItem->m_pDevTexture;
			assert(pDevTexture);
		}
		else
			pDevTexture = m_pDevTexture;
		if (!pDevTexture && bToDevice)
		{




			if( FAILED(h = pDevMan->CreateCubeTexture(m_nWidth, m_nMips, STREAMED_TEXTURE_USAGE, d3dFmt, TEXPOOL, &pDevTexture, NULL)))
			{
				assert(false);
				return;
			}
			m_pDevTexture = pDevTexture;

		}
    D3DCubeTexture *pID3DCubeTexture = pDevTexture->GetCubeTexture();

		int nMipOffset = m_nMips - pID3DCubeTexture->GetLevelCount();

		if (!m_pPoolItem)
		{
#if !defined(XENON)
			m_nMinMipVidUploaded = nStartMip;
			pID3DCubeTexture->SetLOD(nStartMip);
#endif
		}

#	if defined (_DEBUG)
		D3DSURFACE_DESC mainLevelDesc;
		pID3DCubeTexture->GetLevelDesc(0, &mainLevelDesc);
#	endif

		for (int n=0; n<6; n++)
		{
			int SizeToLoad = 0;
			if (m_pPoolItem)
			{
				for (int i=nStartMip; i<=nEndMip; i++)
				{
					int nLod = i-nStartMip;
					SMipData *mp = &mh[i].m_Mips[n];

					if (bToDevice)
						CryInterlockedAdd(&CTexture::s_nTexturesDataBytesUploaded, mh[i].m_SideSize);
#if !defined(XENON)
					D3DSurface* pDestSurf;
					D3DSurface* pSrcSurf;
					h = pID3DCubeTexture->GetCubeMapSurface((D3DCUBEMAP_FACES)n, nLod, &pDestSurf);
					h = m_pPoolItem->m_pOwner->m_pSysTexture->GetCubeTexture()->GetCubeMapSurface((D3DCUBEMAP_FACES)n, nLod, &pSrcSurf);
					h = pSrcSurf->LockRect(&d3dlr, NULL, 0);


#endif
#if defined (_DEBUG) && !defined(XENON)
					int nD3DSize;
					if (IsDXTCompressed(GetDstFormat()))
						nD3DSize = d3dlr.Pitch * (mh[i].m_VSize + 3) / 4;
					else
						nD3DSize = d3dlr.Pitch * mh[i].m_VSize;
					assert(nD3DSize == mh[i].m_SideSize);
#endif
					SizeToLoad += mh[i].m_SideSize;
#ifndef XENON
					// Copy data to system texture 
					if (bToDevice)
						memcpy((byte *)d3dlr.pBits, &mp->DataArray[0], mh[i].m_SideSize);
					else
						memcpy(&mp->DataArray[0], (byte *)d3dlr.pBits, mh[i].m_SideSize);

















































#endif
					// Unlock the system texture
#if !defined(XENON)
					pSrcSurf->UnlockRect();
					h = gcpRendD3D->m_pd3dDevice->UpdateSurface(pSrcSurf, 0, pDestSurf, 0);
					SAFE_RELEASE(pDestSurf);
					SAFE_RELEASE(pSrcSurf);


#endif
				}
			}
			else
			{
				for (int i=nStartMip; i<=nEndMip; i++)
				{
					SMipData *mp = &mh[i].m_Mips[n];
					if (!mp->DataArray.GetMemoryUsage())
						continue;
					if (bToDevice)
					{
#if !defined(XENON)
						CryInterlockedAdd(&m_nDeviceSize, mh[i].m_SideSize);
						CryInterlockedAdd(CTexture::s_nStatsCurManagedStreamedTexMem.Addr(), mh[i].m_SideSize);
						CTexture::StreamValidateTexSize();
#endif
						CryInterlockedAdd(&CTexture::s_nTexturesDataBytesUploaded, mh[i].m_SideSize);
					}
					h = pID3DCubeTexture->LockRect((D3DCUBEMAP_FACES)n, i, &d3dlr, NULL, 0);
#ifdef _DEBUG
					if (mh[i].m_USize >= 4)
					{
						int nD3DSize;
						if (IsDXTCompressed(GetDstFormat()))
							nD3DSize = d3dlr.Pitch * (mh[i].m_VSize + 3) / 4;
						else
							nD3DSize = d3dlr.Pitch * mh[i].m_VSize;
						assert(nD3DSize == mh[i].m_SideSize);
					}
#endif

					bool bLoaded = false;
					SizeToLoad += mh[i].m_SideSize;
					if (mh[i].m_USize < 4 && !IsDXTCompressed(m_eTFDst))
					{
						int nD3DSize = d3dlr.Pitch * mh[i].m_VSize;
						if (nD3DSize != m_pFileTexMips[i].m_SideSize)
						{
							bLoaded = true;
							byte *pDst = (byte *)d3dlr.pBits;
							byte *pSrc = &mp->DataArray[0];
							int nPitchSrc = CTexture::TextureDataSize(mh[i].m_USize, 1, 1, 1, m_eTFDst);
							if (bToDevice)
							{
								for (int j=0; j<mh[i].m_VSize; j++)
								{
									memcpy(pDst, pSrc, nPitchSrc);
									pSrc += nPitchSrc;
									pDst += d3dlr.Pitch;
								}
							}
							else
							{
								for (int j=0; j<mh[i].m_VSize; j++)
								{
									memcpy(pSrc, pDst, nPitchSrc);
									pSrc += nPitchSrc;
									pDst += d3dlr.Pitch;
								}
							}
						}
					}

					if (!bLoaded)
					{
						// Copy data to video texture 
						if (bToDevice)
							memcpy((byte *)d3dlr.pBits, &mp->DataArray[0], mh[i].m_SideSize);
						else
							memcpy(&mp->DataArray[0], (byte *)d3dlr.pBits, mh[i].m_SideSize);
					}

					// Unlock the video texture
					pID3DCubeTexture->UnlockRect((D3DCUBEMAP_FACES)n, i);
				}
			}
#ifdef DO_RENDERLOG
			if (gRenDev->m_LogFileStr)
				gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Uploading mips '%s'. (Side: %d, %d-%d[%d]), Size: %d, Time: %.3f\n", m_SrcName.c_str(), n, nStartMip, nEndMip, m_nMips, SizeToLoad, iTimer->GetAsyncCurTime());
#endif
		}
	}
}

#elif defined(DIRECT3D10) || defined(PS3)
void CTexture::StreamCopyMips(int nStartMip, int nEndMip, bool bToDevice)
{
	PROFILE_FRAME(Texture_StreamUpload);

#ifdef _DEBUG
	int nOldMinMipVidUploaded = m_nMinMipVidUploaded;
#endif

	CD3D9Renderer *r = gcpRendD3D;
	CDeviceManager *pDevMan = &r->m_DevMan;
	HRESULT h = S_OK;
	STexCacheMipHeader *mh = m_pFileTexMips;

	D3DFormat fmt = DeviceFormatFromTexFormat(GetDstFormat());
	if (m_bIsSRGB && gRenDev->IsLinearSpaceShadingEnabled())
		fmt = ConvertToSRGBFmt(fmt);

	LPDEVICETEXTURE* ppDevTexture = &m_pDevTexture;
	uint32 nTexMips = m_nMips;
	if(m_pPoolItem)
	{
		assert(m_pPoolItem->m_pDevTexture);
		assert(*ppDevTexture == m_pPoolItem->m_pDevTexture);
		nTexMips = m_pPoolItem->m_pOwner->m_nMips;
	}
	if (bToDevice && m_pNewPoolItem)
	{
		assert(m_pNewPoolItem->m_pDevTexture);
		ppDevTexture = &m_pNewPoolItem->m_pDevTexture;
		nTexMips = m_pNewPoolItem->m_pOwner->m_nMips;
	}

#ifndef PS3
	if (!(*ppDevTexture))
	{
		if(m_eTT!=eTT_Cube)
			h = pDevMan->Create2DTexture(m_nWidth, m_nHeight, m_nMips, STREAMED_TEXTURE_USAGE, fmt, (D3DPOOL)0, ppDevTexture);
		else
			h = pDevMan->CreateCubeTexture(m_nWidth, m_nMips, STREAMED_TEXTURE_USAGE, fmt, (D3DPOOL)0, ppDevTexture);
		assert(h == S_OK);
	}


#endif

	if (CRenderer::CV_r_texturesstreamingnoupload && bToDevice)
		return;

#ifndef PS3
	if (!m_pPoolItem)
		StreamSetLod(nStartMip, false);



#endif

	const int nMipOffset = m_nMips - nTexMips;
	const int32 nSides = m_eTT == eTT_Cube ? 6 : 1;

	D3DBaseTexture *pID3DTexture = (*ppDevTexture)->GetBaseTexture();

	int SizeToLoad = 0;
	for(int32 iSide = 0;iSide < nSides;++iSide)
	{
		const int32 iSideLockIndex = m_eTT != eTT_Cube ? -1 : iSide;
		for (int nLod=nStartMip; nLod<=nEndMip; nLod++)
		{
			SMipData *mp = &mh[nLod].m_Mips[iSide];

			if(bToDevice && mp->DataArray.empty() && s_bStreamDontKeepSystem)	// we have this mip already loaded
				continue;

			const int nDevTexMip = nLod - nMipOffset;

			if (bToDevice)
			{
				if(!mp->DataArray.empty())
				{
	#if !defined(PS3)
					CryInterlockedAdd(&m_nDeviceSize, mh[nLod].m_SideSize);
					CryInterlockedAdd(CTexture::s_nStatsCurManagedStreamedTexMem.Addr(), mh[nLod].m_SideSize);
					CTexture::StreamValidateTexSize();
	#endif
					CryInterlockedAdd(&CTexture::s_nTexturesDataBytesUploaded, mh[nLod].m_SideSize);
					const int nRowPitch = CTexture::TextureDataSize(mh[nLod].m_USize, 1, 1, 1, m_eTFDst);
					STALL_PROFILER("update texture");















					{
						gcpRendD3D->m_pd3dDeviceContext->UpdateSubresource(pID3DTexture, D3D11CalcSubresource(nDevTexMip, iSide, nTexMips), NULL, &mp->DataArray[0], nRowPitch, 0);
					}
				}
				else
					assert(0);
			}
			else
			{
				const int nMipSize = mh[nLod].m_SideSize;
				mp->Init(nMipSize, mh[nLod].m_USize, mh[nLod].m_VSize);
				const int nRowPitch = CTexture::TextureDataSize(mh[nLod].m_USize, 1, 1, 1, m_eTFDst);
				const int nRows = nMipSize / nRowPitch;
				assert(nMipSize%nRowPitch == 0);
				STexLock rect;
				STALL_PROFILER("update texture");
				HRESULT hr = (*ppDevTexture)->LockRect(iSideLockIndex, nDevTexMip, rect, LF_READ);
				assert(SUCCEEDED(hr));
				for(int iRow = 0;iRow < nRows;++iRow)
					memcpy(&mp->DataArray[iRow * nRowPitch], (byte*)rect.pData + rect.Pitch * iRow, nRowPitch);
				hr = (*ppDevTexture)->UnlockRect(iSideLockIndex, nDevTexMip);
				assert(SUCCEEDED(hr));
				// mark as native
				mp->m_bNative = true;
			}
			SizeToLoad += m_pFileTexMips[nLod].m_SideSize;

			if(s_bStreamDontKeepSystem && bToDevice)
				mp->Free();
		}
	}
#ifdef DO_RENDERLOG
	if (gRenDev->m_LogFileStr)
		gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Uploading mips '%s'. (%d[%d]), Size: %d, Time: %.3f\n", m_SrcName.c_str(), nStartMip, m_nMips, SizeToLoad, iTimer->GetAsyncCurTime());
#endif
}

#else
COMPILE_TIME_ASSERT(0);
#endif

// Just remove item from the texture object and keep Item in Pool list for future use
// This function doesn't release API texture
void CTexture::StreamRemoveFromPool()
{
	if (!m_pPoolItem)
		return;

	CHK_RENDTH;

#if defined(DIRECT3D10)
	SAFE_RELEASE((ID3D11ShaderResourceView*&)m_pDeviceShaderResource);
#endif

	STexPoolItem *pIT = m_pPoolItem;
	assert(!pIT->m_NextFree);
	m_pPoolItem = NULL;
	pIT->m_pTex = NULL;

	pIT->m_fLastTimeUsed = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime;
	m_nDeviceSize = 0;
	m_pDevTexture = NULL;

	pIT->LinkFree(&CTexture::s_FreeTexPoolItems);
	m_nMinMipVidUploaded = MAX_MIP_LEVELS;
#ifdef DO_RENDERLOG
	if (CRenderer::CV_r_logTexStreaming == 2)
		gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Add to FreePool '%s', [%d x %d], Size: %d\n", m_SrcName.c_str(), pIT->m_pOwner->m_Width, pIT->m_pOwner->m_Height, pIT->m_pOwner->m_Size);
#endif
}

void CTexture::StreamAssignPoolItem(STexPoolItem *pItem, int nMinMip)
{
	//CHK_RENDTH;
	if(m_pPoolItem == pItem)
	{
		assert(m_nDeviceSize == m_pPoolItem->m_pOwner->m_Size);
		assert(m_pPoolItem->m_pTex == this);
		assert(m_pDevTexture == m_pPoolItem->m_pDevTexture);
		return;
	}

	// if we have it assigned at the first time after StreamPrepare(), we need to release old texture
	if(m_pPoolItem == NULL)
	{
		if(m_pDevTexture)
		{
#if !defined(XENON) && !defined(PS3)
			SAFE_RELEASE(m_pDevTexture);


#endif
		}
	}

	StreamRemoveFromPool();

	m_pPoolItem = pItem;
	m_pPoolItem->UnlinkFree();
	m_nDeviceSize = m_pPoolItem->m_pOwner->m_Size;
	m_pPoolItem->m_pTex = this;
#if !(defined(XENON) && defined(ENABLE_X360_TEXTURE_CAPTURE))
	m_pDevTexture = m_pPoolItem->m_pDevTexture;
#else	// we need to replace the base address without touching the texture
	if(m_pDevTexture == NULL)
	{
		m_pDevTexture = new CDeviceTexture(m_eTT != eTT_Cube ? new IDirect3DTexture9() : new IDirect3DCubeTexture9());
#if defined(ENABLE_X360_TEXTURE_CAPTURE)
		HRESULT hr = PIXReportNewTexture(m_pDevTexture->GetBaseTexture());
		assert(SUCCEEDED(hr));
#endif
	}
	D3DFormat d3dFmt = DeviceFormatFromTexFormat(GetDstFormat());
	if( m_bIsSRGB && gRenDev->IsLinearSpaceShadingEnabled() )
		d3dFmt = ConvertToSRGBFmt(d3dFmt);
	if(m_eTT != eTT_Cube)
	{
		DWORD dwBaseAddress = m_pPoolItem->m_pDevTexture->Get2DTexture()->Format.BaseAddress << GPU_TEXTURE_ADDRESS_SHIFT;
		XGSetTextureHeader(m_pPoolItem->m_pOwner->m_Width, m_pPoolItem->m_pOwner->m_Height, m_pPoolItem->m_pOwner->m_nMips, STREAMED_TEXTURE_USAGE & ~USAGE_CUSTOM, d3dFmt,
			0,		// Pool - ignored
			dwBaseAddress,		// BaseOffset
			XGHEADER_CONTIGUOUS_MIP_OFFSET,		// MipOffset
			0,		// Pitch(default)
			m_pDevTexture->Get2DTexture(), NULL, NULL);
	}
	else
	{
		DWORD dwBaseAddress = m_pPoolItem->m_pDevTexture->GetCubeTexture()->Format.BaseAddress << GPU_TEXTURE_ADDRESS_SHIFT;
		XGSetCubeTextureHeader(m_pPoolItem->m_pOwner->m_Width, m_pPoolItem->m_pOwner->m_nMips, STREAMED_TEXTURE_USAGE & ~USAGE_CUSTOM, d3dFmt,
			0,		// Pool - ignored
			dwBaseAddress,		// BaseOffset
			XGHEADER_CONTIGUOUS_MIP_OFFSET,		// MipOffset
			m_pDevTexture->GetCubeTexture(), NULL, NULL);
	}
#endif

	m_nMinMipVidUploaded = nMinMip;



























}

STexPoolItem *CTexture::StreamGetPoolItem(int nStartMip, int nMips)
{
	assert (nStartMip<m_nMips);
	STexCacheMipHeader *mh = m_pFileTexMips;
	STexPool *pPool = NULL;

	assert(!m_pNewPoolItem);

	if (m_pPoolItem)
	{
		if (m_pPoolItem->m_pOwner->m_nMips == nMips && m_pPoolItem->m_pOwner->m_Width == mh[nStartMip].m_USize && m_pPoolItem->m_pOwner->m_Height == mh[nStartMip].m_VSize)
		{
			return NULL;
		}
	}

	D3DFormat d3dFmt = DeviceFormatFromTexFormat(GetDstFormat());
	if( m_bIsSRGB && gRenDev->IsLinearSpaceShadingEnabled() )
		d3dFmt = ConvertToSRGBFmt(d3dFmt);

	pPool = CTexture::StreamCreatePool(mh[nStartMip].m_USize, mh[nStartMip].m_VSize, nMips, d3dFmt, m_eTT);
	if (!pPool)
		return NULL;

	// Try to find empty item in the pool
	STexPoolItem *pIT = pPool->m_ItemsList.m_Next;
	while (pIT != &pPool->m_ItemsList)
	{
		if (pIT->m_NextFree && !pIT->IsStillUsedByGPU())
			break;
		pIT = pIT->m_Next;
	}
	if (pIT != &pPool->m_ItemsList)
	{
		pIT->UnlinkFree();
#ifdef DO_RENDERLOG
		if (CRenderer::CV_r_logTexStreaming == 2)
			gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Remove from FreePool '%s', [%d x %d], Size: %d\n", m_SrcName.c_str(), pIT->m_pOwner->m_Width, pIT->m_pOwner->m_Height, pIT->m_pOwner->m_Size);
#endif
	}
	else
	{
		pIT = new STexPoolItem;
		pIT->m_pOwner = pPool;
		pIT->Link(&pPool->m_ItemsList);

		// Create API texture for the item in DEFAULT pool
		HRESULT h = S_OK;
		if(m_eTT!=eTT_Cube)
			h=gcpRendD3D->m_DevMan.Create2DTexture(mh[nStartMip].m_USize, mh[nStartMip].m_VSize, nMips, STREAMED_TEXTURE_USAGE, d3dFmt, D3DPOOL_DEFAULT, &pIT->m_pDevTexture);
		else
			h=gcpRendD3D->m_DevMan.CreateCubeTexture(mh[nStartMip].m_USize, nMips, STREAMED_TEXTURE_USAGE, d3dFmt, D3DPOOL_DEFAULT, &pIT->m_pDevTexture);
		if (FAILED(h))
    {
      delete pIT;
      return NULL;
    }
		CryInterlockedAdd(CTexture::s_nStatsCurManagedStreamedTexMem.Addr(), pPool->m_Size);

#if defined(ENABLE_X360_TEXTURE_CAPTURE)
		h = PIXSetTextureName(pIT->m_pDevTexture->GetBaseTexture(), m_SrcName.c_str());
		assert(SUCCEEDED(h));
#endif
	}
	return pIT;
}

STexPool *CTexture::StreamCreatePool(int nWidth, int nHeight, int nMips, D3DFormat eTF, ETEX_Type eTT)
{
	uint32 i;
	STexPool *pPool = NULL;

	for (i=0; i<s_TexturesPools.Num(); i++)
	{
		pPool = s_TexturesPools[i];
		if (pPool->m_nMips == nMips && pPool->m_Width == nWidth && pPool->m_Height == nHeight && pPool->m_eFormat == eTF && pPool->m_eTT == eTT)
			break;
	}
	// Create new pool
	if (i == s_TexturesPools.Num())
	{
		pPool = new STexPool;
		pPool->m_eTT = eTT;
		pPool->m_eFormat = eTF;
		pPool->m_Width = nWidth;
		pPool->m_Height = nHeight;
		pPool->m_nMips = nMips;
		pPool->m_Size = CTexture::TextureDataSize(nWidth, nHeight, 1, nMips, TexFormatFromDeviceFormat(eTF));
		if (eTT == eTT_Cube)
			pPool->m_Size *= 6;

#if (defined (DIRECT3D9) || defined(OPENGL)) && (!defined(XENON) && !defined(PS3))
		CDeviceTexture *pDevTexture = NULL;
		HRESULT hr = S_OK;
		if (eTT != eTT_Cube)
			hr = gcpRendD3D->m_DevMan.Create2DTexture(nWidth, nHeight, nMips, STREAMED_TEXTURE_USAGE, eTF, D3DPOOL_SYSTEMMEM, &pDevTexture, NULL);
		else
			hr = gcpRendD3D->m_DevMan.CreateCubeTexture(nWidth, nMips, STREAMED_TEXTURE_USAGE, eTF, D3DPOOL_SYSTEMMEM, &pDevTexture, NULL);
		if(FAILED(hr))
			return NULL;
		pPool->m_pSysTexture = pDevTexture;
#endif

		pPool->m_ItemsList.m_Next = &pPool->m_ItemsList;
		pPool->m_ItemsList.m_Prev = &pPool->m_ItemsList;
		CTexture::s_TexturesPools.AddElem(pPool);
	}

	return pPool;
}

struct StreamSortFunc
{
	bool operator()(const CTexture* p1, const CTexture* p2) const
	{
		if(p1->m_nMipFactorUpdateId != p2->m_nMipFactorUpdateId)
			return p1->m_nMipFactorUpdateId < p2->m_nMipFactorUpdateId;

		if(p1->m_nMinMipCur - p1->m_nMinMipVidUploaded != p2->m_nMinMipCur - p2->m_nMinMipVidUploaded)
			return p1->m_nMinMipCur - p1->m_nMinMipVidUploaded > p2->m_nMinMipCur - p2->m_nMinMipVidUploaded;

		if(p1->m_nAccessFrameID != p2->m_nAccessFrameID)
			return p1->m_nAccessFrameID < p2->m_nAccessFrameID;

		if(p1->m_fLastMipFactor != p2->m_fLastMipFactor)
			return p1->m_fLastMipFactor > p2->m_fLastMipFactor;

		if(p1->m_nStreamingSwitchFrameID != p2->m_nStreamingSwitchFrameID)
			return p1->m_nStreamingSwitchFrameID < p2->m_nStreamingSwitchFrameID;

		return p1 > p2;
	}
};

void CTexture::StreamUnloadOldTextures(int nCurTexPoolSize)
{
	CHK_RENDTH;

	const int nMaxPoolSize = CRenderer::CV_r_texturesstreampoolsize*1024*1024;
	const int nLowerPoolLimit = (int)(nMaxPoolSize*0.97f);

	const int nCurObjectUpdateId = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nObjectUpdateId;
	const int nCurFrameId = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

	// output warning
	if(nCurTexPoolSize >= nMaxPoolSize && nCurFrameId % 60 == 0)	// once per 2 seconds
		iLog->Log("WARNING: Texture pool overflow, insufficient: %dKB", CRenderer::CV_r_texturesstreampoolsize*1024 - s_nStatsCurManagedStreamedTexMem / 1024);

	// release all possible mips
	int nUnloadedItems = 0;
	for(int i = 0;i<s_DistanceSortedTextures.size();++i)
	{
		CTexture* tp = s_DistanceSortedTextures[i];
		if(tp == NULL || tp->m_bStreamingInProgress)
			continue;

		// unload textures that are older than 4 update cycles
		const int nMaxPossibleMip = tp->m_nMips - tp->m_CacheFileHeader.m_nMipsPersistent;
		if(tp->m_nMinMipVidUploaded < nMaxPossibleMip)
		{
			nCurTexPoolSize -= tp->StreamSetLod(tp->m_nMinMipVidUploaded + 1, true);
			if (CRenderer::CV_r_texturesstreamingdebug == 1)
				iLog->Log("Agressively unloading texture: %s - From mip: %i, To mip: %i", tp->m_SrcName.c_str(), tp->m_nMinMipVidUploaded, tp->m_nMinMipVidUploaded + 1);
			++nUnloadedItems;
		}
		else
			s_DistanceSortedTextures[i] = NULL;

		if(nCurTexPoolSize < nLowerPoolLimit)
			break;
	}

	s_bPoolOverflow = nCurTexPoolSize >= nLowerPoolLimit;

	// complete overflow of the texture pool
	if (nUnloadedItems == 0)
	{
#if !defined(XENON) && !defined(PS3)
		_SetVar("r_TexturesStreamPoolSize", s_nStatsCurManagedStreamedTexMem/(1024*1024)+30);
		if(nCurFrameId % 60 == 0)
			iLog->Log("WARNING: Texture pool was changed to %d Mb", CRenderer::CV_r_texturesstreampoolsize);



#endif
	}
}

void CTexture::StreamCheckTexLimits()
{
	CHK_RENDTH;

	CTexture::StreamValidateTexSize();
  if (!s_nStreamingEnabled)
    return;





  // To avoid textures thrashing we will try to predict it using some statistics and using 4-Phase textures releasing scheme
  s_TexSizeHistory[s_nTexSizeHistory & 7] = CTexture::s_nStatsCurManagedStreamedTexMem;
  s_nTexSizeHistory++;
  int nCurTexPoolSize = s_TexSizeHistory[0];
  for (int i=1; i<8; ++i)
    nCurTexPoolSize = min(s_TexSizeHistory[i], nCurTexPoolSize);

  const int nMaxPoolSize = CRenderer::CV_r_texturesstreampoolsize*1024*1024;
	const int nLowerPoolLimit = (int)(nMaxPoolSize*0.8f);
/*	if (nCurTexPoolSize < nLowerPoolLimit)
  {
    if (nCurTexPoolSize < (int)(nPoolSize*0.5f))
      s_fAdaptiveStreamDistRatio = max(0.4f, s_fAdaptiveStreamDistRatio * 0.85f);
    else
			s_fAdaptiveStreamDistRatio = max(1.0f, s_fAdaptiveStreamDistRatio * 0.85f);
  }
  else if (nCurTexPoolSize > (int)(nPoolSize*0.9f))
  {
    if (s_fAdaptiveStreamDistRatio < 4.0f)
      s_fAdaptiveStreamDistRatio = min(4.0f, s_fAdaptiveStreamDistRatio * 1.25f);
  }*/

  if (!s_nPhaseProcessingTextures)
    s_nPhaseProcessingTextures = 1;

  // Phase1: delete old freed pool items
  {
		int nMaxItemsToFree = s_bPoolOverflow ? 30 : 10;
    STexPoolItem *pIT = CTexture::s_FreeTexPoolItems.m_PrevFree;
    while (pIT != &CTexture::s_FreeTexPoolItems)
    {
      assert (!pIT->m_pTex);
      STexPoolItem *pITNext = pIT->m_PrevFree;
      if (!pIT->IsStillUsedByGPU())
      {
				nCurTexPoolSize -= pIT->m_pOwner->m_Size;
        delete pIT;
        nMaxItemsToFree--;
      }
      pIT = pITNext;
      if (nMaxItemsToFree <= 0 || nCurTexPoolSize < nLowerPoolLimit)
        break;
    }

		bool debugRemoveMips = false;

		if (CRenderer::CV_r_texturesstreamingdebug)
		{
			if (!s_StreamingTextures.empty())
				debugRemoveMips = true;
		}

		if(pIT == &CTexture::s_FreeTexPoolItems)
		{
			if ((nCurTexPoolSize > nLowerPoolLimit && s_nPhaseProcessingTextures == 1) || debugRemoveMips)
			{
				s_nPhaseProcessingTextures = 2;
			}
		}
		else
		{
			s_nProcessedTextureID1 = 0;
			s_nProcessedTextureID2 = 0;
			s_nPhaseProcessingTextures = 0;
		}
  }
	CTexture::StreamValidateTexSize();

	if((s_nPhaseProcessingTextures > 1 && s_nProcessedTextureID1 == 0 && s_nProcessedTextureID2 == 0) || s_DistanceSortedTextures.empty())
	{
		// reinit indices for phases if we fill the sorted list from scratch
		if(s_DistanceSortedTextures.empty())
		{
			s_nPhaseProcessingTextures = min(2, s_nPhaseProcessingTextures);
			s_nProcessedTextureID1 = 0;
			s_nProcessedTextureID2 = 0;
		}
		// sort all textures by distance
		CTexture *tp = CTexture::s_Root.m_Prev;
		s_DistanceSortedTextures.SetUse(0);	// clear the array without deallocations
		while (tp != &CTexture::s_Root)
		{
			assert(tp->IsStreamed());
			const int nLowestLoadedMip = tp->m_nMips - tp->m_CacheFileHeader.m_nMipsPersistent;
			if(tp->m_nMinMipCur < nLowestLoadedMip)
				s_DistanceSortedTextures.push_back(tp);
			tp = tp->m_Prev;
		}
		if(!s_DistanceSortedTextures.empty())
			std::sort(&s_DistanceSortedTextures[0], &s_DistanceSortedTextures[0]+ s_DistanceSortedTextures.size(), StreamSortFunc());
	}

	const int nCurObjectUpdateId = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nObjectUpdateId;
	const int nCurFrameUpdateId = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

	// Phase2: unload old textures' mips (which was used at least 2 secs ago)
	if(s_nPhaseProcessingTextures == 2 || CRenderer::CV_r_texturesstreamingdebug)
	{
		int nMaxItemsToFree = s_bPoolOverflow ? 30 : 10;
		int nFrame = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
		int nWeight = 0;
		int i=s_nProcessedTextureID1;
		int nPoolMinLimit = nLowerPoolLimit;

		// unload all textures aggressively in debug mode
		if (CRenderer::CV_r_texturesstreamingdebug)
		{
			nMaxItemsToFree = s_DistanceSortedTextures.size();
			nPoolMinLimit = 0;
		}

		for(;i<s_DistanceSortedTextures.size();++i)
		{
			CTexture* tp = s_DistanceSortedTextures[i];
			if(tp == NULL)
				continue;

			if (nCurObjectUpdateId - tp->m_nMipFactorUpdateId > 3 && tp->m_fLastMipFactor < 10e5f)	// unload old textures
			{
				tp->m_fLastMipFactor = 10e5f;
				const int nLowestMip = tp->m_nMips - tp->m_CacheFileHeader.m_nMipsPersistent;
				if(tp->m_nMinMipVidUploaded < nLowestMip)
				{
					nCurTexPoolSize -= tp->StreamSetLod(nLowestMip, true);
					if (CRenderer::CV_r_texturesstreamingdebug == 1)
						iLog->Log("Unloading unused texture: %s - From mip: %i, To mip: %i", tp->m_SrcName.c_str(), tp->m_nMinMipVidUploaded, nLowestMip);
					s_DistanceSortedTextures[i] = NULL;		// mark it as processed
					nMaxItemsToFree--;
				}
			}

			// lazy unloading
			if(nMaxItemsToFree <= 0 || nCurTexPoolSize < nPoolMinLimit)
				break;
		}
		if (i == s_DistanceSortedTextures.size())
		{
			s_nProcessedTextureID1 = 0;
			s_nPhaseProcessingTextures = 3;
		}
		else
		{
			if(nMaxItemsToFree == 0)
			{
				s_nProcessedTextureID1 = i;
				s_nPhaseProcessingTextures = 2;
			}
			else
			{
				s_nProcessedTextureID1 = 0;
				s_nPhaseProcessingTextures = 0;
			}
		}
	}
	CTexture::StreamValidateTexSize();

	// Phase3: More aggressive unload old textures' mips
	if (s_nPhaseProcessingTextures == 3 || CRenderer::CV_r_texturesstreamingdebug)
	{
		int nMaxItemsToFree = s_bPoolOverflow ? 10 : 5;
		int nPoolMinLimit = nLowerPoolLimit;
		// unload all textures aggressively in debug mode
		if (CRenderer::CV_r_texturesstreamingdebug)
		{
			nMaxItemsToFree = s_DistanceSortedTextures.size();
			nPoolMinLimit = 0;
		}

		uint32 i = s_nProcessedTextureID2;
		for(;i<s_DistanceSortedTextures.size();++i)
		{
			CTexture* tp = s_DistanceSortedTextures[i];
			if(tp == NULL)
				continue;

			// unload textures that are older than 4 update cycles
			if(tp->m_nMinMipCur > tp->m_nMinMipVidUploaded)
			{
				nCurTexPoolSize -= tp->StreamSetLod(tp->m_nMinMipCur, true);
				if (CRenderer::CV_r_texturesstreamingdebug == 1)
					iLog->Log("Shrinking texture: %s - From mip: %i, To mip: %i", tp->m_SrcName.c_str(), tp->m_nMinMipVidUploaded, tp->m_nMinMipCur);
				s_DistanceSortedTextures[i] = NULL;		// mark it as processed
				nMaxItemsToFree--;
			}
			if (nMaxItemsToFree <= 0 || nCurTexPoolSize <= nPoolMinLimit)
				break;
		}
		if (i != s_DistanceSortedTextures.size() && nMaxItemsToFree == 0)
		{
			s_nProcessedTextureID2 = i;
			s_nPhaseProcessingTextures = 3;
		}
		else
		{
			s_nPhaseProcessingTextures = 0;
			s_nProcessedTextureID2 = 0;
		}
	}

  if (!(CTexture::s_nStreamingEnabled & 1))
    return;
  if (CRenderer::CV_r_texturesstreampoolsize < 10)
    CRenderer::CV_r_texturesstreampoolsize = 10;

  CTexture::StreamValidateTexSize();

	// indicate pool overflow
	if(nCurTexPoolSize < (int)(nMaxPoolSize*0.9f))
		CTexture::s_bPoolOverflow = FALSE;

  // Usually we do it in case of textures thrashing
  if (nCurTexPoolSize >= (int)(nMaxPoolSize*0.97f) && s_nPhaseProcessingTextures == 0)
    StreamUnloadOldTextures(nCurTexPoolSize);

	if(CRenderer::CV_r_texturesstreamingdebug >= 2)
		OutputDebugInfo();
}

void CTexture::StreamCopyMipsFast( STexPoolItem* pSrcItem, int nMipSrc, STexPoolItem* pDestItem, int nMipDest, int nNumMips )
{
	HRESULT hr = S_OK;
	assert(pSrcItem && pDestItem);
	assert(pSrcItem->m_pOwner->m_eTT == pDestItem->m_pOwner->m_eTT);
	const int32 nSides = pDestItem->m_pOwner->m_eTT != eTT_Cube ? 1 : 6;
	for(int32 iSide = 0;iSide < nSides;++iSide)
	{
		const int32 iSideLockIndex = pDestItem->m_pOwner->m_eTT != eTT_Cube ? -1 : iSide;

		assert(nMipSrc >= 0 && nMipDest >= 0);
	#if ((defined (DIRECT3D9) || defined(OPENGL))) && !defined(XENON)
		assert(pSrcItem->m_pDevTexture->GetBaseTexture()->GetLevelCount() >= nMipSrc + nNumMips);
		assert(pDestItem->m_pDevTexture->GetBaseTexture()->GetLevelCount() >= nMipDest + nNumMips);
		D3DTexture* pSrc = pSrcItem->m_pDevTexture->Get2DTexture();
		D3DCubeTexture* pSrcCube = pSrcItem->m_pDevTexture->GetCubeTexture();
		D3DTexture* pDest = pDestItem->m_pDevTexture->Get2DTexture();
		D3DCubeTexture* pDestCube = pDestItem->m_pDevTexture->GetCubeTexture();
		assert(pSrc && pDest);
		for(int i=0;i<nNumMips;++i)
		{
			D3DSurface* pSurfSrc = NULL;
			D3DSurface* pSurfDest = NULL;
			if(pDestItem->m_pOwner->m_eTT != eTT_Cube)
			{
				pSrc->GetSurfaceLevel(nMipSrc + i, &pSurfSrc);
				pDest->GetSurfaceLevel(nMipDest + i, &pSurfDest);
			}
			else
			{
				pSrcCube->GetCubeMapSurface((D3DCUBEMAP_FACES)iSide, nMipSrc + i, &pSurfSrc);
				pDestCube->GetCubeMapSurface((D3DCUBEMAP_FACES)iSide, nMipDest + i, &pSurfDest);
			}
			hr = gcpRendD3D->m_pd3dDevice->StretchRect(pSurfSrc,NULL,pSurfDest,NULL,D3DTEXF_NONE);
			assert(SUCCEEDED(hr));

			SAFE_RELEASE(pSurfSrc);
			SAFE_RELEASE(pSurfDest);
		}
		assert(SUCCEEDED(hr));


































































	#elif defined(DIRECT3D10)
		for(int i=0;i<nNumMips;++i)
			gcpRendD3D->m_pd3dDeviceContext->CopySubresourceRegion(pDestItem->m_pDevTexture->GetBaseTexture(), 
																		D3D11CalcSubresource(nMipDest + i, iSide, pDestItem->m_pOwner->m_nMips),
																		0, 0, 0, pSrcItem->m_pDevTexture->GetBaseTexture(), 
																		D3D11CalcSubresource(nMipSrc + i, iSide, pSrcItem->m_pOwner->m_nMips), 
																		NULL);

	#endif
	}
}

// Debug routines /////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct StreamDebugSortFunc
{
	bool operator()(const CTexture* p1, const CTexture* p2) const
	{
		if(!p1)
			return true;
		if(!p2)
			return false;
		if(p1->m_nDeviceSize != p2->m_nDeviceSize)
			return p1->m_nDeviceSize < p2->m_nDeviceSize;
		return p1 < p2;
	}
};

void CTexture::OutputDebugInfo()
{
	CD3D9Renderer *r = gcpRendD3D;
	D3DDevice *dv = r->GetD3DDevice();

	int nX = 40;
	int nY = 30;

	r->WriteXY(nX,nY,.8f,.8f,1,1,0,1, "Size, kb | Distance | Mips loaded / total | Name");

	TArray<CTexture*> texSorted(&s_DistanceSortedTextures[0], s_DistanceSortedTextures.size());

	if(CRenderer::CV_r_texturesstreamingdebug == 3 && !texSorted.empty())
	{
		std::stable_sort(&texSorted[0], &texSorted[0] + texSorted.size(), StreamDebugSortFunc());
	}

	for(int i = (int)texSorted.size() - 1, nTexNum = 0;i >= 0;--i)
	{
		CTexture* tp = texSorted[i];
		if(tp == NULL)
			continue;
		ColorF color = tp->m_nDeviceSize >= 1024 * 1024 ? Col_Red : (tp->m_nDeviceSize >= 512 * 1024 ? Col_Yellow : Col_Green);
		r->WriteXY(nX,nY+(nTexNum+1)*11,.8f,.8f,color.r,color.g,color.b,1,"%.2fMb | %.2f | %d/%d | %s", (float)tp->m_nDeviceSize/1024/1024, sqrtf(tp->m_fLastMipFactor / r->GetMipDistFactor()), tp->m_nMinMipVidUploaded, tp->GetNumMipsNonVirtual(), tp->m_SrcName.c_str());
		++nTexNum;
		if(nTexNum > 50)
			break;
	}
}
