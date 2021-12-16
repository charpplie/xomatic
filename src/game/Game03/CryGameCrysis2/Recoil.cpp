/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 23:09:2009   : Created by Filipe Amim

*************************************************************************/

#include "StdAfx.h"

#include "Recoil.h"
#include "Weapon.h"
#include "Player.h"
#include "GameCVars.h"
#include "IronSight.h"
#include "Utility/CryWatch.h"

#define ResetValue(name, defaultValue) if (defaultInit) name=defaultValue; reader.Read(#name, name)
#define ResetValueEx(name, var, defaultValue) if (defaultInit) var=defaultValue; reader.Read(name, var)


namespace
{

#	ifndef _RELEASE

	class CRecoilDebugDraw
	{
	private:
		struct SRecoilPoint
		{
			Vec3 direction;
			float recoil;
			float time;
		};

		typedef std::deque<SRecoilPoint> TRecoilPoints;
		typedef TRecoilPoints::iterator TRecoilPointsIterator;

	public:
		static void AddRecoilPoint(const Vec3& recoilDir, float recoil)
		{
			GetRecoilDebugDraw().PushRecoilPoint(recoilDir, recoil);
		}
		
		static void UpdateRecoilDraw(float frameTime)
		{
			GetRecoilDebugDraw().Update(frameTime);
		}
		
		static void DebugRecoil(float recoilPercentage, const Vec2 recoilOffset)
		{
			if(g_pGameCVars->i_debug_recoil)
			{
				CryWatch("RECOIL:\nCurrent Percentage Of Max Recoil: %.4f\n", recoilPercentage*100.f);
				CryWatch("Current Recoil Offset: %.4f %.4f\n", recoilOffset.x, recoilOffset.y);
			}
		}
		
		static void DebugSpread(float spread)
		{
			if(g_pGameCVars->i_debug_spread)
			{
				CryWatch("SPREAD:\nCurrent Spread: %.4f\n", spread);
			}
		}

	private:
		static CRecoilDebugDraw& GetRecoilDebugDraw()
		{
			static CRecoilDebugDraw debugDraw;
			return debugDraw;
		}

		void PushRecoilPoint(const Vec3& recoilDir, float recoil)
		{
			if (g_pGameCVars->i_debug_recoil)
			{
				SRecoilPoint recoilPoint;
				recoilPoint.direction = recoilDir;
				recoilPoint.recoil = recoil;
				recoilPoint.time = 0.0f;
				m_recoilPoints.push_back(recoilPoint);
			}
		}

		void Update(float frameTime)
		{
			if (g_pGameCVars->i_debug_recoil)
			{
				const float pointTimeOut = 2.0f;
				const Vec2 center = Vec2(400, 400);
				const float yellow[4] = {0, 1, 0, 1};
				const float pointSize = 3.0f;
				const float recoilScale = 100.0f;

				gEnv->pRenderer->Draw2dLabel(
					center.x, center.y,
					pointSize, yellow, false, ".");

				TRecoilPointsIterator it;
				for (it = m_recoilPoints.begin(); it != m_recoilPoints.end(); ++it)
				{
					it->time += frameTime;
					float opacity = SATURATE(1.0f-(it->time / pointTimeOut));
					const float red[4] = {1, 0, 0, opacity};
					gEnv->pRenderer->Draw2dLabel(
						it->direction.x*it->recoil*recoilScale + center.x,
						-it->direction.y*it->recoil*recoilScale + center.y,
						pointSize, red, false, ".");
				}

				while (!m_recoilPoints.empty())
				{
					it = m_recoilPoints.begin();
					if (it->time < pointTimeOut)
						break;
					m_recoilPoints.erase(it);
				}
			}
			else if (!m_recoilPoints.empty())
			{
				m_recoilPoints.clear();
			}
		}

		TRecoilPoints m_recoilPoints;
	};


