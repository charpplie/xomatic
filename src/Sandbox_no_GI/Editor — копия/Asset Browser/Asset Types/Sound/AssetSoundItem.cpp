////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetSoundItem.cpp
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description: Implementation of AssetSoundItem.h
// -------------------------------------------------------------------------  
//  History:
//		05/05/2010	20:12 : Nicusor Nedelcu - created
//		10/05/2010	13:58 : Nicusor Nedelcu - moved common code out of the class to AssetBrowserCommon.h/cpp
//
//////////////////////////////////////////////////////////////////////////// 

#include "StdAfx.h"
#include "AssetSoundItem.h"
#include "Util/MemoryBlock.h"
#include "Util/Image.h"
#include "Util/ImageUtil.h"
#include "Util/PathUtil.h"
#include "Include/IAssetDisplayDatabase.h"
#include "ISound.h"
#include "IMusicSystem.h"
#include "IRenderer.h"
#include "Include/IAssetViewer.h"
#include "ImageExtensionHelper.h"

namespace AssetBrowser
{
	const char* kSoundImageFilename = "Editor/UI/Icons/AssetBrowserSound.png";
};

CAlphaBitmap CAssetSoundItem::m_oSoundBmp;

CAssetSoundItem::CAssetSoundItem(): 
CAssetDisplay()
{
	m_soundId = 0;
	m_flags = eAssetFlags_UseGdiRendering | eAssetFlags_InteractiveRenderSupported;

	if( !m_oSoundBmp.GetBitmap().GetSafeHandle() )
	{
		m_oSoundBmp.Load( AssetBrowser::kSoundImageFilename, true );
	}
}

CAssetSoundItem::~CAssetSoundItem()
{
	// empty, call FreeData first
}

HRESULT STDMETHODCALLTYPE CAssetSoundItem::QueryInterface( const IID &riid, void **ppvObj ) 
{ 
	if( riid == __uuidof(IAssetDisplay) /* && m_pIntegrator*/ )
	{
		*ppvObj = this;
		return S_OK;
	}
	return E_NOINTERFACE; 
}

