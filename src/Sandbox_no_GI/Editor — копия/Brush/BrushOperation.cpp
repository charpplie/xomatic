///////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   BrushOperation.cpp
//  Version:     v1.00
//  Created:     5/5/2010 by Jaesik.
//  Compilers:   Visual Studio.NET
//  Description: 
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Brush.h"
#include "BrushFace.h"
#include "Objects\SubObjSelection.h"


void SBrush::CountFrontBackVertex(	const SBrushPlane& plane, 
																		std::vector<char>& vsign, 
																		SBrushCommon::VertexFaceIndexType& front, 
																		SBrushCommon::VertexFaceIndexType& back, 
																		SBrushCommon::VertexFaceIndexType& zero ) const
{
	size_t vsize	= m_BrushVertices.size();	

	for( size_t i = 0; i < vsize; ++i )
	{
		SBrushVertex* v = m_BrushVertices[i];
		float distance = plane.Distance(v->pos);

		if( distance > VER_EPS )
		{
			vsign[i] = 1;
			++front;
		}
		else if( distance < -VER_EPS )
		{
			vsign[i] = -1;
			++back;
		}
		else
		{
			vsign[i] = 0;
			++zero;
		}
	}
}


void SBrush::FindMeetVertex(	const SBrushPlane& plane, 
															const std::vector<char>& vsign, 
															const SBrushFace* face,
															const bool& bJustForDisplay,
															std::vector<Vec3>& mvl,
															int& touchcount,
															SBrushCommon::VertexFaceIndexType* touchlocation,
															Vec3* contactpt ) const
{
	size_t	facevtxsize	= face->pointindexlist.size();
	size_t	facetrisize	= face->triangleidxlist.size();

	for( int k = 0; k < facevtxsize; ++k )
	{
		int nextk = (k+1)%facevtxsize;

		if( vsign[face->pointindexlist[k]] * vsign[face->pointindexlist[nextk]] == -1 )
		{
			Vec3 pt0 = m_BrushVertices[face->pointindexlist[k]]->pos;
			Vec3 pt1 = m_BrushVertices[face->pointindexlist[nextk]]->pos;
			plane.HitTest( pt0, pt1, NULL, &contactpt[touchcount], VER_EPS );
			touchlocation[touchcount] = k;

			if( bJustForDisplay == false )
			{
				bool bExist = false;
				for( size_t a = 0; a < mvl.size(); ++a )
				{
					if( mvl[a].IsEquivalent( contactpt[touchcount], VER_EPS ) )
					{
						bExist = true;
						break;
					}
				}

				if( bExist == false )
					mvl.push_back( contactpt[touchcount] );
			}

			++touchcount;
		}
	}
}


bool SBrush::SplitFace(	 const std::vector<char>& vsign, 
												 const SBrushFace* face,
												 const SBrushCommon::VertexFaceIndexType* touchlocation,
												 const Vec3* contactpt,
												 const bool& bJustForDisplay,
												 SBrush* frontbrush,
												 SBrush* backbrush ) const
{
	size_t	facevtxsize	= face->pointindexlist.size();

	for( int k = 0; k < 2; ++k )
	{
		int c0 = k;
		int c1 = 1-k;
		SBrushCommon::VertexFaceIndexType loc = touchlocation[c0]+1;
		SBrush* brush = vsign[face->pointindexlist[loc%facevtxsize]] > 0 ? frontbrush : backbrush;
		SBrushFace* newface = new SBrushFace;

		newface->pointindexlist.push_back((SBrushCommon::VertexFaceIndexType)brush->m_BrushVertices.size());
		if( brush->AddVertex(contactpt[c0]) == false )
			return false;

		while( (loc%facevtxsize) != (touchlocation[c1]+1)%facevtxsize )
		{
			newface->pointindexlist.push_back((SBrushCommon::VertexFaceIndexType)brush->m_BrushVertices.size());
			if( brush->AddVertex(m_BrushVertices[face->pointindexlist[loc%facevtxsize]]->pos) == false )
				return false;

			++loc;
		}

		newface->pointindexlist.push_back( (SBrushCommon::VertexFaceIndexType)brush->m_BrushVertices.size() );
		if( brush->AddVertex(contactpt[c1]) == false )
			return false;

		if( bJustForDisplay == false )
		{
			for( size_t a = 0; a < newface->pointindexlist.size()-2; ++a )
			{
				SBrushTriangle* newtri = new SBrushTriangle;
				newtri->SetVertexIndices(	brush->m_BrushVertices, 
																	newface->pointindexlist[0], 
																	newface->pointindexlist[a+1], 
																	newface->pointindexlist[a+2], 
																	(SBrushCommon::VertexFaceIndexType)brush->m_BrushFaces.size() );
				newface->triangleidxlist.push_back((SBrushCommon::VertexFaceIndexType)brush->m_BrushTriangles.size());
				brush->m_BrushTriangles.push_back(newtri);
			}
		}

		brush->m_BrushFaces.push_back(newface);
	}

	return true;
}


