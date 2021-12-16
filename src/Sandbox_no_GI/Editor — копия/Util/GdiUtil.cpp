////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   GdiUtil.h
//  Version:     v1.00
//  Created:     01/02/2010 by Nicusor Nedelcu.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History: 
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GdiUtil.h"
#include "Image.h"

CGdiCanvas::CGdiCanvas()
{
	m_hCompatibleDC = 0;
	m_width = m_height = 0;
}

CGdiCanvas::~CGdiCanvas()
{
	Free();
}

bool CGdiCanvas::Create( HDC hCompatibleDC, UINT aWidth, UINT aHeight )
{
	if( !hCompatibleDC )
		return false;

	if( !aWidth || !aHeight )
		return false;

	Free();

	m_hCompatibleDC = hCompatibleDC;
	CDC cdc;
	cdc.Attach( hCompatibleDC );
	
	try
	{
		if( !m_memBmp.CreateCompatibleBitmap( &cdc, aWidth, aHeight ) )
		{
			throw false;
		}

		if( !m_memDC.CreateCompatibleDC( &cdc ) )
		{
			throw false;
		}

		m_memDC.SelectObject( m_memBmp );
		cdc.Detach();
		m_memDC.SetBkMode( TRANSPARENT );
		m_memDC.SetStretchBltMode( HALFTONE );
	}

	catch(bool)
	{
		cdc.Detach();
		return false;
	}

	return true;
}

UINT CGdiCanvas::GetWidth()
{
	return m_width;
}

UINT CGdiCanvas::GetHeight()
{
	return m_height;
}

bool CGdiCanvas::BlitTo( HDC hDestDC, int aDestX, int aDestY, int aDestWidth, int aDestHeight, int aSrcX, int aSrcY, int aRop )
{
	CDC dc;
	BITMAP bmpInfo;

	dc.Attach( hDestDC );
	
	if( !m_memBmp.GetBitmap( &bmpInfo ) )
	{
		dc.Detach();
		return false;
	}

	aDestWidth = aDestWidth == -1 ?  bmpInfo.bmWidth : aDestWidth;
	aDestHeight = aDestHeight == -1 ?  bmpInfo.bmHeight : aDestHeight;
	dc.BitBlt( aDestX, aDestY, aDestWidth, aDestHeight, &m_memDC, aSrcX, aSrcY, aRop );
	dc.Detach();
}

bool CGdiCanvas::BlitTo( HWND hDestWnd, int aDestX, int aDestY, int aDestWidth, int aDestHeight, int aSrcX, int aSrcY, int aRop )
{
	HDC hDC = ::GetDC( hDestWnd );
	bool bRet = BlitTo( hDC, aDestX, aDestY, aDestWidth, aDestHeight, aSrcX, aSrcY, aRop );
	ReleaseDC( hDestWnd, hDC );

	return bRet;
}

void CGdiCanvas::Free()
{
	if( m_memBmp.GetSafeHandle() )
		m_memBmp.DeleteObject();

	if( m_memDC.GetSafeHdc() )
		m_memDC.DeleteDC();

	m_width = 0;
	m_height = 0;
	m_hCompatibleDC = 0;
}

CDC& CGdiCanvas::GetDC()
{
	return m_memDC;
}

bool CGdiCanvas::GradientFillRect( CRect& rRect, COLORREF aStartColor, COLORREF aEndColor, int aFillType )
{
	TRIVERTEX vert[2];
	GRADIENT_RECT gRect;
	
	vert[0] .x = rRect.left;
	vert[0] .y = rRect.top;
	vert[0] .Red = GetRValue(aStartColor) << 8;
	vert[0] .Green = GetGValue(aStartColor) << 8;
	vert[0] .Blue = GetBValue(aStartColor) << 8;
	vert[0] .Alpha = 0x0000;

	vert[1] .x = rRect.right;
	vert[1] .y = rRect.bottom;
	vert[1] .Red = GetRValue(aEndColor) << 8;
	vert[1] .Green = GetGValue(aEndColor) << 8;
	vert[1] .Blue = GetBValue(aEndColor) << 8;
	vert[1] .Alpha = 0x0000;

	gRect.UpperLeft = 0;
	gRect.LowerRight = 1;
	
	return TRUE == GradientFill( m_memDC.GetSafeHdc(), vert, 2, &gRect, 1, aFillType );
}

