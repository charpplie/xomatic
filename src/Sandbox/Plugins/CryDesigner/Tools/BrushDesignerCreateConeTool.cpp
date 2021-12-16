#include "StdAfx.h"
#include "BrushDesignerCreateConeTool.h"
#include "Core/BrushDesigner.h"
#include "Core/BrushPrimitive.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesignerAdjustHeightHelper.h"
#include "ViewManager.h"
#include "IBaseToolPanel.h"

namespace 
{
	ICreateCylinderConeToolPanel* g_pConeToolPanel;
}

void CBrushDesignerCreateConeTool::Enter()
{
	__super::Enter();
	m_ConePhase = eConePhase_PlaceFirstPoint;
}

void CBrushDesignerCreateConeTool::Leave()
{
	if( m_ConePhase == eConePhase_Done )
	{
		CBrushDesignerBaseTool::FreezeDesigner();
		GetIEditor()->AcceptUndo("Designer : Create a Cone");
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

void CBrushDesignerCreateConeTool::BeginEditParams()
{
	if( !g_pConeToolPanel )
		g_pConeToolPanel = CreateConePanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerCreateConeTool::EndEditParams()
{
	if( g_pConeToolPanel )
	{
		g_pConeToolPanel->DestroyPanel();
		g_pConeToolPanel = NULL;
	}
}

void CBrushDesignerCreateConeTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	bool bKeepInitPlane = m_ConePhase == eConePhase_Radius ? true : false;
	bool bSearchAllShelves = m_ConePhase == eConePhase_Done ? true : false;
	if( !CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, bKeepInitPlane, bSearchAllShelves ) )
		return;

	if( m_ConePhase == eConePhase_Radius )
	{
		BrushVec2 vSpotPos2D = GetPlane().W2P(GetCurrentSpotPos());
		BrushFloat fRadius = (vSpotPos2D-m_vCenterOnPlane).GetLength();
		const BrushFloat kSmallestRadius = 0.05f;
		if( fRadius < kSmallestRadius )
			fRadius = kSmallestRadius;
		m_fAngle = BUtil::ComputeAnglePointedByPos( m_vCenterOnPlane, vSpotPos2D );
		UpdateBaseRegion( fRadius, g_pConeToolPanel->GetSubdivisionNum());
		g_pConeToolPanel->Update( fRadius, 0.01f );
	}
	else if( m_ConePhase == eConePhase_RaiseHeight )
	{
		BrushFloat fHeight = s_AdjustHeightHelper.UpdateHeight(GetWorldTM(), view, point);
		if( fHeight < (BrushFloat)0.01 )
			fHeight = (BrushFloat)0.01;
		UpdateCone( fHeight );
		g_pConeToolPanel->Update( g_pConeToolPanel->GetRadius(), (float)fHeight );
	}
}

void CBrushDesignerCreateConeTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_ConePhase == eConePhase_Done )
	{
		m_ConePhase = eConePhase_PlaceFirstPoint;
		CBrushDesignerBaseTool::FreezeDesigner();
		GetIEditor()->AcceptUndo("Designer : Create a Cone");
	}

	if( m_ConePhase == eConePhase_PlaceFirstPoint )
	{
		if( CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, false, true ) )
		{
			SetStartSpot(GetCurrentSpot());
			m_ConePhase = eConePhase_Radius;
			SetPlane(GetCurrentSpot().m_Plane);
			m_vCenterOnPlane = GetPlane().W2P(GetCurrentSpotPos());
			s_AdjustHeightHelper.Init(GetPlane(),GetCurrentSpotPos());
			GetIEditor()->BeginUndo();
			GetDesigner()->RecordUndo("Designer : Create a Cone",GetBaseObject());
			SetTempRegion(GetCurrentSpot().m_pRegion);
			StoreSeparateStatus();
		}
	}
	else if( m_ConePhase == eConePhase_Radius )
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
		m_ConePhase = eConePhase_RaiseHeight;
	}
	else if( m_ConePhase == eConePhase_RaiseHeight )
	{
		m_ConePhase = eConePhase_Done;
	}
}

bool CBrushDesignerCreateConeTool::OnKeyDown( CViewport *view, uint32 nChar, uint32 nRepCnt, uint32 nFlags )
{
	if (nChar == VK_ESCAPE )
	{
		if( m_ConePhase == eConePhase_Radius || m_ConePhase == eConePhase_RaiseHeight )
		{
			CancelDesigner();
			m_ConePhase = eConePhase_PlaceFirstPoint;
			return true;
		}
		else if( m_ConePhase == eConePhase_Done )
		{
			CBrushDesignerBaseTool::FreezeDesigner();
			m_ConePhase = eConePhase_PlaceFirstPoint;
			GetIEditor()->AcceptUndo("Designer : Create a Cone");
			return true;
		}
		GetEditTool()->GoToSelectDesignerMode();
	}
	return true;
}

void CBrushDesignerCreateConeTool::Display( DisplayContext &dc )
{
	DisplayCurrentSpot(dc);

	if( m_ConePhase == eConePhase_Radius || m_ConePhase == eConePhase_RaiseHeight || m_ConePhase == eConePhase_Done )
		DrawIntermediateRegion(dc);

	if( m_ConePhase == eConePhase_Radius && GetIntermediateRegion() )
		DisplayDimensionHelper(dc,GetIntermediateRegion()->GetBoundBox());
	else
		DisplayDimensionHelper(dc,1);

	if( GetStartSpot().m_pRegion )
		DisplayDimensionHelper(dc);

	s_AdjustHeightHelper.Display(dc);
}

void CBrushDesignerCreateConeTool::UpdateBaseRegion( float fRadius, int nNumOfSubdivision )
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

void CBrushDesignerCreateConeTool::UpdateAll( float fRadius, float fHeight, int nSubDivisionNum )
{
	UpdateBaseRegion( fRadius, nSubDivisionNum );
	UpdateCone( fHeight );
}

void CBrushDesignerCreateConeTool::UpdateCone( float fHeight )
{
	if( m_ConePhase == eConePhase_PlaceFirstPoint )
		return;

	DESIGNER_ASSERT(m_pBaseRegion);
	if( !m_pBaseRegion )
		return;

	std::vector<CBrushRegion::RegionPtr> regionList;
	CBrushPrimitive bp(NULL);
	bp.CreateCone( m_pBaseRegion, fHeight, &regionList );

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
	}

	CBrushDesignerBaseTool::CreateMirroredRegions(GetDesigner());
	UpdateShelf(1);
}

void CBrushDesignerCreateConeTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch(event)
	{
	case eNotify_OnBeginUndoRedo:
	case eNotify_OnBeginSceneSave:
		if( m_ConePhase == eConePhase_Done )
		{
			GetIEditor()->AcceptUndo("Designer : Create a Cone");
			CBrushDesignerBaseTool::FreezeDesigner();
		}
		else
		{
			GetIEditor()->CancelUndo();
			CancelDesigner();
		}
		m_ConePhase = eConePhase_PlaceFirstPoint;
	}
}