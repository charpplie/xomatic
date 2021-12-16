////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeEditor.cpp
//  Version:     v1.00
//  Created:     17/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SelectionTreeEditor.h"

#include "Graph/SelectionTreeGraph.h"
#include "SelectionTreeManager.h"

#include "SelectionTreeModifier.h"


CryCriticalSection CSelectionTreeEditor::m_sLock;

IMPLEMENT_DYNCREATE( CSelectionTreeEditor, CBaseFrameWnd )

BEGIN_MESSAGE_MAP( CSelectionTreeEditor, CBaseFrameWnd )
	ON_BN_CLICKED(ID_BST_BTN_FILE_RELOAD, &CSelectionTreeEditor::OnBtnClickedReload)
	ON_BN_CLICKED(ID_BST_BTN_FILE_SAVEALL, &CSelectionTreeEditor::OnBtnClickedSaveAll)
	ON_BN_CLICKED(ID_BST_BTN_NEW_TREE, &CSelectionTreeEditor::OnBtnClickedNewTree)
	ON_BN_CLICKED(ID_BST_BTN_NEW_BLOCK, &CSelectionTreeEditor::OnBtnClickedNewBlock)
	ON_BN_CLICKED(ID_BST_BTN_CLRHISTORY, &CSelectionTreeEditor::OnBtnClickedClearHistory)
	ON_BN_CLICKED(ID_BST_BTN_DISPLAYWARNING, &CSelectionTreeEditor::OnBtnDisplayWarnings)
END_MESSAGE_MAP()

const int ID_SELECTION_TREE_EDITOR_PANEL = IDD_DATABASE;

enum
{
	ID_SELECTION_TREE_EDITOR_BASE = 60000,
	ID_SELECTION_TREE_GRAPH_VIEW,
	ID_SELECTION_TREE_NODE_PROPERTIES_VIEW,
	ID_SELECTION_TREE_VARIABLES_VIEW,
	ID_SELECTION_TREE_SIGNALS_VIEW,
	ID_SELECTION_TREE_LIST_VIEW,
	ID_SELECTION_TREE_HISTORY_VIEW,
};

static CSelectionTreeEditor* s_pInstance = NULL;
CSelectionTreeEditor* CSelectionTreeEditor::GetInstance()
{
	return s_pInstance;
}

CSelectionTreeEditor::CSelectionTreeEditor()
	: m_pGraphViewDockingPane( NULL )
	, m_pTreeListDockingPane( NULL )
	, m_pVariablesDockingPane( NULL )
	, m_pSignalsDockingPane( NULL )
	, m_pHistoryDockingPane( NULL )
{
	CRect rc( 0, 0, 0, 0 );
	Create( WS_CHILD | WS_VISIBLE, rc, AfxGetMainWnd() );
	s_pInstance = this;
}

CSelectionTreeEditor::~CSelectionTreeEditor()
{
	if (s_pInstance == this)
		s_pInstance = NULL;
}

BOOL CSelectionTreeEditor::OnInitDialog()
{
	CryAutoCriticalSection lock(m_sLock);

	// Get a pointer to the command bars object.
	CXTPCommandBars* pCommandBars = GetCommandBars();
	if(pCommandBars == NULL)
	{
		TRACE0("Failed to create command bars object.\n");
		return -1;      // fail to create
	}

	CXTPCommandBar* pMenuBar = pCommandBars->SetMenu( _T("Menu Bar"),IDR_BST_MENU );
	ASSERT(pMenuBar);
	pMenuBar->SetFlags(xtpFlagStretched);
	pMenuBar->EnableCustomization(TRUE);

	GetDockingPaneManager()->HideClient( TRUE );
	GetDockingPaneManager()->SetTheme( CMainFrame::GetDockingPaneTheme() );

	m_graphView.Create( ID_SELECTION_TREE_EDITOR_PANEL, this );
	m_variablesView.Create( ID_SELECTION_TREE_EDITOR_PANEL, this );
	m_signalsView.Create( ID_SELECTION_TREE_EDITOR_PANEL, this );
	m_treeListView.Create( ID_SELECTION_TREE_EDITOR_PANEL, this );
	m_historyView.Create( ID_SELECTION_TREE_EDITOR_PANEL, this );

	AttachDockingWnd( ID_SELECTION_TREE_GRAPH_VIEW, &m_graphView, "Selection Tree Structure" );
	AttachDockingWnd( ID_SELECTION_TREE_LIST_VIEW, &m_treeListView, "SelectionTrees", xtpPaneDockLeft, GetDockingPaneManager()->FindPane(ID_SELECTION_TREE_GRAPH_VIEW) );
	AttachDockingWnd( ID_SELECTION_TREE_VARIABLES_VIEW, &m_variablesView, "Variables", xtpPaneDockTop, GetDockingPaneManager()->FindPane(ID_SELECTION_TREE_GRAPH_VIEW) );
	AttachDockingWnd( ID_SELECTION_TREE_SIGNALS_VIEW, &m_signalsView, "Signals", xtpPaneDockRight, GetDockingPaneManager()->FindPane(ID_SELECTION_TREE_VARIABLES_VIEW) );
	AttachDockingWnd( ID_SELECTION_TREE_HISTORY_VIEW, &m_historyView, "History", xtpPaneDockRight, GetDockingPaneManager()->FindPane(ID_SELECTION_TREE_SIGNALS_VIEW) );

	AutoLoadFrameLayout( "BSTEditor" );

	m_pGraphViewDockingPane = GetDockingPaneManager()->FindPane(ID_SELECTION_TREE_GRAPH_VIEW);
	m_pTreeListDockingPane = GetDockingPaneManager()->FindPane(ID_SELECTION_TREE_LIST_VIEW);
	m_pVariablesDockingPane = GetDockingPaneManager()->FindPane(ID_SELECTION_TREE_VARIABLES_VIEW);
	m_pSignalsDockingPane = GetDockingPaneManager()->FindPane(ID_SELECTION_TREE_SIGNALS_VIEW);
	m_pHistoryDockingPane = GetDockingPaneManager()->FindPane(ID_SELECTION_TREE_HISTORY_VIEW);
	assert(m_pGraphViewDockingPane && m_pTreeListDockingPane && m_pVariablesDockingPane && m_pSignalsDockingPane && m_pHistoryDockingPane);

	static volatile bool bFirstOpen = true;

	if ( bFirstOpen )
	{
		bFirstOpen = false;
		GetIEditor()->GetSelectionTreeManager()->LoadFromFolder( "Scripts\\AI\\SelectionTrees\\" );
	}
	GetIEditor()->GetSelectionTreeManager()->UnloadViews();
	m_treeListView.ClearList();
	m_treeListView.UpdateSelectionTrees();
	m_historyView.LoadHistory();

	return TRUE;
}


