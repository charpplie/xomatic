/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 3:8:2004   11:26 : Created by Márcio Martins
  - 17:8:2005        : Modified - NickH: Factory registration moved to GameFactory.cpp

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include "GameActions.h"
#include "Menus/FlashMenuObject.h"
#include "Menus/OptionsManager.h"

#include "FrontEnd/FrontEnd.h"
#include "FrontEnd/FlashFrontEnd.h"
#include "FrontEnd/ProfileOptions.h"
#include "FrontEnd/WarningsManager.h"
#include "HUD/HUD.h"
#include "HUD/HUDCVars.h"
#include "HUD/HUDMissionObjectiveSystem.h"

#include "GameRules.h"
#include "Audio/GameAudio.h"
#include "ScreenEffects.h"
#include "WeaponSystem.h"
#include "Effects/GameEffectsSystem.h"

#include <ICryPak.h>
#include <CryPath.h>
#include <IActionMapManager.h>
#include <IViewSystem.h>
#include <ILevelSystem.h>
#include <IItemSystem.h>
#include <IVehicleSystem.h>
#include <IMovieSystem.h>
#include <IPlayerProfiles.h>

#include "ScriptBind_Actor.h"
#include "ScriptBind_Item.h"
#include "ScriptBind_Weapon.h"
#include "ScriptBind_GameRules.h"
#include "ScriptBind_Game.h"
#include "ScriptBind_HitDeathReactions.h"
#include "Boids/ScriptBind_Boids.h"
#include "AI/ScriptBind_GameAI.h"
#include "HUD/ScriptBind_HUD.h"
#include "Environment/ScriptBind_InteractiveObject.h"
#include "LaptopUtil.h"

#include "GameFactory.h"

#include "Player.h"

#include "GameParameters.h"
#include "WeaponSharedParams.h"

#include "Nodes/G2FlowBaseNode.h"

#include "ServerSynchedStorage.h"
#include "ClientSynchedStorage.h"

#include "ServerGameTokenSynch.h"
#include "ClientGameTokenSynch.h"

#include "SPAnalyst.h"

#include "ISaveGame.h"
#include "ILoadGame.h"
#include "CryPath.h"
#include "GameStateRecorder.h"
#include "RecordingSystem.h"
#include "StatsRecordingMgr.h"
#include "TelemetryCollector.h"
#include "TacticalManager.h"

#include "Environment/LedgeManager.h"

#include "Graphics/ColorGradientManager.h"
#include "VehicleClient.h"
#include "AI/TacticalPointLanguageExtender.h"
#include "AI/GameAISystem.h"

#include "Network/Lobby/GameBrowser.h"
#include "Network/Lobby/GameLobby.h"
#include "Network/Lobby/CryLobbySessionHandler.h"

#include "GameCodeCoverage/GameCodeCoverageManager.h"
#include "GameCodeCoverage/GameCodeCoverageGUI.h"

#include "AutoAimManager.h"
#include "HitDeathReactionsSystem.h"
#include "CheckpointGame.h"

#include "PlayerProgression.h"
#include "PersistantStats.h"
#include "Testing/FeatureTester.h"

#include "MikeBullet.h"

#include "GameMechanismManager/GameMechanismManager.h"

#if !defined(_RELEASE)
#include "Editor/GameRealtimeRemoteUpdate.h"
#endif 

#define GAME_DEBUG_MEM  // debug memory usage
#undef  GAME_DEBUG_MEM

#if defined(CRYSIS_BETA)
	#define CRYSIS_GUID "{CDC82B4A-7540-45A5-B92E-9A7C7033DBF4}"
#elif defined(SP_DEMO)
	#define CRYSIS_GUID "{CDC82B4A-7540-45A5-B92E-9A7C7033DBF3}"
#else
	#define CRYSIS_GUID "{CDC82B4A-7540-45A5-B92E-9A7C7033DBF2}"
#endif
// Needed for the Game02 specific flow node
CG2AutoRegFlowNodeBase *CG2AutoRegFlowNodeBase::m_pFirst=0;
CG2AutoRegFlowNodeBase *CG2AutoRegFlowNodeBase::m_pLast=0;

CGame *g_pGame = 0;
SCVars *g_pGameCVars = 0;
CGameActions *g_pGameActions = 0;
CTacticalPointLanguageExtender g_tacticalPointLanguageExtender;

CGame::CGame()
: m_pFramework(0),
	m_pConsole(0),
	m_pWeaponSystem(0),
	m_pFlashMenuObject(0),
	m_pFlashMenu(0),
	m_pOptionsManager(0),
	m_pScriptBindActor(0),
	m_pScriptBindGame(0),
	m_pScriptBindHitDeathReactions(0),
	m_pScriptBindBoids(0),
	m_pPlayerProfileManager(0),
	m_pMOSystem(0),
	m_pGameAudio(0),
	m_pScreenEffects(0),
	m_pServerGameTokenSynch(0),
	m_pClientGameTokenSynch(0),
	m_pServerSynchedStorage(0),
	m_pClientSynchedStorage(0),
	m_uiPlayerID(-1),
	m_pSPAnalyst(0),
	m_pPersistantStats(0),
	m_pLaptopUtil(0),
	m_pLedgeManager(0),
	m_colorGradientManager(0),
	m_pRecordingSystem(0),
	m_statsRecorder(0),
	m_telemetryCollector(0),
	m_performanceBuffer(0),
	m_bandwidthBuffer(0),
	m_memoryTrackingBuffer(0),
	m_pGameBrowser(0),
	m_pGameLobby(0),
	m_pLobbySessionHandler(0),
	m_pHud(NULL),
	m_pTacticalManager(NULL),
	m_pWarningsManager(NULL),
	m_pAutoAimManager(NULL),
	m_pHitDeathReactionsSystem(NULL),
	m_pScriptBindInteractiveObject(NULL),
	m_exclusiveControllerDeviceIndex(0),
	m_previousInputControllerDeviceIndex(0),
	m_hasExclusiveContoller(false)
{
	m_pCVars = new SCVars();
	g_pGameCVars = m_pCVars;
	m_pGameActions = new CGameActions();
	g_pGameActions = m_pGameActions;
	m_pTacticalManager = new CTacticalManager();
	m_pBurnEffectManager = new CBurnEffectManager();
	g_pGame = this;
	m_bReload = false;
	m_inDevMode = false;
	m_gameTypeInited = false;

#if ENABLE_GAME_CODE_COVERAGE
	static CGameCodeCoverageManager   s_gameCodeCoverageManager("Scripts/gameCodeCoverage.xml");
	static CGameCodeCoverageGUI       s_gameCodeCoverageGUI;
#endif

#if !defined(_RELEASE)
	m_pRemoteUpdateListener=&CGameRealtimeRemoteUpdateListener::GetGameRealtimeRemoteUpdateListener();
#endif

	m_pDefaultAM = 0;

	RegisterAnimActions();
	GetISystem()->SetIGame( this );
}

CGame::~CGame()
{
	m_pFramework->EndGameContext();
	m_pFramework->SetGameSessionHandler(0); // Deletes the current session handler
	m_pFramework->UnregisterListener(this);
	ReleaseScriptBinds();
	ReleaseActionMaps();
	SAFE_DELETE(m_pFlashMenuObject);
	SAFE_DELETE(m_pFlashMenu);
	SAFE_DELETE(m_pOptionsManager);
	SAFE_DELETE(m_pProfileOptions);
	SAFE_DELETE(m_pLaptopUtil);
	SAFE_DELETE(m_pMOSystem);
	SAFE_DELETE(m_pGameAudio);
	SAFE_DELETE(m_pScreenEffects);
	SAFE_DELETE(m_pSPAnalyst);
	SAFE_DELETE(m_pPersistantStats);
	m_pWeaponSystem->Release();
	SAFE_DELETE(m_pItemStrings);
	SAFE_DELETE(m_pGameParametersStorage);
	SAFE_DELETE(m_pHud);
	SAFE_DELETE(m_pCVars);
	SAFE_DELETE(m_pLedgeManager);
	SAFE_DELETE(m_pRecordingSystem);
	SAFE_DELETE(m_statsRecorder);
	SAFE_DELETE(m_telemetryCollector);
	SAFE_DELETE(m_performanceBuffer);
	SAFE_DELETE(m_bandwidthBuffer);
	SAFE_DELETE(m_memoryTrackingBuffer);
	SAFE_DELETE(m_colorGradientManager);
	SAFE_DELETE(m_pGameBrowser);
	SAFE_DELETE(m_pGameLobby);
	SAFE_DELETE(m_pTacticalManager);
	SAFE_DELETE(m_pBurnEffectManager);
	SAFE_DELETE(m_pWarningsManager);
	SAFE_DELETE(m_pAutoAimManager);
	SAFE_DELETE(m_pHitDeathReactionsSystem);
	SAFE_DELETE(m_pLobbySessionHandler);
	GAME_FX_SYSTEM.Destroy();

	if(gEnv->pInput)
	{
		gEnv->pInput->RemoveEventListener(this);
	}

	// Delete this after everything else...
	// some of the things deleted above could well be game mechanisms, so we need
	// the game mechanism manager to still exist when they're deleted [TF]
	SAFE_DELETE(m_gameMechanismManager);

	g_pGame = 0;
	g_pGameCVars = 0;
	g_pGameActions = 0;
}

