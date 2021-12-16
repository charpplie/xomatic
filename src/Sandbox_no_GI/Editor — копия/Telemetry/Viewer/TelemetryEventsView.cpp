////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   TelemetryEventsView.h
//  Version:     v1.00
//  Created:     13/05/11 by Steve Humphreys
//  Description: Handles the view of events for timelines (similar to
//								trackview's DopeSheet)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "TelemetryEventsView.h"
#include "Controls\MemDC.h"

#include "TelemetryViewDialog.h"
#include "AnimationContext.h"

#include "Clipboard.h"

#include "Telemetry/TelemetryObjects.h"

enum ETVMouseMode
{
	MOUSE_MODE_NONE = 0,
	MOUSE_MODE_SELECT = 1,
	MOUSE_MODE_MOVE,
	MOUSE_MODE_CLONE,
	MOUSE_MODE_DRAGTIME,
	MOUSE_MODE_DRAGSTARTMARKER,
	MOUSE_MODE_DRAGENDMARKER,
	MOUSE_MODE_PASTE_DRAGTIME,
	MOUSE_MODE_SELECT_WITHIN_TIME,
};

#define KEY_TEXT_COLOR RGB(255,255,255)
#define EDIT_DISABLE_GRAY_COLOR RGB(180,180,180)

//#define KEY_TEXT_COLOR RGB(0,0,50)
#define INACTIVE_TEXT_COLOR RGB(128,128,128)

static const int MARGIN_FOR_MAGNET_SNAPPING = 10;

// CTrackViewKeys

IMPLEMENT_DYNAMIC(CTelemetryEventsView, CWnd)

CTelemetryEventsView::CTelemetryEventsView()
{
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

	m_scrollOffset = CPoint(0,0);
	m_bAnySelected = 0;
	m_mouseMode = MOUSE_MODE_NONE;
	m_currentTime = 40;
	m_storedTime = m_currentTime;
	m_rcSelect = CRect(0,0,0,0);
	m_keyTimeOffset = 0;
	m_currCursor = NULL;

	m_scrollMin = 0;
	m_scrollMax = 1000;

	m_descriptionFont = new CFont();
	m_descriptionFont->CreateFont( 12,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
 		DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"Verdana" );

	m_bCursorWasInKey = false;
	m_snapFrameTime = 0.033333f;
	m_bMouseMovedAfterRButtonDown = false;

	m_tickDisplayMode = TeVTICK_INSECONDS;

	m_leftOffset = 30;

	m_itemWidth = 1000;
	m_itemHeight = 16;

	m_pTimelinesCtrl = NULL;
}

CTelemetryEventsView::~CTelemetryEventsView()
{
	m_descriptionFont->DeleteObject();
	delete m_descriptionFont;
}


BEGIN_MESSAGE_MAP(CTelemetryEventsView, CWnd)
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
END_MESSAGE_MAP()



// CTrackViewKeys message handlers

