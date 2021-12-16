// CryEntityDLL.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include <IEntitySystem.h>
#include "EntitySystem.h"
// Included only once per DLL module.
#include <platform_impl.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

struct CSystemEventListner_Entity : public ISystemEventListener
{
public:
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
	{
		switch (event)
		{
		case ESYSTEM_EVENT_RANDOM_SEED:
			g_random_generator.seed((uint32)wparam);
			break;

		case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
			{
				STLALLOCATOR_CLEANUP;
				break;
			}
		}
	}
};
static CSystemEventListner_Entity g_system_event_listener_entity;

CEntitySystem *g_pIEntitySystem = NULL;


//////////////////////////////////////////////////////////////////////////
class CEngineModule_EntitySystem : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_EntitySystem, "EngineModule_CryEntitySystem", 0x885655072f014c03, 0x820c5a1a9b4d623b)

		//////////////////////////////////////////////////////////////////////////
		virtual const char *GetName() { return "CryEntitySystem"; };
		virtual const char *GetCategory() { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize( SSystemGlobalEnvironment &env,const SSystemInitParams &initParams )
	{
		ISystem* pSystem = env.pSystem;

		CEntitySystem *pEntitySystem= new CEntitySystem(pSystem);
		g_pIEntitySystem = pEntitySystem;
		if(!pEntitySystem->Init(pSystem))
		{
			pEntitySystem->Release();
			return false;
		}
		pSystem->GetISystemEventDispatcher()->RegisterListener( &g_system_event_listener_entity );

		env.pEntitySystem = pEntitySystem;
		return true;
	}
};

CRYREGISTER_CLASS(CEngineModule_EntitySystem)

CEngineModule_EntitySystem::CEngineModule_EntitySystem()
{
};

CEngineModule_EntitySystem::~CEngineModule_EntitySystem()
{
};


#include <CrtDebugStats.h>
