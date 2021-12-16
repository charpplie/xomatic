#include "StdAfx.h"
#include "AIConsoleVariables.h"


void AIConsoleVars::Init()
{
	AILogProgress("[AISYSTEM] Initialization: Creating console vars");

	IConsole *pConsole = gEnv->pConsole;

	pConsole->Register("ai_CompatibilityMode", &CompatibilityMode, "", VF_CHEAT,
		"Set AI features to behave in earlier milestones - please use sparingly");

	pConsole->Register("ai_DebugDrawAStarOpenList", &DebugDrawAStarOpenList, "0", VF_CHEAT|VF_DUMPTODISK,
		"Draws the A* open list for the specified AI agent.\n"
		"Usage: ai_DebugDrawAStarOpenList [AI agent name]\n"
		"Default is 0, which disables the debug draw. Requires ai_DebugPathfinding=1 to be activated.");

	pConsole->Register("ai_DrawNode", &DrawNode, "none", VF_CHEAT|VF_DUMPTODISK,
		"Toggles visibility of named agent's position on AI triangulation.\n"
		"See also: ai_DrawNodeLinkType and ai_DrawNodeLinkCutoff\n"
		"Usage: ai_DrawNode [ai agent's name]\n"
		" none - switch off\n"
		" all - to display nodes of all the AI agents\n"
		" player - to display player's node\n"
		" AI agent's name - to display node of particular agent");
	pConsole->Register("ai_Locate", &DrawLocate, "none", VF_CHEAT|VF_DUMPTODISK,
		"Indicates position and some base states of specified objects.\n"
		"It will pinpoint position of the agents; it's name; it's attention target;\n"
		"draw red cone if the agent is allowed to fire; draw purple cone if agent is pressing trigger.\n"
		" none - off\n"
		" squad - squadmates\n"
		" enemy - all the enemies\n"
		" groupID - members of specified group");
	pConsole->Register("ai_DrawPath", &DrawPath, "none", VF_CHEAT|VF_DUMPTODISK,
		"Draws the generated paths of the AI agents. ai_drawoffset is used.\n"
		"Usage: ai_DrawPath [name]\n"
		" none - off (default)\n"
		" squad - squadmates\n"
		" enemy - all the enemies");
	pConsole->Register("ai_DrawPathAdjustment", &DrawPathAdjustment, "none", VF_CHEAT|VF_DUMPTODISK,
		"Draws the path adjustment for the AI agents.\n"
		"Usage: ai_DrawPathAdjustment [name]\n"
		"Default is none (nobody).");
	pConsole->Register("ai_DrawRefPoints", &DrawRefPoints, "", VF_CHEAT|VF_DUMPTODISK,
		"Toggles reference points and beacon view for debugging AI.\n"
		"Usage: ai_DrawRefPoints [0/1]\n"
		"Default is 0 (off). Indicates the AI reference points by drawing\n"
		"cyan balls at their positions.");	
	pConsole->Register("ai_StatsTarget", &StatsTarget, "none", VF_CHEAT|VF_DUMPTODISK,
		"Focus debugging information on a specific AI\n"
		"Display current goal pipe, current goal, subpipes and agentstats information for the selected AI agent.\n"
		"Long green line will represent the AI forward direction (game forward).\n"
		"Long red/blue (if AI firing on/off) line will represent the AI view direction.\n"
		"Usage: ai_StatsTarget AIName\n"
		"Default is 'none'. AIName is the name of the AI\n"
		"on which to focus.");
	pConsole->Register("ai_DebugDrawPhysicsAccess", &DebugDrawPhysicsAccess, 0, VF_CHEAT|VF_DUMPTODISK,
		"Displays current physics access statistics.");
	pConsole->Register("ai_BehaviorStatsTarget", &BehaviorStatsTarget, "none", VF_CHEAT|VF_DUMPTODISK,
		"Focus behaviour debugging information on a specific AI\n"
		"Usage: ai_BehaviorStatsTarget AIName\n"
		"Default is 'none'. AIName is the name of the AI\n"
		"on which to focus.");
	pConsole->Register("ai_DebugBehaviorSelection", &DebugBehaviorSelection, "", VF_CHEAT|VF_DUMPTODISK,
		"Display behavior selection information for a specific agent\n"
		"Usage: ai_DebugBehaviorSelection <name>");
	pConsole->Register("ai_DebugDrawCoopReadabilities", &DebugDrawCoopReadabilities, "none", VF_CHEAT|VF_DUMPTODISK,
		"Draw debug information of readability play session with named AI Actor\n"
		"Usage: ai_DebugDrawCoopReadabilities AIName\n"
		"Default is 'none'. AIName is the name of the AI\n"
		"on which to focus.");

	pConsole->Register("ai_DebugHideSpotName", &DebugHideSpotName, "0", VF_CHEAT,
		"Debug HideSpot Name!");
	pConsole->Register("ai_DrawShooting", &DrawShooting, "none", VF_CHEAT,
		"Name of puppet to show fire stats");
	pConsole->Register("ai_DrawAgentStats", &DrawAgentStats, "NcBbtGgSf", 0,
		"Flag field specifying which of the overhead agent stats to display:\n"
		"N - name\n"
		"d - distances\n"
		"c - cover info\n"
		"B - currently selected behavior node\n"
		"b - current behavior\n"
		"t - target info\n"
		"G - goal pipe\n"
		"g - goal op\n"
		"S - stance\n"
		"f - fire\n"
		"w - territory/wave\n");
	pConsole->Register("ai_DrawPerceptionHandlerModifiers", &DrawPerceptionHandlerModifiers, "none", VF_CHEAT|VF_DUMPTODISK,
		"Draws perception handler modifiers on a specific AI\n"
		"Usage: ai_DrawPerceptionHandlerModifiers AIName\n"
		"Default is 'none'. AIName is the name of the AI");
	pConsole->Register("ai_ForceAGAction", &ForceAGAction, "0", VF_CHEAT,
		"Forces all AI characters to specified AG action input. 0 to disable.\n");
	pConsole->Register("ai_ForceAGSignal", &ForceAGSignal, "0", VF_CHEAT,
		"Forces all AI characters to specified AG signal input. 0 to disable.\n");
	pConsole->Register("ai_ForcePosture", &ForcePosture, "0", VF_CHEAT,
		"Forces all AI characters to specified posture. 0 to disable.\n");
	pConsole->Register("ai_ForceLookAimTarget", &ForceLookAimTarget, "none", VF_CHEAT,
		"Forces all AI characters to use/not use a fixed look/aim target\n"
		"none disables\n"
		"x, y, xz or yz sets it to the appropriate direction\n"
		"otherwise it forces looking/aiming at the entity with this name (no name -> (0, 0, 0))");
	pConsole->Register("ai_DebugDrawHashSpaceAround", &DebugDrawHashSpaceAround, "0", VF_CHEAT|VF_DUMPTODISK,
		"Validates and draws the navigation node hash space around specified entity.");
	pConsole->Register("ai_BTDebug", &DebugBehaviorTree, "", VF_CHEAT | VF_DUMPTODISK, "Behavior Tree debug");
	pConsole->Register("ai_ACDebug", &DebugActivationConditions, "", VF_CHEAT | VF_DUMPTODISK, "Activation Conditions debug");
	pConsole->Register("ai_emo", &DebugEmotionalSystem, "", VF_CHEAT | VF_DUMPTODISK, "Emotional System Debug");
	pConsole->Register("ai_StartupMode", &StartupMode, "default", VF_CHEAT | VF_DUMPTODISK, "Sets startup mode for AI system (supported values: \"default\"/\"GDC09\"). Important: case-sensitive!!!");

	pConsole->Register("ai_lnmDrawAgentType", &DrawAgentType, "all", VF_CHEAT|VF_DUMPTODISK,
		"Determines which agent type nav mesh gets debug-drawn");

	pConsole->Register("ai_TacticalPointUpdateTime", &TacticalPointUpdateTime, 0.0005f, 0,
		"Maximum allowed update time in main AI thread for Tactical Point System\n"
		"Usage: ai_TacticalPointUpdateTime <number>\n"
		"Default is 0.0003");

	pConsole->Register("ai_UpdateAllAlways", &UpdateAllAlways, 0, VF_CHEAT,
		"If non-zero then over-rides the auto-disabling of invisible/distant AI");
	pConsole->Register("ai_UseCalculationStopperCounter", &UseCalculationStopperCounter, 0, VF_CHEAT,
		"Uses a (calibrated) counter instead of time in AI updates");

	// is not cheat protected because it changes during game, depending on your settings
	pConsole->Register("ai_UpdateInterval", &AIUpdateInterval, 0.13f, 0,
		"In seconds the amount of time between two full updates for AI  \n"
		"Usage: ai_UpdateInterval <number>\n"
		"Default is 0.1. Number is time in seconds");
	pConsole->Register("ai_DynamicWaypointUpdateTime", &DynamicWaypointUpdateTime, 0.00025f, 0,
		"How long (max) to spend updating dynamic waypoint regions per AI update (in sec)\n"
		"0 disables dynamic updates. 0.0005 is a sensible value");
	pConsole->Register("ai_DynamicVolumeUpdateTime", &DynamicVolumeUpdateTime, 0.005f, 0,
		"How long (max) to spend updating dynamic volume regions per AI update (in sec)\n"
		"0 disables dynamic updates. 0.002 is a sensible value");
	pConsole->Register("ai_SmartObjectUpdateTime", &SmartObjectUpdateTime, 0.002f, 0,
		"How long (max) to spend updating smart objects per AI update (in sec)\n"
		"default value is 0.002");
	pConsole->Register("ai_AdjustPathsAroundDynamicObstacles", &AdjustPathsAroundDynamicObstacles, 1, VF_CHEAT,
		"Set to 1/0 to enable/disable AI path adjustment around dynamic obstacles");
	// is not cheat protected because it changes during game, depending on your settings
	pConsole->Register("ai_MaxVisRaysPerFrame", &AIVisRaysPerFrame, 30, 0,
		"Maximum allowed visibility rays per frame - the rest are all assumed to succeed \n"
		"Usage: ai_MaxVisRaysPerFrame <number>\n"
		"Default is 100");
	pConsole->Register("ai_AllowAccuracyIncrease", &TargetMovingAccuracyMultiplier, 0, VF_SAVEGAME,
		"Set to 1 to enable AI accuracy increase when target is standing still.");
	pConsole->Register("ai_AllowAccuracyDecrease", &LateralMovementAccuracyDecay, 1, VF_SAVEGAME,
		"Set to 1 to enable AI accuracy decrease when target is moving lateraly.");
	pConsole->Register("ai_SOMSpeedRelaxed", &SOMSpeedRelaxed, 0.4f, VF_SAVEGAME,
		"Time before the AI will see the enemy while relaxed.\n"
		"Usage: ai_SOMSpeedRelaxed 0.4\n"
		"Default is 0.4. A lower value causes the AI to react to the enemy faster.");
	pConsole->Register("ai_SOMSpeedCombat", &SOMSpeedCombat, 0.15f, VF_SAVEGAME,
		"Time before the AI will see the enemy while alarmed.\n"
		"Usage: ai_SOMSpeedCombat 0.15\n"
		"Default is 0.15. A lower value causes the AI to react to the enemy faster.");
	pConsole->Register("ai_SOMMinimumRelaxed", &SOMMinimumRelaxed, 2.0f, VF_SAVEGAME,
		"Minimum time that must pass before AI will see the enemy while relaxed, regardless of distance.\n"
		"Usage: ai_SOMMinimumRelaxed 2.0\n"
		"Default is 2.0 seconds. A lower value causes the AI to potentially react to the enemy sooner.");
	pConsole->Register("ai_SOMMinimumCombat", &SOMMinimumCombat, 0.5f, VF_SAVEGAME,
		"Minimum time that must pass before AI will see the enemy while alarmed, regardless of distance.\n"
		"Usage: ai_SOMMinimumCombat 0.5\n"
		"Default is 0.5 seconds. A lower value causes the AI to potentially react to the enemy sooner.");
	pConsole->Register("ai_SightRangeDarkIllumMod", &SightRangeDarkIllumMod, 0.5f, VF_SAVEGAME,
		"Multiplier for sightrange when the target is in dark light condition.");
	pConsole->Register("ai_SightRangeMediumIllumMod", &SightRangeMediumIllumMod, 0.8f, VF_SAVEGAME,
		"Multiplier for sightrange when the target is in medium light condition.");
	pConsole->Register("ai_MovementSpeedDarkIllumMod", &MovementSpeedDarkIllumMod, 0.6f, VF_SAVEGAME,
		"Multiplier for movement speed when the target is in dark light condition.");
	pConsole->Register("ai_MovementSpeedMediumIllumMod", &MovementSpeedMediumIllumMod, 0.8f, VF_SAVEGAME,
		"Multiplier for movement speed when the target is in medium light condition.");
	pConsole->Register("ai_PathfindTimeLimit", &AllowedTimeForPathfinding, 0.05f, 0,
		"Specifies how many seconds an individual AI can hold the pathfinder blocked\n"
		"Usage: ai_PathfindTimeLimit 0.15\n"
		"Default is 0.05. A lower value will result in more path requests that end in NOPATH -\n"
		"although the path may actually exist.");	
	pConsole->Register("ai_PathfinderUpdateTime", &PathfinderUpdateTime, 0.0005f, 0,
		"Maximum pathfinder time per AI update");
	pConsole->Register("ai_DrawAgentFOV", &DrawAgentFOV, 0, VF_CHEAT,
		"Toggles the vision cone of the AI agent.\n"
		"Usage: ai_DrawagentFOV [0..1]\n"
		"Default is 0 (off), value 1 will draw the cone all the way to\n"
		"the sight range, value 0.1 will draw the cone to distance of 10%\n"
		"of the sight range, etc. ai_DebugDraw must be enabled before\n"
		"this tool can be used.");
	pConsole->Register("ai_AgentStatsDist", &AgentStatsDist, 150, VF_CHEAT|VF_DUMPTODISK,
		"Sets agent statistics draw distance, such as current goalpipe, command and target.\n"
		"Only information on enabled AI objects will be displayed.\n"
		"To display more information on particular AI agent, use ai_StatsTarget.\n"
		"Yellow line represents direction where AI is trying to move;\n"
		"Red line reprecents direction where AI is trying to look;\n"
		"Blue line represents forward dir (entity forward);\n"
		"Usage: ai_AgentStatsDist [view distance]\n"
		"Default is 20 meters. Works with ai_DebugDraw enabled.");
	pConsole->Register("ai_DebugDraw", &DebugDraw, -1, VF_CHEAT|VF_DUMPTODISK,
		"Toggles the AI debugging view.\n"
		"Usage: ai_DebugDraw [-1/0/1]\n"
		"Default is 0 (minimal), value -1 will draw nothing, value 1 displays AI rays and targets \n"
		"and enables the view for other AI debugging tools.");
	pConsole->Register("ai_NetworkDebug", &NetworkDebug, 0, VF_CHEAT | VF_DUMPTODISK,
		"Toggles the AI network debug.\n"
		"Usage: ai_NetworkDebug [0/1]\n"
		"Default is 0 (off). ai_NetworkDebug is used to direct DebugDraw information \n"
		"from the server to the client.");
	pConsole->Register("ai_DebugDrawHideSpotSearchRays", &DrawHideSpotSearchRays, 0, VF_CHEAT|VF_DUMPTODISK,
		"Toggle drawing rays used in HM_ONLY_IF_CAN_SHOOT_FROM_THERE hide spot search option.\n"
		"Usage: ai_DebugDrawHideSpotSearchRays [0..1]");
	pConsole->Register("ai_DebugDrawVegetationCollisionDist", &DebugDrawVegetationCollisionDist, 0, VF_CHEAT|VF_DUMPTODISK,
		"Enables drawing vegetation collision closer than a distance projected onto the terrain.");
	pConsole->Register("ai_DebugDrawHidespotRange", &DebugDrawHideSpotRange, 0, VF_CHEAT|VF_DUMPTODISK,
		"Sets the range for drawing hidespots around the player (needs ai_DebugDraw > 0).");
	pConsole->Register("ai_DebugDrawDynamicHideObjectsRange", &DebugDrawDynamicHideObjectsRange, 0, VF_CHEAT|VF_DUMPTODISK,
		"Sets the range for drawing dynamic hide objects around the player (needs ai_DebugDraw > 0).");
	pConsole->Register("ai_DebugDrawDeadBodies", &DebugDrawDeadBodies, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws the location of dead bodies to avoid during pathfinding (needs ai_DebugDraw > 0).");
	pConsole->Register("ai_DebugDrawVolumeVoxels", &DebugDrawVolumeVoxels, 0, VF_CHEAT|VF_DUMPTODISK,
		"Toggles the AI debugging drawing of voxels in volume generation.\n"
		"Usage: ai_DebugDrawVolumeVoxels [0, 1, 2 etc]\n"
		"Default is 0 (off)\n"
		"+n draws all voxels with original value >= n\n"
		"-n draws all voxels with original value =  n");
	pConsole->Register("ai_DebugPathfinding", &DebugPathFinding, 0, 0,
		"Toggles output of pathfinding information [default 0 is off]");
	pConsole->Register("ai_DebugDrawBannedNavsos", &DebugDrawBannedNavsos, 0, 0,
		"Toggles drawing banned navsos [default 0 is off]");
	pConsole->Register("ai_DebugDrawGroups", &DebugDrawGroups, 0, VF_CHEAT|VF_DUMPTODISK,
		"Toggles the AI Groups debugging view.\n"
		"Usage: ai_DebugDrawGroups [0/1]\n"
		"Default is 0 (off). ai_DebugDrawGroups displays groups' leaders, members and their desired positions.");
	pConsole->Register("ai_DebugDrawCover", &DebugDrawCover, 0, VF_CHEAT|VF_DUMPTODISK,
		"Displays cover debug information.\n"
		"Usage: ai_DebugDrawCover [0/1/2]\n"
		"Default is 0 (off)\n"
		"0 - off\n"
		"1 - currently being used\n"
		"2 - all in 50m range (slow)\n");
	pConsole->Register("ai_DebugDrawCoordination", &DebugDrawCoordination, 0, VF_CHEAT|VF_DUMPTODISK,
		"Displays coordination debug information.\n"
		"Usage: ai_DebugDrawCoordination [0/1/2]\n"
		"Default is 0 (off). 1 is in-game graphics. 2 is detailed text.");
	pConsole->Register("ai_DebugDrawCommunication", &DebugDrawCommunication, 0, VF_CHEAT|VF_DUMPTODISK,
		"Displays communication debug information.\n"
		"Usage: ai_DebugDrawCommunication [0/1]\n"
		"Default is 0 (off).");
	pConsole->Register("ai_DrawNodeLinkType", &DrawNodeLinkType, 0, VF_CHEAT,
		"Sets the link parameter to draw with ai_DrawNode.\n"
		"Values are:\n"
		" 0 - pass radius (default)\n"
		" 1 - exposure\n"
		" 2 - water max depth\n"
		" 3 - water min depth");
	pConsole->Register("ai_DrawNodeLinkCutoff", &DrawNodeLinkCutoff, 0.0f, VF_CHEAT,
		"Sets the link cutoff value in ai_DrawNodeLinkType. If the link value is more than\n"
		"ai_DrawNodeLinkCutoff the number gets displayed in green, otherwise red.");
	pConsole->Register("ai_SystemUpdate", &AiSystem, 1, VF_CHEAT,
		"Toggles the regular AI system update.\n"
		"Usage: ai_SystemUpdate [0/1]\n"
		"Default is 1 (on). Set to 0 to disable ai system updating.");
	pConsole->Register("ai_SoundPerception", &SoundPerception, 1, VF_CHEAT,
		"Toggles AI sound perception.\n"
		"Usage: ai_SoundPerception [0/1]\n"
		"Default is 1 (on). Used to prevent AI from hearing sounds for\n"
		"debugging purposes. Works with ai_DebugDraw enabled.");
	pConsole->Register("ai_IgnorePlayer", &IgnorePlayer, 0, VF_CHEAT,
		"Makes AI ignore the player.\n"
		"Usage: ai_IgnorePlayer [0/1]\n"
		"Default is 0 (off). Set to 1 to make AI ignore the player.\n"
		"Used with ai_DebugDraw enabled.");
	pConsole->Register("ai_IgnoreVisibilityChecks", &IgnoreVisibilityChecks, 0, VF_CHEAT,
		"Makes certain visibility checks (for teleporting etc) return false.");
	pConsole->Register("ai_DrawFormations", &DrawFormations, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws all the currently active formations of the AI agents.\n"
		"Usage: ai_DrawFormations [0/1]\n"
		"Default is 0 (off). Set to 1 to draw the AI formations.");
	pConsole->Register("ai_DrawPatterns", &DrawPatterns, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws all the currently active track patterns of the AI agents.\n"
		"Usage: ai_DrawPatterns [0/1]\n"
		"Default is 0 (off). Set to 1 to draw the AI track patterns.");
	pConsole->Register("ai_DrawSmartObjects", &DrawSmartObjects, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws smart object debug information.\n"
		"Usage: ai_DrawSmartObjects [0/1]\n"
		"Default is 0 (off). Set to 1 to draw the smart objects.");
	pConsole->Register("ai_DrawReadibilities", &DrawReadibilities, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws all the currently active readibilities of the AI agents.\n"
		"Usage: ai_DrawReadibilities [0/1]\n"
		"Default is 0 (off). Set to 1 to draw the AI readibilities.");
	pConsole->Register("ai_DrawGoals", &DrawGoals, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws all the active goal ops debug info.\n"
		"Usage: ai_DrawGoals [0/1]\n"
		"Default is 0 (off). Set to 1 to draw the AI goal op debug info.");
	pConsole->Register("ai_AllTime", &AllTime, 0, VF_CHEAT,
		"Displays the update times of all agents, in milliseconds.\n"
		"Usage: ai_AllTime [0/1]\n"
		"Default is 0 (off). Times all agents and displays the time used updating\n"
		"each of them. The name is colour coded to represent the update time.\n"
		"	Green: less than 1 ms (ok)\n"
		"	White: 1 ms to 5 ms\n"
		"	Red: more than 5 ms\n"
		"You must enable ai_DebugDraw before you can use this tool.");
	pConsole->Register("ai_ProfileGoals", &ProfileGoals, 0, VF_CHEAT,
		"Toggles timing of AI goal execution.\n"
		"Usage: ai_ProfileGoals [0/1]\n"
		"Default is 0 (off). Records the time used for each AI goal (like\n"
		"approach, run or pathfind) to execute. The longest execution time\n"
		"is displayed on screen. Used with ai_DebugDraw enabled.");
	pConsole->Register("ai_BeautifyPath", &BeautifyPath, 1, VF_CHEAT,
		"Toggles AI optimisation of the generated path.\n"
		"Usage: ai_BeautifyPath [0/1]\n"
		"Default is 1 (on). Optimisation is on by default. Set to 0 to\n"
		"disable path optimisation (AI uses non-optimised path).");
	pConsole->Register("ai_AttemptStraightPath", &AttemptStraightPath, 1, VF_CHEAT,
		"Toggles AI attempting a simple straight path when possible.\n"
		"Default is 1 (on).");
	pConsole->Register("ai_CrowdControlInPathfind", &CrowdControlInPathfind, 1, VF_CHEAT,
		"Toggles AI using crowd control in pathfinding.\n"
		"Usage: ai_CrowdControlInPathfind [0/1]\n"
		"Default is 1 (on).");
	pConsole->Register("ai_DebugDrawCrowdControl", &DebugDrawCrowdControl, 0, VF_CHEAT,
		"Draws crowd control debug information. 0=off, 1=on");
	pConsole->Register("ai_PredictivePathFollowing", &PredictivePathFollowing, 1, VF_CHEAT,
		"Sets if AI should use the predictive path following if allowed by the type's config.");
	pConsole->Register("ai_SteepSlopeUpValue", &SteepSlopeUpValue, 1.0f, VF_CHEAT,
		"Indicates slope value that is borderline-walkable up.\n"
		"Usage: ai_SteepSlopeUpValue 0.5\n"
		"Default is 1.0 Zero means flat. Infinity means vertical. Set it smaller than ai_SteepSlopeAcrossValue");
	pConsole->Register("ai_SteepSlopeAcrossValue", &SteepSlopeAcrossValue, 0.6f, VF_CHEAT,
		"Indicates slope value that is borderline-walkable across.\n"
		"Usage: ai_SteepSlopeAcrossValue 0.8\n"
		"Default is 0.6 Zero means flat. Infinity means vertical. Set it greater than ai_SteepSlopeUpValue");
	pConsole->Register("ai_UpdateProxy", &UpdateProxy, 1, VF_CHEAT,
		"Toggles update of AI proxy (model).\n"
		"Usage: ai_UpdateProxy [0/1]\n"
		"Default is 1 (on). Updates proxy (AI representation in game)\n"
		"set to 0 to disable proxy updating.");
	pConsole->Register("ai_DrawType", &DrawType, -1, VF_CHEAT|VF_DUMPTODISK,
		"Display all AI object of specified type. If object is enabled it will be displayed.\n"
		"with blue ball, otherwice with red ball. Yellow line will reprecent forward direction of the object.\n"
		" <0 - off\n"
		" 0 - display all the dummy objects\n"
		" >0 - type of AI objects to display");
	pConsole->Register("ai_DrawModifiers", &DrawModifiers, 0, VF_CHEAT,
		"Toggles the AI debugging view of navigation modifiers.");
	pConsole->Register("ai_DrawOffset", &DebugDrawOffset, 0.1f, VF_CHEAT,
		"vertical offset during debug drawing (graph nodes, navigation paths, ...)");
	pConsole->Register("ai_DrawTargets", &DrawTargets, 0, VF_CHEAT|VF_DUMPTODISK, 
		"Distance to display the perception events of all enabled puppets.\n"
		"Displays target type and priority");
	pConsole->Register("ai_DrawBadAnchors", &DrawBadAnchors, -1, VF_CHEAT|VF_DUMPTODISK,
		"Toggles drawing out of bounds AI objects of particular type for debugging AI.\n"
		"Valid only for 3D navigation. Draws red spheres at positions of anchors which are\n"
		"located out of navigation volumes. Those anchors have to be moved.\n"
		" 0 - off, 1 - on");
	pConsole->Register("ai_DrawStats", &DrawStats, 00, VF_CHEAT|VF_DUMPTODISK,
		"Toggles drawing stats (in a table on top left of screen) for AI objects within specified range.\n"
		"Will display attention target, goal pipe and current goal.");
	pConsole->Register("ai_DrawAttentionTargetPositions", &DrawAttentionTargetsPosition, 0, VF_CHEAT|VF_DUMPTODISK,
		"Displays markers for the AI's current attention target position. " );
	pConsole->Register("ai_ExtraRadiusDuringBeautification", &ExtraRadiusDuringBeautification, 0.2f, VF_CHEAT,
		"Extra radius added to agents during beautification.");
	pConsole->Register("ai_RadiusForAutoForbidden", &RadiusForAutoForbidden, 1.0f, VF_CHEAT, 
		"If object/vegetation radius is more than this then an automatic forbidden area is created during triangulation.");
	pConsole->Register("ai_ExtraForbiddenRadiusDuringBeautification", &ExtraForbiddenRadiusDuringBeautification, 1.0f, VF_CHEAT,
		"Extra radius added to agents close to forbidden edges during beautification.");
	pConsole->Register("ai_RecordLog", &RecordLog, 0, VF_CHEAT|VF_DUMPTODISK,
		"log all the AI state changes on stats_target");
	pConsole->Register("ai_DrawGroup", &DrawGroups, -2, VF_CHEAT|VF_DUMPTODISK,
		"draw groups:\n"
		" >0 - group ID to draw\n"
		" -1 - all groups\n"
		" -2 - nothing");
	pConsole->Register("ai_DrawGroupTactic", &DrawGroupTactic, 0, VF_CHEAT|VF_DUMPTODISK,
		"draw group tactic: 0 = disabled, 1 = draw simple, 2 = draw complex.");
	pConsole->Register("ai_DrawTrajectory", &DrawTrajectory, 0, VF_CHEAT|VF_DUMPTODISK,
		"Record and draw the actual path taken by the agent specified in ai_StatsTarget.\n"
		"Path is displayed in aqua, and only a certain length will be displayed, after which\n"
		"old path gradually disappears as new path is drawn.\n"
		"0=do not record, 1=record.");
	pConsole->Register("ai_DrawHidespots", &DrawHideSpots, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws latest hide-spot positions for all agents within specified range.");
	pConsole->Register("ai_DrawRadar", &DrawRadar, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws AI radar: 0=no radar, >0 = size of the radar on screen");
	pConsole->Register("ai_DrawRadarDist", &DrawRadarDist, 20, VF_CHEAT|VF_DUMPTODISK,
		"AI radar draw distance in meters, default=20m.");
	// should be off for release
	pConsole->Register("ai_Recorder_Auto", &DebugRecordAuto, 0, VF_CHEAT|VF_DUMPTODISK,
		"Auto record the AI when in Editor mode game\n");
	pConsole->Register("ai_Recorder_Buffer", &DebugRecordBuffer, 1024, VF_CHEAT|VF_DUMPTODISK,
		"Set the size of the AI debug recording buffer");
	pConsole->Register("ai_DrawDistanceLUT", &DrawDistanceLUT, 0, VF_CHEAT,
		"Draws the distance lookup table graph overlay.");
	pConsole->Register("ai_PuppetDirSpeedControl", &PuppetDirSpeedControl, 1, 0,
		"Does puppet speed control based on their current move dir");
	pConsole->Register("ai_DrawAreas", &DrawAreas, 0, VF_CHEAT|VF_DUMPTODISK,
		"Enables/Disables drawing behavior related areas.");
	pConsole->Register("ai_warn_signal_duration", &WarnSignalDuration, 40, 0,
		"Log warning if a signal handler takes longer than x millis");
	pConsole->Register("ai_IgnoreDeadBodies", &IgnoreDeadBodies, 0, 0,
		"Ignore dead bodies during navigation and hidespot finding.");
	pConsole->Register("ai_BurstWhileMovingDestinationRange", &BurstWhileMovingDestinationRange, 2.0f, 0,
		"When using FIREMODE_BURST_WHILE_MOVING - only fire when within this distance to the destination.");
	pConsole->Register("ai_DrawProbableTarget", &DrawProbableTarget, 0, VF_CHEAT|VF_DUMPTODISK,
		"Enables/Disables drawing the position of probable target.");
	pConsole->Register("ai_IncludeNonColEntitiesInNavigation", &IncludeNonColEntitiesInNavigation, 1, 0,
		"Includes/Excludes noncolliding objects from navigation.");
	pConsole->Register("ai_DebugDrawDamageParts", &DebugDrawDamageParts, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws the damage parts of puppets and vehicles.");
	pConsole->Register("ai_DebugDrawStanceSize", &DebugDrawStanceSize, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws the game logic representation of the stance size of the AI agents.");

	pConsole->Register("ai_DebugTargetSilhouette", &DebugTargetSilhouette, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws the silhouette used for missing the target while shooting.");

	pConsole->Register("ai_RODAliveTime", &RODAliveTime, 3.0f, VF_CHEAT|VF_DUMPTODISK,
		"The base level time the player can survive under fire.");
	pConsole->Register("ai_RODMoveInc", &RODMoveInc, 3.0f, VF_CHEAT|VF_DUMPTODISK,
		"Increment how the speed of the target affects the alive time (the value is doubled for supersprint). 0=disable");
	pConsole->Register("ai_RODStanceInc", &RODStanceInc, 2.0f, VF_CHEAT|VF_DUMPTODISK,
		"Increment how the stance of the target affects the alive time, 0=disable.\n"
		"The base value is for crouch, and it is doubled for prone.\n"
		"The crouch inc is disable in kill-zone and prone in kill and combat-near -zones");
	pConsole->Register("ai_RODDirInc", &RODDirInc, 0.0f, VF_CHEAT|VF_DUMPTODISK,
		"Increment how the orientation of the target affects the alive time. 0=disable");
	pConsole->Register("ai_RODKillZoneInc", &RODKillZoneInc, -4.0f, VF_CHEAT|VF_DUMPTODISK,
		"Increment how the target is within the kill-zone of the target.");
	pConsole->Register("ai_RODAmbientFireInc", &RODAmbientFireInc, 3.0f, VF_CHEAT|VF_DUMPTODISK,
		"Increment for the alive time when the target is within the kill-zone of the target.");

	pConsole->Register("ai_RODKillRangeMod", &RODKillRangeMod, 0.15f, VF_CHEAT|VF_DUMPTODISK,
		"Kill-zone distance = attackRange * killRangeMod.");
	pConsole->Register("ai_RODCombatRangeMod", &RODCombatRangeMod, 0.55f, VF_CHEAT|VF_DUMPTODISK,
		"Combat-zone distance = attackRange * combatRangeMod.");

	pConsole->Register("ai_RODCoverFireTimeMod", &RODCoverFireTimeMod, 1.0f, VF_CHEAT|VF_DUMPTODISK,
		"Multiplier for cover fire times set in weapon descriptor.");

	pConsole->Register("ai_RODReactionTime", &RODReactionTime, 1.0f, VF_CHEAT|VF_DUMPTODISK,
		"Uses rate of death as damage control method.");
	pConsole->Register("ai_RODReactionDistInc", &RODReactionDistInc, 0.1f, VF_CHEAT|VF_DUMPTODISK,
		"Increase for the reaction time when the target is in combat-far-zone or warn-zone.\n"
		"In warn-zone the increase is doubled.");
	pConsole->Register("ai_RODReactionDirInc", &RODReactionDirInc, 2.0f, VF_CHEAT|VF_DUMPTODISK,
		"Increase for the reaction time when the enemy is outside the players FOV or near the edge of the FOV.\n"
		"The increment is doubled when the target is behind the player.");
	pConsole->Register("ai_RODReactionLeanInc", &RODReactionLeanInc, 0.2f, VF_CHEAT|VF_DUMPTODISK,
		"Increase to the reaction to when the target is leaning.");
	pConsole->Register("ai_RODReactionDarkIllumInc", &RODReactionDarkIllumInc, 0.3f, VF_CHEAT|VF_DUMPTODISK,
		"Increase for reaction time when the target is in dark light condition.");
	pConsole->Register("ai_RODReactionMediumIllumInc", &RODReactionMediumIllumInc, 0.2f, VF_CHEAT|VF_DUMPTODISK,
		"Increase for reaction time when the target is in medium light condition.");

	pConsole->Register("ai_RODLowHealthMercyTime", &RODLowHealthMercyTime, 1.5f, VF_CHEAT|VF_DUMPTODISK,
		"The amount of time the AI will not hit the target when the target crosses the low health threshold.");

	pConsole->Register("ai_AmbientFireEnable", &AmbientFireEnable, 1, VF_CHEAT|VF_DUMPTODISK,
		"Enable ambient fire system.");
	pConsole->Register("ai_AmbientFireUpdateInterval", &AmbientFireUpdateInterval, 1.0f, VF_CHEAT|VF_DUMPTODISK,
		"Ambient fire update interval. Controls how often puppet's ambient fire status is updated.");
	pConsole->Register("ai_AmbientFireQuota", &AmbientFireQuota, 2, VF_CHEAT|VF_DUMPTODISK,
		"Number of units allowed to hit the player at a time.");
	pConsole->Register("ai_DebugDrawAmbientFire", &DebugDrawAmbientFire, 0, VF_CHEAT|VF_DUMPTODISK,
		"Displays fire quota on puppets.");

	pConsole->Register("ai_DebugDrawExpensiveAccessoryQuota", &DebugDrawExpensiveAccessoryQuota, 0, VF_CHEAT|VF_DUMPTODISK,
		"Displays expensive accessory usage quota on puppets.");

	pConsole->Register("ai_DebugDrawDamageControl", &DebugDrawDamageControl, 0, VF_CHEAT|VF_DUMPTODISK,
		"Debugs the damage control system 0=disabled, 1=collect, 2=collect&draw.");

	pConsole->Register("ai_DrawFakeTracers", &DrawFakeTracers, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws fake tracers around the player.");
	pConsole->Register("ai_DrawFakeHitEffects", &DrawFakeHitEffects, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws fake hit effects the player.");
	pConsole->Register("ai_DrawFakeDamageInd", &DrawFakeDamageInd, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws fake damage indicators on the player.");
	pConsole->Register("ai_DrawPlayerRanges", &DrawPlayerRanges, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws rings around player to assist in gauging target distance");
	pConsole->Register("ai_DrawPerceptionIndicators", &DrawPerceptionIndicators, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws indicators showing enemy current perception level of player");
	pConsole->Register("ai_DrawPerceptionDebugging", &DrawPerceptionDebugging, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws indicators showing enemy view intersection with perception modifiers");
	pConsole->Register("ai_DrawPerceptionModifiers", &DrawPerceptionModifiers, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws perception modifier areas in game mode");
	pConsole->Register("ai_DebugPerceptionManager", &DebugPerceptionManager, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws perception manager performance overlay. 0=disable, 1=vis checks, 2=stimulus");

	pConsole->Register("ai_TargetTracking", &TargetTracking, 1, VF_CHEAT|VF_DUMPTODISK,
		"Set which method to use for target tracking. 0=disable, 1=Perception Handlers, 2=Target Tracks");
	pConsole->Register("ai_TargetTracks_DeadForgetTime", &TargetTracks_DeadForgetTime, 5.0f, VF_CHEAT|VF_DUMPTODISK,
		"At most the length of time it takes for an agent to forget about a target that has been killed");
	pConsole->Register("ai_TargetTracks_GlobalTargetLimit", &TargetTracks_GlobalTargetLimit, 0, VF_CHEAT|VF_DUMPTODISK,
		"Global override to control the number of agents that can actively target another agent (unless there is no other choice)\n"
		"A value of 0 means no global limit is applied. If the global target limit is less than the agent's target limit, the global limit is used.");
	pConsole->Register("ai_DebugTargetTracksTarget", &TargetTracks_TargetDebugDraw, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws lines to illustrate where each agent's target is\n"
		"Usage: ai_DebugTargetTracking 0/1/2\n"
		"0 = Off. 1 = Show best target. 2 = Show all possible targets.");
	pConsole->Register("ai_DebugTargetTracksAgent", &TargetTracks_AgentDebugDraw, "none", VF_CHEAT|VF_DUMPTODISK,
		"Draws the target tracks for the given agent\n"
		"Usage: ai_DebugTargetTracksAgent AIName\n"
		"Default is 'none'. AIName is the name of the AI agent to debug");
	pConsole->Register("ai_DebugTargetTracksConfig", &TargetTracks_ConfigDebugDraw, 0, VF_CHEAT|VF_DUMPTODISK,
		"Draws the information contained in the loaded target track configurations to the screen");
	pConsole->Register("ai_DebugTargetTracksConfig_Filter", &TargetTracks_ConfigDebugFilter, "none", VF_CHEAT|VF_DUMPTODISK,
		"Filter what configurations are drawn when debugging target tracks\n"
		"Usage: ai_DebugTargetTracks_Filter Filter\n"
		"Default is 'none'. Filter is a substring that must be in the configuration name");

	pConsole->Register("ai_ForceStance", &ForceStance, -1, VF_CHEAT,
		"Forces all AI characters to specified stance:\n"
		"Disable = -1, Stand = 0, Crouch = 1, Prone = 2, Relaxed = 3, Stealth = 4, Cover = 5, Swim = 6, Zero-G = 7");

	pConsole->Register("ai_ForceAllowStrafing", &ForceAllowStrafing, -1, VF_CHEAT,
		"Forces all AI characters to use/not use strafing (-1 disables)");

	pConsole->Register("ai_InterestSystem", &InterestSystem, 0, VF_CHEAT|VF_DUMPTODISK,
		"Enable interest system");
	pConsole->Register("ai_DebugInterestSystem", &DebugInterest, 0, VF_CHEAT|VF_DUMPTODISK,
		"Display debugging information on interest system");

	pConsole->Register("ai_BannedNavSoTime", &BannedNavSoTime, 20.0f, VF_CHEAT|VF_DUMPTODISK,
		"Time indicating how long invalid navsos should be banned.");
	pConsole->Register("ai_DebugDrawAdaptiveUrgency", &DebugDrawAdaptiveUrgency, 0, VF_CHEAT|VF_DUMPTODISK,
		"Enables drawing the adaptive movement urgency.");
	pConsole->Register("ai_DebugDrawReinforcements", &DebugDrawReinforcements, -1, VF_CHEAT|VF_DUMPTODISK,
		"Enables debug draw for reinforcement logic for specified group.\n"
		"Usage: ai_DebugDrawReinforcements <groupid>, or -1 to disable.");
	pConsole->Register("ai_DebugDrawPlayerActions", &DebugDrawPlayerActions, 0, VF_CHEAT|VF_DUMPTODISK,
		"Debug draw special player actions.");

	pConsole->Register("ai_DrawCollisionEvents", &DrawCollisionEvents, 0, VF_CHEAT|VF_DUMPTODISK,
		"Debug draw the collision events the AI system processes. 0=disable, 1=enable.");
	pConsole->Register("ai_DrawBulletEvents", &DrawBulletEvents, 0, VF_CHEAT|VF_DUMPTODISK,
		"Debug draw the bullet events the AI system processes. 0=disable, 1=enable.");
	pConsole->Register("ai_DrawSoundEvents", &DrawSoundEvents, 0, VF_CHEAT|VF_DUMPTODISK,
		"Debug draw the sound events the AI system processes. 0=disable, 1=enable.");
	pConsole->Register("ai_DrawGrenadeEvents", &DrawGrenadeEvents, 0, VF_CHEAT|VF_DUMPTODISK,
		"Debug draw the grenade events the AI system processes. 0=disable, 1=enable.");
	pConsole->Register("ai_DrawExplosions", &DrawExplosions, 0, VF_CHEAT|VF_DUMPTODISK,
		"Debug draw the explosion events the AI system processes. 0=disable, 1=enable.");

	pConsole->Register("ai_EnableWaterOcclusion", &WaterOcclusionEnable, 1, VF_CHEAT|VF_DUMPTODISK,
		"Enables applying water occlusion to AI target visibility checks");
	pConsole->Register("ai_WaterOcclusion", &WaterOcclusionScale, .5f, VF_CHEAT|VF_DUMPTODISK,
		"scales how much water hides player from AI");

	pConsole->Register("ai_CloakMinDist", &CloakMinDist, 1.f, VF_CHEAT|VF_DUMPTODISK,
		"closer than that - cloak not effective");
	pConsole->Register("ai_CloakMaxDist", &CloakMaxDist, 5.f, VF_CHEAT|VF_DUMPTODISK,
		"closer than that - cloak starts to fade out");
	pConsole->Register("ai_CloakIncrementMod", &CloakIncrement, 1.f, VF_CHEAT|VF_DUMPTODISK,
		"how fast cloak fades out");

	pConsole->Register("ai_BigBrushCheckLimitSize", &BigBrushLimitSize, 15.f, VF_CHEAT|VF_DUMPTODISK,
		"to be used for finding big objects not enclosed into forbidden areas");

	pConsole->Register("ai_DebugDrawEnabledPuppets", &DebugDrawEnabledPuppets, 0, VF_CHEAT|VF_DUMPTODISK,
		"list of AI puppets that are enabled and metadata");
	pConsole->Register("ai_DebugDrawEnabledPlayers", &DebugDrawEnabledPlayers, 0, VF_CHEAT|VF_DUMPTODISK,
		"list of AI players that are enabled and metadata");

	pConsole->Register("ai_DrawUpdate", &DebugDrawUpdate, 0, VF_CHEAT|VF_DUMPTODISK,
		"list of AI forceUpdated entities");

	pConsole->Register("ai_DebugDrawLightLevel", &DebugDrawLightLevel, 0, VF_CHEAT|VF_DUMPTODISK,
		"Debug AI light level manager");

	pConsole->Register("ai_DrawVisCheckQueue", &DebugDrawVisChecQueue, 0, VF_CHEAT|VF_DUMPTODISK,
		"list of pending vis-check trace requests");

	pConsole->Register("ai_SimpleWayptPassability", &SimpleWayptPassability, 1, VF_CHEAT|VF_DUMPTODISK,
		"Use simplified and faster passability recalculation for human waypoint links where possible.");

	pConsole->Register("ai_UseAlternativeReadability", &UseAlternativeReadability, 0, VF_SAVEGAME,
		"Switch between normal and alternative SoundPack for AI readability.");

	pConsole->Register("ai_ThreadedVolumeNavPreprocess", &ThreadedVolumeNavPreprocess, 1, VF_CHEAT|VF_DUMPTODISK,
		"Parallelizes volume navigation preprocessing by running it on multiple threads.\n"
		"If you experience freezes during volume nav export or corrupted volume nav data, try turning this off. ;)"
		);
	pConsole->Register("ai_ExtraVehicleAvoidanceRadiusBig", &ExtraVehicleAvoidanceRadiusBig, 4.0f, VF_CHEAT|VF_DUMPTODISK,
		"Value in meters to be added to a big obstacle's own size while computing obstacle\n"
		"size for purposes of vehicle steering. See also ai_ObstacleSizeThreshold.");
	pConsole->Register("ai_ExtraVehicleAvoidanceRadiusSmall", &ExtraVehicleAvoidanceRadiusSmall, 0.5f, VF_CHEAT|VF_DUMPTODISK,
		"Value in meters to be added to a big obstacle's own size while computing obstacle\n"
		"size for purposes of vehicle steering. See also ai_ObstacleSizeThreshold.");
	pConsole->Register("ai_ObstacleSizeThreshold", &ObstacleSizeThreshold, 1.2f, VF_CHEAT|VF_DUMPTODISK,
		"Obstacle size in meters that differentiates small obstacles from big ones so that vehicles can ignore the small ones");
	pConsole->Register("ai_DrawGetEnclosingFailures", &DrawGetEnclosingFailures, 0.0f, VF_CHEAT|VF_DUMPTODISK,
		"Set to the number of seconds you want GetEnclosing() failures visualized.  Set to 0 to turn visualization off.");

	pConsole->Register("ai_TacticsDebug", &DebugTactics, 0, VF_CHEAT | VF_DUMPTODISK, "Tactics attached to behavior tree debug information" );

	pConsole->Register("ai_CodeCoverageMode", &CodeCoverage, 0, VF_CHEAT|VF_DUMPTODISK,
		"Set current mode of Code Coverage system.\n0 = off, 1 = smart, 2 = silent, 3 = force");

	pConsole->Register("ai_EnablePerceptionStanceVisibleRange", &EnablePerceptionStanceVisibleRange, 0, VF_CHEAT|VF_DUMPTODISK, "Turn on use of max perception range for AI based on player's stance");
	pConsole->Register("ai_CrouchVisibleRange", &CrouchVisibleRange, 15.0f, VF_CHEAT|VF_DUMPTODISK, "Max perception range for AI when player is crouching");
	pConsole->Register("ai_ProneVisibleRange", &ProneVisibleRange, 6.0f, VF_CHEAT|VF_DUMPTODISK, "Max perception range for AI when player is proning");
	pConsole->Register("ai_IgnoreVisualStimulus", &IgnoreVisualStimulus, 0, VF_CHEAT|VF_DUMPTODISK, "Have the Perception Handler ignore all visual stimulus always");
	pConsole->Register("ai_IgnoreSoundStimulus", &IgnoreSoundStimulus, 0, VF_CHEAT|VF_DUMPTODISK, "Have the Perception Handler ignore all sound stimulus always");
	pConsole->Register("ai_IgnoreBulletRainStimulus", &IgnoreBulletRainStimulus, 0, VF_CHEAT|VF_DUMPTODISK, "Have the Perception Handler ignore all bullet rain stimulus always");

	pConsole->Register("ai_lnmDebugDraw", &lnmDebugDraw, 0, VF_CHEAT|VF_DUMPTODISK, "Draw the Layered Navigation Mesh");
	pConsole->Register("ai_lnmDynamics", &lnmDynamics, 0, VF_CHEAT|VF_DUMPTODISK, "Turn on Layered Navigation Mesh dynamic obstacle handling");
	pConsole->Register("ai_lnmGetEnclosingExtraRange", &lnmGetEnclosingExtraRange, 1.0f, VF_CHEAT|VF_DUMPTODISK,
		"If LNM GetEnclosing() fails to find a node, it will try to get the nearest node within this range. "
		"ATTENTION: if client code needs to rely on this it has to be fixed to provide the range explicitly.");

	pConsole->Register("ai_UseSmartPathFollower", &UseSmartPathFollower, 1, VF_CHEAT|VF_DUMPTODISK, "Enables Smart PathFollower (default: 1).");
	pConsole->Register("ai_DrawPathFollower", &DrawPathFollower, 0, VF_CHEAT|VF_DUMPTODISK, "Enables PathFollower debug drawing displaying agent paths and safe follow target.");

	pConsole->Register("ai_LogConsoleVerbosity", &LogConsoleVerbosity, gAIEnv.configuration.IsDevMode ? AI_LOG_PROGRESS : AI_LOG_OFF, VF_DUMPTODISK, "None = 0, progress/errors/warnings = 1, event = 2, comment = 3");
	pConsole->Register("ai_LogFileVerbosity", &LogFileVerbosity, gAIEnv.configuration.IsDevMode ? AI_LOG_EVENT : AI_LOG_PROGRESS, VF_DUMPTODISK, "None = 0, progress/errors/warnings = 1, event = 2, comment = 3");
	pConsole->Register("ai_EnableWarningsErrors", &EnableWarningsErrors, 1, VF_DUMPTODISK, "Enable AI warnings and errors: 1 or 0");
	pConsole->Register("ai_OverlayMessageDuration", &OverlayMessageDuration, 5.0f, VF_DUMPTODISK, "How long (seconds) to overlay AI warnings/errors");
	pConsole->Register("ai_ShowBehaviorCalls", &ShowBehaviorCalls, 0, VF_DUMPTODISK, "Prints out each behavior LUA call for each puppet: 1 or 0");

	pConsole->Register("ai_Recorder", &Recorder, 0, VF_DUMPTODISK, "Sets AI Recorder mode. Default is 0 - off.");
	pConsole->Register("ai_StatsDisplayMode", &StatsDisplayMode, 0, VF_CHEAT|VF_DUMPTODISK,
		"Select display mode for the AI stats manager\n"
		"Usage: 0 - Hidden, 1 - Show\n");

	pConsole->Register("ai_DebugDrawVisionMap", &DebugDrawVisionMap, 0, VF_CHEAT | VF_DUMPTODISK,
		"Sets AI VisionMap Debug Draw Mode. Default is 0 - off."
		"0 - off\n"
		"1 - performance only\n"
		"2 - everything\n");

	pConsole->Register("ai_DrawFireEffectEnabled", &DrawFireEffectEnabled, 1, VF_CHEAT | VF_DUMPTODISK,
		"Enabled AI to sweep fire when starting to shoot after a break.");

	pConsole->Register("ai_DrawFireEffectDecayRange", &DrawFireEffectDecayRange, 35.0f, VF_CHEAT | VF_DUMPTODISK,
		"Distance under which the draw fire duration starts decaying linearly.");

	pConsole->Register("ai_DrawFireEffectMinDistance", &DrawFireEffectMinDistance, 12.5f, VF_CHEAT | VF_DUMPTODISK,
		"Distance under which the draw fire will be disabled.");

	pConsole->Register("ai_DrawFireEffectMinTargetFOV", &DrawFireEffectMinTargetFOV, 35.0f, VF_CHEAT | VF_DUMPTODISK,
		"FOV under which the draw fire will be disabled.");

	pConsole->Register("ai_DrawFireEffectMaxAngle", &DrawFireEffectMaxAngle, 4.0f, VF_CHEAT | VF_DUMPTODISK,
		"Maximum angle actors actors are allowed to go away from their aiming direction during draw fire.");

	pConsole->Register("ai_AllowedToHitPlayer", &AllowedToHitPlayer, 1, VF_CHEAT | VF_DUMPTODISK,
		"If turned off, all agents will miss the player all the time.");

	pConsole->Register("ai_EnableCoolMisses", &EnableCoolMisses, 1, VF_CHEAT | VF_DUMPTODISK,
		"If turned on, when agents miss the player, they will pick cool objects to shoot at.");

	pConsole->Register("ai_CoolMissesBoxSize", &CoolMissesBoxSize, 15.0f, VF_CHEAT | VF_DUMPTODISK,
		"Horizontal size of the box to collect potential cool objects to shoot at.");

	pConsole->Register("ai_CoolMissesBoxHeight", &CoolMissesBoxHeight, 3.0f, VF_CHEAT | VF_DUMPTODISK,
		"Vertical size of the box to collect potential cool objects to shoot at.");

	pConsole->Register("ai_CoolMissesMaxMissDistance", &CoolMissesMaxMissDistance, 2.0f, VF_CHEAT | VF_DUMPTODISK,
		"Maximum distance to go away from the player.");

	pConsole->Register("ai_WIPCode", &WIPCode, 0, VF_CHEAT | VF_DUMPTODISK,
		"Work in progress code. Used to unlock features still under development.");

	pConsole->Register("ai_DynamicHidespotsEnabled", &DynamicHidespotsEnabled, 0, VF_CHEAT | VF_DUMPTODISK,
		"If enabled, dynamic hidespots are considered when evaluating.");
}
