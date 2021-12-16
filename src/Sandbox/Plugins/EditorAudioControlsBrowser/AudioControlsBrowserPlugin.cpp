////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   main.cpp
//  Version:     v1.00
//  Created:     13/12/2013 by Gabriel Rodriguez Hernandez.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AudioControlsBrowserPlugin.h"
#include "QtViewPane.h"
#include "AudioControlsBrowserWindow.h"
#include "IResourceSelectorHost.h"
#include "AudioControlsLoader.h"
#include "AudioControlsWriter.h"
#include "AudioControl.h"
#include "common/IAudioSystemEditor.h"

#include <CryFile.h>
#include <CryPath.h>

using namespace AudioControls;
using namespace PathUtil;

CATLControlsModel CAudioControlsBrowserPlugin::ms_ATLModel;
IAudioSystemEditor* CAudioControlsBrowserPlugin::ms_pAudioSystemImpl;
std::set<string> CAudioControlsBrowserPlugin::ms_currentFilenames;
HMODULE CAudioControlsBrowserPlugin::ms_hMiddlewarePlugin;

#ifdef WIN64
const string g_sMiddlewareDllPath = "Bin64\\EditorPlugins\\";
#else
const string g_sMiddlewareDllPath = "Bin32\\EditorPlugins\\";
#endif

const string g_sImplementationCVarName = "s_AudioSystemImplementationName";

typedef void (WINAPI* PGNSI)();
typedef IAudioSystemEditor* (*TPfnGetAudioInterface)(IEditor*);

CAudioControlsBrowserPlugin::CAudioControlsBrowserPlugin(IEditor* editor)
{
	RegisterQtViewPane<CAudioControlsBrowserWindow>(editor, "Audio Controls Browser", "Tools");
	RegisterModuleResourceSelectors(GetIEditor()->GetResourceSelectorHost());

	ICVar* pCVar = gEnv->pConsole->GetCVar(g_sImplementationCVarName);
	if (pCVar)
	{
		const string sDLLName = g_sMiddlewareDllPath + pCVar->GetString() + "Editor.dll";

		ms_hMiddlewarePlugin = LoadLibraryA(sDLLName);
		if (!ms_hMiddlewarePlugin)
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "(ACB) Couldn't load the middleware specific editor dll.");
			return;
		}

		TPfnGetAudioInterface pfnAudioInterface = (TPfnGetAudioInterface) GetProcAddress(ms_hMiddlewarePlugin, "GetAudioInterface");
		if (!pfnAudioInterface)
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "(ACB) Couldn't get middleware interface from loaded dll.");
			FreeLibrary(ms_hMiddlewarePlugin);
			return;
		}

		ms_pAudioSystemImpl = pfnAudioInterface(GetIEditor());
		if (!ms_pAudioSystemImpl)
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "(ACB) Data from middleware is empty.");
		}

		ReloadModels();
	}
	else
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "(ACB) CVar %s not defined. Needed to derive the Editor plugin name.", g_sImplementationCVarName);
	}
}

void CAudioControlsBrowserPlugin::Release()
{
	UnregisterQtViewPane<CAudioControlsBrowserWindow>();
	if (ms_hMiddlewarePlugin)
	{
		FreeLibrary(ms_hMiddlewarePlugin);
	}
	delete this;
}

void CAudioControlsBrowserPlugin::SaveModels()
{
	if (ms_pAudioSystemImpl)
	{
		CAudioControlsWriter writer(ms_ATLModel, ms_pAudioSystemImpl, ms_currentFilenames);
	}
}

void CAudioControlsBrowserPlugin::ReloadModels()
{
	GetIEditor()->SuspendUndo();
	ms_ATLModel.SetSuppressMessages(true);

	if (ms_pAudioSystemImpl)
	{
		ms_ATLModel.Clear();
		CAudioControlsLoader ATLLoader(&ms_ATLModel, ms_pAudioSystemImpl);
		ATLLoader.LoadAll();
		ms_currentFilenames = ATLLoader.GetLoadedFilenamesList();
	}

	ms_ATLModel.SetSuppressMessages(false);
	GetIEditor()->ResumeUndo();
}

void CAudioControlsBrowserPlugin::ReloadScopes()
{
	if (ms_pAudioSystemImpl)
	{
		ms_ATLModel.ClearScopes();
		CAudioControlsLoader ATLLoader(&ms_ATLModel, ms_pAudioSystemImpl);
		ATLLoader.LoadScopes();
	}
}

CATLControlsModel* CAudioControlsBrowserPlugin::GetATLModel()
{
	return &ms_ATLModel;
}

AudioControls::IAudioSystemEditor* CAudioControlsBrowserPlugin::GetAudioSystemEditorImpl()
{
	return ms_pAudioSystemImpl;
}
