////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeListView.cpp
//  Version:     v1.00
//  Created:     11/02/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SelectionTreeListView.h"
#include "BSTEditor/SelectionTreeModifier.h"

#define GRAPH_FILE_FILTER "Graph XML Files (*.xml)|*.xml"

IMPLEMENT_DYNAMIC( CSelectionTreeListView, CSelectionTreeBaseDockView )

#define ID_TREE_CONTROL 1

BEGIN_MESSAGE_MAP( CSelectionTreeListView, CSelectionTreeBaseDockView )
	ON_NOTIFY( NM_CLICK, ID_TREE_CONTROL, OnTvnClick )
	ON_NOTIFY( NM_RCLICK , ID_TREE_CONTROL, OnTvnRightClick )
	ON_NOTIFY( NM_DBLCLK , ID_TREE_CONTROL, OnTvnDblClick )
END_MESSAGE_MAP()

typedef enum
{
	eSTVMC_DoNothing = 0,

	eSTVMC_AddTree,
	eSTVMC_EditTree,
	eSTVMC_RemoveTree,

	eSTVMC_AddBlock,
	eSTVMC_EditBlock,
	eSTVMC_RemoveBlock,

	eSTVMC_RemoveTreeFromBlock,
	eSTVMC_RenameTreeFromBlock,
	eSTVMC_AddTreeToBlock,

} SelectionTreeListViewMenuCommands;

CSelectionTreeListView::CSelectionTreeListView()
{
}

CSelectionTreeListView::~CSelectionTreeListView()
{
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UnregisterEventListener( this );
}

BOOL CSelectionTreeListView::OnInitDialog()
{
	BOOL baseInitSuccess = __super::OnInitDialog();
	if ( ! baseInitSuccess )
	{
		return FALSE;
	}

	CRect rc;
	GetClientRect( rc );

	m_treelist.Create( WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS, rc, this, ID_TREE_CONTROL );
	m_treelist.ModifyStyleEx( 0, WS_EX_CLIENTEDGE );

	SetResize( ID_TREE_CONTROL, SZ_TOP_LEFT, SZ_BOTTOM_RIGHT );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RegisterEventListener( this );

	return TRUE;
}

void CSelectionTreeListView::OnTvnClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	CPoint clientPoint;
	::GetCursorPos( &clientPoint );
	m_treelist.ScreenToClient( &clientPoint );

	const HTREEITEM clickedItemHandle = m_treelist.HitTest( clientPoint );

	if ( clickedItemHandle != 0 )
	{
		int treeIndex = 0;
		const char* name = GetDataByItem( clickedItemHandle, &treeIndex );
		if ( name )
		{
			const HTREEITEM parentItemHandle = m_treelist.GetParentItem( clickedItemHandle );
			if ( parentItemHandle == m_treeRoot ) // display tree
			{
				GetIEditor()->GetSelectionTreeManager()->DisplayTree( name );
			}
			else if ( parentItemHandle == m_blockRoot ) // display ref group (with first tree)
			{
				GetIEditor()->GetSelectionTreeManager()->DisplayRefGroup( name );
			}
			else if ( parentItemHandle ) // display ref group (with selected tree)
			{
				name = GetDataByItem( parentItemHandle );
				if ( name )
				{
					GetIEditor()->GetSelectionTreeManager()->DisplayRefGroup( name, treeIndex );
				}
			}
		}
	}
}

void CSelectionTreeListView::OnTvnDblClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	CPoint screenPoint;
	::GetCursorPos( &screenPoint );

	CPoint clientPoint = screenPoint;
	m_treelist.ScreenToClient( &clientPoint );

	const HTREEITEM clickedItemHandle = m_treelist.HitTest( clientPoint );

	const char* name = NULL;
	const char* group = NULL;
	int treeIndex = 0;

	if ( clickedItemHandle != 0 )
	{
		group = name = GetDataByItem( clickedItemHandle, &treeIndex );
		if ( name )
		{
			const HTREEITEM parentItemHandle = m_treelist.GetParentItem( clickedItemHandle );
			if ( parentItemHandle == m_treeRoot ) //  tree
			{
				string nameStr = name;
				GetIEditor()->GetSelectionTreeManager()->GetModifier()->EditTree( nameStr.c_str() );
			}
			else if ( parentItemHandle == m_blockRoot ) // ref group (with first tree)
			{
				string nameStr = name;
				GetIEditor()->GetSelectionTreeManager()->GetModifier()->EditBlock( nameStr.c_str() );
			}
			else if ( parentItemHandle ) // ref group (with selected tree)
			{
				group = GetDataByItem( parentItemHandle );
				if ( group )
				{
					string nameStr = name;
					string groupStr = group;
					GetIEditor()->GetSelectionTreeManager()->GetModifier()->RenameTreeFromBlock( nameStr.c_str(), groupStr.c_str() );
				}
			}
		}
	}
}

