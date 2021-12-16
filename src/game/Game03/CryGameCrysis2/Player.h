/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Implements the player.
  
 -------------------------------------------------------------------------
  History:
  - 29:9:2004: Created by Filippo De Luca

*************************************************************************/
#ifndef __PLAYER_H__
#define __PLAYER_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Actor.h"
#include <IActionMapManager.h>
#include "Network/NetActionSync.h"
#include "IDeferredRaycastManager.h"
#include "CoverAndLean.h"
#include "StealthKill.h"
#include "MindControl.h"
#include "Perk.h"
#include "SkeletonAlignedParticleCreator.h"
#include "PlayerPerkParticleInfo.h"
#include "IPerk.h"
#include "Audio/Announcer.h"
#include "PlayerPlugin_ScoreRewards.h"
#include "PlayerPlugin_CurrentlyTargetting.h"
#include "PlayerPlugin_Perk_SniperCountermeasures.h"
#include "PlayerModifiableValues.h"
#include "Effects/GameEffects/CCTVGameEffect.h"
#include "Effects/GameEffects/HitRecoilGameEffect.h"

#include "IKTorsoAim_Helper.h"
#include "LookAim_Helper.h"
#include "WeaponFPAiming.h"

#include "DualCharacterProxy.h"
#include "MountedGunController.h"
#include "InteractiveActionController.h"

#include "IPlayerInput.h"

class CPlayerMovement;
class CPlayerRotation;
struct IPlayerInput;
class CVehicleClient;
struct IDebugHistory;
struct IDebugHistoryManager;
struct IInteractor;
class CHitDeathReactions;
struct IPlayerUpdateListener;
class CPlayerNavPath;
class CBodyDamage;
class CPlayerCamera;
class CPlayerPlugin_Interaction;

#include <CryExtension/CrySharedPtr.h>
typedef cryshared_ptr<CHitDeathReactions> CHitDeathReactionsPtr;
typedef cryshared_ptr<CHitDeathReactions> CHitDeathReactionsConstPtr;
typedef cryshared_ptr<CBodyDamage> CBodyDamagePtr;
typedef cryshared_ptr<const CBodyDamage> CBodyDamageConstPtr;

#define ENABLE_PLAYER_HEALTH_REDUCTION_POPUPS		(1 && !defined(_RELEASE))
#define ENABLE_MP_SPAWNMODES 0
#define DEFERRED_RAY_CAST_BOTTOM 1

union USpectatorModeData
{
	struct SFixed
	{
		EntityId  location;
	}
	fixed;
	struct SFree
	{
		uint8  _dummy_;
	}
	free;
	struct SFollow
	{
		EntityId  spectatorTarget;	// which player we are following
	}
	follow;
	struct SCctv
	{
		struct { float x; float z; }  rotOffset;
		EntityId  currentCCTV;
		EntityId  entUnderCursor;
	}
	cctv;

	void Reset()
	{
		memset(this, 0, sizeof(*this));
	}
};

struct SSpectatorInfo
{
	USpectatorModeData	dataU;
	struct {
		EntityId  ent;
		float  time;
	}										cctvTagFx;  // the CCTV gfx needs to be active even when switched out of CCTV mode, so this data doesn't go into the modedata union
	uint8								mode;  // see CActor::EActorSpectatorMode enum
	uint8								state; // see CActor::EActorSpectatorState enum

	void Reset()
	{
		uint8  oldstate = state;
		memset(this, 0, sizeof(*this));
		state = oldstate;
	}

	EntityId* GetOtherEntIdPtrForCurMode()
	{
		EntityId*  ptr = NULL;
		switch (mode)
		{
			case CActor::eASM_Fixed:	ptr = &dataU.fixed.location; break;
			case CActor::eASM_Follow:	ptr = &dataU.follow.spectatorTarget; break;
			case CActor::eASM_CCTV:		ptr = &dataU.cctv.currentCCTV; break;
		}
		return ptr;
	}

	EntityId GetOtherEntIdForCurMode()
	{
		EntityId  e = 0;
		if (EntityId* ptr=GetOtherEntIdPtrForCurMode())
			e = (*ptr);
		return e;
	}

	void SetOtherEntIdForCurMode(EntityId e)
	{
		if (EntityId* ptr=GetOtherEntIdPtrForCurMode())
		{
			(*ptr) = e;
		}
	}
};

struct SPlayerStats : public SActorStats
{
	enum EOnLedgeTransition
	{
		eOLT_None = -1,
		eOLT_MidAir,
		eOLT_Falling,
		eOLT_MaxTransitions,
	};

	enum ESlidingState
	{
		eSS_None = 0,
		eSS_Sliding,
		eSS_ExitingSlide
	};

	float flashBangStunMult;
	float flashBangStunTimer;
	float flashBangStunLength;

	float inMovement;//for how long the player is moving
	float inRest;//for how long the player is standing still

	bool landed;
	bool jumped;
	bool wasFlying; 
	bool isAirFrictionActivated;
	float stuckTimeout;

	float jumpLock;
	
	float leanAmount;
	float peekOverAmount;

	// falling things
	float fallSpeed;
	float startFallingHeight;
	float downwardsImpactVelocity;
		
	bool isFiring;
	bool isThirdPerson;	
	bool hasRunIntoSomething;
	bool isInPickAndThrowMode;
	
	EntityId pickAndThrowEntity; // entity that is being hold in PickAndThrowMode (0 when is not in pickandthrow mode )

	struct SSlideStats
	{
		ESlidingState slidingState;
		float slideExitTime;
	}slideStats;
	
	CCoherentValue<uint8> followCharacterHead;
	bool isStandingUp;
	
	CCoherentValue<uint8> firstPersonBody;

	bool animationControlled;

	//cheating stuff
	uint8 flyMode;//0 no fly, 1 fly mode, 2 fly mode + noclip

	SSpectatorInfo spectatorInfo;

	EntityId deathCamSubject;

	Vec3 upVector;
	Vec3 groundNormal;

	//
	Ang3 angularVel;
	
	Ang3 angularImpulse;
	float angularImpulseDeceleration;
	float angularImpulseTime;
	float angularImpulseTimeMax;

	EntityId follow_target_id;
	Vec3 follow_target_dir;
	Vec3 snap_target_dir;

	Quat  playerRotation;

	//grabbing onto ledges
	struct SLedgeStats
	{
		bool	onLedge;
		EOnLedgeTransition onLedgeTransition;
		float ledgeSpeedMultiplier;
		float	lastTimeOnLedge;
		int nearestGrabbableLedgeIndex;
	} ledgeStats;

