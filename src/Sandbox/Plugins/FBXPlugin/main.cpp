////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2011.
// -------------------------------------------------------------------------
//  File name:   main.cpp
//  Version:     v1.00
//  Created:     24 June 2011 by Sergiy Shaykin.
//  Compilers:   Visual Studio 2008
//  Description: Export geometry to FBX file format
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "platform_impl.h"

#include "FBXPlugin.h"
#include "FBXExporter.h"

extern HINSTANCE g_hInstance = NULL;
IEditor* g_pEditor = nullptr;

SANDBOX_API IEditor* GetIEditor()
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
	//ModuleInitISystem(g_pSystem, "FBXPlugin");

	IExportManager* pExportManager = pInitParam->pIEditorInterface->GetExportManager();
	if (pExportManager)
	{
		pExportManager->RegisterExporter(new CFBXExporter());
	}

	GetIEditor()->GetSystem()->GetILog()->Log("FBX plugin: CreatePluginInstance");
	return new CFBXPlugin;
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
