#include "Stdafx.h"
#include "BrushDesignerPolygonDecomposer.h"
#include "BrushBSPTree2D.h"
#include "IBaseToolPanel.h"
#include "BrushDesignerSmoothingGroup.h"
#include "BrushConvexes.h"

const BrushFloat CBrushDesignerPolygonDecomposer::kComparisonEpsilon = 1.0e-5f;

namespace
{
	const BrushFloat kDecomposerEpsilon = 1.0e-5f;	

	typedef std::pair<BrushFloat,BrushFloat> YXPair;

	bool IsSame( BrushFloat v0, BrushFloat v1 )
	{
		return std::abs(v0-v1) < kDecomposerEpsilon;
	}

	bool IsGreaterEqual( BrushFloat v0, BrushFloat v1 )
	{
		return v0-v1 >= -kDecomposerEpsilon;
	}

	bool IsLessEqual( BrushFloat v0, BrushFloat v1 )
	{
		return v0-v1 <= kDecomposerEpsilon;
	}
}

bool CBrushDesignerPolygonDecomposer::TriangulateRegion( CBrushRegion::RegionPtr pRegion, std::vector<CBrushRegion::RegionPtr>& outTrianguleRegions )
{
	std::vector<BrushVec3> vList;
	std::vector<BrushVec3> nList;
	std::vector<SMeshFace> fList;

	if( !TriangulateRegion(pRegion,vList,nList,fList,0,0) )
		return false;

	for( int i = 0, iFaceList(fList.size()); i < iFaceList; ++i )
	{
		std::vector<BrushVec3> face;
		face.push_back(BrushVec3(vList[fList[i].v[0]]));
		face.push_back(BrushVec3(vList[fList[i].v[1]]));
		face.push_back(BrushVec3(vList[fList[i].v[2]]));
		CBrushRegion::RegionPtr pSubdividedRegion = new CBrushRegion(face);
		pSubdividedRegion->SetMaterialID(pRegion->GetMaterialID());
		pSubdividedRegion->SetTexInfo(pRegion->GetTexInfo());
		pSubdividedRegion->SetFlag(pRegion->GetFlag());
		pSubdividedRegion->SetPlane(pRegion->GetPlane());
		outTrianguleRegions.push_back(pSubdividedRegion);
	}

	return true;
}

bool CBrushDesignerPolygonDecomposer::TriangulateRegion( CBrushRegion::RegionPtr pRegion, std::vector<BrushVec3>& outVertexList, std::vector<BrushVec3>& outNormalList, std::vector<SMeshFace>& outFaceList, int vertexOffset, int faceOffset )
{
	if( !pRegion || pRegion->IsOpen() )
		return false;

	m_bGenerateConvexes = false;

	outFaceList.reserve(pRegion->GetVertexListSize());

	m_FaceOffset = faceOffset;
	m_VertexOffset = vertexOffset;
	m_pRegion = pRegion;
	m_Plane = pRegion->GetPlane();
	m_MaterialID = pRegion->GetMaterialID();
	m_nBasedVertexIndex = 0;

	m_pOutVertexList = &outVertexList;
	m_pOutFaceList = &outFaceList;

	Decompose(&CBrushDesignerPolygonDecomposer::DecomposeToTriangules);

	int vertexCount(outVertexList.size());
	outNormalList.reserve(vertexCount);

	const BrushVec3& normal = pRegion->GetPlane().Normal();
	for( int i = vertexOffset; i < vertexCount; ++i )
		outNormalList.push_back(normal);

	for( int i = faceOffset, faceCount(outFaceList.size()); i < faceCount; ++i )
		outFaceList[i].nSubset = 0;

	return true;
}

void CBrushDesignerPolygonDecomposer::CreateConvexes( CBrushRegion::RegionPtr pRegion, CBrushConvexes& outConvexes )
{
	if( !pRegion || pRegion->IsOpen() )
		return;

	m_bGenerateConvexes = true;

	m_FaceOffset = 0;
	m_VertexOffset = 0;
	m_pRegion = pRegion;
	m_Plane = pRegion->GetPlane();
	m_MaterialID = pRegion->GetMaterialID();
	m_nBasedVertexIndex = 0;
	m_pBrushConvexes = &outConvexes;

	std::vector<BrushVec3> outVertices;
	std::vector<SMeshFace> outFaceMesh;

	m_pOutVertexList = &outVertices;
	m_pOutFaceList = &outFaceMesh;

	Decompose(&CBrushDesignerPolygonDecomposer::DecomposeToTriangules);	
}

bool CBrushDesignerPolygonDecomposer::Decompose( DecomposeRoutine pDecomposeRoutine )
{
	CBrushRegion::RegionPtr pRegion = m_pRegion;
	if( !pRegion )
		return false;

	if( pRegion->HasBridgeEdges() )
	{
		pRegion = pRegion->Clone();
		pRegion->RemoveBridgeEdges();
	}

	std::vector<CBrushRegion::RegionPtr> outerRegions;
	if( !pRegion->GetSeparatedRegions(outerRegions,CBrushRegion::eSR_OuterHull,!CheckFlag(BUtil::eDF_SkipOptimizationOfRegionResults)) )
		return false;

	std::vector<CBrushRegion::RegionPtr> innerRegions;
	pRegion->GetSeparatedRegions(innerRegions,CBrushRegion::eSR_InnerHull,!CheckFlag(BUtil::eDF_SkipOptimizationOfRegionResults));

	for( int k = 0, iOuterRegionsCount(outerRegions.size()); k < iOuterRegionsCount; ++k )
	{
		CBrushRegion::RegionPtr pOuterRegion = outerRegions[k];
		if( !pOuterRegion )
			return false;

		IndexList initialIndexList;
		m_VertexList.clear();
		m_PointList.clear();

		if( m_bGenerateConvexes )
			RemoveAllConvexData();

		FillVertexListFromRegion(pOuterRegion,m_VertexList);

		if( m_bGenerateConvexes )
		{
			for( int i = 0, iVertexCount(m_VertexList.size()); i < iVertexCount; ++i )
				m_InitialEdgesSortedByEdge.insert(GetSortedEdgePair(i,(i+1)%iVertexCount));
		}

		int iVertexSize(m_VertexList.size());
		for( int i = 0; i < iVertexSize; ++i )
		{
			initialIndexList.push_back(i);
			m_PointList.push_back(SPointInfo(m_Plane.W2P(m_VertexList[i]),((i-1)+iVertexSize)%iVertexSize,(i+1)%iVertexSize));
		}

		bool bHasInnerHull = false;
		for( int i = 0, innerRegionSize(innerRegions.size()); i < innerRegionSize; ++i )
		{
			if( !pOuterRegion->IncludeAllEdges(innerRegions[i]) )
				continue;
			bHasInnerHull = true;
			int nBaseIndex = m_PointList.size();
			std::vector<BrushVec3> innerVertexList;
			FillVertexListFromRegion(innerRegions[i],innerVertexList);
			for( int k = 0, nVertexSize(innerVertexList.size()); k < nVertexSize; ++k)
			{
				int nPrevIndex = k == 0 ? nBaseIndex+nVertexSize-1 : nBaseIndex+k-1;
				int nNextIndex = k < nVertexSize-1 ? m_PointList.size()+1 : nBaseIndex;
				BrushVec2 pointPos = m_Plane.W2P(innerVertexList[k]);
				m_PointList.push_back(SPointInfo(pointPos,nPrevIndex,nNextIndex));
				initialIndexList.push_back(initialIndexList.size());
			}

			m_VertexList.insert( m_VertexList.end(), innerVertexList.begin(), innerVertexList.end() );

			if( m_bGenerateConvexes )
			{
				for( int i = 0, iVertexCount(innerVertexList.size()); i < iVertexCount; ++i )
					m_InitialEdgesSortedByEdge.insert(GetSortedEdgePair(i+nBaseIndex,(i+1)%iVertexCount+nBaseIndex));
			}
		}

		m_pOutVertexList->insert(m_pOutVertexList->end(), m_VertexList.begin(), m_VertexList.end());

		if( !(this->*pDecomposeRoutine)(&initialIndexList,bHasInnerHull) )
			return false;

		if( m_bGenerateConvexes )
			CreateConvexes();

		m_nBasedVertexIndex += m_VertexList.size();
	}

	return true;
}

