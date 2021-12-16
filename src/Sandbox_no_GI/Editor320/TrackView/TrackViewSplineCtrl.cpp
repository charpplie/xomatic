////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   TrackViewSplineCtrl.cpp
//  Version:     v1.00
//  Created:     30/7/2009 by Jaewon.
//  Compilers:   Visual Studio 2008
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TrackViewSplineCtrl.h"

bool CTrackViewSplineCtrl::GetTangentHandlePts(CPoint& inTangentPt, CPoint& pt, CPoint& outTangentPt, 
																								int nSpline, int nKey, int nDimension)
{
	ISplineInterpolator* pSpline = m_splines[nSpline].pSpline;
	IAnimTrack *pTrack = m_tracks[nSpline];

	float time = pSpline->GetKeyTime(nKey);

	ISplineInterpolator::ValueType value, tin, tout;
	ISplineInterpolator::ZeroValue(value);
	pSpline->GetKeyValue(nKey, value);
	pSpline->GetKeyTangents(nKey, tin, tout);

	if(pTrack->GetType() == ATRACK_TCB_FLOAT)
	{
		ITcbKey key;
		pTrack->GetKey(nKey, &key);

		Vec2 va, vb, vc;
		va.x = time - 1.0f;
		va.y = value[nDimension] - tin[nDimension];
		vb.x = time;
		vb.y = value[nDimension];
		vc.x = time + 1.0f;
		vc.y = value[nDimension] + tout[nDimension];
		inTangentPt = WorldToClient(va);
		pt = WorldToClient(vb);
		outTangentPt = WorldToClient(vc);

		Vec2 tinv, toutv;
		float maxLength = float(outTangentPt.x - pt.x);
		tinv.x = float(inTangentPt.x - pt.x);
		tinv.y = float(inTangentPt.y - pt.y);
		toutv.x = float(outTangentPt.x - pt.x);
		toutv.y = float(outTangentPt.y - pt.y);
		tinv.Normalize();
		toutv.Normalize();
		tinv *= maxLength / (2-key.easeto);
		toutv *= maxLength / (2-key.easefrom);
		
		inTangentPt = pt + CPoint(int(tinv.x), int(tinv.y));
		outTangentPt = pt + CPoint(int(toutv.x), int(toutv.y));
	}
	else
	{
		assert(pTrack->GetType() == ATRACK_BEZIER_FLOAT);
		assert(nDimension == 0);

		I2DBezierKey key;
		pTrack->GetKey(nKey, &key);

		Vec2 va, vb, vc;
		va.x = time - tin[0];
		va.y = value[0] - tin[1];
		vb.x = time;
		vb.y = value[0];
		vc.x = time + tout[0];
		vc.y = value[0] + tout[1];
		inTangentPt = WorldToClient(va);
		pt = WorldToClient(vb);
		outTangentPt = WorldToClient(vc);
	}

	return true;
}

void CTrackViewSplineCtrl::ComputeIncomingTangentAndEaseTo( float& ds, float& easeTo, CPoint inTangentPt, 
																													 int nSpline, int nKey, int nDimension )
{
	ISplineInterpolator* pSpline = m_splines[nSpline].pSpline;
	IAnimTrack *pTrack = m_tracks[nSpline];

	float time = pSpline->GetKeyTime(nKey);

	ISplineInterpolator::ValueType value, tin, tout;
	ISplineInterpolator::ZeroValue(value);
	pSpline->GetKeyValue(nKey, value);
	pSpline->GetKeyTangents(nKey, tin, tout);

	ITcbKey key;
	pTrack->GetKey(nKey, &key);

	// Get the control point.
	Vec2 tinv, vb;
	vb.x = time;
	vb.y = value[nDimension];
	CPoint pt = WorldToClient(vb);
	
	// Get the max length to comute the 'ease' value.
	float maxLength = float(WorldToClient(Vec2(vb.x+1, vb.y)).x - pt.x);
	
	CPoint tmp = inTangentPt - pt;
	tinv.x = float(tmp.x);
	tinv.y = float(tmp.y);

	// Compute the 'easeTo'.
	easeTo = 2.0f - maxLength/tinv.GetLength();

	// Compute the 'ds'.
	Vec2 va = ClientToWorld(inTangentPt);
	if(time < va.x + 0.000001f)
	{
		if(value[nDimension] > va.y)
			ds = 1000000.0f;
		else
			ds = -1000000.0f;
	}
	else
		ds = (value[nDimension] - va.y)/(time - va.x);
}

