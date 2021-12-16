////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   trackviewcurveeditor.cpp
//  Version:     v1.00
//  Created:     23/8/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TrackViewCurveEditor.h"
#include "Controls\MemDC.h"

#include "TrackViewDialog.h"
#include "AnimationContext.h"

//#include "IMovieSystem.h"

#define IDC_TRACKVIEWGRAPH_CURVE 1
#define IDC_TIMELINE             2

#define IDC_HORIZON_SLIDER		3
#define IDC_VERTICAL_SLIDER		4

//! It's for mapping from a slider control range to a real zoom range, and vice versa.
#define SLIDER_MULTIPLIER		100.f
#define SLIDERRANGE_TO_ZOOM(SLIDERVALUE)	(float)SLIDERVALUE/SLIDER_MULTIPLIER
#define ZOOMRANGE_TO_SLIDER(ZOOMVALUE)		(int)(ZOOMVALUE*SLIDER_MULTIPLIER)

//////////////////////////////////////////////////////////////////////////
CTrackViewCurveEditor::CTrackViewCurveEditor()
{
	m_bLevelClosing = false;
	m_bIgnoreSelfEvents = false;
	GetIEditor()->RegisterNotifyListener( this );
}

//////////////////////////////////////////////////////////////////////////
CTrackViewCurveEditor::~CTrackViewCurveEditor()
{
	GetIEditor()->UnregisterNotifyListener( this );
}


BEGIN_MESSAGE_MAP(CTrackViewCurveEditor, CToolbarDialog)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_NOTIFY( SPLN_CHANGE,IDC_TRACKVIEWGRAPH_CURVE,OnSplineChange )

	ON_COMMAND_RANGE( ID_TANGENT_UNIFY,ID_FREEZE_TANGENTS,OnSplineCmd )
	ON_UPDATE_COMMAND_UI_RANGE( ID_TANGENT_UNIFY,ID_FREEZE_TANGENTS,OnSplineCmdUpdateUI )
	ON_NOTIFY( TLN_CHANGE,IDC_TIMELINE,OnTimelineChange )
	ON_NOTIFY( NM_RELEASEDCAPTURE,IDC_HORIZON_SLIDER, OnHorizonSliderChange )
	ON_NOTIFY( NM_RELEASEDCAPTURE,IDC_VERTICAL_SLIDER, OnVerticalSliderChange )
	ON_NOTIFY( SPLN_SCROLL_ZOOM,IDC_TRACKVIEWGRAPH_CURVE,OnSplineScrollZoom )
END_MESSAGE_MAP()

