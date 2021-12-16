/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 11:8:2004   10:50 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "GameCVars.h"
#include "GameRules.h"
#include "ItemSharedParams.h"
#include "WeaponSharedParams.h"
#include "Player.h"
#include "HitDeathReactions.h"
#include "HitDeathReactionsSystem.h"

#include <INetwork.h>
#include <IGameObject.h>
#include <IActorSystem.h>
#include <IItemSystem.h>
#include <IVehicleSystem.h>
#include "WeaponSystem.h"
#include "ServerSynchedStorage.h"
#include "ItemString.h"
#include "FrontEnd/FrontEnd.h"
#include "FrontEnd/FlashFrontEnd.h"
#include "Environment/BattleDust.h"
#include "NetInputChainDebug.h"
#include "HUD/HUD.h"
#include "FrontEnd/ILoadingMessageProvider.h"
#include "INetworkService.h"
#include "MindControlConsole.h"

#include "GameRulesModules/IGameRulesTeamsModule.h"
#include "GameRulesModules/IGameRulesSpectatorModule.h"
#include "GameActions.h"
#include "GodMode.h"
#include "BodyDamage.h"
#include "DummyPlayer.h"
#include "PlayerInput.h"
#include "StatsRecordingMgr.h"

static void BroadcastChangeSafeMode( ICVar * )
{
	SGameObjectEvent event(eCGE_ResetMovementController, eGOEF_ToExtensions);
	IEntitySystem * pES = gEnv->pEntitySystem;
	IEntityItPtr pIt = pES->GetEntityIterator();
	while (!pIt->IsEnd())
	{
		if (IEntity * pEnt = pIt->Next())
			if (IActor * pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEnt->GetId()))
				pActor->HandleEvent( event );
	}
}

static void ClMotionBlurCVarChanged( ICVar * )
{
	IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
	if (pClientActor)
	{
		IScriptTable *pEntityScript = pClientActor->GetEntity()->GetScriptTable();
		if (pEntityScript)
		{
			pEntityScript->SetValue("blurType", g_pGameCVars->cl_motionBlur);
		}
	}
}
//------------------------------------------------------------------------
static void OnTelemetryConfigChanged(ICVar* pCVar)
{
	CStatsRecordingMgr* pMgr = g_pGame->GetStatsRecorder();
	if(pCVar && pMgr)
	{
		pMgr->LoadEventConfig(pCVar->GetString());
	}
}

//------------------------------------------------------------------------
void SCVars::InitAIPerceptionCVars(IConsole *pConsole)
{
	REGISTER_CVAR(ai_perception.movement_useSurfaceType, 0, VF_CHEAT, "Toggle if surface type should be used to get the base radius instead of cvars");
	REGISTER_CVAR(ai_perception.movement_movingSurfaceDefault, 1.0f, VF_CHEAT, "Default value for movement speed effect on footstep radius (overridden by surface type)");
	REGISTER_CVAR(ai_perception.movement_standingRadiusDefault, 5.0f, VF_CHEAT, "Default value for standing footstep sound radius (overridden by surface type)");
	REGISTER_CVAR(ai_perception.movement_crouchRadiusDefault, 0.0f, VF_CHEAT, "Default value for crouching footstep sound radius multiplier (overridden by surface type)");
	REGISTER_CVAR(ai_perception.movement_standingMovingMultiplier, 5.0f, VF_CHEAT, "Multiplier for standing movement speed effect on footstep sound radius");
	REGISTER_CVAR(ai_perception.movement_crouchMovingMultiplier, 0.0f, VF_CHEAT, "Multiplier for crouched movement speed effect on footstep sound radius");
}

static void ClSprintBlurCVarChanged( ICVar * )
{
	IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
	if (pClientActor)
	{
		IScriptTable *pEntityScript = pClientActor->GetEntity()->GetScriptTable();
		if (pEntityScript)
		{
			pEntityScript->SetValue("sprintBlur", g_pGameCVars->cl_sprintBlur);
		}
	}
}

void DifficultyLevelChanged(ICVar* difficultyLevel)
{
	CRY_ASSERT(gEnv);
	CRY_ASSERT(gEnv->pSystem);
	CRY_ASSERT(g_pGame);

	if (gEnv->pSystem->IsDevMode())
	{
		const int iCVarDifficulty = difficultyLevel->GetIVal();
		EDifficulty difficulty = (iCVarDifficulty >= eDifficulty_Default && iCVarDifficulty < eDifficulty_COUNT ? (EDifficulty)iCVarDifficulty : eDifficulty_Default);
		g_pGame->SetDifficultyLevel(difficulty);
	}
}


