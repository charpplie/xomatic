//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : main.cpp
//  Author           : Jaewon Jung
//  Time of creation : 8/5/2011   16:28
//  Compilers        : VS2008
//  Description      : 
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FFMPEGPlugin.h"
#include "Include/IEditorClassFactory.h"

IEditor *g_pEditor = NULL;

HINSTANCE g_hInstance = 0;

PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
	{
		pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
		return 0;
	}

	g_pEditor = pInitParam->pIEditorInterface;
	ISystem *pSystem = pInitParam->pIEditorInterface->GetSystem();
	ModuleInitISystem(pSystem,"FFMPEGPlugin");
	pSystem->GetILog()->Log("FFMPEG plugin: CreatePluginInstance");
	CFFMPEGPlugin::RegisterTheCommand();
	return new CFFMPEGPlugin;
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