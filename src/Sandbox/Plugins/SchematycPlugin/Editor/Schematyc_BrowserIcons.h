/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc browser icons.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_BROWSERICONS_H__
#define __SCHEMATYC_BROWSERICONS_H__

#include "Schematyc_PluginUtils.h"
#include "Resource.h"

namespace Schematyc
{
	namespace BrowserIcon
	{
		enum EValue
		{
			FOLDER = 0,
			DOC,
			BRANCH,
			FUNCTION_GRAPH,
			ENV_ACTION,
			ENV_SIGNAL,
			ENV_GLOBAL_FUNCTION,
			ENV_COMPONENT,
			ENV_COMPONENT_FUNCTION,
			ENUMERATION,
			VARIABLE,
			USER_GROUP,
			FOR_LOOP,
			SIGNAL,
			SCHEMA,
			COMPONENT,
			ACTION_INSTANCE,
			STATE,
			GRAPH,
			TIMER,
			SIGNAL_RECEIVER,
			CONSTRUCTOR,
			CONDITION_GRAPH,
			RETURN,
			ENV_ACTION_FUNCTION,
			ENV_ABSTRACT_INTERFACE,
			ENV_INTERFACE_FUNCTION,
			STATE_MACHINE,
			INPUT,
			OUTPUT,
			STRUCTURE,
			DESTRUCTOR,
			SETTINGS_FOLDER,
			SETTINGS_DOC,
			CONTAINER
		};
	};

	inline void LoadBrowserIcons(CImageList& imageList)
	{
		PluginUtils::LoadTrueColorImageList(IDB_SCHEMATYC_BROWSER_ICONS, 16, RGB(0, 0, 0), imageList);
	}
}

#endif //__SCHEMATYC_BROWSERICONS_H__
