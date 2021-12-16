/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: AssetViewer.h  ,v 1.1 2008/08/19 12:54:41 PauloZaffari Exp wwwrun $
$DateTime$
Description:
This file declares a control which objective is to display 
multiple Assets allowing selection and preview of such things.
It also must handle scrolling and changes in the Asset 
cell display size.
-------------------------------------------------------------------------
History:
- 19:08:2008   12:54 : Created by Paulo Zaffari
- 11:03:2010   17:34 : Nicusor Nedelcu - refactored

*************************************************************************/

#include "StdAfx.h"
#include "AssetViewer.h"
#include "ImageExtensionHelper.h"
#include "Include/ITextureDatabaseUpdater.h"
#include "Include/IAssetDisplay.h"
#include "Include/IAssetDisplayDatabase.h"
#include "GameEngine.h"

//--- Constants / defines

//! timer id for interactive rendering update, when interactive rendering is enabled by dragging mouse inside a thumb
#define ID_ASSET_VIEWER_TIMER_INTERACTIVE_RENDER 1
//! timer id for smooth panning with mouse right-click-release dragging
#define ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING 2
//! timer id for cache assets which cannot be cached inside a separate thread
#define ID_ASSET_VIEWER_TIMER_CACHE_ASSET 3
//! timer for warning blinking on asset thumbs, it will toggle a boolean, m_bWarningBlinkToggler, and redraw asset control
#define ID_ASSET_VIEWER_TIMER_WARNING_BLINK_TOGGLER 4
//! timer for when user selects an asset in the list view, and then the viewer must animate, attract user attention to the selected thumbnail
#define ID_ASSET_VIEWER_TIMER_THUMB_FOCUS_ANIMATION 5

//! window control id to render assets in
#define ID_ASSET_VIEWER_THUMB_RENDER_WINDOW 1

//! this is the delay for the timer which caches assets that cannot be cached on a separate thread (ex.: models, characters)
const int kAssetViewer_NonThreadCacheTimerDelay = 100;
//! this is the delay for the timer which pans/scrolls the canvas smoothly
const UINT kAssetViewer_SmoothPanningTimerDelay = USER_TIMER_MINIMUM;
//! this is the delay for the timer which toggles the warning blinker
const UINT kAssetViewer_WarningBlinkerTimerDelay = 500;
//! timer delay used to animate a rectangle to focus a thumb when selected in list view
const UINT kAssetViewer_ThumbFocusAnimationTimerDelay = USER_TIMER_MINIMUM;
//! the maximum number of thumbnails that can be stored at once in the asset thumb queue
const UINT kAssetViewer_MaxThumbsCacheAssetCount = 3000;
//! boosts the thumb smooth panning delta (multiplier), so it can smooth scroll/pan more/less
const float kAssetViewer_SmoothPanningDeltaBoost = 2;
//! interactive render timer delay
const UINT kAssetBrowser_InteractiveRenderTimerDelay = USER_TIMER_MINIMUM;
//! this is the minimum offset on X and Y the mouse can be dragged over an asset selection, before it is considered a rectangle selection operation
const UINT kAssetViewer_MinimumSelectionDraggingOffset = 2;
//! this is the minimum offset on X and Y the mouse can be dragged over on right click and drag,
//! before it starts to pan the canvas, else it will show the asset context menu
const UINT kAssetViewer_MinimumStartPanningOffset = 1;
//! this is the number multiplied by the current smooth panning delta, to decrease it until in reaches near 0
const float kAssetBrowser_SmoothPanningSlowdownMultiplier = 0.9f;
const float kAssetBrowser_SmoothPanningSpeed = 0.01f;

const int kAssetBrowser_ThumbVerticalMargin = 45;
const int kAssetBrowser_ThumbHorizontalMargin = 15;

const int kAssetViewer_TooltipMargin = 20;
const int kAssetViewer_TooltipMinWidth = 200;
const int kAssetViewer_TooltipMinHeight = 80;
const int kAssetViewer_TooltipTitleContentSpacing = 5;
const int kAssetViewer_TooltipBorderWidth = 2;
const int kAssetViewer_TooltipBorderCornerSize = 6;
const COLORREF kAssetViewer_TooltipBorderColor = RGB( 60, 60, 60 );
const COLORREF kAssetViewer_TooltipFilenameShadowColor = RGB( 0, 0, 0 );
const COLORREF kAssetViewer_TooltipFilenameColor = RGB( 255, 255, 0 );
const COLORREF kAssetViewer_SelectionDragLineColor = RGB( 255, 255, 100 );

//---

BEGIN_MESSAGE_MAP(CAssetViewer, CScrollableWindow)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CAssetViewer::CAssetViewer()
{
	m_nItemHorizontalMargin = kAssetBrowser_ThumbHorizontalMargin;
	m_nItemVerticalMargin = kAssetBrowser_ThumbVerticalMargin;
	m_nAssetThumbSize = gSettings.sAssetBrowserSettings.nThumbSize;
	m_bMouseLeftButtonDown = false;
	m_bMouseRightButtonDown = false;
	m_bDragging = false;
	m_oSelectionRect.SetRect( 0, 0, 0, 0 );
	m_oClientRect.SetRect( 0, 0, 0, 0 );
	m_nYOffset = 0;
	m_nTotalDatabaseSize = 0;
	m_nIdealClientWidth = 0;
	m_nIdealClientHeight = 0;
	m_poEnsureVisible = NULL;
	m_piRenderer = gEnv->pRenderer;
	m_bMustRedraw = false;
	
	m_thumbShadowBmp.Load( "Editor/UI/assetBrowserThumbShadow.png", true );
	m_thumbLoadingBmp.Load( "Editor/UI/assetBrowserThumbLoading.png", true );
	m_thumbInvalidAssetBmp.Load( "Editor/UI/assetBrowserThumbInvalid.png", true );
	m_backgroundBmp.Load( "Editor/UI/assetBrowserBack.png", true );

	m_backBrush.CreatePatternBrush( &m_backgroundBmp.GetBitmap() );
	m_fontLabel.CreatePointFont( 100, "Arial" );
	m_fontInfoTitle.CreatePointFont( 95, "Arial Bold" );
	m_fontInfo.CreatePointFont( 80, "Arial" );

	m_poClickedAsset = NULL;
	m_poHoveredAsset = NULL;
	m_smoothPanLastDelta = 0;
	m_smoothPanLeftAmount = 0;
	m_pStatusDisplayObserver = NULL;
	m_nTotalAssets = 0;
	m_pfnDoubleClickCallback = NULL;

	SetAutoScrollWindowFlag( false );
}

void CAssetViewer::OnDestroy()
{
	KillTimer( ID_ASSET_VIEWER_TIMER_INTERACTIVE_RENDER );
	KillTimer( ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING );
}

CAssetViewer::~CAssetViewer()
{
	GetIEditor()->UnregisterNotifyListener( this );
	m_backBrush.DeleteObject();
	m_fontLabel.DeleteObject();
	m_fontInfo.DeleteObject();
	m_fontInfoTitle.DeleteObject();
	m_thumbShadowBmp.Free();
	m_thumbFlagBmp.Free();
	m_thumbLoadingBmp.Free();
	m_thumbInvalidAssetBmp.Free();
	m_backgroundBmp.Free();
	
	if( m_piRenderer )
	{
		m_piRenderer->DeleteContext( m_wndAssetThumbRender.GetSafeHwnd() );
	}

	m_wndAssetThumbRender.DestroyWindow();
}

void CAssetViewer::FreeData()
{
	m_nTotalAssets = 0;
	m_cPendingCacheAssets.resize( 0 );
	m_cAssetDatabases.resize( 0 );
	m_cAssetItems.resize( 0 );
	m_cAssetDrawingCache.resize( 0 );
	m_cThumbsCache.resize( 0 );
	m_nextFieldsInfoCacheAssetIter.resize( 0 );
}

HRESULT STDMETHODCALLTYPE CAssetViewer::QueryInterface( const IID &riid, void **ppvObj ) 
{
	if( riid == __uuidof(IAssetViewer ) /* && m_pIntegrator*/ )
	{
		*ppvObj = this;
		return S_OK;
	}
	return E_NOINTERFACE ; 
}

