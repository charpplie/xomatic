////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   AddNewTreeDialog.h
//  Version:     v1.00
//  Created:     21/01/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef _ADDNEWTREEDIALOG_H_
#define _ADDNEWTREEDIALOG_H_

#if _MSC_VER > 1000
#pragma once
#endif


class CAddNewTreeDialog: public CDialog
{
	DECLARE_DYNAMIC(CAddNewTreeDialog)

public:
	CAddNewTreeDialog(CWnd* pParent = NULL);
	~CAddNewTreeDialog(){}

	// Dialog Data
	enum { IDD = IDD_BST_ADD_NEW_TREE };

	void Init( const string& title, const string& name, const string& type = "" ) { m_title = title; m_name = name; m_type = type; };

	string GetName() { return m_name; }
	string GetType() { return m_type; }
protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();

private:
	void SetSelection( CComboBox& comboBox, const string& selection );
private:

	CComboBox				m_comboBoxType;
	CEdit						m_editName;
	string					m_name;
	string					m_type;
	string					m_title;
};

#endif // _ADDNEWTREEDIALOG_H_