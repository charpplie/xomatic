//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXJOINTPARAMETERS_H__
#define __MAXJOINTPARAMETERS_H__

#include "IJointParameters.h"

class MaxJointParameters : public IJointParameters
{
public:
	MaxJointParameters(JointParams* pMaxParams);
	virtual ~MaxJointParameters ();

	virtual unsigned int GetFlags();
	virtual int DegreeOfFreedomCount();
	virtual void GetDegreeOfFreedomInfo(int nDegreeOfFreedom, DegreeOfFreedomInfo& dof);

	virtual JointParams* GetMaxJointParams();

private:
	JointParams* pMaxParams;
};

#endif //__MAXJOINTPARAMETERS_H__
