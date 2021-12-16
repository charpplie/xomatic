//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IBREAKABLEPHYSICSINFO_H__
#define __IBREAKABLEPHYSICSINFO_H__

class IBreakablePhysicsInfo
{
public:
	virtual ~IBreakablePhysicsInfo() {}

	virtual int GetGranularity() = 0;
};

#endif //__IBREAKABLEPHYSICSINFO_H__
