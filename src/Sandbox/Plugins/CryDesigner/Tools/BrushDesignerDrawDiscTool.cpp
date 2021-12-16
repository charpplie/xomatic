#include "StdAfx.h"
#include "BrushDesignerDrawDiscTool.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerEditTool.h"
#include "IBaseToolPanel.h"

namespace 
{
	ICreateSphereDiscCurveToolPanel* g_pDiscPanel = NULL;
	int g_nDiscPanelId = 0;
}

void CBrushDesignerDrawDiscTool::Enter()
{
	__super::Enter();
	SetEditMode(eEditMode_Beginning);
}

void CBrushDesignerDrawDiscTool::Leave()
{
	if( GetEditMode() == eEditMode_Done )
	{
		FreezeDesigner();
		GetIEditor()->AcceptUndo("Designer : Create a Disc");
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

void CBrushDesignerDrawDiscTool::BeginEditParams()
{
	if( !g_pDiscPanel  )
		g_pDiscPanel = CreateDiscPanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerDrawDiscTool::EndEditParams()
{
	if( g_pDiscPanel )
	{
		g_pDiscPanel->DestroyPanel();
		g_pDiscPanel = NULL;
	}
}

void CBrushDesignerDrawDiscTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	bool bKeepInitPlane = GetEditMode() == eEditMode_Editing ? true : false;
	bool bSearchAllShelves = GetEditMode() == eEditMode_Done ? true : false;
	if( !CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, bKeepInitPlane, bSearchAllShelves ) )
		return;

	if( GetEditMode() == eEditMode_Editing )
	{
		BrushVec2 vSpotPos2D = GetPlane().W2P(GetCurrentSpotPos());

		BrushFloat fRadius = (vSpotPos2D-m_vCenterOnPlane).GetLength();
		const BrushFloat kSmallestRadius = 0.05f;
		if( fRadius < kSmallestRadius )
			fRadius = kSmallestRadius;

		m_fAngle = BUtil::ComputeAnglePointedByPos( m_vCenterOnPlane, vSpotPos2D );

		UpdateDisc(fRadius,g_pDiscPanel->GetSubdivisionNum());
		g_pDiscPanel->Update(fRadius);
	}
}

void CBrushDesignerDrawDiscTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	if( GetEditMode() == eEditMode_Done )
	{
		SetEditMode(eEditMode_Beginning);
		if( GetIntermediateRegion()->IsValid() )
		{
			FreezeDesigner();
			GetIEditor()->AcceptUndo("Designer : Create a Disc");
		}
	}
	if( GetEditMode() == eEditMode_Beginning )
	{
		if( CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, false, true ) )
		{
			SetStartSpot(GetCurrentSpot());
			SetEditMode(eEditMode_Editing);
			SetPlane(GetCurrentSpot().m_Plane);
			SetTempRegion(GetCurrentSpot().m_pRegion);
			m_vCenterOnPlane = GetPlane().W2P(GetCurrentSpotPos());
			GetIEditor()->BeginUndo();
			GetDesigner()->RecordUndo("Designer : Create a Disc",GetBaseObject());
			StoreSeparateStatus();
		}
	}
	else if( GetEditMode() == eEditMode_Editing )
	{
		SetEditMode(eEditMode_Done);
		RegisterDrawnRegionToDesigner();
	}
}

bool CBrushDesignerDrawDiscTool::OnKeyDown( CViewport *view, uint32 nChar, uint32 nRepCnt, uint32 nFlags )
{
	if (nChar == VK_ESCAPE )
	{
		if( GetEditMode() == eEditMode_Editing )
		{
			CancelDesigner();
			SetEditMode(eEditMode_Beginning);
			return true;
		}
		else if( GetEditMode() == eEditMode_Done )
		{
			FreezeDesigner();
			SetEditMode(eEditMode_Beginning);
			GetIEditor()->AcceptUndo("Designer : Create a Disc");
			return true;
		}
		GetEditTool()->GoToSelectDesignerMode();
	}
	return true;
}

void CBrushDesignerDrawDiscTool::Display( DisplayContext &dc )
{
	DisplayCurrentSpot(dc);

	if( GetEditMode() == eEditMode_Editing && GetIntermediateRegion() )
		DisplayDimensionHelper(dc,GetIntermediateRegion()->GetBoundBox());
	else
		DisplayDimensionHelper(dc,1);

	if( GetStartSpot().m_pRegion )
		DisplayDimensionHelper(dc);
	if( GetEditMode() == eEditMode_Editing || GetEditMode() == eEditMode_Done )
		DrawIntermediateRegion(dc);
}

void CBrushDesignerDrawDiscTool::UpdateDisc( float fRadius, int nSubdivisionNum )
{
	if( GetEditMode() == eEditMode_Beginning )
		return;
	std::vector<BrushVec2> vertices2D;
	BUtil::MakeSectorOfCircle( fRadius, m_vCenterOnPlane, m_fAngle, BUtil::PI2, nSubdivisionNum+1, vertices2D );
	vertices2D.erase(vertices2D.begin());
	BUtil::STexInfo texInfo = GetTempRegion() ? GetTempRegion()->GetTexInfo() : GetTexInfo();
	int nMaterialID = GetTempRegion() ? GetTempRegion()->GetMaterialID() : 0;
	if( GetIntermediateRegion() )
		*GetIntermediateRegion() = CBrushRegion( vertices2D, GetPlane(), GetMatID(), &texInfo, true );
	else
		SetIntermediateRegion( new CBrushRegion( vertices2D, GetPlane(), GetMatID(), &texInfo, true ) );
	GetIntermediateRegion()->SetMaterialID(nMaterialID);
	if( GetEditMode() == eEditMode_Done )
		RegisterDrawnRegionToDesigner();
}

void CBrushDesignerDrawDiscTool::RegisterDrawnRegionToDesigner()
{
	if( !GetIntermediateRegion() )
		return;
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(1);
	GetDesigner()->Clear();
	GetDesigner()->AddRegion(GetIntermediateRegion(),CBrushDesigner::eOpType_Add);
	CBrushDesignerBaseTool::CreateMirroredRegions(GetDesigner());
	UpdateShelf(1);
}

void CBrushDesignerDrawDiscTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch(event)
	{
	case eNotify_OnBeginUndoRedo:
	case eNotify_OnBeginSceneSave:
		if( GetEditMode() == eEditMode_Done )
		{
			GetIEditor()->AcceptUndo("Designer : Create a Disc");
			FreezeDesigner();
		}
		else
		{
			GetIEditor()->CancelUndo();
			CancelDesigner();
		}
		SetEditMode(eEditMode_Beginning);
	}
}