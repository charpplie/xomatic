/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A wrapper around Scaleform's GTexture interface to build textures via CryEngine's ITexture interface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _GTEXTURE_XRENDER_H_
#define _GTEXTURE_XRENDER_H_

#pragma once


#ifndef EXCLUDE_SCALEFORM_SDK

#include <platform.h>
#include <IRenderer.h>
#include <IShader.h>

#pragma warning(push)
#pragma warning(disable : 6326) // Potential comparison of a constant with another constant
#include <GRenderer.h>
#pragma warning(pop)


class GRendererXRender;
class GImageBase;
class ITexture;


class GTextureXRender : public GTexture
{
	// GTexture interface
public: 
	virtual bool InitTexture(GImageBase* pIm, UInt usage = Usage_Wrap);
	virtual bool InitDynamicTexture(int width, int height, GImage::ImageFormat format, int mipmaps, UInt usage);

	virtual void Update(int level, int n, const UpdateRect* pRects, const GImageBase* pIm);

	virtual int Map(int level, int n, MapRect* maps, int flags = 0);
	virtual bool Unmap(int level, int n, MapRect* maps, int flags = 0);

	virtual GRenderer* GetRenderer() const;
	virtual	bool IsDataValid() const;

	virtual Handle GetUserData() const;
	virtual void SetUserData(Handle hData);

	virtual void AddChangeHandler(ChangeHandler* pHandler);
	virtual void RemoveChangeHandler(ChangeHandler* pHandler);

public:
	GTextureXRender(GRendererXRender* pRendererXRender);
	virtual ~GTextureXRender();

	bool InitTextureFromFile(const char* pFilename);

	int32 GetID() const;
	ETEX_Format GetFmt() const;

	static uint32 GetTextureMemoryUsed();

private:
	bool InitTextureInternal(const GImageBase* pIm);
	bool InitTextureInternal(ETEX_Format texFmt, int32 width, int32 height, int32 pitch, uint8* pData);
	void SwapRB(uint8* pImageData, uint32 width, uint32 height, uint32 pitch) const;
#if defined(XENON) || defined(PS3)
	void SwapEndian(uint8* pImageData, uint32 width, uint32 height, uint32 pitch) const;
#endif

private:
	static uint32 ms_textureMemoryUsed;

private:
	int32 m_texID;
	ETEX_Format m_fmt;
	Handle m_userData;
	GRendererXRender* m_pRendererXRender;
};


#endif // #ifndef EXCLUDE_SCALEFORM_SDK


#endif // #ifndef _GTEXTURE_XRENDER_H_