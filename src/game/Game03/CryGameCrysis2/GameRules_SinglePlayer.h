/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 18:6:2009  : Created by Colin Gulliver

*************************************************************************/
#ifndef __GAMERULES_SINGLEPLAYER_H__
#define __GAMERULES_SINGLEPLAYER_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRules.h"


class CGameRules_SinglePlayer :	public CGameRules
{
public:
	CGameRules_SinglePlayer();
	virtual ~CGameRules_SinglePlayer();

	virtual bool Init( IGameObject * pGameObject );

	virtual void SvOnClientConnect( int channelId, bool reset, const char *name );
}; 

#endif //__GAMERULES_SINGLEPLAYER__
