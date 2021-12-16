/*=============================================================================
  TextureStreaming.cpp : Common Texture Streaming manager implementation.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
										* Created by Honich Andrey
- 19:8:2008   12:14 : Refactored by Anton Kaplanyan

=============================================================================*/


#include "StdAfx.h"
#include "../CommonRender.h"
#include "Image/DDSImage.h"
#include "StringUtils.h"								// stristr()
#include "ILocalMemoryUsage.h"

// checks for MT-safety of called functions
#define CHK_RENDTH assert(gRenDev->m_pRT->IsRenderThread())
#define CHK_MAINTH assert(gRenDev->m_pRT->IsMainThread())
#define CHK_MAINORRENDTH assert(gRenDev->m_pRT->IsMainThread() || gRenDev->m_pRT->IsRenderThread())

#define MIP_FORCE_UPDATE (-2)

int CTexture::s_nTexSizeHistory;
int CTexture::s_TexSizeHistory[8];
int CTexture::s_nPhaseProcessingTextures;
int CTexture::s_nProcessedTextureID1;
int CTexture::s_nProcessedTextureID2;
int CTexture::s_bPoolOverflow = 0;
TArray<CTexture *> CTexture::s_StreamingRequested;
TArray<CTexture *> CTexture::s_DistanceSortedTextures;
CryMT::queue<IReadStreamPtr> CTexture::s_StreamingTextures;
//CryCriticalSection STexPool::s_csPoolLock;

bool CTexture::s_bStreamDontKeepSystem;

int CTexture::s_nTexturesDataBytesLoaded;
volatile int CTexture::s_nTexturesDataBytesUploaded;
float CTexture::s_fAdaptiveStreamDistRatio;
volatile TIntAtomic CTexture::s_nStatsCurManagedStreamedTexMem;
#ifndef PS3
	volatile int CTexture::s_nStatsCurManagedNonStreamedTexMem;
	volatile int CTexture::s_nStatsCurDynamicTexMem;
#endif
CTexture CTexture::s_Root;

void CTexture::StreamReleaseMipsData(int nStartMip, int nEndMip)
{
  STexCacheMipHeader *mh = m_pFileTexMips;
  int nSides = (m_eTT == eTT_Cube) ? 6 : 1;
  int nEnd = m_nMips - 1;
  nEndMip = min(nEndMip, nEnd);
  for (int i=0; i<nSides; i++)
    for (int j=nStartMip; j<=nEndMip; j++)
      mh[j].m_Mips[i].Free();
}

int CTexture::StreamUnload(bool bDestroy)
{
	CHK_RENDTH;
	if (m_bWasUnloaded)
    return 0;
  if (!IsStreamed())
    return 0;

  if (CTexture::s_nStreamingEnabled & 1)
  {
		if(m_bStreamingInProgress)
			AbortStreamingTasks(this);
		assert(!m_bStreamingInProgress);
		m_nStreamingSwitchFrameID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
    int nDevSize = m_nDeviceSize;
#ifdef DO_RENDERLOG
    if (gRenDev->m_LogFileStr)
      gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Unload '%s', Time: %.3f\n", m_SrcName.c_str(), iTimer->GetAsyncCurTime());
#endif
    if (bDestroy)
      ReleaseDeviceTexture(true);
    else if (m_pPoolItem)
      StreamRemoveFromPool();
    m_bWasUnloaded = true;
    return nDevSize;
  }
  return 0;
}

void CTexture::StreamRestore()
{
	CHK_RENDTH;
	StreamLoadFromCache(FPR_IMMEDIATELY, 0.f, -1);
}
























































































































































// streaming thread
void CTextureStreamCallback::StreamAsyncOnComplete (IReadStream* pStream, unsigned nError)
{
	STextureStreamingInfo *pTexStreamInfo = (STextureStreamingInfo *)pStream->GetUserData();
	STextureStreamingSharedInfo *pTexStreamSharedInfo = pTexStreamInfo->m_pSharedInfo;
	assert(pTexStreamSharedInfo->m_nAsyncRefCount > 0);
	CryInterlockedDecrement(&pTexStreamSharedInfo->m_nAsyncRefCount);

	if(nError)
	{
		if(nError != ERROR_USER_ABORT)
			gEnv->pLog->LogError("Error streaming texture %s", pStream->GetName().c_str());
		return;
	}

	CTexture *tp = pTexStreamSharedInfo->m_pTexture; 
	assert(tp); 
	assert(pTexStreamSharedInfo->m_nHigherUploadedMip < tp->m_nMinMipVidUploaded);   

	// download data
	byte *Src = (byte *)pStream->GetBuffer();
	STexCacheFileHeader *fh = &tp->m_CacheFileHeader;
	STexCacheMipHeader *mh = tp->m_pFileTexMips;
	const int i = pTexStreamInfo->m_nRelativeMipLevel + pTexStreamSharedInfo->m_nHigherUploadedMip;

	const int32 nSides = tp->GetTexType() != eTT_Cube ? 1 : 6;
	for(int32 iSide = 0;iSide < nSides;++iSide)
	{
		SMipData *mp = &mh[i].m_Mips[iSide];
		const int SrcSideSize = CTexture::TextureDataSize(mh[i].m_USize, mh[i].m_VSize, 1, 1, tp->GetSrcFormat());
		assert(SrcSideSize * nSides == pStream->GetParams().nSize);
		//if (!mp->DataArray.GetMemoryUsage()) // Already uploaded synchronously
		{
			CTimeValue time0 = iTimer->GetAsyncTime();
			mp->Init(mh[i].m_SideSize, mh[i].m_USize, mh[i].m_VSize);
			CTexture::ExpandMipFromFile(&mp->DataArray[0], mh[i].m_SideSize, Src, SrcSideSize, tp->GetSrcFormat());
			gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_fTexUploadTime += (iTimer->GetAsyncTime() - time0).GetSeconds();
		}

		Src += SrcSideSize;
	}

	// collect statistics
	if(pStream->GetParams().nSize >= 1024)
		CTexture::s_nStreamingThroughput += pStream->GetParams().nSize;
}

// main thread
void CTextureStreamCallback::StreamOnComplete (IReadStream* pStream, unsigned nError)
{
	STextureStreamingInfo *pTexStreamInfo = (STextureStreamingInfo *)pStream->GetUserData();
	STextureStreamingSharedInfo *pTexStreamSharedInfo = pTexStreamInfo->m_pSharedInfo;

	CTexture *tp = pTexStreamSharedInfo->m_pTexture;

	// check if we have loaded all chunks
	if((--pTexStreamSharedInfo->m_nSyncRefCount) > 0)
	{
		SAFE_DELETE(pTexStreamInfo);
		CTexture::StreamRemoveTask(tp, IReadStreamPtr(pStream));
		return;
	}

  if(nError)
  {
		if(nError != ERROR_USER_ABORT)
			gEnv->pLog->LogError("Error streaming texture %s", pStream->GetName().c_str());
		SAFE_DELETE(pTexStreamInfo);
		SAFE_DELETE(pTexStreamSharedInfo);
		return;
  }

	assert(pTexStreamSharedInfo->m_nSyncRefCount == 0);
	assert(pTexStreamSharedInfo->m_nAsyncRefCount == 0);

  pStream->AddRef();

  gRenDev->m_pRT->RC_TexStreamComplete(pStream);
}

