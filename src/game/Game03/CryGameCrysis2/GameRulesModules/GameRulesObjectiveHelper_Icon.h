/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Helper objective, manages placing the correct icon over an entity
	-------------------------------------------------------------------------
	History:
	- 04:12:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_OBJECTIVE_HELPER_ICON_H_
#define _GAME_RULES_OBJECTIVE_HELPER_ICON_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesModules/IGameRulesObjective.h"
#include "GameRulesModules/IGameRulesClientConnectionListener.h"
#include "GameRulesModules/IGameRulesTeamChangedListener.h"
#include "Utility/CryFixedArray.h"
#include "GameRulesTypes.h"

class CGameRulesObjectiveHelper_Icon :
										public IGameRulesObjective,
										public IGameRulesClientConnectionListener,
										public IGameRulesTeamChangedListener
{
public:
	CGameRulesObjectiveHelper_Icon();

	// IGameRulesObjective
	virtual ~CGameRulesObjectiveHelper_Icon();

	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime) {};

	virtual void OnStartGame();

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags ) { return true; };

	virtual void Enable(int teamId, bool enable) {};
	virtual bool IsComplete(int teamId) { return true; };

	virtual void OnHostMigration(bool becomeServer) {}
	// ~IGameRulesObjective

	// IGameRulesClientConnectionListener
	virtual void OnClientConnect(int channelId, bool isReset, EntityId playerId) {};
	virtual void OnClientDisconnect(int channelId, EntityId playerId) {};
	virtual void OnClientEnteredGame(int channelId, bool isReset, EntityId playerId) {};
	virtual void OnOwnClientEnteredGame();
	// ~IGameRulesClientConnectionListener

	// IGameRulesTeamChangedListener
	virtual void OnChangedTeam(EntityId entityId, int oldTeamId, int newTeamId);
	// ~IGameRulesTeamChangedListener

protected:

	struct SEntityDetails
	{
		SEntityDetails()
		{
			m_currentIcon = int(EGRMO_Unknown);
			m_id = 0;
		}

		int m_currentIcon;
		EntityId m_id;
	};

	typedef CryFixedArray<SEntityDetails, 16> TEntityDetailsArray;

	void UpdateIcon(SEntityDetails *pEntityDetails);
	void UpdateAllIcons();

	TEntityDetailsArray m_entities;

	int m_neutralIcon;
	int m_friendlyIcon;
	int m_hostileIcon;

	IEntityClass *m_pEntityClass;
};

#endif // _GAME_RULES_OBJECTIVE_HELPER_ICON_H_
