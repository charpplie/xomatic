#include "StdAfx.h"
#include "ProgressSliderEx.h"
#include "AnimEventEditor/AnimEventJobs.h"
#include "AnimEventEditor/IAnimEventDoc.h"

static const float kScaleMarkRatio = 0.3f;
static const float kTrackRatio = 0.2f;
static const float fMarkerRatio = 0.5f;

IMPLEMENT_DYNAMIC(CProgressSliderEx, CProgressSlider)

BEGIN_MESSAGE_MAP(CProgressSliderEx, CProgressSlider)
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

void CProgressSliderEx::OnPaint()
{
	InitialisePainting();

	CRect rect;
	if (!GetUpdateRect(&rect))
		return;
	GetClientRect(&rect);

	Invalidate(FALSE);

	PAINTSTRUCT paintStruct;
	CDC* pDC = BeginPaint(&paintStruct);

	ClearBackbuffer( rect, m_memoryDC );
	DrawNumbers( m_memoryDC );
	DrawTrack( m_memoryDC );
	DrawMarkers( m_memoryDC );
	DrawThumb( m_memoryDC );
	if (!IsWindowEnabled())
		DrawTransparent( rect, m_memoryDC );

	if( m_nIndexForDisplayTooltip != -1 )
		DrawTooltip( m_memoryDC );

	if( m_State == eMouseState_DrawRectangle )
	{
		m_memoryDC.SetDCPenColor(RGB(120,120,120));
		m_memoryDC.MoveTo( m_StartPoint.x, m_StartPoint.y );
		m_memoryDC.LineTo( m_EndPoint.x, m_StartPoint.y );
		m_memoryDC.LineTo( m_EndPoint.x, m_EndPoint.y );
		m_memoryDC.LineTo( m_StartPoint.x, m_EndPoint.y );
		m_memoryDC.LineTo( m_StartPoint.x, m_StartPoint.y );
	}

	pDC->BitBlt(
		0, 
		0, 
		rect.right - rect.left, 
		rect.bottom - rect.top, 
		&m_memoryDC, 
		0, 
		0, 
		SRCCOPY);

	EndPaint(&paintStruct);
}

void CProgressSliderEx::DrawTooltip( CDC& dc )
{
	CRect rect;
	GetClientRect(&rect);

	CRect markRect = GetMarkersRect(m_nIndexForDisplayTooltip);
	CRect clientRect;
	GetClientRect(clientRect);

	CRect tooltipRect( markRect.CenterPoint(), CSize(100,30) );
	if( markRect.CenterPoint().x > rect.CenterPoint().x )
	{
		tooltipRect = CRect( markRect.CenterPoint(), CSize(-100,30) );
		std::swap(tooltipRect.left,tooltipRect.right);
	}

	dc.Rectangle(&tooltipRect);

	CFont* pOldFont = dc.SelectObject(&m_TooltipFont);
	dc.SetBkColor(RGB(255,255,255));
	dc.TextOutA( tooltipRect.left+2, tooltipRect.top+2, m_TooltipContents.GetName() ? m_TooltipContents.GetName() : "" );

	CString timeRange;
	timeRange.Format("[%3.4f ~ %3.4f]", m_TooltipContents.GetNormalizedTime(), m_TooltipContents.GetNormalizedEndTime() );
	dc.TextOutA( tooltipRect.left+2, tooltipRect.top+2+14, timeRange );

	dc.SelectObject(pOldFont);
}

void CProgressSliderEx::ClearBackbuffer( const CRect& rect, CDC& dc )
{
	CBrush backgroundBrush;
	backgroundBrush.Attach(GetSysColorBrush(COLOR_BTNFACE));
	dc.FillRect(rect, &backgroundBrush);
	backgroundBrush.Detach();
}

void CProgressSliderEx::DrawTransparent( const CRect& rect, CDC& dc )
{
	BLENDFUNCTION blend;
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.SourceConstantAlpha = 128;
	blend.AlphaFormat = 0;
	dc.AlphaBlend(
		0, 
		0, 
		rect.right - rect.left, 
		rect.bottom - rect.top, 
		&m_disabledBitmapDC, 
		0, 
		0, 
		rect.right - rect.left, 
		rect.bottom - rect.top, 
		blend);
}