bool CBrushDesignerPolygonDecomposer::DecomposeToTriangules( IndexList* pIndexList, bool bHasInnerHull )
{
	bool bSuccessfulTriangulate = false;
	if( IsConvex(*pIndexList,false) && !bHasInnerHull )
		bSuccessfulTriangulate = TriangulateConvex(*pIndexList,*m_pOutFaceList);
	else
		bSuccessfulTriangulate = TriangulateConcave(*pIndexList,*m_pOutFaceList);
	if( !bSuccessfulTriangulate )
		return false;
	return true;
}

void CBrushDesignerPolygonDecomposer::FillVertexListFromRegion( CBrushRegion::RegionPtr pRegion, std::vector<BrushVec3>& outVertexList )
{
	if( pRegion == NULL )
		return;

	for( int i = 0, iEdgeCount(pRegion->GetEdgeSize()); i < iEdgeCount; ++i )
	{
		const BUtil::SEdge& rEdge = pRegion->GetEdgeIndexPair(i);
		outVertexList.push_back(pRegion->GetVertex(rEdge.m_i[0]));
	}
}

bool CBrushDesignerPolygonDecomposer::TriangulateRegion( CBrushRegion::RegionPtr pRegion, BUtil::VertexList& outVertexList, BUtil::FaceList& outFaceList )
{
	if( !pRegion )
		return false;

	std::vector<BrushVec3> vertexList;
	std::vector<BrushVec3> normalList;
	std::vector<SMeshFace> faceList;
	if( !TriangulateRegion( pRegion, vertexList, normalList, faceList, 0, 0 ) )
		return false;

	int iVertexCount(vertexList.size());
	outVertexList.reserve(iVertexCount);
	for( int i = 0, iVertexCount(vertexList.size()); i < iVertexCount; ++i )
	{
		BUtil::VertexPtr pVertex = new BUtil::SVertex(vertexList[i], normalList[i]);
		outVertexList.push_back(pVertex);
	}

	for( int i = 0, iFaceCount(faceList.size()); i < iFaceCount; ++i )
	{
		BUtil::FacePtr pFace = new BUtil::SFace;
		pFace->m_IndexList.push_back(faceList[i].v[0]);
		pFace->m_IndexList.push_back(faceList[i].v[1]);
		pFace->m_IndexList.push_back(faceList[i].v[2]);
		pFace->m_MaterialID = 0;
		pFace->m_Plane = pRegion->GetPlane();
		outFaceList.push_back(pFace);
	}

	return true;
}

bool CBrushDesignerPolygonDecomposer::TriangulateConvex( const IndexList& indexList, std::vector<SMeshFace>& outFaceList ) const
{
	int iIndexCount(indexList.size());
	if( iIndexCount < 3 )
		return false;

	int nStartIndex = 0;

	if( iIndexCount >= 4 )
	{	
		bool bHasInvalidTriangle = true;
		while(bHasInvalidTriangle)
		{
			bHasInvalidTriangle = false;
			for( int i = 1; i < iIndexCount-1; ++i )
			{
				if( !HasArea(indexList[nStartIndex],indexList[(nStartIndex+i)%iIndexCount],indexList[(nStartIndex+i+1)%iIndexCount]) )
				{
					bHasInvalidTriangle = true;
					break;
				}
			}
			if( bHasInvalidTriangle )
			{
				if( (++nStartIndex) >= iIndexCount )
				{
					nStartIndex = 0;
					break;
				}
			}
		}
	}

	for( int i = 1; i < iIndexCount-1; ++i )
		AddTriangle( indexList[nStartIndex], indexList[(nStartIndex+i)%iIndexCount], indexList[(nStartIndex+i+1)%iIndexCount], outFaceList );

	return true;
}

CBrushBSPTree2D* CBrushDesignerPolygonDecomposer::GenerateBSPTree( const IndexList& indexList ) const
{
	std::vector<BrushEdge3D> edgeList;
	for( int i = 0, iIndexListCount(indexList.size()); i < iIndexListCount; ++i )
	{
		int nCurr = indexList[i];
		int nNext = indexList[(i+1)%iIndexListCount];
		edgeList.push_back(BrushEdge3D(m_VertexList[nCurr],m_VertexList[nNext]));
	}
	CBrushBSPTree2D* pBSPTree = new CBrushBSPTree2D;
	pBSPTree->BuildTree(m_pRegion->GetPlane(),edgeList);
	return pBSPTree;
}

void CBrushDesignerPolygonDecomposer::BuildEdgeListHavingSameY( const IndexList& indexList, std::map<SFloat,IndexList>& outEdgeList ) const
{
	for( int i = 0, nIndexBuffSize(indexList.size()); i < nIndexBuffSize; ++i )
	{
		int nNext = (i+1)%nIndexBuffSize;

		const BrushVec2& v0 = m_PointList[indexList[i]].m_Pos;
		const BrushVec2& v1 = m_PointList[indexList[nNext]].m_Pos;

		if( IsSame(v0.y,v1.y) )
		{
			SFloat fY(v0.y);
			if( outEdgeList.find(fY) != outEdgeList.end() )
			{
				IndexList& edgeList = outEdgeList[fY];
				bool bInserted = false;
				for( int k = 0, edgeListCount(edgeList.size()); k < edgeListCount; ++k )
				{
					if( v0.x < m_PointList[indexList[edgeList[k]]].m_Pos.x )
					{
						edgeList.insert(edgeList.begin()+k,i);
						bInserted = true;
						break;
					}
				}
				if( !bInserted )
					edgeList.push_back(i);
			}
			else
			{
				outEdgeList[fY].push_back(i);
			}
		}
	}
}

