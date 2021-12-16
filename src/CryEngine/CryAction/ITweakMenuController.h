/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Interface for using a Tweak Menu controller

-------------------------------------------------------------------------
History:
- 04:02:2009  : Created by Kevin Kirst

*************************************************************************/

#ifndef __ITWEAKMENUCONTROLLER_H__
#define __ITWEAKMENUCONTROLLER_H__

#pragma once

//! Tweak input action names
enum ETweakAction
{
	eTA_Enter,		// Enter menu
	eTA_Exit,		// Exit menu
	eTA_Left,		// Navigate left (to parent menu)
	eTA_Right,		// Navigate right (to child menu)
	eTA_Up,			// Navigate up (previous entry)
	eTA_Down,		// Navigate down (next entry)
	eTA_Inc,		// Increment current value
	eTA_Dec,		// Decrement current value
};

struct ITweakMenuController
{
	virtual ~ITweakMenuController() {}
	virtual void Release() = 0;

	//! Update - Update the controller
	virtual void Update(float fDeltaTime) = 0;

	//! OnAction - Handle input action (see ActionMapManager)
	virtual bool OnAction(ETweakAction eTweakAction, int nActivationMode, float fValue) = 0;

	virtual void GetMemoryStatistics(ICrySizer * s) = 0;
};

#endif //__ITWEAKMENUCONTROLLER_H__