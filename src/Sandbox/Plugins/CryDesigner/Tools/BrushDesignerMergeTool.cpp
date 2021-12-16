#include "StdAfx.h"
#include "BrushDesignerMergeTool.h"
#include "BrushDesignerEditTool.h"
#include "Objects/DesignerBrushObject.h"
#include "BrushDesignerMirrorTool.h"
#include "BrushDesignerSelectTool.h"

std::vector<CBrushRegion::RegionPtr> GetRegionList( CBrushDesignerElementManager* pElements )
{
	std::vector<CBrushRegion::RegionPtr> regionList;
	for( int i = 0, iElementCount(pElements->GetSize()); i < iElementCount; ++i )
	{
		if( (*pElements)[i].IsFace() )
			regionList.push_back((*pElements)[i].m_pRegion);
	}
	return regionList;
}

void CBrushDesignerMergeTool::Enter()
{
	std::vector<BUtil::SSelectedInfo> selections;
	GetEditTool()->GetSelectedObjectList(selections);

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	std::vector<CBrushRegion::RegionPtr> selectedRegions = GetRegionList(pSelected);

	if( selections.size() < 2 && selectedRegions.size() < 2 )
	{
		AfxMessageBox( "More than 2 designer objects or 2 faces should be selected to be merged.", MB_OK );
		GetEditTool()->SetDesignerMode(BUtil::eDesigner_ObjectMode);
		return;
	}

	CUndo undo("Designer : Object Merge");

	if( selections.size() >= 2 )
		MergeObjects();
	else if( selectedRegions.size() >= 2 )
		MergeRegions();
}

void CBrushDesignerMergeTool::MergeObjects()
{
	std::vector<BUtil::SSelectedInfo> selections;
	GetEditTool()->GetSelectedObjectList(selections);

	int nSelectionCount = selections.size();
	CDesignerBrushObject* pMergedDesignerObj = (CDesignerBrushObject*)selections[nSelectionCount-1].m_pObj;
	CBrushDesigner* pMergedDesigner = (CBrushDesigner*)selections[nSelectionCount-1].m_pDesigner;

	pMergedDesigner->RecordUndo("Designer : Merge",pMergedDesignerObj);
	pMergedDesignerObj->GetBrush()->ResetXForm(pMergedDesignerObj,pMergedDesigner);

	for( int i = 0; i < nSelectionCount-1; ++i )
	{
		BUtil::SSelectedInfo& selection = selections[i];
		if( selection.m_pDesigner->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		{
			CBrushDesignerMirrorTool::ReleaseMirrorMode(selection.m_pDesigner);
			CBrushDesignerMirrorTool::RemoveEdgesOnMirrorPlane(selection.m_pDesigner);
		}
		pMergedDesignerObj->Merge( selection.m_pObj, selection.m_pDesigner );
	}

	pMergedDesignerObj->UpdateBrush();
	pMergedDesignerObj->GetDesigner()->ResetDB(BUtil::eDBRF_ALL);

	for( int i = 0; i < nSelectionCount-1; ++i )
		GetIEditor()->DeleteObject(selections[i].m_pObj);

	GetIEditor()->SelectObject(pMergedDesignerObj);

	pMergedDesignerObj->SwitchToDesignerEditTool();
}

void CBrushDesignerMergeTool::MergeRegions( BUtil::SMainContext& mc )
{
	std::vector<CBrushRegion::RegionPtr> selectedRegions = GetRegionList(GetEditTool()->GetSelectedElements());
	int nSelectedRegionCount = selectedRegions.size();

	std::set<CBrushRegion::RegionPtr> usedRegions;

	while( usedRegions.size() < nSelectedRegionCount )
	{
		CBrushRegion::RegionPtr pRegion = NULL;
		for( int i = 0; i < nSelectedRegionCount; ++i )
		{
			if( usedRegions.find(selectedRegions[i]) == usedRegions.end() )
			{
				pRegion = selectedRegions[i];
				break;
			}
		}

		if( pRegion == NULL )
			break;

		bool bMerged = false;
		for( int i = 0; i < nSelectedRegionCount; ++i )
		{
			if( pRegion == selectedRegions[i] || usedRegions.find(selectedRegions[i]) != usedRegions.end() )
				continue;

			if( BUtil::eIT_None == CBrushRegion::HasIntersection(pRegion,selectedRegions[i]) || pRegion->GetMaterialID() != selectedRegions[i]->GetMaterialID() )
				continue;

			pRegion->Union(selectedRegions[i]);
			mc.pDesigner->RemoveRegion(selectedRegions[i]);
			usedRegions.insert(selectedRegions[i]);
			bMerged = true;
		}

		if( !bMerged )
			usedRegions.insert(pRegion);
	}
}

void CBrushDesignerMergeTool::MergeRegions()
{
	GetDesigner()->RecordUndo("Designer : Merge Regions",GetBaseObject());
	MergeRegions(BUtil::SMainContext(GetBaseObject(),GetBrush(),GetDesigner()));
	UpdateBrush();
}