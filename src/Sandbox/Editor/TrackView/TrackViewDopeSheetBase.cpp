////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002 - 2012.
// -------------------------------------------------------------------------
//  File name:   TrackViewDopeSheetBase.cpp
//  Created:     23/8/2002 by Timur.
// -------------------------------------------------------------------------
//  History: May 2012 - Axel Gneiting - Refactoring
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TrackViewDopeSheetBase.h"
#include "Controls\MemDC.h"

#include "TrackViewDialog.h"
#include "AnimationContext.h"
#include "TrackViewUndo.h"
#include "TVCustomizeTrackColorsDlg.h"
#include "TrackViewAnimNode.h"
#include "TrackViewTrack.h"
#include "TrackViewSequence.h"

#include "Clipboard.h"

#include <afxcolordialog.h>

#define EDIT_DISABLE_GRAY_COLOR RGB(180,180,180)
#define KEY_TEXT_COLOR RGB(0,0,50)
#define INACTIVE_TEXT_COLOR RGB(128,128,128)

namespace
{
	const int kMarginForMagnetSnapping = 10;
	const unsigned int kDefaultTrackHeight = 16;
}

enum ETVMouseMode
{
  eTVMouseMode_None = 0,
  eTVMouseMode_Select = 1,
  eTVMouseMode_Move,
  eTVMouseMode_Clone,
  eTVMouseMode_DragTime,
  eTVMouseMode_DragStartMarker,
  eTVMouseMode_DragEndMarker,
  eTVMouseMode_Paste,
  eTVMouseMode_SelectWithinTime,
  eTVMouseMode_StartTimeAdjust,
  eTVMouseMode_EndTimeAdjust
};

IMPLEMENT_DYNAMIC(CTrackViewDopeSheetBase, CWnd)

//////////////////////////////////////////////////////////////////////////
CTrackViewDopeSheetBase::CTrackViewDopeSheetBase()
{
	m_bkgrBrush.CreateSolidBrush(GetSysColor(COLOR_3DFACE));
	m_bkgrBrushEmpty.CreateSolidBrush(RGB(190, 190, 190));
	m_timeBkgBrush.CreateSolidBrush(RGB(0xE0, 0xE0, 0xE0));
	m_timeHighlightBrush.CreateSolidBrush(RGB(0xFF, 0x0, 0x0));
	m_selectedBrush.CreateSolidBrush(RGB(200, 200, 230));
	m_visibilityBrush.CreateSolidBrush(RGB(120, 120, 255));
	m_selectTrackBrush.CreateSolidBrush(RGB(100, 190, 255));

	m_timeScale = 1.0f;
	m_ticksStep = 10;

	m_bZoomDrag = false;
	m_bMoveDrag = false;

	m_leftOffset = 30;
	m_scrollOffset = CPoint(0, 0);
	m_mouseMode = eTVMouseMode_None;
	m_currentTime = 0.0f;
	m_storedTime = m_currentTime;
	m_rcSelect = CRect(0, 0, 0, 0);
	m_keyTimeOffset = 0;
	m_currCursor = NULL;
	m_mouseActionMode = eTVActionMode_MoveKey;

	m_scrollMin = 0;
	m_scrollMax = 1000;

	m_descriptionFont = new CFont();
	m_descriptionFont->CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
	                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Verdana");

	m_bCursorWasInKey = false;
	m_bJustSelected = false;
	m_snappingMode = eSnappingMode_SnapNone;
	m_snapFrameTime = 0.033333f;
	m_bMouseMovedAfterRButtonDown = false;

	m_tickDisplayMode = eTVTickMode_InSeconds;

	m_bEditLock = false;

	m_bFastRedraw = false;

	m_pLastTrackSelectedOnSpot = NULL;


#ifdef DEBUG
	m_redrawCount = 0;
#endif
	m_bKeysMoved = false;
	ComputeFrameSteps(GetVisibleRange());
}

//////////////////////////////////////////////////////////////////////////
CTrackViewDopeSheetBase::~CTrackViewDopeSheetBase()
{
	m_descriptionFont->DeleteObject();
	delete m_descriptionFont;
}

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CTrackViewDopeSheetBase, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
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