bool CGame::Init(IGameFramework *pFramework)
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

	m_gameMechanismManager = new CGameMechanismManager();

#ifdef GAME_DEBUG_MEM
	DumpMemInfo("CGame::Init start");
#endif

	m_pFramework = pFramework;
	assert(m_pFramework);

	m_pConsole = gEnv->pConsole;

	RegisterConsoleVars();
	RegisterConsoleCommands();
	RegisterGameObjectEvents();

	// Initialize static item strings
	m_pItemStrings = new SItemStrings();

	m_pGameParametersStorage = new CGameSharedParametersStorage();

	LoadActionMaps();

	InitScriptBinds();

	//load user levelnames for ingame text and savegames
	XmlNodeRef lnames = GetISystem()->LoadXmlFile("Scripts/GameRules/LevelNames.xml");
	if(lnames)
	{
		int num = lnames->getNumAttributes();
		const char *nameA, *nameB;
		for(int n = 0; n < num; ++n)
		{
			lnames->getAttributeByIndex(n, &nameA, &nameB);
			m_mapNames[string(nameA)] = string(nameB);
		}
	}

	  // Register all the games factory classes e.g. maps "Player" to CPlayer
	  InitGameFactory(m_pFramework);

		
	m_pWeaponSystem = new CWeaponSystem(this, GetISystem());

	string itemFolder = "scripts/entities/items/crysis2";
	pFramework->GetIItemSystem()->Scan(itemFolder.c_str());
	m_pWeaponSystem->Scan(itemFolder.c_str());

	string actorParamsFolder = "scripts/entities/actor/parameters";
	m_pFramework->GetIActorSystem()->Scan(actorParamsFolder);

	m_pOptionsManager = COptionsManager::CreateOptionsManager();

	m_pSPAnalyst = new CSPAnalyst();

	m_pAutoAimManager = new CAutoAimManager();

	m_pHitDeathReactionsSystem = new CHitDeathReactionsSystem;
	CRY_ASSERT(m_pHitDeathReactionsSystem);

 
	gEnv->pConsole->CreateKeyBind("f12", "r_getscreenshot 2");

	//Ivo: initialites the Crysis conversion file.
	//this is a conversion solution for the Crysis game DLL. Other projects don't need it.
	// No need anymore
	//gEnv->pCharacterManager->LoadCharacterConversionFile("Objects/CrysisCharacterConversion.ccc");

	// set game GUID
	m_pFramework->SetGameGUID(CRYSIS_GUID);

	// TEMP
	// Load the action map beforehand (see above)
	// afterwards load the user's profile whose action maps get merged with default's action map
	m_pPlayerProfileManager = m_pFramework->GetIPlayerProfileManager();

	bool bIsFirstTime = false;
	const bool bResetProfile = gEnv->pSystem->GetICmdLine()->FindArg(eCLAT_Pre,"ResetProfile") != 0;
	if (m_pPlayerProfileManager)
	{
		const char* userName = gEnv->pSystem->GetUserName();

		bool ok = m_pPlayerProfileManager->LoginUser(userName, bIsFirstTime);
		if (ok)
		{

			// activate the always present profile "default"
			int profileCount = m_pPlayerProfileManager->GetProfileCount(userName);
			if (profileCount > 0)
			{
				bool handled = false;
				if(gEnv->pSystem->IsDedicated())
				{
					for(int i = 0; i < profileCount; ++i )
					{
						IPlayerProfileManager::SProfileDescription profDesc;
						bool ok = m_pPlayerProfileManager->GetProfileInfo(userName, i, profDesc);
						if(ok)
						{
							const IPlayerProfile *preview = m_pPlayerProfileManager->PreviewProfile(userName, profDesc.name);
							int iActive = 0;
							if(preview)
							{
								preview->GetAttribute("Activated",iActive);
							}
							if(iActive>0)
							{
								m_pPlayerProfileManager->ActivateProfile(userName,profDesc.name);
								CryLogAlways("[GameProfiles]: Successfully activated profile '%s' for user '%s'", profDesc.name, userName);
								m_pFramework->GetILevelSystem()->LoadRotation();
								handled = true;
								break;
							}
						}
					}
					m_pPlayerProfileManager->PreviewProfile(userName,NULL);
				}

				if(!handled)
				{
					IPlayerProfileManager::SProfileDescription desc;
					ok = m_pPlayerProfileManager->GetProfileInfo(userName, 0, desc);
					if (ok)
					{
						IPlayerProfile* pProfile = m_pPlayerProfileManager->ActivateProfile(userName, desc.name);

						if (pProfile == 0)
						{
							GameWarning("[GameProfiles]: Cannot activate profile '%s' for user '%s'. Trying to re-create.", desc.name, userName);
							IPlayerProfileManager::EProfileOperationResult profileResult;
							m_pPlayerProfileManager->CreateProfile(userName, desc.name, true, profileResult); // override if present!
							pProfile = m_pPlayerProfileManager->ActivateProfile(userName, desc.name);
							if (pProfile == 0)
								GameWarning("[GameProfiles]: Cannot activate profile '%s' for user '%s'.", desc.name, userName);
							else
								GameWarning("[GameProfiles]: Successfully re-created profile '%s' for user '%s'.", desc.name, userName);
						}

						if (pProfile)
						{
							if (bResetProfile)
							{
								bIsFirstTime = true;
								pProfile->Reset();
								gEnv->pCryPak->RemoveFile("%USER%/game.cfg");
								CryLogAlways("[GameProfiles]: Successfully reset and activated profile '%s' for user '%s'", desc.name, userName);
							}
							CryLogAlways("[GameProfiles]: Successfully activated profile '%s' for user '%s'", desc.name, userName);
							m_pFramework->GetILevelSystem()->LoadRotation();

							if (bIsFirstTime)
							{
								CRY_TODO(12,01,2010, "Find a nicer place to have differing default profile settings for multiplayer");
								// note: bIsFirstTime is set, meaning we've just had default profile copied to create a new player profile
								if (g_pGameCVars->g_multiplayerDefault)
								{
									if (ICVar* pVar=gEnv->pConsole->GetCVar("cl_sensitivityController"))
									{
										pVar->Set(g_pGameCVars->cl_mp_sensitivityControllerDefault);
										pProfile->SetAttribute("Sensitivity", g_pGameCVars->cl_mp_sensitivityControllerDefault);
									}
								}
							}
						}
					}
					else
					{
						GameWarning("[GameProfiles]: Cannot get profile info for user '%s'", userName);
					}
				}
			}
			else
			{
				GameWarning("[GameProfiles]: User 'dude' has no profiles");
			}
		}
		else
			GameWarning("[GameProfiles]: Cannot login user '%s'", userName);
	}
	else
		GameWarning("[GameProfiles]: PlayerProfileManager not available. Running without.");

	m_pProfileOptions = new CProfileOptions();
	if (m_pOptionsManager)
	{
		m_pOptionsManager->SetProfileManager(m_pPlayerProfileManager);
	}

	// CLaptopUtil must be created before CFlashMenuObject as this one relies on it
	if(!m_pLaptopUtil)
		m_pLaptopUtil = new CLaptopUtil;
		
	if (!gEnv->bMultiplayer)
	{
		const int iCVarDifficulty = g_pGameCVars->g_difficultyLevel;
		EDifficulty difficulty = (iCVarDifficulty >= eDifficulty_Default && iCVarDifficulty < eDifficulty_COUNT ? (EDifficulty)iCVarDifficulty : eDifficulty_Default);
		SetDifficultyLevel(difficulty);
	}
		
	if (!m_pGameAudio)
	{
		m_pGameAudio = new CGameAudio();
	}

	if(!gEnv->bEditor)
	{
		// stats recorder now available in SP as well
		m_statsRecorder=new CStatsRecordingMgr;
	}

	// START FIXME
	CRY_FIXME( 24, 02, 2010, "Temporarily added early cvar sets so that game init can use them to pick the correct sp/mp game. Needs to be removed when a cleaner MP/SP split is there.");
	// Abusing the pre cmd line args but necessary so that the current game type cvars are read to use for game init
	if (const ICmdLineArg *pCmdArg = gEnv->pSystem->GetICmdLine()->FindArg(eCLAT_Pre,"g_enableGameSwitchMenu"))
	{
		g_pGameCVars->g_enableGameSwitchMenu = pCmdArg->GetIValue();
	}

	if (const ICmdLineArg *pCmdArg = gEnv->pSystem->GetICmdLine()->FindArg(eCLAT_Pre,"g_multiplayerDefault"))
	{
		g_pGameCVars->g_multiplayerDefault = pCmdArg->GetIValue();
	}
	// END FIXME

	if (!( gEnv->pSystem->IsDedicated() || gEnv->IsEditor() ))
	{
		m_pFlashMenu = new CFlashFrontEnd();
		if (g_pGameCVars->g_enableGameSwitchMenu)
		{
			m_pFlashMenu->ScheduleInitialize(CFlashFrontEnd::eFlM_GameSelect);
		}
		else
		{
			InitGameType(g_pGameCVars->g_multiplayerDefault!=0);
		}
	}
	else
	{
		InitGameType(g_pGameCVars->g_multiplayerDefault!=0);
	}

	if (bIsFirstTime)
	{
		if ((m_pOptionsManager && m_pOptionsManager->IsFirstStart()) || bResetProfile)
		{
			CryLogAlways("[GameProfiles]: PlayerProfileManager reported first-time login. Running AutoDetectSpec.");
			// run autodetectspec
			gEnv->pSystem->AutoDetectSpec();

			if (m_pOptionsManager)
			{
				m_pOptionsManager->SystemConfigChanged(true);
			}
		}
		else
		{
			CryLogAlways("[GameProfiles]: PlayerProfileManager reported first-time login. AutoDetectSpec NOT running because g_startFirstTime=0.");
		}
	}

	if (!m_pServerSynchedStorage)
		m_pServerSynchedStorage = new CServerSynchedStorage(GetIGameFramework());

	if (!m_pServerGameTokenSynch)
	{
		m_pServerGameTokenSynch = new CServerGameTokenSynch(m_pFramework->GetIGameTokenSystem());
	}

	if(!m_pMOSystem)
	{
		m_pMOSystem = new CHUDMissionObjectiveSystem();
	}

	if (!m_pScreenEffects)
	{
		m_pScreenEffects = new CScreenEffects();
	}

	if (!m_pLedgeManager)
	{
		m_pLedgeManager = new CLedgeManager();
	}

	m_colorGradientManager = new Graphics::CColorGradientManager();

  m_pFramework->RegisterListener(this,"Game", FRAMEWORKLISTENERPRIORITY_GAME);

  CVehicleClient *pVehicleClient = new CVehicleClient();
  pVehicleClient->Init();
  g_pGame->GetIGameFramework()->GetIVehicleSystem()->RegisterVehicleClient(pVehicleClient);