ULONG STDMETHODCALLTYPE CAssetViewer::AddRef()
{
	return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetViewer::Release() 
{ 
	if( (--m_ref) == 0 )
	{
		delete this;
		return 0; 
	}
	else
		return m_ref;
}

bool CAssetViewer::Create( int nLeft, int nTop, UINT nWidth, UINT nHeight, CWnd* poParentWindow, UINT nId, DWORD dwStyle )
{
	bool bReturn = FALSE != CScrollableWindow::Create( NULL, "AssetViewer", dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
																										 CRect( nLeft, nTop, nWidth, nHeight ), poParentWindow, nId );

	if( !bReturn )
		return false;

	if( !m_wndAssetThumbRender.Create( NULL, "AssetThumbRender", WS_CHILD | WS_EX_TOPMOST | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
																		 CRect( 0, 0, m_nAssetThumbSize, m_nAssetThumbSize ), this, ID_ASSET_VIEWER_THUMB_RENDER_WINDOW ) )
	{
		assert( !"Could not create the asset thumb render window" );
		return false;
	}

	if( m_piRenderer->CreateContext( m_wndAssetThumbRender.GetSafeHwnd() ) )
	{
		m_piRenderer->MakeMainContextActive();
	}
	else
	{
		return false;
	}

	GetIEditor()->RegisterNotifyListener( this );
	SetTimer( ID_ASSET_VIEWER_TIMER_CACHE_ASSET, kAssetViewer_NonThreadCacheTimerDelay, NULL );
	SetTimer( ID_ASSET_VIEWER_TIMER_WARNING_BLINK_TOGGLER, kAssetViewer_WarningBlinkerTimerDelay, NULL );

	return bReturn;
}

void CAssetViewer::LockAll()
{
	m_oAssetDatabasesLock.Lock();
	m_oAssetItemsLock.Lock();
	m_oOperationLock.Lock();
}

void CAssetViewer::UnlockAll()
{
	m_oOperationLock.Unlock();
	m_oAssetItemsLock.Unlock();
	m_oAssetDatabasesLock.Unlock();
}

void CAssetViewer::LockAssetItemsArray()
{
	m_oAssetItemsLock.Lock();
}

void CAssetViewer::UnlockAssetItemsArray()
{
	m_oAssetItemsLock.Unlock();
}

void CAssetViewer::LockAssetDatabasesArray()
{
	m_oAssetDatabasesLock.Lock();
}

void CAssetViewer::UnlockAssetDatabasesArray()
{
	m_oAssetDatabasesLock.Unlock();
}

void CAssetViewer::SetAssetThumbSize( const UINT nThumbSize )
{
	m_nAssetThumbSize = nThumbSize;
	gSettings.sAssetBrowserSettings.nThumbSize = m_nAssetThumbSize;
	m_wndAssetThumbRender.SetWindowPos( 0, 0, 0, m_nAssetThumbSize, m_nAssetThumbSize, SWP_NOMOVE );

	LockAll();

	// uncache all in-view assets, we need new thumbs
	for( size_t i = 0, iCount = m_cAssetDatabases.size(); i < iCount; ++i )
	{
		for( IAssetDisplayDatabase::TFilenameAssetMap::iterator iter = m_cAssetDatabases[i]->GetAssets().begin(),
				iterEnd = m_cAssetDatabases[i]->GetAssets().end();
				iter != iterEnd; ++iter )
		{
			iter->second->UnCacheThumbnail();
		}
	}

	UnlockAll();
	RecalculateLayout();
	UpdateScrollBar();
	UpdateVisibility();
	Invalidate( FALSE );
}

UINT CAssetViewer::GetAssetThumbSize()
{
	return m_nAssetThumbSize;
}

void CAssetViewer::Run()
{	
	// in this thread we only load and prepare new thumbs
	while( IsRunning() && IsStarted() )
	{
		string strFilename;
		CRect stElementRect;
		TAssetItems items;

		LockAssetItemsArray();
		items = m_cAssetDrawingCache;
		UnlockAssetItemsArray();

		for( size_t nCurrentItem = 0, nTotalItems = items.size(); nCurrentItem < nTotalItems; ++nCurrentItem )
		{
			if( !(IsRunning() && IsStarted()) )
			{
				break;
			}

			IAssetDisplay* const cpoDatabaseItem = items[nCurrentItem];

			if(	cpoDatabaseItem->IsFlagSet( IAssetDisplay::eAssetFlags_Cached ) ||
					cpoDatabaseItem->IsFlagSet( IAssetDisplay::eAssetFlags_Invalid ) ||
				 !cpoDatabaseItem->IsFlagSet( IAssetDisplay::eAssetFlags_ThreadCachingSupported ) )
			{
				continue;
			}

			cpoDatabaseItem->Cache();
			m_bMustRedraw = true;
			Sleep( 1 );
		}

		if( m_bOperationInProgress )
		{
			IAssetDisplayDatabase::TFilenameAssetMap::iterator iter;

			// also cache fields info for next asset in each databases
			LockAssetDatabasesArray();

			for( size_t i = 0; i < m_cAssetDatabases.size(); i++ )
			{
				Sleep( 1 );
				iter = m_nextFieldsInfoCacheAssetIter[i];

				if( iter == m_cAssetDatabases[i]->GetAssets().end() )
				{
					continue;
				}

				++m_operationCurrentCount;

				//TODO: check if is not already in m_assetInfoPersistentCache

				if( iter->second->IsFlagSet( IAssetDisplay::eAssetFlags_ThreadFieldsInfoCachingSupported ) )
				{
					iter->second->CacheFieldsInfo();
					//TODO: pack field data into a buffer and put it in m_assetInfoPersistentCache
				}

				++m_nextFieldsInfoCacheAssetIter[i];
			}

			UnlockAssetDatabasesArray();

			m_oOperationLock.Lock();
			
			if( m_operationTotalCount )
			{
				m_operationProgressPercent = ( (float) m_operationCurrentCount / m_operationTotalCount ) * 100;
			}

			m_operationText.Format( "Caching asset fields info (%d of %d)  ", m_operationCurrentCount, m_operationTotalCount );

			if( m_operationProgressPercent >= 100 )
			{
				m_operationText = "";
				m_bOperationInProgress = false;
			}
			else
			{
				m_bOperationInProgress = true;
			}

			m_oOperationLock.Unlock();
		}

		// be friendly with other tasks
		Sleep( 1 );
	}
}

void CAssetViewer::SetDatabases( TAssetDatabases& rcDatabases )
{
	LockAll();
	
	m_cAssetDatabases = rcDatabases;
	m_nextFieldsInfoCacheAssetIter.resize( rcDatabases.size() );

	for( size_t i = 0; i < m_cAssetDatabases.size(); ++i )
	{
		m_cAssetDatabases[i]->SetAssociatedViewer( this );
		m_nextFieldsInfoCacheAssetIter[i] = m_cAssetDatabases[i]->GetAssets().begin();
	}

	UnlockAll();
}

IAssetViewer::TAssetDatabases& CAssetViewer::GetDatabases()
{
	return m_cAssetDatabases;
}

void CAssetViewer::ClearDatabases()
{
	LockAll();

	m_nTotalAssets = 0;
	m_cPendingCacheAssets.resize( 0 );
	m_cAssetDatabases.resize( 0 );
	m_cAssetItems.resize( 0 );
	m_cAssetDrawingCache.resize( 0 );
	m_cThumbsCache.resize( 0 );
	m_nextFieldsInfoCacheAssetIter.resize( 0 );

	UnlockAll();
}

void CAssetViewer::WaitThreadAndFreeData()
{
	Stop();
	WaitForThread();
	FreeData();
}

void CAssetViewer::GetSelectedItems( TAssetItems& rcpoSelectedItemArray )
{
	LockAssetItemsArray();

	for( size_t nCurrentAsset = 0, nTotalAssets = m_cAssetItems.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset )
	{
		IAssetDisplay* const cpoCurrentItem = m_cAssetItems[nCurrentAsset];

		if( cpoCurrentItem->IsFlagSet( IAssetDisplay::eAssetFlags_Selected ) )
		{
			rcpoSelectedItemArray.push_back( cpoCurrentItem );
		}
	}

	UnlockAssetItemsArray();
}

int CAssetViewer::GetFirstSelectedItemIndex()
{
	int index = -1;
	LockAssetItemsArray();

	for( size_t nCurrentAsset = 0, nTotalAssets = m_cAssetItems.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset )
	{
		IAssetDisplay* const cpoCurrentItem = m_cAssetItems[nCurrentAsset];

		if( cpoCurrentItem->IsFlagSet( IAssetDisplay::eAssetFlags_Selected ) )
		{
			index = nCurrentAsset;
			break;
		}
	}

	UnlockAssetItemsArray();

	return index;
}

void CAssetViewer::DeselectAll()
{
	LockAssetItemsArray();

	for( size_t nCurrentAsset = 0, nTotalAssets = m_cAssetItems.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset )
	{
		IAssetDisplay* const cpoCurrentItem = m_cAssetItems[nCurrentAsset];

		cpoCurrentItem->SetFlag( IAssetDisplay::eAssetFlags_Selected, false );
	}

	UnlockAssetItemsArray();
}

void CAssetViewer::RestartThread()
{
	if( IsRunning() )
	{
		Stop();
		WaitForThread();
	}

	Start();
}

void CAssetViewer::SetDoubleClickCallback( TDoubleClickCallback rfnDoubleClickCallback )
{
	m_pfnDoubleClickCallback = rfnDoubleClickCallback;
}

void CAssetViewer::OnEditorNotifyEvent( EEditorNotifyEvent aEvent )
{
	switch( aEvent )
	{
		case eNotify_OnIdleUpdate:
		{
			if( m_bMustRedraw )
			{
				m_bMustRedraw = false;
				Invalidate( FALSE );
			}

			break;
		}
	}
}

void CAssetViewer::OnSize( UINT nType, int cx, int cy )
{
	GetClientRect( &m_oClientRect );

	m_cPendingCacheAssets.resize( 0 );
	
	CDC* pDC = GetDC();
	m_piRenderer->DeleteContext( m_wndAssetThumbRender.GetSafeHwnd() );
	m_piRenderer->CreateContext( m_wndAssetThumbRender.GetSafeHwnd() );
	m_piRenderer->MakeMainContextActive();
	m_canvas.Create( pDC->GetSafeHdc(), cx, cy );
	ReleaseDC( pDC );
	RecalculateLayout();

	int nNewOffset = m_nYOffset;
	int nMaxScroll = m_nIdealClientHeight - m_oClientRect.Height();

	if( nMaxScroll < 0 )
	{
		nNewOffset = 0;
	}

	if( (int)m_nIdealClientHeight - nNewOffset < m_oClientRect.Height() )
	{
		nNewOffset = nMaxScroll;
	}

	if( nNewOffset < 0 )
	{
		nNewOffset = 0;
	}

	m_stDesiredClientSize.SetRect( 0, 0, m_nIdealClientWidth, m_nIdealClientHeight );

	m_nYOffset = nNewOffset;
	UpdateVisibility();
	Invalidate( FALSE );
	__super::OnSize( nType, cx, cy );
}

void CAssetViewer::OnPaint()
{
	CPaintDC dc(this);

	Draw();
}

BOOL CAssetViewer::OnEraseBkgnd( CDC* pDC )
{
	return TRUE;
}

void CAssetViewer::CheckClickedThumb( UINT nFlags, CPoint point, bool bShowInteractiveRenderWnd )
{
	CPoint stHitTestPoint( point );

	stHitTestPoint.y += m_nYOffset;
	m_poClickedAsset = NULL;

	for( size_t nCurrentAsset = 0, nTotalAssets = m_cAssetItems.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset )
	{
		IAssetDisplay* const cpoCurrentItem = m_cAssetItems[nCurrentAsset];

		if( !cpoCurrentItem->IsFlagSet( IAssetDisplay::eAssetFlags_Visible ) )
		{
			continue;
		}
		else
		if( cpoCurrentItem->HitTest( stHitTestPoint.x, stHitTestPoint.y ) )
		{
			m_poClickedAsset = cpoCurrentItem;
			break;
		}
	}

	if( m_poClickedAsset && bShowInteractiveRenderWnd )
	{
		CRect rc;

		m_poClickedAsset->GetDrawingRectangle( rc );
		rc.OffsetRect( 0, -m_nYOffset );

		m_wndAssetThumbRender.SetWindowPos( 0, rc.left, rc.top, rc.Width(), rc.Height(), SWP_FRAMECHANGED );
		m_wndAssetThumbRender.ShowWindow( SW_SHOW );
		SetTimer( ID_ASSET_VIEWER_TIMER_INTERACTIVE_RENDER, kAssetBrowser_InteractiveRenderTimerDelay, NULL );
	}
	else
	{
		m_wndAssetThumbRender.ShowWindow( SW_HIDE );
	}
}

void CAssetViewer::OnLButtonDown( UINT nFlags, CPoint point )
{	
	SetCapture();
	SetFocus();
	m_oSelectionRect.left = point.x;
	m_oSelectionRect.top = point.y;
	m_oSelectionRect.right = point.x;
	m_oSelectionRect.bottom = point.y;
	m_bMouseLeftButtonDown = true;
	m_bDragging = true;
	m_lastPanDragPt = point;
	m_smoothPanLastDelta = 0;
	KillTimer( ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING );
	CheckClickedThumb( nFlags, point, true );
	Invalidate( FALSE );
}

void CAssetViewer::OnMouseMove( UINT nFlags, CPoint point )
{
	if( m_bMouseLeftButtonDown )
	{
		m_oSelectionRect.right = point.x;
		m_oSelectionRect.bottom = point.y;
	}

	if( m_bMouseRightButtonDown )
	{
		m_smoothPanLastDelta = m_lastPanDragPt.y - point.y;
		m_nYOffset += m_smoothPanLastDelta;
		m_smoothPanLastDelta *= kAssetViewer_SmoothPanningDeltaBoost;
		m_lastPanDragPt = point;
		m_poEnsureVisible = NULL;
		UpdateScrollBar();
		UpdateVisibility();
	}

	if( m_bMouseLeftButtonDown )
	{
		if( m_poClickedAsset &&
				m_poClickedAsset->IsFlagSet( IAssetDisplay::eAssetFlags_InteractiveRenderSupported ) )
		{
			CRect rc;
			CPoint pt = point;
			m_wndAssetThumbRender.GetClientRect( &rc );
			ClientToScreen( &pt );
			m_wndAssetThumbRender.ScreenToClient( &pt );
			m_poClickedAsset->InteractiveRender( m_wndAssetThumbRender.GetSafeHwnd(), rc, pt.x, pt.y, point.x - m_lastPanDragPt.x, point.y - m_lastPanDragPt.y, nFlags );
			m_lastPanDragPt = point;
		}
		else
		{
			if( m_wndAssetThumbRender.IsWindowVisible() )
			{
				m_wndAssetThumbRender.ShowWindow( SW_HIDE );
			}
		}
	}

	if( m_smoothPanLastDelta )
	{
		// we must call the smooth panning here too, because when we move the mouse, WM_TIMER messages do not get through, having lower priority
		// and the panning is not smooth, so we call this function in timer and also here, on mouse move
		// the function has builtin timing, does not rely on WM_TIMER tick
		UpdateSmoothPanning();
	}

	CachePendingNonThreadedAssets();
	Invalidate( FALSE );
}

void CAssetViewer::UpdateScrollBar()
{
	int nNewOffset = m_nYOffset;
	int nMaxScroll = m_nIdealClientHeight - m_oClientRect.Height();

	if( nMaxScroll < 0 )
	{
		nNewOffset = 0;
	}

	if( (int)m_nIdealClientHeight - nNewOffset < m_oClientRect.Height() )
	{
		nNewOffset = nMaxScroll;
	}

	if( nNewOffset < 0 )
	{
		nNewOffset = 0;
	}

	m_nYOffset = nNewOffset;
	SetScrollPos( SB_VERT, m_nYOffset );
	SetClientSize( m_nIdealClientWidth, m_nIdealClientHeight );
}

static bool SortAssetsByIndex( IAssetDisplay* pA, IAssetDisplay* pB )
{
	assert( pA && pB );
	return pA->GetIndex() > pB->GetIndex();
}

void CAssetViewer::OnLButtonUp( UINT nFlags, CPoint point )
{
	ReleaseCapture();
	m_bDragging = false;
	LockAssetItemsArray();

	if( m_poClickedAsset )
	{
		CRect rc;

		KillTimer( ID_ASSET_VIEWER_TIMER_INTERACTIVE_RENDER );
		m_wndAssetThumbRender.GetClientRect( &rc );
		m_poClickedAsset->Render( m_wndAssetThumbRender.GetSafeHwnd(), rc, true );
		m_wndAssetThumbRender.ShowWindow( SW_HIDE );
	}

	if( m_bMouseLeftButtonDown )
	{
		m_oSelectionRect.right = point.x;
		m_oSelectionRect.bottom = point.y;

		if( m_oSelectionRect.left > m_oSelectionRect.right )
		{
			std::swap( m_oSelectionRect.left, m_oSelectionRect.right );
		}

		if( m_oSelectionRect.top > m_oSelectionRect.bottom )
		{
			std::swap( m_oSelectionRect.top, m_oSelectionRect.bottom );
		}

		// For adjust the scrolling position to the hit test.
		// For drawing things to the client area we should
		// subtract the scroll position instead.
		CRect stCurrentRect = m_oSelectionRect;

		stCurrentRect.top += m_nYOffset;
		stCurrentRect.bottom += m_nYOffset;
		m_bDragging = false;
		m_bMouseLeftButtonDown = false;
		m_lastPanDragPt = point;

		// For adjust the scrolling position to the hit test.
		// For drawing things to the client area we should
		// subtract the scroll position instead.
		POINT stHitTestPoint( point );
		stHitTestPoint.y += m_nYOffset;

		string		strSelectedFilename;
		string		strDirectory;
		bool			bSelect = false;
		__int64		nTotalSelectionSize = 0;
		bool			bAddSelection = CheckVirtualKey( VK_LCONTROL );
		bool			bUnselect =	CheckVirtualKey( VK_LMENU );
		bool			bSelectedRange =	CheckVirtualKey( VK_LSHIFT ) | CheckVirtualKey( VK_RSHIFT );
		bool			bStartSelection = ( !bAddSelection && !bUnselect );
		bool			bHitItem = false;
		bool			bClickSelection= ( stCurrentRect.Width() <= kAssetViewer_MinimumSelectionDraggingOffset &&
																 stCurrentRect.Height() <= kAssetViewer_MinimumSelectionDraggingOffset );

		// user selected something and then clicked an item + SHIFT, so we select that range
		if( m_poClickedAsset && bSelectedRange && bClickSelection && m_cSelectedAssets.size() )
		{
			IAssetDisplay* pLastSelectedAsset = m_cSelectedAssets.back();

			int direction = pLastSelectedAsset->GetIndex() <= m_poClickedAsset->GetIndex() ? 1 : -1;

			m_cSelectedAssets.resize( 0 );
			
			// unselect all, SHIFT selection is exclusive
			for( size_t nCurrentAsset = 0, nTotalAssets = m_cAssetItems.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset )
			{
				IAssetDisplay* const cpoCurrentItem = m_cAssetItems[nCurrentAsset];

				cpoCurrentItem->SetFlag( IAssetDisplay::eAssetFlags_Selected, false );
			}			

			// get last asset in selection
			for( int iFrom = pLastSelectedAsset->GetIndex() + direction, iTo = m_poClickedAsset->GetIndex(); ( direction > 0 ? iFrom <= iTo : iFrom >= iTo ) ; iFrom += direction )
			{
				IAssetDisplay* const cpoCurrentItem = m_cAssetItems[iFrom];

				if( !cpoCurrentItem->IsFlagSet( IAssetDisplay::eAssetFlags_Visible ) )
				{
					continue;
				}

				cpoCurrentItem->SetFlag( IAssetDisplay::eAssetFlags_Selected, true );
				m_cSelectedAssets.push_back( cpoCurrentItem );
			}

			pLastSelectedAsset->SetFlag( IAssetDisplay::eAssetFlags_Selected, true );
			m_cSelectedAssets.push_back( pLastSelectedAsset );
		}
		else
		{
			m_cSelectedAssets.resize( 0 );

			for( size_t nCurrentAsset = 0, nTotalAssets = m_cAssetItems.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset )
			{
				IAssetDisplay* const cpoCurrentItem = m_cAssetItems[nCurrentAsset];

				if( !cpoCurrentItem->IsFlagSet( IAssetDisplay::eAssetFlags_Visible ) )
				{
					continue;
				}

				if( bClickSelection )
				{
					bHitItem = cpoCurrentItem->HitTest( stHitTestPoint.x, stHitTestPoint.y );
				}
				else
				{
					bHitItem = cpoCurrentItem->HitTest( stCurrentRect );
				}

				if( bHitItem )
				{
					// When you're clicking over an unselected item, even in unselect mode you will
					// want to add it to selection.
					if( bClickSelection )
					{
						// When you're clicking over an unselected item, even in unselect mode you will
						// want to add it to selection. (actually this inverses selection behavior)
						bSelect = !cpoCurrentItem->IsFlagSet( IAssetDisplay::eAssetFlags_Selected );
						cpoCurrentItem->SetFlag( IAssetDisplay::eAssetFlags_Selected, bSelect );

						if( bSelect )
						{
							m_cSelectedAssets.push_back( cpoCurrentItem );
						}
					}
					else
					{
						// We have to add those which hit the selection unless we are unselecting items
						// so if unselect is false, it will select items, otherwise it will unselect them
						bSelect = !bUnselect;
						cpoCurrentItem->SetFlag( IAssetDisplay::eAssetFlags_Selected, bSelect );

						if( bSelect )
						{
							m_cSelectedAssets.push_back( cpoCurrentItem );
						}					
					}
				}
				else
				{
					// If we are starting a new selection, the item wasn't hit in the selection, 
					// we must unselect it.
					if( bStartSelection )
					{
						cpoCurrentItem->SetFlag( IAssetDisplay::eAssetFlags_Selected, false );
					}
					else
					{
						if( cpoCurrentItem->IsFlagSet( IAssetDisplay::eAssetFlags_Selected ) )
						{
							m_cSelectedAssets.push_back( cpoCurrentItem );
						}
					}
				}
			}
		}

		std::sort( m_cSelectedAssets.begin(), m_cSelectedAssets.end(), SortAssetsByIndex );
	}
	
	UnlockAssetItemsArray();
	Invalidate( FALSE );

	if( m_pStatusDisplayObserver )
	{
		m_pStatusDisplayObserver->OnChangeStatusBarInfo( m_cSelectedAssets.size(), m_cAssetItems.size(), m_nTotalAssets );
		m_pStatusDisplayObserver->OnSelectionChanged();
	}
}

void CAssetViewer::OnLButtonDblClk( UINT nFlags, CPoint point )
{
	if( m_pfnDoubleClickCallback )
	{
		LockAssetItemsArray();

		// We only must call the callback if we have one Asset under the mouse...
		// and if we did, at least one was selected.
		POINT							stHitTestPoint = point;
		bool							bAnySelected = false;
		string						strDirectory;
		string						strSelectedFilename;
		unsigned __int64	nTotalSelectionSize = 0;

		stHitTestPoint.y += m_nYOffset;

		for( size_t nCurrentAsset = 0, nTotalAssets = m_cAssetItems.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset )
		{
			IAssetDisplay* const cpoCurrentItem = m_cAssetItems[nCurrentAsset];

			if( cpoCurrentItem->HitTest( stHitTestPoint.x, stHitTestPoint.y ) )
			{
				// We can't simply break here because we must also make sure all other items are unselected.
				cpoCurrentItem->SetFlag( IAssetDisplay::eAssetFlags_Selected, true );
				int nCurrentFileSize = 0;

				cpoCurrentItem->GetAssetFieldValue( "filesize", &nCurrentFileSize );
				nTotalSelectionSize += nCurrentFileSize;
				bAnySelected = true;
			}
			else
			{
				cpoCurrentItem->SetFlag( IAssetDisplay::eAssetFlags_Selected, false );
			}
		}	

		if( m_pStatusDisplayObserver )
		{
			m_pStatusDisplayObserver->OnChangeStatusBarInfo( m_cSelectedAssets.size(), m_cAssetItems.size(), m_nTotalAssets );
			m_pStatusDisplayObserver->OnSelectionChanged();
		}

		UnlockAssetItemsArray();

		if( bAnySelected )
		{
			m_pfnDoubleClickCallback();
		}
	}

	Invalidate( FALSE );
}

void CAssetViewer::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_bMouseRightButtonDown = true;
	m_lastPanDragPt = m_lastRightClickPt = point;
	m_smoothPanLeftAmount = 0;
	m_smoothPanLastDelta = 0;
	CheckClickedThumb( nFlags, point );
	SetCapture();
	SetFocus();
	__super::OnRButtonDown(nFlags, point);
}