// render thread
void CTexture::StreamOnComplete (IReadStream* pStream)
{
	CHK_RENDTH;
	PROFILE_FRAME(Texture_StreamComplete);

	assert(pStream);
	if (!pStream)
		return;
	STextureStreamingInfo *pTexStreamInfo = (STextureStreamingInfo *)pStream->GetUserData();
	assert(pTexStreamInfo);
	if (!pTexStreamInfo)
	{
		pStream->Release();
		return;
	}
	STextureStreamingSharedInfo *pTexStreamSharedInfo = pTexStreamInfo->m_pSharedInfo;
	assert(pTexStreamSharedInfo && pTexStreamSharedInfo->m_nSyncRefCount == 0);

  CTexture *tp = pTexStreamSharedInfo->m_pTexture;
  if (!tp)
  {
    gEnv->pLog->LogWarning("CTextureStreamCallback::StreamOnComplete texture is missing %s", pStream->GetName().c_str());
		SAFE_DELETE(pTexStreamInfo);
		SAFE_DELETE(pTexStreamSharedInfo);
		if(pStream->GetError() != ERROR_USER_ABORT)
			CTexture::StreamRemoveTask(NULL, IReadStreamPtr(pStream));
		pStream->Release();
		tp->SetStreamingInProgress(false);
		return;
  }

#ifdef DO_RENDERLOG
  if (gRenDev->m_LogFileStr)
    gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], 
											"Async Finish Load '%s' (Mips: %d-%d[%d]), Size: %d, Time: %.3f\n", 
											tp->GetSourceName(), pTexStreamSharedInfo->m_nHigherUploadedMip, pTexStreamSharedInfo->m_nHigherUploadedMip + pTexStreamInfo->m_nRelativeMipLevel, 
											tp->m_nMips, pStream->GetParams().nSize, iTimer->GetAsyncCurTime());
#endif

	CTexture::StreamValidateTexSize();

	assert(tp->m_nMinMipVidUploaded - 1 == pTexStreamSharedInfo->m_nLowerUploadedMip);
  tp->StreamUploadMips(pTexStreamSharedInfo->m_nHigherUploadedMip, tp->m_nMinMipVidUploaded - 1);
  if (!pTexStreamSharedInfo->m_nHigherUploadedMip || s_bStreamDontKeepSystem)
    tp->StreamReleaseMipsData(0, tp->m_nMips - 1);
  tp->SetWasUnload(false);
  tp->Relink(&CTexture::s_Root);

	// collect statistics
	const CTimeValue currentTime = iTimer->GetAsyncTime();
	if(currentTime - pTexStreamSharedInfo->m_fStartTime >  .01f)	// avoid measurement errors for small textures
		CTexture::s_nStreamingTotalTime += (currentTime - pTexStreamSharedInfo->m_fStartTime).GetSeconds();

	if(pStream->GetError() != ERROR_USER_ABORT)
		CTexture::StreamRemoveTask(tp, IReadStreamPtr(pStream));

	tp->m_fCurrentMipBias += float(pTexStreamSharedInfo->m_nLowerUploadedMip - pTexStreamSharedInfo->m_nHigherUploadedMip + 1);

	SAFE_DELETE(pTexStreamInfo);
	SAFE_DELETE(pTexStreamSharedInfo);
  pStream->Release();

  CTexture::StreamValidateTexSize();
}

void CTexture::StreamLoadSynchronously(int nStartMip, int nEndMip)
{
	CHK_RENDTH;
	int iMip, iSide;
  int nMips, nSides;
  int nOffsSide = 0;
  STexCacheFileHeader *fh = &m_CacheFileHeader;
  nSides = fh->m_nSides;
  nMips = m_nMips;

  if (nStartMip < 0)
		return;

	// we don't need to load mips that we already loaded
	nEndMip = min(m_nMinMipVidUploaded-1, nEndMip);
	if(nStartMip > nEndMip)
		return;

	// check if it fits to pool
	if(IsStreamed() && m_bStreamPrepared)
	{
		const int nSizeToLoad = (m_pFileTexMips[nStartMip].m_SideSizeWithMips - m_pFileTexMips[nEndMip].m_SideSizeWithMips) * nSides / 1024;
		const int nCurPoolSize = CTexture::s_nStatsCurManagedStreamedTexMem / 1024;
    const int nMaxPoolSize = CRenderer::CV_r_texturesstreampoolsize*1024;
		if(nCurPoolSize + nSizeToLoad >= nMaxPoolSize)
		{
			// trigger pool overflow
			s_bPoolOverflow = 1;
			return;
		}
	}

  PROFILE_FRAME(Texture_StreamSync);

	SetStreamingInProgress(true);

	const int32 nSideSize = m_nSize/nSides;

  assert (nSideSize == m_pFileTexMips[0].m_SideSizeWithMips);

  int SizeToLoad = 0;
  for (iMip=nStartMip; iMip<=nEndMip;++iMip)
  {
		STexCacheMipHeader& mh = m_pFileTexMips[iMip];

		size_t sizeToRead;
		if(IsStreamFromDDS())
			sizeToRead = CTexture::TextureDataSize(mh.m_USize, mh.m_VSize, max(1, m_nDepth>>iMip), 1, GetSrcFormat());
		else
			sizeToRead = mh.m_SideSize;
		if((m_nFlags & FT_REPLICATE_TO_ALL_SIDES) == 0)
			sizeToRead *= nSides;

		byte* pSides = NULL;
		std::vector<byte> vecSides;
		if(nSides > 1 && !(m_nFlags & FT_REPLICATE_TO_ALL_SIDES))
		{
			vecSides.resize(mh.m_SideSize * nSides);
			pSides = &vecSides[0];
		}

		for (iSide=0; iSide<nSides;++iSide)
		{
			SMipData *mp = &mh.m_Mips[iSide];
			if (mp->DataArray.GetMemoryUsage())
        continue;
			mp->Init(mh.m_SideSize, mh.m_USize, mh.m_VSize);
			if(!pSides)
				pSides = &mp->DataArray[0];
      CTexture::s_nTexturesDataBytesLoaded += mh.m_SideSize;
      SizeToLoad += mh.m_SideSize;

			// load chunk from file (all sides at once)
			if (iSide == 0)
			{
				uint32 nImageFlags = 0;
				if(m_nFlags & FT_ALPHA)			nImageFlags |= FIM_ALPHA;
				if(m_nFlags & FT_SPLITTED)	nImageFlags |= FIM_SPLITTED;
				size_t readSize = DDSSplitted::LoadMips( m_SrcName, pSides, m_pFileTexMips[iMip].m_Mips[0].m_Seek, sizeToRead, 
																									m_nWidth, m_nHeight, m_nDepth, nSides,
																									m_nMips, iMip, iMip, m_CacheFileHeader.m_nMipsPersistent, GetSrcFormat(), nImageFlags);
				assert(readSize == sizeToRead);
			}

			if(iSide == 0 || !(m_nFlags & FT_REPLICATE_TO_ALL_SIDES))
			{
				size_t nSrcDataSideSize = CTexture::TextureDataSize(mh.m_USize, mh.m_VSize, max(1, m_nDepth>>iMip), 1, m_eTFSrc);
				CTexture::ExpandMipFromFile(&mp->DataArray[0], mh.m_SideSize, pSides + iSide * nSrcDataSideSize, nSrcDataSideSize, m_eTFSrc);
			}

      if (GetSrcFormat() == eTF_R8G8B8 && IsStreamFromDDS())
      {
        for (int iTexel=mh.m_USize*mh.m_VSize-1; iTexel >= 0; --iTexel)
        {
#ifndef XENON
					ColorB texel(mp->DataArray[iTexel*3+0], mp->DataArray[iTexel*3+1], mp->DataArray[iTexel*3+2], 255);


#endif
					(ColorB&)mp->DataArray[iTexel*4] = texel;
        }
      }
      else if(iSide > 0)
      {
        if (m_nFlags & FT_REPLICATE_TO_ALL_SIDES)
          memcpy(&mp->DataArray[0], &mh.m_Mips[0].DataArray[0], mp->DataArray.GetMemoryUsage());
      }
    }
#ifdef DO_RENDERLOG
    if (gRenDev->m_LogFileStr && SizeToLoad > 0)
      gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Sync Load '%s' (Side: %d, %d-%d[%d]), Size: %d, Time: %.3f\n", m_SrcName.c_str(), iSide, nStartMip, iMip-1, nMips, SizeToLoad, iTimer->GetAsyncCurTime());
#endif
    nOffsSide += nSideSize;
  }
  { 
    PROFILE_FRAME(Texture_LoadFromCache_UploadSync);

















		StreamUploadMips(nStartMip, nMips-1);

		if(s_bStreamDontKeepSystem)
			StreamReleaseMipsData(0, m_nMips - 1);
  }
	SetStreamingInProgress(false);
	m_fCurrentMipBias = 0.f;
	m_bWasUnloaded = false;
}