CRect CProgressSliderEx::GetThumbRect( int nAbsolutePos ) const
{
	int nThumbWidth = GetThumbWidth();
	int nThumbHeight = GetThumbHeight();
	CRect trackRect = GetTrackAreaRect();
	int y = ((trackRect.top+trackRect.bottom)/2)-(nThumbHeight/2);
	CRect thumbRect(
		CPoint(GetRelativePositionX((float)nAbsolutePos/NUM_INCREMENTS)-nThumbWidth/2, y),
		CSize(nThumbWidth,nThumbHeight) );
	return thumbRect;
}

void CProgressSliderEx::SetThumbPosFromAbsolutePos( int nAbsolutePos )
{
	float width = m_ZoomRangeMax - m_ZoomRangeMin;
	int nAdjustedPos = ((((float)nAbsolutePos/(float)NUM_INCREMENTS)-m_ZoomRangeMin)/width) * NUM_INCREMENTS + 0.5f;

	CRect trackRect = GetTrackAreaRect();
	CRect thumbRect = GetThumbRect(nAbsolutePos);
	if( thumbRect.right < trackRect.left || thumbRect.left > trackRect.right )
		m_bThumbInTrackArea = false;
	else
		m_bThumbInTrackArea = true;

	SetPos(nAdjustedPos);
}

int CProgressSliderEx::GetThumbAbsolutePos() const
{
	float width = m_ZoomRangeMax - m_ZoomRangeMin;
	return (((float)GetPos()/(float)NUM_INCREMENTS) * width + m_ZoomRangeMin) * NUM_INCREMENTS + 0.5f;
}

void CProgressSliderEx::DrawThumb( CDC& dc )
{
	if( !m_bThumbInTrackArea )
		return;

	CRect trackRect = GetTrackAreaRect();
	CRect thumbRect = GetThumbRect(GetThumbAbsolutePos());

	CBrush brush;
	brush.CreateSolidBrush(RGB(100,200,100));
	dc.FillRect(&thumbRect,&brush);
}

void CProgressSliderEx::DrawNumbers( CDC& dc )
{
	CRect rect = GetScaleMarkRect();

	COLORREF color = ::GetSysColor(COLOR_BTNFACE);
	dc.SetBkColor(color);

	CSize textSize = dc.GetTextExtent("0");
	dc.TextOutA( GetRelativePositionX(0)-textSize.cx/2, rect.top, "0");

	textSize = dc.GetTextExtent("0.5");
	dc.TextOutA( GetRelativePositionX((float)(rect.CenterPoint().x-rect.left)/(float)rect.Width())-textSize.cx/2, rect.top, "0.5");

	textSize = dc.GetTextExtent("1");
	dc.TextOutA( GetRelativePositionX(1)-textSize.cx/2, rect.top, "1");
}

void CProgressSliderEx::DrawTrack( CDC& dc )
{
	CRect trackRect = GetTrackAreaRect();
	int centerY = (trackRect.top+trackRect.bottom)/2;
	int barHeight = trackRect.Height() * 0.3f;

	CPoint vPivot(trackRect.left, centerY-barHeight);
	CSize vSize(trackRect.Width()+1, barHeight*2);
	CRect r(vPivot,vSize);
	CBrush barBrush;
	barBrush.CreateSolidBrush(RGB(100,100,100));
	dc.FillRect(&r,&barBrush);

	dc.SetDCPenColor(RGB(0,0,0));

	int tickSize = 4;
	float fInterval = (float)trackRect.Width()/10.0f;

	for( int i = 0; i < 11; ++i )
	{
		CPoint vPosition = vPivot + CPoint(fInterval*i,0);
		vPosition.x = GetRelativePositionX((float)(vPosition.x-r.left)/(float)trackRect.Width());
		dc.MoveTo(vPosition);
		dc.LineTo(vPosition+CPoint(0,-tickSize));
	}
}

