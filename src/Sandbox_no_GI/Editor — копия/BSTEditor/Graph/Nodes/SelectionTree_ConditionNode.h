////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   SelectionTree_ConditionNode.h
//  Version:     v1.00
//  Created:     11/1/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE__CONDITION_NODE__H__
#define __SELECTION_TREE__CONDITION_NODE__H__


#include "SelectionTree_BaseNode.h"


class CSelectionTree_ConditionNode
	: public CSelectionTree_BaseNode
{
public:
	CSelectionTree_ConditionNode();
	virtual ~CSelectionTree_ConditionNode();

	// CHyperNode
	virtual CHyperNode* Clone();
	virtual void PopulateContextMenu( CMenu& menu, int baseCommandId );
	virtual void Serialize( XmlNodeRef &node,bool bLoading,CObjectArchive* ar=0 );

	// CSelectionTree_BaseNode interface
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool AcceptChild( CSelectionTree_BaseNode* pChild );
	virtual bool DefaultEditDialog( bool bCreate = false, bool* pHandled = NULL, string* pUndoDesc = NULL );
	virtual bool OnContextMenuCommandEx( int nCmd, string& undoDesc );

	// CSelectionTree_ConditionNode
	const string& GetConditionText() const { return m_conditionText; }
	void SetConditionText( const string& condition ) { m_conditionText = condition; SetName( condition ); }

private:
	string m_conditionText;
};

#endif