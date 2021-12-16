#ifndef AICONSOLEVARIABLES_H
#define AICONSOLEVARIABLES_H

struct AIConsoleVars
{
	AIConsoleVars()
	{
	}

	void Init();

	float TacticalPointUpdateTime;
	const char* CompatibilityMode;
	int UseCalculationStopperCounter;
	float AllowedTimeForPathfinding;
	float PathfinderUpdateTime;
	float DrawAgentFOV;
	float AgentStatsDist;
	int AiSystem;
	int DebugDraw;
	int NetworkDebug;
	int DebugDrawHideSpotRange;
	int DebugDrawDynamicHideObjectsRange;
	int DebugDrawVolumeVoxels;
	int DebugDrawDeadBodies;
	int DebugPathFinding;
	const char* DebugDrawAStarOpenList;
	int DebugDrawBannedNavsos;
	int DebugDrawGroups;
	int DebugDrawCover;
	int DebugDrawCoordination;
	int DebugDrawCommunication;
	int SoundPerception;
	int IgnorePlayer;
	int IgnoreVisibilityChecks;
	const char* DrawPath;
	const char* DrawPathAdjustment;
	int DrawModifiers;
	int AllTime;
	int ProfileGoals;
	int TargetMovingAccuracyMultiplier;
	int LateralMovementAccuracyDecay;
	int BeautifyPath;
	int AttemptStraightPath;
	int PredictivePathFollowing;
	int CrowdControlInPathfind;
	int DebugDrawCrowdControl;
	int UpdateProxy;
	int DrawType;
	const char* StatsTarget;
	const char* BehaviorStatsTarget;
	const char* DebugBehaviorSelection;
	const char* DebugDrawCoopReadabilities;
	float AIUpdateInterval;
	float DynamicWaypointUpdateTime;
	float DynamicVolumeUpdateTime;
	float SmartObjectUpdateTime;
	int AdjustPathsAroundDynamicObstacles;
	int AIVisRaysPerFrame;
	const char* DrawRefPoints;
	int DrawFormations;
	int DrawPatterns;
	int DrawSmartObjects;
	int DrawReadibilities;
	int DrawGoals;
	const char* DrawNode;
	int DrawNodeLinkType;
	float DrawNodeLinkCutoff;
	const char* DrawLocate;
	float DebugDrawOffset;
	int DrawTargets;
	int DrawBadAnchors;
	int DrawStats;
	int DrawAttentionTargetsPosition;
	int DrawHideSpotSearchRays;
	float SteepSlopeUpValue;
	float SteepSlopeAcrossValue;
	int DebugDrawVegetationCollisionDist;
	float ExtraRadiusDuringBeautification;
	float ExtraForbiddenRadiusDuringBeautification;
	float RadiusForAutoForbidden;
	int RecordLog;
	int DrawGroups;
	int DrawTrajectory;
	int DrawHideSpots;
	int DrawRadar;
	int DrawRadarDist;
	int DebugRecordAuto;
	int DebugRecordBuffer;
	int DrawGroupTactic;
	int UpdateAllAlways;
	const char* DrawShooting;
	int DrawDistanceLUT;
	int PuppetDirSpeedControl;
	int DrawAreas;
	int DrawProbableTarget;
	int IncludeNonColEntitiesInNavigation;
	int DebugDrawDamageParts;
	int DebugDrawStanceSize;
	int DebugDrawUpdate;
	int DebugDrawEnabledPuppets;
	int DebugDrawEnabledPlayers;
	int DebugDrawVisChecQueue;
	int DebugDrawLightLevel;
	const char* DebugDrawHashSpaceAround;
	int IgnoreDeadBodies;
	float BurstWhileMovingDestinationRange;
	const char* DrawAgentStats;

	int DebugDrawPhysicsAccess;

	const char* DebugHideSpotName;

	int WarnSignalDuration;

	float SOMSpeedRelaxed;
	float SOMSpeedCombat;
	float SOMMinimumRelaxed;
	float SOMMinimumCombat;
	float SightRangeDarkIllumMod;
	float SightRangeMediumIllumMod;

	float MovementSpeedDarkIllumMod;
	float MovementSpeedMediumIllumMod;

