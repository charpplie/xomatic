/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:	CVars for the AnimationGraph Subsystem

-------------------------------------------------------------------------
History:
- 02:03:2006  12:00 : Created by AlexL

*************************************************************************/

#include "StdAfx.h"
#include "AnimationGraphCVars.h"
#include "AnimationGraphManager.h"
#include "AnimationGraphState.h"
#include <CryAction.h>

CAnimationGraphCVars* CAnimationGraphCVars::s_pThis = 0;

namespace
{
	void ReloadGraphs_FromXml(IConsoleCmdArgs* /* pArgs */)
	{
		CAnimationGraphManager * pAnimGraph = CCryAction::GetCryAction()->GetAnimationGraphManager();
		pAnimGraph->ReloadAllGraphs(false);
	}

	void ReloadGraphs_FromBin(IConsoleCmdArgs* /* pArgs */)
	{
		CAnimationGraphManager * pAnimGraph = CCryAction::GetCryAction()->GetAnimationGraphManager();
		pAnimGraph->ReloadAllGraphs(true);
	}

	void SetVariationInput(IConsoleCmdArgs* pArgs)
	{
		if (pArgs->GetArgCount() != 4)
		{
			CryLogAlways("Error. Syntax: ag_setVariationInput entityName variationInput value");
			return;
		}

		IActor * pActor = NULL;
		if (0 == strcmp(pArgs->GetArg(1), "1"))
		{
			pActor = CCryAction::GetCryAction()->GetClientActor();
		}
		else if (IEntity * pEntity = gEnv->pEntitySystem->FindEntityByName(pArgs->GetArg(1)))
		{
			pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());
		}

		if (!pActor)
		{
			CryLogAlways("Error, Actor %s not found.", pArgs->GetArg(1));
			return;
		}

		pActor->GetAnimationGraphState()->SetVariationInput(pArgs->GetArg(2), pArgs->GetArg(3));
	}

	void ChangeDebug(ICVar* pICVar)
	{
		const char* pVal = pICVar->GetString();
		CAnimationGraphState::ChangeDebug(pVal);
	}

	void ChangeDebugLayer(ICVar* pICVar)
	{
		int iVal = pICVar->GetIVal();
		CAnimationGraphState::ChangeDebugLayer(iVal);
	}

	void SingleStep(IConsoleCmdArgs* /*pArgs*/)
	{
		CAnimationGraphState::Debug_SingleStep();
	}

	void TestPlanner(IConsoleCmdArgs*)
	{
		CAnimationGraphState::TestPlanner();
	}
};

