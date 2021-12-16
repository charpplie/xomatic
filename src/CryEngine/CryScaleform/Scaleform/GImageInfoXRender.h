/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A wrapper around Scaleform's GImageInfo interface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _GIMAGEINFO_XRENDER_H_
#define _GIMAGEINFO_XRENDER_H_

#pragma once


#ifndef EXCLUDE_SCALEFORM_SDK


#pragma warning(push)
#pragma warning(disable : 6326) // Potential comparison of a constant with another constant
#pragma warning(disable : 6011) // Dereferencing NULL pointer
#include <GImageInfo.h>
#pragma warning(pop)

class GTextureXRender;
struct IFlashLoadMovieImage;


class GImageInfoXRender : public GImageInfoBase
{
	// GImageInfoBase interface
public:
	virtual UInt GetWidth() const;
	virtual UInt GetHeight() const;
	virtual UPInt GetBytes() const;
	virtual UPInt GetExternalBytes() const;
	virtual GTexture* GetTexture(GRenderer* pRenderer);

public:
	GImageInfoXRender(GImage* pImage);
	virtual ~GImageInfoXRender();

private:
	uint32 m_width;
	uint32 m_height;
	GPtr<GImage> m_pImg;
	GPtr<GTexture> m_pTex;
};


class GImageInfoFileXRender : public GImageInfoBase
{
	// GImageInfoBase interface
public:
	virtual UInt GetWidth() const;
	virtual UInt GetHeight() const;
	virtual UPInt GetBytes() const;
	virtual UPInt GetExternalBytes() const;
	virtual GTexture* GetTexture(GRenderer* pRenderer);

public:
	GImageInfoFileXRender(const char* pImgFilePath, uint32 targetWidth, uint32 targetHeight);
	virtual ~GImageInfoFileXRender();

private:
	uint32 m_targetWidth;
	uint32 m_targetHeight;
	string m_imgFilePath;
	GPtr<GTexture> m_pTex;
};


class GImageInfoILMISrcXRender : public GImageInfoBase
{
	// GImageInfoBase interface
public:
	virtual UInt GetWidth() const;
	virtual UInt GetHeight() const;
	virtual UPInt GetBytes() const;
	virtual UPInt GetExternalBytes() const;
	virtual GTexture* GetTexture(GRenderer* pRenderer);

public:
	GImageInfoILMISrcXRender(IFlashLoadMovieImage* pSrc);
	virtual ~GImageInfoILMISrcXRender();

private:
	uint32 m_width;
	uint32 m_height;
	IFlashLoadMovieImage* m_pSrc;
	GPtr<GTexture> m_pTex;
};


#endif // #ifndef EXCLUDE_SCALEFORM_SDK


#endif // #ifndef _GIMAGEINFO_XRENDER_H_