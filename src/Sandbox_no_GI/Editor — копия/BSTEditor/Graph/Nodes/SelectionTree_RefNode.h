////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTree_RefNode.h
//  Version:     v1.00
//  Created:     20/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE__REF_NODE__H__
#define __SELECTION_TREE__REF_NODE__H__


#include "SelectionTree_BaseNode.h"


class CSelectionTree_RefNode
	: public CSelectionTree_BaseNode
{
public:
	CSelectionTree_RefNode();
	virtual ~CSelectionTree_RefNode();

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
	virtual void Unlock() {SetReadOnly( false );}

private:
	void ToggleReftree();
	bool CheckForLoops( const XmlNodeRef& xmlNode, const char* refName );

	void SetRefName( const char* refName );
	const char* GetRefName() const {return m_Name.c_str();}

private:
	bool m_bIsRefVisible;
	string m_Name;
};


#endif