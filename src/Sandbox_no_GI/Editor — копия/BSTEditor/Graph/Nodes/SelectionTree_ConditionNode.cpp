////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   SelectionTree_ConditionNode.cpp
//  Version:     v1.00
//  Created:     11/1/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SelectionTree_ConditionNode.h"

#include "BSTEditor/Graph/SelectionTreeGraph.h"

#include "BSTEditor/Dialogs/DialogCommon.h"

enum SelectionTree_ConditionNode_ContextCommands
{
	eSTCNCM_Edit = 0,
	eSTCNCM_Remove
};

CSelectionTree_ConditionNode::CSelectionTree_ConditionNode()
: CSelectionTree_BaseNode( eSTNT_Condition )
{
	SetClass( "Condition" );

	CreateDefaultInPort( ePCT_SingleConnectionAllowed );
	CreateDefaultOutPort( ePCT_SingleConnectionAllowed );
}


CSelectionTree_ConditionNode::~CSelectionTree_ConditionNode()
{
}

CHyperNode* CSelectionTree_ConditionNode::Clone()
{
	CSelectionTree_ConditionNode* pNode = new CSelectionTree_ConditionNode();

	pNode->CopyFrom( *this );
	pNode->SetConditionText( GetConditionText() );

	return pNode;
}

void CSelectionTree_ConditionNode::Serialize( XmlNodeRef &node,bool bLoading,CObjectArchive* ar )
{
	CHyperNode::Serialize( node, bLoading, ar );
	if ( bLoading )
	{
		const char* conditon = node->getAttr( "BSTConditionString" );
		if ( conditon  )
		{
			SetConditionText( conditon );
		}
	}
	else
	{
		node->setAttr("BSTConditionString", GetConditionText().c_str() );
	}
}

void CSelectionTree_ConditionNode::PopulateContextMenu( CMenu& menu, int baseCommandId )
{
	if ( !IsReadOnly() )
	{
		menu.AppendMenu( MF_STRING, baseCommandId + eSTCNCM_Edit, "Edit Condition" );
		menu.AppendMenu( MF_STRING, baseCommandId + eSTCNCM_Remove, "Remove Condition" );
		menu.AppendMenu( MF_SEPARATOR );
	}
}

bool CSelectionTree_ConditionNode::OnContextMenuCommandEx( int nCmd, string& undoDesc )
{
	if ( nCmd == eSTCNCM_Edit )
	{
		return DefaultEditDialog( true );
	}

	if ( nCmd == eSTCNCM_Remove )
	{
		CSelectionTree_BaseNode* pChild = GetChildNode( 0 );
		if ( pChild && GetParentNode() )
		{
			GetTreeGraph()->ConnectNodes( GetParentNode(), pChild );
		}
		GetTreeGraph()->RemoveNode( this );
		return true;
	}
	return false;
}

bool CSelectionTree_ConditionNode::LoadFromXml( const XmlNodeRef& xmlNode )
{
	const char* condition = xmlNode->getAttr( "condition" );
	if ( condition && condition[0] )
	{
		SetConditionText( condition );
		CSelectionTree_BaseNode* pTreeNode = GetTreeGraph()->CreateNewNode( xmlNode, false );
		if ( pTreeNode )
		{
			GetTreeGraph()->ConnectNodes( this, pTreeNode );
			return true;
		}
		GetTreeGraph()->RemoveNode( pTreeNode );
	}
	return false;
}

bool CSelectionTree_ConditionNode::SaveToXml( XmlNodeRef& xmlNode )
{
	string condition = GetConditionText();
	if ( condition.length() > 0 && GetChildNodeCount() == 1 )
	{
		if ( GetChildNode( 0 )->SaveToXml( xmlNode ) )
		{
			xmlNode->setAttr( "condition", condition );
			return true;
		}
	}
	return false;
}

bool CSelectionTree_ConditionNode::AcceptChild( CSelectionTree_BaseNode* pChild )
{
	if ( IsReadOnly() || pChild->IsReadOnly() )
	{
		return false;
	}

	return pChild->GetNodeType() == CSelectionTree_BaseNode::eSTNT_TreeNode;
}

bool CSelectionTree_ConditionNode::DefaultEditDialog( bool bCreate, bool* pHandled, string* pUndoDesc )
{
	if (pHandled) *pHandled = true;

	if ( IsReadOnly() )
		return false;

	CBSTStringDlg dlg ( _T("Edit condition string") );
	dlg.SetString( GetConditionText().c_str() );
	if ( dlg.DoModal() == IDOK )
	{
		string newCondition = dlg.GetString().GetString();
		if (newCondition != GetConditionText())
		{
			if (pUndoDesc) pUndoDesc->Format("Condition node modified");
			SetConditionText( newCondition );
			return true;
		}
	}

	return false;
}