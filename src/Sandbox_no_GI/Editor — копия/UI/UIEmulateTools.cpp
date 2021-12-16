////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEmulateTools.cpp
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UIEmulateTools.h"

#include "UIManager.h"
#include "UIEditor.h"

const char* BTN_CLEAR_ALL      = "Clear all";
const char* BTN_ADD_INVOKE     = "Add Invoke";
const char* BTN_ADD_FORITEM    = "Add Group";
const char* BTN_REMOVE_FORITEM = "Remove Group";

enum EButtonIdx
{
	EBI_BTN_CLEAR_ALL = 0,
	EBI_BTN_ADD_INVOKE,
	EBI_BTN_ADD_FORITEM,
	EBI_BTN_REMOVE_FORITEM,
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CUIEmulateToolButtons::CUIEmulateToolButtons()
{
	std::vector<const char*> categories;
	categories.push_back(BTN_CLEAR_ALL);
	categories.push_back(BTN_ADD_INVOKE);
	// TODO: enable those two buttons (need dialog to select UIElement or UIEventSystem
// 	categories.push_back(BTN_ADD_FORITEM);
// 	categories.push_back(BTN_REMOVE_FORITEM);

	for (int i = 0; i < categories.size(); i++)
	{
		SButtonInfo bi;
		bi.name = categories[i];
		AddButton(bi);
	}
}


////////////////////////////////////////////////////////////////////
BOOL CUIEmulateToolButtons::OnCommand(WPARAM wParam, LPARAM lParam)
{
	CUIEditor* pEditor = GetIEditor()->GetUIManager()->GetEditor();
	if (HIWORD(wParam) == BN_CLICKED && pEditor)
	{
		int ctrlId = LOWORD(wParam);
		switch (ctrlId)
		{
		case EBI_BTN_CLEAR_ALL:
			pEditor->ClearEventEmuPanels();
			break;
		case EBI_BTN_ADD_INVOKE:
			pEditor->AddEventEmuPanelInvoke();
			break;
		case EBI_BTN_ADD_FORITEM:
			break;
		case EBI_BTN_REMOVE_FORITEM:
			break;
		}

	}
	return TRUE;
}