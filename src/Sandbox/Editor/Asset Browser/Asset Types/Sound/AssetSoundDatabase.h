#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetSoundDatabase.h
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	The sound asset database class, which holds and searches
//								for sound asset in the game data folder
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "Asset Browser/AssetBrowserCommon.h"

class CAssetSoundDatabase : public CAssetItemDatabase, public IClassDesc
{
public:
	CAssetSoundDatabase();
	~CAssetSoundDatabase();
	void PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db);
	void FreeData();
	void Refresh();
	const char* GetDatabaseName() const;
	const char* GetSupportedExtensions() const;
	const char* GetTransactionFilename() const;
	CDialog* CreateDbFilterDialog(CWnd* pParent, IAssetViewer* pViewerCtrl);
	void UpdateDbFilterDialogUI(CDialog* pDlg);
	void CollectCachedEventgroup(XmlNodeRef& gr, const CString& Block, const CString& Path, int level);

	//////////////////////////////////////////////////////////////////////////
	// From IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID()
	{
		return ESYSTEM_CLASS_ASSET_DISPLAY;
	};
	REFGUID ClassID()
	{
		// {509EE025-855B-441b-B56E-D4949180968A}
		static const GUID guid = { 0x509ee025, 0x855b, 0x441b, { 0xb5, 0x6e, 0xd4, 0x94, 0x91, 0x80, 0x96, 0x8a } };
		return guid;
	}

	virtual const char* ClassName()
	{
		return "Asset Item Sound";
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
		HRESULT STDMETHODCALLTYPE	QueryInterface( const IID &riid, void **ppvObj );
		ULONG STDMETHODCALLTYPE AddRef();
		ULONG STDMETHODCALLTYPE Release(); 
};
