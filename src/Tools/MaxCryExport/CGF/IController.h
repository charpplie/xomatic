//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __ICONTROLLER_H__
#define __ICONTROLLER_H__

#include "CryHeaders.h"

class IControllerKeyHandler;

class IController
{
public:
	virtual ~IController() {}

	virtual CtrlTypes GetType() = 0;
	virtual void HandleKeys(IControllerKeyHandler* pKeyHandler) = 0;
	virtual unsigned int GetFlags() = 0;
	virtual int KeyCount() = 0;
};

#endif //__ICONTROLLER_H__
