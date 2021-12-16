#include "StdAfx.h"
#include "TacticalMagnet.h"
#include "GameRules.h"



void CTacticalMagnet::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
	CTacticalBullet::Launch(pos, dir, velocity, speedScale);

	m_maximumImpulsePerSecond = 100.0f;
	m_activeTime = 2.5f;
	m_countingTime = 0.0f;
	m_activeTimeSum = 0.25f;
	m_maximumImpulsePerSecondSum = 100.0f;

	m_maximumImpulsePerSecond = GetParam("impulsePerSecond", m_maximumImpulsePerSecond);
	m_activeTime = GetParam("activeTime", m_activeTime);
	m_activeTimeSum = GetParam("activeTimeSum", m_activeTimeSum);
	m_maximumImpulsePerSecondSum = GetParam("maximumImpulsePerSecondSum", m_maximumImpulsePerSecondSum);
}



void CTacticalMagnet::StartBulletAction()
{
	SpawnExplosionEffect();
}



void CTacticalMagnet::UpdateBulletAction(float frameTime)
{
	m_countingTime += frameTime;
	if (m_countingTime > m_activeTime)
	{
		Destroy();
		return;
	}
	const float timeDissipation = 1.0f - (m_countingTime / m_activeTime);

	SImpulse* impulses;
	int numImpulses;
	QueryMagnetAffectedEntities(&impulses, &numImpulses);
	if (numImpulses <= 0)
		return;
	const float totalImpulseValue = ComputeMagnetImpulses(impulses, numImpulses, timeDissipation);
	if (totalImpulseValue <= 0.0f)
		return;
	const Vec3 totalImpulseVec = NormalizeMagnetImpulses(impulses, numImpulses, totalImpulseValue, frameTime);
	ApplyMagnetImpulses(impulses, numImpulses);
	ApplyReactionImpulse(totalImpulseVec);
}



void CTacticalMagnet::QueryMagnetAffectedEntities(CTacticalMagnet::SImpulse** pImpulseArray, int* pNumEntitties) const
{
	const Vec3 center = this->GetEntity()->GetWorldPos();
	const Vec3 aabbMin = center - Vec3(GetRadius(), GetRadius(), GetRadius());
	const Vec3 aabbMax = center + Vec3(GetRadius(), GetRadius(), GetRadius());

	IPhysicalWorld* pPhysicalWorld = gEnv->pPhysicalWorld;
	IPhysicalEntity** pPhysicalEntities = 0;
	int objType = ent_all & (~ent_areas);
	int numEntities = pPhysicalWorld->GetEntitiesInBox(aabbMin, aabbMax, pPhysicalEntities, objType);

	static std::vector<SImpulse> impulses;
	impulses.clear();

	for (int i = 0; i < numEntities; ++i)
	{
		IPhysicalEntity* pPhisicalEntity = pPhysicalEntities[i];

		if (!CanApplyImpulse(pPhisicalEntity) || !HasMetalPart(pPhisicalEntity))
			continue;

		SImpulse imp;
		imp.impulse = Vec3(0, 0, 0);
		imp.target = pPhisicalEntity;
		impulses.push_back(imp);
	}

	*pNumEntitties = impulses.size();
	if (impulses.size() > 0)
	{
		*pImpulseArray = &impulses[0];
	}
}



float CTacticalMagnet::ComputeMagnetImpulses(CTacticalMagnet::SImpulse* impulses, int numEntitties, float timeDissipation) const
{
	float totalImpulseSq = 0.0f;
	for (int i = 0; i < numEntitties; ++i)
	{
		SImpulse& impulse = impulses[i];

		const Vec3 impulseValue = CalculateInteractionImpulse(impulse.target) * timeDissipation;
		totalImpulseSq += impulseValue.len2();

		impulse.impulse = impulseValue;
	}
	const float totalImpulse = cry_sqrtf(totalImpulseSq);

	return totalImpulse;
}



Vec3 CTacticalMagnet::NormalizeMagnetImpulses(CTacticalMagnet::SImpulse* impulses, int numEntitties, float totalImpulse, float frameTime) const
{
	Vec3 accumulatedImpulse = Vec3(0, 0, 0);

	const float maximumImpulse = m_maximumImpulsePerSecond * frameTime;

	for (int i = 0; i < numEntitties; ++i)
	{
		SImpulse& imp = impulses[i];

		const Vec3 impulse = (imp.impulse / totalImpulse) * maximumImpulse;
		accumulatedImpulse += impulse;

		imp.impulse = impulse;
	}

	return accumulatedImpulse;
}