#ifdef GAME_DEBUG_MEM
	DumpMemInfo("CGame::Init end");
#endif

	g_tacticalPointLanguageExtender.Initialize();

	// HUD is always available, but will be loaded when player is joining.
	m_pHud = new CHUD();
	m_pWarningsManager = new CWarningsManager();

	GAME_FX_SYSTEM.Initialise();

	// Initialise game handler for checkpoints
	CCheckpointGame::GetInstance()->Init();

	CPlayerProgression* pPlayerProgression = new CPlayerProgression();
	pPlayerProgression->Init();

	m_pPersistantStats = new CPersistantStats();
	m_pPersistantStats->Init();

#if ENABLE_FEATURE_TESTER
	new CFeatureTester();
#endif

	if(gEnv->pInput)
	{
		gEnv->pInput->SetExclusiveListener(this);
	}

	return true;
}

bool CGame::CompleteInit()
{
	// Initialize Game02 flow nodes

	if (IFlowSystem *pFlow = m_pFramework->GetIFlowSystem())
	{
		CG2AutoRegFlowNodeBase *pFactory = CG2AutoRegFlowNodeBase::m_pFirst;

		while (pFactory)
		{
			pFlow->RegisterType( pFactory->m_sClassName,pFactory );
			pFactory = pFactory->m_pNext;
		}
	}

#ifdef GAME_DEBUG_MEM
	DumpMemInfo("CGame::CompleteInit");
#endif
	return true;
}

// Loads and Inits either singleplayer or multiplayer
// Called when selecting from the selection frontend
// or if g_enableGameSwitchMenu==0 on game Init()
void CGame::InitGameType(bool multiplayer)
{
	CRY_TODO(17, 02, 2010, "Switching between singleplayer/multiplayer will need revisiting to correctly create/destroy game specific classes. Currently it will only init the one you choose.");

	CRY_ASSERT(m_gameTypeInited==false);

	const char * configName = "singleplayer";
	if (multiplayer)
		configName = "multiplayer";

	// Load config file
	gEnv->pSystem->LoadConfiguration(configName);

	if(multiplayer)
	{
		if (!gEnv->bEditor)
		{
			// Multiplayer Init
			CryLog("CGame: Multiplayer game type initialised");
			g_pGameCVars->g_multiplayerDefault = 1;
			// Moved setting of these cvars into the multiplayer.cfg
			//ICVar *pTfrc = gEnv->pConsole->GetCVar("net_enable_tfrc");
			//pTfrc->Set(0);
			//ICVar *pBattleDust = gEnv->pConsole->GetCVar("g_battleDust_enable");
			//pBattleDust->Set(0);
			//ICVar *pBandwidth = gEnv->pConsole->GetCVar("sv_bandwidth");
			//pBandwidth->Set(30000);

			// TODO : Needs to be done before _any_ lobbying code is executed
			m_pGameBrowser = new CGameBrowser();
			m_pGameLobby = new CGameLobby();
			m_pLobbySessionHandler = new CCryLobbySessionHandler();

			// telemetry only used by multiplayer for now
			m_telemetryCollector=new CTelemetryCollector;

			m_performanceBuffer = new CTelemetryBuffer(30*1024, m_telemetryCollector, sizeof(SPerformanceTelemetry));
			m_bandwidthBuffer = new CTelemetryBuffer(60*1024, m_telemetryCollector, sizeof(SBandwidthTelemetry));
			m_memoryTrackingBuffer = new CTelemetryBuffer(30*1024, m_telemetryCollector, sizeof(SMemoryTelemetry));

			m_pRecordingSystem = new CRecordingSystem();
		}
	}
	else
	{
		// Singleplayer Init
		CryLog("CGame: Singleplayer game type initialised");
		g_pGameCVars->g_multiplayerDefault = 0;

		if(g_pGameCVars->g_telemetryEnabledSP != 0 && !gEnv->bEditor)
		{
			gEnv->pNetwork->GetLobby()->Initialise(eCLS_LAN, NULL, NULL, NULL);

			m_telemetryCollector=new CTelemetryCollector;
		}
	}

	// Load menu for game
	if (m_pFlashMenu)
	{
		m_pFlashMenu->Clear();
		m_pFlashMenu->ScheduleInitialize(CFlashFrontEnd::eFlM_Menu);
	}

	m_gameTypeInited = true;
}

