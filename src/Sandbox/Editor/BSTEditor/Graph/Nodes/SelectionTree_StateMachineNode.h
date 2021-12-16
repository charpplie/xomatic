////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013.
// -------------------------------------------------------------------------
//  File name:   SelectionTree_StatMachineNode.h
//  Version:     v1.00
//  Created:     20/09/2013 by Michiel Meesters
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE_STATEMACHINE_NODE_H_
#define __SELECTION_TREE_STATEMACHINE_NODE_H_

#define INVALID_HELPER_NODE "__HELPER_NODE_INVALIDE__"

#include "SelectionTree_TreeNode.h"

class CSelectionTree_StateMachineNode
	: public CSelectionTree_TreeNode
{
public:

	CSelectionTree_StateMachineNode( );
	virtual ~CSelectionTree_StateMachineNode();

	// CHyperNode
	virtual CHyperNode* Clone();
	virtual void PopulateContextMenu( CMenu& menu, int baseCommandId );

	// CSelectionTree_TreeNode interface
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool OnContextMenuCommandEx( int nCmd, string& undoDesc );
	
	virtual bool ShouldAppendLoggingProps() {return false;}
private:
};


#endif