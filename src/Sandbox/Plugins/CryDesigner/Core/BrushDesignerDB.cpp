#include "Stdafx.h"
#include "BrushDesignerDB.h"
#include "BrushDesigner.h"
#include "Viewport.h"

class CPlaneDB
{
public:

	static const BrushFloat& kPlaneEpsilon;	

	CPlaneDB(){}
	~CPlaneDB(){}

	CPlaneDB( const CPlaneDB& planeMgr )
	{
		operator =(planeMgr);
	}

	CPlaneDB& operator =( const CPlaneDB& planeMgr )
	{
		m_NormalXs = planeMgr.m_NormalXs;
		m_NormalYs = planeMgr.m_NormalYs;
		m_NormalZs = planeMgr.m_NormalZs;
		m_Distances = planeMgr.m_Distances;
		return *this;
	}

#define INITIALIZE_INDICES_AND_SIGN_AND_FIND_PLANE() int nNormalXIndex(-1); \
	int nNormalYIndex(-1); \
	int nNormalZIndex(-1); \
	int nDistanceIndex(-1); \
	FindPlane( inPlane, nNormalXIndex, nNormalYIndex, nNormalZIndex, nDistanceIndex );

	BrushPlane AddPlane( const BrushPlane& inPlane )
	{
		INITIALIZE_INDICES_AND_SIGN_AND_FIND_PLANE();

		if( nNormalXIndex == -1 )
		{
			nNormalXIndex = m_NormalXs.size();
			m_NormalXs.push_back(inPlane.Normal().x);
		}

		if( nNormalYIndex == -1 )
		{
			nNormalYIndex = m_NormalYs.size();
			m_NormalYs.push_back(inPlane.Normal().y);
		}

		if( nNormalZIndex == -1 )
		{
			nNormalZIndex = m_NormalZs.size();
			m_NormalZs.push_back(inPlane.Normal().z);
		}

		if( nDistanceIndex == -1 )
		{
			nDistanceIndex = m_Distances.size();
			m_Distances.push_back(inPlane.Distance());
		}

		return BrushPlane( BrushVec3(m_NormalXs[nNormalXIndex], m_NormalYs[nNormalYIndex], m_NormalZs[nNormalZIndex]), m_Distances[nDistanceIndex] );
	}
	bool FindPlane( const BrushPlane& inPlane, BrushPlane& outPlane ) const
	{
		INITIALIZE_INDICES_AND_SIGN_AND_FIND_PLANE();

		if( nNormalXIndex != -1 && nNormalYIndex != -1 && nNormalZIndex != -1 && nDistanceIndex != -1 )
		{
			outPlane.Set(BrushVec3(m_NormalXs[nNormalXIndex], m_NormalYs[nNormalYIndex], m_NormalZs[nNormalZIndex]), m_Distances[nDistanceIndex]);
			return true;
		}

		return false;
	}
	void Clear()
	{
		m_NormalXs.clear();
		m_NormalYs.clear();
		m_NormalZs.clear();
		m_Distances.clear();
	}

private:
	void FindPlane( const BrushPlane& inPlane, int& outNormalXIndex, int& outNormalYIndex, int& outNormalZIndex, int& outDistanceIndex ) const
	{
		Find( inPlane.Normal().x, m_NormalXs, outNormalXIndex );
		Find( inPlane.Normal().y, m_NormalYs, outNormalYIndex );
		Find( inPlane.Normal().z, m_NormalZs, outNormalZIndex );
		Find( inPlane.Distance(), m_Distances, outDistanceIndex );
	}
	void Find( const BrushFloat& inValue, const std::vector<BrushFloat>& valueList, int& outIndex ) const
	{
		for( int i = 0, iValueSize(valueList.size()); i < iValueSize; ++i )
		{
			if( fabs(valueList[i]-inValue) < kPlaneEpsilon )
			{
				outIndex = i;
				return;
			}
		}
		outIndex = -1;
	}

