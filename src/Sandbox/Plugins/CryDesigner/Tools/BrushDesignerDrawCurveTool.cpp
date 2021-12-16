#include "StdAfx.h"
#include "BrushDesignerDrawCurveTool.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesigner.h"
#include "ViewManager.h"
#include "IBaseToolPanel.h"

namespace 
{
	ICreateSphereDiscCurveToolPanel* g_pCurvePanel = NULL;
}

void CBrushDesignerDrawCurveTool::Leave()
{
	__super::Leave();
	m_ArcState = eArcState_ChooseFirstPoint;
}

void CBrushDesignerDrawCurveTool::BeginEditParams()
{
	if( !g_pCurvePanel )
		g_pCurvePanel = CreateCurvePanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerDrawCurveTool::EndEditParams()
{
	if( g_pCurvePanel )
	{
		g_pCurvePanel->DestroyPanel();
		g_pCurvePanel = NULL;
	}
}

bool CBrushDesignerDrawCurveTool::IsPhaseFirstStepOnPrimitiveCreation() const
{
	return GetSpotListCount() == 0 && m_ArcState == eArcState_ChooseFirstPoint;
}

void CBrushDesignerDrawCurveTool::OnLButtonUp( CViewport *view,UINT nFlags,CPoint point )
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	if( m_ArcState == eArcState_ChooseFirstPoint )
	{
		if( GetDesigner()->IsEmpty() && GetSpotListCount() == 0 )
		{
			SSpot spot = GetCurrentSpot();
			GetBaseObject()->SetWorldPos(GetWorldTM().TransformPoint(spot.m_Pos));
			spot.m_Pos = BrushVec3(0,0,0);
			spot.m_Plane.Set(spot.m_Plane.Normal(),0);
			SetPlane(spot.m_Plane);
			SetCurrentSpot(spot);
		}

		SetStartSpot(GetCurrentSpot());
		DESIGNER_ASSERT( GetStartSpot().m_PosState != eSpotPosState_Invalid );
		m_ArcState = eArcState_ChooseLastPoint;
		ResetCurrentSpot();
	}
	else if( m_ArcState == eArcState_ChooseLastPoint )
	{	
		m_LastSpot = GetCurrentSpot();
		ResetCurrentSpot();
		PrepareBeizerSpots( view, nFlags, point );
		m_ArcState = eArcState_ControlMiddlePoint;
	}
	else if( m_ArcState == eArcState_ControlMiddlePoint )
	{
		CUndo undo("Designer : Register Arc");
		GetDesigner()->RecordUndo("Designer:Arc",GetBaseObject());

		SpotList spotList;
		int iSpotListSize(GetSpotListCount());
		for( int i = 0; i < iSpotListSize-1; ++i )
		{
			const SSpot& spot0 = GetSpot(i);
			const SSpot& spot1 = GetSpot(i+1);
			std::vector<CBrushDesigner::IntersectionPair> intersections;
			GetDesigner()->QueryIntersectionByEdge( BrushEdge3D(spot0.m_Pos,spot1.m_Pos), intersections );
			spotList.push_back(spot0);
			for( int k = 0, iSize(intersections.size()); k < iSize; ++k )
			{
				if( intersections[k].second.IsEquivalent(spot0.m_Pos,kDesignerEpsilon) || intersections[k].second.IsEquivalent(spot1.m_Pos,kDesignerEpsilon) )
					continue;
				spotList.push_back(SSpot(intersections[k].second,eSpotPosState_Edge,intersections[k].first));
			}
		}

		if( iSpotListSize-1 >= 0 && iSpotListSize-1 < GetSpotList().size() )
			spotList.push_back(GetSpot(iSpotListSize-1));

		ReplaceSpotList(spotList);
		RegisterSpotListAfterBreaking();
		RegisterEitherEndSpotList();
		UpdateMirroredPartWithPlane( GetDesigner(), GetPlane() );
		ResetAllSpots();
		UpdateBrush();
		Sync();
		m_ArcState = eArcState_ChooseFirstPoint;
	}
}

void CBrushDesignerDrawCurveTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_ArcState == eArcState_ControlMiddlePoint )
	{
		if( nFlags & MK_SHIFT )
			PrepareBeizerSpots( view, nFlags, point );
		else
			PrepareArcSpots( view, nFlags, point );
	}
	else
	{
		bool bKeepInitialPlane = (m_ArcState!=eArcState_ChooseFirstPoint);
		if( CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, bKeepInitialPlane ) )
		{
			SetPlane(GetCurrentSpot().m_Plane);

			if( m_ArcState == eArcState_ChooseLastPoint )
			{
				if( nFlags&MK_SHIFT )
				{
					m_LineState = eLineState_Diagonal;
				}
				else
				{
					BrushVec3 outPos;
					m_LineState = GetAlienedPointWithAxis( GetStartSpotPos(), GetCurrentSpotPos(), GetPlane(), 5.0f, NULL, outPos );
					SetCurrentSpotPos(outPos);
				}
			}
		}
	}
}

