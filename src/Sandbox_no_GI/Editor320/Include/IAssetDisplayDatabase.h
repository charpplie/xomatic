////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   IAssetDisplayDatabase.h
//  Version:     v1.00
//  Created:     15/4/2009 by Paulo Zaffari.
//  Compilers:   Visual Studio.NET 2005
//  Description:	Standard interface for asset database creators used to
//					create an asset plugin for the asset browser.
//					The category of the plugin must be "Asset Display".
// -------------------------------------------------------------------------
//  History: 15/4/2009 - Created by Paulo Zaffari
//			11/03/2010 - Nicusor Nedelcu - refactoring
//
////////////////////////////////////////////////////////////////////////////
#ifndef __IAssetDisplayDatabase_H__
#define __IAssetDisplayDatabase_H__
#pragma once

struct IAssetDisplay;
struct IAssetViewer;

//! This struct keeps the info, filter and sorting settings for an asset field
struct SAssetField
{
	//! the condition for the current filter on the field
	enum EAssetFilterCondition
	{
		eAssertFilterCondition_Any = 0,
		
		//! begin - used for strings
		eAssertFilterCondition_Contains,
		eAssertFilterCondition_Match,
		eAssertFilterCondition_StartsWith,
		eAssertFilterCondition_EndsWith,
		//! end - used for strings
		
		eAssertFilterCondition_Equal,
		eAssertFilterCondition_Greater,
		eAssertFilterCondition_Less,
		eAssertFilterCondition_GreaterOrEqual,
		eAssertFilterCondition_LessOrEqual,
		eAssertFilterCondition_Not,
		eAssertFilterCondition_InsideRange
	};

	//! the field type
	enum EAssetFieldType
	{
		eAssetFieldType_None = 0,
		eAssetFieldType_Int8,
		eAssetFieldType_Int16,
		eAssetFieldType_Int32,
		eAssetFieldType_Int64,
		eAssetFieldType_Float,
		eAssetFieldType_Double,
		eAssetFieldType_String
	};

	//! used when a field can have different specific values
	typedef std::vector<std::pair<string/*display name*/,string/*enum value*/>> TFieldEnumValues;

	SAssetField( const char* pFieldName = "", const char* pDisplayName = "Unnamed field", EAssetFieldType aFieldType = eAssetFieldType_None,
							 UINT aColumnWidth = 50, bool bVisibleInUI = true, bool bReadOnly = true )
	{
		m_fieldName = pFieldName;
		m_displayName = pDisplayName;
		m_fieldType = aFieldType;
		m_filterCondition = eAssertFilterCondition_Equal;
		m_bUseEnumValues = false;
		m_bReadOnly = bReadOnly;
		m_listColumnWidth = aColumnWidth;
		m_bFieldVisibleInUI = bVisibleInUI;
		m_bEnableFilter = false;
	}

	//! the field's display name, used in UI
	string 									m_displayName,
	//! the field internal name, used in C++ code
													m_fieldName,
	//! the current filter value, if its empty "" then no filter is applied
													m_filterValue,
	//! the field's max value, valid when the field's filter condition is eAssertFilterCondition_InsideRange
													m_maxFilterValue;
	//! is this field visible in the UI ?
	bool										m_bFieldVisibleInUI,
	//! if true, then you cannot modify this field of an asset item, only use it
													m_bReadOnly,
	//! if true, this field has its filter enabled
													m_bEnableFilter;
	//! the field data type
	EAssetFieldType					m_fieldType;
	//! the filter's condition
	EAssetFilterCondition 	m_filterCondition;
	//! use the enum list values to choose a value for the field ?
	bool 										m_bUseEnumValues;
	//! this map is used when asset field has m_bUseEnumValues on true,
	//! choose a value for the field from this list in the UI
	TFieldEnumValues				m_enumValues;
	//! recommended list column width
	UINT										m_listColumnWidth;
};

//////////////////////////////////////////////////////////////////////////
// Description
//    This interface allows the programmer to extend asset display types
// visible in the asset browser.
//////////////////////////////////////////////////////////////////////////
struct __declspec( uuid("{FB09B039-1D9D-4057-A5F0-AA3C7B97AEA8}") ) IAssetDisplayDatabase : public IUnknown
{
	typedef std::vector<SAssetField>													TAssetFields;
	typedef std::map<string/*field name*/,SAssetField>				TAssetFieldFiltersMap;
	typedef std::map<string/*asset filename*/,IAssetDisplay*>	TFilenameAssetMap;

	//! refresh the database by scanning the folders/paks for files, does not load the files, only filename and filesize are fetched
	//! should return fast from the function
	virtual void 										Refresh() = 0;
	//! if there are some already loaded assets (ex. by level loading), use them for caching fields info of db items
	virtual void										CacheFieldsInfoForAlreadyLoadedAssets() = 0;
	//! \return all assets loaded/scanned by this database
	virtual TFilenameAssetMap&			GetAssets() = 0;
	//! \return the asset fields this database's items support
	virtual TAssetFields&						GetAssetFields() = 0;
	virtual SAssetField*						GetAssetFieldByName( const char* pFieldName ) = 0;
	virtual bool 										SetAssociatedViewer( IAssetViewer* poAssociatedViewer ) = 0;
	virtual IAssetViewer*						GetAssociatedViewer() = 0;
	virtual const char*							GetDatabaseName() = 0;
	virtual const char*							GetDatabaseTypeExt() = 0;
	virtual void 										FreeData() = 0;
	virtual void										ApplyFilters( const TAssetFieldFiltersMap& rFieldFilters ) = 0;
	virtual void										ClearFilters() = 0;

	//! from IUnknown
	virtual HRESULT STDMETHODCALLTYPE	QueryInterface( REFIID riid,void** ppvObject ) { return E_NOINTERFACE; };
	virtual ULONG STDMETHODCALLTYPE		AddRef() { return 0; };
	virtual ULONG STDMETHODCALLTYPE		Release() { return 0; };
};

#endif // IAssetDisplayDatabase_h__