int CTexture::StreamCalculateMips(float fMipFactor)
{
	assert(IsStreamed());
	float currentMipFactor = fMipFactor * max(m_nWidth, m_nHeight);
	int nMip = int(ceilf(logf(max(currentMipFactor, 1.0f)) / LN2));
  nMip = min(m_nMips-1, nMip);
	const int nNewMip = min(nMip, max(0, m_nMips - m_CacheFileHeader.m_nMipsPersistent));

	return nNewMip;
}

int CTexture::GetStreamableMipNumber()
{
	assert(IsStreamed());
	return max(0, m_nMips - m_CacheFileHeader.m_nMipsPersistent);
}

int CTexture::GetStreamableMemoryUsage(int nStartMip)
{
	assert(IsStreamed());
	if (m_pFileTexMips == NULL)
	{
		assert(0);
		return 0;
	}

	return m_pFileTexMips[nStartMip].m_SideSizeWithMips;
}

void CTexture::StreamUpdateMip(float fMipFactor, int nUpdateId)
{
	// check if we have persistent mips in memory
	m_nMinMipCur = min(m_nMinMipCur, max(0, m_nMips - m_CacheFileHeader.m_nMipsPersistent));

	// wrong id? => dummy render phase(shadows, sprites, etc.)
	if(nUpdateId == -1)
		return;

  if (fMipFactor < 0)
  {
    if (m_nMipFactorUpdateId != nUpdateId)
      fMipFactor = 0;
    else
      fMipFactor = m_fMinMipFactor;
  }

  if (m_nMipFactorUpdateId != nUpdateId)
  {
		if(nUpdateId == MIP_FORCE_UPDATE)
		{
			m_fMinMipFactor = fMipFactor;																			//to avoid unnecessary mip-map loads
		}

		// calculate the new lod value
		float currentMipFactor = m_fMinMipFactor * m_nWidth * m_nHeight * gRenDev->GetMipDistFactor();
		int nMip = fastround_positive(0.5f * logf(max(currentMipFactor, 1.0f)) / LN2) + CRenderer::CV_r_TexturesStreamingMipBias;
	  nMip = min(m_nMips-1, nMip);
		const int nNewMip = min(nMip, max(0, m_nMips - m_CacheFileHeader.m_nMipsPersistent));

		if (CRenderer::CV_r_texturesstreamingdebug == 1)
			iLog->Log("Updating mips: %s - Current: %i, Previous: %i", m_SrcName.c_str(), m_nMinMipCur, nNewMip);

		m_nMinMipCur = nNewMip;

		m_fLastMipFactor = m_fMinMipFactor;

		// reset distance
		if(nUpdateId >= 0)
		{			
			m_nMipFactorUpdateId = max(m_nMipFactorUpdateId, max(gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nObjectUpdateId, nUpdateId));
			m_fMinMipFactor = fMipFactor;
		}
		else
		{
			assert(nUpdateId == MIP_FORCE_UPDATE);
			m_nMipFactorUpdateId = max(m_nMipFactorUpdateId, gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nObjectUpdateId);
		}
  }

	m_fMinMipFactor = min(m_fMinMipFactor, fMipFactor);
}

const bool CTexture::IsParticularMipStreamed(float fMipFactor) const
{
	if(!IsStreamed())
		return true;

	float currentMipFactor = fMipFactor * m_nWidth * m_nHeight;
	int nMip = fastround_positive(0.5f * logf(max(currentMipFactor, 1.0f)) / LN2);
	nMip = min(m_nMips - m_CacheFileHeader.m_nMipsPersistent, max(0, nMip));
	return m_nMinMipVidUploaded <= nMip;
}

void CTexture::PrecacheAsynchronously( float fMipFactor, int nFlags, int nUpdateId )
{
	if(!IsStreamed())
		return;		// already done

	// for distance streaming it's just the same as update rendering distance
	if(!CRenderer::CV_r_texturesstreamingsync && !(nFlags & FPR_SYNCRONOUS))
		{
			StreamUpdateMip(fMipFactor, nUpdateId);

			if ((nFlags & FPR_STARTLOADING) && !m_bStreamingInProgress && !m_bStreamRequested)
			{
				StreamLoadFromCache(FPR_IMMEDIATELY, fMipFactor, nUpdateId);
			}
		}
	else
		StreamLoadFromCache(FPR_IMMEDIATELY | FPR_SYNCRONOUS | nFlags, fMipFactor, MIP_FORCE_UPDATE);
}

void CTexture::StreamLoadFromCache(int Flags, float fMipFactor, int nUpdateId)
{
	if (!(CTexture::s_nStreamingEnabled & 1) || !IsStreamed())
		return;

  PROFILE_FRAME(Texture_Stream);

	if (gRenDev->m_RP.m_pRE == NULL)
	{
	  StreamUpdateMip(fMipFactor, nUpdateId);
	}

	if(!m_bStreamPrepared)
	{
		if(!StreamPrepare(true, m_eTFDst))
		{
			// ignore error for optional attached alpha channel
			if(!m_bNoTexture && !(m_nFlags&FT_ALPHA) && (m_nFlags&FT_FROMIMAGE))
			{
				bool bRes = Reload();
				assert(bRes);
			}
			return;
		}
	}

	if(!m_pFileTexMips)
	{
		assert(0);
		return;
	}

  STexCacheMipHeader *mh = m_pFileTexMips;

  if (m_nMinMipVidUploaded == 0)
    return;
  CTexture::StreamValidateTexSize();

  int nSyncStartMip = -1;
  int nSyncEndMip = -1;
  int nASyncStartMip = -1;
  int nASyncEndMip = -1;
  if (m_nMips == 1 || CRenderer::CV_r_texturesstreamingsync || (Flags & FPR_SYNCRONOUS))
  {
		StreamUpdateMip(fMipFactor, MIP_FORCE_UPDATE);
		nSyncStartMip = m_nMinMipCur;
    nSyncEndMip = m_nMips - 1;
  }
  else
  {
    // Always stream lowest nMipsPersistent mips synchronously
    int nStartLowestM = max(m_nMinMipCur, m_nMips - m_CacheFileHeader.m_nMipsPersistent);
    if(nStartLowestM < m_nMinMipVidUploaded)
    {
      nSyncStartMip = nStartLowestM;
      nSyncEndMip = m_nMips - 1;
    }

    // Let's see which part of the texture not loaded yet and stream it asynchronously
    if (m_nMinMipCur < nStartLowestM && m_nMinMipCur < m_nMinMipVidUploaded)
    {
      nASyncStartMip = m_nMinMipCur;
      nASyncEndMip = max(m_nMinMipVidUploaded, nStartLowestM-1);
    }
  }
  if (nASyncStartMip < 0 && nSyncStartMip < 0)
    return;

  // Synchronous loading
  if (nSyncStartMip >= 0)
	{
		if(!m_bStreamingInProgress && nSyncStartMip < m_nMinMipVidUploaded)
			StreamLoadSynchronously(nSyncStartMip, nSyncEndMip);
		assert(m_pDevTexture);
	}

  // Asynchronous request
  if (nASyncStartMip >= 0)
  {
    assert(nASyncEndMip >= nASyncStartMip);
    assert(nASyncStartMip < m_nMinMipVidUploaded);
		if (!m_bStreamRequested || m_nAsyncStartMip > nASyncStartMip)
		{
			m_nAsyncStartMip = nASyncStartMip;
			if (!m_bStreamRequested && !m_bStreamingInProgress)
			{
				m_bStreamRequested = true;
				s_StreamingRequested.AddElem(this);
				Relink(&CTexture::s_Root);
			}
		}
  }
}

