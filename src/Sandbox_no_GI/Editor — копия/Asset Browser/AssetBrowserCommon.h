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

#ifndef __AssetDatabaseCommon_h__
#define __AssetDatabaseCommon_h__
#pragma once

#include "Include/IAssetDisplayDatabase.h"
#include "Include/IAssetDisplay.h"
#include "Include/IAssetViewer.h"
#include "Util/GdiUtil.h"

// Asset items vector
typedef std::vector<IAssetDisplay*> 				TAssetItems;
// Asset database vector
typedef std::vector<IAssetDisplayDatabase*>	TAssetDatabases;

namespace AssetViewer
{
	// Used in GetAssetFieldValue to replace the ugly and long strncmp, to check if the argument is the requested field
	// fieldName is a constant string, used in the 'switch' of IAssetDisplay::GetAssetFieldValue
	// Usage example: if( IS_ASSET_FIELD_NAME( "filename" ) ) .....
	#define IS_ASSET_FIELD_NAME( fieldName ) ( !strncmp( pFieldName, fieldName, strlen( pFieldName ) ) )
	const int kMinWidthForOverlayText = 256;
	const int kOverlayTextTopMargin = 10;
	const int kOverlayTextLeftMargin = 10;
};

// Description:
//		The base implementation for the IAssetDisplayDatabase interface
class CAssetDisplayDatabase : public IAssetDisplayDatabase
{
	public:

		CAssetDisplayDatabase();
		virtual ~CAssetDisplayDatabase();

		virtual void								CacheFieldsInfoForAlreadyLoadedAssets();
		virtual void								PrecacheFieldsInfoFromFileDB( const XmlNodeRef& db );
		virtual void								FreeData();
		virtual void								Refresh();
		virtual bool								SetAssociatedViewer( IAssetViewer* piAssociatedViewer );
		virtual IAssetViewer*				GetAssociatedViewer() const;
		virtual const char*					GetDatabaseName() const;
		virtual const char*					GetDatabaseTypeExt() const;
		virtual TAssetFields&				GetAssetFields();
		virtual SAssetField*				GetAssetFieldByName( const char* pFieldName );
		virtual TFilenameAssetMap&	GetAssets();
		virtual	IAssetDisplay*			GetAsset( const char* pAssetFilename );
		virtual void								ApplyFilters( const TAssetFieldFiltersMap& rFieldFilters );
		virtual void								ClearFilters();
		virtual const char*					GetTransactionFilename() const;
		virtual bool								AddMetaDataChangeListener(MetaDataChangeListener callBack);
		virtual bool								RemoveMetaDataChangeListener(MetaDataChangeListener callBack);
		virtual void								OnMetaDataChange(const IAssetDisplay* pAssetItem);

	protected:

		ULONG												m_ref;
		IAssetViewer*								m_poAssociatedViewer;
		TFilenameAssetMap						m_assets;
		TAssetFields								m_assetFields;
		std::vector<MetaDataChangeListener> m_metaDataChangeListeners;
};

// Description:
//		Base implementation of the IAssetDisplay interface
class CAssetDisplay: public IAssetDisplay
{
	public:

		CAssetDisplay();
		virtual ~CAssetDisplay();

		virtual IAssetDisplayDatabase*	GetOwnerDisplayDatabase() const;
		virtual void										SetOwnerDisplayDatabase( IAssetDisplayDatabase* piOwnerDisplayDatabase );
		virtual const TAssetDependencyCategFilenamesMap& GetDependencies() const;
		virtual void										SetFileSize( unsigned __int64 aSize );
		virtual unsigned __int64				GetFileSize() const;
		virtual void										SetFilename( const char* pName );
		virtual const char*							GetFilename() const;
		virtual void										SetRelativePath( const char* pPath );
		virtual const char*							GetRelativePath() const;
		virtual void										SetFileExtension( const char* pExt );
		virtual const char*							GetFileExtension() const;
		virtual UINT										GetFlags() const;
		virtual void										SetFlags( UINT aFlags );
		virtual void										SetFlag( EAssetFlags aFlag, bool bSet = true );
		virtual bool										IsFlagSet( EAssetFlags aFlag ) const;
		virtual void										SetIndex( UINT aIndex );
		virtual UINT										GetIndex() const;
		virtual bool										GetAssetFieldValue( const char* pFieldName, void* pDest );
		virtual bool										SetAssetFieldValue( const char* pFieldName, void* pSrc );
		virtual void										GetDrawingRectangle( CRect& rstDrawingRectangle ) const;
		virtual void										SetDrawingRectangle( const CRect& crstDrawingRectangle );
		virtual bool										HitTest( int nX, int nY ) const;
		virtual bool										HitTest( const CRect& roTestRect ) const;
		virtual void*										CreateInstanceInViewport( float aX, float aY, float aZ );
		virtual bool										MoveInstanceInViewport( const void* pDraggedObject, float aNewX, float aNewY, float aNewZ );
		virtual void										AbortCreateInstanceInViewport( const void* pDraggedObject );
		virtual bool										Cache();
		virtual bool										CacheFieldsInfo();
		virtual bool										UnCache();
		virtual bool										UnCacheThumbnail();
		virtual void										FreeData();
		virtual void										OnBeginPreview( const HWND hQuickPreviewWnd, const HDC hMemDC );
		virtual void										OnEndPreview();
		virtual CDialog*								GetCustomPreviewPanel( CWnd* pParentWnd );
		virtual void										InteractiveRender( const HWND hRenderWindow, const CRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, int aMouseWheelDelta, UINT aKeyFlags );
		virtual void										OnInteractiveRenderKeyEvent( bool bKeyDown, UINT aChar, UINT aKeyFlags );
		virtual void										OnThumbClick( const CPoint& point, UINT aKeyFlags );
		virtual void										OnThumbDblClick( const CPoint& point, UINT aKeyFlags );
		virtual bool										Render( const HWND hRenderWindow, const CRect& rstViewport, bool bCacheThumbnail = false );
		virtual bool										DrawThumbImage( const HDC hDC, const CRect& rRect );
		virtual bool										SaveReportImage( const char *filePath ) const;
		virtual bool										SaveReportText( const char *filePath ) const;
		virtual void										ToXML(XmlNodeRef& node) const;
		virtual void										FromXML(const XmlNodeRef& node);

	protected:

		virtual void											DrawTextOnReportImage( CAlphaBitmap &abm ) const;

		ULONG															m_ref;
		CString														m_strFilename;
		CString														m_strDccFilename;
		CString														m_strExtension;
		CString														m_strRelativePath;
		unsigned __int64									m_nFileSize;
		volatile UINT											m_flags;
		CRect															m_oDrawingRectangle;
		IAssetDisplayDatabase*						m_piOwnerDatabase;
		HDC																m_hPreviewDC;
		UINT															m_assetIndex;
		TAssetDependencyCategFilenamesMap	m_dependencies;
		CString														m_errorText, m_toolTipText, m_toolTipOneLineText;
};

#endif //__AssetDatabaseCommon_h__