	void DrawDebugZoomMods(const SRecoilParams& recoilParams, const SSpreadParams& spreadParams)
	{
		if(g_pGameCVars->i_debug_zoom_mods!=0)
		{
			float white[4] = {1,1,1,1};
			gEnv->pRenderer->Draw2dLabel(50.0f,50.0f,1.4f,white,false,"Recoil.angular_impulse : %f", recoilParams.angular_impulse);
			gEnv->pRenderer->Draw2dLabel(50.0f,60.0f,1.4f,white,false,"Recoil.attack : %f", recoilParams.attack);
			gEnv->pRenderer->Draw2dLabel(50.0f,70.0f,1.4f,white,false,"Recoil.back_impulse : %f", recoilParams.back_impulse);
			gEnv->pRenderer->Draw2dLabel(50.0f,80.0f,1.4f,white,false,"Recoil.decay : %f", recoilParams.decay);
			gEnv->pRenderer->Draw2dLabel(50.0f,90.0f,1.4f,white,false,"Recoil.impulse : %f", recoilParams.impulse);
			gEnv->pRenderer->Draw2dLabel(50.0f,100.0f,1.4f,white,false,"Recoil.max x,y : %f, %f", recoilParams.max.x, recoilParams.max.y);
			gEnv->pRenderer->Draw2dLabel(50.0f,110.0f,1.4f,white,false,"Recoil.max_recoil : %f", recoilParams.max_recoil);
			gEnv->pRenderer->Draw2dLabel(50.0f,120.0f,1.4f,white,false,"Recoil.recoil_crouch_m : %f", recoilParams.recoil_crouch_m);
			gEnv->pRenderer->Draw2dLabel(50.0f,130.0f,1.4f,white,false,"Recoil.recoil_jump_m : %f", recoilParams.recoil_jump_m);
			gEnv->pRenderer->Draw2dLabel(50.0f,150.0f,1.4f,white,false,"Recoil.recoil_strMode_m : %f", recoilParams.recoil_powerActive_m);

			gEnv->pRenderer->Draw2dLabel(300.0f, 50.0f, 1.4f, white, false, "Spread.attack : %f", spreadParams.attack);
			gEnv->pRenderer->Draw2dLabel(300.0f, 60.0f, 1.4f, white, false, "Spread.decay : %f", spreadParams.decay);
			gEnv->pRenderer->Draw2dLabel(300.0f, 70.0f, 1.4f, white, false, "Spread.max : %f", spreadParams.max);
			gEnv->pRenderer->Draw2dLabel(300.0f, 80.0f, 1.4f, white, false, "Spread.min : %f", spreadParams.min);
			gEnv->pRenderer->Draw2dLabel(300.0f, 90.0f, 1.4f, white, false, "Spread.rotation_m : %f", spreadParams.rotation_m);
			gEnv->pRenderer->Draw2dLabel(300.0f, 100.0f, 1.4f, white, false, "Spread.speed_m : %f", spreadParams.speed_m);
			gEnv->pRenderer->Draw2dLabel(300.0f, 110.0f, 1.4f, white, false, "Spread.speed_powerActive_m : %f", spreadParams.speed_powerActive_m);
			gEnv->pRenderer->Draw2dLabel(300.0f, 120.0f, 1.4f, white, false, "Spread.spread_crouch_m : %f", spreadParams.spread_crouch_m);
			gEnv->pRenderer->Draw2dLabel(300.0f, 130.0f, 1.4f, white, false, "Spread.spread_jump_m : %f", spreadParams.spread_jump_m);
			gEnv->pRenderer->Draw2dLabel(300.0f, 140.0f, 1.4f, white, false, "Spread.spread_powerActive_m : %f", spreadParams.spread_powerActive_m);
		}
	}


#	else


	class CRecoilDebugDraw
	{
	public:
		inline static void AddRecoilPoint(const Vec3& recoilDir, float recoil) {}
		inline static void UpdateRecoilDraw(float frameTime) {}
		inline static void DebugRecoil(float recoilPercentage, const Vec2 recoilOffset) { };
		inline static void DebugSpread(float spread) { };
	private:
	};

	inline void DrawDebugZoomMods(const SRecoilParams& recoilParams, const SSpreadParams& spreadParams) {}

#	endif

}



SSpreadModParams::SSpreadModParams()
{
	Reset();
}


void SSpreadModParams::Reset( const IItemParamsNode *params/*=0*/, bool defaultInit/*=true*/ )
{
	CItemParamReader reader(params);
	ResetValue(min_mod,					1.0f);
	ResetValue(max_mod,					1.0f);
	ResetValue(attack_mod,				1.0f);
	ResetValue(decay_mod,				1.0f);
	ResetValue(speed_m_mod,				1.0f);
	ResetValue(speed_powerActive_m_mod,	1.0f);
	ResetValue(rotation_m_mod,			1.0f);

	ResetValue(spread_crouch_m_mod,			1.0f);
	ResetValue(spread_jump_m_mod,			1.0f);
	ResetValue(spread_powerActive_m_mod,	1.0f);
	ResetValue(spread_leaning_m_mod,		1.0f);
}



