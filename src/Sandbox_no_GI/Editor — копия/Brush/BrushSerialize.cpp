///////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   BrushSerialize.cpp
//  Version:     v1.00
//  Created:     5/5/2010 by Jaesik.
//  Compilers:   Visual Studio.NET
//  Description: 
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BrushSerialize.h"
#include "BrushFace.h"
#include "Brush.h"

void CBrushSerialize::Serialize( SBrush* brush, XmlNodeRef &xmlNode, bool bLoading, bool bUndo ) const
{
	if(bLoading)
		Load( brush, xmlNode );	
	else	
		Save( brush, xmlNode, bUndo );	
}


void CBrushSerialize::Load( SBrush* brush, const XmlNodeRef& xmlNode )	const
{
	if( xmlNode->getChildCount() < 1 )
		return;

	xmlNode->getAttr( "ZeroTolerance", brush->m_isZeroTolerance );
	const char* tag = xmlNode->getChild(0)->getTag();

	if( !strcmp( tag, "Face" ) )
	{
		LoadFromFace( brush, xmlNode );	
	}
	else if( !strcmp( tag, "Triangle" ) )
	{
		LoadFromTriangle( brush, xmlNode );
	}
	else if( !strcmp( tag, "Polygon") )
	{
		LoadFromPolygon( brush, xmlNode );
	}
}


void CBrushSerialize::LoadFromPolygon( SBrush* brush, const XmlNodeRef& xmlNode )	const
{
	std::vector<SPolygonForLoading> polylist;		
	int numPolygons = xmlNode->getChildCount()-1;
	polylist.resize(numPolygons);
	for( int i = 0; i < numPolygons; ++i )
	{
		SPolygonForLoading *polygon = &(polylist[i]);
		XmlNodeRef polygonNode = xmlNode->getChild(i);
		LoadPolygon( polygon, polygonNode );
	}
	VertexPositionList vertexlist;
	LoadVertexList( vertexlist, xmlNode );
	CompileSolidFromPolygon(brush,polylist, vertexlist);
}


void CBrushSerialize::LoadFromFace( SBrush* brush, const XmlNodeRef& xmlNode )	const
{
	int numFaces = xmlNode->getChildCount();

	std::vector<STriangleForLoading> trilist;
	trilist.resize(numFaces);

	std::vector<SPolygonForLoading> polygonlist;
	polygonlist.resize(numFaces);

	VertexPositionList vertexlist;

	bool bOldFaceLoading = true;

	for (int i = 0; i < numFaces; ++i)
	{
		XmlNodeRef faceNode = xmlNode->getChild(i);

		// This is to load a solid that was saved as a old form in new sandbox version.
		// I tried to load a solid that has only 3 points for representing a face but it caused some problems in CSG calculation
		// because the values that computed from the 3 points are not exact.
		// p0,p1,p2 are used in old version sandbox.
		// Attributes of NumberOfPoints, v0, v1, v2, v3,.... are used in new version sandbox.
		if( faceNode->haveAttr("NumberOfPoints") )
		{
			SPolygonForLoading * polygon = &polygonlist[i];
			XmlNodeRef polygonNode = xmlNode->getChild(i);
			LoadPolygon( polygon, polygonNode );
			bOldFaceLoading = false;
			if( i == 0 )
				LoadVertexList( vertexlist, faceNode );
		}
		else
		{
			STriangleForLoading * triangle = &(trilist[i]);

			faceNode->getAttr( "p1", triangle->pos[0] );
			faceNode->getAttr( "p2", triangle->pos[1] );
			faceNode->getAttr( "p3", triangle->pos[2] );

			int nMatId;
			faceNode->getAttr( "MatId", nMatId );
			triangle->matID = nMatId;

			LoadTexInfo( &triangle->texinfo, faceNode );
		}
	}

	if( bOldFaceLoading )
		CompileSolidFromFace2Triangle(brush,trilist);		
	else		
		CompileSolidFromPolygon(brush,polygonlist,vertexlist);		
}