void CAssetViewer::OnRButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_bMouseRightButtonDown = false;
	m_smoothPanLeftAmount = m_smoothPanLastDelta;

	LockAssetItemsArray();
	
	// if we clicked an asset and we didnt moved the mouse (too much), then show the context menu
	if( m_poClickedAsset && 
			( m_lastRightClickPt.x - point.x ) <= kAssetViewer_MinimumStartPanningOffset &&
			( m_lastRightClickPt.y - point.y ) <= kAssetViewer_MinimumStartPanningOffset )
	{
		int SAVE_REPORT, SHOW_RGBA, SHOW_ALPHA, SHOW_RGB;
		SAVE_REPORT = SHOW_RGBA = SHOW_ALPHA = SHOW_RGB = -2;
		CFileUtil::ExtraMenuItems extraMenuItems;
		SAVE_REPORT = extraMenuItems.AddItem( "Save Report" );
		if( m_poClickedAsset->IsFlagSet( IAssetDisplay::eAssetFlags_AlphaSupported ) )
		{
			extraMenuItems.AddItem( "" );								// Separator menu item
			SHOW_RGBA = extraMenuItems.AddItem( "Show RGBA" );
			SHOW_ALPHA = extraMenuItems.AddItem( "Show Alpha Only" );
			SHOW_RGB = extraMenuItems.AddItem( "Show RGB Only" );
		}
		CFileUtil::PopupMenu( m_poClickedAsset->GetFilename(), m_poClickedAsset->GetRelativePath(), this ,
												0, &extraMenuItems );
		if( extraMenuItems.selectedIndexIfAny == SAVE_REPORT )
		{
			CString filePath = m_poClickedAsset->GetFilename();
			if ( CFileUtil::SelectSaveFile( "Report Files (*.txt, *.bmp)|*.txt;*.bmp|All files|*.*||",
																			"txt", "", filePath ) )
			{
				CString filePathTXT = Path::ReplaceExtension(filePath, "txt");
				m_poClickedAsset->SaveReportText( filePathTXT.GetBuffer() );
				CString filePathBMP = Path::ReplaceExtension(filePath, "bmp");
				m_poClickedAsset->SaveReportImage( filePathBMP.GetBuffer() );
			}
		}
		else if( extraMenuItems.selectedIndexIfAny == SHOW_RGBA ) 
			m_poClickedAsset->SetDrawingOption(IAssetDisplay::eAssetDrawing_RGBA);
		else if( extraMenuItems.selectedIndexIfAny == SHOW_ALPHA )
			m_poClickedAsset->SetDrawingOption(IAssetDisplay::eAssetDrawing_Alpha);
		else if( extraMenuItems.selectedIndexIfAny == SHOW_RGB )
			m_poClickedAsset->SetDrawingOption(IAssetDisplay::eAssetDrawing_RGB);
	}

	UnlockAssetItemsArray();

	// if we have smooth panning movement, start the smooth panning timer operation
	if( fabs( m_smoothPanLeftAmount ) )
	{
		m_smoothPanningLastTickCount = GetTickCount();
		SetTimer( ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING, kAssetViewer_SmoothPanningTimerDelay, NULL );
	}

	SetFocus();

	__super::OnRButtonUp(nFlags, point);
}

