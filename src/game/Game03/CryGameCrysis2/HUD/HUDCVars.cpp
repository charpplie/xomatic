#include "StdAfx.h"
#include "HUDCVars.h"

#include "HUD.h"
#include "FrontEnd/FrontEnd.h"
#include "FrontEnd/FlashFrontEnd.h"
#include "GameCVars.h"

#include "HUD/HUDObjectManager.h"
#include "HUD/HUD_UnifiedAsset.h"
#include "HUD/Objects/HUD_ProgressNotify.h"
#include "HUD/Objects/HUD_ScoreNotify.h"

//////////////////////////////////////////////////////////////////////////


CHUDCVars::CHUDCVars()
{
	RegisterConsoleCommandsAndVars();
}



CHUDCVars::~CHUDCVars()
{
	UnregisterConsoleCommandsAndVars();
}



void CHUDCVars::RegisterConsoleCommandsAndVars()
{
	// HUD Hiding
	ICVar* pCVar = REGISTER_CVAR(hud_hide, 0, 0, "Hides the hud." );
	if(pCVar)
		pCVar->SetOnChangeCallback(CmdOnHideHUD);

	// HUD Loading
	REGISTER_COMMAND("hud_reload", CmdReloadHUD, VF_CHEAT, "Reload HUD");

#if ENABLE_HUD_EXTRA_DEBUG
	REGISTER_COMMAND("hud_leavingBattleAreaToggle", CmdToggleBattleAreaState, VF_CHEAT, "Force display/non-display of battle area notification.");
#endif

	// HUD State and Object Management
	REGISTER_COMMAND("hud_setActiveState", CmdActivateState, VF_CHEAT, "Sets the active HUD state");
	REGISTER_CVAR(hud_debugActiveStates, 0, 0, "Draws the names of the active states etc. on screen");
	REGISTER_CVAR(hud_debugDrawAssets, 0, 0, "Draw outlines and data for HUDAssets");
	hud_debugDrawAssets_search = REGISTER_STRING("hud_debugDrawAssets_search","",0,"A string on which to match assets names for debug drawing");

	// Safe Areas
	REGISTER_CVAR(hud_drawSafeAreas, 0, 0, "Draw safe areas");

	//Crosshair
	REGISTER_CVAR(hud_Crosshair, 1, 0, "Selects the crosshair")->SetOnChangeCallback(CmdOnCrosshairSelected);
	REGISTER_CVAR(hud_CrosshairSpreadMultiplier, 10.0f, 0, "Weapon spread multiplier for the crosshair");
	gEnv->pConsole->Register("hud_CrosshairSpread", &hud_CrosshairSpreadMultiplier, hud_CrosshairSpreadMultiplier, 0, "DEPRECATED: Use 'hud_CrosshairSpreadMultiplier' instead.");
	REGISTER_CVAR(hud_CrosshairMinSpread, 0.f, 0, "Minimum crosshair spread (for readability)");
	REGISTER_CVAR(hud_CrosshairUseSmoothSpread, 0, 0, "Enable/disable cross-hair smoothing on cross hair spread change.");
	REGISTER_CVAR(hud_CrosshairHideOnReload, 1, 0, "Show/hide crosshair on reload.");
#if	ENABLE_HUD_EXTRA_DEBUG
	REGISTER_CVAR(hud_CrosshairShowInfo, 0, 0, "Show hide debug info for the crosshair.");
#endif

	//Interest points
	REGISTER_CVAR(hud_InterestPointsRadius, 50.0f, 0, "Radius in which the interest points are requested");
	REGISTER_CVAR(hud_InterestPointsQueryRate, 1.0f, 0, "Interest points are queried every X seconds");

	//Radar
	REGISTER_CVAR(hud_radarDebug, 0, 0, "Debugging of the radar");
	REGISTER_CVAR(hud_radar_ShowRadarBg, 0, 0, "Debugging of the radar");
	REGISTER_CVAR(hud_radarOnShootTimeOnRadar, 3.0f, 0, "Seconds you appear on the enemy radar when you shoot");
	REGISTER_CVAR(hud_radarShowVisibleEnemiesWithinConeSize, 0.7f, 0, "Size of the cone (dot product cut-off value) in which non-occluded enemies appear on radar");
	REGISTER_CVAR(hud_radarOverrideScale, 0.0f, 0, "Scales the zoom of the radar at runtime");
	REGISTER_CVAR(hud_radarOverrideMapRotation, 0.0f, 0, "Change the rotation of the map at runtime");
	REGISTER_CVAR(hud_radarOverrideMapScale, 0.0f, 0, "Scales the fullscreen map at runtime");

	// Player Nav Path Helper
	REGISTER_CVAR(hud_navPath_enable, 0, 0, "Enable/Disables displaying player nav path");
	REGISTER_CVAR(hud_navPath_updateDistance, 2.0f, 0, "Distance from nearest path point to trigger update");
	REGISTER_CVAR(hud_navPath_autoClearAtEnd, 0, 0, "Clear path when player reaches end");
	REGISTER_CVAR(hud_navPath_autoClearDistance, 5.0f, 0, "Distance from end we can perform auto-clear");
	REGISTER_CVAR(hud_navPath_maxPoints, 100, 0, "Maximum number of points in nav path");
	REGISTER_CVAR(hud_navPath_minSegmentLength, 0.5f, 0, "smallest distance allowed between path points");
	REGISTER_CVAR(hud_navPath_segmentLengthIncrease, 1.0f, 0, "size increase of each subsequent segment");
	REGISTER_CVAR(hud_navPath_smoothLevel, 4, 0, "Number of smoothing points to add between each path point");

	// Tag Names
	REGISTER_CVAR(hud_mpTagNames_Duration, 2.0f, 0, "MP enemy names will fade after this duration (friends' names are always visible).");
	REGISTER_CVAR(hud_mpTagNames_LockDuration, 0.1f, 0, "MP enemy names will appear after this duration.");
	REGISTER_CVAR(hud_mpTagNames_OffScreenTeammates, 1, 0, "MP teammates will be tagged when they are off screen");
	REGISTER_CVAR(hud_mpTagNames_ForceDraw, 0, 0, "MP all tagnames will be drawn.");
	REGISTER_CVAR(hud_mpTagNames_ThroughGeom_friendies, 1, 0, "MP friendly player name tags should be drawn through geometry");
	REGISTER_CVAR(hud_mpTagNames_ThroughGeom_enemies, 0, 0, "MP enemy player name tags should be drawn through geometry");
	REGISTER_CVAR(hud_mpTagNames_ThroughGeom_dead, 0, 0, "MP dead player name tags should be drawn through geometry" );
	REGISTER_CVAR(hud_mpTagNames_debug, 0, 0, "Enable debugging of tagnames");
	REGISTER_CVAR(hud_tagNames_showOverEnemiesWhenLookDirIsThisAccurate, 0.99f, 0, "Show names above enemies when under crosshair");
	REGISTER_CVAR(hud_tagNames_showOverCorpses, 1, 0, "Tag names are rendered over corpses");

	REGISTER_CVAR(hud_tagNames_healthBar_nearDistance, 1.f, 0, "Distance for when health bars fully shown.");
	REGISTER_CVAR(hud_tagNames_healthBar_farDistance, 100.f, 0, "Distance for when health bars hidden.");
	REGISTER_CVAR(hud_tagNames_healthBar_showBarFullWhenHealthIs, 150.f, 0, "Health bars shown over teammates");
	REGISTER_CVAR(hud_tagNames_healthBar_numBlocks, 10, 0, "Health bars shown over teammates");
	REGISTER_CVAR(hud_tagNames_healthBar_blockSizeX, 2.5f, 0, "Health bars shown over teammates");
	REGISTER_CVAR(hud_tagNames_healthBar_blockSizeY, 2.0f, 0, "Health bars shown over teammates");
	REGISTER_CVAR(hud_tagNames_healthBar_blockPositionOffsetY, 0.5f, 0, "Health bars shown over teammates");

	REGISTER_CVAR(hud_mpTagNames_MinFontSize_nearSize, 26.0f, 0, "The smallest font size to use for tagnames when near.");
	REGISTER_CVAR(hud_mpTagNames_MinFontSize_midSize, 26.0f, 0, "The smallest font size to use for tagnames when at a mid-distance.");
	REGISTER_CVAR(hud_mpTagNames_MinFontSize_farSize, 26.0f, 0, "The smallest font size to use for when far away.");

	REGISTER_CVAR(hud_mpTagNames_MinFontSize_nearDist, 1.0f, 0, "The distance to use when using the near size.");
	REGISTER_CVAR(hud_mpTagNames_MinFontSize_midDist, 8.0f, 0, "The distance to use when using the mid size.");
	REGISTER_CVAR(hud_mpTagNames_MinFontSize_farDist, 100.0f, 0, "The distance to use when using the far size.");


	REGISTER_CVAR(hud_mpTagNames_ZOffset_nearOffset, 0.18f, 0, "The height added to the tagname when within 'near' distance.");
	REGISTER_CVAR(hud_mpTagNames_ZOffset_midOffset, 0.18f, 0, "The height added to the tagname when at the 'mid' distance.");
	REGISTER_CVAR(hud_mpTagNames_ZOffset_farOffset, 0.18f, 0, "The height added to the tagname when outside 'far' distance..");

	REGISTER_CVAR(hud_mpTagNames_ZOffset_nearDist, 1.0f, 0, "The distance to use when using the near offset.");
	REGISTER_CVAR(hud_mpTagNames_ZOffset_midDist, 8.0f, 0, "The distance to use when using the mid offset.");
	REGISTER_CVAR(hud_mpTagNames_ZOffset_farDist, 350.0f, 0, "The distance to use when using the far offset.");

	// HUD Testing 
#if ENABLE_HUD_TESTS
	GetISystem()->GetIConsole()->AddCommand("hud_test",CHUD::CmdTestHud,VF_CHEAT,"Runs TRC checks on the HUD.");
	REGISTER_CVAR(hud_test_timeBetweenTestScreenShots, 2, 0, "Time between screen shot when auto testing." );
	REGISTER_CVAR(hud_quitAfterTest, 0, 0, "Draw outlines and data for HUDAssets.");
	REGISTER_CVAR(hud_test_iterateResolutions, 0, 0, "Iterate over the range of target resolutions.");
	REGISTER_CVAR(hud_test_iterateSafeAreas, 1, 0, "Iterate over the range of target safe area borders.");
	REGISTER_CVAR(hud_test_takeScreenShots, 0, 0, "Take screen shots of every test permutation during hud_test." );
#endif // ENABLE_HUD_TESTS
#if ENABLE_HUD_EXTRA_DEBUG
	GetISystem()->GetIConsole()->AddCommand("hud_forceDrawAllAssets", CHUD::CmdForceDrawAll, VF_CHEAT, "Forces all HUDObjects in the active state to render (if listed in teh func)." );
#endif // ENABLE_HUD_EXTRA_DEBUG

	// Misc
	REGISTER_CVAR(hud_hideMenus, 0, 0, "Hide the menus");
	REGISTER_CVAR(hud_drawCentreOfScreen, 0, 0, "Draw centre of screen");
	REGISTER_CVAR(hud_DMode, 0, 0, "Enable Debug Mode HUD");
#if ENABLE_HUD_EXTRA_DEBUG
	REGISTER_COMMAND("hud_fakeXP", CHUD_ProgressNotify::CMDTriggerXP, VF_CHEAT, "Send fake XP messages to the HUD" );
	REGISTER_COMMAND("hud_fakeScores", CHUD_ScoreNotify::CMDTriggerScore, VF_CHEAT, "Send fake score messages to the HUD" );
#endif

	// Menus
	REGISTER_COMMAND("reloadFrontEnd",CmdReloadUI,VF_CHEAT,"Reload FrontEnd User Interface.");
#if !defined(_RELEASE)
	REGISTER_COMMAND("menu_gotoPage",CmdFrontEndSwitchPage,VF_CHEAT,"Goes the to page with the name provided.");
	REGISTER_CVAR(menu_debugScreenStack, 0, 0, "Enable debug watch of the current menu screen stack");
#endif
}



