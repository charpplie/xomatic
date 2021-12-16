#include "stdafx.h"
#include "ExamplePlugin.h"
#include "platform_impl.h"
#include "Include/ICommandManager.h"

namespace PluginInfo
{
	const char* kName = "Example plug-in";
	const char* kGUID = "{DFA4AFF7-2C70-4B29-B736-54393C4ABADF}";
	const int kVersion = 1;
}

void CExamplePlugin::Release()
{
	delete this;
}

void CExamplePlugin::ShowAbout()
{
}

const char* CExamplePlugin::GetPluginGUID()
{
	return PluginInfo::kPluginGUID;
}

DWORD CExamplePlugin::GetPluginVersion()
{
	return PluginInfo::kVersion;
}

const char* CExamplePlugin::GetPluginName()
{
	return PluginInfo::kName;
}

bool CExamplePlugin::CanExitNow()
{
	return true;
}

void CExamplePlugin::Serialize(FILE* hFile, bool bIsStoring)
{
}

void CExamplePlugin::ResetContent()
{
}

bool CExamplePlugin::CreateUIElements()
{
	return true;
}

void CExamplePlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
}