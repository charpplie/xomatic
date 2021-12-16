/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: Game DLL entry point.

	-------------------------------------------------------------------------
	History:
	- 2:8:2004   10:38 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameStartup.h"
#include "EditorGame.h"
#include "BasicEventListener.h"

#include <CryLibrary.h>

extern "C"
{
#ifndef SP_DEMO
	GAME_API IGame *CreateGame(IGameFramework* pGameFramework)
	{
		ModuleInitISystem(pGameFramework->GetISystem(),"CryGame");

		static char pGameBuffer[sizeof(CGame)];
		return new ((void*)pGameBuffer) CGame();
	}

	GAME_API IGameStartup *CreateGameStartup()
	{
		static CBasicEventListener basicEventListener;
		return CGameStartup::Create(basicEventListener);
	}
	GAME_API IEditorGame *CreateEditorGame()
	{
		return new CEditorGame();
	}
#endif
}


/*
 * this section makes sure that the framework dll is loaded and cleaned up
 * at the appropriate time
 */

#if !defined(_LIB) && !defined(LINUX) && !defined(PS3)

static HMODULE s_frameworkDLL;


static void CleanupFrameworkDLL()
{
	assert( s_frameworkDLL );
	FreeLibrary( s_frameworkDLL );
	s_frameworkDLL = 0;
}

HMODULE GetFrameworkDLL(const char* binariesDir)
{
	if (!s_frameworkDLL)
	{
		if (binariesDir && binariesDir[0])
		{
			string dllName = PathUtil::Make(binariesDir, GAME_FRAMEWORK_FILENAME);
			s_frameworkDLL = CryLoadLibrary(dllName.c_str());		
		}
		else
		{
			s_frameworkDLL = CryLoadLibrary(GAME_FRAMEWORK_FILENAME);
		}
		atexit( CleanupFrameworkDLL );
	}
	return s_frameworkDLL;
}

#endif //WIN32


#if defined(SP_DEMO) || defined(CRYSIS_BETA)
///////////////////////////////////////////////
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// we need pass the full command line, including the filename
	// lpCmdLine does not contain the filename.

	HANDLE mutex = CreateMutex(NULL, TRUE, "CrytekApplication");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if(MessageBox(GetDesktopWindow(), "There is already a Crytek application running\nDo you want to start another one?", "Too many apps", MB_YESNO)!=IDYES)
			return 1;
	}

	// create the startup interface
	//IGameStartup *pGameStartup = CreateGameStartup();

	static char gameStartup_buffer[sizeof(CGameStartup)];
	IGameStartup *pGameStartup = new ((void*)gameStartup_buffer) CGameStartup();	

	if (!pGameStartup)
	{
		// failed to create the startup interface
		//CryFreeLibrary(gameDll);

		MessageBox(0, "Failed to create the GameStartup Interface!", "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);

		CloseHandle(mutex);
		return 0;
	}

	static const char logFileName[] = "Game.log";

	SSystemInitParams startupParams;

	startupParams.hInstance = GetModuleHandle(0);
	startupParams.sLogFileName = logFileName;	
	sprintf(startupParams.szSystemCmdLine,"Crysis.exe %s",lpCmdLine);
	//strcpy(startupParams.szSystemCmdLine, lpCmdLine);

	// run the game
	if (pGameStartup->Init(startupParams))
	{
		pGameStartup->Run(NULL);

		pGameStartup->Shutdown();
		pGameStartup = 0;
	}

	CloseHandle(mutex);
	return 0;
}
#endif
