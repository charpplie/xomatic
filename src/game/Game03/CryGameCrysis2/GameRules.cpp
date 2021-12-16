/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$

	-------------------------------------------------------------------------
	History:
		- 7:2:2006   15:38 : Created by Mrcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "ScriptBind_GameRules.h"
#include "GameRules.h"
#include "Game.h"
#include "GameCVars.h"
#include "Actor.h"
#include "Player.h"
#include "Utility/CryWatch.h"

#include "IPlayerInput.h"
#include "IVehicleSystem.h"
#include "IItemSystem.h"
#include "WeaponSystem.h"
#include "IUIDraw.h" // TODO : Remove in favor of 2DRenderUtils.

#include "HUD/HUD.h"
#include "HUD/UI/UIButtonPromptRegion.h"
#include "HUD/HUDMissionObjectiveSystem.h" // TODO : Needed?
#include "FrontEnd/FlashFrontEnd.h"

#include "ServerSynchedStorage.h"

#include "StatHelpers.h"
#include "StatsRecordingMgr.h"
#include "ITelemetryCollector.h"

#include "GameActions.h"
#include "Audio/GameAudio.h"
#include "Environment/BattleDust.h"
#include "Voting.h"
#include "SPAnalyst.h"
#include "IWorldQuery.h"
#include "IMovieSystem.h"
#include "TeamPerks.h"
#include "IGameStatistics.h"
#include "MindControlConsole.h"

#include "Effects/GameEffects/ExplosionGameEffect.h"
#include "Effects/GameEffectsSystem.h"

#include <StlUtils.h>
#include <StringUtils.h>

#include "GameRulesModules/GameRulesModulesManager.h"
#include "GameRulesModules/IGameRulesTeamsModule.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "GameRulesModules/IGameRulesPlayerSetupModule.h"
#include "GameRulesModules/IGameRulesScoringModule.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"
#include "GameRulesModules/IGameRulesActorActionModule.h"
#include "GameRulesModules/IGameRulesAssistScoringModule.h"
#include "GameRulesModules/IGameRulesDamageHandlingModule.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"
#include "GameRulesModules/IGameRulesSpectatorModule.h"
#include "GameRulesModules/IGameRulesVictoryConditionsModule.h"
#include "GameRulesModules/IGameRulesObjectivesModule.h"
#include "GameRulesModules/IGameRulesRoundsModule.h"
#include "GameRulesModules/IGameRulesStatsRecording.h"

#include "EquipmentLoadout.h"

#include "GameRulesModules/IGameRulesPickupListener.h"
#include "GameRulesModules/IGameRulesClientConnectionListener.h"
#include "GameRulesModules/IGameRulesTeamChangedListener.h"
#include "GameRulesModules/IGameRulesRevivedListener.h"
#include "GameRulesModules/IGameRulesSurvivorCountListener.h"
#include "GameRulesModules/IGameRulesKillListener.h"
#include "GameRulesModules/IGameRulesPlayerStatsListener.h"
#include "GameRulesModules/IGameRulesRoundsListener.h"
#include "GameRulesModules/IGameRulesClientScoreListener.h"

#include "PlayerVisTable.h"

#include "AI/GameAISystem.h"

#include "RecordingSystem.h"

#include "GameCodeCoverage/GameCodeCoverageManager.h"
#include "GodMode.h"

#include "Dogtag.h"
#include "PlayerProgression.h"
#include "PersistantStats.h"
#include "Battlechatter.h"
#include "IPerfHud.h"
#include "Audio/AreaAnnouncer.h"
#include "Projectile.h"
#include "PerkSonarVision.h"
#include "Network/Lobby/GameLobby.h"

#define TEMP_PERMANENTSONARVISION_GFX  0
#define TEMP_ACTIVETACTICAL_GFX  0

#if NUM_ASPECTS > 8
	#define GAMERULES_LIMITS_ASPECT				eEA_GameServerC
	#define GAMERULES_TEAMS_SCORE_ASPECT	eEA_GameServerA
#else
	#define GAMERULES_LIMITS_ASPECT				eEA_GameServerStatic
	#define GAMERULES_TEAMS_SCORE_ASPECT	eEA_GameServerStatic
#endif

#ifdef _DEBUG
bool CGameRules::s_dbgAssertOnFailureToFindHitType = true;
#endif

//------------------------------------------------------------------------
int CGameRules::s_invulnID = 0;
int CGameRules::s_barbWireID = 0;
AUTOENUM_BUILDNAMEARRAY(CGameRules::s_gameModeNames, AEGameModeList);

//------------------------------------------------------------------------
CGameRules::CGameRules()
: m_pGameFramework(0),
	m_pGameplayRecorder(0),
	m_pSystem(0),
	m_pActorSystem(0),
	m_pEntitySystem(0),
	m_pScriptSystem(0),
	m_pMaterialManager(0),
	m_onCollisionFunc(0),
	m_pClientNetChannel(0),
	m_teamIdGen(0),
	m_hitMaterialIdGen(0),
	m_hitTypeIdGen(0),
	m_currentStateId(0),
	m_lastHitFeedback(0.0f),
	m_gameStartedTime(0.0f),
	m_roundEndTime(0.0f),
	m_preRoundEndTime(0.0f),
	m_gameStartTime(0.0f),
	m_gameEndedTime(0.0f),
	m_pEquipmentLoadout(0),
	m_pBattleDust(0),
	m_pMPTutorial(0),
  m_pVotingSystem(0),
	m_pPerkInstance(0),
	m_pBattlechatter(0),
	m_pAreaAnnouncer(0),
	m_pExplosionGameEffect(0),
	m_ignoreEntityNextCollision(0),
	m_timeOfDayInitialized(false),
	m_processingHit(0),
	m_iFramesLeftCheckingMoviesSynq(0),
	m_coopPlayerHost(0),
	m_coopPlayerClient(0),
	m_pMigratingPlayerInfo(NULL),
	m_migratingPlayerMaxCount(0),
	m_pHostMigrationItemInfo(NULL),
	m_hostMigrationItemMaxCount(0),
	m_pHostMigrationParams(NULL),
	m_pHostMigrationClientParams(NULL),
	m_hostMigrationTimeStateChanged(0.f),
	m_hostMigrationState(eHMS_NotMigrating)
{
// Initialise module pointers
#define GAMERULES_MODULE_LIST_FUNC(type, name, lowerCase, useInEditor) m_##lowerCase##Module = 0;
	GAMERULES_MODULE_TYPES_LIST
#undef GAMERULES_MODULE_LIST_FUNC

	m_timeLimit = g_pGameCVars->g_timelimit;
	m_scoreLimit = g_pGameCVars->g_scoreLimit;
	m_roundLimit = g_pGameCVars->g_roundlimit;
	
	m_pendingActorsToBeKnockedDown.reserve( 10 ); // arbitrary number, unlikely to reach that

	for(int i = 0; i < MAX_CONCURRENT_EXPLOSIONS; i++)
	{
		m_explosionValidities[i]	= false;
	}
	
	new CPerkSonarVisionManager();

	if (gEnv->bMultiplayer)
	{
		m_migratingPlayerMaxCount = 16;

		if (gEnv->pConsole)
		{
			ICVar* pMaxPlayers = gEnv->pConsole->GetCVar("sv_maxplayers");
			if (pMaxPlayers)
			{
				m_migratingPlayerMaxCount = pMaxPlayers->GetIVal();
			}
		}
		m_pMigratingPlayerInfo = new SMigratingPlayerInfo[m_migratingPlayerMaxCount];
		CRY_ASSERT(m_pMigratingPlayerInfo != NULL);

		m_hostMigrationItemMaxCount = m_migratingPlayerMaxCount * 8;		// Allow for 8 items per person
		m_pHostMigrationItemInfo = new SHostMigrationItemInfo[m_hostMigrationItemMaxCount];
		CRY_ASSERT(m_pHostMigrationItemInfo != NULL);
	}

	ClearAllMigratingPlayers();
}

//------------------------------------------------------------------------
CGameRules::~CGameRules()
{
	SaveSessionStatistics();

	SAFE_DELETE_ARRAY(m_pMigratingPlayerInfo)
	SAFE_DELETE_ARRAY(m_pHostMigrationItemInfo);

	SAFE_DELETE(m_pHostMigrationParams);
	SAFE_DELETE(m_pHostMigrationClientParams);

	if( gEnv->bClient )
	{
		g_pGame->GetHUD()->Unload(); // Maybe not unload the HUD here. To make loads faster next game just unload obsolete Objects and load new ones.
	}

	if (m_onCollisionFunc)
	{
		gEnv->pScriptSystem->ReleaseFunc(m_onCollisionFunc);
		m_onCollisionFunc = 0;
	}

	g_pGame->GetWeaponSystem()->GetTracerManager().Reset();
	m_pGameFramework->GetIGameRulesSystem()->SetCurrentGameRules(0);
	if(m_pGameFramework->GetIViewSystem())
		m_pGameFramework->GetIViewSystem()->RemoveListener(this);

	delete m_pBattleDust;

  delete m_pVotingSystem;

	delete m_pPerkInstance;

	delete m_pBattlechatter;

	delete m_pAreaAnnouncer;

	GAME_FX_SYSTEM.DeleteEffect((IGameEffect**)&m_pExplosionGameEffect);

	SAFE_DELETE(m_pEquipmentLoadout);


// Delete any modules
#define GAMERULES_MODULE_LIST_FUNC(type, name, lowerCase, useInEditor) SAFE_DELETE(m_##lowerCase##Module);
	GAMERULES_MODULE_TYPES_LIST
#undef GAMERULES_MODULE_LIST_FUNC

	g_pGame->GetIGameFramework()->UnregisterListener(this);
	gEnv->pNetwork->RemoveEventListener(this);

	//--- Flushes the buffers mapping first to third person animations
	CAnimationProxyDualCharacterBase::ReleaseBuffers();

#if ENABLE_GAME_CODE_COVERAGE
	// Needs to be done after SetCurrentGameRules(0)
	CGameCodeCoverageManager::GetInstance()->Reset();
#endif

	TEntityTeamIdMap::iterator entityTeamsIt = m_entityteams.begin();
	for (; entityTeamsIt != m_entityteams.end(); ++ entityTeamsIt)
	{
		EntityId entityId = entityTeamsIt->first;
		gEnv->pEntitySystem->RemoveEntityEventListener(entityId, ENTITY_EVENT_DONE, this);
	}

	if (g_pGame->GetFlashMenu())
	{
		g_pGame->GetFlashMenu()->DestroyIngameMenu();
	}

	delete CPerkSonarVisionManager::GetMgr();

	if (m_hostMigrationState != eHMS_NotMigrating)
	{
		// Quitting game mid-migration (probably caused by a failed migration), re-enable timers so that the game isn't paused if we join a new one!
		gEnv->pTimer->PauseTimer(ITimer::ETIMER_GAME, false);
		gEnv->pEntitySystem->PauseTimers(false, true);
	}
}

//------------------------------------------------------------------------
static void CacheTextureHACK(char * texName)
{
	IRenderer*	piRenderer(gEnv->pRenderer);

	ITexture * pCachedTexture=piRenderer->EF_LoadTexture(texName,0, eTF_Unknown);
	if (pCachedTexture)
	{
		piRenderer->EF_PrecacheResource(pCachedTexture,0,0,0,-1);
	}
}

//------------------------------------------------------------------------
bool CGameRules::Init( IGameObject * pGameObject )
{
	CCCPOINT(GameRules_Init);

	g_pGame->GetPlayerVisTable()->Reset();

	SetGameObject(pGameObject);

	if (!GetGameObject()->BindToNetwork())
		return false;

	int  modei;
	const bool  modeOk = AutoEnum_GetEnumValFromString(GetEntity()->GetClass()->GetName(), S_GetGameModeNamesArray(), eGM_NUM_GAMEMODES, &modei);
	CRY_ASSERT(modeOk);
	m_gameMode = (EGameMode) modei;
	CRY_ASSERT((m_gameMode > eGM_INVALID_GAMEMODE) && (m_gameMode < eGM_NUM_GAMEMODES));

	GetGameObject()->EnablePostUpdates(this);

	m_pGameFramework = g_pGame->GetIGameFramework();
	m_pGameplayRecorder = m_pGameFramework->GetIGameplayRecorder();
	m_pSystem = m_pGameFramework->GetISystem();
	m_pActorSystem = m_pGameFramework->GetIActorSystem();
	m_pEntitySystem = gEnv->pEntitySystem;
	m_pScriptSystem = m_pSystem->GetIScriptSystem();
	m_pMaterialManager = gEnv->p3DEngine->GetMaterialManager();
	s_invulnID = m_pMaterialManager->GetSurfaceTypeManager()->GetSurfaceTypeByName("mat_invulnerable")->GetId();
	s_barbWireID = m_pMaterialManager->GetSurfaceTypeManager()->GetSurfaceTypeByName("mat_metal_barbwire")->GetId();
	
	// TODO: this is a hack to prevent the problem that is caused when those materials dont exist: in that case the ID returned is 0, which is also the ID that any object without an specified surface type will return.
	//       that makes any unknown material to be invulnerable to impact damage
	if (s_invulnID==0)
		s_invulnID=-1;
	if (s_barbWireID==0)
		s_barbWireID=-1;

	//Register as ViewSystem listener (for cut-scenes, ...)
	if(m_pGameFramework->GetIViewSystem())
		m_pGameFramework->GetIViewSystem()->AddListener(this);

	m_script = GetEntity()->GetScriptTable();
	m_script->GetValue("Client", m_clientScript);
	m_script->GetValue("Server", m_serverScript);
	m_script->GetValue("OnCollision", m_onCollisionFunc);

	m_collisionTable = gEnv->pScriptSystem->CreateTable();

	m_clientStateScript = m_clientScript;
	m_serverStateScript = m_serverScript;

	m_scriptHitInfo.Create(gEnv->pScriptSystem);
	m_scriptExplosionInfo.Create(gEnv->pScriptSystem);
  SmartScriptTable affected(gEnv->pScriptSystem);
  m_scriptExplosionInfo->SetValue("AffectedEntities", affected);
	SmartScriptTable affectedObstruction(gEnv->pScriptSystem);
	m_scriptExplosionInfo->SetValue("AffectedEntitiesObstruction", affectedObstruction);
  
	m_pGameFramework->GetIGameRulesSystem()->SetCurrentGameRules(this);
	g_pGame->GetGameRulesScriptBind()->AttachTo(this);

#if ENABLE_GAME_CODE_COVERAGE
	// Needs to be done after SetCurrentGameRules(this)
	CGameCodeCoverageManager::GetInstance()->Reset();
#endif

	// setup animation time scaling (until we have assets that cover the speeds we need timescaling).
	gEnv->pCharacterManager->SetScalingLimits( Vec2(0.5f, 3.0f) );

	const bool isMultiplayer=gEnv->bMultiplayer;

	if(gEnv->bClient)
	{
		IActionMapManager *pActionMapMan = g_pGame->GetIGameFramework()->GetIActionMapManager();
		IActionMap *am = NULL;
		pActionMapMan->EnableActionMap("multiplayer",isMultiplayer);
		pActionMapMan->EnableActionMap("singleplayer",!isMultiplayer);
		if(isMultiplayer)
		{
			am=pActionMapMan->GetActionMap("multiplayer");
		}
		else
		{
			am=pActionMapMan->GetActionMap("singleplayer");
		}

		if(am)
		{
			am->SetActionListener(GetEntity()->GetId());
		}
	}

	if (isMultiplayer)
	{
		m_pEquipmentLoadout = new CEquipmentLoadout();
		m_pEquipmentLoadout->Init();

		CRY_FIXME(26, 01, 2010, "HACK FOR MS3, NEEDS TO BE WRITTEN ALL NICELY AND SUCH BY CLAIRE");
		CacheTextureHACK("textures/weapons/ironzoom_blurmask.dds");
		CacheTextureHACK("textures/weapons/assaultscope_blur.dds");	

		CRY_FIXME(27, 01, 2010, "HACK FOR MS3, NEEDS TO BE WRITTEN ALL NICELY AND SUCH BY SOMEONE WHO KNOWS ABOUT HOLOGRAM GUN");
		CacheTextureHACK("objects/characters/human/us/nanosuit_v2/tex/nanosuit_modes_grads.dds");
		// these below are probably only needed if there isn't another player in the game too...
		CacheTextureHACK("objects/characters/human/us/nanosuit_v2/tex/helmet_dif.dds");
		CacheTextureHACK("objects/characters/human/us/nanosuit_v2/tex/helmet_ddn.dds");
		CacheTextureHACK("objects/characters/human/us/nanosuit_v2/tex/glass_dif.dds");
		CacheTextureHACK("objects/characters/human/us/nanosuit_v2/tex/glass_ddn.dds");
		CacheTextureHACK("objects/characters/human/us/nanosuit_v2/tex/nanosuit_mask.dds");
	}

	g_pGame->GetSPAnalyst()->Enable(!isMultiplayer);

	// create battledust object in SP, and on dx10 MP servers.
	if(!gEnv->bMultiplayer || (gEnv->bServer && g_pGame->GetIGameFramework()->IsImmersiveMPEnabled()))
		m_pBattleDust = new CBattleDust;

	SAFE_HUD_FUNC(GameRulesSet(GetEntity()->GetClass()->GetName()));

  if(isMultiplayer && gEnv->bServer)
    m_pVotingSystem = new CVotingSystem;

	if(isMultiplayer && gEnv->bClient)
	{
		m_pBattlechatter = new CBattlechatter;
		m_pAreaAnnouncer = new CAreaAnnouncer;
	}

	InitSessionStatistics();

	//Must be before score rewards init
	if(m_pPerkInstance == NULL)	//Only create an instance if needed
	{
		m_pPerkInstance = new CPerk();
	}
	CPerk::GetInstance()->Read();

	// Create explosion game effect and set active
	if(m_pExplosionGameEffect == NULL)
	{
		m_pExplosionGameEffect = GAME_FX_SYSTEM.CreateEffect<CExplosionGameEffect>();
		m_pExplosionGameEffect->Initialise();
	}
	if(m_pExplosionGameEffect)
	{
		m_pExplosionGameEffect->SetActive(true);
	}

	// Create modules
	const char *gameRulesName = GetEntity()->GetClass()->GetName();
	const char *xmlPath = CGameRulesModulesManager::GetInstance()->GetXmlPath(gameRulesName);
	CryLog ("Loading game rules class='%s' xml='%s'", gameRulesName, xmlPath);

	if (xmlPath)
	{
		XmlNodeRef root = gEnv->pSystem->LoadXmlFile( xmlPath );
		if (root)
		{
			CGameRulesModulesManager *pModulesManager = CGameRulesModulesManager::GetInstance();

			int numModules = root->getChildCount();
			for (int i = 0; i < numModules; ++ i)
			{
				XmlNodeRef childXml = root->getChild(i);

				const char *childTag = childXml->getTag();
				const char *className;

				if (0 == stricmp (childTag, "ScoreRewards"))
				{
					int  ivar;
					if (childXml->getAttr("enabled", ivar) && (ivar > 0))
					{
						m_scoreRewardTable.Init("Scripts/PerkInfo/ScoreRewards.xml");
					}
				}
				else if (childXml->getAttr("class", &className))
				{
					bool ok = false;

// For each module type, check if the current node is of that type and create the appropriate one
#define GAMERULES_MODULE_LIST_FUNC(type, name, lowerCase, useInEditor) \
	if (!stricmp(childTag, #name))	\
	{	\
		if (!gEnv->bEditor || useInEditor) \
		{ \
			CRY_ASSERT_MESSAGE(!m_##lowerCase##Module, "Module already exists");	\
			m_##lowerCase##Module = pModulesManager->Create##name##Module(className);	\
			if (m_##lowerCase##Module)	\
			{	\
				CryLog("CGameRules::Init() created %s module", className);	\
				m_##lowerCase##Module->Init(childXml);	\
				ok = true;	\
			}	\
			else	\
			{	\
				CryLogAlways("CGameRules::Init() ERROR: Failed to create %s module", className);	\
			}	\
		} \
		else \
		{ \
			CryLog("CGameRules::Init() module '%s' not created because we're in the editor", className);	\
			ok = true; \
		} \
	}

					GAMERULES_MODULE_TYPES_LIST

					if (!ok)
					{
						CryLogAlways("Failed to create module %s", className);
						CRY_ASSERT_MESSAGE(ok, "Failed to create gamerules module");
					}

#undef GAMERULES_MODULE_LIST_FUNC
				}
			}
		}
	}

	if (g_pGame->GetFlashMenu())
	{
		g_pGame->GetFlashMenu()->InitIngameMenu();
	}

	g_pGame->GetHUD()->Reload();
	SHUDEvent initGameRules;
	initGameRules.eventType = eHUDEvent_OnInitGameRules;
	CHUD::CallEvent(initGameRules);

	CNanoSuit::ReloadSuitData();

	g_pGame->GetIGameFramework()->RegisterListener(this, "gamerules", FRAMEWORKLISTENERPRIORITY_GAME);
	gEnv->pNetwork->AddEventListener(this, "CGameRules");

	AddGameRulesListener(CPlayerProgression::GetInstance());
	AddGameRulesListener(CPersistantStats::GetInstance());

	return true;
}

void CGameRules::InitSessionStatistics()
{
	CStatsRecordingMgr		*sr=g_pGame->GetStatsRecorder();

	if (sr)
	{
		sr->BeginSession();

		if (sr->IsTrackingEnabled())
		{
			if( IStatsTracker* tr = sr->GetSessionTracker() )
			{
				string gamemode = GetEntity()->GetClass()->GetName();
				gamemode.MakeLower();
				tr->StateValue(eSS_Gamemode, gamemode.c_str());

				string strMapName = "NO_MAP_ASSIGNED";
				string strMapPath;

				if ( ICVar *sv_map = gEnv->pConsole->GetCVar("sv_map") )
				{
					if ( const char* mapName = sv_map->GetString() )
					{	
						strMapName = mapName;
						strMapName.MakeLower();

						if ( ILevelInfo * pInfo = g_pGame->GetIGameFramework()->GetILevelSystem()->GetLevelInfo(mapName) )
							if ( const char* mapPath = pInfo->GetPath() )
								strMapPath = mapPath;
					}
				}

				// strip the path off the beginning of the map, the path is already output in the map path key, and having it in the map name
				// causes the path concatenation in the tool to get a doubled up directory components
				{
					int		pathOffset=strMapName.rfind('/');
					if (pathOffset!=-1)
					{
						strMapName=strMapName.Right(strMapName.length()-pathOffset-1);
					}
				}

				tr->StateValue(eSS_Map,			strMapName.c_str());
				tr->StateValue(eGSS_MapPath,	strMapPath.c_str());
			}

			// if teams are available then add it
			if(gEnv->bMultiplayer)
			{
				TTeamIdMap::iterator it = m_teams.begin();
				while (it != m_teams.end())
				{
					sr->AddTeam(it->second,it->first);
					++it;
				}
			}
			else
			{
				sr->AddTeam(0, "AI");
				sr->AddTeam(1, "Players");
			}
		}
	}

	g_pGame->ClearSessionTelemetry();
}

void CGameRules::SaveSessionStatistics()
{
#if ENABLE_GAME_CODE_COVERAGE
	CGameCodeCoverageManager::GetInstance()->UploadHitCheckpointsToServer();
#endif

	g_pGame->UploadSessionTelemetry();

	CStatsRecordingMgr		*sr=g_pGame->GetStatsRecorder();
	if (sr)
	{
		sr->EndSession();		// NB: EndSession changes the session id so dump all auxillary files for this session before calling EndSession() to ensure they're collated to the same place on the server
	}

	CHANGED_NETWORK_STATE(this, eEA_GameServerStatic);
}

//------------------------------------------------------------------------
void CGameRules::PostInit( IGameObject * pGameObject )
{
	CCCPOINT(GameRules_PostInit);

	pGameObject->EnableUpdateSlot(this, 0);
	pGameObject->SetUpdateSlotEnableCondition(this, 0, eUEC_WithoutAI);
	pGameObject->EnablePostUpdates(this);
	
	IConsole *pConsole=gEnv->pConsole;
	RegisterConsoleCommands(pConsole);
	RegisterConsoleVars(pConsole);

	if (m_teamsModule)
	{
		m_teamsModule->PostInit();
	}
	if (m_stateModule)
	{
		m_stateModule->PostInit();
	}
	if (m_playerSetupModule)
	{
		m_playerSetupModule->PostInit();
	}
	if (m_damageHandlingModule)
	{
		m_damageHandlingModule->PostInit();
	}
	if (m_spawningModule)
	{
		m_spawningModule->PostInit();
	}
	if (m_actorActionModule)
	{
		m_actorActionModule->PostInit();
	}
}

//------------------------------------------------------------------------
void CGameRules::InitClient(int channelId)
{
}

//------------------------------------------------------------------------
void CGameRules::PostInitClient(int channelId)
{
	// update the time
	GetGameObject()->InvokeRMI(ClPostInit(), PostInitParams(m_gameStartedTime, m_roundEndTime, m_preRoundEndTime, m_reviveCycleEndTime), eRMI_ToClientChannel, channelId);

	if (m_gameStartTime.GetMilliSeconds() > GetServerTime())
		GetGameObject()->InvokeRMI(ClSetGameStartTimer(), SetGameTimeParams(m_gameStartTime), eRMI_ToClientChannel, channelId);

	// update team status on the client
	for (TEntityTeamIdMap::const_iterator tit=m_entityteams.begin(); tit!=m_entityteams.end(); ++tit)
		GetGameObject()->InvokeRMIWithDependentObject(ClSetTeam(), SetTeamParams(tit->first, tit->second), eRMI_ToClientChannel, tit->first, channelId);

	if(m_pVotingSystem && m_pVotingSystem->IsInProgress())
	{
		int time_left = g_pGame->GetCVars()->sv_votingTimeout - int(m_pVotingSystem->GetVotingTime().GetSeconds());
		VotingStatusParams params(m_pVotingSystem->GetType(),time_left,m_pVotingSystem->GetEntityId(),m_pVotingSystem->GetSubject());
		if(m_pVotingSystem->GetEntityId()!=0)
			GetGameObject()->InvokeRMIWithDependentObject(ClVotingStatus(),params,eRMI_ToClientChannel,m_pVotingSystem->GetEntityId(),channelId);
		else
			GetGameObject()->InvokeRMI(ClVotingStatus(),params,eRMI_ToClientChannel,channelId);
	}
}

//------------------------------------------------------------------------
void CGameRules::Release()
{
	CCCPOINT(GameRules_Release);

	UnregisterConsoleCommands(gEnv->pConsole);
	UnregisterConsoleVars(gEnv->pConsole);
	delete this;
}

//------------------------------------------------------------------------
void CGameRules::FullSerialize( TSerialize ser )
{
	SAFE_HUD_FUNC(Serialize(ser));

//	SAFE_GAMEAUDIO_FUNC(Serialize(ser));

	if (g_pGame->GetSPAnalyst())
		g_pGame->GetSPAnalyst()->Serialize(ser);

	if (g_pGame->GetMOSystem())
		g_pGame->GetMOSystem()->Serialize(ser);

	bool battleDustActive = (m_pBattleDust)?true:false;
	ser.Value("battleDustActive", battleDustActive);
	if(battleDustActive)
	{
		if(m_pBattleDust) //reading, can be deactivated
			m_pBattleDust->Serialize(ser);
	}
}

//-----------------------------------------------------------------------------------------------------
void CGameRules::PostSerialize()
{
	SAFE_HUD_FUNC(PostSerialize());
}

//------------------------------------------------------------------------
void CGameRules::Update( SEntityUpdateContext& ctx, int updateSlot )
{
	CryWatch3DTick(ctx.fFrameTime);

	if (gEnv->bMultiplayer)
	{
		if (m_hostMigrationState == eHMS_WaitingForPlayers)
		{
			if (gEnv->bServer)
			{
				if (GetRemainingHostMigrationTimeoutTime() <= 0.f)
				{
					CryLog("CGameRules: HostMigration timeout reached");
					FinishHostMigrating();
				}
			}
			return;
		}
		else if (m_hostMigrationState == eHMS_Resuming)
		{
			const float curTime = gEnv->pTimer->GetAsyncCurTime();
			const float timePassed = curTime - m_hostMigrationTimeStateChanged;
			const float timeRemaining = g_pGameCVars->g_hostMigrationResumeTime - timePassed;
			if (timeRemaining > 0.f)
			{
				SHUDEvent resumingEvent(eHUDEvent_OnUpdateGameResumeMessage);
				int time = MAX(int(floor(timeRemaining + 0.5f)), 0);
				resumingEvent.AddData(time);
				CHUD::CallEvent(resumingEvent);
			}
			else
			{
				m_hostMigrationState = eHMS_NotMigrating;
				OnResumeAfterHostMigrating();
			}
			return;
		}
	}

	m_cachedServerTime = g_pGame->GetIGameFramework()->GetServerTime();

#if DEBUG_NEW_SPAWNING
	// needs to compile out for release etc
	if (g_pGameCVars->g_debug_entity_clustering)
	{
		VisuallyTestEntityClustering();
	}
#endif // DEBUG_NEW_SPAWNING

	if (updateSlot!=0)
		return;

	//g_pGame->GetServerSynchedStorage()->SetGlobalValue(15, 1026);

	bool server=gEnv->bServer;

	ProcessQueuedExplosions();

	if (server)
  {
		UpdateEntitySchedules(ctx.fFrameTime);
		KnockBackPendingActors();
  }

	if(m_pBattleDust)
		m_pBattleDust->Update();

	if(m_pBattlechatter)
		m_pBattlechatter->Update(ctx.fFrameTime);

	if(m_pAreaAnnouncer)
		m_pAreaAnnouncer->Update(ctx.fFrameTime);

	for (TTeamPerkMap::iterator iter=m_teamperks.begin(); iter!=m_teamperks.end(); ++iter)
	{
		iter->second.Update(ctx.fFrameTime);
	}

	if (gEnv->bServer)
	{
		if (m_timeLimit != g_pGameCVars->g_timelimit || m_scoreLimit != g_pGameCVars->g_scoreLimit || m_roundLimit != g_pGameCVars->g_roundlimit)
		{
			m_timeLimit = g_pGameCVars->g_timelimit;
			m_scoreLimit = g_pGameCVars->g_scoreLimit;
			m_roundLimit = g_pGameCVars->g_roundlimit;

			CHANGED_NETWORK_STATE(this, GAMERULES_LIMITS_ASPECT );
		}
	}

	CRY_TODO(06, 10, 2009, "[CG] Make these update calls into a list of listeners!");
	if (m_stateModule)
	{
		m_stateModule->Update(ctx.fFrameTime);
	}

	if (m_playerStatsModule)
	{
		m_playerStatsModule->Update(ctx.fFrameTime);
	}

	if (m_spawningModule)
	{
		m_spawningModule->Update(ctx.fFrameTime);
	}
	
	if (m_victoryConditionsModule)
	{
		m_victoryConditionsModule->Update(ctx.fFrameTime);
	}

	if (m_damageHandlingModule)
	{
		m_damageHandlingModule->Update(ctx.fFrameTime);
	}
	
	if (m_spectatorModule)
	{
		m_spectatorModule->Update(ctx.fFrameTime);
	}

	if (m_objectivesModule)
	{
		m_objectivesModule->Update(ctx.fFrameTime * g_pGameCVars->g_objectivesTimeScale);
	}

	if (m_teamsModule)
	{
		m_teamsModule->Update(ctx.fFrameTime);
	}

	if (m_roundsModule)
	{
		m_roundsModule->Update(ctx.fFrameTime);
	}

	CGodMode::GetInstance().Update(ctx.fFrameTime);

#if TEMP_PERMANENTSONARVISION_GFX || TEMP_ACTIVETACTICAL_GFX
	bool showTempEnemyHighlightEffect = false;
#if TEMP_PERMANENTSONARVISION_GFX
  // this is for Assault mode
	showTempEnemyHighlightEffect = (g_pGame->GetCVars()->g_mpPermanentSonarVision != 0);
	if (showTempEnemyHighlightEffect)
	{
		// Check nanosuit mode (only show effect in tactical mode)
		CPlayer *pPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetClientActor());
		if (pPlayer)
		{
			CNanoSuit *pNanosuit = pPlayer->GetNanoSuit();
			if (pNanosuit)
			{
				showTempEnemyHighlightEffect = (pNanosuit->GetGameParams().GetMode() == eNanoSuitMode_Tactical);
			}
		}
	}
#endif
#if TEMP_ACTIVETACTICAL_GFX
	if (!showTempEnemyHighlightEffect)
	{
		if (gEnv->bMultiplayer)
		{
			if (CPlayer* pPlayer=static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetClientActor()))
			{
				if (CNanoSuit* pNanosuit=pPlayer->GetNanoSuit())
				{
					showTempEnemyHighlightEffect = ((pNanosuit->GetGameParams().GetMode() == eNanoSuitMode_Tactical) && pNanosuit->GetGameParams().IsSuitPowerActive());
				}
			}
		}
	}
#endif
	if (showTempEnemyHighlightEffect)
	{
		// setup renderflag
		SAuxGeomRenderFlags  oldRenderFlags = gEnv->pRenderer->GetIRenderAuxGeom()->GetRenderFlags();
		SAuxGeomRenderFlags  renderFlags;
		renderFlags.SetDepthWriteFlag(e_DepthWriteOff);
		renderFlags.SetDepthTestFlag(e_DepthTestOff);
		renderFlags.SetAlphaBlendMode(e_AlphaBlended);  // transparent
		gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(renderFlags);
		// loop thru enemies drawing the fake sonarvision
		CActor*  pActor = static_cast< CActor* >( g_pGame->GetIGameFramework()->GetClientActor() );
		if (pActor)
		{
			const float  time = m_cachedServerTime.GetSeconds();
			const float  pulsedur = 2.0;
			const float  alphahi = 190.f;
			const float  alphalo = (float) g_pGame->GetCVars()->g_mpPermanentSonarVision_alpha;
			const int  alpha = (int) (alphahi - (cry_fmod(time, pulsedur) * (alphahi - alphalo)));
			const ColorB  colour = ColorB(255, 215, 84, alpha);
			const ColorB  olcolour = ColorB(255, 215, 84, g_pGame->GetCVars()->g_mpPermanentSonarVision_outline_alpha);
			const int  friendlyTeam = GetTeam(pActor->GetEntityId());
			const int  teamCount = GetTeamCount();
			for (int i=1; i<=teamCount; i++)
			{
				if (i == friendlyTeam)
					continue;
				TPlayers  teamPlayers;
				GetTeamPlayers(i, teamPlayers);
				TPlayers::const_iterator  it = teamPlayers.begin();
				TPlayers::const_iterator  end = teamPlayers.end();
				for (; it!=end; ++it)
				{
					CPlayer*  loopPlr = static_cast< CPlayer* >( m_pGameFramework->GetIActorSystem()->GetActor(*it) );
					if (!loopPlr)
						continue;
					IEntity*  loopEnt = loopPlr->GetEntity();
					assert(loopEnt);
					if (!g_pGame->GetPlayerVisTable()->CanLocalPlayerSee(loopEnt->GetId(), 5))
					{
						AABB  aabb;
						loopEnt->GetWorldBounds(aabb);
						Vec3  s = aabb.GetSize();
						Vec3  ex ((s.x * -0.15f), (s.y * -0.15f), (s.z * -0.05f));
						aabb.Expand(ex);
						gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(aabb, true, colour, eBBD_Faceted);
						gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(aabb, false, olcolour, eBBD_Faceted);
					}
				}
			}
		}
		// reset the renderflags etc.
		gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(oldRenderFlags);
	}
#endif
}

//------------------------------------------------------------------------
void CGameRules::KnockBackPendingActors()
{
	std::vector<EntityId>::const_iterator itEnd = m_pendingActorsToBeKnockedDown.end();
	for (std::vector<EntityId>::const_iterator it = m_pendingActorsToBeKnockedDown.begin(); it != itEnd; ++it)
		KnockActorDown(*it);

	m_pendingActorsToBeKnockedDown.clear();
}	
	

//------------------------------------------------------------------------
void CGameRules::HandleEvent( const SGameObjectEvent& event)
{
	SAFE_HUD_FUNC(HandleEvent(event));
}

//------------------------------------------------------------------------
void CGameRules::ProcessEvent( SEntityEvent& event)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);

	static ICVar* pTOD = gEnv->pConsole->GetCVar("sv_timeofdayenable");

	switch(event.event)
	{
	case ENTITY_EVENT_PRE_SERIALIZE:
		//reset some game systems that might cause problems during loading
		g_pGame->GetWeaponSystem()->GetTracerManager().Reset();
		g_pGame->GetIGameFramework()->GetIItemSystem()->Reset();
		break;

	case ENTITY_EVENT_RESET:
	{
		// done here rather than CGameRules::Restart so that it is called on clients as well as servers
		SaveSessionStatistics();
		InitSessionStatistics();

		m_timeOfDayInitialized = false;

		for(int i = 0; i < MAX_CONCURRENT_EXPLOSIONS; i++)
		{
			m_explosions[i].raycastHelper.CancelPendingRays();
			m_explosionValidities[i]	= false;
		}
    
    while (!m_queuedExplosions.empty())
      m_queuedExplosions.pop();

		while (!m_queuedExplosionsAwaitingLinetests.empty())
			m_queuedExplosionsAwaitingLinetests.pop();

		while (!m_queuedHits.empty())
			m_queuedHits.pop();

		m_processingHit=0;
		
      // TODO: move this from here
		g_pGame->GetWeaponSystem()->GetTracerManager().Reset();
		m_respawns.clear();
		m_removals.clear();
		m_gameEndedTime = 0.0f;

		if (m_stateModule)
		{
			m_stateModule->OnGameReset();
		}

		int resetScore = 0;
		if (m_scoringModule)
		{
			resetScore = m_scoringModule->GetStartTeamScore();
		}

		for (TTeamScoresMap::iterator iter=m_teamscores.begin(); iter!=m_teamscores.end(); ++iter)
		{
			iter->second.m_teamScore = resetScore;
			iter->second.m_roundTeamScore = 0;
		}

		if (m_playerStatsModule)
			m_playerStatsModule->ClearAllPlayerStats();

		if (GetObjectivesModule())
		{
			GetObjectivesModule()->OnGameReset();
		}

		CGameAISystem::GetInstance()->Reset();

		if (gEnv->pSystem->IsEditor() && event.nParam[0])
		{
			EntityId playerId = g_pGame->GetIGameFramework()->GetClientActorId();
			if (GetPlayerSetupModule())
			{
				GetPlayerSetupModule()->OnPlayerRevived(playerId);
			}
			else
			{
				CallScript(m_script, "EquipPlayer", playerId, false);
			}
		}

		break;
	}
	case ENTITY_EVENT_START_GAME:
		m_timeOfDayInitialized = false;
		g_pGame->GetWeaponSystem()->GetTracerManager().Reset();
		g_pGame->GetGameAudio()->Reset();

		if (gEnv->bServer && gEnv->bMultiplayer && pTOD && pTOD->GetIVal() && g_pGame->GetIGameFramework()->IsImmersiveMPEnabled())
		{
			static ICVar* pStart = gEnv->pConsole->GetCVar("sv_timeofdaystart");
			if (pStart)
				gEnv->p3DEngine->GetTimeOfDay()->SetTime(pStart->GetFVal(), true);
		}

		if(GetVictoryConditionsModule())
		{
			GetVictoryConditionsModule()->OnStartGame();
		}

		if (GetObjectivesModule())
		{
			GetObjectivesModule()->OnStartGame();
		}

		if (GetRoundsModule())
		{
			GetRoundsModule()->OnStartGame();
		}

		if (g_pGame->GetRecordingSystem())
		{
			g_pGame->GetRecordingSystem()->Reset();
		}

#if ENABLE_MINDCONTROL
		CMindControlConsole::OnStartGame();
#endif

		break;

	case ENTITY_EVENT_ENTER_SCRIPT_STATE:
		m_currentStateId=(int)event.nParam[0];

		m_clientStateScript=0;
		m_serverStateScript=0;

		IEntityScriptProxy *pScriptProxy=static_cast<IEntityScriptProxy *>(GetEntity()->GetProxy(ENTITY_PROXY_SCRIPT));
		if (pScriptProxy)
		{
			const char *stateName=pScriptProxy->GetState();

			m_clientScript->GetValue(stateName, m_clientStateScript);
			m_serverScript->GetValue(stateName, m_serverStateScript);
		}
		break;
	}

}

