#include "StdAfx.h"
#include "BrushRegion.h"
#include "Util/GeometryUtil.h"
#include "IDisplayViewport.h"
#include "IBaseToolPanel.h"
#include "Base64.h"
#include "BrushConvexes.h"
#include "BrushBSPTree2D.h"
#include "BrushTriangles.h"
#include "BrushDesignerPolygonDecomposer.h"

CBrushRegion::CBrushRegion() : 
	m_pBSPTree(NULL), 
	m_pConvexes(NULL),
	m_pTriangles(NULL)
{
	Init();
}

CBrushRegion::CBrushRegion( const CBrushRegion& region ) :
CRefCountBase(),
m_pBSPTree(NULL),
m_pConvexes(NULL),
m_pTriangles(NULL)
{	
	operator =( region );
}

CBrushRegion::CBrushRegion( const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edgeList ) :
	CRefCountBase(),
	m_pBSPTree(NULL),
	m_pConvexes(NULL),
	m_pTriangles(NULL),
	m_MaterialID(0),
	m_Flag(0),
	m_PrivateFlag(eRPF_Invalid),
	m_Plane(BrushVec3(0,0,0),0),
	m_bRepresentativePosValid(false)
{
	Reset( vertices, edgeList );
	CoCreateGuid(&m_GUID);
}

CBrushRegion::CBrushRegion( const std::vector<BrushVec3>& vertices ) :
CRefCountBase(),
	m_pBSPTree(NULL),
	m_pConvexes(NULL),
	m_pTriangles(NULL),
	m_MaterialID(0),
	m_Flag(0),
	m_PrivateFlag(eRPF_Invalid),
	m_Plane(BrushVec3(0,0,0),0),
	m_bRepresentativePosValid(false)
{
	Reset( vertices );
	CoCreateGuid(&m_GUID);
}

CBrushRegion::CBrushRegion( const std::vector<BrushVec2>& points, const std::vector<BUtil::SEdge>& edgeList ) :
	m_pBSPTree(NULL),
	m_pConvexes(NULL),
	m_pTriangles(NULL),
	m_MaterialID(0),
	m_Flag(0),
	m_PrivateFlag(eRPF_Invalid),
	m_Plane(BrushVec3(0,0,0),0),
	m_bRepresentativePosValid(false)
{
	int iPointSize(points.size());
	std::vector<BrushVec3> vertices;
	vertices.reserve(iPointSize);
	for( int i = 0; i < iPointSize; ++i )
		vertices.push_back(BrushVec3(points[i].x,points[i].y,0));
	Reset( vertices, edgeList );
	CoCreateGuid(&m_GUID);
}

CBrushRegion::CBrushRegion( const std::vector<BrushVec2>& points ) : 
	m_pBSPTree(NULL),
	m_pConvexes(NULL),
	m_pTriangles(NULL),
	m_MaterialID(0),
	m_Flag(0),
	m_PrivateFlag(eRPF_Invalid),
	m_Plane(BrushVec3(0,0,0),0),
	m_bRepresentativePosValid(false)
{
	int iPointSize(points.size());
	std::vector<BrushVec3> vertices;
	vertices.reserve(iPointSize);
	for( int i = 0; i < iPointSize; ++i )
		vertices.push_back(BrushVec3(points[i].x,points[i].y,0));
	Reset(vertices);
	CoCreateGuid(&m_GUID);
}

CBrushRegion::CBrushRegion( const std::vector<BrushVec2>& points, const std::vector<BUtil::SEdge>& edgeList, const BrushPlane& plane, int matID, const BUtil::STexInfo* pTexInfo ) :
CRefCountBase(),
	m_pBSPTree(NULL),
	m_pConvexes(NULL),
	m_pTriangles(NULL),
	m_MaterialID(matID),
	m_Plane(plane),
	m_Flag(0),
	m_PrivateFlag(eRPF_Invalid),
	m_bRepresentativePosValid(false)
{
	Transform2Vertices(points, plane);
	SetEdgeList_Basic(edgeList);
	if( pTexInfo )
		SetTexInfo(*pTexInfo);
	CoCreateGuid(&m_GUID);
	Optimize();
}

CBrushRegion::CBrushRegion( const std::vector<BrushVec3>& vertices, const BrushPlane& plane, int matID, const BUtil::STexInfo* pTexInfo, bool bClosed ) :
CRefCountBase(),
m_Plane(plane),
m_pBSPTree(NULL),
m_pConvexes(NULL),
m_pTriangles(NULL),
m_MaterialID(matID),
m_Flag(0),
m_PrivateFlag(eRPF_Invalid),
m_bRepresentativePosValid(false)
{
	SetVertexList_Basic(vertices);
	InitializeEdgesAndUpdate(bClosed);
	if( pTexInfo )
		SetTexInfo(*pTexInfo);
	CoCreateGuid(&m_GUID);
	Optimize();
}

CBrushRegion::CBrushRegion( const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edgeList, const BrushPlane& plane, int matID, const BUtil::STexInfo* pTexInfo, bool bOptimizeRegion ) :
CRefCountBase(),
	m_Plane(plane),
	m_pBSPTree(NULL),
	m_pConvexes(NULL),
	m_pTriangles(NULL),
	m_MaterialID(matID),
	m_Flag(0),
	m_PrivateFlag(eRPF_Invalid),
	m_bRepresentativePosValid(false)
{
	SetVertexList_Basic(vertices);
	SetEdgeList_Basic(edgeList);
	if( pTexInfo )
		SetTexInfo(*pTexInfo);
	CoCreateGuid(&m_GUID);
	if( bOptimizeRegion )
		Optimize();
}

CBrushRegion::CBrushRegion( const std::vector<BrushVec2>& points, const BrushPlane& plane, int matID, const BUtil::STexInfo* pTexInfo, bool bClosed ) : 
CRefCountBase(),
	m_Plane(plane),
	m_pBSPTree(NULL),
	m_pConvexes(NULL),
	m_pTriangles(NULL),
	m_MaterialID(matID),
	m_Flag(0),
	m_PrivateFlag(eRPF_Invalid),
	m_bRepresentativePosValid(false)
{
	Transform2Vertices(points, plane);
	InitializeEdgesAndUpdate(bClosed);
	if( pTexInfo )
		SetTexInfo(*pTexInfo);
	CoCreateGuid(&m_GUID);
	Optimize();
}

void CBrushRegion::Reset( const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edgeList )
{
	SetVertexList_Basic(vertices);
	SetEdgeList_Basic(edgeList);

	std::vector<BrushVec3> vList;
	GetLinkedVertices(vList);
	BUtil::ComputePlane(vList,m_Plane);

	UpdateBoundBox();	
	Optimize();
}

void CBrushRegion::Reset( const std::vector<BrushVec3>& vertices )
{
	std::vector<BUtil::SEdge> edgeList;
	for( int i = 0, iVertexCount(vertices.size()); i < iVertexCount; ++i )
		edgeList.push_back(BUtil::SEdge(i,(i+1)%iVertexCount));

	Reset( vertices, edgeList );
}

CBrushRegion::~CBrushRegion()
{
	NullBspTree();
	NullConvexes();
	NullTriangules();
}

void CBrushRegion::InitializeEdgesAndUpdate( bool bClosed )
{
	DeleteAllEdges_Basic();
	m_Edges.reserve(m_Vertices.size());
	for( int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i )
	{
		if( !bClosed )
		{
			if( i == iVertexSize-1 )
				break;
		}
		AddEdge_Basic(BUtil::SEdge(i, (i+1)%iVertexSize));
	}	
	UpdateBoundBox();
}

CBrushRegion& CBrushRegion::operator = (const CBrushRegion& region)
{
	SetVertexList_Basic(region.m_Vertices);	
	m_Plane = region.m_Plane;
	m_MaterialID = region.m_MaterialID;
	m_BoundInfo = region.m_BoundInfo;
	m_BoundInfo.bValid = true;
	m_Flag = region.m_Flag;	
	m_TexInfo = region.m_TexInfo;
	CoCreateGuid(&m_GUID);
	SetEdgeList_Basic(region.m_Edges);
	m_PrivateFlag = region.m_PrivateFlag;
	return *this;
}

CBrushRegion::RegionPtr CBrushRegion::Clone() const
{
	return new CBrushRegion(*this);
}

void CBrushRegion::Clear()
{
	DeleteAllVertices_Basic();
	DeleteAllEdges_Basic();
}

void CBrushRegion::CopyEdges( const std::vector<BUtil::SEdge>& sourceEdges, std::vector<BUtil::SEdge>& destincationEdges )
{
	destincationEdges.clear();
	destincationEdges.reserve(sourceEdges.size());
	for( int i = 0, iEdgeSize(sourceEdges.size()); i < iEdgeSize; ++i )
		destincationEdges.push_back(sourceEdges[i]);
}

void CBrushRegion::Display( DisplayContext &dc ) const
{
	if( !IsValid() )
		return;	

	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
		dc.DrawLine( GetVertex(m_Edges[i].m_i[0]), GetVertex(m_Edges[i].m_i[1]) );
}

bool CBrushRegion::IsPassed( const BrushVec3& raySrc, const BrushVec3& rayDir, BrushFloat& outT ) const
{
	BrushVec3 vIntersection;

	if( GetPlane().HitTest( raySrc, raySrc+rayDir, kDesignerEpsilon, &outT, &vIntersection ) )
	{
		if( outT > 0 && GetBSPTree() )
		{
			BUtil::EPointPosEnum location(GetBSPTree()->IsVertexIn(vIntersection));
			return location == BUtil::ePP_INSIDE || location == BUtil::ePP_BORDER;
		}
	}

	return false;
}

bool CBrushRegion::IsIdentical( RegionPtr pRegion ) const
{
	if( pRegion == NULL )
		return false;

	if( m_Edges.size() != pRegion->m_Edges.size() )
		return false;

	if( !IsPlaneEquivalent(pRegion) )
		return false;

	bool bTestOnceMoreTime = false;

	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		BrushEdge3D edge1(GetVertex(m_Edges[i].m_i[0]),GetVertex(m_Edges[i].m_i[1]));
		BrushEdge3D edge1Inverse(GetVertex(m_Edges[i].m_i[1]),GetVertex(m_Edges[i].m_i[0]));
		bool bSameExist(false);
		for( int k = 0, kEdgeSize(pRegion->m_Edges.size()); k < kEdgeSize; ++k )
		{
			BrushEdge3D edge2( pRegion->GetVertex(pRegion->m_Edges[k].m_i[0]), pRegion->GetVertex(pRegion->m_Edges[k].m_i[1]) );
			if( edge1.IsEquivalent(edge2,kDesignerEpsilon) || edge1Inverse.IsEquivalent(edge2,kDesignerEpsilon) )
			{
				bSameExist = true;
				break;
			}
		}
		if( !bSameExist )
		{
			bTestOnceMoreTime = true;
			break;
		}
	}

	if( !bTestOnceMoreTime )
		return true;

	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		BrushEdge edge1(m_Plane.W2P(GetVertex(m_Edges[i].m_i[0])),m_Plane.W2P(GetVertex(m_Edges[i].m_i[1])));
		BrushEdge edge1Inverse(edge1.m_v[1],edge1.m_v[0]);
		bool bSameExist(false);
		for( int k = 0, kEdgeSize(pRegion->m_Edges.size()); k < kEdgeSize; ++k )
		{
			BrushEdge edge2(m_Plane.W2P(pRegion->GetVertex(pRegion->m_Edges[k].m_i[0])),m_Plane.W2P(pRegion->GetVertex(pRegion->m_Edges[k].m_i[1])));
			if( edge1.IsEquivalent(edge2,kDesignerEpsilon) || edge1Inverse.IsEquivalent(edge2,kDesignerEpsilon) )
			{
				bSameExist = true;
				break;
			}
		}
		if( bSameExist == false )
			return false;
	}

	return true;
}

BUtil::EIntersectionType CBrushRegion::HasIntersection( RegionPtr pRegion )
{
	if( !IsValid() || !pRegion )
		return BUtil::eIT_None;

	if( !GetBSPTree() )
		return BUtil::eIT_None;

	if( !IsPlaneEquivalent(pRegion) )
		return BUtil::eIT_None;

	if( IsIdentical(pRegion) )
		return BUtil::eIT_Intersection;

	if( IsOpen(m_Vertices,m_Edges) )
		return BUtil::eIT_None;

	bool bHaveTouched = false;

	for( int i = 0, iSize(pRegion->m_Vertices.size()); i < iSize; ++i )
	{
		BUtil::EPointPosEnum checkVertexIn = GetBSPTree()->IsVertexIn(pRegion->GetVertex(i));
		if( checkVertexIn == BUtil::ePP_INSIDE )
			return BUtil::eIT_Intersection;
		else if( checkVertexIn == BUtil::ePP_BORDER )
			bHaveTouched = true;
	}

	for( int i = 0, iSize(pRegion->GetEdgeSize()); i < iSize; ++i )
	{
		BrushEdge3D edge = pRegion->GetEdge(i);
		BUtil::EIntersectionType intersectionType(GetBSPTree()->HasIntersection(edge));
		if( intersectionType == BUtil::eIT_Intersection )
			return intersectionType;
		else if( intersectionType == BUtil::eIT_JustTouch )
			bHaveTouched = true; 
	}

	if( bHaveTouched )
	{
		std::vector< std::pair<int,int> > vertexIndices;
		for( int i = 0, iVertexCount(m_Vertices.size()); i < iVertexCount; ++i )
		{
			int nVertexIndex = -1;
			if( !pRegion->GetVertexIndex(m_Vertices[i],nVertexIndex) )
				continue;
			vertexIndices.push_back(std::pair<int,int>(i,nVertexIndex));
		}
		if( vertexIndices.size() == 1 )
		{	
			std::vector<int> edgeIndices[2];
			GetEdgesByVertexIndex(vertexIndices[0].first,edgeIndices[0]);
			pRegion->GetEdgesByVertexIndex(vertexIndices[0].second,edgeIndices[1]);		

			for( int i = 0, iEdgeIndexCount0(edgeIndices[0].size()); i < iEdgeIndexCount0; ++i )
			{
				BrushEdge3D e0 = GetEdge(edgeIndices[0][i]);
				for( int k = 0, iEdgeIndexCount1(edgeIndices[1].size()); k < iEdgeIndexCount1; ++k )
				{
					BrushEdge3D e1 = pRegion->GetEdge(edgeIndices[1][k]);
					if( e0.ContainVertex(e1.m_v[0],kDesignerEpsilon) && e0.ContainVertex(e1.m_v[1],kDesignerEpsilon) )
						return BUtil::eIT_JustTouch;
				}
			}
			return BUtil::eIT_None;
		}
	}

	return bHaveTouched ? BUtil::eIT_JustTouch : BUtil::eIT_None;
}

bool CBrushRegion::IsEdgeOnCrust( const BrushEdge3D& edge, int* pOutEdgeIndex, BrushEdge3D* pOutIntersectedEdge ) const
{
	BrushLine3D edgeline(edge.m_v[0],edge.m_v[1]);
	BrushLine3D invEdgeline(edge.m_v[1],edge.m_v[0]);

	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		BrushEdge3D edgeOnRegion = GetEdge(i);

		if( !edgeOnRegion.IsEquivalent(edge,kDesignerEpsilon) && !edgeOnRegion.GetInverted().IsEquivalent(edge,kDesignerEpsilon) )
		{
			BrushLine3D edgeLineOnRegion(edgeOnRegion.m_v[0],edgeOnRegion.m_v[1]);
			if( !edgeline.m_Dir.IsEquivalent(edgeLineOnRegion.m_Dir,kDesignerEpsilon) && !invEdgeline.m_Dir.IsEquivalent(edgeLineOnRegion.m_Dir,kDesignerEpsilon) )
				continue;

			BrushVec3 vProjectedPos;
			if( !edgeOnRegion.GetProjectedPos(edge.m_v[0],vProjectedPos) )
				continue;

			if( (vProjectedPos-edge.m_v[0]).GetLength() >= kDesignerEpsilon )
				continue;
		}

		BrushEdge3D intersectionEdge;
		EOperationResult intersectionResult(BUtil::IntersectEdge3D(edge,edgeOnRegion,intersectionEdge,kDesignerEpsilon));
		if( intersectionResult == eOR_One )
		{
			BrushFloat fLength = (intersectionEdge.m_v[1]-intersectionEdge.m_v[0]).GetLength();
			if( fLength < kDesignerEpsilon )
				continue;
			if( pOutEdgeIndex )
				*pOutEdgeIndex = i;
			if( pOutIntersectedEdge )
				*pOutIntersectedEdge = intersectionEdge;
			return true;
		}
	}

	return false;
}

bool CBrushRegion::IsEquivalent( const RegionPtr& pRegion ) const
{
	if( this == pRegion )
		return true;

	if( GetEdgeSize() != pRegion->GetEdgeSize() || GetVertexListSize() != pRegion->GetVertexListSize() )
		return false;

	if( GetFlag() != pRegion->GetFlag() )
		return false;

	if( !GetPlane().IsEquivalent(pRegion->GetPlane(),kDesignerEpsilon) )
		return false;

	for( int i = 0, iEdgeSize(GetEdgeSize()); i < iEdgeSize; ++i )
	{
		if( !pRegion->HasEdge(GetEdge(i),true) )
			return false;
	}

	return true;
}

bool CBrushRegion::SubtractEdge( const BrushEdge3D& edge, std::vector<BrushEdge3D>& outSubtractedEdges ) const
{
	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		BrushEdge3D edgeOnRegion = GetEdge(i);
		if( !ToEdge2D(m_Plane,edgeOnRegion).IsIdenticalLine(ToEdge2D(m_Plane,edge),kDesignerEpsilon) )
			continue;
		BrushEdge3D subtractedEdges[2];
		EOperationResult opResult = BUtil::SubtractEdge3D(edge,edgeOnRegion,subtractedEdges,kDesignerEpsilon);
		if( opResult != eOR_Invalid )
		{
			if( opResult == eOR_One || opResult == eOR_Two )
			{
				outSubtractedEdges.push_back(subtractedEdges[0]);
				if( opResult == eOR_Two )
					outSubtractedEdges.push_back(subtractedEdges[1]);
			}
			return true;
		}
	}
	return false;
}

bool CBrushRegion::HasEdge( const BrushEdge3D& edge, bool bApplyDir, int* pOutEdgeIndex ) const
{
	BrushEdge3D invertedEdge(edge.GetInverted());

	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		BrushEdge3D edgeOnRegion = GetEdge(i);
		if( bApplyDir )
		{
			if( edge.IsEquivalent(edgeOnRegion,kDesignerEpsilon) )
			{
				if( pOutEdgeIndex )
					*pOutEdgeIndex = i;
				return true;
			}
		}
		else
		{
			if( edge.IsEquivalent(edgeOnRegion,kDesignerEpsilon) || invertedEdge.IsEquivalent(edgeOnRegion,kDesignerEpsilon) )
			{
				if( pOutEdgeIndex )
					*pOutEdgeIndex = i;
				return true;
			}
		}
	}
	return false;
}

bool CBrushRegion::HasOverlappedEdges( CBrushRegion::RegionPtr pRegion ) const
{
	for( int i = 0; i < GetEdgeSize(); ++i )
	{
		BrushEdge3D edge = GetEdge(i);
		if( pRegion->IsEdgeOnCrust(edge) )
			return true;
	}
	return false;
}

BUtil::EIntersectionType CBrushRegion::HasIntersection( RegionPtr pRegion0, RegionPtr pRegion1 )
{
	if( !pRegion0 || !pRegion1 )
		return BUtil::eIT_None;

	BUtil::EIntersectionType intersetionType0(pRegion0->HasIntersection(pRegion1));
	BUtil::EIntersectionType intersetionType1(pRegion1->HasIntersection(pRegion0));

	if( intersetionType0 == BUtil::eIT_Intersection || intersetionType1 == BUtil::eIT_Intersection )
		return BUtil::eIT_Intersection;

	if( intersetionType0 == BUtil::eIT_JustTouch || intersetionType1 == BUtil::eIT_JustTouch )
		return BUtil::eIT_JustTouch;

	return BUtil::eIT_None;
}

bool CBrushRegion::IncludeAllEdges( RegionPtr pRegion ) const
{
	if( !IsValid() || !pRegion || !GetBSPTree() || !IsPlaneEquivalent(pRegion) )
		return false;

	if( IsIdentical(pRegion) )
		return true;

	for( int i = 0, iEdgeCount(pRegion->GetEdgeSize()); i < iEdgeCount; ++i )
	{
		BrushEdge3D edge = pRegion->GetEdge(i);
		if( !GetBSPTree()->IsInside(edge,false) )
			return false;
	}

	return true;
}

bool CBrushRegion::Include( RegionPtr pRegion ) const
{
	if( !IsValid() || !pRegion || !GetBSPTree() || !IsPlaneEquivalent(pRegion) )
		return false;

	if( IsIdentical(pRegion) )
		return true;

	if( !IncludeAllEdges(pRegion) )
		return false;

	for( int i = 0, iEdgeCount(GetEdgeSize()); i < iEdgeCount; ++i )
	{
		BrushEdge3D edge = GetEdge(i);
		if( pRegion->GetBSPTree()->IsInside(edge,false) )
			return false;
	}

	return true;
}

bool CBrushRegion::IntersectedBetweenAABBs( const AABB& aabb ) const
{
	return aabb.IsIntersectBox(GetBoundBox());
}

bool CBrushRegion::Include( const BrushVec3& vertex ) const
{
	if( !IsValid() || !GetBSPTree() )
		return false;

	return GetBSPTree()->IsVertexIn(vertex) != BUtil::ePP_OUTSIDE;
}