void CTrackViewSplineCtrl::ComputeOutgoingTangentAndEaseFrom( float& dd, float& easeFrom, CPoint outTangentPt, 
																														 int nSpline, int nKey, int nDimension )
{
	ISplineInterpolator* pSpline = m_splines[nSpline].pSpline;
	IAnimTrack *pTrack = m_tracks[nSpline];

	float time = pSpline->GetKeyTime(nKey);

	ISplineInterpolator::ValueType value, tin, tout;
	ISplineInterpolator::ZeroValue(value);
	pSpline->GetKeyValue(nKey, value);
	pSpline->GetKeyTangents(nKey, tin, tout);

	ITcbKey key;
	pTrack->GetKey(nKey, &key);

	// Get the control point.
	Vec2 toutv, vb;
	vb.x = time;
	vb.y = value[nDimension];
	CPoint pt = WorldToClient(vb);
	
	// Get the max length to comute the 'ease' value.
	float maxLength = float(WorldToClient(Vec2(vb.x+1, vb.y)).x - pt.x);
	
	CPoint tmp = outTangentPt - pt;
	toutv.x = float(tmp.x);
	toutv.y = float(tmp.y);

	// Compute the 'easeFrom'.
	easeFrom = 2.0f - maxLength/toutv.GetLength();

	// Compute the 'dd'.
	Vec2 vc = ClientToWorld(outTangentPt);
	if(vc.x < time + 0.000001f)
	{
		if(value[nDimension] < vc.y)
			dd = 1000000.0f;
		else
			dd = -1000000.0f;
	}
	else
	dd = (vc.y - value[nDimension])/(vc.x - time);
}

void CTrackViewSplineCtrl::AddSpline( ISplineInterpolator* pSpline, IAnimTrack* pTrack, COLORREF color )
{
	COLORREF colorArray[4];
	colorArray[0] = colorArray[1] = colorArray[2] = colorArray[3] = color;
	AddSpline(pSpline,pTrack, colorArray);
}

void CTrackViewSplineCtrl::AddSpline( ISplineInterpolator* pSpline, IAnimTrack* pTrack, COLORREF anColorArray[4])
{
	for (int i = 0; i < (int)m_splines.size(); i++)
	{
		if (m_splines[i].pSpline == pSpline)
			return;
	}
	SSplineInfo si;

	for (int nCurrentDimension=0;nCurrentDimension<pSpline->GetNumDimensions();nCurrentDimension++)
	{
		si.anColorArray[nCurrentDimension] = anColorArray[nCurrentDimension];
	}

	si.pSpline = pSpline;
	si.pDetailSpline = NULL;
	m_splines.push_back(si);
	m_tracks.push_back(pTrack);
	m_bKeyTimesDirty = true;
	Invalidate();
}

void CTrackViewSplineCtrl::RemoveAllSplines()
{
	m_tracks.clear();
	CSplineCtrlEx::RemoveAllSplines();
}

