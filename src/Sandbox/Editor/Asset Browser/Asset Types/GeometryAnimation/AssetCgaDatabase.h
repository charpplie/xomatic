#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013
////////////////////////////////////////////////////////////////////////////

#include "Asset Browser/AssetBrowserCommon.h"

// Description:
//		Implements the database which handles CGF model assets
class CAssetCgaDatabase : public CAssetItemDatabase, public IClassDesc
{
public:
	CAssetCgaDatabase();
	~CAssetCgaDatabase();
	void CacheFieldsInfoForAlreadyLoadedAssets();
	void PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db);
	void Refresh();
	const char* GetDatabaseName() const;
	const char* GetSupportedExtensions() const;
	const char* GetTransactionFilename() const;
	CDialog* CreateDbFilterDialog(CWnd* pParent, IAssetViewer* pViewerCtrl);
	void UpdateDbFilterDialogUI(CDialog* pDlg);
	
	// from IClassDesc
	virtual ESystemClassID SystemClassID()
	{
		return ESYSTEM_CLASS_ASSET_DISPLAY;
	};
	REFGUID ClassID()
	{
		static const GUID guid =
		{ 0x8ea6c7bb, 0x381a, 0x4f84, { 0x8e, 0x3f, 0x16, 0x7, 0xa5, 0x37, 0xe6, 0xe3 } };
		return guid;
	}

	virtual const char* ClassName()
	{
		return "Asset Item CGA";
	};
	virtual const char* Category()
	{
		return "Asset Item DB";
	};
	virtual CRuntimeClass* GetRuntimeClass()
	{
		return 0;
	};
	virtual void ShowAbout() {};

	// from IUnknown - Inherited through IClassDesc.
	HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
};