#ifndef __GAMECVARS_H__
#define __GAMECVARS_H__

#define DEBUG_NEW_SPAWNING 1		// eventually will just control debugging. For now activates new spawning mode itself as well as debugging

struct SPowerSprintParams
{
	float foward_angle;
};

struct SJumpAirControl
{
	float air_control_scale;
	float air_resistance_scale;
	float air_inertia_scale;
};

struct SPlayerHealth
{
	float normal_regeneration_rateSP;
	float normal_threshold_time_to_regenerateSP;

	float normal_regeneration_rateMP;
	float normal_threshold_time_to_regenerateMP;

	int   enable_FallandPlay;
	int		collision_health_threshold;

	float fallDamage_SpeedSafe;
	float fallDamage_SpeedFatal;
	float fallDamage_CurveAttack;
	int		fallDamage_health_threshold;
	int		debug_FallDamage;
};

struct SAltNormalization
{
	int enable;
	float hud_ctrl_Curve_Unified;
	float hud_ctrl_Coeff_Unified;
};

struct SPlayerMovement
{
	float nonCombat_heavy_weapon_speed_scale;
	float nonCombat_heavy_weapon_sprint_scale;

	float power_sprint_targetFov;

	float sprint_timeInAirToStopSprinting;
};

struct SPlayerMelee
{
	float	melee_snap_angle_limit;
	float melee_snap_blend_speed;
	float	raycast_max_height;
	int		debug_gfx;
};

struct SPlayerLedgeClamber
{
	float cameraBlendWeight;
	int		debugDraw;
};

struct SPlayerPickAndThrow
{
	int		debugDraw;
};

struct SPlayerAirFrictionPerkControl
{
	float fall_activation_height;
	float fall_maxSeepd_height;

	float forward_speed;
	float min_falling_speed;
	float max_falling_speed;

	float view_pitch_shake;
	float view_recover_time;

};

struct SPlayerSlideControl
{
	float min_speed_threshold;
	float min_speed;

	float deceleration_speed;
	float deceleration_speed_powerMode;
	float min_downhill_threshold;
	float max_downhill_threshold;
	float max_downhill_acceleration;
};

struct SPlayerEnemyRamming
{
	float player_to_player;
	float ragdoll_to_player;
	float fall_damage_threashold;
	float safe_falling_speed;
	float fatal_falling_speed;
	float max_falling_damage;
};

struct SPlayerNanoVision
{
	float max_distance_visible;
};

struct SPlayerCoverAndLean
{
	int enable;
	int debug_draw;
	int num_detection_rays;
	int enable_view_clamp;
};


struct SVolatileSpike
{
	float decalLife;;
	float decalMaxSize;
	float decalMinSize;
	float decalEmitterLife;
	float decalGrowTime;
	int   maxRandomSteamEmitters;
	int		maxDecals;
};

struct SPostEffect
{	// Use same naming convention as the post effects in the engine
	float FilterGrain_Amount;
	float FilterRadialBlurring_Amount;
	float FilterRadialBlurring_ScreenPosX;
	float FilterRadialBlurring_ScreenPosY;
	float FilterRadialBlurring_Radius;
	float Global_User_ColorC;
	float Global_User_ColorM;
	float Global_User_ColorY;
	float Global_User_ColorK;
	float Global_User_Brightness;
	float Global_User_Contrast;
	float Global_User_Saturation;
	float Global_User_ColorHue;
};

struct SSilhouette
{
	int			enableUpdate;
	float		r;
	float		g;
	float		b;
	float		a;
	int			enabled;
};

struct SAIPerceptionCVars
{
	int			movement_useSurfaceType;
	float		movement_movingSurfaceDefault;
	float		movement_standingRadiusDefault;
	float		movement_crouchRadiusDefault;
	float		movement_standingMovingMultiplier;
	float		movement_crouchMovingMultiplier;
};

struct SCVars
{	
	static const float v_altitudeLimitDefault()
	{
		return 600.0f;
	}

		// Flash
	int			g_flash_hud_enable;
	int			g_flash_hud_scale_enable;
	int			g_flash_mainmenu_state;


		//bitmap ui overlays
	int		g_showBitmapUi;
	int		g_show_fullscreen_info;
	int		g_show_crosshair;
	int		g_show_crosshair_tp;	