CAnimationGraphCVars::CAnimationGraphCVars()
{
	assert (s_pThis == 0);
	s_pThis = this;

	IConsole *pConsole = gEnv->pConsole;

	REGISTER_COMMAND( "ag_reload_xml", ReloadGraphs_FromXml, VF_CHEAT,"Reloads all Animation Graphs from their xml files");
	REGISTER_COMMAND( "ag_reload_ag", ReloadGraphs_FromBin, VF_CHEAT,"Reloads all Animation Graphs from their ag files");
	REGISTER_COMMAND( "ag_step", SingleStep, VF_CHEAT,"");
	REGISTER_COMMAND( "ag_testplanner", TestPlanner, VF_CHEAT,"");
	REGISTER_COMMAND( "ag_variationInput", SetVariationInput, VF_CHEAT, "Set this Variation Input once");

	// TODO: remove once animation graph transition is complete
	REGISTER_CVAR2( "ag_debugExactPos", &m_debugExactPos, 0, VF_CHEAT, "Enable/disable exact positioning debugger" );
	REGISTER_CVAR2( "ag_ep_correctMovement", &m_correctMovementInExactPositioning, 1, VF_CHEAT, "enable/disable position correction in exact positioning" );
	REGISTER_CVAR2( "ag_ep_showPath", &m_showExactPositioningCorrectionPath, 0, VF_CHEAT,"" );
	REGISTER_CVAR2( "ag_humanBlending", &m_agHumanBlending, 0, VF_CHEAT, "Ivo's debug stuff. Don't ask!" );
	// ~TODO
	REGISTER_CVAR2( "ag_logeffects", &m_logEffects, 0, 0, "AGAttachmentEffect logging" );    
	REGISTER_CVAR2( "ag_logsounds", &m_logSounds, 0, 0, "AGSound logging" );
	REGISTER_CVAR2( "ag_showPhysSync", &m_showPhysSync, 0, /*VF_CHEAT*/ 0, "Show physics sync" );

	REGISTER_CVAR2( "ag_lockToEntity", &m_LockToEntity, 0, 0, "Lock animation to entity (zero offsetting)" );
	REGISTER_CVAR2( "ag_adjustToCatchUp", &m_adjustToCatchUp, 0, 0, "Adjust requested move direction of animation to catch up with entity" );
	REGISTER_CVAR2( "ag_forceAdjust", &m_ForceAdjust, 0, 0, "Enable forced small step adjustments" );
	REGISTER_CVAR2( "ag_forceInsideErrorDisc", &m_forceInsideErrorDisc, 1, 0, "Force animation to stay within maximum error distance" );
	REGISTER_CVAR2( "ag_measureActualSpeeds", &m_MeasureActualSpeeds, 0, 0, "Measure actual travel speeds of entity and animation origins" );
	REGISTER_CVAR2( "ag_averageTravelSpeed", &m_averageTravelSpeed, 0, 0, "Average travel speed over a few frames" );
	REGISTER_CVAR2( "ag_safeExactPositioning", &m_SafeExactPositioning, 1, 0, "Will teleport the entity to the requested position/orientation when EP think it's done." );

	REGISTER_CVAR2( "ac_forceSimpleMovement", &m_forceSimpleMovement, 0, VF_CHEAT, "Force enable simplified movement (not visible, dedicated server, etc)." );

	REGISTER_CVAR2( "ac_debugAnimEffects", &m_debugAnimEffects, 0, VF_CHEAT, "Print log messages when anim events spawn effects." );

	//REGISTER_CVAR2( "ac_predictionSmoothingPos", &m_PredictionSmoothingPos, 0.0f, 0, "." );
	//REGISTER_CVAR2( "ac_predictionSmoothingOri", &m_PredictionSmoothingOri, 0.0f, 0, "." );
	//REGISTER_CVAR2( "ac_predictionProbabilityPos", &m_PredictionProbabilityPos, -1.0f, 0, "." );
	//REGISTER_CVAR2( "ac_predictionProbabilityOri", &m_PredictionProbabilityOri, -1.0f, 0, "." );

	//REGISTER_CVAR2( "ac_triggercorrectiontimescale", &m_TriggerCorrectionTimeScale, 0.5f, 0, "." );
	//REGISTER_CVAR2( "ac_targetcorrectiontimescale", &m_TargetCorrectionTimeScale, 4.0f, 0, "." );

	REGISTER_CVAR2( "ac_movementControlMethodHor", &m_MCMH, 0, VF_CHEAT, "Overrides the horizontal movement control method specified by AG (overrides filter)." );
	REGISTER_CVAR2( "ac_movementControlMethodVer", &m_MCMV, 0, VF_CHEAT, "Overrides the vertical movement control method specified by AG (overrides filter)." );
	REGISTER_CVAR2( "ac_movementControlMethodFilter", &m_MCMFilter, 0, VF_CHEAT, "Force reinterprets Decoupled/CatchUp MCM specified by AG as Entity MCM (H/V overrides override this)." );
	REGISTER_CVAR2( "ac_templateMCMs", &m_TemplateMCMs, 1, VF_CHEAT, "Use MCMs from AG state templates instead of AG state headers." );
	REGISTER_CVAR2( "ac_ColliderModePlayer", &m_forceColliderModePlayer, 0, VF_CHEAT, "Force override collider mode for all players." );
	REGISTER_CVAR2( "ac_ColliderModeAI", &m_forceColliderModeAI, 0, VF_CHEAT, "Force override collider mode for all AI." );
	REGISTER_CVAR2( "ac_enableExtraSolidCollider", &m_enableExtraSolidCollider, 0, VF_CHEAT, "Enable extra solid collider (for non-pushable characters)." );

	REGISTER_CVAR2( "ac_disableFancyTransitions", &m_disableFancyTransitions, 0, VF_CHEAT, "Disabled Idle2Move and Move2Idle special transition animations." );

	REGISTER_CVAR2( "ac_entityAnimClamp", &m_entityAnimClamp, 1, VF_CHEAT, "Forces the entity movement to be limited by animation." );
	REGISTER_CVAR2( "ac_animErrorClamp", &m_animErrorClamp, 1, VF_CHEAT, "Forces the animation to stay within the maximum error distance/angle." );
	REGISTER_CVAR2( "ac_animErrorMaxDistance", &m_animErrorMaxDistance, 0.5f, VF_CHEAT, "Meters animation location is allowed to stray from entity." );
	REGISTER_CVAR2( "ac_animErrorMaxAngle", &m_animErrorMaxAngle, 45.0f, VF_CHEAT, "Degrees animation orientation is allowed to stray from entity." );
	REGISTER_CVAR2( "ac_debugAnimError", &m_debugAnimError, 0, 0, "Display debug history graphs of anim error distance and angle." );
	REGISTER_CVAR2( "ac_debugAnimTarget", &m_debugAnimTarget, 0, 0, "Display debug history graphs of anim target correction." );
	REGISTER_CVAR2( "ac_debugLocationsGraphs", &m_debugLocationsGraphs, 0, 0, "Display debug history graphs of anim and entity locations and movement." );

	REGISTER_CVAR2( "ac_debugLocations", &m_debugLocations, 0, 0, "Debug render entity location (blue) and logical animation location (red)." );
	REGISTER_CVAR2( "ac_debugSelection", &m_debugSelection, 0, 0, "Display locomotion state selection as text." );
	REGISTER_CVAR2( "ac_debugMotionParams", &m_debugMotionParams, 0, 0, "Display graph of motion parameters." );
	REGISTER_CVAR2( "ac_debugFutureAnimPath", &m_debugFutureAnimPath, 0, 0, "Display future animation path given current motion parameters." );
	REGISTER_CVAR2( "ac_debugPrediction", &m_debugPrediction, 0, 0, "Display graph of motion parameters." );

	REGISTER_CVAR2( "ac_frametime", &m_debugFrameTime, 0, 0, "Display a graph of the frametime." );
	
	REGISTER_CVAR2( "ac_debugSelectionParams", &m_debugSelectionParams, 0, 0, "Display graph of selection parameters values." );
	REGISTER_CVAR2( "ac_debugTweakTrajectoryFit", &m_debugTweakTrajectoryFit, 0, VF_CHEAT, "Don't apply any movement to entity and animation, but allow calculations to think they are moving normally." );
	REGISTER_CVAR2( "ac_debugText", &m_debugText, 0, 0, "Display entity/animation location/movement values, etc." );
	REGISTER_CVAR2( "ac_debugXXXValues", &m_debugTempValues, 0, 0, "Display some values temporarily hooked into temp history graphs." );
	REGISTER_CVAR2( "ac_debugEntityParams", &m_debugEntityParams, 0 , VF_CHEAT, "Display entity params graphs" );
	m_pDebugFilter = REGISTER_STRING( "ac_DebugFilter", "0", 0, "Debug specified entity name only." );

	REGISTER_CVAR2( "ac_debugColliderMode", &m_debugColliderMode, 0, 0, "Display filtered and requested collider modes." );
	REGISTER_CVAR2( "ac_debugMovementControlMethods", &m_debugMovementControlMethods, 0, 0, "Display movement control methods." );

	REGISTER_CVAR2( "ac_disableSlidingContactEvents", &m_disableSlidingContactEvents, 0, 0, "Force disable sliding contact events." );

	REGISTER_CVAR2( "ac_clampTimeEntity", &m_clampDurationEntity, 0.7f, VF_CHEAT, "Time it takes for carry clamping to reduce the deviation to zero." );
	REGISTER_CVAR2( "ac_clampTimeAnimation", &m_clampDurationAnimation, 0.3f, VF_CHEAT, "Time it takes for carry clamping to reduce the deviation to zero." );
	REGISTER_CVAR2( "ac_debugCarryCorrection", &m_debugCarryClamping, 0, 0, "." );

	REGISTER_CVAR2( "ag_debugErrors", &m_debugErrors, 0, 0, "Displays debug error info on the entities (0/1)" );

	//REGISTER_CVAR2( "ag_physErrorInnerRadiusFactor", &m_physErrorInnerRadiusFactor, 0.05f, VF_DUMPTODISK, "");
	//REGISTER_CVAR2( "ag_physErrorOuterRadiusFactor", &m_physErrorOuterRadiusFactor, 0.2f, VF_DUMPTODISK, "");
	//REGISTER_CVAR2( "ag_physErrorMinOuterRadius", &m_physErrorMinOuterRadius, 0.2f, VF_DUMPTODISK, "");
	//REGISTER_CVAR2( "ag_physErrorMaxOuterRadius", &m_physErrorMaxOuterRadius, 0.5f, VF_DUMPTODISK, "");

	REGISTER_CVAR2( "ag_fpAnimPop", &m_fpAnimPop, 1, VF_DUMPTODISK, "" );

	REGISTER_CVAR2( "ag_debugAdditives", &m_debugAdditives, 0, VF_CHEAT, "Print log messages for Additives." );


	REGISTER_STRING("ag_debug", "", VF_CHEAT, "Entity to display debug information for animation graph for")->SetOnChangeCallback(ChangeDebug);
	REGISTER_CVAR2("ag_debugLayer", &m_debugLayer, 0, 0, "Animation graph layer to display debug information for")->SetOnChangeCallback(ChangeDebugLayer);
	REGISTER_CVAR2("ag_debugMusic", &m_debugMusic, 0, VF_CHEAT, "Debug the music graph");
	m_pQueue = REGISTER_STRING("ag_queue", "", VF_CHEAT, "Next state to force");
	m_pSignal = REGISTER_STRING("ag_signal", "", VF_CHEAT, "Send this signal");
	m_pAction = REGISTER_STRING("ag_action", "", VF_CHEAT, "Force this action");
	m_pItem = REGISTER_STRING("ag_item", "", VF_CHEAT, "Force this item");
	m_pStance = REGISTER_STRING("ag_stance", "", VF_CHEAT, "Force this stance");
	m_pBreakOnQuery = REGISTER_STRING("ag_breakOnQuery", "", VF_CHEAT, "If we query for this state, enter break mode");
	REGISTER_INT("ag_drawActorPos", 0, VF_CHEAT, "Draw actor pos/dir");

	REGISTER_CVAR2( "ag_logselections", &m_logSelections, 0, VF_CHEAT, "Log animation graph selection results");
	REGISTER_CVAR2( "ag_logtransitions",&m_logTransitions,  0, VF_CHEAT, "Log animation graph transition calls to the console");
	REGISTER_CVAR2( "ag_breakmode",&m_breakMode,  0, VF_CHEAT, "1=Enable debug break mode; 2=also lock inputs");
	REGISTER_CVAR2( "ag_log", &m_log, 0, VF_CHEAT, "Enable a log of animation graph decisions" );
	m_pLogEntity = REGISTER_STRING( "ag_log_entity", "", VF_CHEAT, "Log only this entity" );

	m_pCaDebug = 0; // Kept for reference, unused

	m_pShowMovementRequests = REGISTER_INT("ag_showmovement", 0, VF_CHEAT, "Show movement requests");

	ca_GameControlledStrafingPtr = pConsole->GetCVar("ca_GameControlledStrafing");

	REGISTER_CVAR2( "g_distanceForceNoIk", &m_distanceForceNoIk, 0.0f, VF_CHEAT, "Distance at which to disable ground alignment IK");
	REGISTER_CVAR2( "g_spectatorCollisions", &m_spectatorCollisions, 0, VF_CHEAT, "Collide against the geometry in spectator mode");
}

