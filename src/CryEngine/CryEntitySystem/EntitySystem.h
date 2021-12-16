////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   EntitySystem.h
//  Version:     v1.00
//  Created:     24/5/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __EntitySystem_h__
#define __EntitySystem_h__
#pragma once

#include "IEntitySystem.h"
#include <ISystem.h>
#include "ITimer.h"
#include "SaltBufferArray.h"					// SaltBufferArray<>
#include "EntityTimeoutList.h"
#include <StlUtils.h>
#include "STLPoolAllocator.h"

//////////////////////////////////////////////////////////////////////////
// forward declarations.
//////////////////////////////////////////////////////////////////////////
class  CEntity;
struct ICVar;
struct IPhysicalEntity;
class  CEntityClassRegistry;
class  CScriptBind_Entity;
class  CPhysicsEventListener;
class  CAreaManager;
class  CBreakableManager;
class  CEntityArchetypeManager;
class  CPartitionGrid;
class  CProximityTriggerSystem;
class  CEntityLayer;
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
typedef stl::hash_map<EntityId,CEntity*> EntityMap;



//////////////////////////////////////////////////////////////////////////
struct SEntityTimerEvent
{
	EntityId entityId;
	int nTimerId;
	int nMilliSeconds;
};


typedef CSaltHandle<unsigned short,unsigned short> CEntityHandle;

//////////////////////////////////////////////////////////////////////
class CEntitySystem : public IEntitySystem
{
public:
	CEntitySystem(ISystem *pSystem);
	~CEntitySystem();

	bool Init(ISystem *pSystem);

	// interface IEntitySystem ------------------------------------------------------

	virtual void Release();
	VIRTUAL void Update();
	VIRTUAL void DeletePendingEntities();
	VIRTUAL void PrePhysicsUpdate();
	VIRTUAL void Reset();
	VIRTUAL IEntityClassRegistry* GetClassRegistry();
	VIRTUAL IEntity* SpawnEntity( SEntitySpawnParams &params,bool bAutoInit=true );
	VIRTUAL bool InitEntity( IEntity* pEntity,SEntitySpawnParams &params );
	VIRTUAL IEntity* GetEntity( EntityId id ) const;
	VIRTUAL IEntity* FindEntityByName( const char *sEntityName ) const;
	VIRTUAL void ReserveEntityId( const EntityId id );
	VIRTUAL void RemoveEntity( EntityId entity,bool bForceRemoveNow=false );
	VIRTUAL uint32 GetNumEntities() const;
	VIRTUAL IEntityIt* GetEntityIterator();
	VIRTUAL void SendEventToAll( SEntityEvent &event );
	VIRTUAL int QueryProximity( SEntityProximityQuery &query );
	VIRTUAL void ResizeProximityGrid( int nWidth,int nHeight );
	VIRTUAL int GetPhysicalEntitiesInBox( const Vec3 &origin, float radius,IPhysicalEntity **&pList, int physFlags ) const;
	VIRTUAL IEntity* GetEntityFromPhysics( IPhysicalEntity *pPhysEntity ) const;
	//VIRTUAL IEntity* GetEntityFromPhysics( IPhysicalEntity *pPhysEntity ); //Diesel new
	VIRTUAL void AddSink( IEntitySystemSink *pSink );
	VIRTUAL void RemoveSink( IEntitySystemSink *pSink );
	VIRTUAL void PauseTimers(bool bPause,bool bResume=false);
	VIRTUAL bool IsIDUsed( EntityId nID ) const;
	VIRTUAL void GetMemoryStatistics(ICrySizer *pSizer) const;
	VIRTUAL ISystem* GetSystem() const { return m_pISystem; };
	VIRTUAL void SetNextSpawnId(EntityId id);
	VIRTUAL void ResetAreas();

	VIRTUAL void AddEntityEventListener( EntityId nEntity,EEntityEvent event,IEntityEventListener *pListener );
	VIRTUAL void RemoveEntityEventListener( EntityId nEntity,EEntityEvent event,IEntityEventListener *pListener );

