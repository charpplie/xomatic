/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Debug display functions
-------------------------------------------------------------------------
History:
- 24:6:2009: Created by Tim Furnish

*************************************************************************/

#ifndef __PERK_DBG_DISPLAY_H__
#define __PERK_DBG_DISPLAY_H__

#include "GameCVars.h"
#include "Utility/CryWatch.h"


#define ENABLE_GAME_DEBUG_TEXT                (1 && CRY_WATCH_ENABLED)


#define PerkDbgDisplay(...)										XDbgDisplay						(g_displayDbgText_perk,								string().Format("<PERK> "__VA_ARGS__))
#define PerkDbgDisplayForEntity(id,...)				XDbgDisplayForEntity	(g_displayDbgText_perk,					id,		string().Format("<PERK> " __VA_ARGS__))
#define HudDbgDisplay(...)										XDbgDisplay						(g_displayDbgText_hud,								string().Format("<HUD> " __VA_ARGS__))
#define HudDbgDisplayForEntity(id,...)				XDbgDisplayForEntity	(g_displayDbgText_hud,					id,		string().Format("<HUD> " __VA_ARGS__))


#if ENABLE_GAME_DEBUG_TEXT

int XDbgDisplayForEntityIDFunc(EntityId id, const char * message);

#define XDbgDisplay(cvar,message)							(g_pGameCVars && g_pGameCVars->cvar && CryWatchFunc(message))
#define XDbgDisplayForEntity(cvar,id,message)	(g_pGameCVars && g_pGameCVars->cvar && XDbgDisplayForEntityIDFunc(id,message))

#else

#define XDbgDisplay(cvar,message)							 (0)
#define XDbgDisplayForEntity(cvar,id,message)	 (0)
#define XDbgDisplayForEntityIDFunc(id,message) (0)

#endif

#endif // __PERK_DBG_DISPLAY_H__
