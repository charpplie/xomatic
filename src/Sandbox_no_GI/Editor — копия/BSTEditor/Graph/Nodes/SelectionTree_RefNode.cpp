////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTree_RefNode.cpp
//  Version:     v1.00
//  Created:     20/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SelectionTree_RefNode.h"
#include "BSTEditor/Graph/SelectionTreeGraph.h"
#include "BSTEditor/Dialogs/SelectionTree_RefBrowser.h"
#include "BSTEditor/SelectionTreeManager.h"
#include "BSTEditor/SelectionTreeModifier.h"


#include "CustomMessageBox.h"

enum CSelectionTree_RefNode_ContextCommands
{
	eSTRNCM_ToggleVisible = 0,
	eSTRNCM_DeRef,
};

CSelectionTree_RefNode::CSelectionTree_RefNode()
: CSelectionTree_BaseNode( eSTNT_Reference )
, m_bIsRefVisible( false )
{
	SetClass( "Reference" );

	CreateDefaultInPort( ePCT_SingleConnectionAllowed );
	CreateDefaultOutPort( ePCT_MultiConnectionAllowed );
}


CSelectionTree_RefNode::~CSelectionTree_RefNode()
{
	if ( !GetTreeGraph()->IsCreatingGraph() )
	{
		if (m_bIsRefVisible)
			GetTreeGraph()->HideRefSubTree( this );

		SSelectionTreeBlockInfo refInfo;
		SSelectionTreeInfo group;
		if ( GetIEditor()->GetSelectionTreeManager()->GetRefInfoByName( eSTTI_Tree, GetName(), refInfo, &group ) )
			GetIEditor()->GetSelectionTreeManager()->GetModifier()->OnReferenceRemoved(group, eSTTI_Tree, refInfo.Name);
	}
}

CHyperNode* CSelectionTree_RefNode::Clone()
{
	CSelectionTree_RefNode* pNode = new CSelectionTree_RefNode();

	pNode->CopyFrom( *this );

	return pNode;
}

void CSelectionTree_RefNode::PopulateContextMenu( CMenu& menu, int baseCommandId )
{
	menu.AppendMenu( MF_STRING, baseCommandId + eSTRNCM_ToggleVisible, m_bIsRefVisible ? "Hide Referenced Tree" : "Show Referenced Tree" );
	if (!GetParentNode()->IsReadOnly() && GetParentNode()->GetNodeType() != eSTNT_Reference)
		menu.AppendMenu( MF_STRING, baseCommandId + eSTRNCM_DeRef, "De-Reference" );
	menu.AppendMenu( MF_SEPARATOR );
}

bool CSelectionTree_RefNode::OnContextMenuCommandEx( int nCmd, string& undoDesc )
{
	if ( nCmd == eSTRNCM_ToggleVisible )
	{
		ToggleReftree();
		return false;
	}
	if ( nCmd == eSTRNCM_DeRef )
	{
		const bool isRefValid = m_bIsRefVisible || GetTreeGraph()->ShowRefSubTree( this );
		CSelectionTree_BaseNode* pRefRoot = GetChildNode( 0 );
 		if ( isRefValid && pRefRoot )
		{
			pRefRoot->SetTreeReadOnly( false );
			if ( GetParentNode() )
			{
				GetTreeGraph()->ConnectNodes( GetParentNode(), pRefRoot );
			}
			GetTreeGraph()->RemoveNode( this );
			return true;
		}
	}
	return false;
}

void CSelectionTree_RefNode::Serialize( XmlNodeRef &node,bool bLoading,CObjectArchive* ar )
{
	CHyperNode::Serialize( node, bLoading, ar );
	if ( bLoading )
	{
		const char* name = node->getAttr( "BSTRefName" );
		if ( name  )
		{
			SetRefName(name);
		}
	}
	else
	{
		node->setAttr("BSTRefName", GetRefName() );
	}
}


bool CSelectionTree_RefNode::LoadFromXml( const XmlNodeRef& xmlNode )
{
	const char* name = xmlNode->getAttr( "name" );
	if ( name && name[0] )
	{
		SetName( name );
		SetRefName( name );
		return true;
	}
	return false;
}

