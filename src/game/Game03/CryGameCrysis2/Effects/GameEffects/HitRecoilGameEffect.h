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

#pragma once

#ifndef HIT_RECOIL_GAME_EFFECT_H
#define HIT_RECOIL_GAME_EFFECT_H

#include "GameEffect.h"


class CPlayer;
class CProjectile;
struct HitInfo;



struct SHitRecoilGameEffectParams : public SGameEffectParams
{
	SHitRecoilGameEffectParams() {}
};



class CHitRecoilGameEffect : public CGameEffect
{
private:
	struct SCameraShake
	{
		SCameraShake();

		float m_rollIntensity;
		float m_pitchIntensity;
		float m_shiftIntensity;
		float m_curveAttack;
		float m_time;
	};

	struct SForceFeedback
	{
		SForceFeedback();

		float m_time;
		float m_AmplifierS;
		float m_AmplifierA;
	};

	struct SHitRecoilParams
	{
		SHitRecoilParams();

		SCameraShake m_cameraShake;
		SForceFeedback m_forceFeedback;
		float m_minDamage;
		float m_maxDamage;
		float m_filterDelay;

		void GetMemoryUsage( ICrySizer *pSizer ) const{}
	};

public:
	CHitRecoilGameEffect();

	virtual void Initialise(const SGameEffectParams* gameEffectParams = NULL);
	virtual void Update(float frameTime);
	virtual void AddHit(CPlayer* pPlayer, IEntityClass* pProjectileClass, float damage, const char* damageTypeId, const Vec3& damageDirection);

	void Reset(const IItemParamsNode* pRootNode);

	static int GetHitRecoilId(const string& name);

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{		
		pSizer->AddObject(m_hitRecoilParams);
		pSizer->AddObject(m_hitTypeToRecoil);		
	}
private:
	void CamShake(CPlayer* pPlayer, const SCameraShake& cameraShake, float intensity, const Vec3& damageDirection);
	void ForceFeedback(const SForceFeedback& feedback, float intensity);

	typedef std::map<int, SHitRecoilParams> THitRecoilParamMap;
	THitRecoilParamMap m_hitRecoilParams;
	typedef std::map<int, int> THitTypeToRecoilMap;
	THitTypeToRecoilMap m_hitTypeToRecoil;

	float m_timeOutCounter;
};


#endif

