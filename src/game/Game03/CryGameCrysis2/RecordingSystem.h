#ifndef __RECORDINGSYSTEM_H__
#define __RECORDINGSYSTEM_H__

#include "IKTorsoAim_Helper.h"
#include "WeaponFPAiming.h"
#include "CryCharAnimationParams.h"
#include "IParticles.h"
#include "RecordingBuffer.h"
#include "TracerManager.h"

#define MAX_WEAPON_ACCESSORIES 3
#define FP_RECORDING_BUFFER_SIZE		(50 * 1024)
#define FP_RECEIVE_BUFFER_SIZE		(20 * 1024)
#define RECORDING_BUFFER_SIZE		(500*1024)
#define RECORDING_BUFFER_QUEUE_SIZE		(5*1024)
#define ALL_PARTS 0xFFFFFFFF
#define SOUND_NAMES_SIZE (50*1024)

class CActor;

enum ERecSysMode
{
	ERECSYS_INACTIVE,
	ERECSYS_RECORDING,
	ERECSYS_PLAYBACK_START,
	ERECSYS_PLAYBACK,
	ERECSYS_STOPPED
};

enum EFirstPersonFlags
{
	eFPF_OnGround = BIT(0),
	eFPF_Sprinting = BIT(1),
	eFPF_SuperJump = BIT(2),
	eFPF_StartZoom = BIT(3),
	eFPF_ThirdPerson = BIT(4),
};

enum EThirdPersonFlags
{
	eTPF_Dead = BIT(0),
	eTPF_Cloaked = BIT(1),
	eTPF_AimIk = BIT(2),
	eTPF_Ragdoll = BIT(3),
	eTPF_HasThreatDetectorPerk = BIT(4),
};

enum EReplayActorFlags
{
	eRAF_Dead = BIT(0),
	eRAF_ShowName = BIT(1),
	eRAF_HasThreatDetectorPerk = BIT(2),
};

enum EPlaybackMode
{
	ePM_FirstPerson,
	ePM_ThirdPerson,
	ePM_ProjectileFollow,
	ePM_Static,
};

enum EProjectileType
{
	ePM_Grenade,
	ePM_Kvolt,
};

enum EThirdPersonPacket
{
	eTPP_TPChar = eRBPT_Custom,
	eTPP_SpawnCustomParticle,
	eTPP_ParticleCreated,
	eTPP_ParticleDeleted,
	eTPP_WeaponSelect,
	eTPP_VehicleChange,
	eTPP_ActorAnim,
	eTPP_ActorUpperAnim,
	eTPP_WeaponAnim,
	eTPP_OnShoot,
	eTPP_EntitySpawn,
	eTPP_EntityRemoved,
	eTPP_EntityLocation,
	eTPP_PlaySound,
	eTPP_StopSound,
	eTPP_BulletTrail,
};

enum EFirstPersonPacket
{
	eFPP_FPChar = eRBPT_Custom,
	eFPP_Flashed,
};

struct SRecording_FPChar : SRecording_Packet
{
	SRecording_FPChar()
		: camlocation(IDENTITY)
		, relativePosition(IDENTITY)
		, fov(0)
		, frametime(0)
		, playerFlags(0)
	{
		size = sizeof(SRecording_FPChar);
		type = eFPP_FPChar;
	}
	QuatT camlocation;
	QuatT relativePosition;
	float fov;
	float frametime;
	uint8 playerFlags;	// EFirstPersonFlags
};

struct SRecording_Flashed : SRecording_Packet
{
	SRecording_Flashed()
		: frametime(0)
		, duration(0)
		, blindAmount(0)
	{
		size = sizeof(SRecording_Flashed);
		type = eFPP_Flashed;
	}
	float frametime;
	float duration;
	float blindAmount;
};

struct SRecording_OnShoot : SRecording_Packet
{
	SRecording_OnShoot()
		: shooter(0)
	{
		size = sizeof(SRecording_OnShoot);
		type = eTPP_OnShoot;
	}
	EntityId shooter;
};

