//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : ScriptHelpDialog.h
//  Author           : Jaewon Jung
//  Time of creation : 4/12/2011   15:33
//  Compilers        : VS2008
//  Description      : For listing available script commands with their descriptions
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#ifndef __SCRIPT_HELP_DIALOG_H__
#define __SCRIPT_HELP_DIALOG_H__
#pragma once

#include "Controls/FilterListCtrl.h"

class CScriptHelpDialog : public CXTResizeDialog
{
public:
	DECLARE_DYNAMIC(CScriptHelpDialog)
	
	static CScriptHelpDialog& GetInstance()
	{
		static CScriptHelpDialog *pInstance = NULL;
		if(pInstance == NULL)
		{
			pInstance = new CScriptHelpDialog();
			pInstance->Create(CScriptHelpDialog::IDD, AfxGetMainWnd());
		}
		return *pInstance;
	}

	enum { IDD = IDD_SCRIPT_HELP };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg void OnHeaderClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListDblClk(NMHDR* pNMHDR, LRESULT* pResult);

	virtual BOOL OnInitDialog();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	void FillCommandList(const CString& commandFilter, const CString& moduleFilter);
	bool SortColumn(int columnIndex, bool ascending);

	CFilterListCtrl m_cmdListCtrl;
	int m_iSortColumn;
	bool m_bAscending;
};

#endif // __SCRIPT_HELP_DIALOG_H__