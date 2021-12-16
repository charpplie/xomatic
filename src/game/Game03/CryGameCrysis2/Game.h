/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: 
  
 -------------------------------------------------------------------------
  History:
  - 3:8:2004   11:23 : Created by Márcio Martins

*************************************************************************/
#ifndef __GAME_H__
#define __GAME_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IGame.h>
#include <IGameFramework.h>
#include <IGameObjectSystem.h>
#include <IGameObject.h>
#include <IActorSystem.h>
#include <StlUtils.h>
#include "ClientGameTokenSynch.h"
#include "ServerGameTokenSynch.h"

#include "ClientSynchedStorage.h"
#include "ServerSynchedStorage.h"
#include "OrganicMotion/OrganicMotionClient.h"
#include "PlayerVisTable.h"


#define GAME_NAME				"Crysis"
#define GAME_LONGNAME		"Crysis"

struct ISystem;
struct IConsole;

class	CScriptBind_Actor;
class CScriptBind_Item;
class CScriptBind_Weapon;
class CScriptBind_GameRules;
class CScriptBind_Game;
class CScriptBind_GameAI;
class CScriptBind_HUD;
class CScriptBind_HitDeathReactions;
class CScriptBind_Boids;
class CScriptBind_InteractiveObject;
class CWeaponSystem;
class CFlashMenuObject;
class CFlashFrontEnd;
class COptionsManager;
class CProfileOptions;
class CTacticalManager;
class CWarningsManager;
class CTelemetryBuffer;
class CGameMechanismManager;

struct IActionMap;
struct IActionFilter;
class  CGameActions;
class CGameRules;
class CBulletTime;
class ServerGameTokenSynch;
class ClientGameTokenSynch;
class CSynchedStorage;
class CClientSynchedStorage;
class CServerSynchedStorage;
struct SCVars;
struct SItemStrings;
class CGameSharedParametersStorage;
class CSPAnalyst;
class CPersistantStats;
class CGameAudio;
class CLaptopUtil;
class CScreenEffects;
class CLedgeManager;
class CPerkSonarVisionManager;
class CRecordingSystem;
class CHUDMissionObjectiveSystem; // TODO : Remove me?
class CGameBrowser;
class CGameLobby;
class CCryLobbySessionHandler;
class CHUD;
class CStatsRecordingMgr;
class CAutoAimManager;
class CHitDeathReactionsSystem;
struct ITelemetryCollector;

class CInteractiveObjectRegistry;

class CBurnEffectManager;

#if !defined(_RELEASE)
class CGameRealtimeRemoteUpdateListener;
#endif

namespace Graphics
{
	class CColorGradientManager;
//	class CScreenFader;
//	class CPostEffectBlender;
}


// when you add stuff here, also update in CGame::RegisterGameObjectEvents
enum ECryGameEvent
{
	eCGE_OnShoot = 256,
	eCGE_BeginReloadLoop,
	eCGE_EndReloadLoop,
	eCGE_ActorRevive,
	eCGE_VehicleDestroyed,
	eCGE_TurnRagdoll,
	eCGE_EnableFallAndPlay,
	eCGE_DisableFallAndPlay,
	eCGE_VehicleTransitionEnter,
	eCGE_VehicleTransitionExit,
	eCGE_HUD_PDAMessage,
	eCGE_HUD_TextMessage,
	eCGE_TextArea,
	eCGE_HUD_Break,
	eCGE_HUD_Reboot,
	eCGE_InitiateAutoDestruction,
	eCGE_Event_Collapsing,
	eCGE_Event_Collapsed,
	eCGE_MultiplayerChatMessage,
	eCGE_ResetMovementController,
	eCGE_AnimateHands,
	eCGE_Ragdoll,
	eCGE_EnablePhysicalCollider,
	eCGE_DisablePhysicalCollider,
	eCGE_RebindAnimGraphInputs,
  eCGE_Turret_LockedTarget,
  eCGE_Turret_LostTarget,
	eCGE_SetTeam,
	eCGE_Launch,
	eCGE_ReactionEnd,
	eCGE_CoverTransitionEnter,
	eCGE_CoverTransitionExit,
};