bool CSelectionTree_RefNode::SaveToXml( XmlNodeRef& xmlNode )
{
	const char* name = GetRefName();
	if ( strlen(name) > 0 )
	{
		xmlNode->setTag( "Ref" );
		xmlNode->setAttr( "name", name );
		return true;
	}
	return false;
}

bool CSelectionTree_RefNode::AcceptChild( CSelectionTree_BaseNode* pChild )
{
	return false;
}

bool CSelectionTree_RefNode::DefaultEditDialog( bool bCreate, bool* pHandled, string* pUndoDesc )
{
	if (pHandled) *pHandled = true;

	if ( !bCreate )
	{
		ToggleReftree();
		return false;
	}

	CSelectionTree_RefBrowser dlg;
	dlg.Init( eSTTI_Tree );
	if ( dlg.DoModal() == IDOK )
	{
		const char* refName = dlg.GetSelectedRef();
		if ( refName && GetName() != refName )
		{
			SSelectionTreeBlockInfo refInfo;
			SSelectionTreeInfo group;
			if ( GetIEditor()->GetSelectionTreeManager()->GetRefInfoByName( eSTTI_Tree, refName, refInfo, &group ) )
			{
				SSelectionTreeInfo currInfo;
				int index = 0;
				if (GetIEditor()->GetSelectionTreeManager()->GetCurrentTreeInfo(currInfo, &index))
				{
					SSelectionTreeBlockInfo currTreeInfo;
					if (currInfo.GetBlockById( currTreeInfo, eSTTI_Tree, index ))
					{
						if ( CheckForLoops( refInfo.XmlData, currTreeInfo.Name ) )
						{
							SetName( refName );
							SetRefName( refName );
							if (!GetIEditor()->GetSelectionTreeManager()->GetModifier()->OnReferenceAdded(group, eSTTI_Tree, refName))
							{
								CString error;
								error.Format( _T("Reference Node \"%s\" was added but some depending References could not be added!\n"
									"This can happen if the SelectionTree/Reference Block already has a Reference\n"
									"that is also referenced by the the Reference that is needen for the Reference Node!"), refName );
								CCustomMessageBox::Show( error,_T("Warning: Reference Node was added but might use undefined Variables!"), "OK" );
							}
							return true;
						}
					}
					CString error;
					error.Format( _T("Reference \"%s\" would cause a infinite loop!" ), refName );
					CCustomMessageBox::Show( error,_T("Error: Faild to add Reference!"), "OK" );
				}
			}
		}
	}

	return false;
}

void CSelectionTree_RefNode::ToggleReftree()
{
	if ( m_bIsRefVisible )
	{
		GetTreeGraph()->HideRefSubTree( this );
		m_bIsRefVisible = false;
	}
	else
	{
		m_bIsRefVisible = GetTreeGraph()->ShowRefSubTree( this );
	}
}

bool CSelectionTree_RefNode::CheckForLoops( const XmlNodeRef& xmlNode, const char* refName )
{
	CSelectionTreeManager* pMan = GetIEditor()->GetSelectionTreeManager();
	for ( int i = 0; i < xmlNode->getChildCount(); ++i )
	{
		const XmlNodeRef& child = xmlNode->getChild( i );
		if ( strcmpi( child->getTag(), "Ref" ) == 0 )
		{
			const char* name = child->getAttr( "name" );
			if( strcmpi( name, refName ) == 0 )
				return false;

			SSelectionTreeBlockInfo refInfo;
			bool ok = GetIEditor()->GetSelectionTreeManager()->GetRefInfoByName( eSTTI_Tree, name, refInfo );
			if ( ok && !CheckForLoops( refInfo.XmlData, refName ) )
				return false;
		}
		if ( !CheckForLoops( child, refName ) )
			return false;
	}
	return true;
}

void CSelectionTree_RefNode::SetRefName( const char* refName )
{
	m_Name = refName;
	SSelectionTreeBlockInfo refInfo;
	if (!GetIEditor()->GetSelectionTreeManager()->GetRefInfoByName( eSTTI_Tree, refName, refInfo ))
	{
		SetName( string().Format("!MISSING: %s", refName).c_str() );
	}
}