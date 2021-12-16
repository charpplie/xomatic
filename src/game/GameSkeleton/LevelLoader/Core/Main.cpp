#include "StdAfx.h"
#include "CryEngineApp.h"

HINSTANCE g_hInstance = 0;

//////////////////////////////////////////////////////////////////////////
// Initializes Root folder of the game.
//////////////////////////////////////////////////////////////////////////
void InitRootDir()
{
#ifdef WIN32
	WCHAR szExeFileName[_MAX_PATH];

	GetModuleFileNameW( GetModuleHandle(NULL), szExeFileName, sizeof(szExeFileName));
	PathRemoveFileSpecW(szExeFileName);

	// Remove Bin32/Bin64 folder/
	WCHAR* lpPath = StrStrIW(szExeFileName,L"\\Bin32");
	if (lpPath)
		*lpPath = 0;
	lpPath = StrStrIW(szExeFileName,L"\\Bin64");
	if (lpPath)
		*lpPath = 0;

	SetCurrentDirectoryW( szExeFileName );
#endif
}



///////////////////////////////////////////////
CLevelLoader* g_pLevelLoader = NULL;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// we need pass the full command line, including the filename
	// lpCmdLine does not contain the filename.

	g_hInstance = hInstance;

	InitRootDir();

	string levelName = lpCmdLine;

	CCryEngineApp engine;

	int nRes = 0;
	if (engine.InitCrySystem( lpCmdLine ) )
	{
		CryLogAlways("cmdline: %s", lpCmdLine);
		g_pLevelLoader  = new CLevelLoader( GetISystem(), "Levels" );
		g_pLevelLoader->LoadLevel( levelName.c_str() );

		nRes = engine.MainLoop();
	}

	return nRes;
}