void CGame::SetDifficultyLevel(EDifficulty difficulty)
{
	assert(!gEnv->bMultiplayer);

	static const char *szDifficultyConfigs[eDifficulty_COUNT] = 
	{
		"diff_normal.cfg",	// Default '0' for normal

		"diff_easy.cfg",
		"diff_normal.cfg",
		"diff_hard.cfg",
		"diff_bauer.cfg"
	};

	if (!gEnv->bMultiplayer)
	{
		if (difficulty < eDifficulty_Default || difficulty >= eDifficulty_COUNT)
			difficulty = eDifficulty_Default;

		const char* szConfig = szDifficultyConfigs[difficulty];
		CDifficultyConfigSink cvarSink("CGame::SetDifficultyLevel");
		CryLog("CGame: Loading singleplayer difficulty config \'%s\'", szConfig);
		gEnv->pSystem->LoadConfiguration(szConfig, &cvarSink);

		g_pGameCVars->g_difficultyLevel = difficulty;
	}
}

int CGame::Update(bool haveFocus, unsigned int updateFlags)
{
	bool bRun = m_pFramework->PreUpdate( true, updateFlags );

	float frameTime = gEnv->pTimer->GetFrameTime();
	
	m_colorGradientManager->UpdateForThisFrame(frameTime);

	if (m_pFramework->IsGamePaused() == false)
	{
		m_pWeaponSystem->Update(frameTime);
		m_pScreenEffects->Update(frameTime);
		m_pAutoAimManager->Update(frameTime);
		m_playerVisTable.Update(frameTime);
		CGameAISystem::GetInstance()->Update(frameTime);
		GAME_FX_SYSTEM.Update(frameTime);
		m_pBurnEffectManager->Update(frameTime);
	}

	m_gameMechanismManager->Update(frameTime);

	if (m_pRecordingSystem)
	{
		m_pRecordingSystem->Update(frameTime);
	}

	m_pPersistantStats->Update(frameTime);

#ifndef DISABLE_OM
	//////////////////////////////////////////////////////////////////////////
	if (m_OrganicMotionClient.bConnected)
		OMUpdate();
	//////////////////////////////////////////////////////////////////////////
#endif

#if ENABLE_GAME_CODE_COVERAGE
	if (CGameCodeCoverageManager::GetInstance()->Update(frameTime))
	{
		CGameCodeCoverageGUI::GetInstance()->Draw();
	}
#endif

#if !defined(_RELEASE)
	m_pRemoteUpdateListener->Update();
#endif 

	m_pFramework->PostUpdate( true, updateFlags );
	
	if (m_pRecordingSystem)
	{
		m_pRecordingSystem->PostUpdate();
	}

	{
		//Beni - For some reason, radial blur FX parameters have to be updated here (not during update)
		m_pScreenEffects->PostUpdate(frameTime);
	}

	if(m_inDevMode != gEnv->pSystem->IsDevMode())
	{
		m_inDevMode = gEnv->pSystem->IsDevMode();
	}

	IActionMapManager *pAMM = m_pFramework->GetIActionMapManager();
	CRY_ASSERT(pAMM);
	IActionMap *pAM = pAMM->GetActionMap("debug");
	if (pAM && pAM->Enabled() != m_inDevMode)
	{
		pAM->Enable(m_inDevMode);
	}

	CheckReloadLevel();

#if NET_PROFILE_ENABLE
	if( g_pGameCVars->g_multiplayerDefault && !gEnv->bEditor && (gEnv->pConsole->GetCVar("net_profile_logging")->GetIVal() != 0) && m_telemetryCollector)
	{
		static ICVar *pName = gEnv->pConsole->GetCVar("net_profile_logname");
		static CTimeValue s_timeValue = gEnv->pTimer->GetAsyncCurTime();
		CTimeValue timeValue = gEnv->pTimer->GetAsyncCurTime();

		if (timeValue - s_timeValue >= CTimeValue(10.f))
		{
			CryFixedStringT<1024> filename;
			const char *fullFilename = pName->GetString();
			const char *forwardSlash = strrchr(fullFilename, '/');
			const char *backSlash = strrchr(fullFilename, '\\');
			const char *lastSlash = (forwardSlash > backSlash) ? forwardSlash : backSlash;
			lastSlash = (lastSlash) ? lastSlash : fullFilename;
			filename.Format("./%s", lastSlash);
			m_telemetryCollector->SubmitFile(filename.c_str());
			s_timeValue = timeValue;
		}
	}
#endif // #if NET_PROFILE_ENABLE
	if( g_pGameCVars->g_multiplayerDefault && !gEnv->bEditor && m_performanceBuffer && m_bandwidthBuffer && m_memoryTrackingBuffer)
	{
		CGameRules *cgr = GetGameRules();
		if (cgr)
		{
			const CTimeValue timeValue = gEnv->pTimer->GetFrameStartTime();

			static CTimeValue s_secondTimePerformance = timeValue;
			static CTimeValue s_secondTimeMemory = timeValue;
			static CTimeValue s_secondTimeBandwidth = timeValue;
			static float s_gpuTime = 0.f;
			static int s_gpuLimited = 0;

			const float serverTimeInSeconds = cgr->GetServerTime() / 1000.0f;

			static int framerateFrameCount = 0;
			framerateFrameCount++;

	
			CTimeValue deltaTime;
			//Render stats must be updated each frame
			IRenderer* pRenderer = gEnv->pRenderer;
			if(pRenderer)
			{
				IRenderer::SRenderTimes renderTimes;

				pRenderer->GetRenderTimes(renderTimes);

				float gpuFrameTime = pRenderer->GetGPUFrameTime();

				if(gpuFrameTime>0.f)
				{
					s_gpuTime += gpuFrameTime;
				}

				//wait for GPU is never zero, using small epsilon to determine if GPU Limited
				const float GPU_EPSILON = 0.0001f;
				s_gpuLimited += (renderTimes.fWaitForGPU>GPU_EPSILON) ? 1 : 0;
			}


			const float sampleTimePerformance = g_pGameCVars->g_telemetrySampleRatePerformance;
			deltaTime = timeValue - s_secondTimePerformance;
			if (deltaTime >= sampleTimePerformance)
			{
				SPerformanceTelemetry spt;
				spt.m_timeInSeconds = serverTimeInSeconds;
				spt.m_numTicks = framerateFrameCount;
				spt.m_gpuTime = s_gpuTime;
				spt.m_gpuLimited = s_gpuLimited;

				m_performanceBuffer->AddData(&spt);

				s_secondTimePerformance = timeValue;
				framerateFrameCount = 0;
				s_gpuTime = 0.f; 
				s_gpuLimited = 0;
			}

			const float sampleTimeBandwidth = g_pGameCVars->g_telemetrySampleRateBandwidth;
			deltaTime = timeValue - s_secondTimeBandwidth;
			if (deltaTime >= sampleTimeBandwidth)
			{
				INetwork* pNetwork = gEnv->pNetwork;
				SBandwidthStats stats;
				pNetwork->GetBandwidthStatistics(&stats);

				SBandwidthTelemetry sbt;
				sbt.m_timeInSeconds = serverTimeInSeconds;
				sbt.m_bandwidthReceived = stats.m_socketBandwidthRecvd;
				sbt.m_bandwidthSent = stats.m_socketBandwidthSent;
				sbt.m_packetsSent = stats.m_numPacketsSent;
				m_bandwidthBuffer->AddData(&sbt);

				s_secondTimeBandwidth = timeValue;
			}

			const float sampleTimeMemory = g_pGameCVars->g_telemetrySampleRateMemory;
			deltaTime = timeValue - s_secondTimeMemory;
			if (deltaTime >= sampleTimeMemory)
			{
				IMemoryManager::SProcessMemInfo processMemInfo;
				GetISystem()->GetIMemoryManager()->GetProcessMemInfo(processMemInfo);
				const float cpuMemUsedInMB = (float)(processMemInfo.PagefileUsage)/(1024.0f*1024.0f);

				SMemoryTelemetry memTelem;
				memTelem.m_timeInSeconds = serverTimeInSeconds;
				memTelem.m_cpuMemUsedInMB = cpuMemUsedInMB;
				m_memoryTrackingBuffer->AddData(&memTelem);

				s_secondTimeMemory = timeValue;
			}
		}
	}
	
	return bRun ? 1 : 0;
}

