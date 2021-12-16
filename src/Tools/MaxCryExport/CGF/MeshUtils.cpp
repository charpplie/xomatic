//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MeshUtils.h"

void MeshUtils::CalcVertexNormals(Tab<VNormal> &vnorms, Mesh *mesh, BOOL ConsiderSmoothing, BOOL negate, Point3* pVertexSubstitute)
{
	int nVerts = mesh->getNumVerts();
	int nFaces = mesh->getNumFaces();
	Face		*Faces	  = mesh->faces;
	Point3		*Verts	  = pVertexSubstitute?pVertexSubstitute:mesh->verts;

	//calculate vertex normals
	vnorms.SetCount(nVerts);
	memset (&vnorms[0],0, nVerts*sizeof(VNormal));
	// Compute face and vertex surface normals
	for (int i = 0; i < nVerts; i++) vnorms[i] = VNormal();	//init

	for (i = 0; i < nFaces; i++) 
	{
		// Calculate the surface normal
		Point3 v0, v1, v2;
		v0 = Verts[Faces[i].v[0]];
		v1 = Verts[Faces[i].v[1]];
		v2 = Verts[Faces[i].v[2]];
		Point3 fnorm = negate ? (v2-v1)^(v1-v0) : (v1-v0)^(v2-v1);
		for (int j=0; j<3; j++) 
		{		
			vnorms[Faces[i].v[j]].AddNormal(fnorm,ConsiderSmoothing?Faces[i].smGroup:1);
		}
	}

	for (i=0; i < nVerts; i++) 
	{
		vnorms[i].Normalize();
	}
}