	std::vector<BrushFloat> m_NormalXs;
	std::vector<BrushFloat> m_NormalYs;
	std::vector<BrushFloat> m_NormalZs;
	std::vector<BrushFloat> m_Distances;
};

const BrushFloat& CBrushDesignerDB::kDBEpsilon = kDesignerEpsilon;
const BrushFloat& CPlaneDB::kPlaneEpsilon = kDesignerEpsilon;

CBrushDesignerDB::CBrushDesignerDB() :
m_pPlaneDB(new CPlaneDB)
{
}

CBrushDesignerDB::CBrushDesignerDB( const CBrushDesignerDB& db ) :
m_pPlaneDB(new CPlaneDB)
{
	operator =(db);
}

CBrushDesignerDB::~CBrushDesignerDB()
{
}

CBrushDesignerDB& CBrushDesignerDB::operator =( const CBrushDesignerDB& db )
{
	m_VertexDB = db.m_VertexDB;
	*m_pPlaneDB = *db.m_pPlaneDB;
	return *this;
}

void CBrushDesignerDB::Reset( CBrushDesigner* pDesigner, int nFlag, int nValidShelfID )
{
	DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);

	if( nFlag & BUtil::eDBRF_Vertex )
	{
		m_VertexDB.clear();
		for( BUtil::ShelfID shelfID = 0; shelfID < BUtil::kMaxShelfCount; ++shelfID )
		{
			if( nValidShelfID != -1 && nValidShelfID != shelfID )
				continue;
			pDesigner->SetShelf(shelfID);
			int iRegionSize(pDesigner->GetRegionSize());
			for( int i = 0; i < iRegionSize; ++i )
			{
				CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
				for( int k = 0, iVertexSize(pRegion->GetVertexListSize()); k < iVertexSize; ++k )
					AddVertex( pRegion->GetVertex(k), k, pRegion );
			}
		}
	}	

	if( nFlag & BUtil::eDBRF_Plane )
	{
		m_pPlaneDB->Clear();
		for( BUtil::ShelfID shelfID = 0; shelfID < BUtil::kMaxShelfCount; ++shelfID )
		{
			if( nValidShelfID != -1 && nValidShelfID != shelfID )
				continue;
			pDesigner->SetShelf(shelfID);
			int iRegionSize(pDesigner->GetRegionSize());
			for( int i = 0; i < iRegionSize; ++i )
			{
				CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
				BrushPlane plane;
				if( m_pPlaneDB->FindPlane(pRegion->GetPlane(),plane) )
				{
					pRegion->UpdatePlane( plane ); 
				}
				else
				{
					plane = m_pPlaneDB->AddPlane(pRegion->GetPlane());
					pRegion->UpdatePlane( plane );
				}
			}
		}
	}

	UpdateAllRegionVertices();
}

void CBrushDesignerDB::AddVertex( const BrushVec3& vertex, int nVertexIndex, CBrushRegion::RegionPtr pRegion )
{
	Vertex newV;
	Vertex * pV = &newV;

	for( int i = 0, iVertexSize(m_VertexDB.size()); i < iVertexSize; ++i )
	{
		Vertex& v = m_VertexDB[i];
		if( v.m_Pos.IsEquivalent(vertex,kDesignerEpsilon) )
		{
			pV = &v;
			break;
		}
	}

	Mark mark;	
	mark.m_pRegion = pRegion;
	mark.m_VertexIndex = nVertexIndex;
	pV->m_MarkList.push_back(mark);

	if( pV == &newV )
	{
		newV.m_Pos = vertex;
		m_VertexDB.push_back(newV);
	}
}