SRecoilModParams::SRecoilModParams()
{
	Reset();
}


void SRecoilModParams::Reset( const IItemParamsNode *params/*=0*/, bool defaultInit/*=true*/ )
{
	CItemParamReader reader(params);
	ResetValue(max_recoil_mod,			1.0f);
	ResetValue(attack_mod,				1.0f);
	ResetValue(first_attack_mod,		1.0f);
	ResetValue(decay_mod,				1.0f);
	ResetValue(end_decay_mod,			1.0f);
	ResetValueEx("maxx_mod", max_mod.x,	1.0f);
	ResetValueEx("maxy_mod", max_mod.y,	1.0f);
	ResetValue(impulse_mod,				1.0f);
	ResetValue(angular_impulse_mod,		1.0f);
	ResetValue(back_impulse_mod,		1.0f);

	ResetValue(recoil_crouch_m_mod,		1.0f);
	ResetValue(recoil_jump_m_mod,		1.0f);
	ResetValue(recoil_leaning_m_mod,	1.0f);

	ResetValue(recoil_powerActive_m_mod,	1.0f);
}




SRecoilParams::SRecoilParams()
{
	Reset();
}

void SRecoilParams::Reset( const IItemParamsNode *params/*=0*/, bool defaultInit/*=true*/ )
{
	CItemParamReader reader(params);
	ResetValue(max_recoil,			0.0f);
	ResetValue(attack,				0.0f);
	ResetValue(first_attack,		0.0f);
	ResetValue(decay,				0.65f);
	ResetValue(end_decay,			0.0f);
	ResetValueEx("maxx", max.x,		8.0f);
	ResetValueEx("maxy", max.y,		4.0f);
	ResetValue(randomness,			1.0f);
	ResetValue(impulse,				0.f);
	ResetValue(angular_impulse,		0.0f);
	ResetValue(back_impulse,		0.0f);
	ResetValue(hint_loop_start,		7);

	ResetValue(recoil_crouch_m,		0.85f);
	ResetValue(recoil_jump_m,		1.5f);
	ResetValue(recoil_leaning_m,	1.0f);

	ResetValue(recoil_powerActive_m,	0.5f);

	if (defaultInit)
		hints.resize(0);

	if (params)
	{
		const IItemParamsNode *phints = params->GetChild("hints");
		if (phints && phints->GetChildCount())
		{
			Vec2 h;
			int n = phints->GetChildCount();
			hints.resize(n);

			for (int i=0; i<n; i++)
			{
				const IItemParamsNode *hint = phints->GetChild(i);
				if (hint->GetAttribute("x", h.x) && hint->GetAttribute("y", h.y))
					hints[i]=h;
			}
		}
	}
}

void SRecoilParams::GetMemoryUsage( ICrySizer * s ) const 
{
	s->AddObject(hints);
}



SProceduralRecoilParams::SProceduralRecoilParams()
{
	Reset();
}

void SProceduralRecoilParams::Reset( const IItemParamsNode *params/*=0*/, bool defaultInit/*=true*/ )
{
	CItemParamReader reader(params);
	ResetValue(duration,					0.1f);
	ResetValue(strength,					0.05f);

	if (defaultInit) 
	{ 
		enabled = (params != NULL); 
	}
	else 
	{ 
		enabled = enabled || (params != NULL); 
	}
}

void SProceduralRecoilParams::GetMemoryStatistics( ICrySizer * s ) const
{
}



SSpreadParams::SSpreadParams()
{
	Reset();
}

void SSpreadParams::Reset( const IItemParamsNode *params/*=0*/, bool defaultInit/*=true*/ )
{
	CItemParamReader reader(params);
	ResetValue(min,					0.015f);
	ResetValue(max,					0.0f);
	ResetValue(attack,				0.95f);
	ResetValue(decay,				0.65f);
	ResetValue(end_decay,			0.65f);
	ResetValue(speed_m,				0.25f);
	ResetValue(speed_powerActive_m,	0.25f);
	ResetValue(rotation_m,			0.35f);

	ResetValue(spread_crouch_m,			0.85f);
	ResetValue(spread_jump_m,			1.5f);
	ResetValue(spread_finalBreath_m,	5.0f);
	ResetValue(spread_powerActive_m,	1.0f);
	ResetValue(spread_leaning_m,		1.0f);
}




