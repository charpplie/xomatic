////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetSoundItem.h
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	Header file for handling and rendering the the sound asset items
//								in the asset browser
// -------------------------------------------------------------------------  
//  History:
//		05/05/2010	20:12 : Nicusor Nedelcu - created
//		10/05/2010	13:58 : Nicusor Nedelcu - moved common code out of the class to AssetBrowserCommon.h/cpp
//
//////////////////////////////////////////////////////////////////////////// 

#ifndef __AssetSoundItem_h__
#define __AssetSoundItem_h__
#pragma once

#include "Asset Browser/AssetBrowserCommon.h"

class CAssetSoundItem : public CAssetDisplay
{
	public:

		CAssetSoundItem();
		~CAssetSoundItem();

		bool											GetAssetFieldValue( const char* pFieldName, void* pDest );
		bool											Cache();
		bool											CacheFieldsInfo();
		bool											UnCache();
		bool											UnCacheThumbnail();
		void											OnBeginPreview( const HWND hQuickPreviewWnd, const HDC hMemDC );
		void											OnEndPreview();
		void											InteractiveRender( const HWND hRenderWindow, const CRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, UINT aKeyFlags );
		void											OnInteractiveRenderKeyEvent( bool bKeyDown, UINT aChar, UINT aKeyFlags );
		bool											Render( const HWND hRenderWindow, const CRect& rstViewport, bool bCacheThumbnail = false );
		bool											DrawThumbImage( const HDC hDC, const CRect& rRect );
		bool											SaveReportImage( const char *filePath ) const;
		bool											SaveReportText( const char *filePath ) const;
		void											CacheFieldsInfoForLoadedTex( const ISound *pSound );

		HRESULT STDMETHODCALLTYPE	QueryInterface( const IID &riid, void **ppvObj ); 
		ULONG STDMETHODCALLTYPE		AddRef();
		ULONG STDMETHODCALLTYPE		Release();

	protected:

		void											DrawTextOnReportImage( CAlphaBitmap &abm ) const;

		UINT											m_nSoundLengthMsec;
		CAlphaBitmap							m_oCachedBmp;
		tSoundID									m_soundId;
		static CAlphaBitmap				m_oSoundBmp;
};

#endif //__AssetSoundItem_h__
