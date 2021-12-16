#include "StdAfx.h"
#include "BrushDesignerDrawTool.h." 
#include "BrushDesignerEditTool.h"
#include "ViewManager.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerResetXFormTool.h"
#include "BrushDesignerSelectTool.h"
#include "Core/BrushDesignerSmoothingGroupManager.h"

void CBrushDesignerDrawTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_EditMode != eEditMode_None )
		return;

	SetStartSpot(GetCurrentSpot());
	m_EditMode = eEditMode_Beginning;
}

void CBrushDesignerDrawTool::OnLButtonUp( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_EditMode != eEditMode_Editing || GetIntermediateRegion() == NULL )
		return;

	if( GetIntermediateRegion()->IsValid() )
	{
		CUndo undo("Designer : Draw a Shape");
		GetDesigner()->RecordUndo("Draw a Shape",GetBaseObject());

		CBrushDesigner::EOperationType opType = CBrushDesigner::eOpType_Split;
		if( nFlags & MK_CONTROL )
			opType = CBrushDesigner::eOpType_Union;

		GetDesigner()->AddRegion( GetIntermediateRegion(), opType );
		GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
		UpdateMirroredPartWithPlane( GetDesigner(), GetIntermediateRegion()->GetPlane() );
		UpdateBrush();

		Sync();

		GetIntermediateRegion()->Init();
		m_EditMode = eEditMode_None;
	}
}

bool CBrushDesignerDrawTool::UpdateCurrentSpotPosition( CViewport *view, UINT nFlags, CPoint point, bool bKeepInitialPlane, bool bSearchAllShelves )
{
	if( nFlags & MK_SHIFT )
		EnableMagnetic(false);
	else
		EnableMagnetic(true);

	if( !CBrushDesignerSpotManager::UpdateCurrentSpotPosition( GetDesigner(), GetBaseObject()->GetWorldTM(), GetPlane(), view, point, bKeepInitialPlane, bSearchAllShelves ) )
	{
		UpdateSelectionMesh(NULL,GetBrush(),GetBaseObject());
		return false;
	}

	UpdateSelectionMesh(GetCurrentSpot().m_pRegion,GetBrush(),GetBaseObject());
	SetPickedRegion(GetCurrentSpot().m_pRegion);

	return true;
}

void CBrushDesignerDrawTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	if( !CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, false ) )
	{
		UpdateSelectionMesh(NULL,GetBrush(),GetBaseObject());
		return;
	}

	SetPlane(GetCurrentSpot().m_Plane);

	if( m_EditMode == eEditMode_Beginning )
		m_EditMode = eEditMode_Editing;
}

bool CBrushDesignerDrawTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if (nChar == VK_ESCAPE )
	{
		if( m_EditMode == eEditMode_Beginning || m_EditMode == eEditMode_Editing )
			m_EditMode = eEditMode_None;
		else
			GetEditTool()->GoToSelectDesignerMode();
	}

	return true;
}

void CBrushDesignerDrawTool::MakeRectangle( const BrushPlane& plane, const BrushVec2& startPos, const BrushVec2& endPos, std::vector<BrushVec3>& outVertices )
{
	outVertices.reserve(4);
	outVertices.push_back( plane.P2W(BrushVec2(startPos.x,startPos.y)) );
	outVertices.push_back( plane.P2W(BrushVec2(startPos.x,endPos.y)) );
	outVertices.push_back( plane.P2W(BrushVec2(endPos.x,endPos.y)) );
	outVertices.push_back( plane.P2W(BrushVec2(endPos.x,startPos.y)) );
}

