/********************************************************************
CryGame Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   GameForceFeedback.cpp
Description: 

-------------------------------------------------------------------------
History:
- 26:6:2009					: Created by Tim Furnish

*********************************************************************/

#include "StdAfx.h"
#include "GameForceFeedback.h"
#include "IGame.h"
#include "IGameFramework.h"

#ifndef CRY_UNIT_NO_TESTING
static int s_count = 0;
#endif

void GameForceFeedback_ForEntity (EntityId entityID, const SFFOutputEvent &event)
{
	if (gEnv->pGame == NULL || entityID == gEnv->pGame->GetIGameFramework()->GetClientActorId())
	{
		GameForceFeedback_Always (event);
	}
}

void GameForceFeedback_Always (const SFFOutputEvent &event)
{
#ifndef CRY_UNIT_NO_TESTING
	++ s_count;
#endif

	if (gEnv->pInput && event.timeInSeconds > 0.0f)
	{
		gEnv->pInput->ForceFeedbackEvent(event);
	}
}

#ifndef CRY_UNIT_NO_TESTING
void GameForceFeedback_ResetCount ()
{
	s_count = 0;
}

int GameForceFeedback_GetCount ()
{
	return s_count;
}
#endif
