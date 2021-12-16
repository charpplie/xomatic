#include "StdAfx.h"
#include "FBXPlugin.h"

namespace PluginInfo
{
	const char* kName = "FBX Exporter";
	const char* kGUID = "{6CD02F95-362C-4ADF-8BAE-87C6342A8027}";
	const int kVersion = 1;
}

void CFBXPlugin::Release()
{
	delete this;
}

void CFBXPlugin::ShowAbout()
{
}

const char* CFBXPlugin::GetPluginGUID()
{
	return PluginInfo::kGUID;
}

DWORD CFBXPlugin::GetPluginVersion()
{
	return PluginInfo::kVersion;
}

const char* CFBXPlugin::GetPluginName()
{
	return PluginInfo::kName;
}

bool CFBXPlugin::CanExitNow()
{
	return true;
}

void CFBXPlugin::Serialize(FILE* hFile, bool bIsStoring)
{
}

void CFBXPlugin::ResetContent()
{
}

bool CFBXPlugin::CreateUIElements()
{
	return true;
}

void CFBXPlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
}