		// bullet time CVars
	int		bt_ironsight;
	int		bt_speed;
	int		bt_end_reload;
	int		bt_end_select;
	int		bt_end_melee;
	float bt_time_scale;
	float bt_pitch;
	float bt_energy_max;
	float bt_energy_decay;
	float bt_energy_regen;





	
	float cl_fov;
	float cl_tpvDist;
	float cl_tpvYaw;
	float cl_nearPlane;
	float cl_sprintShake;
	float cl_sensitivity;
	float cl_sensitivityController;
	float cl_mp_sensitivityControllerDefault;
	int		cl_invertMouse;
	int		cl_invertController;
	int		cl_crouchToggle;
	int		cl_fpBody;
	int		cl_debugSwimming;
	int		cl_suitModeSpecificScreenFX;
	int		cl_logAsserts;
	int   cl_motionBlur;

	int		cl_camModify;
	float cl_camKeepX;
	float cl_camKeepY;
	float cl_camKeepZ;
	float cl_camOffsetX;
	float cl_camOffsetY;
	float cl_camOffsetZ;
	float cl_camRotateX;
	float cl_camRotateY;
	float cl_camRotateZ;
	float cl_camTranslateX;
	float cl_camTranslateY;
	float cl_camTranslateZ;

	ICVar* 	ca_GameControlledStrafingPtr;
	float pl_curvingSlowdownSpeedScale;
	float ac_enableProceduralLeaning;

	float cl_shallowWaterSpeedMulPlayer;
	float cl_shallowWaterSpeedMulAI;
	float cl_shallowWaterDepthLo;
	float cl_shallowWaterDepthHi;

	float cl_speedToBobFactor;
	float cl_bobWidth;
	float cl_bobHeight;
	float cl_strafeHorzScale;
	float cl_sprintBlur;

	float	cl_idleBreaksDelayTime;

	int   sv_votingTimeout;
	int   sv_votingCooldown;
	float sv_votingRatio;
	float sv_votingTeamRatio;

	int   sv_input_timeout;

	ICVar*sv_aiTeamName;

	ICVar *performance_profile_logname;

	int		i_staticfiresounds;
	int		i_soundeffects;
	int		i_lighteffects;
	int		i_particleeffects;
	int		i_rejecteffects;

	float pl_inputAccel;
	int   pl_debug_suit;
	int		pl_debug_pickable_items;

	int g_killcamEnable;
	int g_killcamDebug;
	float g_killcamPlaybackSyncDelay;
	float g_killcamProjectileDistance;
	float g_killcamProjectileHeightOffset;
	float g_killcamProjectileVictimHeightOffset;
	float g_killcamProjectileMinimumVictimDist;
	float g_killcamSmoothing;
	float g_killcamGrenadeSmoothingDist;

	int g_stapEnable;
	int g_translationPinningEnable;

	int g_MPLedgeGrabbing;
	int g_MPCoverAndLean;

	int g_dogtagsEnable;
	int g_dogtagsHUDPopupEnable;
	float g_dogtagsHUDPopupDuration;

	float g_tentacle_joint_limit;
	int		g_detachCamera;
	int		g_moveDetachedCamera;
	float g_detachedCameraMoveSpeed;
	float g_detachedCameraRotateSpeed;
	float g_detachedCameraTurboBoost;
	int g_detachedCameraDebug;
	int		g_enableSpeedLean;
	int   g_difficultyLevel;
	int		g_difficultyHintSystem;
	float g_difficultyRadius;
	int		g_difficultyRadiusThreshold;
	int		g_difficultySaveThreshold;
	float g_flashBangMinSpeedMultiplier;
	float g_flashBangSpeedMultiplierFallOffEase;
	float g_flashBangNotInFOVRadiusFraction;
	float g_flashBangMinFOVMultiplier;
	int		g_flashBangFriends;
	int		g_flashBangSelf;

	int		g_STAPCameraAnimation;

	float g_stanceTransitionSpeed;
	int   g_debugaimlook;
	int		g_enableIdleCheck;
	float g_playerLowHealthThreshold;
	int		g_punishFriendlyDeaths;
	float g_fallAndPlayThreshold;

