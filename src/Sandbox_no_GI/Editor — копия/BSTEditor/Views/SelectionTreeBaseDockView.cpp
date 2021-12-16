////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeBaseDockView.cpp
//  Version:     v1.00
//  Created:     17/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SelectionTreeBaseDockView.h"

IMPLEMENT_DYNCREATE( CSelectionTreeBaseDockView, CXTResizeDialog )

CSelectionTreeBaseDockView::CSelectionTreeBaseDockView()
{

}

CSelectionTreeBaseDockView::~CSelectionTreeBaseDockView()
{

}

BOOL CSelectionTreeBaseDockView::PreTranslateMessage( MSG* pMsg )
{
	return __super::PreTranslateMessage( pMsg );
}