////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeEditor.h
//  Version:     v1.00
//  Created:     17/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE_EDITOR__H__
#define __SELECTION_TREE_EDITOR__H__

#include "Dialogs/BaseFrameWnd.h"

#include "Views/SelectionTreeGraphView.h"
#include "Views/SelectionTreeVariablesView.h"
#include "Views/SelectionTreeSignalsView.h"
#include "Views/SelectionTreeListView.h"
#include "Views/SelectionTreePropertiesView.h"
#include "Views/SelectionTreeHistoryView.h"

class CSelectionTreeEditor
	: public CBaseFrameWnd
{
	DECLARE_DYNCREATE( CSelectionTreeEditor )

public:
	CSelectionTreeEditor();
	virtual ~CSelectionTreeEditor();

	void SetTreeName(const char* name);
	void SetVariablesName(const char* name);
	void SetSignalsName(const char* name);
	void SetTimestampsName(const char* name);

	static CSelectionTreeEditor* GetInstance();

protected:
	virtual BOOL OnInitDialog();

	void AttachDockingWnd( UINT wndId, CWnd* pWnd, const CString& dockingPaneTitle, XTPDockingPaneDirection direction = xtpPaneDockLeft, CXTPDockingPaneBase* pNeighbour = NULL );

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBtnClickedReload();
	afx_msg void OnBtnClickedSaveAll();
	afx_msg void OnBtnClickedNewTree();
	afx_msg void OnBtnClickedClearHistory();
	afx_msg void OnBtnDisplayWarnings();

private:
	CSelectionTreeGraphView m_graphView;
	CSelectionTreeTimestampsView m_timestampView;
	CSelectionTreePropertiesView m_propertiesView;
	CSelectionTreeVariablesView m_variablesView;
	CSelectionTreeSignalsView m_signalsView;
	CSelectionTreeListView m_treeListView;
	CSelectionTreeHistoryView m_historyView;

	CXTPDockingPane* m_pGraphViewDockingPane;
	CXTPDockingPane* m_pTreeListDockingPane;
	CXTPDockingPane* m_pVariablesDockingPane;
	CXTPDockingPane* m_pSignalsDockingPane;
	CXTPDockingPane* m_pPropertiesDockingPane;
	CXTPDockingPane* m_pTimestampsDockingPane;
	CXTPDockingPane* m_pHistoryDockingPane;

	static CryCriticalSection m_sLock;
};

#endif