void CBrushDesignerPolygonDecomposer::BuildMarkSideList( const IndexList& indexList, const std::map<SFloat,IndexList>& edgeListHavingSameY, std::vector<EMarkSide>& outMarkSideList, std::map<SMark,std::pair<int,int>>& outSortedMarksMap ) const
{
	outMarkSideList.resize(m_VertexList.size());

	int nLeftTopIndex = FindLeftTopVertexIndex(indexList);
	int nRightBottomIndex = FindRightBottomVertexIndex(indexList);

	for( int i = 0,nIndexBuffSize(indexList.size()); i < nIndexBuffSize; ++i )
	{
		outMarkSideList[indexList[i]] = QueryMarkSide(i,indexList,nLeftTopIndex,nRightBottomIndex);

		SFloat fY(m_PointList[indexList[i]].m_Pos.y);
		int xPriority = outMarkSideList[indexList[i]] == eMarkSide_Left ? 0 : 100;
		SMark mark(m_PointList[indexList[i]].m_Pos.y,xPriority);
		EMarkSide iSide = outMarkSideList[indexList[i]];		

		std::map<SFloat,IndexList>::const_iterator ifY = edgeListHavingSameY.find(fY);
		if( ifY != edgeListHavingSameY.end() )
		{
			const IndexList& indexEdgeList = ifY->second;
			for( int k = 0, iIndexEdgeCount(indexEdgeList.size()); k < iIndexEdgeCount; ++k )
			{
				if( indexEdgeList[k] == i )
				{
					if( iSide == eMarkSide_Left )
						mark.m_XPriority += k*2;
					else if( iSide == eMarkSide_Right )
						mark.m_XPriority += k*2+1;
					break;
				}
				else if( (indexEdgeList[k]+1)%nIndexBuffSize == i )
				{
					if( iSide == eMarkSide_Left )
						mark.m_XPriority += k*2+1;
					else if( iSide == eMarkSide_Right )
						mark.m_XPriority += k*2;
					break;
				}
			}
		}

		outSortedMarksMap[mark] = std::pair<int,int>(indexList[i],i);
	}

	DESIGNER_ASSERT( outSortedMarksMap.size() == indexList.size() );
#ifdef ENABLE_OUTPUT_DEBUGINFO
	if( outSortedMarksMap.size() != indexList.size() )
		CallDebugger();
#endif
}

bool CBrushDesignerPolygonDecomposer::TriangulateMonotonePolygon( const IndexList& indexList, std::vector<SMeshFace>& outFaceList ) const
{
	CBrushBSPTree2D::BSPTree2DPtr pBSPTree = GenerateBSPTree(indexList);

	std::map<SFloat,IndexList> edgeListHavingSameY;
	BuildEdgeListHavingSameY(indexList,edgeListHavingSameY);

	std::vector<EMarkSide> sideList;
	std::map<SMark,std::pair<int,int>> yxSortedMarks;
	BuildMarkSideList(indexList,edgeListHavingSameY,sideList,yxSortedMarks);

	std::stack<int> vertexStack;
	std::map<SMark,std::pair<int,int>>::iterator ii = yxSortedMarks.begin();

	vertexStack.push(ii->second.first);
	++ii;
	vertexStack.push(ii->second.first);
	std::map<SMark,std::pair<int,int>>::iterator ii_prev = ii;
	++ii;

	for( ; ii != yxSortedMarks.end(); ++ii )
	{
		int top = vertexStack.top();
		int current = ii->second.first;

		DESIGNER_ASSERT( vertexStack.size() >= 2 );
#ifdef ENABLE_OUTPUT_DEBUGINFO
		if( vertexStack.size() < 2 )
			CallDebugger();
#endif

		vertexStack.pop();
		int prev = vertexStack.top();
		vertexStack.push(top);

		EMarkSide topSide = sideList[top];
		EMarkSide currentSide = sideList[current];

		if( topSide != currentSide )
		{
			int nBaseFaceIndex = outFaceList.size();

			while(!vertexStack.empty())
			{
				top = vertexStack.top();

				if( IsCCW(current,prev,top) )
					AddFace( current, prev, top, indexList, outFaceList );
				else
					AddFace( current, top, prev, indexList, outFaceList );

				prev = top;
				vertexStack.pop();
			}
			vertexStack.push(ii_prev->second.first);
		}
		else if( currentSide == eMarkSide_Right && IsCCW(current,top,prev) || currentSide == eMarkSide_Left && IsCW(current,top,prev) )
		{
			prev = top;
			vertexStack.pop();
			while(!vertexStack.empty())
			{
				top = vertexStack.top();

				if( IsInside(pBSPTree,current,top) )
				{
					if( IsCCW(current,prev,top) )
						AddFace( current, prev, top, indexList, outFaceList );
					else
						AddFace( current, top, prev, indexList, outFaceList );
				}
				else
				{
					break;
				}

				prev = top;
				vertexStack.pop();
			}
			vertexStack.push(prev);
		}

		vertexStack.push(ii->second.first);

		ii_prev = ii;
	}

	return true;
}

bool CBrushDesignerPolygonDecomposer::TriangulateConcave( const IndexList& indexList, std::vector<SMeshFace>& outFaceList )
{
	std::vector<IndexList> monotonePieces;
	if( !SplitIntoMonotonePieces(indexList,monotonePieces) )
		return false;

	for( int i = 0, iMonotonePiecesCount(monotonePieces.size()); i < iMonotonePiecesCount; ++i )
	{
		if( IsConvex(monotonePieces[i],false) )
			TriangulateConvex(monotonePieces[i],outFaceList);
		else
			TriangulateMonotonePolygon(monotonePieces[i],outFaceList);
	}

	return true;
}

bool CBrushDesignerPolygonDecomposer::IsCCW( int i0, int i1, int i2 ) const
{
	return IsCCW(m_PointList[i0].m_Pos,m_PointList[i1].m_Pos,m_PointList[i2].m_Pos);
}

BrushFloat CBrushDesignerPolygonDecomposer::IsCCW( const BrushVec2& prev, const BrushVec2& current, const BrushVec2& next ) const
{
	const BrushVec3& v0(prev);
	const BrushVec3& v1(current);
	const BrushVec3& v2(next);

	BrushVec3 v10 = (v0-v1).GetNormalized();
	BrushVec3 v12 = (v2-v1).GetNormalized();

	BrushVec3 v10_x_v12 = v10.Cross(v12);

	return v10_x_v12.z > -kDecomposerEpsilon;
}

