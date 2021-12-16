#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesigner.h
//  Created:     August/12/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushRegion.h"

class CBrushBSPTree3D;
class CBrushDesignerDB;
class CBrushDesignerSmoothingGroupManager;
class CBrushDesignerEdgeSharpnessManager;
class CBrushDesignerHalfEdgeMesh;

class CBrushDesigner : public CRefCountBase
{
public:

	static const BrushFloat kDrillOffsetSize;

	enum EOperationType
	{
		eOpType_Split,
		eOpType_Union,
		eOpType_SubtractAB,
		eOpType_SubtractBA,
		eOpType_Intersection,
		eOpType_ExclusiveOR,
		eOpType_Add,
	};

	enum EHighlight
	{
		eHighlight_Enable = BIT(0),
		eHighlight_OthersReverse = BIT(1),
	};

	enum ESurroundType
	{
		eST_Surrounded,
		eST_Surrounding,
		eST_Partly,
		eST_None,
		eST_WrongInput,
	};

	enum EDesignerMode
	{
		eDesignerMode_DrillAfterOffset = BIT(1),
		eDesignerMode_Mirror = BIT(3),
		eDesignerMode_DisplayBackFace = BIT(4)
	};

	enum ERegionRelation
	{
		eER_None,
		eER_Intersection,
		eER_ZeroDistance
	}; 

	enum EFindOppositeFlag
	{
		eFOF_PushDirection,
		eFOF_PullDirection
	};

	struct SQueryEdgeResult
	{
		SQueryEdgeResult( CBrushRegion::RegionPtr pRegion, const BrushEdge3D& edge )
		{
			m_pRegion = pRegion;
			m_Edge = edge;
		}
		CBrushRegion::RegionPtr m_pRegion;
		BrushEdge3D m_Edge;
	};
	typedef std::pair<CBrushRegion::RegionPtr,BrushVec3> IntersectionPair;

	typedef std::vector<CBrushRegion::RegionPtr> RegionList;

public:

	CBrushDesigner();
	CBrushDesigner( const std::vector<CBrushRegion::RegionPtr>& regionList );
	CBrushDesigner( const CBrushDesigner& designer );
	~CBrushDesigner();

	CBrushDesigner& operator = ( const CBrushDesigner& designer );

	void Clear();
	void Display( DisplayContext& dc, const int nLineThickness = 2, const ColorB& lineColor = ColorB(0,0,2) );
	void DisplaySubdividedMesh( DisplayContext& dc );
	bool AddRegion( CBrushRegion::RegionPtr pRegion, EOperationType opType );
	void AddRegionSeparately( CBrushRegion::RegionPtr pRegion, bool bAddedOnlyAsSeparated = false );
	void AddRegionUnconditionally( CBrushRegion::RegionPtr pRegion );
	bool AddOpenRegion( CBrushRegion::RegionPtr pRegion, bool bOnlyAdd );
	bool DrillRegion( int nRegionIndex, bool bRemainFrame = false );
	bool DrillRegion( CBrushRegion::RegionPtr pRegion, bool bRemainFrame = false );
	bool DrillRegion( CBrushDesigner* pDesigner );

