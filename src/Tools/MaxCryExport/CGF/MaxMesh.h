//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXMESH_H__
#define __MAXMESH_H__

#include "ISourceMesh.h"
#include "CryHeaders.h"
#include "NameList.h"
#include <vector>

class MaxSkinningInfo;
class MaxMorphData;
class MaxMaterial;

class IErrorReporter;

class MaxMesh : public ISourceMesh
{
public:
	MaxMesh(IErrorReporter* pErrorReporter, INode* pNode, Mesh* pMaxMesh, MaxMaterial* pMaterial, const Matrix3& transform, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, float fMorphMinOffset);
	virtual ~MaxMesh();

	// ISourceMesh
	virtual int UVCount();
	virtual CryUV* GetUVs();
	virtual int TextureFaceCount();
	virtual CryTexFace* GetTextureFaces();
	virtual int VertexCount();
	virtual CryVertex* GetVertices();
	virtual int FaceCount();
	virtual CryFace* GetFaces();
	virtual bool HasVertexColours();
	virtual CryIRGB* GetVertexColours();
	virtual bool HasVertexAlpha();
	virtual unsigned char* GetVertexAlpha();
	virtual ISkinningInfo* GetSkinningInfo();
	virtual IMorphData* GetMorphData();
	virtual Point3* GetVertsForBoneSavingRoutines();
	virtual int* GetVertsForBoneSavingRoutinesMap();
	virtual int GetVertexForBoneSavingRoutinesMapSize();
	virtual int* GetVertMap();
	virtual int GetVertMapSize();

	static MaxMesh* Create(IErrorReporter* pErrorReporter, INode* pMaxNode, MaxMaterial* pMaterial, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, float fMorphMinOffset);

private:
	void ReadMesh(IErrorReporter* pErrorReporter, Mesh* pMesh, MaxMaterial* pMaterial, bool bGenerateDefaultUVs);
	void PrepareVertexColours(Mesh* pMesh);
	static void SplitSmoothGroups(Mesh &mesh,Mesh &outMesh,std::vector<int> &arrVertexMap,BOOL negate, Point3* pActualVerts);
	static void GenerateDefaultUVs(Mesh& mesh);
	void CreateSkinningInfo(bool bAllowBlending, NameList& BoneList);
	void CreateMorphData(float fMorphMinOffset);

	std::vector<CryUV> uvs;
	std::vector<CryTexFace> textureFaces;
	std::vector<CryVertex> vertices;
	std::vector<CryFace> faces;
	std::vector<CryIRGB> vertexColours;
	std::vector<unsigned char> vertexAlpha;
	Matrix3 transform;
	bool bHasVertexColours;
	bool bHasVertexAlpha;
	INode* pMaxNode;
	std::vector<Point3> vertsForBoneSavingRoutines;
	std::vector<int> vertsForBoneSavingRoutinesMap;
	std::vector<int> vertMap;
	MaxSkinningInfo* pSkinningInfo;
	MaxMorphData* pMorphData;
};

#endif //__MAXMESH_H__
