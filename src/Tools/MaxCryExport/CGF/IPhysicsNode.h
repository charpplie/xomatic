//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IPHYSICSNODE_H__
#define __IPHYSICSNODE_H__

#include <string>

class IJointParameters;
class IPhysicsFrame;

class IPhysicsNode
{
public:
	virtual ~IPhysicsNode() {}

	virtual std::string GetUserProperty() = 0;
	virtual IJointParameters* GetJointParameters() = 0;
	virtual IPhysicsFrame* GetPhysicsFrame() = 0;
};

#endif //__IPHYSICSNODE_H__