	VIRTUAL EntityId FindEntityByGuid( const EntityGUID &guid ) const;

	VIRTUAL IEntityArchetype* LoadEntityArchetype( XmlNodeRef oArchetype);
	VIRTUAL IEntityArchetype* LoadEntityArchetype( const char *sArchetype );
	VIRTUAL IEntityArchetype* CreateEntityArchetype( IEntityClass *pClass,const char *sArchetype );

	VIRTUAL void Serialize(TSerialize ser);

  VIRTUAL void DumpEntities();  

	VIRTUAL void LockSpawning(bool lock) {m_bLocked = lock;}

	VIRTUAL void AddLayer(const char* name, const char* parent);
	VIRTUAL void LoadLayers(const char* dataFile);
	VIRTUAL void AddEntityToLayer(const char* layer, IEntity* entity);
	VIRTUAL void ClearLayers();
	VIRTUAL void EnableLayer(const char* layer, bool isEnable);
	CEntityLayer* FindLayer(const char* layer);

	// ------------------------------------------------------------------------

	bool OnBeforeSpawn(SEntitySpawnParams &params);

	// Sets new entity timer event.
	void AddTimerEvent( SEntityTimerEvent &event, CTimeValue startTime = gEnv->pTimer->GetFrameStartTime());

	//////////////////////////////////////////////////////////////////////////
	// Load entities from XML.
	void LoadEntities( XmlNodeRef &objectsNode );

	//////////////////////////////////////////////////////////////////////////
	// Called from CEntity implementation.
	//////////////////////////////////////////////////////////////////////////
	void RemoveTimerEvent( EntityId id,int nTimerId );
	void ChangeEntityNameRemoveTimerEvent( EntityId id );

	// Puts entity into active list.
	void ActivateEntity( CEntity *pEntity,bool bActivate );
	void ActivatePrePhysicsUpdateForEntity( CEntity *pEntity,bool bActivate );
	bool IsPrePhysicsActive( CEntity * pEntity );
	void OnEntityEvent( CEntity *pEntity,SEntityEvent &event );

	// Access to class that binds script to entity functions.
	// Used by Script proxy.
	CScriptBind_Entity* GetScriptBindEntity() { return m_pEntityScriptBinding; };

	// Access to area manager.
	IAreaManager* GetAreaManager() const { return (IAreaManager*)(m_pAreaManager); }

	// Access to breakable manager.
	VIRTUAL IBreakableManager* GetBreakableManager() const { return m_pBreakableManager; };

	static ILINE uint16 IdToIndex(const EntityId id) { return id & 0xffff; }
	static ILINE CSaltHandle<> IdToHandle( const EntityId id ) { return CSaltHandle<>(id>>16,id&0xffff); }
	static ILINE EntityId HandleToId( const CSaltHandle<> id ) { return (((uint32)id.GetSalt())<<16) | ((uint32)id.GetIndex()); }

	void RegisterEntityGuid( const EntityGUID &guid,EntityId id );
	void UnregisterEntityGuid( const EntityGUID &guid );

	CPartitionGrid* GetPartitionGrid() { return m_pPartitionGrid; }
	CProximityTriggerSystem * GetProximityTriggerSystem() { return m_pProximityTriggerSystem;	}

	void ChangeEntityName( CEntity *pEntity,const char *sNewName );

	ILINE CEntity* GetEntityFromID( EntityId id ) const { return m_EntityArray[IdToHandle(id).GetIndex()]; }

private: // -----------------------------------------------------------------
	void DoUpdateLoop();

//	void InsertEntity( EntityId id,CEntity *pEntity );
	void DeleteEntity( IEntity *entity );
	void UpdateEntity(CEntity *ce,SEntityUpdateContext &ctx);
	void UpdateDeletedEntities();

	void UpdateNotSeenTimeouts();

	void OnBind( EntityId id,EntityId child );
	void OnUnbind( EntityId id,EntityId child );

