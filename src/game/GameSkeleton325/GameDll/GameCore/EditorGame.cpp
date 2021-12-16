/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 10:2:2010	14:00 : Rewritten by Christian Helmich
  - 30:8:2004   11:19 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "EditorGame.h"
#include "GameStartup.h"

#include <IActorSystem.h>
#include <ILevelSystem.h>
#include <IGameRulesSystem.h>

#define EDITOR_SERVER_PORT 0xed17

//static member vars
ICVar* CEditorGame::s_pEditorGameMode = NULL;
CEditorGame* CEditorGame::s_pEditorGame = NULL;

// DLL interface
extern "C" 
{
	GAME_API IGameStartup* CreateGameStartup();
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// CEditorGame ctor/dtor

CEditorGame::CEditorGame():
	m_pGame(0),
	m_pGameStartup(0),
	//m_pEquipmentSystemInterface(0),
	m_bEnabled(false),
	m_bGameMode(false),
	m_bPlayer(false)
{	
	LOG_CODE_COVERAGE();
	s_pEditorGame = this;
	s_pEditorGameMode = NULL;
}


CEditorGame::~CEditorGame()
{	
	LOG_CODE_COVERAGE();
	s_pEditorGame = NULL;
	SAFE_RELEASE(s_pEditorGameMode);
}



IEditorGame* CEditorGame::CreateInstance()
{
	LOG_CODE_COVERAGE();
	return new CEditorGame();
}

// CEditorGame ctor/dtor/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// IEditorGame implementation

//- game instance lifecycle

bool CEditorGame::Init(ISystem* pSystem, IGameToEditorInterface* pGameToEditorInterface)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(pSystem);
	CRY_SAFE_RETURN(!pSystem, false);

	CRY_ASSERT(pGameToEditorInterface);
	CRY_SAFE_RETURN(!pGameToEditorInterface, false);
	
	//initialize GameStartup
	SSystemInitParams startupParams;
	startupParams.bEditor = true;
	startupParams.pSystem = pSystem;
	startupParams.bExecuteCommandLine = false;		// in editor we do it later - after other things are initialized

	m_pGameStartup = CreateGameStartup();
	CRY_ASSERT(m_pGameStartup);
	CRY_SAFE_RETURN(!m_pGameStartup, false);

	m_pGame = m_pGameStartup->Init(startupParams);
	CRY_ASSERT(m_pGame);
	CRY_SAFE_RETURN(!m_pGame, false);

	InitUIEnums(pGameToEditorInterface);
	
	gEnv->bClient = true;
	gEnv->bServer = true;
	gEnv->bMultiplayer = false;

	s_pEditorGameMode = REGISTER_INT( "net_gamemode", 0, 0, "Should editor connect a new client?");
	CRY_ASSERT(s_pEditorGameMode);
	CRY_SAFE_RETURN(!s_pEditorGameMode, false);
	s_pEditorGameMode->SetOnChangeCallback(&OnChangeEditorMode);

	SetGameMode(false);

	REGISTER_COMMAND( "net_reseteditorclient", ResetClient, 0, "Resets player and gamerules!" );

	ConfigureNetContext(true);

	return true;
}


int CEditorGame::Update(bool haveFocus, unsigned int updateFlags)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(m_pGameStartup);
	return m_pGameStartup->Update(haveFocus, updateFlags);
}


void CEditorGame::Shutdown()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(m_pGameStartup);
	gEnv->pConsole->RemoveCommand("net_reseteditorclient");

	EnablePlayer(false);
	SetGameMode(false);
	m_pGameStartup->Shutdown();
}

//- game instance lifecycle/
//////////////////////////////////////////////////////////////////////////
//- game mode

bool CEditorGame::SetGameMode(bool bGameMode)
{
	LOG_CODE_COVERAGE();

	m_bGameMode = bGameMode;

	bool on = bGameMode;
	CRY_ASSERT(s_pEditorGameMode);
	CRY_SAFE_RETURN(s_pEditorGameMode, false);
	if (s_pEditorGameMode->GetIVal() == 0)
		on = m_bPlayer;

	bool ok = ConfigureNetContext( on );
	if (ok)
	{
		if(gEnv->IsEditor())
		{
			m_pGame->EditorResetGame(bGameMode);
		}

		IGameFramework* pGameFramework = m_pGame->GetIGameFramework();
		CRY_ASSERT(pGameFramework);
		CRY_SAFE_RETURN(pGameFramework, false);
		pGameFramework->OnEditorSetGameMode(bGameMode);
	}
	else
	{
		GameWarning("Failed configuring net context");
	}
	return ok;
}

