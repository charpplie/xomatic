////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2013.
// -------------------------------------------------------------------------
//  File name:   main.cpp
//  Version:     v1.00
//  Created:     13/12/2013 by Gabriel Rodriguez Hernandez.
//  Compilers:   Visual Studio 2010
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include <platform.h>
#include <platform_impl.h>
#include <IEditor.h>
#include <Include/IPlugin.h>
#include <Include/IEditorClassFactory.h>
#include "AudioControlsBrowserPlugin.h"

IEditor *g_pEditor;
IEditor* GetIEditor() { return g_pEditor; }

//------------------------------------------------------------------
PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	g_pEditor = pInitParam->pIEditorInterface;
	ISystem *pSystem = pInitParam->pIEditorInterface->GetSystem();
	ModuleInitISystem(pSystem, "AudioControlsBrowser");
	return new CAudioControlsBrowserPlugin(g_pEditor);
}

//------------------------------------------------------------------
HINSTANCE g_hInstance = 0;
BOOL __stdcall DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		g_hInstance = hinstDLL;
	}

	return TRUE;
}