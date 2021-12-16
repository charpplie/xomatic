#include "StdAfx.h"
#include "BrushDesignerCreateBoxTool.h"
#include "BrushDesignerEditTool.h"
#include "BrushDesignerSelectTool.h"
#include "Viewport.h"
#include "Core/BrushDesignerAdjustHeightHelper.h"
#include "Core/BrushDesignerExtrudeSnappingHelper.h"
#include "IBaseToolPanel.h"

namespace
{
	ICreateBoxToolPanel* s_pBoxToolPanel = NULL;
}

void CBrushDesignerCreateBoxTool::Enter()
{
	__super::Enter();
	m_Phase = eBoxPhase_PlaceFirstPoint;
	m_bStartedUndo = false;
}

void CBrushDesignerCreateBoxTool::Leave()
{
	if( m_Phase == eBoxPhase_Done )
	{
		if( m_bStartedUndo )
			GetIEditor()->AcceptUndo("Designer : Create a Box");
		FreezeDesigner();
	}
	else
	{
		if( GetDesigner() )
		{
			DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
			GetDesigner()->SetShelf(1);
			GetDesigner()->Clear();
			UpdateShelf(1);
		}
		if( m_bStartedUndo )
			GetIEditor()->CancelUndo();
	}
	m_bStartedUndo = false;
	__super::Leave();
}

