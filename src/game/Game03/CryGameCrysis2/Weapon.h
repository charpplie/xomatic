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

#define CHECK_OWNER_REQUEST()	\
	{ \
	uint16 channelId=m_pGameFramework->GetGameChannelId(pNetChannel);	\
	IActor *pOwnerActor=GetOwnerActor(); \
	if (pOwnerActor && pOwnerActor->GetChannelId()!=channelId && !IsDemoPlayback()) \
	return true; \
	}

class CFireMode;
class CProjectile;
class CWeaponSharedParams;
struct SParams_WeaponFPAiming;

class CWeapon :
	public CItem,
	public IWeapon
{
	class ScheduleLayer_Leave;
	class ScheduleLayer_Enter;
	struct PlayLeverLayer;

	typedef CItem	BaseClass;

protected:
	typedef std::map<string, int>								TFireModeIdMap;
	typedef std::vector<CFireMode *>						TFireModeVector;
	typedef std::map<string, int>								TZoomModeIdMap;
	typedef std::vector<IZoomMode *>						TZoomModeVector;

	typedef CListenerSet<IWeaponEventListener*>	TWeaponEventListeners;

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
	};

public:

	enum EWeaponActions
	{
		eWeaponAction_None	 = BIT(0),
		eWeaponAction_Fire	 = BIT(1),
		eWeaponAction_Zoom	 = BIT(2),
		eWeaponAction_Reload = BIT(3),
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
	virtual void ProcessEvent(SEntityEvent& event);
	virtual void SetChannelId(uint16 id) {};
	virtual void GetMemoryUsage(ICrySizer * s) const;
	void GetInternalMemoryUsage(ICrySizer * s) const;
	virtual void Reset();
	virtual bool ResetParams();
	virtual void PreResetParams();

	virtual void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	virtual void UpdateFPView(float frameTime);

	virtual void ResetActionSuffix(float blendTime = -1.0f);

	virtual IWeapon *GetIWeapon() { return this; };

	virtual void MeleeAttack();
	virtual bool CanMeleeAttack() const;
	virtual IFireMode *GetMeleeFireMode() const { return m_melee; };

	virtual void PerformThrow(const SProjectileThrowParams& throwParams, IActor* pActor = NULL) {}

  virtual void Select(bool select);
	virtual void Drop(float impulseScale, bool selectNext = true, bool byDeath = false);

	virtual bool CanDrop() const;

	virtual void OnPickedUp(EntityId actorId, bool destroyed);
	virtual void OnDropped(EntityId actorId);

  virtual void OnDestroyed();
	virtual void EnterWater(bool enter) {};

	//Needed for the mounted weapon
	virtual void StartUse(EntityId userId); 
	virtual void StopUse(EntityId userId); 

	virtual bool CheckAmmoRestrictions(EntityId pickerId);

	virtual bool FilterView(SViewParams &viewParams);
	virtual void PostFilterView(struct SViewParams &viewParams);

	// ~IItem
	virtual bool HasAttachmentAtHelper(const char *helper);
	virtual void GetAttachmentsAtHelper(const char *helper, CCryFixedStringListT<5, 30> &attachments);
	virtual bool IsAttachmentEquipped(const char *attachmentName) const;

	virtual	uint32 StartDeselection();
	bool IsDeselecting() const {return m_isDeselecting;}

	virtual void PickUpAmmo(EntityId pickerId);
	virtual bool HasSomeAmmoToPickUp(EntityId pickerId) const;
	virtual ColorF GetSilhouetteColor() const;

	// Events
	virtual void OnShoot(EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType, 
		const Vec3 &pos, const Vec3 &dir, const Vec3 &vel);
	virtual void OnStartFire(EntityId shooterId);
	virtual void OnStopFire(EntityId shooterId);
	virtual void OnFireModeChanged(int currentFireMode);
	virtual void OnStartReload(EntityId shooterId, IEntityClass* pAmmoType);
	virtual void OnEndReload(EntityId shooterId, IEntityClass* pAmmoType);
	virtual void OnSetAmmoCount(EntityId shooterId);
	virtual void OnOutOfAmmo(IEntityClass* pAmmoType);
	virtual void OnReadyToFire();
	virtual void OnMelee(EntityId shooterId);
	virtual void OnStartTargetting(IWeapon *pWeapon);
	virtual void OnStopTargetting(IWeapon *pWeapon);
	virtual void OnSelected(bool selected);
	virtual void OnProjectileCollided(EntityId projectileId, IPhysicalEntity* pCollider, const Vec3& pos) {};
	virtual void OnEnterFirstPerson();
	virtual void OnEnterThirdPerson();
	void OnFireWhenOutOfAmmo();

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
	virtual bool CanModify() const;
	virtual void StartSprint(CActor* pOwnerActor);

	virtual bool IsReloading() const;
	
	virtual void MountAt(const Vec3 &pos);
	virtual void MountAtEntity(EntityId entityId, const Vec3 &pos, const Ang3 &angles);

	virtual void Reload(bool force=false);
	virtual bool CanReload() const;

	virtual bool OutOfAmmo(bool allFireModes) const;
	virtual bool LowAmmo(float thresholdPerCent) const;

	virtual int GetAmmoCount(IEntityClass* pAmmoType) const;
	virtual void SetAmmoCount(IEntityClass* pAmmoType, int count);
	bool CanPickUpAmmo(IInventory* pDestinationInventory);

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
	bool				 IsFiremodeDisabledByAccessory(int idx) const;
	virtual void FixAccessories(const SAccessoryParams *newParams, bool attach);

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

	virtual float GetSpinUpTime() const;
	virtual float GetSpinDownTime() const;

	virtual void SetHostId(EntityId hostId);
	virtual EntityId GetHostId() const;

	virtual bool	PredictProjectileHit(IPhysicalEntity *pShooter, const Vec3 &pos, const Vec3 &dir,
		const Vec3 &velocity, float speed, Vec3& predictedPosOut, float& projectileSpeedOut,
		Vec3* pTrajectory = 0, unsigned int* trajectorySizeInOut = 0) const;

	virtual const AIWeaponDescriptor& GetAIWeaponDescriptor( ) const;

	virtual bool Query(EWeaponQuery query, const void* param = NULL);

	virtual bool    IsPendingFireRequest() { return IsInputFlagSet(eWeaponAction_Fire); };

	// ~IWeapon

	virtual	void		RaiseWeapon(bool raise, bool faster = false, bool force=false);
	ILINE virtual uint8 GetRaisePose() { return m_raisePose; }

	bool	 IsZoomingIn() const;
	bool	 ShouldSnapToTarget() const;
	float  GetZoomInTime() const;

	int  GetMaxZoomSteps();
	bool IsValidAssistTarget(IEntity *pEntity, IEntity *pSelf, bool includeVehicles=false);
	virtual void AssistAiming(float magnification=1.0f, bool accurate=false) {}
	virtual void AdvancedAssistAiming(float range, const Vec3& pos, Vec3 &dir) {}

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
	bool	IsWeaponRaised() const { return (m_weaponRaised); }
	void	SetWeaponRaised(bool raise) {m_weaponRaised = raise;}

	virtual bool		CanBeRaised();

	ILINE virtual void		LowerWeapon(bool lower) { m_weaponLowered = lower; }
	ILINE virtual bool		IsWeaponLowered() { return m_weaponLowered; }

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

	//Zoom in/out events
	virtual void OnZoomIn();
	virtual void OnZoomOut();
	void OnZoomedIn();
	void OnZoomedOut();
	bool    GetScopePosition(Vec3& pos);

	virtual bool	UpdateAimAnims(SParams_WeaponFPAiming &aimAnimParams, bool &releaseCameraBone);
	virtual float GetMovementModifier() const;
	virtual float GetRotationModifier() const;
	float GetCoverAndLeanModifier() const;

	virtual CWeaponSharedParams*  GetWeaponSharedParams() { return m_weaponsharedparams; }

	virtual bool ShouldSendOnShootHUDEvent() const;

	void ShowDebugInfo();

	// network
	enum ENetReloadState
	{
		eNRS_NoReload,
		eNRS_StartReload,
		eNRS_EndReload,
		eNRS_CancelReload
	};

	struct SNetWeaponData
	{
		uint8	m_firemode;
		uint8 m_reload;
		bool	m_raised;
		bool	m_isFiring;
		bool  m_zoomState;
		int		m_weapon_ammo;
		int		m_inventory_ammo;
		uint8 m_fireCounter;
		uint8 m_meleeCounter;
		uint8 m_expended_ammo;

		void NetSerialise(TSerialize ser);
	};

	
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

	struct ZoomStateParams
	{
		ZoomStateParams() : zoomed(false) {}
		ZoomStateParams(bool _zoomed) : zoomed(_zoomed) {}

		bool zoomed;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("zoomed", zoomed);
		}
	};

	struct SvRequestShootParams
	{
		SvRequestShootParams() {};
		SvRequestShootParams(const Vec3 &at, int ph) : hit(at), predictionHandle(ph) {};

		Vec3 hit;
		int predictionHandle;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("hit", hit, 'sHit');
			ser.Value("predictionHandle", predictionHandle, 'phdl');
		};
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
		void SerializeWith(const TSerialize& ser) {}
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

	struct DefaultParams
	{
		void SerializeWith(const TSerialize& ser) {};
	};


	static const EEntityAspects ASPECT_FIREMODE = eEA_GameServerStatic;
	static const EEntityAspects ASPECT_RAISED = eEA_GameServerDynamic;
	static const EEntityAspects ASPECT_AMMO = eEA_GameServerDynamic;
	static const EEntityAspects ASPECT_RELOAD = eEA_GameServerDynamic;
	static const EEntityAspects ASPECT_STREAM = eEA_GameServerDynamic;


	DECLARE_SERVER_RMI_NOATTACH(SvRequestShoot, SvRequestShootParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestShootEx, SvRequestShootExParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestStartFire, DefaultParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestStopFire, DefaultParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestStartMeleeAttack, DefaultParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestFireMode, SvRequestFireModeParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestReload, DefaultParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestCancelReload, DefaultParams, eNRT_ReliableOrdered);

	DECLARE_CLIENT_RMI_NOATTACH(ClLock, LockParams, eNRT_ReliableOrdered);
	DECLARE_CLIENT_RMI_NOATTACH(ClUnlock, EmptyParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestLock, LockParams, eNRT_ReliableOrdered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestUnlock, EmptyParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestWeaponRaised, WeaponRaiseParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestSetZoomState, ZoomStateParams, eNRT_ReliableOrdered);

	virtual int		NetGetCurrentAmmoCount() const;
	virtual void	NetSetCurrentAmmoCount(int count);

	virtual bool NetGetWeaponRaised() const;
	virtual void NetSetWeaponRaised(bool raise);

	virtual int		GetReloadState() const;
	virtual void	SvSetReloadState(int state);
	virtual void	ClSetReloadState(int state);
	virtual void	SvCancelReload();

	virtual void	NetStateSent();
	virtual void	NetUpdateFireMode();
	virtual bool	NetAllowUpdate();

	virtual void NetShoot(const Vec3 &hit, int predictionHandle);
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle);
	
	virtual void NetStartFire();
	virtual void NetStopFire();

	virtual void NetStartMeleeAttack();

	virtual void NetZoom(float fov);
	virtual void NetSetIsFiring(bool isFiring);

	void SendEndReload();
	void RequestShoot(IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle, bool forceExtended);
	void RequestStartFire();
	void RequestStopFire();
	void RequestReload();
	void RequestFireMode(int fmId);
	void RequestWeaponRaised(bool raise);
	void RequestSetZoomState(bool zoomed);

	void RequestStartMeleeAttack(bool weaponMelee);
	void RequestMeleeAttack(bool weaponMelee, const Vec3 &pos, const Vec3 &dir) {}
	
	void RequestCancelReload();
	void RequestLock(EntityId id, int partId = 0);
	void RequestUnlock();
	virtual void RequestDetonate() {};
	virtual void UpdateMovementIdles(bool sprinting, float runFactor) {};
	
	// PROTOTYPE (need for PhGun)
	virtual void RequestEntityDelegation(EntityId id, bool authorize) {};
	//------------------------------------
	
	bool IsServerSpawn(IEntityClass* pAmmoType) const;
	CProjectile *SpawnAmmo(IEntityClass* pAmmoType, bool remote=false);

	bool	AIUseEyeOffset() const;
  bool	AIUseOverrideOffset(EStance stance, float lean, float peekOver, Vec3& offset) const;

  virtual bool ApplyActorRecoil() const { return true; }
	virtual void ApplyFPViewRecoil(int nFrameId, float recoilX, float recoilY);

	ILINE void	AddPendingReload() { SetInputFlag(eWeaponAction_Reload); }
	virtual void ForcePendingActions(uint8 blockedActions = 0);

	ILINE const TAmmoVector& GetAmmoVector() const { return m_ammo; }

	const char	*GetName();

	void  OnPlayerSuperJumped(bool superJumped);
	void  OnPlayerAirFrictionActivate(bool activate);
	bool IsOwnerClient() const;
	void SetOwnerClientOverride(bool isClient);

	virtual bool	  IsLaserActivated() const;
	ILINE bool IsInputFlagSet(uint8 actionFlag) const { return ((s_requestedActions & actionFlag) != 0); }
	void TriggerMeleeReaction();

