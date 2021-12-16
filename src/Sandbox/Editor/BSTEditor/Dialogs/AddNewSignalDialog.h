////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   AddNewSignalDialog.h
//  Version:     v1.00
//  Created:     21/01/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef _ADDNEWSIGNALDIALOG_H_
#define _ADDNEWSIGNALDIALOG_H_

#if _MSC_VER > 1000
#pragma once
#endif


class CAddNewSignalDialog: public CDialog
{
	DECLARE_DYNAMIC(CAddNewSignalDialog)

public:
	CAddNewSignalDialog(CWnd* pParent = NULL);
	~CAddNewSignalDialog(){}

	// Dialog Data
	enum { IDD = IDD_BST_ADD_NEW_SIGNAL };

	void Init( const string& signalName="", const string& variableName="", bool bVariableValue=true )
	{
		m_signalName = signalName;
		m_variableName = variableName;
		m_bVariableValue = bVariableValue;
	}

	string GetSignalName() { return m_signalName; }
	string GetVariableName() { return m_variableName; }
	bool GetVariableValue() {return m_bVariableValue; }
protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();

private:
	void SetSelection( CComboBox& comboBox, const string& selection );
	void GetVariables( std::list< string >& strList );
	void GetSignals( std::list< string >& strList );
	void LoadVarsFromXml( std::list< string >& strList, const XmlNodeRef& xmlNode );

private:

	CComboBox				m_comboBoxVariableName;
	CComboBox				m_comboBoxVariableValue;
	CComboBox				m_comboBoxSignals;
	string					m_variableName;
	bool					m_bVariableValue;
	string					m_signalName;
};

#endif // _ADDNEWSIGNALDIALOG_H_