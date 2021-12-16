//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxPhysicsNode.h"
#include "MaxJointParameters.h"
#include "MaxPhysicsFrame.h"

MaxPhysicsNode::MaxPhysicsNode(const std::string& sUserProperty, IJointParameters* pJointParameters, IPhysicsFrame* pPhysicsFrame)
:	sUserProperty(sUserProperty),
	pJointParameters(pJointParameters),
	pPhysicsFrame(pPhysicsFrame)
{
}

MaxPhysicsNode::~MaxPhysicsNode()
{
	delete this->pJointParameters;
	delete this->pPhysicsFrame;
}

std::string MaxPhysicsNode::GetUserProperty()
{
	return this->sUserProperty;
}

IJointParameters* MaxPhysicsNode::GetJointParameters()
{
	return this->pJointParameters;
}

IPhysicsFrame* MaxPhysicsNode::GetPhysicsFrame()
{
	return pPhysicsFrame;
}
