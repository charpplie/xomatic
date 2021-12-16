//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXPHYSICSNODE_H__
#define __MAXPHYSICSNODE_H__

#include "IPhysicsNode.h"
class IJointParameters;

class MaxPhysicsNode : public IPhysicsNode
{
public:
	MaxPhysicsNode(const std::string& sUserProperty, IJointParameters* pJointParameters, IPhysicsFrame* pPhysicsFrame);
	~MaxPhysicsNode();

	virtual std::string GetUserProperty();
	virtual IJointParameters* GetJointParameters();
	virtual IPhysicsFrame* GetPhysicsFrame();

private:
	std::string sUserProperty;
	IJointParameters* pJointParameters;
	IPhysicsFrame* pPhysicsFrame;
};

#endif //__MAXPHYSICSNODE_H__