// For now we support only .dds files streaming
bool CTexture::StreamPrepare(bool bReload, ETEX_Format eTFDst)
{
	CHK_MAINORRENDTH;
	if (!(s_nStreamingEnabled & 1))
    return false;

  if (m_nFlags & FT_DONT_STREAM)
    return false;

	LOADING_TIME_PROFILE_SECTION(iSystem);

	// release the old texture
  if (m_pDevTexture)
	  ReleaseDeviceTexture(false);

	const char *szExt = fpGetExtension(m_SrcName.c_str());
	if (szExt && !stricmp(szExt, ".tif") && !gEnv->pCryPak->IsFileExist(m_SrcName.c_str()))
	{
		m_SrcName = PathUtil::ReplaceExtension(m_SrcName, "dds");
#if !defined(XENON) && !defined(PS3)		// for consoles we have *.ctx textures
		if (!gEnv->pCryPak->IsFileExist(m_SrcName))
			return false;
#endif
	}

	if ((m_nFlags & FT_TEX_NORMAL_MAP) && !CryStringUtils::stristr(m_SrcName.c_str(), "_ddn"))
		iLog->Log("Warning: The normal map is without '_ddn' suffix (%s) (texture name should contain _ddn suffix)", m_SrcName.c_str());

	CImageFile *pIM = CImageFile::mfLoad_file(m_SrcName, false, ((m_nFlags & FT_ALPHA) ? FIM_ALPHA : 0) | FIM_STREAM_PREPARE);

  int nSides, nNumMips;
	{
    STexCacheFileHeader fh;
    // Can't stream volume textures yet
    if (pIM && pIM->mfGetFormat() != eTF_Unknown && pIM->mfGet_depth() <= 1 && pIM->mfGet_numMips() > 1)
    {
			if(pIM->mfGet_Flags() & FIM_SPLITTED)
				m_nFlags |= FT_SPLITTED;
			if(pIM->mfGet_Flags() & FIM_X360_NOT_PRETILED)
				m_nFlags |= FT_TEX_WAS_NOT_PRE_TILED;
			if (pIM->mfGet_Flags() & FIM_ALPHA)
				m_nFlags |= FT_HAS_ATTACHED_ALPHA;			// if the image has alpha attached we store this in the CTexture
			// we don't support 3D textures
			if (pIM->mfGet_depth()>1)
			{
				SAFE_DELETE(pIM);
				return false;
			}

      if (m_nFlags & FT_ALPHA)
      {
        if (!pIM->mfIs_image(0))
        {
          assert(!m_pDevTexture);
          m_pDevTexture = s_ptexWhite->m_pDevTexture;
#if defined (DIRECT3D10)
          m_pDeviceShaderResource = s_ptexWhite->m_pDeviceShaderResource;
#endif
          m_nDefState = s_ptexWhite->m_nDefState;
          m_nFlags = s_ptexWhite->GetFlags();
          m_eTFDst = s_ptexWhite->GetDstFormat();
          m_nMips = s_ptexWhite->GetNumMips();
          m_nWidth = s_ptexWhite->GetWidth();
          m_nHeight = s_ptexWhite->GetHeight();
          m_nDepth = 1;
          m_nDefState = s_ptexWhite->m_nDefState;
          m_bNoTexture = true;
          SAFE_DELETE(pIM);
					Unlink();
          return false;
        }
      }

      m_bStreamFromDDS = true;
      fh.m_SizeOf = sizeof(STexCacheFileHeader);
      nNumMips = pIM->mfGet_numMips();
      m_eTFSrc = pIM->mfGetSrcFormat();
      m_eTFDst = pIM->mfGetFormat();
      m_AvgColor = pIM->mfGet_AvgColor();
			m_SrcName = pIM->mfGet_filename();
			m_eTT = pIM->mfGet_NumSides() == 1 ? eTT_2D : eTT_Cube;
			fh.m_nSides = m_eTT != eTT_Cube ? 1 : 6;
      fh.m_DstFormat = m_eTFDst;
			SAFE_DELETE_ARRAY(m_pFileTexMips);
      m_pFileTexMips = new STexCacheMipHeader[nNumMips];
			for(int iSide = 0;iSide < fh.m_nSides;++iSide)
			{
				int wdt = pIM->mfGet_width();
				int hgt = pIM->mfGet_height();
				for (int iMip=0; iMip<nNumMips; iMip++)
				{
					assert(wdt && hgt);
					m_pFileTexMips[iMip].m_USize = wdt;
					m_pFileTexMips[iMip].m_VSize = hgt;
					if (CTexture::IsDXTCompressed(m_eTFSrc))
					{
						m_pFileTexMips[iMip].m_USize = (m_pFileTexMips[iMip].m_USize + 3) & ~3;
						m_pFileTexMips[iMip].m_VSize = (m_pFileTexMips[iMip].m_VSize + 3) & ~3;
					}
					m_pFileTexMips[iMip].m_SideSize = TextureDataSize(max(1, m_pFileTexMips[0].m_USize >> iMip), max(1, m_pFileTexMips[0].m_VSize >> iMip), 1, 1, m_eTFDst);
					m_pFileTexMips[iMip].m_SizeOf = sizeof(STexCacheMipHeader);
					wdt = max(1, wdt >> 1);
					hgt = max(1, hgt >> 1);
				}
			}

			// fill file seeks
			int nSeek = pIM->mfGet_StartSeek();
			for(int iSide = 0;iSide < fh.m_nSides;++iSide)
			{
				int wdt = pIM->mfGet_width();
				int hgt = pIM->mfGet_height();
				for (int iMip=0; iMip<nNumMips;++iMip)
				{
					m_pFileTexMips[iMip].m_Mips[iSide].m_Seek = nSeek;
					nSeek += CTexture::TextureDataSize(wdt, hgt, 1, 1, m_eTFSrc);
					wdt = max(1, wdt >> 1);
					hgt = max(1, hgt >> 1);
				}
			}

      for (int iMip=0; iMip<nNumMips; iMip++)
      {
        m_pFileTexMips[iMip].m_SideSizeWithMips = 0;
        for (int j=iMip; j<nNumMips; j++)
        {
          m_pFileTexMips[iMip].m_SideSizeWithMips += m_pFileTexMips[j].m_SideSize;
        }
      }
      m_nWidth = m_pFileTexMips[0].m_USize;
      m_nHeight = m_pFileTexMips[0].m_VSize;
      m_nSrcWidth = m_nWidth;
      m_nSrcHeight = m_nHeight;
      m_nDepth = pIM->mfGet_depth();
      m_nMips = nNumMips;
      m_nFlags |= FT_FROMIMAGE;
      m_bUseDecalBorderCol = (pIM->mfGet_Flags() & FIM_DECAL) != 0;
			m_bIsSRGB = (pIM->mfGet_Flags() & FIM_SRGB_READ) != 0;

			if (!m_pPixelFormat)
				ClosestFormatSupported(m_eTFDst);
			if(!m_pPixelFormat && !DDSFormats::IsNormalMap(m_eTFDst)) // special case for 3DC and CTX1
			{
				assert(0);
				gEnv->pLog->LogError("Failed to load texture %s': format '%s' is not supported", NameForTextureFormat(m_eTFDst), m_SrcName.c_str());
				SAFE_DELETE(pIM);
				return false;
			}
#	if defined(DIRECT3D10) || defined( XENON )
			m_bIsSRGB &= gRenDev->IsLinearSpaceShadingEnabled();
#	endif
			if(m_pPixelFormat)
				m_bIsSRGB &= m_pPixelFormat->bCanReadSRGB;
			else
				m_bIsSRGB = false;

      // calc num persistent mips
			{
				fh.m_nMipsPersistent = nNumMips;
				if((m_nWidth < DDSSplitted::etexLowerMipMaxSize && m_nHeight < DDSSplitted::etexLowerMipMaxSize)
					|| m_nMips < DDSSplitted::GetNumLastMips(m_nSrcWidth, m_nSrcHeight, m_nMips, m_CacheFileHeader.m_nSides, m_eTFSrc, (m_nFlags & FT_ALPHA) ? FIM_ALPHA : 0))
				{
					// texture is too small to stream it, mark as not streamable
					SAFE_DELETE_ARRAY(m_pFileTexMips);
					m_nFlags |= FT_DONT_STREAM;
					m_bStreamed = false;
					m_bStreamPrepared = false;
					m_bWasUnloaded = false;
					m_bStreamFromDDS = false;
					m_bStreamingInProgress = false;
					m_bStreamRequested = false;
					m_bNoTexture = false;
					return false;
				}
				fh.m_nMipsPersistent = DDSSplitted::GetNumLastMips(m_nSrcWidth, m_nSrcHeight, m_nMips, m_CacheFileHeader.m_nSides, m_eTFSrc, (m_nFlags & FT_ALPHA) ? FIM_ALPHA : 0);
				assert(fh.m_nMipsPersistent > 0);
				if(fh.m_nMipsPersistent < 1)
				{
					gEnv->pLog->LogError("Wrong number of persistent mips");
					return false;
				}
			}
			m_CacheFileHeader = fh;
		}
    else
    {
			if(pIM && pIM->mfGetFormat() != eTF_Unknown && pIM->mfGet_numMips() == 1 && pIM->mfGet_width() > 64 && pIM->mfGet_height() > 64)
			{
				TextureWarning(m_SrcName.c_str(), "Texture has no mips, possible performance impact");
			}
      SAFE_DELETE(pIM);
      return false;
    }
  }

  assert (m_eTFDst != eTF_Unknown);

  SetTexStates();
  PostCreate();

  // Always load lowest nMipsPersistent mips synchronously
  byte *buf = NULL;
  if (m_nMips > 1)
  {
    nSides = m_CacheFileHeader.m_nSides;
    int nSyncStartMip = -1;
    int nSyncEndMip = -1;
    int nStartLowestM = max(0, m_nMips - m_CacheFileHeader.m_nMipsPersistent);
		if(nStartLowestM < m_nMinMipVidUploaded)
		{
			nSyncStartMip = nStartLowestM;
			nSyncEndMip = m_nMips - 1;
		}

    int nOffs = 0;
    assert(nSyncStartMip <= nSyncEndMip);

    for (int iSide=0; iSide<nSides; iSide++)
    {
      nOffs = 0;
      for (int iMip=nSyncStartMip; iMip<=nSyncEndMip; iMip++)
      {
				STexCacheMipHeader& mh = m_pFileTexMips[iMip];
				SMipData *mp = &mh.m_Mips[iSide];
        mp->Init(mh.m_SideSize, mh.m_USize, mh.m_VSize);




        CTexture::s_nTexturesDataBytesLoaded += mh.m_SideSize;
        if (iSide == 0 || (m_nFlags & FT_REPLICATE_TO_ALL_SIDES) == 0)
        {
					assert(pIM->mfIs_image(iSide));
					buf = pIM->mfGet_image(iSide);
          cryMemcpy(&mp->DataArray[0], &buf[nOffs], mh.m_SideSize);
          nOffs += mh.m_SideSize;
        }
        else if (iSide > 0)
        {
          if (m_nFlags & FT_REPLICATE_TO_ALL_SIDES)
            memcpy(&mp->DataArray[0], &mh.m_Mips[0].DataArray[0], mp->DataArray.GetMemoryUsage());
					else
						assert(0);
        }
				else
					assert(0);
      }
    }
  }
  SAFE_DELETE(pIM);



















	StreamCopyMips(m_nMips - m_CacheFileHeader.m_nMipsPersistent, m_nMips - 1, true);


	SetWasUnload(false);
	m_nMipFactorUpdateId = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nObjectUpdateId;
  m_bStreamPrepared = true;
	m_fCurrentMipBias = 0.f;

  return true;
}

