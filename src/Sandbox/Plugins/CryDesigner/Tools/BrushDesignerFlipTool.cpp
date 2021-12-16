#include "StdAfx.h"
#include "BrushDesignerFlipTool.h"
#include "Viewport.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesignerSmoothingGroupManager.h"

void CBrushDesignerFlipTool::Enter()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( !pSelected->IsEmpty() )
		FlipRegions();
	pSelected->Erase(BUtil::ePF_Face);
	GetEditTool()->GoToSelectDesignerMode();
}

void CBrushDesignerFlipTool::Leave()
{
	__super::Leave();
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->Set(m_FlipedSelectedElements);
}

void CBrushDesignerFlipTool::FlipRegions( BUtil::SMainContext& mc, CBrushDesignerElementManager& outFlipedElements )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	outFlipedElements.Clear();	

	for( int i = 0, iElementCount(pSelected->GetSize()); i < iElementCount; ++i )
	{
		if( !(*pSelected)[i].IsFace() )
			continue;

		CBrushRegion::RegionPtr pRegion = (*pSelected)[i].m_pRegion;
		if( pRegion == NULL )
			continue;

		pRegion->Flip();
		mc.pDesigner->GetSmoothingGroupMgr()->RemoveRegion(pRegion);
		SDesignerElement flipedElement;
		flipedElement.SetFace(mc.pObject,pRegion);
		outFlipedElements.Add(flipedElement);
	}
}

void CBrushDesignerFlipTool::FlipRegions()
{
	const CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	CUndo undo("Designer : Flip");

	GetDesigner()->RecordUndo("Flip",GetBaseObject());

	m_FlipedSelectedElements.Clear();
	FlipRegions(BUtil::SMainContext(GetBaseObject(),GetBrush(),GetDesigner()), m_FlipedSelectedElements);

	if( m_FlipedSelectedElements.IsEmpty() )
		undo.Cancel();

	if( !pSelected->IsEmpty() )
	{
		CreateMirroredRegions(GetDesigner());
		UpdateBrush();
		Sync();
		UpdateGameResource(GetBaseObject());
	}
}