BrushFloat CBrushDesignerPolygonDecomposer::IsCW( const BrushVec2& prev, const BrushVec2& current, const BrushVec2& next ) const
{
	return !IsCCW(prev,current,next);
}

bool CBrushDesignerPolygonDecomposer::IsCCW( const IndexList& indexList, int nCurr ) const
{
	int nIndexListCount = indexList.size();
	return IsCCW(indexList[((nCurr-1)+nIndexListCount)%nIndexListCount], indexList[nCurr], indexList[(nCurr+1)%nIndexListCount]);
}

bool CBrushDesignerPolygonDecomposer::IsConvex( const IndexList& indexList, bool bGetPrevNextFromPointList ) const
{
	int iIndexCount(indexList.size());
	if( iIndexCount <= 3 )
		return true;

	for( int i = 0; i < iIndexCount; ++i )
	{
		int nPrev = bGetPrevNextFromPointList ? m_PointList[indexList[i]].m_nPrevIndex : indexList[((i-1)+iIndexCount)%iIndexCount];
		int nNext = bGetPrevNextFromPointList ? m_PointList[indexList[i]].m_nNextIndex : indexList[(i+1)%iIndexCount];
		if( !IsCCW( nPrev, indexList[i], nNext ) )
			return false;
	}

	return true;
}

BrushFloat CBrushDesignerPolygonDecomposer::Cosine( int i0, int i1, int i2 ) const
{
	const BrushVec2 p10 = (m_PointList[i0].m_Pos-m_PointList[i1].m_Pos).GetNormalized();
	const BrushVec2 p12 = (m_PointList[i2].m_Pos-m_PointList[i1].m_Pos).GetNormalized();	
	return p10.Dot(p12);
}

bool CBrushDesignerPolygonDecomposer::GetNextIndex( int nCurr, const IndexList& indices, int& nOutNextIndex ) const
{
	for( int i = 0, iSize(indices.size()); i < iSize; ++i )
	{
		if( indices[i] == nCurr )
		{
			nOutNextIndex = indices[(i+1)%iSize];
			return true;
		}
	}
	return false;
}

bool CBrushDesignerPolygonDecomposer::HasArea( int i0, int i1, int i2 ) const
{
	if( i0 == i1 || i0 == i2 || i1 == i2 )
		return false;

	const BrushVec2& p0 = m_PointList[i0].m_Pos;
	const BrushVec2& p1 = m_PointList[i1].m_Pos;
	const BrushVec2& p2 = m_PointList[i2].m_Pos;

	if( p0.IsEquivalent(p1,kDecomposerEpsilon) || p0.IsEquivalent(p2,kDecomposerEpsilon) || p1.IsEquivalent(p2,kDecomposerEpsilon) )
		return false;

	if( IsColinear(p0, p1, p2) )
		return false;

	return true;
}

void CBrushDesignerPolygonDecomposer::AddFace( int i0, int i1, int i2, const IndexList& indices, std::vector<SMeshFace>& outFaceList ) const
{
	if( !HasArea(i0,i1,i2) )
		return;

	bool bAddedSubTriangles = false;
	int faceIndex[3] = {i0,i1,i2};
	for( int i = 0; i < 3; ++i )
	{
		int nPrev = faceIndex[((i-1)+3)%3];
		int nCurr = faceIndex[i];
		int nNext = faceIndex[(i+1)%3];

		int nIndex = 0;
		int nPrevIndex = nNext;
		if( !GetNextIndex( nPrevIndex, indices, nIndex ) )
			return;
		int nCounter = 0;
		while( nIndex != nCurr )
		{
			if( IsInsideEdge(nNext,nPrev,nIndex) )
			{				
				if( !(nCurr == i0 && nPrevIndex == i1 && nIndex == i2 || nCurr == i1 && nPrevIndex == i2 && nIndex == i0 || nCurr == i2 && nPrevIndex == i0 && nIndex == i1) )
				{
					bAddedSubTriangles = true;
					AddTriangle(nCurr,nPrevIndex,nIndex,outFaceList);
				}
				nPrevIndex = nIndex;
			}			
			if( !GetNextIndex( nIndex, indices, nIndex ) )
				break;
			if( (nCounter++) > indices.size() )
			{
				DESIGNER_ASSERT(0 && "This seems to fall into a infinite loop");
#ifdef DEBUG
				CallDebugger();
#endif
				break;
			}
		}
	}

	if( !bAddedSubTriangles )
		AddTriangle(i0,i1,i2,outFaceList);
}

void CBrushDesignerPolygonDecomposer::AddTriangle( int i0, int i1, int i2, std::vector<SMeshFace>& outFaceList ) const
{
	if( !HasArea(i0,i1,i2) )
		return;	

	SMeshFace newFace;
	int nBaseVertexIndex = m_nBasedVertexIndex + m_VertexOffset;
	int _i0 = i0 + nBaseVertexIndex;
	int _i1 = i1 + nBaseVertexIndex;
	int _i2 = i2 + nBaseVertexIndex;
	if( HasAlreadyAdded(_i0,_i1,_i2,outFaceList) )
		return;

	if( m_bGenerateConvexes )
	{
		IndexList triangle(3);
		triangle[0] = i0;
		triangle[1] = i1;
		triangle[2] = i2;
		int nConvexIndex = m_Convexes.size();	
		m_Convexes.push_back(triangle);

		std::pair<int,int> e[3] = { GetSortedEdgePair(i0,i1), GetSortedEdgePair(i1,i2), GetSortedEdgePair(i2,i0) };
		for( int i = 0; i < 3; ++i )
		{
			if( m_InitialEdgesSortedByEdge.find(e[i]) != m_InitialEdgesSortedByEdge.end() )
				continue;
			m_ConvexesSortedByEdge[e[i]].push_back(nConvexIndex);
			m_EdgesSortedByConvex[nConvexIndex].insert(e[i]);
		}
	}

	newFace.v[0] = _i0;
	newFace.v[1] = _i1;
	newFace.v[2] = _i2;

	outFaceList.push_back(newFace);
}

bool CBrushDesignerPolygonDecomposer::IsInside( CBrushBSPTree2D* pBSPTree, int i0, int i1 ) const
{
	if( !pBSPTree )
		return true;
	return pBSPTree->IsInside(BrushEdge3D(m_VertexList[i0],m_VertexList[i1]),false);
}

bool CBrushDesignerPolygonDecomposer::IsOnEdge( CBrushBSPTree2D* pBSPTree, int i0, int i1 ) const
{
	if( !pBSPTree )
		return true;
	return pBSPTree->IsOnEdge(BrushEdge3D(m_VertexList[i0],m_VertexList[i1]));
}

