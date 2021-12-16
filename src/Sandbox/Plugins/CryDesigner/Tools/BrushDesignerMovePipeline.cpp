#include "StdAfx.h"
#include "BrushDesignerMovePipeline.h"
#include "Core/BrushDesigner.h"
#include "Core/BrushDesignerPolygonDecomposer.h"
#include "Core/BaseBrush.h"
#include "IBaseToolPanel.h"

void CBrushDesignerMovePipeline::TransformSelections( BUtil::SMainContext& mc, const BrushMatrix34& offsetTM )
{
	ComputeIntermediatePositionsBasedOnInitQueryResults(offsetTM);
	CreateOrganizedResultsAroundRegionFromQueryResults();

	if( !ExcutedAdditionPass() )
	{
		if( VertexAdditionFirstPass() )
		{
			ComputeIntermediatePositionsBasedOnInitQueryResults(offsetTM);
			CreateOrganizedResultsAroundRegionFromQueryResults();
		}

		if( VertexAdditionSecondPass() )
		{
			ComputeIntermediatePositionsBasedOnInitQueryResults(offsetTM);
			CreateOrganizedResultsAroundRegionFromQueryResults();
		}

		SetExcutedAdditionPass(true);
	}

	if( SubdivisionPass() )
	{
		mc.pDesigner->ResetDB(BUtil::eDBRF_ALL);
		SetQueryResultsFromSelectedElements(*mc.pSelected);
		ComputeIntermediatePositionsBasedOnInitQueryResults(offsetTM);
		CreateOrganizedResultsAroundRegionFromQueryResults();
	}

	TransformationPass();
	AssignIntermediatedPosToSelectedElements(*mc.pSelected);	

	MergeCoplanarPass();
}

void CBrushDesignerMovePipeline::SetQueryResultsFromSelectedElements( const CBrushDesignerElementManager& selectedElements )
{
	if( selectedElements.IsEmpty() )
		return;
	m_QueryResult = selectedElements.QueryFromElements(GetDesigner());
	DESIGNER_ASSERT(!m_QueryResult.empty());
}

void CBrushDesignerMovePipeline::CreateOrganizedResultsAroundRegionFromQueryResults()
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(1);
	m_OrganizedQueryResult = CBrushDesignerSelectTool::CreateOrganizedResultsAroundRegionFromQueryResults(m_QueryResult);
}

void CBrushDesignerMovePipeline::ComputeIntermediatePositionsBasedOnInitQueryResults( const BrushMatrix34& offsetTM )
{
	m_IntermediateTransQueryPos.clear();

	int iQueryResultSize(m_InitQueryResult.size());
	m_IntermediateTransQueryPos.reserve(iQueryResultSize);

	DESIGNER_ASSERT(iQueryResultSize);

	for( int i = 0; i < iQueryResultSize; ++i )
	{
		const CBrushDesignerDB::Vertex& v = m_InitQueryResult[i];
		m_IntermediateTransQueryPos.push_back(offsetTM.TransformPoint(v.m_Pos));
	}

	SnappedToMirrorPlane();
}