	CBrushRegion::RegionPtr QueryRegion( REFGUID guid ) const;
	bool QueryRegion( const BrushPlane& plane, const BrushVec3& raySrc, const BrushVec3& rayDir, int& nOutIndex ) const;
	bool QueryRegions( const BrushPlane& plane, RegionList& outRegions ) const;
	bool QueryIntersectedRegionsByAABB( const AABB& aabb, RegionList& outRegions ) const;
	bool QueryRegion( const BrushVec3& raySrc, const BrushVec3& rayDir, int& nOutIndex ) const;
	CBrushRegion::RegionPtr QueryEquivalentRegion( CBrushRegion::RegionPtr pRegion, int* pOutRegionIndex = NULL ) const;
	bool QueryNearestEdges( const BrushPlane& plane, const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outPos, BrushVec3& outPosOnEdge, std::vector<SQueryEdgeResult>& outEdges ) const;
	bool QueryNearestEdges( const BrushPlane& plane, const BrushVec3& position, BrushVec3& outPosOnEdge, std::vector<SQueryEdgeResult>& outEdges ) const;
	bool QueryNearestEdges( const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outPos, BrushVec3& outPosOnEdge, BrushPlane& outPlane, std::vector<SQueryEdgeResult>& outEdges ) const;
	bool QueryPosition( const BrushPlane& plane, const BrushVec3& localRayOrigin, const BrushVec3& localRayDir, BrushVec3& outPosition, BrushFloat* outDist = NULL, CBrushRegion::RegionPtr* outRegion = NULL ) const;
	bool QueryPosition( const BrushVec3& localRayOrigin, const BrushVec3& localRayDir, BrushVec3& outPosition, BrushPlane* outPlane = NULL, BrushFloat* outDist = NULL, CBrushRegion::RegionPtr* outRegion = NULL ) const;
	bool QueryEdgesHavingVertex( const BrushVec3& vertex, std::vector<BrushEdge3D>& outEdges ) const;
	bool QueryCenterOfRegion( const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outCenterOfPos ) const;
	bool QueryNearestPosFromBoundary( const BrushVec3& pos, BrushVec3& outNearestPos ) const;	
	bool QueryAdjacentRegionsByEdge( const BrushEdge3D& edge, std::vector<CBrushRegion::RegionPtr>& outRegions ) const;
	void QueryAdjacentPerpendicularRegions( CBrushRegion::RegionPtr pRegion, RegionList& outRegions ) const;
	void QueryPerpendicularRegions( CBrushRegion::RegionPtr pRegion, RegionList& outRegions ) const;
	ERegionRelation QueryOppositeRegion( CBrushRegion::RegionPtr pRegion, EFindOppositeFlag nFlag, BrushFloat fScale, CBrushRegion::RegionPtr& outRegion, BrushFloat& outDistance ) const;
	ESurroundType QuerySurroundType( int nRegionIndex ) const;
	void QueryIntersectionByRegion( CBrushRegion::RegionPtr pRegion, RegionList& outIntersetionRegions ) const;
	void QueryIntersectionByEdge( const BrushEdge3D& edge, std::vector<IntersectionPair>& outIntersections ) const;
	void QueryIntersectionRegionsWith2DRect( IDisplayViewport* pView, const BrushMatrix34& worldTM, CBrushRegion::RegionPtr pRectRegion, bool bExcludeBackFace, RegionList& outIntersectionRegions ) const;
	void QueryIntersectionEdgesWith2DRect( IDisplayViewport* pView, const BrushMatrix34& worldTM, CBrushRegion::RegionPtr pRectRegion, bool bExcludeBackFace, BUtil::EdgeQueryResult& outIntersectionEdges ) const;
	void QueryOpenRegions( const BrushVec3& raySrc, const BrushVec3& rayDir, std::vector<CBrushRegion::RegionPtr>& outRegions ) const;
	void QueryNeighbourRegionsByEdge( const BrushEdge3D& edge, RegionList& neighbourRegions ) const;

	void Serialize( XmlNodeRef &xmlNode, bool bLoading, bool bUndo );
	void Save( CArchive& ar );
	void Load( CArchive& ar );
	void Replace( int nIndex, CBrushRegion::RegionPtr pRegion );
	void Optimize();
	void SeparateRegions( const BrushPlane& plane );
	bool EraseEdge( const BrushEdge3D& edge );
	bool HasIntersection( CBrushRegion::RegionPtr pRegion, bool bStrongCheck = false ) const;
	bool HasTouched( CBrushRegion::RegionPtr pRegion ) const;
	void RecordUndo( const char *sUndoDescription, CBaseObject* pObject ) const;
	bool RemoveRegion( int nRegionIndex );
	bool RemoveRegion( CBrushRegion::RegionPtr pRegion );
	void RemoveRegionsWithSpecificFlagsPlane( int nFlags, const BrushPlane* pPlane = NULL );
	bool IsVertexOnEdge( const BrushPlane& plane, const BrushVec3& vertex, CBrushRegion::RegionPtr pExcludedRegion = NULL ) const;
	void Move( const BrushVec3& offset );
	void MoveShelf( BUtil::ShelfID sourceShelfID, BUtil::ShelfID destShelfID );
	void Transform( const BrushMatrix34& tm );	
	void GetRegionList( RegionList& outExportedRegionList ) const;
	void GetRegions( const BrushPlane& plane, std::vector<int>& outClonedRegions ) const;
	CBrushRegion::RegionPtr GetRegion( int nRegionIndex ) const;
	int GetRegionIndex( CBrushRegion::RegionPtr pRegion ) const;
	void SetShelf( BUtil::ShelfID shelfID ) const{
		assert( shelfID >= 0 && shelfID < BUtil::kMaxShelfCount );
		m_ShelfID = shelfID;
	}
	BUtil::ShelfID GetShelf() const { return m_ShelfID; }
	int GetRegionSize() const { return m_Regions[m_ShelfID].size(); }
	void SetModeFlag( int nFlag ) { m_nModeFlag = nFlag; }
	bool CheckModeFlag( int nFlags ) { return m_nModeFlag & nFlags ? true : false; }
	int GetModeFlag() const { return m_nModeFlag; }
	void AddExcludedEdgeInDrawing( const BrushEdge3D& edge ) { m_ExcludedEdgesInDrawing.push_back(edge); }
	void ClearExcludedEdgesInDrawing() { m_ExcludedEdgesInDrawing.clear(); }