void SBrush::PutFaceIntoOneSide(	const std::vector<char>& vsign, 
																const SBrushFace* face,
																const bool& bJustForDisplay,
																SBrush* frontbrush,
																SBrush* backbrush,
																std::vector<SBrushCommon::VertexFaceIndexType>& frontvidxmap,
																std::vector<SBrushCommon::VertexFaceIndexType>& backvidxmap ) const
{
	size_t	facevtxsize	= face->pointindexlist.size();
	size_t	facetrisize	= face->triangleidxlist.size();

	SBrushFace* newface = new SBrushFace;
	SBrush*	brush	= vsign[face->pointindexlist[0]]	== 1 ? frontbrush : backbrush;
	SBrushCommon::VertexFaceIndexType*	vidxmap = vsign[face->pointindexlist[0]]	== 1 ? &frontvidxmap[0] : &backvidxmap[0];

	for( size_t k = 0; k < facevtxsize; ++k )
	{
		SBrushCommon::VertexFaceIndexType idx = (SBrushCommon::VertexFaceIndexType)brush->m_BrushVertices.size();
		vidxmap[face->pointindexlist[k]] = idx;
		brush->m_BrushVertices.push_back( new SBrushVertex(m_BrushVertices[face->pointindexlist[k]]->pos) );
		newface->pointindexlist.push_back(idx);
	}

	if( bJustForDisplay == false )
	{
		for( int k = 0; k < facetrisize; ++k )
		{
			SBrushTriangle * btri = m_BrushTriangles[face->triangleidxlist[k]];
			SBrushTriangle * newbtri = new SBrushTriangle;
			newbtri->SetVertexIndices(	brush->m_BrushVertices,
				vidxmap[btri->vertexindices[0]],
				vidxmap[btri->vertexindices[1]],
				vidxmap[btri->vertexindices[2]],
				(SBrushCommon::VertexFaceIndexType)brush->m_BrushFaces.size() );
			newface->triangleidxlist.push_back((SBrushCommon::VertexFaceIndexType)brush->m_BrushTriangles.size());
			brush->m_BrushTriangles.push_back(newbtri);
		}
	}

	brush->m_BrushFaces.push_back(newface);
}


void SBrush::SortForMakingConvexHullFace( const std::vector<Vec3>& mvl, std::vector<SBrushCommon::VertexFaceIndexType>& sml ) const
{
	Vec3 basicdir = mvl[1] - mvl[0];
	basicdir.Normalize();

	sml.push_back(0);
	sml.push_back(1);

	for( size_t i = 2; i < mvl.size(); ++i )
	{
		Vec3 dir0 = mvl[i] - mvl[0];
		dir0.Normalize();
		float dot0 = basicdir.Dot(dir0);

		std::vector<SBrushCommon::VertexFaceIndexType>::iterator isv = sml.begin()+2;
		for( ; isv != sml.end(); ++isv )
		{
			Vec3 dir1 = mvl[(*isv)] - mvl[0];
			dir1.Normalize();
			float dot1 = basicdir.Dot(dir1);

			if( dot0 > dot1 )
				break;
		}
		sml.insert( isv, i );
	}
}