	// parameters to use while blending towards/from a ledge (also used to synch code with animation)
	struct SLedgeBlending
	{
		SLedgeBlending()
			:	m_runningTimer(0.0f),
			m_correctionTimer(0.0f),
			m_qtStartLocation(ZERO),
			m_qtTargetLocation(ZERO)
		{
		}

		float m_runningTimer;
		float m_correctionTimer;
		QuatT m_qtStartLocation;
		QuatT m_qtTargetLocation;
		QuatT m_qtCorrection;
	} ledgeBlending;

	bool	bIgnoreSprinting;

	//Force crouch
	float forceCrouchTime;

	SPlayerStats()
	{
		memset(this,0,sizeof(SPlayerStats)); // This will zero everything, fine.
		new (this) SActorStats(); // But this will set certain Vec3 to QNAN, due to the new default constructors.

		flashBangStunMult = 1.f;
		flashBangStunTimer = 0.f;
		flashBangStunLength = 0.f;

		mass = 80.0f;

		inAir = 1.0f;

		firstPersonBody = 2;

		upVector.Set(0,0,1);

		velocity.zero();
		velocityUnconstrained.zero();
		gravity.zero();
		forceUpVector.zero();
		forceLookVector.zero();

		ledgeStats.nearestGrabbableLedgeIndex = -1;
		ledgeStats.ledgeSpeedMultiplier = 1.0f;

		bSprinting = false;
		bIgnoreSprinting = false;
		isInPickAndThrowMode = false;
		
		pickAndThrowEntity = 0;

		hasRunIntoSomething = false;
		groundMaterialIdx = -1;
		
		fallSpeed = 0.0f;
		downwardsImpactVelocity = 0.0f;

		forceCrouchTime = 0;

		inWaterTimer = -1000.0f;
		relativeWaterLevel = 10.0f;
		worldWaterLevel = 0.0f;
		worldWaterLevelDelta = 0.0f;
		swimJumping = false;
		
		spectatorInfo.Reset();

		deathCamSubject = 0;

		startFallingHeight = 0.0f;
		
	}

	void Serialize( TSerialize ser, EEntityAspects aspects );

	void GetMemoryUsage( ICrySizer *pSizer ) const { /*nothing */}
};

// data structs used to collect parameters used for blending in ledge climbing
struct SLedgeBlendingParams
{
	SLedgeBlendingParams() : m_vPositionOffset(ZERO),	m_fMoveDuration(0.0f)	{}
	void SetParamsFromXml(const IItemParamsNode* pParams);

	float		m_fMoveDuration;
	float		m_fCorrectionDuration;
	Vec3	m_vPositionOffset;
};

struct SLedgeNearbyParams
{
	SLedgeNearbyParams() : m_vSearchDir(ZERO), m_fMaxDistance(1.2f), m_fMaxAngleDeviationFromSearchDirInDegrees(45.0f) {}
	void SetParamsFromXml(const IItemParamsNode* pParams);

	Vec3 m_vSearchDir;
	float m_fMaxDistance;
	float m_fMaxAngleDeviationFromSearchDirInDegrees;
};

struct SLedgeGrabbingParams
{
	SLedgeGrabbingParams() : m_fPowerSpeedUp(2.0f), m_fNormalSpeedUp(1.0f) {}
	void SetParamsFromXml(const IItemParamsNode* pParams);

	float m_fPowerSpeedUp;
	float m_fNormalSpeedUp;
	SLedgeNearbyParams m_ledgeNearbyParams;
	SLedgeBlendingParams m_grabTransitionsParams[SPlayerStats::eOLT_MaxTransitions];
};


struct SPlayerParams : public SActorParams
{
	float sprintMultiplier;
	float strafeMultiplier;
	float backwardMultiplier;

	float jumpHeight;

	float leanShift;
	float leanAngle;

	float aimIKFadeDuration;

	float proceduralLeaningFactor;

	//
	char animationAppendix[32];
	string footstepEffectName;
	bool footstepGearEffect;

	bool canUseComplexLookIK;
	string lookAtSimpleHeadBoneName;

	string leftArmAimPose;
	string rightArmAimPose;
	string bothArmsAimPose;

	SPlayerParams()
		: sprintMultiplier(1.25f)
		, strafeMultiplier(0.9f)
		, backwardMultiplier(0.9f)
		, jumpHeight(1.0f)
		, leanShift(0.5f)
		, leanAngle(11.0f)
		, aimIKFadeDuration(0.6f)
		, footstepGearEffect(false)
		, canUseComplexLookIK(true)
		, proceduralLeaningFactor(0.8f) // Crysis1 value
	{
		memset(animationAppendix, 0, sizeof(animationAppendix));
		strcpy(animationAppendix,"nw");
	}
};



struct SAimAccelerationParams
{
	SAimAccelerationParams();

	float angle_min;
	float angle_max;
};


struct SPlayerRotationParams
{
	enum EAimType
	{
		EAimType_NORMAL,
		EAimType_CROUCH,
		EAimType_SLIDING,
		EAimType_SPRINTING,
		EAimType_TOTAL
	};

	SAimAccelerationParams m_horizontalAims[EAimType_TOTAL];
	SAimAccelerationParams m_verticalAims[EAimType_TOTAL];

	void Reset(const IItemParamsNode* pRootNode);

	ILINE const SAimAccelerationParams& GetHorizontalAimParams(SPlayerRotationParams::EAimType aimType) const
	{
		CRY_ASSERT((aimType >= 0) && (aimType < EAimType_TOTAL));

		return m_horizontalAims[aimType];
	}

	ILINE const SAimAccelerationParams& GetVerticalAimParams(SPlayerRotationParams::EAimType aimType) const
	{
		CRY_ASSERT((aimType >= 0) && (aimType < EAimType_TOTAL));

		return m_verticalAims[aimType];
	}

private:
	void ReadAimParams(const IItemParamsNode* pRootNode, const char* aimTypeName, EAimType aimType);
	void ReadAccelerationParams(const IItemParamsNode* node, SAimAccelerationParams* output);
};


struct IPlayerEventListener
{
	enum ESpecialMove
	{
		eSM_Jump = 0,
		eSM_SpeedSprint,
		eSM_StrengthJump,
	};

	virtual void OnEnterVehicle(IActor* pActor, const char* strVehicleClassName, const char* strSeatName, bool bThirdPerson) {};
	virtual void OnExitVehicle(IActor* pActor) {};
	virtual void OnToggleThirdPerson(IActor* pActor, bool bThirdPerson) {};
	virtual void OnItemDropped(IActor* pActor, EntityId itemId) {};
	virtual void OnItemPickedUp(IActor* pActor, EntityId itemId) {};
	virtual void OnItemUsed(IActor* pActor, EntityId itemId) {};
	virtual void OnStanceChanged(IActor* pActor, EStance stance) {};
	virtual void OnSpecialMove(IActor* pActor, ESpecialMove move) {};
	virtual void OnDeath(IActor* pActor, bool bIsGod) {};
	virtual void OnObjectGrabbed(IActor* pActor, bool bIsGrab, EntityId objectId, bool bIsNPC, bool bIsTwoHanded) {};
	virtual void OnHealthChanged(IActor* pActor, float newHealth) {};
};