//------------------------------------------------------------------------
void CGameRules::SetAuthority( bool auth )
{
}

//------------------------------------------------------------------------
void CGameRules::PostUpdate( float frameTime )
{
  if(m_pVotingSystem && m_pVotingSystem->IsInProgress())
  {
    int need_votes = int(ceilf(GetPlayerCount(false)*g_pGame->GetCVars()->sv_votingRatio));
    if(need_votes && m_pVotingSystem->GetNumVotes() >= need_votes)
    {
      EndVoting(true);
    }
    if(int team = m_pVotingSystem->GetTeam())
    {
      int team_votes = int(ceilf(GetTeamPlayerCount(team,false)*g_pGame->GetCVars()->sv_votingRatio));
      if(team_votes && m_pVotingSystem->GetNumTeamVotes() >= team_votes)
      {
        EndVoting(true);
      }
    }
    if(m_pVotingSystem->GetVotingTime().GetSeconds() > g_pGame->GetCVars()->sv_votingTimeout)    
    {
      EndVoting(false);
    }
  }
}

//------------------------------------------------------------------------
IActor *CGameRules::GetActorByChannelId(int channelId) const
{
	return static_cast<IActor *>(m_pGameFramework->GetIActorSystem()->GetActorByChannelId(channelId));
}

//------------------------------------------------------------------------
IActor *CGameRules::GetActorByEntityId(EntityId entityId) const
{
	return static_cast<IActor *>(m_pGameFramework->GetIActorSystem()->GetActor(entityId));
}

//------------------------------------------------------------------------
const char* CGameRules::GetActorNameByEntityId(EntityId entityId) const
{
	IActor *pActor = GetActorByEntityId(entityId);
	if (pActor)
		return pActor->GetEntity()->GetName();
	return 0;
}

//------------------------------------------------------------------------
ILINE const char* CGameRules::GetActorName(IActor *pActor) const
{
	return pActor->GetEntity()->GetName();
};

//------------------------------------------------------------------------
int CGameRules::GetChannelId(EntityId entityId) const
{
	IActor *pActor = static_cast<IActor *>(m_pGameFramework->GetIActorSystem()->GetActor(entityId));
	if (pActor)
		return pActor->GetChannelId();

	return 0;
}

//------------------------------------------------------------------------
bool CGameRules::IsDead(EntityId id) const
{
	if (IActor *pActor=GetActorByEntityId(id))
			return (pActor->GetHealth() <= 0);

	return false;
}

//------------------------------------------------------------------------
bool CGameRules::IsSpectator(EntityId id) const
{
	if (IActor *pActor = GetActorByEntityId(id))
		return (pActor->GetSpectatorMode() != 0);

	return false;
}


//------------------------------------------------------------------------
bool CGameRules::ShouldKeepClient(int channelId, EDisconnectionCause cause, const char *desc) const
{
	return (!strcmp("timeout", desc) || cause==eDC_Timeout);
}

//------------------------------------------------------------------------
void CGameRules::PrecacheLevel()
{
	CallScript(m_script, "PrecacheLevel");
}

//------------------------------------------------------------------------
void CGameRules::OnConnect(struct INetChannel *pNetChannel)
{
	m_pClientNetChannel=pNetChannel;

	CallScript(m_clientStateScript,"OnConnect");
}


//------------------------------------------------------------------------
void CGameRules::OnDisconnect(EDisconnectionCause cause, const char *desc)
{
	CRecordingSystem *crs = g_pGame->GetRecordingSystem();
	if (crs)
	{
		if (crs->IsPlayingBack() || crs->IsPlaybackQueued())
			crs->StopPlayback();
		else if (crs->IsRecording())
			crs->StopRecording();
	}

	m_pClientNetChannel=0;
	int icause=(int)cause;
	CallScript(m_clientStateScript, "OnDisconnect", icause, desc);

	// BecomeRemotePlayer() will put the player camera into 3rd person view, but
	// the player rig will still be first person (headless, not z sorted) so
	// don't do it during host migration events
	if (!gEnv->bHostMigrating)
	{
		CActor *pLocalActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetClientActor());
		if (pLocalActor)
		{
			pLocalActor->BecomeRemotePlayer();
		}
	}
}

//------------------------------------------------------------------------
bool CGameRules::OnClientConnect(int channelId, bool isReset)
{
	CCCPOINT_IF (isReset, GameRules_ClientConnect_Reset);
	CCCPOINT_IF (! isReset, GameRules_ClientConnect_NotReset);

	if (!isReset)
	{
		m_channelIds.push_back(channelId);
		g_pGame->GetServerSynchedStorage()->OnClientConnect(channelId);
		g_pGame->GetServerGameTokenSynch()->OnClientConnect(channelId);
	}

	bool hasSetupModule = (GetPlayerSetupModule() != NULL);

	bool useExistingActor = false;
	IActor *pActor = NULL;

	// Check if there's a migrating player for this channel
	int migratingIndex = GetMigratingPlayerIndex(channelId);
	if (migratingIndex >= 0)
	{
		useExistingActor = true;
		pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_pMigratingPlayerInfo[migratingIndex].m_originalEntityId);
		pActor->SetMigrating(true);
		CryLog("CGameRules::OnClientConnect() migrating player, channelId=%i, name=%s", channelId, pActor->GetEntity()->GetName());
	}

	if (!useExistingActor)
	{
		string playerName;
		if (gEnv->bServer && gEnv->bMultiplayer)
		{
			if (INetChannel *pNetChannel=m_pGameFramework->GetNetChannel(channelId))
			{
				playerName=pNetChannel->GetNickname();
				if (!playerName.empty())
					playerName=VerifyName(playerName);
			}

			if (!hasSetupModule)
			{
				if(!playerName.empty())
					CallScript(m_serverStateScript, "OnClientConnect", channelId, isReset, playerName.c_str());
			    else
					CallScript(m_serverStateScript, "OnClientConnect", channelId, isReset);
			}
		}
		else if (!hasSetupModule)
		{
			CallScript(m_serverStateScript, "OnClientConnect", channelId);
		}
		if (hasSetupModule)
		{
			GetPlayerSetupModule()->OnClientConnect(channelId, isReset, playerName.c_str());
		}

		pActor=GetActorByChannelId(channelId);
	}

	if (pActor)
	{
		// Hide spawned actors until the client *enters* the game
		pActor->GetEntity()->Hide(true);

		//we need to pass team somehow so it will be reported correctly
		int status[2];
		status[0] = GetTeam(pActor->GetEntityId());
		status[1] = pActor->GetSpectatorMode();

		m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Connected, 0, m_pGameFramework->IsChannelOnHold(channelId)?1.0f:0.0f, (void*)status));

		if (useExistingActor)
		{
			// This is a bit of hackery to make sure the aspects get set correctly (BindToNetwork will set all aspect profiles to their default but doesn't tell the GameObject that it's done it!)
			uint8 aspects[NUM_ASPECTS];
			const uint8 notSet = ~uint8(0);

			IGameObject *pActorGameObject = pActor->GetGameObject();
			for (int i = 0; i < NUM_ASPECTS - 1; ++ i)
			{
				aspects[i + 1] = pActorGameObject->GetAspectProfile(EEntityAspects(BIT(i)));
			}

			pActorGameObject->SetChannelId(channelId);
			pActorGameObject->BindToNetwork(eBTNM_Force);

			for (int i = 0; i < NUM_ASPECTS - 1; ++ i)
			{
				if (aspects[i + 1] != notSet)
				{
					pActorGameObject->SetAspectProfile(EEntityAspects(BIT(i)), aspects[i + 1]);
				}
			}

			EntityId playerId = pActor->GetEntityId();
			int teamId = GetTeam(playerId);
			if (teamId)
			{
				GetGameObject()->InvokeRMIWithDependentObject(ClSetTeam(), SetTeamParams(playerId, teamId), eRMI_ToRemoteClients, playerId);
				// Add to channel teams map
				m_channelteams.insert(TChannelTeamIdMap::value_type(channelId, teamId));
			}

			CPlayer *pPlayer = static_cast<CPlayer *>(pActor);
			if (pPlayer)
			{
				pPlayer->SetSpectatorState(CActor::eASS_Ingame);	// Anyone who is going to manage the migration must have reached the InGame state
			}

			SHUDEvent playerInited;
			playerInited.eventType = eHUDEvent_HostMigrationOnNewPlayer;
			playerInited.m_data.push_back(SHUDEventData(int(playerId)));
			CHUD::CallEvent(playerInited);
		}
		else if (isReset)
		{
			SetTeam(GetChannelTeam(channelId), pActor->GetEntityId());

			// On reset try to spawn straight away.
			if (m_spawningModule)
			{
				m_spawningModule->SvRequestRevive(pActor->GetEntityId());
			}
		}

		//notify client he has entered the game
		GetGameObject()->InvokeRMIWithDependentObject(ClEnteredGame(), NoParams(), eRMI_ToClientChannel, pActor->GetEntityId(), channelId);
		
		int numListeners = m_clientConnectionListeners.size();
		for (int i = 0; i < numListeners; ++ i)
		{
			m_clientConnectionListeners[i]->OnClientConnect(channelId, isReset, pActor->GetEntityId());
		}
	}

	if (migratingIndex != -1)
	{
		// Remove the migrating player info (so we don't check again on a game restart!)
		m_pMigratingPlayerInfo[migratingIndex].Reset();

		// Check to see if this was the last player
		bool haveRemainingPlayers = false;
		for (int i = 0; i < m_migratingPlayerMaxCount; ++ i)
		{
			if (m_pMigratingPlayerInfo[i].InUse())
			{
				haveRemainingPlayers = true;
				break;
			}
		}
		if (!haveRemainingPlayers)
		{
			CryLog("CGameRules::OnClientConnect, all players have now migrated, resuming game");
			FinishHostMigrating();
		}
	}
	else if (m_hostMigrationState != eHMS_NotMigrating)
	{
		// This is a new client joining while we're migrating, need to tell them to pause game etc
		const float timeSinceStateChange = gEnv->pTimer->GetAsyncCurTime() - m_hostMigrationTimeStateChanged;
		SMidMigrationJoinParams params(int(m_hostMigrationState), timeSinceStateChange);
		GetGameObject()->InvokeRMI(ClMidMigrationJoin(), params, eRMI_ToClientChannel, channelId);
	}

	return pActor != 0;
}

//------------------------------------------------------------------------
void CGameRules::OnClientDisconnect(int channelId, EDisconnectionCause cause, const char *desc, bool keepClient)
{
	IActor *pActor=GetActorByChannelId(channelId);
	//assert(pActor);

	if (!pActor || !keepClient)
	{
		if (g_pGame->GetServerSynchedStorage())
			g_pGame->GetServerSynchedStorage()->OnClientDisconnect(channelId, false);
		if (g_pGame->GetServerGameTokenSynch())
			g_pGame->GetServerGameTokenSynch()->OnClientDisconnect(channelId, false);
	}

	if (!pActor)
		return;

	ClientDisconnect_NotifyListeners( pActor->GetEntityId() );
		
	if (pActor->GetEntityId()==m_coopPlayerHost)
		m_coopPlayerHost = 0;  // cant really happen, but oh well
	if (pActor->GetEntityId()==m_coopPlayerClient)
		m_coopPlayerClient = 0;

	m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Disconnected,"",keepClient?1.0f:0.0f));

	if (keepClient)
	{
		if (g_pGame->GetServerSynchedStorage())
			g_pGame->GetServerSynchedStorage()->OnClientDisconnect(channelId, true);
		if (g_pGame->GetServerGameTokenSynch())
			g_pGame->GetServerGameTokenSynch()->OnClientDisconnect(channelId, true);

		pActor->GetGameObject()->SetAspectProfile(eEA_Physics, eAP_NotPhysicalized);

		return;
	}

	if (IVehicle *pVehicle = pActor->GetLinkedVehicle())
	{
		if (IVehicleSeat *pSeat = pVehicle->GetSeatForPassenger(pActor->GetEntityId()))
			pSeat->Reset();
	}

  SetTeam(0, pActor->GetEntityId());

	stl::find_and_erase(m_channelIds, channelId);

	CallScript(m_serverStateScript, "OnClientDisconnect", channelId);

	int numListeners = m_clientConnectionListeners.size();
	for (int i = 0; i < numListeners; ++ i)
	{
		m_clientConnectionListeners[i]->OnClientDisconnect(channelId, pActor->GetEntityId());
	}

	return;
}

//------------------------------------------------------------------------
bool CGameRules::OnClientEnteredGame(int channelId, bool isReset)
{ 
	CCCPOINT_IF(isReset, GameRules_ClientEnteredGame_Reset);
	CCCPOINT_IF(!isReset, GameRules_ClientEnteredGame_NotReset);

	UpdateCoopPlayersIds();
	IActor *pActor=GetActorByChannelId(channelId);
	if (!pActor)
		return false;
		
	// Ensure the actor is visible when entering the game (but not in the editor)
	bool isEditing = gEnv->pSystem->IsEditorMode();

	if (!isEditing)
	{
		pActor->GetEntity()->Hide(false);
	}

	ClientEnteredGame_NotifyListeners( pActor->GetEntityId() );

	if (g_pGame->GetServerSynchedStorage())
		g_pGame->GetServerSynchedStorage()->OnClientEnteredGame(channelId);
	if (g_pGame->GetServerGameTokenSynch())
		g_pGame->GetServerGameTokenSynch()->OnClientEnteredGame(channelId);

	IScriptTable *pPlayer=pActor->GetEntity()->GetScriptTable();
	int loadingSaveGame=m_pGameFramework->IsLoadingSaveGame()?1:0;
	CallScript(m_serverStateScript, "OnClientEnteredGame", channelId, pPlayer, isReset, loadingSaveGame);

	// don't do this on reset - have already been added to correct team!
#ifndef OLD_VOICE_SYSTEM_DEPRECATED
	if(!isReset || GetTeamCount() < 2)
		ReconfigureVoiceGroups(pActor->GetEntityId(), -999, 0); /* -999 should never exist :) */
#endif
		
	int numListeners = m_clientConnectionListeners.size();
	for (int i = 0; i < numListeners; ++ i)
	{
		m_clientConnectionListeners[i]->OnClientEnteredGame(channelId, isReset, pActor->GetEntityId());
	}

	// in MP this is handled by GameRulesStandardState; in SP we need to trigger stats recording manually.
	if(!gEnv->bMultiplayer)
	{
		IGameRulesStatsRecording* pST = GetStatsRecordingModule();
		if (pST)
		{
			pST->OnInGameBegin();
		}
	}

	return true;
}

//------------------------------------------------------------------------
void CGameRules::OnEntitySpawn(IEntity *pEntity)
{
}

//------------------------------------------------------------------------
void CGameRules::OnEntityRespawn(IEntity *pEntity)
{
	CallScript(m_serverScript, "OnEntityRespawn", ScriptHandle(pEntity->GetId()));
}

//------------------------------------------------------------------------
void CGameRules::OnEntityRemoved(IEntity *pEntity)
{
	if (gEnv->bClient)
		SetTeam(0, pEntity->GetId());
}

//------------------------------------------------------------------------
void CGameRules::OnItemDropped(EntityId itemId, EntityId actorId)
{
	if (gEnv->bServer)
	{
		ScriptHandle itemIdHandle(itemId);
		ScriptHandle actorIdHandle(actorId);
		CallScript(m_serverStateScript, "OnItemDropped", itemIdHandle, actorIdHandle);
	}

	int numListeners = m_pickupListeners.size();
	for (int i = 0; i < numListeners; ++ i)
	{
		m_pickupListeners[i]->OnItemDropped(itemId, actorId);
	}
}

