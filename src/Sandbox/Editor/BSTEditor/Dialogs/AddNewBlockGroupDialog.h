////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   AddNewBlockGroupDialog.h
//  Version:     v1.00
//  Created:     08/04/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef _AddNewBlockGroupDialog_H_
#define _AddNewBlockGroupDialog_H_

#if _MSC_VER > 1000
#pragma once
#endif


class CAddNewBlockGroupDialog: public CDialog
{
	DECLARE_DYNAMIC(CAddNewBlockGroupDialog)

public:
	CAddNewBlockGroupDialog(CWnd* pParent = NULL);
	~CAddNewBlockGroupDialog(){}

	// Dialog Data
	enum { IDD = IDD_BST_ADD_BLOCKGROUPDIALOG };

	void Init( const string& title, const string& name, bool vars=true, bool signals=true, bool tree=true, string varname="", string signame="", string treename="" )
	{
		m_title = title;
		m_Name = name;
		m_bVars = vars;
		m_bSignals = signals;
		m_bTree = tree;
		m_VarName = varname;
		m_SigName = signame;
		m_TreeName = treename;
	};

	string GetName() { return m_Name; }
	string GetVarsName() { return m_VarName; }
	string GetSignalsName() { return m_SigName; }
	string GetTreeName() { return m_TreeName; }
	bool HasVars() {return m_bVars; }
	bool HasSignals() {return m_bSignals; }
	bool HasTree() {return m_bTree; }
	bool IsAdvanced() { return m_sIsAdvanced; }

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();
	afx_msg void OnToggleAdvanced();

private:
	void SetAdvanced(bool IsAdvanced);

private:
	CEdit		m_NameCtrl;

	CButton	m_VarsCtrl;
	CButton	m_SignalsCtrl;
	CButton	m_TreeCtrl;

	CEdit		m_VarNameCtrl;
	CEdit		m_SigNameCtrl;
	CEdit		m_TreeNameCtrl;

	CButton	m_AdvancedCtrl;

	string m_title;
	string m_Name;
	string m_VarName;
	string m_SigName;
	string m_TreeName;

	bool m_bVars;
	bool m_bSignals;
	bool m_bTree;

	static bool m_sIsAdvanced;
};

#endif // _AddNewBlockGroupDialog_H_