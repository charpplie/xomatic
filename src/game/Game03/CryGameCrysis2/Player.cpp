/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 29:9:2004: Created by Filippo De Luca

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include "GameActions.h"
#include "Player.h"
#include "GameUtils.h"
#include "PlayerHologram.h"

#include "Weapon.h"
#include "WeaponSystem.h"
#include "GameRules.h"

#include <IViewSystem.h>
#include <IItemSystem.h>
#include <IPhysics.h>
#include <ICryAnimation.h>
#include "IAISystem.h"
#include "IAgent.h"
#include <IVehicleSystem.h>
#include <ISerialize.h>
#include <ISound.h>
#include "IMaterialEffects.h"

#include <IRenderAuxGeom.h>
#include <IWorldQuery.h>

#include <IGameTokens.h>

#include <IDebugHistory.h>

#include <IMusicSystem.h>
#include <StringUtils.h>

#include <IInteractor.h>


#include "PlayerMovementController.h"
#include "PerkSonarVision.h"
#include "PerkIconData.h"
#include "PlayerNavPath.h"
#include "PlayerCamera.h"


#include "HUD/HUD.h"
#include "HUD/HUD_Impl.h"
#include "HUD/UI/UISimpleBar.h"
#include "HUD/UI/UICoverEntireScreen.h"

#include "PlayerMovement.h"
#include "PlayerRotation.h"
#include "PlayerInput.h"
#include "NetPlayerInput.h"
#include "AIDemoInput.h"

#include "CryCharAnimationParams.h"

#include "VehicleClient.h"

#include "HitDeathReactions.h"

#include "Network/VoiceListener.h"
#include "Binocular.h"

#include "NanoModeArmor.h"

#include "ScreenEffects.h"
#include "TeamPerks.h"
#include "Utility/StringUtils.h"
#include "PerkDbgDisplay.h"
#include "Utility/CryWatch.h"
#include "SShootHelper.h"
#include "SatelliteStrike.h"
#include "Environment/LedgeManager.h"
#include "EquipmentLoadout.h"

#include "GameRulesModules/IGameRulesPlayerStatsModule.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"
#include "GameRulesModules/IGameRulesAssistScoringModule.h"
#include "GameRulesModules/IGameRulesDamageHandlingModule.h"
#include "GameRulesModules/IGameRulesSpectatorModule.h"
#include "GameRulesModules/IGameRulesStateModule.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#include "INetwork.h"
#include "RecordingSystem.h"
#include "GodMode.h"
#include "PlayerProgression.h"
#include "Battlechatter.h"

#include "PlayerPlugin_Interaction.h"
#include "Environment/InteractiveObjectRegistry.h"

#include "BodyDamage.h"
#include "IAIActor.h"
#include "PickAndThrowWeapon.h"

#include "ArmsAimingController.h"

#define FOOTSTEPS_DEEPWATER_DEPTH 1  // meters
#define raycast raycast_player

#define USE_VOICELISTENER 0

#define JUMP_COUNTER_MAX 8

// enable this to check nan's on position updates... useful for debugging some weird crashes
#if !defined(_RELEASE)
#define ENABLE_NAN_CHECK
#endif

#ifdef ENABLE_NAN_CHECK
#define CHECKQNAN_FLT(x) \
	assert(((*(unsigned*)&(x))&0xff000000) != 0xff000000u && (*(unsigned*)&(x) != 0x7fc00000))
#else
#define CHECKQNAN_FLT(x) (void*)0
#endif

#define CHECKQNAN_VEC(v) \
	CHECKQNAN_FLT(v.x); CHECKQNAN_FLT(v.y); CHECKQNAN_FLT(v.z)

#define REUSE_VECTOR(table, name, value)	\
	{ if (table->GetValueType(name) != svtObject) \
	{ \
	table->SetValue(name, (value)); \
	} \
		else \
	{ \
	SmartScriptTable v; \
	table->GetValue(name, v); \
	v->SetValue("x", (value).x); \
	v->SetValue("y", (value).y); \
	v->SetValue("z", (value).z); \
	} \
	}

#define RANDOM() ((((float)cry_rand()/(float)RAND_MAX)*2.0f)-1.0f)
#define RANDOMR(a,b) ((float)a + ((cry_rand()/(float)RAND_MAX)*(float)(b-a)))

#undef CALL_PLAYER_EVENT_LISTENERS
#define CALL_PLAYER_EVENT_LISTENERS(func) \
{ \
	if (m_playerEventListeners.empty() == false) \
	{ \
		CPlayer::TPlayerEventListeners::const_iterator iter = m_playerEventListeners.begin(); \
		CPlayer::TPlayerEventListeners::const_iterator cur; \
		while (iter != m_playerEventListeners.end()) \
		{ \
			cur = iter; \
			++iter; \
			(*cur)->func; \
		} \
	} \
}

#ifndef _RELEASE
static int sHideArms = 0;
#endif
//--------------------
//this function will be called from the engine at the right time, since bones editing must be placed at the right time.
int PlayerProcessBones(ICharacterInstance *pCharacter,void *pPlayer)
{
//	return 1; //freezing and bone processing is not working very well.

	//FIXME: do something to remove gEnv->pTimer->GetFrameTime()
	//process bones specific stuff (IK, torso rotation, etc)
	float timeFrame = gEnv->pTimer->GetFrameTime();

	ISkeletonAnim* pISkeletonAnim = pCharacter->GetISkeletonAnim();
	uint32 numAnim = pISkeletonAnim->GetNumAnimsInFIFO(0);
	if (numAnim)
		((CPlayer *)pPlayer)->ProcessBonesRotation(pCharacter, timeFrame);

	return 1;
}
//--------------------

#define SONAR_VISION_MOVEMENT_SYMBOL_PARTICLES	"perk_fx.sonar_hearing.sonar_movement_symbol"
#define SONAR_VISION_MOVEMENT_SMOKE_PARTICLES		"perk_fx.sonar_hearing.sonar_movement_smoke"
#define CLOAK_AWARENESS_PARTICLES								"perk_fx.sonar_hearing.cloak_awareness"

#define INTERVAL_BREATHING 5  // seconds

static bool DoCoverAndLean()
{
	return g_pGameCVars->g_MPCoverAndLean || !gEnv->bMultiplayer;
}

// TODO: Probably best to move this into CPlayerPlugin_Interaction when refactoring continues [TF]
CPlayer::SInteractionInfo CPlayer::s_clientInteractionInfo;

CPlayer::CPlayer()
: m_pLocalPlayerInteractionPlugin(NULL)
{
	m_triggerSniperCountermeasuresPlugin.SetOwnerPlayer(this);
	m_currentlyTargettingPlugin.SetOwnerPlayer(this);
	m_scoreRewardsPlugin.SetOwnerPlayer(this);

	m_pInteractor = 0;

	m_sprintTimer = 0.0f;
	m_bHasAssistance = false;
	m_timedemo = false;
	m_ignoreRecoil = false;

	m_bDemoModeSpectator = false;

	m_playerCamera = NULL;
	m_pNanoSuit = NULL;
	m_pNavPath = NULL;

	m_pVehicleClient = 0;

	m_pVoiceListener = NULL;

	m_lastRequestedVelocity.Set(0.0f,0.0f,0.0f);
	m_forcedLookDir.Set(0.0f,0.0f,0.0f);

	m_stickySurfaceTimer = 0.0f;

	m_sufferingHighLatency = false;

	m_viewAngles = Ang3(0.0f, 0.0f, 0.0f);
	m_viewQuatFinal.SetIdentity();
	m_baseQuat.SetIdentity();
	m_eyeOffset.Set(0,0,0);
	m_weaponOffset.Set(0,0,0);
	m_bobOffset.Set(0, 0, 0);
	m_bobCycle = 0;

	m_lastTorsoAimIKDir.Set(0.0f, 0.0f, 0.0f);
	m_lastTorsoAimIKPos.Set(0.0f, 0.0f, 0.0f);

	m_pDebugHistoryManager = NULL;
	m_pSoundProxy = 0;
	m_fLastEffectFootStepTime = 0.f;

	m_vehicleViewDir.Set(0,1,0);

	m_flashbangSound = INVALID_SOUNDID;
	m_isSuperJumping = false;

	m_healthAccumError = 0.0f;
	m_timeOfLastHealthSync = 0.0f;

	m_ragdollTime = 0.0f;
	
	m_standUpTimer = 0.f;

	m_footstepCounter = 0;
	m_timeForBreath = 0;

	for(int i = 0; i < ePerk_Last; i++)
	{
		m_perkInstances[i] = CPerkChoice::AllocateInstance(this, (EPerks) i);
	}

	m_numActivePlayerPlugins = 0;
	memset (m_activePlayerPlugins, 0, sizeof(m_activePlayerPlugins));

	ResetPerksVariables();

#if ENABLE_MP_SPAWNMODES
	m_spawnmode_type=ESMT_Frontline;	// this will be set by the profile and remembered between sessions - but don't want to do it for EVERY player only the local one
#endif
	
	m_mountedGunController.InitWithPlayer(this);
	m_interactiveActionController.InitWithPlayer(this);
	
	// init sound table
	struct 
	{
		EPlayerSounds soundID;
		const char* signalName;
		bool repeated;
	} tmpSoundTable[ESound_Player_Last] = 
	{
		{ ESound_Run, "PlayerFeedback_Run", true },
		{ ESound_StopRun, "PlayerFeedback_StopRun", false },
		{ ESound_Jump, "PlayerFeedback_Jump", false },
		{ ESound_Fall_Drop, "PlayerFeedback_Fall", false },
		{ ESound_Melee, "PlayerFeedback_Melee", false },
		{ ESound_Fear, "PlayerFeedback_AlienFear", true },
		{ ESound_Hit_Wall, "PlayerFeedback_HitWall", false },
		{ ESound_Breathing, "PlayerFeedback_Breathing", true },
		{ ESound_Breathing_Water, "PlayerFeedback_BreathingWater", true },
		{ ESound_Gear_Water, "PlayerFeedback_GearWater", true },
		{ ESound_Underwater, "PlayerFeedback_UnderwaterAmbience", true },
		{ ESound_Drowning, "PlayerFeedback_Drowning", false },
		{ ESound_DiveIn, "PlayerFeedback_DiveIn", false },
		{ ESound_DiveOut, "PlayerFeedback_DiveOut", false },
		{ ESound_Player_Last, "", false }  // need to always be the last element
	};

	for (int i=0; tmpSoundTable[i].soundID!=ESound_Player_Last; ++i)
	{
		SSound& sound = m_sounds[tmpSoundTable[i].soundID];
		sound.audioSignalPlayer.SetSignal( tmpSoundTable[i].signalName );
		sound.isRepeated = tmpSoundTable[i].repeated;
	}

	m_gameRulesSpeedMult = 1.f;
	m_localPerksSet = false;
	m_jumpCounter = 0;
	m_rank = -1;
	
#if DEFERRED_RAY_CAST_BOTTOM
	m_bottomLevel = BOTTOM_LEVEL_UNKNOWN;
	m_raycastHelper.SetReceiver(this);
#endif
}

CPlayer::~CPlayer()
{
	SAFE_DELETE(m_playerCamera);

	SAFE_RELEASE(m_pDebugHistoryManager);

	if(gEnv->bMultiplayer)
	{
		m_cctvScreenEffect.Release();
	}
	m_hitRecoilGameEffect.Release();

	LeaveAllPlayerPlugins();
	SAFE_DELETE(m_pLocalPlayerInteractionPlugin);

	for(int i = 0; i < ePerk_Last; i++)
	{
		delete m_perkInstances[i];
		m_perkInstances[i] = NULL;
	}

	StopLoopingSounds();

	if(CActor::IsClient())
	{
		ResetScreenFX();

		SAFE_HUD_FUNC(PlayerIdSet(0));
	}

#if ENABLE_MINDCONTROL
	m_pMindControlMaster.reset();
	m_pMindControlSlave.reset();
#endif

	m_pPlayerInput.reset();
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
	if(pCharacter)
		pCharacter->GetISkeletonPose()->SetPostProcessCallback(0,0);
	if(m_pNanoSuit)
	{
		UnregisterPlayerSuitEventListener(this);
		delete m_pNanoSuit;
	}

	m_pVehicleClient = NULL;
  
	if(m_pVoiceListener)
		GetGameObject()->ReleaseExtension("VoiceListener");
	if (m_pInteractor)
		GetGameObject()->ReleaseExtension("Interactor");

	CGameRules *pGameRules = g_pGame->GetGameRules();
	IGameRulesPlayerStatsModule *playerStats = pGameRules ? pGameRules->GetPlayerStatsModule() : NULL;
	if (playerStats)
		playerStats->RemovePlayerStats(GetEntityId());

	IGameRulesSpawningModule *spawningModule = pGameRules ? pGameRules->GetSpawningModule() : NULL;
	if (spawningModule)
	{
		spawningModule->PlayerLeft(GetEntityId());
	}

	if (gEnv->bServer)
	{
		IGameRulesAssistScoringModule *assistScoringModule = pGameRules ? pGameRules->GetAssistScoringModule() : NULL;
		if (assistScoringModule)
		{
			assistScoringModule->SvPlayerLeft(GetEntityId());
		}
	}
}

bool CPlayer::Init(IGameObject * pGameObject)
{
	CCCPOINT(PlayerState_Init);

#if ENABLE_PLAYER_HEALTH_REDUCTION_POPUPS
	m_healthAtStartOfUpdate = -1.f;
#endif

	if (!CActor::Init(pGameObject) || !m_pAnimatedCharacter)
		return false;

#if ENABLE_PLAYER_MODIFIABLE_VALUES_DEBUGGING
	m_modifiableValues.DbgInit(GetEntity());
#endif

	m_stealthKill.Init(this);

	m_pAnimatedCharacter->SetAnimationPlayerProxy(&m_animationProxy, 0);
	m_pAnimatedCharacter->SetAnimationPlayerProxy(&m_animationProxyUpper, 1);

	ReadDataFromXML();

	IVehicleSystem* pVehicleSystem = g_pGame->GetIGameFramework()->GetIVehicleSystem();
	IVehicleClient *pVehicleClient = pVehicleSystem->GetVehicleClient();
	m_pVehicleClient = static_cast<CVehicleClient*>(pVehicleClient);
	assert(m_pVehicleClient);

	if(IEntityRenderProxy* pProxy = (IEntityRenderProxy*)GetEntity()->GetProxy(ENTITY_PROXY_RENDER))
	{
		if(IRenderNode* pRenderNode = pProxy->GetRenderNode())
			pRenderNode->SetRndFlags(ERF_REGISTER_BY_POSITION,true);
	}

	EntityId entityId = GetEntityId();

	CGameRules *pGameRules = g_pGame->GetGameRules();
	IGameRulesPlayerStatsModule *playerStats = pGameRules ? pGameRules->GetPlayerStatsModule() : NULL;
	if (playerStats)
		playerStats->CreatePlayerStats(entityId);

	IGameRulesSpawningModule *spawningModule = pGameRules ? pGameRules->GetSpawningModule() : NULL;
	if (spawningModule)
	{
		spawningModule->PlayerJoined(entityId);
	}


	CRY_ASSERT_MESSAGE(spawningModule, "CPlayer::Init() failed to find required gamerules spawning module");

	if (gEnv->bServer)
	{
		IGameRulesAssistScoringModule *assistScoringModule = pGameRules ? pGameRules->GetAssistScoringModule() : NULL;
		if (assistScoringModule)
		{
			assistScoringModule->SvPlayerJoined(entityId);
		}
	}
	
	if(gEnv->bMultiplayer)
	{
		EnterPlayerPlugin(&m_currentlyTargettingPlugin);		
	}

	return true;
}

void CPlayer::PostInit( IGameObject * pGameObject )
{
	CCCPOINT(PlayerState_PostInit);

	CActor::PostInit(pGameObject);

	//--- Update animationPlayerProxies & toggle the part visibility for separate character shadow casting
	m_animationProxy.SetFirstPerson(!m_stats.isThirdPerson);
	m_animationProxyUpper.SetFirstPerson(!m_stats.isThirdPerson);
	UpdateVisibility();

	ResetAnimGraph();

	if (gEnv->bMultiplayer && !gEnv->bServer)
	{
		GetGameObject()->SetUpdateSlotEnableCondition( this, 0, eUEC_VisibleOrInRange );
		//GetGameObject()->ForceUpdateExtension(this, 0);
	}

	if (IsPlayer() || gEnv->bMultiplayer)
	{
		SetUpInventorySlotsAndCategories();
	}

	// Register for things like tagnames.
	if( IsPlayer() )
	{
		SHUDEvent hudEvent_remoteEnterGame(eHUDEvent_OnEnterGame_RemotePlayer);
		hudEvent_remoteEnterGame.AddData(static_cast<int>(GetEntityId()));
		CHUD::CallEvent(hudEvent_remoteEnterGame);

		if( !gEnv->bServer )
		{
			// Clients only know migrating players have arrived when the player is initialised, unfortunately
			// this means we get events for non-migrating players too but since its a rare event and nothing 
			// will happen it's not really an issue
			SHUDEvent hostMigrationOnNewPlayer(eHUDEvent_HostMigrationOnNewPlayer);
			hostMigrationOnNewPlayer.m_data.push_back(SHUDEventData(int(GetEntityId())));
			CHUD::CallEvent(hostMigrationOnNewPlayer);
		}
	}
}

void CPlayer::ReloadClientXmlData()
{
	CRY_ASSERT_MESSAGE(IsClient(), "This function should be called only for the client!");

	ReadDataFromXML(true);

	CNanoSuit::ReloadSuitData();

	g_pGame->GetInteractiveObjectsRegistry().Reload();
}


void CPlayer::ReadDataFromXML(bool isReloading /*= false*/)
{
	const IItemParamsNode* pParamNode = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActorParams(GetActorClass());
	if (!pParamNode)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Could not find actor params xml for the player.");
		return;
	}

	if(gEnv->bMultiplayer)
	{
		InitCCTVScreenEffect(pParamNode);
	}

	if(DoCoverAndLean())
	{
		m_coverAndLean.Reset(pParamNode);
	}
	
	m_playerRotationParams.Reset(pParamNode);

	SHitRecoilGameEffectParams hitRecoilParams;
	m_hitRecoilGameEffect.Initialise(&hitRecoilParams);
	m_hitRecoilGameEffect.Reset(pParamNode);

	m_ledgeGrabbingParams.SetParamsFromXml(pParamNode);

	m_stealthKill.ReadXmlData(pParamNode, isReloading);
}


void CPlayer::InitClient(int channelId )
{
	CActor::InitClient(channelId);
}

void CPlayer::InitLocalPlayer()
{
	GetGameObject()->SetUpdateSlotEnableCondition( this, 0, eUEC_WithoutAI );

	if (!(gEnv->bServer && gEnv->bHostMigrating))
	{
		CRY_ASSERT(!m_pLocalPlayerInteractionPlugin);
		if(!m_pLocalPlayerInteractionPlugin)
		{
			m_pLocalPlayerInteractionPlugin = new CPlayerPlugin_Interaction();
		}
		CRY_ASSERT(m_pLocalPlayerInteractionPlugin);

		m_pLocalPlayerInteractionPlugin->SetOwnerPlayer(this);
		EnterPlayerPlugin(m_pLocalPlayerInteractionPlugin);

		InitNanoSuit();
	}

	m_pNavPath = new CPlayerNavPath(*this);
	CRY_ASSERT(m_pNavPath);

	if( !gEnv->pSystem->IsDedicated() )
	{
		assert( GetEntityId() == gEnv->pGame->GetIGameFramework()->GetClientActor()->GetEntityId() );
		// Notify HUD that NanoSuit and player are ready (setup listeners).
		SHUDEvent hudEvent_initLocalPlayer(eHUDEvent_OnInitPlayer);
		hudEvent_initLocalPlayer.AddData(static_cast<int>(GetEntityId()));
		CHUD::CallEvent(hudEvent_initLocalPlayer);
	}

	m_pSoundProxy = (IEntitySoundProxy*)(GetEntity()->GetProxy(ENTITY_PROXY_SOUND));

	if (!m_pSoundProxy)
	{
		m_pSoundProxy = (IEntitySoundProxy*)GetEntity()->CreateProxy(ENTITY_PROXY_SOUND);
	}
	m_pSoundProxy->SetFlags(m_pSoundProxy->GetFlags()|IEntitySoundProxy::FLAG_DELEGATE_SOUND_ANIM_EVENTS);

	GetRank();	//Sets rank for local player and serialized if it needs to

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if(pGameRules)
	{
		CBattlechatter* pBattlechatter = pGameRules->GetBattlechatter();
		if(pBattlechatter)
		{
			pBattlechatter->SetLocalPlayer(this);
		}

		if (gEnv->bHostMigrating)
		{
			pGameRules->OnHostMigrationGotLocalPlayer(this);
		}

		pGameRules->OwnClientConnected_NotifyListeners();
	}

	CRY_ASSERT_TRACE (m_playerCamera == NULL, ("%s '%s' already has a camera instance when allocating another = memory leak!", GetEntity()->GetClass()->GetName(), GetEntity()->GetName()));
	m_playerCamera = new CPlayerCamera(*this);
}

void CPlayer::BindInputs( IAnimationGraphState * pAGState )
{
	CActor::BindInputs(pAGState);

	if (pAGState)
	{
		m_inputAction = pAGState->GetInputId("Action");
		m_inputItem = pAGState->GetInputId("Item");
		m_inputUsingLookIK = pAGState->GetInputId("UsingLookIK");
		m_inputAiming = pAGState->GetInputId("Aiming");
		m_inputVehicleName = pAGState->GetInputId("Vehicle");
		m_inputVehicleSeat = pAGState->GetInputId("VehicleSeat");
		m_inputSuitMode = pAGState->GetInputId("SuitMode");
		m_inputWaterLevel = pAGState->GetInputId("WaterLevel");
		m_varInputAimMode = pAGState->GetVariationInputId("AimMode");
	}

	ResetAnimGraph();
}

void CPlayer::ResetAnimGraph()
{
	if (m_pAnimatedCharacter)
	{
		IAnimationGraphState* pAGState = m_pAnimatedCharacter->GetAnimationGraphState();
		if (pAGState != NULL)
		{
			pAGState->SetInput( m_inputItem, "nw" );
			m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( m_inputSignal, "none" );
			m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( m_inputVehicleName, "none" );
			m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( m_inputVehicleSeat, 0 );
			m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( m_inputWaterLevel, "Surface" );
			m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( m_inputSuitMode, "tac" );
		}
		
		m_pAnimatedCharacter->SetParams( m_pAnimatedCharacter->GetParams().ModifyFlags( eACF_ImmediateStance, 0 ) );
	}

	SetStance(STANCE_STAND);
}


void CPlayer::NotifyAnimGraphTransition(const char *name)
{
	CRY_ASSERT(gEnv->bServer);
	GetGameObject()->InvokeRMI(ClAnimGraphTransition(), AnimGraphTransitionParams(name), eRMI_ToRemoteClients);
}

void CPlayer::NotifyAnimGraphInput(int id, const char *value)
{
	CRY_ASSERT(gEnv->bServer);
	GetGameObject()->InvokeRMI(ClAnimGraphInput(), AnimGraphInputParams(id, value), eRMI_ToRemoteClients);
}

void CPlayer::NotifyAnimGraphInput(int id, int value)
{
	CRY_ASSERT(gEnv->bServer);
	GetGameObject()->InvokeRMI(ClAnimGraphInput(), AnimGraphInputParams(id, value), eRMI_ToRemoteClients);
}


void CPlayer::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_HIDE || event.event == ENTITY_EVENT_UNHIDE)
	{
		//
	}
	else if (event.event == ENTITY_EVENT_INVISIBLE || event.event == ENTITY_EVENT_VISIBLE)
	{
		//
	}	
	else if (event.event == ENTITY_EVENT_XFORM)
	{
		/*if(gEnv->bMultiplayer)
		{
			// if our local player is spectating this one, move it to this position
			CPlayer* pPlayer = (CPlayer*)gEnv->pGame->GetIGameFramework()->GetClientActor();
			if(pPlayer && pPlayer->GetSpectatorMode() == CPlayer::eASM_Follow && pPlayer->GetSpectatorTarget() == GetEntityId())
			{
				// local player is spectating us. Move them to our position
				pPlayer->MoveToSpectatorTargetPosition();
			}
		}*/

		int flags = (int)event.nParam[0];
		if (flags & ENTITY_XFORM_ROT && !(flags & (ENTITY_XFORM_USER|ENTITY_XFORM_PHYSICS_STEP)))
		{
			if (flags & (ENTITY_XFORM_TRACKVIEW|ENTITY_XFORM_EDITOR|ENTITY_XFORM_TIMEDEMO))
				m_forcedRotation = true;
			else
				m_forcedRotation = false;

			Quat rotation = GetEntity()->GetRotation();

			if ((m_linkStats.linkID == 0) || ((m_linkStats.flags & LINKED_FREELOOK) == 0))
				{
					m_linkStats.viewQuatLinked = m_linkStats.baseQuatLinked = rotation;
					m_viewQuatFinal = m_viewQuat = m_baseQuat = rotation;
				}
			}

		if (m_timedemo && !(flags&ENTITY_XFORM_TIMEDEMO))
		{
			// Restore saved position.
			GetEntity()->SetPos(m_lastKnownPosition);
		}
		m_lastKnownPosition = GetEntity()->GetPos();
	}
	else if (event.event == ENTITY_EVENT_PREPHYSICSUPDATE)
	{
		if (m_pPlayerInput.get())
			m_pPlayerInput->PreUpdate();

		IEntityRenderProxy* pRenderProxy = (IEntityRenderProxy*)(GetEntity()->GetProxy(ENTITY_PROXY_RENDER));
		if ((pRenderProxy != NULL) && pRenderProxy->IsCharactersUpdatedBeforePhysics())
			PrePhysicsUpdate();
	}

	CActor::ProcessEvent(event);

	if (event.event == ENTITY_EVENT_PRE_SERIALIZE)
	{
		SEntityEvent event(ENTITY_EVENT_RESET);
		ProcessEvent(event);
	}

	if(event.event == ENTITY_EVENT_RESET)
	{
		if(gEnv->bEditor && event.nParam[0] && IsClient())
		{
			SNanoSuitEvent event;
			event.event = eNanoSuitEvent_SUIT_CLIENTSTARTUP;
			SendActorSuitEvent(event);

			assert( GetEntityId() == gEnv->pGame->GetIGameFramework()->GetClientActor()->GetEntityId() );

			SHUDEvent hudEvent_initPlayer(eHUDEvent_OnInitPlayer);
			hudEvent_initPlayer.AddData(static_cast<int>(GetEntityId()));
			CHUD::CallEvent(hudEvent_initPlayer);
			SHUDEvent hudEvent_remoteLeaveGame(eHUDEvent_OnLeaveGame_RemotePlayer);
			hudEvent_remoteLeaveGame.AddData(static_cast<int>(GetEntityId()));
			CHUD::CallEvent(hudEvent_remoteLeaveGame);
		}
	}

	if (event.event == ENTITY_EVENT_DONE)
	{
		//NB IsClient doesn't work at this point
		if (m_pLocalPlayerInteractionPlugin)
		{
			CGameRules *pGameRules = g_pGame->GetGameRules();
			if(pGameRules)
			{
				CBattlechatter* pBattlechatter = pGameRules->GetBattlechatter();
				if(pBattlechatter)
				{
					pBattlechatter->SetLocalPlayer(NULL);
				}
			}

			LeavePlayerPlugin(m_pLocalPlayerInteractionPlugin);
			m_pLocalPlayerInteractionPlugin->SetOwnerPlayer(NULL);
			CCCPOINT(PlayerState_LocalPlayerBeingDestroyed);
		}
		else
		{
			CCCPOINT(PlayerState_NonLocalPlayerBeingDestroyed);
			SHUDEvent hudEvent_remoteLeaveGame(eHUDEvent_OnLeaveGame_RemotePlayer);
			hudEvent_remoteLeaveGame.AddData(static_cast<int>(GetEntityId()));
			CHUD::CallEvent(hudEvent_remoteLeaveGame);
		}

		LeaveAllPlayerPlugins();
	}
}

void CPlayer::SetChannelId(uint16 id)
{
	CActor::SetChannelId(id);

	if (id && !(gEnv->bServer && IsMigrating()))
	{
		//foreign coop clients init nanosuit here
		InitNanoSuit();
	}
}

//////////////////////////////////////////////////////////////////////////
void CPlayer::SetViewRotation( const Quat &rotation )
{
	assert( rotation.IsValid() );
	m_viewQuat = rotation;
	m_viewQuatFinal = rotation;
	m_viewAngles.SetAnglesXYZ(rotation);
	m_baseQuat = Quat::CreateRotationZ(m_viewAngles.z);
}

//////////////////////////////////////////////////////////////////////////
Quat CPlayer::GetViewRotation() const
{
	return m_viewQuatFinal;
}

//////////////////////////////////////////////////////////////////////////
void CPlayer::EnableTimeDemo( bool bTimeDemo )
{
	m_viewAnglesOffset.Set(0,0,0);
	m_timedemo = bTimeDemo;
	m_ignoreRecoil = bTimeDemo;
}

//////////////////////////////////////////////////////////////////////////
void CPlayer::AddViewAngleOffsetForFrame(const Ang3 &offset)
{
	if (!m_ignoreRecoil)
		m_viewAnglesOffset += offset;
}

//////////////////////////////////////////////////////////////////////////
const Ang3& CPlayer::GetViewAngleOffset() const
{
	return m_viewAnglesOffset;
}

//////////////////////////////////////////////////////////////////////////
void CPlayer::Draw(bool draw)
{
	if (!GetEntity())
		return;


	uint32 slotFlags = GetEntity()->GetSlotFlags(0);

	if (draw)
		slotFlags |= ENTITY_SLOT_RENDER;
	else
		slotFlags &= ~ENTITY_SLOT_RENDER;

	GetEntity()->SetSlotFlags(0,slotFlags);
}


void CPlayer::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);


	bool client(IsClient());

#if ENABLE_PLAYER_MODIFIABLE_VALUES_DEBUGGING
	m_modifiableValues.DbgTick();
#endif

#if ENABLE_PLAYER_HEALTH_REDUCTION_POPUPS
	if (g_pGameCVars->g_displayPlayerDamageTaken && !client && m_healthAtStartOfUpdate > 0.f && m_healthAtStartOfUpdate > m_health)
	{
		int damageTaken = (int)(0.5f + m_healthAtStartOfUpdate - max(0.f, m_health));
		if (damageTaken > 0)
		{
			Vec3 velocity(2.f * (cry_frand() - 0.5f), 2.f * (cry_frand() - 0.5f), (cry_frand() + 1.0f));
			CryWatch3DAdd(string().Format("%d", damageTaken), GetLocalEyePos() + GetEntity()->GetWorldPos(), 2.f, & velocity, 3.f);
		}
	}
	m_healthAtStartOfUpdate = m_health;
#endif

	IEntityRenderProxy* pRenderProxy = (IEntityRenderProxy*)(GetEntity()->GetProxy(ENTITY_PROXY_RENDER));
	if ((pRenderProxy == NULL) || !pRenderProxy->IsCharactersUpdatedBeforePhysics())
		PrePhysicsUpdate();

	IEntity* pEnt = GetEntity();
	if (pEnt->IsHidden() && !(pEnt->GetFlags() & ENTITY_FLAG_UPDATE_HIDDEN))
		return;

	if (gEnv->bServer && !client && IsPlayer())
	{
		if (INetChannel *pNetChannel=m_pGameFramework->GetNetChannel(GetChannelId()))
		{
			if (pNetChannel->GetContextViewState()>=eCVS_InGame)
			{
				SufferingHighLatency(pNetChannel->IsSufferingHighLatency(gEnv->pTimer->GetAsyncTime()));
			}
		}
	}

	if (m_stats.isStandingUp)
	{
		m_actions=0;
		m_actionFlags=eAF_NONE;
		
		float fNewStandUpTimer = m_standUpTimer - ctx.fFrameTime;
		
		m_stats.isStandingUp = fNewStandUpTimer > 0.0f;

		m_standUpTimer = fNewStandUpTimer;
	}

	CActor::Update(ctx,updateSlot);

	const float frameTime = ctx.fFrameTime;

	CRY_TODO(20,11,2009,"StanF! Check out the multiplayer physics auto-disable, currently set to never turn off should this be so?");
	
	//Why is this being done every frame? 
	EAutoDisablePhysicsMode adpm = eADPM_Never;
	if (m_stats.isRagDoll)
		adpm = eADPM_Never;
	else if (gEnv->bMultiplayer)
		adpm = eADPM_Never;
	else if (IsPlayer())
		adpm = eADPM_WhenInvisibleAndFarAway;
	else
		adpm = eADPM_WhenAIDeactivated;
	GetGameObject()->SetAutoDisablePhysicsMode(adpm);

	int iHealth = GetHealth();

	if (!m_stats.isRagDoll && iHealth>0)
	{
		if(m_pNanoSuit)
		{
			m_pNanoSuit->Update(frameTime);
		}

		UpdateHealthRegeneration(iHealth, frameTime);
		UpdateStats(frameTime);
		UpdateBreathing(frameTime);

		if(m_stats.bSprinting)
		{
			if(m_sprintTimer != 0.0f)
				m_sprintTimer = gEnv->pTimer->GetFrameStartTime().GetSeconds();
			else if((m_stats.inWaterTimer <= 0.0f) && m_stance == STANCE_STAND && (gEnv->pTimer->GetFrameStartTime().GetSeconds() - m_sprintTimer > 3.0f))
				PlaySound(ESound_Run);
			else if(IsSoundPlaying(ESound_Run) && ((m_stats.headUnderWaterTimer > 0.0f) || m_stance == STANCE_CROUCH))
			{
				PlaySound(ESound_Run, false);
				m_sprintTimer = 0.0f;
			}
		}
		else 
		{
			if(IsSoundPlaying(ESound_Run))
			{
				PlaySound(ESound_Run, false);
				PlaySound(ESound_StopRun);
			}
			m_sprintTimer = 0.0f;
		}
	
		if(client)
		{
			UpdateInteractionType();
			UpdateFPAiming();
		}
	}
	else
	{
		WATCH_ACTOR_STATE ("Not updating (ragdoll=%d health=%d)", m_stats.isRagDoll, iHealth);
	}

	if (m_pPlayerInput.get())
	{
		m_pPlayerInput->Update();
	}
	else
	{
		CreateInputClass(client);
	}

	if (client)
	{
		// need to create this even when player is dead, otherwise spectators don't see tank turrets rotate etc until they spawn in.
		GetGameObject()->AttachDistanceChecker();
		UpdateClientHealthFX(m_health);
		CPlayerProgression::GetInstance()->Update(this, frameTime, iHealth);

	}

	// small hack for ded server: fake a view update
	/*
	if (gEnv->bMultiplayer && gEnv->bServer && !IsClient())
	{
		SViewParams viewParams;
		UpdateView(viewParams);
	}
	*/

	UpdateWeaponRaising();

	if(DoCoverAndLean() && IsPlayer())
	{
		m_coverAndLean.Update(this);
	}

	UpdatePerks(ctx);

	UpdateIPerks(frameTime);

	if (gEnv->bServer)
	{
		if (m_ragdollTime > 0.0f)
		{
			float fNewRagdollTime = m_ragdollTime - frameTime;

			if (fNewRagdollTime <= 0.0f)
			{
				if (GetGameObject()->GetAspectProfile(eEA_Physics) == eAP_Alive)
				{

					ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
					if (pCharacter)
					{
						pCharacter->GetISkeletonPose()->SetRagdollDefaultPose();
					}

					GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Ragdoll);
				}
				
				fNewRagdollTime = 0.0f;
			}

			m_ragdollTime = fNewRagdollTime;
		}
	}
	
	if (gEnv->bMultiplayer)
	{
		int health = GetHealth();
		float deathTime = GetDeathTime();
		float timeSinceDeath = gEnv->pTimer->GetFrameStartTime().GetSeconds() - deathTime;
		CRecordingSystem *crs = g_pGame->GetRecordingSystem();

		if (client && health <= 0)
		{
			if (crs && g_pGameCVars->g_killcamEnable && deathTime > 0.0f)
			{
				if ((timeSinceDeath > KILL_CAM_KICK_IN_TIME) && crs->IsStopped() && (GetSpectatorMode() == CActor::eASM_None))
				{
					CGameRules *pGameRules = g_pGame->GetGameRules();
					IGameRulesSpectatorModule *specmod = pGameRules->GetSpectatorModule();

					CryLog("RecordingSystem - replay stopped");

					IGameRulesStateModule*  stateModule = pGameRules->GetStateModule();
					if (!stateModule || (stateModule->GetGameState() != IGameRulesStateModule::EGRS_PostGame))  // ie. scoreboard HUD is not being displayed (don't want spectator HUD to override it!)
					{
						CHUD*  pHUD = g_pGame->GetHUD();
						pHUD->ActivateState("mp_spectator");
					}

					specmod->ChangeSpectatorModeBestAvailable(this, false);
				}
				else if (!crs->IsPlayingBack() && !crs->IsPlaybackQueued() && !crs->IsStopped())
				{
					if ((timeSinceDeath > KILL_CAM_KICK_IN_TIME) && (timeSinceDeath < (KILL_CAM_KICK_IN_TIME + 5.0f)))
					{
						if (m_lastKillParams.shooterId != 0 && m_lastKillParams.shooterId != GetEntityId())
						{
							CryLog("RecordingSystem - queue playback");
							crs->StartPlayback(m_lastKillParams.shooterId, m_lastKillParams.projectileId, m_lastKillParams.hit_type, m_lastKillParams.impulse);
						}
					}
					else if ((timeSinceDeath > KILL_CAM_KICK_IN_TIME + 5.0f) && (GetSpectatorMode() == CActor::eASM_None))
					{
						CGameRules *pGameRules = g_pGame->GetGameRules();
						IGameRulesSpectatorModule *specmod = pGameRules->GetSpectatorModule();

						CryLog("RecordingSystem - replay finished");

						CHUD *pHUD = g_pGame->GetHUD();
						pHUD->ActivateState("mp_spectator");

						specmod->ChangeSpectatorModeBestAvailable(this, false);
					}
				}
			}
			else if ((timeSinceDeath > KILL_CAM_KICK_IN_TIME) && (GetSpectatorMode() == CActor::eASM_None))
			{
				CGameRules *pGameRules = g_pGame->GetGameRules();
				IGameRulesSpectatorModule *specmod = pGameRules->GetSpectatorModule();

				CryLog("RecordingSystem - replay disabled");

				CHUD *pHUD = g_pGame->GetHUD();
				pHUD->ActivateState("mp_spectator");

				specmod->ChangeSpectatorModeBestAvailable(this, false);
			}
		}
	}

	{
		FRAME_PROFILER("Player Update :: UpdateListeners", GetISystem(), PROFILE_GAME);

		TPlayerUpdateListeners::iterator it = m_playerUpdateListeners.begin();
		TPlayerUpdateListeners::iterator end = m_playerUpdateListeners.end();
		for (; it!=end; ++it)
		{
			(*it)->Update(frameTime);
		}
	}

	if (m_pHitDeathReactions)
	{
		m_pHitDeathReactions->Update(frameTime);
	}
	else if (!IsPlayer() || g_pGameCVars->g_hitDeathReactions_enable)
	{
		m_pHitDeathReactions.reset(new CHitDeathReactions(*this));
		CRY_ASSERT(m_pHitDeathReactions);
	}

