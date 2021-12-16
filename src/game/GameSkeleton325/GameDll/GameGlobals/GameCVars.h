/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:	Game-wide global variables, aka CVars	

-------------------------------------------------------------------------
History:
- 15:2:2010		17:00 : Created by Christian Helmich	

*************************************************************************/

#ifndef __GAME_CVARS_H__
#define __GAME_CVARS_H__	1

#include "../GameCore/GameConfig.h"

struct	SGameCVars 
{
	SGameCVars();
	~SGameCVars();

	void	Register();

	static	int gs_LogCodeCoverage;
};

#endif //__GAME_CVARS_H__

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////