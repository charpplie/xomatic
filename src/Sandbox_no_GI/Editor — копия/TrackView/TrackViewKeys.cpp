////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   TrackViewKeys.cpp
//  Version:     v1.00
//  Created:     23/8/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "TrackViewKeys.h"
#include "Controls\MemDC.h"

#include "TrackViewDialog.h"
#include "AnimationContext.h"
#include "TrackViewUndo.h"

#include "Clipboard.h"

//#include "IMovieSystem.h"

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

static const int MARGIN_FOR_MAGNET_SNAPPING = 10;

// CTrackViewKeys

IMPLEMENT_DYNAMIC(CTrackViewKeys, CWnd)

CTrackViewKeys::CTrackViewKeys()
{
	m_pSequence = 0;
	m_wndTrack = NULL;
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
}

CTrackViewKeys::~CTrackViewKeys()
{
	m_descriptionFont->DeleteObject();
	delete m_descriptionFont;
}


BEGIN_MESSAGE_MAP(CTrackViewKeys, CWnd)
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
int CTrackViewKeys::OnCreate(LPCREATESTRUCT lpCreateStruct)
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
void CTrackViewKeys::OnDestroy()
{
	GetIEditor()->UnregisterNotifyListener( this );
	CWnd::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::DrawSelectedKeyIndicators( CDC *dc )
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
void CTrackViewKeys::DrawTimeline( CDC *dc,const CRect &rcUpdate )
{
	CRect rc,temprc;
	
	bool recording = GetIEditor()->GetAnimation()->IsRecording();

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
			sprintf( str,"%g",st );
			dc->TextOut( x+2,rc.top,str );
			dc->SelectObject(ltgray);
		}
		else
			dc->LineTo(x,rc.bottom-6);
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
void CTrackViewKeys::DrawSummary( CDC *dc, CRect rcUpdate )
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
int CTrackViewKeys::TimeToClient( float time ) const
{
	int x = m_leftOffset - m_scrollOffset.x + time*m_timeScale;
	return x;
}

//////////////////////////////////////////////////////////////////////////
Range CTrackViewKeys::GetVisibleRange()
{
	Range r;
	r.start = (m_scrollOffset.x - m_leftOffset)/m_timeScale;
	r.end = r.start + (m_rcClient.Width())/m_timeScale;
	// Intersect range with global time range.
	r = m_timeRange & r;
	return r;
}

//////////////////////////////////////////////////////////////////////////
Range CTrackViewKeys::GetTimeRange( CRect &rc )
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
void CTrackViewKeys::DrawTicks( CDC *dc,CRect &rc,Range &timeRange )
{
	// Draw time ticks every tick step seconds.
	CPen ltgray(PS_SOLID,1,RGB(90,90,90));
	CPen *prevPen = dc->SelectObject( &ltgray );
	Range VisRange=GetVisibleRange();
	int nNumberTicks=10;
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
void CTrackViewKeys::DrawKeys( IAnimTrack *track,CDC *dc,CRect &rc,Range &timeRange )
{
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::RedrawItem( int item )
{
	CRect rc;
	if (GetItemRect( item,rc ) != LB_ERR)
	{
		RedrawWindow( rc,NULL,RDW_INVALIDATE|RDW_ERASE );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	lpMIS->itemWidth = 1000;
  lpMIS->itemHeight = 16;
}

//////////////////////////////////////////////////////////////////////////
HBRUSH CTrackViewKeys::CtlColor(CDC* pDC, UINT nCtlColor)
{
	return m_bkgrBrush;

	// TODO:  Return a non-NULL brush if the parent's handler should not be called
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::SetTimeRange( float start,float end )
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
void CTrackViewKeys::SetTimeScale( float timeScale,float fAnchorTime )
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
	m_ticksStep = 10;
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
void CTrackViewKeys::OnSize(UINT nType, int cx, int cy)
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
BOOL CTrackViewKeys::PreTranslateMessage(MSG* pMsg)
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
BOOL CTrackViewKeys::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
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
void CTrackViewKeys::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
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
float CTrackViewKeys::TickSnap( float time ) const
{
	double t = floor( (double)time*m_ticksStep + 0.5);
	t = t / m_ticksStep;
	return t;
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewKeys::TimeFromPoint( CPoint point ) const
{
	int x = point.x - m_leftOffset + m_scrollOffset.x;
	double t = (double)x / m_timeScale;
	return (float)TickSnap(t);
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewKeys::TimeFromPointUnsnapped( CPoint point ) const
{
	int x = point.x - m_leftOffset + m_scrollOffset.x;
	double t = (double)x / m_timeScale;
	return t;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::AddItem( const Item &item )
{
	m_tracks.push_back(item);
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
const CTrackViewKeys::Item& CTrackViewKeys::GetItem( int item ) const
{
	return m_tracks[item];
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CTrackViewKeys::GetTrack( int item ) const
{
	if (item < 0 || item >= GetCount())
		return 0;
	IAnimTrack *track = m_tracks[item].track;
	return track;
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CTrackViewKeys::GetNode( int item ) const
{
	if (item < 0 || item >= GetCount())
		return 0;
	return m_tracks[item].node;
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewKeys::FirstKeyFromPoint( CPoint point ) const
{
	return -1;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::OnLButtonDown(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonDown(nFlags, point);

	SetFocus();

	m_mouseDownPos = point;

	//int item = ItemFromPoint(point);
	//SetCurSel(item);

	if (m_mouseMode == MOUSE_MODE_PASTE_DRAGTIME)
	{
		GetIEditor()->AcceptUndo( "Paste Keys" );
		SetMouseCursor(NULL);
		ReleaseCapture();
		m_mouseMode = MOUSE_MODE_NONE;
		return;
	}

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

	int key = FirstKeyFromPoint(point);
	if (key >= 0)
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
		SetKeyInfo( track,key );
		return;
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
void CTrackViewKeys::OnLButtonUp(UINT nFlags, CPoint point)
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
		
	if (m_bAnySelected)
	{
		IAnimTrack *track = 0;
		int key = 0;
		if (FindSingleSelectedKey(track,key))
		{
			SetKeyInfo(track,key);
		}
	}
	m_keyTimeOffset = 0;

	//if (GetIEditor()->IsUndoRecording())
	//GetIEditor()->AcceptUndo( "Track Modify" );

	/// Move/Clone Key Undo End
	if(m_mouseMode == MOUSE_MODE_MOVE || m_mouseMode == MOUSE_MODE_CLONE)
		GetIEditor()->AcceptUndo("Move/Clone Keys");

	m_mouseMode = MOUSE_MODE_NONE;
	CWnd::OnLButtonUp(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_rcTimeline.PtInRect(point))
	{
		// Clicked inside timeline.
		return;
	}

	int key = FirstKeyFromPoint(point);
	if (key >= 0)
	{
		int item = ItemFromPoint(point);
		IAnimTrack *track = GetTrack(item);
		UnselectAllKeys(false);
		track->SelectKey(key, true);
		m_bAnySelected = true;
		m_keyTimeOffset = 0;
		Invalidate();

		SetKeyInfo( track,key,true );
		return;
	}

	// Add key here.
	int item = ItemFromPoint(point);
	IAnimTrack *track = GetTrack(item);
	IAnimNode *node = GetNode(item);
	if (track) 
	{
		bool keyCreated = false;
		float keyTime = TimeFromPoint(point);
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
void CTrackViewKeys::OnMButtonDown(UINT nFlags, CPoint point)
{
	OnRButtonDown(nFlags,point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::OnMButtonUp(UINT nFlags, CPoint point)
{
	OnRButtonUp(nFlags,point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::OnRButtonDown(UINT nFlags, CPoint point)
{
	//CWnd::OnRButtonDown(nFlags, point);

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
	if (key >= 0)
	{
		m_bCursorWasInKey = true;

		int item = ItemFromPoint(point);
		IAnimTrack *track = GetTrack(item);
		track->SelectKey(key, true);
		m_bAnySelected = true;
		m_keyTimeOffset = 0;
		Invalidate();

		if (FindSingleSelectedKey(track,key))
		{
			SetKeyInfo(track,key,true);
		}

		// Show a little pop-up menu for copy & delete.
		enum { COPY_KEY=100, DELETE_KEY };
		CMenu menu;
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, COPY_KEY, "Copy");
		menu.AppendMenu(MF_SEPARATOR, 0 ,"");
		menu.AppendMenu(MF_STRING, DELETE_KEY, "Delete");

		CPoint p;
		::GetCursorPos(&p);
		int cmd = menu.TrackPopupMenu(TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_LEFTBUTTON, p.x,p.y, this, NULL);
		if(cmd == COPY_KEY)
			CopyKeys();
		else if(cmd == DELETE_KEY)
			DelSelectedKeys();
	}else
	{
		m_bMoveDrag=true;
		SetCapture();
	}
}

void CTrackViewKeys::OnRButtonUp(UINT nFlags, CPoint point)
{
	m_bZoomDrag=false;
	m_bMoveDrag=false;

	if (GetCapture() == this)
	{
		ReleaseCapture();
	}
	m_mouseMode = MOUSE_MODE_NONE;

	if(m_bCursorWasInKey == false)
	{
		bool bHasCopiedKey = true;
		CClipboard clip;
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
void CTrackViewKeys::OnMouseMove(UINT nFlags, CPoint point)
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
void CTrackViewKeys::OnPaint()
{
//	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
//	CWnd::OnPaint();

	CPaintDC  PaintDC(this);
	CMemDC dc( PaintDC,&m_offscreenBitmap );

	XTPPaintManager()->GradientFill( &dc,&PaintDC.m_ps.rcPaint,RGB(250,250,250),RGB(220,220,220),FALSE );
	//dc->FillRect( &PaintDC.m_ps.rcPaint,&m_bkgrBrush );

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
void CTrackViewKeys::DrawTrack( int item,CDC *dc,CRect &rcItem )
{
}


//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::DrawControl( CDC *dc,const CRect &rcUpdate )
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
void CTrackViewKeys::UnselectAllKeys( bool bNotify )
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
void CTrackViewKeys::SelectKeys( const CRect &rc )
{}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::SelectAllKeysWithinTimeFrame( const CRect &rc )
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
void CTrackViewKeys::DelSelectedKeys()
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
void CTrackViewKeys::OffsetSelectedKeys( float timeOffset,bool bSlide,bool bSnap )
{
	IAnimTrack *pTrack = 0;
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
	for (int k = 0; k<(int)selectedKeys.keys.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];
		if(pTrack != skey.pTrack)
		{
			CUndo::Record( new CUndoTrackObject(skey.pTrack) );
			pTrack = skey.pTrack;
		}

		OffsetKey(skey.pTrack, skey.nKey, timeOffset);
	}
	// Currently, no support for 'sliding'.
	bSlide;	// Sliding will be deprecated in the end...

	UpdateAnimation();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::ScaleSelectedKeys( float timeOffset,bool bSnapKeys )
{
	if (timeOffset <= 0)
		return;
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
	for (int k = 0; k<(int)selectedKeys.keys.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];
		float keyt = skey.pTrack->GetKeyTime(skey.nKey) * timeOffset;
		if (bSnapKeys)
			keyt = TickSnap(keyt);
		skey.pTrack->SetKeyTime( skey.nKey,keyt );
	}
	UpdateAnimation();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::CloneSelectedKeys()
{
	CTrackViewUtils::SelectedKeys selectedKeys;
	CTrackViewUtils::GetSelectedKeys(m_pSequence, selectedKeys);
	IAnimTrack *pTrack = 0;
	for (int k = 0; k<(int)selectedKeys.keys.size(); ++k)
	{
		CTrackViewUtils::SelectedKey skey = selectedKeys.keys[k];
		if(pTrack != skey.pTrack)
		{
			CUndo::Record( new CUndoTrackObject(skey.pTrack) );
			pTrack = skey.pTrack;
		}

		int newKey = skey.pTrack->CloneKey(skey.nKey);
		// Select new key.
		skey.pTrack->SelectKey(newKey, true);
		// Unselect cloned key.
		skey.pTrack->SelectKey(skey.nKey, false);
	}
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewKeys::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_currCursor != NULL)
	{
		return 0;
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CTrackViewKeys::SetMouseCursor( HCURSOR crs )
{
	m_currCursor = crs;
	if (m_currCursor != NULL)
		SetCursor(crs);
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewKeys::OnEraseBkgnd(CDC* pDC)
{
	//return CWnd::OnEraseBkgnd(pDC);
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewKeys::GetCurrTime() const
{
	return m_currentTime;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::SetCurrTime( float time )
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

void CTrackViewKeys::SetStartMarker(float fTime)
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
	
void CTrackViewKeys::SetEndMarker(float fTime)
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
void CTrackViewKeys::SetMouseActionMode( ETVActionMode mode )
{
	m_mouseActionMode = mode;
	if (mode == TVMODE_ADDKEY)
	{
		SetMouseCursor( m_crsAddKey );
	}
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewKeys::CanCopyPasteKeys()
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
bool CTrackViewKeys::CopyPasteKeys()
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
void CTrackViewKeys::SetKeyInfo( IAnimTrack *track,int key,bool openWindow )
{
	EAnimTrackType trackType = track->GetType();
	
	if (openWindow && m_wndTrack != 0 && !::IsWindow(m_wndTrack->m_hWnd))
	{
		m_wndTrack->Create( CTVTrackPropsDialog::IDD,GetParent() );
	}
	if (m_wndTrack && m_wndTrack->m_hWnd != 0 && ::IsWindow(m_wndTrack->m_hWnd))
	{
		int paramId;
		IAnimNode *node = 0;
		for (int i = 0; i < GetCount(); i++)
		{
			if (m_tracks[i].track == track)
			{
				node = m_tracks[i].node;
				paramId = m_tracks[i].paramId;
				break;
			}
		}
		if (node)
			m_wndTrack->SetKey( node,paramId,track,key );
	}

	NotifyKeySelectionUpdate();

}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewKeys::FindSingleSelectedKey( IAnimTrack* &selTrack,int &selKey )
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
int CTrackViewKeys::GetItemRect( int item,CRect &rect ) const
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
int CTrackViewKeys::ItemFromPoint( CPoint pnt ) const
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
void CTrackViewKeys::SetHorizontalExtent( int min,int max )
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
void CTrackViewKeys::UpdateAnimation()
{
	GetIEditor()->GetAnimation()->ForceAnimation();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewKeys::CopyKeys()
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
		return false;

	// Get Selected Track.
	CTrackViewUtils::SelectedTracks selectedTracks;
	CTrackViewUtils::GetSelectedTracks( m_pSequence,selectedTracks );
	if (!bCopySelected 
			&& CTrackViewUtils::IsOneTrackSelected(selectedTracks) == false)
		return false;

	XmlNodeRef copyNode = CreateXmlNode("CopyKeysNode");

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
			if(pCurrTrack->GetNumKeys()>0)
				pCurrTrack->SerializeSelection(childNode, false, false);
		}
	}

	CClipboard clip;
	clip.Put(copyNode, "Track view keys");

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::StartPasteKeys()
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

	PasteKeys(0,0);
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewKeys::PasteKeys( IAnimNode * pAnimNode, float fTimeOffset )
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
			trackNode->getAttr("paramId", paramId);

			IAnimTrack *pTrack = pAnimNode->GetTrackForParameter(paramId);
			if (!pTrack)
			{
				// Make this track.
				pTrack = pAnimNode->CreateTrack(paramId);
				if (pTrack)
					bNewTrackCreated = true;
			}
			if(pTrack)
			{
				CUndo::Record( new CUndoTrackObject(pTrack) );
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





void CTrackViewKeys::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
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
			SetKeyInfo(pTrack,key);
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
void CTrackViewKeys::RecordTrackUndo( const Item &item )
{
	if (item.track != 0)
	{
		CUndo undo("Track Modify");
		CUndo::Record( new CUndoTrackObject(item.track) );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::ShowKeyTooltip( IAnimTrack *pTrack,int nKey,CPoint point )
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
	tipText.Format( "%.3f, {%s}",time,desc );
	m_tooltip.UpdateTipText( tipText,this,1 );
	m_tooltip.Activate(TRUE);
}

//////////////////////////////////////////////////////////////////////////
void  CTrackViewKeys::InvalidateItem( int item )
{
	CRect rc;
	if (GetItemRect(item,rc) != -1)
	{
		InvalidateRect(rc);

	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::OnEditorNotifyEvent( EEditorNotifyEvent event )
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
void CTrackViewKeys::OnRawInput( UINT wParam,HRAWINPUT lParam )
{
	// Forward raw input to the Track View dialog
	CWnd *pWnd = GetOwner();
	if (pWnd)
		pWnd->SendMessage( WM_INPUT,wParam,(LPARAM)lParam );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::ClearSelection()
{
	for (int i = 0; i < GetCount(); i++)
	{
		m_tracks[i].bSelected = false;
	}
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::SelectItem( int item )
{
	if (item >= 0 && item < GetCount())
	{
		m_tracks[item].bSelected = true;
	}
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewKeys::IsSelectedItem( int item )
{
	if (item >= 0 && item < GetCount())
	{
		return m_tracks[item].bSelected;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::OffsetKey( IAnimTrack *track, int keyIndex, float timeOffset )
{
	//float keyt = SnapTime(track->GetKeyTime(keyIndex) + timeOffset);
	float keyt = track->GetKeyTime(keyIndex) + timeOffset;
	//if (bSnap)
	//keyt = SnapTime(keyt);

	track->SetKeyTime( keyIndex,keyt );	
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeys::NotifyKeySelectionUpdate()
{
	GetIEditor()->Notify( eNotify_OnUpdateTVKeySelection );

	// In the end, focus this again in order to properly catch 'KeyDown' messages.
	SetFocus();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewKeys::IsOkToAddKeyHere(const IAnimTrack *pTrack, float time) const
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

void CTrackViewKeys::MouseMoveSelect( CPoint point )
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

static bool CheckVirtualKey( int virtualKey )
{
	GetAsyncKeyState(virtualKey);
	if (GetAsyncKeyState(virtualKey))
		return true;
	return false;
}

void CTrackViewKeys::MouseMoveMove( CPoint point, UINT nFlags )
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
			ScaleSelectedKeys( 1.0f/(1+m_keyTimeOffset),bTickSnap );
	}
	else
	{
		bool bSlide = false;
		if (m_mouseActionMode == TVMODE_SLIDEKEY)
			bSlide = true;

		// Offset all selected keys back by previous offset.
		if (m_keyTimeOffset != 0)
			OffsetSelectedKeys( -m_keyTimeOffset,bSlide,bTickSnap );
	}

	float newTime;
	float oldTime;
	newTime = TimeFromPointUnsnapped(point);

	IAnimNode *node = 0;
	int key = FirstKeyFromPoint(m_mouseDownPos);
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

	m_realTimeRange.ClipValue( newTime );

	float timeOffset = newTime - oldTime;

	if (m_mouseActionMode == TVMODE_SCALEKEY)
	{
		float tscale = 0.005f;
		float tofs = ofs.x * tscale;
		// Offset all selected keys by this offset.
		ScaleSelectedKeys( 1+tofs,bTickSnap );
		m_keyTimeOffset = tofs;
	}
	else
	{
		bool bSlide = false;
		if (m_mouseActionMode == TVMODE_SLIDEKEY)
			bSlide = true;

		// Offset all selected keys by this offset.
		OffsetSelectedKeys( timeOffset,bSlide,bTickSnap );
		m_keyTimeOffset = timeOffset;
	}
	Invalidate();
}

void CTrackViewKeys::MouseMoveDragTime( CPoint point, UINT nFlags )
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

void CTrackViewKeys::MouseMoveDragStartMarker( CPoint point, UINT nFlags )
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

void CTrackViewKeys::MouseMoveDragEndMarker( CPoint point, UINT nFlags )
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

void CTrackViewKeys::MouseMovePaste( CPoint point, UINT nFlags )
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
	PasteKeys( 0,fTimeOffset );
}

void CTrackViewKeys::MouseMoveOver( CPoint point )
{
	// No mouse mode.
	SetMouseCursor(NULL);
	int key = FirstKeyFromPoint(point);
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

int CTrackViewKeys::GetAboveKey(IAnimTrack *&aboveTrack)
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

int CTrackViewKeys::GetBelowKey(IAnimTrack *&belowTrack)
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

int CTrackViewKeys::GetRightKey(IAnimTrack *&track)
{
	int keyIndex;
	if (FindSingleSelectedKey(track, keyIndex) == false)
		return -1;


	return (keyIndex + 1) % track->GetNumKeys();
}

int CTrackViewKeys::GetLeftKey(IAnimTrack *&track)
{
	int keyIndex;
	if (FindSingleSelectedKey(track, keyIndex) == false)
		return -1;

	return (keyIndex + track->GetNumKeys() - 1) % track->GetNumKeys();
}

float CTrackViewKeys::MagnetSnap( float newTime, const IAnimNode * node ) const
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
float CTrackViewKeys::FrameSnap( float time ) const
{
	double t = floor( (double)time/m_snapFrameTime + 0.5);
	t = t * m_snapFrameTime;
	return t;
}