bool CBrushRegion::IsOpen( const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edges ) const
{
	for( int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i )
	{
		const BUtil::SEdge& edge(edges[i]);
		int prevEdge = -1;
		int nextEdge = -1;
		GetAdjacentEdgeIndexWithEdgeIndex( i, prevEdge, nextEdge, vertices, edges );
		if( prevEdge == -1 || nextEdge == -1 )
			return true;
	}
	return false;
}

bool CBrushRegion::QueryIntersections( const BrushEdge3D& edge, std::map<BrushFloat,BrushVec3>& outIntersections ) const
{
	if( std::abs(m_Plane.Distance(edge.m_v[0])) >= kDesignerEpsilon || std::abs(m_Plane.Distance(edge.m_v[1])) >= kDesignerEpsilon )
		return false;

	BrushEdge edge2D(m_Plane.W2P(edge.m_v[0]), m_Plane.W2P(edge.m_v[1]));

	for( int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i )
	{
		BrushEdge regionEdge2D(m_Plane.W2P(m_Vertices[m_Edges[i].m_i[0]]), m_Plane.W2P(m_Vertices[m_Edges[i].m_i[1]]));

		BrushVec2 vIntersection;
		if( !edge2D.GetIntersect(regionEdge2D, kDesignerEpsilon, vIntersection) )
			continue;

		BrushFloat fLength = (edge2D.m_v[0]-vIntersection).GetLength();
		outIntersections[fLength] = m_Plane.P2W(vIntersection);
	}

	return outIntersections.empty() ? false : true;
}

bool CBrushRegion::QueryIntersections( const BrushPlane& plane, const BrushLine3D& crossLine3D, std::vector<BrushEdge3D>& outSortedIntersections ) const
{
	std::vector<BrushVec3> intersections;

	for( int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i )
	{
		const BrushVec3& v0 = m_Vertices[m_Edges[i].m_i[0]];
		const BrushVec3& v1 = m_Vertices[m_Edges[i].m_i[1]];

		BrushFloat d0 = plane.Distance(v0);
		BrushFloat d1 = plane.Distance(v1);

		if( d0 * d1 < 0 )
		{
			BrushVec3 intersection;
			if( plane.HitTest(v0, v1, kDesignerEpsilon, NULL, &intersection) )
				intersections.push_back(intersection);
		}
	}

	return SortVerticesAlongCrossLine( intersections, crossLine3D, outSortedIntersections );
}

bool CBrushRegion::QueryNearestEdge( const BrushVec3& vertex, BrushEdge3D& outNearestEdge, BrushVec3& outNearestPos ) const
{
	BrushFloat fLastDistance = 3e10f;
	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		const BUtil::SEdge& edgeIndex(m_Edges[i]);
		BrushEdge3D edge( GetVertex(edgeIndex.m_i[0]), GetVertex(edgeIndex.m_i[1]) );

		bool bInEdge;
		BrushVec3 outPos;
		if( !edge.GetNearestVertex(vertex,outPos,bInEdge) )
			continue;

		BrushFloat fDistance((vertex-outPos).GetLength());
		if( fDistance < fLastDistance )
		{
			outNearestPos = outPos;
			fLastDistance = fDistance;
			outNearestEdge = edge;
		}
	}

	return fLastDistance < 3e10f;
}

bool CBrushRegion::QueryNearestPosFromBoundary( const BrushVec3& vertex, BrushVec3& outNearestPos ) const
{
	if( !GetBSPTree() )
		return false;

	if( GetBSPTree()->IsVertexIn(vertex) != BUtil::ePP_OUTSIDE )
	{
		outNearestPos = vertex;
		return true;
	}

	BrushFloat fNearestDistance(BUtil::kEnoughBigNumber);
	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		BrushEdge3D edge = GetEdge(i);
		BrushFloat fDistance;
		EResultDistance resultDistance(BrushEdge3D::GetSquaredDistance(edge,vertex,fDistance));
		if( fDistance < fNearestDistance )
		{
			fNearestDistance = fDistance;

			if( resultDistance == eResultDistance_EdgeP0 )
			{
				outNearestPos = edge.m_v[0];
			}
			else if( resultDistance == eResultDistance_EdgeP1 )
			{
				outNearestPos = edge.m_v[1];
			}
			else if( resultDistance == eResultDistance_Middle )
			{
				BrushVec3 edgeVector(edge.m_v[1]-edge.m_v[0]);
				outNearestPos = edge.m_v[0]+(edgeVector.Dot(vertex-edge.m_v[0])/edgeVector.Dot(edgeVector))*edgeVector;
			}
		}
	}

	return fNearestDistance < BUtil::kEnoughBigNumber;
}

bool CBrushRegion::QueryEdgesContainingVertex( const BrushVec3& vertex, std::vector<int>& outEdgeIndices ) const
{
	bool bAdded = false;
	BrushVec2 ptPos = m_Plane.W2P(vertex);
	for( int i = 0, iEdgeListCount(m_Edges.size()); i < iEdgeListCount; ++i )
	{
		BrushEdge edge(m_Plane.W2P(m_Vertices[m_Edges[i].m_i[0]]), m_Plane.W2P(m_Vertices[m_Edges[i].m_i[1]]));
		if( edge.IsInside(ptPos,kDesignerEpsilon) )
		{
			outEdgeIndices.push_back(i);
			bAdded = true;
		}
	}

	return bAdded;
}

bool CBrushRegion::QueryEdgesHavingVertex( const BrushVec3& vertex, std::vector<int>& outEdgeIndices ) const
{
	bool bAdded = false;

	for( int i = 0, iEdgeListCount(m_Edges.size()); i < iEdgeListCount; ++i )
	{
		BrushEdge3D e = GetEdge(i);
		if( e.m_v[0].IsEquivalent(vertex,kDesignerEpsilon) )
		{
			outEdgeIndices.push_back(i);
			bAdded = true;
		}
	}

	for( int i = 0, iEdgeListCount(m_Edges.size()); i < iEdgeListCount; ++i )
	{
		BrushEdge3D e = GetEdge(i);
		if( e.m_v[1].IsEquivalent(vertex,kDesignerEpsilon) )
		{
			outEdgeIndices.push_back(i);
			bAdded = true;
		}
	}

	return bAdded;
}

bool CBrushRegion::QueryEdges( const BrushVec3& vertex, int vIndexInEdge, std::set<int>* pOutEdgeIndices ) const
{
	DESIGNER_ASSERT( vIndexInEdge == 0 || vIndexInEdge == 1 );

	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		if( m_Vertices[m_Edges[i].m_i[vIndexInEdge]].IsEquivalent(vertex,kDesignerEpsilon) )
		{
			if( pOutEdgeIndices )
				pOutEdgeIndices->insert(i);
			else
				return true;
		}
	}

	return !pOutEdgeIndices || pOutEdgeIndices->empty() ? false : true;
}

bool CBrushRegion::QueryAxisAlignedLines( std::vector<BrushLine>& outLines )
{
	BrushVec2 vAxises[] = { BrushVec2(1,0), BrushVec2(-1,0), BrushVec2(0,1), BrushVec2(0,-1) };
	bool bAdded = false;

	for( int i = 0, iEdgeSize(GetEdgeSize()); i < iEdgeSize; ++i )
	{
		BrushEdge3D edge = GetEdge(i);
		BrushVec2 v0_2D = GetPlane().W2P(edge.m_v[0]);
		BrushVec2 v1_2D = GetPlane().W2P(edge.m_v[1]);

		BrushVec2 vDir = (v1_2D-v0_2D).GetNormalized();

		for( int k = 0; k < sizeof(vAxises)/sizeof(*vAxises); ++k )
		{
			if( vDir.Dot(vAxises[k]) >= 1-kDesignerEpsilon )
			{
				outLines.push_back(BrushLine(v0_2D,v1_2D));
				bAdded = true;
				break;
			}
		}
	}

	return bAdded;
}

bool CBrushRegion::ShouldOrderReverse( RegionPtr BRegion ) const
{
	if( !BRegion )
		return false;

	if( !BRegion->IsOpen() )
		return false;

	bool bReverseOrderingEdge = false;
	for( int i = 0, iEdgeSize(BRegion->GetEdgeSize()); i < iEdgeSize; ++i )
	{
		BrushEdge3D edge = BRegion->GetEdge(i);
		for( int a = 0; a < 2; ++a )
		{
			int nExistedIndex;
			if( Exist(edge.m_v[a],kDesignerEpsilon,&nExistedIndex) )
			{
				for( int k = 0, nAEdgeSize(m_Edges.size()); k < nAEdgeSize; ++k )
				{
					if( m_Edges[k].m_i[a] == nExistedIndex )
					{
						bReverseOrderingEdge = true;
						break;
					}
					if( bReverseOrderingEdge )
						break;
				}
			}
			if( bReverseOrderingEdge )
				break;
		}
	}

	return bReverseOrderingEdge;
}

bool CBrushRegion::AddOpenRegion( RegionPtr BRegion )
{
	if( !BRegion || !BRegion->IsOpen() )
		return false;

	std::vector<BrushVec3> linkedVertices;
	if( !BRegion->GetLinkedVertices(linkedVertices) )
		return false;

	BrushVec3 eitherVertices[2] = { linkedVertices[0], linkedVertices[linkedVertices.size()-1] };
	std::vector<int> edgeIndices[2];
	int newVertexIndices[2] = { -1, -1 };

	for( int k = 0; k < 2; ++k )
	{
		if( !QueryEdgesContainingVertex( eitherVertices[k], edgeIndices[k] ) )
			return false;

		int nPrevVertexListCount = m_Vertices.size();
		newVertexIndices[k] = AddVertex( m_Vertices, eitherVertices[k] );
		if( nPrevVertexListCount == m_Vertices.size() )
			continue;

		for( int i = 0, iCount(edgeIndices[k].size()); i < iCount; ++i )
		{
			BUtil::SEdge newEdge(m_Edges[edgeIndices[k][i]].m_i[0],newVertexIndices[k]);
			m_Edges[edgeIndices[k][i]].m_i[0] = newVertexIndices[k];
			m_Edges.push_back(newEdge);
		}
	}

	int nLinkedVertexCount(linkedVertices.size());
	for( int i = 0; i < nLinkedVertexCount-1; ++i )
	{
		int nVertexIndex = AddVertex(m_Vertices,linkedVertices[i]);
		int nNextVertexIndex = AddVertex(m_Vertices,linkedVertices[i+1]);
		m_Edges.push_back(BUtil::SEdge(nVertexIndex,nNextVertexIndex));
		m_Edges.push_back(BUtil::SEdge(nNextVertexIndex,nVertexIndex));
	}

	return true;
}

void CBrushRegion::ConnectNearVertices( std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges ) const
{
	std::vector<int> edgeListFirstDisconnect;
	for( int i = 0, iEdgeCount(edges.size()); i < iEdgeCount; ++i )
	{
		bool bFound = false;
		for( int k = 0; k < iEdgeCount; ++k )
		{
			int nIndex = (i+1+k)%iEdgeCount;
			if( edges[i].m_i[0] == edges[nIndex].m_i[1] )
			{
				bFound = true;
				break;
			}
		}
		if( bFound == false )
			edgeListFirstDisconnect.push_back(i);
	}
	std::vector<int> edgeListSecondDisconnect;
	for( int i = 0, iEdgeCount(edges.size()); i < iEdgeCount; ++i )
	{
		bool bFound = false;
		for( int k = 0; k < iEdgeCount; ++k )
		{
			int nIndex = (i+1+k)%iEdgeCount;
			if( edges[i].m_i[1] == edges[nIndex].m_i[0] )
			{
				bFound = true;
				break;
			}
		}
		if( bFound == false )
			edgeListSecondDisconnect.push_back(i);
	}

	if( !edgeListFirstDisconnect.empty() && !edgeListSecondDisconnect.empty() )
	{
		std::set<int> usedIndices;
		for( int i = 0, iFirstCount(edgeListFirstDisconnect.size()); i < iFirstCount; ++i )
		{
			if( usedIndices.find(edgeListFirstDisconnect[i]) != usedIndices.end() )
				continue;
			BrushFloat theLeastDistance = (BrushFloat)3e10;
			int theLeastIndex = -1;
			for( int k = 0, iSecondCount(edgeListSecondDisconnect.size()); k < iSecondCount; ++k )
			{
				if( usedIndices.find(edgeListSecondDisconnect[k]) != usedIndices.end() )
					continue;
				BrushFloat distance = vertices[edges[edgeListFirstDisconnect[i]].m_i[0]].m_v.GetDistance(vertices[edges[edgeListSecondDisconnect[k]].m_i[1]].m_v);
				if( distance < theLeastDistance )
				{
					theLeastDistance = distance;
					theLeastIndex = k;
				}
			}
			if( theLeastIndex != -1 )
			{
				edges[edgeListFirstDisconnect[i]].m_i[0] = edges[edgeListSecondDisconnect[theLeastIndex]].m_i[1];
				usedIndices.insert(edgeListFirstDisconnect[i]);
				usedIndices.insert(edgeListSecondDisconnect[theLeastIndex]);
			}
		}
	}
}

void CBrushRegion::RemoveUnconnectedEdges( std::vector<BUtil::SEdge>& edges ) const
{
	bool bKeepProcessing = true;
	while(bKeepProcessing)
	{
		std::set<BUtil::SEdge> removedEdges;
		for( int i = 0, iEdgeCount(edges.size()); i < iEdgeCount; ++i )
		{
			int nCountWithConnectionFromBeginningToEnd = 0;
			int nCountWithConnectionFromEndToBeginning = 0;
			for( int k = 0; k < iEdgeCount; ++k )
			{
				if( i == k || edges[i].m_i[0] == edges[k].m_i[1] && edges[i].m_i[1] == edges[k].m_i[0] )
					continue;
				if( edges[i].m_i[0] == edges[k].m_i[1] )
					++nCountWithConnectionFromBeginningToEnd;
				if( edges[i].m_i[1] == edges[k].m_i[0] )
					++nCountWithConnectionFromEndToBeginning;
			}
			if( nCountWithConnectionFromBeginningToEnd == 0 || nCountWithConnectionFromEndToBeginning == 0 )
				removedEdges.insert(edges[i]);
		}
		std::vector<BUtil::SEdge>::iterator iEdge = edges.begin();
		for( ; iEdge != edges.end(); )
		{
			if( removedEdges.find(*iEdge) != removedEdges.end() )
				iEdge  = edges.erase(iEdge);
			else
				++iEdge;
		}
		bKeepProcessing = removedEdges.empty() ? false : true;
	}
}

bool CBrushRegion::Union( RegionPtr BRegion )
{
	if( !BRegion )
		return false;

	if( IsOpen() && BRegion->IsOpen() )
	{
		bool bReverseOrderingEdge = ShouldOrderReverse(BRegion);
		for( int i = 0, iEdgeSize(BRegion->GetEdgeSize()); i < iEdgeSize; ++i )
		{
			BrushEdge3D edge = BRegion->GetEdge(i);
			if( bReverseOrderingEdge )
				edge.Invert();
			AddEdge(edge);
		}

		Optimize();

		return true;
	}
	else if( IsOpen() || BRegion->IsOpen() )
	{
		return false;
	}

	if( !IsValid() )
	{
		SetVertexList_Basic(BRegion->m_Vertices);
		SetEdgeList_Basic(BRegion->m_Edges);
		m_Plane = BRegion->GetPlane();
		UpdateBoundBox();
	}
	else
	{
		if( !IsPlaneEquivalent(BRegion) )
			return false;

		if( !GetBSPTree() || !BRegion->GetBSPTree() )
			return false;

		std::vector<SVertexEx> vertices;
		std::vector<BUtil::SEdge> edges;

		Clip( BRegion->GetBSPTree(), BUtil::eCT_Negative, vertices, edges, BUtil::eCO_Union0, 0 );
		BRegion->Clip( GetBSPTree(), BUtil::eCT_Negative, vertices, edges, BUtil::eCO_Union1, 0 );

#ifdef ENABLE_OUTPUT_DEBUGINFO
		OutputDebugData(vertices,edges);
		std::vector<SVertexEx> backupVertices(vertices);
		std::vector<BUtil::SEdge> backupEdges(edges);
#endif

		ConnectNearVertices( vertices, edges );
		RemoveUnconnectedEdges( edges );

#ifdef ENABLE_OUTPUT_DEBUGINFO
		if( vertices.empty() || edges.empty() )
		{
			DESIGNER_ASSERT( !vertices.empty() && !edges.empty() );
			vertices = backupVertices;
			edges = backupEdges;
		}
#endif

		if( Optimize(vertices,edges) == false )
		{
#ifdef ENABLE_OUTPUT_DEBUGINFO
			OutputDebugData();
			BRegion->OutputDebugData();
			OutputTestCode(this,BRegion,"Union",kDesignerEpsilon);
			DESIGNER_ASSERT(0);
#endif
			return false;
		}
	}
	return true;
}

bool CBrushRegion::Intersect( RegionPtr BRegion, uint8 includeCoEdgeFlags )
{ 
	if( !IsValid() || !BRegion )
		return false;

	if( IsOpen() || BRegion->IsOpen() )
		return false;

	if( !GetBSPTree() || !BRegion->GetBSPTree() )
		return false;

	if( !IsPlaneEquivalent(BRegion) )
		return false;

	std::vector<SVertexEx> vertices;
	std::vector<BUtil::SEdge> edges;

	if( includeCoEdgeFlags & eICEII_IncludeCoSame )
		Clip( BRegion->GetBSPTree(), BUtil::eCT_Positive, vertices, edges, BUtil::eCO_Intersection0IncludingCoSame, 0);
	else
		Clip( BRegion->GetBSPTree(), BUtil::eCT_Positive, vertices, edges, BUtil::eCO_Intersection0, 0);

	if( includeCoEdgeFlags & eICEII_IncludeCoDiff )
		BRegion->Clip( GetBSPTree(), BUtil::eCT_Positive, vertices, edges, BUtil::eCO_Intersection1IncludingCoDiff, 0 );
	else
		BRegion->Clip( GetBSPTree(), BUtil::eCT_Positive, vertices, edges, BUtil::eCO_Intersection1, 0 );

	std::vector<BrushVec3> pureVertices;
	Convert2PureVertices(vertices,pureVertices);

	if( IsOpen(pureVertices,edges) )
	{
		SetEdgeList_Basic(edges);
		DeleteAllVertices_Basic();
		for( int i = 0, iVertexSize(vertices.size()); i < iVertexSize; ++i )
			AddVertex_Basic(vertices[i].m_v);
	}

	if( !Optimize(vertices,edges) )
	{
#ifdef ENABLE_OUTPUT_DEBUGINFO
		OutputDebugData();
		BRegion->OutputDebugData();
		OutputTestCode(this,BRegion,"Intersect",kDesignerEpsilon);
		DESIGNER_ASSERT(0);
#endif
		return false;
	}

	return true;
}

bool CBrushRegion::Subtract( RegionPtr BRegion )
{
	if( !IsValid() || !BRegion )
		return false;

	if( IsOpen() || BRegion->IsOpen() )
		return false;

	if( !GetBSPTree() || !BRegion->GetBSPTree() )
		return false;

	if( !IsPlaneEquivalent(BRegion) )
		return false;

	if( BRegion->Include(this) )
	{
		DeleteAllVertices_Basic();
		DeleteAllEdges_Basic();
		return true;
	}

	std::vector<SVertexEx> vertices;
	std::vector<BUtil::SEdge> edges;

	Clip( BRegion->GetBSPTree(), BUtil::eCT_Negative, vertices, edges, BUtil::eCO_Subtract, 0 );
	BRegion->Clip( GetBSPTree(), BUtil::eCT_Positive, vertices, edges, BUtil::eCO_Subtract, 0 );

	ConnectNearVertices( vertices, edges );
	RemoveUnconnectedEdges( edges );

	if( !Optimize(vertices,edges) )
	{
#ifdef ENABLE_OUTPUT_DEBUGINFO
		OutputDebugData();
		BRegion->OutputDebugData();
		OutputTestCode(this,BRegion,"Subtract",kDesignerEpsilon);
		DESIGNER_ASSERT(0);
#endif
		return false;
	}

	return true;
}

bool CBrushRegion::ExclusiveOR( RegionPtr BRegion )
{
	if( !IsValid() || !BRegion )
		return false;

	if( IsOpen() || BRegion->IsOpen() )
		return false;

	if( !GetBSPTree() || !BRegion->GetBSPTree() )
		return false;

	if( !IsPlaneEquivalent(BRegion) )
		return false;

	std::vector<SVertexEx> vertices;
	std::vector<BUtil::SEdge> edges;

	//A-B
	Clip( BRegion->GetBSPTree(), BUtil::eCT_Negative, vertices, edges, BUtil::eCO_Subtract, 0 );
	BRegion->Clip( GetBSPTree(), BUtil::eCT_Positive, vertices, edges, BUtil::eCO_Subtract, 0 );

	//B-A
	BRegion->Clip( GetBSPTree(), BUtil::eCT_Negative, vertices, edges, BUtil::eCO_Subtract, 1 );
	Clip( BRegion->GetBSPTree(), BUtil::eCT_Positive, vertices, edges, BUtil::eCO_Subtract, 1 );

	ConnectNearVertices( vertices, edges );
	RemoveUnconnectedEdges( edges );

	if( !Optimize(vertices,edges) )
	{
#ifdef ENABLE_OUTPUT_DEBUGINFO
		OutputDebugData();
		BRegion->OutputDebugData();
		OutputTestCode(this,BRegion,"ExclusiveOR",kDesignerEpsilon);
		DESIGNER_ASSERT(0);
#endif
		return false;
	}

	return true;
}