bool CGdiCanvas::BitBltWithAlpha( CDC& rBmpDC, int aDestX, int aDestY, int aDestWidth, int aDestHeight, int aSrcX, int aSrcY, int aSrcWidth, int aSrcHeight, BLENDFUNCTION* pBlendFunc )
{
	BLENDFUNCTION bf;

	if( !pBlendFunc )
	{
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = 0xFF;
		bf.AlphaFormat = AC_SRC_ALPHA;
	}
	else
	{
		bf = *pBlendFunc;
	}

	CBitmap* pBmp = rBmpDC.GetCurrentBitmap();

	assert( pBmp );

	if( !pBmp )
		return false;

	BITMAP bmpInfo;

	if( !pBmp->GetBitmap( &bmpInfo ) )
		return false;

	aDestWidth = aDestWidth == -1 ?  bmpInfo.bmWidth : aDestWidth;
	aDestHeight = aDestHeight == -1 ?  bmpInfo.bmHeight : aDestHeight;
	aSrcWidth = aSrcWidth == -1 ?  bmpInfo.bmWidth : aSrcWidth;
	aSrcHeight = aSrcHeight == -1 ?  bmpInfo.bmHeight : aSrcHeight;

	return TRUE == AlphaBlend( m_memDC.GetSafeHdc(), aDestX, aDestY, aDestWidth, aDestHeight, rBmpDC.GetSafeHdc(), aSrcX, aSrcY, aSrcWidth, aSrcHeight, bf );
}

void CGdiCanvas::BreakTextByChars( const char* pText, string& rOutStr, const CRect& rRect )
{
	UINT i = 0, nSize = strlen( pText );
	string chunk;
	CSize textSize;
	int rectWidth = rRect.Width();

	rOutStr = "";

	while( i < nSize )
	{
		chunk += pText[i];
		textSize = m_memDC.GetTextExtent( chunk.c_str() );

		// if current chunk of text os bigger than the rect width, cut
		if( textSize.cx > rectWidth )
		{
			// special case, rect width smaller than the first char in the text
			if( !i )
			{
				rOutStr = pText;
				return;
			}

			rOutStr += chunk.substr( 0, chunk.size() - 1 );
			rOutStr += string( "\r" );
			chunk = "";
			continue;
		}

		++i;
	}

	rOutStr += chunk;
}

bool CGdiCanvas::ComputeThumbsLayoutInfo( float aContainerWidth, float aThumbWidth, float aMargin, UINT aThumbCount, UINT& rThumbsPerRow, float& rNewMargin )
{
	 rThumbsPerRow = 0;
	 rNewMargin = 0;

	if( aThumbWidth <= 0 || aMargin <= 0 || ( aThumbWidth + aMargin * 2 ) <= 0 )
	{
		return false;
	}

	if( aContainerWidth <= 0 )
	{
		return true;
	}

	rThumbsPerRow = (int) aContainerWidth / ( aThumbWidth + aMargin * 2 );

	if( ( aThumbWidth + aMargin * 2 ) * aThumbCount < aContainerWidth )
	{
		rNewMargin = aMargin;
	}
	else
	{
		if( rThumbsPerRow > 0 )
		{
			rNewMargin = ( aContainerWidth - rThumbsPerRow * aThumbWidth );
			
			if( rNewMargin > 0 )
			{
				rNewMargin = (float)rNewMargin / rThumbsPerRow / 2.0f;
			}
		}
	}

	return true;
}

//---

CAlphaBitmap::CAlphaBitmap()
{
	m_width = m_height = 0;
	m_hOldBmp = NULL;
}

CAlphaBitmap::~CAlphaBitmap()
{
	Free();
}

bool CAlphaBitmap::Load( const char* pFilename, bool bVerticalFlip )
{
	Free();
	WCHAR wzFilename[MAX_PATH];
	mbstowcs( wzFilename, pFilename, MAX_PATH );
	Gdiplus::Bitmap img( wzFilename );
	Gdiplus::BitmapData bmData;

	if( 0 != img.GetLastStatus() )
		return false;

	HDC hDC = ::GetDC( GetDesktopWindow() );
	CDC dc;

	dc.Attach( hDC );
	
	if( !m_dcForBlitting.CreateCompatibleDC( &dc ) )
	{
		dc.Detach();
		ReleaseDC( GetDesktopWindow(), hDC );
		return false;
	}
	
	if( bVerticalFlip )
	{
		img.RotateFlip( Gdiplus::RotateNoneFlipY );
	}

	img.LockBits( NULL, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmData );

	if( !bmData.Scan0 )
	{
		dc.Detach();
		ReleaseDC( GetDesktopWindow(), hDC );
		return false;
	}

	// premultiply alpha, AlphaBlend GDI expects it
	for( UINT y = 0; y < bmData.Height; ++y )
	{
		BYTE *pPixel = (BYTE*) bmData.Scan0 + bmData.Width * 4 * y;
		
		for( UINT x = 0; x < bmData.Width; ++x )
		{
			pPixel[0] = ( (int)pPixel[0] * pPixel[3] + 127 ) >> 8;
			pPixel[1] = ( (int)pPixel[1] * pPixel[3] + 127 ) >> 8;
			pPixel[2] = ( (int)pPixel[2] * pPixel[3] + 127 ) >> 8;
			pPixel += 4;
		}
	}

	BITMAPINFO bmi;

	ZeroMemory( &bmi, sizeof( BITMAPINFO ) );
	bmi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	bmi.bmiHeader.biWidth = bmData.Width;
	bmi.bmiHeader.biHeight = bmData.Height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = bmData.Width * bmData.Height * 4;
	void* pBits = NULL;
	HBITMAP hBmp = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS, &pBits, 0, 0 );

	if( !hBmp )
	{
		dc.Detach();
		ReleaseDC( GetDesktopWindow(), hDC );
		img.UnlockBits( &bmData );
		return false;
	}

	if( !pBits || !bmData.Scan0 )
	{
		dc.Detach();
		ReleaseDC( GetDesktopWindow(), hDC );
		img.UnlockBits( &bmData );
		return false;
	}

	memcpy( pBits, bmData.Scan0, bmi.bmiHeader.biSizeImage );
	m_bmp.Attach( hBmp );
	m_width = bmData.Width;
	m_height = bmData.Height;
	img.UnlockBits( &bmData );
	m_dcForBlitting.SelectObject( m_bmp );
	dc.Detach();
	ReleaseDC( GetDesktopWindow(), hDC );

	return true;
}