// game related cvars must start with an g_
// game server related cvars must start with sv_
// game client related cvars must start with cl_
// no other types of cvars are allowed to be defined here!
void SCVars::InitCVars(IConsole *pConsole)
{
	//Multiplayer mode by default
	REGISTER_CVAR(g_enableGameSwitchMenu,0,VF_CHEAT|VF_READONLY,"Enable the singleplayer/multiplayer switching menu.");
	REGISTER_CVAR(g_multiplayerDefault,0,0,"Enable multiplayer gameplay by default");

	//Exclusive Playable Demo version
	REGISTER_CVAR(g_EPD, 0, VF_REQUIRE_APP_RESTART, "Enable Exclusive Playable Demo version");

	//client cvars
	REGISTER_CVAR(cl_fov, 55.0f, 0, "field of view.");
	REGISTER_CVAR(cl_tpvDist, 3.5f, 0, "camera distance in 3rd person view");
	REGISTER_CVAR(cl_tpvYaw, 0, 0, "camera angle offset in 3rd person view");
	REGISTER_CVAR(cl_nearPlane, 0, 0, "overrides the near clipping plane if != 0, just for testing.");
	REGISTER_CVAR(cl_sprintShake, 0.0f, 0, "sprint shake");
	REGISTER_CVAR(cl_sensitivity, 30.0f, VF_DUMPTODISK, "Set mouse sensitivity!");
	REGISTER_CVAR(cl_sensitivityController, 0.33f, VF_DUMPTODISK, "Set controller sensitivity! Expecting 0.0f to 2.0f");
	REGISTER_CVAR(cl_mp_sensitivityControllerDefault, 1.0f, VF_DUMPTODISK|VF_READONLY, "Multiplayer override for default sensitivity - this value will be poked into cl_sensitivityController when init'ing for a multiplayer game after the default profile XMLs have been loaded for a new player");
	REGISTER_CVAR(cl_invertMouse, 0, VF_DUMPTODISK, "mouse invert?");
	REGISTER_CVAR(cl_invertController, 0, VF_DUMPTODISK, "Controller Look Up-Down invert");
	REGISTER_CVAR(cl_crouchToggle, 1, VF_DUMPTODISK, "To make the crouch key work as a toggle");
	REGISTER_CVAR(cl_fpBody, 2, 0, "first person body");
	REGISTER_CVAR(cl_logAsserts, 0, 0, "1 = log assert only. 2 = log assert and callstack");


	REGISTER_CVAR(cl_camModify, 0, 0, "Enable camera modifiers.");
	REGISTER_CVAR(cl_camKeepX, 1.0f, 0, "Fraction of camera x-axis rotation to keep.");
	REGISTER_CVAR(cl_camKeepY, 1.0f, 0, "Fraction of camera y-axis rotation to keep.");
	REGISTER_CVAR(cl_camKeepZ, 1.0f, 0, "Fraction of camera z-axis rotation to keep.");
	REGISTER_CVAR(cl_camOffsetX, 0.0f, 0, "Degrees to offset camera around x-axis.");
	REGISTER_CVAR(cl_camOffsetY, 0.0f, 0, "Degrees to offset camera around y-axis.");
	REGISTER_CVAR(cl_camOffsetZ, 0.0f, 0, "Degrees to offset camera around z-axis.");
	REGISTER_CVAR(cl_camRotateX, 0.0f, 0, "Degrees to rotate camera per second around x-axis.");
	REGISTER_CVAR(cl_camRotateY, 0.0f, 0, "Degrees to rotate camera per second around y-axis.");
	REGISTER_CVAR(cl_camRotateZ, 0.0f, 0, "Degrees to rotate camera per second around z-axis.");
	REGISTER_CVAR(cl_camTranslateX, 0.0f, 0, "Translate along camera local x-axis.");
	REGISTER_CVAR(cl_camTranslateY, 0.0f, 0, "Translate along camera local y-axis.");
	REGISTER_CVAR(cl_camTranslateZ, 0.0f, 0, "Translate along camera local z-axis.");

	REGISTER_CVAR(i_staticfiresounds, 1, VF_DUMPTODISK, "Enable/Disable static fire sounds. Static sounds are not unloaded when idle.");
	REGISTER_CVAR(i_soundeffects,	1, VF_DUMPTODISK, "Enable/Disable playing item sound effects.");
	REGISTER_CVAR(i_lighteffects, 1, VF_DUMPTODISK, "Enable/Disable lights spawned during item effects.");
	REGISTER_CVAR(i_particleeffects,	1, VF_DUMPTODISK, "Enable/Disable particles spawned during item effects.");
	REGISTER_CVAR(i_rejecteffects, 1, VF_DUMPTODISK, "Enable/Disable ammo reject effects during weapon firing.");

	REGISTER_CVAR(cl_speedToBobFactor, 0.35f, 0, "");
	REGISTER_CVAR(cl_bobWidth, 0.02f, 0, "");
	REGISTER_CVAR(cl_bobHeight, 0.025f, 0, "");
	REGISTER_CVAR(cl_strafeHorzScale, 0.05f, 0, "");


	CRY_FIXME(1,10,09, "GAME_WARS2 difference on cvar");
	//REGISTER_INT("i_maxheavymedweapons", 1, VF_CHEAT, "The number of heavy and medium weapons the player can carry");
	REGISTER_INT("i_maxheavymedweapons", 2, VF_CHEAT, "The number of heavy and medium weapons the player can carry");

	// marcok TODO: seem to be only used on script side ... 
	REGISTER_CVAR(cl_motionBlur, 2, 0, "motion blur type (0=off, 1=accumulation-based, 2=velocity-based)")->SetOnChangeCallback(ClMotionBlurCVarChanged);
	REGISTER_CVAR(cl_sprintBlur, 0.6f, 0, "sprint blur")->SetOnChangeCallback(ClSprintBlurCVarChanged);
	REGISTER_FLOAT("cl_hitShake", 1.25f, 0, "hit shake");
	REGISTER_FLOAT("cl_hitBlur", 0.25f, 0, "blur on hit");

	REGISTER_INT("cl_righthand", 1, 0, "Select right-handed weapon!");
	REGISTER_INT("cl_screeneffects", 1, 0, "Enable player screen effects (depth-of-field, motion blur, ...).");

	REGISTER_CVAR(cl_suitModeSpecificScreenFX, 1, 0, "When enabled, it will trigger specific screen effects for the suit modes.");
	
	REGISTER_CVAR(cl_debugSwimming, 0, VF_CHEAT, "enable swimming debugging");

	ca_GameControlledStrafingPtr = pConsole->GetCVar("ca_GameControlledStrafing");
	REGISTER_CVAR(pl_curvingSlowdownSpeedScale, 0.5f, VF_CHEAT, "Player only slowdown speedscale when curving/leaning extremely.");
	REGISTER_CVAR(ac_enableProceduralLeaning, 1.0f, VF_CHEAT, "Enable procedural leaning (disabled asset leaning and curving slowdown).");

	REGISTER_CVAR(cl_shallowWaterSpeedMulPlayer, 0.6f, VF_CHEAT, "shallow water speed multiplier (Players only)");
	REGISTER_CVAR(cl_shallowWaterSpeedMulAI, 0.8f, VF_CHEAT, "Shallow water speed multiplier (AI only)");
	REGISTER_CVAR(cl_shallowWaterDepthLo, 0.3f, VF_CHEAT, "Shallow water depth low (below has zero slowdown)");
	REGISTER_CVAR(cl_shallowWaterDepthHi, 1.0f, VF_CHEAT, "Shallow water depth high (above has full slowdown)");

	REGISTER_CVAR(cl_idleBreaksDelayTime, 8.0f, VF_CHEAT, "Time delay between idle breaks");

	REGISTER_INT("g_grabLog", 0, 0, "verbosity for grab logging (0-2)");

	REGISTER_CVAR(pl_inputAccel, 30.0f, 0, "Movement input acceleration");
	REGISTER_CVAR(pl_debug_suit, 0, 0, "Display debug Nanosuit debug info");
	REGISTER_CVAR(pl_debug_pickable_items, 0, 0, "Display information about pickable item at which the player is about to interact with");

	REGISTER_INT("cl_actorsafemode", 0, VF_CHEAT, "Enable/disable actor safe mode")->SetOnChangeCallback(BroadcastChangeSafeMode);
	REGISTER_CVAR(h_useIK, 1, 0, "Hunter uses always IK");
	REGISTER_CVAR(h_drawSlippers, 0, 0, "Red ball when tentacle is lifted, green when on ground");
	REGISTER_CVAR(g_tentacle_joint_limit, -1.0f, 0, "forces specific tentacle limits; used for tweaking");
	REGISTER_CVAR(g_enableSpeedLean, 0, 0, "Enables player-controlled curve leaning in speed mode.");
	//
	REGISTER_FLOAT("aa_maxDist", 10.0f, VF_CHEAT, "max lock distance");

	REGISTER_CVAR(g_gameRules_startTimerLength,            15.f,    0, "The N in 'Game will start in N seconds' (triggered once there are enough players)");
	REGISTER_CVAR(g_gameRules_nextLevelTimerLength,        22000.f, 0, "Was GR_STATE_NEXT_LEVEL_TIMER_LENGTH");
	REGISTER_CVAR(g_gameRules_showScoreboardTimerLength,	 5000.f,  0, "Duration for which the scoreboard appears when a game is finished");
	REGISTER_CVAR(g_gameRules_minPlayerWarningTimerLength, 15000.f, 0, "Was GR_STATE_MIN_PLAYER_WARNING_TIMER_LENGTH");

	REGISTER_CVAR(g_stanceTransitionSpeed, 15.0f, VF_CHEAT, "Set speed of camera transition from stance to stance");

	REGISTER_CVAR(g_flashBangMinSpeedMultiplier, 0.2f, VF_CHEAT, "Set the minimum movement and rotation speed multiplier when stunned by a flashbang");
	REGISTER_CVAR(g_flashBangSpeedMultiplierFallOffEase, 5.f, VF_CHEAT, "Alters the falloff curve for the flashbang multiplier");
	REGISTER_CVAR(g_flashBangNotInFOVRadiusFraction, 0.66f, VF_CHEAT, "Set the radius fraction which will still blind a player even if not looking at a flashbang");
	REGISTER_CVAR(g_flashBangMinFOVMultiplier, 0.75f, VF_CHEAT, "Set the minimum multiplier for the dot product comparison");
	REGISTER_CVAR(g_flashBangFriends, 0, VF_CHEAT, "Applies flash bangs to friends");
	REGISTER_CVAR(g_flashBangSelf, 1, VF_CHEAT, "Applies flash bangs to your self");
	gEnv->pConsole->Register("g_difficultyLevel", &g_difficultyLevel, 2, VF_CHEAT|VF_READONLY, "Difficulty level", DifficultyLevelChanged);
	REGISTER_CVAR(g_difficultyHintSystem, 2, VF_CHEAT|VF_READONLY, "Lower difficulty hint system (0 is off, 1 is radius based, 2 is save-game based)");
	REGISTER_CVAR(g_difficultyRadius, 300, VF_CHEAT|VF_READONLY, "Radius in which player needs to die to display lower difficulty level hint.");
	REGISTER_CVAR(g_difficultyRadiusThreshold, 5, VF_CHEAT|VF_READONLY, "Number of times player has to die within radius to trigger difficulty hint.");
	REGISTER_CVAR(g_difficultySaveThreshold, 5, VF_CHEAT|VF_READONLY, "Number of times player has to die with same savegame active to trigger difficulty hint.");
	REGISTER_CVAR(g_pp_scale_income, 1, VF_CHEAT, "Scales incoming PP.");
	REGISTER_CVAR(g_pp_scale_price, 1, VF_CHEAT, "Scales PP prices.");
	REGISTER_CVAR(g_energy_scale_price, 0, VF_CHEAT, "Scales energy prices.");
	REGISTER_CVAR(g_energy_scale_income, 1, VF_CHEAT, "Scales incoming energy.");
	REGISTER_CVAR(g_enableFriendlyFallAndPlay, 0, 0, "Enables fall&play feedback for friendly actors.");

	REGISTER_CVAR(g_STAPCameraAnimation, 1, VF_CHEAT, "Enable STAP camera animation");
	
	REGISTER_CVAR(g_mpCoreGameplayTesting, 0, VF_CHEAT, "Used for testing core gameplay - currently disables the nanosuit");
	REGISTER_CVAR(g_mpPermanentSonarVision, 0, VF_READONLY, "Player has Sonar Vision mod always on (for Attackers in Assault mode).");
	REGISTER_CVAR(g_mpPermanentSonarVision_alpha, 111, 0, "Alpha value for the fake gfx used by the above PermanentSonarVision mod. This is for the fill colour.");
	REGISTER_CVAR(g_mpPermanentSonarVision_outline_alpha, 222, 0, "Alpha value for the fake gfx used by the above PermanentSonarVision mod. This is for the outline colour.");
	REGISTER_CVAR(g_mpAllSeeingRadar, 0, VF_READONLY,	"Player has radar that permanently shows all friends and enemies (for Attackers in Assault mode).");
	REGISTER_CVAR(g_mpAllSeeingRadarSv, 0, VF_READONLY,	"All players have radar that permanently shows all friends and enemies, controlled by server.");
	REGISTER_CVAR(g_mpDisableRadar, 0, VF_READONLY,	"Player has no radar (for Defenders when starting Assault mode).");
	REGISTER_CVAR(g_mpNoEnemiesOnRadar, 0, VF_READONLY,	"Player will not see any enemies on radar (for Attackers when Defenders are at high alert level in Assault mode).");
	
	REGISTER_CVAR(g_playerLowHealthThreshold, 20.0f, VF_CHEAT, "The player health threshold when the low health effect kicks in.");
	REGISTER_CVAR(g_punishFriendlyDeaths, 1, VF_CHEAT, "The player gets punished by death when killing a friendly unit.");
	REGISTER_CVAR(g_fallAndPlayThreshold, 50, VF_CHEAT, "Minimum damage for fall and play.");

	REGISTER_CVAR(g_fireStabilizationThreshold, 0.5f, 0, "threshold time player can press the fire button before he can fire then next round.");

	REGISTER_CVAR(g_SurvivorOneVictoryConditions_watchLvl, 0, VF_CHEAT, "Debug watch level for the SurvivorOneVictoryConditions gamerules module.");
	REGISTER_CVAR(g_SimpleEntityBasedObjective_watchLvl, 0, VF_CHEAT, "Debug watch level for the SimpleEntityBasedObjective gamerules module.");
	REGISTER_CVAR(g_CarryObjective_watchLvl, 0, VF_CHEAT, "Debug watch level for the CarryObjective gamerules module.");
	REGISTER_CVAR(g_CombiCaptureObjective_watchLvl, 0, VF_CHEAT, "Debug watch level for the CombiCaptureObjective gamerules module.");
	REGISTER_CVAR(g_CTFScoreElement_watchLvl, 0, VF_CHEAT, "Debug watch level for the CTFScoreElement HUD module.");
	REGISTER_CVAR(g_KingOfTheHillObjective_watchLvl, 0, VF_CHEAT, "Debug watch level for the KingOfTheHill objective implementation.");

	// Depth of Field control
	REGISTER_CVAR(g_dofset_minScale, 1.0f, VF_CHEAT, "Scale Dof_FocusMin param when it gets set Default = 1");
	REGISTER_CVAR(g_dofset_maxScale, 3.0f, VF_CHEAT, "Scale Dof_FocusMax param when it gets set Default = 3");
	REGISTER_CVAR(g_dofset_limitScale, 9.0f, VF_CHEAT, "Scale Dof_FocusLimit param when it gets set Default = 9");
	REGISTER_CVAR(g_dof_minHitScale, 0.25f, VF_CHEAT, "Scale of ray hit distance which Min tries to approach. Default = 0.25");
	REGISTER_CVAR(g_dof_maxHitScale, 2.0f, VF_CHEAT, "Scale of ray hit distance which Max tries to approach. Default = 2.0f");
	REGISTER_CVAR(g_dof_sampleAngle, 5.0f, VF_CHEAT, "Sample angle in degrees. Default = 5");
	REGISTER_CVAR(g_dof_minAdjustSpeed, 100.0f, VF_CHEAT, "Speed that DoF can adjust the min value with. Default = 100");
	REGISTER_CVAR(g_dof_maxAdjustSpeed, 200.0f, VF_CHEAT, "Speed that DoF can adjust the max value with. Default = 200");
	REGISTER_CVAR(g_dof_averageAdjustSpeed, 20.0f, VF_CHEAT, "Speed that the average between min and max can be approached. Default = 20");
	REGISTER_CVAR(g_dof_distAppart, 10.0f, VF_CHEAT, "Minimum distance that max and min can be apart. Default = 10");
	REGISTER_CVAR(g_dof_ironsight, 1, VF_CHEAT, "Enable ironsight dof. Default = 1");

	REGISTER_CVAR(g_bulletPenetrationEnable, 1, VF_CHEAT, "Enables bullet penetration.");
	REGISTER_CVAR(g_bulletPenetrationDebug, 0, VF_CHEAT, "Enable bullet penetration debugging");
	REGISTER_CVAR(g_bulletPenetrationDebugTimeout, 8.0f, VF_CHEAT, "Display time of debug messages");

	REGISTER_CVAR(g_hologram_maxBlockedTime, 0.2f, VF_CHEAT, "Length of time movement must be blocked before hologram will stop running");
	REGISTER_CVAR(g_hologram_initialNoMovementCheckTime, 0.2f, VF_CHEAT, "Length of initial 'don't check for blocked movement' period");
	REGISTER_CVAR(g_hologram_negligibleMovementSqrd, 0.001f, VF_CHEAT, "Minimum speed squared used to check for blocked movement");
	REGISTER_CVAR(g_hologram_spawnOffsetDist, 1.5f, VF_CHEAT, "Distance in front of player to spawn hologram");

	REGISTER_CVAR(g_explosion_materialFX_raycastLength, 1.f, VF_CHEAT, "Length of raycast for non-direct impact explosions to find appropriate surface effect");

	// explosion culling
	REGISTER_CVAR(g_ec_enable, 1, VF_CHEAT, "Enable/Disable explosion culling of small objects. Default = 1");
	REGISTER_CVAR(g_ec_radiusScale, 2.0f, VF_CHEAT, "Explosion culling scale to apply to explosion radius for object query.");
	REGISTER_CVAR(g_ec_volume, 0.75f, VF_CHEAT, "Explosion culling volume which needs to be exceed for objects to not be culled.");
	REGISTER_CVAR(g_ec_extent, 2.0f, VF_CHEAT, "Explosion culling length of an AABB side which needs to be exceed for objects to not be culled.");
	REGISTER_CVAR(g_ec_removeThreshold, 20, VF_CHEAT, "At how many items in exploding area will it start removing items.");
	REGISTER_CVAR(g_radialBlur, 1.0f, VF_CHEAT, "Radial blur on explosions. Default = 1, 0 to disable");
	REGISTER_CVAR(g_playerFallAndPlay, 0, 0, "When enabled, the player doesn't die from direct damage, but goes to fall and play.");

	REGISTER_CVAR(g_corpseMinTime, 8.0f, 0, "Minimum time in seconds that a corpse will be visible");
	REGISTER_CVAR(g_corpseUnseenTime, 3.0f, 0, "Time in seconds that the player has to look away from the corpse before it disappears");
	REGISTER_CVAR(g_corpsePollTime, 1.0f, 0, "Time in seconds where 'unseen' polling is done");
	REGISTER_CVAR(g_corpseMinDistance, 10.0f, 0, "Distance in meters that the player has to be away from the ragdoll before it can disappear");

	REGISTER_CVAR(g_debugaimlook, 0, VF_CHEAT, "Debug aim/look direction");
	REGISTER_CVAR(g_enableIdleCheck, 1, 0,"");

	// Crysis supported gamemode CVars
	REGISTER_CVAR(g_timelimit, 60.0f, 0, "Duration of a time-limited game (in minutes). 0 means no time-limit.");
	REGISTER_CVAR(g_timelimitextratime, 0.0f, VF_READONLY, "TEMPORARILY READ-ONLY AND IN CVARS.TXT UNTIL FRONTEND CAN SET THIS. Amount of time to use as Extra Time/Sudden Death Time (in minutes) for MP game modes that support it. Default is 0, 0 means no extra time.");
	REGISTER_CVAR(g_roundtime, 2.5f, 0, "Duration of a round (in minutes). Default is 2.5, 0 means no time-limit.");
	REGISTER_CVAR(g_preroundtime, 8, 0, "Frozen time before round starts. Default is 8, 0 Disables freeze time.");
	REGISTER_CVAR(g_suddendeathtime, 30, 0, "Number of seconds before round end to start sudden death. Default if 30. 0 Disables sudden death.");
	REGISTER_CVAR(g_roundlimit, 30, 0, "Maximum numbers of rounds to be played. Default is 0, 0 means no limit.");
	REGISTER_CVAR(g_fraglimit, 0, 0, "Number of frags before a round restarts. Default is 0, 0 means no frag-limit.");
	REGISTER_CVAR(g_fraglead, 1, 0, "Number of frags a player has to be ahead of other players once g_fraglimit is reached. Default is 1.");
	REGISTER_CVAR(g_friendlyfireratio, 0.0f, 0, "Sets friendly damage ratio.");
	REGISTER_CVAR(g_revivetime, 7, 0, "Revive wave timer.");
	REGISTER_CVAR(g_autoteambalance, 1, 0, "Enables auto team balance.");
	REGISTER_CVAR(g_autoTeamBalanceAmount, 2, 0, "Amount of players one side is allowed more than the other before auto balancing will occur.");
	REGISTER_CVAR(g_minplayerlimit, 2, 0, "Minimum number of players to start a match.");
	REGISTER_CVAR(g_minteamlimit, 1, 0, "Minimum number of players in each team to start a match.");
	REGISTER_CVAR(g_tk_punish, 1, 0, "Turns on punishment for team kills");
	REGISTER_CVAR(g_tk_punish_limit, 5, 0, "Number of team kills user will be banned for");
	REGISTER_CVAR(g_teamlock, 2, 0, "Number of players one team needs to have over the other, for the game to deny joining it. 0 disables.");
	REGISTER_CVAR(g_scoreLimit, 3, 0, "Max number of points need to win in a team game.  Default is 3");
	REGISTER_CVAR(g_wavesLimit, 3, 0, "Max number of waves in a game of survival.  Default is 3");
	REGISTER_CVAR(g_spawndeathdist, 20, 0, "TIA; reject spawn-points closer than this to death position.");
	REGISTER_CVAR(g_spawn_enemyDistLevelSizeDivider, 6.0f, 0, "calculate spawning enemy reject distance by scaling bounds of level's spawns by this divider");
	REGISTER_CVAR(g_objectivesTimeScale, 1.0f, 0, "Time multiplier for multiplayer objectives");
	REGISTER_CVAR(g_hostMigrationConnectionTimeout, 30.0f, 0, "Time after which players are assumed to have been lost during a host migration");
	REGISTER_CVAR(g_hostMigrationResumeTime, 3.0f, VF_CHEAT, "Time after players have rejoined before the game resumes");

	REGISTER_CVAR(g_spawn_lastKillerDist, 30.0f, 0, "minimum distance between new spawnpoint and the player who last killed you on spawn. May need changing to a divider as with enemy distance");
	REGISTER_CVAR(g_spawn_lastSpawnDist, 10.0f, 0, "minimum distance between new spawnpoint and last spawnpoint you spawned at");
	REGISTER_CVAR(g_spawn_explosiveSafeDist, 7.0f, 0, "minimum distance between new spawnpoint and any explosives");
	REGISTER_CVAR(g_spawn_ctfFlagAvoidDist, 30.0f, 0, "minimum distance between new spawnpoint and any carried CTF flags");

	REGISTER_CVAR(g_inventoryNoLimits, 0, VF_CHEAT, "Removes inventory slots restrictions. Allows carrying as many weapons as desired");
	REGISTER_CVAR(g_inventoryWeaponCapacity, 2, VF_CHEAT, "Capacity for weapons slot in inventory");
	REGISTER_CVAR(g_inventoryExplosivesCapacity, 2, VF_CHEAT, "Capacity for explosives slot in inventory");
	REGISTER_CVAR(g_inventoryGrenadesCapacity, 3, VF_CHEAT, "Capacity for grenades slot in inventory");

	REGISTER_CVAR(g_loadoutServerControlled, 0, 0, "Perk/Weapon Loadout is handed to clients from the server.");

	REGISTER_CVAR(g_debugNetPlayerInput, 0, VF_CHEAT, "Show some debug for player input");
	REGISTER_CVAR(g_debug_fscommand, 0, 0, "Print incoming fscommands to console");
	REGISTER_CVAR(g_debugDirectMPMenu, 0, 0, "Jump directly to MP menu on application start.");
	REGISTER_CVAR(g_skipIntro, 0, VF_NULL, "Skip all the intro videos.");
	REGISTER_CVAR(g_resetActionmapOnStart, 0, 0, "Resets Keyboard mapping on application start.");
	REGISTER_CVAR(g_useProfile, 1, 0, "Don't save anything to or load anything from profile.");
	REGISTER_CVAR(g_startFirstTime, 1, VF_DUMPTODISK, "1 before the game was started first time ever.");

	REGISTER_CVAR(g_killcamEnable, 1, 0, "Enables the KillCam");
	REGISTER_CVAR(g_killcamDebug, 0, VF_CHEAT, "Enables debugging of the KillCam with the P and R keys");
	REGISTER_CVAR(g_killcamPlaybackSyncDelay, 0.05f, VF_CHEAT, "Additional delay added between remote killcam recording and local recording to account for player interpolation");
	REGISTER_CVAR(g_killcamProjectileDistance, 0.3f, VF_CHEAT, "Sets the distance behind which the killcam will follow projectiles");
	REGISTER_CVAR(g_killcamProjectileHeightOffset, 0.05f, VF_CHEAT, "Sets the height offset of the camera during projectile killcam replay");
	REGISTER_CVAR(g_killcamProjectileVictimHeightOffset, 0.65f, VF_CHEAT, "Sets the height offset of the camera focus point during projectile killcam replay");
	REGISTER_CVAR(g_killcamProjectileMinimumVictimDist, 4.3f, VF_CHEAT, "Sets the minimum distance between the victim and the camera");
	REGISTER_CVAR(g_killcamSmoothing, 0.05f, VF_CHEAT, "Sets the amount of camera smoothing to use during projectile replay (range from 0 to less than 1)");
	REGISTER_CVAR(g_killcamGrenadeSmoothingDist, 10.0f, VF_CHEAT, "Sets the distance at which camera smoothing kicks in for grenades");

	REGISTER_CVAR(g_stapEnable, 1, 0, "Enables the STAP system");
	REGISTER_CVAR(g_translationPinningEnable, 1, 0, "Enables Translation Pinning for the client player");

	REGISTER_CVAR(g_MPLedgeGrabbing, 0, 0, "Enables ledge grabbing in MP");
	REGISTER_CVAR(g_MPCoverAndLean,  0, 0, "Enables cover & lean in MP");

	REGISTER_CVAR(g_dogtagsEnable, 1, 0, "Enables dogtags");
	REGISTER_CVAR(g_dogtagsHUDPopupEnable, 1, 0, "Enables the \"dogtag collected\" HUD popup");
	REGISTER_CVAR(g_dogtagsHUDPopupDuration, 3.0f, 0, "How long (in secs) the \"dogtag collected\" popup displays on the HUD for");
	//
	REGISTER_CVAR(g_detachCamera, 0, VF_CHEAT, "Detach camera");
	REGISTER_CVAR(g_moveDetachedCamera, 0, VF_CHEAT, "Move detached camera");
	REGISTER_CVAR(g_detachedCameraMoveSpeed, 6.0f, VF_CHEAT, "Move detached camera");
	REGISTER_CVAR(g_detachedCameraRotateSpeed, 1.5f, VF_CHEAT, "Move detached camera");
	REGISTER_CVAR(g_detachedCameraTurboBoost, 4.0f, VF_CHEAT, "Move speed turbo boost when holding down (360) A button");
	REGISTER_CVAR(g_detachedCameraDebug, 0, VF_CHEAT, "Display debug graphics for detached camera spline playback.");

	REGISTER_CVAR(g_debugCollisionDamage, 0, VF_DUMPTODISK, "Log collision damage");
	REGISTER_CVAR(g_debugHits, 0, VF_DUMPTODISK, "Log hits");

	REGISTER_CVAR(g_trooperTentacleAnimBlend, 0, VF_DUMPTODISK, "Trooper tentacle physic_anim blend (0..1) - overrides the physic_blend AG parameter when it's not 0");
	REGISTER_CVAR(g_trooperBankingMultiplier, 1, VF_DUMPTODISK, "Trooper banking multiplier coeff (0..x)");
	REGISTER_CVAR(g_alienPhysicsAnimRatio, 0.0f, VF_CHEAT,"" );  

	REGISTER_CVAR(g_useHitSoundFeedback, 1, 0, "use hit sound feedback");

	pl_debug_filter = REGISTER_STRING("pl_debug_filter","",VF_CHEAT,"");
	REGISTER_CVAR(pl_debug_movement, 0, VF_CHEAT,"");
	REGISTER_CVAR(pl_debug_jumping, 0, VF_CHEAT,"");
	REGISTER_CVAR(pl_debug_aiming, 0, VF_CHEAT,"");
	REGISTER_CVAR(pl_debug_aiming_input, 0, VF_CHEAT,"");
	REGISTER_CVAR(pl_debug_vistable, 0, VF_CHEAT, "View debug information for vistable");
	REGISTER_CVAR(pl_debug_view, 0, VF_CHEAT,"");

#if !defined(_RELEASE)
	REGISTER_CVAR(pl_debug_watch_camera_mode, 0, VF_CHEAT, "Display on-screen text showing player's current camera mode (requires 'watch_enabled')");
	REGISTER_CVAR(pl_debug_log_camera_mode_changes, 1, VF_CHEAT, "Write CPlayerCamera-related messages to game log");
#endif

	REGISTER_CVAR(pl_aim_cloaked_multiplier, 0.0f, 0, "Multiplier for autoaim slow and follow vs cloaked targets");

	REGISTER_CVAR(pl_aim_acceleration_enabled, 1, 0, "Aim Acceleration: Enable/Disable aim acceleration.");
	REGISTER_CVAR(pl_targeting_debug, 0, 0, "Targeting Assistance: Enable debugging of current target.");
	REGISTER_CVAR(pl_TacticalScanDuration, 2.0f, 0, "Time in seconds it takes to scan a interest point object using the visor.");

	REGISTER_CVAR(pl_switchTPOnKill, 1, VF_CHEAT, "Enable/Disable auto switch to third person on kill in multiplayer");

	REGISTER_CVAR(pl_power_sprint.foward_angle, 45.0f, 0, "Power sprint: Stick angle threshold for sprinting (0.0 - 90.0f");

	REGISTER_CVAR(pl_jump_control.air_control_scale, 1.0f, 0, "Scales base air control while in air");
	REGISTER_CVAR(pl_jump_control.air_resistance_scale, 1.3f, 0, "Scales base air resitance while in air");
	REGISTER_CVAR(pl_jump_control.air_inertia_scale, 0.3f, 0, "Scales inertia while in air");

	REGISTER_CVAR(pl_jump_maxTimerValue, 2.5f, 0, "The maximum value of the timer");
	REGISTER_CVAR(pl_jump_baseTimeAddedPerJump, 0.4f, 0, "The amount of time that is added on per jump");
	REGISTER_CVAR(pl_jump_currentTimeMultiplierOnJump, 1.5f, 0, "Multiplier for the current timer per jump");

	REGISTER_CVAR(pl_jump_quickPressThresh, 0.08f, 0, "If jump button is released quicker than this threshold (in secs) then the jump will be flagged as 'quick'. In Power mode, this will do a normal (Tactical) jump instead of a power jump");

	REGISTER_CVAR(pl_velocityBasedInterpolation, 0, 0, "Use velocity based interpolation method");	
	REGISTER_CVAR(pl_velocityInterpFactor, 3.0f, 0, "Velocity interp factor");	
	
	REGISTER_CVAR(pl_ledgeClamber.debugDraw, 0, VF_CHEAT, "Turn on debug drawing of Ledge Clamber");
	REGISTER_CVAR(pl_ledgeClamber.cameraBlendWeight, 0.6f, VF_CHEAT, "FP camera blending weight when performing ledge grab action.");

	REGISTER_CVAR(pl_pickAndThrow.debugDraw, 0, VF_CHEAT, "Turn on debug drawing of Pick And Throw");

	REGISTER_CVAR(pl_airFrictionPerk_control.fall_activation_height, 4.0f, 0, "Falling height (in meters) to be able to activate the glider");
	REGISTER_CVAR(pl_airFrictionPerk_control.fall_maxSeepd_height, 20.0f, 0, "Falling height (in meters) to reach maximum falling speed when the glider is active");
	REGISTER_CVAR(pl_airFrictionPerk_control.forward_speed, 5.0f, 0, "Forward speed while gliding");
	REGISTER_CVAR(pl_airFrictionPerk_control.min_falling_speed, 4.0f, 0, "Minimum (start) falling speed, when glider activates");
	REGISTER_CVAR(pl_airFrictionPerk_control.max_falling_speed, 17.5f, 0, "Maximum falling speed with glider active");
	REGISTER_CVAR(pl_airFrictionPerk_control.view_pitch_shake, 15.0f, 0, "View shake (up) when activating air friction perk");
	REGISTER_CVAR(pl_airFrictionPerk_control.view_recover_time, 1.0f, 0, "Recover time from initial view shake");

	REGISTER_CVAR(pl_sliding_control.min_speed_threshold, 5.0f, 0, "Min sprinting movement speed to trigger sliding");
	REGISTER_CVAR(pl_sliding_control.min_speed, 5.0f, 0, "Speed threshold at which sliding will stop automatically");
	REGISTER_CVAR(pl_sliding_control.deceleration_speed, 1.5f, 0, "Deceleration while sliding (in m/s)");
	REGISTER_CVAR(pl_sliding_control.deceleration_speed_powerMode, 3.0f, 0, "Deceleration while sliding (in m/s)");
	REGISTER_CVAR(pl_sliding_control.min_downhill_threshold, 5.0f, 0, "Slope angle threshold for down hill sliding");
	REGISTER_CVAR(pl_sliding_control.max_downhill_threshold, 50.0f, 0, "Slope angle threshold at which we reach maximum downhill acceleration");
	REGISTER_CVAR(pl_sliding_control.max_downhill_acceleration, 15.0f, 0, "Extra speed added to sliding, when going down hill (scaled linearly to slope)");

	REGISTER_CVAR(pl_enemy_ramming.player_to_player, 1.0f, 0, "Max damage if both actors are alive (and in movement)");
	REGISTER_CVAR(pl_enemy_ramming.ragdoll_to_player, 50.0f, 0, "Max ragdoll to player damage");
	REGISTER_CVAR(pl_enemy_ramming.fall_damage_threashold, 4.0f, 0, "Damage threshold to activate fall and play");
	REGISTER_CVAR(pl_enemy_ramming.safe_falling_speed, 6.0f, 0, "the falling speed the player starts to hurt the enemy from falling");
	REGISTER_CVAR(pl_enemy_ramming.fatal_falling_speed, 10.0f, 0, "the speed at where the falling is always fatal to the enemy");
	REGISTER_CVAR(pl_enemy_ramming.max_falling_damage, 250.0f, 0, "the maximum hit damage to the enemy can receive from being rammed from a falling player");

	REGISTER_CVAR(pl_nano_vision.max_distance_visible, 50.0f, 0, "maximum distance the nano vision helper objects are still visible");

	REGISTER_CVAR(pl_melee.melee_snap_angle_limit, 90.0f, 0, "Angle limit (in degrees) in which melee snap applies");
	REGISTER_CVAR(pl_melee.melee_snap_blend_speed, 0.33f, 0, "Smooths transition to target direction. Range[0.0 - 1.0f]. Set to 1 for instant turning");
	REGISTER_CVAR(pl_melee.raycast_max_height, 1.15f, 0, "If the melee raycast height above the ground is more than this then it'll be lowered to this");
	REGISTER_CVAR(pl_melee.debug_gfx, 0, 0, "Enable/Disables debug gfx for melees");
	
	REGISTER_CVAR(pl_health.normal_regeneration_rateSP, 250.0f, 0, "Regeneration rate in points per second");
	REGISTER_CVAR(pl_health.normal_threshold_time_to_regenerateSP, 4.0f, 0, "Time between hits to start regeneration");

	REGISTER_CVAR(pl_health.normal_regeneration_rateMP, 50.0f, 0, "Regeneration rate in points per second");
	REGISTER_CVAR(pl_health.normal_threshold_time_to_regenerateMP, 6.0f, 0, "Time between hits to start regeneration");

	REGISTER_CVAR(pl_health.enable_FallandPlay, 0, 0, "Enables/Disables fall&play for the player");
	REGISTER_CVAR(pl_health.collision_health_threshold, 41, 0, "Collision damage will never put the player below this health");

	REGISTER_CVAR(pl_health.fallDamage_SpeedSafe, 10.0f, VF_CHEAT, "Safe fall speed.");
	REGISTER_CVAR(pl_health.fallDamage_SpeedFatal, 17.0f, VF_CHEAT, "Fatal fall");
	REGISTER_CVAR(pl_health.fallDamage_CurveAttack, 2.0f, VF_CHEAT, "Damage curve attack for medium fall speed");
	REGISTER_CVAR(pl_health.fallDamage_health_threshold, 1, VF_CHEAT, "Falling damage will never pu the player below this health, unless falling above fatal speed");
	REGISTER_CVAR(pl_health.debug_FallDamage, 0, VF_CHEAT, "Enables console output of fall damage information.");
	//REGISTER_CVAR(pl_health.mercy_time, 3.0f, 0, "Mercy time");

	REGISTER_CVAR(pl_movement.nonCombat_heavy_weapon_speed_scale, 0.75f, 0, "Base speed multiplier in non combat mode while carrying a heavy (ripped-off) weapon");
	REGISTER_CVAR(pl_movement.nonCombat_heavy_weapon_sprint_scale, 1.25f, 0, "Spring speed multiplier in non combat mode while carrying a heavy (ripped-off) weapon");
	REGISTER_CVAR(pl_movement.power_sprint_targetFov, 51.0f, 0, "Fov while sprinting in power mode");
	REGISTER_CVAR(pl_movement.sprint_timeInAirToStopSprinting, 0.3f, 0, "Amount of time (in seconds) the player can be in the air and still be considered sprinting (helps you get over small bumps)");

	REGISTER_CVAR(pl_mike_debug, 1, 0, "Show debug information for MIKE weapon");
	REGISTER_CVAR(pl_cover_and_lean.enable, 1, 0, "enabled cover and lean");
	REGISTER_CVAR(pl_cover_and_lean.debug_draw, 0, 0, "Show debug information for cover and lean mechanic");
	REGISTER_CVAR(pl_cover_and_lean.num_detection_rays, 12, 0, "configures the number of rays needed to do the edge detection for each side");
	REGISTER_CVAR(pl_cover_and_lean.enable_view_clamp, 1, 0, "clamps the view to the detected cove and lean edges");

	REGISTER_CVAR(aln_debug_movement, 0, VF_CHEAT,"");
	aln_debug_filter = REGISTER_STRING("aln_debug_filter","",VF_CHEAT,"");

	REGISTER_CVAR(g_assertWhenVisTableNotUpdatedForNumFrames, 255, 0, "");

	// emp grenade
	REGISTER_CVAR(g_empStyle, 0, VF_CHEAT, "");

	// ui2 cvars
	
	REGISTER_CVAR(hud_DeathRedScreen, 1, 0, "When the Player gets in the MercyTime health range, the screen turns red.");
	REGISTER_CVAR(hud_ContextualHealthIndicator, 0, 0, "Display the HealthBar directly on the enemy screen position when scanned.");

	// hud cvars
	REGISTER_CVAR(hud_colorLine, 4481854, 0, "HUD line color.");
	REGISTER_CVAR(hud_colorOver, 14125840, 0, "HUD hovered color.");
	REGISTER_CVAR(hud_colorText, 12386209, 0, "HUD text color.");
	REGISTER_CVAR(hud_voicemode, 1, 0, "Usage of the voice when switching of Nanosuit mode.");
	CRY_FIXME(1,10,09, "GAME_WARS2 difference on cvar");
	//REGISTER_CVAR(hud_showAllObjectives, 1, 0, "Show all on screen objectives, not only the active one.");
	REGISTER_CVAR(hud_showAllObjectives, 0, 0, "Show all on screen objectives, not only the active one.");
//	REGISTER_CVAR(hud_subtitles, 0,0,"Subtitle mode. 0==Off, 1=All, 2=CutscenesOnly")->SetOnChangeCallback(CHUD::OnSubtitleCVarChanged);
	REGISTER_CVAR(hud_subtitlesDebug, 0,0,"Debug subtitles");
	REGISTER_CVAR(hud_subtitlesRenderMode, 0,0,"Subtitle RenderMode. 0==Flash, 1=3DEngine");
	REGISTER_CVAR(hud_subtitlesFontSize, 16, 0, "FontSize for Subtitles.");
	REGISTER_CVAR(hud_subtitlesHeight, 10, 0,"Height of Subtitles in Percent. Normally same as hud_PanoramicHeight");
	REGISTER_CVAR(hud_subtitlesShowCharName, 1, 0,"Show Character talking along with Subtitle");
	REGISTER_CVAR(hud_subtitlesQueueCount, 1, 0,"Maximum amount of subtitles in Update Queue");
	REGISTER_CVAR(hud_subtitlesVisibleCount, 1, 0,"Maximum amount of subtitles in Visible Queue");
	REGISTER_CVAR(hud_startPaused, 1, 0, "The game starts paused, waiting for user input.");
	REGISTER_CVAR(hud_faderDebug, 0, 0, "Show Debug Information for FullScreen Faders.");
	REGISTER_CVAR(hud_DebugFrontEnd, 0, 0, "Show Debug Informations for the FrontEnd.");// FIXME : Define this here or in FrontEnd.cpp not both.
	REGISTER_CVAR(hud_UseFlashFrontEndLoading, 0, 0, "Use the flash front end loading screen instead of the texture front end loading screen.");

	REGISTER_CVAR(hud_objectiveIcons_attachToAllBullets, 0, 0, "Create an objective icon for each bullet launched (for testing/debugging objective icon code)");
	REGISTER_CVAR(hud_objectiveIcons_flashTime, 1.6f, 0, "Time between icon changes for flashing objective icons");

	REGISTER_CVAR(hud_mapEnable, 1, 0, "Enables the map (on m key and from the menu in mp)");	
	REGISTER_CVAR(hud_mapOverrideCenterX, 0.0f, 0, "Moves the center of the map at runtime");	
	REGISTER_CVAR(hud_mapOverrideCenterY, 0.0f, 0, "Moves the center of the map at runtime");	
 
	// Controller aim helper cvars
	REGISTER_CVAR(aim_assistMinDistance, 0.0f, 0, "The minimum range at which autoaim operates");
	REGISTER_CVAR(aim_assistMaxDistance, 75.0f, 0, "The maximum range at which autoaim operates");
	REGISTER_CVAR(aim_assistSprintMultiplier, 2.0f, 0, "The intensity of aim assistance while sprinting in power mode");
	REGISTER_CVAR(aim_assistSlidingMultiplier, 8.0f, 0, "The intensity of aim assistance while sliding in power mode");
	REGISTER_CVAR(aim_assistGlidingMultiplier, 2.0f, 0, "The intensity of aim assistance while gliding in power mode");
	REGISTER_CVAR(aim_assistMinTurnScale, 0.45f, 0, "The minimum turn speed as a fraction when aimed right at a target.");
	REGISTER_CVAR(aim_assistSlowFalloffStartDistance, 5.0f, 0, "The minimum range at which autoaim operates");
	REGISTER_CVAR(aim_assistSlowDisableDistance, 100.f, 0, "The minimum range at which autoaim operates");
	REGISTER_CVAR(aim_assistSlowDistanceModifier, 1.0f, 0, "Modify the distance for slowing by this amount.");
	REGISTER_CVAR(aim_assistSlowThresholdOuter, 2.7f, 0, "The minimum turn speed as a fraction when aimed right at a target.");	

	// Controller control
	REGISTER_CVAR(hud_aspectCorrection, 0, 0, "Aspect ratio corrections for controller rotation: 0-off, 1-direct, 2-inverse");

	REGISTER_CVAR(controller_power_curve, 2.5f, 0, "Analog controller input curve for both axes");

	REGISTER_CVAR(controller_multiplier_z, 2.0f, VF_RESTRICTEDMODE, "Horizontal linear sensitivity multiplier");
	REGISTER_CVAR(controller_multiplier_x, 1.5f, VF_RESTRICTEDMODE, "Vertical linear sensitivity multiplier");
	
	REGISTER_CVAR(controller_full_turn_multiplier_x, 1.6f, VF_RESTRICTEDMODE, "The multiplier applied when x rotation has been at full lock for the required time");
	REGISTER_CVAR(controller_full_turn_multiplier_z, 2.35f, VF_RESTRICTEDMODE, "The multiplier applied when x rotation has been at full lock for the required time");

	REGISTER_CVAR(hud_ctrlZoomMode, 0, 0, "Weapon aiming mode with controller. 0 is same as mouse zoom, 1 cancels at release");

	REGISTER_CVAR(ctrlr_OUTPUTDEBUGINFO, 0, VF_RESTRICTEDMODE, "Enable controller debugging.");
	REGISTER_CVAR(ctrlr_acceleration_vertical_up, 275.0f, VF_RESTRICTEDMODE, "How quickly gets to the desired control value when accelerating on curve 1");
	REGISTER_CVAR(ctrlr_acceleration_vertical_down, 1500.0f, VF_RESTRICTEDMODE, "How quickly gets to the desired control value when decelerating on curve 1");
	REGISTER_CVAR(ctrlr_acceleration_horizontal_up, 255.0f, VF_RESTRICTEDMODE, "How quickly gets to the desired control value when accelerating on curve 1");
	REGISTER_CVAR(ctrlr_acceleration_horizontal_down, 1600.0f, VF_RESTRICTEDMODE, "How quickly gets to the desired control value when decelerating on curve 1");
	REGISTER_CVAR(ctrlr_exponent, 1.425f, VF_RESTRICTEDMODE, "Sensitivity exponent");
	REGISTER_CVAR(ctrlr_timeaccel_multiplier_vertical, 4.5f, VF_RESTRICTEDMODE, "The speed at which the vertical acceleration drops with respect to time on curve 2");
	REGISTER_CVAR(ctrlr_timeaccel_multiplier_horizontal, 5.5f, VF_RESTRICTEDMODE, "The speed at which the horizontal acceleration drops with respect to time on curve 2");
	REGISTER_CVAR(ctrlr_timeaccel_base_multiplier_vertical, 1.4f, VF_RESTRICTEDMODE, "The base vertical acceleration on curve 2");
	REGISTER_CVAR(ctrlr_timeaccel_base_multiplier_horizontal, 1.35f, VF_RESTRICTEDMODE, "The base horizontal acceleration on curve 2");


	REGISTER_CVAR(ctrlr_maxspeed_horizontal, 105.0f, VF_RESTRICTEDMODE, "Max horizontal turn speed");
	REGISTER_CVAR(ctrlr_maxspeed_vertical, 75.0f, VF_RESTRICTEDMODE, "Max vertical turn speed");
	REGISTER_CVAR(ctrlr_timeaccel_boost_time, 0.25f, VF_RESTRICTEDMODE, "Time over which the boosted turn rate is lerped down");




	REGISTER_CVAR(vehicle_steering_curve, 2.5, 0, "Analogue controller vehicle steering curve");
	REGISTER_CVAR(vehicle_steering_curve_scale, 1, 0, "Analogue controller vehicle steering curve scale");
	REGISTER_CVAR(vehicle_acceleration_curve, 2.5, 0, "Analogue controller vehicle acceleration curve");
	REGISTER_CVAR(vehicle_acceleration_curve_scale, 1, 0, "Analogue controller vehicle acceleration curve scale");
	REGISTER_CVAR(vehicle_deceleration_curve, 2.5, 0, "Analogue controller vehicle deceleration curve");
	REGISTER_CVAR(vehicle_deceleration_curve_scale, 1, 0, "Analogue controller vehicle steering deceleration scale");

	// Alternative player input normalization code
	REGISTER_CVAR(aim_altNormalization.enable, 0, 0, "Enables/disables alternative input code");
	REGISTER_CVAR(aim_altNormalization.hud_ctrl_Curve_Unified, 2.5f, 0, "Analog controller rotation curve");
	REGISTER_CVAR(aim_altNormalization.hud_ctrl_Coeff_Unified, 1.0f, 0, "Analog controller rotation scale"); 
	// Alternative player input normalization code
		
	//movement cvars
#if !defined(_RELEASE)
	REGISTER_CVAR(v_debugMovement, 0, VF_CHEAT, "Cheat mode, freeze the vehicle and activate debug movement");    
	REGISTER_CVAR(v_debugMovementMoveVertically, 0.f, VF_CHEAT, "Add this value to the vertical position of the vehicle");    
	REGISTER_CVAR(v_debugMovementX, 0.f, VF_CHEAT, "Add this rotation to the x axis");    
	REGISTER_CVAR(v_debugMovementY, 0.f, VF_CHEAT, "Add this rotation to the y axis");    
	REGISTER_CVAR(v_debugMovementZ, 0.f, VF_CHEAT, "Add this rotation to the z axis");    
	REGISTER_CVAR(v_debugMovementSensitivity, 30.f, VF_CHEAT, "if v_debugMovement is set you can rotate the vehicle, this controls the speed");    
#endif

	REGISTER_CVAR(v_profileMovement, 0, 0, "Used to enable profiling of the current vehicle movement (1 to enable)");    
	REGISTER_CVAR(v_pa_surface, 1, VF_CHEAT, "Enables/disables vehicle surface particles");
	REGISTER_CVAR(v_wind_minspeed, 0.f, VF_CHEAT, "If non-zero, vehicle wind areas always set wind >= specified value");
	REGISTER_CVAR(v_draw_suspension, 0, VF_DUMPTODISK, "Enables/disables display of wheel suspension, for the vehicle that has v_profileMovement enabled");
	REGISTER_CVAR(v_draw_slip, 0, VF_DUMPTODISK, "Draw wheel slip status");  
	REGISTER_CVAR(v_invertPitchControl, 0, VF_DUMPTODISK, "Invert the pitch control for driving some vehicles, including the helicopter and the vtol");
	REGISTER_CVAR(v_sprintSpeed, 0.f, 0, "Set speed for acceleration measuring");
	REGISTER_CVAR(v_rockBoats, 1, 0, "Enable/disable boats idle rocking");  
	REGISTER_CVAR(v_dumpFriction, 0, 0, "Dump vehicle friction status");
	REGISTER_CVAR(v_debugSounds, 0, 0, "Enable/disable vehicle sound debug drawing");
	REGISTER_CVAR(v_debugMountedWeapon, 0, 0, "Enable/disable vehicle mounted weapon camera debug draw");

	pAltitudeLimitCVar = REGISTER_CVAR(v_altitudeLimit, v_altitudeLimitDefault(), VF_CHEAT, "Used to restrict the helicopter and VTOL movement from going higher than a set altitude. If set to zero, the altitude limit is disabled.");
	REGISTER_CVAR(v_help_tank_steering, 0, 0, "Enable tank steering help for AI");

	REGISTER_CVAR(v_stabilizeVTOL, 0.35f, VF_DUMPTODISK, "Specifies if the air movements should automatically stabilize");

	CRY_FIXME(1,10,09, "GAME_WARS2 difference on cvar");
	//REGISTER_CVAR(pl_grabOntoLedges, 0, VF_CHEAT, "Enable/Disable grabbing onto ledges (Set to '2' to enable debugging)");
	REGISTER_CVAR(pl_grabOntoLedges, 1, VF_CHEAT, "Enable/Disable grabbing onto ledges (Set to '2' to enable debugging)");

	REGISTER_CVAR(pl_swimBaseSpeed, 4.0f, VF_CHEAT, "Swimming base speed.");
	REGISTER_CVAR(pl_swimGetOutBoost, 11.0f, VF_CHEAT, "vertical velocity boost for leaving water to aid getting out");
	REGISTER_CVAR(pl_swimGetOutOffset, -1.0f, VF_CHEAT, "Offset for depth boosting for water getout");
	REGISTER_CVAR(pl_swimBackSpeedMul, 0.8f, VF_CHEAT, "Swimming backwards speed mul.");
	REGISTER_CVAR(pl_swimSideSpeedMul, 0.9f, VF_CHEAT, "Swimming sideways speed mul.");
	REGISTER_CVAR(pl_swimVertSpeedMul, 0.5f, VF_CHEAT, "Swimming vertical speed mul.");
	REGISTER_CVAR(pl_swimNormalSprintSpeedMul, 1.5f, VF_CHEAT, "Swimming Non-Speed sprint speed mul.");
	REGISTER_CVAR(pl_swimSpeedSprintSpeedMul, 2.5f, VF_CHEAT, "Swimming Speed sprint speed mul.");
	REGISTER_CVAR(pl_swimUpSprintSpeedMul, 2.0f, VF_CHEAT, "Swimming sprint while looking up (dolphin rocket).");

	// animation triggered footsteps
	REGISTER_CVAR(g_FootstepSoundsFollowEntity, 1, VF_CHEAT, "Toggles moving of footsteps sounds with it's entity.");
	REGISTER_CVAR(g_FootstepSoundsDebug, 0, VF_CHEAT, "Toggles debug messages of footstep sounds.");
	REGISTER_CVAR(g_footstepSoundMaxDistanceSq, 900.0f, 0, "Maximum squared distance for footstep sounds / fx spawned by Players.");

	// weapon system
	i_debuggun_1 = REGISTER_STRING("i_debuggun_1", "ai_statsTarget", VF_DUMPTODISK, "Command to execute on primary DebugGun fire");
	i_debuggun_2 = REGISTER_STRING("i_debuggun_2", "ai_BehaviorStatsTarget", VF_DUMPTODISK, "Command to execute on secondary DebugGun fire");

	REGISTER_CVAR(tracer_min_distance, 4.0f, 0, "Distance at which to start scaling/lengthening tracers.");
	REGISTER_CVAR(tracer_max_distance, 50.0f, 0, "Distance at which to stop scaling/lengthening tracers.");
	REGISTER_CVAR(tracer_min_scale, 1.0f, 0, "Scale at min distance.");
	REGISTER_CVAR(tracer_max_scale, 5.0f, 0, "Scale at max distance.");
	REGISTER_CVAR(tracer_max_count, 32, 0, "Max number of active tracers.");
	REGISTER_CVAR(tracer_player_radiusSqr, 400.0f, 0, "Sqr Distance around player at which to start decelerate/acelerate tracer speed.");

	REGISTER_CVAR(i_debug_projectiles, 0, VF_CHEAT, "Displays info about projectile status, where available.");
	REGISTER_CVAR(i_debug_weaponActions, 0, VF_CHEAT, "Displays info about weapon actions that are happening (1 shows only new actions, 2 shows you when duplicate actions are called)");
	REGISTER_CVAR(i_debug_recoil, 0, VF_CHEAT, "Displays info about current recoil");
	REGISTER_CVAR(i_debug_spread, 0, VF_CHEAT, "Displays info about current spread");
	REGISTER_CVAR(i_auto_turret_target, 1, VF_CHEAT, "Enables/Disables auto turrets aquiring targets.");
	REGISTER_CVAR(i_auto_turret_target_tacshells, 0, 0, "Enables/Disables auto turrets aquiring TAC shells as targets");

	REGISTER_CVAR(i_debug_itemparams_memusage, 0, VF_CHEAT, "Displays info about the item params memory usage");
	REGISTER_CVAR(i_debug_weaponparams_memusage, 0, VF_CHEAT, "Displays info about the weapon params memory usage");
	REGISTER_CVAR(i_debug_zoom_mods, 0, VF_CHEAT, "Use zoom mode spread/recoil mods");
	REGISTER_CVAR(i_debug_sounds, 0, VF_CHEAT, "Enable item sound debugging");
	REGISTER_CVAR(i_debug_turrets, 0, VF_CHEAT, 
		"Enable GunTurret debugging.\n"
		"Values:\n"
		"0:  off\n"
		"1:  basics\n"
		"2:  prediction\n"
		"3:  sweeping\n"
		"4:  searching\n"      
		"5:  deviation\n"
		"6:  Always Hostile (will shoot at you)\n"
		);
	REGISTER_CVAR(i_debug_mp_flowgraph, 0, VF_CHEAT, "Displays info on the MP flowgraph node");

	// quick game

	g_quickGame_map = REGISTER_STRING("g_quickGame_map","",VF_DUMPTODISK, "QuickGame option");
	g_quickGame_mode = REGISTER_STRING("g_quickGame_mode","PowerStruggle", VF_DUMPTODISK, "QuickGame option");
	REGISTER_CVAR(g_quickGame_min_players,0,VF_DUMPTODISK,"QuickGame option");
	REGISTER_CVAR(g_quickGame_prefer_lan,0,VF_DUMPTODISK,"QuickGame option");
	REGISTER_CVAR(g_quickGame_prefer_favorites,0,VF_DUMPTODISK,"QuickGame option");
	REGISTER_CVAR(g_quickGame_prefer_my_country,0,VF_DUMPTODISK,"QuickGame option");
	REGISTER_CVAR(g_quickGame_ping1_level,80,VF_DUMPTODISK,"QuickGame option");
	REGISTER_CVAR(g_quickGame_ping2_level,170,VF_DUMPTODISK,"QuickGame option");

	REGISTER_CVAR(g_quickGame_debug,0,VF_CHEAT,"QuickGame option");

	REGISTER_CVAR(g_displayIgnoreList,1,VF_DUMPTODISK,"Display ignore list in chat tab.");
	REGISTER_CVAR(g_buddyMessagesIngame,1,VF_DUMPTODISK,"Output incoming buddy messages in chat while playing game.");

	REGISTER_CVAR(g_showIdleStats, 0, VF_NULL, "");

	// battledust
	REGISTER_CVAR(g_battleDust_enable, 1, 0, "Enable/Disable battledust");
	REGISTER_CVAR(g_battleDust_debug, 0, 0, "0: off, 1: text, 2: text+gfx");
	g_battleDust_effect = REGISTER_STRING("g_battleDust_effect", "misc.battledust.light", 0, "Sets the effect to use for battledust");

	REGISTER_CVAR(g_PSTutorial_Enabled, 1, 0, "Enable/disable powerstruggle tutorial");

	REGISTER_CVAR(sv_votingTimeout, 60, 0, "Voting timeout");
	REGISTER_CVAR(sv_votingCooldown, 180, 0, "Voting cooldown");
	REGISTER_CVAR(sv_votingRatio, 0.51f, 0, "Part of player's votes needed for successful vote.");
	REGISTER_CVAR(sv_votingTeamRatio, 0.67f, 0, "Part of team member's votes needed for successful vote.");

	REGISTER_CVAR(sv_input_timeout, 0, 0, "Experimental timeout in ms to stop interpolating client inputs since last update.");

	sv_aiTeamName = REGISTER_STRING("sv_aiTeamName", "", 0, "Team name for AIs");
	performance_profile_logname = REGISTER_STRING("performance_profile_logname", "performance.log", 0, "Filename for framerate and memory logging.");

	REGISTER_CVAR(g_spectate_TeamOnly, 1, 0, "If true, you can only spectate players on your team");
	REGISTER_CVAR(g_spectate_cctv_RotScale, 5.0f, 0, "Scaling applied to Spectator CCTV mode rotating");
	REGISTER_CVAR(g_spectate_DisableManual, 0, 0, "");
	REGISTER_CVAR(g_spectate_DisableSpawnOptOut, 0, 0, "");
	REGISTER_CVAR(g_spectate_DisableDead, 0, 0, "");
	REGISTER_CVAR(g_spectate_DisableFree, 0, 0, "");
	REGISTER_CVAR(g_spectate_DisableFollow, 0, 0, "");
	REGISTER_CVAR(g_spectate_DisableCCTV, 0, 0, "");

	REGISTER_CVAR(g_claymore_limit, 3, 0, "Max claymores a player can place (recycled above this value)");
	REGISTER_CVAR(g_avmine_limit, 3, 0, "Max avmines a player can place (recycled above this value)");
	REGISTER_CVAR(g_debugMines, 0, 0, "Enable debug output for mines and claymores");

	REGISTER_CVAR(aim_assistCrosshairSize, 25, VF_CHEAT, "screen size used for crosshair aim assistance");
	REGISTER_CVAR(aim_assistCrosshairDebug, 0, VF_CHEAT, "debug crosshair aim assistance");

	REGISTER_CVAR(g_deathCam, 1, 0, "Enables / disables the MP death camera (shows the killer's location)");

	REGISTER_CVAR(g_tpdeathcam_focalHeightBump, /*0.8f*/0.f, 0, "Focal target height gets bumped by this");
	REGISTER_CVAR(g_tpdeathcam_initDist, /*3.75f*/6.f, 0, "Initial camera distance from dying player");
	REGISTER_CVAR(g_tpdeathcam_defaultCameraHeightOffset, /*0.3f*/0.f, 0, "Default camera height above focal target");
	REGISTER_CVAR(g_tpdeathcam_collideFocalRaiseFrac, 0.2f, 0, "If collided, fraction of additional camera raise height the focal point is also raised by");

	REGISTER_CVAR(sv_pacifist, 0, 0, "Pacifist mode (only works on dedicated server)");

	REGISTER_CVAR(g_nanoSuitMenuToggleTime, 0.0f, 0, "Time that you have to keep pressing suit menu button to enter the menu");
	REGISTER_CVAR(g_nanoSuitEnableSuitShapeDeformation, 1, VF_CHEAT, "Enables/Disables shape deformation on nano suit");
	REGISTER_CVAR(g_logSuitActions, 0, 0, "Log suit action processing");

	REGISTER_CVAR(g_teamSilhouette.enableUpdate, 0, 0, "Enable updating of team silhouette each frame (useful when modifying these numbers)");
	REGISTER_CVAR(g_teamSilhouette.r, 0.20f, 0, "team silhouette red");
	REGISTER_CVAR(g_teamSilhouette.g, 0.75f, 0, "team silhouette green");
	REGISTER_CVAR(g_teamSilhouette.b, 1.00f, 0, "team silhouette blue");
	REGISTER_CVAR(g_teamSilhouette.a, 2.0f, 0, "team silhouette alpha");
	REGISTER_CVAR(g_teamSilhouette.enabled, 0, 0, "turn on/off team silhouettes");

	REGISTER_CVAR(g_enemySilhouette.enableUpdate, 0, 0, "Enable updating of enemy silhouette each frame (useful when modifying these numbers)");
	REGISTER_CVAR(g_enemySilhouette.r, 1.00f, 0, "enemy silhouette red");
	REGISTER_CVAR(g_enemySilhouette.g, 0.20f, 0, "enemy silhouette green");
	REGISTER_CVAR(g_enemySilhouette.b, 0.20f, 0, "enemy silhouette blue");
	REGISTER_CVAR(g_enemySilhouette.a, 2.0f, 0, "enemy silhouette alpha");
	REGISTER_CVAR(g_enemySilhouette.enabled, 0, 0, "turn on/off enemy silhouettes");

	REGISTER_CVAR(g_volatileSpike.decalLife, 10.0f, 0, "Maximum life of decals");
	REGISTER_CVAR(g_volatileSpike.decalMinSize, 0.5f, 0, "Minimum size of acid decals");
	REGISTER_CVAR(g_volatileSpike.decalMaxSize, 3.0f, 0, "Maximum size of acid decals");
	REGISTER_CVAR(g_volatileSpike.decalEmitterLife, 0.5f, 0, "Maximum life of decal emitter");
	REGISTER_CVAR(g_volatileSpike.decalGrowTime, 0.1f, 0, "Time it takes for decal to grow");
	REGISTER_CVAR(g_volatileSpike.maxRandomSteamEmitters, 2, 0, "Maximum number of steam emitters");
	REGISTER_CVAR(g_volatileSpike.maxDecals, 12, 0, "Maximum number of acid decals");

	REGISTER_CVAR(g_postEffect.FilterGrain_Amount, 0.0f, 0, "Filter grain amount");
	REGISTER_CVAR(g_postEffect.FilterRadialBlurring_Amount, 0.0f, 0, "Radial blurring amount");
	REGISTER_CVAR(g_postEffect.FilterRadialBlurring_ScreenPosX, 0.5f, 0, "Radial blurring screen position X");
	REGISTER_CVAR(g_postEffect.FilterRadialBlurring_ScreenPosY, 0.5f, 0, "Radial blurring screen position Y");
	REGISTER_CVAR(g_postEffect.FilterRadialBlurring_Radius, 1.0f, 0, "Radial blurring radius");
	REGISTER_CVAR(g_postEffect.Global_User_ColorC, 0.0f, 0, "Global cyan");
	REGISTER_CVAR(g_postEffect.Global_User_ColorM, 0.0f, 0, "Global magenta");
	REGISTER_CVAR(g_postEffect.Global_User_ColorY, 0.0f, 0, "Global yellow");
	REGISTER_CVAR(g_postEffect.Global_User_ColorK, 0.0f, 0, "Global luminance");
	REGISTER_CVAR(g_postEffect.Global_User_Brightness, 1.0f, 0, "Global brightness");
	REGISTER_CVAR(g_postEffect.Global_User_Contrast, 1.0f, 0, "Global contrast");
	REGISTER_CVAR(g_postEffect.Global_User_Saturation, 1.0f, 0, "Global saturation");
	REGISTER_CVAR(g_postEffect.Global_User_ColorHue, 0.0f, 0, "Global color hue");

	REGISTER_CVAR(g_gameFXSystemDebug, 0, 0, "Toggles game effects system debug state");
	
	REGISTER_CVAR(g_hasWindowFocus, 1, 0, "");
	REGISTER_CVAR(coop_respawn_time, 15, 0, "Time until player gets auto-respawned in Coop.");

	pVehicleQuality = pConsole->GetCVar("v_vehicle_quality");		assert(pVehicleQuality);

	REGISTER_CVAR(g_displayPlayerDamageTaken, 0, 0, "Display the amount of damage being taken above a player who's getting injured");
	REGISTER_CVAR(g_displayDbgText_perk, 0, 0, "Show perk-related debugging text on the screen");
	REGISTER_CVAR(g_displayDbgText_hud, 0, 0, "Show HUD-related debugging text on the screen");
	REGISTER_CVAR(g_displayDbgText_plugins, 0, 0, "Show player plug-in-related debugging text on the screen");
	REGISTER_CVAR(g_displayDbgText_pmv, 0, 0, "Show list of 'player modifiable values' on the screen");
	REGISTER_CVAR(g_displayDbgText_psGuns, 0, 0, "Show info about Power Struggle Lite gun turrets on the screen");
	REGISTER_CVAR(g_displayDbgText_actorState, 0, 0, "Show information (health, current state etc.) about each actor");
	
#if DEBUG_NEW_SPAWNING
	REGISTER_CVAR(g_debug_spawn_mode, 2, 0, "DEBUG - temporary enabling/disabling of new spawn modes: Values: 0=normal; 1=HQ spawn mode; 2=Frontline spawn mode");
	REGISTER_CVAR(g_debug_spawning_visually, 0, 0, "debug spawning visually. For use in the editor.");
	g_debug_spawning_visually_teamA = REGISTER_STRING("visual_spawn_team_A", "Grunt", 0, "default actor class to consider as teamA (friendlies) when debugging spawning visually");
	g_debug_spawning_visually_teamB = REGISTER_STRING("visual_spawn_team_B", "Pinger", 0, "default actor class to consider as teamB (enemies) when debugging spawning visually");
	REGISTER_CVAR(g_debug_entity_clustering, 0, 0, "debug RDC clustering code on all entities: Values: 0=off; 1:consider entities; 2: consider actors");
	REGISTER_CVAR(g_debug_entity_clustering_radius, 10.0f, 0, "radius to use for debug RDC clustering code on all entities");
#endif // DEBUG_NEW_SPAWNING
	REGISTER_CVAR(g_spawn_conflict_clustering_radius, 7.0f, 0, "the radius that the conflict detection algorithm uses when detecting clusters");
	REGISTER_CVAR(g_spawn_vistable_show, 0, 0, "on the server show the vistable visability - 1: show visibility of spawns; 2: show hierarchies of vistesting and non-vistesting spawns");
	REGISTER_CVAR(g_spawn_vistable_numLineTestsPerFrame, 5, 0, "on the server - the number of linetests the vistable should do per frame");
	REGISTER_CVAR(g_spawn_vistable_maxDistToAltSpawnEntity, 4.0f, 0, "on the server - the maximum distance a non vistesting spawn can be away from a vistesting spawn to automatically setup the vistesting spawn as its altSpawnEntity");

	REGISTER_CVAR(g_SuitModeSlowMotionRatio, 1.0f, 0,"Causes a slow motion effect when opening nano suit menu. Default=1.0f (disabled) the lower the ratio value, the higher the slow mo");


	REGISTER_CVAR(g_showShadowChar, false, 0, "Render the shadow casting character");
	REGISTER_CVAR(g_animateCameraOrientation, false, 0, "Locks the player view to the camera bone's orientation");

	REGISTER_CVAR(g_infiniteAmmo, false, 0, "Infinite inventory ammo!");

	REGISTER_CVAR(g_animatorDebug, false, 0, "Animator Debug Info");
	REGISTER_CVAR(g_hideArms, false, 0, "Hide arms in first person");
	REGISTER_CVAR(g_useHitReactController, true, 0, "Use the hit react controller");
	REGISTER_CVAR(g_debugHitReacts, false, 0, "Debug the hit reacts");

#if !defined(_RELEASE)
	REGISTER_CVAR(g_EAWhiteBuild, false, 0, "Internal release, switch on White Build"); // CW2
	REGISTER_CVAR(g_playerUsesDedicatedInput, 0, 0, "player's will use the automated dedicated input");
	REGISTER_CVAR(g_playerEnableDedicatedInput, 1, 0, "dedicated clients do random movements and actions");
#endif
	
	REGISTER_CVAR(watch_enabled, 1, 0, "On-screen watch text is enabled/disabled");
	REGISTER_CVAR(watch_text_render_start_pos_x, 35.0f, 0, "On-screen watch text render start x position");
	REGISTER_CVAR(watch_text_render_start_pos_y, 180.0f, 0, "On-screen watch text render start y position");
	REGISTER_CVAR(watch_text_render_size, 1.75f, 0, "On-screen watch text render size");
	REGISTER_CVAR(watch_text_render_lineSpacing, 9.3f, 0, "On-screen watch text line spacing (to cram more text on screen without shrinking the font)");
	REGISTER_CVAR(watch_text_render_fxscale, 13.0f, 0, "Draw2d label to IFFont x scale value (for calcing sizes)." );

	REGISTER_CVAR(autotest_enabled, 0, 0, "enabled autotesting");
	autotest_state_setup = REGISTER_STRING("autotest_state_setup", "", 0, "setup string for autotesting");
	REGISTER_CVAR(autotest_quit_when_done, 0, 0, "quit the game when tests are done");

#if !defined(_RELEASE)
	net_onlyListGameServersContainingText = REGISTER_STRING("net_onlyListGameServersContainingText", "", 0, "Server list will only display host names containing this text");
	net_nat_type = REGISTER_STRING("net_nat_type", "Uninitialized", VF_READONLY, "The current NAT Type set.");
#endif

	REGISTER_CVAR(designer_warning_enabled, 1, 0, "designer warnings are enabled");
	REGISTER_CVAR(ai_DebugVisualScriptErrors, 0, VF_CHEAT|VF_DUMPTODISK,
		"Toggles the visual signal when something goes wrong in the AI scripts.\n"
		"Usage: ai_DebugVisualScriptErrors [0/1]\n"
		"Default is 0 (off). ai_DebugVisualScriptErrors changes the alien model to a dog model"
		"when an error occurs. When that happens there should be an error description in the log.\n");

	REGISTER_CVAR(ai_DebugPressureSystem, 0, VF_CHEAT|VF_DUMPTODISK, "Toggles AI's pressure system debug mode.\n");
	REGISTER_CVAR(ai_DebugAggressionSystem, 0, VF_CHEAT|VF_DUMPTODISK, "Toggles AI's aggression system debug mode.\n");
	REGISTER_CVAR(ai_DebugBattleFront, 0, VF_CHEAT|VF_DUMPTODISK, "Toggles AI's battlefront system debug mode.\n");
	REGISTER_CVAR(ai_DebugSearch, 0, VF_CHEAT|VF_DUMPTODISK, "Toggles AI's search module debug mode.\n");

	REGISTER_CVAR(g_actorViewDistRatio, 127, 0, "Sets the view dist ratio for actors.\n");
	REGISTER_CVAR(g_mpPlayerLodRatio, 80, VF_REQUIRE_LEVEL_RELOAD, "Sets the lod ratio for players in MP.\n");

	REGISTER_CVAR(g_hitDeathReactions_enable, 1, 0, "Enables/Disables Hit/Death reaction system");
	REGISTER_CVAR(g_hitDeathReactions_useLuaDefaultFunctions, 0, 0, "If enabled, it'll use the default lua methods inside HitDeathReactions script instead of the default c++ version");
	REGISTER_CVAR(g_hitDeathReactions_disable_ai, 1, 0, "If enabled, it'll not allow to execute any AI instruction during the hit/death reaction");
	REGISTER_CVAR(g_hitDeathReactions_debug, 0, 0, "Enables/Disables debug information for hit and death reactions system");
	REGISTER_CVAR(g_hitDeathReactions_disableRagdoll, 0, 0, "Disables switching to ragdoll at the end of animations");
	REGISTER_CVAR(g_hitDeathReactions_disableHitAnimatedCollisions, 0, 0, "Temporary CVar: Disables Hit Reactions animated collisions");

	REGISTER_CVAR(g_maximumDamage, -1.0f, 0, "Maximum health reduction allowed on actors. If negative it is ignored");

	REGISTER_CVAR(g_muzzleFlashCull, 1, VF_NULL, "Enable muzzle flash culling");
	REGISTER_CVAR(g_muzzleFlashCullDistance, 30000.0f, 0, "Culls distant muzzle flashes");
	REGISTER_CVAR(g_rejectEffectVisibilityCull, 1, VF_NULL, "Enable reject effect culling");
	REGISTER_CVAR(g_rejectEffectCullDistance, 25.f*25.f, 0, "Culls distant shell casing effects");

	REGISTER_CVAR(g_flyCamLoop, 0, 0, "Toggles whether the flycam should loop at the end of playback");
	
	REGISTER_CVAR(g_dummyPlayersFire, 0, 0, "Enable/Disable firing on the dummy players");
	REGISTER_CVAR(g_dummyPlayersMove, 0, 0, "Enable/Disable moving on the dummy players");

	REGISTER_CVAR(g_mpCullShootProbablyHits, 1, 0, "Culls ProbablyHits in Shoot function, to avoid line tests");

	REGISTER_CVAR(g_cloakRefractionScale, 1.0f, 0, "Sets cloak refraction scale");

	REGISTER_CVAR(g_telemetry_gameplay_enabled, 1, 0, "if telemetry is enabled are gameplay related telemetry stats being gathered");
	REGISTER_CVAR(g_telemetryEnabledSP, 0, 0, "Enables telemetry collection in singleplayer");
	REGISTER_CVAR(g_telemetrySampleRatePerformance, 1.0f, 0, "How often to gather performance telemetry statistics");
	REGISTER_CVAR(g_telemetrySampleRateBandwidth, 3.0f, 0, "How often to gather bandwidth telemetry statistics");
	REGISTER_CVAR(g_telemetrySampleRateMemory, 2.0f, 0, "How often to gather memory telemetry statistics");

	REGISTER_CVAR(g_deathFadeDelay, 3.0f, 0, "Delay before fade to black when player dies");
	REGISTER_CVAR(g_deathFadeTime, 1.0f, 0, "Fade out time when player dies");
	REGISTER_CVAR(g_deathReloadDelay, -1.0f, 0, "Time before reload after player dies and fades out (negative values disable reloading on death)");

	gEnv->pConsole->Register("g_telemetryConfig", &g_telemetryConfig, "none", 0, "Sets the telemetry config to use (see Libs/Telemetry/).", OnTelemetryConfigChanged);
	
#if ENABLE_MINDCONTROL
	CMindControlConsole::InitCVars(pConsole);
#endif
	NetInputChainInitCVars();

	CGodMode::GetInstance().RegisterConsoleVars(pConsole);

	CBodyDamageCVars::RegisterVariables();

	InitAIPerceptionCVars(pConsole);
}

