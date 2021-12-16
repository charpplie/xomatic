/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Debug display functions
-------------------------------------------------------------------------
History:
- 24:6:2009: Created by Tim Furnish

*************************************************************************/

#include "StdAfx.h"
#include "PerkDbgDisplay.h"
#include "IEntitySystem.h"
#include "Actor.h"
#include "Utility/CryWatch.h"

#if ENABLE_GAME_DEBUG_TEXT

int XDbgDisplayForEntityIDFunc(EntityId id, const char * message)
{
	IEntity * entity = gEnv->pEntitySystem->GetEntity(id);
	bool isLocalActor = entity && id == gEnv->pGame->GetIGameFramework()->GetClientActorId();

	return CryWatch("$%c[%d \"%s\"]$1 %s", isLocalActor ? '5' : '6', id, entity ? entity->GetName() : "NULL", message);
}

#endif
