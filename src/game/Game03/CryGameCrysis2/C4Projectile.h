/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  C4 projectile specific stuff
-------------------------------------------------------------------------
History:
- 08:06:2007   : Created by Benito G.R.

*************************************************************************/

#ifndef __C4PROJECTILE_H__
#define __C4PROJECTILE_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Projectile.h"

struct IAttachmentManager;

class CC4Projectile : public CProjectile
{
private:
	typedef CProjectile BaseClass;

public:
	CC4Projectile();
	virtual ~CC4Projectile();

	virtual void HandleEvent(const SGameObjectEvent &event);
	virtual void Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale);
	virtual bool Detonate();
	virtual void ProcessEvent(SEntityEvent &event);
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags);
	virtual void Update(SEntityUpdateContext &ctx, int updateSlot);
	
	struct ProjectileStickToEntity
	{
		ProjectileStickToEntity():targetId(0) {localCollisonPos.Set(0,0,0); localRotation.SetIdentity();}
		ProjectileStickToEntity(EntityId id, Vec3 &pos, Quat &r):targetId(id),localCollisonPos(pos),localRotation(r) {}

		EntityId targetId;
		Vec3 localCollisonPos;
		Quat localRotation;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("targetId",targetId,'eid');
			ser.Value("localPos",localCollisonPos);
			ser.Value("localRot",localRotation);
		}
		
	};	

	DECLARE_CLIENT_RMI_NOATTACH(ClStickToEntity, ProjectileStickToEntity, eNRT_ReliableUnordered);

private:

	void Stick(EventPhysCollision *pCollision);
	void StickToStaticObject(EventPhysCollision *pCollision, IPhysicalEntity* pTarget);
	void StickToEntity(IEntity* pEntity, Matrix34 &localMatrix);
	void StickToCharacter(EventPhysCollision* pCollision, CActor* pActor, int sourceIndex, int targetIndex);
	bool AttachToHelper(IAttachmentManager* pAttachmentManager, CActor* pActor, Vec3 c4Direction);
	void CreateAttachment(EventPhysCollision* pCollision, CActor* pActor, ICharacterInstance* pCharacter, IAttachmentManager* pAttachmentManager, int targetIndex);
	
	virtual void SetParams(EntityId ownerId, EntityId hostId, EntityId weaponId, int damage, float damageFallOffStart, float damageFallOffAmount, float damageFalloffMin, int hitTypeId, int8 bulletPierceabilityModifier);

	EntityId	m_stuckEntityID;
	int				m_stuckAttIndex;
	int				m_teamId;
	bool			m_stuck;
	bool			m_armed;
	bool			m_uniqueAttachment;

	static int s_attachNameID;
};

#endif
