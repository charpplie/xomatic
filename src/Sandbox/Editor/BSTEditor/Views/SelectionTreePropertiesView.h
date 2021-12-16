////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeTimestampsView.h
//  Version:     v1.00
//  Created:     21/09/2013 by Michiel Meesters
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE_PROPERTIES_VIEW__H__
#define __SELECTION_TREE_PROPERTIES_VIEW__H__


#include "SelectionTreeBaseDockView.h"
#include "Util/IXmlHistoryManager.h"
#include "Controls\PropertyCtrl.h"
#include "..\Graph\Nodes\SelectionTree_BaseNode.h"

class CSelectionTreePropertiesView
	: public CSelectionTreeBaseDockView
	, public IXmlHistoryView
	, public IXmlUndoEventHandler
{
	DECLARE_DYNAMIC( CSelectionTreePropertiesView )

public:
	CSelectionTreePropertiesView();
	virtual ~CSelectionTreePropertiesView();

	// IXmlHistoryView
	virtual bool LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex );
	virtual void UnloadXml( int typeId ) {}

	// IXmlUndoEventHandler
	virtual bool SaveToXml( XmlNodeRef& xmlNode ) {return true;}
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode ) {return true;}
	virtual bool ReloadFromXml( const XmlNodeRef& xmlNode );

	void OnNodeClicked(CSelectionTree_BaseNode* pNode);
	void OnUpdateProperties( XmlNodeRef var );

protected:
	virtual BOOL OnInitDialog();

private:
	CPropertyCtrl m_propsCtrl;
	CSelectionTree_BaseNode* m_CurrentSelectedNode;
	bool m_bLoading;

};


#endif