void CProgressSliderEx::DrawMarkers( CDC& dc )
{
	CBrush solidBrush;
	solidBrush.CreateSolidBrush(RGB(255,255,20));

	CBrush selectedBrush;
	selectedBrush.CreateSolidBrush(RGB(255,20,20));

	for( int k = 0; k < 2; ++k )
	{
		for (uint32 i = 0, iSize(m_MarkerRgnList.size()); i < iSize; ++i)
		{
			if( k == 1 && m_SelectedRgnIndices.find(i) != m_SelectedRgnIndices.end() )
				DrawMarker(dc,*m_MarkerRgnList[i].m_pRgn,selectedBrush);
			else if( k == 0 && m_SelectedRgnIndices.find(i) == m_SelectedRgnIndices.end() )
				DrawMarker(dc,*m_MarkerRgnList[i].m_pRgn,solidBrush);
		}
	}
}

void CProgressSliderEx::DrawMarker( CDC& dc, CRgn& region, CBrush& brush )
{
	dc.FillRgn(&region,&brush);
	CBrush frameBrush;
	frameBrush.CreateSolidBrush(RGB(0,0,0));
	dc.SetDCPenColor(RGB(0,0,0));
	dc.FrameRgn(&region,&frameBrush,1,1);
}

int CProgressSliderEx::GetRelativePositionX( float fAbsolute ) const
{
	if( fAbsolute > 1 )
		fAbsolute = 1.0f;

	float width = m_ZoomRangeMax - m_ZoomRangeMin;

	CRect trackRect = GetTrackAreaRect();
	return ((fAbsolute-m_ZoomRangeMin)/width)*trackRect.Width() + trackRect.left;
}

int CProgressSliderEx::GetTrackLength() const
{
	CRect rect = GetTrackAreaRect();
	return rect.Width();
}

CRect CProgressSliderEx::GetScaleMarkRect() const
{
	if( m_ScaleAreaRect.Width() == 0 || m_ScaleAreaRect.Height() == 0 )
		ComputeScaleAreaRect();
	return m_ScaleAreaRect;
}

void CProgressSliderEx::ComputeScaleAreaRect() const
{
	GetClientRect(&m_ScaleAreaRect);
	m_ScaleAreaRect.bottom = m_ScaleAreaRect.Height() * kScaleMarkRatio;
}

CRect CProgressSliderEx::GetTrackAreaRect() const
{
	if( m_TrackAreaRect.Width() == 0 || m_TrackAreaRect.Height() == 0 )
		ComputeTrackAreaRect();
	return m_TrackAreaRect;
}

void CProgressSliderEx::ComputeTrackAreaRect() const
{
	GetClientRect(&m_TrackAreaRect);

	m_TrackAreaRect.left	+= GetMarkerWidth()/2;
	m_TrackAreaRect.right	-= GetMarkerWidth()/2;

	m_TrackAreaRect.top = m_TrackAreaRect.Height()*kScaleMarkRatio;
	m_TrackAreaRect.bottom = m_TrackAreaRect.top + m_TrackAreaRect.Height()*kTrackRatio;
}

CRect CProgressSliderEx::GetMarkerAreaRect() const
{
	if( m_MarkerAreaRect.Width() == 0 || m_MarkerAreaRect.Height() == 0 )
		ComputeMarkerAreaRect();
	return m_MarkerAreaRect;
}

void CProgressSliderEx::ComputeMarkerAreaRect() const
{
	GetClientRect(&m_MarkerAreaRect);

	m_MarkerAreaRect.left		+= GetMarkerWidth()/2;
	m_MarkerAreaRect.right	-= GetMarkerWidth()/2;

	m_MarkerAreaRect.top = m_MarkerAreaRect.Height()*kScaleMarkRatio + m_MarkerAreaRect.Height()*kTrackRatio;
	m_MarkerAreaRect.bottom = m_MarkerAreaRect.top + m_MarkerAreaRect.Height()*fMarkerRatio;
}

int CProgressSliderEx::GetThumbHeight() const
{
	CRect trackRect = GetTrackAreaRect();
	return trackRect.Height() * 1.5f;
}

float CProgressSliderEx::GetThumbPositionX()
{
	CRect rect = GetTrackAreaRect();
	float fProgress = float(GetPos()) / NUM_INCREMENTS;
	float fPositionX = fProgress * rect.Width() + rect.left;
	return fPositionX;
}

