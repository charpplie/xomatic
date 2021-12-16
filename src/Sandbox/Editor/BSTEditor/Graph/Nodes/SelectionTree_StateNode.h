////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013.
// -------------------------------------------------------------------------
//  File name:   SelectionTree_StateNode.h
//  Version:     v1.00
//  Created:     20/09/2013 by Michiel Meesters
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE_STATENODE_H_
#define __SELECTION_TREE_STATENODE_H_

#define INVALID_HELPER_NODE "__HELPER_NODE_INVALIDE__"

#include "SelectionTree_TreeNode.h"

class CSelectionTree_StateNode
	: public CSelectionTree_TreeNode
{
public:
	struct Transition
	{
		Transition(string toState, string onEvent)
		{
			m_toState = toState;
			m_onEvent = onEvent;
		}
		Transition()
		{
			m_toState = "";
			m_onEvent = "";
		}
		string m_toState;
		string m_onEvent;
	};

	enum ESelectionTreeNodeSubType
	{
		eSTNST_Invalid = -1,
		eSTNST_Leaf = 0,
		eSTNST_Priority,
		eSTNST_StateMachine,
		eSTNST_Sequence,
		eSTNST_COUNT
	};

	CSelectionTree_StateNode( );
	virtual ~CSelectionTree_StateNode();

	// CHyperNode
	virtual CHyperNode* Clone();
	virtual void PopulateContextMenu( CMenu& menu, int baseCommandId );
	virtual void Serialize( XmlNodeRef &node,bool bLoading,CObjectArchive* ar=0 );
	
	// CSelectionTree_BaseNode interface
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool DefaultEditDialog( bool bCreate = false, bool* pHandled = NULL, string* pUndoDesc = NULL );
	virtual bool OnContextMenuCommandEx( int nCmd, string& undoDesc );
	
	// CSelectionTree_TreeNode
	virtual void PropertyChanged(XmlNodeRef var);
	virtual void FillPropertyTable(XmlNodeRef var);
	virtual bool ShouldAppendLoggingProps() { return false; }

protected:
	bool LoadChildrenFromXml( const XmlNodeRef& xmlNode );
	void LoadTransitions(XmlNodeRef transitionsNode);

private:
	std::map<string, Transition> m_Transitions;
};


#endif