	void SetSubdivisionLevel( unsigned char nLevel ) { m_SubdivisionLevel = nLevel; }
	unsigned char GetSubdivisionLevel() const { return m_SubdivisionLevel; }

	void SetTessFactor( unsigned char nTessFactor ) { m_nTessFactor = nTessFactor; }
	unsigned char GetTessFactor() const { return m_nTessFactor; }

	CBrushDesignerDB* GetDB() const{ return m_pDB; }
	void ResetDB( int nFlag, int nValidShelfID = -1 );

	CBrushDesignerSmoothingGroupManager* GetSmoothingGroupMgr() const { return m_pSmoothingGroupMgr; }
	CBrushDesignerEdgeSharpnessManager* GetEdgeSharpnessMgr() const { return m_pEdgeSharpnessMgr; }

	void SetSubdivisionResult( CBrushDesignerHalfEdgeMesh* pSubdividedHalfMesh );
	CBrushDesignerHalfEdgeMesh* GetSubdivisionResult() const { return m_pSubdividionResult; }

	enum EClipRegionResult
	{
		eCRR_CLIPFAILED,
		eCRR_SUCCESSED,
		eCRR_CLIPSUCCESSEDBUTFILLFAILED
	};
	EClipRegionResult Clip( const BrushPlane& clipPlane, _smart_ptr<CBrushDesigner>& pOutFrontPart, _smart_ptr<CBrushDesigner>& pOutBackPart, bool bFillFacet ) const;
	void SetMirrorPlane( const BrushPlane& mirrorPlane ){ m_MirrorPlane = mirrorPlane; }
	const BrushPlane& GetMirrorPlane() const { return m_MirrorPlane; }
	void ResetFromList( const std::vector<CBrushRegion::RegionPtr>& regionList );

	bool IsEmpty( int nShelf = -1 ) const;
	bool HasClosedRegion( int nShelf = -1 ) const;

	void Union( CBrushDesigner* BDesigner );
	void Subtract( CBrushDesigner* BDesigner );
	void Intersect( CBrushDesigner* BDesigner );
	void ClipOutside( CBrushDesigner* BDesigner );
	void ClipInside( CBrushDesigner* BDesigner );
	bool IsInside( const BrushVec3& vPos ) const;	

	AABB GetBoundBox( int nShelf = -1 );
	void InvalidateAABB( int nShelf = -1 ) const
	{	
		if( nShelf == 0 || nShelf == 1 )
			m_BoundBox[nShelf].bValid = false;
		if( nShelf == -1 )
			m_BoundBox[0].bValid = m_BoundBox[1].bValid = false;
	}

private:

	struct SInterectedRegion
	{
		CBrushRegion::RegionPtr m_pRegion;
		int m_nEdgeIndex;
		Vec2d m_IntersectionPt;
	};

	struct SControlPointInfo
	{
		SControlPointInfo( const Vec2d& point, bool bSkip )
		{
			m_Point = point;
			m_bSkip = bSkip;
		}
		Vec2d m_Point;
		bool m_bSkip;
	};

	typedef std::vector<SInterectedRegion> IntersectedRegionList;
	typedef std::map< CBrushRegion*,IntersectedRegionList > IntersectedRegionMap;

private:	

	void Init();

