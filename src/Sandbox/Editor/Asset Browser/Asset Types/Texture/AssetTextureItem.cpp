/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: AssetTextureItem.h,v 1.0 2009/04/15 11:00:00 PauloZaffari Exp wwwrun $
$DateTime$
Description: Spirce file for the class implementing IAssetDisplayItem
interface. It declares the headers of the actual used 
functions.
-------------------------------------------------------------------------
History:
- 15:04:2009   11:00 : Created by Paulo Zaffari

*************************************************************************/
#include "StdAfx.h"
#include "AssetTextureItem.h"
#include "..\Util/MemoryBlock.h"
#include "..\Util/Image.h"
#include "..\Util/ImageUtil.h"
#include "..\Util/PathUtil.h"
#include "..\Include/IAssetDisplayDatabase.h"
#include "ITexture.h"
#include "IRenderer.h"
#include "..\Include/IAssetViewer.h"
#include "ImageExtensionHelper.h"

//! used in GetAssetFieldValue to replace the ugly strcmp
//! fieldName is constant string, used in the 'switch' of IAssetDisplay::GetAssetFieldValue
#define isAssetField( fieldName ) !strncmp( pFieldName, fieldName, strlen( pFieldName ) )

static const int kAssetViewer_MinWidthForOverlayText = 256;
static const int kAssetViewer_OverlayTextTopMargin = 10;
static const int kAssetViewer_OverlayTextLeftMargin = 10;

CAssetTextureItem::CAssetTextureItem():
m_strFilename(""),
m_strExtension(""),
m_strRelativePath(""),
m_strSurfaceTypeString(""),
m_bUsedInLevel(false),
m_bHasAlphaChannel(false),
m_bIsCubemap(false),
m_nTextureWidth(0),
m_nTextureHeight(0),
m_nMips(0),
m_format(eTF_Unknown),
m_flags( eAssetFlags_ThreadCachingSupported | eAssetFlags_ThreadFieldsInfoCachingSupported | eAssetFlags_AlphaSupported ),
m_nFileSize(0),
m_oDrawingRectangle( 0, 0, m_nTextureWidth, m_nTextureHeight ),
m_piOwnerDatabase( NULL ),
m_ref( 1 ),
m_assetIndex(0),
m_drawingOption(eAssetDrawing_RGBA)
{
}

CAssetTextureItem::~CAssetTextureItem()
{
}

HRESULT STDMETHODCALLTYPE CAssetTextureItem::QueryInterface( const IID &riid, void **ppvObj ) 
{ 
	if( riid == __uuidof(IAssetDisplay) /* && m_pIntegrator*/ )
	{
		*ppvObj = this;
		return S_OK;
	}
	return E_NOINTERFACE ; 
}

