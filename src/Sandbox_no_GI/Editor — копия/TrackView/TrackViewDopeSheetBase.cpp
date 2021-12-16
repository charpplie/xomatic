////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   TrackViewDopeSheetBase.cpp
//  Version:     v1.00
//  Created:     23/8/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "TrackViewDopeSheetBase.h"
#include "Controls\MemDC.h"

#include "TrackViewDialog.h"
#include "AnimationContext.h"
#include "TrackViewUndo.h"

#include "Clipboard.h"

//#include "IMovieSystem.h"

#define KEY_TEXT_COLOR RGB(255,255,255)
#define EDIT_DISABLE_GRAY_COLOR RGB(180,180,180)

static const int MARGIN_FOR_MAGNET_SNAPPING = 10;

// CTrackViewKeys

IMPLEMENT_DYNAMIC(CTrackViewDopeSheetBase, CWnd)

CTrackViewDopeSheetBase::CTrackViewDopeSheetBase()
{
	m_pSequence = 0;
	m_bkgrBrush.CreateSolidBrush( GetSysColor(COLOR_3DFACE) );
	//m_bkgrBrushEmpty.CreateHatchBrush( HS_BDIAGONAL,GetSysColor(COLOR_3DFACE) );
	m_bkgrBrushEmpty.CreateSolidBrush( RGB(190,190,190) );
	m_timeBkgBrush.CreateSolidBrush(RGB(0xE0,0xE0,0xE0));
	m_timeHighlightBrush.CreateSolidBrush(RGB(0xFF,0x0,0x0));
	m_selectedBrush.CreateSolidBrush(RGB(200,200,230));
	//m_visibilityBrush.CreateSolidBrush( RGB(0,150,255) );
	m_visibilityBrush.CreateSolidBrush( RGB(120,120,255) );
	m_selectTrackBrush.CreateSolidBrush( RGB(100,190,255) );

	m_timeScale = 1;
	m_ticksStep = 10;

	m_bZoomDrag=false;
	m_bMoveDrag=false;

	m_bMouseOverKey = false;

	m_leftOffset = 0;
	m_scrollOffset = CPoint(0,0);
	m_bAnySelected = 0;
	m_mouseMode = MOUSE_MODE_NONE;
	m_currentTime = 40;
	m_storedTime = m_currentTime;
	m_rcSelect = CRect(0,0,0,0);
	m_keyTimeOffset = 0;
	m_currCursor = NULL;
	m_mouseActionMode = TVMODE_MOVEKEY;

	m_itemWidth = 1000;
	m_scrollMin = 0;
	m_scrollMax = 1000;
	m_itemHeight = 16;

	m_descriptionFont = new CFont();
	m_descriptionFont->CreateFont( 12,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
 		DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"Verdana" );

	m_bCursorWasInKey = false;
	m_bJustSelected = false;
	m_snappingMode = TVKEY_SNAP_NONE;
	m_snapFrameTime = 0.033333f;
	m_bMouseMovedAfterRButtonDown = false;

	m_tickDisplayMode = TVTICK_INSECONDS;

	m_bEditLock = false;

	m_pLastTrackSelectedOnSpot = NULL;

	m_pTrackForTimeAdjust = NULL;
	m_keyForTimeAdjust = -1;
}

CTrackViewDopeSheetBase::~CTrackViewDopeSheetBase()
{
	m_descriptionFont->DeleteObject();
	delete m_descriptionFont;
}