bool CBrushDesignerPolygonDecomposer::IsColinear( const BrushVec2& p0, const BrushVec2& p1, const BrushVec2& p2 ) const
{
	BrushVec2 p10 = (p0-p1).GetNormalized();
	BrushVec2 p12 = (p2-p1).GetNormalized();

	if( p10.IsEquivalent(p12,kDecomposerEpsilon) || p10.IsEquivalent(-p12,kDecomposerEpsilon))
		return true;

	return false;
}

bool CBrushDesignerPolygonDecomposer::HasAlreadyAdded( int i0, int i1, int i2, const std::vector<SMeshFace>& faceList ) const
{
	for( int i = m_FaceOffset, iFaceCount(faceList.size()); i < iFaceCount; ++i )
	{
		if( faceList[i].v[0] == i0 && ( faceList[i].v[1] == i1 && faceList[i].v[2] == i2 || faceList[i].v[1] == i2 && faceList[i].v[2] == i1 ) )
			return true;
		if( faceList[i].v[1] == i0 && ( faceList[i].v[0] == i1 && faceList[i].v[2] == i2 || faceList[i].v[0] == i2 && faceList[i].v[2] == i1 ) )
			return true;
		if( faceList[i].v[2] == i0 && ( faceList[i].v[0] == i1 && faceList[i].v[1] == i2 || faceList[i].v[0] == i2 && faceList[i].v[1] == i1 ) )
			return true;
	}
	return false;
}

bool CBrushDesignerPolygonDecomposer::IsInsideEdge(int i0, int i1, int i2) const
{
	const BrushVec2& v0 = m_PointList[i0].m_Pos;
	const BrushVec2& v1 = m_PointList[i1].m_Pos;
	const BrushVec2& v2 = m_PointList[i2].m_Pos;

	return BrushEdge(v0,v1).IsInside(v2,kDecomposerEpsilon);
}

bool CBrushDesignerPolygonDecomposer::IsDifferenceOne( int i0, int i1, const IndexList& indexList ) const
{
	int nIndexCount = indexList.size();
	if( i0 == 0 && i1 == nIndexCount-1 || i0 == nIndexCount-1 && i1 == 0 )
		return true;
	return std::abs(i0-i1) == 1;
}

int CBrushDesignerPolygonDecomposer::FindLeftTopVertexIndex( const IndexList& indexList ) const
{
	return FindExtreamVertexIndex< std::less<YXPair>>(indexList);
}

int CBrushDesignerPolygonDecomposer::FindRightBottomVertexIndex( const IndexList& indexList ) const
{
	return FindExtreamVertexIndex< std::greater<YXPair>>(indexList);
}

template<class _Pr>
int CBrushDesignerPolygonDecomposer::FindExtreamVertexIndex( const IndexList& indexList ) const
{
	std::map< YXPair, int, _Pr > YXSortedList;

	for( int i = 0, nIndexBuffSize(indexList.size()); i < nIndexBuffSize; ++i )	
		YXSortedList[YXPair(m_PointList[indexList[i]].m_Pos.y,m_PointList[indexList[i]].m_Pos.x)] = i;

	DESIGNER_ASSERT(!YXSortedList.empty());
#ifdef DEBUG
	if( YXSortedList.empty() )
		CallDebugger();
#endif

	return YXSortedList.begin()->second;
}

CBrushDesignerPolygonDecomposer::EMarkSide CBrushDesignerPolygonDecomposer::QueryMarkSide( int nIndex, const IndexList& indexList, int nLeftTopIndex, int nRightBottomIndex ) const
{
	int nBoundary[] = { nLeftTopIndex, nRightBottomIndex };
	CBrushDesignerPolygonDecomposer::EMarkSide markSide[] = { eMarkSide_Left, eMarkSide_Right };
	int nIndexBuffSize = indexList.size();

	for( int i = 0, iSize(sizeof(nBoundary)/sizeof(*nBoundary)); i < iSize; ++i )
	{
		int nTraceIndex = nBoundary[i];
		while( nTraceIndex != nBoundary[1-i] )
		{
			if( nTraceIndex == nIndex )
				return markSide[i];
			++nTraceIndex;
			if( nTraceIndex >= nIndexBuffSize )
				nTraceIndex = 0;
		}
	}

	DESIGNER_ASSERT(0 && "Return Value should be either eMarkSide_Left or eMarkSide_Right");
#ifdef ENABLE_OUTPUT_DEBUGINFO
	CallDebugger();
#endif

	return eMarkSide_Invalid;
}

int CBrushDesignerPolygonDecomposer::FindDirectlyLeftEdge( int nBeginIndex, const IndexList& edgeSearchList, const IndexList& indexList ) const
{
	int nIndexCount = indexList.size();
	const BrushVec2& point = m_PointList[nBeginIndex].m_Pos;
	const BrushVec2 leftDir(-1.0f,0);
	BrushFloat nearestDist = 3e10f;
	int nNearestEdge = -1;

	for( int i = 0, iEdgeCount(edgeSearchList.size()); i < iEdgeCount; ++i )
	{
		int nIndex = indexList[edgeSearchList[i]];
		int nNextIndex = m_PointList[nIndex].m_nNextIndex;
		BrushEdge edge(m_PointList[nIndex].m_Pos, m_PointList[nNextIndex].m_Pos);
		BrushLine line(edge.m_v[0],edge.m_v[1]);

		if( line.Distance(point) > 0 )
			continue;

		if( point.y < edge.m_v[0].y-kComparisonEpsilon || point.y > edge.m_v[1].y+kComparisonEpsilon )
			continue;

		BrushFloat dist;
		if( !line.HitTest(point, point+leftDir, kDecomposerEpsilon, &dist) )
		{
			if( IsSame(edge.m_v[0].y,point.y) && edge.m_v[1].IsEquivalent(point,kDecomposerEpsilon) )
				dist = (point-edge.m_v[0]).GetLength();
			else	
				continue;
		}

		if( dist < nearestDist )
		{
			nNearestEdge = edgeSearchList[i];
			nearestDist = dist;
		}
	}

	return nNearestEdge;
}

void CBrushDesignerPolygonDecomposer::EraseElement( int nIndex, IndexList& indexList ) const
{
	for( int i = 0, indexCount(indexList.size()); i < indexCount; ++i )
	{
		if( indexList[i] != nIndex )
			continue;
		indexList.erase(indexList.begin()+i);
		return;
	}
}

