//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __ISOURCEMESH_H__
#define __ISOURCEMESH_H__

#include "CryHeaders.h"

class ISkinningInfo;
class IMorphData;

class ISourceMesh
{
public:
	virtual ~ISourceMesh() {}

	virtual int UVCount() = 0;
	virtual CryUV* GetUVs() = 0;
	virtual int TextureFaceCount() = 0;
	virtual CryTexFace* GetTextureFaces() = 0;
	virtual int VertexCount() = 0;
	virtual CryVertex* GetVertices() = 0;
	virtual int FaceCount() = 0;
	virtual CryFace* GetFaces() = 0;
	virtual bool HasVertexColours() = 0;
	virtual CryIRGB* GetVertexColours() = 0;
	virtual bool HasVertexAlpha() = 0;
	virtual unsigned char* GetVertexAlpha() = 0;
	virtual ISkinningInfo* GetSkinningInfo() = 0;
	virtual IMorphData* GetMorphData() = 0;

	// TEMPORARY workaround - REMOVE ONCE CODE IS SUFFICIENTLY REFACTORED.
	virtual Point3* GetVertsForBoneSavingRoutines() = 0;
	virtual int* GetVertsForBoneSavingRoutinesMap() = 0;
	virtual int GetVertexForBoneSavingRoutinesMapSize() = 0;
	virtual int* GetVertMap() = 0;
	virtual int GetVertMapSize() = 0;
};

#endif //__ISOURCEMESH_H__
