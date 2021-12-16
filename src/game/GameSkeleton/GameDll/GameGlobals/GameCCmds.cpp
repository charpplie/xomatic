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

#include "StdAfx.h"
#include "GameCCmds.h"

#include <IGameFramework.h>
#include <ILevelSystem.h>

//////////////////////////////////////////////////////////////////////////
//- static console command functions

#define	ERR_NO_LEVEL_PARAM	(GameWarning( "No level parameter received" ))
#define ERR_IN_GAME_ONLY	(GameWarning( "Works in Game mode only" ))
#define ERR_LEVEL_LOAD_FAIL	(GameWarning( "Failed to load level" ))

static void CmdMap(IConsoleCmdArgs* args)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(args);
	CRY_SAFE_RETURN(!args, ERR_NO_LEVEL_PARAM);
	CRY_SAFE_RETURN(!args->GetArgCount(), ERR_NO_LEVEL_PARAM);

	const char* levelName = args->GetArg(1);
	CRY_ASSERT(levelName);
	CRY_SAFE_RETURN(!levelName, ERR_NO_LEVEL_PARAM);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);
	CRY_SAFE_RETURN(gEnv->IsEditor(), ERR_IN_GAME_ONLY);

	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pGame);
	CRY_SAFE_RETURN(!gEnv->pGame, CRY_NO_RETURN_VALUE);

	IGameFramework* pGameFramework = gEnv->pGame->GetIGameFramework();
	CRY_ASSERT(pGameFramework);
	CRY_SAFE_RETURN(!pGameFramework, CRY_NO_RETURN_VALUE);

	ILevelSystem* pLevelSystem = pGameFramework->GetILevelSystem();
	CRY_ASSERT(pLevelSystem);
	CRY_SAFE_RETURN(!pLevelSystem, CRY_NO_RETURN_VALUE);

	ILevel* pLevel = pLevelSystem->LoadLevel(levelName);
	CRY_ASSERT(pLevel);
	CRY_SAFE_RETURN(!pLevel, ERR_LEVEL_LOAD_FAIL);

}

static void CmdMap_blah(IConsoleCmdArgs* args)
{
	LOG_CODE_COVERAGE();

	uint32 flags = eGSF_ImmersiveMultiplayer;

	CRY_ASSERT(args);
	CRY_SAFE_RETURN(!args, (GameWarning( "No level parameter received" )) );
	CRY_SAFE_RETURN(!args->GetArgCount(), (GameWarning( "No level parameter received" )));

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);
	CRY_SAFE_RETURN(gEnv->IsEditor(), (GameWarning( "Won't load level in editor" )) );

	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pGame);
	CRY_SAFE_RETURN(!gEnv->pGame, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pNetwork);
	CRY_SAFE_RETURN(!gEnv->pNetwork, CRY_NO_RETURN_VALUE);

	IGameFramework* pGameFramework = gEnv->pGame->GetIGameFramework();
	CRY_ASSERT(pGameFramework);
	CRY_SAFE_RETURN(!pGameFramework, CRY_NO_RETURN_VALUE);

	ILevelSystem* pLevelSystem = pGameFramework->GetILevelSystem();
	CRY_ASSERT(pLevelSystem);
	CRY_SAFE_RETURN(!pLevelSystem, CRY_NO_RETURN_VALUE);
	
	const char* levelName = args->GetArg(1);
	CRY_ASSERT(levelName);
	CRY_SAFE_RETURN(!levelName, CRY_NO_RETURN_VALUE);

	ILevelInfo* pLevelInfo = pLevelSystem->GetLevelInfo(levelName);
	CRY_ASSERT(pLevelInfo);
	CRY_SAFE_RETURN(!pLevelInfo, (GameWarning( "Map '%s' not found", levelName)) );

	IGameRulesSystem* pGameRulesSys = pGameFramework->GetIGameRulesSystem();
// 	CRY_ASSERT(pGameRulesSys);
// 	CRY_SAFE_RETURN(!pGameRulesSys, CRY_NO_RETURN_VALUE);

