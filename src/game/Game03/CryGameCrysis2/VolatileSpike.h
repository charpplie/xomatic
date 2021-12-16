/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Volatile Spike

-------------------------------------------------------------------------
History:
- 16:04:2009   12:00 : Created by Claire Allan

*************************************************************************/
#ifndef __VOLATILESPIKE_H__
#define __VOLATILESPIKE_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Projectile.h"

#define VOLATILE_SPIKE_MAX_STATIC_OBJECTS		8

struct IAttachment;

class CVolatileSpike : public CProjectile
{
public:
	CVolatileSpike();
	~CVolatileSpike();

	virtual void	Update( SEntityUpdateContext &ctx, int updateSlot);
	virtual void	HandleEvent(const SGameObjectEvent &event);
	virtual void	PostInit(IGameObject *pGameObject);
	virtual void	Explode(bool destroy, bool impact=false, const Vec3 &pos=ZERO, const Vec3 &normal=FORWARD_DIRECTION, const Vec3 &vel=ZERO, EntityId targetId=0, float explosionScale = 1.f);
	virtual bool	NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags);

	bool			IsStuck() const { return IsFlagSet(VS_STUCK); }
	void			SetStuck(bool stuck);

protected:

	struct StaticEntity
	{
		StaticEntity()
		{
			volume = 0;
		}

		Vec3 dirTo;
		float volume;
	};

	pe_action_add_constraint m_constraintParams;
	StaticEntity m_staticEntity[VOLATILE_SPIKE_MAX_STATIC_OBJECTS];
	Vec3			m_offsetPosition;
	Quat			m_offsetRotation;
	IAttachment*	m_characterAttachment;
	int				m_stuckJointID;
	EntityId		m_stuckEntityID;
	int					m_numCachedStaticEntities;
	IPhysicalEntity* m_floor;
	IPhysicalEntity* m_backWall;
	float			m_explosionTimer;
	float			m_stuckLife;
	short			m_numDecalsSpawned;
	short			m_numSteamEmittersSpawned;

	enum EVolatileSpikeFlags
	{
		VS_STUCK_TO_CHARACTER										= (1<<0),
		VS_STUCK																=	(1<<1),
		VS_CONSTRAINT														= (1<<2),
		VS_SEND_CONSTRAINT											= (1<<3),
		VS_HAS_EXPLODED													= (1<<4),
		VS_IS_STUCK_IN_WALL											= (1<<5),
		VS_HAS_SPAWNED_EXPLOSION_EFFECT					= (1<<6),
		VS_HAS_SPAWNED_DECALS_ON_WALL_AND_FLOOR	= (1<<7)
	};
	uchar			m_flags;

	void			PlayImpactSound();
	void			StickToStaticObject(Vec3 collisionPos);
	void			StickToEntity(IEntity* pTargetEntity, Vec3 collisionPos);
	bool			StickToCharacter(IEntity* pActor, EventPhysCollision* pCollision);
	void			AttachToCharacter(ICharacterInstance* pCharacter, const char* boneName);
	bool			FindWallContact(IEntity* pEntity, Quat currentRotation, Vec3 currentPosition);
	void			DebugRender();
	void			SpawnExplosionEffect();

	inline void		SetFlag(EVolatileSpikeFlags flag,bool state) { (state)?(m_flags |= flag):(m_flags &= ~flag);}
	inline bool		IsFlagSet(EVolatileSpikeFlags flag) const { return (m_flags & flag)?true:false; }

	void			CacheDirectionsToStaticEntitiesForExplosionDecals();
	void			CacheFloorAndBackWall();
	void			UpdateDecals(SEntityUpdateContext &ctx);
	bool			CreateDecal(const Vec3 &origin, const Vec3 &dir, float distance, float decalSize,bool spawnSteam);
	void			DebugRenderDecals();
	void			GetEntityCentre(Vec3* entityCentre) const;

private:
	typedef CProjectile inherited;

	static int s_attachNameID;
};


#endif // __VOLATILESPIKE_H__