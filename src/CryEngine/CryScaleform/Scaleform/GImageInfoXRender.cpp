#include "StdAfx.h"


#ifndef EXCLUDE_SCALEFORM_SDK


#include <IFlashPlayer.h>
#include "GImageInfoXRender.h"
#include "GTextureXRender.h"


GImageInfoXRender::GImageInfoXRender(GImage* pImage)
: m_width(pImage ? pImage->Width : 0)
, m_height(pImage ? pImage->Height : 0)
, m_pImg(pImage)
, m_pTex(0)
{
}


GImageInfoXRender::~GImageInfoXRender()
{
	m_pImg = 0;
	m_pTex = 0;
}


UInt GImageInfoXRender::GetWidth() const
{
	return m_width;
}


UInt GImageInfoXRender::GetHeight() const
{
	return m_height;
}


UPInt GImageInfoXRender::GetBytes() const
{
	return 0; // proper AMP Server support might come later
}


UPInt GImageInfoXRender::GetExternalBytes() const
{
	return 0; // proper AMP Server support might come later
}


GTexture* GImageInfoXRender::GetTexture(GRenderer* pRenderer)
{
	if (!m_pTex)
	{
		if (m_pImg)
		{
			GPtr<GTexture> pTex = *pRenderer->CreateTexture();
			if (pTex && pTex->InitTexture(m_pImg))
			{
				m_pTex = pTex;
				m_pImg = 0;
			}
		}
	}
	return m_pTex;
}

//////////////////////////////////////////////////////////////////////////

GImageInfoFileXRender::GImageInfoFileXRender(const char* pImgFilePath, uint32 targetWidth, uint32 targetHeight)
: m_targetWidth(targetWidth)
, m_targetHeight(targetHeight)
, m_imgFilePath(pImgFilePath)
, m_pTex(0)
{
}


GImageInfoFileXRender::~GImageInfoFileXRender()
{
	m_pTex = 0;
}


UInt GImageInfoFileXRender::GetWidth() const
{
	return m_targetWidth;
}


UInt GImageInfoFileXRender::GetHeight() const
{
	return m_targetHeight;
}


UPInt GImageInfoFileXRender::GetBytes() const
{
	return 0; // proper AMP Server support might come later
}


UPInt GImageInfoFileXRender::GetExternalBytes() const
{
	return 0; // proper AMP Server support might come later
}


GTexture* GImageInfoFileXRender::GetTexture(GRenderer* pRenderer)
{
	if (!m_pTex)
	{
		if (m_imgFilePath.c_str() && m_imgFilePath.c_str()[0] != '\0')
		{
			GPtr<GTexture> pTex = *pRenderer->CreateTexture();
			if (pTex && ((GTextureXRender*)pTex.GetPtr())->InitTextureFromFile(m_imgFilePath)) // InitTextureFromFile() no longer part of GTexture interface! :( TODO: find better way to avoid cast
				m_pTex = pTex;
		}
	}
	return m_pTex;
}

//////////////////////////////////////////////////////////////////////////

GImageInfoILMISrcXRender::GImageInfoILMISrcXRender(IFlashLoadMovieImage* pSrc)
: m_width(pSrc ? pSrc->GetWidth() : 0)
, m_height(pSrc ? pSrc->GetHeight() : 0)
, m_pSrc(pSrc)
, m_pTex(0)
{
}


GImageInfoILMISrcXRender::~GImageInfoILMISrcXRender()
{
	SAFE_RELEASE(m_pSrc);
	m_pTex = 0;
}


UInt GImageInfoILMISrcXRender::GetWidth() const
{
	return m_width;
}


UInt GImageInfoILMISrcXRender::GetHeight() const
{
	return m_height;
}


UPInt GImageInfoILMISrcXRender::GetBytes() const
{
	return 0; // proper AMP Server support might come later
}


UPInt GImageInfoILMISrcXRender::GetExternalBytes() const
{
	return 0; // proper AMP Server support might come later
}


GTexture* GImageInfoILMISrcXRender::GetTexture(GRenderer* pRenderer)
{
	if (!m_pTex)
	{
		if (m_pSrc)
		{
			IFlashLoadMovieImage::EFmt fmt(m_pSrc->GetFormat());
			if (fmt == IFlashLoadMovieImage::eFmt_ARGB_8888 || fmt == IFlashLoadMovieImage::eFmt_RGB_888)
			{
				GImageBase img(fmt == IFlashLoadMovieImage::eFmt_ARGB_8888 ? GImage::Image_ARGB_8888 : GImage::Image_RGB_888, 
					m_pSrc->GetWidth(), m_pSrc->GetHeight(), m_pSrc->GetPitch());
				img.pData = (UByte*) m_pSrc->GetPtr();

				GPtr<GTexture> pTex = *pRenderer->CreateTexture();
				if (pTex && pTex->InitTexture(&img))
				{
					m_pTex = pTex;
					SAFE_RELEASE(m_pSrc);
				}
			}
		}
	}
	return m_pTex;
}


#endif // #ifndef EXCLUDE_SCALEFORM_SDK