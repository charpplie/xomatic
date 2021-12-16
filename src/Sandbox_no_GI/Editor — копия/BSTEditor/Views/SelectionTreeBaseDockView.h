////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeBaseDockView.h
//  Version:     v1.00
//  Created:     17/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE_BASE_VIEW__H__
#define __SELECTION_TREE_BASE_VIEW__H__

class CSelectionTreeBaseDockView
	: public CXTResizeDialog
{
	DECLARE_DYNCREATE( CSelectionTreeBaseDockView )

public:
	CSelectionTreeBaseDockView();
	virtual ~CSelectionTreeBaseDockView();

protected:
	virtual BOOL PreTranslateMessage( MSG* pMsg );

	virtual void OnOK() {}
	virtual void OnCancel() {}

};

#endif