void CBrushSerialize::LoadFromTriangle( SBrush* brush, const XmlNodeRef& xmlNode )	const
{
	std::vector<STriangleForLoading> trilist;		
	int numTriangles = xmlNode->getChildCount();
	trilist.resize(numTriangles);
	for (int i = 0; i < numTriangles; ++i)
	{
		STriangleForLoading *tri = &(trilist[i]);

		XmlNodeRef triangleNode = xmlNode->getChild(i);
		triangleNode->getAttr( "p1", tri->pos[0] );
		triangleNode->getAttr( "p2", tri->pos[1] );
		triangleNode->getAttr( "p3", tri->pos[2] );

		int nMatId;
		triangleNode->getAttr( "MatId", nMatId );
		tri->matID = nMatId;

		LoadTexInfo( &tri->texinfo, triangleNode );
	}
	CompileSolidFromTriangle2Face(brush,trilist);
}



void CBrushSerialize::Save( SBrush* brush, XmlNodeRef& xmlNode, bool bUndo ) const
{
	xmlNode->setAttr( "ZeroTolerance", brush->m_isZeroTolerance );

	if( bUndo == false && brush->IsConvexHull() )
	{
		SaveToFace( brush, xmlNode );
	}
	else
	{
		SaveToPolygon( brush, xmlNode );
	}
}


void CBrushSerialize::SaveToFace( SBrush* brush, XmlNodeRef& xmlNode ) const
{
	for( size_t i = 0; i < brush->GetNumberOfFaces(); ++i )
	{
		SBrushFace*			face			= brush->m_BrushFaces[i];
		SBrushTriangle* triangle	= brush->m_BrushTriangles[face->triangleidxlist[0]];

		XmlNodeRef faceNode = xmlNode->newChild( "Face" );

		Vec3 v[3] = {	brush->GetVertexPos(triangle->vertexindices[0]),
			brush->GetVertexPos(triangle->vertexindices[1]),
			brush->GetVertexPos(triangle->vertexindices[2]) };

		faceNode->setAttr( "p1", v[2] );
		faceNode->setAttr( "p2", v[1] );
		faceNode->setAttr( "p3", v[0] );

		// This is to keep compatibility between old and new version.
		// Only old face information can hardly make perfect solid in new solid version 
		// so in order to create a precise solid without any problems in new solid version
		// we must save all points consisted of face.
		// These data aren't used in old version so don't cause any problem in old solid version.
		faceNode->setAttr( "NumberOfPoints", face->pointindexlist.size() );	
		for( size_t k = 0; k < face->pointindexlist.size(); ++k )
		{
			string attribute;
			attribute.Format("v%d", k );
			faceNode->setAttr( attribute, face->pointindexlist[k] );
		}
		//////////////////////////////////////////////////////////////////////////////////////////

		faceNode->setAttr( "MatId", face->matID );
		SaveTexInfo( &face->texinfo, faceNode );
		if( i == 0 )
			SaveVertexPos( brush, faceNode );
	}
}


void CBrushSerialize::SaveToPolygon( SBrush* brush, XmlNodeRef& xmlNode ) const
{
	xmlNode->setAttr( "ZeroTolerance", brush->m_isZeroTolerance );
	for( size_t i = 0; i < brush->GetNumberOfFaces(); ++i )
	{
		SBrushFace* face = brush->m_BrushFaces[i];
		XmlNodeRef polyNode = xmlNode->newChild( "Polygon" );
		polyNode->setAttr( "NumberOfPoints", (int)face->pointindexlist.size() );
		for( size_t k = 0; k < face->pointindexlist.size(); ++k )
		{
			string attribute;
			attribute.Format("v%d", k );
			polyNode->setAttr( attribute, face->pointindexlist[k] );
		}
		polyNode->setAttr( "MatId", face->matID );
		SaveTexInfo( &face->texinfo, polyNode);
	}
	SaveVertexPos( brush, xmlNode );
}


