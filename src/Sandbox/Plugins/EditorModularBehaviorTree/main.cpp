// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"

#include <platform.h>
#include <platform_impl.h>

#include <IEditor.h>
#include <Include/IPlugin.h>
#include <Include/IEditorClassFactory.h>

#include "../EditorCommon/QtViewPane.h"

#include "MainWindow.h"

IEditor* g_pEditor;
IEditor* GetIEditor() {
	return g_pEditor; }

class Plugin : public IPlugin
{
	enum
	{
		Version = 1,
	};

public:
	bool Init( IEditor* pEditor )
	{
		RegisterQtViewPane<MainWindow>( pEditor, "Modular Behavior Tree Editor", "Game" );
		return true;
	}

	void Release() override
	{
		UnregisterQtViewPane<MainWindow>();
		delete this;
	}

	void ShowAbout() override {}
	const char* GetPluginGUID() override { return "{0AE29C33-36BB-4DB0-8596-DE662AFE0E98}"; }
	DWORD GetPluginVersion() override { return DWORD( Version ); }
	const char* GetPluginName() override { return "Modular Behavior Tree Editor"; }
	bool CanExitNow() override { return true; }
	void OnEditorNotify( EEditorNotifyEvent aEventId ) override {}
};

PLUGIN_API IPlugin* CreatePluginInstance( PLUGIN_INIT_PARAM* pInitParam )
{
	g_pEditor = pInitParam->pIEditorInterface;

	ISystem* pSystem = pInitParam->pIEditorInterface->GetSystem();
	ModuleInitISystem( pSystem, "Modular Behavior Tree Editor" );

	Plugin* pPlugin = new Plugin();
	if( pPlugin->Init( g_pEditor ) )
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

BOOL __stdcall DllMain( HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved )
{
	if( fdwReason == DLL_PROCESS_ATTACH )
	{
		g_hInstance = hinstDLL;
	}

	return TRUE;
}
