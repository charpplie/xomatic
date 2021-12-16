#define FORCE_NOT_USE_CRY_MEMORY_MANAGER
#include "pch.h"
#include "DescEditorPlugin.h"
#include "platform_impl.h"
#include "Include/ICommandManager.h"
#include "DescEditor.h"
#include "Properties/ClassProfile.h"

namespace PluginInfo
{
	const char* kName = "Desc Editor plug-in";
	const char* kGUID = "{4B9B7074-2D58-4AFD-BBE1-BE469D48456A}";
	const int kVersion = 1;
}

CDescEditorPlugin::CDescEditorPlugin()
{
	CryGame::CClassProfileManager::GetInstance()->Init();
	CryGame::CDescEditor::RegisterViewClass();
}

CDescEditorPlugin::~CDescEditorPlugin()
{
	CryGame::CClassProfileManager::GetInstance()->DeInit();
}

void CDescEditorPlugin::Release()
{
	delete this;
}

void CDescEditorPlugin::ShowAbout()
{
}

const char* CDescEditorPlugin::GetPluginGUID()
{
	return PluginInfo::kGUID;
}

DWORD CDescEditorPlugin::GetPluginVersion()
{
	return PluginInfo::kVersion;
}

const char* CDescEditorPlugin::GetPluginName()
{
	return PluginInfo::kName;
}

bool CDescEditorPlugin::CanExitNow()
{
	return true;
}

void CDescEditorPlugin::Serialize(FILE* hFile, bool bIsStoring)
{
}

void CDescEditorPlugin::ResetContent()
{
}

bool CDescEditorPlugin::CreateUIElements()
{
	return true;
}

void CDescEditorPlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
}