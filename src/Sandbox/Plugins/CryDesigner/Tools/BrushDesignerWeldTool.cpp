#include "StdAfx.h"
#include "BrushDesignerWeldTool.h"
#include "BrushDesignerSelectTool.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesignerPolygonDecomposer.h"

void CBrushDesignerWeldTool::Weld( BUtil::SMainContext& mc, const BrushVec3& vSrc, const BrushVec3& vTarget )
{
	BrushEdge3D e(vSrc,vTarget);
	std::vector<CBrushRegion::RegionPtr> unnecessaryRegionList;
	std::vector<CBrushRegion::RegionPtr> newRegionList;
	for( int i = 0, iRegionCount(mc.pDesigner->GetRegionSize()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = mc.pDesigner->GetRegion(i);
		DESIGNER_ASSERT(pRegion);
		if( !pRegion )
			continue;

		int nVertexIndex = -1;
		int nEdgeIndex = -1;
		if( pRegion->HasEdge(e,false,&nEdgeIndex) )
		{
			BUtil::SEdge edgeIndexPair = pRegion->GetEdgeIndexPair(nEdgeIndex);
			BrushVec3 v0 = pRegion->GetVertex(edgeIndexPair.m_i[0]);
			BrushVec3 v1 = pRegion->GetVertex(edgeIndexPair.m_i[1]);
			if( e.m_v[0].IsEquivalent(v1,kDesignerEpsilon) )
			{
				std::swap(edgeIndexPair.m_i[0],edgeIndexPair.m_i[1]);
				std::swap(v0,v1);
			}
			pRegion->SetVertex(edgeIndexPair.m_i[0],v1);
			pRegion->Optimize();
			if( !pRegion->IsValid() || pRegion->IsOpen() )
				unnecessaryRegionList.push_back(pRegion);
		}
		else if( pRegion->HasVertex(e.m_v[0],&nVertexIndex) )
		{
			if( std::abs(pRegion->GetPlane().Distance(e.m_v[1])) > kDesignerEpsilon )
			{
				unnecessaryRegionList.push_back(pRegion);

				CBrushDesignerPolygonDecomposer decomposer;
				std::vector<CBrushRegion::RegionPtr> triangulatedRegions;
				if( decomposer.TriangulateRegion(pRegion,triangulatedRegions) )
				{
					for( int k = 0, iTriangulatedRegionCount(triangulatedRegions.size()); k < iTriangulatedRegionCount; ++k )
					{
						newRegionList.push_back(triangulatedRegions[k]);
						int nVertexIndexInSubTri = -1;
						if( !triangulatedRegions[k]->HasVertex(e.m_v[0],&nVertexIndexInSubTri) )
							continue;
						triangulatedRegions[k]->SetVertex(nVertexIndexInSubTri,e.m_v[1]);
						BrushPlane newPlane;
						triangulatedRegions[k]->GetComputedPlane(newPlane);
						triangulatedRegions[k]->SetPlane(newPlane);
					}
				}
				else
				{
					DESIGNER_ASSERT(0);
				}
			}
			else
			{
				pRegion->SetVertex(nVertexIndex,e.m_v[1]);
				pRegion->Optimize();
				if( !pRegion->IsValid() || pRegion->IsOpen() )
					unnecessaryRegionList.push_back(pRegion);
			}
		}
	}

	for( int i = 0, iUnnecessaryCount(unnecessaryRegionList.size()); i < iUnnecessaryCount; ++i )
		mc.pDesigner->RemoveRegion(unnecessaryRegionList[i]);

	for( int i = 0, iNewRegionCount(newRegionList.size()); i < iNewRegionCount; ++i )
		mc.pDesigner->AddRegion(newRegionList[i],CBrushDesigner::eOpType_Union);
}

void CBrushDesignerWeldTool::Enter()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	int nSelectionCount = pSelected->GetSize();
	if( nSelectionCount != 2 )
	{
		AfxMessageBox("Only two vertices should be selected for using this tool.", MB_OK);
		GetEditTool()->GoToSelectDesignerMode();
		return;
	}

	for( int i = 0; i < nSelectionCount; ++i )
	{
		if( !(*pSelected)[i].IsVertex() )
		{
			AfxMessageBox("Only two vertices should be selected for using this tool.", MB_OK);
			GetEditTool()->GoToSelectDesignerMode();
			return;
		}
	}

	CUndo undo("Designer : Weld");
	GetDesigner()->RecordUndo("Designer : Weld",GetBaseObject());

	Weld(GetMainContext(),(*pSelected)[0].GetVertex(),(*pSelected)[1].GetVertex());

	CBrushDesignerBaseTool::CreateMirroredRegions(GetDesigner());
	UpdateBrush();
	Sync();

	SDesignerElement lastSelection = (*pSelected)[1];
	pSelected->Clear();
	pSelected->Add(lastSelection);
	GetEditTool()->GoToSelectDesignerMode();
}