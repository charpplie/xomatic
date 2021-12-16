#define FORCE_NOT_USE_CRY_MEMORY_MANAGER
#include "pch.h"
#include "DescEditorPlugin.h"
#include "Include/IEditorClassFactory.h"

IEditor *g_pEditor = NULL;

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
	ISystem *pSystem = pInitParam->pIEditorInterface->GetSystem();
	ModuleInitISystem(pSystem, "DescEditor");
	pSystem->GetILog()->Log("Desc Editorplugin: CreatePluginInstance");

	return new CDescEditorPlugin;
}