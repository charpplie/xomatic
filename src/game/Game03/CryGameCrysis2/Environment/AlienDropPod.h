/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: A pod dropped from a floating ship to transport an Alien
				into the combat zone
  
 -------------------------------------------------------------------------
  History:
  - 02:15:2009: Created by Kevin Kirst

*************************************************************************/

#ifndef __ALIENDROPPOD_H__
#define __ALIENDROPPOD_H__

#include "IGameRulesSystem.h"

// Pod launch properties
struct SAlienPodLaunchProperties
{
	Vec3 vDestination;
	string sTerritory;
	string sWave;
	float fMaxForce;
	float fBounceForce;
	int nBounceCount;
	EntityId idHost;
	EntityId idLinkedAI;
	bool bComeToRest;

	SAlienPodLaunchProperties();
};

// Pod event listener
class CAlienDropPodListener
{
public:
	virtual ~CAlienDropPodListener() {}

	virtual void OnPodLaunched(bool bSuccess, const Vec3& vPos, const SAlienPodLaunchProperties& launchProperties) = 0;
	virtual void OnPodBounced(int nBounceCount, const Vec3& vPos) = 0;
	virtual void OnPodLanded(const Vec3& vPos) = 0;
	virtual void OnPodOpened(EntityId alienSpawnedId) = 0;
	virtual void OnPodDestroyed(EntityId alienPodId) = 0;
};

class CAlienDropPod : public CGameObjectExtensionHelper<CAlienDropPod, IGameObjectExtension>, IHitListener
{
public:
	CAlienDropPod();
	virtual ~CAlienDropPod();

	// IGameObjectExtension
	virtual bool Init(IGameObject *pGameObject);
	virtual void InitClient(int channelId) {}
	virtual void PostInit(IGameObject *pGameObject) {}
	virtual void PostInitClient(int channelId) {}
	virtual void Release();
	virtual void FullSerialize(TSerialize ser);
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) { return true; }
	virtual void PostSerialize() {}
	virtual void SerializeSpawnInfo(TSerialize ser) {}
	virtual ISerializableInfoPtr GetSpawnInfo() { return 0; }
	virtual void Update(SEntityUpdateContext &ctx, int updateSlot);
	virtual void PostUpdate(float frameTime) {}
	virtual void PostRemoteSpawn() {}
	virtual void HandleEvent(const SGameObjectEvent& event);
	virtual void ProcessEvent(SEntityEvent& event) {}
	virtual void SetChannelId(uint16 id) {}
	virtual void SetAuthority(bool auth) {}
	virtual void GetMemoryUsage(ICrySizer *pSizer) const;
	
	//~IGameObjectExtension

	//IHitListener
	virtual void OnHit(const HitInfo& hit) {}
	virtual void OnExplosion(const ExplosionInfo& explosionInfo) {}
	virtual void OnServerExplosion(const ExplosionInfo& explosionInfo);
	//~IHitListener

	// Launch the pod
	bool LaunchPod(const SAlienPodLaunchProperties& properties);

	bool AddListener(CAlienDropPodListener *pListener);
	bool RemoveListener(CAlienDropPodListener *pListener);

private:
	bool GetTableFromProperties(const char* szTableName, SmartScriptTable &pOutTable) const;

	IEntity* GetLinkedAIEntity() const;

	void SetHostEntity(EntityId idHost);
	void IgnoreEntityCollision(EntityId idEntity, bool bIgnore) const;

	void ApplyMovement(const Vec3& vDir, float fForceRatio) const;
	void CreateExplosion(const SmartScriptTable& pExplosionTable, const Vec3& vPos, const Vec3& vNormal) const;
	void OnBounce(const EventPhysCollision *pCollision);
	void OnFinishedBouncing();
	void OnAtRestingPoint();
	void OnSpawnAlien();

	enum EState
	{
		eState_Invalid = 0,
		eState_Launching,
		eState_Bouncing,
		eState_Rolling,
		eState_Resting,
		eState_Finished,

		eState_COUNT,
	};

	typedef std::vector<CAlienDropPodListener*> TListeners;
	TListeners m_Listeners;

	string m_sTerritory;
	string m_sWave;
	float m_fMaxForce;
	float m_fBounceForce;
	int m_nBounceLimit;
	int m_nCurrentBounces;
	EntityId m_idHost;
	EntityId m_idLinkedAI;
	EState m_eState;
	bool m_bComeToRest;
};

#endif //__ALIENDROPPOD_H__