//------------------------------------------------------------------------
void SCVars::ReleaseAIPerceptionCVars(IConsole* pConsole)
{
	pConsole->UnregisterVariable("ai_perception.movement_useSurfaceType");
	pConsole->UnregisterVariable("ai_perception.movement_movingSurfaceDefault");
	pConsole->UnregisterVariable("ai_perception.movement_standingRadiusDefault");
	pConsole->UnregisterVariable("ai_perception.movement_crouchRadiusDefault");
	pConsole->UnregisterVariable("ai_perception.movement_standingMovingMultiplier");
	pConsole->UnregisterVariable("ai_perception.movement_crouchMovingMultiplier");
}

//------------------------------------------------------------------------
void SCVars::ReleaseCVars()
{
	IConsole* pConsole = gEnv->pConsole;

	pConsole->UnregisterVariable("g_enableGameSwitchMenu", true);
	pConsole->UnregisterVariable("g_multiplayerDefault", true);
	pConsole->UnregisterVariable("g_EPD", true);

	pConsole->UnregisterVariable("cl_fov", true);
	pConsole->UnregisterVariable("cl_tpvDist", true);
	pConsole->UnregisterVariable("cl_tpvYaw", true);
	pConsole->UnregisterVariable("cl_nearPlane", true);
	pConsole->UnregisterVariable("cl_sprintShake", true);
	pConsole->UnregisterVariable("cl_sensitivity", true);
	pConsole->UnregisterVariable("cl_sensitivityController", true);
	pConsole->UnregisterVariable("cl_mp_sensitivityControllerDefault", true);
	pConsole->UnregisterVariable("cl_invertMouse", true);
	pConsole->UnregisterVariable("cl_invertController", true);
	pConsole->UnregisterVariable("cl_crouchToggle", true);
	pConsole->UnregisterVariable("cl_fpBody", true);
	pConsole->UnregisterVariable("cl_idleBreaksDelayTime", true);

	pConsole->UnregisterVariable("i_staticfiresounds", true);
	pConsole->UnregisterVariable("i_soundeffects", true);
	pConsole->UnregisterVariable("i_lighteffects", true);
	pConsole->UnregisterVariable("i_particleeffects", true);
	pConsole->UnregisterVariable("i_offset_front", true);
	pConsole->UnregisterVariable("i_offset_up", true);
	pConsole->UnregisterVariable("i_offset_right", true);
	pConsole->UnregisterVariable("i_maxheavymedweapons", true);
	
	pConsole->UnregisterVariable("cl_motionBlur", true);
	pConsole->UnregisterVariable("cl_sprintBlur", true);
	pConsole->UnregisterVariable("cl_hitShake", true);
	pConsole->UnregisterVariable("cl_hitBlur", true);

	pConsole->UnregisterVariable("cl_righthand", true);
	pConsole->UnregisterVariable("cl_screeneffects", true);

	pConsole->UnregisterVariable("cl_suitModeSpecificScreenFX", true);

	pConsole->UnregisterVariable("pl_inputAccel", true);
	pConsole->UnregisterVariable("pl_debug_suit", true);
	pConsole->UnregisterVariable("pl_debug_pickable_items", true);

	pConsole->UnregisterVariable("cl_actorsafemode", true);
	pConsole->UnregisterVariable("g_hunterIK", true);
	pConsole->UnregisterVariable("g_tentacle_joint_limit", true);
	pConsole->UnregisterVariable("g_enableSpeedLean", true);

	pConsole->UnregisterVariable("aa_maxDist", true);

	pConsole->UnregisterVariable("g_stanceTransitionSpeed", true);

	pConsole->UnregisterVariable("g_gameRules_startTimerLength");
	pConsole->UnregisterVariable("g_gameRules_nextLevelTimerLength");
	pConsole->UnregisterVariable("g_gameRules_showScoreboardTimerLength");
	pConsole->UnregisterVariable("g_gameRules_minPlayerWarningTimerLength");

	pConsole->UnregisterVariable("g_flashBangMinSpeedMultiplier", true);
	pConsole->UnregisterVariable("g_flashBangSpeedMultiplierFallOffEase", true);
	pConsole->UnregisterVariable("g_flashBangNotInFOVRadiusFraction", true);
	pConsole->UnregisterVariable("g_flashBangMinFOVMultiplier", true);
	pConsole->UnregisterVariable("g_flashBangFriends", true);
	pConsole->UnregisterVariable("g_flashBangSelf", true);
	
	pConsole->UnregisterVariable("g_STAPCameraAnimation", true);
	
	pConsole->UnregisterVariable("g_suitSpeedMult", true);
	pConsole->UnregisterVariable("g_suitSpeedMultMultiplayer", true);
	pConsole->UnregisterVariable("g_suitSpeedEnergyConsumption", true);
	pConsole->UnregisterVariable("g_suitSpeedEnergyConsumptionMultiplayer", true);

	pConsole->UnregisterVariable("g_pp_scale_income", true);
	pConsole->UnregisterVariable("g_pp_scale_price", true);

	pConsole->UnregisterVariable("g_mpPermanentSonarVision", true);
	pConsole->UnregisterVariable("g_mpPermanentSonarVision_alpha", true);
	pConsole->UnregisterVariable("g_mpPermanentSonarVision_outline_alpha", true);
	pConsole->UnregisterVariable("g_mpAllSeeingRadar", true);
	pConsole->UnregisterVariable("g_mpAllSeeingRadarSv", true);
	pConsole->UnregisterVariable("g_mpDisableRadar", true);
	pConsole->UnregisterVariable("g_mpNoEnemiesOnRadar", true);

	pConsole->UnregisterVariable("g_radialBlur", true);
	pConsole->UnregisterVariable("g_PlayerFallAndPlay", true);
	pConsole->UnregisterVariable("g_fallAndPlayThreshold", true);

	pConsole->UnregisterVariable("g_fireStabilizationThreshold", true);

	pConsole->UnregisterVariable("g_SurvivorOneVictoryConditions_watchLvl", true);
	pConsole->UnregisterVariable("g_SimpleEntityBasedObjective_watchLvl", true);
	pConsole->UnregisterVariable("g_CarryObjective_watchLvl", true);
	pConsole->UnregisterVariable("g_CombiCaptureObjective_watchLvl", true);
	pConsole->UnregisterVariable("g_CTFScoreElement_watchLvl", true);
	pConsole->UnregisterVariable("g_KingOfTheHillObjective_watchLvl", true);

	pConsole->UnregisterVariable("g_timelimit", true);
	pConsole->UnregisterVariable("g_timelimitextratime", true);
	pConsole->UnregisterVariable("g_teamlock", true);
	pConsole->UnregisterVariable("g_roundlimit", true);
	pConsole->UnregisterVariable("g_preroundtime", true);
	pConsole->UnregisterVariable("g_suddendeathtime", true);
	pConsole->UnregisterVariable("g_roundtime", true);
	pConsole->UnregisterVariable("g_fraglimit", true);
	pConsole->UnregisterVariable("g_fraglead", true);
	pConsole->UnregisterVariable("g_debugNetPlayerInput", true);
	pConsole->UnregisterVariable("g_debug_fscommand", true);
	pConsole->UnregisterVariable("g_debugDirectMPMenu", true);
	pConsole->UnregisterVariable("g_skipIntro", true);
	pConsole->UnregisterVariable("g_resetActionmapOnStart", true);
	pConsole->UnregisterVariable("g_useProfile", true);
	pConsole->UnregisterVariable("g_startFirstTime", true);
	pConsole->UnregisterVariable("g_killcamEnable", true);
	pConsole->UnregisterVariable("g_killcamDebug", true);
	pConsole->UnregisterVariable("g_killcamPlaybackSyncDelay", true);
	pConsole->UnregisterVariable("g_killcamProjectileDistance", true);
	pConsole->UnregisterVariable("g_killcamProjectileHeightOffset", true);
	pConsole->UnregisterVariable("g_killcamProjectileVictimHeightOffset", true);
	pConsole->UnregisterVariable("g_killcamProjectileMinimumVictimDist", true);
	pConsole->UnregisterVariable("g_killcamSmoothing", true);
	pConsole->UnregisterVariable("g_killcamGrenadeSmoothingDist", true);
	pConsole->UnregisterVariable("g_scoreLimit", true);
	pConsole->UnregisterVariable("g_wavesLimit", true);
	pConsole->UnregisterVariable("g_spawndeathdist", true);
	pConsole->UnregisterVariable("g_spawn_enemyDistLevelSizeDivider", true);
	pConsole->UnregisterVariable("g_objectivesTimeScale", true);
	pConsole->UnregisterVariable("g_hostMigrationConnectionTimeout", true);
	pConsole->UnregisterVariable("g_hostMigrationResumeTime", true);
	pConsole->UnregisterVariable("g_spawn_lastKillerDist", true);
	pConsole->UnregisterVariable("g_spawn_lastSpawnDist", true);
	pConsole->UnregisterVariable("g_spawn_explosiveSafeDist", true);
	pConsole->UnregisterVariable("g_spawn_ctfFlagAvoidDist", true);

	pConsole->UnregisterVariable("pl_sliding_control.min_speed_threshold", true);
	pConsole->UnregisterVariable("pl_sliding_control.min_speed", true);
	pConsole->UnregisterVariable("pl_sliding_control.deceleration_speed", true);
	pConsole->UnregisterVariable("pl_sliding_control.deceleration_speed_powerMode", true);
	pConsole->UnregisterVariable("pl_sliding_control.min_downhill_threshold", true);
	pConsole->UnregisterVariable("pl_sliding_control.max_downhill_threshold", true);
	pConsole->UnregisterVariable("pl_sliding_control.max_downhill_acceleration", true);

	pConsole->UnregisterVariable( "pl_airFrictionPerk_control.fall_activation_height", true);
	pConsole->UnregisterVariable( "pl_airFrictionPerk_control.fall_maxSeepd_height", true);
	pConsole->UnregisterVariable( "pl_airFrictionPerk_control.forward_speed", true);
	pConsole->UnregisterVariable( "pl_airFrictionPerk_control.min_falling_speed", true);
	pConsole->UnregisterVariable( "pl_airFrictionPerk_control.max_falling_speed", true);
	pConsole->UnregisterVariable( "pl_airFrictionPerk_control.view_pitch_shake", true);
	pConsole->UnregisterVariable( "pl_airFrictionPerk_control.view_recover_time", true);

	pConsole->UnregisterVariable("g_inventoryNoLimits", true);
	pConsole->UnregisterVariable("g_inventoryWeaponCapacity", true);
	pConsole->UnregisterVariable("g_inventoryExplosivesCapacity", true);
	pConsole->UnregisterVariable("g_inventoryGrenadesCapacity", true);

	pConsole->UnregisterVariable("g_loadoutServerControlled", true);

	pConsole->UnregisterVariable("g_tk_punish", true);
	pConsole->UnregisterVariable("g_tk_punish_limit", true);

	pConsole->UnregisterVariable("g_stapEnable", true);
	pConsole->UnregisterVariable("g_translationPinningEnable", true);

	pConsole->UnregisterVariable("g_MPLedgeGrabbing", true);
	pConsole->UnregisterVariable("g_MPCoverAndLean", true);

	pConsole->UnregisterVariable("g_dogtagsEnable", true);
	pConsole->UnregisterVariable("g_dogtagsHUDPopupEnable", true);
	pConsole->UnregisterVariable("g_dogtagsHUDPopupDuration", true);

	pConsole->UnregisterVariable("g_detachCamera", true);
	pConsole->UnregisterVariable("g_moveDetachedCamera", true);
	pConsole->UnregisterVariable("g_detachedCameraMoveSpeed", true);
	pConsole->UnregisterVariable("g_detachedCameraRotateSpeed", true);
	pConsole->UnregisterVariable("g_detachedCameraTurboBoost", true);
	pConsole->UnregisterVariable("g_detachedCameraDebug", true);
	
	pConsole->UnregisterVariable("g_debugCollisionDamage", true);
	pConsole->UnregisterVariable("g_debugHits", true);

	pConsole->UnregisterVariable("g_trooperTentacleAnimBlend", true);
	pConsole->UnregisterVariable("g_trooperBankingMultiplier", true);

	pConsole->UnregisterVariable("g_bulletPenetrationEnable", true);
	pConsole->UnregisterVariable("g_bulletPenetrationDebug", true);
	pConsole->UnregisterVariable("g_bulletPenetrationTimeout", true);

	pConsole->UnregisterVariable("g_hologram_maxBlockedTime", true);
	pConsole->UnregisterVariable("g_hologram_negligibleMovementSqrd", true);
	pConsole->UnregisterVariable("g_hologram_initialNoMovementCheckTime", true);
	pConsole->UnregisterVariable("g_hologram_spawnOffsetDist", true);

	pConsole->UnregisterVariable("g_explosion_materialFX_raycastLength", true);

	pConsole->UnregisterVariable("v_profileMovement", true);    
	pConsole->UnregisterVariable("v_pa_surface", true);
	pConsole->UnregisterVariable("v_wind_minspeed", true);
	pConsole->UnregisterVariable("v_draw_suspension", true);
	pConsole->UnregisterVariable("v_draw_slip", true);  
	pConsole->UnregisterVariable("v_invertPitchControl", true);
	pConsole->UnregisterVariable("v_sprintSpeed", true);
	pConsole->UnregisterVariable("v_rockBoats", true);  
	pConsole->UnregisterVariable("v_debugMountedWeapon", true);  
	pConsole->UnregisterVariable("v_dumpFriction", true);
	pConsole->UnregisterVariable("v_debugSounds", true);
	pConsole->UnregisterVariable("v_altitudeLimit", true);
	pConsole->UnregisterVariable("v_altitudeLimitLowerOffset", true);
	pConsole->UnregisterVariable("v_airControlSensivity", true);

	// animation triggered footsteps
	pConsole->UnregisterVariable("g_FootstepSoundsFollowEntity", true);
	pConsole->UnregisterVariable("g_FootstepSoundsDebug", true);
	pConsole->UnregisterVariable("g_footstepSoundMaxDistanceSq", true);

	// variables from CPlayer
	pConsole->UnregisterVariable("player_DrawIK", true);
	pConsole->UnregisterVariable("player_NoIK", true);
	pConsole->UnregisterVariable("g_enableIdleCheck", true);
	pConsole->UnregisterVariable("pl_debug_movement", true);
	pConsole->UnregisterVariable("pl_debug_filter", true);
	pConsole->UnregisterVariable("pl_debug_view", true);	
	pConsole->UnregisterVariable("pl_jump_quickPressThresh", true);
	pConsole->UnregisterVariable("pl_switchTPOnKill", true);

#if !defined(_RELEASE)
	pConsole->UnregisterVariable("pl_debug_watch_camera_mode", true);
	pConsole->UnregisterVariable("pl_debug_log_camera_mode_changes", true);
#endif

	// alien debugging
	pConsole->UnregisterVariable("aln_debug_movement", true);
	pConsole->UnregisterVariable("aln_debug_filter", true);

	// variables from CPlayerMovementController
	pConsole->UnregisterVariable("g_showIdleStats", true);
	pConsole->UnregisterVariable("g_debugaimlook", true);

	// variables from CHUD
	pConsole->UnregisterVariable("hud_onScreenNearDistance", true);
	pConsole->UnregisterVariable("hud_onScreenFarDistance", true);
	pConsole->UnregisterVariable("hud_onScreenNearSize", true);
	pConsole->UnregisterVariable("hud_onScreenFarSize", true);
	pConsole->UnregisterVariable("hud_colorLine", true);
	pConsole->UnregisterVariable("hud_colorOver", true);
	pConsole->UnregisterVariable("hud_colorText", true);
	pConsole->UnregisterVariable("hud_showAllObjectives", true);
	pConsole->UnregisterVariable("hud_centernames", true);
	pConsole->UnregisterVariable("hud_panoramic", true);
	pConsole->UnregisterVariable("hud_voicemode", true);
	pConsole->UnregisterVariable("hud_attachBoughEquipment", true);
	pConsole->UnregisterVariable("hud_subtitlesRenderMode", true);
	pConsole->UnregisterVariable("hud_subtitles", true);
	pConsole->UnregisterVariable("hud_subtitlesFontSize", true);
	pConsole->UnregisterVariable("hud_subtitlesHeight", true);
	pConsole->UnregisterVariable("hud_DebugFrontEnd", true); // FIXME : Define this here or in FrontEnd.cpp not both.
	pConsole->UnregisterVariable("hud_UseFlashFrontEndLoading", true);
	pConsole->UnregisterVariable("hud_startPaused", true);
	pConsole->UnregisterVariable("hud_autoTagTargettedActor", true);

	pConsole->UnregisterVariable("hud_mapEnable", true);
	pConsole->UnregisterVariable("hud_mapOverrideCenterX", true);
	pConsole->UnregisterVariable("hud_mapOverrideCenterY", true);

	pConsole->UnregisterVariable("hud_DeathRedScreen", true);
	pConsole->UnregisterVariable("hud_ContextualHealthIndicator", true);

	pConsole->UnregisterVariable("hud_aspectCorrection", true);
	pConsole->UnregisterVariable("controller_power_curve_z", true);
	pConsole->UnregisterVariable("controller_power_curve_z", true);
	pConsole->UnregisterVariable("controller_full_turn_multiplier_z", true);
	pConsole->UnregisterVariable("controller_full_turn_multiplier_x", true);
	pConsole->UnregisterVariable("hud_ctrlZoomMode", true);

	// weapon system
	pConsole->UnregisterVariable("i_debuggun_1", true);
	pConsole->UnregisterVariable("i_debuggun_2", true);

	pConsole->UnregisterVariable("tracer_min_distance", true);
	pConsole->UnregisterVariable("tracer_max_distance", true);
	pConsole->UnregisterVariable("tracer_min_scale", true);
	pConsole->UnregisterVariable("tracer_max_scale", true);
	pConsole->UnregisterVariable("tracer_max_count", true);
	pConsole->UnregisterVariable("tracer_player_radiusSqr", true);

	pConsole->UnregisterVariable("i_debug_projectiles", true);
	pConsole->UnregisterVariable("i_debug_weaponActions", true);
	pConsole->UnregisterVariable("i_debug_recoil", true);
	pConsole->UnregisterVariable("i_debug_spread", true);
	pConsole->UnregisterVariable("i_auto_turret_target", true);
	pConsole->UnregisterVariable("i_auto_turret_target_tacshells", true);

	pConsole->UnregisterVariable("i_debug_itemparams_memusage", true);
	pConsole->UnregisterVariable("i_debug_weaponparams_memusage", true);
	pConsole->UnregisterVariable("i_debug_zoom_mods", true);
	pConsole->UnregisterVariable("i_debug_mp_flowgraph", true);

	pConsole->UnregisterVariable("g_quickGame_map",true);
	pConsole->UnregisterVariable("g_quickGame_mode",true);
	pConsole->UnregisterVariable("g_quickGame_min_players",true);
	pConsole->UnregisterVariable("g_quickGame_prefer_lan",true);
	pConsole->UnregisterVariable("g_quickGame_prefer_favorites",true);
	pConsole->UnregisterVariable("g_quickGame_prefer_mycountry",true);
	pConsole->UnregisterVariable("g_quickGame_ping1_level",true);
	pConsole->UnregisterVariable("g_quickGame_ping2_level",true);
	pConsole->UnregisterVariable("g_quickGame_debug",true);
	pConsole->UnregisterVariable("g_skip_tutorial",true);

	pConsole->UnregisterVariable("g_displayIgnoreList",true);
	pConsole->UnregisterVariable("g_buddyMessagesIngame",true);

	pConsole->UnregisterVariable("g_battleDust_enable", true);
	pConsole->UnregisterVariable("g_battleDust_debug", true);
	pConsole->UnregisterVariable("g_battleDust_effect", true);

	pConsole->UnregisterVariable("g_PSTutorial_Enabled", true);

	pConsole->UnregisterVariable("sv_voting_timeout",true);
	pConsole->UnregisterVariable("sv_voting_cooldown",true);
	pConsole->UnregisterVariable("sv_voting_ratio",true);
	pConsole->UnregisterVariable("sv_voting_team_ratio",true);

	pConsole->UnregisterVariable("sv_input_timeout", true);

	pConsole->UnregisterVariable("sv_aiTeamName", true);
	pConsole->UnregisterVariable("performance_profile_logname", true);

	pConsole->UnregisterVariable("g_spectate_TeamOnly", true);
	pConsole->UnregisterVariable("g_spectate_cctv_RotScale", true);
	pConsole->UnregisterVariable("g_spectate_DisableManual", true);
	pConsole->UnregisterVariable("g_spectate_DisableSpawnOptOut", true);
	pConsole->UnregisterVariable("g_spectate_DisableDead", true);
	pConsole->UnregisterVariable("g_spectate_DisableFree", true);
	pConsole->UnregisterVariable("g_spectate_DisableFollow", true);
	pConsole->UnregisterVariable("g_spectate_DisableCCTV", true);

	pConsole->UnregisterVariable("g_claymore_limit", true);
	pConsole->UnregisterVariable("g_avmine_limit", true);
	pConsole->UnregisterVariable("g_debugMines", true);

	pConsole->UnregisterVariable("aim_assistCrosshairSize", true);
	pConsole->UnregisterVariable("aim_assistCrosshairDebug", true);

	// Alternative player input normalization code
	pConsole->UnregisterVariable("aim_altNormalization.enable");
	pConsole->UnregisterVariable("aim_altNormalization.hud_ctrl_Curve_Unified");
	pConsole->UnregisterVariable("aim_altNormalization.hud_ctrl_Coeff_Unified"); 
	// Alternative player input normalization code

	pConsole->UnregisterVariable("pl_debug_aiming_input");

	pConsole->UnregisterVariable("g_nanoSuitEnableSuitShapeDeformation", true);
	pConsole->UnregisterVariable("g_teamSilhouette.enableUpdate",true);
	pConsole->UnregisterVariable("g_teamSilhouette.r", true);
	pConsole->UnregisterVariable("g_teamSilhouette.g", true);
	pConsole->UnregisterVariable("g_teamSilhouette.b", true);
	pConsole->UnregisterVariable("g_teamSilhouette.a", true);
	pConsole->UnregisterVariable("g_teamSilhouette.enabled", true);

	pConsole->UnregisterVariable("g_enemySilhouette.enableUpdate",true);
	pConsole->UnregisterVariable("g_enemySilhouette.r", true);
	pConsole->UnregisterVariable("g_enemySilhouette.g", true);
	pConsole->UnregisterVariable("g_enemySilhouette.b", true);
	pConsole->UnregisterVariable("g_enemySilhouette.a", true);
	pConsole->UnregisterVariable("g_enemySilhouette.enabled", true);

	pConsole->UnregisterVariable("g_volatileSpike.decalLife",true);
	pConsole->UnregisterVariable("g_volatileSpike.decalMinSize",true);
	pConsole->UnregisterVariable("g_volatileSpike.decalMaxSize",true);
	pConsole->UnregisterVariable("g_volatileSpike.decalEmitterLife",true);
	pConsole->UnregisterVariable("g_volatileSpike.decalGrowTime",true);
	pConsole->UnregisterVariable("g_volatileSpike.maxRandomSteamEmitters",true);
	pConsole->UnregisterVariable("g_volatileSpike.maxDecals",true);

	pConsole->UnregisterVariable("g_postEffect.FilterGrain_Amount",true);
	pConsole->UnregisterVariable("g_postEffect.FilterRadialBlurring_Amount",true);
	pConsole->UnregisterVariable("g_postEffect.FilterRadialBlurring_ScreenPosX",true);
	pConsole->UnregisterVariable("g_postEffect.FilterRadialBlurring_ScreenPosY",true);
	pConsole->UnregisterVariable("g_postEffect.FilterRadialBlurring_Radius",true);
	pConsole->UnregisterVariable("g_postEffect.Global_User_ColorC",true);
	pConsole->UnregisterVariable("g_postEffect.Global_User_ColorM",true);
	pConsole->UnregisterVariable("g_postEffect.Global_User_ColorY",true);
	pConsole->UnregisterVariable("g_postEffect.Global_User_ColorK",true);
	pConsole->UnregisterVariable("g_postEffect.Global_User_Brightness",true);
	pConsole->UnregisterVariable("g_postEffect.Global_User_Contrast",true);
	pConsole->UnregisterVariable("g_postEffect.Global_User_Saturation",true);
	pConsole->UnregisterVariable("g_postEffect.Global_User_ColorHue",true);

	pConsole->UnregisterVariable("g_gameFXSystemDebug",true);

#if ENABLE_MINDCONTROL
	CMindControlConsole::ReleaseCVars(pConsole);
#endif

#if DEBUG_NEW_SPAWNING
	pConsole->UnregisterVariable("g_debug_spawn_mode", true);
	pConsole->UnregisterVariable("g_debug_spawning_visually", true);
	pConsole->UnregisterVariable("visual_spawn_team_A", true);
	pConsole->UnregisterVariable("visual_spawn_team_B", true);
	pConsole->UnregisterVariable("g_debug_entity_clustering", true);
	pConsole->UnregisterVariable("g_debug_entity_clustering_radius", true);
#endif // DEBUG_NEW_SPAWNING
	pConsole->UnregisterVariable("g_spawn_conflict_clustering_radius", true);
	pConsole->UnregisterVariable("g_spawn_vistable_show", true);
	pConsole->UnregisterVariable("g_spawn_vistable_numLineTestsPerFrame", true);
	pConsole->UnregisterVariable("g_spawn_vistable_maxDistToAltSpawnEntity", true);

	pConsole->UnregisterVariable("g_showShadowChar");
	pConsole->UnregisterVariable("g_animateCameraOrientation");

	pConsole->UnregisterVariable("g_infiniteAmmo");

	pConsole->UnregisterVariable("g_animatorDebug");
	pConsole->UnregisterVariable("g_hideArms");
	pConsole->UnregisterVariable("g_useHitReactController");
	pConsole->UnregisterVariable("g_debugHitReacts");

#if !defined(_RELEASE)
	pConsole->UnregisterVariable("g_EAWhiteBuild");
	pConsole->UnregisterVariable("g_playerUsesDedicatedInput");
	pConsole->UnregisterVariable("net_onlyListGameServersContainingText");
	pConsole->UnregisterVariable("net_nat_type");
#endif

	pConsole->UnregisterVariable("autotest_enabled");
	pConsole->UnregisterVariable("autotest_state_setup");
	pConsole->UnregisterVariable("autotest_quit_when_done");
	pConsole->UnregisterVariable("designer_warning_enabled");


	pConsole->UnregisterVariable("ai_DebugPressureSystem");
	pConsole->UnregisterVariable("ai_DebugAggressionSystem");
	pConsole->UnregisterVariable("ai_DebugBattleFront");

	pConsole->UnregisterVariable("g_actorViewDistRatio");
	pConsole->UnregisterVariable("g_mpPlayerLodRatio");
	CGodMode::GetInstance().UnregisterConsoleVars(pConsole);

	pConsole->UnregisterVariable("g_hitDeathReactions_enable", true);
	pConsole->UnregisterVariable("g_hitDeathReactions_useLuaDefaultFunctions", true);
	pConsole->UnregisterVariable("g_hitDeathReactions_disable_ai", true);
	pConsole->UnregisterVariable("g_hitDeathReactions_debug", true);
	pConsole->UnregisterVariable("g_hitDeathReactions_disableRagdoll", true);
	pConsole->UnregisterVariable("g_hitDeathReactions_disableHitAnimatedCollisions", true);

	pConsole->UnregisterVariable("g_maximumDamage", true);

	pConsole->UnregisterVariable("g_muzzleFlashCull", true);
	pConsole->UnregisterVariable("g_muzzleFlashCullDistance", true);
	pConsole->UnregisterVariable("g_rejectEffectVisibilityCull", true);
	pConsole->UnregisterVariable("g_rejectEffectCullDistance", true);
	
	pConsole->UnregisterVariable("g_flyCamLoop", true);
	pConsole->UnregisterVariable("g_mpCullShootProbablyHits", true);

	pConsole->UnregisterVariable("g_cloakRefractionScale", true);

	pConsole->UnregisterVariable("g_dummyPlayersFire", true);
	pConsole->UnregisterVariable("g_dummyPlayersMove", true);	
	CBodyDamageCVars::UnregisterVariables(pConsole);

	pConsole->UnregisterVariable("g_telemetry_gameplay_enabled", true);
	ReleaseAIPerceptionCVars(pConsole);
}