float CProgressSliderEx::SetThumbPositionX(float fClientPosX)
{
	CRect rect = GetTrackAreaRect();
	float fRelativeProgress = float(fClientPosX - rect.left) / rect.Width();
	float fRelativePos = fRelativeProgress * NUM_INCREMENTS;
	SetPos(int(fRelativePos));
	Invalidate();
	return GetThumbAbsolutePos();
}

void CProgressSliderEx::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_nIndexForDisplayTooltip = -1;

	if( IsInTrack(point) )
	{
		m_nFirstSelectedIndex = -1;
		m_SelectedRgnIndices.clear();
		__super::OnLButtonDown( nFlags, point );
		return;
	}

	m_State = eMouseState_None;

	int nIndex = HitTest(point);

	if( nIndex != -1 )
	{
		if( m_SelectedRgnIndices.find(nIndex) == m_SelectedRgnIndices.end() )
		{
			m_SelectedRgnIndices.clear();
			m_SelectedRgnIndices.insert(nIndex);
		}

		m_nFirstSelectedIndex = nIndex;
		m_State = eMouseState_PickMarker;
		GetCursorPos(&m_PointWhenLButtonDown);
		m_PrevPoint = point;
		__super::OnLButtonDown( nFlags, CPoint(GetMarkersCenterX(m_nFirstSelectedIndex),point.y) );
	}
	else
	{
		m_SelectedRgnIndices.clear();
		m_StartPoint = point;
		m_EndPoint = point;
		m_State = eMouseState_DrawRectangle;
		SetCapture();
	}

	Invalidate();
}

void CProgressSliderEx::OnMouseMove(UINT nFlags, CPoint point)
{
	CPoint cursorPos;
	GetCursorPos(&cursorPos);

	if( m_State == eMouseState_PickMarker )
	{
		m_State = eMouseState_Draging;
	}
	if( m_State == eMouseState_Draging )
	{
		std::set<int>::iterator ii = m_SelectedRgnIndices.begin();
		int offset = point.x-m_PrevPoint.x;

		CRect markerRect(GetSelectedMarkersRect());
		CRect offsetedMarkerRect(markerRect);
		offsetedMarkerRect.left += offset;
		offsetedMarkerRect.right += offset;

		int halfMarkerWidth = GetMarkerWidth()/2;

		CRect trackRect = GetTrackAreaRect();

		if( offset < 0 && offsetedMarkerRect.left-halfMarkerWidth < trackRect.left )
			offset = trackRect.left - markerRect.left - halfMarkerWidth;

		if( offset > 0  && offsetedMarkerRect.right+halfMarkerWidth > trackRect.right )
			offset = trackRect.right - markerRect.right + halfMarkerWidth;

		if( offset != 0 )
		{
			CRect trackRect = GetTrackAreaRect();
			for( ; ii != m_SelectedRgnIndices.end(); ++ii )
			{
				m_MarkerRgnList[*ii].m_fAbsolutePos += ((float)offset/(float)trackRect.Width())*(m_ZoomRangeMax-m_ZoomRangeMin);
				m_MarkerRgnList[*ii].m_pRgn->OffsetRgn(offset,0);
			}
		}

		m_PrevPoint = point;

		__super::OnMouseMove( nFlags, CPoint(GetMarkersCenterX(m_nFirstSelectedIndex), point.y) );

		Invalidate();
	}
	else if( m_State == eMouseState_None )
	{
		if( nFlags & MK_LBUTTON )
		{
			m_nFirstSelectedIndex = -1;
			__super::OnMouseMove( nFlags, point );
			Invalidate();
			return;
		}

		int nIndex = HitTest(point);
		if( nIndex != m_nIndexForDisplayTooltip )
		{
			m_nIndexForDisplayTooltip = nIndex;
			if( nIndex != -1 )
				m_pDoc->AcquireJob(new AnimEventQueryInfoJob(m_MarkerRgnList[nIndex].m_EventID,&m_TooltipContents),true);
			Invalidate();
		}
	}
	else if( m_State == eMouseState_DrawRectangle )
	{
		m_EndPoint = point;

		CRect rect(m_StartPoint,m_EndPoint);

		if( rect.left > rect.right )
			std::swap(rect.left,rect.right);

		if( rect.top > rect.bottom )
			std::swap(rect.top,rect.bottom);

		m_SelectedRgnIndices.clear();

		for( int i = 0, iSize(m_MarkerRgnList.size()); i < iSize; ++i )
		{
			if( m_MarkerRgnList[i].m_pRgn->RectInRegion(&rect) )
				m_SelectedRgnIndices.insert(i);
		}

		Invalidate();
	}
}