	float g_fireStabilizationThreshold;

	int g_SurvivorOneVictoryConditions_watchLvl;
	int g_SimpleEntityBasedObjective_watchLvl;
	int g_CarryObjective_watchLvl;
	int g_CombiCaptureObjective_watchLvl;
	int g_CTFScoreElement_watchLvl;
	int g_KingOfTheHillObjective_watchLvl;

	int sv_pacifist;

	int g_empStyle;
		
	float g_pp_scale_income;
	float g_pp_scale_price;
	float g_energy_scale_income;
	float g_energy_scale_price;

	float g_dofset_minScale;
	float g_dofset_maxScale;
	float g_dofset_limitScale;

	float g_dof_minHitScale;
	float g_dof_maxHitScale;
	float g_dof_sampleAngle;
	float g_dof_minAdjustSpeed;
	float g_dof_maxAdjustSpeed;
	float g_dof_averageAdjustSpeed;
	float g_dof_distAppart;
	int		g_dof_ironsight;

	int g_bulletPenetrationEnable;
	int g_bulletPenetrationDebug;
	float g_bulletPenetrationDebugTimeout;

	float g_hologram_maxBlockedTime;
	float g_hologram_negligibleMovementSqrd;
	float g_hologram_initialNoMovementCheckTime;
	float g_hologram_spawnOffsetDist;

	float g_explosion_materialFX_raycastLength;

	// explosion culling
	int		g_ec_enable;
	float g_ec_radiusScale;
	float g_ec_volume;
	float g_ec_extent;
	int		g_ec_removeThreshold;

	float g_radialBlur;
	int		g_playerFallAndPlay;

	float g_timelimit;
	float g_timelimitextratime;
	int		g_teamlock;
	float g_roundtime;
	int		g_preroundtime;
	int		g_suddendeathtime;
	int		g_roundlimit;
	int		g_fraglimit;
	int		g_fraglead;
	float g_friendlyfireratio;
	int   g_revivetime; 
	int   g_autoteambalance;
	int   g_autoTeamBalanceAmount;
	int   g_minplayerlimit;
	int   g_minteamlimit;
	float g_objectivesTimeScale;
	float g_hostMigrationConnectionTimeout;
	float g_hostMigrationResumeTime;

	int   g_tk_punish;
	int		g_tk_punish_limit;

	int   g_debugNetPlayerInput;
	int   g_debugCollisionDamage;
	int   g_debugHits;

	int		g_enableGameSwitchMenu;
	int		g_multiplayerDefault;
	int		g_EPD;

	float g_gameRules_startTimerLength;
	float g_gameRules_nextLevelTimerLength;
	float g_gameRules_showScoreboardTimerLength;
	float g_gameRules_minPlayerWarningTimerLength;

	float g_trooperTentacleAnimBlend;
	float g_trooperBankingMultiplier;
	float g_alienPhysicsAnimRatio;  

	int		g_debug_fscommand;
	int		g_debugDirectMPMenu;
	int		g_skipIntro;
	int		g_resetActionmapOnStart;
	int		g_useProfile;
	int		g_startFirstTime;

	float	g_corpseMinTime;
	float	g_corpseUnseenTime;
	float	g_corpsePollTime;
	float	g_corpseMinDistance;

	int		g_scoreLimit;
	int		g_wavesLimit;

	float	g_spawndeathdist;
	float g_spawn_enemyDistLevelSizeDivider;
	float g_spawn_lastKillerDist;
	float g_spawn_lastSpawnDist;
	float g_spawn_explosiveSafeDist;
	float g_spawn_ctfFlagAvoidDist;

	//Inventory control
	int		g_inventoryNoLimits;
	int   g_inventoryWeaponCapacity;
	int		g_inventoryExplosivesCapacity;
	int		g_inventoryGrenadesCapacity;

	int g_loadoutServerControlled;

	ICVar*pl_debug_filter;
	int		pl_debug_vistable;
	int		pl_debug_movement;
	int		pl_debug_jumping;
	int		pl_debug_aiming;
	int		pl_debug_aiming_input;
	int		pl_debug_view;

#if !defined(_RELEASE)
	int		pl_debug_watch_camera_mode;
	int		pl_debug_log_camera_mode_changes;
#endif

