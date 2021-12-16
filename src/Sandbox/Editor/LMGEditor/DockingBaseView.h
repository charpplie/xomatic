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

#ifndef __Docking_Base_View__h__
#define __Docking_Base_View__h__
#pragma once

class CDockingBaseView
	: public CXTResizeDialog
{
DECLARE_DYNCREATE( CDockingBaseView )

public:
	CDockingBaseView();
	virtual ~CDockingBaseView();

protected:
	virtual BOOL PreTranslateMessage( MSG* pMsg );

};

#endif