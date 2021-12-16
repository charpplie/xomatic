/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Simple Actor implementation
  
 -------------------------------------------------------------------------
  History:
  - 7:10:2004   14:46 : Created by Márcio Martins

*************************************************************************/
#ifndef __Actor_H__
#define __Actor_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IActorSystem.h>
#include <IActionMapManager.h>
#include <CoherentValue.h>
#include <IMusicSystem.h>

//#include "Game.h" // for stance enum
#include "IAgent.h" // for stance enum
#include "IAnimationGraph.h"
#include "IAnimatedCharacter.h"
#include "IMovementController.h"
#include "GrabHandler.h"
#include "WeaponAttachmentManager.h"
#include "AutoEnum.h"

#include "NanoSuit_v2.h"
#include "ActorDamageEffectController.h"
#include "IAIObject.h"
#include "ActorRef.h"
#include "ActorTelemetry.h"

struct SActorFrameMovementParams
{
	SActorFrameMovementParams() : 
		desiredVelocity(ZERO), 
		desiredLean(0.0f),
		desiredPeekOver(0.0f),
		deltaAngles(ZERO),
		lookTarget(ZERO),
		aimTarget(ZERO),
		lookIK(false), 
		aimIK(false), 
		jump(false),
		strengthJump(false),
		sprint(0.0f),
		stance(STANCE_NULL),
		allowStrafe(false)
	{
	}

	// desired velocity for this frame (meters/second) (in local space)
	Vec3 desiredVelocity;
	// desired lean
	float desiredLean;
	float desiredPeekOver;
	// desired turn
	Ang3 deltaAngles;
	Vec3 lookTarget;
	Vec3 aimTarget;
	// prediction
	SPredictedCharacterStates prediction;
	// look target
	bool lookIK;
	// aim target
	bool aimIK;
	bool jump;
	bool strengthJump;
	float sprint;
	// stance
	EStance stance;

	bool allowStrafe;
};

struct IActorMovementController : public IMovementController
{
	struct SStats
	{
		bool idle;
	};

	virtual void Reset() = 0;
	// return true if params was filled out, false if not
	virtual bool Update( float frameTime, SActorFrameMovementParams& params ) = 0;
	virtual bool GetStats(SStats& stats) = 0;
	virtual void PostUpdate( float frameTime ) = 0;
	virtual void Release() = 0;
	virtual void BindInputs( IAnimationGraphState * ) {}
	virtual void Serialize(TSerialize &ser) = 0;
	virtual void ApplyControllerAcceleration( float& rotX, float& rotZ, float dt ) {}

	virtual void GetMemoryUsage(ICrySizer *pSizer ) const  = 0;
};

//FIXME:not sure to put this here
static const int ZEROG_AREA_ID = PHYS_FOREIGN_ID_USER+1;

// Collision ray piercability to ignore leaves and other things
#define COLLISION_RAY_PIERCABILITY 10

enum EBonesID
{
	BONE_BIP01 = 0,
	BONE_SPINE,
	BONE_SPINE2,
	BONE_SPINE3,
	BONE_HEAD,
	BONE_EYE_R,
	BONE_EYE_L,
	BONE_WEAPON,
	BONE_WEAPON2,
	BONE_FOOT_R,
	BONE_FOOT_L,
	BONE_ARM_R,
	BONE_ARM_L,
	BONE_CALF_R,
	BONE_CALF_L,
	BONE_CAMERA,
	BONE_AUTOAIMTARGET,
	BONE_AUTOAIMTARGET_ALIEN,
	BONE_ALIEN_HEAD,
	BONE_ID_NUM
};

//For first person grabbing
enum EGrabbedCharacterType
{
	eGCT_UNKNOWN = 0,
	eGCT_HUMAN,
	eGCT_TROOPER,
	eGCT_SPOTTER,
	eGCT_HARVESTER,
	eGCT_ALIEN,
	eGCT_PINGER,
};

//represent the key status of the actor
#define ACTION_JUMP						(1<<0)
#define ACTION_CROUCH					(1<<1)
#define ACTION_SPRINT					(1<<2)
#define ACTION_LEANLEFT				(1<<3)
#define ACTION_LEANRIGHT			(1<<4)
#define ACTION_RELAXED				(1<<5)
#define ACTION_STEALTH				(1<<6)
#define ACTION_MOVE						(1<<7)
#define ACTION_USE						(1<<8)
#define ACTION_FIRE						(1<<9)

#if !defined(_RELEASE)
#define WATCH_ACTOR_STATE(format, ...) \
	do {                                 \
		g_pGameCVars && g_pGameCVars->g_displayDbgText_actorState && CryWatch("[ACTOR STATE] <%s> %s '%s' " format, IsClient() ? "Client" : "Non-client", GetEntity()->GetClass()->GetName(), GetEntity()->GetName(), __VA_ARGS__); \
	} while(0)
#else
#define WATCH_ACTOR_STATE(...) (void)(0)
#endif

struct IVehicle;
struct IInventory;
struct IInteractor;
struct IStatsTracker;

struct SActorParams
{
	float maxGrabMass;
	float maxGrabVolume;

	Vec3 mountedWeaponCameraTarget;

	float	viewFoVScale;
	float viewFoVScale_Aux;

	Vec3	vLimitDir;
	float	vLimitRangeH;
	float	vLimitRangeV;
	float vLimitRangeVUp;
	float vLimitRangeVDown;

	float	weaponBobbingMultiplier;
	float	weaponInertiaMultiplier;

	float speedMultiplier;
	float meeleHitRagdollImpulseScale;

	bool canDisablePlayerCloak;  //Prototype flag
	bool nanoSuitActive;

	float lookFOVRadians;
	float lookInVehicleFOVRadians;
	float aimFOVRadians;
	float maxLookAimAngleRadians;

	float fallNPlayStiffness_scale;

