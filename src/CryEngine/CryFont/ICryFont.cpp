//////////////////////////////////////////////////////////////////////
//
//  CryFont Source Code
//
//  File: ICryFont.cpp
//  Description: Create the font interface.
//
//  History:
//  - August 17, 2001: Created by Alberto Demichelis
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
// Included only once per DLL module.
#include <platform_impl.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include "CryFont.h"
#if defined(USE_NULLFONT)
#include "NullFont.h"
#endif

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Font : public ISystemEventListener
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
static CSystemEventListner_Font g_system_event_listener_font;

///////////////////////////////////////////////
extern "C" ICryFont* CreateCryFontInterface(ISystem *pSystem)
{
	ModuleInitISystem(pSystem,"CryFont");

	if (pSystem->IsDedicated())
	{
#if defined(USE_NULLFONT)
		return new CCryNullFont();
#else
		// The NULL font implementation must be present for all platforms
		// supporting running as a pure dedicated server.
		pSystem->GetILog()->LogError("Missing NULL font implementation for dedicated server");
		return NULL;
#endif
	}
	else
	{
		pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_font);
		return new CCryFont(pSystem);
	}
}

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryFont : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryFont, "EngineModule_CryFont", 0x6758643f43214957, 0x9b920d898d31f434)

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() { return "CryFont"; };
	virtual const char *GetCategory() { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize( SSystemGlobalEnvironment &env,const SSystemInitParams &initParams )
	{
		ISystem* pSystem = env.pSystem;
		env.pCryFont = CreateCryFontInterface(pSystem);
		return env.pCryFont != 0;
	}
};

CRYREGISTER_CLASS(CEngineModule_CryFont)

CEngineModule_CryFont::CEngineModule_CryFont()
{
};

CEngineModule_CryFont::~CEngineModule_CryFont()
{
};
