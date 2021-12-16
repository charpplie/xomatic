#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
// -------------------------------------------------------------------------
//  File name:   BrushTree2D.h
//  Created:     8/23/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////
class CBSPTree2DNode;

class CBrushBSPTree2D : public CRefCountBase
{

public:

	CBrushBSPTree2D()
	{
		m_pRootNode = NULL;
	}
	~CBrushBSPTree2D();

	BUtil::EPointPosEnum IsVertexIn( const Vec3d& vertex ) const;
	BUtil::EIntersectionType HasIntersection( const BrushEdge3D& edge ) const;
	bool IsInside( const BrushEdge3D& edge, bool bCheckCoDiff ) const;
	bool IsOnEdge( const BrushEdge3D& edge ) const;

	struct SOutputEdges
	{
		BrushEdge3D::Edge3DList posList;
		BrushEdge3D::Edge3DList negList;
		BrushEdge3D::Edge3DList coSameList;
		BrushEdge3D::Edge3DList coDiffList;
	};

	void GetPartitions( const BrushEdge3D& inEdge, SOutputEdges& outEdges ) const;

	void BuildTree( const BrushPlane& plane, const std::vector<BrushEdge3D>& edgeList );
	void GetEdgeList( BrushEdge3D::Edge3DList& outEdgeList ) const	{ GetEdgeList( m_pRootNode, outEdgeList ); }

	bool HasNegativeNode() const;

private:

	struct SIntersection
	{
		BrushVec2 point;
		int lineIndex[2];
	};
	typedef std::vector<SIntersection> IntersectionList;

	static void GetEdgeList( CBSPTree2DNode* pTree, BrushEdge3D::Edge3DList& outEdgeList );
	static CBSPTree2DNode* ConstructTree( const BrushPlane& plane, const std::vector<BrushEdge3D>& edgeList );

private:
	CBSPTree2DNode* m_pRootNode;

public:
	typedef _smart_ptr<CBrushBSPTree2D> BSPTree2DPtr;
};