//------------------------------------------------------------------------
void CGame::CmdDumpSS(IConsoleCmdArgs *pArgs)
{
	g_pGame->GetSynchedStorage()->Dump();
}

//------------------------------------------------------------------------
void CGame::RegisterConsoleVars()
{
	assert(m_pConsole);

	if (m_pCVars)
	{
		m_pCVars->InitCVars(m_pConsole);    
	}
}

//------------------------------------------------------------------------
void CmdDumpItemNameTable(IConsoleCmdArgs *pArgs)
{
	SharedString::CSharedString::DumpNameTable();
}


void CmdPlayerGoto(IConsoleCmdArgs *pArgs)
{
	int iArgCount = pArgs->GetArgCount();
	if (iArgCount == 4 || iArgCount == 7)
	{
		CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());

		if (pPlayer)
		{
			IEntity * pEntity = pPlayer->GetEntity();

			Vec3 pos(ZERO);
			Ang3 rot(ZERO);

			CryLogAlways("CmdPlayerGoto() %s '%s' (%s, health=%d/%d%s) BEFORE pos=<%.2f %.2f %.2f> dir=<%.2f %.2f %.2f>", pEntity->GetClass()->GetName(), pEntity->GetName(),
				pPlayer->IsDead() ? "dead" : "alive", pPlayer->GetHealth(), pPlayer->GetMaxHealth(),
				pPlayer->GetActorStats()->isRagDoll ? ", RAGDOLL" : "", pEntity->GetWorldPos().x, pEntity->GetWorldPos().y, pEntity->GetWorldPos().z, pEntity->GetForwardDir().x, pEntity->GetForwardDir().y, pEntity->GetForwardDir().z);

			pos.x = (float)atof(pArgs->GetArg(1));
			pos.y = (float)atof(pArgs->GetArg(2));
			pos.z = (float)atof(pArgs->GetArg(3));

			CryLogAlways("CmdPlayerGoto() %s '%s' pos=%f, %f, %f", pEntity->GetClass()->GetName(), pEntity->GetName(), pos.x, pos.y, pos.z);
			if (iArgCount == 7)
			{
				rot.x = (float)atof(pArgs->GetArg(4));
				rot.y = (float)atof(pArgs->GetArg(5));
				rot.z = (float)atof(pArgs->GetArg(6));
				CryLogAlways("CmdPlayerGoto() %s '%s' rot=%f, %f, %f", pEntity->GetClass()->GetName(), pEntity->GetName(), rot.x, rot.y, rot.z);
			}
			
			Quat q;
			q.SetRotationXYZ(DEG2RAD(rot));
	
			pEntity->SetPos(pos);
			pEntity->SetRotation(q);
			pPlayer->SetViewRotation(q);

			CryLogAlways("CmdPlayerGoto() %s '%s' (%s, health=%d/%d%s) AFTER pos=<%.2f %.2f %.2f> dir=<%.2f %.2f %.2f>", pEntity->GetClass()->GetName(), pEntity->GetName(),
				pPlayer->IsDead() ? "dead" : "alive", pPlayer->GetHealth(), pPlayer->GetMaxHealth(),
				pPlayer->GetActorStats()->isRagDoll ? ", RAGDOLL" : "", pEntity->GetWorldPos().x, pEntity->GetWorldPos().y, pEntity->GetWorldPos().z, pEntity->GetForwardDir().x, pEntity->GetForwardDir().y, pEntity->GetForwardDir().z);
		}
		else
		{
			GameWarning("Something called the '%s' command when there's no local player", pArgs->GetArg(0));
		}
	}
	else
	{
		GameWarning("Something called the '%s' command with the wrong number of arguments (given %d but expected either 4 or 7)", pArgs->GetArg(0), iArgCount);
	}
}

