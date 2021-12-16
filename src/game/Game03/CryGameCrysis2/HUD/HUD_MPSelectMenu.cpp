#include "StdAfx.h"
#include "HUD_MPSelectMenu.h"
#include "Flash/Flash.h"
#include "FlashAnimation.h"
#include "HUD.h"
#include "HUD/HUDState.h"
#include "Game.h"
#include "GameRules.h"
#include "GameActions.h"
#include "IActionMapManager.h"
#include "EquipmentLoadout.h"
#include "Player.h"
#include "PlayerInput.h"
#include "PlayerProgression.h"
#include "HUD/HUD_Impl.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"
#include "FrontEnd/FlashFrontEnd.h"

//------------------------------------------------------------------------
CHUD_MPSelectMenu::CHUD_MPSelectMenu()
{
	if (gEnv->pInput)
	{
		gEnv->pInput->AddEventListener(this);
	}
	SAFE_HARDWARE_MOUSE_FUNC(AddListener(this));

	g_pGame->GetIGameFramework()->GetILevelSystem()->AddListener(this);

	m_state = eMS_None;
	m_bVisible = false;
	m_bGameEnded = false;
	m_bHostMigrating = false;
	m_bHostMigratingNeedsUpdate = false;

	m_pFlashButtonHandler = new CFlashMenuButtonHandler();

	m_gameModeString.Format("@ui_rules_%s", g_pGame->GetGameRules() ? g_pGame->GetGameRules()->GetEntity()->GetClass()->GetName() : "");
	m_gameModeString = g_pGame->GetHUD()->LocalizeString(m_gameModeString.c_str());

	// Level available on restart, but on level load wait for - OnLoadingStart();
	if (ILevel *pLevel = g_pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel())
	{
		if (ILevelInfo *pLevelInfo = pLevel->GetLevelInfo())
		{
			m_levelNameString = pLevelInfo->GetDisplayName();
		}
	}
}

//------------------------------------------------------------------------
CHUD_MPSelectMenu::~CHUD_MPSelectMenu()
{
	if (m_bVisible)
	{
		ShowMenu(false, true);
	}

	if (gEnv->pInput)
	{
		gEnv->pInput->RemoveEventListener(this);
	}
	SAFE_HARDWARE_MOUSE_FUNC(RemoveListener(this));

	g_pGame->GetIGameFramework()->GetILevelSystem()->RemoveListener(this);

	IHUDAsset *selctMenu = GetAsset("selectMenu"); // can return CHUD::s_pHUDAssetNULL
	IFlashPlayer *pFlashPlayer = selctMenu->GetFlashPlayer();
	pFlashPlayer->SetFSCommandHandler(NULL);

	SAFE_DELETE(m_pFlashButtonHandler);
}

//------------------------------------------------------------------------
void CHUD_MPSelectMenu::OnLoadingStart(ILevelInfo *pLevel)
{
	if(pLevel)
	{
		m_levelNameString = pLevel->GetDisplayName();
	}
}

//------------------------------------------------------------------------
void CHUD_MPSelectMenu::Init()
{
	IHUDAsset *selctMenu = GetAsset("selectMenu");
	IFlashPlayer *pFlashPlayer = selctMenu->GetFlashPlayer();
	if (m_pFlashButtonHandler)
	{
		pFlashPlayer->SetFSCommandHandler(this);
	}

#if defined(XENON)
	const char* strPlatform = "xbox";
#elif defined(PS3)
	const char* strPlatform ="ps3";
#else
	const char* strPlatform ="pc";
#endif
	selctMenu->Invoke("setGamePlatform", strPlatform);
}

//------------------------------------------------------------------------
void CHUD_MPSelectMenu::Update(float frameTime)
{
	if (m_bVisible)
	{
		if (m_bHostMigrating)
		{
			UpdateHostMigrationState();
		}

		GetAsset("selectMenu")->Update(frameTime);
	}
}

//------------------------------------------------------------------------
void CHUD_MPSelectMenu::Draw()
{
	if (m_bVisible)
	{
		GetAsset("selectMenu")->Draw();
	}
}

