#include "StdAfx.h"
#include "BrushDesignerCreateCylinderTool.h"
#include "Core/BrushDesigner.h"
#include "Core/BrushPrimitive.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesignerAdjustHeightHelper.h"
#include "Core/BrushDesignerExtrudeSnappingHelper.h"
#include "IBaseToolPanel.h"
#include "ViewManager.h"

namespace 
{
	ICreateCylinderConeToolPanel* g_pCylinderToolPanel = NULL;
}

void CBrushDesignerCreateCylinderTool::Enter()
{
	__super::Enter();
	m_CylinderPhase = eCylinderPhase_PlaceFirstPoint;
}

void CBrushDesignerCreateCylinderTool::Leave()
{
	if( m_CylinderPhase == eCylinderPhase_Done )
	{
		FreezeDesigner();
		GetIEditor()->AcceptUndo("Designer : Create a Cylinder");
	}
	else
	{
		DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
		GetDesigner()->SetShelf(1);
		GetDesigner()->Clear();
		UpdateShelf(1);
		GetIEditor()->CancelUndo();
	}
	__super::Leave();
}

void CBrushDesignerCreateCylinderTool::BeginEditParams()
{
	if( !g_pCylinderToolPanel )
		g_pCylinderToolPanel = CreateCylinderPanel(this,(void*)GetPanelIndex());	
}

void CBrushDesignerCreateCylinderTool::EndEditParams()
{
	if( g_pCylinderToolPanel )
	{
		g_pCylinderToolPanel->DestroyPanel();
		g_pCylinderToolPanel = NULL;
	}
}

void CBrushDesignerCreateCylinderTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	bool bKeepInitPlane = m_CylinderPhase == eCylinderPhase_Radius ? true : false;
	bool kSearchAllShelves = m_CylinderPhase == eCylinderPhase_Done ? true : false;
	if( !CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, bKeepInitPlane, kSearchAllShelves ) )
		return;

	if( m_CylinderPhase == eCylinderPhase_Radius )
	{
		BrushVec2 vSpotPos2D = GetPlane().W2P(GetCurrentSpotPos());
		BrushFloat fRadius = (vSpotPos2D-m_vCenterOnPlane).GetLength();
		const BrushFloat kSmallestRadius = 0.05f;
		if( fRadius < kSmallestRadius )
			fRadius = kSmallestRadius;
		m_fAngle = BUtil::ComputeAnglePointedByPos( m_vCenterOnPlane, vSpotPos2D );
		UpdateBaseRegion( fRadius, g_pCylinderToolPanel->GetSubdivisionNum());
		g_pCylinderToolPanel->Update( fRadius, 0 );		
	}
	else if( m_CylinderPhase == eCylinderPhase_RaiseHeight )
	{
		BrushFloat fHeight = s_AdjustHeightHelper.UpdateHeight( GetWorldTM(), view, point );
		if( fHeight < kInitialPrimitiveHeight )
			fHeight = 0;
		if( nFlags & MK_SHIFT )
		{
			CBrushRegion::RegionPtr pAlignedRegion = s_SnappingHelper.FindAlignedRegion(m_pCapRegion,GetWorldTM(),view,point);
			if( pAlignedRegion )
				fHeight = GetPlane().Distance() - pAlignedRegion->GetPlane().Distance();
		}
		UpdateHeightWithBoundaryCheck(fHeight);
		g_pCylinderToolPanel->Update( g_pCylinderToolPanel->GetRadius(), (float)fHeight );
	}
}

void CBrushDesignerCreateCylinderTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_CylinderPhase == eCylinderPhase_Done )
	{
		m_CylinderPhase = eCylinderPhase_PlaceFirstPoint;
		FreezeDesigner();
		GetIEditor()->AcceptUndo("Designer : Create a Cylinder");
	}

	if( m_CylinderPhase == eCylinderPhase_PlaceFirstPoint )
	{
		if( CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, false, true ) )
		{
			SetStartSpot(GetCurrentSpot());
			m_CylinderPhase = eCylinderPhase_Radius;
			SetPlane(GetCurrentSpot().m_Plane);
			m_vCenterOnPlane = GetPlane().W2P(GetCurrentSpotPos());
			GetIEditor()->BeginUndo();
			GetDesigner()->RecordUndo("Designer : Create a Cylinder",GetBaseObject());
			SetTempRegion(GetCurrentSpot().m_pRegion);
			StoreSeparateStatus();
		}
	}
	else if( m_CylinderPhase == eCylinderPhase_Radius )
	{
		if( GetTempRegion() )
		{
			BrushVec3 localRaySrc, localRayDir;
			BUtil::GetLocalViewRay( GetWorldTM(), view, point, localRaySrc, localRayDir );
			BrushVec3 vHit;
			if( GetTempRegion()->GetPlane().HitTest( localRaySrc, localRaySrc+localRayDir,kDesignerEpsilon,NULL,&vHit) )
				s_AdjustHeightHelper.Init(GetTempRegion()->GetPlane(),vHit);
		}
		else if( CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, false, true ) )
			s_AdjustHeightHelper.Init(GetPlane(),GetCurrentSpotPos());
		s_SnappingHelper.Init(GetDesigner());
		UpdateHeight(0);
		if( m_pCapRegion )
			s_SnappingHelper.SearchForOppositeRegions(m_pCapRegion);
		m_bIsOverOpposite = false;
		m_CylinderPhase = eCylinderPhase_RaiseHeight;
	}
	else if( m_CylinderPhase == eCylinderPhase_RaiseHeight )
	{
		m_CylinderPhase = eCylinderPhase_Done;
	}
}