	SActorParams()
		: mountedWeaponCameraTarget(ZERO)
		, vLimitDir(ZERO)
		, vLimitRangeH(0.0f)
		, vLimitRangeV(0.0f)
		, vLimitRangeVUp(0.0f)
		, vLimitRangeVDown(0.0f)
		, viewFoVScale(1.0f)
		, viewFoVScale_Aux(1.0f)
		, weaponInertiaMultiplier(1.0f)
		, weaponBobbingMultiplier(1.0f)
		, speedMultiplier(1.0f)
		, canDisablePlayerCloak(false)
		, lookFOVRadians(gf_PI)
		, lookInVehicleFOVRadians(gf_PI)
		, aimFOVRadians(gf_PI)
		, maxLookAimAngleRadians(gf_PI)
		, meeleHitRagdollImpulseScale(1.0f)
		, fallNPlayStiffness_scale(1200)
	{

	}
};

struct SActorStats
{
	struct SItemExchangeStats
	{
		EntityId switchingToItemID;
		bool		 keepHistory;
	};

	float inAir;//double purpose, tells if the actor is in air and for how long
	float onGround;//double purpose, tells if the actor is on ground and for how long

	float inWaterTimer;
	float relativeWaterLevel;
	float relativeBottomDepth;
	float worldWaterLevelDelta;
	bool swimJumping;

	float headUnderWaterTimer;
	float worldWaterLevel;

	float speed;
	float speedFlat;
	Vec3	angVelocity;

	Vec3 velocity;
	Vec3 velocityUnconstrained;

	Vec3 gravity;

	int movementDir;
	float inFiring;
	float mass;
	Vec3 forceUpVector;
	Vec3 forceLookVector;

	float grabbedTimer;
	bool isGrabbed;
	bool isRagDoll;
	bool isInFallNPlay; // To signal when the actor is in the FallNPlay Animation Graph state

	bool bSprinting;

	CCoherentValue<bool> isHidden;

	EntityId mountedWeaponID;

	int groundMaterialIdx;

	SItemExchangeStats exchangeItemStats;

	SActorStats()
	{
		memset( this,0,sizeof(SActorStats) );
		bSprinting = false;
	}

};

struct SStanceInfo
{
	//dimensions
	float heightCollider;
	float heightPivot;

	bool useCapsule;

	Vec3 size;

	//view
	Vec3 viewOffset;
	Vec3 leanLeftViewOffset;
	Vec3 leanRightViewOffset;
	float viewDownYMod;

	Vec3 peekOverViewOffset;
	Vec3 peekOverWeaponOffset;

	//weapon
	Vec3 weaponOffset;
	Vec3 leanLeftWeaponOffset;
	Vec3 leanRightWeaponOffset;

	//movement
	float normalSpeed;
	float maxSpeed;

	//misc
	char name[32];

	Vec3 modelOffset;

	inline Vec3	GetViewOffsetWithLean(float lean, float peekOver) const
	{
		float peek = clamp(peekOver, 0.0f, 1.0f);
		Vec3 peekOffset = peek * (peekOverViewOffset - viewOffset);

		if (lean < -0.01f)
		{
			float a = clamp(-lean, 0.0f, 1.0f);
			return viewOffset + a * (leanLeftViewOffset - viewOffset) + peekOffset;
		}
		else if (lean > 0.01f)
		{
			float a = clamp(lean, 0.0f, 1.0f);
			return viewOffset + a * (leanRightViewOffset - viewOffset) + peekOffset;
		}
		return viewOffset + peekOffset;
	}

	inline Vec3	GetWeaponOffsetWithLean(float lean, float peekOver) const
	{
		float peek = clamp(peekOver, 0.0f, 1.0f);
		Vec3 peekOffset = peek * (peekOverWeaponOffset - weaponOffset);

		if (lean < -0.01f)
		{
			float a = clamp(-lean, 0.0f, 1.0f);
			return weaponOffset + a * (leanLeftWeaponOffset - weaponOffset) + peekOffset;
		}
		else if (lean > 0.01f)
		{
			float a = clamp(lean, 0.0f, 1.0f);
			return weaponOffset + a * (leanRightWeaponOffset - weaponOffset) + peekOffset;
		}
		return weaponOffset + peekOffset;
	}

	static inline Vec3 GetOffsetWithLean(float lean, float peekOver, const Vec3& sOffset, const Vec3& sLeftLean, const Vec3& sRightLean, const Vec3& sPeekOffset)
	{
		float peek = clamp(peekOver, 0.0f, 1.0f);
		Vec3 peekOffset = peek * (sPeekOffset - sOffset);

		if (lean < -0.01f)
		{
			float a = clamp(-lean, 0.0f, 1.0f);
			return sOffset + a * (sLeftLean - sOffset) + peekOffset;
		}
		else if (lean > 0.01f)
		{
			float a = clamp(lean, 0.0f, 1.0f);
			return sOffset + a * (sRightLean - sOffset) + peekOffset;
		}
		return sOffset + peekOffset;
	}


	// Returns the size of the stance including the collider and the ground location.
	ILINE AABB	GetStanceBounds() const
	{
		AABB	aabb(-size, size);
		// Compensate for capsules.
		if(useCapsule)
		{
			aabb.min.z -= max(size.x, size.y);
			aabb.max.z += max(size.x, size.y);
		}
		// Lift from ground.
		aabb.min.z += heightCollider;
		aabb.max.z += heightCollider;
		// Include ground position
		aabb.Add(Vec3(0,0,0));

		// Make relative to the entity
		aabb.min.z -= heightPivot;
		aabb.max.z -= heightPivot;
		return aabb;
	}

	// Returns the size of the collider of the stance.
	inline AABB	GetColliderBounds() const
	{
		AABB	aabb(-size, size);
		// Compensate for capsules.
		if(useCapsule)
		{
			aabb.min.z -= max(size.x, size.y);
			aabb.max.z += max(size.x, size.y);
		}
		// Lift from ground.
		aabb.min.z += heightCollider;
		aabb.max.z += heightCollider;
		// Make relative to the entity
		aabb.min.z -= heightPivot;
		aabb.max.z -= heightPivot;
		return aabb;
	}