bool CBrushDesignerMovePipeline::VertexAdditionFirstPass()
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	std::vector<CBrushDesignerDB::Vertex> newVertices;
	for( int i = 0, iQuerySize(m_InitQueryResult.size()); i < iQuerySize; ++i )
	{
		CBrushDesignerDB::Vertex& v = m_InitQueryResult[i];
		int vertexMarkListSize(v.m_MarkList.size());

		for( int k = 0; k < vertexMarkListSize; ++k )
		{
			CBrushDesignerDB::Mark& mark = v.m_MarkList[k];
			CBrushRegion::RegionPtr pRegion = mark.m_pRegion;
			if( pRegion == NULL )
				continue;

			BrushVec3 nextVertex;
			if( !pRegion->GetNextVertex( mark.m_VertexIndex, nextVertex ) )
				continue;

			BrushVec3 prevVertex;
			if( !pRegion->GetPrevVertex( mark.m_VertexIndex, prevVertex ) )
				continue;

			BrushVec3 nextPrevVertices[2] = { nextVertex, prevVertex };
			bool bValid[2] = { true, true };
			for( int b = 0; b < 2; ++b )

			{
				for( int a = 0; a < iQuerySize; ++a )
				{
					if( a == i )
						continue;

					if( m_QueryResult[a].m_Pos.IsEquivalent(nextPrevVertices[b],kDesignerEpsilon) )
					{
						bValid[b] = false;
						break;
					}
				}
			}

			CBrushDesignerDB::QueryResult qResult[2];
			if( !GetDesigner()->GetDB()->QueryAsVertex( nextVertex, qResult[0] ) )
				continue;
			if( !GetDesigner()->GetDB()->QueryAsVertex( prevVertex, qResult[1] ) )
				continue;

			for( int a = 0; a < 2; ++a )
			{
				if( bValid[a] == false )
					continue;

				if( qResult[a].size() != 1 )
					continue;

				GetDesigner()->SetShelf(1);
				int nAdjacentRegionIndex(-1);
				CBrushRegion::RegionPtr pAdjacentRegion = FindAdjacentRegion( pRegion, nextPrevVertices[a], nAdjacentRegionIndex );
				if( !pAdjacentRegion )
					continue;

				CBrushDesignerDB::Mark newMark;
				newMark.m_VertexIndex = pAdjacentRegion->GetVertexListSize();
				newMark.m_pRegion = GetDesigner()->GetRegion(nAdjacentRegionIndex);

				bool bExistVertex(false);
				for( int b = 0, iNewVertexSize(newVertices.size()); b < iNewVertexSize; ++b )
				{
					if( newVertices[b].m_Pos.IsEquivalent(nextPrevVertices[a], kDesignerEpsilon) )
					{
						bExistVertex = true;
						newVertices[b].m_MarkList.push_back(newMark);
						break;
					}
				}

				if( !bExistVertex )
				{
					CBrushDesignerDB::Vertex v;
					v.m_Pos = nextPrevVertices[a];
					v.m_MarkList.push_back(newMark);
					newVertices.push_back(v);
				}
			}
		}
	}

	for( int i = 0, iNewVertexSize(newVertices.size()); i < iNewVertexSize; ++i )
	{
		CBrushDesignerDB::Vertex& v = newVertices[i];
		for( int k = 0, iMarkListSize(v.m_MarkList.size()); k < iMarkListSize; ++k )
		{
			CBrushRegion* pRegion = v.m_MarkList[k].m_pRegion;
			if( pRegion == NULL )
				continue;

			if( pRegion->AddVertex(v.m_Pos,&v.m_MarkList[k].m_VertexIndex,NULL) )
				GetDesigner()->GetDB()->AddMarkToVertex( v.m_Pos, v.m_MarkList[k] );
		}
	}

	return !newVertices.empty();
}
bool CBrushDesignerMovePipeline::VertexAdditionSecondPass()
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	bool bAdded = false;

	for( int i = 0, iQuerySize(m_InitQueryResult.size()); i < iQuerySize; ++i )
	{
		CBrushDesignerDB::Vertex& v = m_InitQueryResult[i];
		int markListSize(v.m_MarkList.size());

		if( markListSize > 2 )
			continue;

		CBrushRegion::RegionPtr pRegion = v.m_MarkList[0].m_pRegion;
		if( pRegion == NULL )
			continue;

		for( int k = 0; k < BUtil::kMaxShelfCount; ++k )
		{
			GetDesigner()->SetShelf(k);
			int nAdjacentRegionIndex(-1);
			CBrushRegion::RegionPtr pAdjacentRegion = FindAdjacentRegion( pRegion, v.m_Pos, nAdjacentRegionIndex );
			if( pAdjacentRegion )
			{
				CBrushRegion::RegionPtr pOldAdjacentRegion = pAdjacentRegion->Clone();
				CBrushDesignerDB::Mark newMark;
				if( !pAdjacentRegion->AddVertex(v.m_Pos,&newMark.m_VertexIndex,NULL) )
					continue;
				if( k != 1 )
					GetDesigner()->RemoveRegion(nAdjacentRegionIndex);
				CBrushDesignerBaseTool::RemoveMirroredRegion(GetDesigner(),pOldAdjacentRegion);

				if( k != 1 )
				{
					GetDesigner()->SetShelf(1);		
					nAdjacentRegionIndex = GetDesigner()->GetRegionSize();
					GetDesigner()->AddRegion(pAdjacentRegion,CBrushDesigner::eOpType_Add);
				}
				newMark.m_pRegion = GetDesigner()->GetRegion(nAdjacentRegionIndex);
				v.m_MarkList.push_back(newMark);

				bAdded = true;
			}
		}
	}

	if( bAdded )
		m_QueryResult = m_InitQueryResult;

	return bAdded;
}