protected:

	bool HasCompatibleAmmo(IInventory* pInventory) const;
	bool CheckAmmoRestrictionsForAccessories(EntityId pickerId) const;
	bool CheckAmmoRestrictionsForBonusAndMagazineAmmo(IInventory& inventory) const;

	virtual void InitItemFromParams();
	virtual void InitFireModes();
	void InitZoomModes();
	void InitAmmo();
	void InitAIData();
	void InitAIOffsets(const IItemParamsNode *aiOffsetData);
	void UpdateFPIdling(float frameTime);

	//Activate/Deactivate Laser and Light for the AI (also player if neccessary)
	virtual bool		IsLaserAttached() const;
	virtual bool    IsFlashlightAttached() const;
	virtual void    ActivateLaser(bool activate);
	virtual void		ActivateFlashlight(bool activate);

	bool IsOwnerSuperJumpingOrUsingAirFriction() const;

	virtual void AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event);
	
	void RegisterUsedAmmoWithInventory(IInventory* pInventory);
	void UnregisterUsedAmmoWithInventory(IInventory* pInventory);
	
	void OnDroppedByAI(IInventory* pAIInventory);
	void OnDroppedByPlayer(IInventory* pPlayerInventory);
	void OnDualdWieldWeaponDropped(IInventory* pPlayerInventory);

	void SetInventoryAmmoCountInternal(IInventory* pInventory, IEntityClass* pAmmoType, int count);

	virtual bool		IsFlashlightActivated() const;

	EntityId	GetLaserAttachment() const;
	EntityId  GetFlashlightAttachment() const;

	void SetNextShotTime(bool activate);

	const SPlayerMovementModifiers& GetPlayerMovementModifiers() const;

	ILINE void ClearInputFlags() { s_requestedActions = eWeaponAction_None;}
	ILINE void SetInputFlag(uint8 actionFlag) { s_requestedActions |= actionFlag; }
	ILINE void ClearInputFlag(uint8 actionFlag) { s_requestedActions &= ~actionFlag; }


	IFireMode					*m_fm;

	IFireMode					*m_melee;

	IZoomMode					*m_zm;
	int								m_zmId;

	TFireModeIdMap		m_fmIds;
	TFireModeVector		m_firemodes;

	TZoomModeIdMap		m_zmIds;
	TZoomModeVector		m_zoommodes;

	TAmmoVector					m_ammo;
	TAmmoVector					m_bonusammo;

	bool							m_fire_alternation;

	bool							m_restartZoom; //this is a serialization helper
	int								m_restartZoomStep;

	TWeaponEventListeners	m_listeners;

	IWeaponFiringLocator	*m_pFiringLocator;

	CWeaponSharedParams* m_weaponsharedparams;

	//Just needed for the current weapon of the client
	static SWeaponCrosshairStats	s_crosshairstats; 

	static float	s_dofValue;
	static float	s_dofSpeed;
	static float	s_focusValue;

  Vec3	m_destination;
	Vec3	m_aimPosition;
	Vec3	m_targetPosition;

	bool	m_forcingRaise;
	bool	m_targetOn;

	bool	m_weaponRaised;
	bool	m_weaponLowered;
	uint8	m_raisePose;

	bool	m_switchingFireMode;
	bool  m_switchLeverLayers;
	bool	m_doingMagazineSwap;

	float m_nextShotTime;
	float	m_zoomedTime;

	int		m_lastRecoilUpdate;

	float	m_delayedFireActionTimeOut;
	float	m_delayedZoomActionTimeOut;
	bool	m_delayedZoomStayZoomedVal;

	bool	m_isClientOwnerOverride;
	bool	m_minDropAmmoAvailable;
	bool  m_isRegisteredAmmoWithInventory;

	// network
	int		m_reloadState;
	int		m_firemode;
	int		m_shootCounter;	// num bullets to shoot
	int		m_lastRecvInventoryAmmo;
	bool	m_isFiring;
	bool	m_isFiringStarted;
	uint8	m_fireCounter;	// total that have been fired
	uint8 m_expended_ammo;
	uint8 m_meleeCounter;
	bool	m_doMelee;
	bool	m_netInitialised;
	bool	m_isDeselecting;
	