	SStanceInfo()
		:	peekOverWeaponOffset(ZERO)
		, peekOverViewOffset(ZERO)
	{
		heightCollider = 0;
		heightPivot = 0;

		useCapsule = false;

		size.Set(0.5f,0.5f,0.5f);
		modelOffset.Set(0,0,0);
		viewOffset.Set(0,0,0);
		leanLeftViewOffset.Set(0,0,0);
		leanRightViewOffset.Set(0,0,0);
		weaponOffset.Set(0,0,0);
		leanLeftWeaponOffset.Set(0,0,0);
		leanRightWeaponOffset.Set(0,0,0);
		viewDownYMod = 0.0f;

		maxSpeed = 0.001f;

		strcpy(name,"null");
	}
};

#define IKLIMB_RIGHTHAND (1<<0)
#define IKLIMB_LEFTHAND (1<<1)

struct SIKLimb
{
	int flags;

	int rootBoneID;
	int endBoneID;
	int middleBoneID;//optional for custom IK

	Vec3 goalWPos;
	Vec3 currentWPos;

	Vec3 goalNormal;

	//limb local position in the animation
	Vec3 lAnimPos;
	Vec3 lAnimPosLast;

	char name[64];

	float recoverTime;//the time to go back to the animation pose
	float recoverTimeMax;

	float blendTime;
	float blendTimeMax;

	int blendID;

	int characterSlot;

	bool keepGoalPos;

	SIKLimb()
	{
		memset(this,0,sizeof(SIKLimb));
	}

	void SetLimb(int slot,const char *limbName,int rootID,int midID,int endID,int iFlags);
	void SetWPos(IEntity *pOwner,const Vec3 &pos,const Vec3 &normal,float blend,float recover,int requestID);
	void Update(IEntity *pOwner,float frameTime);

	void GetMemoryUsage(ICrySizer *pSizer) const{}
};

struct SLinkStats
{
	#define LINKED_FREELOOK (1<<0)
	#define LINKED_VEHICLE (1<<1)

	//which entity are we linked to?
	EntityId linkID;
	//
	uint32 flags;
	//when freelook is on, these are used to update the player rotation
	Quat	baseQuatLinked;
	Quat	viewQuatLinked;

	//
	SLinkStats() : linkID(0), flags(0), baseQuatLinked(IDENTITY), viewQuatLinked(IDENTITY)
	{
	}

	SLinkStats(EntityId eID,uint32 flgs = 0) : linkID(eID), flags(flgs), baseQuatLinked(IDENTITY), viewQuatLinked(IDENTITY)
	{
	}

	ILINE void UnLink()
	{
		linkID = 0;
		flags = 0;
	}

	ILINE bool CanRotate()
	{
		return (!linkID || flags & LINKED_FREELOOK);
	}

	ILINE bool CanMove()
	{
		return (!linkID);
	}

	ILINE bool CanDoIK()
	{
		return (!linkID);
	}

	ILINE bool CanMoveCharacter()
	{
		return (!linkID);
	}

	ILINE IEntity *GetLinked()
	{
		if (!linkID)
			return NULL;
		else
		{
			IEntity *pEnt = gEnv->pEntitySystem->GetEntity(linkID);
			//if the entity doesnt exist anymore forget about it.
			if (!pEnt)
				UnLink();

			return pEnt;
		}
	}

	IVehicle *GetLinkedVehicle();

	void Serialize( TSerialize ser );
};

struct SActorAnimationEvents
{
	SActorAnimationEvents()
		: m_initialized(false)
	{

	}

	void Init();

	uint32 m_soundId;
	uint32 m_plugginTriggerId;
	uint32 m_ragdollStartId;
	uint32 m_footStepId;
	uint32 m_swimmingStrokeId;
	uint32 m_stealthKillId;
	uint32 m_deathReactionEndId;
	uint32 m_footStepImpulseId;
	uint32 m_reactionOnCollision;

private:
	bool   m_initialized;
};

#define ReasonForReviveList(f)    \
	f(kRFR_FromInit)                \
	f(kRFR_StartSpectating)         \
	f(kRFR_Spawn)                   \
	f(kRFR_ScriptBind)              \

#define ActorStateList(f)             \
	f(kActorState_uninitialized)        \
	f(kActorState_initialized)          \
	f(kActorState_justJoined)           \
	f(kActorState_alive)                \
	f(kActorState_incapacitated)        \
	f(kActorState_dying)                \
	f(kActorState_watchingKillReplay)   \
	f(kActorState_inGameSpectating)     \
	f(kActorState_beingDestroyed)       \

class CItem;
class CWeapon;
union USpectatorModeData;

