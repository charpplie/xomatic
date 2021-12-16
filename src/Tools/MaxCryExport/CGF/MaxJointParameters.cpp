//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxJointParameters.h"
#include "interpik.h"

MaxJointParameters::MaxJointParameters(JointParams* pMaxParams)
:	pMaxParams(pMaxParams)
{
}

MaxJointParameters::~MaxJointParameters ()
{
}

unsigned int MaxJointParameters::GetFlags()
{
	return (~this->pMaxParams->flags & JNT_XACTIVE * 7);
}

int MaxJointParameters::DegreeOfFreedomCount()
{
	return this->pMaxParams->dofs;
}

void MaxJointParameters::GetDegreeOfFreedomInfo(int nDegreeOfFreedom, DegreeOfFreedomInfo& dof)
{
	if (this->pMaxParams->flags & JNT_XLIMITED << nDegreeOfFreedom)
	{
		dof.fMin = this->pMaxParams->min[nDegreeOfFreedom];
		dof.fMax = this->pMaxParams->max[nDegreeOfFreedom];
	}
	else
	{
		dof.fMin = -1E10;
		dof.fMax = 1E10;
	}
	dof.fSpringAngle = this->pMaxParams->spring[nDegreeOfFreedom];
	dof.fSpringTension = this->pMaxParams->stens[nDegreeOfFreedom] * 50.0f;
	dof.fDamping = this->pMaxParams->damping[nDegreeOfFreedom];
}

JointParams* MaxJointParameters::GetMaxJointParams()
{
	return this->pMaxParams;
}
