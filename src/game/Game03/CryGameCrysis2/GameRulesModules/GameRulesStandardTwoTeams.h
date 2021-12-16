/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 02:09:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GameRulesStandardTwoTeams_h_
#define _GameRulesStandardTwoTeams_h_

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameRulesTeamsModule.h"
#include "CryFixedString.h"

class CGameRules;

class CGameRulesStandardTwoTeams : public IGameRulesTeamsModule
{
public:
	CGameRulesStandardTwoTeams();
	virtual ~CGameRulesStandardTwoTeams();

	virtual void Init(XmlNodeRef xml);
	virtual void PostInit();
	virtual void Update(float frameTime);

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual void RequestChangeTeam(EntityId playerId, int teamId);

	virtual int GetAutoAssignTeamId(EntityId playerId);

protected:
	enum EAutoBalanceState
	{
		EABS_OFF,			// Not doing auto-balancing
		EABS_CHECKING,		// In a time-based loop, checking for unbalanced teams
		EABS_BALANCING,		// Found unbalanced team, waiting to do the actual balancing
	};

	void DoTeamChange(EntityId playerId, int teamId);

	typedef CryFixedStringT<4> TSmallString;
	TSmallString m_timeToBalance;

	CGameRules *m_pGameRules;

	float m_sleepTime;
	EAutoBalanceState m_autoBalanceState;
};

#endif // _GameRulesStandardTwoTeams_h_