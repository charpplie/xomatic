#include "StdAfx.h"
#include "BrushDesignerHalfEdgeMesh.h"
#include "BrushConvexes.h"
#include "BrushDesigner.h"
#include "BrushDesignerEdgesSharpnessManager.h"

const HE_Edge& CBrushDesignerHalfEdgeMesh::GetPrevEdge( const HE_Edge& e ) const
{
	const HE_Edge* e_variable = &e;
	while( m_Edges[e_variable->next_edge].vertex != e.vertex )
		e_variable = &m_Edges[e_variable->next_edge];
	return *e_variable;
}

void CBrushDesignerHalfEdgeMesh::ConstructMesh( CBrushDesigner* pDesigner )
{
	Clear();

	CBrushDesignerEdgeSharpnessManager* pEdgeSharpnessMgr = pDesigner->GetEdgeSharpnessMgr();
	DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);

	for( BUtil::ShelfID id = 0; id < BUtil::kMaxShelfCount; ++id )
	{
		pDesigner->SetShelf(id);
		for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
		{
			CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
			if( pRegion->IsOpen() )
				continue;
			CBrushConvexes* pConvexList = pRegion->GetConvexes();
			SolveTJunction(pDesigner,pRegion,pConvexList);
			for( int k = 0, iConvexCount(pConvexList->GetConvexCount()); k < iConvexCount; ++k )
				AddConvex(pRegion,pConvexList->GetConvex(k));
		}
	}

	for( int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i )
	{
		BrushEdge3D e(GetPos(m_Edges[i]),GetPos(m_Edges[m_Edges[i].next_edge]));
		m_Edges[i].sharpness = pEdgeSharpnessMgr->FindSharpness(e);
		m_Edges[i].irregular = m_Edges[i].sharpness > kDesignerEpsilon;
	}

	FindEachPairEdge();
	ConstructEdgeSharpnessTable(pDesigner);
}

void CBrushDesignerHalfEdgeMesh::AddConvex( CBrushRegion::RegionPtr pRegion, const std::vector<BrushVec3>& vConvex )
{	
	int nStartEdgeIndex = (int)m_Edges.size();
	int nFaceIndex = (int)m_Faces.size();

	for( int i = 0, iVListCount(vConvex.size()); i < iVListCount; ++i )
	{
		int nVertexIndex = (int)m_Vertices.size();
		int nEdgeIndex = (int)m_Edges.size();

		HE_Vertex v;
		v.pos_index = AddPos(vConvex[i]);
		v.edge = nEdgeIndex;
		m_Vertices.push_back(v);

		HE_Edge e;
		e.next_edge = (i == iVListCount-1) ? nStartEdgeIndex : (nEdgeIndex+1);
		e.face = nFaceIndex;
		e.vertex = nVertexIndex;
		m_Edges.push_back(e);
	}

	BUtil::HE_Face f;
	f.edge = nStartEdgeIndex;
	f.pOriginRegion = pRegion;
	m_Faces.push_back(f);
}

int CBrushDesignerHalfEdgeMesh::AddPos( const BrushVec3& vPos )
{
	for( int i = 0, iCount(m_Positions.size()); i < iCount; ++i )
	{
		if( m_Positions[i].pos.IsEquivalent(vPos,kDesignerEpsilon) )
			return i;
	}
	m_Positions.push_back(vPos);
	return m_Positions.size()-1;
}

void CBrushDesignerHalfEdgeMesh::FindEachPairEdge()
{
	int nEdgeCount(m_Edges.size());

	for( int i = 0; i < nEdgeCount; ++i )
	{
		if( m_Edges[i].pair_edge != -1 )
			continue;
		std::pair<int,int> e0(GetVertex(m_Edges[i].vertex).pos_index, GetVertex(m_Edges[m_Edges[i].next_edge].vertex).pos_index);
		for( int k = 0; k < nEdgeCount; ++k )
		{
			if( i == k || m_Edges[k].pair_edge != -1 || m_Edges[i].face == m_Edges[k].face )
				continue;
			std::pair<int,int> e1(GetVertex(m_Edges[k].vertex).pos_index, GetVertex(m_Edges[m_Edges[k].next_edge].vertex).pos_index);
			if( e0.first == e1.second && e0.second == e1.first )
			{
				m_Edges[i].pair_edge = k;
				m_Edges[k].pair_edge = i;
				break;
			}
		}
	}
}

void CBrushDesignerHalfEdgeMesh::ConstructEdgeSharpnessTable( CBrushDesigner* pDesigner )
{
	CBrushDesignerEdgeSharpnessManager* pSharpnessMgr = pDesigner->GetEdgeSharpnessMgr();
	for( int i = 0, iEdgeCount(m_Edges.size()); i < iEdgeCount; ++i )
	{
		BrushEdge3D e = GetRealEdge(m_Edges[i]);
		m_Edges[i].sharpness = pSharpnessMgr->FindSharpness(e);
	}

	for( int i = 0, iVertexCount(m_Vertices.size()); i < iVertexCount; ++i )
	{
		HE_Vertex& v = m_Vertices[i];
		HE_Edge& e = m_Edges[m_Vertices[i].edge];
		if( e.sharpness > 0 )
			m_Positions[v.pos_index].edges.push_back(m_Vertices[i].edge);
	}
}