//------------------------------------------------------------------------
void CHUD_MPSelectMenu::OnHUDEvent(const SHUDEvent& event)
{
	if(event.eventType == eHUDEvent_OpenedIngameMenu)
	{
		ShowMenu(false, true);
	}
	else if(event.eventType == eHUDEvent_ClosedIngameMenu)
	{
		if (m_bGameEnded)
		{
			ShowMenu(true, true, eOM_EndScoreboard);
		}
		if (m_bHostMigrating)
		{
			ShowMenu(true, true, eOM_HostMigration);
		}
	}
	else if(event.eventType == eHUDEvent_OnHUDReload)
	{
			SHUDEvent toggleEvent(eHUDEvent_HideSelectMenu);
			CHUD::CallEvent(toggleEvent);

			ShowMenu(false, true);
			m_bGameEnded = false;
	}
	else if(event.eventType == eHUDEvent_ToggleMap )
	{
		ShowMenu(!m_bVisible, true, eOM_Map);
		SAFE_HARDWARE_MOUSE_FUNC(SetHardwareMouseClientPosition(gEnv->pRenderer->GetWidth() * 0.5f, gEnv->pRenderer->GetHeight() * 0.5f));
	}
	else if(event.eventType == eHUDEvent_ToggleSelectMenu)
	{
		SHUDEvent toggleEvent;
		if( m_bVisible )
		{
			toggleEvent.eventType = eHUDEvent_HideSelectMenu;
		}
		else
		{
			toggleEvent.eventType = eHUDEvent_ShowSelectMenu;
		}
		CHUD::CallEvent(toggleEvent);

	}
	else if(event.eventType == eHUDEvent_ShowSelectMenu)
	{
		ShowMenu(true);
	}
	else if(event.eventType == eHUDEvent_HideSelectMenu)
	{
		ShowMenu(false);
	}
	else if(event.eventType == eHUDEvent_GameEnded)
	{
		m_bGameEnded = true;
		CFlashFrontEnd *pFlashMenu = g_pGame->GetFlashMenu();
		if (pFlashMenu)
		{
			if (pFlashMenu->IsMenuActive(CFlashFrontEnd::eFlM_IngameMenu) == false)
			{
				ShowMenu(true, true, eOM_EndScoreboard);
			}
		}

		SendEndStatsToFlash();
	}
	else if (event.eventType == eHUDEvent_ShowHostMigrationScreen)
	{
		InitHostMigration();
	}
	else if (event.eventType == eHUDEvent_HideHostMigrationScreen)
	{
		FinishHostMigration();
	}
	else if (event.eventType == eHUDEvent_HostMigrationOnNewPlayer)
	{
		if (m_bHostMigrating)
		{
			EntityId playerId = event.GetData(0).GetInt();
			HostMigrationOnNewPlayer(playerId);
		}
	}
}

//------------------------------------------------------------------------
void CHUD_MPSelectMenu::OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta/* = 0*/)
{
	IHUDAsset *selctMenu = GetAsset("selectMenu");
	IFlashPlayer *flashPlayer = selctMenu->GetFlashPlayer();
	if (m_bVisible)
	{
		SFlashCursorEvent::ECursorState eCursorState = SFlashCursorEvent::eCursorMoved;
		if(HARDWAREMOUSEEVENT_LBUTTONDOWN == eHardwareMouseEvent || HARDWAREMOUSEEVENT_LBUTTONDOUBLECLICK == eHardwareMouseEvent)
		{
			eCursorState = SFlashCursorEvent::eCursorPressed;
		}
		else if(HARDWAREMOUSEEVENT_LBUTTONUP == eHardwareMouseEvent)
		{
			eCursorState = SFlashCursorEvent::eCursorReleased;
		}

		int x(iX), y(iY);
		flashPlayer->ScreenToClient(x,y);
		flashPlayer->SendCursorEvent(SFlashCursorEvent(eCursorState,x,y));
	}
}

