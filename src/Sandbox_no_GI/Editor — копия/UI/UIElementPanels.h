////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIElementPanels.h
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#ifndef __UIElementPanels__H_
#define __UIElementPanels__H_

#include "PropertiesPanel.h"
#include <IFlashUI.h>

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class CUIElementPropertiesPanel : public CPropertiesPanel
{
public:
	CUIElementPropertiesPanel();

	void SetElement( IUIElement *pElement );
	virtual void UpdateChanges() = 0;

protected:
	virtual void InitVars() = 0;
	virtual void UpdateVars() = 0;
	virtual void OnVarChange( IVariable *pVar ) = 0;

	inline IUIElement* GetElement() const { return m_pElement; }
	inline CVarBlock* GetVarBlock() const { return m_pVarBlock; }

	inline void BeginUpdate() { m_bIsUpdating = true; }
	inline void EndUpdate() { m_bIsUpdating = false; }
	inline bool IsUpdating() const { return m_bIsUpdating; } 

	void AddVariables()	{	SetVarBlock( m_pVarBlock, functor(*this,&CUIElementPropertiesPanel::OnVarChangeInt) ); }

private:
	void OnVarChangeInt( IVariable *pVar ) { if (!m_bIsUpdating) OnVarChange( pVar ); }

private:
	_smart_ptr<CVarBlock> m_pVarBlock;
	IUIElement* m_pElement;
	bool m_bIsInit;
	bool m_bIsUpdating;
};


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class CUIElementPropertiesPanelProperties : public CUIElementPropertiesPanel
{
public:
	virtual void UpdateChanges();

protected:
	virtual void InitVars();
	virtual void UpdateVars();
	virtual void OnVarChange( IVariable *pVar );

private:
	CSmartVariable<bool> mv_visible;
	CSmartVariable<bool> mv_init;
	CSmartVariable<int> mv_exTextures;
	CSmartVariable<int> mv_layer;
	CSmartVariable<float> mv_alpha;
	CSmartVariable<CString> mv_FlashFile;
};


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class CUIElementPropertiesPanelFlags : public CUIElementPropertiesPanel
{
public:
	virtual void UpdateChanges();

protected:
	virtual void InitVars();
	virtual void UpdateVars();
	virtual void OnVarChange( IVariable *pVar );

private:
	struct SFlagVar
	{
		SFlagVar() : DisplayName("") { mv_Var = false; }
		SFlagVar(const char* displayName) : DisplayName(displayName) { mv_Var = false; }
		const char* DisplayName;
		CSmartVariable<bool> mv_Var;
	};
	typedef std::map< IUIElement::EFlashUIFlags, SFlagVar > TFlagVarMap;
	TFlagVarMap mv_FlagVars;
};


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class CUIElementPropertiesPanelConstraints 
	: public CUIElementPropertiesPanel
	, public IEditorNotifyListener
{
public:
	CUIElementPropertiesPanelConstraints();
	virtual ~CUIElementPropertiesPanelConstraints();

	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );
	virtual void UpdateChanges();

protected:
	virtual void InitVars();
	virtual void UpdateVars();
	virtual void OnVarChange( IVariable *pVar );

private:
	void UpdateAvailVars( IUIElement::SUIConstraints::EPositionType type );

private:
	bool m_bInvalidated;

	_smart_ptr< CVarEnumList<int> > enumType;
	_smart_ptr< CVarEnumList<int> > enumHAlign;
	_smart_ptr< CVarEnumList<int> > enumVAlign;

	CSmartVariableEnum<int> mv_Type;
	CSmartVariableEnum<int> mv_HAlign;
	CSmartVariableEnum<int> mv_VAlign;

	CSmartVariable<int> mv_Top;
	CSmartVariable<int> mv_Left;
	CSmartVariable<int> mv_Width;
	CSmartVariable<int> mv_Height;

	CSmartVariable<bool> mv_scale;
	CSmartVariable<bool> mv_max;
};

#endif //__UIElementPanels__H_