void CGame::ConfigureGameChannel(bool isServer, IProtocolBuilder *pBuilder)
{
	if (isServer)
	{
		m_pServerSynchedStorage->DefineProtocol(pBuilder);
		m_pServerGameTokenSynch->DefineProtocol(pBuilder);
	}
	else
	{
		m_pClientSynchedStorage = new CClientSynchedStorage(GetIGameFramework());
		m_pClientSynchedStorage->DefineProtocol(pBuilder);

		m_pClientGameTokenSynch = new CClientGameTokenSynch(m_pFramework->GetIGameTokenSystem());
		m_pClientGameTokenSynch->DefineProtocol(pBuilder);
	}
}

void CGame::EditorResetGame(bool bStart)
{
	CRY_ASSERT(gEnv->IsEditor());

	if(bStart)
	{
		IActionMapManager* pAM = m_pFramework->GetIActionMapManager();
		if (pAM)
		{
			pAM->EnableActionMap(0, true); // enable all action maps
			pAM->EnableFilter(0, false); // disable all filters
		}

		// load & reset hud and related data.
		if( m_pHud )
		{
			CHUDCVars::ShowHUD(true);
			m_pHud->ActivateDefaultState();
			CHUD::CallEvent(SHUDEvent(eHUDEvent_OnHUDReload));
		}
	}
	else
	{
		// Hide hud
		if( m_pHud )
		{
			m_pHud->ActivateState( "no_hud" );
			CHUDCVars::ShowHUD(false);
		}

		m_pBurnEffectManager->Reset();
	}
}

void CGame::PlayerIdSet(EntityId playerId)
{
	m_uiPlayerID = playerId;
}

string CGame::InitMapReloading()
{
	string levelFileName = GetIGameFramework()->GetLevelName();
	levelFileName = PathUtil::GetFileName(levelFileName);
	if(const char* visibleName = GetMappedLevelName(levelFileName))
		levelFileName = visibleName;
	//levelFileName.append("_levelstart.crysisjmsf"); //because of the french law we can't do this ...
	levelFileName.append("_crysis.crysisjmsf");
	bool foundSaveGame = false;
	if (m_pPlayerProfileManager)
	{
		const char* userName = GetISystem()->GetUserName();
		IPlayerProfile* pProfile = m_pPlayerProfileManager->GetCurrentProfile(userName);
		if (pProfile)
		{
			const char* sharedSaveGameFolder = m_pPlayerProfileManager->GetSharedSaveGameFolder();
			if (sharedSaveGameFolder && *sharedSaveGameFolder)
			{
				string prefix = pProfile->GetName();
				prefix+="_";
				levelFileName = prefix + levelFileName;
			}
			ISaveGameEnumeratorPtr pSGE = pProfile->CreateSaveGameEnumerator();
			ISaveGameEnumerator::SGameDescription desc;	
			const int nSaveGames = pSGE->GetCount();
			for (int i=0; i<nSaveGames; ++i)
			{
				if (pSGE->GetDescription(i, desc))
				{
					if(!stricmp(desc.name,levelFileName.c_str()))
					{
						m_bReload = true;
						return levelFileName;
					}
				}
			}
		}
	}
#ifndef WIN32
	m_bReload = true; //using map command
#else
	m_bReload = false;
	levelFileName.clear();
#endif
	return levelFileName;
}

void CGame::Shutdown()
{
	g_tacticalPointLanguageExtender.Deinitialize();

	if (m_pPlayerProfileManager)
	{
		m_pPlayerProfileManager->LogoutUser(m_pPlayerProfileManager->GetCurrentUser());
	}

	IVehicleSystem * pVehicleSystem = g_pGame->GetIGameFramework()->GetIVehicleSystem();
	IVehicleClient *pVehicleClient = pVehicleSystem->GetVehicleClient();
	pVehicleSystem->RegisterVehicleClient(NULL);
	SAFE_DELETE(pVehicleClient);

	delete m_pServerSynchedStorage;
	m_pServerSynchedStorage	= 0;

	delete m_pServerGameTokenSynch;
	m_pServerGameTokenSynch = 0;

	this->~CGame();
}

const char *CGame::GetLongName()
{
	CRY_TODO(22, 2, 2010, "Can we remove this function (and the matching pure virtual function in IGame.h)? It is never called and does something different to what the comments in IGame.h say it does! [TF]");
	return "Crysis";
}

const char *CGame::GetName()
{
	CRY_TODO(22, 2, 2010, "Can we remove this function (and the matching pure virtual function in IGame.h)? It is never called and does something different to what the comments in IGame.h say it does! [TF]");
	return "Crysis";
}

EPlatform CGame::GetPlatform() const
{
	EPlatform platform = ePlatform_Unknown;

#if defined(PS3)
	platform = ePlatform_PS3;
#elif defined(XENON)
	platform = ePlatform_Xbox;
#elif defined(WIN32) || defined(WIN64)
	platform = ePlatform_PC;
#endif

	return platform;
}

void CGame::OnPostUpdate(float fDeltaTime)
{
}

void CGame::OnSaveGame(ISaveGame* pSaveGame)
{
	CPlayer *pPlayer = static_cast<CPlayer*>(GetIGameFramework()->GetClientActor());
	GetGameRules()->PlayerPosForRespawn(pPlayer, true);

	//save difficulty
	pSaveGame->AddMetadata("sp_difficulty", g_pGameCVars->g_difficultyLevel);

	//write file to profile
	if(m_pPlayerProfileManager)
	{
		const char* saveGameFolder = m_pPlayerProfileManager->GetSharedSaveGameFolder();
		const bool bSaveGameFolderShared = saveGameFolder && *saveGameFolder;
		const char *user = m_pPlayerProfileManager->GetCurrentUser();
		if(IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(user))
		{
			string filename(pSaveGame->GetFileName());
			CryFixedStringT<128> profilename(pProfile->GetName());
			profilename+='_';
			filename = filename.substr(filename.rfind('/')+1);
			// strip profileName_ prefix
			if (bSaveGameFolderShared)
			{
				if(strnicmp(filename.c_str(), profilename.c_str(), profilename.length()) == 0)
					filename = filename.substr(profilename.length());
			}
			pProfile->SetAttribute("Singleplayer.LastSavedGame", filename);
		}
	}

	pSaveGame->AddMetadata("v_altitudeLimit", g_pGameCVars->pAltitudeLimitCVar->GetString());
}

void CGame::OnLoadGame(ILoadGame* pLoadGame)
{
	m_pMOSystem->DeactivateObjectives( false );
	int difficulty = g_pGameCVars->g_difficultyLevel;
	pLoadGame->GetMetadata("sp_difficulty", difficulty);
	if(difficulty != g_pGameCVars->g_difficultyLevel)
	{
		//ICVar *diff = gEnv->pConsole->GetCVar("g_difficultyLevel");
		//if(diff)
		//{
			//string diffVal = "Option.";
			//diffVal.append(diff->GetName());
			//GetOptions()->SaveCVarToProfile(diffVal.c_str(), diff->GetString());
			IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(m_pPlayerProfileManager->GetCurrentUser());
			if(pProfile)
			{
				pProfile->SetAttribute("Singleplayer.LastSelectedDifficulty", difficulty);
				pProfile->SetAttribute("Option.g_difficultyLevel", difficulty);
				IPlayerProfileManager::EProfileOperationResult result;
				m_pPlayerProfileManager->SaveProfile(m_pPlayerProfileManager->GetCurrentUser(), result);
			}
		//}
	}

	// altitude limit
	const char* v_altitudeLimit =	pLoadGame->GetMetadata("v_altitudeLimit");
	if (v_altitudeLimit && *v_altitudeLimit)
		g_pGameCVars->pAltitudeLimitCVar->ForceSet(v_altitudeLimit);
	else
	{
		CryFixedStringT<128> buf;
		buf.FormatFast("%g", g_pGameCVars->v_altitudeLimitDefault());
		g_pGameCVars->pAltitudeLimitCVar->ForceSet(buf.c_str());
	}
}

