/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Rocket

-------------------------------------------------------------------------
History:
- 25:02:2009   11:15 : Created by Filipe Amim
- 16:09:2009   Re-factor to work together with LTagSingle (Benito G.R.)

*************************************************************************/
# pragma once

#ifndef _LTAG_GRENADE_H_
#define _LTAG_GRENADE_H_

#include "Projectile.h"
#include "ItemDefinitions.h"

struct IAttachment;

class CLTAGGrenade : public CProjectile
{
private:
	typedef CProjectile inherited;


public:
	CLTAGGrenade();
	virtual ~CLTAGGrenade();


	// CProjectile
	virtual void ReInitFromPool();

	virtual void HandleEvent(const SGameObjectEvent &);
	virtual void Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale);
	virtual void Update(SEntityUpdateContext &ctx, int updateSlot);
	virtual void Explode(bool destroy, bool impact=false, const Vec3 &pos=ZERO, const Vec3 &normal=FORWARD_DIRECTION, const Vec3 &vel=ZERO, EntityId targetId=0, float explosionScale = 1.f);
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags);
	// ~CProjectile

	bool IsStuck() const { return m_stuck; }
	void NetSetStuck(bool stuck);
	void SetGrenadeType(ELTAGGrenadeType grenadeType)
	{
		m_grenadeType = grenadeType; 
		CHANGED_NETWORK_STATE(this, eEA_GameServerStatic);
	}

protected:
	enum EState
	{
		EState_NONE,

		EStickyState_SAFE,
		EStickyState_UNSAFE,
		EStickyState_ARMING,

		ERicochetState_FLYING,
		ERicochetState_PROXY,
		ERicochetState_ARMED,
	};

	void UpdateLTagTimeOut();

	void OnCollision(const SGameObjectEvent &event);

	void ProxymityGrenadeUpdate(SEntityUpdateContext &ctx);
	void StateUpdate(SEntityUpdateContext &ctx);

	void AttachTo(IEntity * pTargetEntity);
	bool AttachToCharacter(IEntity& pEntity, ICharacterInstance& pCharacter, const char* boneName);

	void ChangeTexture(const string& textureName);
	void ChangeTrail(const string& trailString);

	// FIXME this code was taken directly from C4Projectile for prototyping proposes.
	// Stick behavior should be implemented in Projectile itself
	void Stick(EventPhysCollision *pCollision);

	bool IsProximityEnabled() const;
	bool ProximityDetector();

	void ExplodeGrenade(IActor* pHitActor=0);

	bool IsOwnerToClose();

	int PlaySound(const string& soundName);
	void StopSound(int id);

	void ChangeState(EState state);
	void StateEnter(EState state);
	void StateExit(EState state);

	virtual bool ShouldKnockTarget() const;

	const SLTagGrenadeParams::SCommonParams& GetModeParams() const;

	Vec3				m_launchLoc;
	
	ELTAGGrenadeType	m_grenadeType;


	
	bool		m_stuck;
	bool		m_stuckToLivingEntity;
	bool		m_enemyKilledByCollision;
	int			m_stuckJoint;
	EntityId	m_stickedEntityId;

	Quat		m_stuckRot;
	Vec3		m_stuckPos;
	Vec3		m_normal;
	float		m_extraArmTime;
	float		m_activeTime;
	float		m_lifeTime;
	bool		m_bounced;

	EState	m_state;

	IAttachment* m_characterAttachment;

	int				m_armedSoundId;

	static int		s_attachNameID;

public:
	int NetGetState() const { return m_state; }
	void NetSetState(int inState);
};


#endif // __LTAG_GRENADE_H__