#ifndef _RELEASE
	if (g_pGameCVars->g_animatorDebug && IsClient())
	{
		ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
		IAttachmentManager* pAttachMan = pCharacter->GetIAttachmentManager();
		ISkeletonPose* pSkel = pCharacter->GetISkeletonPose();
		int32 jntID = pSkel->GetJointIDByName("Bip01 LHand2Weapon_IKBlend");
		IAttachment* pLeftAttachment = pAttachMan->GetInterfaceByName("left_weapon");
		IAttachment* pRightAttachment = pAttachMan->GetInterfaceByName("weapon");

		IRenderAuxGeom* pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();

		SAuxGeomRenderFlags currRenderFlags = pAuxGeom->GetRenderFlags();
		EAuxGeomPublicRenderflags_DepthTest depthTest = currRenderFlags.GetDepthTestFlag();

		currRenderFlags.SetDepthTestFlag(e_DepthTestOff);
		pAuxGeom->SetRenderFlags(currRenderFlags);

		if(pLeftAttachment)
		{
			QuatT leftQuatt = pLeftAttachment->GetAttWorldAbsolute();

			pAuxGeom->DrawSphere(leftQuatt.t, 0.02f, ColorB(0,0,255,255));
			pAuxGeom->DrawLine(leftQuatt.t, ColorB(0,0,255,255), leftQuatt.t + leftQuatt.GetColumn1(), ColorB(0,0,255,255));
		}

		if(pRightAttachment)
		{
			QuatT rightQuatt = pRightAttachment->GetAttWorldAbsolute();

			pAuxGeom->DrawSphere(rightQuatt.t, 0.02f, ColorB(255,0,0,255));
			pAuxGeom->DrawLine(rightQuatt.t, ColorB(255,0,0,255), rightQuatt.t + rightQuatt.GetColumn1(), ColorB(255,0,0,255));
		}

		if (jntID >= 0)
		{
			float blendFactor = pSkel->GetRelJointByID(jntID).t.x;
			volatile static float XPOS = 500.0f;
			volatile static float YPOS = 100.0f;
			float offColor[4] = { 0.2f, 0.2f, 0.2f, 1 };
			float onColor[4]  = { 0.0f, 1.0f, 0.0f, 1 };

			gEnv->pRenderer->Draw2dLabel(XPOS, YPOS, 2.5f, (blendFactor > 0.0001f) ? onColor : offColor, false, "LHand IK Blend %.2f", blendFactor);
		}

		currRenderFlags.SetDepthTestFlag(depthTest);
		pAuxGeom->SetRenderFlags(currRenderFlags);
	}

	if(g_pGameCVars->g_hideArms != sHideArms && IsClient())
	{
		ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
		IAttachmentManager* pAttachMan = pCharacter->GetIAttachmentManager();

		IAttachment* pHandsAttachment = pAttachMan->GetInterfaceByName("hands");
		IAttachment* pArmsAttachment = pAttachMan->GetInterfaceByName("arms_1p");

		pArmsAttachment->HideAttachment(g_pGameCVars->g_hideArms);
		pHandsAttachment->HideAttachment(g_pGameCVars->g_hideArms);

		sHideArms = g_pGameCVars->g_hideArms;
	}
	
	if (g_pGameCVars->pl_pickAndThrow.debugDraw!=0)
	{
		const char* const name = "PickAndThrowWeapon";
		IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass( name );
		EntityId pickAndThrowWeaponId = GetInventory()->GetItemByClass( pClass );
		IItem* pItem = m_pItemSystem->GetItem( pickAndThrowWeaponId );
		if (pItem)
		{
			CPickAndThrowWeapon* pPTW = static_cast<CPickAndThrowWeapon*>(pItem);
			pPTW->DebugDraw();
		}
	}

	if (g_pGameCVars->pl_debug_pickable_items != 0)
	{
		CItem* pItem = static_cast<CItem*>(g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(s_clientInteractionInfo.interactiveEntityId));
		CWeapon* pWeapon = pItem ? static_cast<CWeapon*>(pItem->GetIWeapon()) : NULL;
		if (pWeapon)
		{
			pWeapon->ShowDebugInfo();
		}
	}
#endif

}

void CPlayer::CreateInputClass(bool client)
{
	CCCPOINT (PlayerState_CreateInputClassDuringUpdate);

	// init input systems if required
	if (client) //|| ((demoMode == 2) && this == gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetOriginalDemoSpectator()))
	{
		if ( GetISystem()->IsDedicated() )
		{
			m_pPlayerInput.reset( new CDedicatedInput(this) );
		}
#if !defined(_RELEASE)
		else if (g_pGameCVars->g_playerUsesDedicatedInput)
		{
			m_pPlayerInput.reset( new CDedicatedInput(this) );
		}
#endif
		else
		{
			m_pPlayerInput.reset( new CPlayerInput(this) );
		}
	}
	else if (GetGameObject()->GetChannelId())
	{
		m_pPlayerInput.reset( new CNetPlayerInput(this) );
	}
	else
	{
		if (gEnv->bServer)
			m_pPlayerInput.reset( new CAIInput(this) );
		else
			m_pPlayerInput.reset( new CNetPlayerInput(this) );
	}

	if (m_pPlayerInput.get())
	{
		GetGameObject()->EnablePostUpdates(this);

		if(m_pPlayerInput->GetType() == IPlayerInput::NETPLAYER_INPUT && m_recvPlayerInput.isDirty)
		{
			m_pPlayerInput->SetState(m_recvPlayerInput);
			m_recvPlayerInput.isDirty = false;
		}
	}
}


//-------------------------------------------------------------------
void CPlayer::UpdateWeaponRaising()
{
	uint8 pose = (uint8)eWeaponRaisedPose_None;

/*	EntityId currentItem = GetCurrentItemId(true);
	CWeapon* pWeapon = GetWeapon(currentItem);
	if (pWeapon != NULL)
	{
		bool raised = false;
		if (pWeapon->IsDualWield())
		{
			CWeapon* pWeaponMaster = static_cast<CWeapon*>(pWeapon->GetDualWieldMaster());
			CWeapon* pWeaponSlave = static_cast<CWeapon*>(pWeapon->GetDualWieldSlave());

			if (pWeaponMaster == NULL)
				pWeaponMaster = pWeapon;

			if (pWeaponSlave && pWeaponSlave->IsWeaponRaised())
				pose |= (uint8)eWeaponRaisedPose_DualLft;

			if (pWeaponMaster && pWeaponMaster->IsWeaponRaised())
				pose |= (uint8)eWeaponRaisedPose_DualRgt;

			if (pose != eWeaponRaisedPose_None)
				raised = true;
		}
		else if (pWeapon->IsWeaponRaised())
		{		
			raised = true;
		}

		if (raised)
		{
			pose |= pWeapon->GetRaisePose();
		}
	}*/
 
	if(m_pAnimatedCharacter)
		m_pAnimatedCharacter->SetWeaponRaisedPose((EWeaponRaisedPose)pose);
}

bool CPlayer::ShouldUsePhysicsMovement()
{
	//swimming use physics, for everyone
	if (m_stats.inWaterTimer > 0.01f)
		return true;

	if (IsPlayer())
	{
		//Client in first person uses physics movement
		//Other players always physics
		return m_isClient ? !m_stats.isThirdPerson : true;
	}

	//in demo playback - use physics for recorded entities
	//AIs in normal conditions are supposed to use LM
	return IsDemoPlayback();
}

void CPlayer::PrePhysicsUpdate()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	// TODO: This whole function needs to be optimized.
	// TODO: Especially when characters are dead, alot of stuff here can be skipped.

	if (!m_pAnimatedCharacter)
		return;

	IEntity* pEnt = GetEntity();
	if (pEnt->IsHidden() && !(pEnt->GetFlags() & ENTITY_FLAG_UPDATE_HIDDEN))
		return;

	Debug();

	//HACK - Avoid collision with grabbed NPC - Beni
	/*if(m_pHumanGrabEntity && !m_throwingNPC)
	{
		IMovementController * pMC = GetMovementController();
		if(pMC)
		{
			SMovementState info;
			pMC->GetMovementState(info);

			Matrix34 prePhysics = m_pHumanGrabEntity->GetWorldTM();
			prePhysics.AddTranslation(info.eyeDirection*0.5f);
			m_pHumanGrabEntity->SetWorldTM(prePhysics);
		}

	}*/

	float frameTime = gEnv->pTimer->GetFrameTime();

	//TODO: Check if this is actually used.
	//It early outs for actual players in 'IdleUpdate'. Remove CVar from MP?
	if (m_pMovementController)
	{
		if (g_pGame->GetCVars()->g_enableIdleCheck)
		{
			IActorMovementController::SStats stats;
			if (m_pMovementController->GetStats(stats) && stats.idle == true)
			{
				if (GetGameObject()->IsProbablyVisible()==false && GetGameObject()->IsProbablyDistant() )
				{
					CPlayerMovementController* pMC = static_cast<CPlayerMovementController*> (m_pMovementController);
					pMC->IdleUpdate(frameTime);
					return;
				}
			}		
		}
	}

	bool client(IsClient());
	bool bThirdPerson(IsThirdPerson());

	//FIXME:
	// small hack to make first person ignore animation speed, and everything else use it
	// will need to be reconsidered for multiplayer
	SAnimatedCharacterParams params = m_pAnimatedCharacter->GetParams();
	params.flags &= ~(eACF_AlwaysAnimation | eACF_PerAnimGraph | eACF_AlwaysPhysics | eACF_ConstrainDesiredSpeedToXY | eACF_NoLMErrorCorrection);

	if (ShouldUsePhysicsMovement())
	{
		params.flags |= eACF_AlwaysPhysics | eACF_ImmediateStance | eACF_NoLMErrorCorrection;

		if ((gEnv->bMultiplayer && !client) || bThirdPerson)
		{	
			params.flags |= eACF_UseHumanBlending|eACF_PerAnimGraph;
			params.flags &= ~(eACF_NoLMErrorCorrection | eACF_AlwaysPhysics);
		}
	}
	else
	{
		params.flags |= eACF_ImmediateStance | eACF_PerAnimGraph | eACF_UseHumanBlending | eACF_ConstrainDesiredSpeedToXY;
	}

	if (client && !bThirdPerson) //Therefore in first person
	{
		params.flags &= ~(eACF_AlwaysAnimation | eACF_PerAnimGraph);
		params.flags |= eACF_AlwaysPhysics;
	}

	m_pAnimatedCharacter->SetParams(params);

	bool fpMode = IsPlayer();

	m_pAnimatedCharacter->GetAnimationGraphState()->SetFirstPersonMode( fpMode );

	SActorFrameMovementParams frameMovementParams;
	if (m_pMovementController)
	{
		if (m_pMovementController->Update(frameTime, frameMovementParams))
		{
/*
#ifdef _DEBUG
			if(m_pMovementDebug)
				m_pMovementDebug->AddValue(frameMovementParams.desiredVelocity.len());
			if(m_pDeltaXDebug)
				m_pDeltaXDebug->AddValue(frameMovementParams.desiredVelocity.x);
			if(m_pDeltaYDebug)
				m_pDeltaYDebug->AddValue(frameMovementParams.desiredVelocity.y);
#endif
*/

			Quat	tempq = m_viewQuatFinal;

			Quat baseQuatBackup(m_baseQuat);
			if (!m_linkStats.CanRotate() || m_stats.isStandingUp || m_stealthKill.IsBusy())
			{
				frameMovementParams.deltaAngles.x = 0.0f;
				frameMovementParams.deltaAngles.y = 0.0f;
				frameMovementParams.deltaAngles.z = 0.0f;
			}

			SPlayerRotationParams::EAimType aimType = GetCurrentAimType();
			CPlayerRotation playerRotation(
				*this, frameMovementParams,
				m_playerRotationParams.m_horizontalAims[aimType],
				m_playerRotationParams.m_verticalAims[aimType],
				frameTime);
			playerRotation.Process();
			playerRotation.Commit(*this);

			if (m_forcedRotation)
			{
				m_baseQuat = m_viewQuat = m_viewQuatFinal = tempq;
				m_viewAngles.SetAnglesXYZ(tempq);
				
				m_forcedRotation = false;
			}
			
			CRecordingSystem * pRecordingSystem = g_pGame->GetRecordingSystem();
			if( pRecordingSystem )
			{
				pRecordingSystem->SetPlayerVelocity(GetEntity()->GetId(), frameMovementParams.desiredVelocity);
			}

			if (m_linkStats.CanMove())
			{
				if (m_stats.animationControlled == false)
				{
					CPlayerMovement playerMovement( *this, frameMovementParams, frameTime );
					playerMovement.Process(*this);

					if (m_netSetPosition && g_pGameCVars->pl_velocityBasedInterpolation)
					{
						Vec3 error = m_netCurrentLocation - GetAnimatedCharacter()->GetAnimLocation().t;
						playerMovement.AddVelocity(error * g_pGameCVars->pl_velocityInterpFactor);
					}

					playerMovement.Commit(*this);
				}
				else 
				{
					m_interactiveActionController.Update(frameTime, frameMovementParams);
				}

				float fNewCrouchTime = (float)__fsel(m_stats.forceCrouchTime, m_stats.forceCrouchTime - frameTime, m_stats.forceCrouchTime);
				
				const SNanoSuitGameParameters& suitParams = GetActorSuitGameParameters();

				if (ShouldSwim())
					SetStance(STANCE_SWIM);
				else if((fNewCrouchTime > 0.0f) || (m_stats.slideStats.slidingState == SPlayerStats::eSS_Sliding))
					SetStance(STANCE_CROUCH);
				else if (m_stats.bSprinting || IsOnLedge()) //Holding and NPC
					SetStance(STANCE_STAND);
				else if (frameMovementParams.stance != STANCE_NULL)
				{
					SetStance(frameMovementParams.stance);					
				}

				m_stats.forceCrouchTime = fNewCrouchTime;
			}
			else
			{
				frameMovementParams.desiredVelocity = ZERO;
				CPlayerMovement playerMovement( *this, frameMovementParams, frameTime );
				playerMovement.Process(*this);
				playerMovement.Commit(*this);
			}

			//if (m_linkStats.CanDoIK() || (gEnv->bMultiplayer && GetLinkedVehicle()))
			//	SetIK(frameMovementParams);

			if (frameTime > 0.0f)
				m_weaponParams.inputRot = frameMovementParams.deltaAngles / frameTime;
			m_weaponParams.inputMove = frameMovementParams.desiredVelocity;
		}
	}

	//offset the character so its hip is at entity's origin
	ICharacterInstance *pCharacter = pEnt->GetCharacter(0);

	if (pCharacter)
	{
		if (client)
		{
			// clear the players look target every frame
			if (m_pMovementController)
			{
				CMovementRequest mr;
				mr.ClearLookTarget();
				m_pMovementController->RequestMovement( mr );
			}
		}
	}
	//

	if (m_pMovementController)
	{
		m_pMovementController->PostUpdate(frameTime);

		if (m_stats.mountedWeaponID)
		{
			UpdateMountedGunController(false);
		}


		//--- STAP, delayed IK update until after the PostUpdate so that the AimDirection is up to date
		if (m_linkStats.CanDoIK() || (gEnv->bMultiplayer && GetLinkedVehicle()))
			SetIK(frameMovementParams);
	}

	m_stats.hasRunIntoSomething = false;
}

IActorMovementController * CPlayer::CreateMovementController()
{
	return new CPlayerMovementController(this);
}

void CPlayer::SetIK( const SActorFrameMovementParams& frameMovementParams )
{
	if (!m_pAnimatedCharacter)
		return;

	IAnimationGraphState * pGraph = m_pAnimatedCharacter->GetAnimationGraphState();
	if (!pGraph)
		return;

	IEntity * pEntity = GetEntity();
	ICharacterInstance * pCharacter = pEntity->GetCharacter(0);
	if (!pCharacter)
		return;

	SMovementState curMovementState;
	m_pMovementController->GetMovementState(curMovementState);

	pGraph->SetInput( m_inputUsingLookIK, int(frameMovementParams.lookIK || frameMovementParams.aimIK) );

	// -----------------------------------
	// LOOKING
	// -----------------------------------

	bool lookEnabled = frameMovementParams.lookIK && !m_stats.isGrabbed && m_pAnimatedCharacter->IsLookIkAllowed();

	const SActorParams * pActorParams = GetActorParams();
	if (!lookEnabled || IsClient() || !GetLinkedVehicle())
	{
		// Normal case
		m_lookAim.UpdateLook( this, pCharacter, lookEnabled, GetLookFOV(pActorParams), frameMovementParams.lookTarget );
	}
	else
	{
		// Special case for non-clients in vehicles

		// don't allow spine to move in vehicles else the hands won't be on the wheel
		float lookIKBlends[5];
		lookIKBlends[0] = 0.00f;		// spine 1
		lookIKBlends[1] = 0.00f;		// spine 2
		lookIKBlends[2] = 0.00f;		// spine 3
		lookIKBlends[3] = 0.00f;		// neck
		lookIKBlends[4] = 0.7f;		// head 

		// look in 'vehicleviewdir' (NOTE: this code should probably be somewhere else, including the vehicleViewDir concept)
		Vec3 target = curMovementState.eyePosition + 5.0f * m_vehicleViewDir;

		m_lookAim.UpdateLook( this, pCharacter, true, GetLookFOV(pActorParams), target, lookIKBlends );
	}

	// -----------------------------------
	// AIMING 
	// -----------------------------------

		Vec3 aimTarget = frameMovementParams.aimTarget;
		bool aimEnabled = frameMovementParams.aimIK && m_pAnimatedCharacter->IsLookIkAllowed();

		if (IsPlayer())
		{
		aimTarget = curMovementState.eyePosition + curMovementState.aimDirection * 5.0f; // If this is too close the aiming will fade out.
			aimEnabled = true;

			// TODO: This should probably be moved somewhere else and not done every frame.
			ICharacterInstance* pCharacterShadow = GetShadowCharacter();
			if (pCharacter)
				pCharacter->GetISkeletonPose()->SetAimIKFadeOut(0);
			if (pCharacterShadow)
				pCharacterShadow->GetISkeletonPose()->SetAimIKFadeOut(0);
		}
		else
		{
			// Even for AI we need to guarantee a valid aim target position (only needed for LAW/Rockets though).
			// We should not set aimEnabled to true though, because that will force aiming for all weapons.
			// The AG templates will communicate an animation synched "force aim" flag to the animation playback.
			if (!aimEnabled)
			{
				if (frameMovementParams.lookIK)
				{
					// Use look target
					aimTarget = frameMovementParams.lookTarget;
				}
				else
				{
					// Synthesize target
				aimTarget = curMovementState.eyePosition + curMovementState.aimDirection * 5.0f; // If this is too close the aiming will fade out.
				}
			}
		}

		CRecordingSystem * pRecordingSystem = g_pGame->GetRecordingSystem();
		if( pRecordingSystem )
		{
			AimIKInfo aiki(aimEnabled, aimTarget);
			pRecordingSystem->SetPlayerAimIK(GetEntityId(), aiki);
		}

	ISkeletonPose * pSkeletonPose = pCharacter->GetISkeletonPose();
		pSkeletonPose->SetAimIK( aimEnabled && !DoSTAPAiming(), aimTarget);
		pSkeletonPose->SetAimIKLayer(1);

		pGraph->SetInput( m_inputAiming, aimEnabled ? 1 : 0 );

		if (ICharacterInstance * pCharacterShadow = GetShadowCharacter())
		{
			ISkeletonPose * pSkeletonPoseShadow = pCharacterShadow->GetISkeletonPose();
			pSkeletonPoseShadow->SetAimIK( aimEnabled, aimTarget );
			pSkeletonPoseShadow->SetAimIKLayer(1);
		}

		if (pCharacter != 0)
		{
			CArmsAimingController armsAimingController;
			armsAimingController.Update(aimEnabled, m_params, curMovementState, *pCharacter);
		}


		/*if (aimEnabled && !m_params.leftArmAimPose.empty() && !m_params.bothArmsAimPose.empty() && !m_params.rightArmAimPose.empty())
		{
			CryCharAnimationParams animParams;
			animParams.m_nLayerID = 1;
			animParams.m_fTransTime = 0.5f;
			animParams.m_nFlags |= CA_LOOP_ANIMATION;

			const Vec3 rotatedMovementDirection(-curMovementState.movementDirection.y, curMovementState.movementDirection.x, 0.0f);
			const float rotatedAimDotMovement = curMovementState.aimDirection.dot(rotatedMovementDirection);
			const float aimDotMovement = curMovementState.aimDirection.dot(curMovementState.movementDirection);

			const char* animName = m_params.bothArmsAimPose;

			if (aimDotMovement < 0.7f)
			{
				if (rotatedAimDotMovement > 0.0f)
				{
					animName = m_params.leftArmAimPose;
				}
				else
				{
					animName = m_params.rightArmAimPose;
				}
			}


			ISkeletonAnim* pISkeletonAnim = pCharacter->GetISkeletonAnim();
			IAnimationSet* pAnimSet = pCharacter->GetIAnimationSet();
			const int animId = pAnimSet->GetAnimIDByName(animName);
			pISkeletonAnim->StartAnimationById(animId, animParams);
		}*/
}

void CPlayer::UpdateView(SViewParams &viewParams)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	CRY_TODO(25, 3, 2010, "Benito: Why do we need this pointer check here? Find out, and remove if not needed")
	if (!m_pAnimatedCharacter)
		return;

	if (g_pGameCVars->pl_debug_view != 0)
	{
		CryLog("CPlayer::UpdateView: Pre Pos(%f, %f, %f) Rot(%f, %f, %f, %f)", viewParams.position.x, viewParams.position.y, viewParams.position.z, viewParams.rotation.v.x, viewParams.rotation.v.y, viewParams.rotation.v.z, viewParams.rotation.w);
	}

	CRY_TODO(25, 3, 2010, "Benito: What is this doing? Move to player camera itself if needed");
	if (viewParams.groundOnly)
	{
		if (m_stats.inAir > 0.0f)
		{
			float factor = m_stats.inAir;
			Limit(factor, 0.0f, 0.2f);
			factor = 1.0f - (factor * 5.0f);
			factor = factor*factor;

			viewParams.currentShakeQuat.SetSlerp(IDENTITY, viewParams.currentShakeQuat, factor);
			viewParams.currentShakeShift *= factor;
		}
	}

	if (m_playerCamera)
	{
		m_playerCamera->Update(viewParams, gEnv->pTimer->GetFrameTime());
	}

	CRY_TODO(25, 3, 2010, "Benito: Move this piece inside player camera as well");
	if (!IsThirdPerson() && (GetSpectatorMode() == CActor::eASM_None))
  {
		if (CItem *pItem=GetItem(GetInventory()->GetCurrentItem()))
		{
			if (pItem->FilterView(viewParams))
			{
				assert( viewParams.rotation.IsValid() );
				m_baseQuat = m_viewQuat = m_viewQuatFinal = viewParams.rotation;
			}
		}
  }

	viewParams.blend = m_viewBlending;
	m_viewBlending=true;	// only disable blending for one frame

	//store the view matrix, without vertical component tough, since its going to be used by the VectorToLocal function.
	Vec3 forward(viewParams.rotation.GetColumn1());
	Vec3 up(m_baseQuat.GetColumn2());
	Vec3 right(-(up % forward));

	m_clientViewMatrix.SetFromVectors(right,up%right,up,viewParams.position);
	m_clientViewMatrix.OrthonormalizeFast();

	// finally, update the network system
	if (gEnv->bMultiplayer)
		g_pGame->GetIGameFramework()->GetNetContext()->ChangedFov( GetEntityId(), viewParams.fov );

	CRecordingSystem* recordingSystem = g_pGame->GetRecordingSystem();
	if(recordingSystem && recordingSystem->IsPlayingBack())
	{
		recordingSystem->UpdateView(viewParams);
	}

	if (g_pGameCVars->pl_debug_view != 0)
	{
		CryLog("CPlayer::UpdateView: Post Pos(%f, %f, %f) Rot(%f, %f, %f, %f)", viewParams.position.x, viewParams.position.y, viewParams.position.z, viewParams.rotation.v.x, viewParams.rotation.v.y, viewParams.rotation.v.z, viewParams.rotation.w);
	}

}

void CPlayer::PostUpdateView(SViewParams &viewParams)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	//Benito: Update STAP after view has been updated, and blended
	//        Let the item go after, if it wants to add some post processing
	bool useViewPosition = IsSliding() || IsOnLedge() || m_stats.animationControlled;

	Vec3 cameraPositionForTorso = (!useViewPosition) ? GetFPCameraPosition() : viewParams.position;

	UpdateFPIKTorso(cameraPositionForTorso);

	//const float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	//const Vec3 desiredCameraOffset = m_baseQuat.GetInverted() * (cameraPositionForTorso - GetEntity()->GetWorldPos());
	//const Vec3 finalCameraOffset = m_baseQuat.GetInverted() * (viewParams.position - GetEntity()->GetWorldPos());
	//gEnv->pRenderer->Draw2dLabel(50.0f, 50.0f, 1.75f, white, false, "Desired offset(1): %.3f, %.3f, %.3f", desiredCameraOffset.x, desiredCameraOffset.y, desiredCameraOffset.z);

	SHUDEvent viewUpdateEvent(eHUDEvent_OnViewUpdate);
	viewUpdateEvent.AddData(SHUDEventData((void*)(&viewParams)));
	CHUD::CallEvent(viewUpdateEvent);

	if (CItem *pItem=GetItem(GetInventory()->GetCurrentItem()))
		pItem->PostFilterView(viewParams);

	//gEnv->pRenderer->Draw2dLabel(50.0f, 75.0f, 1.75f, white, false, "Final   offset(2): %.3f, %.3f, %.3f", finalCameraOffset.x, finalCameraOffset.y, finalCameraOffset.z);
	//gEnv->pRenderer->Draw2dLabel(50.0f, 100.0f, 1.75f, white, false, "Near clip plane dist: %.3f", (float)gEnv->pSystem->GetViewCamera().GetNearPlane());

}

void CPlayer::RegisterPlayerEventListener(IPlayerEventListener *pPlayerEventListener)
{
	stl::push_back_unique(m_playerEventListeners, pPlayerEventListener);
}

void CPlayer::UnregisterPlayerEventListener(IPlayerEventListener *pPlayerEventListener)
{
	stl::find_and_erase(m_playerEventListeners, pPlayerEventListener);
}

void CPlayer::RegisterPlayerSuitEventListener(CNanoSuit::INanoSuitListener *pNanoSuitListener)
{
	if(m_pNanoSuit)
		m_pNanoSuit->AddListener(pNanoSuitListener);
}

void CPlayer::UnregisterPlayerSuitEventListener(CNanoSuit::INanoSuitListener *pNanoSuitListener)
{
	if(m_pNanoSuit)
		m_pNanoSuit->RemoveListener(pNanoSuitListener);
}

void CPlayer::RegisterPlayerUpdateListener(IPlayerUpdateListener *pPlayerUpdateListener)
{
	stl::push_back_unique(m_playerUpdateListeners, pPlayerUpdateListener);
}

void CPlayer::UnregisterPlayerUpdateListener(IPlayerUpdateListener *pPlayerUpdateListener)
{
	stl::find_and_erase(m_playerUpdateListeners, pPlayerUpdateListener);
}

IEntity *CPlayer::LinkToVehicle(EntityId vehicleId) 
{
	IEntity *pLinkedEntity = CActor::LinkToVehicle(vehicleId);

	if (pLinkedEntity)
	{
		IVehicle *pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(vehicleId);
		
		if (IVehicleSeat *pSeat = pVehicle->GetSeatForPassenger(GetEntity()->GetId()))
		{
			CALL_PLAYER_EVENT_LISTENERS(OnEnterVehicle(this,pVehicle->GetEntity()->GetClass()->GetName(),pSeat->GetSeatName(),m_stats.isThirdPerson));
		}

		// don't interpolate to vehicle camera (otherwise it intersects the vehicle)
		if(IsClient())
		{
			SupressViewBlending();
		}

		if (m_playerCamera)
		{
			m_playerCamera->SetBestCameraMode("Player now linked to vehicle");
		}
	}
	else
	{
		CALL_PLAYER_EVENT_LISTENERS(OnExitVehicle(this));
		m_vehicleViewDir.Set(0,1,0);

		// don't interpolate back from vehicle camera (otherwise you see your own legs)
		if(IsClient())
			SupressViewBlending();

		if (m_playerCamera)
		{
			m_playerCamera->SetBestCameraMode("Player now unlinked from vehicle");
	}
	}

	return pLinkedEntity;
}

IEntity *CPlayer::LinkToEntity(EntityId entityId, bool bKeepTransformOnDetach) 
{
#ifdef __PLAYER_KEEP_ROTATION_ON_ATTACH
	Quat rotation = GetEntity()->GetRotation();
#endif
	IEntity *pLinkedEntity = CActor::LinkToEntity(entityId, bKeepTransformOnDetach);

	if (pLinkedEntity)
	{
#ifdef __PLAYER_KEEP_ROTATION_ON_ATTACH
		if (bKeepTransformOnDetach)
		{
			assert( rotation.IsValid() );
			m_linkStats.viewQuatLinked = m_linkStats.baseQuatLinked = rotation;
			m_viewQuatFinal = m_viewQuat = m_baseQuat = rotation;
		}
		else
#endif
		{
			m_baseQuat.SetIdentity();
			m_viewQuat.SetIdentity();
			m_viewQuatFinal.SetIdentity();
		}
	}

	return pLinkedEntity;
}

void CPlayer::LinkToMountedWeapon(EntityId weaponId) 
{
	m_stats.mountedWeaponID = weaponId;

	if(!m_pAnimatedCharacter)
		return;

	SAnimatedCharacterParams params = m_pAnimatedCharacter->GetParams();
	
	if (weaponId)
	{
		if (!IsClient())
			params.flags &= ~eACF_EnableMovementProcessing;
		params.flags |= eACF_NoLMErrorCorrection;

		m_mountedGunController.OnEnter(weaponId);
		m_pAnimatedCharacter->SetNoMovementOverride(true);
	}
	else
	{
		if (!IsClient())
			params.flags |= eACF_EnableMovementProcessing;
		params.flags &= ~eACF_NoLMErrorCorrection;

		m_mountedGunController.OnLeave();
		m_pAnimatedCharacter->SetNoMovementOverride(false);
	}
	
	m_pAnimatedCharacter->SetParams( params );
}

void CPlayer::StartInteractiveAction(EntityId entityId)
{
	AnimationControlled(true);

	m_interactiveActionController.OnEnter(entityId);
}

void CPlayer::StartInteractiveActionByName( const char* interaction )
{
	AnimationControlled(true);

	m_interactiveActionController.OnEnterByName(interaction);
}

void CPlayer::EndInteractiveAction(EntityId entityId)
{
	AnimationControlled(false);

	m_interactiveActionController.OnLeave();
}

//------------------------------------------------------------------------
bool CPlayer::GetForcedLookDir(Vec3& vDir) const
{
	bool bResult = false;

	if (!m_forcedLookDir.IsZero())
	{
		vDir = m_forcedLookDir;
		bResult = true;
	}

	return bResult;
}

//------------------------------------------------------------------------
void CPlayer::SetForcedLookDir(const Vec3& vDir)
{
	m_forcedLookDir = vDir;
}

//------------------------------------------------------------------------
void CPlayer::ClearForcedLookDir()
{
	m_forcedLookDir.zero();
}

//------------------------------------------------------------------------
bool CPlayer::CanMove() const
{
	return (!m_pHitDeathReactions || m_pHitDeathReactions->CanActorMove());
}

void CPlayer::SufferingHighLatency(bool highLatency)
{
#ifndef PS3 //ignore on PS3 for now
	if (highLatency && !m_sufferingHighLatency)
	{
		if (IsClient() && !gEnv->bServer)
			g_pGameActions->FilterNoConnectivity()->Enable(true);
	}
	else if (!highLatency && m_sufferingHighLatency)
	{
		if (IsClient() && !gEnv->bServer)
			g_pGameActions->FilterNoConnectivity()->Enable(false);

		// deal with vehicles here as well
	}

	// the following is done each frame on the server, and once on the client, when we're suffering high latency
	if (highLatency)
	{
		if (m_pPlayerInput.get())
			m_pPlayerInput->Reset();

		if (IVehicle *pVehicle=GetLinkedVehicle())
		{
			if (IActor *pActor=pVehicle->GetDriver())
			{
				if (pActor->GetEntityId()==GetEntityId())
				{
					if (IVehicleMovement *pMovement=pVehicle->GetMovement())
						pMovement->ResetInput();
				}
			}
		}
	}

	m_sufferingHighLatency = highLatency;
#endif
}

void CPlayer::SetViewInVehicle(Quat viewRotation)
{
	m_vehicleViewDir = viewRotation * Vec3(0,1,0); 
	CHANGED_NETWORK_STATE(this, ASPECT_VEHICLEVIEWDIR_CLIENT);
}

void CPlayer::OnStanceChanged(EStance newStance, EStance oldStance)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	CActor::OnStanceChanged(newStance, oldStance);

	CRY_ASSERT_TRACE(newStance == m_stance, ("Expected 'newStance' (%d) to be equal to 'm_stance' (%d)", newStance, m_stance));
	CRY_ASSERT_TRACE(newStance != oldStance, ("Expected 'newStance' (%d) to be different to 'oldStance' (%d)", newStance, oldStance));

	if (IsClient())
	{
		CCCPOINT_IF(oldStance == STANCE_CROUCH, PlayerMovement_LocalPlayerStopCrouch);
		CCCPOINT_IF(newStance == STANCE_CROUCH, PlayerMovement_LocalPlayerStartCrouch);
	}
	else
	{
		CCCPOINT_IF(oldStance == STANCE_CROUCH, PlayerMovement_NonLocalPlayerStopCrouch);
		CCCPOINT_IF(newStance == STANCE_CROUCH, PlayerMovement_NonLocalPlayerStartCrouch);
	}

	CALL_PLAYER_EVENT_LISTENERS(OnStanceChanged(this, newStance));
}

float CPlayer::CalculatePseudoSpeed(bool wantSprint, float speedOverride) const
{
	if(m_health < 0.0f)
	{
		return 0.0f;
	}

	if (wantSprint)
	{
		return 1.0f;
	}
	else
	{
		const float MIN_WALK_SPEED = 0.1f;
		const float MAX_SPEED = 4.0f;
		const float MIN_PSEUDO_SPEED = 0.1f;
		const float MAX_PSEUDO_SPEED = 0.9f;
		const float fINV_MAGNITUDE = 1.0f / (MAX_SPEED - MIN_WALK_SPEED);

		float speed = (float) __fsel(speedOverride, speedOverride, m_stats.velocity.len());

		const float ret = min((speed - MIN_WALK_SPEED) * fINV_MAGNITUDE, 1.0f);

		return (float)__fsel(speed - MIN_WALK_SPEED, MIN_PSEUDO_SPEED + (ret * (MAX_PSEUDO_SPEED - MIN_PSEUDO_SPEED)), 0.0f);

		//Slower code below so you can work out what the hell is happening above

// 		if (speed < MIN_WALK_SPEED)
// 		{
// 			return 0.0f;
// 		}
// 		else
// 		{
// 			float ret = ((speed - MIN_WALK_SPEED) * fINV_MAGNITUDE);
// 			if (ret > 1.0f)
// 				ret = 1.0f;
// 			return MIN_PSEUDO_SPEED + (ret * (MAX_PSEUDO_SPEED - MIN_PSEUDO_SPEED));
// 		}
	}
}

float CPlayer::GetStanceMaxSpeed(EStance stance) const
{
	return GetStanceInfo(stance)->maxSpeed * m_params.speedMultiplier * GetWeaponMovementFactor() * m_gameRulesSpeedMult;
}

void CPlayer::SetParams(SmartScriptTable &rTable,bool resetFirst)
{
	//not sure about this
	if (resetFirst)
	{
		m_params = SPlayerParams();
	}

	CActor::SetParams(rTable,resetFirst);

	CScriptSetGetChain params(rTable);
	params.GetValue("sprintMultiplier",m_params.sprintMultiplier);
	params.GetValue("strafeMultiplier",m_params.strafeMultiplier);
	params.GetValue("backwardMultiplier",m_params.backwardMultiplier);

	params.GetValue("jumpHeight",m_params.jumpHeight);

	params.GetValue("leanShift",m_params.leanShift);
	params.GetValue("leanAngle",m_params.leanAngle);

	params.GetValue("weaponBobbingMultiplier",m_params.weaponBobbingMultiplier);
	params.GetValue("weaponInertiaMultiplier",m_params.weaponInertiaMultiplier);

	params.GetValue("speedMultiplier",m_params.speedMultiplier);

	//view related
	params.GetValue("viewLimitDir",m_params.vLimitDir);
	params.GetValue("viewLimitYaw",m_params.vLimitRangeH);
	params.GetValue("viewLimitPitch",m_params.vLimitRangeV);

	params.GetValue("viewFoVScale",m_params.viewFoVScale);

	//TODO:move to SetStats()
	int followHead=m_stats.followCharacterHead.Value();
	params.GetValue("followCharacterHead", followHead);
	m_stats.followCharacterHead=followHead;

	const char* footstepName;
	if (params.GetValue("footstepEffect", footstepName))
	{
		m_params.footstepEffectName = footstepName;
	}
	params.GetValue("footstepGearEffect", m_params.footstepGearEffect);

	const char* footstepIndGearAudioSignal = NULL;
	if (params.GetValue("footstepIndGearAudioSignal", footstepIndGearAudioSignal))
	{
		m_soundFootstepIndGear.SetSignal( footstepIndGearAudioSignal );
	}

	params.GetValue("canUseComplexLookIK", m_params.canUseComplexLookIK);

	const char* lookAtSimpleHeadBoneName;
	if (params.GetValue("lookAtSimpleHeadBone", lookAtSimpleHeadBoneName))
	{
		m_params.lookAtSimpleHeadBoneName = lookAtSimpleHeadBoneName;
	}

	params.GetValue("aimIKFadeDuration", m_params.aimIKFadeDuration);
	params.GetValue("proceduralLeaningFactor", m_params.proceduralLeaningFactor);

	const char* leftArmAimPose = 0;
	if (params.GetValue("leftArmAimPose", leftArmAimPose))
	{
		m_params.leftArmAimPose = leftArmAimPose;
	}

	const char* rightArmAimPose = 0;
	if (params.GetValue("rightArmAimPose", rightArmAimPose))
	{
		m_params.rightArmAimPose = rightArmAimPose;
	}

	const char* bothArmsAimPose = 0;
	if (params.GetValue("bothArmsAimPose", bothArmsAimPose))
	{
		m_params.bothArmsAimPose = bothArmsAimPose;
	}
}

