#include "StdAfx.h"


#ifndef EXCLUDE_SCALEFORM_SDK


#include "GTextureXRender.h"
#include "GRendererXRender.h"
#include "FlashPlayerInstance.h"
#include "SharedStates.h"
#include <StringUtils.h>
#include <GImage.h>

#include <IRenderer.h>
#include <ISystem.h>
#include <ILog.h>
#include <vector>

#if defined(XENON) || defined(PS3)
#include <Endian.h>
#endif


unsigned int GTextureXRender::ms_textureMemoryUsed(0);


GTextureXRender::GTextureXRender(GRendererXRender* pRendererXRender)
: m_texID(-1)
, m_fmt(eTF_Unknown)
, m_userData(0)
, m_pRendererXRender(pRendererXRender)
{
	assert(m_pRendererXRender);
}


GTextureXRender::~GTextureXRender()
{
	if (m_texID > 0)
	{
		IRenderer* pRenderer(m_pRendererXRender->GetXRender());
		ITexture* pTexture(pRenderer->EF_GetTextureByID(m_texID));
		assert(pTexture);
		if (!pTexture->IsShared())
			ms_textureMemoryUsed -= pTexture->GetDeviceDataSize();
		pRenderer->RT_FlashRemoveTexture(pTexture);
	}
}


bool GTextureXRender::InitTexture(GImageBase* pIm, UInt /*usage*/)
{
	assert(m_texID == -1);
	if (InitTextureInternal(pIm))
	{
		IRenderer* pRenderer(m_pRendererXRender->GetXRender());
		ITexture* pTexture(pRenderer->EF_GetTextureByID(m_texID));
		assert(pTexture && !pTexture->IsShared());
		ms_textureMemoryUsed += pTexture->GetDeviceDataSize();
		m_fmt = pTexture->GetTextureSrcFormat();
	}
	return m_texID > 0;
}


bool GTextureXRender::InitTextureFromFile(const char* pFilename)
{
	assert(m_texID == -1);
	IRenderer* pRenderer(m_pRendererXRender->GetXRender());
	ITexture* pTexture(pRenderer->EF_LoadTexture(pFilename, FT_DONT_STREAM | FT_DONT_RESIZE | FT_NOMIPS, eTT_2D));
	if (pTexture)
	{
		m_texID = pTexture->GetTextureID();
		if (!pTexture->IsShared())
			ms_textureMemoryUsed += pTexture->GetDeviceDataSize();
		m_fmt = pTexture->GetTextureSrcFormat();
	}
	return m_texID > 0;
}


static inline ETEX_Format MapImageType(const GImageBase::ImageFormat& fmt)
{
	switch (fmt)
	{
	case GImageBase::Image_ARGB_8888:
		return eTF_A8R8G8B8;
	case GImageBase::Image_RGB_888:
		return eTF_R8G8B8;
	case GImageBase::Image_L_8:
		return eTF_L8;
	case GImageBase::Image_A_8:
		return eTF_A8;
	case GImageBase::Image_DXT1:
		return eTF_DXT1;
	case GImageBase::Image_DXT3:
		return eTF_DXT3;
	case GImageBase::Image_DXT5:
		return eTF_DXT5;
	default:
		return eTF_Unknown;
	}
}


bool GTextureXRender::InitTextureInternal(const GImageBase* pIm)
{
	ETEX_Format texFmt(MapImageType(pIm->Format));
	if (texFmt != eTF_Unknown)
	{
		return InitTextureInternal(texFmt, pIm->Width, pIm->Height, pIm->Pitch, pIm->pData);
	}
	else
	{
		gEnv->pLog->LogWarning("<Flash> GTextureXRender::InitTextureInternal( ... ) -- Attempted to load texture with unsupported texture format!\n");
		return false;
	}
}


bool GTextureXRender::InitTextureInternal(ETEX_Format texFmt, int32 width, int32 height, int32 pitch, uint8* pData)
{
	IRenderer* pRenderer(m_pRendererXRender->GetXRender());

	bool rgba((pRenderer->GetFeatures() & RFT_RGBA) != 0 || pRenderer->GetRenderType() == eRT_DX11);
	bool swapRB(!rgba && texFmt == eTF_A8R8G8B8);
	ETEX_Format texFmtOrig(texFmt);

	// expand RGB to RGBA if necessary
	std::vector<uint8> expandRGB8;
	if (texFmt == eTF_R8G8B8)
	{
		expandRGB8.reserve(width * height * 4);
		for (int32 y(0); y<height; ++y)
		{
			uint8* pCol(&pData[pitch * y]);
			if (rgba)
			{
				for (int32 x(0); x<width; ++x, pCol += 3)
				{
					expandRGB8.push_back(pCol[0]);
					expandRGB8.push_back(pCol[1]);
					expandRGB8.push_back(pCol[2]);
					expandRGB8.push_back(255);
				}
			}
			else
			{
				for (int32 x(0); x<width; ++x, pCol += 3)
				{
					expandRGB8.push_back(pCol[2]);
					expandRGB8.push_back(pCol[1]);
					expandRGB8.push_back(pCol[0]);
					expandRGB8.push_back(255);
				}
			}
		}
		
		pData = &expandRGB8[0];
		pitch = width * 4;
		texFmt = eTF_X8R8G8B8;
		swapRB = false;
	}

	if (swapRB)
		SwapRB(pData, width, height, pitch);

#if defined(XENON) || defined(PS3)
	if (texFmt == eTF_A8R8G8B8 || texFmt == eTF_X8R8G8B8)
		SwapEndian(pData, width, height, pitch);
#endif

	// Create texture...
	// Note that mip-maps should be generated for font textures (A8/L8) as 
	// Scaleform generates font textures only once and relies on mip-mapping
	// to implement various font sizes.
	m_texID = pRenderer->SF_CreateTexture(width, height, (texFmt == eTF_A8 || texFmt == eTF_L8) ? 0 : 1, pData, texFmt);

	if (m_texID <= 0)
	{
		static const char* s_fomats[] =
		{
			"Unknown",
			"A8R8G8B8",
			"R8G8B8->X8R8G8B8",
			"L8",
			"A8",
			"DXT1",
			"DXT3",
			"DXT5",
			0
		};

		int fmtIdx(0);
		switch (texFmtOrig)
		{
		case eTF_A8R8G8B8:
				fmtIdx = 1;
				break;
		case eTF_X8R8G8B8:
				fmtIdx = 2;
				break;
		case eTF_L8:
				fmtIdx = 3;
				break;
		case eTF_A8:
				fmtIdx = 4;
				break;
		case eTF_DXT1:
				fmtIdx = 5;
				break;
		case eTF_DXT3:
				fmtIdx = 6;
				break;
		case eTF_DXT5:
				fmtIdx = 7;
				break;
		}

		gEnv->pLog->LogWarning("<Flash> GTextureXRender::InitTextureInternal( ... ) "
			"-- Texture creation failed (%dx%d, %s)\n", width, height, s_fomats[fmtIdx]);
	}

#if defined(XENON) || defined(PS3)
	if (texFmt == eTF_A8R8G8B8 || texFmt == eTF_X8R8G8B8)
		SwapEndian(pData, width, height, pitch);
#endif

	if (swapRB)
		SwapRB(pData, width, height, pitch);

	return m_texID > 0;
}