//------------------------------------------------------------------------
void CmdGoto(IConsoleCmdArgs *pArgs)
{
	// feature is mostly useful for QA purposes, the editor has a similar feature, here we can call the editor command as well

	// todo:
	// * move to CryAction
	// * if in editor and game is not active it should move editor camera
	// * third person game should work by using player position
	// * level name could be part of the string

	const CCamera &rCam = gEnv->pRenderer->GetCamera();
	Matrix33 m = Matrix33(rCam.GetMatrix());

	int iArgCount = pArgs->GetArgCount();

	Ang3 aAngDeg = RAD2DEG(Ang3::GetAnglesXYZ(m));		// in degrees
	Vec3 vPos = rCam.GetPosition();

	if(iArgCount==1)
	{
		gEnv->pLog->Log("$5GOTO %d %d %d %d %d %d",(int)vPos.x,(int)vPos.y,(int)vPos.z, (int)aAngDeg.x,(int)aAngDeg.y,(int)aAngDeg.z);
		return;
	}

	// complicated but maybe the best Entity we can move to the given spot
	IGame *pGame = gEnv->pGame;																								if(!pGame)return;
	IGameFramework *pGameFramework=pGame->GetIGameFramework();								if(!pGameFramework)return;
	IViewSystem *pViewSystem=pGameFramework->GetIViewSystem();								if(!pViewSystem)return;
	IView *pView=pViewSystem->GetActiveView();																if(!pView)return;
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(pView->GetLinkedId());	if(!pEntity)return;

	if((iArgCount==4 || iArgCount==7)
		&& sscanf(pArgs->GetArg(1),"%f",&vPos.x)==1
		&& sscanf(pArgs->GetArg(2),"%f",&vPos.y)==1
		&& sscanf(pArgs->GetArg(3),"%f",&vPos.z)==1)
	{
		Matrix34 tm = pEntity->GetWorldTM();

		tm.SetTranslation(vPos);

		if(iArgCount==7
			&& sscanf(pArgs->GetArg(4),"%f",&aAngDeg.x)==1
			&& sscanf(pArgs->GetArg(5),"%f",&aAngDeg.y)==1
			&& sscanf(pArgs->GetArg(6),"%f",&aAngDeg.z)==1)
		{
			tm.SetRotation33( Matrix33::CreateRotationXYZ(DEG2RAD(aAngDeg)) );
		}

		// if there is an editor
		char str[256];
		sprintf(str,"ED_GOTO %.f %.f %.f %.f %.f %.f",vPos.x,vPos.y,vPos.z,aAngDeg.x,aAngDeg.y,aAngDeg.z);
		gEnv->pConsole->ExecuteString(str,true);

		pEntity->SetWorldTM(tm);
		return;
	}

	gEnv->pLog->LogError("GOTO: Invalid arguments");
}