void CAssetViewer::OnMButtonDown(UINT nFlags, CPoint point)
{
	m_bMouseMiddleButtonDown = true;
	m_lastPanDragPt = point;
	m_smoothPanLeftAmount = 0;
	m_smoothPanLastDelta = 0;
	SetCapture();
	SetFocus();
	CheckClickedThumb( nFlags, point );

	__super::OnMButtonDown(nFlags, point);
}

void CAssetViewer::OnMButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_bMouseMiddleButtonDown = false;
	LockAssetItemsArray();

	if( m_poClickedAsset )
	{
		CRect rc;

		KillTimer( ID_ASSET_VIEWER_TIMER_INTERACTIVE_RENDER );
		m_wndAssetThumbRender.GetClientRect( &rc );
		m_poClickedAsset->Render( m_wndAssetThumbRender.GetSafeHwnd(), rc, true );
		m_poClickedAsset = NULL;
		m_wndAssetThumbRender.ShowWindow( SW_HIDE );
	}

	UnlockAssetItemsArray();

	__super::OnMButtonUp(nFlags, point);
}

void CAssetViewer::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	__super::OnVScroll( nSBCode, nPos, pScrollBar );
	m_poEnsureVisible = NULL;
	m_nYOffset = GetScrollPos( SB_VERT );

	m_smoothPanLeftAmount = 0;
	m_smoothPanLastDelta = 0;
	KillTimer( ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING );
	KillTimer( ID_ASSET_VIEWER_TIMER_INTERACTIVE_RENDER );

	UpdateScrollBar();
	UpdateVisibility();
	Invalidate( FALSE );
}

BOOL CAssetViewer::OnMouseWheel( UINT nFlags, short zDelta, CPoint pt )
{
	m_nYOffset -= zDelta;
	m_wndAssetThumbRender.ShowWindow( SW_HIDE );
	m_poEnsureVisible = NULL;
	UpdateScrollBar();
	UpdateVisibility();
	Invalidate( FALSE );

	return TRUE;
}

void CAssetViewer::OnTimer( UINT_PTR nIDEvent )
{
	if( nIDEvent == ID_ASSET_VIEWER_TIMER_INTERACTIVE_RENDER && m_poClickedAsset )
	{
		CRect rc;
		
		m_wndAssetThumbRender.GetClientRect( &rc );
		m_poClickedAsset->InteractiveRender( m_wndAssetThumbRender.GetSafeHwnd(), rc, 0, 0, 0, 0, 0 );
	}
	else
	if( nIDEvent == ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING )
	{
		UpdateSmoothPanning();
		Invalidate( FALSE );
	}
	else
	if( nIDEvent == ID_ASSET_VIEWER_TIMER_CACHE_ASSET )
	{
		CachePendingNonThreadedAssets();
	}
	else
	if( nIDEvent == ID_ASSET_VIEWER_TIMER_WARNING_BLINK_TOGGLER )
	{
		m_bWarningBlinkToggler = !m_bWarningBlinkToggler;
		Invalidate( FALSE );
	}
	else
	if( nIDEvent == ID_ASSET_VIEWER_TIMER_THUMB_FOCUS_ANIMATION )
	{
		if( !m_poEnsureVisible )
		{
			m_thumbFocusAnimationTime = 1;
			KillTimer( ID_ASSET_VIEWER_TIMER_THUMB_FOCUS_ANIMATION );
			Invalidate( FALSE );
			return;
		}

		CRect rc;
		
		m_poEnsureVisible->GetDrawingRectangle( rc );
		rc.OffsetRect( 0, -m_nYOffset );

		m_thumbFocusAnimationRect.left = m_oClientRect.left + m_thumbFocusAnimationTime * ( rc.left - m_oClientRect.left );
		m_thumbFocusAnimationRect.right = m_oClientRect.right + m_thumbFocusAnimationTime * ( rc.right - m_oClientRect.right );
		m_thumbFocusAnimationRect.top = m_oClientRect.top + m_thumbFocusAnimationTime * ( rc.top - m_oClientRect.top );
		m_thumbFocusAnimationRect.bottom = m_oClientRect.bottom + m_thumbFocusAnimationTime * ( rc.bottom - m_oClientRect.bottom );
		m_thumbFocusAnimationTime += 0.1f;

		Invalidate( FALSE );
		
		if( m_thumbFocusAnimationTime >= 1 )
		{
			m_thumbFocusAnimationTime = 1;
			KillTimer( ID_ASSET_VIEWER_TIMER_THUMB_FOCUS_ANIMATION );
		}
	}
}