//fill the status table for the scripts
bool CPlayer::GetParams(SmartScriptTable &rTable)
{
	CScriptSetGetChain params(rTable);

	params.SetValue("sprintMultiplier", m_params.sprintMultiplier);
	params.SetValue("strafeMultiplier", m_params.strafeMultiplier);
	params.SetValue("backwardMultiplier", m_params.backwardMultiplier);

	params.SetValue("jumpHeight", m_params.jumpHeight);
	params.SetValue("leanShift", m_params.leanShift);
	params.SetValue("leanAngle", m_params.leanAngle);

	params.SetValue("weaponInertiaMultiplier",m_params.weaponInertiaMultiplier);
	params.SetValue("weaponBobbingMultiplier",m_params.weaponBobbingMultiplier);

	params.SetValue("speedMultiplier",m_params.speedMultiplier);

	//view related
	REUSE_VECTOR(rTable, "viewLimitDir",m_params.vLimitDir);
	params.SetValue("viewLimitYaw",m_params.vLimitRangeH);
	params.SetValue("viewLimitPitch",m_params.vLimitRangeV);

	params.SetValue("viewFoVScale",m_params.viewFoVScale);

	params.SetValue("canUseComplexLookIK", m_params.canUseComplexLookIK);
	params.SetValue("lookAtSimpleHeadBone",(const char*)m_params.lookAtSimpleHeadBoneName);

	params.SetValue("aimIKFadeDuration", m_params.aimIKFadeDuration);
	params.SetValue("proceduralLeaningFactor", m_params.proceduralLeaningFactor);

	return true;
}
void CPlayer::SetStats(SmartScriptTable &rTable)
{
	CActor::SetStats(rTable);

	rTable->GetValue("inFiring",m_stats.inFiring);
}

//fill the status table for the scripts
void CPlayer::UpdateScriptStats(SmartScriptTable &rTable)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);

	CActor::UpdateScriptStats(rTable);

	CScriptSetGetChain stats(rTable);

	//stats.SetValue("isWalkingOnWater",m_stats.isWalkingOnWater);
	
	//stats.SetValue("shakeAmount",m_stats.shakeAmount);	
	m_stats.followCharacterHead.SetDirtyValue(stats, "followCharacterHead");
	m_stats.firstPersonBody.SetDirtyValue(stats, "firstPersonBody");
	
	stats.SetValue("onLedge", m_stats.ledgeStats.onLedge);
	stats.SetValue("onLedgeTranstion", (int)m_stats.ledgeStats.onLedgeTransition);

	if (HasNanoSuit())
	{
		const SNanoSuitGameParameters& suitParams = GetActorSuitGameParameters();
		const SNanoSuitProperties& suitModeProperties = suitParams.GetProps();

		bool power = (suitParams.GetMode() == eNanoSuitMode_Power);
		bool stealth = (suitParams.GetMode() == eNanoSuitMode_Stealth);
		bool armor = (suitParams.GetMode() == eNanoSuitMode_Armor);
		bool tactical = (suitParams.GetMode() == eNanoSuitMode_Tactical);

		stats.SetValue("nanoModeArmor", armor);
		stats.SetValue("nanoSuitDamageAbsorption", suitModeProperties.damageAbsorption);
		stats.SetValue("nanoSuitDamageAbsorptionMelee", suitModeProperties.damageAbsorptionMelee);

		stats.SetValue("nanoModeStealth", stealth);
		stats.SetValue("nanoModeStealthNoiseSupression", suitModeProperties.noiseSupression);

		stats.SetValue("nanoModePower", power);

		stats.SetValue("nanoModeTactical", tactical);

		stats.SetValue("isSliding", IsSliding() ? 1 : 0);
		stats.SetValue("isSprinting", (m_stats.bSprinting) ? 1 : 0);
		stats.SetValue("isInAir", m_stats.inAir);
		stats.SetValue("fallSpeed", m_stats.fallSpeed);
	}

	//stats.SetValue("cloakState",(m_pNanoSuit)?m_pNanoSuit->GetCloak()->GetState():0);	
	//stats.SetValue("visualDamp",(m_pNanoSuit)?m_pNanoSuit->GetCloak()->GetVisualDamp():0);
	//stats.SetValue("soundDamp",(m_pNanoSuit)?m_pNanoSuit->GetCloak()->GetSoundDamp():0);
	//stats.SetValue("heatDamp",(m_pNanoSuit)?m_pNanoSuit->GetCloak()->GetHeatDamp():0);
}

//------------------------------------------------------------------------
bool CPlayer::ShouldSwim() const
{
	CRY_FIXME(30, 3, 2010, "Benito: Restore after MS3 demo, we don't have water in the level");
	return false;

	if (m_stats.relativeBottomDepth < 1.3f)
		return false;

	if ((m_stats.relativeBottomDepth < 2.0f) && (m_stats.inWaterTimer > -1.0f) && (m_stats.inWaterTimer < -0.0f))
		return false;

	if (m_stats.relativeWaterLevel > 3.0f)
		return false;

	if ((m_stats.relativeWaterLevel > 0.1f) && (m_stats.inWaterTimer < -2.0f))
		return false;

	if ((m_stats.velocity.z < -1.0f) && (m_stats.relativeWaterLevel > 0.0f) && (m_stats.inWaterTimer < -2.0f))
		return false;

	if (GetLinkedVehicle() != NULL)
		return false;

	return true;
}

//------------------------------------------------------------------------
void CPlayer::UpdateSwimStats(float frameTime)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	bool isClient(IsClient());

	const Vec3 vEntityPos = GetEntity()->GetWorldPos();
	
	Vec3 localReferencePos = ZERO;
	Vec3 referencePos = vEntityPos;
	int spineID = GetBoneID(BONE_SPINE3);
	if (spineID > -1)
	{
		ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
		ISkeletonPose* pSkeletonPose = (pCharacter != NULL) ? pCharacter->GetISkeletonPose() : NULL;
		if (pSkeletonPose != NULL)
		{
			localReferencePos = pSkeletonPose->GetAbsJointByID(spineID).t;
			localReferencePos.x = 0.0f;
			localReferencePos.y = 0.0f;
#if !defined(_RELEASE)
			bool bLocalRefPosValid = localReferencePos.IsValid();
			assert(bLocalRefPosValid);
			if (!bLocalRefPosValid)
				localReferencePos = ZERO;
#endif
			if (localReferencePos.GetLengthSquared() > (2.0f * 2.0f))
				localReferencePos = ZERO;
		}

		referencePos += GetEntity()->GetWorldRotation() * localReferencePos;
	}

	float worldWaterLevel = gEnv->p3DEngine->GetWaterLevel(&referencePos);
	//early out and clear some stats just in case
	if(worldWaterLevel < 0.0f)
	{
		m_stats.relativeBottomDepth = 0.0f;
		m_stats.worldWaterLevel = worldWaterLevel;
		ExitSwimming();
		return;
	}

	m_stats.worldWaterLevelDelta = clamp(worldWaterLevel - m_stats.worldWaterLevel, -0.5f, +0.5f); // In case worldWaterLevel is reset or fucked up, preventing huge deltas.
	m_stats.worldWaterLevel = worldWaterLevel;
	float playerWaterLevel = -WATER_LEVEL_UNKNOWN;
	float bottomDepth = 0;
	Vec3 surfacePos(referencePos.x, referencePos.y, worldWaterLevel);

#if DEFERRED_RAY_CAST_BOTTOM
	RayTestBottomLevel(referencePos, 10.0f);
	Vec3 bottomPos(referencePos.x, referencePos.y, m_bottomLevel);
#else
	float bottomLevel = gEnv->p3DEngine->GetBottomLevel(referencePos, 10.0f);
	Vec3 bottomPos(referencePos.x, referencePos.y, bottomLevel);
#endif

	playerWaterLevel = referencePos.z - surfacePos.z;
	bottomDepth = surfacePos.z - bottomPos.z;

	m_stats.relativeWaterLevel = clamp(playerWaterLevel, -5.0f, 5.0f);
	m_stats.relativeBottomDepth = bottomDepth;

	float fLocalHeadZ = GetLocalEyePos().z + 0.2f;

	float fWorldHeadZ = vEntityPos.z + fLocalHeadZ;
	float headWaterLevel = fWorldHeadZ - surfacePos.z;

	// when inside a vehicle (like a boat or amphibious APC) we always assume to be 'above water'
	if (GetLinkedVehicle())
		headWaterLevel = 0.3f;

	if (headWaterLevel < 0.0f)
	{
		GetAnimationGraphState()->SetInput(m_inputWaterLevel, "Depth");

		if (m_stats.headUnderWaterTimer >= 0.0f)
		{
			m_stats.headUnderWaterTimer += frameTime;
		}
		else
		{
			PlaySound(ESound_DiveIn);
			m_stats.headUnderWaterTimer = 0.0f;
		}
	}
	else
	{
		GetAnimationGraphState()->SetInput(m_inputWaterLevel, "Surface");

		if (m_stats.headUnderWaterTimer <= 0.0f)
		{
			m_stats.headUnderWaterTimer -= frameTime;			
		}
		else
		{
			PlaySound(ESound_DiveOut);
			PlaySound(ESound_Breathing_Water, true, "action", 2.5f, "speed", m_stats.speedFlat );
			m_stats.headUnderWaterTimer = 0.0f;
		}
	}

	// Update inWater timer (positive is in water, negative is out of water).
	if (ShouldSwim())
	{
		//by design : AI cannot swim and drowns no matter what
		if((GetHealth() > 0) && !isClient && !gEnv->bMultiplayer)
		{
			// apply damage same way as all the other kinds
			HitInfo hitInfo;
			hitInfo.damage = max(1.0f, 30.0f * frameTime);
			hitInfo.targetId = GetEntity()->GetId();
			g_pGame->GetGameRules()->ServerHit(hitInfo);
			CreateScriptEvent("splash",0);
		}

		if (m_stats.inWaterTimer < 0.0f)
		{
			m_stats.inWaterTimer = 0.0f;

			if(m_stats.inAir>0.0f)
			{
				CreateScriptEvent("jump_splash",m_stats.worldWaterLevel-GetEntity()->GetWorldPos().z,"NoSound");
			}
			EnterSwimming();
		}
		else
		{
			m_stats.inWaterTimer = min(m_stats.inWaterTimer + frameTime, 1000.0f);
		}
	}
	else
	{
		ExitSwimming();
		m_stats.inWaterTimer = (float)__fsel(-m_stats.inWaterTimer, max(m_stats.inWaterTimer - frameTime, -1000.0f), 0.0f);
	}

	// Set underwater level for sound listener.
	if (isClient)
	{
		CCamera& camera = gEnv->pSystem->GetViewCamera();
		float cameraWaterLevel = (camera.GetPosition().z - surfacePos.z);

		IListener* pListener = gEnv->pSoundSystem->GetListener(LISTENERID_STANDARD);
		if (pListener)
			pListener->SetUnderwater(cameraWaterLevel); // TODO: Make sure listener interface expects zero at surface and negative under water.

		PlaySound(ESound_Underwater, (cameraWaterLevel < 0.0f));
	}

#if !defined(XENON) && !defined(PS3)
	// DEBUG RENDERING
	bool debugSwimming = (g_pGameCVars->cl_debugSwimming != 0);
	if (debugSwimming && (playerWaterLevel > -10.0f) && (playerWaterLevel < 10.0f))
	{
		Vec3 vRight(m_baseQuat.GetColumn0());

		static ColorF referenceColor(1,1,1,1);
		static ColorF surfaceColor1(0,0.5f,1,1);
		static ColorF surfaceColor0(0,0,0.5f,0);
		static ColorF bottomColor(0,0.5f,0,1);

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(referencePos, 0.1f, referenceColor);

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(referencePos, surfaceColor1, surfacePos, surfaceColor1, 2.0f);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(surfacePos, 0.2f, surfaceColor1);
		gEnv->pRenderer->DrawLabel(referencePos + vRight * 0.5f, 1.5f, "WaterLevel %3.2f (HWL %3.2f) (SurfaceZ %3.2f)", playerWaterLevel, headWaterLevel, surfacePos.z);
		gEnv->pRenderer->DrawLabel(referencePos + vRight * 0.5f - Vec3(0,0,-0.2f), 1.5f, "InWaterTimer %3.2f", m_stats.inWaterTimer);

		static int lines = 16;
		static float radius0 = 0.5f;
		static float radius1 = 1.0f;
		static float radius2 = 2.0f;
		for (int i = 0; i < lines; ++i)
		{
			float radians = ((float)i / (float)lines) * gf_PI2;
			Vec3 offset0(radius0 * cry_cosf(radians), radius0 * cry_sinf(radians), 0);
			Vec3 offset1(radius1 * cry_cosf(radians), radius1 * cry_sinf(radians), 0);
			Vec3 offset2(radius2 * cry_cosf(radians), radius2 * cry_sinf(radians), 0);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(surfacePos+offset0, surfaceColor0, surfacePos+offset1, surfaceColor1, 2.0f);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(surfacePos+offset1, surfaceColor1, surfacePos+offset2, surfaceColor0, 2.0f);
		}

		if (bottomDepth > 0.0f)
		{
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(referencePos, bottomColor, bottomPos, bottomColor, 2.0f);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(bottomPos, 0.2f, bottomColor);
			gEnv->pRenderer->DrawLabel(bottomPos + Vec3(0,0,0.5f) - vRight * 0.5f, 1.5f, "BottomDepth %3.3f", bottomDepth);
		}
	}
#endif
}

//------------------------------------------------------------------------
void CPlayer::UpdateBreathing(float frameTime)
{
	if (!IsClient())
		return;

	m_timeForBreath -= frameTime;
	if (m_timeForBreath<=0 && m_stats.speedFlat==0)
	{
		EActionSoundParamValues actionParam = EActionSoundParam_RunWalkIdle;

		if (IsSwimming())
		{
			 actionParam = m_stats.headUnderWaterTimer>0 ? EActionSoundParam_Underwater : EActionSoundParam_SwimOnWater;
		}
		PlayBreathingSound( actionParam );

		m_timeForBreath = INTERVAL_BREATHING;
				}
}


//////////////////////////////////////////////////////////////////////////

void CPlayer::PlayBreathingSound( EActionSoundParamValues actionSoundParam )
{
	EPlayerSounds sound = ESound_Breathing;
	if (actionSoundParam==EActionSoundParam_SwimOnWater || actionSoundParam==EActionSoundParam_Underwater) 
		sound = ESound_Breathing_Water;

	PlaySound( sound, true, "action", float(actionSoundParam), "speed", m_stats.speedFlat );
	m_timeForBreath = INTERVAL_BREATHING;
}



//------------------------------------------------------------------------
//TODO: Clean up this whole function, unify this with CAlien via CActor.
void CPlayer::UpdateStats(float frameTime)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);
	
	//The results from GetEntity() are used without checking in
	//	CPlayer::Update, so should be safe here!
	assert(GetEntity());	


	bool isClient(IsClient());
	if (isClient)
	{
		m_stats.firstPersonBody = (uint8)g_pGameCVars->cl_fpBody;
	}	

	const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
	IAnimationGraphState* pAGState = GetAnimationGraphState();

	IPhysicalEntity *pPhysEnt = GetEntity()->GetPhysics();

	if (!pPhysEnt)
		return;

  if (gEnv->pSystem->IsDedicated() && GetLinkedVehicle())
  {
    // leipzig: force inactive (was active on ded. servers)
    pe_player_dynamics paramsGet;
    if (pPhysEnt->GetParams(&paramsGet))
    {      
      if (paramsGet.bActive && m_pAnimatedCharacter)
				m_pAnimatedCharacter->RequestPhysicalColliderMode(eColliderMode_Disabled, eColliderModeLayer_Game, "Player::UpdateStats");
    }			
  }  

  bool isPlayer(IsPlayer());

	CHECKQNAN_VEC(m_eyeOffset);
	CRY_HACK(18, 09, 2009, "filipe - smoothing of eye offset view was temporarily removed for cover and lean.");

	//players use a faster interpolation, and using only the Z offset. A bit different for AI.
	Vec3 svOffset = GetStanceViewOffset(m_stance, &m_stats.leanAmount, true);
	
	CRY_ASSERT(svOffset.IsValid());

	if (!IsMigrating())
	{
		float interpFactor = isPlayer ? 15.0f : 5.0f;
		Interpolate(m_eyeOffset, svOffset,interpFactor,frameTime);
	}
	else
	{
		m_eyeOffset = svOffset;
	}

	CHECKQNAN_VEC(m_eyeOffset);

	CHECKQNAN_VEC(m_weaponOffset);
	Interpolate(m_weaponOffset, GetWeaponOffsetWithLean(m_stance, m_stats.leanAmount, m_stats.peekOverAmount, m_eyeOffset), 5.0f, frameTime);
	CHECKQNAN_VEC(m_weaponOffset);
	
	pe_player_dynamics simPar;

	if (pPhysEnt->GetParams(&simPar) == 0)
		return;

	const SAnimationTarget * pTarget = NULL;
	
	IAnimationGraphState * pAnimGraphState = GetAnimationGraphState();	
	if(pAnimGraphState)
		pTarget = pAnimGraphState->GetAnimationTarget();

	bool forceForAnimTarget = false;
	if (pTarget && pTarget->doingSomething)
		forceForAnimTarget = true;

	CRY_FIXME(30, 3, 2010, "Benito: Restore after MS3 demo, quite heavy, and we don't have water in the level");
	//UpdateSwimStats(frameTime);


	const bool shouldSwim = ShouldSwim();

	const uint8 iSpectatorMode = GetSpectatorMode();

	//FIXME:
	if ((iSpectatorMode || (!simPar.bActive && !isClient) || m_stats.flyMode || GetLinkedVehicle()) && !forceForAnimTarget)
	{
		m_stats.velocity = m_stats.velocityUnconstrained.Set(0,0,0);
		m_stats.speed = m_stats.speedFlat = 0.0f;
		m_stats.fallSpeed = 0.0f;
		m_stats.startFallingHeight = 0.0f;
		m_stats.inFiring = 0;
		m_stats.jumpLock = 0;
		m_stats.inWaterTimer = -1000.0f;
		m_stats.groundMaterialIdx = -1;

		pe_player_dynamics simParSet;
		simParSet.bSwimming = true;
		simParSet.gravity.zero();
		pPhysEnt->SetParams(&simParSet);

		if (!shouldSwim)  //Underwater 
		{
			return;
		}
	}

	//retrieve some information about the status of the player
	pe_status_dynamics dynStat;
	pe_status_living livStat;

	int dynStatType = dynStat.type;
	memset(&dynStat, 0, sizeof(pe_status_dynamics));
	dynStat.type = dynStatType;

	int livStatType = livStat.type;
	memset(&livStat, 0, sizeof(pe_status_living));
	livStat.groundSlope = Vec3(0,0,1);
	livStat.type = livStatType;

	pPhysEnt->GetStatus(&dynStat);
	pPhysEnt->GetStatus(&livStat);


	//Beni - If animation controlled, do not update gravity from physics, it's being updated to 0 in PlayerMovement.cpp (ProcessOnGroundJumping)
	//       If not when we go back to entity control the wrong gravity will be set ;/
	if((m_pAnimatedCharacter != NULL)	 && (m_pAnimatedCharacter->GetMCMV() == eMCM_Animation))
		m_stats.gravity = m_stats.gravity;
	else
		m_stats.gravity = simPar.gravity;

	float groundNormalBlend = clamp(frameTime / 0.15f, 0.0f, 1.0f);
	m_stats.groundNormal = LERP(m_stats.groundNormal, livStat.groundSlope, groundNormalBlend);

	if (livStat.groundSurfaceIdxAux > 0)
		m_stats.groundMaterialIdx = livStat.groundSurfaceIdxAux;
	else
		m_stats.groundMaterialIdx = livStat.groundSurfaceIdx;

	Vec3 ppos(GetWBodyCenter());

	m_stats.upVector.Set(0,0,1);


	//
	if (m_stats.forceUpVector.len2()>0.01f)
	{
		m_stats.upVector = m_stats.forceUpVector;    
		m_stats.forceUpVector.zero(); 
	}
	//


	DebugGraph_AddValue("PhysVelo", livStat.vel.GetLength());
	DebugGraph_AddValue("PhysVeloX", livStat.vel.x);
	DebugGraph_AddValue("PhysVeloY", livStat.vel.y);
	DebugGraph_AddValue("PhysVeloZ", livStat.vel.z);

	DebugGraph_AddValue("PhysVeloUn", livStat.velUnconstrained.GetLength());
	DebugGraph_AddValue("PhysVeloUnX", livStat.velUnconstrained.x);
	DebugGraph_AddValue("PhysVeloUnY", livStat.velUnconstrained.y);
	DebugGraph_AddValue("PhysVeloUnZ", livStat.velUnconstrained.z);

	DebugGraph_AddValue("PhysVelReq", livStat.velRequested.GetLength());
	DebugGraph_AddValue("PhysVelReqX", livStat.velRequested.x);
	DebugGraph_AddValue("PhysVelReqY", livStat.velRequested.y);
	DebugGraph_AddValue("PhysVelReqZ", livStat.velRequested.z);

	bool onGround = false;
	bool isFlying = (livStat.bFlying != 0);
	bool isStuck = (livStat.bStuck != 0);
	// if not flying then character is currently touching ground.
	// if flying but was not flying character is seen as on ground still, unless he has jumped.
	// wasFlying tracks the last frame, since flying and stuck altenates/toggles when living entity is stuck, in which case we wanna move and jump as if on ground.
	// Given the stuck timeout wasFlying might be redundant.
	if (!isFlying || (!m_stats.wasFlying && !m_stats.jumped) || (m_stats.stuckTimeout > 0.0f))
	{
		if (livStat.groundHeight > -1E10f) // Sanity check to prohibit "false landing" on quickload (physics/isFlying not updated in first frame)
			onGround = true; 
	}
	
	if (isStuck)
		m_stats.stuckTimeout = 0.3f;
	else
		m_stats.stuckTimeout = max(0.0f, m_stats.stuckTimeout - frameTime);

	m_stats.wasFlying = isFlying;

	if (livStat.groundHeight > -1E10f)
	{
		float feetHeight = GetEntity()->GetWorldPos().z;
		float feetElevation = (feetHeight - livStat.groundHeight);
		// Only consider almost on ground if not jumped and not yet having been well in the air.
		if ((feetElevation < 0.1f) && !m_stats.jumped && (m_stats.inAir == 0.0f))
			onGround = true;
	}

/*
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(GetEntity()->GetWorldPos()+Vec3(0,0,0.5f), ColorF(1,1,0,1), 
																								GetEntity()->GetWorldPos()+Vec3(0,0,0.5f)+m_stats.onWallNormal, ColorF(1,1,0,1), 10.0f);
/**/

//*
	DebugGraph_AddValue("OnGround", onGround ? 1.0f : 0.0f);
	DebugGraph_AddValue("Jumping", m_stats.jumped ? 1.0f : 0.0f);
	DebugGraph_AddValue("Flying", isFlying ? 1.0f : 0.0f);
	DebugGraph_AddValue("StuckTimer", m_stats.stuckTimeout);
	DebugGraph_AddValue("InAirTimer", m_stats.inAir);
	DebugGraph_AddValue("OnGroundTimer", m_stats.onGround);
	DebugGraph_AddValue("InWaterTimer", m_stats.inWaterTimer);
/**/

	//update status table
	if (!onGround && !iSpectatorMode && !GetLinkedVehicle())
	{
		if (shouldSwim)
		{
			if(pAGState)
			{
				char action[256];
				pAGState->GetInput(m_inputAction, action);
				if ( strcmp(action,"jumpMP")==0 || strcmp(action,"jumpMPStrength")==0 )
					pAGState->SetInput(m_inputAction, "idle");
			}
		}
		else
		{
			m_stats.inAir += frameTime;
			// Danny - changed this since otherwise AI has a fit going down stairs - it seems that when off
			// the ground it can only decelerate.
			// If you revert this (as it's an ugly hack) - test AI on stairs!
			static float minTimeForOffGround = 0.5f;
			
			if(isPlayer)
			{
				//instead of zeroing onGround for player, reduce it according to cumulative air time. This means that minor
				//  'inAir' time, eg, running over a small gap between two planks, doesn't disallow the ability to jump for 0.2 seconds
				m_stats.onGround = max(m_stats.onGround - (m_stats.inAir * 0.33f), 0.0f);
			}
			else if (m_stats.inAir > minTimeForOffGround)
			{
				m_stats.onGround = 0.0f;
			}

			if (IsOnLedge())
				m_stats.inAir = 0.0f;
		}
	}
	else
	{

		bool landed = false;

		if ( (m_stats.inAir > 0.0f) && onGround)
			landed = true;


		// This is needed when jumping while standing directly under something that causes an immediate land,
		// before and without even being airborne for one frame.
		// PlayerMovement set m_stats.onGround=0.0f when the jump is triggered,
		// which prevents the on ground timer from before the jump to be inherited
		// and incorrectly and prematurely used to identify landing in MP.
		bool jumpFailed = (m_stats.jumped && (m_stats.onGround > 0.0f) && (livStat.velUnconstrained.z <= 0.0f));

		if (jumpFailed)
		{
			m_stats.jumped = false;
			SetSuperJumping(false);
		}

		if (landed)
		{
			if(!GetPerkData<bool>(EPD_MuteJumping))
			{
				SonarVisionPerk_SetNumParticlesToCreate(perkVars->perk_sonarVision_skelAlignedParticles_numToCreate_land,eBoneEmitter_Feet);
				SonarVisionPerk_SetNumParticlesToCreate(perkVars->perk_sonarVision_skelAlignedParticles_numToCreate_random,eBoneEmitter_Skeleton);
				m_perkParticleEmitterInfo.AddNoiseBurst(perkVars->perk_sonarVision_IncreaseParticleSizeWhenLandOnGround * min(1.f, m_stats.inAir * 2.f));
			}

			if (m_stats.inAir > 0.3f)
				m_stats.landed = true; // This is only used for landing camera bobbing in PlayerView.cpp.

			SendPerkEvent(EPE_OverrideLandVelocity, &m_stats.velocityUnconstrained);


			if(gEnv->bMultiplayer && m_stats.jumped)
			{
				m_stats.jumpLock = min(g_pGameCVars->pl_jump_baseTimeAddedPerJump + (m_stats.jumpLock * g_pGameCVars->pl_jump_currentTimeMultiplierOnJump), g_pGameCVars->pl_jump_maxTimerValue);
			}

			m_stats.jumped = false;

			Landed(m_stats.fallSpeed); // fallspeed might be incorrect on a dedicated server (pos is synced from client, but also smoothed).

			SetSuperJumping(false);	//Must be set after landed, to trigger some effects

			if(m_stats.fallSpeed)
			{
				CreateScriptEvent("landed",m_stats.fallSpeed);
				m_stats.fallSpeed = 0.0f;

				Vec3 playerPos = GetEntity()->GetWorldPos();

				if(playerPos.z<m_stats.worldWaterLevel)
					CreateScriptEvent("jump_splash",m_stats.worldWaterLevel-playerPos.z);
			}
		}
		else
		{
			m_stats.landed = false;
		}


		if((landed || shouldSwim || jumpFailed) && pAGState)
		{
			char action[256];
			pAGState->GetInput(m_inputAction, action);
			if ( strcmp(action,"jumpMP")==0 || strcmp(action,"jumpMPStrength")==0 )
				pAGState->SetInput(m_inputAction, "idle");
		}

		m_stats.onGround = min(m_stats.onGround + frameTime, 1.0f);		//Capping this allows better logic for allowing people to jump, as it can be reduced when
		//  in the air instead of just zeroed.
		m_stats.inAir = 0.0f;
	}

	Vec3 prevVelocityU = m_stats.velocityUnconstrained;
	m_stats.velocity = livStat.vel-livStat.velGround;
	m_stats.velocityUnconstrained = livStat.velUnconstrained;
	m_stats.angVelocity = dynStat.w;

	// On dedicated server the player can still be flying this frame as well, 
	// since synced pos from client is interpolated/smoothed and will not land immediately,
	// even though the velocity is set to zero.
	// Thus we need to use the velocity change instead of landing to identify impact.
	float newDownwardsImpactVelocity = max(0.0f, min(m_stats.velocityUnconstrained.z, 0.0f) - min(prevVelocityU.z, 0.0f));

	// If you land while in a vehicle, don't apply damage.
	if (GetLinkedVehicle() != NULL)
		newDownwardsImpactVelocity = 0.0f;

	// If you land in deep enough water, don't apply damage. 
	// (In MP the remote ShouldSwim() might lag behind though, so you will get damage unless you have a parachute).
	if (shouldSwim)
		newDownwardsImpactVelocity = 0.0f;

	// Zero downwards impact velocity used for fall damage calculations if player was in water within the last 0.5 seconds.
	// Strength jumping straight up and down should theoretically land with a safe velocity, 
	// but together with the water surface stickyness the velocity can sometimes go above the safe impact velocity threshold.

	newDownwardsImpactVelocity = (float)__fsel(-(m_stats.inWaterTimer + 0.5f), newDownwardsImpactVelocity, 0.0f);

	m_stats.downwardsImpactVelocity = newDownwardsImpactVelocity;

	// Apply Fall Damage
	assert(NumberValid(m_stats.downwardsImpactVelocity));
	if (newDownwardsImpactVelocity > 0.0f)
	{
		float velSafe = g_pGameCVars->pl_health.fallDamage_SpeedSafe;
		float velFatal = g_pGameCVars->pl_health.fallDamage_SpeedFatal;

		float velFraction = (float)__fsel(-(velFatal - velSafe), 1.0f , (newDownwardsImpactVelocity - velSafe) / (velFatal - velSafe));

		assert(NumberValid(velFraction));

		if (velFraction > 0.0f)
		{
			velFraction = powf(velFraction, g_pGameCVars->pl_health.fallDamage_CurveAttack);

			float maxHealth = (float)GetMaxHealth();
			float currentHealth  = (float)GetHealth();
			
			HitInfo hit;
			hit.dir.y = 0.f;
			assert(hit.dir.IsZero());
			hit.type = g_pGame->GetGameRules()->GetHitTypeId("fall");
			hit.targetId = GetEntity()->GetId();
			hit.shooterId = GetEntity()->GetId();

			float maxDamage = (float)__fsel(velFraction - 1.0f, maxHealth, max(0.0f, currentHealth - g_pGameCVars->pl_health.fallDamage_health_threshold));
			
			hit.damage = velFraction * maxDamage;
			//Stop crouching after taking falling damage
			if(GetStance() == STANCE_CROUCH)
			{
				static_cast<CPlayerInput*>(GetPlayerInput())->ClearCrouchAction();
			}


			// This is a bit nasty, should be able to do this on the server however apparently we don't know about anyone else's velocity properly (possibly due to lerping stuff)
			if (IsClient())
			{
				g_pGame->GetGameRules()->ClientHit(hit);
			}

			if (g_pGameCVars->pl_health.debug_FallDamage != 0)
			{
				const char* side = gEnv->bServer ? "Server" : "Client";

				const char* color = "";
				if (velFraction < 0.33f)
					color = "$6"; // Yellow
				else if (velFraction < 0.66f)
					color = "$8"; // Orange
				else
					color = "$4"; // Red

				CryLogAlways("%s[%s][%s] ImpactVelo=%3.2f, FallDamage=%3.1f", 
					color, side, GetEntity()->GetName(), newDownwardsImpactVelocity, hit.damage);
			}
		}
		else if (g_pGameCVars->pl_health.debug_FallDamage != 0)
		{
			if (newDownwardsImpactVelocity > 0.5f)
			{
				const char* side = gEnv->bServer ? "Server" : "Client";
				const char* color = "$3"; // Green
				CryLogAlways("%s[%s][%s] ImpactVelo=%3.2f, FallDamage: NONE", 
					color, side, GetEntity()->GetName(), newDownwardsImpactVelocity);
			}
		}
	}


	//calculate the flatspeed from the player ground orientation
	Vec3 flatVel(m_baseQuat.GetInverted()*m_stats.velocity);
	flatVel.z = 0;
	m_stats.speedFlat = flatVel.len();

	if (m_stats.inAir && m_stats.velocity*m_stats.gravity>0.0f && (m_stats.inWaterTimer <= 0.0f))
	{
		if (!m_stats.fallSpeed)
			CreateScriptEvent("fallStart",0);

		m_stats.fallSpeed = -m_stats.velocity.z;
	}
	else
	{
		m_stats.fallSpeed = 0.0f;
		m_stats.startFallingHeight = 0.0f;
		//CryLogAlways( "[player] end falling %f", ppos.z);
	}

	if (g_pGameCVars->pl_health.debug_FallDamage == 2)
	{
		Vec3 pos = GetEntity()->GetWorldPos();
		const char* side = gEnv->bServer ? "Sv" : "Cl";
		CryLogAlways("[%s] liv.vel=%0.1f,%0.1f,%3.2f liv.velU=%0.1f,%0.1f,%3.2f impactVel=%3.2f posZ=%3.2f (liv.velReq=%0.1f,%0.1f,%3.2f) (fallspeed=%3.2f) gt=%3.3f, pt=%3.3f", 
								side, 
								livStat.vel.x, livStat.vel.y, livStat.vel.z, 
								livStat.velUnconstrained.x, livStat.velUnconstrained.y, livStat.velUnconstrained.z, 
								m_stats.downwardsImpactVelocity, 
								/*pos.x, pos.y,*/ pos.z, 
								livStat.velRequested.x, livStat.velRequested.y, livStat.velRequested.z, 
								m_stats.fallSpeed, 
								gEnv->pTimer->GetCurrTime(), gEnv->pPhysicalWorld->GetPhysicsTime());
	}

	m_stats.mass = dynStat.mass;

	float fSpeedFlatSelector = m_stats.speedFlat - 0.1f;
	m_stats.inMovement	= (float)__fsel(fSpeedFlatSelector, m_stats.inMovement + frameTime, 0.0f);
	m_stats.inRest			= (float)__fsel(fSpeedFlatSelector, 0.0f, m_stats.inRest + frameTime);

	if (shouldSwim)
	{
		m_stats.inAir = 0.0f;
	}

	//
	pe_player_dimensions ppd;

	ppd.headRadius = 0.0f;
	pPhysEnt->SetParams(&ppd);

	pe_player_dynamics simParSet;
	simParSet.bSwimming = (iSpectatorMode || m_stats.flyMode || shouldSwim);
	//set gravity to 0 in water
	if (shouldSwim && (m_stats.relativeWaterLevel <= 0.0f))
		simParSet.gravity.zero();

	pPhysEnt->SetParams(&simParSet);

	//update some timers
	m_stats.inFiring = max(0.0f,m_stats.inFiring - frameTime);

	m_stats.jumpLock = (float)__fsel(-fabsf(m_stats.inAir), max(0.0f, m_stats.jumpLock - frameTime), m_stats.jumpLock);

	if(IsClient())
	{
		UpdateFlashbangEffect(frameTime);
	
		//Crysis 2 feature updates
		UpdateCrysis2Features(frameTime);
	}
}

void CPlayer::UpdateVisibility()
{
	bool forceDontRenderNearest = IsOnLedge() || m_stealthKill.IsBusy() || m_stats.animationControlled;
#if ENABLE_MINDCONTROL
	forceDontRenderNearest = forceDontRenderNearest || IsMindControlling();
#endif
	UpdatePlayerVisibility(GetEntity(), IsThirdPerson(), GetShadowCharacterSlot(), forceDontRenderNearest);
}

void UpdatePlayerVisibility(IEntity* playerEntity, bool isThirdPerson, int shadowCharacterSlot, bool forceDontRenderNearest)
{
	ICharacterInstance *mainChar			= playerEntity->GetCharacter(0);
	
	if (mainChar == NULL)
		return;
	
	IAttachmentManager *attachmentMan		= mainChar->GetIAttachmentManager();
	ICharacterInstance *shadowChar			= NULL;
	IAttachmentManager *attachmentManShadow	= NULL;
	if (shadowCharacterSlot >= 0)
	{
		shadowChar = playerEntity->GetCharacter(shadowCharacterSlot);
		if (shadowChar)
		{
			attachmentManShadow = shadowChar->GetIAttachmentManager();
		}
	}

	bool showShadowChar = g_pGameCVars->g_showShadowChar != 0;

	uint32 flags = playerEntity->GetSlotFlags(0);
	if (isThirdPerson || forceDontRenderNearest)
	{
		flags &= ~ENTITY_SLOT_RENDER_NEAREST;
	}
	else
	{
		flags |= ENTITY_SLOT_RENDER_NEAREST;
	}
	playerEntity->SetSlotFlags(0, flags);

	if (attachmentManShadow)
	{
		for (uint32 i=0; i<attachmentManShadow->GetAttachmentCount(); i++)
		{
			attachmentManShadow->GetInterfaceByIndex(i)->HideAttachment(!showShadowChar);
		}
	}

	//--- Toggle head attachment
	IAttachment *headAttachment = attachmentMan->GetInterfaceByName("Head");
	if (headAttachment)
	{
		headAttachment->HideAttachment(!isThirdPerson);
	}

	IAttachment *attachment;
	if (isThirdPerson)
	{
		for (uint32 i=0; i<attachmentMan->GetAttachmentCount(); i++)
		{
			attachment = attachmentMan->GetInterfaceByIndex(i);
			attachment->HideInShadow(false);
			attachment->HideInRecursion(false);
		}
		if (attachmentManShadow)
		{
			for (uint32 i=0; i<attachmentManShadow->GetAttachmentCount(); i++)
			{
				attachment = attachmentManShadow->GetInterfaceByIndex(i);
				attachment->HideInShadow(true);
				attachment->HideInRecursion(true);
			}
		}
	}
	else
	{
		for (uint32 i=0; i<attachmentMan->GetAttachmentCount(); i++)
		{
			attachment = attachmentMan->GetInterfaceByIndex(i);
			attachment->HideInShadow(shadowChar ? true : false);
			attachment->HideInRecursion(shadowChar ? true : false);
		}
		if (attachmentManShadow)
		{
			for (uint32 i=0; i<attachmentManShadow->GetAttachmentCount(); i++)
			{
				attachment = attachmentManShadow->GetInterfaceByIndex(i);
				attachment->HideInShadow(false);
				attachment->HideInRecursion(false);
			}
		}
	}
}