bool GTextureXRender::InitDynamicTexture(int width, int height, GImage::ImageFormat format, int mipmaps, UInt usage)
{
	if (Usage_Update == usage)
	{
		IRenderer* pRenderer(m_pRendererXRender->GetXRender());
		assert(m_texID == -1);
		m_texID = pRenderer->SF_CreateTexture(width, height, mipmaps + 1, 0, MapImageType(format));
		if (m_texID > 0)
		{
			ITexture* pTexture(pRenderer->EF_GetTextureByID(m_texID));
			assert(pTexture && !pTexture->IsShared());
			ms_textureMemoryUsed += pTexture->GetDeviceDataSize();
			m_fmt = pTexture->GetTextureSrcFormat();
		}
	}
	else
	{
		assert(0 && "GTextureXRender::InitDynamicTexture() -- Unsupported usage flag!");
	}
	return m_texID > 0;
}


void GTextureXRender::Update(int level, int n, const UpdateRect* pRects, const GImageBase* pIm)
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);

	assert(m_texID > 0);
	if (!pRects || !n || !pIm || m_texID <= 0)
		return;
	
	IRenderer::SUpdateRect* pSrcRects((IRenderer::SUpdateRect*) alloca(n * sizeof(IRenderer::SUpdateRect)));
	if (pSrcRects)
	{
		for (int i(0); i<n; ++i)
			pSrcRects[i].Set(pRects[i].dest.x, pRects[i].dest.y, pRects[i].src.Left, pRects[i].src.Top, pRects[i].src.Width(), pRects[i].src.Height());

		IRenderer* pRenderer(m_pRendererXRender->GetXRender());
		pRenderer->SF_UpdateTexture(m_texID, level, n, pSrcRects, pIm->pData, pIm->Pitch, MapImageType(pIm->Format));
	}
}


int GTextureXRender::Map(int level, int n, MapRect* maps, int flags)
{
	assert(0 && "GTextureXRender::Map() -- Not implemented!");
	return -1;
}


bool GTextureXRender::Unmap(int level, int n, MapRect* maps, int flags)
{
	assert(0 && "GTextureXRender::Unmap() -- Not implemented!");
	return false;
}


GRenderer* GTextureXRender::GetRenderer() const
{
	return m_pRendererXRender;
}


bool GTextureXRender::IsDataValid() const
{
	return m_texID > 0;
}


GTexture::Handle GTextureXRender::GetUserData() const
{
	return m_userData;
}


void GTextureXRender::SetUserData(Handle hData)
{
	m_userData = hData;
}


void GTextureXRender::AddChangeHandler(ChangeHandler* pHandler)
{
}


void GTextureXRender::RemoveChangeHandler(ChangeHandler* pHandler)
{
}


int32 GTextureXRender::GetID() const
{
	return m_texID;
}


ETEX_Format GTextureXRender::GetFmt() const
{
	return m_fmt;
}


uint32 GTextureXRender::GetTextureMemoryUsed()
{
	return ms_textureMemoryUsed;
}


void GTextureXRender::SwapRB(uint8* pImageData, uint32 width, uint32 height, uint32 pitch) const
{
	for (uint32 y(0); y<height; ++y)
	{
		uint8* pCol(&pImageData[pitch * y]);
		for (uint32 x(0); x<width; ++x, pCol += 4)
		{
			uint8 s(pCol[0]);
			pCol[0] = pCol[2];
			pCol[2] = s;
		}
	}
}


#if defined(XENON) || defined(PS3)
void GTextureXRender::SwapEndian(uint8* pImageData, uint32 width, uint32 height, uint32 pitch) const
{
	for (uint32 y = 0; y < height; y++)
	{
		uint32* pCol((uint32*) &pImageData[pitch * y]);
		::SwapEndian(pCol, width);
	}
}
#endif

#endif // #ifndef EXCLUDE_SCALEFORM_SDK