class CActor :
	public CGameObjectExtensionHelper<CActor, IActor, 25>,
	public IGameObjectView,
	public IGameObjectProfileManager
{
	friend class CStatsRecordingMgr;

public:

	struct ItemIdParam
	{
		ItemIdParam(): itemId(0), pickOnlyAmmo(false) {};
		ItemIdParam(EntityId item): itemId(item), pickOnlyAmmo(false) {};
		ItemIdParam(EntityId item, bool onlyAmmo): itemId(item), pickOnlyAmmo(onlyAmmo) {};
		void SerializeWith(TSerialize ser)
		{
			ser.Value("itemId", itemId, 'eid');
			ser.Value("pickOnlyAmmo", pickOnlyAmmo, 'bool');
		}
		EntityId itemId;
		bool pickOnlyAmmo;
	};

	struct DropItemParams
	{
		DropItemParams(): itemId(0), impulseScale(1.0f), selectNext(true), byDeath(false) {};
		DropItemParams(EntityId item, float scale, bool next=true, bool death=false): itemId(item), impulseScale(scale), selectNext(next), byDeath(death) {};
		
		void SerializeWith(TSerialize ser)
		{
			ser.Value("itemId", itemId, 'eid');
			ser.Value("impulseScale", impulseScale, 'iScl');
			ser.Value("selectNext", selectNext, 'bool');
			ser.Value("byDeath", byDeath, 'bool');
		}

		float impulseScale;
		EntityId itemId;
		bool selectNext;
		bool byDeath;
	};

	struct ReviveParams
	{
		ReviveParams(): spawnPointId(0), teamId(0), physCounter(0) {};
		ReviveParams(const EntityId id, int tId, uint8 counter): spawnPointId(id), teamId(tId), physCounter(counter) {};
		void SerializeWith(TSerialize ser)
		{
			ser.Value("spawnPointId", spawnPointId, 'eid');
			ser.Value("teamId", teamId, 'team');
			ser.Value("physCounter", physCounter, 'ui4');
		};

		EntityId spawnPointId;
		int	 teamId;
		uint8 physCounter;
	};

	struct KillParams
	{
		KillParams()
		: shooterId(0),
			projectileId(0),
			weaponClassId(0),
			damage(0.0f),
			material(0),
			hit_type(0),
			hit_joint(0),
			hit_part(0),
			impulse(ZERO),
			ragdoll(false)
		{};
		KillParams(EntityId _shooterId, EntityId _projectileId, int _weaponClassId, float _damage, int _material, int _hit_type, int _hit_joint, int _hit_part, const Vec3& _impulse, bool _ragdoll)
		: shooterId(_shooterId),
			projectileId(_projectileId),
			weaponClassId(_weaponClassId),
			damage(_damage),
			material(_material),
			hit_type(_hit_type),
			hit_joint(_hit_joint),
			hit_part(_hit_part),
			impulse(_impulse),
			ragdoll(_ragdoll)
		{};

		EntityId shooterId;
		EntityId projectileId;
		int weaponClassId;
		float damage;
		int material;
		int hit_type;
		uint16 hit_joint;
		uint16 hit_part;
		bool ragdoll;
		Vec3 impulse;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("shooterId", shooterId, 'eid');
			ser.Value("projectileId", projectileId, 'eid');
			ser.Value("weaponClassId", weaponClassId, 'ui16');
			ser.Value("damage", damage, 'dmg');
			ser.Value("material", material, 'mat');
			ser.Value("hit_type", hit_type, 'hTyp');
			ser.Value("hit_joint", hit_joint, 'ui16');
			ser.Value("impulse", impulse, 'kImp');
			ser.Value("ragdoll", ragdoll, 'bool');
		};
	};
	struct MoveParams
	{
		MoveParams() {};
		MoveParams(const Vec3 &p, const Quat &q): pos(p), rot(q) {};
		void SerializeWith(TSerialize ser)
		{
			ser.Value("pos", pos, 'wrld');
			ser.Value("rot", rot, 'ori1');
		}
		Vec3 pos;
		Quat rot;
	};
	struct AmmoParams
	{
		AmmoParams() {};
		AmmoParams(const char *name, int amount): ammo(name), count(amount) {};
		void SerializeWith(TSerialize ser)
		{
			ser.Value("ammo", ammo);
			ser.Value("amount", count);
		}
		string ammo;
		int	count;
	};

	struct PickItemParams
	{
		PickItemParams(): itemId(0), select(false), sound(false), pickOnlyAmmo(false) {};
		PickItemParams(EntityId item, bool slct, bool snd): itemId(item), select(slct), sound(snd), pickOnlyAmmo(false) {};
		PickItemParams(EntityId item, bool slct, bool snd, bool onlyAmmo): itemId(item), select(slct), sound(snd), pickOnlyAmmo(onlyAmmo) {};
		void SerializeWith(TSerialize ser)
		{
			ser.Value("itemId", itemId, 'eid');
			ser.Value("select", select, 'bool');
			ser.Value("sound", sound, 'bool');
			ser.Value("pickOnlyAmmo", pickOnlyAmmo, 'bool');
		}

		EntityId	itemId;
		bool			select;
		bool			sound;
		bool      pickOnlyAmmo;
	};

	struct NoParams
	{
		void SerializeWith(const TSerialize& ser) {};
	};

	struct KillCamData
	{
		static const int DATASIZE = 1000;
		int m_packetIndex;
		uint16 m_dataSize;
		uint16 m_numParts;
		uint8 m_data[DATASIZE];
		EntityId m_sendToId;

		KillCamData() {};
		KillCamData(EntityId sendToId, int packetIndex, int dataSize, int numparts, uint8 *data)
		{
			m_packetIndex = packetIndex;
			memcpy(m_data, data, DATASIZE);
			m_dataSize = dataSize;
			m_numParts = numparts;
			m_sendToId = sendToId;
		};

		void SerializeWith(TSerialize ser)
		{
			ser.Value("packetIndex", m_packetIndex);
			ser.Value("dataSize", m_dataSize);
			ser.Value("dataSize", m_numParts);
			ser.Value("sendToId", m_sendToId, 'eid');

			for (int i = 0; i < m_dataSize; i++)
			{
				char temp[255];
				sprintf(temp, "data%d", i);
				ser.Value(temp, m_data[i]);
			}
		};
	};

	AUTOENUM_BUILDENUMWITHTYPE(EReasonForRevive, ReasonForReviveList);
	AUTOENUM_BUILDENUMWITHTYPE(EActorState, ActorStateList);

	enum EActorSpectatorState
	{
		eASS_None = 0,				// Just joined - fixed spectating, no actor.
		eASS_Ingame,				// Ingame, dead ready to respawn, with actor.
		eASS_Spectating				// Has chosen to enter spectator mode, cannot spawn without leaving, no actor.
	};

	enum EActorSpectatorMode
	{
		eASM_None = 0,												// normal, non-spectating

		eASM_FirstMPMode,
		eASM_Fixed = eASM_FirstMPMode,				// fixed position camera
		eASM_Free,														// free roaming, no collisions
		eASM_Follow,													// follows an entity in 3rd person
		eASM_CCTV,														// spectate via designer-placed CCTV security camera entities
		eASM_LastMPMode = eASM_CCTV,

		eASM_Cutscene,												// HUDInterfaceEffects.cpp sets this
	};

	DECLARE_SERVER_RMI_NOATTACH(SvRequestDropItem, DropItemParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestPickUpItem, ItemIdParam, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestUseItem, ItemIdParam, eNRT_ReliableOrdered);
	// cannot be _FAST - see comment on InvokeRMIWithDependentObject
	DECLARE_CLIENT_RMI_NOATTACH(ClPickUp, PickItemParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClDrop, DropItemParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClStartUse, ItemIdParam, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClStopUse, ItemIdParam, eNRT_ReliableOrdered);

	//virtual void SendRevive(const Vec3& position, const Quat& orientation, int team, bool clearInventory);
	DECLARE_CLIENT_RMI_NOATTACH(ClRevive, ReviveParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClSimpleKill, NoParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClKill, KillParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClMoveTo, MoveParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClAddAmmo, AmmoParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvKillCamData, KillCamData, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClKillCamData, KillCamData, eNRT_ReliableUnordered);

	void ClientSendKillcamData(EntityId shooterId, EntityId victimId);
	void ServerSendKillcamData(IActor *sendto);
	void ServerSendKillcamDataPart(IActor *sendto, int partindex, size_t datasize, int numparts, uint8 *fpcamdataptr);

	CItem *GetItem(EntityId itemId) const;
	CItem *GetItemByClass(IEntityClass* pClass) const;
	CWeapon *GetWeapon(EntityId itemId) const;
	CWeapon *GetWeaponByClass(IEntityClass* pClass) const;

	virtual void SelectNextItem(int direction, bool keepHistory, const char *category=0);
	virtual void HolsterItem(bool holster);
	virtual void SelectLastItem(bool keepHistory, bool forceNext = false);
	virtual void SelectItemByName(const char *name, bool keepHistory);
	virtual void SelectItem(EntityId itemId, bool keepHistory);
	virtual bool ScheduleItemSwitch(EntityId itemId, bool keepHistory);

	virtual bool UseItem(EntityId itemId);
	virtual bool PickUpItem(EntityId itemId, bool sound);
	virtual bool DropItem(EntityId itemId, float impulseScale=1.0f, bool selectNext=true, bool byDeath=false);
	virtual void DropAttachedItems();
	bool PickUpItemAmmo(EntityId itemId);

	virtual void NetReviveAt(const Vec3 &pos, const Quat &rot, int teamId);
	virtual void NetReviveInVehicle(EntityId vehicleId, int seatId, int teamId);
	virtual void NetSimpleKill();
	virtual void NetKill(const KillParams &killParams);

	virtual void SetSleepTimer(float timer);
	Vec3 GetWeaponOffsetWithLean(EStance stance, float lean, float peekOver, const Vec3& eyeOffset);

	virtual bool CanRagDollize() const;
		
