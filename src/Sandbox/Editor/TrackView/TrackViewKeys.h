////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   TrackViewKeys.h
//  Version:     v1.00
//  Created:     23/8/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __trackviewkeys_h__
#define __trackviewkeys_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include <IMovieSystem.h>

class CTVTrackPropsDialog;

// CTrackViewKeys

enum ETVActionMode
{
	TVMODE_MOVEKEY = 1,
	TVMODE_ADDKEY,
	TVMODE_SLIDEKEY,
	TVMODE_SCALEKEY,
};

enum ESnappingMode
{
	TVKEY_SNAP_NONE = 0,
	TVKEY_SNAP_TICK,
	TVKEY_SNAP_MAGNET,
	TVKEY_SNAP_FRAME,
};
	
/** Base class for TrackView key editing dialogs.
*/
class CTrackViewKeys : public CWnd, public IEditorNotifyListener
{
	DECLARE_DYNAMIC(CTrackViewKeys)
public:
	struct Item
	{
		int nHeight;
		_smart_ptr<IAnimTrack> track;
		_smart_ptr<IAnimNode> node;
		int paramId;
		bool bSelected;
		//////////////////////////////////////////////////////////////////////////
		Item() { nHeight = 16; track = 0; node = 0; paramId = 0; bSelected = false; }
		Item( int height ) { nHeight = height; track = 0; node = 0; paramId = 0; bSelected = false; }
		Item( IAnimNode *pNode,int nParamId,IAnimTrack *pTrack )
		{
			nHeight = 16;
			track = pTrack; paramId = nParamId; node = pNode;
			bSelected = false;
		}
		Item( int height,IAnimNode *pNode,int nParamId,IAnimTrack *pTrack )
		{
			nHeight = height;
			track = pTrack; paramId = nParamId; node = pNode;
			bSelected = false;
		}
		Item( int height,IAnimNode *pNode )
		{
			nHeight = height;
			track = 0; paramId = 0; node = pNode;
			bSelected = false;
		}
	};

	CTrackViewKeys();
	virtual ~CTrackViewKeys();

	void SetSequence( IAnimSequence *pSequence ) { m_pSequence = pSequence; };
	void SetTimeScale( float timeScale,float fAnchorTime );
	float GetTimeScale() { return m_timeScale; }

	void SetTimeRange( float start,float end );
	void SetCurrTime( float time );
	float GetCurrTime() const;
	void SetStartMarker(float fTime);
	void SetEndMarker(float fTime);

	void DelSelectedKeys();
	void SetMouseActionMode( ETVActionMode mode );

	bool CanCopyPasteKeys();
	bool CopyPasteKeys();

	bool CopyKeys();
	bool PasteKeys( IAnimNode * pAnimNode,float fTimeOffset );
	void StartPasteKeys();

	void SetTrackDialog( CTVTrackPropsDialog *dlg ) { m_wndTrack = dlg; };

	//////////////////////////////////////////////////////////////////////////
	// Tracks access.
	//////////////////////////////////////////////////////////////////////////
	int GetCount() const { return m_tracks.size(); }
	void AddItem( const Item &item );
	const Item& GetItem( int item ) const;
	IAnimTrack* GetTrack( int item ) const;
	IAnimNode*  GetNode( int item ) const;
	int GetHorizontalExtent() const { return m_itemWidth; };	
	
	virtual void	SetHorizontalExtent( int min,int max );
	virtual int		ItemFromPoint( CPoint pnt ) const;
	virtual int		GetItemRect( int item,CRect &rect ) const;
	virtual void  InvalidateItem( int item );
	virtual void	ResetContent() { m_tracks.clear(); }

	void ClearSelection();
	void SelectItem( int item );
	bool IsSelectedItem( int item );