void CBrushDesignerPolygonDecomposer::AddDiagonalEdge( int i0, int i1, BUtil::EdgeSet& diagonalSet ) const
{
	int i0Prev = m_PointList[i0].m_nPrevIndex;
	int i0Next = m_PointList[i0].m_nNextIndex;
	int i1Prev = m_PointList[i1].m_nPrevIndex;
	int i1Next = m_PointList[i1].m_nNextIndex;

	if( i0Prev == i1 || i0Next == i1 || i1Prev == i0 || i1Next == i0 )
		return;

	if( IsSame(m_PointList[i0].m_Pos.y, m_PointList[i1].m_Pos.y) )
	{
		if( i1 != i0Prev && IsSame(m_PointList[i0].m_Pos.y,m_PointList[i0Prev].m_Pos.y) && IsInsideEdge(i0,i1,i0Prev) )
		{
			diagonalSet.insert(BUtil::SEdge(i1,i0Prev));
			diagonalSet.insert(BUtil::SEdge(i0Prev,i1));
			return;
		}
		else if( i1 != i0Next && IsSame(m_PointList[i0].m_Pos.y,m_PointList[i0Next].m_Pos.y) && IsInsideEdge(i0,i1,i0Next) )
		{
			diagonalSet.insert(BUtil::SEdge(i1,i0Next));
			diagonalSet.insert(BUtil::SEdge(i0Next,i1));
			return;
		}
		else if( i0 != i1Prev && IsSame(m_PointList[i1].m_Pos.y,m_PointList[i1Prev].m_Pos.y) && IsInsideEdge(i0,i1,i1Prev) )
		{
			diagonalSet.insert(BUtil::SEdge(i0,i1Prev));
			diagonalSet.insert(BUtil::SEdge(i1Prev,i0));
			return;
		}
		else if( i0 != i1Next && IsSame(m_PointList[i1].m_Pos.y,m_PointList[i1Next].m_Pos.y) && IsInsideEdge(i0,i1,i1Next) )
		{
			diagonalSet.insert(BUtil::SEdge(i0,i1Next));
			diagonalSet.insert(BUtil::SEdge(i1Next,i0));
			return;
		}
	}

	diagonalSet.insert(BUtil::SEdge(i0,i1));
	diagonalSet.insert(BUtil::SEdge(i1,i0));
}

bool CBrushDesignerPolygonDecomposer::SplitIntoMonotonePieces( const IndexList& indexList, std::vector<IndexList>& outMonatonePieces )
{
	int nIndexCount(indexList.size());

	std::vector<EMarkType> markTypeList;
	IndexList helperList;
	IndexList edgeSearchList;
	BUtil::EdgeSet diagonalSet;
	std::vector<EInteriorDir> interiorDirList;

	std::map< SMark, int, std::less<SMark>> yMap;

	for( int i = 0, nIndexCount(indexList.size()); i < nIndexCount; ++i )
	{
		markTypeList.push_back(QueryMarkType(i,indexList));
		SMark newMark(m_PointList[indexList[i]].m_Pos.y,(int)((m_PointList[indexList[i]].m_Pos.x)*10000.0f));
		if( yMap.find(newMark) != yMap.end() )
			--newMark.m_XPriority;
		yMap[newMark] = i;
		helperList.push_back(-1);
	}

	std::map< SMark, int, std::less<SMark>>::iterator yIter = yMap.begin();
	for( ; yIter != yMap.end(); ++yIter )
	{ 
		const SMark& top = yIter->first;
		int index = yIter->second;
		int prev = m_PointList[index].m_nPrevIndex;

		switch(markTypeList[index])
		{
		case eMarkType_Start:
			helperList[index] = index;
			edgeSearchList.push_back(index);
			break;

		case eMarkType_End:
			if( helperList[prev] != -1 )
			{
				if( markTypeList[helperList[prev]] == eMarkType_Merge )
					AddDiagonalEdge(index,helperList[prev],diagonalSet);
				EraseElement(prev,edgeSearchList);
			}
			break;

		case eMarkType_Split:
			{
				int nDirectlyLeftEdgeIndex = FindDirectlyLeftEdge(index,edgeSearchList,indexList);

				DESIGNER_ASSERT( nDirectlyLeftEdgeIndex != -1 );
#ifdef ENABLE_OUTPUT_DEBUGINFO
				if( nDirectlyLeftEdgeIndex == -1 )
					CallDebugger();
#endif

				if( nDirectlyLeftEdgeIndex != -1 )
				{
					AddDiagonalEdge(index,helperList[nDirectlyLeftEdgeIndex],diagonalSet);
					helperList[nDirectlyLeftEdgeIndex] = index;
				}
				edgeSearchList.push_back(index);
				helperList[index] = index;
			}
			break;

		case eMarkType_Merge:
			{
				if( helperList[prev] != -1 )
				{
					if( markTypeList[helperList[prev]] == eMarkType_Merge )
						AddDiagonalEdge(index,helperList[prev],diagonalSet);
				}
				EraseElement(prev,edgeSearchList);
				int nDirectlyLeftEdgeIndex = FindDirectlyLeftEdge(index,edgeSearchList,indexList);
				DESIGNER_ASSERT( nDirectlyLeftEdgeIndex != -1 );
#ifdef ENABLE_OUTPUT_DEBUGINFO
				if( nDirectlyLeftEdgeIndex == -1 )
					CallDebugger();
#endif
				if( nDirectlyLeftEdgeIndex != -1 )
				{
					if( markTypeList[helperList[nDirectlyLeftEdgeIndex]] == eMarkType_Merge )
						AddDiagonalEdge(index,helperList[nDirectlyLeftEdgeIndex],diagonalSet);
					helperList[nDirectlyLeftEdgeIndex] = index;
				}
			}
			break;

		case eMarkType_Regular:
			{
				EInteriorDir interiorDir = QueryInteriorDirection(index,indexList);
				if( interiorDir == eInteriorDir_Right )
				{
					if( helperList[prev] != -1 )
					{
						if( markTypeList[helperList[prev]] == eMarkType_Merge )
							AddDiagonalEdge(index,helperList[prev],diagonalSet);
					}
					EraseElement(prev,edgeSearchList);
					edgeSearchList.push_back(index);
					helperList[index] = index;
				}
				else if( interiorDir == eInteriorDir_Left )
				{
					int nDirectlyLeftEdgeIndex = FindDirectlyLeftEdge(index,edgeSearchList,indexList);
					DESIGNER_ASSERT( nDirectlyLeftEdgeIndex != -1 );
#ifdef ENABLE_OUTPUT_DEBUGINFO
					if( nDirectlyLeftEdgeIndex == -1 )
						CallDebugger();

#endif
					if( nDirectlyLeftEdgeIndex != -1 )
					{
						if( markTypeList[helperList[nDirectlyLeftEdgeIndex]] == eMarkType_Merge )
							AddDiagonalEdge(index,helperList[nDirectlyLeftEdgeIndex],diagonalSet); 
						helperList[nDirectlyLeftEdgeIndex] = index;
					}
				}
				else
				{
					DESIGNER_ASSERT( 0 && "Interior should lie to the right or the left of point.");
#ifdef ENABLE_OUTPUT_DEBUGINFO
					CallDebugger();
#endif
				}
			}
			break;
		}
	}

	SearchMonotoneLoops(diagonalSet, indexList, outMonatonePieces);
	return true;
}