void CSelectionTreeEditor::AttachDockingWnd( UINT wndId, CWnd* pWnd, const CString& dockingPaneTitle, XTPDockingPaneDirection direction, CXTPDockingPaneBase* pNeighbour )
{
	assert( pWnd != NULL );
	if ( pWnd != NULL )
	{
		CRect rc( 0, 0, 400, 400 );

		CXTPDockingPane* pDockPane = CreateDockingPane(dockingPaneTitle, pWnd, wndId, rc, direction, pNeighbour );// pDockingPaneManager->CreatePane( wndId, rc, direction, pNeighbour );
		pDockPane->SetOptions( xtpPaneNoCloseable );

		pWnd->SetOwner( this );
	}
}

void CSelectionTreeEditor::SetTreeName(const char* name)
{
	if (m_pGraphViewDockingPane)
	{
		m_pGraphViewDockingPane->SetTitle( name );
	}
}

void CSelectionTreeEditor::SetVariablesName(const char* name)
{
	if (m_pVariablesDockingPane)
	{
		m_pVariablesDockingPane->SetTitle( name );
	}
}

void CSelectionTreeEditor::SetSignalsName(const char* name)
{
	if (m_pSignalsDockingPane)
	{
		m_pSignalsDockingPane->SetTitle( name );
	}
}


void CSelectionTreeEditor::OnBtnClickedReload()
{
	if (m_graphView.GetGraph() && m_graphView.GetGraph()->IsInTranslationEditMode()) return;

	GetIEditor()->GetSelectionTreeManager()->LoadFromFolder( "Scripts\\AI\\SelectionTrees\\" );
	m_treeListView.UpdateSelectionTrees();
}

void CSelectionTreeEditor::OnBtnClickedSaveAll()
{
	if (m_graphView.GetGraph() && m_graphView.GetGraph()->IsInTranslationEditMode()) return;

	GetIEditor()->GetSelectionTreeManager()->SaveAll();
}

void CSelectionTreeEditor::OnBtnClickedNewTree()
{
	if (m_graphView.GetGraph() && m_graphView.GetGraph()->IsInTranslationEditMode()) return;

	GetIEditor()->GetSelectionTreeManager()->GetModifier()->CreateNewTree();
}

void CSelectionTreeEditor::OnBtnClickedNewBlock()
{
	if (m_graphView.GetGraph() && m_graphView.GetGraph()->IsInTranslationEditMode()) return;

	GetIEditor()->GetSelectionTreeManager()->GetModifier()->CreateNewRefBlock();
}

void CSelectionTreeEditor::OnBtnClickedClearHistory()
{
	if (m_graphView.GetGraph() && m_graphView.GetGraph()->IsInTranslationEditMode()) return;

	GetIEditor()->GetSelectionTreeManager()->GetHistory()->ClearHistory();
	m_historyView.LoadHistory();
}

void CSelectionTreeEditor::OnBtnDisplayWarnings()
{
	GetIEditor()->GetSelectionTreeManager()->DisplayErrors();
}


