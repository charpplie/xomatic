//////////////////////////////////////////////////////////////////////
//
//  CryScaleformGFx Source Code
//
//  File: ICryScaleformGFx.cpp
//  Description: Create the Scaleform GFx interface.
//
//  History:
//  - July 3, 2010: Created by Paul Reindell
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"

#include <platform_impl.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>


#include "ScaleformGFx.h"
#include "Scaleform/FlashPlayerInstance.h"

extern "C" IScaleformGFx* CreateCryScaleformGFxInterface(ISystem *pSystem)
{
	ModuleInitISystem(pSystem,"CryScaleformGFx");

#ifndef EXCLUDE_SCALEFORM_SDK
	CFlashPlayer::InitCVars();
	gEnv->pLog->Log("Using Scaleform GFx "GFC_FX_VERSION_STRING);
#else
	CryLogAlways("No Scaleform GFx support");
#endif

	return new CScaleformGFx();
}

class CEngineModule_CryScaleformGFx : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryScaleformGFx, "EngineModule_CryScaleformGFx", 0x28E7D9FDABF442AA, 0xACB7513ADAE73A74)

		//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() { return "CryScaleformGFx"; };
	virtual const char *GetCategory() { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize( SSystemGlobalEnvironment &env,const SSystemInitParams &initParams )
	{
		env.pScaleformGFx = CreateCryScaleformGFxInterface(env.pSystem);
		return true;
	}
};

CEngineModule_CryScaleformGFx::CEngineModule_CryScaleformGFx()
{
};

CEngineModule_CryScaleformGFx::~CEngineModule_CryScaleformGFx()
{
};

CRYREGISTER_CLASS(CEngineModule_CryScaleformGFx)