bool CBrushDesignerMovePipeline::SubdivisionPass()
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(1);

	std::vector<CBrushRegion::RegionPtr> removedRegions;
	std::vector<CBrushRegion::RegionPtr> addedRegions;

	m_UnsubdividedRegions.clear();
	m_SubdividedRegions.clear();

	CBrushDesignerSelectTool::OrganizedQueryResults::iterator ii = m_OrganizedQueryResult.begin();
	for( ; ii != m_OrganizedQueryResult.end(); ++ii )
	{
		CBrushRegion::RegionPtr pRegion = ii->first;
		if( pRegion == NULL )
			continue;

		if( pRegion->IsOpen() || pRegion->GetVertexListSize() == 3 )
			continue;

		CBrushDesignerSelectTool::QueryInputs queryInputs(ii->second);
		int iQuerySize(queryInputs.size());

		if( iQuerySize == 0 || iQuerySize == pRegion->GetVertexListSize() )
			continue;

		int i = 0;
		for( ; i < iQuerySize; ++i )
		{
			int nQueryIndex(queryInputs[i].first);
			// check if a position of the transformed vertex in a region gets out of the plane of the region, 
			if( std::abs(pRegion->GetPlane().Distance(m_IntermediateTransQueryPos[nQueryIndex])) > kDesignerLooseEpsilon )		
				break;
		}

		if( i == iQuerySize )
		{
			m_UnsubdividedRegions.insert(pRegion);
			continue;
 		}
// 		else if( pRegion->GetVertexListSize() <= 4 )
// 		{	
// 			std::vector<BrushVec2> vList(4);
// 			const BrushPlane& plane = pRegion->GetPlane();
// 			for( i = 0; i < 4; ++i )
// 				vList[i] = plane.W2P(pRegion->GetVertex(i));
// 			for( i = 0; i < iQuerySize; ++i )
// 			{
// 				int nQueryIndex(queryInputs[i].first);
// 				const CBrushDesignerDB::Vertex& v = m_QueryResult[nQueryIndex];
// 				int nMarkIndex = queryInputs[i].second;	
// 				vList[v.m_MarkList[nMarkIndex].m_VertexIndex] = plane.W2P(m_IntermediateTransQueryPos[nQueryIndex]);
// 			}
// 			if( BUtil::IsConvex(vList) )
// 			{
// 				m_UnsubdividedRegions.insert(pRegion);
// 				continue;
// 			}
// 		}

		std::vector<CBrushRegion::RegionPtr> triangleRegions;
		CBrushDesignerPolygonDecomposer decomposer(BUtil::eDF_SkipOptimizationOfRegionResults);
		if( !decomposer.TriangulateRegion(pRegion,triangleRegions) )
			continue;
		if( triangleRegions.size() == 1 )
			continue;

		for( int i = 0, iTriangleRegionSize(triangleRegions.size()); i < iTriangleRegionSize; ++i )
			m_SubdividedRegions[pRegion].push_back(triangleRegions[i]);

		addedRegions.insert( addedRegions.end(), triangleRegions.begin(), triangleRegions.end() );
		removedRegions.push_back(pRegion);
	}

	for( int i = 0, iRemovedRegionSize(removedRegions.size()); i < iRemovedRegionSize; ++i )
		GetDesigner()->RemoveRegion(removedRegions[i]);

	for( int i = 0, iRegionSize(addedRegions.size()); i < iRegionSize; ++i )
		GetDesigner()->AddRegion(addedRegions[i],CBrushDesigner::eOpType_Add);

	return removedRegions.empty() ? false : true;
}