bool SBrush::CheckIfFaceIsConvexHull( const std::vector<Vec3>& mvl, const std::vector<SBrushCommon::VertexFaceIndexType>& sml ) const
{
	size_t s_size = sml.size();
	Vec3 prev_v0 = mvl[sml[1]] - mvl[sml[0]];
	Vec3 prev_v1 = mvl[sml[s_size-1]] - mvl[sml[0]];
	Vec3 initnormal = prev_v0.Cross(prev_v1);
	initnormal.Normalize();
	float prevdot = 0;

	for( int i = 1; i < s_size+1; ++i )
	{
		int curr =  i%s_size;
		int prev = (i-1)%s_size;
		int next = (i+1)%s_size;

		Vec3 v0 = mvl[sml[next]] - mvl[sml[curr]];
		Vec3 v1 = mvl[sml[prev]] - mvl[sml[curr]];
		Vec3 normal = v0.Cross(v1);
		normal.Normalize();
		float dot = initnormal.dot(normal);

		if( i > 1 && fabs(prevdot-dot) > VER_EPS )
		{
			return false;
		}

		prevdot = dot;
	}

	return true;
}


void SBrush::MakeConvexHullFace(	const SBrushPlane& plane,
																const std::vector<Vec3>& mvl, 
																const std::vector<SBrushCommon::VertexFaceIndexType>& sml,
																SBrush* frontbrush,
																SBrush* backbrush ) const
{
	Vec3 normal = (mvl[sml[2]] - mvl[sml[0]]) ^ (mvl[sml[1]]-mvl[sml[0]]);
	normal.Normalize();
	float normaldot = normal|plane.normal;

	for( int i = 0; i < 2; ++i )
	{
		SBrush* brush = i==0 ? frontbrush : backbrush;
		SBrushFace* face = new SBrushFace;

		for( int k = 0; k < sml.size(); ++k )
		{
			face->pointindexlist.push_back((SBrushCommon::VertexFaceIndexType)brush->m_BrushVertices.size());

			if( brush == frontbrush )
			{
				if( normaldot > 0 )
					brush->m_BrushVertices.push_back( new SBrushVertex(mvl[sml[k]]) );
				else
					brush->m_BrushVertices.push_back( new SBrushVertex(mvl[sml[sml.size()-k-1]]) );
			}
			else 
			{
				if( normaldot > 0 )
					brush->m_BrushVertices.push_back( new SBrushVertex(mvl[sml[sml.size()-k-1]]) );							
				else
					brush->m_BrushVertices.push_back( new SBrushVertex(mvl[sml[k]]) );
			}
		}

		assert( face->pointindexlist.size() > 2 );

		for( size_t k = 0; k < face->pointindexlist.size()-2; ++k )
		{
			SBrushTriangle* tri = new SBrushTriangle;
			tri->SetVertexIndices(	brush->m_BrushVertices, 
				face->pointindexlist[0], 
				face->pointindexlist[k+1], 
				face->pointindexlist[k+2], 
				(SBrushCommon::VertexFaceIndexType)brush->m_BrushFaces.size() );
			face->triangleidxlist.push_back((SBrushCommon::VertexFaceIndexType)brush->m_BrushTriangles.size());
			brush->m_BrushTriangles.push_back(tri);
		}

		brush->m_BrushFaces.push_back(face);
	}		
}