bool CBrushDesignerDB::QueryAsVertex( const BrushVec3& pos, QueryResult& qResult ) const
{
	for( int i = 0, iVertexSize(m_VertexDB.size()); i < iVertexSize; ++i )
	{
		const Vertex& v = m_VertexDB[i];
		if( v.m_Pos.IsEquivalent(pos,kDBEpsilon) )
		{
			bool bFoundSame(false);
			for( int k = 0, iQuerySize(qResult.size()); k < iQuerySize; ++k )
			{
				if( qResult[k].m_Pos.IsEquivalent(v.m_Pos,kDBEpsilon) )
				{
					bFoundSame = true;
					break;
				}
			}
			if( !bFoundSame )
				qResult.push_back(v);
		}
	}

	return !qResult.empty();
}

bool CBrushDesignerDB::QueryAsRectangle( CViewport* pView, const BrushMatrix34& worldTM, const CRect& rect, QueryResult& qResult ) const
{
	for( int i = 0, iVertexSize(m_VertexDB.size()); i < iVertexSize; ++i )
	{
		const Vertex& v = m_VertexDB[i];
		CPoint pt = pView->WorldToView(worldTM.TransformPoint(v.m_Pos));
		if( rect.PtInRect(pt) )
		{
			bool bFoundSame(false);
			for( int k = 0, iQuerySize(qResult.size()); k < iQuerySize; ++k )
			{
				if( qResult[k].m_Pos.IsEquivalent(v.m_Pos,kDBEpsilon) )
				{
					bFoundSame = true;
					break;
				}
			}
			if( !bFoundSame )
				qResult.push_back(v);
		}
	}

	return !qResult.empty();
}

void CBrushDesignerDB::AddMarkToVertex( const BrushVec3& vPos, const Mark& mark )
{
	for( int i = 0, iVertexSize(m_VertexDB.size()); i < iVertexSize; ++i )
	{
		Vertex& v = m_VertexDB[i];
		if( v.m_Pos.IsEquivalent(vPos,kDBEpsilon) )
		{
			v.m_MarkList.push_back(mark);
			return;
		}
	}
}

void CBrushDesignerDB::UpdateAllRegionVertices()
{
	for( int i = 0, vListSize(m_VertexDB.size()); i < vListSize; ++i )
	{
		Vertex& v = m_VertexDB[i];
		for( int k = 0, markListSize(v.m_MarkList.size()); k < markListSize; ++k )
		{
			CBrushRegion::RegionPtr pRegion = v.m_MarkList[k].m_pRegion;
			if( pRegion == NULL )
				continue;
			if( v.m_MarkList[k].m_VertexIndex >= pRegion->GetVertexListSize() )
				continue;
			pRegion->SetVertex( v.m_MarkList[k].m_VertexIndex, v.m_Pos );
		}
	}
}

bool CBrushDesignerDB::UpdateRegionVertices( CBrushRegion::RegionPtr pRegion )
{
	if( !pRegion )
		return false;

	bool bChanged = false;

	for( int i = 0, iVertexSize(pRegion->GetVertexListSize()); i < iVertexSize; ++i )
	{
		BrushVec3 vPos = pRegion->GetVertex(i);
		QueryResult qResult;
		if( QueryAsVertex(vPos,qResult) )
		{
			bChanged = true;
			pRegion->SetVertex(i, qResult[0].m_Pos);
		}
	}

	return bChanged;
}

BrushVec3 CBrushDesignerDB::Snap( const BrushVec3& pos )
{
	QueryResult qResult;
	if( QueryAsVertex( pos, qResult ) )
		return qResult[0].m_Pos;
	return pos;
}

BrushPlane CBrushDesignerDB::AddPlane( const BrushPlane& plane )
{
	return m_pPlaneDB->AddPlane(plane);
}

bool CBrushDesignerDB::FindPlane( const BrushPlane& inPlane, BrushPlane& outPlane ) const
{
	return m_pPlaneDB->FindPlane(inPlane,outPlane);
}

void CBrushDesignerDB::GetVertexList( std::vector<BrushVec3>& outVertexList ) const
{
	int vListSize = m_VertexDB.size();
	outVertexList.resize(vListSize);
	for( int i = 0; i < vListSize; ++i )
		outVertexList[i] = m_VertexDB[i].m_Pos;
}