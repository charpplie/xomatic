////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTree_TreeNode.cpp
//  Version:     v1.00
//  Created:     20/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SelectionTree_TreeNode.h"

#include "BSTEditor/SelectionTreeManager.h"
#include "BSTEditor/Graph/SelectionTreeGraph.h"
#include "BSTEditor/Dialogs/EditBehaviorDialog.h"
#include "BSTEditor/Dialogs/DialogCommon.h"

enum CSelectionTree_TreeNode_ContextCommands
{
	eSTTNCM_ChangeToLeaf = CSelectionTree_TreeNode::eSTNST_Leaf,
	eSTTNCM_ChangeToPriority,
	eSTTNCM_ChangeToStateMachine,
	eSTTNCM_ChangeToSequence,
	eSTTNCM_Edit,
	eSTTNCM_MakeRef,
	eSTTNCM_EditLeafTranslation,
};


CSelectionTree_TreeNode::CSelectionTree_TreeNode()
: CSelectionTree_BaseNode( eSTNT_TreeNode )
, m_nodeSubType( eSTNST_Invalid )
, m_TranslatedName("")
, m_TranslationId(-1)
{
	SetClass( "TreeNode" );

	CreateDefaultInPort( ePCT_SingleConnectionAllowed );
	CreateDefaultOutPort( ePCT_MultiConnectionAllowed );
}

CSelectionTree_TreeNode::~CSelectionTree_TreeNode()
{
}

void CSelectionTree_TreeNode::Serialize( XmlNodeRef &node,bool bLoading,CObjectArchive* ar )
{
	CHyperNode::Serialize( node, bLoading, ar );
	if ( bLoading )
	{
		int subType;
		if ( node->getAttr( "BSTNodeType", subType ) )
		{
			m_nodeSubType = (ESelectionTreeNodeSubType) subType;
		}
		UpdateTranslations();
	}
	else
	{
		node->setAttr("BSTNodeType", (int) m_nodeSubType );
	}
}

void CSelectionTree_TreeNode::SetNodeSubType( ESelectionTreeNodeSubType type )
{
	m_nodeSubType = type;
}

CHyperNode* CSelectionTree_TreeNode::Clone()
{
	CSelectionTree_TreeNode* pNode = new CSelectionTree_TreeNode();

	pNode->CopyFrom( *this );

	return pNode;
}

void CSelectionTree_TreeNode::PopulateContextMenu( CMenu& menu, int baseCommandId )
{
	if ( m_nodeSubType == eSTNST_Leaf )
	{
		SSelectionTreeInfo info;
		if (GetIEditor()->GetSelectionTreeManager()->GetCurrentTreeInfo(info) && info.IsTree)
		{
			menu.AppendMenu( MF_STRING, baseCommandId + eSTTNCM_EditLeafTranslation, IsTranslated() ? "Modify Behavior Mapping" : "Create  Behavior Mapping");
			menu.AppendMenu( MF_SEPARATOR );
		}
	}

	if ( !IsReadOnly() )
	{
		static CMenu subMenu;
		static bool bInit = false;
		if ( bInit )
		{
			subMenu.DestroyMenu();
		}
		bInit = true;
		subMenu.CreatePopupMenu();
// 		if ( m_nodeSubType != eSTNST_Leaf )
// 			subMenu.AppendMenu( MF_STRING, baseCommandId + eSTTNCM_ChangeToLeaf, "Leaf" );
		if ( m_nodeSubType != eSTNST_Priority )
			subMenu.AppendMenu( MF_STRING, baseCommandId + eSTTNCM_ChangeToPriority, "Priority" );
		if ( m_nodeSubType != eSTNST_Sequence )
			subMenu.AppendMenu( MF_STRING, baseCommandId + eSTTNCM_ChangeToSequence, "Sequence" );
		if ( m_nodeSubType != eSTNST_StateMachine )
			subMenu.AppendMenu( MF_STRING, baseCommandId + eSTTNCM_ChangeToStateMachine, "StateMachine" );

		if ( m_nodeSubType != eSTNST_Leaf)
			menu.AppendMenu( MF_POPUP, reinterpret_cast< UINT_PTR >( subMenu.GetSafeHmenu() ), "Change Type" );
		menu.AppendMenu( MF_STRING, baseCommandId + eSTTNCM_Edit, m_nodeSubType == eSTNST_Leaf ? "Choose Behavior" : "Edit Name" );
// 		if (  m_nodeSubType != eSTNST_Leaf )
// 			menu.AppendMenu( MF_STRING, baseCommandId + eSTTNCM_MakeRef, "Make Reference" );
		menu.AppendMenu( MF_SEPARATOR );
	}
}

