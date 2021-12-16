#ifndef __GAMEOBJECTSYSTEM_H__
#define __GAMEOBJECTSYSTEM_H__

#pragma once

// FIXME: Cell SDK GCC bug workaround.
#ifndef __IGAMEOBJECTSYSTEM_H__
#include "IGameObjectSystem.h"
#endif

#include "GameObjectDispatch.h"
#include <vector>
#include <map>

struct SEntitySchedulingProfiles
{
	uint32 normal;
	uint32 owned;
};

class CGameObjectSystem : public IGameObjectSystem
{
public:
	bool Init();

	IGameObjectSystem::ExtensionID GetID( const char * name );
	const char * GetName( IGameObjectSystem::ExtensionID id );
	IGameObjectExtension * Instantiate( IGameObjectSystem::ExtensionID id, IGameObject * pObject );
	VIRTUAL void RegisterExtension( const char * name, IGameObjectExtensionCreatorBase * pCreator, IEntityClassRegistry::SEntityClassDesc * pClsDesc );
	VIRTUAL void DefineProtocol( bool server, IProtocolBuilder * pBuilder );
	VIRTUAL void BroadcastEvent( const SGameObjectEvent& evt );

	VIRTUAL void RegisterEvent( uint32 id, const char* name );
	VIRTUAL uint32 GetEventID( const char* name );
	VIRTUAL const char* GetEventName( uint32 id );

	VIRTUAL IGameObject *CreateGameObjectForEntity(EntityId entityId);

	VIRTUAL void PostUpdate( float frameTime );
	VIRTUAL void SetPostUpdate( IGameObject * pGameObject, bool enable );

	const SEntitySchedulingProfiles * GetEntitySchedulerProfiles( IEntity * pEnt );

	void RegisterFactories( IGameFramework * pFW );

	IEntity * CreatePlayerProximityTrigger();
	ILINE IEntityClass * GetPlayerProximityTriggerClass() { return m_pClassPlayerProximityTrigger; }

	void SetSpawnSerializer( TSerialize* ser ) { m_pSpawnSerializer = ser; }
	void GetMemoryStatistics(ICrySizer * s);

  VIRTUAL void AddSink( IGameObjectSystemSink *pSink );
  VIRTUAL void RemoveSink( IGameObjectSystemSink *pSink );
private:
	std::map<string, ExtensionID> m_nameToID;

	struct SExtensionInfo
	{
		string name;
		IGameObjectExtensionCreatorBase * pFactory;
	};
	std::vector<SExtensionInfo> m_extensionInfo;

	static IEntityProxy * CreateGameObjectWithPreactivatedExtension( 
		IEntity * pEntity, SEntitySpawnParams &params, void * pUserData );

	CGameObjectDispatch m_dispatch;

	std::vector<IGameObject*> m_postUpdateObjects;
	IEntityClass * m_pClassPlayerProximityTrigger;
	TSerialize * m_pSpawnSerializer;

	std::map<string, SEntitySchedulingProfiles> m_schedulingParams;
	SEntitySchedulingProfiles m_defaultProfiles;

	// event ID management
	std::map<string, uint32> m_eventNameToID;
	std::map<uint32, string> m_eventIDToName;
  //
  typedef std::list<IGameObjectSystemSink*> SinkList;
  SinkList												m_lstSinks;							// registered sinks get callbacks
};

#endif