void CBrushDesignerPolygonDecomposer::SearchMonotoneLoops( BUtil::EdgeSet& diagonalSet, const IndexList& indexList, std::vector<IndexList>& monotonePieces ) const
{
	int iIndexListCount(indexList.size());

	BUtil::EdgeSet handledEdgeSet;
	BUtil::EdgeSet edgeSet;
	BUtil::EdgeMap edgeMap;

	for( int i = 0; i < iIndexListCount; ++i )
	{
		edgeSet.insert(BUtil::SEdge(indexList[i],m_PointList[indexList[i]].m_nNextIndex));
		edgeMap[indexList[i]].insert(m_PointList[indexList[i]].m_nNextIndex);
	}

	BUtil::EdgeSet::iterator iDiagonalEdgeSet = diagonalSet.begin();
	for( ;iDiagonalEdgeSet != diagonalSet.end(); ++iDiagonalEdgeSet )
	{
		edgeSet.insert(*iDiagonalEdgeSet);
		edgeMap[(*iDiagonalEdgeSet).m_i[0]].insert((*iDiagonalEdgeSet).m_i[1]);
	}

	int nCounter(0);
	BUtil::EdgeSet::iterator iEdgeSet = edgeSet.begin();
	for(iEdgeSet = edgeSet.begin() ;iEdgeSet != edgeSet.end(); ++iEdgeSet )
	{
		if( handledEdgeSet.find(*iEdgeSet) != handledEdgeSet.end() )
			continue;

		IndexList monotonePiece;
		BUtil::SEdge edge = *iEdgeSet;
		handledEdgeSet.insert(edge);

		do
		{
			monotonePiece.push_back(edge.m_i[0]);

			DESIGNER_ASSERT( edgeMap.find(edge.m_i[1]) != edgeMap.end() );
#ifdef ENABLE_OUTPUT_DEBUGINFO
			if( edgeMap.find(edge.m_i[1]) == edgeMap.end() )
				CallDebugger();
#endif

			BUtil::EdgeIndexSet& secondIndexSet = edgeMap[edge.m_i[1]];

			if( secondIndexSet.size() == 1 )
			{
				edge = BUtil::SEdge(edge.m_i[1],*secondIndexSet.begin());
				secondIndexSet.clear();
			}
			else if( secondIndexSet.size() > 1 )
			{
				BrushFloat ccwCosMax = -1.5f;
				BrushFloat cwCosMin = 1.5f;
				int ccwIndex = -1;
				int cwIndex = -1;
				BUtil::EdgeIndexSet::iterator iSecondIndexSet = secondIndexSet.begin();
				for( ; iSecondIndexSet != secondIndexSet.end() ; ++iSecondIndexSet )
				{
					if( *iSecondIndexSet == edge.m_i[0] )
						continue;
					BrushFloat cosine = Cosine(edge.m_i[0], edge.m_i[1], *iSecondIndexSet);
					if( IsCCW(edge.m_i[0], edge.m_i[1], *iSecondIndexSet) )
					{
						if( cosine > ccwCosMax )
						{
							ccwIndex = *iSecondIndexSet;
							ccwCosMax = cosine;
						}
					}  
					else if( cosine < cwCosMin )
					{
						cwIndex = *iSecondIndexSet;
						cwCosMin = cosine;
					}
				}

				if( ccwIndex != -1 )
				{
					edge = BUtil::SEdge(edge.m_i[1],ccwIndex);
					secondIndexSet.erase(ccwIndex);
				}
				else
				{
					edge = BUtil::SEdge(edge.m_i[1],cwIndex);
					secondIndexSet.erase(cwIndex);
				}
			}
			handledEdgeSet.insert(edge);
			if(++nCounter>10000)
			{
				DESIGNER_ASSERT(0 && "CBrushDesignerPolygonDecomposer::SearchMonotoneLoops() - Searching connected an edge doesn't seem possible.");
#ifdef ENABLE_OUTPUT_DEBUGINFO
				CallDebugger();
#endif
				return;
			}
		}while( edge.m_i[1] != (*iEdgeSet).m_i[0] );

		monotonePiece.push_back(edge.m_i[0]);
		monotonePieces.push_back(monotonePiece);
	}
}

CBrushDesignerPolygonDecomposer::EMarkType CBrushDesignerPolygonDecomposer::QueryMarkType( int nIndex, const IndexList& indexList ) const
{
	int iIndexCount(indexList.size());

	int nCurrIndex = indexList[nIndex];
	int nPrevIndex = m_PointList[nCurrIndex].m_nPrevIndex;
	int nNextIndex = m_PointList[nCurrIndex].m_nNextIndex;

	const BrushVec2& prev = m_PointList[nPrevIndex].m_Pos;
	const BrushVec2& current = m_PointList[nIndex].m_Pos;
	const BrushVec2& next = m_PointList[nNextIndex].m_Pos;

	bool bCCW = IsCCW(prev,current,next);
	bool bCW = !bCCW;

	if( IsSame(prev.y,current.y) )
	{
		if( bCW )
		{
			if( QueryInteriorDirection(nIndex, indexList) == eInteriorDir_Left )
				return eMarkType_Merge;
			else
				return eMarkType_Regular;
		}
		else if( IsGreaterEqual(next.y,current.y) )
			return eMarkType_Start;
		else
			return eMarkType_End;
	}
	else if( IsSame(current.y,next.y) )
	{
		if( bCW )
		{
			if( QueryInteriorDirection(nIndex, indexList) == eInteriorDir_Right )
				return eMarkType_Regular;
			else
				return eMarkType_Split;
		}
		else if( IsGreaterEqual(prev.y,current.y) )
			return eMarkType_Start;
		else
			return eMarkType_End;
	}
	else if( IsGreaterEqual(prev.y,current.y) && IsGreaterEqual(next.y,current.y) )
	{
		if( bCCW )
			return eMarkType_Start;
		else
			return eMarkType_Split;
	}
	else if( IsLessEqual(prev.y,current.y) && IsLessEqual(next.y,current.y) )
	{
		if( bCCW )
			return eMarkType_End;
		else
			return eMarkType_Merge;
	}
	else if( IsLessEqual(prev.y,current.y) && IsGreaterEqual(next.y,current.y) || IsGreaterEqual(prev.y,current.y) && IsLessEqual(next.y,current.y) )
	{
		return eMarkType_Regular;
	}

	DESIGNER_ASSERT( 0 && "eMarkType_Invalid can't be returned.");
#ifdef ENABLE_OUTPUT_DEBUGINFO
	CallDebugger();
#endif

	return eMarkType_Invalid;
}

