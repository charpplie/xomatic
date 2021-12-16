#include "StdAfx.h"
#include "BrushDesignerSnapToGrid.h"
#include "BrushDesignerSelectTool.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesignerPolygonDecomposer.h"
#include "Grid.h"
#include "ViewManager.h"

void CBrushDesignerSnapToGridTool::Enter()
{
	CUndo undo("Designer : Snap to Grid");
	GetDesigner()->RecordUndo("Snap To Grid",GetBaseObject());

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	for( int i = 0, iCount(pSelected->GetSize()); i < iCount; ++i )
	{
		for( int k = 0, iVertexCount((*pSelected)[i].m_Vertices.size()); k < iVertexCount; ++k )
		{
			BrushVec3 vSnappedPos = SnapVertexToGrid((*pSelected)[i].m_Vertices[k]);
			(*pSelected)[i].m_Vertices[k] = vSnappedPos;
		}
	}

	if( gSettings.bDesignerKeepCenterPivot )
		GetBrush()->PivotToCenter(GetBaseObject(),GetDesigner());
	GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
	UpdateBrush();
	Sync();
	UpdateGameResource(GetBaseObject());

	GetEditTool()->GoToPrevDesignerMode();
}

BrushVec3 CBrushDesignerSnapToGridTool::SnapVertexToGrid( const BrushVec3& vPos )
{
	CGrid* pGrid = GetIEditor()->GetViewManager()->GetGrid();

	BrushVec3 vWorldPos = GetWorldTM().TransformPoint(vPos);

	BrushVec3 vSnappedPos;
	vSnappedPos.x = floor((vWorldPos.x/pGrid->size)/pGrid->scale + 0.5) * pGrid->size * pGrid->scale;
	vSnappedPos.y = floor((vWorldPos.y/pGrid->size)/pGrid->scale + 0.5) * pGrid->size * pGrid->scale;
	vSnappedPos.z = floor((vWorldPos.z/pGrid->size)/pGrid->scale + 0.5) * pGrid->size * pGrid->scale;

	vSnappedPos = GetWorldTM().GetInverted().TransformPoint(vSnappedPos);

	CBrushDesignerDB::QueryResult qResult;
	GetDesigner()->GetDB()->QueryAsVertex(vPos,qResult);

	std::vector<CBrushRegion::RegionPtr> oldRegions;
	std::vector<CBrushRegion::RegionPtr> newRegions;

	for( int i = 0, iQueryCount(qResult.size()); i < iQueryCount; ++i )
	{
		for( int k = 0, iMarkCount(qResult[i].m_MarkList.size()); k < iMarkCount; ++k )
		{
			CBrushRegion::RegionPtr pRegion = qResult[i].m_MarkList[k].m_pRegion;
			int nVertexIndex = qResult[i].m_MarkList[k].m_VertexIndex;

			if( std::abs(pRegion->GetPlane().Distance(vSnappedPos)) > kDesignerEpsilon )
			{	
				oldRegions.push_back(pRegion);
				CBrushDesignerPolygonDecomposer triangulator;
				std::vector<CBrushRegion::RegionPtr> triangules;
				triangulator.TriangulateRegion(pRegion,triangules);

				for( int a = 0, iTriangleCount(triangules.size()); a < iTriangleCount; ++a )
				{
					int nVertexIndexInTriangle;
					if( !triangules[a]->GetVertexIndex(vPos,nVertexIndexInTriangle) )
						continue;
					triangules[a]->SetVertex(nVertexIndexInTriangle,vSnappedPos);
					BrushPlane updatedPlane;
					if( triangules[a]->GetComputedPlane(updatedPlane) )
						triangules[a]->SetPlane(updatedPlane);
				}

				newRegions.insert( newRegions.end(), triangules.begin(), triangules.end() );
			}
			else
			{
				pRegion->SetVertex(nVertexIndex, vSnappedPos);
			}
		}
	}

	for( int i = 0, iRegionCount(oldRegions.size()); i < iRegionCount; ++i )
		GetDesigner()->RemoveRegion(oldRegions[i]);

	for( int i = 0, iRegionCount(newRegions.size()); i < iRegionCount; ++i )
		GetDesigner()->AddRegion(newRegions[i],CBrushDesigner::eOpType_Union);

	GetDesigner()->ResetDB(BUtil::eDBRF_ALL);

	return vSnappedPos;
}