class CPlayer :
	public CActor, public CNanoSuit::INanoSuitListener
#if DEFERRED_RAY_CAST_BOTTOM
	, public IDeferredRaycastReceiver
#endif
{
	friend class CPlayerMovement;
	friend class CPlayerRotation;
	friend class CPlayerInput;
	friend class CAIInput;
	friend class CNetPlayerInput;
#if ENABLE_MINDCONTROL
	friend class CMindControlMaster;
	friend class CMindControlSlave;
#endif

public:

	enum EActionSoundParamValues
	{
		EActionSoundParam_RunWalkIdle = 0,
		EActionSoundParam_SwimOnWater,
		EActionSoundParam_Underwater,
		EActionSoundParam_Jump,
		EActionSoundParam_Land,
		EActionSoundParam_HeavyLand,
		EActionSoundParam_LedgeGrab,
		EActionSoundParam_Sliding,
		EActionSoundParam_Reload,
		EActionSoundParam_Pain
	};

	enum EPlayerSounds //most of these sounds are supposed to be played, when the nanosuit is not in use
	{
		ESound_Player_First,
		ESound_Run,
		ESound_StopRun,
		ESound_Jump,
		ESound_Fall_Drop,
		ESound_Melee,
		ESound_Fear,
		ESound_Hit_Wall,
		ESound_Breathing,
		ESound_Breathing_Water,
		ESound_Gear_Water,
		ESound_Drowning,
		ESound_DiveIn,
		ESound_DiveOut,
		ESound_Underwater,
		ESound_Player_Last
	};

	enum EClientPostEffect //first person screen effects for the client player
	{
		EEffect_ChromaShift = 1,
		EEffect_AlienInterference,
		EEffect_WaterDroplets
	};

	typedef uint8 EActionFlags;
	enum
	{
		eAF_NONE				= (0),
		eAF_JUMP_QUICK	= (1<<0),
	};

#define PlayerInteractionTypeList(f)   \
	f(eInteraction_None)                 \
	f(eInteraction_PickupItem)           \
	f(eInteraction_ExchangeItem)         \
	f(eInteraction_PickupAmmo)           \
	f(eInteraction_Use)                  \
	f(eInteraction_Grab)                 \
	f(eInteraction_GrabEnemy)            \
	f(eInteraction_Stealthkill)          \
	f(eInteraction_LetGo)                \
	f(eInteraction_StopUsing)            \

#if ENABLE_MP_SPAWNMODES
	enum ESpawnModeType
	{
		EMSG_NONE=0,
		ESMT_HQ,
		ESMT_Frontline,

		ESMT_NumSpawnModes,
	};

	static const NetworkAspectType ASPECT_SPAWNMODE_TYPE = eEA_GameClientStatic;
#endif

	static const NetworkAspectType ASPECT_NANO_SUIT_ENERGY			= eEA_GameServerDynamic;
	static const NetworkAspectType ASPECT_NANO_SUIT_INVULNERABLE= eEA_GameServerDynamic;
	static const NetworkAspectType ASPECT_NANO_SUIT_DEFENSE_HIT = eEA_GameServerDynamic;

	static const NetworkAspectType ASPECT_HEALTH				= eEA_GameServerStatic;
	static const NetworkAspectType ASPECT_FROZEN				= eEA_GameServerStatic;
	static const NetworkAspectType ASPECT_SPECTATOR			= eEA_GameServerC;

	static const NetworkAspectType ASPECT_INPUT_CLIENT											= eEA_GameClientA;
	static const NetworkAspectType ASPECT_NANO_SUIT_SETTING									= ASPECT_INPUT_CLIENT; // needs to be the same as ASPECT_INPUT_CLIENT, so that both get serialized together

	static const NetworkAspectType ASPECT_CURRENTLYTARGETTING_CLIENT				= eEA_GameClientB;
	static const NetworkAspectType ASPECT_PERK_SNIPERCOUNTERMEASURES_CLIENT = eEA_GameClientB;

	static const NetworkAspectType ASPECT_CURRENT_ITEM											= eEA_GameClientC;
	
	static const NetworkAspectType ASPECT_PLAYERSTATS_SERVER								= eEA_GameServerA;

	static const NetworkAspectType ASPECT_PERK_STAMP_CLIENT									= eEA_GameClientC;
	static const NetworkAspectType ASPECT_PERK_STRIKES_CLIENT								= eEA_GameClientC;
	static const NetworkAspectType ASPECT_PERK_SELFDESTRUCT_CLIENT					= eEA_GameClientC;
	static const NetworkAspectType ASPECT_PERK_TEAMPERK_CLIENT							= eEA_GameClientC;

	static const NetworkAspectType ASPECT_RANK_CLIENT												= eEA_GameClientC;

	static const NetworkAspectType ASPECT_PERK_SELFDESTRUCT_SERVER					= eEA_GameServerB;
	static const NetworkAspectType ASPECT_PERK_MISSILECOUNTERMEASURES_SERVER= eEA_GameServerB;

	static const NetworkAspectType ASPECT_PERK_SETTINGS_CLIENT							= eEA_GameClientC;
	static const NetworkAspectType ASPECT_VEHICLEVIEWDIR_CLIENT						= eEA_GameClientD;
	
	CPlayer();
	virtual ~CPlayer();

	virtual void Draw(bool draw);
	virtual void BindInputs( IAnimationGraphState * pAGState );
	//marcok: GC hack
	virtual bool ShouldSwim() const;
	virtual bool IsSwimming() const {	return m_bSwimming; }
	virtual bool IsSprinting();
	virtual bool CanFire();
	virtual bool Init( IGameObject * pGameObject );
	virtual void PostInit( IGameObject * pGameObject );
	void ReloadClientXmlData();
	virtual void InitClient( int channelId );
	virtual void InitLocalPlayer();
	virtual void ProcessEvent(SEntityEvent& event);
	virtual void SetAuthority( bool auth );
	virtual void SerializeXML( XmlNodeRef& node, bool bLoading );
	virtual void Update(SEntityUpdateContext& ctx, int updateSlot);
	virtual void PrePhysicsUpdate();
	virtual void UpdateView(SViewParams &viewParams);
	virtual void PostUpdateView(SViewParams &viewParams);
	void OnFootStepImpulseAnimEvent(ICharacterInstance* pCharacter, const AnimEventInstance &event);
	virtual void GetMemoryUsage(ICrySizer * s) const;
	void GetInternalMemoryUsage(ICrySizer * s) const;
	virtual void OnFootStepAnimEvent(ICharacterInstance* pCharacter, const char* boneName);
	void OnSwimmingStrokeAnimEvent();
	void ExecuteFootStep(ICharacterInstance* pCharacter, const float frameTime, const int32 nFootJointID);
	bool ShouldUpdateNextFootStep() const;
	void ExecuteFootStepsAIStimulus(const float relativeSpeed, const float noiseSupression);
	
	virtual void Physicalize(EStance stance=STANCE_NULL);

	virtual void SetActorModel();

	virtual void SetChannelId(uint16 id);

	virtual IEntity *LinkToVehicle(EntityId vehicleId);
	virtual IEntity *LinkToEntity(EntityId entityId, bool bKeepTransformOnDetach=true);
	virtual void LinkToMountedWeapon(EntityId weaponId);
	
	virtual void StartInteractiveAction(EntityId entityId);
	virtual void StartInteractiveActionByName(const char* interaction);
	virtual void EndInteractiveAction(EntityId entityId);

	void EquipAttachmentsOnCarriedWeapons();

	void ResetFPWeapon();
	virtual float GetReloadSpeedScale(void) const;

	virtual void SetViewInVehicle(Quat viewRotation);
	virtual Vec3 GetVehicleViewDir() const { return m_vehicleViewDir; }

	virtual void SupressViewBlending() { m_viewBlending = false; };

	ILINE Vec3 GetLastRequestedVelocity() const { return m_lastRequestedVelocity; }
	const QuatT& GetAnimationRelativeMovement(int slot = 0) const;
	void SetDeathTimer() { m_fDeathTime = gEnv->pTimer->GetFrameStartTime().GetSeconds(); }
	float GetDeathTime() const { return m_fDeathTime; }

	bool GetForcedLookDir(Vec3& vDir) const;
	void SetForcedLookDir(const Vec3& vDir);
	void ClearForcedLookDir();

	bool CanMove() const;

	void SufferingHighLatency(bool highLatency);

	static  const char* GetActorClassType() { return "CPlayer"; }
	virtual const char* GetActorClass() const { return CPlayer::GetActorClassType(); }
	virtual EntityId	GetGrabbedEntityId() const { return GetPickAndThrowEntity(); }

	IInteractor* GetInteractor();

	virtual void UpdateMountedGunController(bool forceIKUpdate);

	AUTOENUM_BUILDENUMWITHTYPE(EInteractionType, PlayerInteractionTypeList);

	struct SInteractionInfo
	{
		SInteractionInfo()
			: interactiveEntityId(0)
			, interactionType(eInteraction_None)
		{

		}

		void Reset()
		{
			interactiveEntityId = 0;
			interactionType = eInteraction_None;
		}

		EntityId interactiveEntityId;
		EInteractionType interactionType;
	};

	ILINE const SInteractionInfo& GetCurrentInteractionInfo() const { return s_clientInteractionInfo; }

	void SetSuperJumping(bool set);
	virtual	bool IsSuperJumping() const { return m_isSuperJumping; }

protected:

	void UpdateInteractionType();
	EInteractionType GetInteractionTypeForEntity(EntityId entityId);
	EInteractionType GetInteractionTypeWithItem(CItem* pItem);
	bool HasObjectGrabHelper(EntityId entityId) const;

	virtual IActorMovementController * CreateMovementController();
	virtual void SetIK( const SActorFrameMovementParams& );
	void ResetPerksVariables();
	virtual void UpdatePerks(SEntityUpdateContext& ctx);
	virtual void UpdateIPerks(const float dt);
	void LeaveAllPlayerPlugins();
	void UpdateActivePerkInstances(const uint64 previousPerkFlags, const uint64 newPerkFlags);
	virtual void UpdatePerkSonarVision(SEntityUpdateContext& ctx);
	virtual void UpdatePerkSolarBoost(SEntityUpdateContext& ctx);

	bool DoSTAPAiming() const;
	void UpdateFPAiming();
	void UpdateFPIKTorso(const Vec3& cameraPosition);
	bool ShouldUpdateTranslationPinning() const;
	
	const Vec3  GetFPCameraOffset() const;
	const Vec3	GetFPTotalTorsoOffset() const;
	virtual void OnChangeTeam() {};
	void PhysicalizeBodyDamage();
public:

	const Vec3  GetFPCameraPosition() const;

	// Made public, so that a plug-in class can tell the player to enter or leave it
	void EnterPlayerPlugin(CPlayerPlugin * pluginClass);
	void LeavePlayerPlugin(CPlayerPlugin * pluginClass);

	struct EMPParams
	{
		EMPParams(): time(1.0f) {};
		EMPParams(float _time): time(_time) {};

		float time;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("time", time);
		}
	};

	struct AnimGraphTransitionParams
	{
		AnimGraphTransitionParams() {};
		AnimGraphTransitionParams(const char *_name): name(_name) {};

		string name;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("name", name);
		}
	};

	struct AnimGraphInputParams
	{
		AnimGraphInputParams() {};
		AnimGraphInputParams(int _id, const char *_value): id(_id), svalue(_value), typ(1) {};
		AnimGraphInputParams(int _id, int _value): id(_id), ivalue(_value), typ(0) {};

		int id;
		int typ;
		string svalue;
		int ivalue;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("id", id, 'ui8');
			ser.Value("type", typ, 'ui3');
			if (typ==0)
				ser.Value("value", ivalue, 'i16');
			else
				ser.Value("value", svalue);
		}
	};

	struct SetPerksParams
	{
		SetPerksParams(): perkBitField(0) {};
		SetPerksParams(uint64 _perkBitField): perkBitField(_perkBitField) {};

		uint64 perkBitField;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("perk", perkBitField); // TODO: Look at splitting into 2 ui32s or something as uint64 is bad.
		}
	};

	DECLARE_CLIENT_RMI_NOATTACH(ClAnimGraphTransition, AnimGraphTransitionParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClAnimGraphInput, AnimGraphInputParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClEMP, EMPParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClSetThesePerks, SetPerksParams, eNRT_ReliableUnordered);

	//set/get actor status
	virtual void SetStats(SmartScriptTable &rTable);
	virtual void UpdateScriptStats(SmartScriptTable &rTable);
	virtual void UpdateStats(float frameTime);
	void UpdateSwimStats(float frameTime);
	void UpdateBreathing(float frameTime);
	virtual void UpdateWeaponRaising();
	virtual void SetParams(SmartScriptTable &rTable,bool resetFirst);
	virtual bool GetParams(SmartScriptTable &rTable);

	virtual float CalculatePseudoSpeed(bool wantSprint, float speedOverride = -1.0f) const;

	// Accessed via function to allow game based modifiers to stance speed without multiplying the number of stances.
	virtual float GetStanceMaxSpeed(EStance stance) const;

	virtual void ToggleThirdPerson();

	virtual int  IsGod();
	
	virtual void Revive( EReasonForRevive reasonForRevive );
	virtual void Kill();
  virtual void Reset(bool toGame);

	//stances
	Vec3 GetStanceViewOffset(EStance stance,float *pLeanAmt=NULL,bool withY = false) const;
	virtual bool IsThirdPerson() const;
	virtual void OnStanceChanged(EStance newStance, EStance oldStance);

	//reset function clearing state and animations for teleported actor
	virtual void OnTeleported();

	virtual void ResetAnimGraph();
	virtual void NotifyAnimGraphTransition(const char *name);
	virtual void NotifyAnimGraphInput(int id, const char *value);
	virtual void NotifyAnimGraphInput(int id, int value);

	// Nanosuit access functions
	virtual CNanoSuit* GetNanoSuit() const { return m_pNanoSuit; }
	ILINE virtual bool HasNanoSuit() const { return (m_pNanoSuit)? true : false; }
	virtual void SendActorSuitEvent(const SNanoSuitEvent& event);
	virtual const SNanoSuitGameParameters& GetActorSuitGameParameters() const;

	//CNanoSuit::INanoSuitListener
	virtual void ModeChanged(ENanoSuitMode mode);
	virtual void OnModeSelected(ENanoSuitMode mode){}
	virtual void EnergyChanged(float energy){}
	virtual void OnSuitStateChanged(ENanoSuitState state);
	virtual void OnSuitPowerActivated(const bool activated) {}
	//~CNanoSuit::INanoSuitListener

	virtual void SetFlyMode(uint8 flyMode);
	virtual uint8 GetFlyMode() const { return m_stats.flyMode; };
	virtual bool IsStandingUp() const { return m_stats.isStandingUp; }

	virtual void SetSpectatorState(uint8 state) { m_stats.spectatorInfo.state = state; }
	virtual EActorSpectatorState GetSpectatorState() { return (EActorSpectatorState)m_stats.spectatorInfo.state; }

	virtual void SetSpectatorModeAndOtherEntId(uint8 mode, EntityId othEntId);

	virtual uint8 GetSpectatorMode() const { return m_stats.spectatorInfo.mode; };
	virtual void SetSpectatorTarget(EntityId targetId) { assert(m_stats.spectatorInfo.mode==eASM_Follow); m_stats.spectatorInfo.dataU.follow.spectatorTarget = targetId; };
	virtual EntityId GetSpectatorTarget() const { assert(m_stats.spectatorInfo.mode==eASM_Follow); return m_stats.spectatorInfo.dataU.follow.spectatorTarget; };
	virtual void SetSpectatorCCTVCam(EntityId camId) { assert(m_stats.spectatorInfo.mode==eASM_CCTV); m_stats.spectatorInfo.dataU.cctv.currentCCTV = camId; };
	virtual EntityId GetSpectatorCCTVCam() const { assert(m_stats.spectatorInfo.mode==eASM_CCTV); return m_stats.spectatorInfo.dataU.cctv.currentCCTV; };
	virtual EntityId GetSpectatorFixedLocation() const { assert(m_stats.spectatorInfo.mode==eASM_Fixed); return m_stats.spectatorInfo.dataU.fixed.location; };
	CCCTVGameEffect* GetCCTVScreenEffect() { return &m_cctvScreenEffect; }

	void MoveToSpectatorTargetPosition();

	virtual void SetDeathCamSubject(EntityId subjectId) { m_stats.deathCamSubject = subjectId; };

	virtual void SelectNextItem(int direction, bool keepHistory, const char *category);
	virtual void HolsterItem(bool holster);
	virtual void SelectLastItem(bool keepHistory, bool forceNext = false);
	virtual void SelectItemByName(const char *name, bool keepHistory);
	virtual void SelectItem(EntityId itemId, bool keepHistory);
	virtual bool ScheduleItemSwitch(EntityId itemId, bool keepHistory);

	virtual void RagDollize( bool fallAndPlay );
	virtual void HandleEvent( const SGameObjectEvent& event );

	virtual void UpdateAnimGraph(IAnimationGraphState * pState);
	virtual void PostUpdate(float frameTime);
	virtual void PostRemoteSpawn();

	virtual void AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event, const uint32 eventNameCRC);
	
	virtual void SetViewRotation( const Quat &rotation );
	virtual Quat GetViewRotation() const;
	virtual void EnableTimeDemo( bool bTimeDemo );

	virtual void AddViewAngleOffsetForFrame(const Ang3 &offset);
	virtual const Ang3& GetViewAngleOffset() const;

	virtual bool SetAspectProfile(EEntityAspects aspect, uint8 profile );

	virtual void FullSerialize( TSerialize ser );
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
	virtual void PostSerialize();
	//set/get actor params
	virtual void SetHealth( int health );
	virtual SActorStats *GetActorStats() { return &m_stats; };
	virtual const SActorStats *GetActorStats() const { return &m_stats; };
	virtual SActorParams *GetActorParams() { return &m_params; };
	virtual const SActorParams *GetActorParams() const { return &m_params; };
	virtual void PostPhysicalize();
	virtual void CameraShake(float angle,float shift,float duration,float frequency,Vec3 pos,int ID,const char* source="");
	virtual bool CreateCodeEvent(SmartScriptTable &rTable);
	ILINE virtual void VectorToLocal(Vec3 &v) {v = m_clientViewMatrix.GetInverted() * v;}
	ILINE virtual Matrix34 GetViewMatrix() const { return m_clientViewMatrix; }
	virtual void AddAngularImpulse(const Ang3 &angular,float deceleration, float duration);
	virtual void SetAngles(const Ang3 &angles);
	virtual Ang3 GetAngles();
	virtual void PlayAction(const char *action,const char *extension, bool looping=false);
	virtual void UpdateGrab(float frameTime);
	virtual void ProcessIKLegs(ICharacterInstance *pCharacter,float frameTime);
	virtual void Landed(float fallSpeed);

	virtual void UpdateClientHealthFX(float oldHealth);

	//Player can grab north koreans
	virtual int GetActorSpecies() const { return eGCT_HUMAN; }

	//Support sleep 
	virtual bool CanSleep() {return true;}
	virtual bool CanFall() const { return true; }

	virtual void DamageInfo(EntityId shooterID, EntityId weaponID, IEntityClass *pProjectileClass, float damage, const char *damageType, const Vec3 hitDirection);

	void GenerateCloakAwarenessParticles(int num);

	const Quat& GetBaseQuat() const { return m_baseQuat; }
	const Quat& GetViewQuat() const { return m_viewQuat; }
	const Quat& GetViewQuatFinal() const { return m_viewQuatFinal; }
	virtual void ResetAnimations();
	IPlayerInput* GetPlayerInput() const {return m_pPlayerInput.get();}
	ILINE CPlayerCamera	* GetPlayerCamera() const { return m_playerCamera; }

