////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2011.
// -------------------------------------------------------------------------
//  File name:   FlowGraphTokens.h
//  Version:     v1.00
//  Created:     1/09/201 by SNH
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __FLOWGRAPH_TOKENS_H__
#define __FLOWGRAPH_TOKENS_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Controls/ObjectListCtrl.h"
#include "Controls/PropertyCtrl.h"
#include "FlowGraph.h"
#include "resource.h"

class CHyperGraphDialog;
class CXTPTaskPanel;
class CHyperGraph;

//////////////////////////////////////////////////////////////////////////
//	CFlowGraphTokensCtrl - read-only display of graph tokens
//		in the right-hand properties panel
//////////////////////////////////////////////////////////////////////////

class CFlowGraphTokensCtrl : public CWnd
{
public:
	DECLARE_DYNAMIC(CFlowGraphTokensCtrl)

	CFlowGraphTokensCtrl(CHyperGraphDialog* pParent, CXTPTaskPanel* pPanel);
	virtual ~CFlowGraphTokensCtrl();

	BOOL Create( DWORD dwStyle, const CRect& rc, CWnd* pParentWnd, UINT nID );

	void SetGraph(CHyperGraph* pGraph);
	void UpdateGraphProperties(CHyperGraph* pGraph);

protected:
	DECLARE_MESSAGE_MAP()
	
	void Init(int nID);
	void FillProps();
	void ResizeProps(bool bHide=false);

protected:

	CSmartVariableEnum<CString> CreateVariable(const char* name, EFlowDataTypes type);

	CFlowGraph*        m_pGraph;
	CHyperGraphDialog* m_pParent;
	CXTPTaskPanel*     m_pTaskPanel;
	CXTPTaskPanelGroup*m_pGroup;
	CXTPTaskPanelGroupItem* m_pPropItem;

	CPropertyCtrl m_graphProps;

	bool m_bUpdate;
};

//////////////////////////////////////////////////////////////////////////
// CFlowGraphEditTokensDlg - popup dialog allowing adding /
//	removing / editing graph tokens
//////////////////////////////////////////////////////////////////////////

struct STokenData
{
	STokenData() : type(eFDT_Void)
	{}

	inline bool operator==(const STokenData& other)
	{
		return (name == other.name) && (type == other.type);
	}

	CString name;
	EFlowDataTypes type;
};
typedef std::vector<STokenData> TTokens;

class CFlowGraphTokensDlg : public CDialog
{
	DECLARE_DYNAMIC(CFlowGraphTokensDlg)

public:
	CFlowGraphTokensDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFlowGraphTokensDlg() {};

	// Dialog Data
	enum { IDD = IDD_FG_EDIT_GRAPH_TOKEN };

	CButton m_newTokenButton;
	CButton m_deleteTokenButton;
	CButton m_editTokenButton;
	CListBox m_tokenListCtrl;

	void RefreshControl();

	TTokens& GetTokenData() { return m_tokens; }
	void SetTokenData(const TTokens& tokens) { m_tokens = tokens; }

protected:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	
	afx_msg void OnCommand_NewToken();
	afx_msg void OnCommand_DeleteToken();
	afx_msg void OnCommand_EditToken();

	DECLARE_MESSAGE_MAP()
	
	TTokens m_tokens;
};


//////////////////////////////////////////////////////////////////////////
// CFlowGraphNewTokenDlg - popup dialog creating a new graph token
//	or editing an existing one
//////////////////////////////////////////////////////////////////////////

class CFlowGraphNewTokenDlg : public CDialog
{
	DECLARE_DYNAMIC(CFlowGraphNewTokenDlg)

public:
	CFlowGraphNewTokenDlg(STokenData* pTokenData, CWnd* pParent = NULL);   // standard constructor
	virtual ~CFlowGraphNewTokenDlg() {}

	// Dialog Data
	enum { IDD = IDD_FG_NEW_GRAPH_TOKEN };

	CComboBox m_tokenTypesCtrl;
	CEdit m_tokenNameCtrl;
	 
protected:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

	void RefreshControl();

	STokenData* m_pTokenData;
};


#endif