void CHUDCVars::UnregisterConsoleCommandsAndVars()
{
	IConsole* pConsole = gEnv->pConsole;

	// HUD Hiding - possibility of designer abuse.
	pConsole->UnregisterVariable("hud_hide", true );

	// HUD Loading
	pConsole->RemoveCommand("hud_reload");

#if ENABLE_HUD_EXTRA_DEBUG
	pConsole->RemoveCommand("hud_leavingBattleAreaToggle");
#endif

	// HUD State and Object Management
	pConsole->RemoveCommand("hud_setActiveState");
	pConsole->UnregisterVariable("hud_debugActiveStates", true);
	pConsole->UnregisterVariable("hud_debugDrawAssets", true);
	pConsole->UnregisterVariable("hud_debugDrawAssets_search", true);

	// Safe Areas
	pConsole->UnregisterVariable("hud_drawSafeAreas", true);

	//Crosshair
	pConsole->UnregisterVariable("hud_Crosshair", true);
	pConsole->UnregisterVariable("hud_CrosshairSpreadMultiplier", true);
	pConsole->UnregisterVariable("hud_CrosshairMinSpread", true);
	pConsole->UnregisterVariable("hud_CrosshairSpread", true);
	pConsole->UnregisterVariable("hud_CrosshairUseSmoothSpread", true);
	pConsole->UnregisterVariable("hud_CrosshairHideOnReload", true);
#if	ENABLE_HUD_EXTRA_DEBUG
	pConsole->UnregisterVariable("hud_CrosshairShowInfo", true);
#endif

	//Interest points
	pConsole->UnregisterVariable("hud_InterestPointsRadius", true);
	pConsole->UnregisterVariable("hud_InterestPointsQueryRate", true);

	//Radar
	pConsole->UnregisterVariable("hud_radarDebug", true);
	pConsole->UnregisterVariable("hud_radar_ShowRadarBg", true );
	pConsole->UnregisterVariable("hud_radarOnShootTimeOnRadar", true);
	pConsole->UnregisterVariable("hud_radarShowVisibleEnemiesWithinConeSize", true);
	pConsole->UnregisterVariable("hud_radarOverrideScale", true);
	pConsole->UnregisterVariable("hud_radarOverrideMapRotation", true);
	pConsole->UnregisterVariable("hud_radarOverrideMapScale", true);

	// Player Nav Path Helper
	pConsole->UnregisterVariable("hud_navPath_enable", true);
	pConsole->UnregisterVariable("hud_navPath_updateDistance", true);
	pConsole->UnregisterVariable("hud_navPath_autoClearAtEnd", true);
	pConsole->UnregisterVariable("hud_navPath_autoClearDistance", true);
	pConsole->UnregisterVariable("hud_navPath_maxPoints", true);
	pConsole->UnregisterVariable("hud_navPath_minSegmentLength", true);
	pConsole->UnregisterVariable("hud_navPath_segmentLengthIncrease", true);
	pConsole->UnregisterVariable("hud_navPath_smoothLevel", true);

	// Tag Names
	pConsole->UnregisterVariable("hud_mpTagNames_Duration", true);
	pConsole->UnregisterVariable("hud_mpTagNames_LockDuration", true);
	pConsole->UnregisterVariable("hud_mpTagNames_OffScreenTeammates", true);
	pConsole->UnregisterVariable("hud_mpTagNames_ForceDraw", true);
	pConsole->UnregisterVariable("hud_mpTagNames_ThroughGeom_friendies", true);
	pConsole->UnregisterVariable("hud_mpTagNames_ThroughGeom_enemies", true);
	pConsole->UnregisterVariable("hud_mpTagNames_ThroughGeom_dead", true);
	pConsole->UnregisterVariable("hud_mpTagNames_debug", true);
	pConsole->UnregisterVariable("hud_tagNames_showOverEnemiesWhenLookDirIsThisAccurate", true);
	pConsole->UnregisterVariable("hud_tagNames_showOverCorpses", true);

	pConsole->UnregisterVariable("hud_tagNames_healthBar_nearDistance", true);
	pConsole->UnregisterVariable("hud_tagNames_healthBar_farDistance", true);
	pConsole->UnregisterVariable("hud_tagNames_healthBar_showBarFullWhenHealthIs", true);
	pConsole->UnregisterVariable("hud_tagNames_healthBar_numBlocks", true);
	pConsole->UnregisterVariable("hud_tagNames_healthBar_blockSizeX", true);
	pConsole->UnregisterVariable("hud_tagNames_healthBar_blockSizeY", true);
	pConsole->UnregisterVariable("hud_tagNames_healthBar_blockPositionOffsetY", true);

	pConsole->UnregisterVariable("hud_mpTagNames_MinFontSize_nearSize", true);
	pConsole->UnregisterVariable("hud_mpTagNames_MinFontSize_midSize", true);
	pConsole->UnregisterVariable("hud_mpTagNames_MinFontSize_farSize", true);

	pConsole->UnregisterVariable("hud_mpTagNames_MinFontSize_nearDist", true);
	pConsole->UnregisterVariable("hud_mpTagNames_MinFontSize_midDist", true);
	pConsole->UnregisterVariable("hud_mpTagNames_MinFontSize_farDist", true);

	pConsole->UnregisterVariable("hud_mpTagNames_ZOffset_nearOffset", true);
	pConsole->UnregisterVariable("hud_mpTagNames_ZOffset_midOffset", true);
	pConsole->UnregisterVariable("hud_mpTagNames_ZOffset_farOffset", true);

	pConsole->UnregisterVariable("hud_mpTagNames_ZOffset_nearDist", true);
	pConsole->UnregisterVariable("hud_mpTagNames_ZOffset_midDist", true);
	pConsole->UnregisterVariable("hud_mpTagNames_ZOffset_farDist", true);

	// HUD Testing 
#if ENABLE_HUD_TESTS
	pConsole->RemoveCommand("hud_test");
	pConsole->UnregisterVariable("hud_test_timeBetweenTestScreenShots", true);
	pConsole->UnregisterVariable("hud_quitAfterTest", true);
	pConsole->UnregisterVariable("hud_test_iterateResolutions", true);
	pConsole->UnregisterVariable("hud_test_iterateSafeAreas", true);
	pConsole->UnregisterVariable("hud_test_takeScreenShots", true);
#endif // ENABLE_HUD_TESTS
#if ENABLE_HUD_EXTRA_DEBUG
	pConsole->RemoveCommand("hud_forceDrawAllAssets");
#endif // ENABLE_HUD_EXTRA_DEBUG

	// Misc
	pConsole->UnregisterVariable("hud_hideMenus", true);
	pConsole->UnregisterVariable("hud_drawCentreOfScreen", true);
	pConsole->UnregisterVariable("hud_DMode", true);

#if ENABLE_HUD_EXTRA_DEBUG
	pConsole->RemoveCommand("hud_fakeScores");
	pConsole->RemoveCommand("hud_fakeXP");
#endif // ENABLE_HUD_EXTRA_DEBUG

#if !defined(_RELEASE)
	// Menus
	pConsole->RemoveCommand("reloadFrontEnd");
	pConsole->RemoveCommand("menu_gotoPage");
	pConsole->UnregisterVariable("menu_debugScreenStack", true);
#endif
}



