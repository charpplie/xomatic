#pragma once


#include "Resource.h"

// Shader Cache dialog
class CSCDialog : public CDialog
{
	DECLARE_DYNAMIC(CSCDialog)

public:
	CSCDialog(CWnd* pParent = NULL);
	enum { IDD = IDD_SC_DIALOG };

	void Create(UINT temp, CWnd* parent, CEngineSettingsManager* settingsManager);

	void ReloadDialog(const string& sRootPath);

	void SaveSettingsToSystemCfg(const string& sRootPath) const;

protected:
	virtual BOOL OnInitDialog();

	virtual void DoDataExchange(CDataExchange* pDX);
	DECLARE_MESSAGE_MAP()

private:
	bool LoadSettingsFromSystemCfg(const string& sRootPath);

	CEngineSettingsManager* m_settingsManager;

	CButton m_buttonShadersRemoteCompile;
	CEdit m_editShaderCompileServer;
	CStatic m_labelServer;
	CStatic m_labelInfo;
};