public:
	CActor();
	virtual ~CActor();

	// IActor
	virtual void ProcessEvent(SEntityEvent& event);
	virtual void Release() { delete this; };
	virtual void ResetAnimGraph();
	virtual void NotifyAnimGraphTransition(const char *anim0){};
	virtual void NotifyAnimGraphInput(int id, const char *value) {};
	virtual void NotifyAnimGraphInput(int id, int value) {};
	virtual void FullSerialize( TSerialize ser );
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
	virtual void PostSerialize();
	virtual void SerializeSpawnInfo( TSerialize ser );
	virtual ISerializableInfoPtr GetSpawnInfo();
	virtual void SetChannelId(uint16 id);
	virtual void  SerializeLevelToLevel( TSerialize &ser );
	virtual IInventory* GetInventory() const;

	virtual bool IsPlayer() const;
	virtual bool IsClient() const;
	virtual bool IsMigrating() const { return m_isMigrating; }
	virtual void SetMigrating(bool isMigrating) { m_isMigrating = isMigrating; }
	virtual IMaterial *GetReplacementMaterial() { return m_pReplacementMaterial; };

	virtual bool Init( IGameObject * pGameObject );
	virtual void InitClient( int channelId );
	virtual void PostInit( IGameObject * pGameObject );
	virtual void PostInitClient(int channelId) {};
	virtual void Update(SEntityUpdateContext& ctx, int updateSlot);
	virtual void UpdateView(SViewParams &viewParams) {};
	virtual void PostUpdateView(SViewParams &viewParams) {};

	void SetActorState(EActorState newState);

	virtual void InitLocalPlayer() {};

	virtual void SetIKPos(const char *pLimbName, const Vec3& goalPos, int priority);

	virtual void HandleEvent( const SGameObjectEvent& event );
	virtual void PostUpdate(float frameTime);
	virtual void PostRemoteSpawn() {};

	virtual bool IsThirdPerson() const { return true; };
  virtual void ToggleThirdPerson(){}


	virtual void RequestFacialExpression(const char* pExpressionName /* = NULL */);
	virtual void PrecacheFacialExpression(const char* pExpressionName);

	virtual void NotifyInventoryAmmoChange(IEntityClass* pAmmoClass, int amount);
	virtual EntityId	GetGrabbedEntityId() const { return 0; }

	virtual void HideAllAttachments(bool isHiding);

	virtual void OnAIProxyEnabled(bool enabled);
	// ~IActor

	// IGameObjectProfileManager
	virtual bool SetAspectProfile( EEntityAspects aspect, uint8 profile );
	virtual uint8 GetDefaultProfile( EEntityAspects aspect ) { return aspect == eEA_Physics? eAP_NotPhysicalized : 0; }
	// ~IGameObjectProfileManager

	// IActionListener
	virtual void OnAction(const ActionId& actionId, int activationMode, float value);
	// ~IActionListener

	virtual void ProfileChanged( uint8 newProfile );

	// Nanosuit access functions
	virtual CNanoSuit* GetNanoSuit() const { return NULL; }
	ILINE virtual bool HasNanoSuit() const { return false; }
	virtual void SendActorSuitEvent(const SNanoSuitEvent& event) {}
	virtual const SNanoSuitGameParameters& GetActorSuitGameParameters() const;

	//------------------------------------------------------------------------
	ILINE float GetAirControl() const { return m_airControl; };
	ILINE float GetAirResistance() const { return m_airResistance; };
	ILINE float GetInertia() const { return m_inertia; }
	ILINE float GetInertiaAccel() const { return m_inertiaAccel; }
	ILINE float GetTimeImpulseRecover() const { return m_timeImpulseRecover; };

	virtual void SetViewRotation( const Quat &rotation ) {};
	virtual Quat GetViewRotation() const { return GetEntity()->GetRotation(); };
	virtual void EnableTimeDemo( bool bTimeDemo ) {};

	// offset to add to the computed camera angles every frame
	virtual void AddViewAngleOffsetForFrame(const Ang3 &offset);
	virtual const Ang3& GetViewAngleOffset() const;

	//------------------------------------------------------------------------
	virtual void Revive( EReasonForRevive reasonForRevive = kRFR_Spawn);
  virtual void Reset(bool toGame) {};
	//physicalization
	virtual void Physicalize(EStance stance=STANCE_NULL);
	virtual void PostPhysicalize();
	virtual void RagDollize( bool fallAndPlay );
	//
  virtual int IsGod(){ return 0; }

	//reset function clearing state and animations for teleported actor
	virtual void OnTeleported() {}

	virtual void SetSpectatorState(uint8 state) {}
	virtual EActorSpectatorState GetSpectatorState() { return eASS_None; }

	virtual void SetSpectatorModeAndOtherEntId(uint8 mode, EntityId othEntId) {};

	virtual uint8 GetSpectatorMode() const { return 0; };
	virtual void SetSpectatorTarget(EntityId targetId) {};
	virtual EntityId GetSpectatorTarget() const { return 0; };
	virtual void SetSpectatorCCTVCam(EntityId camId) {};
	virtual EntityId GetSpectatorCCTVCam() const { return 0; };
	
	virtual void SetDeathCamSubject(EntityId subjectId) {};

	//get actor status
	virtual SActorStats *GetActorStats() { return 0; };
	virtual const SActorStats *GetActorStats() const { return 0; };
	virtual SActorParams *GetActorParams() { return 0; };
	virtual const SActorParams *GetActorParams() const { return 0; };

	virtual void SetStats(SmartScriptTable &rTable);
	virtual void UpdateScriptStats(SmartScriptTable &rTable);
	virtual ICharacterInstance *GetFPArms(int i) const { return GetEntity()->GetCharacter(3+i); };
	//set/get actor params
	virtual void SetParams(SmartScriptTable &rTable,bool resetFirst = false);
	virtual bool GetParams(SmartScriptTable &rTable) { return false; };
	//
	virtual void Freeze(bool freeze) {};
	virtual void Fall(Vec3 hitPos = Vec3(0,0,0), float time = 0.0f);
	//throw away the actors helmet (if available) [Jan Müller]
	virtual bool LooseHelmet(Vec3 hitDir = Vec3(0,0,0), Vec3 hitPos = Vec3(0,0,0));
	virtual void GoLimp();
	virtual void StandUp();
	//virtual void NotifyLeaveFallAndPlay();
	virtual bool IsFallen() const;
	virtual bool IsStandingUp() const;	// Returns true if the entity is playing the standup animation of the fall and play
	virtual bool CanFall() const { return false; };
	//
	virtual IEntity *LinkToVehicle(EntityId vehicleId);
	virtual IEntity *LinkToVehicleRemotely(EntityId vehicleId);
	virtual void LinkToMountedWeapon(EntityId weaponId) {};
	virtual IEntity *LinkToEntity(EntityId entityId, bool bKeepTransformOnDetach=true);
	virtual void StartInteractiveAction(EntityId entityId);
	virtual void StartInteractiveActionByName(const char* interaction);
	virtual void EndInteractiveAction(EntityId entityId);
	
	virtual IEntity *GetLinkedEntity() const
	{
		return m_linkStats.GetLinked();
	}

	virtual IVehicle *GetLinkedVehicle() const
	{
		return m_linkStats.GetLinkedVehicle();
	}

	float GetLookFOV(const SActorParams * pActorParams) const
	{
		return GetLinkedVehicle() ? pActorParams->lookInVehicleFOVRadians : pActorParams->lookFOVRadians;
	}

	virtual void SetViewInVehicle(Quat viewRotation) {};

	virtual void SupressViewBlending() {};

	ILINE Vec3 GetLBodyCenter()
	{
		const SStanceInfo *pStance(GetStanceInfo(GetStance()));
		return Vec3(0,0,(pStance->viewOffset.z - pStance->heightPivot) * 0.5f);
	}

	ILINE Vec3 GetWBodyCenter()
	{
		return GetEntity()->GetWorldTM() * GetLBodyCenter();
	}

	//for animations
	virtual void RequestMechanic(const char* mechanicName) {}
	virtual void PlayAction(const char *action,const char *extension, bool looping=false) {};
	//
	virtual void SetMovementTarget(const Vec3 &position,const Vec3 &looktarget,const Vec3 &up,float speed) {};
	//
	virtual void CreateScriptEvent(const char *event,float value,const char *str = NULL);
	virtual bool CreateCodeEvent(SmartScriptTable &rTable);
	virtual void AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event, const uint32 eventNameCRC);
	//
	virtual void CameraShake(float angle,float shift,float duration,float frequency,Vec3 pos,int ID,const char* source="") {};
	//
	virtual void VectorToLocal(Vec3 &v) {};
	//
	virtual void SetAngles(const Ang3 &angles) {};
	virtual Ang3 GetAngles() {return Ang3(0,0,0);};
	virtual void AddAngularImpulse(const Ang3 &angular,float deceleration=0.0f,float duration=0.0f){}
	//
	virtual void SetViewLimits(Vec3 dir,float rangeH,float rangeV) {};
	virtual void SetHealth( int health );
	virtual void DamageInfo(EntityId shooterID, EntityId weaponID, IEntityClass *pProjectileClass, float damage, const char *damageType, const Vec3 hitDirection);
	virtual IAnimatedCharacter * GetAnimatedCharacter() { return m_pAnimatedCharacter; }
	virtual void PlayExactPositioningAnimation( const char* sAnimationName, bool bSignal, const Vec3& vPosition, const Vec3& vDirection, float startWidth, float startArcAngle, float directionTolerance ) {}
	virtual void CancelExactPositioningAnimation() {}
	virtual void PlayAnimation( const char* sAnimationName, bool bSignal ) {}
	virtual EntityId GetCurrentTargetEntityId() const { return 0; }
	virtual const Vec3 * GetCurrentTargetPos() const { return NULL; }

	void SetMaxHealth( int maxHealth );
	virtual int32 GetHealth() const { return static_cast<int32>(int_round(m_health)); }
  virtual int32 GetMaxHealth() const { return int32(m_maxHealth); }
	virtual int32 GetArmor() const { return 0; }
	virtual int32 GetMaxArmor() const { return 0; }
	virtual void Kill();

	void ResetHelmetAttachment();

	virtual void BindInputs( IAnimationGraphState * pAGState );
	//marcok: GC hack
	virtual bool IsSwimming() const {	return false; };
	virtual bool ShouldSwim() const { return false; };

	virtual bool IsSprinting() { return false;}
	virtual bool CanFire(){ return true; }

	//stances
	ILINE EStance GetStance() const 
	{
		return m_stance;
	}

	ILINE const SStanceInfo *GetStanceInfo(EStance stance) const 
	{
		if (stance < 0 || stance > STANCE_LAST)
			return &m_defaultStance;
		return &m_stances[stance];
	}

	ILINE SActorWeakRef GetWeakRef() const { return SActorWeakRef(m_selfRefWrapper); }

	virtual void	SetupStance(EStance stance,SStanceInfo *info);
	//

	// forces the animation graph to select a state
	void QueueAnimationState( const char * state );

	virtual bool SetAnimationInput( const char * inputID, const char * value )
	{
		// Handle action and signal inputs via AIproxy, since the AI system and
		// the AI agent behavior depend on those inputs.
		if (IEntity* pEntity = GetEntity())
			if (IAIObject* pAI = pEntity->GetAI())
				if (IAIActorProxy* pProxy = pAI->GetProxy())
				{
					if(pProxy->IsEnabled())
					{
						bool	bSignal = strcmp(inputID, "Signal") == 0;
						bool	bAction = strcmp(inputID, "Action") == 0;
						if(bSignal)
						{
							return pProxy->SetAGInput( AIAG_SIGNAL, value );
						}
						else if(bAction)
						{
							// Dejan: actions should not go through the ai proxy anymore!
							/*
							if(_stricmp(value, "idle") == 0)
								return pProxy->ResetAGInput( AIAG_ACTION );
							else
							{
								return pProxy->SetAGInput( AIAG_ACTION, value );
							}
							*/
						}
					}
				}

		if (IAnimationGraphState * pState = GetAnimationGraphState())
		{
			pState->SetInput( pState->GetInputId(inputID), value );
			return true;
		}

		return false;
	}

	//
	virtual int GetBoneID(int ID,int slot = 0) const;
	virtual Vec3 GetLocalEyePos(int slot = 0) const;
	virtual Vec3 GetLocalEyePos2(int slot = 0) const;

	QuatT GetCameraTran(int slot = 0) const;
	ILINE const SActorAnimationEvents& GetAnimationEventsTable() const { return s_animationEventsTable; };

	virtual void UpdateMountedGunController(bool forceIKUpdate);

	//
	virtual void ProcessBonesRotation(ICharacterInstance *pCharacter,float frameTime);

	virtual void OnPhysicsPreStep(float frameTime){};

	//grabbing
	virtual IGrabHandler *CreateGrabHanlder();
	virtual void UpdateGrab(float frameTime);
	
	virtual bool CheckInventoryRestrictions(const char *itemClassName);

	virtual bool CanPickUpObject(IEntity *obj, float& heavyness, float& volume);
	virtual bool CanPickUpObject(float mass, float volume);

	//
	virtual void ProcessIKLimbs(ICharacterInstance *pCharacter,float frameTime);

	//IK limbs
	int GetIKLimbIndex(const char *limbName);
	ILINE SIKLimb *GetIKLimb(int limbIndex)
	{
		return &m_IKLimbs[limbIndex];
	}
	void CreateIKLimb(int characterSlot, const char *limbName, const char *rootBone, const char *midBone, const char *endBone, int flags = 0);

	//
	virtual IMovementController * GetMovementController() const
	{
		return m_pMovementController;
	}

	CDamageEffectController* GetDamageEffectController() { return &m_damageEffectController; }
	uint8 GetActiveDamageEffects() const { return m_damageEffectController.GetActiveEffects(); }
	uint8 GetDamageEffectsResetSwitch() const { return m_damageEffectController.GetEffectResetSwitch(); }
	uint8 GetDamageEffectsKilled() const { return m_damageEffectController.GetEffectsKilled(); }
	void SetActiveDamageEffects(uint8 active) { m_damageEffectController.SetActiveEffects(active); }
	void SetDamageEffectsResetSwitch(uint8 reset) { m_damageEffectController.SetEffectResetSwitch(reset); }
	void SetDamageEffectsKilled(uint8 killed) { m_damageEffectController.SetEffectsKilled(killed); }

	//stances
	virtual void SetStance(EStance stance);
	virtual void OnStanceChanged(EStance newStance, EStance oldStance);
	virtual bool TrySetStance(EStance stance); // Shared between humans and aliens.
	
	
	virtual	bool IsSuperJumping() const { return false; }

	IAnimationGraphState * GetAnimationGraphState();
	void SetFacialAlertnessLevel(int alertness);

	//weapons
	virtual IItem *GetCurrentItem(bool includeVehicle=false) const;
	EntityId GetCurrentItemId(bool includeVehicle=false) const;
	virtual IItem *GetHolsteredItem() const;
	void ProceduralRecoil( float duration, float strength );

	//Net
	EntityId NetGetCurrentItem() const;
	void NetSetCurrentItem(EntityId id);

	EntityId NetGetScheduledItem() const;
	void NetSetScheduledItem(EntityId id);

	//AI
	Vec3 GetAIAttentionPos();

	CWeaponAttachmentManager* GetWeaponAttachmentManager() { return m_pWeaponAM; }
	void InitActorAttachments();

	virtual void SwitchDemoModeSpectator(bool activate) {};	//this is a player only function

	//misc
	virtual void ReplaceMaterial(const char *strMaterial);
	virtual void SendMusicLogicEvent(EMusicLogicEvents event){};
	
	// ugly: replace by real class system
	static  const char* GetActorClassType() { return "CActor"; }
	virtual const char* GetActorClass() const { return CActor::GetActorClassType(); }

	//Grabbing
	virtual int	GetActorSpecies() const { return eGCT_UNKNOWN; }

	virtual void SetAnimTentacleParams(pe_params_rope& rope, float animBlend) {};

  virtual bool IsCloaked() const { return false; }

  virtual void DumpActorInfo();

	int IsFriendlyEntity(EntityId entityId);
	virtual float GetReloadSpeedScale(void) const { return 1.f; }

	void EnableSwitchingItems(bool enable);
	void EnableIronSights(bool enable);
	void EnablePickingUpItems(bool enable);

	bool CanSwitchItems() const { return m_enableSwitchingItems; }
	bool CanUseIronSights() const { return m_enableIronSights; }
	bool CanPickupItems() const { return m_enablePickupItems; }
	
	void BecomeRemotePlayer();

	void SetRagdollOnCollision();

	ILINE uint8 GetNetPhysCounter() { return m_netPhysCounter; }

	virtual void GetMemoryUsage( ICrySizer * pSizer ) const;
	void GetInternalMemoryUsage( ICrySizer * pSizer ) const;
