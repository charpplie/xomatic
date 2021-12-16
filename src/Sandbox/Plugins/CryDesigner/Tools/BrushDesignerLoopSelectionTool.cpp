#include "StdAfx.h"
#include "BrushDesignerLoopSelectionTool.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesignerElementManager.h"

namespace
{
	int s_nEdgeCount = 0;
}

void CBrushDesignerLoopSelectionTool::LoopSelection( BUtil::SMainContext& mc )
{	
	CBrushDesignerElementManager copiedSelected = *mc.pSelected;
	int nSelectedElementCount = copiedSelected.GetSize();
	mc.pSelected->Clear();
	s_nEdgeCount = CountAllEdges(mc);

	for( int i = 0; i < nSelectedElementCount; ++i )
	{
		if( !copiedSelected[i].IsEdge() )
			continue;
		if( !SelectLoop(mc,copiedSelected[i].GetEdge()) )
		{
			mc.pSelected->Clear();

			if( SelectBorderInOneRegion(mc,copiedSelected[i].GetEdge()) )
				continue;

			CBrushDesignerElementManager elements;
			if( SelectBorder(mc,copiedSelected[i].GetEdge(),elements) )
			{
				mc.pSelected->Add(elements);
				mc.pSelected->Add(SDesignerElement(mc.pObject,copiedSelected[i].GetEdge()));
			}
		}		
	}
}

void AddEdgeToList( std::vector<BrushEdge3D>& edgeList, const BrushEdge3D& edge )
{
	BrushEdge3D invEdge = edge.GetInverted();

	for( int i = 0, iEdgeCount(edgeList.size()); i < iEdgeCount; ++i )
	{
		if( edgeList[i].IsEquivalent(edge,kDesignerEpsilon) || edgeList[i].IsEquivalent(invEdge,kDesignerEpsilon) )
			return;
	}

	edgeList.push_back(edge);
}

void CBrushDesignerLoopSelectionTool::Enter()
{
	LoopSelection(GetMainContext());
	GetEditTool()->GoToPrevDesignerMode();
}

bool CBrushDesignerLoopSelectionTool::SelectLoop( BUtil::SMainContext& mc, const BrushEdge3D& initialEdge )
{
	BrushEdge3D edge = initialEdge;
	CBrushDesignerElementManager edgeElements;

	int nCount = 0;

	//First check to see if the selected element connects to only 3 other edges.
	//If the edge in question has already been added to the list, the selection ends.
	//Of the 3 edges that connect to the current edge, the ones that share a face with the current edge are eliminated and the remaining edge is added to the list and is made the current edge. 
	for( int i = 0; i < 2; ++i )
	{
		edge = initialEdge;
		bool bMeetFirst = false;
		bool bFoundNext = true;

		while(!bMeetFirst && bFoundNext)
		{
			if( nCount++ > s_nEdgeCount )
			{
				DESIGNER_ASSERT(0);
				break;
			}

			CBrushDesignerDB::QueryResult q;
			mc.pDesigner->GetDB()->QueryAsVertex(edge.m_v[i],q);

			if( q.size() != 1 )
				return false;

			std::vector<BrushEdge3D> linkedEdges;
			std::vector<CBrushRegion::RegionPtr> regionsSharingEdge;

			for( int k = 0, iMarkSize(q[0].m_MarkList.size()); k < iMarkSize; ++k )
			{
				CBrushRegion::RegionPtr pRegion = q[0].m_MarkList[k].m_pRegion;
				if( pRegion->HasEdge(edge) )
				{
					regionsSharingEdge.push_back(pRegion);
					continue;
				}

				std::vector<int> edgeIndices;
				if( !pRegion->QueryEdgesHavingVertex(edge.m_v[i],edgeIndices) )
					continue;
				for( int a = 0, iEdgeIndexCount(edgeIndices.size()); a < iEdgeIndexCount; ++a )
				{
					BrushEdge3D edgeInRegion = pRegion->GetEdge(edgeIndices[a]);
					if( edge.m_v[i].IsEquivalent(edgeInRegion.m_v[i]) )
						std::swap(edgeInRegion.m_v[0],edgeInRegion.m_v[1]);
					AddEdgeToList(linkedEdges,edgeInRegion);
				}
			}

			if( linkedEdges.size() != 3 || regionsSharingEdge.size() != 2 )
				break;

			bFoundNext = false;
			for( int k = 0, iLinkedEdgeCount(linkedEdges.size()); k < iLinkedEdgeCount; ++k )
			{
				if( !regionsSharingEdge[0]->HasEdge(linkedEdges[k]) && !regionsSharingEdge[1]->HasEdge(linkedEdges[k]) )
				{
					bMeetFirst = linkedEdges[k].m_v[i].IsEquivalent(initialEdge.m_v[1-i],kDesignerEpsilon);
					bFoundNext = true;
					edge = linkedEdges[k];
					SDesignerElement ei;
					ei.m_Vertices.push_back(linkedEdges[k].m_v[0]);
					ei.m_Vertices.push_back(linkedEdges[k].m_v[1]);
					ei.m_pObject = mc.pObject;
					ei.m_pRegion = regionsSharingEdge[0];
					edgeElements.Add(ei);
					break;
				}
			}
		}
		if( bMeetFirst )
			break;
	}

	if( edgeElements.IsEmpty() )
		return false;

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->Add(edgeElements);
	pSelected->Add(SDesignerElement(mc.pObject,initialEdge));

	return true;
}