//! Difficulty levels
enum EDifficulty
{
	eDifficulty_Default = 0,

	eDifficulty_Easy,
	eDifficulty_Normal,
	eDifficulty_Hard,
	eDifficulty_Delta,

	eDifficulty_COUNT,
};

//! Platform defines
enum EPlatform
{
	ePlatform_Unknown = 0,
	ePlatform_PC,
	ePlatform_Xbox,
	ePlatform_PS3,

	ePlatform_COUNT,
};

static const int GLOBAL_SERVER_IP_KEY						=	1000;
static const int GLOBAL_SERVER_PUBLIC_PORT_KEY	= 1001;
static const int GLOBAL_SERVER_NAME_KEY					=	1002;

class CGame :
  public IGame, public IGameFrameworkListener, public IInputEventListener
{
public:
  typedef bool (*BlockingConditionFunction)();
public:
	CGame();
	virtual ~CGame();

	// IGame
	virtual bool  Init(IGameFramework *pFramework);
	virtual bool  CompleteInit();
	virtual void  Shutdown();
	virtual int   Update(bool haveFocus, unsigned int updateFlags);
	virtual void  ConfigureGameChannel(bool isServer, IProtocolBuilder *pBuilder);
	virtual void  EditorResetGame(bool bStart);
	virtual void  PlayerIdSet(EntityId playerId);
	virtual string  InitMapReloading();
	virtual bool IsReloading() { return m_bReload; }
	virtual IGameFramework *GetIGameFramework() { return m_pFramework; }

	virtual const char *GetLongName();
	virtual const char *GetName();
	virtual EPlatform GetPlatform() const;

	virtual void UploadSessionTelemetry(void);
	virtual void ClearSessionTelemetry(void);
	virtual void GetMemoryStatistics(ICrySizer * s);

	virtual void OnClearPlayerIds();
	//auto-generated save game file name
	virtual const char* CreateSaveGameName();
	//level names were renamed without changing the file/directory
	virtual const char* GetMappedLevelName(const char *levelName) const;
	// 
	virtual IGameStateRecorder* CreateGameStateRecorder(IGameplayListener* pL);

	virtual const bool DoInitialSavegame() const { return true; }

	// ~IGame

  // IGameFrameworkListener
  virtual void OnPostUpdate(float fDeltaTime);
  virtual void OnSaveGame(ISaveGame* pSaveGame);
  virtual void OnLoadGame(ILoadGame* pLoadGame);
	virtual void OnLevelEnd(const char* nextLevel);;
  virtual void OnActionEvent(const SActionEvent& event);
  // ~IGameFrameworkListener

	bool OnInputEvent(const SInputEvent& inputEvent);
	void SetExclusiveControllerFromPreviousInput();
	unsigned int GetExclusiveControllerDeviceIndex() const { return m_exclusiveControllerDeviceIndex; }
	void RemoveExclusiveController();

	void InitGameType(bool multiplayer);

	//! Handle difficulty setting changes
	void SetDifficultyLevel(EDifficulty difficulty);

	//! Register action maps to local client at start of level
	void RegisterActionMaps();

	void BlockingProcess(BlockingConditionFunction f);
	void GameChannelDestroyed(bool isServer);
	void DestroyHUD() {};

	virtual CScriptBind_HitDeathReactions* GetHitDeathReactionsScriptBind() { return m_pScriptBindHitDeathReactions; }
	virtual CScriptBind_Actor *GetActorScriptBind() { return m_pScriptBindActor; }
	virtual CScriptBind_Item *GetItemScriptBind() { return m_pScriptBindItem; }
	virtual CScriptBind_Weapon *GetWeaponScriptBind() { return m_pScriptBindWeapon; }
	virtual CScriptBind_GameRules *GetGameRulesScriptBind() { return m_pScriptBindGameRules; }
	virtual CScriptBind_HUD *GetHUDScriptBind() { return m_pScriptBindHUD; }
	CScriptBind_InteractiveObject* GetInteractiveObjectScriptBind() { return m_pScriptBindInteractiveObject; }
	virtual CWeaponSystem *GetWeaponSystem() { return m_pWeaponSystem; };
	virtual CGameSharedParametersStorage *GetGameSharedParametersStorage() { return m_pGameParametersStorage; };
	virtual CRecordingSystem *GetRecordingSystem() { return m_pRecordingSystem; };
	CStatsRecordingMgr *GetStatsRecorder()			{ return m_statsRecorder; }
	ITelemetryCollector	*GetITelemetryCollector()	{ return m_telemetryCollector; }

	CLedgeManager*	GetLedgeManager() const { return m_pLedgeManager; };

	CGameActions&	Actions() const {	return *m_pGameActions;	};

	CGameRules *GetGameRules() const;
	CHUDMissionObjectiveSystem* GetMOSystem() const { return m_pMOSystem; }; 

	CLaptopUtil *GetLaptopUtil() const;
	virtual CGameAudio *GetGameAudio() const { return m_pGameAudio; }

	CFlashMenuObject *GetMenu() const; // To be removed

	ILINE CFlashFrontEnd *GetFlashMenu() const
	{
		return m_pFlashMenu;
	}

	COptionsManager *GetOptions() const;
	CProfileOptions *GetProfileOptions() const;
	CPlayerVisTable *GetPlayerVisTable() { return &m_playerVisTable; }
	CPersistantStats *GetPersistantStats() const { return m_pPersistantStats; }

	CScreenEffects *GetScreenEffects() const;
	CAutoAimManager& GetAutoAimManager() const { return *m_pAutoAimManager; }
	CHitDeathReactionsSystem& GetHitDeathReactionsSystem() const { CRY_ASSERT(m_pHitDeathReactionsSystem); return *m_pHitDeathReactionsSystem; }
	CInteractiveObjectRegistry& GetInteractiveObjectsRegistry() const;

	CBurnEffectManager* GetBurnEffectManager() const { return m_pBurnEffectManager; }

	ILINE CSynchedStorage *GetSynchedStorage() const
	{
		if (m_pServerSynchedStorage && gEnv->bServer)
			return m_pServerSynchedStorage;

		return m_pClientSynchedStorage;
	}

	ILINE CServerSynchedStorage *GetServerSynchedStorage() const
	{
		return m_pServerSynchedStorage;
	}

	ILINE CServerGameTokenSynch *GetServerGameTokenSynch() const
	{
		return m_pServerGameTokenSynch;
	}

	ILINE CClientGameTokenSynch *GetClientGameTokenSynch() const
	{
		return m_pClientGameTokenSynch;
	}

	CSPAnalyst* GetSPAnalyst() const { return m_pSPAnalyst; }

	const string& GetLastSaveGame(string &levelName);
	const string& GetLastSaveGame() { string tmp; return GetLastSaveGame(tmp); }

  ILINE SCVars *GetCVars() {return m_pCVars;}
	static void DumpMemInfo(const char* format, ...) PRINTF_PARAMS(1, 2);
	
	Graphics::CColorGradientManager& GetColorGradientManager();

	CGameLobby* GetGameLobby() { return m_pGameLobby; }
	CGameBrowser* GetGameBrowser() { return m_pGameBrowser; }
	void ClearGameSessionHandler() { m_pLobbySessionHandler = 0; }

	ILINE CHUD* GetHUD( void ){ assert(m_pHud); return m_pHud; }
	ILINE CTacticalManager* GetTacticalManager( void ) const { return m_pTacticalManager; }
	ILINE CWarningsManager* GetWarnings( void ) const { return m_pWarningsManager; }

	static void ExpandTimeSeconds(int secs, int& days, int& hours, int& minutes, int& seconds);




protected:

	//! Difficulty config loading helper
	class CDifficultyConfigSink : public ILoadConfigurationEntrySink
	{
	public:
		CDifficultyConfigSink(const char* who) : m_szWho(who) {}

		void OnLoadConfigurationEntry(const char *szKey, const char *szValue, const char *szGroup)
		{
			ICVar *pCVar = gEnv->pConsole->GetCVar(szKey);
			if (pCVar)
			{
				pCVar->ForceSet(szValue);
			}
			else
			{
				GameWarning("%s : Can only set existing CVars during loading (no commands!) (%s = %s)", 
					m_szWho ? m_szWho : "CDifficultyConfigSink", szKey, szValue);
			}
		}

	private:
		const char* m_szWho;
	};

	//! Platform information as defined in defaultProfile.xml
	struct SPlatformInfo
	{
		enum EDevices
		{
			eDevice_Keyboard = 0x01,
			eDevice_XboxPad = 0x02,
			eDevice_PS3Pad = 0x04,

			eDevice_All = (eDevice_Keyboard|eDevice_XboxPad|eDevice_PS3Pad),
		};

		EPlatform	platformId;
		BYTE		devices;				// Devices to use when registering actions

		SPlatformInfo(EPlatform _platformId = ePlatform_Unknown) : platformId(_platformId), devices(eDevice_All) {}
	};
	SPlatformInfo m_platformInfo;

	virtual void LoadActionMaps(const char* filename = "libs/config/defaultProfile.xml");

	// Registers the game specific animActions with CryAnimation
	virtual void RegisterAnimActions() {}

	virtual void ReleaseActionMaps();
	bool ReadProfile(XmlNodeRef &rootNode);
	bool ReadProfilePlatform(XmlNodeRef &platformsNode, EPlatform platformId);
	bool ReadProfileActionMap(XmlNodeRef &actionMapNode);

	virtual void InitScriptBinds();
	virtual void ReleaseScriptBinds();

	virtual void CheckReloadLevel();

	// These funcs live in GameCVars.cpp
	virtual void RegisterConsoleVars();
	virtual void RegisterConsoleCommands();
	virtual void UnregisterConsoleCommands();

	virtual void RegisterGameObjectEvents();

	// marcok: this is bad and evil ... should be removed soon
	static void CmdRestartGame(IConsoleCmdArgs *pArgs);
	static void CmdDumpAmmoPoolStats(IConsoleCmdArgs *pArgs);

	static void CmdDumpSS(IConsoleCmdArgs *pArgs);

	static void CmdLastInv(IConsoleCmdArgs *pArgs);
	static void CmdName(IConsoleCmdArgs *pArgs);
	static void CmdTeam(IConsoleCmdArgs *pArgs);
	static void CmdLoadLastSave(IConsoleCmdArgs *pArgs);
	static void CmdSpectator(IConsoleCmdArgs *pArgs);
	static void CmdJoinGame(IConsoleCmdArgs *pArgs);
	static void CmdKill(IConsoleCmdArgs *pArgs);
	static void CmdRevive(IConsoleCmdArgs *pArgs);
  static void CmdVehicleKill(IConsoleCmdArgs *pArgs);
	static void CmdRestart(IConsoleCmdArgs *pArgs);
	static void CmdSay(IConsoleCmdArgs *pArgs);
	static void CmdEcho(IConsoleCmdArgs *pArgs);
	static void CmdReloadItems(IConsoleCmdArgs *pArgs);
	static void CmdLoadActionmap(IConsoleCmdArgs *pArgs);
  static void CmdReloadGameRules(IConsoleCmdArgs *pArgs);
  static void CmdNextLevel(IConsoleCmdArgs* pArgs);
	static void CmdReloadHitDeathReactions(IConsoleCmdArgs* pArgs);
  static void CmdReloadSkillSystem(IConsoleCmdArgs* pArgs);
  static void CmdStartKickVoting(IConsoleCmdArgs* pArgs);
  static void CmdStartNextMapVoting(IConsoleCmdArgs* pArgs);
  static void CmdVote(IConsoleCmdArgs* pArgs);

  static void CmdQuickGame(IConsoleCmdArgs* pArgs);
  static void CmdQuickGameStop(IConsoleCmdArgs* pArgs);
  static void CmdBattleDustReload(IConsoleCmdArgs* pArgs);
  static void CmdLogin(IConsoleCmdArgs* pArgs);
	static void CmdLoginProfile(IConsoleCmdArgs* pArgs);
  static void CmdCryNetConnect(IConsoleCmdArgs* pArgs);

	static void CmdListAllRandomLoadingMessages(IConsoleCmdArgs* pArgs);
	
	static void CmdFreeCamEnable(IConsoleCmdArgs *pArgs);
	static void CmdFreeCamDisable(IConsoleCmdArgs *pArgs);
	static void CmdFreeCamLockCamera(IConsoleCmdArgs *pArgs);
	static void CmdFreeCamUnlockCamera(IConsoleCmdArgs *pArgs);
	static void CmdFlyCamSetPoint(IConsoleCmdArgs *pArgs);
	static void CmdFlyCamPlay(IConsoleCmdArgs *pArgs);

	static void CmdIgnoreAllAsserts(IConsoleCmdArgs *pArgs);

	static void CmdReloadPlayer(IConsoleCmdArgs* cmdArgs);
	static void CmdSetPlayerHealth(IConsoleCmdArgs* pArgs);
	static void CmdSpawnActorAtPos(IConsoleCmdArgs *pArgs);
	static void CmdSpawnDummyPlayer(IConsoleCmdArgs* cmdArgs);
	static void CmdRemoveDummyPlayers(IConsoleCmdArgs* cmdArgs);
	static void CmdSetDummyPlayerState(IConsoleCmdArgs* cmdArgs);

#if !defined(_RELEASE)
	// TODO: Move more console commands which shouldn't feature in final release into this block...
	static void CmdSendConsoleCommand(IConsoleCmdArgs *pArgs);
	static void CmdNetSetOnlineMode(IConsoleCmdArgs *pArgs);
#endif

#ifndef DISABLE_OM
public:
	//////////////////////////////////////////////////////////////////////////
	// organic motion data
	static void CmdOMConnect(IConsoleCmdArgs* pArgs);
	static void CmdOMDisconnect(IConsoleCmdArgs* pArgs);
	void				OMConnect(IConsoleCmdArgs* pArgs);
	void				OMDisconnect();
	void				OMUpdate();
	tOrganicMotionClient m_OrganicMotionClient;
	//////////////////////////////////////////////////////////////////////////
protected:
#endif

	
	IGameFramework			*m_pFramework;
	IConsole						*m_pConsole;

	CWeaponSystem				*m_pWeaponSystem;

	bool								m_bReload;

	// script binds
	CScriptBind_Actor*     m_pScriptBindActor;
	CScriptBind_Item*      m_pScriptBindItem;
	CScriptBind_Weapon*    m_pScriptBindWeapon;
	CScriptBind_GameRules* m_pScriptBindGameRules;
	CScriptBind_Game*      m_pScriptBindGame;
	CScriptBind_GameAI*    m_pScriptBindGameAI;
	CScriptBind_HUD*       m_pScriptBindHUD;
	CScriptBind_InteractiveObject* m_pScriptBindInteractiveObject;
	CScriptBind_HitDeathReactions* m_pScriptBindHitDeathReactions;
	CScriptBind_Boids* m_pScriptBindBoids;

	
	//vis table
	CPlayerVisTable			m_playerVisTable;

	//menus
	CFlashMenuObject		*m_pFlashMenuObject; // To be removed
	CFlashFrontEnd		  *m_pFlashMenu;
	COptionsManager			*m_pOptionsManager;
	CProfileOptions			*m_pProfileOptions;

	CRecordingSystem		*m_pRecordingSystem;
	CStatsRecordingMgr		*m_statsRecorder;
	ITelemetryCollector		*m_telemetryCollector;
	CTelemetryBuffer		*m_performanceBuffer;
	CTelemetryBuffer		*m_bandwidthBuffer;
	CTelemetryBuffer		*m_memoryTrackingBuffer;

	IActionMap					*m_pDefaultAM;
	IActionMap					*m_pMultiplayerAM;
	CGameActions				*m_pGameActions;	
	IPlayerProfileManager* m_pPlayerProfileManager;

	CServerSynchedStorage	*m_pServerSynchedStorage;
	CClientSynchedStorage	*m_pClientSynchedStorage;

	CClientGameTokenSynch	*m_pClientGameTokenSynch;
	CServerGameTokenSynch *m_pServerGameTokenSynch;

	CSPAnalyst          *m_pSPAnalyst;
	CPersistantStats		*m_pPersistantStats;
	bool								m_inDevMode;
	bool								m_gameTypeInited;

	bool				 m_hasExclusiveContoller;
	unsigned int m_exclusiveControllerDeviceIndex;
	unsigned int m_previousInputControllerDeviceIndex;

	EntityId m_uiPlayerID;

	SCVars*	m_pCVars;
	SItemStrings						*m_pItemStrings;
	CGameSharedParametersStorage *m_pGameParametersStorage;
	string                 m_lastSaveGame;
	string								 m_newSaveGame;

	CHUDMissionObjectiveSystem *m_pMOSystem;

	CLaptopUtil						*m_pLaptopUtil;
	CScreenEffects				*m_pScreenEffects;

	typedef std::map<string, string, stl::less_stricmp<string> > TLevelMapMap;
	TLevelMapMap m_mapNames;

	CGameAudio *m_pGameAudio;


	// Manager the ledges in the level (markup) that the player can grab onto
	CLedgeManager*	m_pLedgeManager;

	Graphics::CColorGradientManager* m_colorGradientManager;

	// Game side browser - searching for games
	CGameBrowser* m_pGameBrowser;
	// Game side Lobby handler
	CGameLobby* m_pGameLobby;
	// Game side session handler implementation
	CCryLobbySessionHandler* m_pLobbySessionHandler;

	// Game side HUD, only valid when client, 
	// only functions after player joins.
	CHUD* m_pHud;
	CTacticalManager*					m_pTacticalManager;
	CWarningsManager*					m_pWarningsManager;
	CAutoAimManager*					m_pAutoAimManager;
	CHitDeathReactionsSystem*	m_pHitDeathReactionsSystem;
	CBurnEffectManager*				m_pBurnEffectManager;
	CGameMechanismManager * m_gameMechanismManager;


	//CBitmapUi* m_pBitmapUi;
	//CFlashHUD* m_pFlashHUD;
	//CFlashMenu* m_pFlashMenu;


#if !defined(_RELEASE)
	// Live Create support for things that must/should be updated in the game
	// on the Main Thread.
	CGameRealtimeRemoteUpdateListener*	m_pRemoteUpdateListener;
#endif
};

extern CGame *g_pGame;

#define SAFE_HARDWARE_MOUSE_FUNC(func)\
	if(gEnv->pHardwareMouse)\
		gEnv->pHardwareMouse->func

#define SAFE_MENU_FUNC(func)\
	{	}

#define SAFE_MENU_FUNC_RET(func)\
	(0)

#define SAFE_HUD_FUNC(func)\
	{	}

#define SAFE_HUD_FUNC_RET(func)\
	(0)

#define SAFE_LAPTOPUTIL_FUNC(func)\
	{	if(g_pGame && g_pGame->GetLaptopUtil()) g_pGame->GetLaptopUtil()->func; }

#define SAFE_LAPTOPUTIL_FUNC_RET(func)\
	((g_pGame && g_pGame->GetLaptopUtil()) ? g_pGame->GetLaptopUtil()->func : NULL)


//#define CRAPDOLLS

#endif //__GAME_H__
