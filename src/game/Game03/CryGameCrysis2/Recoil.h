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

#pragma once

#ifndef RECOIL_H
#define RECOIL_H

#include "ItemSystem.h"
#include "ItemParamReader.h"


class CWeapon;
class CActor;
struct IFireMode;


struct SRecoilParams
{
	SRecoilParams();
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true);

	float max_recoil;
	float attack;
	float first_attack;
	float decay;
	float end_decay;
	Vec2 max;
	float randomness;
	std::vector<Vec2> hints;
	float impulse;
	float angular_impulse;
	float back_impulse;
	int hint_loop_start;

	//Stance modifiers
	float recoil_crouch_m;
	float recoil_jump_m;
	float recoil_leaning_m;

	//Nano suit modifiers
	float recoil_powerActive_m;

	void GetMemoryUsage(ICrySizer * s) const;

};


struct SProceduralRecoilParams
{
	SProceduralRecoilParams();

	void Reset(const IItemParamsNode *params=0, bool defaultInit=true);

	float		duration;
	float		strength;
	bool enabled;

	void GetMemoryStatistics(ICrySizer * s) const;
};


struct SSpreadParams
{
	SSpreadParams();
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true);

	float min;
	float max;
	float attack;
	float decay;
	float end_decay;
	float speed_m;
	float speed_powerActive_m;
	float rotation_m;

	//Stance modifiers
	float spread_crouch_m;
	float spread_jump_m;
	float spread_finalBreath_m;
	float spread_powerActive_m;
	float spread_leaning_m;

	void GetMemoryUsage(ICrySizer * s) const {}
};



struct SSpreadModParams
{
	SSpreadModParams();
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true);

	float min_mod;
	float max_mod;
	float attack_mod;
	float decay_mod;
	float speed_m_mod;
	float speed_powerActive_m_mod;
	float rotation_m_mod;

	//Stance modifiers
	float spread_crouch_m_mod;
	float spread_jump_m_mod;
	float spread_powerActive_m_mod;
	float spread_leaning_m_mod;

	void GetMemoryUsage(ICrySizer* s) const {}
};



struct SRecoilModParams
{
	SRecoilModParams();
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true);

	float max_recoil_mod;
	float attack_mod;
	float first_attack_mod;
	float decay_mod;
	float end_decay_mod;
	Vec2 max_mod;
	float impulse_mod;
	float angular_impulse_mod;
	float back_impulse_mod;

	//Stance modifiers
	float recoil_crouch_m_mod;
	float recoil_jump_m_mod;
	float recoil_leaning_m_mod;

	//Nano suit modifiers
	float recoil_powerActive_m_mod;

	void GetMemoryUsage(ICrySizer* s) const {}
};



class CRecoil
{
public:
	CRecoil();

	void Init(CWeapon* pWeapon, IFireMode* pFiremode);
	void Reset(bool spread);

	void Update(float frameTime, bool weaponFired, int frameId, bool firstShot);
	void SetRecoilMultiplier(float multiplier) { m_recoilMultiplier = multiplier; }
	float GetRecoilmultiplier() const { return m_recoilMultiplier; }
	float GetRecoil() const { return m_recoil; }
	float GetSpread() const;

	float GetMinSpread() const { return m_spreadParams.min; }
	float GetMaxSpread() const { return m_spreadParams.max; }
	float GetBackImpulse() const { return m_recoilParams.back_impulse; }
	float GetAngularImpulse() const { return m_recoilParams.angular_impulse; }

	void RecoilImpulse(const Vec3& firingPos, const Vec3& firingDir);

	void GetMemoryUsage(ICrySizer* s) const;


	void PatchSpreadMod(const SSpreadModParams& spreadMod);
	void ResetSpreadMod(const SSpreadParams& originalSpreadParams);
	void PatchRecoilMod(const SRecoilModParams& recoilMod);
	void ResetRecoilMod(const SRecoilParams& originalRecoilParams);

private:
	void RecoilShoot(bool firstShot);
	void UpdateRecoil(const CActor& weaponOwner, bool weaponFired, bool weaponFiring, float frameTime);
	void SpreadShoot();
	void UpdateSpread(bool weaponFired, bool weaponFiring, float frameTime);
	float GetRecoilScale(const CActor& weaponOwner) const;

	bool IsSingleFireMode() const;
	bool IsSuitInActivePowerMode(const CActor& weaponOwner) const;

	void ResetRecoilInternal();
	void ResetSpreadInternal();


	float m_recoil;
	Vec2 m_recoil_offset;
	Vec3 m_recoil_dir;
	int m_recoil_dir_idx;
	float m_spread;
	float m_recoilMultiplier;

	SRecoilParams m_recoilParams;
	SSpreadParams m_spreadParams;

	bool m_singleShot;

	IFireMode* m_pFireMode;
	CWeapon* m_pWeapon;
};


#endif
