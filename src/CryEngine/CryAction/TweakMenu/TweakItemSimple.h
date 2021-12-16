/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2008.
-------------------------------------------------------------------------

Description: 
Simple tweak menu item for debugging and other items not requiring LUA

-------------------------------------------------------------------------
History:
- 1:11:2008  : Created by Adam Rutkowski

*************************************************************************/


#ifndef __CTWEAKITEMSIMPLE_H__
#define __CTWEAKITEMSIMPLE_H__

#pragma once

#include "TweakCommon.h"

//-------------------------------------------------------------------------

class CTweakItemSimple : public CTweakCommon 
{
public:

	CTweakItemSimple(const string& itemName);
	~CTweakItemSimple() {};

	virtual string GetValue(void) { return "no value"; }
	virtual bool DecreaseValue(void) { return true; }
	virtual bool IncreaseValue(void) { return true; }

	ETweakType GetType(void) { return eTT_ItemSimple; }

};

#endif // __CTWEAKITEMSIMPLE_H__