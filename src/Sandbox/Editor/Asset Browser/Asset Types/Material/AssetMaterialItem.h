#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetMaterialItem.h
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description: Header file for handling and rendering the the Material asset items
//				in the asset browser
// -------------------------------------------------------------------------
//  History:
//		01/09/2012	12:27 : Nicusor Nedelcu - created
//
////////////////////////////////////////////////////////////////////////////
#include "Asset Browser/AssetBrowserCommon.h"

class CPreviewModelCtrl;

class CAssetMaterialItem : public CAssetItem
{
public:
	CAssetMaterialItem();
	~CAssetMaterialItem();

	bool GetAssetFieldValue(const char* pFieldName, void* pDest);
	void LoadMaterial();
	bool Cache();
	void OnBeginPreview(const HWND hQuickPreviewWnd, const HDC hMemDC);
	void OnEndPreview();
	void PreviewRender(const HWND hRenderWindow, const CRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, UINT aKeyFlags);
	bool Render(const HWND hRenderWindow, const CRect& rstViewport, bool bCacheThumbnail);
	void CacheThumbnail(const HWND hRenderWindow, const CRect& rc);
	void OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags);
	bool SaveReportImage(const char* filePath) const;
	bool SaveReportText(const char* filePath) const;
	void ToXML(XmlNodeRef& node) const;
	void FromXML(const XmlNodeRef& node);

	HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

protected:
	void DrawTextOnReportImage(CAlphaBitmap& abm) const;

private:
	static CAlphaBitmap s_uncachedMaterialThumbBmp;
	static CPreviewModelCtrl* s_pPreviewCtrl;
	_smart_ptr<CMaterial> m_pMaterial;
};