bool SBrush::SplitByPlane( const SBrushPlane& plane, SBrush* &front, SBrush* &back, bool bJustForDisplay) const
{
	size_t vsize		= m_BrushVertices.size();
	size_t tsize		= m_BrushTriangles.size();
	size_t facesize	= m_BrushFaces.size();	

	std::vector<char> vsign;
	vsign.resize(vsize);

	SBrushCommon::VertexFaceIndexType FrontCounter	= 0;
	SBrushCommon::VertexFaceIndexType BackCounter		= 0;
	SBrushCommon::VertexFaceIndexType ZeroCounter		= 0;

	CountFrontBackVertex( plane,
												vsign,
												FrontCounter,
												BackCounter,
												ZeroCounter );

	if( FrontCounter > 0 && BackCounter > 0 && ZeroCounter == 0 )
	{
		SBrush* frontbrush	= new SBrush;
		SBrush* backbrush		= new SBrush;

		std::vector<SBrushCommon::VertexFaceIndexType> frontvidxmap;
		std::vector<SBrushCommon::VertexFaceIndexType> backvidxmap;

		frontvidxmap.resize(vsize);
		backvidxmap.resize(vsize);

		for( int i = 0; i < vsize; ++i )
		{
			frontvidxmap[i] = SBrushCommon::VertexFaceMax;
			backvidxmap[i]	= SBrushCommon::VertexFaceMax;
		}

		std::vector<Vec3> mvl; // meet vertex list

		for( size_t i = 0; i < facesize; ++i )
		{
			SBrushFace* face = m_BrushFaces[i];
			int		touchcount = 0;
			SBrushCommon::VertexFaceIndexType	touchlocation[2] = {SBrushCommon::VertexFaceMax,SBrushCommon::VertexFaceMax};
			Vec3	contactpt[2];

			FindMeetVertex(	plane,
											vsign,
											face,
											bJustForDisplay,
											mvl,
											touchcount,
											touchlocation,
											contactpt );

			if( touchcount == 2 )
			{
				if( SplitFace(	vsign, 
												face, 
												touchlocation, 
												contactpt, 
												bJustForDisplay, 
												frontbrush, 
												backbrush ) == false )
				{
					delete frontbrush;
					delete backbrush;
					front = NULL;
					back = NULL;
					return false;
				};
			}
			else if( touchcount == 0 )
			{
				PutFaceIntoOneSide( vsign, 
														face, 
														bJustForDisplay, 
														frontbrush, 
														backbrush, 
														frontvidxmap, 
														backvidxmap );
			}
			else
			{
				delete frontbrush;
				delete backbrush;
				front = NULL;
				back = NULL;
				return true;
			}
		}

		if( bJustForDisplay == false )
		{
			if( mvl.size() > 2 )
			{
				std::vector<SBrushCommon::VertexFaceIndexType> sml; // sorted meet vertex list
				SortForMakingConvexHullFace( mvl, sml );

				if( CheckIfFaceIsConvexHull( mvl, sml ) == false )
				{
					delete frontbrush;
					delete backbrush;
					front = NULL;
					back = NULL;
					return true;
				}

				MakeConvexHullFace( plane, mvl, sml, frontbrush, backbrush );
			}

			for( int i = 0; i < 2; ++i )
			{
				SBrush* brush = i == 0 ? frontbrush : backbrush;
				brush->ComputeAdjacentTriangle();
				brush->ComputeFacePlanes();
				brush->ComputeTexCoord();
				brush->ComputeBoundBox();
				brush->Invalidate();
			}
		}

		front	= frontbrush;
		back	= backbrush;
	}
	else if( FrontCounter > BackCounter )
	{
		SBrush* frontbrush = new SBrush;
		*frontbrush = *this;
		frontbrush->Invalidate();
		front = frontbrush;
	}
	else if( FrontCounter <= BackCounter )
	{
		SBrush* backbrush = new SBrush;
		*backbrush = *this;
		backbrush->Invalidate();
		back = backbrush;
	}

	return true;
}

