#include "StdAfx.h"
#include "BrushDesignerRingSelectionTool.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesignerElementManager.h"

void CBrushDesignerRingSelectionTool::Enter()
{
	CUndo undo("Designer : Ring Selection");
	GetEditTool()->StoreSelectionUndo();
	RingSelection(GetMainContext());
	GetEditTool()->GoToPrevDesignerMode();
}

void CBrushDesignerRingSelectionTool::RingSelection( BUtil::SMainContext& mc )
{
	int nSelectedElementCount = mc.pSelected->GetSize();
	for( int i = 0; i < nSelectedElementCount; ++i )
	{
		if( !(*mc.pSelected)[i].IsEdge() )
			continue;
		SelectRing(mc,(*mc.pSelected)[i].GetEdge());
	}
}

void CBrushDesignerRingSelectionTool::SelectRing( BUtil::SMainContext& mc, const BrushEdge3D& inputEdge )
{
	BrushEdge3D edge(inputEdge);

	std::set<CBrushRegion::RegionPtr> usedRegions;
	std::vector<CBrushRegion::RegionPtr> startRegions;
	if( mc.pDesigner->QueryAdjacentRegionsByEdge(edge,startRegions) )
	{
		for( int i = 0, iCount(startRegions.size()); i < iCount; ++i )
		{
			CBrushRegion::RegionPtr pRegion = startRegions[i];
			while(1)
			{
				int bFoundOutNewRegions = false;
				if( !pRegion->IsOpen() && pRegion->GetVertexListSize() == 4 && pRegion->GetEdgeSize() == 4 && usedRegions.find(pRegion) == usedRegions.end() )
				{
					usedRegions.insert(pRegion);
					for( int k = 0, iEdgeCount(pRegion->GetEdgeSize()); k < iEdgeCount; ++k )
					{
						BrushEdge3D edgeInRegion = pRegion->GetEdge(k);
						if( !edgeInRegion.m_v[0].IsEquivalent(edge.m_v[0],kDesignerEpsilon) && !edgeInRegion.m_v[0].IsEquivalent(edge.m_v[1],kDesignerEpsilon) &&
							!edgeInRegion.m_v[1].IsEquivalent(edge.m_v[0],kDesignerEpsilon) && !edgeInRegion.m_v[1].IsEquivalent(edge.m_v[1],kDesignerEpsilon) )
						{
							edge = edgeInRegion;
							mc.pSelected->Add(SDesignerElement(mc.pObject,edge));
							break;
						}
					}
					bFoundOutNewRegions = true;
				}
				if(!bFoundOutNewRegions)
					break;
				std::vector<CBrushRegion::RegionPtr> adjacentRegions;
				if( mc.pDesigner->QueryAdjacentRegionsByEdge(edge,adjacentRegions) && adjacentRegions.size() == 2 )
				{
					for( int k = 0, iAdjacentRegionCount(adjacentRegions.size()); k < iAdjacentRegionCount; ++k )
					{
						if( adjacentRegions[k] != pRegion )
						{
							pRegion = adjacentRegions[k];
							break;
						}
					}
				}
			}
		}
	}
}