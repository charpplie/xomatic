#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetMaterialDatabase.h
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	The Material asset database class, which holds and searches
//								for Material asset in the game data folder
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "Asset Browser/AssetBrowserCommon.h"

class CAssetMaterialDatabase : public CAssetItemDatabase, public IClassDesc
{
public:
	CAssetMaterialDatabase();
	~CAssetMaterialDatabase();
	void PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db);
	void FreeData();
	void Refresh();
	const char* GetDatabaseName() const;
	const char* GetSupportedExtensions() const;
	const char* GetTransactionFilename() const;
	CDialog* CreateDbFilterDialog(CWnd* pParent, IAssetViewer* pViewerCtrl);
	void UpdateDbFilterDialogUI(CDialog* pDlg);

	//////////////////////////////////////////////////////////////////////////
	// From IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID()
	{
		return ESYSTEM_CLASS_ASSET_DISPLAY;
	};
	REFGUID ClassID()
	{
		static const GUID guid = { 0x509ee025, 0x811b, 0x441b, { 0xb5, 0x6e, 0xd4, 0x94, 0x91, 0x80, 0x96, 0x8a } };
		return guid;
	}

	virtual const char* ClassName()
	{
		return "Asset Item Material";
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

	//////////////////////////////////////////////////////////////////////////
	// From IUnknown - Inherited through IClassDesc.
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE	QueryInterface(const IID& riid, void** ppvObj);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

protected:
};