void CTacticalMagnet::ApplyMagnetImpulses(CTacticalMagnet::SImpulse* impulses, int numEntitties) const
{
	for (int i = 0; i < numEntitties; ++i)
	{
		const SImpulse& imp = impulses[i];
		ApplyImpulse(imp.target, imp.impulse);
	}
}



void CTacticalMagnet::ApplyReactionImpulse(const Vec3& totalImpulseVec) const
{
	IEntity* pTargetEntity = gEnv->pEntitySystem->GetEntity(this->GetTargetId());
	IPhysicalEntity* pTargetPhysEntity = pTargetEntity ? pTargetEntity->GetPhysics() : 0;
	if (pTargetPhysEntity)
	{
		pe_params_bbox entityBB;
		pTargetPhysEntity->GetParams(&entityBB);
		const Vec3 entityCenter = (entityBB.BBox[0] + entityBB.BBox[1])*0.5f;

		ApplyImpulse(pTargetPhysEntity, -totalImpulseVec);
	}
}



Vec3 CTacticalMagnet::CalculateInteractionImpulse(IPhysicalEntity* pPhisicalEntity) const
{
	pe_params_bbox entityBB;
	pPhisicalEntity->GetParams(&entityBB);
	const Vec3 entityCenter = (entityBB.BBox[0] + entityBB.BBox[1])*0.5f;
	const Vec3 magnetCenter = this->GetEntity()->GetWorldPos();
	const float distanceSq = (entityCenter - magnetCenter).len2();
	const float radiusSq = GetRadius() * GetRadius();

	if (distanceSq > radiusSq)
		return Vec3(0, 0, 0);

	const float dissipation = -(1.0f/radiusSq)*distanceSq + 1.0f;
	const Vec3 impulseDirection = (magnetCenter - entityCenter).normalized();

	return impulseDirection * dissipation;
}



bool CTacticalMagnet::CanApplyImpulse(IPhysicalEntity* pPhisicalEntity) const
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPhisicalEntity);

	if (!pEntity)
		return false;
	if (pEntity && (pEntity->GetId() == this->GetEntity()->GetId()))
		return false;
	if (pEntity && (pEntity->GetId() == this->GetTargetId()))
		return false;
	//	if (pEntity == GetWeapon()->GetOwner())
	//		return false;

	return true;
}



bool CTacticalMagnet::HasMetalPart(IPhysicalEntity* pPhisicalEntity) const
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPhisicalEntity);
	//	if (pEntity->GetAI() && pEntity->GetAI()->IsHostile(m_pWeapon->GetOwner()->GetAI()))
	//		return true;
	//	else if (pEntity->GetAI() && !pEntity->GetAI()->IsHostile(m_pWeapon->GetOwner()->GetAI()))
	//		return false;

	pe_status_nparts nparts; 
	pe_params_part part;
	for (part.ipart = pPhisicalEntity->GetStatus(&nparts)-1; part.ipart >= 0; part.ipart--)
	{
		pPhisicalEntity->GetParams(&part);

		for (int i = 0; i < part.nMats; ++i)
		{
			int matMap = part.pMatMapping[i];
			ISurfaceType* pSurfacetype = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceType(matMap);
			if (pSurfacetype && strcmp(pSurfacetype->GetType(), "metal") == 0)
				return true;
		}
	}

	return false;
}



void CTacticalMagnet::ApplyImpulse(IPhysicalEntity* pPhisicalEntity, const Vec3& impulse) const
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPhisicalEntity);
	if (!pEntity)
		return;
	CActor* pActor = (CActor*)g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());

	pe_action_impulse	imp;
	imp.impulse = impulse;
	imp.iApplyTime = 0;
	pPhisicalEntity->Action(&imp);

	if (pActor && impulse.len2() > 1.0f)
	{
		//	pActor->Fall(Vec3(0, 0, 0), false, 2.0f);
	}
}



void CTacticalMagnet::IntensifyActionEffect()
{
	CTacticalBullet::IntensifyActionEffect();
	m_activeTime += m_activeTimeSum;
	m_maximumImpulsePerSecond += m_maximumImpulsePerSecondSum;
}
