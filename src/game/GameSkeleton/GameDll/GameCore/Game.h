/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: 
  
 -------------------------------------------------------------------------
  History:
  - 9:2:2010	14:00 : Rewritten by Christian Helmich
  - 3:8:2004	11:23 : Created by Márcio Martins

*************************************************************************/
#ifndef __GAME_H__
#define __GAME_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IGame.h>
#include <IGameFramework.h>
#include <ILevelSystem.h>
#include <StlUtils.h>

#include "GameConfig.h"
#include "../GameGlobals/GameCCmds.h"
#include "../GameGlobals/GameCVars.h"

//forward declares to avoid including headers
struct ISystem;
struct IConsole;
struct IActionMap;

extern class CGame*	g_pGame;


#if !defined(FINAL_RELEASE)
class CGameRealtimeRemoteUpdateListener;
#endif

class CGame :
  public IGame, 
  public IGameFrameworkListener,
  public ILevelSystemListener,
  public IInputEventListener	//for camera update
{
public:
	typedef bool (*BlockingConditionFunction)();
	typedef std::map<string, string, stl::less_stricmp<string> > TLevelMapMap;	
public:

	CGame();
	VIRTUAL ~CGame();
	static	IGame*	CreateInstance(IGameFramework* pGameFramework);

	//from IGame interface
	//- game framework
	VIRTUAL	IGameFramework*	GetIGameFramework()	{	LOG_CODE_COVERAGE();	return m_pFramework;	}

	VIRTUAL	const char*		GetLongName()		{	LOG_CODE_COVERAGE();	return GAME_LONGNAME;	}
	VIRTUAL	const char*		GetName()			{	LOG_CODE_COVERAGE();	return GAME_NAME;		}

	//- game instance lifecycle
	VIRTUAL	bool	Init(IGameFramework *pFramework);
	VIRTUAL	bool	CompleteInit();
	VIRTUAL	int		Update(bool haveFocus, unsigned int updateFlags);	
	VIRTUAL	void	Shutdown();

	VIRTUAL	void	EditorResetGame(bool bStart);

	//- memory stats
	VIRTUAL	void	GetMemoryStatistics(ICrySizer * s);	
	
	//- game channel	
	VIRTUAL	void	ConfigureGameChannel(bool isServer, IProtocolBuilder *pBuilder);

	//- player id
	VIRTUAL	void	PlayerIdSet(EntityId playerId);
	VIRTUAL	void	OnClearPlayerIds();
		
	//- action maps
	VIRTUAL	void	LoadActionMaps(const char* filename)	{}	
	
	//- save game
	VIRTUAL	const char*	CreateSaveGameName();		//auto-generated save game file name	
	VIRTUAL	const bool	DoInitialSavegame() const	{	LOG_CODE_COVERAGE();	return true;	}

	//- level name
	VIRTUAL	const char* GetMappedLevelName(const char *levelName) const;	//level names were renamed without changing the file/directory

	//- game state recorder
	VIRTUAL	IGameStateRecorder* CreateGameStateRecorder(IGameplayListener* pL);
	
	//- multiplayer lobby session
	VIRTUAL	void	CreateLobbySession( const SGameStartParams* pGameStartParams )	{}
	VIRTUAL	void	DeleteLobbySession()	{LOG_CODE_COVERAGE();}
	//from IGame interface/


	//from IGameFrameworkListener interface
	VIRTUAL void	OnPostUpdate(float fDeltaTime);
	VIRTUAL void	OnSaveGame(ISaveGame* pSaveGame);
	VIRTUAL void	OnLoadGame(ILoadGame* pLoadGame);
	VIRTUAL void	OnLevelEnd(const char* nextLevel) {LOG_CODE_COVERAGE();};
	VIRTUAL void	OnActionEvent(const SActionEvent& event);
	//from IGameFrameworkListener interface/


	//from ILevelSystemListener interface
	VIRTUAL void	OnLevelNotFound(const char *levelName) {LOG_CODE_COVERAGE();}
	VIRTUAL void	OnLoadingStart(ILevelInfo *pLevel) {LOG_CODE_COVERAGE();}
	VIRTUAL void	OnLoadingComplete(ILevel *pLevel);
	VIRTUAL void	OnLoadingError(ILevelInfo *pLevel, const char *error) {LOG_CODE_COVERAGE();}
	VIRTUAL void	OnLoadingProgress(ILevelInfo *pLevel, int progressAmount) {LOG_CODE_COVERAGE();}
	//from ILevelSystemListener interface/

	//from IInputEventListener interface
	VIRTUAL bool	OnInputEvent( const SInputEvent &event );
	VIRTUAL bool	OnInputEventUI( const SInputEvent &event );
	//from IInputEventListener interface/


	//more utilities	
	string	InitMapReloading();
	bool	IsReloading() { LOG_CODE_COVERAGE();	return m_bReload; }
	void	CheckReloadLevel();

	void	BlockingProcess(BlockingConditionFunction f);
	void	GameChannelDestroyed(bool isServer);  
	void	DumpMemInfo(const char* format, ...);
	
protected:
	// These funcs live in GameCVars.cpp
// 	void	RegisterConsoleVars();
// 	void	RegisterConsoleCommands();
// 	void	UnregisterConsoleCommands();

	bool	UpdateInputs(const SInputEvent& event);
	void	UpdateCamera();

private:
	//gameplay foundation
	IGameFramework*			m_pFramework;
	IConsole*				m_pConsole;	
	
	//instance states
	bool					m_bReload;
	bool					m_inDevMode;
	
	//player id
	EntityId				m_uiPlayerID;

	//level mapped names
	TLevelMapMap			m_mapNames;

	//cvars and ccmds
	SGameCCmds				m_cCmds;
	SGameCVars				m_cVars;

	//camera pos/ang
	Ang3	m_angCam;
	Vec3	m_vCamPos;
	
#if !defined(FINAL_RELEASE)
	// Live Create support for things that must/should be updated in the game
	// on the Main Thread.
	CGameRealtimeRemoteUpdateListener*	m_pRemoteUpdateListener;
#endif
};

#endif //__GAME_H__