/*static*/ void CHUDCVars::CmdReloadHUD(IConsoleCmdArgs *pArgs)
{
	if( g_pGame )
	{
		g_pGame->GetHUD()->Reload();
		SHUDEvent hudevent;
		hudevent.eventType = eHUDEvent_OnInitPlayer;
		CHUD::CallEvent(hudevent);
		hudevent.eventType = eHUDEvent_RescanActors;
		CHUD::CallEvent(hudevent);
	}
}



/*static*/ void CHUDCVars::CmdActivateState(IConsoleCmdArgs *pArgs)
{
	if( pArgs->GetArgCount()>1 && g_pGame )
	{
		g_pGame->GetHUD()->ActivateState(pArgs->GetArg(1));
	}
}



/*static*/ void CHUDCVars::CmdOnCrosshairSelected(ICVar* pCVar)
{
	SHUDEvent event(eHUDEvent_OnCrosshairSelected);
	event.AddData(SHUDEventData(pCVar->GetIVal()));
	CHUD::CallEvent(event);
}

/*static*/ void CHUDCVars::CmdReloadUI(IConsoleCmdArgs *pArgs)
{
	if (g_pGame->GetFlashMenu())
	{
		g_pGame->GetFlashMenu()->Clear();
		g_pGame->GetFlashMenu()->Initialize();
	}
}

