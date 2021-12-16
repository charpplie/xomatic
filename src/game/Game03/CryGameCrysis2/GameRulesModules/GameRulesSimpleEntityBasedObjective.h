/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Class for managing a series of SimpleEntityObjectives based on xml
		parameterisation
	-------------------------------------------------------------------------
	History:
	- 20:10:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GAME_RULES_SIMPLE_ENTITY_BASED_OBJECTIVE_H_
#define _GAME_RULES_SIMPLE_ENTITY_BASED_OBJECTIVE_H_

#if _MSC_VER > 1000
# pragma once
#endif

#include "GameRulesModules/IGameRulesObjective.h"
#include "GameRulesModules/IGameRulesEntityObjective.h"
#include "GameRulesModules/IGameRulesModuleRMIListener.h"
#include "GameRulesModules/IGameRulesClientConnectionListener.h"
#include "IEntitySystem.h"

class CGameRulesSimpleEntityBasedObjective :	public IGameRulesObjective,
												public IGameRulesModuleRMIListener,
												public IGameRulesClientConnectionListener,
												public IEntitySystemSink,
												public IGameFrameworkListener
{
public:
	CGameRulesSimpleEntityBasedObjective();

	// IGameRulesObjective
	virtual ~CGameRulesSimpleEntityBasedObjective();

	virtual void Init(XmlNodeRef xml);
	virtual void Update(float frameTime);

	virtual void OnStartGame();

	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );

	virtual void Enable(int teamId, bool enable);
	virtual bool IsComplete(int teamId);

	virtual void OnHostMigration(bool becomeServer);
	// ~IGameRulesObjective
	
	// IGameRulesModuleRMIListener
	virtual void OnSingleEntityRMI(CGameRules::SModuleRMIEntityParams params);
	virtual void OnDoubleEntityRMI(CGameRules::SModuleRMITwoEntityParams params) {};
	// ~IGameRulesModuleRMIListener

	// IGameRulesClientConnectionListener
	virtual void OnClientConnect(int channelId, bool isReset, EntityId playerId) {};
	virtual void OnClientDisconnect(int channelId, EntityId playerId) {};
	virtual void OnClientEnteredGame(int channelId, bool isReset, EntityId playerId);
	virtual void OnOwnClientEnteredGame() {};
	// ~IGameRulesClientConnectionListener

	// IEntitySystemSink
	virtual bool OnBeforeSpawn( SEntitySpawnParams &params ) { return true; };
	virtual void OnSpawn( IEntity *pEntity,SEntitySpawnParams &params );
	virtual bool OnRemove( IEntity *pEntity );
	virtual void OnEvent( IEntity *pEntity, SEntityEvent &event ) {};
	// ~IEntitySystemSink

	// IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime) {}
	virtual void OnSaveGame(ISaveGame* pSaveGame) {}
	virtual void OnLoadGame(ILoadGame* pLoadGame) {}
	virtual void OnLevelEnd(const char* pNextLevel) {}
	virtual void OnActionEvent(const SActionEvent& event);
	// ~IGameFrameworkListener

	void GetMemoryUsage(class ICrySizer *pSizer) const 
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_entityDetails);		
	}
protected:
	typedef std::vector<EntityId> TEntityIdVec;

	void SvAddEntity(int type, EntityId id, int index);
	void SvRemoveEntity(int type, int index, bool sendRMI);
	void SvDoRandomSelection(int type);
	int SvResetAvailableEntityList(int type);

	void CallScriptUpdateFunction(TEntityIdVec &entitiesVec, float frameTime);

	enum EEntitySelectType
	{
		EEST_Default,
		EEST_All = EEST_Default,
		EEST_Random,
	};

	struct SEntityDetails
	{
		enum EUpdateType
		{
			eUT_None,
			eUT_All,
			eUT_Active,
		};

		SEntityDetails()
		{
			m_pEntityClass = NULL;
			m_entitySelectCount = 0;
			memset(m_entitySelectNthWaveCount, 0, sizeof(m_entitySelectNthWaveCount));
			m_nthSpawn = -1;
			m_randomChangeTimeLength = 0.f;
			m_timeToRandomChange = 0.f;
			m_timeBetweenSites = 0.f;
			m_entitySelectType = EEST_Default;
			m_updateType = eUT_None;
			m_useRandomChangeTimer = false;
			m_scan = false;
		}

		void GetMemoryUsage(class ICrySizer *pSizer) const 
		{
			pSizer->AddObject(m_allEntities);
			pSizer->AddObject(m_currentEntities);
			pSizer->AddObject(m_availableEntities);
		}
		TEntityIdVec m_allEntities;				// All entities of class 'm_pEntityClass' in the level
		TEntityIdVec m_currentEntities;			// List of entities from inside 'm_allEntities' that are currently set as active objectives
		TEntityIdVec m_availableEntities;		// List of entities from 'm_allEntities' that have not been made active (used in random select type only)
		const IEntityClass *m_pEntityClass;

		static const int MAX_ENTITY_SELECTS = 10;

		int m_entitySelectCount; // Num of entity select counts
		int m_entitySelectNthWaveCount[MAX_ENTITY_SELECTS]; // Count to spawn depending on num times respawned

		int m_nthSpawn;	// Number of times entities have respawned (all entities in count, not individual respawns)

		float m_randomChangeTimeLength;			// Time before a site is disabled (timer length)
		float m_timeToRandomChange;					// Time before a site is disabled (actual timer)
		float m_timeBetweenSites;						// Time between a site being disabled and a new one being activated

		EEntitySelectType m_entitySelectType;
		EUpdateType m_updateType;

		bool m_useRandomChangeTimer;
		bool m_scan;
	};

	typedef std::vector<SEntityDetails> TEntityDetailsVec;

	TEntityDetailsVec m_entityDetails;

	IGameRulesEntityObjective *m_pObjective;

	int m_moduleRMIIndex;
};

#endif // _GAME_RULES_SIMPLE_ENTITY_BASED_OBJECTIVE_H_
