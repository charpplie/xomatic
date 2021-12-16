#pragma once
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
#include "Asset Browser/AssetBrowserCommon.h"

class CAssetSoundItem : public CAssetItem
{
public:
	CAssetSoundItem();
	~CAssetSoundItem();

	bool GetAssetFieldValue(const char* pFieldName, void* pDest);
	void LoadSound();
	bool Cache();
	void OnBeginPreview(const HWND hQuickPreviewWnd, const HDC hMemDC);
	void OnEndPreview();
	void PreviewRender(const HWND hRenderWindow, const CRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, UINT aKeyFlags);
	void* CreateInstanceInViewport(float aX, float aY, float aZ);
	bool MoveInstanceInViewport(const void* pDraggedObject, float aX, float aY, float aZ);
	void AbortCreateInstanceInViewport(const void* pDraggedObject);
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

	UINT m_nSoundLengthMsec;
	UINT m_timeStamp;
	bool m_bLoopingSound;
	//tSoundID m_soundId;
	static CAlphaBitmap s_uncachedSoundThumbBmp;
};