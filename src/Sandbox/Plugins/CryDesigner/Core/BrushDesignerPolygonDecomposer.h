#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
// -------------------------------------------------------------------------
//  File name:   BrushDesignerPolygonDecomposer.h
//  Created:     Feb/26/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "BrushRegion.h"

class CBrushBSPTree2D;
class CBrushConvexes;

namespace BUtil
{
	enum EDecomposerFlag
	{
		eDF_SkipOptimizationOfRegionResults = BIT(0)
	};
}

class CBrushDesignerPolygonDecomposer
{
public:
	CBrushDesignerPolygonDecomposer( int nFlag = 0 ) : m_nFlag(nFlag), m_bGenerateConvexes(false)
	{
	}

	void CreateConvexes( CBrushRegion::RegionPtr pRegion, CBrushConvexes& outConvexes );

	bool TriangulateRegion( CBrushRegion::RegionPtr pRegion, std::vector<BrushVec3>& outVertexList, std::vector<BrushVec3>& outNormalList, std::vector<SMeshFace>& outFaceList, int vertexOffset = 0, int faceOffset = 0 );
	bool TriangulateRegion( CBrushRegion::RegionPtr pRegion, BUtil::VertexList& outVertexList, BUtil::FaceList& outFaceList );
	bool TriangulateRegion( CBrushRegion::RegionPtr pRegion, std::vector<CBrushRegion::RegionPtr>& outTrianguleRegions );

private:
	enum EMarkSide
	{
		eMarkSide_Invalid = 0,
		eMarkSide_Left	= 1,
		eMarkSide_Right = 2
	};

	enum EMarkType
	{
		eMarkType_Invalid,
		eMarkType_Start,
		eMarkType_End,
		eMarkType_Regular,
		eMarkType_Split,
		eMarkType_Merge
	};

	enum EInteriorDir
	{
		eInteriorDir_Invalid,
		eInteriorDir_Right,
		eInteriorDir_Left
	};

	struct SPointInfo
	{
		SPointInfo( const BrushVec2& pos, int nPrevIndex, int nNextIndex ) :
			m_Pos(pos),
			m_nPrevIndex(nPrevIndex),
			m_nNextIndex(nNextIndex)
		{
		}
		BrushVec2 m_Pos;
		int m_nPrevIndex;
		int m_nNextIndex;
	};

	typedef std::vector<int> IndexList;

private:
	static const BrushFloat kComparisonEpsilon;

	struct SFloat
	{
		SFloat( BrushFloat rhs ) :
			v(rhs)
		{
		}
		BrushFloat v;
		bool operator < ( const SFloat& value ) const
		{
			if( v - value.v < -kComparisonEpsilon )
				return true;
			return false;
		}
	};

	struct SMark 
	{
		SMark( const BrushFloat yPos, int xPriority )
		{
			m_yPos = yPos;
			m_XPriority = xPriority;
		}

		BrushFloat m_yPos;
		int m_XPriority;

		bool operator < ( const SMark& mark ) const
		{
			if( m_yPos-mark.m_yPos < -kComparisonEpsilon )
				return true;
			if( fabs(m_yPos-mark.m_yPos) < kComparisonEpsilon && m_XPriority < mark.m_XPriority )
				return true;
			return false;
		}

		bool operator > ( const SMark& mark ) const
		{
			if( m_yPos-mark.m_yPos > kComparisonEpsilon )
				return true;
			if( fabs(m_yPos-mark.m_yPos) < kComparisonEpsilon && m_XPriority > mark.m_XPriority )
				return true;
			return false;
		}
	};

private:
	typedef bool (CBrushDesignerPolygonDecomposer::*DecomposeRoutine)( IndexList* pIndexList, bool bHasInnerHull );
	bool Decompose( DecomposeRoutine pDecomposeRoutine );
	bool DecomposeToTriangules( IndexList* pIndexList, bool bHasInnerHull );

	bool TriangulateConvex( const IndexList& indexList, std::vector<SMeshFace>& outFaceList ) const;
	bool TriangulateConcave( const IndexList& indexList, std::vector<SMeshFace>& outFaceList );
	bool TriangulateMonotonePolygon( const IndexList& indexList, std::vector<SMeshFace>& outFaceList ) const;
	bool SplitIntoMonotonePieces( const IndexList& indexList, std::vector<IndexList>& outMonatonePieces );

	void BuildEdgeListHavingSameY( const IndexList& indexList, std::map<SFloat,IndexList>& outEdgeList ) const;
	void BuildMarkSideList( const IndexList& indexList, const std::map<SFloat,IndexList>& edgeListHavingSameY, std::vector<EMarkSide>& outMarkSideList, std::map<SMark,std::pair<int,int>>& outSortedMarksMap ) const;