bool CBrushRegion::ClipInside( RegionPtr BRegion )
{
	if( !IsValid() || !BRegion )
		return false;

	if( IsOpen() || BRegion->IsOpen() )
		return false;

	if( !BRegion->GetBSPTree() )
		return false;

	if( !IsPlaneEquivalent(BRegion) )
		return false;

	std::vector<SVertexEx> vertices;
	std::vector<BUtil::SEdge> edges;
	Clip( BRegion->GetBSPTree(), BUtil::eCT_Negative, vertices, edges, BUtil::eCO_JustClip, 0 );

	return Optimize(vertices,edges);
}

bool CBrushRegion::ClipOutside( RegionPtr BRegion )
{
	if( !IsValid() || !BRegion )
		return false;

	if( IsOpen() || BRegion->IsOpen() )
		return false;

	if( !BRegion->GetBSPTree() )
		return false;

	if( !IsPlaneEquivalent(BRegion) )
		return false;

	std::vector<SVertexEx> vertices;
	std::vector<BUtil::SEdge> edges;
	Clip( BRegion->GetBSPTree(), BUtil::eCT_Positive, vertices, edges, BUtil::eCO_JustClip, 0 );

	return Optimize(vertices,edges);
}

void CBrushRegion::ReverseEdges()
{
	std::vector<BUtil::SEdge> reverseOrder;
	reverseOrder.resize(m_Edges.size());
	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		SwapEdgeIndex_Basic(i);
		reverseOrder[iEdgeSize-i-1] = m_Edges[i];
	}
	SetEdgeList_Basic(reverseOrder);
}

void CBrushRegion::UpdateBoundBox() const
{
	m_BoundInfo.aabb.Reset();

	for( int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i )
		m_BoundInfo.aabb.Add(GetVertex(i));

	m_BoundInfo.raidus = 0;
	Vec3 vCenter = m_BoundInfo.aabb.GetCenter();
	for( int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i )
	{
		float fDistance = vCenter.GetDistance(GetVertex(i));
		if( m_BoundInfo.raidus < fDistance )
			m_BoundInfo.raidus = fDistance;
	}

	m_BoundInfo.bValid = true;
}

const AABB& CBrushRegion::GetBoundBox() const
{
	if( !m_BoundInfo.bValid )
		UpdateBoundBox();
	return m_BoundInfo.aabb;
}

CBrushRegion::RegionPtr CBrushRegion::Flip()
{
	m_Plane.Invert();
	ReverseEdges();
	return this;
}

bool CBrushRegion::GetSeparatedRegions( std::vector<RegionPtr>& outSeparatedRegions, int nSprateRegionFlag, bool bOptimizeRegion ) const
{
	if( m_Edges.empty() )
		return false;

	std::vector<RegionPtr> outerRegions;
	std::vector<RegionPtr> innerRegions;
	bool bErrorHappen(false);

	std::vector<BUtil::EdgeList> outLoops;
	FindLoops(outLoops);

	for( int i = 0, nLoopCount(outLoops.size()); i < nLoopCount; ++i )
	{
		RegionPtr pRegion;
		if( CreateNewRegionFromEdges( outLoops[i], pRegion, bOptimizeRegion ) )
		{
			pRegion->SetFlag(GetFlag());
			if( pRegion->IsCCW() )
				outerRegions.push_back(pRegion);
			else
				innerRegions.push_back(pRegion);
		}
	}

	if( nSprateRegionFlag & eSR_Together )
	{
		for( int k = 0; k < outerRegions.size(); k++ )
		{
			CBrushRegion oldOuterRegion(*outerRegions[k]);
			for( int i = 0; i < innerRegions.size(); ++i )
			{
				if( oldOuterRegion.IncludeAllEdges(innerRegions[i]) )
					outerRegions[k]->Attach(innerRegions[i]);
			}
		}	
	}
	else if( nSprateRegionFlag & eSR_InnerHull )
	{
		outerRegions = innerRegions;
	}

	outSeparatedRegions = outerRegions;

	return !outSeparatedRegions.empty();
}

bool CBrushRegion::Attach( RegionPtr pRegion )
{
	if( !pRegion )
		return false;

	int nBaseIndex = m_Vertices.size();

	std::map<int,int> vertexIndexMapper;
	for( int i = 0; i < pRegion->m_Vertices.size(); ++i )
		AddVertex_Basic(pRegion->GetVertex(i));

	for( int i = 0; i < pRegion->m_Edges.size(); ++i )
		AddEdge_Basic(BUtil::SEdge(pRegion->m_Edges[i].m_i[0]+nBaseIndex,pRegion->m_Edges[i].m_i[1]+nBaseIndex));			

	return true;
}

bool CBrushRegion::Optimize( const std::vector<BrushEdge3D>& edges )
{
	std::vector<SVertexEx> vertices;
	std::vector<BUtil::SEdge> edgeIndices;
	for( int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i )
		edgeIndices.push_back(BUtil::SEdge(AddVertex(vertices,SVertexEx(edges[i].m_v[0],0)),AddVertex(vertices,SVertexEx(edges[i].m_v[1],0))));
	return Optimize( vertices, edgeIndices );
}

void CBrushRegion::Optimize()
{
	std::vector<BUtil::SEdge> replicatedEdges;
	CopyEdges(m_Edges,replicatedEdges);
	Optimize(m_Vertices,replicatedEdges);
}

bool CBrushRegion::Optimize( std::vector<BrushVec3>& vertices, std::vector<BUtil::SEdge>& edges )
{
	std::vector<SVertexEx> extendedVertices;
	int nVertexSize = vertices.size();
	extendedVertices.reserve(nVertexSize);
	for( int i = 0; i < nVertexSize; ++i )
		extendedVertices.push_back(SVertexEx(vertices[i],0));
	return Optimize(extendedVertices,edges);
}

bool CBrushRegion::Optimize( std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges )
{
	std::vector<BrushFloat> xyzList[3];
	for( int i = 0, iVertexCount(vertices.size()); i < iVertexCount; ++i )
	{
		for( int a = 0; a < 3; ++a )
		{
			bool bAdjusted = false;
			for( int k = 0, iXYZListCount(xyzList[a].size()); k < iXYZListCount; ++k )
			{
				if( vertices[i].m_v[a] != xyzList[a][k] && std::abs(vertices[i].m_v[a]-xyzList[a][k]) < kDesignerEpsilon )
				{
					vertices[i].m_v[a] = xyzList[a][k];
					bAdjusted = true;
					break;
				}
			}
			if( !bAdjusted )
				xyzList[a].push_back(vertices[i].m_v[a]);
		}
	}

	bool bSuccessOptimizeEdge = OptimizeEdges( vertices, edges );
	if( bSuccessOptimizeEdge )
	{
		OptimizeVertices( vertices, edges );
	}
	else
	{
#ifdef ENABLE_OUTPUT_DEBUGINFO
		OutputDebugData(vertices,edges);
#endif
		DESIGNER_ASSERT( bSuccessOptimizeEdge || edges.size() > 0 );
		return false;
	}

	SetEdgeList_Basic(edges);
	int nVertexSize(vertices.size());
	DeleteAllVertices_Basic();
	m_Vertices.reserve(nVertexSize);
	for( int i = 0; i < nVertexSize; ++i )
		AddVertex_Basic(vertices[i].m_v);

	if( m_Edges.empty() && m_Vertices.empty() )
		return true;

	UpdateBoundBox();

	return true;
}

void CBrushRegion::UpdatePrivateFlags() const
{
	if( m_PrivateFlag != eRPF_Invalid )
		return;

	m_PrivateFlag = 0;

	if( m_Vertices.empty() || m_Edges.empty() )
		return;	

	if( !IsOpen(m_Vertices,m_Edges) )
	{
		std::vector<RegionPtr> innerRegions;
		GetSeparatedRegions( innerRegions, eSR_InnerHull );
		bool bHasHoles = !innerRegions.empty();
		if( bHasHoles )
			AddPrivateFlags(eRPF_HasHoles);
		
		if( GetEdgeSize() == 3 )
		{
			AddPrivateFlags(eRPF_Convex);
		}
		else if( !HasHoles() )
		{
			std::vector<BrushVec3> polygon;
			if( GetLinkedVertices(polygon) )
			{
				AddPrivateFlags(eRPF_Convex);
				for( int i = 0, iPolygonSize(polygon.size()); i < iPolygonSize; ++i )
				{
					int nNextI = (i+1)%iPolygonSize;
					int nDoubleNextI = (i+2)%iPolygonSize;
					const BrushVec3& v0 = polygon[i];
					const BrushVec3& v1 = polygon[nNextI];
					const BrushVec3& v2 = polygon[nDoubleNextI];
					BrushVec3 vNormal = (v2-v1)^(v0-v1);
					if( !m_Plane.IsSameFacing(vNormal) )
					{
						RemovePrivateFlags(eRPF_Convex);
						break;
					}
				}
			}
		}
	}
	else
	{
		AddPrivateFlags(eRPF_Open);
	}
}

void CBrushRegion::OutputDebugData() const
{
#ifdef ENABLE_OUTPUT_DEBUGINFO
	OutputDebugData(m_Vertices,m_Edges);
#endif
}

void CBrushRegion::OutputDebugData( const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edges ) const
{
#ifdef ENABLE_OUTPUT_DEBUGINFO
	OutputDebugString("\n");
	for( int i = 0, iSize(edges.size()); i < iSize; ++i )
	{
		CString buffer;
		buffer.Format( "%d:%d,%d\n", i,edges[i].m_i[0], edges[i].m_i[1] );
		OutputDebugString(buffer);
	}
	for( int i = 0, iSize(vertices.size()); i < iSize; ++i )
	{
		CString buffer;
		BrushVec2 point(m_Plane.W2P(vertices[i]));
		buffer.Format("%d:%.16lf,%.16lf\n", i, point.x, point.y);
		OutputDebugString(buffer);
	}
#endif
}

void CBrushRegion::OutputDebugData( const std::vector<SVertexEx>& vertices, const std::vector<BUtil::SEdge>& edges ) const
{
#ifdef ENABLE_OUTPUT_DEBUGINFO
	OutputDebugString("\n");
	for( int i = 0, iSize(edges.size()); i < iSize; ++i )
	{
		CString buffer;
		buffer.Format( "%d:%d,%d\n", i,edges[i].m_i[0], edges[i].m_i[1] );
		OutputDebugString(buffer);
	}
	for( int i = 0, iSize(vertices.size()); i < iSize; ++i )
	{
		CString buffer;
		BrushVec2 point(m_Plane.W2P(vertices[i].m_v));
		buffer.Format( "%d:%.16lf,%.16lf\n", i, point.x, point.y );
		OutputDebugString(buffer);
	}
#endif
}

void CBrushRegion::OutputTestCode( RegionPtr pRegion0, RegionPtr pRegion1, const char* command, const BrushFloat& kEpsilon )
{
#ifdef ENABLE_OUTPUT_DEBUGINFO
	OutputDebugString("{\n");
	OutputDebugString("#ifdef DEBUG\n");
	OutputDebugString( "std::vector<BrushVec3> AList;\n" );

	CString buffer;

	std::vector<BrushVec3> vertices0;
	pRegion0->GetLinkedVertices(vertices0);
	for( int i = 0; i < vertices0.size(); ++i )
	{
		buffer.Format( "AList.push_back(BrushVec3(%.16lf,%.16lf,%.16lf));\n",vertices0[i].x, vertices0[i].y, vertices0[i].z );
		OutputDebugString(buffer);
	}

	buffer.Format("CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(%.16lf,%.16lf,%.16lf),%.16lf),0,NULL,true);\n",
		pRegion0->GetPlane().Normal().x,
		pRegion0->GetPlane().Normal().y,
		pRegion0->GetPlane().Normal().z,
		pRegion0->GetPlane().Distance());
	OutputDebugString(buffer);

	OutputDebugString( "std::vector<BrushVec3> BList;\n" );
	std::vector<BrushVec3> vertices1;
	pRegion1->GetLinkedVertices(vertices1);
	for( int i = 0; i < vertices1.size(); ++i )
	{
		buffer.Format( "BList.push_back(BrushVec3(%.16lf,%.16lf,%.16lf));\n",vertices1[i].x, vertices1[i].y, vertices1[i].z );
		OutputDebugString(buffer);
	}
	buffer.Format("CBrushRegion::RegionPtr BRegion = new CBrushRegion(BList,BrushPlane(BrushVec3(%.16lf,%.16lf,%.16lf),%.16lf),0,NULL,true);\n",
		pRegion1->GetPlane().Normal().x,
		pRegion1->GetPlane().Normal().y,
		pRegion1->GetPlane().Normal().z,
		pRegion1->GetPlane().Distance());
	OutputDebugString(buffer);

	OutputDebugString("ARegion->OutputDebugData();\n");
	OutputDebugString("BRegion->OutputDebugData();\n");	
	OutputDebugString("CBrushRegion::RegionPtr CRegion = ARegion->Clone();\n");

	buffer.Format("CRegion->%s(BRegion);\n",command);
	OutputDebugString(buffer);

	OutputDebugString("DESIGNER_ASSERT(CRegion->IsValid() && !CRegion->IsOpen());\n");
	OutputDebugString("CRegion->OutputDebugData();\n");

	OutputDebugString("IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();\n");
	OutputDebugString("dlg->AddRegion(ARegion.get(),\"ARegion\");\n");
	OutputDebugString("dlg->AddRegion(BRegion.get(),\"BRegion\");\n");
	OutputDebugString("dlg->ddRegion(CRegion.get(),\"CRegion\");\n");
	OutputDebugString("dlg->Open();\n");

	OutputDebugString("#endif\n");

	OutputDebugString("}\n");
#endif
}

void CBrushRegion::OutputTestCode() const
{
	OutputDebugString("{\n");
	OutputDebugString("#ifdef DEBUG\n");
	OutputDebugString( "std::vector<BrushVec3> AList;\n" );

	CString buffer;

	std::vector<BrushVec3> vertices0;
	GetLinkedVertices(vertices0);
	for( int i = 0; i < vertices0.size(); ++i )
	{
		buffer.Format( "AList.push_back(BrushVec3(%.16lf,%.16lf,%.16lf));\n",vertices0[i].x, vertices0[i].y, vertices0[i].z );
		OutputDebugString(buffer);
	}

	buffer.Format("CBrushRegion::RegionPtr ARegion = new CBrushRegion(AList,BrushPlane(BrushVec3(%.16lf,%.16lf,%.16lf),%.16lf),0,NULL,true);\n",
		GetPlane().Normal().x,
		GetPlane().Normal().y,
		GetPlane().Normal().z,
		GetPlane().Distance());

	OutputDebugString("IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();\n");
	OutputDebugString("dlg->AddRegion(ARegion.get(),\"ARegion\");\n");
	OutputDebugString("dlg->Open();\n");

	OutputDebugString("#endif\n");
	OutputDebugString("}\n");
}

bool CBrushRegion::ExtractEdge3DList( std::vector<BrushEdge3D>& outList ) const
{
	for( int i = 0, iSize(GetEdgeSize()); i < iSize; ++i )
	{
		BrushEdge3D edge = GetEdge(i);
		if( edge.IsPoint(kDesignerEpsilon) )
			return false;

		for( int a = 0; a < 3; ++a )
		{
			if( edge.m_v[0][a] != edge.m_v[1][a] && std::abs(edge.m_v[0][a]-edge.m_v[1][a]) < kDesignerEpsilon )
				edge.m_v[1][a] = edge.m_v[0][a];
		}

		outList.push_back(edge);
	}
	return true;
}

bool CBrushRegion::BuildBSP() const
{
	DESIGNER_ASSERT(!m_pBSPTree);
	if( m_pBSPTree )
		return false;
	if( !IsValid() )
		return false;
	if( IsOpen(m_Vertices,m_Edges) )
		return false;

	std::vector<BrushEdge3D> edgeList;
	if( !ExtractEdge3DList(edgeList) )
		return false;

	m_pBSPTree = new CBrushBSPTree2D();
	m_pBSPTree->AddRef();

	m_pBSPTree->BuildTree(GetPlane(),edgeList);

	return true;
}

void CBrushRegion::ModifyOrientation()
{
	if( !GetBSPTree() )
		return;

	if( !IsCCW() )
	{
		for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
			SwapEdgeIndex_Basic(i);
	}
}

void CBrushRegion::Clip( const CBrushBSPTree2D* pTree, BUtil::EClipType cliptype, std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges, BUtil::EClipObjective clipObjective, int nVertexID ) const
{
	if( !IsValid() )
		return;

	if( pTree == NULL )
		return;

	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		BrushEdge3D edge(GetVertex(m_Edges[i].m_i[0]),GetVertex(m_Edges[i].m_i[1]));
		CBrushBSPTree2D::SOutputEdges outEdges;

		pTree->GetPartitions( edge, outEdges );

		std::vector<BrushEdge3D::Edge3DList> validEdges;

		if( clipObjective == BUtil::eCO_JustClip )
		{
			if( cliptype == BUtil::eCT_Negative )
			{
				validEdges.push_back(outEdges.posList);
				validEdges.push_back(outEdges.coSameList);
			}
			else
			{
				validEdges.push_back(outEdges.negList);
				validEdges.push_back(outEdges.coDiffList);
			}
		}
		if( clipObjective == BUtil::eCO_Union0 )
		{
			if( cliptype == BUtil::eCT_Negative )
			{
				validEdges.push_back(outEdges.posList);
				validEdges.push_back(outEdges.coSameList);
			}
		}
		else if( clipObjective == BUtil::eCO_Union1 )
		{
			if( cliptype == BUtil::eCT_Negative )
				validEdges.push_back(outEdges.posList);
		}
		else if( clipObjective == BUtil::eCO_Intersection0 || clipObjective == BUtil::eCO_Intersection0IncludingCoSame )
		{
			if( cliptype == BUtil::eCT_Positive)
			{
				validEdges.push_back(outEdges.negList);
				if( clipObjective == BUtil::eCO_Intersection0IncludingCoSame )
					validEdges.push_back(outEdges.coSameList);
			}
		}
		else if( clipObjective == BUtil::eCO_Intersection1 || clipObjective == BUtil::eCO_Intersection1IncludingCoDiff )
		{
			if( cliptype == BUtil::eCT_Positive)
			{
				validEdges.push_back(outEdges.negList);
				if( clipObjective == BUtil::eCO_Intersection1IncludingCoDiff )
					validEdges.push_back(outEdges.coDiffList);
			}
		}
		else if( clipObjective == BUtil::eCO_Subtract )
		{
			if( cliptype == BUtil::eCT_Positive)
			{
				for( int a = 0, iCount(outEdges.negList.size()); a < iCount; ++a )
					std::swap(outEdges.negList[a].m_v[0],outEdges.negList[a].m_v[1]);
				validEdges.push_back(outEdges.negList);
			}
			else if( cliptype == BUtil::eCT_Negative)
			{
				validEdges.push_back(outEdges.posList);
				validEdges.push_back(outEdges.coDiffList);
			}
		}

		for( int a = 0; a < validEdges.size(); ++a )
		{
			for( int k = 0, iPosEdgeSize(validEdges[a].size()); k < iPosEdgeSize; k++ )
			{
				int index0( AddVertex( vertices, SVertexEx(validEdges[a][k].m_v[0],nVertexID) ));
				int index1( AddVertex( vertices, SVertexEx(validEdges[a][k].m_v[1],nVertexID) ));

				if( index0 == index1 )
					continue;

				BUtil::SEdge e(index0,index1);
				if( DoesIdenticalEdgeExist(edges,e)  )
					continue;

				edges.push_back(e);
			}
		}
	}
}

int CBrushRegion::AddVertex( std::vector<SVertexEx>& vertices, const SVertexEx& newVertex ) const
{
	for( int i = 0, vSize(vertices.size()); i < vSize; ++i )
	{
		if( newVertex == vertices[i] )
			return i;
	}
	vertices.push_back(newVertex);
	return vertices.size()-1;
}

int CBrushRegion::AddVertex( std::vector<BrushVec3>& vertices, const BrushVec3& newVertex ) const
{
	for( int i = 0, vSize(vertices.size()); i < vSize; ++i )
	{
		if( newVertex.IsEquivalent(vertices[i],kDesignerEpsilon) )
			return i;
	}
	vertices.push_back(newVertex);
	return vertices.size()-1;
}

void CBrushRegion::Transform2Vertices( const std::vector<BrushVec2>& points, const BrushPlane& plane )
{
	int iPtSize(points.size());
	DeleteAllVertices_Basic();
	m_Vertices.reserve(iPtSize);
	for( int i = 0; i < iPtSize; ++i )
		AddVertex_Basic(plane.P2W(points[i]));
}

bool CBrushRegion::GetAdjacentPrevEdgeIndicesWithVertexIndex( int vertexIndex, std::vector<int>& outEdgeIndices, const std::vector<BUtil::SEdge>& edges ) const
{
	for( int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i )
	{
		const BUtil::SEdge& edge(edges[i]);
		if( edge.m_i[1] == vertexIndex )
			outEdgeIndices.push_back(i);
	}

	return !outEdgeIndices.empty();
}

bool CBrushRegion::GetAdjacentNextEdgeIndicesWithVertexIndex( int vertexIndex, std::vector<int>& outEdgeIndices, const std::vector<BUtil::SEdge>& edges ) const
{
	for( int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i )
	{
		const BUtil::SEdge& edge(edges[i]);
		if( edge.m_i[0] == vertexIndex )
			outEdgeIndices.push_back(i);
	}

	return !outEdgeIndices.empty();
}

