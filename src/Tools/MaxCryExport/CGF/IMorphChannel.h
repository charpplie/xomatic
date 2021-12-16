//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IMORPHCHANNEL_H__
#define __IMORPHCHANNEL_H__

#include <string>

class IMorphChannel
{
public:
	virtual int GetNumPoints() = 0;
	virtual std::string GetName() = 0;
	virtual const Vec3& GetPosition(int i) = 0;
	virtual const Vec3& GetDelta(int i) = 0;
};

#endif //__IMORPHCHANNEL_H__