//
//-----------------------------------------------------------------------------
void CPlayer::ToggleThirdPerson()
{
	m_stats.isThirdPerson = !m_stats.isThirdPerson;

	m_animationProxy.SetFirstPerson(!IsThirdPerson());
	m_animationProxyUpper.SetFirstPerson(!IsThirdPerson());

	if (m_stats.isThirdPerson)
	{
		m_torsoAimIK.Disable(true);
	}
	else
	{
		m_torsoAimIK.Enable(true);
	}

	UpdateVisibility();

	CALL_PLAYER_EVENT_LISTENERS(OnToggleThirdPerson(this,m_stats.isThirdPerson));
}

int CPlayer::IsGod()
{
	if (!m_pGameFramework->CanCheat())
		return 0;

	CGodMode& godMode = CGodMode::GetInstance();

	EGodModeState godModeState = godMode.GetCurrentState();

	// Demi-Gods are not Gods
	if (eGMS_DemiGodMode == godModeState)
		return 0;

	//check if is the player
	if (IsPlayer())
		return (int)godModeState;

	//check if is a squadmate
	IAIActor* pAIActor = CastToIAIActorSafe(GetEntity()->GetAI());
	if (pAIActor)
	{
		int group=pAIActor->GetParameters().m_nGroup;
		if(group>= 0 && group<10)
			return (godModeState == eGMS_TeamGodMode?1:0);
	}
	return 0;
}

bool CPlayer::IsThirdPerson() const
{
	//force thirdperson view for non-clients
  return !m_isClient || m_stats.isThirdPerson;	
}

#ifndef _RELEASE
static AUTOENUM_BUILDNAMEARRAY(s_reasonForReviveNames, ReasonForReviveList);
#endif

void CPlayer::Revive( EReasonForRevive reasonForRevive )
{
	CActor::Revive(reasonForRevive);

	InitNanoSuit();

	ResetScreenFX();

	m_bSwimming = false;
	m_isSuperJumping = false;
	m_actions = 0;
	m_actionFlags = eAF_NONE;
	m_forcedRotation = false;
	m_fSpeedLean = 0.0f;

	m_ragdollTime = 0.0f;

	m_healthAccumError = 0.0f;

	ResetFrameAngleViewOffset();

	if (IsClient() == m_stats.isThirdPerson)
		ToggleThirdPerson();
	UpdateVisibility();

	const Vec3 oldGravity = m_stats.gravity; 
	// HAX: to fix player spawning and floating in dedicated server: Marcio fix me?
	if (gEnv->IsEditor() == false)  // AlexL: in editor, we never keep spectator mode
	{
		SSpectatorInfo  specInfo = m_stats.spectatorInfo;
		m_stats = SPlayerStats();
		m_stats.spectatorInfo = specInfo;
	}
	else
	{
		m_stats = SPlayerStats();
	}
	m_stats.gravity = oldGravity; //Restore the gravity (it's set initialy in PostPhysicalize, and was being override here!)

	m_headAngles.Set(0,0,0);
	m_eyeOffset = GetStanceViewOffset(STANCE_STAND);
	m_weaponOffset.Set(0,0,0);
	m_bobOffset.Set(0, 0, 0);
	m_bobCycle = 0;

	m_velocity.Set(0,0,0);

	m_feetWpos[0] = ZERO;
	m_feetWpos[1] = ZERO;

	m_angleOffset.Set(0,0,0);

	//m_baseQuat.SetIdentity();
	m_viewQuatFinal = m_baseQuat = m_viewQuat = GetEntity()->GetRotation();
	m_viewAngles.SetAnglesXYZ(m_viewQuat);
	m_viewAngles.x = 0.0f;
	m_viewAngles.y = 0.0f;
	m_clientViewMatrix.SetIdentity();

	m_lastRequestedVelocity.Set(0.0f,0.0f,0.0f);
	m_forcedLookDir.Set(0.0f,0.0f,0.0f);

	m_viewRoll = 0;
	m_upVector.Set(0,0,1);

	m_lastTorsoAimIKDir.Set(0.0f, 0.0f, 0.0f);
	m_lastTorsoAimIKPos.Set(0.0f, 0.0f, 0.0f);

	m_viewBlending = true;

	if (reasonForRevive != kRFR_StartSpectating)
	{
		m_fDeathTime = 0.0f;  // we need to know this value whilst spectating so we can display the respawn countdown and so on
	}
	
	GetEntity()->SetFlags(GetEntity()->GetFlags() | (ENTITY_FLAG_CASTSHADOW));
	GetEntity()->SetSlotFlags(0,GetEntity()->GetSlotFlags(0)|ENTITY_SLOT_RENDER);

	if (m_pPlayerInput.get())
		m_pPlayerInput->Reset();

#if ENABLE_MINDCONTROL
	m_pMindControlMaster.reset();
	m_pMindControlSlave.reset();

	if(!gEnv->bMultiplayer)
	{
		if (m_isClient)
			m_pMindControlMaster.reset( new CMindControlMaster(*this) );
		else if (GetEntity()->GetAI())
			m_pMindControlSlave.reset( new CMindControlSlave(*this) );
	}
#endif

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);

	if (pCharacter)
		pCharacter->EnableStartAnimation(true);

	ResetAnimations();
	// if we're coming from initialize, then we're not part of the game object yet -- therefore we can't
	// receive events from the animation graph, and hence, we'll delay the initial update until PostInit()
	// is called...
	//if (!fromInit)
	//	UpdateAnimGraph();

	//	m_nanoSuit.Reset(this);
	//	m_nanoSuit.Activate(m_params.nanoSuitActive);

	if (reasonForRevive != kRFR_FromInit || GetISystem()->IsSerializingFile() == 1)
		ResetAnimGraph();

	if(reasonForRevive != kRFR_FromInit && IsClient())
	{
		
		if(gEnv->bMultiplayer)
		{
			if(GetHealth() > 0 || (g_pGame && g_pGame->GetGameRules() && !g_pGame->GetGameRules()->IsPlayerActivelyPlaying(GetEntityId())))
			{
				// Only cancel respawn cycle timer if we're 'leaving the game' again and going to spectator mode.
				// Still want it shown if we're 3rd-person spectating and waiting to respawn...
				bool show = false;
				SHUDEvent showReviveCycle(eHUDEvent_ShowReviveCycle);
				showReviveCycle.eventPtrData = (void*)&show;
				CHUD::CallEvent(showReviveCycle);

				if( reasonForRevive != kRFR_StartSpectating )
				{
					g_pGame->GetHUD()->ActivateDefaultState();
				}
			}
				
			IView *pView = g_pGame->GetIGameFramework()->GetIViewSystem()->GetViewByEntityId(GetEntityId());
			if(pView)
				pView->ResetShaking();
				
				// stop menu music
					gEnv->pMusicSystem->EndTheme(EThemeFade_FadeOut, 0, true);

			// cancel voice recording (else channel can be stuck open)
			g_pGame->GetIGameFramework()->EnableVoiceRecording(false);
		}
	}

	// marcio: reset pose on the dedicated server (dedicated server doesn't update animationgraphs)
	if (!gEnv->bClient && gEnv->bServer)
	{
		if (ICharacterInstance *pCharacter=GetEntity()->GetCharacter(0))
			pCharacter->GetISkeletonPose()->SetDefaultPose();
	}

	//Restore near fov to default value (60.0f) and FP weapon position, just in case
	if (IsClient())
	{
		ResetFPView();
		s_clientInteractionInfo.Reset();
		g_pGameActions->FilterNoMove()->Enable(false);
	}

	// restore interactor
	if (IsPlayer() && !GetGameObject()->GetUpdateSlotEnables(GetInteractor(), 0))
		GetGameObject()->EnableUpdateSlot(GetInteractor(), 0);

	ResetPerksVariables();

	m_torsoAimIK.Reset();
	m_lookAim.Reset();

	if (reasonForRevive == kRFR_Spawn)
	{
		SendPerkEvent(EPE_Spawn);

		CRY_ASSERT(! m_triggerSniperCountermeasuresPlugin.IsEntered());

		EnterPlayerPlugin(& m_triggerSniperCountermeasuresPlugin);

		if(IsClient())
		{
			CCCPOINT(PlayerState_LocalPlayerSpawn);
			CHUD::CallEvent(SHUDEvent(eHUDEvent_OnSpawn)); // local player revive
		}
		else
		{
			CCCPOINT(PlayerState_NonLocalPlayerSpawn);
		}
	}

	ResetFPWeapon();

	// disable spectating if we are spawning
	if(reasonForRevive == kRFR_Spawn)
	{
		SetSpectatorModeAndOtherEntId(0, 0);
	}

	if (m_pHitDeathReactions)
	{
		m_pHitDeathReactions->OnRevive();
	}

	CRY_TODO(03,02,2010,"Maybe this would be better in Physicalize? Some of the functionality this relies on only exists after Physicalize() is called, which is not certain to have happened at this point in C2MP");
	if (!m_pBodyDamage)
	{
		m_pBodyDamage.reset(new CBodyDamage());
		m_pBodyDamage->Init(*this);
	}
}

void CPlayer::Kill()
{
	// set this so we can see the local players head when they die
	if(!IsThirdPerson() && gEnv->bMultiplayer && g_pGameCVars->pl_switchTPOnKill)
	{
		ToggleThirdPerson();
	}

	if(IsClient())
	{
		if (m_stats.flashBangStunTimer > 0.0f)
		{
			StopFlashbangEffects();
		}
		else
		{
			StopTinnitus();
		}
		IPlayerInput *pInput = GetPlayerInput();
		if(pInput && (pInput->GetType() == IPlayerInput::PLAYER_INPUT))
		{
			CPlayerInput *pPlayerInput = static_cast<CPlayerInput *>(pInput);
			pPlayerInput->ClearAllExceptAction(ACTION_USE);
		}
		g_pGameActions->FilterNoMove()->Enable(true);
	}

	CActor::Kill();
}

Vec3 CPlayer::GetStanceViewOffset(EStance stance,float *pLeanAmt, bool withY) const
{	
	Vec3 offset(GetStanceInfo(m_stance)->viewOffset);
	if (!withY)
		offset.y = 0.0f;

	float leanAmt = 0.0f;

	if (DoCoverAndLean())
	{
		leanAmt += m_coverAndLean.GetLeanAmount();
		offset.x += leanAmt;
		offset.z += m_coverAndLean.GetRaiseAmount();
	}

	//apply leaning
	if (!pLeanAmt)
		leanAmt += m_stats.leanAmount;
	else
		leanAmt += *pLeanAmt;

	if (!IsPlayer())
	{
		offset = GetStanceInfo(stance)->GetViewOffsetWithLean(leanAmt, m_stats.peekOverAmount);
	}

	return offset;
}		

void CPlayer::RagDollize( bool fallAndPlay )
{
	if (m_stats.isRagDoll && !gEnv->pSystem->IsSerializingFile())
	{
		if(!IsPlayer() && !fallAndPlay)
			DropAttachedItems();
		return;
	}

	ResetAnimations();

	CActor::RagDollize( fallAndPlay );

	PhysicalizeBodyDamage();

	m_stats.followCharacterHead = 1;

	IPhysicalEntity *pPhysEnt = GetEntity()->GetPhysics();

	if (pPhysEnt)
	{
		pe_simulation_params sp;
		sp.gravity = sp.gravityFreefall = m_stats.gravity;
		//sp.damping = 1.0f;
		sp.dampingFreefall = 0.0f;
		sp.mass = m_stats.mass * 2.0f;
		if(sp.mass <= 0.0f)
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Tried ragdollizing player with 0 mass.");
			sp.mass = 80.0f;
		}

		pPhysEnt->SetParams(&sp);

		if (gEnv->bMultiplayer)
		{
			pe_params_part pp;
			pp.flagsAND = ~(geom_colltype_player|geom_colltype_vehicle|geom_colltype6);
			pp.flagsColliderAND=pp.flagsColliderOR = geom_colltype_debris;
			pPhysEnt->SetParams(&pp);
		}
	}

	ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
	if (!fallAndPlay)
	{
		if (pCharacter)
		{
			pCharacter->EnableStartAnimation(false);
		}
		if(!IsPlayer())
		{
			DropAttachedItems();
		}
	}
	else
	{
		if (pCharacter)
		{
			pCharacter->GetISkeletonPose()->Fall();
		}
	}
}

void CPlayer::PostPhysicalize()
{
	CActor::PostPhysicalize();

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
	if (!pCharacter)
		return;
	pCharacter->GetISkeletonPose()->SetPostProcessCallback(PlayerProcessBones,this);
	pe_simulation_params sim;
	sim.maxLoggedCollisions = 5;
	pe_params_flags flags;
	flags.flagsOR = pef_log_collisions;
  if (!pCharacter->GetISkeletonPose())
    return;

  if (!pCharacter->GetISkeletonPose()->GetCharacterPhysics())
    return;

	pCharacter->GetISkeletonPose()->GetCharacterPhysics()->SetParams(&sim);
	pCharacter->GetISkeletonPose()->GetCharacterPhysics()->SetParams(&flags);
	

	//set a default offset for the character, so in the editor the bbox is correct
	if(m_pAnimatedCharacter)
	{
		m_pAnimatedCharacter->SetExtraAnimationOffset(QuatT(GetStanceInfo(STANCE_STAND)->modelOffset, IDENTITY));

		// Physicalize() was overriding some things for spectators on loading (eg gravity). Forcing a collider mode update
		//	will reinit it properly.
		if(GetSpectatorMode() != CActor::eASM_None)
		{
			EColliderMode mode = m_pAnimatedCharacter->GetPhysicalColliderMode();
			m_pAnimatedCharacter->ForceRefreshPhysicalColliderMode();
			m_pAnimatedCharacter->RequestPhysicalColliderMode(mode, eColliderModeLayer_Game, "Player::PostPhysicalize" );
		}
	}

	// Set correct value for gravity
	IPhysicalEntity *pPhysEnt = GetEntity()->GetPhysics();
	if (pPhysEnt)
	{
		pe_player_dynamics pd;
		if (pPhysEnt->GetParams(&pd))
			m_stats.gravity=pd.gravity;
	}

//	if (m_pGameFramework->IsMultiplayer())
//		GetGameObject()->ForceUpdateExtension(this, 0);
}

//------------------------------------------------------------
void CPlayer::UpdateAnimGraph(IAnimationGraphState * pState)
{
	CItem *pItem = static_cast<CItem*>(GetCurrentItem());
	CItem::TempAGInputName pose = "nw";
	CItem::TempAGInputName suffix = "";
	if (pItem)
	{
		//--- Update animParams
		pItem->GetAnimGraphInputs(pose, suffix);
		pose = IsSliding() ? pItem->GetParams().slidePose.c_str() : pose.c_str();
	}

	pState->SetInput( m_inputItem, pose.c_str() );
	pState->SetVariationInput(m_varInputAimMode, suffix.c_str());
	
	pState->SetInput( m_inputSuitMode, "tac" );

	CActor::UpdateAnimGraph( pState );
}

void CPlayer::PostUpdate(float frameTime)
{
	CActor::PostUpdate(frameTime);
	if (m_pPlayerInput.get())
		m_pPlayerInput->PostUpdate();
}

void CPlayer::PostRemoteSpawn()
{
	CActor::PostRemoteSpawn();
#if USE_VOICELISTENER
	if(gEnv->bMultiplayer && !IsClient())
	{
		// remote players need a voice listener
		if(!m_pVoiceListener)
		{
			m_pVoiceListener = GetGameObject()->AcquireExtension("VoiceListener");
		}
	}
#endif
}

void CPlayer::CameraShake(float angle,float shift,float duration,float frequency,Vec3 pos,int ID,const char* source) 
{
	float angleAmount(max(-90.0f,min(90.0f,angle)) * gf_PI/180.0f);
	float shiftAmount(shift);
  
  if (IVehicle* pVehicle = GetLinkedVehicle())
  {
    if (IVehicleSeat* pSeat = pVehicle->GetSeatForPassenger(GetEntityId()))    
      pSeat->OnCameraShake(angleAmount, shiftAmount, pos, source);    
  }

	Ang3 shakeAngle(\
		RANDOMR(0.0f,1.0f)*angleAmount*0.15f, 
		(angleAmount*min(1.0f,max(-1.0f,RANDOM()*7.7f)))*1.15f,
		RANDOM()*angleAmount*0.05f
		);

	Vec3 shakeShift(RANDOM()*shiftAmount,0,RANDOM()*shiftAmount);

	IView *pView = g_pGame->GetIGameFramework()->GetIViewSystem()->GetViewByEntityId(GetEntityId());
	if (pView)
		pView->SetViewShake(shakeAngle,shakeShift,duration,frequency,0.5f,ID);

	/*//if a position is defined, execute directional shake
	if (pos.len2()>0.01f)
	{
	Vec3 delta(pos - GetEntity()->GetWorldPos());
	delta.NormalizeSafe();

	float dotSide(delta * m_viewQuatFinal.GetColumn0());
	float dotFront(delta * m_viewQuatFinal.GetColumn1() - delta * m_viewQuatFinal.GetColumn2());

	float randomRatio(0.5f);
	dotSide += RANDOM() * randomRatio;
	dotFront += RANDOM() * randomRatio;

	m_viewShake.angle.Set(dotFront*shakeAngle, -dotSide*shakeAngle*RANDOM()*0.5f, dotSide*shakeAngle);
	}
	else
	{
	m_viewShake.angle.Set(RANDOMR(0.0f,1.0f)*shakeAngle, RANDOM()*shakeAngle*0.15f, RANDOM()*shakeAngle*0.75f);
	}*/
}
  

void CPlayer::ResetAnimations()
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);

	if (pCharacter)
	{
		pCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();

//		if (m_pAnimatedCharacter)
//			m_pAnimatedCharacter->GetAnimationGraphState()->Pause(true, eAGP_StartGame);
		//disable any IK
		//pCharacter->SetLimbIKGoal(LIMB_LEFT_LEG);
		//pCharacter->SetLimbIKGoal(LIMB_RIGHT_LEG);
		//pCharacter->SetLimbIKGoal(LIMB_LEFT_ARM);
		//pCharacter->SetLimbIKGoal(LIMB_RIGHT_ARM);

		//
		for (int i=0;i<BONE_ID_NUM;++i)
		{
			/*int boneID = GetBoneID(i);
			if (boneID>-1)
				pCharacter->GetISkeleton()->SetPlusRotation(boneID, IDENTITY);*/
		}

		pCharacter->GetISkeletonPose()->SetLookIK(false,0,ZERO);
	}
}

void CPlayer::SetHealth(int health )
{
	if(m_stats.isGrabbed)
		health -=1;  //Trigger automatic thrown

	if(health == m_health)
		return;

	SendPerkEvent(EPE_OverrideHealth, &health);

	float oldHealth = m_health;

	CActor::SetHealth(health);

	CALL_PLAYER_EVENT_LISTENERS(OnHealthChanged(this, m_health));

	if(oldHealth > m_health)
	{
		InformHealthHasBeenReduced();
	}

	if ((health <= 0) && (IsGod() > 0))
	{
		CALL_PLAYER_EVENT_LISTENERS(OnDeath(this, true));
	}

	if (oldHealth > 0.0f && GetHealth() <= 0)	//client deathFX are triggered in the lua gamerules
	{
		StopLoopingSounds();

		SetDeathTimer();

		if(IsClient())
		{
			CAnnouncer::GetInstance()->Announce(GetEntityId(), "PlayerDeath");

			SAFE_HUD_FUNC(GetRadar()->Reset());
			SAFE_HUD_FUNC(BreakHUD(2));
			SAFE_HUD_FUNC(OnSetActorItem(NULL,NULL));

			if(IItem *pItem = GetCurrentItem(false))
			{
				pItem->Select(false);
			}

			CCCPOINT(PlayerState_LocalPlayerDied);
		}
		else
		{
			CCCPOINT(PlayerState_NonLocalPlayerDied);
		}

		// report normal death
		CALL_PLAYER_EVENT_LISTENERS(OnDeath(this, false));
		g_pGame->GetGameRules()->OnActorDeath( this );
		if (gEnv->bMultiplayer == false && IsClient())
			g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(GetEntity(), eGE_Death);

		SendMusicLogicEvent(eMUSICLOGICEVENT_PLAYER_KILLED);

		SendPerkEvent(EPE_Die);
	}

	if(IsClient())
		UpdateClientHealthFX(oldHealth);

	CHANGED_NETWORK_STATE(this, ASPECT_HEALTH);
}

//------------------------------------------------
void CPlayer::UpdateClientHealthFX(float oldHealth)
{
	assert (IsClient());

	static float lastIntensity = 0.2f;
	const float blendTime = 0.5f;

	//Client damage screen FX
	IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
	TMFXEffectId fxId = pMaterialEffects->GetEffectIdByName("suit_fx", "player_damaged");

	const float healthThrLow = g_pGameCVars->g_playerLowHealthThreshold;

	SMFXRunTimeEffectParams params;
	params.pos = GetEntity()->GetWorldPos();
	params.soundSemantic = eSoundSemantic_Player_Foley;
	if(m_health <= 0)
	{
		SMFXCustomParamValue fxParamValue;
		fxParamValue.fValue = 1.5f;
		pMaterialEffects->SetCustomParameter(fxId, "BlendOutTime", fxParamValue);
	}
	else if(m_health <= healthThrLow && oldHealth > healthThrLow)
	{
		IAIObject* pAI = GetEntity() ? GetEntity()->GetAI() : 0;
		if (pAI)
		{
			pAI->Event(AIEVENT_LOWHEALTH, 0);
		}

		pMaterialEffects->StopEffect(fxId);
		pMaterialEffects->ExecuteEffect(fxId, params);
		SMFXCustomParamValue fxParamValue;
		fxParamValue.fValue = lastIntensity = 0.2f;
		pMaterialEffects->SetCustomParameter(fxId, "Intensity", fxParamValue);
	}
	else if((m_health > healthThrLow) && (oldHealth <= healthThrLow))
	{
		SMFXCustomParamValue fxParamValue;
		fxParamValue.fValue = 1.5f;
		pMaterialEffects->SetCustomParameter(fxId, "BlendOutTime", fxParamValue);
	}
	else if((m_health > 0.0f) && (m_health <= healthThrLow))
	{
		SMFXCustomParamValue fxParamValue;
		fxParamValue.fValue = lastIntensity = LERP( lastIntensity, max(0.2f, 1.0f - (m_health / healthThrLow)), blendTime);
		pMaterialEffects->SetCustomParameter(fxId, "Intensity", fxParamValue);
	}

	const static float k_minDamageForHit = 10.0f;	//this is partly to deal with small differences in health over the network
	if(m_health > 0 && m_health < oldHealth - k_minDamageForHit)
	{
		SendMusicLogicEvent(eMUSICLOGICEVENT_PLAYER_WOUNDED);

		if(gEnv->bMultiplayer)
		{
			CAnnouncer::GetInstance()->Announce(GetEntityId(), "PlayerHit");
		}
	}

	float barFraction = 0.f;
	float fullScreenRedness = 1.f;
	
	if(!gEnv->bMultiplayer)
		fullScreenRedness = 0.f;
	
	if (m_health > 0.0f) 
	{
		const float maxHealth = (float)GetMaxHealth();
		barFraction = min(min(m_health, maxHealth) * __fres(maxHealth), 1.0f); // Thanks to network sync compression, actualHealth CAN exceed maxHealth :|

		if(gEnv->bMultiplayer)
		{
			fullScreenRedness = 1.f - barFraction;
		}
	}
	else if(GetSpectatorMode() != CActor::eASM_None)
	{
		fullScreenRedness = 0.0f;
	}

	CUISimpleBar * healthBar = CUISimpleBar::GetInstanceWithName("HealthBar");
	if (healthBar)
	{
		healthBar->Set(barFraction, barFraction > 0.f);
	}

	CUICoverEntireScreen * redWhenDamaged = CUICoverEntireScreen::GetInstanceWithName("RedWhenDamaged");
	if (redWhenDamaged)
	{
		redWhenDamaged->Set(fullScreenRedness);
	}
}

//-------------------------------------------------
// This calculates the regeneration amount in a given amount of time, assuming that the
// m_lastTimeDamaged value was updated once or less during that period

float CPlayer::GetRegenerationAmount(float frameTime)
{
	const float elapsedTime = gEnv->pTimer->GetAsyncTime().GetSeconds() - m_lastTimeDamaged.GetSeconds();
	float regenAmount = 0.0f;
	
	if (gEnv->bMultiplayer)
	{
		const float timeThreshold = g_pGameCVars->pl_health.normal_threshold_time_to_regenerateMP;
		const float recharge = g_pGameCVars->pl_health.normal_regeneration_rateMP * frameTime;

		regenAmount = (float)__fsel((timeThreshold-elapsedTime), 0.0f, recharge);
	}
	else
	{
		const float timeThreshold = g_pGameCVars->pl_health.normal_threshold_time_to_regenerateSP;
		const float recharge = g_pGameCVars->pl_health.normal_regeneration_rateSP * frameTime;

		regenAmount = (float)__fsel((timeThreshold-elapsedTime), 0.0f, recharge);
	}
	return regenAmount;
}

//-------------------------------------------------
void CPlayer::UpdateHealthRegeneration(int iHealth, float frameTime)
{
	//Should only update if there is a suit
	if(HasNanoSuit() && !GetPerkData<bool>(EPD_SelfDestructMinigame))
	{
		//Check if it's possible to update
		const float regenAmount = GetRegenerationAmount(frameTime);
		if(regenAmount <= 0.0f)
			return;
	
		//Health update
		const int maxHealth = GetMaxHealth();

		if(iHealth < maxHealth)
		{
			const float healthRegen = m_healthAccumError + regenAmount;
			const float fHealthRegenRoundDown = floorf(healthRegen);
			const int iHealthRegen = (int)healthRegen;
			m_healthAccumError = healthRegen - fHealthRegenRoundDown;

			if(iHealthRegen > 0)
			{
				CCCPOINT_IF(IsClient(), PlayerState_LocalPlayerHealthRegenerate);
				CCCPOINT_IF(! IsClient(), PlayerState_OtherPlayerHealthRegenerate);

				SetHealth(min(iHealth+iHealthRegen,maxHealth));
			}
		}
	}
	else
	{
		WATCH_ACTOR_STATE ("health not regenerating because %s", HasNanoSuit() ? "playing self destruct minigame" : "has no nano-suit");
	}
}

void CPlayer::SerializeXML( XmlNodeRef& node, bool bLoading )
{
}

void CPlayer::SetAuthority( bool auth )
{
	// we've been given authority of this entity, mark the physics as changed
	// so that we send a current position, failure to do this can result in server/client
	// disagreeing on where the entity is. most likely to happen on restart
	if(auth)
	{
		CHANGED_NETWORK_STATE(this, eEA_Physics);
	}
}

//------------------------------------------------------------------------

void CPlayer::SetAngles(const Ang3 &angles) 
{
	Matrix33 rot(Matrix33::CreateRotationXYZ(angles));
	CMovementRequest mr;
	mr.SetLookTarget( GetEntity()->GetWorldPos() + 20.0f * rot.GetColumn(1) );
	m_pMovementController->RequestMovement(mr);
}

Ang3 CPlayer::GetAngles() 
{
	if(IsClient() && GetLinkedVehicle())
		return Ang3(m_clientViewMatrix);

	return Ang3(m_viewQuatFinal.GetNormalized());
}

void CPlayer::AddAngularImpulse(const Ang3 &angular,float deceleration,float duration)
{
	m_stats.angularImpulse = angular;
	m_stats.angularImpulseDeceleration = deceleration;
	m_stats.angularImpulseTimeMax = m_stats.angularImpulseTime = duration;
}

void CPlayer::SelectNextItem(int direction, bool keepHistory, const char *category)
{
	IItem* pItem = GetCurrentItem();
	bool ripedOff = pItem && (static_cast<CItem*>(pItem)->IsRippedOff());

	if(!CanSwitchItems())
		return;
	
	if(ripedOff)
		UseItem(pItem->GetEntityId());
	else
		CActor::SelectNextItem(direction, keepHistory, category);
}

void CPlayer::HolsterItem(bool holster)
{
	CActor::HolsterItem(holster);

	CHANGED_NETWORK_STATE(this, ASPECT_CURRENT_ITEM);
}

void CPlayer::SelectLastItem(bool keepHistory, bool forceNext /* = false */)
{
	if (IsInPickAndThrowMode())
		return;
		
	CActor::SelectLastItem(keepHistory, forceNext);

	//CHANGED_NETWORK_STATE(this, ASPECT_CURRENT_ITEM);
}

void CPlayer::SelectItemByName(const char *name, bool keepHistory)
{
	if (IsInPickAndThrowMode())
		return;
		
	CActor::SelectItemByName(name, keepHistory);

	//CHANGED_NETWORK_STATE(this, ASPECT_CURRENT_ITEM);
}

bool CPlayer::ScheduleItemSwitch(EntityId itemId, bool keepHistory)
{
	if(CActor::ScheduleItemSwitch(itemId, keepHistory))
	{
		CHANGED_NETWORK_STATE(this, ASPECT_CURRENT_ITEM);
		return true;
	}

	return false;
}

void CPlayer::SelectItem(EntityId itemId, bool keepHistory)
{
	if (IsInPickAndThrowMode())
		return;

	CActor::SelectItem(itemId, keepHistory);

	ResetFPWeapon();

	CHANGED_NETWORK_STATE(this, ASPECT_CURRENT_ITEM);
}

bool CPlayer::SetAspectProfile(EEntityAspects aspect, uint8 profile )
{
	uint8 currentphys=GetGameObject()->GetAspectProfile(eEA_Physics);
	bool setViewFromOrientation = false;

	if (aspect == eEA_Physics)
	{
		if (profile==eAP_Alive && currentphys==eAP_Sleep)
		{
			m_stats.isStandingUp=true;
			m_stats.isRagDoll=false;
			setViewFromOrientation = true;
			m_standUpTimer = 4;
		}
	}

	bool res=CActor::SetAspectProfile(aspect, profile);

	if (res && setViewFromOrientation)
	{
		QuatT quat( GetEntity()->GetWorldTM() );
		SetViewRotation( quat.q ); // sets the view to the actual orientation. else, the actor would try to rotate back to the view position while standing up, or jump after)
	}

	return res;
}

void CPlayer::InformHealthHasBeenReduced()
{
	CCCPOINT_IF(IsClient(), PlayerState_LocalPlayerHealthReduced);
	CCCPOINT_IF(! IsClient(), PlayerState_OtherPlayerHealthReduced);

	m_lastTimeDamaged = gEnv->pTimer->GetAsyncTime();
}

bool CPlayer::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (!CActor::NetSerialize(ser, aspect, profile, flags))
		return false;

	bool reading=ser.IsReading();

	if (aspect == ASPECT_HEALTH)
	{
		NET_PROFILE_SCOPE("Health", ser.IsReading());
		if(reading)
		{
			float health = 0.0f;
			ser.Value("health", health, 'hlth');
			float newHealth = (health * 0.01f) * m_maxHealth;
			int		iNewHealth = (int)newHealth;
			//--- Ensure the health recharge time works correctly for remote players
			float healthRegeneration = 0.0f;
			float currentFrameTime = gEnv->pTimer->GetAsyncTime().GetSeconds();
			
			if(m_health < m_maxHealth)
			{
				float timeSinceLastHealthSync = (currentFrameTime - m_timeOfLastHealthSync);
				healthRegeneration = GetRegenerationAmount(timeSinceLastHealthSync);
			}

			if (newHealth < m_health - healthRegeneration)	//account for health regeneration
			{
				InformHealthHasBeenReduced();
			}

			SetHealth(iNewHealth);
			m_timeOfLastHealthSync = currentFrameTime;
		}
		else
		{
			float health = m_health * 100.f / m_maxHealth;
			ser.Value("health", health, 'hlth');
		}
	}    
	if (aspect == ASPECT_CURRENT_ITEM)
	{
		NET_PROFILE_SCOPE("Item", ser.IsReading());

		ser.Value("currentItemId", static_cast<CActor*>(this), &CActor::NetGetCurrentItem, &CActor::NetSetCurrentItem, 'eid');
		ser.Value("scheduledItemId", static_cast<CActor*>(this), &CActor::NetGetScheduledItem, &CActor::NetSetScheduledItem, 'eid');
	}

	NET_PROFILE_BEGIN("Perks", ser.IsReading());
	for(int i = 0; i < ePerk_Last; i++)
	{
		if(m_perkInstances[i])
		{
			m_perkInstances[i]->NetSerialize(ser, aspect, profile, flags);
			if (aspect == CPlayer::ASPECT_PERK_SETTINGS_CLIENT)
			{
				uint8 tier = 0;
				if(ser.IsWriting())
				{
					tier = (uint8) m_perkInstances[i]->GetTier();
				}
				string perkId("perk%d", i);
				ser.Value(perkId.c_str(), tier, 'reld');	//reld - int from 0-3
				if(ser.IsReading())
				{
					m_perkInstances[i]->SetTier((IPerk::EPerkTier) tier);
				}
			}
		}
	}

	m_triggerSniperCountermeasuresPlugin.NetSerialize(ser, aspect, profile, flags);
	m_currentlyTargettingPlugin.NetSerialize(ser, aspect, profile, flags);
	NET_PROFILE_END();

	if (aspect == CPlayer::ASPECT_PERK_SETTINGS_CLIENT)
	{
		NET_PROFILE_SCOPE("PerkSettings", ser.IsReading());
		ser.Value("perkFlags", this, &CPlayer::NetGetPerksBitfield, &CPlayer::NetSetPerksBitfield);
	}

	if(m_pNanoSuit)													// nanosuit needs to be serialized before input
	{
		NET_PROFILE_SCOPE("Nanosuit", ser.IsReading());
		m_pNanoSuit->NetSerialize(ser, aspect);	// because jumping/punching/sprinting energy consumption will vary with suit settings
	}

	if (g_pGame->GetGameRules()->GetPlayerStatsModule())
	{
		NET_PROFILE_SCOPE("PlayerStats", ser.IsReading());
		g_pGame->GetGameRules()->GetPlayerStatsModule()->NetSerialize(GetEntityId(), ser, aspect, profile, flags);
	}

	if (aspect == CPlayer::ASPECT_INPUT_CLIENT)
	{
		NET_PROFILE_SCOPE("PlayerInput", ser.IsReading());
		SSerializedPlayerInput serializedInput;
		if (m_pPlayerInput.get() && ser.IsWriting())
			m_pPlayerInput->GetState(serializedInput);

		serializedInput.Serialize(ser);
		assert(/*serializedInput.stance > STANCE_NULL &&*/ serializedInput.stance < STANCE_LAST);	// asserting here catches both read and write

		if (ser.IsReading())
		{
			if(m_pPlayerInput.get())
			{
				m_pPlayerInput->SetState(serializedInput);
			}
			else
			{
				m_recvPlayerInput = serializedInput;
				m_recvPlayerInput.isDirty = true;
			}
		}

		ser.Value("superJumping", m_isSuperJumping, 'bool');
		ser.Value("jumpCounter", this, &CPlayer::GetJumpCounter, &CPlayer::SetJumpCounter, 'ui3');
	
		const SPlayerStats::ESlidingState prevSlideState = m_stats.slideStats.slidingState;
		int slidingState = m_stats.slideStats.slidingState;
		ser.Value("sliding", slidingState , 'ui2');
		if (ser.IsReading() && (slidingState != prevSlideState))
		{
			SetSlideState((SPlayerStats::ESlidingState) slidingState);
		}

		//Gliding Net serialization, enable for testing
		//bool isAirFrictionActivated = m_stats.isAirFrictionActivated;
		//ser.Value("gliding", m_stats.isAirFrictionActivated, 'bool');
		//if (ser.IsReading() && (m_stats.isAirFrictionActivated != isAirFrictionActivated))
		//{
			//EnableAirFriction(m_stats.isAirFrictionActivated);
		//}
	}

	if(aspect == ASPECT_VEHICLEVIEWDIR_CLIENT)
	{
		ser.Value("VehicleViewRotation", m_vehicleViewDir, 'dir0');
	}

	if(aspect == ASPECT_SPECTATOR)
	{
		NET_PROFILE_SCOPE("Spectator", ser.IsReading());

		SSpectatorInfo*  spinf = &m_stats.spectatorInfo;
		uint8 mode = 0;
		EntityId id = 0;

		if(!reading)
		{
			EntityId *ptr = spinf->GetOtherEntIdPtrForCurMode();
			mode = spinf->mode;
			id = ptr ? *ptr : 0;
		}

		ser.Value("mode", mode, 'spec');
		ser.Value("id", id, 'eid');

		if(reading && mode != eASM_None)	// the only way to disable spectator mode currently should be through spawning
		{
			SetSpectatorModeAndOtherEntId(mode, id);
		}
	}

	if(aspect == CPlayer::ASPECT_RANK_CLIENT)
	{
		NET_PROFILE_SCOPE("Ranks", ser.IsReading());
		if(!reading)
		{
			m_rank = GetRank(false);
		}
		ser.Value("rank", m_rank, 'i8');
		if(reading && !IsClient())
		{
			SetRemotePlayerRank(m_rank);
		}
		//ser.Value("rank", this, &CPlayer::GetRank, &CPlayer::SetRank, 'i8');
	}

#if ENABLE_MP_SPAWNMODES
	if (aspect == ASPECT_SPAWNMODE_TYPE)
	{
		NET_PROFILE_SCOPE("SpawnModeType", ser.IsReading());
		ser.Value("spawnModeType", m_spawnmode_type, 'ui2');
	}
#endif

	return true;
}

void CPlayer::FullSerialize( TSerialize ser )
{
	CActor::FullSerialize(ser);

	m_pMovementController->Serialize(ser);

	if(ser.IsReading())
	{
		CCCPOINT(PlayerState_FullSerializeRead);
		ResetScreenFX();
	}
	else
	{
		CCCPOINT(PlayerState_FullSerializeWrite);
	}

	ser.BeginGroup( "BasicProperties" );
	//ser.EnumValue("stance", this, &CPlayer::GetStance, &CPlayer::SetStance, STANCE_NULL, STANCE_LAST);		
	// skip matrices... not supported
	ser.Value( "velocity", m_velocity );
	ser.Value( "feetWpos0", m_feetWpos[0] );
	ser.Value( "feetWpos1", m_feetWpos[1] );
	// skip animation to play for now
	// skip currAnimW
	ser.Value( "eyeOffset", m_eyeOffset );
	ser.Value( "viewAngles" , m_viewAngles );
	ser.Value( "viewAnglesOffset", m_viewAnglesOffset );
	ser.Value( "angleOffset", m_angleOffset );
	ser.Value( "headAngles", m_headAngles );
	ser.Value( "viewRoll", m_viewRoll );
	ser.Value( "upVector", m_upVector );

	//[AlexMcC|19.03.10]: TODO: delete these once we stop reviving players on quick load!
	// When we don't revive, these are overwritten by a calculation that reads from m_viewAngles,
	// which we serialize above.
	ser.Value( "viewQuat", m_viewQuat );
	ser.Value( "viewQuatFinal", m_viewQuatFinal );
	ser.Value( "baseQuat", m_baseQuat );

	ser.Value( "weaponOffset", m_weaponOffset ); 
	ser.EndGroup();

	//serializing stats
	m_stats.Serialize(ser, eEA_All);
	
	if(ser.BeginOptionalGroup("haveNanoSuit", m_pNanoSuit != NULL))
	{
		m_pNanoSuit->Serialize(ser);
		ser.EndGroup();
	}

	//input-actions
	bool hasInput = (m_pPlayerInput.get())?true:false;
	ser.Value("PlayerInputExists", hasInput);
	if(hasInput)
	{
		if(ser.IsReading() && !(m_pPlayerInput.get()))
			m_pPlayerInput.reset( new CPlayerInput(this) );
		m_pPlayerInput.get()->SerializeSaveGame(ser);
	}

	ser.Value("mountedWeapon", m_stats.mountedWeaponID);

	if (IsClient())
	{
		// Staging params
		if (ser.IsWriting())
		{
			m_stagingParams.Serialize(ser);
		}
		else
		{
			SStagingParams stagingParams;
			stagingParams.Serialize(ser);
			if (stagingParams.bActive || stagingParams.bActive != m_stagingParams.bActive)
				StagePlayer(stagingParams.bActive, &stagingParams);
		}
	}
}