bool SBrush::MergeBrush( const SBrush* brush, Vec3* newPivot )
{
	if( m_BrushVertices.size() + brush->m_BrushVertices.size() > SBrushCommon::VertexFaceMax-1 )
		return false;

	size_t basevtxidx		= m_BrushVertices.size();
	size_t basetriidx		= m_BrushTriangles.size();
	size_t basefaceidx	= m_BrushFaces.size();

	AABB BoundBox;
	BoundBox.Reset();
	BoundBox.Add(brush->GetBoundBox());

	AABB MyBoundBox( m_BrushVertices.empty() ? BoundBox : this->GetBoundBox() );
	Vec3 Offset = brush->GetMatrix().GetTranslation() - GetMatrix().GetTranslation();

	Matrix34 OffsetTM(brush->GetMatrix());
	OffsetTM.SetTranslation(Offset);

	for( size_t i = 0; i < brush->m_BrushVertices.size(); ++i )
	{
		SBrushVertex* vertex = new SBrushVertex(*brush->m_BrushVertices[i]);
		vertex->pos = OffsetTM * vertex->pos;
		m_bounds.Add(vertex->pos);
		m_BrushVertices.push_back(vertex);
	}

	Vec3 pivot;
	PivotToCenter(&pivot);
	m_matrix.SetTranslation(pivot);

	if( newPivot )
		*newPivot = pivot;

	for( size_t i = 0; i < brush->m_BrushTriangles.size(); ++i )
	{
		SBrushTriangle* triangle = new SBrushTriangle(*brush->m_BrushTriangles[i]);
		for( int k = 0; k < 3; ++k )
		{
			triangle->vertexindices[k] += basevtxidx;
			triangle->adjacentfidx[k]	+= basetriidx;
		}
		triangle->faceidx += basefaceidx;
		m_BrushTriangles.push_back(triangle);
	}

	for( size_t i = 0; i < brush->m_BrushFaces.size(); ++i )
	{
		SBrushFace* face = new SBrushFace(*brush->m_BrushFaces[i]);
		for( size_t k = 0; k < face->pointindexlist.size(); ++k )
			face->pointindexlist[k] += basevtxidx;
		for( size_t k = 0; k < face->triangleidxlist.size(); ++k )
			face->triangleidxlist[k] += basetriidx;		
		m_BrushFaces.push_back(face);
	}
	
	ComputeFacePlanes();
	Invalidate();

	return true;
}


void SBrush::Selection_DeleteVertex()
{
	if( !(m_selectionType == SO_ELEM_VERTEX) )
		return;

	RecordUndo( "Brush, Delete vertex");

	SBackupBrushInfo BackupInfo;
	BackupBrush(BackupInfo);

	size_t nFaces = m_BrushFaces.size();

	for( int i = 0; i < nFaces; ++i )
	{
		SBrushFace * face = m_BrushFaces[i];
		size_t ptssize = face->pointindexlist.size();

		for( int k = 0; k < ptssize; ++k )
		{
			SBrushVertex * bs = m_BrushVertices[face->pointindexlist[k]];

			if( bs->bSelected )
			{
				face->bSelected = true;
				break;
			}
		}
	}

	if( DeleteSelectedFace() == false )
	{
		RestoreBrush(BackupInfo);
		CryMessageBox( "This vertex cannot be deleted.", "SolidError", 0x00000000L );
	}
}