	int pl_aim_acceleration_enabled;
	float pl_aim_cloaked_multiplier;
	int pl_targeting_debug;

	int pl_switchTPOnKill;

	float pl_TacticalScanDuration;

	float ctrlr_acceleration_vertical_up;
	float ctrlr_acceleration_vertical_down;
	float ctrlr_acceleration_horizontal_up;
	float ctrlr_acceleration_horizontal_down;

	float controller_power_curve;
	float controller_multiplier_z;
	float controller_multiplier_x;
	float controller_full_turn_multiplier_x;
	float controller_full_turn_multiplier_z;

	float ctrlr_maxspeed_horizontal;
	float ctrlr_maxspeed_vertical;
	float ctrlr_exponent;
	float ctrlr_timeaccel_multiplier_horizontal;
	float ctrlr_timeaccel_multiplier_vertical;
	float ctrlr_timeaccel_base_multiplier_horizontal;
	float ctrlr_timeaccel_base_multiplier_vertical;
	float ctrlr_timeaccel_boost_time;

	int ctrlr_OUTPUTDEBUGINFO;

	float pl_jump_maxTimerValue;
	float pl_jump_baseTimeAddedPerJump;
	float pl_jump_currentTimeMultiplierOnJump;

	float pl_jump_quickPressThresh;

	int pl_velocityBasedInterpolation;
	float pl_velocityInterpFactor;

	SPowerSprintParams	 pl_power_sprint;
	SJumpAirControl pl_jump_control;
	SPlayerHealth pl_health;
	SPlayerMovement pl_movement;
	SPlayerLedgeClamber pl_ledgeClamber;
	SPlayerPickAndThrow pl_pickAndThrow;
	SPlayerAirFrictionPerkControl pl_airFrictionPerk_control;
	SPlayerSlideControl	 pl_sliding_control;
	SPlayerEnemyRamming pl_enemy_ramming;
	SPlayerNanoVision pl_nano_vision;
	SPlayerMelee		pl_melee;
	SAltNormalization aim_altNormalization;

	int		aln_debug_movement;
	ICVar*aln_debug_filter;
	
	// animation triggered footsteps
	int   g_FootstepSoundsFollowEntity;
	int   g_FootstepSoundsDebug;
	float g_footstepSoundMaxDistanceSq;

#if !defined(_RELEASE)
	int   v_debugMovement;
	float v_debugMovementMoveVertically;
	float v_debugMovementX;
	float v_debugMovementY;
	float v_debugMovementZ;
	float v_debugMovementSensitivity;
#endif

	int   v_profileMovement;  
	int   v_draw_suspension;
	int   v_draw_slip;
	int   v_pa_surface;    
	int   v_invertPitchControl;  
	float v_wind_minspeed; 
	float v_sprintSpeed;
	int   v_dumpFriction;
	int   v_rockBoats;
	int   v_debugSounds;
	float v_altitudeLimit;
	ICVar* pAltitudeLimitCVar;
	float v_altitudeLimitLowerOffset;
	float v_airControlSensivity;
	float v_stabilizeVTOL;
	int   v_help_tank_steering;
	int   v_debugMountedWeapon;
	ICVar* pVehicleQuality;

	int		pl_grabOntoLedges;

	float pl_swimBaseSpeed;
	float pl_swimBackSpeedMul;
	float pl_swimSideSpeedMul;
	float pl_swimVertSpeedMul;
	float pl_swimGetOutBoost;
	float pl_swimGetOutOffset;
	float pl_swimNormalSprintSpeedMul;
	float pl_swimSpeedSprintSpeedMul;
	float pl_swimUpSprintSpeedMul;

	int		pl_mike_debug;
	SPlayerCoverAndLean pl_cover_and_lean;

	int   g_assertWhenVisTableNotUpdatedForNumFrames;

	int		hud_DebugFrontEnd;
	int		hud_UseFlashFrontEndLoading;

	int		hud_DeathRedScreen;
	int		hud_ContextualHealthIndicator;

