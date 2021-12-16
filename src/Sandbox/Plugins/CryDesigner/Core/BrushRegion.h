#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushRegion.h
//  Created:     8/26/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////
class CBrushBSPTree2D;
class CBrushConvexes;
class CBrushTriangles;

class CBrushRegion : public CRefCountBase
{
public:

	typedef _smart_ptr<CBrushRegion> RegionPtr;

	enum ERegionFlag
	{
		eRF_Mirrored = BIT(1),
		eRF_Hidden   = BIT(4),
		eRF_All      = 0xFFFFFFFF
	};

	enum EIncludeCoEdgeInIntersecting
	{
		eICEII_IncludeCoSame = BIT(0),
		eICEII_IncludeCoDiff = BIT(1),
		eICEII_IncludeBothBoundary = eICEII_IncludeCoSame | eICEII_IncludeCoDiff
	};

	enum ESeparateRegions
	{
		eSR_OuterHull = 0x01,
		eSR_InnerHull = 0x02,
		eSR_Together = 0x04,
	};

	enum EResultExtract
	{
		eRE_Fail,
		eRE_EndAtEndVtx,
		eRE_EndAtStartVtx
	};

	struct SIntersectedEdge
	{
		SIntersectedEdge(){}
		SIntersectedEdge( int nEdgeIndex, const BrushVec2& pt ) :
		m_nEdgeIndex(nEdgeIndex),
			m_IntersectedPt(pt)
		{
		}
		int m_nEdgeIndex;
		BrushVec2 m_IntersectedPt;
	};

	static const BrushFloat fEpsilonForHull;

public:
	
	CBrushRegion();
	CBrushRegion( const CBrushRegion& region );
	CBrushRegion( const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edgeList );
	CBrushRegion( const std::vector<BrushVec3>& vertices );
	CBrushRegion( const std::vector<BrushVec2>& points, const std::vector<BUtil::SEdge>& edgeList );
	CBrushRegion( const std::vector<BrushVec2>& points );
	CBrushRegion( const std::vector<BrushVec2>& points, const std::vector<BUtil::SEdge>& edgeList, const BrushPlane& plane, int matID, const BUtil::STexInfo* pTexInfo );
	CBrushRegion( const std::vector<BrushVec2>& points, const BrushPlane& plane, int matID, const BUtil::STexInfo* pTexInfo, bool bClosed );
	CBrushRegion( const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edgeList, const BrushPlane& plane, int matID, const BUtil::STexInfo* pTexInfo, bool bOptimizeRegion = true );
	CBrushRegion( const std::vector<BrushVec3>& vertices, const BrushPlane& plane, int matID, const BUtil::STexInfo* pTexInfo, bool bClose );
	~CBrushRegion();

	CBrushRegion& operator = (const CBrushRegion& region);

