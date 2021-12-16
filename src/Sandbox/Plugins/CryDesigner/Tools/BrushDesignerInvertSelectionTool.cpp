#include "StdAfx.h"
#include "BrushDesignerInvertSelectionTool.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesignerElementManager.h"
#include "BrushDesignerSelectTool.h"

void CBrushDesignerInvertSelectionTool::InvertSelection( BUtil::SMainContext& mc )
{
	int nSelectedElementCount = mc.pSelected->GetSize();
	CBrushDesignerElementManager newSelectionList;
	for( int k = 0, iRegionCount(mc.pDesigner->GetRegionSize()); k < iRegionCount; ++k )
	{
		bool bSameExist = false;
		CBrushRegion::RegionPtr pRegion = mc.pDesigner->GetRegion(k);
		for( int i = 0; i < nSelectedElementCount; ++i )
		{
			if( !(*mc.pSelected)[i].IsFace() || (*mc.pSelected)[i].m_pRegion == NULL )
				continue;
			if( (*mc.pSelected)[i].m_pRegion == pRegion )
			{
				bSameExist = true;
				break;
			}
		}
		if( !bSameExist )
		{
			SDesignerElement de;
			de.SetFace( mc.pObject, pRegion );
			newSelectionList.Add(de);
		}
	}

	mc.pSelected->Clear();
	mc.pSelected->Add(newSelectionList);	
}

void CBrushDesignerInvertSelectionTool::Enter()
{
	CUndo undo("Designer : Invert Selection");
	GetEditTool()->StoreSelectionUndo();
	InvertSelection(GetMainContext());
	CBrushDesignerSelectTool::UpdateSelectionMeshFromSelectedElementList(GetMainContext());
	GetEditTool()->GoToPrevDesignerMode();
}