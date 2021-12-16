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

#ifndef __TELEMETRYEVENTSVIEW_H__
#define __TELEMETRYEVENTSVIEW_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "Controls\PropertyCtrl.h"

class CTelemetryViewTimelinesCtrl;

namespace Telemetry { class CTimelineRecord; class CTelemetryTimeline; struct STelemetryEvent; }

enum ETeVTickMode
{
	TeVTICK_INSECONDS = 0,
	TeVTICK_INFRAMES,
};
	
/** Class for Telemetry event view dialog.
*/
class CTelemetryEventsView : public CWnd, public IEditorNotifyListener
{
	DECLARE_DYNAMIC(CTelemetryEventsView)
public:

	CTelemetryEventsView();
	virtual ~CTelemetryEventsView();

	void SetTimeScale( float timeScale,float fAnchorTime );
	float GetTimeScale() { return m_timeScale; }

	void SetScrollOffset(int hpos);
	void SetVerticalScroll(int vertPos) { m_scrollOffset.y = vertPos; }

	void SetTimeRange( float start,float end );
	void SetCurrTime( float time );
	float GetCurrTime() const;
	void SetStartMarker(float fTime);
	void SetEndMarker(float fTime);

	void ShowKeyPropertyCtrlOnSpot(int x, int y, bool bMultipleKeysSelected, bool bKeyChangeInSameTrack);
	void HideKeyPropertyCtrlOnSpot();


	int GetHorizontalExtent() const { return m_itemWidth; };	
	
	virtual void	SetHorizontalExtent( int min,int max );
	virtual int		ItemFromPoint( CPoint pnt ) const;
	virtual int		GetItemRect( int item,CRect &rect ) const;
	virtual void  InvalidateItem( int item );

	void ClearSelection();
	void SelectItem( int item );
	bool IsSelectedItem( int item );

	void SetSnapFPS(UINT fps)
	{ m_snapFrameTime = fps == 0 ? 0.033333f : 1.0f / float(fps); }

	ETeVTickMode GetTickDisplayMode() const
	{ return m_tickDisplayMode; }
	void SetTickDisplayMode(ETeVTickMode mode)
	{ 
		m_tickDisplayMode = mode; 
		SetTimeScale(GetTimeScale(), 0);	// for refresh
	}

	void SetTimelinesCtrl(CTelemetryViewTimelinesCtrl* pCtrl) { m_pTimelinesCtrl = pCtrl; }

	//! Return client position for given time.
	int TimeToClient( float time ) const;

	//////////////////////////////////////////////////////////////////////////
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );
	//////////////////////////////////////////////////////////////////////////

protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	afx_msg void MeasureItem(LPMEASUREITEMSTRUCT /*lpMeasureItemStruct*/);
	afx_msg HBRUSH CtlColor(CDC* /*pDC*/, UINT /*nCtlColor*/);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRawInput( UINT wParam,HRAWINPUT lParam );

	//////////////////////////////////////////////////////////////////////////
	// Drawing methods.
	//////////////////////////////////////////////////////////////////////////
	void DrawControl( CDC *dc,const CRect &rcUpdate );
	void DrawTimeline( int item, CDC *dc, CRect &rcItem, Telemetry::CTelemetryTimeline* pTimeline );
	void DrawTicks( CDC *dc,CRect &rc,Range &timeRange );
	void DrawRuler( CDC *dc,const CRect &rcUpdate );
	void DrawEvent( const Telemetry::STelemetryEvent& event, CDC *dc, CRect &rcItem );
	void RedrawItem( int item );

	//////////////////////////////////////////////////////////////////////////
	//! Return time snapped to timestep,
	float TickSnap( float time ) const;
	float MagnetSnap( float time, const IAnimNode * node ) const;
	float FrameSnap( float time) const;

	//! Returns visible time range.
	Range GetVisibleRange();
	Range GetTimeRange( CRect &rc );

	float TimeFromPoint( CPoint point ) const;
	float TimeFromPointUnsnapped( CPoint point ) const;

	void SetLeftOffset( int ofs ) { m_leftOffset = ofs; };

	void SetMouseCursor( HCURSOR crs );

	void ShowEventTooltip( Telemetry::CTimelineRecord* pRecord, CPoint point );

protected:
	//////////////////////////////////////////////////////////////////////////
	// FIELDS.
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	CBrush m_bkgrBrush;
	CBrush m_bkgrBrushEmpty;
	CBrush m_selectedBrush;
	CBrush m_timeBkgBrush;
	CBrush m_timeHighlightBrush;
	CBrush m_visibilityBrush;
	CBrush m_selectTrackBrush;

	HCURSOR m_currCursor;
	HCURSOR m_crsLeftRight;
	HCURSOR m_crsAddKey;
	HCURSOR m_crsCross;

	CRect m_rcClient;
	CPoint m_scrollOffset;
	CRect m_rcSelect;
	CRect m_rcTimeline;
	CRect m_rcSummary;

	CPoint m_lastTooltipPos;
	CPoint m_mouseDownPos;
	CPoint m_mouseOverPos;
	CImageList m_imageList;
	CImageList m_imgMarker;

	CBitmap m_offscreenBitmap;

	bool m_bZoomDrag;
	bool m_bMoveDrag;

	//////////////////////////////////////////////////////////////////////////
	// Time.
	float m_timeScale;
	float m_currentTime;
	float m_storedTime;
	Range m_timeRange;
	Range m_realTimeRange;
	Range m_timeMarked;

	//! This is how often to place ticks.
	//! value of 10 means place ticks every 10 second.
	double m_ticksStep;

	//////////////////////////////////////////////////////////////////////////
	int m_mouseMode;
	int m_actionMode;
	bool m_bAnySelected;
	float m_keyTimeOffset;

	CPropertyCtrl m_wndPropsOnSpot;
	CTelemetryViewTimelinesCtrl* m_pTimelinesCtrl;

	CFont *m_descriptionFont;


	int m_itemWidth;
	int m_itemHeight;
 
	int m_leftOffset;
	int m_scrollMin,m_scrollMax;

	CToolTipCtrl m_tooltip;

	bool m_bCursorWasInKey;

	bool m_bMouseMovedAfterRButtonDown;

	float m_snapFrameTime;

	ETeVTickMode m_tickDisplayMode;

	void MouseMoveSelect( CPoint point );
	void MouseMoveMove( CPoint point, UINT nFlags );

	void MouseMoveDragTime( CPoint point, UINT nFlags );
	void MouseMoveOver( CPoint point );
	void MouseMovePaste( CPoint point, UINT nFlags );
	void MouseMoveDragEndMarker( CPoint point, UINT nFlags );
	void MouseMoveDragStartMarker( CPoint point, UINT nFlags );
};

#endif // __trackviewkeys_h__