void CAssetViewer::CachePendingNonThreadedAssets()
{
	// CryPak and other classes are showing non full app modal dialog boxes, and the WM_TIMER is still sent, so check if we're not already inside
	static volatile bool bWorking = false;

	if( bWorking )
		return;

	bWorking = true;

	if( m_cPendingCacheAssets.empty() )
	{
		KillTimer( ID_ASSET_VIEWER_TIMER_CACHE_ASSET );
		bWorking = false;
		return;
	}

	IAssetDisplay* pAsset = m_cPendingCacheAssets[0];

	if( pAsset )
	{
		// no need to cache it if not visible anymore
		if( m_cAssetDrawingCache.end() == std::find( m_cAssetDrawingCache.begin(), m_cAssetDrawingCache.end(), pAsset ) )
		{
			m_cPendingCacheAssets.erase( m_cPendingCacheAssets.begin() );
			bWorking = false;
			return;
		}

		if( !pAsset->IsFlagSet( IAssetDisplay::eAssetFlags_Cached ) )
		{
			pAsset->Cache();
			m_cPendingCacheAssets.erase( m_cPendingCacheAssets.begin() );
			Invalidate( FALSE );
		}
	}

	bWorking = false;
}

void CAssetViewer::UpdateSmoothPanning()
{
	if( 0 == m_smoothPanLeftAmount )
	{
		return;
	}

	UINT crtTick = GetTickCount();
	UINT delta = crtTick - m_smoothPanningLastTickCount;
	m_smoothPanningLastTickCount = crtTick;
	float fDeltaTime = (float)delta / 1000.0f;
	static float sfTimerAmount = 0;
	sfTimerAmount += fDeltaTime;
	
	if( sfTimerAmount < kAssetBrowser_SmoothPanningSpeed )
	{
		return;
	}

	sfTimerAmount = 0;
	m_smoothPanLeftAmount *= kAssetBrowser_SmoothPanningSlowdownMultiplier;
	m_nYOffset += m_smoothPanLeftAmount;

	// if smaller than 1 pixel, dont bother, end smooth panning operation
	if( fabs( m_smoothPanLeftAmount ) < 1.0f )
	{
		m_smoothPanLeftAmount = 0;
		m_smoothPanLastDelta = 0;
		KillTimer( ID_ASSET_VIEWER_TIMER_SMOOTH_PANNING );
	}

	m_poEnsureVisible = NULL;
	UpdateScrollBar();
	UpdateVisibility();
}

void CAssetViewer::RecalculateLayout()
{
	int		nCurrentTop = 0, nCurrentLeft = 0;
	UINT	thumbsPerRow = 0;
	float	horizMargin = 0;
	UINT	rowItemCount = 0;
	CRect	rstElementRect;

	// magic! compute number of thumbs per one row of asset thumbs, and the needed margin between them
	CGdiCanvas::ComputeThumbsLayoutInfo( m_oClientRect.Width(), m_nAssetThumbSize, m_nItemHorizontalMargin, m_cAssetItems.size(), thumbsPerRow, horizMargin );

	nCurrentTop = m_nItemVerticalMargin;
	nCurrentLeft = horizMargin;

	for( size_t nCurrentItem = 0, nTotalItems = m_cAssetItems.size(); nCurrentItem < nTotalItems; ++nCurrentItem )
	{
		IAssetDisplay* const cpoCurrentAssetItem = m_cAssetItems[nCurrentItem];

		// Invisible items, due to filtering or any other reason
		// are not considered in the layout calculation...
		if( !cpoCurrentAssetItem->IsFlagSet( IAssetDisplay::eAssetFlags_Visible ) )
		{
			continue;
		}

		rstElementRect.left = nCurrentLeft;
		rstElementRect.right = rstElementRect.left + m_nAssetThumbSize;
		rstElementRect.top = nCurrentTop;
		rstElementRect.bottom = rstElementRect.top + m_nAssetThumbSize;

		cpoCurrentAssetItem->SetDrawingRectangle( rstElementRect );

		nCurrentLeft += m_nAssetThumbSize + horizMargin * 2;
		rowItemCount++;

		// begin a new row of thumbs
		if( rowItemCount >= thumbsPerRow )
		{
			rowItemCount = 0;
			nCurrentLeft = horizMargin;
			nCurrentTop += m_nItemVerticalMargin * 2 + m_nAssetThumbSize;
		}
	}

	m_nIdealClientHeight = nCurrentTop + m_nAssetThumbSize + 2*m_nItemVerticalMargin;

	if( m_nIdealClientHeight > m_oClientRect.Height() )
	{
		m_nIdealClientWidth = m_oClientRect.Width() - GetSystemMetrics( SM_CXVSCROLL );
	}
	else
	{
		m_nIdealClientWidth = m_oClientRect.Width();
	}
}

void CAssetViewer::UpdateVisibility()
{
	CRect oCurrentRect;
	CRect oIntersection;

	// First we check all bitmaps which were previously visible and which are not anymore
	// and then we uncache them. The ones still visible will keep cached.
	for( TAssetItems::iterator iter = m_cAssetItems.begin(), iterEnd = m_cAssetItems.end(); iter != iterEnd; ++iter )
	{
		IAssetDisplay* const cpoDatabaseItem = *iter;

		// No need to cache bitmaps that should not be displayed for one reason or another...
		// specially because we have no guarantee that their positions will be valid...
		// so it is a cached item, we must remove it from the cache.
		if( !cpoDatabaseItem->IsFlagSet( IAssetDisplay::eAssetFlags_Visible ) )
		{
			cpoDatabaseItem->UnCache();
			continue;
		}

		cpoDatabaseItem->GetDrawingRectangle( oCurrentRect );
		oCurrentRect.top -= m_nYOffset;
		oCurrentRect.bottom -= m_nYOffset;

		// We don't need to uncache Assets which are still visible...
		if( oIntersection.IntersectRect( &oCurrentRect, &m_oClientRect ) )
		{
			continue;
		}

		cpoDatabaseItem->UnCache();
	}

	LockAssetItemsArray();
	m_cAssetDrawingCache.resize( 0 );
	UnlockAssetItemsArray();
	
	for( size_t nCurrentItem = 0, nTotalItems = m_cAssetItems.size(); nCurrentItem < nTotalItems; ++nCurrentItem )
	{
		IAssetDisplay* const cpoDatabaseItem = m_cAssetItems[nCurrentItem];

		// No need to cache bitmaps that should not be displayed for one reason or another...
		// specially because we have no guarantee that their positions will be valid...
		if( !cpoDatabaseItem->IsFlagSet( IAssetDisplay::eAssetFlags_Visible ) )
		{
			continue;
		}

		cpoDatabaseItem->GetDrawingRectangle( oCurrentRect );

		// We may need to have 2 copies of the rectangles, one the drawing and one the
		// layout rectangle...
		oCurrentRect.top -= m_nYOffset;
		oCurrentRect.bottom -= m_nYOffset;

		// If the item is entirely outside of the client rect, no need to cache a bitmap for it.
		if( oCurrentRect.bottom < 0 )
		{
			continue;
		}

		// If the item is entirely below the client rect, as Y coordinates grow monotonically,
		// no need to cache anything else.
		if( oCurrentRect.top > m_oClientRect.bottom )
		{
			break;
		}

		// Added to use the same criteria as in the remove list.
		if( !oIntersection.IntersectRect( m_oClientRect, oCurrentRect ) )
		{
			continue;
		}

		LockAssetItemsArray();
		m_cAssetDrawingCache.push_back( cpoDatabaseItem );
		UnlockAssetItemsArray();
	}
}

