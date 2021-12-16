////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   EditBehaviorDialog.h
//  Version:     v1.00
//  Created:     22/01/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __EditBehaviorDialog_H__
#define __EditBehaviorDialog_H__

#if _MSC_VER > 1000
#pragma once
#endif

class CSelectionTreeBlock;

class CEditBehaviorDialog: public CDialog
{
	DECLARE_DYNAMIC(CEditBehaviorDialog)

public:
	CEditBehaviorDialog(CWnd* pParent = NULL);
	~CEditBehaviorDialog(){}

	// Dialog Data
	enum { IDD = IDD_BST_CHANGE_BEHAVIOR };

	void Init( const string& title, const string& behaviorName ) { m_title = title; m_behaviorName = behaviorName; }
	string GetBehaviorName() { return m_behaviorName; }

protected:
	virtual BOOL OnInitDialog();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();

private:
	void LoadBehaviors();

private:
	CComboBox				m_comboBehaviorList;
	string					m_behaviorName;
	string					m_title;
};

#endif // _EditBehaviorDIALOG_H_