#if ENABLE_MINDCONTROL
	CMindControlMaster* GetMindControlMaster() const { return m_pMindControlMaster.get(); }
	CMindControlSlave* GetMindControlSlave() const { return m_pMindControlSlave.get(); }
	bool IsMindControlling() const { return m_pMindControlMaster.get() && m_pMindControlMaster->IsBusy(); }
	bool IsMindControlled() const { return m_pMindControlSlave.get() && m_pMindControlSlave->IsBusy(); }
#endif

	virtual void SwitchDemoModeSpectator(bool activate);
	bool IsTimeDemo() const { return m_timedemo; }

	void StopLoopingSounds();

	void RegisterPlayerEventListener	(IPlayerEventListener *pPlayerEventListener);
	void UnregisterPlayerEventListener(IPlayerEventListener *pPlayerEventListener);
	void RegisterPlayerSuitEventListener(CNanoSuit::INanoSuitListener *pNanoSuitListener);
	void UnregisterPlayerSuitEventListener(CNanoSuit::INanoSuitListener *pNanoSuitListener);
	void RegisterPlayerUpdateListener(IPlayerUpdateListener *pListener);
	void UnregisterPlayerUpdateListener(IPlayerUpdateListener *pListener);

	ILINE float GetSpeedLean() const { return m_fSpeedLean; }
	ILINE void SetSpeedLean(float speedLean) { m_fSpeedLean = speedLean; }
	
	// Crysis 2 feature prototypes
	void UpdateCrysis2Features(float frameTime);

	// Gliding
	void EnableAirFriction(bool enable);
	ILINE bool IsAirFrictionActive() const { return (m_stats.isAirFrictionActivated); }
	void UpdateAirFrictionState();

	// Sliding
	void SetSlideState(SPlayerStats::ESlidingState slideState);
	bool CanSlide() const;
	ILINE bool IsSliding() const { return (m_stats.slideStats.slidingState != SPlayerStats::eSS_None); }
	bool IsAboutToFinishSlideTransition(float timeThreshold) const;

	// Ledge Grabbing
	SPlayerStats::EOnLedgeTransition CanGrabOntoLedge();
	void GrabOntoLedge(bool grab, SPlayerStats::EOnLedgeTransition transition);
	ILINE bool IsOnLedge() const { return (m_stats.ledgeStats.onLedge); }

	bool CanSwitchItems() const;
	bool HasHeavyWeaponEquipped() const;
	bool HasHeavyWeaponEquipped(IItem * pCurrentItem) const;
	//~Crysis2 

	virtual bool UseItem(EntityId itemId);
	virtual bool PickUpItem(EntityId itemId, bool sound);
	virtual bool DropItem(EntityId itemId, float impulseScale=1.0f, bool slectNext=true, bool byDeath=false);
	virtual void NetKill(const KillParams &killParams);

	ILINE const Vec3& GetEyeOffset() const { return m_eyeOffset; }
	ILINE const Vec3& GetWeaponOffset() const { return m_weaponOffset; }

	void SpawnParticleEffect(const char* effectName, const Vec3& pos, const Vec3& dir);
	void PlaySound(EPlayerSounds sound, bool play = true, const char* paramName = NULL, float paramValue = 0.0f, const char* paramName2 = NULL, float paramValue2 = 0.0f);
	void PlayBreathingSound( EActionSoundParamValues actionSoundParam );
	virtual void SendMusicLogicEvent(EMusicLogicEvents event);

	//Pick Up Items (pos parameter is worldspace position of item to pick up)
	bool NeedToCrouch(const Vec3& pos);

	//First person fists/hands actions
	void EnterSwimming();
	void ExitSwimming();

	void ResetScreenFX();
	void ResetFPView();

	//Hit assistance
	bool HasHitAssistance();

	float GetLastDamageSeconds() { return m_lastTimeDamaged.GetSeconds(); }

	virtual EntityId GetCurrentTargetEntityId() const
	{
		return m_currentlyTargettingPlugin.GetCurrentTargetEntityId();
	}

	virtual const float GetCurrentTargetTime() const
	{
		return m_currentlyTargettingPlugin.GetCurrentTargetTime();
	}

	//Perks
	bool IsPerkActive(EPerks perk) const;
	ILINE bool IsPerkSet(EPerks perk) const { return m_perkChoice.IsPerkBitActive(PERKBIT(perk)); }
	bool SetPerkActive(EPerks perk, bool enable = true);
	void SendPerkEvent(EPlayerPlugInEvent, void * data = NULL);
	template <class T>
	const T GetPerkData(EPlayerPlugInData dataType)
	{
		for(int i = 0; i < m_numActivePlayerPlugins; i++)
		{
			const T* returnData = static_cast<const T*>(m_activePlayerPlugins[i]->GetData(dataType));
			if(returnData)
			{
				return *returnData;
			}
		}

		const T* defaultData = static_cast<const T*>(CPlayerPlugin::DefaultData(dataType));
		return *defaultData;
	}
	EPerks GetPerkFromSlot(EPerkCategory slot);
	void SonarVisionPerk_SetNumParticlesToCreate(int numParticles,EBoneEmitterLocation boneEmitterLocation);
	void SonarVisPerkRegisterGunFire(IEntityClass* pAmmoType);
	void SetPerksAllowed(bool a)													{		m_perkChoice.SetAllowed(a); }
	bool IsPerksAllowed()																	{		return m_perkChoice.IsAllowed(); }
	IPerk* GetPerkInstance(EPerks perk)										{		return m_perkInstances[perk]; }

	//Weapon movement modifiers
	float GetWeaponMovementFactor() const;
	float GetWeaponRotationFactor() const;

	void PerkRequestDamage(EPerks perk, float damage);

	void RegisterKill(IActor *pKilledActor, int hit_type);

	uint64 NetGetPerksBitfield() const;
	void NetSetPerksBitfield(uint64 newBitfield);
	void ClUpdatePerks(uint64 newBitfield);