ULONG STDMETHODCALLTYPE CAssetSoundItem::AddRef()
{
	return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetSoundItem::Release() 
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

bool CAssetSoundItem::GetAssetFieldValue( const char* pFieldName, void* pDest )
{
	if( IS_ASSET_FIELD_NAME( "thumbToolTipText" ) )
	{
		stack_string str;

		CString tagsText;
		CAssetDisplay::GetAssetFieldValue( "tags", &tagsText );
		//TODO: pre-bake in the caching code, as a member
		str.Format( "Path: %s\nLength: %d\nFilesize: %.2f kB\nTags: %s\n", 
			m_strRelativePath.GetBuffer(), m_nSoundLengthMsec, (float) m_nFileSize / 1024.0f, tagsText.GetBuffer() );
		*(CString*)pDest = str.c_str();
		return true;
	}
	else
	if( IS_ASSET_FIELD_NAME( "thumbOneLineText" ) && m_nSoundLengthMsec )
	{
		stack_string str;

		//TODO: pre-bake in the caching code, as a member
		str.Format( "[ Length: %d ms ]", m_nSoundLengthMsec );
		*(CString*)pDest = str.c_str();
		return true;
	}
	else
	if( IS_ASSET_FIELD_NAME( "errors" ) )
	{
		CString str;

		//TODO: pre-bake in the caching code, as a member
		if( m_nSoundLengthMsec == 1 )
		{
			str += "WARNING: Sound length (msec) is zero\n";
		}

		*(CString*)pDest = str.GetBuffer();
		return true;
	}
	else
	if( IS_ASSET_FIELD_NAME( "length" ) )
	{
		*(int*)pDest = m_nSoundLengthMsec;
		return true;
	}

	// else, check if the common fields are requested
	return CAssetDisplay::GetAssetFieldValue( pFieldName, pDest );
}

bool CAssetSoundItem::Cache()
{
	if( m_flags & eAssetFlags_Cached )
	{
		return true;
	}

	if( m_flags & eAssetFlags_ThumbnailCached )
	{
		return true;
	}

	ISystem *pSystem = GetISystem();
	ISoundSystem *pSoundSystem = pSystem->GetISoundSystem();
	CString soundname = m_strRelativePath + m_strFilename;
	ISound* pSound = pSoundSystem->CreateSound( soundname.GetBuffer(), FLAG_SOUND_LOAD_SYNCHRONOUSLY );
	m_soundId = 0;

	if( pSound )
	{
		m_nSoundLengthMsec = pSound->GetLengthMs();
		m_soundId = pSound->GetId();
	}
	
	SetFlag( eAssetFlags_Cached, true );
	SetFlag( eAssetFlags_ThumbnailCached, true );
	SetFlag( eAssetFlags_CachedFieldsInfo, true );

	if( 0 == m_nSoundLengthMsec )
	{
		SetFlag( eAssetFlags_Invalid, true );
	}

	return true;
}

bool CAssetSoundItem::CacheFieldsInfo()
{
	// if not already loaded
	if( m_flags & eAssetFlags_CachedFieldsInfo )
		return true;

	ISystem *pSystem = GetISystem();
	ISoundSystem *pSoundSystem = pSystem->GetISoundSystem();
	CString soundname = m_strRelativePath + m_strFilename;
	ISound* pSound = pSoundSystem->CreateSound( soundname.GetBuffer(), FLAG_SOUND_LOAD_SYNCHRONOUSLY|FLAG_SOUND_EDITOR );
	m_soundId = 0;

	if( pSound )
	{
		m_nSoundLengthMsec = pSound->GetLengthMs();
		m_soundId = pSound->GetId();
	}

	SetFlag( eAssetFlags_CachedFieldsInfo, true );

	if( 0 == m_nSoundLengthMsec )
	{
		SetFlag( eAssetFlags_Invalid, true );
	}

	return true;
}

bool CAssetSoundItem::UnCache()
{
	if( !( m_flags & eAssetFlags_Cached ) )
	{
		return true;
	}

	//TODO: how to unload a sound ?
	SetFlag( eAssetFlags_Cached, false );

	return true;
}

bool CAssetSoundItem::UnCacheThumbnail()
{
	if( !( m_flags & eAssetFlags_ThumbnailCached ) )
	{
		return true;
	}
	
	SetFlag( eAssetFlags_ThumbnailCached, false );

	return true;
}

void CAssetSoundItem::OnBeginPreview( const HWND hQuickPreviewWnd, const HDC hMemDC )
{
	ISound* pSound = GetISystem()->GetISoundSystem()->GetSound( m_soundId );

	if( pSound )
	{
		pSound->GetInterfaceExtended()->SetFlags( pSound->GetFlags() & ~FLAG_SOUND_RADIUS );
		pSound->SetSemantic( eSoundSemantic_Sandbox_Browser );
		pSound->Play();
	}
}

void CAssetSoundItem::OnEndPreview()
{
	ISound* pSound = GetISystem()->GetISoundSystem()->GetSound( m_soundId );

	if( pSound )
	{
		pSound->Stop();
	}

	m_hPreviewDC = 0;
}

void CAssetSoundItem::InteractiveRender( HWND hRenderWindow, const CRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, UINT aKeyFlags )
{
	Render( hRenderWindow, rstViewport, false );
}

void CAssetSoundItem::OnInteractiveRenderKeyEvent( bool bKeyDown, UINT aChar, UINT aKeyFlags )
{
}

bool CAssetSoundItem::Render( HWND hRenderWindow, const CRect& rstViewport, bool bCacheThumbnail )
{
	return true;
}

bool CAssetSoundItem::DrawThumbImage( HDC hDC, const CRect& rRect )
{
	if( m_flags & eAssetFlags_ThumbnailCached )
	{
		CDC dc;

		dc.Attach( hDC );
		dc.StretchBlt( rRect.left, rRect.top, rRect.Width(), rRect.Height(), &m_oSoundBmp.GetDC(), 0, 0, m_oSoundBmp.GetWidth(), m_oSoundBmp.GetHeight(), SRCCOPY );
		dc.Detach();

		return true;
	}

	return false;
}

void CAssetSoundItem::DrawTextOnReportImage( CAlphaBitmap &rDestDmp ) const
{
}

bool CAssetSoundItem::SaveReportImage( const char *pFilePath ) const
{
	return false;
}

bool CAssetSoundItem::SaveReportText( const char *pFilePath ) const
{
	return false;
}
