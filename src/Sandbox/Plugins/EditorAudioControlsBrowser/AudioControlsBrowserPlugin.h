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

#pragma once

#include <IEditor.h>
#include <Include/IPlugin.h>
#include "ATLControlsModel.h"
#include "common/IAudioSystemEditor.h"

//------------------------------------------------------------------
class CAudioControlsBrowserPlugin : public IPlugin
{
public:
	CAudioControlsBrowserPlugin(IEditor* editor);

	void Release() override;
	void ShowAbout() override {}
	const char* GetPluginGUID() override { return "{DFA4AFF7-2C70-4B29-B736-GRH00040314}"; }
	DWORD GetPluginVersion() override { return 1; }
	const char* GetPluginName() override { return "AudioControlsBrowser"; }
	bool CanExitNow() override { return true; }
	void OnEditorNotify(EEditorNotifyEvent aEventId) override {}

	static void SaveModels();
	static void ReloadModels();
	static void ReloadScopes();
	static AudioControls::CATLControlsModel* GetATLModel();
	static AudioControls::IAudioSystemEditor* GetAudioSystemEditorImpl();

	static AudioControls::CATLControlsModel ms_ATLModel;
	static std::set<string> ms_currentFilenames;
	
	static HMODULE ms_hMiddlewarePlugin;
	static AudioControls::IAudioSystemEditor* ms_pAudioSystemImpl;
};