bool CBrushRegion::GetAdjacentEdgeIndexWithEdgeIndex( int edgeIndex, int& outPrevEdgeIndex, int& outNextEdgeIndex, const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edges ) const
{
	outPrevEdgeIndex = -1;
	outNextEdgeIndex = -1;

	const BUtil::SEdge& edge(edges[edgeIndex]);

	std::vector<int> edgesPreviousFirstVertex;
	if( GetAdjacentPrevEdgeIndicesWithVertexIndex( edge.m_i[0], edgesPreviousFirstVertex, edges ) )
	{
		std::vector<int>::iterator ii = edgesPreviousFirstVertex.begin();
		for( ; ii != edgesPreviousFirstVertex.end(); ++ii )
		{
			if( edge.m_i[1] == edges[*ii].m_i[0] && edge.m_i[0] == edges[*ii].m_i[1] )
			{
				edgesPreviousFirstVertex.erase(ii);
				break;
			}
		}
		if( edgesPreviousFirstVertex.size() == 1 )
		{
			outPrevEdgeIndex = *edgesPreviousFirstVertex.begin(); 
		}
		else if( edgesPreviousFirstVertex.size() > 1 )
		{
			BUtil::EdgeIndexSet candidateSecondIndices;
			for( int i = 0, iEdgeCount(edgesPreviousFirstVertex.size()); i < iEdgeCount; ++i )
				candidateSecondIndices.insert(edges[edgesPreviousFirstVertex[i]].m_i[0]);
			BUtil::SEdge outEdge;
			ChoosePrevEdge( vertices, edge, candidateSecondIndices, outEdge );
			GetEdgeIndex( edges, outEdge.m_i[0], outEdge.m_i[1], outPrevEdgeIndex );
		}
	}

	std::vector<int> edgesNextSecondVertex;
	if( GetAdjacentNextEdgeIndicesWithVertexIndex( edge.m_i[1], edgesNextSecondVertex, edges ) )
	{
		std::vector<int>::iterator ii = edgesNextSecondVertex.begin();
		for( ; ii != edgesNextSecondVertex.end(); ++ii )
		{
			if( edge.m_i[1] == edges[*ii].m_i[0] && edge.m_i[0] == edges[*ii].m_i[1] )
			{
				edgesNextSecondVertex.erase(ii);
				break;
			}
		}

		if( edgesNextSecondVertex.size() == 1 )
		{
			outNextEdgeIndex = *edgesNextSecondVertex.begin();
		}
		else if( edgesNextSecondVertex.size() > 1 )
		{
			BUtil::EdgeIndexSet candidateSecondIndices;
			for( int i = 0, iEdgeCount(edgesNextSecondVertex.size()); i < iEdgeCount; ++i )
				candidateSecondIndices.insert(edges[edgesNextSecondVertex[i]].m_i[1]);
			BUtil::SEdge outEdge;
			ChooseNextEdge( vertices, edge, candidateSecondIndices, outEdge );
			GetEdgeIndex( edges, outEdge.m_i[0], outEdge.m_i[1], outNextEdgeIndex );
		}
	}

	return outPrevEdgeIndex != -1 || outNextEdgeIndex != -1;
}

bool CBrushRegion::GetLinkedVertices( std::vector<BrushVec3>& outVertexList, const std::vector<BrushVec3>& vertices, const std::vector<BUtil::SEdge>& edges ) const
{	
	BUtil::SEdge edge(edges[0]);
	outVertexList.clear();
	int counter(0);

	if( IsOpen(vertices,edges) )
	{
		// In a case of edges not being closed, 
		// the starting edge which first index is connected to nothing should be found.
		for( int i = 0, nEdgeSize(edges.size()); i < nEdgeSize; ++i )
		{
			bool bExist(false);
			for( int k = 0; k < nEdgeSize; ++k )
			{
				if( i == k )
					continue;
				if( edges[i].m_i[0] == edges[k].m_i[1] )
				{
					bExist = true;
					break;
				}
			}
			if( !bExist )
			{
				edge = edges[i];
				break;
			}
		}
	}
	else if( !m_Plane.Normal().IsEquivalent(BrushVec3(0,0,0),0) )
	{
		BrushFloat smallestX = 3e10;
		int nSmallestVIndex = -1;
		for( int i = 0, iVertexCount(m_Vertices.size()); i < iVertexCount; ++i )
		{
			BrushVec2 v2D = m_Plane.W2P(m_Vertices[i]);
			if( v2D.x < smallestX )
			{
				smallestX = v2D.x;
				nSmallestVIndex = i;
			}
		}
		DESIGNER_ASSERT(nSmallestVIndex != -1);
		for( int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i )
		{
			if( m_Edges[i].m_i[0] == nSmallestVIndex || m_Edges[i].m_i[1] == nSmallestVIndex )
			{
				edge = m_Edges[i];
				break;
			}
		}
	}

	BUtil::SEdge entryEdge(edge);

	while( outVertexList.size() < edges.size() && (counter++) < edges.size() )
	{
		std::vector<int> linkedNextEdges;
		GetAdjacentNextEdgeIndicesWithVertexIndex(edge.m_i[1], linkedNextEdges, edges);
		if( linkedNextEdges.empty() )
		{
			if( edge.m_i[0] == edge.m_i[1] )
				return false;
			outVertexList.push_back(vertices[edge.m_i[0]]);
			outVertexList.push_back(vertices[edge.m_i[1]]);
			return true;
		}
		outVertexList.push_back(vertices[edge.m_i[0]]);

		if( linkedNextEdges.size() == 1 )
		{
			edge = edges[*linkedNextEdges.begin()];
		}
		else if( linkedNextEdges.size() > 1 )
		{
			BUtil::EdgeIndexSet candidateSecondIndices;
			for( int i = 0, iEdgeCount(linkedNextEdges.size()); i < iEdgeCount; ++i )
				candidateSecondIndices.insert(edges[linkedNextEdges[i]].m_i[1]);
			ChooseNextEdge( vertices, edge, candidateSecondIndices, edge );
		}

		if( edge == entryEdge )
			return true;
	}

	return outVertexList.size() == edges.size();
}

bool CBrushRegion::OptimizeEdges( std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges ) const
{
	RemoveEdgesHavingSameIndices(edges);
	RemoveEdgesRegardedAsVertex(vertices,edges);
	return FlattenEdges(vertices,edges);
}

bool CBrushRegion::FlattenEdges( std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges ) const
{
	std::set<int> usedEdges;
	std::vector<BUtil::SEdge> newEdges;
	newEdges.reserve(edges.size());
	for( int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i )
	{
		if( usedEdges.find(i) != usedEdges.end() )
			continue;

		std::vector<int> edgeLists[2];
		SearchLinkedColinearEdges( i, eSD_Previous, vertices, edges, edgeLists[eSD_Previous] );
		SearchLinkedColinearEdges( i, eSD_Next, vertices, edges, edgeLists[eSD_Next] );

		int edgeIndexInEnds[2]={i,i};
		for( int direction = 0; direction< 2; ++direction )
		{
			if( edgeLists[direction].empty() )
				continue;
			int iEdgeListCount = edgeLists[direction].size();
			for( int k = 0; k < iEdgeListCount; ++k )
			{
				int edgeIndex(edgeLists[direction][k]);
				if( usedEdges.find(edgeIndex) != usedEdges.end() )
				{
					DESIGNER_ASSERT(0);
					return false;
				}
				usedEdges.insert(edgeIndex);
			}
			edgeIndexInEnds[direction] = edgeLists[direction][iEdgeListCount-1];
		}
		BUtil::SEdge edge;
		if( edgeIndexInEnds[eSD_Previous] == i && edgeIndexInEnds[eSD_Next] == i )
		{ 
			edge = edges[i];
		}
		else
		{
			edge = BUtil::SEdge(
				edges[edgeIndexInEnds[eSD_Previous]].m_i[0],
				edges[edgeIndexInEnds[eSD_Next]].m_i[1]
			);
		}
		newEdges.push_back(edge);
	}
	edges = newEdges;
	return true;
}

void CBrushRegion::RemoveEdgesRegardedAsVertex( std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges ) const
{
	int iVertexSize(vertices.size());
	std::vector<BUtil::SEdge>::iterator ie = edges.begin();
	for( ; ie != edges.end(); )
	{
		BUtil::SEdge edge = *ie;
		if( edge.m_i[0] >= iVertexSize || edge.m_i[1] >= iVertexSize )
		{
			++ie;
			continue;
		}
		const SVertexEx& p0(vertices[edge.m_i[0]]);
		const SVertexEx& p1(vertices[edge.m_i[1]]);
		if( p0 == p1 )
		{
			ie = edges.erase(ie);
			BrushVec3 midVertex = (p0.m_v+p1.m_v)*0.5f;
			int nNewIndex = vertices.size();
			vertices.push_back(SVertexEx(midVertex,0));
			for( int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i )
			{
				if( edges[i].m_i[0] == edge.m_i[1] )
					edges[i].m_i[0] = nNewIndex;
				else if( edges[i].m_i[1] == edge.m_i[0] )
					edges[i].m_i[1] = nNewIndex;
			}
		}
		else
		{
			++ie;
		}
	}
}


void CBrushRegion::RemoveEdgesHavingSameIndices( std::vector<BUtil::SEdge>& edges )
{
	std::set<int> removedEdgeIndices;
	for( int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i )
	{
		if( removedEdgeIndices.find(i) != removedEdgeIndices.end() )
			continue;

		const BUtil::SEdge& edge0(edges[i]);
		if( edge0.m_i[0] == edge0.m_i[1] ) 
			removedEdgeIndices.insert(i);

		for( int k = i+1; k < iEdgeSize; ++k )
		{
			if( removedEdgeIndices.find(k) != removedEdgeIndices.end() )
				continue;
			const BUtil::SEdge& edge1(edges[k]);
			if( edge0.m_i[0] == edge1.m_i[0] && edge0.m_i[1] == edge1.m_i[1] )
				removedEdgeIndices.insert(k);
		}
	}
	std::vector<BUtil::SEdge>::iterator ii = edges.begin();
	for( int counter=0; ii != edges.end(); ++counter)
	{	
		if( removedEdgeIndices.find(counter) != removedEdgeIndices.end() )
			ii = edges.erase(ii);
		else
			++ii;
	}
}

void CBrushRegion::Convert2PureVertices( const std::vector<SVertexEx>& inputVertices, std::vector<BrushVec3>& outVertices ) const
{
	int iVertexCount(inputVertices.size());
	outVertices.reserve(iVertexCount);
	for( int i = 0, iVertexCount(inputVertices.size()); i < iVertexCount; ++i )
		outVertices.push_back(inputVertices[i].m_v);
}

void CBrushRegion::SearchLinkedColinearEdges( int edgeIndex, ESearchDirection direction, const std::vector<SVertexEx>& vertices, const std::vector<BUtil::SEdge>& edges, std::vector<int>& outLinkedEdges ) const
{
	int outEdgeIndices[2];
	BrushLine edgeLine(GetLineFromEdge(edgeIndex,vertices,edges));
	int counter(0);
	int edgeSize(edges.size());

	std::vector<BrushVec3> pureVertices;
	Convert2PureVertices( vertices, pureVertices );

	while( (counter++) < edgeSize )
	{
		if( !GetAdjacentEdgeIndexWithEdgeIndex( edgeIndex, outEdgeIndices[eSD_Previous], outEdgeIndices[eSD_Next], pureVertices, edges ) )
			break;
		if( outEdgeIndices[direction] == -1 )
			break;
		BrushLine adjacentEdgeLine(GetLineFromEdge(outEdgeIndices[direction],vertices,edges));
		if( !edgeLine.IsEquivalent(adjacentEdgeLine,kDesignerEpsilon) )
			break;
		edgeIndex = outEdgeIndices[direction];
		outLinkedEdges.push_back(edgeIndex);
	}	
}

void CBrushRegion::SearchLinkedEdges( int edgeIndex, ESearchDirection direction, const std::vector<SVertexEx>& vertices, const std::vector<BUtil::SEdge>& edges, std::vector<int>& outLinkedEdges ) const
{
	int edgeIndices[2];
	int counter(0);
	int edgeSize(edges.size());

	std::vector<BrushVec3> pureVertices;
	Convert2PureVertices( vertices, pureVertices );

	while( (counter++) < edgeSize )
	{
		if( !GetAdjacentEdgeIndexWithEdgeIndex( edgeIndex, edgeIndices[eSD_Previous], edgeIndices[eSD_Next], pureVertices, edges ) )
			break;
		if( edgeIndices[direction] == -1 || edgeIndices[direction] == edgeIndex )
			break;
		edgeIndex = edgeIndices[direction];
		outLinkedEdges.push_back(edgeIndex);
	}
}

void CBrushRegion::OptimizeVertices( std::vector<SVertexEx>& vertices, std::vector<BUtil::SEdge>& edges ) const
{
	std::vector<SVertexEx> newVertices;
	std::map<int,int> vertexMapper;
	newVertices.reserve(vertices.size());
	for( int k = 0, vertexSize(vertices.size()); k < vertexSize; ++k )
	{
		for( int i = 0, edgeSize(edges.size()); i < edgeSize; ++i )
		{
			for( int a = 0; a < 2; ++a )
			{
				if( edges[i].m_i[a] == k )
				{
					if( vertexMapper.find(k) == vertexMapper.end() )
					{
						int newIndex = newVertices.size();
						vertexMapper[k] = newIndex;
						newVertices.push_back(vertices[k]);
						edges[i].m_i[a] = newIndex;
					}
					else
					{
						edges[i].m_i[a] = vertexMapper[edges[i].m_i[a]];
					}
				}
			}
		}
	}
	vertices = newVertices;
}

bool CBrushRegion::DoesIdenticalEdgeExist( const std::vector<BUtil::SEdge>& edges, const BUtil::SEdge& e, int* pOutIndex )
{
	for( int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i )
	{
		if( edges[i].m_i[0] == e.m_i[0] && edges[i].m_i[1] == e.m_i[1] )
		{
			if( pOutIndex )
				*pOutIndex = i;
			return true;
		}
	}
	return false;
}

bool CBrushRegion::DoesReverseEdgeExist( const std::vector<BUtil::SEdge>& edges, const BUtil::SEdge& e, int* pOutIndex )
{
	for( int i = 0, iEdgeSize(edges.size()); i < iEdgeSize; ++i )
	{
		if( edges[i].m_i[1] == e.m_i[0] && edges[i].m_i[0] == e.m_i[1] )
		{
			if( pOutIndex )
				*pOutIndex = i;
			return true;
		}
	}
	return false;
}

BrushFloat CBrushRegion::Cosine( int i0, int i1, int i2, const std::vector<BrushVec3>& vertices ) const
{
	BrushVec2 p0 = m_Plane.W2P(vertices[i0]);
	BrushVec2 p1 = m_Plane.W2P(vertices[i1]);
	BrushVec2 p2 = m_Plane.W2P(vertices[i2]);

	const BrushVec2 p10 = (p0-p1).GetNormalized();
	const BrushVec2 p12 = (p2-p1).GetNormalized();

	return p10.Dot(p12);
}

bool CBrushRegion::IsCCW( int i0, int i1, int i2, const std::vector<BrushVec3>& vertices ) const
{
	const BrushVec3& v0 = m_Plane.W2P(vertices[i0]);
	const BrushVec3& v1 = m_Plane.W2P(vertices[i1]);
	const BrushVec3& v2 = m_Plane.W2P(vertices[i2]);

	BrushVec3 v10 = (v0-v1).GetNormalized();
	BrushVec3 v12 = (v2-v1).GetNormalized();

	BrushVec3 v10_x_v12 = v10.Cross(v12);

	return v10_x_v12.z > 0;
}

void CBrushRegion::FindLoops( std::vector<BUtil::EdgeList>& outLoopList ) const
{
	BUtil::EdgeSet handledEdgeSet;
	BUtil::EdgeSet edgeSet;
	BUtil::EdgeMap edgeMapFrom1stTo2nd;
	BUtil::EdgeMap edgeMapFrom2ndTo1st;

	for( int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i )
	{
		edgeSet.insert(m_Edges[i]);

		BUtil::EdgeMap::iterator iEdge = edgeMapFrom1stTo2nd.find(m_Edges[i].m_i[0]);
		edgeMapFrom1stTo2nd[m_Edges[i].m_i[0]].insert(m_Edges[i].m_i[1]);
		edgeMapFrom2ndTo1st[m_Edges[i].m_i[1]].insert(m_Edges[i].m_i[0]);
	}

	int nCounter(0);
	BUtil::EdgeSet::iterator iEdgeSet = edgeSet.begin();
	for( iEdgeSet = edgeSet.begin(); iEdgeSet != edgeSet.end(); ++iEdgeSet )
	{
		if( edgeMapFrom1stTo2nd[(*iEdgeSet).m_i[0]].size() > 1 || edgeMapFrom2ndTo1st[(*iEdgeSet).m_i[1]].size() > 1 )
			continue;

		if( handledEdgeSet.find(*iEdgeSet) != handledEdgeSet.end() )
			continue;

		std::vector<int> subPiece;
		BUtil::SEdge edge = *iEdgeSet;
		handledEdgeSet.insert(edge);

		do
		{
			if( edgeMapFrom1stTo2nd.find(edge.m_i[1]) != edgeMapFrom1stTo2nd.end() )
			{
				subPiece.push_back(edge.m_i[0]);
			}
			else
			{
				subPiece.clear();
				break;
			}

			const BUtil::EdgeIndexSet& secondIndexSet = edgeMapFrom1stTo2nd[edge.m_i[1]];

			if( secondIndexSet.size() == 1 )
			{
				edge = BUtil::SEdge(edge.m_i[1],*secondIndexSet.begin());
			}
			else if( secondIndexSet.size() > 1 )
			{
				ChooseNextEdge( m_Vertices, BUtil::SEdge(edge.m_i[0],edge.m_i[1]), secondIndexSet, edge );
			}
			if( handledEdgeSet.find(edge) != handledEdgeSet.end() )
			{
				subPiece.clear();
				break;
			}
			if(++nCounter>10000)
			{
				DESIGNER_ASSERT(0 && "Searching connected an edge doesn't seem possible.");
#ifdef ENABLE_OUTPUT_DEBUGINFO
				IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
				dlg->AddRegion(this->Clone().get(),"ARegion");
				dlg->Open();
#endif
				return;
			}
		}while( edge.m_i[1] != (*iEdgeSet).m_i[0] );

		if( !subPiece.empty() )
		{
			BUtil::EdgeList subLoop;
			subPiece.push_back(edge.m_i[0]);
			for( int i = 0, subPieceCount(subPiece.size()); i < subPieceCount; ++i )
			{
				subLoop.push_back(BUtil::SEdge(subPiece[i],subPiece[(i+1)%subPieceCount]));
				handledEdgeSet.insert(BUtil::SEdge(subPiece[i],subPiece[(i+1)%subPieceCount]));
			}
			outLoopList.push_back(subLoop);
		}
	}
}

void CBrushRegion::ChoosePrevEdge( const std::vector<BrushVec3>& vertices, const BUtil::SEdge& edge, const BUtil::EdgeIndexSet& candidateSecondIndices, BUtil::SEdge& outEdge ) const
{
	BrushFloat cwCosMax = -1.5f;
	BrushFloat ccwCosMin = 1.5f;
	int ccwIndex = -1;
	int cwIndex = -1;
	BUtil::EdgeIndexSet::iterator iSecondIndexSet = candidateSecondIndices.begin();
	for( ; iSecondIndexSet != candidateSecondIndices.end() ; ++iSecondIndexSet )
	{
		if( *iSecondIndexSet == edge.m_i[1] )
			continue;
		BrushFloat cosine = Cosine(edge.m_i[1], edge.m_i[0], *iSecondIndexSet, vertices);
		if( IsCW(edge.m_i[1], edge.m_i[0], *iSecondIndexSet, vertices) )
		{
			if( cosine > cwCosMax )
			{
				cwIndex = *iSecondIndexSet;
				cwCosMax = cosine;
			}
		}  
		else if( cosine < ccwCosMin )
		{
			ccwIndex = *iSecondIndexSet;
			ccwCosMin = cosine;
		}
	}

	if( cwIndex != -1 )
		outEdge = BUtil::SEdge(cwIndex,edge.m_i[0]);
	else
		outEdge = BUtil::SEdge(ccwIndex,edge.m_i[0]);
}