void CBrushDesignerMovePipeline::TransformationPass()
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(1);

	CBrushDesignerSelectTool::OrganizedQueryResults::iterator ii = m_OrganizedQueryResult.begin();
	for( ; ii != m_OrganizedQueryResult.end(); ++ii )
	{
		CBrushRegion::RegionPtr pRegion = ii->first;
		if( pRegion == NULL )
			continue;

		const CBrushDesignerSelectTool::QueryInputs& queryInputs = ii->second;
		int iQuerySize(queryInputs.size());

		for( int i = 0; i < iQuerySize; ++i )
		{
			int nQueryIndexInQueryInputs(queryInputs[i].first);
			pRegion->SetVertex( GetMark(queryInputs[i]).m_VertexIndex, m_IntermediateTransQueryPos[nQueryIndexInQueryInputs] );
		}

		if( m_UnsubdividedRegions.find(pRegion) == m_UnsubdividedRegions.end() )
		{
			BrushPlane computedPlane;
			if( pRegion->GetComputedPlane(computedPlane) )
				pRegion->SetPlane(computedPlane);
		}
	}

	GetDesigner()->InvalidateAABB(1);
}

bool CBrushDesignerMovePipeline::MergeCoplanarPass()
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(1);

	std::map<CBrushRegion::RegionPtr,CBrushDesigner::RegionList>::iterator ii = m_SubdividedRegions.begin();
	bool bMergeHappened = false;

	for( ; ii != m_SubdividedRegions.end(); ++ii )
	{
		CBrushDesigner::RegionList& regionList = ii->second;
		int iRegionSize(regionList.size());
		if( iRegionSize == 1 )
			continue;

		for( int i = 0; i < iRegionSize; ++i )
		{
			if( regionList[i] == NULL )
				continue;

			for( int k = 0; k < iRegionSize; ++k )
			{
				if( i == k || regionList[k] == NULL )
					continue;

				if( CBrushRegion::HasIntersection(regionList[i],regionList[k]) == BUtil::eIT_JustTouch )
				{
					CBrushRegion::RegionPtr previous = regionList[i]->Clone();
					regionList[i]->Union(regionList[k]);
					DESIGNER_ASSERT( regionList[i]->IsValid() );

#ifdef ENABLE_OUTPUT_DEBUGINFO
					if( previous->IsEquivalent(regionList[i]) )
					{
						DESIGNER_ASSERT(0);
						IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
						dlg->AddRegion(regionList[i].get(),"regionList[i]"); 
						dlg->AddRegion(regionList[k].get(),"regionList[k]");
						regionList[i]->Union(regionList[k]);
						dlg->AddRegion(regionList[i].get(),"regionList[i] U regionList[k]");
						dlg->Open();
					}
#endif

					GetDesigner()->RemoveRegion(regionList[k]);
					regionList[k] = NULL;
					bMergeHappened = true;
				}
			}
		}
	}

	return bMergeHappened;
}

void CBrushDesignerMovePipeline::AssignIntermediatedPosToSelectedElements( CBrushDesignerElementManager& selectedElements )
{
	for( int i = 0, iResultSize(m_QueryResult.size()); i < iResultSize; ++i )
	{
		for( int k = 0, iSelectedSize(selectedElements.GetSize()); k < iSelectedSize; ++k )
		{
			for( int a = 0, iVertexSize(selectedElements[k].m_Vertices.size()); a < iVertexSize; ++a )
			{
				if( selectedElements[k].m_Vertices[a].IsEquivalent(m_QueryResult[i].m_Pos,kDesignerEpsilon) )
				{
					selectedElements[k].m_Vertices[a] = m_IntermediateTransQueryPos[i];
					break;
				}
			}
		}
	}
}