CRecoil::CRecoil()
	: m_pWeapon(0)
	, m_pFireMode(0)
	, m_recoil(0)
	,	m_recoil_dir_idx(0)
	,	m_recoil_dir(ZERO)
	,	m_recoil_offset(ZERO)
	,	m_spread(0)
	,	m_recoilMultiplier(1.0f)
	, m_singleShot(false)
{
}



void CRecoil::Update(float frameTime, bool weaponFired, int frameId, bool firstShot)
{
	CActor* pActor = m_pWeapon->GetOwnerActor();

	bool isOwnerClient = (pActor != NULL) && (pActor->IsClient());

	if (!isOwnerClient)
		return;

	const CActor& ownerClient = *pActor;
	const bool weaponFiring = m_pFireMode->IsFiring();

	if (weaponFired)
	{
		RecoilShoot(firstShot);
		SpreadShoot();
	}

	UpdateRecoil(ownerClient, weaponFired, weaponFiring, frameTime);
	UpdateSpread(weaponFired, weaponFiring, frameTime);

	if (m_recoil < 0.0f)
	{
		ResetRecoilInternal();
	}
	else if (m_recoil > 0.0f)
	{
		m_pWeapon->ApplyFPViewRecoil(frameId, m_recoil_offset.x, m_recoil_offset.y);
	}

	DrawDebugZoomMods(m_recoilParams, m_spreadParams);
	CRecoilDebugDraw::UpdateRecoilDraw(frameTime);
}



void CRecoil::RecoilShoot(bool firstShot)
{
	float attack = firstShot ? m_recoilParams.first_attack : m_recoilParams.attack;
	attack = (float)__fsel(attack, m_recoilParams.attack, attack);
	m_recoil = clamp(m_recoil + attack, 0.0f, m_recoilParams.max_recoil);

	Vec2 direction = Vec2(ZERO);
	if (m_recoilParams.hints.size() > 0)
	{
		direction = m_recoilParams.hints[m_recoil_dir_idx];
		m_recoil_dir_idx = (m_recoil_dir_idx+1) % m_recoilParams.hints.size();
	}

	const Vec2 randomDirectionAdd = Vec2(Random(m_recoilParams.randomness), BiRandom(m_recoilParams.randomness));

	m_recoil_dir = direction + randomDirectionAdd;
	m_recoil_dir.NormalizeSafe();

	CRecoilDebugDraw::AddRecoilPoint(m_recoil_dir, m_recoil);
}



void CRecoil::UpdateRecoil(const CActor& weaponOwner, bool weaponFired, bool weaponFiring, float frameTime)
{
	const float recoilScale = GetRecoilScale(weaponOwner);
	const bool isFiring = m_singleShot ? weaponFired : weaponFiring;
	const float decay = isFiring ? m_recoilParams.decay : m_recoilParams.end_decay;
	const float frameDecay = (float)__fsel(-decay, 0.0f, frameTime * recoilScale * m_recoilParams.max_recoil / (decay+FLT_EPSILON));
	m_recoil = clamp(m_recoil - frameDecay, 0.0f, m_recoilParams.max_recoil);

	const float t = (float)__fsel(-m_recoilParams.max_recoil, 0.0f, m_recoil / (m_recoilParams.max_recoil+FLT_EPSILON));
	const Vec2 new_offset = Vec2(m_recoil_dir.x * sin_tpl(DEG2RAD(m_recoilParams.max.x)), m_recoil_dir.y * sin_tpl(DEG2RAD(m_recoilParams.max.y))) * t;

	m_recoil_offset = (new_offset * 0.66f) + (m_recoil_offset * 0.33f);

	CRecoilDebugDraw::DebugRecoil(t, m_recoil_offset);
}



void CRecoil::SpreadShoot()
{
	m_spread = clamp(m_spread + m_spreadParams.attack, m_spreadParams.min, m_spreadParams.max);
}



void CRecoil::UpdateSpread(bool weaponFired, bool weaponFiring, float frameTime)
{
	const bool isFiring = m_singleShot ? weaponFired : weaponFiring;
	const float decay = isFiring ? m_spreadParams.decay : m_spreadParams.end_decay;
	const float frameDecay = (float)__fsel(-decay, 0.0f, ((m_spreadParams.max - m_spreadParams.min) / (decay + FLT_EPSILON)) * frameTime);

	m_spread = clamp(m_spread - frameDecay, m_spreadParams.min, m_spreadParams.max);

	CRecoilDebugDraw::DebugSpread(m_spread);
}