//=========================================================================
void CTexture::StreamValidateTexSize()
{
#if defined(_DEBUG)
	CHK_MAINORRENDTH;

	// check if all textures in the list are valid and streamable
	static int j=0;
	if(j>=s_DistanceSortedTextures.size())
		j=0;
	for(int nIter=0;nIter<10,j<s_DistanceSortedTextures.size();++j,++nIter)
	{
		CTexture* tp = s_DistanceSortedTextures[j];
		if(tp != NULL)
		{
			assert(tp->IsValid());
			assert(tp->IsStreamed());
		}
	}

	int nSizeManagedStreamed = 0;
  int nSizeManagedNonStreamed = 0;
  int nSizeDynamic = 0;
  static SResourceContainer *pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
  ResourcesMapItor itor;
  if (!pRL)
    return;

	// do this check once per 120 frames
	if(gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID%120 != 0)
		return;

  for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
  {
    CTexture *tp = (CTexture *)itor->second;
    if (!tp->IsDynamic())
    {
      if (tp->IsStreamed())
			{
				if(tp->m_pPoolItem || tp->m_pNewPoolItem)
				{
					if(tp->m_pPoolItem)
						nSizeManagedStreamed += tp->m_pPoolItem->m_pOwner->m_Size;
					if(tp->m_pNewPoolItem)
						nSizeManagedStreamed += tp->m_pNewPoolItem->m_pOwner->m_Size;
				}
				else
					nSizeManagedStreamed += tp->GetDeviceDataSize();
			}
      else
        nSizeManagedNonStreamed += tp->GetDeviceDataSize();
    }
    else
      nSizeDynamic += tp->GetDeviceDataSize();
  }
  STexPoolItem *pIT = CTexture::s_FreeTexPoolItems.m_PrevFree;
  while (pIT != &CTexture::s_FreeTexPoolItems)
  {
    nSizeManagedStreamed += pIT->m_pOwner->m_Size;
    pIT = pIT->m_PrevFree;
  }

//  assert(nSizeManagedStreamed == s_nStatsCurManagedStreamedTexMem);
//  assert(nSizeManagedNonStreamed == s_nStatsCurManagedNonStreamedTexMem);
//  assert(nSizeDynamic == s_nStatsCurDynamicTexMem);
#endif
}

