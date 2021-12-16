/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: Game DLL entry point.

	-------------------------------------------------------------------------
	History:
	- 9:2:2010   10:00 : Rewritten by Christian Helmich
	- 2:8:2004   10:38 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"

#include "GameConfig.h"
#include "GameStartup.h"
#include "Game.h"
#include "EditorGame.h"

#include <CryLibrary.h>

extern "C"
{
	GAME_API IGame*	CreateGame(IGameFramework* pGameFramework)
	{
		LOG_CODE_COVERAGE();

		return CGame::CreateInstance(pGameFramework);
	}

	GAME_API IGameStartup*	CreateGameStartup()
	{
		LOG_CODE_COVERAGE();

		return CGameStartup::CreateInstance();
	}
	GAME_API IEditorGame*	CreateEditorGame()
	{
		LOG_CODE_COVERAGE();

		return CEditorGame::CreateInstance();
	}
}


#ifdef USE_EXTERNAL_FRAMEWORK_DLL
//	this section makes sure that the framework dll is loaded and cleaned up 
//	at the appropriate time

#if !defined(_LIB) && !defined(LINUX) && !defined(PS3)

static HMODULE s_frameworkDLL;

static void CleanupFrameworkDLL()
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT( s_frameworkDLL );
	FreeLibrary( s_frameworkDLL );
	s_frameworkDLL = 0;
}

HMODULE GetFrameworkDLL()
{
	LOG_CODE_COVERAGE();
	if (!s_frameworkDLL)
	{
		s_frameworkDLL = CryLoadLibrary(GAME_FRAMEWORK_DLL);
		atexit( CleanupFrameworkDLL );
	}
	return s_frameworkDLL;
}

#endif //!defined(_LIB) && !defined(LINUX) && !defined(PS3)
#endif //USE_EXTERNAL_FRAMEWORK_DLL
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////