//------------------------------------------------------------------------
void CGameRules::OnItemPickedUp(EntityId itemId, EntityId actorId)
{
	if (gEnv->bServer)
	{
		ScriptHandle itemIdHandle(itemId);
		ScriptHandle actorIdHandle(actorId);
		CallScript(m_serverStateScript, "OnItemPickedUp", itemIdHandle, actorIdHandle);
	}

	int numListeners = m_pickupListeners.size();
	for (int i = 0; i < numListeners; ++ i)
	{
		m_pickupListeners[i]->OnItemPickedUp(itemId, actorId);
	}

	if (actorId == g_pGame->GetIGameFramework()->GetClientActorId() && m_pHostMigrationClientParams)
	{
		if (!m_pHostMigrationClientParams->m_doneSetAmmo)
		{
			-- m_pHostMigrationClientParams->m_numExpectedItems;
			if (!m_pHostMigrationClientParams->m_numExpectedItems)
			{
				CryLog("CGameRules::OnItemPickedUp, now received all expected items from host migration, setting ammo");
				IActor *pActor = g_pGame->GetIGameFramework()->GetClientActor();
				CRY_ASSERT(pActor);
				if (pActor)
				{
					IInventory *pInventory = pActor->GetInventory();
					for (int i = 0; i < m_pHostMigrationClientParams->m_numAmmoParams; ++ i)
					{
						CryLog("    %s : %i", m_pHostMigrationClientParams->m_pAmmoParams[i].m_pAmmoClass->GetName(), m_pHostMigrationClientParams->m_pAmmoParams[i].m_count);
						// Set ammo locally so the HUD reports it correctly, we still have to tell the server though
						pInventory->SetAmmoCount(m_pHostMigrationClientParams->m_pAmmoParams[i].m_pAmmoClass, m_pHostMigrationClientParams->m_pAmmoParams[i].m_count);
						pInventory->RMIReqToServer_SetAmmoCount(m_pHostMigrationClientParams->m_pAmmoParams[i].m_pAmmoClass->GetName(), m_pHostMigrationClientParams->m_pAmmoParams[i].m_count);
					}

					EntityId itemId = pInventory->GetCurrentItem();
					IItem *pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(itemId);
					if (pItem && pItem->GetIWeapon())
					{
						SHUDEvent event;
						event.eventType = eHUDEvent_OnSetAmmoCount;
						event.eventPtrData = pItem->GetIWeapon();

						CHUD::CallEvent(event);
					}
				}

				m_pHostMigrationClientParams->m_doneSetAmmo = true;
				if (m_pHostMigrationClientParams->IsDone())
				{
					SAFE_DELETE(m_pHostMigrationClientParams);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::OnPickupEntityDetached(EntityId entityId, EntityId actorId, bool isOnRemove)
{
	if (IEntity* pEntity=gEnv->pEntitySystem->GetEntity(entityId))
	{
		const char*  pFuncName = "AttachTo";
		IScriptTable*  pScript = pEntity->GetScriptTable();
		if (pScript && (pScript->GetValueType(pFuncName) == svtFunction))
		{
			IScriptSystem*  pScriptSystem = gEnv->pScriptSystem;
			pScriptSystem->BeginCall(pScript, pFuncName);
			pScriptSystem->PushFuncParam(pScript);
			pScriptSystem->EndCall();
		}
	}

	int  numListeners = m_pickupListeners.size();
	for (int i=0; i<numListeners; i++)
	{
		m_pickupListeners[i]->OnPickupEntityDetached(entityId, actorId, isOnRemove);
	}
}

//------------------------------------------------------------------------
void CGameRules::OnPickupEntityAttached(EntityId entityId, EntityId actorId)
{
	if (IEntity* pEntity=gEnv->pEntitySystem->GetEntity(entityId))
	{
		const char*  pFuncName = "AttachTo";
		IScriptTable*  pScript = pEntity->GetScriptTable();
		if (pScript && (pScript->GetValueType(pFuncName) == svtFunction))
		{
			IScriptSystem*  pScriptSystem = gEnv->pScriptSystem;
			pScriptSystem->BeginCall(pScript, pFuncName);
			pScriptSystem->PushFuncParam(pScript);
			if (IEntity* pActorEnt=gEnv->pEntitySystem->GetEntity(actorId))
			{
				if (pActorEnt->GetScriptTable())
				{
					pScriptSystem->PushFuncParam(pActorEnt->GetScriptTable());
				}
			}
			pScriptSystem->EndCall();
		}
	}

	int  numListeners = m_pickupListeners.size();
	for (int i=0; i<numListeners; i++)
	{
		m_pickupListeners[i]->OnPickupEntityAttached(entityId, actorId);
	}
}

//------------------------------------------------------------------------
void CGameRules::OnTextMessage(ETextMessageType type, const char *msg,
															 const char *p0, const char *p1, const char *p2, const char *p3)
{
	switch(type)
	{
	case eTextMessageConsole:
		CryLogAlways("%s", msg);
		break;
	case eTextMessageServer:
		{
			string completeMsg("** Server: ");
			completeMsg.append(msg);
			completeMsg.append(" **");

			SAFE_HUD_FUNC(DisplayTextMessage(completeMsg.c_str(), 0.0f, 3, ColorF(1,1,1)));

			CUIButtonPromptRegion::SetOnScreenMessageText("messagesFromServer", msg, NULL, 3.f);
			CryLogAlways("[server] %s", msg);
		}
		break;
	case eTextMessageError:
			SAFE_HUD_FUNC(DisplayTextMessage(msg, 0.0f, 0, ColorF(0.85f,0,0)));
			break;
	case eTextMessageInfo:
		SAFE_HUD_FUNC(DisplayTextMessage(msg, 0.0f, 2, ColorF(1,1,1)));
		break;
	case eTextMessageCenter:
		if (gEnv->bMultiplayer)
		{
			SAFE_HUD_FUNC(DisplayFlashMessage(msg, 2, ColorF(1.f,1.f,1.f), p0!=0, p0, p1, p2, p3));
		}
		else
		{
			SAFE_HUD_FUNC(DisplayTextMessage(msg, 0.0f, 1, ColorF(1,1,1)));
		}
		break;
	case eTextMessageAnnouncement:  
		SAFE_HUD_FUNC(DisplayFlashMessage(msg, 1, ColorF(0.12f,0.54f,1.f), p0!=0, p0, p1, p2, p3));
		CUIButtonPromptRegion::SetOnScreenMessageText("announcements", msg, p0, p1, NULL, 3.f);
		break;
	/*case eTextMessageTopRed:  PJH_MERGE_HACK
		SAFE_HUD_FUNC(DisplayFlashMessage(msg, 1, ColorF(0.87f,0.13f,0.13f), p0!=0, p0, p1, p2, p3));
		break;
	case eTextMessageTopYellow:
		SAFE_HUD_FUNC(DisplayFlashMessage(msg, 1, ColorF(1,0.8f,0), p0!=0, p0, p1, p2, p3));
		break;*/
	}
}

//------------------------------------------------------------------------
void CGameRules::OnChatMessage(EChatMessageType type, EntityId sourceId, EntityId targetId, const char *msg, bool teamChatOnly)
{
	//send chat message to hud
	int teamFaction = 0;
	if(IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor())
	{
		if(pActor->GetEntityId() != sourceId)
		{
			if(GetTeamCount() > 1)
			{
				if(GetTeam(pActor->GetEntityId()) == GetTeam(sourceId))
					teamFaction = 1;
				else
					teamFaction = 2;
			}
			else
				teamFaction = 2;
		}
	}	
}

//------------------------------------------------------------------------
void CGameRules::OnRevive(IActor *pActor, const Vec3 &pos, const Quat &rot, int teamId)
{
	ScriptHandle handle(pActor->GetEntityId());
	Vec3 rotVec = Vec3(Ang3(rot));
	CallScript(m_clientScript, "OnRevive", handle, pos, rotVec, teamId);
}

//------------------------------------------------------------------------
void CGameRules::OnKill(IActor *pActor, EntityId shooterId, EntityId projectileId, const char *weaponClassName, int damage, int hit_joint, int hit_type)
{
	IActor *pShooterActor=gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(shooterId);
	if (pShooterActor && pShooterActor->IsPlayer())
	{
		CPlayer *pShooterPlayer=static_cast<CPlayer*>(pShooterActor);
		pShooterPlayer->RegisterKill(pActor, hit_type);
	}

	const EntityId victimId = pActor->GetEntityId();

	ScriptHandle handleEntity(victimId), handleShooter(shooterId);
	CallScript(m_clientStateScript, "OnKill", handleEntity, handleShooter, weaponClassName, damage, hit_joint, hit_type);

	if(CPlayerProgression::GetInstance()->SkillKillEvent(this, pActor, pShooterActor, weaponClassName, damage, hit_joint, hit_type))
	{
		CDogtag::SpawnDogtag(shooterId, victimId);
	}

	SHUDEvent battleLogEvent(eHUDEvent_OnNewBattleLogMessage);
	battleLogEvent.ReserveData(3);
	battleLogEvent.AddData( hit_type );
	battleLogEvent.AddData( static_cast<int>(shooterId) );
	battleLogEvent.AddData( static_cast<int>(victimId) );
	CHUD::CallEvent(battleLogEvent);

	if (gEnv->bServer)
	{
		IGameRulesScoringModule *scoringModule = GetScoringModule();
		if (scoringModule)
		{
			scoringModule->DoScoringForDeath(pActor, shooterId, weaponClassName, damage, hit_joint, hit_type);
		}

		IGameRulesAssistScoringModule *assistScoringModule = GetAssistScoringModule();
		if (assistScoringModule)
		{
			assistScoringModule->SvDoScoringForDeath(pActor, shooterId, weaponClassName, damage, hit_joint, hit_type);
		}
	}
}


//------------------------------------------------------------------------
void CGameRules::OnActorDeath( CActor* pActor )
{
	OnActorDeath_NotifyListeners( pActor );
}


//------------------------------------------------------------------------
void CGameRules::OnReviveInVehicle(IActor *pActor, EntityId vehicleId, int seatId, int teamId)
{
	SGameObjectEvent evt(eCGE_ActorRevive,eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, (void*)pActor);
	SAFE_HUD_FUNC(HandleEvent(evt));

	ScriptHandle handle(pActor->GetEntityId());
	ScriptHandle vhandle(pActor->GetEntityId());
	CallScript(m_clientScript, "OnReviveInVehicle", handle, vhandle, seatId, teamId);
}

//------------------------------------------------------------------------
void CGameRules::OnVehicleDestroyed(EntityId id)
{
	RemoveSpawnGroup(id);

	if (gEnv->bServer)
		CallScript(m_serverScript, "OnVehicleDestroyed", ScriptHandle(id));

	if (gEnv->bClient)
		CallScript(m_clientScript, "OnVehicleDestroyed", ScriptHandle(id));

	SAFE_HUD_FUNC(VehicleDestroyed(id));
}

//------------------------------------------------------------------------
void CGameRules::OnVehicleSubmerged(EntityId id, float ratio)
{
	RemoveSpawnGroup(id);

	if (gEnv->bServer)
		CallScript(m_serverScript, "OnVehicleSubmerged", ScriptHandle(id), ratio);

	if (gEnv->bClient)
		CallScript(m_clientScript, "OnVehicleSubmerged", ScriptHandle(id), ratio);
}

//------------------------------------------------------------------------
void CGameRules::IncreasePoints(EntityId who, const SGameRulesScoreInfo & scoreInfo)
{
	CryLogAlways ("CGameRules::IncreasePoints [bServer=%d bClient=%d] rewarding %d points to player %s", gEnv->bServer, gEnv->bClient, scoreInfo.score, GetEntityName(who));
	assert (gEnv->bServer);

	CCCPOINT(GameRules_SvModifyScore);

	IGameRulesStateModule *pStateModule = GetStateModule();

	// No scoring at game end
	if (pStateModule && pStateModule->GetGameState() != IGameRulesStateModule::EGRS_PostGame && scoreInfo.score != 0)
	{
		// Part 1: add to the magical and definitive server-side table of many scores! - Only if you have the module
		IGameRulesPlayerStatsModule *pPlayerStats = GetPlayerStatsModule();
		if (pPlayerStats)
		{
			pPlayerStats->IncreasePoints(who, scoreInfo.score);
		}

		// Part 2: send an RMI to the machine belonging to the person who scored so they can display a message!
		IActor * whoActor =(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(who));
		CRY_ASSERT_MESSAGE(whoActor, string().Format("Can't give a score of %d to entity %d '%s' because it's not an actor", scoreInfo.score, who, GetEntityName(who)));
		if (whoActor)
		{
			CGameRules::ScoreChangeParams params(scoreInfo.data.PlayerKill.victim, scoreInfo.score, scoreInfo.type);
			GetGameObject()->InvokeRMI(CGameRules::ClAddPoints(), params, eRMI_ToClientChannel, whoActor->GetChannelId());
		}

		// Part 3: Tell listeners
		if(who == g_pGame->GetIGameFramework()->GetClientActorId())
		{
			ClientScoreEvent(scoreInfo.type, scoreInfo.score);
		}

	}
#ifndef _RELEASE
	else
	{
		CryLogAlways ("CGameRules::IncreasePoints NOT adding to score because game %s", pStateModule ? "has finished" : "has no 'state' module");
	}
#endif
}

//------------------------------------------------------------------------
void CGameRules::AddTaggedEntity(EntityId shooter, EntityId targetId, bool temporary, float time, ERadarTagReason reason)
{
	if(!gEnv->bServer) // server sends to all clients
		return;

	if(GetTeamCount() > 1)
	{
		EntityId teamId = GetTeam(shooter);
		TPlayerTeamIdMap::const_iterator tit=m_playerteams.find(teamId);
		if (tit!=m_playerteams.end())
		{
			for (TPlayers::const_iterator it=tit->second.begin(); it!=tit->second.end(); ++it)
			{
				if(temporary)
				{
					TempRadarTaggingParams params(shooter, targetId, time, reason);
					GetGameObject()->InvokeRMI(ClTempRadarEntity(), params, eRMI_ToClientChannel, GetChannelId(*it));
				}
				else
				{
					EntityParams params(targetId);
					GetGameObject()->InvokeRMI(ClTaggedEntity(), params, eRMI_ToClientChannel, GetChannelId(*it));
				}
			}
		}
	}
	else
	{
		if(temporary)
		{
			TempRadarTaggingParams params(shooter, targetId, time, reason);
			GetGameObject()->InvokeRMI(ClTempRadarEntity(), params, eRMI_ToClientChannel, GetChannelId(shooter));
		}
		else
		{
			EntityParams params(targetId);
			GetGameObject()->InvokeRMI(ClTaggedEntity(), params, eRMI_ToClientChannel, GetChannelId(shooter));
		}
	}

	if (reason==eRTR_AssaultBinoculars)
	SAFE_HUD_FUNC(GetRadar()->AddBinocularEntityTemporarily(targetId, 15.0f));
	// add PP and CP for tagging this entity
	ScriptHandle shooterHandle(shooter);
	ScriptHandle targetHandle(targetId);
	CallScript(m_serverScript, "OnAddTaggedEntity", shooterHandle, targetHandle);
}

//------------------------------------------------------------------------
void CGameRules::RequestAddTempRadarEntity(EntityId shooter, EntityId targetId, float time)
{
	CryLog("[tlh] @ CGameRules::RequestAddTempRadarEntity()");

	if (gEnv->bServer)
	{
		AddTaggedEntity(shooter, targetId, true, time);  // this sends RMIs to the clients whose radars should be affected

		InformTaggedByCCTVParams  params(shooter);
		GetGameObject()->InvokeRMI(ClInformTaggedByCCTV(), params, eRMI_ToClientChannel, GetChannelId(targetId));
	}
	else
	{
		TempRadarTaggingParams  params(shooter, targetId, time);
		GetGameObject()->InvokeRMI(SvRequestTempRadarEntity(), params, eRMI_ToServer);
	}
}

//------------------------------------------------------------------------
void CGameRules::OnKillMessage(EntityId targetId, EntityId shooterId, const char *weaponClassName, float damage, int material, int hit_type)
{
	if(EntityId client_id = g_pGame->GetIGameFramework()->GetClientActor()?g_pGame->GetIGameFramework()->GetClientActor()->GetEntityId():0)
	{
		if(!gEnv->bServer && gEnv->bClient && client_id == shooterId && client_id != targetId)
		{
			m_pGameplayRecorder->Event(gEnv->pGame->GetIGameFramework()->GetClientActor()->GetEntity(), GameplayEvent(eGE_Kill, weaponClassName)); 
		}
		SAFE_HUD_FUNC(ObituaryMessage(targetId, shooterId, weaponClassName, material, hit_type));
	}

	// check shooter and target exist.
	IActorSystem *pActorSystem = g_pGame->GetIGameFramework()->GetIActorSystem();
	assert(pActorSystem);
	IActor* target = pActorSystem->GetActor(targetId);
	IActor* shooter = pActorSystem->GetActor(shooterId);
	if( target && shooter )
	{
		IGameRulesPlayerStatsModule*  pPlayStatsMo = GetPlayerStatsModule();
		if( pPlayStatsMo )
		{
			//increment shooter's kill count on target
			pPlayStatsMo->IncreaseKillCount( shooterId, targetId );
		}
	}
}

//------------------------------------------------------------------------
IActor *CGameRules::SpawnPlayer(int channelId, const char *name, const char *className, const Vec3 &pos, const Ang3 &angles)
{ 
	if (!gEnv->bServer)
		return 0;

	IActor *pActor=GetActorByChannelId(channelId);
	if (!pActor)
		pActor = m_pActorSystem->CreateActor(channelId, VerifyName(name).c_str(), className, pos, Quat(angles), Vec3(1, 1, 1));

	return pActor;
}

//------------------------------------------------------------------------
IActor *CGameRules::ChangePlayerClass(int channelId, const char *className)
{
	if (!gEnv->bServer)
		return 0;

	IActor *pOldActor = GetActorByChannelId(channelId);
	if (!pOldActor)
		return 0;

	if (!strcmp(pOldActor->GetEntity()->GetClass()->GetName(), className))
		return pOldActor;

	EntityId oldEntityId = pOldActor->GetEntityId();
	string oldName = pOldActor->GetEntity()->GetName();
	Quat oldOri = pOldActor->GetEntity()->GetWorldRotation();
	Vec3 oldPos = pOldActor->GetEntity()->GetWorldPos();

	m_pEntitySystem->RemoveEntity(pOldActor->GetEntityId(), true);

	IActor *pActor = m_pActorSystem->CreateActor(channelId, oldName.c_str(), className, oldPos, oldOri, Vec3(1, 1, 1), oldEntityId);
	if (pActor)
		MovePlayer(pActor, oldPos, oldOri);

	return pActor;
}

void CGameRules::RevivePlayerMP(IActor *pActor, IEntity *pSpawnPoint, int teamId, bool clearInventory)
{
	if(!gEnv->bServer)
	{
		GameWarning("CGameRules::RevivePlayer() called on client");
		return;
	}

	if(!pActor)
		return;

	assert(pSpawnPoint);

	EntityId spawnPointId = pSpawnPoint->GetId();
	CActor *pCActor = static_cast<CActor*>(pActor);

	RevivePlayer(pActor, pSpawnPoint->GetWorldPos(), pSpawnPoint->GetWorldAngles(), teamId, clearInventory);

	pActor->GetGameObject()->InvokeRMIWithDependentObject(CActor::ClRevive(), CActor::ReviveParams(spawnPointId, teamId, pCActor->GetNetPhysCounter()), eRMI_ToAllClients|eRMI_NoLocalCalls, spawnPointId);
}

//------------------------------------------------------------------------
void CGameRules::RevivePlayer(IActor *pActor, const Vec3 &pos, const Ang3 &angles, int teamId, bool clearInventory)
{
	CryLog("CGameRules::RevivePlayer() actor=%s", pActor->GetEntity()->GetName());

	if (!gEnv->bServer)
	{
		GameWarning("CGameRules::RevivePlayer() called on client");
		return;
	}

	if (!pActor)
		return;

	if (pActor->IsClient())
	{
		if( g_pGame->GetRecordingSystem() )
		{
			g_pGame->GetRecordingSystem()->StartRecording();
		}
	}

	// get out of vehicles before reviving
	if (IVehicle *pVehicle = pActor->GetLinkedVehicle())
	{
		if (IVehicleSeat *pSeat = pVehicle->GetSeatForPassenger(GetEntityId()))
			pSeat->Exit(false);
	}

	// stop using any mounted weapons before reviving
	if (CItem *pItem=static_cast<CItem *>(pActor->GetCurrentItem()))
	{
		if (pItem->IsMounted())
			pItem->StopUse(GetEntityId());
	}

	if (!m_pGameFramework->IsChannelOnHold(pActor->GetChannelId()))
		GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Alive);

	Matrix34 tm(GetEntity()->GetWorldTM());
	tm.SetTranslation(pos);
	tm.SetRotationXYZ(angles);

	GetEntity()->SetWorldTM(tm);
	//SetAngles(angles);

	if (clearInventory && !gEnv->bMultiplayer)
	{
		IInventory *pInventory = pActor->GetInventory();
		pInventory->Destroy();
		pInventory->Clear();
	}

	if (CActor* pCActor = static_cast<CActor*>(pActor))
	{
		pCActor->NetReviveAt(pos, Quat(angles), teamId);

		if (gEnv->bMultiplayer)
		{
			if (pCActor->GetSpectatorState()==CActor::eASS_None)
			{
				pCActor->SetSpectatorState(CActor::eASS_Ingame);
			}
		}
	}

	if (GetPlayerSetupModule())
	{
		GetPlayerSetupModule()->OnPlayerRevived(pActor->GetEntityId());
	}
	else
	{
		CallScript(m_script, "EquipPlayer", pActor->GetEntity()->GetScriptTable(), false);
	}

	m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Revive));

	if (m_statsRecordingModule)
	{
		CRY_ASSERT_MESSAGE(m_stateModule,"stats recording module requires an implementation of game state module to work. make sure there's one in this game modes XML");
		if (m_stateModule->GetGameState()==IGameRulesStateModule::EGRS_InGame)
		{
			m_statsRecordingModule->OnPlayerRevived(pActor);
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::RevivePlayerInVehicle(IActor *pActor, EntityId vehicleId, int seatId, int teamId/* =0 */, bool clearInventory/* =true */)
{
	if (!gEnv->bServer)
	{
		GameWarning("CGameRules::RevivePlayerInVehicle() called on client");
		return;
	}

	if (!pActor)
		return;

	CActor* pCActor = static_cast<CActor*>(pActor);

	// might get here with an invalid (-ve) seat id if all seats are currently occupied. 
	// In that case we use the seat exit code to find a valid position to spawn at.
	if(seatId < 0)
	{
		IVehicle* pSpawnVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(vehicleId);
		Vec3 pos = ZERO;
		if(pSpawnVehicle && pSpawnVehicle->GetExitPositionForActor(pActor, pos, true))
		{
			Ang3 angles = pSpawnVehicle->GetEntity()->GetWorldAngles();	// face same direction as vehicle.
			RevivePlayer(pActor, pos, angles, teamId, clearInventory);
			return;
		}
	}

	if (IVehicle *pVehicle = pActor->GetLinkedVehicle())
	{
		if (IVehicleSeat *pSeat=pVehicle->GetSeatForPassenger(pActor->GetEntityId()))
			pSeat->Exit(false); 
	}

	// stop using any mounted weapons before reviving
	if (CItem *pItem=static_cast<CItem *>(pActor->GetCurrentItem()))
	{
		if (pItem->IsMounted())
			pItem->StopUse(pActor->GetEntityId());
	}

	pCActor->SetHealth(100);
	pCActor->SetMaxHealth(100);

	if (!m_pGameFramework->IsChannelOnHold(pActor->GetChannelId()))
		pActor->GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Alive);

	if (clearInventory && !gEnv->bMultiplayer)
	{
		IInventory *pInventory = pActor->GetInventory();
		pInventory->Destroy();
		pInventory->Clear();
	}

	pCActor->NetReviveInVehicle(vehicleId, seatId, teamId);

	m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Revive));
}

//------------------------------------------------------------------------
void CGameRules::RenamePlayer(IActor *pActor, const char *name)
{
	string fixed=VerifyName(name, pActor->GetEntity());
	RenameEntityParams params(pActor->GetEntityId(), fixed.c_str());
	if (!stricmp(fixed.c_str(), pActor->GetEntity()->GetName()))
		return;

	if (gEnv->bServer)
	{
		if (!gEnv->bClient)
			pActor->GetEntity()->SetName(fixed.c_str());

		GetGameObject()->InvokeRMIWithDependentObject(ClRenameEntity(), params, eRMI_ToAllClients, params.entityId);

		if (INetChannel* pNetChannel = pActor->GetGameObject()->GetNetChannel())
			pNetChannel->SetNickname(fixed.c_str());

		m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Renamed, fixed));
	}
	else if (pActor->GetEntityId() == m_pGameFramework->GetClientActor()->GetEntityId())
		GetGameObject()->InvokeRMIWithDependentObject(SvRequestRename(), params, eRMI_ToServer, params.entityId);
}

//------------------------------------------------------------------------
string CGameRules::VerifyName(const char *name, IEntity *pEntity)
{
	string nameFormatter(name);

	// size limit is 26
	if (nameFormatter.size()>26)
		nameFormatter.resize(26);

	// no spaces at start/end
	nameFormatter.TrimLeft(' ');
	nameFormatter.TrimRight(' ');

	// no empty names
	if (nameFormatter.empty())
		nameFormatter="empty";

	// no @ signs
	nameFormatter.replace("@", "_");

	// search for duplicates
	if (IsNameTaken(nameFormatter.c_str(), pEntity))
	{
		int n=1;
		string appendix;
		do 
		{
			appendix.Format("(%d)", n++);
		} while(IsNameTaken(nameFormatter+appendix));

		nameFormatter.append(appendix);
	}

	return nameFormatter;
}

//------------------------------------------------------------------------
bool CGameRules::IsNameTaken(const char *name, IEntity *pEntity)
{
	for (std::vector<int>::const_iterator it=m_channelIds.begin(); it!=m_channelIds.end(); ++it)
	{
		IActor *pActor=GetActorByChannelId(*it);
		if (pActor && pActor->GetEntity()!=pEntity && !stricmp(name, pActor->GetEntity()->GetName()))
			return true;
	}

	return false;
}

//------------------------------------------------------------------------
void CGameRules::KillPlayer(IActor *pActor, bool dropItem, bool ragdoll, EntityId shooterId, EntityId weaponId, float damage,
														int hitJoint, int hit_type, const Vec3 &impulse, EntityId projectileId)
{
	if(!gEnv->bServer)
		return;

	if (!pActor)
		return;

	//if self destruct then 'adjust' the shooter
	if(pActor->IsPlayer())
	{
		CPlayer* pTargetPlayer = static_cast<CPlayer*>(pActor);
		if(pTargetPlayer->GetPerkData<bool>(EPD_SelfDestructMinigame))
		{
			shooterId = pTargetPlayer->GetPerkData<EntityId>(EPD_SelfDestructKiller);
		}
	}

	EntityId actorEid = pActor->GetEntityId();
	CActor* pCActor = static_cast<CActor*>(pActor);

	IInventory *pInventory = pActor->GetInventory();
	EntityId itemId = pInventory ? pInventory->GetCurrentItem() : 0;
	if (itemId && !pActor->GetLinkedVehicle())
	{
		CItem *pItem = pCActor->GetItem(itemId);
		if (pItem && pItem->IsUsed())
			pItem->StopUse(actorEid);
		else if (pItem && dropItem)
			pActor->DropItem(itemId, 1.0f, false, true);
	}

	uint16 weaponClassId=0;
	const char *weaponClassName="";
	if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(weaponId))
	{
		weaponClassName=pEntity->GetClass()->GetName();
		m_pGameFramework->GetNetworkSafeClassId(weaponClassId, weaponClassName);
		assert(weaponClassId!=0);
	}
/*
	if (pInventory)
		pInventory->Destroy();
*/
	IActor *pShooter= NULL;
	if (pShooter=m_pGameFramework->GetIActorSystem()->GetActor(shooterId))
	{
		CRecordingSystem *crs = g_pGame->GetRecordingSystem();
		if ( crs && pShooter->IsClient() )
		{
			crs->QueueSendRequest(SKillCamSendRequest(shooterId, actorEid));
		}
	}

	CActor::KillParams params;
	params.shooterId = shooterId;
	params.projectileId = projectileId;
	params.weaponClassId = weaponClassId;
	params.damage = damage;
	params.material = -1;
	params.hit_type = hit_type;
	params.hit_joint = hitJoint;

	CRY_TODO(12, 2, 2010, "Plug in direct net-serializing of the PART id?");
	//params.hit_part  = GetDamageHandlingModule() ? GetDamageHandlingModule()->GetPartId(hitJoint) : hitJoint;

	params.impulse = impulse;
	params.ragdoll = ragdoll;

	CryLogAlways("[CGameRules::KillPlayer] HitJoint: %d  PartID: %d", hitJoint, params.hit_joint);

	pCActor->NetKill(params);

	pActor->GetGameObject()->InvokeRMI(CActor::ClKill(), params, eRMI_ToAllClients|eRMI_NoLocalCalls);

	m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Death));

	CStatsRecordingMgr		*sr=g_pGame->GetStatsRecorder();
	if (sr && sr->ShouldRecordEvent(eSE_Death, pActor))
	{
		if (IStatsTracker *tracker = sr->GetStatsTracker(pActor) )
		{
			tracker->Event(eSE_Death, new CDeathStats(projectileId, shooterId, GetHitType(hit_type, "unknown hit type")));
		}
	}

	if (shooterId && shooterId!=actorEid && pShooter && sr && sr->ShouldRecordEvent(eSE_Kill, pShooter))
	{
		m_pGameplayRecorder->Event(pShooter->GetEntity(), GameplayEvent(eGE_Kill, 0, 0, (void *)&weaponId));
		if ( IStatsTracker *tracker = sr->GetStatsTracker(pShooter) )
		{
			tracker->Event( eSE_Kill, new CKillStats(projectileId, pActor->GetEntityId(), GetHitType(hit_type, "unknown hit type")));
		}
	}

	HitInfo  dummyHitInfo;
	dummyHitInfo.shooterId = shooterId;
	dummyHitInfo.targetId = actorEid;
	dummyHitInfo.weaponId = weaponId;
	dummyHitInfo.damage = damage;
	dummyHitInfo.partId = hitJoint;
	dummyHitInfo.type = hit_type;

	CRY_TODO(02, 10, 2009, "CG: Change these to use the listener stuff");
	if (m_spawningModule)
	{
		m_spawningModule->SvOnPlayerKilled(dummyHitInfo);
	}
	IGameRulesPlayerStatsModule *statsModule = GetPlayerStatsModule();
	if (statsModule)
	{
		statsModule->OnPlayerKilled(dummyHitInfo);
	}
	if (m_statsRecordingModule)
	{
		m_statsRecordingModule->OnPlayerKilled(pActor);
	}

	OnEntityKilled(dummyHitInfo);
}

//------------------------------------------------------------------------
void CGameRules::MovePlayer(IActor *pActor, const Vec3 &pos, const Quat &orientation)
{
	IVehicle *pVehicle = static_cast<CActor*>(pActor)->GetLinkedVehicle();
	if(pVehicle)
	{
		pVehicle->ExitVehicleAtPosition(pActor->GetEntityId(), pos);
	}

	CActor::MoveParams params(pos, orientation);
	//move player on client
	pActor->GetGameObject()->InvokeRMI(CActor::ClMoveTo(), params, eRMI_ToClientChannel|eRMI_NoLocalCalls, pActor->GetChannelId());

	//move player on server
	static_cast<CActor*>(pActor)->OnTeleported();
	pActor->GetEntity()->SetWorldTM(Matrix34::Create(Vec3(1,1,1), params.rot, params.pos));
}


//------------------------------------------------------------------------
void CGameRules::ChangeTeam(IActor *pActor, int teamId)
{
	if (teamId == GetTeam(pActor->GetEntityId()))
		return;

	ChangeTeamParams params(pActor->GetEntityId(), teamId);

	if (gEnv->bServer)
	{
		if (m_teamsModule)
		{
			m_teamsModule->RequestChangeTeam(pActor->GetEntityId(), teamId);
		}
		else
		{
			ScriptHandle handle(params.entityId);
			CallScript(m_serverStateScript, "OnChangeTeam", handle, params.teamId);
		}
	}
	else if (pActor->GetEntityId() == m_pGameFramework->GetClientActor()->GetEntityId())
		GetGameObject()->InvokeRMIWithDependentObject(SvRequestChangeTeam(), params, eRMI_ToServer, params.entityId);
}

//------------------------------------------------------------------------
void CGameRules::ChangeTeam(IActor *pActor, const char *teamName)
{
	if (!teamName)
		return;

	int teamId=GetTeamId(teamName);

	if (!teamId)
	{
		CryLogAlways("Invalid team: %s", teamName);
		return;
	}

	ChangeTeam(pActor, teamId);
}

//------------------------------------------------------------------------
int CGameRules::GetPlayerCount(bool inGame) const
{
	if (!inGame)
		return (int)m_channelIds.size();

	int count=0;
	for (std::vector<int>::const_iterator it=m_channelIds.begin(); it!=m_channelIds.end(); ++it)
	{
		if (IsChannelInGame(*it))
			++count;
	}

	return count;
}

//------------------------------------------------------------------------
int CGameRules::GetSpectatorCount(bool inGame) const
{
	int count=0;
	for (std::vector<int>::const_iterator it=m_channelIds.begin(); it!=m_channelIds.end(); ++it)
	{
		IActor *pActor = GetActorByChannelId(*it);
		if (pActor && (pActor->GetSpectatorMode() != 0))
		{
			if (!inGame || IsChannelInGame(*it))
				++count;
		}
	}

	return count;
}

//------------------------------------------------------------------------
EntityId CGameRules::GetPlayer(int idx)
{
	if (idx<0||idx>=m_channelIds.size())
		return 0;

	IActor *pActor = GetActorByChannelId(m_channelIds[idx]);
	return pActor ? pActor->GetEntityId() : 0;
}


//------------------------------------------------------------------------
// called everytime a new player enters the game or is disconnected

void CGameRules::UpdateCoopPlayersIds()
{
	m_coopPlayerHost = 0;
	m_coopPlayerClient = 0;

	IActor* pLocalActor = g_pGame->GetIGameFramework()->GetClientActor();
	if (!pLocalActor)
		return;
		
	EntityId localPlayerId = pLocalActor->GetEntityId();
	EntityId otherPlayerId = 0;

	IActorIteratorPtr pIter = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
	while (IActor *pActor = pIter->Next())
	{
		if (pActor->GetEntityId()!=localPlayerId && pActor->GetChannelId())
		{
			otherPlayerId = pActor->GetEntityId();
			break;
		}
	}
	
	if (gEnv->bServer)
	{
		m_coopPlayerHost = localPlayerId;
		m_coopPlayerClient = otherPlayerId;
	}
	else
	{
		m_coopPlayerClient = localPlayerId;
		m_coopPlayerHost = otherPlayerId;
	}
}


//------------------------------------------------------------------------
void CGameRules::GetPlayers(TPlayers &players) const
{
	players.resize(0);
	players.reserve(m_channelIds.size());

	for (std::vector<int>::const_iterator it=m_channelIds.begin(); it!=m_channelIds.end(); ++it)
	{
		IActor *pActor=GetActorByChannelId(*it);
		if (pActor)
			players.push_back(pActor->GetEntityId());
	}
}

void CGameRules::GetPlayersClient(TPlayers &players) const
{
	players.resize(0);

	for (TPlayerTeamIdMap::const_iterator it=m_playerteams.begin(); it!=m_playerteams.end(); ++it)
	{
		TPlayers tpl = it->second;
		for (TPlayers::const_iterator player=tpl.begin();player!=tpl.end(); ++player)
		{
			players.push_back(*player);
		}
	}
}


//------------------------------------------------------------------------
// returns true if the id is a player
//------------------------------------------------------------------------
bool CGameRules::IsPlayer(EntityId playerId) const
{
	IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId);
	
	return (pActor && pActor->IsPlayer());
}


//------------------------------------------------------------------------
bool CGameRules::IsPlayerInGame(EntityId playerId) const
{
	INetChannel *pNetChannel=g_pGame->GetIGameFramework()->GetNetChannel(GetChannelId(playerId));
	if (pNetChannel && pNetChannel->GetContextViewState()>=eCVS_InGame)
		return true;
	return false;
}

//------------------------------------------------------------------------
bool CGameRules::IsPlayerActivelyPlaying(EntityId playerId, bool mustBeAlive) const
{
	if(!gEnv->bMultiplayer)
		return true;

	// 'actively playing' means they have selected a team / joined the game.

	if(GetTeamCount() == 1)
	{
		IActor* pActor = reinterpret_cast<IActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));
		if(!pActor) 
			return false;

		if (mustBeAlive)
			return (pActor->GetHealth() >= 0 && pActor->GetSpectatorMode() == CActor::eASM_None);
		else
			return (pActor->GetHealth() >= 0 || pActor->GetSpectatorMode() == CActor::eASM_None);
	}
	else
	{
		// in PS/TIA, out of the game if not yet on a team
		if(!mustBeAlive)
			return (GetTeam(playerId) != 0 );

		CActor* pActor = reinterpret_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));
		if(!pActor) 
			return false;
		return (pActor->GetHealth() > 0 && GetTeam(playerId) != 0);

	}
}

