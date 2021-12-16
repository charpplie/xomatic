#include "stdafx.h"
#include "PerforcePlugin.h"
#include "PerforceSourceControl.h"
#include "Include/ISourceControl.h"


extern CPerforceSourceControl* g_pPerforceControl;

namespace PluginInfo
{
	const char* kName = "Perforce Client";
	const char* kGUID = "{FD5F1023-8F02-4051-89FA-DF1F038863A2}";
	const int kVersion = 1;
}

void CPerforcePlugin::Release()
{
	delete this;
}

void CPerforcePlugin::ShowAbout()
{
}

const char* CPerforcePlugin::GetPluginGUID()
{
	return PluginInfo::kGUID;
}

DWORD CPerforcePlugin::GetPluginVersion()
{
	return PluginInfo::kVersion;
}

const char* CPerforcePlugin::GetPluginName()
{
	return PluginInfo::kName;
}

bool CPerforcePlugin::CanExitNow()
{
	return true;
}

void CPerforcePlugin::Serialize(FILE* hFile, bool bIsStoring)
{
}

void CPerforcePlugin::ResetContent()
{
}

bool CPerforcePlugin::CreateUIElements()
{
	return true;
}

void CPerforcePlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
	if (eNotify_OnInit == aEventId)
	{
		g_pPerforceControl->Init();
	}
}