void CAssetViewer::Draw()
{
	string		strFilename;
	CRect			stElementRect;
	CRect			stIntersection;
	CDC&			dc = m_canvas.GetDC();

	dc.SetBrushOrg( 0, -m_nYOffset );
	dc.FillRect( &m_oClientRect, &m_backBrush );
	dc.SetBrushOrg( 0, 0 );

	for( size_t nCurrentAsset = 0, nTotalAssets = m_cAssetDrawingCache.size(); nCurrentAsset < nTotalAssets; ++nCurrentAsset )
	{
		IAssetDisplay* const cpoAssetDatabaseItem = m_cAssetDrawingCache[nCurrentAsset];

		cpoAssetDatabaseItem->GetDrawingRectangle( stElementRect );
		stElementRect.top -= m_nYOffset;
		stElementRect.bottom -= m_nYOffset;

		// If the rectangle is outside, skip it
		if( !stIntersection.IntersectRect( &stElementRect, &m_oClientRect ) )
		{
			continue;
		}

		DrawAsset( cpoAssetDatabaseItem, stElementRect );
	}

	if( m_bDragging && !m_poClickedAsset )
	{
		CPen pen;
		pen.CreatePen( PS_SOLID, 2, kAssetViewer_SelectionDragLineColor );
		dc.SelectObject( pen );
		dc.SelectObject( GetStockObject( NULL_BRUSH ) );
		dc.Rectangle( m_oSelectionRect );
		pen.DeleteObject();
	}

	// check hovered asset
	m_poHoveredAsset = NULL;
	CPoint point;

	GetCursorPos( &point );
	ScreenToClient( &point );

	for( size_t i = 0, iCount = m_cAssetDrawingCache.size(); i < iCount; ++i )
	{
		if( !m_cAssetDrawingCache[i]->IsFlagSet( IAssetDisplay::eAssetFlags_Visible ) )
		{
			continue;
		}

		if( m_cAssetDrawingCache[i]->HitTest( point.x, point.y + m_nYOffset ) )
		{
			m_poHoveredAsset = m_cAssetDrawingCache[i];
			break;
		}
	}

	if( m_poHoveredAsset && !m_bMouseLeftButtonDown )
	{
		CPen		penTip;
		CBrush	brushTip;
		CRect		rcTip, rcTitle, rcInfo;
		string	strFilename, strAssetThumbInfo;
		string	strAssetThumbInfoFinal;
		CSize		titleSize;

		m_poHoveredAsset->GetAssetFieldValue( "filename", &strFilename );
		m_poHoveredAsset->GetAssetFieldValue( "thumbTipInfo", &strAssetThumbInfo );

		rcTip.left = point.x;
		rcTip.top = point.y + kAssetViewer_TooltipMargin;
		rcTip.right = rcTip.left + kAssetViewer_TooltipMinWidth;
		rcTip.bottom = rcTip.top + kAssetViewer_TooltipMinHeight;

		dc.SelectObject( m_fontInfoTitle );
		titleSize = dc.GetTextExtent( strFilename.c_str() );

		// choose the larger one
		rcTip.right = rcTip.left + MAX( kAssetViewer_TooltipMinWidth, titleSize.cx ) + kAssetViewer_TooltipMargin;
		rcInfo = rcTip;

		// compute text bounds, no draw
		dc.SelectObject( m_fontInfo );
		strAssetThumbInfoFinal = strAssetThumbInfo;
		dc.DrawText( strAssetThumbInfoFinal.c_str(), &rcInfo, DT_CALCRECT|DT_WORDBREAK );

		const int kSomeSoe = 2;

		rcTip.bottom = rcTip.top + titleSize.cy + kAssetViewer_TooltipMargin + rcInfo.Height() + kAssetViewer_TooltipTitleContentSpacing;
		rcTip.right = max( rcTip.left + rcInfo.Width() + kAssetViewer_TooltipMargin, rcTip.right );

		//
		// relocate tip box, if out of bounds
		//

		if( rcTip.right > m_oClientRect.Width() )
		{
			int delta = rcTip.right - m_oClientRect.Width();
			rcTip.left -= delta;
			rcTip.right -= delta;
		}

		if( rcTip.bottom > m_oClientRect.Height() )
		{
			int delta = rcTip.bottom - m_oClientRect.Height();
			rcTip.top -= delta;
			rcTip.bottom -= delta;
		}

		//
		// draw tip box shadow
		//
		CRect rcShadow = rcTip;
		rcShadow.InflateRect( 12, 5 );
		m_canvas.BitBltWithAlpha( m_thumbShadowBmp.GetDC(), rcShadow.left, rcShadow.top, rcShadow.Width(), rcShadow.Height() );

		//
		// draw tip box round rectangle
		//
		penTip.CreatePen( PS_SOLID, kAssetViewer_TooltipBorderWidth, kAssetViewer_TooltipBorderColor );
		dc.SelectObject( penTip );
		dc.SelectObject( GetStockObject( HOLLOW_BRUSH ) );
		dc.RoundRect( &rcTip, CPoint( kAssetViewer_TooltipBorderCornerSize, kAssetViewer_TooltipBorderCornerSize ) );

		//
		// draw small info icon
		//
		HICON hIconInfo = LoadIcon( NULL, IDI_INFORMATION );
		DrawIconEx( dc.GetSafeHdc(), rcTip.right - 32,  rcTip.bottom - 32, hIconInfo, 16, 16, 0, 0, DI_NORMAL );

		//
		// draw filename
		//
		dc.SelectObject( m_fontInfoTitle );
		dc.SetTextColor( kAssetViewer_TooltipFilenameShadowColor );
		dc.TextOut( rcTip.left + 12, rcTip.top + 12, strFilename.c_str() );
		dc.SetTextColor( kAssetViewer_TooltipFilenameColor );
		dc.TextOut( rcTip.left + 10, rcTip.top + 10, strFilename.c_str() );

		//
		// draw info text lines
		//
		CRect rcTextInfo = rcTip;
		rcTextInfo.left += 10;
		rcTextInfo.right -= 10;
		rcTextInfo.top = rcTip.top + titleSize.cy + 15;
		rcTextInfo.bottom -= 10;
		dc.SetTextColor( RGB( 0, 255, 255 ) );
		dc.SelectObject( m_fontInfo );
		dc.DrawText( strAssetThumbInfoFinal.c_str(), &rcTextInfo, DT_WORDBREAK );

		penTip.DeleteObject();
	}

	//
	// draw thumb focus animation rect
	//
	if( m_thumbFocusAnimationTime != 1 )
	{
		CPen pen;
		pen.CreatePen( PS_SOLID, 2, RGB( 255, 255, 255 ) );
		dc.SelectObject( GetStockObject( HOLLOW_BRUSH ) );
		dc.SelectObject( pen );
		dc.Rectangle( &m_thumbFocusAnimationRect );
		pen.DeleteObject();
	}

	m_canvas.BlitTo( m_hWnd );
}

void CAssetViewer::DrawAsset( IAssetDisplay* poItem, CRect& roUpdateRect )
{
	string		strOutputText;
	bool			bSelected = poItem->IsFlagSet( IAssetDisplay::eAssetFlags_Selected ), bHovered = ( poItem == m_poHoveredAsset );
	string		strTempText, strAssetThumbOneLineInfo;
	string		strLabel, strInfo;
	CString		strTmp;

	CDC& dc = m_canvas.GetDC();
	CRect rcShadow = roUpdateRect;
	CRect rcBorder = roUpdateRect;
	CRect rcLabel = roUpdateRect;

	rcBorder.InflateRect( 5, 5 );
	rcShadow.InflateRect( 15, 15 );

	poItem->GetAssetFieldValue( "filename", &strLabel );
	poItem->GetAssetFieldValue( "thumbOneLineInfo", &strAssetThumbOneLineInfo );
	strOutputText = strLabel;

	dc.SelectObject( m_fontLabel );

	rcLabel.top = rcLabel.bottom + 10;
	rcLabel.bottom = rcLabel.top + m_nItemVerticalMargin;
	m_canvas.BreakTextByChars( strOutputText.c_str(), strLabel, rcLabel );
	// no text draw, just compute rect
	dc.DrawText( strLabel.c_str(), &rcLabel, DT_CENTER | DT_CALCRECT );

	//
	// draw the label box
	//
	CBrush brushLabelBox;
	CPen penLabelBox;
	CRect rcLabelBox = rcBorder;

	brushLabelBox.CreateSolidBrush( bSelected ? RGB( 20, 116, 173 ) : RGB( 20, 20, 20 ) );
	penLabelBox.CreatePen( PS_SOLID, 1, bSelected ? RGB( 10, 106, 163 ) : RGB( 0, 0, 0 ) );
	dc.SelectObject( brushLabelBox );
	dc.SelectObject( penLabelBox );
	rcLabelBox.top = rcBorder.bottom - 5;
	rcLabelBox.bottom = rcLabelBox.top + rcLabel.Height() + 15;
	dc.RoundRect( &rcLabelBox, CPoint( 5, 5 ) );

	//
	// draw shadow
	//
	m_canvas.BitBltWithAlpha( m_thumbShadowBmp.GetDC(), rcShadow.left, rcShadow.top, rcShadow.Width(), rcShadow.Height() );

	//
	// draw the label
	//
	rcLabel.left = roUpdateRect.left;
	rcLabel.right = roUpdateRect.right;
	dc.SetTextColor( RGB( 255, 255, 255 ) );
	dc.DrawText( strLabel.c_str(), &rcLabel, DT_CENTER );

	//
	// draw one line info text for thumb
	//
	rcLabel.top = rcLabel.bottom + 5;
	rcLabel.bottom = rcLabel.top + 15;
	dc.SetTextColor( RGB( 255, 155, 55 ) );
	dc.SelectObject( m_fontInfo );
	dc.DrawText( strAssetThumbOneLineInfo.c_str(), &rcLabel, DT_CENTER );

	//
	// draw the thumb container
	//
	CPen penContainer;
	CBrush brushContainer;
	
	penContainer.CreatePen( PS_SOLID, 1, bHovered ? RGB( 255, 255, 255 ) : RGB( 50, 50, 50 ) );
	brushContainer.CreateSolidBrush( poItem->IsFlagSet( IAssetDisplay::eAssetFlags_HasErrors ) && m_bWarningBlinkToggler ? RGB( 255, 0, 0 ) : RGB( 0, 0, 0 ) );
	dc.SelectObject( brushContainer );
	dc.SelectObject( penContainer );
	
	if( bSelected )
	{
		rcBorder.InflateRect( 4, 4 );
	}

	dc.RoundRect( &rcBorder, CPoint( 5, 5 ) );
	penContainer.DeleteObject();

	//
	// draw selection rect
	//
	if( poItem->IsFlagSet( IAssetDisplay::eAssetFlags_Selected ) )
	{
		CRect stSelectionRect( roUpdateRect );

		stSelectionRect.InflateRect( 5, 5 );
		stSelectionRect.left++;
		stSelectionRect.top++;

		CPen pen( PS_SOLID, 2, RGB( 255, 255, 0 ) );
		dc.SelectObject( pen );
		dc.SelectObject( GetStockObject( HOLLOW_BRUSH ) );
		dc.RoundRect( &stSelectionRect, CPoint( 1, 1 ) );
		pen.DeleteObject();
	}

	// cache items that were not cached in the caching thread, due to no eAssetFlags_ThreadCachingSupported flag
	if( !poItem->IsFlagSet( IAssetDisplay::eAssetFlags_Cached ) &&
		  !poItem->IsFlagSet( IAssetDisplay::eAssetFlags_ThreadCachingSupported ) )
	{
		// evict pending assets not seen anymore, no need to cache them
		TAssetItems::iterator iter = m_cPendingCacheAssets.begin();
		
		while( iter != m_cPendingCacheAssets.end() )
		{
			// if not anymore in drawing list, then take it out of pending cache list
			if( m_cAssetDrawingCache.end() == std::find( m_cAssetDrawingCache.begin(), m_cAssetDrawingCache.end(), *iter ) )
			{
				iter = m_cPendingCacheAssets.erase( iter );
				continue;
			}

			++iter;
		}
		
		// if not already in the pending list, then enqueue it
		if( m_cPendingCacheAssets.end() == std::find( m_cPendingCacheAssets.begin(), m_cPendingCacheAssets.end(), poItem ) )
		{
			m_cPendingCacheAssets.push_back( poItem );
			SetTimer( ID_ASSET_VIEWER_TIMER_CACHE_ASSET, kAssetViewer_NonThreadCacheTimerDelay, NULL );
		}
	}

	// this asset needs to cache its thumbnail
	if( poItem->IsFlagSet( IAssetDisplay::eAssetFlags_Cached ) )
	{
		if( !poItem->IsFlagSet( IAssetDisplay::eAssetFlags_ThumbnailCached ) &&
				!poItem->IsFlagSet( IAssetDisplay::eAssetFlags_ThreadCachingSupported ) )
		{
				CRect rc;
				m_wndAssetThumbRender.GetClientRect( &rc );
				m_wndAssetThumbRender.SetWindowPos( 0, 0, 0, m_nAssetThumbSize, m_nAssetThumbSize, 0 );
				poItem->Render( m_wndAssetThumbRender.GetSafeHwnd(), &rc, true /*cache thumbnail*/ );
		}
	}

	// if we have a thumbnail ready, draw it
	if( poItem->IsFlagSet( IAssetDisplay::eAssetFlags_ThumbnailCached ) )
	{
		if( !poItem->DrawThumbImage( dc.GetSafeHdc(), roUpdateRect ) )
		{
			m_canvas.GetDC().StretchBlt( roUpdateRect.left, roUpdateRect.top, roUpdateRect.Width(), roUpdateRect.Height(), &m_thumbLoadingBmp.GetDC(), 0, 0, m_thumbLoadingBmp.GetWidth(), m_thumbLoadingBmp.GetHeight(), SRCCOPY );
		}
	}
	else
	{
		if( poItem->IsFlagSet( IAssetDisplay::eAssetFlags_Invalid ) )
		{
			// draw a INVALID ASSET placeholder
			m_canvas.GetDC().StretchBlt( roUpdateRect.left, roUpdateRect.top, roUpdateRect.Width(), roUpdateRect.Height(), &m_thumbInvalidAssetBmp.GetDC(), 0, 0, m_thumbInvalidAssetBmp.GetWidth(), m_thumbInvalidAssetBmp.GetHeight(), SRCCOPY );
		}	
		else
		{
			// draw a LOADING ASSET placeholder
			m_canvas.GetDC().StretchBlt( roUpdateRect.left, roUpdateRect.top, roUpdateRect.Width(), roUpdateRect.Height(), &m_thumbLoadingBmp.GetDC(), 0, 0, m_thumbLoadingBmp.GetWidth(), m_thumbLoadingBmp.GetHeight(), SRCCOPY );
		}
	}
}