//////////////////////////////////////////////////////////////////////////
int CTelemetryEventsView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_imageList.Create( MAKEINTRESOURCE(IDB_TRACKVIEW_KEYS),14,0,RGB(255,0,255) );
	m_imgMarker.Create( MAKEINTRESOURCE(IDB_MARKER),8,0,RGB(255,0,255) );
	m_crsLeftRight = AfxGetApp()->LoadStandardCursor(IDC_SIZEWE);
	m_crsAddKey = AfxGetApp()->LoadCursor(IDC_ARROW_ADDKEY);
	m_crsCross = AfxGetApp()->LoadCursor(IDC_POINTER_OBJHIT);

	GetIEditor()->RegisterNotifyListener( this );

	//InitializeFlatSB(GetSafeHwnd());

	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::OnDestroy()
{
	HideKeyPropertyCtrlOnSpot();
	GetIEditor()->UnregisterNotifyListener( this );
	CWnd::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::DrawRuler( CDC *dc,const CRect &rcUpdate )
{
	CRect rc,temprc;
	
	bool recording = false;// GetIEditor()->GetAnimation()->IsRecording();

	COLORREF lineCol = RGB(255,0,255);
	COLORREF textCol = RGB(0,0,0);
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
	CPen ltgray(PS_SOLID,1,RGB(90,90,90));
	CPen black(PS_SOLID,1,textCol);
	CPen redpen(PS_SOLID,1,lineCol );
	// Draw time ticks every tick step seconds.
	Range timeRange = m_timeRange;
	CString str;

	dc->SetTextColor( textCol );
	dc->SetBkMode( TRANSPARENT );
	dc->SelectObject( gSettings.gui.hSystemFont );

	dc->SelectObject(ltgray);

	Range VisRange=GetVisibleRange();
	int nNumberTicks=10;
	if (GetTickDisplayMode() == TeVTICK_INFRAMES)
		nNumberTicks = RoundFloatToInt(1.0f / m_snapFrameTime);
	double step = (double)1.0 / m_ticksStep;
	for (double t = TickSnap(timeRange.start); t <= timeRange.end+step; t += step)
	{
		double st = TickSnap(t);
		if (st > timeRange.end)
			st = timeRange.end;
		if (st < VisRange.start)
			continue;
		if (st > VisRange.end)
			break;
		if (st < m_timeRange.start || st > m_timeRange.end)
			continue;
		int x = TimeToClient(st);
		dc->MoveTo(x,rc.bottom-2);

		int k = RoundFloatToInt(st * m_ticksStep);
		if (k % nNumberTicks == 0)
		{
			dc->SelectObject(black);
			dc->LineTo(x,rc.bottom-14);
			char str[32];
			if (GetTickDisplayMode() == TeVTICK_INSECONDS)
				sprintf( str,"%g",st );
			else if (GetTickDisplayMode() == TeVTICK_INFRAMES)
				sprintf( str,"%g",st/m_snapFrameTime );
			else
				assert(0);
			dc->TextOut( x+2,rc.top,str );
			dc->SelectObject(ltgray);
		}
		else
			dc->LineTo(x,rc.bottom-6);
	}

	// Draw time markers.
	int x;
	CPen dkbluedash(PS_DOT,1,RGB(20,20,150));
	dc->SelectObject(dkbluedash);

	x=TimeToClient(m_timeMarked.start);
	m_imgMarker.Draw(dc, 1, CPoint(x, m_rcTimeline.bottom-9), ILD_TRANSPARENT);
	dc->MoveTo(x,m_rcClient.top);
	dc->LineTo(x,m_rcClient.bottom);

	x=TimeToClient(m_timeMarked.end);
	m_imgMarker.Draw(dc, 0, CPoint(x-7, m_rcTimeline.bottom-9), ILD_TRANSPARENT);
	dc->MoveTo(x,m_rcClient.top);
	dc->LineTo(x,m_rcClient.bottom);

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
int CTelemetryEventsView::TimeToClient( float time ) const
{
	int x = m_leftOffset - m_scrollOffset.x + time*m_timeScale;
	return x;
}

//////////////////////////////////////////////////////////////////////////
Range CTelemetryEventsView::GetVisibleRange()
{
	Range r;
	r.start = (m_scrollOffset.x - m_leftOffset)/m_timeScale;
	r.end = r.start + (m_rcClient.Width())/m_timeScale;
	// Intersect range with global time range.
	r = m_timeRange & r;
	return r;
}

//////////////////////////////////////////////////////////////////////////
Range CTelemetryEventsView::GetTimeRange( CRect &rc )
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
void CTelemetryEventsView::DrawTicks( CDC *dc,CRect &rc,Range &timeRange )
{
	// Draw time ticks every tick step seconds.
	CPen ltgray(PS_SOLID,1,RGB(90,90,90));
	CPen *prevPen = dc->SelectObject( &ltgray );
	Range VisRange=GetVisibleRange();
	int nNumberTicks=10;
	if (GetTickDisplayMode() == TeVTICK_INFRAMES)
		nNumberTicks = RoundFloatToInt(1.0f / m_snapFrameTime);
	double step = 1.0 / m_ticksStep;
	for (double t = TickSnap(timeRange.start); t <= timeRange.end+step; t += step)
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
			dc->SelectObject( GetStockObject(BLACK_PEN) );
			dc->LineTo(x,rc.bottom-5);
			dc->SelectObject( ltgray );
		}
		else
			dc->LineTo(x,rc.bottom-3);
	}
	dc->SelectObject( prevPen );
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::RedrawItem( int item )
{
	CRect rc;
	if (GetItemRect( item,rc ) != LB_ERR)
	{
		RedrawWindow( rc,NULL,RDW_INVALIDATE|RDW_ERASE );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	lpMIS->itemWidth = 1000;
  lpMIS->itemHeight = 16;
}

//////////////////////////////////////////////////////////////////////////
HBRUSH CTelemetryEventsView::CtlColor(CDC* pDC, UINT nCtlColor)
{
	return m_bkgrBrush;

	// TODO:  Return a non-NULL brush if the parent's handler should not be called
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::SetTimeRange( float start,float end )
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

	m_realTimeRange.Set(start,end);
	m_timeRange.Set( start,end );
	//SetHorizontalExtent( m_timeRange.Length() *m_timeScale + 2*m_leftOffset );
	
	SetHorizontalExtent( m_timeRange.start*m_timeScale-m_leftOffset,m_timeRange.end*m_timeScale-m_leftOffset );
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::SetTimeScale( float timeScale,float fAnchorTime )
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
	if (GetTickDisplayMode() == TeVTICK_INSECONDS)
		m_ticksStep = 10;
	else if (GetTickDisplayMode() == TeVTICK_INFRAMES)
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

	//SetHorizontalExtent( m_timeRange.Length()*m_timeScale + 2*m_leftOffset );
	SetHorizontalExtent( m_timeRange.start*m_timeScale-m_leftOffset,m_timeRange.end*m_timeScale );
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::OnSize(UINT nType, int cx, int cy)
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
BOOL CTelemetryEventsView::PreTranslateMessage(MSG* pMsg)
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
BOOL CTelemetryEventsView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
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
void CTelemetryEventsView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
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
float CTelemetryEventsView::TickSnap( float time ) const
{
	double t = floor( (double)time*m_ticksStep + 0.5);
	t = t / m_ticksStep;
	return t;
}

//////////////////////////////////////////////////////////////////////////
float CTelemetryEventsView::TimeFromPoint( CPoint point ) const
{
	int x = point.x - m_leftOffset + m_scrollOffset.x;
	double t = (double)x / m_timeScale;
	return (float)TickSnap(t);
}

//////////////////////////////////////////////////////////////////////////
float CTelemetryEventsView::TimeFromPointUnsnapped( CPoint point ) const
{
	int x = point.x - m_leftOffset + m_scrollOffset.x;
	double t = (double)x / m_timeScale;
	return t;
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::OnLButtonDown(UINT nFlags, CPoint point)
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
		// adjust markers.
		int nMarkerStart=TimeToClient(m_timeMarked.start);
		int nMarkerEnd=TimeToClient(m_timeMarked.end);
		if ((abs(point.x-nMarkerStart))<8)
		{
			SetStartMarker(TimeFromPoint(point));
			m_mouseMode = MOUSE_MODE_DRAGSTARTMARKER;
			SetMouseCursor(m_crsLeftRight);

			SetCapture();
			return;
		}
		else if((abs(point.x-nMarkerEnd)) < 8)
		{
			SetEndMarker(TimeFromPoint(point));
			m_mouseMode = MOUSE_MODE_DRAGENDMARKER;
			SetMouseCursor(m_crsLeftRight);

			SetCapture();
			return;
		}

		// Clicked inside timeline but not near time markers
		m_mouseMode = MOUSE_MODE_DRAGTIME;
		// If mouse over selected key, change cursor to left-right arrows.
		SetMouseCursor( m_crsLeftRight );
		SetCapture();
		
		SetCurrTime( TimeFromPoint(point) );
		return;
	}

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

	if (nFlags & MK_SHIFT)
		m_mouseMode = MOUSE_MODE_SELECT_WITHIN_TIME;
	else
		m_mouseMode = MOUSE_MODE_SELECT;
	SetCapture();
	if (m_bAnySelected && !(nFlags & MK_CONTROL))
	{
		// First unselect all buttons.
		Invalidate();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::OnLButtonUp(UINT nFlags, CPoint point)
{
	switch(m_mouseMode)
	{
	case MOUSE_MODE_SELECT:
		{
			bool prevSelected = m_bAnySelected;
			// Check if any key are selected.
			m_rcSelect-=m_scrollOffset;

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
		break;

	case MOUSE_MODE_SELECT_WITHIN_TIME:
		{
			m_rcSelect-=m_scrollOffset;

			Invalidate();
			m_rcSelect = CRect(0,0,0,0);
		}
		break;

	case MOUSE_MODE_DRAGTIME:
		{
			SetMouseCursor(NULL);
		}
		break;

	case MOUSE_MODE_PASTE_DRAGTIME:
		{
			GetIEditor()->AcceptUndo( "Paste Keys" );
			SetMouseCursor(NULL);
		}
		break;

	case MOUSE_MODE_DRAGSTARTMARKER:
	case MOUSE_MODE_DRAGENDMARKER:
		SetMouseCursor(NULL);
		break;

	default:
		break;
	}

	if (GetCapture() == this)
	{
		ReleaseCapture();
	}

	//if (GetIEditor()->IsUndoRecording())
	//GetIEditor()->AcceptUndo( "Track Modify" );

	/// Move/Clone Key Undo End
	if(m_mouseMode == MOUSE_MODE_MOVE || m_mouseMode == MOUSE_MODE_CLONE)
		GetIEditor()->AcceptUndo("Move/Clone Keys");

	m_mouseMode = MOUSE_MODE_NONE;
	CWnd::OnLButtonUp(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_rcTimeline.PtInRect(point))
	{
		// Clicked inside timeline.
		return;
	}

// 	int key = FirstKeyFromPoint(point);
// 	if (key >= 0)
// 	{
// 		int item = ItemFromPoint(point);
// 		IAnimTrack *track = GetTrack(item);
// 		UnselectAllKeys(false);
// 		track->SelectKey(key, true);
// 		m_bAnySelected = true;
// 		m_keyTimeOffset = 0;
// 		Invalidate();
// 
// 		SetKeyInfo( track,key,true );
// 
// 		CPoint p;
// 		::GetCursorPos(&p);
// 
// 		bool bKeyChangeInSameTrack
// 			= m_pLastTrackSelectedOnSpot
// 				&& track == m_pLastTrackSelectedOnSpot;
// 		m_pLastTrackSelectedOnSpot = track;
// 
// 		ShowKeyPropertyCtrlOnSpot(p.x, p.y, false, bKeyChangeInSameTrack);
// 		return;
// 	}
// 
// 	// Add key here.
// 	int item = ItemFromPoint(point);
// 	IAnimTrack *track = GetTrack(item);
// 	IAnimNode *node = GetNode(item);
// 	if (track) 
// 	{
// 		bool keyCreated = false;
// 		float keyTime = TimeFromPoint(point);
// 		if ((nFlags & MK_SHIFT) && node->GetParent())		// Add keys in group
// 		{
// 
// 			CTelemetryViewUtils::SelectedTracks tracksInGroup;
// 			CTelemetryViewUtils::GetTracksInGroup(m_pSequence, node->GetParent(), track->GetParameterType(), tracksInGroup);
// 			for(int i=0; i<(int)tracksInGroup.tracks.size(); ++i)
// 			{
// 				IAnimTrack *pCurrTrack = tracksInGroup.tracks[i].pTrack;
// 
// 				if (pCurrTrack->GetSubTrackCount() == 0)	// A simple track
// 				{
// 					if (IsOkToAddKeyHere(pCurrTrack, keyTime))
// 					{
// 						pCurrTrack->CreateKey( keyTime );
// 						keyCreated = true;
// 					}			
// 				}
// 				else																			// A compound track
// 				{
// 					for (int k=0; k<pCurrTrack->GetSubTrackCount(); ++k)
// 					{
// 						IAnimTrack *subTrack = pCurrTrack->GetSubTrack(k);
// 						if (IsOkToAddKeyHere(subTrack, keyTime))
// 						{
// 							subTrack->CreateKey( keyTime );
// 							keyCreated = true;
// 						}
// 					}
// 				}
// 			}
// 		}
// 		else if (track->GetSubTrackCount() == 0)			// A simple track
// 		{
// 			if (IsOkToAddKeyHere(track, keyTime))
// 			{
// 				RecordTrackUndo( GetItem(item) );
// 				track->CreateKey( keyTime );
// 				keyCreated = true;
// 			}
// 		}
// 		else																				// A compound track
// 		{
// 			RecordTrackUndo( GetItem(item) );
// 			for (int i=0; i<track->GetSubTrackCount(); ++i)
// 			{
// 				IAnimTrack *subTrack = track->GetSubTrack(i);
// 				if (IsOkToAddKeyHere(subTrack, keyTime))
// 				{
// 					subTrack->CreateKey( keyTime );
// 					keyCreated = true;
// 				}
// 			}
// 		}
// 		if (keyCreated)
// 		{
// 			Invalidate();
// 			UpdateAnimation();
// 		}
// 	}
	
	m_mouseMode = MOUSE_MODE_NONE;
	CWnd::OnLButtonDblClk(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::OnMButtonDown(UINT nFlags, CPoint point)
{
	OnRButtonDown(nFlags,point);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::OnMButtonUp(UINT nFlags, CPoint point)
{
	OnRButtonUp(nFlags,point);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::OnRButtonDown(UINT nFlags, CPoint point)
{
	//CWnd::OnRButtonDown(nFlags, point);

	HideKeyPropertyCtrlOnSpot();

	m_bCursorWasInKey = false;
	m_bMouseMovedAfterRButtonDown = false;

	SetFocus();

	//int item = ItemFromPoint(point);
	//SetCurSel(item);

	m_mouseDownPos = point;

	if (nFlags & MK_SHIFT)	// alternative zoom
	{
		m_bZoomDrag=true;
		SetCapture();
		return;
	}
// 
// 	int key = FirstKeyFromPoint(point);
// 	if (key >= 0)
// 	{
// 		m_bCursorWasInKey = true;
// 
// 		int item = ItemFromPoint(point);
// 		IAnimTrack *track = GetTrack(item);
// 		track->SelectKey(key, true);
// 		m_bAnySelected = true;
// 		m_keyTimeOffset = 0;
// 		Invalidate();
// 
// 		if (FindSingleSelectedKey(track,key))
// 		{
// 			SetKeyInfo(track,key,true);
// 		}
// 
// 		// Show a little pop-up menu for copy & delete.
// 		enum { COPY_KEY=100, DELETE_KEY, EDIT_KEY_ON_SPOT };
// 		CMenu menu;
// 		menu.CreatePopupMenu();
// 		bool bEnableEditOnSpot = false;
// 		CTelemetryViewUtils::SelectedKeys selectedKeys;
// 		CTelemetryViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
// 		if (selectedKeys.keys.size() > 0 && selectedKeys.bAllOfSameType)
// 			bEnableEditOnSpot = true;
// 		menu.AppendMenu(bEnableEditOnSpot ? MF_STRING : MF_STRING|MF_GRAYED, 
// 										EDIT_KEY_ON_SPOT, "Edit On Spot");
// 		menu.AppendMenu(MF_SEPARATOR, 0 ,"");
// 		menu.AppendMenu(MF_STRING, COPY_KEY, "Copy");
// 		menu.AppendMenu(MF_SEPARATOR, 0 ,"");
// 		menu.AppendMenu(MF_STRING, DELETE_KEY, "Delete");
// 
// 		CPoint p;
// 		::GetCursorPos(&p);
// 		int cmd = menu.TrackPopupMenu(TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_LEFTBUTTON, p.x,p.y, this, NULL);
// 		if (cmd == EDIT_KEY_ON_SPOT)
// 		{	
// 			bool bKeyChangeInSameTrack
// 				= m_pLastTrackSelectedOnSpot
// 					&& selectedKeys.keys.size() == 1 
// 					&& selectedKeys.keys[0].pTrack == m_pLastTrackSelectedOnSpot;
// 
// 			if (selectedKeys.keys.size() == 1)
// 				m_pLastTrackSelectedOnSpot = selectedKeys.keys[0].pTrack;
// 			else
// 				m_pLastTrackSelectedOnSpot = NULL;
// 
// 			ShowKeyPropertyCtrlOnSpot(p.x, p.y, selectedKeys.keys.size() > 1, bKeyChangeInSameTrack);
// 		}
// 		else if(cmd == COPY_KEY)
// 			CopyKeys();
// 		else if(cmd == DELETE_KEY)
// 			DelSelectedKeys();
// 	}else
 	{
 		m_bMoveDrag=true;
 		SetCapture();
 	}
}

void CTelemetryEventsView::OnRButtonUp(UINT nFlags, CPoint point)
{
	m_bZoomDrag=false;
	m_bMoveDrag=false;

	if (GetCapture() == this)
	{
		ReleaseCapture();
	}
	m_mouseMode = MOUSE_MODE_NONE;

	SetMouseCursor(NULL);

	//CClipboard clip;
// 	if(m_bCursorWasInKey == false && clip.GetTitle() == "Track view keys")
// 	{
// 		bool bHasCopiedKey = true;
// 		
// 		if(clip.IsEmpty())
// 			bHasCopiedKey = false;
// 		
// 		{
// 			XmlNodeRef copyNode = clip.Get();
// 			if(copyNode==NULL || strcmp(copyNode->getTag(), "CopyKeysNode"))
// 				bHasCopiedKey = false;
// 			else
// 			{
// 				int nNumTracksToPaste = copyNode->getChildCount();
// 				if(nNumTracksToPaste==0)
// 					bHasCopiedKey = false;
// 			}
// 		}
// 		if(bHasCopiedKey 
// 		&& m_bMouseMovedAfterRButtonDown == false)	// Once moved, it means the user wanted to scroll, so no paste pop-up.
// 		{
// 			// Show a little pop-up menu for paste.
// 			enum { PASTE_KEY=100 };
// 			CMenu menu;
// 			menu.CreatePopupMenu();
// 			menu.AppendMenu(MF_STRING, PASTE_KEY, "Paste");
// 
// 			CPoint p;
// 			::GetCursorPos(&p);
// 			int cmd = menu.TrackPopupMenu(TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_LEFTBUTTON, p.x,p.y, this, NULL);
// //			if(cmd == PASTE_KEY)
// // 				StartPasteKeys();
// 		}
// 	}

	CWnd::OnRButtonUp(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::OnMouseMove(UINT nFlags, CPoint point)
{
	m_bMouseMovedAfterRButtonDown = true;

	m_mouseOverPos = point;

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
// 		CloneSelectedKeys();
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
	else
	{
		//////////////////////////////////////////////////////////////////////////
// 		if (m_mouseActionMode == TeVMODE_ADDKEY)
// 		{
// 			SetMouseCursor(m_crsAddKey);
// 		}
// 		else
 		{
 			MouseMoveOver(point);
 		}
	}

	CWnd::OnMouseMove(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::OnPaint()
{
	CPaintDC  PaintDC(this);
	
	CMemDC dc( PaintDC,&m_offscreenBitmap );

	// main section
	XTPPaintManager()->GradientFill( &dc,&PaintDC.m_ps.rcPaint,RGB(250,250,250),RGB(220,220,220),FALSE );

	DrawControl( dc,PaintDC.m_ps.rcPaint );
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::DrawTimeline( int item,CDC *dc,CRect &rcItem, Telemetry::CTelemetryTimeline* pTimeline )
{
	CPen pen(PS_SOLID,1,RGB(120,120,120));
	CPen *prevPen = dc->SelectObject( &pen );
	dc->MoveTo( rcItem.left,rcItem.bottom );
	dc->LineTo( rcItem.right,rcItem.bottom );
	dc->SelectObject( prevPen );

 	// draw coloured backgrounds to show the current time filter
//  	CRect rcTimeFilter = rcItem;
//  	rcTimeFilter.right = TimeToClient(m_timeMarked.start);
//  	dc->FillSolidRect(rcTimeFilter, RGB(160,160,160));
//  	rcTimeFilter = rcItem;
//  	rcTimeFilter.left = TimeToClient(m_timeMarked.end);
//  	dc->FillSolidRect(rcTimeFilter, RGB(160,160,160));

	dc->Draw3dRect( rcItem,GetSysColor(COLOR_3DHILIGHT),GetSysColor(COLOR_3DDKSHADOW) );


	CRect rcInner = rcItem;
	// 	//rcInner.DeflateRect(m_leftOffset,0,m_leftOffset,0);
	rcInner.left = max( rcItem.left,m_leftOffset - m_scrollOffset.x );
	rcInner.right = min( rcItem.right,(m_scrollMax + m_scrollMin) - m_scrollOffset.x + m_leftOffset*2 );
	 
	CRect rcInnerDraw( rcInner.left-6,rcInner.top,rcInner.right+6,rcInner.bottom );
	

	// Left outside
	CRect rcOutside = rcItem;
	rcOutside.right = rcInnerDraw.left-1;
	rcOutside.DeflateRect(1,1,1,0);
	dc->SelectObject( m_bkgrBrushEmpty );
	//dc->Rectangle( rcOutside );

	//rcOutside.DeflateRect(1,1,1,1);
	XTPPaintManager()->GradientFill( dc,rcOutside,RGB(210,210,210),RGB(180,180,180),FALSE );

	// Right outside.
	rcOutside = rcItem;
	rcOutside.left = rcInnerDraw.right+1;
	rcOutside.DeflateRect(1,1,1,0);
	//dc->Rectangle( rcOutside );

	//rcOutside.DeflateRect(1,1,1,1);
	XTPPaintManager()->GradientFill( dc,rcOutside,RGB(210,210,210),RGB(180,180,180),FALSE );

	// Get time range of update rectangle.
	Range timeRange = GetTimeRange(rcItem);

	// Draw tick marks in time range.
	DrawTicks( dc,rcInner,timeRange );

	// and finally all the events
	uint32 count = pTimeline->GetEventCount();
	for(uint32 i=0; i<count; ++i)
	{
		const Telemetry::STelemetryEvent& event = pTimeline->GetEvent(i);
		if(event.filterOk)
		{
			DrawEvent(event, dc, rcItem);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::DrawEvent(const Telemetry::STelemetryEvent& event, CDC *dc, CRect &rcItem )
{
	const COLORREF lineCol = RGB(50,50,50);
	const COLORREF lineColTimeFiltered = RGB(200,200,200);

	COLORREF col = lineColTimeFiltered;
	float time = event.timeMs / 1000.0f;
	if(m_timeMarked.IsInside(time))
	{
		col = lineCol;
	}

	POINT point;
	point.x = TimeToClient(time);

	CPen pen( PS_SOLID,2,col );
	CPen *prevPen = dc->SelectObject(&pen);

	point.y = rcItem.top;
	dc->MoveTo(point);
	point.y = rcItem.bottom;
	dc->LineTo(point);
	
	dc->SelectObject( prevPen );
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::DrawControl( CDC *dc,const CRect &rcUpdate )
{
	CRect rect;
	rect.SetRect( 0, 0, m_rcClient.Width(), 0 );
	int y = -m_scrollOffset.y;

	// Draw all items.
	uint count = m_pTimelinesCtrl->GetRecordCount();
		
	for(uint i=0; i<count; ++i)
	{
		Telemetry::CTimelineRecord* pRecord = m_pTimelinesCtrl->GetRecord(i);
		rect.top = y;
		y += TELEMETRY_TIMELINE_ITEM_HEIGHT;
		rect.bottom = y;


		if(pRecord->m_timelines.empty())
		{
			dc->FillSolidRect(rect, RGB(180,0,180));

			// draw element name instead
			COLORREF textCol = RGB(0,0,0);
			dc->SetTextColor( textCol );
			dc->SetBkMode( TRANSPARENT );
			dc->SelectObject( gSettings.gui.hSystemFont );

			dc->TextOut(12, rect.top+2, pRecord->GetName());
		}
		else
		{
			for(Telemetry::TTimelines::const_iterator it = pRecord->m_timelines.begin(), end = pRecord->m_timelines.end(); it != end; ++it)
			{
				Telemetry::CTelemetryTimeline* pTimeline = *it;
				
				if(pTimeline)
				{
					DrawTimeline(i, dc, rect, pTimeline);
				}
			}
		}
	}

	DrawRuler( dc,rcUpdate );
}

//////////////////////////////////////////////////////////////////////////
BOOL CTelemetryEventsView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_currCursor != NULL)
	{
		return 0;
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CTelemetryEventsView::SetMouseCursor( HCURSOR crs )
{
	m_currCursor = crs;
	if (m_currCursor != NULL)
		SetCursor(crs);
}

//////////////////////////////////////////////////////////////////////////
BOOL CTelemetryEventsView::OnEraseBkgnd(CDC* pDC)
{
	//return CWnd::OnEraseBkgnd(pDC);
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
float CTelemetryEventsView::GetCurrTime() const
{
	return m_currentTime;
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::SetCurrTime( float time )
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

		Telemetry::CTelemetryRepository* pRepo = CTelemetryViewDialog::GetCurrentInstance()->GetRepository();
		if(pRepo)
		{
			pRepo->SetCurrentSelectedTime(1000 * m_currentTime);
		}

		CRect rc(x1-3,m_rcClient.top,x1+4,m_rcClient.bottom);
		RedrawWindow( rc,NULL,RDW_INVALIDATE );
		CRect rc1(x2-3,m_rcClient.top,x2+4,m_rcClient.bottom);
		RedrawWindow( rc1,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE );
		//GetIEditor()->GetAnimation()->SetTime( m_currentTime );
	}
}

void CTelemetryEventsView::SetStartMarker(float fTime)
{
	m_timeMarked.start=fTime;
	if (m_timeMarked.start<m_timeRange.start)
		m_timeMarked.start=m_timeRange.start;
	if (m_timeMarked.start>m_timeRange.end)
		m_timeMarked.start=m_timeRange.end;
	if (m_timeMarked.start>m_timeMarked.end)
		m_timeMarked.end=m_timeMarked.start;

	Telemetry::CTelemetryRepository* pRepo = CTelemetryViewDialog::GetCurrentInstance()->GetRepository();
	if(pRepo)
	{
		uint64 start = 1000 * m_timeMarked.start;
		uint64 end = 1000 * m_timeMarked.end;
		pRepo->SetFilteredTime(start, end);
	}

	Invalidate();
}
	
void CTelemetryEventsView::SetEndMarker(float fTime)
{
	m_timeMarked.end=fTime;
	if (m_timeMarked.end<m_timeRange.start)
		m_timeMarked.end=m_timeRange.start;
	if (m_timeMarked.end>m_timeRange.end)
		m_timeMarked.end=m_timeRange.end;
	if (m_timeMarked.start>m_timeMarked.end)
		m_timeMarked.start=m_timeMarked.end;

	Telemetry::CTelemetryRepository* pRepo = CTelemetryViewDialog::GetCurrentInstance()->GetRepository();
	if(pRepo)
	{
		uint64 start = 1000 * m_timeMarked.start;
		uint64 end = 1000 * m_timeMarked.end;
		pRepo->SetFilteredTime(start, end);
	}

	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
int CTelemetryEventsView::GetItemRect( int item,CRect &rect ) const
{
	int count = m_pTimelinesCtrl->GetRecordCount();
	if (item < 0 || item >= count)
		return -1;

	int y = -m_scrollOffset.y;
	
	int x = 0;
	for (int i = 0,num = count; i < num && i < item; i++)
	{
		y += TELEMETRY_TIMELINE_ITEM_HEIGHT;
	}
	//int y = item*m_itemHeight - m_scrollOffset.y;
	rect.SetRect( x,y,x+m_rcClient.Width(),y + TELEMETRY_TIMELINE_ITEM_HEIGHT );
	return 0;
}

//////////////////////////////////////////////////////////////////////////
int CTelemetryEventsView::ItemFromPoint( CPoint pnt ) const
{
	CRect rc;
	int num = m_pTimelinesCtrl->GetRecordCount();
	for (int i = 0; i < num; i++)
	{
		GetItemRect(i,rc);
		if (rc.PtInRect(pnt))
			return i;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::SetHorizontalExtent( int min,int max )
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

void CTelemetryEventsView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
// 	if (nChar == VK_UP || nChar == VK_DOWN || nChar == VK_RIGHT || nChar == VK_LEFT)
// 	{
// 		IAnimTrack *pTrack;
// 		int key;
// 		switch (nChar)
// 		{
// 		case VK_UP:
// 			key = GetAboveKey(pTrack);
// 			break;
// 		case VK_DOWN:
// 			key = GetBelowKey(pTrack);
// 			break;
// 		case VK_RIGHT:
// 			key = GetRightKey(pTrack);
// 			break;
// 		case VK_LEFT:
// 			key = GetLeftKey(pTrack);
// 			break;
// 		}
// 		if (key >= 0)
// 		{
// 			UnselectAllKeys(false);
// 			pTrack->SelectKey(key, true);
// 			Invalidate();
// 			SetKeyInfo(pTrack,key);
// 		}
// 		return;
// 	}
// 
// 
// 	if(nChar == 'C')
// 	{
// 		if(GetKeyState(VK_CONTROL))
// 			if(CopyKeys())
// 				return;
// 	}
// 
// 	if(nChar == 'V')
// 	{
// 		if(GetKeyState(VK_CONTROL))
// 		{
// 			StartPasteKeys();
// 			return;
// 		}
// 	}
// 
// 
// 	if(nChar == 'Z')
// 	{
// 		if(GetKeyState(VK_CONTROL))
// 		{
// 			GetIEditor()->Undo();
// 				return;
// 		}
// 	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::ShowEventTooltip( Telemetry::CTimelineRecord* pRecord, CPoint point )
{
	if (m_lastTooltipPos.x == point.x && m_lastTooltipPos.y == point.y)
		return;

	assert(pRecord);

	m_lastTooltipPos = point;

	float time = TimeFromPointUnsnapped(point);
	CPoint prevPixel = point;
	prevPixel.x -= 1;
	float pixelTime = time - TimeFromPointUnsnapped(prevPixel);

	uint64 timeMS = time * 1000;
	uint64 pixelTimeMS = pixelTime * 1000;
	Telemetry::STelemetryEvent* pEvent = NULL;

	for(Telemetry::TTimelines::const_iterator it = pRecord->m_timelines.begin(), end = pRecord->m_timelines.end(); it != end; ++it)
	{
		pEvent = (*it)->GetNearestEventToTime(timeMS, 2 * pixelTimeMS);
		if(pEvent)
			break;
	}

	if(pEvent)
	{
		CString tipText;
		tipText.Format( "%s: %.2fs", pRecord->GetName(), pEvent->timeMs / 1000.0f);
		for(uint i=0; i<pEvent->states.size(); ++i)
		{
			CString temp;
			string temp2 = pEvent->states[i].ToString();
			temp.Format("\n %s: %s", pRecord->m_timelines[0]->GetStateDescription(i), temp2.c_str());
			tipText.Append(temp);
		}

		m_tooltip.UpdateTipText( tipText,this,1 );
		m_tooltip.Activate(TRUE);
	}
	else
	{
		if (m_tooltip.m_hWnd)
			m_tooltip.Activate(FALSE);
	}
}

//////////////////////////////////////////////////////////////////////////
void  CTelemetryEventsView::InvalidateItem( int item )
{
	CRect rc;
	if (GetItemRect(item,rc) != -1)
	{
		InvalidateRect(rc);

	}
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnBeginGameMode:
		m_storedTime = m_currentTime;
		break;
	case eNotify_OnEndGameMode:
		//GetIEditor()->GetAnimation()->SetResetTime( m_storedTime );
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::OnRawInput( UINT wParam,HRAWINPUT lParam )
{
	// Forward raw input to the Track View dialog
	CWnd *pWnd = GetOwner();
	if (pWnd)
		pWnd->SendMessage( WM_INPUT,wParam,(LPARAM)lParam );
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::ClearSelection()
{
// 	for (int i = 0; i < GetCount(); i++)
// 	{
// 		m_tracks[i].bSelected = false;
// 	}
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::SelectItem( int item )
{
// 	if (item >= 0 && item < GetCount())
// 	{
// 		m_tracks[item].bSelected = true;
// 	}
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
bool CTelemetryEventsView::IsSelectedItem( int item )
{
// 	if (item >= 0 && item < GetCount())
// 	{
// 		return m_tracks[item].bSelected;
// 	}
	return false;
}

void CTelemetryEventsView::MouseMoveSelect( CPoint point )
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

void CTelemetryEventsView::MouseMoveMove( CPoint point, UINT nFlags )
{
	SetMouseCursor(m_crsLeftRight);
	if (point.x < m_rcClient.left)
		point.x = m_rcClient.left;
	if (point.x > m_rcClient.right)
		point.x = m_rcClient.right;
	CPoint ofs = point - m_mouseDownPos;

// 	if (m_mouseActionMode == TeVMODE_SCALEKEY)
// 	{
// 		// Offset all selected keys back by previous offset.
// 		if (m_keyTimeOffset != 0)
// 			ScaleSelectedKeys( 1.0f/(1+m_keyTimeOffset),bTickSnap );
// 	}
// 	else
// 	{
// 		bool bSlide = false;
// 		if (m_mouseActionMode == TeVMODE_SLIDEKEY)
// 			bSlide = true;
// 
// 		// Offset all selected keys back by previous offset.
// 		if (m_keyTimeOffset != 0)
// 			OffsetSelectedKeys( -m_keyTimeOffset,bSlide,bTickSnap );
// 	}

	float newTime;
	//float oldTime;
	newTime = TimeFromPointUnsnapped(point);

// 	IAnimNode *node = 0;
// 	int key = FirstKeyFromPoint(m_mouseDownPos);
// 	if (key >= 0)
// 	{
// 		int item = ItemFromPoint(m_mouseDownPos);
// 		IAnimTrack *track = GetTrack(item);
// 		node = GetNode(item);
// 
// 		oldTime = track->GetKeyTime(key);
// 	}
// 	else
// 	{
// 		oldTime = TimeFromPointUnsnapped(m_mouseDownPos);
// 	}
	
// 	Snap it, if necessary.
// 		if (snappingMode == TeVKEY_SNAP_MAGNET)
// 		{
// 			newTime = MagnetSnap(newTime, node);
// 		}
// 		else if (snappingMode == TeVKEY_SNAP_FRAME)
// 		{
// 			newTime = FrameSnap(newTime);
// 		}
// 		else if (snappingMode == TeVKEY_SNAP_TICK)
// 		{
// 			newTime = TickSnap(newTime);
// 		}
// 	
// 		m_realTimeRange.ClipValue( newTime );
// 	
// 		float timeOffset = newTime - oldTime;
// 	
// 		if (m_mouseActionMode == TeVMODE_SCALEKEY)
// 		{
// 			float tscale = 0.005f;
// 			float tofs = ofs.x * tscale;
// 			// Offset all selected keys by this offset.
// 			ScaleSelectedKeys( 1+tofs,bTickSnap );
// 			m_keyTimeOffset = tofs;
// 		}
// 		else
// 		{
// 			bool bSlide = false;
// 			if (m_mouseActionMode == TeVMODE_SLIDEKEY)
// 				bSlide = true;
// 	
// 			// Offset all selected keys by this offset.
// 			OffsetSelectedKeys( timeOffset,bSlide,bTickSnap );
// 			m_keyTimeOffset = timeOffset;
// 		}
	Invalidate();
}

void CTelemetryEventsView::MouseMoveDragTime( CPoint point, UINT nFlags )
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
	m_realTimeRange.ClipValue( time );
	if (!bNoSnap)
		time = TickSnap(time);
	SetCurrTime( time );
}

void CTelemetryEventsView::MouseMoveDragStartMarker( CPoint point, UINT nFlags )
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
	m_realTimeRange.ClipValue( time );
	if (!bNoSnap)
		time = TickSnap(time);
	SetStartMarker( time );
}

void CTelemetryEventsView::MouseMoveDragEndMarker( CPoint point, UINT nFlags )
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
	m_realTimeRange.ClipValue( time );
	if (!bNoSnap)
		time = TickSnap(time);
	SetEndMarker( time );
}

void CTelemetryEventsView::MouseMovePaste( CPoint point, UINT nFlags )
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
/*	PasteKeys( 0,0,fTimeOffset );*/
}

void CTelemetryEventsView::MouseMoveOver( CPoint point )
{
	// No mouse mode.
	SetMouseCursor(NULL);

	uint record = ItemFromPoint(point);
	Telemetry::CTimelineRecord* pRecord = m_pTimelinesCtrl->GetRecord(record);
	if(pRecord && !pRecord->m_timelines.empty())
	{
		ShowEventTooltip( pRecord, point );
	}
 	else
 	{
		if (m_tooltip.m_hWnd)
 			m_tooltip.Activate(FALSE);
 	}
}

float CTelemetryEventsView::MagnetSnap( float newTime, const IAnimNode * node ) const
{
// 	CTelemetryViewUtils::SelectedKeys selectedKeys;
// 	CTelemetryViewUtils::GetKeysInTimeRange(m_pSequence, selectedKeys, 
// 		newTime - MARGIN_FOR_MAGNET_SNAPPING/m_timeScale, 
// 		newTime + MARGIN_FOR_MAGNET_SNAPPING/m_timeScale);
// 	if (selectedKeys.keys.size() > 0)
// 	{
// 		// By default, just use the first key that belongs to the time range as a magnet.
// 		newTime = selectedKeys.keys[0].pTrack->GetKeyTime(selectedKeys.keys[0].nKey);
// 		// But if there is an in-range key in a sibling track, use it instead.
// 		// Here a 'sibling' means a track that belongs to a same node.
// 		for (int i=0; i<selectedKeys.keys.size(); ++i)
// 		{
// 			if (selectedKeys.keys[i].pNode == node)
// 			{
// 				newTime = selectedKeys.keys[i].pTrack->GetKeyTime(selectedKeys.keys[i].nKey);
// 				break;
// 			}
// 		}
// 	}
	return newTime;
}

//////////////////////////////////////////////////////////////////////////
float CTelemetryEventsView::FrameSnap( float time ) const
{
	double t = floor( (double)time/m_snapFrameTime + 0.5);
	t = t * m_snapFrameTime;
	return t;
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::ShowKeyPropertyCtrlOnSpot(int x, int y, bool bMultipleKeysSelected, bool bKeyChangeInSameTrack)
{
	//if(m_keyPropertiesDlg == NULL)
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
		//m_keyPropertiesDlg->PopulateVariables(m_wndPropsOnSpot);
	}
	if (bMultipleKeysSelected)
		m_wndPropsOnSpot.SetDisplayOnlyModified(true);
	else
		m_wndPropsOnSpot.SetDisplayOnlyModified(false);
	m_wndPropsOnSpot.ShowWindow(SW_SHOW);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::HideKeyPropertyCtrlOnSpot()
{
	if (m_wndPropsOnSpot.m_hWnd)
	{
		m_wndPropsOnSpot.ShowWindow(SW_HIDE);
		m_wndPropsOnSpot.ClearSelection();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryEventsView::SetScrollOffset(int hpos)
{
	SetScrollPos( SB_HORZ,hpos );
	m_scrollOffset.x = hpos;
	Invalidate();
}
