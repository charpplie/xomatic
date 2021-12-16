////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTree_RefBrowser.h
//  Version:     v1.00
//  Created:     17/01/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SelectionTree_RefBrowser_h__
#define __SelectionTree_RefBrowser_h__

#include "BSTEditor/SelectionTreeManager.h"

class CSelectionTree_RefBrowser : public CDialog
{
	DECLARE_DYNAMIC(CSelectionTree_RefBrowser)

public:
	CSelectionTree_RefBrowser(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSelectionTree_RefBrowser();

	void Init( ESelectionTreeTypeId type );
	const char* GetSelectedRef();

// Dialog Data
	enum { IDD = IDD_BST_REFBROWSER };

protected:
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL		OnInitDialog();

	DECLARE_MESSAGE_MAP()

protected:
	CTreeCtrl m_RefTree;
	ESelectionTreeTypeId m_refType;

	afx_msg void OnSelChanged( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnShowContents();

private:
	void LoadReferences();

	typedef std::map< HTREEITEM, string > TRefNames;
	TRefNames m_RefNames;
	string m_SelectedRef;

};

#endif // __soundbrowserdialog_h__