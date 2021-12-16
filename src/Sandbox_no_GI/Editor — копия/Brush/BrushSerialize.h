///////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   BrushSerialize.h
//  Version:     v1.00
//  Created:     5/5/2010 by Jaesik.
//  Compilers:   Visual Studio.NET
//  Description: 
////////////////////////////////////////////////////////////////////////////

#include "BrushCommon.h"

struct SBrush;

class CBrushSerialize
{
public:

	void Serialize( SBrush* brush, XmlNodeRef &xmlNode, bool bLoading, bool bUndo ) const;


private:

	struct STriangleForLoading
	{
		Vec3 pos[3];
		int matID;
		SBrushCommon::STexInfo texinfo;
	};

	struct SPolygonForLoading
	{
		std::vector<SBrushCommon::VertexFaceIndexType> vertexindexlist;
		int matID;
		SBrushCommon::STexInfo texinfo;
	};

	typedef std::vector<Vec3> VertexPositionList;

	struct SVertexForCompiling
	{
		Vec3 pos;
		Vec3 normal;
		float uv[2];
	};


private:

	void Load( SBrush* brush, const XmlNodeRef& xmlNode )	const;
	void LoadFromPolygon( SBrush* brush, const XmlNodeRef& xmlNode )	const;
	void LoadFromFace( SBrush* brush, const XmlNodeRef& xmlNode )	const;
	void LoadFromTriangle( SBrush* brush, const XmlNodeRef& xmlNode )	const;

	void Save( SBrush* brush, XmlNodeRef& xmlNode, bool bUndo ) const;
	void SaveToFace( SBrush* brush, XmlNodeRef& xmlNode ) const;
	void SaveToPolygon( SBrush* brush, XmlNodeRef& xmlNode ) const;

	void CompileSolidFromTriangle2Face( SBrush* brush, const std::vector<STriangleForLoading>& trilist ) const;
	void CompileSolidFromFace2Triangle( SBrush* brush, const std::vector<STriangleForLoading>& facelist ) const;
	void CompileSolidFromPolygon( SBrush* brush, const std::vector<SPolygonForLoading>& polylist, const VertexPositionList& vertexlist ) const;

	void LoadTexInfo( SBrushCommon::STexInfo*		texinfo, const XmlNodeRef& node ) const;
	void LoadPolygon( SPolygonForLoading* polygon, const XmlNodeRef& polygonNode ) const;
	void LoadVertexList( VertexPositionList& vertexlist, const XmlNodeRef& node ) const;

	void SaveTexInfo( const SBrushCommon::STexInfo* texinfo, XmlNodeRef& node ) const;
	void SaveVertexPos( const SBrush* brush, XmlNodeRef& node ) const;
};