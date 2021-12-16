/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2010.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Base implementation for a take and hold objective
			- Handles keeping track of who is inside the objective area
	-------------------------------------------------------------------------
	History:
	- 10:02:2010  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_HOLD_OBJECTIVE_BASE_H_
#define _GAME_RULES_HOLD_OBJECTIVE_BASE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesModules/IGameRulesEntityObjective.h"
#include "IEntitySystem.h"
#include "GameRulesModules/IGameRulesKillListener.h"
#include "GameRulesModules/IGameRulesClientConnectionListener.h"
#include "GameRulesModules/IGameRulesTeamChangedListener.h"
#include "IGameFramework.h"
#include "Audio/AudioSignalPlayer.h"
#include "GameRulesTypes.h"

class CGameRulesHoldObjectiveBase :	public IGameRulesEntityObjective,
																		public IEntityEventListener,
																		public IGameRulesKillListener,
																		public IGameRulesClientConnectionListener,
																		public IGameRulesTeamChangedListener,
																		public IGameFrameworkListener
{
public:
	CGameRulesHoldObjectiveBase();
	~CGameRulesHoldObjectiveBase();

	// IGameRulesSimpleEntityObjective
	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void OnStartGame() {}

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual void EnableCompletion(int teamId, bool enable)	{ }
	virtual void EnableScoring(int teamId, bool enable)			{ }

	virtual bool IsComplete(int teamId) { return false; }

	virtual void AddEntityId(int type, EntityId entityId, int index, bool isNewEntity);
	virtual void RemoveEntityId(int type, EntityId entityId);
	virtual void ClearEntities(int type);
	virtual bool IsEntityFinished(int type, int index)	{ return false; }
	virtual bool CanRemoveEntity(int type, int index)		{ return true; }
	virtual void SetWaveNumber(int num, int waveCount)	{ }

	virtual void OnHostMigration(bool becomeServer) 		{ }
	// ~IGameRulesSimpleEntityObjective

	// IEntityEventListener
	virtual void OnEntityEvent( IEntity *pEntity,SEntityEvent &event );
	// ~IEntityEventListener

	// IGameRulesKillListener
	virtual void OnEntityKilled(const HitInfo &hitInfo);
	// ~IGameRulesKillListener

	// IGameRulesClientConnectionListener
	virtual void OnClientConnect(int channelId, bool isReset, EntityId playerId) {}
	virtual void OnClientDisconnect(int channelId, EntityId playerId);
	virtual void OnClientEnteredGame(int channelId, bool isReset, EntityId playerId) {}
	virtual void OnOwnClientEnteredGame();
	// ~IGameRulesClientConnectionListener

	// IGameRulesTeamChangedListener
	virtual void OnChangedTeam(EntityId entityId, int oldTeamId, int newTeamId);
	// ~IGameRulesTeamChangedListener

	// IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime) {}
	virtual void OnSaveGame(ISaveGame* pSaveGame) {}
	virtual void OnLoadGame(ILoadGame* pLoadGame) {}
	virtual void OnLevelEnd(const char* pNextLevel) {}
	virtual void OnActionEvent(const SActionEvent& event);
	// ~IGameFrameworkListener

	static const int HOLD_OBJECTIVE_MAX_ENTITIES = 10;
protected:
	static const int NUM_TEAMS = 2;
	static const int CONTESTED_TEAM_ID = -1;

	typedef std::vector<EntityId> TEntityIdVec;

	struct IHoldEntityAdditionalDetails
	{
	};

	struct SHoldEntityDetails
	{
		SHoldEntityDetails()
		{
			Reset();
		}

		void Reset()
		{
			m_id = 0;
			m_pAdditionalData = NULL;
			m_localPlayerIsWithinRange = false;
			m_controllingTeamId = 0;
			m_totalInsideBoxCount = 0;
			m_controlRadiusSqr = 25.f;
			m_controlRadius = 5.f;
			m_controlHeight = 5.f;
			m_controlOffsetZ = 0.f;

			for (int i = 0; i < NUM_TEAMS; ++ i)
			{
				m_insideCount[i] = 0;
				m_insideEntities[i].clear();
				m_insideEntities[i].reserve(16);

				m_insideBoxEntities[i].clear();
				m_insideBoxEntities[i].reserve(16);
			}
		}

		TEntityIdVec m_insideBoxEntities[NUM_TEAMS];			// Entities inside the bounding box
		TEntityIdVec m_insideEntities[NUM_TEAMS];					// Entities inside the cylinder
		IHoldEntityAdditionalDetails *m_pAdditionalData;
		EntityId m_id;
		int m_controllingTeamId;
		int m_insideCount[NUM_TEAMS];
		int m_totalInsideBoxCount;
		float m_controlRadiusSqr;
		float m_controlRadius;
		float m_controlHeight;
		float m_controlOffsetZ;
		bool m_localPlayerIsWithinRange;
	};

	enum ESpawnPOIType
	{
		eSPT_None,
		eSPT_Avoid,
	};

	// Functions to be overridden in the child class
	virtual void OnInsideStateChanged(SHoldEntityDetails *pDetails) {}
	virtual void OnNewHoldEntity(SHoldEntityDetails *pDetails, int index) {}
	virtual void OnRemoveHoldEntity(SHoldEntityDetails *pDetails) {}
	virtual void OnNetSerializeHoldEntity(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags, SHoldEntityDetails *pDetails, int index) {}
	// ~ Functions to be overridden in the child class

	// Functions that aren't really part of this class but are needed by multiple children
	void AwardPlayerPoints(TEntityIdVec *pEntityVec, EGRST scoreType);
	// ~ Functions that aren't really part of this class but are needed by multiple children

private:
	void CleanUpEntity(SHoldEntityDetails *pDetails);
	void InsideStateChanged(SHoldEntityDetails *pDetails);
	void DetermineControllingTeamId(SHoldEntityDetails *pDetails, const int team1Count, const int team2Count);
	void CheckCylinder(SHoldEntityDetails *pDetails, EntityId localPlayerId);
	void CheckLocalPlayerInside(SHoldEntityDetails *pDetails, const IEntity *pHoldEntity, const IEntity *pLocalPlayer);
	void CheckLocalPlayerInsideAllEntities();

protected:
	SHoldEntityDetails m_entities[HOLD_OBJECTIVE_MAX_ENTITIES];

	ESpawnPOIType m_spawnPOIType;
	float m_spawnPOIDistance;
};

#endif // _GAME_RULES_HOLD_OBJECTIVE_BASE_H_