struct SRecording_TPChar : SRecording_Packet
{
	SRecording_TPChar()
		: eid(0)
		, entitylocation(IDENTITY)
		, velocity(ZERO)
		, aimdir(ZERO)
		, playerFlags(0)
	{
		size = sizeof(SRecording_TPChar);
		type = eTPP_TPChar;
		layerEffectParams[0] = 0;
		layerEffectParams[1] = 0;
	}
	EntityId eid;
	QuatT entitylocation;
	Vec3 velocity;
	Vec3 aimdir;
	uint32 layerEffectParams[2];	// Used for suit mode effects
	uint8 playerFlags;		// EThirdPersonFlags
};

struct SRecording_Animation : SRecording_Packet
{
	SRecording_Animation()
		: eid(0)
		, animID(0)
		, speedMultiplier(1.0f)
	{
		size = sizeof(SRecording_Animation);
		type = eRBPT_Invalid;
	}
	EntityId eid;
	int animID;
	float speedMultiplier;
	CryCharAnimationParams animparams;
};

struct SRecording_WeaponSelect : SRecording_Packet
{
	SRecording_WeaponSelect()
		: entityId(0)
		, pWeaponClass(NULL)
		, isRippedOff(false)
	{
		size = sizeof(SRecording_WeaponSelect);
		type = eTPP_WeaponSelect;
		for (int i=0; i<MAX_WEAPON_ACCESSORIES; i++)
		{
			pAccessoryClasses[i] = NULL;
		}
	}
	EntityId entityId;
	IEntityClass* pWeaponClass;
	IEntityClass* pAccessoryClasses[MAX_WEAPON_ACCESSORIES];
	bool isRippedOff;
};

struct SRecording_VehicleChange : SRecording_Packet
{
	SRecording_VehicleChange()
		: playerEntityId(0)
		, vehicleEntityId(0)
	{
		size = sizeof(SRecording_VehicleChange);
		type = eTPP_VehicleChange;
	}
	EntityId playerEntityId;
	EntityId vehicleEntityId;
};

struct SRecording_SpawnCustomParticle : SRecording_Packet
{
	SRecording_SpawnCustomParticle()
		: pParticleEffect(NULL)
		, location(IDENTITY)
	{
		size = sizeof(SRecording_SpawnCustomParticle);
		type = eTPP_SpawnCustomParticle;
	}
	IParticleEffect *pParticleEffect;
	Matrix34 location;
};

struct SRecording_ParticleCreated : SRecording_Packet
{
	SRecording_ParticleCreated()
		: pParticleEmitter(NULL)
		, pParticleEffect(NULL)
		, location(IDENTITY)
		, entityId(0)
		, entitySlot(0)
	{
		size = sizeof(SRecording_ParticleCreated);
		type = eTPP_ParticleCreated;
	}
	void GetMemoryUsage(ICrySizer *pSizer) const{}

	IParticleEmitter *pParticleEmitter;
	IParticleEffect *pParticleEffect;
	Matrix34 location;
	EntityId entityId;
	int entitySlot;
};

struct SRecording_ParticleDeleted : SRecording_Packet
{
	SRecording_ParticleDeleted()
		: pParticleEmitter(NULL)
	{
		size = sizeof(SRecording_ParticleDeleted);
		type = eTPP_ParticleDeleted;
	}
	IParticleEmitter *pParticleEmitter;
};

struct SRecording_EntitySpawn : SRecording_Packet
{
	SRecording_EntitySpawn()
		: pClass(NULL)
		, szCharacterSlot(NULL)
		, szStatObjSlot(NULL)
		, entityFlags(0)
		, entitylocation(IDENTITY)
		, entityId(0)
	{
		size = sizeof(SRecording_EntitySpawn);
		type = eTPP_EntitySpawn;
	}
	void GetMemoryUsage(ICrySizer *pSizer) const{}

	IEntityClass* pClass;
	const char* szCharacterSlot;
	const char* szStatObjSlot;
	uint32 entityFlags;
	QuatT entitylocation;
	EntityId entityId;
};

struct SRecording_EntityRemoved : SRecording_Packet
{
	SRecording_EntityRemoved()
		: entityId(0)
	{
		size = sizeof(SRecording_EntityRemoved);
		type = eTPP_EntityRemoved;
	}
	EntityId entityId;
};