void CSelectionTreeListView::OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	const HTREEITEM selectedItemHandle = m_treelist.GetSelectedItem();

	CPoint screenPoint;
	::GetCursorPos( &screenPoint );

	CPoint clientPoint = screenPoint;
	m_treelist.ScreenToClient( &clientPoint );

	const HTREEITEM clickedItemHandle = m_treelist.HitTest( clientPoint );

	CMenu menu;
	menu.CreatePopupMenu();
	const char* name = NULL;
	const char* group = NULL;
	int treeIndex = 0;
	if ( clickedItemHandle != 0 )
	{
		group = name = GetDataByItem( clickedItemHandle, &treeIndex );
		if ( name )
		{
			const HTREEITEM parentItemHandle = m_treelist.GetParentItem( clickedItemHandle );
			if ( parentItemHandle == m_treeRoot ) //  tree
			{
				menu.AppendMenu( MF_STRING, eSTVMC_EditTree, "Edit tree" );
				menu.AppendMenu( MF_STRING, eSTVMC_RemoveTree, "Delete tree" );
			}
			else if ( parentItemHandle == m_blockRoot ) // ref group (with first tree)
			{
				menu.AppendMenu( MF_STRING, eSTVMC_EditBlock, "Edit ref block" );
				menu.AppendMenu( MF_STRING, eSTVMC_RemoveBlock, "Delete ref block" );
				menu.AppendMenu(MF_SEPARATOR);
				menu.AppendMenu( MF_STRING, eSTVMC_AddTreeToBlock, "Add new tree" );
			}
			else if ( parentItemHandle ) // ref group (with selected tree)
			{
				group = GetDataByItem( parentItemHandle );
				if ( group )
				{
					menu.AppendMenu( MF_STRING, eSTVMC_RenameTreeFromBlock, "Edit tree" );
					menu.AppendMenu( MF_STRING, eSTVMC_RemoveTreeFromBlock, "Delete tree from ref block" );
				}
			}
		}
		else if ( clickedItemHandle == m_treeRoot )
		{
			menu.AppendMenu( MF_STRING, eSTVMC_AddTree, "Add new tree" );
		}
		else if ( clickedItemHandle == m_blockRoot )
		{
			menu.AppendMenu( MF_STRING, eSTVMC_AddBlock, "Add new ref block" );
		}

	}

	string nameStr = name ? name : "";
	string groupStr = group ? group : "";

	const int commandId = ::TrackPopupMenuEx( menu.GetSafeHmenu(), TPM_LEFTBUTTON | TPM_RETURNCMD, screenPoint.x, screenPoint.y, GetSafeHwnd(), NULL );

	if ( commandId == eSTVMC_DoNothing )
	{
		return;
	}

	if ( commandId == eSTVMC_AddTree )
	{
		GetIEditor()->GetSelectionTreeManager()->GetModifier()->CreateNewTree();
		return;
	}

	if ( commandId == eSTVMC_EditTree )
	{
		GetIEditor()->GetSelectionTreeManager()->GetModifier()->EditTree( nameStr.c_str() );
		return;
	}

	if ( commandId == eSTVMC_RemoveTree )
	{
		GetIEditor()->GetSelectionTreeManager()->DeleteTree( nameStr.c_str() );
		return;
	}

	if ( commandId == eSTVMC_AddBlock )
	{
		GetIEditor()->GetSelectionTreeManager()->GetModifier()->CreateNewRefBlock();
		return;
	}

	if ( commandId == eSTVMC_EditBlock )
	{
		GetIEditor()->GetSelectionTreeManager()->GetModifier()->EditBlock( nameStr.c_str() );
		return;
	}

	if ( commandId == eSTVMC_RemoveBlock )
	{
		GetIEditor()->GetSelectionTreeManager()->DeleteRef( nameStr.c_str() );
		return;
	}

	if ( commandId == eSTVMC_RemoveTreeFromBlock )
	{
		GetIEditor()->GetSelectionTreeManager()->DeleteTreeFromBlock( nameStr.c_str(), groupStr.c_str() );
		return;
	}

	if ( commandId == eSTVMC_RenameTreeFromBlock )
	{
		GetIEditor()->GetSelectionTreeManager()->GetModifier()->RenameTreeFromBlock( nameStr.c_str(), groupStr.c_str() );
		return;
	}

	if ( commandId == eSTVMC_AddTreeToBlock )
	{
		GetIEditor()->GetSelectionTreeManager()->GetModifier()->AddTreeToBlock( groupStr.c_str() );
		return;
	}
}

void CSelectionTreeListView::OnEvent( EHistoryEventType event, void* pData )
{
	switch ( event )
	{
	case eHET_HistoryDeleted:
		ClearList();
		break;
	case eHET_HistoryGroupAdded:
	case eHET_HistoryGroupRemoved:
	case eHET_HistoryInvalidate:
	case eHET_HistoryCleared:
		ClearList();
	default:
		UpdateSelectionTrees();
		break;
	}

}

