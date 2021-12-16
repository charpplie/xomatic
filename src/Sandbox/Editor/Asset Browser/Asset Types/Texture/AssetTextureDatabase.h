/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id: AssetTextureDatabase.h,v 1.0 2009/04/15 11:00:00 PauloZaffari Exp wwwrun $
	$DateTime$
	Description: Header file for the class implementing IAssetDisplay
	interface. It declares the headers of the actual used 
	functions.
	-------------------------------------------------------------------------
	History:
	- 15/04/2009		11:00 - Created by Paulo Zaffari
	- 12/03/2010		17:16 - Nicusor Nedelcu - refactored
*************************************************************************/
#ifndef __AssetTextureDatabse_H__
#define __AssetTextureDatabse_H__
#pragma once

#include "..\Include/IAssetDisplayDatabase.h"
#include "CryThread.h"

// As CryThread is not an interface, we don't care if we are hiding its methods
// in this plugin.
#pragma warning(push)
#pragma warning(disable:4266)

class CAssetTextureDatabase : public IAssetDisplayDatabase, public IClassDesc
{
	public:

		CAssetTextureDatabase();
		~CAssetTextureDatabase();

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
		void								ApplyFilters( const TAssetFieldFiltersMap& rFieldFilters );
		void								ClearFilters();

		//////////////////////////////////////////////////////////////////////////
		// From IClassDesc
		//////////////////////////////////////////////////////////////////////////
		virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_ASSET_DISPLAY; };
		REFGUID ClassID()
		{
			// {509EE025-855B-441b-B56E-D4949180968A}
			static const GUID guid = { 0x509ee025, 0x855b, 0x441b, { 0xb5, 0x6e, 0xd4, 0x94, 0x91, 0x80, 0x96, 0x8a } };
			return guid;
		}
		virtual const char*			ClassName() { return "Asset Display Texture"; };
		virtual const char*			Category() { return "Asset Display"; };
		virtual CRuntimeClass*	GetRuntimeClass(){return 0;};
		virtual void						ShowAbout() {};

		//////////////////////////////////////////////////////////////////////////
		// From IUnknown - Inherited through IClassDesc.
		//////////////////////////////////////////////////////////////////////////
		HRESULT STDMETHODCALLTYPE	QueryInterface( const IID &riid, void **ppvObj );
		ULONG STDMETHODCALLTYPE		AddRef();
		ULONG STDMETHODCALLTYPE		Release();

	protected:

		ULONG									m_ref;
		IAssetViewer*					m_poAssociatedViewer;
		TFilenameAssetMap			m_assets;
		TAssetFields					m_assetFields;
};

#pragma warning(pop)

#endif // __AssetTextureDatabse_H__
