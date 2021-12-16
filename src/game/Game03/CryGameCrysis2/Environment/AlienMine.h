/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Crysis 2 Alien Mine reacting to player proximity

-------------------------------------------------------------------------
History:
- 10/08/09: Created by Jan Müller

*************************************************************************/

#pragma once

#include "IGameRulesSystem.h"
#include "DeferredRaycastWrapper.h"

struct IActor;

class CAlienMine : public CGameObjectExtensionHelper<CAlienMine, IGameObjectExtension>, IHitListener
{
public:
	CAlienMine();
	~CAlienMine();

	// IGameObjectExtension
	virtual bool Init(IGameObject *pGameObject);
	virtual void InitClient(int channelId) {};
	virtual void PostInit(IGameObject *pGameObject);
	virtual void PostInitClient(int channelId) {};
	virtual void Release() {};
	virtual void FullSerialize( TSerialize ser );
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags ) {return true;};
	virtual void PostSerialize() {}
	virtual void SerializeSpawnInfo( TSerialize ser ) {}
	virtual ISerializableInfoPtr GetSpawnInfo() {return 0;}
	virtual void Update( SEntityUpdateContext &ctx, int updateSlot) {};
	virtual void PostUpdate(float frameTime );
	virtual void PostRemoteSpawn() {};
	virtual void HandleEvent( const SGameObjectEvent &) {};
	virtual void ProcessEvent(SEntityEvent &event);
	virtual void SetChannelId(uint16 id) {}
	virtual void SetAuthority(bool auth) {};
	virtual void GetMemoryUsage(ICrySizer *pSizer) const	
	{
		pSizer->AddObject(this, sizeof(*this));
	}
	//~IGameObjectExtension

	//IHitListener
	virtual void OnHit(const HitInfo& hit);
	virtual void OnExplosion(const ExplosionInfo&);
	virtual void OnServerExplosion(const ExplosionInfo&) {};
	//~IHitListener

protected:

	bool	m_deactivated;
	bool	m_sleeping; //reduced scan frequency
	bool	m_armed;	//armed to detonate
	bool	m_exploding; //exploding - don't remove listeners in the same frame
	bool	m_returnToSpawnPos;	//returning to spawn location
	bool	m_invincibleUntilArmed; //don't take damage while sleeping
	bool	m_ignoreCloak; //attack cloaked player
	float	m_distanceTestTimer; //distance check and ray check update ratio
	float m_health; //health when taking damage
	float	m_maxDistanceToSpawnLocation;

	Vec3	m_spawnLocation;	//spawn location the alien mine moves back to

	CDeferredRaycastHelper m_raycastHelper;	//deferred visibility check

	//test against distance thresholds
	bool CheckDistanceToTarget(const Vec3 &targetPos, const Vec3 &minePos);
	//line of sight (raycast) at the active update frequence (DISTANCE_CHECK_TIMER_ACTIVE)
	bool HasLineOfSight(const Vec3 &targetPos, const Vec3 &dir, IActor *pTarget);
	//move to target and detonate
	void SeekAndDestroy(const Vec3 &targetPos, const Vec3 &minePos);
	//read script value from properties
	template<class T>
	bool GetScriptValue(const char *name, T &propValue);
	//explode and deactivate
	void Explode(const Vec3 &pos, const Vec3 &dir);
	//turn this mine on/off
	void Activate(bool active);
	//return to spawn location
	void ReturnToSpawnPos();
	//physicalize object
	void Physicalize();
	//update animation playback
	void UpdateAnimations();
};