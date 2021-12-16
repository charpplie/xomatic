#include "StdAfx.h"
#include "ThreatDetectorPerk.h"

#include "Player.h"
#include "Perk.h"
#include "Projectile.h"
#include "WeaponSystem.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"


//Tiers
//One - Bullet Trails
//Two - Grenade Trails
//Three - Hud Icons on explosives

ThreatDetectorPerk::ThreatDetectorPerk()
{
	m_grenadesTracked = 0;
	for(int i = 0; i < k_maxGrenadeTrails; i++)
	{
		m_pGrenades[i] = NULL;
	}
	m_threatIcons = false;
}

void ThreatDetectorPerk::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
	case EPE_GrenadeLaunched:
		{
			STrailInfo* pTrailInfo = static_cast<STrailInfo*>(data);
			CProjectile *pProj = pTrailInfo->pProjectile;
			if (ShouldTrackGrenade(pProj))
			{
				if(m_grenadesTracked < k_maxGrenadeTrails)
				{
					m_pGrenades[m_grenadesTracked] = pTrailInfo->pProjectile;
					m_grenadesTracked++;
				}
			}
			break;
		}
	case EPE_GrenadeDestroyed:
		{
			STrailInfo* pTrailInfo = static_cast<STrailInfo*>(data);
			for(int i = 0; i < m_grenadesTracked; i++)
			{
				if(m_pGrenades[i] == pTrailInfo->pProjectile)
				{
					RemoveGrenade(i);
					return;
				}
			}
			break;
		}
	case EPE_BulletTrail:
		{
			if(m_ownerPlayer->IsClient())
			{
				STrailInfo* pTrailInfo = static_cast<STrailInfo*>(data);
				CProjectile *pProj = pTrailInfo->pProjectile;
				Vec3 currentPos = pTrailInfo->pos;

				if(ShouldCreateBulletTrail(pProj, currentPos))
				{
					CreateBulletTrail(pProj->GetInitialPos(), currentPos);
					CCCPOINT(Perk_ThreatTrails_RenderBulletTrail);
				}
			}

			break;
		}
	default:
		{
			IPerk::HandleEvent (perkEvent, data);
			break;
		}
	}
}

const void* ThreatDetectorPerk::GetData(EPlayerPlugInData dataType)
{
	switch(dataType)
	{
	case EPD_ThreatIcons:
		{
			m_threatIcons = IsTier(eTierThree);
			return &m_threatIcons;
		}
	}

	return NULL;
}

bool ThreatDetectorPerk::ShouldTrackGrenade(const CProjectile* pProj)
{
	const SAmmoParams& ammoParams = pProj->GetAmmoParams();

	return IsTier(eTierTwo) &&
		m_ownerPlayer->IsClient() &&
		ammoParams.bulletType == -1 &&
		ammoParams.physicalizationType == ePT_Particle &&
		CPlayerPerkParticleInfo::ShouldGenerateEffectsForEntity(pProj->GetOwnerId(), m_ownerPlayer, ePPPEI_Projectile) &&
		pProj->GetEntity()->GetParent() == NULL;
}

bool ThreatDetectorPerk::ShouldCreateBulletTrail(const CProjectile* pProj, const Vec3 currentPos)
{
	float t;
	const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
	const SAmmoParams& ammoParams = pProj->GetAmmoParams();

	return (ammoParams.bulletType != -1) &&
		ammoParams.physicalizationType == ePT_Particle &&
		CPlayerPerkParticleInfo::ShouldGenerateEffectsForEntity(pProj->GetOwnerId(), m_ownerPlayer, ePPPEI_Projectile) &&
		Distance::Point_LinesegSq(m_ownerPlayer->GetEntity()->GetPos(), Lineseg(pProj->GetInitialPos(), currentPos), t) < sqr(perkVars->perk_threatTrails_BulletRenderRange) &&
		!pProj->CheckAnyProjectileFlags(CProjectile::ePFlag_threatTrailEmitted);
}

void ThreatDetectorPerk::Update(const float dt)
{
	for(int i = 0; i < m_grenadesTracked; i++)
	{
		if(m_pGrenades[i])
		{
			TryUpdateGrenadeTrail(m_pGrenades[i], m_pGrenades[i]->GetEntity()->GetWorldPos());
		}
		else
		{
			RemoveGrenade(i);
		}
	}
}

void ThreatDetectorPerk::RemoveGrenade(int i)
{
	m_pGrenades[i] = NULL;
	m_grenadesTracked--;
	m_pGrenades[i] = m_pGrenades[m_grenadesTracked];
}

