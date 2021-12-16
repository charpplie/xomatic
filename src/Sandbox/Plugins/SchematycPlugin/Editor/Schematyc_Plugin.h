#pragma once

#include <Include/IPlugin.h>

class CSchematycPlugin : public IPlugin
{
public:

	CSchematycPlugin();

	// IPlugin
	void Release();
	void ShowAbout();
	const char* GetPluginGUID();
	DWORD GetPluginVersion();
	const char* GetPluginName();
	bool CanExitNow();
	void Serialize(FILE* hFile, bool bIsStoring);
	void ResetContent();
	bool CreateUIElements();
	void OnEditorNotify(EEditorNotifyEvent eventId);
	// ~IPlugin
};