BrushVec3 CBrushDesignerHalfEdgeMesh::GetFaceAveragePos( const BUtil::HE_Face& f ) const
{
	BrushVec3 vSum(0,0,0);
	int nStartingEdge = f.edge;
	int nEdge = nStartingEdge;
	int nCount = 0;
	do
	{
		const HE_Edge& e = GetEdge(nEdge);
		vSum += GetPos(e);
		nEdge = e.next_edge;
		++nCount;
	} while(nEdge != nStartingEdge);
	return vSum/(BrushFloat)nCount;
}

void CBrushDesignerHalfEdgeMesh::GetFaceVertices( const BUtil::HE_Face& f, std::vector<BrushVec3>& outVertices ) const
{
	int nStartingEdge = f.edge;
	int nEdge = nStartingEdge;
	outVertices.reserve(4);
	do
	{
		const HE_Edge& e = GetEdge(nEdge);
		outVertices.push_back(GetPos(e));
		nEdge = e.next_edge;
	} while(nEdge != nStartingEdge);
}

void CBrushDesignerHalfEdgeMesh::CreateMeshFaces( std::vector<BUtil::SMeshInfo>& outMeshes, bool bGenerateBackFaces )
{
	BUtil::SMeshInfo* m = NULL;
	for( int i = 0, iFaceCount(m_Faces.size()); i < iFaceCount; ++i )
	{
		std::vector<BrushVec3> vList;
		const HE_Face& f = m_Faces[i];
		GetFaceVertices(f,vList);

		int estimatedVertexCount = 0;
		if( m != NULL )
		{
			estimatedVertexCount = m->vertexList.size() + vList.size();
			if( bGenerateBackFaces )
				estimatedVertexCount += vList.size();
		}

		if( m == NULL || estimatedVertexCount > 0xffff )
		{
			outMeshes.push_back(BUtil::SMeshInfo());
			m = &outMeshes[outMeshes.size()-1];
		}

		for( int a = 0; a < 2; ++a )
		{
			if( a == 1 && !bGenerateBackFaces )
				break;

			if( a == 1 )
				std::reverse(vList.begin(),vList.end());

			BrushVec3 vNormal = (vList[0]-vList[1]).Cross(vList[2]-vList[1]).GetNormalized();
			int nVertexOffset = m->vertexList.size();

			int iVListCount(vList.size());
			for( int k = 0; k < iVListCount; ++k )
			{
				m->vertexList.push_back(vList[k]);
				m->normalList.push_back(vNormal);
				SMeshTexCoord uv;
				BUtil::CalcTexCoords(SBrushPlane<float>(ToVec3(vNormal),0), f.pOriginRegion ? f.pOriginRegion->GetTexInfo() : STexInfo(), ToVec3(vList[k]), uv.s, uv.t);
				m->uvList.push_back(uv);
			}

			SMeshFace mf;
			mf.nSubset = m->AddMatID(f.pOriginRegion->GetMaterialID());
			for( int k = 0; k < iVListCount-2; ++k )
			{
				mf.v[0] = nVertexOffset;
				mf.v[1] = nVertexOffset+k+1;
				mf.v[2] = nVertexOffset+k+2;
				m->faceList.push_back(mf);
			}
		}
	}
}

void FindPointBetweenEdge( const BrushEdge3D& edge, CBrushRegion::RegionPtr pRegion, std::vector<CBrushRegion::RegionPtr>& neighbourRegions, std::map<BrushFloat,BrushVec3>& outPoints )
{
	AABB edgeAABB;
	edgeAABB.Reset();
	edgeAABB.Add(ToVec3(edge.m_v[0]));
	edgeAABB.Add(ToVec3(edge.m_v[1]));
	edgeAABB.Expand(Vec3(0.01f,0.01f,0.01f));

	BrushLine3D edgeLine(edge.m_v[0],edge.m_v[1]);

	for( int i = 0, iNeighbourRegionCount(neighbourRegions.size()); i < iNeighbourRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pNeighbourRegion = neighbourRegions[i];
		if( pRegion == pNeighbourRegion || pNeighbourRegion->IsOpen() )
			continue;

		for( int k = 0, iEdgeCount(neighbourRegions[i]->GetEdgeSize()); k < iEdgeCount; ++k )
		{
			BrushEdge3D neighbourEdge = pNeighbourRegion->GetEdge(k);
			if( !edgeAABB.IsContainPoint(neighbourEdge.m_v[0]) && !edgeAABB.IsContainPoint(neighbourEdge.m_v[1]) )
				continue;

			BrushFloat d0 = edgeLine.GetDistance(neighbourEdge.m_v[0]);
			BrushFloat d1 = edgeLine.GetDistance(neighbourEdge.m_v[1]);

			if( d0 > kDesignerEpsilon || d1 > kDesignerEpsilon )
				continue;

			BrushEdge3D intersectedEdge;
			if( BUtil::IntersectEdge3D(edge,neighbourEdge,intersectedEdge,kDesignerEpsilon) == eOR_One )
			{
				for( int a = 0; a < 2; ++a )
				{
					if( edge.m_v[0].IsEquivalent(intersectedEdge.m_v[a],kDesignerEpsilon) || edge.m_v[1].IsEquivalent(intersectedEdge.m_v[a],kDesignerEpsilon) )
						continue;
					BrushFloat dist = edge.m_v[0].GetDistance(intersectedEdge.m_v[a]);
					outPoints[dist] = intersectedEdge.m_v[a];
				}
			}
		}
	}
}

