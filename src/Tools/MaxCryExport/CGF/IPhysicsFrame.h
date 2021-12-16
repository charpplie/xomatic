//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IPHYSICSFRAME_H__
#define __IPHYSICSFRAME_H__

class IPhysicsFrame
{
public:
	virtual ~IPhysicsFrame() {}

	virtual void ReadFrameMatrix(float fMatrix[3][3]) = 0;
};

#endif //__IPHYSICSFRAME_H__