#if ENABLE_MP_SPAWNMODES
	ILINE int GetSpawnModeType() const { return m_spawnmode_type; }
	bool SetSpawnModeType(ESpawnModeType newType);
#endif

	void ResetFrameAngleViewOffset() { m_viewAnglesOffset.Set(0, 0, 0); }

	struct SStagingParams
	{
		SStagingParams() : 
			bActive(false), bLocked(false), vLimitDir(ZERO), vLimitRangeH(0.0f), vLimitRangeV(0.0f), stance(STANCE_NULL)
		{
		}

		bool  bActive;
		bool  bLocked;
		Vec3  vLimitDir;
		float vLimitRangeH;
		float vLimitRangeV;
		EStance stance;
		void Serialize(TSerialize ser)
		{
			assert( ser.GetSerializationTarget() != eST_Network );
			ser.BeginGroup("SStagingParams");
			ser.Value("bActive", bActive);
			ser.Value("bLocked", bLocked);
			ser.Value("vLimitDir", vLimitDir);
			ser.Value("vLimitRangeH", vLimitRangeH);
			ser.Value("vLimitRangeV", vLimitRangeV);
			ser.EnumValue("stance", stance, STANCE_NULL, STANCE_LAST);
			ser.EndGroup();
		}
	};

	void StagePlayer(bool bStage, SStagingParams* pStagingParams = 0); 

	void NotifyObjectGrabbed(bool bIsGrab, EntityId objectId, bool bIsNPC, bool bIsTwoHanded = false); // called from OffHand.cpp. bIsGrab always true atm

	ILINE CPlayerNavPath* GetNavPath() { return m_pNavPath; }
	ILINE const CPlayerNavPath* GetNavPath() const { return m_pNavPath; }
	bool HasActiveNavPath() const;

	bool HasShadowCharacter() const;
	int  GetShadowCharacterSlot() const;
	ICharacterInstance *GetShadowCharacter() const;

	void UpdateVisibility();

	ILINE const CCoverAndLean& GetCoverAndLean() const {return m_coverAndLean;}
	ILINE CPlayerModifiableValues& GetModifiableValues() { return m_modifiableValues; }
	ILINE const CPlayerModifiableValues& GetModifiableValues() const { return m_modifiableValues; }

	void StartFlashbangEffects(float time);
	void StopFlashbangEffects();
	void UpdateFlashbangEffect(float frameTime);
	
	void StartTinnitus();
	void UpdateTinnitus(float frameTime);
	void StopTinnitus();

	void EnterStealthKill(int enemyEntityId) { m_stealthKill.Enter(enemyEntityId);}
	void EnterPickAndThrow( EntityId entityPicked );
	void ExitPickAndThrow();
	bool IsInPickAndThrowMode() const { return m_stats.isInPickAndThrowMode; }
	EntityId GetPickAndThrowEntity() const { return m_stats.pickAndThrowEntity; }

	void ApplyGameRulesSpeedMultiplier( float mult );

	CHitDeathReactionsPtr GetHitDeathReactions() { return m_pHitDeathReactions; } 
	CHitDeathReactionsConstPtr GetHitDeathReactions() const { return m_pHitDeathReactions; } 

	CBodyDamagePtr GetBodyDamage() { return m_pBodyDamage; } 
	CBodyDamageConstPtr GetBodyDamage() const { return m_pBodyDamage; } 
	
	int		GetRank(bool serialized = true);

	bool IsHeadShot(const HitInfo &hitInfo) const;

	float GetCameraAnimationFactor() const;
	void TriggerMeleeReaction();