void CBrushRegion::ChooseNextEdge( const std::vector<BrushVec3>& vertices, const BUtil::SEdge& edge, const BUtil::EdgeIndexSet& candidateSecondIndices, BUtil::SEdge& outEdge ) const
{
	BrushFloat ccwCosMax = -1.5f;
	BrushFloat cwCosMin = 1.5f;
	int ccwIndex = -1;
	int cwIndex = -1;
	BUtil::EdgeIndexSet::iterator iSecondIndexSet = candidateSecondIndices.begin();
	for( ; iSecondIndexSet != candidateSecondIndices.end() ; ++iSecondIndexSet )
	{
		if( *iSecondIndexSet == edge.m_i[0] )
			continue;
		BrushFloat cosine = Cosine(edge.m_i[0], edge.m_i[1], *iSecondIndexSet, vertices);
		if( IsCCW(edge.m_i[0], edge.m_i[1], *iSecondIndexSet, vertices) )
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
		outEdge = BUtil::SEdge(edge.m_i[1],ccwIndex);
	else
		outEdge = BUtil::SEdge(edge.m_i[1],cwIndex);
}

bool CBrushRegion::CreateNewRegionFromEdges( const std::vector<BUtil::SEdge>& inputEdges, RegionPtr& outRegion, bool bOptimizeRegion ) const
{
	std::vector<BrushVec3> vertices;
	std::vector<BUtil::SEdge> edges;

	for( int i = 0, iSize(inputEdges.size()); i < iSize; ++i )
	{
		const BUtil::SEdge& edge(inputEdges[i]);
		int edgeIndex0( AddVertex( vertices, GetVertex(edge.m_i[0]) ));
		int edgeIndex1( AddVertex( vertices, GetVertex(edge.m_i[1]) ));

		if( edgeIndex0 != edgeIndex1 )
			edges.push_back(BUtil::SEdge(edgeIndex0, edgeIndex1));
	}

	if( vertices.size() >= 3 && edges.size() >= 3 )
	{
		outRegion = new CBrushRegion( vertices, edges, GetPlane(), m_MaterialID, &m_TexInfo, bOptimizeRegion );				
		return true;
	}

	outRegion = NULL;
	return false;
}

void CBrushRegion::SaveBinary( CArchive& ar )
{
	int nVertexCount = m_Vertices.size();
	ar.Write( &nVertexCount, sizeof(int) );
	if( nVertexCount > 0 )
		ar.Write( &m_Vertices[0], sizeof(BrushVec3)*nVertexCount );
	int nEdgeCount = m_Edges.size();
	ar.Write( &nEdgeCount, sizeof(int) );
	if( nEdgeCount > 0 )
		ar.Write( &m_Edges[0], sizeof(BUtil::SEdge)*nEdgeCount );
	ar.Write(&m_Flag,sizeof(unsigned int));
	ar.Write(&m_MaterialID,sizeof(int));
	ar.Write(&m_TexInfo,sizeof(BUtil::STexInfo));
	BrushFloat distance = GetPlane().Distance();
	ar.Write(&GetPlane().Normal(),sizeof(BrushVec3));
	ar.Write(&distance,sizeof(BrushFloat));
}

void CBrushRegion::SaveBinary( std::vector<char>& buffer )
{
	int nVertexCount = m_Vertices.size();
	int nEdgeCount = m_Edges.size();

	buffer.reserve(nVertexCount*sizeof(BrushVec3)+nEdgeCount*sizeof(nEdgeCount)+100);

	BUtil::Write2Buffer(buffer, &nVertexCount,sizeof(int));	
	if( nVertexCount > 0 )
		BUtil::Write2Buffer(buffer, &m_Vertices[0], sizeof(BrushVec3)*nVertexCount);
	BUtil::Write2Buffer(buffer, &nEdgeCount, sizeof(int));
	if( nEdgeCount > 0 )
		BUtil::Write2Buffer(buffer, &m_Edges[0], sizeof(BUtil::SEdge)*nEdgeCount);
	BUtil::Write2Buffer(buffer,&m_Flag,sizeof(unsigned int));
	BUtil::Write2Buffer(buffer,&m_MaterialID,sizeof(int));
	BUtil::Write2Buffer(buffer,&m_TexInfo,sizeof(BUtil::STexInfo));
	BrushFloat distance = GetPlane().Distance();
	BUtil::Write2Buffer(buffer,&GetPlane().Normal(),sizeof(BrushVec3));
	BUtil::Write2Buffer(buffer,&distance,sizeof(BrushFloat));
}

void CBrushRegion::LoadBinary( CArchive& ar )
{
	int nVertexCount = 0;
	ar.Read(&nVertexCount, sizeof(int));
	m_Vertices.clear();
	if( nVertexCount > 0 )
	{
		m_Vertices.resize(nVertexCount);
		ar.Read(&m_Vertices[0], sizeof(BrushVec3)*nVertexCount);
	}
	int nEdgeCount = 0;
	m_Edges.clear();
	ar.Read(&nEdgeCount, sizeof(int));
	if( nEdgeCount > 0 )
	{
		m_Edges.resize(nEdgeCount);
		ar.Read(&m_Edges[0], sizeof(BUtil::SEdge)*nEdgeCount);
	}
	ar.Read(&m_Flag,sizeof(unsigned int));
	ar.Read(&m_MaterialID,sizeof(int));
	ar.Read(&m_TexInfo,sizeof(BUtil::STexInfo));
	BrushVec3 planeNormal = GetPlane().Normal();
	ar.Read(&planeNormal,sizeof(BrushVec3));
	BrushFloat planeDist = GetPlane().Distance();
	ar.Read(&planeDist,sizeof(BrushFloat));
	SetPlane(BrushPlane(planeNormal,planeDist));
}

void CBrushRegion::LoadBinary( std::vector<char>& buffer )
{
	int nBufferPos = 0;
	int nVertexCount = 0;
	nBufferPos = BUtil::ReadFromBuffer(buffer, nBufferPos, &nVertexCount, sizeof(int));
	m_Vertices.clear();
	if( nVertexCount > 0 )
	{
		m_Vertices.resize(nVertexCount);
		nBufferPos = BUtil::ReadFromBuffer(buffer, nBufferPos, &m_Vertices[0], sizeof(BrushVec3)*nVertexCount);
	}
	int nEdgeCount = 0;
	m_Edges.clear();
	nBufferPos = BUtil::ReadFromBuffer(buffer, nBufferPos, &nEdgeCount, sizeof(int));
	if( nEdgeCount > 0 )
	{
		m_Edges.resize(nEdgeCount);
		nBufferPos = BUtil::ReadFromBuffer(buffer, nBufferPos, &m_Edges[0], sizeof(BUtil::SEdge)*nEdgeCount);
	}
	nBufferPos = BUtil::ReadFromBuffer(buffer, nBufferPos, &m_Flag,sizeof(unsigned int));
	nBufferPos = BUtil::ReadFromBuffer(buffer, nBufferPos, &m_MaterialID,sizeof(int));
	nBufferPos = BUtil::ReadFromBuffer(buffer, nBufferPos, &m_TexInfo,sizeof(BUtil::STexInfo));
	BrushVec3 planeNormal = GetPlane().Normal();
	nBufferPos = BUtil::ReadFromBuffer(buffer, nBufferPos, &planeNormal,sizeof(BrushVec3));
	BrushFloat planeDist = GetPlane().Distance();
	nBufferPos = BUtil::ReadFromBuffer(buffer, nBufferPos, &planeDist,sizeof(BrushFloat));
	SetPlane(BrushPlane(planeNormal,planeDist));
}

void CBrushRegion::Serialize( XmlNodeRef &xmlNode, bool bLoading, bool bUndo )
{
	if( bLoading )
	{
		const char* srcString = NULL;
		if( xmlNode->getAttr("BinaryData",&srcString) )
		{
			int nLength = strlen(srcString);
			if( nLength > 0 )
			{
				std::vector<char> buffer;
				int nDestBufferLen = Base64::decodedsize_base64(nLength);
				buffer.resize(nDestBufferLen);
				Base64::decode_base64(&buffer[0],srcString,nLength,false);
				if( !buffer.empty() )
					LoadBinary(buffer);
			}
		}
		else
		{
			Load( xmlNode, bUndo );
		}

		xmlNode->getAttr("GUID",m_GUID);
		UpdateBoundBox();
		Optimize();
	}
	else 
	{
		std::vector<char> buffer;
		SaveBinary(buffer);
		if( !buffer.empty() )
		{
			std::vector<char> encodedStr(Base64::encodedsize_base64(buffer.size())+1);
			Base64::encode_base64(&encodedStr[0],&buffer[0],buffer.size(),true);
			xmlNode->setAttr("GUID",m_GUID);
			xmlNode->setAttr("BinaryData",&encodedStr[0]);
		}
	}
}

void CBrushRegion::Load( XmlNodeRef &xmlNode, bool bUndo )
{
	BrushVec3 p_normal(0,0,0);
	BrushFloat p_distance(0);

	xmlNode->getAttr( "matID", m_MaterialID );
	m_TexInfo.Load(xmlNode);

	DeleteAllVertices_Basic();
	int nVer(0);
	xmlNode->getAttr("Ver",nVer);
	for( int i = 0;; ++i )
	{
		if( nVer == 0 )
		{
			BrushVec2 position;
			CString attribute;
			attribute.Format("p%d", i );
			if( !xmlNode->getAttr( attribute, position ) )
				break;
			AddVertex_Basic(m_Plane.P2W(position));
		}
		else if( nVer == 1 )
		{
			BrushVec3 position;
			CString attribute;
			attribute.Format("v%d", i );
			if( !xmlNode->getAttr( attribute, position ) )
				break;
			AddVertex_Basic(position);
		}
	}

	DeleteAllEdges_Basic();
	for( int i = 0;; ++i )
	{
		CString attribute;
		attribute.Format("e%d", i );
		int edgeindices(0);
		if( !xmlNode->getAttr(attribute,edgeindices) )
			break;
		int e[2] = { (edgeindices&0xFFFF0000)>>16, edgeindices&0x0000FFFF };
		if( e[0] >= m_Vertices.size() || e[1] >= m_Vertices.size() )
		{
			DESIGNER_ASSERT(0);
			continue;
		}
		AddEdge_Basic(BUtil::SEdge(e[0],e[1]));
	}

	xmlNode->getAttr("Flags", m_Flag);

	bool bReadPlaneNormal = xmlNode->getAttr( "planeNormal", p_normal );
	bool bReadPlaneDist = xmlNode->getAttr( "planeDistance", p_distance );

	if( IsValid() && (!bReadPlaneNormal || !bReadPlaneDist || !IsOpen()) )
	{
		BrushPlane plane;
		if( GetComputedPlane(plane) )
			SetPlane(plane);
		else
			Clear();
	}
	else
	{
		m_Plane.Set(p_normal,p_distance);
	}
}

void CBrushRegion::Save( XmlNodeRef &xmlNode, bool bUndo )
{
	xmlNode->setAttr("Ver","1");

	for( int i = 0, iSize(m_Vertices.size()); i < iSize; ++i )
	{
		CString attribute;
		attribute.Format("v%d", i );
		xmlNode->setAttr( attribute, m_Vertices[i] );
	}

	for( int i = 0, iSize(m_Edges.size()); i < iSize; ++i )
	{
		CString attribute;
		attribute.Format("e%d", i );
		int edgeindices((m_Edges[i].m_i[0]<<16)|(m_Edges[i].m_i[1]));
		xmlNode->setAttr(attribute,edgeindices);
	}

	xmlNode->setAttr( "matID", m_MaterialID );
	m_TexInfo.Save(xmlNode);

	if( IsOpen() )
	{
		xmlNode->setAttr( "planeNormal", GetPlane().Normal() );
		xmlNode->setAttr( "planeDistance", GetPlane().Distance() ) ;
	}

	xmlNode->setAttr( "Flags", m_Flag );
}

void CBrushRegion::AddEdges( const std::vector<BrushVec2>& positivePoints, const std::vector<BrushVec2>& negativePoints, std::vector<BrushVec2>& outPoints, std::vector<BUtil::SEdge>& outEdges )
{
	if( positivePoints.empty() || negativePoints.empty() )
	{
		DESIGNER_ASSERT(0);
		return;
	}

	int basePointIndex = outPoints.size();

	for( int i = 0, iSize(positivePoints.size()); i < iSize; ++i )
		outPoints.push_back(positivePoints[i]);

	for( int i = 0, iSize(negativePoints.size()); i < iSize; ++i )
		outPoints.push_back(negativePoints[iSize-i-1]);

	for( int i = basePointIndex, iPointSize(outPoints.size()); i < iPointSize; ++i )
	{
		int nexti = (i+1 == iPointSize) ? basePointIndex : i+1;
		outEdges.push_back(BUtil::SEdge(i,nexti));
	}
}

bool CBrushRegion::UpdatePlane( const BrushPlane& plane, const BrushVec3& directionForHitTest )
{
	if( plane.IsEquivalent(GetPlane(),kDesignerEpsilon) )
		return false;

	std::vector<BrushVec3> newVertices;
	std::vector<BUtil::SEdge> newEdges;

	int numberOfVertices = (int)m_Vertices.size();
	int numberOfEdges = (int)m_Edges.size();

	newVertices.reserve(numberOfVertices);
	newEdges.reserve(numberOfEdges);

	std::map<int,int> indexMap;

	for( int i = 0; i < numberOfVertices; ++i )
	{
		BrushVec3 worldPT(GetVertex(i));
		BrushVec3 vOut;
		if( plane.HitTest( worldPT, worldPT+directionForHitTest, kDesignerEpsilon, NULL, &vOut ) )
			indexMap[i] = AddVertex(newVertices,vOut);
	}

	if( newVertices.size() < 3 )
		return false;

	for( int i = 0; i < numberOfEdges; ++i )
	{
		std::map<int,int>::iterator iIndex0 = indexMap.find(m_Edges[i].m_i[0]);
		if( iIndex0 == indexMap.end() )
		{
			DESIGNER_ASSERT(0);
			continue;
		}
		std::map<int,int>::iterator iIndex1 = indexMap.find(m_Edges[i].m_i[1]);
		if( iIndex1 == indexMap.end() )
		{
			DESIGNER_ASSERT(0);
			continue;
		}
		int newIndex0 = iIndex0->second;
		int newIndex1 = iIndex1->second;
		if( newIndex0 == newIndex1 )
			continue;
		newEdges.push_back(BUtil::SEdge(newIndex0,newIndex1));
	}

	if( newEdges.size() < 3 )
		return false;

	SetVertexList_Basic(newVertices);
	SetEdgeList_Basic(newEdges);

	if( plane.Normal().Dot(GetPlane().Normal()) < 0 )
		ReverseEdges();

	SetPlane(plane);

	return true;
}

BrushVec3 CBrushRegion::GetCenterPosition() const
{
	BrushVec3 minVertex(3e10f,3e10f,3e10f);
	BrushVec3 maxVertex(-3e10f,-3e10f,-3e10f);
	BrushVec3 centerPos(0,0,0);
	int iVertexSize(m_Vertices.size());
	for( int i = 0; i < iVertexSize; ++i )
	{
		if( GetVertex(i).x < minVertex.x )
			minVertex.x = GetVertex(i).x;
		if( GetVertex(i).x > maxVertex.x )
			maxVertex.x = GetVertex(i).x;
		if( GetVertex(i).y < minVertex.y )
			minVertex.y = GetVertex(i).y;
		if( GetVertex(i).y > maxVertex.y )
			maxVertex.y = GetVertex(i).y;
		if( GetVertex(i).z < minVertex.z )
			minVertex.z = GetVertex(i).z;
		if( GetVertex(i).z > maxVertex.z )
			maxVertex.z = GetVertex(i).z;
	}
	centerPos = (minVertex+maxVertex)*0.5f;
	return centerPos;
}

BrushVec3 CBrushRegion::GetAveragePosition()	const
{
	BrushVec3 averagePos(0,0,0);
	int iVertexSize(m_Vertices.size());
	for( int i = 0; i < iVertexSize; ++i )
		averagePos += GetVertex(i);
	averagePos /= iVertexSize;
	return averagePos;
}

BrushVec3 CBrushRegion::GetRepresentativePosition() const
{
	if( m_bRepresentativePosValid )
		return m_RepresentativePos;

#if 1
	if( IsOpen() )
	{
		std::vector<BrushVec3> linkedVertices;
		GetLinkedVertices(linkedVertices);
		if( !linkedVertices.empty() )
		{
			int nSize = linkedVertices.size();
			if( (nSize%2) == 1 )
				m_RepresentativePos = linkedVertices[nSize/2];
			else
				m_RepresentativePos = (BrushFloat)0.5*(linkedVertices[nSize/2]+linkedVertices[(nSize/2-1)]);
		}
	}
	else
	{
		m_RepresentativePos = GetAveragePosition();
	}
#else
	if( IsConvex() )
	{
		m_RepresentativePos = GetAveragePosition();	
	}
	else
	{
		std::map< BrushFloat,int,std::greater<BrushFloat> > verticesSortedByDistance;
		for( int i = 1, iVertexCount(m_Vertices.size()); i < iVertexCount; ++i )
		{
			BrushVec3 vDir = m_Vertices[i] - m_Vertices[0];
			verticesSortedByDistance[vDir.GetLength()] = i;
		}

		bool bFound = false;

		std::map< BrushFloat,int,std::greater<BrushFloat> >::iterator iter = verticesSortedByDistance.begin();
		for( ; iter != verticesSortedByDistance.end(); ++iter )
		{
			int nTargetIndex = iter->second;
			if( HasEdge(BrushEdge3D(m_Vertices[0],m_Vertices[nTargetIndex])) )
				continue;
			BrushVec3 vDir = m_Vertices[nTargetIndex] - m_Vertices[0];
			BrushVec3 v0 = m_Vertices[0] + vDir*(BrushFloat)0.001f;
			BrushVec3 v1 = m_Vertices[nTargetIndex] - vDir*(BrushFloat)0.001f;
			if( GetBSPTree()->IsInside(BrushEdge3D(v0,v1),false) )
			{
				m_RepresentativePos = (v0+v1)*(BrushFloat)0.5f;
				bFound = true;
				break;
			}
		}

		if( !bFound )
			m_RepresentativePos = m_Vertices[0];
	}
#endif

	m_bRepresentativePosValid = true;
	return m_RepresentativePos;
}

bool CBrushRegion::Scale( const BrushFloat& kScale, bool bCheckBoundary, std::vector<BrushEdge3D>* pOutEdgesBeforeOptimization )
{
	if( std::abs(kScale) < BrushFloat(0.001) )
		return true;

	if( IsOpen() )
		return false;

	if( bCheckBoundary )
	{
		std::vector<RegionPtr> outRegion;
		std::vector<RegionPtr> innerRegions;
		GetSeparatedRegions( outRegion, eSR_OuterHull );
		GetSeparatedRegions( innerRegions, eSR_InnerHull );

		if( outRegion.size() == 1 && !innerRegions.empty() )
		{
			if( !outRegion[0]->Scale(kScale,false) )
				return false;

			int innerRegionSize(innerRegions.size());
			for( int i = 0; i < innerRegionSize; ++i )
			{
				if( !innerRegions[i]->Scale(kScale,false) )
					return false;
				innerRegions[i]->ReverseEdges();
			}

			if( !outRegion[0]->IncludeAllEdges(innerRegions[0]) )
				return false;

			for( int i = 0; i < innerRegionSize; ++i )
			{
				if( !outRegion[0]->IncludeAllEdges(innerRegions[i]) )
					return false;
				for( int k = i+1; k < innerRegionSize; ++k )
				{
					BUtil::EIntersectionType interestionType = CBrushRegion::HasIntersection(innerRegions[i], innerRegions[k]);
					if( interestionType != BUtil::eIT_None )
						return false;
				}
			}
		}
	}

	std::vector<BrushEdge> edges;
	std::vector<BrushLine> lines;

	int iEdgeSize(m_Edges.size());

	edges.resize(iEdgeSize);
	lines.resize(iEdgeSize);

	for( int i = 0; i < iEdgeSize; ++i )
	{
		BrushEdge3D edge3D = GetEdge(i);
		edges[i] = BrushEdge(m_Plane.W2P(edge3D.m_v[0]),m_Plane.W2P(edge3D.m_v[1]));

		BrushLine line(edges[i].m_v[0],edges[i].m_v[1]);
		line.m_Distance += kScale;
		lines[i] = line;
	}

	for( int i = 0; i < iEdgeSize; ++i )
	{
		int edgeIndices[2] = {-1,-1}; 
		if( !GetAdjacentEdgeIndexWithEdgeIndex( i, edgeIndices[eSD_Previous], edgeIndices[eSD_Next], m_Vertices, m_Edges ) )
		{
			DESIGNER_ASSERT(0);
			return false;
		}

		if( edgeIndices[eSD_Previous] != -1 && !lines[i].Intersect( lines[edgeIndices[eSD_Previous]], edges[i].m_v[0], kDesignerEpsilon*kDesignerEpsilon) )
		{
			if( !lines[i].HitTest( edges[i].m_v[0], edges[i].m_v[0]+lines[i].m_Normal, kDesignerEpsilon, NULL, &edges[i].m_v[0] ) )
			{
				DESIGNER_ASSERT(0);
				return false;
			}
		}

		if( edgeIndices[eSD_Next] != -1 && !lines[i].Intersect( lines[edgeIndices[eSD_Next]], edges[i].m_v[1], kDesignerEpsilon*kDesignerEpsilon) )
		{
			if( !lines[i].HitTest( edges[i].m_v[1], edges[i].m_v[1]+lines[i].m_Normal, kDesignerEpsilon, NULL, &edges[i].m_v[1] ) )
			{
				DESIGNER_ASSERT(0);
				return false;
			}
		}
	}

	std::vector<BrushEdge3D> edges3D;
	edges3D.reserve(iEdgeSize);
	for( int i = 0; i < iEdgeSize; ++i )
	{
		BrushEdge3D edge3D = BrushEdge3D(m_Plane.P2W(edges[i].m_v[0]),m_Plane.P2W(edges[i].m_v[1]));

		Vec3_tpl<BrushFloat> vScaledDir = edge3D.m_v[1] - edge3D.m_v[0];
		Vec3_tpl<BrushFloat> vOriginalDir = GetVertex(m_Edges[i].m_i[1]) - GetVertex(m_Edges[i].m_i[0]);

		if( bCheckBoundary )
		{
			if( vScaledDir.Dot(vOriginalDir) <= 0 )
				return false;

			int nNextI = (i+1)%iEdgeSize;
			int nPrevI = (i-1) < 0 ? iEdgeSize-1 : (i-1);

			for( int k = 0; k < iEdgeSize; ++k )
			{
				if( k == i || k == nNextI || k == nPrevI )
					continue;
				if( edges[i].IsIntersect(edges[k], kDesignerEpsilon) )
					return false;
			}
		}

		edges3D.push_back(edge3D);
	}

	if( pOutEdgesBeforeOptimization )
	{
		for( int i = 0, iEdgeSize(edges3D.size()); i < iEdgeSize; ++i )
			*pOutEdgesBeforeOptimization = edges3D;
	}

	if( !Optimize(edges3D) )
		return false;

	if( IsOpen() )
		return false;

	return true;
}

bool CBrushRegion::BroadenVertex( const BrushFloat& kScale, int nVertexIndex, const BrushEdge3D* pBaseEdge )
{
	int nPrevEdgeIndex = -1;
	int nNextEdgeIndex = -1;
	if( !GetAdjacentEdgesByVertexIndex( nVertexIndex, &nPrevEdgeIndex, &nNextEdgeIndex ) )
		return false;

	BrushEdge3D nextEdge = GetEdge(nNextEdgeIndex);
	BrushEdge3D prevEdge = GetEdge(nPrevEdgeIndex);
	BrushFloat fDeltaToNextEdge = kScale;
	BrushFloat fDeltaToPrevEdge = kScale;
	if( pBaseEdge )
	{
		BrushEdge3D baseEdge(*pBaseEdge);
		if( !baseEdge.m_v[0].IsEquivalent(nextEdge.m_v[0],kDesignerEpsilon) )
			baseEdge.Invert();

		BrushVec3 vBaseEdgeDir = (baseEdge.m_v[1]-baseEdge.m_v[0]).GetNormalized();
		BrushVec3 vNextEdgeDir = (nextEdge.m_v[1]-nextEdge.m_v[0]).GetNormalized();
		BrushVec3 vPrevEdgeDir = (prevEdge.m_v[1]-prevEdge.m_v[0]).GetNormalized();

		if( nextEdge.m_v[0].IsEquivalent(baseEdge.m_v[0],kDesignerEpsilon) )
			fDeltaToNextEdge = fDeltaToNextEdge/std::sin(std::acos(vNextEdgeDir.Dot(-vBaseEdgeDir)));
		else if( nextEdge.m_v[1].IsEquivalent(baseEdge.m_v[0],kDesignerEpsilon) )
			fDeltaToNextEdge = fDeltaToNextEdge/std::sin(std::acos(vNextEdgeDir.Dot(vBaseEdgeDir)));

		if( prevEdge.m_v[0].IsEquivalent(baseEdge.m_v[0],kDesignerEpsilon) )
			fDeltaToPrevEdge = fDeltaToPrevEdge/std::sin(std::acos(vPrevEdgeDir.Dot(-vBaseEdgeDir)));
		else if( prevEdge.m_v[1].IsEquivalent(baseEdge.m_v[0],kDesignerEpsilon) )
			fDeltaToPrevEdge = fDeltaToPrevEdge/std::sin(std::acos(vPrevEdgeDir.Dot(vBaseEdgeDir)));
	}

	m_Vertices[nVertexIndex] = nextEdge.m_v[0] + fDeltaToNextEdge*(nextEdge.m_v[1]-nextEdge.m_v[0]).GetNormalized();
	BrushVec3 newVertex = prevEdge.m_v[1] + fDeltaToPrevEdge*(prevEdge.m_v[0]-prevEdge.m_v[1]).GetNormalized();
	AddVertex(newVertex);

	return true;
}

bool CBrushRegion::GetMaximumScale( BrushFloat& fOutShortestScale ) const
{
	int iEdgeSize(m_Edges.size());
	int nShortestEdgeIndex(-1);
	BrushFloat fShortestLength = 3e10f;

	for( int i = 0; i < iEdgeSize; ++i )
	{
		BrushEdge3D edge3D = GetEdge(i);
		BrushFloat fEdgeLength = edge3D.GetLength();
		if( fEdgeLength < fShortestLength )
		{
			fShortestLength = fEdgeLength;
			nShortestEdgeIndex = i;
		}
	}

	if( nShortestEdgeIndex == -1 )
		return false;

	int nNextEdge = (nShortestEdgeIndex+1)%iEdgeSize;
	BrushEdge3D nextEdge3D = GetEdge(nNextEdge);
	int nPrevEdge = (nShortestEdgeIndex-1) < 0 ? iEdgeSize-1 : (nShortestEdgeIndex-1);
	BrushEdge3D prevEdge3D = GetEdge(nPrevEdge);

	BrushEdge prevEdge = BrushEdge(m_Plane.W2P(prevEdge3D.m_v[0]),m_Plane.W2P(prevEdge3D.m_v[1]));
	BrushEdge nextEdge = BrushEdge(m_Plane.W2P(nextEdge3D.m_v[0]),m_Plane.W2P(nextEdge3D.m_v[1]));

	Vec2 prevDir = (prevEdge.m_v[1]-prevEdge.m_v[0]).GetNormalized();
	Vec2 nextDir = (nextEdge.m_v[1]-nextEdge.m_v[0]).GetNormalized();

	BrushLine prevLine(prevEdge.m_v[0],prevEdge.m_v[1]);
	BrushLine nextLine(nextEdge.m_v[0],nextEdge.m_v[1]);

	BrushEdge3D shortestEdge3D = GetEdge(nShortestEdgeIndex);
	BrushEdge shortestEdge(m_Plane.W2P(shortestEdge3D.m_v[0]),m_Plane.W2P(shortestEdge3D.m_v[1]));
	BrushLine shortestLine(shortestEdge.m_v[0],shortestEdge.m_v[1]);
	BrushVec2 shortestEdgeDir = (shortestEdge.m_v[1]-shortestEdge.m_v[0]).GetNormalized();

	float shortestEdgeSpeed0 = std::tan(std::acos(prevDir.Dot(shortestLine.m_Normal)));
	float shortestEdgeSpeed1 = std::tan(std::acos(nextDir.Dot(-shortestLine.m_Normal)));

	float prevSpeed = 1/shortestEdgeDir.Dot(-prevLine.m_Normal);
	float nextSpeed = 1/shortestEdgeDir.Dot(nextLine.m_Normal);

	fOutShortestScale = -(fShortestLength)/(prevSpeed+nextSpeed-shortestEdgeSpeed0-shortestEdgeSpeed1)+kDesignerEpsilon*100.0f;

	return true;
}

CBrushRegion::RegionPtr CBrushRegion::RemoveInside()
{
	std::vector<RegionPtr> regionList;
	if( GetSeparatedRegions(regionList,eSR_OuterHull) && regionList.size() == 1 )
		*this = *regionList[0];
	return this;
}

BrushFloat CBrushRegion::GetNearestDistance( RegionPtr pRegion, const BrushVec3& direction ) const
{
	BrushFloat fShortestDistance(3e10f);
	for( int i = 0, iVertexSize(pRegion->m_Vertices.size()); i < iVertexSize; ++i )
	{
		BrushVec3 targetVertex(pRegion->GetVertex(i));
		BrushFloat distance(3e10f);
		if( !GetPlane().HitTest(targetVertex, targetVertex+direction, kDesignerEpsilon, &distance) )
			continue;
		if( distance < fShortestDistance )
			fShortestDistance = distance;
	}
	if( fabs(fShortestDistance) < kDesignerEpsilon )
		fShortestDistance = 0;
	return fShortestDistance;
}

void CBrushRegion::MakeThisConvex()
{
	std::vector<BrushVec3> face;
	if( !GetLinkedVertices( face, m_Vertices, m_Edges ) )
		return;

	std::vector<Vec3> regionVec3;
	std::vector<Vec3> convexHullRegionVec3;

	regionVec3.resize(face.size());
	for( int i = 0, iSize(face.size()); i < iSize; ++i )
	{
		BrushVec2 pt = m_Plane.W2P(face[i]);
		regionVec3[i] = Vec3( (float)pt.x, (float)pt.y, 0 );
	}

	ConvexHull2D( convexHullRegionVec3, regionVec3 );

	int iConvexHullSize = convexHullRegionVec3.size();
	if( iConvexHullSize < 3 )
		return;

	DeleteAllVertices_Basic();
	DeleteAllEdges_Basic();
	m_Vertices.reserve(iConvexHullSize);
	m_Edges.reserve(iConvexHullSize);

	for( int i = 0; i < iConvexHullSize; ++i )
		AddVertex_Basic(m_Plane.P2W(BrushVec2(convexHullRegionVec3[i].x,convexHullRegionVec3[i].y)));		

	for( int i = 0; i < iConvexHullSize; ++i )
	{
		int nexti = (i+1)%iConvexHullSize;
		BrushEdge3D edge(GetVertex(i),GetVertex(nexti));
		if( edge.IsPoint(kDesignerEpsilon) )
			continue;
		AddEdge_Basic(BUtil::SEdge(i,nexti));
	}

	BrushVec3 v0 = convexHullRegionVec3[0]-convexHullRegionVec3[1];
	BrushVec3 v1 = convexHullRegionVec3[2]-convexHullRegionVec3[1];
	BrushVec3 vCrossV0V1 = v0.Cross(v1);

	if( vCrossV0V1.z < 0 )
		ReverseEdges();
}

bool CBrushRegion::GetEdgeIndex( const std::vector<BUtil::SEdge>& edgeList, int nVertexIndex0, int nVertexIndex1, int& nEdgeIndex )
{
	for( int i = 0, iEdgeSize(edgeList.size()); i < iEdgeSize; ++i )
	{
		if( edgeList[i].m_i[0] == nVertexIndex0 && edgeList[i].m_i[1] == nVertexIndex1 )
		{
			nEdgeIndex = i;
			return true;
		}
	}
	return false;
}

int CBrushRegion::GetEdgeIndex( const BrushEdge3D& edge3D ) const
{
	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		BrushEdge3D edgeFromList = GetEdge(i);
		if( edgeFromList.IsEquivalent(edge3D,kDesignerEpsilon) )
			return i;
	}
	return -1;
}

