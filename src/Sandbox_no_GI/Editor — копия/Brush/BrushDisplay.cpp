///////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   BrushDisplay.cpp
//  Version:     v1.00
//  Created:     5/5/2010 by Jaesik.
//  Compilers:   Visual Studio.NET
//  Description: 
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Brush.h"
#include "BrushFace.h"
#include "Objects\SubObjSelection.h"
#include "SolidBrushObject.h"
#include "DisplaySettings.h"
#include "Viewport.h"

void SBrush::Display( DisplayContext &dc )
{
	bool b2D = false;
	const int NormalThickness = 2;
	const int HightlightedThickness = 12;
	const ColorB orangecolor(255,75,0,255);	// orange color

	dc.PushMatrix(m_matrix);

	bool bSelected = (m_nFlags & BRF_SELECTED) != 0;

	float OldThickness = dc.GetLineWidth();
	if( m_Owner && m_Owner->IsHighlighted() )
		dc.SetLineWidth(HightlightedThickness);
	else 
		dc.SetLineWidth(NormalThickness);

	dc.SetFillMode( e_FillModeSolid );	
	dc.SetColor(orangecolor);

	if( dc.flags & DISPLAY_2D )
	{
		b2D = true;
		int cullAxis = 0;
		EViewportType viewType = dc.view->GetType();

		switch( viewType )
		{
		case ET_ViewportXY:
			cullAxis = 2;
			break;

		case ET_ViewportXZ:
			cullAxis = 1;
			break;

		case ET_ViewportYZ:
			cullAxis = 0;
			break;
		}	

		for( size_t i = 0; i < m_BrushFaces.size(); ++i )
		{
			SBrushFace* f = m_BrushFaces[i];

			Vec3 norm = m_matrix.TransformVector( f->plane.normal );

			if( norm[cullAxis] <= 0 )
				continue;

			size_t numv = f->pointindexlist.size();

			for( size_t j = 0; j < f->pointindexlist.size(); ++j)
			{
				int k = ((j+1) < numv) ? j+1 : 0;
				dc.DrawLine( m_BrushVertices[f->pointindexlist[j]]->pos, m_BrushVertices[f->pointindexlist[k]]->pos );
			}
		}
	}
	else if( !m_BrushFaces.empty() )
	{
		if( !b2D )
			dc.SetDrawInFrontMode(true);

		for( size_t i = 0; i < m_BrushFaces.size(); ++i )
		{
			SBrushFace* face = m_BrushFaces[i];
			size_t numv = face->pointindexlist.size();

			for( size_t j = 0; j < numv; ++j )
			{
				int k = ((j+1) < numv) ? j+1 : 0;
				const Vec3 &p0 = m_BrushVertices[face->pointindexlist[j]]->pos;
				const Vec3 &p1 = m_BrushVertices[face->pointindexlist[k]]->pos;
				dc.DrawLine( p0, p1 );			
			}
		}
	}

	if (bSelected)
		dc.SetLineWidth(0);

	dc.PopMatrix();
	dc.SetDrawInFrontMode(false);

	if( m_Owner && m_Owner->IsHighlighted() )
		dc.SetLineWidth(OldThickness);

	if( !(m_nFlags&BRF_SUB_OBJ_SEL) || !GetEditModeFlag() )
	{
		return;
	}

	uint32 nPrevState = dc.GetState();

	if (m_pStatObj)
	{
		int nStatObjFlags = m_pStatObj->GetFlags();
		if (g_SubObjSelOptions.displayType != SO_DISPLAY_GEOMETRY)
			nStatObjFlags |= STATIC_OBJECT_HIDDEN;
		m_pStatObj->SetFlags( nStatObjFlags );
	}

	const Matrix34 &worldTM = m_matrix;
	Matrix34 invWorldTM		= worldTM.GetInverted();
	Vec3 vWSCameraVector	= worldTM.GetTranslation() - dc.view->GetViewTM().GetTranslation();
	Vec3 vOSCameraVector	= invWorldTM.TransformVector(vWSCameraVector).GetNormalized();
	Vec3 vOSCameraPos			= invWorldTM.TransformPoint(dc.view->GetViewTM().GetTranslation());

	dc.PushMatrix( worldTM );

	if (g_SubObjSelOptions.displayType == SO_DISPLAY_FLAT)
	{
		ColorB faceColor(0,250,250,255);
		ColorB col = faceColor;

		if( !b2D )
			dc.SetDrawInFrontMode(false);

		dc.SetDrawInFrontMode(false);
		dc.SetFillMode( e_FillModeSolid );
		dc.CullOn();
		for (int i = 0; i < m_BrushFaces.size(); ++i)
		{
			SBrushFace * face = m_BrushFaces[i];
			if ((m_selectionType != SO_ELEM_FACE && m_selectionType != SO_ELEM_POLYGON) || !face->bSelected)
			{
				ColorB col = faceColor;
				float dt = -face->plane.normal.Dot(vOSCameraVector);
				dt = max(0.4f,dt);
				dt = min(1.0f,dt);
				col.r = ftoi(faceColor.r*dt);
				col.g = ftoi(faceColor.g*dt);
				col.b = ftoi(faceColor.b*dt);
				col.a = faceColor.a;
				dc.SetColor(col);

				for( size_t k = 0; k < face->triangleidxlist.size(); ++k )
				{
					SBrushTriangle * tri = m_BrushTriangles[face->triangleidxlist[k]];
					dc.DrawTri( m_BrushVertices[tri->vertexindices[0]]->pos,
											m_BrushVertices[tri->vertexindices[1]]->pos,
											m_BrushVertices[tri->vertexindices[2]]->pos );
				}
			}
		}
	}

	ColorB edgeColor(255,255,255,155);
	if (m_selectionType != SO_ELEM_EDGE)
	{
		if (g_SubObjSelOptions.bDisplayBackfacing)
			dc.CullOff();
		else
			dc.CullOn();

		if( !b2D )
			dc.SetDrawInFrontMode(true);

		dc.SetDrawInFrontMode(true);
		dc.SetFillMode( e_FillModeWireframe );

		for( int i = 0; i < m_BrushFaces.size(); ++i )
		{
			dc.SetColor( edgeColor );
			SBrushFace * face = m_BrushFaces[i];
			if( !face->bSelected )
			{
				size_t numv = face->pointindexlist.size();
				for( size_t j = 0; j < numv; ++j)
				{
					int k = ((j+1) < (int)numv) ? j+1 : 0;
					SBrushVertex &p0 = *m_BrushVertices[face->pointindexlist[j]];
					SBrushVertex &p1 = *m_BrushVertices[face->pointindexlist[k]];
					dc.DrawLine( p0.pos, p1.pos );
				}
			}
		}
	}

	if (g_SubObjSelOptions.bDisplayNormals)
	{
		dc.SetColor(edgeColor);
		for (int i = 0; i < m_BrushFaces.size(); ++i)
		{
			SBrushFace * face = m_BrushFaces[i];
			face->CalcCenter(m_BrushVertices);
			dc.DrawLine(face->center,face->center+face->plane.normal*g_SubObjSelOptions.fNormalsLength,RGB(0,255,255),RGB(255,255,255) );
		}
	}

	if( m_selectionType == SO_ELEM_VERTEX )
	{
		ColorB pointColor(0,255,255,255);
		ColorB pointColorSelected(255,0,0,255);
		dc.SetColor(pointColor);

		for (int i = 0; i < m_BrushFaces.size(); ++i)
		{
			SBrushFace * face = m_BrushFaces[i];

			for( int j = 0; j < face->pointindexlist.size(); ++j )
			{
				SBrushVertex &vert = *m_BrushVertices[face->pointindexlist[j]];

				if( !g_SubObjSelOptions.bDisplayBackfacing && (vert.pos-vOSCameraPos).Dot(face->plane.normal) > 0)
					continue;

				if (vert.bSelected)
					dc.SetColor(pointColorSelected);
				dc.DrawPoint( vert.pos,4 );
				if (vert.bSelected)
					dc.SetColor(pointColor);
			}
		}
	}
	else if( m_selectionType == SO_ELEM_EDGE )
	{
		ColorB edgeColor(200,255,200,255);
		ColorB selEdgeColor(255,0,0,255);

		dc.CullOff();
		dc.SetFillMode( e_FillModeSolid );

		size_t	nTotalFaces(m_BrushFaces.size());
		size_t	nCurrentFace(0);
		int			nFirstIndex(0),nSecondIndex(0);
		int			nCurrentEdge(0);

		for (nCurrentFace=0;nCurrentFace<nTotalFaces;++nCurrentFace)
		{
			SBrushFace	&roCurrentFace=*m_BrushFaces[nCurrentFace];
			if( !g_SubObjSelOptions.bDisplayBackfacing && vOSCameraVector.Dot(roCurrentFace.plane.normal) > 0 )
			{
				continue;
			}

			for (nCurrentEdge=0;nCurrentEdge<GetNumberOfFaceEdges(&roCurrentFace);++nCurrentEdge)
			{
				roCurrentFace.MapEdgeIndexToPolyIndices( m_BrushVertices, m_BrushTriangles, nCurrentEdge, nFirstIndex, nSecondIndex );
				if (roCurrentFace.selectededge&(1<<nCurrentEdge))
				{
					dc.SetColor(selEdgeColor);
				}
				else
				{
					dc.SetColor(edgeColor);
				}
				dc.DrawLine(m_BrushVertices[nFirstIndex]->pos,m_BrushVertices[nSecondIndex]->pos);
			}
		}
	}
	else if (m_selectionType == SO_ELEM_FACE || m_selectionType == SO_ELEM_POLYGON)
	{
		ColorB pointColor(0,255,255,255);
		ColorB selFaceColor(255,0,0,255);
		if (g_SubObjSelOptions.displayType != SO_DISPLAY_FLAT)
		{
			selFaceColor.a = 100;
		}

		dc.CullOff();
		dc.SetFillMode( e_FillModeSolid );
		dc.SetColor(selFaceColor);

		for( int i = 0; i < m_BrushFaces.size(); ++i )
		{
			SBrushFace * face = m_BrushFaces[i];
			if (face->bSelected)
			{
				dc.SetColor(selFaceColor);
				for( size_t j = 0; j < face->triangleidxlist.size(); ++j )
				{
					SBrushTriangle* tri = m_BrushTriangles[face->triangleidxlist[j]];
					dc.DrawTri( m_BrushVertices[tri->vertexindices[0]]->pos, 
											m_BrushVertices[tri->vertexindices[1]]->pos,
											m_BrushVertices[tri->vertexindices[2]]->pos );
				}
			}

			if (!g_SubObjSelOptions.bDisplayBackfacing && vOSCameraVector.Dot(face->plane.normal) > 0)
				continue;

			dc.SetColor(pointColor);
			face->CalcCenter(m_BrushVertices);
			dc.DrawPoint( face->center, 4 );
		}
	}

	dc.SetDrawInFrontMode(false);
	dc.PopMatrix();
	dc.SetState(nPrevState);
}