void CPlayer::PostSerialize()
{
	CActor::PostSerialize();

	StopLoopingSounds();

	if (IsClient())
	{
		if(m_pVehicleClient)
			m_pVehicleClient->Reset();
		SupressViewBlending();
	}
}

void SPlayerStats::Serialize(TSerialize ser, EEntityAspects aspects)
{
	assert( ser.GetSerializationTarget() != eST_Network );
	ser.BeginGroup("PlayerStats");

	if (ser.GetSerializationTarget() != eST_Network)
	{
		//when reading, reset the structure first.
		if (ser.IsReading())
			*this = SPlayerStats();

		ser.Value("inAir", inAir);
		ser.Value("onGround", onGround);
		ser.Value("landed", landed);
		ser.Value("jumped", jumped);
		ser.Value("inMovement", inMovement);
		ser.Value("inRest", inRest);
		ser.Value("inWater", inWaterTimer);
		ser.Value("waterLevel", relativeWaterLevel);
		ser.Value("flatSpeed", speedFlat);
		ser.Value("gravity", gravity);
		//ser.Value("mass", mass);		//serialized in Actor::SerializeProfile already ...
		ser.Value("leanAmount", leanAmount);
		ser.Value("fallSpeed", fallSpeed);
		ser.Value("isFiring", isFiring);
		ser.Value("isRagDoll", isRagDoll);
		followCharacterHead.Serialize(ser, "followCharacterHead");
		firstPersonBody.Serialize(ser, "firstPersonBody");
		ser.Value("velocity", velocity);
		ser.Value("velocityUnconstrained", velocityUnconstrained);
		ser.Value("wasFlying", wasFlying);
		ser.Value("stuckTimeout", stuckTimeout);

		ser.Value("upVector", upVector);
		ser.Value("groundNormal", groundNormal);
		ser.Value("isThirdPerson", isThirdPerson);
    isHidden.Serialize(ser, "isHidden");

		//FIXME:serialize cheats or not?
		//int godMode(g_pGameCVars->g_godMode);
		//ser.Value("godMode", godMode);
		//g_pGameCVars->g_godMode = godMode;
		//ser.Value("flyMode", flyMode);
		//

		ser.Value("playerRotation", playerRotation);

		ser.Value("forceCrouchTime", forceCrouchTime);    

	}

	ser.EndGroup();
}

bool CPlayer::CreateCodeEvent(SmartScriptTable &rTable)
{
	const char *event = NULL;
	rTable->GetValue("event",event);

	if (event && !strcmp(event,"addSuitEnergy") && m_pNanoSuit)
	{
		float amount(0.0f);
		rTable->GetValue("amount",amount);

		//m_pNanoSuit->SetSuitEnergy(m_pNanoSuit->GetSuitEnergy() + amount);

		return true;
	}
	else
	{
		return CActor::CreateCodeEvent(rTable);
	}
}

void CPlayer::PlayAction(const char *action,const char *extension, bool looping) 
{
	if (!m_pAnimatedCharacter)
		return;

	if (extension==NULL || strcmp(extension,"ignore")!=0)
	{
	if (extension && extension[0])
			cry_strncpy(m_params.animationAppendix,extension,sizeof(m_params.animationAppendix));
		else
			strcpy(m_params.animationAppendix,"nw");

		m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( m_inputItem, m_params.animationAppendix );
	}

	if (looping)
		m_pAnimatedCharacter->GetAnimationGraphState()->SetInputOptional( m_inputAction, action );
	else
		m_pAnimatedCharacter->GetAnimationGraphState()->SetInputOptional( m_inputSignal, action );
}

void CPlayer::AnimationControlled(bool activate)
{
	if (m_stats.animationControlled != activate)
	{
		m_stats.animationControlled = activate;
		m_stats.followCharacterHead = activate?1:0;

		UpdateVisibility(); 
		//HolsterItem(activate);
	}
}

void CPlayer::OnCollision(EventPhysCollision *physCollision)
{
	int sourceIdx = 0;
	int playerIdx = 1;

	if (physCollision->pEntity[sourceIdx] == GetEntity()->GetPhysics())
	{
		sourceIdx = 1;
		playerIdx = 0;
	}

	
	//TODO: if(!LivingEntity)
	{
		//Collision with terrain
		Vec3 movementDirection = GetLastRequestedVelocity();
		movementDirection.Normalize();
		float dot = physCollision->n.Dot(movementDirection);
		if(dot < -0.85f)
		{
			m_stats.hasRunIntoSomething = true;
		}
	}
}

void CPlayer::HandleEvent( const SGameObjectEvent& event )
{
	bool bHandled = false;

	switch(event.event)
	{
	case eGFE_OnCollision:
		{
			EventPhysCollision *physCollision = reinterpret_cast<EventPhysCollision *>(event.ptr);
			OnCollision(physCollision);

			CRY_ASSERT(physCollision);
			if (m_pHitDeathReactions)
				m_pHitDeathReactions->OnCollision(*physCollision);
		}
		break;
	case eCGE_AnimateHands:
		CreateScriptEvent("AnimateHands",0,(const char *)event.param);
		break;
	case eCGE_SetTeam:
		{
			const STeamChangeInfo * teamChangeInfo = (const STeamChangeInfo *)event.param;
			CryLog("CPlayer::HandleEvent being told SetTeam(%d=>%d) for '%s'", teamChangeInfo->oldTeamNum, teamChangeInfo->newTeamNum, GetEntity()->GetName());
			SendPerkEvent(EPE_SetTeam, event.param);

			SHUDEvent hudevent(eHUDEvent_PlayerSwitchTeams);
			hudevent.AddData(SHUDEventData((int)GetEntityId()));
			CHUD::CallEvent(hudevent);

			OnChangeTeam();
		}
		break;
	case eCGE_Launch:
		{
			//--- Initiate rag-doll between now & the end of the animation
			CAnimationPlayerProxy *animPlayerProxy = GetAnimatedCharacter()->GetAnimationPlayerProxy(0);
			const CAnimation *animPtr = animPlayerProxy ? animPlayerProxy->GetTopAnimation(GetEntity(), 0) : NULL;
			if (animPtr)
			{
				IAnimationSet *animSet = GetEntity()->GetCharacter(0)->GetIAnimationSet();
				float duration = animSet->GetDuration_sec(animPtr->m_Parametric.m_nAnimID[0]);
				m_ragdollTime = duration * (1.0f - animPtr->m_fAnimTime);
			}
		}
		break;
	case eCGE_CoverTransitionEnter:
		{	
			const float FAST_AIM_IK_FADE_OUT = 0.2f;
			ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
			if (pCharacter)
				pCharacter->GetISkeletonPose()->SetAimIKFadeOutSpeed(min(FAST_AIM_IK_FADE_OUT, m_params.aimIKFadeDuration));
		}
		break;
	case eCGE_CoverTransitionExit:
		{
			ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
			if (pCharacter)
				pCharacter->GetISkeletonPose()->SetAimIKFadeOutSpeed(m_params.aimIKFadeDuration);
		}
		break;
	default:
		{
			// HitDeathReactions event handling
			bHandled = m_pHitDeathReactions && m_pHitDeathReactions->HandleEvent(event);

			if (!bHandled)
			{
				CActor::HandleEvent(event);

				if (event.event == eGFE_BecomeLocalPlayer && !(gEnv->bHostMigrating && gEnv->bServer))
				{
					SetActorModel();
					InitActorAttachments();
					Physicalize();

					if (gEnv->bMultiplayer)
					{
						EnterPlayerPlugin(& m_scoreRewardsPlugin);
					}

					if(g_pGame->GetScreenEffects())
						g_pGame->GetScreenEffects()->SetClientActor(this);

					ResetFPView();
					UnRegisterInAutoAimManager();
					if(!gEnv->bEditor)
					{
						SNanoSuitEvent event;
						event.event = eNanoSuitEvent_SUIT_CLIENTSTARTUP;
						SendActorSuitEvent(event);
					}

					//Set Actor model, destroys attachments, and makes weapon invisible, re-attach again
					if (CItem* pCurrentItem = static_cast<CItem*>(GetCurrentItem()))
					{
						pCurrentItem->AttachToHand(true);
					}

					IScriptTable *pEntityScript = GetEntity()->GetScriptTable();
					if (pEntityScript)
					{
						pEntityScript->SetValue("blurType", g_pGameCVars->cl_motionBlur);
						pEntityScript->SetValue("sprintBlur", g_pGameCVars->cl_sprintBlur);
					}

					if (gEnv->bMultiplayer)
					{
						const SHUDEvent hudevent_rescanActors(eHUDEvent_RescanActors);
						CHUD::CallEvent(hudevent_rescanActors);

						// TODO: When the join game GUI goes in the place where the first loadout is set will be moved.
						if (gEnv->bClient && (GetEntityId() == gEnv->pGame->GetIGameFramework()->GetClientActorId()))
						{
							if (CEquipmentLoadout *equipmentLoadout = g_pGame->GetGameRules()->GetEquipmentLoadout())
								equipmentLoadout->ClSendCurrentEquipmentLoadout(GetChannelId());

							if (gEnv->pSystem->IsDedicated())
							{
								IGameRulesSpawningModule *pSpawningModule = g_pGame->GetGameRules()->GetSpawningModule();
								if (pSpawningModule)
								{
									CryLog("CPlayer Dedicated Client Requesting revive");
									pSpawningModule->ClRequestRevive(GetEntityId());
								}
							}
						}
					}
				}
			}
			break;
		}
	}
}

void CPlayer::UpdateGrab(float frameTime)
{
	CActor::UpdateGrab(frameTime);
}

//TODO: clean it up, less redundancy, more efficiency etc..
void CPlayer::ProcessIKLegs(ICharacterInstance *pCharacter,float frameTime)
{
	if (GetGameObject()->IsProbablyDistant() && !GetGameObject()->IsProbablyVisible())
		return;

	static bool bOnce = true;
	static int nDrawIK = 0;
	static int nNoIK = 0;
	if (bOnce)
	{
		bOnce = false;
		REGISTER_CVAR2( "player_DrawIK",&nDrawIK,0,VF_NULL,"" );
		REGISTER_CVAR2( "player_NoIK",&nNoIK,0,VF_NULL,"" );
	}

	if (nNoIK)
	{
		//pCharacter->SetLimbIKGoal(LIMB_LEFT_LEG);
		//pCharacter->SetLimbIKGoal(LIMB_RIGHT_LEG);
		return;
	}

	//Vec3 localCenter(GetEntity()->GetSlotLocalTM(0,false).GetTranslation());
	int32 id = GetBoneID(BONE_BIP01);
	Vec3 localCenter(0,0,0);
	if (id>=0)
		//localCenter = bip01->GetBonePosition();
		localCenter = pCharacter->GetISkeletonPose()->GetAbsJointByID(id).t;

	Vec3 feetLpos[2];

	Matrix33 transMtx(GetEntity()->GetSlotWorldTM(0));
	transMtx.Invert();

	for (int i=0;i<2;++i)
	{
	//	int limb = (i==0)?LIMB_LEFT_LEG:LIMB_RIGHT_LEG;
		feetLpos[i] = Vec3(ZERO); //pCharacter->GetLimbEndPos(limb);

		Vec3 feetWpos = GetEntity()->GetSlotWorldTM(0) * feetLpos[i];
		ray_hit hit;

		float testExcursion(localCenter.z);

		int rayFlags = (COLLISION_RAY_PIERCABILITY & rwi_pierceability_mask);
		if (gEnv->pPhysicalWorld->RayWorldIntersection(feetWpos + m_baseQuat.GetColumn2()*testExcursion, m_baseQuat.GetColumn2()*(testExcursion*-0.95f), ent_terrain|ent_static/*|ent_rigid*/, rayFlags, &hit, 1))
		{		
			m_feetWpos[i] = hit.pt; 
			Vec3 footDelta = transMtx * (m_feetWpos[i] - feetWpos);
			Vec3 newLPos(feetLpos[i] + footDelta);

			//pCharacter->SetLimbIKGoal(limb,newLPos,ik_leg,0,transMtx * hit.n);

			//CryLogAlways("%.1f,%.1f,%.1f",hit.n.x,hit.n.y,hit.n.z);
		}
		//else
		//pCharacter->SetLimbIKGoal(limb);
	}
}

void CPlayer::Landed(float fallSpeed)
{
	if(GetLinkedEntity())
		return;

	static const int objTypes = ent_all;    
	static const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;
	ray_hit hit;
	Vec3 down = Vec3(0,0,-1.0f);
	Vec3 playerPosition = GetEntity()->GetWorldPos();
	IPhysicalEntity *phys = GetEntity()->GetPhysics();
	IMaterialEffects *mfx = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
	

	// No Need to do an expensive immediate linetest, pe_status_living has already gathered a material id!
#if 0
	int matID = mfx->GetDefaultSurfaceIndex();
	int col = gEnv->pPhysicalWorld->RayWorldIntersection(GetEntity()->GetWorldPos(), (down * 5.0f), objTypes, flags, &hit, 1, phys);
	if (col)
	{
		matID = hit.surface_idx;
	}
#endif

	pe_status_living livStat;
	phys->GetStatus(&livStat);
	int matID = livStat.groundSurfaceIdx != -1 ? livStat.groundSurfaceIdx : mfx->GetDefaultSurfaceIndex();
	
	TMFXEffectId effectId = mfx->GetEffectId("bodyfall", matID);
	if (effectId != InvalidEffectId)
	{
		SMFXRunTimeEffectParams params;
		Vec3 direction = Vec3(0,0,0);
		if (IMovementController *pMV = GetMovementController())
		{
			SMovementState state;
			pMV->GetMovementState(state);
			direction = state.aimDirection;
		}
		params.pos = playerPosition + direction;
		params.soundSemantic = eSoundSemantic_Player_Foley;
		
		float landFallParamVal = (float)__fsel( -(fallSpeed - 7.5f), 0.25f, 0.75f);
		params.AddSoundParam("landfall", landFallParamVal);

		const float speedParamVal = min(fabsf((m_stats.velocity.z * 0.1f)), 1.0f);
		params.AddSoundParam("speed", speedParamVal);
		
		params.soundScale = GetPerkData<bool>(EPD_MuteJumping) ? 0.0f : 1.0f;
		mfx->ExecuteEffect(effectId, params);
	}

	if(IsClient())
	{
		if (fallSpeed > 0.0f)
		{
			const float direction = ((cry_rand()%2)==0) ? -1.0f : 1.0f;
			const float shakeTime = IsSuperJumping() ? 0.05f : 0.1f;
			const float fallspeedMultipluer = 0.03f;
			const Vec3 rotation = Vec3(0.4f, 0.2f*direction, 0.1f);
			const float intensity = clamp(fallspeedMultipluer*fallSpeed, 0.0f, 0.5f);
			const float superJumpLandScale = IsSuperJumping() ? 1.5f : 1.0f;
			if (g_pGame->GetScreenEffects())
			{
				g_pGame->GetScreenEffects()->CamShake(rotation*intensity*superJumpLandScale, Vec3(0, 0, 0), shakeTime*superJumpLandScale, shakeTime*superJumpLandScale, 0.05f, CScreenEffects::eCS_GID_Player);
			}
			
			if (gEnv->pInput)
			{
				gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.09f + (fallSpeed * 0.01f), 0.5f, 0.9f*clamp_tpl(fallSpeed*0.2f, 0.1f, 1.0f) ) );
			}

			if(fallSpeed > 7.0f)
			{
				PlaySound(ESound_Fall_Drop);
			}

			EActionSoundParamValues actionSoundParam = EActionSoundParam_Land;
			if (!gEnv->bMultiplayer)
			{
				const float fallSpeed = fabs(m_stats.velocity.z);
				const float thresholdSpeedForHeavyLand = (g_pGameCVars->pl_health.fallDamage_SpeedSafe + g_pGameCVars->pl_health.fallDamage_SpeedFatal) * 0.5f;
				if (fallSpeed >= thresholdSpeedForHeavyLand)
				{
					StartInteractiveActionByName("HeavyLand");
					actionSoundParam = EActionSoundParam_HeavyLand;
				}
			}
			PlayBreathingSound( actionSoundParam );
		}
		CCCPOINT(PlayerMovement_LocalPlayerLanded);
	}
	else
	{
		CCCPOINT(PlayerMovement_NonLocalPlayerLanded);
	}

	IActor *pClientActor=gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (pClientActor && pClientActor->IsPlayer())
	{
		CPlayer *pClientPlayer=static_cast<CPlayer*>(pClientActor);
		EntityId entityId = GetEntityId();
		pClientPlayer->SendPerkEvent(EPE_Footstep, &entityId);
	}

	SendPerkEvent(EPE_Landed, &fallSpeed);
}

//------------------------------------------------------------------------
// animation-based footsteps sound playback
void CPlayer::ExecuteFootStep(ICharacterInstance* pCharacter, const float frameTime, const int32 nFootJointID)
{
	CRY_ASSERT(nFootJointID >= 0);
	m_footstepCounter++;

	const SNanoSuitGameParameters& suitParams = GetActorSuitGameParameters();
	const float noiseSupression = suitParams.GetProps().noiseSupression;

	const bool isThirdPerson = IsThirdPerson();

	ISkeletonAnim* pSkeletonAnim = pCharacter->GetISkeletonAnim();
	CRY_ASSERT(pSkeletonAnim);

	// Old hacky way to normalize to Crysis1 max speed of 7m/s, newer physics sounds are absolute m/s speed parameter
	const float relativeSpeed = pSkeletonAnim->GetCurrentVelocity().GetLength() * 0.142f;

	m_fLastEffectFootStepTime = gEnv->pTimer->GetAsyncCurTime();

	// Sonar Perk
	bool muteFootsteps = GetPerkData<bool>(EPD_MuteFootsteps);
	if(!muteFootsteps)
	{
		if (!(gEnv->bMultiplayer && g_pGameCVars->g_mpPermanentSonarVision))
		{
			EBoneEmitterLocation boneEmitterLocation = (GetBoneID(BONE_FOOT_L)==nFootJointID) ? eBoneEmitter_LeftFoot : eBoneEmitter_RightFoot;
			SonarVisionPerk_SetNumParticlesToCreate(CPerk::GetInstance()->GetVars()->perk_sonarVision_skelAlignedParticles_numToCreate_footstep, boneEmitterLocation);
			SonarVisionPerk_SetNumParticlesToCreate(CPerk::GetInstance()->GetVars()->perk_sonarVision_skelAlignedParticles_numToCreate_random,eBoneEmitter_Skeleton);
		}
	}

	f32 fZRotation = 0.0f;
	Vec3 vDeltaMovment(0,0,0);
	if (frameTime > 0.0f)
	{
		fZRotation = abs( RAD2DEG( pSkeletonAnim->GetRelMovement().q.GetRotZ() ) ) / frameTime;
		Vec3 vRelTrans = pSkeletonAnim->GetRelMovement().t;
		Vec3 vCurrentVel = pSkeletonAnim->GetCurrentVelocity() * frameTime;
		vDeltaMovment = ( vRelTrans - vCurrentVel) / frameTime;
	}

	ISkeletonPose *pSkeletonPose = pCharacter->GetISkeletonPose();
	CRY_ASSERT(pSkeletonPose);

	// Setup FX params
	SMFXRunTimeEffectParams params;
	params.soundProxyEntityId = g_pGameCVars->g_FootstepSoundsFollowEntity ? GetEntityId() : 0;
	params.angle = GetEntity()->GetWorldAngles().z + (gf_PI * 0.5f);
	params.pos = params.decalPos = GetEntity()->GetSlotWorldTM(0) * pSkeletonPose->GetAbsJointByID(nFootJointID).t;
	params.decalPos = params.pos;

	params.AddSoundParam("speed", relativeSpeed);
	params.AddSoundParam("turn", fZRotation);
	params.AddSoundParam("acceleration", vDeltaMovment.y); 
	params.soundSemantic = eSoundSemantic_Physics_Footstep;
	params.soundProxyOffset = GetEntity()->GetWorldTM().GetInverted().TransformVector(params.pos - GetEntity()->GetWorldPos());

	if(m_stats.relativeWaterLevel < 0.0f)
	{
		CreateScriptEvent("splash",0);
	}

	params.playSoundFP = !IsThirdPerson();

	//create effects
	IMaterialEffects* pMaterialEffects = gEnv->pMaterialEffects;

	TMFXEffectId effectId = InvalidEffectId;

	IActor *pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (pClientActor && pClientActor->IsPlayer())
	{
		CPlayer *pClientPlayer=static_cast<CPlayer*>(pClientActor);
		EntityId entityId = GetEntityId();
		pClientPlayer->SendPerkEvent(EPE_Footstep, &entityId);
	}

	if(IsClient())
	{
		SHUDEvent eventFootStep(eHUDEvent_OnPhysicalImpulse);
		eventFootStep.ReserveData(3);
		eventFootStep.AddData(0.0f);
		eventFootStep.AddData(-0.003f);
		eventFootStep.AddData(0.003f);
		CHUD::CallEvent(eventFootStep);
	}

	CryFixedStringT<16> sEffectWater = "water_shallow";

	bool usingWaterEffectId = false;
	const float feetWaterLevel = gEnv->p3DEngine->GetWaterLevel(&params.pos);

	if (feetWaterLevel != WATER_LEVEL_UNKNOWN)
	{
		const float depth = feetWaterLevel - params.pos.z;
		if (depth >= 0.0f)
		{
			usingWaterEffectId = true;

			if (depth > FOOTSTEPS_DEEPWATER_DEPTH)
			{
				sEffectWater = "water_deep";
			}
		}
	}

	// Effect is called by footstepEffectName which is defined in BasicActor.lua as "footstep"
	// and in alien as "footstep_alienname". By this we can play special effects
	if(!usingWaterEffectId)
	{
		effectId = pMaterialEffects->GetEffectId(m_params.footstepEffectName.c_str(), m_stats.groundMaterialIdx);
	}
	else
	{
		effectId = pMaterialEffects->GetEffectIdByName(m_params.footstepEffectName.c_str(), sEffectWater);
	}

	// Gear Effect

	// Gear and Foley sounds will be improved soon in a way that like footsteps, foley signals can be added to the animation
	// which can be processes here as well. For now we play gear for humans and skipp aliens

	TMFXEffectId gearEffectId = InvalidEffectId;
	TMFXEffectId gearSearchEffectId = InvalidEffectId;

	if (!gEnv->bMultiplayer && m_params.footstepGearEffect)
	{
		EStance stance = GetStance();
		float gearEffectPossibility = relativeSpeed*1.3f;


		if(stance == STANCE_CROUCH)
		{
			gearEffectPossibility *= 10.0f;
		}
		else if ((stance == STANCE_STEALTH) && (cry_frand() < 0.3f))
		{
			// this is a Crysis1 hack to play a special gear sound if NKs are in search mode
			if (IAIObject* pAI = GetEntity()->GetAI())
			{
				if (pAI->GetProxy() && pAI->GetProxy()->GetAlertnessState() > 0)
				{
					gearSearchEffectId = pMaterialEffects->GetEffectIdByName(m_params.footstepEffectName.c_str(), "gear_search");
				}
			}
		}

		if(cry_frand() < gearEffectPossibility)
		{
			gearEffectId = pMaterialEffects->GetEffectIdByName(m_params.footstepEffectName.c_str(), "gear");
		}
	}

	bool  useSilentFeet = !usingWaterEffectId && muteFootsteps;
	if (useSilentFeet)
	{
		SendPerkEvent(EPE_OverrideFootstepSoundScale, &params.soundScale);
	}

	//Play effects
	if (m_soundFootstepIndGear.HasValidSignal())
	{
		m_soundFootstepIndGear.Play( GetEntityId(), "action", float(EActionSoundParam_RunWalkIdle));
		m_soundFootstepIndGear.SetParam( GetEntityId(), "speed", m_stats.speedFlat );
	}
	const int BREATH_EVERY_X_STEPS = 3;
	if (IsClient() && (m_footstepCounter%BREATH_EVERY_X_STEPS)==0)
	{
		PlayBreathingSound( EActionSoundParam_RunWalkIdle );
	}
	if(effectId != InvalidEffectId)
	{
		pMaterialEffects->ExecuteEffect(effectId, params);
	}
	if(gearEffectId != InvalidEffectId)
	{
		pMaterialEffects->ExecuteEffect(gearEffectId, params);
	}
	if(gearSearchEffectId != InvalidEffectId)
	{
		pMaterialEffects->ExecuteEffect(gearSearchEffectId, params);
	}

	//////////////////////////////////////////////////////////////////////////
	// DEBUG INFO
	if (g_pGameCVars->g_FootstepSoundsDebug != 0)
	{
		const char* side = "Footstep:";

		const char* color = "";
		if (vDeltaMovment.y < 0.33f)
			color = "$6"; // Yellow
		else if (vDeltaMovment.y < 0.66f)
			color = "$8"; // Orange
		else
			color = "$4"; // Red

		if (gEnv->bEditor)
			CryLogAlways("%s[%s] speed=%3.2f, turn=%3.2f, acceleration=%3.1f", color, GetEntity()->GetName(), relativeSpeed, fZRotation, vDeltaMovment.y);
		else
			CryWatch("%s[%s] speed=%3.2f, turn=%3.2f, acceleration=%3.1f", color, GetEntity()->GetName(), relativeSpeed, fZRotation, vDeltaMovment.y);
	}
	//////////////////////////////////////////////////////////////////////////

	// Don't emit footstep stims if 'silent feet' perk is enabled
	if (!useSilentFeet)
	{
		ExecuteFootStepsAIStimulus(relativeSpeed, noiseSupression);
	}
}


//////////////////////////////////////////////////////////////////////////
void CPlayer::ExecuteFootStepsAIStimulus(const float relativeSpeed, const float noiseSupression)
{
	if (gEnv->pAISystem)
	{
		//handle AI sound recognition *************************************************
		float fStandingRadius = g_pGameCVars->ai_perception.movement_standingRadiusDefault;
		float fCrouchRadius = g_pGameCVars->ai_perception.movement_crouchRadiusDefault;
		float fMovingBaseMult = g_pGameCVars->ai_perception.movement_movingSurfaceDefault;

		if (g_pGameCVars->ai_perception.movement_useSurfaceType > 0)
		{
		IMaterialManager *pMaterialManager = gEnv->p3DEngine->GetMaterialManager();
			CRY_ASSERT(pMaterialManager);
		ISurfaceType *pSurface = pMaterialManager->GetSurfaceTypeManager()->GetSurfaceType(m_stats.groundMaterialIdx);
		if (pSurface) 
		{
				const ISurfaceType::SSurfaceTypeAIParams *pAiParams = pSurface->GetAIParams();
				if (pAiParams)
			{
					fStandingRadius = pAiParams->fFootStepRadius;
					fCrouchRadius = pAiParams->crouchMult;
					fMovingBaseMult = pAiParams->movingMult;
			}
		}
		}

		// Use correct base radius
		float fBaseRadius = fStandingRadius;
		float fMovementRadius = g_pGameCVars->ai_perception.movement_standingMovingMultiplier;
		const EStance stance = GetStance();
		if (!m_stats.bSprinting && stance == STANCE_CROUCH)
		{
			fBaseRadius = fCrouchRadius;
			fMovementRadius = g_pGameCVars->ai_perception.movement_crouchMovingMultiplier;
		}

		const float fFootstepSpeed = m_lastRequestedVelocity.GetLength();
		if (fFootstepSpeed > FLT_EPSILON)
		{
			float fFootstepRadius = fBaseRadius + (fFootstepSpeed * fMovingBaseMult * fMovementRadius);
			fFootstepRadius *= (1.0f - noiseSupression);

		SAIStimulus stim(AISTIM_SOUND, AISOUND_MOVEMENT, GetEntityId(), 0,
				GetEntity()->GetWorldPos(), ZERO, fFootstepRadius);
		gEnv->pAISystem->RegisterStimulus(stim);
	}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CPlayer::ShouldUpdateNextFootStep() const
{
	//Cull distant dudes
	if(!IsClient())
	{
		IActor *pClient = g_pGame->GetIGameFramework()->GetClientActor();
		if(pClient)
		{
			const Vec3& clientPos = pClient->GetEntity()->GetWorldPos();
			const Vec3& actorPos = GetEntity()->GetWorldPos();
			const Vec3 distanceVector = clientPos - actorPos;
			//distance check - running footsteps fx on all AIs is expensive
			if(distanceVector.GetLengthSquared() > g_pGameCVars->g_footstepSoundMaxDistanceSq)
			{
				return false;
			}
		}
	}

	return ((gEnv->pTimer->GetAsyncCurTime() - m_fLastEffectFootStepTime) > 0.2f);
}

//////////////////////////////////////////////////////////////////////////

void CPlayer::SwitchDemoModeSpectator(bool activate)
{
	if(!IsDemoPlayback())
		return;

	m_bDemoModeSpectator = activate;

	m_stats.isThirdPerson = !activate;
	if(activate)
		m_stats.firstPersonBody = (uint8)g_pGameCVars->cl_fpBody;
	CItem *pItem = GetItem(GetInventory()->GetCurrentItem());
	if(pItem)
		pItem->UpdateFPView(0);

	IVehicle* pVehicle = GetLinkedVehicle();
	if (pVehicle)
	{
		IVehicleSeat* pVehicleSeat = pVehicle->GetSeatForPassenger( GetEntityId() );
		if (pVehicleSeat)
			pVehicleSeat->SetView( activate ? pVehicleSeat->GetNextView(InvalidVehicleViewId) : InvalidVehicleViewId );
	}

	if (activate)
	{
		IScriptSystem * pSS = gEnv->pScriptSystem;
		pSS->SetGlobalValue( "g_localActor", GetGameObject()->GetEntity()->GetScriptTable() );
		pSS->SetGlobalValue( "g_localActorId", ScriptHandle( GetGameObject()->GetEntityId() ) );
	}

	m_isClient = activate;
}

//------------------------------------------------------
void CPlayer::SendActorSuitEvent(const SNanoSuitEvent& event)
{
	if(m_pNanoSuit)
		m_pNanoSuit->ProcessEvent(event);
}

//-------------------------------------------------------
const SNanoSuitGameParameters& CPlayer::GetActorSuitGameParameters() const
{
	if(m_pNanoSuit)
		return m_pNanoSuit->GetGameParams();
	else
		return CActor::GetActorSuitGameParameters();
}

//------------------------------------------------------
void CPlayer::InitNanoSuit()
	{
	if(!IsPlayer())
		return;

	if(m_pNanoSuit == NULL)
	{
		m_pNanoSuit = new CNanoSuit(*this);
	}

	m_pNanoSuit->Reset();

	RegisterPlayerSuitEventListener(this);
}

//------------------------------------------------------
void CPlayer::InitCCTVScreenEffect(const IItemParamsNode* pRootNode)
{
	// Initialise cctv screen effect
	const IItemParamsNode* cctvParamNode = pRootNode->GetChild("CCTVScreenEffect");
	if(cctvParamNode)
	{
		m_cctvScreenEffect.Release(); // Allow for reloading effect

		SCCTVGameEffectParams cctvEffectParams;
		cctvEffectParams.xmlNode = cctvParamNode;

		m_cctvScreenEffect.Initialise(&cctvEffectParams);
	}
}

//------------------------------------------------------
void CPlayer::SetFlyMode(uint8 flyMode)
{
	if (GetSpectatorMode())
		return;

	m_stats.flyMode = flyMode;

	if (m_stats.flyMode>2)
		m_stats.flyMode = 0;

	if(m_pAnimatedCharacter)
		m_pAnimatedCharacter->RequestPhysicalColliderMode((m_stats.flyMode==2)?eColliderMode_Disabled:eColliderMode_Undefined, eColliderModeLayer_Game, "Player::SetFlyMode");

	CCCPOINT_IF(flyMode == 0, PlayerState_FlyMode_Off);
	CCCPOINT_IF(flyMode == 1, PlayerState_FlyMode_FlyCollisionsOn);
	CCCPOINT_IF(flyMode == 2, PlayerState_FlyMode_FlyCollisionsOff);
}

void CPlayer::SetSpectatorModeAndOtherEntId(uint8 mode, EntityId othEntId)
{
	//CryLog("%s setting spectator mode %d", GetEntity()->GetName(), mode);

	SSpectatorInfo*  spinf = &m_stats.spectatorInfo;

	uint8 oldSpectatorMode=spinf->mode;

	bool server=gEnv->bServer;

	if(gEnv->bClient && (mode || spinf->mode))
		m_pPlayerInput.reset();

	EntityId localClientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	bool isLocalPlayer = localClientId != 0 && GetEntityId() == localClientId ? true : false;

	if (mode && !spinf->mode)  // turning (any) spectator mode ON
	{
		if(mode == eASM_CCTV && isLocalPlayer)
		{
			// Start cctv screen effect
			m_cctvScreenEffect.SetActive(true);
		}

		if (IVehicle *pVehicle=GetLinkedVehicle())
		{
			if (IVehicleSeat *pSeat=pVehicle->GetSeatForPassenger(GetEntityId()))
				pSeat->Exit(false);
		}

		spinf->Reset();
		spinf->mode = mode;
		spinf->SetOtherEntIdForCurMode(othEntId);

		Revive(kRFR_StartSpectating);

		if (server)
		{
			GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Spectator);
			CHANGED_NETWORK_STATE(this, ASPECT_SPECTATOR);
		}

		Draw(false);

		m_stats.inAir=0.0f;
		m_stats.onGround=0.0f;

		// spectators should be dead
		m_health=-1;
		CHANGED_NETWORK_STATE(this, ASPECT_HEALTH);

		/*
		if(mode == CActor::eASM_Follow)
			MoveToSpectatorTargetPosition();
		*/
	}
	else if (!mode && spinf->mode)  // turning (any) spectator mode OFF
	{
		if (server)
		{
			GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Alive);
			CHANGED_NETWORK_STATE(this, ASPECT_SPECTATOR);
		}

		if(spinf->mode == eASM_CCTV && isLocalPlayer)
		{
			// Stop cctv screen effect
			m_cctvScreenEffect.SetActive(false);
		}

		Draw(true);

		spinf->Reset();  // sets mode = CActor::eASM_None
		spinf->state = CActor::eASS_Ingame;

		m_stats.inAir=0.0f;
		m_stats.onGround=0.0f;
	}
	else if ((mode != spinf->mode) || (othEntId != spinf->GetOtherEntIdForCurMode()))  // mode or data has changed (enough)
	{
		if (server)
		{
			//SetHealth(GetMaxHealth());
			CHANGED_NETWORK_STATE(this, ASPECT_SPECTATOR);
		}

		if(spinf->mode == eASM_CCTV && isLocalPlayer)
		{
			// Stop cctv screen effect
			m_cctvScreenEffect.SetActive(false);
		}

		spinf->Reset();
		spinf->mode = mode;
		spinf->SetOtherEntIdForCurMode(othEntId);

		if(spinf->mode == eASM_CCTV && isLocalPlayer)
		{
			// Start cctv screen effect
			m_cctvScreenEffect.SetActive(true);
		}
		/*
		if(mode == CActor::eASM_Follow)
		{
			MoveToSpectatorTargetPosition();
		}
		*/
	}

	/*
	// switch on/off spectator HUD
	if (IsClient())
		SAFE_HUD_FUNC(Show(mode==0));
	*/
}

void CPlayer::MoveToSpectatorTargetPosition()
{
	// called when our target entity moves.
	IEntity* pTarget = gEnv->pEntitySystem->GetEntity(GetSpectatorTarget());
	if(!pTarget)
		return;

	Matrix34 tm = pTarget->GetWorldTM();
	tm.AddTranslation(Vec3(0,0,2));
	GetEntity()->SetWorldTM(tm);
}

bool CPlayer::UseItem(EntityId itemId)
{
	const bool bOK = CActor::UseItem(itemId);
	if (bOK)
	{
		CALL_PLAYER_EVENT_LISTENERS(OnItemUsed(this, itemId));
	}
	return bOK;
}

bool CPlayer::PickUpItem(EntityId itemId, bool sound)
{
	const bool bOK = CActor::PickUpItem(itemId, sound);
	if (bOK)
	{
		CALL_PLAYER_EVENT_LISTENERS(OnItemPickedUp(this, itemId));
	}
	return bOK;
}

bool CPlayer::DropItem(EntityId itemId, float impulseScale, bool selectNext, bool byDeath)
{
	const bool bOK = CActor::DropItem(itemId, impulseScale, selectNext, byDeath);
	if (bOK)
	{
		CALL_PLAYER_EVENT_LISTENERS(OnItemDropped(this, itemId));
	}
	return bOK;
}

void CPlayer::NetKill(const KillParams &killParams)
{
	//--- Ensure the character is killed before the death reaction code is triggered
	if (GetHealth() > 0)
	{
		SetHealth(0);
	}

	//--- Ensure the AI is killed - if it exists
	if ( GetEntity() && GetEntity()->GetAI() )
	{
		GetEntity()->GetAI()->Event(AIEVENT_DISABLE, NULL);
	}

	bool bRagdoll = killParams.ragdoll;

	if (g_pGameCVars->g_hitDeathReactions_enable && m_pHitDeathReactions)
	{
		HitInfo hitInfo;
		hitInfo.type = killParams.hit_type;
		hitInfo.damage = (float)killParams.damage;
		hitInfo.partId	= killParams.hit_joint;
		hitInfo.dir = killParams.impulse;
		hitInfo.material = killParams.material;

		//CryLogAlways("[CPlayer::NetKill(] HitJoint: %d", killParams.hit_joint);

		// If hitDeathReactions system fails, rely on CActor::NetKill to decide if we should trigger the ragdoll from
		// KillParams struct parameters
		bRagdoll = !m_pHitDeathReactions->OnKill(hitInfo) && bRagdoll;
	}
	else
	{
		m_ragdollTime = 0.1f;

		// we are forcing ragdollization by m_ragdollTime, no need for CActor::NetKill to trigger it
		bRagdoll = false;
	}

	CActor::KillParams modifiedKillParams(killParams);
	modifiedKillParams.ragdoll = bRagdoll;

	m_lastKillParams = modifiedKillParams;

	CActor::NetKill(modifiedKillParams);
}

