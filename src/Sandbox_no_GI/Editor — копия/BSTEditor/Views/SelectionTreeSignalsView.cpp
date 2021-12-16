////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeSignalsView.cpp
//  Version:     v1.00
//  Created:     22/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SelectionTreeSignalsView.h"

#include "StringDlg.h"
#include "CustomMessageBox.h"

#include "BSTEditor/Dialogs/SelectionTree_RefBrowser.h"
#include "BSTEditor/Dialogs/AddNewSignalDialog.h"

#include "BSTEditor/SelectionTreeManager.h"
#include "BSTEditor/SelectionTreeModifier.h"

IMPLEMENT_DYNAMIC( CSelectionTreeSignalsView, CSelectionTreeBaseDockView )

#define ID_TREE_CONTROL 1

BEGIN_MESSAGE_MAP( CSelectionTreeSignalsView, CSelectionTreeBaseDockView )
	ON_NOTIFY( NM_RCLICK , ID_TREE_CONTROL, OnTvnRightClick )
	ON_NOTIFY( NM_DBLCLK , ID_TREE_CONTROL, OnTvnDblClick )
END_MESSAGE_MAP()

typedef enum
{
	eSTVMC_DoNothing = 0,

	eSTVMC_AddSignal,
	eSTVMC_RemoveSignal,

	eSTVMC_AddSignalVariable,
	eSTVMC_RemoveSignalVariable,
	eSTVMC_ToggleValue,

	eSTVMC_AddReference,
	eSTVMC_RemoveReference,

} SelectionTreeSignalsMenuCommands;


CSelectionTreeSignalsView::CSelectionTreeSignalsView()
	: m_bLoaded( false )
{
}

CSelectionTreeSignalsView::~CSelectionTreeSignalsView()
{
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UndoEventHandlerDestroyed( this, eSTTI_Signals, false );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UnregisterView( this );
	GetIEditor()->GetSelectionTreeManager()->SetSigView( NULL );
}

BOOL CSelectionTreeSignalsView::OnInitDialog()
{
	BOOL baseInitSuccess = __super::OnInitDialog();
	if ( ! baseInitSuccess )
	{
		return FALSE;
	}

	CRect rc;
	GetClientRect( rc );

	m_signals.Create( WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS, rc, this, ID_TREE_CONTROL );
	m_signals.ModifyStyleEx( 0, WS_EX_CLIENTEDGE );

	SetResize( ID_TREE_CONTROL, SZ_TOP_LEFT, SZ_BOTTOM_RIGHT );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RegisterView( this );
	GetIEditor()->GetSelectionTreeManager()->SetSigView( this );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RestoreUndoEventHandler( this, eSTTI_Signals );

	return TRUE;
}


void CSelectionTreeSignalsView::OnTvnDblClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	if ( !m_bLoaded ) return;

	CPoint clientPoint;
	::GetCursorPos( &clientPoint );
	m_signals.ScreenToClient( &clientPoint );

	const HTREEITEM clickedItemHandle = m_signals.HitTest( clientPoint );
	HTREEITEM rootItem = clickedItemHandle;

	if ( clickedItemHandle != 0 )
	{
		m_signals.SelectItem( clickedItemHandle );

		EItemType type = GetItemType(clickedItemHandle);

		int depth = 0;
		for (;GetParentItem(rootItem) != TVI_ROOT; rootItem = GetParentItem(rootItem), depth++);

		if (type == eIT_SignalVariable && depth == 1)
		{
			ToggleValue( clickedItemHandle );
		}
	}
}

