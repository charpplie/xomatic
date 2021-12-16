////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIElementTools.cpp
//  Version:     v1.00
//  Created:     13/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "UIElementTools.h"

#include "UIManager.h"
#include "UIEditor.h"
#include "PanelSimpleTreeBrowser.h"


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CUIElementTool::CUIElementTool()
: m_pPanelProperies(NULL)
, m_pPanelConstraints(NULL)
, m_pPanelFlags(NULL)
{
}

//////////////////////////////////////////////////////////////////////////
CUIElementTool::~CUIElementTool()
{
	CloseTool();
}

//////////////////////////////////////////////////////////////////////////
void CUIElementTool::OpenTool()
{
	{
		CSimpleTreeBrowser* panel = new CSimpleTreeBrowser(300, IDB_TREE_VIEW);
		panel->Create( &m_elementsScanner, AfxGetMainWnd() );
		AddPanel("UIElements", panel);
		panel->SetOnDblClickCallback( functor(*this,&CUIElementTool::OnElementDblClicked) );
		panel->SetOnSelectCallback( functor(*this,&CUIElementTool::OnElementSelected) );
	}

	{
		m_pPanelProperies = new CUIElementPropertiesPanelProperties();
		m_pPanelConstraints = new CUIElementPropertiesPanelConstraints();
		m_pPanelFlags = new CUIElementPropertiesPanelFlags();
		AddPanel("Properties", m_pPanelProperies );
		AddPanel("Constraints", m_pPanelConstraints );
		AddPanel("Flags", m_pPanelFlags );
	}

	{
		CSimpleTreeBrowser* panel = new CSimpleTreeBrowser(300, IDB_TREE_VIEW);
		panel->Create( &m_fctScanner, AfxGetMainWnd() );
		panel->SetAutoResize(true, 150, 300);
		AddPanel("Functions", panel, false);
		m_browser[&m_fctScanner] = panel;
		panel->SetOnDblClickCallback( functor(*this,&CUIElementTool::OnElementFctCallback) );
	}

	{
		CSimpleTreeBrowser* panel = new CSimpleTreeBrowser(300, IDB_TREE_VIEW);
		panel->Create( &m_eventScanner, AfxGetMainWnd() );
		panel->SetAutoResize(true, 150, 300);
		AddPanel("Events", panel, false);
		m_browser[&m_eventScanner] = panel;
	}

	{
		CSimpleTreeBrowser* panel = new CSimpleTreeBrowser(300, IDB_TREE_VIEW);
		panel->Create( &m_varScanner, AfxGetMainWnd() );
		panel->SetAutoResize(true, 150, 300);
		AddPanel("Variables", panel, false);
		m_browser[&m_varScanner] = panel;
		panel->SetOnDblClickCallback( functor(*this,&CUIElementTool::OnElementVarCallback) );
	}

	{
		CSimpleTreeBrowser* panel = new CSimpleTreeBrowser(300, IDB_TREE_VIEW);
		panel->Create( &m_arrayScanner, AfxGetMainWnd() );
		panel->SetAutoResize(true, 150, 300);
		AddPanel("Arrays", panel, false);
		m_browser[&m_arrayScanner] = panel;
		panel->SetOnDblClickCallback( functor(*this,&CUIElementTool::OnElementArrayCallback) );
	}

	{
		CSimpleTreeBrowser* panel = new CSimpleTreeBrowser(300, IDB_TREE_VIEW);
		panel->Create( &m_mcScanner, AfxGetMainWnd() );
		panel->SetAutoResize(true, 150, 300);
		AddPanel("Movieclips", panel, false);
		m_browser[&m_mcScanner] = panel;
		panel->SetOnDblClickCallback( functor(*this,&CUIElementTool::OnElementMCCallback) );
	}

	AfxGetMainWnd()->SetFocus();
}

//////////////////////////////////////////////////////////////////////////
void CUIElementTool::CloseTool()
{
	ClearPanels();
	m_pPanelProperies = NULL;
	m_pPanelConstraints = NULL;
	m_pPanelFlags = NULL;
	m_browser.clear();
}

//////////////////////////////////////////////////////////////////////////
void CUIElementTool::Update()
{
	if (m_pPanelProperies) m_pPanelProperies->UpdateChanges();
	if (m_pPanelConstraints) m_pPanelConstraints->UpdateChanges();
	if (m_pPanelFlags) m_pPanelFlags->UpdateChanges();
}