bool CAssetViewer::CheckVirtualKey( int virtualKey )
{
	//TODO: ?????
	if( GetAsyncKeyState( virtualKey ) & ( 1 << 15 ) )
	{
		return true;
	}

	return false;
}

void CAssetViewer::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
}

void CAssetViewer::RefreshDatabases()
{
	LockAll();

	m_cAssetItems.clear();
	m_cThumbsCache.clear();
	m_cAssetDrawingCache.clear();
	m_cSelectedAssets.clear();
	m_nTotalAssets = 0;

	for( size_t i = 0, iCount = m_cAssetDatabases.size(); i < iCount; ++i )
	{
		m_cAssetDatabases[i]->Refresh();
		IAssetDisplayDatabase::TFilenameAssetMap& rAssets = m_cAssetDatabases[i]->GetAssets();
		m_nTotalAssets += rAssets.size();

		m_cAssetDatabases[i]->CacheFieldsInfoForAlreadyLoadedAssets();

		// add items in the list if not already added
		for( IAssetDisplayDatabase::TFilenameAssetMap::iterator iter = rAssets.begin(), iterEnd = rAssets.end(); iter != iterEnd; ++iter )
		{
			// skip this one, is filtered out
			if( !iter->second->IsFlagSet( IAssetDisplay::eAssetFlags_Visible ) )
				continue;

			if( m_cAssetItems.end() == std::find( m_cAssetItems.begin(), m_cAssetItems.end(), iter->second ) )
			{
				iter->second->SetIndex( m_cAssetItems.size() );
				m_cAssetItems.push_back( iter->second );
			}
		}
	}

	UnlockAll();
	
	RecalculateLayout();
	UpdateScrollBar();
	UpdateVisibility();

	LockAll();
	m_operationTotalCount = 0;
	m_operationCurrentCount = 0;

	for( size_t i = 0; i < m_cAssetDatabases.size(); ++i )
	{
		m_nextFieldsInfoCacheAssetIter[i] = m_cAssetDatabases[i]->GetAssets().begin();
		m_operationTotalCount += m_cAssetDatabases[i]->GetAssets().size();
	}

	m_bOperationInProgress = true;
	m_operationText = "Precaching asset fields infos...";
	m_operationProgressPercent = 0;

	UnlockAll();

	if( m_pStatusDisplayObserver )
	{
		m_pStatusDisplayObserver->OnChangeStatusBarInfo( m_cSelectedAssets.size(), m_cAssetItems.size(), m_nTotalAssets );
		m_pStatusDisplayObserver->OnSelectionChanged();
	}
}

HWND CAssetViewer::GetRenderWindow()
{
	return m_wndAssetThumbRender.GetSafeHwnd();
}

CAssetViewer::TAssetItems& CAssetViewer::GetAssetItems()
{
	return m_cAssetItems;
}

bool CAssetViewer::IsAssetVisibleInView( IAssetDisplay* pAsset )
{
	assert(pAsset);

	if( !pAsset )
		return false;

	if( !pAsset->IsFlagSet( IAssetDisplay::eAssetFlags_Visible ) )
	{
		return false;
	}

	CRect oCurrentRect, oIntersection;

	pAsset->GetDrawingRectangle( oCurrentRect );
	oCurrentRect.top -= m_nYOffset;
	oCurrentRect.bottom -= m_nYOffset;

	// If the item is entirely outside of the client rect, no need to cache a bitmap for it.
	if( oCurrentRect.bottom < 0 )
	{
		return false;
	}

	// If the item is entirely below the client rect, as Y coordinates grow monotonically,
	// no need to cache anything else.
	if( oCurrentRect.top > m_oClientRect.bottom )
	{
		return false;
	}

	// Added to use the same criteria as in the remove list.
	if( !oIntersection.IntersectRect( m_oClientRect, oCurrentRect ) )
	{
		return false;
	}

	return true;
}

void CAssetViewer::PushToThumbsCacheQueue( IAssetDisplay* pAsset )
{
	// first check if this is already in the cache
	if( std::find( m_cThumbsCache.begin(), m_cThumbsCache.end(), pAsset ) != m_cThumbsCache.end() )
	{
		return;
	}

	// first lets validate the queue, by removing all non valid assets, not found in m_cAssetItems
	std::deque<IAssetDisplay*>::iterator iter = m_cThumbsCache.begin(), iterEnd = m_cThumbsCache.end();

	while( iter != iterEnd )
	{
		if( m_cAssetItems.end() == std::find( m_cAssetItems.begin(), m_cAssetItems.end(), *iter ) )
		{
			iter = m_cThumbsCache.erase( iter );
			continue;
		}

		++iter;
	}

	// if we have a filled up queue, then uncache oldest thumb
	if( m_cThumbsCache.size() >= kAssetViewer_MaxThumbsCacheAssetCount )
	{
		IAssetDisplay* pOldest = m_cThumbsCache.front();
		
		pOldest->UnCacheThumbnail();
		m_cThumbsCache.erase(	m_cThumbsCache.begin() );
	}

	m_cThumbsCache.push_back( pAsset );
}

string CAssetViewer::GetAssetFieldDisplayValue( IAssetDisplay* pAsset, const char* pFieldname )
{
	assert( pAsset );
	assert( pAsset->GetOwnerDisplayDatabase() );

	SAssetField* pField = pAsset->GetOwnerDisplayDatabase()->GetAssetFieldByName( pFieldname );

	if( !pField )
		return "";

	switch( pField->m_fieldType )
	{
		case SAssetField::eAssetFieldType_Int8:
		{
			char value;

			if( pAsset->GetAssetFieldValue( pFieldname, &value ) )
			{
				stack_string str;
				str.Format( "%d", value );
				return str.c_str();
			}

			break;
		}
	
		case SAssetField::eAssetFieldType_Int16:
		{
			short int value;

			if( pAsset->GetAssetFieldValue( pFieldname, &value ) )
			{
				stack_string str;
				str.Format( "%d", value );
				return str.c_str();
			}

			break;
		}

		case SAssetField::eAssetFieldType_Int32:
		{
			int value;

			if( pAsset->GetAssetFieldValue( pFieldname, &value ) )
			{
				stack_string str;
				str.Format( "%d", value );
				return str.c_str();
			}

			break;
		}

		case SAssetField::eAssetFieldType_Int64:
		{
			__int64 value;

			if( pAsset->GetAssetFieldValue( pFieldname, &value ) )
			{
				stack_string str;
				str.Format( "%I64", value );
				return str.c_str();
			}

			break;
		}

		case SAssetField::eAssetFieldType_Float:
		{
			float value;
			
			if( pAsset->GetAssetFieldValue( pFieldname, &value ) )
			{
				stack_string str;
				str.Format( "%.4f", value );
				return str.c_str();
			}

			break;
		}

		case SAssetField::eAssetFieldType_Double:
		{
			double value;
			
			if( pAsset->GetAssetFieldValue( pFieldname, &value ) )
			{
				stack_string str;
				str.Format( "%g", value );
				return str.c_str();
			}

			break;
		}

		case SAssetField::eAssetFieldType_String:
		{
			string value;
			
			if( pAsset->GetAssetFieldValue( pFieldname, &value ) )
			{
				return value;
			}
		}
	}

	return "";
}