void CRecoil::Reset(bool spread)
{
	ResetRecoilInternal();
	
	if (spread)
	{
		ResetSpreadInternal();
	}
}

void CRecoil::ResetRecoilInternal()
{
	m_recoil = 0.0f;
	m_recoil_dir_idx = 0;
	m_recoil_dir = Vec2(0.0f,0.0f);
	m_recoil_offset = Vec2(0.0f,0.0f);
}

void CRecoil::ResetSpreadInternal()
{
	m_spread = m_spreadParams.min;
}

float CRecoil::GetRecoilScale(const CActor& weaponOwner) const
{
	//Same as for the spread (apply stance multipliers)
	float stanceScale = 1.0f;
	float suitScale = 1.0f;
	float leanScale = 1.0f;

	bool inAir  = (weaponOwner.GetActorStats()->inAir >= 0.05f);

	if (inAir)
	{
		stanceScale = m_recoilParams.recoil_jump_m;
	}
	else if (weaponOwner.GetStance() == STANCE_CROUCH)
	{
		stanceScale = m_recoilParams.recoil_crouch_m;
	}			

	// Rather than doing this every frame, going into eNanoSuitMode_Power could just do player->GetModifiableValues()->GetValue(kPMV_WeaponRecoil)->AddModifier(& floatValue)
	if (IsSuitInActivePowerMode(weaponOwner))
	{
		suitScale *= (float)__fsel(-m_recoilParams.recoil_powerActive_m, 1.0f, m_recoilParams.recoil_powerActive_m);
	}

	CRY_ASSERT(weaponOwner.IsPlayer());	//Client is a CPlayer, we can downcast
	const CPlayer& weaponOwnerPlayer = static_cast<const CPlayer&>(weaponOwner);

	suitScale *= weaponOwnerPlayer.GetModifiableValues().GetValue(kPMV_WeaponRecoil);

	if (weaponOwnerPlayer.GetCoverAndLean().IsLeaning())
	{
		leanScale = m_recoilParams.recoil_leaning_m;
	}
	

	return m_recoilMultiplier * stanceScale * suitScale * leanScale;
}



float CRecoil::GetSpread() const
{
	CActor *pOwnerActor = m_pWeapon->GetOwnerActor();
	bool playerIsOwner = (pOwnerActor && pOwnerActor->IsPlayer());

	if (!playerIsOwner)
		return m_spread;

	const CActor& ownerActor = *pOwnerActor;

	bool onActivePowerMode = IsSuitInActivePowerMode(ownerActor);
	float stanceScale = 1.0f;
	float speedSpread = 0.0f;
	float rotationSpread = 0.0f;
	float suitScale = 1.0f;
	float leanScale = 1.0f;

	const SActorStats* pOwnerStats = ownerActor.GetActorStats();

	bool inAir = (pOwnerStats->inAir >= 0.05f) && !ownerActor.GetLinkedVehicle();

	if ((ownerActor.GetStance() == STANCE_CROUCH) && !inAir)
		stanceScale = m_spreadParams.spread_crouch_m;
	else if (inAir)
		stanceScale = m_spreadParams.spread_jump_m;

	const float speed_m = onActivePowerMode ? m_spreadParams.speed_powerActive_m : m_spreadParams.speed_m;
	speedSpread = pOwnerStats->velocity.len() * speed_m;
	rotationSpread = clamp(pOwnerStats->angVelocity.len() * m_spreadParams.rotation_m, 0.0f, 3.0f);

	suitScale *= onActivePowerMode ? m_spreadParams.spread_powerActive_m : 1.0f;

	CRY_ASSERT(ownerActor.IsPlayer());
	const CPlayer& weaponOwnerPlayer = static_cast<const CPlayer&>(ownerActor);

	const float playerScale = weaponOwnerPlayer.GetModifiableValues().GetValue(kPMV_WeaponSpread);
	if (weaponOwnerPlayer.GetCoverAndLean().IsLeaning())
	{
		leanScale = m_spreadParams.spread_leaning_m;
	}

	stanceScale *= playerScale;
	rotationSpread *= playerScale;

	return (speedSpread+rotationSpread+m_spread)*stanceScale*suitScale*leanScale;

}