//------------------------------------------------------------------------
void CGame::RegisterConsoleCommands()
{
	assert(m_pConsole);

	REGISTER_COMMAND("quit", "System.Quit()", VF_RESTRICTEDMODE, "Quits the game");

	REGISTER_COMMAND("playerGoto", CmdPlayerGoto, VF_CHEAT, 
		"Get or set the current position and orientation for the player - unlike goto it actually sets the player rotation correctly\n"
		"Usage: goto\n"
		"Usage: goto x y z\n"
		"Usage: goto x y z wx wy wz\n");
	REGISTER_COMMAND("goto", CmdGoto, VF_CHEAT, 
		"Get or set the current position and orientation for the player\n"
		"Usage: goto\n"
		"Usage: goto x y z\n"
		"Usage: goto x y z wx wy wz\n");
	REGISTER_COMMAND("gotoe", "local e=System.GetEntityByName(%1); if (e) then g_localActor:SetWorldPos(e:GetWorldPos()); end", VF_CHEAT, "Set the position of a entity with an given name");
	REGISTER_COMMAND("freeze", "g_gameRules:SetFrozenAmount(g_localActor,1)", 0, "Freezes player");

	REGISTER_COMMAND("loadactionmap", CmdLoadActionmap, 0, "Loads a key configuration file");
	REGISTER_COMMAND("restartgame", CmdRestartGame, 0, "Restarts Crysis completely.");
	REGISTER_COMMAND("i_dump_ammo_pool_stats", CmdDumpAmmoPoolStats, 0, "Dumps statistics related to the weapon ammo pool.");

	REGISTER_COMMAND("lastinv", CmdLastInv, 0, "Selects last inventory item used.");
	REGISTER_COMMAND("name", CmdName, VF_RESTRICTEDMODE, "Sets player name.");
	REGISTER_COMMAND("team", CmdTeam, VF_RESTRICTEDMODE, "Sets player team.");
	REGISTER_COMMAND("loadLastSave", CmdLoadLastSave, 0, "Loads the last savegame if available.");
	REGISTER_COMMAND("spectator", CmdSpectator, 0, "Sets the player as a spectator.");
	REGISTER_COMMAND("join_game", CmdJoinGame, VF_RESTRICTEDMODE, "Enter the current ongoing game.");
	REGISTER_COMMAND("kill", CmdKill, VF_RESTRICTEDMODE, "Kills the player.");
	REGISTER_COMMAND("revive", CmdRevive, VF_RESTRICTEDMODE, "Revives the player.");
	REGISTER_COMMAND("v_kill", CmdVehicleKill, VF_CHEAT, "Kills the players vehicle.");
	REGISTER_COMMAND("sv_restart", CmdRestart, 0, "Restarts the round.");
	REGISTER_COMMAND("sv_say", CmdSay, 0, "Broadcasts a message to all clients.");

#if !defined(_RELEASE)
	REGISTER_COMMAND("sv_sendConsoleCommand", CmdSendConsoleCommand, 0, "Makes all other connected machines execute a console command.");
	REGISTER_COMMAND("net_setOnlineMode", CmdNetSetOnlineMode, 0, "Sets the net mode where available. Options are online or lan.");
#endif

	REGISTER_COMMAND("echo", CmdEcho, 0, "Echo's the text back to the console and log files.");
	REGISTER_COMMAND("i_reload", CmdReloadItems, 0, "Reloads item scripts.");

	REGISTER_COMMAND("dumpss", CmdDumpSS, 0, "test synched storage.");
	REGISTER_COMMAND("dumpnt", CmdDumpItemNameTable, 0, "Dump ItemString table.");

	REGISTER_COMMAND("g_reloadGameRules", CmdReloadGameRules, 0, "Reload GameRules script");
	REGISTER_COMMAND("g_quickGame", CmdQuickGame, 0, "Quick connect to good server.");
	REGISTER_COMMAND("g_quickGameStop", CmdQuickGameStop, 0, "Cancel quick game search.");

	REGISTER_COMMAND("g_hitDeathReactions_reload", CmdReloadHitDeathReactions, VF_CHEAT, "Reloads hitDeathReactions for the specified actor, or for everyone if not specified");

	REGISTER_COMMAND("g_nextlevel", CmdNextLevel,0,"Switch to next level in rotation or restart current one.");
	REGISTER_COMMAND("vote", CmdVote, VF_RESTRICTEDMODE, "Vote on current topic.");
	REGISTER_COMMAND("startKickVoting",CmdStartKickVoting, VF_RESTRICTEDMODE, "Initiate voting.");
	REGISTER_COMMAND("startNextMapVoting",CmdStartNextMapVoting, VF_RESTRICTEDMODE, "Initiate voting.");

	REGISTER_COMMAND("g_battleDust_reload", CmdBattleDustReload, 0, "Reload the battle dust parameters xml");
	REGISTER_COMMAND("login",CmdLogin,0,"Log in as to CryNetwork using nickname and password as arguments");
	REGISTER_COMMAND("login_profile",CmdLoginProfile,0,"Log in as to CryNetwork using email, profile and password as arguments");
	REGISTER_COMMAND("connect_crynet",CmdCryNetConnect,0,"Connect to online game server");
	REGISTER_COMMAND("preloadforstats","PreloadForStats()",VF_CHEAT,"Preload multiplayer assets for memory statistics.");

	REGISTER_COMMAND("DumpLoadingMessages", CmdListAllRandomLoadingMessages, 0, "List all messages which could appear during loading");

	REGISTER_COMMAND("FreeCamEnable", CmdFreeCamEnable, VF_CHEAT, "Enable the freecam");
	REGISTER_COMMAND("FreeCamDisable", CmdFreeCamDisable, VF_CHEAT, "Disable the freecam");
	REGISTER_COMMAND("FreeCamLockCamera", CmdFreeCamLockCamera, VF_CHEAT, "Stay in freecam but lock the camera, allowing player controls to resume");
	REGISTER_COMMAND("FreeCamUnlockCamera", CmdFreeCamUnlockCamera, VF_CHEAT, "Stay in freecam unlock the camera, stopping player controls");
	REGISTER_COMMAND("FlyCamSetPoint", CmdFlyCamSetPoint, VF_CHEAT, "Sets a fly cam point");
	REGISTER_COMMAND("FlyCamPlay", CmdFlyCamPlay, VF_CHEAT, "Plays the flycam path");

	REGISTER_COMMAND("IgnoreAllAsserts", CmdIgnoreAllAsserts, VF_CHEAT, "Ignore all asserts");

	REGISTER_COMMAND("pl_reload", CmdReloadPlayer, VF_CHEAT, "Reload player's data including nanosuit.");
	REGISTER_COMMAND("pl_health", CmdSetPlayerHealth, VF_CHEAT, "Sets a player's health.");

	REGISTER_COMMAND("spawnActorAtPos", CmdSpawnActorAtPos, VF_CHEAT, "(<actorName>, <x>,<y>,<z>) Spawn an actor of class <actorName>, at position (<x>,<y>,<z>).");
	REGISTER_COMMAND("spawnDummyPlayers", CmdSpawnDummyPlayer, VF_CHEAT, "Spawn some dummy players for profiling purposes.");
	REGISTER_COMMAND("removeDummyPlayers", CmdRemoveDummyPlayers, VF_CHEAT, "Remove all dummy players.");
	REGISTER_COMMAND("setDummyPlayerState", CmdSetDummyPlayerState, VF_CHEAT, "(<entityName>, <state(s)>) Set the state of a dummy player.");
#ifndef DISABLE_OM
	//////////////////////////////////////////////////////////////////////////	
	REGISTER_COMMAND("OM_Connect", CmdOMConnect,0,"Connects to Organic Motion server.");
	REGISTER_COMMAND("OM_Disconnect", CmdOMDisconnect,0,"Disconnects from Organic Motion server.");
	memset(&m_OrganicMotionClient,sizeof(m_OrganicMotionClient),0);
	//////////////////////////////////////////////////////////////////////////
#endif

	CBodyDamageCVars::RegisterCommands();
}