// All input is filtered through this function, if return true then other listeners will not recieve the input
bool CGame::OnInputEvent(const SInputEvent& inputEvent)
{
	bool isGamePadController = (inputEvent.deviceId==eDI_XI);
/* //Diesel cut
	if(isGamePadController)
	{
		// Store device index of controller providing input
		bool isConnectionChangeEvent = ((inputEvent.keyId == eKI_SYS_ConnectDevice) || (inputEvent.keyId == eKI_SYS_DisconnectDevice) ||
																		(inputEvent.keyId == eKI_XI_Connect) || (inputEvent.keyId == eKI_XI_Disconnect));
		if(isConnectionChangeEvent == false) // Only want to set device index when real input comes through, not device changes
		{
			m_previousInputControllerDeviceIndex = inputEvent.deviceIndex;
		}

		// If there is an exclusive controller, then ignore other controllers
		if(	m_hasExclusiveContoller && (inputEvent.deviceIndex != m_exclusiveControllerDeviceIndex) )
		{
			return true; // Return true so that other listeners won't recieve this event
		}

		// Add any game specific handling of controllers connecting/disconnecting here:-
		switch(inputEvent.keyId)
		{
			case eKI_SYS_ConnectDevice:
			{
				// Controller connected
#if defined(PS3)
				CRY_TODO(22,3,2010,"Ben Johnson to hide controller disconnect msg for PS3");
#endif
				break;
			}
			case eKI_SYS_DisconnectDevice:
			{
				// Controller disconnected
#if defined(PS3)
				CRY_TODO(22,3,2010,"Ben Johnson to add controller disconnect msg for PS3");
				// eg please reconnect controller (m_exclusiveControllerDeviceIndex+1)
#endif
				break;
			}
		}
	}
*/ //Diesel cut
	return false; // Return false so that other listeners will get this event
}

void CGame::SetExclusiveControllerFromPreviousInput()
{
	m_hasExclusiveContoller = true;
	m_exclusiveControllerDeviceIndex = m_previousInputControllerDeviceIndex;
}

void CGame::RemoveExclusiveController()
{
	m_hasExclusiveContoller = false;
}


void CGame::OnActionEvent(const SActionEvent& event)
{
	switch(event.m_event)
  {
	case eAE_connectFailed:
		CCCPOINT(Net_ConnectFailed);
		break;
  case  eAE_channelDestroyed:
		CCCPOINT(Net_ChannelDestroyed);
    GameChannelDestroyed(event.m_value == 1);
    break;
	case eAE_serverIp:
		CCCPOINT(Net_GetServerIp);
		if(gEnv->bServer && GetServerSynchedStorage())
		{
			GetServerSynchedStorage()->SetGlobalValue(GLOBAL_SERVER_IP_KEY,string(event.m_description));
			GetServerSynchedStorage()->SetGlobalValue(GLOBAL_SERVER_PUBLIC_PORT_KEY,event.m_value);
		}
		break;
	case eAE_serverName:
		CCCPOINT(Net_GetServerName);
		if(gEnv->bServer && GetServerSynchedStorage())
			GetServerSynchedStorage()->SetGlobalValue(GLOBAL_SERVER_NAME_KEY,string(event.m_description));
		break;
	case eAE_earlyPreUpdate:
		break;
	}
}

void CGame::GameChannelDestroyed(bool isServer)
{
  if (!isServer)
  {
    delete m_pClientSynchedStorage;
    m_pClientSynchedStorage=0;

		delete m_pClientGameTokenSynch;
		m_pClientGameTokenSynch=0;

		if (!gEnv->pSystem->IsSerializingFile())
		{
			CryFixedStringT<128> buf;
			buf.FormatFast("%g", g_pGameCVars->v_altitudeLimitDefault());
			g_pGameCVars->pAltitudeLimitCVar->ForceSet(buf.c_str());
		}
    //the hud continues existing when the player got diconnected - it's part of the game
    /*if(!gEnv->IsEditor())
    {
    SAFE_DELETE(m_pHUD);
    }*/
  }
}

void CGame::BlockingProcess(BlockingConditionFunction f)
{
  INetwork* pNetwork = gEnv->pNetwork;

  bool ok = false;

  ITimer * pTimer = gEnv->pTimer;
  CTimeValue startTime = pTimer->GetAsyncTime();

  while (!ok)
  {
    pNetwork->SyncWithGame(eNGS_FrameStart);
    pNetwork->SyncWithGame(eNGS_FrameEnd);
    gEnv->pTimer->UpdateOnFrameStart();
    ok |= (*f)();
  }
}

CGameRules *CGame::GetGameRules() const
{
	return static_cast<CGameRules *>(m_pFramework->GetIGameRulesSystem()->GetCurrentGameRules());
}


CScreenEffects *CGame::GetScreenEffects() const
{
	return m_pScreenEffects;
}

CLaptopUtil *CGame::GetLaptopUtil() const
{
	return m_pLaptopUtil;
}

CFlashMenuObject *CGame::GetMenu() const // To be removed
{
	return m_pFlashMenuObject;
}

COptionsManager *CGame::GetOptions() const
{
	return m_pOptionsManager;
}

CProfileOptions *CGame::GetProfileOptions() const
{
	return m_pProfileOptions;
}

void CGame::LoadActionMaps(const char* filename)
{
	if (g_pGame->GetIGameFramework()->IsGameStarted())
	{
		CryLogAlways("[Profile] Can't change configuration while game is running (yet)");
		return;
	}

	XmlNodeRef rootNode = m_pFramework->GetISystem()->LoadXmlFile(filename);
	if (rootNode && ReadProfile(rootNode))
	{
		IActionMapManager *pActionMapMan = m_pFramework->GetIActionMapManager();
		m_pDefaultAM = pActionMapMan->GetActionMap("default");
	}
	else
	{
		CryLogAlways("[Profile] Warning: Could not open configuration file");
	}

	m_pGameActions->Init();
}

void CGame::RegisterActionMaps()
{
	IActionMapManager *pActionMapMan = m_pFramework->GetIActionMapManager();

	// Tweak menu is disabled at start
	pActionMapMan->EnableActionMap("default", true);

	// Disable Remote View map at start
	pActionMapMan->EnableActionMap("satellite_uplink", false);

	if (IActor *pActor = m_pFramework->GetClientActor())
	{
		// Set the listeners (may already be set)
		if (m_pDefaultAM)
		{
			m_pDefaultAM->SetActionListener(pActor->GetEntityId());
		}
	}
}

void CGame::ReleaseActionMaps()
{
	SAFE_RELEASE(m_pDefaultAM);
	g_pGameActions = NULL;
}

bool CGame::ReadProfile(XmlNodeRef &rootNode)
{
	bool bResult = false;

	IActionMapManager *pActionMapMan = m_pFramework->GetIActionMapManager();
	if (pActionMapMan)
	{
		pActionMapMan->Clear();

		// Load platform information in
		XmlNodeRef platforms = rootNode->findChild("platforms");
		if (!platforms || !ReadProfilePlatform(platforms, GetPlatform()))
		{
			CryLogAlways("[Profile] Warning: No platform information specified!");
		}

		// Load action maps
		int childCount = rootNode->getChildCount();
		XmlNodeRef actionMap = NULL;
		for (int i = 0; i < childCount; ++i)
		{
			actionMap = rootNode->getChild(i);
			if (actionMap && !strcmp(actionMap->getTag(), "actionmap"))
			{
				bResult |= ReadProfileActionMap(actionMap);
			}
		}
	}

	return bResult;
}

bool CGame::ReadProfilePlatform(XmlNodeRef &platformsNode, EPlatform platformId)
{
	bool bResult = false;

	//! Platform names
	static char const* sPlatformNames[ePlatform_COUNT] =
	{
		"Unknown",
		"PC",
		"Xbox",
		"PS3"
	};

	if (platformsNode && platformId > ePlatform_Unknown && platformId < ePlatform_COUNT)
	{
		XmlNodeRef platform = platformsNode->findChild(sPlatformNames[platformId]);
		if (platform)
		{
			if (!strcmp(platform->getAttr("keyboard"), "0"))
				m_platformInfo.devices &= ~SPlatformInfo::eDevice_Keyboard;
			if (!strcmp(platform->getAttr("xboxpad"), "0"))
				m_platformInfo.devices &= ~SPlatformInfo::eDevice_XboxPad;
			if (!strcmp(platform->getAttr("ps3pad"), "0"))
				m_platformInfo.devices &= ~SPlatformInfo::eDevice_PS3Pad;

			bResult = true;
		}
	}

	return bResult;
}