void CRecoil::RecoilImpulse(const Vec3& firingPos, const Vec3& firingDir)
{
	if (m_recoilParams.impulse > 0.f)
	{
		EntityId id = (m_pWeapon->GetHostId()) ? m_pWeapon->GetHostId() : m_pWeapon->GetOwnerId();
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(id);
		IPhysicalEntity* pPhysicalEntity = pEntity ? pEntity->GetPhysics() : NULL;

		if (pPhysicalEntity)
		{        
			pe_action_impulse impulse;
			impulse.impulse = -firingDir * m_recoilParams.impulse; 
			impulse.point = firingPos;
			pPhysicalEntity->Action(&impulse);
		}
	}
}



void CRecoil::GetMemoryUsage(ICrySizer* s) const
{	
	s->AddObject(m_recoilParams);
	s->AddObject(m_spreadParams);	
}



void CRecoil::PatchSpreadMod(const SSpreadModParams& spreadMod)
{
	m_spreadParams.attack				*= spreadMod.attack_mod;
	m_spreadParams.decay				*= spreadMod.decay_mod;
	m_spreadParams.max					*= spreadMod.max_mod;
	m_spreadParams.min					*= spreadMod.min_mod;
	m_spreadParams.rotation_m			*= spreadMod.rotation_m_mod;
	m_spreadParams.speed_m				*= spreadMod.speed_m_mod;
	m_spreadParams.speed_powerActive_m	*= spreadMod.speed_powerActive_m_mod;

	m_spreadParams.spread_crouch_m		*= spreadMod.spread_crouch_m_mod;
	m_spreadParams.spread_jump_m		*= spreadMod.spread_jump_m_mod;
	m_spreadParams.spread_powerActive_m	*= spreadMod.spread_powerActive_m_mod;
	m_spreadParams.spread_leaning_m		*= spreadMod.spread_leaning_m_mod;

	m_spread = m_spreadParams.min;
}



void CRecoil::ResetSpreadMod(const SSpreadParams& originalSpreadParams)
{
	m_spreadParams = originalSpreadParams;
	m_spread = m_spreadParams.min;
}



void CRecoil::PatchRecoilMod(const SRecoilModParams& recoilMod)
{
	m_recoilParams.angular_impulse		*= recoilMod.angular_impulse_mod;
	m_recoilParams.attack				*= recoilMod.attack_mod;
	m_recoilParams.first_attack			*= recoilMod.first_attack_mod;
	m_recoilParams.back_impulse			*= recoilMod.back_impulse_mod;
	m_recoilParams.decay				*= recoilMod.decay_mod;
	m_recoilParams.end_decay			*= recoilMod.end_decay_mod;
	m_recoilParams.impulse				*= recoilMod.impulse_mod;
	m_recoilParams.max.x				*= recoilMod.max_mod.x;
	m_recoilParams.max.y				*= recoilMod.max_mod.y;
	m_recoilParams.max_recoil			*= recoilMod.max_recoil_mod;
	m_recoilParams.recoil_crouch_m		*= recoilMod.recoil_crouch_m_mod;
	m_recoilParams.recoil_jump_m		*= recoilMod.recoil_jump_m_mod;
	m_recoilParams.recoil_powerActive_m	*= recoilMod.recoil_powerActive_m_mod;
	m_recoilParams.recoil_leaning_m		*= recoilMod.recoil_leaning_m_mod;
}



void CRecoil::ResetRecoilMod(const SRecoilParams& originalRecoilParams)
{
	m_recoilParams = originalRecoilParams;
}



bool CRecoil::IsSingleFireMode() const
{
	return (strcmpi(m_pFireMode->GetType(), "Single") == 0);
}

bool CRecoil::IsSuitInActivePowerMode(const CActor& weaponOwner) const
{
	const SNanoSuitGameParameters& suitParams = weaponOwner.GetActorSuitGameParameters();

	return ((suitParams.GetMode() == eNanoSuitMode_Power) && (suitParams.IsSuitPowerActive()));
}

void CRecoil::Init( CWeapon* pWeapon, IFireMode *pFiremode )
{
	assert(pWeapon);
	assert(pFiremode);

	m_pWeapon = pWeapon;
	m_pFireMode = pFiremode;

	m_singleShot = IsSingleFireMode();
}