void CBrushDesignerHalfEdgeMesh::SolveTJunction( CBrushDesigner* pDesigner, CBrushRegion::RegionPtr pRegion, CBrushConvexes* pConvexes )
{
	AABB bbox = pRegion->GetBoundBox();
	bbox.Expand(Vec3(0.01f,0.01f,0.01f));
	std::vector<CBrushRegion::RegionPtr> neighbourRegions;

	{
		DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);
		for( int i = 0; i < BUtil::kMaxShelfCount; ++i )
		{
			pDesigner->SetShelf(i);
			pDesigner->QueryIntersectedRegionsByAABB(bbox,neighbourRegions);
		}
		if( neighbourRegions.empty() )
			return;
	}

	for( int i = 0, iConvexCount(pConvexes->GetConvexCount()); i < iConvexCount; ++i )
	{
		BUtil::Convex updatedConvex;
		BUtil::Convex& convex = pConvexes->GetConvex(i);
		int iVertexCount(convex.size());
		updatedConvex.reserve(iVertexCount);
		for( int k = 0; k < iVertexCount; ++k )
		{
			updatedConvex.push_back(convex[k]);

			BrushEdge3D e(convex[k],convex[(k+1)%iVertexCount]);
			std::map<BrushFloat,BrushVec3> pointsOnEdge;
			FindPointBetweenEdge(e,pRegion,neighbourRegions,pointsOnEdge);
			if( pointsOnEdge.empty() )
				continue;

			std::map<BrushFloat,BrushVec3>::iterator iiForPoionts = pointsOnEdge.begin();
			for( ; iiForPoionts != pointsOnEdge.end(); ++iiForPoionts )
				updatedConvex.push_back(iiForPoionts->second);
		}
		if( updatedConvex.size() > convex.size() )
			convex = updatedConvex;
	}
}

const HE_Edge* CBrushDesignerHalfEdgeMesh::FindNextEdgeClockwiseAroundVertex( const HE_Edge& edge ) const
{
	const HE_Edge* pPairEdge = GetPairEdge(edge);
	if( pPairEdge == NULL )
		return NULL;
	return &m_Edges[pPairEdge->next_edge];
}

const HE_Edge& CBrushDesignerHalfEdgeMesh::FindEndEdgeCounterClockwiseAroundVertex( const HE_Edge& edge ) const
{
	const HE_Edge* pEdge = &edge;
	const HE_Edge* pPrevEdge = NULL;
	do 
	{
		pPrevEdge = &(GetPrevEdge(*pEdge));
		const HE_Edge* pPairOfPrevEdge = GetPairEdge(*pPrevEdge);
		if( pPairOfPrevEdge == NULL )
			break;
		pEdge = pPairOfPrevEdge;
	} while( pEdge != &edge );
	return *pPrevEdge;
}

bool CBrushDesignerHalfEdgeMesh::IsIrregularFace( const HE_Face& f ) const
{
	int eindex = -1;

	while(eindex != f.edge)
	{
		if( eindex == -1 )
			eindex = f.edge;

		if( GetEdge(eindex).irregular )
			return true;

		const HE_Vertex& v = GetVertex(GetEdge(eindex));
		int nValenceCount = 0;
		if( GetValenceCount(v,nValenceCount) && nValenceCount != 4 )
			return true;

		eindex = GetEdge(eindex).next_edge;
	}

	return false;
}

bool CBrushDesignerHalfEdgeMesh::GetValenceCount( const HE_Vertex& v, int& nOutValenceCount ) const
{
	const HE_Edge* pFirstEdge = &GetEdge(v);
	const HE_Edge* pEdge = NULL;
	int nValenceCount = 0;

	while( pEdge != pFirstEdge )
	{
		if( pEdge == NULL )
			pEdge = pFirstEdge;

		++nValenceCount;
		
		pEdge = FindNextEdgeClockwiseAroundVertex(*pEdge);
		if( pEdge == NULL )
			return false;
	}

	nOutValenceCount = nValenceCount;
	return true;
}