	void Init()
	{
		NullBspTree();
		NullConvexes();
		NullTriangules();
		m_MaterialID = 0;
		m_bRepresentativePosValid = false;
		m_Flag = 0;
		m_PrivateFlag = eRPF_Invalid;
		m_Plane.Set(BrushVec3(0,0,0),0);
		m_Vertices.clear();
		m_Edges.clear();
		CoCreateGuid(&m_GUID);
	}
	RegionPtr Clone() const;
	void Clear();
	void Display( DisplayContext &dc ) const;
	bool IsPassed( const BrushVec3& raySrc, const BrushVec3& rayDir, BrushFloat& outT ) const;
	bool IncludeAllEdges( RegionPtr pRegion ) const;
	bool Include( RegionPtr pRegion ) const;
	bool Include( const BrushVec3& vertex ) const;
	bool IntersectedBetweenAABBs( const AABB& aabb ) const;
	bool IsOpen( const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edges ) const;
	bool IsOpen() const {  return CheckPrivateFlags(eRPF_Open); }
	bool IsIdentical( RegionPtr pRegion ) const;
	bool IsEdgeOnCrust( const BrushEdge3D& edge, int* pOutEdgeIndex = NULL, BrushEdge3D* pOutIntersectedEdge = NULL ) const;
	bool IsEquivalent( const RegionPtr& pRegion ) const;
	bool SubtractEdge( const BrushEdge3D& edge, std::vector<BrushEdge3D>& outSubtractedEdges ) const;
	bool HasOverlappedEdges( CBrushRegion::RegionPtr pRegion ) const;
	bool HasEdge( const BrushEdge3D& edge, bool bApplyDir = false, int* pOutEdgeIndex = NULL ) const;
	bool QueryIntersections( const BrushEdge3D& edge, std::map<BrushFloat,BrushVec3>& outIntersections ) const;
	bool QueryIntersections( const BrushPlane& plane, const BrushLine3D& crossLine3D, std::vector<BrushEdge3D>& outSortedIntersections ) const;
	bool QueryNearestEdge( const BrushVec3& vertex, BrushEdge3D& outNearestEdge, BrushVec3& outNearestPos ) const;
	bool QueryNearestPosFromBoundary( const BrushVec3& vertex, BrushVec3& outNearestPos ) const;
	bool QueryEdgesHavingVertex( const BrushVec3& vertex, std::vector<int>& outEdgeIndices ) const;
	bool QueryAxisAlignedLines( std::vector<BrushLine>& outLines );
	void QueryIntersectionEdgesWith2DRect( IDisplayViewport* pView, const BrushMatrix34& worldTM, CBrushRegion::RegionPtr pRectRegion, bool bExcludeBackFace, BUtil::EdgeQueryResult& outIntersectionEdges );
	void Serialize( XmlNodeRef &xmlNode, bool bLoading, bool bUndo );
	void SaveBinary( CArchive& ar );
	void SaveBinary( std::vector<char>& buffer );
	void LoadBinary( CArchive& ar );
	void LoadBinary( std::vector<char>& buffer );
	bool AddOpenRegion( RegionPtr BRegion );
	bool Union( RegionPtr BRegion );
	bool Subtract( RegionPtr BRegion );
	bool ExclusiveOR( RegionPtr BRegion );
	bool Intersect( RegionPtr BRegion, uint8 includeCoEdgeFlags = eICEII_IncludeBothBoundary );
	bool ClipInside( RegionPtr BRegion );
	bool ClipOutside( RegionPtr BRegion );
	CBrushRegion::RegionPtr RemoveInside();
	void MakeThisConvex();
	RegionPtr Flip();
	void ReverseEdges();	
	bool Attach( RegionPtr pRegion );
	BrushFloat GetNearestDistance( RegionPtr pRegion, const BrushVec3& direction ) const;
	void ClipByEdge( int nEdgeIndex, std::vector<RegionPtr>& outSplittedRegions ) const;
	const AABB& GetBoundBox() const;
	BrushFloat GetRadius() const;
	bool GetSeparatedRegions( std::vector<RegionPtr>& outSeparatedRegions, int nSprateRegionFlag = eSR_Together, bool bOptimizeRegion = true ) const;
	void ModifyOrientation();
	bool IsValid() const {	return !m_Vertices.empty() && !m_Edges.empty();	}
	const BrushPlane& GetPlane() const { return m_Plane; }
	void SetPlane( const BrushPlane& plane ){ m_Plane = plane; }
	bool UpdatePlane( const BrushPlane& plane ){
		return UpdatePlane( plane, -plane.Normal() );
	}
	bool UpdatePlane( const BrushPlane& plane, const BrushVec3& directionForHitTest );
	void SetMaterialID( int nMatID ) { m_MaterialID = nMatID; }
	int GetMaterialID() const { return m_MaterialID; }
	const BUtil::STexInfo& GetTexInfo() const { return m_TexInfo; }
	void SetTexInfo( const BUtil::STexInfo& texInfo )
	{ 
		Invalidate(); 
		m_TexInfo = texInfo;
	}
	bool IsConvex() const { return CheckPrivateFlags(eRPF_Convex); }
	int GetEdgeSize() const{ return m_Edges.size(); }
	BrushEdge3D GetEdge(int nEdgeIndex) const;
	BrushEdge GetEdge2D(int nEdgeIndex) const;
	bool GetEdgesByVertexIndex( int nVertexIndex, std::vector<int>& outEdgeIndices ) const;
	int GetEdgeIndex( int nVertexIndex0, int nVertexIndex1 ) const;
	int GetEdgeIndex( const BrushEdge3D& edge3D ) const;
	bool GetEdge( int nVertexIndex0, int nVertexIndex1, BUtil::SEdge& outEdge ) const;
	const BUtil::SEdge& GetEdgeIndexPair( int nEdgeIndex ) const { return m_Edges[nEdgeIndex]; }
	bool GetAdjacentEdgesByEdgeIndex( int nEdgeIndex, int* pOutPrevEdgeIndex, int* pOutNextEdgeIndex ) const;
	bool GetAdjacentEdgesByVertexIndex( int nVertexIndex, int* pOutPrevEdgeIndex, int* pOutNextEdgeIndex ) const;
	BrushVec3 GetCenterPosition() const;
	BrushVec3 GetAveragePosition() const;
	BrushVec3 GetRepresentativePosition() const;
	bool Scale( const BrushFloat& kScale, bool bCheckBoundary = false, std::vector<BrushEdge3D>* pOutEdgesBeforeOptimization = NULL );
	bool BroadenVertex( const BrushFloat& kScale, int nVertexIndex, const BrushEdge3D* pBaseEdge = NULL );
	bool GetMaximumScale( BrushFloat& fOutShortestScale ) const;
	bool IsPlaneEquivalent( RegionPtr pRegion ) const;
	bool CheckFlags( int nFlags ) const	
	{	
		if( m_Flag == 0 && nFlags == eRF_All )
			return true;
		return m_Flag & nFlags ? true : false;
	}
	int AddFlags( int nFlags ){return m_Flag |= nFlags;}
	void SetFlag( int nFlag ){m_Flag = nFlag;}
	int RemoveFlags( int nFlags ){ return m_Flag &= ~nFlags;}
	int GetFlag()const {return m_Flag;}
	bool GetVertexIndex( const BrushVec3& vertex, int& nOutIndex ) const;
	bool GetNearestVertexIndex( const BrushVec3& vertex, int& nOutIndex ) const;
	const BrushVec3& GetVertex( int nIndex ) const { return m_Vertices[nIndex]; }
	int GetVertexListSize() const{return m_Vertices.size();}
	void SetVertex( int nIndex, const BrushVec3& vertex );
	bool AddVertex( const BrushVec3& vertex, int* pOutNewVertexIndex = NULL, BUtil::EdgeIndexSet* pOutNewEdgeIndices = NULL );
	bool SwapEdge( int nEdgeIndex0, int nEdgeIndex1 );
	int AddEdge( const BrushEdge3D& edge );	
	bool GetEdgeIndex( int nVertexIndex0, int nVertexIndex1, int& nOutEdgeIndex ) const{
		return GetEdgeIndex( m_Edges, nVertexIndex0, nVertexIndex1, nOutEdgeIndex );
	}
	static bool GetEdgeIndex( const std::vector<BUtil::SEdge>& edgeList, int nVertexIndex0, int nVertexIndex1, int& nOutEdgeIndex );	
	bool GetNextVertex( int nVertexIndex, BrushVec3& outVertex ) const;
	bool GetPrevVertex( int nVertexIndex, BrushVec3& outVertex ) const;
	bool GetLinkedVertices( std::vector<BrushVec3>& outVertexList ) const{
		return GetLinkedVertices( outVertexList, m_Vertices, m_Edges );
	}
	void Optimize();
	bool Exist( const BrushVec3& vertex, const BrushFloat& kEpsilon, int* pOutIndex = NULL ) const;
	bool Exist( const BrushEdge3D& edge3D, bool bAllowReverse, int* pOutIndex = NULL ) const;
	bool IsEndPoint( const BrushVec3& position, bool* bOutFirst = NULL ) const;
	void Move( const BrushVec3& offset );
	void Transform( const Matrix34& tm );	
	bool Concatenate( RegionPtr pRegion );
	// These functions are only valid in the case that the region is not close.
	// If the region is not close, the methods will return false.
	bool GetFirstVertex( BrushVec3& outVertex ) const;
	bool GetLastVertex( BrushVec3& outVertex ) const;
	// Rearrange vertices and edges so that they can have the sequential orders
	void Rearrange();
	CBrushBSPTree2D* GetBSPTree() const{
		if( !m_pBSPTree )
			BuildBSP();
		return m_pBSPTree;
	}
	bool IsCCW() const;
	bool HasHoles() const { return CheckPrivateFlags(eRPF_HasHoles); }
	bool IsVertexOnCrust( const BrushVec3& vertex ) const;
	bool HasVertex( const BrushVec3& vertex, int* pOutVertexIndex = NULL ) const;
	bool IsBridgeEdgeRelation( const BUtil::SEdge& pEdge0, const BUtil::SEdge& pEdge1 ) const;
	bool HasBridgeEdges() const;
	void RemoveBridgeEdges();
	void GetBridgeEdges( std::vector<BrushEdge3D>& outBridgeEdges ) const;
	void RemoveEdge( const BrushEdge3D& edge );
	void RemoveEdge( int nEdgeIndex );
	void GetUnconnectedRegions( std::vector<RegionPtr>& outRegions );
	EResultExtract ExtractVertexList( int nStartEdgeIdx, int nEndEdgeIdx, std::vector<BrushVec3>& outVertexList, std::vector<int>* pOutVertexIndices = NULL ) const;
	bool InRectangle( IDisplayViewport* pView, const BrushMatrix34& worldTM, CBrushRegion::RegionPtr pRectRegion, bool bExcludeBackFace ) const;