struct SRecording_EntityLocation : SRecording_Packet
{
	SRecording_EntityLocation()
		: entityId(0)
		, entitylocation(IDENTITY)
	{
		size = sizeof(SRecording_EntityLocation);
		type = eTPP_EntityLocation;
	}
	EntityId entityId;
	QuatT entitylocation;
};

struct SRecording_PlaySound : SRecording_Packet
{
	SRecording_PlaySound()
		: position(ZERO)
		, soundId(INVALID_SOUNDID)
		, szName(0)
		, flags(0)
		, soundSemantic(eSoundSemantic_None)
	{
		size = sizeof(SRecording_PlaySound);
		type = eTPP_PlaySound;
	}
	Vec3 position;
	tSoundID soundId;
	const char* szName;
	uint32 flags;
	ESoundSemantic soundSemantic;
};

struct SRecording_StopSound : SRecording_Packet
{
	SRecording_StopSound()
		: soundId(INVALID_SOUNDID)
	{
		size = sizeof(SRecording_StopSound);
		type = eTPP_StopSound;
	}
	tSoundID soundId;
};

struct SRecording_BulletTrail : SRecording_Packet
{
	SRecording_BulletTrail()
		: start(ZERO)
		, end(ZERO)
		, friendly(false)
	{
		size = sizeof(SRecording_BulletTrail);
		type = eTPP_BulletTrail;
	}
	Vec3 start;
	Vec3 end;
	bool friendly;
};

struct SKillCamSendRequest
{
	EntityId shooter;
	EntityId victim;

	float m_time;

	SKillCamSendRequest(EntityId sh, EntityId vic) : shooter(sh), victim(vic) {}
	void GetMemoryUsage(ICrySizer *pSizer) const{}
};

struct SPlayerInitialState
{
	SPlayerInitialState()
		: animationStartTime(0)
		, upperAnimationStartTime(0)
		, weaponAnimationStartTime(0)
	{
		// Set the sizes to 0 to indicate the state has not been set
		animation.size = 0;
		upperAnimation.size = 0;
		weaponAnimation.size = 0;
		weapon.size = 0;
		vehicle.size = 0;
	}
	float animationStartTime;
	float upperAnimationStartTime;
	float weaponAnimationStartTime;
	SRecording_Animation animation;
	SRecording_Animation upperAnimation;
	SRecording_Animation weaponAnimation;
	SRecording_WeaponSelect weapon;
	SRecording_VehicleChange vehicle;
};

struct IWeaponEventListener;
enum EPlayerPlugInEvent;

struct AimIKInfo
{
	bool aimIKEnabled;
	Vec3 aimIKTarget;

	AimIKInfo() : aimIKEnabled(false), aimIKTarget(0, 0, 1) {};
	AimIKInfo( bool enabled, Vec3 &target ) : aimIKEnabled(enabled), aimIKTarget(target) {};

	void GetMemoryUsage(ICrySizer *pSizer) const{}
};

class CWeapon;
class CPlayer;

struct SWeaponInfo
{
	CWeapon *weaponPtr;
	bool isMounted;

		SWeaponInfo() : weaponPtr(NULL), isMounted(false) {}
	SWeaponInfo(CWeapon *weapon, bool mounted) : weaponPtr(weapon), isMounted(mounted) {}

	void GetMemoryUsage(ICrySizer *pSizer) const{}
};


typedef std::map<EntityId, Vec3> ChrVelocityMap;
typedef std::map<EntityId,AimIKInfo> EntityAimIKMap;
typedef std::map<EntityId,int> EntityIntMap;
typedef std::map<EntityId,SWeaponInfo> EntityWeaponMap;
typedef std::map<EntityId,EntityId> EntityVehicleMap;
typedef std::map<IParticleEffect*, IParticleEffect*> ParticleEffectMap;
typedef std::map<EntityId, EntityId> TReplayEntityMap;
typedef std::map<EntityId, SRecording_EntitySpawn> TEntitySpawnMap;
typedef std::map<IParticleEmitter*, IParticleEmitter*> TReplayParticleMap;
typedef std::map<tSoundID, tSoundID> TReplaySoundMap;
typedef std::map<IParticleEmitter*, SRecording_ParticleCreated> TParticleCreatedMap;