bool CBrushDesignerCreateCylinderTool::OnKeyDown( CViewport *view, uint32 nChar, uint32 nRepCnt, uint32 nFlags )
{
	if (nChar == VK_ESCAPE )
	{
		if( m_CylinderPhase == eCylinderPhase_Radius || m_CylinderPhase == eCylinderPhase_RaiseHeight )
		{
			CancelDesigner();
			m_CylinderPhase = eCylinderPhase_PlaceFirstPoint;
			return true;
		}
		else if( m_CylinderPhase == eCylinderPhase_Done )
		{
			FreezeDesigner();
			m_CylinderPhase = eCylinderPhase_PlaceFirstPoint;
			GetIEditor()->AcceptUndo("Designer : Create a Cylinder");
			return true;
		}
		GetEditTool()->GoToSelectDesignerMode();
	}
	return true;
}

void CBrushDesignerCreateCylinderTool::Display( DisplayContext &dc )
{
	if( !GetDesigner() || !GetBaseObject() )
		return;

	DisplayCurrentSpot(dc);

	if( m_CylinderPhase == eCylinderPhase_Radius || m_CylinderPhase == eCylinderPhase_RaiseHeight || m_CylinderPhase == eCylinderPhase_Done )
		DrawIntermediateRegion(dc);

	if( m_CylinderPhase == eCylinderPhase_Radius && GetIntermediateRegion() )
		DisplayDimensionHelper(dc,GetIntermediateRegion()->GetBoundBox());
	else
		DisplayDimensionHelper(dc,1);

	if( GetStartSpot().m_pRegion )
		DisplayDimensionHelper(dc);

	s_AdjustHeightHelper.Display(dc);
}

void CBrushDesignerCreateCylinderTool::UpdateBaseRegion( float fRadius, int nNumOfSubdivision )
{
	std::vector<BrushVec2> vertices2D;
	BUtil::MakeSectorOfCircle( fRadius, m_vCenterOnPlane, m_fAngle, BUtil::PI2, nNumOfSubdivision+1, vertices2D );
	vertices2D.erase(vertices2D.begin());
	BUtil::STexInfo texInfo = GetTexInfo();
	if( GetIntermediateRegion() )
		*GetIntermediateRegion() = CBrushRegion( vertices2D, GetPlane(), GetMatID(), &texInfo, true );
	else
		SetIntermediateRegion( new CBrushRegion( vertices2D, GetPlane(), GetMatID(), &texInfo, true ) );
	m_pBaseRegion = GetIntermediateRegion()->Flip();
}

void CBrushDesignerCreateCylinderTool::UpdateHeightWithBoundaryCheck( BrushFloat fHeight )
{
	UpdateHeight(fHeight);
	m_bIsOverOpposite = m_pCapRegion && s_SnappingHelper.IsOverOppositeRegion(m_pCapRegion,BUtil::ePP_Pull);
	if( m_bIsOverOpposite )
	{
		fHeight = s_SnappingHelper.GetNearestDistanceToOpposite(BUtil::ePP_Pull);
		UpdateHeight(fHeight);
	}
}

void CBrushDesignerCreateCylinderTool::UpdateAll( float fRadius, float fHeight, int nSubDivisionNum )
{
	UpdateBaseRegion( fRadius, nSubDivisionNum );
	UpdateHeightWithBoundaryCheck( fHeight );
}

void CBrushDesignerCreateCylinderTool::UpdateHeight( float fHeight )
{
	if( m_CylinderPhase == eCylinderPhase_PlaceFirstPoint )
		return;

	DESIGNER_ASSERT(m_pBaseRegion);
	if( !m_pBaseRegion )
		return;

	std::vector<CBrushRegion::RegionPtr> regionList;
	CBrushPrimitive bp(NULL);
	bp.CreateCylinder( m_pBaseRegion, fHeight, &regionList );

	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(1);
	GetDesigner()->Clear();
	for( int i = 0, iRegionCount(regionList.size()); i < iRegionCount; ++i )
	{
		if( GetTempRegion() )
		{
			regionList[i]->SetTexInfo(GetTempRegion()->GetTexInfo());
			regionList[i]->SetMaterialID(GetTempRegion()->GetMaterialID());
		}
		GetDesigner()->AddRegion(regionList[i],CBrushDesigner::eOpType_Add);
		if( regionList[i]->GetPlane().Normal().IsEquivalent(GetPlane().Normal(),kDesignerEpsilon) )
			m_pCapRegion = regionList[i];
	}

	CBrushDesignerBaseTool::CreateMirroredRegions(GetDesigner());
	UpdateShelf(1);
}

void CBrushDesignerCreateCylinderTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch(event)
	{
	case eNotify_OnBeginUndoRedo:
	case eNotify_OnBeginSceneSave:
		if( m_CylinderPhase == eCylinderPhase_Done )
		{
			GetIEditor()->AcceptUndo("Designer : Create a Cylinder");
			FreezeDesigner();
		}
		else
		{
			GetIEditor()->CancelUndo();
			CancelDesigner();
		}
		m_CylinderPhase = eCylinderPhase_PlaceFirstPoint;
	}
}

void CBrushDesignerCreateCylinderTool::FreezeDesigner()
{
	if( m_bIsOverOpposite )
		s_SnappingHelper.ApplyOppositeRegions(m_pCapRegion,BUtil::ePP_Pull);
	CBrushDesignerBaseTool::FreezeDesigner();
}