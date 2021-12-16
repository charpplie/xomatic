#pragma once
//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : FFMPEGPlugin.h
//  Author           : Jaewon Jung
//  Time of creation : 8/10/2011   16:33
//  Compilers        : VS2008
//  Description      : 
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#include <Include/IPlugin.h>

#define COMMAND_MODULE "plugin"
#define COMMAND_NAME "ffmpeg_encode"

class CFFMPEGPlugin : public IPlugin
{
public:
	void Release();
	void ShowAbout();
	const char* GetPluginGUID();
	DWORD GetPluginVersion();
	const char* GetPluginName();
	bool CanExitNow();
	static void RegisterTheCommand();
	void OnEditorNotify(EEditorNotifyEvent aEventId);
};