void CPlayer::SpawnParticleEffect(const char* effectName, const Vec3& pos, const Vec3& dir)
{
	IParticleEffect *pEffect = gEnv->pParticleManager->FindEffect(effectName, "CPlayer::SpawnParticleEffect");
	if (pEffect == NULL)
		return;

	pEffect->Spawn(true, IParticleEffect::ParticleLoc(pos, dir, 1.0f));
}

void CPlayer::PlaySound(EPlayerSounds soundID, bool play, const char* paramName, float paramValue, const char* paramName2, float paramValue2 )
{
	if(!IsClient()) //currently this is only supposed to be heared by the client (not 3D, not MP)
		return;

	bool mute = false;

	switch(soundID)
	{
		// temporary ignored 
		case ESound_Run:
		case ESound_StopRun:  
			mute = true;
			break;
		
		case ESound_Jump:
		  mute = GetPerkData<bool>(EPD_MuteJumping);
			CCCPOINT_IF(mute, Perk_SilentFeet_AdjustJump);

			if (gEnv->pInput && play) gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.05f, 0.05f, 0.1f) );
			break;
			
		case ESound_Fall_Drop:
			if (gEnv->pInput && play) gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.2f, 0.3f, 0.2f) );
			break;
			
		case ESound_Melee:
			if (gEnv->pInput && play) gEnv->pInput->ForceFeedbackEvent( SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.15f, 0.6f, 0.2f) );
			break;
	}

	SSound& sound = m_sounds[soundID];
	
	EntityId entityId = GetEntity()->GetId();

	if(play)
	{
		if (sound.isRepeated && sound.audioSignalPlayer.IsPlaying( entityId ))
			return;
	
		if (!mute)
		{
			sound.audioSignalPlayer.Play( entityId );
			float noiseSupression = GetActorSuitGameParameters().GetProps().noiseSupression;
			if (noiseSupression>0)
			{
				float vol = CLAMP((1.0f-noiseSupression), 0.0f, 1.0f);
				sound.audioSignalPlayer.SetVolume( entityId, vol );
			}
			if (paramName)
				sound.audioSignalPlayer.SetParam( entityId, paramName, paramValue );
			if (paramName2)
				sound.audioSignalPlayer.SetParam( entityId, paramName2, paramValue2 );
		}
	}
	else
	{
		sound.audioSignalPlayer.Stop( entityId );
	} 
}


//-----------------------------------------------------------------------
void CPlayer::GetMemoryUsage(ICrySizer * pSizer) const
{
	pSizer->AddObject(this,sizeof(*this));
	GetInternalMemoryUsage(pSizer);
}

void CPlayer::GetInternalMemoryUsage(ICrySizer * pSizer) const
{
	pSizer->AddObject(m_pHitDeathReactions);
	pSizer->AddObject(m_pNanoSuit);
	pSizer->AddObject(m_pPlayerInput);
	pSizer->AddObject(m_pDebugHistoryManager);
	pSizer->AddObject(m_pBodyDamage);
	pSizer->AddObject(m_hitRecoilGameEffect);
	CActor::GetInternalMemoryUsage(pSizer); // collect memory of parent class
}


