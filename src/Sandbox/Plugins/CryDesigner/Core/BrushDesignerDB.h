#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011
// -------------------------------------------------------------------------
//  File name:   BrushDesignerDB.h
//  Created:     March/5/2012 by Jaesik
//  Description: Brush designer database
////////////////////////////////////////////////////////////////////////////
#include "BrushRegion.h"

class CBrushDesigner;
class CPlaneDB;

class CBrushDesignerDB : public CRefCountBase
{
public:

	static const BrushFloat& kDBEpsilon;	

	struct Mark
	{
		Mark() :
		m_pRegion(NULL),
		m_VertexIndex(-1)
		{
		}

		CBrushRegion::RegionPtr m_pRegion;
		int m_VertexIndex;
	};
	typedef std::vector<Mark> MarkList;

	struct Vertex
	{
		BrushVec3 m_Pos;
		MarkList m_MarkList;

		void Merge( const Vertex& v )
		{
			m_MarkList.insert(m_MarkList.end(),v.m_MarkList.begin(),v.m_MarkList.end());
		}
	};

	typedef std::vector<Vertex> QueryResult;

public:
	CBrushDesignerDB();
	CBrushDesignerDB( const CBrushDesignerDB& db );
	~CBrushDesignerDB();
	CBrushDesignerDB& operator =( const CBrushDesignerDB& db );
	
	void Reset( CBrushDesigner* pDesigner, int nFlag, int nValidShelfID = -1 );	
	bool QueryAsVertex( const BrushVec3& pos, QueryResult& qResult ) const;
	bool QueryAsRectangle( CViewport* pView, const BrushMatrix34& worldTM, const CRect& rect, QueryResult& qResult ) const;
	bool QueryAsRay( BrushVec3& vRaySrc, BrushVec3& vRayDir, QueryResult& qResult ) const;
	void AddMarkToVertex( const BrushVec3& vPos, const Mark& mark );
	bool UpdateRegionVertices( CBrushRegion::RegionPtr pRegion );
	BrushVec3 Snap( const BrushVec3& pos );

	BrushPlane AddPlane( const BrushPlane& plane );
	bool FindPlane( const BrushPlane& inPlane, BrushPlane& outPlane ) const;

	void GetVertexList( std::vector<BrushVec3>& outVertexList ) const;	

private:
	void AddVertex( const BrushVec3& vertex, int nVertexIndex, CBrushRegion::RegionPtr pRegion );
	void UpdateAllRegionVertices();

private:

	std::vector<Vertex> m_VertexDB;
	std::unique_ptr<CPlaneDB> m_pPlaneDB;
};