struct CompareItemDist
{
	bool operator()(const CTexture *a, const CTexture *b) const
	{
		if(a->m_nAccessFrameID != b->m_nAccessFrameID)
			return a->m_nAccessFrameID > b->m_nAccessFrameID;
		if(a->m_nMinMipVidUploaded - a->m_nMinMipCur != b->m_nMinMipVidUploaded - b->m_nMinMipCur)
			return a->m_nMinMipVidUploaded - a->m_nMinMipCur > b->m_nMinMipVidUploaded - b->m_nMinMipCur;
		if(a->m_fLastMipFactor != b->m_fLastMipFactor)
			return a->m_fLastMipFactor < b->m_fLastMipFactor;
		return a < b;
	}
};

void CTexture::AsyncRequestsProcessing()
{
	CHK_RENDTH;
	CTexture *pTex;
  int nSize;
	uint32 n = 0;

  PROFILE_FRAME(Texture_StreamASyncRequests);

	const int nPoolSize = CRenderer::CV_r_texturesstreampoolsize*1024*1024;
	int nCurrentPoolSize = CTexture::s_nStatsCurManagedStreamedTexMem;

	// update streaming icon if we have some tasks
	if(!s_StreamingTextures.empty())
		CTexture::s_bStreamingShow = true;

  int nCurSizeToLoad = 0;



  if (s_StreamingRequested.Num())
  {
    std::sort(&s_StreamingRequested[0], &s_StreamingRequested[0]+s_StreamingRequested.Num(), CompareItemDist());
    for (n=0; n<s_StreamingRequested.Num(); n++)
    {
      pTex = s_StreamingRequested[n];
      if (!pTex)
        continue;

      // async processing doesn't support concurrent texture uploads
      if (pTex->m_bStreamingInProgress)
        continue;

      STexCacheMipHeader *mh = pTex->m_pFileTexMips;
      if (!mh)
        continue;
      assert (pTex->m_bStreamRequested);
      assert (pTex->m_nAsyncStartMip >= 0);
      const int nASyncStartMip = pTex->m_nAsyncStartMip;
			assert(nASyncStartMip < pTex->m_nMinMipVidUploaded);
			int nSides = pTex->m_CacheFileHeader.m_nSides;
      const int nMips = pTex->m_nMips;
      const int nASyncEndMip = pTex->m_nMinMipVidUploaded-1;
      assert(nASyncStartMip <= nASyncEndMip);
      int nSeekFromStart = mh[nASyncStartMip].m_Mips[0].m_Seek;
      nSize = pTex->m_nSize/nSides;
      pTex->m_bStreamRequested = false;
      for (int i=nASyncStartMip; i<=nASyncEndMip; i++)
				nCurSizeToLoad += CTexture::TextureDataSize(mh[i].m_USize, mh[i].m_VSize, 1, 1, pTex->GetSrcFormat()) * nSides;

			// not allow more that CV_r_texturesstreamingmaxasync KB per frame
			if (CRenderer::CV_r_texturesstreamingmaxasync)
				if (float(nCurSizeToLoad) > CRenderer::CV_r_texturesstreamingmaxasync * 1024.f * 1024.f && n)
          break;

			// check if we're out of pool size
			if (nCurrentPoolSize + nCurSizeToLoad >= nPoolSize)
			{
				// trigger pool overflow
				s_bPoolOverflow = 1;
				break;
			}
			nCurrentPoolSize -= nCurSizeToLoad;

			STexPoolItem *pIT = NULL;
      int nSizeToLoad = 0;
      for (int i=nASyncStartMip; i<=nASyncEndMip; i++)
				nSizeToLoad += CTexture::TextureDataSize(mh[i].m_USize, mh[i].m_VSize, 1, 1, pTex->GetSrcFormat()) * ((pTex->m_nFlags & FT_REPLICATE_TO_ALL_SIDES) != 0 ? 1 : nSides);

      if (nSizeToLoad)
      {
#ifdef DO_RENDERLOG
        if (gRenDev->m_LogFileStr)
          gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Async Start Load '%s' (Mips: %d-%d[%d]), Size: %d, Time: %.3f\n", pTex->m_SrcName.c_str(), nASyncStartMip, nASyncEndMip, nMips, nSizeToLoad, iTimer->GetAsyncCurTime());
#endif
				CTexture::s_nTexturesDataBytesLoaded += nSizeToLoad;
				bool requestStreaming = true;












				if(requestStreaming)
				{
					if (CRenderer::CV_r_texturesstreamingdebug == 1)
						iLog->Log("Requesting mips: %s - From: %i, To: %i", pTex->m_SrcName.c_str(), nASyncStartMip, nASyncEndMip);
					StartStreaming(pTex, nSeekFromStart, nSizeToLoad, nASyncStartMip, nASyncEndMip);
				}
      }
    }
  }
  for (; n<s_StreamingRequested.Num(); n++)
  {
    s_StreamingRequested[n]->m_bStreamRequested = false;
  }
  s_StreamingRequested.SetNum(0);

  CTexture::StreamCheckTexLimits();
}

void  CTexture::StartStreaming( CTexture* pTex, const size_t nOffset, const size_t nSize, int nStartMip, int nEndMip )
{
	CHK_RENDTH;
	assert(nEndMip < max(0, pTex->m_nMips - pTex->m_CacheFileHeader.m_nMipsPersistent));

	DDSSplitted::Chunks chunks;

	uint32 nImageFlags = 0;
	if(pTex->m_nFlags&FT_SPLITTED)
		nImageFlags |= FIM_SPLITTED;
	if(pTex->m_nFlags&FT_ALPHA)
		nImageFlags |= FIM_ALPHA;

	DDSSplitted::GetFilesToRead(chunks, pTex->m_SrcName, nOffset, nSize, pTex->m_nWidth, pTex->m_nHeight, 
															pTex->m_nDepth, pTex->m_CacheFileHeader.m_nSides, pTex->m_nMips, nStartMip, 
															nEndMip, pTex->m_CacheFileHeader.m_nMipsPersistent, pTex->GetSrcFormat(), 
															nImageFlags);
	assert(chunks.size() == nEndMip - nStartMip + 1);

	STextureStreamingSharedInfo *pTexStreamingSharedInfo = new STextureStreamingSharedInfo;
	pTexStreamingSharedInfo->m_pTexture = pTex;
	pTexStreamingSharedInfo->m_fStartTime = iTimer->GetAsyncTime();
	pTexStreamingSharedInfo->m_nAsyncRefCount = chunks.size();
	pTexStreamingSharedInfo->m_nSyncRefCount = chunks.size();
	pTexStreamingSharedInfo->m_nHigherUploadedMip = nStartMip;
	pTexStreamingSharedInfo->m_nLowerUploadedMip = nEndMip;

	// update streaming frame ID
	pTex->m_nStreamingSwitchFrameID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

	// add requests
	size_t nBufferOffset = 0;
	for(DDSSplitted::Chunks::const_iterator it = chunks.begin();it != chunks.end();++it)
	{
		STextureStreamingInfo* pTexStreamingInfo = new STextureStreamingInfo;
		assert(it->m_nMipLevel >= nStartMip);
		pTexStreamingInfo->m_nRelativeMipLevel = it->m_nMipLevel - nStartMip;
		pTexStreamingInfo->m_pSharedInfo = pTexStreamingSharedInfo;

		const DDSSplitted::ChunkInfo& chunk = *it;

		StreamReadParams StrParams;
		StrParams.nFlags = 0;
		StrParams.dwUserData = (DWORD_PTR)pTexStreamingInfo;
		StrParams.nLoadTime = 1;
		StrParams.nMaxLoadTime = 4;
		StrParams.nPriority = (int)(1000000.f / max(.1f, pTex->m_fLastMipFactor)) + (nSize - nBufferOffset) / 1024;	// distance + buffer m_nOffsetInFile
		StrParams.pBuffer = NULL;
		StrParams.nOffset = chunk.m_nOffsetInFile;
		StrParams.nSize = chunk.size;
		CTexture::StreamAddTask(pTex, chunk.fileName.c_str(), &pTexStreamingInfo->m_Callback, &StrParams);
		nBufferOffset += chunk.size;
	}
	assert(nBufferOffset == nSize);
}