private:

	static TActionHandler<CWeapon>	s_actionHandler;

	void  RegisterActions();

	virtual bool OnActionAttackPrimary(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	virtual bool OnActionAttackSecondary(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	virtual bool OnActionFiremode(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSprint(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionReload(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSpecial(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionModify(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionZoomIn(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionZoomOut(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionZoom(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionZoomXI(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	bool PreActionAttack(bool startFire);
	void CacheRaisePose();

	void AutoSelectNextItem();

	//Flags for force input states (make weapon more responsive)
	//Static: It's client only, and this way they can be remembered through weapon switches
	static uint8 s_requestedActions;
};

class CSimpleFiringLocator : public IWeaponFiringLocator
{
public:
	virtual bool GetProbableHit(EntityId weaponId, const IFireMode* pFireMode, Vec3& hit)
	{
		return false;
	}
	virtual bool GetFiringPos(EntityId weaponId, const IFireMode* pFireMode, Vec3& pos)
	{
		pos=m_pos;
		return true;
	}
	virtual bool GetFiringDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos)
	{
		dir=m_dir;
		return true;
	}
	virtual bool GetActualWeaponDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos)
	{
		dir=m_dir;
		return true;
	}
	virtual bool GetFiringVelocity(EntityId weaponId, const IFireMode* pFireMode, Vec3& vel, const Vec3& firingDir)
	{
		return false;
	}
	virtual void WeaponReleased()
	{
		delete this;
	}

	Vec3 m_dir;
	Vec3 m_pos;
};

#endif //__WEAPON_H__