//////////////////////////////////////////////////////////////////////////
CUIRollupControl* CUIElementTool::GetRollupControl()
{
	CUIEditor* pEditor = GetIEditor()->GetUIManager()->GetEditor();
	assert(pEditor);
	return pEditor ? pEditor->GetElementRollup() : NULL;
}

//////////////////////////////////////////////////////////////////////////
void CUIElementTool::OnElementSelected( SSimpleTreeBrowserItem* pItem )
{
	if (pItem && pItem->UserData)
	{
		IUIElement* pElement = (IUIElement*)pItem->UserData;

		if (m_pPanelProperies) m_pPanelProperies->SetElement( pElement );
		if (m_pPanelConstraints) m_pPanelConstraints->SetElement( pElement );
		if (m_pPanelFlags) m_pPanelFlags->SetElement( pElement );

		for (TRefreshMap::iterator it = m_browser.begin(); it != m_browser.end(); ++it)
		{
			it->first->SetElement( pElement );
			it->second->Refresh();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CUIElementTool::OnElementDblClicked( SSimpleTreeBrowserItem* pItem )
{
	if (pItem && pItem->UserData)
	{
		IUIElement* pElement = (IUIElement*)pItem->UserData;
		const bool wasVisible = pElement->IsVisible();

		const int count = gEnv->pFlashUI->GetUIElementCount();
		for (int i = 0; i < count; ++i)
		{
			IUIElement* pElement = gEnv->pFlashUI->GetUIElement(i);
			IUIElementIteratorPtr instances = pElement->GetInstances();
			while (IUIElement* pInstance = instances->Next())
			{
				pInstance->SetVisible(false);
			}
		}
		pElement->SetVisible(!wasVisible);
	}
}

//////////////////////////////////////////////////////////////////////////
void CUIElementTool::OnElementFctCallback( SSimpleTreeBrowserItem* pItem )
{
	if (pItem && pItem->UserData)
	{
		IUIElement* pElement = (IUIElement*)pItem->UserData;
		for (int i = 0; i < pElement->GetFunctionCount(); ++i)
		{
			const SUIEventDesc* evt = pElement->GetFunctionDesc(i);
			if (strcmp(evt->sDisplayName, pItem->DisplayName.GetString()) == 0)
			{
				GetIEditor()->GetUIManager()->GetEditor()->AddEventEmuPanelElement( pElement, *evt );
				return;
			}
		}
	}
	assert( false );
}

//////////////////////////////////////////////////////////////////////////
void CUIElementTool::OnElementVarCallback( SSimpleTreeBrowserItem* pItem )
{
	if (pItem && pItem->UserData)
	{
		IUIElement* pElement = (IUIElement*)pItem->UserData;
		for (int i = 0; i < pElement->GetVariableCount(); ++i)
		{
			const SUIParameterDesc* desc = pElement->GetVariableDesc(i);
			if (strcmp(desc->sDisplayName, pItem->DisplayName.GetString()) == 0)
			{
				GetIEditor()->GetUIManager()->GetEditor()->AddEventEmuPanelElementVar( pElement, *desc );
				return;
			}
		}
	}
	assert( false );
}

//////////////////////////////////////////////////////////////////////////
void CUIElementTool::OnElementArrayCallback( SSimpleTreeBrowserItem* pItem )
{
	if (pItem && pItem->UserData)
	{
		IUIElement* pElement = (IUIElement*)pItem->UserData;
		for (int i = 0; i < pElement->GetArrayCount(); ++i)
		{
			const SUIParameterDesc* desc = pElement->GetArrayDesc(i);
			if (strcmp(desc->sDisplayName, pItem->DisplayName.GetString()) == 0)
			{
				GetIEditor()->GetUIManager()->GetEditor()->AddEventEmuPanelElementArray( pElement, *desc );
				return;
			}
		}
	}
	assert( false );
}

//////////////////////////////////////////////////////////////////////////
void CUIElementTool::OnElementMCCallback( SSimpleTreeBrowserItem* pItem )
{
	if (pItem && pItem->UserData)
	{
		IUIElement* pElement = (IUIElement*)pItem->UserData;
		for (int i = 0; i < pElement->GetMovieClipCount(); ++i)
		{
			const SUIParameterDesc* desc = pElement->GetMovieClipDesc(i);
			if (strcmp(desc->sDisplayName, pItem->DisplayName.GetString()) == 0)
			{
				GetIEditor()->GetUIManager()->GetEditor()->AddEventEmuPanelElementMovieClip( pElement, *desc );
				return;
			}
		}
	}
	assert( false );
}