//------------------------------------------------------------------------
void CGame::UnregisterConsoleCommands()
{
	assert(m_pConsole);

	m_pConsole->RemoveCommand("quit");
	m_pConsole->RemoveCommand("playerGoto");
	m_pConsole->RemoveCommand("goto");
	m_pConsole->RemoveCommand("gotoe");
	m_pConsole->RemoveCommand("freeze");

	m_pConsole->RemoveCommand("loadactionmap");
	m_pConsole->RemoveCommand("restartgame");
	m_pConsole->RemoveCommand("i_dump_ammo_pool_stats");

	m_pConsole->RemoveCommand("lastinv");
	m_pConsole->RemoveCommand("name");
	m_pConsole->RemoveCommand("team");
	m_pConsole->RemoveCommand("loadLastSave");
	m_pConsole->RemoveCommand("spectator");
	m_pConsole->RemoveCommand("join_game");
	m_pConsole->RemoveCommand("kill");
	m_pConsole->RemoveCommand("revive");
	m_pConsole->RemoveCommand("v_kill");
	m_pConsole->RemoveCommand("sv_restart");
	m_pConsole->RemoveCommand("sv_say");
	m_pConsole->RemoveCommand("echo");
	m_pConsole->RemoveCommand("i_reload");

	m_pConsole->RemoveCommand("dumpss");
	m_pConsole->RemoveCommand("dumpnt");

	m_pConsole->RemoveCommand("g_reloadGameRules");
	m_pConsole->RemoveCommand("g_quickGame");
	m_pConsole->RemoveCommand("g_quickGameStop");

	m_pConsole->RemoveCommand("g_reloadHitDeathReactions");

	m_pConsole->RemoveCommand("g_nextlevel");
	m_pConsole->RemoveCommand("vote");
	m_pConsole->RemoveCommand("startKickVoting");
	m_pConsole->RemoveCommand("startNextMapVoting");

	m_pConsole->RemoveCommand("g_battleDust_reload");
	m_pConsole->RemoveCommand("login");
	m_pConsole->RemoveCommand("login_profile");
	m_pConsole->RemoveCommand("connect_crynet");
	m_pConsole->RemoveCommand("preloadforstats");

	m_pConsole->RemoveCommand("DumpLoadingMessages");

	m_pConsole->RemoveCommand("FreeCamEnable");
	m_pConsole->RemoveCommand("FreeCamDisable");
	m_pConsole->RemoveCommand("FreeCamLockCamera");
	m_pConsole->RemoveCommand("FreeCamUnlockCamera");
	m_pConsole->RemoveCommand("FlyCamSetPoint");

	m_pConsole->RemoveCommand("IgnoreAllAsserts");

	m_pConsole->RemoveCommand("pl_reload");
	m_pConsole->RemoveCommand("pl_health");
	m_pConsole->RemoveCommand("spawnActorAtPos");
	m_pConsole->RemoveCommand("spawnDummyPlayers");
	m_pConsole->RemoveCommand("removeDummyPlayers");
	m_pConsole->RemoveCommand("setDummyPlayerState");

	m_pConsole->RemoveCommand("bulletTimeMode");
	m_pConsole->RemoveCommand("GOCMode");

#ifndef DISABLE_OM
	m_pConsole->RemoveCommand("OM_Connect");
	m_pConsole->RemoveCommand("OM_Disconnect");
#endif

	// variables from CHUDCommon
	m_pConsole->RemoveCommand("ShowGODMode");
	m_pConsole->RemoveCommand("reloadUI");

	CBodyDamageCVars::UnregisterCommands(m_pConsole);
}

void CGame::CmdListAllRandomLoadingMessages(IConsoleCmdArgs *pArgs)
{
	CLoadingMessageProviderListNode::ListAll();
}

//------------------------------------------------------------------------
static bool s_freeCamActive=false;

void CGame::CmdFreeCamEnable(IConsoleCmdArgs *pArgs)
{
	if (s_freeCamActive)
	{
		CryLogAlways("free cam already enabled");
	}
	else
	{
		CryLogAlways("Enabling free cam");

		s_freeCamActive=true;

		g_pGame->GetHUD()->ActivateState("no_hud");
		g_pGameCVars->g_detachCamera = 1;
		g_pGameCVars->g_moveDetachedCamera = 1;
		g_pGameCVars->watch_enabled = 0;

		// TODO cache existing enabled status?
		IActionMapManager* pActionMapMan = g_pGame->GetIGameFramework()->GetIActionMapManager();
		pActionMapMan->EnableActionMap("crysis2_common", false);
		pActionMapMan->EnableActionMap("player", false);
		pActionMapMan->EnableActionMap("flycam", true);

		IActionMap *flyCamActionMap = pActionMapMan->GetActionMap("flycam");
		IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
		assert(flyCamActionMap);
		assert(pClientActor);
		if (flyCamActionMap && pClientActor)
		{
			flyCamActionMap->SetActionListener(pClientActor->GetEntityId());
		}

		CPlayer *pClientPlayer=static_cast<CPlayer *>(pClientActor);
		assert(pClientPlayer);
		if (pClientPlayer && !pClientPlayer->IsThirdPerson())
		{
			pClientPlayer->ToggleThirdPerson();
		}

		/*
		if (g_pGameActions->FilterNoMove() )
		{
		g_pGameActions->FilterNoMove()->Enable(true);
		}
		if (g_pGameActions->FilterNoMouse())
		{
		g_pGameActions->FilterNoMouse()->Enable(true);
		}
		*/
	}
}
	
void CGame::CmdFreeCamDisable(IConsoleCmdArgs *pArgs)
{
	if (s_freeCamActive)
	{
		CryLogAlways("Disabling free cam");
		s_freeCamActive=false;

		g_pGame->GetHUD()->ActivateDefaultState();
		g_pGameCVars->g_detachCamera = 0;
		g_pGameCVars->g_moveDetachedCamera = 0;
		g_pGameCVars->watch_enabled = 1;

		IActionMapManager* pActionMapMan = g_pGame->GetIGameFramework()->GetIActionMapManager();
		pActionMapMan->EnableActionMap("crysis2_common", true);
		pActionMapMan->EnableActionMap("player", true);
		pActionMapMan->EnableActionMap("flycam", false);

		/*
		if (g_pGameActions->FilterNoMove() )
		{
		g_pGameActions->FilterNoMove()->Enable(false);
		}
		if (g_pGameActions->FilterNoMouse())
		{
		g_pGameActions->FilterNoMouse()->Enable(false);
		}
		*/

		IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();

		CPlayer *pClientPlayer=static_cast<CPlayer *>(pClientActor);
		assert(pClientPlayer);
		if (pClientPlayer && pClientPlayer->IsThirdPerson())
		{
			pClientPlayer->ToggleThirdPerson();
		}
	}
	else
	{
		CryLogAlways("free cam is not enabled");
	}
}

void CGame::CmdFreeCamLockCamera(IConsoleCmdArgs *pArgs)
{
	if (s_freeCamActive)
	{
		CryLogAlways("free cam - locking camera, unlocking player");

		IActionMapManager* pActionMapMan = g_pGame->GetIGameFramework()->GetIActionMapManager();
		pActionMapMan->EnableActionMap("crysis2_common", true);
		pActionMapMan->EnableActionMap("player", true);
		pActionMapMan->EnableActionMap("flycam", false);
	}
	else
	{
		CryLogAlways("free cam is not enabled");
	}
}

void CGame::CmdFreeCamUnlockCamera(IConsoleCmdArgs *pArgs)
{
	if (s_freeCamActive)
	{
		CryLogAlways("free cam - unlocking camera, locking player");

		IActionMapManager* pActionMapMan = g_pGame->GetIGameFramework()->GetIActionMapManager();
		pActionMapMan->EnableActionMap("crysis2_common", false);
		pActionMapMan->EnableActionMap("player", false);
		pActionMapMan->EnableActionMap("flycam", true);
	}
	else
	{
		CryLogAlways("free cam is not enabled");
	}
}

void CGame::CmdFlyCamSetPoint(IConsoleCmdArgs *pArgs)
{
	CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(pPlayer)
	{
		CPlayerInput* pPlayerInput = (CPlayerInput*)pPlayer->GetPlayerInput();
		if (pPlayerInput)
		{
			if (pArgs->GetArgCount() > 3)
			{
				Vec3 position(ZERO), lookAt(0, 1, 0);
				position.x = (float)atof(pArgs->GetArg(1));
				position.y = (float)atof(pArgs->GetArg(2));
				position.z = (float)atof(pArgs->GetArg(3));
				if (pArgs->GetArgCount() > 6)
				{
					lookAt.x = (float)atof(pArgs->GetArg(4));
					lookAt.y = (float)atof(pArgs->GetArg(5));
					lookAt.z = (float)atof(pArgs->GetArg(6));
				}
				pPlayerInput->AddFlyCamPoint(position, lookAt);
			}
			else
			{
				pPlayerInput->AddFlyCamPoint();
			}
		}
	}	
}

void CGame::CmdFlyCamPlay(IConsoleCmdArgs *pArgs)
{
	CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(pPlayer)
	{
		CPlayerInput* pPlayerInput = (CPlayerInput*)pPlayer->GetPlayerInput();
		if (pPlayerInput)
		{
			pPlayerInput->FlyCamPlay();
		}
	}
}

void CGame::CmdIgnoreAllAsserts(IConsoleCmdArgs *pArgs)
{
	gEnv->bIgnoreAllAsserts=true;	
	gEnv->bTesting=true;
}

//------------------------------------------------------------------------
void CGame::CmdLastInv(IConsoleCmdArgs *pArgs)
{
	if (!gEnv->bClient)
		return;

	if (CActor *pClientActor=static_cast<CActor *>(g_pGame->GetIGameFramework()->GetClientActor()))
		pClientActor->SelectLastItem(true);
}

//------------------------------------------------------------------------
void CGame::CmdName(IConsoleCmdArgs *pArgs)
{
	if (!gEnv->bClient)
		return;

	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
		pGameRules->RenamePlayer(pGameRules->GetActorByEntityId(pClientActor->GetEntityId()), pArgs->GetArg(1));
}

//------------------------------------------------------------------------
void CGame::CmdTeam(IConsoleCmdArgs *pArgs)
{
	if (!gEnv->bClient)
		return;

	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
		pGameRules->ChangeTeam(pGameRules->GetActorByEntityId(pClientActor->GetEntityId()), pArgs->GetArg(1));
}

//------------------------------------------------------------------------
void CGame::CmdLoadLastSave(IConsoleCmdArgs *pArgs)
{
	if (!gEnv->bClient || gEnv->bMultiplayer)
		return;

	bool bLoadSave = true;
	if (gEnv->bEditor)
	{
		ICVar* pAllowSaveLoadInEditor = gEnv->pConsole->GetCVar("g_allowSaveLoadInEditor");
		if (pAllowSaveLoadInEditor)
		{
			bLoadSave = (pAllowSaveLoadInEditor->GetIVal() != 0);
		}
		else
		{
			bLoadSave = false;
		}
	}

	if (bLoadSave)
	{
		const string& file = g_pGame->GetLastSaveGame();

		if(file.length())
		{
			if(!g_pGame->GetIGameFramework()->LoadGame(file.c_str(), true))
				g_pGame->GetIGameFramework()->LoadGame(file.c_str(), false);
		}
	}
	else
	{
		CGodMode::GetInstance().RespawnPlayerIfDead();
	}
}

//------------------------------------------------------------------------
void CGame::CmdSpectator(IConsoleCmdArgs *pArgs)
{
	if (g_pGame->GetCVars()->g_spectate_DisableManual)
		return;

	if (!gEnv->bClient)
		return;

	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules && pGameRules->GetSpectatorModule())
	{
		int mode=2;
		if (pArgs->GetArgCount()==2)
			mode=atoi(pArgs->GetArg(1));
		pGameRules->GetSpectatorModule()->ChangeSpectatorMode(pGameRules->GetActorByEntityId(pClientActor->GetEntityId()), mode, 0, true);
	}
}

//------------------------------------------------------------------------
void CGame::CmdJoinGame(IConsoleCmdArgs *pArgs)
{
	if (!gEnv->bClient)
		return;

	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	if (g_pGame->GetGameRules()->GetTeamCount()>0)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules && pGameRules->GetSpectatorModule())
		pGameRules->GetSpectatorModule()->ChangeSpectatorMode(pGameRules->GetActorByEntityId(pClientActor->GetEntityId()), 0, 0, true);
}

//------------------------------------------------------------------------
void CGame::CmdKill(IConsoleCmdArgs *pArgs)
{
	if (!gEnv->bClient)
		return;

	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		HitInfo suicideInfo(pClientActor->GetEntityId(), pClientActor->GetEntityId(), pClientActor->GetEntityId(),
			1000, 0, 0, -1, pGameRules->GetHitTypeId("normal"), ZERO, ZERO, ZERO);

		pGameRules->SanityCheckHitInfo(suicideInfo, "CGame::CmdKill");
		pGameRules->ClientHit(suicideInfo);
	}
}

//------------------------------------------------------------------------
void CGame::CmdRevive(IConsoleCmdArgs *pArgs)
{
	if (!gEnv->bClient)
		return;

	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	CActor *pActor = static_cast<CActor*>(pClientActor);
	pActor->Revive();
}

//------------------------------------------------------------------------
void CGame::CmdVehicleKill(IConsoleCmdArgs *pArgs)
{
	if (!gEnv->bClient)
		return;

	IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	IVehicle* pVehicle = pClientActor->GetLinkedVehicle();
	if (!pVehicle)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		HitInfo suicideInfo(pVehicle->GetEntityId(), pVehicle->GetEntityId(), pVehicle->GetEntityId(),
			10000, 0, 0, -1, 0, pVehicle->GetEntity()->GetWorldPos(), ZERO, ZERO);
		pGameRules->ClientHit(suicideInfo);
	}
}

//------------------------------------------------------------------------
void CGame::CmdRestart(IConsoleCmdArgs *pArgs)
{
	if(g_pGame && g_pGame->GetGameRules())
		g_pGame->GetGameRules()->Restart();
}

//------------------------------------------------------------------------
void CGame::CmdSay(IConsoleCmdArgs *pArgs)
{
	if (gEnv->bServer)
	{
		if (pArgs->GetArgCount()>1)
		{
			const char *msg=pArgs->GetCommandLine()+strlen(pArgs->GetArg(0))+1;
			g_pGame->GetGameRules()->SendTextMessage(eTextMessageServer, msg, eRMI_ToAllClients);

			if (!gEnv->bClient)
				CryLogAlways("** Server: %s **", msg);
		}
	}
	else
	{
		GameWarning("%s is a server-only command! It can't be used on a client.", pArgs->GetArg(0));
	}
}

//------------------------------------------------------------------------
#if !defined(_RELEASE)
void CGame::CmdSendConsoleCommand(IConsoleCmdArgs *pArgs)
{
	if (gEnv->bServer)
	{
		if (pArgs->GetArgCount()>1)
		{
			const char *msg=pArgs->GetCommandLine()+strlen(pArgs->GetArg(0))+1;
			g_pGame->GetGameRules()->SendNetConsoleCommand(msg, eRMI_ToRemoteClients);
		}
	}
	else
	{
		GameWarning("%s is a server-only command! It can't be used on a client.", pArgs->GetArg(0));
	}
}

