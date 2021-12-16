//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __VERTEXCOLORS_H__
#define __VERTEXCOLORS_H__

namespace VertexColors
{
	void GetVertexColorsForMesh(std::vector<CryIRGB>& colors, Mesh& mesh);
	void GetVertexAlphasForMesh(std::vector<unsigned char>& alphas, Mesh& mesh);
	void GetVertexColorsFromChannel(std::vector<CryIRGB>& colors, Mesh& mesh, int channel);
	void GetVertexAlphasFromChannel(std::vector<unsigned char>& alphas, Mesh& mesh, int channel);
}

#endif //__VERTEXCOLORS_H__
