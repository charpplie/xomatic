/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 

-------------------------------------------------------------------------
History:
- 10:2:2010		17:00 : Rewritten by Christian Helmich	
- 2:8:2004		11:04 : Created by Márcio Martins

*************************************************************************/
#ifndef __GAMESTARTUP_H__
#define __GAMESTARTUP_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IGameFramework.h>


#include "GameConfig.h"

// implemented in GameDll.cpp
extern HMODULE GetFrameworkDLL();

class CGameStartup :
	public IGameStartup
{
public:
	CGameStartup();
	VIRTUAL ~CGameStartup();
	static IGameStartup* CreateInstance();

	//IGameStartup implementation
	//- instance lifecycle
	VIRTUAL IGameRef	Init(SSystemInitParams& startupParams);
	VIRTUAL IGameRef	Reset(const char* modName);
	VIRTUAL void		Shutdown();
	VIRTUAL int			Update(bool haveFocus, unsigned int updateFlags);

	//- level
	VIRTUAL bool		GetRestartLevel(char** levelName);
	VIRTUAL int			Run(const char* autoStartLevelName);

	//- patch & mod
	VIRTUAL const char*	GetPatch() const;
	VIRTUAL bool		GetRestartMod(char* pModName, int nameLenMax);	
	//IGameStartup implementation/

	//public static utilities
	static void		ForceCursorUpdate();
	static void		AllowAccessibilityShortcutKeys(bool bAllowKeys);
	//public static utilities/

protected:
	//per-instance utilities
	//- localization
	void			LoadLocalizationData();

	//- load mod or game, according to cmd line
	IGameRef		LoadGameMod();
	//per-instance utilities/

	//static utilities
	static bool		InitWindow(SSystemInitParams& startupParams);
	static void		ShutdownWindow();
#ifdef WIN32
	static LRESULT	CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif //WIN32
	static bool		InitFramework(SSystemInitParams& startupParams);
	static void		ShutdownFramework();
	//static utilities/

	

private:
	static IGame*				m_pGame;
	static IGameRef				m_gameRef;	//why having both?
	static bool					m_initWindow;

	static HMODULE				m_hSystemDll;
	static HMODULE				m_hGameDll;
	
	static HMODULE				m_hFrameworkDll;
	static IGameFramework*		m_pFramework;

	static string				m_rootDir;
	static string				m_binDir;
	static string				m_reqModName;


	static HWND					m_hWnd;
};


struct CSystemEventListener_Game :
	ISystemEventListener
{
	//ISystemEventListener implementation
	VIRTUAL void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam );
	//ISystemEventListener implementation/
};

#endif //__GAMESTARTUP_H__
