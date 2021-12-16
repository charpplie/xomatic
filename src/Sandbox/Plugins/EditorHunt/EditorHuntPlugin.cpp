#include "pch.h"
#include "EditorHuntPlugin.h"
#include "platform_impl.h"
#include "Include/ICommandManager.h"
#include "ActorEntityObject.h"
#include "OffMeshLinkObject.h"

namespace PluginInfo
{
	const char* kName = "Hunt Editor plug-in";
	const char* kGUID = "{71CED8AB-54E2-4739-AA78-7590A5DC5AEB}";
	const int kVersion = 1;
}

CHuntEditorPlugin::CHuntEditorPlugin()
{
	GetIEditor()->GetClassFactory()->RegisterClass(new CryGame::CActorEntityObjectClassDesc);
	GetIEditor()->GetClassFactory()->RegisterClass(new CryGame::COffMeshLinkObjectClassDesc);
}

void CHuntEditorPlugin::Release()
{
	delete this;
}

void CHuntEditorPlugin::ShowAbout()
{
}

const char* CHuntEditorPlugin::GetPluginGUID()
{
	return PluginInfo::kGUID;
}

DWORD CHuntEditorPlugin::GetPluginVersion()
{
	return PluginInfo::kVersion;
}

const char* CHuntEditorPlugin::GetPluginName()
{
	return PluginInfo::kName;
}

bool CHuntEditorPlugin::CanExitNow()
{
	return true;
}

void CHuntEditorPlugin::Serialize(FILE* hFile, bool bIsStoring)
{
}

void CHuntEditorPlugin::ResetContent()
{
}

bool CHuntEditorPlugin::CreateUIElements()
{
	return true;
}

void CHuntEditorPlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
}