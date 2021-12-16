#include "StdAfx.h"
#include "SatelliteStrike.h"
#include "Game.h"
#include "GameCVars.h"
#include "HUD/HUD.h"
#include <IRenderAuxGeom.h>
#include "Perk.h"
#include "Audio/AudioSignalPlayer.h"

SStrikeParams CSatelliteStrike::s_satelliteStrikeParams;

//------------------------------------------------------------------------
CSatelliteStrike::CSatelliteStrike()
{
	m_targetPos(0.0f, 0.0f, 0.0f);
	m_explodeTimer = 0.0f;
	m_distanceToTravel = 0.0f;
	m_effect = NULL;
	m_explodeTime = 0.0f;
	m_audioSignal = "StrikeSatellite";
}

//------------------------------------------------------------------------
CSatelliteStrike::~CSatelliteStrike()
{

}

//------------------------------------------------------------------------
bool CSatelliteStrike::Init(IGameObject *pGameObject)
{
	if(CProjectile::Init(pGameObject))
	{
		// Only initialise static data once
		if(s_satelliteStrikeParams.isInitialised == false)
		{
			s_satelliteStrikeParams.effect = m_pAmmoParams->GetCachedEffect("ballistic_strike");
			s_satelliteStrikeParams.isInitialised = true;
		}

		// Initialise instance data from static data
		m_explodeTime = CPerk::GetInstance()->GetVars()->perk_satStrikeImpactTime;
		m_effect			= s_satelliteStrikeParams.effect;

		return true;
	}
	return false;
}

//------------------------------------------------------------------------
void CSatelliteStrike::GetHitPosition(const Vec3 &pos, Vec3 &hitPos, float &dist)
{
	//Targetting
	const Vec3 down(0.0f, 0.0f, -1.0f);
	const float range = CPerk::GetInstance()->GetVars()->perk_empSatelliteStrikeRange;
	Vec3 origin(pos);

	ray_hit	hit;
	if (gEnv->pPhysicalWorld->RayWorldIntersection(origin, down*range, ent_terrain|ent_static|ent_sleeping_rigid|ent_rigid, rwi_stop_at_pierceable|rwi_colltype_any, &hit, 1))
	{
		hitPos = hit.pt;
		dist = hit.dist;
	}
	else
	{
		hitPos = pos + (down * range);
		dist = range;
	}
}

//------------------------------------------------------------------------
void CSatelliteStrike::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
	GetHitPosition(pos, m_targetPos, m_distanceToTravel);

	//not attached to projectile so it doesn't stop when the it is destroyed (cutting off the end of the sound)
	CAudioSignalPlayer::JustPlay(m_audioSignal, m_targetPos);

	LaunchEffect();

	CProjectile::Launch(pos, dir, velocity, 0.0f);
}

//------------------------------------------------------------------------
void CSatelliteStrike::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	UpdateEffect();

	//Update
	CProjectile::Update(ctx, updateSlot);

	//Explode after fixed Time
	m_explodeTimer += ctx.fFrameTime;

	if(m_explodeTimer > m_explodeTime)
	{
		const float k_SatelliteStrike_sizeOnRadar = 10.0f;
		Explode(true, true, m_targetPos);
		SAFE_HUD_FUNC(ShowSoundOnRadar(m_targetPos, k_SatelliteStrike_sizeOnRadar));
	}
}

void CSatelliteStrike::LaunchEffect()
{
	// Create particle emitter
	if(m_effect)
	{
		m_effect->Spawn(true, IParticleEffect::ParticleLoc(m_targetPos, Vec3(0.0f, 0.0f, 1.0f), 1.0f));
	}
}

void CSatelliteStrike::UpdateEffect()
{
#ifdef _DEBUG
	static bool drawDebug = false;
	if(drawDebug)
	{
		ColorB color = ColorB(128, 64, 0);

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(m_initial_pos, color, m_targetPos, color, 6.0f);

		float distanceTravelled = m_explodeTimer / m_explodeTime;
		const Vec3 down(0.0f, 0.0f, -1.0f);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(m_initial_pos + ((down * m_distanceToTravel) * distanceTravelled), 1.0f, color);
	}
#endif
}

void CSatelliteStrike::StrikeTargetPosition(const int currentPoint, const int numberOfPoints, Vec3& targetPos)
{
	const float k_spinScale = 1.5f;
	float spin = gEnv->pTimer->GetCurrTime() * k_spinScale;
	const static float k_StrikeRadius = 5.0f;
	float angle = ((float) currentPoint/(float) numberOfPoints) * (float) g_PI2;
	angle += spin;
	targetPos.x += (sinf(angle) * k_StrikeRadius);
	targetPos.y += (cosf(angle) * k_StrikeRadius);
}