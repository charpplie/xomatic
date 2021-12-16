////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetBrowserPreviewDlg.cpp
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	Implementation of AssetBrowserPreviewDlg.h
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "stdafx.h"
#include "AssetBrowserPreviewDlg.h"

// CAssetBrowserPreviewDlg dialog

namespace AssetBrowser
{
	const UINT	kPreview_ViewportMargin = 5;
};

IMPLEMENT_DYNAMIC(CAssetBrowserPreviewDlg, CDialog)
IMPLEMENT_DYNAMIC(COwnerDrawnCStatic, CStatic)

CAssetBrowserPreviewDlg::CAssetBrowserPreviewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserPreviewDlg::IDD, pParent)
{
	m_pAssetDisplay = NULL;
	m_bDragging = false;
	m_pAssetPreviewDlg = NULL;
	m_noPreviewTextFont.CreatePointFont( 90, "Arial Bold" );
}

CAssetBrowserPreviewDlg::~CAssetBrowserPreviewDlg()
{
	m_noPreviewTextFont.DeleteObject();
}

void CAssetBrowserPreviewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_ASSET_BROWSER_PREVIEW_VIEWPORT, m_wndAssetRender);
}


BEGIN_MESSAGE_MAP(CAssetBrowserPreviewDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()


// CAssetBrowserPreviewDlg message handlers

void CAssetBrowserPreviewDlg::OnPaint()
{
	CPaintDC dc(this); 
	CRect rc;
	
	if( m_pAssetDisplay && m_wndAssetRender.GetSafeHwnd() )
	{
		m_wndAssetRender.GetClientRect( &rc );
		m_pAssetDisplay->Render( m_wndAssetRender.m_hWnd, rc );

		if( m_pAssetDisplay->IsFlagSet( IAssetDisplay::eAssetFlags_UseGdiRendering ) )
		{
			m_canvas.BlitTo( m_wndAssetRender.m_hWnd );
		}
	}
	else
	{
		m_wndAssetRender.GetClientRect( &rc );
		m_canvas.GradientFillRect( rc, RGB( 0, 0, 0 ), RGB( 70, 70, 110 ), GRADIENT_FILL_RECT_V );
		m_canvas.GetDC().SelectObject( m_noPreviewTextFont );
		m_canvas.GetDC().SetTextColor( RGB( 100, 100, 155 ) );
		m_canvas.GetDC().DrawText( "<No preview available>", &rc, DT_SINGLELINE|DT_VCENTER|DT_CENTER|DT_WORD_ELLIPSIS|DT_WORDBREAK );
		m_canvas.BlitTo( m_wndAssetRender.m_hWnd );
	}
}

BOOL CAssetBrowserPreviewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_piRenderer = gEnv->pRenderer;

	if( m_piRenderer->CreateContext( m_wndAssetRender.m_hWnd ) )
	{
		m_piRenderer->MakeMainContextActive();
	}

	CDC* pDC = m_wndAssetRender.GetDC();
	CRect rc;

	m_wndAssetRender.GetClientRect( &rc );
	m_canvas.Create( pDC->GetSafeHdc(), rc.Width(), rc.Height() );
	m_wndAssetRender.ReleaseDC( pDC );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAssetBrowserPreviewDlg::StartPreviewAsset( IAssetDisplay* pAsset )
{
	if( !GetSafeHwnd() )
		return;

	m_pAssetDisplay = pAsset;

	if( m_pAssetDisplay )
	{
		m_pAssetDisplay->OnBeginPreview( m_wndAssetRender.m_hWnd, m_canvas.GetDC().GetSafeHdc() );
		m_pAssetPreviewDlg = m_pAssetDisplay->GetCustomPreviewPanel( this );
	}
	else
	{
		m_pAssetDisplay = NULL;
		m_pAssetPreviewDlg = NULL;
	}

	ResizeControls();
	Invalidate( FALSE );
}

void CAssetBrowserPreviewDlg::EndPreviewAsset()
{
	// lets remake the small thumb image by rendering the asset again, before closing the preview
	if( m_pAssetDisplay && m_pAssetDisplay->IsFlagSet( IAssetDisplay::eAssetFlags_InteractiveRenderSupported ) )
	{
		m_pAssetDisplay->OnEndPreview();
		m_pAssetPreviewDlg = m_pAssetDisplay->GetCustomPreviewPanel( this );

		if( m_pAssetPreviewDlg )
		{
			m_pAssetPreviewDlg->ShowWindow( SW_HIDE );
		}
	}

	m_pAssetDisplay = NULL;
	m_pAssetPreviewDlg = NULL;
}

void CAssetBrowserPreviewDlg::ResizeControls()
{
	if( !GetSafeHwnd() )
		return;

	CRect rc, rcThis;

	GetClientRect( &rcThis );

	if( m_pAssetDisplay )
	{
		rc.SetRectEmpty();

		if( m_pAssetPreviewDlg )
		{
			m_pAssetPreviewDlg->GetWindowRect( &rc );
			m_pAssetPreviewDlg->SetWindowPos( 0, 0, 0, rcThis.Width(), rc.Height(), 0 );
			m_pAssetPreviewDlg->ShowWindow( SW_SHOW );
		}

		m_wndAssetRender.SetWindowPos(	0, AssetBrowser::kPreview_ViewportMargin, AssetBrowser::kPreview_ViewportMargin + rc.Height(),
																		rcThis.Width() - AssetBrowser::kPreview_ViewportMargin*2,
																		rcThis.Height() - rc.Height() - AssetBrowser::kPreview_ViewportMargin*2, 0 );

		if( m_canvas.GetDC().GetSafeHdc() )
		{
			HDC hdc = ::GetDC( m_wndAssetRender.m_hWnd );
			m_wndAssetRender.GetClientRect( &rc );
			m_canvas.Resize( hdc, rc.Width(), rc.Height() );
			::ReleaseDC( m_wndAssetRender.m_hWnd, hdc );
		}
	}
	else
	{
		m_pAssetDisplay = NULL;
		m_pAssetPreviewDlg = NULL;

		m_wndAssetRender.SetWindowPos(	0, AssetBrowser::kPreview_ViewportMargin, AssetBrowser::kPreview_ViewportMargin,
																		rcThis.Width() - AssetBrowser::kPreview_ViewportMargin*2,
																		rcThis.Height() - AssetBrowser::kPreview_ViewportMargin*2, 0 );

		if( m_canvas.GetDC().GetSafeHdc() )
		{
			HDC hdc = ::GetDC( m_wndAssetRender.m_hWnd );
			m_wndAssetRender.GetClientRect( &rc );
			m_canvas.Resize( hdc, rc.Width(), rc.Height() );
			::ReleaseDC( m_wndAssetRender.m_hWnd, hdc );
		}
	}

	Invalidate( FALSE );
}

void CAssetBrowserPreviewDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	SetFocus();
	m_bDragging = true;
	m_lastPanDragPt = point;
	Invalidate( FALSE );

	CDialog::OnLButtonDown(nFlags, point);
}

void CAssetBrowserPreviewDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_bDragging = false;
	Invalidate( FALSE );

	CDialog::OnLButtonUp(nFlags, point);
}

void CAssetBrowserPreviewDlg::OnMButtonDown(UINT nFlags, CPoint point)
{
	Invalidate( FALSE );

	CDialog::OnMButtonDown(nFlags, point);
}

void CAssetBrowserPreviewDlg::OnMButtonUp(UINT nFlags, CPoint point)
{
	Invalidate( FALSE );

	CDialog::OnMButtonUp(nFlags, point);
}

void CAssetBrowserPreviewDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	Invalidate( FALSE );

	CDialog::OnRButtonDown(nFlags, point);
}

void CAssetBrowserPreviewDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	Invalidate( FALSE );

	CDialog::OnRButtonUp(nFlags, point);
}

void CAssetBrowserPreviewDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if( m_bDragging )
	{
		if( m_pAssetDisplay && m_pAssetDisplay->IsFlagSet( IAssetDisplay::eAssetFlags_InteractiveRenderSupported ) )
		{
			CRect rc;
			CPoint pt = point;

			m_wndAssetRender.GetClientRect( &rc );
			m_wndAssetRender.ScreenToClient( &pt );
			m_pAssetDisplay->InteractiveRender( m_wndAssetRender.m_hWnd, rc, pt.x, pt.y, point.x - m_lastPanDragPt.x, point.y - m_lastPanDragPt.y, 0, nFlags );
			m_lastPanDragPt = point;
		}
	}

	RedrawWindow();

	CDialog::OnMouseMove(nFlags, point);
}

