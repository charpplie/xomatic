////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2011.
// -------------------------------------------------------------------------
//  File name:   FlowGraphModuleDlgs.h
//  Version:     v1.00
//
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __FLOWGRAPH_MODULE_DLG_H__
#define __FLOWGRAPH_MODULE_DLG_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <IFlowGraphModuleManager.h>
#include "resource.h"

class CHyperGraphDialog;
class CXTPTaskPanel;
class CHyperGraph;

//////////////////////////////////////////////////////////////////////////
// CFlowGraphEditModuleDlg - popup dialog allowing adding /
//	removing / editing module inputs + outputs
//////////////////////////////////////////////////////////////////////////

class CFlowGraphEditModuleDlg : public CDialog
{
	DECLARE_DYNAMIC(CFlowGraphEditModuleDlg)

public:
	CFlowGraphEditModuleDlg(IFlowGraphModule* pModule, CWnd* pParent = NULL);   // standard constructor
	virtual ~CFlowGraphEditModuleDlg() {};
	
protected:

	// Dialog Data
	enum { IDD = IDD_FG_EDIT_MODULE };

	CListBox m_inputListCtrl;
	CListBox m_outputListCtrl;

	void RefreshControl();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	
	afx_msg void OnCommand_NewInput();
	afx_msg void OnCommand_DeleteInput();
	afx_msg void OnCommand_EditInput();

	afx_msg void OnCommand_NewOutput();
	afx_msg void OnCommand_DeleteOutput();
	afx_msg void OnCommand_EditOutput();

	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

	IFlowGraphModule* m_pModule;

	typedef std::vector<IFlowGraphModule::SModulePortConfig> TPorts;
	TPorts m_inputs;
	TPorts m_outputs;
};


//////////////////////////////////////////////////////////////////////////
// CFlowGraphNewModuleInputDlg - popup dialog creating a new module input/output
//	or editing an existing one
//////////////////////////////////////////////////////////////////////////

class CFlowGraphNewModuleInputDlg : public CDialog
{
	DECLARE_DYNAMIC(CFlowGraphNewModuleInputDlg)

public:
	CFlowGraphNewModuleInputDlg(IFlowGraphModule::SModulePortConfig* pPort, CWnd* pParent = NULL);   // standard constructor
	virtual ~CFlowGraphNewModuleInputDlg() {}
	 
protected:

	// Dialog Data
	enum { IDD = IDD_FG_NEW_MODULE_INPUT };

	CComboBox m_inputTypesCtrl;
	CEdit m_inputNameCtrl;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

	void RefreshControl();

	IFlowGraphModule::SModulePortConfig* m_pPort;
};


#endif