//------------------------------------------------------------------------------
// SORTING ASSETS
//------------------------------------------------------------------------------
SAssetField*	CAssetViewer::m_spSortField = NULL;
bool					CAssetViewer::m_sbSortDescending = false;

bool CAssetViewer::AssetViewerSortAssetItems( IAssetDisplay* pA, IAssetDisplay* pB )
{
	// this should not happen, but we check it still in debug
	assert( pA );
	assert( pB );

	switch( CAssetViewer::m_spSortField->m_fieldType )
	{
		case SAssetField::eAssetFieldType_Int8:
		{
			char valueA, valueB;

			if( !pA->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueA ) ||
					!pB->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueB ) )
					return false;

			return ( CAssetViewer::m_sbSortDescending ) ? valueA < valueB : valueA > valueB;
		}

		case SAssetField::eAssetFieldType_Int16:
		{
			short int valueA, valueB;

			if( !pA->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueA ) ||
					!pB->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueB ) )
					return false;

			return ( CAssetViewer::m_sbSortDescending ) ? valueA < valueB : valueA > valueB;
		}

		case SAssetField::eAssetFieldType_Int32:
		{
			int valueA, valueB;

			if( !pA->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueA ) ||
					!pB->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueB ) )
					return false;

			return ( CAssetViewer::m_sbSortDescending ) ? valueA < valueB : valueA > valueB;
		}

		case SAssetField::eAssetFieldType_Int64:
		{
			__int64 valueA, valueB;

			if( !pA->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueA ) ||
					!pB->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueB ) )
					return false;

			return ( CAssetViewer::m_sbSortDescending ) ? valueA < valueB : valueA > valueB;
		}

		case SAssetField::eAssetFieldType_Float:
		{
			float valueA, valueB;

			if( !pA->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueA ) ||
					!pB->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueB ) )
				return false;

			return ( CAssetViewer::m_sbSortDescending ) ? valueA < valueB : valueA > valueB;
		}

		case SAssetField::eAssetFieldType_Double:
		{
			double valueA, valueB;

			if( !pA->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueA ) ||
					!pB->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueB ) )
				return false;

			return ( CAssetViewer::m_sbSortDescending ) ? valueA < valueB : valueA > valueB;
		}

		case SAssetField::eAssetFieldType_String:
		{
			string valueA, valueB;

			if( !pA->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueA ) ||
					!pB->GetAssetFieldValue( CAssetViewer::m_spSortField->m_fieldName.c_str(), &valueB ) )
				return false;

			return ( CAssetViewer::m_sbSortDescending ) ? valueA < valueB : valueA > valueB;
		}
	}

	return false;
}

void CAssetViewer::SortAssets( const char* pFieldname, bool bDescending )
{
	LockAll();

	if( m_cAssetItems.empty() )
	{
		UnlockAll();
		return;
	}

	m_strCurrentSortFieldname = pFieldname;
	CAssetViewer::m_sbSortDescending = bDescending;
	CAssetViewer::m_spSortField = m_cAssetItems[0]->GetOwnerDisplayDatabase()->GetAssetFieldByName( pFieldname );

	if( CAssetViewer::m_spSortField )
	{
		std::sort( m_cAssetItems.begin(), m_cAssetItems.end(), AssetViewerSortAssetItems );
	}

	UnlockAll();
	RecalculateLayout();
	UpdateScrollBar();
	UpdateVisibility();

	if( m_pStatusDisplayObserver )
	{
		m_pStatusDisplayObserver->OnChangeStatusBarInfo( m_cSelectedAssets.size(), m_cAssetItems.size(), m_nTotalAssets );
		m_pStatusDisplayObserver->OnSelectionChanged();
	}
}

void CAssetViewer::ApplyFilters( const IAssetDisplayDatabase::TAssetFieldFiltersMap& rFieldFilters )
{
	LockAll();

	for( size_t i = 0, iCount = m_cAssetDatabases.size(); i < iCount; ++i )
	{
		m_cAssetDatabases[i]->ApplyFilters( rFieldFilters );
	}

	m_cAssetItems.clear();
	m_cThumbsCache.clear();
	m_cAssetDrawingCache.clear();

	for( size_t i = 0, iCount = m_cAssetDatabases.size(); i < iCount; ++i )
	{
		IAssetDisplayDatabase::TFilenameAssetMap& rAssets = m_cAssetDatabases[i]->GetAssets();

		// add items in the list if not already added
		for( IAssetDisplayDatabase::TFilenameAssetMap::iterator iter = rAssets.begin(), iterEnd = rAssets.end(); iter != iterEnd; ++iter )
		{
			// skip this one, is filtered out
			if( !iter->second->IsFlagSet( IAssetDisplay::eAssetFlags_Visible ) )
				continue;

			if( m_cAssetItems.end() == std::find( m_cAssetItems.begin(), m_cAssetItems.end(), iter->second ) )
			{
				iter->second->SetIndex( m_cAssetItems.size() );
				m_cAssetItems.push_back( iter->second );
			}
		}
	}

	UnlockAll();

	m_nYOffset = 0;
	RecalculateLayout();
	UpdateScrollBar();
	UpdateVisibility();

	if( m_pStatusDisplayObserver )
	{
		m_pStatusDisplayObserver->OnChangeStatusBarInfo( m_cSelectedAssets.size(), m_cAssetItems.size(), m_nTotalAssets );
		m_pStatusDisplayObserver->OnSelectionChanged();
	}
}

void CAssetViewer::ClearFilters()
{
	LockAll();

	for( size_t i = 0, iCount = m_cAssetDatabases.size(); i < iCount; ++i )
	{
		m_cAssetDatabases[i]->ClearFilters();
	}

	m_cAssetItems.clear();
	m_cThumbsCache.clear();
	m_cAssetDrawingCache.clear();
	m_nTotalAssets = 0;

	for( size_t i = 0, iCount = m_cAssetDatabases.size(); i < iCount; ++i )
	{
		IAssetDisplayDatabase::TFilenameAssetMap& rAssets = m_cAssetDatabases[i]->GetAssets();
		m_nTotalAssets += rAssets.size();

		// add items in the list if not already added
		for( IAssetDisplayDatabase::TFilenameAssetMap::iterator iter = rAssets.begin(), iterEnd = rAssets.end(); iter != iterEnd; ++iter )
		{
			// skip this one, is filtered out
			if( !iter->second->IsFlagSet( IAssetDisplay::eAssetFlags_Visible ) )
				continue;

			if( m_cAssetItems.end() == std::find( m_cAssetItems.begin(), m_cAssetItems.end(), iter->second ) )
			{
				iter->second->SetIndex( m_cAssetItems.size() );
				m_cAssetItems.push_back( iter->second );
			}
		}
	}

	UnlockAll();
	m_nYOffset = 0;
	RecalculateLayout();
	UpdateScrollBar();
	UpdateVisibility();

	if( m_pStatusDisplayObserver )
	{
		m_pStatusDisplayObserver->OnChangeStatusBarInfo( m_cSelectedAssets.size(), m_cAssetItems.size(), m_nTotalAssets );
		m_pStatusDisplayObserver->OnSelectionChanged();
	}
}

void CAssetViewer::EnsureAssetVisible( UINT aIndex )
{
	if( aIndex >= m_cAssetItems.size() )
	{
		m_poEnsureVisible = NULL;
		return;
	}

	m_poEnsureVisible = m_cAssetItems[aIndex];
	EnsureAssetVisible( m_poEnsureVisible );
}

void CAssetViewer::EnsureAssetVisible( IAssetDisplay* pAsset )
{
	m_poEnsureVisible = pAsset;
	DeselectAll();

	m_smoothPanLastDelta = 0;
	m_smoothPanLeftAmount = 0;

	if( m_poEnsureVisible )
	{
		m_poEnsureVisible->SetFlag( IAssetDisplay::eAssetFlags_Selected, true );

		if( m_poEnsureVisible->IsFlagSet( IAssetDisplay::eAssetFlags_Visible ) )
		{
			CRect oElementRect, oDrawRect;

			m_poEnsureVisible->GetDrawingRectangle( oElementRect );
			oDrawRect = oElementRect;
			oDrawRect.OffsetRect( 0, -m_nYOffset );

			if( oDrawRect.top < 0 || oDrawRect.bottom > m_oClientRect.Height() )
			{
				m_nYOffset = oElementRect.top - m_nItemVerticalMargin;
			}
		}
	}

	UpdateScrollBar();
	UpdateVisibility();
	m_thumbFocusAnimationTime = 0;
	SetTimer( ID_ASSET_VIEWER_TIMER_THUMB_FOCUS_ANIMATION, kAssetViewer_ThumbFocusAnimationTimerDelay, NULL );
	Invalidate( FALSE );
}

bool CAssetViewer::IsAnyOperationInProgress()
{
	bool bOn = false;

	m_oOperationLock.Lock();
	bOn = m_bOperationInProgress;
	m_oOperationLock.Unlock();

	return bOn;
}

void CAssetViewer::GetCurrentOperationTextAndProgress( CString& rText, int& rProgress )
{
	m_oOperationLock.Lock();
	rText = m_operationText;
	rProgress = m_operationProgressPercent;
	m_oOperationLock.Unlock();
}

void CAssetViewer::SetStatusDisplay( IAssetViewerStatusDisplay* pObserver )
{
	m_pStatusDisplayObserver = pObserver;
}

IAssetViewerStatusDisplay* CAssetViewer::GetStatusDisplay()
{
	return m_pStatusDisplayObserver;
}
