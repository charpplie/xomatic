////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   main.cpp
//  Version:     v1.00
//  Created:     21 Sen 2004 by Sergiy Shaykin.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "PerforcePlugin.h"

#include "Include/ISourceControl.h"
#include "Include/IEditorClassFactory.h"
#include "PerforceSourceControl.h"


IEditor* g_pEditor = 0;
HINSTANCE g_hInstance = 0;
CPerforceSourceControl* g_pPerforceControl = 0;


IEditor* GetIEditor()
{
	return g_pEditor;
}


PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
	{
		pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
		return 0;
	}

	g_pEditor = pInitParam->pIEditorInterface;
	ModuleInitISystem(GetIEditor()->GetSystem(),"PerforcePlugin");
	g_pPerforceControl = new CPerforceSourceControl();
	pInitParam->pIEditorInterface->GetClassFactory()->RegisterClass(g_pPerforceControl);
	return new CPerforcePlugin();
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
{
   if( fdwReason == DLL_PROCESS_ATTACH )
   {
      g_hInstance = hinstDLL;
      //DisableThreadLibraryCalls(hInstance);
   }

   return(TRUE);
}