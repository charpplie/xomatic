//////////////////////////////////////////////////////////////////////
//
//	Crytek Network source code
//	
//	File: crynetwork.cpp
//  Description: dll entry point
//
//	History:
//	-July 25,2001:Created by Alberto Demichelis
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Network.h"
// Included only once per DLL module.
#include <platform_impl.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Network : public ISystemEventListener
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

static CSystemEventListner_Network g_system_event_listener_network;

/*
//////////////////////////////////////////////////////////////////////////
#if !defined(XBOX) && !defined(LINUX)
BOOL APIENTRY DllMain( HANDLE hModule, 
                       uint32  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{


    return TRUE;
}

#endif
*/

#ifndef _XBOX
CRYNETWORK_API INetwork *CreateNetwork(ISystem *pSystem, int ncpu)


#endif
{
	ModuleInitISystem(pSystem,"CryNetwork");

	CNetwork *pNetwork=new CNetwork;

	if(!pNetwork->Init(ncpu))
	{
		delete pNetwork;
		return NULL;
	}
	pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_network);
	return pNetwork;
}

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryNetwork : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryNetwork, "EngineModule_CryNetwork", 0x7dc5c3b8bb374063, 0xa29ac2d6dd718e0f)

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() { return "CryNetwork"; };
	virtual const char *GetCategory() { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize( SSystemGlobalEnvironment &env,const SSystemInitParams &initParams )
	{
		ISystem* pSystem = env.pSystem;

		CNetwork *pNetwork=new CNetwork;

		int ncpu = env.pi.numCoresAvailableToProcess;
		if(!pNetwork->Init(ncpu))
		{
			delete pNetwork;
			return NULL;
		}
		pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_network);
		env.pNetwork = pNetwork;
		
		return true;
	}
};

CRYREGISTER_CLASS(CEngineModule_CryNetwork)

CEngineModule_CryNetwork::CEngineModule_CryNetwork()
{

};

CEngineModule_CryNetwork::~CEngineModule_CryNetwork()
{
};


#include <CrtDebugStats.h>