	float RODAliveTime;
	float RODMoveInc;
	float RODStanceInc;
	float RODDirInc;
	float RODAmbientFireInc;
	float RODKillZoneInc;

	float RODKillRangeMod;
	float RODCombatRangeMod;

	float RODReactionTime;
	float RODReactionDarkIllumInc;
	float RODReactionMediumIllumInc;
	float RODReactionDistInc;
	float RODReactionDirInc;
	float RODReactionLeanInc;

	float RODLowHealthMercyTime;

	float RODCoverFireTimeMod;

	int AmbientFireEnable;
	float AmbientFireUpdateInterval;
	int AmbientFireQuota;
	int DebugDrawAmbientFire;

	int DebugDrawExpensiveAccessoryQuota;

	int DebugTargetSilhouette;
	int DebugDrawDamageControl;
	int DrawFakeTracers;
	int DrawFakeHitEffects;
	int DrawFakeDamageInd;
	int DrawPlayerRanges;
	int DrawPerceptionIndicators;
	int DrawPerceptionDebugging;
	int DrawPerceptionModifiers;
	const char* DrawPerceptionHandlerModifiers;
	int DebugPerceptionManager;

	int TargetTracking;
	float TargetTracks_DeadForgetTime;
	int TargetTracks_GlobalTargetLimit;
	int TargetTracks_TargetDebugDraw;
	const char* TargetTracks_AgentDebugDraw;
	int TargetTracks_ConfigDebugDraw;
	const char* TargetTracks_ConfigDebugFilter;

	int ForceStance;
	const char* ForceAGAction;
	const char* ForceAGSignal;
	const char* ForcePosture;
	int ForceAllowStrafing;
	const char* ForceLookAimTarget;

	float BannedNavSoTime;
	int	  WaterOcclusionEnable;
	float WaterOcclusionScale;

	int DebugDrawAdaptiveUrgency;
	int DebugDrawReinforcements;
	int DebugDrawPlayerActions;
	int DrawCollisionEvents;
	int DrawBulletEvents;
	int DrawSoundEvents;
	int DrawGrenadeEvents;
	int DrawExplosions;

	float CloakMinDist;
	float CloakMaxDist;
	float CloakIncrement;

	float BigBrushLimitSize;

	int SimpleWayptPassability;
	int ThreadedVolumeNavPreprocess;
	float ExtraVehicleAvoidanceRadiusBig;
	float ExtraVehicleAvoidanceRadiusSmall;
	float ObstacleSizeThreshold;
	float DrawGetEnclosingFailures;

	int UseAlternativeReadability;


	// Interest system
	int InterestSystem;
	int DebugInterest;

	// Behavior Tree & Activation Conditions
	const char* DebugBehaviorTree;
	const char* DebugActivationConditions;
	int DebugTactics;

	// Emotional System
	const char* DebugEmotionalSystem;

	// Code coverage
	int CodeCoverage;

	// Startup mode
	const char* StartupMode;

	// Perception handler override
	int EnablePerceptionStanceVisibleRange;
	float CrouchVisibleRange;
	float ProneVisibleRange;
	int IgnoreVisualStimulus;
	int IgnoreSoundStimulus;
	int IgnoreBulletRainStimulus;

	// Layered Navigation Mesh
	int lnmDebugDraw;
	int lnmDynamics;
	float lnmGetEnclosingExtraRange;
	const char* DrawAgentType;

	// Path Follower
	int UseSmartPathFollower;
	int DrawPathFollower;

	int LogConsoleVerbosity;
	int LogFileVerbosity;
	int EnableWarningsErrors;
	float OverlayMessageDuration;
	int ShowBehaviorCalls;

	int Recorder;
	int StatsDisplayMode;
	int DebugDrawVisionMap;

	int DrawFireEffectEnabled;
	float DrawFireEffectDecayRange;
	float DrawFireEffectMinDistance;
	float DrawFireEffectMinTargetFOV;
	float DrawFireEffectMaxAngle;

	int AllowedToHitPlayer;
	int EnableCoolMisses;
	float CoolMissesBoxSize;
	float CoolMissesBoxHeight;
	float CoolMissesMaxMissDistance;

	int WIPCode;
	int DynamicHidespotsEnabled;
};


#endif
