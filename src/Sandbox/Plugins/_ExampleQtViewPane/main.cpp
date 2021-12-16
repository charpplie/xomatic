#include "stdafx.h"

#define VC_EXTRALEAN
#include <afxwin.h>
#include <afxext.h>

#include <platform.h>
#include <platform_impl.h>

#include <IEditor.h>
#include <Include/IPlugin.h>
#include <Include/IEditorClassFactory.h>

#include "../EditorCommon/QtViewPane.h"
#include "ExampleWindow.h"


IEditor *g_pEditor;
IEditor* GetIEditor() { return g_pEditor; }


class CExamplePlugin : public IPlugin
{
public:
	CExamplePlugin(IEditor* editor)
	{
		RegisterQtViewPane<CExampleWindow>(editor, "_Example Qt ViewPane", "Test");
	}

	void Release() override
	{
		UnregisterQtViewPane<CExampleWindow>();
		delete this;
	}

	void ShowAbout() override {}
	const char* GetPluginGUID() override { return "{DFA4AFF7-2C70-4B29-B736-54391C4ABACF}"; }
	DWORD GetPluginVersion() override { return 1; }
	const char* GetPluginName() override { return "ExampleQtViewPane"; }
	bool CanExitNow() override { return true; }
	void OnEditorNotify(EEditorNotifyEvent aEventId) override {}
};


PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	g_pEditor = pInitParam->pIEditorInterface;
	ISystem *pSystem = pInitParam->pIEditorInterface->GetSystem();
	ModuleInitISystem(pSystem, "ExampleQtViewPane");
	return new CExamplePlugin(g_pEditor);
}

HINSTANCE g_hInstance = 0;
BOOL __stdcall DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		g_hInstance = hinstDLL;
	}

	return TRUE;
}
