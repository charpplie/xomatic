////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   DockingBaseView.h
//  Version:     v1.00
//  Created:     22/09/2009 by Pau Novau
//  Description: Base class for use by the Lmg Editor views.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LMGEditor/DockingBaseView.h"

IMPLEMENT_DYNCREATE( CDockingBaseView, CXTResizeDialog )

CDockingBaseView::CDockingBaseView()
{

}

CDockingBaseView::~CDockingBaseView()
{

}


BOOL CDockingBaseView::PreTranslateMessage( MSG* pMsg )
{
	if ( GetOwner() != NULL )
	{
		if ( GetOwner()->PreTranslateMessage( pMsg ) )
		{
			return TRUE;
		}
	}

	return __super::PreTranslateMessage( pMsg );
}