void CBrushSerialize::CompileSolidFromTriangle2Face( SBrush* brush, const std::vector<STriangleForLoading>& trilist ) const
{
	std::vector<bool> bFaceSelectedArray;
	size_t OldFaceSize = brush->m_BrushFaces.size();
	if( brush->m_BrushFaces.empty() == false )
	{
		bFaceSelectedArray.resize(OldFaceSize);
		for( int i = 0; i < OldFaceSize; ++i )
			bFaceSelectedArray[i] = brush->IsFaceSelected(i);	
	}

	brush->ClearFaces();
	brush->Invalidate();

	std::vector<SVertexForCompiling> vlist;
	std::vector<STriangleForLoading>::const_iterator itri = trilist.begin();
	for( ; itri != trilist.end(); ++itri )
	{
		const STriangleForLoading* tri = &(*itri);
		Vec3 trinormal = brush->CalcNormal( tri->pos[0], tri->pos[1], tri->pos[2] );

		SBrushCommon::VertexFaceIndexType newindex[3] = { SBrushCommon::VertexFaceMax, 
																											SBrushCommon::VertexFaceMax, 
																											SBrushCommon::VertexFaceMax };

		for( int i = 0; i < 3; ++i )
		{	
			for( size_t k = 0; k < vlist.size(); ++k )
			{
				SVertexForCompiling* v = &vlist[k];

				if( v->pos.IsEquivalent( tri->pos[i], SBrush::VER_EPS ) && 
						v->normal.IsEquivalent( trinormal, SBrush::VER_EPS ) )
				{
					newindex[i] = k;
					break;
				}
			}

			if( newindex[i] == SBrushCommon::VertexFaceMax )
			{
				newindex[i] = vlist.size();
				SVertexForCompiling newv;
				newv.pos = tri->pos[i];
				newv.normal = trinormal;
				vlist.push_back(newv);
			}
		}

		brush->AddTriangle(	vlist[newindex[0]].pos, 
												vlist[newindex[1]].pos, 
												vlist[newindex[2]].pos, 
												newindex[0], newindex[1], newindex[2] );
	}

	std::vector<SVertexForCompiling>::iterator iv = vlist.begin();
	brush->m_BrushVertices.reserve(vlist.size());
	for( ; iv != vlist.end(); ++iv )
	{
		SVertexForCompiling* v = &(*iv);
		SBrushVertex* bv = new SBrushVertex;

		bv->pos = v->pos;
		bv->st[0] = v->uv[0];
		bv->st[1] = v->uv[1];
		brush->m_BrushVertices.push_back(bv);
	}

	brush->ComputeAdjacentTriangle();
	brush->ComputeCoplanarTriangle();
	brush->ComputeBoundBox();	

	for( size_t i = 0; i < brush->GetNumberOfFaces(); ++i  )
	{
		SBrushFace* face = brush->m_BrushFaces[i];

		face->matID		= trilist[face->triangleidxlist[0]].matID;
		face->texinfo	= trilist[face->triangleidxlist[0]].texinfo;

		if( !bFaceSelectedArray.empty() &&  i < OldFaceSize )
			face->bSelected = bFaceSelectedArray[i];
	}

	brush->ComputeTexCoord();
}


void CBrushSerialize::CompileSolidFromFace2Triangle( SBrush* brush, const std::vector<STriangleForLoading>& facelist ) const
{
	brush->ClearFaces();
	brush->Invalidate();	

	for( size_t i = 0; i < facelist.size(); ++i )
	{
		std::vector<Vec3> vlist;
		const STriangleForLoading& f1 = facelist[i];
		SBrushPlane p1( f1.pos[2], f1.pos[1], f1.pos[0] );
		p1.CreatePoly(vlist);

		if( vlist.empty() )
			continue;

		for( size_t k = 0; k < facelist.size(); ++k )
		{
			if( i == k )
				continue;

			const STriangleForLoading& f2 = facelist[k];
			SBrushPlane p2( f2.pos[2], f2.pos[1], f2.pos[0] );

			std::vector<Vec3> outvlist;
			if( p1.ClipByPlane( p2, vlist, outvlist ) == true )
				vlist = outvlist;
		}

		if( vlist.size() < 3 )
			continue;

		SBrushFace* face = new SBrushFace;

		for( size_t k = 0; k < vlist.size(); ++k )
		{
			face->pointindexlist.push_back( (SBrushCommon::VertexFaceIndexType)brush->GetNumberOfVertices() );
			brush->AddVertex(vlist[k]);
		}

		for( size_t k = 0; k < face->pointindexlist.size()-2; ++k )
		{			
			if( brush->AddTriangle(	face->pointindexlist[0], 
															face->pointindexlist[k+1], 
															face->pointindexlist[k+2], 
															(SBrushCommon::VertexFaceIndexType)brush->GetNumberOfFaces() ) )
			{
				face->triangleidxlist.push_back( brush->GetNumberOfTriangles()-1 );
			}
		}

		face->matID		= f1.matID;
		face->texinfo = f1.texinfo;
		face->plane		= p1;

		brush->m_BrushFaces.push_back(face);
	}

	brush->ComputeAdjacentTriangle();
	brush->ComputeBoundBox();
	brush->ComputeTexCoord();
}


