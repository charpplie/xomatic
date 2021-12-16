////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013.
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FragmentSplitter.h"

#include "MannequinDialog.h"

IMPLEMENT_DYNAMIC(CFragmentSplitter, CClampedSplitterWnd)

BEGIN_MESSAGE_MAP(CFragmentSplitter, CClampedSplitterWnd)
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

void CFragmentSplitter::OnSetFocus(CWnd* pOldWnd)
{
	__super::OnSetFocus(pOldWnd);

	if (CMannequinDialog::GetCurrentInstance()->GetDockingPaneManager()->IsPaneSelected(CMannequinDialog::IDW_PREVIEWER_PANE) == false)
	{
		CMannequinDialog::GetCurrentInstance()->GetDockingPaneManager()->ShowPane(CMannequinDialog::IDW_FRAGMENT_EDITOR_PANE, FALSE);
	}
}