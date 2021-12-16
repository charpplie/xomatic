//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IVERTEXANIMATION_H__
#define __IVERTEXANIMATION_H__

class IVertexAnimationFrame
{
public:
	virtual ~IVertexAnimationFrame() {}

	virtual int GetNumVertices() = 0;
	virtual int GetNumFaces() = 0;
	virtual Point3* GetVertices() = 0;
	virtual Face* GetFaces() = 0;
};

class IVertexAnimation
{
public:
	virtual ~IVertexAnimation() {}

	virtual int GetNumVerts() = 0;
	virtual int GetNumFaces() = 0;
	virtual IVertexAnimationFrame* GetFrame(TimeValue time) = 0;
};

#endif //__IVERTEXANIMATION_H__
