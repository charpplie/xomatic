/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Basic Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 26:02:2010   12:00 : Created by Claire Allan

*************************************************************************/
#ifndef __FIREMODE_H__
#define __FIREMODE_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Weapon.h"
#include "WeaponSharedParams.h"

class CFireMode : public IFireMode
{
public:
	CFireMode() : m_pWeapon(NULL), m_fireParams(NULL), m_parentFireParams(NULL), m_fmIdx(0) {};
	virtual ~CFireMode() {};

	virtual void Init(IWeapon* pWeapon, const IItemParamsNode* params, uint32 id) { CRY_ASSERT_MESSAGE(0, "Init(IWeapon *pWeapon, const IItemParamsNode *params, uint32 id) should never be called, use InitFiremode(IWeapon *pWeapon, const SFireMode* pParams, uint32 id)"); };
	virtual void InitFireMode(IWeapon* pWeapon, const SParentFireModeParams* pParams, uint32 id)
	{
		CRY_ASSERT_MESSAGE(pParams, "Fire Mode Params NULL! Have you set up the weapon xml correctly?");

		m_pWeapon = static_cast<CWeapon *>(pWeapon);
		m_fireParams = &pParams->baseFireMode;
		m_parentFireParams = pParams;
		m_fmIdx = id;
	}

	void ResetSharedParams(const SFireModeParams* pParams) { m_fireParams = pParams; ResetParams(NULL); }
	const SFireModeParams* GetShared() const { return m_fireParams; }
	const SParentFireModeParams* GetParentShared() const { return m_parentFireParams; }

	virtual void ResetParams(const struct IItemParamsNode *params) {};
	virtual void PatchParams(const struct IItemParamsNode *patch) {};
	virtual void ModifyParams(bool modify, bool modified = true) {};

	virtual void PatchSpreadMod(const SSpreadModParams &sSMP){};
	virtual void ResetSpreadMod(){};

	virtual void PatchRecoilMod(const SRecoilModParams &sRMP){};
	virtual void ResetRecoilMod(){};

	virtual bool CanZoom() const {return true;}

	virtual void GetMemoryUsage(ICrySizer * pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
		GetInternalMemoryUsage(pSizer);
	}
	void GetInternalMemoryUsage(ICrySizer * pSizer) const
	{
		pSizer->AddObject(m_pWeapon);
		pSizer->AddObject(m_fireParams);
		pSizer->AddObject(m_parentFireParams);
	}
protected:
	CWeapon*								m_pWeapon;
	const SFireModeParams*	m_fireParams;
	const SParentFireModeParams*	m_parentFireParams;
	uint32									m_fmIdx;
};

#endif //__FIREMODE_H__
