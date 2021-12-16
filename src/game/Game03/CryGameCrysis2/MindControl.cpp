/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:
Part of Player that handles 'mind controlling', the act of temporarily
taking over the control of an otherwise AI-controlled CPlayer.

-------------------------------------------------------------------------
History:
- 22:10:2009: Created by Sven Van Soom

*************************************************************************/
#include "StdAfx.h"

#if ENABLE_MINDCONTROL

#include "Player.h"
#include "PlayerInput.h"
#include "MindControl.h"
#include "MindControlInput.h"


//-----------------------------------------------------------------------
CMindControlMaster::~CMindControlMaster()
{
	if (m_isBusy)
		Leave();
}


//-----------------------------------------------------------------------
bool CMindControlMaster::CanExecuteOn(const CPlayer* pSlave) const
{
	if (!pSlave->GetEntity()->GetAI())
		return false;

	return true;
}


//-----------------------------------------------------------------------
void CMindControlMaster::Enter(CMindControlSlave* pSlaveMind)
{
	CRY_ASSERT_MESSAGE(!m_isBusy, "Mind control should not be initiated while it is already in progress");
	if (m_isBusy)
		return;

	CRY_ASSERT(pSlaveMind);
	if (!pSlaveMind)
		return;

	m_player.m_pPlayerInput.reset( new CAIInput(&m_player) ); // replace the PlayerInput by the AIInput, which does nothing (would-be-nice to have a Dummy input instead)

	pSlaveMind->Enter(this);

	m_pSlaveMind = pSlaveMind;
	m_isBusy = true;

	// Update visibility to change render mode of 1st person character
	m_player.UpdateVisibility();
}

//-----------------------------------------------------------------------
void CMindControlMaster::Leave()
{
	CRY_ASSERT(m_isBusy);
	if (!m_isBusy)
		return;

	CRY_ASSERT(m_pSlaveMind);
	if (!m_pSlaveMind)
		return;

	m_pSlaveMind->Leave();

	m_player.m_pPlayerInput.reset();

	// Reset State
	m_isBusy = false;
	m_pSlaveMind = NULL;

	// Update visibility to reset render mode of 1st person character
	m_player.UpdateVisibility();
}

IPlayerInput* CMindControlMaster::CreateInputToControl(CPlayer* pSlave) const
{
	return new CMindControlInput(pSlave, &m_player);
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
CMindControlSlave::~CMindControlSlave()
{
	if (m_isBusy)
		m_pMasterMind->Leave();
}


//-----------------------------------------------------------------------
void CMindControlSlave::Enter(CMindControlMaster* pMasterMind)
{
	CRY_ASSERT_MESSAGE(!m_isBusy, "Mind control should not be initiated while it is already in progress");
	if (m_isBusy)
		return;

	m_pMasterMind = pMasterMind;
	m_isBusy = true;

	// Disable AI
	m_player.GetEntity()->GetAI()->Event(AIEVENT_DISABLE, 0);

	m_player.m_pPlayerInput.reset( pMasterMind->CreateInputToControl(&m_player) );
}

//-----------------------------------------------------------------------
void CMindControlSlave::Leave()
{
	CRY_ASSERT(m_isBusy);
	if (!m_isBusy)
		return;

	m_player.m_pPlayerInput.reset();

	// Enable AI
	m_player.GetEntity()->GetAI()->Event(AIEVENT_ENABLE, 0);

	// Reset State
	m_isBusy = false;
	m_pMasterMind = NULL;
}

#endif // ENABLE_MINDCONTROL