//------------------------------------------------------------------------
bool CGameRules::IsChannelInGame(int channelId) const
{
	INetChannel *pNetChannel=g_pGame->GetIGameFramework()->GetNetChannel(channelId);
	if (pNetChannel && pNetChannel->GetContextViewState()>=eCVS_InGame)
		return true;
	return false;
}

//------------------------------------------------------------------------
void CGameRules::StartVoting(IActor *pActor, EVotingState t, EntityId id, const char* param)
{
  if(!pActor)
    return;

  StartVotingParams params(t,id,param);
  EntityId entityId = pActor->GetEntityId();
  
  if (gEnv->bServer)
  {
    if(!m_pVotingSystem)
      return;
    CTimeValue st;
    CTimeValue curr_time = gEnv->pTimer->GetFrameStartTime();

    if(!m_pVotingSystem->GetCooldownTime(entityId,st) || (curr_time-st).GetSeconds()>g_pGame->GetCVars()->sv_votingCooldown)
    {
      if(m_pVotingSystem->StartVoting(entityId,curr_time,t,id,param,GetTeam(id)))
      {
        m_pVotingSystem->Vote(entityId,GetTeam(entityId), true);
        VotingStatusParams st_param(t,g_pGame->GetCVars()->sv_votingTimeout,id,param);
        GetGameObject()->InvokeRMI(ClVotingStatus(), st_param, eRMI_ToAllClients);
				if(t == eVS_kick)
				{
					CryFixedStringT<256> feedbackString("@mp_vote_initialized_kick:#:");

					feedbackString.append(param);
					SendChatMessage(eChatToAll, id, 0, feedbackString.c_str());
				}
				else
					SendChatMessage(eChatToAll, id, 0, "@mp_vote_initialized_nextmap");
      }
    }
    else
    {
      CryLog("Player %s cannot start voting yet",pActor->GetEntity()->GetName());
    }
  }
  else if (pActor->GetEntityId() == m_pGameFramework->GetClientActor()->GetEntityId())
    GetGameObject()->InvokeRMIWithDependentObject(SvStartVoting(), params, eRMI_ToServer, entityId);
}

//------------------------------------------------------------------------
void CGameRules::Vote(IActor* pActor, bool yes)
{
  if(!pActor)
    return;
  EntityId id = pActor->GetEntityId();

  if (gEnv->bServer)
  {
    if(!m_pVotingSystem)
      return;
    if(m_pVotingSystem->CanVote(id) && m_pVotingSystem->IsInProgress())
    {
      m_pVotingSystem->Vote(id,GetTeam(id), yes);
			if(yes)
				SendChatMessage(eChatToAll, id, 0, "@mp_voted");
			else
				SendChatMessage(eChatToAll, id, 0, "@mp_votedno");
    }
    else
    {
      CryLog("Player %s cannot vote",pActor->GetEntity()->GetName());
    }
  }
  else if (id == m_pGameFramework->GetClientActor()->GetEntityId())
	{
		if(yes)
			GetGameObject()->InvokeRMIWithDependentObject(SvVote(), NoParams(), eRMI_ToServer, id);
		else
			GetGameObject()->InvokeRMIWithDependentObject(SvVoteNo(), NoParams(), eRMI_ToServer, id);
	}
}

//------------------------------------------------------------------------
void CGameRules::EndVoting(bool success)
{
  if(!m_pVotingSystem || !gEnv->bServer)
    return;

  if(success)
  {
    CryLog("Voting \'%s\' succeeded.",m_pVotingSystem->GetSubject().c_str());
    switch(m_pVotingSystem->GetType())
    {
    case eVS_consoleCmd:
      gEnv->pConsole->ExecuteString(m_pVotingSystem->GetSubject());
      break;
    case eVS_kick:
      {
        int ch_id = GetChannelId(m_pVotingSystem->GetEntityId());
        if(INetChannel* pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(ch_id))
          pNetChannel->Disconnect(eDC_Kicked,"Kicked from server by voting");
      }
      break;
    case eVS_nextMap:
      NextLevel();
      break;
    case eVS_changeMap:
      m_pGameFramework->ExecuteCommandNextFrame(string("map ")+m_pVotingSystem->GetSubject());
      break;
    case eVS_none:
      break;
    }
  }
  else
    CryLog("Voting \'%s\' ended.",m_pVotingSystem->GetSubject().c_str());

  m_pVotingSystem->EndVoting();
  VotingStatusParams params(eVS_none, 0, GetEntityId(), "");
  GetGameObject()->InvokeRMI(ClVotingStatus(), params, eRMI_ToAllClients);
}

//------------------------------------------------------------------------
int CGameRules::CreateTeam(const char *name)
{
	TTeamIdMap::iterator it = m_teams.find(CONST_TEMP_STRING(name));
	if (it != m_teams.end())
		return it->second;

	CTeamPerks defaultPerks;

	m_teams.insert(TTeamIdMap::value_type(name, ++m_teamIdGen));
	m_playerteams.insert(TPlayerTeamIdMap::value_type(m_teamIdGen, TPlayers()));

	int startTeamScore = 0;
	if (m_scoringModule)
	{
		startTeamScore = m_scoringModule->GetStartTeamScore();
	}

	CRY_TODO(08, 09, 2009, "Team scores need moving into some kind of team info when there's a teams module. Hopefully will hold player, name etc. instead of all these maps.");
	m_teamscores.insert(TTeamScoresMap::value_type(m_teamIdGen, STeamScore(startTeamScore,0)));

	if (gEnv->bMultiplayer && !IsGameRulesClass("Coop"))
	{
		m_teamperks.insert(TTeamPerkMap::value_type(m_teamIdGen, defaultPerks));
	}

	CStatsRecordingMgr		*sr=g_pGame->GetStatsRecorder();
	if(sr)
	{
		sr->AddTeam(m_teamIdGen, name);
	}

	return m_teamIdGen;
}

//------------------------------------------------------------------------
void CGameRules::RemoveTeam(int teamId)
{
	if(!stl::member_find_and_erase(m_teams, CONST_TEMP_STRING(GetTeamName(teamId))))
		return;

	for (TEntityTeamIdMap::iterator eit=m_entityteams.begin(); eit != m_entityteams.end(); ++eit)
	{
		if (eit->second == teamId)
			eit->second = 0; // 0 is no team
	}

	stl::member_find_and_erase(m_playerteams, teamId);
	stl::member_find_and_erase(m_teamperks, teamId);
	stl::member_find_and_erase(m_teamscores, teamId);
}

//------------------------------------------------------------------------
const char *CGameRules::GetTeamName(int teamId) const
{
	for (TTeamIdMap::const_iterator it = m_teams.begin(); it!=m_teams.end(); ++it)
	{
		if (teamId == it->second)
			return it->first;
	}

	return 0;
}

//------------------------------------------------------------------------
int CGameRules::GetTeamId(const char *name) const
{
	TTeamIdMap::const_iterator it=m_teams.find(CONST_TEMP_STRING(name));
	if (it!=m_teams.end())
		return it->second;

	return 0;
}

//------------------------------------------------------------------------
int CGameRules::GetTeamCount() const
{
	return (int)m_teams.size();
}

//------------------------------------------------------------------------
int CGameRules::GetTeamPlayerCount(int teamId, bool inGame) const
{
	if (!inGame)
	{
		TPlayerTeamIdMap::const_iterator it=m_playerteams.find(teamId);
		if (it!=m_playerteams.end())
			return (int)it->second.size();
		return 0;
	}
	else
	{
		TPlayerTeamIdMap::const_iterator it=m_playerteams.find(teamId);
		if (it!=m_playerteams.end())
		{
			int count=0;

			const TPlayers &players=it->second;
			for (TPlayers::const_iterator pit=players.begin(); pit!=players.end(); ++pit)
				if (IsPlayerInGame(*pit))
					++count;

			return count;
		}
		return 0;
	}
}

//------------------------------------------------------------------------
void CGameRules::SetTeamsScore(int teamId, int score)
{
	CRY_ASSERT(gEnv->bServer);
	TTeamScoresMap::iterator it=m_teamscores.find(teamId);
	if (it!=m_teamscores.end())
	{
		it->second.m_teamScore = score;
		g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(GetEntityId(), GAMERULES_TEAMS_SCORE_ASPECT);
	}
}

//------------------------------------------------------------------------
int CGameRules::GetTeamsScore(int teamId) const
{
	TTeamScoresMap::const_iterator it=m_teamscores.find(teamId);
	if (it!=m_teamscores.end())
		return it->second.m_teamScore;

	return 0;
}

//------------------------------------------------------------------------
void CGameRules::SetTeamRoundScore(int teamId, int score)
{
	CRY_ASSERT(gEnv->bServer);
	TTeamScoresMap::iterator it=m_teamscores.find(teamId);
	if (it!=m_teamscores.end())
	{
		it->second.m_roundTeamScore = score;
		g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(GetEntityId(), GAMERULES_TEAMS_SCORE_ASPECT);
	}
}

//------------------------------------------------------------------------
int CGameRules::SvGetTeamsScoreScoredThisRound(int teamId) const
{
	CRY_ASSERT(gEnv->bServer);

	TTeamScoresMap::const_iterator it=m_teamscores.find(teamId);
	if (it!=m_teamscores.end())
		return (it->second.m_teamScore - it->second.m_teamScoreRoundStart);

	return 0;
}

//------------------------------------------------------------------------
int CGameRules::GetTeamRoundScore(int teamId) const
{
	TTeamScoresMap::const_iterator it=m_teamscores.find(teamId);
	if (it!=m_teamscores.end())
		return it->second.m_roundTeamScore;

	return 0;
}

//------------------------------------------------------------------------
void CGameRules::SvCacheRoundStartTeamScores()
{
	CRY_ASSERT(gEnv->bServer);
	for (TTeamScoresMap::iterator iter=m_teamscores.begin(); iter!=m_teamscores.end(); ++iter)
	{
		iter->second.m_teamScoreRoundStart = iter->second.m_teamScore;
		g_pGame->GetIGameFramework()->GetNetContext()->ChangedAspects(GetEntityId(), GAMERULES_TEAMS_SCORE_ASPECT);
	}
}

//------------------------------------------------------------------------
// skipPlayerId - newly spawned player, might have not health yet (if respawning in game), but must be considered alive
int CGameRules::GetTotalAlivePlayerCount( const EntityId skipPlayerId ) const
{
	int count=0;
	for(TPlayerTeamIdMap::const_iterator it=m_playerteams.begin(); it!=m_playerteams.end(); ++it)
	{
		const TPlayers &players=it->second;
		for (TPlayers::const_iterator pit=players.begin(); pit!=players.end(); ++pit)
			if ( skipPlayerId==(*pit) || IsPlayerActivelyPlaying(*pit, true))
				++count;
	}
	return count;
}

//------------------------------------------------------------------------
int CGameRules::GetTeamChannelCount(int teamId, bool inGame) const
{
	int count=0;
	for (TChannelTeamIdMap::const_iterator it=m_channelteams.begin(); it!=m_channelteams.end(); ++it)
	{
		if (teamId==it->second)
		{
			if (!inGame || IsChannelInGame(it->first))
				++count;
		}
	}

	return count;
}

//------------------------------------------------------------------------
EntityId CGameRules::GetTeamActivePlayer(int teamId, int idx) const
{
	TPlayerTeamIdMap::const_iterator it=m_playerteams.find(teamId);
	if (it==m_playerteams.end())
		return 0;
	int count=0;
	const TPlayers &players=it->second;
	for (TPlayers::const_iterator pit=players.begin(); pit!=players.end(); ++pit)
		if ((IsPlayerActivelyPlaying(*pit, true)))
			if((count++)==idx)
				return (*pit);
	return 0;
}

//------------------------------------------------------------------------
EntityId CGameRules::GetTeamPlayer(int teamId, int idx)
{
	TPlayerTeamIdMap::const_iterator it=m_playerteams.find(teamId);
	if (it!=m_playerteams.end())
	{
		if (idx>=0 && idx<it->second.size())
			return it->second[idx];
	}

	return 0;
}

//------------------------------------------------------------------------
void CGameRules::GetTeamPlayers(int teamId, TPlayers &players)
{
	players.resize(0);
	TPlayerTeamIdMap::const_iterator it=m_playerteams.find(teamId);
	if (it!=m_playerteams.end())
		players=it->second;
}

//------------------------------------------------------------------------
void CGameRules::SetTeam(int teamId, EntityId id)
{
	assert(gEnv->bServer);

	

	int oldTeam = GetTeam(id);
	if (oldTeam==teamId)
		return;

	stl::member_find_and_erase(m_entityteams, id);

	IActor *pActor=m_pActorSystem->GetActor(id);
	bool isplayer = (pActor != 0)  && (pActor->IsPlayer());
	if (isplayer && oldTeam)
	{	
		TPlayerTeamIdMap::iterator pit=m_playerteams.find(oldTeam);
		assert(pit!=m_playerteams.end());
		stl::find_and_erase(pit->second, id);
	}

	if (teamId)
	{
		m_entityteams.insert(TEntityTeamIdMap::value_type(id, teamId));

		if (isplayer)
		{
			TPlayerTeamIdMap::iterator pit=m_playerteams.find(teamId);
			assert(pit!=m_playerteams.end());
			pit->second.push_back(id);

			UpdateObjectivesForPlayer(GetChannelId(id), teamId);
		}
	}

	IEntity * entity = gEnv->pEntitySystem->GetEntity(id);
#if !defined(_RELEASE)
	if (entity)
	{
		CryLogAlways("[RS] SERVER: Entity '%s' changing to team number: %d", entity->GetName(), teamId);
	}
#endif

	// Doing nothing so commented out for now
//	if(IActor *pClient = g_pGame->GetIGameFramework()->GetClientActor())
//	{
//		if(GetTeam(pClient->GetEntityId()) == teamId)
//		{
//			if(id == pClient->GetGameObject()->GetWorldQuery()->GetLookAtEntityId())
//			{
//				//HUD_Interactive
//			}
//		}
//	}

	if(isplayer)
	{
#ifndef OLD_VOICE_SYSTEM_DEPRECATED
		ReconfigureVoiceGroups(id,oldTeam,teamId);
#endif

		int channelId=GetChannelId(id);

		TChannelTeamIdMap::iterator it=m_channelteams.find(channelId);
		if (it!=m_channelteams.end())
		{
			if (!teamId)
				m_channelteams.erase(it);
			else
				it->second=teamId;
		}
		else if(teamId)
			m_channelteams.insert(TChannelTeamIdMap::value_type(channelId, teamId));
	}

	
	ScriptHandle setTeamHandle(id);
	CallScript(m_serverStateScript, "OnSetTeam", setTeamHandle, teamId);
	
	IScriptTable *pEntityScript = entity->GetScriptTable();
	if (pEntityScript)
	{
		if (pEntityScript->GetValueType("OnSetTeam") == svtFunction)
		{
			m_pScriptSystem->BeginCall(pEntityScript, "OnSetTeam");
			m_pScriptSystem->PushFuncParam(pEntityScript);
			m_pScriptSystem->PushFuncParam(teamId);
			m_pScriptSystem->EndCall();
		}
	}

	if (gEnv->bClient)
	{
		ScriptHandle handle(id);
		CallScript(m_clientStateScript, "OnSetTeam", handle, teamId);
	}
	
	// if this is a spawn group, update it's validity
	if (m_spawnGroups.find(id)!=m_spawnGroups.end())
		CheckSpawnGroupValidity(id);

	GetGameObject()->InvokeRMIWithDependentObject(ClSetTeam(), SetTeamParams(id, teamId), eRMI_ToRemoteClients, id);

	int numListeners = m_teamChangedListeners.size();
	for (int i = 0; i < numListeners; ++ i)
	{
		m_teamChangedListeners[i]->OnChangedTeam(id, oldTeam, teamId);
	}

	if (IEntity *pEntity=m_pEntitySystem->GetEntity(id))
		m_pGameplayRecorder->Event(pEntity, GameplayEvent(eGE_ChangedTeam, 0, (float)teamId));

	IGameObject* pGameObject = m_pGameFramework->GetGameObject(id);

	if(pGameObject)
	{
		STeamChangeInfo teamChangeInfo = {oldTeam, teamId};
		pGameObject->SendEvent(SGameObjectEvent(eCGE_SetTeam, eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, & teamChangeInfo));
	}
}

//------------------------------------------------------------------------
int CGameRules::GetTeam(EntityId entityId) const
{
	TEntityTeamIdMap::const_iterator it = m_entityteams.find(entityId);
	if (it != m_entityteams.end())
		return it->second;

	return 0;
}

//------------------------------------------------------------------------
int CGameRules::GetChannelTeam(int channelId) const
{
	TChannelTeamIdMap::const_iterator it = m_channelteams.find(channelId);
	if (it != m_channelteams.end())
		return it->second;

	return 0;
}

//------------------------------------------------------------------------
void CGameRules::AddObjective(int teamId, const char *objective, int status, EntityId entityId)
{
	TObjectiveMap *pObjectives=GetTeamObjectives(teamId);
	if (!pObjectives)
		m_objectives.insert(TTeamObjectiveMap::value_type(teamId, TObjectiveMap()));

	if (pObjectives=GetTeamObjectives(teamId))
	{
		if (pObjectives->find(CONST_TEMP_STRING(objective))==pObjectives->end())
			pObjectives->insert(TObjectiveMap::value_type(objective, TObjective(status, entityId)));
	}
}

//------------------------------------------------------------------------
void CGameRules::SetObjectiveStatus(int teamId, const char *objective, int status)
{
	if (TObjective *pObjective=GetObjective(teamId, objective))
		pObjective->status=status;

	if (gEnv->bServer)
	{
		GAMERULES_INVOKE_ON_TEAM(teamId, ClSetObjectiveStatus(), SetObjectiveStatusParams(objective, status))
	}
}

//------------------------------------------------------------------------
void CGameRules::SetObjectiveEntity(int teamId, const char *objective, EntityId entityId)
{
	if (TObjective *pObjective=GetObjective(teamId, objective))
		pObjective->entityId=entityId;

	if (gEnv->bServer)
	{
		GAMERULES_INVOKE_ON_TEAM(teamId, ClSetObjectiveEntity(), SetObjectiveEntityParams(objective, entityId))
	}
}

//------------------------------------------------------------------------
void CGameRules::RemoveObjective(int teamId, const char *objective)
{
	if (TObjectiveMap *pObjectives=GetTeamObjectives(teamId))
	{
		TObjectiveMap::iterator it=pObjectives->find(CONST_TEMP_STRING(objective));
		if (it!=pObjectives->end())
			pObjectives->erase(it);
	}
}

//------------------------------------------------------------------------
void CGameRules::ResetObjectives()
{
	m_objectives.clear();
}

//------------------------------------------------------------------------
CGameRules::TObjectiveMap *CGameRules::GetTeamObjectives(int teamId)
{
	TTeamObjectiveMap::iterator it=m_objectives.find(teamId);
	if (it!=m_objectives.end())
		return &it->second;
	return 0;
}

//------------------------------------------------------------------------
CGameRules::TObjective *CGameRules::GetObjective(int teamId, const char *objective)
{
	if (TObjectiveMap *pObjectives=GetTeamObjectives(teamId))
	{
		TObjectiveMap::iterator it=pObjectives->find(CONST_TEMP_STRING(objective));
		if (it!=pObjectives->end())
			return &it->second;
	}
	return 0;
}

//------------------------------------------------------------------------
void CGameRules::UpdateObjectivesForPlayer(int channelId, int teamId)
{
	if (TObjectiveMap *pObjectives=GetTeamObjectives(teamId))
	{
		for (TObjectiveMap::iterator it=pObjectives->begin(); it!=pObjectives->end(); ++it)
		{
			if (it->second.status!=CHUDMissionObjective::DEACTIVATED)
				GetGameObject()->InvokeRMI(ClSetObjective(), SetObjectiveParams(it->first.c_str(), it->second.status, it->second.entityId), eRMI_ToClientChannel, channelId);
		}
	}
}

//------------------------------------------------------------------------
bool CGameRules::IsFrozen(EntityId entityId) const
{
	return false;
}
#if 0
struct compare_spawns
{
	bool operator() (EntityId lhs, EntityId rhs ) const
	{
		int lhsT=g_pGame->GetGameRules()->GetTeam(lhs);
		int rhsT=g_pGame->GetGameRules()->GetTeam(rhs);
		if (lhsT == rhsT)
		{
			EntityId lhsG=g_pGame->GetGameRules()->GetSpawnLocationGroup(lhs);
			EntityId rhsG=g_pGame->GetGameRules()->GetSpawnLocationGroup(rhs);
			if (lhsG==rhsG)
			{
				IEntity *pLhs=gEnv->pEntitySystem->GetEntity(lhs);
				IEntity *pRhs=gEnv->pEntitySystem->GetEntity(rhs);

				return strcmp(pLhs->GetName(), pRhs->GetName())<0;
			}
			return lhsG<rhsG;
		}
		return lhsT<rhsT;
	}
};
#endif

void CGameRules::AddSpawnLocation(EntityId location, bool isBaseSpawn, bool doVisTest)
{
	CRY_ASSERT_TRACE (m_spawningModule, ("No spawning module present while trying to add spawn location '%s'", GetEntityName(location)));
	m_spawningModule->AddSpawnLocation(location, isBaseSpawn, doVisTest);
}

void CGameRules::RemoveSpawnLocation(EntityId id, bool isBaseSpawn)
{
	CRY_ASSERT_TRACE (m_spawningModule, ("No spawning module present while trying to remove spawn location '%s'", GetEntityName(id)));
	m_spawningModule->RemoveSpawnLocation(id, isBaseSpawn);
}

#if 0
//------------------------------------------------------------------------
void CGameRules::AddSpawnLocation(EntityId location, bool isBaseSpawn)
{
	stl::push_back_unique(m_spawnLocations, location);

	if (isBaseSpawn)
	{
		stl::push_back_unique(m_baseSpawnLocations, location);
	}
	else
	{
		stl::push_back_unique(m_nonBaseSpawnLocations, location);
	}

	std::sort(m_spawnLocations.begin(), m_spawnLocations.end(), compare_spawns());
}

//------------------------------------------------------------------------
void CGameRules::RemoveSpawnLocation(EntityId id, bool isBaseSpawn)
{
	stl::find_and_erase(m_spawnLocations, id);

	if (isBaseSpawn)
	{
		stl::find_and_erase(m_baseSpawnLocations, id);
	}
	else
	{
		stl::find_and_erase(m_nonBaseSpawnLocations, id);
	}

	//std::sort(m_spawnLocations.begin(), m_spawnLocations.end(), compare_spawns());
}
#endif

int CGameRules::GetSpawnLocationCount() const
{
	CRY_ASSERT_TRACE (m_spawningModule, ("No spawning module present while trying to count spawn locations"));
	return m_spawningModule->GetSpawnLocationCount();
}

EntityId CGameRules::GetSpawnLocation(int idx) const
{
	CRY_ASSERT_TRACE (m_spawningModule, ("No spawning module present while trying to look up spawn location %d", idx));
	return m_spawningModule->GetNthSpawnLocation(idx);
}

#if 0
//------------------------------------------------------------------------
int CGameRules::GetSpawnLocationCount() const
{
	return (int)m_spawnLocations.size();
}

//------------------------------------------------------------------------
EntityId CGameRules::GetSpawnLocation(int idx) const
{
	if (idx>=0 && idx<(int)m_spawnLocations.size())
		return m_spawnLocations[idx];
	return 0;
}

//------------------------------------------------------------------------
void CGameRules::GetSpawnLocations(TSpawnLocations &locations) const
{
	locations.resize(0);
	locations = m_spawnLocations;
}

//------------------------------------------------------------------------
bool CGameRules::IsSpawnLocationSafe(EntityId playerId, EntityId spawnLocationId, float safeDistance, bool ignoreTeam, float zoffset) const
{
	IEntity *pSpawn=gEnv->pEntitySystem->GetEntity(spawnLocationId);
	if (!pSpawn)
		return false;

	if (safeDistance<=0.01f)
		return true;

	int playerTeamId = GetTeam(playerId);

	SEntityProximityQuery query;
	Vec3	c(pSpawn->GetWorldPos());
	float l(safeDistance*1.5f);
	float safeDistanceSq=safeDistance*safeDistance;

	query.box = AABB(Vec3(c.x-l,c.y-l,c.z-0.15f), Vec3(c.x+l,c.y+l,c.z+2.0f));
	query.nEntityFlags = 0;
	query.pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Player");
	gEnv->pEntitySystem->QueryProximity(query);

	bool result=true;

	if (zoffset<=0.0001f)
	{
		for (int i=0; i<query.nCount; i++)
		{
			EntityId entityId=query.pEntities[i]->GetId();
			if (playerId==entityId) // ignore self
				continue;

			IActor *pActor = (g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entityId));

			if (pActor && (pActor->GetSpectatorMode() != 0)) // ignore spectators
				continue;

			if (playerTeamId && (playerTeamId == GetTeam(entityId))) // ignore team players on team games
			{
				if (pActor && (pActor->GetEntity()->GetWorldPos()-c).len2()<=safeDistanceSq) // only if they are not too close
				{
					result=false;
					break;
				}

				continue;
			}

			result=false;
			break;
		}
	}
	else
		result=TestSpawnLocationWithEnvironment(spawnLocationId, playerId, zoffset, 2.0f);

	return result;
}

//------------------------------------------------------------------------
bool CGameRules::IsSpawnLocationFarEnough(EntityId spawnLocationId, float minDistance, const Vec3 &testPosition) const
{
	if (minDistance<=0.1f)
		return true;

	IEntity *pSpawn=gEnv->pEntitySystem->GetEntity(spawnLocationId);
	if (!pSpawn)
		return false;

	if ((pSpawn->GetWorldPos()-testPosition).len2()<minDistance*minDistance)
		return false;

	return true;
}

//------------------------------------------------------------------------
bool CGameRules::TestSpawnLocationWithEnvironment(EntityId spawnLocationId, EntityId playerId, float offset, float height) const
{
	if (!spawnLocationId)
		return false;

	IEntity *pSpawn=gEnv->pEntitySystem->GetEntity(spawnLocationId);
	if (!pSpawn)
		return false;

	IPhysicalEntity *pPlayerPhysics=0;

	IEntity *pPlayer=gEnv->pEntitySystem->GetEntity(playerId);
	if (pPlayer)
		pPlayerPhysics=pPlayer->GetPhysics();

	static float r = 0.3f;
	primitives::sphere sphere;
	sphere.center = pSpawn->GetWorldPos();
	sphere.r = r;
	sphere.center.z+=offset+r;

	Vec3 end = sphere.center;
	end.z += height-2.0f*r;

	geom_contact *pContact = 0;
	float dst = gEnv->pPhysicalWorld->PrimitiveWorldIntersection(sphere.type, &sphere, end-sphere.center, ent_static|ent_terrain|ent_rigid|ent_sleeping_rigid|ent_living,
		&pContact, 0, (geom_colltype_player<<rwi_colltype_bit)|rwi_stop_at_pierceable, 0, 0, 0, &pPlayerPhysics, pPlayerPhysics?1:0);

	if(dst>0.001f)
		return false;
	else
		return true;
}

//------------------------------------------------------------------------
EntityId CGameRules::GetSpawnLocation(EntityId playerId, bool ignoreTeam, bool includeNeutral, EntityId groupId, float minDistToDeath, const Vec3 &deathPos, float *pZOffset) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	const TSpawnLocations *locations=0;

	if (groupId)
	{
		TSpawnGroupMap::const_iterator it=m_spawnGroups.find(groupId);
		if (it==m_spawnGroups.end())
			return 0;

		locations=&it->second;
	}
	else
		locations=&m_spawnLocations;

	if (locations->empty())
		return 0;

	static TSpawnLocations candidates;
	candidates.resize(0);

	int playerTeamId=GetTeam(playerId);
	for (TSpawnLocations::const_iterator it=locations->begin(); it!=locations->end(); ++it)
	{
		int teamId=GetTeam(*it);

		if ((ignoreTeam || playerTeamId==teamId) || (!teamId && includeNeutral))
			candidates.push_back(*it);
	}

	int n=candidates.size();
	if (!n)
		return 0;

	int s=Random(n);
	int i=s;

	float mdtd=minDistToDeath;
	float zoffset=0.0f;
	float safeDistance=0.82f; // this is 2x the radius of a player collider (capsule/cylinder)

	while (!IsSpawnLocationSafe(playerId, candidates[i], safeDistance, ignoreTeam, zoffset) ||
		!IsSpawnLocationFarEnough(candidates[i], mdtd, deathPos))
	{
		++i;

		if (i==n)
			i=0;

		if (i==s)
		{
			if (mdtd>0.0f && mdtd==minDistToDeath)// if we have a min distance to death point
				mdtd*=0.5f;													// half it and see if it helps
			else if (mdtd>0.0f)										// if that didn't help
				mdtd=0.0f;													// ignore death point
			else if (zoffset==0.0f)								// nothing worked, so we'll have to resort to height offset
				zoffset=2.0f;
			else
				return 0;														// can't do anything else, just don't spawn and wait for the situation to clear up

			s=Random(n);													// select a random starting point again
			i=s;
		}
	}

	if (pZOffset)
		*pZOffset=zoffset;

	return candidates[i];
}

//	TIA spawn behavior - ideal min distance is approximately a 6th of the distance between the 2 furthest apart spawn points
//	Note: Won't always find the 2 furthest apart spawn points but this is only an indication so it doesn't matter
//------------------------------------------------------------------------
float CGameRules::GetMinEnemyDist( ) const
{
	float	maxY = 0.f;
	float	maxX = 0.f;
	float minY = (float)GetISystem()->GetI3DEngine()->GetTerrainSize();
	float minX = minY;

	int count=GetSpawnLocationCount()-1;
	for(; count>=0; --count)
	{
		const IEntity *pEntity( gEnv->pEntitySystem->GetEntity(GetSpawnLocation(count)));
		if(!pEntity)
			continue;
		const Vec3 pos(pEntity->GetWorldPos());
		if(minX > pos.x)
			minX = pos.x;
		if(minY > pos.y)
			minY = pos.y;
		if(maxX < pos.x)
			maxX = pos.x;
		if(maxY < pos.y)
			maxY = pos.y;
	}
	float result=max(maxX-minX, maxY-minY)/6.f;
	//float result=max(maxX-minX, maxY-minY)/3.f;

	CryLog("CGameRules::GetMinEnemyDist() calculated result as %f\n", result);
	return result;
}

#endif