BEGIN_MESSAGE_MAP(CTrackViewDopeSheetBase, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_MEASUREITEM_REFLECT()
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
	ON_WM_HSCROLL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_PAINT()
	ON_WM_SETCURSOR()
	ON_WM_ERASEBKGND()
	ON_WM_RBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_INPUT()
	ON_WM_CAPTURECHANGED()
END_MESSAGE_MAP()



// CTrackViewKeys message handlers

//////////////////////////////////////////////////////////////////////////
int CTrackViewDopeSheetBase::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_imageList.Create( MAKEINTRESOURCE(IDB_TRACKVIEW_KEYS),14,0,RGB(255,0,255) );
	m_imgMarker.Create( MAKEINTRESOURCE(IDB_MARKER),8,0,RGB(255,0,255) );
	m_crsLeftRight = AfxGetApp()->LoadStandardCursor(IDC_SIZEWE);
	m_crsAddKey = AfxGetApp()->LoadCursor(IDC_ARROW_ADDKEY);
	m_crsCross = AfxGetApp()->LoadCursor(IDC_POINTER_OBJHIT);
	m_crsAdjustLR = AfxGetApp()->LoadCursor(IDC_LEFTRIGHT);

	GetIEditor()->RegisterNotifyListener( this );

	//InitializeFlatSB(GetSafeHwnd());

	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnDestroy()
{
	HideKeyPropertyCtrlOnSpot();
	GetIEditor()->UnregisterNotifyListener( this );
	CWnd::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawSelectedKeyIndicators( CDC *dc )
{
	if (m_pSequence == NULL)
		return;

	CPen selPen( PS_SOLID,1,RGB(255,255,0) );
	CPen *prevPen = dc->SelectObject( &selPen );
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys( m_pSequence,selectedKeys );
	for (int i = 0; i < selectedKeys.keys.size(); ++i)
	{
		IAnimTrack *pTrack = selectedKeys.keys[i].pTrack;
		int keyIndex = selectedKeys.keys[i].nKey;
		int x = TimeToClient(pTrack->GetKeyTime(keyIndex));
		dc->MoveTo(x,m_rcClient.top);
		dc->LineTo(x,m_rcClient.bottom);
	}
	dc->SelectObject(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawTimeline( CDC *dc,const CRect &rcUpdate )
{
	CRect rc,temprc;
	
	bool recording = GetIEditor()->GetAnimation()->IsRecording();

	COLORREF lineCol = RGB(255,0,255);
	COLORREF textCol = RGB(0,0,0);	
	COLORREF dkgrayCol = RGB(90,90,90);	
	COLORREF ltgrayCol = RGB(150,150,150);	

	if (recording)
	{
		lineCol = RGB(255,0,0);
		//textCol = RGB(255,255,255);
	}

	// Draw vertical line showing current time.
	{
		int x = TimeToClient(m_currentTime);
		if (x > m_rcClient.left && x < m_rcClient.right)
		{
			CPen pen( PS_SOLID,1,lineCol );
			CPen *prevPen = dc->SelectObject(&pen);
			dc->MoveTo( x,0 );
			dc->LineTo( x,m_rcClient.bottom );
			dc->SelectObject( prevPen );
		}
	}

	rc = m_rcTimeline;
	if (temprc.IntersectRect(rc,rcUpdate) == 0)
		return;

	/*
	if (recording)
	{
		dc->FillRect( rc,&m_timeHighlightBrush );
	}
	else
	*/
	{
//		dc->FillRect( rc,&m_timeBkgBrush );
	}
	//dc->Draw3dRect( rc,GetSysColor(COLOR_3DHILIGHT),GetSysColor(COLOR_3DDKSHADOW) );

	//XTPPaintManager()->GradientFill( dc,rc,GetSysColor(COLOR_3DHILIGHT),GetSysColor(COLOR_3DDKSHADOW),FALSE );
	XTPPaintManager()->GradientFill( dc,rc,RGB(250,250,250),RGB(180,180,180),FALSE );

	CPen *prevPen;
	CPen dkgray(PS_SOLID,1,dkgrayCol);
	CPen ltgray(PS_SOLID,1,ltgrayCol);	
	CPen black(PS_SOLID,1,textCol);	
	CPen redpen(PS_SOLID,1,lineCol );
	// Draw time ticks every tick step seconds.
	Range timeRange = m_timeRange;
	CString str;

	dc->SetTextColor( textCol );
	dc->SetBkMode( TRANSPARENT );
	dc->SelectObject( gSettings.gui.hSystemFont );

	dc->SelectObject(dkgray);

	Range VisRange=GetVisibleRange();
	int nNumberTicks=10;
	if (GetTickDisplayMode() == TVTICK_INFRAMES)
		nNumberTicks = RoundFloatToInt(1.0f / m_snapFrameTime);
	
	double start = TickSnap( timeRange.start );
	double step = (double)1.0 / m_ticksStep;	

	for (double t = 0.0f; t <= timeRange.end+step; t += step)
	{
		double st = TickSnap(t);
		if (st > timeRange.end)
			st = timeRange.end;
		if (st < VisRange.start)
			continue; 
		if (st > VisRange.end)
			break;

		int x = TimeToClient(st);
		dc->MoveTo(x,rc.bottom-2);

		if(st >= start)
		{
			dc->SelectObject(black);
			dc->SetTextColor(textCol);
		}
		else
		{
			dc->SelectObject(ltgray);
			dc->SetTextColor(ltgrayCol);
		}

		int k = RoundFloatToInt(st * m_ticksStep);
		if (k % nNumberTicks == 0)
		{
			dc->LineTo(x,rc.bottom-14);
			char str[32];
			if (GetTickDisplayMode() == TVTICK_INSECONDS)
				sprintf( str,"%g",st );
			else if (GetTickDisplayMode() == TVTICK_INFRAMES)
				sprintf( str,"%g",st/m_snapFrameTime );
			else
				assert(0);
			dc->TextOut( x+2,rc.top,str );
		}
		else
			dc->LineTo(x,rc.bottom-6);		

		dc->SelectObject(dkgray);
	}

	// Draw time markers.
	int x;

	x=TimeToClient(m_timeMarked.start);
	m_imgMarker.Draw(dc, 1, CPoint(x, m_rcTimeline.bottom-9), ILD_TRANSPARENT);
	x=TimeToClient(m_timeMarked.end);
	m_imgMarker.Draw(dc, 0, CPoint(x-7, m_rcTimeline.bottom-9), ILD_TRANSPARENT);

	prevPen = dc->SelectObject(&redpen);
	x=TimeToClient(m_currentTime);
	dc->SelectObject( GetStockObject(NULL_BRUSH) );
	dc->Rectangle( x-3,rc.top,x+4,rc.bottom );

	dc->SelectObject(redpen);
	dc->MoveTo( x,rc.top ); dc->LineTo( x,rc.bottom );
	dc->SelectObject( GetStockObject(NULL_BRUSH) );
//	dc->Rectangle( x-3,rc.top,x+4,rc.bottom );

	dc->SelectObject(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawSummary( CDC *dc, CRect rcUpdate )
{
	if (m_pSequence == NULL)
		return;

	CRect rc,temprc;
	COLORREF lineCol = RGB(0,0,0);
	COLORREF fillCol = RGB(150,100,220);

	rc = m_rcSummary;
	if (temprc.IntersectRect(rc,rcUpdate) == 0)
		return;

	dc->FillSolidRect(rc.left, rc.top, rc.Width(), rc.Height(), fillCol);
	
	CPen *prevPen;
	CPen blackPen(PS_SOLID,3,lineCol);
	Range timeRange = m_timeRange;

	prevPen = dc->SelectObject(&blackPen);

	// Draw a short thick line at each place where there is a key in any tracks.
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetAllKeys( m_pSequence,selectedKeys );
	for (int i = 0; i < selectedKeys.keys.size(); ++i)
	{
		IAnimTrack *pTrack = selectedKeys.keys[i].pTrack;
		int keyIndex = selectedKeys.keys[i].nKey;
		int x = TimeToClient(pTrack->GetKeyTime(keyIndex));
		dc->MoveTo(x,rc.bottom-2);
		dc->LineTo(x,rc.top+2);
	}

	dc->SelectObject(prevPen);
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewDopeSheetBase::TimeToClient( float time ) const
{
	int x = m_leftOffset - m_scrollOffset.x + time*m_timeScale;
	return x;
}

//////////////////////////////////////////////////////////////////////////
Range CTrackViewDopeSheetBase::GetVisibleRange()
{
	Range r;
	r.start = (m_scrollOffset.x - m_leftOffset)/m_timeScale;
	r.end = r.start + (m_rcClient.Width())/m_timeScale;

	Range extendedTimeRange( 0.0f, m_timeRange.end );
	r = extendedTimeRange & r;

	return r;
}

//////////////////////////////////////////////////////////////////////////
Range CTrackViewDopeSheetBase::GetTimeRange( CRect &rc )
{
	Range r;
	r.start = (rc.left-m_leftOffset+m_scrollOffset.x)/m_timeScale;
	r.end = r.start + (rc.Width())/m_timeScale;

	r.start = TickSnap(r.start);
	r.end = TickSnap(r.end);

	// Intersect range with global time range.	
	r = m_timeRange & r;

	return r;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawTicks( CDC *dc,CRect &rc,Range &timeRange )
{
	// Draw time ticks every tick step seconds.
	CPen dkgray(PS_SOLID,1,RGB(90,90,90));	
	CPen ltgray(PS_SOLID,1,RGB(120,120,120));

	CPen *prevPen = dc->SelectObject( &dkgray );
	Range VisRange=GetVisibleRange();
	int nNumberTicks=10;
	if (GetTickDisplayMode() == TVTICK_INFRAMES)
		nNumberTicks = RoundFloatToInt(1.0f / m_snapFrameTime);
	
	double start = TickSnap(timeRange.start);
	double step = 1.0 / m_ticksStep;

	for (double t = 0.0f; t <= timeRange.end+step; t += step)
	{
		double st = TickSnap(t);
		if (st > timeRange.end)
			st = timeRange.end;
		if (st < VisRange.start)
			continue;
		if (st > VisRange.end)
			break;
		int x = TimeToClient(st);
		if (x < 0)
			continue;
		dc->MoveTo(x,rc.bottom-1);

		int k = RoundFloatToInt(st * m_ticksStep);
		if (k % nNumberTicks == 0)
		{
			if(st >= start)
				dc->SelectObject( GetStockObject(BLACK_PEN) );
			else
				dc->SelectObject( dkgray );

			dc->LineTo(x,rc.bottom-5);
			dc->SelectObject( dkgray );
		}
		else
		{
			if(st >= start)
				dc->SelectObject( dkgray );
			else
				dc->SelectObject( ltgray );

			dc->LineTo(x,rc.bottom-3);
		}
	}
	dc->SelectObject( prevPen );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawKeys( IAnimTrack *track,CDC *dc,CRect &rc,Range &timeRange )
{
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::RedrawItem( int item )
{
	CRect rc;
	if (GetItemRect( item,rc ) != LB_ERR)
	{
		RedrawWindow( rc,NULL,RDW_INVALIDATE|RDW_ERASE );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	lpMIS->itemWidth = 1000;
  lpMIS->itemHeight = 16;
}

//////////////////////////////////////////////////////////////////////////
HBRUSH CTrackViewDopeSheetBase::CtlColor(CDC* pDC, UINT nCtlColor)
{
	return m_bkgrBrush;

	// TODO:  Return a non-NULL brush if the parent's handler should not be called
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetTimeRange( float start,float end )
{
	/*
	if (m_timeMarked.start==m_timeRange.start)
		m_timeMarked.start=start;
	if (m_timeMarked.end==m_timeRange.end)
		m_timeMarked.end=end;
	if (m_timeMarked.end>end)
		m_timeMarked.end=end;
		*/
	if (m_timeMarked.start < start)
		m_timeMarked.start = start;
	if (m_timeMarked.end > end)
		m_timeMarked.end = end;

	m_timeRange.Set( start,end );

	SetHorizontalExtent( -m_leftOffset,m_timeRange.end*m_timeScale-m_leftOffset );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetTimeScale( float timeScale,float fAnchorTime )
{
	//m_leftOffset - m_scrollOffset.x + time*m_timeScale
	double fOldOffset = -fAnchorTime*m_timeScale;

	double fOldScale=m_timeScale;
	if (timeScale < 0.001f)
		timeScale = 0.001f;
	if (timeScale > 100000.0f)
		timeScale = 100000.0f;
	m_timeScale = timeScale;
	double fPixelsPerTick;

	int steps = 0;
	if (GetTickDisplayMode() == TVTICK_INSECONDS)
		m_ticksStep = 10;
	else if (GetTickDisplayMode() == TVTICK_INFRAMES)
		m_ticksStep = 1 / m_snapFrameTime;
	else
		assert(0);
	do
	{
		fPixelsPerTick = (1.0/m_ticksStep)*(double)m_timeScale;
		if (fPixelsPerTick<6.0)
		{
			//if (m_ticksStep>=10)
				m_ticksStep /= 2;
		}
		if (m_ticksStep <= 0)
		{
			m_ticksStep = 1;
			break;
		}
		steps++;
	}	while (fPixelsPerTick < 6.0 && steps < 100);

	steps = 0;
	do 
	{
		fPixelsPerTick = (1.0/m_ticksStep)*(double)m_timeScale;
		if (fPixelsPerTick>=12.0)
		{
			m_ticksStep *= 2;
		}
		if (m_ticksStep <= 0)
		{
			m_ticksStep = 1;
			break;
		}
		steps++;
	} while (fPixelsPerTick>=12.0 && steps < 100);

	//float 
	//m_scrollOffset.x*=timeScale/fOldScale;

	float fCurrentOffset = -fAnchorTime*m_timeScale;
	m_scrollOffset.x += fOldOffset - fCurrentOffset;

	Invalidate();

	SetHorizontalExtent( -m_leftOffset,m_timeRange.end*m_timeScale );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	GetClientRect(m_rcClient);

	if (m_offscreenBitmap.GetSafeHandle() != NULL)
		m_offscreenBitmap.DeleteObject();
	
	CDC *dc = GetDC();
	m_offscreenBitmap.CreateCompatibleBitmap( dc,m_rcClient.Width(),m_rcClient.Height() );
	ReleaseDC(dc);

	GetClientRect(m_rcTimeline);
	//m_rcTimeline.top = m_rcTimeline.bottom - m_itemHeight;
	m_rcTimeline.bottom = m_rcTimeline.top + m_itemHeight;
	m_rcSummary = m_rcTimeline;
	m_rcSummary.top = m_rcTimeline.bottom;
	m_rcSummary.bottom = m_rcSummary.top + 8;

	SetHorizontalExtent( m_scrollMin,m_scrollMax );

	if (m_tooltip.m_hWnd)
	{
		m_tooltip.DelTool(this,1);
		m_tooltip.AddTool( this,"",m_rcClient,1 );
	}
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewDopeSheetBase::PreTranslateMessage(MSG* pMsg)
{
	if (!m_tooltip.m_hWnd)
	{
		CRect rc;
		GetClientRect(rc);
		m_tooltip.Create( this );
		m_tooltip.SetDelayTime( TTDT_AUTOPOP,5000 );
		m_tooltip.SetDelayTime( TTDT_INITIAL,0 );
		m_tooltip.SetDelayTime( TTDT_RESHOW,0 );
		m_tooltip.SetMaxTipWidth(600);
		m_tooltip.AddTool( this,"",rc,1 );
		m_tooltip.Activate(FALSE);
	}
	m_tooltip.RelayEvent(pMsg);

	return __super::PreTranslateMessage(pMsg);
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewDopeSheetBase::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	//float z = m_timeScale + (zDelta/120.0f) * 1.0f;
	float z;
	if (zDelta>0)
		z = m_timeScale * 1.25f;
	else
		z = m_timeScale * 0.8f;

	GetCursorPos(&pt);
	ScreenToClient(&pt);
	
	float fAnchorTime = TimeFromPointUnsnapped(pt);
	SetTimeScale(z,fAnchorTime);
	return 1;
	//return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO si;
	GetScrollInfo( SB_HORZ,&si );

	// Get the minimum and maximum scroll-bar positions.
	int minpos = si.nMin;
	int maxpos = si.nMax;
	int nPage = si.nPage;

	// Get the current position of scroll box.
	int curpos = si.nPos;

	// Determine the new position of scroll box.
	switch (nSBCode)
	{
	case SB_LEFT:      // Scroll to far left.
		curpos = minpos;
		break;

	case SB_RIGHT:      // Scroll to far right.
		curpos = maxpos;
		break;

	case SB_ENDSCROLL:   // End scroll.
		break;

	case SB_LINELEFT:      // Scroll left.
		if (curpos > minpos)
			curpos--;
		break;

	case SB_LINERIGHT:   // Scroll right.
		if (curpos < maxpos)
			curpos++;
		break;

	case SB_PAGELEFT:    // Scroll one page left.
		if (curpos > minpos)
			curpos = max(minpos, curpos - (int)nPage);
		break;

	case SB_PAGERIGHT:      // Scroll one page right.
		if (curpos < maxpos)
			curpos = min(maxpos, curpos + (int)nPage);
		break;

	case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
		curpos = nPos;      // of the scroll box at the end of the drag operation.
		break;

	case SB_THUMBTRACK:   // Drag scroll box to specified position. nPos is the
		curpos = nPos;     // position that the scroll box has been dragged to.
		break;
	}

	// Set the new position of the thumb (scroll box).
	SetScrollPos( SB_HORZ,curpos );

	m_scrollOffset.x = curpos;
	Invalidate();

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::TickSnap( float time ) const
{
	double t = floor( (double)time*m_ticksStep + 0.5);
	t = t / m_ticksStep;
	return t;
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::TimeFromPoint( CPoint point ) const
{
	int x = point.x - m_leftOffset + m_scrollOffset.x;
	double t = (double)x / m_timeScale;
	return (float)TickSnap(t);
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::TimeFromPointUnsnapped( CPoint point ) const
{
	int x = point.x - m_leftOffset + m_scrollOffset.x;
	double t = (double)x / m_timeScale;
	return t;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::AddItem( const Item &item )
{
	m_tracks.push_back(item);
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
const CTrackViewDopeSheetBase::Item& CTrackViewDopeSheetBase::GetItem( int item ) const
{
	return m_tracks[item];
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CTrackViewDopeSheetBase::GetTrack( int item ) const
{
	if (item < 0 || item >= GetCount())
		return 0;
	IAnimTrack *track = m_tracks[item].track;
	return track;
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CTrackViewDopeSheetBase::GetNode( int item ) const
{
	if (item < 0 || item >= GetCount())
		return 0;
	return m_tracks[item].node;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnLButtonDown(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonDown(nFlags, point);

	HideKeyPropertyCtrlOnSpot();

	SetFocus();

	m_mouseDownPos = point;

	//int item = ItemFromPoint(point);
	//SetCurSel(item);

	if (m_rcTimeline.PtInRect(point))
	{
		// Clicked inside timeline.
		m_mouseMode = MOUSE_MODE_DRAGTIME;
		// If mouse over selected key, change cursor to left-right arrows.
		SetMouseCursor( m_crsLeftRight );
		SetCapture();
		
		SetCurrTime( TimeFromPoint(point) );
		return;
	}

	if(m_bEditLock)
		return;

	if (m_mouseMode == MOUSE_MODE_PASTE_DRAGTIME)
	{
		GetIEditor()->AcceptUndo( "Paste Keys" );
		SetMouseCursor(NULL);
		ReleaseCapture();
		m_mouseMode = MOUSE_MODE_NONE;
		return;
	}

	// The summary region is used for moving already selected keys.
	if (m_rcSummary.PtInRect(point) && m_bAnySelected)
	{
		/// Move/Clone Key Undo Begin
		GetIEditor()->BeginUndo();

		m_keyTimeOffset = 0;
		m_mouseMode = MOUSE_MODE_MOVE;
		SetMouseCursor( m_crsLeftRight );
		return;
	}

	bool bStart = false;
	int key = CheckCursorOnStartEndTimeAdjustBar(point, bStart);
	if (key >= 0)
	{
		return LButtonDownOnTimeAdjustBar(point, key, bStart);
	}

	key = FirstKeyFromPoint(point);
	if (key < 0)
		key = DurationKeyFromPoint(point);
	if (key >= 0)
	{
		return LButtonDownOnKey(point, key, nFlags);
	}
	
	if (m_mouseActionMode == TVMODE_ADDKEY)
	{
		// Add key here.
		int item = ItemFromPoint(point);
		IAnimTrack *track = GetTrack(item);
		if (track
		&& track->GetSubTrackCount() == 0)	// You can't make a key on the compound track.
		{
			float keyTime = TimeFromPoint(point);
			if (IsOkToAddKeyHere(track, keyTime))
			{
				RecordTrackUndo( GetItem(item) );
				track->CreateKey( keyTime );
				Invalidate();
				UpdateAnimation();
			}
		}
		return;
	}
	
	if (nFlags & MK_SHIFT)
		m_mouseMode = MOUSE_MODE_SELECT_WITHIN_TIME;
	else
		m_mouseMode = MOUSE_MODE_SELECT;
	SetCapture();
	if (m_bAnySelected && !(nFlags & MK_CONTROL))
	{
		// First unselect all buttons.
		UnselectAllKeys(true);
		Invalidate();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_mouseMode == MOUSE_MODE_SELECT)
	{
		bool prevSelected = m_bAnySelected;
		// Check if any key are selected.
		m_rcSelect-=m_scrollOffset;
		SelectKeys( m_rcSelect );
		NotifyKeySelectionUpdate();
		/*
		if (prevSelected == m_bAnySelected)
		Invalidate();
		else
		{
		CDC *dc = GetDC();
		dc->DrawDragRect( CRect(0,0,0,0),CSize(0,0),m_rcSelect,CSize(1,1) );
		ReleaseDC(dc);
		}
		*/
		Invalidate();
		m_rcSelect = CRect(0,0,0,0);
	}
	else if (m_mouseMode == MOUSE_MODE_SELECT_WITHIN_TIME)
	{
		m_rcSelect-=m_scrollOffset;
		SelectAllKeysWithinTimeFrame( m_rcSelect );
		NotifyKeySelectionUpdate();
		Invalidate();
		m_rcSelect = CRect(0,0,0,0);
	}
	else if (m_mouseMode == MOUSE_MODE_DRAGTIME)
	{
		SetMouseCursor(NULL);
	}
	else if (m_mouseMode == MOUSE_MODE_PASTE_DRAGTIME)
	{
		GetIEditor()->AcceptUndo( "Paste Keys" );
		SetMouseCursor(NULL);
	}

	if (GetCapture() == this)
	{
		ReleaseCapture();
	}

	if (m_pSequence == NULL)
	{
		m_bAnySelected = false;
	}
		
	m_keyTimeOffset = 0;
	m_keyForTimeAdjust = -1;
	m_pTrackForTimeAdjust = NULL;

	//if (GetIEditor()->IsUndoRecording())
	//GetIEditor()->AcceptUndo( "Track Modify" );

	/// Move/Clone Key Undo End
	if(m_mouseMode == MOUSE_MODE_MOVE || m_mouseMode == MOUSE_MODE_CLONE)
		GetIEditor()->AcceptUndo("Move/Clone Keys");
	else if(m_mouseMode == MOUSE_MODE_START_TIME_ADJUST 
		|| m_mouseMode == MOUSE_MODE_END_TIME_ADJUST)
		GetIEditor()->AcceptUndo("Adjust Start/End Time of an Animation Key");

	m_mouseMode = MOUSE_MODE_NONE;
	CWnd::OnLButtonUp(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_rcTimeline.PtInRect(point))
	{
		// Clicked inside timeline.
		return;
	}

	if(m_bEditLock)
		return;

	int key = FirstKeyFromPoint(point);
	if (key < 0)
		key = DurationKeyFromPoint(point);
	if (key >= 0)
	{
		int item = ItemFromPoint(point);
		IAnimTrack *track = GetTrack(item);
		UnselectAllKeys(false);
		track->SelectKey(key, true);
		m_bAnySelected = true;
		m_keyTimeOffset = 0;
		Invalidate();

		NotifyKeySelectionUpdate();

		CPoint p;
		::GetCursorPos(&p);

		bool bKeyChangeInSameTrack
			= m_pLastTrackSelectedOnSpot
				&& track == m_pLastTrackSelectedOnSpot;
		m_pLastTrackSelectedOnSpot = track;

		ShowKeyPropertyCtrlOnSpot(p.x, p.y, false, bKeyChangeInSameTrack);
		return;
	}

	// Add key here.
	int item = ItemFromPoint(point);
	IAnimTrack *track = GetTrack(item);
	IAnimNode *node = GetNode(item);
	float keyTime = TimeFromPoint(point);
	bool inRange = m_timeRange.IsInside(keyTime);
	if (track && inRange) 
	{
		bool keyCreated = false;
		if ((nFlags & MK_SHIFT) && node->GetParent())		// Add keys in group
		{
			CUndo undo("Add Keys in Group");
			CUndo::Record( new CUndoAnimSequenceObject(m_pSequence) );

			CTrackViewUtils::SelectedTracks tracksInGroup;
			CTrackViewUtils::GetTracksInGroup(m_pSequence, node->GetParent(), track->GetParameterType(), tracksInGroup);
			for(int i=0; i<(int)tracksInGroup.tracks.size(); ++i)
			{
				IAnimTrack *pCurrTrack = tracksInGroup.tracks[i].pTrack;

				if (pCurrTrack->GetSubTrackCount() == 0)	// A simple track
				{
					if (IsOkToAddKeyHere(pCurrTrack, keyTime))
					{
						pCurrTrack->CreateKey( keyTime );
						keyCreated = true;
					}			
				}
				else																			// A compound track
				{
					for (int k=0; k<pCurrTrack->GetSubTrackCount(); ++k)
					{
						IAnimTrack *subTrack = pCurrTrack->GetSubTrack(k);
						if (IsOkToAddKeyHere(subTrack, keyTime))
						{
							subTrack->CreateKey( keyTime );
							keyCreated = true;
						}
					}
				}
			}
		}
		else if (track->GetSubTrackCount() == 0)			// A simple track
		{
			if (IsOkToAddKeyHere(track, keyTime))
			{
				RecordTrackUndo( GetItem(item) );
				track->CreateKey( keyTime );
				keyCreated = true;
			}
		}
		else																				// A compound track
		{
			RecordTrackUndo( GetItem(item) );
			for (int i=0; i<track->GetSubTrackCount(); ++i)
			{
				IAnimTrack *subTrack = track->GetSubTrack(i);
				if (IsOkToAddKeyHere(subTrack, keyTime))
				{
					subTrack->CreateKey( keyTime );
					keyCreated = true;
				}
			}
		}
		if (keyCreated)
		{
			Invalidate();
			UpdateAnimation();
		}
	}
	
	m_mouseMode = MOUSE_MODE_NONE;
	CWnd::OnLButtonDblClk(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnMButtonDown(UINT nFlags, CPoint point)
{
	OnRButtonDown(nFlags,point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnMButtonUp(UINT nFlags, CPoint point)
{
	OnRButtonUp(nFlags,point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnRButtonDown(UINT nFlags, CPoint point)
{
	//CWnd::OnRButtonDown(nFlags, point);

	HideKeyPropertyCtrlOnSpot();

	m_bCursorWasInKey = false;
	m_bMouseMovedAfterRButtonDown = false;

	SetFocus();

	if (m_rcTimeline.PtInRect(point))
	{
		// Clicked inside timeline.
		// adjust markers.
		int nMarkerStart=TimeToClient(m_timeMarked.start);
		int nMarkerEnd=TimeToClient(m_timeMarked.end);
		if ((abs(point.x-nMarkerStart))<(abs(point.x-nMarkerEnd)))
		{
			SetStartMarker(TimeFromPoint(point));
			m_mouseMode = MOUSE_MODE_DRAGSTARTMARKER;
		}else
		{
			SetEndMarker(TimeFromPoint(point));
			m_mouseMode = MOUSE_MODE_DRAGENDMARKER;
		}
		SetCapture();
		return;
	}

	//int item = ItemFromPoint(point);
	//SetCurSel(item);

	m_mouseDownPos = point;

	if (nFlags & MK_SHIFT)	// alternative zoom
	{
		m_bZoomDrag=true;
		SetCapture();
		return;
	}

	int key = FirstKeyFromPoint(point);
	if (key < 0)
		key = DurationKeyFromPoint(point);
	if (key >= 0)
	{
		m_bCursorWasInKey = true;

		int item = ItemFromPoint(point);
		IAnimTrack *track = GetTrack(item);
		track->SelectKey(key, true);
		m_bAnySelected = true;
		m_keyTimeOffset = 0;
		Invalidate();

		NotifyKeySelectionUpdate();

		// Show a little pop-up menu for copy & delete.
		enum { COPY_KEY=100, DELETE_KEY, EDIT_KEY_ON_SPOT };
		CMenu menu;
		menu.CreatePopupMenu();
		bool bEnableEditOnSpot = false;
		CTrackViewUtils::SelectedKeys selectedKeys;
		CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
		if (selectedKeys.keys.size() > 0 && selectedKeys.bAllOfSameType)
			bEnableEditOnSpot = true;
		menu.AppendMenu(bEnableEditOnSpot ? MF_STRING : MF_STRING|MF_GRAYED, 
										EDIT_KEY_ON_SPOT, "Edit On Spot");
		menu.AppendMenu(MF_SEPARATOR, 0 ,"");
		menu.AppendMenu(MF_STRING, COPY_KEY, "Copy");
		menu.AppendMenu(MF_SEPARATOR, 0 ,"");
		menu.AppendMenu(MF_STRING, DELETE_KEY, "Delete");

		CPoint p;
		::GetCursorPos(&p);
		int cmd = menu.TrackPopupMenu(TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_LEFTBUTTON, p.x,p.y, this, NULL);
		if (cmd == EDIT_KEY_ON_SPOT)
		{	
			bool bKeyChangeInSameTrack
				= m_pLastTrackSelectedOnSpot
					&& selectedKeys.keys.size() == 1 
					&& selectedKeys.keys[0].pTrack == m_pLastTrackSelectedOnSpot;

			if (selectedKeys.keys.size() == 1)
				m_pLastTrackSelectedOnSpot = selectedKeys.keys[0].pTrack;
			else
				m_pLastTrackSelectedOnSpot = NULL;

			ShowKeyPropertyCtrlOnSpot(p.x, p.y, selectedKeys.keys.size() > 1, bKeyChangeInSameTrack);
		}
		else if(cmd == COPY_KEY)
			CopyKeys();
		else if(cmd == DELETE_KEY)
			DelSelectedKeys();
	}else
	{
		m_bMoveDrag=true;
		SetCapture();
	}
}

void CTrackViewDopeSheetBase::OnRButtonUp(UINT nFlags, CPoint point)
{
	m_bZoomDrag=false;
	m_bMoveDrag=false;

	if (GetCapture() == this)
	{
		ReleaseCapture();
	}
	m_mouseMode = MOUSE_MODE_NONE;

	CClipboard clip;
	if(m_bCursorWasInKey == false && clip.GetTitle() == "Track view keys")
	{
		bool bHasCopiedKey = true;
		
		if(clip.IsEmpty())
			bHasCopiedKey = false;
		
		{
			XmlNodeRef copyNode = clip.Get();
			if(copyNode==NULL || strcmp(copyNode->getTag(), "CopyKeysNode"))
				bHasCopiedKey = false;
			else
			{
				int nNumTracksToPaste = copyNode->getChildCount();
				if(nNumTracksToPaste==0)
					bHasCopiedKey = false;
			}
		}
		if(bHasCopiedKey 
		&& m_bMouseMovedAfterRButtonDown == false)	// Once moved, it means the user wanted to scroll, so no paste pop-up.
		{
			// Show a little pop-up menu for paste.
			enum { PASTE_KEY=100 };
			CMenu menu;
			menu.CreatePopupMenu();
			menu.AppendMenu(MF_STRING, PASTE_KEY, "Paste");

			CPoint p;
			::GetCursorPos(&p);
			int cmd = menu.TrackPopupMenu(TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_LEFTBUTTON, p.x,p.y, this, NULL);
			if(cmd == PASTE_KEY)
				StartPasteKeys();
		}
	}

	//	CWnd::OnRButtonUp(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnMouseMove(UINT nFlags, CPoint point)
{
	// To prevent the key moving while selecting
	if (m_bJustSelected)
	{
		m_bJustSelected = false;
		return;
	}

	m_bMouseMovedAfterRButtonDown = true;

	m_mouseOverPos = point;
	m_bMouseOverKey = false;
	if (m_bZoomDrag && (nFlags & MK_SHIFT))
	{
		float fAnchorTime = TimeFromPointUnsnapped(m_mouseDownPos);
		SetTimeScale(m_timeScale*(1.0f+(point.x-m_mouseDownPos.x)*0.0025f),fAnchorTime);
		m_mouseDownPos=point;
		return;
	}
	else
		m_bZoomDrag=false;
	if (m_bMoveDrag)
	{
		m_scrollOffset.x+= m_mouseDownPos.x-point.x;
		if (m_scrollOffset.x < m_scrollMin)
			m_scrollOffset.x = m_scrollMin;
		if (m_scrollOffset.x > m_scrollMax)
			m_scrollOffset.x = m_scrollMax;
		m_mouseDownPos = point;
		// Set the new position of the thumb (scroll box).
		SetScrollPos( SB_HORZ,m_scrollOffset.x );
		Invalidate();
		SetMouseCursor( m_crsLeftRight );
		return;
	}

	if (m_mouseMode == MOUSE_MODE_SELECT
	|| m_mouseMode == MOUSE_MODE_SELECT_WITHIN_TIME)
	{
		MouseMoveSelect(point);
	}
	else if (m_mouseMode == MOUSE_MODE_MOVE)
	{
		MouseMoveMove(point, nFlags);
	}
	else if (m_mouseMode == MOUSE_MODE_CLONE)
	{
		CloneSelectedKeys();
		m_mouseMode = MOUSE_MODE_MOVE;
	}
	else if (m_mouseMode == MOUSE_MODE_DRAGTIME)
	{
		MouseMoveDragTime(point, nFlags);
	}
	else if (m_mouseMode == MOUSE_MODE_DRAGSTARTMARKER)
	{
		MouseMoveDragStartMarker(point, nFlags);
	}
	else if (m_mouseMode == MOUSE_MODE_DRAGENDMARKER)
	{
		MouseMoveDragEndMarker(point, nFlags);
	}
	else if (m_mouseMode == MOUSE_MODE_PASTE_DRAGTIME)
	{
		MouseMovePaste(point, nFlags);
	}
	else if (m_mouseMode == MOUSE_MODE_START_TIME_ADJUST)
	{
		MouseMoveStartEndTimeAdjust(point, true);
	}
	else if (m_mouseMode == MOUSE_MODE_END_TIME_ADJUST)
	{
		MouseMoveStartEndTimeAdjust(point, false);
	}
	else
	{
		//////////////////////////////////////////////////////////////////////////
		if (m_mouseActionMode == TVMODE_ADDKEY)
		{
			SetMouseCursor(m_crsAddKey);
		}
		else
		{
			MouseMoveOver(point);
		}
	}

	CWnd::OnMouseMove(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnPaint()
{
//	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
//	CWnd::OnPaint();

	CPaintDC  PaintDC(this);
	
	CMemDC dc( PaintDC,&m_offscreenBitmap );

	XTPPaintManager()->GradientFill( &dc,&PaintDC.m_ps.rcPaint,RGB(250,250,250),RGB(220,220,220),FALSE );

	if(m_bEditLock)
	{
		CBrush brushGray(EDIT_DISABLE_GRAY_COLOR);
		dc->FillRect( &PaintDC.m_ps.rcPaint,&brushGray );
	}
	
	DrawControl( dc,PaintDC.m_ps.rcPaint );

	/*
	CRect rc = m_rcClient;
	for (int i = 0; i < GetCount(); i++)
	{
		IAnimTrack *track = GetTrack(i);
		if (!track)
			continue;
		
		float xoffset = 0;
		int y = (m_rcClient.bottom+m_rcClient.top)/2;
		dc->MoveTo( m_rcClient.left,y );
		// Draw first track spline.
		for (int x = m_rcClient.left; x < m_rcClient.right; x++)
		{
			float time = TimeFromPointUnsnapped(CPoint(x,y));
			Vec3 val;
			track->GetValue( time,val );
			if (x == m_rcClient.left)
				xoffset = val.x;
			dc->LineTo(x,y + val.x - xoffset);
		}
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawTrack( int item,CDC *dc,CRect &rcItem )
{
}


//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawControl( CDC *dc,const CRect &rcUpdate )
{
	CRect rc;
	CRect rcTemp;

	// Draw all items.
	int count = GetCount();
	for (int i = 0; i < count; i++)
	{
		GetItemRect( i,rc );
		//if (rcTemp.IntersectRect(rc,rcUpdate) != 0)
		{
			DrawTrack( i,dc,rc );
		}
	}

	DrawTimeline( dc,rcUpdate );

	DrawSummary( dc,rcUpdate );

	DrawSelectedKeyIndicators(dc);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::UnselectAllKeys( bool bNotify )
{
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys( m_pSequence,selectedKeys );
	for (int i = 0; i < selectedKeys.keys.size(); ++i)
	{
		IAnimTrack *pTrack = selectedKeys.keys[i].pTrack;
		int keyIndex = selectedKeys.keys[i].nKey;
		pTrack->SelectKey(keyIndex, false);
	}
	m_bAnySelected = false;

	if (bNotify)
		NotifyKeySelectionUpdate();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SelectAllKeysWithinTimeFrame( const CRect &rc )
{
	if (m_pSequence == NULL)
		return;

	// put selection rectangle from client to item space.
	CRect rci = rc;
	rci.OffsetRect( m_scrollOffset );

	Range selTime = GetTimeRange( rci );

	for (int k = 0; k < m_pSequence->GetNodeCount(); ++k)
	{
		IAnimNode *node = m_pSequence->GetNode(k);

		for (int i = 0; i < node->GetTrackCount(); i++)
		{
				IAnimTrack *track = node->GetTrackByIndex(i);

				// Check which keys we intersect.
				for (int j = 0; j < track->GetNumKeys(); j++)
				{
					float time = track->GetKeyTime(j);
					if (selTime.IsInside(time))
					{
						track->SelectKey(j, true);
						m_bAnySelected = true;
					}
				}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DelSelectedKeys()
{
	// Cofirm.
	if (AfxMessageBox("Delete selected keys?",MB_OKCANCEL|MB_ICONQUESTION ) != IDOK)
		return;

	if (m_pSequence)
	{
		CUndo undo("Delete Keys");
		CUndo::Record( new CUndoAnimSequenceObject(m_pSequence) );
	}

	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
	for (int k = (int)selectedKeys.keys.size()-1; k>=0; --k)
	{
		CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];
		skey.pTrack->RemoveKey(skey.nKey);
	}
	Invalidate();
	UpdateAnimation();
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::ClipTimeOffsetForSliding( float timeOffset ) const
{
	CTrackViewUtils::SelectedKeys keys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, keys);
	std::set<IAnimTrack*> tracks;
	std::set<IAnimTrack*>::const_iterator trackIter;
	Range timeRange = m_pSequence->GetTimeRange();

	// Get the first key in the timeline among selected and
	// also gather tracks.
	float time0 = timeRange.end;
	for (int k=0; k<(int)keys.keys.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = keys.keys[k];
		tracks.insert(skey.pTrack);
		float keyTime = skey.pTrack->GetKeyTime(skey.nKey);
		if (keyTime < time0)
			time0 = keyTime;
	}

	// If 'bAll' is true, slide all tracks.
	// (Otherwise, slide only selected tracks.)
	bool bAll = GetAsyncKeyState(VK_MENU);
	if (bAll)
	{
		CTrackViewUtils::GetKeysInTimeRange(m_pSequence, keys, time0, timeRange.end);
		// Gather tracks again.
		tracks.clear();
		for (int k=0; k<(int)keys.keys.size(); ++k)
		{
			CTrackViewUtils::SelectedKey skey = keys.keys[k];
			tracks.insert(skey.pTrack);
		}
	}

	float newTimeOffset = timeOffset;
	for (trackIter = tracks.begin(); trackIter != tracks.end(); ++trackIter)
	{
		IAnimTrack* pTrack = *trackIter;
		for (int i=0; i<pTrack->GetNumKeys(); ++i)
		{
			float keyTime = pTrack->GetKeyTime(i);
			if (keyTime >= time0)
			{
				float newKeyTime = keyTime + timeOffset;
				m_timeRange.ClipValue(newKeyTime);
				float offset = newKeyTime - keyTime;
				if (fabs(offset) < fabs(newTimeOffset))
					newTimeOffset = offset;
			}
		}
	}

	return newTimeOffset;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SlideKeys( float timeOffset )
{
	CTrackViewUtils::SelectedKeys keys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, keys);
	std::set<IAnimTrack*> tracks;
	std::set<IAnimTrack*>::const_iterator trackIter;
	Range timeRange = m_pSequence->GetTimeRange();

	// Get the first key in the timeline among selected and
	// also gather tracks.
	float time0 = timeRange.end;
	for (int k=0; k<(int)keys.keys.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = keys.keys[k];
		tracks.insert(skey.pTrack);
		float keyTime = skey.pTrack->GetKeyTime(skey.nKey);
		if (keyTime < time0)
			time0 = keyTime;
	}
	
	// If 'bAll' is true, slide all tracks.
	// (Otherwise, slide only selected tracks.)
	bool bAll = GetAsyncKeyState(VK_MENU);
	if (bAll)
	{
		CTrackViewUtils::GetKeysInTimeRange(m_pSequence, keys, time0, timeRange.end);
		// Gather tracks again.
		tracks.clear();
		for (int k=0; k<(int)keys.keys.size(); ++k)
		{
			CTrackViewUtils::SelectedKey skey = keys.keys[k];
			tracks.insert(skey.pTrack);
		}
	}

	for (trackIter = tracks.begin(); trackIter != tracks.end(); ++trackIter)
	{
		IAnimTrack* pTrack = *trackIter;
		CUndo::Record(new CUndoTrackObject(pTrack, m_pSequence));
		SlideKey(pTrack, time0, timeOffset);
	}

	UpdateAnimation();
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::ClipTimeOffsetForOffsetting( float timeOffset ) const
{
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
	float newTimeOffset = timeOffset;
	for (int k = 0; k<(int)selectedKeys.keys.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];
		float keyTime = skey.pTrack->GetKeyTime(skey.nKey);
		float newKeyTime = keyTime + timeOffset;
		
		Range extendedTimeRange( 0.0f, m_timeRange.end );
		extendedTimeRange.ClipValue( newKeyTime );
				
		float offset =  newKeyTime - keyTime;
		if (fabs(offset) < fabs(newTimeOffset))
			newTimeOffset = offset;
	}

	return newTimeOffset;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OffsetSelectedKeys( float timeOffset )
{
	IAnimTrack *pTrack = 0;
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
	for (int k = 0; k<(int)selectedKeys.keys.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];
		if(pTrack != skey.pTrack)
		{
			CUndo::Record( new CUndoTrackObject(skey.pTrack, m_pSequence) );
			pTrack = skey.pTrack;
		}

		OffsetKey(skey.pTrack, skey.nKey, timeOffset);
	}

	UpdateAnimation();
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::ClipTimeOffsetForScaling( float timeOffset ) const
{
	if (timeOffset <= 0)
		return timeOffset;
	
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
	float newTimeOffset = timeOffset;
	for (int k = 0; k<(int)selectedKeys.keys.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];
		float keyTime = skey.pTrack->GetKeyTime(skey.nKey);
		float newKeyTime = keyTime * timeOffset;
		m_timeRange.ClipValue(newKeyTime);
		float offset = newKeyTime / keyTime;
		if (offset < newTimeOffset)
			newTimeOffset = offset;
	}

	return newTimeOffset;
}
//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::ScaleSelectedKeys( float timeOffset )
{
	if (timeOffset <= 0)
		return;
	IAnimTrack *pTrack = 0;
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
	for (int k = 0; k<(int)selectedKeys.keys.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];
		if(pTrack != skey.pTrack)
		{
			CUndo::Record( new CUndoTrackObject(skey.pTrack, m_pSequence) );
			pTrack = skey.pTrack;
		}
		
		float keyt = skey.pTrack->GetKeyTime(skey.nKey) * timeOffset;
		skey.pTrack->SetKeyTime( skey.nKey,keyt );
	}
	
	UpdateAnimation();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::CloneSelectedKeys()
{
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
	IAnimTrack *pTrack = 0;
	// In case of multiple cloning, indices cannot be used as a solid pointer to the original.
	// So use the time of keys as an identifier, instead.
	std::vector<float> selectedKeyTimes;
	for (size_t k = 0; k<selectedKeys.keys.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];
		if(pTrack != skey.pTrack)
		{
			CUndo::Record( new CUndoTrackObject(skey.pTrack, m_pSequence) );
			pTrack = skey.pTrack;
		}

		selectedKeyTimes.push_back(skey.pTrack->GetKeyTime(skey.nKey));
	}

	// Now, do the actual cloning.
	for (size_t k = 0; k<selectedKeyTimes.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];

		int keyIndex = skey.pTrack->FindKey(selectedKeyTimes[k]);
		assert(keyIndex > -1);
		if (keyIndex <= -1)
			continue;

		int newKey = skey.pTrack->CloneKey(keyIndex);

		// Select new key.
		skey.pTrack->SelectKey(newKey, true);
		// Unselect cloned key.
		skey.pTrack->SelectKey(keyIndex, false);
	}
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewDopeSheetBase::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_currCursor != NULL)
	{
		return 0;
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CTrackViewDopeSheetBase::SetMouseCursor( HCURSOR crs )
{
	m_currCursor = crs;
	if (m_currCursor != NULL)
		SetCursor(crs);
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewDopeSheetBase::OnEraseBkgnd(CDC* pDC)
{
	//return CWnd::OnEraseBkgnd(pDC);
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::GetCurrTime() const
{
	return m_currentTime;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetCurrTime( float time )
{
	if (time < m_timeRange.start)
		time = m_timeRange.start;
	if (time > m_timeRange.end)
		time = m_timeRange.end;

	//bool bChange = fabs(time-m_currentTime) >= (1.0f/m_ticksStep);
	bool bChange = fabs(time-m_currentTime) >= 0.001f;

	if (bChange)
	{
		int x1 = TimeToClient(m_currentTime);
		int x2 = TimeToClient(time);
		m_currentTime = time;
		//Invalidate();


		CRect rc(x1-3,m_rcClient.top,x1+4,m_rcClient.bottom);
		RedrawWindow( rc,NULL,RDW_INVALIDATE );
		CRect rc1(x2-3,m_rcClient.top,x2+4,m_rcClient.bottom);
		RedrawWindow( rc1,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE );
		GetIEditor()->GetAnimation()->SetTime( m_currentTime );
	}
}

void CTrackViewDopeSheetBase::SetStartMarker(float fTime)
{
	m_timeMarked.start=fTime;
	if (m_timeMarked.start<m_timeRange.start)
		m_timeMarked.start=m_timeRange.start;
	if (m_timeMarked.start>m_timeRange.end)
		m_timeMarked.start=m_timeRange.end;
	if (m_timeMarked.start>m_timeMarked.end)
		m_timeMarked.end=m_timeMarked.start;
	GetIEditor()->GetAnimation()->SetMarkers(m_timeMarked);
	Invalidate();
}

void CTrackViewDopeSheetBase::SetEndMarker(float fTime)
{
	m_timeMarked.end=fTime;
	if (m_timeMarked.end<m_timeRange.start)
		m_timeMarked.end=m_timeRange.start;
	if (m_timeMarked.end>m_timeRange.end)
		m_timeMarked.end=m_timeRange.end;
	if (m_timeMarked.start>m_timeMarked.end)
		m_timeMarked.start=m_timeMarked.end;
	GetIEditor()->GetAnimation()->SetMarkers(m_timeMarked);
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetMouseActionMode( ETVActionMode mode )
{
	m_mouseActionMode = mode;
	if (mode == TVMODE_ADDKEY)
	{
		SetMouseCursor( m_crsAddKey );
	}
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewDopeSheetBase::CanCopyPasteKeys()
{
	IAnimTrack *pCopyFromTrack=NULL;
	// are all selected keys from the same source track ?
	if (!m_bAnySelected)
		return false;
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
	for (int k = 0; k<(int)selectedKeys.keys.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];
		if (!pCopyFromTrack)
		{
			pCopyFromTrack=skey.pTrack;
		}
		else
		{
			if (pCopyFromTrack!=skey.pTrack)
				return false;
		}
	}
	if (!pCopyFromTrack)
		return false;

	// is a destination-track selected ?
	CTrackViewUtils::SelectedTracks selectedTracks;
	CTrackViewUtils::GetSelectedTracks( m_pSequence,selectedTracks );
	if (selectedTracks.tracks.size() != 1)
		return false;
	IAnimTrack *pCurrTrack = selectedTracks.tracks[0].pTrack;
	if (!pCurrTrack)
		return false;
	if (pCurrTrack->GetType()!=pCopyFromTrack->GetType())
		return false;
	return (pCopyFromTrack==pCurrTrack);
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewDopeSheetBase::CopyPasteKeys()
{
	IAnimTrack *pCopyFromTrack=NULL;
	std::vector<int> vecKeysToCopy;
	if (!CanCopyPasteKeys())
		return false;
	if (!m_bAnySelected)
		return false;
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
	for (int k = 0; k<(int)selectedKeys.keys.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];
		pCopyFromTrack=skey.pTrack;
		vecKeysToCopy.push_back(skey.nKey);
	}
	if (!pCopyFromTrack)
		return false;

	// Get Selected Track.
	IAnimTrack *pCurrTrack = 0;
	CTrackViewUtils::SelectedTracks selectedTracks;
	CTrackViewUtils::GetSelectedTracks( m_pSequence,selectedTracks );
	if (selectedTracks.tracks.size() == 1)
	{
		pCurrTrack = selectedTracks.tracks[0].pTrack;
	}

	if (!pCurrTrack)
		return false;
	for (int i=0;i<(int)vecKeysToCopy.size();i++)
	{
		pCurrTrack->CopyKey(pCopyFromTrack, vecKeysToCopy[i]);
	}
	Invalidate();
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewDopeSheetBase::FindSingleSelectedKey( IAnimTrack* &selTrack,int &selKey )
{
	selTrack = 0;
	selKey = 0;
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
	if (selectedKeys.keys.size() != 1)
		return false;
	CTrackViewUtils::SelectedKey skey = selectedKeys.keys[0];
	selTrack = skey.pTrack;
	selKey = skey.nKey;
	return true;
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewDopeSheetBase::GetItemRect( int item,CRect &rect ) const
{
	if (item < 0 || item >= GetCount())
		return -1;

	int y = -m_scrollOffset.y;

	int x = 0;
	for (int i = 0,num = (int)m_tracks.size(); i < num && i < item; i++)
	{
		y += m_tracks[i].nHeight;
	}
	//int y = item*m_itemHeight - m_scrollOffset.y;
	rect.SetRect( x,y,x+m_rcClient.Width(),y + m_tracks[item].nHeight );
	return 0;
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewDopeSheetBase::ItemFromPoint( CPoint pnt ) const
{
	CRect rc;
	int num = GetCount();
	for (int i = 0; i < num; i++)
	{
		GetItemRect(i,rc);
		if (rc.PtInRect(pnt))
			return i;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetHorizontalExtent( int min,int max )
{
	m_scrollMin = min;
	m_scrollMax = max;
	m_itemWidth = max - min;
	int nPage = m_rcClient.Width()/2;
	int sx = m_itemWidth - nPage + m_leftOffset;

	SCROLLINFO si;
	ZeroStruct(si);
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = m_scrollMin;
	si.nMax = m_scrollMax - nPage + m_leftOffset;
	si.nPage = m_rcClient.Width()/2;
	si.nPos = m_scrollOffset.x;
	//si.nPage = max(0,m_rcClient.Width() - m_leftOffset*2);
	//si.nPage = 1;
	//si.nPage = 1;
	SetScrollInfo( SB_HORZ,&si,TRUE );
};

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::UpdateAnimation()
{
	GetIEditor()->GetAnimation()->ForceAnimation();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewDopeSheetBase::CopyKeys()
{
	bool bCopySelected=false;
	bool bIsSeveralTracks = false;
	bool bIsSeveralEntity = false;

	IAnimTrack *pCopyFromTrack=0;
	IAnimNode * pCurNode = 0;

	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
	for (int k = 0; k<(int)selectedKeys.keys.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];
		if(pCopyFromTrack && pCopyFromTrack!=skey.pTrack)
			bIsSeveralTracks=true;

		pCopyFromTrack=skey.pTrack;
		bCopySelected=true;

		if(pCurNode && pCurNode!=skey.pNode)
			bIsSeveralEntity=true;
		pCurNode=skey.pNode;
	}

	if(bIsSeveralEntity)
	{
		AfxMessageBox("Copying aborted. You should select keys in a single node only.", MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	// Get Selected Track.
	CTrackViewUtils::SelectedTracks selectedTracks;
	CTrackViewUtils::GetSelectedTracks( m_pSequence,selectedTracks );
	if (!bCopySelected 
		&& CTrackViewUtils::IsOneTrackSelected(selectedTracks) == false)
		return false;

	int iSelTrackCount = (int)selectedTracks.tracks.size();
	bool bContainOldTCBTrack = false;
	for(int i=0; i< iSelTrackCount; ++i)
	{
		EAnimTrackType trackType = selectedTracks.tracks[i].pTrack->GetType();
		if (trackType == ATRACK_TCB_FLOAT || trackType == ATRACK_TCB_QUAT || trackType == ATRACK_TCB_VECTOR)
		{
			bContainOldTCBTrack = true;
			break;
		}
	}


	if(bContainOldTCBTrack)
	{
		AfxMessageBox("Copying aborted: One of the selected tracks contains obsolete TCB tracks (marked red).", MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	XmlNodeRef copyNode = XmlHelpers::CreateXmlNode("CopyKeysNode");

	if(bCopySelected)
	{
		pCopyFromTrack=0;
		XmlNodeRef childNode;
		CTrackViewUtils::SelectedKeys selectedKeys;
		CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
		for (int k = 0; k<(int)selectedKeys.keys.size(); ++k)
		{
			CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];

			if(pCopyFromTrack!=skey.pTrack)
			{
				childNode = copyNode->newChild("Track");
				childNode->setAttr("paramId", skey.pTrack->GetParameterType());
				childNode->setAttr("paramIndex", skey.pNode->GetTrackParamIndex(skey.pTrack));
				skey.pTrack->SerializeSelection(childNode, false, true);
				pCopyFromTrack=skey.pTrack;
			}
		}
	}
	else
	{
		for(int i=0; i<(int)selectedTracks.tracks.size(); ++i)
		{
			IAnimTrack *pCurrTrack = selectedTracks.tracks[i].pTrack;
			if(!pCurrTrack)
				continue;

			XmlNodeRef childNode = copyNode->newChild("Track");
			childNode->setAttr("paramId", pCurrTrack->GetParameterType() );
			childNode->setAttr("paramIndex", selectedTracks.tracks[i].pNode->GetTrackParamIndex(pCurrTrack));
			if(pCurrTrack->GetNumKeys()>0)
				pCurrTrack->SerializeSelection(childNode, false, false);
		}
	}

	CClipboard clip;
	clip.Put(copyNode, "Track view keys");

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::StartPasteKeys()
{
	if (m_pSequence)
	{
		CUndo undo("Paste Keys");
		CUndo::Record( new CUndoAnimSequenceObject(m_pSequence) );
	}

	m_mouseMode = MOUSE_MODE_PASTE_DRAGTIME;
	// If mouse over selected key, change cursor to left-right arrows.
	SetMouseCursor( m_crsLeftRight );
	SetCapture();
	m_mouseDownPos = m_mouseOverPos;
	GetIEditor()->BeginUndo();

	PasteKeys(0,0,0);
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewDopeSheetBase::PasteKeys( IAnimNode * pAnimNode, IAnimTrack * pAnimTrack, float fTimeOffset )
{

	int nPasteToItem = -1;
	if (!pAnimNode)
	{
		// Find mouse-over item.
		int nPasteToItem = ItemFromPoint( m_mouseOverPos );
		if (nPasteToItem >= 0)
		{
			pAnimNode = GetItem(nPasteToItem).node;
		}
	}

	if (!pAnimNode)
		return false;

	CClipboard clip;
	if(clip.IsEmpty())
		return false;

	XmlNodeRef copyNode = clip.Get();
	if(copyNode == NULL || strcmp(copyNode->getTag(), "CopyKeysNode"))
		return false;

	int nNumTracksToPaste = copyNode->getChildCount();

	if(nNumTracksToPaste==0)
		return false;

	bool bUseGivenTrackIfPossible = false;
	if (nNumTracksToPaste == 1 && pAnimTrack)
		bUseGivenTrackIfPossible = true;

	bool bIsPasted=false;

	bool bSaveUndo = !CUndo::IsRecording();

	if (bSaveUndo)
		GetIEditor()->BeginUndo();

	UnselectAllKeys(false);
	m_bAnySelected = true;

	{
		bool bNewTrackCreated = false;
		for (int i=0; i<copyNode->getChildCount(); i++)
		{
			XmlNodeRef trackNode = copyNode->getChild(i);
			int paramId;
			uint32 paramIndex = 0;
			trackNode->getAttr("paramId", paramId);
			trackNode->getAttr("paramIndex", paramIndex);

			IAnimTrack *pTrack = pAnimNode->GetTrackForParameter(paramId, paramIndex);
			if (bUseGivenTrackIfPossible && pAnimTrack->GetParameterType() == paramId)
				pTrack = pAnimTrack;
			if (!pTrack)
			{
				// Make this track.
				pTrack = pAnimNode->CreateTrack(paramId);
				if (pTrack)
					bNewTrackCreated = true;
			}
			if(pTrack)
			{
				CUndo::Record( new CUndoTrackObject(pTrack, m_pSequence) );
				pTrack->SerializeSelection(trackNode, true,true,fTimeOffset);
			}
		}
		if (bNewTrackCreated)
		{
			GetIEditor()->Notify( eNotify_OnUpdateTrackView );
			NotifyKeySelectionUpdate();
		}
	}

	if (bSaveUndo)
		GetIEditor()->AcceptUndo("Paste keys");

	Invalidate();

	return true;
}





void CTrackViewDopeSheetBase::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_DELETE)
	{
		DelSelectedKeys();
	}

	if (nChar == VK_UP || nChar == VK_DOWN || nChar == VK_RIGHT || nChar == VK_LEFT)
	{
		IAnimTrack *pTrack;
		int key;
		switch (nChar)
		{
		case VK_UP:
			key = GetAboveKey(pTrack);
			break;
		case VK_DOWN:
			key = GetBelowKey(pTrack);
			break;
		case VK_RIGHT:
			key = GetRightKey(pTrack);
			break;
		case VK_LEFT:
			key = GetLeftKey(pTrack);
			break;
		}
		if (key >= 0)
		{
			UnselectAllKeys(false);
			pTrack->SelectKey(key, true);
			Invalidate();
			NotifyKeySelectionUpdate();
		}
		return;
	}


	if(nChar == 'C')
	{
		if(GetKeyState(VK_CONTROL))
			if(CopyKeys())
				return;
	}

	if(nChar == 'V')
	{
		if(GetKeyState(VK_CONTROL))
		{
			StartPasteKeys();
			return;
		}
	}


	if(nChar == 'Z')
	{
		if(GetKeyState(VK_CONTROL))
		{
			GetIEditor()->Undo();
			return;
		}
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}


//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::RecordTrackUndo( const Item &item )
{
	if (item.track != 0)
	{
		CUndo undo("Track Modify");
		CUndo::Record( new CUndoTrackObject(item.track, m_pSequence) );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::ShowKeyTooltip( IAnimTrack *pTrack,int nKey,CPoint point )
{
	if (m_lastTooltipPos.x == point.x && m_lastTooltipPos.y == point.y)
		return;

	m_lastTooltipPos = point;

	assert(pTrack);
	float time = pTrack->GetKeyTime(nKey);
	const char *desc = "";
	float duration = 0;
	pTrack->GetKeyInfo( nKey,desc,duration );

	CString tipText;
	if(GetTickDisplayMode() == TVTICK_INSECONDS)
		tipText.Format( "%.3f, {%s}",time,desc );
	else // if(GetTickDisplayMode() == TVTICK_INFRAMES)
		tipText.Format( "%d, {%s}",FloatToIntRet(time/m_snapFrameTime),desc );
	m_tooltip.UpdateTipText( tipText,this,1 );
	m_tooltip.Activate(TRUE);
}

//////////////////////////////////////////////////////////////////////////
void  CTrackViewDopeSheetBase::InvalidateItem( int item )
{
	CRect rc;
	if (GetItemRect(item,rc) != -1)
	{
		InvalidateRect(rc);

	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnBeginGameMode:
		m_storedTime = m_currentTime;
		break;
	case eNotify_OnEndGameMode:
		GetIEditor()->GetAnimation()->SetResetTime( m_storedTime );
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnRawInput( UINT wParam,HRAWINPUT lParam )
{
	// Forward raw input to the Track View dialog
	CWnd *pWnd = GetOwner();
	if (pWnd)
		pWnd->SendMessage( WM_INPUT,wParam,(LPARAM)lParam );
}


//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnCaptureChanged( CWnd* pWnd )
{
	m_mouseMode = MOUSE_MODE_NONE;
	m_bZoomDrag = false;
	m_bMoveDrag = false;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::ClearSelection()
{
	for (int i = 0; i < GetCount(); i++)
	{
		m_tracks[i].bSelected = false;
	}
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SelectItem( int item )
{
	if (item >= 0 && item < GetCount())
	{
		m_tracks[item].bSelected = true;
	}
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewDopeSheetBase::IsSelectedItem( int item )
{
	if (item >= 0 && item < GetCount())
	{
		return m_tracks[item].bSelected;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SlideKey( IAnimTrack *track, float time0, float timeOffset )
{
	for (int i=0; i<track->GetNumKeys(); ++i)
	{
		float keyTime = track->GetKeyTime(i);
		if (keyTime >= time0)
			track->SetKeyTime(i, keyTime+timeOffset);	
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OffsetKey( IAnimTrack *track, int keyIndex, float timeOffset )
{
	float keyt = track->GetKeyTime(keyIndex) + timeOffset;
	track->SetKeyTime(keyIndex, keyt);	
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::NotifyKeySelectionUpdate()
{
	GetIEditor()->Notify( eNotify_OnUpdateTVKeySelection );

	// In the end, focus this again in order to properly catch 'KeyDown' messages.
	SetFocus();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewDopeSheetBase::IsOkToAddKeyHere(const IAnimTrack *pTrack, float time) const
{
	const float timeEpsilon = 0.05f;

	for(int i=0; i<pTrack->GetNumKeys(); ++i)
	{
		if(fabs(pTrack->GetKeyTime(i) - time) < timeEpsilon)
		{
			AfxMessageBox("Too close keys in time are not allowed!", MB_OK|MB_ICONEXCLAMATION);
			return false;
		}
	}

	return true;
}

void CTrackViewDopeSheetBase::MouseMoveSelect( CPoint point )
{
	SetMouseCursor(NULL);
	CRect rc( m_mouseDownPos.x,m_mouseDownPos.y,point.x,point.y );
	rc.NormalizeRect();
	CRect rcClient;
	GetClientRect(rcClient);
	rc.IntersectRect(rc,rcClient);

	CDC *dc = GetDC();
	if (m_mouseMode == MOUSE_MODE_SELECT_WITHIN_TIME)
	{
		rc.top = m_rcClient.top;
		rc.bottom = m_rcClient.bottom;
	}
	dc->DrawDragRect( rc,CSize(1,1),m_rcSelect,CSize(1,1) );
	ReleaseDC(dc);
	m_rcSelect = rc;
}


void CTrackViewDopeSheetBase::MouseMoveStartEndTimeAdjust( CPoint point, bool bStart )
{
	SetMouseCursor(m_crsAdjustLR);

	if (point.x < m_rcClient.left)
		point.x = m_rcClient.left;
	if (point.x > m_rcClient.right)
		point.x = m_rcClient.right;
	CPoint ofs = point - m_mouseDownPos;

	ICharacterKey key;
	m_pTrackForTimeAdjust->GetKey(m_keyForTimeAdjust, &key);
	float& timeToAdjust = bStart ? key.m_startTime : key.m_endTime; 

	// Undo the last offset.
	timeToAdjust += -m_keyTimeOffset;

	// Apply a new offset.
	m_keyTimeOffset = ofs.x / m_timeScale * key.m_speed;
	timeToAdjust += m_keyTimeOffset;

	// Check the validity.
	if (bStart)
	{
		if (timeToAdjust < 0)
			timeToAdjust = 0;
		else if (timeToAdjust > key.GetValidEndTime())
			timeToAdjust = key.GetValidEndTime();
	}
	else
	{
		if (timeToAdjust < key.m_startTime)
			timeToAdjust = key.m_startTime;
		else if (timeToAdjust > key.m_duration) 
			timeToAdjust = key.m_duration;
	}

	CUndo::Record( new CUndoTrackObject(m_pTrackForTimeAdjust, m_pSequence) );
	m_pTrackForTimeAdjust->SetKey(m_keyForTimeAdjust, &key);

	UpdateAnimation();
	Invalidate();
}

void CTrackViewDopeSheetBase::MouseMoveMove( CPoint point, UINT nFlags )
{
	SetMouseCursor(m_crsLeftRight);
	if (point.x < m_rcClient.left)
		point.x = m_rcClient.left;
	if (point.x > m_rcClient.right)
		point.x = m_rcClient.right;
	CPoint ofs = point - m_mouseDownPos;

	ESnappingMode snappingMode = m_snappingMode;

	bool bAltClick = CheckVirtualKey(VK_MENU);
	if (nFlags & MK_CONTROL)
		snappingMode = TVKEY_SNAP_NONE;
	else if (nFlags & MK_SHIFT)
		snappingMode = TVKEY_SNAP_MAGNET;
	else if (bAltClick)
		snappingMode = TVKEY_SNAP_FRAME;

	bool bTickSnap = snappingMode==TVKEY_SNAP_TICK;

	if (m_mouseActionMode == TVMODE_SCALEKEY)
	{
		// Offset all selected keys back by previous offset.
		if (m_keyTimeOffset != 0)
			ScaleSelectedKeys( 1.0f/(1+m_keyTimeOffset) );
	}
	else
	{
		bool bSlide = false;
		if (m_mouseActionMode == TVMODE_SLIDEKEY)
			bSlide = true;

		// Offset all selected keys back by previous offset.
		if (m_keyTimeOffset != 0)
		{
			if (bSlide)
				SlideKeys(-m_keyTimeOffset);
			else
				OffsetSelectedKeys( -m_keyTimeOffset );
		}
	}

	float oldTime;

	IAnimNode *node = 0;
	int key = FirstKeyFromPoint(m_mouseDownPos);
	if (key < 0)
		key = DurationKeyFromPoint(m_mouseDownPos);
	if (key >= 0)
	{
		int item = ItemFromPoint(m_mouseDownPos);
		IAnimTrack *track = GetTrack(item);
		node = GetNode(item);

		oldTime = track->GetKeyTime(key);
	}
	else
	{
		oldTime = TimeFromPointUnsnapped(m_mouseDownPos);
	}

	float timeOffset = ofs.x / m_timeScale;
	float newTime = oldTime + timeOffset;
	
	// Snap it, if necessary.
	if (snappingMode == TVKEY_SNAP_MAGNET)
	{
		newTime = MagnetSnap(newTime, node);
	}
	else if (snappingMode == TVKEY_SNAP_FRAME)
	{
		newTime = FrameSnap(newTime);
	}
	else if (snappingMode == TVKEY_SNAP_TICK)
	{
		newTime = TickSnap(newTime);
	}

	Range extendedTimeRange( 0.0f, m_timeRange.end );
	extendedTimeRange.ClipValue( newTime );	

	timeOffset = newTime - oldTime;	// Re-compute the time offset using snapped & clipped 'newTime'.

	if (m_mouseActionMode == TVMODE_SCALEKEY)
	{
		float tscale = 0.005f;
		float tofs = ofs.x * tscale;
		tofs = ClipTimeOffsetForScaling( 1+tofs ) - 1;
		// Offset all selected keys by this offset.
		ScaleSelectedKeys( 1+tofs );
		m_keyTimeOffset = tofs;
	}
	else
	{
		bool bSlide = false;
		if (m_mouseActionMode == TVMODE_SLIDEKEY)
			bSlide = true;

		// Offset all selected keys by this offset.
		if (bSlide)
		{
			timeOffset = ClipTimeOffsetForSliding( timeOffset );
			SlideKeys(timeOffset);
		}
		else
		{
			timeOffset = ClipTimeOffsetForOffsetting( timeOffset );
			OffsetSelectedKeys( timeOffset);
		}

		m_keyTimeOffset = timeOffset;
	}
	Invalidate();
}

void CTrackViewDopeSheetBase::MouseMoveDragTime( CPoint point, UINT nFlags )
{
	CPoint p = point;
	if (p.x < m_rcClient.left)
		p.x = m_rcClient.left;
	if (p.x > m_rcClient.right)
		p.x = m_rcClient.right;
	if (p.y < m_rcClient.top)
		p.y = m_rcClient.top;
	if (p.y > m_rcClient.bottom)
		p.y = m_rcClient.bottom;

	bool bNoSnap = (nFlags & MK_CONTROL) != 0;
	float time = TimeFromPointUnsnapped(p);
	m_timeRange.ClipValue( time );
	if (!bNoSnap)
		time = TickSnap(time);
	SetCurrTime( time );
}

void CTrackViewDopeSheetBase::MouseMoveDragStartMarker( CPoint point, UINT nFlags )
{
	CPoint p = point;
	if (p.x < m_rcClient.left)
		p.x = m_rcClient.left;
	if (p.x > m_rcClient.right)
		p.x = m_rcClient.right;
	if (p.y < m_rcClient.top)
		p.y = m_rcClient.top;
	if (p.y > m_rcClient.bottom)
		p.y = m_rcClient.bottom;

	bool bNoSnap = (nFlags & MK_CONTROL) != 0;
	float time = TimeFromPointUnsnapped(p);
	m_timeRange.ClipValue( time );
	if (!bNoSnap)
		time = TickSnap(time);
	SetStartMarker( time );
}

void CTrackViewDopeSheetBase::MouseMoveDragEndMarker( CPoint point, UINT nFlags )
{
	CPoint p = point;
	if (p.x < m_rcClient.left)
		p.x = m_rcClient.left;
	if (p.x > m_rcClient.right)
		p.x = m_rcClient.right;
	if (p.y < m_rcClient.top)
		p.y = m_rcClient.top;
	if (p.y > m_rcClient.bottom)
		p.y = m_rcClient.bottom;

	bool bNoSnap = (nFlags & MK_CONTROL) != 0;
	float time = TimeFromPointUnsnapped(p);
	m_timeRange.ClipValue( time );
	if (!bNoSnap)
		time = TickSnap(time);
	SetEndMarker( time );
}

void CTrackViewDopeSheetBase::MouseMovePaste( CPoint point, UINT nFlags )
{
	CPoint p = point;
	if (p.x < m_rcClient.left)
		p.x = m_rcClient.left;
	if (p.x > m_rcClient.right)
		p.x = m_rcClient.right;

	bool bNoSnap = (nFlags & MK_CONTROL) != 0;
	float time0 = TimeFromPointUnsnapped(m_mouseDownPos);
	float time = TimeFromPointUnsnapped(p);
	if (!bNoSnap)
	{
		time = TickSnap(time);
		time0 = TickSnap(time0);
	}
	float fTimeOffset = time - time0;

	// Undo previous past keys.
	if (CUndo::IsRecording())
		GetIEditor()->RestoreUndo();
	PasteKeys( 0,0,fTimeOffset );
}

void CTrackViewDopeSheetBase::MouseMoveOver( CPoint point )
{
	// No mouse mode.
	SetMouseCursor(NULL);

	bool bStart = false;
	int key = CheckCursorOnStartEndTimeAdjustBar(point, bStart);
	if (key >= 0)
	{
		SetMouseCursor(m_crsAdjustLR);
		return;
	}

	key = FirstKeyFromPoint(point);
	if (key < 0)
		key = DurationKeyFromPoint(point);
	if (key >= 0)
	{
		m_bMouseOverKey = true;
		int item = ItemFromPoint(point);
		IAnimTrack *track = GetTrack(item);
		if (track && track->IsKeySelected(key))
		{
			// If mouse over selected key, change cursor to left-right arrows.
			SetMouseCursor(m_crsLeftRight);
		}
		else
		{
			SetMouseCursor(m_crsCross);
		}

		if (track)
		{
			ShowKeyTooltip( track,key,point );
		}
	}
	else
	{
		if (m_tooltip.m_hWnd)
			m_tooltip.Activate(FALSE);
	}
}

int CTrackViewDopeSheetBase::GetAboveKey(IAnimTrack *&aboveTrack)
{
	IAnimTrack *track;
	int keyIndex;
	if (FindSingleSelectedKey(track, keyIndex) == false)
		return -1;

	// First, finds a track above.
	IAnimTrack *pLastTrack = NULL;
	for (int i=0; i < GetCount(); ++i)
	{
		if (m_tracks[i].track == track)
			break;

		bool bValidSingleTrack = m_tracks[i].track && 
														m_tracks[i].track->GetNumKeys() > 0 &&
														m_tracks[i].track->GetSubTrackCount() == 0;
		if (bValidSingleTrack)
			pLastTrack = m_tracks[i].track;
	}

	if (pLastTrack == NULL)
		return -1;
	else
		aboveTrack = pLastTrack;

	// A track found. Now gets a proper key index there.
	int k;
	float selectedKeyTime = track->GetKeyTime(keyIndex);
	for (k=0; k < aboveTrack->GetNumKeys(); ++k)
	{
		if (aboveTrack->GetKeyTime(k) >= selectedKeyTime)
			break;
	}
	
	return k % aboveTrack->GetNumKeys();
}

int CTrackViewDopeSheetBase::GetBelowKey(IAnimTrack *&belowTrack)
{
	IAnimTrack *track;
	int keyIndex;
	if (FindSingleSelectedKey(track, keyIndex) == false)
		return -1;

	// First, finds a track below.
	IAnimTrack *pLastTrack = NULL;
	for (int i=GetCount()-1; i >= 0; --i)
	{
		if (m_tracks[i].track == track)
			break;

		bool bValidSingleTrack = m_tracks[i].track && 
														m_tracks[i].track->GetNumKeys() > 0 &&
														m_tracks[i].track->GetSubTrackCount() == 0;
		if (bValidSingleTrack)
			pLastTrack = m_tracks[i].track;
	}

	if (pLastTrack == NULL)
		return -1;
	else
		belowTrack = pLastTrack;

	// A track found. Now gets a proper key index there.
	int k;
	float selectedKeyTime = track->GetKeyTime(keyIndex);
	for (k=0; k < belowTrack->GetNumKeys(); ++k)
	{
		if (belowTrack->GetKeyTime(k) >= selectedKeyTime)
			break;
	}

	return k % belowTrack->GetNumKeys();
}

int CTrackViewDopeSheetBase::GetRightKey(IAnimTrack *&track)
{
	int keyIndex;
	if (FindSingleSelectedKey(track, keyIndex) == false)
		return -1;


	return (keyIndex + 1) % track->GetNumKeys();
}

int CTrackViewDopeSheetBase::GetLeftKey(IAnimTrack *&track)
{
	int keyIndex;
	if (FindSingleSelectedKey(track, keyIndex) == false)
		return -1;

	return (keyIndex + track->GetNumKeys() - 1) % track->GetNumKeys();
}

float CTrackViewDopeSheetBase::MagnetSnap( float newTime, const IAnimNode * node ) const
{
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetKeysInTimeRange(m_pSequence, selectedKeys, 
		newTime - MARGIN_FOR_MAGNET_SNAPPING/m_timeScale, 
		newTime + MARGIN_FOR_MAGNET_SNAPPING/m_timeScale);
	if (selectedKeys.keys.size() > 0)
	{
		// By default, just use the first key that belongs to the time range as a magnet.
		newTime = selectedKeys.keys[0].pTrack->GetKeyTime(selectedKeys.keys[0].nKey);
		// But if there is an in-range key in a sibling track, use it instead.
		// Here a 'sibling' means a track that belongs to a same node.
		for (int i=0; i<selectedKeys.keys.size(); ++i)
		{
			if (selectedKeys.keys[i].pNode == node)
			{
				newTime = selectedKeys.keys[i].pTrack->GetKeyTime(selectedKeys.keys[i].nKey);
				break;
			}
		}
	}
	return newTime;
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::FrameSnap( float time ) const
{
	double t = floor( (double)time/m_snapFrameTime + 0.5);
	t = t * m_snapFrameTime;
	return t;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::ShowKeyPropertyCtrlOnSpot(int x, int y, bool bMultipleKeysSelected, bool bKeyChangeInSameTrack)
{
	if(m_keyPropertiesDlg == NULL)
		return;

	if(m_wndPropsOnSpot.m_hWnd == 0)
	{
		CRect rc;
		GetClientRect(rc);
		m_wndPropsOnSpot.Create( WS_POPUP,rc,this );
		m_wndPropsOnSpot.ModifyStyleEx( 0,WS_EX_PALETTEWINDOW|WS_EX_CLIENTEDGE );
		bKeyChangeInSameTrack = false;
	}
	m_wndPropsOnSpot.MoveWindow(x, y, 200, 200);
	if (bKeyChangeInSameTrack)
	{
		m_wndPropsOnSpot.ClearSelection();
		m_wndPropsOnSpot.ReloadValues();
	}
	else
	{
		m_keyPropertiesDlg->PopulateVariables(m_wndPropsOnSpot);
	}
	if (bMultipleKeysSelected)
		m_wndPropsOnSpot.SetDisplayOnlyModified(true);
	else
		m_wndPropsOnSpot.SetDisplayOnlyModified(false);
	m_wndPropsOnSpot.ShowWindow(SW_SHOW);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::HideKeyPropertyCtrlOnSpot()
{
	if (m_wndPropsOnSpot.m_hWnd)
	{
		m_wndPropsOnSpot.ShowWindow(SW_HIDE);
		m_wndPropsOnSpot.ClearSelection();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetScrollOffset(int hpos)
{
	SetScrollPos( SB_HORZ,hpos );
	m_scrollOffset.x = hpos;
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::LButtonDownOnTimeAdjustBar( CPoint point, int key, bool bStart )
{
	m_keyTimeOffset = 0;
	m_keyForTimeAdjust = key;
	int item = ItemFromPoint(point);
	m_pTrackForTimeAdjust = GetTrack(item);

	GetIEditor()->BeginUndo();

	if (bStart)
		m_mouseMode = MOUSE_MODE_START_TIME_ADJUST;
	else
	{
		// In case of the end time, make it have a valid (not zero)
		// end time, first.
		ICharacterKey animKey;
		m_pTrackForTimeAdjust->GetKey(m_keyForTimeAdjust, &animKey);
		if (animKey.m_endTime == 0)
		{
			animKey.m_endTime = animKey.m_duration;
			CUndo::Record( new CUndoTrackObject(m_pTrackForTimeAdjust, m_pSequence) );
			m_pTrackForTimeAdjust->SetKey(m_keyForTimeAdjust, &animKey);
		}
		m_mouseMode = MOUSE_MODE_END_TIME_ADJUST;
	}
	SetMouseCursor(m_crsAdjustLR);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::LButtonDownOnKey( CPoint point, int key, UINT nFlags )
{
	/// Move/Clone Key Undo Begin
	GetIEditor()->BeginUndo();

	int item = ItemFromPoint(point);
	IAnimTrack *track = GetTrack(item);

	if (!track->IsKeySelected(key) && !(nFlags&MK_CONTROL))
	{
		UnselectAllKeys(false);
	}
	m_bAnySelected = true;
	m_bJustSelected = true;
	m_keyTimeOffset = 0;
	track->SelectKey(key, true);
	if (nFlags & MK_SHIFT)
	{
		m_mouseMode = MOUSE_MODE_CLONE;
		SetMouseCursor( m_crsLeftRight );
	}
	else
	{
		m_mouseMode = MOUSE_MODE_MOVE;
		SetMouseCursor( m_crsLeftRight );
	}
	Invalidate();
	NotifyKeySelectionUpdate();
}