CAnimationGraphCVars::~CAnimationGraphCVars()
{
	assert (s_pThis != 0);
	s_pThis = 0;

	IConsole *pConsole = gEnv->pConsole;

	pConsole->RemoveCommand("ag_reload_xml");
	pConsole->RemoveCommand("ag_reload_ag");
	pConsole->RemoveCommand("ag_step");
	pConsole->RemoveCommand("ag_testplanner");
	pConsole->RemoveCommand("ag_variationInput");

	pConsole->UnregisterVariable("ag_debugExactPos", true);
	pConsole->UnregisterVariable("ag_ep_correctMovement", true);
	pConsole->UnregisterVariable("ag_ep_showPath", true);
	pConsole->UnregisterVariable("ag_humanBlending", true);
	
	pConsole->UnregisterVariable("ag_logeffects", true);
	pConsole->UnregisterVariable("ag_logsounds", true);
	pConsole->UnregisterVariable("ag_showPhysSync", true);

	pConsole->UnregisterVariable("ag_lockToEntity", true);
	pConsole->UnregisterVariable("ag_adjustToCatchUp", true);
	pConsole->UnregisterVariable("ag_forceAdjust", true);
	pConsole->UnregisterVariable("ag_forceInsideErrorDisc", true);
	pConsole->UnregisterVariable("ag_measureActualSpeeds", true);
	pConsole->UnregisterVariable("ag_averageTravelSpeed", true);
	pConsole->UnregisterVariable("ag_safeExactPositioning", true);

	pConsole->UnregisterVariable("ac_predictionSmoothingPos", true);
	pConsole->UnregisterVariable("ac_predictionSmoothingOri", true);
	pConsole->UnregisterVariable("ac_predictionProbabilityPos", true);
	pConsole->UnregisterVariable("ac_predictionProbabilityOri", true);

	pConsole->UnregisterVariable("ac_triggercorrectiontimescale", true);
	pConsole->UnregisterVariable("ac_targetcorrectiontimescale", true);

	pConsole->UnregisterVariable("ac_new", true);
	pConsole->UnregisterVariable("ac_movementControlMethodHor", true);
	pConsole->UnregisterVariable("ac_movementControlMethodVer", true);
	pConsole->UnregisterVariable("ac_movementControlMethodFilter", true);
	pConsole->UnregisterVariable("ac_templateMCMs", true);

	pConsole->UnregisterVariable("ac_entityAnimClamp", true);
	pConsole->UnregisterVariable("ac_animErrorClamp", true);
	pConsole->UnregisterVariable("ac_animErrorMaxDistance", true);
	pConsole->UnregisterVariable("ac_animErrorMaxAngle", true);
	pConsole->UnregisterVariable("ac_debugAnimError", true);

	pConsole->UnregisterVariable("ac_debugLocations", true);
	pConsole->UnregisterVariable("ac_debugSelection", true);
	pConsole->UnregisterVariable("ac_debugMotionParams", true);
	pConsole->UnregisterVariable("ac_debugFutureAnimPath", true);
	pConsole->UnregisterVariable("ac_debugPrediction", true);

	pConsole->UnregisterVariable("ac_frametime", true);

	pConsole->UnregisterVariable("ac_debugSelectionParams", true);
	pConsole->UnregisterVariable("ac_debugTweakTrajectoryFit", true);
	pConsole->UnregisterVariable("ac_debugText", true);
	pConsole->UnregisterVariable("ac_debugXXXValues", true);
	pConsole->UnregisterVariable("ac_debugEntityParams", true);
	pConsole->UnregisterVariable("ac_DebugFilter", true);

	pConsole->UnregisterVariable("ac_debugColliderMode", true);
	pConsole->UnregisterVariable("ac_debugMovementControlMethods", true);

	pConsole->UnregisterVariable("ac_disableSlidingContactEvents", true);

	pConsole->UnregisterVariable("ac_clampTimeEntity", true);
	pConsole->UnregisterVariable("ac_clampTimeAnimation", true);
	pConsole->UnregisterVariable("ac_debugCarryCorrection", true);

	pConsole->UnregisterVariable("ag_forceStanceTimeout", true);
	pConsole->UnregisterVariable("ag_debugErrors", true);

	pConsole->UnregisterVariable("ag_physErrorInnerRadiusFactor", true);
	pConsole->UnregisterVariable("ag_physErrorOuterRadiusFactor", true);
	pConsole->UnregisterVariable("ag_physErrorMinOuterRadius", true);
	pConsole->UnregisterVariable("ag_physErrorMaxOuterRadius", true);

	pConsole->UnregisterVariable("ag_fpAnimPop", true);

	pConsole->UnregisterVariable("ag_debug", true);
	pConsole->UnregisterVariable("ag_debugLayer", true);
	pConsole->UnregisterVariable("ag_debugMusic", true);
	pConsole->UnregisterVariable("ag_queue", true);
	pConsole->UnregisterVariable("ag_signal", true);
	pConsole->UnregisterVariable("ag_action", true);
	pConsole->UnregisterVariable("ag_item", true);
	pConsole->UnregisterVariable("ag_stance", true);
	pConsole->UnregisterVariable("ag_breakOnQuery", true);
	pConsole->UnregisterVariable("ag_drawActorPos", true);

	pConsole->UnregisterVariable("ag_logselections", true);
	pConsole->UnregisterVariable("ag_logtransitions", true);
	pConsole->UnregisterVariable("ag_breakmode", true);
	pConsole->UnregisterVariable("ag_log", true);
	pConsole->UnregisterVariable("ag_log_entity", true);

	pConsole->UnregisterVariable("ag_showmovement", true);

	pConsole->UnregisterVariable("g_distanceForceNoIk", true);
	pConsole->UnregisterVariable("g_spectatorCollisions", true);

}