class CRecordingSystem : public IInputEventListener, IEntitySystemSink, IParticleEffectListener, ISoundSystemEventListener
{
public:
	CRecordingSystem();
	~CRecordingSystem();

	void Reset(void);

	void Update(float frameTime);
	void PostUpdate();
	void RecordPlayerStateChange(IActor* pPlayer);
	void RecordEntityInfo();
	void RecordParticleInfo();
	void RecordEntitySpawn(IEntity* pEntity, bool recordClass = false, uint32 entityFlags = 0);
	bool OnInputEvent(const SInputEvent &rInputEvent);

	// IEntitySystemSink interface implementation
	virtual bool OnBeforeSpawn(SEntitySpawnParams &params);
	virtual void OnSpawn(IEntity *pEntity, SEntitySpawnParams &params);
	virtual bool OnRemove(IEntity *pEntity);
	virtual void OnEvent(IEntity *pEntity, SEntityEvent &event) {}

	// IParticleEffectListener interface implementation
	virtual void OnCreateEmitter(IParticleEmitter* pEmitter, bool bIndependent, Matrix34 const& mLoc, const IParticleEffect* pEffect, const ParticleParams* pParams);
	virtual void OnDeleteEmitter(IParticleEmitter* pEmitter);

	// ISoundSystemEventListener interface implementation
	virtual void OnSoundSystemEvent(ESoundSystemCallbackEvent event, ISound *pSound);

	// Event indicating a tracer is being created, returns false to prevent creation of the tracer
	bool OnEmitTracer(const CTracerManager::STracerParams &params);

	// Called when a perk event is created
	void OnPerkEvent(CPlayer* pPlayer, EPlayerPlugInEvent perkEvent, void *pData);

	bool IsPlaybackQueued(void) { return (m_mode == ERECSYS_PLAYBACK_START); }
	bool IsPlayingBack(void) { return (m_mode == ERECSYS_PLAYBACK); }
	bool IsRecording(void) { return m_mode == ERECSYS_RECORDING; }
	bool IsStopped(void) { return m_mode == ERECSYS_STOPPED; }

	void StartRecording();
	void StopRecording();
	void StartPlayback(EntityId killer, EntityId projectileId, int hitType, Vec3 impulse);
	void StopPlayback();

	size_t GetFirstPersonData(uint8 **data);
	size_t GetTPCameraData(uint8 **data = NULL);
	void SetFirstPersonData(uint8 *data, size_t datasize);
	void SetFirstPersonDataPart(uint8 *data, int part, int numparts, size_t datasize);

	void QueueSendRequest(SKillCamSendRequest data);

	void UpdateView(SViewParams &viewParams);

	void AddPacket(const SRecording_Packet &packet);
	void QueueAddPacket(const SRecording_Packet &packet);
	void SetPlayerVelocity(EntityId eid, Vec3 vel) {m_chrvelmap[eid] = vel;}
	void SetPlayerAimIK(EntityId eid, AimIKInfo aikinf) {m_eaikm[eid] = aikinf;}
	void DiscardingPacket(SRecording_Packet *packet, float recordedTime);
	void OnPlayerFlashed(float duration, float blindAmount);

	struct SReplayActorInfo
	{
		EntityId chr_id;
		EntityId gun_id;
		EntityId vehicleId;
		uint8 flags;		// EReplayActorFlags
		
		// These are used for HUD player name tags
		Vec3 drawPos;
		float size;
		int8 team;

		void GetMemoryUsage(ICrySizer *pSizer) const{} 
	};

	typedef std::map<EntityId, SReplayActorInfo> TReplayActorMap;

