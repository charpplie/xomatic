/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: DummyPlayer.cpp$
$DateTime$
Description: A dummy player used to simulate a client player for profiling purposes

-------------------------------------------------------------------------
History:
- 01/07/2010 11:15:00: Created by Martin Sherburn

*************************************************************************/

#include "StdAfx.h"
#include "DummyPlayer.h"
#include "NetPlayerInput.h"
#include "Weapon.h"
#include "GameCVars.h"
#include "GameRules.h"

CDummyPlayer::CDummyPlayer()
: m_initialised(false)
{
}

CDummyPlayer::~CDummyPlayer()
{
}

void CDummyPlayer::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	CPlayer::Update(ctx, updateSlot);

	if(!gEnv->bServer)
		return;

	if (!m_initialised)
	{
		IGameFramework *pGameFramework = gEnv->pGame->GetIGameFramework();
		IItemSystem		*pItemSystem = pGameFramework->GetIItemSystem();

		const char *itemName = "SCAR";
		//Check item name before giving (it will resolve case sensitive 'issues')
		itemName = (const char*)(pItemSystem->Query(eISQ_Find_Item_By_Name, itemName));

		pItemSystem->GiveItem(this, itemName, true, true, true);

		m_initialised = true;
	}

	if (!GetPlayerInput() || (GetPlayerInput()->GetType() != IPlayerInput::DEDICATED_INPUT))
	{
		CDedicatedInput* const pDedicatedInput = new CDedicatedInput(this);
#if !defined(_RELEASE)		
		if (g_pGameCVars->g_playerEnableDedicatedInput == false)
		{
			pDedicatedInput->SetDummyPlayerBehaviour(true);
		}
#endif // #if !defined(_RELEASE)		
		m_pPlayerInput.reset(pDedicatedInput);
	}
}

EDefaultableBool CDummyPlayer::GetFire()
{
	CDedicatedInput* pPlayerInput = (CDedicatedInput*)GetPlayerInput();
	if (pPlayerInput)
	{
		return pPlayerInput->GetFire();
	}
	return eDB_Default;
}

void CDummyPlayer::SetFire(EDefaultableBool value)
{
	CDedicatedInput* pPlayerInput = (CDedicatedInput*)GetPlayerInput();
	if (pPlayerInput)
	{
		pPlayerInput->SetFire(value);
	}
}

EDefaultableBool CDummyPlayer::GetMove()
{
	CDedicatedInput* pPlayerInput = (CDedicatedInput*)GetPlayerInput();
	if (pPlayerInput)
	{
		return pPlayerInput->GetMove();
	}
	return eDB_Default;
}

void CDummyPlayer::SetMove(EDefaultableBool value)
{
	CDedicatedInput* pPlayerInput = (CDedicatedInput*)GetPlayerInput();
	if (pPlayerInput)
	{
		pPlayerInput->SetMove(value);
	}
}
