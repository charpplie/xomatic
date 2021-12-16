////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEventSystemTools.cpp
//  Version:     v1.00
//  Created:     13/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "UIEventSystemTools.h"

#include "UIManager.h"
#include "UIEditor.h"
#include "PanelSimpleTreeBrowser.h"


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CUIEventSystemTool::CUIEventSystemTool(IUIEventSystem::EEventSystemType type)
: m_pEventBrowser(NULL)
, m_eventSystemScanner(type)
, m_bCanInvokeEvents(type == IUIEventSystem::eEST_SYSTEM_TO_UI)
{
}

//////////////////////////////////////////////////////////////////////////
CUIEventSystemTool::~CUIEventSystemTool()
{
	CloseTool();
}

//////////////////////////////////////////////////////////////////////////
void CUIEventSystemTool::OpenTool()
{
	{
		CSimpleTreeBrowser* panel = new CSimpleTreeBrowser(300, IDB_TREE_VIEW);
		panel->Create( &m_eventSystemScanner, AfxGetMainWnd() );
		AddPanel("UIEventSystems", panel);
		panel->SetOnSelectCallback( functor(*this,&CUIEventSystemTool::OnEventSystemSelected) );
	}
	{
		m_pEventBrowser = new CSimpleTreeBrowser(300, IDB_TREE_VIEW);
		m_pEventBrowser->Create( &m_eventsScanner, AfxGetMainWnd() );
		m_pEventBrowser->SetAutoResize(true, 150, 300);
		if (m_bCanInvokeEvents)
			m_pEventBrowser->SetOnDblClickCallback( functor(*this,&CUIEventSystemTool::OnEventDblClick) );
		AddPanel("Events", m_pEventBrowser);
	}

	AfxGetMainWnd()->SetFocus();
}

//////////////////////////////////////////////////////////////////////////
void CUIEventSystemTool::CloseTool()
{
	ClearPanels();
	m_pEventBrowser = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CUIEventSystemTool::Update()
{
}

//////////////////////////////////////////////////////////////////////////
CUIRollupControl* CUIEventSystemTool::GetRollupControl()
{
	CUIEditor* pEditor = GetIEditor()->GetUIManager()->GetEditor();
	assert(pEditor);
	return pEditor ? pEditor->GetElementRollup() : NULL;
}

//////////////////////////////////////////////////////////////////////////
void CUIEventSystemTool::OnEventSystemSelected( SSimpleTreeBrowserItem* pItem )
{
	if (pItem && pItem->UserData)
	{
		IUIEventSystem* pEventSystem = (IUIEventSystem*)pItem->UserData;
		m_eventsScanner.SetEventSystem(pEventSystem);
		if (m_pEventBrowser) m_pEventBrowser->Refresh();
	}
}

//////////////////////////////////////////////////////////////////////////
void CUIEventSystemTool::OnEventDblClick( SSimpleTreeBrowserItem* pItem )
{
	if (pItem && pItem->UserData)
	{
		IUIEventSystem* pEventSystem = (IUIEventSystem*)pItem->UserData;

		for (int i = 0; i < pEventSystem->GetEventCount(); ++i)
		{
			const SUIEventDesc* evt = pEventSystem->GetEventDesc(i);
			if (strcmp(evt->sDisplayName, pItem->DisplayName.GetString()) == 0)
			{
				GetIEditor()->GetUIManager()->GetEditor()->AddEventEmuPanelEventSystem( pEventSystem, *evt );
				return;
			}
		}
	}
	assert(false);
}