void CHUD_MPSelectMenu::ShowMenu(bool onOff, bool force/*=false*/, EOpenMenu menu/*=eOM_Main*/)
{
	bool sameState = (m_bVisible == onOff);

	if (force == false)
	{
		if (sameState)
			return;

		if (onOff==false && (m_bGameEnded || m_bHostMigrating))
		{
			return; // Don't allow closing on the endgame or during a host migration
		}
	}

	IHUDAsset *selectMenu = GetAsset("selectMenu");
	m_bVisible = onOff;

	if (gEnv->pInput) gEnv->pInput->ClearKeyState();

	const CGameActions& actions = g_pGame->Actions();
	if (IActionFilter* selectMenuFilter = actions.FilterNoMPSelectMenuOpen())
	{
		selectMenuFilter->Enable(onOff);
	}

	ChangeMenuState(eMS_None);
	if (onOff)
	{
		if (m_pFlashButtonHandler)
		{
			m_pFlashButtonHandler->SetLastInputDevice(eDI_Unknown);
		}

		switch (menu)
		{
			case eOM_Main:
				{
					selectMenu->Invoke("openMenuName", SFlashVarValue("main"));
				}
				break;

			case eOM_EndScoreboard:
				{
					selectMenu->Invoke("openMenuName", SFlashVarValue("endScoreboard"));
					ChangeMenuState(eMS_Scoreboard);
				}
				break;
			case eOM_Map:
				{
					selectMenu->Invoke("openMenuName", SFlashVarValue("fullScreenMap"));
				}
				break;
			case eOM_HostMigration:
				{
					selectMenu->Invoke("openMenuName", SFlashVarValue("hostMigration"));
				}
				break;
		};

		// Player Progression
		CPlayerProgression *pPlayerProgression = CPlayerProgression::GetInstance();
		if (pPlayerProgression)
		{
			EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
			if (IEntity *pPlayer = gEnv->pEntitySystem->GetEntity(localActorId))
			{
				SFlashVarValue ppArgs[3] = { pPlayer->GetName(), pPlayerProgression->GetData(EPP_Rank), pPlayerProgression->GetData(EPP_XP) };
				selectMenu->Invoke("SetPlayerProgression", ppArgs, 3);
			}
		}

		// Game info
		SFlashVarValue gameInfoArgs[2] = { m_gameModeString.c_str(), m_levelNameString.c_str() };
		selectMenu->Invoke("SetGameInfo", gameInfoArgs, 2);

		CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
		if(pPlayer && pPlayer->GetPlayerInput())
			pPlayer->GetPlayerInput()->ClearXIMovement();

		if (!sameState)
		{
			SAFE_HARDWARE_MOUSE_FUNC(IncrementCounter());

			g_pGame->GetHUD()->ActivateState("mp_menus");

			g_pGame->GetHUD()->GetHUDImpl()->ShowHUD(false, eHideHUDReason_General);
		}
	}
	else
	{
		if (!sameState)
		{
			SAFE_HARDWARE_MOUSE_FUNC(DecrementCounter());

			g_pGame->GetHUD()->ActivateDefaultState();

			g_pGame->GetHUD()->GetHUDImpl()->ShowHUD(true, eHideHUDReason_General);
		}
	}
}

