////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEmulateTools.h
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#ifndef __UIEmulateTools_H__
#define __UIEmulateTools_H__

#include "Dialogs/ButtonsPanel.h"
#include "UIRollupView.h"


////////////////////////////////////////////////////////////////////
class CUIEmulateToolButtons : public CButtonsPanel
{
public:
	CUIEmulateToolButtons();

	enum { IDD = IDD_UITOOLS };

protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};

#endif // __UIEmulateTools_H__
