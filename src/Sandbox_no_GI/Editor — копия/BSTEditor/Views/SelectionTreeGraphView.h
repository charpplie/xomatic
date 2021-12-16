////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeGraphView.h
//  Version:     v1.00
//  Created:     17/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE_GRAPH_VIEW__H__
#define __SELECTION_TREE_GRAPH_VIEW__H__

#include "SelectionTreeBaseDockView.h"
#include "BSTEditor/Graph/SelectionTreeHyperGraphView.h"

#include "Util/IXmlHistoryManager.h"

class CSelectionTreeGraphManager;
class CSelectionTreeGraph;

class CSelectionTreeGraphView
	: public CSelectionTreeBaseDockView
	, public IXmlHistoryView
{
	DECLARE_DYNCREATE( CSelectionTreeGraphView )

public:
	CSelectionTreeGraphView();
	virtual ~CSelectionTreeGraphView();

	// IXmlHistoryView
	virtual bool LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex );
	virtual void UnloadXml( int typeId );

	CSelectionTreeGraph* GetGraph() const { return m_pGraph; }
protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();

private:
	CSelectionTreeHyperGraphView m_hyperGraphView;
	CSelectionTreeGraphManager* m_pGraphManager;
	CSelectionTreeGraph* m_pGraph;
};

#endif