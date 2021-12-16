// Copyright (c) 1999-2014 Crytek.

#include "StdAfx.h"
#include <platform_impl.h>
#include <QtIntegration.h>
#include "FbxImportPlugin.h"

namespace
{
	struct SEnsureQtInitialized
	{
		SEnsureQtInitialized(IEditor *pEditor) { InitializeQt(pEditor); }
		~SEnsureQtInitialized() { FinalizeQt(); }
	};
}

PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	IEditor *pEditor = pInitParam->pIEditorInterface;
	ISystem *pSystem = pInitParam->pIEditorInterface->GetSystem();
	ModuleInitISystem(pSystem, "QtFbxImport");
	SEnsureQtInitialized qtInitialized(pEditor);
	return new CFbxImportPlugin(pEditor);
}

IEditor *GetIEditor()
{
	CFbxImportPlugin *pPlugin = CFbxImportPlugin::GetInstance();
	return pPlugin ? pPlugin->GetIEditor() : NULL;
}
