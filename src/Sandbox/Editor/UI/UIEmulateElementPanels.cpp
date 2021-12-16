////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEmulateElementPanels.cpp
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "UIEmulateElementPanels.h"
#include <limits>

#define MAX_INSTANCE_ID  (std::numeric_limits<int>::max())

////////////////////////////////////////////////////////////////////
/////////////// Base emu panel for UIElement items /////////////////
////////////////////////////////////////////////////////////////////
CUIEmulateUIElementPanel::CUIEmulateUIElementPanel( IUIElement* pElement, const SUIParameterDesc& desc )
: CUIEmulatePanel("Set")
, m_pElement(pElement)
, m_Desc(desc)
, m_ParameterName(desc.sDisplayName)
{
	mv_InstanceId = 0;
	mv_InstanceId->SetLimits(0, MAX_INSTANCE_ID);
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIElementPanel::Refresh()
{
	if (m_pElement->IsValid())
	{
		for (int i = 0; i < GetDescCount(); ++i)
		{
			const SUIParameterDesc* pDesc = GetDescInfo(i);
			if (strcmp(pDesc->sDisplayName, GetParamName() ) == 0)
			{
				m_Desc = *pDesc;
				return;
			}
		}
	}
	RemoveThis();
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIElementPanel::InitVars()
{
	GetVarBlock()->AddVariable(mv_InstanceId, "Instance ID");
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIElementPanel::OnVarChange( IVariable *pVar )
{
}

////////////////////////////////////////////////////////////////////
IUIElement* CUIEmulateUIElementPanel::GetElement() const 
{
	return m_pElement ? m_pElement->GetInstance( mv_InstanceId ) : NULL;
}


////////////////////////////////////////////////////////////////////
////////////////// IUIElement var emu panel ////////////////////////
////////////////////////////////////////////////////////////////////
CUIEmulateUIElementVarPanel::CUIEmulateUIElementVarPanel( IUIElement* pElement, const SUIParameterDesc& desc )
: CUIEmulateUIElementPanel(pElement, desc)
{
	mv_Value = "";
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIElementVarPanel::CallEvent( const SUIArguments& args )
{
 	if (GetElement())
 	{
		CString val = mv_Value;
		TUIData value( string(val.GetString()) );
 		GetElement()->SetVariable( &GetDesc(), value );
 	}
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIElementVarPanel::InitVars()
{
	CUIEmulateUIElementPanel::InitVars();
 	GetVarBlock()->AddVariable( mv_Value, "Value" );
}


////////////////////////////////////////////////////////////////////
///////////////// IUIElement array emu panel ///////////////////////
////////////////////////////////////////////////////////////////////
CUIEmulateUIElementArrayPanel::CUIEmulateUIElementArrayPanel( IUIElement* pElement, const SUIParameterDesc& desc )
: CUIEmulateUIElementPanel(pElement, desc)
{
	mv_ArgCount = 0;
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIElementArrayPanel::CallEvent( const SUIArguments& args )
{
	if (GetElement())
	{
		GetElement()->SetArray( &GetDesc(), args );
	}
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIElementArrayPanel::InitVars()
{
	CUIEmulateUIElementPanel::InitVars();

	GetVarBlock()->AddVariable(mv_ArgCount,   "Count");

	const int argCount = mv_ArgCount;
	SetArgVarsCount(argCount);
	for (int i = 0; i < argCount; ++i)
	{
		GetVarBlock()->AddVariable(GetArgVar(i), string().Format("Value %i", i+1).c_str());
	}
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIElementArrayPanel::OnVarChange( IVariable *pVar )
{
	if (GetArgVarsCount() != mv_ArgCount)
	{
		Invalidate();
	}
}
