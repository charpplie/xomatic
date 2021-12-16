#include "StdAfx.h"
#include "BrushDesignerSelectGrowTool.h"
#include "BrushDesignerEditTool.h"
#include "BrushDesignerSelectGrowTool.h"
#include "Core/BrushDesignerElementManager.h"

void CBrushDesignerSelectGrowTool::Enter()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( pSelected->IsEmpty() )
	{
		AfxMessageBox("At least one element should be selected to use this selection.");
		GetEditTool()->GoToPrevDesignerMode();
		return;
	}
	CUndo undo("Designer : Growing Selection");
	GetEditTool()->StoreSelectionUndo();
	GrowSelection(GetMainContext());
	GetEditTool()->GoToPrevDesignerMode();
}

void CBrushDesignerSelectGrowTool::GrowSelection( BUtil::SMainContext& mc )
{
	std::set<CBrushRegion::RegionPtr> selectedSet = MakeInitialSelectedSet(mc);
	SelectAdjacentRegionsFromEdgeVertex(mc,selectedSet,false);
	if( !selectedSet.empty() )
		SelectAdjacentRegions(mc,selectedSet,false);
}

std::set<CBrushRegion::RegionPtr> CBrushDesignerSelectGrowTool::MakeInitialSelectedSet( BUtil::SMainContext& mc )
{
	std::set<CBrushRegion::RegionPtr> selectedSet;
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( pSelected->IsEmpty() )
		return selectedSet;

	for( int i = 0, iSelectionCount(pSelected->GetSize()); i < iSelectionCount; ++i )
	{
		if( !(*pSelected)[i].IsFace() || (*pSelected)[i].m_pRegion == NULL ) 
			continue;
		selectedSet.insert((*pSelected)[i].m_pRegion);
	}

	return selectedSet;
}

void CBrushDesignerSelectGrowTool::SelectAdjacentRegionsFromEdgeVertex( BUtil::SMainContext& mc, std::set<CBrushRegion::RegionPtr>& selectedSet, bool bAddNewSelections )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();	
	if( pSelected->IsEmpty() )
		return;

	for( int i = 0, iSelectionCount(pSelected->GetSize()); i < iSelectionCount; ++i )
	{
		if( !(*pSelected)[i].IsVertex() && !(*pSelected)[i].IsEdge() )
			continue;

		CBrushDesignerDB::QueryResult qResult;
		for( int k = 0, iElementCount((*pSelected)[i].m_Vertices.size()); k < iElementCount; ++k )
			mc.pDesigner->GetDB()->QueryAsVertex((*pSelected)[i].m_Vertices[k],qResult);

		for( int k = 0, iQueryCount(qResult.size()); k < iQueryCount; ++k )
		{
			for( int a=0,iMarkCount(qResult[k].m_MarkList.size()); a < iMarkCount; ++a )
			{
				CBrushRegion::RegionPtr pRegion = qResult[k].m_MarkList[a].m_pRegion;
				SDesignerElement de;
				de.SetFace(mc.pObject, pRegion);
				pSelected->Add(de);
				if( bAddNewSelections && (*pSelected)[i].m_pRegion )
					selectedSet.insert((*pSelected)[i].m_pRegion);
			}
		}
	}
}

bool CBrushDesignerSelectGrowTool::SelectAdjacentRegions( BUtil::SMainContext& mc, std::set<CBrushRegion::RegionPtr>& selectedSet, bool bAddNewSelections )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	bool bAddedNewAdjacent = false;
	for( int i = 0, nRegionCount(mc.pDesigner->GetRegionSize()); i < nRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = mc.pDesigner->GetRegion(i);
		DESIGNER_ASSERT(pRegion);
		if( !pRegion || selectedSet.find(pRegion) != selectedSet.end() )
			continue;

		bool bAdjacent = false;
		std::set<CBrushRegion::RegionPtr>::iterator ii = selectedSet.begin();
		for( ; ii != selectedSet.end(); ++ii )
		{
			if( pRegion->HasOverlappedEdges(*ii) )
			{
				bAdjacent = true;
				break;
			}
		}
		if( !bAdjacent )
			continue;

		pSelected->Add(SDesignerElement(mc.pObject,pRegion));
		if( bAddNewSelections )
			selectedSet.insert(pRegion);

		bAddedNewAdjacent = true;
	}

	return bAddedNewAdjacent;
}