bool CBrushRegion::GetEdge( int nVertexIndex0, int nVertexIndex1, BUtil::SEdge& outEdge ) const
{
	int nEdgeIndex(-1);
	if( !GetEdgeIndex( nVertexIndex0, nVertexIndex1, nEdgeIndex ) )
		return false;
	outEdge = m_Edges[nEdgeIndex];
	return true;
}

bool CBrushRegion::GetEdgesByVertexIndex( int nVertexIndex, std::vector<int>& outEdgeIndices ) const
{
	bool bAdded = false;
	for( int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i )
	{
		if( m_Edges[i].m_i[0] == nVertexIndex || m_Edges[i].m_i[1] == nVertexIndex )
		{
			bAdded = true;
			outEdgeIndices.push_back(i);
		}
	}
	return bAdded;
}

bool CBrushRegion::GetNextVertex( int nVertexIndex, BrushVec3& outVertex ) const
{
	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		if( m_Edges[i].m_i[0] == nVertexIndex )
		{
			outVertex = GetVertex(m_Edges[i].m_i[1]);
			return true;
		}
	}
	return false;
}

bool CBrushRegion::GetPrevVertex( int nVertexIndex, BrushVec3& outVertex ) const
{
	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		if( m_Edges[i].m_i[1] == nVertexIndex )
		{
			outVertex = GetVertex(m_Edges[i].m_i[0]);
			return true;
		}
	}
	return false;
}

bool CBrushRegion::AddVertex( const BrushVec3& vertex, int* pOutNewVertexIndex, BUtil::EdgeIndexSet* pOutNewEdgeIndices )
{
	bool bEquivalentExist = false;
	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{ 
		BrushEdge3D edge = GetEdge(i);
		if( edge.m_v[0].IsEquivalent(vertex,kDesignerEpsilon) )
		{
			if( pOutNewVertexIndex )
				*pOutNewVertexIndex = m_Edges[i].m_i[0];
			if( pOutNewEdgeIndices )
				pOutNewEdgeIndices->insert(i);
			bEquivalentExist = true;
		}
	}

	if( bEquivalentExist )
		return true;

	BrushFloat fNearestDistance(3e10f);
	int nNearestEdgeIndex(-1);

	int iEdgeSize(GetEdgeSize());

	for( int i = 0; i < iEdgeSize; ++i )
	{
		BrushEdge3D edge = GetEdge(i);
		BrushFloat fDistance(0);
		if( eResultDistance_Middle != BrushEdge3D::GetSquaredDistance(edge,vertex,fDistance) )
			continue;
		if( fDistance < fNearestDistance )
		{
			fNearestDistance = fDistance;
			nNearestEdgeIndex = i;
		}
	}

	if( nNearestEdgeIndex == -1 )
		return false;

	int nNewVertexIndex(m_Vertices.size());
	AddVertex_Basic(vertex);

	AddEdge_Basic(BUtil::SEdge(nNewVertexIndex,m_Edges[nNearestEdgeIndex].m_i[1]));
	m_Edges[nNearestEdgeIndex].m_i[1] = nNewVertexIndex;

	if( pOutNewVertexIndex )
		*pOutNewVertexIndex = nNewVertexIndex;

	if( pOutNewEdgeIndices )
		pOutNewEdgeIndices->insert(m_Edges.size()-1);

	return true;
}

bool CBrushRegion::Exist( const BrushVec3& vertex, const BrushFloat& kEpsilon, int* pOutIndex ) const
{
	for( int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i )
	{
		if( m_Vertices[i].IsEquivalent(vertex,kDesignerEpsilon) )
		{
			if( pOutIndex )
				*pOutIndex = i;
			return true;
		}
	}
	return false;
}

bool CBrushRegion::Exist( const BrushEdge3D& edge3D, bool bAllowReverse, int* pOutIndex ) const
{
	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		if( m_Vertices[m_Edges[i].m_i[0]].IsEquivalent(edge3D.m_v[0],kDesignerEpsilon) && m_Vertices[m_Edges[i].m_i[1]].IsEquivalent(edge3D.m_v[1],kDesignerEpsilon) )
		{
			if( pOutIndex )
				*pOutIndex = i;
			return true;
		}
		if( bAllowReverse )
		{
			if( m_Vertices[m_Edges[i].m_i[1]].IsEquivalent(edge3D.m_v[0],kDesignerEpsilon) && m_Vertices[m_Edges[i].m_i[0]].IsEquivalent(edge3D.m_v[1],kDesignerEpsilon) )
			{
				if( pOutIndex )
					*pOutIndex = i;
				return true;
			}
		}
	}
	return false;
}

bool CBrushRegion::SwapEdge( int nEdgeIndex0, int nEdgeIndex1 )
{
	if( nEdgeIndex0 < 0 || nEdgeIndex0 >= GetEdgeSize() || nEdgeIndex1 < 0 || nEdgeIndex1 >= GetEdgeSize() )
		return false;
	if( nEdgeIndex0 == nEdgeIndex1 )
		return true;

	std::swap( m_Edges[nEdgeIndex0], m_Edges[nEdgeIndex0] );

	return true;
}

void CBrushRegion::Move( const BrushVec3& offset )
{
	if( !IsValid() )
		return;
	for( int i = 0, iVertexCount(m_Vertices.size()); i < iVertexCount; ++i )
		m_Vertices[i] += offset;
	const BrushVec3& planeNormal = m_Plane.Normal();
	m_Plane.Set(planeNormal, -planeNormal.Dot(m_Vertices[0]));
	Invalidate();
}

void CBrushRegion::Transform( const Matrix34& tm )
{
	if( !IsValid() )
		return;

	int iVertexSize(m_Vertices.size());

	std::vector<BrushVec3> transformedVertices;
	transformedVertices.resize(iVertexSize);
	for( int i = 0; i < iVertexSize; ++i )
		transformedVertices[i] = tm.TransformPoint(GetVertex(i));

	if( transformedVertices.size() >= 3 )
	{
		std::vector<BrushVec3> linkedTransformedVertices;
		GetLinkedVertices(linkedTransformedVertices,transformedVertices,m_Edges);
		m_Plane = BrushPlane(linkedTransformedVertices[0],linkedTransformedVertices[1],linkedTransformedVertices[2],kDesignerEpsilon);
	}
	else
	{
		BrushVec4 vPlane(m_Plane.Normal().x,m_Plane.Normal().y,m_Plane.Normal().z,m_Plane.Distance());
		Matrix44 tm44(tm);
		tm44.Invert();
		tm44.Transpose();
		vPlane = tm44 * vPlane;
		m_Plane.Set(BrushVec3(vPlane.x,vPlane.y,vPlane.z),vPlane.w);
	}
	
	SetVertexList_Basic(transformedVertices);
}

bool CBrushRegion::IsEndPoint( const BrushVec3& position, bool* bOutFirst ) const
{
	if( !IsOpen() )
		return false;

	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		for( int k = 0; k < 2; ++k )
		{
			int index = m_Edges[i].m_i[k];
			const BrushVec3& vertex = GetVertex(index);
			if( !vertex.IsEquivalent(position,kDesignerEpsilon) )
				continue;

			std::vector<int> linkedPrevEdgeIndices;
			std::vector<int> linkedNextEdgeIndices;

			GetAdjacentPrevEdgeIndicesWithVertexIndex(index,linkedPrevEdgeIndices,m_Edges);
			GetAdjacentNextEdgeIndicesWithVertexIndex(index,linkedNextEdgeIndices,m_Edges);

			if( bOutFirst )
				*bOutFirst = linkedPrevEdgeIndices.empty() && !linkedNextEdgeIndices.empty();

			return true;
		}
	}

	return false;
}

int CBrushRegion::AddEdge( const BrushEdge3D& edge )
{
	BUtil::SEdge newEdge(-1,-1);
	for( int i = 0; i < 2; ++i )
	{
		if( !Exist(edge.m_v[i],kDesignerEpsilon,&newEdge.m_i[i]) )
		{
			newEdge.m_i[i] = m_Vertices.size();
			AddVertex_Basic(edge.m_v[i]);
		}
	}
	if( newEdge.m_i[0] == -1 && newEdge.m_i[1] == -1 )
		return -1;
	int nNewIndex = m_Edges.size();
	AddEdge_Basic(BUtil::SEdge(newEdge));
	return nNewIndex;
}

bool CBrushRegion::Concatenate( RegionPtr pRegion )
{
	if( !pRegion )
		return false;

	if( !IsOpen() || !pRegion->IsOpen() )
		return false;

	for( int i = 0, nVertexSize(pRegion->m_Vertices.size()); i < nVertexSize; ++i )
	{
		bool bThisFirst(false);
		if( IsEndPoint(pRegion->GetVertex(i),&bThisFirst) )
		{
			bool bTargetFirst(false);
			if( !pRegion->IsEndPoint(pRegion->GetVertex(i),&bTargetFirst) )
				continue;
			for( int k = 0, nEdgeSize(pRegion->m_Edges.size()); k < nEdgeSize; ++k )
			{
				if( bThisFirst == bTargetFirst )
					AddEdge( BrushEdge3D(pRegion->GetVertex(pRegion->m_Edges[k].m_i[1]),pRegion->GetVertex(pRegion->m_Edges[k].m_i[0])) );
				else
					AddEdge( BrushEdge3D(pRegion->GetVertex(pRegion->m_Edges[k].m_i[0]),pRegion->GetVertex(pRegion->m_Edges[k].m_i[1])) );
			}			
			return true;
		}
	}

	return false;
}

bool CBrushRegion::GetFirstVertex( BrushVec3& outVertex ) const
{
	if( !IsOpen() )
		return false;

	for( int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i )
	{
		std::vector<int> adjacentPrevEdgeIndices;
		std::vector<int> adjacentNextEdgeIndices;

		GetAdjacentPrevEdgeIndicesWithVertexIndex( i, adjacentPrevEdgeIndices, m_Edges );
		GetAdjacentNextEdgeIndicesWithVertexIndex( i, adjacentNextEdgeIndices, m_Edges );

		if( !adjacentNextEdgeIndices.empty() && adjacentPrevEdgeIndices.empty() )
		{
			outVertex = GetVertex(i);
			return true;
		}
	}

	return false;
}

bool CBrushRegion::GetLastVertex( BrushVec3& outVertex ) const
{
	if( !IsOpen() )
		return false;

	for( int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i )
	{
		std::vector<int> adjacentPrevEdgeIndices;
		std::vector<int> adjacentNextEdgeIndices;

		GetAdjacentPrevEdgeIndicesWithVertexIndex( i, adjacentPrevEdgeIndices, m_Edges );
		GetAdjacentNextEdgeIndicesWithVertexIndex( i, adjacentNextEdgeIndices, m_Edges );

		if( adjacentNextEdgeIndices.empty() && !adjacentPrevEdgeIndices.empty() )
		{
			outVertex = GetVertex(i);
			return true;
		}
	}

	return false;
}

void CBrushRegion::Rearrange()
{
	std::vector<BrushVec3> linkedVertices;
	if( !GetLinkedVertices(linkedVertices) )
		return;

	std::vector<BUtil::SEdge> newEdges;
	for( int i = 0, iVertexSize(linkedVertices.size()); i < iVertexSize-1; ++i )
		newEdges.push_back(BUtil::SEdge(i,i+1));

	SetVertexList_Basic(linkedVertices);
	SetEdgeList_Basic(newEdges);
}