CBrushDesignerPolygonDecomposer::EInteriorDir CBrushDesignerPolygonDecomposer::QueryInteriorDirection( int nIndex, const IndexList& indexList ) const
{
	int nIndexSize = indexList.size();

	int nCurrIndex = indexList[nIndex];
	int nPrevIndex = m_PointList[nCurrIndex].m_nPrevIndex;
	int nNextIndex = m_PointList[nCurrIndex].m_nNextIndex;

	const BrushVec2& prev = m_PointList[nPrevIndex].m_Pos;
	const BrushVec2& current = m_PointList[nCurrIndex].m_Pos;
	const BrushVec2& next = m_PointList[nNextIndex].m_Pos;

	if( IsGreaterEqual(prev.y,current.y) && IsGreaterEqual(current.y,next.y) )
		return eInteriorDir_Left;

	if( IsLessEqual(prev.y,current.y) && IsLessEqual(current.y,next.y) )
		return eInteriorDir_Right;

	return eInteriorDir_Invalid;
}

void CBrushDesignerPolygonDecomposer::RemoveIndexWithSameAdjacentPoint( IndexList& indexList ) const
{
	IndexList::iterator ii = indexList.begin();
	int nStart = *ii;

	for( ; ii != indexList.end(); )
	{
		int nCurr = *ii;
		int nNext = (ii+1) == indexList.end() ? nStart : *(ii+1);

		if( m_VertexList[nCurr].IsEquivalent(m_VertexList[nNext],kDecomposerEpsilon) )
			ii = indexList.erase(ii);
		else
			++ii;
	}
}

void CBrushDesignerPolygonDecomposer::CallDebugger() const
{
#ifdef ENABLE_OUTPUT_DEBUGINFO
	m_pRegion->OutputTestCode();
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	dlg->AddRegion(m_pRegion.get(),"Region");
	dlg->Open();
#endif
}

void CBrushDesignerPolygonDecomposer::FindMatchedConnectedVertexIndices( int iV0, int iV1, const IndexList& indexList, int& nOutIndex0, int& nOutIndex1 ) const
{
	int i0 = -1, i1 = -1;
	for( int i = 0, iVertexCount(indexList.size()); i < iVertexCount; ++i )
	{
		int nNext = (i+1)%iVertexCount;
		if( indexList[i] == iV0 && indexList[nNext] == iV1 || indexList[nNext] == iV0 && indexList[i] == iV1 )
		{
			i0 = i;
			i1 = nNext;
			break;
		}
	}
	nOutIndex0 = i0;
	nOutIndex1 = i1;
}

bool CBrushDesignerPolygonDecomposer::MergeTwoConvexes( int iV0, int iV1, int iConvex0, int iConvex1, IndexList& outMergedPolygon )
{
	DESIGNER_ASSERT(!m_Convexes[iConvex0].empty());
	DESIGNER_ASSERT(!m_Convexes[iConvex1].empty());
	if( m_Convexes[iConvex0].empty() || m_Convexes[iConvex1].empty() )
		return false;

	int i00 = -1, i01 = -1;
	FindMatchedConnectedVertexIndices( iV0, iV1, m_Convexes[iConvex0], i00, i01 );
	DESIGNER_ASSERT( i00 != -1 && i01 != -1 );
	if( i00 == -1 || i01 == -1 )
		return false;

	int i10 = -1, i11 = -1;
	FindMatchedConnectedVertexIndices( iV0, iV1, m_Convexes[iConvex1], i10, i11 );
	DESIGNER_ASSERT( i10 != -1 && i11 != -1 );
	if( i10 == -1 || i11 == -1 )
		return false;

	outMergedPolygon.clear();
	for( int i = 0, iVertexCount(m_Convexes[iConvex0].size()); i < iVertexCount; ++i )
		outMergedPolygon.push_back(m_Convexes[iConvex0][(i+i01)%iVertexCount]);
	for( int i = 0, iVertexCount(m_Convexes[iConvex1].size()); i < iVertexCount-2; ++i )
		outMergedPolygon.push_back(m_Convexes[iConvex1][(i+i11+1)%iVertexCount]);

	return true;
}

void CBrushDesignerPolygonDecomposer::CreateConvexes()
{
	if( !m_bGenerateConvexes )
		return;

	std::map<std::pair<int,int>,std::vector<int>>::iterator ii = m_ConvexesSortedByEdge.begin();
	for( ;ii != m_ConvexesSortedByEdge.end(); ++ii )
	{
		DESIGNER_ASSERT(ii->second.size()==2);
		if( ii->second.size()!=2 )
			continue;

		int i0 = ii->first.first;
		int i1 = ii->first.second;

		bool bReflex0 = IsCW(m_PointList[i0].m_nPrevIndex,i0,m_PointList[i0].m_nNextIndex);
		bool bReflex1 = IsCW(m_PointList[i1].m_nPrevIndex,i1,m_PointList[i1].m_nNextIndex);

		IndexList mergedPolygon;
		bool bIsResultConvex = false;
		if( !bReflex0 && !bReflex1 )
		{
			bIsResultConvex = MergeTwoConvexes(i0,i1,ii->second[0],ii->second[1],mergedPolygon);
		}
		else
		{
			if( MergeTwoConvexes(i0,i1,ii->second[0],ii->second[1],mergedPolygon) )
				bIsResultConvex = IsConvex(mergedPolygon,false);
		}

		if( bIsResultConvex )
		{
			int nConvexIndex = ii->second[0];
			m_Convexes[nConvexIndex].clear();
			m_Convexes[ii->second[1]] = mergedPolygon;
			std::set<std::pair<int,int>>::iterator iEdge = m_EdgesSortedByConvex[nConvexIndex].begin();
			for( ;iEdge != m_EdgesSortedByConvex[nConvexIndex].end(); ++iEdge )
			{
				if( nConvexIndex == m_ConvexesSortedByEdge[*iEdge][0] )
				{
					m_ConvexesSortedByEdge[*iEdge][0] = ii->second[1];
					m_EdgesSortedByConvex[ii->second[1]].insert(*iEdge);
				}
				if( nConvexIndex == m_ConvexesSortedByEdge[*iEdge][1] )
				{
					m_ConvexesSortedByEdge[*iEdge][1] = ii->second[1];
					m_EdgesSortedByConvex[ii->second[1]].insert(*iEdge);
				}
			}
			m_EdgesSortedByConvex.erase(nConvexIndex);
		}
	}

	for( int i = 0, iConvexCount(m_Convexes.size()); i < iConvexCount; ++i )
	{
		if( m_Convexes[i].empty() )
			continue;
		BUtil::Convex convex;
		for( int k = 0, iVListCount(m_Convexes[i].size()); k < iVListCount; ++k )
			convex.push_back(m_VertexList[m_Convexes[i][k]]);
		m_pBrushConvexes->AddConvex(convex);
	}
}