void CSelectionTreeSignalsView::OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	if ( !m_bLoaded ) return;

	CPoint screenPoint;
	::GetCursorPos( &screenPoint );

	CPoint clientPoint = screenPoint;
	m_signals.ScreenToClient( &clientPoint );

	HTREEITEM clickedItemHandle = m_signals.HitTest( clientPoint );

	enum ESelectedItemType
	{
		eSIT_NothingSelected = 0,
		eSIT_Signal,
		eSIT_SignalVariable,
		eSIT_Reference,
		eSIT_InReference
	};

	ESelectedItemType selectionType = eSIT_NothingSelected;
	bool bVal = false;
	bool isInRef = false; 
	HTREEITEM rootItem = clickedItemHandle;

	CMenu menu;
	menu.CreatePopupMenu();

	if ( clickedItemHandle != 0 )
	{
		m_signals.SelectItem( clickedItemHandle );

		EItemType type = GetItemType(clickedItemHandle);

		int depth = 0;
		for (;GetParentItem(rootItem) != TVI_ROOT; rootItem = GetParentItem(rootItem), depth++);

		switch (type)
		{
		case eIT_Ref:
			selectionType = depth == 0 ? eSIT_Reference : eSIT_InReference;
			break;
		case eIT_Signal:
			selectionType = depth == 0 ? eSIT_Signal : eSIT_InReference;
			break;
		case eIT_SignalVariable:
			selectionType = depth == 1 ? eSIT_SignalVariable : eSIT_InReference;
			{
				const SVarInfo& info = GetItemInfo(m_TreeInfo.VarInfo, clickedItemHandle);
				bVal = info.Value;
			}
			break;
		}
	}

	switch(selectionType)
	{
	case eSIT_NothingSelected:
		menu.AppendMenu( MF_STRING, eSTVMC_AddSignal, "Add New Signal" );
		menu.AppendMenu( MF_STRING, eSTVMC_AddReference, "Add New Reference" );
		break;
	case eSIT_Signal:
		menu.AppendMenu( MF_STRING, eSTVMC_AddSignalVariable, "Add Signal Variable" );
		menu.AppendMenu( MF_STRING, eSTVMC_RemoveSignal, "Remove All Signal Variables" );	
		break;
	case eSIT_SignalVariable:
		menu.AppendMenu( MF_STRING, eSTVMC_RemoveSignalVariable, "Remove Signal Variable" );
		menu.AppendMenu( MF_STRING, eSTVMC_ToggleValue, string().Format("Set Value to %s", bVal ? "false" : "true" ).c_str() );
		break;
	case eSIT_InReference:
		clickedItemHandle = rootItem;
	case eSIT_Reference:
		menu.AppendMenu( MF_STRING, eSTVMC_RemoveReference, "Remove Reference" );
		break;
	}

	const int commandId = ::TrackPopupMenuEx( menu.GetSafeHmenu(), TPM_LEFTBUTTON | TPM_RETURNCMD, screenPoint.x, screenPoint.y, GetSafeHwnd(), NULL );

	if ( commandId == eSTVMC_DoNothing )
	{
		return;
	}

	if ( commandId == eSTVMC_AddSignal )
	{
		AddSignal( clickedItemHandle );
		return;
	}

	if ( commandId == eSTVMC_RemoveSignal )
	{
		RemoveSignalVars( clickedItemHandle );
		return;
	}

	if ( commandId == eSTVMC_AddSignalVariable )
	{
		AddSigVar( clickedItemHandle );
		return;
	}

	if ( commandId == eSTVMC_RemoveSignalVariable )
	{
		DeleteSigVar( clickedItemHandle );
		return;
	}

	if ( commandId == eSTVMC_ToggleValue )
	{
		ToggleValue( clickedItemHandle );
		return;
	}

	if ( commandId == eSTVMC_AddReference )
	{
		AddRef( clickedItemHandle );
		return;
	}

	if ( commandId == eSTVMC_RemoveReference )
	{
		DeleteRef( clickedItemHandle );
		return;
	}
}

bool CSelectionTreeSignalsView::LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex )
{
	bool res = false;
	if ( typeId == eSTTI_Signals )
	{
		m_bLoaded = true;
		pUndoEventHandler = this;
		res = LoadFromXml( xmlNode );
	}
	return res;
}

void CSelectionTreeSignalsView::UnloadXml( int typeId )
{
	if ( typeId == eSTTI_Signals || typeId == eSTTI_All )
	{
		m_bLoaded = false;
		ClearList();
	}
}

void CSelectionTreeSignalsView::ClearList()
{
	m_TreeInfo.Clear();
	m_signals.DeleteAllItems();
}

HTREEITEM CSelectionTreeSignalsView::GetParentItem(HTREEITEM hItem)
{
	HTREEITEM parent = m_signals.GetParentItem(hItem);
	return parent ? parent : TVI_ROOT;
}

