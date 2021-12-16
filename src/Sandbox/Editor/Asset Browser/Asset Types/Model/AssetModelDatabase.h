/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id: AssetModelDatabase.h,v 1.0 2009/04/15 11:00:00 PauloZaffari Exp wwwrun $
	$DateTime$
	Description: Header file for the class implementing IAssetDisplay
							 interface. It declares the headers of the actual used 
							 functions.
	-------------------------------------------------------------------------
	History:
	- 15/04/2009	11:00 : Created by Paulo Zaffari
	- 15/03/2010	19:10 : Nicusor Nedelcu - refactored
*************************************************************************/
#ifndef __AssetModelDatabase_H__
#define __AssetModelDatabase_H__
#pragma once;

#include "..\Include/IEditorClassFactory.h"
#include "..\Include/IAssetDisplayDatabase.h"
#include "CryThread.h"

// As CryThread is not an interface, we don't care if we are hiding its methods
// in this plugin.
#pragma warning(push)
#pragma warning(disable:4266)

class CAssetModelDatabase : public IAssetDisplayDatabase, public IClassDesc
{
	public:

		CAssetModelDatabase();
		~CAssetModelDatabase();
		
		void								CacheFieldsInfoForAlreadyLoadedAssets();
		void								FreeData();
		void								Refresh();
		bool								SetAssociatedViewer( IAssetViewer* piAssociatedViewer );
		IAssetViewer*				GetAssociatedViewer();
		const char*					GetDatabaseName();
		const char*					GetDatabaseTypeExt();
		TAssetFields&				GetAssetFields();
		SAssetField*				GetAssetFieldByName( const char* pFieldName );
		TFilenameAssetMap&	GetAssets();
		void								ApplyFilters( const IAssetDisplayDatabase::TAssetFieldFiltersMap& rFieldFilters );
		void								ClearFilters();


		// from IClassDesc
		virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_ASSET_DISPLAY; };
		REFGUID ClassID()
		{
			// {8EA6C7BB-484A-4f84-8E3F-1607A537E6E3}
			static const GUID guid = 
			{ 0x8ea6c7bb, 0x484a, 0x4f84, { 0x8e, 0x3f, 0x16, 0x7, 0xa5, 0x37, 0xe6, 0xe3 } };
			return guid;
		}

		virtual const char* ClassName() { return "Asset Display Texture"; };
		virtual const char* Category() { return "Asset Display"; };
		virtual CRuntimeClass* GetRuntimeClass(){return 0;};
		virtual void ShowAbout() {};

		// from IUnknown - Inherited through IClassDesc.
		HRESULT STDMETHODCALLTYPE QueryInterface( const IID &riid, void **ppvObj );
		ULONG STDMETHODCALLTYPE AddRef();
		ULONG STDMETHODCALLTYPE Release();

	protected:

		ULONG															m_ref;
		IAssetViewer*											m_poAssociatedViewer;
		TFilenameAssetMap									m_assets;
		TAssetFields											m_assetFields;
};

#pragma warning(pop)

#endif // AssetModelDatabse_h__