	static BUtil::EIntersectionType HasIntersection( RegionPtr pRegion0, RegionPtr pRegion1 );
	static void MakeRegionsFromEdgeList( const std::vector<BrushEdge3D>& edgeList, const BrushPlane& plane, std::vector<RegionPtr>& outRegions );

	REFGUID GetGUID() const{ return m_GUID; }
	bool ClipByPlane( const BrushPlane& clipPlane, std::vector<RegionPtr>& pOutFrontRegions, std::vector<RegionPtr>& pOutBackRegions, std::vector<BrushEdge3D>* pOutBoundaryEdges = NULL ) const;
	RegionPtr Mirror( const BrushPlane& mirrorPlane );
	bool GetComputedPlane( BrushPlane& outPlane ) const;

	int ChoosePrevEdge( const BUtil::SEdge& edge, const BUtil::EdgeIndexSet& candidateSecondIndices ) const{
		BUtil::SEdge outEdge;
		ChoosePrevEdge( m_Vertices, edge, candidateSecondIndices, outEdge );
		return GetEdgeIndex(outEdge.m_i[0],outEdge.m_i[1]);
	}
	int ChooseNextEdge( const BUtil::SEdge& edge, const BUtil::EdgeIndexSet& candidateSecondIndices ) const{
		BUtil::SEdge outEdge;
		ChooseNextEdge( m_Vertices, edge, candidateSecondIndices, outEdge );
		return GetEdgeIndex(outEdge.m_i[0],outEdge.m_i[1]);
	}