// functions to allow visual debugging to replace normal team mate logic
float CGameRules::GetClosestEnemyDistSqrToPlayer(int playerTeamID, Vec3 pos) const
{
	float closeDistSqr=std::numeric_limits<float>::max();

#if DEBUG_NEW_SPAWNING
	if (g_pGameCVars->g_debug_spawning_visually)
	{
		string enemyTeamClassName = g_pGameCVars->g_debug_spawning_visually_teamB->GetString();
		
		IActorIteratorPtr actorIt = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
		IActor *actor;

		while (actor=actorIt->Next())
		{
			const char *actorClassName = actor->GetEntity()->GetClass()->GetName();
			if (enemyTeamClassName == actorClassName)
			{
				Vec3 worldPos = actor->GetEntity()->GetWorldPos();
				float squareDist = (pos - worldPos).GetLengthSquared();
				
				if (squareDist < closeDistSqr)
				{
					closeDistSqr = squareDist;
				}
			}
		}
	}
	else
#endif // DEBUG_NEW_SPAWNING
	{
		int enemyTeamId(GetEnemyTeamId(playerTeamID));
		closeDistSqr = GetClosestTeamMateDistSqr(enemyTeamId, pos);
	}

	return closeDistSqr;
}

float CGameRules::GetClosestTeamMateDistSqrToPlayer(int playerTeamID, Vec3 pos) const
{
	float closeDistSqr=std::numeric_limits<float>::max();

#if DEBUG_NEW_SPAWNING
	if (g_pGameCVars->g_debug_spawning_visually)
	{
		string friendlyTeamClassName = g_pGameCVars->g_debug_spawning_visually_teamA->GetString();
		
		IActorIteratorPtr actorIt = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
		IActor *actor;

		while (actor=actorIt->Next())
		{
			const char *actorClassName = actor->GetEntity()->GetClass()->GetName();
			if (friendlyTeamClassName == actorClassName)
			{
				Vec3 worldPos = actor->GetEntity()->GetWorldPos();
				float squareDist = (pos - worldPos).GetLengthSquared();
				
				if (squareDist < closeDistSqr)
				{
					closeDistSqr = squareDist;
				}
			}
		}
	}
	else
#endif // DEBUG_NEW_SPAWNING
	{
		closeDistSqr = GetClosestTeamMateDistSqr(playerTeamID, pos);
	}

	return closeDistSqr;
}

//------------------------------------------------------------------------
int CGameRules::GetEnemyTeamId(int myTeamId) const
{
	for(TPlayerTeamIdMap::const_iterator it=m_playerteams.begin(); it!=m_playerteams.end(); ++it)
	{
		if(it->first!=myTeamId)
			return it->first;
	}
	return -1;
}

//------------------------------------------------------------------------
float CGameRules::GetClosestPlayerDistSqr(const EntityId spawnLocationId, const EntityId skipId) const
{
	float	closestDist(std::numeric_limits<float>::max());
	IEntity *pSpawn=gEnv->pEntitySystem->GetEntity(spawnLocationId);
	if (!pSpawn)
		return closestDist;

	const Vec3& pos(pSpawn->GetWorldPos());
	CGameRules::TPlayers players;
	GetPlayers(players);
	if(	players.empty() ||
			players[0]==skipId && players.size()==1)
		return closestDist;
	for(CGameRules::TPlayers::iterator it=players.begin();it!=players.end();++it)
	{
		if(*it == skipId)
			continue;
		const IEntity *pOther = gEnv->pEntitySystem->GetEntity(*it);
		float	squareDist = (pos - pOther->GetWorldPos()).GetLengthSquared();
		if( closestDist < squareDist )
			continue;
		closestDist = squareDist;
	}
	return closestDist;
}

//------------------------------------------------------------------------
float CGameRules::GetClosestTeamMateDistSqr(int teamId, const Vec3& pos, EntityId skipId) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);
	float	closestDist(std::numeric_limits<float>::max());

	int frame = gEnv->pRenderer->GetFrameID();
//	CryLog("CGameRules::GetClosestTeamMateDistSqr() frame=%d; teamId=%d; pos=(%f, %f, %f)", frame, teamId, pos.x, pos.y, pos.z);

	int idx=0;
	EntityId teamMateId;
	while( teamMateId=GetTeamActivePlayer(teamId, idx++) )
	{
		const IEntity *pTeamMate = gEnv->pEntitySystem->GetEntity(teamMateId);	// resolve later, only for debugging
//		CryLog("working on teamMate %s id=%d", pTeamMate->GetName(), teamMateId);
		if(teamMateId == skipId)
		{
//			CryLog("skipping %s as skipID", pTeamMate->GetName());
			continue;
		}
		if(!IsPlayerActivelyPlaying(teamMateId))
		{	
//			CryLog("skipping %s as NOT activelyPlaying team=%d", pTeamMate->GetName(), GetTeam(teamMateId));
			continue;
		}

		Vec3 teamMatePos = pTeamMate->GetWorldPos();
//		const IEntity *pTeamMate = gEnv->pEntitySystem->GetEntity(teamMateId);
		float	squareDist = (pos - teamMatePos).GetLengthSquared();
//		CryLog("teamMate %s is %f sqrdist away at worldPos (%f, %f, %f)", pTeamMate->GetName(), squareDist, teamMatePos.x, teamMatePos.y, teamMatePos.z);
		if( closestDist < squareDist )
			continue;
		closestDist = squareDist;
	}
	return closestDist;
}

//------------------------------------------------------------------------
EntityId CGameRules::GetFirstSpawnLocation(int teamId, EntityId groupId) const
{
	CRY_ASSERT_TRACE (m_spawningModule, ("No spawning module present while trying to get first spawn location for team=%d group='%s'", teamId, GetEntityName(groupId)));

	return m_spawningModule->GetFirstSpawnLocation(teamId);

#if 0
	if (!m_spawnLocations.empty())
	{
		for (TSpawnLocations::const_iterator it=m_spawnLocations.begin(); it!=m_spawnLocations.end(); ++it)
		{
			if (teamId==GetTeam(*it) && (!groupId || groupId==GetSpawnLocationGroup(*it)))
				return *it;
		}
	}

	return 0;
#endif
}

//------------------------------------------------------------------------
void CGameRules::AddSpawnGroup(EntityId groupId)
{
	if (m_spawnGroups.find(groupId)==m_spawnGroups.end())
		m_spawnGroups.insert(TSpawnGroupMap::value_type(groupId, TSpawnLocations()));
}

//------------------------------------------------------------------------
void CGameRules::AddSpawnLocationToSpawnGroup(EntityId groupId, EntityId location)
{
	TSpawnGroupMap::iterator it=m_spawnGroups.find(groupId);
	if (it==m_spawnGroups.end())
		return;

	CryLog("CGameRules::AddSpawnLocationToSpawnGroup() no longer resorting spawns. spawngroups unused at the moment");
	stl::push_back_unique(it->second, location);
#if 0
	std::sort(m_spawnLocations.begin(), m_spawnLocations.end(), compare_spawns()); // need to resort spawn location
#endif
}

//------------------------------------------------------------------------
void CGameRules::RemoveSpawnLocationFromSpawnGroup(EntityId groupId, EntityId location)
{
	TSpawnGroupMap::iterator it=m_spawnGroups.find(groupId);
	if (it==m_spawnGroups.end())
		return;

	CryLog("CGameRules::RemoveSpawnLocationFromSpawnGroup() no longer resorting spawns. spawngroups unused at the moment");
	stl::find_and_erase(it->second, location);
#if 0
	std::sort(m_spawnLocations.begin(), m_spawnLocations.end(), compare_spawns()); // need to resort spawn location
#endif
}

//------------------------------------------------------------------------
void CGameRules::RemoveSpawnGroup(EntityId groupId)
{
	stl::member_find_and_erase(m_spawnGroups, groupId);

	CryLog("CGameRules::RemoveSpawnGroup() no longer resorting spawns. spawngroups unused at the moment");
#if 0
	std::sort(m_spawnLocations.begin(), m_spawnLocations.end(), compare_spawns()); // need to resort spawn location
#endif

	if (gEnv->bServer)
	{
		TTeamIdEntityIdMap::iterator next;
		for (TTeamIdEntityIdMap::iterator dit=m_teamdefaultspawns.begin(); dit!=m_teamdefaultspawns.end(); dit=next)
		{
			next=dit;
			++next;
			if (dit->second==groupId)
				m_teamdefaultspawns.erase(dit);
		}
	}

	CheckSpawnGroupValidity(groupId);
}

//------------------------------------------------------------------------
EntityId CGameRules::GetSpawnLocationGroup(EntityId spawnId) const
{
	for (TSpawnGroupMap::const_iterator it=m_spawnGroups.begin(); it!=m_spawnGroups.end(); ++it)
	{
		TSpawnLocations::const_iterator sit=std::find(it->second.begin(), it->second.end(), spawnId);
		if (sit!=it->second.end())
			return it->first;
	}

	return 0;
}

//------------------------------------------------------------------------
int CGameRules::GetSpawnGroupCount() const
{
	return (int)m_spawnGroups.size();
}

//------------------------------------------------------------------------
EntityId CGameRules::GetSpawnGroup(int idx) const
{
	if (idx>=0 && idx<(int)m_spawnGroups.size())
	{
		TSpawnGroupMap::const_iterator it=m_spawnGroups.begin();
		std::advance(it, idx);
		return it->first;
	}

	return 0;
}

//------------------------------------------------------------------------
void CGameRules::GetSpawnGroups(TSpawnLocations &groups) const
{
	groups.resize(0);
	groups.reserve(m_spawnGroups.size());
	for (TSpawnGroupMap::const_iterator it=m_spawnGroups.begin(); it!=m_spawnGroups.end(); ++it)
		groups.push_back(it->first);
}

//------------------------------------------------------------------------
bool CGameRules::IsSpawnGroup(EntityId id) const
{
	TSpawnGroupMap::const_iterator it=m_spawnGroups.find(id);
	return it!=m_spawnGroups.end();
}
//------------------------------------------------------------------------

bool CGameRules::AllowNullSpawnGroups() const
{
	bool allow = false;
	if (m_script->GetValueType("AllowNullSpawnGroups") == svtBool)
	{
		m_script->GetValue("AllowNullSpawnGroups", allow);
	}

	return allow;
}

//------------------------------------------------------------------------
void CGameRules::RequestSpawnGroup(EntityId spawnGroupId)
{
	CallScript(m_script, "RequestSpawnGroup", ScriptHandle(spawnGroupId));
}

//------------------------------------------------------------------------
void CGameRules::SetPlayerSpawnGroup(EntityId playerId, EntityId spawnGroupId)
{
	CallScript(m_script, "SetPlayerSpawnGroup", ScriptHandle(playerId), ScriptHandle(spawnGroupId));
}

//------------------------------------------------------------------------
EntityId CGameRules::GetPlayerSpawnGroup(IActor *pActor)
{
	if (m_script->GetValueType("GetPlayerSpawnGroup") != svtFunction)
		return 0;

	ScriptHandle ret(0);
	m_pScriptSystem->BeginCall(m_script, "GetPlayerSpawnGroup");
	m_pScriptSystem->PushFuncParam(m_script);
	m_pScriptSystem->PushFuncParam(pActor->GetEntity()->GetScriptTable());
	m_pScriptSystem->EndCall(ret);

	return (EntityId)ret.n;
}

//------------------------------------------------------------------------
void CGameRules::SetTeamDefaultSpawnGroup(int teamId, EntityId spawnGroupId)
{
	TTeamIdEntityIdMap::iterator it=m_teamdefaultspawns.find(teamId);
	
	if (it!=m_teamdefaultspawns.end())
		it->second=spawnGroupId;
	else
		m_teamdefaultspawns.insert(TTeamIdEntityIdMap::value_type(teamId, spawnGroupId));
}

//------------------------------------------------------------------------
EntityId CGameRules::GetTeamDefaultSpawnGroup(int teamId)
{
	TTeamIdEntityIdMap::iterator it=m_teamdefaultspawns.find(teamId);
	if (it!=m_teamdefaultspawns.end())
		return it->second;
	return 0;
}