//------------------------------------------------------------------------
bool CHUD_MPSelectMenu::OnInputEvent(const SInputEvent &rInputEvent)
{
	if (m_bVisible)
	{
		IHUDAsset *selctMenu = GetAsset("selectMenu");
		IFlashPlayer *pFlashPlayer = selctMenu->GetFlashPlayer();
		if (m_pFlashButtonHandler && (rInputEvent.state == eIS_Pressed))
		{
			bool move = false;
			const char* direction = "";
			Vec2 dirvec(0,0);

			if(rInputEvent.keyId == eKI_Up || rInputEvent.keyId == eKI_XI_DPadUp || rInputEvent.keyId == eKI_PS3_Up /*|| rInputEvent.keyId == eKI_XI_ThumbLUp*/)
			{
				move = true;
				direction = "up";
				dirvec = Vec2(0,-1);
			}
			else if(rInputEvent.keyId == eKI_Down || rInputEvent.keyId == eKI_XI_DPadDown || rInputEvent.keyId == eKI_PS3_Down /*|| rInputEvent.keyId == eKI_XI_ThumbLDown*/)
			{
				move = true;
				direction = "down";
				dirvec = Vec2(0,1);
			}
			else if(rInputEvent.keyId == eKI_Left || rInputEvent.keyId == eKI_XI_DPadLeft || rInputEvent.keyId == eKI_PS3_Left /*|| rInputEvent.keyId == eKI_XI_ThumbLLeft*/)
			{
				move = true;
				direction = "left";
				dirvec = Vec2(-1,0);
			}
			else if(rInputEvent.keyId == eKI_Right || rInputEvent.keyId == eKI_XI_DPadRight || rInputEvent.keyId == eKI_PS3_Right /*|| rInputEvent.keyId == eKI_XI_ThumbLRight*/)
			{
				move = true;
				direction = "right";
				dirvec = Vec2(1,0);
			}

			if (move)
			{
				if (!m_pFlashButtonHandler->CheckSnapToNextButton(direction))
					move = false;
			}

			if (rInputEvent.deviceId == eDI_XI)
			{
				if(rInputEvent.keyId == eKI_XI_DPadUp)
					pFlashPlayer->Invoke("onUp", NULL, 0);
				else if(rInputEvent.keyId == eKI_XI_DPadDown)
					pFlashPlayer->Invoke("onDown", NULL, 0);
				else if(rInputEvent.keyId == eKI_XI_DPadLeft || rInputEvent.keyId == eKI_PS3_Left || rInputEvent.keyId == eKI_XI_ThumbLLeft)
					pFlashPlayer->Invoke("onLeft", NULL, 0);
				else if(rInputEvent.keyId == eKI_XI_DPadRight || rInputEvent.keyId == eKI_PS3_Right || rInputEvent.keyId == eKI_XI_ThumbLRight)
					pFlashPlayer->Invoke("onRight", NULL, 0);
				else if(rInputEvent.keyId == eKI_XI_A || rInputEvent.keyId == eKI_PS3_Cross)
					pFlashPlayer->Invoke("onOk", NULL, 0);
				else if(rInputEvent.keyId == eKI_XI_B || rInputEvent.keyId == eKI_PS3_Circle)
					pFlashPlayer->Invoke("onBack", NULL, 0);
				else if(rInputEvent.keyId == eKI_XI_X || rInputEvent.keyId == eKI_PS3_Square)
					pFlashPlayer->Invoke("onGamepadX", NULL, 0);
				else if(rInputEvent.keyId == eKI_XI_Y || rInputEvent.keyId == eKI_PS3_Triangle)
					pFlashPlayer->Invoke("onGamepadY", NULL, 0);
				else if(rInputEvent.keyId == eKI_XI_Start || rInputEvent.keyId == eKI_PS3_Start)
					pFlashPlayer->Invoke("onGamepadStart", NULL, 0);
			}

			// Controller menu navigation:
			//	Navigate
			if(move)
			{
				m_pFlashButtonHandler->SnapToNextButton(pFlashPlayer, dirvec);
			}

			if (m_state != eMS_None)
			{
				m_pFlashButtonHandler->SetLastInputDevice(rInputEvent.deviceId);
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------
bool CHUD_MPSelectMenu::OnInputEventUI(const SInputEvent &rInputEvent)
{
	return false;
}

//------------------------------------------------------------------------
void CHUD_MPSelectMenu::HandleFSCommand(const char *strCommand,const char *strArgs,void* pUserData/* = 0*/)
{
	//CryLog("CHUD_MPSelectMenu::HandleFSCommand %s %s", strCommand?strCommand:"", strArgs?strArgs:"");

	bool bHideScoreboard = false;
	IHUDAsset *selectMenu = GetAsset("selectMenu");

	if (!stricmp(strCommand,"Select_ShowScoreboard"))
	{
		ChangeMenuState(eMS_Scoreboard);
	}
	else if (!stricmp(strCommand,"Select_HideScoreboard"))
	{
		ChangeMenuState(eMS_None);
	}
	else if (!stricmp(strCommand,"Select_ShowMap") || !stricmp(strCommand,"Select_ShowFullscreenMap"))
	{
		ChangeMenuState(eMS_Map);
	}
	else if (!stricmp(strCommand,"Select_ShowLoadout"))
	{
		ChangeMenuState(eMS_Loadout);
	}
	else if (!stricmp(strCommand,"Select_EndStats"))
	{
		ChangeMenuState(eMS_Stats);
	}
	else if (!stricmp(strCommand,"Select_Close"))
	{
		SHUDEvent toggleEvent(eHUDEvent_HideSelectMenu);
		CHUD::CallEvent(toggleEvent);

		ShowMenu(false, true);

		CEquipmentLoadout *loadout = g_pGame->GetGameRules()->GetEquipmentLoadout();
		if (loadout)
		{
			loadout->LoadoutMenuClosed();
		}
	}
	else if (!stricmp(strCommand,"Select_Disconnect"))
	{
		ShowMenu(false, true);

		gEnv->pGame->GetIGameFramework()->ExecuteCommandNextFrame("disconnect");
	}
	else if (!strnicmp(strCommand,"Loadout_",8)) // Loadout handled
	{
		CEquipmentLoadout *loadout = g_pGame->GetGameRules()->GetEquipmentLoadout();
		if (loadout)
		{
			loadout->HandleFSCommand(selectMenu->GetFlashPlayer(), strCommand, strArgs);
		}
	}

	if (m_pFlashButtonHandler)
	{
		// Button stuff
		if(!strcmp(strCommand,"LayerCallBack"))
		{
			if(!strcmp(strArgs,"Clear"))
			{
				m_pFlashButtonHandler->CallbackClear();
			}
			else if(!strcmp(strArgs,"AddOnTop"))
			{
				m_pFlashButtonHandler->CallbackAddOnTop();
			}
			else if(!strcmp(strArgs,"RemoveFromTop"))
			{
				IFlashPlayer *flashPlayer = selectMenu->GetFlashPlayer();
				m_pFlashButtonHandler->CallbackRemoveFromTop(flashPlayer);
			}
		}
		else if(!strcmp(strCommand,"BtnCallBack"))
		{
			//CryLog("BtnCallBack %s %s", szCommand, szArgs);
			IFlashPlayer *flashPlayer = selectMenu->GetFlashPlayer();
			m_pFlashButtonHandler->CallbackAddNewButton(flashPlayer, strArgs);
		}
	}
}

void CHUD_MPSelectMenu::ChangeMenuState(EMPSelectMenuState newState)
{
	if(m_state != newState)
	{
		LeaveMenuState(m_state);
		EnterMenuState(newState);
		m_state = newState;
	}
}

void CHUD_MPSelectMenu::EnterMenuState(EMPSelectMenuState state)
{
	switch(state)
	{
	case eMS_Loadout:
		{
			CEquipmentLoadout *loadout = g_pGame->GetGameRules()->GetEquipmentLoadout();
			if (loadout)
			{
				IHUDAsset *selectMenu = GetAsset("selectMenu");
				loadout->InitializeLoadoutPage(selectMenu->GetFlashPlayer());
			}
			break;
		}
	case eMS_Map:
		{
			SHUDEvent hudEvent(eHUDEvent_ShowSelectMenuMap);
			CHUD::CallEvent(hudEvent);
			g_pGame->GetHUD()->ActivateState("mp_map");
			break;
		}
	case eMS_Scoreboard:
		{
			SHUDEvent hudEvent;
			hudEvent.eventType = eHUDEvent_ShowScoreboard;
			//hudEvent.eventPtrData = (void *)"SelectMenu";
			CHUD::CallEvent(hudEvent);
			break;
		}
	case eMS_Stats:
		{
			break;
		}
	}
}

void CHUD_MPSelectMenu::LeaveMenuState(EMPSelectMenuState state)
{
	switch(state)
	{
		case eMS_Loadout:
		{
			break;
		}
		case eMS_Map:
		{
			SHUDEvent hudEvent(eHUDEvent_HideSelectMenuMap);
			CHUD::CallEvent(hudEvent);
			g_pGame->GetHUD()->ActivateState("mp_menus"); // leaving map state, hide the radar/map.
			break;
		}
		case eMS_Scoreboard:
		{
			SHUDEvent hudEvent;
			hudEvent.eventType = eHUDEvent_HideScoreboard;
			CHUD::CallEvent(hudEvent);
			break;
		}
	}
}

void CHUD_MPSelectMenu::SendEndStatsToFlash()
{
	CPlayerProgression *pPlayerProgression = CPlayerProgression::GetInstance();
	if (pPlayerProgression)
	{
		IHUDAsset *selectMenu = GetAsset("selectMenu");

		// TODO: Possibiliy make this SetVars instead of flash function calls - faster and nicer.

		// Suit mode progression
		for (int i=0; i<(int)eNanoSuitMode_Last; i++)
		{
			ENanoSuitMode suitMode = (ENanoSuitMode)i;
			SFlashVarValue ppSuitArgs[10] = {	i,
				pPlayerProgression->GetSuitModeData(suitMode, ePPS_Level),
				pPlayerProgression->GetSuitModeData(suitMode, ePPS_MaxLevel),
				pPlayerProgression->GetSuitModeData(suitMode, ePPS_XP),
				pPlayerProgression->GetSuitModeData(suitMode, ePPS_XPToNextLevel),
				pPlayerProgression->GetSuitModeData(suitMode, ePPS_XPLastMatch),
				pPlayerProgression->GetSuitModeData(suitMode, ePPS_XPInCurrentLevel),
				pPlayerProgression->GetSuitModeData(suitMode, ePPS_MatchStartLevel),
				pPlayerProgression->GetSuitModeData(suitMode, ePPS_MatchStartXPInCurrentLevel),
				pPlayerProgression->GetSuitModeData(suitMode, ePPS_MatchStartXPToNextLevel)};

			selectMenu->Invoke("SetEndMatchSuitProgressionStats", ppSuitArgs, 10);
		}

		m_tempString = pPlayerProgression->GetRankName(pPlayerProgression->GetData(EPP_MatchStartRank));

		// Player progression
		SFlashVarValue ppArgs[12] = { pPlayerProgression->GetData(EPP_Rank),
			pPlayerProgression->GetRankName(pPlayerProgression->GetData(EPP_Rank)),
			m_tempString.c_str(),
			pPlayerProgression->GetData(EPP_MaxRank),
			pPlayerProgression->GetData(EPP_XP),
			pPlayerProgression->GetData(EPP_XPToNextRank),
			pPlayerProgression->GetData(EPP_XPLastMatch),
			pPlayerProgression->GetData(EPP_XPInCurrentRank),
			pPlayerProgression->GetData(EPP_MatchStartRank),
			pPlayerProgression->GetData(EPP_MatchStartXPInCurrentRank),
			pPlayerProgression->GetData(EPP_MatchStartXPToNextRank),
			pPlayerProgression->GetData(EPP_MatchBonus)};
		selectMenu->Invoke("SetEndMatchProgressionStats", ppArgs, 12);

		pPlayerProgression->SendUnlocksToFlash(selectMenu->GetFlashPlayer());
	}
}

void CHUD_MPSelectMenu::InitHostMigration()
{
	m_bHostMigrating = true;

	// Store players (we're about to lose access to the list)
	CGameRules *pGameRules = g_pGame->GetGameRules();
	IGameRulesPlayerStatsModule *pPlayerStatsModule = pGameRules->GetPlayerStatsModule();

	if (pPlayerStatsModule)
	{
		const int numStats = pPlayerStatsModule->GetNumPlayerStats();
		m_hostMigratingPlayerInfoVec.resize(numStats);
		for (int i = 0; i < numStats; ++ i)
		{
			SHostMigratingPlayerInfo *pPlayerInfo = &m_hostMigratingPlayerInfoVec[i];

			const SGameRulesPlayerStat *playerStats = pPlayerStatsModule->GetNthPlayerStats(i);
			if (playerStats)
			{
				IEntity *pEntity = gEnv->pEntitySystem->GetEntity(playerStats->playerId);
				if (pEntity)
				{
					pPlayerInfo->m_playerName = pEntity->GetName();
				}
				pPlayerInfo->m_hasMigrated = false;
			}
		}
	}

	ShowMenu(true, true, eOM_HostMigration);

	m_bHostMigratingNeedsUpdate = true;
}

void CHUD_MPSelectMenu::UpdateHostMigrationState()
{
	IHUDAsset *selectMenu = GetAsset("selectMenu");

	if (m_bHostMigratingNeedsUpdate)
	{
		const int numPlayers = m_hostMigratingPlayerInfoVec.size();
		for (int playerIndex = 0; playerIndex < numPlayers; ++ playerIndex)
		{
			SHostMigratingPlayerInfo *pPlayerInfo = &m_hostMigratingPlayerInfoVec[playerIndex];

			SFlashVarValue args[3] = { playerIndex, pPlayerInfo->m_playerName.c_str(), pPlayerInfo->m_hasMigrated };
			selectMenu->Invoke("SetPlayerEntry", args, 3);
		}
		selectMenu->Invoke("ClearPlayers"); // Clear all other rows	

		m_bHostMigratingNeedsUpdate = false;
	}

	int secs = int(floor(g_pGame->GetGameRules()->GetRemainingHostMigrationTimeoutTime()));
	TFixedString16 timeString;
	timeString.Format("%.2d", secs);

	selectMenu->Invoke("SetHostMigrationTimeRemaining", timeString.c_str());
}

void CHUD_MPSelectMenu::FinishHostMigration()
{
	m_bHostMigrating = false;
	ShowMenu(false, true);
}

void CHUD_MPSelectMenu::HostMigrationOnNewPlayer( EntityId playerId )
{
	// Find the player
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(playerId);
	if (pEntity)
	{
		const char *pEntityName = pEntity->GetName();

		const int numMigratingPlayers = m_hostMigratingPlayerInfoVec.size();
		for (int i = 0; i < numMigratingPlayers; ++ i)
		{
			SHostMigratingPlayerInfo *pPlayerInfo = &m_hostMigratingPlayerInfoVec[i];
			if (!stricmp(pPlayerInfo->m_playerName.c_str(), pEntityName))
			{
				pPlayerInfo->m_hasMigrated = true;
				m_bHostMigratingNeedsUpdate = true;
				break;
			}
		}
	}
}