bool CBrushDesignerDrawTool::IsPointInTriangle( const BrushVec2& vTriV0, const BrushVec2& vTriV1, const BrushVec2& vTriV2, const BrushVec2& vPoint )
{
	BrushLine vLine0(vTriV1, vTriV0);
	BrushLine vLine1(vTriV2, vTriV1);
	BrushLine vLine2(vTriV0, vTriV2);

	return vLine0.Distance(vPoint) < kDesignerEpsilon && vLine1.Distance(vPoint) < kDesignerEpsilon && vLine2.Distance(vPoint) < kDesignerEpsilon ||
		vLine0.Distance(vPoint) > -kDesignerEpsilon && vLine1.Distance(vPoint) > -kDesignerEpsilon && vLine2.Distance(vPoint) > -kDesignerEpsilon;
}

void CBrushDesignerDrawTool::Display( DisplayContext &dc )
{
	__super::Display(dc);

	DisplayCurrentSpot(dc);
	if( m_EditMode == eEditMode_Editing || m_EditMode == eEditMode_Done )
		DrawIntermediateRegion(dc);
}

void CBrushDesignerDrawTool::DisplayCurrentSpot( DisplayContext &dc )
{
	dc.SetFillMode( e_FillModeSolid );
	DrawCurrentSpot(dc,GetWorldTM());
}

void CBrushDesignerDrawTool::DrawIntermediateRegion( DisplayContext &dc )
{
	int oldThickness = dc.GetLineWidth();
	dc.SetLineWidth(BUtil::kLineThickness);
	dc.SetColor(BUtil::RegionLineColor);

	if( GetIntermediateRegion() )
		GetIntermediateRegion()->Display(dc);

	dc.SetLineWidth(oldThickness);
}

BUtil::STexInfo CBrushDesignerDrawTool::GetTexInfo() const
{
	if( GetPickedRegion() )
		return GetPickedRegion()->GetTexInfo();
	return BUtil::STexInfo();
}

int CBrushDesignerDrawTool::GetMatID() const
{
	if( GetPickedRegion() )
		return GetPickedRegion()->GetMaterialID();
	return 0;
}

void CBrushDesignerDrawTool::UpdateDrawnRegion( const BrushVec2& p0, const BrushVec2& p1 )
{
	BrushVec2 sp0(p0),sp1(p1);
	for( int i = 0; i < 2; ++i )
	{
		if( sp0[i] > sp1[i] )
			std::swap(sp0[i], sp1[i]);
	}
	std::vector<BrushVec3> vertices;
	MakeRectangle( GetPlane(), sp0, sp1, vertices );
	BUtil::STexInfo texInfo = GetTexInfo();

	if( m_pIntermediateRegion )
		*m_pIntermediateRegion = CBrushRegion(vertices,GetPlane(),GetMatID(),&texInfo,true);
	else
		SetIntermediateRegion( new CBrushRegion(vertices,GetPlane(),GetMatID(),&texInfo,true) );
}


void CBrushDesignerDrawTool::FreezeDesigner()
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	Separate1stStep();

	bool bEmptyDesigner = GetDesigner()->IsEmpty(0);

	GetDesigner()->SetShelf(1);
	GetDesigner()->Clear();

	if( GetIntermediateRegion() )
	{
		GetDesigner()->SetShelf(0);
		if( bEmptyDesigner )
			GetDesigner()->AddRegion( GetIntermediateRegion()->Clone(), CBrushDesigner::eOpType_Split );
 		else
 			GetDesigner()->AddRegion( GetIntermediateRegion(), CBrushDesigner::eOpType_Split );
		UpdateMirroredPartWithPlane( GetDesigner(), GetIntermediateRegion()->GetPlane() );
	}

	if( bEmptyDesigner || gSettings.bDesignerKeepCenterPivot )
		GetBrush()->PivotToCenter(GetBaseObject(),GetDesigner());

	GetDesigner()->GetSmoothingGroupMgr()->InvalidateAll();
	GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
	UpdateBrush();

	if( !bEmptyDesigner && IsSeparateStatus() )
	{
		Separate2ndStep();
	}
	else
	{
		CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
		pSelected->Clear();
	}

	if( GetIntermediateRegion() )
		GetIntermediateRegion()->Init();
	UpdateGameResource(GetBaseObject());
}