bool CSelectionTreeSignalsView::SaveToXml( XmlNodeRef& xmlNode )
{
	xmlNode->setTag("SignalVariables");
	const TChildItemList& rootChilds = GetChildList(TVI_ROOT);
	for ( TChildItemList::const_iterator it = rootChilds.begin(); it != rootChilds.end(); ++it )
	{
		const HTREEITEM& child = it->Item;
		assert(it->Type != eIT_SignalVariable);
		if (it->Type == eIT_Ref)
		{
			const SRefInfo& info = GetItemInfo(m_TreeInfo.RefInfo, child);

			XmlNodeRef newNode = gEnv->pSystem->CreateXmlNode( "Ref" );
			newNode->setAttr( "name", info.Name );
			xmlNode->addChild( newNode );
		}
		else if (it->Type == eIT_Signal)
		{
			const TChildItemList& signalChilds = GetChildList(child);
			for ( TChildItemList::const_iterator it = signalChilds.begin(); it != signalChilds.end(); ++it )
			{
				const HTREEITEM& variable = it->Item;
				assert(it->Type == eIT_SignalVariable);

				const SVarInfo& varinfo =GetItemInfo(m_TreeInfo.VarInfo, variable);
				XmlNodeRef newNode = gEnv->pSystem->CreateXmlNode( "Signal" );
				newNode->setAttr( "name", varinfo.Signal );
				newNode->setAttr( "variable", varinfo.Var );
				newNode->setAttr( "value", varinfo.Value ? "true" : "false" );
				xmlNode->addChild( newNode );
			}	
		}
	}
	return true;
}

bool CSelectionTreeSignalsView::LoadFromXml( const XmlNodeRef& xmlNode )
{
	ClearList();
	return LoadSignalNodes( xmlNode, TVI_ROOT );
}

bool CSelectionTreeSignalsView::ReloadFromXml( const XmlNodeRef& xmlNode )
{
	return LoadFromXml( xmlNode );
}

bool CSelectionTreeSignalsView::LoadSignalNodes( const XmlNodeRef& xmlNode, HTREEITEM hItem )
{
	// Signals
	for ( int i = 0; i < xmlNode->getChildCount(); ++i )
	{
		XmlNodeRef xmlChild = xmlNode->getChild( i );

		const char* tag = xmlChild->getTag();
		const char* name = xmlChild->getAttr( "name" );
		if ( tag && name && strcmpi( tag, "Signal" ) == 0 )
		{
			const char* variable = xmlChild->getAttr( "variable" );
			const char* value = xmlChild->getAttr( "value" );
			bool bValue = strcmpi( value, "true" ) == 0;

			if ( !CreateSignalItem( name, variable, bValue, hItem, TVI_SORT ) )
				return false;
		}
	}

	// References
	for ( int i = 0; i < xmlNode->getChildCount(); ++i )
	{
		XmlNodeRef xmlChild = xmlNode->getChild( i );

		const char* tag = xmlChild->getTag();
		const char* name = xmlChild->getAttr( "name" );
		if ( tag && name && strcmpi( tag, "Ref" ) == 0 )
		{
			if ( !CreateRefItem( name, hItem ) )
				return false;
		}
	}
	return true;
}

bool CSelectionTreeSignalsView::LoadRefSignals( const char* refname, HTREEITEM hItem )
{
	SSelectionTreeBlockInfo refInfo;
	bool ok = GetIEditor()->GetSelectionTreeManager()->GetRefInfoByName( eSTTI_Signals, refname, refInfo );
	if ( ok )
	{
		return LoadSignalNodes( refInfo.XmlData, hItem );
	}
	return false;
}

bool CSelectionTreeSignalsView::CreateSignalItem( const char* name, const char* varname, bool val, HTREEITEM hItem, HTREEITEM hInsertAfter )
{
	HTREEITEM hSignal = NULL;

	TSignalInfoMap::iterator it = m_TreeInfo.SignalInfo.begin();
	for (;it != m_TreeInfo.SignalInfo.end(); ++it)
	{
		if (it->second.Name == name && GetParentItem(it->first) == hItem) break;
	}

	if ( it != m_TreeInfo.SignalInfo.end() )
	{
		hSignal = it->first;
	}
	else
	{
		hSignal = InsertItem( hItem, hInsertAfter, name, m_TreeInfo.SignalInfo, SSignalInfo(name) );
	}

	string namestr;
	namestr.Format("%s [%s]", varname, val ? "true" : "false" );
	InsertItem( hSignal, hInsertAfter, namestr.c_str(), m_TreeInfo.VarInfo, SVarInfo(name, varname, val) );

	return true;
}

