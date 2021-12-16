#############################################################################
## Crytek Source File
## Copyright (C) 2006, Crytek Studios
##
## Creator: Sascha Demetrio
## Date: Jul 31, 2006
## Description: GNU-make based build system
#############################################################################

PROJECT_TYPE := module
PROJECT_VCPROJ := GameDll.vcproj

-include $(PROJECT_CODE)/Project_override.mk

PROJECT_CPPFLAGS_COMMON += \
	-I$(CODE_ROOT)/CryEngine/CryCommon \
	-I$(CODE_ROOT)/CryEngine/CryAction

ifeq ($(MKOPTION_SNC),1)
  PROJECT_CPPFLAGS_COMMON += --diag_suppress=175
endif

PROJECT_SOURCES_CPP_REMOVE := \
	StdAfx.cpp \
	OrganicMotion/%.cpp

ifeq ($(MKOPTION_UNITYBUILD),1)
PROJECT_SOURCES_CPP_REMOVE += Accessory.cpp \
	Actor.cpp \
	Audio/BattleStatus.cpp \
	Audio/GameAudio.cpp \
	Audio/ScriptBind_GameAudio.cpp \
	Audio/SoundMoods.cpp \
	Automatic.cpp \
	Beam.cpp \
	Binocular.cpp \
	BlendedEffect.cpp \
	Bullet.cpp \
	BulletTime.cpp \
	Burst.cpp \
	Charge.cpp \
	Claymore.cpp \
	ClientSynchedStorage.cpp \
	Editor/GameRealtimeRemoteUpdate.cpp \
	EditorGame.cpp \
	EquipmentSystemInterface.cpp \
	Fists.cpp \
	FlowItemAnimation.cpp \
	FreezingBeam.cpp \
	GrabHandler.cpp \
	Lam.cpp \
	Melee.cpp \
	NetInputChainDebug.cpp \
	NetPlayerInput.cpp \
	OffHand.cpp \
	Plant.cpp \
	Radio.cpp \
	Rapid.cpp \
	RealtimeRemoteUpdate.cpp \
	Rock.cpp \
	SPAnalyst.cpp \
	Scan.cpp \
	Scope.cpp \
	Scout.cpp \
	ScreenEffects.cpp \
	ServerSynchedStorage.cpp \
	Shark.cpp \
	SharkMovementController.cpp \
	Single.cpp \
	SingleTG.cpp \
	SynchedStorage.cpp \
	Throw.cpp \
	TracerManager.cpp \
	Voting.cpp \
	Boids/BoidsProxy.cpp \
	Boids/BugsFlock.cpp \
	Boids/ChickenBoids.cpp \
	Boids/Flock.cpp \
	Boids/FrogBoids.cpp \
	Boids/ScriptBind_Boids.cpp \
	Environment/BattleDust.cpp \
	Environment/FlowTornado.cpp \
	Environment/Shake.cpp \
	Environment/Tornado.cpp \
	Game.cpp \
	GameActions.cpp \
	GameCVars.cpp \
	GameDll.cpp \
	GameFactory.cpp \
	GameRules.cpp \
	GameRulesClientServer.cpp \
	GameStartup.cpp \
	GameStateRecorder.cpp \
	Item.cpp \
	ItemAccessory.cpp \
	ItemClientServer.cpp \
	ItemDualWield.cpp \
	ItemEffect.cpp \
	ItemEvents.cpp \
	ItemParams.cpp \
	ItemResource.cpp \
	ItemScheduler.cpp \
	ItemSharedParams.cpp \
	ItemView.cpp \
	Player.cpp \
	PlayerFeature.cpp \
	PlayerInput.cpp \
	PlayerMovement.cpp \
	PlayerMovementController.cpp \
	PlayerRotation.cpp \
	PlayerView.cpp \
	Nodes/FlowActorSensor.cpp \
	Nodes/FlowFadeNode.cpp \
	Nodes/FlowHitInfoNode.cpp \
	Nodes/FlowPlayerStagingNode.cpp \
	Nodes/FlowPostFXNodes.cpp \
	Nodes/MPNodes.cpp \
	Nodes/WeaponNodes.cpp \
	FlowVehicleNodes.cpp \
	BlendNode.cpp \
	Weapon.cpp \
	EMPField.cpp \
	AIGrenade.cpp \
	Projectile.cpp \
	ReferenceWeapon.cpp \
	AVMine.cpp \
	AmmoParams.cpp \
	AmmoPickup.cpp \
	ThrowableWeapon.cpp \
	IronSight.cpp \
	Rocket.cpp \
	RocketLauncher.cpp \
	WeaponAttachmentManager.cpp \
	WeaponClientServer.cpp \
	WeaponEvent.cpp \
	WeaponInput.cpp \
	WeaponSharedParams.cpp \
	WeaponSystem.cpp \
	WorkOnTarget.cpp \
	TacBullet.cpp \
	TagBullet.cpp \
	DebugGun.cpp \
	Detonate.cpp \
	C4.cpp \
	C4Detonator.cpp \
	C4Projectile.cpp \
	GunTurret.cpp \
	HomingMissile.cpp \
	Shotgun.cpp \
	ScriptBind_Actor.cpp \
	ScriptBind_Game.cpp \
	ScriptBind_GameRules.cpp \
	ScriptBind_Item.cpp \
	ScriptBind_Weapon.cpp \
	ScriptControlledPhysics.cpp \
	ScriptUtils.cpp \
	VehicleActionAutomaticDoor.cpp \
	VehicleActionDeployRope.cpp \
	VehicleActionEntityAttachment.cpp \
	VehicleActionLandingGears.cpp \
	VehicleClient.cpp \
	VehicleDamageBehaviorBurn.cpp \
	VehicleDamageBehaviorCameraShake.cpp \
	VehicleDamageBehaviorCollisionEx.cpp \
	VehicleDamageBehaviorExplosion.cpp \
	VehicleDamageBehaviorTire.cpp \
	VehicleMovementAmphibious.cpp \
	VehicleMovementBase.cpp \
	VehicleMovementHelicopter.cpp \
	VehicleMovementHovercraft.cpp \
	VehicleMovementStdBoat.cpp \
	VehicleMovementStdWheeled.cpp \
	VehicleMovementTank.cpp \
	VehicleMovementTweaks.cpp \
	VehicleMovementVTOL.cpp \
	VehicleMovementWarrior.cpp \
	VehicleWeapon.cpp \
	HUD/BitmapUi.cpp \
	HUD/LocalizedStringManager.cpp \
	CachedAIValues.cpp \
	

PROJECT_SOURCES_CPP_ADD += Game02_main_uber.cpp\
	Game02_game_uber.cpp\
	Game02_menus_uber.cpp \
	Game02_player_uber.cpp \
	Game02_nodes_vehicle_uber.cpp \
	Game02_weapon_uber.cpp \
	Game02_hud_uber.cpp

endif

# vim:ts=8:sw=8
