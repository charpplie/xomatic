#pragma once
#include <Include/IPlugin.h>

class CExamplePlugin : public IPlugin
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