void SBrush::Selection_SplitFace()
{
	if(! (m_selectionType == SO_ELEM_FACE || m_selectionType == SO_ELEM_POLYGON) )
		return;

	int nFaces = m_BrushFaces.size();
	int nFurtureFaces = nFaces;

	for( int i = 0; i < nFaces; ++i)
	{
		SBrushFace * face = m_BrushFaces[i];
		if( !face->bSelected )
			continue;
		nFurtureFaces += face->pointindexlist.size()*3;
	}

	if( nFurtureFaces >= SBrushCommon::VertexFaceMax-1 )		
	{
		CryMessageBox( "Fail to face split. There are too much vertices.", "Solid Error", 0x00000000L );
		return;
	}

	RecordUndo("Brush, Split Face");	

	SBackupBrushInfo BackupInfo;
	BackupBrush(BackupInfo);
	
	for( int i = 0; i < nFaces; ++i)
	{
		SBrushFace * face = m_BrushFaces[i];

		if (!face->bSelected)
			continue;

		if( face->pointindexlist.empty() || face->triangleidxlist.empty() )
			continue;

		Vec3 midp(0,0,0);
		size_t ptssize = face->pointindexlist.size();
		for( int j = 0; j < ptssize; ++j)
		{
			midp += m_BrushVertices[face->pointindexlist[j]]->pos;
		}
		midp /= ptssize;

		face->CalcBound(m_BrushVertices);

		float height = face->boundbox.GetRadius() * 0.1f;
		Vec3 newp = midp + face->plane.normal * height;

		for( int j = 0; j < ptssize; ++j)
		{
			SBrushFace * resultface	= new SBrushFace;
			SBrushTriangle * resulttriangle	= new SBrushTriangle;

			resultface->matID		= face->matID;
			resultface->texinfo	= face->texinfo;

			SBrushCommon::VertexFaceIndexType newpidx0 = (SBrushCommon::VertexFaceIndexType)m_BrushVertices.size();
			m_BrushVertices.push_back( new SBrushVertex(newp) );

			SBrushCommon::VertexFaceIndexType newpidx1 = (SBrushCommon::VertexFaceIndexType)m_BrushVertices.size();
			m_BrushVertices.push_back( new SBrushVertex( m_BrushVertices[face->pointindexlist[j]]->pos ) );

			SBrushCommon::VertexFaceIndexType newpidx2 = (SBrushCommon::VertexFaceIndexType)m_BrushVertices.size();
			m_BrushVertices.push_back( new SBrushVertex( m_BrushVertices[face->pointindexlist[(j+1)%ptssize]]->pos ) );

			resultface->pointindexlist.push_back(newpidx0);
			resultface->pointindexlist.push_back(newpidx1);
			resultface->pointindexlist.push_back(newpidx2);

			resultface->triangleidxlist.push_back((SBrushCommon::VertexFaceIndexType)m_BrushTriangles.size());
			resultface->plane = SBrushPlane(	m_BrushVertices[newpidx0]->pos,
																				m_BrushVertices[newpidx1]->pos,
																				m_BrushVertices[newpidx2]->pos );
			resulttriangle->SetVertexIndices( m_BrushVertices, 
																				newpidx0, 
																				newpidx1, 
																				newpidx2, 
																				(SBrushCommon::VertexFaceIndexType)m_BrushFaces.size());
			m_BrushFaces.push_back(resultface);
			m_BrushTriangles.push_back(resulttriangle);
		}
	}

	if( DeleteSelectedFace() == false )
	{
		RestoreBrush(BackupInfo);

		// Deleting is a part of the splitting procedure.
		CryMessageBox( "This face cannot be splitted.", "SolidError", 0x00000000L );
	}
}


void SBrush::Selection_FlipFace()
{
	RecordUndo("Brush, Flip Face");	

	for( int i = 0, nFaces(m_BrushFaces.size()); i < nFaces; ++i)
	{
		SBrushFace * pFace = m_BrushFaces[i];

		assert(pFace);

		if (!pFace->bSelected)
			continue;

		pFace->Invert(m_BrushTriangles);
	}
}


