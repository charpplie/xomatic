/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id: AssetTextureItem.h,v 1.0 2009/04/15 11:00:00 PauloZaffari Exp wwwrun $
	$DateTime$
	Description: Header file for the class implementing IAssetDisplayItem
	interface. It declares the headers of the actual used 
	functions.
	-------------------------------------------------------------------------
	History:
	- 15/04/2009	11:00 : Created by Paulo Zaffari
	- 12/03/2010	17:30 : Nicusor Nedelcu - refactored

*************************************************************************/
#ifndef __AssetTextureItem_H__
#define __AssetTextureItem_H__
#pragma once

class ITexture;

#include "..\Include/IAssetDisplay.h"
#include "..\Util/GdiUtil.h"

class CAssetTextureItem: public IAssetDisplay
{
	public:

		CAssetTextureItem();
		~CAssetTextureItem();

		IAssetDisplayDatabase*	GetOwnerDisplayDatabase();
		void										SetOwnerDisplayDatabase( IAssetDisplayDatabase* piOwnerDisplayDatabase );
		const std::vector<string>& GetDependencies();
		void										SetFileSize( unsigned __int64 aSize );
		unsigned __int64				GetFileSize();
		void										SetFilename( const char* pName );
		const char*							GetFilename();
		void										SetRelativePath( const char* pPath );
		const char*							GetRelativePath();
		void										SetFileExtension( const char* pExt );
		const char*							GetFileExtension();
		void										SetUsedInLevel( bool bUsedInLevel )
		{ m_bUsedInLevel = bUsedInLevel; }
		bool										IsUsedInLevel() const
		{ return m_bUsedInLevel; }
		UINT										GetFlags() const;
		void										SetFlags( UINT aFlags );
		void										SetFlag( EAssetFlags aFlag, bool bSet = true );
		bool										IsFlagSet( EAssetFlags aFlag );
		void										SetIndex( UINT aIndex );
		UINT										GetIndex();
		bool										GetAssetFieldValue( const char* pFieldName, void* pDest );
		void										GetDrawingRectangle( CRect& rstDrawingRectangle ) const;
		void										SetDrawingRectangle( const CRect& crstDrawingRectangle );
		bool										HitTest( int nX, int nY );
		bool										HitTest( const CRect& roTestRect );
		bool										Cache();
		bool										CacheFieldsInfo();
		bool										UnCache();
		bool										UnCacheThumbnail();
		void										FreeData();
		void										InteractiveRender( HWND hRenderWindow, const CRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, UINT aKeyFlags );
		bool										Render( HWND hRenderWindow, const CRect& rstViewport, bool bCacheThumbnail = false );
		bool										DrawThumbImage( HDC hDC, const CRect& rRect );
		EAssetDrawingOption			GetDrawingOption() const
		{ return m_drawingOption; }
		void										SetDrawingOption( EAssetDrawingOption option ) 
		{ m_drawingOption = option; }
		bool										SaveReportImage( const char *filePath ) const;
		bool										SaveReportText( const char *filePath ) const;
		void										CacheFieldsInfoForLoadedTex( const ITexture *pTexture );

		HRESULT STDMETHODCALLTYPE	QueryInterface( const IID &riid, void **ppvObj ); 
		ULONG STDMETHODCALLTYPE		AddRef();
		ULONG STDMETHODCALLTYPE		Release();

	protected:
		void										DrawTextOnReportImage( CAlphaBitmap &abm ) const;

		ULONG										m_ref;
		string									m_strFilename;
		string									m_strExtension;
		string									m_strRelativePath;
		string									m_strSurfaceTypeString;
		bool										m_bUsedInLevel;
		bool										m_bHasAlphaChannel;
		bool										m_bIsCubemap;
		int											m_nTextureWidth;
		int											m_nTextureHeight;
		int											m_nMips;
		unsigned __int64				m_nFileSize;
		ETEX_Format							m_format;
		volatile UINT						m_flags;
		CRect										m_oDrawingRectangle;
		IAssetDisplayDatabase*	m_piOwnerDatabase;
		CAlphaBitmap						m_oCachedBmp, m_oCachedBmpAlpha, m_oCachedBmpRGB;
		std::vector<string>			m_dependencies;
		UINT										m_assetIndex;
		EAssetDrawingOption			m_drawingOption;
		//TODO: move common members and methods to a proxy class, visible only in the editor and not plugins
};

#endif // __AssetTextureItem_H__