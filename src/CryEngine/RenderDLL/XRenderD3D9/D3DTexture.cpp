/*=============================================================================
 D3DTexture.cpp : Direct3D specific texture manager implementation.
 Copyright (c) 2001 Crytek Studios. All Rights Reserved.
 
 Revision history:
 * Created by Honitch Andrey
 
 =============================================================================*/
 
 #include "StdAfx.h"
 #include "DriverD3D.h"
 #include "D3DTexture.h"
 #include "D3DCubeMaps.h"
 #include "I3DEngine.h"
 #include "StringUtils.h"
 #include "IDirectBee.h"			// connection to D3D9 wrapper
 #include <ImageExtensionHelper.h>
 #include <IFlashPlayer.h>
 #include <IVideoPlayer.h>
 #include "BitFiddling.h"		// ConvertBlock3DcToDXT5()
 
 
 
 
 
 
 //===============================================================================
 
 byte *CTexture::Convert(byte *pSrc, int nWidth, int nHeight, int nMips, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nOutMips, int& nOutSize, bool bLinear)
 {
 	nOutSize = 0;
 	byte *pDst = NULL;
 	if (eTFSrc == eTFDst && nMips == nOutMips)
 		return pSrc;
 	CD3D9Renderer *r = gcpRendD3D;
 	HRESULT h;
 #if defined (DIRECT3D9) || defined(OPENGL)
 	D3DFormat DeviceFormatSRC = DeviceFormatFromTexFormat(eTFSrc);
 	D3DFormat DeviceFormatDST = DeviceFormatFromTexFormat(eTFDst);
 	if (DeviceFormatSRC == D3DFMT_UNKNOWN || DeviceFormatDST == D3DFMT_UNKNOWN)
 #elif defined (DIRECT3D10)
 	DXGI_FORMAT DeviceFormatSRC = (DXGI_FORMAT)DeviceFormatFromTexFormat(eTFSrc);
 	if (eTFDst == eTF_L8)
 		eTFDst = eTF_A8;
 	DXGI_FORMAT DeviceFormatDST = (DXGI_FORMAT)DeviceFormatFromTexFormat(eTFDst);
 	if (DeviceFormatSRC == DXGI_FORMAT_UNKNOWN || DeviceFormatDST == DXGI_FORMAT_UNKNOWN)
 #endif
 	{
 		assert(0);
 		return NULL;
 	}
 	if (nMips <= 0)
 		nMips = 1;
 	int nSizeDSTMips = 0;
 	int i;
 
 	if (eTFSrc == eTF_3DC)
 	{
 		if(eTFDst == eTF_DXT5)
 		{
 			// convert 3Dc to DXT5 - for compressed normals
 			if (!nOutMips)
 				nOutMips = nMips;
 			int wdt = nWidth;
 			int hgt = nHeight;
 			nSizeDSTMips = CTexture::TextureDataSize(wdt, hgt, 1, nOutMips, eTF_DXT5);
 			pDst = new byte [nSizeDSTMips];
 			int nOffsDST = 0;
 			int nOffsSRC = 0;
 			for (i=0; i<nOutMips; i++)
 			{
 				if (i >= nMips)
 					break;
 				if (wdt <= 0)
 					wdt = 1;
 				if (hgt <= 0)
 					hgt = 1;
 				void *outSrc = &pSrc[nOffsSRC];
 				DWORD outSize = CTexture::TextureDataSize(wdt, hgt, 1, 1, eTFDst);
 
 				nOffsSRC += CTexture::TextureDataSize(wdt, hgt, 1, 1, eTFSrc);
 
 				{
 					byte *src = (byte *)outSrc;
 
 					for (uint32 n=0; n < outSize/16; n++)			// for each block
 					{
 						uint8 *pSrcBlock = &src[n*16];
 						uint8 *pDstBlock = &pDst[nOffsDST+n*16];
 
 						ConvertBlock3DcToDXT5(pDstBlock,pSrcBlock);
 					}
 
 					nOffsDST += outSize;
 				}
 
 				wdt >>= 1;
 				hgt >>= 1;
 			}
 			if ((nOutMips>1 && nOutMips != nMips))
 			{
 				assert(0);			// generate mips if needed - should not happen
 				return 0;
 			}
 			nOutSize = nSizeDSTMips;
 			return pDst;
 		}
 		else
 		{
       assert(0);
 			return NULL;
 		}
 	}
 	else
 	if (DDSFormats::IsNormalMap(eTFDst))
 	{
     assert(0);
 		return NULL;
 	}
 	else
 	{
 
 
 
 
 
 
 
 #if defined (DIRECT3D9) || defined(OPENGL)
 		D3DTexture *pID3DTextureDST = NULL;
 		D3DSurface *pSurfDST;
 		D3DLOCKED_RECT d3dlr;
 		if(FAILED(h=D3DXCreateTexture(gcpRendD3D->GetDevice(), nWidth, nHeight, nOutMips, 0, DeviceFormatDST, D3DPOOL_SYSTEMMEM, &pID3DTextureDST)))
 			return NULL;
 		int wdt = nWidth;
 		int hgt = nHeight;
 		int nOffsSRC = 0;
 		for (i=0; i<nMips; i++)
 		{
 			if (!wdt)
 				wdt = 1;
 			if (!hgt)
 				hgt = 1;
 			int nSizeSRC = TextureDataSize(wdt, hgt, 1, 1, eTFSrc);
 			h = pID3DTextureDST->GetSurfaceLevel(i, &pSurfDST);
 
 			int nPitch;
 			if (!IsDXTCompressed(eTFSrc))
 				nPitch = nSizeSRC / hgt;
 			else
 			{
 				int blockSize = (eTFSrc == eTF_DXT1 || eTFSrc == eTF_CTX1) ? 8 : 16;
 				nPitch = (wdt+3)/4 * blockSize;
 			}
 			RECT srcRect;
 			srcRect.left = 0;
 			srcRect.top = 0;
 			srcRect.right = wdt;
 			srcRect.bottom = hgt;
 
 			h = D3DXLoadSurfaceFromMemory(pSurfDST, NULL, NULL, &pSrc[nOffsSRC], DeviceFormatSRC, nPitch, NULL, &srcRect, D3DX_FILTER_NONE, 0);
 
 			SAFE_RELEASE(pSurfDST);
 
 			nOffsSRC += nSizeSRC;
 			wdt >>= 1;
 			hgt >>= 1;
 		}
 		if (nMips <= 1 && nOutMips > 1)
 			h = D3DXFilterTexture(pID3DTextureDST, NULL, D3DX_DEFAULT, D3DX_FILTER_LINEAR);
 		wdt = nWidth;
 		hgt = nHeight;
 		nSizeDSTMips = TextureDataSize(wdt, hgt, 1, nOutMips, eTFDst);
 		pDst = new byte[nSizeDSTMips];
 		int nOffsDST = 0;
 		for (i=0; i<nOutMips; i++)
 		{
 			if (!wdt)
 				wdt = 1;
 			if (!hgt)
 				hgt = 1;
 			int nSizeDST = TextureDataSize(wdt, hgt, 1, 1, eTFDst);
 
 			pID3DTextureDST->LockRect(i, &d3dlr, NULL, 0);
 			memcpy(&pDst[nOffsDST], d3dlr.pBits, nSizeDST);
 			pID3DTextureDST->UnlockRect(i);
 
 			nOffsDST += nSizeDST;
 			wdt >>= 1;
 			hgt >>= 1;
 		}
 		SAFE_RELEASE(pID3DTextureDST);
 #elif defined (DIRECT3D10)
 		if (eTFDst == eTF_A8 && (eTFSrc == eTF_X8R8G8B8 || eTFSrc == eTF_A8R8G8B8) && nMips == nOutMips)
 		{
 			int nSrcSize = CTexture::TextureDataSize(nWidth, nHeight, 1, nMips, eTFSrc) / 4;
 			pDst = new byte[nSrcSize];
 			for (i=0; i<nSrcSize; i++)
 			{
 				pDst[i] = (pSrc[i*4+0] + pSrc[i*4+1] + pSrc[i*4+2]) / 3;
 			}
 			nSizeDSTMips = nSrcSize;
 			return pDst;
 		}
 		D3D11_TEXTURE2D_DESC Desc;
 		ZeroStruct(Desc);
 		D3DTexture *pID3DTextureDST = NULL;
 		Desc.Format = DeviceFormatDST;
 		Desc.Width = nWidth;
 		Desc.Height = nHeight;
 		Desc.MipLevels = nOutMips;
 		Desc.ArraySize = 1;
 		Desc.SampleDesc.Count = 1;
 		Desc.Usage = D3D11_USAGE_STAGING;
 		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
 		Desc.BindFlags = 0;
 		gcpRendD3D->m_pd3dDevice->CreateTexture2D(&Desc, NULL, &pID3DTextureDST);
 		int nSrcSize = CTexture::TextureDataSize(nWidth, nHeight, 1, nMips, eTFSrc);
 		D3DX11_IMAGE_LOAD_INFO LoadInfo;
 		ZeroStruct(LoadInfo);
 		LoadInfo.Width = D3DX_FROM_FILE;
 		LoadInfo.Height = D3DX_FROM_FILE;
 		LoadInfo.Depth = D3DX_FROM_FILE;
 		LoadInfo.FirstMipLevel = 0;
 		LoadInfo.MipLevels = nOutMips;
 		LoadInfo.Usage = D3D11_USAGE_STAGING;
 		LoadInfo.BindFlags = 0;
 		LoadInfo.CpuAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
 		LoadInfo.MiscFlags = 0;
 		LoadInfo.Format = DeviceFormatDST;
 		LoadInfo.Filter = D3DX11_FILTER_NONE;
 		LoadInfo.MipFilter = D3DX11_FILTER_NONE;
 
 		D3DX11_IMAGE_INFO ImgInfo;
 		ZeroStruct(ImgInfo);
 		ImgInfo.Width = nWidth;
 		ImgInfo.Height = nHeight;
 		ImgInfo.Depth = 1;
 		ImgInfo.Format = DeviceFormatSRC;
 		ImgInfo.MipLevels = nMips;
 		ImgInfo.ArraySize = 1;
 		ImgInfo.ImageFileFormat = D3DX11_IFF_DDS;
 		ImgInfo.ResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D;
 
 		LoadInfo.pSrcInfo = &ImgInfo;
 
 		nSizeDSTMips = 0;
 		int nSizeDDS = 0;
 		pDst = WriteDDS(pSrc, nWidth, nHeight, 1, NULL, eTFSrc, nMips, eTT_2D, true, &nSizeDDS);
 		h = D3DX11CreateTextureFromMemory(gcpRendD3D->m_pd3dDevice, pDst, nSizeDDS, &LoadInfo, NULL, (ID3D11Resource **)&pID3DTextureDST, &h);
 		assert(SUCCEEDED(h));
 		SAFE_DELETE_ARRAY(pDst);
 		int nDstSize = CTexture::TextureDataSize(nWidth, nHeight, 1, nMips, eTFDst);
 		pDst = new byte[nDstSize];
 		if (pID3DTextureDST)
 		{
 			int nOffsDST = 0;
 			for (i=0; i<nOutMips; i++)
 			{
 				if (!nWidth)
 					nWidth = 1;
 				if (!nHeight)
 					nHeight = 1;
 				int nSizeDST = TextureDataSize(nWidth, nHeight, 1, 1, eTFDst);
 
 				D3D11_MAPPED_SUBRESOURCE MappedTex2D;
 				STALL_PROFILER("set texture");
 				h = gcpRendD3D->m_pd3dDeviceContext->Map(pID3DTextureDST, i, D3D11_MAP_READ, 0, &MappedTex2D);
 				assert(SUCCEEDED(h));
 				memcpy(&pDst[nOffsDST], MappedTex2D.pData, nSizeDST);
 				gcpRendD3D->m_pd3dDeviceContext->Unmap(pID3DTextureDST, i);
 
 				nOffsDST += nSizeDST;
 				nSizeDSTMips += nSizeDST;
 				nWidth >>= 1;
 				nHeight >>= 1;
 			}
 			SAFE_RELEASE(pID3DTextureDST);
 		}
 #endif
 	}
 	nOutSize = nSizeDSTMips;
 	return pDst;
 }
 
 D3DSurface *CTexture::GetSurface(int nCMSide, int nLevel)
 {
 	if (!m_pDevTexture)
 		return NULL;
 	HRESULT hr = S_OK;
 #if defined (DIRECT3D9) || defined (OPENGL)
 	LPDIRECT3DSURFACE9 pTargSurf = NULL;
 	if (m_eTT == eTT_Cube || m_eTT == eTT_AutoCube)
 	{
 		assert(nCMSide >= 0 && nCMSide < 6);
 		D3DCubeTexture *pID3DTargetCubeTexture = m_pDevTexture->GetCubeTexture();
 		hr = pID3DTargetCubeTexture->GetCubeMapSurface((D3DCUBEMAP_FACES)nCMSide, nLevel, &pTargSurf);
 	}
 	else
   if (m_eTT == eTT_2D || m_eTT == eTT_Auto2D)
 	{
 		D3DTexture *pID3DTargetTexture = m_pDevTexture->Get2DTexture();
 		hr = pID3DTargetTexture->GetSurfaceLevel(nLevel, &pTargSurf);
 	}
 #elif defined (DIRECT3D10)
 	ID3D11Texture2D *pID3DTexture = NULL;
 	ID3D11Texture3D *pID3DTexture3D = NULL;
 	ID3D11Texture2D *pID3DTextureCube = NULL;
 	ID3D11RenderTargetView *pTargSurf = (ID3D11RenderTargetView *)m_pDeviceRTV;
 	if (!pTargSurf)
 	{
 		if (m_eTT == eTT_2D)
 		{
 			pID3DTexture = m_pDevTexture->Get2DTexture();
 			if (m_nFlags & FT_USAGE_FSAA)
 				pID3DTexture = (ID3D11Texture2D *)m_pDeviceTextureMSAA;
 
 			D3D11_RENDER_TARGET_VIEW_DESC DescRT;
 			DescRT.Format = (DXGI_FORMAT)m_pPixelFormat->DeviceFormat;
 			DescRT.ViewDimension = (m_nFlags & FT_USAGE_FSAA) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
 			DescRT.Texture2D.MipSlice = 0;
 
 
 
 			hr = gcpRendD3D->m_pd3dDevice->CreateRenderTargetView(pID3DTexture, &DescRT, &pTargSurf);
 		}
 		else
 		if (m_eTT == eTT_Cube)
 		{
 			pID3DTextureCube = m_pDevTexture->GetCubeTexture();
 
 			D3D11_RENDER_TARGET_VIEW_DESC DescRT;	
 			DescRT.Format = (DXGI_FORMAT)m_pPixelFormat->DeviceFormat;
 			DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
 			DescRT.Texture2DArray.FirstArraySlice = nCMSide;
 			DescRT.Texture2DArray.ArraySize = 1;
 			DescRT.Texture2DArray.MipSlice = 0;
 			hr = gcpRendD3D->m_pd3dDevice->CreateRenderTargetView(pID3DTextureCube, &DescRT, &pTargSurf);
 		}
 		m_pDeviceRTV = pTargSurf;
 	}
 #endif
 	assert(hr == S_OK);
 
 	return pTargSurf;
 }
 
 D3DPOOL CTexture::GetPool()
 {
   if (!m_pDevTexture)
     return D3DPOOL_SYSTEMMEM;
   HRESULT hr = S_OK;
   D3DPOOL Pool = D3DPOOL_SYSTEMMEM;
   int nLevel = 0;
 #if defined (DIRECT3D9) || defined (OPENGL)
   if (m_eTT == eTT_Cube || m_eTT == eTT_AutoCube)
   {
     D3DSURFACE_DESC Desc;
     D3DCubeTexture *pID3DTexture = m_pDevTexture->GetCubeTexture();
     hr = pID3DTexture->GetLevelDesc(nLevel, &Desc);
     Pool = Desc.Pool;
   }
   else
   if (m_eTT == eTT_2D || m_eTT == eTT_Auto2D)
   {
     D3DSURFACE_DESC Desc;
     D3DTexture *pID3DTexture = m_pDevTexture->Get2DTexture();
     hr = pID3DTexture->GetLevelDesc(nLevel, &Desc);
     Pool = Desc.Pool;
   }
   else
   if (m_eTT == eTT_3D)
   {
     D3DVOLUME_DESC Desc;
     D3DVolumeTexture *pID3DTexture = m_pDevTexture->GetVolumeTexture();
     hr = pID3DTexture->GetLevelDesc(nLevel, &Desc);
     Pool = Desc.Pool;
   }
 #elif defined (DIRECT3D10)
   Pool = D3DPOOL_MANAGED;
   if (m_eTT == eTT_2D || m_eTT == eTT_Auto2D)
   {
     D3D11_TEXTURE2D_DESC Desc;
     D3DTexture *pID3DTexture = m_pDevTexture->Get2DTexture();
     pID3DTexture->GetDesc(&Desc);
     if (Desc.BindFlags & D3D11_BIND_RENDER_TARGET)
       Pool = D3DPOOL_DEFAULT;
   }
   else
   if (m_eTT == eTT_Cube || m_eTT == eTT_AutoCube)
   {
     D3D11_TEXTURE2D_DESC Desc;
     D3DCubeTexture *pID3DTexture = m_pDevTexture->GetCubeTexture();
     pID3DTexture->GetDesc(&Desc);
     if (Desc.BindFlags & D3D11_BIND_RENDER_TARGET)
       Pool = D3DPOOL_DEFAULT;
   }
   else
   if (m_eTT == eTT_3D)
   {
     D3D11_TEXTURE3D_DESC Desc;
     D3DVolumeTexture *pID3DTexture = m_pDevTexture->GetVolumeTexture();
     pID3DTexture->GetDesc(&Desc);
     if (Desc.BindFlags & D3D11_BIND_RENDER_TARGET)
       Pool = D3DPOOL_DEFAULT;
   }
 #endif
   assert(hr == S_OK);
 
   return Pool;
 }
 
 D3DFormat CTexture::DeviceFormatFromTexFormat(ETEX_Format eTF)
 {
 #if defined (DIRECT3D9) || defined (OPENGL)
 	switch (eTF)
 	{
 	case eTF_A8R8G8B8:
 		return D3DFMT_A8R8G8B8;
 	case eTF_X8R8G8B8:
 		return D3DFMT_X8R8G8B8;
 	case eTF_A8:
 		return D3DFMT_A8;
 
 
 
 
 
 
 
 
 
 
 
 
 	case eTF_A8L8:
 		return D3DFMT_A8L8;
 	case eTF_L8:
 		return D3DFMT_L8;
 	case eTF_A4R4G4B4:
 		return D3DFMT_A4R4G4B4;
 	case eTF_R5G6B5:
 		return D3DFMT_R5G6B5;
 	case eTF_R5G5B5:
 		return D3DFMT_X1R5G5B5;
 	case eTF_V8U8:
 		return D3DFMT_V8U8;
 #if !defined(XENON)
 	case eTF_R8G8B8:
 		return D3DFMT_R8G8B8;
 	case eTF_CxV8U8:
 		return D3DFMT_CxV8U8;
 #endif
 	case eTF_X8L8V8U8:
 		return D3DFMT_X8L8V8U8;
 	case eTF_L8V8U8:
 		assert(0);
 		break;
 	case eTF_V16U16:
 		return D3DFMT_V16U16;
 	case eTF_A16B16G16R16:
 		return D3DFMT_A16B16G16R16;
 	case eTF_A16B16G16R16F:
 		return D3DFMT_A16B16G16R16F;
 	case eTF_A32B32G32R32F:
 		return D3DFMT_A32B32G32R32F;
 	case eTF_R32F:
 		return D3DFMT_R32F;
 	case eTF_R16F:
 		return D3DFMT_R16F;
 	case eTF_G16R16:
 		return D3DFMT_G16R16;
 	case eTF_G16R16F:
 		return D3DFMT_G16R16F;
 	case eTF_DXT1:
 		return D3DFMT_DXT1;
 	case eTF_DXT3:
 		return D3DFMT_DXT3;
 	case eTF_DXT5:
 		return D3DFMT_DXT5;
 	case eTF_3DC:
 		return D3DFMT_3DC;
 	case eTF_DEPTH16:
 		return D3DFMT_D16;
 	case eTF_DEPTH24:
 #if defined(INVERT_DEPTH_RANGE)
 		return D3DFMT_D24FS8;
 #else
 		return D3DFMT_D24X8;
 #endif
 	case eTF_DF16:
 		return D3DFMT_DF16;
 	case eTF_DF24:
 		return D3DFMT_DF24;
 	case eTF_D16:
 		return D3DFMT_D16;
 	case eTF_D24S8:
 		return D3DFMT_D24S8;
 	case eTF_NULL:
 		return D3DFMT_NULL;
 
 
 	default:
 		assert(0);
 	}
 	return D3DFMT_UNKNOWN;
 #elif defined (DIRECT3D10)
 	switch (eTF)
 	{
 	case eTF_R8G8B8:
 		return DXGI_FORMAT_R8G8B8A8_UNORM;
 	case eTF_A8R8G8B8:
 		return DXGI_FORMAT_R8G8B8A8_UNORM;
 	case eTF_X8R8G8B8:
 		return DXGI_FORMAT_R8G8B8A8_UNORM;
 	case eTF_A8:
 		return DXGI_FORMAT_A8_UNORM;
 	case eTF_A8L8:
 		return DXGI_FORMAT_UNKNOWN;
 	case eTF_L8:
 
 
 
 		return DXGI_FORMAT_UNKNOWN;
 	case eTF_A4R4G4B4:
 		return DXGI_FORMAT_UNKNOWN;
 	case eTF_R5G6B5:
 		return DXGI_FORMAT_B5G6R5_UNORM;
 	case eTF_R5G5B5:
 		return DXGI_FORMAT_B5G5R5A1_UNORM;
 	case eTF_V8U8:
 		return DXGI_FORMAT_R8G8_UNORM;
 	case eTF_X8L8V8U8:
 		return DXGI_FORMAT_R8G8B8A8_UNORM;
 	case eTF_L8V8U8:
 		assert(0);
 		break;
 	case eTF_V16U16:
 		return DXGI_FORMAT_R16G16_UNORM;
 	case eTF_A16B16G16R16:
 		return DXGI_FORMAT_R16G16B16A16_UNORM;
 	case eTF_A16B16G16R16F:
 		return DXGI_FORMAT_R16G16B16A16_FLOAT;
 	case eTF_A32B32G32R32F:
 		return DXGI_FORMAT_R32G32B32A32_FLOAT;
 	case eTF_R32F:
 		return DXGI_FORMAT_R32_FLOAT;
 	case eTF_R16F:
 		return DXGI_FORMAT_R16_FLOAT;
 	case eTF_G16R16F:
 		return DXGI_FORMAT_R16G16_FLOAT;
 	case eTF_DXT1:
 		return DXGI_FORMAT_BC1_UNORM;
 	case eTF_DXT3:
 		return DXGI_FORMAT_BC2_UNORM;
 	case eTF_DXT5:
 		return DXGI_FORMAT_BC3_UNORM;
 	case eTF_3DC:
 		return DXGI_FORMAT_BC5_UNORM;
 	case eTF_DEPTH16:
 		return DXGI_FORMAT_D16_UNORM;
 	case eTF_D24S8:
 		return DXGI_FORMAT_D24_UNORM_S8_UINT;
 	case eTF_D32F:
 		return DXGI_FORMAT_D32_FLOAT;
 
 	default:
 		assert(0);
 	}
 	return DXGI_FORMAT_UNKNOWN;
 #endif
 }
 
 D3DFormat CTexture::GetD3DLinFormat(D3DFormat nFormat)
 {
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 	return nFormat;
 }
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 D3DFormat CTexture::ConvertToSRGBFmt(D3DFormat fmt)
 {
 #if defined(DIRECT3D10) && !defined(PS3)
 	switch (fmt)
 	{
 	case DXGI_FORMAT_R8G8B8A8_UNORM:
 		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
 	case DXGI_FORMAT_BC1_UNORM:
 		return DXGI_FORMAT_BC1_UNORM_SRGB;
 	case DXGI_FORMAT_BC2_UNORM:
 		return DXGI_FORMAT_BC2_UNORM_SRGB;
 	case DXGI_FORMAT_BC3_UNORM:
 		return DXGI_FORMAT_BC3_UNORM_SRGB;
 		// AntonK: we don't need sRGB space for fp formats, because there is enough precision
 	case DXGI_FORMAT_R16G16B16A16_FLOAT:
 		return DXGI_FORMAT_R16G16B16A16_FLOAT;
 	default:
 		assert(0);
 		return fmt;
 	}
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 	#else
	return fmt;
 #endif
 }
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 #if defined (DIRECT3D10)
 ETEX_Format CTexture::TexFormatFromDeviceFormat(D3DFormat nFormat)
 {
 	switch (nFormat)
 	{
 		case DXGI_FORMAT_R8G8B8A8_UNORM:
 			return eTF_A8R8G8B8;
 		case DXGI_FORMAT_A8_UNORM:
 			return eTF_A8;
 		case DXGI_FORMAT_B5G6R5_UNORM:
 			return eTF_R5G6B5;
 		case DXGI_FORMAT_B5G5R5A1_UNORM:
 			return eTF_R5G5B5;
 		case DXGI_FORMAT_R8G8_UNORM:
 			return eTF_V8U8;
 		case DXGI_FORMAT_R8G8B8A8_SNORM:
 			return eTF_X8L8V8U8;
 		case DXGI_FORMAT_R16G16_SNORM:
 			return eTF_V16U16;
 		case DXGI_FORMAT_R16G16B16A16_UNORM:
 			return eTF_A16B16G16R16;
 		case DXGI_FORMAT_R16G16B16A16_FLOAT:
 			return eTF_A16B16G16R16F;
 		case DXGI_FORMAT_R32G32B32A32_FLOAT:
 			return eTF_A32B32G32R32F;
 		case DXGI_FORMAT_R32_FLOAT:
 		case DXGI_FORMAT_R32_TYPELESS:
 			return eTF_R32F;
 		case DXGI_FORMAT_R16_TYPELESS:
 		case DXGI_FORMAT_R16_FLOAT:
 			return eTF_R16F;
 		case DXGI_FORMAT_R16G16_FLOAT:
 			return eTF_G16R16F;
 		case DXGI_FORMAT_BC1_UNORM:
 			return eTF_DXT1;
 		case DXGI_FORMAT_BC2_UNORM:
 			return eTF_DXT3;
 		case DXGI_FORMAT_BC3_UNORM:
 			return eTF_DXT5;
 		case DXGI_FORMAT_BC5_UNORM:
 			return eTF_3DC;
 		case DXGI_FORMAT_D16_UNORM:
 			return eTF_DEPTH16;
 		case DXGI_FORMAT_R24G8_TYPELESS:
 		case DXGI_FORMAT_D24_UNORM_S8_UINT:
 			return eTF_D24S8;
 		case DXGI_FORMAT_D32_FLOAT:
 			return eTF_D32F;
 	default:
 		assert(0);
 	}
 	return eTF_Unknown;
 }
 
 D3DFormat CTexture::ConvertToDepthStencilFmt( D3DFormat nFormat )
 {
 	switch (nFormat)
 	{
 	case (DXGI_FORMAT_R24G8_TYPELESS):    return DXGI_FORMAT_D24_UNORM_S8_UINT;
 	case (DXGI_FORMAT_R32G8X24_TYPELESS): return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
 		//case (DXGI_FORMAT_R32G32_TYPELESS):   return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
 	case (DXGI_FORMAT_R32_TYPELESS):      return DXGI_FORMAT_D32_FLOAT;
 	case (DXGI_FORMAT_R16_TYPELESS):      return DXGI_FORMAT_D16_UNORM;
 	default: break;
 	}
 	assert( 0 );
 	return DXGI_FORMAT_UNKNOWN;
 }
 
 D3DFormat CTexture::ConvertToShaderResourceFmt(D3DFormat nFormat)
 {
 	//handle special cases
 	switch (nFormat)
 	{
 	case (DXGI_FORMAT_R24G8_TYPELESS):    return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
 	case (DXGI_FORMAT_R32G8X24_TYPELESS): return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
 		//case (DXGI_FORMAT_R32G32_TYPELESS):   return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
 	case (DXGI_FORMAT_R32_TYPELESS):      return DXGI_FORMAT_R32_FLOAT;
 	case (DXGI_FORMAT_R16_TYPELESS):      return DXGI_FORMAT_R16_UNORM;
 
 		/*
 		//128 bits
 		DXGI_FORMAT_R32G32B32A32_TYPELESS,
 		DXGI_FORMAT_R32G32B32_TYPELESS,
 
 		//64 bits
 		DXGI_FORMAT_R16G16B16A16_TYPELESS,
 		DXGI_FORMAT_R32G32_TYPELESS,
 		DXGI_FORMAT_R32G8X24_TYPELESS,
 		DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
 		DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,
 
 		//32 bits
 		DXGI_FORMAT_R10G10B10A2_TYPELESS,
 		DXGI_FORMAT_R8G8B8A8_TYPELESS,
 		DXGI_FORMAT_R16G16_TYPELESS,
 		DXGI_FORMAT_R32_TYPELESS,
 		DXGI_FORMAT_R24G8_TYPELESS,
 		DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
 		DXGI_FORMAT_X24_TYPELESS_G8_UINT,
 
 		//16 bits
 		DXGI_FORMAT_R8G8_TYPELESS,
 		DXGI_FORMAT_R16_TYPELESS,
 
 		//8 bits
 		DXGI_FORMAT_R8_TYPELESS,
 
 		//block formats
 		DXGI_FORMAT_BC1_TYPELESS,
 		DXGI_FORMAT_BC2_TYPELESS,
 		DXGI_FORMAT_BC3_TYPELESS,
 		DXGI_FORMAT_BC4_TYPELESS,
 		DXGI_FORMAT_BC5_TYPELESS,
 		*/
 
 	default: break;
 	}
 
 	//pass through
 	return nFormat;
 }
 
 
 
 D3DFormat CTexture::ConvertToTypelessFmt(D3DFormat fmt)
 {
   switch (fmt)
   {
     case DXGI_FORMAT_R32G32B32A32_FLOAT:
     case DXGI_FORMAT_R32G32B32A32_UINT:
     case DXGI_FORMAT_R32G32B32A32_SINT:
       return DXGI_FORMAT_R32G32B32A32_TYPELESS; break;
 
     case DXGI_FORMAT_R32G32B32_FLOAT:
     case DXGI_FORMAT_R32G32B32_UINT:
     case DXGI_FORMAT_R32G32B32_SINT:
       return DXGI_FORMAT_R32G32B32_TYPELESS; break;
 
     case DXGI_FORMAT_R16G16B16A16_FLOAT:
     case DXGI_FORMAT_R16G16B16A16_UNORM:
     case DXGI_FORMAT_R16G16B16A16_UINT:
     case DXGI_FORMAT_R16G16B16A16_SNORM:
     case DXGI_FORMAT_R16G16B16A16_SINT:
       return DXGI_FORMAT_R16G16B16A16_TYPELESS; break;
 
     case DXGI_FORMAT_R32G32_FLOAT:
     case DXGI_FORMAT_R32G32_UINT:
     case DXGI_FORMAT_R32G32_SINT:
       return DXGI_FORMAT_R32G32_TYPELESS; break;
 
     case DXGI_FORMAT_R10G10B10A2_UNORM:
     case DXGI_FORMAT_R10G10B10A2_UINT:
       return DXGI_FORMAT_R10G10B10A2_TYPELESS; break;
 
     case DXGI_FORMAT_R8G8B8A8_UNORM:
     case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
     case DXGI_FORMAT_R8G8B8A8_UINT:
     case DXGI_FORMAT_R8G8B8A8_SNORM:
     case DXGI_FORMAT_R8G8B8A8_SINT:
       return DXGI_FORMAT_R8G8B8A8_TYPELESS; break;
 
     case DXGI_FORMAT_BC1_UNORM:
     case DXGI_FORMAT_BC1_UNORM_SRGB:
       return DXGI_FORMAT_BC1_TYPELESS; break;
 
     case DXGI_FORMAT_BC2_UNORM:
     case DXGI_FORMAT_BC2_UNORM_SRGB:
       return DXGI_FORMAT_BC2_TYPELESS; break;
 
     case DXGI_FORMAT_BC3_UNORM:
     case DXGI_FORMAT_BC3_UNORM_SRGB:
       return DXGI_FORMAT_BC3_TYPELESS; break;
 
       // todo: add missing formats if they found required
 
   default:
     assert(0);
     return fmt;
   }
 }
 
 #else
 ETEX_Format CTexture::TexFormatFromDeviceFormat(D3DFormat nFormat)
 {
 	switch (nFormat)
 	{
 #if !defined(XENON)
 		case D3DFMT_R8G8B8:
 			return eTF_R8G8B8;
 #endif
 		case D3DFMT_A8R8G8B8:
 			return eTF_A8R8G8B8;
 		case D3DFMT_X8R8G8B8:
 			return eTF_X8R8G8B8;
 		case D3DFMT_A8:
 			return eTF_A8;
 
 
 
 
 
 
 
 
 
 
 
 
 
 		case D3DFMT_A8L8:
 			return eTF_A8L8;
 		case D3DFMT_L8:
 			return eTF_L8;
 		case D3DFMT_A4R4G4B4:
 			return eTF_A4R4G4B4;
 		case D3DFMT_R5G6B5:
 			return eTF_R5G6B5;
 		case D3DFMT_X1R5G5B5:
 			return eTF_R5G5B5;
 		case D3DFMT_V8U8:
 			return eTF_V8U8;
 #if !defined(XENON)
 		case D3DFMT_CxV8U8:
 			return eTF_CxV8U8;
 #endif
 		case D3DFMT_X8L8V8U8:
 			return eTF_X8L8V8U8;
 		case D3DFMT_V16U16:
 			return eTF_V16U16;
 		case D3DFMT_A16B16G16R16:
 			return eTF_A16B16G16R16;
 		case D3DFMT_A16B16G16R16F:
 			return eTF_A16B16G16R16F;
 		case D3DFMT_A32B32G32R32F:
 			return eTF_A32B32G32R32F;
 		case D3DFMT_R32F:
 			return eTF_R32F;
 		case D3DFMT_R16F:
 			return eTF_R16F;
 		case D3DFMT_G16R16:
 			return eTF_G16R16;
 		case D3DFMT_G16R16F:
 			return eTF_G16R16F;
 		case D3DFMT_DXT1:
 			return eTF_DXT1;
 		case D3DFMT_DXT3:
 			return eTF_DXT3;
 		case D3DFMT_DXT5:
 			return eTF_DXT5;
 #if !defined(PS3)
 		case D3DFMT_3DC:
 			return eTF_3DC;
 #endif
 		case D3DFMT_D24X8:
 			return eTF_DEPTH24;
 		case D3DFMT_DF16:
 			return eTF_DF16;
 		case D3DFMT_DF24:
 			return eTF_DF24;
 		case D3DFMT_D16:
 			return eTF_D16;
 		case D3DFMT_D24S8:
 			return eTF_D24S8;
 		case D3DFMT_NULL:
 			return eTF_NULL;
 
 			// sRGB formats
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 		default:
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 			break;
 	}
 	assert(0);
 	return eTF_Unknown;
 }
 
 D3DFormat CTexture::ConvertToDepthStencilFmt( D3DFormat nFormat )
 {
 	assert(0);
 	return (D3DFormat)0;
 }
 
 D3DFormat CTexture::ConvertToShaderResourceFmt(D3DFormat nFormat)
 {
 	assert(0);
 	return (D3DFormat)0;
 }
 
 #endif
 bool CTexture::IsFormatSupported(ETEX_Format eTFDst)
 {
 	CD3D9Renderer *rd = gcpRendD3D;
 	int D3DFmt = DeviceFormatFromTexFormat(eTFDst);
 	if (!D3DFmt)
 		return false;
 	SPixFormat *pFmt;
 	for (pFmt=rd->m_FirstPixelFormat; pFmt; pFmt=pFmt->Next)
 	{
 		if (pFmt->DeviceFormat == D3DFmt && pFmt->IsValid())
 			return true;
 	}
 	return false;
 }
 
 ETEX_Format CTexture::ClosestFormatSupported(ETEX_Format eTFDst)
 {
 	CD3D9Renderer *rd = gcpRendD3D;
 
 	switch (eTFDst)
 	{
 	case eTF_A8R8G8B8:
 		if (rd->m_FormatA8R8G8B8.BitsPerPixel)
 		{
 			m_pPixelFormat = &rd->m_FormatA8R8G8B8;
 			return eTF_A8R8G8B8;
 		}
 		if (rd->m_FormatA4R4G4B4.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA4R4G4B4;
 			return eTF_A4R4G4B4;
 		}
 		return eTF_Unknown;
 
 	case eTF_X8R8G8B8:
 		if (rd->m_FormatX8R8G8B8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatX8R8G8B8;
 			return eTF_X8R8G8B8;
 		}
 		if (rd->m_FormatA8R8G8B8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA8R8G8B8;
 			return eTF_A8R8G8B8;
 		}
 		return eTF_Unknown;
 
 	case eTF_A4R4G4B4:
 		if (rd->m_FormatA4R4G4B4.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA4R4G4B4;
 			return eTF_A4R4G4B4;
 		}
 		if (rd->m_FormatA8R8G8B8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA8R8G8B8;
 			return eTF_A8R8G8B8;
 		}
 		return eTF_Unknown;
 
 	case eTF_R5G6B5:
 		if (rd->m_FormatR5G6B5.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatR5G6B5;
 			return eTF_R5G6B5;
 		}
 		if (rd->m_FormatX8R8G8B8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatX8R8G8B8;
 			return eTF_X8R8G8B8;
 		}
 		return eTF_Unknown;
 
 	case eTF_A8:
 		if (rd->m_FormatA8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA8;
 			return eTF_A8;
 		}
 		if (rd->m_FormatA8L8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA8L8;
 			return eTF_A8L8;
 		}
 		return eTF_Unknown;
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 	case eTF_L8:
 		if (rd->m_FormatL8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatL8;
 			return eTF_L8;
 		}
 		if (rd->m_FormatA8L8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA8L8;
 			return eTF_A8L8;
 		}
 		return eTF_Unknown;
 
 	case eTF_A8L8:
 		if (rd->m_FormatA8L8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA8L8;
 			return eTF_A8L8;
 		}
 		return eTF_Unknown;
 
 	case eTF_DXT1:
 		if (rd->m_FormatDXT1.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatDXT1;
 			return eTF_DXT1;
 		}
 		if (rd->m_FormatX8R8G8B8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatX8R8G8B8;
 			return eTF_X8R8G8B8;
 		}
 		if (rd->m_FormatA8R8G8B8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA8R8G8B8;
 			return eTF_A8R8G8B8;
 		}
 		return eTF_Unknown;
 
 	case eTF_DXT3:
 		if (rd->m_FormatDXT3.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatDXT3;
 			return eTF_DXT3;
 		}
 		if (rd->m_FormatA8R8G8B8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA8R8G8B8;
 			return eTF_A8R8G8B8;
 		}
 		if (rd->m_FormatA4R4G4B4.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA4R4G4B4;
 			return eTF_A4R4G4B4;
 		}
 		return eTF_Unknown;
 
 	case eTF_DXT5:
 		if (rd->m_FormatDXT5.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatDXT5;
 			return eTF_DXT5;
 		}
 		if (rd->m_FormatA8R8G8B8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA8R8G8B8;
 			return eTF_A8R8G8B8;
 		}
 		if (rd->m_FormatA4R4G4B4.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA4R4G4B4;
 			return eTF_A4R4G4B4;
 		}
 		return eTF_Unknown;
 
 	case eTF_3DC:
 		if (rd->DoCompressedNormalmapEmulation())
 		{
 			return eTF_Unknown;																// compressed normal map xy stored in ba, z reconstructed in shader 
 		}
 		else if (rd->m_Format3Dc.IsValid())
 		{
 			m_pPixelFormat = &rd->m_Format3Dc;
 			return eTF_3DC;
 		}
 		return eTF_Unknown;
 
 	case eTF_A16B16G16R16F:
 		if (rd->m_FormatA16B16G16R16F.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA16B16G16R16F;
 			return eTF_A16B16G16R16F;
 		}
 		return eTF_Unknown;
 
   case eTF_A16B16G16R16:
     if (rd->m_FormatA16B16G16R16.IsValid())
     {
       m_pPixelFormat = &rd->m_FormatA16B16G16R16;
       return eTF_A16B16G16R16;
     }
     return eTF_Unknown;
 
   case eTF_A2R10G10B10:
     if (rd->m_FormatA2R10G10B10.IsValid())
     {
       m_pPixelFormat = &rd->m_FormatA2R10G10B10;
       return eTF_A2R10G10B10;
     }
     return eTF_Unknown;
 
 	case eTF_G16R16:
 		if (rd->m_FormatG16R16.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatG16R16;
 			return eTF_G16R16;
 		}
 		return eTF_Unknown;
 
 	case eTF_G16R16F:
 		if (rd->m_FormatG16R16F.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatG16R16F;
 			return eTF_G16R16F;
 		}
 		return eTF_Unknown;
 
 	case eTF_V16U16:
 		if (rd->m_FormatV16U16.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatV16U16;
 			return eTF_V16U16;
 		}
 		return eTF_Unknown;
 
 	case eTF_A32B32G32R32F:
 		if (rd->m_FormatA32B32G32R32F.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatA32B32G32R32F;
 			return eTF_A32B32G32R32F;
 		}
 		return eTF_Unknown;
 
 	case eTF_R32F:
 		if (rd->m_FormatR32F.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatR32F;
 			return eTF_R32F;
 		}
 		if (rd->m_FormatG16R16F.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatG16R16F;
 			return eTF_G16R16F;
 		}
 		return eTF_Unknown;
 
 	case eTF_R16F:
 		if (rd->m_FormatR16F.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatR16F;
 			return eTF_R16F;
 		}
 		if (rd->m_FormatG16R16F.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatG16R16F;
 			return eTF_G16R16F;
 		}
 		return eTF_Unknown;
 
 	case eTF_DEPTH24:
 		if (rd->m_FormatDepth24.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatDepth24;
 			return eTF_DEPTH24;
 		}
 		if (rd->m_FormatDepth16.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatDepth16;
 			return eTF_DEPTH16;
 		}
 		return eTF_Unknown;
 
 	case eTF_DEPTH16:
 		if (rd->m_FormatDepth16.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatDepth16;
 			return eTF_DEPTH16;
 		}
 		if (rd->m_FormatDepth24.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatDepth24;
 			return eTF_DEPTH24;
 		}
 		return eTF_Unknown;
 
 	case eTF_DF16:
 		if (rd->m_FormatDF16.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatDF16;
 			return eTF_DF16;
 		}
 		return eTF_Unknown;
 	case eTF_DF24:
 		if (rd->m_FormatDF24.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatDF24;
 			return eTF_DF24;
 		}
 	case eTF_D16:
 		if (rd->m_FormatD16.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatD16;
 			return eTF_D16;
 		}
 	case eTF_D24S8:
 		if (rd->m_FormatD24S8.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatD24S8;
 			return eTF_D24S8;
 		}
 	case eTF_D32F:
 		if (rd->m_FormatD32F.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatD32F;
 			return eTF_D32F;
 		}
 	case eTF_NULL:
 		if (rd->m_FormatNULL.IsValid())
 		{
 			m_pPixelFormat = &rd->m_FormatNULL;
 			return eTF_NULL;
 		}
 		return eTF_Unknown;
 
 	default:
 		assert(0);
 	}
 	return eTF_Unknown;
 }
 
 bool CTexture::CreateRenderTarget(ETEX_Format eTF)
 {
 	if (eTF == eTF_Unknown)
 		eTF = m_eTFDst;
 	if (eTF == eTF_Unknown)
 		return false;
 	byte *pData[6];
 	for (int i=0; i<6; i++)
 	{
 		pData[i] = NULL;
 	}
 	ETEX_Format eTFDst = ClosestFormatSupported(eTF);
 	if (eTF != eTFDst)
 		return false;
 	m_eTFDst = eTF;
 	m_nFlags |= FT_USAGE_RENDERTARGET;
 	bool bRes = CreateDeviceTexture(pData);
 	PostCreate();
 
 	return bRes;
 }
 
 
 
 
 
 
 
 
 // Resolve anti-aliased RT to the texture
 bool CTexture::Resolve(int nTarget, bool bUseViewportSize)
 {
 	if (m_bResolved)
 		return true;
 
 	m_bResolved = true;
 #if !defined(XENON)
 	if (!(m_nFlags & FT_USAGE_FSAA))
 		return true;
 #endif
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 #if defined (DIRECT3D9) || defined (OPENGL)
 	assert ((m_nFlags & FT_USAGE_RENDERTARGET) && (m_nFlags & FT_USAGE_FSAA) && m_pDeviceRTV && m_pDevTexture);
 	D3DSurface *pDestSurf = GetSurface(-1, 0);
 	D3DSurface *pSrcSurf = (D3DSurface *)m_pDeviceRTV;
 
 	RECT pSrcRect={0, 0, m_nWidth, m_nHeight};
 	RECT pDstRect={0, 0, m_nWidth, m_nHeight};
 
 	gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_RTCopied++;
 	gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_RTCopiedSize += GetDeviceDataSize();
 	gcpRendD3D->m_pd3dDevice->StretchRect(pSrcSurf, &pSrcRect, pDestSurf, &pDstRect, D3DTEXF_NONE); 
 	SAFE_RELEASE(pDestSurf);
 #elif defined (DIRECT3D10)
 	assert ((m_nFlags & FT_USAGE_RENDERTARGET) && (m_nFlags & FT_USAGE_FSAA) && m_pDeviceShaderResource && m_pDevTexture && m_pDeviceTextureMSAA && m_pDeviceShaderResourceMS);
 	CDeviceTexture *pDestSurf = GetDevTexture();
 	CDeviceTexture *pSrcSurf =  GetDevTextureMSAA();
 
 	assert(pSrcSurf!=NULL);
 	assert(pDestSurf!=NULL);
 
 	gcpRendD3D->m_pd3dDeviceContext->ResolveSubresource(pDestSurf->Get2DTexture(), 0, pSrcSurf->Get2DTexture(), 0, (DXGI_FORMAT)m_pPixelFormat->DeviceFormat); 
 #endif
 	return true;
 }
 
 
 
 
 bool CTexture::CreateDeviceTexture(byte *pData[6])
 {
 	if (!m_pPixelFormat)
 	{
 		ETEX_Format eTF = ClosestFormatSupported(m_eTFDst);
 
 		//PS3workaround as long as there is no dedicated texture-pack with pre-converted
 		//3dc->dxt textures, we're converting them on-load
 		//|| defined(PS3)
 #if defined(_XBOX)  || defined(LINUX)
 		if (eTF == eTF_Unknown)
 			return false;
 #endif
 
 		assert(eTF != eTF_Unknown);
 		assert(eTF == m_eTFDst);
 	}
 	assert(m_pPixelFormat);
 	if (!m_pPixelFormat)
 		return false;
 
 	return gRenDev->m_pRT->RC_CreateDeviceTexture(this, pData);
 }
 
 bool CTexture::RT_CreateDeviceTexture(byte *pData[6])
 {
 	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Texture, 0, "%s", this->GetSourceName() ? this->GetSourceName() : "Unknown texture");
 
 	HRESULT hr;
 
 	ReleaseDeviceTexture(false);
 
 	CD3D9Renderer *r = gcpRendD3D;
 	int i;
 	int nWdt = m_nWidth;
 	int nHgt = m_nHeight;
 	int nDepth = m_nDepth;
 	int nMips = m_nMips;
 	assert(nWdt && nHgt && nMips);
 
 	byte *pTemp = NULL;
 
   CDeviceManager *pDevMan = &r->m_DevMan;
 
 	bool resetSRGB = true;
 
 	if(m_nFlags & FT_USAGE_VERTEX_BUFFER)
 	{
 		if(!(gcpRendD3D->GetFeatures() & RFT_HW_R2VB))
 		{
 			assert(0);
 			iLog->Log("Error: CTexture::CreateDeviceTexture: hw doesn't support r2vb, failed to create the texture %s", GetSourceName());
 			return false;
 		}
 		else if(m_eTT != eTT_2D)
 		{
 			assert(0);
 			iLog->Log("Error: CTexture::CreateDeviceTexture: only 2D textures supported for r2vb, failed to create the texture %s", GetSourceName());
 			return false;
 		}
 		else if(m_nMips != 1)
 		{
 			assert(0);
 			iLog->Log("Error: CTexture::CreateDeviceTexture: r2vb doesn't support textures with mips, failed to create the texture %s", GetSourceName());
 			return false;
 		}
 		else if(IsDXTCompressed(m_eTFDst))
 		{
 			assert(0);
 			iLog->Log("Error: CTexture::CreateDeviceTexture: r2vb doesn't support compressed textures, failed to create the texture %s", GetSourceName());
 			return false;
 		}
 	}
 
 	uint32 nUsage = 0;
 
 #if defined (DIRECT3D9) || defined (OPENGL)
 	int D3DUsage = 0;
 	D3DPOOL D3DPool = TEXPOOL;
 	STexLock lr;
 
 	if (m_nFlags & FT_USAGE_RENDERTARGET)
 	{
 		D3DUsage |= D3DUSAGE_RENDERTARGET;
 		D3DPool = D3DPOOL_DEFAULT;
 	}
 	if (m_nFlags & FT_USAGE_DYNAMIC)
 	{
 		D3DUsage |= D3DUSAGE_DYNAMIC;
 		D3DPool = D3DPOOL_DEFAULT;
 	}
 
 	if( m_nFlags & FT_USAGE_READBACK )
 	{
 		//D3DUsage |= D3DUSAGE_DYNAMIC;
 		D3DPool = D3DPOOL_SYSTEMMEM;
 	}
 
 	/*if (m_eTFDst == eTF_DEPTH16 || m_eTFDst == eTF_DEPTH24)
 	{
 		D3DUsage |= D3DUSAGE_DEPTHSTENCIL;
 		D3DUsage &= ~D3DUSAGE_RENDERTARGET;
 		D3DPool = D3DPOOL_DEFAULT;
 	}*/
 	// for DX9 ATI R2VB feature
 #if !defined(XENON)
 	if(m_nFlags & FT_USAGE_VERTEX_BUFFER)
 		D3DUsage |= D3DUSAGE_DMAP;
 #endif
 	if (nMips <= 1 && (m_nFlags & FT_FORCE_MIPS))
 	{
 		if (m_pPixelFormat->bCanAutoGenMips)
 		{
 			D3DUsage |= D3DUSAGE_AUTOGENMIPMAP;
 			nMips = 1;
 		}
 		else
 		if (pData[0])
 		{
 			CDeviceTexture *pSrcTexture = NULL;
 			if(FAILED(hr = pDevMan->Create2DTexture(nWdt, nHgt, D3DX_DEFAULT, nUsage, m_pPixelFormat->DeviceFormat, D3DPOOL_SYSTEMMEM, &pSrcTexture)))
 			{
 				m_nFlags &= ~FT_FORCE_MIPS;
 				nMips = 1;
 				assert(0);
 			}
 			else
 			{
 				int nSize = TextureDataSize(nWdt, nHgt, 1, 1, m_eTFDst);
 				hr = pSrcTexture->LockRect(0, lr, 0);
 				assert(SUCCEEDED(hr));
 				// Copy data to src texture
 				memcpy((byte *)lr.pData, pData[0], nSize);
 				// Unlock the system texture
 				pSrcTexture->UnlockRect(0);
 				hr = D3DXFilterTexture(pSrcTexture->Get2DTexture(), NULL, 0, D3DX_FILTER_LINEAR);
 				assert(SUCCEEDED(hr));
 				nMips = CalcNumMips(nWdt, nHgt);
 				nSize = TextureDataSize(nWdt, nHgt, 1, nMips, m_eTFDst);
 				pTemp = new byte[nSize];
 				int n = pSrcTexture->Get2DTexture()->GetLevelCount();
 				int w = nWdt;
 				int h = nHgt;
 				int nOffs = 0;
 				i = 0;
 				while (w || h)
 				{
 					if (!w)
 						w = 1;
 					if (!h)
 						h = 1;
 					nSize = TextureDataSize(w, h, 1, 1, m_eTFDst);
 					hr = pSrcTexture->LockRect(i, lr, 0);
 					assert(SUCCEEDED(hr));
 					// Copy data to src texture
 					cryMemcpy(&pTemp[nOffs], (byte *)lr.pData, nSize);
 					// Unlock the system texture
 					hr = pSrcTexture->UnlockRect(i);
 					assert(SUCCEEDED(hr));
 					w >>= 1;
 					h >>= 1;
 					nOffs  += nSize;
 					i++;
 				}
 				assert(i == nMips);
 				m_nMips = nMips;
 				pData[0] = pTemp;
 				SAFE_RELEASE(pSrcTexture);
 			}
 		}
 	}
 	assert(nWdt && nHgt && nMips);
 
 	D3DSurface *pSurf = NULL;
   CDeviceTexture *pDevTexture = NULL;
 
 	D3DTEXTUREFILTERTYPE MipFilter = D3DTEXF_LINEAR;
 	D3DFormat D3DFmt = m_pPixelFormat->DeviceFormat;
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 	// Create the video texture
 	if (m_eTT == eTT_2D)
 	{
 		if (m_eTFDst == eTF_DF16 || m_eTFDst == eTF_DF24 || m_eTFDst == eTF_D16 || m_eTFDst == eTF_D24S8)
 		{
 			//FIX: should pool be changed to the D3DPool
 			if (FAILED(hr=pDevMan->Create2DTexture (nWdt, nHgt, 1, D3DUSAGE_DEPTHSTENCIL, D3DFmt, D3DPOOL_DEFAULT, &pDevTexture)))
 			{
 				iLog->Log("Error: CTexture::CreateDeviceTexture: failed to create the texture %s (%s)", GetSourceName(), r->D3DError(hr));
 				return false;
 			}
 		}
 		else
 		//if (!(m_eTFDst == eTF_3DC) || !r->DoCompressedNormalmapEmulation())
 		if (m_eTFDst != eTF_3DC)
 		{
 			resetSRGB = false;
 			m_bIsSRGB &= m_pPixelFormat->bCanReadSRGB && (m_nFlags & (FT_USAGE_FSAA | FT_USAGE_RENDERTARGET)) == 0;
 #if !defined(XENON)
 			if (FAILED(hr=pDevMan->Create2DTexture(nWdt, nHgt, nMips, D3DUsage, D3DFmt, D3DPool, &pDevTexture)))
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 #endif
 			{
 			  iLog->Log("Error: CTexture::CreateDeviceTexture: failed to create the texture %s (%s)", GetSourceName(), r->D3DError(hr));
 			  assert(0);
 			  return false;
 			}
 
 
 
 
 
 		}
 		else
     if (m_eTFDst == eTF_3DC)
 		{
 #if !defined(XENON) && !defined(PS3)
 			int nM = 0;
 			int w = nWdt;
 			int h = nHgt;
 			if (w < 4 || h < 4)
 			{
 				iLog->Log("Error: 3DC texture '%s' couldn't be loaded due to small size (%dx%d) - skipped", GetName(), w, h);
 				return false;
 			}
 			while (w && h)
 			{
 				if (!w)
 					w = 1;
 				if (!h)
 					h = 1;
 				if (w >= 4 && h >= 4)
 					nM++;
 				w >>= 1;
 				h >>= 1;
 			}
 			nMips = min(nM, nMips);
 #endif
 			if (FAILED(hr=pDevMan->Create2DTexture(nWdt, nHgt, nMips, D3DUsage, D3DFmt, D3DPool, &pDevTexture)))
 			{
 				iLog->Log("Error: CD3D9TexMan::D3DCreateVideoTexture: failed to create the texture %s (%s)", GetSourceName(), r->D3DError(hr));
 				return false;
 			}
 		}
 		m_pDevTexture = pDevTexture;
 
 
 
 
 
 
 
 
 
 
 
 
 
 	}
 	else
   if (m_eTT == eTT_Cube)
 	{
 		if (!(r->m_pd3dCaps->TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP))
 		{
 			nMips = 1;
 			m_nMips = 1;
 		}
 		if (FAILED(hr = pDevMan->CreateCubeTexture(nWdt, nMips, D3DUsage, D3DFmt, D3DPool, &pDevTexture)))
 			return false;
 		m_pDevTexture = pDevTexture;
 	}
 	else
   if (m_eTT == eTT_3D)
 	{
 		if (!(r->m_pd3dCaps->TextureCaps & D3DPTEXTURECAPS_MIPVOLUMEMAP))
 		{
 			nMips = 1;
 			m_nMips = 1;
 		}
 		if (FAILED(hr = pDevMan->CreateVolumeTexture(nWdt, nHgt, nDepth, nMips, D3DUsage, D3DFmt, D3DPool, &pDevTexture)))
 			return false;
 		m_pDevTexture = pDevTexture;
 	}
 	if (m_nFlags & FT_USAGE_FSAA)
 	{
 		assert(m_nFlags & FT_USAGE_RENDERTARGET);
 		hr = gcpRendD3D->GetDevice()->CreateRenderTarget(nWdt, nHgt, D3DFmt, r->m_RP.m_FSAAData.Type, r->m_RP.m_FSAAData.Quality, FALSE, &pSurf, NULL);
 		if (FAILED(hr))
 		{
 			assert(0);
 			iLog->LogError("Error: Cannot create MSAA render-target (MSAA switched off).");
 			_SetVar("r_FSAA", 0);
 		}
 		m_nFSAASamples = r->m_RP.m_FSAAData.Type;
 		m_nFSAAQuality = r->m_RP.m_FSAAData.Quality;
 		m_pDeviceRTV = pSurf;
 		m_bResolved = false;
 	}
 #elif defined (DIRECT3D10)
   STextureInfo TI;
   TI.m_nArraySize = m_nArraySize;
   assert(m_nArraySize<=4);
   D3DFormat D3DFmt = m_pPixelFormat->DeviceFormat;
 	if (m_eTT == eTT_2D)
 	{
     if (m_nFlags & FT_USAGE_FSAA)
     {
       m_nFSAASamples = r->m_RP.m_FSAAData.Type;
       m_nFSAAQuality = r->m_RP.m_FSAAData.Quality;
 
       TI.m_nMSAASamples = m_nFSAASamples;
       TI.m_nMSAAQuality = m_nFSAAQuality;
       hr = pDevMan->Create2DTexture(nWdt, nHgt, nMips, 0, D3DFmt, (D3DPOOL)0, &m_pDeviceTextureMSAA, &TI);
 
       assert(SUCCEEDED(hr));
       m_bResolved = false;
 
       TI.m_nMSAASamples = 1;
       TI.m_nMSAAQuality = 0;
     }
 
 		//nMips = 1;
     DXGI_FORMAT nFormatOrig = D3DFmt;
     DXGI_FORMAT nFormatSRGB = D3DFmt;
     
     resetSRGB = false;
 #if !defined(PS3)		
     if( gRenDev->IsLinearSpaceShadingEnabled() )
     {
 		  m_bIsSRGB &= m_pPixelFormat->bCanReadSRGB && (m_nFlags & (FT_USAGE_FSAA | FT_USAGE_RENDERTARGET)) == 0;
       if ((m_bIsSRGB || m_nFlags & FT_USAGE_ALLOWREADSRGB) && gRenDev->IsLinearSpaceShadingEnabled())
         nFormatSRGB = ConvertToSRGBFmt(D3DFmt);
 
 		  if (m_bIsSRGB)
 			  D3DFmt = nFormatSRGB;
 
       // must use typeless format to allow runtime casting
       if (m_nFlags & FT_USAGE_ALLOWREADSRGB)
         D3DFmt = ConvertToTypelessFmt(D3DFmt);
     }    
 #endif
 
     uint32 nUsage = 0;
 		if (m_nFlags & FT_USAGE_DEPTHSTENCIL)
       nUsage |= USAGE_DEPTH_STENCIL;
     if (m_nFlags & FT_USAGE_RENDERTARGET)
       nUsage |= USAGE_RENDER_TARGET;
     if (m_nFlags & FT_USAGE_DYNAMIC)
       nUsage |= USAGE_DYNAMIC;
 
     if (pData[0])
 		{
 			assert(TI.m_nArraySize==1); //there is no implementation for tex array data
 
 			//if (!IsDXTCompressed(m_eTFSrc))
 			{
 				STextureInfoData InitData[20];
 				int w = nWdt;
 				int h = nHgt;
 				int nOffset = 0;
 				byte *src = pData[0];
 				for (i=0; i<nMips; i++)
 				{
 					if (!w && !h)
 						break;
 					if (!w) w = 1;
 					if (!h) h = 1;
 
 					int nSize = TextureDataSize(w, h, 1, 1, m_eTFSrc);
 					InitData[i].pSysMem = &src[nOffset];
 					if (!IsDXTCompressed(m_eTFSrc))
 						InitData[i].SysMemPitch = TextureDataSize(w, 1, 1, 1, m_eTFSrc);
 					else
 					{
 						int blockSize = (m_eTFSrc == eTF_DXT1 || m_eTFSrc == eTF_CTX1) ? 8 : 16;
 						InitData[i].SysMemPitch = (w + 3) / 4 * blockSize;
 					}
 
 					//ignored
 					InitData[i].SysMemSlicePitch = nSize;
 
 					w >>= 1;
 					h >>= 1;
 					nOffset += nSize;
 				}
 
         TI.m_pData = InitData;
         hr = pDevMan->Create2DTexture(nWdt, nHgt, nMips, nUsage, D3DFmt, (D3DPOOL)0, &m_pDevTexture, &TI);
 			}
 		}
 		else
 		{
 			//hr = dv->CreateTexture2D(&Desc, 0, 0); // validates parameters
 			//assert(hr == S_FALSE);
       hr = pDevMan->Create2DTexture(nWdt, nHgt, nMips, nUsage, D3DFmt, (D3DPOOL)0, &m_pDevTexture, &TI);
 		}
 		if (FAILED(hr))
 		{
 			assert(0);
 			return false;
 		}
 		
     // Restore format
     if (m_nFlags & FT_USAGE_ALLOWREADSRGB)
       D3DFmt = nFormatOrig;
 
     D3DDevice *dv = gcpRendD3D->m_pd3dDevice;
 
 		//////////////////////////////////////////////////////////////////////////
 		//ShaderResourceViews creation 
 		ID3D11ShaderResourceView* pSRVMS = NULL;
 		if (m_nFlags & FT_USAGE_FSAA)
 		{
 			D3D11_SHADER_RESOURCE_VIEW_DESC SRVDescMS;
 			ZeroStruct(SRVDescMS);
       
 			SRVDescMS.Format = ConvertToShaderResourceFmt( D3DFmt );
 			SRVDescMS.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
 			hr = dv->CreateShaderResourceView(m_pDeviceTextureMSAA->Get2DTexture(), &SRVDescMS, &pSRVMS);
 			if (FAILED(hr))
 			{
 				assert(0);
 				return false;
 			}
 		}
 		m_pDeviceShaderResourceMS = pSRVMS;
 
 		ID3D11ShaderResourceView *pSRV = NULL;    
 		ID3D11ShaderResourceView* ppSRVs[4];
 		for( int i = 0; i < 4; ++i ) ppSRVs[i] = NULL;
 
 		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
 		ZeroStruct(SRVDesc);
 		SRVDesc.Format = (DXGI_FORMAT)ConvertToShaderResourceFmt(D3DFmt);
     D3DTexture *pID3DTexture = m_pDevTexture->Get2DTexture();
 
 		if (m_nArraySize > 1)
 		{
 			SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
 			SRVDesc.Texture2DArray.FirstArraySlice = 0;
 			SRVDesc.Texture2DArray.ArraySize = m_nArraySize;
 			SRVDesc.Texture2DArray.MipLevels = nMips;
 			SRVDesc.Texture2DArray.MostDetailedMip = 0;
 
 			// Create the multi-slice shader resource view
 			hr = dv->CreateShaderResourceView(pID3DTexture, &SRVDesc, &pSRV);
 			if (FAILED(hr))
 			{
 				assert(0);
 				return false;
 			}
 			// Create the one-slice shader resource views
 			SRVDesc.Texture2DArray.ArraySize = 1;
 			for( uint32 i = 0; i < m_nArraySize; ++i )
 			{
 				SRVDesc.Texture2DArray.FirstArraySlice = i;
 				hr = dv->CreateShaderResourceView( pID3DTexture, &SRVDesc, &ppSRVs[i]);
 				if (FAILED(hr))
 				{
 					assert(0);
 					return false;
 				}
 			}
 
 			//assign shader resource views
 			m_pDeviceShaderResource = pSRV;
 			for( uint32 i = 0; i < m_nArraySize; ++i )
 			{
 				m_pShaderResourceViews[i] = ppSRVs[i];
 			}
 		}
 		else
 		{
 			SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
 			SRVDesc.Texture2D.MipLevels = nMips;
 			SRVDesc.Texture2D.MostDetailedMip = 0;
 			hr = dv->CreateShaderResourceView(pID3DTexture, &SRVDesc, &pSRV);
 			if (FAILED(hr))
 			{
 				assert(0);
 				return false;
 			}
 			//assign shader resource views
 			m_pDeviceShaderResource = pSRV;
 
 #if !defined(PS3)
       if( gRenDev->IsLinearSpaceShadingEnabled() && m_nFlags & FT_USAGE_ALLOWREADSRGB )
       {
         SRVDesc.Format = (DXGI_FORMAT)ConvertToShaderResourceFmt( nFormatSRGB );
 
         ID3D11ShaderResourceView *pSRVSRGB = NULL;
         hr = dv->CreateShaderResourceView(pID3DTexture, &SRVDesc, &pSRVSRGB);
         if (FAILED(hr))
         {
           assert(0);
           return false;
         }
         //assign shader resource views
         m_pDeviceShaderResourceSRGB = pSRVSRGB;
       }
 #endif
 		}
 
 		//////////////////////////////////////////////////////////////////////////
 		// Vertex buffer creation (R2VB feature for PS3)
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 		//////////////////////////////////////////////////////////////////////////
 		//DepthStencilViews creation
 		ID3D11DepthStencilView* pDSV = NULL;
 		ID3D11DepthStencilView* ppDSVs[4];
 		for( int i = 0; i < 4; ++i ) ppDSVs[i] = NULL;
 
 		if (m_nFlags & FT_USAGE_DEPTHSTENCIL)
 		{
 			D3D11_DEPTH_STENCIL_VIEW_DESC DsvDesc;
 			ZeroStruct(DsvDesc);
 			DsvDesc.Format = (DXGI_FORMAT)ConvertToDepthStencilFmt( D3DFmt );
 
 			if (m_nArraySize > 1)
 			{
 				DsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
 				DsvDesc.Texture2DArray.FirstArraySlice = 0;
 				DsvDesc.Texture2DArray.ArraySize = m_nArraySize;
 				DsvDesc.Texture2DArray.MipSlice = 0;
 
 				//multi-faces render target view
 				hr = dv->CreateDepthStencilView(pID3DTexture, &DsvDesc, &pDSV);
 				if (FAILED(hr) )
 				{
 					assert(0);
 					return false;
 				}
 
 				// Create the one-face render target views
 				DsvDesc.Texture2DArray.ArraySize = 1;
 				for( uint32 i = 0; i < m_nArraySize; ++i )
 				{
 					DsvDesc.Texture2DArray.FirstArraySlice = i;
 					hr = dv->CreateDepthStencilView( pID3DTexture, &DsvDesc, &ppDSVs[i] );
 					if (FAILED(hr) )
 					{
 						assert(0);
 						return false;
 					}
 				}
 
 				//assign depth stencil views
 				m_pDeviceDepthStencilSurf = pDSV;
 				for( uint32 i = 0; i < m_nArraySize; ++i )
 				{
 					m_pDeviceDepthStencilCMSurfs[i] = ppDSVs[i];
 				}
 			}
 			else
 			{
 				DsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
 				DsvDesc.Texture2D.MipSlice = 0;
 				hr = dv->CreateDepthStencilView(pID3DTexture, &DsvDesc, &pDSV);
 				if (FAILED(hr) )
 				{
 					assert(0);
 					return false;
 				}
 				m_pDeviceDepthStencilSurf = pDSV;
 			}
 
 		}
 
 	}
 	else
   if (m_eTT == eTT_Cube)
 	{
     STextureInfo TI;
     TI.m_nArraySize = 6;
     D3DFormat D3DFmt = m_pPixelFormat->DeviceFormat;
     uint32 nUsage = 0;
     if (m_nFlags & FT_USAGE_DEPTHSTENCIL)
       nUsage |= USAGE_DEPTH_STENCIL;
     if (m_nFlags & FT_USAGE_RENDERTARGET)
       nUsage |= USAGE_RENDER_TARGET;
     if (m_nFlags & FT_USAGE_DYNAMIC)
       nUsage |= USAGE_DYNAMIC;
 
     if (m_nFlags & FT_USAGE_FSAA)
     {
       m_nFSAASamples = r->m_RP.m_FSAAData.Type;
       m_nFSAAQuality = r->m_RP.m_FSAAData.Quality;
 
       TI.m_nMSAASamples = m_nFSAASamples;
       TI.m_nMSAAQuality = m_nFSAAQuality;
       hr = pDevMan->CreateCubeTexture(nWdt, nMips, nUsage, D3DFmt, (D3DPOOL)0, &m_pDeviceTextureMSAA, &TI);
 
       assert(SUCCEEDED(hr));
       m_bResolved = false;
 
       TI.m_nMSAASamples = 1;
       TI.m_nMSAAQuality = 0;
     }
 
     if (pData[0])
     {
       STextureInfoData InitData[g_nD3D10MaxSupportedSubres];
 
       for (int nSide=0; nSide<6; nSide++)
       {
         int w = nWdt;
         int h = nHgt;
         int nOffset = 0;
         byte *src = (!(m_nFlags & FT_REPLICATE_TO_ALL_SIDES)) ? pData[nSide] : pData[0];
 
         for (i=0; i<nMips; i++)
         {
           if (!w && !h)
             break;
           if (!w) w = 1;
           if (!h) h = 1;
 
           int nSubresInd = nSide * nMips + i;
           int nSize = TextureDataSize(w, h, 1, 1, m_eTFSrc);
           InitData[nSubresInd].pSysMem = &src[nOffset];
 
           InitData[nSubresInd].SysMemPitch = TextureDataSize(w, 1, 1, 1, m_eTFSrc);
 
           //ignored
           InitData[nSubresInd].SysMemSlicePitch = nSize;
 
           w >>= 1;
           h >>= 1;
           nOffset += nSize;
         }
       }
 
 			TI.m_pData = InitData;
       hr = pDevMan->CreateCubeTexture(nWdt, nMips, nUsage, D3DFmt, (D3DPOOL)0, &m_pDevTexture, &TI);
     }
     else
     {
       //hr = dv->CreateTexture2D(&Desc, 0, 0); // validates parameters
       //assert(hr == S_FALSE);
       hr = pDevMan->CreateCubeTexture(nWdt, nMips, nUsage, D3DFmt, (D3DPOOL)0, &m_pDevTexture, &TI);
     }
 		if (FAILED(hr))
 		{
 			assert(0);
 			return false;
 		}
 		D3DCubeTexture *pID3DTexture = m_pDevTexture->GetCubeTexture();
     D3DDevice *dv = gcpRendD3D->m_pd3dDevice;
 
 		ID3D11ShaderResourceView *pRes = NULL;
 		D3D11_SHADER_RESOURCE_VIEW_DESC ResDesc;
 		ZeroStruct(ResDesc);
 		ResDesc.Format = (DXGI_FORMAT)ConvertToShaderResourceFmt( D3DFmt );
 		ResDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
 		ResDesc.TextureCube.MipLevels = nMips;
 		ResDesc.TextureCube.MostDetailedMip = 0;
 		hr = dv->CreateShaderResourceView(pID3DTexture, &ResDesc, &pRes);
 		if (FAILED(hr))
 		{
 			assert(0);
 			return false;
 		}
 		m_pDeviceShaderResource = pRes;
 
 		//depth-stencil views for cubemaps
 		ID3D11DepthStencilView* pDSV = NULL;
 		ID3D11DepthStencilView* ppDSVs[6];
 
 		for( int i = 0; i < 6; ++i ) ppDSVs[i] = NULL;
 
 		if (m_nFlags & FT_USAGE_DEPTHSTENCIL)
 		{
 			D3D11_DEPTH_STENCIL_VIEW_DESC DsvDesc;
 			ZeroStruct(DsvDesc);
 			DsvDesc.Format = (DXGI_FORMAT)ConvertToDepthStencilFmt( D3DFmt );
 			DsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
 			DsvDesc.Texture2DArray.FirstArraySlice = 0;
 			DsvDesc.Texture2DArray.ArraySize = 6;
 			DsvDesc.Texture2DArray.MipSlice = 0;
 
 			//six-faces render target view
 			hr = dv->CreateDepthStencilView(pID3DTexture, &DsvDesc, &pDSV);
 			if (FAILED(hr) )
 			{
 				assert(0);
 				return false;
 			}
 
 			// Create the one-face render target views
 			DsvDesc.Texture2DArray.ArraySize = 1;
 			DsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
 			for( int i = 0; i < 6; ++i )
 			{
 				DsvDesc.Texture2DArray.FirstArraySlice = i;
 				hr = dv->CreateDepthStencilView( pID3DTexture, &DsvDesc, &ppDSVs[i] );
 				if (FAILED(hr) )
 				{
 					assert(0);
 					return false;
 				}
 			}
 
 		}
 
 		//assign depth stencil views
 		m_pDeviceDepthStencilSurf = pDSV;
 
 		for( int i = 0; i < 6; ++i )
 		{
 			m_pDeviceDepthStencilCMSurfs[i] = ppDSVs[i];
 		}
 
 
 	}
 	else
   if( m_eTT == eTT_3D)
 	{
     STextureInfo TI;
     TI.m_nArraySize = 1;
     D3DFormat D3DFmt = m_pPixelFormat->DeviceFormat;
 
     uint32 nUsage = 0;
     if (m_nFlags & FT_USAGE_DEPTHSTENCIL)
       nUsage |= USAGE_DEPTH_STENCIL;
     if (m_nFlags & FT_USAGE_RENDERTARGET)
       nUsage |= USAGE_RENDER_TARGET;
     if (m_nFlags & FT_USAGE_DYNAMIC)
       nUsage |= USAGE_DYNAMIC;
 
 
     if (m_nFlags & FT_USAGE_FSAA)
     {
       m_nFSAASamples = r->m_RP.m_FSAAData.Type;
       m_nFSAAQuality = r->m_RP.m_FSAAData.Quality;
 
       TI.m_nMSAASamples = m_nFSAASamples;
       TI.m_nMSAAQuality = m_nFSAAQuality;
       hr = pDevMan->CreateVolumeTexture(nWdt, nHgt, m_nDepth, nMips, nUsage, D3DFmt, (D3DPOOL)0, &m_pDeviceTextureMSAA, &TI);
 
       assert(SUCCEEDED(hr));
       m_bResolved = false;
 
       TI.m_nMSAASamples = 1;
       TI.m_nMSAAQuality = 0;
     }
 
     if (pData[0])
     {
       STextureInfoData InitData[15];
 
       int w = nWdt;
       int h = nHgt;
       int d = nDepth;
       int nOffset = 0;
       byte *src = pData[0];
 
       for (i=0; i<nMips; i++)
       {
         if (!w && !h && !d)
           break;
         if (!w) w = 1;
         if (!h) h = 1;
         if (!d) d = 1;
 
 				int nSliceSize = TextureDataSize(w, h, 1, 1, m_eTFSrc);
 				int nMipSize = TextureDataSize(w, h, d, 1, m_eTFSrc);
         InitData[i].pSysMem = &src[nOffset];
         InitData[i].SysMemPitch = TextureDataSize(w, 1, 1, 1, m_eTFSrc);
 
         //ignored
         InitData[i].SysMemSlicePitch = nSliceSize;
 
         w >>= 1;
         h >>= 1;
         d >>= 1;
 
         nOffset += nMipSize;
       }
 
 			TI.m_pData = InitData;
 
       hr = pDevMan->CreateVolumeTexture(nWdt, nHgt, nDepth, nMips, nUsage, D3DFmt, (D3DPOOL)0, &m_pDevTexture, &TI);
     }
     else
     {
       //hr = dv->CreateTexture2D(&Desc, 0, 0); // validates parameters
       //assert(hr == S_FALSE);
       hr = pDevMan->CreateVolumeTexture(nWdt, nHgt, nDepth, nMips, nUsage, D3DFmt, (D3DPOOL)0, &m_pDevTexture, &TI);
     }
 
 		D3DVolumeTexture *pID3DTexture3D = m_pDevTexture->GetVolumeTexture();
     D3DDevice *dv = gcpRendD3D->m_pd3dDevice;
 
 		ID3D11ShaderResourceView *pRes = NULL;
 		D3D11_SHADER_RESOURCE_VIEW_DESC ResDesc;
 		ZeroStruct(ResDesc);
 		ResDesc.Format = D3DFmt;
 		ResDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
 		ResDesc.Texture3D.MipLevels = nMips;
 		ResDesc.Texture3D.MostDetailedMip = 0;
 		hr = dv->CreateShaderResourceView(pID3DTexture3D, &ResDesc, &pRes);
 		if (FAILED(hr))
 		{
 			assert(0);
 			return false;
 		}
 		m_pDeviceShaderResource = pRes;
 
 	}
 	else 
 	{
 		assert(0);
 		return false;
 	}
 #endif
 
 	SetTexStates();
 
 	assert(!IsStreamed());
 	if (m_pDevTexture)
 	{
 		m_nDeviceSize = CTexture::TextureDataSize(nWdt, nHgt, nDepth, nMips, m_eTFDst);
 		m_nDeviceSize *= m_nArraySize;
 		if(m_eTT == eTT_Cube) m_nDeviceSize *= 6;
 #ifndef PS3
 		if (m_nFlags & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET))
 			CryInterlockedAdd(&CTexture::s_nStatsCurDynamicTexMem, m_nDeviceSize);
 		else	
 			CryInterlockedAdd(&CTexture::s_nStatsCurManagedNonStreamedTexMem, m_nDeviceSize);
 #endif
 		AddPhysicalD3DBlock(m_nDeviceSize);
 	}
 
 
 
 
 
 
 	if (!pData[0])
 		return true;
 
 	int nOffset = 0;
 #if defined (DIRECT3D9) || defined (OPENGL)
 	if (m_eTT == eTT_3D)
 	{
     D3DVolumeTexture *pID3DVolTexture = pDevTexture->GetVolumeTexture();
 		// Fill the volume texture
 		D3DLOCKED_BOX LockedBox;
 
 		int w = nWdt;
 		int h = nHgt;
 		int d = nDepth;
 		nOffset = 0;
 		byte *src = pData[0];
 		for (i=0; i<m_nMips; i++)
 		{
 			if (!w && !h && !d)
 				break;
 			hr = pID3DVolTexture->LockBox(i, &LockedBox, 0, 0);
 			assert(SUCCEEDED(hr));
 			if (!w) w = 1;
 			if (!h) h = 1;
 			if (!d) d = 1;
 			const int nSliceSize = TextureDataSize(w, h, 1, 1, m_eTFDst);
 			const int nPitch = TextureDataSize(w, 1, 1, 1, m_eTFDst);
 
 
 
 
 
 
 
 
 
 			for (int r=0; r<d; r++)
 			{
 				byte* pSliceStart = (byte*)LockedBox.pBits;
 				for (int t=0; t<h; t++)
 				{
 					memcpy((byte *)LockedBox.pBits, &src[nOffset], nPitch);
 					LockedBox.pBits = (BYTE*)LockedBox.pBits + LockedBox.RowPitch;
 					nOffset += nPitch;
 				}
 				LockedBox.pBits = pSliceStart + LockedBox.SlicePitch;
 			}
 
 			w >>= 1;
 			h >>= 1;
 			d >>= 1;
 
 			pID3DVolTexture->UnlockBox(i);
 		}
 	}
 	else
 	if (m_eTT == eTT_Cube)
 	{
 		for (int nSide=0; nSide<6; nSide++)
 		{
 			int w = nWdt;
 			int h = nHgt;
 			nOffset = 0;
 			byte *src = pData[nSide];
 			for (i=0; i<m_nMips; i++)
 			{
 				if (!w && !h)
 					break;
 
 				const bool isNative = w > 64 && h > 64 && !(m_nFlags & FT_TEX_WAS_NOT_PRE_TILED);
 
 				if (!w) w = 1;
 				if (!h) h = 1;
 				const int nSize = TextureDataSize(w, h, 1, 1, m_eTFDst);
 
 				if (nSide && !src && (m_nFlags & FT_REPLICATE_TO_ALL_SIDES))
           src = pData[0];
 				hr = pDevTexture->LockRect(nSide, i, lr, 0);
 				assert(SUCCEEDED(hr));
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 				cryMemcpy((byte *)lr.pData, &src[nOffset], nSize); 
 
 				pDevTexture->UnlockRect(nSide, i);
 
         w >>= 1;
 				h >>= 1;
 				nOffset += nSize;
 			}
 		}
 	}
   else
 	if (m_eTT == eTT_2D)
 	{
 		int w = nWdt;
 		int h = nHgt;
 		nOffset = 0;
 		byte *src = pData[0];
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 		for (i=0; i<nMips; i++)
 		{
 			if (!w && !h)
 				break;
 			if (!w) w = 1;
 			if (!h) h = 1;
 			const int nSize = TextureDataSize(w, h, 1, 1, m_eTFDst);
 			bool bLoaded = false;
 			hr = pDevTexture->LockRect(i, lr, 0);
 			assert(SUCCEEDED(hr));
 #ifdef _DEBUG
 			if (w >= 4)
 			{
 				int nD3DSize;
 				if (IsDXTCompressed(GetDstFormat()))
 					nD3DSize = CTexture::TextureDataSize(w, h, 1, 1, GetDstFormat());
 				else
 					nD3DSize = lr.Pitch * h;
 				assert(nD3DSize == nSize);
 			}
 #endif
 			if (w < 4 && !IsDXTCompressed(m_eTFDst))
 			{
 				int nD3DSize = lr.Pitch * h;
 				if (nD3DSize != nSize)
 				{
 					bLoaded = true;
 					byte *pDst = (byte *)lr.pData;
 					byte *pSrc = &src[nOffset];
 					const int nPitchSrc = CTexture::TextureDataSize(w, 1, 1, 1, m_eTFDst);
 					for (int j=0; j<j; j++)
 					{
 						memcpy(pDst, pSrc, nPitchSrc);
 						pSrc += nPitchSrc;
 						pDst += lr.Pitch;
 					}
 				}
 			}
 			if (!bLoaded)
 			{
 				cryMemcpy((byte *)lr.pData, &src[nOffset], nSize);
 			}
 			pDevTexture->UnlockRect(i);
 			w >>= 1;
 			h >>= 1;
 			nOffset += nSize;
 		}
 
 	}
 #endif
 
 	if (resetSRGB)
 		m_bIsSRGB = false;
 
 	SAFE_DELETE_ARRAY(pTemp);
 	return true;
 }
 
 void CTexture::ReleaseDeviceTexture(bool bKeepLastMips)
 {
 	PROFILE_FRAME(CTexture_ReleaseDeviceTexture);
 
   assert (!m_bStreamingInProgress);
 	AbortStreamingTasks(this);
 
 	if (m_pDevTexture)
 	{
 		int nnn = 0;
 	}
 	if (!m_bNoTexture)
 	{
 #if defined (DIRECT3D9)
 		CDeviceTexture *pTex = m_pDevTexture;
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 		SAFE_RELEASE(pTex);
 
 #endif
 
 #if defined (DIRECT3D10)
     SAFE_RELEASE(m_pDevTexture);
 
 		ID3D11Texture2D * pTexMSAA = (ID3D11Texture2D*)m_pDeviceTextureMSAA;
 		SAFE_RELEASE( pTexMSAA );
 		m_pDeviceTextureMSAA = NULL;
 
 		ID3D11ShaderResourceView* pSRV = (ID3D11ShaderResourceView*)m_pDeviceShaderResource;
 		SAFE_RELEASE( pSRV );
 		m_pDeviceShaderResource = NULL;
 
     ID3D11ShaderResourceView* pSRVSRGB = (ID3D11ShaderResourceView*)m_pDeviceShaderResourceSRGB;
     SAFE_RELEASE( pSRVSRGB );
     m_pDeviceShaderResourceSRGB = NULL;
 
 		ID3D11ShaderResourceView* pSRVMS = (ID3D11ShaderResourceView*)m_pDeviceShaderResourceMS;
 		SAFE_RELEASE( pSRVMS );
 		m_pDeviceShaderResourceMS = NULL;
 
 		ID3D11RenderTargetView* pRTV = (ID3D11RenderTargetView*)m_pDeviceRTV;
 		SAFE_RELEASE( pRTV );
 		m_pDeviceRTV = NULL;
 
 #else
 		LPDIRECT3DSURFACE9 pSurf = (LPDIRECT3DSURFACE9)m_pDeviceRTV;
 		if (pSurf)
 		{
 			SAFE_RELEASE(pSurf);
 			m_pDeviceRTV = NULL;
 		}
 #endif
 
 #if defined (DIRECT3D10)
 		ID3D11DepthStencilView* pDSV = (ID3D11DepthStencilView*)m_pDeviceDepthStencilSurf;
 		SAFE_RELEASE( pDSV );
 		m_pDeviceDepthStencilSurf = NULL;
 		for( int i = 0; i < 6; ++i )
 		{
 			pDSV = (ID3D11DepthStencilView*)m_pDeviceDepthStencilCMSurfs[i];
 			SAFE_RELEASE( pDSV );
 			m_pDeviceDepthStencilCMSurfs[i] = NULL;
 		}
 
 		for( int i = 0; i < 4; ++i )
 		{
 			pSRV = (ID3D11ShaderResourceView*)m_pShaderResourceViews[i];
 			SAFE_RELEASE( pSRV );
 			m_pShaderResourceViews[i] = NULL;
 		}
 
 #endif
 
 		m_pDevTexture = NULL;
 		// otherwise it's already taken into account in the m_pPoolItem's dtor
 		if(!m_pPoolItem)
 		{
 			if (IsStreamed())
 				CryInterlockedAdd(CTexture::s_nStatsCurManagedStreamedTexMem.Addr(), -m_nDeviceSize);
 #ifndef PS3
 			else if (IsDynamic())
 				CryInterlockedAdd(&CTexture::s_nStatsCurDynamicTexMem, -m_nDeviceSize);
 			else 
 				CryInterlockedAdd(&CTexture::s_nStatsCurManagedNonStreamedTexMem, -m_nDeviceSize);
 #endif
 		}
 		SAFE_DELETE(m_pPoolItem);
 		int nSides = m_eTT == eTT_Cube ? 6 : 1;
 		if (m_pFileTexMips)
 		{
 			if (bKeepLastMips)
 			{
 				const int nLastMipsStart = m_nMips - 1 - m_CacheFileHeader.m_nMipsPersistent;
 				for (int nS=0; nS<nSides; nS++)
 				{
 					for (int i=0; i<m_nMips; i++)
 					{
 						SMipData *mp = &m_pFileTexMips[i].m_Mips[nS];
 						if (i < nLastMipsStart)
 							mp->Free();
 					}
 				}
 			}
 			else
 			{
 				SAFE_DELETE_ARRAY(m_pFileTexMips);
 				m_bStreamed = false;
 				m_bStreamPrepared = false;
 				m_bWasUnloaded = false;
 				m_bStreamFromDDS = false;
 				m_bStreamingInProgress = false;
 				m_bStreamRequested = false;
 			}
 		}
 		Unlink();
 		m_nDeviceSize = 0;
 	}
 	else
 	{
 		m_pDevTexture = NULL;
 		m_pDeviceRTV = NULL;
 #if defined (DIRECT3D10)
 		m_pDeviceShaderResource = NULL;
     m_pDeviceShaderResourceSRGB = NULL;
 #endif
 	}
 	m_bNoTexture = false;
 	m_nMinMipVidUploaded = 100;
 	m_nFSAASamples = 0;
 	m_nFSAAQuality = 0;
 
 	// release vertex buffer binded to this texture (R2VB feature)
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 }
 
 void CTexture::SetTexStates()
 {
 	s_sDefState = s_sGlobalDefState;
 	if (m_nFlags & FT_FILTER_MASK)
 	{
 		int nFilter = m_nFlags & FT_FILTER_MASK;
 		if (nFilter == FT_FILTER_POINT)
 			SetFilter(FILTER_POINT);
 		else
     if (nFilter == FT_FILTER_LINEAR)
       SetFilter(FILTER_LINEAR);
     else
     if (nFilter == FT_FILTER_BILINEAR)
       SetFilter(FILTER_BILINEAR);
     else
     if (nFilter == FT_FILTER_TRILINEAR)
       SetFilter(FILTER_TRILINEAR);
     else
     if (nFilter == FT_FILTER_ANISO2)
       SetFilter(FILTER_ANISO2X);
     else
     if (nFilter == FT_FILTER_ANISO4)
       SetFilter(FILTER_ANISO4X);
     else
     if (nFilter == FT_FILTER_ANISO8)
       SetFilter(FILTER_ANISO8X);
     else
     if (nFilter == FT_FILTER_ANISO16)
       SetFilter(FILTER_ANISO16X);
 	}
 	if (m_nMips <= 1 && !(m_nFlags & FT_FORCE_MIPS))
 		s_sDefState.m_nMipFilter = D3DTEXF_NONE;
 	if (m_nFlags & FT_STATE_CLAMP || m_eTT == eTT_Cube)
 		SetClampingMode(TADDR_CLAMP, TADDR_CLAMP, TADDR_CLAMP);
 	m_nDefState = CTexture::GetTexState(s_sDefState);
 }
 
 static signed char sAddressMode(int nAddress)
 {
 	signed char nMode = -1;
 	if (nAddress < 0)
 		return nMode;
 #if defined (DIRECT3D9) || defined (OPENGL)
 	switch (nAddress)
 	{
 	case TADDR_WRAP:
 		nMode = D3DTADDRESS_WRAP;
 		break;
 	case TADDR_CLAMP:
 		nMode = D3DTADDRESS_CLAMP;
 		break;
 	case TADDR_BORDER:
 		nMode = D3DTADDRESS_BORDER;
 		break;
 	case TADDR_MIRROR:
 		nMode = D3DTADDRESS_MIRROR;
 		break;
 	default:
 		assert(0);
 		return D3DTADDRESS_WRAP;
 	}
 #elif defined (DIRECT3D10)
 	switch (nAddress)
 	{
 	case TADDR_WRAP:
 		nMode = D3D11_TEXTURE_ADDRESS_WRAP;
 		break;
 	case TADDR_CLAMP:
 		nMode = D3D11_TEXTURE_ADDRESS_CLAMP;
 		break;
 	case TADDR_BORDER:
 		nMode = D3D11_TEXTURE_ADDRESS_BORDER;
 		break;
 	case TADDR_MIRROR:
 		nMode = D3D11_TEXTURE_ADDRESS_MIRROR;
 		break;
 	default:
 		assert(0);
 		return D3D11_TEXTURE_ADDRESS_WRAP;
 	}
 #endif
 	return nMode;
 }
 
 void STexState::SetComparisonFilter(bool bEnable)
 {
 #if defined (DIRECT3D10)
 	if (m_pDeviceState)
 	{
 		ID3D11SamplerState *pSamp = (ID3D11SamplerState *)m_pDeviceState;
 		SAFE_RELEASE(pSamp);
 		m_pDeviceState = NULL;
 	}
 #endif
 	m_bComparison = bEnable;
 }
 
 bool STexState::SetClampMode(int nAddressU, int nAddressV, int nAddressW)
 {
 
 #if defined (DIRECT3D10)
 	if (m_pDeviceState)
 	{
 		ID3D11SamplerState *pSamp = (ID3D11SamplerState *)m_pDeviceState;
 		SAFE_RELEASE(pSamp);
 		m_pDeviceState = NULL;
 	}
 #endif
 
 	m_nAddressU = sAddressMode(nAddressU);
 	m_nAddressV = sAddressMode(nAddressV);
 	m_nAddressW = sAddressMode(nAddressW);
 	if (m_nAddressU < 0)
 		m_nAddressU = CTexture::s_TexStates[CTexture::s_nGlobalDefState].m_nAddressU;
 	if (m_nAddressV < 0)
 		m_nAddressV = CTexture::s_TexStates[CTexture::s_nGlobalDefState].m_nAddressV;
 	if (m_nAddressW < 0)
 		m_nAddressW = CTexture::s_TexStates[CTexture::s_nGlobalDefState].m_nAddressW;
 	return true;
 }
 
 bool STexState::SetFilterMode(int nFilter)
 {
 	if (nFilter < 0)
 	{
 		STexState *pTS = &CTexture::s_TexStates[CTexture::s_nGlobalDefState];
 		m_nMinFilter = pTS->m_nMinFilter;
 		m_nMagFilter = pTS->m_nMagFilter;
 		m_nMipFilter = pTS->m_nMipFilter;
 		return true;
 	}
 
 #if defined (DIRECT3D9) || defined (OPENGL)
 	switch(nFilter)
 	{
 	case FILTER_POINT:
 	case FILTER_NONE:
 		m_nMinFilter = D3DTEXF_POINT;
 		m_nMagFilter = D3DTEXF_POINT;
 		m_nMipFilter = D3DTEXF_NONE;
 		break;
 	case FILTER_LINEAR:
 		m_nMinFilter = D3DTEXF_LINEAR;
 		m_nMagFilter = D3DTEXF_LINEAR;
 		m_nMipFilter = D3DTEXF_NONE;
 		break;
 	case FILTER_BILINEAR:
 		m_nMinFilter = D3DTEXF_LINEAR;
 		m_nMagFilter = D3DTEXF_LINEAR;
 		m_nMipFilter = D3DTEXF_POINT;
 		break;
 	case FILTER_TRILINEAR:
 		m_nMinFilter = D3DTEXF_LINEAR;
 		m_nMagFilter = D3DTEXF_LINEAR;
 		m_nMipFilter = D3DTEXF_LINEAR;
 		break;
 	case FILTER_ANISO2X:
 	case FILTER_ANISO4X:
 	case FILTER_ANISO8X:
 	case FILTER_ANISO16X:
 		if (gcpRendD3D->m_pd3dCaps->TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC)
 			m_nMinFilter = D3DTEXF_ANISOTROPIC;
 		else
 			m_nMinFilter = D3DTEXF_LINEAR;
 		if (gcpRendD3D->m_pd3dCaps->TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC)
 			m_nMagFilter = D3DTEXF_ANISOTROPIC;
 		else
 			m_nMagFilter = D3DTEXF_LINEAR;
 		m_nMipFilter = D3DTEXF_LINEAR;
 		if (nFilter == FILTER_ANISO2X)
 			m_nAnisotropy = min(gcpRendD3D->m_MaxAnisotropyLevel, 2);
 		else
 		if (nFilter == FILTER_ANISO4X)
 			m_nAnisotropy = min(gcpRendD3D->m_MaxAnisotropyLevel, 4);
 		else
 		if (nFilter == FILTER_ANISO8X)
 			m_nAnisotropy = min(gcpRendD3D->m_MaxAnisotropyLevel, 8);
 		else
 		if (nFilter == FILTER_ANISO16X)
 			m_nAnisotropy = min(gcpRendD3D->m_MaxAnisotropyLevel, 16);
 		break;
 	default:
 		assert(0);
 		return false;
 	}
 #elif defined (DIRECT3D10)
 
 	if (m_pDeviceState)
 	{
 		ID3D11SamplerState *pSamp = (ID3D11SamplerState *)m_pDeviceState;
 		SAFE_RELEASE(pSamp);
 		m_pDeviceState = NULL;
 	}
 
 	switch(nFilter)
 	{
 	case FILTER_POINT:
 	case FILTER_NONE:
 		m_nMinFilter = FILTER_POINT;
 		m_nMagFilter = FILTER_POINT;
 		m_nMipFilter = FILTER_NONE;
 		break;
 	case FILTER_LINEAR:
 		m_nMinFilter = FILTER_LINEAR;
 		m_nMagFilter = FILTER_LINEAR;
 		m_nMipFilter = FILTER_NONE;
 		break;
 	case FILTER_BILINEAR:
 		m_nMinFilter = FILTER_LINEAR;
 		m_nMagFilter = FILTER_LINEAR;
 		m_nMipFilter = FILTER_POINT;
 		break;
 	case FILTER_TRILINEAR:
 		m_nMinFilter = FILTER_LINEAR;
 		m_nMagFilter = FILTER_LINEAR;
 		m_nMipFilter = FILTER_LINEAR;
 		break;
 	case FILTER_ANISO2X:
 	case FILTER_ANISO4X:
 	case FILTER_ANISO8X:
 	case FILTER_ANISO16X:
 		m_nMinFilter = nFilter;
 		m_nMagFilter = nFilter;
 		m_nMipFilter = nFilter;
 		if (nFilter == FILTER_ANISO2X)
 			m_nAnisotropy = min(gcpRendD3D->m_MaxAnisotropyLevel, 2);
 		else
 			if (nFilter == FILTER_ANISO4X)
 				m_nAnisotropy = min(gcpRendD3D->m_MaxAnisotropyLevel, 4);
 			else
 				if (nFilter == FILTER_ANISO8X)
 					m_nAnisotropy = min(gcpRendD3D->m_MaxAnisotropyLevel, 8);
 				else
 					if (nFilter == FILTER_ANISO16X)
 						m_nAnisotropy = min(gcpRendD3D->m_MaxAnisotropyLevel, 16);
 		break;
 	default:
 		assert(0);
 		return false;
 	}
 #endif
 	if (CRenderer::CV_r_texturesfilteringquality >= 2 && m_nAnisotropy > 2)
 		m_nAnisotropy = 2;
 	else
 		if (CRenderer::CV_r_texturesfilteringquality == 1 && m_nAnisotropy > 4)
 			m_nAnisotropy = 4;
 	return true;
 }
 
 void STexState::SetBorderColor(DWORD dwColor) 
 { 
 #if defined (DIRECT3D10)
 	if (m_pDeviceState)
 	{
 		ID3D11SamplerState *pSamp = (ID3D11SamplerState *)m_pDeviceState;
 		SAFE_RELEASE(pSamp);
 		m_pDeviceState = NULL;
 	}
 #endif
 
 	m_dwBorderColor = dwColor; 
 }
 
 void STexState::PostCreate()
 {
 #if defined (DIRECT3D10)
 	if (m_pDeviceState)
 		return;
 
 	D3D11_SAMPLER_DESC Desc;
 	ZeroStruct(Desc);
 	ID3D11SamplerState *pSamp = NULL;
 	Desc.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)m_nAddressU;
 	Desc.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)m_nAddressV;
 	Desc.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)m_nAddressW;
 	ColorF col((uint32)m_dwBorderColor);
 	Desc.BorderColor[0] = col.r;
 	Desc.BorderColor[1] = col.g;
 	Desc.BorderColor[2] = col.b;
 	Desc.BorderColor[3] = col.a;
 	if (m_bComparison)
 		Desc.ComparisonFunc = D3D11_COMPARISON_LESS;
 	else
 		Desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
 
 	Desc.MaxAnisotropy = 1;
 	Desc.MinLOD = 0;
   if (m_nMipFilter == FILTER_NONE)
   {
     Desc.MaxLOD = 0.0f;
   }
   else
   {
     Desc.MaxLOD = 100.0f;
   }
 
 	Desc.MipLODBias = 0;
 
 	if (m_bComparison)
 	{
 
 		if (m_nMinFilter == FILTER_LINEAR && m_nMagFilter == FILTER_LINEAR && m_nMipFilter == FILTER_LINEAR)
 		{
 			Desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
 		}
 		else
 			if (m_nMinFilter == FILTER_LINEAR && m_nMagFilter == FILTER_LINEAR && m_nMipFilter == FILTER_NONE)
 				Desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
 			else
 				if (m_nMinFilter == FILTER_LINEAR && m_nMagFilter == FILTER_LINEAR && m_nMipFilter == FILTER_POINT)
 					Desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
 				else
 					if (m_nMinFilter == FILTER_POINT && m_nMagFilter == FILTER_POINT && (m_nMipFilter == FILTER_NONE || m_nMipFilter == FILTER_POINT))
 						Desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
 					else
 						if (m_nMinFilter >= FILTER_ANISO2X && m_nMagFilter >= FILTER_ANISO2X && m_nMipFilter >= FILTER_ANISO2X)
 						{
 							Desc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
 							Desc.MaxAnisotropy = m_nAnisotropy;
 						}
 	}
 	else
 	{
 		if (m_nMinFilter == FILTER_LINEAR && m_nMagFilter == FILTER_LINEAR && m_nMipFilter == FILTER_LINEAR)
 		{
 			Desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
 		}
 		else
 			if (m_nMinFilter == FILTER_LINEAR && m_nMagFilter == FILTER_LINEAR && m_nMipFilter == FILTER_NONE)
 				Desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
 			else
 				if (m_nMinFilter == FILTER_LINEAR && m_nMagFilter == FILTER_LINEAR && m_nMipFilter == FILTER_POINT)
 					Desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
 				else
 					if (m_nMinFilter == FILTER_POINT && m_nMagFilter == FILTER_POINT && (m_nMipFilter == FILTER_NONE || m_nMipFilter == FILTER_POINT))
 						Desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
 					else
 						if (m_nMinFilter >= FILTER_ANISO2X && m_nMagFilter >= FILTER_ANISO2X && m_nMipFilter >= FILTER_ANISO2X)
 						{
 							Desc.Filter = D3D11_FILTER_ANISOTROPIC;
 							Desc.MaxAnisotropy = m_nAnisotropy;
 						}
 						else
 							assert(0);
 	}
 	HRESULT hr = gcpRendD3D->m_pd3dDevice->CreateSamplerState(&Desc, &pSamp);
 	if (SUCCEEDED(hr))
 		m_pDeviceState = pSamp;
 	else
 		assert(0);
 #endif
 }
 
 STexState::~STexState()
 {
 #if defined (DIRECT3D10)
 	if (m_pDeviceState)
 	{
 		ID3D11SamplerState *pSamp = (ID3D11SamplerState *)m_pDeviceState;
 		SAFE_RELEASE(pSamp);
 		m_pDeviceState = NULL;
 	}
 #endif
 }
 
 STexState::STexState(const STexState& src)
 {
 	memcpy(this, &src, sizeof(src));
 #if defined (DIRECT3D10)
 	if (m_pDeviceState)
 	{
 		ID3D11SamplerState *pSamp = (ID3D11SamplerState *)m_pDeviceState;
 		pSamp->AddRef();
 	}
 #endif
 }
 
 bool CTexture::SetClampingMode(int nAddressU, int nAddressV, int nAddressW)
 {
 	return s_sDefState.SetClampMode(nAddressU, nAddressV, nAddressW);
 }
 
 bool CTexture::SetFilterMode(int nFilter)
 {
 	return s_sDefState.SetFilterMode(nFilter);
 }
 
 void CTexture::UpdateTexStates() 
 { 
 	m_nDefState = CTexture::GetTexState(s_sDefState); 
 }
 
 bool CTexture::SetDefTexFilter(const char *szFilter)
 {
 	uint32 i;
 	bool bRes = false;
 	struct textype
 	{
 		char *name;
 		uint32 filter;
 	};
 
 	static textype tt[] =
 	{
 		{"NEAREST", FILTER_POINT},
 		{"LINEAR", FILTER_LINEAR},
 		{"BILINEAR", FILTER_BILINEAR},
 		{"TRILINEAR", FILTER_TRILINEAR},
 	};
 
 	s_sGlobalDefState.m_nAnisotropy = CLAMP(CRenderer::CV_r_texture_anisotropic_level, 1, gcpRendD3D->m_MaxAnisotropyLevel);
 	if (!(gcpRendD3D->GetFeatures() & RFT_ALLOWANISOTROPIC))
 		s_sGlobalDefState.m_nAnisotropy = 1;
 	_SetVar("r_Texture_Anisotropic_Level", s_sGlobalDefState.m_nAnisotropy);
 	s_GlobalTextureFilter = szFilter;
 	if ((gcpRendD3D->GetFeatures() & RFT_ALLOWANISOTROPIC) && s_sDefState.m_nAnisotropy > 1)
 		szFilter = "TRILINEAR";
 
 	s_sGlobalDefState.SetClampMode(TADDR_WRAP, TADDR_WRAP, TADDR_WRAP);
 
 	if (s_sGlobalDefState.m_nAnisotropy > 1)
 	{
 		if (s_sGlobalDefState.m_nAnisotropy == 2)
 			s_sGlobalDefState.SetFilterMode(FILTER_ANISO2X);
 		else
 			if (s_sGlobalDefState.m_nAnisotropy == 4)
 				s_sGlobalDefState.SetFilterMode(FILTER_ANISO4X);
 			else
 				if (s_sGlobalDefState.m_nAnisotropy == 8)
 					s_sGlobalDefState.SetFilterMode(FILTER_ANISO8X);
 				else
 					if (s_sGlobalDefState.m_nAnisotropy == 16)
 						s_sGlobalDefState.SetFilterMode(FILTER_ANISO16X);
 					else
 					{
 						assert(0);
 						s_sGlobalDefState.m_nAnisotropy = 2;
 						s_sGlobalDefState.SetFilterMode(FILTER_ANISO2X);
 					}
 					bRes = true;
 	}
 	else
 	{
 		for (i=0; i<4; i++)
 		{
 			if (!stricmp(szFilter, tt[i].name) )
 			{
 				s_sGlobalDefState.SetFilterMode(tt[i].filter);
 				bRes = true;
 				break;
 			}
 		}
 	}
 	s_nGlobalDefState = CTexture::GetTexState(s_sGlobalDefState);
 	if (bRes)
 	{
 		SResourceContainer *pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
 		if (pRL)
 		{
 			ResourcesMapItor itor;
 			for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
 			{
 				CTexture *tx = (CTexture *)itor->second;
 				if (!tx)
 					continue;
 				tx->SetTexStates();
 			}
 		}
 		return true;
 	}
 
 	assert(0);
 	Warning("Bad texture filter name <%s>\n", szFilter);
 	return false;
 }
 
 void CTexture::SetSamplerState(int nTUnit, int nTS, int nSSlot, int nVtxTexOffSet)
 {  
 	assert(gcpRendD3D->m_pRT->IsRenderThread());
 	STexStageInfo* TexStages = s_TexStages;
 #if defined (DIRECT3D9) || defined (OPENGL)
 
 	if (CRenderer::CV_r_texnoaniso)
 	{
 		STexState *pTS = &s_TexStates[nTS];
 		if (pTS->m_nAnisotropy > 1)
 		{
 			STexState TS = *pTS;
 			TS.SetFilterMode(FILTER_BILINEAR);
 			TS.PostCreate();
 			nTS = CTexture::GetTexState(TS);
 		}
 	}
 
 #if !defined(XENON)
 	{
 		LPDIRECT3DDEVICE9 dv = gcpRendD3D->GetD3DDevice();   
 		bool sRGBRead = gRenDev->IsLinearSpaceShadingEnabled() && ( m_bIsSRGB || s_TexStates[nTS].m_bSRGBLookup && (m_nFlags& FT_USAGE_ALLOWREADSRGB));    
 		STexState *pDTS = &TexStages[nTUnit].m_State;
 		if (pDTS->m_bSRGBLookup != sRGBRead )
 		{
 		  pDTS->m_bSRGBLookup = sRGBRead;
 		  dv->SetSamplerState(nTUnit + nVtxTexOffSet, D3DSAMP_SRGBTEXTURE, sRGBRead ? TRUE : FALSE);
 		}
 	}
 #endif
 
 	LPDIRECT3DDEVICE9 dv = gcpRendD3D->GetD3DDevice();
 	if (TexStages[nTUnit].m_nCurState != nTS)
 	{
 		TexStages[nTUnit].m_nCurState = nTS;
 		STexState *pTS = &s_TexStates[nTS];
 		STexState *pDTS = &TexStages[nTUnit].m_State;
 		if(pTS->m_nMipFilter!=pDTS->m_nMipFilter)
 		{
 			pDTS->m_nMipFilter = pTS->m_nMipFilter;
 			dv->SetSamplerState(nTUnit + nVtxTexOffSet, D3DSAMP_MIPFILTER, pTS->m_nMipFilter);            
 		}
 		if(pTS->m_nMinFilter!=pDTS->m_nMinFilter || pTS->m_nMagFilter!=pDTS->m_nMagFilter || pTS->m_nAnisotropy!=pDTS->m_nAnisotropy)
 		{
 			pDTS->m_nMinFilter = pTS->m_nMinFilter;
 			pDTS->m_nMagFilter = pTS->m_nMagFilter;
 			pDTS->m_nAnisotropy = pTS->m_nAnisotropy;
 			dv->SetSamplerState(nTUnit + nVtxTexOffSet, D3DSAMP_MINFILTER, pTS->m_nMinFilter);
 			dv->SetSamplerState(nTUnit + nVtxTexOffSet, D3DSAMP_MAGFILTER, pTS->m_nMagFilter);
 			if (pTS->m_nMinFilter == D3DTEXF_ANISOTROPIC)
 				dv->SetSamplerState(nTUnit + nVtxTexOffSet, D3DSAMP_MAXANISOTROPY, pTS->m_nAnisotropy);
 		}
 		if (pTS->m_nAddressU != pDTS->m_nAddressU)
 		{
 			pDTS->m_nAddressU = pTS->m_nAddressU;
 			dv->SetSamplerState(nTUnit + nVtxTexOffSet, D3DSAMP_ADDRESSU, pTS->m_nAddressU);
 		}
 		if (pTS->m_nAddressV != pDTS->m_nAddressV)
 		{
 			pDTS->m_nAddressV = pTS->m_nAddressV;
 			dv->SetSamplerState(nTUnit + nVtxTexOffSet, D3DSAMP_ADDRESSV, pTS->m_nAddressV);
 		}
 		if (pTS->m_dwBorderColor != pDTS->m_dwBorderColor)
 		{
 			pDTS->m_dwBorderColor = pTS->m_dwBorderColor;
 			dv->SetSamplerState(nTUnit + nVtxTexOffSet, D3DSAMP_BORDERCOLOR, pTS->m_dwBorderColor);
 		}
 		if ((m_eTT == eTT_Cube || m_eTT == eTT_3D) && pTS->m_nAddressW != pDTS->m_nAddressW)
 		{
 			pDTS->m_nAddressW = pTS->m_nAddressW;
 			dv->SetSamplerState(nTUnit + nVtxTexOffSet, D3DSAMP_ADDRESSW, pTS->m_nAddressW);
 		}
 	}
 	if(TexStages[nTUnit].m_fMipBias != m_fCurrentMipBias)
 	{
 		dv->SetSamplerState(nTUnit + nVtxTexOffSet, D3DSAMP_MIPMAPLODBIAS, *(DWORD*)&m_fCurrentMipBias);
 		TexStages[nTUnit].m_fMipBias = m_fCurrentMipBias;
 	}
 #elif defined (DIRECT3D10)
 	//PS3 workaround, setting samplers every frame for debugging reasons
 #if !defined(PS3)
 	if (s_TexStateIDs[nSSlot] != nTS)
 #endif
 	{
 		s_TexStateIDs[nSSlot] = nTS;
 		STexState *pTS = &s_TexStates[nTS];
 		ID3D11SamplerState *pSamp = (ID3D11SamplerState *)pTS->m_pDeviceState;
 
 		assert(pSamp);
 		if (pSamp)
 		{
 			if(!m_bVertexTexture)
       {
 
 
 
 
 
 
 
 
 
 				gcpRendD3D->GetDeviceContext()->PSSetSamplers(nSSlot, 1, &pSamp);
 
       }
 			else
 				gcpRendD3D->GetDeviceContext()->VSSetSamplers(nSSlot, 1, &pSamp);
 		}
 
 	}
 #endif
 }
 
 void CTexture::Apply(int nTUnit, int nTS, int nTSlot, int nSSlot, int nResView)
 {
 	static int sRecursion = 0;
 	if (!sRecursion)
 	{
 		/*if (this != s_ptexNoTexture)
 		{
 		s_ptexNoTexture->Apply(nTUnit, pTS, nTSlot, bSRGB);
 		return;
 		}*/
 		int nT = nTSlot;
 		if (nT < 0)
 			nT = 0;
 		if (CRenderer::CV_r_texgrid==nT+1 && !(m_nFlags & (FT_DONT_RELEASE | FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET | FT_TEX_FONT)) && m_eTT == eTT_2D)
 		{
 			sRecursion++;
 			CTexture::SetGridTexture(nTUnit, this, nTSlot);
 			sRecursion--;
 			return;
 		}
 		else
 			if (CRenderer::CV_r_texbindmode>=1)
 			{
 				if (CRenderer::CV_r_texbindmode==1 && !(m_nFlags & FT_TEX_FONT))
 				{
 					sRecursion++;
 					CTexture::s_ptexNoTexture->Apply();
 					sRecursion--;
 					return;
 				}
 				if (CRenderer::CV_r_texbindmode==3 && (m_nFlags & FT_FROMIMAGE) && !(m_nFlags & FT_DONT_RELEASE))
 				{
 					sRecursion++;
 					//if (m_Flags & FT_TEX_NORMAL_MAP)
 					//  CTexture::SetGridTexture(this);
 					//else
 					CTexture::s_ptexGray->Apply();
 					sRecursion--;
 					return;
 				}
 				if (CRenderer::CV_r_texbindmode==4 && (m_nFlags & FT_FROMIMAGE) && !(m_nFlags & FT_DONT_RELEASE) && m_eTT == eTT_2D)
 				{
 					sRecursion++;
 					CTexture::s_ptexGray->Apply();
 					sRecursion--;
 					return;
 				}
 				if (CRenderer::CV_r_texbindmode==5 && (m_nFlags & FT_FROMIMAGE) && !(m_nFlags & FT_DONT_RELEASE) && (m_nFlags & FT_TEX_NORMAL_MAP))
 				{
 					sRecursion++;
 					CTexture::s_ptexFlatBump->Apply();
 					sRecursion--;
 					return;
 				}
 				if (CRenderer::CV_r_texbindmode==6 && nTSlot==EFTT_DIFFUSE)
 				{
 					sRecursion++;
 					CTexture::s_ptexWhite->Apply();
 					sRecursion--;
 					return;
 				}
 				if (CRenderer::CV_r_texbindmode==7 && nTSlot==EFTT_DIFFUSE)
 				{
 					sRecursion++;
 					CTexture::s_ptexMipMapDebug->Apply();
 					sRecursion--;
 					return;
 				}
 				if (CRenderer::CV_r_texbindmode==8 && nTSlot==EFTT_DIFFUSE)
 				{
 					sRecursion++;
 					Apply();
 					switch (m_nMinMipVidUploaded)
 					{
 						case 0:
 							CTexture::s_ptexColorWhite->Apply();
 							break;
 						case 1:
 							CTexture::s_ptexColorBlue->Apply();
 							break;
 						case 2:
 							CTexture::s_ptexColorGreen->Apply();
 							break;
 						case 3:
 							CTexture::s_ptexColorCyan->Apply();
 							break;
 						case 4:
 							CTexture::s_ptexColorRed->Apply();
 							break;
 						case 5:
 							CTexture::s_ptexColorPurple->Apply();
 							break;
 						default:
 							CTexture::s_ptexColorYellow->Apply();
 							break;
 					}
 					sRecursion--;
 					return;
 				}
 			}
 	}
 
 	if (nTUnit < 0)
 		nTUnit = s_CurStage;
 
 
 
 
 
 	if (nSSlot < 0)		nSSlot = nTUnit;
 
 	assert(nTUnit >= 0 && nTUnit < gcpRendD3D->m_numtmus);
 
 	if (IsStreamed())
 	{
 		Relink(&CTexture::s_Root);
 		if (IsUnloaded() || IsPartiallyLoaded())
 		{
 			PROFILE_FRAME(Texture_Precache);
 			if (!s_nStreamingEnabled || !m_bStreamPrepared)
 				Load(m_eTFDst, -1, -1);
 			if (s_nStreamingEnabled && m_bStreamPrepared)
 				StreamRestore();
 		}
 		// mip map fade in
 		if(fabsf(m_fCurrentMipBias) > 0.06667f)
 		{
 			// one mip per half a second
 			m_fCurrentMipBias -= 0.06667f * m_fCurrentMipBias;
 #if defined(DIRECT3D10) && !defined(PS3)
 			// AntonK: supported only by DX11 feature set
 			// needs to be enabled for pure DX11 context
 			//if(m_pDevTexture->Get2DTexture())
 			//	gcpRendD3D->GetDeviceContext()->SetResourceMinLOD(m_pDevTexture->Get2DTexture(), m_fCurrentMipBias + (float)m_nMinMipVidUploaded);
 #endif
 		}
 		else
 			m_fCurrentMipBias = 0.f;
 	}
 
 	if (!m_pDevTexture || !m_pDevTexture->GetBaseTexture())
 	{
 #if !defined(XENON) && !defined(PS3)
 		assert(!"m_pDevTexture is NULL");
 #endif
 		// apply black by default
 		if(CTexture::s_ptexBlack && sRecursion == 0)
 		{
 			++sRecursion;
 			CTexture::s_ptexBlack->Apply(nTUnit, nTS, nTSlot, nResView);
 			--sRecursion;
 		}
 		return;
 	}
 
 	// Resolve multisampled RT to texture
 	if (!m_bResolved)
 		Resolve();
 	CD3D9Renderer *const __restrict rd = gcpRendD3D;
 	int nFrameID = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
 	if (m_nAccessFrameID != nFrameID)
 	{ 
 		m_nAccessFrameID = nFrameID;
 		rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_NumTextures++;
 		if (m_nFlags & (FT_USAGE_RENDERTARGET | FT_USAGE_DYNAMIC))
 			rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_DynTexturesSize += m_nDeviceSize;
 		else
 		{
 			rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_ManagedTexturesSize += m_nDeviceSize;
 			rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_ManagedTexturesFullSize += m_nSize;
 		}
 	}
 
 
 
 
 
 
 
 
 
 
 	if (nTS < 0)
 		nTS = m_nDefState;
 	assert(nTS >= 0 && nTS < s_TexStates.size());
 
 	int nVtxTexOffSet = 0;
 #if defined (DIRECT3D9) || defined (OPENGL)  
 	if (m_bVertexTexture)
 		nVtxTexOffSet = D3DVERTEXTEXTURESAMPLER0;
 #endif
 	SetSamplerState(nTUnit, nTS, nSSlot, nVtxTexOffSet);
 	STexStageInfo *TexStages = s_TexStages;
 
 #if defined(DIRECT3D10)
 	if (this == TexStages[nTUnit].m_Texture && nResView == TexStages[nTUnit].m_nCurResView) 
 		return;
 	TexStages[nTUnit].m_nCurResView = nResView;
   
   #if !defined(PS3) // We allow reads from destination RT on PS3
 
     // in DX11 don't allow to set texture while it still bound as RT
     if (rd->m_pCurTarget[0] == this)
     {
       assert(rd->m_pCurTarget[0]->m_pDeviceRTV);
       rd->m_pNewTarget[0]->m_bWasSetRT = false;
       rd->m_pd3dDeviceContext->OMSetRenderTargets(1, &rd->m_pNewTarget[0]->m_pTarget, rd->m_pNewTarget[0]->m_pDepth);
     }
 
   #endif
 
 #else
 	if (this == TexStages[nTUnit].m_Texture )
 		return;
 #endif
 
 	TexStages[nTUnit].m_Texture = this;
 	rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_NumTextChanges++;
 
 #ifdef DO_RENDERLOG
 	if (CRenderer::CV_r_log >= 3)
 	{
 		if (IsNoTexture())
 			rd->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "CTexture::Apply(): (%d) \"%s\" (Not found)\n", nTUnit, m_SrcName.c_str());
 		else
 			rd->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "CTexture::Apply(): (%d) \"%s\"\n", nTUnit, m_SrcName.c_str());
 	}
 #endif
 
 #if defined (DIRECT3D9) || defined (OPENGL)  
 	HRESULT hr = rd->GetD3DDevice()->SetTexture(nTUnit+nVtxTexOffSet, m_pDevTexture->Get2DTexture());
 	assert(SUCCEEDED(hr));
 #elif defined (DIRECT3D10)
 	if (m_pDevTexture)
 	{
 		ID3D11ShaderResourceView *pRes = NULL;
 		if (nResView>=0)
 		{
 			assert(nResView<5);
 			if (nResView==4) //multisample view
 			{
 				pRes = (ID3D11ShaderResourceView *)m_pDeviceShaderResourceMS;
 			}
 			else
 			{
 				pRes = (ID3D11ShaderResourceView *)m_pShaderResourceViews[nResView];
 			}
 		}
 		else
 		{
       pRes = (ID3D11ShaderResourceView *)m_pDeviceShaderResource;
 #if !defined(PS3)
       if( gRenDev->IsLinearSpaceShadingEnabled() && m_nFlags& FT_USAGE_ALLOWREADSRGB )
       {
         if (s_TexStates[nTS].m_bSRGBLookup)
           pRes = (ID3D11ShaderResourceView *)m_pDeviceShaderResourceSRGB; 
       }	
 #endif
 		}
 
 #ifdef DO_RENDERLOG
 		if (CRenderer::CV_r_log >= 3 && nResView>=0)
 		{
 			rd->Logv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "CTexture::Apply(): Shader Resource View: %d \n", nResView);
 		}
 #endif
 
 		//test
 		//D3D11_SHADER_RESOURCE_VIEW_DESC Desc;
 		//pRes->GetDesc( &Desc );
 
 		//assert(pRes);
 		if ( pRes )
 		{
 			if( !m_bVertexTexture  )
 				rd->GetDeviceContext()->PSSetShaderResources(nTUnit, 1, &pRes);
 			else
 				rd->GetDeviceContext()->VSSetShaderResources(nTUnit, 1, &pRes);
 		}
 	}
 #endif
 }
 
 void CTexture::BindNULLFrom(int nFrom)
 {
 	assert(nFrom < MAX_TMU);
 #if defined (DIRECT3D10) 
 	ID3D11ShaderResourceView *RV[MAX_TMU];
 	memset(&RV[0], 0, sizeof(RV));
 	gcpRendD3D->GetDeviceContext()->PSSetShaderResources(nFrom, MAX_TMU - nFrom, RV);
 	for (int i=nFrom; i<MAX_TMU; i++)
 	{
 		s_TexStages[i].m_Texture = NULL;
 	}
 #else
 	for (int i=nFrom; i<MAX_TMU; i++)
 	{
 #if defined (DIRECT3D9)
 		HRESULT hr = gcpRendD3D->GetD3DDevice()->SetTexture(i, NULL);
 
 
 
 
 #endif
 		s_TexStages[i].m_Texture = NULL;
 	}
 #endif
 }
 
 void CTexture::UpdateTextureRegion(byte *data, int nX, int nY, int USize, int VSize, ETEX_Format eTFSrc)
 {
 	gRenDev->m_pRT->RC_UpdateTextureRegion(this, data, nX, nY, USize, VSize, eTFSrc);
 }
 
 void CTexture::RT_UpdateTextureRegion(byte *data, int nX, int nY, int USize, int VSize, ETEX_Format eTFSrc)
 {
 	PROFILE_FRAME(UpdateTextureRegion);
 
 	if(m_eTT != eTT_2D)
 	{
 		assert(0);
 		return;
 	}
 
 #if defined (DIRECT3D9) || defined(OPENGL)
 
 	CDeviceTexture *pDevTexture = NULL;
 	D3DSurface *pDestSurf = NULL;
 	pDevTexture = GetDevTexture();
 	assert(pDevTexture);
 	if (!pDevTexture)
 		return;
 
 	D3DFormat frmtSrc = CTexture::DeviceFormatFromTexFormat(eTFSrc);
 	bool bDone = false;
 	RECT rc = {nX, nY, nX+USize, nY+VSize};
 	if (m_eTT == eTT_2D)
 	{
 		if (!IsDXTCompressed(m_eTFDst))
 		{
 			int nBPPSrc = CTexture::BitsPerPixel(eTFSrc);
 			int nBPPDst = CTexture::BitsPerPixel(m_eTFDst);
 			if (nBPPSrc == nBPPDst)
 			{
 				IDirect3DSurface9 *pSurf = GetSurface(0, 0);
 				D3DLOCKED_RECT lr;
 
 
 
 
 
 
 
 
 
 
 				if(pSurf->LockRect(&lr, &rc, 0)==D3D_OK)
 				{
 					byte * p = (byte*)lr.pBits;
 					int nOffsSrc = 0;
 					int nSizeLine = CTexture::TextureDataSize(USize, 1, 1, 1, eTFSrc);
 					for(int y=0; y<VSize; y++)
 					{
 						cryMemcpy(&p[y*lr.Pitch], &data[nOffsSrc], nSizeLine);
 						nOffsSrc += nSizeLine;
 					}
 					pSurf->UnlockRect();
 				}
 
 				SAFE_RELEASE(pSurf);
 				bDone = true;
 			}
 			else
       if ((eTFSrc==eTF_A8R8G8B8 || eTFSrc==eTF_X8R8G8B8) && (m_eTFDst==eTF_R5G6B5))
 			{
 				IDirect3DSurface9 *pSurf = GetSurface(0, 0);
 				D3DLOCKED_RECT lr;
 				if (pSurf->LockRect(&lr, &rc, 0)==D3D_OK)
 				{
 					byte *pBits = (byte *)lr.pBits;
 					int nOffsSrc = 0;
 					int nSizeLine = CTexture::TextureDataSize(USize, 1, 1, 1, eTFSrc);
 					for (int i=0; i<VSize; i++)
 					{
 						uint32 *src = (uint32 *)&data[nOffsSrc];
 						uint16 *dst = (uint16 *)&pBits[i*lr.Pitch];
 						for (int j=0; j<USize; j++)
 						{
 							uint32 argb = *src++;
 							*dst++ =  ((argb >> 8) & 0xF800) |
 								((argb >> 5) & 0x07E0) |
 								((argb >> 3) & 0x001F);
 						}
 						nOffsSrc += nSizeLine;
 					}
 					pSurf->UnlockRect();
 				}
 				SAFE_RELEASE(pSurf);
 				bDone = true;
 			}
 		}
 	}
 
 	// It looks stupid, but (!bDone) DOESNT work on XENON! The compiler just remove this branch
 	if (bDone)
 	{
 	}
 	else
 	{
 #ifndef XENON
 		int nPitch;
 		if (IsDXTCompressed(eTFSrc))
 		{
 			int blockSize = (eTFSrc == eTF_DXT1 || eTFSrc == eTF_CTX1) ? 8 : 16;
 			nPitch = (USize+3)/4 * blockSize;
 		}
 		else
 		{
 			int size = TextureDataSize(USize, VSize, 1, 1, eTFSrc);
 			nPitch = size / VSize;
 		}
 		RECT rcs;
 		rcs.left = 0;
 		rcs.right = USize;
 		rcs.top = 0;
 		rcs.bottom = VSize;
 		HRESULT hr;
 		pDestSurf = GetSurface(0, 0);
 		assert(pDestSurf);
 		if (pDestSurf)
 		{
 			hr = D3DXLoadSurfaceFromMemory(pDestSurf, NULL, &rc, data, frmtSrc, nPitch, NULL, &rcs, D3DX_FILTER_NONE, 0);
 			assert(SUCCEEDED(hr));
 		}
 		SAFE_RELEASE(pDestSurf);
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 #endif
 	}
 #elif defined (DIRECT3D10)
 	HRESULT hr = S_OK;
 	CDeviceTexture* pDevTexture = GetDevTexture();
 	assert(pDevTexture);
 	if (!pDevTexture)
 		return;
 
 	DXGI_FORMAT frmtSrc = (DXGI_FORMAT)CTexture::DeviceFormatFromTexFormat(eTFSrc);
 	bool bDone = false;
 	D3D11_BOX rc = {nX, nY, 0, nX+USize, nY+VSize, 1};
 	if (m_eTT == eTT_2D)
 	{
 		if (!IsDXTCompressed(m_eTFDst))
 		{
 			int nBPPSrc = CTexture::BitsPerPixel(eTFSrc);
 			int nBPPDst = CTexture::BitsPerPixel(m_eTFDst);
 			if(nBPPSrc == nBPPDst)
 			{
 				int nRowPitch = CTexture::TextureDataSize(USize, 1, 1, 1, eTFSrc);
 				gcpRendD3D->m_pd3dDeviceContext->UpdateSubresource(pDevTexture->Get2DTexture(), 0, &rc, data, nRowPitch, 0);
 				bDone = true;
 			}
 			else
 			if((eTFSrc==eTF_A8R8G8B8 || eTFSrc==eTF_X8R8G8B8) && (m_eTFDst==eTF_R5G6B5))
 			{
 				assert(0);
 				bDone = true;
 			}
 		}
 	}
 	if (!bDone)
 	{
 		CTexture t;
 		byte *pData[6] = {data,0,0,0,0,0};
 		t.m_eTFDst = t.m_eTFSrc = eTFSrc;
 		t.m_nWidth = USize;
 		t.m_nHeight = VSize;
 		t.CreateDeviceTexture(pData);
 		assert(t.GetDevTexture());
 		CDeviceTexture *pSrcRes = t.GetDevTexture();
 		gcpRendD3D->m_pd3dDeviceContext->CopySubresourceRegion(pDevTexture->Get2DTexture(), 0, nX, nY, 0, pSrcRes->Get2DTexture(), 0, NULL);
 	}
 #endif
 }
 
 bool CTexture::Fill(const ColorF& color)
 {
 	if (!(m_nFlags & FT_USAGE_RENDERTARGET))
 		return false;
 
 #if defined (DIRECT3D9) || defined(OPENGL)
 	D3DSurface *pSurf = GetSurface(0, 0);
 	if (!pSurf)
 		return false;
 	HRESULT hr = gcpRendD3D->m_pd3dDevice->ColorFill(pSurf, NULL, D3DRGBA(color.r, color.g, color.b, color.a));
 	SAFE_RELEASE(pSurf);
 	if (FAILED(hr))
 		return false;
 	return true;
 #elif defined (DIRECT3D10)
 	//assert(0);
 	return false;
 #endif
 }
 
 void SEnvTexture::Release()
 {
 	ReleaseDeviceObjects();
 	SAFE_DELETE(m_pTex);
 }
 
 void SEnvTexture::RT_SetMatrix()
 {
 	Matrix44A matView, matProj;
 	gRenDev->GetModelViewMatrix(matView.GetData());
 	gRenDev->GetProjectionMatrix(matProj.GetData());
 
 	Matrix44A matScaleBias(0.5f, 0, 0,   0,
 		0, -0.5f,    0,   0,
 		0,     0, 0.5f,   0,
 		// texel alignment - also push up y axis reflection up a bit
 		0.5f+0.5f/m_pTex->GetWidth(), 0.5f+ 1.0f/m_pTex->GetHeight(), 0.5f, 1.0f );
 
 	Matrix44A m;
 	m.Multiply(matProj, matScaleBias);
 	Matrix44A mm;
 	mm.Multiply(matView, m);
 	m_Matrix = mm;
 }
 
 byte *CTexture::LockData(int& nPitch, int nSide, int nLevel)
 {
 	CDeviceTexture *pDevTexture = GetDevTexture();
   assert(pDevTexture);
   if (!pDevTexture)
     return NULL;
 	if (m_eTT == eTT_2D)
 	{
 		STexLock lr;
 		if (pDevTexture->LockRect(nLevel, lr, 0)==D3D_OK)
 		{
 			nPitch = lr.Pitch;
 			return lr.pData;
 		}
 	}
 	else
 	if (m_eTT == eTT_Cube)
 	{
 		STexLock lr;
 		if (pDevTexture->LockRect(nSide, nLevel, lr, 0)==D3D_OK)
 		{
 			nPitch = lr.Pitch;
 			return lr.pData;
 		}
 	}
 	else
 	if (m_eTT == eTT_3D)
 	{
 		STexLockBox lr;
 		if (pDevTexture->LockBox(nLevel, lr, 0)==D3D_OK)
 		{
 			nPitch = lr.RowPitch;
 			return lr.pData;
 		}
 	}
 	return NULL;
 }
 
 void CTexture::UnlockData(int nSide, int nLevel)
 {
   CDeviceTexture *pDevTexture = GetDevTexture();
   assert(pDevTexture);
   if (!pDevTexture)
     return;
 	if (m_eTT == eTT_2D)
 		pDevTexture->UnlockRect(nLevel);
 	else
   if (m_eTT == eTT_Cube)
 	  pDevTexture->UnlockRect(nSide, nLevel);
   else
   if (m_eTT == eTT_3D)
     pDevTexture->UnlockBox(nLevel);
 }
 
 void SEnvTexture::ReleaseDeviceObjects()
 {
 	//if (m_pTex)
 	//  m_pTex->ReleaseDynamicRT(true);
 }
 
 bool CTexture::RenderEnvironmentCMHDR(int size, Vec3& Pos, TArray<unsigned short>& vecData)
 {
 	int RendFlags = -1;
 
 	if((uint32)vecData.GetMemoryUsage() < size*size*6*4*sizeof(unsigned short))
 	{
 		assert(0);
 		return false;
 	}
 	vecData.SetUse(0);
 
 	// how to skip characters only ?
 	//RendFlags &= ~DLD_ENTITIES;
 
 	// workaround: no way to make 3dengine skip characters for now
 	ICVar *ca_DrawCharacter = gEnv->pConsole->GetCVar("ca_DrawCharacter");
 	int nDrawCharacterState = 0;
 	if( ca_DrawCharacter )
 		nDrawCharacterState = ca_DrawCharacter->GetIVal();
 
 	// disable postprocessing!
 	ICVar *r_Postprocess = gEnv->pConsole->GetCVar("r_PostProcessEffects");
 	int nPostProcessState;
 	if(r_Postprocess)
 	{
 		nPostProcessState = r_Postprocess->GetIVal();
 		r_Postprocess->Set(0);
 	}
 	else
 		assert(0);
 
 	int vX, vY, vWidth, vHeight;
 	gRenDev->GetViewport(&vX, &vY, &vWidth, &vHeight);
 
 
 #if defined (DIRECT3D9) || defined(OPENGL)
 	int n;
 	LPDIRECT3DDEVICE9 dv = gcpRendD3D->GetD3DDevice();
 	LPDIRECT3DCUBETEXTURE9 pID3DSysTexture = NULL;
 	D3DLOCKED_RECT d3dlrTarg, d3dlrSys;
 	HRESULT h;
 
 	//CRenderer::CV_r_log = 3;
 	//gcpRendD3D->ChangeLog();
 	LPDIRECT3DSURFACE9 pTargetSurf;
 	h = dv->CreateRenderTarget(size, size, D3DFMT_A16B16G16R16F, D3DMULTISAMPLE_NONE, 0, TRUE, &pTargetSurf, NULL);
 	if (FAILED(h))
 		return false;
 
 	SDynTexture *pDynT = new SDynTexture(size, size, eTF_A16B16G16R16F, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, "$TempEnv");
 	pDynT->Update(size, size);
 	CTexture *tpSrc = pDynT->m_pTexture;
 
 	h = D3DXCreateCubeTexture(gcpRendD3D->GetDevice(), size, 1, 0, D3DFMT_A16B16G16R16F, D3DPOOL_SYSTEMMEM, &pID3DSysTexture);
 	if (FAILED(h))
 		return false;
 
 	int *pFR = (int *)gRenDev->EF_Query(EFQ_Pointer2FrameID);
 	I3DEngine *eng = (I3DEngine *)gEnv->p3DEngine;
 	float fMaxDist = eng->GetMaxViewDistance();
 	ColorF black(Col_Black);
 	SD3DSurface *srfDepth = gcpRendD3D->FX_GetDepthSurface(size, size, false);
 	int nPFlags = gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags;
 
 	if( ca_DrawCharacter )
 		ca_DrawCharacter->Set( 0 );
 
 	gcpRendD3D->EF_StartEf();
 	for(n=0; n<6; n++)
 	{ 
 		(*pFR)++;
 
 		// draw cube face
 		gcpRendD3D->FX_PushRenderTarget(0, tpSrc, srfDepth);
 		gcpRendD3D->RT_SetViewport(0, 0, size, size);
 		gcpRendD3D->EF_ClearBuffers(FRT_CLEAR, &black);
 		gcpRendD3D->FX_Commit();
 		DrawCubeSide(Pos, size, n, RendFlags, fMaxDist);
 		gcpRendD3D->FX_PopRenderTarget(0);
 
 
 		// Post process (blurring)
 		if (pTargetSurf)
 		{
 			STexState TexStateLinear = STexState(FILTER_LINEAR, true);
 			gcpRendD3D->FX_PushRenderTarget(0, pTargetSurf, srfDepth);
 			gcpRendD3D->SetCullMode(R_CULL_NONE);
 			gcpRendD3D->RT_SetViewport(0, 0, size, size);
 
 			tpSrc->Apply(0, CTexture::GetTexState(TexStateLinear));
 
 			static CCryName techName("StretchHDR");
 			gcpRendD3D->DrawFullScreenQuad(CShaderMan::m_shHDRPostProcess, techName, 0, 0, 1, 1);
 			gcpRendD3D->FX_PopRenderTarget(0);
 			gcpRendD3D->FX_Commit();
 
 			// Copy data
 			h = pTargetSurf->LockRect(&d3dlrTarg, NULL, 0);
 			h = pID3DSysTexture->LockRect((D3DCUBEMAP_FACES)n, 0, &d3dlrSys, NULL, 0);
 			byte *pSys = (byte *)d3dlrSys.pBits;
 			byte *pTarg = (byte *)d3dlrTarg.pBits;
 			memcpy(pSys, pTarg, size*size*4*sizeof(unsigned short));
 			h = pID3DSysTexture->UnlockRect((D3DCUBEMAP_FACES)n, 0);
 			h = pTargetSurf->UnlockRect();
 		}
 	}
 
 	SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID]--;
 	//vecData.ReserveNoClear(CTexture::TextureDataSize(size, size, 1, 1, eTF_A16B16G16R16F)/sizeof(unsigned short)*6);
 	//vecData.SetUse(0);
 
 	for (n=0; n<6; n++)
 	{
 		pID3DSysTexture->LockRect((D3DCUBEMAP_FACES)n, 0, &d3dlrSys, NULL, 0);
 		vecData.Copy((unsigned short *)d3dlrSys.pBits, CTexture::TextureDataSize(size, size, 1, 1, eTF_A16B16G16R16F)/sizeof(unsigned short));
 		h = pID3DSysTexture->UnlockRect((D3DCUBEMAP_FACES)n, 0);
 	}
 	assert(IsHeapValid());
 
 	// switch pp back
 	if(r_Postprocess)
 	{
 		r_Postprocess->Set(nPostProcessState);
 	}
 
 
 	gcpRendD3D->RT_SetViewport(vX, vY, vWidth, vHeight);
 	gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags = nPFlags;
 
 	SAFE_RELEASE(pTargetSurf);
 	SAFE_RELEASE(pID3DSysTexture);
 	SAFE_DELETE(pDynT);
 
 #elif defined (DIRECT3D10)
 	assert(0);
 #endif
 
 	if( ca_DrawCharacter )
 		ca_DrawCharacter->Set( nDrawCharacterState );
 
 	return true;
 }
 
 bool CTexture::ScanEnvironmentCM(const char *name, int size, Vec3& Pos)
 {
 	int RendFlags = -1;
 
   // how to skip characters only ?
 	//RendFlags &= ~DLD_ENTITIES;
 
   // workaround: no way to make 3dengine skip characters for now
   ICVar *ca_DrawCharacter = gEnv->pConsole->GetCVar("ca_DrawCharacter");
   int nDrawCharacterState = 0;
   if( ca_DrawCharacter )
     nDrawCharacterState = ca_DrawCharacter->GetIVal();
 
 	int vX, vY, vWidth, vHeight;
 	gRenDev->GetViewport(&vX, &vY, &vWidth, &vHeight);
 
 #if defined (DIRECT3D9) || defined(OPENGL)
 
 	int n;
 	LPDIRECT3DDEVICE9 dv = gcpRendD3D->GetD3DDevice();
 	LPDIRECT3DCUBETEXTURE9 pID3DSysTexture = NULL;
 	D3DLOCKED_RECT d3dlrTarg, d3dlrSys;
 	HRESULT h;
 
 	//CRenderer::CV_r_log = 3;
 	//gcpRendD3D->ChangeLog();
 	LPDIRECT3DSURFACE9 pTargetSurf;
 	h = dv->CreateRenderTarget(size, size, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &pTargetSurf, NULL);
 	if (FAILED(h))
 		return false;
 
 	SDynTexture *pDynT = new SDynTexture(size, size, eTF_A16B16G16R16F, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, "$TempEnv");
 	pDynT->Update(size, size);
 	CTexture *tpSrc = pDynT->m_pTexture;
 
 	h = D3DXCreateCubeTexture(gcpRendD3D->GetDevice(), size, D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pID3DSysTexture);
 	if (FAILED(h))
 		return false;
 
 	int *pFR = (int *)gRenDev->EF_Query(EFQ_Pointer2FrameID);
 	I3DEngine *eng = (I3DEngine *)gEnv->p3DEngine;
 	float fMaxDist = eng->GetMaxViewDistance();
 	ColorF black(Col_Black);
 	SD3DSurface *srfDepth = gcpRendD3D->FX_GetDepthSurface(size, size, false);
 	int nPFlags = gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags;
 	//gcpRendD3D->m_RP.m_TI.m_PersFlags |= RBPF_HDR;
 
   if( ca_DrawCharacter )
     ca_DrawCharacter->Set( 0 );
 
   gcpRendD3D->EF_StartEf();
 	for(n=0; n<6; n++)
 	{ 
 		(*pFR)++;
 
 		gcpRendD3D->FX_PushRenderTarget(0, tpSrc, srfDepth);
 		//gcpRendD3D->FX_PushRenderTarget(0, pTargetSurf, gcpRendD3D->FX_GetDepthSurface(size, size, false));
 
 		gcpRendD3D->RT_SetViewport(0, 0, size, size);
 		gcpRendD3D->EF_ClearBuffers(FRT_CLEAR, &black);
     
 
 		gcpRendD3D->FX_Commit();
 		DrawCubeSide(Pos, size, n, RendFlags, fMaxDist);
 		gcpRendD3D->FX_PopRenderTarget(0);
 
     //tpSrc->SaveJPG("cm_debug.jpg", 0);
 
 		// Post process (blurring)
 		if (pTargetSurf)
 		{
 			STexState TexStateLinear = STexState(FILTER_LINEAR, true);
 			gcpRendD3D->FX_PushRenderTarget(0, pTargetSurf, srfDepth);
 			gcpRendD3D->SetCullMode(R_CULL_NONE);
 			gcpRendD3D->RT_SetViewport(0, 0, size, size);
 			
       tpSrc->Apply(0, CTexture::GetTexState(TexStateLinear));
 
 			static CCryName techName("EncodeHDR");
 
       CCryName techName2("Encode_ToLDR");      
 			gcpRendD3D->DrawFullScreenQuad(CShaderMan::m_shHDRPostProcess, techName2, 0, 0, 1, 1);
 			gcpRendD3D->FX_PopRenderTarget(0);
 			gcpRendD3D->FX_Commit();
 
 			// Copy data
 			h = pTargetSurf->LockRect(&d3dlrTarg, NULL, 0);
 			h = pID3DSysTexture->LockRect((D3DCUBEMAP_FACES)n, 0, &d3dlrSys, NULL, 0);
 			byte *pSys = (byte *)d3dlrSys.pBits;
 			byte *pTarg = (byte *)d3dlrTarg.pBits;
 			memcpy(pSys, pTarg, size*size*4);
 			h = pID3DSysTexture->UnlockRect((D3DCUBEMAP_FACES)n, 0);
 			h = pTargetSurf->UnlockRect();
 		}
 	}
 	SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID]--;
 	TArray<byte> Data;
 	h = D3DXFilterCubeTexture(pID3DSysTexture, NULL, D3DX_DEFAULT, D3DTEXF_LINEAR);
 	int nMips = CTexture::CalcNumMips(size, size);
 	for (n=0; n<6; n++)
 	{
 		int wd = size;
 		int hg = size;
 		for (int i=0; i<nMips; i++)
 		{
 			assert(wd || hg);
 			if (!wd)
 				wd = 1;
 			if (!hg)
 				hg = 1;
 			pID3DSysTexture->LockRect((D3DCUBEMAP_FACES)n, i, &d3dlrSys, NULL, 0);
 			Data.Copy((byte *)d3dlrSys.pBits, CTexture::TextureDataSize(wd, hg, 1, 1, eTF_A8R8G8B8));
 			h = pID3DSysTexture->UnlockRect((D3DCUBEMAP_FACES)n, i);
 			wd >>= 1;
 			hg >>= 1;
 		}
 	}
 
 	::WriteDDS(&Data[0], size, size, 1, name, eTF_A8R8G8B8, nMips, eTT_Cube);
 
 	gcpRendD3D->RT_SetViewport(vX, vY, vWidth, vHeight);
 	gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags = nPFlags;
 
 	SAFE_RELEASE(pTargetSurf);
 	SAFE_RELEASE(pID3DSysTexture);
 	SAFE_DELETE(pDynT);
 
 #elif defined (DIRECT3D10)
 	assert(0);
 #endif
 
   if( ca_DrawCharacter )
     ca_DrawCharacter->Set( nDrawCharacterState );
 
 	//CRenderer::CV_r_log = 0;
 	//gcpRendD3D->ChangeLog();
 
 	return true;
 }
 
 //////////////////////////////////////////////////////////////////////////
 
 void CTexture::DrawCubeSide(Vec3& Pos, int tex_size, int side, int RendFlags, float fMaxDist)
 {  
 	const float sCubeVector[6][7] = 
 	{
 		{ 1, 0, 0, 0, 0, 1,-90}, //posx
 		{-1, 0, 0, 0, 0, 1, 90}, //negx
 		{ 0,-1, 0, 0, 0, 1,  0}, //posy
 		{ 0, 1, 0, 0, 0,-1,  0}, //negy    
 		{ 0, 0, 1, 0, 1, 0,  0}, //posz 
 		{ 0, 0,-1, 0, 1, 0,  0}, //negz
 	};
 
 	if (!iSystem)
 		return;
 
 	CRenderer *r = gRenDev;
 	CCamera prevCamera = r->GetCamera();
 	CCamera tmpCamera = prevCamera;
 
 	I3DEngine *eng = gEnv->p3DEngine;
 	float fMinDist = 0.25f;
 
 	Vec3 vForward = Vec3(sCubeVector[side][0], sCubeVector[side][1], sCubeVector[side][2]);
 	Vec3 vUp      = Vec3(sCubeVector[side][3], sCubeVector[side][4], sCubeVector[side][5]);
   Matrix33 matRot = Matrix33::CreateOrientation(vForward, vUp, DEG2RAD(sCubeVector[side][6]));
   
   // magic orientation we use in engine
   Matrix33 matScale = Matrix33::CreateScale( Vec3(1, -1, 1) ); 
   matRot = matScale * matRot;
    
   tmpCamera.SetMatrix( Matrix34(matRot, Pos) );
 	tmpCamera.SetFrustum(tex_size, tex_size, 90.0f*gf_PI/180.0f, fMinDist, fMaxDist);
 	int nPersFlags = r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags;
 	int nPersFlags2 = r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags2;
 	r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags |= /*RBPF_MIRRORCAMERA | */ RBPF_MIRRORCULL | RBPF_DRAWTOTEXTURE;
 	r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_ENCODE_HDR;
 	int nOldZ = CRenderer::CV_r_usezpass;
 	CRenderer::CV_r_usezpass = 0;
 
 	r->RT_SetViewport(0, 0, tex_size, tex_size);
 
 #ifdef DO_RENDERLOG
 	if (CRenderer::CV_r_log)
 		r->Logv(SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID], ".. DrawLowDetail .. (DrawCubeSide %d)\n", side);
 #endif
 
 	eng->RenderWorld(SHDF_ALLOWHDR | SHDF_SORT | SHDF_NOASYNC | SHDF_STREAM_SYNC, &tmpCamera, 1, __FUNCTION__, RendFlags, -1);
 
 #ifdef DO_RENDERLOG
 	if (CRenderer::CV_r_log)
 		r->Logv(SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID], ".. End DrawLowDetail .. (DrawCubeSide %d)\n", side);
 #endif
 
 	r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags = nPersFlags;
 	r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags2 = nPersFlags2;
 	r->SetCamera(prevCamera);
 	CRenderer::CV_r_usezpass = nOldZ;
 }
 
 #define HDR_FAKE_MAXOVERBRIGHT 16
 
 void CTexture::GetAverageColor(SEnvTexture *cm, int nSide)
 {
 	assert (nSide>=0 && nSide<=5);
 
 	//if (!cm->m_pRenderTargets[nSide])
 	//  return;
 
 	if (!cm->m_pTex || !cm->m_pTex->m_pTexture)
 		return;
 	if (cm->m_nFrameCreated[nSide] == -1)
 		return;
 
 	HRESULT hr = S_OK;
 	/*assert(cm->m_pQuery);
 	if (cm->m_pQuery[nSide])
 	{
 	LPDIRECT3DQUERY9 pQuery = (LPDIRECT3DQUERY9)cm->m_pQuery[nSide];
 	BOOL bQuery = false;
 	hr = pQuery->GetData((void *)&bQuery, sizeof(BOOL), 0);
 	if (hr == S_FALSE)
 	return;
 	//int nFrame = gcpRendD3D->GetFrameID(false);
 	//nFrame -= cm->m_nFrameCreated[nSide];
 	//if (nFrame < 100)
 	//  return;
 	}*/
 	ColorF col = ColorF(0,0,0,0);
 
 #if defined (DIRECT3D9) || defined(OPENGL)
 
 	SDynTexture *pTex = cm->m_pTex;
 	CTexture *pT = pTex->m_pTexture;
 	D3DSurface *pTargSurf, *pSrcSurf;
 	//pTargSurf = (LPDIRECT3DSURFACE9)cm->m_pRenderTargets[nSide];
 	gcpRendD3D->m_pd3dDevice->CreateOffscreenPlainSurface(pTex->GetWidth(), pTex->GetHeight(), pT->m_pPixelFormat->DeviceFormat, D3DPOOL_SYSTEMMEM, &pTargSurf, NULL);
 	pSrcSurf = pT->GetSurface(nSide, 0);
 	gcpRendD3D->m_pd3dDevice->GetRenderTargetData(pSrcSurf, pTargSurf);
 
 	int i, j;
 	D3DLOCKED_RECT d3dlr;
 	hr = pTargSurf->LockRect(&d3dlr, NULL, D3DLOCK_READONLY);
 	//if (hr == D3DERR_WASSTILLDRAWING)
 	//  return;
 
 	byte *pData = (byte *)d3dlr.pBits;
 	for (i=0; i<cm->m_TexSize; i++)
 	{
 		byte *pSrc = &pData[d3dlr.Pitch*i];
 		for (j=0; j<cm->m_TexSize; j++)
 		{
 			float fMul = (float)pSrc[3] / 255.0f * HDR_FAKE_MAXOVERBRIGHT;
 			col[0] += pSrc[2] / 255.0f * fMul;
 			col[1] += pSrc[1] / 255.0f * fMul;
 			col[2] += pSrc[0] / 255.0f * fMul;
 
 			pSrc += 4; 
 		}
 	}
 	pTargSurf->UnlockRect();
 	SAFE_RELEASE(pTargSurf);
 	SAFE_RELEASE(pSrcSurf);
 	cm->m_pTex->UnLock();
 #elif defined (DIRECT3D10)
 	assert(0);
 #endif
 	int size = cm->m_TexSize * cm->m_TexSize;
 	cm->m_nFrameCreated[nSide] = -1;
 	cm->m_EnvColors[nSide] = col / (float)size;
 	//if (cm->m_pQuery[nSide])
 	//{
 	//  LPDIRECT3DQUERY9 pQuery = (LPDIRECT3DQUERY9)cm->m_pQuery[nSide];
 	//  pQuery->Issue(D3DISSUE_END);
 	//}
 
 	if (cm->m_EnvColors[nSide][0] > 1 || cm->m_EnvColors[nSide][1] > 1 || cm->m_EnvColors[nSide][2] > 1)
 		cm->m_EnvColors[nSide].NormalizeCol(cm->m_EnvColors[nSide]);
 }
 
 HRESULT GetSampleOffsets_DownScale4x4( DWORD dwWidth, DWORD dwHeight, Vec4 avSampleOffsets[]);
 
 static void sSceneToSceneScaled8x8(CShader *pSH, CTexture *pDstTex, int nSrcTexSize, int nSide)
 {
 	assert(nSrcTexSize == 8);
 	CD3D9Renderer *r = gcpRendD3D;
 	int nTempTexSize = nSrcTexSize >> 1;
 
 	ETEX_Format eTF = r->IsHDRModeEnabled() ? eTF_A16B16G16R16F : eTF_A8R8G8B8;
 	SDynTexture *pTempTex = new SDynTexture(nTempTexSize, nTempTexSize, eTF, eTT_2D, FT_NOMIPS, "TempEnvCMap");
 	pTempTex->Update(nTempTexSize, nTempTexSize);
 	CTexture *tpDst = pTempTex->m_pTexture;
 
 	Vec4 avSampleOffsets[16];
 	SD3DSurface *srfDepth = r->FX_GetDepthSurface(nTempTexSize, nTempTexSize, false);
 	r->FX_SetRenderTarget(0, tpDst, srfDepth);
 	r->RT_SetViewport(0, 0, nTempTexSize, nTempTexSize);
 	GetSampleOffsets_DownScale4x4(nSrcTexSize, nSrcTexSize, avSampleOffsets);
 	static CCryName SampleOffsetsName("SampleOffsets");
 	pSH->FXSetPSFloat(SampleOffsetsName, avSampleOffsets, 16);
 	static CCryName DownScale4x4_techName("DownScale4x4");
 	r->DrawFullScreenQuad(CShaderMan::m_shHDRPostProcess, DownScale4x4_techName, 0, 0, 1, 1);
 
 	r->FX_SetRenderTarget(0, pDstTex, srfDepth, false, false, nSide);
 	STexState TexStatePoint = STexState(FILTER_POINT, true);
 	tpDst->Apply(0, CTexture::GetTexState(TexStatePoint));
 	r->RT_SetViewport(0, 0, 1, 1);
 	static CCryName DownScale4x4_EncodeLDR_techName("DownScale4x4_EncodeLDR");
 	r->DrawFullScreenQuad(CShaderMan::m_shHDRPostProcess, DownScale4x4_EncodeLDR_techName, 0, 0, 1, 1);
 
 	SAFE_DELETE(pTempTex);
 }
 
 bool CTexture::ScanEnvironmentCube(SEnvTexture *pEnv, uint32 nRendFlags, int Size, bool bLightCube)
 {
 	if (pEnv->m_bInprogress)
 		return true;
 
 	HRESULT hr = S_OK;
 	CD3D9Renderer *r = gcpRendD3D;
 	int vX, vY, vWidth, vHeight;
 	r->GetViewport(&vX, &vY, &vWidth, &vHeight);
 	r->EF_PushFog();
 
 	int Start, End;
 	if (!pEnv->m_bReady || CRenderer::CV_r_envcmwrite)
 	{
 		Start = 0;
 		End = 6;
 	}
 	else
 	{
 		Start = pEnv->m_MaskReady;
 		End = pEnv->m_MaskReady+1;
 		//Start = 0;
 		//End = 6;
 	}
 	int nPersFlags = r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags;
 	int nPersFlags2 = r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags2;
 	r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_DRAWTOTEXTURE;
 	r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_ENCODE_HDR;
 
 	int nTexSize = 0;
 	int nTempTexSize = nTexSize;
 	if (bLightCube)
 	{
 		nTexSize = 1;
 		nTempTexSize = 8;
 	}
 	else
 	{
 		switch (CRenderer::CV_r_envcmresolution)
 		{
 		case 0:
 			nTexSize = 64;
 			break;
 		case 1:
 			nTexSize = 128;
 			break;
 		case 2:
 		default:
 			nTexSize = 256;
 			break;
 		case 3:
 			nTexSize = 512;
 			break;
 		}
 		nTexSize = sLimitSizeByScreenRes(nTexSize);
 		nTempTexSize = nTexSize;
 	}
 
 	int n;
 	Vec3 cur_pos;
 
 	pEnv->m_bInprogress = true;
 	pEnv->m_pTex->Update(nTexSize, nTexSize);
 	ETEX_Format eTF = r->IsHDRModeEnabled() ? eTF_A16B16G16R16F : eTF_A8R8G8B8;
 	SDynTexture *pTempTex = new SDynTexture(nTempTexSize, nTempTexSize, eTF, eTT_2D, FT_NOMIPS, "TempEnvCMap");
 	pTempTex->Update(nTempTexSize, nTempTexSize);
 	pEnv->m_TexSize = nTexSize;
 
 	CTexture *tpDst = pEnv->m_pTex->m_pTexture;
 	CTexture *tpSrc = pTempTex->m_pTexture;
 	Vec3 Pos;
 	if (r->m_RP.m_pRE)
 		r->m_RP.m_pRE->mfCenter(Pos, r->m_RP.m_pCurObject);
 	else
 		Pos = pEnv->m_ObjPos;
 	nRendFlags &= ~DLD_ENTITIES;
 
 	I3DEngine *eng = (I3DEngine *)gEnv->p3DEngine;
 	float fMaxDist = eng->GetMaxViewDistance();
 	if (bLightCube)
 		fMaxDist *= 0.25f;
 	int *pFR = (int *)r->EF_Query(EFQ_Pointer2FrameID);
 	for(n=Start; n<End; n++)
 	{ 
 		(*pFR)++;
 
 		if (bLightCube)
 		{
 			/*if (!pEnv->m_pRenderTargets[n])
 			{
 			IDirect3DSurface9* pSurface;
 			hr = r->m_pd3dDevice->CreateRenderTarget(tex_size, tex_size, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &pSurface, NULL);
 			if (!FAILED(hr))
 			pEnv->m_pRenderTargets[n] = pSurface;
 			}*/
 			/*if (!pEnv->m_pQuery[n])
 			{
 			LPDIRECT3DQUERY9 pQuery;
 			hr = r->m_pd3dDevice->CreateQuery(D3DQUERYTYPE_EVENT, &pQuery);
 			pEnv->m_pQuery[n] = pQuery;
 			pQuery->Issue(D3DISSUE_END);
 			}
 			else*/
 			//if (pEnv->m_nFrameCreated[n] >= 0)
 			//  continue;
 		}
 
 		// render object
 		SD3DSurface *srfDepth = r->FX_GetDepthSurface(nTempTexSize, nTempTexSize, false);
 		bool bRes = r->FX_PushRenderTarget(0, tpSrc, srfDepth);
 		ColorF col = r->m_cClearColor;
 		col.a = 0;
 		r->EF_ClearBuffers(FRT_CLEAR, &col);
 		r->FX_Commit();
 		DrawCubeSide(Pos, nTempTexSize, n, nRendFlags, fMaxDist);
 
 		// Post process (bluring)
 		if (tpDst) // || pEnv->m_pRenderTargets[n])
 		{
 			STexState TexStatePoint = STexState(FILTER_POINT, true);
 			tpSrc->Apply(0, CTexture::GetTexState(TexStatePoint));
 
 			if (!bLightCube)
 			{
 #if defined(DIRECT3D10)
 				// COMMENT: CarstenW
 				// Can no longer bind 2d depth buffer when rendering to cube map faces. As per DX11 spec a cube map depth 
 				// buffer needs to be used (driver crashed when binding the targets in FX_Commit/FX_SetActiveRenderTargets). 
 				// For the following blitting op there's no depth buffer needed anyway so simply don't bind one to fix the 
 				// problem. Needs to be verified once auto cube map rendering is fully supported again (if ever).
 				r->EF_SetState(GS_NODEPTHTEST);
 				r->FX_SetRenderTarget(0, tpDst, 0, false, false, n);
 #else
 				r->FX_SetRenderTarget(0, tpDst, srfDepth, false, false, n);
 #endif
 				r->RT_SetViewport(0, 0, nTexSize, nTexSize);
 				static CCryName techName("Encode_ToLDR");
 				r->DrawFullScreenQuad(CShaderMan::m_shHDRPostProcess, techName, 0, 0, 1, 1);
 			}
 			else
 				sSceneToSceneScaled8x8(CShaderMan::m_shHDRPostProcess, tpDst, nTempTexSize, n);
 			assert(n < 6);
 			pEnv->m_nFrameCreated[n] = r->GetFrameID(false);
 		}
 		r->FX_PopRenderTarget(0);
 	}
 
 	if (CRenderer::CV_r_envcmwrite)
 	{
 		/*for(n=Start; n<End; n++)
 		{ 
 		static char* cubefaces[6] = {"posx","negx","posy","negy","posz","negz"};
 		char str[64];
 		int width = tex_size;
 		int height = tex_size;
 		byte *pic = new byte [width * height * 4];
 		LPDIRECT3DSURFACE9 pSysSurf, pTargetSurf;
 		LPDIRECT3DTEXTURE9 pID3DSysTexture;
 		D3DLOCKED_RECT d3dlrSys;
 		if (!pID3DTargetTextureCM)
 		pTargetSurf = (LPDIRECT3DSURFACE9)pEnv->m_pRenderTargets[n];
 		else
 		h = pID3DTargetTextureCM->GetCubeMapSurface((D3DCUBEMAP_FACES)n, 0, &pTargetSurf);
 		h = D3DXCreateTexture(r->GetDevice(), tex_size, tex_size, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pID3DSysTexture);
 		h = pID3DSysTexture->GetSurfaceLevel(0, &pSysSurf);
 		if (pID3DTargetTexture)
 		h = D3DXLoadSurfaceFromSurface(pSysSurf, NULL, NULL, pTargetSurf, NULL, NULL,  D3DX_FILTER_NONE, 0);
 		else
 		h = r->m_pd3dDevice->GetRenderTargetData(pTargetSurf, pSysSurf);
 		h = pID3DSysTexture->LockRect(0, &d3dlrSys, NULL, 0);
 		byte *src = (byte *)d3dlrSys.pBits;
 		for (int i=0; i<width*height; i++)
 		{
 		*(uint32 *)&pic[i*4] = *(uint32 *)&src[i*4];
 		Exchange(pic[i*4+0], pic[i*4+2]);
 		}
 		h = pID3DSysTexture->UnlockRect(0);
 		sprintf(str, "Cube_%s.jpg", cubefaces[n]);
 		WriteJPG(pic, width, height, str, 24); 
 		delete [] pic;
 		if (pID3DTargetTextureCM)
 		SAFE_RELEASE (pTargetSurf);
 		SAFE_RELEASE (pSysSurf);
 		SAFE_RELEASE (pID3DSysTexture);
 		}*/
 		CRenderer::CV_r_envcmwrite = 0;
 	}
 	SAFE_DELETE(pTempTex);
 
 	r->RT_SetViewport(vX, vY, vWidth, vHeight);
 	pEnv->m_bInprogress = false;
 	pEnv->m_MaskReady = End;
 	if (pEnv->m_MaskReady == 6)
 	{
 		pEnv->m_MaskReady = 0;
 		pEnv->m_bReady = true;
 	}
 	r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags = nPersFlags;
 	r->m_RP.m_TI[r->m_RP.m_nProcessThreadID].m_PersFlags2 = nPersFlags2;
 
 	r->EF_PopFog();
 
 	return true;
 }
 
 void CTexture::GetBackBuffer(CTexture *pTex, int nRendFlags)
 {
 #if defined (DIRECT3D9) || defined(OPENGL)
   if (!CTexture::IsTextureExist(pTex))
 	{
 		//iLog->Log("Error: %s", pTex->GetName());
 		return;
 	}
 
 	// Beware with e_widescreen usage. Only copy visible screen area.
 	int iTempX, iTempY, iWidth, iHeight;
 	gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
 	RECT pSrcRect={iTempX, iTempY, iTempX+iWidth, iTempY+iHeight };
 	RECT pDstRect={0, 0, iWidth, iHeight };
 
 	// recreate texture when necessary
 	pTex->Invalidate(iWidth, iHeight, eTF_Unknown);
 
 	ETEX_Format eTF = eTF_A8R8G8B8;
 	// if not created yet, create texture
 	if (!pTex->GetDevTexture())
 	{
 		if (!pTex->CreateRenderTarget(eTF))
 		{
 			return;
 		}
 		// Clear render target surface before using it
 		ColorF c = ColorF(0, 0, 0, 1);
 		pTex->Fill(c);
 	}
 
 	gcpRendD3D->Logv(SRendItem::m_RecurseLevel[gcpRendD3D->m_RP.m_nProcessThreadID], "Copying backbuffer into texture: %s\n", pTex->GetName()); 
 
 #if !defined(XENON)
 	// get texture surface
 	D3DSurface *pTexSurfCopy = pTex->GetSurface(0, 0);
 	if (!pTexSurfCopy)
 		return;
 
 	D3DSurface *pBackSurface=gcpRendD3D->GetBackSurface(); 
 	HRESULT hr = gcpRendD3D->m_pd3dDevice->StretchRect(pBackSurface, &pSrcRect, pTexSurfCopy, &pDstRect, D3DTEXF_NONE);
 	SAFE_RELEASE(pTexSurfCopy);
 
 
 
 
 
 
 
 
 
 #endif
 
 	gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_RTCopied++;
 	gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_RTCopiedSize += pTex->GetDeviceDataSize();
 
 #elif defined (DIRECT3D10)
 	assert(0);
 #endif
 }
 
 CTexture *CTexture::MakeSpecularTexture(float fExp)
 {
 	assert(0);
 	return NULL;
 }
 
 //================================================================================
 
 VOID CFnMap::Fill2DWrapper(D3DXVECTOR4* pOut, const D3DXVECTOR2* pTexCoord, const D3DXVECTOR2* pTexelSize, LPVOID pData)
 {
 	CFnMap* map = (CFnMap*)pData;
 	const D3DXCOLOR& c = map->Function(pTexCoord, pTexelSize);
 	*pOut = D3DXVECTOR4((const float*)c);
 }
 
 int CFnMap::Initialize()
 {
 #if defined (DIRECT3D9) || defined(OPENGL)
 
 	D3DTexture *pTexture;
 	HRESULT hr;
 	LPDIRECT3DDEVICE9 dv = gcpRendD3D->GetD3DDevice();
 
 	hr = D3DXCreateTexture(gcpRendD3D->GetDevice(), m_dwWidth, m_dwHeight, m_nLevels, 0, m_Format, TEXPOOL, &pTexture);
 	if (FAILED(hr))
 	{
 		OutputDebugString("Can't create texture\n");
 		return hr;
 	}
 
 	if (FAILED(hr=D3DXFillTexture(pTexture, Fill2DWrapper, (void*)this)))
 		return hr;
 
 	CTexture *tp = m_pTex;
 	SAFE_DELETE(tp->m_pFunc);
 	tp->m_pFunc = this;
 	tp->m_pDevTexture = new CDeviceTexture(pTexture);
 	tp->m_nWidth = m_dwWidth;
 	tp->m_eTFDst = tp->m_eTFSrc = CTexture::TexFormatFromDeviceFormat(m_Format);
 	tp->m_nHeight = m_dwHeight;
 	tp->m_nMips = m_nLevels;
 	tp->m_nDeviceSize = CTexture::TextureDataSize(tp->m_nWidth, tp->m_nHeight, tp->m_nDepth, tp->m_nMips, tp->GetDstFormat());
 #elif defined (DIRECT3D10)
 	byte *pData[6] = {0,0,0,0,0,0};
 	CTexture *tp = m_pTex;
 	SAFE_DELETE(tp->m_pFunc);
 	tp->m_pFunc = this;
 	tp->m_nWidth = m_dwWidth;
 	tp->m_nHeight = m_dwHeight;
 	tp->m_nMips = m_nLevels;
 	tp->CreateDeviceTexture(pData);
 #endif
 	return S_OK;
 }
 
 VOID CFnMap3D::Fill3DWrapper(D3DXVECTOR4* pOut, const D3DXVECTOR3* pTexCoord, const D3DXVECTOR3* pTexelSize, LPVOID pData)
 {
 	CFnMap3D* map = (CFnMap3D*)pData;
 	const D3DXCOLOR& c = map->Function(pTexCoord, pTexelSize);
 	*pOut = D3DXVECTOR4((const float*)c);
 }
 
 int CFnMap3D::Initialize()
 {
 #if defined (DIRECT3D9) || defined(OPENGL)
 
 	LPDIRECT3DVOLUMETEXTURE9 pID3DVolTexture = NULL;
 	HRESULT hr;
 	LPDIRECT3DDEVICE9 dv = gcpRendD3D->GetD3DDevice();
 
 	hr = D3DXCreateVolumeTexture (gcpRendD3D->GetDevice(), m_dwWidth, m_dwHeight, m_dwDepth, m_nLevels, 0, m_Format, TEXPOOL, &pID3DVolTexture);
 	if (FAILED(hr))
 	{
 		OutputDebugString("Can't create texture\n");
 		return hr;
 	}
 
 	if (FAILED(hr=D3DXFillVolumeTexture(pID3DVolTexture, Fill3DWrapper, (void*)this)))
 		return hr;
 
 	CTexture *tp = m_pTex;
 	SAFE_DELETE(tp->m_pFunc);
 	tp->m_pFunc = this;
 	tp->m_pDevTexture = new CDeviceTexture(pID3DVolTexture);
 	tp->m_nWidth = m_dwWidth;
 	tp->m_nHeight = m_dwHeight;
 	tp->m_nDepth = m_dwDepth;
 	tp->m_nMips = m_nLevels;
 #elif defined (DIRECT3D10)
 	byte *pData[6] = {0,0,0,0,0,0};
 	CTexture *tp = m_pTex;
 	SAFE_DELETE(tp->m_pFunc);
 	tp->m_pFunc = this;
 	tp->m_nWidth = m_dwWidth;
 	tp->m_nHeight = m_dwHeight;
 	tp->m_nDepth = m_dwDepth;
 	tp->m_nMips = m_nLevels;
 	tp->CreateDeviceTexture(pData);
 #endif
 	return S_OK;
 }
 
 //=======================================================================
 
 D3DXCOLOR CMipsColorMap::Function(const D3DXVECTOR2* pTexCoord, const D3DXVECTOR2* pTexelSize)
 {
 	D3DXCOLOR vOut;
 
 	float d = 1.0f/((float)(pTexCoord->x+pTexCoord->y)/2.0f);
 	Vec3 p = Vec3(0,0,0);
 	if (d < 8)
 	{
 		p = Vec3(1,1,1);
 	}
 	else
 	{
 		float q = logf(d)/logf(2);
 		q = floorf(q+.5f);
 		if (fmodf(q,3) == 0)
 		{
 			p.x = .5;
 		}
 		if (fmodf(q,6) == 0)
 		{
 			p.x = 1;
 		}
 		if (fmodf(q,2) == 0)
 		{
 			p.y = .5;
 		}
 		if (fmodf(q,4) == 0)
 		{
 			p.y = 1;
 		}
 		q = q / m_nLevelMips;
 		p.z = q;
 	}
 	vOut.r = p.x;
 	vOut.g = p.y;
 	vOut.b = p.z;
 	vOut.a = 1;
 
 	return vOut;
 }
 
 CTexture *CTexture::GenerateMipsColorMap(int nWidth, int nHeight)
 {
 	CD3D9Renderer *rd = gcpRendD3D;
 	bool bMips = false;
 	ETEX_Format eTF = eTF_A8R8G8B8;
 	char str[256];
 	sprintf(str, "$MipColors_%dx%d", nWidth, nHeight);
 	CCryName Name = GenName(str, 0, -1, -1);
 	CTexture *pTex = GetByName(Name.c_str());
 	if (pTex)
 		return pTex;
 	pTex = CTexture::CreateTextureObject(str, nWidth, nHeight, 1, eTT_2D,  FT_DONT_RELEASE, eTF);
 	CMipsColorMap *pMap = new CMipsColorMap(nWidth, nHeight, CTexture::DeviceFormatFromTexFormat(eTF), pTex, true);
 	pMap->m_nLevelMips = LogBaseTwo(nWidth);
 	pMap->m_nLevelMips = max(LogBaseTwo(nHeight), pMap->m_nLevelMips);
 	pMap->Initialize();
 	pTex->SetTexStates();
 
 	return pTex;
 }
 
 //==========================================================================================
 
 #define NOISE_DIMENSION 128
 #define NOISE_MIP_LEVELS 1
 
 #if !defined(LINUX)
 static double drand48(void)
 {
 	double dVal;
 	dVal=(double)cry_rand()/(double)RAND_MAX;
 
 	return dVal;
 }
 #endif
 
 void CTexture::GenerateNoiseVolumeMap()
 {
 	s_ptexNoiseVolumeMap = CTexture::ForName("Textures/Defaults/Noise3D.dds", FT_DONT_RELEASE |  FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
 }
 
 //==========================================================================================
 // Volume Map Creation: Noise vectors
 //==========================================================================================
 
 Vec3 CVectorNoiseVolMap::GetNoiseVector()
 {      
 	float alpha = ((float)drand48()) * 2.0f * 3.1415926535f;
 	float beta = ((float)drand48()) * 3.1415926535f;
 	return Vec3(cosf(alpha) * sinf(beta), sinf(alpha) * sinf(beta), cosf(beta));
 }
 
 D3DXCOLOR CVectorNoiseVolMap::Function(const D3DXVECTOR3* p, const D3DXVECTOR3* s)
 {
 	Vec3 pRandVec=GetNoiseVector();
 	pRandVec.x=pRandVec.x*0.5f+0.5f;
 	pRandVec.y=pRandVec.y*0.5f+0.5f;
 	pRandVec.z=pRandVec.z*0.5f+0.5f;
 
 	return D3DXCOLOR(pRandVec.x, pRandVec.y, pRandVec.z, 0);
 }
 
 void CTexture::GenerateVectorNoiseVolMap()
 {
 	int nVolTexSize=CVectorNoiseVolMap::m_nTexSize;
 
 	ETEX_Format eTF = eTF_X8R8G8B8;
 	if(!s_ptexVectorNoiseVolMap)
 	{
 		s_ptexVectorNoiseVolMap = CTexture::CreateTextureObject("$VectorNoiseVolume", nVolTexSize, nVolTexSize, nVolTexSize, eTT_3D,  FT_DONT_RELEASE, eTF);
 	}
 
 	CVectorNoiseVolMap *pMap = new CVectorNoiseVolMap(nVolTexSize, nVolTexSize, nVolTexSize, CTexture::DeviceFormatFromTexFormat(eTF), s_ptexVectorNoiseVolMap, 1);
 	pMap->Initialize();
 	s_ptexVectorNoiseVolMap->SetTexStates();
 }
 
 void CTexture::GenerateFlareMap()
 {
 	int i, j;
 
 	byte data[4][32][4];
 	for (i=0; i<32; i++)
 	{
 		float f = 1.0f - ((fabsf((float)i - 15.5f) - 0.5f) / 16.0f);
 		int n = (int)(f*f*255.0f);
 		for (j=0; j<4; j++)
 		{
 			byte b = n;
 			if (n < 0)
 				b = 0;
 			else
 				if (n > 255)
 					b = 255;
 			data[j][i][0] = b;
 			data[j][i][1] = b;
 			data[j][i][2] = b;
 			data[j][i][3] = 255;
 		}
 	}
 	CTexture::s_ptexFlare = CTexture::Create2DTexture("$Flare", 32, 4, 1, FT_STATE_CLAMP | FT_DONT_RELEASE, &data[0][0][0], eTF_A8R8G8B8, eTF_A8R8G8B8);
 }
 
 void CTexture::GenerateFuncTextures()
 {
 	// GenerateFlareMap();
 	// GenerateNoiseVolumeMap();
 	// GenerateNormalizationCubeMap();
 }
 
 void CTexture::DestroyZMaps()
 {
 	//SAFE_RELEASE(s_ptexZTarget);
 }
 
 void CTexture::GenerateZMaps()
 {
   int nWidth = gcpRendD3D->m_MainViewport.nWidth; //m_d3dsdBackBuffer.Width;
   int nHeight = gcpRendD3D->m_MainViewport.nHeight; //m_d3dsdBackBuffer.Height;
   ETEX_Format eTFZ = CTexture::s_eTFZ;
   uint32 nFlags =  FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_DONT_RELEASE | FT_USAGE_PREDICATED_TILING | FT_DONT_RESIZE;
 #if !defined(PS3)
   if (CRenderer::CV_r_fsaa)
     nFlags |= FT_USAGE_FSAA;
   if (!s_ptexZTarget)
     s_ptexZTarget = CreateRenderTarget("$ZTarget", nWidth, nHeight, eTT_2D, nFlags, eTFZ, -1, 100);
   else
   {
     s_ptexZTarget->m_nFlags = nFlags;
     s_ptexZTarget->m_nWidth = nWidth;
     s_ptexZTarget->m_nHeight = nHeight;
     s_ptexZTarget->CreateRenderTarget(eTFZ);
   }
 
 
 
 
   // Generate stereo textures
   if (CD3D9Renderer::CV_r_Stereo)
   {
     s_ptexStereoL->m_nFlags = nFlags | FT_USAGE_ALLOWREADSRGB;
     s_ptexStereoL->m_nWidth = nWidth;
     s_ptexStereoL->m_nHeight = nHeight;
     s_ptexStereoL->CreateRenderTarget( eTF_A8R8G8B8 );
 
     s_ptexStereoR->m_nFlags = nFlags | FT_USAGE_ALLOWREADSRGB;
     s_ptexStereoR->m_nWidth = nWidth;
     s_ptexStereoR->m_nHeight = nHeight;
     s_ptexStereoR->CreateRenderTarget( eTF_A8R8G8B8 );
   }
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 #endif
 
 }
 
 void CTexture::DestroySceneMap()
 {
 	//SAFE_RELEASE(s_ptexSceneTarget);
 }
 
 void CTexture::GenerateSceneMap(ETEX_Format eTF)
 {
 	int nWidth = gcpRendD3D->GetWidth(); //m_d3dsdBackBuffer.Width;
 	int nHeight = gcpRendD3D->GetHeight(); // m_d3dsdBackBuffer.Height;
 	uint32 nFlags =  FT_USAGE_PREDICATED_TILING | FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_DONT_RELEASE|FT_DONT_RESIZE;
 	if( !gRenDev->IsHDRModeEnabled() )
 	{
 		if( gRenDev->IsLinearSpaceShadingEnabled() )
 			nFlags |= FT_USAGE_ALLOWREADSRGB; 
 	}
 	else
 		nFlags |= (!CRenderer::CV_r_HDRTexFormat)? FT_CUSTOM_FORMAT : 0;
 
 	if (CRenderer::CV_r_fsaa)
 		nFlags |= FT_USAGE_FSAA;
 	if (!s_ptexSceneTarget)
 		s_ptexSceneTarget = CreateRenderTarget("$SceneTarget", nWidth, nHeight, eTT_2D, nFlags, eTF, TO_SCENE_TARGET, 100);
 	else
 	{
 		s_ptexSceneTarget->m_nFlags = nFlags;
 		s_ptexSceneTarget->m_nWidth = nWidth;
 		s_ptexSceneTarget->m_nHeight = nHeight;
 		s_ptexSceneTarget->CreateRenderTarget(eTF);
 	}
 
 	//s_ptexSceneTarget->SetHighQualityRT();
 
 
 
 	//s_ptexSceneTarget->m_bFP10 = false;
 
 	nFlags &= ~(FT_USAGE_FSAA|FT_USAGE_ALLOWREADSRGB|FT_CUSTOM_FORMAT);
 
 	// This RT used for glow pass and shadow mask (group 0) as well
 	if (!CTexture::IsTextureExist(s_ptexBackBuffer))
 		s_ptexBackBuffer = CreateRenderTarget("$BackBuffer", nWidth, nHeight, eTT_2D, nFlags, eTF_A8R8G8B8, TO_BACKBUFFERMAP, 100);
 	else
 	{
 		s_ptexBackBuffer->m_nFlags = nFlags;
 		s_ptexBackBuffer->m_nWidth = nWidth;
 		s_ptexBackBuffer->m_nHeight = nHeight;
 		s_ptexBackBuffer->CreateRenderTarget(eTF_A8R8G8B8);
 	}
 
 
 
 
 }
 
 void CTexture::DestroyLightInfo()
 {
 	s_ptexLightInfo[SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID]-1]->ReleaseDeviceTexture(false);
 }
 
 void CTexture::GenerateLightInfo(ETEX_Format eTF, int nWidth, int nHeight)
 {
 #ifndef XENON
 	DestroyLightInfo();
 
 	int nFlags =  FT_DONT_STREAM | FT_DONT_RELEASE | FT_DONT_RESIZE;
 	if (CRenderer::CV_r_shadersdynamicbranching == 2)
 		nFlags |= FT_USAGE_DYNAMIC;
 	else
 		nFlags |= FT_USAGE_RENDERTARGET;
 	s_ptexLightInfo[SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID]-1]->m_nFlags = nFlags;
 	s_ptexLightInfo[SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID]-1]->Create2DTexture(nWidth, nHeight, 1, nFlags, NULL, eTF, eTF);
 #endif
 }
 
 bool SDynTexture::RT_SetRT(int nRT, int nWidth, int nHeight, bool bPush, bool bScreenVP)
 {
   Update(m_nWidth, m_nHeight);
 
   SD3DSurface *pDepthSurf = nWidth > 0 ? gcpRendD3D->FX_GetDepthSurface(nWidth, nHeight, false) : &gcpRendD3D->m_DepthBufferOrig;
 
   assert(m_pTexture);
   if (m_pTexture)
   {
     if (bPush)
       return gcpRendD3D->FX_PushRenderTarget(nRT, m_pTexture, pDepthSurf, false, -1, bScreenVP);
     else
       return gcpRendD3D->FX_SetRenderTarget(nRT, m_pTexture, pDepthSurf, false, false, -1, bScreenVP);
   }
   return false;
 }
 
 bool SDynTexture::SetRT(int nRT, bool bPush, SD3DSurface *pDepthSurf, bool bScreenVP)
 {
 	Update(m_nWidth, m_nHeight);
 
 	assert(m_pTexture);
 	if (m_pTexture)
 	{
 		if (bPush)
 			return gcpRendD3D->FX_PushRenderTarget(nRT, m_pTexture, pDepthSurf, false, -1, bScreenVP);
 		else
 			return gcpRendD3D->FX_SetRenderTarget(nRT, m_pTexture, pDepthSurf, false, false, -1, bScreenVP);
 	}
 	return false;
 }
 
 bool SDynTexture::RestoreRT(int nRT, bool bPop)
 {
 	if (bPop)
 		return gcpRendD3D->FX_PopRenderTarget(nRT);
 	else
 		return gcpRendD3D->FX_RestoreRenderTarget(nRT);
 }
 
 
 bool SDynTexture2::SetRT(int nRT, bool bPush, SD3DSurface *pDepthSurf, bool bScreenVP)
 {
 	Update(m_nWidth, m_nHeight);
 
 	assert(m_pTexture);
 	if (m_pTexture)
 	{
 		bool bRes = false;
 		if (bPush)
 			bRes = gcpRendD3D->FX_PushRenderTarget(nRT, m_pTexture, pDepthSurf);
 		else
 			bRes = gcpRendD3D->FX_SetRenderTarget(nRT, m_pTexture, pDepthSurf);
 		SetRectStates();
 		gcpRendD3D->FX_Commit();
 	}
 	return false;
 }
 
 bool SDynTexture2::SetRectStates()
 {
 	assert(m_pTexture);
 	gcpRendD3D->RT_SetViewport(m_nX, m_nY, m_nWidth, m_nHeight);
 	gcpRendD3D->EF_Scissor(true, m_nX, m_nY, m_nWidth, m_nHeight);
   RECT rc;
   rc.left = m_nX; rc.right = m_nX+m_nWidth;
   rc.top = m_nY; rc.bottom = m_nY+m_nHeight;
   if (m_pTexture)
     m_pTexture->AddDirtRect(rc);
 	return true;
 }
 
 bool SDynTexture2::RestoreRT(int nRT, bool bPop)
 {
 	bool bRes = false;
 	gcpRendD3D->EF_Scissor(false, m_nX, m_nY, m_nWidth, m_nHeight);
 	if (bPop)
 		bRes = gcpRendD3D->FX_PopRenderTarget(nRT);
 	else
 		bRes = gcpRendD3D->FX_RestoreRenderTarget(nRT);
 	gcpRendD3D->FX_Commit();
 
 	return bRes;
 }
 
 void _DrawText(ISystem * pSystem, int x, int y, const float fScale, const char * format, ...);
 
 static int __cdecl RTCallback(const VOID* arg1, const VOID* arg2)
 {
 	CTexture *p1 = *(CTexture **)arg1;
 	CTexture *p2 = *(CTexture **)arg2;
 
   // show big textures first
   int nSize1 = p1->GetDataSize();
   int nSize2 = p2->GetDataSize();
   if(nSize1>nSize2)
     return -1;
   else if(nSize2>nSize1)
     return 1;
 
 	return strcmp(p1->GetName(),p2->GetName());
 }
 
 void CD3D9Renderer::DrawAllDynTextures(const char *szFilter, const bool bLogNames, const bool bOnlyIfUsedThisFrame)
 {
 	SDynTexture2::TextureSet2Itor itor;
 	char name[256]; //, nm[256];
 	strcpy_s(name, szFilter);
 	strlwr(name);
 	TArray<CTexture *> UsedRT;
 	int nMaxCount = CV_r_ShowDynTexturesMaxCount;
 
 	float width = 800;
 	float height = 600;
 	float fArrDim = max(1.f, sqrtf((float)nMaxCount));
 	float fPicDimX = width/fArrDim;
 	float fPicDimY = height/fArrDim;
 	float x = 0;
 	float y = 0;
 	Set2DMode(true, (int)width, (int)height);
 	EF_SelectTMU(0);
 	EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
 	if (name[0] == '*' && !name[1])
 	{
 		SResourceContainer *pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
 		ResourcesMapItor it;
 		for (it=pRL->m_RMap.begin(); it!=pRL->m_RMap.end(); it++)
 		{
 			CTexture *tp = (CTexture *)it->second;
 			if (tp && !tp->IsNoTexture())
 			{
 				if ((tp->GetFlags() & (FT_USAGE_RENDERTARGET | FT_USAGE_DYNAMIC)) && tp->GetDevTexture())
 					UsedRT.AddElem(tp);
 			}
 		}
 	}
 	else
 	{
 		SResourceContainer *pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
 		ResourcesMapItor it;
 		for (it=pRL->m_RMap.begin(); it!=pRL->m_RMap.end(); it++)
 		{
 			CTexture *tp = (CTexture *)it->second;
 			if (!tp || tp->IsNoTexture())
 				continue;
 			if ((tp->GetFlags() & (FT_USAGE_RENDERTARGET | FT_USAGE_DYNAMIC)) && tp->GetDevTexture())
 			{
 				char nameBuffer[128];
 				strncpy(nameBuffer, tp->GetName(), sizeof nameBuffer);
 				nameBuffer[sizeof nameBuffer - 1] = 0;
 				strlwr(nameBuffer);
 				if (CryStringUtils::MatchWildcard(nameBuffer, name))
 					UsedRT.AddElem(tp);
 			}
 		}
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 	}
 	if (true /* szFilter[0] == '*' */)
 	{
 		if (UsedRT.Num() > 1)
 		{
 			qsort(&UsedRT[0], UsedRT.Num(), sizeof(CTexture *), RTCallback);
 		}
 	}
 	fPicDimX = width/fArrDim;
 	fPicDimY = height/fArrDim;
 	x = 0;
 	y = 0;
 	for (uint32 i=0; i<UsedRT.Num(); i++)
 	{
 		SetState(GS_NODEPTHTEST);
 		CTexture *tp = UsedRT[i];
 		int nSavedAccessFrameID = tp->m_nAccessFrameID;
 
 		if(bOnlyIfUsedThisFrame)
 			if(nSavedAccessFrameID != m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID)
 				continue;
 
 		if (tp->GetTextureType() == eTT_2D)
 			Draw2dImage(x, y, fPicDimX-2, fPicDimY-2, tp->GetID(), 0,1,1,0,0);
 		else
 		{
 			float fSizeX = fPicDimX / 3;
 			float fSizeY = fPicDimY / 2;
 			float fx = ScaleCoordX(x); fSizeX = ScaleCoordX(fSizeX);
 			float fy = ScaleCoordY(y); fSizeY = ScaleCoordY(fSizeY);
 			float fOffsX[] = {fx, fx+fSizeX, fx+fSizeX*2, fx, fx+fSizeX, fx+fSizeX*2};
 			float fOffsY[] = {fy, fy, fy, fy+fSizeY, fy+fSizeY, fy+fSizeY};
 			Vec3 vTC0[] = {Vec3(1,1,1), Vec3(-1,1,-1), Vec3(-1,1,-1), Vec3(-1,-1,1), Vec3(-1,1,1), Vec3(1,1,-1)};
 			Vec3 vTC1[] = {Vec3(1,1,-1), Vec3(-1,1,1), Vec3(1,1,-1), Vec3(1,-1,1), Vec3(1,1,1), Vec3(-1,1,-1)};
 			Vec3 vTC2[] = {Vec3(1,-1,-1), Vec3(-1,-1,1), Vec3(1,1,1), Vec3(1,-1,-1), Vec3(1,-1,1), Vec3(-1,-1,-1)};
 			Vec3 vTC3[] = {Vec3(1,-1,1), Vec3(-1,-1,-1), Vec3(-1,1,1), Vec3(-1,-1,-1), Vec3(-1,-1,1), Vec3(1,-1,-1)};
 
 			m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Push();
 			EF_PushMatrix();
 			Matrix44A *m = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop();
 			mathMatrixOrthoOffCenterLH(m, 0.0f, (float)m_width, (float)m_height, 0.0f, -1e30f, 1e30f);
 			m = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->GetTop();
 			m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->LoadIdentity();
 
 			EF_SetColorOp(eCO_REPLACE, eCO_REPLACE, DEF_TEXARG0, DEF_TEXARG0);
 			STexState ts(FILTER_LINEAR, false);
 			ts.m_nAnisotropy = 1;
 			tp->Apply(0, CTexture::GetTexState(ts));
 			SetCullMode(R_CULL_NONE);
 
 			for (int iSide=0; iSide<6; iSide++)
 			{
 				int nOffs;
 				SVF_P3F_T3F *vQuad = (SVF_P3F_T3F *)GetVBPtr(4, nOffs, POOL_P3F_TEX3F);
 				if (!vQuad)
 					return;
 #if defined(OPENGL)
 				vQuad[0].p = Vec3(fOffsX[iSide], fOffsY[iSide]+fSizeY-1, 1);
 				vQuad[0].st = vTC0[iSide];
 				vQuad[1].p = Vec3(fOffsX[iSide]+fSizeX-1, fOffsY[iSide]+fSizeY-1, 1);
 				vQuad[1].st = vTC1[iSide];
 				vQuad[2].p = Vec3(fOffsX[iSide]+fSizeX-1, fOffsY[iSide], 1);
 				vQuad[2].st = vTC2[iSide];
 				vQuad[3].p = Vec3(fOffsX[iSide], fOffsY[iSide], 1);
 				vQuad[3].st = vTC3[iSide];
 #else
 				vQuad[0].p = Vec3(fOffsX[iSide], fOffsY[iSide], 1);
 				vQuad[0].st = vTC0[iSide];
 				vQuad[1].p = Vec3(fOffsX[iSide]+fSizeX-1, fOffsY[iSide], 1);
 				vQuad[1].st = vTC1[iSide];
 				vQuad[2].p = Vec3(fOffsX[iSide]+fSizeX-1, fOffsY[iSide]+fSizeY-1, 1);
 				vQuad[2].st = vTC2[iSide];
 				vQuad[3].p = Vec3(fOffsX[iSide], fOffsY[iSide]+fSizeY-1, 1);
 				vQuad[3].st = vTC3[iSide];
 #endif
 
 				// We are finished with accessing the vertex buffer
 				UnlockVB(POOL_P3F_TEX3F);
 
 				if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_T3F)))
 				{
 					// Bind our vertex as the first data stream of our device
 					FX_SetVStream(0, m_pVB[POOL_P3F_TEX3F], 0, sizeof(SVF_P3F_T3F));
 					FX_SetFPMode();
 
 					// Render the two triangles from the data stream
 #if defined (DIRECT3D9) || defined(OPENGL)
 					HRESULT hReturn = m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, nOffs, 2);
 #elif defined (DIRECT3D10)
 					assert(0);
 #endif
 				}
 			}
 			EF_PopMatrix();
 			m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();
 		}
 		tp->m_nAccessFrameID = nSavedAccessFrameID;
 
 		const char *pTexName = tp->GetName();
 		char nameBuffer[128];
 		memset(nameBuffer, 0, sizeof nameBuffer);
 		for (int iStr = 0, jStr = 0; pTexName[iStr] && jStr < sizeof nameBuffer - 1; ++iStr)
 		{
 			if (pTexName[iStr] == '$')
 			{
 				nameBuffer[jStr] = '$';
 				nameBuffer[jStr + 1] = '$';
 				jStr += 2;
 			}
 			else
 			{
 				nameBuffer[jStr] = pTexName[iStr];
 				++jStr;
 			}
 		}
 		nameBuffer[sizeof nameBuffer - 1] = 0;
 		pTexName = nameBuffer;
 
 		int32 nPosX = (int32)ScaleCoordX(x);
 		int32 nPosY = (int32)ScaleCoordY(y);
 		_DrawText(iSystem, nPosX, nPosY, 1.0f, "%8s", pTexName);
 		_DrawText(iSystem, nPosX, nPosY+=10, 1.0f, "%d-%d", tp->m_nUpdateFrameID, tp->m_nAccessFrameID);
 		_DrawText(iSystem, nPosX, nPosY+=10, 1.0f, "%dx%d", tp->GetWidth(), tp->GetHeight());
  
 		if(bLogNames)
 		{
 			iLog->Log("Mem:%d  %dx%d  Type:%s  Format:%s (%s)",
 				tp->GetDeviceDataSize(),tp->GetWidth(), tp->GetHeight(),tp->NameForTextureType(tp->GetTextureType()), tp->NameForTextureFormat(tp->GetDstFormat()), tp->GetName());
 		}
 
 		x += fPicDimX;
 		if (x >= width-10)
 		{
 			x = 0;
 			y += fPicDimY;
 		}
 	}
 
 	Set2DMode(false, m_width, m_height);
 	
 	RT_FlushTextMessages(); 
 }
 
 //////////////////////////////////////////////////////////////////////////
 bool CFlashTextureSource::Update(float distToCamera)
 {
 	// sanity check: need player and some texture to draw to
 	assert(m_pFlashPlayer && m_pDynTexture);
 	if (!m_pFlashPlayer || !m_pDynTexture)
 		return false;
 
 	// determine if texture resize is required
 	int newWidth(0), newHeight(0);
 	CalcSize(newWidth, newHeight, distToCamera);
 	bool needResize(newWidth != m_width || newHeight != m_height);
 
 	// resize texture
 	bool forceUpdate(false);
 	if (!m_pDynTexture->IsValid() || needResize)
 	{
 		if (!m_pDynTexture->Update(newWidth, newHeight))
 			return false;
 
 		forceUpdate = true;
 
 		assert(newWidth <= m_pDynTexture->m_nWidth && newHeight <= m_pDynTexture->m_nHeight);
 		m_width = newWidth;
 		m_height = newHeight;
 	}
 
 	// update texture if required
 	float currentTime(gEnv->pTimer->GetCurrTime());
 	float updateTimeDelta = m_highQualityRender ? 0 : CRenderer::CV_r_envtexupdateinterval;
 	int updateFrameID = gRenDev->GetFrameID(false);
 	if (forceUpdate || updateFrameID != m_lastUpdateFrameID && (currentTime > m_lastUpdateTime + updateTimeDelta || currentTime < m_lastUpdateTime))
 	{
 		float ar = ((float) m_pFlashPlayer->GetWidth() / (float) m_width) * ((float) m_height / (float)m_pFlashPlayer->GetHeight());
 		m_pFlashPlayer->SetViewport(m_pDynTexture->m_nX, m_pDynTexture->m_nY, m_width, m_height, ar);
 		m_pFlashPlayer->SetScissorRect(m_pDynTexture->m_nX, m_pDynTexture->m_nY, m_width, m_height);
 		m_pFlashPlayer->SetBackgroundAlpha(0.0f);
 
 		uint32 dynTexRectX(0), dynTexRectY(0), dynTexRectWidth(0), dynTexRectHeight(0);
 		m_pDynTexture->GetImageRect(dynTexRectX, dynTexRectY, dynTexRectWidth, dynTexRectHeight);
 		m_pDynTexture->SetRT(0, true, gcpRendD3D->FX_GetDepthSurface(dynTexRectWidth, dynTexRectHeight, false));
 #if defined(DIRECT3D10)
 		int clearFlags(FRT_CLEAR_COLOR | FRT_CLEAR_IMMEDIATE);
 #else
 		int clearFlags(FRT_CLEAR_COLOR);
 #endif
 		ColorF clearCol(0, 0, 0, 0);
 		gcpRendD3D->EF_ClearBuffers(clearFlags, &clearCol);
 		gcpRendD3D->FX_Commit(true);
 		m_pFlashPlayer->Advance(max(currentTime - m_lastUpdateTime, 0.0f));
 		m_pFlashPlayer->Render();
 		m_pDynTexture->RestoreRT(0, true);
 		gcpRendD3D->EF_SetState(gcpRendD3D->m_RP.m_CurState & ~GS_BLEND_MASK);
 
 		m_pDynTexture->SetUpdateMask();
 		m_lastUpdateTime = currentTime;
 		m_lastUpdateFrameID = updateFrameID;
 	}
 
 	return true;
 }
 
 //////////////////////////////////////////////////////////////////////////
 bool CFlashTextureSource::Update(CTexture* pDstRT)
 {
 	// sanity check: need player and some texture to draw to
 	assert(m_pFlashPlayer || pDstRT);
 	if (!m_pFlashPlayer || !pDstRT)
 		return false;
 
 	// set view port size and add 2 pixels border  to avoid bilinear filtering artifacts
 	int newWidth = m_pFlashPlayer->GetWidth() + 2;
 	int newHeight = m_pFlashPlayer->GetHeight() + 2;
 
 	// and must be dividable by 8 for resolves to work on x360
 	newWidth += 8 - newWidth %8;
 	newHeight += 8- newHeight %8;
 
 	newWidth = min(newWidth, pDstRT->GetWidth());
 	newHeight = min(newHeight, pDstRT->GetHeight());
 
 	uint32 nDstWidth = newWidth;//pDstRT->GetWidth();
 	uint32 nDstHeight = newHeight;//pDstRT->GetHeight();
 
 	// update texture if required
 	float currentTime(gEnv->pTimer->GetCurrTime());
 	float ar = ((float) m_pFlashPlayer->GetWidth() / (float) nDstWidth) * ((float) nDstHeight / (float)m_pFlashPlayer->GetHeight());
 
 	m_pFlashPlayer->SetViewport(0, 0, nDstWidth, nDstHeight, ar);
 	m_pFlashPlayer->SetScissorRect(0, 0, nDstWidth, nDstHeight);
 	m_pFlashPlayer->SetBackgroundAlpha(0.0f);
 
 	uint32 dynTexRectX(0), dynTexRectY(0), dynTexRectWidth(0), dynTexRectHeight(0);
 	gcpRendD3D->FX_PushRenderTarget(0,  pDstRT,  &gcpRendD3D->m_DepthBufferOrig);
 	gcpRendD3D->RT_SetViewport(0, 0, nDstWidth, nDstHeight );
 
 #if defined(DIRECT3D10)
 	int clearFlags(FRT_CLEAR_COLOR | FRT_CLEAR_IMMEDIATE);
 #else
 	int clearFlags(FRT_CLEAR_COLOR);
 #endif
 	ColorF clearCol(0, 0, 0, 0);
 	gcpRendD3D->EF_ClearBuffers(clearFlags, &clearCol);
 	gcpRendD3D->FX_Commit(true);
 
 	m_pFlashPlayer->Advance(max(currentTime - m_lastUpdateTime, 0.0f));
 	m_pFlashPlayer->Render();
 
 	gcpRendD3D->EF_SetState(gcpRendD3D->m_RP.m_CurState & ~GS_BLEND_MASK);
 
 	m_lastUpdateTime = currentTime;
 
 
 
 
 
 
 	gcpRendD3D->FX_PopRenderTarget(0);
 
 	return true;
 }
 
 //////////////////////////////////////////////////////////////////////////
 bool CVideoTextureSource::Update(float distToCamera)
 {
 	// sanity check: need player and some texture to draw to
 	assert(m_pVideoPlayer && m_pDynTexture);
 	if (!m_pVideoPlayer || !m_pDynTexture)
 		return false;
 
 	// determine if texture resize is required
 	int newWidth(0), newHeight(0);
 	CalcSize(newWidth, newHeight, distToCamera);
 	bool needResize(newWidth != m_width || newHeight != m_height);
 
 	// resize texture
 	bool forceUpdate(false);
 	if (!m_pDynTexture->IsValid() || needResize)
 	{
 		if (!m_pDynTexture->Update(newWidth, newHeight))
 			return false;
 
 		forceUpdate = true;
 
 		assert(newWidth <= m_pDynTexture->m_nWidth && newHeight <= m_pDynTexture->m_nHeight);
 		m_width = newWidth;
 		m_height = newHeight;
 	}
 
 	// update texture if required
 	float currentTime(gEnv->pTimer->GetCurrTime());
 	int updateFrameID = gRenDev->GetFrameID(false);
 	if (forceUpdate || updateFrameID != m_lastUpdateFrameID && (currentTime > m_lastUpdateTime + CRenderer::CV_r_envtexupdateinterval || currentTime < m_lastUpdateTime))
 	{
 		m_pVideoPlayer->SetViewport(m_pDynTexture->m_nX, m_pDynTexture->m_nY, m_width, m_height);
 		m_pDynTexture->SetRT(0, true, 0);
 #if defined(DIRECT3D10)
 		int clearFlags(FRT_CLEAR_COLOR | FRT_CLEAR_IMMEDIATE);
 #else
 		int clearFlags(FRT_CLEAR_COLOR);
 #endif
 		ColorF clearCol(0, 0, 0, 0);
 		gcpRendD3D->EF_ClearBuffers(clearFlags, &clearCol);
 		gcpRendD3D->FX_Commit(true);
 		m_pVideoPlayer->Render();
 		m_pDynTexture->RestoreRT(0, true);
 		gcpRendD3D->EF_SetState(gcpRendD3D->m_RP.m_CurState & ~GS_BLEND_MASK);
 
 		m_pDynTexture->SetUpdateMask();
 		m_lastUpdateTime = currentTime;
 		m_lastUpdateFrameID = updateFrameID;
 	}
 
 	return true;
 }
 
 //////////////////////////////////////////////////////////////////////////
 bool CVideoTextureSource::Update(CTexture* pDstRT)
 {
 	assert(0);
 	return false;
 }
 
 bool CTexture::SaveDDS(const char *szName, bool bMips)
 {
 	if (!m_pDevTexture)
 		return false;
 	HRESULT hr = S_OK;
 #if defined (DIRECT3D9) || defined (OPENGL)
 
 
 
 
 
 
 	hr = D3DXSaveTextureToFile(szName, D3DXIFF_DDS, m_pDevTexture->Get2DTexture(), NULL);
 #elif defined (DIRECT3D10)
 	hr = D3DX11SaveTextureToFile(gcpRendD3D->GetDeviceContext(), m_pDevTexture->Get2DTexture(), D3DX11_IFF_DDS, szName);
 #endif
 	return (hr == S_OK);
 }
 
 bool CTexture::SaveJPG(const char *szName, bool bMips)
 {
 	if (!m_pDevTexture)
 		return false;
 	HRESULT hr = S_OK;
 #if defined (DIRECT3D9) || defined (OPENGL)
 	D3DXSaveTextureToFile(szName, D3DXIFF_JPG, m_pDevTexture->Get2DTexture(), NULL);
 #elif defined (DIRECT3D10)
 	hr = D3DX11SaveTextureToFile(gcpRendD3D->GetDeviceContext(), m_pDevTexture->Get2DTexture(), D3DX11_IFF_JPG, szName);
 #endif
 	return (hr == S_OK);
 }
 
 bool CTexture::SaveTGA(const char *szName, bool bMips)
 {
 	if (!m_pDevTexture)
 		return false;
 	HRESULT hr = S_OK;
 #if defined (DIRECT3D9) || defined (OPENGL)
 	hr = D3DXSaveTextureToFile(szName, D3DXIFF_TGA, m_pDevTexture->Get2DTexture(), NULL);
 #elif defined (DIRECT3D10)
 	hr = D3DX11SaveTextureToFile(gcpRendD3D->GetDeviceContext(), m_pDevTexture->Get2DTexture(), D3DX11_IFF_TIFF, szName);
 #endif
 	return (hr == S_OK);
 }
 