//------------------------------------------------------------------------
void CGameRules::CheckSpawnGroupValidity(EntityId spawnGroupId)
{
	bool exists=spawnGroupId &&
		(m_spawnGroups.find(spawnGroupId)!=m_spawnGroups.end()) &&
		(gEnv->pEntitySystem->GetEntity(spawnGroupId)!=0);
	bool valid=exists && GetTeam(spawnGroupId)!=0;

	for (std::vector<int>::const_iterator it=m_channelIds.begin(); it!=m_channelIds.end(); ++it)
	{
		IActor *pActor=GetActorByChannelId(*it);
		if (!pActor)
			continue;

		EntityId playerId=pActor->GetEntityId();
		if (GetPlayerSpawnGroup(pActor)==spawnGroupId)
		{
			if (!valid || GetTeam(spawnGroupId)!=GetTeam(playerId))
				CallScript(m_serverScript, "OnSpawnGroupInvalid", ScriptHandle(playerId), ScriptHandle(spawnGroupId));
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::AddHitListener(IHitListener* pHitListener)
{
	stl::push_back_unique(m_hitListeners, pHitListener);
}

//------------------------------------------------------------------------
void CGameRules::RemoveHitListener(IHitListener* pHitListener)
{
	stl::find_and_erase(m_hitListeners, pHitListener);
}

//------------------------------------------------------------------------
void CGameRules::AddGameRulesListener(SGameRulesListener* pRulesListener)
{
	stl::push_back_unique(m_rulesListeners, pRulesListener);
}

//------------------------------------------------------------------------
void CGameRules::RemoveGameRulesListener(SGameRulesListener* pRulesListener)
{
	stl::find_and_erase(m_rulesListeners, pRulesListener);
}

//------------------------------------------------------------------------
int CGameRules::RegisterHitMaterial(const char *materialName)
{
	if (int id=GetHitMaterialId(materialName))
		return id;

	ISurfaceType *pSurfaceType=m_pMaterialManager->GetSurfaceTypeByName(materialName);
	if (pSurfaceType)
	{
		m_hitMaterials.insert(THitMaterialMap::value_type(++m_hitMaterialIdGen, pSurfaceType->GetId()));
		return m_hitMaterialIdGen;
	}
	return 0;
}

//------------------------------------------------------------------------
int CGameRules::GetHitMaterialId(const char *materialName) const
{
	ISurfaceType *pSurfaceType=m_pMaterialManager->GetSurfaceTypeByName(materialName);
	if (!pSurfaceType)
		return 0;

	int id=pSurfaceType->GetId();

	for (THitMaterialMap::const_iterator it=m_hitMaterials.begin(); it!=m_hitMaterials.end(); ++it)
	{
		if (it->second==id)
			return it->first;
	}

	return 0;
}

//------------------------------------------------------------------------
int CGameRules::GetHitMaterialIdFromSurfaceId(int surfaceId) const
{
	for (THitMaterialMap::const_iterator it=m_hitMaterials.begin(); it!=m_hitMaterials.end(); ++it)
	{
		if (it->second==surfaceId)
			return it->first;
	}

	return 0;
}

//------------------------------------------------------------------------
ISurfaceType *CGameRules::GetHitMaterial(int id) const
{
	THitMaterialMap::const_iterator it=m_hitMaterials.find(id);
	if (it==m_hitMaterials.end())
		return 0;

	ISurfaceType *pSurfaceType=m_pMaterialManager->GetSurfaceType(it->second);
	
	return pSurfaceType;
}

//------------------------------------------------------------------------
void CGameRules::ResetHitMaterials()
{
	m_hitMaterials.clear();
	m_hitMaterialIdGen=0;
}

//------------------------------------------------------------------------
int CGameRules::RegisterHitType(const char *type)
{
#ifdef _DEBUG
	bool old = DbgSetAssertOnFailureToFindHitType(false);
#endif

	int id=GetHitTypeId(type);

#ifdef _DEBUG
	DbgSetAssertOnFailureToFindHitType(old);
#endif

	if (id)
		return id;

	m_hitTypes.insert(THitTypeMap::value_type(++m_hitTypeIdGen, type));
	return m_hitTypeIdGen;
}

//------------------------------------------------------------------------
int CGameRules::GetHitTypesCount() const
{
	return m_hitTypeIdGen;
}

//------------------------------------------------------------------------
int CGameRules::GetHitTypeId(const char *type) const
{
	for (THitTypeMap::const_iterator it=m_hitTypes.begin(); it!=m_hitTypes.end(); ++it)
	{
		if (it->second.compareNoCase(type) == 0)
			return it->first;
	}

#ifdef _DEBUG
	CRY_ASSERT_TRACE(!s_dbgAssertOnFailureToFindHitType, ("\"%s\" is not one of the %d registered hit types! Please register it in appropriate lua file or GameRulesMPDamageHandling.cpp", type, m_hitTypes.size()));
#endif

	return 0;
}

//------------------------------------------------------------------------
const char *CGameRules::GetHitType(int id) const
{
	THitTypeMap::const_iterator it=m_hitTypes.find(id);
	if (it==m_hitTypes.end())
		return 0;

	return it->second.c_str();
}

//------------------------------------------------------------------------
const char *CGameRules::GetHitType(int id, const char* defaultValue) const
{
	const char* res = GetHitType( id );
	return res ? res : defaultValue;
}

//------------------------------------------------------------------------
void CGameRules::ResetHitTypes()
{
	m_hitTypes.clear();
	m_hitTypeIdGen=0;
}

//------------------------------------------------------------------------
void CGameRules::SendTextMessage(ETextMessageType type, const char *msg, unsigned int to, int channelId,
																 const char *p0, const char *p1, const char *p2, const char *p3)
{
	if (!gEnv->bServer && (to & (eRMI_ToClientChannel | eRMI_ToOtherClients | eRMI_ToAllClients)))
	{
		GameWarning("CGameRules::SendTextMessage() called on client sending to other clients");
		return;
	}
	GetGameObject()->InvokeRMI(ClTextMessage(), TextMessageParams(type, msg, p0, p1, p2, p3), to, channelId);
}

//------------------------------------------------------------------------
bool CGameRules::CanReceiveChatMessage(EChatMessageType type, EntityId sourceId, EntityId targetId) const
{
	if(sourceId == targetId)
		return true;

	bool sspec=!IsPlayerActivelyPlaying(sourceId);
	bool sdead=IsDead(sourceId);

	bool tspec=!IsPlayerActivelyPlaying(targetId);
	bool tdead=IsDead(targetId);

	if(sdead != tdead)
	{
		//CryLog("Disallowing msg (dead): source %d, target %d, sspec %d, sdead %d, tspec %d, tdead %d", sourceId, targetId, sspec, sdead, tspec, tdead);
		return false;
	}

	if(!(tspec || (sspec==tspec)))
	{
		//CryLog("Disallowing msg (spec): source %d, target %d, sspec %d, sdead %d, tspec %d, tdead %d", sourceId, targetId, sspec, sdead, tspec, tdead);
		return false;
	}

	//CryLog("Allowing msg: source %d, target %d, sspec %d, sdead %d, tspec %d, tdead %d", sourceId, targetId, sspec, sdead, tspec, tdead);
	return true;
}

//------------------------------------------------------------------------
void CGameRules::ChatLog(EChatMessageType type, EntityId sourceId, EntityId targetId, const char *msg)
{
	IEntity * pSource = gEnv->pEntitySystem->GetEntity(sourceId);
	IEntity * pTarget = gEnv->pEntitySystem->GetEntity(targetId);
	const char * sourceName = pSource? pSource->GetName() : "<unknown>";
	const char * targetName = pTarget? pTarget->GetName() : "<unknown>";
	int teamId = GetTeam(sourceId);

	char tempBuffer[64];

	switch (type)
	{
	case eChatToTeam:
		if (teamId)
		{
			targetName = tempBuffer;
			sprintf(tempBuffer, "Team %s", GetTeamName(teamId));
		}
		else
		{
	case eChatToAll:
			targetName = "ALL";
		}
		break;
	}

	CryLogAlways("CHAT %s to %s:", sourceName, targetName);
	CryLogAlways("   %s", msg);
}

//------------------------------------------------------------------------
void CGameRules::SendChatMessage(EChatMessageType type, EntityId sourceId, EntityId targetId, const char *msg)
{
	ChatMessageParams params(type, sourceId, targetId, msg, (type == eChatToTeam)?true:false);

	ChatLog(type, sourceId, targetId, msg);

	if (gEnv->bServer)
	{
		switch(type)
		{
		case eChatToTarget:
			{
				if (CanReceiveChatMessage(type, sourceId, targetId))
					GetGameObject()->InvokeRMIWithDependentObject(ClChatMessage(), params, eRMI_ToClientChannel, targetId, GetChannelId(targetId));
			}
			break;
		case eChatToAll:
			{
				std::vector<int>::const_iterator begin=m_channelIds.begin();
				std::vector<int>::const_iterator end=m_channelIds.end();

				for (std::vector<int>::const_iterator it=begin; it!=end; ++it)
				{
					if (IActor *pActor=GetActorByChannelId(*it))
					{
						if (CanReceiveChatMessage(type, sourceId, pActor->GetEntityId()) && IsPlayerInGame(pActor->GetEntityId()))
							GetGameObject()->InvokeRMIWithDependentObject(ClChatMessage(), params, eRMI_ToClientChannel, pActor->GetEntityId(), *it);
					}
				}
			}
			break;
		case eChatToTeam:
			{
				int teamId = GetTeam(sourceId);
				if (teamId)
				{
					TPlayerTeamIdMap::const_iterator tit=m_playerteams.find(teamId);
					if (tit!=m_playerteams.end())
					{
						TPlayers::const_iterator begin=tit->second.begin();
						TPlayers::const_iterator end=tit->second.end();

						for (TPlayers::const_iterator it=begin; it!=end; ++it)
						{
							if (CanReceiveChatMessage(type, sourceId, *it))
								GetGameObject()->InvokeRMIWithDependentObject(ClChatMessage(), params, eRMI_ToClientChannel, *it, GetChannelId(*it));
						}
					}
				}
			}
			break;
		}
	}
	else
		GetGameObject()->InvokeRMI(SvRequestChatMessage(), params, eRMI_ToServer);
}

//------------------------------------------------------------------------
void CGameRules::ForbiddenAreaWarning(bool active, int timer, EntityId targetId)
{
	if (active)
	{
		CHUD::CallEvent(SHUDEvent(eHUDEvent_LeavingBattleArea));
	}
	else
	{
		CHUD::CallEvent(SHUDEvent(eHUDEvent_ReturningToBattleArea));
	}
}

//------------------------------------------------------------------------
void CGameRules::ResetGameTime()
{
	m_gameStartedTime = m_cachedServerTime;

	if (gEnv->bServer)
	{
		GetGameObject()->InvokeRMI(ClSetGameStartedTime(), SetGameTimeParams(m_gameStartedTime), eRMI_ToRemoteClients);
	}
}

//------------------------------------------------------------------------
void CGameRules::SetRemainingGameTime(float seconds)
{
	if (!gEnv->bHostMigrating || !gEnv->bServer)
	{
		// This function should only ever be called as part of the host migration
		// process, when the new server is being created
		GameWarning("CGameRules::SetRemainingGameTime() should only be called by the new server during host migration");
		return;
	}

	float currentTime = m_cachedServerTime.GetSeconds();
	float timeLimit = MAX(m_timeLimit * 60.0f, currentTime + seconds);
	
	// Set the start of the game at the appropriate point back in time...
	m_gameStartedTime.SetSeconds(currentTime + seconds - timeLimit);
}
//------------------------------------------------------------------------
float CGameRules::GetRemainingGameTimeNotZeroCapped() const
{
	if (m_gameEndedTime != 0.0f) // Game has ended, time stops
		return ((m_gameStartedTime - m_gameEndedTime).GetSeconds() + (m_timeLimit*60.f));
	else
		return ((m_gameStartedTime - m_cachedServerTime).GetSeconds() + (m_timeLimit*60.f));
}
//------------------------------------------------------------------------
float CGameRules::GetRemainingGameTime() const
{
	return MAX(0.f, GetRemainingGameTimeNotZeroCapped());
}

//------------------------------------------------------------------------
float CGameRules::GetCurrentGameTime() const
{
	if (m_gameEndedTime != 0.0f) // Game has ended, time stops
		return MAX(0.f, (m_gameEndedTime - m_gameStartedTime).GetSeconds());
	else
		return MAX(0.f, (m_cachedServerTime - m_gameStartedTime).GetSeconds());
}

//------------------------------------------------------------------------
bool CGameRules::IsTimeLimited() const
{
	return m_timeLimit>0.0f;
}

//------------------------------------------------------------------------
void CGameRules::ResetRoundTime()
{
	if (!gEnv->bServer)
	{
		GameWarning("CGameRules::ResetRoundTime() called on client");
		return;
	}

	m_roundEndTime.SetSeconds(0.0f);

	float roundTime=g_pGameCVars->g_roundtime;
	if (roundTime>0.0f)
		m_roundEndTime.SetSeconds(m_cachedServerTime.GetSeconds()+roundTime*60.0f);

	GetGameObject()->InvokeRMI(ClSetRoundTime(), SetGameTimeParams(m_roundEndTime), eRMI_ToRemoteClients);
}

//------------------------------------------------------------------------
float CGameRules::GetRemainingRoundTime() const
{
	return MAX(0, (m_roundEndTime-m_cachedServerTime).GetSeconds());
}

//------------------------------------------------------------------------
bool CGameRules::IsRoundTimeLimited() const
{
	return m_roundEndTime.GetSeconds()>0.0f;
}

//------------------------------------------------------------------------
void CGameRules::ResetPreRoundTime()
{
	if (!gEnv->bServer)
	{
		GameWarning("CGameRules::ResetPreRoundTime() called on client");
		return;
	}

	m_preRoundEndTime.SetSeconds(0.0f);

	int preRoundTime=g_pGameCVars->g_preroundtime;
	if (preRoundTime>0)
		m_preRoundEndTime.SetSeconds(m_cachedServerTime.GetSeconds()+preRoundTime);

	GetGameObject()->InvokeRMI(ClSetPreRoundTime(), SetGameTimeParams(m_preRoundEndTime), eRMI_ToRemoteClients);
}

//------------------------------------------------------------------------
float CGameRules::GetRemainingPreRoundTime() const
{
	return MAX(0, (m_preRoundEndTime-m_cachedServerTime).GetSeconds());
}

//------------------------------------------------------------------------
void CGameRules::ResetReviveCycleTime()
{
	if (!gEnv->bServer)
	{
		GameWarning("CGameRules::ResetReviveCycleTime() called on client");
		return;
	}

	m_reviveCycleEndTime.SetSeconds(0.0f);

	if (g_pGameCVars->g_revivetime < 5)
	{
		g_pGameCVars->g_revivetime = 5;
	}

	m_reviveCycleEndTime = m_cachedServerTime + float(g_pGameCVars->g_revivetime);

	GetGameObject()->InvokeRMI(ClSetReviveCycleTime(), SetGameTimeParams(m_reviveCycleEndTime), eRMI_ToRemoteClients);
}

//------------------------------------------------------------------------
float CGameRules::GetRemainingReviveCycleTime() const
{
	return MAX(0, (m_reviveCycleEndTime-m_cachedServerTime).GetSeconds());
}


//------------------------------------------------------------------------
void CGameRules::ResetGameStartTimer(float time)
{
	if (!gEnv->bServer)
	{
		GameWarning("CGameRules::ResetGameStartTimer() called on client");
		return;
	}

	CCCPOINT_IF(time < 0.f, GameRules_SvGameStartCountdownAbort);
	CCCPOINT_IF(time >= 0.f, GameRules_SvGameStartCountdownBegin);

	m_gameStartTime = m_cachedServerTime + time;

	GetGameObject()->InvokeRMI(ClSetGameStartTimer(), SetGameTimeParams(m_gameStartTime), eRMI_ToRemoteClients);
}

//------------------------------------------------------------------------
float CGameRules::GetRemainingStartTimer() const
{
	return (m_gameStartTime - m_cachedServerTime).GetSeconds();
}

//------------------------------------------------------------------------
float CGameRules::GetServerTime() const
{
	return m_cachedServerTime.GetMilliSeconds();
}

//------------------------------------------------------------------------
bool CGameRules::OnCollision(const SGameCollision& event)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	CWeaponSystem* pWeaponSystem = g_pGame->GetWeaponSystem();
	CProjectile* pProjectile = pWeaponSystem && event.pSrcEntity ? pWeaponSystem->GetProjectile(event.pSrcEntity->GetId()) : 0;
	if (pProjectile && !pProjectile->IsAlive())
		return false;

	if (gEnv->bMultiplayer)
	{
		if (event.pSrcEntity && event.pTrgEntity)
		{
			// Check if the source entity is a projectile
			if (pProjectile)
			{
				EntityId targetId = event.pTrgEntity->GetId();
				// Prevent friendly fire blood effects
				if (g_pGameCVars->g_friendlyfireratio == 0 && GetTeamCount() > 1)
				{
					EntityId shooterId = pProjectile->GetOwnerId();
					int shooterTeamId = GetTeam(shooterId);
					int targetTeamId = GetTeam(targetId);

					if (shooterTeamId && (shooterTeamId == targetTeamId))
					{
						// If the players have a team and their team is the same then this
						// is friendly fire and we don't want the material effects (i.e. blood)
						// to be applied. So return false.
						return false;
					}
				}
				if (targetId == m_pGameFramework->GetClientActorId())
				{
					// Record blood effects on self for killcam replay purposes. These blood effects
					// are suppressed in CryAction so that they can be replaced by post-processor effects.
					CRecordingSystem *pRecordingSystem = g_pGame->GetRecordingSystem();
					if (pRecordingSystem)
					{
						char effectName[] = "bullet.hit_flesh.a";
						// Randomly change the a into either a, b or c to get a different blood effect each time
						effectName[sizeof(effectName)-2] = 'a' + Random(3);
						IParticleEffect *pParticle = gEnv->pParticleManager->FindEffect(effectName);
						if (pParticle)
						{
							SRecording_SpawnCustomParticle customParticle;
							customParticle.location = IParticleEffect::ParticleLoc(event.pCollision->pt, event.pCollision->n);
							customParticle.pParticleEffect = pParticle;
							pRecordingSystem->AddPacket(customParticle);
						}
					}
				}
			}
		}
	}

	// currently this function only calls server functions
	// prevent unnecessary script callbacks on the client
	if (!gEnv->bServer || IsDemoPlayback())
		return true; 

	if (!m_damageHandlingModule && !m_onCollisionFunc)
		return true;

	if(!event.pCollision)
		return true;

	// filter out self-collisions
	if (event.pSrcEntity == event.pTrgEntity)
		return true;

	// collisions involving partId<-1 are to be ignored by game's damage calculations
	// usually created articially to make stuff break. See CMelee::Impulse
	if (event.pCollision->partid[0]<-1||event.pCollision->partid[1]<-1)
		return true;

	//Prevent squad-mates being hit by bullets/collision damage from object held by the player
	if(!gEnv->bMultiplayer)
	{
		IEntity *pTarget = event.pCollision->iForeignData[1]==PHYS_FOREIGN_ID_ENTITY ? (IEntity*)event.pCollision->pForeignData[1]:0;
		if(pTarget)
		{
			if(pTarget->GetId()==m_ignoreEntityNextCollision)
			{
				m_ignoreEntityNextCollision = 0;
				return false;
			}
		}
	}

	// collisions with very low resulting impulse are ignored
	if (event.pCollision->normImpulse<=0.001f)
		return true;

	static IEntityClass* s_pBasicEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("BasicEntity");
	static IEntityClass* s_pDefaultClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Default");
	bool srcClassFilter = false;
	bool trgClassFilter = false;

	IEntityClass* pSrcClass = 0;
	if (event.pSrcEntity)
	{
		pSrcClass = event.pSrcEntity->GetClass();
		// filter out any projectile collisions
		if (g_pGame->GetWeaponSystem()->GetProjectile(event.pSrcEntity->GetId()))
			return true;
		srcClassFilter = (pSrcClass == s_pBasicEntityClass || pSrcClass == s_pDefaultClass);
		if (srcClassFilter && !event.pTrgEntity)
			return true;
	}
	IEntityClass* pTrgClass = 0;
	if (event.pTrgEntity)
	{
		// filter out any projectile collisions
		if (g_pGame->GetWeaponSystem()->GetProjectile(event.pTrgEntity->GetId()))
			return true;
		pTrgClass = event.pTrgEntity->GetClass();
		trgClassFilter = (pTrgClass == s_pBasicEntityClass || pTrgClass == s_pDefaultClass);
		if (trgClassFilter && !event.pSrcEntity)
			return true;
	}

	if (srcClassFilter && trgClassFilter)
		return true;

	if (event.pCollision->idmat[0] != s_invulnID && event.pSrcEntity && event.pSrcEntity->GetScriptTable())
	{
		SCollisionHitInfo colHitInfo;
		PrepCollision(0, 1, event, event.pTrgEntity, colHitInfo);

		if (m_damageHandlingModule)
		{
			m_damageHandlingModule->SvOnCollision(event.pSrcEntity, colHitInfo);
		}
		else
		{
			PrepCollisionForScript(colHitInfo, m_collisionTable);
			Script::CallMethod(m_script, m_onCollisionFunc, event.pSrcEntity->GetScriptTable(), m_collisionTable);
		}
	}
	if (event.pCollision->idmat[1] != s_invulnID && event.pTrgEntity && event.pTrgEntity->GetScriptTable())
	{
		SCollisionHitInfo colHitInfo;
		PrepCollision(1, 0, event, event.pSrcEntity, colHitInfo);

		if (m_damageHandlingModule)
		{
			m_damageHandlingModule->SvOnCollision(event.pTrgEntity, colHitInfo);
		}
		else
		{
			PrepCollisionForScript(colHitInfo, m_collisionTable);
			Script::CallMethod(m_script, m_onCollisionFunc, event.pTrgEntity->GetScriptTable(), m_collisionTable);
		}
	}

	return true;
}

//------------------------------------------------------------------------
void CGameRules::ClearAllMigratingPlayers(void)
{
	for (uint32 index = 0; index < m_migratingPlayerMaxCount; ++index)
	{
		m_pMigratingPlayerInfo[index].Reset();
	}
}

//------------------------------------------------------------------------
void CGameRules::SetChannelForMigratingPlayer(const char* name, uint16 channelID)
{
	CryLog("CGameRules::SetChannelForMigratingPlayer, channel=%i, name=%s", channelID, name);
	for (uint32 index = 0; index < m_migratingPlayerMaxCount; ++index)
	{
		if (m_pMigratingPlayerInfo[index].InUse() && stricmp(m_pMigratingPlayerInfo[index].m_originalName, name) == 0)
		{
			m_pMigratingPlayerInfo[index].SetChannelID(channelID);
			break;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::StoreMigratingPlayer(IActor* pActor)
{
	if (pActor == NULL)
	{
		GameWarning("Invalid data for migrating player");
		return;
	}
	
	IEntity* pEntity = pActor->GetEntity();
	EntityId id = pEntity->GetId();
	bool registered = false;

	for (uint32 index = 0; index < m_migratingPlayerMaxCount; ++index)
	{
		if (!m_pMigratingPlayerInfo[index].InUse())
		{
			m_pMigratingPlayerInfo[index].SetData(pEntity->GetName(), id, GetTeam(id), pEntity->GetWorldPos(), pEntity->GetWorldAngles(), pActor->GetHealth());
			registered = true;
			break;
		}
	}

	pEntity->Hide(true);		// Hide the player, they will be unhidden when they rejoin

	if (!registered)
	{
		GameWarning("Too many migrating players!");
	}
}

//------------------------------------------------------------------------

int CGameRules::GetMigratingPlayerIndex(TNetChannelID channelID)
{
	int migratingPlayerIndex = -1;

	for (uint32 index = 0; index < m_migratingPlayerMaxCount; ++index)
	{
		if (m_pMigratingPlayerInfo[index].InUse() && m_pMigratingPlayerInfo[index].m_channelID == channelID)
		{
			migratingPlayerIndex = index;
			break;
		}
	}

	return migratingPlayerIndex;
}

//------------------------------------------------------------------------
void CGameRules::RegisterConsoleCommands(IConsole *pConsole)
{
	// todo: move to power struggle implementation when there is one
	//REGISTER_COMMAND("buy",			"if (g_gameRules and g_gameRules.Buy) then g_gameRules:Buy(%1); end",VF_NULL,"");
	//REGISTER_COMMAND("buyammo", "if (g_gameRules and g_gameRules.BuyAmmo) then g_gameRules:BuyAmmo(%%); end",VF_NULL,"");

	REGISTER_COMMAND("resetloadout", "if (g_gameRules and g_gameRules.ResetLoadout) then g_gameRules:ResetLoadout(); end",VF_INVISIBLE,"");
	REGISTER_COMMAND("additemtoloadout", "if (g_gameRules and g_gameRules.AddItemToLoadout) then g_gameRules:AddItemToLoadout(%1); end",VF_INVISIBLE,"");
	REGISTER_COMMAND("addperktoloadout", "if (g_gameRules and g_gameRules.AddPerkToLoadout) then g_gameRules:AddPerkToLoadout(%1); end",VF_INVISIBLE,"");
	REGISTER_COMMAND("addammotoloadout", "if (g_gameRules and g_gameRules.AddAmmoToLoadout) then g_gameRules:AddAmmoToLoadout(%%); end",VF_INVISIBLE,"");

	REGISTER_COMMAND("g_debug_spawns", CmdDebugSpawns,VF_NULL,"");
	REGISTER_COMMAND("g_debug_teams", CmdDebugTeams,VF_NULL,"");
	REGISTER_COMMAND("g_debug_objectives", CmdDebugObjectives,VF_NULL,"");

	REGISTER_COMMAND("g_giveScore", CmdGiveScore, VF_NULL, "");
}

//------------------------------------------------------------------------
void CGameRules::UnregisterConsoleCommands(IConsole *pConsole)
{
	//pConsole->RemoveCommand("buy");
	//pConsole->RemoveCommand("buyammo");

	pConsole->RemoveCommand("resetloadout");
	pConsole->RemoveCommand("additemtoloadout");
	pConsole->RemoveCommand("addammotoloadout");

	pConsole->RemoveCommand("g_debug_spawns");
	pConsole->RemoveCommand("g_debug_teams");
	pConsole->RemoveCommand("g_debug_objectives");

	pConsole->RemoveCommand("g_giveScore");
}

//------------------------------------------------------------------------
void CGameRules::RegisterConsoleVars(IConsole *pConsole)
{

}

//------------------------------------------------------------------------
void CGameRules::UnregisterConsoleVars(IConsole *pConsole)
{

}

//------------------------------------------------------------------------
void CGameRules::CmdDebugSpawns(IConsoleCmdArgs *pArgs)
{
	CGameRules *pGameRules=g_pGame->GetGameRules();
	if (!pGameRules->m_spawnGroups.empty())
	{
		CryLogAlways("// Spawn Groups //");
		for (TSpawnGroupMap::const_iterator sit=pGameRules->m_spawnGroups.begin(); sit!=pGameRules->m_spawnGroups.end(); ++sit)
		{
			IEntity *pEntity=gEnv->pEntitySystem->GetEntity(sit->first);
			int groupTeamId=pGameRules->GetTeam(pEntity->GetId());
			const char *Default="$5*DEFAULT*";
			CryLogAlways("Spawn Group: %s  (eid: %d %08x  team: %d) %s", pEntity->GetName(), pEntity->GetId(), pEntity->GetId(), groupTeamId, 
				(sit->first==pGameRules->GetTeamDefaultSpawnGroup(groupTeamId))?Default:"");

			for (TSpawnLocations::const_iterator lit=sit->second.begin(); lit!=sit->second.end(); ++lit)
			{
				int spawnTeamId=pGameRules->GetTeam(pEntity->GetId());
				IEntity *pEntity=gEnv->pEntitySystem->GetEntity(*lit);
				CryLogAlways("    -> Spawn Location: %s  (eid: %d %08x  team: %d)", pEntity->GetName(), pEntity->GetId(), pEntity->GetId(), spawnTeamId);
			}
		}
	}

#if 0
	CryLogAlways("// Spawn Locations //");
	for (TSpawnLocations::const_iterator lit=pGameRules->m_spawnLocations.begin(); lit!=pGameRules->m_spawnLocations.end(); ++lit)
	{
		IEntity *pEntity=gEnv->pEntitySystem->GetEntity(*lit);
		Vec3 pos=pEntity?pEntity->GetWorldPos():ZERO;
		CryLogAlways("Spawn Location: %s  (eid: %d %08x  team: %d) %.2f,%.2f,%.2f", pEntity->GetName(), pEntity->GetId(), pEntity->GetId(), pGameRules->GetTeam(pEntity->GetId()), pos.x, pos.y, pos.z);
	}
#endif
}

//------------------------------------------------------------------------
void CGameRules::CmdDebugTeams(IConsoleCmdArgs *pArgs)
{
	CGameRules *pGameRules=g_pGame->GetGameRules();
	if (!pGameRules->m_entityteams.empty())
	{
		CryLogAlways("// Teams //");
		for (TTeamIdMap::const_iterator tit=pGameRules->m_teams.begin(); tit!=pGameRules->m_teams.end(); ++tit)
		{
			CryLogAlways("Team: %s  (id: %d)", tit->first.c_str(), tit->second);
			for (TEntityTeamIdMap::const_iterator eit=pGameRules->m_entityteams.begin(); eit!=pGameRules->m_entityteams.end(); ++eit)
			{
				if (eit->second==tit->second)
				{
					IEntity *pEntity=gEnv->pEntitySystem->GetEntity(eit->first);
					CryLogAlways("    -> Entity: %s  class: %s  (eid: %d %08x)", pEntity?pEntity->GetName():"<null>", pEntity?pEntity->GetClass()->GetName():"<null>", eit->first, eit->first);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::CmdDebugObjectives(IConsoleCmdArgs *pArgs)
{
	const char *status[CHUDMissionObjective::LAST+1];
	status[0]="$5invalid$o";
	status[CHUDMissionObjective::LAST]="$5invalid$o";

	status[CHUDMissionObjective::ACTIVATED]="$3active$o";
	status[CHUDMissionObjective::DEACTIVATED]="$9inactive$o";
	status[CHUDMissionObjective::COMPLETED]="$2complete$o";
	status[CHUDMissionObjective::FAILED]="$4failed$o";

	CGameRules *pGameRules=g_pGame->GetGameRules();
	if (!pGameRules->m_objectives.empty())
	{
		CryLogAlways("// Teams //");
		for (TTeamIdMap::const_iterator tit=pGameRules->m_teams.begin(); tit!=pGameRules->m_teams.end(); ++tit)
		{
			if (TObjectiveMap *pObjectives=pGameRules->GetTeamObjectives(tit->second))
			{
				for (TObjectiveMap::const_iterator it=pObjectives->begin(); it!=pObjectives->end(); ++it)
					CryLogAlways("  -> Objective: %s  teamId: %d  status: %s  (eid: %d %08x)", it->first.c_str(), tit->second,
						status[CLAMP(it->second.status, 0, CHUDMissionObjective::LAST)], it->second.entityId, it->second.entityId);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::CmdGiveScore(IConsoleCmdArgs *pArgs)
{
	if (!gEnv->bServer)
	{
		CryLogAlways ("Server only command, sorry!");
	}
	else if (pArgs->GetArgCount() == 4)
	{
		const char * findWithName = pArgs->GetArg(1);
		bool getLocalPlayer = (findWithName[0] == '.' && findWithName[1] == '\0');
		IEntity * theEntity = getLocalPlayer ? gEnv->pEntitySystem->GetEntity(g_pGame->GetIGameFramework()->GetClientActorId()) : gEnv->pEntitySystem->FindEntityByName(findWithName);

		if (theEntity)
		{
			const int points = atoi(pArgs->GetArg(2));
			CRY_ASSERT_MESSAGE( points < SGameRulesScoreInfo::SCORE_MAX && points > SGameRulesScoreInfo::SCORE_MIN, string().Format("Adding score for player which is out of net-serialize bounds (%d is not within [%d .. %d])", points, SGameRulesScoreInfo::SCORE_MIN, SGameRulesScoreInfo::SCORE_MAX) );
			SGameRulesScoreInfo si( (EGameRulesScoreType)atoi(pArgs->GetArg(3)), static_cast<TGameRulesScoreInt>(points) );
			g_pGame->GetGameRules()->IncreasePoints(theEntity->GetId(), si);
		}
		else
		{
			CryLogAlways ("Found no entity by the name of '%s'", findWithName);
		}
	}
	else
	{
		CryLogAlways ("Command syntax: %s <entityName> <score> <score-type 0..9>", pArgs->GetArg(0));
	}
}

//------------------------------------------------------------------------
#if !defined(_RELEASE)

#define MESSAGE_FORMAT_STRING  "Damage being done by '%s' to '%s' (with weapon '%s' of class %s, hit type %d '%s') dir={%.2f %.2f %.2f} %s [%s]"
#define MESSAGE_PARAMETERS     GetEntityName(shooter), GetEntityName(target), GetEntityName(weapon), weaponEntity ? weaponEntity->GetClass()->GetName() : "N/A", hitType, GetHitType(hitType), dir.x, dir.y, dir.z
#define DO_WARNING(a)          CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, MESSAGE_FORMAT_STRING, MESSAGE_PARAMETERS, a, funcName)
#define DO_ASSERT(a)           CRY_ASSERT_TRACE(false, (MESSAGE_FORMAT_STRING, MESSAGE_PARAMETERS, a, funcName))

void CGameRules::SanityCheckHitData(const Vec3 & dir, EntityId shooter, EntityId target, EntityId weapon, uint16 hitType, const char * funcName)
{
	if (dir.x == 0.f && dir.y == 1.f && dir.z == 0.f)
	{
		IEntity * weaponEntity = gEnv->pEntitySystem->GetEntity(weapon);
		DO_WARNING("appears to have uninitialized direction vector!");
	}

	if (hitType == 0)
	{
		IEntity * weaponEntity = gEnv->pEntitySystem->GetEntity(weapon);
		DO_WARNING("has invalid hit type ID");
		DO_ASSERT("has invalid hit type ID");
	}
}

#undef MESSAGE_FORMAT_STRING
#undef MESSAGE_PARAMETERS
#undef DO_WARNING
#undef DO_ASSERT

#endif

//------------------------------------------------------------------------
void CGameRules::CreateScriptHitInfo(SmartScriptTable &scriptHitInfo, const HitInfo &hitInfo)
{
	CScriptSetGetChain hit(scriptHitInfo);
	{
		hit.SetValue("normal", hitInfo.normal);
		hit.SetValue("pos", hitInfo.pos);
		hit.SetValue("dir", hitInfo.dir);
		hit.SetValue("partId", hitInfo.partId);
		hit.SetValue("backface", hitInfo.normal.Dot(hitInfo.dir)>=0.0f);

		hit.SetValue("targetId", ScriptHandle(hitInfo.targetId));		
		hit.SetValue("shooterId", ScriptHandle(hitInfo.shooterId));
		hit.SetValue("weaponId", ScriptHandle(hitInfo.weaponId));
		hit.SetValue("projectileId", ScriptHandle(hitInfo.projectileId));

		IEntity *pTarget=m_pEntitySystem->GetEntity(hitInfo.targetId);
		IEntity *pShooter=m_pEntitySystem->GetEntity(hitInfo.shooterId);
		IEntity *pWeapon=m_pEntitySystem->GetEntity(hitInfo.weaponId);
		IEntity *pProjectile=m_pEntitySystem->GetEntity(hitInfo.projectileId);

		hit.SetValue("projectile", pProjectile?pProjectile->GetScriptTable():(IScriptTable *)0);
		hit.SetValue("target", pTarget?pTarget->GetScriptTable():(IScriptTable *)0);
		hit.SetValue("shooter", pShooter?pShooter->GetScriptTable():(IScriptTable *)0);
		hit.SetValue("weapon", pWeapon?pWeapon->GetScriptTable():(IScriptTable *)0);
		//hit.SetValue("projectile_class", pProjectile?pProjectile->GetClass()->GetName():"");

		hit.SetValue("materialId", hitInfo.material);
		
		ISurfaceType *pSurfaceType=GetHitMaterial(hitInfo.material);
		if (pSurfaceType)
		{
			hit.SetValue("material", pSurfaceType->GetName());
			hit.SetValue("material_type", pSurfaceType->GetType());
		}
		else
		{
			hit.SetToNull("material");
			hit.SetToNull("material_type");
		}

		hit.SetValue("damage", hitInfo.damage);
		hit.SetValue("radius", hitInfo.radius);
		hit.SetValue("knocksDownLeg", hitInfo.knocksDownLeg);
		
		hit.SetValue("typeId", hitInfo.type);
		const char *type=GetHitType(hitInfo.type);
    hit.SetValue("type", type ? type : "");
		hit.SetValue("remote", hitInfo.remote);
		hit.SetValue("bulletType", hitInfo.bulletType);
	
		// Check for hit assistance
		float assist=0.0f;
		hit.SetValue("assistance", assist);		

		hit.SetValue("projectileClassId", hitInfo.projectileClassId);
	}
}

//------------------------------------------------------------------------
void CGameRules::CreateScriptExplosionInfo(SmartScriptTable &scriptExplosionInfo, const ExplosionInfo &explosionInfo)
{
	CScriptSetGetChain explosion(scriptExplosionInfo);
	{
		explosion.SetValue("pos", explosionInfo.pos);
		explosion.SetValue("dir", explosionInfo.dir);

		explosion.SetValue("shooterId", ScriptHandle(explosionInfo.shooterId));
		explosion.SetValue("weaponId", ScriptHandle(explosionInfo.weaponId));    
		IEntity *pShooter=m_pEntitySystem->GetEntity(explosionInfo.shooterId);
		IEntity *pWeapon=m_pEntitySystem->GetEntity(explosionInfo.weaponId);    
		explosion.SetValue("shooter", pShooter?pShooter->GetScriptTable():(IScriptTable *)0);
		explosion.SetValue("weapon", pWeapon?pWeapon->GetScriptTable():(IScriptTable *)0);
		explosion.SetValue("materialId", 0);
		explosion.SetValue("damage", explosionInfo.damage);
		explosion.SetValue("min_radius", explosionInfo.minRadius);
		explosion.SetValue("radius", explosionInfo.radius);
		explosion.SetValue("pressure", explosionInfo.pressure);
		explosion.SetValue("hole_size", explosionInfo.hole_size);
		explosion.SetValue("effect", explosionInfo.effect_name.c_str());
		explosion.SetValue("effectScale", explosionInfo.effect_scale);
		explosion.SetValue("effectClass", explosionInfo.effect_class.c_str());
		explosion.SetValue("typeId", explosionInfo.type);
		const char *type=GetHitType(explosionInfo.type);
		explosion.SetValue("type", type);
		explosion.SetValue("angle", explosionInfo.angle);
		
		explosion.SetValue("impact", explosionInfo.impact);
		explosion.SetValue("impact_velocity", explosionInfo.impact_velocity);
		explosion.SetValue("impact_normal", explosionInfo.impact_normal);
    explosion.SetValue("impact_targetId", ScriptHandle(explosionInfo.impact_targetId));		

		explosion.SetValue("shakeMinR", explosionInfo.shakeMinR);
		explosion.SetValue("shakeMaxR", explosionInfo.shakeMaxR);
		explosion.SetValue("shakeScale", explosionInfo.shakeScale);
		explosion.SetValue("shakeRnd", explosionInfo.shakeRnd);
	}
  
  SmartScriptTable temp;
  if (scriptExplosionInfo->GetValue("AffectedEntities", temp))
  {
    temp->Clear();
	}
	if (scriptExplosionInfo->GetValue("AffectedEntitiesObstruction", temp))
	{
		temp->Clear();
	}
}

//------------------------------------------------------------------------

void CGameRules::ShowScores(bool show)
{
	CallScript(m_script, "ShowScores", show);
}

//------------------------------------------------------------------------
void CGameRules::UpdateAffectedEntitiesSet(TExplosionAffectedEntities &affectedEnts, const pe_explosion *pExplosion)
{
	if (pExplosion)
	{
		for (int i=0; i<pExplosion->nAffectedEnts; ++i)
		{ 
			if (IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pExplosion->pAffectedEnts[i]))
			{ 
				if (IScriptTable *pEntityTable = pEntity->GetScriptTable())
				{
					IPhysicalEntity* pEnt = pEntity->GetPhysics();
					if (pEnt)
					{
						float affected=gEnv->pPhysicalWorld->IsAffectedByExplosion(pEnt);

						AddOrUpdateAffectedEntity(affectedEnts, pEntity, affected);
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::AddOrUpdateAffectedEntity(TExplosionAffectedEntities &affectedEnts, IEntity* pEntity, float affected)
{
	TExplosionAffectedEntities::iterator it=affectedEnts.find(pEntity);
	if (it!=affectedEnts.end())
	{
		if (it->second<affected)
			it->second=affected;
	}
	else
		affectedEnts.insert(TExplosionAffectedEntities::value_type(pEntity, affected));
}

//------------------------------------------------------------------------
void CGameRules::CommitAffectedEntitiesSet(SmartScriptTable &scriptExplosionInfo, TExplosionAffectedEntities &affectedEnts)
{
	CScriptSetGetChain explosion(scriptExplosionInfo);

	SmartScriptTable affected;
	SmartScriptTable affectedObstruction;

	if (scriptExplosionInfo->GetValue("AffectedEntities", affected) && 
		scriptExplosionInfo->GetValue("AffectedEntitiesObstruction", affectedObstruction))
	{
		if (affectedEnts.empty())
		{
			affected->Clear();
			affectedObstruction->Clear();
		}
		else
		{
			int k=0;      
			for (TExplosionAffectedEntities::const_iterator it=affectedEnts.begin(),end=affectedEnts.end(); it!=end; ++it)
			{
				float obstruction = 1.0f-it->second;
				affected->SetAt(++k, it->first->GetScriptTable());
				affectedObstruction->SetAt(k, obstruction);
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::RemoveFriendlyAffectedEntities(const ExplosionInfo &explosionInfo, TExplosionAffectedEntities &affectedEntities)
{
	if(explosionInfo.friendlyfire == eFriendyFireSelf)
	{
		IEntity *pEntity=m_pEntitySystem->GetEntity(explosionInfo.shooterId);
		if(pEntity)
		{
			TExplosionAffectedEntities::iterator iter = affectedEntities.find(pEntity);
			if(iter != affectedEntities.end())
			{
				affectedEntities.erase(iter);
			}
		}
	}
	else if(explosionInfo.friendlyfire == eFriendyFireTeam)
	{
		CActor* shooterActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(explosionInfo.shooterId));
		if(shooterActor)
		{
			TExplosionAffectedEntities::iterator iter = affectedEntities.begin();
			TExplosionAffectedEntities::iterator end = affectedEntities.end();
			for (; iter != end;)
			{
				if(shooterActor->IsFriendlyEntity(iter->first->GetId()))
				{
					TExplosionAffectedEntities::iterator nextIter = iter;
					nextIter++;
					affectedEntities.erase(iter);
					iter = nextIter;
				}
				else
				{
					 ++iter;
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::PrepCollisionForScript(const SCollisionHitInfo& colHitInfo, SmartScriptTable& collisionTable)
{
	CScriptSetGetChain chain(collisionTable);

	chain.SetValue("normal", colHitInfo.normal);
	chain.SetValue("pos", colHitInfo.pos);

	if (!colHitInfo.dir_null)
	{
		chain.SetValue("dir", colHitInfo.dir);
	}
	else
	{
		chain.SetToNull("dir");
	}

	chain.SetValue("velocity", colHitInfo.velocity);

	chain.SetValue("target_velocity", colHitInfo.target_velocity);
	chain.SetValue("target_mass", colHitInfo.target_mass);

	chain.SetValue("backface", colHitInfo.backface);
	//chain.SetValue("partid", pCollision->partid[src]);
	//chain.SetValue("backface", pCollision->n.Dot(dir) >= 0);
	/*float deltaE = 0;
	if (pCollision->mass[0])
	deltaE += -pCollision->normImpulse*(pCollision->vloc[0]*pCollision->n + pCollision->normImpulse*0.5f/pCollision->mass[0]);
	if (pCollision->mass[1])
	deltaE +=  pCollision->normImpulse*(pCollision->vloc[1]*pCollision->n - pCollision->normImpulse*0.5f/pCollision->mass[1]);
	chain.SetValue("energy_loss", deltaE);*/

	//IEntity *pTarget = gEnv->pEntitySystem->GetEntityFromPhysics(pCollision->pEntity[trg]);

	//chain.SetValue("target_type", (int)pCollision->pEntity[trg]->GetType());

	IEntity *pTarget = m_pEntitySystem->GetEntity(colHitInfo.targetId);
	if (pTarget)
	{
		ScriptHandle sh;
		sh.n = colHitInfo.targetId;
		chain.SetValue("target_id", sh);
		chain.SetValue("target_type", colHitInfo.target_type);

		if (pTarget->GetScriptTable())
		{
			chain.SetValue("target", pTarget->GetScriptTable());
		}
		else
		{
			chain.SetToNull("target");  
		}
	}
	else
	{
		chain.SetToNull("target"); 
		chain.SetToNull("target_id");
	}

	chain.SetValue("materialId",colHitInfo.materialId);
	//chain.SetValue("target_materialId", pCollision->idmat[trg]);

	//ISurfaceTypeManager *pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
}

//------------------------------------------------------------------------
void CGameRules::PrepCollision(int src, int trg, const SGameCollision& event, IEntity* pTarget, SCollisionHitInfo &result)
{
	const EventPhysCollision* pCollision = event.pCollision;
	result.pos = pCollision->pt;
	result.normal = pCollision->n;
	result.dir = Vec3(0.f, 0.f, 0.f);
	if (pCollision->vloc[src].GetLengthSquared() > 1e-6f)
	{
		result.dir = pCollision->vloc[src].GetNormalized();
	}
	result.velocity = pCollision->vloc[src];
	
	pe_status_living sl;
	if (pCollision->pEntity[src]->GetStatus(&sl) && sl.bSquashed)
	{
		result.target_velocity = pCollision->n*(200.0f*(1-src*2));
		result.target_mass = pCollision->mass[trg]>0 ? pCollision->mass[trg] : 10000.0f;
	}
	else
	{
		result.target_velocity = pCollision->vloc[trg];
		result.target_mass = pCollision->mass[trg];
	}
	
	result.backface = (pCollision->n.Dot(result.dir) >= 0);

	if (pTarget)
	{
		result.targetId = pTarget->GetId();

		if (pTarget->GetPhysics())
		{
			result.target_type = pTarget->GetPhysics()->GetType();
		}
	}

	if(pCollision->idmat[trg]==s_barbWireID)
		result.materialId = pCollision->idmat[trg];
	else
		result.materialId = pCollision->idmat[src];
}

//------------------------------------------------------------------------
void CGameRules::Restart()
{
	CCCPOINT(GameRules_Restart);

	ICryPerfHUD* pPerfHud = gEnv->pSystem->GetPerfHUD();
	if (pPerfHud)
	{
		static int roundCount = 0;
		string filename;
		filename.Format("PerfHudStatsRound%d.xml", ++roundCount);
		pPerfHud->SaveStats(filename);
		pPerfHud->ResetWidgets();
	}

	CryWatch3DReset();

	g_pGame->GetPlayerVisTable()->Reset();

	if (gEnv->bServer)
	{
		if (m_stateModule)
		{
			m_stateModule->OnGameRestart();
		}

		CallScript(m_script, "RestartGame", true);
		ResetEntities();	// used to be done by lua Restart handler, but having it here means we don't need a Restart handler in lua (eg when using game state module)
							// lua RestartGame() now only needs to restart its internal state
		
		if (m_pEquipmentLoadout)
			m_pEquipmentLoadout->SvReset();
	}
}

//------------------------------------------------------------------------
void CGameRules::NextLevel()
{
  if (!gEnv->bServer)
    return;

	ILevelRotation *pLevelRotation=m_pGameFramework->GetILevelSystem()->GetLevelRotation();
	if (!pLevelRotation->GetLength())
		Restart();
	else
		pLevelRotation->ChangeLevel();
}

//------------------------------------------------------------------------
void CGameRules::ResetEntities()
{
	g_pGame->GetWeaponSystem()->GetTracerManager().Reset();

	for(int i = 0; i < MAX_CONCURRENT_EXPLOSIONS; i++)
	{
		m_explosions[i].raycastHelper.CancelPendingRays();
		m_explosionValidities[i]	= false;
	}

	while (!m_queuedExplosions.empty())
		m_queuedExplosions.pop();

	while (!m_queuedExplosionsAwaitingLinetests.empty())
		m_queuedExplosionsAwaitingLinetests.pop();

	while (!m_queuedHits.empty())
		m_queuedHits.pop();
	m_processingHit=0;

	// remove voice groups too. They'll be recreated when players are put back on their teams after reset.
#ifndef OLD_VOICE_SYSTEM_DEPRECATED
 	TTeamIdVoiceGroupMap::iterator it = m_teamVoiceGroups.begin();
 	TTeamIdVoiceGroupMap::iterator next;
 	for(; it != m_teamVoiceGroups.end(); it=next)
 	{
 		next = it; ++next;
 
		m_teamVoiceGroups.erase(it);
 	}
#endif

	m_respawns.clear();
	m_entityteams.clear();
	m_teamdefaultspawns.clear();

	for (TPlayerTeamIdMap::iterator tit=m_playerteams.begin(); tit!=m_playerteams.end(); ++tit)
		tit->second.resize(0);

	g_pGame->GetIGameFramework()->Reset(gEnv->bServer);

//	SEntityEvent event(ENTITY_EVENT_START_GAME);
//	gEnv->pEntitySystem->SendEventToAll(event);
}

//------------------------------------------------------------------------
void CGameRules::OnEndGame()
{
	bool isMultiplayer=gEnv->bMultiplayer ;

	m_gameEndedTime = m_cachedServerTime;

#ifndef OLD_VOICE_SYSTEM_DEPRECATED
	if (isMultiplayer && gEnv->bServer)
		m_teamVoiceGroups.clear();
#endif

	if(gEnv->bClient)
	{
		IActionMapManager *pActionMapMan = g_pGame->GetIGameFramework()->GetIActionMapManager();
		pActionMapMan->EnableActionMap("multiplayer", !isMultiplayer);
		pActionMapMan->EnableActionMap("singleplayer", isMultiplayer);

		IActionMap *am = NULL;
		if(isMultiplayer)
		{
			am = pActionMapMan->GetActionMap("multiplayer");
		}
		else
		{
			am = pActionMapMan->GetActionMap("singleplayer");
		}
		if(am)
		{
			am->SetActionListener(0);
		}
	}

	if (isMultiplayer && gEnv->bServer)
	{
		if (m_statsRecordingModule)
		{
			m_statsRecordingModule->OnGameEnd();
		}
	}

	SaveSessionStatistics();
}

//------------------------------------------------------------------------
void CGameRules::GameOver(EGameOverType localWinner)
{
	g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(0, GameplayEvent(eGE_GameEnd, 0, 1.0f));
	if(m_rulesListeners.empty() == false)
	{
		TGameRulesListenerVec::iterator iter = m_rulesListeners.begin();
		while (iter != m_rulesListeners.end())
		{
			(*iter)->GameOver(localWinner);
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::EnteredGame()
{
	if(m_rulesListeners.empty() == false)
	{
		TGameRulesListenerVec::iterator iter = m_rulesListeners.begin();
		while (iter != m_rulesListeners.end())
		{
			(*iter)->EnteredGame();
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::EndGameNear(EntityId id)
{
	if(m_rulesListeners.empty() == false)
	{
		TGameRulesListenerVec::iterator iter = m_rulesListeners.begin();
		while(iter != m_rulesListeners.end())
		{
			(*iter)->EndGameNear(id);
			++iter;
		}
	}
}


//------------------------------------------------------------------------
void CGameRules::ClientEnteredGame_NotifyListeners( EntityId clientId )
{
	if(m_rulesListeners.empty() == false)
	{
		TGameRulesListenerVec::iterator iter = m_rulesListeners.begin();
		while(iter != m_rulesListeners.end())
		{
			(*iter)->ClientEnteredGame( clientId );
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::ClientDisconnect_NotifyListeners( EntityId clientId )
{
	if(m_rulesListeners.empty() == false)
	{
		TGameRulesListenerVec::iterator iter = m_rulesListeners.begin();
		while(iter != m_rulesListeners.end())
		{
			(*iter)->ClientDisconnect( clientId );
			++iter;
		}
	}
}


//------------------------------------------------------------------------
void CGameRules::OnActorDeath_NotifyListeners( CActor* pActor )
{
	if(m_rulesListeners.empty() == false)
	{
		TGameRulesListenerVec::iterator iter = m_rulesListeners.begin();
		while(iter != m_rulesListeners.end())
		{
			(*iter)->OnActorDeath( pActor );
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::SvOnTimeLimitExpired_NotifyListeners()
{
	CRY_ASSERT(gEnv->bServer);
	if(m_rulesListeners.empty() == false)
	{
		TGameRulesListenerVec::iterator  iter = m_rulesListeners.begin();
		while (iter != m_rulesListeners.end())
		{
			(*iter)->SvOnTimeLimitExpired();
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::EntityRevived_NotifyListeners( EntityId entityId )
{
	if(m_revivedListenersVec.empty() == false)
	{
		TRevivedListenersVec::iterator  iter = m_revivedListenersVec.begin();
		while(iter != m_revivedListenersVec.end())
		{
			(*iter)->EntityRevived( entityId );
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::SvSurvivorCountRefresh_NotifyListeners(int count, const EntityId survivors[], int numKills)
{
	CRY_ASSERT(gEnv->bServer);
	if(m_survivorCountListenersVec.empty() == false)
	{
		TSurvivorCountListenersVec::iterator  iter = m_survivorCountListenersVec.begin();
		while(iter != m_survivorCountListenersVec.end())
		{
			(*iter)->SvSurvivorCountRefresh(count, survivors, numKills);
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::ClPlayerStatsNetSerializeReadDeath_NotifyListeners(const SGameRulesPlayerStat* s, uint16 prevDeathsThisRound, uint8 prevFlags)
{
	CRY_ASSERT(gEnv->bClient);
	if (m_playerStatsListenersVec.empty() == false)
	{
		TPlayerStatsListenersVec::iterator  iter = m_playerStatsListenersVec.begin();
		while (iter != m_playerStatsListenersVec.end())
		{
			(*iter)->ClPlayerStatsNetSerializeReadDeath(s, prevDeathsThisRound, prevFlags);
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::OnRoundStart_NotifyListeners()
{
	if (m_roundsListenersVec.empty() == false)
	{
		TRoundsListenersVec::iterator  iter = m_roundsListenersVec.begin();
		while (iter != m_roundsListenersVec.end())
		{
			(*iter)->OnRoundStart();
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::ClRoundsNetSerializeReadState_NotifyListeners(int newState, int curState)
{
	CRY_ASSERT(!gEnv->bServer);
	CRY_ASSERT(gEnv->bClient);
	if (m_roundsListenersVec.empty() == false)
	{
		TRoundsListenersVec::iterator  iter = m_roundsListenersVec.begin();
		while (iter != m_roundsListenersVec.end())
		{
			(*iter)->ClRoundsNetSerializeReadState(newState, curState);
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::ClientScoreEvent(EGameRulesScoreType scoreType, int points)
{
	if(m_clientScoreListenersVec.empty() == false)
	{
		TClientScoreListenersVec::iterator iter = m_clientScoreListenersVec.begin();
		while (iter != m_clientScoreListenersVec.end())
		{
			(*iter)->ClientScoreEvent(scoreType, points);
			++iter;
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::CreateEntityRespawnData(EntityId entityId)
{
	if (!gEnv->bServer || m_pGameFramework->IsEditing())
		return;

	IEntity *pEntity=m_pEntitySystem->GetEntity(entityId);
	if (!pEntity)
		return;

	SEntityRespawnData respawn;
	respawn.position = pEntity->GetWorldPos();
	respawn.rotation = pEntity->GetWorldRotation();
	respawn.scale = pEntity->GetScale();
	respawn.flags = pEntity->GetFlags();
	respawn.pClass = pEntity->GetClass();
#ifdef _DEBUG
	respawn.name = pEntity->GetName();
#endif
	
	IScriptTable *pScriptTable = pEntity->GetScriptTable();

	if (pScriptTable)
		pScriptTable->GetValue("Properties", respawn.properties);

	m_respawndata.insert(TEntityRespawnDataMap::value_type(entityId, respawn));
}

//------------------------------------------------------------------------
const CGameRules::SEntityRespawnData *CGameRules::GetEntityRespawnData(EntityId entityId) const
{
	TEntityRespawnDataMap::const_iterator it = m_respawndata.find(entityId);
	if (it != m_respawndata.end())
	{
		return &it->second;
	}
	else
	{
		return NULL;
	}
}

//------------------------------------------------------------------------
bool CGameRules::HasEntityRespawnData(EntityId entityId) const
{
	return m_respawndata.find(entityId)!=m_respawndata.end();
}

//------------------------------------------------------------------------
void CGameRules::ScheduleEntityRespawn(EntityId entityId, bool unique, float timer)
{
	if (!gEnv->bServer || m_pGameFramework->IsEditing())
		return;

	IEntity *pEntity=m_pEntitySystem->GetEntity(entityId);
	if (!pEntity)
		return;

	SEntityRespawn respawn;
	respawn.timer = timer;
	respawn.unique = unique;

	m_respawns.insert(TEntityRespawnMap::value_type(entityId, respawn));
}

//------------------------------------------------------------------------
void CGameRules::DoEntityRespawn(EntityId id)
{
	TEntityRespawnDataMap::iterator dit=m_respawndata.find(id);

	if (dit != m_respawndata.end())
	{
		SEntityRespawnData &data=dit->second;

		SEntitySpawnParams params;
		params.pClass=data.pClass;
		params.qRotation=data.rotation;
		params.vPosition=data.position;
		params.vScale=data.scale;
		params.nFlags=data.flags;

		string name;
#ifdef _DEBUG
		name=data.name;
		name.append("_repop");
#else
		name=data.pClass->GetName();
#endif
		params.sName = name.c_str();

		IEntity *pEntity=m_pEntitySystem->SpawnEntity(params, false);
		if (pEntity && data.properties.GetPtr())
		{
			SmartScriptTable properties;
			IScriptTable *pScriptTable=pEntity->GetScriptTable();
			if (pScriptTable && pScriptTable->GetValue("Properties", properties))
			{
				if (properties.GetPtr())
				{
					properties->Clone(data.properties, true);
				}
			}
		}

		m_pEntitySystem->InitEntity(pEntity, params);
		m_respawndata.erase(dit);

		if (pEntity)
			OnEntityRespawn(pEntity);
	}
}

//------------------------------------------------------------------------
void CGameRules::UpdateEntitySchedules(float frameTime)
{
	if (!gEnv->bServer || m_pGameFramework->IsEditing())
		return;

	TEntityRespawnMap::iterator next;
	for (TEntityRespawnMap::iterator it=m_respawns.begin(); it!=m_respawns.end(); it=next)
	{
		next=it; ++next;
		EntityId id=it->first;
		SEntityRespawn &respawn=it->second;

		if (respawn.unique)
		{
			IEntity *pEntity=m_pEntitySystem->GetEntity(id);
			if (pEntity)
				continue;
		}

		respawn.timer -= frameTime;
		if (respawn.timer<=0.0f)
		{
			DoEntityRespawn(id);
			m_respawns.erase(it);
		}
	}

	TEntityRemovalMap::iterator rnext;
	for (TEntityRemovalMap::iterator it=m_removals.begin(); it!=m_removals.end(); it=rnext)
	{
		rnext=it; ++rnext;
		EntityId id=it->first;
		SEntityRemovalData &removal=it->second;

		IEntity *pEntity=m_pEntitySystem->GetEntity(id);
		if (!pEntity)
		{
			m_removals.erase(it);
			continue;
		}

		if (removal.visibility)
		{
			AABB aabb;
			pEntity->GetWorldBounds(aabb);

			CCamera &camera=m_pSystem->GetViewCamera();
			if (camera.IsAABBVisible_F(aabb))
			{
				removal.timer=removal.time;
				continue;
			}
		}

		removal.timer-=frameTime;
		if (removal.timer<=0.0f)
		{
			m_pEntitySystem->RemoveEntity(id);
			m_removals.erase(it);
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::FlushEntitySchedules()
{
	TEntityRemovalMap::iterator removalIt;
	for (removalIt = m_removals.begin(); removalIt != m_removals.end(); ++ removalIt)
	{
		EntityId id = removalIt->first;

		IEntity *pEntity=m_pEntitySystem->GetEntity(id);
		if (pEntity)
		{
			m_pEntitySystem->RemoveEntity(id);
		}
	}
	m_removals.clear();

	TEntityRespawnMap::iterator respawnIt;
	for (respawnIt = m_respawns.begin(); respawnIt != m_respawns.end(); ++ respawnIt)
	{
		DoEntityRespawn(respawnIt->first);
	}
	m_respawns.clear();
}

//------------------------------------------------------------------------
void CGameRules::ForceScoreboard(bool force)
{
	SAFE_HUD_FUNC(ForceScoreBoard(force));
}

//------------------------------------------------------------------------
void CGameRules::FreezeInput(bool freeze)
{
#if !defined(CRY_USE_GCM_HUD)
	if (gEnv->pInput) gEnv->pInput->ClearKeyState();
#endif

	g_pGameActions->FilterFreezeTime()->Enable(freeze);

	if (freeze)
	{
		CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
		IPlayerInput* pPlayerInput = pPlayer ? pPlayer->GetPlayerInput() : NULL;
		if(pPlayerInput)
		{
			pPlayerInput->Reset();
		}
	}

/*
	if (IActor *pClientIActor=g_pGame->GetIGameFramework()->GetClientActor())
	{
		IActor *pClientActor=static_cast<IActor *>(pClientIActor);
		if (CWeapon *pWeapon=pClientActor->GetWeapon(pClientActor->GetCurrentItemId()))
			pWeapon->StopFire(pClientActor->GetEntityId());
	}
	*/
}

//------------------------------------------------------------------------
bool CGameRules::IsProjectile(EntityId id) const
{
	return g_pGame->GetWeaponSystem()->GetProjectile(id)!=0;
}

//------------------------------------------------------------------------
void CGameRules::AbortEntityRespawn(EntityId entityId, bool destroyData)
{
	stl::member_find_and_erase(m_respawns, entityId);

	if (destroyData)
	{
		stl::member_find_and_erase(m_respawndata, entityId);
	}
}

//------------------------------------------------------------------------
void CGameRules::ScheduleEntityRemoval(EntityId entityId, float timer, bool visibility)
{
	if (!gEnv->bServer || gEnv->bEditor)
		return;

	IEntity *pEntity=m_pEntitySystem->GetEntity(entityId);
	if (!pEntity)
		return;

	SEntityRemovalData removal;
	removal.time = timer;
	removal.timer = timer;
	removal.visibility = visibility;

	m_removals.insert(TEntityRemovalMap::value_type(entityId, removal));
}

//------------------------------------------------------------------------
void CGameRules::AbortEntityRemoval(EntityId entityId)
{
	stl::member_find_and_erase(m_removals, entityId);
}

void CGameRules::ShowStatus()
{
	float timeRemaining = GetRemainingGameTime();
	int mins = (int)(timeRemaining / 60.0f);
	int secs = (int)(timeRemaining - mins*60);
	CryLogAlways("time remaining: %d:%02d", mins, secs);
}
#ifndef OLD_VOICE_SYSTEM_DEPRECATED
void CGameRules::ReconfigureVoiceGroups(EntityId id,int old_team,int new_team)
{
	INetContext *pNetContext = g_pGame->GetIGameFramework()->GetNetContext();
	if(!pNetContext)
		return;

	IVoiceContext *pVoiceContext = pNetContext->GetVoiceContext();
	if(!pVoiceContext)
		return; // voice context is now disabled in single player game. talk to me if there are any problems - Lin

	if(old_team==new_team)	
		return;

	TTeamIdVoiceGroupMap::iterator iter=m_teamVoiceGroups.find(old_team);
	if(iter!=m_teamVoiceGroups.end())
	{
		iter->second->RemoveEntity(id);
		//CryLog("<--Removing entity %d from team %d", id, old_team);
	}
	else
	{
		//CryLog("<--Failed to remove entity %d from team %d", id, old_team);
	}

	iter=m_teamVoiceGroups.find(new_team);
	if(iter==m_teamVoiceGroups.end())
	{
		IVoiceGroup* pVoiceGroup=pVoiceContext->CreateVoiceGroup();
		iter=m_teamVoiceGroups.insert(std::make_pair(new_team,pVoiceGroup)).first;
	}
	iter->second->AddEntity(id);
	pVoiceContext->InvalidateRoutingTable();
	//CryLog("-->Adding entity %d to team %d", id, new_team);
}
#endif

float CGameRules::GameRulesPlayerSpeedAdjustment(EntityId playerId)
{
	float result = 1.f;

	HSCRIPTFUNCTION pfnAdjustSpeed=0;
	if (m_script->GetValue("AdjustPlayerSpeed", pfnAdjustSpeed))
	{
		Script::CallReturn(gEnv->pScriptSystem, pfnAdjustSpeed, m_script, ScriptHandle(playerId), result);
		gEnv->pScriptSystem->ReleaseFunc(pfnAdjustSpeed);
	}

	return result;
}

CBattleDust* CGameRules::GetBattleDust() const
{
	return m_pBattleDust;
}

CMPTutorial* CGameRules::GetMPTutorial() const
{
	return m_pMPTutorial;
}

void CGameRules::ForceSynchedStorageSynch(int channel)
{
	if (!gEnv->bServer)
		return;

	g_pGame->GetServerSynchedStorage()->FullSynch(channel, true);
}


void CGameRules::PlayerPosForRespawn(CPlayer* pPlayer, bool save)
{
	static 	Matrix34	respawnPlayerTM(IDENTITY);
	if (save)
	{
		respawnPlayerTM = pPlayer->GetEntity()->GetWorldTM();
	}
	else
	{
		pPlayer->GetEntity()->SetWorldTM(respawnPlayerTM);
	}
}

void CGameRules::SPNotifyPlayerKill(EntityId targetId, EntityId weaponId, bool bHeadShot)
{
	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	EntityId wepId[1] = {weaponId};
	if (pActor)
		m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Kill,0,0,wepId)); 
}


void CGameRules::GetMemoryUsage(ICrySizer *s) const
{
	s->Add(*this);
	s->AddObject(m_channelIds);
	s->AddObject(m_teams);
	s->AddObject(m_entityteams);
	s->AddObject(m_channelteams);
	s->AddObject(m_teamdefaultspawns);
	s->AddObject(m_playerteams);
	s->AddObject(m_hitMaterials);
	s->AddObject(m_hitTypes);
	s->AddObject(m_respawndata);
	s->AddObject(m_respawns);
	s->AddObject(m_removals);
	s->AddObject(m_objectives);
#if 0
	s->AddObject(m_spawnLocations);
	s->AddObject(m_baseSpawnLocations);
	s->AddObject(m_nonBaseSpawnLocations);
#endif
	s->AddObject(m_spawnGroups);
#ifndef OLD_VOICE_SYSTEM_DEPRECATED
	s->AddObject(m_teamVoiceGroups);
#endif
	s->AddObject(m_rulesListeners);
	s->AddObject(m_teamperks);
	
}

// sync up the telemetry session name so all clients in the game file their telemetry in the same place
// ITelemetryCollector can be NULL
bool CGameRules::NetSerializeTelemetry( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (aspect == eEA_GameServerStatic)
	{
		// FIXME probably not the wisest use of our bandwidth, is the server name not stored elsewhere already?
		// FIXME session id changes when sessions are started and ended, we aren't marking this aspect as dirty in those cases. potential fix is to have a listener on the session id changing

		ITelemetryCollector		*tc=static_cast<CGame*>(gEnv->pGame)->GetITelemetryCollector();

		if (ser.IsWriting())
		{
			ser.Value("sessionid",tc ? tc->GetSessionId() : "");
		}
		else
		{
			string	session;
			ser.Value("sessionid",session);
			if (tc)
			{
				tc->SetSessionId(session);
			}
		}
	}

	return true;
}

bool CGameRules::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (aspect == eEA_GameServerDynamic)
	{
		uint32 flags = 0;
		if (ser.IsReading())
		{
			flags |= ITimeOfDay::NETSER_COMPENSATELAG;
			if (!m_timeOfDayInitialized)
			{
				flags |= ITimeOfDay::NETSER_FORCESET;
				m_timeOfDayInitialized = true;
			}
		}
		gEnv->p3DEngine->GetTimeOfDay()->NetSerialize( ser, 0.0f, flags );			
	}
	else
	{
		if (aspect == eEA_GameServerStatic)
		{
			gEnv->p3DEngine->GetTimeOfDay()->NetSerialize( ser, 0.0f, ITimeOfDay::NETSER_STATICPROPS );
		}
		if (aspect == GAMERULES_TEAMS_SCORE_ASPECT)
		{
			int teamId = 0;
			uint16 score = 0;
			uint16 roundScore = 0;
			uint16 scoreRoundStart = 0;
			int numTeams = m_teamscores.size();

			ser.Value("numTeams", numTeams, 'team');

			for (TTeamScoresMap::iterator iter=m_teamscores.begin(); iter!=m_teamscores.end(); ++iter)
			{
				if (ser.IsWriting())
				{
					teamId=iter->first;
					score=iter->second.m_teamScore;
					roundScore=iter->second.m_roundTeamScore;
					scoreRoundStart=iter->second.m_teamScoreRoundStart;
				}

				ser.Value("teamId", teamId, 'team');
				ser.Value("score", score, 'ui16');
				ser.Value("roundScore", roundScore, 'ui16');
				ser.Value("scoreRoundStart", scoreRoundStart, 'ui16');

				if (ser.IsReading())
				{
					CRY_TODO(8,2,2010, "Providing the assert below never triggers, look into removing the serialiation of 'teamId' here - if needed it should be able to be got from the 'first' member of the iterator anyways");
					CRY_ASSERT(teamId == iter->first);
					iter->second.m_teamScore = score;
					iter->second.m_roundTeamScore = roundScore;
					iter->second.m_teamScoreRoundStart = scoreRoundStart;
				}
			}
		}
		if (aspect == GAMERULES_LIMITS_ASPECT)
		{
			float newTimeLimit = m_timeLimit;
			int newScoreLimit = m_scoreLimit;
			int newRoundLimit = m_roundLimit;

			ser.Value("timeLimit", newTimeLimit, 'fsec');
			ser.Value("scoreLimit", newScoreLimit, 'ui16');
			ser.Value("roundLimit", newRoundLimit, 'ui16');

			if (ser.IsReading())
			{
				// Ensure that the cvars are kept in sync as well as the limits that actually get used (otherwise host migration will grab incorrect values)
				if (m_timeLimit != newTimeLimit)
				{
					m_timeLimit = newTimeLimit;
					g_pGameCVars->g_timelimit = m_timeLimit;
				}
				if (m_scoreLimit != newScoreLimit)
				{
					m_scoreLimit = newScoreLimit;
					g_pGameCVars->g_scoreLimit = m_scoreLimit;
				}
				if (m_roundLimit != newRoundLimit)
				{
					m_roundLimit = newRoundLimit;
					g_pGameCVars->g_roundlimit = m_roundLimit;
				}
			}
		}
	}

	CPerk::GetInstance()->NetSerialize(ser, aspect, profile, flags);

	bool success = true;

	CRY_TODO(06, 10, 2009, "[CG] Make these serialise calls into a list of listeners!");
	if (m_stateModule)
	{
		success &= m_stateModule->NetSerialize(ser, aspect, profile, flags);
	}
	if (m_objectivesModule)
	{
		success &= m_objectivesModule->NetSerialize(ser, aspect, profile, flags);
	}
	if (m_teamsModule)
	{
		success &= m_teamsModule->NetSerialize(ser, aspect, profile, flags);
	}
	if (m_spawningModule)
	{
		success &= m_spawningModule->NetSerialize(ser, aspect, profile, flags);
	}
	if (m_roundsModule)
	{
		success &= m_roundsModule->NetSerialize(ser, aspect, profile, flags);
	}
	success&=NetSerializeTelemetry(ser,aspect,profile,flags);

	return success;
}

bool CGameRules::OnBeginCutScene(IAnimSequence* pSeq, bool bResetFX)
{
	if(!pSeq)
		return false;

	if(m_pExplosionGameEffect)
	{
		m_pExplosionGameEffect->SetCutSceneActive(true);
	}
	
	return true;
}

bool CGameRules::OnEndCutScene(IAnimSequence* pSeq)
{
	if(!pSeq)
		return false;

	if(m_pExplosionGameEffect)
	{
		m_pExplosionGameEffect->SetCutSceneActive(false);
	}

	return true;
}

bool CGameRules::IsGameRulesClass(const char *cls)
{
	if(!cls || stricmp(cls, GetEntity()->GetClass()->GetName()))
		return false;
	return true;
}

void CGameRules::UpdateTeamPerkCount(int teamId, ETeamPerks teamPerkID, EntityId activatorId, int change)
{
	TTeamPerkMap::iterator tpit=m_teamperks.find(teamId);
	if (tpit!=m_teamperks.end())
	{
		CTeamPerks &pTeamPerks = tpit->second;
		pTeamPerks.UpdateTeamPerkCount(teamId, teamPerkID, activatorId, change);
	}
}

CTeamPerks *CGameRules::GetTeamPerks(int teamId)
{
	TTeamPerkMap::iterator tpit=m_teamperks.find(teamId);
	if (tpit!=m_teamperks.end())
	{
		return &tpit->second;
	}
	return NULL;
}

//	need this to mark used spawn-points, to make sure no two actors are placed at the same location on Restart
//------------------------------------------------------------------------
bool CGameRules::IsSpawnUsed( const EntityId spawnId )
{
	float curTime = GetISystem()->GetITimer()->GetCurrTime();

	TSpawnPointUseTime::iterator it=m_SpawnPointUseTime.find(spawnId);
	if (it!=m_SpawnPointUseTime.end())
	{
		float diff = curTime - it->second;
		it->second = curTime;
		if(diff>2.f)
			return false;
		return true;
	}
	m_SpawnPointUseTime[spawnId] = curTime;
	return false;
}

bool CGameRules::CanPlayerSwitchItem( EntityId playerId )
{
	int canSwitch = 1;

	HSCRIPTFUNCTION pfnCanPlayerSwitchItem = 0;
	if (m_script->GetValue("CanPlayerSwitchItem", pfnCanPlayerSwitchItem))
	{
		ScriptHandle playerIdHandle(playerId);
		Script::CallReturn(gEnv->pScriptSystem, pfnCanPlayerSwitchItem, m_script, playerIdHandle, canSwitch);
		gEnv->pScriptSystem->ReleaseFunc(pfnCanPlayerSwitchItem);
	}

	return (canSwitch != 0);
}

#if 0
EntityId CGameRules::GetSpawnLocationNonTeamGame(EntityId playerId, const Vec3 &deathPos)
{
	EntityId id=0;
	float minDistToDeathSqr(g_pGameCVars->g_spawndeathdist*g_pGameCVars->g_spawndeathdist);

	id = GetSpawnLocationFurthestAwayFromAnyPlayer(playerId, deathPos, minDistToDeathSqr);
	return id;
}

EntityId CGameRules::GetSpawnLocationTeamGame(EntityId playerId, const Vec3 &deathPos)
{
	EntityId id=0;
	int spawnMode=g_pGameCVars->g_debug_spawn_mode;

	bool useSpawnPointTeams=true;

	/*switch (rules)
	{
		case EHUD_TEAMINSTANTACTION:		// non-team spawns
		case EHUD_RACE:						// non-team spawns
		case EHUD_CRASHSITE:				// non-team spawns
		case EHUD_COUNTDOWN:				// non-team spawns
		case EHUD_PAYLOAD:					// non-team spawns
			CryLog("CGameRules::GetSpawnLocationTeamGame() gamemode=%d uses non-team based spawns", rules);
			useSpawnPointTeams=false;
			break;

		case EHUD_CAPTURETHEFLAG: 
		case EHUD_BOMBTHEBASE:
		case EHUD_EXTRACTION:
		case EHUD_ASSAULT:
			CryLog("CGameRules::GetSpawnLocationTeamGame() gamemode=%d uses team based spawns", rules);
			useSpawnPointTeams=true;
			break;

		default:
			CryLog("CGameRules::GetSpawnLocationTeamGame() unhandled gamemode=%d needs fixing", rules);
			break;
	}*/

	const IActor *playerActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId);
	if (playerActor)
	{
		const CPlayer* pPlayer = static_cast<const CPlayer*>(playerActor);
		spawnMode=pPlayer->GetSpawnModeType();

		CryLog("CGameRules::GetSpawnLocationTeamGame() Player %s found - using their spawn mode type=%d", pPlayer->GetEntity()->GetName(), spawnMode);
	}
	else
	{
		CryLog("CGameRules::GetSpawnLocationTeamGame() failed to find a valid playerActor, using cvar debugspawn mode=%d", spawnMode);
	}

	float zOffset=0.0f;


#if DEBUG_NEW_SPAWNING		// This whole function 

	CryLog("CGameRules::GetSpawnLocationTeamGame() spawnMode=%d", spawnMode);
	switch (spawnMode)
	{
//		case 0:
//			CryLog("debug spawn mode is 0 normal behaviour");
			//id = pGameRules->GetSpawnLocationTeam((EntityId)playerId.n, deathPos);	// actually doesn't respect teams on the spawn points themselves
//			id=pGameRules->GetSpawnLocation((EntityId)playerId.n, false, false, 0, 0.0f, deathPos, &zOffset);
//			break;
		case 1:
		{
			CryLog("debug spawn mode is 1: HQ spawn mode");
			int numBaseSpawns = GetNumberOfBaseSpawnLocations();
			int numNonBaseSpawns = GetNumberOfNonBaseSpawnLocations();

			CryLog("Num Base Spawns=%d; Num nonBaseSpawns=%d", numBaseSpawns, numNonBaseSpawns);

			// TODO - find a better way of getting the game rules than asking the HUD

			bool gameModeIsHQCompatible=false;
/*			if (rules==EHUD_POWERSTRUGGLE || rules==EHUD_POWERSTRUGGLELITE || rules==EHUD_CAPTURETHEFLAG || rules==EHUD_BOMBTHEBASE || rules==EHUD_EXTRACTION)	// probably doesn't need extraction here, only single spawns in bases per round anyway 
			{
				if (numBaseSpawns > 0)	// really need enough for each team but we're trying to detect a level that's had some base spawns setup
				{
					gameModeIsHQCompatible=true;
				}
				else
				{
					CryLog("Valid game mode but there are no basespawns so aren't able to do HQ spawning");
				}
			}*/

			if (gameModeIsHQCompatible)
			{
				id = GetSpawnLocationHQMode(playerId, deathPos);
			}
			else
			{
				CryLog("spawn mode is HQ yet gamerules is not HQ compatible. Using Frontline spawning mode");
				id = GetSpawnLocationFrontlineMode(playerId, deathPos, useSpawnPointTeams);
			}
			break;
		}
		case 2:
		{
			CryLog("debug spawn mode is 2: Frontline spawn mode");
			int numBaseSpawns=GetNumberOfBaseSpawnLocations();
			int numNonBaseSpawns=GetNumberOfNonBaseSpawnLocations();

			CryLog("Num Base Spawns=%d; Num nonBaseSpawns=%d", numBaseSpawns, numNonBaseSpawns);

			id = GetSpawnLocationFrontlineMode(playerId, deathPos, useSpawnPointTeams);
			break;
		}
		default:
			CryLog("unhandled spawn mode of %d", g_pGameCVars->g_debug_spawn_mode);
			break;
	}
#else // DEBUG_NEW_SPAWNING
	CryLog("CGameRules::GetSpawnLocationTeamGame() new behaviour DISABLED due to DEBUG_NEW_SPAWNING=0");
	id=GetSpawnLocation(playerId, !useSpawnPointTeams, !useSpawnPointTeams, false, 0, 0.0f, deathPos, &zOffset);
#endif // DEBUG_NEW_SPAWNING

	return id;
}
#endif


EntityId CGameRules::GetSpawnLocationNew(EntityId playerId, float &zOffset)
{
	CRY_ASSERT_TRACE (m_spawningModule, ("No spawning module present while trying to get a spawn location for player '%s'", GetEntityName(playerId)));

	return m_spawningModule->GetSpawnLocation(playerId, zOffset);
}

#if 0
//------------------------------------------------------------------------
EntityId CGameRules::GetSpawnLocationNew(EntityId playerId, const Vec3 &deathPos, float &zOffset)
{
	EntityId id=0;
	
	zOffset = 0.0f;	// not being passed through to new spawning algorithms at the moment

//	EHUDGAMERULES rules = (EHUDGAMERULES) SAFE_HUD_FUNC_RET(GetGameRules());
	IEntity *playerEntity = gEnv->pEntitySystem->GetEntity(playerId);
	if (!playerEntity)
	{
		CryLog("ERROR: GetSpawnLocationNew() failed to find a playerEntity to work with. Returning.");
		return 0;
	}

/*	switch (rules)
	{
		case EHUD_ALLORNOTHING:
		case EHUD_INSTANTACTION:
			CryLog("CGameRules::GetSpawnLocationNew() player=%s; gamerules=%d is a non-teamgame doing NonTeamGame spawning\n", playerEntity->GetName(), rules);
			id = GetSpawnLocationNonTeamGame(playerId, deathPos);
			break;

		case EHUD_POWERSTRUGGLE:
		case EHUD_POWERSTRUGGLELITE:
		case EHUD_TEAMINSTANTACTION:		// non-team spawns
		case EHUD_CAPTURETHEFLAG: 
		case EHUD_BOMBTHEBASE:
		case EHUD_RACE:						// non-team spawns
		case EHUD_CRASHSITE:				// non-team spawns
		case EHUD_EXTRACTION:
		case EHUD_COUNTDOWN:				// non-team spawns
		case EHUD_PAYLOAD:				// non-team spawns
		case EHUD_ASSAULT:
			CryLog("CGameRules::GetSpawnLocationNew() player=%s; gamerules=%d is a teamgame doing teamGame spawning\n", playerEntity->GetName(), rules);
			id = GetSpawnLocationTeamGame(playerId, deathPos);
			break;

		default:
			CryLog("CGameRules::GetSpawnLocationNew() player=%s; unhandled gamerules=%d - this needs fixing\n", playerEntity->GetName(), rules);
			assert(0);
			break;
	}*/

	if (!id)
	{
		CryLog("GetSpawnLocationNew() player=%s; has failed to find a valid spawn, so now calling GetSpawnLocation() to find a spawn point", playerEntity->GetName());
		id=GetSpawnLocation(playerId, true, true, 0, g_pGameCVars->g_spawndeathdist, deathPos, &zOffset);
	}

	if (id)
	{
		if(IsSpawnUsed(id))
			zOffset += .5f;
		return id;
	}
	else
	{
		CryLog("GetSpawnLocationNew() player=%s; has failed to find a valid spawn itself and within GetSpawnLocation(). Something is seriously broken", playerEntity->GetName());
	}

	return 0;
}
#endif

bool CGameRules::UseNanoSuit( EntityId playerId )
{
	int useSuit = 1;

	HSCRIPTFUNCTION pfnPlayerUseNanoSuit = 0;
	if (m_script->GetValue("PlayerUseNanoSuit", pfnPlayerUseNanoSuit))
	{
		ScriptHandle playerIdHandle(playerId);
		Script::CallReturn(gEnv->pScriptSystem, pfnPlayerUseNanoSuit, m_script, playerIdHandle, useSuit);
		gEnv->pScriptSystem->ReleaseFunc(pfnPlayerUseNanoSuit);
	}

	return (useSuit != 0);
}

bool CGameRules::RulesUseWeaponLoadouts()
{
	int  use = 1;

	HSCRIPTFUNCTION  func = 0;
	if (m_script->GetValue("RulesUseWeaponLoadouts", func))
	{
		Script::CallReturn(gEnv->pScriptSystem, func, m_script, use);
		gEnv->pScriptSystem->ReleaseFunc(func);
	}

	return (use != 0);
}

bool CGameRules::RulesUsePerkLoadouts()
{
	int  use = 1;

	HSCRIPTFUNCTION  func = 0;
	if (m_script->GetValue("RulesUsePerkLoadouts", func))
	{
		Script::CallReturn(gEnv->pScriptSystem, func, m_script, use);
		gEnv->pScriptSystem->ReleaseFunc(func);
	}

	return (use != 0);
}

void CGameRules::RegisterPickupListener( IGameRulesPickupListener *pListener )
{
	if (!stl::find(m_pickupListeners, pListener))
	{
		m_pickupListeners.push_back(pListener);
	}
}

void CGameRules::UnRegisterPickupListener( IGameRulesPickupListener *pListener )
{
	stl::find_and_erase(m_pickupListeners, pListener);
}

void CGameRules::RegisterClientConnectionListener( IGameRulesClientConnectionListener *pListener )
{
	if (!stl::find(m_clientConnectionListeners, pListener))
	{
		m_clientConnectionListeners.push_back(pListener);
	}
}

void CGameRules::UnRegisterClientConnectionListener( IGameRulesClientConnectionListener *pListener )
{
	stl::find_and_erase(m_clientConnectionListeners, pListener);
}

void CGameRules::RegisterTeamChangedListener( IGameRulesTeamChangedListener *pListener )
{
	if (!stl::find(m_teamChangedListeners, pListener))
	{
		m_teamChangedListeners.push_back(pListener);
	}
}

void CGameRules::UnRegisterTeamChangedListener( IGameRulesTeamChangedListener *pListener )
{
	stl::find_and_erase(m_teamChangedListeners, pListener);
}

void CGameRules::RegisterRevivedListener( IGameRulesRevivedListener *pListener )
{
	if (!stl::find(m_revivedListenersVec, pListener))
	{
		m_revivedListenersVec.push_back(pListener);
	}
}

void CGameRules::UnRegisterRevivedListener( IGameRulesRevivedListener *pListener )
{
	stl::find_and_erase(m_revivedListenersVec, pListener);
}

void CGameRules::RegisterSurvivorCountListener( IGameRulesSurvivorCountListener *pListener )
{
	if (!stl::find(m_survivorCountListenersVec, pListener))
	{
		m_survivorCountListenersVec.push_back(pListener);
	}
}

void CGameRules::UnRegisterSurvivorCountListener( IGameRulesSurvivorCountListener *pListener )
{
	stl::find_and_erase(m_survivorCountListenersVec, pListener);
}

void CGameRules::RegisterPlayerStatsListener( IGameRulesPlayerStatsListener *pListener )
{
	if (!stl::find(m_playerStatsListenersVec, pListener))
	{
		m_playerStatsListenersVec.push_back(pListener);
	}
}

void CGameRules::UnRegisterPlayerStatsListener( IGameRulesPlayerStatsListener *pListener )
{
	stl::find_and_erase(m_playerStatsListenersVec, pListener);
}

void CGameRules::RegisterRoundsListener( IGameRulesRoundsListener *pListener )
{
	if (!stl::find(m_roundsListenersVec, pListener))
	{
		m_roundsListenersVec.push_back(pListener);
	}
}

void CGameRules::UnRegisterRoundsListener( IGameRulesRoundsListener *pListener )
{
	stl::find_and_erase(m_roundsListenersVec, pListener);
}

void CGameRules::RegisterClientScoreListener( IGameRulesClientScoreListener *pListener )
{
	stl::push_back_unique(m_clientScoreListenersVec, pListener);
}

void CGameRules::UnRegisterClientScoreListener( IGameRulesClientScoreListener *pListener )
{
	stl::find_and_erase(m_clientScoreListenersVec, pListener);
}

void CGameRules::RegisterKillListener( IGameRulesKillListener *pListener )
{
	if (!stl::find(m_killListeners, pListener))
	{
		m_killListeners.push_back(pListener);
	}
}

void CGameRules::UnRegisterKillListener( IGameRulesKillListener *pListener )
{
	stl::find_and_erase(m_killListeners, pListener);
}

void CGameRules::OnEntityKilled( const HitInfo &hitInfo )
{
	int numListeners = m_killListeners.size();
	for (int i = 0; i < numListeners; ++ i)
	{
		IGameRulesKillListener *pKillListener = m_killListeners[i];
		pKillListener->OnEntityKilled(hitInfo);
	}
}

int CGameRules::RegisterModuleRMIListener( IGameRulesModuleRMIListener *pRMIListener )
{
	int index = m_moduleRMIListenersVec.size();

	m_moduleRMIListenersVec.push_back(pRMIListener);

	return index;
}

void CGameRules::UnRegisterModuleRMIListener( int index )
{
	assert(m_moduleRMIListenersVec.size() > index);
	m_moduleRMIListenersVec[index] = NULL;
}

void CGameRules::OnActionEvent( const SActionEvent& event )
{
	switch(event.m_event)
	{
	case eAE_resetBegin:
		{
			const SHUDEvent event(eHUDEvent_OnHUDReload);
			CHUD::CallEvent(event);

			for (TPlayerTeamIdMap::iterator tit=m_playerteams.begin(); tit!=m_playerteams.end(); ++tit)
				tit->second.resize(0);
			m_entityteams.clear();
		}
		break;
	}
}

//bool CGameRules::IsHeadshot(int partId) const
//{
//	if(m_damageHandlingModule)
//	{
//		return (m_damageHandlingModule->GetPartFlags(partId) & IGameRulesDamageHandlingModule::PID_Headshot) == IGameRulesDamageHandlingModule::PID_Headshot;
//	}
//	CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Missing damage Handling Module - therefore IsHeadshot will always return false");
//	return false;
//}

bool CGameRules::GetMigratedPlayerLocation(uint16 channelId, Vec3& pos, Ang3& ori)
{
	// Check if there's a migrating player for this channel
	int migratingIndex = GetMigratingPlayerIndex(channelId);

	if (migratingIndex >= 0)
	{
		pos = m_pMigratingPlayerInfo[migratingIndex].m_pos;
		ori = m_pMigratingPlayerInfo[migratingIndex].m_ori;

		return true;
	}

	return false;
}

void CGameRules::SetupMigratedPlayer(uint16 channelId)
{
	// Check if there's a migrating player for this channel
	int migratingIndex = GetMigratingPlayerIndex(channelId);

	if (migratingIndex >= 0)
	{
		IActor *pActor = GetActorByChannelId(channelId);

		// SetTeam kicks off reviving the player and giving the initial load out
		SetTeam(m_pMigratingPlayerInfo[migratingIndex].m_team, pActor->GetEntityId());

		//TODO: Set up the remaining migrating player parameters here
		pActor->SetHealth(m_pMigratingPlayerInfo[migratingIndex].m_health);

		m_pMigratingPlayerInfo[migratingIndex].Reset();
	}
}

//------------------------------------------------------------------------
#if !defined(_RELEASE)
void CGameRules::SendNetConsoleCommand(const char *msg, unsigned int to, int channelId)
{
	GetGameObject()->InvokeRMI(ClNetConsoleCommand(), NetConsoleCommandParams(msg), to, channelId);
}
#endif

//------------------------------------------------------------------------
bool CGameRules::OnInitiate(SHostMigrationInfo& hostMigrationInfo)
{
	CryLog("Host Migration: CGameRules::OnInitiate() Saving character for host migration started");

	if (gEnv->bClient)
	{
		if (!m_pHostMigrationParams)
		{
			m_pHostMigrationParams = new SHostMigrationClientRequestParams();
			m_pHostMigrationClientParams = new SHostMigrationClientControlledParams();
		}
		m_pEquipmentLoadout->ClGetCurrentLoadoutParams(m_pHostMigrationParams->m_loadoutParams);

		CPlayer *pPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetClientActor());
		if (pPlayer)
		{
			m_pHostMigrationClientParams->m_viewQuat = pPlayer->GetViewRotation();
			m_pHostMigrationClientParams->m_position = pPlayer->GetEntity()->GetPos();

			IInventory *pInventory = pPlayer->GetInventory();

			m_pHostMigrationClientParams->m_numExpectedItems = pInventory->GetCount();

			int numAmmoTypes = pInventory->GetAmmoTypesCount();
			m_pHostMigrationClientParams->m_pAmmoParams = new SHostMigrationClientControlledParams::SAmmoParams[numAmmoTypes];
			m_pHostMigrationClientParams->m_numAmmoParams = numAmmoTypes;

			CryLog("  player has %i different ammo types", numAmmoTypes);
			for (int i = 0; i < numAmmoTypes; ++ i)
			{
				IEntityClass *pAmmoType = pInventory->GetAmmoType(i);
				int ammoCount = pInventory->GetAmmoCount(pAmmoType);

				m_pHostMigrationClientParams->m_pAmmoParams[i].m_pAmmoClass = pAmmoType;
				m_pHostMigrationClientParams->m_pAmmoParams[i].m_count = ammoCount;

				CryLog("    %s : %i", pAmmoType->GetName(), ammoCount);
			}

			const IGameRulesSpawningModule *pSpawningModule = GetSpawningModule();
			if (pSpawningModule)
			{
				if (pPlayer->IsDead() && !pSpawningModule->HasOptedOut())
				{
					// Calculate how long it should be till we auto-revive so we can tell the new server
					float autoReviveTime = (pSpawningModule->GetTimeFromDeathTillAutoRevive() * 0.001f);
					if (autoReviveTime >= 0.f)
					{
						float deathTime = pPlayer->GetDeathTime();
						if (deathTime > 0.f)
						{
							float curTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();
							float rem = MAX(0.f, autoReviveTime - (curTime - deathTime));

							m_pHostMigrationParams->m_timeToAutoRevive = rem;
						}
					}
				}
			}
		}
		else
		{
			CRY_ASSERT_MESSAGE(false, "Failed to find client actor when initiating a host migration");
			g_pGame->GetIGameFramework()->ExecuteCommandNextFrame("disconnect");
			return true;
		}

		if (m_pBattlechatter)
		{
			m_pBattlechatter->SetLocalPlayer(NULL);
		}
	}

	if (GetObjectivesModule())
	{
		GetObjectivesModule()->OnHostMigration(hostMigrationInfo.IsNewHost());
	}

	if (hostMigrationInfo.IsNewHost())
	{
		// Server time will change after we migrate (change from old server time to new server time)
		m_gameStartedTime.SetValue(m_gameStartedTime.GetValue() - m_cachedServerTime.GetValue());
	}

	SHUDEvent showHostMigration;
	showHostMigration.eventType = eHUDEvent_ShowHostMigrationScreen;
	CHUD::CallEvent(showHostMigration);

	m_hostMigrationTimeStateChanged = gEnv->pTimer->GetAsyncCurTime();
	m_hostMigrationState = eHMS_WaitingForPlayers;

	gEnv->pEntitySystem->PauseTimers(true, false);
	gEnv->pTimer->PauseTimer(ITimer::ETIMER_GAME, true);

	return true;
}

//------------------------------------------------------------------------
bool CGameRules::OnDemoteToClient(SHostMigrationInfo& hostMigrationInfo)
{
	CryLog("Host Migration: CGameRules::OnDemoteToClient() started");

	typedef std::vector<EntityId> TEntityIdVec;

	TEntityIdVec entitiesToRemove;
	entitiesToRemove.reserve(128);

	IItemSystem *pItemSystem = g_pGame->GetIGameFramework()->GetIItemSystem();
	IEntityIt *pEntityIt = gEnv->pEntitySystem->GetEntityIterator();

	IEntity *pEntity = NULL;
	while (pEntity = pEntityIt->Next())
	{
		if (pEntity->GetFlags() & ENTITY_FLAG_NEVER_NETWORK_STATIC)
		{
			entitiesToRemove.push_back(pEntity->GetId());
			CryLog("    found dynamic entity '%s'", pEntity->GetName());
		}
		else
		{
			CItem *pItem = static_cast<CItem*>(pItemSystem->GetItem(pEntity->GetId()));
			if (pItem)
			{
				// Need to reset owner on static items since they will be given to players again once we've rejoined
				pItem->SetOwnerId(0);
			}
		}
	}

	int numEntities = entitiesToRemove.size();
	CryLog("    removing %i entities", numEntities);
	for (int i = 0; i < numEntities; ++ i)
	{
		EntityId entId = entitiesToRemove[i];
		gEnv->pEntitySystem->RemoveEntity(entId, true);
	}

	// Clear teams
	for (TPlayerTeamIdMap::iterator it=m_playerteams.begin(); it!=m_playerteams.end(); ++ it)
	{
		it->second.resize(0);
	}
	m_entityteams.clear();

	CryLog("Host Migration: CGameRules::OnDemoteToClient() finished");

	return true;
}

//------------------------------------------------------------------------
bool CGameRules::OnPromoteToServer(SHostMigrationInfo& hostMigrationInfo)
{
	CryLog("Host Migration: CGameRules::OnPromoteToServer() started");
	IItemSystem *pItemSystem = g_pGame->GetIGameFramework()->GetIItemSystem();

	IEntityIt *it = gEnv->pEntitySystem->GetEntityIterator();
	it->MoveFirst();

	for (int i = 0; i < m_hostMigrationItemMaxCount; ++ i)
	{
		m_pHostMigrationItemInfo[i].Reset();
	}
	int itemIndex = 0;

	IEntity *pEntity = NULL;
	while (pEntity = it->Next())
	{
		IItem *pItem = pItemSystem->GetItem(pEntity->GetId());
		if (pItem)
		{
			if (pItem->GetOwnerId())
			{
				IEntity *pOwner = gEnv->pEntitySystem->GetEntity(pItem->GetOwnerId());
				if (pOwner)
				{
					EntityId currentItemId = 0;

					IActor *pOwnerActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pOwner->GetId());
					if (pOwnerActor)
					{
						IItem *pCurrentItem = pOwnerActor->GetCurrentItem();
						currentItemId = pCurrentItem ? pCurrentItem->GetEntityId() : 0;
					}

					CryLog("[CG] Item '%s' is owned by '%s'", pEntity->GetName(), pOwner->GetName());
					m_pHostMigrationItemInfo[itemIndex].Set(pEntity->GetId(), pOwner->GetId(), pItem->IsUsed(), (pItem->GetEntityId() == currentItemId));
					itemIndex ++;
					if (itemIndex >= m_hostMigrationItemMaxCount)
					{
						CRY_ASSERT(itemIndex < m_hostMigrationItemMaxCount);
						break;
					}
				}
			}
		}
		// Tell entities that we're host migrating
		// - Currently only used by ForbiddenArea but may well be needed for other entities later
		// - Currently only called on the new server, add to OnDemoteToClient if we need to use this on a client
		IScriptTable *pScript = pEntity->GetScriptTable();
		if (pScript && pScript->GetValueType("OnHostMigration") == svtFunction)
		{
			m_pScriptSystem->BeginCall(pScript, "OnHostMigration");
			m_pScriptSystem->PushFuncParam(pScript);
			m_pScriptSystem->PushFuncParam(true);
			m_pScriptSystem->EndCall();
		}
	}

	if (GetAssistScoringModule())
	{
		GetAssistScoringModule()->SvBecomeServer();
	}

	// This needs initialising on the new server otherwise the respawn timer will be counting down
	// from uninitialised data.  Likewise for the pre-round timer.
	ResetReviveCycleTime();
	ResetPreRoundTime();

	CryLog("Host Migration: CGameRules::OnPromoteToServer() finished");
	return true;
}

//------------------------------------------------------------------------
bool CGameRules::OnReconnectClient(SHostMigrationInfo& hostMigrationInfo)
{
	CryLog("Host Migration: CGameRules::OnReconnectClient() started");
	if (hostMigrationInfo.IsNewHost())
	{
		// Can't use gamerules cached version of server time since this function will be called before the Update()
		m_gameStartedTime.SetValue(m_gameStartedTime.GetValue() + g_pGame->GetIGameFramework()->GetServerTime().GetValue());
	}

	CryLog("Host Migration: CGameRules::OnReconnectClient() finished");
	return true;
}

//------------------------------------------------------------------------
bool CGameRules::OnFinalise(SHostMigrationInfo& hostMigrationInfo)
{
	CryLog("Host Migration: CGameRules::OnFinalise() started");
	if (g_pGame->GetIGameFramework()->GetClientActor())
	{
		CryLog("Host Migration: CGameRules::OnFinalise() finished");
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
void CGameRules::OnEntityEvent( IEntity *pEntity, SEntityEvent &event )
{
	if (event.event == ENTITY_EVENT_DONE)
	{
		if (!gEnv->bServer)
		{
			ClDoSetTeam(0, pEntity->GetId());
		}
		else
		{
			SetTeam(0, pEntity->GetId());
		}
	}
}

//------------------------------------------------------------------------
void CGameRules::OwnClientConnected_NotifyListeners()
{
	int numListeners = m_clientConnectionListeners.size();
	for (int i = 0; i < numListeners; ++ i)
	{
		m_clientConnectionListeners[i]->OnOwnClientEnteredGame();
	}
}

//------------------------------------------------------------------------
void CGameRules::OnResumeAfterHostMigrating()
{
	if (gEnv->bServer)
	{
		TPlayers players;
		GetPlayers(players);

		const int numPlayers = players.size();
		for (int i = 0; i < numPlayers; ++ i)
		{
			IActor *pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(players[i]);
			if (pActor)
			{
				pActor->SetMigrating(false);
			}
		}
	}
	else
	{
		IActor *pActor = g_pGame->GetIGameFramework()->GetClientActor();
		if (pActor)
		{
			pActor->SetMigrating(false);
		}
	}

	CHUD::CallEvent( SHUDEvent(eHUDEvent_OnGameStart) );

	gEnv->pEntitySystem->PauseTimers(false, true);
	gEnv->pTimer->PauseTimer(ITimer::ETIMER_GAME, false);

	FreezeInput(false);
}

//------------------------------------------------------------------------
float CGameRules::GetRemainingHostMigrationTimeoutTime() const
{
	const float curTime = gEnv->pTimer->GetAsyncCurTime();
	const float timePassed = curTime - m_hostMigrationTimeStateChanged;
	const float timeRemaining = g_pGameCVars->g_hostMigrationConnectionTimeout - timePassed;
	return MAX(timeRemaining, 0.f);
}

//------------------------------------------------------------------------
float CGameRules::GetHostMigrationTimeTillResume() const
{
	float timeRemaining = 0.f;
	if (m_hostMigrationState == eHMS_WaitingForPlayers)
	{
		timeRemaining = GetRemainingHostMigrationTimeoutTime() + g_pGameCVars->g_hostMigrationResumeTime;
	}
	else if (m_hostMigrationState == eHMS_Resuming)
	{
		const float curTime = gEnv->pTimer->GetAsyncCurTime();
		const float timePassed = curTime - m_hostMigrationTimeStateChanged;
		timeRemaining = MAX(g_pGameCVars->g_hostMigrationResumeTime - timePassed, 0.f);
	}
	return timeRemaining;
}

//------------------------------------------------------------------------
void CGameRules::FinishHostMigrating()
{
	// Assume remaining players aren't going to make it
	const uint32 maxMigratingPlayers = m_migratingPlayerMaxCount;
	for (uint32 index = 0; index < maxMigratingPlayers; ++ index)
	{
		SMigratingPlayerInfo *pPlayerInfo = &m_pMigratingPlayerInfo[index];
		if (pPlayerInfo->InUse())
		{
			// Pretend the player has disconnected
			CRY_TODO(09, 02, 2010, "Deprecate one of these listeners");
			ClientDisconnect_NotifyListeners(pPlayerInfo->m_originalEntityId);
			const int numListeners = m_clientConnectionListeners.size();
			for (int i = 0; i < numListeners; ++ i)
			{
				m_clientConnectionListeners[i]->OnClientDisconnect(-1, pPlayerInfo->m_originalEntityId);
			}
			// Remove the actor
			gEnv->pEntitySystem->RemoveEntity(pPlayerInfo->m_originalEntityId);
			pPlayerInfo->Reset();
		}
	}

	SHUDEvent hideHostMigration(eHUDEvent_HideHostMigrationScreen);
	CHUD::CallEvent(hideHostMigration);

	m_hostMigrationState = eHMS_Resuming;
	m_hostMigrationTimeStateChanged = gEnv->pTimer->GetAsyncCurTime();

	GetGameObject()->InvokeRMI(ClHostMigrationFinished(), NoParams(), eRMI_ToRemoteClients);
}

//------------------------------------------------------------------------
int CGameRules::GetLivingPlayerCount() const
{
	TPlayers players;
	GetPlayers(players);

	int numLivingPlayers = 0;
	IActorSystem *pActorSystem = g_pGame->GetIGameFramework()->GetIActorSystem();
	const int numPlayers = players.size();
	for (int i = 0; i < numPlayers; ++ i)
	{
		CPlayer *pPlayer = static_cast<CPlayer*>(pActorSystem->GetActor(players[i]));
		if (pPlayer)
		{
			if ((pPlayer->GetSpectatorState() == CActor::eASS_Ingame) && (pPlayer->GetSpectatorMode() == CActor::eASM_None) && (pPlayer->GetHealth() > 0))
			{
				++ numLivingPlayers;
			}
		}
	}

	return numLivingPlayers;
}






//----------------------------------------Diesel new--------------------------------
/*
void CGameRules::FreezeEntity(EntityId entityId, bool freeze, bool vapor, bool force)
{
	if (IsFrozen(entityId)==freeze)
	{
		if (freeze) // need to refresh the timer
		{
			TFrozenEntities::iterator it=m_frozen.find(entityId);
			if (it!=m_frozen.end())
				it->second=gEnv->pTimer->GetFrameStartTime();
		}

		return;
	}

	IEntity* pEntity = m_pEntitySystem->GetEntity(entityId);
	if (!pEntity)
		return;

	IGameObject *pGameObject=m_pGameFramework->GetGameObject(entityId);
	IScriptTable *pScriptTable=pEntity->GetScriptTable();
	IActor *pActor=m_pGameFramework->GetIActorSystem()->GetActor(entityId);

	if (freeze && gEnv->bServer)
	{
		// don't freeze if ai doesn't want to
		if (pActor && !pActor->IsPlayer() && gEnv->pAISystem)
		{
			IEntity *pObject=0;
			if (gEnv->pAISystem->GetSmartObjectManager()->SmartObjectEvent("DontFreeze", pEntity, pObject, 0))
				return;

			//Check also for invulnerable flag (just in case)
			SmartScriptTable props;
			if(pScriptTable && pScriptTable->GetValue("Properties", props))
			{
				int invulnerable = 0;
				if(props->GetValue("bInvulnerable", invulnerable) && invulnerable!=0)
					return;
			}
		}	

    // if entity implements "GetFrozenAmount", check it and freeze only when >= 1
    HSCRIPTFUNCTION pfnGetFrozenAmount=0;
    if (!force && pScriptTable && pScriptTable->GetValue("GetFrozenAmount", pfnGetFrozenAmount))
    {
      float frost = 1.0f;
      Script::CallReturn(gEnv->pScriptSystem, pfnGetFrozenAmount, pScriptTable, frost);
      gEnv->pScriptSystem->ReleaseFunc(pfnGetFrozenAmount);

      if (g_pGameCVars->cl_debugFreezeShake)
        CryLog("%s frost amount: %.2f", pEntity->GetName(), frost);

      if (frost < 0.99f)
        return;
    }
	}

	// call script event
	if (pScriptTable && pScriptTable->GetValueType("OnPreFreeze")==svtFunction)
	{
		bool ret=false;
		HSCRIPTFUNCTION func=0;
		pScriptTable->GetValue("OnPreFreeze", func);
		if (Script::CallReturn(pScriptTable->GetScriptSystem(), func, pScriptTable, freeze, vapor, ret) && !ret)
			return;
	}

	// send event to game object
	if (pGameObject)
	{
		SGameObjectEvent event(eCGE_PreFreeze, eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, (void *)freeze);
		pGameObject->SendEvent(event);
	}

	{
		// apply frozen material layer
		IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER);
		if (pRenderProxy)
		{
			uint8 activeLayers = pRenderProxy->GetMaterialLayersMask();
			if (freeze)
			{
				activeLayers |= MTL_LAYER_FROZEN;
			}
			else
				activeLayers &= ~MTL_LAYER_FROZEN;

			gEnv->p3DEngine->DeleteEntityDecals(pRenderProxy->GetRenderNode()); // remove any decals on this entity
			pRenderProxy->SetMaterialLayersMask(activeLayers);
		}

		// set the ice physics material
		
		IPhysicalEntity *pPhysicalEntity = pEntity->GetPhysics();

		int matId = -1;
		if (ISurfaceType *pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeByName("mat_ice"))
			matId=pSurfaceType->GetId();
		
		if (pPhysicalEntity && matId>-1)
		{
			pe_status_nparts status_nparts;
			int nparts = pPhysicalEntity->GetStatus(&status_nparts);
			if (nparts)
			{
				for (int i=0; i<nparts; i++)
				{
					pe_params_part part,partset;
					part.ipart = partset.ipart = i;

					if (!pPhysicalEntity->GetParams(&part))
						continue;
					if (!part.pMatMapping || !part.nMats)
						continue;

					if (freeze)
					{
						if (part.pPhysGeom)
						{
							static int map[255];
							for (int m=0; m<part.nMats; m++)
								map[m]=matId;
							partset.pMatMapping = map;
							partset.nMats = part.nMats;
						}
					}
					else if (part.pPhysGeom)
					{
						partset.pMatMapping = part.pPhysGeom->pMatMapping;
						partset.nMats = part.pPhysGeom->nMats;
					}

					pPhysicalEntity->SetParams(&partset);
				}
			}

			pe_action_awake awake;
			awake.bAwake=1;
			if (pPhysicalEntity)
				pPhysicalEntity->Action(&awake);
		}

		// freeze children
		int n=pEntity->GetChildCount();
		for (int i=0; i<n;i++)
		{
			IEntity *pChild=pEntity->GetChild(i);

			// don't freeze players attached to entities (vehicles)
			if (IActor *pActor=m_pGameFramework->GetIActorSystem()->GetActor(pChild->GetId()))
			{
				if (pActor && pActor->IsPlayer())
					continue;
			}

			FreezeEntity(pChild->GetId(), freeze, vapor);
		}
	}

	if (freeze)
	{
		std::pair<TFrozenEntities::iterator, bool> result=m_frozen.insert(TFrozenEntities::value_type(entityId, CTimeValue(0.0f)));
		result.first->second=gEnv->pTimer->GetFrameStartTime();
	}
	else
		m_frozen.erase(entityId);

	// send event to game object
	if (pGameObject)
	{
		SGameObjectEvent event(eCGE_PostFreeze, eGOEF_ToAll, IGameObjectSystem::InvalidExtensionID, (void *)freeze);
		pGameObject->SendEvent(event);
	}

	// call script event
	if (pScriptTable && pScriptTable->GetValueType("OnPostFreeze")==svtFunction)
		Script::CallMethod(pScriptTable, "OnPostFreeze", freeze);

	if (gEnv->bClient)
	{
		// spawn the vapor
		if (freeze && vapor)
		{
			SpawnParams params;
			params.eAttachForm=GeomForm_Surface;
			params.eAttachType=GeomType_Physics;
			params.bIgnoreLocation=true;

			gEnv->pEntitySystem->GetBreakableManager()->AttachSurfaceEffect(pEntity, 0, SURFACE_BREAKAGE_TYPE("freeze_vapor"), params);
		}
	}


	if (gEnv->bServer)
		GetGameObject()->InvokeRMIWithDependentObject(ClFreezeEntity(), FreezeEntityParams(entityId, freeze, vapor), eRMI_ToRemoteClients, entityId);
}
*/
//Diesel new