void CBrushDesignerCreateBoxTool::BeginEditParams()
{
	if( !s_pBoxToolPanel )
		s_pBoxToolPanel = CreateBoxPanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerCreateBoxTool::EndEditParams()
{
	if( s_pBoxToolPanel )
	{
		s_pBoxToolPanel->DestroyPanel();
		s_pBoxToolPanel = NULL;
	}
}

void CBrushDesignerCreateBoxTool::OnLButtonDown( CViewport *view, UINT nFlags, CPoint point )
{
	if( m_Phase == eBoxPhase_Done )
	{
		FreezeDesigner();
		if( m_bStartedUndo )
			GetIEditor()->AcceptUndo("Designer : Create a box");
		m_Phase = eBoxPhase_PlaceFirstPoint;
		m_bStartedUndo = false;
	}

	if( m_Phase == eBoxPhase_PlaceFirstPoint )
	{
		if( !UpdateCurrentSpotPosition(view,nFlags,point,false,false) )
			return;

		SetStartSpot(GetCurrentSpot());
		SetPlane(GetCurrentSpot().m_Plane);
		SetTempRegion(GetCurrentSpot().m_pRegion);
		m_Phase = eBoxPhase_DrawRectangle;
		s_SnappingHelper.Init(GetDesigner());
		GetIEditor()->BeginUndo();
		GetDesigner()->RecordUndo("Designer : Create a box",GetBaseObject());
		StoreSeparateStatus();
		m_bStartedUndo = true;
	}
	else if( m_Phase == eBoxPhase_RaiseHeight )
	{
		m_Phase = eBoxPhase_Done;
	}
}

void CBrushDesignerCreateBoxTool::OnLButtonUp( CViewport *view, UINT nFlags, CPoint point )
{
	if( m_Phase == eBoxPhase_DrawRectangle )
	{
		if( !GetCurrentSpotPos().IsEquivalent(GetStartSpotPos(),(BrushFloat)0.01) )
		{
			m_Phase = eBoxPhase_RaiseHeight;
			s_SnappingHelper.SearchForOppositeRegions(m_pCapRegion);
			m_pCapRegion = NULL;
			m_bIsOverOpposite = false;
			s_AdjustHeightHelper.Init(GetPlane(),GetCurrentSpotPos());
		}
	}
}

void CBrushDesignerCreateBoxTool::OnMouseMove( CViewport *view, UINT nFlags, CPoint point )
{
	if( m_Phase == eBoxPhase_PlaceFirstPoint || m_Phase == eBoxPhase_Done )
	{
		UpdateCurrentSpotPosition(view,nFlags,point,false,true);
		if( m_Phase == eBoxPhase_PlaceFirstPoint )
			SetPlane(GetCurrentSpot().m_Plane);
	}
	else if( m_Phase == eBoxPhase_DrawRectangle )
	{
		if( (nFlags&MK_LBUTTON) && UpdateCurrentSpotPosition(view,nFlags,point,true) )
			UpdateBox( GetStartSpotPos(), GetCurrentSpotPos(), 0 );
	}
	else if( m_Phase == eBoxPhase_RaiseHeight )
	{
		BrushFloat fHeight = s_AdjustHeightHelper.UpdateHeight( GetWorldTM(), view, point);

		if( fHeight < kInitialPrimitiveHeight )
			fHeight = 0;

		if( nFlags & MK_SHIFT )
		{
			CBrushRegion::RegionPtr pAlignedRegion = s_SnappingHelper.FindAlignedRegion(m_pCapRegion,GetWorldTM(),view,point);
			if( pAlignedRegion )
				fHeight = GetPlane().Distance() - pAlignedRegion->GetPlane().Distance();
		}

		UpdateBoxWithBoundaryCheck( GetStartSpotPos(), GetCurrentSpotPos(), fHeight );
	}
}

bool CBrushDesignerCreateBoxTool::OnKeyDown( CViewport *view, uint32 nChar, uint32 nRepCnt, uint32 nFlags )
{
	if (nChar == VK_ESCAPE )
	{
		if( m_Phase == eBoxPhase_DrawRectangle || m_Phase == eBoxPhase_RaiseHeight )
		{
			CancelDesigner();
			m_Phase = eBoxPhase_PlaceFirstPoint;
			return true;
		}
		else if( m_Phase == eBoxPhase_Done )
		{
			FreezeDesigner();
			GetIEditor()->AcceptUndo("Designer : Create a box");
			m_Phase = eBoxPhase_PlaceFirstPoint;
			return true;
		}
		GetEditTool()->GoToSelectDesignerMode();
	}
	return true;
}

void CBrushDesignerCreateBoxTool::UpdateBoxWithBoundaryCheck( const BrushVec3& v0, const BrushVec3& v1, BrushFloat fHeight )
{
	UpdateBox(v0,v1,fHeight);
	m_bIsOverOpposite = m_pCapRegion && s_SnappingHelper.IsOverOppositeRegion(m_pCapRegion,BUtil::ePP_Pull);
	if( m_bIsOverOpposite )
	{
		fHeight = s_SnappingHelper.GetNearestDistanceToOpposite(BUtil::ePP_Pull);
		UpdateBox(v0,v1,fHeight);
	}
}

void CBrushDesignerCreateBoxTool::UpdateBox( const BrushVec3& v0, const BrushVec3& v1, BrushFloat fHeight )
{
	std::vector<CBrushRegion::RegionPtr> regionList;

	std::vector<BrushVec3> vList(4);

	BrushVec2 p0 = GetPlane().W2P(v0);
	BrushVec2 p1 = GetPlane().W2P(v1);

	m_v[0] = GetPlane().P2W(p0);
	m_v[1] = GetPlane().P2W(p1);

	vList[0] = m_v[0];
	vList[1] = GetPlane().P2W(BrushVec2(p1.x,p0.y));
	vList[2] = m_v[1];
	vList[3] = GetPlane().P2W(BrushVec2(p0.x,p1.y));

	regionList.push_back(new CBrushRegion(vList));	
	if( regionList[0]->IsOpen() )
		return;
	if( regionList[0]->GetPlane().IsSameFacing(GetPlane()) )
		regionList[0]->Flip();

	for( int i = 0; i < 4; ++i )
		vList[i] += GetPlane().Normal() * fHeight;
	regionList.push_back(new CBrushRegion(vList));
	if( !regionList[1]->GetPlane().IsSameFacing(GetPlane()) )
		regionList[1]->Flip();

	for( int i = 0; i < 4; ++i )
	{
		BrushEdge3D e = regionList[0]->GetEdge(i);

		vList[0] = e.m_v[1];
		vList[1] = e.m_v[0];
		vList[2] = e.m_v[0] + GetPlane().Normal() * fHeight;
		vList[3] = e.m_v[1] + GetPlane().Normal() * fHeight;

		regionList.push_back(new CBrushRegion(vList));
	}

	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	GetDesigner()->SetShelf(1);
	GetDesigner()->Clear();

	for( int i = 0, iCount(regionList.size()); i < iCount; ++i )
	{
		if( regionList[i]->IsOpen() )
			continue;

		if( GetTempRegion() )
		{
			regionList[i]->SetTexInfo(GetTempRegion()->GetTexInfo());
			regionList[i]->SetMaterialID(GetTempRegion()->GetMaterialID());
		}

		GetDesigner()->AddRegion(regionList[i], CBrushDesigner::eOpType_Add);

		if( regionList[i]->GetPlane().Normal().IsEquivalent(GetPlane().Normal(),kDesignerEpsilon) )
			m_pCapRegion = regionList[i];
	}

	s_pBoxToolPanel->Update(p0,p1,fHeight);

	CBrushDesignerBaseTool::CreateMirroredRegions(GetDesigner());
	GetBrush()->Update(GetBaseObject(), GetDesigner(), GetDesigner()->GetShelf());
}

void CBrushDesignerCreateBoxTool::Display( DisplayContext &dc )
{
	DisplayCurrentSpot(dc);
	DisplayDimensionHelper(dc,1);
	if( GetStartSpot().m_pRegion )
		DisplayDimensionHelper(dc);
	s_AdjustHeightHelper.Display(dc);
}

void CBrushDesignerCreateBoxTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch(event)
	{
	case eNotify_OnBeginUndoRedo:
	case eNotify_OnBeginSceneSave:
		if( m_Phase == eBoxPhase_Done )
		{
			GetIEditor()->AcceptUndo("Designer : Create a Box");
			FreezeDesigner();
		}
		else
		{
			GetIEditor()->CancelUndo();
			CancelDesigner();
		}
		m_Phase = eBoxPhase_PlaceFirstPoint;
		break;
	}
}