	void OutputDebugData() const;
	static void OutputTestCode( RegionPtr pRegion0, RegionPtr pRegion1, const char* command, const BrushFloat& kEpsilon );
	void OutputTestCode() const;

	CBrushConvexes* GetConvexes();
	CBrushTriangles* GetTriangles( bool bGenerateBackFaces );

	static CBrushRegion::RegionPtr MakeRegionFromRectangle( const CRect& rectangle );
	static void CopyRegions( std::vector<CBrushRegion::RegionPtr>& sourceRegions, std::vector<CBrushRegion::RegionPtr>& destRegions );

private:

	struct SVertexEx
	{
		SVertexEx( const BrushVec3& v ) : m_v(v)
		{
			m_id = 0;
		}

		SVertexEx( const BrushVec3& v, unsigned char id ) : m_v(v), m_id(id)
		{
		}

		SVertexEx& operator = ( const SVertexEx& rv )
		{
			m_v = rv.m_v;
			m_id = rv.m_id;
			return *this;
		}

		bool operator == ( const SVertexEx& rv ) const
		{
			return m_id == rv.m_id && m_v.IsEquivalent(rv.m_v,kDesignerEpsilon);
		}

		BrushVec3 m_v;
		unsigned char m_id;
	};

	enum ESearchDirection
	{
		eSD_Previous, 
		eSD_Next,
	};
	
	enum ERegionPrivateFlag
	{
		eRPF_Open	   = BIT(0),
		eRPF_Convex    = BIT(1),
		eRPF_HasHoles  = BIT(2),
		eRPF_Invalid   = BIT(31)
	};

	void OutputDebugData( const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edges ) const;
	void OutputDebugData( const std::vector<SVertexEx>& vertices, const std::vector<BUtil::SEdge>& edges ) const;

private:

	void Reset( const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edgeList );
	void Reset( const std::vector<BrushVec3>& vertices );
	bool QueryEdgesContainingVertex( const BrushVec3& vertex, std::vector<int>& outEdgeIndices ) const;
	bool BuildBSP() const;
	bool OptimizeEdges( std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges ) const;
	void OptimizeVertices( std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges ) const;
	void Transform2Vertices( const std::vector<BrushVec2>& points, const BrushPlane& plane );
	BUtil::EIntersectionType HasIntersection( RegionPtr pRegion );
	int AddVertex( std::vector<SVertexEx>& vertices, const SVertexEx& newVertex ) const;
	int AddVertex( std::vector<BrushVec3>& vertices, const BrushVec3& newVertex ) const;
	void Clip( const CBrushBSPTree2D* pTree, BUtil::EClipType cliptype, std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges, BUtil::EClipObjective clipObjective, int nVertexID ) const;
	bool Optimize( std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges );
	bool Optimize( std::vector<BrushVec3>& vertices, std::vector<BUtil::SEdge>& edges );
	bool Optimize( const std::vector<BrushEdge3D>& edges );
	bool GetAdjacentPrevEdgeIndicesWithVertexIndex( int vertexIndex, std::vector<int>& outEdgeIndices, const std::vector<BUtil::SEdge>& edges ) const;
	bool GetAdjacentNextEdgeIndicesWithVertexIndex( int vertexIndex, std::vector<int>& outEdgeIndices, const std::vector<BUtil::SEdge>& edges ) const;
	bool GetAdjacentEdgeIndexWithEdgeIndex( int edgeIndex, int& outPrevIndex, int& outNextIndex, const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edges ) const;
	BrushLine GetLineFromEdge( int edgeIndex, const std::vector<SVertexEx>& vertices, const std::vector<BUtil::SEdge>& edges ) const{
		return BrushLine( m_Plane.W2P(vertices[edges[edgeIndex].m_i[0]].m_v), m_Plane.W2P(vertices[edges[edgeIndex].m_i[1]].m_v) );
	}
	bool GetLinkedVertices( std::vector<BrushVec3>& outVertexList, const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edges ) const;
	void FindLoops( std::vector<BUtil::EdgeList>& outLoopList ) const;
	void SearchLinkedColinearEdges( int edgeIndex, ESearchDirection direction, const std::vector<SVertexEx>& vertices, const std::vector<BUtil::SEdge>& edges, std::vector<int>& outLinkedEdges ) const;	
	void SearchLinkedEdges( int edgeIndex, ESearchDirection direction, const std::vector<SVertexEx>& vertices, const std::vector<BUtil::SEdge>& edges, std::vector<int>& outLinkedEdges ) const;	
	static bool DoesIdenticalEdgeExist( const std::vector<BUtil::SEdge>& edges, const BUtil::SEdge& e, int* pOutIndex = NULL );
	static bool DoesReverseEdgeExist( const std::vector<BUtil::SEdge>& edges, const BUtil::SEdge& e, int* pOutIndex = NULL );
	bool CreateNewRegionFromEdges( const std::vector<BUtil::SEdge>& inputEdges, RegionPtr& outRegion, bool bOptimizeRegion = true ) const;
	void Load( XmlNodeRef &xmlNode, bool bUndo );
	void Save( XmlNodeRef &xmlNode, bool bUndo );
	static void CopyEdges( const std::vector<BUtil::SEdge>& sourceEdges, std::vector<BUtil::SEdge>& destincationEdges );
	static void AddEdges( const std::vector<BrushVec2>& positivePoints, const std::vector<BrushVec2>& negativePoints, std::vector<BrushVec2>& outPoints, std::vector<BUtil::SEdge>& outEdges );
	void InitializeEdgesAndUpdate( bool bClosed );
	//! Remove colinear edges which have a same normal vector.
	bool FlattenEdges( std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges ) const;
	//! Remove edges whose two indices are same or which exist already.
	static void RemoveEdgesHavingSameIndices( std::vector<BUtil::SEdge>& edges );
	//! enough short edges should be regarded as a vertex
	void RemoveEdgesRegardedAsVertex( std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges ) const;
	static BrushEdge ToEdge2D( const BrushPlane& plane, const BrushEdge3D& edge3D ){
		return BrushEdge( plane.W2P(edge3D.m_v[0]), plane.W2P(edge3D.m_v[1]) );
	}
	bool ShouldOrderReverse( RegionPtr BRegion ) const;
	void Invalidate() const
	{
		NullBspTree();
		NullConvexes();
		NullTriangules();
		m_BoundInfo.bValid = false;
		m_bRepresentativePosValid = false;
		m_PrivateFlag = eRPF_Invalid;
	}
	BrushFloat Cosine( int i0, int i1, int i2, const std::vector<BrushVec3>& vertices ) const;
	bool IsCCW( int i0, int i1, int i2, const std::vector<BrushVec3>& vertices ) const;
	bool IsCW( int i0, int i1, int i2, const std::vector<BrushVec3>& vertices ) const{ return !IsCCW(i0,i1,i2,vertices); }
	void ChoosePrevEdge( const std::vector<BrushVec3>& vertices, const BUtil::SEdge& edge, const BUtil::EdgeIndexSet& candidateSecondIndices, BUtil::SEdge& outEdge ) const;
	void ChooseNextEdge( const std::vector<BrushVec3>& vertices, const BUtil::SEdge& edge, const BUtil::EdgeIndexSet& candidateSecondIndices, BUtil::SEdge& outEdge ) const;
	void Convert2PureVertices( const std::vector<SVertexEx>& inputVertices, std::vector<BrushVec3>& outVertices ) const;
	bool FindFirstEdgeIndex( const std::set<int>& edgeSet, int& outEdgeIndex ) const;
	bool ExtractEdge3DList( std::vector<BrushEdge3D>& outList ) const;
	bool SortVerticesAlongCrossLine( std::vector<BrushVec3>& vList, const BrushLine3D& crossLine, std::vector<BrushEdge3D>& outGapEdges ) const;
	bool SortEdgesAlongCrossLine( std::vector<BrushEdge3D>& edgeList, const BrushLine3D& crossLine, std::vector<BrushEdge3D>& outGapEdges ) const;
	bool QueryEdges( const BrushVec3& vertex, int vIndexInEdge, std::set<int>* pOutEdgeIndices = NULL ) const;
	void GetBridgeEdgeSet( std::set<BUtil::SEdge>& outBridgeEdgeSet, bool bOutputBothDirection ) const;
	void UpdateBoundBox() const;
	void ConnectNearVertices( std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges ) const;
	void RemoveUnconnectedEdges( std::vector<BUtil::SEdge>& edges ) const;
	void UpdatePrivateFlags() const;

private:

	void AddVertex_Basic( const BrushVec3& vertex ){
		Invalidate();
		m_Vertices.push_back(vertex);
	}
	void SetVertex_Basic( int nIndex, const BrushVec3& vertex ){
		Invalidate();
		m_Vertices[nIndex] = vertex;
	}
	void DeleteAllVertices_Basic(){
		Invalidate();
		m_Vertices.clear();
	}
	void SetVertexList_Basic( const std::vector<BrushVec3>& vertexList ){
		Invalidate();
		int iVertexListCount(vertexList.size());
		m_Vertices.clear();
		m_Vertices.reserve(iVertexListCount);
		for( int i = 0; i < iVertexListCount; ++i )
			m_Vertices.push_back(vertexList[i]);
	}
	void AddEdge_Basic( const BUtil::SEdge& edge ){
		Invalidate();
		m_Edges.push_back(edge);
	}
	void DeleteAllEdges_Basic(){
		Invalidate();
		m_Edges.clear();
	}
	void SetEdgeList_Basic( const std::vector<BUtil::SEdge>& edgeList ){
		Invalidate();
		CopyEdges( edgeList, m_Edges );
	}
	void SwapEdgeIndex_Basic( int nIndex ){
		Invalidate();
		std::swap( m_Edges[nIndex].m_i[0], m_Edges[nIndex].m_i[1] );
	}

	bool CheckPrivateFlags( int nFlags ) const	
	{	
		DESIGNER_ASSERT(nFlags!=eRPF_Invalid);
		if( nFlags == eRPF_Invalid )
			return false;
		if( m_PrivateFlag == eRPF_Invalid )
			UpdatePrivateFlags();
		return m_PrivateFlag & nFlags ? true : false;
	}
	int AddPrivateFlags( int nFlags ) const   {return m_PrivateFlag |= nFlags;}
	int RemovePrivateFlags( int nFlags )const { return m_PrivateFlag &= ~nFlags;}

	void NullBspTree() const;
	void NullConvexes() const;
	void NullTriangules() const;

private:

	GUID m_GUID;
	std::vector<BrushVec3> m_Vertices;
	std::vector<BUtil::SEdge> m_Edges;
	BrushPlane m_Plane;
	int m_MaterialID;
	BUtil::STexInfo m_TexInfo;
	unsigned int m_Flag;

private:

	struct SRegionBound
	{
		SRegionBound() : bValid(false)
		{
		}
		AABB aabb;
		BrushFloat raidus;
		bool bValid;
	};
	mutable CBrushBSPTree2D* m_pBSPTree;
	mutable CBrushConvexes* m_pConvexes;
	mutable CBrushTriangles* m_pTriangles;
	mutable BrushVec3 m_RepresentativePos;
	mutable bool m_bRepresentativePosValid;
	mutable unsigned int m_PrivateFlag;
	mutable SRegionBound m_BoundInfo;
};
