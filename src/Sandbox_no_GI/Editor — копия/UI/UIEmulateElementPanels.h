////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEmulateElementPanels.h
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#ifndef __UIEmulateElementPanels__H_
#define __UIEmulateElementPanels__H_

#include "UIEmulatePanel.h"


////////////////////////////////////////////////////////////////////
/////////////// Base emu panel for UIElement items /////////////////
////////////////////////////////////////////////////////////////////
class CUIEmulateUIElementPanel : public CUIEmulatePanel
{
public:
	CUIEmulateUIElementPanel( IUIElement* pElement, const SUIParameterDesc& desc );

	virtual void* GetData() const { return m_pElement; }
	virtual void Refresh();

protected:
	virtual void InitVars();
	virtual void OnVarChange( IVariable *pVar );

	virtual int GetDescCount() = 0;
	virtual const SUIParameterDesc* GetDescInfo(int index) = 0;

	IUIElement* GetElement() const;
	inline const SUIParameterDesc& GetDesc() const { return m_Desc; }
	inline void SetDesc( const SUIParameterDesc& desc ) { m_Desc = desc; m_ParameterName = desc.sDisplayName; }
	inline const char* GetParamName() const { return m_ParameterName.c_str(); }
private:
	IUIElement* m_pElement;
	CSmartVariable<int> mv_InstanceId;
	SUIParameterDesc m_Desc;
	string m_ParameterName;
};


////////////////////////////////////////////////////////////////////
////////////////// IUIElement var emu panel ////////////////////////
////////////////////////////////////////////////////////////////////
class CUIEmulateUIElementVarPanel : public CUIEmulateUIElementPanel
{
public:
	CUIEmulateUIElementVarPanel( IUIElement* pElement, const SUIParameterDesc& event );

protected:
	virtual void CallEvent( const SUIArguments& args );
	virtual void InitVars();

	virtual int GetDescCount() { return GetElement()->GetVariableCount(); }
	virtual const SUIParameterDesc* GetDescInfo(int index) { return GetElement()->GetVariableDesc(index); }
private:
	CSmartVariable<CString> mv_Value;
};

////////////////////////////////////////////////////////////////////
///////////////// IUIElement array emu panel ///////////////////////
////////////////////////////////////////////////////////////////////
class CUIEmulateUIElementArrayPanel : public CUIEmulateUIElementPanel
{
public:
	CUIEmulateUIElementArrayPanel( IUIElement* pElement, const SUIParameterDesc& desc );

protected:
	virtual void CallEvent( const SUIArguments& args );
	virtual void InitVars();
	virtual void OnVarChange( IVariable *pVar );

	virtual int GetDescCount() { return GetElement()->GetArrayCount(); }
	virtual const SUIParameterDesc* GetDescInfo(int index) { return GetElement()->GetArrayDesc(index); }
private:
	CSmartVariable<int> mv_ArgCount;
};

////////////////////////////////////////////////////////////////////
/////////////// IUIElement movieclip emu panel /////////////////////
////////////////////////////////////////////////////////////////////
class CUIEmulateUIElementMCPanel : public CUIEmulateUIElementPanel
{
public:
	CUIEmulateUIElementMCPanel( IUIElement* pElement, const SUIParameterDesc& desc );

protected:
	virtual void CallEvent( const SUIArguments& args );
	virtual void InitVars();

	virtual int GetDescCount() { return GetElement()->GetMovieClipCount(); }
	virtual const SUIParameterDesc* GetDescInfo(int index) { return GetElement()->GetMovieClipDesc(index); }
private:
};

#endif //__UIEmulateElementPanels__H_