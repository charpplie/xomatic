////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   trackviewgraph.cpp
//  Version:     v1.00
//  Created:     23/8/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TrackViewGraph.h"
#include "..\Controls\MemDC.h"

#include "TrackViewDialog.h"
#include "..\AnimationContext.h"

//#include "IMovieSystem.h"

#define IDC_TRACKVIEWGRAPH_CURVE 1
#define IDC_TIMELINE             2

//////////////////////////////////////////////////////////////////////////
CTrackViewGraph::CTrackViewGraph()
{
	m_bIgnoreSelfEvents = false;
	GetIEditor()->RegisterNotifyListener( this );
}

//////////////////////////////////////////////////////////////////////////
CTrackViewGraph::~CTrackViewGraph()
{
	GetIEditor()->UnregisterNotifyListener( this );
}


BEGIN_MESSAGE_MAP(CTrackViewGraph, CToolbarDialog)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_NOTIFY( SPLN_CHANGE,IDC_TRACKVIEWGRAPH_CURVE,OnSplineChange )

	ON_COMMAND_RANGE( ID_TANGENT_UNIFY,ID_SPLINE_SNAP_GRID_Y,OnSplineCmd )
	ON_UPDATE_COMMAND_UI_RANGE( ID_TANGENT_UNIFY,ID_SPLINE_SNAP_GRID_Y,OnSplineCmdUpdateUI )
	ON_NOTIFY( TLN_CHANGE,IDC_TIMELINE,OnTimelineChange )
END_MESSAGE_MAP()

// CTrackViewGraph message handlers

//////////////////////////////////////////////////////////////////////////
void CTrackViewGraph::SetSequence( IAnimSequence *pSequence )
{
	m_pSequence = pSequence;
	AddSpline();
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewGraph::OnInitDialog()
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

	return bResult;
}

void CTrackViewGraph::OnSize( UINT nType, int cx, int cy )
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

		CRect rctb = rcClient;
		rctb.bottom = rctb.top + sz.cy;
		m_wndToolBar.MoveWindow(rctb);

		rcClient.top = rctb.bottom + 1;

		m_wndSpline.MoveWindow( rcClient );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewGraph::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if (m_bIgnoreSelfEvents)
		return;
	switch (event)
	{
	case eNotify_OnBeginSceneOpen:
		m_wndSpline.RemoveAllSplines();
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
void CTrackViewGraph::AddSpline()
{
	if (!m_pSequence)
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
void CTrackViewGraph::OnSplineChange( NMHDR *pNMHDR, LRESULT *pResult )
{
	m_bIgnoreSelfEvents = true;
	GetIEditor()->Notify( eNotify_OnUpdateTrackViewKeys );
	GetIEditor()->Notify( eNotify_OnUpdateTVKeySelection );
	m_bIgnoreSelfEvents = false;
	
	// In the end, focus this again in order to properly catch 'KeyDown' messages.
	m_wndSpline.SetFocus();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewGraph::OnSplineCmd( UINT cmd )
{
	m_wndSpline.OnUserCommand( cmd );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewGraph::OnSplineCmdUpdateUI( CCmdUI *pCmdUI )
{
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
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewGraph::SetCurrTime( float time )
{
	if (m_wndSpline.m_hWnd)
		m_wndSpline.SetTimeMarker( time );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewGraph::OnTimelineChange( NMHDR *pNMHDR, LRESULT *pResult )
{
	float fTime = m_timelineCtrl.GetTimeMarker();
	GetIEditor()->GetAnimation()->SetTime( fTime );
}