// CTrackViewGraph message handlers

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::SetSequence( IAnimSequence *pSequence )
{
	m_pSequence = pSequence;
	m_wndSpline.SetSequence(pSequence);
	AddSpline();
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewCurveEditor::OnInitDialog()
{
	BOOL bResult = __super::OnInitDialog();

	//IDR_SPLINE_EDIT_BAR

	VERIFY(m_wndToolBar.CreateToolBar(WS_VISIBLE|WS_CHILD|CBRS_TOOLTIPS|CBRS_GRIPPER, this, AFX_IDW_TOOLBAR));
	VERIFY(m_wndToolBar.LoadToolBar(IDR_SPLINE_EDIT_BAR));
	m_wndToolBar.SetFlags(xtpFlagAlignTop|xtpFlagStretched);
	m_wndToolBar.EnableCustomization(FALSE);

	m_timelineCtrl.Create( WS_CHILD|WS_VISIBLE,CRect(0,0,1,1),this,IDC_TIMELINE );
	m_timelineCtrl.SetTimeRange( Range(0,1) );
	m_timelineCtrl.SetTicksTextScale( 1.0f );

	CRect rc;
	GetClientRect(rc);
	m_wndSpline.Create( WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,rc,this,IDC_TRACKVIEWGRAPH_CURVE );

	m_wndSpline.SetTimelineCtrl(&m_timelineCtrl);
	m_timelineCtrl.SetOwner(this);

	m_horizonSlider.Create(WS_CHILD|WS_VISIBLE|TBS_BOTTOM,CRect(0,0,1,1),this, IDC_HORIZON_SLIDER);
	m_verticlSlider.Create(WS_CHILD|WS_VISIBLE|TBS_VERT|TBS_RIGHT,CRect(0,0,1,1),this, IDC_VERTICAL_SLIDER);
	ResetSliderRange();

	return bResult;
}

void CTrackViewCurveEditor::OnSize( UINT nType, int cx, int cy )
{
	__super::OnSize(nType, cx, cy);

	if (m_wndSpline.m_hWnd)
	{
		CRect rc;
		GetClientRect(rc);
		m_wndSpline.MoveWindow(rc);

		CRect rcClient;
		GetClientRect(rcClient);

		DWORD dwMode = LM_HORZ|LM_HORZDOCK|LM_STRETCH|LM_COMMIT;
		CSize sz = m_wndToolBar.CalcDockingLayout(32000, dwMode);

		LONG horizonSliderWidth = 200;
		CRect rctb = rcClient;
		rctb.bottom = rctb.top + sz.cy;
		rctb.right-=horizonSliderWidth;

		m_wndToolBar.MoveWindow(rctb);

		rcClient.top = rctb.bottom + 1;

		LONG vertSliderWidth = 30;
		rcClient.right-=vertSliderWidth;

		m_wndSpline.MoveWindow( rcClient );

		rctb.left= rctb.right;
		rctb.right = rctb.left+horizonSliderWidth;
		m_horizonSlider.MoveWindow(rctb);

		rcClient.left=rcClient.right;
		rcClient.right = rcClient.left + vertSliderWidth;
		m_verticlSlider.MoveWindow(rcClient);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if (m_bIgnoreSelfEvents)
		return;
	switch (event)
	{
	case eNotify_OnCloseScene:
		m_wndSpline.RemoveAllSplines();
		m_bLevelClosing = true;
		break;
	case eNotify_OnBeginNewScene:
	case eNotify_OnBeginSceneOpen:
		m_bLevelClosing = false;
		break;
	case eNotify_OnUpdateTrackViewKeys:
		m_wndSpline.Invalidate();
		break;
	case eNotify_OnUpdateTVKeySelection:
		if (IsWindowVisible())
			AddSpline();
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::AddSpline()
{
	if (!m_pSequence)
		return;

	if (m_bLevelClosing)
		return;

	CTrackViewUtils::SelectedTracks selectedTracks;
	CTrackViewUtils::GetSelectedTracks( m_pSequence,selectedTracks );

	m_wndSpline.RemoveAllSplines();

	if (selectedTracks.bAllOfSameType)
	{
		for (int i = 0; i < selectedTracks.tracks.size(); i++)
		{
			IAnimTrack *pTrack = selectedTracks.tracks[i].pTrack;
			if (!pTrack->GetSpline())
				continue;

			if (selectedTracks.tracks[i].m_nSubTrackIndex >= 0)
			{
				COLORREF trackColor = RGB(255,0,0);
				switch (selectedTracks.tracks[i].m_nSubTrackIndex)
				{
				case 0:
					trackColor = RGB(255,0,0);
					break;
				case 1:
					trackColor = RGB(0,255,0);
					break;
				case 2:
					trackColor = RGB(0,0,255);
					break;
				case 3:
					trackColor = RGB(255,255,0);
					break;
				}
				m_wndSpline.AddSpline( pTrack->GetSpline(),pTrack,trackColor );
			}
			else
			{
				COLORREF	afColorArray[4];
				afColorArray[0]=RGB(255,0,0);
				afColorArray[1]=RGB(0,255,0);
				afColorArray[2]=RGB(0,0,255);
				afColorArray[3]=RGB(255,0,255); //Pink... so you know it's wrong if you see it.
				m_wndSpline.AddSpline( pTrack->GetSpline(),pTrack,afColorArray );
			}
		}
	}

	Range timeRange =	m_pSequence->GetTimeRange();
	m_wndSpline.SetTimeRange( timeRange );
	m_timelineCtrl.SetTimeRange( timeRange );
	m_wndSpline.SetValueRange( Range(-2000.0f,2000.0f) );
	// If it is a rotation track, adjust the default value range properly to accommodate some degree values.
	if(selectedTracks.bHasRotationTrack)
		m_wndSpline.SetDefaultValueRange(Range(-180.0f, 180.0f));
	else
		m_wndSpline.SetDefaultValueRange(Range(-1.1f, 1.1f));

	m_wndSpline.FitSplineToViewHeight();
	m_wndSpline.FitSplineToViewWidth();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::OnSplineChange( NMHDR *pNMHDR, LRESULT *pResult )
{
	m_bIgnoreSelfEvents = true;
	GetIEditor()->Notify( eNotify_OnUpdateTrackViewKeys );
	GetIEditor()->Notify( eNotify_OnUpdateTVKeySelection );
	m_bIgnoreSelfEvents = false;
	
	// In the end, focus this again in order to properly catch 'KeyDown' messages.
	m_wndSpline.SetFocus();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::OnSplineCmd( UINT cmd )
{
	m_wndSpline.OnUserCommand( cmd );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::OnSplineCmdUpdateUI( CCmdUI *pCmdUI )
{
	if (m_bLevelClosing)
		return;

	switch (pCmdUI->m_nID)
	{
	case ID_SPLINE_SNAP_GRID_X:
		pCmdUI->SetCheck( (m_wndSpline.IsSnapTime()) ? TRUE : FALSE );
		break;
	case ID_SPLINE_SNAP_GRID_Y:
		pCmdUI->SetCheck( (m_wndSpline.IsSnapValue()) ? TRUE : FALSE );
		break;
	case ID_TANGENT_UNIFY:
		pCmdUI->SetCheck( (m_wndSpline.IsUnifiedKeyCurrentlySelected()) ? TRUE : FALSE );
		break;
	case ID_FREEZE_KEYS:
		pCmdUI->SetCheck( (m_wndSpline.IsKeysFrozen()) ? TRUE : FALSE );
		break;
	case ID_FREEZE_TANGENTS:
		pCmdUI->SetCheck( (m_wndSpline.IsTangentsFrozen()) ? TRUE : FALSE );
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::SetCurrTime( float time )
{
	if (m_wndSpline.m_hWnd)
		m_wndSpline.SetTimeMarker( time );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::SetEditLock(bool bLock)
{
	if(m_wndSpline.m_hWnd)
		m_wndSpline.SetEditLock(bLock);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::OnTimelineChange( NMHDR *pNMHDR, LRESULT *pResult )
{
	float fTime = m_timelineCtrl.GetTimeMarker();
	GetIEditor()->GetAnimation()->SetTime( fTime );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::OnHorizonSliderChange( NMHDR *pNMHDR, LRESULT *pResult )
{
	if(m_wndSpline.m_hWnd)
	{
		int pos = m_horizonSlider.GetPos();
		Vec2 zoom = m_wndSpline.GetZoom();

		// Zero value is not acceptable.
		zoom.x = max( SLIDERRANGE_TO_ZOOM(pos), 1.f/SLIDER_MULTIPLIER );
		m_wndSpline.SetZoom(zoom);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::OnVerticalSliderChange( NMHDR *pNMHDR, LRESULT *pResult )
{
	if(m_wndSpline.m_hWnd)
	{
		int pos = m_verticlSlider.GetPos();
		Vec2 zoom = m_wndSpline.GetZoom();

		// Zero value is not acceptable.
		zoom.y = max( SLIDERRANGE_TO_ZOOM(pos), 1.f/SLIDER_MULTIPLIER);
		m_wndSpline.SetZoom(zoom);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::OnSplineScrollZoom( NMHDR *pNMHDR, LRESULT *pResult )
{
	ResetSliderRange();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::ResetSliderRange()
{
	if(!m_wndSpline.m_hWnd)
		return;

	Vec2 zoom = m_wndSpline.GetZoom();
	Vec2 minValue = zoom / 2.f;
	Vec2 maxValue = zoom * 2.f;

	m_horizonSlider.SetRange(ZOOMRANGE_TO_SLIDER(minValue.x),ZOOMRANGE_TO_SLIDER(maxValue.x));
	m_horizonSlider.SetPos(ZOOMRANGE_TO_SLIDER( (minValue.x + maxValue.x)/2.f ) );

	m_verticlSlider.SetRange(ZOOMRANGE_TO_SLIDER(minValue.y),ZOOMRANGE_TO_SLIDER(maxValue.y));
	m_verticlSlider.SetPos(ZOOMRANGE_TO_SLIDER( (minValue.y + maxValue.y)/2.f ) );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::SetFPS(float fps)
{
	m_timelineCtrl.SetFPS(fps);
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewCurveEditor::GetFPS() const
{
	return m_timelineCtrl.GetFPS();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewCurveEditor::SetTickDisplayMode(ETVTickMode mode)
{
	if(mode == TVTICK_INFRAMES)
	{
		m_timelineCtrl.SetMarkerStyle(CTimelineCtrl::MARKER_STYLE_FRAMES);
		m_wndSpline.SetTooltipValueScale(GetFPS(), 1.0f);
	}
	else if(mode == TVTICK_INSECONDS)
	{
		m_timelineCtrl.SetMarkerStyle(CTimelineCtrl::MARKER_STYLE_SECONDS);
		m_wndSpline.SetTooltipValueScale(1.0f, 1.0f);
	}

	m_timelineCtrl.Invalidate();
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewCurveEditor::PreTranslateMessage(MSG* pMsg)
{
	CTrackViewDialog *pTrackViewDialog = CTrackViewDialog::GetCurrentInstance();
	assert(pTrackViewDialog);
	if (pTrackViewDialog)
	{
		if (pTrackViewDialog->GetCommandBars()->GetShortcutManager()->TranslateAccelerator(pMsg))
			return TRUE;
	}

	return __super::PreTranslateMessage(pMsg);
}