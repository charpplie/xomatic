////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIActionPanels.cpp
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "UIActionPanels.h"

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
CUIActionPropertiesPanel::CUIActionPropertiesPanel()
: m_pAction(NULL)
{
	m_pVarBlock = new CVarBlock;

	mv_Name = "";
	mv_File = "";
	mv_enabled = false;

	mv_Name->SetFlags( mv_Name->GetFlags() | IVariable::UI_DISABLED );
	mv_File->SetFlags( mv_File->GetFlags() | IVariable::UI_DISABLED );

	m_pVarBlock->AddVariable( mv_Name, "Name" );
	m_pVarBlock->AddVariable( mv_File, "File location" );
	m_pVarBlock->AddVariable( mv_enabled, "Enabled" );
}

////////////////////////////////////////////////////////////////////
void CUIActionPropertiesPanel::SetAction( IUIAction *pAction )
{
	DeleteVars();
	m_pAction = pAction;
	if (m_pAction)
	{
		UpdateVars();
		AddVariables();
	}
}

////////////////////////////////////////////////////////////////////
void CUIActionPropertiesPanel::UpdateVars()
{
	if (m_pAction)
	{
		mv_Name = m_pAction->GetName();
		mv_File = string().Format("Libs/UI/UIActions/%s.xml", m_pAction->GetName()).c_str();
		mv_enabled = m_pAction->IsEnabled();
	}
}

////////////////////////////////////////////////////////////////////
void CUIActionPropertiesPanel::OnVarChange( IVariable *pVar )
{
	if (m_pAction)
	{
		bool enabled;
		mv_enabled->Get(enabled);
		m_pAction->SetEnabled(enabled);

		UpdateVars();
	}
}