void CBrushRegion::ClipByEdge( int nEdgeIndex, std::vector<RegionPtr>& outSplittedRegions ) const
{
	std::vector<SVertexEx> exVertices;
	int nVertexSize = m_Vertices.size();
	exVertices.reserve(nVertexSize);
	for( int i = 0; i < nVertexSize; ++i )
		exVertices.push_back(m_Vertices[i]);

	ESearchDirection directions[2] = { eSD_Previous, eSD_Next };

	for( int i = 0; i < 2; ++i )
	{
		std::vector<int> oneDirectionEdges;
		std::vector<BrushVec3> oneDirectionVertices;

		SearchLinkedEdges( nEdgeIndex, directions[i], exVertices, m_Edges, oneDirectionEdges );

		int nEdgeSize(oneDirectionEdges.size());
		if( nEdgeSize > 0 )
		{
			if( directions[i] == eSD_Next )
			{
				for( int i = 0; i < nEdgeSize; ++i )
					oneDirectionVertices.push_back(GetVertex(m_Edges[oneDirectionEdges[i]].m_i[0]));
				oneDirectionVertices.push_back(GetVertex(m_Edges[oneDirectionEdges[nEdgeSize-1]].m_i[1]));
			}
			else
			{
				for( int i = 0; i < nEdgeSize; ++i )
					oneDirectionVertices.push_back(GetVertex(m_Edges[oneDirectionEdges[i]].m_i[1]));
				oneDirectionVertices.push_back(GetVertex(m_Edges[oneDirectionEdges[nEdgeSize-1]].m_i[0]));
			}

			CBrushRegion* pRegion = new CBrushRegion(oneDirectionVertices, m_Plane, m_MaterialID, &m_TexInfo, false);
			pRegion->SetFlag(GetFlag());
			outSplittedRegions.push_back(pRegion);
		}
	}
}

void CBrushRegion::MakeRegionsFromEdgeList( const std::vector<BrushEdge3D>& edgeList, const BrushPlane& plane, std::vector<RegionPtr>& outRegions )
{
	std::set<int> usedEdgeIndexSet;

	for( int i = 0, iEdgeSize(edgeList.size()); i < iEdgeSize; ++i )
	{
		if( usedEdgeIndexSet.find(i) != usedEdgeIndexSet.end() )
			continue;

		int nLeftConnectionIndex = -1;
		int nRightConnectionIndex = -1;

		for( int a = 0; a < iEdgeSize; ++a )
		{
			if( a == i || usedEdgeIndexSet.find(a) != usedEdgeIndexSet.end() )
				continue;

			if( nLeftConnectionIndex == -1 && edgeList[i].m_v[0].IsEquivalent(edgeList[a].m_v[1],kDesignerEpsilon) )
				nLeftConnectionIndex = a;

			if( nRightConnectionIndex == -1  && edgeList[i].m_v[1].IsEquivalent(edgeList[a].m_v[0],kDesignerEpsilon) )
				nRightConnectionIndex = a;

			if( nLeftConnectionIndex != -1 && nRightConnectionIndex != -1 )
				break;
		}

		if( nLeftConnectionIndex != -1 && nRightConnectionIndex != -1 )
			continue;

		usedEdgeIndexSet.insert(i);

		RegionPtr pNewRegion = new CBrushRegion;
		pNewRegion->SetPlane(plane);
		pNewRegion->AddEdge(edgeList[i]);

		BrushEdge3D currentEdge(edgeList[i]);
		int bExistConnection = true;

		while( bExistConnection )
		{
			bExistConnection = false;

			for( int a = 0; a < iEdgeSize; ++a )
			{
				if( usedEdgeIndexSet.find(a) != usedEdgeIndexSet.end() )
					continue;
				if( nLeftConnectionIndex == -1 && nRightConnectionIndex != -1 )
				{
					if( currentEdge.m_v[1].IsEquivalent(edgeList[a].m_v[0],kDesignerEpsilon) )
						bExistConnection = true;
				}
				else if( nLeftConnectionIndex != -1 && nRightConnectionIndex == -1 )
				{
					if( currentEdge.m_v[0].IsEquivalent(edgeList[a].m_v[1],kDesignerEpsilon) )
						bExistConnection = true;
				}
				if( bExistConnection )
				{
					pNewRegion->AddEdge(edgeList[a]);
					usedEdgeIndexSet.insert(a);
					currentEdge = edgeList[a];
					break;
				}
			}
		}

		outRegions.push_back(pNewRegion);
	}
}

void CBrushRegion::SetVertex( int nIndex, const BrushVec3& vertex )
{
	if( nIndex < 0 || nIndex >= m_Vertices.size() )
		return;
	SetVertex_Basic(nIndex,vertex);	
}

bool CBrushRegion::GetVertexIndex( const BrushVec3& vertex, int& nOutIndex ) const
{
	for( int i = 0, nVertexSize(m_Vertices.size()); i < nVertexSize; ++i )
	{
		if( m_Vertices[i].IsEquivalent(vertex,kDesignerEpsilon) )
		{
			nOutIndex = i;
			return true;
		}
	}
	return false;
}

bool CBrushRegion::GetNearestVertexIndex( const BrushVec3& vertex, int& nOutIndex ) const
{
	BrushFloat fMinDistance = (BrushFloat)3e10;
	int nIndex = -1;
	for( int i = 0, nVertexSize(m_Vertices.size()); i < nVertexSize; ++i )
	{
		BrushFloat fDistance = m_Vertices[i].GetDistance(vertex);
		if( fDistance < fMinDistance )
		{
			fMinDistance = fDistance;
			nIndex = i;
		}
	}
	
	if( nIndex == -1 )
		return false;

	nOutIndex = nIndex;

	return true;
}

BrushEdge CBrushRegion::GetEdge2D(int nEdgeIndex) const
{
	return BrushEdge( m_Plane.W2P(GetVertex(m_Edges[nEdgeIndex].m_i[0])), m_Plane.W2P(GetVertex(m_Edges[nEdgeIndex].m_i[1])) );
}

BrushEdge3D CBrushRegion::GetEdge(int nEdgeIndex) const
{	
	return BrushEdge3D(GetVertex(m_Edges[nEdgeIndex].m_i[0]),GetVertex(m_Edges[nEdgeIndex].m_i[1]));
}

int CBrushRegion::GetEdgeIndex( int nVertexIndex0, int nVertexIndex1 ) const
{
	for( int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i )
	{
		if( m_Edges[i].m_i[0] == nVertexIndex0 && m_Edges[i].m_i[1] == nVertexIndex1 )
			return i;
	}
	return -1;
}

bool CBrushRegion::GetAdjacentEdgesByEdgeIndex( int nEdgeIndex, int* pOutPrevEdgeIndex, int* pOutNextEdgeIndex ) const
{
	int nPrevEdgeIndex = -1;
	int nNextEdgeIndex = -1;

	if( !GetAdjacentEdgeIndexWithEdgeIndex(nEdgeIndex,nPrevEdgeIndex,nNextEdgeIndex,m_Vertices,m_Edges) )
		return false;

	if( pOutPrevEdgeIndex )
		*pOutPrevEdgeIndex = nPrevEdgeIndex;
	if( pOutNextEdgeIndex )
		*pOutNextEdgeIndex = nNextEdgeIndex;

	return true;
}

bool CBrushRegion::GetAdjacentEdgesByVertexIndex( int nVertexIndex, int* pOutPrevEdgeIndex, int* pOutNextEdgeIndex ) const
{
	std::vector<int> prevEdgeIndices;
	GetAdjacentPrevEdgeIndicesWithVertexIndex( nVertexIndex, prevEdgeIndices, m_Edges );
	if( !prevEdgeIndices.empty() && pOutPrevEdgeIndex )
		*pOutPrevEdgeIndex = prevEdgeIndices[0];

	std::vector<int> nextEdgeIndices;
	GetAdjacentNextEdgeIndicesWithVertexIndex( nVertexIndex, nextEdgeIndices, m_Edges );
	if( !nextEdgeIndices.empty() && pOutNextEdgeIndex )
		*pOutNextEdgeIndex = nextEdgeIndices[0];

	return prevEdgeIndices.empty() || nextEdgeIndices.empty() ? false : true;
}

BrushFloat CBrushRegion::GetRadius() const
{	
	return m_BoundInfo.raidus;
}

bool CBrushRegion::IsPlaneEquivalent( RegionPtr pRegion ) const
{
	if( !pRegion )
		return false;
	return GetPlane().IsEquivalent( pRegion->GetPlane(), kDesignerEpsilon );
}

bool CBrushRegion::IsCCW() const
{
	std::vector<BrushVec3> vertices;
	if( !GetLinkedVertices(vertices) )
		return false;
	int nVertexCount = vertices.size();
	if( nVertexCount < m_Vertices.size() )
		return false;

	std::map< std::pair<BrushFloat,BrushFloat>, int > sortedPoints;
	for( int i = 0; i < nVertexCount; ++i )	
	{
		BrushVec2 point = m_Plane.W2P(vertices[i]);
		sortedPoints[std::pair<BrushFloat,BrushFloat>(point.y,point.x)] = i;
	}

	int nLeftTopIndex = sortedPoints.begin()->second;
	int nPrev = ((nLeftTopIndex-1)+nVertexCount)%nVertexCount;
	int nNext = (nLeftTopIndex+1)%nVertexCount;

	BrushVec3 v0 = (vertices[nPrev]-vertices[nLeftTopIndex]).GetNormalized();
	BrushVec3 v1 = (vertices[nNext]-vertices[nLeftTopIndex]).GetNormalized();
	BrushVec3 vCross = v0.Cross(v1);

	return vCross.Dot(m_Plane.Normal()) < 0;
}

bool CBrushRegion::IsVertexOnCrust( const BrushVec3& vertex ) const
{
	BrushVec2 point = m_Plane.W2P(vertex);

	for( int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i )
	{
		BrushEdge edge = GetEdge2D(i);
		if( edge.IsInside(point,kDesignerEpsilon) )
			return true;
	}
	return false;
}

bool CBrushRegion::HasVertex( const BrushVec3& vertex, int* pOutVertexIndex ) const
{
	for( int i = 0, iVertexCount(m_Vertices.size()); i < iVertexCount; ++i )
	{
		if( m_Vertices[i].IsEquivalent(vertex,kDesignerEpsilon) )
		{
			if( pOutVertexIndex )
				*pOutVertexIndex = i;
			return true;
		}
	}

	return false;
}

bool CBrushRegion::IsBridgeEdgeRelation( const BUtil::SEdge& pEdge0, const BUtil::SEdge& pEdge1 ) const
{
	return pEdge0.m_i[0] == pEdge1.m_i[1] && pEdge0.m_i[1] == pEdge1.m_i[0];
}

bool CBrushRegion::HasBridgeEdges() const
{
	for( int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i )
	{
		const BUtil::SEdge& edge0 = m_Edges[i];
		for( int k = i+1; k < iEdgeCount; ++k )
		{
			const BUtil::SEdge& edge1 = m_Edges[k];
			if( IsBridgeEdgeRelation(edge0,edge1) )
				return true;
		}
	}
	return false;
}

void CBrushRegion::GetBridgeEdgeSet( std::set<BUtil::SEdge>& outBridgeEdgeSet, bool bOutputBothDirection ) const
{
	for( int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i )
	{
		const BUtil::SEdge& edge0 = m_Edges[i];

		if( outBridgeEdgeSet.find(edge0) != outBridgeEdgeSet.end() )
			continue;

		for( int k = i+1; k < iEdgeCount; ++k )
		{
			const BUtil::SEdge& edge1 = m_Edges[k];
			if( IsBridgeEdgeRelation(edge0,edge1) )
			{
				outBridgeEdgeSet.insert(edge0);
				if( bOutputBothDirection )
					outBridgeEdgeSet.insert(edge1);
				break;
			}
		}
	}
}

void CBrushRegion::RemoveBridgeEdges()
{
	std::set<BUtil::SEdge> bridgeEdgeSet;
	GetBridgeEdgeSet(bridgeEdgeSet,true);

	if( bridgeEdgeSet.empty() )
		return;

	std::vector<BUtil::SEdge> newEdgeList;
	for( int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i )
	{
		const BUtil::SEdge& edge = m_Edges[i];
		if( bridgeEdgeSet.find(edge) != bridgeEdgeSet.end() )
			continue;
		newEdgeList.push_back(edge);
	}

	std::vector<BrushVec3> vList(m_Vertices);
	Optimize( vList, newEdgeList );
}

void CBrushRegion::GetBridgeEdges( std::vector<BrushEdge3D>& outBridgeEdges ) const
{
	std::set<BUtil::SEdge> bridgeEdgeSet;
	GetBridgeEdgeSet(bridgeEdgeSet,false);

	if( bridgeEdgeSet.empty() )
		return;

	std::set<BUtil::SEdge>::iterator ii = bridgeEdgeSet.begin();
	for( ; ii != bridgeEdgeSet.end(); ++ii )
		outBridgeEdges.push_back(BrushEdge3D(m_Vertices[(*ii).m_i[0]],m_Vertices[(*ii).m_i[1]]));
}

void CBrushRegion::RemoveEdge( const BrushEdge3D& edge )
{
	BrushEdge3D invEdge(edge.GetInverted());
	std::vector<BrushEdge3D> subtractedEdges;

	std::vector<BUtil::SEdge>::iterator ie = m_Edges.begin();
	for( ;ie != m_Edges.end(); )
	{
		BrushEdge3D e(m_Vertices[(*ie).m_i[0]], m_Vertices[(*ie).m_i[1]]);
		if( edge.IsEquivalent(e,kDesignerEpsilon) || invEdge.IsEquivalent(e,kDesignerEpsilon) || e.GetSubtractedEdges(edge,subtractedEdges,kDesignerEpsilon) || edge.Include(e,kDesignerEpsilon) )
			ie = m_Edges.erase(ie);
		else
			++ie;
	}

	for( int i = 0, iSize(subtractedEdges.size()); i < iSize; ++i )
	{
		int nEdgeIdx0 = AddVertex( m_Vertices, subtractedEdges[i].m_v[0] );
		int nEdgeIdx1 = AddVertex( m_Vertices, subtractedEdges[i].m_v[1] );
		m_Edges.push_back(BUtil::SEdge(nEdgeIdx0,nEdgeIdx1));
	}

	Optimize();
}

void CBrushRegion::RemoveEdge( int nEdgeIndex )
{
	if( nEdgeIndex < 0 || nEdgeIndex >= m_Edges.size()-1 )
	{
		DESIGNER_ASSERT(0);
		return;
	}
	m_Edges.erase(m_Edges.begin()+nEdgeIndex);
	Optimize();
}

bool CBrushRegion::FindFirstEdgeIndex( const std::set<int>& edgeSet, int& outEdgeIndex ) const
{
	int iEdgeSize(m_Edges.size());
	std::set<int>::iterator ii = edgeSet.begin();

	for( ; ii != edgeSet.end(); ++ii )
	{
		bool bFirstEdge = true;
		for( int k = 0; k < iEdgeSize; ++k )
		{
			if( *ii == k )
				continue;
			if( m_Edges[*ii].m_i[0] == m_Edges[k].m_i[1] )
			{
				bFirstEdge = false;
				break;
			}
		}
		if( bFirstEdge )
		{
			outEdgeIndex = *ii;
			return true;
		}
	}

	return false;
}

void CBrushRegion::GetUnconnectedRegions( std::vector<RegionPtr>& outRegions )
{
	if( !IsOpen() )
	{
		outRegions.push_back(this);
		return;
	}

	int iEdgeSize(m_Edges.size());
	int nEdgeIndex = 0;

	std::set<int> leftovers;
	for( int i = 0; i < iEdgeSize; ++i )
		leftovers.insert(i);

	if( !FindFirstEdgeIndex(leftovers,nEdgeIndex) )
	{
		DESIGNER_ASSERT(0);
		return;
	}

	int nStartEdgeIndex = nEdgeIndex;
	std::vector<BrushVec3> vList;
	vList.push_back(m_Vertices[m_Edges[nEdgeIndex].m_i[0]]);
	vList.push_back(m_Vertices[m_Edges[nEdgeIndex].m_i[1]]);

	DESIGNER_ASSERT( leftovers.find(nEdgeIndex) != leftovers.end() );
	while( !leftovers.empty() )
	{
		std::vector<int> nextEdgeIndices;
		leftovers.erase(nEdgeIndex);
		if( !GetAdjacentNextEdgeIndicesWithVertexIndex(m_Edges[nEdgeIndex].m_i[1], nextEdgeIndices, m_Edges) || nStartEdgeIndex == nextEdgeIndices[0] )
		{
			bool bClosed = !nextEdgeIndices.empty() && nStartEdgeIndex == m_Edges[nextEdgeIndices[0]].m_i[0];
			RegionPtr pRegion = new CBrushRegion( vList, GetPlane(), m_MaterialID, &m_TexInfo, bClosed );
			pRegion->SetFlag(GetFlag());
			outRegions.push_back(pRegion);
			if( !leftovers.empty() )
			{
				if( !FindFirstEdgeIndex(leftovers,nEdgeIndex) )
				{
					DESIGNER_ASSERT(0);
					break;
				}
				nStartEdgeIndex = nEdgeIndex;
				vList.clear();
				vList.push_back(m_Vertices[m_Edges[nEdgeIndex].m_i[0]]);
				vList.push_back(m_Vertices[m_Edges[nEdgeIndex].m_i[1]]);
			}
		}
		else
		{
			nEdgeIndex = nextEdgeIndices[0];
			DESIGNER_ASSERT( leftovers.find(nEdgeIndex) != leftovers.end() );
			vList.push_back(m_Vertices[m_Edges[nEdgeIndex].m_i[1]]);
		}
	}
}

CBrushRegion::EResultExtract CBrushRegion::ExtractVertexList( int nStartEdgeIdx, int nEndEdgeIdx, std::vector<BrushVec3>& outVertexList, std::vector<int>* pOutVertexIndices ) const
{
	int nCounter = 0;
	int outEdgeIndices[2];
	int nCurrentEdgeIdx = nStartEdgeIdx;
	outVertexList.push_back(m_Vertices[m_Edges[nCurrentEdgeIdx].m_i[0]]);
	if( pOutVertexIndices )
		pOutVertexIndices->push_back(m_Edges[nCurrentEdgeIdx].m_i[0]);

	while( GetAdjacentEdgeIndexWithEdgeIndex( nCurrentEdgeIdx, outEdgeIndices[eSD_Previous], outEdgeIndices[eSD_Next], m_Vertices, m_Edges ) && outEdgeIndices[eSD_Next] != -1 )
	{
		nCurrentEdgeIdx = outEdgeIndices[eSD_Next];

		outVertexList.push_back(m_Vertices[m_Edges[nCurrentEdgeIdx].m_i[0]]);
		if( pOutVertexIndices )
			(*pOutVertexIndices).push_back(m_Edges[nCurrentEdgeIdx].m_i[0]);

		if( nCurrentEdgeIdx == nEndEdgeIdx)
			return eRE_EndAtEndVtx;
		else if( nCurrentEdgeIdx == nStartEdgeIdx )
			return eRE_EndAtStartVtx;

		if( ++nCounter > m_Edges.size() )
		{
			DESIGNER_ASSERT( 0 && "Can't find the next edge" );
			break;
		}
	}

	return eRE_Fail;
}

