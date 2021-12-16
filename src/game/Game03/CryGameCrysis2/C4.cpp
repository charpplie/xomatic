/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 2:3:2005   16:06 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "C4.h"
#include "Plant.h"

#include "Game.h"
#include "Actor.h"
#include "WeaponSystem.h"
#include "GameActions.h"
#include "Detonate.h"


//------------------------------------------------------------------------
CC4::CC4()
{
	m_detonateSwitch = false;
	m_detonateFM = -1;
	m_plantFM  = -1;
}


//------------------------------------------------------------------------
CC4::~CC4()
{
};

//------------------------------------------------------------------------
void CC4::InitFireModes()
{
	BaseClass::InitFireModes();

	int firemodeCount = m_firemodes.size();

	m_plantFM = -1;
	m_detonateFM = -1;

	for(int i = 0; i < firemodeCount; i++)
	{
		if(!strcmp(m_firemodes[i]->GetType(), "Plant"))
		{
			CRY_ASSERT_MESSAGE((m_plantFM == -1), "Multiple Plant firemodes assigned to weapon");

			m_plantFM = i;
		}
		else if(!strcmp(m_firemodes[i]->GetType(), "Detonate"))
		{
			CRY_ASSERT_MESSAGE((m_detonateFM == -1), "Multiple Detonate firemodes assigned to weapon");

			m_detonateFM = i;
		}
	}

	CRY_ASSERT_MESSAGE(m_detonateFM >= 0, "No Detonate firemode assigned to weapon");
	CRY_ASSERT_MESSAGE(m_plantFM >= 0, "No Plant firemode assigned to weapon");

	SetCurrentFireMode(m_plantFM);
}

//------------------------------------------------------------------------
bool CC4::OnActionAttackSecondary(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if(activationMode == eAAM_OnPress)
	{
		IFireMode* pFM = GetFireMode(m_detonateFM);

		pFM->StartFire();

		return true;
	}

	return false;
}

//------------------------------------------------------------------------
void CC4::Update(SEntityUpdateContext& ctx, int update)
{
	if(update == eIUS_FireMode)
	{
		IFireMode* pFM = GetFireMode(m_detonateFM);
		pFM->Update(ctx.fFrameTime, ctx.nFrameID);
	}

	BaseClass::Update(ctx, update);
}

//------------------------------------------------------------------------
bool CC4::CanSelect() const
{
	bool canSelect = (BaseClass::CanSelect() && !OutOfAmmo(false));

	//Check for remaining projectiles to detonate
	if(!canSelect)
	{
		IFireMode* pFM = GetFireMode(GetCurrentFireMode());
		if(pFM)
		{
			//CC4::Select will select the detonator in this case
			EntityId projectileId = pFM->GetProjectileId();
			if(projectileId && g_pGame->GetWeaponSystem()->GetProjectile(projectileId))
				return true;

		}
	}

	return canSelect;
};

//---------------------------------------------------------------------------------
bool CC4::OnActionFiremode(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	return true;
}

//---------------------------------------------------------------------------------
ColorF CC4::GetSilhouetteColor() const
{
	return ColorF(0.89411f, 0.89411f, 0.10588f, 1.0f);
}

//---------------------------------------------------------------------------------
bool CC4::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if(aspect == ASPECT_DETONATE)
	{
		ser.Value("canDet", static_cast<CC4*>(this), &CC4::NetGetCanDetonate, &CC4::NetSetCanDetonate, 'bool');
		ser.Value("detSwitch", static_cast<CC4*>(this), &CC4::NetGetDetonateSwitch, &CC4::NetSetDetonateSwitch, 'bool');
	}

	return BaseClass::NetSerialize(ser, aspect, profile, flags);
}

//---------------------------------------------------------------------------------
void CC4::NetSetDetonateSwitch(bool detonate)
{
	IActor* pActor = GetOwnerActor();
	if(pActor && !pActor->IsClient() && m_detonateSwitch != detonate)
	{
		IFireMode* pFM = GetFireMode(m_detonateFM);

		pFM->StartFire();
	}

	m_detonateSwitch = detonate;
}

//---------------------------------------------------------------------------------
bool CC4::NetGetCanDetonate() const
{
	return (m_fm->GetProjectileId() != 0);
}

//---------------------------------------------------------------------------------
void CC4::NetSetCanDetonate(bool canDetonate)
{
	CDetonate* pFM = static_cast<CDetonate*>(GetFireMode(m_detonateFM));

	if(pFM)
	{
		pFM->SetCanDetonate(canDetonate);
	}
}

//---------------------------------------------------------------------------------
void CC4::RequestDetonate()
{
	if(!gEnv->bServer)
	{
		GetGameObject()->InvokeRMI(CC4::SvRequestDetonate(), DefaultParams(), eRMI_ToServer);
	}
}

//---------------------------------------------------------------------------------
IMPLEMENT_RMI(CC4, SvRequestDetonate)
{
	NetSetDetonateSwitch(!m_detonateSwitch);
	CHANGED_NETWORK_STATE(this, ASPECT_DETONATE);

	return true;
}

