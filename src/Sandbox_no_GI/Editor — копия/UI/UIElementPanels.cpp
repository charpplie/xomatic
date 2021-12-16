////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIElementPanels.cpp
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "UIElementPanels.h"

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
CUIElementPropertiesPanel::CUIElementPropertiesPanel()
: m_pElement(NULL)
, m_bIsUpdating(false)
, m_bIsInit(false)
{
	m_pVarBlock = new CVarBlock;

	SetElement(NULL);
}

////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanel::SetElement( IUIElement *pElement )
{
	DeleteVars();
	m_pElement = pElement;
	if (m_pElement)
	{
		if (!m_bIsInit)
		{
			InitVars();
			m_bIsInit = true;
		}
		UpdateVars();
		AddVariables();
	}
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelProperties::InitVars()
{
	mv_FlashFile = "";
	mv_visible = false;
	mv_init = false;
	mv_layer = 0;
	mv_alpha = 0;
	mv_exTextures = 0;
	mv_alpha->SetLimits(0,1.f);

	mv_exTextures->SetFlags( mv_exTextures->GetFlags() | IVariable::UI_DISABLED );

	GetVarBlock()->AddVariable( mv_FlashFile, "Flash file" );
	GetVarBlock()->AddVariable( mv_visible, "Visible" );
	GetVarBlock()->AddVariable( mv_init, "Initialized" );
	GetVarBlock()->AddVariable( mv_exTextures, "Dynamic Textures" );
	GetVarBlock()->AddVariable( mv_layer, "Layer" );
	GetVarBlock()->AddVariable( mv_alpha, "Background Alpha" );
}

////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelProperties::UpdateVars()
{
	if (GetElement())
	{
		BeginUpdate();

		mv_FlashFile = GetElement()->GetFlashFile();
		mv_visible = GetElement()->IsVisible();
		mv_init = GetElement()->IsInit();
		mv_exTextures = GetElement()->GetNumExtTextures();
		if (GetElement()->GetNumExtTextures() > 0)
		{
			mv_exTextures->SetFlags( mv_exTextures->GetFlags() | IVariable::UI_DISABLED );
			mv_exTextures->SetFlags( mv_exTextures->GetFlags() | IVariable::UI_DISABLED );
		}
		else
		{
			mv_exTextures->SetFlags( mv_exTextures->GetFlags() & ~IVariable::UI_DISABLED );
			mv_exTextures->SetFlags( mv_exTextures->GetFlags() & ~IVariable::UI_DISABLED );
		}
		mv_layer = GetElement()->GetLayer();
		mv_alpha = GetElement()->GetAlpha();

		EndUpdate();
	}
}

////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelProperties::OnVarChange( IVariable *pVar )
{
	if (GetElement())
	{
		bool visible, init;
		int layer;
		float alpha;
		CString file;
		mv_visible->Get(visible);
		mv_init->Get(init);
		mv_layer->Get(layer);
		mv_alpha->Get(alpha);
		mv_FlashFile->Get(file);

		GetElement()->SetFlashFile(file.GetString());
		GetElement()->SetLayer(layer);
		GetElement()->SetAlpha(alpha);
		const bool wasVisible = GetElement()->IsVisible();
		GetElement()->SetVisible(visible);

		// init / unload at the end since the functinos above can lazy init the element
		// only unload if not visible
		if (init) 
			GetElement()->Init();
		else if (wasVisible == visible)
			GetElement()->Unload();

		UpdateVars();
	}
}

////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelProperties::UpdateChanges()
{
	UpdateVars();
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelFlags::InitVars()
{
	mv_FlagVars[IUIElement::eFUI_HARDWARECURSOR] = SFlagVar("Hardware Cursor");
	mv_FlagVars[IUIElement::eFUI_MOUSEEVENTS] = SFlagVar("Mouse Events");
	mv_FlagVars[IUIElement::eFUI_KEYEVENTS] = SFlagVar("Key Events");
	mv_FlagVars[IUIElement::eFUI_CONSOLE_MOUSE] = SFlagVar("Console Mouse");
	mv_FlagVars[IUIElement::eFUI_CONSOLE_CURSOR] = SFlagVar("Console Mouse Cursor");
	mv_FlagVars[IUIElement::eFUI_CONTROLLER_INPUT] = SFlagVar("Constroller Input");
	mv_FlagVars[IUIElement::eFUI_EVENTS_EXCLUSIVE] = SFlagVar("Events Exclusive");
	mv_FlagVars[IUIElement::eFUI_RENDER_LOCKLESS] = SFlagVar("Render Lockless");
	mv_FlagVars[IUIElement::eFUI_FORCE_NO_UNLOAD] = SFlagVar("Force no Unload");

	for (TFlagVarMap::iterator it = mv_FlagVars.begin(); it != mv_FlagVars.end(); ++it)
		GetVarBlock()->AddVariable( it->second.mv_Var, it->second.DisplayName );
}

////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelFlags::UpdateVars()
{
	if (GetElement())
	{
		BeginUpdate();
		for (TFlagVarMap::iterator it = mv_FlagVars.begin(); it != mv_FlagVars.end(); ++it)
			it->second.mv_Var = GetElement()->HasFlag(it->first);
		EndUpdate();
	}
}

////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelFlags::OnVarChange( IVariable *pVar )
{
	if (GetElement())
	{
		for (TFlagVarMap::iterator it = mv_FlagVars.begin(); it != mv_FlagVars.end(); ++it)
		{
			int set;
			it->second.mv_Var->Get(set);
			GetElement()->SetFlag(it->first, set > 0);
		}
		UpdateVars();
	}
}

////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelFlags::UpdateChanges()
{
	UpdateVars();
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
CUIElementPropertiesPanelConstraints::CUIElementPropertiesPanelConstraints()
: m_bInvalidated(false)
{
	enumType = new CVarEnumList<int>;
	enumType->AddItem( "Fullscreen", IUIElement::SUIConstraints::ePT_Fullscreen );
	enumType->AddItem( "Dynamic", IUIElement::SUIConstraints::ePT_Dynamic );
	enumType->AddItem( "Fixed", IUIElement::SUIConstraints::ePT_Fixed );

	enumHAlign = new CVarEnumList<int>;
	enumHAlign->AddItem( "Left", IUIElement::SUIConstraints::ePA_Lower );
	enumHAlign->AddItem( "Center", IUIElement::SUIConstraints::ePA_Mid );
	enumHAlign->AddItem( "Right", IUIElement::SUIConstraints::ePA_Upper );

	enumVAlign = new CVarEnumList<int>;
	enumVAlign->AddItem( "Top", IUIElement::SUIConstraints::ePA_Lower );
	enumVAlign->AddItem( "Center", IUIElement::SUIConstraints::ePA_Mid );
	enumVAlign->AddItem( "Bottom", IUIElement::SUIConstraints::ePA_Upper );

	GetIEditor()->RegisterNotifyListener(this);
}

////////////////////////////////////////////////////////////////////
CUIElementPropertiesPanelConstraints::~CUIElementPropertiesPanelConstraints()
{
	GetIEditor()->UnregisterNotifyListener(this);
}

////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelConstraints::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if (event == eNotify_OnIdleUpdate)
	{
		if (m_bInvalidated)
		{
			DeleteVars();
			AddVariables();
			m_bInvalidated = false;
		}
	}
}

////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelConstraints::InitVars()
{
	mv_Type->SetEnumList( enumType );
	mv_HAlign->SetEnumList( enumHAlign );
	mv_VAlign->SetEnumList( enumVAlign );

	mv_Type = IUIElement::SUIConstraints::ePT_Dynamic;
	mv_HAlign = IUIElement::SUIConstraints::ePA_Mid;
	mv_VAlign = IUIElement::SUIConstraints::ePA_Mid;

	GetVarBlock()->AddVariable( mv_Type, "Align Mode" );
	GetVarBlock()->AddVariable( mv_HAlign, "Horizontal Align" );
	GetVarBlock()->AddVariable( mv_VAlign, "Vertical Align" );

	GetVarBlock()->AddVariable( mv_Top, "Top" );
	GetVarBlock()->AddVariable( mv_Left, "Left" );
	GetVarBlock()->AddVariable( mv_Width, "Width" );
	GetVarBlock()->AddVariable( mv_Height, "Heigth" );

	GetVarBlock()->AddVariable( mv_scale, "Scale" );
	GetVarBlock()->AddVariable( mv_max, "Maximize" );

}

////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelConstraints::UpdateVars()
{
	if (GetElement())
	{
		BeginUpdate();

		const IUIElement::SUIConstraints& c = GetElement()->GetConstraints();

		UpdateAvailVars(c.eType);

		mv_Type = c.eType;
		mv_HAlign = c.eHAlign;
		mv_VAlign = c.eVAlign;
		mv_Top = c.iTop;
		mv_Left = c.iLeft;
		mv_Width = c.iWidth;
		mv_Height = c.iHeight;
		mv_scale = c.bScale;
		mv_max = c.bMax;

		EndUpdate();
	}
}

////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelConstraints::OnVarChange( IVariable *pVar )
{
	if (GetElement())
	{
		IUIElement::SUIConstraints c = GetElement()->GetConstraints();

		int type, halign, valign;
		mv_Type->Get(type);
		mv_HAlign->Get(halign);
		mv_VAlign->Get(valign);
		c.eType = (IUIElement::SUIConstraints::EPositionType) type;
		c.eHAlign = (IUIElement::SUIConstraints::EPositionAlign) halign;
		c.eVAlign = (IUIElement::SUIConstraints::EPositionAlign) valign;

		mv_Top->Get(c.iTop);
		mv_Left->Get(c.iLeft);
		mv_Width->Get(c.iWidth);
		mv_Height->Get(c.iHeight);
		mv_scale->Get(c.bScale);
		mv_max->Get(c.bMax);

		GetElement()->SetConstraints(c);
		GetElement()->Reload();

		UpdateVars();
		m_bInvalidated = true;
	}
}

////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelConstraints::UpdateAvailVars( IUIElement::SUIConstraints::EPositionType type )
{
	GetVarBlock()->Clear();
	GetVarBlock()->AddVariable( mv_Type, "Align Mode" );
	switch (type)
	{
	case IUIElement::SUIConstraints::ePT_Fullscreen:
		GetVarBlock()->AddVariable( mv_scale, "Scale" );
		break;
	case IUIElement::SUIConstraints::ePT_Dynamic:
		GetVarBlock()->AddVariable( mv_HAlign, "Horizontal Align" );
		GetVarBlock()->AddVariable( mv_VAlign, "Vertical Align" );
		GetVarBlock()->AddVariable( mv_scale, "Scale" );
		GetVarBlock()->AddVariable( mv_max, "Maximize" );
		break;
	case IUIElement::SUIConstraints::ePT_Fixed:
		GetVarBlock()->AddVariable( mv_Top, "Top" );
		GetVarBlock()->AddVariable( mv_Left, "Left" );
		GetVarBlock()->AddVariable( mv_Width, "Width" );
		GetVarBlock()->AddVariable( mv_Height, "Heigth" );
		GetVarBlock()->AddVariable( mv_scale, "Scale" );
		break;
	}
}

////////////////////////////////////////////////////////////////////
void CUIElementPropertiesPanelConstraints::UpdateChanges()
{
	UpdateVars();
}