void CTrackViewSplineCtrl::MoveSelectedTangentHandleTo( CPoint point )
{
	assert(m_pHitSpline && m_nHitKeyIndex >=0 && m_bHitIncomingHandle >=0);

	// Set the custom flag to the key.
	int nRemoveFlags, nAddFlags;
	if(m_bHitIncomingHandle)
	{
		nRemoveFlags = SPLINE_KEY_TANGENT_IN_MASK;
		nAddFlags = SPLINE_KEY_TANGENT_CUSTOM << SPLINE_KEY_TANGENT_IN_SHIFT;
	}
	else
	{
		nRemoveFlags = SPLINE_KEY_TANGENT_OUT_MASK;
		nAddFlags = SPLINE_KEY_TANGENT_CUSTOM << SPLINE_KEY_TANGENT_OUT_SHIFT;
	}
	int flags = m_pHitSpline->GetKeyFlags(m_nHitKeyIndex);
	flags &= ~nRemoveFlags;
	flags |= nAddFlags;
	m_pHitSpline->SetKeyFlags(m_nHitKeyIndex, flags);

	// Adjust the incoming or outgoing tangent.
	int splineIndex;
	for (splineIndex = 0; splineIndex < m_splines.size(); ++splineIndex)
	{
		if(m_pHitSpline == m_splines[splineIndex].pSpline)
			break;
	}
	assert(splineIndex < m_splines.size());
	IAnimTrack* pTrack = m_tracks[splineIndex];
	if(pTrack->GetType() == ATRACK_TCB_FLOAT)
	{
		if(m_bHitIncomingHandle)
		{
			float ds, easeTo;
			ComputeIncomingTangentAndEaseTo(ds, easeTo, point, splineIndex, m_nHitKeyIndex, m_nHitDimension);
			// ease-to
			ITcbKey key;
			pTrack->GetKey(m_nHitKeyIndex, &key);
			key.easeto += easeTo;
			if(key.easeto > 1)
				key.easeto = 1.0f;
			else if(key.easeto < 0)
				key.easeto = 0;
			pTrack->SetKey(m_nHitKeyIndex, &key);
			// tin
			ISplineInterpolator::ValueType tin, tout;
			m_pHitSpline->GetKeyTangents(m_nHitKeyIndex, tin, tout);
			tin[m_nHitDimension] = ds;
			m_pHitSpline->SetKeyInTangent(m_nHitKeyIndex, tin);
		}
		else
		{
			float dd, easeFrom;
			ComputeOutgoingTangentAndEaseFrom(dd, easeFrom, point, splineIndex, m_nHitKeyIndex, m_nHitDimension);
			// ease-from
			ITcbKey key;
			pTrack->GetKey(m_nHitKeyIndex, &key);
			key.easefrom += easeFrom;
			if(key.easefrom > 1)
				key.easefrom = 1.0f;
			else if(key.easefrom < 0)
				key.easefrom = 0;
			pTrack->SetKey(m_nHitKeyIndex, &key);
			// tout
			ISplineInterpolator::ValueType tin, tout;
			m_pHitSpline->GetKeyTangents(m_nHitKeyIndex, tin, tout);
			tout[m_nHitDimension] = dd;
			m_pHitSpline->SetKeyOutTangent(m_nHitKeyIndex, tout);
		}
	}
	else
	{
		assert(pTrack->GetType() == ATRACK_BEZIER_FLOAT);
		assert(m_nHitDimension == 0);

		Vec2 tp = ClientToWorld(point);
		if(m_bHitIncomingHandle)
		{
			// tin
			ISplineInterpolator::ValueType value, tin, tout;
			float time = m_pHitSpline->GetKeyTime(m_nHitKeyIndex);
			m_pHitSpline->GetKeyValue(m_nHitKeyIndex, value);
			m_pHitSpline->GetKeyTangents(m_nHitKeyIndex, tin, tout);
			tin[0] = time - tp.x;
			// Constrain the time range so that the time curve is always monotonically increasing.
			if(tin[0] < 0)
				tin[0] = 0;
			else if(m_nHitKeyIndex > 0
				&& tin[0] > (time - m_pHitSpline->GetKeyTime(m_nHitKeyIndex-1)))
				tin[0] = time - m_pHitSpline->GetKeyTime(m_nHitKeyIndex-1);
			tin[1] = value[0] - tp.y;
			m_pHitSpline->SetKeyInTangent(m_nHitKeyIndex, tin);
		}
		else
		{
			// tout
			ISplineInterpolator::ValueType value, tin, tout;
			float time = m_pHitSpline->GetKeyTime(m_nHitKeyIndex);
			m_pHitSpline->GetKeyValue(m_nHitKeyIndex, value);
			m_pHitSpline->GetKeyTangents(m_nHitKeyIndex, tin, tout);
			tout[0] = tp.x - time;
			// Constrain the time range so that the time curve is always monotonically increasing.
			if(tout[0] < 0)
				tout[0] = 0;
			else if(m_nHitKeyIndex < m_pHitSpline->GetKeyCount() - 1
				&& tout[0] > (m_pHitSpline->GetKeyTime(m_nHitKeyIndex+1) - time))
				tout[0] = m_pHitSpline->GetKeyTime(m_nHitKeyIndex+1) - time;
			tout[1] = tp.y - value[0];
			m_pHitSpline->SetKeyOutTangent(m_nHitKeyIndex, tout);
		}
	}

	SendNotifyEvent( SPLN_CHANGE );
	Invalidate();
}

BEGIN_MESSAGE_MAP(CTrackViewSplineCtrl, CSplineCtrlEx)
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

void CTrackViewSplineCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	CPoint cMousePosPrev = m_cMousePos;
	m_cMousePos = point;

	if(GetCapture() != this)
	{
		//StopTracking();
	}

	if (m_editMode == SelectMode)
	{
		SetCursor(NULL);
		CRect rc( m_cMouseDownPos,point );
		rc.NormalizeRect();
		rc.IntersectRect(rc,m_rcSpline);

		CDC *dc = GetDC();
		dc->DrawDragRect( rc,CSize(1,1),m_rcSelect,CSize(1,1) );
		ReleaseDC(dc);
		m_rcSelect = rc;
	}

	if (m_editMode == TimeMarkerMode)
	{
		SetCursor(NULL);
		SetTimeMarker(XOfsToTime(point.x));
		SendNotifyEvent(SPLN_TIME_CHANGE);
	}

	if (m_boLeftMouseButtonDown)
	{
		if (m_editMode == TrackingMode && point != m_cMouseDownPos)
		{
			m_startedDragging = true;
			GetIEditor()->RestoreUndo();
			StoreUndo();

			bool bAltClick = CheckVirtualKey(VK_MENU);
			bool bShiftClick = CheckVirtualKey(VK_SHIFT);
			bool bSpaceClick = CheckVirtualKey(VK_SPACE);

			Vec2 v0 = ClientToWorld(m_cMouseDownPos);
			Vec2 v1 = ClientToWorld(point);
			if(m_hitCode == HIT_TANGENT_HANDLE)
			{
				MoveSelectedTangentHandleTo(point);
			}
			else if (bAltClick && bShiftClick)
			{
				ValueScaleKeys(v0.y, v1.y);
			}
			else if (bAltClick)
			{
				TimeScaleKeys(m_fTimeMarker, v0.x, v1.x);
			}
			else if (bShiftClick)
			{
				// Constrains the move to the vertical direction.
				MoveSelectedKeys(Vec2(0,v1.y - v0.y), false);
			}
			else if (bSpaceClick)
			{
				// Reset to the original position.
				MoveSelectedKeys(Vec2(0,0), false);
			}
			else
			{
				MoveSelectedKeys(v1 - v0, m_copyKeys);
			}		
		}
	}

	if (m_editMode == TrackingMode && GetNumSelected() == 1)
	{
		float time = 0;
		ISplineInterpolator::ValueType	afValue;
		CString							tipText;
		bool                            boFoundTheSelectedKey(false);

		for (int splineIndex = 0, endSpline = m_splines.size(); splineIndex < endSpline; ++splineIndex)
		{
			ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;
			IAnimTrack *pTrack = m_tracks[splineIndex];
			for (int i = 0; i < pSpline->GetKeyCount(); i++)
			{
				for (int nCurrentDimension=0;nCurrentDimension<pSpline->GetNumDimensions();nCurrentDimension++)
				{
					if (pSpline->IsKeySelectedAtDimension(i,nCurrentDimension))
					{
						time = pSpline->GetKeyTime(i);
						pSpline->GetKeyValue(i,afValue);
						if(pTrack->GetType() == ATRACK_TCB_FLOAT)
						{
							ITcbKey key;
							pTrack->GetKey(i, &key);
							tipText.Format( "t=%.3f  v=%2.3f / T=%.3f  C=%.3f  B=%.3f",
														time*m_fTooltipScaleX,afValue[nCurrentDimension]*m_fTooltipScaleY,
														key.tens, key.cont, key.bias );
						}
						else
						{
							assert(pTrack->GetType() == ATRACK_BEZIER_FLOAT);
							ISplineInterpolator::ValueType tin, tout;
							pSpline->GetKeyTangents(i, tin, tout);
							tipText.Format( "t=%.3f  v=%2.3f / tin=(%.3f,%2.3f)  tout=(%.3f,%2.3f)",
														time*m_fTooltipScaleX,afValue[0]*m_fTooltipScaleY,
														tin[0], tin[1], tout[0], tout[1] );
						}
						boFoundTheSelectedKey=true;
						break;
					}
				}
				if (boFoundTheSelectedKey)
				{
					break;
				}
			}
		}		

		if (point != m_lastToolTipPos)
		{
			m_lastToolTipPos = point;
			m_tooltipText = tipText;
			Invalidate();
			//m_tooltip.UpdateTipText( tipText,this,1 );
			//m_tooltip.Activate(TRUE);

		}
	}

	switch (m_editMode)
	{
	case ScrollMode:
		{
			// Set the new scrolled coordinates
			float ofsx = m_grid.origin.x - (point.x - m_cMouseDownPos.x)/m_grid.zoom.x;
			float ofsy = m_grid.origin.y + (point.y - m_cMouseDownPos.y)/m_grid.zoom.y;
			SetScrollOffset( Vec2(ofsx,ofsy) );
			m_cMouseDownPos = point;
		}
		break;

	case ZoomMode:
		{
			float ofsx = (point.x - m_cMouseDownPos.x) * 0.01f;
			float ofsy = (point.y - m_cMouseDownPos.y) * 0.01f;

			Vec2 z = m_grid.zoom;
			if (ofsx != 0)
				z.x = max(z.x * (1.0f + ofsx), 0.001f);
			if (ofsy != 0)
				z.y = max(z.y * (1.0f + ofsy), 0.001f);
			SetZoom( z,m_cMouseDownPos );
			m_cMouseDownPos = point;
		}
		break;
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CTrackViewSplineCtrl::AdjustTCB(float d_tension, float d_continuity, float d_bias)
{
	CUndo undo( "Modify Spline Keys TCB" );
	ConditionalStoreUndo();

	SendNotifyEvent( SPLN_BEFORE_CHANGE );

	for (int splineIndex = 0, splineCount = m_splines.size(); splineIndex < splineCount; ++splineIndex)
	{
		ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;
		IAnimTrack* pTrack = m_tracks[splineIndex];

		if(pTrack->GetType() != ATRACK_TCB_FLOAT)
			continue;

		for (int i = 0; i < (int)pSpline->GetKeyCount(); i++)
		{
			// If the key is selected in any dimension...
			for (
				int nCurrentDimension=0;
				nCurrentDimension<pSpline->GetNumDimensions();
			nCurrentDimension++
				)
			{
				if (IsKeySelected(pSpline, i,nCurrentDimension))
				{
					ITcbKey key;
					pTrack->GetKey(i, &key);
					// tension
					key.tens += d_tension;
					if(key.tens > 1)
						key.tens = 1.0f;
					else if(key.tens < -1)
						key.tens = -1.0f;
					// continuity
					key.cont += d_continuity;
					if(key.cont > 1)
						key.cont = 1.0f;
					else if(key.cont < -1)
						key.cont = -1.0f;
					// bias
					key.bias += d_bias;
					if(key.bias > 1)
						key.bias = 1.0f;
					else if(key.bias < -1)
						key.bias = -1.0f;
					pTrack->SetKey(i, &key);
					OnUserCommand(ID_TANGENT_AUTO);
					break;
				}
			}
		}
	}

	SendNotifyEvent( SPLN_CHANGE );
	Invalidate();
}

void CTrackViewSplineCtrl::OnUserCommand( UINT cmd )
{
	if(cmd == ID_TANGENT_UNIFY)
	{
		if(IsUnifiedKeyCurrentlySelected() == false)
			ModifySelectedKeysFlags(SPLINE_KEY_TANGENT_ALL_MASK, SPLINE_KEY_TANGENT_UNIFIED);
		else
			ModifySelectedKeysFlags(SPLINE_KEY_TANGENT_ALL_MASK, SPLINE_KEY_TANGENT_BROKEN);
	}
	else
		CSplineCtrlEx::OnUserCommand(cmd);
}

bool CTrackViewSplineCtrl::IsUnifiedKeyCurrentlySelected() const
{
	for (int splineIndex = 0, splineCount = m_splines.size(); splineIndex < splineCount; ++splineIndex)
	{
		ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

		for (int i = 0; i < (int)pSpline->GetKeyCount(); i++)
		{
			// If the key is selected in any dimension...
			for (
				int nCurrentDimension=0;
				nCurrentDimension<pSpline->GetNumDimensions();
			nCurrentDimension++
				)
			{
				if (IsKeySelected(pSpline, i,nCurrentDimension))
				{
					int flags = pSpline->GetKeyFlags(i);
					if((flags & SPLINE_KEY_TANGENT_ALL_MASK) != SPLINE_KEY_TANGENT_UNIFIED)
						return false;
				}
			}
		}
	}

	return true;
}
