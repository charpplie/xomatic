//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXBREAKABLEPHYSICSINFO_H__
#define __MAXBREAKABLEPHYSICSINFO_H__

#include "IBreakablePhysicsInfo.h"

class MaxBreakablePhysicsInfo : public IBreakablePhysicsInfo
{
public:
	MaxBreakablePhysicsInfo(int nGranularity);
	virtual ~MaxBreakablePhysicsInfo();

	virtual int GetGranularity();

private:
	int nGranularity;
};

#endif //__MAXBREAKABLEPHYSICSINFO_H__