void CBrushDesignerCreateBoxTool::FreezeDesigner()
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	if( m_pCapRegion )
	{
		if( m_bIsOverOpposite )
			s_SnappingHelper.ApplyOppositeRegions(m_pCapRegion,BUtil::ePP_Pull,true);

		if( !IsSeparateStatus() )
		{
			GetDesigner()->SetShelf(1);
			std::vector<CBrushRegion::RegionPtr> sideRegions;

			BrushPlane invertedCapRegionPlane = m_pCapRegion->GetPlane().GetInverted();
			for( int i = 0, iRegionCount(GetDesigner()->GetRegionSize()); i < iRegionCount; ++i )
			{
				CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(i);
				if( pRegion != m_pCapRegion && !invertedCapRegionPlane.Normal().IsEquivalent(pRegion->GetPlane().Normal(),kDesignerEpsilon) )
					sideRegions.push_back(pRegion);
			}

			std::vector<CBrushRegion::RegionPtr> intersectedSideRegions;
			for( int i = 0, iSideRegionCount(sideRegions.size()); i < iSideRegionCount; ++i )
			{
				for( int k = 0; k < 2; ++k )
				{
					CBrushRegion::RegionPtr pSideRegion = k == 0 ? sideRegions[i]->Clone()->Flip() : sideRegions[i];

					GetDesigner()->SetShelf(0);
					bool bHasIntersected = GetDesigner()->HasIntersection(pSideRegion,true);
					bool bTouched = GetDesigner()->HasTouched(pSideRegion);
					if( (!bHasIntersected && !bTouched) || (bTouched && k == 0) )
						continue;

					GetDesigner()->SetShelf(1);
					GetDesigner()->RemoveRegion(sideRegions[i]);

					GetDesigner()->SetShelf(0);
					if( bHasIntersected )
					{
						GetDesigner()->AddRegion(pSideRegion,CBrushDesigner::eOpType_ExclusiveOR);
						intersectedSideRegions.push_back(pSideRegion);
					}
					else if( bTouched )
					{
						GetDesigner()->AddRegion(pSideRegion,CBrushDesigner::eOpType_Union);
					}
					break;
				}
			}

			for( int i = 0, iCount(intersectedSideRegions.size()); i < iCount; ++i )
			{
				GetDesigner()->SetShelf(0);
				GetDesigner()->SeparateRegions(intersectedSideRegions[i]->GetPlane());
			}
		}
	}

	CBrushDesignerBaseTool::FreezeDesigner();
}
