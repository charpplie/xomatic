/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: C++ Weapon Implementation

-------------------------------------------------------------------------
History:
- 22:8:2005   12:50 : Created by Márcio Martins

*************************************************************************/
#ifndef __WEAPON_H__
#define __WEAPON_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IItemSystem.h>
#include <IWeapon.h>
#include <IAgent.h>
#include <VectorMap.h>
#include <IMusicSystem.h>
#include "Item.h"


#define WEAPON_FADECROSSHAIR_SELECT	(0.250f)
#define WEAPON_FADECROSSHAIR_ZOOM		(0.200f)
#define WEAPON_SHOOT_TIMER					(5000)


struct SWeaponAmmo
{
	const IEntityClass* pClass;
	int count;
	SWeaponAmmo(const IEntityClass* pc, int c)
	{
		pClass = pc;
		count = c;
	}
};

class CProjectile;
class CWeaponSharedParams;

class CWeapon :
	public CItem,
	public IWeapon
{
	class ScheduleLayer_Leave;
	class ScheduleLayer_Enter;
	struct EndChangeFireModeAction;
	struct PlayLeverLayer;

protected:
	typedef std::map<string, int>								TFireModeIdMap;
	typedef std::vector<IFireMode *>						TFireModeVector;
	typedef std::map<string, int>								TZoomModeIdMap;
	typedef std::vector<IZoomMode *>						TZoomModeVector;
	typedef std::map<IEntityClass*, int>				TAmmoMap;

	struct SListenerInfo
	{
		IWeaponEventListener	*pListener;
#ifdef _DEBUG
		char who[64];
#endif
	};

	typedef std::vector<SListenerInfo>					TEventListenerVector;

	typedef struct SWeaponCrosshairStats
	{
		SWeaponCrosshairStats()
		{
			Reset();
		}

		void Reset()
		{
			fading = false;
			visible = true;
			fadefrom = 1.0f;
			fadeto = 1.0f;
			fadetime = 0.0f;
			fadetimer = 0.0f;
			opacity = 1.0f;
		}

		bool fading;
		bool visible;
		float fadefrom;
		float fadeto;
		float fadetime;
		float fadetimer;
		float opacity;
	}SWeaponCrosshairStats;

	typedef VectorMap<EStance, Vec3>	TStanceWeaponOffset;

public:

	struct SAIWeaponOffset
	{
		SAIWeaponOffset(): useEyeOffset(false){};

		bool								useEyeOffset;

		TStanceWeaponOffset	stanceWeponOffset;
		TStanceWeaponOffset	stanceWeponOffsetLeanLeft;
		TStanceWeaponOffset	stanceWeponOffsetLeanRight;

	};

	CWeapon();
	virtual ~CWeapon();

	// IItem, IGameObjectExtension
	virtual bool Init(IGameObject * pGameObject);
	virtual void InitClient(int channelId) { CItem::InitClient(channelId); };
	virtual void Release();
	virtual void FullSerialize( TSerialize ser );
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
	virtual void PostSerialize();
	virtual void SerializeLTL(TSerialize ser);
	virtual void Update(SEntityUpdateContext& ctx, int);
	virtual void PostUpdate( float frameTime );
	virtual void HandleEvent(const SGameObjectEvent&);
	virtual void ProcessEvent(SEntityEvent& event);
	virtual void SetChannelId(uint16 id) {};
	virtual void SetAuthority(bool auth);
	virtual void GetMemoryStatistics(ICrySizer * s);

	virtual void Reset();

	virtual void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	virtual void UpdateFPView(float frameTime);

	virtual IWeapon *GetIWeapon() { return this; };
	virtual const IWeapon *GetIWeapon() const { return this; };

	virtual void MeleeAttack();
	virtual bool CanMeleeAttack() const;
	virtual IFireMode *GetMeleeFireMode() const { return m_melee; };

	virtual void PerformThrow(const SProjectileThrowParams& throwParams, IActor* pActor = NULL) {};

  virtual void Select(bool select);
	virtual void Drop(float impulseScale, bool selectNext=true, bool byDeath=false);
	virtual void Freeze(bool freeze);

	virtual void OnPickedUp(EntityId actorId, bool destroyed);
	virtual void OnDropped(EntityId actorId);

  virtual void OnHit(float damage, const char* damageType);
  virtual void OnDestroyed();
	virtual void EnterWater(bool enter) {};

	//Needed for the mounted weapon
	virtual void StartUse(EntityId userId); 
	virtual void StopUse(EntityId userId); 

	virtual bool CheckAmmoRestrictions(EntityId pickerId);

	virtual bool SetAspectProfile( EEntityAspects aspect, uint8 profile );
	virtual uint8 GetDefaultProfile( EEntityAspects aspect );

	virtual bool FilterView(SViewParams &viewParams);
	virtual void PostFilterView(struct SViewParams &viewParams);

	// ~IItem
	virtual bool HasAttachmentAtHelper(const char *helper);
	virtual void GetAttachmentsAtHelper(const char *helper, CCryFixedStringListT<5, 30> &attachments);
	// Events
	virtual void OnShoot(EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType, 
		const Vec3 &pos, const Vec3 &dir, const Vec3 &vel);
	virtual void OnStartFire(EntityId shooterId);
	virtual void OnStopFire(EntityId shooterId);
	virtual void OnStartReload(EntityId shooterId, IEntityClass* pAmmoType);
	virtual void OnEndReload(EntityId shooterId, IEntityClass* pAmmoType);
	virtual void OnOutOfAmmo(IEntityClass* pAmmoType);
	virtual void OnReadyToFire();
	virtual void OnMelee(EntityId shooterId);
	virtual void OnStartTargetting(IWeapon *pWeapon);
	virtual void OnStopTargetting(IWeapon *pWeapon);
	virtual void OnSelected(bool selected);

	// IWeapon
	virtual void SetFiringLocator(IWeaponFiringLocator *pLocator);
	virtual IWeaponFiringLocator *GetFiringLocator() const;

	virtual void AddEventListener(IWeaponEventListener *pListener, const char *who);
	virtual void RemoveEventListener(IWeaponEventListener *pListener);

  virtual void SetDestinationEntity(EntityId targetId);
  virtual void SetDestination(const Vec3& pos){ m_destination = pos; }
	virtual const Vec3& GetDestination(){ return m_destination; }

	virtual Vec3 GetFiringPos(const Vec3 &probableHit) const;
	virtual Vec3 GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const;
	
	virtual void StartFire(const Vec3 *fireTarget=NULL);
	virtual void StopFire();
	virtual bool CanFire() const;

	virtual void StartZoom(EntityId shooterId, int zoomed = 0);
	virtual void StopZoom(EntityId shooterId);
	virtual bool CanZoom() const;
	virtual void ExitZoom(bool force=false);
	virtual bool IsZoomed() const;
	virtual bool IsZoomingInOrOut() const;
	virtual EZoomState GetZoomState() const;

	virtual bool IsReloading() const;
	
	virtual void MountAt(const Vec3 &pos);
	virtual void MountAtEntity(EntityId entityId, const Vec3 &pos, const Ang3 &angles);

	virtual void Reload(bool force=false);
	virtual bool CanReload() const;

	virtual bool OutOfAmmo(bool allFireModes) const;
	virtual bool LowAmmo(float) const { return false; }; // TODO: Implement properly if required. This is for Crysis2 compatibility.

	virtual int GetAmmoCount(IEntityClass* pAmmoType) const;
	virtual void SetAmmoCount(IEntityClass* pAmmoType, int count);

	virtual int GetInventoryAmmoCount(IEntityClass* pAmmoType) const;
	virtual void SetInventoryAmmoCount(IEntityClass* pAmmoType, int count);

	virtual int GetNumOfFireModes() const { return m_firemodes.size(); }
	virtual IFireMode *GetFireMode(int idx) const;
	virtual IFireMode *GetFireMode(const char *name) const;
	virtual int GetFireModeIdx(const char *name) const;
	virtual int GetFireModeIdxWithAmmo(const IEntityClass* pAmmoClass) const;
	virtual int GetCurrentFireMode() const;
	virtual void SetCurrentFireMode(int idx);
	virtual void SetCurrentFireMode(const char *name);
	virtual void ChangeFireMode();
	virtual int GetNextFireMode(int currMode) const;
	virtual void EnableFireMode(int idx, bool enable);
	virtual void FixAccessories(SAccessoryParams *newParams, bool attach);

	virtual IZoomMode *GetZoomMode(int idx) const;
	virtual IZoomMode *GetZoomMode(const char *name) const;
	virtual int GetZoomModeIdx(const char *name) const;
	virtual int GetCurrentZoomMode() const;
	virtual void SetCurrentZoomMode(int idx);
	virtual void SetCurrentZoomMode(const char *name);
	virtual void ChangeZoomMode();
	virtual void EnableZoomMode(int idx, bool enable);
	virtual void RestartZoom(bool force = false);

	virtual void SetCrosshairVisibility(bool visible);
	virtual bool GetCrosshairVisibility() const;
	virtual void SetCrosshairOpacity(float opacity);
	virtual float GetCrosshairOpacity() const;
	virtual void FadeCrosshair(float from, float to, float time);
	virtual void UpdateCrosshair(float frameTime);
  
	virtual void AccessoriesChanged();
	virtual bool PatchFireModeWithAccessory(IFireMode *pFireMode, const char *firemodeName);
	virtual bool PatchZoomModeWithAccessory(IZoomMode *pZoomMode, const char *zoommodeName);

	virtual float GetSpinUpTime() const;
	virtual float GetSpinDownTime() const;

	virtual void SetHostId(EntityId hostId);
	virtual EntityId GetHostId() const;

	virtual bool	PredictProjectileHit(IPhysicalEntity *pShooter, const Vec3 &pos, const Vec3 &dir,
		const Vec3 &velocity, float speed, Vec3& predictedPosOut, float& projectileSpeedOut,
		Vec3* pTrajectory = 0, unsigned int* trajectorySizeInOut = 0) const;

	virtual const AIWeaponDescriptor& GetAIWeaponDescriptor( ) const;

	virtual bool Query(EWeaponQuery query, const void* param = NULL);

	//Activate/Deactivate Laser and Light for the AI (also player if neccessary)
	virtual bool		IsLamAttached();
	virtual bool    IsFlashlightAttached();
	virtual void    ActivateLamLaser(bool activate, bool aiRequest = true);
	virtual void		ActivateLamLight(bool activate, bool aiRequest = true);
	virtual bool	  IsLamLaserActivated();
	virtual bool		IsLamLightActivated();
	virtual	void		RaiseWeapon(bool raise, bool faster = false);
	ILINE virtual uint8 GetRaisePose() { return m_raisePose; }

	// ~IWeapon

	int  GetMaxZoomSteps();
	void AssistAiming(float magnification=1.0f, bool accurate=false);
	bool IsValidAssistTarget(IEntity *pEntity, IEntity *pSelf, bool includeVehicles=false);

	void AdvancedAssistAiming(float range, const Vec3& pos, Vec3 &dir);

	bool		IsSilencerAttached() { return m_silencerAttached; }

	void		StartChangeFireMode();
	void		EndChangeFireMode();
	bool    IsSwitchingFireMode() { return m_switchingFireMode; };

	//Targeting stuff
	bool		IsTargetOn() { return m_targetOn; }
	void		ActivateTarget(bool activate) { m_targetOn = activate; }
	void		SetAimLocation(Vec3 &location) { m_aimPosition = location; }
	void		SetTargetLocation(Vec3 &location) { m_targetPosition = location; }
	Vec3&   GetAimLocation(){ return m_aimPosition; }
	Vec3&		GetTargetLocation() { return m_targetPosition; }

	//Raise weapon
	ILINE virtual	bool		IsWeaponRaised() const { return (m_weaponRaised); }
	virtual	float		GetRaiseDistance();
	ILINE virtual void    SetWeaponRaised(bool raise) {m_weaponRaised = raise;}

	virtual bool		CanBeRaised();
	virtual void    UpdateWeaponRaising(float frameTime);
	virtual void    UpdateWeaponLowering(float frameTime);

	ILINE virtual void		LowerWeapon(bool lower) { m_weaponLowered = lower; }
	ILINE virtual bool		IsWeaponLowered() { return m_weaponLowered; }
	ILINE virtual bool    IsPendingFireRequest() { return m_requestedFire; }

	bool		GetFireAlternation() { return m_fire_alternation;}
	void		SetFireAlternation(bool fireAlt) { m_fire_alternation = fireAlt;}

	//LAW special stuff
	virtual	void		AutoDrop() {};
	virtual void    AddFiredRocket() {};

	//AI using dual wield socoms
	bool    FireSlave(EntityId actorId, bool fire);
	void    ReloadSlave();

	virtual EntityId	GetHeldEntityId() const { return 0; }

	virtual void SendMusicLogicEvent(EMusicLogicEvents event);

	//Scopes
	void    OnZoomIn();
	void    OnZoomOut();
	bool    GetScopePosition(Vec3& pos);

	virtual CWeaponSharedParams*  GetWeaponSharedParams() { return m_weaponsharedparams; }

	// network
	struct WeaponRaiseParams
	{
		WeaponRaiseParams() : raise(false) {}
		WeaponRaiseParams(bool _raise) : raise(_raise) {}

		bool raise;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("raise", raise);
		}
	};

	struct SvRequestShootParams
	{
		SvRequestShootParams() {};
		SvRequestShootParams(const Vec3& pos_, const Vec3& dir_, const Vec3 &at, int ph) : pos(pos_), dir(dir_), hit(at), predictionHandle(ph) {};

		Vec3 pos;
		Vec3 dir;
		Vec3 hit;
		int predictionHandle;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("pos", pos, 'wrld');
			ser.Value("dir", dir, 'dir3');
			ser.Value("hit", hit, 'sHit');
			ser.Value("predictionHandle", predictionHandle, 'phdl');
		};
	};

	struct ClShootParams
	{
		ClShootParams() {};
		ClShootParams(const Vec3 &at, int ph): hit(at), predictionHandle(ph) {};

		Vec3 hit;
		int predictionHandle;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("hit", hit, 'sHit');
			ser.Value("predictionHandle", predictionHandle, 'phdl');
		};
	};

	struct ClShootXParams
	{
		ClShootXParams() {};
		ClShootXParams(EntityId eid_, bool hit0_, const Vec3& hit_, int ph) : eid(eid_), hit0(hit0_), hit(hit_), predictionHandle(ph) {};

		EntityId eid;
		bool hit0;
		Vec3 hit;
		int predictionHandle;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("ref_eid", eid, 'eid');
			ser.Value("use_hit", hit0, 'bool');
			ser.Value("ref_hit", hit, hit0 ? 'hit0' : 'hit1');
			ser.Value("predictionHandle", predictionHandle, 'phdl');
		}
	};

	struct SvRequestShootExParams
	{
		SvRequestShootExParams() {};
		SvRequestShootExParams(const Vec3 &_pos, const Vec3 &_dir, const Vec3 &_vel, const Vec3 &_hit, float _extra, int ph)
		: pos(_pos), dir(_dir), vel(_vel), hit(_hit), extra(_extra), predictionHandle(ph) {};

		Vec3 pos;
		Vec3 dir;
		Vec3 vel;
		Vec3 hit;
		float extra;
		int predictionHandle;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("pos", pos, 'wrld');
			ser.Value("dir", dir, 'dir3');
			ser.Value("vel", vel, 'vel0');
			ser.Value("hit", hit, 'wrld');
			ser.Value("extra", extra, 'smal');
			ser.Value("predictionHandle", predictionHandle, 'phdl');
		};
	};

	struct SvRequestFireModeParams
	{
		SvRequestFireModeParams(): id(0) {};
		SvRequestFireModeParams(int fmId): id(fmId) {};

		int id;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("id", id, 'fmod');
		};
	};

	struct RequestMeleeAttackParams
	{
		RequestMeleeAttackParams() {};
		RequestMeleeAttackParams(bool _wmelee, const Vec3 &_pos, const Vec3 &_dir): wmelee(_wmelee), pos(_pos), dir(_dir) {};

		bool wmelee; // is this the special weapon melee mode?
		Vec3 pos;
		Vec3 dir;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("wmelee", wmelee, 'bool');
			ser.Value("pos", pos, 'wrld');
			ser.Value("dir", dir, 'dir3');
		}
	};

	struct RequestStartMeleeAttackParams
	{
		RequestStartMeleeAttackParams() {};
		RequestStartMeleeAttackParams(bool _wmelee): wmelee(_wmelee) {};

		bool wmelee; // is this the special weapon melee mode?

		void SerializeWith(TSerialize ser)
		{
			ser.Value("wmelee", wmelee, 'bool');
		}
	};

	struct ClSetFireModeParams
	{
		ClSetFireModeParams(): id(0) {};
		ClSetFireModeParams(int fmId): id(fmId) {};

		int id;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("id", id, 'fmod');
		};
	};

	struct EmptyParams
	{
		EmptyParams() {};
		void SerializeWith(TSerialize ser) {}
	};

	struct LockParams
	{
		LockParams(): entityId(0), partId(0) {};
		LockParams(EntityId id, int part): entityId(id), partId(part) {};

		EntityId entityId;
		int      partId;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("entityId", entityId, 'eid');
			ser.Value("partId", partId);
		}
	};

	struct ZoomParams
	{
		ZoomParams(): fov(0) {};
		ZoomParams(float _fov): fov(_fov) {};

		float fov;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("fov", fov, 'frad');
		}
	};


	static const EEntityAspects ASPECT_FIREMODE = eEA_GameServerStatic;
	static const EEntityAspects ASPECT_RAISED = eEA_GameClientDynamic;
	static const EEntityAspects ASPECT_AMMO = eEA_GameServerDynamic;

	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestShoot, SvRequestShootParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestShootEx, SvRequestShootExParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClShoot, ClShootParams, eNRT_UnreliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClShootX, ClShootXParams, eNRT_UnreliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestStartFire, EmptyParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestStopFire, EmptyParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClStartFire, EmptyParams, eNRT_UnreliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClStopFire, EmptyParams, eNRT_UnreliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestStartMeleeAttack, RequestStartMeleeAttackParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClStartMeleeAttack, RequestStartMeleeAttackParams, eNRT_UnreliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestMeleeAttack, RequestMeleeAttackParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClMeleeAttack, RequestMeleeAttackParams, eNRT_UnreliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestZoom, ZoomParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClZoom, ZoomParams, eNRT_UnreliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestFireMode, SvRequestFireModeParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_POSTATTACH(ClSetFireMode, ClSetFireModeParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestReload, EmptyParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClReload, EmptyParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_POSTATTACH(ClEndReload, EmptyParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH_FAST(SvRequestCancelReload, EmptyParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH_FAST(ClCancelReload, EmptyParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClLock, LockParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClUnlock, EmptyParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestLock, LockParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestUnlock, EmptyParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestWeaponRaised, WeaponRaiseParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClWeaponRaised, WeaponRaiseParams, eNRT_ReliableUnordered);

	virtual int NetGetCurrentAmmoCount() const;
	virtual void NetSetCurrentAmmoCount(int count);

	virtual void NetShoot(const Vec3 &hit, int predictionHandle);
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle);
	
	virtual void NetStartFire();
	virtual void NetStopFire();

	virtual void NetStartMeleeAttack(bool weaponMelee);
	virtual void NetMeleeAttack(bool weaponMelee, const Vec3 &pos, const Vec3 &dir);

	virtual void NetZoom(float fov);

	void SendEndReload();
	void RequestShoot(IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle, bool forceExtended);
	void RequestStartFire();
	void RequestStopFire();
	void RequestReload();
	void RequestFireMode(int fmId);
	void RequestWeaponRaised(bool raise);

	void RequestStartMeleeAttack(bool weaponMelee);
	void RequestMeleeAttack(bool weaponMelee, const Vec3 &pos, const Vec3 &dir);
	void RequestZoom(float fov);

	void RequestCancelReload();
	void RequestLock(EntityId id, int partId = 0);
	void RequestUnlock();
	
	bool IsServerSpawn(IEntityClass* pAmmoType) const;
	CProjectile *SpawnAmmo(IEntityClass* pAmmoType, bool remote=false);

	bool	AIUseEyeOffset() const;
  bool	AIUseOverrideOffset(EStance stance, float lean, float peekOver, Vec3& offset) const;

  virtual bool ApplyActorRecoil() const { return true; }

	virtual void ForcePendingActions(uint8 blockedActions = 0);

	SWeaponAmmo GetFirstAmmo();
	SWeaponAmmo GetNextAmmo();

protected:
	virtual bool ReadItemParams(const IItemParamsNode *params);
	const IItemParamsNode *GetFireModeParams(const char *name);
	const IItemParamsNode *GetZoomModeParams(const char *name);
	void InitFireModes(const IItemParamsNode *firemodes);
	void InitZoomModes(const IItemParamsNode *zoommodes);
	void InitAmmos(const IItemParamsNode *ammo);
	void InitAIData(const IItemParamsNode *aiDescriptor);
	void InitAIOffsets(const IItemParamsNode *aiOffsetData);

	EntityId	GetLAMAttachment();
	EntityId  GetFlashlightAttachment();

	void SetNextShotTime(bool activate);

	IFireMode					*m_fm;

	IFireMode					*m_melee;

	IZoomMode					*m_zm;
	int								m_zmId;

	TFireModeIdMap		m_fmIds;
	TFireModeVector		m_firemodes;

	TZoomModeIdMap		m_zmIds;
	TZoomModeVector		m_zoommodes;

	TAmmoMap					m_ammo;
	TAmmoMap					m_bonusammo;
	TAmmoMap					m_accessoryAmmo;
	TAmmoMap					m_minDroppedAmmo; //SP only (AI drops always a minimum amount)

	bool							m_fire_alternation;

	bool							m_restartZoom; //this is a serialization helper
	int								m_restartZoomStep;

	const IItemParamsNode	*m_fmDefaults;
	const IItemParamsNode *m_zmDefaults;
	const IItemParamsNode	*m_xmlparams;

	TEventListenerVector	m_listeners;
	static TEventListenerVector * m_listenerCache;
	static bool m_listenerCacheInUse;
	IWeaponFiringLocator	*m_pFiringLocator;

	_smart_ptr<class CWeaponSharedParams> m_weaponsharedparams;

	//Just needed for the current weapon of the client
	static SWeaponCrosshairStats	s_crosshairstats; 

	static float	s_dofValue;
	static float	s_dofSpeed;
	static float	s_focusValue;

  Vec3 m_destination;

	bool					m_silencerAttached;

	bool					m_targetOn;
	Vec3					m_aimPosition;
	Vec3					m_targetPosition;

	bool					m_weaponRaised;
	bool					m_weaponLowered;
	float         m_raiseProbability;
	uint8					m_raisePose;

	bool					m_switchingFireMode;
	bool          m_switchLeverLayers;

	float         m_nextShotTime;
	TAmmoMap::const_iterator m_itAmmoMap;

private:

	static TActionHandler<CWeapon>	s_actionHandler;

	void  RegisterActions();

	bool OnActionAttack(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionReload(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSpecial(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionModify(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionFiremode(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionZoomIn(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionZoomOut(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionZoom(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionZoomXI(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	bool PreActionAttack(bool startFire);
	void RestorePlayerSprintingStats();
	void CacheRaisePose();

	//Flags for force input states (make weapon more responsive)
	bool m_requestedFire;

	ILINE void ClearInputFlags() {m_requestedFire=false;}
};


#endif //__WEAPON_H__
