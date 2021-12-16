#include "StdAfx.h"
#include "BrushDesignerDrawLineTool.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesigner.h"
#include "ViewManager.h"
#include "Grid.h"

void CBrushDesignerDrawLineTool::RegisterSpotListAfterBreaking()
{
	int nFirstSpotIndexOnEdge = -1;
	for( int i = 0, iSpotSize(GetSpotListCount()); i < iSpotSize; ++i )
	{
		const SSpot& spot = GetSpot(i);
		if( spot.m_bProcessed )
			continue;
		if( spot.IsOnEdge() )
		{
			if( nFirstSpotIndexOnEdge == -1 )
			{
				nFirstSpotIndexOnEdge = i;
			}
			else
			{
				SpotList spotList;
				for( int k = nFirstSpotIndexOnEdge; k <= i; ++k )
				{
					spotList.push_back(GetSpot(k));
					SetSpotProcessed(k,true);
				}

				SetSpotProcessed(i,false);
				GetDesigner()->RecordUndo("Add Region",GetBaseObject());

				RegisterSpotList(GetDesigner(),spotList);
				UpdateMirroredPartWithPlane(GetDesigner(), GetPlane());
				nFirstSpotIndexOnEdge = i;
			}
		}
	}
}

void CBrushDesignerDrawLineTool::RegisterEitherEndSpotList()
{
	if( GetSpotListCount() == 0 )
		return;
	bool bAddedLastSpot = false;
	if( GetSpot(0).m_bProcessed == false )
	{
		for( int i = 1; i < GetSpotListCount(); ++i )
		{
			if( GetSpot(i).IsOnEdge() || i == GetSpotListCount()-1 )
			{
				bAddedLastSpot = i == GetSpotListCount()-1;
				SpotList spotList;
				for( int k = 0; k <= i; ++k )
					spotList.push_back(GetSpot(k));
				RegisterSpotList(GetDesigner(),spotList);
				break;
			}
		}
	}

	if( !bAddedLastSpot && !GetSpot(GetSpotListCount()-1).IsOnEdge() )
	{
		for( int i = GetSpotListCount()-2; i >= 0; --i )
		{
			if( GetSpot(i).IsOnEdge() )
			{
				SpotList spotList;
				for( int k = i; k < GetSpotListCount(); ++k )
					spotList.push_back(GetSpot(k));
				RegisterSpotList(GetDesigner(),spotList);
				break;
			}
		}
	}
}

void CBrushDesignerDrawLineTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	m_bAlignedToRecentSpotLine = false;
	m_bAlignedToAnotherEdge = false;
}

void CBrushDesignerDrawLineTool::OnLButtonUp( CViewport *view,UINT nFlags,CPoint point )
{
	m_bHasValidRecentEdge = false;

	if( GetSpotListCount() == 1 && GetSpotPos(0).IsEquivalent(GetCurrentSpotPos(),kDesignerEpsilon) )
		return;

	if( GetDesigner()->IsEmpty() && GetSpotListCount() == 0 )
	{
		SSpot spot = GetCurrentSpot();
		GetBaseObject()->SetWorldPos(GetWorldTM().TransformPoint(spot.m_Pos));
		spot.m_Pos = BrushVec3(0,0,0);
		spot.m_Plane.Set(spot.m_Plane.Normal(),0);
		SetPlane(spot.m_Plane);
		SetCurrentSpot(spot);
	}

	PutCurrentSpot();
}

void CBrushDesignerDrawLineTool::AddRegionWithCurrentSameAsFirst()
{
	if( GetDesigner() )
		GetDesigner()->RecordUndo("Add Region",GetBaseObject());

	int nFirstSpotIndex = GetSpotListCount()-1;
	for( ; nFirstSpotIndex >= 0; --nFirstSpotIndex )
	{
		if( GetSpot(nFirstSpotIndex).IsOnEdge() && !GetSpot(nFirstSpotIndex).m_bProcessed )
			break;
	}

	if( nFirstSpotIndex == -1 )
	{
		RegisterClosedRegion();
	}
	else
	{
		std::vector<SSpot> spotList;
		int nSpotIndex = nFirstSpotIndex;
		do
		{
			const SSpot& spot = GetSpot(nSpotIndex%GetSpotListCount());
			spotList.push_back(spot);
		} while(!GetSpot((++nSpotIndex)%GetSpotListCount()).IsOnEdge());
		spotList.push_back(GetSpot(nSpotIndex%GetSpotListCount()));
		RegisterSpotList(GetDesigner(),spotList);
	}
	ResetAllSpots();
	Sync();
}

