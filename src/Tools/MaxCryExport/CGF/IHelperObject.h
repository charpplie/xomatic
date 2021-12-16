//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IHELPEROBJECT_H__
#define __IHELPEROBJECT_H__

#include "CryHeaders.h"

class IHelperObject
{
public:
	virtual HelperTypes GetType() = 0;
};

#endif //__IHELPEROBJECT_H__
