// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include <platform.h>
#include <platform_impl.h>
#include <IEditor.h>
#include "AudioSystemEditor_wwise.h"

IEditor *g_pEditor;
IEditor* GetIEditor() { return g_pEditor; }

using namespace AudioControls;

CAudioSystemEditor_wwise* g_pWwiseInterface;

//------------------------------------------------------------------
extern "C" PLUGIN_API IAudioSystemEditor* GetAudioInterface(IEditor* pEditor)
{
	g_pEditor = pEditor;
	ModuleInitISystem(pEditor->GetSystem(), "EditorWwise");
	g_pWwiseInterface = new CAudioSystemEditor_wwise();
	return g_pWwiseInterface;
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