/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:  Register the factory templates used to create classes from names
e.g. REGISTER_FACTORY(pFramework, "Player", CPlayer, false);
or   REGISTER_FACTORY(pFramework, "Player", CPlayerG4, false);

Since overriding this function creates template based linker errors,
it's been replaced by a standalone function in its own cpp file.

-------------------------------------------------------------------------
History:
- 17:8:2005   Created by Nick Hesketh - Refactor'd from Game.cpp/h

*************************************************************************/

#include "StdAfx.h"
#include "Game.h"
#include "Player.h"

//aliens
#include "Alien.h"
#include "Scout.h"
#include "Hunter.h"
#include "Trooper.h"
//#include "Observer.h"
#include "Shark.h"
#include "Spotter.h"
#include "Harvester.h"

//
#include "Item.h"
#include "Weapon.h"
#include "VehicleWeapon.h"
#include "VehicleHMG.h"
#include "AmmoPickup.h"
#include "Binocular.h"
#include "C4.h"
#include "DebugGun.h"
#include "ReferenceWeapon.h"
#include "GunTurret.h"
#include "GunTurret_AimWhereControllingPlayerIsAiming.h"
#include "ThrowableWeapon.h"
#include "JAW.h"
#include "AIGrenade.h"
#include "Accessory.h"
#include "HandGrenades.h"

#include "Flashlight.h"
#include "Laser.h"
#include "LTAG.h"
#include "HMG.h"
#include "Javelin.h"
#include "WeaponWithPickupTimer.h"
#include "PickAndThrowWeapon.h"
#include "BTBBomb.h"
#include "Dogtag.h"

#include "PlayerHologram.h"
#include "DummyPlayer.h"

#include "VehicleMovementBase.h"
#include "Vehicle/VehicleMovementDummy.h"
#include "VehicleActionAutomaticDoor.h"
#include "VehicleActionDeployRope.h"
#include "VehicleActionEntityAttachment.h"
#include "VehicleActionLandingGears.h"
#include "VehicleDamageBehaviorBurn.h"
#include "VehicleDamageBehaviorCameraShake.h"
#include "VehicleDamageBehaviorCollisionEx.h"
#include "VehicleDamageBehaviorExplosion.h"
#include "VehicleDamageBehaviorTire.h"
#include "VehicleMovementStdWheeled.h"
#include "VehicleMovementArcadeWheeled.h"
#include "VehicleMovementHovercraft.h"
#include "VehicleMovementHelicopter.h"
#include "VehicleMovementStdBoat.h"
#include "VehicleMovementTank.h"
#include "VehicleMovementVTOL.h"
#include "VehicleMovementAmphibious.h"
#include "Vehicle/VehicleViewSteer.h"

#include "ScriptControlledPhysics.h"

#include "GameRules.h"
#include "GameRulesModules/GameRulesModulesManager.h"
#include "GameRulesModules/IGameRulesTeamsModule.h"
#include "GameRulesModules/GameRulesStandardTwoTeams.h"
#include "GameRulesModules/IGameRulesStateModule.h"
#include "GameRulesModules/GameRulesStandardState.h"
#include "GameRulesModules/GameRulesStandardVictoryConditionsTeam.h"
#include "GameRulesModules/GameRulesStandardVictoryConditionsPlayer.h"
#include "GameRulesModules/GameRulesObjectiveVictoryConditionsTeam.h"
#include "GameRulesModules/GameRulesSurvivorOneVictoryConditions.h"
#include "GameRulesModules/GameRulesStandardSetup.h"
#include "GameRulesModules/GameRulesStandardScoring.h"
#include "GameRulesModules/GameRulesAssistScoring.h"
#include "GameRulesModules/GameRulesStandardPlayerStats.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"
#include "GameRulesModules/GameRulesSpawningBase.h"
#include "GameRulesModules/GameRulesMPSpawning.h"
#include "GameRulesModules/GameRulesMPHQFrontlineSpawning.h"
#include "GameRulesModules/GameRulesMPSimpleSpawning.h"
#include "GameRulesModules/GameRulesMPSpawningWithLives.h"
#include "GameRulesModules/GameRulesMPDamageHandling.h"
#include "GameRulesModules/GameRulesMPActorAction.h"
#include "GameRulesModules/GameRulesMPSpectator.h"
#include "GameRulesModules/GameRulesProgressiveObjectives.h"
#include "GameRulesModules/GameRulesLuaObjectives.h"
#include "GameRulesModules/GameRulesStandardRounds.h"
#include "GameRulesModules/GameRulesStatsRecording.h"

