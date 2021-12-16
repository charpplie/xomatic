#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetBrowserCommon.h
//  Version:	v1.00
//  Created:	21/04/2010 by Nicusor Nedelcu
//  Description:	Common code interface implementation for the asset database
//
// -------------------------------------------------------------------------
//  History:
//		12/03/2010	12:48	:	Nicusor Nedelcu - refactored
//		08/07/2010	17:53	:	Nicusor Nedelcu - cleaned and commented, added consts
//
////////////////////////////////////////////////////////////////////////////
#include "Include/IAssetItemDatabase.h"
#include "Include/IAssetItem.h"
#include "Include/IAssetViewer.h"
#include "AssetBrowserManager.h"
#include "Util/GdiUtil.h"

namespace AssetViewer
{
const int kMinWidthForOverlayText = 256;
const int kOverlayTextTopMargin = 10;
const int kOverlayTextLeftMargin = 10;
};

namespace AssetBrowser
{
extern const char* kThumbnailsRoot;
}

// Description:
//		The base implementation for the IAssetItemDatabase interface
class CAssetItemDatabase : public IAssetItemDatabase
{
public:
	CAssetItemDatabase();
	virtual ~CAssetItemDatabase();
	virtual void PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db);
	virtual void FreeData();
	virtual void Refresh();
	virtual const char* GetDatabaseName() const;
	virtual const char* GetSupportedExtensions() const;
	virtual TAssetFields& GetAssetFields();
	virtual SAssetField* GetAssetFieldByName(const char* pFieldName);
	virtual TFilenameAssetMap& GetAssets();
	virtual	IAssetItem* GetAsset(const char* pAssetFilename);
	virtual void ApplyFilters(const TAssetFieldFiltersMap& rFieldFilters);
	virtual void ApplyTagFilters(const TAssetFieldFiltersMap& rFieldFilters, CAssetBrowserManager::StrVector& assetList);
	virtual void ClearFilters();
	virtual CDialog* CreateDbFilterDialog(CWnd* pParent, IAssetViewer* pViewerCtrl);
	virtual void UpdateDbFilterDialogUI(CDialog* pDlg);
	virtual void OnAssetBrowserOpen(){}
	virtual void OnAssetBrowserClose(){}
	virtual const char* GetTransactionFilename() const;
	virtual bool AddMetaDataChangeListener(MetaDataChangeListener callBack);
	virtual bool RemoveMetaDataChangeListener(MetaDataChangeListener callBack);
	virtual void OnMetaDataChange(const IAssetItem* pAssetItem);
	virtual HRESULT STDMETHODCALLTYPE	QueryInterface(const IID& riid, void** ppvObj);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

protected:
	ULONG m_ref;
	TFilenameAssetMap m_assets;
	TAssetFields m_assetFields;
	std::vector<MetaDataChangeListener> m_metaDataChangeListeners;
};

// Description:
//		Base implementation of the IAssetItem interface
class CAssetItem : public IAssetItem
{
public:
	CAssetItem();
	virtual ~CAssetItem();
	virtual uint32 GetHash() const;
	virtual void SetHash(uint32 hash);
	virtual IAssetItemDatabase* GetOwnerDatabase() const;
	virtual void SetOwnerDatabase(IAssetItemDatabase* piOwnerDatabase);
	virtual const TAssetDependenciesMap& GetDependencies() const;
	virtual void SetFileSize(unsigned __int64 aSize);
	virtual unsigned __int64 GetFileSize() const;
	virtual void SetFilename(const char* pName);
	virtual const char* GetFilename() const;
	virtual void SetRelativePath(const char* pPath);
	virtual const char* GetRelativePath() const;
	virtual void SetFileExtension(const char* pExt);
	virtual const char* GetFileExtension() const;
	virtual UINT GetFlags() const;
	virtual void SetFlags(UINT aFlags);
	virtual void SetFlag(EAssetFlags aFlag, bool bSet = true);
	virtual bool IsFlagSet(EAssetFlags aFlag) const;
	virtual void SetIndex(UINT aIndex);
	virtual UINT GetIndex() const;
	virtual bool GetAssetFieldValue(const char* pFieldName, void* pDest);
	virtual bool SetAssetFieldValue(const char* pFieldName, void* pSrc);
	virtual void GetDrawingRectangle(CRect& rstDrawingRectangle) const;
	virtual void SetDrawingRectangle(const CRect& crstDrawingRectangle);
	virtual bool HitTest(int nX, int nY) const;
	virtual bool HitTest(const CRect& roTestRect) const;
	virtual void* CreateInstanceInViewport(float aX, float aY, float aZ);
	virtual bool MoveInstanceInViewport(const void* pDraggedObject, float aNewX, float aNewY, float aNewZ);
	virtual void AbortCreateInstanceInViewport(const void* pDraggedObject);
	virtual bool Cache();
	virtual bool ForceCache();
	virtual bool LoadThumbnail();
	virtual void UnloadThumbnail();
	virtual void FreeData();
	virtual void OnBeginPreview(const HWND hQuickPreviewWnd, const HDC hMemDC);
	virtual void OnEndPreview();
	virtual CDialog* GetCustomPreviewPanelHeader(CWnd* pParentWnd);
	virtual CDialog* GetCustomPreviewPanelFooter(CWnd* pParentWnd);
	virtual void PreviewRender(
		const HWND hRenderWindow,
		const CRect& rstViewport,
		int aMouseX, int aMouseY,
		int aMouseDeltaX, int aMouseDeltaY,
		int aMouseWheelDelta, UINT aKeyFlags);
	virtual void OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags);
	virtual void OnThumbClick(const CPoint& point, UINT aKeyFlags);
	virtual void OnThumbDblClick(const CPoint& point, UINT aKeyFlags);
	virtual bool DrawThumbImage(const HDC hDC, const CRect& rRect);
	virtual bool SaveReportImage(const char* filePath) const;
	virtual bool SaveReportText(const char* filePath) const;
	virtual void ToXML(XmlNodeRef& node) const;
	virtual void FromXML(const XmlNodeRef& node);
	virtual HRESULT STDMETHODCALLTYPE	QueryInterface(const IID& riid, void** ppvObj);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

protected:
	virtual void DrawTextOnReportImage(CAlphaBitmap& abm) const;

	ULONG m_ref;
	uint64 m_hash;
	CString m_strFilename;
	CString m_strDccFilename;
	CString m_strExtension;
	CString m_strRelativePath;
	unsigned __int64 m_nFileSize;
	volatile UINT m_flags;
	CRect m_oDrawingRectangle;
	CAlphaBitmap m_cachedThumbBmp;
	CAlphaBitmap* m_pUncachedThumbBmp;
	IAssetItemDatabase* m_pOwnerDatabase;
	HDC m_hPreviewDC;
	UINT m_assetIndex;
	TAssetDependenciesMap m_dependencies;
	CString m_errorText, m_toolTipText, m_toolTipOneLineText;
};