void CBrushSerialize::CompileSolidFromPolygon( SBrush* brush, 
																							 const std::vector<SPolygonForLoading>& polylist, 
																							 const VertexPositionList& vertexlist ) const
{
	std::vector<bool> bFaceSelectedArray;
	size_t OldFaceSize = brush->GetNumberOfFaces();
	if( brush->m_BrushFaces.empty() == false )
	{
		bFaceSelectedArray.resize(OldFaceSize);
		for( int i = 0; i < OldFaceSize; ++i )
			bFaceSelectedArray[i] = brush->IsFaceSelected(i);
	}

	brush->ClearFaces();
	brush->Invalidate();

	for( size_t i = 0; i < vertexlist.size(); i++ )	
		brush->AddVertex( vertexlist[i] );

	for( size_t i = 0; i < polylist.size(); ++i )
	{
		short matID = (short)polylist[i].matID;
		brush->AddFace( polylist[i].vertexindexlist, 
										&polylist[i].texinfo,
										&matID );
	}

	brush->ComputeBoundBox();

	for( size_t i = 0; i < brush->GetNumberOfFaces(); ++i  )
	{
		SBrushFace* face = brush->m_BrushFaces[i];
		if( !bFaceSelectedArray.empty() &&  i < OldFaceSize )
			face->bSelected = bFaceSelectedArray[i];
	}

	brush->ComputeTexCoord();
}


void CBrushSerialize::LoadPolygon( SPolygonForLoading* polygon, const XmlNodeRef& polygonNode ) const
{
	int numberOfPoints = 0;
	polygonNode->getAttr("NumberOfPoints",numberOfPoints);

	for( int j = 0; true/*j < numberOfPoints*/; ++j )
	{
		string attribute;
		attribute.Format( "v%d", j );
		SBrushCommon::VertexFaceIndexType vertexindex;
		bool ok = polygonNode->getAttr(attribute, vertexindex);
		if(ok == false)
			break;
		polygon->vertexindexlist.push_back(vertexindex);
	}

	int nMatId;
	polygonNode->getAttr( "MatId", nMatId );
	polygon->matID = nMatId;

	LoadTexInfo( &polygon->texinfo, polygonNode );
}


void CBrushSerialize::LoadVertexList( VertexPositionList& vertexlist, const XmlNodeRef& node ) const
{
	XmlNodeRef vertexNode = node->findChild( "Vertex" );

	int numberOfVertices;
	vertexNode->getAttr("NumberOfVertices",numberOfVertices);
	for( size_t i = 0; true/*i < numberOfVertices*/; ++i )
	{
		Vec3 position;
		string attribute;
		attribute.Format("p%d", i );
		bool ok = vertexNode->getAttr( attribute, position );
		if(ok == false)
			break;
		vertexlist.push_back(position);
	}
}


void CBrushSerialize::LoadTexInfo( SBrushCommon::STexInfo* texinfo, const XmlNodeRef& node ) const
{
	Vec3 texScale( 1,1,1 );
	Vec3 texShift( 0,0,0 );
	node->getAttr( "TexScale",texScale );
	node->getAttr( "TexShift",texShift );
	node->getAttr( "TexRotate",texinfo->rotate );
	texinfo->scale[0] = texScale.x;
	texinfo->scale[1] = texScale.y;
	texinfo->shift[0] = texShift.x;
	texinfo->shift[1] = texShift.y;
}


void CBrushSerialize::SaveTexInfo( const SBrushCommon::STexInfo* texinfo, XmlNodeRef& node ) const
{
	Vec3 texScale( texinfo->scale[0], texinfo->scale[1], 0 );
	Vec3 texShift( texinfo->shift[0], texinfo->shift[1], 0 );
	node->setAttr( "TexScale",	texScale );
	node->setAttr( "TexShift",	texShift );
	node->setAttr( "TexRotate", texinfo->rotate );
}


void CBrushSerialize::SaveVertexPos( const SBrush* brush, XmlNodeRef& node ) const
{
	XmlNodeRef vertexNode = node->newChild( "Vertex" );
	vertexNode->setAttr( "NumberOfVertices", (int)brush->GetNumberOfVertices() );

	for( size_t i = 0; i < brush->GetNumberOfVertices(); ++i )
	{
		SBrushVertex* vertex = brush->m_BrushVertices[i];
		string attribute;
		attribute.Format("p%d", i );
		vertexNode->setAttr( attribute, vertex->pos );
	}
}