#include "Environment/Tornado.h"
#include "Environment/AlienMine.h"
#include "Environment/Shake.h"
#include "Environment/InteractiveObject.h"
#include "Environment/AlienDropPod.h"

#include "Environment/BattleDust.h"

#include <IItemSystem.h>
#include <IVehicleSystem.h>
#include <IGameRulesSystem.h>

#include "GameCVars.h"

#define HIDE_FROM_EDITOR(className)																																				\
  { IEntityClass *pItemClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(className);\
  pItemClass->SetFlags(pItemClass->GetFlags() | ECLF_INVISIBLE); }																				\

#define REGISTER_GAME_OBJECT(framework, name, script)\
	{\
	IEntityClassRegistry::SEntityClassDesc clsDesc;\
	clsDesc.sName = #name;\
	clsDesc.sScriptFile = script;\
struct C##name##Creator : public IGameObjectExtensionCreatorBase\
		{\
		C##name *Create()\
			{\
			return new C##name();\
			}\
			void GetGameObjectExtensionRMIData( void ** ppRMI, size_t * nCount )\
			{\
			C##name::GetGameObjectExtensionRMIData( ppRMI, nCount );\
			}\
		};\
		static C##name##Creator _creator;\
		framework->GetIGameObjectSystem()->RegisterExtension(#name, &_creator, &clsDesc);\
	}

#define REGISTER_GAME_OBJECT_EXTENSION(framework, name)\
	{\
struct C##name##Creator : public IGameObjectExtensionCreatorBase\
		{\
		C##name *Create()\
			{\
			return new C##name();\
			}\
			void GetGameObjectExtensionRMIData( void ** ppRMI, size_t * nCount )\
			{\
			C##name::GetGameObjectExtensionRMIData( ppRMI, nCount );\
			}\
		};\
		static C##name##Creator _creator;\
		framework->GetIGameObjectSystem()->RegisterExtension(#name, &_creator, NULL);\
	}

// Register the factory templates used to create classes from names. Called via CGame::Init()
void InitGameFactory(IGameFramework *pFramework)
{
	assert(pFramework);

	REGISTER_FACTORY(pFramework, "Player", CPlayer, false);
	REGISTER_FACTORY(pFramework, "Hologram", CPlayerHologram, false);

	REGISTER_FACTORY(pFramework, "Grunt", CPlayer, true);
	
	REGISTER_FACTORY(pFramework, "DamageTestEnt", CPlayer, true);
	REGISTER_FACTORY(pFramework, "DummyPlayer", CDummyPlayer, true);

	REGISTER_FACTORY(pFramework, "Civilian", CPlayer, true);

	// Crysis2 
	REGISTER_FACTORY(pFramework, "NewAliens/AlienStalker", CPlayer, true);
	REGISTER_FACTORY(pFramework, "NewAliens/AlienGrunt", CPlayer, true);
	REGISTER_FACTORY(pFramework, "NewAliens/AlienHeavy", CPlayer, true);
	REGISTER_FACTORY(pFramework, "NewAliens/AlienPinger", CPlayer, true);
	REGISTER_FACTORY(pFramework, "NewAliens/AlienTick", CPlayer, true);

	REGISTER_FACTORY(pFramework, "NewHumans/HumanGrunt", CPlayer, true);

	// Prototypes using BT
	//REGISTER_FACTORY(pFramework, "Drone", CPlayer, true);
	//REGISTER_FACTORY(pFramework, "Prototype/Pinger", CPinger, true);
	REGISTER_FACTORY(pFramework, "Prototype/Spotter", CSpotter, true);
	REGISTER_FACTORY(pFramework, "Prototype/Harvester", CHarvester, true);
	//REGISTER_FACTORY(pFramework, "Prototype/Pinger", CPlayer, true);

	// Items
	REGISTER_FACTORY(pFramework, "Item", CItem, false);
	REGISTER_FACTORY(pFramework, "Accessory", CAccessory, false);
	REGISTER_FACTORY(pFramework, "Flashlight", CFlashLight, false);
	REGISTER_FACTORY(pFramework, "Laser", CLaser, false);
	REGISTER_FACTORY(pFramework, "HandGrenades", CHandGrenades, false);
	REGISTER_FACTORY(pFramework, "Javelin", CJavelin, false);
	REGISTER_FACTORY(pFramework, "BTBBomb", CBTBBomb, false);
	REGISTER_FACTORY(pFramework, "Dogtag", CDogtag, false);

	// Weapons
	REGISTER_FACTORY(pFramework, "Weapon", CWeapon, false);
	REGISTER_FACTORY(pFramework, "VehicleWeapon", CVehicleWeapon, false);
	REGISTER_FACTORY(pFramework, "VehicleHMG", CVehicleHMG, false);
	REGISTER_FACTORY(pFramework, "AmmoPickup", CAmmoPickup, false);
	REGISTER_FACTORY(pFramework, "AVMine", CThrowableWeapon, false);
	REGISTER_FACTORY(pFramework, "Claymore", CThrowableWeapon, false);
	REGISTER_FACTORY(pFramework, "Binocular", CBinocular, false);
	REGISTER_FACTORY(pFramework, "C4", CC4, false);
	REGISTER_FACTORY(pFramework, "DebugGun", CDebugGun, false);
	REGISTER_FACTORY(pFramework, "ReferenceWeapon", CReferenceWeapon, false);
	REGISTER_FACTORY(pFramework, "GunTurret", CGunTurret, false);
	REGISTER_FACTORY(pFramework, "SubservientGunTurret", CGunTurret_AimWhereControllingPlayerIsAiming, false);
	REGISTER_FACTORY(pFramework, "JAW", CJaw, false);
	REGISTER_FACTORY(pFramework, "AIGrenade", CAIGrenade, false);
	REGISTER_FACTORY(pFramework, "LTAG", CLTag, false);
	REGISTER_FACTORY(pFramework, "ExtractionGun", CWeaponWithPickupTimer, false);
	REGISTER_FACTORY(pFramework, "PickAndThrowWeapon", CPickAndThrowWeapon, false);

	REGISTER_FACTORY(pFramework, "HMG", CHMG, false);

	// vehicle objects
	IVehicleSystem* pVehicleSystem = pFramework->GetIVehicleSystem();

#define REGISTER_VEHICLEOBJECT(name, obj) \
	REGISTER_FACTORY((IVehicleSystem*)pVehicleSystem, name, obj, false); \
	obj::m_objectId = pVehicleSystem->AssignVehicleObjectId(name);

	REGISTER_VEHICLEOBJECT("Burn", CVehicleDamageBehaviorBurn);
	REGISTER_VEHICLEOBJECT("CameraShake", CVehicleDamageBehaviorCameraShake);
	REGISTER_VEHICLEOBJECT("CollisionEx", CVehicleDamageBehaviorCollisionEx);
	REGISTER_VEHICLEOBJECT("Explosion", CVehicleDamageBehaviorExplosion);
	REGISTER_VEHICLEOBJECT("BlowTire", CVehicleDamageBehaviorBlowTire);
	REGISTER_VEHICLEOBJECT("AutomaticDoor", CVehicleActionAutomaticDoor);
	REGISTER_VEHICLEOBJECT("DeployRope", CVehicleActionDeployRope);
	REGISTER_VEHICLEOBJECT("EntityAttachment", CVehicleActionEntityAttachment);
	REGISTER_VEHICLEOBJECT("LandingGears", CVehicleActionLandingGears);
	REGISTER_VEHICLEOBJECT("SteerThirdPerson", CVehicleViewSteer);

	// vehicle movements
	REGISTER_FACTORY(pVehicleSystem, "DummyMovement", CVehicleMovementDummy, false);
	REGISTER_FACTORY(pVehicleSystem, "Hovercraft", CVehicleMovementHovercraft, false);
	REGISTER_FACTORY(pVehicleSystem, "Helicopter", CVehicleMovementHelicopter, false);
	REGISTER_FACTORY(pVehicleSystem, "StdBoat", CVehicleMovementStdBoat, false);
	REGISTER_FACTORY(pVehicleSystem, "StdWheeled", CVehicleMovementStdWheeled, false);
	REGISTER_FACTORY(pVehicleSystem, "ArcadeWheeled", CVehicleMovementArcadeWheeled, false);
	REGISTER_FACTORY(pVehicleSystem, "Tank", CVehicleMovementTank, false);
	REGISTER_FACTORY(pVehicleSystem, "VTOL", CVehicleMovementVTOL, false);
	REGISTER_FACTORY(pVehicleSystem, "Amphibious", CVehicleMovementAmphibious, false);
	REGISTER_FACTORY(pVehicleSystem, "AmphibiousArcade", CVehicleMovementAmphibiousAracde, false);

#ifndef SP_DEMO
	//aliens
	REGISTER_FACTORY(pFramework, "AlienPlayer", CAlien, false);
	REGISTER_FACTORY(pFramework, "Aliens/Alien", CAlien, true);
	//  REGISTER_FACTORY(pFramework, "Aliens/Observer", CObserver, true);
	REGISTER_FACTORY(pFramework, "Aliens/Trooper", CTrooper, true);
	REGISTER_FACTORY(pFramework, "Aliens/Scout", CScout, true);
	REGISTER_FACTORY(pFramework, "Aliens/Hunter", CHunter, true);
	REGISTER_FACTORY(pFramework, "Misc/Shark", CShark, true);
	//	REGISTER_FACTORY(m_pFramework, "Aliens/Warrior", CDrone, true);
	//REGISTER_FACTORY(pFramework, "Aliens/Coordinator", CObserver, true);
#else
	REGISTER_FACTORY(pFramework, "Aliens/Scout", CScout, true);
	REGISTER_FACTORY(pFramework, "Aliens/Trooper", CTrooper, true);
#endif

	// Custom GameObjects
	REGISTER_GAME_OBJECT(pFramework, Tornado, "Scripts/Entities/Environment/Tornado.lua");
	REGISTER_GAME_OBJECT(pFramework, AlienMine, "Scripts/Entities/Environment/AlienMine.lua");
	REGISTER_GAME_OBJECT(pFramework, Shake, "Scripts/Entities/Environment/Shake.lua");
	REGISTER_GAME_OBJECT(pFramework, InteractiveObjectEx, "Scripts/Entities/PlayerInteractive/InteractiveObjectEx.lua");
	REGISTER_GAME_OBJECT(pFramework, AlienDropPod, "Scripts/Entities/Others/AlienDropPod.lua");

	// Custom Extensions
	//REGISTER_GAME_OBJECT(pFramework, CustomFreezing);
	//REGISTER_GAME_OBJECT(pFramework, CustomShatter);

	REGISTER_GAME_OBJECT(pFramework, BattleEvent, "");
	HIDE_FROM_EDITOR("BattleEvent");

	//GameRules
	REGISTER_FACTORY(pFramework, "GameRules", CGameRules, false);

#if !USE_OLD_GAMERULES

	IGameRulesModulesManager *pGameRulesModulesManager = CGameRulesModulesManager::GetInstance();

	REGISTER_FACTORY(pGameRulesModulesManager, "StandardTwoTeams", CGameRulesStandardTwoTeams, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "StandardState", CGameRulesStandardState, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "StandardVictoryConditionsTeam", CGameRulesStandardVictoryConditionsTeam, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "StandardVictoryConditionsPlayer", CGameRulesStandardVictoryConditionsPlayer, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "ObjectiveVictoryConditionsTeam", CGameRulesObjectiveVictoryConditionsTeam, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "SurvivorOneVictoryConditions", CGameRulesSurvivorOneVictoryConditions, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "StandardSetup", CGameRulesStandardSetup, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "StandardScoring", CGameRulesStandardScoring, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "AssistScoring", CGameRulesAssistScoring, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "StandardPlayerStats", CGameRulesStandardPlayerStats, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "SpawningBase", CGameRulesSpawningBase, false);
	//REGISTER_FACTORY(pGameRulesModulesManager, "MPSpawning", CGameRulesMPSpawning, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "MPHQFrontlineSpawning", CGameRulesMPHQFrontlineSpawning, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "MPSimpleSpawning", CGameRulesMPSimpleSpawning, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "StandardStatsRecording", CGameRulesStatsRecording, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "MPSpawningWithLives", CGameRulesMPSpawningWithLives, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "MPDamageHandling", CGameRulesMPDamageHandling, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "MPActorAction", CGameRulesMPActorAction, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "MPSpectator", CGameRulesMPSpectator, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "ProgressiveObjectives", CGameRulesProgressiveObjectives, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "LuaObjectives", CGameRulesLuaObjectives, false);
	REGISTER_FACTORY(pGameRulesModulesManager, "StandardRounds", CGameRulesStandardRounds, false);



	pGameRulesModulesManager->Init();


#else


	REGISTER_GAME_OBJECT_EXTENSION(pFramework, ScriptControlledPhysics);

#ifndef CRYSIS_BETA
	pFramework->GetIGameRulesSystem()->RegisterGameRules("SinglePlayer", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("SinglePlayer", "sp");

#ifndef SP_DEMO
	pFramework->GetIGameRulesSystem()->RegisterGameRules("InstantAction", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("InstantAction", "ia");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("InstantAction", "dm");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("InstantAction", "multiplayer/ia/");

	pFramework->GetIGameRulesSystem()->RegisterGameRules("TeamInstantAction", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("TeamInstantAction", "tia");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("TeamInstantAction", "tdm");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("TeamInstantAction", "multiplayer/tia/");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("TeamInstantAction", "multiplayer/ia/");
	pFramework->GetIGameRulesSystem()->RegisterGameRules("CaptureTheFlag", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("CaptureTheFlag", "ctf");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("CaptureTheFlag", "multiplayer/ctf/");
#endif //spdemo

	pFramework->GetIGameRulesSystem()->RegisterGameRules("Coop", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("Coop", "coop");

	//pFramework->GetIGameRulesSystem()->RegisterGameRules("TeamAction", "GameRules");
#endif //crysis_beta

#ifndef SP_DEMO
	pFramework->GetIGameRulesSystem()->RegisterGameRules("PowerStruggle", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("PowerStruggle", "ps");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("PowerStruggle", "multiplayer/ps/");

	pFramework->GetIGameRulesSystem()->RegisterGameRules("AllOrNothing", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("AllOrNothing", "aon");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("AllOrNothing", "multiplayer/ia/");
	
	pFramework->GetIGameRulesSystem()->RegisterGameRules("Survival", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("Survival", "sv");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("Survival", "multiplayer/sv/");
	
	pFramework->GetIGameRulesSystem()->RegisterGameRules("Assault", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("Assault", "as");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("Assault", "multiplayer/as/");

	pFramework->GetIGameRulesSystem()->RegisterGameRules("BombTheBase", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("BombTheBase", "btb");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("BombTheBase", "multiplayer/btb/");

	pFramework->GetIGameRulesSystem()->RegisterGameRules("CrashSite", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("CrashSite", "cs");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("CrashSite", "multiplayer/cs/");

	pFramework->GetIGameRulesSystem()->RegisterGameRules("Race", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("Race", "rc");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("Race", "multiplayer/rc/");

	pFramework->GetIGameRulesSystem()->RegisterGameRules("Extraction", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("Extraction", "ext");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("Extraction", "multiplayer/ext/");

	pFramework->GetIGameRulesSystem()->RegisterGameRules("PowerStruggleLite", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("PowerStruggleLite", "psl");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("PowerStruggleLite", "multiplayer/psl/");

	pFramework->GetIGameRulesSystem()->RegisterGameRules("Countdown", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("Countdown", "cd");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("Countdown", "multiplayer/cd/");

	pFramework->GetIGameRulesSystem()->RegisterGameRules("Payload", "GameRules");
	pFramework->GetIGameRulesSystem()->AddGameRulesAlias("Payload", "pl");
	pFramework->GetIGameRulesSystem()->AddGameRulesLevelLocation("Payload", "multiplayer/pl/");
#endif //spdemo
#endif // USE_OLD_GAMERULES

}