void CBrushDesignerDrawLineTool::PutCurrentSpot()
{
	if( GetLineState() == eLineState_Cross )
		return;

	if( GetSpotListCount() == 0 )
	{
		SetStartSpot(GetCurrentSpot());
		DESIGNER_ASSERT( GetStartSpot().m_PosState != eSpotPosState_Invalid );
	}

	CUndo undo("Designer : Add Region");

	if( GetSpotListCount() > 0 && GetDesigner() )
	{
		const BrushVec3& lastPos = GetSpotPos(GetSpotListCount()-1);
		const BrushVec3& currPos = GetCurrentSpot().m_Pos;

		const SSpot& lastSpot = GetSpot(GetSpotListCount()-1);
		if( lastSpot.IsInRegion() && GetCurrentSpot().IsInRegion() && !GetCurrentSpot().m_Plane.IsEquivalent(lastSpot.m_Plane,kDesignerEpsilon) )
			return;

		std::vector<CBrushDesigner::IntersectionPair> intersections;
		GetDesigner()->QueryIntersectionByEdge( BrushEdge3D(lastPos,currPos), intersections );
		for( int i = 0, iSize(intersections.size()); i < iSize; ++i )
		{
			if( intersections[i].second.IsEquivalent(lastPos,kDesignerEpsilon) || intersections[i].second.IsEquivalent(currPos,kDesignerEpsilon) )
				continue;
			AddSpotToSpotList(SSpot(intersections[i].second,eSpotPosState_Edge,intersections[i].first));
		}
	}

	AddSpotToSpotList(GetCurrentSpot());
	RegisterSpotListAfterBreaking();

	if( GetSpotListCount() > 1 ) 
	{
		if( GetSpotListCount() > 2 && GetCurrentSpot().IsSamePos(GetSpot(0)) && !GetSpot(0).m_bProcessed )
			AddRegionWithCurrentSameAsFirst();
	}

	ResetCurrentSpotWeakly();
	m_RecentSpotOnEdge.Reset();
}

void CBrushDesignerDrawLineTool::CreateRegionFromSpots( bool bCloseRegion, const SpotList& spotList )
{
	std::vector<BrushVec3> vList;
	GenerateVertexListFromSpotList(spotList,vList);

	BUtil::STexInfo texInfo = GetTexInfo();
	SetIntermediateRegion( new CBrushRegion( vList, GetPlane(), GetMatID(), &texInfo, bCloseRegion ) );
	if( GetIntermediateRegion() )
	{
		if( bCloseRegion )
			GetIntermediateRegion()->ModifyOrientation();
		GetDesigner()->AddRegion( GetIntermediateRegion(), CBrushDesigner::eOpType_Split );
		UpdateMirroredPartWithPlane( GetDesigner(), GetPlane() );
		UpdateBrush();
	}
}

void CBrushDesignerDrawLineTool::RegisterClosedRegion()
{
	CreateRegionFromSpots( true, GetSpotList() );
	if( GetDesigner() )
		GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
}

void CBrushDesignerDrawLineTool::Complete()
{
	if( GetSpotListCount() )
	{
		CUndo undo("Designer : Add Region");
		if( GetDesigner() )
			GetDesigner()->RecordUndo("Add Region",GetBaseObject());
		SetEditMode(eEditMode_None);
		RegisterEitherEndSpotList();
		UpdateMirroredPartWithPlane( GetDesigner(), GetPlane() );
		ResetAllSpots();
		UpdateBrush();
		Sync();		
	}

}

bool CBrushDesignerDrawLineTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if (nChar == VK_ESCAPE )
	{
		if( GetSpotListCount() )
		{
			Complete();
			return true;
		}
		GetEditTool()->GoToSelectDesignerMode();
	}

	return true;
}

