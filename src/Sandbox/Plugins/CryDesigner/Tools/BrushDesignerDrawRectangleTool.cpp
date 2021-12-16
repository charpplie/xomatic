#include "StdAfx.h"
#include "BrushDesignerDrawRectangleTool.h"
#include "BrushDesignerEditTool.h"
#include "IBaseToolPanel.h"

namespace 
{
	ICreateRectangleToolPanel* g_pRectanglePanel = NULL;
}

void CBrushDesignerDrawRectangleTool::Enter()
{
	__super::Enter();
	m_Phase = eRectanglePhase_PlaceFirstPoint;
}

void CBrushDesignerDrawRectangleTool::Leave()
{
	if( m_Phase == eRectanglePhase_Done )
	{
		FreezeDesigner();
		GetIEditor()->AcceptUndo("Designer : Create a Rectangle");
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

void CBrushDesignerDrawRectangleTool::BeginEditParams()
{
	if( !g_pRectanglePanel )
		g_pRectanglePanel =  CreateRectanglePanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerDrawRectangleTool::EndEditParams()
{
	if( g_pRectanglePanel )
	{
		g_pRectanglePanel->DestroyPanel();
		g_pRectanglePanel = NULL;
	}
}

void CBrushDesignerDrawRectangleTool::OnLButtonDown( CViewport *view, UINT nFlags, CPoint point )
{
	if( m_Phase == eRectanglePhase_Done )
	{
		FreezeDesigner();
		m_Phase = eRectanglePhase_PlaceFirstPoint;
		SetIntermediateRegion(NULL);
		GetIEditor()->AcceptUndo("Designer : Create a Rectangle");
	}

	if( m_Phase == eRectanglePhase_PlaceFirstPoint )
	{
		if( CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, false, true ) )
		{
			SetPlane(GetCurrentSpot().m_Plane);
			SetStartSpot(GetCurrentSpot());
			SetTempRegion(GetCurrentSpot().m_pRegion);
			m_Phase = eRectanglePhase_DrawRectangle;
			GetIEditor()->BeginUndo();
			GetDesigner()->RecordUndo("Designer : Create a Rectangle",GetBaseObject());
			StoreSeparateStatus();
		}
	}
}

bool CBrushDesignerDrawRectangleTool::OnKeyDown( CViewport *view, uint32 nChar, uint32 nRepCnt, uint32 nFlags )
{
	if (nChar == VK_ESCAPE )
	{
		if( m_Phase == eRectanglePhase_DrawRectangle )
		{
			CancelDesigner();
			SetEditMode(eEditMode_None);
			return true;
		}
		else if( m_Phase == eRectanglePhase_Done )
		{
			FreezeDesigner();
			m_Phase = eRectanglePhase_PlaceFirstPoint;
			GetIEditor()->AcceptUndo("Designer : Create a Rectangle");
			return true;
		}
		GetEditTool()->GoToSelectDesignerMode();
	}
	return true;
}

void CBrushDesignerDrawRectangleTool::OnLButtonUp( CViewport *view, UINT nFlags, CPoint point )
{
	if( m_Phase == eRectanglePhase_DrawRectangle )
	{
		if( std::abs(GetCurrentSpotPos().x-GetStartSpotPos().x) > (BrushFloat)0.01 || 
			std::abs(GetCurrentSpotPos().y-GetStartSpotPos().y) > (BrushFloat)0.01 || 
			std::abs(GetCurrentSpotPos().z-GetStartSpotPos().z) > (BrushFloat)0.01 )
		{
			m_Phase = eRectanglePhase_Done;
			UpdateRectangle( GetStartSpotPos(), GetCurrentSpotPos(), true );
		}
	}
}

void CBrushDesignerDrawRectangleTool::OnMouseMove( CViewport *view, UINT nFlags, CPoint point )
{
	if( m_Phase == eRectanglePhase_PlaceFirstPoint || m_Phase == eRectanglePhase_Done )
	{
		UpdateCurrentSpotPosition(view,nFlags,point,false,true);
		if( m_Phase == eRectanglePhase_PlaceFirstPoint )
			SetPlane(GetCurrentSpot().m_Plane);
	}
	else if( m_Phase == eRectanglePhase_DrawRectangle )
	{
		if( (nFlags&MK_LBUTTON) && UpdateCurrentSpotPosition(view,nFlags,point,true,true) )
			UpdateRectangle( GetStartSpotPos(), GetCurrentSpotPos(), false );
	}
}

void CBrushDesignerDrawRectangleTool::UpdateRectangle( const BrushVec3& v0, const BrushVec3& v1, bool bRenderFace )
{
	CBrushRegion::RegionPtr pRegion = GetIntermediateRegion();
	if( !pRegion )
	{
		SetIntermediateRegion(new CBrushRegion);
		pRegion = GetIntermediateRegion();
	}

	std::vector<BrushVec3> vList(4);

	BrushVec2 p0 = GetPlane().W2P(v0);
	BrushVec2 p1 = GetPlane().W2P(v1);

	m_v[0] = v0;
	m_v[1] = v1;

	vList[0] = v0;
	vList[1] = GetPlane().P2W(BrushVec2(p1.x,p0.y));
	vList[2] = v1;
	vList[3] = GetPlane().P2W(BrushVec2(p0.x,p1.y));

	g_pRectanglePanel->Update(std::abs(p0.x-p1.x),std::abs(p0.y-p1.y));

	*pRegion = CBrushRegion(vList);
	if( !pRegion->GetPlane().IsSameFacing(GetPlane()) )
		pRegion->Flip();

	if( bRenderFace )
	{
		DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

		GetDesigner()->SetShelf(1);
		GetDesigner()->Clear();

		if( GetTempRegion() )
		{
			pRegion->SetTexInfo(GetTempRegion()->GetTexInfo());
			pRegion->SetMaterialID(GetTempRegion()->GetMaterialID());
		}

		if( !pRegion->IsOpen() )
			GetDesigner()->AddRegion(pRegion, CBrushDesigner::eOpType_Add);

		CBrushDesignerBaseTool::CreateMirroredRegions(GetDesigner());

		UpdateShelf(1);
	}
}

void CBrushDesignerDrawRectangleTool::Display( DisplayContext &dc )
{
	DisplayCurrentSpot(dc);

	if( m_Phase == eRectanglePhase_DrawRectangle && GetIntermediateRegion() )
		DisplayDimensionHelper(dc,GetIntermediateRegion()->GetBoundBox());
	else
		DisplayDimensionHelper(dc,1);

	if( GetStartSpot().m_pRegion )
		DisplayDimensionHelper(dc);

	if( m_Phase == eRectanglePhase_DrawRectangle || m_Phase == eRectanglePhase_Done )
		DrawIntermediateRegion(dc);
}

void CBrushDesignerDrawRectangleTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch(event)
	{
	case eNotify_OnBeginUndoRedo:
	case eNotify_OnBeginSceneSave:
		if( m_Phase == eRectanglePhase_Done )
		{
			GetIEditor()->AcceptUndo("Designer : Create a Rectangle");
			FreezeDesigner();
		}
		else
		{
			GetIEditor()->CancelUndo();
			CancelDesigner();
		}
		m_Phase = eRectanglePhase_PlaceFirstPoint;
	}
}