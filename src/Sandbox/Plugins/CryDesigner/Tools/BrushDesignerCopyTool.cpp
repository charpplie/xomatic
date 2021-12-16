#include "StdAfx.h"
#include "BrushDesignerCopyTool.h"
#include "BrushDesignerSelectTool.h"
#include "Core/BrushDesigner.h"
#include "Objects/DesignerBrushObject.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesignerElementManager.h"

void CBrushDesignerCopyTool::Enter()
{
	CUndo undo("Designer : Copy a Part");

	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	GetDesigner()->SetShelf(0);
	GetDesigner()->RecordUndo("Designer : Copy a Part",GetBaseObject());

	CBrushDesignerElementManager copiedElements;
	Copy(BUtil::SMainContext(GetBaseObject(),GetBrush(),GetDesigner()),&copiedElements);

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->Clear();
	pSelected->Add(copiedElements);

	GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
	UpdateBrush();
	GetEditTool()->GoToSelectDesignerMode();
}

void CBrushDesignerCopyTool::Copy( BUtil::SMainContext& mc, CBrushDesignerElementManager* pOutCopiedElements )
{
	DESIGNER_ASSERT(pOutCopiedElements);
	if( !pOutCopiedElements )
		return;

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	std::vector<CBrushRegion::RegionPtr> selectedRegions;

	for( int i = 0, iElementCount(pSelected->GetSize()); i < iElementCount; ++i )
	{
		const SDesignerElement& elementInfo = pSelected->Get(i);
		if( !elementInfo.IsFace() )
			continue;

		DESIGNER_ASSERT(elementInfo.m_pRegion);
		if( !elementInfo.m_pRegion || !elementInfo.m_pRegion->IsValid() )
			continue;

		CBrushRegion::RegionPtr pRegion = mc.pDesigner->QueryEquivalentRegion(elementInfo.m_pRegion);
		selectedRegions.push_back(pRegion);
	}
	
	pOutCopiedElements->Clear();
	for( int i = 0, iRegionCount(selectedRegions.size()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pClone = selectedRegions[i]->Clone();
		mc.pDesigner->AddRegionUnconditionally(pClone);
		SDesignerElement ei;
		ei.SetFace(mc.pObject,pClone);
		pOutCopiedElements->Add(ei);
	}
}