CBrushRegion::RegionPtr CBrushDesignerMovePipeline::FindAdjacentRegion( CBrushRegion::RegionPtr pRegion, const BrushVec3& vPos, int& outAdjacentRegionIndex )
{
	for( int a = 0, iRegionSize(GetDesigner()->GetRegionSize()); a < iRegionSize; ++a )
	{
		CBrushRegion::RegionPtr pCandidateRegion(GetDesigner()->GetRegion(a));
		if( pRegion == pCandidateRegion )
			continue;
		if( std::abs(pCandidateRegion->GetPlane().Distance(vPos)) > kDistanceLimitation )
			continue;
		BrushVec3 nearestPos;
		if( !pCandidateRegion->QueryNearestPosFromBoundary(vPos,nearestPos) )
			continue;
		if( (vPos-nearestPos).GetLength() > kDistanceLimitation )
			continue;
		if( pCandidateRegion->Exist(vPos,kDistanceLimitation) )
			continue;
		outAdjacentRegionIndex = a;
		return pCandidateRegion;
	}
	return NULL;
}

void CBrushDesignerMovePipeline::Initialize( const CBrushDesignerElementManager& elements )
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	SetQueryResultsFromSelectedElements(elements);
	SetExcutedAdditionPass(false);

	GetDesigner()->SetShelf(0);
	std::set<CBrushRegion::RegionPtr> regionSet;
	for( int i = 0, iQueryResult(m_QueryResult.size()); i < iQueryResult; ++i )
	{
		const CBrushDesignerDB::Vertex& v = m_QueryResult[i];
		for( int k = 0, iMarkSize(v.m_MarkList.size()); k < iMarkSize; ++k )
		{
			CBrushRegion::RegionPtr pRegion = v.m_MarkList[k].m_pRegion;
			if( !pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) )
				regionSet.insert(pRegion);
		}
	}
	std::set<CBrushRegion::RegionPtr>::iterator ii = regionSet.begin();
	for( ; ii != regionSet.end(); ++ii )
	{
		GetDesigner()->SetShelf(0);
		GetDesigner()->RemoveRegion(*ii);

		GetDesigner()->SetShelf(1);
		GetDesigner()->AddRegion(*ii,CBrushDesigner::eOpType_Add);

		if( !GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
			continue;

		GetDesigner()->SetShelf(0);

		CBrushRegion::RegionPtr pMirroredRegion = GetDesigner()->QueryEquivalentRegion((*ii)->Clone()->Mirror(GetDesigner()->GetMirrorPlane()));
		DESIGNER_ASSERT(pMirroredRegion);

		if( !pMirroredRegion )
			continue;
		GetDesigner()->RemoveRegion(pMirroredRegion);
	}

	GetDesigner()->ResetDB(BUtil::eDBRF_ALL,1);
	SetQueryResultsFromSelectedElements(elements);
	m_InitQueryResult = m_QueryResult;
}

