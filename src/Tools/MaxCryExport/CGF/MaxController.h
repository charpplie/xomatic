//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXCONTROLLER_H__
#define __MAXCONTROLLER_H__

#include "IController.h"

class MaxController : public IController
{
public:
	MaxController(Control* pMaxController);
	virtual ~MaxController();

	virtual CtrlTypes GetType();
	virtual void HandleKeys(IControllerKeyHandler* pKeyHandler);
	virtual unsigned int GetFlags();
	virtual int KeyCount();

private:
	Control* pMaxController;
};

#endif //__MAXCONTROLLER_H__