void ThreatDetectorPerk::CreateBulletTrail(const Vec3 &initialPos, const Vec3 &currentPos)
{
	const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();

	CTracerManager::STracerParams tracerParams;
	tracerParams.geometry = "Objects/effects/weapons/cw2_threat_trails_tracer.cgf";
	tracerParams.effect = NULL;
	tracerParams.position = initialPos;
	tracerParams.destination = currentPos;
	tracerParams.speed = 0.0f;
	tracerParams.lifetime = perkVars->perk_threatTrails_BulletRenderTime;
	tracerParams.delayBeforeDestroy = perkVars->perk_threatTrails_BulletRenderTime;
	tracerParams.scaleToDistance = true;
	tracerParams.startFadeOutTime = 1.0f;
	tracerParams.scale = perkVars->perk_threatTrails_BulletThickness;
	tracerParams.geometryOpacity = perkVars->perk_threatTrails_BulletOpacity;

	g_pGame->GetWeaponSystem()->GetTracerManager().EmitTracer(tracerParams);
	CAudioSignalPlayer::JustPlay("Perk_ThreatTrails_Bullet", currentPos);
}

void ThreatDetectorPerk::TryUpdateGrenadeTrail(CProjectile* pProj, Vec3 newPos)
{
	if (pProj->CheckAnyProjectileFlags(CProjectile::ePFlag_launched))
	{
		const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
		if (Distance::Point_Point(newPos, m_ownerPlayer->GetEntity()->GetPos()) < perkVars->perk_threatTrails_GrenadeRenderRange)
		{
			UpdateGrenadeTrail(pProj, m_ownerPlayer, newPos, "perk_fx.sonar_hearing.grenade_awareness", 0.1f, 0.f, 50.f, 0.02f, 10.f);
			CAudioSignalPlayer::JustPlay("Perk_ThreatTrails_Grenade", newPos);
			CCCPOINT(Perk_ThreatTrails_RenderGrenadeTrail);
		}
	}
}

void ThreatDetectorPerk::UpdateGrenadeTrail(CProjectile* pProj, CPlayer *pPlayer, const Vec3 &newPos, const char * particleEffect, float gap, float moveSpeed, float visibilityRange, float sizeIn, float appearDistance)
{
	Vec3 lastPos = pProj->GetLastPos();
	CRY_ASSERT_MESSAGE(lastPos.IsValid(), "Last position of projectile is invalid!");
	CRY_ASSERT_MESSAGE(newPos.IsValid(), "New position of projectile is invalid!");

	Vec3 dir = newPos-lastPos;
	float dist = dir.len();
	int numParticles = (int)((dist + (gap * 0.9f)) / gap);

	if (numParticles > 0)
	{
		Vec3 vel = dir;
		vel.normalize();
		vel *= moveSpeed;

		float appearDistanceSquared = appearDistance * appearDistance;

		IParticleEffect *pEffect=gEnv->pParticleManager->FindEffect(particleEffect);
		if (!pEffect)
		{
			return;
		}

		for (int i=0; i<numParticles; i++)
		{
			Vec3 pos=lastPos+(i/(float)numParticles)*dir;
			float scale = CPlayerPerkParticleInfo::CalculateBaseEffectScale(pos, pPlayer, visibilityRange, 0.f);

			if (scale > 0.01f)
			{
				int slot = pProj->GetEntity()->LoadParticleEmitter(-1, pEffect);
				IParticleEmitter * emitter = pProj->GetEntity()->GetParticleEmitter(slot);

				if (emitter)
				{
					// Do a nice curve on the sizes of the particles generated for the first 'appearDistance' meters of a projectile's life
					Vec3 offsetFromLaunchPos = pos - pProj->GetInitialPos();
					float distanceFromLaunchPosSquared = offsetFromLaunchPos.GetLengthSquared();
					if (distanceFromLaunchPosSquared < appearDistanceSquared)
					{
						float invRatio = 1.f - (distanceFromLaunchPosSquared / appearDistanceSquared);
						scale *= 1.f - (invRatio * invRatio * invRatio);
					}

					SpawnParams spawnParams;
					spawnParams.fCountScale = 0.0f;
					spawnParams.fSizeScale = sizeIn * scale * scale;

					QuatTS loc;
					loc.SetIdentity();
					loc.t = pos;
					loc.s = 1.f;

					emitter->SetRndFlags(ERF_RENDER_ALWAYS, true);
					emitter->SetSpawnParams(spawnParams);
					emitter->EmitParticle(0, 0, &loc, &vel);
				}
			}
		}
	}
}

