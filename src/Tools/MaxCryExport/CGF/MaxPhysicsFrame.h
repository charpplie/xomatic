//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXPHYSICSFRAME_H__
#define __MAXPHYSICSFRAME_H__

#include "IPhysicsFrame.h"

class MaxPhysicsFrame : public IPhysicsFrame
{
public:
	MaxPhysicsFrame(INode* pFrameNode, INode* pFrameParentNode);
	virtual ~MaxPhysicsFrame();

	virtual void ReadFrameMatrix(float fMatrix[3][3]);

private:
	INode* pFrameNode;
	INode* pFrameParentNode;
};

#endif //__MAXPHYSICSFRAME_H__
