////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeListView.h
//  Version:     v1.00
//  Created:     11/02/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE_LIST_VIEW__H__
#define __SELECTION_TREE_LIST_VIEW__H__

#include "SelectionTreeBaseDockView.h"
#include "BSTEditor/SelectionTreeManager.h"

class CSelectionTreeListView
	: public CSelectionTreeBaseDockView
	, public IXmlHistoryEventListener
{
	DECLARE_DYNAMIC( CSelectionTreeListView )

public:
	CSelectionTreeListView();
	virtual ~CSelectionTreeListView();

	void UpdateSelectionTrees();
	void ClearList();

	// IXmlHistoryEventListener
	virtual void OnEvent( EHistoryEventType event, void* pData = NULL );

protected:
	virtual BOOL OnInitDialog();

	afx_msg void OnTvnClick( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnTvnDblClick( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()

private:
	CTreeCtrl m_treelist;
	HTREEITEM m_treeRoot;
	HTREEITEM m_blockRoot;

	struct SItemInfo
	{
		string Name;
		HTREEITEM Item;
		int Index;
	};
	typedef std::vector< SItemInfo > TItemList;
	TItemList m_ItemList;

private:
	const char* GetDataByItem( HTREEITEM item, int* pIndex = NULL );

	HTREEITEM GetItemByInfo(const SSelectionTreeInfo& info);
	HTREEITEM GetItemByRefTreeInfo(const SSelectionTreeBlockInfo &info, HTREEITEM parent);
	HTREEITEM GetOrCreateItemForInfo( const SSelectionTreeInfo &info );
	HTREEITEM GetOrCreateItemForRefTreeInfo( const SSelectionTreeBlockInfo &info, HTREEITEM parent, int index );

	string GetDisplayString(const SSelectionTreeInfo& info) const;
};


#endif
