////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Crytek Character Animation source code
//	
//	History:
//	20/10/2004 - Created by Ivo Herzeg <ivo@crytek.de>
//
//  Contains:
//  provides access-interfaces to all DLLs in the system 
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <ICryAnimation.h>
#include <IFacialAnimation.h>
#include "CharacterManager.h"

#include "SkeletonAnim.h"

// Must be included once in the module.
#include <platform_impl.h>

#include <IEngineModule.h>

#include "AnimationThreadTask.h"

#undef GetClassName


//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Animation : public ISystemEventListener
{
public:
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
	{
		switch (event)
		{
		case ESYSTEM_EVENT_RANDOM_SEED:
			g_random_generator.seed((uint32)wparam);
			break;
		case ESYSTEM_EVENT_LEVEL_UNLOAD:
			break;
		case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
			{
				STLALLOCATOR_CLEANUP;
				break;
			}
		case ESYSTEM_EVENT_LEVEL_LOAD_END:
			g_pCharacterManager->UnlockResources();
		}
	}
};
static CSystemEventListner_Animation g_system_event_listener_anim;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryAnimation : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryAnimation, "EngineModule_CryAnimation", 0x9c73d2cd142c4256, 0xa8f0706d80cd7ad2)

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() { return "CryAnimation"; };
	virtual const char *GetCategory() { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize( SSystemGlobalEnvironment &env,const SSystemInitParams &initParams )
	{
		ISystem* pSystem = env.pSystem;

#ifndef PS3
		CAnimationThreadPool::Instance().Initialize();
#endif

		g_pISystem = pSystem;
		g_InitInterfaces();

		pSystem->GetISystemEventDispatcher()->RegisterListener( &g_system_event_listener_anim );

		g_pCharacterManager=new CharacterManager;
		g_pCharacterManager->m_IsDedicatedServer=g_pISystem->IsDedicated();
		env.pCharacterManager = g_pCharacterManager;

		return true;
	}
};

CRYREGISTER_CLASS(CEngineModule_CryAnimation)

CEngineModule_CryAnimation::CEngineModule_CryAnimation()
{

};

CEngineModule_CryAnimation::~CEngineModule_CryAnimation()
{
};


// cached interfaces - valid during the whole session, when the character manager is alive; then get erased
ISystem*						g_pISystem				= NULL;
ITimer*							g_pITimer					= NULL; //module implemented in CrySystem
ILog*								g_pILog						= NULL; //module implemented in CrySystem
IConsole*						g_pIConsole				= NULL;	//module implemented in CrySystem
ICryPak*						g_pIPak						= NULL;	//module implemented in CrySystem
IStreamEngine*			g_pIStreamEngine	= NULL; //module implemented in CrySystem
Crc32Gen*						g_pCrc32Gen				= NULL;	//module implemented in CrySystem

IRenderer*					g_pIRenderer			= NULL;
IRenderAuxGeom*			g_pAuxGeom				= NULL;
IPhysicalWorld*			g_pIPhysicalWorld	= NULL;
I3DEngine*					g_pI3DEngine			= NULL; //Need just for loading of chunks. Should be part of CrySystem


f32 g_AverageFrameTime=0;
CAnimation g_DefaultAnim;
CharacterManager* g_pCharacterManager; 
QuatT g_IdentityQuatT = QuatT(IDENTITY);


ILINE void g_LogToFile (const char* szFormat, ...)
{
	char szBuffer[0x800];
	va_list args;
	va_start(args,szFormat);
	_vsnprintf (szBuffer, sizeof(szBuffer), szFormat, args);
	va_end(args);
	g_pILog->LogToFile ("%s", szBuffer);
}









f32 g_fCurrTime=0;
int g_CpuFlags=0;
bool g_bProfilerOn = false;

AnimStatisticsInfo g_AnimStatisticsInfo;

// TypeInfo implementations for CryAnimation
#ifndef _LIB
	#include "Common_TypeInfo.h"
	#include "CGFContent_info.h"
#endif