bool CAlphaBitmap::Create( void* pData, UINT aWidth, UINT aHeight, bool bVerticalFlip )
{
	Free();
	HDC hDC = ::GetDC( GetDesktopWindow() );
	CDC dc;

	dc.Attach( hDC );
	
	if( !m_dcForBlitting.CreateCompatibleDC( &dc ) )
	{
		dc.Detach();
		ReleaseDC( GetDesktopWindow(), hDC );
		return false;
	}

	if( !m_bmp.CreateCompatibleBitmap( &dc, aWidth, aHeight ) )
	{
		dc.Detach();
		ReleaseDC( GetDesktopWindow(), hDC );
		return false;
	}

	if( pData )
	{
		// copy over the raw 32bpp data
		if( bVerticalFlip )
		{
			UINT* pScanline = new UINT[aWidth];

			assert( pScanline );
			
			if( !pScanline )
			{
				dc.Detach();
				ReleaseDC( GetDesktopWindow(), hDC );
				Free();
				return false;
			}
			
			UINT scanlineSize = aWidth*4;

			for( UINT i = 0, iCount = aHeight >> 1; i < iCount; ++i )
			{
				// top scanline position
				UINT* pTopScanPos = ((UINT*)pData) + i*aWidth;
				// bottom scanline position
				UINT* pBottomScanPos = ((UINT*)pData) + ( aHeight - i - 1 ) * aWidth;
				// save a scanline from top
				memcpy( pScanline, pTopScanPos, scanlineSize );
				// copy a scanline from bottom to top
				memcpy( pTopScanPos, pBottomScanPos, scanlineSize );
				// put the top scan to bottom
				memcpy( pBottomScanPos, pScanline, scanlineSize );
			}

			delete [] pScanline;
		}

		BITMAPINFO bmi;
		ZeroMemory( &bmi.bmiHeader, sizeof( bmi.bmiHeader ) );
		bmi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biWidth = aWidth;
		bmi.bmiHeader.biHeight = aHeight;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biSizeImage = aWidth*aHeight*4;
		bmi.bmiHeader.biCompression = BI_RGB;

		if( !SetDIBits( m_dcForBlitting.GetSafeHdc(), m_bmp, 0, aHeight, pData, &bmi, DIB_RGB_COLORS ) )
		{
			dc.Detach();
			ReleaseDC( GetDesktopWindow(), hDC );
			return false;
		}
	}
	else
	{
		BITMAP bm;

		ZeroMemory( &bm, sizeof(BITMAP) );

		if( !m_bmp.GetBitmap( &bm ) )
		{
			dc.Detach();
			ReleaseDC( GetDesktopWindow(), hDC );
			return false;
		}

		if( bm.bmBits )
		{
			// just clear up the bitmap bits to zero
			ZeroMemory( bm.bmBits, bm.bmWidth * bm.bmHeight * 4 );
		}
	}

	// select it into the DC so we can draw on it or blit it
	m_hOldBmp = (HBITMAP)m_dcForBlitting.SelectObject( m_bmp );
	// we dont need this screen DC anymore
	m_width = aWidth;
	m_height = aHeight;
	dc.Detach();
	ReleaseDC( GetDesktopWindow(), hDC );

	return true;
}

CBitmap& CAlphaBitmap::GetBitmap()
{
	return m_bmp;
}

CDC& CAlphaBitmap::GetDC()
{
	return m_dcForBlitting;
}

void CAlphaBitmap::Free()
{
	if( m_dcForBlitting.GetSafeHdc() )
	{
		m_dcForBlitting.SelectObject( m_hOldBmp );
		
		if( m_bmp.m_hObject )
		{
			m_bmp.DeleteObject();
		}
		
		m_dcForBlitting.DeleteDC();
		m_hOldBmp = NULL;
	}
}

UINT CAlphaBitmap::GetWidth()
{
	return m_width;
}

UINT CAlphaBitmap::GetHeight()
{
	return m_height;
}
