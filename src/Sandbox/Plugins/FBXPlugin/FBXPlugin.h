#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2012
// -------------------------------------------------------------------------
//  Created:     24 June 2011 by Sergiy Shaykin.
//  Description: Attachment to the Sandbox plug-in system
//
////////////////////////////////////////////////////////////////////////////
#include <Include/IPlugin.h>

class CFBXPlugin : public IPlugin
{
public:
	void Release();
	void ShowAbout();
	const char* GetPluginGUID();
	DWORD GetPluginVersion();
	const char* GetPluginName();
	bool CanExitNow();
	void Serialize(FILE *hFile, bool bIsStoring);
	void ResetContent();
	bool CreateUIElements();
	void OnEditorNotify(EEditorNotifyEvent aEventId);
};