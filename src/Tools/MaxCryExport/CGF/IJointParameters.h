//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IJOINTPARAMETERS_H__
#define __IJOINTPARAMETERS_H__

class JointParams;

class IJointParameters
{
public:
	class DegreeOfFreedomInfo
	{
	public:
		float fMin;
		float fMax;
		float fSpringAngle;
		float fSpringTension;
		float fDamping;
	};

	virtual ~IJointParameters() {}

	virtual unsigned int GetFlags() = 0;
	virtual int DegreeOfFreedomCount() = 0;
	virtual void GetDegreeOfFreedomInfo(int nDegreeOfFreedom, DegreeOfFreedomInfo& dof) = 0;

	// TEMPORARY workaround
	virtual JointParams* GetMaxJointParams() = 0;
};

#endif //__IJOINTPARAMETERS_H__
