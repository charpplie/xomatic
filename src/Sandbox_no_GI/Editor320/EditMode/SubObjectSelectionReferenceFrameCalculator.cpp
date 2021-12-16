////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
//  File name:   SubObjectSelectionReferenceFrameCalculator.cpp
//  Version:     v1.00
//  Created:     9/3/2006 Michael Smith
//  Compilers:   Visual Studio.NET 2005
//  Description: Calculate the reference frame for sub-object selections.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SubObjectSelectioNReferenceFrameCalculator.h"
#include "Brush/Brush.h"

SubObjectSelectionReferenceFrameCalculator::SubObjectSelectionReferenceFrameCalculator(ESubObjElementType selectionType)
:	bAnySelected(false),
	pos(0.0f, 0.0f, 0.0f),
	normal(0.0f, 0.0f, 0.0f),
	nNormals(0),
	selectionType(selectionType),
	bUseExplicitFrame(false),
	bExplicitAnySelected(false)
{
}

void SubObjectSelectionReferenceFrameCalculator::AddBrush(const Matrix34& worldTM, SBrush* pBrush)
{
	if (this->selectionType == SO_ELEM_VERTEX)
	{
		for (int i = 0; i < pBrush->m_Faces.size(); i++)
		{
			SBrushFace &face = *pBrush->m_Faces[i];
			SBrushPoly *poly = face.m_Poly;
			if (!poly)
				continue;
			for (int j = 0; j < poly->m_Pts.size(); j++)
			{
				SBrushVert &vert = poly->m_Pts[j];
				if (vert.bSelected)
				{
					Vec3 worldPosition = worldTM * vert.xyz;
					if (std::find(this->positions.begin(),this->positions.end(),worldPosition) == this->positions.end())
					{
						this->positions.push_back(worldPosition);
						// This position is not selected yet.
						this->bAnySelected = true;
						this->normal = this->normal - face.m_Plane.normal;
						this->nNormals++;
						this->pos = this->pos + worldPosition;
					}
				}
			}
		}
	}

	if (this->selectionType == SO_ELEM_EDGE)
	{
		SBrushPoly*	poBrushPoly(NULL);
		int         nNumberOfFaces(pBrush->m_Faces.size());
		int			nCurrentFace(0);
		int			nCurrrentEdge(0);
		int			anPolyIndices[2];

		for (nCurrentFace=0;nCurrentFace<nNumberOfFaces;nCurrentFace++)
		{
			SBrushFace& roCurrentFace=*pBrush->m_Faces[nCurrentFace];
			poBrushPoly=roCurrentFace.m_Poly;
			if (!poBrushPoly)
			{
				continue;
			}

			for (nCurrrentEdge=0;nCurrrentEdge<roCurrentFace.m_nNumberOfEdges;nCurrrentEdge++)
			{	
				if (roCurrentFace.m_nSelectedEdges&(1<<nCurrrentEdge))
				{
					roCurrentFace.MapEdgeIndexToPolyIndices(nCurrrentEdge,anPolyIndices[0],anPolyIndices[1]);

					this->bAnySelected = true;

					// Avarages all face normals.
					// OBS: the current algorithm is not a good one to estime the correct normals.
					// Spherical or quaternion interpolation should be used instead of arithmetic avarage.
					this->normal = this->normal + roCurrentFace.m_Plane.normal;
					nNormals++;

					// As we just need a position where to place the gizmo, we don't need to refer to
					// both faces in order to find the correct position. We just need to place between any
					// two selected vertices (an edge means 2 vertices).
					this->pos=
						poBrushPoly->m_Pts[anPolyIndices[0]].xyz
						+
						// This term calculates the vector from the second vertex to the first vertex of
						// the edge. As we want to place the gizmo between them, we must divide it's lenght
						// by two.
						(
						poBrushPoly->m_Pts[anPolyIndices[1]].xyz
						-
						poBrushPoly->m_Pts[anPolyIndices[0]].xyz
						)
						/
						2.0f;

					this->pos=worldTM.TransformPoint(this->pos)*nNormals;
				}
			}
		}

		if (nNormals > 0)
		{
			this->normal = this->normal / nNormals;
			if (!this->normal.IsZero())
			{
				this->normal.Normalize();
			}
		}
	}

	if (this->selectionType == SO_ELEM_FACE || this->selectionType == SO_ELEM_POLYGON)
	{
		// Average all face normals.
		for (int i = 0; i < pBrush->m_Faces.size(); i++)
		{
			SBrushFace &face = *pBrush->m_Faces[i];
			if (face.m_bSelected)
			{
				this->bAnySelected = true;
				this->normal = this->normal - face.m_Plane.normal;
				this->nNormals++;
				face.CalcCenter();
				this->pos = this->pos + worldTM * face.m_vCenter;
			}
		}
	}
}

void SubObjectSelectionReferenceFrameCalculator::SetExplicitFrame(bool bAnySelected, const Matrix34& refFrame)
{
	this->refFrame = refFrame;
	this->bUseExplicitFrame = true;
	this->bExplicitAnySelected = bAnySelected;
}

bool SubObjectSelectionReferenceFrameCalculator::GetFrame(Matrix34& refFrame)
{
	if (this->bUseExplicitFrame)
	{
		refFrame = this->refFrame;
		return this->bExplicitAnySelected;
	}
	else
	{
		refFrame.SetIdentity();

		if (this->nNormals > 0)
		{
			this->normal = this->normal / this->nNormals;
			if (!this->normal.IsZero())
				this->normal.Normalize();

			// Average position.
			this->pos = this->pos / this->nNormals;
			refFrame.SetTranslation(this->pos);
		}

		if (this->bAnySelected)
		{
			if (!this->normal.IsZero())
			{
				Vec3 xAxis(1,0,0),yAxis(0,1,0),zAxis(0,0,1);
				if (this->normal.IsEquivalent(zAxis) || normal.IsEquivalent(-zAxis))
					zAxis = xAxis;
				xAxis = this->normal.Cross(zAxis).GetNormalized();
				yAxis = xAxis.Cross(this->normal).GetNormalized();
				refFrame.SetFromVectors( xAxis,yAxis,normal,pos );
			}
		}

		return bAnySelected;
	}
}
