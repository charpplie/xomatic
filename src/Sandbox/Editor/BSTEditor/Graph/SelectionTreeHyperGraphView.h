////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeHyperGraphView.h
//  Version:     v1.00
//  Created:     17/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#ifndef __SELCTION_TREE_HYPER_GRAPH_VIEW__H__
#define __SELCTION_TREE_HYPER_GRAPH_VIEW__H__

#include "HyperGraph/HyperGraphView.h"

class CSelectionTreeGraph;
class CSelectionTreeGraphManager;
class CSelectionTree_BaseNode;
class CSelectionTreeEditor;

enum EBSTOrderMode
{
	eBOM_Disabled,
	eBOM_Auto,
	eBOM_AutoSmooth,
};

class CSelectionTreeHyperGraphView
	: public CHyperGraphView
{
	DECLARE_DYNAMIC( CSelectionTreeHyperGraphView )

public:
	CSelectionTreeHyperGraphView();
	virtual ~CSelectionTreeHyperGraphView();

	void ResetView();
	EBSTOrderMode GetOrderMode() const { return m_NodeOrderMode; }

	void CenterAroundNode(CHyperNode* pNode) { CenterViewAroundNode(pNode); }
protected:
	afx_msg void OnUndo();
	afx_msg void OnRedo();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	DECLARE_MESSAGE_MAP()

	virtual void ShowContextMenu( CPoint point, CHyperNode* pNode );
	virtual void UpdateTooltip( CHyperNode* pNode, CHyperNodePort* pPort ) {}
	virtual bool HandleRenameNode( CHyperNode *pNode );
	virtual void OnNodesConnected();
	virtual void OnEdgeRemoved();
	virtual void OnNodeRenamed();


	void ReorderAllNodes();
	void UnselectAll();
private:
	void LoadTemplates();
	CSelectionTreeGraph* GetSelectionTreeGraph();
	const CSelectionTreeGraph* GetSelectionTreeGraph() const;

	CSelectionTreeGraphManager* GetSelectionTreeGraphManager();
	const CSelectionTreeGraphManager* GetSelectionTreeGraphManager() const;

	void DeleteNodesInt();
	void CutNodesInt();
	void CopyNodesInt();
	void PasteNodesInt( CPoint point, bool withLinks = true );

	bool CheckForReadOnlyNodes();

	bool IsGraphLoaded();
private:
	EBSTOrderMode m_NodeOrderMode;
	EBSTOrderMode m_InternalNodeOrderMode;
	bool m_bCtrlPressed;

	std::map<int, string> m_TemplateMap;
	std::map<string, int> m_TemplateMapSorted;
};

#endif