	int		hud_colorLine;
	int		hud_colorOver;
	int		hud_colorText;
	int		hud_voicemode;
	int		hud_showAllObjectives;
	int		hud_subtitles;
	int		hud_subtitlesDebug;
	int   hud_subtitlesRenderMode;
	int   hud_subtitlesHeight;
	int   hud_subtitlesFontSize;
	int   hud_subtitlesShowCharName;
	int   hud_subtitlesQueueCount;
	int   hud_subtitlesVisibleCount;
	int		hud_aspectCorrection;
	int		hud_ctrlZoomMode;
	int   hud_faderDebug;
	int		hud_startPaused;

	int   hud_objectiveIcons_attachToAllBullets;
	float hud_objectiveIcons_flashTime;

	int		hud_mapEnable;
	float hud_mapOverrideCenterX;
	float hud_mapOverrideCenterY;

	float aim_assistMinDistance;
	float aim_assistMaxDistance;
	float aim_assistSprintMultiplier;
	float aim_assistSlidingMultiplier;
	float aim_assistGlidingMultiplier;
	float aim_assistMinTurnScale;
	float aim_assistSlowFalloffStartDistance;
	float aim_assistSlowDisableDistance;
	float aim_assistSlowThresholdOuter;
	float aim_assistSlowDistanceModifier;

	int aim_assistCrosshairSize;
	int aim_assistCrosshairDebug;

	ICVar*i_debuggun_1;
	ICVar*i_debuggun_2;

	int		coop_respawn_time;

	float	tracer_min_distance;
	float	tracer_max_distance;
	float	tracer_min_scale;
	float	tracer_max_scale;
	int		tracer_max_count;
	float	tracer_player_radiusSqr;
	int		i_debug_projectiles;
	int		i_debug_weaponActions;
	int		i_debug_spread;
	int		i_debug_recoil;
	int		i_auto_turret_target;
	int		i_auto_turret_target_tacshells;
	int		i_debug_zoom_mods;
	int   i_debug_turrets;
	int   i_debug_sounds;
	int		i_debug_mp_flowgraph;

	int		i_debug_itemparams_memusage;
	int		i_debug_weaponparams_memusage;

	int		h_useIK;
	int		h_drawSlippers;

	ICVar*  g_quickGame_map;
	ICVar*  g_quickGame_mode;
	int     g_quickGame_min_players;
	int     g_quickGame_prefer_lan;
	int     g_quickGame_prefer_favorites;
	int     g_quickGame_prefer_my_country;
	int     g_quickGame_ping1_level;
	int     g_quickGame_ping2_level;
	int     g_quickGame_debug;
	int			g_skip_tutorial;

	int     g_displayIgnoreList;
	int     g_buddyMessagesIngame;

	int			g_battleDust_enable;
	int			g_battleDust_debug;
	ICVar*  g_battleDust_effect;

	int			g_PSTutorial_Enabled;

	int			g_enableFriendlyFallAndPlay;

	int			g_mpCoreGameplayTesting; 
	int			g_mpPermanentSonarVision;
	int			g_mpPermanentSonarVision_alpha;
	int			g_mpPermanentSonarVision_outline_alpha;
	int			g_mpAllSeeingRadar;
	int			g_mpAllSeeingRadarSv;
	int			g_mpDisableRadar;
	int			g_mpNoEnemiesOnRadar;

	int			g_spectate_TeamOnly;
	float		g_spectate_cctv_RotScale;
	int			g_spectate_DisableManual;
	int			g_spectate_DisableSpawnOptOut;
	int			g_spectate_DisableDead;
	int			g_spectate_DisableFree;
	int			g_spectate_DisableFollow;
	int			g_spectate_DisableCCTV;

	int			g_claymore_limit;
	int			g_avmine_limit;
	int			g_debugMines;
	int			g_deathCam;

	float		g_tpdeathcam_focalHeightBump;
	float		g_tpdeathcam_initDist;
	float		g_tpdeathcam_defaultCameraHeightOffset;
	float		g_tpdeathcam_collideFocalRaiseFrac;

	float		g_nanoSuitMenuToggleTime;
	int			g_nanoSuitEnableSuitShapeDeformation;
	int			g_logSuitActions;

	int			g_useHitSoundFeedback;
	int			g_hasWindowFocus;

	int			g_displayPlayerDamageTaken;
	int			g_displayDbgText_perk;
	int			g_displayDbgText_hud;
	int			g_displayDbgText_plugins;
	int			g_displayDbgText_pmv;
	int			g_displayDbgText_psGuns;
	int			g_displayDbgText_actorState;