	void SetSnappingMode(ESnappingMode mode)
	{ m_snappingMode = mode; }
	void SetSnapFPS(UINT fps)
	{ m_snapFrameTime = fps == 0 ? 0.033333f : 1.0f / float(fps); }

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
	virtual void DrawControl( CDC *dc,const CRect &rcUpdate );
	virtual void DrawTrack( int item,CDC *dc,CRect &rcItem );
	virtual void DrawTicks( CDC *dc,CRect &rc,Range &timeRange );
	virtual void DrawTimeline( CDC *dc,const CRect &rcUpdate );
	virtual void DrawSummary( CDC *dc, CRect rcUpdate );
	virtual void DrawSelectedKeyIndicators( CDC *dc );
	virtual void DrawKeys( IAnimTrack *track,CDC *dc,CRect &rc,Range &timeRange );
	virtual void RedrawItem( int item );

	//////////////////////////////////////////////////////////////////////////
	// Must be overriden.
	//////////////////////////////////////////////////////////////////////////
	//! Find a key near this point.
	virtual int FirstKeyFromPoint( CPoint point ) const;
	//! Select keys inside this client rectangle.
	virtual void SelectKeys( const CRect &rc );
	//! Select all keys within time frame defined by this client rectangle.
	virtual void SelectAllKeysWithinTimeFrame( const CRect &rc );

	//////////////////////////////////////////////////////////////////////////
	//! Return time snapped to timestep,
	float TickSnap( float time ) const;
	float MagnetSnap( float time, const IAnimNode * node ) const;
	float FrameSnap( float time) const;

	//! Returns visible time range.
	Range GetVisibleRange();
	Range GetTimeRange( CRect &rc );

	//! Return client position for given time.
	int TimeToClient( float time ) const;

	float TimeFromPoint( CPoint point ) const;
	float TimeFromPointUnsnapped( CPoint point ) const;

	//! Unselect all selected keys.
	void UnselectAllKeys( bool bNotify );
	//! Offset all selected keys by this offset.
	void OffsetSelectedKeys( float timeOffset,bool bSlide,bool bSnapKeys );
	//! Scale all selected keys by this offset.
	void ScaleSelectedKeys( float timeOffset,bool bSnapKeys );
	void CloneSelectedKeys();

	bool FindSingleSelectedKey( IAnimTrack* &track,int &key );

	//////////////////////////////////////////////////////////////////////////
	void SetKeyInfo( IAnimTrack *track,int key,bool openWindow=false );

	void UpdateAnimation();
	void SetLeftOffset( int ofs ) { m_leftOffset = ofs; };

	void SetMouseCursor( HCURSOR crs );

	void RecordTrackUndo( const Item &item );
	void ShowKeyTooltip( IAnimTrack *pTrack,int nKey,CPoint point );

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

	bool m_bMouseOverKey;

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
	int m_mouseActionMode;
	int m_actionMode;
	bool m_bAnySelected;
	float m_keyTimeOffset;

	CTVTrackPropsDialog *m_wndTrack;

	CFont *m_descriptionFont;

	//////////////////////////////////////////////////////////////////////////
	// Track list related.
	//////////////////////////////////////////////////////////////////////////
	std::vector<Item> m_tracks;

	int m_itemWidth;
	int m_itemHeight;
 
	int m_leftOffset;
	int m_scrollMin,m_scrollMax;

	CToolTipCtrl m_tooltip;

	IAnimSequence *m_pSequence;

	bool m_bCursorWasInKey;

	bool m_bJustSelected;

	bool m_bMouseMovedAfterRButtonDown;

	ESnappingMode m_snappingMode;
	float m_snapFrameTime;

	void OffsetKey( IAnimTrack *pTrack, int keyIndex, float timeOffset );
	void NotifyKeySelectionUpdate();
	bool IsOkToAddKeyHere(const IAnimTrack *pTrack, float time) const;

	void MouseMoveSelect( CPoint point );
	void MouseMoveMove( CPoint point, UINT nFlags );

	void MouseMoveDragTime( CPoint point, UINT nFlags );
	void MouseMoveOver( CPoint point );
	void MouseMovePaste( CPoint point, UINT nFlags );
	void MouseMoveDragEndMarker( CPoint point, UINT nFlags );
	void MouseMoveDragStartMarker( CPoint point, UINT nFlags );

	int GetAboveKey(IAnimTrack *&track);
	int GetBelowKey(IAnimTrack *&track);
	int GetRightKey(IAnimTrack *&track);
	int GetLeftKey(IAnimTrack *&track);
};

#endif // __trackviewkeys_h__