bool CBrushDesignerLoopSelectionTool::SelectBorderInOneRegion( BUtil::SMainContext& mc, const BrushEdge3D& edge )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	std::vector<CBrushRegion::RegionPtr> adjacentRegions;
	mc.pDesigner->QueryAdjacentRegionsByEdge(edge,adjacentRegions);

	if( adjacentRegions.size() == 1 )
	{
		std::vector<CBrushRegion::RegionPtr> outerRegions;	
		adjacentRegions[0]->GetSeparatedRegions(outerRegions,CBrushRegion::eSR_OuterHull);
		for( int i = 0, outerRegionCount(outerRegions.size()); i < outerRegionCount; ++i )
		{
			if( !outerRegions[i]->HasEdge(edge) )
				continue;
			int nAddedCount = 0;
			int iEdgeCount(outerRegions[i]->GetEdgeSize());
			for( int k = 0; k < iEdgeCount; ++k )
			{
				BrushEdge3D e = outerRegions[i]->GetEdge(k);
				if( e.IsEquivalent(edge,kDesignerEpsilon) )
					continue;
				if( GetRegionCountSharingEdge(mc,e) == 1 )
				{
					++nAddedCount;
					pSelected->Add(SDesignerElement(mc.pObject,e));
				}
			}
			if( nAddedCount != iEdgeCount )
			{
				pSelected->Clear();
				return false;
			}
			return true;
		}
	}

	for( int a = 0, iAdjacentRegionCount(adjacentRegions.size()); a < iAdjacentRegionCount; ++a )
	{
		std::vector<CBrushRegion::RegionPtr> innterRegions;
		adjacentRegions[a]->GetSeparatedRegions(innterRegions,CBrushRegion::eSR_InnerHull);
		for( int i = 0, innerRegionCount(innterRegions.size()); i < innerRegionCount; ++i )
		{
			if( !innterRegions[i]->HasEdge(edge) )
				continue;
			for( int k = 0, iEdgeCount(innterRegions[i]->GetEdgeSize()); k < iEdgeCount; ++k )
			{
				BrushEdge3D e = innterRegions[i]->GetEdge(k);
				if( GetRegionCountSharingEdge(mc,e,&(innterRegions[i]->GetPlane())) == 1 )
					pSelected->Add(SDesignerElement(mc.pObject,e));
			}
			return true;
		}
	}

	return false;
}

bool CBrushDesignerLoopSelectionTool::SelectBorder( BUtil::SMainContext& mc, const BrushEdge3D& edge, CBrushDesignerElementManager& outElementInfos )
{
	BrushEdge3D e(edge);
	int nCount = 0;

	while(nCount++<s_nEdgeCount)
	{
		std::vector<BrushEdge3D> edgeList;
		mc.pDesigner->QueryEdgesHavingVertex(e.m_v[1],edgeList);

		bool bAdded = false;
		for( int i = 0, iEdgeCount(edgeList.size()); i < iEdgeCount; ++i )
		{
			if( e.IsEquivalent(edgeList[i],kDesignerEpsilon) || e.GetInverted().IsEquivalent(edgeList[i],kDesignerEpsilon) )
				continue;
			if( GetRegionCountSharingEdge(mc,edgeList[i]) != 1 )
				continue;
			outElementInfos.Add(SDesignerElement(mc.pObject,edgeList[i]));
			e = edgeList[i];
			bAdded = true;
			break;
		}
		if( !bAdded )
			break;

		if( e.m_v[1].IsEquivalent(edge.m_v[0],kDesignerEpsilon) )
			return true;
	}

	return false;
}

int CBrushDesignerLoopSelectionTool::GetRegionCountSharingEdge( BUtil::SMainContext& mc, const BrushEdge3D& edge, const BrushPlane* pPlane )
{
	std::vector<CBrushRegion::RegionPtr> regions;
	mc.pDesigner->QueryAdjacentRegionsByEdge(edge,regions);
	int nCount = 0;
	for( int i = 0, iCount(regions.size()); i < iCount; ++i )
	{
		if( !pPlane || regions[i]->GetPlane().IsEquivalent(*pPlane,kDesignerEpsilon) )
			++nCount;
	}
	return nCount;
}

int CBrushDesignerLoopSelectionTool::CountAllEdges( BUtil::SMainContext& mc )
{
	int nEdgeCount = 0;	
	for( int i = 0, iRegionCount(mc.pDesigner->GetRegionSize()); i < iRegionCount; ++i )
		nEdgeCount += mc.pDesigner->GetRegion(i)->GetEdgeSize();
	return nEdgeCount;
}