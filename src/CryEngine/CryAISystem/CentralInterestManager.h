/********************************************************************
StalTech Source File.
Copyright  , StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   CentralInterestManager.cpp
$Id$
$DateTime$
Description: Global manager that distributes interest events and
             allows personal managers to work together
						 Effectively, this is the interface to the Interest System
---------------------------------------------------------------------
History:
- 06:03:2007 : Created by Matthew Jack

*********************************************************************/

#ifndef __CentralInterestManager_H__
#define __CentralInterestManager_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "PersonalInterestManager.h"

// Forward declarations
struct IPersistantDebug;

// Basic structure for storing which objects are interesting and how much
struct SEntityInterest
{
	SEntityInterest()
		: entityId(0), fRadius(0.0f), fInterest(0.0f), actionName("") {}

	SEntityInterest(EntityId eid, float fRadius, float fInterest, const char* sActionName)
		: entityId(eid), fRadius(fRadius), fInterest(fInterest), actionName(sActionName) 
	{
		gAIEnv.pSmartObjectManager->AddSmartObjectState(GetEntity() , "Registered" );
	}

	bool Set(EntityId eid, float radius, float interest, const char* action);

	const char * GetAction() const    { return actionName.empty() ? actionName.c_str() : "None"; }
	IEntity const * GetEntity() const { return gEnv->pEntitySystem->GetEntity(entityId); }
	IEntity * GetEntity()							{ return gEnv->pEntitySystem->GetEntity(entityId); }
	bool IsValid() const							{ return(entityId > 0); }
	void Invalidate()									
	{ 
		if(GetEntity())
			gAIEnv.pSmartObjectManager->RemoveSmartObjectState(GetEntity() , "Registered" ); 
		entityId = 0; 
	}

	EntityId entityId;
	float fRadius;
	float fInterest;
	string actionName;
};

// There need only ever be one of these in game, hence a singleton
// Creates on first request, which is lightweight

class CCentralInterestManager : 
	public ICentralInterestManager,
	public IEntitySystemSink,
	public IEntityEventListener
{
public:
	// Get the CIM singleton
	static CCentralInterestManager* GetInstance(void);

	// Reset the CIM system
	// Do this when all caches should be emptied, e.g. on loading levels
	// Resetting during game should do no serious harm
	void Reset();

	// Enable or disable (suppress) the interest system
	bool Enable(bool bEnable);

	// Is the Interest System enabled?
	bool IsEnabled(void) { return m_bEnabled; }

	bool IsDebuggingEnabled(void) { return (m_cvDebugInterest ? m_cvDebugInterest->GetIVal() != 0 : false); }

	// Update the CIM
	void Update( float fDelta );

	// Register/update an interesting entity
	// Returns true if this was accepted as a useful interest target
	// If the object is already registered, it will be updated and still return true
	bool RegisterInterestingEntity( IEntity *pEntity, float fRadius = -1.0f, float fBaseInterest = -1.0f, const char* actionName = 0 );

	// Deregister an interesting entity
	void DeregisterInterestingEntity( IEntity *pEntity );

	// Register a potential interested AIActor
	// Returns true if this was accepted as a potentially useful interested entity
	// If the object is already registered, it will be updated and still return true
	bool RegisterInterestedAIActor( IEntity *pEntity, bool bEnablePIM, float fInterestFilter = -1.0f, float fAngle = -1.0f );

	// Deregister a potential interested AIActor
	bool DeregisterInterestedAIActor( IEntity *pEntity );

	// Central shared debugging function, should really be a private/friend of PIM
	void AddDebugTag( EntityId id, const char *pStr, float fTime = -1.0f);

	// Add a class to be notified when an entity is interested or interesting
	void RegisterListener( IInterestListener* pClass, EntityId idInterestingEntity );
	void UnRegisterListener( IInterestListener* pClass, EntityId idInterestingEntity );

	//-------------------------------------------------------------
	// IEntitySystemSink methods, for listening to moving entities
	//-------------------------------------------------------------
	bool OnBeforeSpawn( SEntitySpawnParams &params ) { return true; }
	void OnSpawn( IEntity *pEntity,SEntitySpawnParams &params );
	bool OnRemove( IEntity *pEntity );
	void OnEvent( IEntity *pEntity, SEntityEvent &event ) {}

	// IEntityEventListener
	void OnEntityEvent( IEntity *pEntity,SEntityEvent &event );
	// ~IEntityEventListener

	// Expose for DebugDraw
	typedef std::vector<CPersonalInterestManager> TVecPIMs;
	const TVecPIMs* GetPIMs() const { return &m_PIMs; }

	// Expose for PIMs
	typedef std::vector<SEntityInterest> TVecInteresting;
	TVecInteresting* GetInterestingEntities() { return &m_InterestingEntities; }
	TVecInteresting const * GetInterestingEntities() const { return &m_InterestingEntities; }

	CPersonalInterestManager* FindPIM(IEntity* pEntity);

protected:
	friend class CPersonalInterestManager;

	void OnEntityInteresting( EntityId idActor, EntityId idInterestingEntity, bool bInteresting );

private:
	// Construct/destruct
	CCentralInterestManager(void);
	~CCentralInterestManager(void);

	// Accept objects for PIMs to consider
	void DeregisterObject(IEntity * pEntity);

	void RegisterFromTable( IEntity *pEntity, const SmartScriptTable& Table );
	void RegisterEntityFromTable( IEntity *pEntity, const SmartScriptTable& Table );
	void RegisterActorFromTable( IEntity *pEntity, const SmartScriptTable& Table );

	void RefreshInterestingEntityData(SEntityInterest& InterestingEntity);

	// CIM init/deinit
	void Init(void);																		// Initialise as defaults

private:
	// Basic
	static CCentralInterestManager * m_pCIMInstance;		// The singleton
	bool m_bEnabled;																		// Toggle Interest system on/off
	bool m_bEntityEventListenerInstalled;

	typedef std::multimap<EntityId, IInterestListener* > TMapListeners;

	// The tracking lists
	TVecPIMs					m_PIMs;									// Instantiated PIMs, no NULL, some can be unassigned
	TVecInteresting		m_InterestingEntities;	// Interesting objects we might consider pointing out to PIMs
	TMapListeners			m_Listeners;						// Listeners to be notificated when an entity is interested OR interesting

	// Performance
	TVecPIMs::iterator m_itLastUpdated;
	float m_fUpdateTime;

	// Debug
	ICVar *m_cvDebugInterest;
	IPersistantDebug* m_pPersistantDebug;								// The persistant debugging framework
};





#endif __CentralInterestManager_H__