//////////////////////////////////////////////////////////////////////////
int CTrackViewDopeSheetBase::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	m_imageList.Create(MAKEINTRESOURCE(IDB_TRACKVIEW_KEYS), 14, 0, RGB(255, 0, 255));
	m_imgMarker.Create(MAKEINTRESOURCE(IDB_MARKER), 8, 0, RGB(255, 0, 255));
	m_crsLeftRight = AfxGetApp()->LoadStandardCursor(IDC_SIZEWE);
	m_crsAddKey = AfxGetApp()->LoadCursor(IDC_ARROW_ADDKEY);
	m_crsCross = AfxGetApp()->LoadCursor(IDC_POINTER_OBJHIT);
	m_crsAdjustLR = AfxGetApp()->LoadCursor(IDC_LEFTRIGHT);

	GetIEditor()->GetAnimation()->AddListener(this);

	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnDestroy()
{
	HideKeyPropertyCtrlOnSpot();
	GetIEditor()->GetAnimation()->RemoveListener(this);
	CWnd::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewDopeSheetBase::TimeToClient(float time) const
{
	int x = m_leftOffset - m_scrollOffset.x + (time * m_timeScale);
	return x;
}

//////////////////////////////////////////////////////////////////////////
Range CTrackViewDopeSheetBase::GetVisibleRange() const
{
	Range r;
	r.start = (m_scrollOffset.x - m_leftOffset) / m_timeScale;
	r.end = r.start + (m_rcClient.Width()) / m_timeScale;

	Range extendedTimeRange(0.0f, m_timeRange.end);
	r = extendedTimeRange & r;

	return r;
}

//////////////////////////////////////////////////////////////////////////
Range CTrackViewDopeSheetBase::GetTimeRange(CRect &rc) const
{
	Range r;
	r.start = (rc.left - m_leftOffset + m_scrollOffset.x) / m_timeScale;
	r.end = r.start + (rc.Width()) / m_timeScale;

	r.start = TickSnap(r.start);
	r.end = TickSnap(r.end);

	// Intersect range with global time range.
	r = m_timeRange & r;

	return r;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetTimeRange(float start, float end)
{
	if (m_timeMarked.start < start)
	{
		m_timeMarked.start = start;
	}
	if (m_timeMarked.end > end)
	{
		m_timeMarked.end = end;
	}

	m_timeRange.Set(start, end);

	SetHorizontalExtent(-m_leftOffset, m_timeRange.end * m_timeScale - m_leftOffset);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetTimeScale(float timeScale, float fAnchorTime)
{
	const double fOldOffset = -fAnchorTime * m_timeScale;
	const double fOldScale = m_timeScale;

	timeScale = std::max(timeScale, 0.001f);
	timeScale = std::min(timeScale, 100000.0f);
	m_timeScale = timeScale;
	
	int steps = 0;
	if (GetTickDisplayMode() == eTVTickMode_InSeconds)
	{
		m_ticksStep = 10;
	}
	else if (GetTickDisplayMode() == eTVTickMode_InFrames)
	{
		m_ticksStep = 1 / m_snapFrameTime;
	}
	else
	{
		assert(0);
	}

	double fPixelsPerTick;
	do
	{
		fPixelsPerTick = (1.0 / m_ticksStep) * (double)m_timeScale;

		if (fPixelsPerTick < 6.0)
		{
			m_ticksStep /= 2;
		}

		if (m_ticksStep <= 0)
		{
			m_ticksStep = 1;
			break;
		}
		steps++;
	}
	while (fPixelsPerTick < 6.0 && steps < 100);

	steps = 0;

	do
	{
		fPixelsPerTick = (1.0 / m_ticksStep) * (double)m_timeScale;
		if (fPixelsPerTick >= 12.0)
		{
			m_ticksStep *= 2;
		}
		if (m_ticksStep <= 0)
		{
			m_ticksStep = 1;
			break;
		}
		steps++;
	}
	while (fPixelsPerTick >= 12.0 && steps < 100);

	float fCurrentOffset = -fAnchorTime * m_timeScale;
	m_scrollOffset.x += fOldOffset - fCurrentOffset;

	Invalidate();

	SetHorizontalExtent(-m_leftOffset, m_timeRange.end * m_timeScale);

	ComputeFrameSteps(GetVisibleRange());
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	GetClientRect(m_rcClient);

	if (m_offscreenBitmap.GetSafeHandle() != NULL)
	{
		m_offscreenBitmap.DeleteObject();
	}

	CDC *pDC = GetDC();
	m_offscreenBitmap.CreateCompatibleBitmap(pDC, m_rcClient.Width(), m_rcClient.Height());
	ReleaseDC(pDC);

	GetClientRect(m_rcTimeline);
	m_rcTimeline.bottom = m_rcTimeline.top + kDefaultTrackHeight;
	m_rcSummary = m_rcTimeline;
	m_rcSummary.top = m_rcTimeline.bottom;
	m_rcSummary.bottom = m_rcSummary.top + 8;

	SetHorizontalExtent(m_scrollMin, m_scrollMax);

	if (m_tooltip.m_hWnd)
	{
		m_tooltip.DelTool(this, 1);
		m_tooltip.AddTool(this, "", m_rcClient, 1);
	}
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewDopeSheetBase::PreTranslateMessage(MSG* pMsg)
{
	if (!m_tooltip.m_hWnd)
	{
		CRect rc;
		GetClientRect(rc);
		m_tooltip.Create(this);
		m_tooltip.SetDelayTime(TTDT_AUTOPOP, 5000);
		m_tooltip.SetDelayTime(TTDT_INITIAL, 0);
		m_tooltip.SetDelayTime(TTDT_RESHOW, 0);
		m_tooltip.SetMaxTipWidth(600);
		m_tooltip.AddTool(this, "", rc, 1);
		m_tooltip.Activate(FALSE);
	}
	m_tooltip.RelayEvent(pMsg);

	return __super::PreTranslateMessage(pMsg);
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewDopeSheetBase::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	if (!pSequence)
	{
		return FALSE;
	}

	float z = (zDelta > 0) ? (m_timeScale * 1.25f) : (m_timeScale * 0.8f);

	GetCursorPos(&pt);
	ScreenToClient(&pt);

	float fAnchorTime = TimeFromPointUnsnapped(pt);
	SetTimeScale(z, fAnchorTime);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO si;
	GetScrollInfo(SB_HORZ, &si);

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
		{
			curpos--;
		}
		break;

	case SB_LINERIGHT:   // Scroll right.
		if (curpos < maxpos)
		{
			curpos++;
		}
		break;

	case SB_PAGELEFT:    // Scroll one page left.
		if (curpos > minpos)
		{
			curpos = max(minpos, curpos - (int)nPage);
		}
		break;

	case SB_PAGERIGHT:      // Scroll one page right.
		if (curpos < maxpos)
		{
			curpos = min(maxpos, curpos + (int)nPage);
		}
		break;

	case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
		curpos = nPos;      // of the scroll box at the end of the drag operation.
		break;

	case SB_THUMBTRACK:   // Drag scroll box to specified position. nPos is the
		curpos = nPos;     // position that the scroll box has been dragged to.
		break;
	}

	// Set the new position of the thumb (scroll box).
	SetScrollPos(SB_HORZ, curpos);

	m_scrollOffset.x = curpos;
	Invalidate();

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}


//////////////////////////////////////////////////////////////////////////
double CTrackViewDopeSheetBase::GetTickTime() const
{
	if ( GetTickDisplayMode() == eTVTickMode_InFrames )
		return m_fFrameTickStep;
	else
		return 1.0f / m_ticksStep;
}


//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::TickSnap(float time) const
{
	double tickTime = GetTickTime();
	double t = floor( ((double)time / tickTime) + 0.5);
	t *= tickTime;
	return t;
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::TimeFromPoint(CPoint point) const
{
	int x = point.x - m_leftOffset + m_scrollOffset.x;
	double t = (double)x / m_timeScale;
	return (float)TickSnap(t);
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::TimeFromPointUnsnapped(CPoint point) const
{
	int x = point.x - m_leftOffset + m_scrollOffset.x;
	double t = (double)x / m_timeScale;
	return t;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnLButtonDown(UINT nFlags, CPoint point)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	if (!pSequence)
	{
		return;
	}

	CWnd::OnLButtonDown(nFlags, point);

	HideKeyPropertyCtrlOnSpot();

	SetFocus();

	if (m_rcTimeline.PtInRect(point))
	{
		m_mouseDownPos = point;

		// Clicked inside timeline.
		m_mouseMode = eTVMouseMode_DragTime;
		// If mouse over selected key, change cursor to left-right arrows.
		SetMouseCursor(m_crsLeftRight);
		SetCapture();

		SetCurrTime(TimeFromPoint(point));
		return;
	}

	if (m_bEditLock)
	{
		m_mouseDownPos = point;
		return;
	}

	if (m_mouseMode == eTVMouseMode_Paste)
	{
		m_mouseMode = eTVMouseMode_None;

		CTrackViewAnimNode *pAnimNode = GetAnimNodeFromPoint(m_mouseOverPos);
		CTrackViewTrack *pTrack = GetTrackFromPoint(m_mouseOverPos);

		if (pAnimNode)
		{
			CUndo undo("Paste Keys");
			CUndo::Record(new CUndoAnimKeySelection(pSequence));
			pSequence->DeselectAllKeys();
			pSequence->PasteKeysFromClipboard(pAnimNode, pTrack, ComputeSnappedMoveOffset());
		}

		SetMouseCursor(NULL);
		ReleaseCapture();		
		return;
	}

	m_mouseDownPos = point;

	// The summary region is used for moving already selected keys.
	if (m_rcSummary.PtInRect(point))
	{
		CTrackViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();
		if (selectedKeys.GetKeyCount() > 0)
		{
			/// Move/Clone Key Undo Begin
			GetIEditor()->BeginUndo();
			pSequence->StoreUndoForTracksWithSelectedKeys();
			StoreMementoForTracksWithSelectedKeys();

			m_keyTimeOffset = 0;
			m_mouseMode = eTVMouseMode_Move;
			SetMouseCursor(m_crsLeftRight);
			SetCapture();
			return;
		}
	}

	bool bStart = false;
	CTrackViewKeyHandle &keyHandle = CheckCursorOnStartEndTimeAdjustBar(point, bStart);
	if (keyHandle.IsValid())
	{
		return LButtonDownOnTimeAdjustBar(point, keyHandle, bStart);
	}

	keyHandle = FirstKeyFromPoint(point);
	if (!keyHandle.IsValid())
	{
		keyHandle = DurationKeyFromPoint(point);
	}
	else
	{
		return LButtonDownOnKey(point, keyHandle, nFlags);
	}

	if (m_mouseActionMode == eTVActionMode_AddKeys)
	{
		AddKeys(point, nFlags & MK_SHIFT);
		return;
	}

	if (nFlags & MK_SHIFT)
	{
		m_mouseMode = eTVMouseMode_SelectWithinTime;
	}
	else
	{
		m_mouseMode = eTVMouseMode_Select;
	}
	SetCapture();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnLButtonUp(UINT nFlags, CPoint point)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	
	if (!pSequence)
	{		
		return;
	}

	if (m_mouseMode == eTVMouseMode_Select)
	{
		// Check if any key are selected.
		m_rcSelect -= m_scrollOffset;
		SelectKeys(m_rcSelect, (nFlags & MK_CONTROL));
		m_rcSelect = CRect(0, 0, 0, 0);
	}
	else if (m_mouseMode == eTVMouseMode_SelectWithinTime)
	{
		m_rcSelect -= m_scrollOffset;
		SelectAllKeysWithinTimeFrame(m_rcSelect, (nFlags & MK_CONTROL));
		m_rcSelect = CRect(0, 0, 0, 0);
	}
	else if (m_mouseMode == eTVMouseMode_DragTime)
	{
		SetMouseCursor(NULL);
	}
	else if (m_mouseMode == eTVMouseMode_Paste)
	{
		SetMouseCursor(NULL);
	}

	if (GetCapture() == this)
	{
		ReleaseCapture();
	}

	m_keyTimeOffset = 0;
	m_keyForTimeAdjust = CTrackViewKeyHandle();

	AcceptUndo();
	
	RedrawWindow();
	CWnd::OnLButtonUp(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	if (!pSequence || m_rcTimeline.PtInRect(point) || m_bEditLock)
	{
		return;
	}

	CTrackViewKeyHandle &keyHandle = FirstKeyFromPoint(point);
	
	if (!keyHandle.IsValid())
	{
		keyHandle = DurationKeyFromPoint(point);
	}
	else
	{
		GetIEditor()->BeginUndo();
		CUndoAnimKeySelection *pUndoKeySelection = new CUndoAnimKeySelection(pSequence);
		CUndo::Record(pUndoKeySelection);
		
		CTrackViewTrack *pTrack = GetTrackFromPoint(point);		
		if (pTrack)
		{
			CTrackViewSequenceNotificationContext context(pSequence);
			pSequence->DeselectAllKeys();
			keyHandle.Select(true);			
			
			m_keyTimeOffset = 0;			
			
			if (pUndoKeySelection->IsSelectionChanged())
			{
				GetIEditor()->AcceptUndo("Select Key");
			}
			else
			{
				GetIEditor()->CancelUndo();
			}

			CPoint p;
			::GetCursorPos(&p);

			bool bKeyChangeInSameTrack = m_pLastTrackSelectedOnSpot && pTrack == m_pLastTrackSelectedOnSpot;
			m_pLastTrackSelectedOnSpot = pTrack;

			ShowKeyPropertyCtrlOnSpot(p.x, p.y, pSequence->GetSelectedKeys().GetKeyCount() > 0, bKeyChangeInSameTrack);			
		}

		return;
	}

	const bool bTryAddKeysInGroup = (nFlags & MK_SHIFT);

	AddKeys(point, bTryAddKeysInGroup);

	m_mouseMode = eTVMouseMode_None;
	CWnd::OnLButtonDblClk(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnMButtonDown(UINT nFlags, CPoint point)
{
	OnRButtonDown(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnMButtonUp(UINT nFlags, CPoint point)
{
	OnRButtonUp(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnRButtonDown(UINT nFlags, CPoint point)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	if (!pSequence)
	{
		return;
	}

	HideKeyPropertyCtrlOnSpot();

	m_bCursorWasInKey = false;
	m_bMouseMovedAfterRButtonDown = false;

	SetFocus();

	if (m_rcTimeline.PtInRect(point))
	{
		// Clicked inside timeline.
		// adjust markers.
		int nMarkerStart = TimeToClient(m_timeMarked.start);
		int nMarkerEnd = TimeToClient(m_timeMarked.end);
		if ((abs(point.x - nMarkerStart)) < (abs(point.x - nMarkerEnd)))
		{
			SetStartMarker(TimeFromPoint(point));
			m_mouseMode = eTVMouseMode_DragStartMarker;
		}
		else
		{
			SetEndMarker(TimeFromPoint(point));
			m_mouseMode = eTVMouseMode_DragEndMarker;
		}
		SetCapture();
		return;
	}

	m_mouseDownPos = point;

	if (nFlags & MK_SHIFT)	// alternative zoom
	{
		m_bZoomDrag = true;
		SetCapture();
		return;
	}

	CTrackViewKeyHandle &keyHandle = FirstKeyFromPoint(point);
	if (!keyHandle.IsValid())
	{
		keyHandle = DurationKeyFromPoint(point);
	}

	if (keyHandle.IsValid())
	{
		m_bCursorWasInKey = true;

		CTrackViewNode *pNode = GetNodeFromPoint(point);		
		CTrackViewTrack *pTrack = static_cast<CTrackViewTrack*>(pNode);

		keyHandle.Select(true);
		m_keyTimeOffset = 0;
		Invalidate();

		// Show a little pop-up menu for copy & delete.
		enum { COPY_KEY = 100, DELETE_KEY, EDIT_KEY_ON_SPOT };
		CMenu menu;
		menu.CreatePopupMenu();
		bool bEnableEditOnSpot = false;
		CTrackViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();

		if (selectedKeys.GetKeyCount() > 0 && selectedKeys.AreAllKeysOfSameType())
		{
			bEnableEditOnSpot = true;
		}

		menu.AppendMenu(bEnableEditOnSpot ? MF_STRING : MF_STRING | MF_GRAYED,
		                EDIT_KEY_ON_SPOT, "Edit On Spot");
		menu.AppendMenu(MF_SEPARATOR, 0 , "");
		menu.AppendMenu(MF_STRING, COPY_KEY, "Copy");
		menu.AppendMenu(MF_SEPARATOR, 0 , "");
		menu.AppendMenu(MF_STRING, DELETE_KEY, "Delete");

		CPoint p;
		::GetCursorPos(&p);

		int cmd = menu.TrackPopupMenu(TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON, p.x, p.y, this, NULL);
		if (cmd == EDIT_KEY_ON_SPOT)
		{
			bool bKeyChangeInSameTrack
			  = m_pLastTrackSelectedOnSpot
			    && selectedKeys.GetKeyCount() == 1
			    && selectedKeys.GetKey(0).GetTrack() == m_pLastTrackSelectedOnSpot;

			if (selectedKeys.GetKeyCount() == 1)
			{
				m_pLastTrackSelectedOnSpot = selectedKeys.GetKey(0).GetTrack();
			}
			else
			{
				m_pLastTrackSelectedOnSpot = NULL;
			}

			ShowKeyPropertyCtrlOnSpot(p.x, p.y, selectedKeys.GetKeyCount() > 1, bKeyChangeInSameTrack);
		}
		else if (cmd == COPY_KEY)
		{
			pSequence->CopyKeysToClipboard(true, false);
		}
		else if (cmd == DELETE_KEY)
		{
			CUndo undo("Delete Keys");
			pSequence->DeleteSelectedKeys();
		}
	}
	else
	{
		m_bMoveDrag = true;
		SetCapture();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnRButtonUp(UINT nFlags, CPoint point)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	if (!pSequence)
	{
		return;
	}

	m_bZoomDrag = false;
	m_bMoveDrag = false;

	if (GetCapture() == this)
	{
		ReleaseCapture();
	}
	m_mouseMode = eTVMouseMode_None;

	if (!m_bCursorWasInKey)
	{
		const bool bHasCopiedKey = (GetKeysInClickboard() != NULL);

		if (bHasCopiedKey && m_bMouseMovedAfterRButtonDown == false)	// Once moved, it means the user wanted to scroll, so no paste pop-up.
		{
			// Show a little pop-up menu for paste.
			enum { PASTE_KEY = 100 };
			CMenu menu;
			menu.CreatePopupMenu();
			menu.AppendMenu(MF_STRING, PASTE_KEY, "Paste");

			CPoint p;
			::GetCursorPos(&p);
			int cmd = menu.TrackPopupMenu(TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON, p.x, p.y, this, NULL);
			if (cmd == PASTE_KEY)
			{
				StartPasteKeys();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnMouseMove(UINT nFlags, CPoint point)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	if (!pSequence)
	{
		return;
	}

	// To prevent the key moving while selecting
	if (m_bJustSelected)
	{
		m_bJustSelected = false;
		return;
	}

	m_bMouseMovedAfterRButtonDown = true;
	m_mouseOverPos = point;

	if (m_bZoomDrag && (nFlags & MK_SHIFT))
	{
		float fAnchorTime = TimeFromPointUnsnapped(m_mouseDownPos);
		SetTimeScale(m_timeScale * (1.0f + (point.x - m_mouseDownPos.x) * 0.0025f), fAnchorTime);
		m_mouseDownPos = point;
		return;
	}
	else
	{
		m_bZoomDrag = false;
	}

	if (m_bMoveDrag)
	{
		m_scrollOffset.x += m_mouseDownPos.x - point.x;
		if (m_scrollOffset.x < m_scrollMin)
		{
			m_scrollOffset.x = m_scrollMin;
		}
		if (m_scrollOffset.x > m_scrollMax)
		{
			m_scrollOffset.x = m_scrollMax;
		}
		m_mouseDownPos = point;
		// Set the new position of the thumb (scroll box).
		SetScrollPos(SB_HORZ, m_scrollOffset.x);
		Invalidate();
		SetMouseCursor(m_crsLeftRight);
		return;
	}

	if (m_mouseMode == eTVMouseMode_Select
	    || m_mouseMode == eTVMouseMode_SelectWithinTime)
	{
		MouseMoveSelect(point);
	}
	else if (m_mouseMode == eTVMouseMode_Move)
	{
		MouseMoveMove(point, nFlags);
	}
	else if (m_mouseMode == eTVMouseMode_Clone)
	{
		pSequence->CloneSelectedKeys();
		m_mouseMode = eTVMouseMode_Move;
	}
	else if (m_mouseMode == eTVMouseMode_DragTime)
	{
		MouseMoveDragTime(point, nFlags);
	}
	else if (m_mouseMode == eTVMouseMode_DragStartMarker)
	{
		MouseMoveDragStartMarker(point, nFlags);
	}
	else if (m_mouseMode == eTVMouseMode_DragEndMarker)
	{
		MouseMoveDragEndMarker(point, nFlags);
	}
	else if (m_mouseMode == eTVMouseMode_Paste)
	{
		Invalidate();
	}
	else if (m_mouseMode == eTVMouseMode_StartTimeAdjust)
	{
		MouseMoveStartEndTimeAdjust(point, true);
	}
	else if (m_mouseMode == eTVMouseMode_EndTimeAdjust)
	{
		MouseMoveStartEndTimeAdjust(point, false);
	}
	else
	{
		//////////////////////////////////////////////////////////////////////////
		if (m_mouseActionMode == eTVActionMode_AddKeys)
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
	CPaintDC paintDC(this);

	{
		CMemoryDC dc(paintDC, &m_offscreenBitmap);

		// In case of the fast-redraw mode, just draw the saved bitmap.
		// Otherwise, actually redraw all things.
		// This mode is helpful when playing a sequence if the sequence has a lot of keys.
		if (!m_bFastRedraw)
		{
			XTPPaintManager()->GradientFill(&dc, &paintDC.m_ps.rcPaint, RGB(250, 250, 250), RGB(220, 220, 220), FALSE);

			if (GetIEditor()->GetAnimation()->GetSequence())
			{
				if (m_bEditLock)
				{
					CBrush brushGray(EDIT_DISABLE_GRAY_COLOR);
					dc->FillRect(&paintDC.m_ps.rcPaint, &brushGray);
				}

				DrawControl(dc, paintDC.m_ps.rcPaint);
			}
		}
	}

	if (GetIEditor()->GetAnimation()->GetSequence())
	{
		// Drawing the timeline is handled separately. In other words, it's not saved to the 'm_offscreenBitmap'.
		// This is for the fast-redraw mode mentioned above.
		DrawTimeline(&paintDC, paintDC.m_ps.rcPaint);
	}

#ifdef DEBUG
	paintDC.SelectObject(m_descriptionFont);
	paintDC.SetTextColor(RGB(255, 255, 255));
	paintDC.SetBkColor(RGB(0, 0, 0));
	paintDC.SetBkMode(OPAQUE);

	CString redrawCountStr;
	redrawCountStr.Format("Redraw Count: %d", m_redrawCount);
	CRect redrawCountRect(0, 0, 150, 20);
	paintDC.DrawText(redrawCountStr, redrawCountRect, DT_LEFT | DT_SINGLELINE);

	++m_redrawCount;
#endif	
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SelectAllKeysWithinTimeFrame(const CRect &rc, const bool bMultiSelection)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	if (!pSequence)
	{
		return;
	}

	GetIEditor()->BeginUndo();
	CUndoAnimKeySelection *pUndoKeySelection = new CUndoAnimKeySelection(pSequence);
	CUndo::Record(pUndoKeySelection);

	if (!bMultiSelection)
	{
		pSequence->DeselectAllKeys();
	}

	// put selection rectangle from client to track space.
	CRect trackRect = rc;
	trackRect.OffsetRect(m_scrollOffset);

	Range selTime = GetTimeRange(trackRect);

	CTrackViewTrackBundle tracks = pSequence->GetAllTracks();

	CTrackViewSequenceNotificationContext context(pSequence);
	for (int i = 0; i < tracks.GetCount(); ++i)
	{
		CTrackViewTrack *pTrack = tracks.GetTrack(i);

		// Check which keys we intersect.
		for (int j = 0; j < pTrack->GetKeyCount(); j++)
		{
			CTrackViewKeyHandle &keyHandle = pTrack->GetKey(j);
			const float time = keyHandle.GetTime();
						
			if (selTime.IsInside(time))
			{
				keyHandle.Select(true);
			}
		}
	}

	if (pUndoKeySelection->IsSelectionChanged())
	{
		GetIEditor()->AcceptUndo("Select keys");
	}
	else
	{
		GetIEditor()->CancelUndo();
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

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetMouseCursor(HCURSOR crs)
{
	m_currCursor = crs;
	if (m_currCursor != NULL)
	{
		SetCursor(crs);
	}
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewDopeSheetBase::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetCurrTime(float time)
{
	if (time < m_timeRange.start)
	{
		time = m_timeRange.start;
	}
	if (time > m_timeRange.end)
	{
		time = m_timeRange.end;
	}

	GetIEditor()->GetAnimation()->SetTime(time);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnTimeChanged( float newTime )
{
	int x1 = TimeToClient(m_currentTime);
	int x2 = TimeToClient(newTime);		

	m_currentTime = newTime;

	m_bFastRedraw = true;
	CRect rc(x1 - 3, m_rcClient.top, x1 + 4, m_rcClient.bottom);
	RedrawWindow(rc, NULL, RDW_INVALIDATE);
	CRect rc1(x2 - 3, m_rcClient.top, x2 + 4, m_rcClient.bottom);
	RedrawWindow(rc1, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
	m_bFastRedraw = false;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetStartMarker(float fTime)
{
	m_timeMarked.start = fTime;

	if (m_timeMarked.start < m_timeRange.start)
	{
		m_timeMarked.start = m_timeRange.start;
	}
	if (m_timeMarked.start > m_timeRange.end)
	{
		m_timeMarked.start = m_timeRange.end;
	}
	if (m_timeMarked.start > m_timeMarked.end)
	{
		m_timeMarked.end = m_timeMarked.start;
	}

	GetIEditor()->GetAnimation()->SetMarkers(m_timeMarked);
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetEndMarker(float fTime)
{
	m_timeMarked.end = fTime;
	if (m_timeMarked.end < m_timeRange.start)
	{
		m_timeMarked.end = m_timeRange.start;
	}
	if (m_timeMarked.end > m_timeRange.end)
	{
		m_timeMarked.end = m_timeRange.end;
	}
	if (m_timeMarked.start > m_timeMarked.end)
	{
		m_timeMarked.start = m_timeMarked.end;
	}
	GetIEditor()->GetAnimation()->SetMarkers(m_timeMarked);
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetMouseActionMode(ETVActionMode mode)
{
	m_mouseActionMode = mode;
	if (mode == eTVActionMode_AddKeys)
	{
		SetMouseCursor(m_crsAddKey);
	}
}

//////////////////////////////////////////////////////////////////////////
CTrackViewNode *CTrackViewDopeSheetBase::GetNodeFromPointRec(CTrackViewNode *pCurrentNode, CPoint point)
{
	CRect currentNodeRect = GetNodeRect(pCurrentNode);

	if (currentNodeRect.top > point.y)
	{
		return nullptr;
	}

	if (currentNodeRect.bottom >= point.y)
	{
		return pCurrentNode;
	}

	if (pCurrentNode->IsExpanded())
	{
		unsigned int childCount = pCurrentNode->GetChildCount();
		for (unsigned int i = 0; i < childCount; ++i)
		{
			CTrackViewNode *pFoundNode = GetNodeFromPointRec(pCurrentNode->GetChild(i), point);
			if (pFoundNode)
			{
				return pFoundNode;
			}
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewNode *CTrackViewDopeSheetBase::GetNodeFromPoint(CPoint point)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	return GetNodeFromPointRec(pSequence, point);
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNode *CTrackViewDopeSheetBase::GetAnimNodeFromPoint(CPoint point)
{
	CTrackViewNode *pNode = GetNodeFromPoint(point);

	if (pNode)
	{
		if (pNode->GetNodeType() == eTVNT_Track)
		{
			CTrackViewTrack *pTrack = static_cast<CTrackViewTrack*>(pNode);
			return static_cast<CTrackViewAnimNode*>(pTrack->GetAnimNode());
		}
		else if (pNode->GetNodeType() == eTVNT_AnimNode)
		{
			return static_cast<CTrackViewAnimNode*>(pNode);
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrack * CTrackViewDopeSheetBase::GetTrackFromPoint(CPoint point)
{
	CTrackViewNode *pNode = GetNodeFromPoint(point);

	if (pNode && pNode->GetNodeType() == eTVNT_Track)
	{
		return static_cast<CTrackViewTrack*>(pNode);
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetHorizontalExtent(int min, int max)
{
	m_scrollMin = min;
	m_scrollMax = max;	
	int nPage = m_rcClient.Width() / 2;
	int sx = (max - min) - nPage + m_leftOffset;

	SCROLLINFO si;
	ZeroStruct(si);
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = m_scrollMin;
	si.nMax = m_scrollMax - nPage + m_leftOffset;
	si.nPage = m_rcClient.Width() / 2;
	si.nPos = m_scrollOffset.x;

	SetScrollInfo(SB_HORZ, &si, TRUE);
};

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CTrackViewDopeSheetBase::GetKeysInClickboard() const
{
	CClipboard clip;
	if (clip.IsEmpty())
	{
		return NULL;
	}

	if (clip.GetTitle() != "Track view keys")
	{
		return NULL;
	}

	XmlNodeRef copyNode = clip.Get();
	if (copyNode == NULL || strcmp(copyNode->getTag(), "CopyKeysNode"))
	{
		return NULL;
	}

	int nNumTracksToPaste = copyNode->getChildCount();
	if (nNumTracksToPaste == 0)
	{
		return NULL;
	}

	return copyNode;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::StartPasteKeys()
{
	m_clipboardKeys = GetKeysInClickboard();

	if (m_clipboardKeys)
	{
		m_mouseMode = eTVMouseMode_Paste;
		// If mouse over selected key, change cursor to left-right arrows.
		SetMouseCursor(m_crsLeftRight);
		SetCapture();
		m_mouseDownPos = m_mouseOverPos;
	}
}

void CTrackViewDopeSheetBase::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	if (!pSequence)
	{
		return;
	}

	if (nChar == VK_DELETE)
	{
		CUndo undo("Delete Keys");
		pSequence->DeleteSelectedKeys();
		return;
	}
	
	if (nChar == VK_UP || nChar == VK_DOWN || nChar == VK_RIGHT || nChar == VK_LEFT)
	{
		CTrackViewKeyBundle keyBundle = pSequence->GetSelectedKeys();
		CTrackViewKeyHandle keyHandle = keyBundle.GetSingleSelectedKey();

		if (keyHandle.IsValid())
		{
			switch (nChar)
			{
			case VK_UP:
				keyHandle = keyHandle.GetAboveKey();
				break;
			case VK_DOWN:
				keyHandle = keyHandle.GetBelowKey();				
				break;
			case VK_RIGHT:
				keyHandle = keyHandle.GetNextKey();				
				break;
			case VK_LEFT:
				keyHandle = keyHandle.GetPrevKey();				
				break;
			}

			if (keyHandle.IsValid())
			{
				GetIEditor()->BeginUndo();
				CUndoAnimKeySelection *pUndoKeySelection = new CUndoAnimKeySelection(pSequence);
				CUndo::Record(pUndoKeySelection);

				CTrackViewSequenceNotificationContext context(pSequence);
				pSequence->DeselectAllKeys();
				keyHandle.Select(true);				

				if (pUndoKeySelection->IsSelectionChanged())
				{
					GetIEditor()->AcceptUndo("Select Key");
				}
				else
				{
					GetIEditor()->CancelUndo();
				}
			}
		}

		return;
	}

	if (GetKeyState(VK_CONTROL))
	{
		switch (nChar)
		{
		case 'C':
			pSequence->CopyKeysToClipboard(true, false);
			return;
		case 'V':
			StartPasteKeys();
			return;
		case 'Z':
			GetIEditor()->Undo();
			return;
		case 'Y':
			GetIEditor()->Redo();
			return;
		}
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}


//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::RecordTrackUndo(CTrackViewTrack *pTrack)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	if (pTrack && pSequence)
	{
		CUndo undo("Track Modify");
		CUndo::Record(new CUndoTrackObject(pTrack, pSequence));
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::ShowKeyTooltip(CTrackViewKeyHandle &keyHandle, CPoint point)
{
	if (m_lastTooltipPos.x == point.x && m_lastTooltipPos.y == point.y)
	{
		return;
	}

	m_lastTooltipPos = point;
		
	const float time = keyHandle.GetTime();
	const char *desc = keyHandle.GetDescription();
	float duration = keyHandle.GetDuration();	

	CString tipText;
	if (GetTickDisplayMode() == eTVTickMode_InSeconds)
	{
		tipText.Format("%.3f, {%s}", time, desc);
	}
	else
	{
		tipText.Format("%d, {%s}", ftoi(time / m_snapFrameTime), desc);
	}

	m_tooltip.UpdateTipText(tipText, this, 1);
	m_tooltip.Activate(TRUE);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnRawInput(UINT wParam, HRAWINPUT lParam)
{
	// Forward raw input to the Track View dialog
	CWnd *pWnd = GetOwner();
	if (pWnd)
	{
		pWnd->SendMessage(WM_INPUT, wParam, (LPARAM)lParam);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnCaptureChanged(CWnd* pWnd)
{
	AcceptUndo();

	m_bZoomDrag = false;
	m_bMoveDrag = false;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewDopeSheetBase::IsOkToAddKeyHere(const CTrackViewTrack *pTrack, float time) const
{
	const float timeEpsilon = 0.05f;

	for (int i = 0; i < pTrack->GetKeyCount(); ++i)
	{
		CTrackViewKeyHandle &keyHandle = const_cast<CTrackViewTrack*>(pTrack)->GetKey(i);

		if (keyHandle.GetTime()==time)
		{
			return false;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::MouseMoveSelect(CPoint point)
{
	SetMouseCursor(NULL);
	CRect rc(m_mouseDownPos.x, m_mouseDownPos.y, point.x, point.y);
	rc.NormalizeRect();
	CRect rcClient;
	GetClientRect(rcClient);
	rc.IntersectRect(rc, rcClient);

	CDC *pDC = GetDC();

	if (pDC)
	{
		if (m_mouseMode == eTVMouseMode_SelectWithinTime)
		{
			rc.top = m_rcClient.top;
			rc.bottom = m_rcClient.bottom;
		}
		
		pDC->DrawDragRect(rc, CSize(1, 1), m_rcSelect, CSize(1, 1));
		ReleaseDC(pDC);
		m_rcSelect = rc;
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::MouseMoveStartEndTimeAdjust(CPoint point, bool bStart)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	if (!pSequence)
	{
		return;
	}

	SetMouseCursor(m_crsAdjustLR);
	point.x = std::max(point.x, m_rcClient.left);
	point.x = std::min(point.x, m_rcClient.right);

	CPoint ofs = point - m_mouseDownPos;

	CTrackViewKeyHandle &keyHandle = m_keyForTimeAdjust;
	
	ICharacterKey characterKey;	
	keyHandle.GetKey(&characterKey);
	
	float& timeToAdjust = bStart ? characterKey.m_startTime : characterKey.m_endTime;

	// Undo the last offset.
	timeToAdjust += -m_keyTimeOffset;

	// Apply a new offset.
	m_keyTimeOffset = (ofs.x / m_timeScale) * characterKey.m_speed;
	timeToAdjust += m_keyTimeOffset;

	// Check the validity.
	if (bStart)
	{
		if (timeToAdjust < 0)
		{
			timeToAdjust = 0;
		}
		else if (timeToAdjust > characterKey.GetValidEndTime())
		{
			timeToAdjust = characterKey.GetValidEndTime();
		}
	}
	else
	{
		if (timeToAdjust < characterKey.m_startTime)
		{
			timeToAdjust = characterKey.m_startTime;
		}
		else if (timeToAdjust > characterKey.GetValidEndTime())
		{
			timeToAdjust = characterKey.GetValidEndTime();
		}
	}

	CUndo::Record(new CUndoTrackObject(m_keyForTimeAdjust.GetTrack(), pSequence));
	keyHandle.SetKey(&characterKey);

	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::MouseMoveMove(CPoint point, UINT nFlags)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	CTrackViewSequenceNotificationContext context(pSequence);

	SetMouseCursor(m_crsLeftRight);
	point.x = std::max(point.x, m_rcClient.left);
	point.x = std::min(point.x, m_rcClient.right);

	// Reset tracks to their initial state before starting the move
	for (auto iter = m_trackMementos.begin(); iter != m_trackMementos.end(); ++iter)
	{
		CTrackViewTrack *pTrack = iter->first;
		
		const TrackMemento &trackMemento = iter->second;
		pTrack->RestoreFromMemento(trackMemento.m_memento);

		const unsigned int numKeys = trackMemento.m_keySelectionStates.size();
		for (unsigned int i = 0; i < numKeys; ++i)
		{
			pTrack->GetKey(i).Select(trackMemento.m_keySelectionStates[i]);
		}
	}

	CTrackViewKeyHandle &keyHandle = FirstKeyFromPoint(m_mouseDownPos);
	if (!keyHandle.IsValid())
	{
		keyHandle = DurationKeyFromPoint(m_mouseDownPos);
	}

	float oldTime;
	if (keyHandle.IsValid())
	{
		oldTime = keyHandle.GetTime();
	}
	else
	{
		oldTime = TimeFromPointUnsnapped(m_mouseDownPos);
	}

	CPoint ofs = point - m_mouseDownPos;
	float timeOffset = ofs.x / m_timeScale;
	float newTime = oldTime + timeOffset;

	// Snap it, if necessary.
	ESnappingMode snappingMode = GetKeyModifiedSnappingMode();
	if (snappingMode == eSnappingMode_SnapFrame)
		snappingMode = m_snappingMode;

	if (snappingMode == eSnappingMode_SnapMagnet)
		newTime = MagnetSnap(newTime, GetAnimNodeFromPoint(m_mouseOverPos));

	else if (snappingMode == eSnappingMode_SnapTick)
		newTime = TickSnap(newTime);

	else if (snappingMode == eSnappingMode_SnapFrame) 
		newTime = FrameSnap(newTime);

	Range extendedTimeRange(0.0f, m_timeRange.end);
	extendedTimeRange.ClipValue(newTime);

	timeOffset = newTime - oldTime;	// Re-compute the time offset using snapped & clipped 'newTime'.
	if (timeOffset == 0.0f)
	{		
		return;
	}

	m_bKeysMoved = true;

	if (m_mouseActionMode == eTVActionMode_ScaleKey)
	{
		float tscale = 0.005f;
		float tofs = ofs.x * tscale;
		tofs = pSequence->ClipTimeOffsetForScaling(1 + tofs) - 1;
		// Offset all selected keys by this offset.
		pSequence->ScaleSelectedKeys(1 + tofs);
		m_keyTimeOffset = tofs;
	}
	else
	{
		// Offset all selected keys by this offset.
		if (m_mouseActionMode == eTVActionMode_SlideKey)
		{
			timeOffset = pSequence->ClipTimeOffsetForSliding(timeOffset);
			pSequence->SlideKeys(timeOffset);
		}
		else
		{
			timeOffset = pSequence->ClipTimeOffsetForOffsetting(timeOffset);
			pSequence->OffsetSelectedKeys(timeOffset);
		}

		if ( CheckVirtualKey(VK_MENU) )
		{	
			CTrackViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();
			CTrackViewKeyHandle selectedKey = selectedKeys.GetSingleSelectedKey();

			if (selectedKey.IsValid())
			{					
				GetIEditor()->GetAnimation()->SetTime(selectedKey.GetTime());
			}
		}
		m_keyTimeOffset = timeOffset;
	}
}

void CTrackViewDopeSheetBase::MouseMoveDragTime(CPoint point, UINT nFlags)
{
	CPoint p = point;
	p.x = std::max(p.x, m_rcClient.left);
	p.x = std::min(p.x, m_rcClient.right);
	p.y = std::max(p.y, m_rcClient.top);
	p.y = std::min(p.y, m_rcClient.bottom);
	
	float time = TimeFromPointUnsnapped(p);
	m_timeRange.ClipValue(time);
	
	bool bSnap = (nFlags & MK_CONTROL) != 0;
	if (bSnap)
	{
		time = TickSnap(time);
	}
	SetCurrTime(time);
}

void CTrackViewDopeSheetBase::MouseMoveDragStartMarker(CPoint point, UINT nFlags)
{
	CPoint p = point;
	p.x = std::max(p.x, m_rcClient.left);
	p.x = std::min(p.x, m_rcClient.right);
	p.y = std::max(p.y, m_rcClient.top);
	p.y = std::min(p.y, m_rcClient.bottom);

	bool bNoSnap = (nFlags & MK_CONTROL) != 0;
	float time = TimeFromPointUnsnapped(p);
	m_timeRange.ClipValue(time);
	if (!bNoSnap)
	{
		time = TickSnap(time);
	}
	SetStartMarker(time);
}

void CTrackViewDopeSheetBase::MouseMoveDragEndMarker(CPoint point, UINT nFlags)
{
	CPoint p = point;
	p.x = std::max(p.x, m_rcClient.left);
	p.x = std::min(p.x, m_rcClient.right);
	p.y = std::max(p.y, m_rcClient.top);
	p.y = std::min(p.y, m_rcClient.bottom);

	bool bNoSnap = (nFlags & MK_CONTROL) != 0;
	float time = TimeFromPointUnsnapped(p);
	m_timeRange.ClipValue(time);
	if (!bNoSnap)
	{
		time = TickSnap(time);
	}
	SetEndMarker(time);
}

void CTrackViewDopeSheetBase::MouseMoveOver(CPoint point)
{
	// No mouse mode.
	SetMouseCursor(NULL);

	bool bStart = false;
	CTrackViewKeyHandle &keyHandle = CheckCursorOnStartEndTimeAdjustBar(point, bStart);
	if (keyHandle.IsValid())
	{
		SetMouseCursor(m_crsAdjustLR);
		return;
	}

	keyHandle = FirstKeyFromPoint(point);
	if (!keyHandle.IsValid())
	{
		keyHandle = DurationKeyFromPoint(point);
	}

	if (keyHandle.IsValid())
	{
		CTrackViewTrack *pTrack = GetTrackFromPoint(point);

		if (pTrack && keyHandle.IsSelected())
		{		
			// If mouse over selected key, change cursor to left-right arrows.
			SetMouseCursor(m_crsLeftRight);
		}
		else
		{
			SetMouseCursor(m_crsCross);
		}

		if (pTrack)
		{
			ShowKeyTooltip(keyHandle, point);
		}
	}
	else
	{
		if (m_tooltip.m_hWnd)
		{
			m_tooltip.Activate(FALSE);
		}
	}
}

float CTrackViewDopeSheetBase::MagnetSnap(float newTime, const CTrackViewAnimNode *pNode) const
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	if (!pSequence)
	{
		return newTime;
	}

	CTrackViewKeyBundle keys = pSequence->GetKeysInTimeRange(newTime - kMarginForMagnetSnapping / m_timeScale, 
		newTime + kMarginForMagnetSnapping / m_timeScale);

	if (keys.GetKeyCount() > 0)
	{
		// By default, just use the first key that belongs to the time range as a magnet.
		newTime = keys.GetKey(0).GetTime();
		// But if there is an in-range key in a sibling track, use it instead.
		// Here a 'sibling' means a track that belongs to a same node.
		for (int i = 0; i < keys.GetKeyCount(); ++i)
		{
			CTrackViewKeyHandle keyHandle = keys.GetKey(i);
			if (keyHandle.GetTrack()->GetAnimNode() == pNode)
			{
				newTime = keyHandle.GetTime();
				break;
			}
		}
	}

	return newTime;
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::FrameSnap(float time) const
{
	double t = floor((double)time / m_snapFrameTime + 0.5);
	t = t * m_snapFrameTime;
	return t;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::ShowKeyPropertyCtrlOnSpot(int x, int y, bool bMultipleKeysSelected, bool bKeyChangeInSameTrack)
{
	if (m_keyPropertiesDlg == NULL)
	{
		return;
	}

	if (m_wndPropsOnSpot.m_hWnd == 0)
	{
		CRect rc;
		GetClientRect(rc);
		m_wndPropsOnSpot.Create(WS_POPUP, rc, this);
		m_wndPropsOnSpot.ModifyStyleEx(0, WS_EX_PALETTEWINDOW | WS_EX_CLIENTEDGE);
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
	
	m_wndPropsOnSpot.SetDisplayOnlyModified(bMultipleKeysSelected);	
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
	SetScrollPos(SB_HORZ, hpos);
	m_scrollOffset.x = hpos;
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::LButtonDownOnTimeAdjustBar(CPoint point, CTrackViewKeyHandle &keyHandle, bool bStart)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();

	m_keyTimeOffset = 0;
	m_keyForTimeAdjust = keyHandle;	

	GetIEditor()->BeginUndo();

	if (bStart)
	{
		m_mouseMode = eTVMouseMode_StartTimeAdjust;
	}
	else
	{
		// In case of the end time, make it have a valid (not zero)
		// end time, first.
		ICharacterKey animKey;
		keyHandle.GetKey(&animKey);

		if (animKey.m_endTime == 0)
		{
			animKey.m_endTime = animKey.m_duration;
			CUndo::Record(new CUndoTrackObject(keyHandle.GetTrack(), pSequence));
			keyHandle.SetKey(&animKey);			
		}
		m_mouseMode = eTVMouseMode_EndTimeAdjust;
	}
	SetMouseCursor(m_crsAdjustLR);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::LButtonDownOnKey(CPoint point, CTrackViewKeyHandle &keyHandle, UINT nFlags)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();

	if (!keyHandle.IsSelected() && !(nFlags & MK_CONTROL))
	{
		CUndo undo("Select Keys");
		CUndoAnimKeySelection *pUndoKeySelection = new CUndoAnimKeySelection(pSequence);
		CUndo::Record(pUndoKeySelection);

		CTrackViewSequenceNotificationContext context(pSequence);
		pSequence->DeselectAllKeys();
		m_bJustSelected = true;
		m_keyTimeOffset = 0;
		keyHandle.Select(true);		

		if (!pUndoKeySelection->IsSelectionChanged())
			undo.Cancel();
	}
	else
	{
		GetIEditor()->CancelUndo();
	}

	/// Move/Clone Key Undo Begin
	GetIEditor()->BeginUndo();
	pSequence->StoreUndoForTracksWithSelectedKeys();
	StoreMementoForTracksWithSelectedKeys();

	if (nFlags & MK_SHIFT)
	{
		m_mouseMode = eTVMouseMode_Clone;
		SetMouseCursor(m_crsLeftRight);
	}
	else
	{
		m_mouseMode = eTVMouseMode_Move;
		SetMouseCursor(m_crsLeftRight);
	}

	Invalidate();
	SetCapture();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewDopeSheetBase::CreateColorKey(CTrackViewTrack *pTrack, float keyTime)
{
	bool keyCreated = false;
	Vec3 vColor(0, 0, 0);
	pTrack->GetValue(keyTime, vColor);

	COLORREF defaultColor = RGB(clamp_tpl(FloatToIntRet(vColor.x), 0, 255),
	                            clamp_tpl(FloatToIntRet(vColor.y), 0, 255),
	                            clamp_tpl(FloatToIntRet(vColor.z), 0, 255));
	CMFCColorDialog dlg(defaultColor, 0, this);
	if (dlg.DoModal() == IDOK)
	{
		COLORREF col = dlg.GetColor();
		ColorF colArray = ColorGammaToLinear(col) * 255.0f;

		RecordTrackUndo(pTrack);
		CTrackViewSequenceNotificationContext context(pTrack->GetSequence());

		const unsigned int numChildNodes = pTrack->GetChildCount();
		for (int i = 0; i < numChildNodes; ++i)
		{
			CTrackViewTrack *subTrack = static_cast<CTrackViewTrack*>(pTrack->GetChild(i));
			if (IsOkToAddKeyHere(subTrack, keyTime))
			{
				CTrackViewKeyHandle newKey = subTrack->CreateKey(keyTime);
				
				I2DBezierKey bezierKey;
				newKey.GetKey(&bezierKey);
				bezierKey.value = Vec2(keyTime, colArray[i]);
				newKey.SetKey(&bezierKey);

				keyCreated = true;
			}
		}
	}

	return keyCreated;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::AcceptUndo()
{
	if (CUndo::IsRecording())
	{
		CPoint mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(&mousePos);

		if (m_mouseMode == eTVMouseMode_Paste)
		{
			GetIEditor()->CancelUndo();
		}
		else if (m_mouseMode == eTVMouseMode_Move || m_mouseMode == eTVMouseMode_Clone)
		{
			CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();

			if (pSequence && m_bKeysMoved)
			{
				CUndo::Record(new CUndoAnimKeySelection(pSequence));
				GetIEditor()->AcceptUndo("Move/Clone Keys");
			}
			else
			{
				GetIEditor()->CancelUndo();
			}
		}
		else if (m_mouseMode == eTVMouseMode_StartTimeAdjust
		         || m_mouseMode == eTVMouseMode_EndTimeAdjust)
		{
			GetIEditor()->AcceptUndo("Adjust Start/End Time of an Animation Key");
		}
	}

	m_mouseMode = eTVMouseMode_None;
	m_trackMementos.clear();
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::ComputeSnappedMoveOffset()
{
	// Compute time offset
	CPoint currentMousePos = m_mouseOverPos;
	currentMousePos.x = std::max(currentMousePos.x, m_rcClient.left);
	currentMousePos.x = std::min(currentMousePos.x, m_rcClient.right);

	float time0 = TimeFromPointUnsnapped(m_mouseDownPos);
	float time = TimeFromPointUnsnapped(currentMousePos);

	if (GetKeyModifiedSnappingMode() == eSnappingMode_SnapTick)
	{
		time0 = TickSnap(time0);
		time = TickSnap(time);
	}

	return time - time0;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::AddKeys(CPoint point, const bool bTryAddKeysInGroup)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	if (!pSequence)
	{
		return;
	}

	// Add keys here.
	CTrackViewTrack *pTrack = GetTrackFromPoint(point);

	if (!pTrack)
	{
		return;
	}

	CTrackViewSequenceNotificationContext context(pSequence);

	CTrackViewAnimNode *pNode = pTrack->GetAnimNode();
	float keyTime = TimeFromPoint(point);
	bool inRange = m_timeRange.IsInside(keyTime);

	if (pTrack && inRange)
	{
		bool keyCreated = false;
		if (bTryAddKeysInGroup && pNode->GetParentNode())		// Add keys in group
		{
			CTrackViewTrackBundle tracksInGroup = pNode->GetTracksByParam(pTrack->GetParameterType());
			for (int i = 0; i < (int)tracksInGroup.GetCount(); ++i)
			{
				CTrackViewTrack *pCurrTrack = tracksInGroup.GetTrack(i);

				if (pCurrTrack->GetChildCount() == 0)	// A simple track
				{
					if (IsOkToAddKeyHere(pCurrTrack, keyTime))
					{
						RecordTrackUndo(pCurrTrack);
						pCurrTrack->CreateKey(keyTime);
						keyCreated = true;
					}
				}
				else																			// A compound track
				{
					for (int k = 0; k < pCurrTrack->GetChildCount(); ++k)
					{
						CTrackViewTrack *pSubTrack = static_cast<CTrackViewTrack*>(pCurrTrack->GetChild(k));
						if (IsOkToAddKeyHere(pSubTrack, keyTime))
						{
							RecordTrackUndo(pSubTrack);
							pSubTrack->CreateKey(keyTime);
							keyCreated = true;
						}
					}
				}
			}
		}
		else if (pTrack->GetChildCount() == 0)			// A simple track
		{
			if (IsOkToAddKeyHere(pTrack, keyTime))
			{
				RecordTrackUndo(pTrack);
				pTrack->CreateKey(keyTime);
				keyCreated = true;
			}
		}
		else																				// A compound track
		{
			if (pTrack->GetValueType() == eAnimValue_RGB)
			{
				keyCreated = CreateColorKey(pTrack, keyTime);
			}
			else
			{
				RecordTrackUndo(pTrack);
				for (int i = 0; i < pTrack->GetChildCount(); ++i)
				{
					CTrackViewTrack *pSubTrack = static_cast<CTrackViewTrack*>(pTrack->GetChild(i));
					if (IsOkToAddKeyHere(pSubTrack, keyTime))
					{
						pSubTrack->CreateKey(keyTime);
						keyCreated = true;
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawControl(CDC *pDC, const CRect &rcUpdate)
{
	CRect rc;
	CRect rcTemp;

	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	DrawNodesRecursive(pSequence, pDC, rcUpdate);

	DrawSummary(pDC, rcUpdate);

	DrawSelectedKeyIndicators(pDC);

	if (m_mouseMode == eTVMouseMode_Paste)
	{
		// If in paste mode draw keys that are in clipboard
		DrawClipboardKeys(pDC, rc);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawNodesRecursive(CTrackViewNode *pNode, CDC *pDC, const CRect &rcUpdate)
{
	CRect rect = GetNodeRect(pNode);

	if (!rect.IsRectEmpty())
	{
		switch (pNode->GetNodeType())
		{
		case eTVNT_AnimNode:
			DrawNodeTrack(static_cast<CTrackViewAnimNode*>(pNode), pDC, rect);
			break;
		case eTVNT_Track:		
			DrawTrack(static_cast<CTrackViewTrack*>(pNode), pDC, rect);
			break;
		}
	}

	if (pNode->IsExpanded())
	{
		unsigned int numChildren = pNode->GetChildCount();
		for (unsigned int i = 0; i < numChildren; ++i)
		{
			DrawNodesRecursive(pNode->GetChild(i), pDC, rcUpdate);
		}		
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawTicks(CDC *pDC, CRect &rc, Range &timeRange)
{
	// Draw time ticks every tick step seconds.
	CPen dkgray(PS_SOLID, 1, RGB(90, 90, 90));
	CPen ltgray(PS_SOLID, 1, RGB(120, 120, 120));

	CPen *prevPen = pDC->SelectObject(&dkgray);
	Range VisRange = GetVisibleRange();
	int nNumberTicks = 10;
	if (GetTickDisplayMode() == eTVTickMode_InFrames)
	{
		nNumberTicks = 8;
	}

	double start = TickSnap(timeRange.start);
	double step = 1.0 / m_ticksStep;

	for (double t = 0.0f; t <= timeRange.end + step; t += step)
	{
		double st = TickSnap(t);
		if (st > timeRange.end)
		{
			st = timeRange.end;
		}
		if (st < VisRange.start)
		{
			continue;
		}
		if (st > VisRange.end)
		{
			break;
		}
		int x = TimeToClient(st);
		if (x < 0)
		{
			continue;
		}
		pDC->MoveTo(x, rc.bottom - 1);

		int k = RoundFloatToInt(st * m_ticksStep);
		if (k % nNumberTicks == 0)
		{
			if (st >= start)
			{
				pDC->SelectObject(GetStockObject(BLACK_PEN));
			}
			else
			{
				pDC->SelectObject(dkgray);
			}

			pDC->LineTo(x, rc.bottom - 5);
			pDC->SelectObject(dkgray);
		}
		else
		{
			if (st >= start)
			{
				pDC->SelectObject(dkgray);
			}
			else
			{
				pDC->SelectObject(ltgray);
			}

			pDC->LineTo(x, rc.bottom - 3);
		}
	}
	pDC->SelectObject(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawTrack(CTrackViewTrack *pTrack, CDC *pDC, CRect trackRect)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	
	CPen pen(PS_SOLID, 1, RGB(120, 120, 120));
	CPen *prevPen = pDC->SelectObject(&pen);
	pDC->MoveTo(trackRect.left, trackRect.bottom);
	pDC->LineTo(trackRect.right, trackRect.bottom);
	pDC->SelectObject(prevPen);

	CRect rcInner = trackRect;
	rcInner.left = max(trackRect.left, m_leftOffset - m_scrollOffset.x);
	rcInner.right = min(trackRect.right, (m_scrollMax + m_scrollMin) - m_scrollOffset.x + m_leftOffset * 2);

	bool bLightAnimationSetActive = pSequence->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet;
	if (bLightAnimationSetActive && pTrack->GetKeyCount() > 0)
	{
		// In the case of the light animation set, the time of of the last key
		// determines the end of the track.
		float lastKeyTime = pTrack->GetKey(pTrack->GetKeyCount() - 1).GetTime();
		rcInner.right = min(rcInner.right, static_cast<LONG>(TimeToClient(lastKeyTime)));
	}

	CRect rcInnerDraw(rcInner.left - 6, rcInner.top, rcInner.right + 6, rcInner.bottom);
	COLORREF trackColor = CTVCustomizeTrackColorsDlg::GetTrackColor(pTrack->GetParameterType());
	if (pTrack->HasCustomColor())
	{
		ColorB customColor = pTrack->GetCustomColor();
		trackColor = RGB(customColor.r, customColor.g, customColor.b);
	}
	// For the case of tracks belonging to an inactive director node,
	// changes the track color to a custom one.
	COLORREF colorForDisabled = CTVCustomizeTrackColorsDlg::GetColorForDisabledTracks();
	COLORREF colorForMuted = CTVCustomizeTrackColorsDlg::GetColorForMutedTracks();

	CTrackViewAnimNode *pDirectorNode = pTrack->GetDirector();
	if (!pDirectorNode->IsActiveDirector())
	{
		trackColor = colorForDisabled;
	}

	// A disabled/muted track or any track in a disabled node also uses a custom color.
	CTrackViewAnimNode *pAnimNode = pTrack->GetAnimNode();
	bool bTrackDisabled = pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled;
	bool bTrackMuted = pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Muted;
	bool bTrackInvalid = !pTrack->IsSubTrack() && !pAnimNode->IsParamValid(pTrack->GetParameterType());
	bool bTrackInDisabledNode = pAnimNode->GetFlags() & eAnimNodeFlags_Disabled;
	if (bTrackDisabled || bTrackInDisabledNode || bTrackInvalid)
	{
		trackColor = colorForDisabled;
	}
	else if (bTrackMuted)
	{
		trackColor = colorForMuted;
	}
	CRect rc = rcInnerDraw;
	rc.DeflateRect(0, 1, 0, 0);


	const EAnimCurveType trackType = pTrack->GetCurveType();
	if (trackType == eAnimCurveType_TCBFloat || trackType == eAnimCurveType_TCBQuat || trackType == eAnimCurveType_TCBVector)
	{
		trackColor = RGB(245, 80, 70);
	}

	if (pTrack->IsSelected())
	{
		XTPPaintManager()->GradientFill(pDC, rc, trackColor,
		                                RGB(GetRValue(trackColor) / 2, GetGValue(trackColor) / 2, GetBValue(trackColor) / 2), FALSE);
	}
	else if (pTrack->GetValueType() == eAnimValue_RGB && pTrack->GetKeyCount() > 0)
	{
		DrawColorGradient(pDC, rc, pTrack);
	}
	else
	{
		pDC->FillSolidRect(rc.left, rc.top, rc.Width(), rc.Height(),
		                  trackColor);
	}

	// Left outside
	CRect rcOutside = trackRect;
	rcOutside.right = rcInnerDraw.left - 1;
	rcOutside.DeflateRect(1, 1, 1, 0);
	pDC->SelectObject(m_bkgrBrushEmpty);

	XTPPaintManager()->GradientFill(pDC, rcOutside, RGB(210, 210, 210), RGB(180, 180, 180), FALSE);

	// Right outside.
	rcOutside = trackRect;
	rcOutside.left = rcInnerDraw.right + 1;
	rcOutside.DeflateRect(1, 1, 1, 0);

	XTPPaintManager()->GradientFill(pDC, rcOutside, RGB(210, 210, 210), RGB(180, 180, 180), FALSE);

	// Get time range of update rectangle.
	Range timeRange = GetTimeRange(trackRect);

	// Draw tick marks in time range.
	DrawTicks(pDC, rcInner, timeRange);

	// Draw special track features
	EAnimValue trackValueType = pTrack->GetValueType();
	CAnimParamType trackParamType = pTrack->GetParameterType();

	if (trackValueType == eAnimValue_Bool)
	{
		// If this track is bool Track draw bars where track is true
		DrawBoolTrack(timeRange, pDC, pTrack, rc);
	}
	else if (trackValueType == eAnimValue_Select)
	{
		// If this track is Select Track draw bars to show where selection is active.
		DrawSelectTrack(timeRange, pDC, pTrack, rc);
	}
	else if (trackParamType == eAnimParamType_Sequence)
	{
		// If this track is Sequence Track draw bars to show where sequence is active.
		DrawSequenceTrack(timeRange, pDC, pTrack, rc);
	}
	else if (trackParamType == eAnimParamType_Goto)
	{
		// if this track is GoTo Track, draw an arrow to indicate jump position.
		DrawGoToTrackArrow(pTrack, pDC, rc);
	}

	// Draw keys in time range.
	DrawKeys(pTrack, pDC, rcInner, timeRange);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawSelectTrack(const Range &timeRange, CDC *pDC, CTrackViewTrack *pTrack, const CRect &rc)
{
	int x0 = TimeToClient(timeRange.start);
	float t0 = timeRange.start;

	CBrush *prevBrush = pDC->SelectObject(&m_selectTrackBrush);

	const int numKeys = pTrack->GetKeyCount();
	for (int i = 0; i < numKeys; ++i)
	{		
		CTrackViewKeyHandle &keyHandle = pTrack->GetKey(i);

		ISelectKey selectKey;
		keyHandle.GetKey(&selectKey);
		
		if (*selectKey.szSelection != 0)
		{
			float time = keyHandle.GetTime();
			float nextTime = timeRange.end;
			if (i < numKeys - 1)
			{
				nextTime = pTrack->GetKey(i + 1).GetTime();
			}
			
			time = clamp_tpl(time, timeRange.start, timeRange.end);
			nextTime = clamp_tpl(nextTime, timeRange.start, timeRange.end);

			int x0 = TimeToClient(time);

			float fBlendTime = selectKey.fBlendTime;
			int blendTimeEnd = 0;

			if (fBlendTime>0.0f && fBlendTime< (nextTime-time))
			{
				blendTimeEnd=TimeToClient(nextTime);
				nextTime-=fBlendTime;
			}

			int x = TimeToClient(nextTime);

			if (x != x0)
			{
				XTPPaintManager()->GradientFill(pDC, CRect(x0, rc.top + 1, x, rc.bottom), RGB(255, 255, 255), RGB(100, 190, 255), FALSE);
			}

			if (fBlendTime>0.0f)
			{
				XTPPaintManager()->GradientFill(pDC,CRect(x,rc.top+1,blendTimeEnd,rc.bottom),RGB(255,255,255),RGB(0,115,230),FALSE );
			}
		}
	}
	pDC->SelectObject(&prevBrush);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawBoolTrack(const Range &timeRange, CDC *pDC, CTrackViewTrack *pTrack, const CRect &rc)
{
	int x0 = TimeToClient(timeRange.start);
	float t0 = timeRange.start;
	CRect trackRect;

	CBrush *prevBrush = pDC->SelectObject(&m_visibilityBrush);

	const int numKeys = pTrack->GetKeyCount();
	for (int i = 0; i < numKeys; ++i)
	{
		CTrackViewKeyHandle &keyHandle = pTrack->GetKey(i);
		
		const float time = keyHandle.GetTime();
		if (time < timeRange.start)
		{
			continue;
		}
		if (time > timeRange.end)
		{
			break;
		}

		int x = TimeToClient(time);
		bool val = false;
		pTrack->GetValue(time - 0.001f, val);
		if (val)
		{
			XTPPaintManager()->GradientFill(pDC, CRect(x0, rc.top + 4, x, rc.bottom - 4), RGB(250, 250, 250), RGB(00, 80, 255), FALSE);
		}

		t0 = time;
		x0 = x;
	}
	int x = TimeToClient(timeRange.end);
	bool val = false;
	pTrack->GetValue(timeRange.end - 0.001f, val);
	if (val)
	{
		XTPPaintManager()->GradientFill(pDC, CRect(x0, rc.top + 4, x, rc.bottom - 4), RGB(250, 250, 250), RGB(00, 80, 255), FALSE);
	}
	pDC->SelectObject(&prevBrush);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawSequenceTrack(const Range &timeRange, CDC *pDC, CTrackViewTrack *pTrack, const CRect &rc)
{
	int x0 = TimeToClient(timeRange.start);
	float t0 = timeRange.start;
	CRect trackRect;

	CBrush *prevBrush = pDC->SelectObject(&m_selectTrackBrush);

	const int numKeys = pTrack->GetKeyCount();
	for (int i = 0; i < numKeys - 1; ++i)
	{
		CTrackViewKeyHandle &keyHandle = pTrack->GetKey(i);

		ISequenceKey sequenceKey;
		keyHandle.GetKey(&sequenceKey);
		if (*sequenceKey.szSelection != 0)
		{
			float time = keyHandle.GetTime();
			float nextTime = timeRange.end;
			if (i < numKeys - 1)
			{
				nextTime = pTrack->GetKey(i + 1).GetTime();
			}
			time = clamp_tpl(time, timeRange.start, timeRange.end);
			nextTime = clamp_tpl(nextTime, timeRange.start, timeRange.end);

			int x0 = TimeToClient(time);
			int x = TimeToClient(nextTime);

			if (x != x0)
			{
				COLORREF startColour = RGB(100, 190, 255);
				COLORREF endColour = RGB(250, 250, 250);
				XTPPaintManager()->GradientFill(pDC, CRect(x0, rc.top + 1, x, rc.bottom), startColour, endColour, FALSE);
			}
		}
	}
	pDC->SelectObject(&prevBrush);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawKeys(CTrackViewTrack *pTrack, CDC *pDC, CRect &rect, Range &timeRange)
{
	int numKeys = pTrack->GetKeyCount();

	CFont *prevFont = pDC->SelectObject(m_descriptionFont);

	pDC->SetTextColor(KEY_TEXT_COLOR);
	pDC->SetBkMode(TRANSPARENT);

	int prevKeyPixel = -10000;	
	const int kDefaultWidthForDescription = 200;
	const int kSmallMargin = 10;

	FixedDynArray<float> drawnKeyTimes;
	drawnKeyTimes.set( ArrayT( (float*)alloca( numKeys * sizeof(float)), numKeys) );

	// Draw keys.
	for (int i = 0; i < numKeys; ++i)
	{
		CTrackViewKeyHandle &keyHandle = pTrack->GetKey(i);

		const float time = keyHandle.GetTime();
		if(!stl::push_back_unique(drawnKeyTimes, time))
		{
			continue;
		}

		int x = TimeToClient(time);
		if (x - kSmallMargin > rect.right)
		{
			continue;
		}

		int x1 = x + kDefaultWidthForDescription;						
		CTrackViewKeyHandle nextKey = keyHandle.GetNextKey();
		
		if (nextKey.IsValid())
		{
			x1 = TimeToClient(nextKey.GetTime()) - kSmallMargin;
		}

		if (x1 > x + kSmallMargin)	// Enough space for description text or duration bar
		{
			// Get info about that key.
			const char *pDescription = keyHandle.GetDescription();
			const float duration = keyHandle.GetDuration();			

			int xlast = x;
			if (duration > 0)
			{
				xlast = TimeToClient(time + duration);
			}
			if (xlast + kSmallMargin < rect.left)
			{
				continue;
			}

			if (duration > 0)
			{
				DrawKeyDuration(pTrack, pDC, rect, i);
			}

			if (pDescription && pDescription[0] != 0)
			{
				char keydesc[1024];
				bool bSelectedAndBeingMoved = m_mouseMode == eTVMouseMode_Move && keyHandle.IsSelected();
				if (bSelectedAndBeingMoved)
				{
					// Show its time or frame number additionally.
					if (GetTickDisplayMode() == eTVTickMode_InSeconds)
						sprintf_s(keydesc, "%.3f, {", time);
					else
						sprintf_s(keydesc, "%d, {", ftoi(time / m_snapFrameTime));
				}
				else
					strcpy_s(keydesc, "{");
				strcat_s(keydesc, pDescription);
				strcat_s(keydesc, "}");
				// Draw key description text.
				// Find next key.
				CRect textRect(x + 10, rect.top, x1, rect.bottom);
				pDC->DrawText(keydesc, strlen(keydesc), textRect, DT_LEFT | DT_END_ELLIPSIS | DT_VCENTER | DT_SINGLELINE);
			}
		}

		if (x < 0)
		{
			continue;
		}

		if (pTrack->GetChildCount() == 0 // At compound tracks, keys are all green.
		    && abs(x - prevKeyPixel) < 2)
		{
			// If multiple keys on the same time.
			m_imageList.Draw(pDC, 2, CPoint(x - 6, rect.top + 2), ILD_TRANSPARENT);
		}
		else
		{
			if (keyHandle.IsSelected())
			{
				m_imageList.Draw(pDC, 1, CPoint(x - 6, rect.top + 2), ILD_TRANSPARENT);
			}
			else
			{
				m_imageList.Draw(pDC, 0, CPoint(x - 6, rect.top + 2), ILD_TRANSPARENT);
			}
		}

		prevKeyPixel = x;
	}
	pDC->SelectObject(prevFont);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawClipboardKeys(CDC *pDC, const CRect &rc)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();

	const float timeOffset = ComputeSnappedMoveOffset();

	// Get node & track under cursor
	CTrackViewAnimNode *pAnimNode = GetAnimNodeFromPoint(m_mouseOverPos);
	CTrackViewTrack *pTrack = GetTrackFromPoint(m_mouseOverPos);

	auto matchedLocations = pSequence->GetMatchedPasteLocations(m_clipboardKeys, pAnimNode, pTrack);

	for (size_t i = 0; i < matchedLocations.size(); ++i)
	{
		auto &matchedLocation = matchedLocations[i];
		CTrackViewTrack *pMatchedTrack = matchedLocation.first;
		XmlNodeRef trackNode = matchedLocation.second;

		if (pMatchedTrack->IsCompoundTrack())
		{
			// Both child counts should be the same, but make sure
			const unsigned int numSubTrack = std::min(pMatchedTrack->GetChildCount(), (unsigned int)trackNode->getChildCount());

			for (unsigned int subTrackIndex = 0; subTrackIndex < numSubTrack; ++subTrackIndex)
			{
				CTrackViewTrack *pSubTrack = static_cast<CTrackViewTrack*>(pMatchedTrack->GetChild(subTrackIndex));
				XmlNodeRef subTrackNode = trackNode->getChild(subTrackIndex);
				DrawTrackClipboardKeys(pDC, pSubTrack, subTrackNode, timeOffset);

				// Also draw to parent track. This is intentional
				DrawTrackClipboardKeys(pDC, pMatchedTrack, subTrackNode, timeOffset);
			}
		}
		else
		{
			DrawTrackClipboardKeys(pDC, pMatchedTrack, trackNode, timeOffset);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawTrackClipboardKeys(CDC *pDC, CTrackViewTrack *pTrack, XmlNodeRef trackNode, const float timeOffset)
{
	CPen selPen(PS_SOLID, 1, RGB(255, 255, 0));
	CPen *prevPen = pDC->SelectObject(&selPen);

	CRect trackRect = GetNodeRect(pTrack);
	const int numKeysToPaste = trackNode->getChildCount();

	for (int i = 0; i < numKeysToPaste; ++i)
	{
		XmlNodeRef keyNode = trackNode->getChild(i);

		float time;
		if(keyNode->getAttr("time", time))
		{
			int x = TimeToClient(time + timeOffset);
			m_imageList.Draw(pDC, 3, CPoint(x - 6, trackRect.top + 2), ILD_TRANSPARENT);
			pDC->MoveTo(x, m_rcClient.top);
			pDC->LineTo(x, m_rcClient.bottom);		
		}
	}

	pDC->SelectObject(prevPen);
}

//////////////////////////////////////////////////////////////////////////
CTrackViewKeyHandle CTrackViewDopeSheetBase::FirstKeyFromPoint(CPoint point)
{
	CTrackViewTrack *pTrack = GetTrackFromPoint(point);
	if (!pTrack)
	{
		return CTrackViewKeyHandle();
	}

	float t1 = TimeFromPointUnsnapped(CPoint(point.x - 4, point.y));
	float t2 = TimeFromPointUnsnapped(CPoint(point.x + 4, point.y));

	int numKeys = pTrack->GetKeyCount();
	for (int i = 0; i < numKeys; ++i)
	{
		CTrackViewKeyHandle &keyHandle = pTrack->GetKey(i);

		float time = keyHandle.GetTime();
		if (time >= t1 && time <= t2)
		{
			return keyHandle;
		}
	}

	return CTrackViewKeyHandle();
}

//////////////////////////////////////////////////////////////////////////
CTrackViewKeyHandle CTrackViewDopeSheetBase::DurationKeyFromPoint(CPoint point)
{
	CTrackViewTrack *pTrack = GetTrackFromPoint(point);
	if (!pTrack)
	{
		return CTrackViewKeyHandle();
	}

	float t = TimeFromPointUnsnapped(CPoint(point.x, point.y));

	int numKeys = pTrack->GetKeyCount();
	// Iterate in a reverse order to prioritize later nodes.
	for (int i = numKeys - 1; i >= 0; --i)
	{
		CTrackViewKeyHandle &keyHandle = pTrack->GetKey(i);

		const float time = keyHandle.GetTime();
		const float duration = keyHandle.GetDuration();
		
		if (t >= time && t <= time + duration)
		{
			return keyHandle;
		}
	}

	return CTrackViewKeyHandle();
}

//////////////////////////////////////////////////////////////////////////
CTrackViewKeyHandle CTrackViewDopeSheetBase::CheckCursorOnStartEndTimeAdjustBar(CPoint point, bool& bStart)
{
	CTrackViewTrack *pTrack = GetTrackFromPoint(point);

	if (!pTrack || (pTrack->GetParameterType() != eAnimParamType_Animation && 
		pTrack->GetParameterType() != eAnimParamType_TimeRanges && pTrack->GetValueType() != eAnimValue_CharacterAnim))
	{
		return CTrackViewKeyHandle();
	}

	int numKeys = pTrack->GetKeyCount();
	for (int i = 0; i < numKeys; ++i)
	{
		CTrackViewKeyHandle &keyHandle = pTrack->GetKey(i);

		if (!keyHandle.IsSelected())
		{
			continue;
		}

		const float time = keyHandle.GetTime();
		const float duration = keyHandle.GetDuration();
		
		if (duration == 0)
		{
			continue;
		}

		int stime = TimeToClient(time);
		int etime = TimeToClient(time + duration);
		if (point.x >= stime - 3 && point.x <= stime)
		{
			bStart = true;
			return keyHandle;
		}
		else if (point.x >= etime && point.x <= etime + 3)
		{
			bStart = false;
			return keyHandle;
		}
	}

	return CTrackViewKeyHandle();
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewDopeSheetBase::NumKeysFromPoint(CPoint point)
{
	CTrackViewTrack *pTrack = GetTrackFromPoint(point);
	if (!pTrack)
	{
		return -1;
	}

	float t1 = TimeFromPointUnsnapped(CPoint(point.x - 4, point.y));
	float t2 = TimeFromPointUnsnapped(CPoint(point.x + 4, point.y));

	int count = 0;
	int numKeys = pTrack->GetKeyCount();
	for (int i = 0; i < numKeys; ++i)
	{
		CTrackViewKeyHandle &keyHandle = pTrack->GetKey(i);

		const float time = keyHandle.GetTime();
		if (time >= t1 && time <= t2)
		{
			++count;
		}
	}
	return count;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SelectKeys(const CRect &rc, const bool bMultiSelection)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();

	GetIEditor()->BeginUndo();
	CUndoAnimKeySelection *pUndoKeySelection = new CUndoAnimKeySelection(pSequence);
	CUndo::Record(pUndoKeySelection);

	CTrackViewSequenceNotificationContext context(pSequence);
	if (!bMultiSelection)
	{	
		pSequence->DeselectAllKeys();
	}

	// put selection rectangle from client to track space.
	CRect rci = rc;
	rci.OffsetRect(m_scrollOffset);

	Range selTime = GetTimeRange(rci);

	CTrackViewTrackBundle tracks = pSequence->GetAllTracks();

	CRect trackRect;
	for (int i = 0; i < tracks.GetCount(); ++i)
	{
		CTrackViewTrack *pTrack = tracks.GetTrack(i);

		CRect trackRect = GetNodeRect(pTrack);
		// Decrease item rectangle a bit.
		trackRect.DeflateRect(4, 4, 4, 4);
		// Check if item rectangle intersects with selection rectangle in y axis.
		if ((trackRect.top >= rc.top && trackRect.top <= rc.bottom) ||
		    (trackRect.bottom >= rc.top && trackRect.bottom <= rc.bottom) ||
		    (rc.top >= trackRect.top && rc.top <= trackRect.bottom) ||
		    (rc.bottom >= trackRect.top && rc.bottom <= trackRect.bottom))
		{
			// Check which keys we intersect.
			for (int j = 0; j < pTrack->GetKeyCount(); j++)
			{
				CTrackViewKeyHandle &keyHandle = pTrack->GetKey(j);

				const float time = keyHandle.GetTime();
				if (selTime.IsInside(time))
				{
					keyHandle.Select(true);
				}
			}
		}
	}

	if (pUndoKeySelection->IsSelectionChanged())
	{
		GetIEditor()->AcceptUndo("Select keys");
	}
	else
	{
		GetIEditor()->CancelUndo();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetTickDisplayMode(ETVTickMode mode)
{
	m_tickDisplayMode = mode;
	SetTimeScale(GetTimeScale(), 0); // for refresh
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetSnapFPS(UINT fps)
{
	m_snapFrameTime = (fps == 0) ? 0.033333f : (1.0f / float(fps));
}

//////////////////////////////////////////////////////////////////////////
ESnappingMode CTrackViewDopeSheetBase::GetKeyModifiedSnappingMode()
{
	ESnappingMode snappingMode = m_snappingMode;

	if (CheckVirtualKey(VK_CONTROL))
	{
		snappingMode = eSnappingMode_SnapNone;
	}
	else if (CheckVirtualKey(VK_SHIFT))
	{
		snappingMode = eSnappingMode_SnapMagnet;
	}
	else if (CheckVirtualKey(VK_MENU))
	{
		snappingMode = eSnappingMode_SnapFrame;
	}

	return snappingMode;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawSelectedKeyIndicators(CDC *pDC)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	
	CPen selPen(PS_SOLID, 1, RGB(255, 255, 0));
	CPen *prevPen = pDC->SelectObject(&selPen);
	
	CTrackViewKeyBundle keys = pSequence->GetSelectedKeys();
	for (int i = 0; i < keys.GetKeyCount(); ++i)
	{
		CTrackViewKeyHandle &keyHandle = keys.GetKey(i);		
		int x = TimeToClient(keyHandle.GetTime());
		pDC->MoveTo(x, m_rcClient.top);
		pDC->LineTo(x, m_rcClient.bottom);
	}

	pDC->SelectObject(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::ComputeFrameSteps( const Range &visRange ) 
{
	float fNbFrames = fabsf ( (  visRange.end - visRange.start ) / m_snapFrameTime ) ;
	float afStepTable [4] = { 1.0f, 0.5f, 0.2f, 0.1f };
	bool bDone = false;
	float fFact = 1.0f;
	unsigned int nStepIdx = 0;
	for (unsigned int nAttempts = 0; nAttempts < 10 && !bDone; ++nAttempts)
	{
		bool bLess = true;
		for ( nStepIdx = 0; nStepIdx < 4; ++nStepIdx ) 
		{
			float fFactNbFrames = fNbFrames / ( afStepTable[nStepIdx] * fFact );
			if ( fFactNbFrames >= 3 && fFactNbFrames <= 9 ) 
			{
				bDone = true; 
				break;
			}
			else
			{
				bLess = ( fFactNbFrames < 3 );
			}
		}
		if (!bDone) 
		{
			fFact *= (bLess)? 0.1f : 10.0f;
		}
	}

	float nBIntermediateTicks = 5;
	m_fFrameLabelStep = fFact * afStepTable[nStepIdx];

	if ( TimeToClient(m_fFrameLabelStep) - TimeToClient(0) > 1300 )
		nBIntermediateTicks = 10;

	m_fFrameTickStep = m_fFrameLabelStep * double (m_snapFrameTime) / double(nBIntermediateTicks); 
}


void CTrackViewDopeSheetBase::DrawTimeLineInFrames( CDC *dc, CRect &rc, COLORREF &lineCol, COLORREF &textCol, double step )
{
	float fFramesPerSec = 1.0f / m_snapFrameTime;
	float fInvFrameLabelStep = 1.0f / m_fFrameLabelStep;
	Range VisRange = GetVisibleRange();

	const Range & timeRange = m_timeRange;

	CPen ltgray(PS_SOLID,1,RGB(90,90,90));
	CPen black(PS_SOLID,1,textCol);

	for (double t = TickSnap(timeRange.start); t <= timeRange.end + m_fFrameTickStep; t += m_fFrameTickStep)
	{
		double st = t;
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

		float fFrame = st * fFramesPerSec; 
		float fFrameScaled = fFrame * fInvFrameLabelStep;
		if ( fabsf (fFrameScaled - RoundFloatToInt(fFrameScaled)) < 0.001f ) 
		{
			dc->SelectObject(black);
			dc->LineTo(x,rc.bottom-14);
			char str[32];
			sprintf( str,"%g", fFrame);
			dc->TextOut( x+2,rc.top,str );
			dc->SelectObject(ltgray);
		}
		else
			dc->LineTo(x,rc.bottom-6);
	}
}


void CTrackViewDopeSheetBase::DrawTimeLineInSeconds( CDC *dc, CRect &rc, COLORREF &lineCol, COLORREF &textCol, double step )
{
	Range VisRange = GetVisibleRange();
	const Range & timeRange = m_timeRange;
	int nNumberTicks=10;

	CPen ltgray(PS_SOLID,1,RGB(90,90,90));
	CPen black(PS_SOLID,1,textCol);

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
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawTimeline(CDC *pDC, const CRect &rcUpdate)
{
	CRect rc, temprc;

	bool recording = GetIEditor()->GetAnimation()->IsRecording();

	COLORREF lineCol = RGB(255,0,255);
	COLORREF textCol = RGB(0,0,0);
	COLORREF dkgrayCol = RGB(90, 90, 90);
	COLORREF ltgrayCol = RGB(150, 150, 150);

	if (recording)
	{
		lineCol = RGB(255, 0, 0);
	}

	// Draw vertical line showing current time.
	{
		int x = TimeToClient(m_currentTime);
		if (x > m_rcClient.left && x < m_rcClient.right)
		{
			CPen pen(PS_SOLID, 1, lineCol);
			CPen *prevPen = pDC->SelectObject(&pen);
			pDC->MoveTo(x, 0);
			pDC->LineTo(x, m_rcClient.bottom);
			pDC->SelectObject(prevPen);
		}
	}

	rc = m_rcTimeline;
	if (temprc.IntersectRect(rc, rcUpdate) == 0)
		return;

	XTPPaintManager()->GradientFill(pDC, rc, RGB(250, 250, 250), RGB(180, 180, 180), FALSE);

	CPen *prevPen;
	CPen dkgray(PS_SOLID, 1, dkgrayCol);
	CPen ltgray(PS_SOLID, 1, ltgrayCol);
	CPen black(PS_SOLID, 1, textCol);
	CPen redpen(PS_SOLID, 1, lineCol);
	// Draw time ticks every tick step seconds.
	const Range &timeRange = m_timeRange;
	CString str;

	pDC->SetTextColor(textCol);
	pDC->SetBkMode(TRANSPARENT);
	pDC->SelectObject(gSettings.gui.hSystemFont);
	pDC->SelectObject(dkgray);

	double step = 1.0 / double(m_ticksStep);
	if (GetTickDisplayMode() == eTVTickMode_InFrames)
		DrawTimeLineInFrames(pDC, rc, lineCol, textCol, step);
	else if (GetTickDisplayMode() == eTVTickMode_InSeconds)
		DrawTimeLineInSeconds(pDC, rc, lineCol, textCol, step);
	else 
		assert (0);

	// Draw time markers.
	int x;

	x = TimeToClient(m_timeMarked.start);
	m_imgMarker.Draw(pDC, 1, CPoint(x, m_rcTimeline.bottom - 9), ILD_TRANSPARENT);
	x = TimeToClient(m_timeMarked.end);
	m_imgMarker.Draw(pDC, 0, CPoint(x - 7, m_rcTimeline.bottom - 9), ILD_TRANSPARENT);

	prevPen = pDC->SelectObject(&redpen);
	x = TimeToClient(m_currentTime);
	pDC->SelectObject(GetStockObject(NULL_BRUSH));
	pDC->Rectangle(x - 3, rc.top, x + 4, rc.bottom);

	pDC->SelectObject(redpen);
	pDC->MoveTo(x, rc.top);
	pDC->LineTo(x, rc.bottom);
	pDC->SelectObject(GetStockObject(NULL_BRUSH));

	pDC->SelectObject(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawSummary(CDC *pDC, CRect rcUpdate)
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	
	CRect rc, temprc;
	COLORREF lineCol = RGB(0, 0, 0);
	COLORREF fillCol = RGB(150, 100, 220);

	rc = m_rcSummary;
	if (temprc.IntersectRect(rc, rcUpdate) == 0)
	{
		return;
	}

	pDC->FillSolidRect(rc.left, rc.top, rc.Width(), rc.Height(), fillCol);

	CPen *prevPen;
	CPen blackPen(PS_SOLID, 3, lineCol);
	Range timeRange = m_timeRange;

	prevPen = pDC->SelectObject(&blackPen);

	// Draw a short thick line at each place where there is a key in any tracks.
	CTrackViewKeyBundle keys = pSequence->GetAllKeys();
	for (int i = 0; i < keys.GetKeyCount(); ++i)
	{
		CTrackViewKeyHandle &keyHandle = keys.GetKey(i);
		int x = TimeToClient(keyHandle.GetTime());
		pDC->MoveTo(x, rc.bottom - 2);
		pDC->LineTo(x, rc.top + 2);
	}

	pDC->SelectObject(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawNodeTrack(CTrackViewAnimNode *pAnimNode, CDC *pDC, CRect trackRect)
{
	CFont *prevFont = pDC->SelectObject(m_descriptionFont);

	CTrackViewAnimNode *pDirectorNode = pAnimNode->GetDirector();

	if (pDirectorNode->GetNodeType() != eTVNT_Sequence && !pDirectorNode->IsActiveDirector())
	{
		pDC->SetTextColor(INACTIVE_TEXT_COLOR);
	}
	else
	{
		pDC->SetTextColor(KEY_TEXT_COLOR);
	}

	pDC->SetBkMode(TRANSPARENT);

	CRect textRect = trackRect;
	textRect.left += 4;
	textRect.right -= 4;

	CString sAnimNodeName = pAnimNode->GetName();
	const bool hasObsoleteTrack = pAnimNode->HasObsoleteTrack();	

	if (hasObsoleteTrack)
	{
		pDC->SetTextColor(RGB(245, 80, 70));
		sAnimNodeName += ": Some of the sub-tracks contains obsoleted TCB splines (marked in red), thus cannot be copied or pasted.";
	}

	pDC->DrawText(sAnimNodeName, sAnimNodeName.GetLength(), textRect, DT_LEFT | DT_END_ELLIPSIS | DT_VCENTER | DT_SINGLELINE);

	pDC->SelectObject(prevFont);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawGoToTrackArrow(CTrackViewTrack *pTrack, CDC *pDC, CRect &rc)
{
	int numKeys = pTrack->GetKeyCount();
	const COLORREF colorLine = RGB(150, 150, 150);
	const COLORREF colorHeader = RGB(50, 50, 50);
	const int tickness = 2;
	const int halfMargin = (rc.Height() - tickness) / 2;

	for (int i = 0; i < numKeys; ++i)
	{
		CTrackViewKeyHandle &keyHandle = pTrack->GetKey(i);

		IDiscreteFloatKey discreteFloatKey;
		keyHandle.GetKey(&discreteFloatKey);

		int arrowStart = TimeToClient(discreteFloatKey.time);
		int arrowEnd = TimeToClient(discreteFloatKey.m_fValue);

		if (discreteFloatKey.m_fValue < 0.f)
		{
			continue;
		}

		// draw arrow body line
		if (arrowStart < arrowEnd)
		{
			XTPPaintManager()->GradientFill
			(pDC, CRect(arrowStart, rc.top + halfMargin, arrowEnd, rc.bottom - halfMargin), colorLine, colorLine, FALSE);
		}
		else if (arrowStart > arrowEnd)
		{
			XTPPaintManager()->GradientFill
			(pDC, CRect(arrowEnd, rc.top + halfMargin, arrowStart, rc.bottom - halfMargin), colorLine, colorLine, FALSE);
		}

		// draw arrow head
		if (arrowStart != arrowEnd)
		{
			XTPPaintManager()->GradientFill
			(pDC, CRect(arrowEnd, rc.top + 2, arrowEnd + 1, rc.bottom - 2), colorHeader, colorHeader, FALSE);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawKeyDuration(CTrackViewTrack *pTrack, CDC *pDC, CRect &rc, int keyIndex)
{
	CTrackViewKeyHandle &keyHandle = pTrack->GetKey(keyIndex);

	const float time = keyHandle.GetTime();	
	const float duration = keyHandle.GetDuration();

	int x = TimeToClient(time);

	// Draw key duration.
	float endt = min(time + duration, m_timeRange.end);
	int x1 = TimeToClient(endt);
	if (x1 < 0)
	{
		if (x > 0)
		{
			x1 = rc.right;
		}
	}
	CBrush *prevBrush = pDC->SelectObject(&m_visibilityBrush);
	COLORREF colorFrom = RGB(120, 120, 255);
	if (pTrack->GetParameterType() == eAnimParamType_Sound)    // If it is a sound key
	{
		ISoundKey soundKey;
		keyHandle.GetKey(&soundKey);
		colorFrom = RGB(uint8(soundKey.customColor.x * 255.0f), uint8(soundKey.customColor.y * 255.0f), uint8(soundKey.customColor.z * 255.0f));
	}
	XTPPaintManager()->GradientFill(pDC, CRect(x, rc.top + 3, x1 + 1, rc.bottom - 3), colorFrom, RGB(250, 250, 250), FALSE);

	pDC->SelectObject(&prevBrush);
	pDC->MoveTo(x1, rc.top);
	pDC->LineTo(x1, rc.bottom);

	// If it is a selected animation track, draw the whole animation box (in green)
	// and two adjust bars (in red) for start/end time each, too.
	if (keyHandle.IsSelected() && (pTrack->GetParameterType() == eAnimParamType_Animation 
		|| pTrack->GetParameterType() == eAnimParamType_TimeRanges || pTrack->GetValueType() == eAnimValue_CharacterAnim))
	{
		// Draw the whole animation box.
		
		// This will work for both character & time range keys because 
		// ICharacterKey is derived from ITimeRangeKey. Not the most beautiful code.
		ICharacterKey characterKey;
		keyHandle.GetKey(&characterKey);
		
		// Make sure to only use time range elements of struct
		ITimeRangeKey &timeRangeKey = characterKey;

		int startX = TimeToClient(time - timeRangeKey.m_startTime / timeRangeKey.m_speed);
		int endX = TimeToClient(time + (timeRangeKey.m_duration - timeRangeKey.m_startTime) / timeRangeKey.m_speed);
		CPen greenPen(PS_SOLID, 1, RGB(0, 255, 0));
		CPen *prevPen = pDC->SelectObject(&greenPen);
		pDC->MoveTo(startX, rc.top);
		pDC->LineTo(endX, rc.top);
		pDC->LineTo(endX, rc.bottom);
		pDC->LineTo(startX, rc.bottom);
		pDC->LineTo(startX, rc.top);
		pDC->SelectObject(prevPen);

		// Draw two adjust bars.
		int durationX = TimeToClient(time + duration);
		CPen redPen(PS_SOLID, 3, RGB(255, 0, 0));
		prevPen = pDC->SelectObject(&redPen);
		pDC->MoveTo(x - 2, rc.top);
		pDC->LineTo(x - 2, rc.bottom);
		pDC->MoveTo(durationX + 2, rc.top);
		pDC->LineTo(durationX + 2, rc.bottom);
		pDC->SelectObject(prevPen);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawColorGradient(CDC *pDC, const CRect &rc, const CTrackViewTrack *pTrack)
{
	CPen dummy;
	dummy.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	CPen *pOldPen = pDC->SelectObject(&dummy);
	for (int x = rc.left; x < rc.right; ++x)
	{
		// This is really slow. Is there a better way?
		Vec3 vColor(0, 0, 0);
		pTrack->GetValue(TimeFromPointUnsnapped(CPoint(x, rc.top)), vColor);
		
		CPen pen;
		pen.CreatePen(PS_SOLID, 1, ColorLinearToGamma(vColor / 255.0f));

		pDC->SelectObject(&pen);
		pDC->MoveTo(CPoint(x, rc.top));
		pDC->LineTo(CPoint(x, rc.bottom));
	}
	pDC->SelectObject(pOldPen);
}

//////////////////////////////////////////////////////////////////////////
CRect CTrackViewDopeSheetBase::GetNodeRect(const CTrackViewNode *pNode) const
{
	CTrackViewNodesCtrl::CRecord *pRecord = m_pNodesCtrl->GetNodeRecord(pNode);
	
	if (pRecord && pRecord->IsVisible())
	{
		CRect rect = pRecord->GetRect();		
		rect.left = 0;
		rect.right = m_rcClient.right;
		return rect;
	}

	return CRect();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::StoreMementoForTracksWithSelectedKeys()
{
	CTrackViewSequence *pSequence = GetIEditor()->GetAnimation()->GetSequence();
	CTrackViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();

	m_trackMementos.clear();

	// Construct the set of tracks that have selected keys
	std::set<CTrackViewTrack*> tracks;
	
	const unsigned int numKeys = selectedKeys.GetKeyCount();
	for (int keyIndex = 0; keyIndex < numKeys; ++keyIndex)
	{
		CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);
		tracks.insert(keyHandle.GetTrack());
	}

	// For each of those tracks store an undo object
	for (auto iter = tracks.begin(); iter != tracks.end(); ++iter)
	{
		CTrackViewTrack *pTrack = *iter;

		TrackMemento trackMemento;
		trackMemento.m_memento = pTrack->GetMemento();
		
		const unsigned int numKeys = pTrack->GetKeyCount();		
		for (unsigned int i = 0; i < numKeys; ++i)
		{
			trackMemento.m_keySelectionStates.push_back(pTrack->GetKey(i).IsSelected());
		}

		m_trackMementos[pTrack] = trackMemento;
	}
}