ULONG STDMETHODCALLTYPE CAssetTextureItem::AddRef()
{
	return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetTextureItem::Release() 
{ 
	if( (--m_ref) == 0 )
	{
		FreeData();
		delete this;
		return 0; 
	}
	else
		return m_ref;
}

void CAssetTextureItem::FreeData()
{
	UnCache();
	UnCacheThumbnail();
}

IAssetDisplayDatabase* CAssetTextureItem::GetOwnerDisplayDatabase()
{
	return m_piOwnerDatabase;
}

void CAssetTextureItem::SetOwnerDisplayDatabase( IAssetDisplayDatabase* piOwnerDisplayDatabase )
{
	if( !m_piOwnerDatabase )
	{
		m_piOwnerDatabase = piOwnerDisplayDatabase;
	}
}

const std::vector<string>& CAssetTextureItem::GetDependencies()
{
	return m_dependencies;
}

void CAssetTextureItem::SetFileSize( unsigned __int64 aSize )
{
	m_nFileSize = aSize;
}

void CAssetTextureItem::SetFileExtension( const char* pExt )
{
	m_strExtension = pExt;
}

const char* CAssetTextureItem::GetFileExtension()
{
	return m_strExtension.c_str();
}

unsigned __int64 CAssetTextureItem::GetFileSize()
{
	return m_nFileSize;
}

void CAssetTextureItem::SetFilename( const char* pName )
{
	m_strFilename = pName;
}

const char* CAssetTextureItem::GetFilename()
{
	return m_strFilename.c_str();
}

void CAssetTextureItem::SetRelativePath( const char* pPath )
{
	m_strRelativePath = pPath;
}

const char* CAssetTextureItem::GetRelativePath()
{
	return m_strRelativePath;
}

UINT CAssetTextureItem::GetFlags() const
{
	return m_flags;
}

void CAssetTextureItem::SetFlags( UINT aFlags )
{
	m_flags = aFlags;
}

void CAssetTextureItem::SetFlag( EAssetFlags aFlag, bool bSet )
{
	m_flags = bSet ? ( m_flags | (UINT)aFlag ) : ( m_flags & (~(UINT)aFlag) );
}

bool CAssetTextureItem::IsFlagSet( EAssetFlags aFlag )
{
	return m_flags & (UINT) aFlag;
}

void CAssetTextureItem::SetIndex( UINT aIndex )
{
	m_assetIndex = aIndex;
}

UINT CAssetTextureItem::GetIndex()
{
	return m_assetIndex;
}

bool CAssetTextureItem::GetAssetFieldValue( const char* pFieldName, void* pDest )
{
	if( isAssetField( "thumbTipInfo" ) )
	{
		stack_string str;
		str.Format( "Path: %s\nWidth: %d\nHeight: %d\nMips: %d\nFilesize: %.2f kB", m_strRelativePath.c_str(), m_nTextureWidth, m_nTextureHeight, m_nMips, (float) m_nFileSize / 1024.0f );
		*(string*)pDest = str.c_str();
		return true;
	}
	else
	if( isAssetField( "thumbOneLineInfo" ) && m_nTextureWidth )
	{
		stack_string str;
		str.Format( "[ %d x %d ]", m_nTextureWidth, m_nTextureHeight );
		*(string*)pDest = str.c_str();
		return true;
	}
	else
	if( isAssetField( "errors" ) )
	{
		string str;

		if( m_nMips == 1 )
		{
			str += "WARNING: MipCount is zero\n";
		}

		*(string*)pDest = str.c_str();
		return true;
	}
	else
	if( isAssetField( "filename" ) )
	{
		*(string*)pDest = m_strFilename;
		return true;
	}
	else
	if( isAssetField( "relativepath" ) )
	{
		*(string*)pDest = m_strRelativePath;
		return true;
	}
	else
	if( isAssetField( "extension" ) )
	{
		*(string*)pDest = m_strExtension;
		return true;
	}
	else
	if( isAssetField( "filesize" ) )
	{
		*(int*)pDest = m_nFileSize;
		return true;
	}
	else
	if( isAssetField( "width" ) )
	{
		*(int*)pDest = m_nTextureWidth;
		return true;
	}
	else
	if( isAssetField( "height" ) )
	{
		*(int*)pDest = m_nTextureHeight;
		return true;
	}
	else
	if( isAssetField( "mips" ) )
	{
		*(int*)pDest = m_nMips;
		return true;
	}
	else
	if( isAssetField( "format" ) )
	{
		*(string*)pDest = CImageExtensionHelper::NameForTextureFormat(m_format);
		return true;
	}
	else
	if( isAssetField( "type" ) )
	{
		*(string*)pDest = m_bIsCubemap ? "Cube" : "2D";
		return true;
	}
	else
	if( isAssetField( "usedinlevel" ) )
	{
		*(string*)pDest = m_bUsedInLevel ? "Yes" : "No";
		return true;
	}

	return false;
}

void CAssetTextureItem::GetDrawingRectangle( CRect& rstDrawingRectangle ) const
{ 
	rstDrawingRectangle = m_oDrawingRectangle;
}

void	CAssetTextureItem::SetDrawingRectangle( const CRect& crstDrawingRectangle )
{
	m_oDrawingRectangle = crstDrawingRectangle;
}

bool CAssetTextureItem::Cache()
{
	if( m_flags & eAssetFlags_Cached )
	{
		return true;
	}

	if( m_flags & eAssetFlags_ThumbnailCached )
	{
		return true;
	}

	CImage img;
	CAlphaBitmap bmp;
	string str = m_strRelativePath;

	str += m_strFilename;
	
	if( !CImageUtil::LoadImage( str.c_str(), img ) )
	{
		SetFlag( eAssetFlags_Invalid, true );
		return false;
	}

	img.SwapRedAndBlue();
	unsigned int* pData = img.GetData();

	if( !pData )
	{
		return false;
	}

	if( 0 == img.GetWidth() || 0 == img.GetHeight() )
	{
		return false;
	}

	if( !bmp.Create( pData, img.GetWidth(), img.GetHeight(), true ) )
	{
		return false;
	}

	// resize the bitmap, so we dont have to stretch when drawing the thumb item
	if( !m_oCachedBmp.Create( NULL, gSettings.sAssetBrowserSettings.nThumbSize, gSettings.sAssetBrowserSettings.nThumbSize ) )
	{
		return false;
	}

	// compute correct ratio to scale the image right
	float ratioX = 1, ratioY = 1;
	UINT newWidth = img.GetWidth();
	UINT newHeight = img.GetHeight();

	if( newWidth > newHeight )
	{
		if( newWidth > 0 )
		{
			ratioY = (float) newHeight / newWidth;
		}
	}
	else
	{
		if( newHeight > 0 )
		{
			ratioX = (float) newWidth / newHeight;
		}
	}

	newWidth = ratioX * gSettings.sAssetBrowserSettings.nThumbSize;
	newHeight = ratioY * gSettings.sAssetBrowserSettings.nThumbSize;

	if( !( m_flags & eAssetFlags_ThumbnailCached ) )
	{
		BLENDFUNCTION blendFunc;
		blendFunc.AlphaFormat = AC_SRC_ALPHA;
		blendFunc.BlendOp = AC_SRC_OVER;
		blendFunc.SourceConstantAlpha = 0xFF;
		blendFunc.BlendFlags = 0;
		m_oCachedBmp.GetDC().AlphaBlend(( gSettings.sAssetBrowserSettings.nThumbSize - newWidth ) / 2,
																		( gSettings.sAssetBrowserSettings.nThumbSize - newHeight ) / 2,
																		newWidth, newHeight, &bmp.GetDC(), 0, 0, img.GetWidth(), img.GetHeight(), blendFunc );

		// Create the alpha-only thumbnail.
		BITMAP bmpInfo;
		m_oCachedBmp.GetBitmap().GetBitmap(&bmpInfo);
		int byteCount = bmpInfo.bmWidth*bmpInfo.bmWidth*(bmpInfo.bmBitsPixel/8);
		std::vector<unsigned char> bits;
		bits.resize(byteCount);
		
		m_oCachedBmp.GetBitmap().GetBitmapBits(byteCount, &bits[0]);
		for(int i = 0; i < byteCount; i += 4)
		{
			bits[i] = bits[i+1] = bits[i+2] = bits[i+3];
			bits[i+3] = 255;
		}
		m_oCachedBmpAlpha.Create(&bits[0], bmpInfo.bmWidth, bmpInfo.bmHeight, true);

		// Create the RGB-only thumbnail. (The 'StretchBlt' ignores the alpha channel.)
		if( m_oCachedBmpRGB.Create(NULL, bmpInfo.bmWidth, bmpInfo.bmHeight, true) )
		{
			m_oCachedBmpRGB.GetDC().SetStretchBltMode( HALFTONE );
			m_oCachedBmpRGB.GetDC().StretchBlt(( gSettings.sAssetBrowserSettings.nThumbSize - newWidth ) / 2,
																				( gSettings.sAssetBrowserSettings.nThumbSize - newHeight ) / 2,
																				newWidth, newHeight, &bmp.GetDC(), 0, 0, img.GetWidth(), img.GetHeight(), SRCCOPY );
		}
	}

	bmp.Free();
	m_nTextureWidth = img.GetWidth();
	m_nTextureHeight = img.GetHeight();
	m_nMips = img.GetNumberOfMipMaps();
	m_format = img.GetFormat();
	m_bIsCubemap = img.IsCubemap();

	if( 0 == m_nMips )
	{
		m_flags |= eAssetFlags_HasErrors;
	}

	SetFlag( eAssetFlags_Cached, true );
	SetFlag( eAssetFlags_ThumbnailCached, true );
	SetFlag( eAssetFlags_CachedFieldsInfo, true );

	if( 0 >= m_nTextureWidth || 0 >= m_nTextureHeight )
	{
		SetFlag( eAssetFlags_Invalid, true );
	}

	// lets push this asset to the thumbs pool/queue, so we can have a pool of thumbs kept in memory until they are too old
	// this help browsing the assets, no need to recache each time a row of assets thumbs previously 
	if( m_piOwnerDatabase && m_piOwnerDatabase->GetAssociatedViewer() )
	{
		m_piOwnerDatabase->GetAssociatedViewer()->PushToThumbsCacheQueue( this );
	}

	return true;
}

bool CAssetTextureItem::CacheFieldsInfo()
{
	// just load the texture and get its fields, like mip, width, height
	// if not already loaded
	if( m_flags & eAssetFlags_CachedFieldsInfo )
		return true;

	CImage img;
	string str = m_strRelativePath;

	str += m_strFilename;

	if( !CImageUtil::LoadImage( str.c_str(), img ) )
	{
		SetFlag( eAssetFlags_Invalid, true );
		return false;
	}

	m_nTextureWidth = img.GetWidth();
	m_nTextureHeight = img.GetHeight();
	m_nMips = img.GetNumberOfMipMaps();
	m_format = img.GetFormat();
	m_bIsCubemap = img.IsCubemap();

	if( 0 == m_nMips )
	{
		m_flags |= eAssetFlags_HasErrors;
	}

	SetFlag( eAssetFlags_CachedFieldsInfo, true );

	if( 0 >= m_nTextureWidth || 0 >= m_nTextureHeight )
	{
		SetFlag( eAssetFlags_Invalid, true );
	}

	return true;
}

bool CAssetTextureItem::UnCache()
{
	if( !( m_flags & eAssetFlags_Cached ) )
	{
		return true;
	}

	SetFlag( eAssetFlags_Cached, false );

	return true;
}

bool CAssetTextureItem::UnCacheThumbnail()
{
	if( !( m_flags & eAssetFlags_ThumbnailCached ) )
	{
		return true;
	}
	
	SetFlag( eAssetFlags_ThumbnailCached, false );
	m_oCachedBmp.Free();
	m_oCachedBmpAlpha.Free();
	m_oCachedBmpRGB.Free();

	return true;
}

void CAssetTextureItem::InteractiveRender( HWND hRenderWindow, const CRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, UINT aKeyFlags )
{
	//TODO: pan and zoom texture ?
	Render( hRenderWindow, rstViewport, false );
}

bool CAssetTextureItem::Render( HWND hRenderWindow, const CRect& rstViewport, bool bCacheThumbnail )
{
	//TODO: maybe render cube maps as a 3D cube ?

	return true;
}

bool CAssetTextureItem::DrawThumbImage( HDC hDC, const CRect& rRect )
{
	if( m_flags & eAssetFlags_ThumbnailCached )
	{
		CDC dc;

		dc.Attach( hDC );

		if( GetDrawingOption() == eAssetDrawing_RGBA )
			dc.BitBlt( rRect.left, rRect.top, rRect.Width(), rRect.Height(), &m_oCachedBmp.GetDC(), 0, 0, SRCCOPY );
		else if( GetDrawingOption() == eAssetDrawing_Alpha )
			dc.BitBlt( rRect.left, rRect.top, rRect.Width(), rRect.Height(), &m_oCachedBmpAlpha.GetDC(), 0, 0, SRCCOPY );
		else if( GetDrawingOption() == eAssetDrawing_RGB )
			dc.BitBlt( rRect.left, rRect.top, rRect.Width(), rRect.Height(), &m_oCachedBmpRGB.GetDC(), 0, 0, SRCCOPY );

		dc.Detach();

		return true;
	}

	return false;
}

bool CAssetTextureItem::HitTest( int nX, int nY )
{
	return m_oDrawingRectangle.PtInRect( CPoint( nX, nY ) ) == TRUE;
}

bool CAssetTextureItem::HitTest( const CRect& roTestRect )
{
	CRect oIntersection;

	return oIntersection.IntersectRect( &m_oDrawingRectangle, &roTestRect ) == TRUE;
}

void CAssetTextureItem::CacheFieldsInfoForLoadedTex( const ITexture *pTexture )
{
	if( m_flags & eAssetFlags_CachedFieldsInfo )
		return;

	m_nTextureWidth = pTexture->GetWidth();
	m_nTextureHeight = pTexture->GetHeight();
	m_nMips = pTexture->GetNumMips();
	m_format = pTexture->GetTextureDstFormat();
	m_bIsCubemap = pTexture->GetTextureType() == eTT_Cube; 

	if( 0 == m_nMips )
	{
		m_flags |= eAssetFlags_HasErrors;
	}

	SetFlag( eAssetFlags_CachedFieldsInfo, true );

	if( 0 >= m_nTextureWidth || 0 >= m_nTextureHeight )
	{
		SetFlag( eAssetFlags_Invalid, true );
	}
}

void CAssetTextureItem::DrawTextOnReportImage( CAlphaBitmap &abm ) const
{
	const COLORREF filenameShadowColor = RGB( 0, 0, 0 );
	const COLORREF filenameColor = RGB( 255, 255, 0 );
	const COLORREF otherInfosColor = RGB( 0, 0, 0 );
	CFont fontInfoTitle, fontInfo;
	fontInfoTitle.CreatePointFont(95, "Arial Bold");
	fontInfo.CreatePointFont(80, "Arial");

	CDC dc;
	dc.Attach(abm.GetDC());
	dc.SetBkMode(TRANSPARENT);
	dc.SelectObject( fontInfoTitle );
	dc.SetTextColor(filenameShadowColor);
	dc.TextOut(kAssetViewer_OverlayTextLeftMargin+1, 
						kAssetViewer_OverlayTextTopMargin+1, m_strFilename.c_str());
	dc.SetTextColor(filenameColor);
	dc.TextOut(kAssetViewer_OverlayTextLeftMargin, 
						kAssetViewer_OverlayTextTopMargin, m_strFilename.c_str());

	string reportInfos;
	reportInfos.Format( 
		"Path: %s\nRes: %dx%d\nFormat: %s\nType: %s\n",
		m_strRelativePath,
		m_nTextureWidth,
		m_nTextureHeight,
		CImageExtensionHelper::NameForTextureFormat(m_format),
		m_bIsCubemap ? "Cube" : "2D");

	CSize	titleSize;
	titleSize = dc.GetTextExtent(m_strFilename.c_str());
	CRect rcTextInfo;
	rcTextInfo.left = kAssetViewer_OverlayTextLeftMargin;
	rcTextInfo.right = abm.GetWidth();
	rcTextInfo.top = titleSize.cy + 15;
	rcTextInfo.bottom = abm.GetHeight();
	dc.SetTextColor(otherInfosColor);
	dc.SelectObject(fontInfo);
	dc.DrawText(reportInfos.c_str(), &rcTextInfo, DT_WORD_ELLIPSIS);
	dc.Detach();

	fontInfoTitle.DeleteObject();
	fontInfo.DeleteObject();
}

bool CAssetTextureItem::SaveReportImage( const char *filePath ) const
{
	if((m_flags & eAssetFlags_ThumbnailCached) == 0)
		return false;

	CAlphaBitmap& abm = const_cast<CAlphaBitmap&>(m_oCachedBmp);
	CBitmap& bitmap = abm.GetBitmap();
	BITMAP bmpInfo;
	bitmap.GetBitmap(&bmpInfo);
	int byteCount = bmpInfo.bmWidth*bmpInfo.bmWidth*(bmpInfo.bmBitsPixel/8);

	bool bImageIsWideEnough = bmpInfo.bmWidth >= kAssetViewer_MinWidthForOverlayText;

	std::vector<unsigned char> bitsBackUp;
	if(bImageIsWideEnough)					// Back up the bitmap and draw the overlay text.
	{
		bitsBackUp.resize(byteCount);
		bitmap.GetBitmapBits(byteCount, &bitsBackUp[0]);
		DrawTextOnReportImage(abm);
	}

	CImage image;
	image.Allocate(bmpInfo.bmWidth, bmpInfo.bmWidth);
	bitmap.GetBitmapBits(byteCount, image.GetData());
	image.SwapRedAndBlue();

	if(bImageIsWideEnough)				// Restore the bitmap.
		bitmap.SetBitmapBits(byteCount, &bitsBackUp[0]);

	return CImageUtil::SaveBitmap(filePath, image);
}

bool CAssetTextureItem::SaveReportText( const char *filePath ) const
{
	
	FILE *file = fopen(filePath, "wt");
	if(file == NULL)
		return false;

	fprintf(file,
					"Filename: %s\nPath: %s\nRes: %dx%d\nFormat: %s\nType: %s\n",
					m_strFilename,
					m_strRelativePath,
					m_nTextureWidth,
					m_nTextureHeight,
					CImageExtensionHelper::NameForTextureFormat(m_format),
					m_bIsCubemap ? "Cube" : "2D");

	fclose(file);

	return true;
}