void CSelectionTreeSignalsView::DeleteSignalItem( HTREEITEM hItem )
{
	HTREEITEM hSignal = GetParentItem( hItem );

	DeleteItem( hItem, m_TreeInfo.VarInfo );

	// if the signal var was last in signal group remove also the signal group
	if ( GetChildList(hSignal).size() == 0 )
	{
		DeleteItem( hSignal, m_TreeInfo.SignalInfo );
	}
}

void CSelectionTreeSignalsView::DeleteSignalItems( HTREEITEM hItem )
{
	SSignalInfo info = GetItemInfo(m_TreeInfo.SignalInfo, hItem);
	TChildItemList childs = GetChildList(hItem);
	for ( TChildItemList::const_iterator it = childs.begin(); it != childs.end(); ++it )
	{
		assert(it->Type == eIT_SignalVariable);
		DeleteSignalItem( it->Item );
	}
}

bool CSelectionTreeSignalsView::CreateRefItem( const char* name, HTREEITEM hItem )
{
	if ( IsRefValid( name ) )
	{
		string refstr;
		refstr.Format("Ref: %s", name);

		HTREEITEM hRef = InsertItem( hItem, TVI_FIRST, refstr.c_str(), m_TreeInfo.RefInfo, SRefInfo(name) );

		bool bRes = LoadRefSignals( name, hRef );
// 		if ( !bRes )
// 		{
// 			DeleteRefItem( hRef );
// 		}
// 		return bRes;
		if (!bRes)
		{
			refstr.Format("!MISSING Ref: %s", name);
			m_signals.SetItemText( hRef,  refstr.c_str() );
		}
		return true;
	}
	return false;
}

void CSelectionTreeSignalsView::DeleteRefItem( HTREEITEM hItem )
{
	TChildItemList childs = GetChildList(hItem);
	
	for ( TChildItemList::iterator it = childs.begin(); it != childs.end(); ++it )
	{
		assert(it->Type != eIT_SignalVariable);
		if ( it->Type == eIT_Ref )
			DeleteRefItem( it->Item );
		else if ( it->Type == eIT_Signal )
			DeleteSignalItems( it->Item );
	}

	DeleteItem( hItem, m_TreeInfo.RefInfo );
}

bool CSelectionTreeSignalsView::IsRefValid( const string& name )
{
	for ( TRefInfoMap::iterator it = m_TreeInfo.RefInfo.begin(); it != m_TreeInfo.RefInfo.end(); ++it )
	{
		if ( it->second.Name == name )
			return false;
	}
	return true;
}

void CSelectionTreeSignalsView::AddSignal( HTREEITEM hItem )
{
	CAddNewSignalDialog newDlg;
	newDlg.Init();
	if(newDlg.DoModal() == IDOK)
	{
		string signalName = newDlg.GetSignalName();
		string variableName = newDlg.GetVariableName();
		bool bVariableValue = newDlg.GetVariableValue();
		if ( variableName.size() > 0 && signalName.size() > 0)
		{
			CreateSignalItem( signalName, variableName, bVariableValue, TVI_ROOT, TVI_LAST );
			string desc;
			desc.Format( "Added Signal Variable \"%s\" to Signal \"%s\"", variableName, signalName );
			GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
		}
	}
}

void CSelectionTreeSignalsView::RemoveSignalVars( HTREEITEM hItem )
{
	SSignalInfo info = GetItemInfo(m_TreeInfo.SignalInfo, hItem);

	DeleteSignalItems( hItem );

	string desc;
	desc.Format( "All Signal Variables deleted for \"%s\"", info.Name );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
}