//- game mode/
//////////////////////////////////////////////////////////////////////////
//- player

IEntity* CEditorGame::GetPlayer()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(m_pGame);
	CRY_SAFE_RETURN(!m_pGame, NULL);

	IGameFramework* pGameFramework = m_pGame->GetIGameFramework();	
	CRY_ASSERT(pGameFramework);
	CRY_SAFE_RETURN(!pGameFramework, NULL);	

	IActor* pActor = pGameFramework->GetClientActor();
	//CRY_ASSERT(pActor);	//NULL is valid

	return pActor? pActor->GetEntity() : NULL;
}


void CEditorGame::SetPlayerPosAng(Vec3 pos,Vec3 viewDir)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(m_pGame);
	CRY_SAFE_RETURN(!m_pGame, CRY_NO_RETURN_VALUE);

	IGameFramework* pGameFramework = m_pGame->GetIGameFramework();	
	CRY_ASSERT(pGameFramework);
	CRY_SAFE_RETURN(!pGameFramework, CRY_NO_RETURN_VALUE);	

	IActor* pClActor = pGameFramework->GetClientActor();
	//CRY_ASSERT(pClActor); //NULL is valid
	CRY_SAFE_RETURN(!pClActor, CRY_NO_RETURN_VALUE);	
	
	// pos coming from editor is a camera position, we must convert it into the player position by subtructing eye height.
	IEntity* pPlayer = pClActor->GetEntity();
	CRY_ASSERT(pPlayer);
	CRY_SAFE_RETURN(!pPlayer, CRY_NO_RETURN_VALUE);	

	pe_player_dimensions dim;
	dim.heightEye = 0;
	IPhysicalEntity* pPhyEntity = pPlayer->GetPhysics();
	CRY_ASSERT(pPhyEntity);
	//CRY_SAFE_RETURN(!pPhyEntity, CRY_NO_RETURN_VALUE);	

	if (pPhyEntity)
	{
		pPhyEntity->GetParams( &dim );
		pos.z = pos.z - dim.heightEye;
	}
	pPlayer->SetPosRotScale( pos, Quat::CreateRotationVDir(viewDir), Vec3(1,1,1), ENTITY_XFORM_EDITOR );	
}


void CEditorGame::HidePlayer(bool bHide)
{
	LOG_CODE_COVERAGE();

	IEntity* pEntity = GetPlayer();
	if(pEntity)
		pEntity->Hide( bHide );
}

//- player/
//////////////////////////////////////////////////////////////////////////
//- level loading

void CEditorGame::OnBeforeLevelLoad()
{
	LOG_CODE_COVERAGE();

	EnablePlayer(false);
	ConfigureNetContext(true);

	CRY_ASSERT(m_pGame);
	CRY_SAFE_RETURN(!m_pGame, CRY_NO_RETURN_VALUE);

	IGameFramework* pFramework = m_pGame->GetIGameFramework();
	CRY_ASSERT(pFramework);
	CRY_SAFE_RETURN(!pFramework, CRY_NO_RETURN_VALUE);

	IGameRulesSystem* pGameRulesSystem = pFramework->GetIGameRulesSystem();
	//CRY_ASSERT(pGameRulesSystem);	//NULL is valid
	//CRY_SAFE_RETURN(!pGameRulesSystem, CRY_NO_RETURN_VALUE);
	if(pGameRulesSystem)
		pGameRulesSystem->CreateGameRules("SinglePlayer");

	ILevelSystem*	pLevelSystem = pFramework->GetILevelSystem();
	//CRY_ASSERT(pLevelSystem);	//NULL is valid
	CRY_SAFE_RETURN(!pLevelSystem, CRY_NO_RETURN_VALUE);
	pLevelSystem->OnLoadingStart(0);
}


void CEditorGame::OnAfterLevelLoad(const char* levelName, const char* levelFolder)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(levelName);
	CRY_ASSERT(levelFolder);
	CRY_ASSERT(m_pGame);
	CRY_SAFE_RETURN(!m_pGame, CRY_NO_RETURN_VALUE);

	IGameFramework* pFramework = m_pGame->GetIGameFramework();
	CRY_ASSERT(pFramework);
	CRY_SAFE_RETURN(!pFramework, CRY_NO_RETURN_VALUE);

	pFramework->SetEditorLevel(levelName, levelFolder);

	ILevelSystem* pLevelSystem = pFramework->GetILevelSystem();
	//CRY_ASSERT(pLevelSystem);	//NULL is still valid
	CRY_SAFE_RETURN(!pLevelSystem, CRY_NO_RETURN_VALUE);

	pLevelSystem->OnLoadingComplete(0);

	EnablePlayer(true);
}

