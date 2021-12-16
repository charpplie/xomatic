/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: C++ Item Implementation
  
 -------------------------------------------------------------------------
  History:
  - 27:10:2004   11:25 : Created by Márcio Martins

*************************************************************************/
#ifndef __ITEM_H__
#define __ITEM_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IScriptSystem.h>
#include <IGameFramework.h>
#include <IItemSystem.h>
#include <IActionMapManager.h>
#include <CryCharAnimationParams.h>
#include <ISound.h>
#include <map>
#include <list>


#include "ItemScheduler.h"
#include "ItemString.h"
#include "ItemDefinitions.h"
#include "NanoSuitDefs.h"
#include "ActorRef.h"

#define ITEM_ARMS_ATTACHMENT_NAME	"arms_fp"

#define ITEM_DESELECT_POSE			"nw"

#define ITEM_OWNER_ACTION_LAYER		WeaponAnimLayerID::Total

class CItemSharedParams;

struct ICharacterInstance;
struct AnimEventInstance;
struct IAttachmentObject;
struct SParams;
struct SMountParams;

enum EItemUpdateSlots
{
  eIUS_General = 0,
	eIUS_Zooming = 1,
	eIUS_FireMode = 2,  
	eIUS_Scheduler = 3,  
};

class CActor;
class CItem :
	public CGameObjectExtensionHelper<CItem, IItem, 28>,
	public IGameObjectProfileManager
{
	friend class CScriptBind_Item;
public:
	struct AttachAction;
	struct DetachAction;
	struct SelectAction;
	struct SwitchHandAction;

	//------------------------------------------------------------------------
	// Typedefs
	//------------------------------------------------------------------------
	enum ePhysicalization
	{
		eIPhys_PhysicalizedRigid,
		eIPhys_PhysicalizedStatic,
		//eIPhys_DemoRecording,
		eIPhys_NotPhysicalized,
	};

	enum ETimer
	{
		eIT_Flying = 0,
		eIT_Last,
	};

	enum ePlayActionFlags
	{
		eIPAF_FirstPerson						= 1 << eIGS_FirstPerson,
		eIPAF_ThirdPerson						= 1 << eIGS_ThirdPerson,
		eIPAF_Owner									= 1 << eIGS_Owner,
		eIPAF_OwnerAnimGraph				= 1 << eIGS_OwnerAnimGraph,
		eIPAF_OwnerAnimGraphLooped	= 1 << eIGS_OwnerAnimGraphLooped,
		eIPAF_NoBlend								= 1 << 15,
		eIPAF_CleanBlending					= 1 << 16,
		eIPAF_Animation							= 1 << 17,
		eIPAF_Sound									= 1 << 18,
	  eIPAF_SoundLooped						= 1 << 19,
		eIPAF_SoundStartPaused			= 1 << 20,
		eIPAF_RestartAnimation			= 1 << 21,
		eIPAF_RepeatLastFrame				= 1 << 22,
		eIPAF_Shoot									= 1 << 23,
		eIPAF_EndCurrentWeaponAnim	= 1 << 24,
		eIPAF_ProceduralRecoil			= 1 << 25, // use procedural recoil instead of animation

		eIPAF_Default								= eIPAF_FirstPerson|eIPAF_ThirdPerson|eIPAF_Owner|eIPAF_OwnerAnimGraph|eIPAF_OwnerAnimGraphLooped|eIPAF_Sound|eIPAF_Animation,
	};

	enum eViewMode
	{
		eIVM_FirstPerson = 1,
		eIVM_ThirdPerson = 2,
	};

	enum eScriptEventTable
	{
		eISET_Server				= 1<<1,
		eISET_Client				= 1<<2,
		eISET_Root					= 1<<3,
		eISET_All						= eISET_Client|eISET_Server|eISET_Root,
		eISET_ClientServer	= eISET_Client|eISET_Server
	};

	enum eItemBackAttachment
	{
		eIBA_Primary = 0,
		eIBA_Secondary,
		eIBA_Unknown
	};

	typedef CryFixedStringT<16> TempAGInputName;

	struct SStats
	{
		SStats()
		:	fp(false),
			mounted(false),
			mount_last_aimdir(Vec3(0.0f,0.0f,0.0f)),
			mount_dir(Vec3(0.0f,1.0f,0.0f)),      
			pickable(true),
			selectable(true),
			selected(false),
			dropped(false),
			brandnew(true),      
			flying(false),      
			viewmode(0),
      used(false),
			sound_enabled(true),
			hand(eIH_Right),
      health(0.f),
			first_selection(true),
			backAttachment(eIBA_Unknown)
		{
		};

		void Serialize(TSerialize &ser)
		{
			ser.BeginGroup("ItemStats");
			//int vmode = viewmode;
			//ser.Value("viewmode", vmode);
			bool fly = flying;
			ser.Value("flying", fly);
			bool sel = selected;
			ser.Value("selected", sel);
			bool mount = mounted;
			ser.Value("mounted", mount);
			bool use = used;
			ser.Value("used", use);
			int han = hand;
			ser.Value("hand", han);
			bool drop = dropped;
			ser.Value("dropped", drop);
			//bool firstPerson = fp;
			//ser.Value("firstPerson", firstPerson);
			bool bnew = brandnew;
			ser.Value("brandnew", bnew);      
      float fHealth = health;
      ser.Value("health", fHealth);   
			bool bfirstSelection = first_selection;
			ser.Value("first_selection", bfirstSelection);
			int aback = (int)backAttachment;
			ser.Value("backAttachment", aback);
			bool pick = pickable;
			ser.Value("pickable", pick);
			ser.EndGroup();

			if(ser.IsReading())
			{
				//viewmode = vmode;
				flying = fly;
				selected = sel;
				mounted = mount;
				used = use;
				hand = han;
				dropped = drop;
				brandnew = bnew;        
				//fp = firstPerson;
        health = fHealth;
				first_selection = bfirstSelection;
				backAttachment = (eItemBackAttachment)aback;
				pickable = pick;
			}
		}

		Vec3	mount_dir;
		Vec3	mount_last_aimdir;
		int		hand:3;
		int		viewmode:3;
		float health;
		bool	fp:1;
		bool	mounted:1;
		bool	pickable:1;
		bool	selectable:1;
		bool	selected:1;
		bool	dropped:1;
		bool	brandnew:1;    
		bool	flying:1;
		bool	used:1;
		bool	sound_enabled:1;
		bool  first_selection:1;
		eItemBackAttachment   backAttachment;
	};

	struct SEditorStats
	{
		SEditorStats()
		:	ownerId(0),
			current(false)
		{};

		SEditorStats(EntityId _ownerId, bool _current)
		:	ownerId(_ownerId),
			current(_current)
		{};


		EntityId	ownerId;
		bool			current;
	};

	struct SEntityProperties
	{
		SEntityProperties()
			: mounted_min_pitch(-30.0f)
			, mounted_max_pitch(30.0f)
			, mounted_yaw_range(60.0f)
			, hitpoints(0)
			, pickable(true)
			, mounted(false)
			, physics(false)
			, usable(false) 
		{ 
			initialSetup.resize(0);
		};

		float		mounted_min_pitch;
		float		mounted_max_pitch;
		float		mounted_yaw_range;

		int hitpoints;
		bool pickable;
		bool mounted;
		bool physics;
		bool usable;
		string initialSetup;
		
		void GetMemoryStatistics(ICrySizer * s) const
		{
			s->Add(initialSetup);
		}
	};

	struct SRespawnProperties
	{
		SRespawnProperties(): timer(0.0f), unique(false), respawn(false) {};
		float timer;
		bool	unique;
		bool	respawn;
	};

	struct SCameraAnimationStats
	{
		SCameraAnimationStats()
		{
			animating=false;
			position=true;
			rotation=true;
			follow=false;
			reorient=false;
			pos.zero();
			rot.SetIdentity();
		};

		void GetMemoryStatistics(ICrySizer * s) const
		{
			s->AddObject(helper);
		}

		ItemString	helper;
		Vec3		pos;
		Quat		rot;
		bool		animating ;
		bool		position  ;
		bool		rotation  ;
		bool		follow    ;
		bool		reorient  ;
	};

	struct SInstanceAudio
	{
		SInstanceAudio(): id(INVALID_SOUNDID),synch(false) {};
		ItemString		static_name;
		tSoundID	    id;
		bool          synch;

		void GetMemoryStatistics(ICrySizer * s) const
		{
			s->Add(static_name);
		}
	};

	struct SInstanceAction
	{
		SInstanceAudio sound[2];

		void GetMemoryUsage(ICrySizer * s) const
		{
			for (int i=0; i<2; i++)
				sound[i].GetMemoryStatistics(s);
		}		
	};

	struct SEffectInfo
	{
		int			characterSlot;
		int			slot;
		ItemString	helper;

    SEffectInfo() : characterSlot(-1), slot(-1) {}
		void GetMemoryUsage(ICrySizer * s) const
		{
			s->AddObject(helper);
		}		
	};

	struct SGeometry
	{
		SGeometry(): position(ZERO), angles(ZERO), scale(1.0f) {};

		void GetMemoryStatistics(ICrySizer * s) const
		{
			s->AddObject(name);
		}
		ItemString	name;
		Vec3		position;
		Ang3		angles;
		float		scale;
	};


	typedef std::map<ItemString, SInstanceAction>	TInstanceActionMap;
	typedef std::map<ItemString, int>							TActiveLayerMap;
	typedef	std::map<uint32, SEffectInfo>					TEffectInfoMap;
	typedef std::map<ItemString, EntityId>				TAccessoryMap;
	

	static const NetworkAspectType ASPECT_OWNER_ID	= eEA_GameServerStatic;

public:
	CItem();
	virtual ~CItem();

	// IItem, IGameObjectExtension
	virtual bool Init( IGameObject * pGameObject );
	virtual void InitClient(int channelId);
	virtual void PostInitClient(int channelId);
	virtual void PostInit( IGameObject * pGameObject );
	virtual void Release();
	virtual void FullSerialize( TSerialize ser );
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
	virtual void PostSerialize();
	virtual void SerializeLTL( TSerialize ser );
	virtual void SerializeSpawnInfo( TSerialize ser ) {};
	virtual ISerializableInfoPtr GetSpawnInfo() { return 0;};
	virtual void Update( SEntityUpdateContext& ctx, int );
	virtual void PostUpdate( float frameTime ) {};
	virtual void PostRemoteSpawn() {};
	virtual void HandleEvent( const SGameObjectEvent& );
	virtual void ProcessEvent(SEntityEvent& );
	virtual void SetChannelId(uint16 id) {};
	virtual void SetAuthority(bool auth) {}
	virtual void GetMemoryUsage(ICrySizer *pSizer) const;
	void GetInternalMemoryUsage(ICrySizer *pSizer) const;
	virtual void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	virtual void UpdateFPView(float frameTime);
	virtual bool FilterView(struct SViewParams &viewParams);
	virtual void PostFilterView(struct SViewParams &viewParams);

	virtual bool CheckAmmoRestrictions(EntityId pickerId);

	virtual void EnableAnimations(bool enable) { };

	virtual IWeapon *GetIWeapon() { return 0; };
	virtual const IWeapon *GetIWeapon() const { return 0; }

	virtual bool IsAccessory() { return false; };

	virtual void SetOwnerId(EntityId ownerId);
	virtual EntityId GetOwnerId() const;

	virtual void Reset();
  virtual void ResetOwner();
	virtual bool ResetParams();
	virtual void PreResetParams() {};

	virtual void RemoveEntity(bool force = false);

	virtual void SetParentId(EntityId parentId);
	virtual EntityId GetParentId() const;

	virtual void SetHand(int hand);
	virtual void Use(EntityId userId);
	virtual void Select(bool select);
	virtual void Drop(float impulseScale=1.0f, bool selectNext=true, bool byDeath=false);
	virtual void PickUp(EntityId picker, bool sound, bool select=true, bool keepHistory=true, const char *setup = NULL);
	virtual void Physicalize(bool enable, bool rigid);
	virtual void Pickalize(bool enable, bool dropped);
	virtual void IgnoreCollision(bool ignore);
	virtual void Impulse(const Vec3 &position, const Vec3 &direction, float impulse);

	virtual bool CanPickUp(EntityId userId) const;
	virtual bool CanDrop() const;
	virtual bool CanUse(EntityId userId) const;
	virtual bool IsMountable() const;
	virtual bool IsMounted() const;
	virtual bool IsRippedOff() const { return false; }
	virtual bool IsUsed() const;
	virtual bool IsPickable() const { return IsRippedOff() ? !IsUsed() : !IsMounted(); }

	virtual bool InitRespawn();
	virtual void TriggerRespawn();
	virtual void TakeAccessories(EntityId receiver);

	virtual void Cloak(bool cloak, IMaterial *cloakMat = 0);
	virtual void SetMaterialRecursive(ICharacterInstance *charInst, bool undo, IMaterial *newMat);

	void CloakEnable(bool enable, bool fade);
	void CloakSync(bool fade);
	void ApplyMaterialLayerSettings(uint8 mask, uint32 blend);
	
	virtual int TwoHandMode() const;
	virtual bool SupportsDualWield(const char *itemName) const;
	virtual void ResetDualWield();
	virtual IItem *GetDualWieldSlave() const;
	virtual EntityId GetDualWieldSlaveId() const;
	virtual IItem *GetDualWieldMaster() const;
	virtual EntityId GetDualWieldMasterId() const;
	virtual void SetDualWieldMaster(EntityId masterId);
	virtual void SetDualWieldSlave(EntityId slaveId);
	virtual bool IsDualWield() const;
	virtual bool IsDualWieldMaster() const;
	virtual bool IsDualWieldSlave() const;

	virtual Vec3 GetMountedAngleLimits() const;
	virtual Vec3 GetMountedDir() const {return GetStats().mount_dir;}
	virtual void SetMountedAngleLimits(float min_pitch, float max_pitch, float yaw_range);

	virtual void EnableSelect(bool enable);	
	virtual bool CanSelect() const;
	virtual bool CanDeselect() const;
	virtual bool IsSelected() const;
	virtual void OnParentSelect(bool select) {};

	virtual void MountAt(const Vec3 &pos);
	virtual void MountAtEntity(EntityId entityId, const Vec3 &pos, const Ang3 &angles);
	virtual void StartUse(EntityId userId);
	virtual void StopUse(EntityId  userId);
	virtual void ApplyViewLimit(EntityId userId, bool apply);

	virtual void EnableSound(bool enable);
	virtual bool IsSoundEnabled() const;
	virtual bool IsModifying() const { return m_modifying || m_transitioning; }
  virtual bool IsDestroyed() const { return m_properties.hitpoints > 0 && m_stats.health <= 0.f; }

	virtual void EnterWater(bool enter) {};

	// ~IItem

	virtual void PickUpAmmo(EntityId pickerId) {};
	virtual bool HasSomeAmmoToPickUp(EntityId pickerId) const { return false; }
	virtual ColorF GetSilhouetteColor() const;

	virtual bool GetAnimGraphInputs(CItem::TempAGInputName &pose, CItem::TempAGInputName &suffix);

	// IGameObjectProfileManager
	virtual bool SetAspectProfile( EEntityAspects aspect, uint8 profile );
	virtual uint8 GetDefaultProfile( EEntityAspects aspect );
	// ~IGameObjectProfileManager
  
	virtual uint32 StartDeselection();

	// Events
	virtual void OnStartUsing();
	virtual void OnStopUsing();
	virtual void OnSelect(bool select);
	virtual void OnSelected(bool selected);
	virtual void OnEnterFirstPerson();
	virtual void OnEnterThirdPerson();
  virtual void OnReset();
	virtual void OnPickedUp(EntityId actorId, bool destroyed);
	virtual void OnDropped(EntityId actorId);
  
	virtual const SStats &GetStats() const { return m_stats; };
	virtual const SParams &GetParams() const;
	virtual const SMountParams* GetMountedParams() const;
	virtual const SEntityProperties &GetProperties() const { return m_properties; };

	// Silhouette purpose: we need to get all the currently attached accessories to highlight them
	const TAccessoryMap *GetAttachedAccessories() const { return &m_accessories; }

public:

	// params
	virtual bool ReadItemParams(const IItemParamsNode *root) { return true; }
	virtual void InitItemFromParams();
	virtual void InitGeometry();
	virtual void InitAccessories();
	virtual void InitDamageLevels();
	virtual void ReadProperties(IScriptTable *pScriptTable);

	// accessories
	virtual CItem *AddAccessory(const ItemString& name);
	virtual void RemoveAccessory(const ItemString& name);
	virtual void RemoveAllAccessories();
	virtual void AttachAccessory(const ItemString& name, bool attach, bool noanim, bool force = false, bool initialSetup = false);
	virtual void AttachAccessoryPlaceHolder(const ItemString& name, bool attach);
	virtual CItem *GetAccessoryPlaceHolder(const ItemString& name);
	virtual CItem *GetAccessory(const ItemString& name);
	virtual const SAccessoryParams *GetAccessoryParams(const ItemString& name) const;
	virtual bool IsAccessoryHelperFree(const ItemString& helper);
	virtual void InitialSetup();
	virtual void PatchInitialSetup(); 
	virtual void ReAttachAccessories();
	virtual void ReAttachAccessory(const ItemString& name);
	virtual void ReAttachAccessory(EntityId id);
	virtual void AccessoriesChanged();
	virtual void FixAccessories(const SAccessoryParams *newParams, bool attach) {};
	virtual void ResetAccessoriesScreen(IActor* pOwner);
	virtual void RemoveOwnerAttachedAccessories();

	virtual void SwitchAccessory(const ItemString& accessory);
	virtual void DoSwitchAccessory(const ItemString& accessory);
	virtual void ScheduleAttachAccessory(const ItemString& accessoryName, bool attach);
	virtual const char* CurrentAttachment(const ItemString& attachmentPoint);

	virtual void AddAccessoryAmmoToInventory(IEntityClass* pAmmoType, int count, CActor* pOwner = NULL);
	virtual int  GetAccessoryAmmoCount(IEntityClass* pAmmoType);
	virtual bool GivesAmmo();

	const TAccessoryMap &GetAccessories() const	{ return m_accessories; }

	// effects
	uint32 AttachEffect(int slot, uint32 id, bool attach, const char *effectName=0, const char *helper=0,
		const Vec3 &offset=Vec3Constants<float>::fVec3_Zero, const Vec3 &dir=Vec3Constants<float>::fVec3_OneY, float scale=1.0f, bool prime=true);
  uint32 AttachLight(int slot, uint32 id, bool attach, float radius=5.0f, const Vec3 &color=Vec3Constants<float>::fVec3_One,
		const float fSpecularMult=1.0f, const char *projectTexture=0, float projectFov=0, const char *helper=0,
		const Vec3 &offset=Vec3Constants<float>::fVec3_Zero, const Vec3 &dir=Vec3Constants<float>::fVec3_OneY, 
    const char* material=0, float fHDRDynamic=0.f );
	uint32 AttachLightEx(int slot, uint32 id, bool attach, bool fakeLight = false , bool castShadows = false, IRenderNode* pCasterException = NULL, float radius=5.0f, const Vec3 &color=Vec3Constants<float>::fVec3_One,
		const float fSpecularMult=1.0f, const char *projectTexture=0, float projectFov=0, const char *helper=0,
		const Vec3 &offset=Vec3Constants<float>::fVec3_Zero, const Vec3 &dir=Vec3Constants<float>::fVec3_OneY, 
		const char* material=0, float fHDRDynamic=0.f );
	void SpawnEffect(int slot, const char *effectName, const char *helper, const Vec3 &offset=Vec3Constants<float>::fVec3_Zero,
		const Vec3 &dir=Vec3Constants<float>::fVec3_OneY, float scale = 0.0f, float speed = 0.0f);
	IParticleEmitter *GetEffectEmitter(uint32 id) const;
	void SetEffectWorldTM(uint32 id, const Matrix34 &tm);
	Matrix34 GetEffectWorldTM(uint32 it);
  void EnableLight(bool enable, uint32 id);
	void SetLightRadius(float radius, uint32 id);

	// misc
	bool AttachToHand(bool attach, bool checkAttachment = false);
	bool AttachToBack(bool attach);
	void EnableUpdate(bool enable, int slot=-1);
	void RequireUpdate(int slot=-1);
	void Hide(bool hide, bool remoteUpdate = false);
	void HideArms(bool hide);
	void HideItem(bool hide);
	virtual void SetBusy(bool busy) { m_scheduler.SetBusy(busy); };
	virtual bool IsBusy() const { return m_scheduler.IsBusy(); };
	CItemScheduler *GetScheduler() { return &m_scheduler; };
	IItemSystem *GetIItemSystem() { return m_pItemSystem; };
	virtual void SetDualSlaveAccessory(bool noNetwork = false);

	void EnableEntitySystemFPCharacterUpdate(bool enable);

	IEntity *GetOwner() const;
	CActor *GetOwnerActor() const;
	CActor *GetActor(EntityId actorId) const;
  IInventory *GetActorInventory(IActor *pActor) const;
	CItem *GetActorItem(IActor *pActor) const;
	EntityId GetActorItemId(IActor *pActor) const;
  EntityId GetHostId() const { return m_hostId; }
	CActor *GetActorByNetChannel(INetChannel *pNetChannel) const;

	const char *GetDisplayName() const;

	virtual void ForcePendingActions(uint8 blockedActions = 0) {}

	//Special FP weapon render method
	virtual void RegisterFPWeaponForRenderingAlways(bool registerRenderAlways);

	// view
	bool IsOwnerFP();
	bool IsCurrentItem();
	virtual void UpdateMounted(float frameTime);  
	void CheckViewChange();
	void SetViewMode(int mode);
	void SetViewModeForShadowCharacter(int mode);
	void CopyRenderFlags(IEntity *pOwner);
  virtual void UseManualBlending(bool enable);
  virtual bool GetAimBlending(OldBlendSpace& params);
  virtual void UpdateIKMounted(IActor* pActor, const Vec3& vGunXAxis);

	// character attachments
	bool CreateCharacterAttachment(int slot, const char *name, int type, const char *bone);
	void DestroyCharacterAttachment(int slot, const char *name);
	ICharacterInstance* GetAppropriateCharacter(int slot, bool owner);
	void ResetCharacterAttachment(int slot, const char *name, bool owner);
	const char *GetCharacterAttachmentBone(int slot, const char *name);
	void SetCharacterAttachment(int slot, const char *name, IEntity *pEntity, bool owner);
	void SetCharacterAttachment(int slot, const char *name, IStatObj *pObj, bool owner);
	void SetCharacterAttachment(int slot, const char *name, ICharacterInstance *pCharacter, bool owner);
	void SetCharacterAttachment(int slot, const char *name, CDLight &light, int flags);
	void SetCharacterAttachment(int slot, const char *name, IEntity *pEntity, int objSlot, bool owner);
	virtual void SetCharacterAttachmentLocalTM(int slot, const char *name, const Matrix34 &tm);
	void SetCharacterAttachmentWorldTM(int slot, const char *name, const Matrix34 &tm);
	Matrix34 GetCharacterAttachmentLocalTM(int slot, const char *name);
	Matrix34 GetCharacterAttachmentWorldTM(int slot, const char *name);
	void HideCharacterAttachment(int slot, const char *name, bool hide);
	void HideCharacterAttachmentMaster(int slot, const char *name, bool hide);

	void CreateAttachmentHelpers(int slot);
	void DestroyAttachmentHelpers(int slot);
	const THelperVector& GetAttachmentHelpers();

	// freeze
	virtual void Freeze(bool freeze) { };

  // damage
  virtual void OnHit(float damage, const char* damageType);
  virtual void OnDestroyed();
  virtual void OnRepaired();
	virtual void DestroyedGeometry(bool use);
	virtual void UpdateDamageLevel();

	// resource
	virtual bool SetGeometry(int slot, const ItemString& name, const Vec3& poffset=Vec3(0,0,0), const Ang3& aoffset=Ang3(0,0,0), float scale=1.0f, bool forceReload=false);
	void SetDefaultIdleAnimation(int slot, const ItemString& actionName);
	const char *GetDefaultIdleAnimation(int slot) { return m_idleAnimation[slot]; };
	void ForceSkinning(bool always);
	void ForceFPSkeletonUpdate();
	void EnableHiddenSkinning(bool force);

	int FindCachedAnimationId(const ItemString& inName, ICharacterInstance* pCharacter);
	void FixSoundResourceName(const ItemString& inName, TempResourceName& fixedName);
	tSoundID PlayAction(const ItemString& action, int layer=0, bool loop=false, uint32 flags = eIPAF_Default, float speedOverride = -1.0f, float animWeigth = 1.0f, float ffeedbackWeight = 1.0f);
	void PlayAnimation(const char* animationName, int layer=0, bool loop=false, uint32 flags = eIPAF_Default);
	void PlayAnimationEx(const ItemString& animationName, int slot=eIGS_FirstPerson, int layer=0, bool loop=false, float blend=0.175f, float speed=1.0f, uint32 flags = eIPAF_Default, float overrideTime = -1.0f, float weigth = 1.0f);
	void EndItemAnimation(int slot);
	void EndActionAnimations(float blendOut = 0.1f);
	void StopActionAnimations(float blendOut = 0.1f);
	void PlayLayer(const ItemString& name, int flags = eIPAF_Default, bool record=true);
	void StopLayer(const ItemString& name, int flags = eIPAF_Default, bool record=true);
	void RestoreLayers();
	void ResetAllAnimations();
	void ResetAllSlotAnimations(eGeometrySlot slot);
	void ResetOwnerAnimationsInLayer(int layer = ITEM_OWNER_ACTION_LAYER, float blendOut = 0.1f);
	uint32 GetCurrentAnimationTime(int slot);
	void DrawSlot(int slot, bool draw, bool near=false);
	Vec3 GetSlotHelperPos(int slot, const char *helper, bool worldSpace, bool relative=false) const;
	const Matrix33 &GetSlotHelperRotation(int slot, const char *helper, bool worldSpace, bool relative=false);
	void StopSound(tSoundID id);
	void Quiet();
	ISound *GetISound(tSoundID id);
	void ReleaseStaticSound(SInstanceAudio *sound);
	void ReleaseStaticSounds();

	void SetActionSuffix(const char *suffix, const char *suffixAG, float blendTime = -1.0f) 
	{ 
		m_actionSuffix = string(suffix); 
		m_actionSuffixAG = string(suffixAG); 
		m_actionSuffixBlendTime = blendTime;
	};
	virtual void ResetActionSuffix(float blendTime = -1.0f);
	const char *GetActionSuffix() const { return m_actionSuffix.c_str(); };

	virtual void OnAttach(bool attach);

	IEntitySoundProxy *GetSoundProxy(bool create=false);
	IEntityRenderProxy *GetRenderProxy(bool create=false);
	IEntityPhysicalProxy *GetPhysicalProxy(bool create=false);
		
	EntityId NetGetOwnerId() const;
	void NetSetOwnerId(EntityId id);

	//JAW/C4 special stuff
	bool IsAutoDroppable() const;
	bool CanPickUpAutomatically() const;

	// Utils
	static void DeselectItem(EntityId itemId);

	struct RequestAttachAccessoryParams
	{
		RequestAttachAccessoryParams() {};
		RequestAttachAccessoryParams(const char *name): accessory(name) {};
		void SerializeWith(TSerialize ser)
		{
			ser.Value("accessory", accessory);
		};

		string accessory;
	};

	struct EmptyParams
	{
		void SerializeWith(const TSerialize& ser) {};
	};

	DECLARE_SERVER_RMI_NOATTACH(SvRequestAttachAccessory, RequestAttachAccessoryParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClAttachAccessory, RequestAttachAccessoryParams, eNRT_ReliableUnordered);
	
	DECLARE_SERVER_RMI_NOATTACH(SvRequestEnterModify, EmptyParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestLeaveModify, EmptyParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClEnterModify, EmptyParams, eNRT_ReliableUnordered);
	DECLARE_CLIENT_RMI_NOATTACH(ClLeaveModify, EmptyParams, eNRT_ReliableUnordered);

	// properties
	template<typename T>bool GetEntityProperty(const char *name, T &value) const
	{
		return GetEntityProperty(GetEntity(), name, value);
	}

  template<typename T>bool GetEntityProperty(IEntity* pEntity, const char *name, T &value) const
  {
    SmartScriptTable props;
    IScriptTable* pScriptTable = pEntity->GetScriptTable();
    if (pScriptTable && pScriptTable->GetValue("Properties", props))
      return props->GetValue(name, value);
    return false;
  }
  
	template<typename T>bool GetEntityProperty(const char *table, const char *name, T &value) const
	{
    return GetEntityProperty(GetEntity(), table, name, value);
	}

  template<typename T>bool GetEntityProperty(IEntity* pEntity, const char *table, const char *name, T &value) const
  {
    SmartScriptTable props;
    IScriptTable* pScriptTable = pEntity->GetScriptTable();
    if (pScriptTable && pScriptTable->GetValue("Properties", props))
    {
      SmartScriptTable subprop;
      if (props->GetValue(table, subprop))
        return subprop->GetValue(name, value);
    }
    return false;
  }

  template<typename T>void SetEntityProperty(const char *name, const T &value)
  {
    return SetEntityProperty(GetEntity(), name, value);
  }

  template<typename T>void SetEntityProperty(IEntity* pEntity, const char *name, const T &value)
  {
    SmartScriptTable props;
    IScriptTable* pScriptTable = pEntity->GetScriptTable();
    if (pScriptTable && pScriptTable->GetValue("Properties", props))
      props->SetValue(name, value);    
  }

	// script
	bool CallScriptEvent(IScriptTable *pScriptTable, const char *name)
	{
		IScriptSystem *pSS = pScriptTable->GetScriptSystem();
		if (pScriptTable->GetValueType(name) != svtFunction)
			return false;
		pSS->BeginCall(pScriptTable, name); pSS->PushFuncParam(GetEntity()->GetScriptTable());
		bool result = false;
		pSS->EndCall(result);
		return result;
	}
	template<typename P1>
		bool CallScriptEvent(IScriptTable *pScriptTable, const char *name, P1 &p1)
	{
		IScriptSystem *pSS = pScriptTable->GetScriptSystem();
		if (pScriptTable->GetValueType(name) != svtFunction)
			return false;
		pSS->BeginCall(pScriptTable, name); pSS->PushFuncParam(GetEntity()->GetScriptTable());
		pSS->PushFuncParam(p1);
		bool result = false;
		pSS->EndCall(result);
		return result;
	}
	template<typename P1, typename P2>
		bool CallScriptEvent(IScriptTable *pScriptTable, const char *name, P1 &p1, P2 &p2)
	{
		IScriptSystem *pSS = pScriptTable->GetScriptSystem();
		if (pScriptTable->GetValueType(name) != svtFunction)
			return false;
		pSS->BeginCall(pScriptTable, name); pSS->PushFuncParam(GetEntity()->GetScriptTable());
		pSS->PushFuncParam(p1); pSS->PushFuncParam(p2);
		bool result = false;
		pSS->EndCall(result);
		return result;
	}
	template<typename P1, typename P2, typename P3, typename P4>
		bool CallScriptEvent(IScriptTable *pScriptTable, const char *name, P1 &p1, P2 &p2, P3 &p3)
	{
		IScriptSystem *pSS = pScriptTable->GetScriptSystem();
		if (pScriptTable->GetValueType(name) != svtFunction)
			return false;
		pSS->BeginCall(pScriptTable, name); pSS->PushFuncParam(GetEntity()->GetScriptTable());
		pSS->PushFuncParam(p1); pSS->PushFuncParam(p2); pSS->PushFuncParam(p3);
		bool result = false;
		pSS->EndCall(result);
		return result;
	}
	template<typename P1, typename P2, typename P3, typename P4>
		bool CallScriptEvent(IScriptTable *pScriptTable, const char *name, P1 &p1, P2 &p2, P3 &p3, P4 &p4)
	{
		IScriptSystem *pSS = pScriptTable->GetScriptSystem();
		if (pScriptTable->GetValueType(name) != svtFunction)
			return false;
		pSS->BeginCall(pScriptTable, name); pSS->PushFuncParam(GetEntity()->GetScriptTable());
		pSS->PushFuncParam(p1); pSS->PushFuncParam(p2); pSS->PushFuncParam(p3); pSS->PushFuncParam(p4);
		bool result = false;
		pSS->EndCall(result);
		return result;
	}
	void CallScriptEvent(int tables, const char *name, bool *rserver, bool *rclient, bool *rroot)
	{
		if ((tables&eISET_Root) && !!m_stateTable[0])
		{
			bool r = CallScriptEvent(m_stateTable[0], name);
			if(rroot) *rroot = r;
		}
		if ((tables&eISET_Server) && !!m_stateTable[1] && gEnv->bServer)
		{
			bool s = CallScriptEvent(m_stateTable[1], name);
			if(rserver) *rserver = s;
		}
		if ((tables&eISET_Client) && !!m_stateTable[2] && gEnv->bClient)
		{
			bool c = CallScriptEvent(m_stateTable[2], name);
			if(rclient) *rclient = c;
		}
	}

	template<typename P1>
		void CallScriptEvent(int tables, const char *name, P1 &p1, bool *rserver, bool *rclient, bool *rroot)
	{
		if ((tables&eISET_Root) && !!m_stateTable[0])
		{
			bool r = CallScriptEvent(m_stateTable[0], name, p1);
			if(rroot) *rroot = r;
		}
		if ((tables&eISET_Server) && !!m_stateTable[1] && IsServer())
		{
			bool s = CallScriptEvent(m_stateTable[1], name, p1);
			if(rserver) *rserver = s;
		}
		if ((tables&eISET_Client) && !!m_stateTable[2] && IsClient())
		{
			bool c = CallScriptEvent(m_stateTable[2], name, p1);
			if(rclient) *rclient = c;
		}
	}
	template<typename P1, typename P2>
		void CallScriptEvent(int tables, const char *name, P1 &p1, P2 &p2, bool *rserver, bool *rclient, bool *rroot)
	{
		if ((tables&eISET_Root) && !!m_stateTable[0])
		{
			bool r = CallScriptEvent(m_stateTable[0], name, p1, p2);
			if(rroot) *rroot = r;
		}
		if ((tables&eISET_Server) && !!m_stateTable[1] && IsServer())
		{
			bool s = CallScriptEvent(m_stateTable[1], name, p1, p2);
			if(rserver) *rserver = s;
		}
		if ((tables&eISET_Client) && !!m_stateTable[2] && IsClient())
		{
			bool c = CallScriptEvent(m_stateTable[2], name, p1, p2);
			if(rclient) *rclient = c;
		}
	}
	template<typename P1, typename P2, typename P3>
		void CallScriptEvent(int tables, const char *name, P1 &p1, P2 &p2, P3 &p3, bool *rserver, bool *rclient, bool *rroot)
	{
		if ((tables&eISET_Root) && !!m_stateTable[0])
		{
			bool r = CallScriptEvent(m_stateTable[0], name, p1, p2, p3);
			if(rroot) *rroot = r;
		}
		if ((tables&eISET_Server) && !!m_stateTable[1] && IsServer())
		{
			bool s = CallScriptEvent(m_stateTable[1], name, p1, p2, p3);
			if(rserver) *rserver = s;
		}
		if ((tables&eISET_Client) && !!m_stateTable[2] && IsClient())
		{
			bool c = CallScriptEvent(m_stateTable[2], name, p1, p2, p3);
			if(rclient) *rclient = c;
		}
	}
	template<typename P1, typename P2, typename P3, typename P4>
		void CallScriptEvent(int tables, const char *name, P1 &p1, P2 &p2, P3 &p3, P4 &p4, bool *rserver, bool *rclient, bool *rroot)
	{
		if ((tables&eISET_Root) && !!m_stateTable[0])
		{
			bool r = CallScriptEvent(m_stateTable[0], name, p1, p2, p3, p4);
			if(rroot) *rroot = r;
		}
		if ((tables&eISET_Server) && !!m_stateTable[1] && IsServer())
		{
			bool s = CallScriptEvent(m_stateTable[1], name, p1, p2, p3, p4);
			if(rserver) *rserver = s;
		}
		if ((tables&eISET_Client) && !!m_stateTable[2] && IsClient())
		{
			bool c = CallScriptEvent(m_stateTable[2], name, p1, p2, p3, p4);
			if(rclient) *rclient = c;
		}
	};

	const SCachedItemAnimation GenerateAnimCacheID(const char *parameterisedAnimName) const;

	ILINE bool IsServer() {	return gEnv->bServer; }
	ILINE bool IsClient() {	return gEnv->bClient; }
protected:

	void CheckUserStealthyState(float frameTime);

	class COwnerInfo
	{
	public:
		COwnerInfo() : m_id(0) {}

		ILINE EntityId GetId() const { return m_id; }
		ILINE const SActorWeakRef& GetActorRef() const { return m_actorRef; }

		void Set(EntityId id, const SActorWeakRef& actorRef)
		{
			m_id = id;
			m_actorRef = actorRef;
		}

		void Reset()
		{
			m_id = 0;
			m_actorRef.Reset();
		}

	private:
		EntityId m_id;
		SActorWeakRef m_actorRef;
	};

	// data
	const CItemSharedParams*	m_sharedparams;

	SStats								m_stats;
	SEditorStats					m_editorstats;
	SCameraAnimationStats	m_camerastats;
	TEffectInfoMap				m_effects;
	TActiveLayerMap				m_activelayers;
	TAccessoryMap					m_accessories;
	TInstanceActionMap		m_instanceActions;
	std::vector<uint32>		m_damageLevelEffects;
	
	TInitialSetup					m_initialSetup;

	uint32								m_effectGenId;

	EntityId							m_dualWieldMasterId;
	EntityId							m_dualWieldSlaveId;

	CItemScheduler				m_scheduler;
	COwnerInfo						m_owner;
	EntityId							m_parentId;

	SmartScriptTable			m_stateTable[3];		// root, server, client

	ItemString						m_idleAnimation[eIGS_Last];
	uint32								m_animationTime[eIGS_Last];

	string								m_actionSuffix;
	string								m_actionSuffixAG;
	string								m_actionSuffixSerializationHelper;
	float									m_actionSuffixBlendTime;

	SGeometry							m_fpgeometry[3];		// have to save the first person geometry
																						// to support dynamic change of hand
	SRespawnProperties		m_respawnprops;
	SEntityProperties			m_properties;
	EntityId							m_hostId;
	EntityId							m_postSerializeMountedOwner;

	static IEntitySystem		*m_pEntitySystem;
	static IItemSystem			*m_pItemSystem;
	static IGameFramework		*m_pGameFramework;
	static IGameplayRecorder*m_pGameplayRecorder;

	float									m_fpOverlayOverride;
	float									m_fpOverlayOverrideEndTime;

	float									m_userStealthyTime;
	bool									m_isUserStealthy;

	ItemString						m_geometry[eIGS_Last];
	bool									m_modifying;
	bool									m_transitioning;
	bool									m_cloaked;
	bool									m_serializeCloaked; //used for cloaked serialization
	bool									m_accessoryAmmoAvailable;

	int										m_constraintId;

	Vec3									m_serializeActivePhysics;
	bool									m_serializeDestroyed;
	bool                  m_serializeRigidPhysics;

	bool									m_releaseCameraBone;

	tSoundID								m_idleSoundId;
	bool									m_inIdleAction;

public:

	bool									m_noDrop;				//Fix reseting problem in editor

	static IEntityClass*	sC4Class;
	static IEntityClass*	sBinocularsClass;
	static IEntityClass*	sDebugGunClass;
	static IEntityClass*	sRefWeaponClass;
	static IEntityClass*	sDSG1Class;
	static IEntityClass*	sLTagGrenade;
	static IEntityClass*  sIncendiaryAmmo;
};


#endif //__ITEM_H__
