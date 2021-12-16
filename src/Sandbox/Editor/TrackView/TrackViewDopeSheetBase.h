////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002 - 2012.
// -------------------------------------------------------------------------
//  File name:   TrackViewDopeSheetBase.h
//  Created:     23/8/2002 by Timur.
// -------------------------------------------------------------------------
//  History: May 2012 - Axel Gneiting - Refactoring
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <IMovieSystem.h>
#include "Controls\PropertyCtrl.h"
#include "TrackViewNode.h"
#include "TrackViewSequence.h"
#include "AnimationContext.h"

class CTVTrackPropsDialog;
class CTrackViewNodesCtrl;
class CTrackViewKeyPropertiesDlg;
class CTrackViewNode;
class CTrackViewTrack;
class CTrackViewAnimNode;

enum ETVActionMode
{
	eTVActionMode_MoveKey = 1,
	eTVActionMode_AddKeys,
	eTVActionMode_SlideKey,
	eTVActionMode_ScaleKey,
};

enum ESnappingMode
{
	eSnappingMode_SnapNone = 0,
	eSnappingMode_SnapTick,
	eSnappingMode_SnapMagnet,
	eSnappingMode_SnapFrame,
};

enum ETVTickMode
{
	eTVTickMode_InSeconds = 0,
	eTVTickMode_InFrames,
};
	
/** TrackView DopeSheet interface
*/
class CTrackViewDopeSheetBase : public CWnd, public IAnimationContextListener, public ITrackViewSequenceListener
{	
public:
	CTrackViewDopeSheetBase();
	virtual ~CTrackViewDopeSheetBase();

	void SetNodesCtrl(CTrackViewNodesCtrl *pNodesCtrl) { m_pNodesCtrl = pNodesCtrl; }

	void SetTimeScale(float timeScale,float fAnchorTime);
	float GetTimeScale() { return m_timeScale; }

	void SetScrollOffset(int hpos);

	void SetTimeRange(float start,float end);		
	void SetStartMarker(float fTime);
	void SetEndMarker(float fTime);

	void SetMouseActionMode(ETVActionMode mode);

	void SetKeyPropertiesDlg(CTrackViewKeyPropertiesDlg *dlg) { m_keyPropertiesDlg = dlg; }

	void SetSnappingMode(ESnappingMode mode) { m_snappingMode = mode; }
	ESnappingMode GetSnappingMode() const { return m_snappingMode; }
	void SetSnapFPS(UINT fps);

	ETVTickMode GetTickDisplayMode() const { return m_tickDisplayMode; }
	void SetTickDisplayMode(ETVTickMode mode);

	void SetEditLock(bool bLock) { m_bEditLock = bLock; }

	// IAnimationContextListener
	virtual void OnTimeChanged(float newTime) override;

	float TickSnap(float time) const;

protected:
	DECLARE_DYNAMIC(CTrackViewDopeSheetBase)
	DECLARE_MESSAGE_MAP()

private:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
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
	afx_msg void OnRawInput(UINT wParam,HRAWINPUT lParam);
	afx_msg void OnCaptureChanged(CWnd* pWnd);

private:
	void AddKeys(CPoint point, const bool bTryAddKeysInGroup);

	void ShowKeyPropertyCtrlOnSpot(int x, int y, bool bMultipleKeysSelected, bool bKeyChangeInSameTrack);
	void HideKeyPropertyCtrlOnSpot();

	XmlNodeRef GetKeysInClickboard() const;
	void StartPasteKeys();	

	CTrackViewKeyHandle FirstKeyFromPoint(CPoint point);
	CTrackViewKeyHandle DurationKeyFromPoint(CPoint point);
	CTrackViewKeyHandle CheckCursorOnStartEndTimeAdjustBar(CPoint point, bool& bStart);

	void SelectKeys(const CRect &rc, const bool bMultiSelection);

	int NumKeysFromPoint(CPoint point);

	//! Select all keys within time frame defined by this client rectangle.
	void SelectAllKeysWithinTimeFrame(const CRect &rc, const bool bMultiSelection);
		
	//! Return time snapped to time step,
	double GetTickTime() const;
	float MagnetSnap(float time, const CTrackViewAnimNode *pNode) const;
	float FrameSnap(float time) const;

	//! Return move time offset snapped with current snap settings
	float ComputeSnappedMoveOffset();

	//! Returns visible time range.
	Range GetVisibleRange() const;
	Range GetTimeRange(CRect &rc) const;

	void SetHorizontalExtent(int min, int max);

	void SetCurrTime(float time);

	//! Return client position for given time.
	int TimeToClient(float time) const;

	float TimeFromPoint(CPoint point) const;
	float TimeFromPointUnsnapped(CPoint point) const;

	void SetLeftOffset(int ofs) { m_leftOffset = ofs; };

	void SetMouseCursor(HCURSOR crs);
		
	void ShowKeyTooltip(CTrackViewKeyHandle &keyHandle, CPoint point);
		
	bool IsOkToAddKeyHere(const CTrackViewTrack *pTrack, float time) const;

	void MouseMoveSelect(CPoint point);
	void MouseMoveMove(CPoint point, UINT nFlags);

	void MouseMoveDragTime(CPoint point, UINT nFlags);
	void MouseMoveOver(CPoint point);
	void MouseMoveDragEndMarker(CPoint point, UINT nFlags);

	float SnapTime(UINT nFlags, CPoint p);

	void MouseMoveDragStartMarker(CPoint point, UINT nFlags);
	void MouseMoveStartEndTimeAdjust(CPoint point, bool bStart);