bool SBrush::DeleteSelectedFace()
{
	std::vector<SBrushVertex*>		newvertexlist;
	std::vector<SBrushTriangle*>	newtrianglelist;
	std::vector<SBrushFace*>			newfacelist;

	size_t vertexsize		= m_BrushVertices.size();
	size_t trianglesize = m_BrushTriangles.size();
	size_t facesize			= m_BrushFaces.size();

	std::vector<SBrushCommon::VertexFaceIndexType> vertexmap;
	std::vector<SBrushCommon::VertexFaceIndexType> trianglemap;
	std::vector<SBrushCommon::VertexFaceIndexType> facemap;

	vertexmap.resize(vertexsize);
	trianglemap.resize(trianglesize);
	facemap.resize(facesize);

	for( size_t i = 0; i < vertexsize; ++i )
		vertexmap[i] = SBrushCommon::VertexFaceMax;

	for( size_t i = 0; i < trianglesize; ++i )
		trianglemap[i] = SBrushCommon::VertexFaceMax;

	for( size_t i = 0; i < facesize; ++i )
		facemap[i] = SBrushCommon::VertexFaceMax;

	for( size_t i = 0; i < facesize; ++i )
	{
		SBrushFace* face = m_BrushFaces[i];
		if( face->bSelected )
		{
			for( size_t k = 0; k < face->pointindexlist.size(); ++k )
			{
				delete m_BrushVertices[face->pointindexlist[k]];
				m_BrushVertices[face->pointindexlist[k]] = NULL;
			}
			for( size_t k = 0; k < face->triangleidxlist.size(); ++k )
			{
				delete m_BrushTriangles[face->triangleidxlist[k]];
				m_BrushTriangles[face->triangleidxlist[k]] = NULL;
			}
			delete m_BrushFaces[i];
			m_BrushFaces[i] = NULL;
		}
		else
		{
			std::vector<SBrushCommon::VertexFaceIndexType> newfacevertexindexlist;
			std::vector<SBrushCommon::VertexFaceIndexType> newfacetriindexlist;

			size_t newfaceindex = newfacelist.size();

			for( size_t k = 0; k < face->pointindexlist.size(); ++k )
			{
				if( vertexmap[face->pointindexlist[k]] == SBrushCommon::VertexFaceMax )
				{
					vertexmap[face->pointindexlist[k]] = (SBrushCommon::VertexFaceIndexType)newvertexlist.size();
					if( m_BrushVertices[face->pointindexlist[k]] == NULL )
						return false;
					newvertexlist.push_back(m_BrushVertices[face->pointindexlist[k]]);
				}
				newfacevertexindexlist.push_back(vertexmap[face->pointindexlist[k]]);
			}

			for( size_t k = 0; k < face->triangleidxlist.size(); ++k )
			{
				if( trianglemap[face->triangleidxlist[k]] == SBrushCommon::VertexFaceMax )
				{
					int newtriangleindex = (SBrushCommon::VertexFaceIndexType)newtrianglelist.size();
					trianglemap[face->triangleidxlist[k]] = newtriangleindex;
					if( m_BrushTriangles[face->triangleidxlist[k]] == NULL )
						return false;
					newtrianglelist.push_back( m_BrushTriangles[face->triangleidxlist[k]] );
					m_BrushTriangles[face->triangleidxlist[k]]->faceidx = newfaceindex;

					for( int j = 0; j < 3; ++j )
						newtrianglelist[newtriangleindex]->vertexindices[j] = vertexmap[ newtrianglelist[newtriangleindex]->vertexindices[j] ];
				}
				newfacetriindexlist.push_back(trianglemap[face->triangleidxlist[k]]);
			}

			face->pointindexlist	= newfacevertexindexlist;
			face->triangleidxlist	= newfacetriindexlist;

			newfacelist.push_back(face);
		}
	}

	m_BrushVertices		= newvertexlist;
	m_BrushTriangles	= newtrianglelist;
	m_BrushFaces			= newfacelist;	

	ComputeAdjacentTriangle();
	ComputeFacePlanes();
	ComputeTexCoord();
	ComputeBoundBox();

	Invalidate();

	return true;
}

void SBrush::Selection_DeleteFace()
{
	SBackupBrushInfo BackupInfo;
	BackupBrush(BackupInfo);

	if( DeleteSelectedFace() == false )
	{
		RestoreBrush(BackupInfo);
		CryMessageBox( "This face cannot be deleted.", "SolidError", 0x00000000L );
	}
}