void CTexture::StreamAddTask( CTexture* pTex, const char* szFile, IStreamCallback* pCallback /*= NULL*/, StreamReadParams* pParams /*= NULL*/ )
{
	CHK_RENDTH;
	if(pTex)
		pTex->SetStreamingInProgress(true);
	IReadStreamPtr task = iSystem->GetStreamEngine()->StartRead(eStreamTaskTypeTexture, szFile, pCallback, pParams);
	s_StreamingTextures.push(task);
	if(pTex)
		pTex->m_TextureStreamTasks.push(task);
	else
		assert(0);
}

void CTexture::StreamRemoveTask( CTexture* pTex, IReadStreamPtr pTask )
{
	CHK_MAINORRENDTH;
	bool res = s_StreamingTextures.try_remove(pTask);
	assert(res);
	if(pTex)
	{
		res = pTex->m_TextureStreamTasks.try_remove(pTask);
		assert(res);
		if(pTex->m_TextureStreamTasks.empty())
		{
			// if we have streaming error, release new pool item
			if(pTex->m_pNewPoolItem)		
			{
				pTex->m_pNewPoolItem->m_pTex = NULL;
				pTex->m_pNewPoolItem->Unlink();
				pTex->m_pNewPoolItem->LinkFree(&CTexture::s_FreeTexPoolItems);
				pTex->m_pNewPoolItem = NULL;
			}
			pTex->SetStreamingInProgress(false);
		}
	}
	else
		assert(0);
}

void CTexture::StreamUploadMips(int nStartMip, int nEndMip)
{
  CTimeValue time0 = iTimer->GetAsyncTime();























  StreamCopyMips(nStartMip, nEndMip, true);

  if (s_bStreamDontKeepSystem)
    StreamReleaseMipsData(0, m_nMips - 1);

  gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_fTexUploadTime += (iTimer->GetAsyncTime() - time0).GetSeconds();
}

void CTexture::InitStreaming()
{
	CHK_MAINORRENDTH;
	iLog->Log("Init textures management (%d Mb of texture memory)...", gRenDev->m_MaxTextureMemory/1024/1024);
  s_nStreamingEnabled = 1;

	s_nStreamingThroughput = 0;
	s_nStreamingTotalTime = 0;

  bool bLowResTextures = false;
	
	if(CRenderer::CV_r_texbumpresolution!=0	|| CRenderer::CV_r_texresolution!=0)
		bLowResTextures=true;

  // unless user explicitly wants so
  if (CRenderer::CV_r_texturesstreaming == 0)
    s_nStreamingEnabled=0;

  SSystemGlobalEnvironment *pEnv = iSystem->GetGlobalEnvironment();

  // auto mode
  if (CRenderer::CV_r_texturesstreaming == 2)
  {
    // or we have enough memory resources to handle
    if (sizeof(void*)==8 && gRenDev->m_MaxTextureMemory >= 500*1024*1024)
      s_nStreamingEnabled=0;

#ifdef WIN32
    // if we have low res textures we should consider
    if (bLowResTextures && gRenDev->m_MaxTextureMemory >= 200*1024*1024 && pEnv->pi.numCoresAvailableToProcess == 1)
    {
      // no vista
      if (pEnv->pi.winVer<SPlatformInfo::WinVista)
        s_nStreamingEnabled = 0;

      // vista 32bit with patch installed
      if (pEnv->pi.winVer>=SPlatformInfo::WinVista && !pEnv->pi.win64Bit && !pEnv->pi.vistaKB940105Required)
        s_nStreamingEnabled = 0;

      // 64-bit Vista + 64 bit CryENGINE
      if (pEnv->pi.winVer>=SPlatformInfo::WinVista && pEnv->pi.win64Bit && sizeof(void*) == 8) // potentially add sizeof(void*) == 8 if you want to say 
        s_nStreamingEnabled = 0;
    }                                
    if (s_nStreamingEnabled)
      iLog->Log("  Textures streaming automatically enabled (Configuration: %d cores, OS: '%s', %d bits OS)...", pEnv->pi.numCoresAvailableToProcess, pEnv->pi.winVer>=SPlatformInfo::Win7 ? "Windows 7" : pEnv->pi.winVer>=SPlatformInfo::WinVista ? "Vista" : pEnv->pi.winVer==SPlatformInfo::WinXP ? "XP" : pEnv->pi.winVer==SPlatformInfo::Win2000 ? "Win2000" : pEnv->pi.winVer==SPlatformInfo::WinSrv2003 ? "Windows Server 2003" : "Unknown", pEnv->pi.win64Bit ? 64 : 32);
    else
      iLog->Log("  Textures streaming automatically disabled (Configuration: %d cores, OS: '%s', %d bits OS)...", pEnv->pi.numCoresAvailableToProcess, pEnv->pi.winVer>=SPlatformInfo::Win7 ? "Windows 7" : pEnv->pi.winVer>=SPlatformInfo::WinVista ? "Vista" : pEnv->pi.winVer==SPlatformInfo::WinXP ? "XP" : pEnv->pi.winVer==SPlatformInfo::Win2000 ? "Win2000" : pEnv->pi.winVer==SPlatformInfo::WinSrv2003 ? "Windows Server 2003" : "Unknown", pEnv->pi.win64Bit ? 64 : 32);
#endif
  }
  else
  {
#ifdef WIN32
    if (s_nStreamingEnabled)
      iLog->Log("  Textures streaming forced on (Configuration: %d cores, OS: '%s', %d bits OS)...", pEnv->pi.numCoresAvailableToProcess, pEnv->pi.winVer>=SPlatformInfo::Win7 ? "Windows 7" : pEnv->pi.winVer==SPlatformInfo::WinVista ? "Vista" : pEnv->pi.winVer==SPlatformInfo::WinXP ? "XP" : pEnv->pi.winVer==SPlatformInfo::Win2000 ? "Win2000" : pEnv->pi.winVer==SPlatformInfo::WinSrv2003 ? "Windows Server 2003" : "Unknown", pEnv->pi.win64Bit ? 64 : 32);
    else
      iLog->Log("  Textures streaming forced off (Configuration: %d cores, OS: '%s', %d bits OS)...", pEnv->pi.numCoresAvailableToProcess, pEnv->pi.winVer>=SPlatformInfo::Win7 ? "Windows 7" : pEnv->pi.winVer==SPlatformInfo::WinVista ? "Vista" : pEnv->pi.winVer==SPlatformInfo::WinXP ? "XP" : pEnv->pi.winVer==SPlatformInfo::Win2000 ? "Win2000" : pEnv->pi.winVer==SPlatformInfo::WinSrv2003 ? "Windows Server 2003" : "Unknown", pEnv->pi.win64Bit ? 64 : 32);
#endif
  }

  s_nStreamingMode = CRenderer::CV_r_texturesstreaming;
#if !defined(XENON) && !defined(PS3)
  s_bStreamDontKeepSystem = CRenderer::CV_r_texturesstreamingonlyvideo == 0;


#endif
  if (!s_FreeTexPoolItems.m_NextFree)
  {
    s_FreeTexPoolItems.m_NextFree = &s_FreeTexPoolItems;
    s_FreeTexPoolItems.m_PrevFree = &s_FreeTexPoolItems;
  }
  if (s_nStreamingEnabled && CRenderer::CV_r_texturesstreaming == 2)
  {
#ifdef WIN32
    // no 64 bits
    if (!pEnv->pi.win64Bit)
    {
      if (CRenderer::CV_r_texturesstreampoolsize > 128)
        CRenderer::CV_r_texturesstreampoolsize = 128;
    }
    else
#endif
#if !defined(XENON) && !defined(PS3)
    if (gRenDev->m_MaxTextureMemory <= 220*1024*1024)
    {
      if (CRenderer::CV_r_texturesstreampoolsize > 100)
        CRenderer::CV_r_texturesstreampoolsize = 100;
      if (CRenderer::CV_r_texturesstreamingmaxasync > 0.18f)
        CRenderer::CV_r_texturesstreamingmaxasync = 0.18f;
    }
    else
    if (gRenDev->m_MaxTextureMemory <= 500*1024*1024)
    {
      if (CRenderer::CV_r_texturesstreampoolsize > 148)
        CRenderer::CV_r_texturesstreampoolsize = 148;
    }
    else
    if (gRenDev->m_MaxTextureMemory <= 700*1024*1024)
    {
      if (CRenderer::CV_r_texturesstreampoolsize > 256)
        CRenderer::CV_r_texturesstreampoolsize = 256;
    }
    else
    {
      if (CRenderer::CV_r_texturesstreampoolsize < 256)
        CRenderer::CV_r_texturesstreampoolsize = 256;
    }
    if (CRenderer::CV_r_texturesstreampoolsize < 100)
      CRenderer::CV_r_texturesstreampoolsize = 100;






#endif
  }

	if (s_nStreamingEnabled)
	{
		iLog->Log("  Enabling of textures streaming...");
		iLog->Log("  Using %d Mb of textures pool for streaming...", CRenderer::CV_r_texturesstreampoolsize);
	}
	else
		iLog->Log("  Disabling of textures streaming...");

  if (gRenDev->m_MaxTextureMemory <= 256*1024*1024)
  {
    if (SDynTexture::s_CurDynTexAtlasCloudsMaxsize > 24)
      SDynTexture::s_CurDynTexAtlasCloudsMaxsize = 24;
    if (SDynTexture::s_CurDynTexAtlasSpritesMaxsize > 32)
      SDynTexture::s_CurDynTexAtlasSpritesMaxsize = 32;
    if (SDynTexture::s_CurTexAtlasSize > 1024)
      SDynTexture::s_CurTexAtlasSize = 1024;
    if (SDynTexture::s_CurDynTexMaxSize > 64)
      SDynTexture::s_CurDynTexMaxSize = 64;
  }
  iLog->Log("  Video textures: Atlas clouds max size: %d Mb", SDynTexture::s_CurDynTexAtlasCloudsMaxsize);
  iLog->Log("  Video textures: Atlas sprites max size: %d Mb", SDynTexture::s_CurDynTexAtlasSpritesMaxsize);
  iLog->Log("  Video textures: Dynamic managed max size: %d Mb", SDynTexture::s_CurDynTexMaxSize);

  SResourceContainer *pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
  ResourcesMapItor itor;

  if (pRL)
  {
    int nID = 0;
    for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++, nID++)
    {
      CTexture *tp = (CTexture *)itor->second;
      if (!tp)
        continue;
      if (!(tp->m_nFlags & FT_FROMIMAGE) || (tp->m_nFlags & FT_DONT_STREAM))
        continue;
      tp->ToggleStreaming(s_nStreamingEnabled);
    }
  }

	if (gEnv->pLocalMemoryUsage)
	{
		gEnv->pLocalMemoryUsage->DeleteGlobalData();
	}
}


