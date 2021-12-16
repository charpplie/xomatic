#include "StdAfx.h"
#include "BrushDesignerSelectConnectedTool.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesignerElementManager.h"

void CBrushDesignerSelectConnectedTool::Enter()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( pSelected->IsEmpty() )
	{
		AfxMessageBox("At least one element should be selected to use this selection.");
		GetEditTool()->GoToPrevDesignerMode();
		return;
	}
	CUndo undo("Designer : Selection of connected elements");
	GetEditTool()->StoreSelectionUndo();
	SelectConnectedRegions(GetMainContext());
	GetEditTool()->GoToPrevDesignerMode();
}

void CBrushDesignerSelectConnectedTool::SelectConnectedRegions( BUtil::SMainContext& mc )
{
	std::set<CBrushRegion::RegionPtr> selectedSet = MakeInitialSelectedSet(mc);
	SelectAdjacentRegionsFromEdgeVertex(mc,selectedSet,true);
	if( !selectedSet.empty() )
	{
		int nRegionCount = mc.pDesigner->GetRegionSize();
		while(SelectAdjacentRegions(mc,selectedSet,true) && selectedSet.size() < nRegionCount);
	}
}