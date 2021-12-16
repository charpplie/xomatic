#include "StdAfx.h"
#include "CryActionCVars.h"
#include "IAIRecorder.h"

CCryActionCVars * CCryActionCVars::s_pThis = 0;

CCryActionCVars::CCryActionCVars()
{
	CRY_ASSERT(!s_pThis);
	s_pThis = this;

	IConsole * console = gEnv->pConsole;

	REGISTER_CVAR2("g_playerInteractorRadius", &playerInteractorRadius, 2.0f, VF_CHEAT, "Maximum radius at which player can interact with other entities");
	REGISTER_CVAR2("i_itemSystemDebugMemStats", &debugItemMemStats, 0, VF_CHEAT, "Display item memory stats on screen");

	console->Register("g_debug_stats", &g_debug_stats, 0, 0, "Enabled massive gameplay events debug");
	console->Register("g_statisticsMode", &g_statisticsMode, 2, 0, "Statistics mode\n"
																																 " 0 - disabled\n"
																																 " 1 - enabled crysis mode\n"
																																 " 2 - enabled K01 mode\n");
	console->Register("g_distanceForceNoIk", &g_distanceForceNoIk, 35.0f, 0, "if bigger then 0, distance at that the foot and aim ik is disabled");

	REGISTER_CVAR2("cl_useCurrentUserNameAsDefault", &useCurrentUserNameAsDefault, 1, 0, "Use the current user name instead of the default profile's name");

#if !defined(_RELEASE)
	REGISTER_CVAR2("g_userNeverAutoSignsIn", &g_userNeverAutoSignsIn, 0, VF_CHEAT, "for autobuilds never automatically bring up the user sign in window, if the user isn't signed in. Can affect performance and skew performance test results. Has to be added to system.cfg, doesn't work if added to the commandline!");
#endif


#ifdef AI_LOG_SIGNALS
	REGISTER_CVAR2("ai_LogSignals", &aiLogSignals, 0, VF_CHEAT, "Maximum radius at which player can interact with other entities");
	REGISTER_CVAR2("ai_MaxSignalDuration", &aiMaxSignalDuration, 3.f, VF_CHEAT, "Maximum radius at which player can interact with other entities");
#endif

	// Currently, GameDLLs should set this cvar - it's a fundamental change in AI
	// 0 is the preferred value, 1 is for Crysis compatibility
	int defaultAiFlowNodeAlertnessCheck = 1;
	REGISTER_CVAR2("ai_FlowNodeAlertnessCheck", &aiFlowNodeAlertnessCheck, defaultAiFlowNodeAlertnessCheck, VF_INVISIBLE, "Enable the alertness check in AI flownodes");

	// Disable HUD debug text
	REGISTER_CVAR2("cl_DisableHUDText", &cl_DisableHUDText, 0, 0, "Force disable all output from HUD Debug text nodes");

	// Disable logging
	REGISTER_CVAR2("cl_DisableCombatLog", &cl_DisableCombatLog, 0, 0, "Turn off combat logging");

	//Gameplay Analyst
	REGISTER_CVAR2("g_gameplayAnalyst", &g_gameplayAnalyst, 0, VF_REQUIRE_APP_RESTART, "Enable/Disable Gameplay Analyst");

	// Cooperative Animation System
	REGISTER_CVAR2("cl_coopDebug", &co_coopAnimDebug, 0, 0, "Enable Cooperative Animation debug output");
	REGISTER_CVAR2( "co_usenewcoopanimsystem", &co_usenewcoopanimsystem, 1, 0, "Uses the new cooperative animation system which works without the animation graph" );
	// ~Cooperative Animation System

	// AI stances
	ag_defaultAIStance = REGISTER_STRING("ag_defaultAIStance", "combat", 0,  "Specifies default stance name for AI");
	g_TweakProfile = REGISTER_STRING("g_TweakProfile","Default",VF_DUMPTODISK,"The currently active Tweak profile, used for all tweakable values and for saving/loading");

	REGISTER_CVAR2("g_syncClassRegistry", &g_syncClassRegistry, 0, VF_NULL, "synchronize class registry from server to clients");

	REGISTER_CVAR(g_usePlatformSavingAPI, 0, VF_CHEAT, "Use the platform APIs for saving and loading (complies with TRCs, but allocates lots of memory)");
}

CCryActionCVars::~CCryActionCVars()
{
	assert (s_pThis != 0);
	s_pThis = 0;

	IConsole *pConsole = gEnv->pConsole;

	pConsole->UnregisterVariable("g_playerInteractorRadius", true);
	pConsole->UnregisterVariable("i_itemSystemDebugMemStats", true);
	pConsole->UnregisterVariable("g_debug_stats", true);
	pConsole->UnregisterVariable("g_statisticsMode", true);
	pConsole->UnregisterVariable("g_distanceForceNoIk", true);

#ifdef AI_LOG_SIGNALS
	pConsole->UnregisterVariable("ai_LogSignals", true);
	pConsole->UnregisterVariable("ai_MaxSignalDuration", true);
#endif
	pConsole->UnregisterVariable("ai_FlowNodeAlertnessCheck", true);

	pConsole->UnregisterVariable("cl_DisableHUDText", true);
	pConsole->UnregisterVariable("cl_DisableCombatLog", true);

	pConsole->UnregisterVariable("g_usePlatformSavingAPI", true);
}
