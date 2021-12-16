#include "StdAfx.h"
#include "BrushDesignerRemoveTool.h"
#include "Viewport.h"
#include "BrushDesignerSelectTool.h"
#include "BrushDesignerEditTool.h"

void CBrushDesignerRemoveTool::Enter()
{
	__super::Enter();
	RemoveSelectedElements();
	GetEditTool()->GoToSelectDesignerMode();
}

bool CBrushDesignerRemoveTool::RemoveSelectedElements( BUtil::SMainContext& mc, bool bEraseMirrored )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();	

	if( pSelected->IsEmpty() )
		return false;

	int iSelectedElementCount(pSelected->GetSize());

	for( int i = 0; i < iSelectedElementCount; ++i )
	{
		if( !(*pSelected)[i].IsFace() || (*pSelected)[i].m_pRegion == NULL )
			continue;

		int nRegionIndex = -1;
		CBrushRegion::RegionPtr pRegion = mc.pDesigner->QueryEquivalentRegion((*pSelected)[i].m_pRegion,&nRegionIndex);
		if( !pRegion )
			continue;

		mc.pDesigner->DrillRegion(nRegionIndex,IsFrameRemainInRemovingFace(mc.pObject));
		if( bEraseMirrored )
			DrillMirroredRegion(mc.pDesigner,pRegion);
	}

	for( int i = 0; i < iSelectedElementCount; ++i )
	{
		if( !(*pSelected)[i].IsEdge() )
			continue;
		BrushEdge3D edge = (*pSelected)[i].GetEdge();
		if( mc.pDesigner->EraseEdge(edge) )
		{
			mc.pDesigner->EraseEdge(edge.GetInverted());
			if( bEraseMirrored )
				EraseMirroredEdge(mc.pDesigner,edge);
		}
		else
		{
			DESIGNER_ASSERT(!"Erasing an edge Failed.");
		}
	}

	pSelected->Clear();
	return true;
}

bool CBrushDesignerRemoveTool::RemoveSelectedElements()
{
	CUndo undo("CryDesigner : Remove elements");
	GetDesigner()->RecordUndo("Remove elements",GetBaseObject());

	if( RemoveSelectedElements(GetMainContext(),true) )
	{		
		if( gSettings.bDesignerKeepCenterPivot )
			GetBrush()->PivotToCenter(GetBaseObject(),GetDesigner());
		Sync();
		UpdateBrush();
		return true;
	}

	UpdateGameResource(GetBaseObject());

	return false;
}