#if !defined(_RELEASE)
/*static*/ void CHUDCVars::CmdFrontEndSwitchPage(IConsoleCmdArgs *pArgs)
{
	if (pArgs->GetArgCount()>1 && g_pGame->GetFlashMenu())
	{
		g_pGame->GetFlashMenu()->Execute(CFlashFrontEnd::eFECMD_goto, pArgs->GetArg(1));
	}
}
#endif


/*static*/ void CHUDCVars::CmdOnHideHUD(ICVar* pCVar)
{
	ShowHUD(pCVar->GetIVal()==0);
}



/*static*/ void CHUDCVars::ShowHUD(const bool show)
{
	if(CHUDObjectManager* pMgr = g_pGame->GetHUD()->GetObjectManager())
	{
		if(CHUD_UnifiedAsset* pObj = static_cast<CHUD_UnifiedAsset*>(pMgr->FindObjectByName("UnifiedAsset")))
		{
			pObj->Show(show);
		}
	}
}



#if ENABLE_HUD_EXTRA_DEBUG
/*static*/ void CHUDCVars::CmdToggleBattleAreaState(IConsoleCmdArgs* pArgs)
{
	static bool on = false;
	if( on )
	{
		CHUD::CallEvent(SHUDEvent(eHUDEvent_ReturningToBattleArea));
	}
	else
	{
		CHUD::CallEvent(SHUDEvent(eHUDEvent_LeavingBattleArea));
	}
	on = !on;
}
#endif //ENABLE_HUD_EXTRA_DEBUG

//////////////////////////////////////////////////////////////////////////
