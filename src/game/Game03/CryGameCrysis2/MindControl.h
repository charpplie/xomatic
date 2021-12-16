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

#pragma once

#ifndef __MIND_CONTROL_H__
#define __MIND_CONTROL_H__

#if ENABLE_MINDCONTROL

class CPlayer;
struct IPlayerInput;
class CMindControlSlave;

// ---------------------------------------------------------------------------
class CMindControlMaster
{
	friend class CMindControlSlave;

public:
	explicit CMindControlMaster(CPlayer& player)
		: m_player(player), m_pSlaveMind(NULL), m_isBusy(false)
	{
	}

	~CMindControlMaster();

	ILINE bool IsBusy() const { return (m_isBusy); }
	void Enter(CMindControlSlave* pSlaveMind);
	void Leave();
	bool CanExecuteOn(const CPlayer* pSlave) const;

	CMindControlSlave* GetSlaveMind() const { return m_pSlaveMind; }

private:
	IPlayerInput* CreateInputToControl(CPlayer* pSlave) const;

private:
	CPlayer& m_player;

	CMindControlSlave* m_pSlaveMind;
	bool m_isBusy;
};


// ---------------------------------------------------------------------------
class CMindControlSlave
{
	friend class CMindControlMaster;

public:
	explicit CMindControlSlave(CPlayer& player)
		: m_player(player), m_pMasterMind(NULL), m_isBusy(false)
	{
	}

	~CMindControlSlave();

	ILINE bool IsBusy() const { return (m_isBusy); }

	// TODO: Rewrite camera capturing code so access to player isn't necessary anymore!
	CPlayer* GetPlayer() const { return &m_player; }

private:
	void Enter(CMindControlMaster* pMasterMind);
	void Leave();

private:
	CPlayer& m_player;

	CMindControlMaster* m_pMasterMind;
	bool m_isBusy;
};

#endif // ENABLE_MINDCONTROL
#endif // __MIND_CONTROL_H__