	void UpdateTimers();
	void DebugDraw( CEntity *pEntity,float fUpdateTime );

	void ClearEntityArray();

  void DumpEntity(IEntity* pEntity);

	void UpdateTempActiveEntities();

	// slow - to find specific problems
	void CheckInternalConsistency() const;

	//////////////////////////////////////////////////////////////////////////
	// Variables.
	//////////////////////////////////////////////////////////////////////////

	typedef std::list<IEntitySystemSink*> SinkList;
	typedef std::list<CEntity*>	DeletedEntities;
	typedef std::multimap<CTimeValue,SEntityTimerEvent,std::less<CTimeValue>,stl::STLPoolAllocator<std::pair<CTimeValue,SEntityTimerEvent>,stl::PoolAllocatorSynchronizationSinglethreaded> > EntityTimersMap;
	typedef std::multimap<const char*,EntityId,stl::less_stricmp<const char*> > EntityNamesMap;
	typedef std::map<EntityId,CEntity*> EntitiesMap;
	typedef std::set<EntityId> EntitiesSet;
	typedef std::vector<SEntityTimerEvent> EntityTimersVector;

	SinkList												m_lstSinks;							// registered sinks get callbacks for creation and removal
	ISystem *												m_pISystem;
	std::vector<CEntity *>					m_EntityArray;					// [id.GetIndex()]=CEntity
	DeletedEntities									m_deletedEntities;
	EntitiesMap                     m_mapActiveEntities;		// Map of currently active entities (All entities that need per frame update).
	bool                            m_tempActiveEntitiesValid; // must be set to false whenever m_mapActiveEntities is changed
	EntitiesSet                     m_mapPrePhysicsEntities; // map of entities requiring pre-physics activation

	EntityNamesMap                  m_mapEntityNames;  // Map entity name to entity ID.

	CSaltBufferArray<>							m_EntitySaltBuffer;			// used to create new entity ids (with uniqueid=salt)
	std::vector<EntityId>           m_tempActiveEntities;   // Temporary array of active entities.

	//////////////////////////////////////////////////////////////////////////

	// Entity timers.
	EntityTimersMap									m_timersMap;
	EntityTimersVector							m_currentTimers;
	bool														m_bTimersPause;
	CTimeValue											m_nStartPause;

	// Binding entity.
	CScriptBind_Entity *						m_pEntityScriptBinding;

	// Entity class registry.
	CEntityClassRegistry *					m_pClassRegistry;
	CPhysicsEventListener *					m_pPhysicsEventListener;

	CAreaManager *									m_pAreaManager;

	// There`s a map of entity id to event listeners for each event.
	typedef std::multimap<EntityId,IEntityEventListener*> EventListenersMap;
	EventListenersMap m_eventListeners[ENTITY_EVENT_LAST];

	typedef std::map<EntityGUID,EntityId> EntityGuidMap;
	EntityGuidMap m_guidMap;

	IBreakableManager* m_pBreakableManager;
	CEntityArchetypeManager *m_pEntityArchetypeManager;

	// Partition grid used by the entity system
	CPartitionGrid *m_pPartitionGrid;
	CProximityTriggerSystem* m_pProximityTriggerSystem;

	EntityId m_idForced;

	//don't spawn any entities without being forced to
	bool		m_bLocked;

	CEntityTimeoutList m_entityTimeoutList;

	friend class CEntityItMap;
	class CCompareEntityIdsByClass;

	// helper to satisfy GCC
	static IEntityClass * GetClassForEntity( CEntity * );

	std::map<string, CEntityLayer*> m_layers;

	//////////////////////////////////////////////////////////////////////////
	// Pool Allocators.
	//////////////////////////////////////////////////////////////////////////
public:
	bool m_bReseting;
	//////////////////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////
// Precache resources mode state.
//////////////////////////////////////////////////////////////////////////
extern bool gPrecacheResourcesMode;

#endif // __EntitySystem_h__
