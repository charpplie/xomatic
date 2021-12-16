/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: TextureViewer.h  ,v 1.1 2008/08/28 09:28:02 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This file declares the texture database item. Each item is
part of a texture database. The original database was created for the
texture browser control TextureViewer.
-------------------------------------------------------------------------
History:
- 28:08:2008   09:28 : Created by Paulo Zaffari

*************************************************************************/
#ifndef TextureDatabaseItem_h__
#define TextureDatabaseItem_h__

#pragma once

class CTextureDatabaseItem
{
public:
	CTextureDatabaseItem();
	~CTextureDatabaseItem();

	void		GetFilename(string& rstrFilename) const;
	void		SetFilename(const string& val);

	void		GetExtension(string& rstrExtension) const;
	void		SetExtension(const string& val);

	void 		GetRelativePath(string& rstrRelativePath) const;
	void		SetRelativePath(const string& val);

	void		GetSurfaceType(string& rstrSurfaceTypeString) const;
	void		SetSurfaceType(const string& val);

	bool		GetUsedInLevel() const;
	void		SetUsedInLevel(bool val);

	bool		GetHasAlphaChannelFlag() const;
	void		SetHasAlphaChannelFlag(bool val);

	bool    GetIsCubemap() const;
	void    SetIsCubeMap(bool boIsCubemap);

	int			GetTextureWidth() const;
	void		SetTextureWidth(int val);

	int			GetTextureHeight() const;
	void		SetTextureHeight(int val);

	int			GetMips() const;
	void		SetMips(int val);

	int			GetFileSize() const;
	void		SetFileSize(int val);

	bool		GetVisibleFlag() const;
	void		SetVisibleFlag(bool val);

	bool		GetSelectedFlag();
	void		SetSelectedFlag(bool boIsSeletec);

	bool		GetHoverFlag();
	void		SetHoverFlag(bool boIsHover);

	CRect&	GetDrawingRectangle();		

	bool     GetCachedFlag();

	CBitmap& GetBitmap();
	bool     CacheBitmap(bool boShowOnlyAlpha);
	bool     UnCacheBitmap();

	bool     HitTest(int nX,int nY);
	bool     HitTest(const CRect& roTestRect);
protected:
	string	m_strFilename;
	string	m_strExtension;
	string	m_strRelativePath;
	string  m_strSurfaceTypeString;
	bool    m_boUsedInLevel;
	bool    m_boHasAlphaChannel;
	bool    m_boIsCubemap;

	int			m_nTextureWidth;
	int			m_nTextureHeight;			
	int			m_nMips;
	int			m_nFileSize;

	bool    m_boVisible;
	CRect		m_oDrawingRectangle;

	CBitmap	m_oCachedBitmap;

	bool    m_boIsSelected;
	bool	m_boIsHover;
private:
	bool		m_boBitmapIsChached;

	void    GenerateAlphaMap(void *pBitmapBits,int nWidth,int nHeight);
};

#endif // TextureDatabaseItem_h__