protected:

	virtual void SetMaterialRecursive(ICharacterInstance *charInst, bool undo, IMaterial *newMat = 0);

	void DisplayDebugInfo();

	virtual void UpdateAnimGraph( IAnimationGraphState * pState );

	void NetPrePhysicsUpdate(float dt);

	//movement
	virtual IActorMovementController * CreateMovementController() = 0;
	//

	virtual Vec3 GetModelOffset() const { return GetStanceInfo(GetStance())->modelOffset; }

	void RegisterInAutoAimManager();
	void UnRegisterInAutoAimManager();
	bool GetAimTargetParams(SmartScriptTable& targetParams);

	CItem * StartRevive(int teamId);
	void FinishRevive(CItem * pItem);

	virtual bool ShouldRegisterAsAutoAimTarget() { return true; }


private:
	mutable IInventory * m_pInventory;
	void ClearExtensionCache();
	void CrapDollize();
	void UpdateRagdollOnCollision();
	void UpdateSleep(float fFrameTime);


protected:
	void RequestServerResync()
	{
		if (!IsClient())
			GetGameObject()->RequestRemoteUpdate(eEA_Physics | eEA_GameClientDynamic | eEA_GameServerDynamic | eEA_GameClientStatic | eEA_GameServerStatic);
	}
	
	//
	typedef std::vector<SIKLimb> TIKLimbs;
	
	//
	virtual void SetActorModel();
	virtual bool UpdateStance();
  virtual void OnCloaked(bool cloaked){};
  virtual void PhysicalizeBodyDamage(){};
	mutable int16 m_boneIDs[BONE_ID_NUM];

	CActorTelemetry m_telemetry;
	IStatsTracker		*m_statsTracker;
	SActorRefWrapper	m_selfRefWrapper;

	static SActorAnimationEvents s_animationEventsTable;

	bool	m_isClient;
	bool	m_isMigrating;
	float m_health;
	int32 m_maxHealth;

	EActorState m_actorState;
 
	EStance m_stance;
	EStance m_desiredStance;

	static SStanceInfo m_defaultStance;
	SStanceInfo m_stances[STANCE_LAST];

	SmartScriptTable m_actorStats;

	IAnimatedCharacter *m_pAnimatedCharacter;
	IActorMovementController * m_pMovementController;
	CDamageEffectController m_damageEffectController;

	static IItemSystem			*m_pItemSystem;
	static IGameFramework		*m_pGameFramework;
	static IGameplayRecorder*m_pGameplayRecorder;

	//
	IAnimationGraph::InputID m_inputHealth;
	IAnimationGraph::InputID m_inputStance;
	IAnimationGraph::InputID m_inputFiring;
	IAnimationGraph::InputID m_inputSignal;
	IAnimationGraph::InputID m_inputAction;

	IGrabHandler *m_pGrabHandler;

	mutable SLinkStats m_linkStats;

	TIKLimbs m_IKLimbs;

	// Weapon Attachment manager
	CWeaponAttachmentManager *m_pWeaponAM;

	uint8 m_currentPhysProfile;

	float m_airControl;
	float m_airResistance;
	float m_inertia;
	float m_inertiaAccel;
	float m_timeImpulseRecover;

	EntityId m_netLastSelectablePickedUp;
	
	//helmet serialization
	EntityId	m_lostHelmet, m_serializeLostHelmet;
	string		m_lostHelmetObj, m_serializelostHelmetObj;
	string		m_lostHelmetMaterial;
	string		m_lostHelmetPos;

	_smart_ptr<IMaterial> m_pReplacementMaterial;

	//
  std::map< ICharacterInstance*, _smart_ptr<IMaterial> > m_testOldMats;
  std::map< IAttachmentObject*, _smart_ptr<IMaterial> > m_attchObjMats;
  //std::map< EntityId, IMaterial* > m_wepAttchMats;

	float			m_sleepTimer,m_sleepTimerOrg;

	int				m_teamId;

	bool m_enableSwitchingItems;
	bool m_enableIronSights;
	bool m_enablePickupItems;
	bool m_ragdollOnCollision;

	// Net Interpolation

	CTimeValue	m_netLastUpdate;     	 // Time we last received information
	Vec3		m_netDesiredLocation;    // Interpolate just position, since dir is done by player-input serialisation
	Vec3		m_netCurrentLocation;    // The location set by interpolation, need to store this since real pos can be influenced by the physics
	Vec3		m_netLerpVelocity;       // speed to move there
	float		m_netDesiredSpeed;       // Speed
	float		m_netLerpSpeed;          // Cur Lerp speed
	bool		m_netNewUpdate;          // used to detect when we've received new information
	bool		m_netSetPosition;        // Are we forcing the physics position
	bool		m_netDoLerp;             // Enable net lerping
	bool		m_netFirstPhysUpdate;		 // To allow us to always process the first update
	uint8		m_netPhysCounter;				 //	Physics counter, to enable us to throw away old updates

	bool		m_registeredInAutoAimMng;
};

#endif //__Actor_H__