	void AddTriangle( int i0, int i1, int i2, std::vector<SMeshFace>& outFaceList ) const;
	void AddFace( int i0, int i1, int i2, const IndexList& indices, std::vector<SMeshFace>& outFaceList ) const;
	bool IsInside( CBrushBSPTree2D* pBSPTree, int i0, int i1 ) const;
	bool IsOnEdge( CBrushBSPTree2D* pBSPTree, int i0, int i1 ) const;
	bool IsCCW( int i0, int i1, int i2 ) const;	
	bool IsCW( int i0, int i1, int i2 ) const{ return !IsCCW(i0,i1,i2);	}
	bool IsCCW( const IndexList& indexList, int nCurr ) const;
	bool IsCW( const IndexList& indexList, int nCurr ) const { return !IsCCW(indexList,nCurr); }
	BrushFloat IsCCW( const BrushVec2& prev, const BrushVec2& current, const BrushVec2& next ) const;
	BrushFloat IsCW( const BrushVec2& prev, const BrushVec2& current, const BrushVec2& next ) const;
	bool IsConvex( const IndexList& indexList, bool bGetPrevNextFromPointList ) const;
	BrushFloat Cosine( int i0, int i1, int i2 ) const;
	bool HasAlreadyAdded( int i0, int i1, int i2, const std::vector<SMeshFace>& faceList ) const;	
	bool IsColinear( const BrushVec2& p0, const BrushVec2& p1, const BrushVec2& p2 ) const;
	bool IsDifferenceOne( int i0, int i1, const IndexList& indexList ) const;
	bool IsInsideEdge( int i0, int i1, int i2 ) const;
	bool GetNextIndex( int nCurr, const IndexList& indices, int& nOutNextIndex ) const;
	bool HasArea( int i0, int i1, int i2 ) const;

	int FindLeftTopVertexIndex( const IndexList& indexList ) const;
	int FindRightBottomVertexIndex( const IndexList& indexList ) const;
	template<class _Pr>
	int FindExtreamVertexIndex( const IndexList& indexList ) const;

	EMarkSide QueryMarkSide( int nIndex, const IndexList& indexList, int nLeftTopIndex, int nRightBottomIndex ) const;
	EMarkType QueryMarkType( int nIndex, const IndexList& indexList ) const;
	EInteriorDir QueryInteriorDirection( int nIndex, const IndexList& indexList ) const;

	int FindDirectlyLeftEdge( int nBeginIndex, const IndexList& edgeSearchList, const IndexList& indexList ) const;
	void EraseElement( int nIndex, IndexList& edgeSearchList ) const;

	void SearchMonotoneLoops( BUtil::EdgeSet& diagonalSet, const IndexList& indexList, std::vector<IndexList>& monotonePieces ) const;
	void AddDiagonalEdge( int i0, int i1, BUtil::EdgeSet& diagonalList ) const;
	CBrushBSPTree2D* GenerateBSPTree( const IndexList& indexList ) const;

	void RemoveIndexWithSameAdjacentPoint( IndexList& indexList ) const;
	static void FillVertexListFromRegion( CBrushRegion::RegionPtr pRegion, std::vector<BrushVec3>& outVertexList );

	void CreateConvexes();
	void CallDebugger() const;

	int CheckFlag( int nFlag ) const { return m_nFlag & nFlag; }

private: // Related to Triangulation
	int m_nFlag;
	std::vector<BrushVec3>* m_pOutVertexList;
	std::vector<SMeshFace>* m_pOutFaceList;
	int m_VertexOffset;
	int m_FaceOffset;
	std::vector<BrushVec3> m_VertexList;
	std::vector<SPointInfo> m_PointList;
	BrushPlane m_Plane;
	short m_MaterialID;
	CBrushRegion::RegionPtr m_pRegion;
	int m_nBasedVertexIndex;
	bool m_bGenerateConvexes;

private: // Related to Decomposition into convexes
	std::pair<int,int> GetSortedEdgePair( int i0, int i1 ) const
	{
		if( i1 < i0 ) 
			std::swap(i0,i1);
		return std::pair<int,int>(i0,i1);
	}
	void FindMatchedConnectedVertexIndices( int iV0, int iV1, const IndexList& indexList, int& nOutIndex0, int& nOutIndex1 ) const;
	bool MergeTwoConvexes( int iV0, int iV1, int iConvex0, int iConvex1, IndexList& outMergedPolygon );
	void RemoveAllConvexData()
	{
		m_InitialEdgesSortedByEdge.clear();
		m_ConvexesSortedByEdge.clear();
		m_EdgesSortedByConvex.clear();
		m_Convexes.clear();
	}
	mutable std::set<std::pair<int,int>> m_InitialEdgesSortedByEdge;
	mutable std::map<std::pair<int,int>,std::vector<int>> m_ConvexesSortedByEdge;
	mutable std::map<int,std::set<std::pair<int,int>>> m_EdgesSortedByConvex;
	mutable std::vector<IndexList> m_Convexes;
	mutable CBrushConvexes* m_pBrushConvexes;
};
