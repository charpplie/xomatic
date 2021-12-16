#include "stdafx.h"
#include "AssetTaggingPlugin.h"
#include "AssetTaggingImpl.h"
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
	ModuleInitISystem(pSystem,"AssetTaggingPlugin");
	pSystem->GetILog()->Log("AssetTaggingPlugin: CreatePluginInstance");
	g_pEditor->GetClassFactory()->RegisterClass(new CAssetTaggingImpl());

	return new CAssetTaggingPlugin;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		g_hInstance = hinstDLL;
	}

	return TRUE;
}