bool CGame::ReadProfileActionMap(XmlNodeRef &actionMapNode)
{
	bool bResult = false;

	IActionMapManager *pActionMapMan = m_pFramework->GetIActionMapManager();
	if (actionMapNode && pActionMapMan)
	{
		IActionMap *pMap = pActionMapMan->CreateActionMap(actionMapNode->getAttr("name"));
		if (pMap)
		{
			int childCount = actionMapNode->getChildCount();
			XmlNodeRef action = NULL;
			for (int i = 0; i < childCount; ++i)
			{
				action = actionMapNode->getChild(i);
				if (action && !strcmp(action->getTag(), "action"))
				{
					int	actionFlags = 0;
					const char* actionName = action->getAttr("name");
					if (!strcmp(action->getAttr("onPress"), "1"))
						actionFlags |= eAAM_OnPress;
					if (!strcmp(action->getAttr("onRelease"), "1"))
						actionFlags |= eAAM_OnRelease;
					if (!strcmp(action->getAttr("onHold"), "1"))
						actionFlags |= eAAM_OnHold;
					if (!strcmp(action->getAttr("always"), "1"))
						actionFlags |= eAAM_Always;
					if (!strcmp(action->getAttr("consoleCmd"), "1"))
						actionFlags |= eAAM_ConsoleCmd;
					if (!strcmp(action->getAttr("noModifiers"), "1"))
						actionFlags |= eAAM_NoModifiers;
					if (!strcmp(action->getAttr("retriggerable"), "1"))
						actionFlags |= eAAM_Retriggerable;

					pMap->CreateAction(actionName, actionFlags);

					// Get keys based on platform's settings
					int totalKeys = 0;
					if (m_platformInfo.devices & SPlatformInfo::eDevice_Keyboard)
					{
						pMap->BindAction(actionName, action->getAttr("keyboard"), totalKeys++);
					}
					if (m_platformInfo.devices & SPlatformInfo::eDevice_XboxPad)
					{
						pMap->BindAction(actionName, action->getAttr("xboxpad"), totalKeys++);
					}
					if (m_platformInfo.devices & SPlatformInfo::eDevice_PS3Pad)
					{
						pMap->BindAction(actionName, action->getAttr("ps3pad"), totalKeys++);
					}

					bResult = true;
				}
			}
		}
	}

	return bResult;
}

