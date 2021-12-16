////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UITools.cpp
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UITools.h"

#include "UIElementTools.h"
#include "UIActionTools.h"
#include "UIEventSystemTools.h"

#include "UIManager.h"
#include "UIEditor.h"

const char* TOOL_NAME			= "UITools.UIMainTools";
const char* TOOL_DATAKEY  = "UIToolKey";
const char* TOOL_CATEGORY = "UITool";

const char* CATEGORY_ELEMENTS      = "Elements";
const char* CATEGORY_ACTIONS       = "Actions";
const char* CATEGORY_EVENTS_TOUI   = "Sys->UI";
const char* CATEGORY_EVENTS_FROMUI = "UI->Sys";

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CUIToolButtons::CUIToolButtons()
{
	std::vector<const char*> categories;
	categories.push_back(CATEGORY_ELEMENTS);
	categories.push_back(CATEGORY_ACTIONS);
	categories.push_back(CATEGORY_EVENTS_TOUI);
	categories.push_back(CATEGORY_EVENTS_FROMUI);

	IClassDesc *pClass = GetIEditor()->GetClassFactory()->FindClass( TOOL_NAME );
	if (!pClass) return;

	for (int i = 0; i < categories.size(); i++)
	{
		const char* category = categories[i];
		SButtonInfo bi;
		bi.toolClassName = TOOL_NAME;
		bi.name = category;
		bi.toolUserDataKey = TOOL_DATAKEY;
		bi.toolUserData = category;
		bi.pToolClass = pClass->GetRuntimeClass();
		AddButton(bi);
	}
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CUIBaseTool::AddPanel( const char* category, CDialog* panel, bool bExpanded /*= true*/ )
{
	CUIRollupControl* pControl = GetRollupControl();
	assert(pControl);
	if (pControl)
	{
		int id = pControl->AddPage(category, panel, bExpanded );
		assert(id != -1);
		if (id != -1)
		{
			m_panels[id] = panel;
		}
		else
		{
			delete panel;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CUIBaseTool::ClearPanels()
{
	CUIRollupControl* pControl = GetRollupControl();
	assert(pControl);
	if (pControl)
	{
		for (TPanelMap::iterator it = m_panels.begin(); it != m_panels.end(); ++it)
			pControl->RemovePage(it->first);
	}
	m_panels.clear();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CUITool,CEditTool)

//////////////////////////////////////////////////////////////////////////
CUITool::CUITool()
{
	// {4825CAEB-F944-4FBF-878A-61A4004D265C}
	static const GUID guid = 	{ 0x4825CAEB, 0xF944, 0x4FBF, { 0x87, 0x8A, 0x61, 0xA4, 0x00, 0x4D, 0x26, 0x5C } };
	m_pClassDesc = GetIEditor()->GetClassFactory()->FindClass(guid);

	SetStatusText( "" );

	m_Tools[CATEGORY_ELEMENTS] = new CUIElementTool;
	m_Tools[CATEGORY_ACTIONS] = new CUIActionTool;
	m_Tools[CATEGORY_EVENTS_TOUI] = new CUIEventSystemTool(IUIEventSystem::eEST_SYSTEM_TO_UI);
	m_Tools[CATEGORY_EVENTS_FROMUI] = new CUIEventSystemTool(IUIEventSystem::eEST_UI_TO_SYSTEM);

	if (GetIEditor()->GetUIManager()->GetEditor())
		GetIEditor()->GetUIManager()->GetEditor()->SetMainTool(this);
}

//////////////////////////////////////////////////////////////////////////
CUITool::~CUITool()
{
	if (GetIEditor()->GetUIManager()->GetEditor() && GetIEditor()->GetUIManager()->GetEditor()->GetMainTool() == this)
		GetIEditor()->GetUIManager()->GetEditor()->SetMainTool(NULL);

	CloseTools();
	for (TToolMap::iterator it = m_Tools.begin(); it != m_Tools.end(); ++it)
		delete it->second;
}

//////////////////////////////////////////////////////////////////////////
void CUITool::CloseTools()
{
	for (TToolMap::iterator it = m_Tools.begin(); it != m_Tools.end(); ++it)
		it->second->CloseTool();
}

//////////////////////////////////////////////////////////////////////////
void CUITool::UpdateTools()
{
	for (TToolMap::iterator it = m_Tools.begin(); it != m_Tools.end(); ++it)
		it->second->Update();
}

//////////////////////////////////////////////////////////////////////////
void CUITool::SelectCategory( const CString &category )
{
	CloseTools();

	assert(m_Tools[category]);
	m_Tools[category]->OpenTool();
}

//////////////////////////////////////////////////////////////////////////
void CUITool::BeginEditParams( IEditor *ie,int flags )
{
}

//////////////////////////////////////////////////////////////////////////
void CUITool::EndEditParams()
{
	CloseTools();
}

//////////////////////////////////////////////////////////////////////////
bool CUITool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{ 
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CUITool::OnSetCursor( CViewport *vp )
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CUITool::SetUserData( const char *key,void *userData )
{
	if (0 == strcmp(key, TOOL_DATAKEY))
	{
		SelectCategory((const char*)userData);
	}
}

//////////////////////////////////////////////////////////////////////////
// Class description.
//////////////////////////////////////////////////////////////////////////
class CUITool_ClassDesc : public CRefCountClassDesc
{
	//! This method returns an Editor defined GUID describing the class this plugin class is associated with.
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_UITOOLS; }

	//! Return the GUID of the class created by plugin.
	virtual REFGUID ClassID() 
	{
		// {51B82330-6E95-4CD0-888F-3BBCA6ACBE4B}
		static const GUID guid = 	{ 0x51B82330, 0x6E95, 0x4CD0, { 0x88, 0x8F, 0x3B, 0xBC, 0xA6, 0xAC, 0xBE, 0x4B } };
		return guid;
	}

	//! This method returns the human readable name of the class.
	virtual const char* ClassName() { return TOOL_NAME; };

	//! This method returns Category of this class, Category is specifing where this plugin class fits best in
	//! create panel.
	virtual const char* Category() { return TOOL_CATEGORY; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CUITool); }
	//////////////////////////////////////////////////////////////////////////
};

REGISTER_CLASS_DESC(CUITool_ClassDesc)