// 	IGameRules* pGameRules = NULL;
// 	if(pGameRulesSys)
// 		pGameRules = pGameRulesSys->GetCurrentGameRules();

	SGameContextParams ctx;
	ctx.gameRules = "";
	ctx.levelName = levelName;

	//check if we want to run a dedicated server
	bool bIsDedicated		= gEnv->pSystem->IsDedicated();
	bool bIsServer			= bIsDedicated;	
	bool bForceNewContext	= false;

	//if running dedicated network server - default nonblocking mode
	bool bBlocking = true;
	if(pGameFramework->StartedGameContext())
		bBlocking = !bIsDedicated;
	
	if (bBlocking)
	{
		flags |= eGSF_BlockingClientConnect | eGSF_BlockingMapLoad;
		bForceNewContext = true;
	}

	if(bIsServer)
	{		
		//gEnv->pNetwork->GetService("GameSpy");
	}

	// if we already have a game context, then we just change it
	bool bStartedContext = false;	
	if(pGameFramework->StartedGameContext())
	{
		if (ctx.levelName)
			pLevelSystem->PrepareNextLevel( ctx.levelName );

		if (bForceNewContext)
			pGameFramework->EndGameContext();
		else
		{
			pGameFramework->ChangeGameContext(&ctx);
			bStartedContext = true;
		}
	}
	if(!bStartedContext)
	{
		CRY_ASSERT(!pGameFramework->StartedGameContext());
		CRY_SAFE_RETURN(pGameFramework->StartedGameContext(), CRY_NO_RETURN_VALUE);

		SGameStartParams params;
		params.flags = flags | eGSF_Server;

		if (!bIsDedicated)
		{
			params.flags |= eGSF_Client;
			params.hostname = "localhost";
		}
		if (bIsServer)
		{
			ICVar* max_players = gEnv->pConsole->GetCVar("sv_maxplayers");
			params.maxPlayers = max_players?max_players->GetIVal():16; 
			ICVar* loading = gEnv->pConsole->GetCVar("g_enableloadingscreen");
			if(loading)
				loading->Set(0);
			//gEnv->pConsole->GetCVar("g_enableitems")->Set(0);
		}
		else
		{
			params.flags |= eGSF_LocalOnly;
			params.maxPlayers = 1;
		}

		params.pContextParams = &ctx;
		params.port = gEnv->pConsole->GetCVar("sv_port")->GetIVal();

		//GetCryAction()->GetIGameSessionHandler()->CreateSession(&params);
	}	
}

//- static console command functions/
//////////////////////////////////////////////////////////////////////////
//- ctor & dtor

SGameCCmds::SGameCCmds()
{
	LOG_CODE_COVERAGE();
}

SGameCCmds::~SGameCCmds()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, CRY_NO_RETURN_VALUE);

	gEnv->pConsole->RemoveCommand("quit");
	gEnv->pConsole->RemoveCommand("exit");
	gEnv->pConsole->RemoveCommand("x");
	gEnv->pConsole->RemoveCommand("q");
	gEnv->pConsole->RemoveCommand("map");
}

//- ctor & dtor/
//////////////////////////////////////////////////////////////////////////
//- registration

void	SGameCCmds::Register()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, CRY_NO_RETURN_VALUE);

	REGISTER_COMMAND("quit", "System.Quit()", VF_RESTRICTEDMODE, "Quits the game");
	//aliases to a given command are possible like this
	REGISTER_COMMAND("exit", "System.Quit()", VF_RESTRICTEDMODE, "Quits the game");
	REGISTER_COMMAND("x", "System.Quit()", VF_RESTRICTEDMODE, "Quits the game");
	REGISTER_COMMAND("q", "System.Quit()", VF_RESTRICTEDMODE, "Quits the game");

	REGISTER_COMMAND("map", "System.Quit()", VF_RESTRICTEDMODE, "Quits the game");
}

//- registration/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////