	// CryAISystem.cpp : Defines the entry point for the DLL application.
//

#include "StdAfx.h"
#include "CryAISystem.h"
#include "CAISystem.h"
#include "AILog.h"
#include <platform_impl.h>
#include <ISystem.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

CAISystem *g_pAISystem;

/*
//////////////////////////////////////////////////////////////////////////
// Pointer to Global ISystem.
static ISystem* gISystem = 0;
ISystem* GetISystem()
{
	return gISystem;
}
*/
//////////////////////////////////////////////////////////////////////////

#ifdef WIN32
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved )
{
	return TRUE;
}
#endif

int g_random_count = 0;


//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_AI : public ISystemEventListener
{
public:
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
	{
		switch (event)
		{
		case ESYSTEM_EVENT_RANDOM_SEED:
			g_random_generator.seed((uint32)wparam);
			g_random_count = 0;
			break;
		case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
			{
				STLALLOCATOR_CLEANUP;
				break;
			}
		}
	}
};
static CSystemEventListner_AI g_system_event_listener_ai;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryAISystem : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryAISystem, "EngineModule_CryAISystem", 0x6b8e79a784004f44, 0x97db7614428ad251)

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() { return "CryAISystem"; };
	virtual const char *GetCategory() { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize( SSystemGlobalEnvironment &env,const SSystemInitParams &initParams )
	{
		ISystem* pSystem = env.pSystem;

		pSystem->GetISystemEventDispatcher()->RegisterListener( &g_system_event_listener_ai );

#if (defined(WIN32) || defined(WIN64))
		HKEY  key;
		DWORD type;
		// Open the appropriate registry key
		LONG result = RegOpenKeyEx( HKEY_CURRENT_USER,"Software\\StalTech\\AIConfig",0, KEY_READ, &key );
		if( ERROR_SUCCESS == result )
		{
			DWORD dwVal;
			DWORD size=sizeof(dwVal);

			result = RegQueryValueEx( key, "DevMode", NULL,&type, (LPBYTE)&dwVal, &size );

			if(ERROR_SUCCESS==result && type==REG_DWORD)
				gAIEnv.configuration.IsDevMode = (dwVal!=0);
			else
				gAIEnv.configuration.IsDevMode = false;
		}
#else
		gAIEnv.configuration.IsDevMode = false;
#endif

		AIInitLog(pSystem);

		g_pAISystem = new CAISystem(pSystem);
		env.pAISystem = g_pAISystem;

		return true;
	}
};

CRYREGISTER_CLASS(CEngineModule_CryAISystem)

CEngineModule_CryAISystem::CEngineModule_CryAISystem()
{
};

CEngineModule_CryAISystem::~CEngineModule_CryAISystem()
{
};

//////////////////////////////////////////////////////////////////////////
#include <CrtDebugStats.h>

#ifndef _LIB
	#include "Common_TypeInfo.h"
	#include "Cry_Geo_info.h"
#endif

#include "TypeInfo_impl.h"
#include "AutoTypeStructs_info.h"