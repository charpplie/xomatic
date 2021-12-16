// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include <platform.h>
#include <platform_impl.h>
#include <IEditor.h>
#include "AudioSystemEditor_sdlmixer.h"

IEditor *g_pEditor;
IEditor* GetIEditor() { return g_pEditor; }

using namespace AudioControls;

CAudioSystemEditor_sdlmixer* g_pSDLMixerInterface;

//------------------------------------------------------------------
extern "C" PLUGIN_API IAudioSystemEditor* GetAudioInterface(IEditor* pEditor)
{
	g_pEditor = pEditor;
	ModuleInitISystem(pEditor->GetSystem(), "EditorSDLMixer");
	g_pSDLMixerInterface = new CAudioSystemEditor_sdlmixer();
	return g_pSDLMixerInterface;
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