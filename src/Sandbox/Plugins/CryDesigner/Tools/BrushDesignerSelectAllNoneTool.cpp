#include "StdAfx.h"
#include "BrushDesignerSelectAllNoneTool.h"
#include "BrushDesignerEditTool.h"
#include "BrushDesignerSelectTool.h"

void CBrushDesignerSelectAllNoneTool::Enter()
{
	CUndo undo("Designer : All or None Selection");
	GetEditTool()->StoreSelectionUndo();

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	bool bSelectedElementsExist = !pSelected->IsEmpty();
	pSelected->Clear();

	if( !bSelectedElementsExist )
	{
		if( GetEditTool()->GetPrevDesignerMode() & BUtil::eDesigner_Select_Edge )
			SelectAllEdges(GetBaseObject(),GetDesigner());
		else if( GetEditTool()->GetPrevDesignerMode() & BUtil::eDesigner_Select_Vertex )
			SelectAllVertices(GetBaseObject(),GetDesigner());
		else
			SelectAllFaces(GetBaseObject(),GetDesigner());
	}

	CBrushDesignerSelectTool::ResetDesignerRejectedEdgeList(GetMainContext());
	UpdateTMManipulatorBasedOnElements(pSelected);
	GetEditTool()->GoToPrevDesignerMode();
}

void CBrushDesignerSelectAllNoneTool::SelectAllVertices( CBaseObject* pObject, CBrushDesigner* pDesigner )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
		for( int k = 0, iVertexCount(pRegion->GetVertexListSize()); k < iVertexCount; ++k )
			pSelected->Add(SDesignerElement(pObject,pRegion->GetVertex(k)));
	}
}

void CBrushDesignerSelectAllNoneTool::SelectAllEdges( CBaseObject* pObject, CBrushDesigner* pDesigner )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
		for( int k = 0, iEdgeCount(pRegion->GetEdgeSize()); k < iEdgeCount; ++k )
		{
			BrushEdge3D e = pRegion->GetEdge(k);
			pSelected->Add(SDesignerElement(pObject,e));
		}
	}
}

void CBrushDesignerSelectAllNoneTool::SelectAllFaces( CBaseObject* pObject, CBrushDesigner* pDesigner )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
		pSelected->Add(SDesignerElement(pObject,pRegion));
	}
}

void CBrushDesignerSelectAllNoneTool::DeselectAllVertices()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->Erase(BUtil::ePF_Vertex);
}

void CBrushDesignerSelectAllNoneTool::DeselectAllEdges()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->Erase(BUtil::ePF_Edge);
}

void CBrushDesignerSelectAllNoneTool::DeselectAllFaces()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->Erase(BUtil::ePF_Face);
}