void CBrushDesignerMovePipeline::InitializeIndependently( CBrushDesignerElementManager& elements )
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	m_QueryResult.clear();

	for( int i = 0, iElementCount(elements.GetSize()); i < iElementCount; ++i )
	{
		GetDesigner()->SetShelf(0);
		GetDesigner()->RemoveRegion(elements[i].m_pRegion);

		if( GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		{
			CBrushRegion::RegionPtr pMirroredRegion = GetDesigner()->QueryEquivalentRegion(elements[i].m_pRegion->Clone()->Mirror(GetDesigner()->GetMirrorPlane()));
			DESIGNER_ASSERT(pMirroredRegion);
			if( pMirroredRegion )					
				GetDesigner()->RemoveRegion(pMirroredRegion);
		}

		GetDesigner()->SetShelf(1);
		GetDesigner()->AddRegion(elements[i].m_pRegion,CBrushDesigner::eOpType_Add);

		for( int k = 0, iVertexCount(elements[i].m_pRegion->GetVertexListSize()); k < iVertexCount; ++k )
		{
			CBrushDesignerDB::Vertex v;
			CBrushDesignerDB::Vertex *pV = &v;

			BrushVec3 pos = elements[i].m_pRegion->GetVertex(k);
			for( int a = 0, iQueryResultCount(m_QueryResult.size()); a < iQueryResultCount; ++a )
			{
				if( m_QueryResult[a].m_Pos.IsEquivalent(pos,kDesignerEpsilon) )
				{
					pV = &m_QueryResult[a];
					break;
				}
			}

			if( pV == &v )
				pV->m_Pos = pos;		

			if( elements[i].m_pRegion )
			{
				CBrushDesignerDB::Mark m;
				m.m_pRegion = elements[i].m_pRegion;
				m.m_VertexIndex = k;
				pV->m_MarkList.push_back(m);
				if( pV == &v )
					m_QueryResult.push_back(v);
			}
		}
	}

	m_InitQueryResult = m_QueryResult;
}

void CBrushDesignerMovePipeline::End()
{
	GetDesigner()->MoveShelf(1,0);
	GetDesigner()->ResetDB(BUtil::eDBRF_ALL);

	if( m_QueryResult.size() == m_IntermediateTransQueryPos.size() )
	{
		for( int i = 0, iQuerySize(m_QueryResult.size()); i < iQuerySize; ++i )
			m_QueryResult[i].m_Pos = m_IntermediateTransQueryPos[i];
	}
}

bool CBrushDesignerMovePipeline::GetAveragePos( BrushVec3& outAveragePos ) const
{
	if( m_IntermediateTransQueryPos.empty() )
		return false;
	BrushVec3 vAveragePos(0,0,0);
	int iQSize(m_IntermediateTransQueryPos.size());
	if( iQSize > 0 )
	{
		for( int i = 0; i < iQSize; ++i )
			vAveragePos += m_IntermediateTransQueryPos[i];
		vAveragePos /= iQSize;
	}
	outAveragePos = vAveragePos;
	return true;
}

void CBrushDesignerMovePipeline::SnappedToMirrorPlane()
{
	if( m_IntermediateTransQueryPos.empty() )
		return;

	if( !GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		return;

	int iIntermediateCount(m_IntermediateTransQueryPos.size());
	BrushVec3 mirrorNormal = GetDesigner()->GetMirrorPlane().Normal();
	int nFarthestPosIndex = -1;
	BrushFloat fFarthestDist = 0;
	for( int i = 0; i < iIntermediateCount; ++i )
	{
		BrushFloat distanceFromPosToMirrorPlane = GetDesigner()->GetMirrorPlane().Distance(m_IntermediateTransQueryPos[i]);
		if( distanceFromPosToMirrorPlane > 0 && distanceFromPosToMirrorPlane > fFarthestDist )
		{
			nFarthestPosIndex = i;
			fFarthestDist = distanceFromPosToMirrorPlane;
		}
	}

	if( nFarthestPosIndex != -1 )
	{
		BrushVec3 vOffsetedPos;
		if( GetDesigner()->GetMirrorPlane().HitTest( m_IntermediateTransQueryPos[nFarthestPosIndex], m_IntermediateTransQueryPos[nFarthestPosIndex]+mirrorNormal, kDesignerEpsilon, NULL, &vOffsetedPos ) )
		{
			BrushVec3 vOffset = vOffsetedPos - m_IntermediateTransQueryPos[nFarthestPosIndex];
			for( int i = 0; i < iIntermediateCount; ++i )
				m_IntermediateTransQueryPos[i] += vOffset;
		}
	}
}