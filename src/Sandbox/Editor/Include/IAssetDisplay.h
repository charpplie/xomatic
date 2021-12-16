////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   IAssetDisplay.h
//  Version:     v1.00
//  Created:     15/4/2009 by Paulo Zaffari.
//  Compilers:   Visual Studio.NET 2005
//  Description: Standard interface for asset display in the asset browser,
//               this headed should be used to create plugins.
//							
// The method Release of this interface should NOT be called.
// Instead, the FreeData from the database (from IAssetDisplayDatabase) should
// be used as it will safely release all the items from the database.
// It is still possible to call the release method, but this is not the 
// recomended method, specially for usage outside of the plugins because there
// is no guarantee that a the asset will be properly removed from the database
// manager.
// -------------------------------------------------------------------------
//  History:
//	15/04/2009 - Created by Paulo Zaffari
//  12/03/2010 - Nicusor Nedelcu - refactored
////////////////////////////////////////////////////////////////////////////
#ifndef __IAssetDisplay_H__
#define __IAssetDisplay_H__
#pragma once

class CRect;
struct IAssetDisplayDatabase;

//! This interface allows the programmer to extend asset display types visible in the asset browser.
struct __declspec( uuid("{04F20346-2EC3-43f2-BDA1-2C0B9776F384}") ) IAssetDisplay : public IUnknown
{
	enum EAssetFlags
	{
		//! asset is visible in the database for filtering and sorting (not asset view control related)
		eAssetFlags_Visible = BIT(0),
		//! the asset is loaded
		eAssetFlags_Cached = BIT(1),
		//! the asset is loaded
		eAssetFlags_ThumbnailCached = BIT(2),
		//! the asset field data/info is loaded in
		eAssetFlags_CachedFieldsInfo = BIT(3),
		//! the asset is selected in a selection set
		eAssetFlags_Selected = BIT(4),
		//! this is set if the asset supports calling Cache() from a separate thread from main
		eAssetFlags_ThreadCachingSupported = BIT(5),
		//! this is set if the asset supports loading its fields info from a thread, using CacheFieldsInfo()
		eAssetFlags_ThreadFieldsInfoCachingSupported = BIT(6),
		//! this is set if the asset supports interactive rendering in a thumb small window
		eAssetFlags_InteractiveRenderSupported = BIT(7),
		//! this is set if the asset supports alpha transparency
		eAssetFlags_AlphaSupported = BIT(8),
		//! this asset is invalid, no thumb is shown/available
		eAssetFlags_Invalid = BIT(9),
		//! this asset has some errors/warnings, in the asset browser it will show some blinking/red elements
		//! and the user can check out the errors. Error text will be fetched using GetAssetFieldValue( "errors", &someStringVar )
		eAssetFlags_HasErrors = BIT(10),
	};

	enum EAssetDrawingOption
	{
		eAssetDrawing_RGBA = 0,
		eAssetDrawing_Alpha,
		eAssetDrawing_RGB,
	};

	typedef std::map<string/*field*/,string/*value*/> TAssetFieldValuesMap;

	virtual IAssetDisplayDatabase*	GetOwnerDisplayDatabase() = 0;
	virtual void										SetOwnerDisplayDatabase( IAssetDisplayDatabase* piOwnerDisplayDatabase ) = 0;
	//! \return the vector with filenames which this asset is dependent upon
	virtual const std::vector<string>& GetDependencies() = 0;
	virtual void										SetFileSize( unsigned __int64 aSize ) = 0;
	virtual unsigned __int64				GetFileSize() = 0;
	//! set asset filename (extension included also)
	virtual void										SetFilename( const char* pName ) = 0;
	//! get asset filename (extension included also)
	virtual const char*							GetFilename() = 0;
	virtual void										SetRelativePath( const char* pName ) = 0;
	virtual const char*							GetRelativePath() = 0;
	virtual void										SetFileExtension( const char* pExt ) = 0;
	virtual const char*							GetFileExtension() = 0;
	virtual void										SetUsedInLevel( bool bUsedInLevel ) = 0;
	virtual bool										IsUsedInLevel() const = 0;
	//! \return asset flags from EAssetFlags
	virtual UINT										GetFlags() const = 0;
	//! set asset flags from EAssetFlags
	virtual void										SetFlags( UINT aFlags ) = 0;
	//! set an asset flag from EAssetFlags
	virtual void										SetFlag( EAssetFlags aFlag, bool bSet = true ) = 0;
	//! check an asset flag if set, from EAssetFlags
	virtual bool										IsFlagSet( EAssetFlags aFlag ) = 0;
	//! set this assets' index, used in sorting, selections, and to know where an assets is in the current array
	virtual void										SetIndex( UINT aIndex ) = 0;
	virtual UINT										GetIndex() = 0;
	//! get the asset's field raw data value into an user location, you must check the field's type (from item's database) before using this function
	//! and send the correct pointer to destination according to the type (int,float,string)
	//! \return true if field was found and data retrieved successfuly
	virtual bool										GetAssetFieldValue( const char* pFieldName, void* pDest ) = 0;
	virtual void										GetDrawingRectangle( CRect& rstDrawingRectangle ) const = 0;
	virtual void										SetDrawingRectangle( const CRect& crstDrawingRectangle ) = 0;
	virtual bool										HitTest( int nX, int nY ) = 0;
	virtual bool										HitTest( const CRect& roTestRect ) = 0;
	//! this is used to cache/load asset's data
	virtual bool										Cache() = 0;
	//! this is used to cache/load asset's field information (for example load image, retrieve info, unload image, keep info about it), called in a background info caching thread
	virtual bool										CacheFieldsInfo() = 0;
	virtual bool										UnCache() = 0;
	virtual bool										UnCacheThumbnail() = 0;
	//! used when dragging/rotate/zoom a model, or other asset that can support interactive rendering inside the thumbnail's small window
	virtual void										InteractiveRender( HWND hRenderWindow, const CRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, UINT aKeyFlags ) = 0;
	//! render the asset inside a window in the given viewport, also it can take a snapshot/thumbnail of the rendered image, of requested by bCacheThumbnail
	virtual bool										Render( HWND hRenderWindow, const CRect& rstViewport, bool bCacheThumbnail = false ) = 0;
	//! draw the cached thumb bitmap only, if any, no rendering
	virtual bool										DrawThumbImage( HDC hDC, const CRect& rRect ) = 0;
	virtual EAssetDrawingOption			GetDrawingOption() const { return eAssetDrawing_RGBA; }
	virtual void										SetDrawingOption( EAssetDrawingOption option ) {}
	// Description:
	//     This saves an image of the asset as an image file.
	// Arguments:
	//     filePath - Target path of the output file
	// Return Value:
	//     True if successful, false otherwise
	// See Also:
	//     SaveReportText()
	virtual bool										SaveReportImage( const char *filePath ) const = 0;
	// Description:
	//     This saves a text description of the asset as a text file.
	// Arguments:
	//     filePath - Target path of the output file
	// Return Value:
	//     True if successful, false otherwise
	// See Also:
	//     SaveReportImage()
	virtual bool										SaveReportText( const char *filePath ) const = 0;

	// from IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,void** ppvObject ) { return E_NOINTERFACE; };
	virtual ULONG		STDMETHODCALLTYPE AddRef()	{ return 0; };
	virtual ULONG		STDMETHODCALLTYPE Release() { return 0; };
};

#endif // __IAssetDisplay_H__