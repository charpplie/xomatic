//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __ISCENEPROPERTIES_H__
#define __ISCENEPROPERTIES_H__

#include <string>

class ISceneProperties
{
public:
	virtual ~ISceneProperties() {}

	virtual int PropertyCount() = 0;
	virtual void GetProperty(int i, std::string& sName, std::string& sValue) = 0;
};

#endif //__ISCENEPROPERTIES_H__
