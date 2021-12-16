/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Implements the Editor->Game communication interface.
  
 -------------------------------------------------------------------------
  History:
  - 10:2:2010	14:00 : Rewritten by Christian Helmich
  - 30:8:2004   11:17 : Created by Márcio Martins

*************************************************************************/
#ifndef __EDITORGAME_H__
#define __EDITORGAME_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IGameRef.h>
#include <IEditorGame.h>
#include "GameConfig.h"
#include "../GameGlobals/GameCVars.h"

struct IGameStartup;

class CEditorGame :
	public IEditorGame
{
public:
	CEditorGame();
	VIRTUAL ~CEditorGame();
	static IEditorGame*	CreateInstance();

	//IEditorGame implementation
	//- game instance lifecycle
	VIRTUAL bool	Init(ISystem *pSystem,IGameToEditorInterface *pGameToEditorInterface);
	VIRTUAL int		Update(bool haveFocus, unsigned int updateFlags);
	VIRTUAL void	Shutdown();

	//- game mode
	VIRTUAL bool	SetGameMode(bool bGameMode);

	//- player
	VIRTUAL IEntity* GetPlayer();
	VIRTUAL void	SetPlayerPosAng(Vec3 pos,Vec3 viewDir);
	VIRTUAL void	HidePlayer(bool bHide);

	//- level loading
	VIRTUAL void	OnBeforeLevelLoad();
	VIRTUAL void	OnAfterLevelLoad(const char *levelName, const char *levelFolder);

	//- game sub systems
	VIRTUAL IFlowSystem* GetIFlowSystem();
	VIRTUAL IGameTokenSystem* GetIGameTokenSystem();
	VIRTUAL IEquipmentSystemInterface* GetIEquipmentSystemInterface();
	//IEditorGame implementation/

protected:
	//utility
	void	InitUIEnums(IGameToEditorInterface* pGTE);
	void	InitGlobalFileEnums(IGameToEditorInterface* pGTE);
	void	InitActionEnums(IGameToEditorInterface* pGTE);
	bool	ConfigureNetContext(bool on);
	static void OnChangeEditorMode(ICVar*);
	void	EnablePlayer(bool bPlayer);
	static void ResetClient(IConsoleCmdArgs*);

private:
	IGameRef		m_pGame;
	IGameStartup*	m_pGameStartup;
	
	bool          m_bEnabled;
	bool          m_bGameMode;
	bool          m_bPlayer;

	static ICVar *	s_pEditorGameMode;
	static CEditorGame* s_pEditorGame;
};


#endif //__EDITORGAME_H__
