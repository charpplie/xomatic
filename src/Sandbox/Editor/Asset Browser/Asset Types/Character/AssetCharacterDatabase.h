#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetModelDatabase.h
//  Version:	v1.00
//  Created:	15/04/2009 by Paulo Zaffari
//  Description:	Header file for the class implementing IAssetDisplay
//								interface. It declares the headers of the actual used
//								functions.
// -------------------------------------------------------------------------
//  History:
//			15/04/2009	11:00 - Paulo Zaffari - created
//			15/03/2010	19:10 - Nicusor Nedelcu - refactored
//
////////////////////////////////////////////////////////////////////////////
#include "Asset Browser/AssetBrowserCommon.h"

// Description:
//		Implements the database which handles CGF model assets
class CAssetCharacterDatabase : public CAssetItemDatabase, public IClassDesc
{
public:
	CAssetCharacterDatabase();
	~CAssetCharacterDatabase();
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
		{ 0x8ea6c7b1, 0x484a, 0x4f84, { 0x11, 0x3f, 0x16, 0x7, 0xa5, 0x37, 0xe6, 0xe3 } };
		return guid;
	}

	virtual const char* ClassName()
	{
		return "Asset Item Character";
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