void CProgressSliderEx::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bPropagatingSelection = true;

	std::map<int,float> eventSet;
	if( !m_SelectedRgnIndices.empty() )
	{
		CRect trackRect = GetTrackAreaRect();
		std::set<int>::iterator ii = m_SelectedRgnIndices.begin();
		for( ; ii != m_SelectedRgnIndices.end(); ++ii )
			eventSet[m_MarkerRgnList[*ii].m_EventID] = m_ZoomRangeMin+(m_ZoomRangeMax-m_ZoomRangeMin)*float((GetMarkersCenterX(*ii)-trackRect.left)+0.5f)/float(trackRect.Width());
	}

	if( m_State == eMouseState_PickMarker || m_State == eMouseState_Draging )
	{
		if( m_State == eMouseState_Draging && !m_SelectedRgnIndices.empty() )
		{
			if( m_nFirstSelectedIndex != -1 )
				m_MarkerRgnList[m_nFirstSelectedIndex].m_fAbsolutePos = (float)GetThumbAbsolutePos()/(float)NUM_INCREMENTS;
			m_pListener->OnProgressSliderExEvent( IProgressSliderExListener::eEvent_TimeUpdate, &eventSet );
		}
	}

	if( m_pListener )
	{
		if( m_SelectedRgnIndices.empty() )
			m_pListener->OnProgressSliderExEvent( IProgressSliderExListener::eEvent_ClearAllSelections, NULL );
		else
			m_pListener->OnProgressSliderExEvent( IProgressSliderExListener::eEvent_Select, &eventSet );
	}

	m_State = eMouseState_None;

	if( m_nFirstSelectedIndex != -1 )
		__super::OnLButtonUp( nFlags, CPoint(GetMarkersCenterX(m_nFirstSelectedIndex), point.y) );
	else
		__super::OnLButtonUp( nFlags, point );

	Invalidate();
	m_bPropagatingSelection = false;
}

CRect CProgressSliderEx::GetMarkersRect( int nIndex ) const
{
	if( nIndex == -1 )
		return CRect(0,0,0,0);
	CRect rect;
	if( ERROR == m_MarkerRgnList[nIndex].m_pRgn->GetRgnBox(&rect) )
		return CRect(0,0,0,0);
	return rect;
}

CRect CProgressSliderEx::GetSelectedMarkersRect() const
{
	std::set<int>::iterator ii = m_SelectedRgnIndices.begin();

	CRect wholeRect(0,0,0,0);
	for( ; ii != m_SelectedRgnIndices.end(); ++ii )
	{
		CRect rect = GetMarkersRect(*ii);
		if( wholeRect.Width() == 0 && wholeRect.Height() == 0 )
			wholeRect = rect;
		else
			wholeRect.UnionRect( &wholeRect, &rect );
	}

	return wholeRect;
}

int CProgressSliderEx::GetMarkersCenterX( int nIndex ) const
{
	CRect markRect = GetMarkersRect(nIndex);
	return (float(markRect.left+markRect.right)*0.5f)+0.5f;
}

void CProgressSliderEx::RemoveAllRgnList()
{
	m_MarkerRgnList.clear();
}

int CProgressSliderEx::HitTest( const CPoint& point )
{
	for( int i = 0, iSize(m_MarkerRgnList.size()); i < iSize; ++i )
	{
		if( m_MarkerRgnList[i].m_pRgn->PtInRegion(point) )
			return i;
	}
	return -1;
}

