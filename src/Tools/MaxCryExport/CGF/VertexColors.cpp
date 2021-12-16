//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "VertexColors.h"

void VertexColors::GetVertexColorsForMesh(std::vector<CryIRGB>& colors, Mesh& mesh)
{
	// [MichaelS 24/11/2006] Vertex colours for a node can either be accessed via dedicated
	// functions or via the generalized map channels, in which case the map index to use is 0.
	GetVertexColorsFromChannel(colors, mesh, 0);
}

void VertexColors::GetVertexAlphasForMesh(std::vector<unsigned char>& alphas, Mesh& mesh)
{
	// [MichaelS 24/11/2006] Vertex alphas, unlike colours, have no special means of access and
	// are quite separate from the colours. They have clearly been workarounded in most egregiously.
	GetVertexAlphasFromChannel(alphas, mesh, MAP_ALPHA);
}

void VertexColors::GetVertexColorsFromChannel(std::vector<CryIRGB>& colors, Mesh& mesh, int channel)
{
	colors.clear();

	if (!mesh.mapSupport(channel))
		return;

	int nVerts = mesh.getNumVerts();
	int nFaces = mesh.getNumFaces();
	int nCVerts = mesh.getNumMapVerts(channel);
	VertColor* ColVerts = mesh.mapVerts(channel);
	TVFace* ColFaces = mesh.mapFaces(channel);
	Face* Faces = mesh.faces;

	if (!ColVerts || !ColFaces || !Faces)
		return;

	int* count = (int*)calloc(nVerts,sizeof(int));
	VertColor* fcols = (VertColor*)calloc(nVerts,sizeof(VertColor));

	if (!count)
		return;
	if (!fcols)
		return;

	if (ColVerts)
	{
		if (!(nCVerts && nFaces && nVerts))
			return;

		for(int i=0;i<nFaces;i++)
		{
			int v0=Faces[i].v[0];
			int v1=Faces[i].v[1];
			int v2=Faces[i].v[2];

			VertColor c0=ColVerts[ColFaces[i].t[0]];
			VertColor c1=ColVerts[ColFaces[i].t[1]];
			VertColor c2=ColVerts[ColFaces[i].t[2]];

			fcols[v0]+=c0; count[v0]++;
			fcols[v1]+=c1; count[v1]++;
			fcols[v2]+=c2; count[v2]++;
		}
	}
	else
	{
		if (!(nVerts))
			return;

		// We may be forced to write out vertex colours, even if none are defined in the object.
		// In this case, write out all white colours.
		for (int nVertex = 0; nVertex < nVerts; ++nVertex)
		{
			fcols[nVertex].x = 1.0f;
			fcols[nVertex].y = 1.0f;
			fcols[nVertex].z = 1.0f;
			count[nVertex] = 1;
		}
	}

	colors.resize(nVerts);
	for(int i=0;i<nVerts;i++)
	{
		if(count[i])
		{
			colors[i].r = int(fcols[i].x*255/count[i]);
			colors[i].g = int(fcols[i].y*255/count[i]);
			colors[i].b = int(fcols[i].z*255/count[i]);
		}
		else
		{
			colors[i].r = colors[i].g = colors[i].b = 0;
		}
	}

	free(count);
	free(fcols);
}

void VertexColors::GetVertexAlphasFromChannel(std::vector<unsigned char>& alphas, Mesh& mesh, int channel)
{
	alphas.clear();

	if (!mesh.mapSupport(channel))
		return;

	int nVerts = mesh.getNumVerts();
	int nFaces = mesh.getNumFaces();
	int nCVerts = mesh.getNumMapVerts(channel);
	UVVert* AlphaVerts = mesh.mapVerts(channel);
	TVFace* AlphaFaces = mesh.mapFaces(channel);
	Face* Faces = mesh.faces;

	if (!AlphaVerts || !AlphaFaces || !Faces)
		return;

	int* count = (int*)calloc(nVerts,sizeof(int));
	UVVert* falphas = (VertColor*)calloc(nVerts,sizeof(VertColor));

	if (!count)
		return;
	if (!falphas)
		return;

	if (AlphaVerts)
	{
		if (!(nCVerts && nFaces && nVerts))
			return;

		for(int i=0;i<nFaces;i++)
		{
			int v0=Faces[i].v[0];
			int v1=Faces[i].v[1];
			int v2=Faces[i].v[2];

			UVVert c0=AlphaVerts[AlphaFaces[i].t[0]];
			UVVert c1=AlphaVerts[AlphaFaces[i].t[1]];
			UVVert c2=AlphaVerts[AlphaFaces[i].t[2]];

			falphas[v0]+=c0; count[v0]++;
			falphas[v1]+=c1; count[v1]++;
			falphas[v2]+=c2; count[v2]++;
		}
	}
	else
	{
		if (!(nVerts))
			return;

		// We may be forced to write out vertex colours, even if none are defined in the object.
		// In this case, write out all white colours.
		for (int nVertex = 0; nVertex < nVerts; ++nVertex)
		{
			falphas[nVertex].x = 1.0f;
			falphas[nVertex].y = 1.0f;
			falphas[nVertex].z = 1.0f;
			count[nVertex] = 1;
		}
	}

	alphas.resize(nVerts);
	for(int i=0;i<nVerts;i++)
	{
		if(count[i])
			alphas[i] = int(falphas[i].x*255/count[i]);
		else
			alphas[i] = 0;
	}

	free(count);
	free(falphas);
}