//- level loading/
//////////////////////////////////////////////////////////////////////////
//- game sub systems

IFlowSystem * CEditorGame::GetIFlowSystem()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(m_pGame);
	CRY_SAFE_RETURN(!m_pGame, NULL);

	IGameFramework* pFramework = m_pGame->GetIGameFramework();
	CRY_ASSERT(pFramework);
	CRY_SAFE_RETURN(!pFramework, NULL);

	return pFramework->GetIFlowSystem();
}


IGameTokenSystem* CEditorGame::GetIGameTokenSystem()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(m_pGame);
	CRY_SAFE_RETURN(!m_pGame, NULL);

	IGameFramework* pFramework = m_pGame->GetIGameFramework();
	CRY_ASSERT(pFramework);
	CRY_SAFE_RETURN(!pFramework, NULL);

	return pFramework->GetIGameTokenSystem();
}


IEquipmentSystemInterface* CEditorGame::GetIEquipmentSystemInterface()
{
	LOG_CODE_COVERAGE();

	return NULL; //m_pEquipmentSystemInterface;
}

//- game sub systems/
// IEditorGame implementation/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// utility functions

void CEditorGame::InitUIEnums(IGameToEditorInterface* pGTE)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(pGTE);
	InitGlobalFileEnums(pGTE);
	InitActionEnums(pGTE);
}

//-----------------------------------

void CEditorGame::InitGlobalFileEnums(IGameToEditorInterface* pGTE)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(pGTE);
	CRY_SAFE_RETURN(!pGTE, CRY_NO_RETURN_VALUE);
	// Read in enums stored offline XML. Format is
	// <GlobalEnums>
	//   <EnumName>
	//     <entry enum="someName=someValue" />  <!-- displayed name != value -->
	// 	   <entry enum="someNameValue" />       <!-- displayed name == value -->
	//   </EnumName>
	// </GlobalEnums>
	//
	XmlNodeRef rootNode = GetISystem()->LoadXmlFile("Libs/UI/GlobalEnums.xml");
	if (!rootNode || !rootNode->getTag() || stricmp(rootNode->getTag(), "GlobalEnums") != 0)
	{
		// GameWarning("CEditorGame::InitUIEnums: File 'Libs/UI/GlobalEnums.xml' is not a GlobalEnums file");
		return;
	}
	for (int i = 0; i < rootNode->getChildCount(); ++i)
	{
		XmlNodeRef enumNameNode = rootNode->getChild(i);
		const char* enumId = enumNameNode->getTag();
		if (enumId == 0 || *enumId=='\0')
			continue;
		int maxChilds = enumNameNode->getChildCount();
		if (maxChilds > 0)
		{
			// allocate enough space to hold all strings
			const char** nameValueStrings = new const char*[maxChilds];
			int curEntryIndex = 0;
			for (int j = 0; j < maxChilds; ++j)
			{
				XmlNodeRef enumNode = enumNameNode->getChild(j);
				const char* nameValue = enumNode->getAttr("enum");
				if (nameValue != 0 && *nameValue!='\0')
				{
					// put in the nameValue pair
					nameValueStrings[curEntryIndex++] = nameValue;
				}
			}
			// if we found some entries inform CUIDataBase about it
			if (curEntryIndex > 0)
				pGTE->SetUIEnums(enumId, nameValueStrings, curEntryIndex);

			// be nice and free our array
			delete[] nameValueStrings;
		}
	}
}

//-----------------------------------