//-----------------------------------------------------------------------
void CPlayer::Debug()
{
	bool debug = true;

	const char* filter = g_pGameCVars->pl_debug_filter->GetString();
	const char* name = GetEntity()->GetName();
	if ((strcmp(filter, "0") != 0) && (strcmp(filter, name) != 0))
		debug = false;

	if (!debug)
	{
		if (m_pDebugHistoryManager != NULL)
			m_pDebugHistoryManager->Clear();
    return;
	}

  if (m_pDebugHistoryManager == NULL)
    m_pDebugHistoryManager = g_pGame->GetIGameFramework()->CreateDebugHistoryManager();

	bool showReqVelo = (g_pGameCVars->pl_debug_movement != 0);
	m_pDebugHistoryManager->LayoutHelper("ReqVelo", NULL, showReqVelo, -20, 20, 0, 5, 0.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("ReqVeloX", NULL, showReqVelo, -20, 20, -5, 5, 1.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("ReqVeloY", NULL, showReqVelo, -20, 20, -5, 5, 2.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("ReqVeloZ", NULL, showReqVelo, -20, 20, -5, 5, 3.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("ReqRotZ", NULL, showReqVelo, -360, 360, -5, 5, 4.0f, 0.0f);

	m_pDebugHistoryManager->LayoutHelper("PhysVelReq", NULL, showReqVelo, -20, 20, 0, 5, 0.0f, 1.0f, 1,0.9f);
	m_pDebugHistoryManager->LayoutHelper("PhysVelReqX", NULL, showReqVelo, -20, 20, -5, 5, 1.0f, 1.0f, 1,0.9f);
	m_pDebugHistoryManager->LayoutHelper("PhysVelReqY", NULL, showReqVelo, -20, 20, -5, 5, 2.0f, 1.0f, 1,0.9f);
	m_pDebugHistoryManager->LayoutHelper("PhysVelReqZ", NULL, showReqVelo, -20, 20, -5, 5, 3.0f, 1.0f, 1,0.9f);

	m_pDebugHistoryManager->LayoutHelper("PhysVelo", NULL, showReqVelo, -20, 20, 0, 5, 0.0f, 1.9f, 1,0.9f);
	m_pDebugHistoryManager->LayoutHelper("PhysVeloX", NULL, showReqVelo, -20, 20, -5, 5, 1.0f, 1.9f, 1,0.9f);
	m_pDebugHistoryManager->LayoutHelper("PhysVeloY", NULL, showReqVelo, -20, 20, -5, 5, 2.0f, 1.9f, 1,0.9f);
	m_pDebugHistoryManager->LayoutHelper("PhysVeloZ", NULL, showReqVelo, -20, 20, -5, 5, 3.0f, 1.9f, 1,0.9f);

	m_pDebugHistoryManager->LayoutHelper("PhysVeloUn", NULL, showReqVelo, -20, 20, 0, 5, 0.0f, 2.8f, 1,0.2f);
	m_pDebugHistoryManager->LayoutHelper("PhysVeloUnX", NULL, showReqVelo, -20, 20, -5, 5, 1.0f, 2.8f, 1,0.2f);
	m_pDebugHistoryManager->LayoutHelper("PhysVeloUnY", NULL, showReqVelo, -20, 20, -5, 5, 2.0f, 2.8f, 1,0.2f);
	m_pDebugHistoryManager->LayoutHelper("PhysVeloUnZ", NULL, showReqVelo, -20, 20, -5, 5, 3.0f, 2.8f, 1,0.2f);

	bool showReqAim = (g_pGameCVars->pl_debug_aiming != 0);
	m_pDebugHistoryManager->LayoutHelper("AimH", NULL, showReqAim, -0.2f, 0.2f, -0.1f, 0.1f, 0.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("AxxAimH", NULL, showReqAim, -1.0f, 1.0f, -0.1f, 0.1f, 1.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("AimHAxxMul", NULL, showReqAim, 0, 10, 0, 1, 2.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("AimHAxxFrac", NULL, showReqAim, 0, 1, 0, 1, 3.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("AimHAxxTime", NULL, showReqAim, 0, 2, 0, 1, 4.0f, 0.0f);

	m_pDebugHistoryManager->LayoutHelper("AimV", NULL, showReqAim, -0.2f, 0.2f, -0.1f, 0.1f, 0.0f, 1.0f);
	m_pDebugHistoryManager->LayoutHelper("AxxAimV", NULL, showReqAim, -1.0f, 1.0f, -0.1f, 0.1f, 1.0f, 1.0f);
	m_pDebugHistoryManager->LayoutHelper("AimVAxxMul", NULL, showReqAim, 0, 10, 0, 1, 2.0f, 1.0f);
	m_pDebugHistoryManager->LayoutHelper("AimVAxxFrac", NULL, showReqAim, 0, 1, 0, 1, 3.0f, 1.0f);
	m_pDebugHistoryManager->LayoutHelper("AimVAxxTime", NULL, showReqAim, 0, 2, 0, 1, 4.0f, 1.0f);

	bool showAimAssist = false;
	m_pDebugHistoryManager->LayoutHelper("AimFollowH", NULL, showAimAssist, -0.05f, 0.05f, -0.05f, 0.05f, 0.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("AimFollowV", NULL, showAimAssist, -0.05f, 0.05f, -0.05f, 0.05f, 1.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("AimScale", NULL, showAimAssist, 0, 1, 0, 1, 2.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("AimDeltaH", NULL, showAimAssist, -0.03f, +0.03f, -0.03f, +0.03f, 0.0f, 1.0f);
	m_pDebugHistoryManager->LayoutHelper("AimDeltaV", NULL, showAimAssist, -0.03f, +0.03f, -0.03f, +0.03f, 1.0f, 1.0f);

	m_pDebugHistoryManager->LayoutHelper("FollowCoolDownH", NULL, showAimAssist, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 2.0f);
	m_pDebugHistoryManager->LayoutHelper("FollowCoolDownV", NULL, showAimAssist, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 2.0f);

/*
	m_pDebugHistoryManager->LayoutHelper("InputMoveX", NULL, showReqVelo, -1, 1, -1, 1, 0.0f, 1.0f);
	m_pDebugHistoryManager->LayoutHelper("InputMoveY", NULL, showReqVelo, -1, 1, -1, 1, 1.0f, 1.0f);
/**/

/*
	bool showVelo = true;
	m_pDebugHistoryManager->LayoutHelper("Velo", NULL, showVelo, -20, 20, 0, 8, 0.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("VeloX", NULL, showVelo, -20, 20, -5, 5, 1.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("VeloY", NULL, showVelo, -20, 20, -5, 5, 2.0f, 0.0f);
	m_pDebugHistoryManager->LayoutHelper("VeloZ", NULL, showVelo, -20, 20, -5, 5, 3.0f, 0.0f);
/**/

/*
	m_pDebugHistoryManager->LayoutHelper("Axx", NULL, showVelo, -20, 20, -1, 1, 0.0f, 1.0f);
	m_pDebugHistoryManager->LayoutHelper("AxxX", NULL, showVelo, -20, 20, -1, 1, 1.0f, 1.0f);
	m_pDebugHistoryManager->LayoutHelper("AxxY", NULL, showVelo, -20, 20, -1, 1, 2.0f, 1.0f);
	m_pDebugHistoryManager->LayoutHelper("AxxZ", NULL, showVelo, -20, 20, -1, 1, 3.0f, 1.0f);
/**/

	//m_pDebugHistoryManager->LayoutHelper("ModelOffsetX", NULL, true, 0, 1, -0.5, 0.5, 5.0f, 0.5f);
	//m_pDebugHistoryManager->LayoutHelper("ModelOffsetY", NULL, true, 0, 1, 0, 1, 5.0f, 1.5f, 1.0f, 0.5f);

//*
	bool showJump = (g_pGameCVars->pl_debug_jumping != 0);
	m_pDebugHistoryManager->LayoutHelper("OnGround", NULL, showJump, 0, 1, 0, 1, 5.0f, 0.5f, 1.0f, 0.5f);
	m_pDebugHistoryManager->LayoutHelper("Jumping", NULL, showJump, 0, 1, 0, 1, 5.0f, 1.0f, 1.0f, 0.5f);
	m_pDebugHistoryManager->LayoutHelper("Flying", NULL, showJump, 0, 1, 0, 1, 5.0f, 1.5f, 1.0f, 0.5f);
	m_pDebugHistoryManager->LayoutHelper("StuckTimer", NULL, showJump, 0, 0.5, 0, 0.5, 5.0f, 2.0f, 1.0f, 1.0f);
	m_pDebugHistoryManager->LayoutHelper("InAirTimer", NULL, showJump, 0, 5, 0, 5, 4.0f, 2.0f, 1.0f, 1.0f);
	m_pDebugHistoryManager->LayoutHelper("InWaterTimer", NULL, showJump, -5, 5, -0.5, 0.5, 4, 3);
	m_pDebugHistoryManager->LayoutHelper("OnGroundTimer", NULL, showJump, 0, 5, 0, 5, 4.0f, 1.0f, 1.0f, 1.0f);
/**/

//*
	m_pDebugHistoryManager->LayoutHelper("GroundSlope", NULL, showJump, 0, 90, 0, 90, 0, 3);
	m_pDebugHistoryManager->LayoutHelper("GroundSlopeMod", NULL, showJump, 0, 90, 0, 90, 1, 3);
/**/

	//m_pDebugHistoryManager->LayoutHelper("ZGDashTimer", NULL, showVelo, -20, 20, -0.5, 0.5, 5.0f, 0.5f);
/*
	m_pDebugHistoryManager->LayoutHelper("StartTimer", NULL, showVelo, -20, 20, -0.5, 0.5, 5.0f, 0.5f);
	m_pDebugHistoryManager->LayoutHelper("DodgeFraction", NULL, showVelo, 0, 1, 0, 1, 5.0f, 1.5f, 1.0f, 0.5f);
	m_pDebugHistoryManager->LayoutHelper("RampFraction", NULL, showVelo, 0, 1, 0, 1, 5.0f, 2.0f, 1.0f, 0.5f);
	m_pDebugHistoryManager->LayoutHelper("ThrustAmp", NULL, showVelo, 0, 5, 0, 5, 5.0f, 2.5f, 1.0f, 0.5f);
*/
}

void CPlayer::DebugGraph_AddValue(const char* id, float value) const
{
	if (m_pDebugHistoryManager == NULL)
		return;

	if (id == NULL)
		return;

	// NOTE: It's alright to violate the const here. The player is a good common owner for debug graphs, 
	// but it's also not non-const in all places, even though graphs might want to be added from those places.
	IDebugHistory* pDH = const_cast<IDebugHistoryManager*>(m_pDebugHistoryManager)->GetHistory(id);
	if (pDH != NULL)
		pDH->AddValue(value);
}

//Try to predict if the player needs to go to crouch stance to pick up a weapon/item
bool CPlayer::NeedToCrouch(const Vec3& pos)
{
	if (IMovementController *pMC = GetMovementController())
	{
		SMovementState state;
		pMC->GetMovementState(state);

		// determine height delta (0 is at player feet ... greater zero is higher and means shorter crouch time) 
		float delta = pos.z - GetEntity()->GetWorldPos().z;
		Limit(delta, 0.0f, 0.8f);
		delta /= 0.8f;

		//Maybe this "prediction" is not really accurate...
		//If the player is looking down, probably the item is on the ground
		if(state.aimDirection.z < -0.7f)
		{
			m_stats.forceCrouchTime = 0.75f * (1.0f - delta);
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------
void CPlayer::EnterSwimming( )
{
	if (m_bSwimming)
		return;

	m_bSwimming = true;

	SendPerkEvent(EPE_EnterSwimming);

	if (IsClient())
	{
		//Select underwater weapon here
		SendMusicLogicEvent(eMUSICLOGICEVENT_PLAYER_SWIM_ENTER);
		HolsterItem(true);
	}
}

//------------------------------------------------------------------------
void CPlayer::ExitSwimming()
{
	if (!m_bSwimming)
		return;

	m_bSwimming = false;

	if (IsClient())
	{
		//Unselect underwater weapon here
		SendMusicLogicEvent(eMUSICLOGICEVENT_PLAYER_SWIM_LEAVE);
		HolsterItem(false);
	}
}


bool CPlayer::HasHitAssistance()
{
	return m_bHasAssistance;
}

//-----------------------------------------------------------------------
bool CPlayer::CanFire()
{
	//AI can always
	if(!IsPlayer() || GetISystem()->IsDedicated())		// dedicated clients can always fire too
		return true;

	//For the player

	float velSqr = m_stats.velocity.len2();
/*
	//1- Player can not fire while sprinting
	if(m_stats.bSprinting && velSqr>0.2f &&
		GetPlayerInput() && (GetPlayerInput()->GetActions()&ACTION_SPRINT))
		return false;
*/
	//1- Player can not fire while sliding
	if(IsSliding())
		return false;

	//2- Player can not fire while his weapon is underwater level
	//	(doesn't apply in vehicles - CVehicleWeapon::CanFire() takes care of this,
	//	 else it breaks in 3rd person view)
	const Vec3 weaponPosition = GetEntity()->GetWorldPos() + GetWeaponOffset();
	if (!GetLinkedVehicle() && (m_stats.worldWaterLevel > weaponPosition.z))
		return false;

	return true;
}

//-----------------------------------------------------------------------
bool CPlayer::IsSprinting()
{
	//Only for the player
	return (IsPlayer() && m_stats.bSprinting);
}


IMPLEMENT_RMI(CPlayer, ClAnimGraphTransition)
{
	if (m_pAnimatedCharacter)
	{
		if (IAnimationGraphState* pAGState = m_pAnimatedCharacter->GetAnimationGraphState())
			pAGState->PushForcedState(params.name);
	}

	return true;
}

IMPLEMENT_RMI(CPlayer, ClAnimGraphInput)
{
	if (m_pAnimatedCharacter)
	{
		if (IAnimationGraphState* pAGState = m_pAnimatedCharacter->GetAnimationGraphState())
		{
			if (params.typ==0)
				pAGState->SetInput(params.id, params.ivalue);
			else
				pAGState->SetInput(params.id, params.svalue);
		}
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CPlayer, ClEMP)
{
	SNanoSuitEvent suitEvent;
	suitEvent.event = eNanoSuitEvent_EMP_DISCHARGE;
	suitEvent.fParam = params.time;

	this->SendActorSuitEvent(suitEvent);

	return true;
}

//------------------------------------------------------------------------
void
CPlayer::StagePlayer(bool bStage, SStagingParams* pStagingParams /* = 0 */)
{
	if (IsClient() == false)
		return;

	EStance stance = STANCE_NULL;
	bool bLock = false;
	if (bStage == false)
	{
		m_params.vLimitDir.zero();
		m_params.vLimitRangeH = 0.0f;
		m_params.vLimitRangeV = 0.0f;
	}
	else if (pStagingParams != 0)
	{
		bLock = pStagingParams->bLocked;
		m_params.vLimitDir = pStagingParams->vLimitDir;
		m_params.vLimitRangeH = pStagingParams->vLimitRangeH;
		m_params.vLimitRangeV = pStagingParams->vLimitRangeV;
		stance = pStagingParams->stance;
		if (bLock)
		{
			IPlayerInput* pPlayerInput = GetPlayerInput();
			if(pPlayerInput)
				pPlayerInput->Reset();
		}
	}
	else
	{
		bStage = false;
	}

	if (bLock || m_stagingParams.bLocked)
	{
		g_pGameActions->FilterNoMove()->Enable(bLock);
	}
	m_stagingParams.bActive = bStage;
	m_stagingParams.bLocked = bLock;
	m_stagingParams.vLimitDir = m_params.vLimitDir;
	m_stagingParams.vLimitRangeH = m_params.vLimitRangeH;
	m_stagingParams.vLimitRangeV = m_params.vLimitRangeV;
	m_stagingParams.stance = stance;
	if (stance != STANCE_NULL)
		SetStance(stance);
}

void CPlayer::ResetScreenFX()
{
	if (IsClient())
	{
		SAFE_HUD_FUNC(ShowDeathFX(0));
		
		gEnv->p3DEngine->ResetPostEffects();
		if(g_pGame->GetScreenEffects())
			g_pGame->GetScreenEffects()->ResetAllBlendGroups(true);
	}
		
	m_perkParticleEmitterInfo.Reset();
}

void CPlayer::ResetFPView()
{
	float defaultFov = 55.0f;
	gEnv->pRenderer->EF_Query(EFQ_DrawNearFov,(INT_PTR)&defaultFov);
}

void CPlayer::NotifyObjectGrabbed(bool bIsGrab, EntityId objectId, bool bIsNPC, bool bIsTwoHanded /*= false*/)
{
	CALL_PLAYER_EVENT_LISTENERS(OnObjectGrabbed(this, bIsGrab, objectId, bIsNPC, bIsTwoHanded));
}

void CPlayer::StopLoopingSounds()
{
	//stop sounds
	for(int i = (int)ESound_Player_First+1; i < (int)ESound_Player_Last;++i)
		PlaySound((EPlayerSounds)i, false);
	//if(m_pNanoSuit)
	{
		//for(int i = (int)NO_SOUND+1; i < (int)ESound_Suit_Last;++i)
			//m_pNanoSuit->PlaySound((ENanoSound)i, 0.0f, true);
	}
}

//----------------------------------------------------------
void CPlayer::SendMusicLogicEvent(EMusicLogicEvents event)
{
	if(IsClient())
		m_pGameFramework->GetMusicLogic()->SetEvent(event);
}

struct RecursionFlagLock
{
	RecursionFlagLock(bool& flag) : m_bFlag(flag) { m_bFlag = true; }
	~RecursionFlagLock() { m_bFlag = false; }
	bool m_bFlag;
};

//--------------------------------------------------------
void CPlayer::AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event, const uint32 eventNameCRC)
{
	CActor::AnimationEvent(pCharacter,event, eventNameCRC);

	bool isMainCharacter = (pCharacter != GetShadowCharacter());
	if (isMainCharacter)
	{
		if (m_stealthKill.AnimationEvent(pCharacter, event, eventNameCRC))
			return;

		// HitDeathReactions can "eat" animation events
		if (m_pHitDeathReactions && m_pHitDeathReactions->OnAnimationEvent(event, eventNameCRC))
			return;

		const SActorAnimationEvents& animEventsTable = GetAnimationEventsTable();
		
		if (animEventsTable.m_soundId == eventNameCRC)
		{
			//Only client ones (the rest are processed in SoundProxy)
			if (IsClient())
			{
				Vec3 offset(0.0f,0.0f,1.0f);
				if (event.m_BonePathName && event.m_BonePathName[0])
				{
					ISkeletonPose* pSkeletonPose = (pCharacter ? pCharacter->GetISkeletonPose() : 0);

					int id = (pSkeletonPose ? pSkeletonPose->GetJointIDByName(event.m_BonePathName) : -1);
					if (pSkeletonPose && id >= 0)
					{
						QuatT boneQuat(pSkeletonPose->GetAbsJointByID(id));
						offset = boneQuat.t;
					}
				}

				int flags = FLAG_SOUND_DEFAULT_3D;
				if (strchr(event.m_CustomParameter, ':') == NULL)
					flags |= FLAG_SOUND_VOICE;

				if(m_pSoundProxy)
					m_pSoundProxy->PlaySound(event.m_CustomParameter, offset, FORWARD_DIRECTION, flags, eSoundSemantic_Animation, 0, 0);
			}

		}
		else if (animEventsTable.m_plugginTriggerId == eventNameCRC)
		{
			const char* parameter = event.m_CustomParameter;
			SendPerkEvent(EPE_AnimationEvent, (void*) parameter);
		}
		else if (animEventsTable.m_ragdollStartId == eventNameCRC)
		{
			//--- Initiate rag-doll between now & the end of the animation
			CAnimationPlayerProxy *animPlayerProxy = GetAnimatedCharacter()->GetAnimationPlayerProxy(0);
			const CAnimation *animPtr = animPlayerProxy ? animPlayerProxy->GetTopAnimation(GetEntity(), 0) : NULL;
			if (animPtr)
			{
				m_ragdollTime = cry_frand() * (animPtr->m_fCurrentDuration * (1.0f - animPtr->m_fAnimTime));
			}
		}
		else if(animEventsTable.m_footStepId == eventNameCRC)
		{
			if (event.m_BonePathName && event.m_BonePathName[0])
			{
				OnFootStepAnimEvent(pCharacter, event.m_BonePathName);
			}
		}
		else if(animEventsTable.m_footStepImpulseId == eventNameCRC)
		{
			OnFootStepImpulseAnimEvent(pCharacter, event);
		}
		else if(animEventsTable.m_swimmingStrokeId == eventNameCRC)
		{
			OnSwimmingStrokeAnimEvent();
		}
	}
	else
	{
		const SActorAnimationEvents& animEventsTable = GetAnimationEventsTable();
		if(animEventsTable.m_footStepId == eventNameCRC)
		{
			if (event.m_BonePathName && event.m_BonePathName[0])
			{
				OnFootStepAnimEvent(pCharacter, event.m_BonePathName);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPlayer::OnFootStepAnimEvent(ICharacterInstance* pCharacter, const char* boneName)
{
	ISkeletonPose* pSkeletonPose = (pCharacter ? pCharacter->GetISkeletonPose() : 0);
	int32 nFootJointId = (pSkeletonPose ? pSkeletonPose->GetJointIDByName(boneName) : -1);

	if (nFootJointId != -1)
	{
		if (ShouldUpdateNextFootStep())
		{
			const float fFrameTime = gEnv->pTimer->GetFrameTime();
			ExecuteFootStep(pCharacter, fFrameTime, nFootJointId);
		}
	}
	else
	{
		GameWarning("%s has incorrect foot JointID in animation triggered footstep sounds", GetEntity()->GetName());
	}
}


//////////////////////////////////////////////////////////////////////////
void CPlayer::OnFootStepImpulseAnimEvent(ICharacterInstance* pCharacter, const AnimEventInstance &event)
{
	// special event for smart object animations, trigger a downwards impulse from the specified bone position
	//	(intended for vehicle-climb SO, initially, but should work more generally too)
	ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
	int32 nFootJointId = (pSkeletonPose ? pSkeletonPose->GetJointIDByName(event.m_BonePathName) : -1);

	if (nFootJointId != -1)
	{
		QuatT foot = pSkeletonPose->GetAbsJointByID(nFootJointId);

		ray_hit hit;	

		Vec3 pos = GetEntity()->GetSlotWorldTM(0) * foot.t;
		if (gEnv->pPhysicalWorld->RayWorldIntersection(pos, Vec3(0,0,-0.2f), ent_sleeping_rigid|ent_rigid,
			rwi_ignore_noncolliding | rwi_stop_at_pierceable, &hit, 1, GetEntity()->GetPhysics()))
		{
			if(hit.pCollider)
			{
				float amount = (float)atof(event.m_CustomParameter);
				pe_action_impulse impulse;
				impulse.point = hit.pt;
				impulse.impulse = event.m_vDir * amount;
				impulse.partid = hit.partid;
				hit.pCollider->Action(&impulse, true);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
void CPlayer::OnSwimmingStrokeAnimEvent()
{
	EActionSoundParamValues actionParam = m_stats.headUnderWaterTimer>0 ? EActionSoundParam_Underwater : EActionSoundParam_SwimOnWater;
	PlaySound( ESound_Gear_Water, true, "action", float(actionParam), "speed", m_stats.speedFlat );
}


void CPlayer::EnterPickAndThrow( EntityId entityPicked )
{
	const char* const name = "PickAndThrowWeapon";
	IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass( name );
	EntityId pickAndThrowWeaponId = GetInventory()->GetItemByClass( pClass );
	
	CRY_ASSERT_MESSAGE( pickAndThrowWeaponId, "Player does not have the 'PickAndThrowWeapon' item" );
	if (pickAndThrowWeaponId)
	{
		m_stats.pickAndThrowEntity = entityPicked;
		SelectItem( pickAndThrowWeaponId, true );
		m_stats.isInPickAndThrowMode = true;
	}
}

void CPlayer::ExitPickAndThrow()
{
	m_stats.pickAndThrowEntity = 0;
	m_stats.isInPickAndThrowMode = false;
	SelectLastItem(false);
}


bool CPlayer::HasShadowCharacter() const
{
	return GetEntity()->GetCharacter(GetShadowCharacterSlot()) != NULL;
}

int  CPlayer::GetShadowCharacterSlot() const
{
	return 5;
}

ICharacterInstance *CPlayer::GetShadowCharacter() const
{
	return GetEntity()->GetCharacter(GetShadowCharacterSlot());
}

IInteractor *CPlayer::GetInteractor()
{
	if (!m_pInteractor)
		m_pInteractor = (IInteractor*) GetGameObject()->AcquireExtension("Interactor");
	return m_pInteractor;
}


bool CPlayer::CanInteract() const
{
	bool canNotInteractInState = IsSliding() || m_stats.isGrabbed || m_stats.pickAndThrowEntity!=0 || IsSwimming();

	if (canNotInteractInState)
		return false;

	if (m_stealthKill.IsBusy())
		return false;

	//currently busy fiddling with the own weapon
	IItem *pItem = GetCurrentItem(false);
	CWeapon * pMainWeapon = pItem?static_cast<CWeapon*>(pItem->GetIWeapon()):NULL;
	if (pMainWeapon)
	{
		bool mainWeaponBusy = pMainWeapon->IsBusy() || pMainWeapon->IsModifying() || pMainWeapon->IsReloading();
		if(mainWeaponBusy)
			return false;

		bool suitVisorOn = (pMainWeapon->GetEntity()->GetClass() == CItem::sBinocularsClass);
		if (suitVisorOn)
			return false;
	}

	return true;
}

// Should probably be moved into PlayerPlugin_Interaction.cpp Update function...
void CPlayer::UpdateInteractionType()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	CRY_ASSERT_MESSAGE(IsClient(), "This function should only be called for the client!");

	if (m_pInteractor == NULL)
	{
		s_clientInteractionInfo.Reset();
	}
	else
	{
		s_clientInteractionInfo.interactiveEntityId = m_pInteractor->GetOverEntityId();
		s_clientInteractionInfo.interactionType = (s_clientInteractionInfo.interactiveEntityId != 0) ? GetInteractionTypeForEntity(s_clientInteractionInfo.interactiveEntityId) : eInteraction_None;
	}
}

CPlayer::EInteractionType CPlayer::GetInteractionTypeForEntity(EntityId entityId)
{
	CRY_ASSERT_MESSAGE(IsClient(), "This function should only be called for the client!");
	
	if (!CanInteract())
		return eInteraction_None;

	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(s_clientInteractionInfo.interactiveEntityId);
	if(!pEntity)
		return eInteraction_None;

	SmartScriptTable propertiesTable;
	IScriptTable *pEntityScript = pEntity->GetScriptTable();
	bool hasPropertyTable = pEntityScript ? pEntityScript->GetValue("Properties", propertiesTable) : false;

	if(CActor *pActorAI = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(s_clientInteractionInfo.interactiveEntityId)))
	{
		if(!gEnv->bMultiplayer && m_stealthKill.CanExecuteOn(pActorAI))
			return eInteraction_Stealthkill;

		if (CanGrabEnemy( entityId ))
			return eInteraction_GrabEnemy;

		if (hasPropertyTable)
		{
			int usable=0;
			if(propertiesTable->GetValue("bUsable", usable) && usable)
				return eInteraction_Use;
		}

		return eInteraction_None;
	}

	else if(CItem *pItem = static_cast<CItem*>(g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(s_clientInteractionInfo.interactiveEntityId)))
	{
		return GetInteractionTypeWithItem(pItem);
	}

	else if(IPhysicalEntity* pPhysicalEntity = pEntity->GetPhysics())
	{
		float heightOffset = 0.0f;
		AABB bbox;
		pEntity->GetWorldBounds(bbox);
		if (!bbox.IsEmpty())
		{
			heightOffset = bbox.GetCenter().z - pEntity->GetWorldPos().z;
		}

		if (g_pGame->GetPlayerVisTable()->CanLocalPlayerSee(s_clientInteractionInfo.interactiveEntityId, 2, heightOffset))
		{
			bool isMarkedAsNonPickable = false;
			bool isRigidBody = (pPhysicalEntity->GetType() == PE_RIGID);
			if (hasPropertyTable)
			{
				if(isRigidBody)
				{
					int pickable = 0;
					if (propertiesTable->GetValue("bPickable", pickable))
					{
						if (pickable)
						return eInteraction_Grab;
						else
							isMarkedAsNonPickable = true;
				}
				}

				int usable = 0;
				if(propertiesTable->GetValue("bUsable", usable) && usable)
					return eInteraction_Use;
			}

			if (!isMarkedAsNonPickable && isRigidBody)
			{
				if (HasObjectGrabHelper(entityId))
					return eInteraction_Grab;
			}
		}
	}

	return eInteraction_None;
}

#if !defined(_RELEASE)
static AUTOENUM_BUILDNAMEARRAY(s_playerInteractionTypeNames, PlayerInteractionTypeList);
#endif

CPlayer::EInteractionType CPlayer::GetInteractionTypeWithItem(CItem* pTargetItem)
{
	// NB: Please do not return early from this function! [TF]

	EInteractionType interactionType = eInteraction_None;
	CRY_ASSERT(pTargetItem);

	if (CanPickupItems())
	{
		bool canBeUsed = pTargetItem->CanUse(GetEntityId());

		if(canBeUsed)
		{
			if(!pTargetItem->IsUsed())
			{
				interactionType = pTargetItem->IsPickable() ? eInteraction_PickupItem : eInteraction_Use;
			}
			else if(pTargetItem->GetOwnerId() == GetEntityId())
			{
				interactionType = pTargetItem->IsRippedOff() ? eInteraction_None : eInteraction_StopUsing;
			}
		}
		else 
		{
			//Should we check this?? (just ported from previous version)
			IInventory *pInventory = GetInventory();

			if (pInventory->FindItem(GetEntityId()) == -1 && pTargetItem->CanPickUp(GetEntityId()))
			{
				IEntityClass* pTargetItemClass = pTargetItem->GetEntity()->GetClass();
				EntityId playerId = GetEntityId();

				bool isSlotAvailable = CheckInventoryRestrictions(pTargetItemClass->GetName());
				bool inventoryHasItemOfTargetClass = (pInventory->GetItemByClass(pTargetItemClass) != NULL);
				bool ammoRestrictionsOk = pTargetItem->CheckAmmoRestrictions(playerId);
				bool targetItemIsNotAutoPickableButCanBePickedUp = (inventoryHasItemOfTargetClass && ammoRestrictionsOk && !pTargetItem->CanPickUpAutomatically());
				bool hasBonusAmmoToPickup = pTargetItem->HasSomeAmmoToPickUp(playerId);
				
				if (isSlotAvailable)
				{
					static IEntityClass* pDogtag = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Dogtag");

					if(gEnv->bMultiplayer && pTargetItemClass == pDogtag)
					{
						//auto pickup dogtags in multiplayer
						interactionType = eInteraction_PickupAmmo;
					}
					else if (ammoRestrictionsOk && hasBonusAmmoToPickup)
					{
						//I do have it, and have passed check for ammo capacity (item has all data stored)
						interactionType = eInteraction_PickupAmmo;
					}
					else if ((!inventoryHasItemOfTargetClass) || targetItemIsNotAutoPickableButCanBePickedUp)
					{
						//Slot available and I have this item class already
						interactionType = eInteraction_PickupItem;
					}
				}
				else if (targetItemIsNotAutoPickableButCanBePickedUp)
				{
					//I have this one already, but it requires user confirmation to be picked up (JAW, C4)
					interactionType = eInteraction_PickupItem;
				}
				else
				{
					//No Slot available, first try to grab its ammo
					if (ammoRestrictionsOk && hasBonusAmmoToPickup)
					{
						//I do have it, and have passed check for ammo capacity (item has all data stored)
						interactionType = eInteraction_PickupAmmo;
					}
					else 	if (!inventoryHasItemOfTargetClass)
					{
						//Check if I can exchange with current item
						IItem* pCurrentItem = GetCurrentItem();
						if (pCurrentItem && pInventory->AreItemsInSameSlot(pCurrentItem->GetEntity()->GetClass()->GetName(), pTargetItemClass->GetName()))
						{
							interactionType = eInteraction_ExchangeItem;
						}
					}
				}
			}
		}

		WATCH_ACTOR_STATE ("%s a %s '%s', canBeUsed=%d rippedOff=%d mountable=%d mounted=%d owner=%d used=%d usePickUpInput=%d RESULT=%s", (pTargetItem->GetOwnerId() == GetEntityId()) ? "using" : "near", pTargetItem->GetEntity()->GetClass()->GetName(), pTargetItem->GetDisplayName(), canBeUsed, pTargetItem->IsRippedOff(), pTargetItem->IsMountable(), pTargetItem->IsMounted(), pTargetItem->GetOwnerId(), pTargetItem->IsUsed(), pTargetItem->IsPickable(), s_playerInteractionTypeNames[interactionType]);
	}

	return interactionType;
}

//////////////////////////////////////////////////////////////////////////
bool CPlayer::HasObjectGrabHelper(EntityId entityId) const
{
	static IEntityClass* pPickAndThrowWeaponClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("PickAndThrowWeapon");

	EntityId pickAndThrowWeaponId = GetInventory()->GetItemByClass( pPickAndThrowWeaponClass );
	IItem* pItem = m_pItemSystem->GetItem( pickAndThrowWeaponId );
	if (pItem)
	{
		CPickAndThrowWeapon* pPickAndThrowWeapon = static_cast<CPickAndThrowWeapon*>(pItem);
		return pPickAndThrowWeapon->HasObjectGrabHelper(entityId);
	}

	return false;
}
//////////////////////////////////////////////////////////////////////////


bool CPlayer::CanGrabEnemy( EntityId entityId )
{
	const SAutoaimTarget* pTargetInfo = g_pGame->GetAutoAimManager().GetTargetInfo( entityId );

	bool canGrab = pTargetInfo && pTargetInfo->HasFlagsSet((int8)(eAATF_AIHostile|eAATF_CanBeGrabbed));
	
	return canGrab;
}


// NB: This function crashes if passed a NULL damageType... so please don't do it!
void CPlayer::DamageInfo(EntityId shooterID, EntityId weaponID, IEntityClass *pProjectileClass, float damage, const char *damageType, const Vec3 hitDirection)
{
	assert (damageType);

	CActor::DamageInfo(shooterID, weaponID, pProjectileClass, damage, damageType, hitDirection);
	if (IsClient())
	{
		m_hitRecoilGameEffect.AddHit(this, pProjectileClass, damage, damageType, hitDirection);
	}	
}


//------------------------------------------------------------
// Some Crysis 2 features ------------------------------------
//------------------------------------------------------------

void CPlayer::UpdateCrysis2Features(float frameTime)
{
	//Gliding state
	UpdateAirFrictionState();

	if(!gEnv->bMultiplayer)
	{
		m_stealthKill.Update();
	}	

	//Grabbing onto ledges
	UpdateLedgeState();

}

//------------------------------------------------------------

void CPlayer::UpdateLedgeState()
{
	SPlayerStats::EOnLedgeTransition ledgeTransition = CanGrabOntoLedge();
	if(ledgeTransition != SPlayerStats::eOLT_None)
	{
		if(IsAirFrictionActive())
		{
			EnableAirFriction(false);
		}
		GrabOntoLedge(true, ledgeTransition);
	}

	if (g_pGameCVars->pl_ledgeClamber.debugDraw)
	{
		// Debug drawing of nearest ledge
		if (IsGrabbableLedgeNearby())
		{
			SLedge ledge = g_pGame->GetLedgeManager()->GetLedgeByIndex(m_stats.ledgeStats.nearestGrabbableLedgeIndex);
			IEntity* pLedge = gEnv->pEntitySystem->GetEntity(ledge.m_entityId);
			AABB box;
			pLedge->GetWorldBounds(box);
			Vec3 vLedgePosition = pLedge->GetWorldPos();
			ColorF cColor;
			float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
			if(IsOnLedge())
			{
				cColor = Col_Green;
			}

			CryFixedStringT<32> transitionType;
			switch(m_stats.ledgeStats.onLedgeTransition)
			{
			case SPlayerStats::eOLT_Falling: transitionType = "Falling Transition"; break;
			case SPlayerStats::eOLT_MidAir: transitionType = "MidAir Transition"; break;
			}

			//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(vLedgePosition, 0.1f, cColor);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(box, true, cColor,eBBD_Faceted);

			Vec3 vLedgeNormal = pLedge->GetForwardDir();
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(pLedge->GetWorldPos() + vLedgeNormal*0.3f, 0.05f, ColorB(0,255,255,255) );
			gEnv->pRenderer->DrawLabelEx(pLedge->GetWorldPos(), 1.25f, white, true, false, "%s", transitionType.c_str());
		}
	}
}

//--------------------------------------------------------------
void CPlayer::UpdateAirFrictionState()
{
	bool checkAirFricitionActivation = (m_stats.fallSpeed > 0.0f) && !IsAirFrictionActive();
	
	if(checkAirFricitionActivation)
	{
		const float currentHeight = GetEntity()->GetWorldPos().z;
		m_stats.startFallingHeight = (float) __fsel(-m_stats.startFallingHeight, currentHeight, m_stats.startFallingHeight);

		bool heightThresholdOk = ((m_stats.startFallingHeight - currentHeight) > g_pGameCVars->pl_airFrictionPerk_control.fall_activation_height);
		if(heightThresholdOk && IsPerkActive(ePerk_AirFriction))
		{
				EnableAirFriction(true);
		}
	}
	else if (IsAirFrictionActive())
	{
		if ((m_stats.onGround > 0.001f) || !IsPerkActive(ePerk_AirFriction))
		{
			EnableAirFriction(false);
		}
	}
}

//----------------------------------------------------
void CPlayer::EnableAirFriction(bool enable)
{
	m_stats.isAirFrictionActivated = enable;
	Vec3 impulse(0.0f, 0.0f, 0.0f);

	const SPlayerAirFrictionPerkControl& airFrictionControl = g_pGameCVars->pl_airFrictionPerk_control;

	if(enable)
	{
		AddAngularImpulse(Ang3(DEG2RAD(airFrictionControl.view_pitch_shake),RANDOM()*0.25f,RANDOM()*0.15f), 0.0f, airFrictionControl.view_recover_time);

		impulse.Set(0.0f, 0.0f, -m_stats.gravity.z * m_stats.mass);
	}
	else
	{
		//Only if still in air, if not we just landed
		if(m_stats.inAir > 0.001f)
		{
			AddAngularImpulse(Ang3(-DEG2RAD(airFrictionControl.view_pitch_shake),RANDOM()*0.25f,RANDOM()*0.15f), 0.0f, airFrictionControl.view_recover_time);

			impulse.Set(0.0f, 0.0f, m_stats.gravity.z * m_stats.mass * 0.5f);
		}
	}

	//Add some impulse up or down
	if (IPhysicalEntity *pPlayerPhysics = GetEntity()->GetPhysics())
	{	
		pe_action_impulse actionImp;
		actionImp.impulse = impulse;
		pPlayerPhysics->Action(&actionImp);
	}	

	//Notify nanosuit
	SNanoSuitEvent suitEvent;
	suitEvent.event = eNanoSuitEvent_AIRFRICTION;
	suitEvent.bParam = enable;
	SendActorSuitEvent(suitEvent);

	if (m_isClient)
	{
		CWeapon* pWeapon = GetWeapon(GetCurrentItemId());
		if (pWeapon)
		{
			pWeapon->OnPlayerAirFrictionActivate(enable);
		}
	}

	//Note: Perhaps would make sense to relate it to some other aspect?
	//CHANGED_NETWORK_STATE(this, CPlayer::ASPECT_INPUT_CLIENT);
}

//-----------------------------------------------------------
void CPlayer::SetSlideState(SPlayerStats::ESlidingState slideState)
{
	if(m_stats.slideStats.slidingState == slideState)
		return;

	m_stats.slideStats.slidingState = slideState;

	SNanoSuitEvent suitEvent;
	suitEvent.event = eNanoSuitEvent_SLIDE_ACTIVE;
	suitEvent.bParam = IsSliding();

	SActorParams *pParams = GetActorParams();

	if (slideState == SPlayerStats::eSS_Sliding)
	{
		pParams->vLimitDir = GetEntity()->GetWorldTM().GetColumn1();
		pParams->vLimitRangeH = DEG2RAD(m_playerRotationParams.GetHorizontalAimParams(SPlayerRotationParams::EAimType_SLIDING).angle_max);
		pParams->vLimitRangeVUp = DEG2RAD(m_playerRotationParams.GetHorizontalAimParams(SPlayerRotationParams::EAimType_SLIDING).angle_max);
		pParams->vLimitRangeVDown = DEG2RAD(m_playerRotationParams.GetHorizontalAimParams(SPlayerRotationParams::EAimType_SLIDING).angle_min);
		pParams->speedMultiplier = 0.0f;

		SendActorSuitEvent(suitEvent);
		PlayBreathingSound( EActionSoundParam_Sliding );
	}
	else 
	{
		pParams->vLimitDir.zero();
		pParams->vLimitRangeH = 0.0f;
		pParams->vLimitRangeVUp = pParams->vLimitRangeVDown = 0.0f;
		pParams->speedMultiplier = 1.0f;

		static_cast<CPlayerInput*>(GetPlayerInput())->ClearCrouchAction();
		GetAnimationGraphState()->SetInput("Action", "idle");

		if (slideState == SPlayerStats::eSS_None)
		{
			m_stats.slideStats.slideExitTime = 0.0f;
			SendActorSuitEvent(suitEvent);
		}
		else 
		{
			m_stats.slideStats.slideExitTime = gEnv->pTimer->GetAsyncCurTime();
		}
	}

	CHANGED_NETWORK_STATE(this, CPlayer::ASPECT_INPUT_CLIENT);
}

//-------------------------------------------------------
bool CPlayer::CanSlide() const
{
	if(gEnv->bMultiplayer && !IsPerkActive(ePerk_ManeuverMKII))
	{
		return false;
	}
	bool canSlide = (m_stats.bSprinting) && ((m_stats.onGround > 0.0f) || (m_stats.landed)) && (m_stats.speedFlat >= g_pGameCVars->pl_sliding_control.min_speed_threshold);

	return canSlide;
}

//-------------------------------------------------
bool CPlayer::IsAboutToFinishSlideTransition(float timeThreshold) const
{
	bool aboutToFinish = (m_stats.slideStats.slidingState == SPlayerStats::eSS_ExitingSlide) && ((gEnv->pTimer->GetAsyncCurTime() - m_stats.slideStats.slideExitTime) > timeThreshold);
	
	return aboutToFinish;
}

//----------------------------------------------------
namespace 
{
	bool raycast(IPhysicalEntity* pPE, const Vec3& pos, const Vec3& dir,  const int objects, ray_hit & hitResult)
	{
		const int flags = geom_colltype_player<<rwi_colltype_bit | rwi_stop_at_pierceable |rwi_ignore_back_faces;

		int hits = gEnv->pPhysicalWorld->RayWorldIntersection(pos, dir , objects, flags, &hitResult, 1, &pPE, pPE ? 1 : 0);

		bool debug = (g_pGameCVars->pl_grabOntoLedges == 2);
		if(debug)
		{
			if(hits > 0)
			{
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(pos, ColorB(0, 0, 255), hitResult.pt, ColorB(0, 0, 255), 3.0f);
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(hitResult.pt, 0.075f, ColorB(255, 255, 0));
			}
			else
			{
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(pos, ColorB(0, 0, 255), pos + dir, ColorB(0, 0, 255), 3.0f);
			}
		}

		return (hits > 0);
	}
}
//----------------------------------------------------
SPlayerStats::EOnLedgeTransition CPlayer::CanGrabOntoLedge()
{
	if(g_pGameCVars->pl_grabOntoLedges == 0 || (gEnv->bMultiplayer && !g_pGameCVars->g_MPLedgeGrabbing))
		return SPlayerStats::eOLT_None;

	//Only if in air, and not there already
	if(	IsOnLedge() || (m_stats.inAir < 0.01f) || (m_stats.pickAndThrowEntity != 0))
		return SPlayerStats::eOLT_None;

	//Time threshold
	if((gEnv->pTimer->GetAsyncCurTime() - m_stats.ledgeStats.lastTimeOnLedge) < 1.0f)
		return SPlayerStats::eOLT_None;

	UpdateNearestGrabbableLedge();

	return m_stats.ledgeStats.onLedgeTransition;
}

//----------------------------------------------------
void CPlayer::UpdateNearestGrabbableLedge()
{
	m_stats.ledgeStats.onLedgeTransition = SPlayerStats::eOLT_None;
	m_stats.ledgeStats.nearestGrabbableLedgeIndex = -1;

	// get the position and movement direction for this entity
	// prepare as parameters for the ledge manager
	Vec3 vPosition = GetEntity()->GetWorldPos();
	vPosition += Vec3(0.0f, 0.0f, GetStanceInfo(GetStance())->viewOffset.z);

	Vec3 vScanDirection = GetEntity()->GetWorldTM().TransformVector(m_ledgeGrabbingParams.m_ledgeNearbyParams.m_vSearchDir);
	vScanDirection.NormalizeFast();

	// retrieve the ledge manager and run a query on whether a ledge is within the parameters
	int iLedgeId = g_pGame->GetLedgeManager()->FindNearestLedge(vPosition, vScanDirection, m_ledgeGrabbingParams.m_ledgeNearbyParams.m_fMaxDistance, DEG2RAD(m_ledgeGrabbingParams.m_ledgeNearbyParams.m_fMaxAngleDeviationFromSearchDirInDegrees));
	if (iLedgeId != -1)
	{
		CWeapon* pCurrentWeapon = GetWeapon(GetCurrentItemId());
		bool canLedgeGrabWithWeapon = (pCurrentWeapon == NULL) || (pCurrentWeapon->IsRippedOff() == false);
		if (canLedgeGrabWithWeapon)
		{
			const bool characterMovesTowardLedge = IsCharacterMovingTowardsLedge(iLedgeId);
			const bool ledgePointsTowardCharacter = IsLedgePointingTowardCharacter(iLedgeId);
			if (characterMovesTowardLedge && ledgePointsTowardCharacter)
			{
				m_stats.ledgeStats.nearestGrabbableLedgeIndex = iLedgeId;
				m_stats.ledgeStats.onLedgeTransition = GetBestTransitionToLedge(GetEntity()->GetWorldPos(), iLedgeId);
			}	
		}
	}
}

//--------------------------------------------------------------------------------
bool CPlayer::IsCharacterMovingTowardsLedge(int ledgeIdx) const
{
	CRY_ASSERT_MESSAGE(ledgeIdx != -1, "Ledge index should be valid");

	// if stick is pressed forwards, always take this ledge
	Vec3 inputMovement = m_baseQuat.GetInverted() * GetLastRequestedVelocity();
	if (inputMovement.y < 0.2f) // 0.2f --> big enough to allow some room for stick deadzone
	{
		return false;
	}

	const IEntity* pCharacterEntity = GetEntity();
	const Vec3 vCharacterPos = pCharacterEntity->GetWorldPos();

	Vec3 vCharactertoLedge(ZERO);
	const CLedgeManager* pLedgeManager = g_pGame->GetLedgeManager();
	const SLedge detectedLedge = pLedgeManager->GetLedgeByIndex(ledgeIdx);
	pLedgeManager->FindVectorToClosestPointOnLedge(vCharacterPos, detectedLedge, &vCharactertoLedge);

	vCharactertoLedge.z = 0.0f;

	Vec3 vVelocity = m_stats.velocity;
	vVelocity.z = 0.0f;

	return (vCharactertoLedge * vVelocity > 0.0f);
}

//----------------------------------------------------------
bool CPlayer::IsLedgePointingTowardCharacter(int ledgeIdx) const
{
	CRY_ASSERT_MESSAGE(ledgeIdx != -1, "Ledge index should be valid");

	const CLedgeManager* pLedgeManager = g_pGame->GetLedgeManager();
	const SLedge detectedLedge = pLedgeManager->GetLedgeByIndex(ledgeIdx);

	const IEntity* pCharacterEntity = GetEntity();
	const IEntity* pLedgeEntity = gEnv->pEntitySystem->GetEntity(detectedLedge.m_entityId);
	const Vec3 vLedgePos = pLedgeEntity->GetWorldPos();
	const Vec3 vCharacterPos = pCharacterEntity->GetWorldPos();
	const Vec3 vLedgeToCharacter = vCharacterPos - vLedgePos;

	if ((pLedgeEntity->GetForwardDir() * vLedgeToCharacter) < 0.0f)
	{
		return false;
	}
	return true;
}

//---------------------------------------------------------
SPlayerStats::EOnLedgeTransition CPlayer::GetBestTransitionToLedge(const Vec3& refPosition, int ledgeIdx) const
{
	CRY_ASSERT_MESSAGE(ledgeIdx != -1, "Ledge index should be valid");

	const CLedgeManager* pLedgeManager = g_pGame->GetLedgeManager();
	const SLedge detectedLedge = pLedgeManager->GetLedgeByIndex(ledgeIdx);

	const IEntity* pLedgeEntity = gEnv->pEntitySystem->GetEntity(detectedLedge.m_entityId);
	const Vec3 ledgePos = pLedgeEntity->GetWorldPos();

	if (CanReachPlatform(ledgePos, refPosition))
	{
		return SPlayerStats::eOLT_None;
	}

	QuatT targetLocation;

	const Quat pRot = pLedgeEntity->GetWorldRotation();

	Vec3 vToVec(ZERO);
	pLedgeManager->FindVectorToClosestPointOnLedge(refPosition, detectedLedge, &vToVec);
	targetLocation.q = pRot * Quat::CreateRotationZ(DEG2RAD(180)); 

	float bestDistanceSqr = 100.0f;
	SPlayerStats::EOnLedgeTransition bestTransition = SPlayerStats::eOLT_None;

	//Get nearest entry point, from available transitions
	for (int i = 0; i < SPlayerStats::eOLT_MaxTransitions; ++i)
	{
		const SLedgeBlendingParams& ledgeBlendParams = m_ledgeGrabbingParams.m_grabTransitionsParams[i];
		targetLocation.t = refPosition + vToVec - (Vec3(0, 0, ledgeBlendParams.m_vPositionOffset.z)) - ((targetLocation.q * FORWARD_DIRECTION) * ledgeBlendParams.m_vPositionOffset.y);

		float distanceSqr = (targetLocation.t - refPosition).len2();
		if (distanceSqr < bestDistanceSqr)
		{
			bestTransition = (SPlayerStats::EOnLedgeTransition)(i);
			bestDistanceSqr = distanceSqr;
		}
	}

	return bestTransition;
}

//----------------------------------------------------------
bool CPlayer::CanReachPlatform(const Vec3& ledgePosition, const Vec3& refPosition) const
{
	const float heightDiff = (ledgePosition.z - refPosition.z);
	const Vec3 distanceVector = ledgePosition - refPosition;
	const float distanceSqr = distanceVector.len2();

	//Some rough prediction for vertical movement...
	const Vec2 distanceVector2D = Vec2(distanceVector.x, distanceVector.y);
	const float distance2DtoWall = distanceVector2D.GetLength();
	const float estimatedTimeToReachWall = m_stats.speedFlat ? distance2DtoWall / m_stats.speedFlat : distance2DtoWall;

	const float predictedHeightWhenReachingWall = refPosition.z + (m_stats.velocity.z * estimatedTimeToReachWall) + (0.5f * m_stats.gravity.z * (estimatedTimeToReachWall*estimatedTimeToReachWall));
	const float predictedHeightWhenReachingWallDiff = (ledgePosition.z - predictedHeightWhenReachingWall);

	//If falling already ...
	if (m_stats.velocity.z < 0.0f)
	{
		return (predictedHeightWhenReachingWallDiff < 0.1f);
	}
	else if (fabs(m_stats.gravity.z) > 0.0f)
	{
		//Going up ...
		const float estimatedTimeToReachZeroVerticalSpeed = -m_stats.velocity.z / m_stats.gravity.z;
		const float predictedHeightWhenReachingZeroSpeed = refPosition.z + (m_stats.velocity.z * estimatedTimeToReachZeroVerticalSpeed) + (0.5f * m_stats.gravity.z * (estimatedTimeToReachZeroVerticalSpeed*estimatedTimeToReachZeroVerticalSpeed));
		const float predictedHeightWhenReachingZeroSpeedDiff = (ledgePosition.z - predictedHeightWhenReachingZeroSpeed);

		return (predictedHeightWhenReachingZeroSpeedDiff < 0.1f);
	}

	return false;
}

//----------------------------------------------------------
void CPlayer::GrabOntoLedge(bool grab, SPlayerStats::EOnLedgeTransition transition)
{
	m_stats.ledgeStats.onLedge = grab;
	m_stats.ledgeStats.onLedgeTransition = transition;

	if(m_pAnimatedCharacter)
	{
		if(m_stats.ledgeStats.onLedge)
		{
			m_pAnimatedCharacter->RequestPhysicalColliderMode(eColliderMode_Disabled, eColliderModeLayer_Game, "CPlayer::GrabOntoLedge(entering)");
			PlayBreathingSound( EActionSoundParam_LedgeGrab );

			ResetFPWeapon();
			HolsterItem(true);

			bool isInPowerMode = (GetActorSuitGameParameters().GetMode() == eNanoSuitMode_Power);
			m_stats.ledgeStats.ledgeSpeedMultiplier = (isInPowerMode) ? m_ledgeGrabbingParams.m_fPowerSpeedUp : m_ledgeGrabbingParams.m_fNormalSpeedUp;

			if (transition == SPlayerStats::eOLT_MidAir)
			{
				const char* graphInput = (isInPowerMode) ? "ledgeGrabMidAirPower" : "ledgeGrabMidAir";
				m_pAnimatedCharacter->GetAnimationGraphState()->SetInput(m_inputAction, graphInput);
				StartLedgeBlending(m_ledgeGrabbingParams.m_grabTransitionsParams[SPlayerStats::eOLT_MidAir]);
			}
			else
			{
				CRY_TODO(26, 3, 2010, "Benito - Restore to previous setup after MS3, only trigger the first transition type as requested!")

				const char* graphInput = (isInPowerMode) ? "ledgeGrabMidAirPower" : "ledgeGrabMidAir";
				//const char* graphInput = (isInPowerMode) ? "ledgeGrabFallingPower" : "ledgeGrabFalling";

				m_pAnimatedCharacter->GetAnimationGraphState()->SetInput(m_inputAction, graphInput);
				StartLedgeBlending(m_ledgeGrabbingParams.m_grabTransitionsParams[SPlayerStats::eOLT_Falling]);
			}

			//TargetLocation is set in StartLedge blending
			m_params.vLimitDir = m_stats.ledgeBlending.m_qtTargetLocation.q.GetColumn1();
			m_params.vLimitRangeH = DEG2RAD(25.0f);
		}
		else
		{
			HolsterItem(false);

			m_stats.ledgeStats.lastTimeOnLedge = gEnv->pTimer->GetAsyncCurTime();

			m_params.vLimitDir.zero();
			m_params.vLimitRangeH = 0.0f;

			m_pAnimatedCharacter->RequestPhysicalColliderMode(eColliderMode_Undefined, eColliderModeLayer_Game, "Player::GrabOntoLedge(exiting)");
			m_pAnimatedCharacter->GetAnimationGraphState()->SetInput("Action","idle");
		}
	}
}

//--------------------------------------------------------------------------------
QuatT CPlayer::CalculateLedgeOffsetLocation( const Vec3& vPositionOffset ) const
{
	QuatT targetLocation;

	const CLedgeManager* pLedgeManager = g_pGame->GetLedgeManager();
	const SLedge ledge = pLedgeManager->GetLedgeByIndex(m_stats.ledgeStats.nearestGrabbableLedgeIndex);
	assert(ledge.IsValid());

	const IEntity* pLedgeEntity = gEnv->pEntitySystem->GetEntity(ledge.m_entityId);
	assert(pLedgeEntity);

	const Quat pRot = pLedgeEntity->GetWorldRotation();
	const Vec3 vWorldPos = GetEntity()->GetWorldPos();

	Vec3 vToVec(ZERO);
	pLedgeManager->FindVectorToClosestPointOnLedge(vWorldPos, ledge, &vToVec);

	targetLocation.q = pRot * Quat::CreateRotationZ(DEG2RAD(180)); 
	targetLocation.t = vWorldPos + vToVec - (Vec3(0, 0, vPositionOffset.z)) - ((targetLocation.q * FORWARD_DIRECTION) * vPositionOffset.y);

	return targetLocation;
}

//--------------------------------------------------------------------------------
void CPlayer::StartLedgeBlending( const SLedgeBlendingParams &blendingParams)
{
	assert(m_stats.ledgeStats.ledgeSpeedMultiplier > 0);

	m_stats.ledgeBlending.m_qtTargetLocation = CalculateLedgeOffsetLocation(blendingParams.m_vPositionOffset);
	m_stats.ledgeBlending.m_runningTimer = blendingParams.m_fMoveDuration / m_stats.ledgeStats.ledgeSpeedMultiplier;
	m_stats.ledgeBlending.m_correctionTimer = blendingParams.m_fCorrectionDuration / m_stats.ledgeStats.ledgeSpeedMultiplier;	

	CRY_ASSERT(m_stats.ledgeBlending.m_correctionTimer <= m_stats.ledgeBlending.m_runningTimer);

	m_stats.ledgeBlending.m_qtStartLocation.t = GetEntity()->GetWorldPos();
	m_stats.ledgeBlending.m_qtStartLocation.q = GetEntity()->GetWorldRotation();

	m_stats.ledgeBlending.m_qtCorrection.q = m_stats.ledgeBlending.m_qtStartLocation.q.GetInverted() * m_stats.ledgeBlending.m_qtTargetLocation.q;
	m_stats.ledgeBlending.m_qtCorrection.t = m_stats.ledgeBlending.m_qtTargetLocation.t - m_stats.ledgeBlending.m_qtStartLocation.t;
}

//--------------------------------------------------------------
void CPlayer::OnTeleported()
{
	//fix state here for example grabbing objects, turrets, vehicles ..
	GrabOntoLedge(false, SPlayerStats::eOLT_None);
	ResetAnimations();
}

//--------------------------------------------------------------
bool CPlayer::CanSwitchItems() const
{
	if (m_health && m_stats.animationControlled || ShouldSwim() || IsSliding() || IsAirFrictionActive() || IsInPickAndThrowMode() || IsSuperJumping())
		return false;

	if (g_pGame->GetGameRules() && !g_pGame->GetGameRules()->CanPlayerSwitchItem(GetEntity()->GetId()))
		return false;

	if (!CActor::CanSwitchItems())
	{
		return false;
	}

	return true;
}

//-----------------------------------------
bool CPlayer::HasHeavyWeaponEquipped() const
{
	IItem * pCurrentItem = GetCurrentItem();
	return (pCurrentItem && static_cast<CItem*>(pCurrentItem)->IsRippedOff());
}

//-----------------------------------------
bool CPlayer::HasHeavyWeaponEquipped(IItem * pCurrentItem) const
{
	return (pCurrentItem && static_cast<CItem*>(pCurrentItem)->IsRippedOff());
}

SPlayerRotationParams::EAimType CPlayer::GetCurrentAimType() const
{
	EStance playerStance = GetStance();

	if (IsSliding())
	{
		return SPlayerRotationParams::EAimType_SLIDING;
	}
	else if (m_stats.bSprinting)
	{
		return SPlayerRotationParams::EAimType_SPRINTING;
	}
	else if (playerStance == STANCE_CROUCH)
	{
		return SPlayerRotationParams::EAimType_CROUCH;
	}

	return SPlayerRotationParams::EAimType_NORMAL;
}




SAimAccelerationParams::SAimAccelerationParams()
	:	angle_min(-80.0f)
	,	angle_max(80.0f)
{
}


void SPlayerRotationParams::Reset(const IItemParamsNode* pRootNode)
{
	const IItemParamsNode* pParamNode = pRootNode->GetChild("PlayerRotation");
	if (pParamNode)
	{
		ReadAimParams(pParamNode, "Normal", EAimType_NORMAL);
		ReadAimParams(pParamNode, "Crouch", EAimType_CROUCH);
		ReadAimParams(pParamNode, "Sliding", EAimType_SLIDING);
		ReadAimParams(pParamNode, "Sprinting", EAimType_SPRINTING);
	}
}



void SPlayerRotationParams::ReadAimParams(const IItemParamsNode* pRootNode, const char* aimTypeName, EAimType aimType)
{
	const IItemParamsNode* pAimNode = pRootNode->GetChild(aimTypeName);

	if (pAimNode)
	{
		ReadAccelerationParams(pAimNode->GetChild("Horizontal"), &m_horizontalAims[aimType]);
		ReadAccelerationParams(pAimNode->GetChild("Vertical"), &m_verticalAims[aimType]);
	}
}



void SPlayerRotationParams::ReadAccelerationParams(const IItemParamsNode* pNode, SAimAccelerationParams* output)
{
	if (pNode)
	{
		pNode->GetAttribute("angle_min", output->angle_min);
		pNode->GetAttribute("angle_max", output->angle_max);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void SLedgeGrabbingParams::SetParamsFromXml(const IItemParamsNode* pParams)
{
	const IItemParamsNode* pLedgeGrabParams = pParams ? pParams->GetChild("LedgeGrabbingParams") : NULL;

	if (pLedgeGrabParams)
	{
		pLedgeGrabParams->GetAttribute("normalSpeedUp", m_fNormalSpeedUp);
		pLedgeGrabParams->GetAttribute("powerSpeedUp", m_fPowerSpeedUp);

		m_ledgeNearbyParams.SetParamsFromXml(pLedgeGrabParams->GetChild("LedgeNearByParams"));
		m_grabTransitionsParams[SPlayerStats::eOLT_MidAir].SetParamsFromXml(pLedgeGrabParams->GetChild("PullUpMidAir"));
		m_grabTransitionsParams[SPlayerStats::eOLT_Falling].SetParamsFromXml(pLedgeGrabParams->GetChild("PullUpFalling"));
	}
}

void SLedgeNearbyParams::SetParamsFromXml(const IItemParamsNode* pParams)
{
	if (pParams)
	{
		pParams->GetAttribute("searchDir", m_vSearchDir);
		pParams->GetAttribute("maxDistance", m_fMaxDistance);
		pParams->GetAttribute("maxAngleDeviationFromSearchDirInDegrees", m_fMaxAngleDeviationFromSearchDirInDegrees);
	}
}

void SLedgeBlendingParams::SetParamsFromXml(const IItemParamsNode* pParams)
{
	if (pParams)
	{
		pParams->GetAttribute("positionOffset", m_vPositionOffset);
		pParams->GetAttribute("moveDuration", m_fMoveDuration);
		pParams->GetAttribute("correctionDuration", m_fCorrectionDuration);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
				// crouch
				// lean right
				// lean left
void CPlayer::RegisterKill(IActor *pKilledActor, int hit_type)
{
	if (pKilledActor)
	{
		int friendly = IsFriendlyEntity(pKilledActor->GetEntityId());

		if (! friendly)
		{
			if (IsClient())
			{
				if (IsPerkActive(ePerk_ComboLink) && m_pNanoSuit && hit_type == g_pGame->GetGameRules()->GetHitTypeId("melee"))
				{
					SNanoSuitEvent suitEvent;
					suitEvent.event = eNanoSuitEvent_COMBO_LINK;
					this->SendActorSuitEvent(suitEvent);
				}
			}
		}

//	CHUD_Impl::GetInstance()->SetOnScreenMessageText("announcements", string().Format("You %s%s$o!", friendly ? "team-killed $2" : "killed $4", pKilledActor->GetEntity()->GetName()), NULL, 3.f);
	}
}

bool CPlayer::SetPerkActive(EPerks perkIndex, bool enable)
{
	uint64 previousPerkFlags = *m_perkChoice.GetPointerToBitfield();
	if (m_perkChoice.SetPerkActive(perkIndex, enable))
	{
		UpdateActivePerkInstances(previousPerkFlags, *m_perkChoice.GetPointerToBitfield());

		if(IGameObject *pGameObject = GetGameObject())
		{
			CHANGED_NETWORK_STATE(this, CPlayer::ASPECT_PERK_SETTINGS_CLIENT);
			return true;
		}
		else
		{
			assert( !"CPlayer::SetPerkActive() Expecting pGameObject to exist!" );
		}
	}
	return false;
}

EPerks CPlayer::GetPerkFromSlot(EPerkCategory slot)
{
	for(int i = 0; i < ePerk_Last; i++)
	{
		if(CPerk::GetInstance()->GetPerkData(i)->m_slot == slot && IsPerkActive((EPerks)i))
		{
			return (EPerks)i;
		}
	}
	return ePerk_Null;
}

float CPlayer::GetReloadSpeedScale(void) const
{
	return GetModifiableValues().GetValue(kPMV_WeaponReloadSpeedScale);
}

void CPlayer::GenerateCloakAwarenessParticles(int num)
{
	const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
	IEntity *pEntity = GetEntity();

	SSkeletonEmitterParams emitterParams;
	emitterParams.pPlayer = this;
	emitterParams.pEmitter1 = m_perkParticleEmitterInfo.GetEmitter(GetEntity(), ePPPET_CloakAwareness, CLOAK_AWARENESS_PARTICLES);
	emitterParams.boneEmitterNumParticles[eBoneEmitter_Skeleton] = num;
	emitterParams.scaleVelocityMin = perkVars->perk_cloakAwareness_skelAlignedParticles_awayVelocityMin;
	emitterParams.scaleVelocityMax = perkVars->perk_cloakAwareness_skelAlignedParticles_awayVelocityMax;
	emitterParams.velocityUpMin = perkVars->perk_cloakAwareness_skelAlignedParticles_upVelocityMin;
	emitterParams.velocityUpMax = perkVars->perk_cloakAwareness_skelAlignedParticles_upVelocityMax;
	emitterParams.useEntityVel = true;

	m_bodyParticleCreator.GenerateParticlesOnSkeleton(emitterParams);

	CAudioSignalPlayer::JustPlay("Perk_CloakAwareness", pEntity->GetId());
}

void CPlayer::UpdatePerkSonarVision(SEntityUpdateContext& ctx)
{
	bool  anyParticlesToCreate = false;
	for(int i=0; i<eMAX_BoneEmitters; i++)
	{
		if (m_sonarVisionNumParticlesNextFrame[i] > 0)
		{
			anyParticlesToCreate = true;
			break;
		}
	}

	if (anyParticlesToCreate)
	{
		IEntityRenderProxy* renderProxy = (IEntityRenderProxy*)GetEntity()->GetProxy(ENTITY_PROXY_RENDER);

		if(renderProxy)
		{
			AABB playerAABB;
			renderProxy->GetWorldBounds(playerAABB);
			if(gEnv->p3DEngine->GetCurrentCamera().IsAABBVisible_FH(playerAABB) != CULL_EXCLUSION) // Only spawn particles on visible players
			{
				const CPerk::SPerkVars* perkVars = CPerk::GetInstance()->GetVars();
				IActor *pClient=gEnv->pGame->GetIGameFramework()->GetClientActor();
				CPlayer *pPlayerClient=static_cast<CPlayer*>(pClient);

				if(pPlayerClient && pClient)
				{
					const bool permanentEnabled = (gEnv->bMultiplayer && g_pGameCVars->g_mpPermanentSonarVision);

					// Only spawn if player is within sonar vision max view distance
					float sonarVisionMaxViewDistSq = 0.0f;
					if (permanentEnabled)
					{
						sonarVisionMaxViewDistSq = perkVars->perk_sonarVision_perm_maxDist;
					}
					else if (pPlayerClient->GetNanoSuit()->IsSuitPowerActive())
					{
						sonarVisionMaxViewDistSq = perkVars->perk_sonarVision_activeMaxViewDist;
					}
					else
					{
						sonarVisionMaxViewDistSq = perkVars->perk_sonarVision_passiveMaxViewDist;
					}
					sonarVisionMaxViewDistSq *= sonarVisionMaxViewDistSq;

					const float distSq = (pPlayerClient->GetEntity()->GetWorldPos() - GetEntity()->GetWorldPos()).GetLengthSquared();

					if(distSq < sonarVisionMaxViewDistSq)
					{
						const SNanoSuitGameParameters & viewerSuitParams = pPlayerClient->GetActorSuitGameParameters();
						const SNanoSuitGameParameters & emitterSuitParams = GetActorSuitGameParameters();

						SSkeletonEmitterParams emitterParams;
						emitterParams.pPlayer = this;
						emitterParams.pEmitter1 = m_perkParticleEmitterInfo.GetEmitter(GetEntity(), ePPPET_Movement, SONAR_VISION_MOVEMENT_SYMBOL_PARTICLES);
						emitterParams.pEmitter2 = m_perkParticleEmitterInfo.GetEmitter(GetEntity(), ePPPET_Refraction, SONAR_VISION_MOVEMENT_SMOKE_PARTICLES);

						for(int i=0; i<emitterParams.boneEmitterNumParticles.size(); i++)
						{
							emitterParams.boneEmitterNumParticles[i] = m_sonarVisionNumParticlesNextFrame[i];
						}

						emitterParams.scaleVelocityMin = perkVars->perk_sonarVision_skelAlignedParticles_awayVelocityMin;
						emitterParams.scaleVelocityMax = perkVars->perk_sonarVision_skelAlignedParticles_awayVelocityMax;
						emitterParams.velocityUpMin = perkVars->perk_sonarVision_skelAlignedParticles_upVelocityMin;
						emitterParams.velocityUpMax = perkVars->perk_sonarVision_skelAlignedParticles_upVelocityMax;
						emitterParams.useEntityVel = false;
						emitterParams.boneEmitterSpawnRadius = perkVars->perk_sonarVision_skelAlignedParticles_boneEmitterRadius;
						if (permanentEnabled)
						{
							emitterParams.scale = perkVars->perk_sonarVision_perm_baseScale;
							emitterParams.maxScaleForAllBoneEmitter = perkVars->perk_sonarVision_perm_randScale;
							emitterParams.randomiseScaleForAllBoneEmitter = true;
						}
						else
						{
							emitterParams.scale = 1.0f;
							emitterParams.maxScaleForAllBoneEmitter = perkVars->perk_sonarVision_maxScaleForAllBoneEmitter;
							emitterParams.randomiseScaleForAllBoneEmitter = true;
						}
						emitterParams.randomOffsetScale = perkVars->perk_sonarVision_skelAlignedParticles_randomOffsetScale;
						emitterParams.emitterSpawnRatio = perkVars->perk_sonarVision_emitterSpawnRatio;

						m_bodyParticleCreator.GenerateParticlesOnSkeleton(emitterParams);
						CPerkSonarVisionManager::GetMgr()->InformCalculatedSonarVisionScale(perkVars->perk_sonarVision_VolumeAffectedByMovement);
						CCCPOINT(Perk_SonarVision_GenerateParticles);
					}
				}
			}
		}

		for(int i=0; i<eMAX_BoneEmitters; i++)
		{
			m_sonarVisionNumParticlesNextFrame[i] = 0;
		}
	}
	m_perkParticleEmitterInfo.Tick(ctx);
}

void CPlayer::UpdatePerkSolarBoost(SEntityUpdateContext& ctx)
{
	if(m_pNanoSuit)		
	{
		AABB playerAABB;
		GetEntity()->GetWorldBounds(playerAABB);
		const Vec3 player_pos = playerAABB.GetCenter();
		Vec3 sun_dir = gEnv->p3DEngine->GetSunDirNormalized(); // Get the sun direction

		const static float k_perkSolarBoostTestRange = 30.0f;

		primitives::sphere sphere;
		sphere.center = player_pos;
		sphere.r = 0.5f;

		float dist = gEnv->pPhysicalWorld->PrimitiveWorldIntersection(sphere.type, &sphere, sun_dir*k_perkSolarBoostTestRange, ent_static|ent_terrain);

		if( dist == 0.0f )
		{
			m_pNanoSuit->ScaleEnergyUse(CPerk::GetInstance()->GetVars()->perk_solarBoostScaleEnergyUse);
		}
	}
}

void CPlayer::PerkRequestDamage(EPerks perk, float damage)
{
	if(gEnv->bServer)
	{
			HitInfo hitInfo;
			hitInfo.targetId = GetEntityId();
			hitInfo.damage = damage;
			CGameRules *rules = g_pGame->GetGameRules();
			//event type will instant kill
			hitInfo.type = damage >= 100.0f ? rules->GetHitTypeId("event") : rules->GetHitTypeId("normal");
			rules->ServerHit(hitInfo);
	}
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CPlayer, ClSetThesePerks)
{
	ClUpdatePerks(params.perkBitField);
	m_localPerksSet = true;

	return true;
}

uint64 CPlayer::NetGetPerksBitfield() const
{
	return m_perkChoice.GetBitfield();
}

void CPlayer::NetSetPerksBitfield(uint64 newBitfield)
{
	if (m_localPerksSet)
	{
		return;
	}

	ClUpdatePerks(newBitfield);
}

void CPlayer::ClUpdatePerks(uint64 newBitfield)
{
	if (!m_perkChoice.IsAllowed() && (newBitfield != (uint64)0))
	{
		return;
	}

	uint64 currentBitfield = m_perkChoice.GetBitfield();
	if(currentBitfield != newBitfield)
	{
		m_perkChoice.SetBitfield(newBitfield);
		CHANGED_NETWORK_STATE(this, CPlayer::ASPECT_PERK_SETTINGS_CLIENT);
		UpdateActivePerkInstances(currentBitfield, newBitfield);
	}
}

void CPlayer::ResetPerksVariables()
{
	while(m_sonarVisionNumParticlesNextFrame.size() < m_sonarVisionNumParticlesNextFrame.max_size())
	{
		m_sonarVisionNumParticlesNextFrame.push_back(0);
	}

	for(int i=0; i<eMAX_BoneEmitters; i++)
	{
		m_sonarVisionNumParticlesNextFrame[i] = 0;
	}
}

void CPlayer::ResetFPWeapon()
{
	if (!IsThirdPerson())
	{
		CWeapon *weapon = GetWeapon(GetCurrentItemId());
		if (weapon)
		{
			bool releaseCameraBone = false;
			bool enableWeaponAim = weapon->UpdateAimAnims(m_weaponParams, releaseCameraBone);
			m_weaponFPAiming.SetActive(enableWeaponAim);
			m_weaponFPAiming.SnapSwitch(true);
			m_weaponFPAiming.Update(m_weaponParams);
			m_torsoAimIK.SetCameraBonePinning(!releaseCameraBone);
		}
	}
}

void CPlayer::EquipAttachmentsOnCarriedWeapons()
{
	IInventory *pInventory=GetInventory();
	if (pInventory)
	{
		const int itemCount = pInventory->GetCount();
		for(int j = 0; j < itemCount; j++)
		{
			EntityId itemId = pInventory->GetItem(j);

			IItem * pItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(itemId);

			CRY_ASSERT_MESSAGE(pItem, "No item found!");

			IWeapon *pWeapon = pItem->GetIWeapon();

			if(pWeapon)
			{
				const int accessoryCount = pInventory->GetAccessoryCount();
				for (int i = 0; i < accessoryCount; i++)
				{
					const char* name = pInventory->GetAccessory(i);

					if (name)
					{
						CItem * pCItem = static_cast<CItem*>(pItem);
						const SAccessoryParams *invAccessory = pCItem->GetAccessoryParams(name);

						if(invAccessory)
						{
							pCItem->AttachAccessory(name, true, true, true);
						}
					}
				}
			}
		}
	}
}

void CPlayer::UpdateIPerks(const float dt)
{
	for(int i = 0; i < m_numActivePlayerPlugins; i++)
	{
		m_activePlayerPlugins[i]->Update(dt);
	}
}

void CPlayer::UpdatePerks(SEntityUpdateContext& ctx)
{
#if 0
	if (IsClient())
	{
		CryWatch("Perks: %s", m_perkChoice.GetListAsString().c_str());
	}
#endif

	if (!m_stats.isRagDoll)
	{
		if (IActor* pClient=gEnv->pGame->GetIGameFramework()->GetClientActor())
		{
			CPlayer*  pPlayerClient = static_cast< CPlayer* >( pClient );

			const bool permanentEnabled = (gEnv->bMultiplayer && g_pGameCVars->g_mpPermanentSonarVision);
			const bool  sonarVisionPerkIsActive = (permanentEnabled || pPlayerClient->IsPerkActive(ePerk_SonarVision));

			if (sonarVisionPerkIsActive)
			{
				if (permanentEnabled)
				{
					const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();

					if (fmod((g_pGame->GetGameRules()->GetServerTime() / 1000.f), perkVars->perk_sonarVision_perm_interval) < ctx.fFrameTime)
					{
						if (m_sonarVisionNumParticlesNextFrame[eBoneEmitter_Skeleton] == 0)
						{
							SonarVisionPerk_SetNumParticlesToCreate(perkVars->perk_sonarVision_perm_numCreate, eBoneEmitter_Skeleton);
						}
					}
				}

				UpdatePerkSonarVision(ctx);
			}
		}
	}
	
	if(IsPerkActive(ePerk_SolarCellSkin))
	{
		UpdatePerkSolarBoost(ctx);
	}

}

void CPlayer::SonarVisionPerk_SetNumParticlesToCreate(int numParticles, EBoneEmitterLocation boneEmitterLocation)
{
	IActor *pClient = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (pClient)
	{
		CPlayer *pPlayerClient = static_cast<CPlayer*>(pClient);

		const bool sonarVisionPerkIsActive = (pPlayerClient->IsPerkActive(ePerk_SonarVision) || (gEnv->bMultiplayer && g_pGameCVars->g_mpPermanentSonarVision));
		const bool shouldGenerateEffectsForEntity = CPlayerPerkParticleInfo::ShouldGenerateEffectsForEntity(GetEntityId(), pPlayerClient, ePPPEI_Body);
		const bool isHologram = (GetActorClass() == CPlayerHologram::GetActorClassType());

		if(sonarVisionPerkIsActive && shouldGenerateEffectsForEntity && (!isHologram))
		{
			m_sonarVisionNumParticlesNextFrame[boneEmitterLocation] = max(numParticles, m_sonarVisionNumParticlesNextFrame[boneEmitterLocation]);
		}
	}
}

void CPlayer::SonarVisPerkRegisterGunFire(IEntityClass* pAmmoType)
{
	const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();

	IActor *pClient=gEnv->pGame->GetIGameFramework()->GetClientActor();
	CPlayer *pPlayer=static_cast<CPlayer*>(pClient);

	if (pPlayer)
	{
		const SNanoSuitGameParameters & viewerSuitParams = pPlayer->GetActorSuitGameParameters();
		const SNanoSuitGameParameters & emitterSuitParams = GetActorSuitGameParameters();

		bool enable = (viewerSuitParams.GetMode() == eNanoSuitMode_Tactical) && ! emitterSuitParams.IsCloakEnabled();

		if (enable && CPlayerPerkParticleInfo::ShouldGenerateEffectsForEntity(GetEntityId(), pPlayer, ePPPEI_Gunfire))
		{
			const SAmmoParams *projectileParams = g_pGame->GetWeaponSystem()->GetAmmoParams(pAmmoType);
			const int BULLET_TYPE_GRENADE = -1;
			if((projectileParams) && (projectileParams->bulletType != BULLET_TYPE_GRENADE))
			{
				m_perkParticleEmitterInfo.AddNoiseBurst(perkVars->perk_sonarVision_IncreaseParticleSizeWhenShoot);
				SonarVisionPerk_SetNumParticlesToCreate(perkVars->perk_sonarVision_skelAlignedParticles_numToCreate_shoot,eBoneEmitter_Weapon);
				SonarVisionPerk_SetNumParticlesToCreate(perkVars->perk_sonarVision_skelAlignedParticles_numToCreate_random,eBoneEmitter_Skeleton);

				float reduceAmountWhenBusy = CPerkSonarVisionManager::GetMgr()->GetObserver()->GetRangeReductionValue() * perkVars->perk_sonarVision_VolumeControlReduceBodyEffects;
				float effectScale = m_perkParticleEmitterInfo.CalculateBaseEffectScale(GetEntity()->GetPos(), pPlayer, perkVars->perk_sonarVision_RangeGunshot, reduceAmountWhenBusy);
				CPerkSonarVisionManager::GetMgr()->InformCalculatedSonarVisionScale(effectScale * perkVars->perk_sonarVision_VolumeAffectedByGunshot);
			}
		}
	}
}

bool CPlayer::IsPerkActive(EPerks perk) const
{
	if(m_perkChoice.IsPerkActive(perk))
	{
		CPerk * perkInstance = CPerk::GetInstance();
		if (perkInstance->GetVars()->perk_disable || (perkInstance->GetLockedPerksBitfield() & PERKBIT(perk)))
		{
			return false;
		}

		const SNanoSuitGameParameters& suitParams = GetActorSuitGameParameters();
		if(!(suitParams.GetState()&~eNanoSuitState_Disabled))
		{
			return false;
		}

		ENanoSuitMode requiredMode = perkInstance->GetPerkRequiredSuitMode(perk);
		if(requiredMode != eNanoSuitMode_Last && requiredMode != suitParams.GetMode())
		{
			return false;
		}

		return true;
	}
	
	return false;
}

void CPlayer::SendPerkEvent(EPlayerPlugInEvent perkEvent, void * data)
{
	assert (perkEvent >= 0 && perkEvent < EPE_Num);

	CRecordingSystem* pRecordingSystem = g_pGame->GetRecordingSystem();
	if (pRecordingSystem)
	{
		pRecordingSystem->OnPerkEvent(this, perkEvent, data);
	}

	for(int i = 0; i < m_numActivePlayerPlugins; i++)
	{
		int oldPerkActiveCount = m_numActivePlayerPlugins;
		CPlayerPlugin * thisPlugin = m_activePlayerPlugins[i];

		CRY_ASSERT(thisPlugin->IsEntered());
		thisPlugin->HandleEvent(perkEvent, data);
		
		if (! thisPlugin->IsEntered())
		{
#if PLAYER_PLUGIN_DEBUGGING
			CRY_ASSERT_MESSAGE (thisPlugin != m_activePlayerPlugins[i], "Plug-in was disabled but not removed from list");
			CryLog ("[PLAYER PLUG-IN] %s left %s while being sent event %s!", GetEntity()->GetName(), thisPlugin->DbgGetClassDetails().c_str(), CPlayerPlugin::s_playerPluginEventNames[perkEvent]);
#endif
			-- i;
		}
	}
}

#undef raycast

#if ENABLE_MP_SPAWNMODES
bool CPlayer::SetSpawnModeType( ESpawnModeType newType )
{
	bool spawnModeChanged=false;

	int newTypeInt = static_cast<int>(newType);

	if (m_spawnmode_type == newTypeInt)
	{
		CryLog("CPlayer::SetSpawnModeType() player %s not changing type as newType is same as currenttype=%d", GetEntity()->GetName(), m_spawnmode_type);
	}
	else
	{
		CryLog("CPlayer::SetSpawnModeType() player %s changing type from %d to %d - changed network state ASPECT_SPAWNMODE_TYPE", GetEntity()->GetName(), m_spawnmode_type, newTypeInt);
		m_spawnmode_type = newTypeInt;
		CHANGED_NETWORK_STATE(this, ASPECT_SPAWNMODE_TYPE);
		spawnModeChanged=true;
	}

	return spawnModeChanged;
}
#endif

void CPlayer::SetActorModel()
{
	CActor::SetActorModel();

	m_torsoAimIK.Reset();
	m_lookAim.Reset();

	//--- Update animationPlayerProxies & toggle the part visibility for separate character shadow casting
	m_animationProxy.SetFirstPerson(!IsThirdPerson());
	m_animationProxyUpper.SetFirstPerson(!IsThirdPerson());
	UpdateVisibility();

	if (HasShadowCharacter())
	{
		m_pAnimatedCharacter->SetShadowCharacterSlot(GetShadowCharacterSlot());
		//GetShadowCharacter()->GetISkeletonAnim()->SetEventCallback(NULL, NULL);
	}

	ICharacterInstance *mainChar = GetEntity()->GetCharacter(0);
	CRY_ASSERT_MESSAGE(mainChar != 0, "Character instance is NULL");
	if (mainChar)
	{
		//--- Initialise the weapon params cached model pointers
		m_weaponParams.skelAnim = mainChar->GetISkeletonAnim();
		m_weaponParams.characterInst = mainChar;

		CAnimationProxyDualCharacterBase::RegisterAnimSet(mainChar->GetIAnimationSet());
	}

	m_bodyParticleCreator.Initialise(this);
}

void CPlayer::UpdateActivePerkInstances(const uint64 previousPerkFlags, const uint64 newPerkFlags)
{
	assert (newPerkFlags == *m_perkChoice.GetPointerToBitfield());

	if (previousPerkFlags != newPerkFlags)
	{
		int i;
		CPerkIconData * perkIcons = CPerkIconData::GetForEntity(GetEntityId());

		// Add/remove perk icons before entering perk classes so that the perk classes can modify icon info in their constructors... [TF]
		if (perkIcons)
		{
			uint64 bit;

			CryLogAlways ("Local player's perks changing to: %s", m_perkChoice.GetListAsString().c_str());
			CRY_ASSERT_MESSAGE(m_perkChoice.CountPerksActive() <= k_numberOfPerkIcons, string().Format("%d is too many perks to have active: %s", m_perkChoice.CountPerksActive(), m_perkChoice.GetListAsString().c_str()));

			// Remove icons for any perks which have become deactivated
			for (bit = 1, i = 0; i < ePerk_Last; bit <<= 1, ++ i)
			{
				if ((newPerkFlags & bit) == 0 && (previousPerkFlags & bit))
				{
					perkIcons->RemoveIcon((EPerks)i);
				}
			}

			// Then add an icon for each perk which has become activated - NB: we can't merge these into one loop, otherwise we can run out of perk icon slots!
			for (bit = 1, i = 0; i < ePerk_Last; bit <<= 1, ++ i)
			{
				if ((newPerkFlags & bit) && ((previousPerkFlags & bit) == 0))
				{
					perkIcons->AddIcon((EPerks)i);
				}
			}
		}

		for(i = 0; i < ePerk_Last; i++)
		{
			EPerks perk = (EPerks) i;
			if(CPerkChoice::IsPerkActive(previousPerkFlags, perk) && !CPerkChoice::IsPerkActive(newPerkFlags, perk))
			{
				LeavePlayerPlugin(m_perkInstances[perk]);
			}
			else if (!CPerkChoice::IsPerkActive(previousPerkFlags, perk) && CPerkChoice::IsPerkActive(newPerkFlags, perk))
			{
				EnterPlayerPlugin(m_perkInstances[perk]);
			}
		}
	}
}

void CPlayer::EnterPlayerPlugin(CPlayerPlugin * pluginClass)
{
	if(pluginClass)
	{
		if (m_numActivePlayerPlugins < k_maxActivePlayerPlugIns)
		{
			CRY_ASSERT_MESSAGE(this == pluginClass->GetOwnerPlayer(), string().Format("Player '%s' shouldn't enter a player plug-in owned by player '%s'", GetEntity()->GetName(), pluginClass->GetOwnerPlayer() ? pluginClass->GetOwnerPlayer()->GetEntity()->GetName() : "NULL"));
			pluginClass->Enter();

			CRY_ASSERT_MESSAGE(pluginClass->IsEntered(), "CPlayerPlugin was entered but IsEntered() is still returning false! Maybe the base class Enter function wasn't called from the subclass?");
			ASSERT_IS_NULL(m_activePlayerPlugins[m_numActivePlayerPlugins]);
			m_activePlayerPlugins[m_numActivePlayerPlugins] = pluginClass;
			m_numActivePlayerPlugins++;
		}
#if PLAYER_PLUGIN_DEBUGGING
		else
		{
			CryLog ("Can't enter plug-in \"%s\" because array of size %d is full:", pluginClass->DbgGetClassDetails().c_str(), k_maxActivePlayerPlugIns);
			for (int i = 0; i < k_maxActivePlayerPlugIns; ++ i)
			{
				CryLog ("    %d/%d: %s", i + 1, k_maxActivePlayerPlugIns, m_activePlayerPlugins[i]->DbgGetClassDetails().c_str());
		}
			CRY_ASSERT_MESSAGE(0, string().Format("Can't enter plug-in \"%s\" because array of size %d is full! Maybe you need to increase k_maxActivePlayerPlugIns...", pluginClass->DbgGetClassDetails().c_str(), k_maxActivePlayerPlugIns));
	}
#endif
	}
}

void CPlayer::LeavePlayerPlugin(CPlayerPlugin * pluginClass)
{
	if(pluginClass)
	{
		pluginClass->Leave();
		CRY_ASSERT_MESSAGE(!pluginClass->IsEntered(), "CPlayerPlugin was left but IsEntered() is still returning true! Maybe the base class Leave function wasn't called from the subclass?");

		for(int i = 0; i < m_numActivePlayerPlugins; i++)
		{
			ASSERT_IS_NOT_NULL(m_activePlayerPlugins[i]);

			if(m_activePlayerPlugins[i] == pluginClass)
			{
				m_numActivePlayerPlugins--;
				m_activePlayerPlugins[i] = m_activePlayerPlugins[m_numActivePlayerPlugins];
				m_activePlayerPlugins[m_numActivePlayerPlugins] = NULL;
				return;
			}
		}

		CRY_ASSERT_MESSAGE(false, string().Format("Player was told to leave plug-in %p which isn't in the list of active plug-ins!", pluginClass));
	}
}

void CPlayer::LeaveAllPlayerPlugins()
{
	uint64 currentBitfield = m_perkChoice.GetBitfield();
	m_perkChoice.SetBitfield(0);
	CHANGED_NETWORK_STATE(this, CPlayer::ASPECT_PERK_SETTINGS_CLIENT);
	UpdateActivePerkInstances(currentBitfield, 0);

	for(int i = 0; i < m_numActivePlayerPlugins; i++)
	{
		ASSERT_IS_NOT_NULL (m_activePlayerPlugins[i]);
		assert(m_activePlayerPlugins[i]->IsEntered());
		m_activePlayerPlugins[i]->Leave();
		CRY_ASSERT_MESSAGE(!m_activePlayerPlugins[i]->IsEntered(), "CPlayerPlugin was left but IsEntered() is still returning true! Maybe the base class Leave function wasn't called from the subclass?");
		m_activePlayerPlugins[i] = NULL;
	}

	m_numActivePlayerPlugins = 0;
}

float CPlayer::GetWeaponMovementFactor() const
{
	CWeapon* pWeapon = GetWeapon(GetCurrentItemId());
	if (pWeapon)
		return pWeapon->GetMovementModifier();
	return 1.0;
}

float CPlayer::GetWeaponRotationFactor() const
{
	CWeapon* pWeapon = GetWeapon(GetCurrentItemId());
	float rotationScale = 1.0f;

	if (pWeapon)
	{
		rotationScale = pWeapon->GetRotationModifier();

	if (m_coverAndLean.IsLeaning())
		rotationScale *= pWeapon->GetCoverAndLeanModifier();
	}

	return rotationScale;
}

void CPlayer::ModeChanged(ENanoSuitMode mode)
{
	CCCPOINT(Nanosuit_InformPlayerWhenModeChanges);
	SendPerkEvent(EPE_ChangedSuitMode);
}

void CPlayer::OnSuitStateChanged(ENanoSuitState state)
{
	CCCPOINT(Nanosuit_InformPlayerWhenStateChanges);
	SendPerkEvent(EPE_ChangedSuitState);
}

void CPlayer::Physicalize( EStance stance/*=STANCE_NULL*/ )
{
	CActor::Physicalize(stance);

	//Disable physics hit reactions on client (only way it is setting skeleton mass to 0)
	if (IsClient())
	{
		ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
		IPhysicalEntity* pCharacterPhysics = pCharacter ? pCharacter->GetISkeletonPose()->GetCharacterPhysics() : NULL;
		if (pCharacterPhysics != NULL)
		{
			pe_simulation_params simParams;
			simParams.mass = 0.0f; 

			pCharacterPhysics->SetParams(&simParams);
		}
	}

	PhysicalizeBodyDamage();
}

void CPlayer::PhysicalizeBodyDamage()
{
	if (m_pBodyDamage)
	{
		m_pBodyDamage->Physicalize(*this);
	}
}

void CPlayer::UpdateMountedGunController( bool forceIKUpdate )
{
	const float frameTime = gEnv->pTimer->GetFrameTime();

	m_mountedGunController.Update(m_stats.mountedWeaponID, frameTime);

	if (forceIKUpdate && (IsThirdPerson() == false))
	{
		IMovementController* pMovementController = GetMovementController();
		assert(pMovementController);
		SMovementState info;
		pMovementController->GetMovementState(info);

		CIKTorsoAim_Helper::SIKTorsoParams IKParams(GetEntity()->GetCharacter(0), GetShadowCharacter(), info.aimDirection, ZERO, m_params.mountedWeaponCameraTarget);
		m_torsoAimIK.Update(IKParams);
	}
}

void CPlayer::StartFlashbangEffects(float flashBangTime)
{
	if(GetHealth() > 0 && m_stats.spectatorInfo.mode == eASM_None)	//if you do a flashbang when you are dead it doesn't clear the effects properly
	{
		m_stats.flashBangStunLength = m_stats.flashBangStunTimer = flashBangTime;
		m_stats.flashBangStunMult = g_pGameCVars->g_flashBangMinSpeedMultiplier;

		StartTinnitus();

		SHUDEvent hudEvent_onBlind(eHUDEvent_OnBlind);
		hudEvent_onBlind.ReserveData(1);
		SHUDEventData dataAlpha(0.0f);
		hudEvent_onBlind.AddData(dataAlpha);
		CHUD::CallEvent(hudEvent_onBlind);
	}
}

void CPlayer::StartTinnitus()
{
	if(GetHealth() > 0 && m_stats.spectatorInfo.mode == eASM_None)
	{
		CAudioSignalPlayer::JustPlay("FlashbangEnter");

		Vec3 zero = Vec3(0.0f, 0.0f, 0.0f);
		
		if(m_flashbangSound == INVALID_SOUNDID)
		{
			// [Tomas] TODO please avoid hardcoded sound references, use Game Audio Signal System instead
			m_flashbangSound = m_pSoundProxy->PlaySound("Sounds/crysiswars2:weapons:flashbang/flashbang_tinitus", zero, zero, FLAG_SOUND_EVENT, eSoundSemantic_Player_Foley);
		}
		
		if(m_flashbangSound != INVALID_SOUNDID)
		{
			ISound* pSound = m_pSoundProxy->GetSound(m_flashbangSound);
			if(pSound)
			{
				pSound->SetParam("effect", 1.0f);
			}
		}
	}
}

void CPlayer::UpdateFlashbangEffect(float frameTime)
{
	if (m_stats.flashBangStunTimer > 0.f)
	{
		m_stats.flashBangStunTimer = max(0.f, m_stats.flashBangStunTimer - frameTime);

		if (m_stats.flashBangStunTimer <= 0.f)
		{
			StopFlashbangEffects();
		}
		else
		{
			m_stats.flashBangStunMult = LERP(m_stats.flashBangStunMult, 1.f, min(1.f, (1.f/(m_stats.flashBangStunTimer*g_pGameCVars->g_flashBangSpeedMultiplierFallOffEase)) * frameTime)); 
			float tinnitusFraction = clamp(m_stats.flashBangStunTimer/m_stats.flashBangStunLength, 0.0f, 1.0f);
			UpdateTinnitus(tinnitusFraction);

			SHUDEvent hudEvent_onBlind(eHUDEvent_OnBlind);
			hudEvent_onBlind.ReserveData(1);
			SHUDEventData dataAlpha(1.0f - tinnitusFraction);
			hudEvent_onBlind.AddData(dataAlpha);
			CHUD::CallEvent(hudEvent_onBlind);

		}
	}
}

void CPlayer::UpdateTinnitus(float tinnitusFraction)
{
	if(m_flashbangSound != INVALID_SOUNDID)
	{
		ISound* pSound = m_pSoundProxy->GetSound(m_flashbangSound);
		if (pSound)
			pSound->SetParam("effect", tinnitusFraction);
	}
}

void CPlayer::StopFlashbangEffects()
{
	m_stats.flashBangStunMult = 1.f;

	StopTinnitus();

	CHUD::CallEvent(SHUDEvent(eHUDEvent_OnEndBlind));
}

void CPlayer::StopTinnitus()
{
	if(m_pSoundProxy)
	{
		m_pSoundProxy->StopSound(m_flashbangSound);
	}

	CAudioSignalPlayer::JustPlay("FlashbangLeave");
	m_flashbangSound = INVALID_SOUNDID;
}

void CPlayer::SetUpInventorySlotsAndCategories()
{
	//Use same config for now

	IInventory* pInventory = GetInventory();
	if (gEnv->bMultiplayer)
	{
		//Two slots for primary/secondary weapon
		pInventory->SetInventorySlotCapacity(IInventory::eInventorySlot_Weapon, g_pGameCVars->g_inventoryWeaponCapacity);
		pInventory->AssociateItemCategoryToSlot("primary", IInventory::eInventorySlot_Weapon);
		pInventory->AssociateItemCategoryToSlot("secondary", IInventory::eInventorySlot_Weapon);

		//Two slots for explosives grenades
		pInventory->SetInventorySlotCapacity(IInventory::eInventorySlot_Explosives, g_pGameCVars->g_inventoryExplosivesCapacity);
		pInventory->AssociateItemCategoryToSlot("grenade", IInventory::eInventorySlot_Explosives);
		pInventory->AssociateItemCategoryToSlot("explosive", IInventory::eInventorySlot_Explosives);
	}
	else
	{
		pInventory->SetInventorySlotCapacity(IInventory::eInventorySlot_Weapon, g_pGameCVars->g_inventoryWeaponCapacity);
		pInventory->AssociateItemCategoryToSlot("primary", IInventory::eInventorySlot_Weapon);
		pInventory->AssociateItemCategoryToSlot("secondary", IInventory::eInventorySlot_Weapon);

		pInventory->SetInventorySlotCapacity(IInventory::eInventorySlot_Explosives, g_pGameCVars->g_inventoryExplosivesCapacity);
		pInventory->AssociateItemCategoryToSlot("explosive", IInventory::eInventorySlot_Explosives);

		//Maybe we don't need this one?
		pInventory->SetInventorySlotCapacity(IInventory::eInventorySlot_Grenades, g_pGameCVars->g_inventoryGrenadesCapacity);
		pInventory->AssociateItemCategoryToSlot("grenade", IInventory::eInventorySlot_Grenades);
	}

}

void CPlayer::ApplyGameRulesSpeedMultiplier( float mult )
{
	m_gameRulesSpeedMult *= mult;
}

bool CPlayer::DoSTAPAiming() const
{
	return (g_pGameCVars->g_stapEnable != 0) && !IsThirdPerson() && (m_animationProxy.CanMixUpperBody() || IsAboutToFinishSlideTransition(0.25f));

}

void CPlayer::UpdateFPAiming()
{
	EntityId currentItem = GetCurrentItemId(true);
	CItem * pItem = GetItem(currentItem);
	CWeapon* pWeapon = pItem ? static_cast<CWeapon *>(pItem->GetIWeapon()) : 0;
	bool enableWeaponAim = false;
	bool torsoAimIK	= DoSTAPAiming();


	ICharacterInstance * pCharacter = GetEntity()->GetCharacter(0);

	CRY_ASSERT(pCharacter);

	if (pCharacter == 0)
	{
		return;
	}

	m_weaponParams.skelAnim = pCharacter->GetISkeletonAnim();
	m_weaponParams.characterInst = pCharacter;

	bool releaseCameraBone = false;
	bool updateFPAiming = (GetStance() != STANCE_NULL) && pWeapon && torsoAimIK;
	if (updateFPAiming)
	{
		enableWeaponAim = pWeapon->UpdateAimAnims(m_weaponParams, releaseCameraBone);
	}

	//const float XPOS = 200.0f;
	//const float YPOS = 60.0f;
	//const float FONT_SIZE = 2.0f;
	//const float FONT_COLOUR[4] = {1,1,1,1};
	//gEnv->pRenderer->Draw2dLabel(XPOS, YPOS, FONT_SIZE, FONT_COLOUR, false, "UpdateFPAiming: %s w:%s", enableWeaponAim ? "update" : "dont update", pWeapon ? "Armed" : "UnArmed");

	if (enableWeaponAim)
	{
		SMovementState curMovementState;
		m_pMovementController->GetMovementState(curMovementState);

		m_torsoAimIK.SetCameraBonePinning(!releaseCameraBone);
		m_torsoAimIK.Enable();

		m_weaponParams.isZoomed = pWeapon->IsZoomed() || pWeapon->IsZoomingIn();
		m_weaponParams.isHeavyWeapon = HasHeavyWeaponEquipped(pItem);
		m_weaponParams.aimDirection = curMovementState.aimDirection;
		m_weaponParams.isOnGround = !GetActorStats()->inAir;
		m_weaponParams.velocity	= curMovementState.movementDirection * curMovementState.desiredSpeed;
		m_weaponParams.velocity.z = m_stats.velocity.z;
		m_weaponParams.position   = curMovementState.pos;
		m_weaponParams.isSprinting = m_stats.bSprinting && !pWeapon->IsReloading();
		m_weaponParams.groundDistance = fabs_tpl(gEnv->p3DEngine->GetTerrainElevation(curMovementState.pos.x, curMovementState.pos.y) - curMovementState.pos.z);
		m_weaponParams.superJump = (!gEnv->bMultiplayer || !m_weaponParams.isZoomed) && IsSuperJumping() && !IsAirFrictionActive();
		m_weaponParams.blendBasePose = pWeapon->GetParams().blendBasePose;
		m_weaponParams.runToSprintBlendTime = pWeapon->GetParams().runToSprintBlendTime;
		m_weaponParams.sprintToRunBlendTime = pWeapon->GetParams().sprintToRunBlendTime;
	}
	else
	{
		m_torsoAimIK.Disable();
	}

	if (m_bSwimming)	//Avoid IsSwimming() internally due to virtual call penalty
	{
		m_torsoAimIK.Enable();
	}

	m_weaponFPAiming.SetActive(enableWeaponAim);
	m_weaponFPAiming.Update(m_weaponParams);
	
	if(pWeapon)
	{
		float runFactor = m_weaponFPAiming.GetRunFactor();
		pWeapon->UpdateMovementIdles(m_stats.bSprinting, runFactor);
	}
}

void CPlayer::SetSuperJumping( bool set )
{
	m_isSuperJumping = set;

	if (!m_isClient)
		return;

	CWeapon* pWeapon = GetWeapon(GetCurrentItemId());
	if (pWeapon)
	{
		pWeapon->OnPlayerSuperJumped(m_isSuperJumping);
	}
}


const Vec3 CPlayer::GetFPCameraPosition() const
{
	bool useCameraBone = IsSwimming();
	if (useCameraBone)
	{
		return GetEntity()->GetWorldTM().TransformPoint(GetCameraTran().t);
	}
	else if (!m_params.mountedWeaponCameraTarget.IsZero())
	{
		return m_params.mountedWeaponCameraTarget;
	}

	Vec3	fpCameraPosition = GetEntity()->GetWorldPos() + (m_baseQuat * (m_eyeOffset+ GetFPCameraOffset()));
	fpCameraPosition.z += m_bobOffset.z;

	return fpCameraPosition;
}

const Vec3 CPlayer::GetFPCameraOffset() const
{
	static Vec3 fpcameraOffset = ZERO;

	Vec3 frameOffset(0.0f, 0.0f, 0.0f);

	bool canOffset = m_linkStats.CanMoveCharacter() && !IsThirdPerson() && !m_stats.followCharacterHead && (!IsOnLedge()) && (!IsSliding());

	if (canOffset)
	{
		const SStanceInfo *stanceInfo = GetStanceInfo(m_stance);
		float lookDown = m_viewAngles.x * 2.0f / gf_PI;
		frameOffset.y += max(-lookDown,0.0f)*stanceInfo->viewDownYMod;
	}

	Interpolate(fpcameraOffset, frameOffset, 5.0f, gEnv->pTimer->GetFrameTime());

	return (fpcameraOffset);
}

void CPlayer::UpdateFPIKTorso(const Vec3& cameraPosition)
{
	bool shouldUpdate = IsClient() && !IsThirdPerson() && !m_stats.isRagDoll && (m_stance != STANCE_NULL);
	bool setAimIK = m_stats.animationControlled;

	SMovementState info;
	m_pMovementController->GetMovementState(info);

	if (shouldUpdate && !m_stats.animationControlled)
	{
		ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
		ICharacterInstance* pCharacterShadow = GetShadowCharacter();

		Vec3 torsoOffset = GetFPTotalTorsoOffset();

		if (pCharacter != 0)
		{
		CIKTorsoAim_Helper::SIKTorsoParams IKParams(pCharacter, pCharacterShadow, info.aimDirection, torsoOffset, cameraPosition, ShouldUpdateTranslationPinning());
		m_torsoAimIK.Update(IKParams);
		}

		setAimIK = m_torsoAimIK.IsEnabled();
	}

	if (setAimIK)
	{
	Matrix34 invWorld = GetEntity()->GetSlotWorldTM(0).GetInverted();
	m_lastTorsoAimIKDir = invWorld.TransformVector(info.aimDirection);
	m_lastTorsoAimIKPos = invWorld.TransformPoint(cameraPosition);
	}
	else
	{
		m_lastTorsoAimIKDir.Set(0.0f, 0.0f, 0.0f);
		m_lastTorsoAimIKPos.Set(0.0f, 0.0f, 0.0f);
	}

	//const float XPOS = 200.0f;
	//const float YPOS = 80.0f;
	//const float FONT_SIZE = 2.0f;
	//const float FONT_COLOUR[4] = {1,1,1,1};
	//gEnv->pRenderer->Draw2dLabel(XPOS, YPOS, FONT_SIZE, FONT_COLOUR, false, "UpdateFPIKTorso: %s Pos: (%f, %f, %f)", shouldUpdate ? "update" : "dont update", m_lastTorsoAimIKPos.x, m_lastTorsoAimIKPos.y, m_lastTorsoAimIKPos.z);
}

const Vec3 CPlayer::GetFPTotalTorsoOffset() const
{
	Vec3 torsoOffset(ZERO);

	CItem *pItem = (CItem *)GetCurrentItem();
	if (pItem)
	{
		torsoOffset += pItem->GetParams().fp_offset;
	}
	return torsoOffset;
}

void CPlayer::HasJumped()
{
	m_jumpCounter = (m_jumpCounter + 1)&(JUMP_COUNTER_MAX - 1);
	CHANGED_NETWORK_STATE(this, ASPECT_INPUT_CLIENT);
}

uint8 CPlayer::GetJumpCounter() const
{
	return m_jumpCounter;
}

void CPlayer::SetJumpCounter(uint8 counter)
{
	if(!IsClient() && m_jumpCounter != counter)
	{
		CPlayerMovementController *pPMC = static_cast<CPlayerMovementController *>(GetMovementController());
		if(pPMC)
		{
			pPMC->StrengthJump(m_isSuperJumping);
		}

		m_jumpCounter = counter;

		CMovementRequest request;
		request.SetJump();
		GetMovementController()->RequestMovement(request);
	}
}

int CPlayer::GetRank(bool serialized)
{
	if(IsClient())
	{
		int newRank = CPlayerProgression::GetInstance()->GetData(EPP_Rank);
		if(serialized && m_rank != newRank)
		{
			CHANGED_NETWORK_STATE(this, CPlayer::ASPECT_RANK_CLIENT);
		}
		m_rank = newRank;
	}
	return m_rank;
}

bool CPlayer::IsHeadShot(const HitInfo &hitInfo) const
{
	CRY_ASSERT(m_pBodyDamage);

	if (m_pBodyDamage)
	{
		return ((m_pBodyDamage->GetPartFlags(hitInfo) & CBodyDamage::PID_Headshot) == CBodyDamage::PID_Headshot);
	}

	return false;
}

float CPlayer::GetCameraAnimationFactor() const
{
	CRY_TODO(08, 03, 2010, "Implement an explicit camera factor in the weapon system & use it here, rather than the STAP pinning");
	ICharacterInstance *mainCharacter = GetEntity()->GetCharacter(0);
	float cameraFactor = g_pGameCVars->g_STAPCameraAnimation && !m_torsoAimIK.GetCameraBonePinning() ? 1.0f : 0.0f;
	const float animControlled = mainCharacter ? mainCharacter->GetISkeletonAnim()->GetUserData(eAGUD_AnimationControlledView) : 0.0f;
	cameraFactor = max(cameraFactor, animControlled);

	return cameraFactor;
}

void CPlayer::TriggerMeleeReaction()
{
	CWeapon* weapon = GetWeapon(GetCurrentItemId());
	if (weapon)
	{
		weapon->TriggerMeleeReaction();
	}

	SNanoSuitEvent suitEvent;
	suitEvent.event = eNanoSuitEvent_ACTION;
	suitEvent.sParam = "Heavy_MeleeReaction";
	SendActorSuitEvent(suitEvent);
}

void CPlayer::SetRemotePlayerRank(int newRank)
{
	if(m_rank != newRank && m_rank != -1 && newRank != -1)
	{
		SHUDEvent promotionEvent(eHUDEvent_OnPlayerPromotion);
		promotionEvent.ReserveData(2);
		promotionEvent.AddData( static_cast<int>(GetEntityId()) );
		promotionEvent.AddData( newRank );
		CHUD::CallEvent(promotionEvent);
	}
	m_rank = newRank;
}

void CPlayer::SetupAimIKProperties()
{
	ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
	if (pCharacter)
	{
		ISkeletonPose* pSkeleton = pCharacter->GetISkeletonPose();
		pSkeleton->SetAimIKFadeInSpeed(m_params.aimIKFadeDuration);
		pSkeleton->SetAimIKFadeOutSpeed(m_params.aimIKFadeDuration);
	}
}

void CPlayer::Reset( bool toGame )
{
	SendPerkEvent(EPE_Die);
	m_actorState = kActorState_initialized;
	CRY_TODO(16, 12, 2009, "[CG] Find out what else this needs to reset");
	if (m_pNavPath)
		m_pNavPath->Reset();

	SetupAimIKProperties();
}

//////////////////////////////////////////////////////////////////////////
bool CPlayer::ShouldUpdateTranslationPinning() const
{
	return (g_pGameCVars->g_translationPinningEnable != 0) && !IsSwimming();
}

//////////////////////////////////////////////////////////////////////////
const QuatT& CPlayer::GetAnimationRelativeMovement(int slot /* = 0 */) const
{
	ICharacterInstance* pUserCharacter = GetEntity()->GetCharacter(slot);
	ISkeletonAnim* pSkeletonAnimation = pUserCharacter ? pUserCharacter->GetISkeletonAnim() : NULL;

	if (pSkeletonAnimation)
	{
		return pSkeletonAnimation->GetRelMovement();
	}
	else
	{
		static QuatT defaultRelativeMovement(IDENTITY, ZERO);
		return defaultRelativeMovement;
	}
}

///////////////////////////////////////////////////////////////////////////
#if DEFERRED_RAY_CAST_BOTTOM
void CPlayer::OnDataReceived(const EventPhysRWIResult *pRWIResult)
{
	if (pRWIResult && pRWIResult->nHits > 0 && pRWIResult->pHits)
	{
		m_bottomLevel = pRWIResult->pHits[0].pt.z;
	}
	else
	{
		m_bottomLevel = BOTTOM_LEVEL_UNKNOWN;
	}
}

void CPlayer::OnDataReceived(const EventPhysPWIResult *pPWIResult)
{

}

void CPlayer::OnDRWReset()
{

}

void CPlayer::RayTestBottomLevel(const Vec3 & referencePos, float maxRelevantDepth)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_GAME);
	const float terrainWorldZ = gEnv->p3DEngine->GetTerrainElevation(referencePos.x, referencePos.y);
	int rayFlags = geom_colltype_player<<rwi_colltype_bit | rwi_stop_at_pierceable;
	int entityFlags = ent_terrain|ent_static|ent_sleeping_rigid|ent_rigid;
	const float padding = 0.2f;
	// NOTE: Terrain is above referencePos, so referencePos is probably inside a voxel or something.
	const float fPosWorldDiff = referencePos.z - terrainWorldZ;
	float rayLength = (float)__fsel(fPosWorldDiff, min(maxRelevantDepth, fPosWorldDiff), maxRelevantDepth) + (padding * 2.0f);
	m_raycastHelper->CastRay(referencePos + Vec3(0,0,padding), Vec3(0,0,-rayLength), entityFlags, rayFlags);
}
#endif
