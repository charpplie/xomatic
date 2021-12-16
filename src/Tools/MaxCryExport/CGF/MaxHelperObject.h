//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXHELPEROBJECT_H__
#define __MAXHELPEROBJECT_H__

#include "IHelperObject.h"

class MaxHelperObject : public IHelperObject
{
public:
	MaxHelperObject(INode* pMaxNode);

	// IHelperObject
	virtual HelperTypes GetType();

private:
	INode* pMaxNode;
};

#endif //__MAXHELPEROBJECT_H__