	CTrackViewNode *GetNodeFromPointRec(CTrackViewNode *pCurrentNode, CPoint point);
	CTrackViewNode *GetNodeFromPoint(CPoint point);
	CTrackViewAnimNode *GetAnimNodeFromPoint(CPoint point);
	CTrackViewTrack *GetTrackFromPoint(CPoint point);

	void LButtonDownOnTimeAdjustBar(CPoint point, CTrackViewKeyHandle &keyHandle, bool bStart);
	void LButtonDownOnKey(CPoint point, CTrackViewKeyHandle &keyHandle, UINT nFlags);

	bool CreateColorKey(CTrackViewTrack *pTrack, float keyTime);
	
	void RecordTrackUndo(CTrackViewTrack *pTrack);
	void AcceptUndo();	

	// Returns the snapping mode modified active keys
	ESnappingMode GetKeyModifiedSnappingMode();

	CRect GetNodeRect(const CTrackViewNode *pNode) const;

	void StoreMementoForTracksWithSelectedKeys();

	// Drawing methods.
	void DrawControl(CDC *pDC, const CRect &rcUpdate);
	void DrawNodesRecursive(CTrackViewNode *pNode, CDC *pDC, const CRect &rcUpdate);
	void DrawTimeline(CDC *pDC, const CRect &rcUpdate);
	void DrawSummary(CDC *pDC, CRect rcUpdate);
	void DrawSelectedKeyIndicators(CDC *pDC);
	void DrawTicks(CDC *pDC,CRect &rc, Range &timeRange);
	void DrawNodeTrack(CTrackViewAnimNode *pAnimNode, CDC *pDC, CRect trackRect);
	void DrawTrack(CTrackViewTrack *pTrack, CDC *pDC, CRect trackRect);
	void DrawKeys(CTrackViewTrack *pTrack, CDC *pDC, CRect &rc, Range &timeRange);
	void DrawSequenceTrack(const Range &timeRange, CDC *pDC, CTrackViewTrack *pTrack, const CRect &rc);
	void DrawBoolTrack(const Range &timeRange, CDC *pDC, CTrackViewTrack *pTrack, const CRect &rc);
	void DrawSelectTrack(const Range &timeRange, CDC *pDC, CTrackViewTrack *pTrack, const CRect &rc);
	void DrawKeyDuration(CTrackViewTrack *pTrack, CDC *pDC, CRect &rc, int keyIndex);
	void DrawGoToTrackArrow(CTrackViewTrack *pTrack, CDC *pDC, CRect &rc);
	void DrawColorGradient(CDC *pDC, const CRect &rc, const CTrackViewTrack *pTrack);
	void DrawClipboardKeys(CDC *pDC, const CRect &rc);
	void DrawTrackClipboardKeys(CDC *pDC, CTrackViewTrack *pTrack, XmlNodeRef trackNode, const float timeOffset);

	CTrackViewNodesCtrl *m_pNodesCtrl;
	void ComputeFrameSteps( const Range &VisRange ); 
	void DrawTimeLineInFrames( CDC *dc, CRect &rc, COLORREF &lineCol, COLORREF &textCol, double step );
	void DrawTimeLineInSeconds( CDC *dc, CRect &rc, COLORREF &lineCol, COLORREF &textCol, double step );

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
	HCURSOR m_crsAdjustLR;

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

	// Time
	float m_timeScale;
	float m_currentTime;
	float m_storedTime;
	Range m_timeRange;
	Range m_timeMarked;

	// This is how often to place ticks.
	// value of 10 means place ticks every 10 second.
	double m_ticksStep;

	CTrackViewKeyPropertiesDlg *m_keyPropertiesDlg;
	CPropertyCtrl m_wndPropsOnSpot;
	const CTrackViewTrack *m_pLastTrackSelectedOnSpot;

	CFont *m_descriptionFont;

	// Mouse interaction state
	int m_mouseMode;
	int m_mouseActionMode;		
	bool m_bZoomDrag;
	bool m_bMoveDrag;
	bool m_bCursorWasInKey;
	bool m_bJustSelected;
	bool m_bMouseMovedAfterRButtonDown;	
	bool m_bKeysMoved;

	// Offset for keys while moving/pasting
	float m_keyTimeOffset;

	// If control is locked for editing
	bool m_bEditLock;	

	// Fast redraw: Only redraw time slider. Everything else is buffered.
	bool m_bFastRedraw;

	// Scrolling
	int m_leftOffset;
	int m_scrollMin;
	int m_scrollMax;

	// Snapping
	ESnappingMode m_snappingMode;	
	float m_snapFrameTime;

	// Tooltip for keys
	CToolTipCtrl m_tooltip;

	// Ticks in frames or seconds
	ETVTickMode m_tickDisplayMode;
	double m_fFrameTickStep; 
	double m_fFrameLabelStep; 	

	// Key for time adjust
	CTrackViewKeyHandle m_keyForTimeAdjust;

	// Cached clipboard XML for eTVMouseMode_Paste
	XmlNodeRef m_clipboardKeys;

	// Mementos of unchanged tracks for Move/Scale/Slide etc.
	struct TrackMemento
	{
		CTrackViewTrackMemento m_memento;

		// Also need to store key selection states, 
		// because RestoreMemento will destroy them
		std::vector<bool> m_keySelectionStates;
	};

	std::unordered_map<CTrackViewTrack*, TrackMemento> m_trackMementos;

#ifdef DEBUG
	unsigned int m_redrawCount;
#endif
};