CBrushDesignerDrawLineTool::ELineState CBrushDesignerDrawLineTool::GetAlienedPointWithAxis( const BrushVec3& v0, const BrushVec3& v1, const BrushPlane& plane, BrushFloat angle, std::vector<BrushEdge3D>* pAxisList, BrushVec3& outPos ) const
{
	BrushVec2 prevPosOnPlane = plane.W2P(v0);
	BrushVec2 curPosOnPlane = plane.W2P(v1);
	BrushVec2 vPrev2Current = (curPosOnPlane-prevPosOnPlane).GetNormalized();

	static const BrushFloat kMaxLength = 15.0f;
	BrushFloat fLength = std::min( (curPosOnPlane-prevPosOnPlane).GetLength(), kMaxLength );

	ELineState lineState = eLineState_Diagonal;
	const BrushFloat kBoundaryCos = std::cos((angle/180.0f)*BUtil::PI*std::pow(1.1-fLength/kMaxLength,1.2));

	if( !pAxisList )
	{
		BrushVec2 xAxis(1,0);
		BrushVec2 yAxis(0,1);

		if( vPrev2Current.Dot(xAxis) > kBoundaryCos || vPrev2Current.Dot(-xAxis) > kBoundaryCos )
		{
			lineState = eLineState_ParallelToAxis;
			curPosOnPlane.y = prevPosOnPlane.y;
		}
		else if( vPrev2Current.Dot(yAxis) > kBoundaryCos || vPrev2Current.Dot(-yAxis) > kBoundaryCos )
		{
			lineState = eLineState_ParallelToAxis;
			curPosOnPlane.x = prevPosOnPlane.x;
		}
	}
	else
	{
		for( int i = 0, iSize(pAxisList->size()); i < iSize; ++i )
		{
			BrushVec2 vAxisV0 = plane.W2P((*pAxisList)[i].m_v[0]);
			BrushVec2 vAxisV1 = plane.W2P((*pAxisList)[i].m_v[1]);
			BrushVec2 customAxis = (vAxisV1-vAxisV0).GetNormalized();
			BrushLine vAxisLine(prevPosOnPlane,prevPosOnPlane+customAxis);

			if( vPrev2Current.Dot(customAxis) > kBoundaryCos || vPrev2Current.Dot(-customAxis) > kBoundaryCos )
			{
				if( vAxisLine.HitTest( curPosOnPlane, curPosOnPlane+vAxisLine.m_Normal, kDesignerEpsilon, NULL, &curPosOnPlane ) )
				{
					lineState = eLineState_ParallelToAxis;
					break;
				}
			}
		}
	}

	outPos = plane.P2W(curPosOnPlane);

	return lineState;
}

bool CBrushDesignerDrawLineTool::IsPhaseFirstStepOnPrimitiveCreation() const
{
	return GetSpotListCount() == 0;
}

void CBrushDesignerDrawLineTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	if( !GetDesigner() )
		return;

	m_bAlignedToRecentSpotLine = false;
	const float fMagneticSize = 22.0f;

	BrushVec3 localRaySrc, localRayDir;
	BUtil::GetLocalViewRay( GetWorldTM(), view, point, localRaySrc, localRayDir );
	int nRegionIdx = 0;
	bool bPickedDesignerObject = GetDesigner()->QueryRegion(localRaySrc,localRayDir,nRegionIdx);
	bool bKeepInitialPlane = GetSpotListCount() > 0 && (GetSpot(0).m_pRegion == NULL || !bPickedDesignerObject)? true : false;

	if( CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, bKeepInitialPlane ) )
	{
		SetPlane(GetCurrentSpot().m_Plane);
		SetLineState(eLineState_Diagonal);

		bool bProceedFurther = true;

		if( nFlags == MK_LBUTTON )
		{
			if( GetCurrentSpot().IsAtEitherPointOnEdge() || GetCurrentSpot().IsCenterOfEdge() )
			{
				if( !GetStartSpotPos().IsEquivalent(GetCurrentSpotPos(),kDesignerEpsilon) )
					m_RecentSpotOnEdge = GetCurrentSpot();
			}
			else if( GetCurrentSpot().IsOnEdge() )
			{
				if( GetCurrentSpot().m_pRegion && !m_bAlignedToAnotherEdge )
				{
					BrushVec3 pos;
					m_bHasValidRecentEdge = GetCurrentSpot().m_pRegion->QueryNearestEdge( GetCurrentSpotPos(), m_RecentEdge, pos );
				}
			}

			if( m_RecentSpotOnEdge.IsOnEdge() )
			{
				BrushVec3 outPos;
				ELineState lineState = GetAlienedPointWithAxis( m_RecentSpotOnEdge.m_Pos, GetCurrentSpotPos(), GetPlane(), fMagneticSize, NULL, outPos );
				if( lineState == eLineState_ParallelToAxis )
				{
					m_bAlignedToRecentSpotLine = true;
					SetCurrentSpotPos(outPos);
					bProceedFurther = false;
				}
			}

			if( m_bHasValidRecentEdge && GetSpotListCount() > 0 )
			{
				std::vector<BrushEdge3D> vAxisList;
				vAxisList.push_back(m_RecentEdge);
				BrushVec3 outPos;
				ELineState lineState = GetAlienedPointWithAxis( GetSpotPos(GetSpotListCount()-1), GetCurrentSpotPos(), GetPlane(), fMagneticSize, &vAxisList, outPos );
				if( lineState == eLineState_ParallelToAxis )
				{
					if( m_bAlignedToRecentSpotLine || GetCurrentSpot().IsOnEdge() && GetCurrentSpot().m_pRegion )
					{
						BrushVec2 v0 = GetPlane().W2P(GetSpotPos(GetSpotListCount()-1));
						BrushVec2 v1 = GetPlane().W2P(GetCurrentSpotPos());
						BrushVec2 vEdgeDir = (GetPlane().W2P(m_RecentEdge.m_v[1])-GetPlane().W2P(m_RecentEdge.m_v[0])).GetNormalized();
						BrushLine line(v0,v0+vEdgeDir);
						BrushVec2 hitPos2D;

						BrushVec2 vTarget;
						if( GetCurrentSpot().IsOnEdge() && GetCurrentSpot().m_pRegion )
						{
							BrushEdge3D nearestEdge;
							BrushVec3 posOnEdge;
							if( GetCurrentSpot().m_pRegion->QueryNearestEdge(GetCurrentSpotPos(), nearestEdge, posOnEdge) )
								vTarget = GetPlane().W2P(nearestEdge.m_v[1]);
							else
								vTarget = v1 + line.m_Normal;
						}
						else
						{
							vTarget = GetPlane().W2P(m_RecentSpotOnEdge.m_Pos);
						}

						if( line.HitTest( v1, vTarget, kDesignerEpsilon, NULL, &hitPos2D ) )
							outPos = GetPlane().P2W(hitPos2D);
					}

					SetLineState(lineState);
					SetCurrentSpotPos(outPos);
					bProceedFurther = false;
					m_bAlignedToAnotherEdge = true;
				}
			}
		}

		if( bProceedFurther )
		{
			if( !GetCurrentSpot().IsAtEitherPointOnEdge() && !GetCurrentSpot().IsAtEndPoint() )
				AlignEdgeWithPrincipleAxises( view, (nFlags&MK_SHIFT) ? false : true );
		}
	}
}

void CBrushDesignerDrawLineTool::AlignEdgeWithPrincipleAxises( IDisplayViewport* view, bool bAlign )
{
	if( GetSpotListCount() == 0 )
		return;

	int nSpotIndex = -1;
	bool bIntersectAgainstOtherEdges = IntersectExisintingLines(GetSpot(GetSpotListCount()-1).m_Pos, GetCurrentSpotPos(),&nSpotIndex);
	bool bStartAndCurrentSame = BUtil::AreTwoPositionsNear(GetCurrentSpotPos(),GetStartSpotPos(),GetWorldTM(),view,kLimitForMagnetic);

	if( bIntersectAgainstOtherEdges && ( nSpotIndex > 0 || !bStartAndCurrentSame ) )
	{
		SetLineState(eLineState_Cross);
		return;
	}

	if( bAlign )
	{
		BrushVec3 outPos;
		ELineState lineState = GetAlienedPointWithAxis( GetSpot(GetSpotListCount()-1).m_Pos, GetCurrentSpotPos(), GetPlane(), 8.0f, NULL, outPos );

		if( GetCurrentSpot().IsOnEdge() )
		{
			std::vector<CBrushDesigner::SQueryEdgeResult> queryResults;
			BrushVec3 nearestPos;
			if( GetDesigner()->QueryNearestEdges( GetPlane(), outPos, nearestPos, queryResults ) && !queryResults.empty() )
			{
				BrushVec2 lastSpotPos2D = GetPlane().W2P(GetSpot(GetSpotListCount()-1).m_Pos);
				BrushVec2 currentSpotPos2D = GetPlane().W2P(GetCurrentSpotPos());
				BrushVec2 pos2D = GetPlane().W2P(outPos);
				BrushLine line(GetPlane().W2P(queryResults[0].m_Edge.m_v[0]), GetPlane().W2P(queryResults[0].m_Edge.m_v[1]));
				if( line.HitTest( pos2D, pos2D+(lastSpotPos2D-currentSpotPos2D), kDesignerEpsilon, NULL, &pos2D ) )
					outPos = GetPlane().P2W(pos2D);
			}
		}

		SetLineState(lineState);
		SetCurrentSpotPos(outPos);
	}
}

