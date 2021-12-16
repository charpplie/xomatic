#pragma once


#include "EngineSettingsManager.h"
#include "SettingsDialog.h"
#include "RCDialog.h"
#include "EditorDialog.h"
#include "SCDialog.h"
#include "LicenseDialog.h"


//////////////////////////////////////////////////////////////////////////
// Contains setting dialogs of all categories in a tab view
class CSettingsTab : public CTabCtrl
{
	DECLARE_DYNAMIC(CSettingsTab)

public:
	CSettingsTab();
	~CSettingsTab();

	void Create(CEngineSettingsManager* settingsManager);

	void ActivateTabDialog(int nSel);
	void DeactivateTabDialog(int nSel);

	// stores settings of all dialogs (where applicable) to settingsManager. 
	// returns false if root engine folder is invalid.
	bool PollSettings(CEngineSettingsManager* settingsManager) const;

	// saves settings of all dialogs (where applicable) to a config file.
	// returns false if root engine folder is invalid.
	bool SaveSettingsToSystemCfg() const;

protected:
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()

private:
	CEngineSettingsManager* m_settingsManager;

	CSettingsDialog* m_settingsDialog;
	CRCDialog* m_rcDialog;
	CEditorDialog* m_editorDialog;
	CSCDialog* m_shaderCompilerDialog;
	CLicenseDialog* m_licenseDialog;
	CDialog* m_tabDialogs[5];
};