private:

	void CreateInputClass(bool client);

	void OnCollision(EventPhysCollision *physCollision);
	void AnimationControlled(bool activate);
	bool ShouldUsePhysicsMovement();
	void InformHealthHasBeenReduced();
	void Debug();
	bool CanGrabEnemy( EntityId entityId );
	SPlayerRotationParams::EAimType GetCurrentAimType() const;

	tSoundID m_flashbangSound;

	bool m_bConcentration;

#if ENABLE_PLAYER_HEALTH_REDUCTION_POPUPS
	float	m_healthAtStartOfUpdate;
#endif

	typedef std::list<IPlayerEventListener *> TPlayerEventListeners;
	TPlayerEventListeners m_playerEventListeners;

	typedef std::list<IPlayerUpdateListener*> TPlayerUpdateListeners;
	TPlayerUpdateListeners m_playerUpdateListeners;

	//Health update
	float				m_healthAccumError;
	float				m_timeOfLastHealthSync;
	CTimeValue  m_lastTimeDamaged;

	void UpdateHealthRegeneration(int iHealth, float frameTime);
	float GetRegenerationAmount(float frameTime);
	void InitNanoSuit();
	void InitCCTVScreenEffect(const IItemParamsNode* pRootNode);

	// void Cover and lean
	void UpdateCrouchAndLeanReferencePoints();

	void ReadDataFromXML(bool isReloading = false);

	// Ledge Grabbing
	void UpdateLedgeState();
	void UpdateNearestGrabbableLedge();
	bool CanReachPlatform(const Vec3& ledgePosition, const Vec3& refPosition) const;
	SPlayerStats::EOnLedgeTransition GetBestTransitionToLedge(const Vec3& refPosition, int ledgeIdx) const;
	bool IsGrabbableLedgeNearby() const { return (m_stats.ledgeStats.nearestGrabbableLedgeIndex != -1); }
	bool IsCharacterMovingTowardsLedge(int ledgeIdx) const;
	bool IsLedgePointingTowardCharacter(int ledgeIdx) const;
	QuatT CalculateLedgeOffsetLocation( const Vec3& vPositionOffset ) const;
	static void StrikeTargetPosition(const int currentPoint, const int numberOfPoints, Vec3& targetPos);
	void StartLedgeBlending( const SLedgeBlendingParams &positionParams);


	bool CanInteract() const;

	void SetUpInventorySlotsAndCategories();

	void	HasJumped();
	uint8	GetJumpCounter() const;
	void	SetJumpCounter(uint8 counter);

	void	SetRemotePlayerRank(int newRank);
	void SetupAimIKProperties();