bool CSelectionTree_TreeNode::OnContextMenuCommandEx( int nCmd, string& undoDesc )
{
	if ( nCmd == eSTTNCM_Edit )
	{
		string oldname = GetName();
		bool res = DefaultEditDialog( true );
		undoDesc.Format( "Renamed Node (from %s to %s)", oldname.c_str(), GetName() );
		return res && oldname != GetName();
	}

	if ( nCmd >= eSTTNCM_ChangeToLeaf && nCmd <= eSTTNCM_ChangeToSequence )
	{
		ESelectionTreeNodeSubType oldType = m_nodeSubType;
		m_nodeSubType = (ESelectionTreeNodeSubType) nCmd;
		Invalidate(true);
		if ( m_nodeSubType == eSTNST_Leaf )
			GetTreeGraph()->DisconnectAllChilds( this );
		undoDesc.Format("Changed Node Type from %s to %s for Node %s", GetSubTypeStr( oldType ), GetSubTypeStr( m_nodeSubType ), GetName() );
		return true;
	}

	if ( nCmd == eSTTNCM_EditLeafTranslation)
	{
		GetTreeGraph()->StartMappingEditMode(this);
	}
	return false;
}

bool CSelectionTree_TreeNode::HasCondition() const
{
	if ( !IsRoot() )
	{
		return GetParentNode()->GetNodeType() == eSTNT_Condition;
	}
	return false;
}

bool CSelectionTree_TreeNode::LoadFromXml( const XmlNodeRef& xmlNode )
{
	const char* name = xmlNode->getAttr( "name" );
	ESelectionTreeNodeSubType subType = GetSubTypeEnum( xmlNode->getTag() );
	if ( name && name[0] && subType != eSTNST_Invalid )
	{
		SetName( name );
		SetNodeSubType( subType );
		Invalidate( true );
		return LoadChildrenFromXml( xmlNode );
	}
	return false;
}

bool CSelectionTree_TreeNode::SaveToXml( XmlNodeRef& xmlNode )
{
	string name = GetName();
	if ( name.length() > 0 )
	{
		xmlNode->setTag( GetSubTypeStr( GetNodeSubType() ) );
		xmlNode->setAttr( "name", name );
		return SaveChildrenToXml( xmlNode );
	}
	return false;
}

bool CSelectionTree_TreeNode::AcceptChild( CSelectionTree_BaseNode* pChild )
{
	if ( IsReadOnly() || pChild->IsReadOnly() )
	{
		return false;
	}
	return m_nodeSubType != eSTNST_Leaf;
}

CSelectionTree_TreeNode::ESelectionTreeNodeSubType CSelectionTree_TreeNode::GetSubTypeEnum( const char* typeStr )
{
	if ( !strcmpi( typeStr, "Leaf") ) return eSTNST_Leaf;
	if ( !strcmpi( typeStr, "Priority") ) return eSTNST_Priority;
	if ( !strcmpi( typeStr, "Sequence") ) return eSTNST_Sequence;
	if ( !strcmpi( typeStr, "StateMachine") ) return eSTNST_StateMachine;
	return eSTNST_Invalid;
}
const char* CSelectionTree_TreeNode::GetSubTypeStr( ESelectionTreeNodeSubType subType )
{
	switch ( subType )
	{
	case eSTNST_Leaf: return "Leaf";
	case eSTNST_Priority: return "Priority";
	case eSTNST_Sequence: return "Sequence";
	case eSTNST_StateMachine: return "StateMachine";
	}
	return "Invalid";
}

bool CSelectionTree_TreeNode::DefaultEditDialog( bool bCreate, bool* pHandled, string* pUndoDesc )
{
	if (pHandled) *pHandled = true;
	if ( IsReadOnly() )
	{
		return false;
	}

	if ( m_nodeSubType == eSTNST_Leaf )
	{
		CEditBehaviorDialog newDlg;
		newDlg.Init( "Select Behavior", GetName() );

		if(newDlg.DoModal() == IDOK)
		{
			string newBehavior = newDlg.GetBehaviorName();
			if (newBehavior != GetName())
			{
				if (pUndoDesc) pUndoDesc->Format("Changed node Behavior %s to %s",  GetName(), newBehavior.c_str() );
				SetName( newBehavior.c_str() );
				return true;
			}
		}
		return false;
	}

	CBSTStringDlg dlg ( _T("Edit node name") );
	dlg.SetString( GetName() );
	if ( dlg.DoModal() == IDOK )
	{
		string newName = dlg.GetString().GetString();
		if (newName.length() > 0 && (bCreate || newName != GetName()) )
		{
			if (pUndoDesc) pUndoDesc->Format("Renamed node %s to %s", GetName(), newName.c_str());
			SetName( newName.c_str() );
			return true;
		}
	}
	return false;
}

void CSelectionTree_TreeNode::UpdateTranslations()
{
	m_TranslatedName = "";
	m_TranslationId = -1;
	if (m_nodeSubType == eSTNST_Leaf )
	{
		GetTreeGraph()->CheckNodeMapping(this, m_TranslatedName, m_TranslationId);
		Invalidate(true);
	}
	else
	{
		CSelectionTree_BaseNode::UpdateTranslations();
	}
}