void CEditorGame::InitActionEnums(IGameToEditorInterface* pGTE)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(pGTE);
	CRY_SAFE_RETURN(!pGTE, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(m_pGame);
	CRY_SAFE_RETURN(!m_pGame, CRY_NO_RETURN_VALUE);

	IGameFramework* pFramework = m_pGame->GetIGameFramework();
	CRY_ASSERT(pFramework);
	CRY_SAFE_RETURN(!pFramework, CRY_NO_RETURN_VALUE);

	// init ActionFilter enums
	IActionMapManager* pActionMapMgr = pFramework->GetIActionMapManager();
	//CRY_ASSERT(pActionMapMgr);	//NULL is valid
	CRY_SAFE_RETURN(!pActionMapMgr, CRY_NO_RETURN_VALUE);
	
	std::vector<string> filterNames;
	filterNames.push_back(""); // empty
	IActionFilterIteratorPtr pFilterIter = pActionMapMgr->CreateActionFilterIterator();
	while (IActionFilter* pFilter = pFilterIter->Next())
	{
		filterNames.push_back(pFilter->GetName());
	}
	size_t numFilters = 0;
	const char** allFilters = new const char*[filterNames.size()];
	std::vector<string>::const_iterator iter = filterNames.begin();
	std::vector<string>::const_iterator iterEnd = filterNames.end();
	while (iter != iterEnd)
	{
		allFilters[numFilters++] = iter->c_str();
		++iter;
	}
	pGTE->SetUIEnums("action_filter", allFilters, numFilters);
	delete[] allFilters;	
}

//-----------------------------------

bool CEditorGame::ConfigureNetContext( bool on )
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(m_pGame);
	CRY_SAFE_RETURN(!m_pGame, false);

	IGameFramework* pFramework = m_pGame->GetIGameFramework();
	CRY_ASSERT(pFramework);
	CRY_SAFE_RETURN(!pFramework, false);

	bool ok = false;
	if (on == m_bEnabled)
	{
		ok = true;
	}
	else if (on)
	{
		CryLogAlways( "EDITOR: Set game mode: on" );

		SGameContextParams ctx;

		SGameStartParams gameParams;
		gameParams.flags = eGSF_Server
			| eGSF_NoSpawnPlayer
			| eGSF_Client
			| eGSF_NoLevelLoading
			| eGSF_BlockingClientConnect
			| eGSF_NoGameRules
			| eGSF_LocalOnly
			| eGSF_NoQueries;
		gameParams.connectionString = "";
		gameParams.hostname = "localhost";
		gameParams.port = EDITOR_SERVER_PORT;
		gameParams.pContextParams = &ctx;
		gameParams.maxPlayers = 1;

		if (pFramework->StartGameContext( &gameParams ))
			ok = true;
	}
	else
	{
		CryLogAlways( "EDITOR: Set game mode: off" );

		pFramework->EndGameContext();
		gEnv->pNetwork->SyncWithGame(eNGS_Shutdown);
		ok = true;
	}

	m_bEnabled = on && ok;
	return ok;
}

//-----------------------------------

void CEditorGame::OnChangeEditorMode(ICVar* pVar)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(pVar);
	CRY_ASSERT(s_pEditorGameMode);
	CRY_ASSERT(pVar == s_pEditorGameMode);
	if (s_pEditorGame)
	{
		s_pEditorGame->SetGameMode( s_pEditorGame->m_bGameMode );
	}
}

//-----------------------------------

void CEditorGame::EnablePlayer(bool bPlayer)
{
	LOG_CODE_COVERAGE();

	bool spawnPlayer = false;
	if (m_bPlayer != bPlayer)
	{
		spawnPlayer = m_bPlayer = bPlayer;
	}
	if (!SetGameMode( m_bGameMode ))
	{
		GameWarning("Failed setting game mode");
	}
	else if (m_bEnabled && spawnPlayer)
	{
		CRY_ASSERT(m_pGame);
		CRY_SAFE_RETURN(!m_pGame, CRY_NO_RETURN_VALUE);

		IGameFramework* pFramework = m_pGame->GetIGameFramework();
		CRY_ASSERT(pFramework);
		CRY_SAFE_RETURN(!pFramework, CRY_NO_RETURN_VALUE);

		if (!pFramework->BlockingSpawnPlayer())
			GameWarning("Failed spawning player");
	}
}

//-----------------------------------

void CEditorGame::ResetClient(IConsoleCmdArgs*)
{
	LOG_CODE_COVERAGE();

	bool value = s_pEditorGame->m_bPlayer;
	s_pEditorGame->EnablePlayer(false);

	IEntityClass *pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Player");
	if (pClass)	pClass->LoadScript(true);

	if (value)
		s_pEditorGame->m_pGame->GetIGameFramework()->GetIGameRulesSystem()->CreateGameRules("SinglePlayer");
	s_pEditorGame->EnablePlayer(value);
}

// utility functions/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include UNIQUE_VIRTUAL_WRAPPER(IEditorGame)

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////