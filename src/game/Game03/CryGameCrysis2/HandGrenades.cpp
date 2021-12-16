//---------------------------
// Hand grenades weapon
//
// 18:07:2008 - Benito G.R.
//------------------------------------

#include "StdAfx.h"
#include "HandGrenades.h"

#include "Game.h"
#include "GameActions.h"
#include "Throw.h"

CHandGrenades::CHandGrenades()
{
}

CHandGrenades::~CHandGrenades()
{
}

//-------------------------------------------------
bool CHandGrenades::OnActionAttack(EntityId actorId, const ActionId& actionId, int activationMode, float value)
{
	if(activationMode == eAAM_OnPress)
	{
		if(m_fm && !strcmp(m_fm->GetType(),"Thrown"))
		{
			CThrow* pThrow = static_cast<CThrow*>(m_fm);
			pThrow->Prime();
		}
	}
	else if (activationMode == eAAM_OnRelease)
	{
		StopFire();
	}

	return true;
}

//-------------------------------------------------
bool CHandGrenades::CanSelect() const
{
	return (inherited::CanSelect() && !OutOfAmmo(false));
}

//-------------------------------------------------
bool CHandGrenades::CanDeselect() const
{
	return (!m_fm || !m_fm->IsFiring());
}

//-------------------------------------------------
void CHandGrenades::UpdateMovementIdles(bool sprinting, float runFactor)
{
	bool update = true;

	if(m_fm && !strcmp(m_fm->GetType(),"Thrown"))
	{
		CThrow* pThrow = static_cast<CThrow*>(m_fm);
		if(pThrow->IsReadyToThrow())
		{
			update = false;
		}
	}

	if(update)
	{
		if(sprinting)
		{
			SetDefaultIdleAnimation(eIGS_Owner,"idle_sprint");
		}
		else if(runFactor > 0.6f)
		{
			SetDefaultIdleAnimation(eIGS_Owner, "idle_run");
		}
		else
		{
			SetDefaultIdleAnimation(eIGS_Owner, "idle");
		}
	}
};

void CHandGrenades::StartSprint(CActor* pOwnerActor)
{
	//don't stop firing grenades
}

bool CHandGrenades::ShouldSendOnShootHUDEvent() const
{
	return false;
}