void CTexture::FlushAllStreamingTasks(CTexture* pTex)
{
	CHK_MAINORRENDTH;
	if(pTex == NULL)	// flush all tasks globally
	{
		iLog->Log("Flushing pended textures...");
		while(!s_StreamingTextures.empty())
		{
			IReadStreamPtr pStream;
			if(s_StreamingTextures.try_pop(pStream))
			{
				if(pStream->IsFinished() || pStream->IsError())
				{
					s_StreamingTextures.try_remove(pStream);
				}
				pStream->Wait();
			}
		}
		iLog->Log("Finished flushing pended textures...");
	}
	else
	{
		while(!pTex->m_TextureStreamTasks.empty())
		{
			IReadStreamPtr pStream;
			if(pTex->m_TextureStreamTasks.try_pop(pStream))
			{
				if(pStream->IsFinished() || pStream->IsError())
				{
					pTex->m_TextureStreamTasks.try_remove(pStream);
					s_StreamingTextures.try_remove(pStream);
				}
				pStream->Wait();
			}
		}
	}
}

void CTexture::AbortStreamingTasks( CTexture* pTex )
{
  CHK_MAINORRENDTH;
  while(!pTex->m_TextureStreamTasks.empty())
  {
    IReadStreamPtr pStream = pTex->m_TextureStreamTasks.back();
		if(pStream->IsFinished())
		{
			bool res = pTex->m_TextureStreamTasks.try_remove(pStream);
			assert(res);
			res = s_StreamingTextures.try_remove(pStream);
			assert(res);
		}
    pStream->Abort();
  }

  if(pTex->m_pNewPoolItem)		// if we have not assigned pool item, release it
  {
    pTex->m_pNewPoolItem->m_pTex = NULL;
    pTex->m_pNewPoolItem->Unlink();
    pTex->m_pNewPoolItem->LinkFree(&CTexture::s_FreeTexPoolItems);
    pTex->m_pNewPoolItem = NULL;
  }
  pTex->SetStreamingInProgress(false);
}


//Diesel new
int CTexture::StreamCalculateMips(float fMipFactor) const
{
	assert(IsStreamed());
	const uint32 nMaxExtent = max(m_nWidth, m_nHeight);
	float currentMipFactor = fMipFactor * nMaxExtent * nMaxExtent * gRenDev->GetMipDistFactor();
	int nMip = int(ceilf(0.5f * logf(max(currentMipFactor, 1.0f)) / LN2)) + CRenderer::CV_r_TexturesStreamingMipBias;
	const int nNewMip = min(nMip, max(0, m_nMips - m_CacheFileHeader.m_nMipsPersistent));
	return nNewMip;
}
int CTexture::GetStreamableMipNumber() const
{
	assert(IsStreamed());
	return max(0, m_nMips - m_CacheFileHeader.m_nMipsPersistent);
}
int CTexture::GetStreamableMemoryUsage(int nStartMip) const
{
	assert(IsStreamed());
	if (m_pFileTexMips == NULL)
	{
		assert(0);
		return 0;
	}
//Diesel cut
	//return m_pFileTexMips->m_pMipHeader[nStartMip].m_SideSizeWithMips;
//Diesel new
	return 1;
}
//Diesel new