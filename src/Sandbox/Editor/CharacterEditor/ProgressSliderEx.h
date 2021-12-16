#pragma once

#include "ProgressSlider.h"

class IAnimEventDoc;

class IProgressSliderExListener
{
public:

	enum EEvent
	{
		eEvent_Select,
		eEvent_ClearAllSelections,
		eEvent_TimeUpdate
	};

	virtual void OnProgressSliderExEvent( EEvent event, std::map<int,float>* ids ) = 0;
};

class CProgressSliderEx : public CProgressSlider
{
	DECLARE_DYNAMIC(CProgressSliderEx)

public:

	CProgressSliderEx()
	{
		m_bPropagatingSelection = false;
		m_State = eMouseState_None;
		m_nFirstSelectedIndex = -1;
		m_nIndexForDisplayTooltip = -1;
		m_TooltipFont.CreatePointFont(65,_T("Tahoma"));
		m_ZoomRangeMin = 0;
		m_ZoomRangeMax = 1;
		m_bThumbInTrackArea = true;
		m_TrackAreaRect = CRect(0,0,0,0);
		m_MarkerAreaRect = CRect(0,0,0,0);
		m_ScaleAreaRect = CRect(0,0,0,0);
	}

	~CProgressSliderEx()
	{
		RemoveAllRgnList();
	}

	void SetSliderExListener( IProgressSliderExListener* pListener )
	{
		m_pListener = pListener;
	}

	void SetDoc( IAnimEventDoc* pDoc )
	{
		m_pDoc = pDoc;
	}

	void SetMarkerListWithIDs(DynArray<f32> markers, const std::vector<int>& idList) override;

	void ClearSelectedMarkers();
	void SelectMarkers( std::set<int>& idList );
	void ResetSelections()	{	m_SelectedRgnIndices.clear();	}
	void ResetZoom();

	void SetThumbPosFromAbsolutePos( int nAbsolutePos ) override;
	int GetThumbAbsolutePos() const;

	DECLARE_MESSAGE_MAP()

private:

	void OnPaint() override;
	void OnLButtonDown(UINT nFlags, CPoint point) override;
	void OnLButtonUp(UINT nFlags, CPoint point) override;
	void OnMouseMove(UINT nFlags, CPoint point) override;
	afx_msg BOOL OnMouseWheel( UINT nFlags, short zDelta, CPoint pt );

	void ClearBackbuffer( const CRect& rect, CDC& dc );
	void DrawTrack( CDC& dc );
	void DrawMarkers( CDC& dc );
	void DrawMarker( CDC& dc, CRgn& region, CBrush& brush );
	void DrawThumb( CDC& dc );
	void DrawNumbers( CDC& dc );
	void DrawTransparent( const CRect& rect, CDC& dc );
	void DrawTooltip( CDC& dc );

	int GetThumbWidth() const			{	return 6; }
	int GetThumbHeight() const;

	int GetMarkerWidth() const	{	return 10; }
	int GetTrackLength() const;

	int GetRelativePositionX( float fAbsolute ) const;
	float GetThumbPositionX() override;
	float SetThumbPositionX(float fClientPosX) override;

	CRect GetScaleMarkRect() const;
	void ComputeScaleAreaRect() const;

	CRect GetTrackAreaRect() const;
	void ComputeTrackAreaRect() const;

	CRect GetMarkerAreaRect() const;
	void ComputeMarkerAreaRect() const;

	CRect GetThumbRect( int nAbsolutePos ) const;

	void RemoveAllRgnList();
	int HitTest( const CPoint& point );

	CRect GetMarkersRect( int nIndex ) const;
	int GetMarkersCenterX( int nIndex ) const;

	CRect GetSelectedMarkersRect() const;
	void UpdateAllMarkers();

	bool IsInTrack( const CPoint& point ) const{
		return GetTrackAreaRect().PtInRect(point);
	}

	CRgn* CreateRgn( float fPos ) const;

	struct SMarkerInfo
	{
		SMarkerInfo()
		{
			m_pRgn = NULL;
		}
		~SMarkerInfo()
		{
			if( m_pRgn )
				delete m_pRgn;
		}
		float m_fAbsolutePos;
		CRgn* m_pRgn;
		int m_EventID;
	};

	enum eMouseState
	{
		eMouseState_None,
		eMouseState_PickMarker,
		eMouseState_DrawRectangle,
		eMouseState_Draging
	};

	mutable CRect m_TrackAreaRect;
	mutable CRect m_MarkerAreaRect;
	mutable CRect m_ScaleAreaRect;

	std::vector<SMarkerInfo> m_MarkerRgnList;
	std::set<int> m_SelectedRgnIndices;

	CPoint m_StartPoint;
	CPoint m_EndPoint;
	eMouseState m_State;

	bool m_bPropagatingSelection;

	CPoint m_PointWhenLButtonDown;
	CPoint m_PrevPoint;
	int m_nFirstSelectedIndex;

	int m_nIndexForDisplayTooltip;
	CAnimEventData m_TooltipContents;

	IProgressSliderExListener* m_pListener;
	IAnimEventDoc* m_pDoc;

	float m_ZoomRangeMin;
	float m_ZoomRangeMax;

	bool m_bThumbInTrackArea;
	CFont m_TooltipFont;

};