	void GetMemoryUsage(class ICrySizer *pSizer) const  
	{ 
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_pBuffer);
		pSizer->AddObject(m_pBufferFirstPerson);
		pSizer->AddObject(m_killcamQueue);
		pSizer->AddObject(m_replayActors);
		pSizer->AddObject(m_replayEntities);
//		pSizer->AddObject(m_replayParticles); //Diesel cut
		pSizer->AddObject(m_replaySounds);
		pSizer->AddObject(m_tempReplayEntities);
		pSizer->AddObject(m_recordingEntities);
		pSizer->AddObject(m_newParticles);
		pSizer->AddObject(m_newActorEntities);
		pSizer->AddObject(m_chrvelmap);
		pSizer->AddObject(m_eaikm);
		pSizer->AddObject(m_weaponMap);
		pSizer->AddObject(m_vehicleMap);
		pSizer->AddObject(m_recordEntityClassFilter);
//		pSizer->AddObject(m_excludedParticleEffects); //Diesel cut
//		pSizer->AddObject(m_replacementParticleEffects); //Diesel cut
		pSizer->AddObject(m_playerInitialStatesMap);
		pSizer->AddObject(m_discardedEntitySpawns);
//		pSizer->AddObject(m_discardedParticleSpawns); //Diesel cut
	}