void CBrushDesignerDrawCurveTool::PrepareArcSpots( CViewport *view,UINT nFlags,CPoint point )
{
	if( !CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, true ) )
		return;

	BrushVec3 crossPoint = GetCurrentSpotPos();
	BrushVec2 vCrossPointOnPlane(GetPlane().W2P(crossPoint));
	BrushVec2 vFirstPointOnPlane(GetPlane().W2P(GetStartSpotPos()));
	BrushVec2 vLastPointOnPlane(GetPlane().W2P(m_LastSpot.m_Pos));

	int nEdgeCount = g_pCurvePanel->GetSubdivisionNum();
	std::vector<BrushVec2> arcLastVertexList;
	if( MakeListConsistingOfArc( vCrossPointOnPlane, vFirstPointOnPlane, vLastPointOnPlane, nEdgeCount, arcLastVertexList ) && !arcLastVertexList.empty() )
	{
		ClearSpotList();
		AddSpotToSpotList(m_LastSpot);
		for( int i = 0; i < nEdgeCount-1; ++i )
			AddSpotToSpotList(SSpot(GetPlane().P2W(arcLastVertexList[i]),eSpotPosState_InRegion,GetPlane()));
		AddSpotToSpotList(GetStartSpot());
	}
}

void CBrushDesignerDrawCurveTool::PrepareBeizerSpots( CViewport *view,UINT nFlags,CPoint point )
{	
	if( !CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, true ) )
		return;

	BrushVec3 crossPoint = GetCurrentSpotPos();
	BrushVec2 firstPointOnPlane(GetPlane().W2P(GetStartSpotPos()));
	BrushVec2 lastPointOnPlane(GetPlane().W2P(m_LastSpot.m_Pos));

	BrushEdge edge(firstPointOnPlane,lastPointOnPlane);
	BrushLine line(edge.m_v[0],edge.m_v[1]);
	BrushFloat distance = line.Distance(BrushVec2(crossPoint.x,crossPoint.z));
	edge.m_v[0] += line.m_Normal*distance;
	edge.m_v[1] += line.m_Normal*distance;

	BrushFloat t(0.3f);
	BrushVec2 middlePointOnPlane0 = edge.m_v[0]*(1-t) + edge.m_v[1]*t;
	BrushVec2 middlePointOnPlane1 = edge.m_v[0]*t + edge.m_v[1]*(1-t);

	int nEdgeCount = g_pCurvePanel->GetSubdivisionNum();

	ClearSpotList();
	AddSpotToSpotList(GetStartSpot());
	for( int i = 1; i <= nEdgeCount-1; ++i )
	{
		BrushFloat t = (BrushFloat)i/(BrushFloat)nEdgeCount;
		// Calculation of cubic bezier curve as t.
		SSpot spot;
		spot.m_Pos = GetPlane().P2W(
			(1-t)*(1-t)*(1-t)*firstPointOnPlane + 
			3*(1-t)*(1-t)*t*middlePointOnPlane0 +
			3*(1-t)*t*t*middlePointOnPlane1 +
			t*t*t*lastPointOnPlane);
		spot.m_Plane = GetPlane();
		AddSpotToSpotList(spot);
	}
	AddSpotToSpotList(m_LastSpot);
}

void CBrushDesignerDrawCurveTool::Display( DisplayContext &dc )
{
	int oldThickness = dc.GetLineWidth();

	if( m_ArcState == eArcState_ChooseFirstPoint || m_ArcState == eArcState_ChooseLastPoint )
	{
		dc.SetFillMode(e_FillModeSolid);
		DrawCurrentSpot(dc,GetWorldTM());
	}

	if( m_ArcState == eArcState_ControlMiddlePoint )
	{
		if( GetSpotListCount() )
		{
			dc.SetColor(BUtil::RegionLineColor);
			DrawPolyline(dc);
		}
	}
	else if( m_ArcState == eArcState_ChooseLastPoint )
	{
		if( m_LineState == eLineState_ParallelToAxis ) 
			dc.SetColor(BUtil::RegionParallelToAxis);
		else
			dc.SetColor(BUtil::RegionLineColor);
		dc.DrawLine(GetStartSpotPos(), GetCurrentSpotPos());
	}

	dc.SetLineWidth(oldThickness);
}

bool CBrushDesignerDrawCurveTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if (nChar == VK_ESCAPE )
	{
		if( m_ArcState == eArcState_ControlMiddlePoint )
		{
			m_ArcState = eArcState_ChooseLastPoint;
			ResetAllSpots();
			return true;
		}
		else if( m_ArcState == eArcState_ChooseLastPoint )
		{
			m_ArcState = eArcState_ChooseFirstPoint;
			return true;
		}
		GetEditTool()->GoToSelectDesignerMode();
	}

	return true;
}