bool CBrushDesignerDrawLineTool::IntersectExisintingLines( const BrushVec3& v0, const BrushVec3& v1, int* pOutSpotIndex ) const
{
	BrushVec2 vP0 = GetPlane().W2P(v0);
	BrushVec2 vP1 = GetPlane().W2P(v1);
	BrushEdge edgeP(vP0, vP1);
	BrushLine lineP(edgeP.m_v[0],edgeP.m_v[1]);

	for( int i = 0, iSpotSize(GetSpotListCount()-1); i < iSpotSize; ++i )
	{
		if( GetSpot(i).m_bProcessed )
			continue;

		BrushVec2 vQ0 = GetPlane().W2P(GetSpot(i).m_Pos);
		BrushVec2 vQ1 = GetPlane().W2P(GetSpot(i+1).m_Pos);
		BrushEdge edgeQ(vQ0, vQ1);
		BrushLine lineQ(edgeQ.m_v[0],edgeQ.m_v[1]);

		BrushFloat d0 = lineQ.Distance(edgeP.m_v[0]);
		BrushFloat d1 = lineQ.Distance(edgeP.m_v[1]);

		if( d0 > kDesignerEpsilon && d1 > kDesignerEpsilon || d0 < -kDesignerEpsilon && d1 < -kDesignerEpsilon )
			continue;

		if( d0 > -kDesignerEpsilon && d0 < kDesignerEpsilon || d1 > -kDesignerEpsilon && d1 < kDesignerEpsilon )
			continue;

		BrushVec2 intersection;
		if( lineQ.Intersect(lineP,intersection,kDesignerEpsilon) )
		{
			if( (intersection.x > edgeQ.m_v[0].x && intersection.x < edgeQ.m_v[1].x || intersection.x > edgeQ.m_v[1].x && intersection.x < edgeQ.m_v[0].x) &&
				(intersection.y > edgeQ.m_v[0].y && intersection.y < edgeQ.m_v[1].y || intersection.y > edgeQ.m_v[1].y && intersection.y < edgeQ.m_v[0].y) )
			{
				if( pOutSpotIndex )
					*pOutSpotIndex = i;
				return true;
			}
		}
	}

	return false;
}

void CBrushDesignerDrawLineTool::Display( DisplayContext &dc )
{
	__super::Display(dc);

	int oldThickness = dc.GetLineWidth();
	dc.SetFillMode( e_FillModeSolid );
	DrawCurrentSpot(dc,GetWorldTM());

	dc.SetColor(BUtil::RegionLineColor);
	DrawPolyline(dc);

	if( GetSpotListCount() )
	{
		if( GetLineState() == eLineState_Cross )
			dc.SetColor(BUtil::RegionInvalidLineColor);
		else if( GetLineState() == eLineState_ParallelToAxis ) 
			dc.SetColor(BUtil::RegionParallelToAxis);
		dc.DrawLine( GetSpot(GetSpotListCount()-1).m_Pos, GetCurrentSpotPos() );
	}

	if( m_bAlignedToRecentSpotLine )
	{
		dc.SetColor(ColorB(210,210,210,255));
		dc.DrawLine(m_RecentSpotOnEdge.m_Pos, GetCurrentSpotPos());
		BUtil::DrawSpot(dc, GetWorldTM(), m_RecentSpotOnEdge.m_Pos, ColorB(100,100,180,255));
	}

	dc.SetLineWidth(oldThickness);
}

void CBrushDesignerDrawLineTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	__super::OnEditorNotifyEvent(event);
	switch(event)
	{
	case eNotify_OnEndUndoRedo:
		ResetAllSpots();
		break;
	}
}