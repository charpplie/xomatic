/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
Description:
Handles player interaction...
* Sometimes prompts and responds to inputs
* Sometimes automatic
**************************************************************************/

#ifndef __PLAYERPLUGIN_INTERACTION_H__
#define __PLAYERPLUGIN_INTERACTION_H__

#include "PlayerPlugin.h"

class CPlayerPlugin_Interaction : public CPlayerPlugin
{
	public:
		SET_PLAYER_PLUGIN_NAME(CPlayerPlugin_Interaction);

	private:
		virtual void Update(const float dt);
		virtual void HandleEvent(EPlayerPlugInEvent theEvent, void * data);

#if defined(_DEBUG)
		EntityId m_lastNearestEntityId;
#endif
};

#endif __PLAYERPLUGIN_INTERACTION_H__

