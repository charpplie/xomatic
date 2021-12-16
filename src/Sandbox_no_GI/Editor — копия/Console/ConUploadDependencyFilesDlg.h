#pragma once

#include "Console/ConsoleHotUpdate.h"

// CConUploadDependencyFilesDlg dialog

class CConUploadDependencyFilesDlg : public CDialog, CConsoleHotUpdate::SHotUpdateJobObserver
{
	DECLARE_DYNAMIC(CConUploadDependencyFilesDlg)

public:
	CConUploadDependencyFilesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConUploadDependencyFilesDlg();

	bool StartUpload();
	void WriteToLog( const char* pText, COLORREF aTextColor = 0 );

	// implement CConsoleHotUpdate::SHotUpdateJobObserver
	void OnUploadStart();
	void OnUploadEnd( bool bAborted );
	void OnUploadFileProgress( CHotUpdateSystem::EConsolePlatform aPlatform, const char* pCurrentFilename, UINT aUploadedFileCount, UINT aTotalFileCount, EResultCode aResult, const char* pAdditionalResultMsg );
	void OnRcMessage( CHotUpdateSystem::EConsolePlatform aPlatform, IResourceCompilerListener::MessageSeverity aSeverity, const char* pMsg );
	
// Dialog Data
	enum { IDD = IDD_UPLOAD_ENTITY_DEPENDENCY_FILES };

protected:

	bool m_bCollapsed;
	int m_expandedDialogHeight, m_objectCount;
	CConsoleHotUpdate::TJobHandle m_hUploadJob;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl m_progressUpload;
	afx_msg void OnBnClickedButtonMore();
	CRichEditCtrl m_richedLog;
	afx_msg void OnBnClickedButtonClearLog();
	afx_msg void OnBnClickedCancel();
protected:
	virtual void OnOK();
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonAbortUpload();
	afx_msg void OnClose();
};