//------------------------------------------------------------------------
void CGame::CmdNetSetOnlineMode(IConsoleCmdArgs *pArgs)
{
	if (g_pGame->GetIGameFramework()->StartedGameContext()==false && g_pGame->GetIGameFramework()->GetClientChannel()==NULL) // Not in a game.
	{
		if (pArgs->GetArgCount()>1)
		{
			const char *arg = pArgs->GetArg(1);
			if (arg && strcmpi(arg,"online")==0)
				gEnv->pNetwork->GetLobby()->SetLobbyService(eCLS_Online);
			else if (arg && strcmpi(arg,"lan")==0)
				gEnv->pNetwork->GetLobby()->SetLobbyService(eCLS_LAN);
			else
				GameWarning("%s unknown type. Options are lan or online.", pArgs->GetArg(0));
		}
	}
	else
	{
		GameWarning("%s cannot be used while in a game.", pArgs->GetArg(0));
	}
}
#endif

//------------------------------------------------------------------------
void CGame::CmdEcho(IConsoleCmdArgs *pArgs)
{
	const char* message = pArgs->GetCommandLine();
	if (strlen(message) > 5)
	{
		CryLogAlways(pArgs->GetCommandLine() + 5);
	}
}

//------------------------------------------------------------------------
void CGame::CmdLoadActionmap(IConsoleCmdArgs *pArgs)
{
	if(pArgs->GetArg(1))
		g_pGame->LoadActionMaps(pArgs->GetArg(1));
}

//------------------------------------------------------------------------
void CGame::CmdRestartGame(IConsoleCmdArgs *pArgs)
{
	GetISystem()->Relaunch(true);
	GetISystem()->Quit();
}

//------------------------------------------------------------------------
void CGame::CmdDumpAmmoPoolStats(IConsoleCmdArgs *pArgs)
{
	g_pGame->GetWeaponSystem()->DumpPoolSizes();
}

//------------------------------------------------------------------------
void CGame::CmdReloadItems(IConsoleCmdArgs *pArgs)
{
	IItemSystem* pItemSystem = g_pGame->GetIGameFramework()->GetIItemSystem();
	pItemSystem->PreReload();

	g_pGame->GetGameSharedParametersStorage()->ResetItemParameters();
	g_pGame->GetGameSharedParametersStorage()->ResetWeaponParameters();
	
	g_pGame->GetWeaponSystem()->Reload();
	pItemSystem->PostReload();
}

//------------------------------------------------------------------------
void CGame::CmdReloadGameRules(IConsoleCmdArgs *pArgs)
{
	if (gEnv->bMultiplayer)
		return;

	IGameRulesSystem* pGameRulesSystem = g_pGame->GetIGameFramework()->GetIGameRulesSystem();
	IGameRules* pGameRules = pGameRulesSystem->GetCurrentGameRules();

	const char* name = "SinglePlayer";
	IEntityClass* pEntityClass = 0; 

	if (pGameRules)    
	{
		pEntityClass = pGameRules->GetEntity()->GetClass();
		name = pEntityClass->GetName();
	}  
	else
		pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);

	if (pEntityClass)
	{
		pEntityClass->LoadScript(true);

		if (pGameRulesSystem->CreateGameRules(name))
			CryLog("reloaded GameRules <%s>", name);
		else
			GameWarning("reloading GameRules <%s> failed!", name);
	}  
}

//------------------------------------------------------------------------
void CGame::CmdReloadHitDeathReactions(IConsoleCmdArgs* pArgs)
{
	g_pGame->GetHitDeathReactionsSystem().Reload();

	if (pArgs->GetArgCount() > 1)
	{
		IEntity* pEntity = gEnv->pEntitySystem->FindEntityByName(pArgs->GetArg(1));
		if (pEntity)
		{
			IActor* pIActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());
			if (pIActor && (strcmpi(pIActor->GetActorClass(), "CPlayer") == 0))
			{
				CPlayer* pActor = static_cast<CPlayer*>(pIActor);
				CHitDeathReactionsPtr pHitDeathReactions = pActor->GetHitDeathReactions();
				if (pHitDeathReactions)
					pHitDeathReactions->Reload();
			}
		}
	}
	else
	{
		IActorIteratorPtr pIt = g_pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
		while (IActor* pIActor = pIt->Next())
		{
			if (strcmpi(pIActor->GetActorClass(), "CPlayer") == 0)
			{
				CPlayer* pActor = static_cast<CPlayer*>(pIActor);
				CHitDeathReactionsPtr pHitDeathReactions = pActor->GetHitDeathReactions();
				if (pHitDeathReactions)
					pHitDeathReactions->Reload();
			}
		}
	}
}

//------------------------------------------------------------------------
void CGame::CmdNextLevel(IConsoleCmdArgs* pArgs)
{
	ILevelRotation *pLevelRotation = g_pGame->GetIGameFramework()->GetILevelSystem()->GetLevelRotation();
	if (pLevelRotation->GetLength())
		pLevelRotation->ChangeLevel(pArgs);
}

void CGame::CmdStartKickVoting(IConsoleCmdArgs* pArgs)
{
	if (!gEnv->bClient)
		return;

	if (pArgs->GetArgCount() < 2)
	{
		GameWarning("usage: g_startKickVoting player_name");
		return;
	}

	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	IEntity* pEntity = gEnv->pEntitySystem->FindEntityByName(pArgs->GetArg(1));
	if(pEntity)
	{
		IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());
		if(pActor && pActor->IsPlayer())
		{
			CGameRules *pGameRules = g_pGame->GetGameRules();
			if (pGameRules)
			{
				pGameRules->StartVoting(pGameRules->GetActorByEntityId(pClientActor->GetEntityId()),eVS_kick,pEntity->GetId(),"");
			}
		}
	}
}

void CGame::CmdStartNextMapVoting(IConsoleCmdArgs* pArgs)
{
	if (!gEnv->bClient)
		return;

	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->StartVoting(pGameRules->GetActorByEntityId(pClientActor->GetEntityId()),eVS_nextMap,0,"");
	}
}


void CGame::CmdVote(IConsoleCmdArgs* pArgs)
{
	if (!gEnv->bClient)
		return;

	IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
	{
		pGameRules->Vote(pGameRules->GetActorByEntityId(pClientActor->GetEntityId()), true);
	}
}

void CGame::CmdQuickGame(IConsoleCmdArgs* pArgs)
{
//	g_pGame->GetMenu()->GetMPHub()->OnQuickGame();
}

void CGame::CmdQuickGameStop(IConsoleCmdArgs* pArgs)
{

}

void CGame::CmdBattleDustReload(IConsoleCmdArgs* pArgs)
{
	if(CBattleDust* pBD = g_pGame->GetGameRules()->GetBattleDust())
	{
		pBD->ReloadXml();
	}
}

static bool GSCheckComplete()
{
	INetworkService* serv = gEnv->pNetwork->GetService("GameSpy");
	if(!serv)
		return true;
	return serv->GetState() != eNSS_Initializing;
}

static bool GSLoggingIn()
{
	return false; //!g_pGame->GetMenu()->GetMPHub()->IsLoggingIn();
}

void CGame::CmdLogin(IConsoleCmdArgs* pArgs)
{
	if(pArgs->GetArgCount()>2)
	{
		g_pGame->BlockingProcess(&GSCheckComplete);
		INetworkService* serv = gEnv->pNetwork->GetService("GameSpy");
		if(!serv || serv->GetState() != eNSS_Ok)
			return;
		if(gEnv->pSystem->IsDedicated())
		{
			if(INetworkProfile* profile = serv->GetNetworkProfile())
			{
				profile->Login(pArgs->GetArg(1),pArgs->GetArg(2));
			}			
		}
		else
		{
/*			if(g_pGame->GetMenu() && g_pGame->GetMenu()->GetMPHub())
			{
				g_pGame->GetMenu()->GetMPHub()->DoLogin(pArgs->GetArg(1),pArgs->GetArg(2));
				g_pGame->BlockingProcess(&GSLoggingIn);
			}
*/
		}
	}
	else
		GameWarning("Invalid parameters.");
}

void CGame::CmdLoginProfile(IConsoleCmdArgs* pArgs)
{
	if(pArgs->GetArgCount()>3)
	{
		g_pGame->BlockingProcess(&GSCheckComplete);
		INetworkService* serv = gEnv->pNetwork->GetService("GameSpy");
		if(!serv || serv->GetState() != eNSS_Ok)
			return;
/*		g_pGame->GetMenu()->GetMPHub()->DoLoginProfile(pArgs->GetArg(1),pArgs->GetArg(2),pArgs->GetArg(3));

		g_pGame->BlockingProcess(&GSLoggingIn);
*/
	}
	else
		GameWarning("Invalid parameters.");
}

void CGame::CmdReloadPlayer(IConsoleCmdArgs* pArgs)
{
	gEnv->pGame->GetIGameFramework()->GetIActorSystem()->Reload();

	CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(pPlayer)
	{
		pPlayer->ReloadClientXmlData();
	}
}

void CGame::CmdSetPlayerHealth(IConsoleCmdArgs* pArgs)
{
	IActor *pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(pActor)
	{
		if( pArgs->GetArgCount() != 2 )
		{
			CryLog("pl_health : invalid input : 'pl_health <new_health>' (max health is %d)", pActor->GetMaxHealth() );
			return;
		}

		int newHealth = atoi( pArgs->GetArg(1) );
		if( !newHealth && strcmp( "0", pArgs->GetArg(1) ) ) // newHealth is 0 and string input is not "0" i.e. atoi fails to parse
		{
			CryLog("pl_health : int required for input. 'pl_health <new_health>' (max health is %d)", pActor->GetMaxHealth() );
			return;		
		}

		pActor->SetHealth(newHealth);
	}
	else
	{
		CryLog("pl_health : no actor found!");
	}
}

void CGame::CmdSpawnActorAtPos(IConsoleCmdArgs *pArgs)
{
	int argCount=pArgs->GetArgCount();
	//CryLogAlways("CmdSpawnActorAtPos() argCount=%d\n", argCount);

	//for (int i=0; i<5; i++)
	//{
	//	CryLogAlways("CmdSpawnActorAtPos() arg[%d] is %s", i, pArgs->GetArg(i));
	//}

	if (argCount==5)
	{
		const char *actorClass=pArgs->GetArg(1);
		string actorName;
		actorName = "spawned_";
		actorName.append(actorClass);

		Vec3 pos(ZERO);
		Vec3 scale(1.0f, 1.0f, 1.0f);

		pos.x = (float)atof(pArgs->GetArg(2));
		pos.y = (float)atof(pArgs->GetArg(3));
		pos.z = (float)atof(pArgs->GetArg(4));

		CryLogAlways("CmdSpawnActorAtPos() spawning actorClass=%s; with name=%s; at pos=(%f, %f, %f)", actorClass, actorName.c_str(), pos.x, pos.y, pos.z);
		IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActor(0, actorName.c_str(), actorClass, pos, Quat(IDENTITY), scale);
		if (pActor)
		{
			CryLogAlways("CmdSpawnActorAtPos() successfully created actor");
		}
		else
		{
			CryLogAlways("CmdSpawnActorAtPos() failed to create actor");
		}
	}
	else
	{
		CryLogAlways("CmdSpawnActorAtPos() unhandled number of arguments %d (expecting 5)", argCount);
	}
}

void CGame::CmdSpawnDummyPlayer(IConsoleCmdArgs* pArgs)
{
	if(!gEnv->bServer)
		return;
#if 0
	const CCamera &rCam = gEnv->pRenderer->GetCamera();

	Vec3 pos = rCam.GetPosition();
	Vec3 forward = rCam.GetViewdir();
#else
	CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	Vec3 pos = pPlayer->GetEntity()->GetPos();
	Vec3 forward = pPlayer->GetEntity()->GetWorldRotation().GetColumn1();
#endif

	Vec3 scale(1.0f, 1.0f, 1.0f);

	static int numDummiesSoFar = 0;
	int numDummies = 1;
	bool chooseTeams = true;

	if (pArgs->GetArgCount() > 1)
	{
		numDummies = atoi(pArgs->GetArg(1));
	}

	string commandLine = pArgs->GetCommandLine();
	if (commandLine.find("noteams") != string::npos)
	{
		chooseTeams = false;
	}

	CGameRules* pGameRules = g_pGame->GetGameRules();
	if(pGameRules)
	{
		for (int i=0; i<numDummies; i++)
		{
			pos += forward * 2;
			CryFixedStringT<16> name("DummyPlayer");
			name.Format("%s%d", name.c_str(), numDummiesSoFar+i);
			CDummyPlayer* pDummy = (CDummyPlayer*)gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActor(0, name.c_str(), "DummyPlayer", pos, Quat(IDENTITY), scale);
			if (chooseTeams)
			{
				IGameRulesTeamsModule *teamModule = pGameRules->GetTeamsModule();
				if (teamModule)
				{
					EntityId playerEntityId = pDummy->GetEntityId();
					pGameRules->SetTeam(teamModule->GetAutoAssignTeamId(playerEntityId), playerEntityId);
				}
			}
		}
	}

	numDummiesSoFar += numDummies;
}

void CGame::CmdRemoveDummyPlayers(IConsoleCmdArgs* pArgs)
{
	if(!gEnv->bServer)
		return;

	int numPlayersRemoved = 0;
	IEntityClass* pDummyClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DummyPlayer");
	IEntityItPtr entIt = gEnv->pEntitySystem->GetEntityIterator();
	if (entIt)
	{
		entIt->MoveFirst();
		while (!entIt->IsEnd())
		{
			IEntity* ent = entIt->Next();
			if (ent && ent->GetClass() == pDummyClass)
			{
				gEnv->pEntitySystem->RemoveEntity(ent->GetId());
				numPlayersRemoved++;
			}
		}
	}
	CryLogAlways("CmdRemoveDummyPlayers() removed %d dummy players", numPlayersRemoved);
}

void CGame::CmdSetDummyPlayerState(IConsoleCmdArgs* pArgs)
{
	if (pArgs->GetArgCount() > 1)
	{
		const char* entityName = pArgs->GetArg(1);
		IEntity* pEntity = gEnv->pEntitySystem->FindEntityByName(entityName);
		if (pEntity)
		{
			IEntityClass* pDummyClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DummyPlayer");
			if (pEntity->GetClass() == pDummyClass)
			{
				CDummyPlayer* pDummyPlayer = (CDummyPlayer*)g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());
				for (int i = 2; i<pArgs->GetArgCount(); i++)
				{
					bool parsed = false;
					string stateString = pArgs->GetArg(i);
					int pos = 0;
					string key = stateString.Tokenize(":", pos);
					if (!key.empty())
					{
						string valueString = stateString.Tokenize(":", pos);
						if (!valueString.empty())
						{
							parsed = true;
							bool validValue = true;
							EDefaultableBool value = eDB_Default;
							if (valueString.compareNoCase("default") == 0 || valueString.compareNoCase("-1") == 0)
							{
								value = eDB_Default;
							}
							else if (valueString.compareNoCase("false") == 0 || valueString.compareNoCase("0") == 0)
							{
								value = eDB_False;
							}
							else if (valueString.compareNoCase("true") == 0 || valueString.compareNoCase("1") == 0)
							{
								value = eDB_True;
							}
							else
							{
								validValue = false;
								CryLogAlways("CmdSetDummyPlayerState() unrecognised value: %s in %s", valueString.c_str(), stateString.c_str());
							}
							if (validValue)
							{
								if (key.compareNoCase("fire") == 0)
								{
									pDummyPlayer->SetFire(value);
								}
								else if (key.compareNoCase("move") == 0)
								{
									pDummyPlayer->SetMove(value);
								}
								else
								{
									CryLogAlways("CmdSetDummyPlayerState() unrecognised key: %s in %s", key.c_str(), stateString.c_str());
								}
							}
						}
					}
					if (!parsed)
					{
						CryLogAlways("CmdSetDummyPlayerState() unrecognised argument: %s", stateString.c_str());
					}
				}
			}
			else
			{
				CryLogAlways("CmdSetDummyPlayerState() wrong type: %s is a %s and not a DummyPlayer", entityName, pEntity->GetClass()->GetName());
			}
		}
		else
		{
			CryLogAlways("CmdSetDummyPlayerState() couldn't find %s", entityName);
		}
	}
	else
	{
		CryLogAlways("CmdSetDummyPlayerState() must be given some parameters e.g. setDummyPlayerState DummyPlayer0 fire:true move:default");
	}
}

static bool gGSConnecting = false;

struct SCryNetConnectListener : public IServerListener
{
	virtual void RemoveServer(const int id){}
	virtual void UpdatePing(const int id,const int ping){}
	virtual void UpdateValue(const int id,const char* name,const char* value){}
	virtual void UpdatePlayerValue(const int id,const int playerNum,const char* name,const char* value){}
	virtual void UpdateTeamValue(const int id,const int teamNum,const char *name,const char* value){}
	virtual void UpdateComplete(bool cancelled){}

	//we only need this thing to connect to server

	virtual void OnError(const EServerBrowserError)
	{
		End(false);
	}

	virtual void NewServer(const int id,const SBasicServerInfo* info)
	{
		UpdateServer(id, info);
	}

	virtual void UpdateServer(const int id,const SBasicServerInfo* info)
	{
		m_port = info->m_hostPort;
	}

	virtual void ServerUpdateFailed(const int id)
	{
		End(false);
	}
	virtual void ServerUpdateComplete(const int id)
	{ 
		m_browser->CheckDirectConnect(id,m_port);
	}

	virtual void ServerDirectConnect(bool neednat, uint32 ip, uint16 port)
	{
		string connect;
		if(neednat)
		{
			int cookie = rand() + (rand()<<16);
			connect.Format("connect <nat>%d|%d.%d.%d.%d:%d",cookie,ip&0xFF,(ip>>8)&0xFF,(ip>>16)&0xFF,(ip>>24)&0xFF,port);
			m_browser->SendNatCookie(ip,port,cookie);
		}
		else
		{
			connect.Format("connect %d.%d.%d.%d:%d",ip&0xFF,(ip>>8)&0xFF,(ip>>16)&0xFF,(ip>>24)&0xFF,port);
		}
		m_browser->Stop();
		End(true);
		g_pGame->GetIGameFramework()->ExecuteCommandNextFrame(connect.c_str());
	}

	void End(bool success)
	{
		if(!success)
			CryLog("Server is not responding.");
		gGSConnecting = false;
		m_browser->Stop();
		m_browser->SetListener(0);
		delete this;
	}

	IServerBrowser* m_browser;
	uint16 m_port;
};


static bool GSConnect()
{
	return !gGSConnecting;
}

void CGame::CmdCryNetConnect(IConsoleCmdArgs* pArgs)
{
	uint16 port = 64087;

	if(pArgs->GetArgCount()>2)
		port = atoi(pArgs->GetArg(2));
	else
	{
		ICVar* pv = gEnv->pConsole->GetCVar("cl_serverport");
		if(pv)
			port = pv->GetIVal();
	}
	if(pArgs->GetArgCount()>1)
	{
		g_pGame->BlockingProcess(&GSCheckComplete);
		INetworkService* serv = gEnv->pNetwork->GetService("GameSpy");
		if(!serv || serv->GetState() != eNSS_Ok)
			return;
		IServerBrowser* sb = serv->GetServerBrowser();

		SCryNetConnectListener* lst = new SCryNetConnectListener();
		lst->m_browser = sb;
		sb->SetListener(lst);
		sb->Start(false);
		sb->BrowseForServer(pArgs->GetArg(1),port);
		gGSConnecting = true;
		g_pGame->BlockingProcess(&GSConnect);
	}
	else
	{
		GameWarning("Invalid parameters.");
	}
}
