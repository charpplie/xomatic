/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:
Groups all methods related to accessing 'mind control' through the console.

-------------------------------------------------------------------------
History:
- 3:11:2009: Created by Sven Van Soom

*************************************************************************/

#pragma once

#ifndef __MIND_CONTROL_CONSOLE_H__
#define __MIND_CONTROL_CONSOLE_H__

#if ENABLE_MINDCONTROL

/*static*/ class CMindControlConsole
{
public:
	static void InitCVars(IConsole *pConsole);
	static void ReleaseCVars(IConsole *pConsole);
	static void OnStartGame(); // Re-attach to mind controlled entity when entering game

private:
	CMindControlConsole() {}

	static void OnChangeMindControlEntityCVar(ICVar* pICVar);
	static void ChangeMindControlEntity(const char* szEntityName);
	static void ControlEntity(uint32 entityId);
	static IEntity* FindPlayerEntityByPartialName(const char* szPartialName);
};

#endif

#endif // __MIND_CONTROL_CONSOLE_H__
