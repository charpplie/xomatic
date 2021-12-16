////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeGraphView.cpp
//  Version:     v1.00
//  Created:     17/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SelectionTreeGraphView.h"

#include "BSTEditor/Graph/SelectionTreeGraph.h"
#include "BSTEditor/Graph/SelectionTreeLeafMapping.h"
#include "BSTEditor/Graph/SelectionTreeGraphManager.h"

#include "BSTEditor/SelectionTreeManager.h"

IMPLEMENT_DYNAMIC( CSelectionTreeGraphView, CSelectionTreeBaseDockView )

BEGIN_MESSAGE_MAP( CSelectionTreeGraphView, CSelectionTreeBaseDockView )
END_MESSAGE_MAP()


#define IDC_SELECTION_TREE_HYPERGRAPH_CONTROL 666666


CSelectionTreeGraphView::CSelectionTreeGraphView()
: m_pGraphManager( NULL )
, m_pGraph( NULL )
{
}


CSelectionTreeGraphView::~CSelectionTreeGraphView()
{
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UndoEventHandlerDestroyed( m_pGraph, eSTTI_Tree, false );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UndoEventHandlerDestroyed( m_pGraph->GetLeafTranslationMan(), eSTTI_LeafTranslations, false );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UnregisterView( this );
	GetIEditor()->GetSelectionTreeManager()->SetTreeGraph( NULL );

	m_hyperGraphView.SetGraph( NULL );
	m_pGraph = NULL;
	// Tmp workaround for crash on closing
	//SAFE_DELETE( m_pGraphManager );
}

BOOL CSelectionTreeGraphView::OnInitDialog()
{
	__super::OnInitDialog();

	CRect rc;
	GetClientRect( rc );

	m_hyperGraphView.Create( WS_CHILD | WS_VISIBLE | WS_BORDER, rc, this, IDC_SELECTION_TREE_HYPERGRAPH_CONTROL );
	m_hyperGraphView.ModifyStyleEx( 0, WS_EX_CLIENTEDGE );
	m_hyperGraphView.MoveWindow( &rc );

	SetResize( IDC_SELECTION_TREE_HYPERGRAPH_CONTROL, SZ_TOP_LEFT, SZ_BOTTOM_RIGHT );

	m_pGraphManager = new CSelectionTreeGraphManager();
	m_pGraph = static_cast< CSelectionTreeGraph* >( m_pGraphManager->CreateGraph() );
	m_pGraph->SetGraphView( &m_hyperGraphView );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RegisterView( this );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RestoreUndoEventHandler( m_pGraph, eSTTI_Tree );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RestoreUndoEventHandler( m_pGraph->GetLeafTranslationMan(), eSTTI_LeafTranslations );

	return TRUE;
}

bool CSelectionTreeGraphView::LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex )
{
	bool res = false;
	switch ( typeId  )
	{
	case eSTTI_Tree:
		m_hyperGraphView.SetGraph( m_pGraph );
		GetIEditor()->GetSelectionTreeManager()->SetTreeGraph( m_pGraph );
		pUndoEventHandler = m_pGraph;
		res = m_pGraph->LoadFromXml( xmlNode );
		m_hyperGraphView.ResetView();
		break;
	case eSTTI_LeafTranslations:
		pUndoEventHandler = m_pGraph->GetLeafTranslationMan();
		res = m_pGraph->GetLeafTranslationMan()->LoadFromXml( xmlNode );
		break;
	}
	return res;
}

void CSelectionTreeGraphView::UnloadXml( int typeId )
{
	if ( typeId == eSTTI_Tree || typeId == eSTTI_All )
	{
		m_hyperGraphView.SetGraph( NULL );
		m_pGraph->ClearGraph();
		GetIEditor()->GetSelectionTreeManager()->SetTreeGraph( NULL );
	}
}
