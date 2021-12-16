////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIActionTools.cpp
//  Version:     v1.00
//  Created:     13/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "UIActionTools.h"

#include "UIManager.h"
#include "UIEditor.h"
#include "PanelSimpleTreeBrowser.h"


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CUIActionTool::CUIActionTool()
: m_pPropertiesPanel(NULL)
{
}

//////////////////////////////////////////////////////////////////////////
CUIActionTool::~CUIActionTool()
{
	CloseTool();
}

//////////////////////////////////////////////////////////////////////////
void CUIActionTool::OpenTool()
{
	{
		CSimpleTreeBrowser* pPanel = new CSimpleTreeBrowser(300, IDB_TREE_VIEW);
		pPanel->Create( &m_ActionScanner, AfxGetMainWnd() );
		AddPanel("UIActions", pPanel);
		pPanel->SetOnSelectCallback( functor(*this,&CUIActionTool::OnActionSelected) );
		pPanel->SetOnDblClickCallback( functor(*this,&CUIActionTool::OnActionDblClick) );
	}
	{
		m_pPropertiesPanel = new CUIActionPropertiesPanel();
		AddPanel("Properties", m_pPropertiesPanel );
	}

	AfxGetMainWnd()->SetFocus();
}

//////////////////////////////////////////////////////////////////////////
void CUIActionTool::CloseTool()
{
	ClearPanels();
	m_pPropertiesPanel = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CUIActionTool::Update()
{
}

//////////////////////////////////////////////////////////////////////////
CUIRollupControl* CUIActionTool::GetRollupControl()
{
	CUIEditor* pEditor = GetIEditor()->GetUIManager()->GetEditor();
	assert(pEditor);
	return pEditor ? pEditor->GetElementRollup() : NULL;
}

//////////////////////////////////////////////////////////////////////////
void CUIActionTool::OnActionSelected( SSimpleTreeBrowserItem* pItem )
{
	if (pItem && pItem->UserData)
	{
		IUIAction* pAction = (IUIAction*)pItem->UserData;
		if (m_pPropertiesPanel) m_pPropertiesPanel->SetAction(pAction);
	}
}

//////////////////////////////////////////////////////////////////////////
void CUIActionTool::OnActionDblClick( SSimpleTreeBrowserItem* pItem )
{
	if (pItem && pItem->UserData)
	{
		IUIAction* pAction = (IUIAction*)pItem->UserData;
		GetIEditor()->GetUIManager()->GetEditor()->AddEventEmuPanelAction( pAction );
	}
}