protected:

	CPlayerCamera	* m_playerCamera;

	Ang3 		m_viewAnglesOffset;	//used for recoil/zoom sway
	Ang3		m_headAngles;//head angles in local space, for bones rotation

	// bool IsMaterialBootable(int matId) const;
	ILINE bool IsMaterialBootable(int matId) const { return true; }
	bool IsSoundPlaying(EPlayerSounds sound) const { return m_sounds[sound].audioSignalPlayer.IsPlaying( GetEntity()->GetId() ); }

	CNanoSuit *m_pNanoSuit;

	Ang3		m_angleOffset;	// Used only by the view system. (retained state)
	Ang3	m_viewAngles;

	Quat	m_viewQuat;//view quaternion
	Quat	m_viewQuatFinal;//the final exact view quaternion (with recoil added)
	Quat	m_baseQuat;//base rotation quaternion, rotating on the Z axis

	Matrix34 m_clientViewMatrix;

	float		m_bobCycle;
	Vec3		m_bobOffset;

	Vec3		m_eyeOffset;	// View system - used to interpolate to goal eye offset
												//the offset from the entity origin to eyes, its not the real offset vector but its referenced to player view direction.

	Vec3		m_weaponOffset;

	float		m_viewRoll;

	Vec3		m_upVector;//using gravity boots (or when prone) is the normal of the surface where the player is standing on, otherwise is the default (0,0,1)

	// updated by PlayerMovement for tracking time based acceleration
	Vec3		m_velocity;
	Vec3		m_lastRequestedVelocity;
	Vec3    m_lastKnownPosition;

	Vec3		m_forcedLookDir;
	
	float   m_standUpTimer;

	SPlayerStats		m_stats;
	SPlayerParams		m_params;
	
	std::auto_ptr<IPlayerInput> m_pPlayerInput;

	CPlayerNavPath* m_pNavPath;

	// for foot/leg ik
	Vec3 m_feetWpos[2];
	float m_fLastEffectFootStepTime;

	// compatibility with old code: which actions are set
	int m_actions;

	EActionFlags m_actionFlags;

	// TODO: wish i could figure out a way of removing this
	// was the rotation forced this frame?
	bool m_forcedRotation;
	bool m_viewBlending;
	bool m_ignoreRecoil;
	bool m_timedemo;

	//Demo Mode
	bool m_bDemoModeSpectator;

	// animation graph input ids
	IAnimationGraph::InputID m_inputAction;
	IAnimationGraph::InputID m_inputItem;
	IAnimationGraph::InputID m_inputUsingLookIK;
	IAnimationGraph::InputID m_inputAiming;
	IAnimationGraph::InputID m_inputVehicleName;
	IAnimationGraph::InputID m_inputVehicleSeat;
	IAnimationGraph::InputID m_inputSuitMode;
	IAnimationGraph::InputID m_inputWaterLevel;
	IAnimationGraph::InputID m_varInputAimMode;
	
	// probably temporary, feel free to figure out better place
	Quat m_lastAnimContRot;
	bool m_bSwimming;
	float m_fSpeedLean;
	float m_fDeathTime;
	float m_stickySurfaceTimer;
	float m_gameRulesSpeedMult;

	bool		m_sufferingHighLatency;
	bool		m_localPerksSet;

	CVehicleClient* m_pVehicleClient;
	Vec3 m_vehicleViewDir;

	float		 m_sprintTimer;
	
	struct SSound
	{
		CAudioSignalPlayer audioSignalPlayer;
		bool isRepeated;
	
		SSound() : isRepeated(false)	{}
	};
	
	SSound m_sounds[ESound_Player_Last];
	CAudioSignalPlayer m_soundFootstepIndGear;
	int m_footstepCounter;
	float m_timeForBreath;

	bool										m_bHasAssistance;
	bool										m_isSuperJumping;
	IGameObjectExtension*		m_pVoiceListener;
	IInteractor*			m_pInteractor;
	IEntitySoundProxy*      m_pSoundProxy;

	SStagingParams m_stagingParams;

	static const int									k_maxActivePlayerPlugIns = CPerkChoice::k_maxPerks + 4;

	KillParams												m_lastKillParams;
	CryFixedArray<int,eMAX_BoneEmitters> m_sonarVisionNumParticlesNextFrame;
	CCoverAndLean											m_coverAndLean;
	CStealthKill											m_stealthKill;