void CProgressSliderEx::SetMarkerListWithIDs(DynArray<f32> markers, const std::vector<int>& idList)
{
	assert( markers.size() == idList.size() );
	if( markers.size() != idList.size() || m_bPropagatingSelection )
		return;

	__super::SetMarkerList(markers);

	int nSize = m_markers.size();

	RemoveAllRgnList();
	m_MarkerRgnList.resize(nSize);

	int nThumbWidth = GetThumbWidth();

	for( int i = 0; i < nSize; ++i )
	{
		m_MarkerRgnList[i].m_pRgn = CreateRgn(m_markers[i]);
		m_MarkerRgnList[i].m_fAbsolutePos = m_markers[i];
		m_MarkerRgnList[i].m_EventID = idList[i];
	}
}

CRgn* CProgressSliderEx::CreateRgn( float fPos ) const
{
	CRect markerRect = GetMarkerAreaRect();
	CRect trackRect = GetTrackAreaRect();

	float kY = markerRect.top - trackRect.Height()*0.4f;
	float kMarkerHeight = markerRect.Height();
	int halfMarkWidth = GetMarkerWidth() * 0.5f;
	float fPositionX = GetRelativePositionX(fPos);
	CPoint vPivot(fPositionX-1, kY);
	CPoint vPoints[4] = { vPivot, vPivot+CPoint(-halfMarkWidth,kMarkerHeight-1), vPivot+CPoint(halfMarkWidth,kMarkerHeight-1), vPivot };

	CRgn* pRgn = new CRgn;
	pRgn->CreatePolygonRgn(vPoints, 4, WINDING);

	return pRgn;
}

void CProgressSliderEx::ClearSelectedMarkers()
{
	if( m_bPropagatingSelection )
		return;
	m_SelectedRgnIndices.clear();
	Invalidate();
}

void CProgressSliderEx::SelectMarkers( std::set<int>& idList )
{
	if( m_bPropagatingSelection )
		return;
	for( int i = 0, iSize(m_MarkerRgnList.size()); i < iSize; ++i )
	{
		if( idList.find(m_MarkerRgnList[i].m_EventID) != idList.end() )
			m_SelectedRgnIndices.insert(i);
	}
}

void CProgressSliderEx::UpdateAllMarkers()
{
	for( int i = 0, iSize(m_MarkerRgnList.size()); i < iSize; ++i )
	{
		SMarkerInfo& markerInfo = m_MarkerRgnList[i];
		int nRelativePos = GetRelativePositionX(markerInfo.m_fAbsolutePos);
		if( markerInfo.m_pRgn )
			delete markerInfo.m_pRgn;
		markerInfo.m_pRgn = CreateRgn(markerInfo.m_fAbsolutePos);
	}
}

BOOL CProgressSliderEx::OnMouseWheel( UINT nFlags, short zDelta, CPoint pt )
{
	float width = m_ZoomRangeMax - m_ZoomRangeMin;

	if( zDelta > 0 )
	{
		width *= 0.8f;
		if( width < 0.1 )
			width = 0.1f;
	}
	else
	{
		width *= 1.25f;
		if( width > 1.0f )
			width = 1;
	}

	int nPrevThumbAbsloutePos = GetThumbAbsolutePos();

	float fThumbPos = (float)GetThumbAbsolutePos()/(float)NUM_INCREMENTS;
	m_ZoomRangeMin = fThumbPos - width*0.5f;
	m_ZoomRangeMax = fThumbPos + width*0.5f;

	if( m_ZoomRangeMin < 0 )
	{
		m_ZoomRangeMax -= m_ZoomRangeMin;
		m_ZoomRangeMin = 0;
	}

	if( m_ZoomRangeMax > 1 )
	{
		m_ZoomRangeMin -= (m_ZoomRangeMax - 1);
		m_ZoomRangeMax = 1;
	}

	SetThumbPosFromAbsolutePos(nPrevThumbAbsloutePos);
	UpdateAllMarkers();
	Invalidate();

	return TRUE;
}

void CProgressSliderEx::ResetZoom()
{
	int nPrevThumbAbsloutePos = GetThumbAbsolutePos();

	m_ZoomRangeMax = 1.0f;
	m_ZoomRangeMin = 0.0f;

	SetThumbPosFromAbsolutePos(nPrevThumbAbsloutePos);
	UpdateAllMarkers();
	Invalidate();
}