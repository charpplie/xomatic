// CryInput.cpp : Defines the entry point for the DLL application.
//

#include "StdAfx.h"

// Included only once per DLL module.
#include <platform_impl.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include "BaseInput.h"

#if defined(WIN32) || defined(XENON)
#ifndef _LIB
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	return TRUE;
}
#endif
#endif // WIN32


//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryInput : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryInput, "EngineModule_CryInput", 0x3cc0516071bb44f6, 0xae525949f30277f9)

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() { return "CryInput"; };
	virtual const char *GetCategory() { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize( SSystemGlobalEnvironment &env,const SSystemInitParams &initParams )
	{
		ISystem* pSystem = env.pSystem;

		IInput *pInput = 0;
		if (!pSystem->IsDedicated())
		{
#if defined(USE_DXINPUT)
			pInput = new CDXInput(pSystem, (HWND) initParams.hWnd);




#elif defined(USE_LINUXINPUT)
			pInput = new CLinuxInput(pSystem);


#endif
		}
		else
			pInput = new CBaseInput();

		if (!pInput->Init())
		{
			delete pInput;
			return false;
		}
		env.pInput = pInput;
		return true;
	}
};

CRYREGISTER_CLASS(CEngineModule_CryInput)

CEngineModule_CryInput::CEngineModule_CryInput()
{
};

CEngineModule_CryInput::~CEngineModule_CryInput()
{
};

#include <CrtDebugStats.h>