private:
	// The purpose of SReplayEventGuard is to provide a mechanism by which certain events be guarded against.
	// This is best explained with an example:
	// The recording system will listen to all entity spawn events to track when new entities are created.
	// During killcam replay we will spawn new fake entities to replace the real ones for replay purposes.
	// The spawning of the fake entities will result in spawn events to be fired off, however we are not
	// interested in listening to spawn events of the fake entities. So in order to guard against recording
	// these events we can use this class.
	// When the object is constructed it increments a counter, and decrements it upon destruction. When
	// the counter is at 0 that means there is no guard in place and the event should be recorded.
	struct SReplayEventGuard
	{
		SReplayEventGuard() { CRecordingSystem::m_replayEventGuard++; }
		~SReplayEventGuard() { CRecordingSystem::m_replayEventGuard--; }
	};

	// This macro provides a convenient way to guard against listenting to events we are not interested in.
	#define REPLAY_EVENT_GUARD SReplayEventGuard __ReplayEventGuard__;

	float GetLiveBufferLenSeconds(void);
	float GetBufferLenSeconds(void);
	void FillOutFPCamera(SRecording_FPChar *cam, float playbacktime);
	void PlaybackRecordedFirstPersonActions(float playbacktime);
	float GetFPCamDataTime();
	void DropTPFrame(bool recordDiscarded);
	void DropTPFramesUpTo(float trimTime);		// Drops frames until the given time is reached
	void PlaybackStartingSetupEntities(void);
	void DebugDrawAnimationData(void);

	void UpdatePlayback(float frameTime);
	float PlaybackRecordedFrame(float frameTime);
	void RecordTPInfo(void);
	void RecordTPCharPacket(IEntity *pEntity, CActor *pActor);
	void RecordWeaponSelection(CActor *pActor, CWeapon *pCurrentWeapon);
	void RecordActorWeapon();
	void AddQueuedPackets();
	bool ApplyPlayerInitialState(EntityId entityId);
	void UpdateFirstPersonPosition();
	void UpdateThirdPersonPosition(const SRecording_TPChar *tpchar, float frameTime);
	void ApplyVehicleChange(const SRecording_VehicleChange *vehicleChange);
	void ApplyWeaponSelect(const SRecording_WeaponSelect *weaponSelect);
	void ApplyAnimPacket(const SRecording_Animation *animation, float recordedTime = 0);
	void PlayInitialAnimation(ISkeletonAnim* pSkeletonAnim, IAnimationSet* pAnimSet, int32 animId, const CryCharAnimationParams& params, float startTime, float speedMultiplier);
	void SetDefaultInitialState(SPlayerInitialState &initialState, EntityId playerEntityId);
	void OnPlayerFirstPersonChange(IEntity* pPlayerEntity, EntityId weaponId, bool firstPerson);
	void CloakEnable(IEntityRenderProxy* pRenderProxy, bool enable, bool fade);
	void ApplyEntitySpawn(const SRecording_EntitySpawn *entitySpawn);
	void ApplyEntityRemoved(const SRecording_EntityRemoved *entityRemoved);
	void ApplyEntityLocation(const SRecording_EntityLocation *entityLocation);
	void ApplySpawnCustomParticle(const SRecording_SpawnCustomParticle *spawnCustomParticle);
	bool ApplyParticleCreated(const SRecording_ParticleCreated *particleCreated);
	void ApplyParticleDeleted(const SRecording_ParticleDeleted *particleDeleted);
	void ApplyFlashed(const SRecording_Flashed *pFlashed);
	void HideEntityKeepingPhysics(IEntity *pEntity, bool hide);
	void SwapFPCameraEndian(uint8* pBuffer, size_t size, bool sending);
	void PhysicalizeEntity(IEntity *pEntity, bool ragdoll);
	void DisableFlashBangEffect();
	void ApplyPlaySound(const SRecording_PlaySound* pPlaySound);
	void ApplyStopSound(const SRecording_StopSound* pStopSound);
	const char* CacheSoundString(const char* name);
	void ClearSoundCache();
	void ApplyBulletTrail(const SRecording_BulletTrail* pBulletTrail);

	std::deque<SKillCamSendRequest> m_killcamQueue;
	TReplayActorMap m_replayActors;
	TReplayEntityMap m_replayEntities;
	TReplayParticleMap m_replayParticles;
	TReplaySoundMap m_replaySounds;
	std::vector<EntityId> m_tempReplayEntities;
	std::vector<EntityId> m_recordingEntities;
	std::vector<SRecording_ParticleCreated> m_newParticles;
	std::vector<EntityId> m_newActorEntities;
	CRecordingBuffer		*m_pBuffer;
	CRecordingBuffer		*m_pBufferFirstPerson;
	IWeaponEventListener *m_iwel;
	float m_playbackStartTime;
	float m_recordedStartTime;
	float m_playbackRequestTime;

	// Buffer used for replaying first person camera positions
	uint32 m_fppacketparts;
	uint32 m_firstPersonDataSize;
	uint8 m_firstPersonData[FP_RECEIVE_BUFFER_SIZE];

	// Buffer used for sending first person camera data
	uint8 m_firstPersonSendBuffer[FP_RECORDING_BUFFER_SIZE];

	// Buffer used for replaying third person data
	uint32 m_tpdatasize;
	uint8 m_tpdatabuffer[RECORDING_BUFFER_SIZE];

	// Recorded packets which are queued to be added to the third person buffer next frame
	uint32 m_queuedPacketsSize;
	uint8 m_pQueuedPackets[RECORDING_BUFFER_QUEUE_SIZE];

	ERecSysMode	m_mode;
	EPlaybackMode m_playbackMode;
	EPlaybackMode m_prevPlaybackMode;
	EProjectileType m_projectileType;
	bool m_cameraSmoothing;

	EntityId m_killer;
	EntityId m_projectileId;

	ChrVelocityMap m_chrvelmap;
	EntityAimIKMap m_eaikm;
	CIKTorsoAim_Helper m_torsoAimIK;
	SParams_WeaponFPAiming m_weaponParams;
	CWeaponFPAiming m_weaponFPAiming;
	SRecording_FPChar m_firstPersonCharacter;
	EntityWeaponMap m_weaponMap;
	EntityVehicleMap m_vehicleMap;
	std::set<IEntityClass*> m_recordEntityClassFilter;
	std::set<IParticleEffect*> m_excludedParticleEffects;
	ParticleEffectMap m_replacementParticleEffects;

	EntityIntMap m_playerInitialStatesMap;
	SPlayerInitialState m_playerInitialStates[32];
	TEntitySpawnMap m_discardedEntitySpawns;
	TParticleCreatedMap m_discardedParticleSpawns;
	static int m_replayEventGuard;
	bool m_debugPlayBack;
	bool m_spawningReplayEntity;
	bool m_recordingPlaybackUpdate;
	bool m_killerIsFriendly;
	QuatT m_newProjectilePosition;
	char m_soundNames[SOUND_NAMES_SIZE];
};

#define KILL_CAM_KICK_IN_TIME			(2.f)
#define MAX_KILLCAM_LEN		(5.f)
#define RECORDING_SYSTEM_EXTRA_DEBUG 0


#endif // __RECORDINGSYSTEM_H__
