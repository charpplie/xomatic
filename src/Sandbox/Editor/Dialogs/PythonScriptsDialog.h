////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Dialogs/BaseFrameWnd.h"

class CFolderTreeCtrl;
class CPreviewerPage;

class CPythonScriptsDialog : public CXTResizeDialog
{
	DECLARE_DYNCREATE(CPythonScriptsDialog)

public:
	CPythonScriptsDialog();
	virtual ~CPythonScriptsDialog() {}

	static void RegisterViewClass();

	enum { IDD = IDD_PYTHON_SCRIPTS_DIALOG };

protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg void OnTreeDoubleClicked( NMHDR* pNMHDR, LRESULT* pResult );	
	afx_msg void OnExecute();

	CFolderTreeCtrl * m_pTree;
};