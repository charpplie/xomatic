////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013.
// -------------------------------------------------------------------------
//  File name:   AddNewTimestampDialog.h
//  Version:     v1.00
//  Created:     03/09/2013 by Michiel Meesters
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef _ADDNEWTIMESTAMPDIALOG_H_
#define _ADDNEWTIMESTAMPDIALOG_H_

#if _MSC_VER > 1000
#pragma once
#endif


class CAddNewTimestampDialog: public CDialog
{
	DECLARE_DYNAMIC(CAddNewTimestampDialog)

public:
	CAddNewTimestampDialog(CWnd* pParent = NULL);
	~CAddNewTimestampDialog(){}

	// Dialog Data
	enum { IDD = IDD_BST_ADD_NEW_TIMESTAMP };

	void Init( const string& timestampName="", const string& eventName="", string exclusiveToTimestampName="" )
	{
		m_timestampName = timestampName;
		m_eventName = eventName;
		m_exclusiveToTimestampName = exclusiveToTimestampName;
	}

	string GetName() { return m_timestampName; }
	string GetEventName() { return m_eventName; }
	string GetExclusiveToTimestampName() {return m_exclusiveToTimestampName; }
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

	CComboBox				m_cbTimeStamp;
	CComboBox				m_cbEventName;
	CComboBox				m_cbExclusiveTo;
	string					m_timestampName;
	string					m_eventName;
	string					m_exclusiveToTimestampName;
};

#endif // _ADDNEWTIMESTAMPDIALOG_H_