void CGame::InitScriptBinds()
{
	m_pScriptBindActor = new CScriptBind_Actor(m_pFramework->GetISystem());
	m_pScriptBindItem = new CScriptBind_Item(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindWeapon = new CScriptBind_Weapon(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindHUD = new CScriptBind_HUD(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindGameRules = new CScriptBind_GameRules(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindGame = new CScriptBind_Game(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindHitDeathReactions = new CScriptBind_HitDeathReactions(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindGameAI = new CScriptBind_GameAI(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindInteractiveObject = new CScriptBind_InteractiveObject(m_pFramework->GetISystem(), m_pFramework);
	m_pScriptBindBoids = new CScriptBind_Boids(m_pFramework->GetISystem());
}

void CGame::ReleaseScriptBinds()
{
	SAFE_DELETE(m_pScriptBindActor);
	SAFE_DELETE(m_pScriptBindItem);
	SAFE_DELETE(m_pScriptBindWeapon);
	SAFE_DELETE(m_pScriptBindHUD);
	SAFE_DELETE(m_pScriptBindGameRules);
	SAFE_DELETE(m_pScriptBindGame);
	SAFE_DELETE(m_pScriptBindInteractiveObject);
	SAFE_DELETE(m_pScriptBindHitDeathReactions);
	SAFE_DELETE(m_pScriptBindBoids);
}

void CGame::CheckReloadLevel()
{
	if(!m_bReload)
		return;

	m_bReload = false;

	if(gEnv->IsEditor() || gEnv->bMultiplayer)
	{
		return;
	}

	// (MATT) Crysis 2 doesn't yet support serialisation - so all platforms should reload the level from scratch {2009/12/11}
#if 0 // #ifdef WIN32  
	// Restart interrupts cutscenes
	gEnv->pMovieSystem->StopAllCutScenes();

	GetISystem()->SerializingFile(1);

	//load levelstart
	ILevelSystem* pLevelSystem = m_pFramework->GetILevelSystem();
	ILevel*			pLevel = pLevelSystem->GetCurrentLevel();
	ILevelInfo* pLevelInfo = pLevelSystem->GetLevelInfo(m_pFramework->GetLevelName());
	//**********
	EntityId playerID = GetIGameFramework()->GetClientActorId();
	pLevelSystem->OnLoadingStart(pLevelInfo);
	PlayerIdSet(playerID);
	string levelstart(GetIGameFramework()->GetLevelName());
	if(const char* visibleName = GetMappedLevelName(levelstart))
		levelstart = visibleName;
	//levelstart.append("_levelstart.crysisjmsf"); //because of the french law we can't do this ...
	levelstart.append("_crysis.crysisjmsf");
	GetIGameFramework()->LoadGame(levelstart.c_str(), true, true);
	//**********
	pLevelSystem->OnLoadingComplete(pLevel);

	//if paused - start game
	m_pFramework->PauseGame(false, true);

	GetISystem()->SerializingFile(0);
#else
	string command("map ");
	command.append(m_pFramework->GetLevelName());
	gEnv->pConsole->ExecuteString(command);
#endif
}

void CGame::RegisterGameObjectEvents()
{
	IGameObjectSystem* pGOS = m_pFramework->GetIGameObjectSystem();

	pGOS->RegisterEvent(eCGE_OnShoot,"OnShoot");
	pGOS->RegisterEvent(eCGE_BeginReloadLoop,"BeginReloadLoop");
	pGOS->RegisterEvent(eCGE_EndReloadLoop,"EndReloadLoop");
	pGOS->RegisterEvent(eCGE_ActorRevive,"ActorRevive");
	pGOS->RegisterEvent(eCGE_VehicleDestroyed,"VehicleDestroyed");
	pGOS->RegisterEvent(eCGE_TurnRagdoll,"TurnRagdoll");
	pGOS->RegisterEvent(eCGE_EnableFallAndPlay,"EnableFallAndPlay");
	pGOS->RegisterEvent(eCGE_DisableFallAndPlay,"DisableFallAndPlay");
	pGOS->RegisterEvent(eCGE_VehicleTransitionEnter,"VehicleTransitionEnter");
	pGOS->RegisterEvent(eCGE_VehicleTransitionExit,"VehicleTransitionExit");
	pGOS->RegisterEvent(eCGE_HUD_PDAMessage,"HUD_PDAMessage");
	pGOS->RegisterEvent(eCGE_HUD_TextMessage,"HUD_TextMessage");
	pGOS->RegisterEvent(eCGE_TextArea,"TextArea");
	pGOS->RegisterEvent(eCGE_HUD_Break,"HUD_Break");
	pGOS->RegisterEvent(eCGE_HUD_Reboot,"HUD_Reboot");
	pGOS->RegisterEvent(eCGE_InitiateAutoDestruction,"InitiateAutoDestruction");
	pGOS->RegisterEvent(eCGE_Event_Collapsing,"Event_Collapsing");
	pGOS->RegisterEvent(eCGE_Event_Collapsed,"Event_Collapsed");
	pGOS->RegisterEvent(eCGE_MultiplayerChatMessage,"MultiplayerChatMessage");
	pGOS->RegisterEvent(eCGE_ResetMovementController,"ResetMovementController");
	pGOS->RegisterEvent(eCGE_AnimateHands,"AnimateHands");
	pGOS->RegisterEvent(eCGE_Ragdoll,"Ragdoll");
	pGOS->RegisterEvent(eCGE_EnablePhysicalCollider,"EnablePhysicalCollider");
	pGOS->RegisterEvent(eCGE_DisablePhysicalCollider,"DisablePhysicalCollider");
	pGOS->RegisterEvent(eCGE_RebindAnimGraphInputs,"RebindAnimGraphInputs");
	pGOS->RegisterEvent(eCGE_SetTeam, "SetTeam");
	pGOS->RegisterEvent(eCGE_Launch, "Launch");
	// [*DavidR | 1/Sep/2009] CHECK: Can we put this on some HitDeathReaction 
	// initialization code?
	pGOS->RegisterEvent(eCGE_ReactionEnd, "ReactionEnd");
	pGOS->RegisterEvent(eCGE_CoverTransitionEnter,"CoverTransitionEnter");
	pGOS->RegisterEvent(eCGE_CoverTransitionExit,"CoverTransitionExit");
}

void CGame::GetMemoryStatistics(ICrySizer * s)
{
	{
		SIZER_COMPONENT_NAME(s,"WarningsManager");
		s->AddObject( m_pWarningsManager );
	}

	s->AddObject( m_pMOSystem );
	s->AddObject( m_pGameAudio );
	s->AddObject( m_pHud );
	
	m_pWeaponSystem->GetMemoryStatistics(s);
	m_pScreenEffects->GetMemoryStatistics(s);

	s->Add(*m_pScriptBindActor);
	s->Add(*m_pScriptBindItem);
	s->Add(*m_pScriptBindWeapon);
	s->Add(*m_pScriptBindGameRules);
	s->Add(*m_pScriptBindGame);
	s->Add(*m_pScriptBindHUD);
	s->Add(*m_pScriptBindInteractiveObject);
	s->Add(*m_pScriptBindHitDeathReactions);
	s->Add(*m_pScriptBindBoids);

	SAFE_MENU_FUNC(GetMemoryStatistics(s));

	s->Add(*m_pGameActions);


	m_pGameParametersStorage->GetMemoryStatistics(s);

	if (m_pPlayerProfileManager)
	  m_pPlayerProfileManager->GetMemoryStatistics(s);

	if (m_pServerSynchedStorage)
		m_pServerSynchedStorage->GetMemoryStatistics(s);

	if (m_pClientSynchedStorage)
		m_pClientSynchedStorage->GetMemoryStatistics(s);

	if (m_pServerGameTokenSynch)
		m_pServerGameTokenSynch->GetMemoryUsage(s);

	if (m_pClientGameTokenSynch)
		m_pClientGameTokenSynch->GetMemoryStatistics(s);
}

void CGame::OnClearPlayerIds()
{
	// do nothing
}

void CGame::DumpMemInfo(const char* format, ...)
{
	CryModuleMemoryInfo memInfo;
	CryGetMemoryInfoForModule(&memInfo);

	va_list args;
	va_start(args,format);
	gEnv->pLog->LogV( ILog::eAlways,format,args );
	va_end(args);

	gEnv->pLog->LogWithType( ILog::eAlways, "Alloc=%I64d kb  String=%I64d kb  STL-alloc=%I64d kb  STL-wasted=%I64d kb", (memInfo.allocated - memInfo.freed) >> 10 , memInfo.CryString_allocated >> 10, memInfo.STL_allocated >> 10 , memInfo.STL_wasted >> 10);
	// gEnv->pLog->LogV( ILog::eAlways, "%s alloc=%llu kb  instring=%llu kb  stl-alloc=%llu kb  stl-wasted=%llu kb", text, memInfo.allocated >> 10 , memInfo.CryString_allocated >> 10, memInfo.STL_allocated >> 10 , memInfo.STL_wasted >> 10);
}

const string& CGame::GetLastSaveGame(string &levelName)
{
	if (m_pPlayerProfileManager)
	{
		const char* userName = GetISystem()->GetUserName();
		IPlayerProfile* pProfile = m_pPlayerProfileManager->GetCurrentProfile(userName);
		if (pProfile)
		{
			ISaveGameEnumeratorPtr pSGE = pProfile->CreateSaveGameEnumerator();
			ISaveGameEnumerator::SGameDescription desc;	
			time_t curLatestTime = (time_t) 0;
			const char* lastSaveGame = "";
			const int nSaveGames = pSGE->GetCount();
			for (int i=0; i<nSaveGames; ++i)
			{
				if (pSGE->GetDescription(i, desc))
				{
					if (desc.metaData.saveTime > curLatestTime)
					{
						lastSaveGame = desc.name;
						curLatestTime = desc.metaData.saveTime;
						levelName = desc.metaData.levelName;
					}
				}
			}
			m_lastSaveGame = lastSaveGame;
		}
	}

	return m_lastSaveGame;
}

/*static */void CGame::ExpandTimeSeconds(int secs, int& days, int& hours, int& minutes, int& seconds)
{
	days  = secs / 86400;
	secs -= days * 86400;
	hours = secs / 3600;
	secs -= hours * 3600;
	minutes = secs / 60;
	seconds = secs - minutes * 60;
}

const char* CGame::CreateSaveGameName()
{
	//design wants to have different, more readable names for the savegames generated
	char buffer[16];
	int id = 0;

	//saves a running savegame id which is displayed with the savegame name
	if(IPlayerProfileManager *m_pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager())
	{
		const char *user = m_pPlayerProfileManager->GetCurrentUser();
		if(IPlayerProfile *pProfile = m_pPlayerProfileManager->GetCurrentProfile(user))
		{
			pProfile->GetAttribute("Singleplayer.SaveRunningID", id);
			pProfile->SetAttribute("Singleplayer.SaveRunningID", id+1);
			IPlayerProfileManager::EProfileOperationResult result;
			m_pPlayerProfileManager->SaveProfile(user, result);
		}
	}

	itoa(id, buffer, 10);
	m_newSaveGame.clear();
	if(id < 10)
		m_newSaveGame += "0";
	m_newSaveGame += buffer;
	m_newSaveGame += "_";

	const char* levelName = GetIGameFramework()->GetLevelName();
	const char* mappedName = GetMappedLevelName(levelName);
	m_newSaveGame += mappedName;

	m_newSaveGame += "_";
	string timeString;

	int d,h,m,s;
	CGame::ExpandTimeSeconds(m_pSPAnalyst->GetTimePlayed(), d, h, m, s);
	h += d*24;

	if (h > 0)
		timeString.Format("%02dh_%02dm_%02ds", h, m, s);
	else
		timeString.Format("%02dm_%02ds", m, s);

	m_newSaveGame += timeString;

	m_newSaveGame+=".CRYSISJMSF";

	return m_newSaveGame.c_str();
}

const char* CGame::GetMappedLevelName(const char *levelName) const
{ 
	TLevelMapMap::const_iterator iter = m_mapNames.find(CONST_TEMP_STRING(levelName));
	return (iter == m_mapNames.end()) ? levelName : iter->second.c_str();
}

IGameStateRecorder* CGame::CreateGameStateRecorder(IGameplayListener* pL)
{
	CGameStateRecorder* pGSP = new CGameStateRecorder();
	
	if(pGSP)
		pGSP->RegisterListener(pL);

	return (IGameStateRecorder*)pGSP;

}

Graphics::CColorGradientManager& CGame::GetColorGradientManager()
{
	return *m_colorGradientManager;
}

CInteractiveObjectRegistry& CGame::GetInteractiveObjectsRegistry() const
{
	return m_pScriptBindInteractiveObject->GetObjectDataRegistry();
}

void CGame::ClearSessionTelemetry(void)
{
	if (m_performanceBuffer)
	{
		m_performanceBuffer->Reset();
	}
	if (m_bandwidthBuffer)
	{
		m_bandwidthBuffer->Reset();
	}
	if (m_memoryTrackingBuffer)
	{
		m_memoryTrackingBuffer->Reset();
	}
}

void CGame::UploadSessionTelemetry(void)
{
	if (m_telemetryCollector)
	{
		CryFixedStringT<255> localFileName;

		if (m_performanceBuffer)
		{
			m_performanceBuffer->SubmitToServer("frametimes.log");
			localFileName.Format("./%s_frametimes.log", m_telemetryCollector->GetSessionId().c_str() );
			m_performanceBuffer->DumpToFile(localFileName.c_str());
			m_performanceBuffer->Reset();
		}
		if (m_bandwidthBuffer)
		{
			m_bandwidthBuffer->SubmitToServer("bandwidth.log");
			localFileName.Format("./%s_bandwidth.log", m_telemetryCollector->GetSessionId().c_str() );
			m_bandwidthBuffer->DumpToFile(localFileName.c_str());
			m_bandwidthBuffer->Reset();
		}
		if (m_memoryTrackingBuffer)
		{
			m_memoryTrackingBuffer->SubmitToServer("memory.log");
			localFileName.Format("./%s_memory.log", m_telemetryCollector->GetSessionId().c_str() );
			m_memoryTrackingBuffer->DumpToFile(localFileName.c_str());
			m_memoryTrackingBuffer->Reset();
		}
	}
}

void CGame::OnLevelEnd( const char* nextLevel )
{
	m_pBurnEffectManager->Reset();
}


#include UNIQUE_VIRTUAL_WRAPPER(IGame)