bool CBrushRegion::ClipByPlane( const BrushPlane& clipPlane, std::vector<RegionPtr>& pOutFrontRegions, std::vector<RegionPtr>& pOutBackRegions, std::vector<BrushEdge3D>* pOutBoundaryEdges ) const
{
	if( GetPlane().IsEquivalent(clipPlane,kDesignerEpsilon) )
	{
		pOutFrontRegions.push_back(Clone());
		return false;
	}

	if( GetPlane().GetInverted().IsEquivalent(clipPlane,kDesignerEpsilon) )
	{
		pOutBackRegions.push_back(Clone());
		return false;
	}

	RegionPtr pFrontPart = new CBrushRegion;
	RegionPtr pBackPart = new CBrushRegion;

	if( IsOpen() )
	{
		pFrontPart->SetPlane(GetPlane());
		pBackPart->SetPlane(GetPlane());

		for( int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i )
		{
			BrushEdge3D e = GetEdge(i);
			BrushFloat d0 = clipPlane.Distance(e.m_v[0]);
			BrushFloat d1 = clipPlane.Distance(e.m_v[1]);

			if( d0 > kDesignerEpsilon && d1 > kDesignerEpsilon )
			{
				pFrontPart->AddEdge(e);
			}
			else if( d0 < -kDesignerEpsilon && d1 < -kDesignerEpsilon )
			{
				pBackPart->AddEdge(e);
			}
			else if( (d0 > kDesignerEpsilon && d1 < -kDesignerEpsilon) || (d1 > kDesignerEpsilon && d0 < -kDesignerEpsilon) ) 
			{
				BrushVec3 vDir = e.m_v[1] - e.m_v[0]; 
				BrushVec3 vHitPos;
				if( clipPlane.HitTest(e.m_v[0], vDir, kDesignerEpsilon, NULL, &vHitPos) )
				{
					if( d0 > kDesignerEpsilon )
					{
						pFrontPart->AddEdge(BrushEdge3D(e.m_v[0],vHitPos));
						pBackPart->AddEdge(BrushEdge3D(vHitPos,e.m_v[1]));
					}
					else
					{
						pBackPart->AddEdge(BrushEdge3D(e.m_v[0],vHitPos));
						pFrontPart->AddEdge(BrushEdge3D(vHitPos,e.m_v[1]));
					}
				}
			}
		}

		if( pFrontPart->IsValid() )
			pOutFrontRegions.push_back(pFrontPart);
		if( pBackPart->IsValid() )
			pOutBackRegions.push_back(pBackPart);

		return true;
	}

	CBrushRegion::RegionPtr pRegions[] = { pFrontPart, pBackPart };
	std::vector<RegionPtr>* pOutRegions[] = { &pOutFrontRegions, &pOutBackRegions };

	pFrontPart->m_Plane = pBackPart->m_Plane = m_Plane;
	pFrontPart->m_Flag = pBackPart->m_Flag = m_Flag;
	pFrontPart->m_TexInfo = pBackPart->m_TexInfo = m_TexInfo;
	pFrontPart->m_MaterialID = pBackPart->m_MaterialID = m_MaterialID;

	std::vector<BrushVec3> boundaryVertices;
	std::vector<BrushEdge3D> boundaryEdges;

	for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
	{
		BrushEdge3D edge = GetEdge(i);
		BrushFloat d0 = clipPlane.Distance(edge.m_v[0]);
		BrushFloat d1 = clipPlane.Distance(edge.m_v[1]);
		if( std::abs(d0) < kDesignerEpsilon )
			d0 = 0;
		if( std::abs(d1) < kDesignerEpsilon )
			d1 = 0;

		if( d0 < 0 && d1 > 0 || d0 > 0 && d1 < 0 )
		{
			BrushVec3 vHitPos;

			int nEdgeIndex = 0;
			if( pOutBoundaryEdges && BUtil::DoesEquivalentExist(*pOutBoundaryEdges,edge,&nEdgeIndex) )
			{
				if( (*pOutBoundaryEdges)[nEdgeIndex].m_v[0].IsEquivalent(edge.m_v[1]) )
					edge = (*pOutBoundaryEdges)[nEdgeIndex].GetInverted();
				else
					edge = (*pOutBoundaryEdges)[nEdgeIndex];
			}

			if( clipPlane.HitTest(edge.m_v[0], edge.m_v[1], kDesignerEpsilon, NULL, &vHitPos) )
			{
				if( d0 > 0 )
				{
					pFrontPart->AddEdge(BrushEdge3D(edge.m_v[0],vHitPos));
					pBackPart->AddEdge(BrushEdge3D(vHitPos,edge.m_v[1]));
				}
				else
				{
					pFrontPart->AddEdge(BrushEdge3D(vHitPos,edge.m_v[1]));
					pBackPart->AddEdge(BrushEdge3D(edge.m_v[0],vHitPos));
				}

				if( !BUtil::DoesEquivalentExist(boundaryVertices,vHitPos) )
					boundaryVertices.push_back(vHitPos);
			}
		}
		else if( d0 > 0 && d1 >= 0 || d0 >= 0 && d1 > 0 )
		{
			pFrontPart->AddEdge(edge);
		}
		else if( d0 < 0 && d1 <= 0 || d0 <= 0 && d1 < 0 )
		{
			pBackPart->AddEdge(edge);
		}
	}

	if( pFrontPart->IsOpen() || pBackPart->IsOpen() )
	{
		for( int i = 0, iEdgeSize(m_Edges.size()); i < iEdgeSize; ++i )
		{
			BrushEdge3D edge = GetEdge(i);
			BrushFloat d0 = clipPlane.Distance(edge.m_v[0]);
			BrushFloat d1 = clipPlane.Distance(edge.m_v[1]);
			if( std::abs(d0) < kDesignerEpsilon )
				d0 = 0;
			if( std::abs(d1) < kDesignerEpsilon )
				d1 = 0;
			if( d0 == 0 && d1 == 0 )
			{
				if( !BUtil::DoesEquivalentExist(boundaryEdges,edge) )
					boundaryEdges.push_back(edge);
				for( int k = 0; k < 2; ++k )
				{
					if( !pRegions[k]->IsValid() || !pRegions[k]->IsOpen() )
						continue;

					bool bConnect0to0 = pRegions[k]->QueryEdges(edge.m_v[0],0);
					bool bConnect1to1 = pRegions[k]->QueryEdges(edge.m_v[1],1);

					if( bConnect0to0 || bConnect1to1 )
						pRegions[k]->AddEdge(edge.GetInverted());
					else
						pRegions[k]->AddEdge(edge);
				}
				if( pOutBoundaryEdges )
					(*pOutBoundaryEdges).push_back(edge);
			}
			else if( d0 == 0 )
			{
				if( !BUtil::DoesEquivalentExist(boundaryVertices,edge.m_v[0]) )
					boundaryVertices.push_back(edge.m_v[0]);
			}
			else if( d1 == 0 )
			{
				if( !BUtil::DoesEquivalentExist(boundaryVertices,edge.m_v[1]) )
					boundaryVertices.push_back(edge.m_v[1]);
			}
		}
	}

	if( pFrontPart->IsOpen() || pBackPart->IsOpen() )
	{
		for( int i = 0, iBoundaryVertexCount(boundaryVertices.size()); i < iBoundaryVertexCount; ++i )
		{
			BrushEdge3D vHitEdge(boundaryVertices[i],boundaryVertices[i]);
			if( !BUtil::DoesEquivalentExist(boundaryEdges,vHitEdge) )
				boundaryEdges.push_back(vHitEdge);
		}

		if( boundaryEdges.size() == 1 && !boundaryEdges[0].m_v[0].IsEquivalent(boundaryEdges[0].m_v[1],kDesignerEpsilon) )
		{
			boundaryEdges.push_back(BrushEdge3D(boundaryEdges[0].m_v[1],boundaryEdges[0].m_v[1]));
			boundaryEdges[0].m_v[1] = boundaryEdges[0].m_v[0];
		}

		BrushLine3D intersectionLine;
		if( GetPlane().IntersectionLine(clipPlane,intersectionLine) )
		{
			std::vector<BrushEdge3D> sortedGapEdges;
			if( SortEdgesAlongCrossLine(boundaryEdges,intersectionLine,sortedGapEdges) )
			{
				for( int i = 0, iSortedGapEdgeCount(sortedGapEdges.size()); i < iSortedGapEdgeCount; ++i )
				{
					for( int k = 0; k < 2; ++k )
					{
						if( !pRegions[k]->IsOpen() )
							continue;

						bool bConnect0to0 = pRegions[k]->QueryEdges(sortedGapEdges[i].m_v[0],0);
						bool bConnect1to1 = pRegions[k]->QueryEdges(sortedGapEdges[i].m_v[1],1);
						bool bConnect0to1 = pRegions[k]->QueryEdges(sortedGapEdges[i].m_v[0],1);
						bool bConnect1to0 = pRegions[k]->QueryEdges(sortedGapEdges[i].m_v[1],0);

						if( bConnect0to0 && bConnect1to1 && bConnect0to1 && bConnect1to0 )
						{
							for( int a = 0; a < 2; ++a )
							{
								BrushEdge3D candidateEdge = a==0 ? sortedGapEdges[i] : sortedGapEdges[i].GetInverted();
								int nEdgeIndex = pRegions[k]->AddEdge(candidateEdge);
								int nNextIndex = -1;
								bool bLoopExist = false;
								int nCount = 0;
								while( pRegions[k]->GetAdjacentEdgesByEdgeIndex(nEdgeIndex, NULL, &nNextIndex) && nNextIndex != -1 )
								{
									BrushEdge3D nextEdge = pRegions[k]->GetEdge(nNextIndex);
									if( nextEdge.m_v[1].IsEquivalent(sortedGapEdges[i].m_v[0],kDesignerEpsilon) )
									{
										bLoopExist = true;
										break;
									}
									if( ++nCount > 10000 )
									{
										DESIGNER_ASSERT(0 && "Falled Into an infinite loop");
										break;
									}
									nEdgeIndex = nNextIndex;
								}
								if( bLoopExist )
									break;
								else
									pRegions[k]->RemoveEdge(candidateEdge);
							}
						}
						else if( bConnect0to0 && bConnect1to1 )
						{
							pRegions[k]->AddEdge(sortedGapEdges[i].GetInverted());
						}
						else if( bConnect0to1 && bConnect1to0 )
						{
							pRegions[k]->AddEdge(sortedGapEdges[i]);
						}
						if( pOutBoundaryEdges && !BUtil::DoesEquivalentExist(*pOutBoundaryEdges,sortedGapEdges[i]) )
							pOutBoundaryEdges->push_back(sortedGapEdges[i]);
					}
				}
			}
		}
	}

	if( !pFrontPart->IsValid() && !pBackPart->IsValid() )
	{
		if( GetPlane().IsSameFacing(clipPlane) )
			pOutFrontRegions.push_back(Clone());
		else
			pOutBackRegions.push_back(Clone());
		return true;
	}

	for( int k = 0; k < 2; ++k )
	{
		pOutRegions[k]->clear();

		if( pRegions[k]->IsValid() && pRegions[k]->GetEdgeSize() > 2 && pRegions[k]->GetVertexListSize() > 2 )
		{
			pRegions[k]->Optimize();
			if( pRegions[k]->IsOpen() || !pRegions[k]->HasHoles() )
				pRegions[k]->GetSeparatedRegions(*pOutRegions[k],eSR_OuterHull);
			else
				pOutRegions[k]->push_back(pRegions[k]);
			DESIGNER_ASSERT(!pOutRegions[k]->empty());
			if( pOutRegions[k]->empty() )
				return false;
		}
	}

	return !pOutFrontRegions.empty() || !pOutBackRegions.empty() ? true : false;
}

bool CBrushRegion::SortVerticesAlongCrossLine( std::vector<BrushVec3>& vList, const BrushLine3D& crossLine3D, std::vector<BrushEdge3D>& outGapEdges ) const
{
	int nIndex = 0;
	if( std::abs(crossLine3D.m_Dir.y) > std::abs(crossLine3D.m_Dir.x) && std::abs(crossLine3D.m_Dir.y) > std::abs(crossLine3D.m_Dir.z) )
		nIndex = 1;
	else if( std::abs(crossLine3D.m_Dir.z) > std::abs(crossLine3D.m_Dir.x) && std::abs(crossLine3D.m_Dir.z) > std::abs(crossLine3D.m_Dir.y) )
		nIndex = 2;

	if( crossLine3D.m_Dir[nIndex] == 0 )
		return false;

	std::map<BrushFloat,BrushVec3> sortedIntersections;

	for( int k = 0, iIntersectionSize(vList.size()); k < iIntersectionSize; ++k )
	{
		BrushFloat t = (vList[k][nIndex]-crossLine3D.m_Pivot[nIndex])/crossLine3D.m_Dir[nIndex];
		sortedIntersections[t] = vList[k];
	}

	bool bAdded = false;

	std::map<BrushFloat,BrushVec3>::iterator ii = sortedIntersections.begin();
	for( ; ii != sortedIntersections.end(); ++ii )
	{
		BrushVec3 vBegin = ii->second;
		++ii;
		if( ii == sortedIntersections.end() )
			break;

		BrushVec3 vEnd = ii->second;
		if( Include((vBegin+vEnd)*0.5f) )
		{
			bAdded = true;
			outGapEdges.push_back(BrushEdge3D(vBegin,vEnd));
		}
	}

	return bAdded;
}

bool CBrushRegion::SortEdgesAlongCrossLine( std::vector<BrushEdge3D>& edgeList, const BrushLine3D& crossLine3D, std::vector<BrushEdge3D>& outGapEdges ) const
{
	int nIndex = 0;
	if( std::abs(crossLine3D.m_Dir.y) > std::abs(crossLine3D.m_Dir.x) && std::abs(crossLine3D.m_Dir.y) > std::abs(crossLine3D.m_Dir.z) )
		nIndex = 1;
	else if( std::abs(crossLine3D.m_Dir.z) > std::abs(crossLine3D.m_Dir.x) && std::abs(crossLine3D.m_Dir.z) > std::abs(crossLine3D.m_Dir.y) )
		nIndex = 2;

	if( crossLine3D.m_Dir[nIndex] == 0 )
		return false;

	std::map<BrushFloat,BrushEdge3D> sortedIntersections;

	for( int k = 0, iIntersectionSize(edgeList.size()); k < iIntersectionSize; ++k )
	{
		const BrushVec3& v0 = edgeList[k].m_v[0];
		const BrushVec3& v1 = edgeList[k].m_v[1];
		BrushFloat t0 = (v0[nIndex]-crossLine3D.m_Pivot[nIndex])/crossLine3D.m_Dir[nIndex];
		BrushFloat t1 = (v1[nIndex]-crossLine3D.m_Pivot[nIndex])/crossLine3D.m_Dir[nIndex];

		if( t0 <= t1 )
			sortedIntersections[t0] = BrushEdge3D(v0,v1);
		else
			sortedIntersections[t1] = BrushEdge3D(v1,v0);
	}

	bool bAdded = false;

	std::map<BrushFloat,BrushEdge3D>::iterator ii = sortedIntersections.begin();
	for( ; ii != sortedIntersections.end(); )
	{
		const BrushEdge3D& vBegin = ii->second;
		++ii;
		if( ii == sortedIntersections.end() )
			break;
		const BrushEdge3D& vEnd = ii->second;

		if( Include((vBegin.m_v[1]+vEnd.m_v[0])*0.5f) )
		{
			bAdded = true;
			outGapEdges.push_back(BrushEdge3D(vBegin.m_v[1],vEnd.m_v[0]));
		}
	}

	return bAdded;
}

CBrushRegion::RegionPtr CBrushRegion::Mirror( const BrushPlane& mirrorPlane )
{
	if( m_Vertices.empty() )
		return NULL;

	for( int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i )
		m_Vertices[i] = mirrorPlane.MirrorVertex(m_Vertices[i]);

	ReverseEdges();

	AddFlags(eRF_Mirrored);
	m_Plane = mirrorPlane.MirrorPlane(m_Plane);

	return this;
}

bool CBrushRegion::GetComputedPlane( BrushPlane& outPlane ) const
{
	std::vector<BrushVec3> vLinkedVertices;
	if( m_Vertices.empty() || !GetLinkedVertices(vLinkedVertices) )
	{
		return false;
	}

	return BUtil::ComputePlane( vLinkedVertices, outPlane );
}

CBrushRegion::RegionPtr Convert2ViewRegion( IDisplayViewport* pView, const BrushMatrix34& worldTM, const CBrushRegion* pRegion, CBrushRegion::RegionPtr* pOutRegion )
{
	BrushMatrix34 vInvMatrix = worldTM.GetInverted();

	const BrushVec3 vCameraPos = vInvMatrix.TransformPoint(ToBrushVec3(pView->GetViewTM().GetTranslation()));
	const BrushVec3 vDir = vInvMatrix.TransformVector(ToBrushVec3(pView->GetViewTM().GetColumn1()));

	std::set<int> verticesBehindCamera;	
	for( int i = 0, iVertexCount(pRegion->GetVertexListSize()); i < iVertexCount; ++i )
	{
		if( vDir.Dot((pRegion->GetVertex(i)-vCameraPos).GetNormalized()) <= 0 )
			verticesBehindCamera.insert(i);
	}

	if( verticesBehindCamera.size() == pRegion->GetVertexListSize() )
		return NULL;

	CBrushRegion::RegionPtr pViewRegion = new CBrushRegion(*pRegion);
	pViewRegion->Transform(worldTM);

	if( !verticesBehindCamera.empty() )
	{
		const CCamera& camera = GetIEditor()->GetRenderer()->GetCamera();
		const Plane* pNearPlane = camera.GetFrustumPlane(FR_PLANE_NEAR);
		BrushPlane nearPlane(pNearPlane->n, pNearPlane->d);

		std::vector<CBrushRegion::RegionPtr> frontRegions;
		std::vector<CBrushRegion::RegionPtr> backRegions;
		pViewRegion->ClipByPlane(nearPlane,frontRegions,backRegions);

		int nBackRegionCount = backRegions.size();
		if( nBackRegionCount == 1 )
		{
			pViewRegion = backRegions[0];
		}
		else if( nBackRegionCount > 1 )
		{
			pViewRegion->Clear();
			for( int i = 0; i < nBackRegionCount; ++i )
				pViewRegion->Union(backRegions[i]);
		}

		for( int i = 0, iVertexCount(pViewRegion->GetVertexListSize()); i < iVertexCount; ++i )
		{
			const BrushVec3& v = pViewRegion->GetVertex(i);
			BrushFloat fDistance = nearPlane.Distance(v);
			if( fDistance > -kDesignerEpsilon )
				pViewRegion->SetVertex(i,v-nearPlane.Normal()*(BrushFloat)0.001);
		}

		if( pOutRegion )
		{
			*pOutRegion = pViewRegion->Clone();
			(*pOutRegion)->Transform(worldTM.GetInverted());
		}
	}
	else
	{
		if( pOutRegion )
			*pOutRegion = NULL;
	}

	for( int i = 0, iVertexCount(pViewRegion->GetVertexListSize()); i < iVertexCount; ++i )
	{
		CPoint pt = pView->WorldToView(pViewRegion->GetVertex(i));
		pViewRegion->SetVertex(i,BrushVec3((BrushFloat)pt.x,(BrushFloat)pt.y,0));
	}

	BrushPlane plane(BrushVec3(0,0,1),0);
	pViewRegion->GetComputedPlane(plane);
	pViewRegion->SetPlane(plane);
	
	return pViewRegion;
}

bool CBrushRegion::InRectangle( IDisplayViewport* pView, const BrushMatrix34& worldTM, CBrushRegion::RegionPtr pRectRegion, bool bExcludeBackFace ) const
{
	CBrushRegion::RegionPtr pViewRegion = Convert2ViewRegion(pView, worldTM, this, NULL);
	if( !pViewRegion )
		return false;

	if( bExcludeBackFace && pViewRegion->GetPlane().Normal().z > 0 )
		return false;

	if( !pViewRegion->GetBoundBox().IsIntersectBox(pRectRegion->GetBoundBox()) )
		return false;

	if( !pViewRegion->GetPlane().IsEquivalent(pRectRegion->GetPlane(),kDesignerEpsilon) )
		pRectRegion->Flip();

	if( CBrushRegion::HasIntersection(pViewRegion,pRectRegion) == BUtil::eIT_Intersection )
		return true;

	return false;
}

void CBrushRegion::QueryIntersectionEdgesWith2DRect( IDisplayViewport* pView, const BrushMatrix34& worldTM, CBrushRegion::RegionPtr pRectRegion, bool bExcludeBackFace, BUtil::EdgeQueryResult& outIntersectionEdges )
{
	CBrushRegion::RegionPtr pRegion = NULL;
	CBrushRegion::RegionPtr pViewRegion = Convert2ViewRegion(pView, worldTM, this, &pRegion);
	if( !pViewRegion )
		return;

	if( bExcludeBackFace && pViewRegion->GetPlane().Normal().z > 0 )
		return;

	const AABB& viewRegionAABB = pViewRegion->GetBoundBox();
	const AABB& rectAABB = pRectRegion->GetBoundBox();
	if( !viewRegionAABB.IsIntersectBox(rectAABB) )
		return;

	for( int i = 0, iEdgeCount(pViewRegion->GetEdgeSize()); i < iEdgeCount; ++i )
	{
		BrushEdge3D ve = pViewRegion->GetEdge(i);
		CBrushBSPTree2D::SOutputEdges partitions;
		pRectRegion->GetBSPTree()->GetPartitions(ve,partitions);

		if( partitions.negList.size() == 1 )
		{
			BrushVec3 vCenter = (partitions.negList[0].m_v[0]+partitions.negList[0].m_v[1])*(BrushFloat)0.5;
			BrushFloat edgeDistance = ve.m_v[1].GetDistance(ve.m_v[0]);
			if( edgeDistance > 0 )
			{
				BrushFloat t = vCenter.GetDistance(ve.m_v[0])/edgeDistance;
				BrushEdge3D e;
				if( pRegion )
				{
					e = pRegion->GetEdge(i);
					int nEdgeIndex = 0;
					if( IsEdgeOnCrust(e,&nEdgeIndex) )
						e = GetEdge(nEdgeIndex);
				}
				else
				{
					e = GetEdge(i);
				}
				outIntersectionEdges.push_back(std::pair<BrushEdge3D,BrushVec3>(e,e.m_v[0]+(e.m_v[1]-e.m_v[0])*t));
			}
		}
	}
}

CBrushRegion::RegionPtr CBrushRegion::MakeRegionFromRectangle( const CRect& rectangle )
{
	std::vector<BrushVec3> vRectangleList;
	vRectangleList.push_back( BrushVec3(rectangle.right,rectangle.top,0) );
	vRectangleList.push_back( BrushVec3(rectangle.right,rectangle.bottom,0) );
	vRectangleList.push_back( BrushVec3(rectangle.left,rectangle.bottom,0) );
	vRectangleList.push_back( BrushVec3(rectangle.left,rectangle.top,0) );
	return new CBrushRegion(vRectangleList);
}

void CBrushRegion::CopyRegions( std::vector<CBrushRegion::RegionPtr>& sourceRegions, std::vector<CBrushRegion::RegionPtr>& destRegions )
{
	for( int i = 0, iSourceRegionCount(sourceRegions.size()); i < iSourceRegionCount; ++i )
		destRegions.push_back(sourceRegions[i]->Clone());
}

void CBrushRegion::NullBspTree() const
{
	if( m_pBSPTree )
		m_pBSPTree->Release();
	m_pBSPTree = NULL;
}

void CBrushRegion::NullConvexes() const
{
	if( m_pConvexes )
		m_pConvexes->Release();
	m_pConvexes = NULL;
}

void CBrushRegion::NullTriangules() const
{
	if( m_pTriangles )
		m_pTriangles->Release();
	m_pTriangles = NULL;
}

CBrushConvexes* CBrushRegion::GetConvexes()
{
	if( !m_pConvexes )
	{
		m_pConvexes = new CBrushConvexes;
		m_pConvexes->AddRef();
		CBrushDesignerPolygonDecomposer decomposer;	
		decomposer.CreateConvexes(this,*m_pConvexes);
	}
	return m_pConvexes;
}

CBrushTriangles* CBrushRegion::GetTriangles( bool bGenerateBackFaces )
{
	if( m_pTriangles && m_pTriangles->HasBackFaces() != bGenerateBackFaces )
		NullTriangules();

	if( !m_pTriangles )
	{
		m_pTriangles = new CBrushTriangles;
		m_pTriangles->AddRef();
		m_pTriangles->EnableBackFaces(bGenerateBackFaces);
		BUtil::CreateMeshFacesFromRegion(this,m_pTriangles->GetMesh(),bGenerateBackFaces);
	}

	return m_pTriangles;
}