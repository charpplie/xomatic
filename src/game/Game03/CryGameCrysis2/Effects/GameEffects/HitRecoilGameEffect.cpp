
/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:

-------------------------------------------------------------------------
History:
- 17:11:2009: Created by Filipe Amim

*************************************************************************/

#include "StdAfx.h"
#include "Effects/GameEffectsSystem.h"
#include "GameForceFeedback.h"
#include "HitRecoilGameEffect.h"
#include "GameRules.h"
#include "Player.h"
#include "Weapon.h"
#include "AmmoParams.h"
#include "Projectile.h"
#include "WeaponSystem.h"
#include "ScreenEffects.h"



// curve attack for [-1, 1] range
static float CurveAttack(float linearVal, float curvePower)
{
	return cry_powf(linearVal*0.5f+0.5f, curvePower)*2.0f-1.0f;
}


static float Sign(float v)
{
	return v < 0.0f ? -1.0f : v > 0.0f ? 1.0f : 0.0f;
}



CHitRecoilGameEffect::SCameraShake::SCameraShake()
	:	m_pitchIntensity(0.0f)
	,	m_rollIntensity(0.0f)
	,	m_curveAttack(1.0f)
	,	m_time(0.0f)
{
}



CHitRecoilGameEffect::SForceFeedback::SForceFeedback()
	:	m_time(0.0f)
	,	m_AmplifierS(0.0f)
	,	m_AmplifierA(0.0f)
{
}



CHitRecoilGameEffect::SHitRecoilParams::SHitRecoilParams()
	:	m_minDamage(0.0f)
	,	m_maxDamage(1.0f)
	,	m_filterDelay(0.0f)
{
}



CHitRecoilGameEffect::CHitRecoilGameEffect()
	:	m_timeOutCounter(0.0f)
{
}



void CHitRecoilGameEffect::Initialise(const SGameEffectParams* gameEffectParams)
{
	CGameEffect::Initialise(gameEffectParams);
}



void CHitRecoilGameEffect::Update(float frameTime)
{
	m_timeOutCounter -= frameTime;

	if (m_timeOutCounter < 0)
		SetActive(false);
}



void CHitRecoilGameEffect::AddHit(CPlayer* pPlayer, IEntityClass *pProjectileClass, float damage, const char* damageTypeId, const Vec3& damageDirection)
{
	if (m_timeOutCounter > 0)
		return;

	int hitRecoilType = -1;
	if(pProjectileClass)
	{
		const SAmmoParams *projectileParams = g_pGame->GetWeaponSystem()->GetAmmoParams(pProjectileClass);
		const bool armorModeActive = pPlayer->GetNanoSuit()->GetGameParams().GetMode()==eNanoSuitMode_Armor;
		hitRecoilType = armorModeActive ? projectileParams->m_hitRecoilIdArmorMode : projectileParams->m_hitRecoilId;
	}
	else
	{
		CGameRules *pGameRules = g_pGame->GetGameRules();
		int hitTypeId = pGameRules ? pGameRules->GetHitTypeId(damageTypeId): 0;
		if(hitTypeId != 0)
		{
			THitTypeToRecoilMap::iterator it = m_hitTypeToRecoil.find(hitTypeId);
			if (it == m_hitTypeToRecoil.end())
				return;

			hitRecoilType = it->second;
		}
	}

	THitRecoilParamMap::iterator it = m_hitRecoilParams.find(hitRecoilType);
	if (it == m_hitRecoilParams.end())
		return;
	const SHitRecoilParams& hitRecoilParams = it->second;

	const float effectIntensity = SATURATE((damage - hitRecoilParams.m_minDamage) / (hitRecoilParams.m_maxDamage - hitRecoilParams.m_minDamage));
	CamShake(pPlayer, hitRecoilParams.m_cameraShake, effectIntensity, damageDirection);
	ForceFeedback(hitRecoilParams.m_forceFeedback, effectIntensity);

	m_timeOutCounter += hitRecoilParams.m_filterDelay;

	SetActive(true);
}



