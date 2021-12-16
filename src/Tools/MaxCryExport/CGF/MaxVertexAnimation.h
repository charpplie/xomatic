//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXVERTEXANIMATION_H__
#define __MAXVERTEXANIMATION_H__

#include "IVertexAnimation.h"
#include <vector>

class MaxVertexAnimationFrame : public IVertexAnimationFrame
{
public:
	MaxVertexAnimationFrame(Mesh* pMesh);
	virtual ~MaxVertexAnimationFrame();

	virtual int GetNumVertices();
	virtual int GetNumFaces();
	virtual Point3* GetVertices();
	virtual Face* GetFaces();

private:
	std::vector<Point3> vertices;
	std::vector<Face> faces;
};

class MaxVertexAnimation : public IVertexAnimation
{
public:
	MaxVertexAnimation(INode* pNode);
	virtual ~MaxVertexAnimation();

	virtual int GetNumVerts();
	virtual int GetNumFaces();
	virtual IVertexAnimationFrame* GetFrame(TimeValue time);

private:
	void ReadNode();

	INode* pNode;
	int nNumVerts;
	int nNumFaces;
};

#endif //__MAXVERTEXANIMATION_H__
