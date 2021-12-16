// CryMovie.cpp : Defines the entry point for the DLL application.
//

#include "StdAfx.h"
#include "CryMovie.h"
#include "Movie.h"
#include <CrtDebugStats.h>

// Included only once per DLL module.
#include <platform_impl.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

/*
#ifdef WIN32
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH: 
			break;
  }
  return TRUE;
}
#endif //WIN32
*/

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Movie : public ISystemEventListener
{
public:
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
	{
		switch (event)
		{
		case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
			{
				STLALLOCATOR_CLEANUP;
				break;
			}
		}
	}
};

static CSystemEventListner_Movie g_system_event_listener_movie;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryMovie : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryMovie, "EngineModule_CryMovie", 0xdce26beebdc6400f, 0xa0e9b42839f2dd5b)

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() { return "CryMovie"; };
	virtual const char *GetCategory() { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize( SSystemGlobalEnvironment &env,const SSystemInitParams &initParams )
	{
		ISystem* pSystem = env.pSystem;

		pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_movie);

		env.pMovieSystem = new CMovieSystem(pSystem);
		return true;
	}
};

CRYREGISTER_CLASS(CEngineModule_CryMovie)

CEngineModule_CryMovie::CEngineModule_CryMovie()
{

};

CEngineModule_CryMovie::~CEngineModule_CryMovie()
{
};