void CHitRecoilGameEffect::CamShake(CPlayer* pPlayer, const SCameraShake& cameraShake, float intensity, const Vec3& damageDirection)
{
	const Vec3 localRecoilDirection = -(Matrix33(pPlayer->GetEntity()->GetWorldTM().GetInverted()) * damageDirection);
	const Vec3 shakeShiftVec = Vec3(localRecoilDirection.x, 0, 0) * cameraShake.m_shiftIntensity * intensity;
	Vec3 shakeRotationAngles(ZERO);
	const float fSign = (float)__fsel(localRecoilDirection.y, 1.0f, -1.0f);
	shakeRotationAngles.x = CurveAttack(fabsf(localRecoilDirection.y), cameraShake.m_curveAttack) * fSign;
	shakeRotationAngles.y = (1.0f-fabsf(shakeRotationAngles.x)) * Sign(-localRecoilDirection.x);
	shakeRotationAngles.x *= cameraShake.m_pitchIntensity * intensity;
	shakeRotationAngles.y *= cameraShake.m_rollIntensity * intensity;

	if (g_pGame->GetScreenEffects())
		g_pGame->GetScreenEffects()->CamShake(shakeRotationAngles, shakeShiftVec, cameraShake.m_time, cameraShake.m_time, 0.0f, CScreenEffects::eCS_GID_HitRecoil);
}



void CHitRecoilGameEffect::ForceFeedback(const SForceFeedback& feedback, float intensity)
{
	GameForceFeedback_Always(
		SFFOutputEvent(
		eDI_XI, eFF_Rumble_Basic,
		feedback.m_time,
		feedback.m_AmplifierS * intensity,
		feedback.m_AmplifierA * intensity));
}



int CHitRecoilGameEffect::GetHitRecoilId(const string& name)
{
	static std::map<string, int> registration;
	std::map<string, int>::iterator i = registration.find(name);
	if (i == registration.end())
	{
		int id = registration.size();
		registration[name] = id;
		return id;
	}
	return i->second;
}



void CHitRecoilGameEffect::Reset(const IItemParamsNode* pRootNode)
{
	const IItemParamsNode* pParams = pRootNode->GetChild("HitRecoilTable");
	if (!pParams)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();

#ifdef _DEBUG
	// This code calls pGameRules->GetHitTypeId which (in debug builds) asserts if it fails to find the hit type specified.
	// Calling this stops the asserts from happening... [TF]

	bool old = CGameRules::DbgSetAssertOnFailureToFindHitType(false);
#endif

	for (int i = 0; i < pParams->GetChildCount(); ++i)
	{
		const IItemParamsNode* pHitRecoil = pParams->GetChild(i);
		if (strcmp(pHitRecoil->GetName(), "HitRecoil") != 0)
			continue;

		string name;
		name = pHitRecoil->GetAttribute("name");
		int id = GetHitRecoilId(name);
		SHitRecoilParams hitRecoilParams;

		pHitRecoil->GetAttribute("shakeCurveAttack", hitRecoilParams.m_cameraShake.m_curveAttack);
		pHitRecoil->GetAttribute("shakeShiftIntensity", hitRecoilParams.m_cameraShake.m_shiftIntensity);
		pHitRecoil->GetAttribute("shakePitchIntensity", hitRecoilParams.m_cameraShake.m_pitchIntensity);
		pHitRecoil->GetAttribute("shakeRollIntensity", hitRecoilParams.m_cameraShake.m_rollIntensity);
		pHitRecoil->GetAttribute("shakeTime", hitRecoilParams.m_cameraShake.m_time);
		pHitRecoil->GetAttribute("ffAmplifierA", hitRecoilParams.m_forceFeedback.m_AmplifierS);
		pHitRecoil->GetAttribute("ffAmplifierB", hitRecoilParams.m_forceFeedback.m_AmplifierA);
		pHitRecoil->GetAttribute("ffTime", hitRecoilParams.m_forceFeedback.m_time);
		pHitRecoil->GetAttribute("minDamage", hitRecoilParams.m_minDamage);
		pHitRecoil->GetAttribute("maxDamage", hitRecoilParams.m_maxDamage);
		pHitRecoil->GetAttribute("filterDelay", hitRecoilParams.m_filterDelay);

		m_hitRecoilParams[id] = hitRecoilParams;

		if(pGameRules)
		{
			int hitTypeId = pGameRules->GetHitTypeId(name.c_str());
			if(hitTypeId != 0)
			{
				m_hitTypeToRecoil[hitTypeId] = id;
			}
		}
	}

#ifdef _DEBUG
	CGameRules::DbgSetAssertOnFailureToFindHitType(old);
#endif
}
