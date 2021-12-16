#include "StdAfx.h"

#include <platform.h>
#include <platform_impl.h>

#include <IEditor.h>
#include <Include/IPlugin.h>
#include <Include/IEditorClassFactory.h>

#include "../EditorCommon/QtViewPane.h"
#include "MainEditorWindow.h"

#include "GameBind.h"

#define REGISTER_EDITOR_PANELS 1

IEditor *g_pEditor;
IEditor* GetIEditor() { return g_pEditor; }

class GameDatabasePlugin : public IPlugin
{
	enum 
	{
		Version = 1,
	};

public:

	bool Init(IEditor* pEditor)
	{
		if(!m_gameBind.Init( pEditor ))
			return false;

#if REGISTER_EDITOR_PANELS
		RegisterQtViewPane<MainEditorWindow>(pEditor, "Hunt Descriptor Database", "Game");
#endif
	
		return true;
	}

	void Release() override
	{
#if REGISTER_EDITOR_PANELS
		UnregisterQtViewPane<MainEditorWindow>();
#endif
		delete this;
	}

	void ShowAbout() override {}
	const char* GetPluginGUID() override { return "{083556EE-9120-4AEF-89A2-96271B1C4242}"; }
	DWORD GetPluginVersion() override { return DWORD(Version); }
	const char* GetPluginName() override { return "Game Database"; }
	bool CanExitNow() override { return true; }
	void OnEditorNotify(EEditorNotifyEvent aEventId) override {}

private:
	GameBind m_gameBind;

};


PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	g_pEditor = pInitParam->pIEditorInterface;
	ISystem *pSystem = pInitParam->pIEditorInterface->GetSystem();
	ModuleInitISystem(pSystem, "GameDatabase");
	
	GameDatabasePlugin* pPlugin = new GameDatabasePlugin();
	if(pPlugin->Init(g_pEditor))
	{
		return pPlugin;
	}
	else
	{
		pPlugin->Release();
		return NULL;
	}
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