	float		vehicle_steering_curve_scale;
	float		vehicle_steering_curve;
	float		vehicle_acceleration_curve_scale;
	float		vehicle_acceleration_curve;
	float		vehicle_deceleration_curve_scale;
	float		vehicle_deceleration_curve;

#if DEBUG_NEW_SPAWNING
	int			g_debug_spawn_mode;
	ICVar		*g_debug_spawning_visually_teamA;
	ICVar		*g_debug_spawning_visually_teamB;
	int			g_debug_spawning_visually;
	int			g_debug_entity_clustering;
	float		g_debug_entity_clustering_radius;
#endif // DEBUG_NEW_SPAWNING
	float		g_spawn_conflict_clustering_radius;
	int			g_spawn_vistable_show;
	int			g_spawn_vistable_numLineTestsPerFrame;
	float		g_spawn_vistable_maxDistToAltSpawnEntity;
	int			g_showShadowChar;
	int			g_animateCameraOrientation;
	int			g_showIdleStats;

	// Causes a slow motion effect when opening nano suit menu.
	// default =1 (disabled) the lower the ratio value, the higher the slow mo
	float		g_SuitModeSlowMotionRatio;

	int			g_infiniteAmmo;
	int		  g_animatorDebug;
	int			g_hideArms;
	int			g_useHitReactController;
	int			g_debugHitReacts;
	
#if !defined(_RELEASE)
	int			g_EAWhiteBuild;
	int			g_playerUsesDedicatedInput;
	int			g_playerEnableDedicatedInput;
#endif

	int watch_enabled;
	float watch_text_render_start_pos_x;
	float watch_text_render_start_pos_y;
	float watch_text_render_size;
	float watch_text_render_lineSpacing;
	float watch_text_render_fxscale;

	int autotest_enabled;
	ICVar* autotest_state_setup;
	int autotest_quit_when_done;
	int designer_warning_enabled;

#if !defined(_RELEASE)
	ICVar* net_onlyListGameServersContainingText;
	ICVar* net_nat_type;
#endif

	SVolatileSpike g_volatileSpike;

	SSilhouette g_teamSilhouette;
	SSilhouette g_enemySilhouette;

	SPostEffect g_postEffect;
	int g_gameFXSystemDebug;

	int ai_DebugVisualScriptErrors;
	int ai_DebugPressureSystem;
	int ai_DebugAggressionSystem;
	int ai_DebugBattleFront;
	int ai_DebugSearch;

	int g_actorViewDistRatio;
	int g_mpPlayerLodRatio;

	int			g_hitDeathReactions_enable;
	int			g_hitDeathReactions_useLuaDefaultFunctions;
	int			g_hitDeathReactions_disable_ai;
	int			g_hitDeathReactions_debug;
	int			g_hitDeathReactions_disableRagdoll;
	int			g_hitDeathReactions_disableHitAnimatedCollisions;

	float		g_maximumDamage;

	int g_flyCamLoop;
	int g_dummyPlayersFire;
	int g_dummyPlayersMove;

	int			g_muzzleFlashCull;
	float		g_muzzleFlashCullDistance;
	int			g_rejectEffectVisibilityCull;
	float		g_rejectEffectCullDistance;
	int 		g_mpCullShootProbablyHits;

	float		g_cloakRefractionScale;

	int g_telemetry_gameplay_enabled;
	int g_telemetryEnabledSP;
	float g_telemetrySampleRatePerformance;
	float g_telemetrySampleRateBandwidth;
	float g_telemetrySampleRateMemory;
	const char* g_telemetryConfig;

	float		g_deathFadeDelay;
	float		g_deathFadeTime;
	float		g_deathReloadDelay;

	SAIPerceptionCVars ai_perception;

	SCVars()
	{
		memset(this,0,sizeof(SCVars));
	}

	~SCVars() { ReleaseCVars(); }

	void InitCVars(IConsole *pConsole);
	void ReleaseCVars();
	
	void InitAIPerceptionCVars(IConsole *pConsole);
	void ReleaseAIPerceptionCVars(IConsole* pConsole);
};

#endif //__GAMECVARS_H__
