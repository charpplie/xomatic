////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeVariablesView.h
//  Version:     v1.00
//  Created:     22/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------  
//  History:
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE_VARIABLES_VIEW__H__
#define __SELECTION_TREE_VARIABLES_VIEW__H__


#include "SelectionTreeBaseDockView.h"
#include "Util/IXmlHistoryManager.h"

class CSelectionTreeVariablesView
	: public CSelectionTreeBaseDockView
	, public IXmlHistoryView
	, public IXmlUndoEventHandler
{
	DECLARE_DYNAMIC( CSelectionTreeVariablesView )

public:
	CSelectionTreeVariablesView();
	virtual ~CSelectionTreeVariablesView();

	// IXmlHistoryView
	virtual bool LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex );
	virtual void UnloadXml( int typeId );

	// IXmlUndoEventHandler
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool ReloadFromXml( const XmlNodeRef& xmlNode );

	bool ShouldAddRef( const char* refName ) { return IsRefValid(refName); }
	bool AddNewRefEx( const char* refName, bool promptError = true );

	void SetDebugging(bool debug);
	void SetVarVal(const char* varName, bool val);

protected:
	virtual BOOL OnInitDialog();

	afx_msg void OnTvnDblClick( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()

private:
	bool LoadVarsNode( const XmlNodeRef& xmlNode, HTREEITEM hItem );
	bool LoadRefVars( const char* refname, HTREEITEM hItem );

	bool CreateVarItem( const char* name, HTREEITEM hItem, HTREEITEM hInsertAfter );
	void DeleteVarItem( HTREEITEM hItem );

	bool CreateRefItem( const char* name, HTREEITEM hItem );
	void DeleteRefItem( HTREEITEM hItem );

	void AddVar( HTREEITEM hItem );
	void RenameVar( HTREEITEM hItem );
	void DeleteVar( HTREEITEM hItem );
	void AddRef( HTREEITEM hItem );

	void DeleteRef( HTREEITEM hItem );

	bool IsVarValid( const string& name );
	bool IsRefValid( const string& name );

	void ClearList();

private:
	CTreeCtrl m_variables;
	bool m_bLoaded;
	bool m_bDebugging;

	typedef std::list< HTREEITEM > TItemList;
	typedef std::map< HTREEITEM, TItemList > TItemMap;
	TItemMap m_VarMap;

	struct SItemInfo
	{
		SItemInfo( const string& name = "", bool isRef = false )
			: Name( name )
			, IsRef( isRef )
			, DebugVal(-1)
		{}

		string Name;
		bool IsRef;
		int DebugVal;
	};

	typedef std::map< HTREEITEM, SItemInfo > TItemInfoMap;
	TItemInfoMap m_ItemInfoMap;
};


#endif