#ifndef AFX_SETTINGSMANAGERDIALOG_H
#define AFX_SETTINGSMANAGERDIALOG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Dialogs\BaseFrameWnd.h"

#define SETTINGS_MANAGER_WINDOW_NAME "Editor Settings Manager"

class CSettingsManager;

//////////////////////////////////////////////////////////////////////////
// Settings Manager Dialog
class CSettingsManagerDialog : public CDialog
{
public:
	CSettingsManagerDialog(CWnd* pParent = NULL);
	~CSettingsManagerDialog();
	enum { IDD = IDD_SETTINGS_MANAGER_DIALOG };
	afx_msg void OnReadBtnClick();
	afx_msg void OnExportBtnClick();
	afx_msg void ImportSettings(CString file);
	afx_msg void ImportLayouts(CString file);
	afx_msg void OnImportBtnClick();
	afx_msg void OnCloseAllTools();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();

	CListBox m_layoutListBox;
	CButton m_importSettingsChk;
	CButton m_importThemeChk;
	CButton m_skipToolsChk;
	CString m_importFileStr;

	DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////////////
class CSettingsManagerFrame : public CBaseFrameWnd
{
	DECLARE_DYNCREATE(CSettingsManagerFrame)

public:
	CSettingsManagerFrame();

	enum { IDD = IDD_SETTINGS_MANAGER_VIEW };

	afx_msg CSettingsManagerDialog* GetDialog();

protected:
	virtual BOOL OnInitDialog();
	CSettingsManagerDialog dlg;
};

#endif // #if !defined(AFX_SETTINGSMANAGERDIALOG_H)