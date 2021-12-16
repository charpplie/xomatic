/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:	Game-wide console commands (CCmds)

-------------------------------------------------------------------------
History:
- 15:2:2010		17:00 : Created by Christian Helmich	

*************************************************************************/

#ifndef __GAME_CCMDS_H__
#define __GAME_CCMDS_H__	1

#include "../GameCore/GameConfig.h"
#include "GameCVars.h"

struct	SGameCCmds
{
	SGameCCmds();
	~SGameCCmds();

	void	Register();
};

#endif //__GAME_CCMDS_H__

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