#if ENABLE_MINDCONTROL 
	std::auto_ptr<CMindControlMaster>	m_pMindControlMaster;
	std::auto_ptr<CMindControlSlave>	m_pMindControlSlave;
#endif

	CPerkChoice												m_perkChoice;
	CPlayerPerkParticleInfo						m_perkParticleEmitterInfo;
	CSkeletonAlignedParticleCreator		m_bodyParticleCreator;
	IPerk*														m_perkInstances[ePerk_Last];
	int																m_numActivePlayerPlugins;

	CPlayerPlugin*																m_activePlayerPlugins[k_maxActivePlayerPlugIns];
	CPlayerPlugin_ScoreRewards										m_scoreRewardsPlugin;
	CPlayerPlugin_TriggerSniperCountermeasures		m_triggerSniperCountermeasuresPlugin;
	CPlayerPlugin_CurrentlyTargetting							m_currentlyTargettingPlugin;
	CPlayerPlugin_Interaction* m_pLocalPlayerInteractionPlugin;

	CPlayerModifiableValues                       m_modifiableValues;
	
	CMountedGunController		m_mountedGunController;
	CInteractiveActionController m_interactiveActionController;
	SPlayerRotationParams m_playerRotationParams;

	static SInteractionInfo s_clientInteractionInfo;

	SLedgeGrabbingParams m_ledgeGrabbingParams;

#if ENABLE_MP_SPAWNMODES
	int m_spawnmode_type;
#endif
	
	float										m_ragdollTime;

	CCCTVGameEffect			 m_cctvScreenEffect;
	CHitRecoilGameEffect m_hitRecoilGameEffect;

	// Network
	uint8 m_jumpCounter;

	int m_rank;
	CHitDeathReactionsPtr m_pHitDeathReactions;
	CBodyDamagePtr m_pBodyDamage;

	SSerializedPlayerInput	m_recvPlayerInput;

#if DEFERRED_RAY_CAST_BOTTOM
private:
	float m_bottomLevel;
	CDeferredRaycastHelper m_raycastHelper;
	virtual void OnDataReceived(const EventPhysRWIResult *pRWIResult);
	virtual void OnDataReceived(const EventPhysPWIResult *pPWIResult);
	virtual void OnDRWReset();
	virtual void RayTestBottomLevel(const Vec3 & referencePos, float maxRelevantDept);
#endif

public:
	IDebugHistoryManager* m_pDebugHistoryManager;
	void DebugGraph_AddValue(const char* id, float value) const;

	CAnimationProxyDualCharacter m_animationProxy;
	CAnimationProxyDualCharacterUpper m_animationProxyUpper;

	CIKTorsoAim_Helper	m_torsoAimIK;
	Vec3								m_lastTorsoAimIKDir;
	Vec3								m_lastTorsoAimIKPos;
	CLookAim_Helper	m_lookAim;
	CWeaponFPAiming		m_weaponFPAiming;
	SParams_WeaponFPAiming m_weaponParams;

};

void UpdatePlayerVisibility(IEntity* playerEntity, bool isThirdPerson, int shadowCharacterSlot = -1, bool forceDontRenderNearest = false);

#endif //__PLAYER_H__
