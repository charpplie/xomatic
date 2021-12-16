#include "stdafx.h"
#include "AssetTaggingPlugin.h"
#include "platform_impl.h"
#include "Include/ICommandManager.h"

namespace PluginInfo
{
	const char* kName = "AssetTagging";
	const char* kGUID = "{DBB805D2-B302-4B84-8986-82C4A8061C4D}";
	const int kVersion = 1;
}

void CAssetTaggingPlugin::Release()
{
	delete this;
}

void CAssetTaggingPlugin::ShowAbout()
{
}

const char* CAssetTaggingPlugin::GetPluginGUID()
{
	return PluginInfo::kGUID;
}

DWORD CAssetTaggingPlugin::GetPluginVersion()
{
	return PluginInfo::kVersion;
}

const char* CAssetTaggingPlugin::GetPluginName()
{
	return PluginInfo::kName;
}

bool CAssetTaggingPlugin::CanExitNow()
{
	return true;
}

void CAssetTaggingPlugin::Serialize(FILE* hFile, bool bIsStoring)
{
}

void CAssetTaggingPlugin::ResetContent()
{
}

bool CAssetTaggingPlugin::CreateUIElements()
{
	return true;
}

void CAssetTaggingPlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
}