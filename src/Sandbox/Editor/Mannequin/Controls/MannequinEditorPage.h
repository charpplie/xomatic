////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   MannequinEditorPage.h
//  Version:     v1.00
//  Created:     2014-02-07 by Timothy Brookes.
//  Description: A parent class for mannequin editor pages
// 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __MannequinEditorPage_h__
#define __MannequinEditorPage_h__
#pragma once

#include "../ToolbarDialog.h"

class CMannequinModelViewport;
class CMannDopeSheet;
class CMannNodesCtrl;

class CMannequinEditorPage : public CToolbarDialog
{
	DECLARE_DYNAMIC(CMannequinEditorPage)

public:
	CMannequinEditorPage(UINT nIDTemplate, CWnd* pParentWnd = NULL);
	virtual ~CMannequinEditorPage();

	virtual CMannequinModelViewport* ModelViewport() const { return NULL; }
	virtual CMannDopeSheet* TrackPanel() { return NULL; }
	virtual CMannNodesCtrl* Nodes() { return NULL; }

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void ValidateToolbarButtonsState() {};

private:
};

#endif // __MannequinEditorPage_h__