void CSelectionTreeListView::UpdateSelectionTrees()
{
	TSelectionTreeInfoList infoList;
	GetIEditor()->GetSelectionTreeManager()->GetInfoList( infoList );

	HTREEITEM selectedItem = 0;
	for ( TSelectionTreeInfoList::const_iterator it = infoList.begin(); it != infoList.end(); ++it )
	{

		const SSelectionTreeInfo& info = *it;

		HTREEITEM item = GetOrCreateItemForInfo(info);
		if ( info.IsLoaded ) selectedItem = item;

		if ( !info.IsTree && info.GetBlockCountById(eSTTI_Tree) > 1 )
		{
			int treeCount = 0;
			for ( int i = 0; i < info.GetBlockCount(); ++i )
			{

				if (info.Blocks[ i ].Type != eSTTI_Tree)
					continue;

				HTREEITEM subitem = GetOrCreateItemForRefTreeInfo(info.Blocks[ i ], item, treeCount);
				if ( info.IsLoaded && treeCount == info.CurrTreeIndex ) selectedItem = subitem;
				treeCount++;
			}
		}
	}
	m_treelist.SelectItem( selectedItem );
}

void CSelectionTreeListView::ClearList()
{
	m_treelist.DeleteAllItems();
	m_ItemList.clear();
	m_treeRoot = m_treelist.InsertItem( "SelectionTrees", 0, 0, TVI_ROOT, TVI_LAST );
	m_blockRoot = m_treelist.InsertItem( "Blocks", 0, 0, TVI_ROOT, TVI_LAST );
}

const char* CSelectionTreeListView::GetDataByItem( HTREEITEM item, int* pIndex /*= NULL*/ )

{
	for ( TItemList::const_iterator it = m_ItemList.begin(); it != m_ItemList.end(); ++it )
	{
		if ( it->Item == item )
		{
			if(pIndex) *pIndex = it->Index;
			return it->Name.c_str();
		}
	}
	return NULL;
}

HTREEITEM CSelectionTreeListView::GetItemByInfo(const SSelectionTreeInfo& info)
{
	for ( TItemList::const_iterator it = m_ItemList.begin(); it != m_ItemList.end(); ++it )
	{
		if (it->Name == info.Name)
		{
			if (info.IsTree && m_treelist.GetParentItem(it->Item) == m_treeRoot)
				return it->Item;
			else if (!info.IsTree && m_treelist.GetParentItem(it->Item) == m_blockRoot)
				return it->Item;
		}
	}
	return 0;
}

HTREEITEM CSelectionTreeListView::GetItemByRefTreeInfo(const SSelectionTreeBlockInfo &info, HTREEITEM parent)
{
	for ( TItemList::const_iterator it = m_ItemList.begin(); it != m_ItemList.end(); ++it )
	{
		if (it->Name == info.Name)
		{
			if (m_treelist.GetParentItem(it->Item) == parent)
				return it->Item;
		}
	}
	return 0;
}

HTREEITEM CSelectionTreeListView::GetOrCreateItemForInfo( const SSelectionTreeInfo &info )
{
	HTREEITEM item = GetItemByInfo(info);

	string name = GetDisplayString(info);
	if ( info.IsModified )
		name += "*";

	if (!item)
	{
		HTREEITEM parent = info.IsTree ? m_treeRoot : m_blockRoot;
		item = m_treelist.InsertItem( name, 0, 0, parent, TVI_SORT );
		SItemInfo iteminfo;
		iteminfo.Name = info.Name;
		iteminfo.Item = item;
		iteminfo.Index = 0;
		m_ItemList.push_back(iteminfo);
		m_treelist.Expand(parent, TVE_EXPAND);
	}
	else
	{
		m_treelist.SetItemText( item,  name.c_str() );
	}
	return item;
}

HTREEITEM CSelectionTreeListView::GetOrCreateItemForRefTreeInfo( const SSelectionTreeBlockInfo &info, HTREEITEM parent, int index )
{
	HTREEITEM item = GetItemByRefTreeInfo(info, parent);

	string name = info.Name;
	if ( info.IsModified )
		name += "*";

	if (!item)
	{
		item = m_treelist.InsertItem( name, 0, 0, parent, TVI_SORT );
		SItemInfo iteminfo;
		iteminfo.Name = info.Name;
		iteminfo.Item = item;
		iteminfo.Index = index;
		m_ItemList.push_back(iteminfo);
		m_treelist.Expand(parent, TVE_EXPAND);
	}
	else
	{
		m_treelist.SetItemText( item,  name.c_str() );
	}
	return item;
}


string CSelectionTreeListView::GetDisplayString(const SSelectionTreeInfo& info) const
{
	string out;
	if (info.IsTree)
	{
		out = info.Name;
	}
	else
	{
		int treecount = info.GetBlockCountById(eSTTI_Tree);
		SSelectionTreeBlockInfo tree;
		info.GetBlockById(tree, eSTTI_Tree);
		if (treecount == 1) out = tree.Name;
		else out = info.Name;
		if (treecount > 1) out += string().Format(" (%i Trees)", treecount);
	}
	return out;
}