	bool QueryNearestEdge( int nRegionIndex, const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outPos, BrushVec3& outPosOnEdge, BrushPlane& outPlane, BrushEdge3D& outEdge ) const;
	bool QueryNearestEdge( int nRegionIndex, const BrushVec3& position, BrushVec3& outPosOnEdge, BrushPlane& outPlane, BrushEdge3D& outEdge ) const;
	bool UnionRegion( CBrushRegion::RegionPtr pRegion );
	bool SubtractRegionAB( CBrushRegion::RegionPtr pRegion );
	bool SubtractRegionBA( CBrushRegion::RegionPtr pRegion );
	bool IntersectRegion(CBrushRegion::RegionPtr pRegion );
	bool ExclusiveORRegion( CBrushRegion::RegionPtr pRegion );
	bool SplitRegion(CBrushRegion::RegionPtr pRegion );
	bool SplitRegionsByOpenRegion( CBrushRegion::RegionPtr pOpenRegion );
	bool GetRegionPlane( int nRegionIndex, BrushPlane& outPlane ) const;
	void DeleteRegions( std::set<CBrushRegion::RegionPtr>& deletedRegions );
	RegionList::iterator RemoveRegion( const RegionList::iterator& iRegion );	
	CBrushRegion::RegionPtr GetRegionPtr( int nIndex ) const;
	void DisplayRegions( DisplayContext& dc, const int nLineThickness = 2, const ColorB& lineColor = ColorB(0,0,2) );
	bool GetVisibleEdge( const BrushEdge3D& edge, const BrushPlane& plane, std::vector<BrushEdge3D>& outVisibleEdges ) const;
	void AddRegion( CBrushRegion::RegionPtr pRegion );
	void AddRegion( int nShelf, CBrushRegion::RegionPtr pRegion );
	void Clip( const CBrushBSPTree3D* pTree, RegionList& regionList, BUtil::EClipType cliptype, RegionList& outRegions, BUtil::EClipObjective clipObjective );
	std::vector<CBrushRegion::RegionPtr> GetIntersectedParts( CBrushRegion::RegionPtr pRegion ) const;
	CBrushRegion::RegionPtr QueryEquivalentRegion( int nShelfID, CBrushRegion::RegionPtr pRegion, int* pOutRegionIndex = NULL ) const;
	static bool GenerateRegionsFromEdgeList( std::vector<BrushEdge3D>& edgeList, const BrushPlane& plane, RegionList& outRegions );

private:

	struct SDesignerAABB
	{
		SDesignerAABB() : bValid(false)
		{
			aabb.Reset();
		}
		AABB aabb;
		bool bValid;
	};

	mutable BUtil::ShelfID m_ShelfID;
	std::vector<BrushEdge3D> m_ExcludedEdgesInDrawing;
	RegionList m_Regions[BUtil::kMaxShelfCount];	
	mutable SDesignerAABB m_BoundBox[BUtil::kMaxShelfCount];
	BrushPlane m_MirrorPlane;
	unsigned char m_SubdivisionLevel;
	unsigned char m_nTessFactor;
	int m_nModeFlag;

	CBrushDesignerDB* m_pDB;
	CBrushDesignerSmoothingGroupManager* m_pSmoothingGroupMgr;
	CBrushDesignerEdgeSharpnessManager* m_pEdgeSharpnessMgr;
	CBrushDesignerHalfEdgeMesh* m_pSubdividionResult;
};

class CShelfIDReconstructor
{
public: 

	CShelfIDReconstructor( CBrushDesigner* pDesigner ) : m_pDesigner(pDesigner)
	{
		if( m_pDesigner )
			m_ShelfID = m_pDesigner->GetShelf();
	}

	CShelfIDReconstructor( CBrushDesigner* pDesigner, BUtil::ShelfID shelfID ) : m_pDesigner(pDesigner), m_ShelfID(shelfID)
	{
	}

	~CShelfIDReconstructor()
	{
		if( m_pDesigner )
			m_pDesigner->SetShelf(m_ShelfID);
	}

private:
	_smart_ptr<CBrushDesigner> m_pDesigner;
	BUtil::ShelfID m_ShelfID;
};

#define DESIGNER_SHELF_RECONSTRUCTOR_POSTFIX(pDesigner,nPostFix) CShelfIDReconstructor shelfIDReconstructor##nPostFix(pDesigner);
#define DESIGNER_SHELF_RECONSTRUCTOR(pDesigner) DESIGNER_SHELF_RECONSTRUCTOR_POSTFIX(pDesigner,0);
