////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeHistoryView.h
//  Version:     v1.00
//  Created:     28/03/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------  
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE_HISTORY_VIEW__H__
#define __SELECTION_TREE_HISTORY_VIEW__H__


#include "SelectionTreeBaseDockView.h"
#include "Util/IXmlHistoryManager.h"

class CSelectionTreeHistoryView
	: public CSelectionTreeBaseDockView
	, public IXmlHistoryEventListener
{
	DECLARE_DYNAMIC( CSelectionTreeHistoryView )

public:
	CSelectionTreeHistoryView();
	virtual ~CSelectionTreeHistoryView();

	void LoadHistory();

	// ISelectionTreeUndoEventListener
	virtual void OnEvent( EHistoryEventType event, void* pData = NULL );

protected:
	virtual BOOL OnInitDialog();

	afx_msg void OnTvnClick( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()

private:
	void ChangeHistory( const HTREEITEM clickedItemHandle );

private:
	CTreeCtrl m_history;

	typedef std::map< int, HTREEITEM > THistoryMap;
	THistoryMap m_HistoryMap;
	bool m_bNeedReload;
};


#endif