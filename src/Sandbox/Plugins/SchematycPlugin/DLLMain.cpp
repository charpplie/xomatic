#include "StdAfx.h"

#include <platform_impl.h>

#include <IResourceSelectorHost.h>
#include <Include/IEditorClassFactory.h>
#include <Schematyc/Schematyc_IFramework.h>

#include "Editor/Schematyc_MainFrameWnd.h"
#include "Editor/Schematyc_Plugin.h"

IEditor*	g_pEditor		= NULL;
HINSTANCE	g_hInstance	= 0;

SCHEMATYC_PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	if(pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
	{
		pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
		return NULL;
	}
	
	g_pEditor = pInitParam->pIEditorInterface;
	ModuleInitISystem(pInitParam->pIEditorInterface->GetSystem(), "Schematyc_Plugin");
	
	if(GetSchematycFrameworkPtr() == NULL)
	{
		pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
		return NULL;
	}

	RegisterModuleResourceSelectors(GetIEditor()->GetResourceSelectorHost());
	Schematyc::CMainFrameWnd::RegisterViewClass();

	return new CSchematycPlugin;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
	if(fdwReason == DLL_PROCESS_ATTACH)
	{
		g_hInstance = hinstDLL;
	}
	return TRUE;
}

IEditor* GetIEditor()
{
	return g_pEditor;
}

HINSTANCE GetHInstance()
{
	return g_hInstance;
}