void CSelectionTreeSignalsView::AddSigVar( HTREEITEM hItem )
{
	const SSignalInfo& info = GetItemInfo(m_TreeInfo.SignalInfo, hItem);
	
	CAddNewSignalDialog newDlg;
	newDlg.Init( info.Name );
	if(newDlg.DoModal() == IDOK)
	{
		string signalName = newDlg.GetSignalName();
		string variableName = newDlg.GetVariableName();
		bool bVariableValue = newDlg.GetVariableValue();
		if ( variableName.length() > 0 && signalName.length() > 0 )
		{
			CreateSignalItem( signalName, variableName, bVariableValue, TVI_ROOT, TVI_LAST );
			string desc;
			desc.Format( "Added Signal Variable \"%s\" to Signal \"%s\"", variableName, signalName );
			GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
		}
	}
}

void CSelectionTreeSignalsView::DeleteSigVar( HTREEITEM hItem )
{
	SVarInfo info = GetItemInfo(m_TreeInfo.VarInfo, hItem);

	DeleteSignalItem( hItem );

	string desc;
	desc.Format( "Variable \"%s\" deleted from Signal \"%s\" ", info.Var, info.Signal );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
}

void CSelectionTreeSignalsView::ToggleValue( HTREEITEM hItem )
{
	SVarInfo& info = GetItemInfo(m_TreeInfo.VarInfo, hItem);
	info.Value = !info.Value;

	string name;
	name.Format("%s [%s]", info.Var, info.Value ? "true" : "false" );
	m_signals.SetItemText( hItem, name.c_str() );

	string desc;
	desc.Format( "Signal Var \"%s -> %s\" toggled to \"%s\" ", info.Signal, info.Var, info.Value ? "true" : "false" );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
}

void CSelectionTreeSignalsView::AddRef( HTREEITEM hItem )
{
	CSelectionTree_RefBrowser dlg;
	dlg.Init( eSTTI_Signals );
	if ( dlg.DoModal() == IDOK )
	{
		const char* refName = dlg.GetSelectedRef();
		AddNewRefEx(refName);
		return;
	}
}

void CSelectionTreeSignalsView::DeleteRef( HTREEITEM hItem )
{
	SRefInfo info = GetItemInfo(m_TreeInfo.RefInfo, hItem);

	DeleteRefItem( hItem );

	string desc;
	desc.Format( "Signal Reference \"%s\" removed", info.Name );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );

	SSelectionTreeBlockInfo refInfo;
	SSelectionTreeInfo group;
	if (GetIEditor()->GetSelectionTreeManager()->GetRefInfoByName( eSTTI_Signals, info.Name, refInfo, &group ))
		GetIEditor()->GetSelectionTreeManager()->GetModifier()->OnReferenceRemoved(group, eSTTI_Signals, refInfo.Name);
}

bool CSelectionTreeSignalsView::AddNewRefEx( const char* refName, bool promptError /*= true*/ )
{
	if ( refName && m_bLoaded )
	{
		if ( CreateRefItem( refName, TVI_ROOT ) )
		{
			string desc;
			desc.Format( "Signal Reference \"%s\" added ", refName );
			GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );


			SSelectionTreeBlockInfo refInfo;
			SSelectionTreeInfo group;
			bool ok = false;
			if (GetIEditor()->GetSelectionTreeManager()->GetRefInfoByName( eSTTI_Signals, refName, refInfo, &group ))
				ok = GetIEditor()->GetSelectionTreeManager()->GetModifier()->OnReferenceAdded(group, eSTTI_Signals, refInfo.Name);
			if (promptError && !ok)
			{
				CString error;
				error.Format( _T("Signal Reference \"%s\" was added but some depending References could not be added!\n"
					"This can happen if the SelectionTree/Reference Block already has a Reference\n"
					"that is also referenced by the the Reference that is needen for this Signal Reference!"), refName );
				CCustomMessageBox::Show( error,_T("Warning: Signal Reference was added but might use undefined Variables!"), "OK" );
			}
			return true;
		}
		if (promptError)
		{
			CString error;
			error.Format( _T("Signal Reference \"%s\" is already referenced or has reference to this or referenced block!" ), refName );
			CCustomMessageBox::Show( error,_T("Error: Faild to add Signal Reference!"), "OK" );
		}
	}
	return false;
}
