#include "StdAfx.h"
#include "LiveCreateSettingsPanel.h"
#include "LiveCreate/EditorLiveCreate.h"

#ifndef NO_LIVECREATE

BEGIN_MESSAGE_MAP(CLiveCreateSettingsPanel, CConfigPanel)
END_MESSAGE_MAP();

CLiveCreateSettingsPanel::CLiveCreateSettingsPanel(CWnd* pParent /*= NULL*/)
	: CConfigPanel(IDD, pParent)
{
	Create(IDD,pParent);
}

CLiveCreateSettingsPanel::~CLiveCreateSettingsPanel()
{
}

void CLiveCreateSettingsPanel::DoDataExchange(CDataExchange* pDX)
{
	CConfigPanel::DoDataExchange(pDX);
}

BOOL CLiveCreateSettingsPanel::OnInitDialog()
{
	CConfigPanel::OnInitDialog();

	// create configuration tabs
	DisplayGroup(&GetIEditor()->GetLiveCreate()->GetGeneralSettings(), "General options");
	DisplayGroup(&GetIEditor()->GetLiveCreate()->GetAdvancedSettings(), "Advanced options");

	return TRUE;
}

void CLiveCreateSettingsPanel::OnConfigValueChanged(Config::IConfigVar* pVar)
{
	if (pVar->GetName() == "bShowSelectionBoxes" ||
		pVar->GetName() == "bShowSelectionNames")
	{
		GetIEditor()->GetLiveCreate()->SyncSelection();
	}

	else if (pVar->GetName() == "bEnableLog")
	{
		// sync the config value with the internal manager value
		gEnv->pLiveCreateManager->SetLogEnabled(GetIEditor()->GetLiveCreate()->GetAdvancedSettings().bEnableLog);
	}

	// always flush the settings
	GetIEditor()->GetLiveCreate()->SaveSettings();	
}

#endif