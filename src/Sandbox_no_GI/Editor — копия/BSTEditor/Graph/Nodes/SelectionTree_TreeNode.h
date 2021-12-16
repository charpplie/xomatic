////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   CSelectionTree_TreeNode.h
//  Version:     v1.00
//  Created:     20/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE__LEAF_NODE__H__
#define __SELECTION_TREE__LEAF_NODE__H__

#define INVALID_HELPER_NODE "__HELPER_NODE_INVALIDE__"

#include "SelectionTree_BaseNode.h"

class CSelectionTree_TreeNode
	: public CSelectionTree_BaseNode
{
public:
	enum ESelectionTreeNodeSubType
	{
		eSTNST_Invalid = -1,
		eSTNST_Leaf = 0,
		eSTNST_Priority,
		eSTNST_StateMachine,
		eSTNST_Sequence,
		eSTNST_COUNT
	};

	CSelectionTree_TreeNode( );
	virtual ~CSelectionTree_TreeNode();

	// CHyperNode
	virtual CHyperNode* Clone();
	virtual void PopulateContextMenu( CMenu& menu, int baseCommandId );
	virtual void Serialize( XmlNodeRef &node,bool bLoading,CObjectArchive* ar=0 );

	// CSelectionTree_BaseNode interface
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool AcceptChild( CSelectionTree_BaseNode* pChild );
	virtual bool HasCondition() const;
	virtual bool DefaultEditDialog( bool bCreate = false, bool* pHandled = NULL, string* pUndoDesc = NULL );
	virtual bool OnContextMenuCommandEx( int nCmd, string& undoDesc );
	virtual void UpdateTranslations();

	// CSelectionTree_TreeNode
	ESelectionTreeNodeSubType GetNodeSubType() const { return m_nodeSubType; }
	void SetNodeSubType( ESelectionTreeNodeSubType type );
	const char* GetTranslation() const { return m_TranslatedName.c_str(); }
	bool IsTranslated() const { return m_TranslationId != -1; }
	int GetTranslationId() const { return m_TranslationId; }

	static ESelectionTreeNodeSubType GetSubTypeEnum( const char* typeStr );
	static const char* GetSubTypeStr( ESelectionTreeNodeSubType subType );

private:
	ESelectionTreeNodeSubType m_nodeSubType;
	string m_TranslatedName;
	int m_TranslationId;
};


#endif