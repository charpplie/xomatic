#include "StdAfx.h"

#include "Schematyc_Plugin.h"

//#include <platform_impl.h>

#include "Include/ICommandManager.h"

#include <Schematyc/Schematyc_GUID.h>
#include <Schematyc/Schematyc_ICompiler.h>
#include <Schematyc/Schematyc_IDocManager.h>

namespace
{
	const char*	PLUGIN_GUID			= "{91A8A207-F8F0-4D5B-B8CA-613B4920581F}";
	const int		PLUGIN_VERSION	= 1;
	const char*	PLUGIN_NAME			= "Schematyc Plug-in";

	Schematyc::SGUID GenerateGUID()
	{
		Schematyc::SGUID	guid;
#if defined(WIN32) || defined(WIN64)
		::CoCreateGuid(&guid.sysGUID);
#endif
		return guid;
	}
}

CSchematycPlugin::CSchematycPlugin()
{
	// Hook up GUID generator then patch up documents and fix broken/deprecated dependencies.
	GetSchematycFramework().SetGUIDGenerator(MAKE_DELEGATE(GenerateGUID));
	GetSchematycFramework().GetDocManager().RefreshDocs(Schematyc::DocRefreshReason::PATCH_UP);
}

void CSchematycPlugin::Release()
{
	delete this;
}

void CSchematycPlugin::ShowAbout() {}

const char* CSchematycPlugin::GetPluginGUID()
{
	return PLUGIN_GUID;
}

DWORD CSchematycPlugin::GetPluginVersion()
{
	return PLUGIN_VERSION;
}

const char* CSchematycPlugin::GetPluginName()
{
	return PLUGIN_NAME;
}

bool CSchematycPlugin::CanExitNow()
{
	return true;
}

void CSchematycPlugin::Serialize(FILE* hFile, bool bIsStoring) {}

void CSchematycPlugin::ResetContent() {}

bool CSchematycPlugin::CreateUIElements()
{
	return true;
}

void CSchematycPlugin::OnEditorNotify(EEditorNotifyEvent eventId) {}