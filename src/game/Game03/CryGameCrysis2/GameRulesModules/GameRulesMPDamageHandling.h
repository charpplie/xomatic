/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
	
	-------------------------------------------------------------------------
	History:
	- 07:09:2009  : Created by Ben Johnson
	- 08:09:2009  : Written by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_MP_DAMAGE_HANDLING_H_
#define _GAME_RULES_MP_DAMAGE_HANDLING_H_

#define MAX_BONES 100
#define MAX_PARTS 16

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameRulesDamageHandlingModule.h"

class CGameRulesMPDamageHandling : public IGameRulesDamageHandlingModule
{
public:
	CGameRulesMPDamageHandling();

	// IGameRulesDamageHandlingModule
	virtual ~CGameRulesMPDamageHandling();

	virtual void Init(XmlNodeRef xml);
	virtual void PostInit();
	virtual void Update(float frameTime);

	virtual void SvOnHit(const HitInfo &hitInfo);
	virtual void SvOnExplosion(const ExplosionInfo &explosionInfo, const CGameRules::TExplosionAffectedEntities& affectedEntities);
	virtual void SvOnCollision(const IEntity *entity, const CGameRules::SCollisionHitInfo& colHitInfo);

protected:

	void ProcessDeath(const HitInfo &hitInfo);
	void ProcessVehicleDeath(const HitInfo &hitInfo);
	float CalcExplosionDamage(IEntity* entity, const ExplosionInfo &explosionInfo, float obstruction);

	// Collisions
	float GetCollisionEnergy(const IEntity *pEntity, const CGameRules::SCollisionHitInfo& colHitInfo);
	float GetCollisionMinVelocity(const IEntity *pEntity, const CGameRules::SCollisionHitInfo& colHitInfo);
	float GetCollisionDamageMult(const IEntity *pEntity, const IEntity *pCollider, const CGameRules::SCollisionHitInfo& colHitInfo);
	float AdjustPlayerCollisionDamage(const IEntity *pEntity, const IEntity *pCollider, const CGameRules::SCollisionHitInfo& colHitInfo, float colInfo_damage);
	float ProcessPlayerToActorCollision(const IEntity *pEntity, const IEntity *pCollider, const CGameRules::SCollisionHitInfo& colHitInfo, float colInfo_damage);

	void LogHit(const HitInfo &hit, bool extended, bool dead);

	struct SEntityLastCollision
	{
		SEntityLastCollision() { entityId = 0; time = 0.f; }
		SEntityLastCollision(EntityId _entityId, float _time) : entityId(_entityId), time(_time) { }

		EntityId entityId;
		float time;
	};

	typedef std::map<EntityId,SEntityLastCollision> TEntityLastDamageMap;

	TEntityLastDamageMap m_entityLastDamage;

	float m_entityLastDamageUpdateTimer;

	SmartScriptTable m_scriptHitInfo;

	CGameRules *m_pGameRules;

	int m_barbwireMaterialTypeId;
	int m_hitTypeId_melee;
	
	bool m_initialised;

};

#endif // _GAME_RULES_MP_DAMAGE_HANDLING_H_