void CAssetBrowserPreviewDlg::OnDestroy()
{
	if( m_piRenderer )
	{
		m_piRenderer->DeleteContext( m_wndAssetRender.m_hWnd );
	}

	CDialog::OnDestroy();
}

BOOL CAssetBrowserPreviewDlg::PreTranslateMessage(MSG* pMsg)
{
	if( ( pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP ) && ( pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_RETURN ) )
	{
		return CDialog::PreTranslateMessage(pMsg);
	}

	if( pMsg->message == WM_MOUSEWHEEL )
	{
		if( m_wndAssetRender.GetFocus() == &m_wndAssetRender && m_pAssetDisplay &&
				m_pAssetDisplay->IsFlagSet( IAssetDisplay::eAssetFlags_InteractiveRenderSupported ) )
		{
			CRect rc;
			CPoint pt;
			UINT fwKeys = GET_KEYSTATE_WPARAM(pMsg->wParam);
			int zDelta = GET_WHEEL_DELTA_WPARAM(pMsg->wParam);

			GetCursorPos( &pt );

			m_wndAssetRender.ScreenToClient( &pt );
			m_wndAssetRender.GetClientRect( &rc );

			// send this message only if the mouse is hovering the viewport
			if( rc.PtInRect( pt ) )
			{
				m_pAssetDisplay->InteractiveRender( m_wndAssetRender.m_hWnd, rc, 0, 0, 0, 0, zDelta, fwKeys );
				Invalidate( FALSE );
			}
		}
	}

	if( pMsg->message == WM_KEYDOWN && m_wndAssetRender.GetFocus() == &m_wndAssetRender )
	{
		if( m_pAssetDisplay )
		{
			UINT nChar = pMsg->wParam;
			UINT nFlags = pMsg->lParam;

			m_pAssetDisplay->OnInteractiveRenderKeyEvent( true, nChar, nFlags );
			Invalidate( FALSE );
			OnKeyDown( nChar, 0, nFlags );

			return FALSE;
		}
	}
	else
	if( pMsg->message == WM_KEYUP && m_wndAssetRender.GetFocus() == &m_wndAssetRender  )
	{
		if( m_pAssetDisplay )
		{
			UINT nChar = pMsg->wParam;
			UINT nFlags = pMsg->lParam;

			m_pAssetDisplay->OnInteractiveRenderKeyEvent( false, nChar, nFlags );
			Invalidate( FALSE );

			return FALSE;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CAssetBrowserPreviewDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	CRect rc;

	if( m_wndAssetRender.m_hWnd )
	{
		CRect rcThis;

		GetClientRect( &rcThis );
		rc.SetRectEmpty();

		if( m_pAssetPreviewDlg )
		{
			m_pAssetPreviewDlg->GetWindowRect( &rc );
			m_pAssetPreviewDlg->SetWindowPos( 0, 0, 0, rcThis.Width(), rc.Height(), SWP_NOMOVE );
		}

		m_wndAssetRender.SetWindowPos(	0, AssetBrowser::kPreview_ViewportMargin, AssetBrowser::kPreview_ViewportMargin + rc.Height(),
																		cx - AssetBrowser::kPreview_ViewportMargin*2, cy - rc.Height() - AssetBrowser::kPreview_ViewportMargin*2, 0 );
	}

	if( m_canvas.GetDC().GetSafeHdc() )
	{
		HDC hdc = ::GetDC( m_wndAssetRender.m_hWnd );
		m_wndAssetRender.GetClientRect( &rc );
		m_canvas.Resize( hdc, rc.Width(), rc.Height() );
		::ReleaseDC( m_wndAssetRender.m_hWnd, hdc );
	}

	RedrawWindow();
}

void CAssetBrowserPreviewDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}
