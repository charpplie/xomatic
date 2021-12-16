//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MESHUTILS_H__
#define __MESHUTILS_H__

#include "VNormal.h"

namespace MeshUtils
{
	void CalcVertexNormals(Tab<VNormal> &vnorms, Mesh *mesh, BOOL ConsiderSmoothing, BOOL negate = false, Point3* pVertexSubstitute = NULL);
}

#endif //__MESHUTILS_H__
