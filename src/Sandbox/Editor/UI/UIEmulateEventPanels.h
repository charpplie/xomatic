////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEmulateEventPanel.h
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#ifndef __UIEmulateEventPanels__H_
#define __UIEmulateEventPanels__H_

#include "UIEmulatePanel.h"


////////////////////////////////////////////////////////////////////
//////// Base emu panel for events that have SUIEventDesc //////////
////////////////////////////////////////////////////////////////////
class CUIEmulateUIEventPanel : public CUIEmulatePanel
{
public:
	CUIEmulateUIEventPanel( const SUIEventDesc& event );

protected:
	virtual void InitVars();
	virtual void OnVarChange( IVariable *pVar );

	inline const SUIEventDesc& GetEventDesc() const { return m_Event; }
	inline void SetEventDesc( const SUIEventDesc& event ) { m_Event = event; m_EventName = event.sDisplayName; }
	inline const char* GetEventName() const { return m_EventName.c_str(); }
private:
	SUIEventDesc m_Event;
	string m_EventName;
};


////////////////////////////////////////////////////////////////////
//////////////// IUIElement function emu panel /////////////////////
////////////////////////////////////////////////////////////////////
class CUIEmulateUIElementEventPanel : public CUIEmulateUIEventPanel
{
public:
	CUIEmulateUIElementEventPanel( IUIElement* pElement, const SUIEventDesc& event );

	virtual void Refresh();
	virtual void* GetData() const { return m_pElement; }

protected:
	virtual void CallEvent( const SUIArguments& args );
	virtual void InitVars();

private:
	IUIElement* m_pElement;
	CSmartVariable<int> mv_InstanceId;
};


////////////////////////////////////////////////////////////////////
///////////////// IUIEventSystem event emu panel ///////////////////
////////////////////////////////////////////////////////////////////
class CUIEmulateUIEventSystemEventPanel : public CUIEmulateUIEventPanel
{
public:
	CUIEmulateUIEventSystemEventPanel( IUIEventSystem* pEventSystem, const SUIEventDesc& event );

	virtual void Refresh();
	virtual void* GetData() const { return m_pEventSystem; }

protected:
	virtual void CallEvent( const SUIArguments& args );

private:
	IUIEventSystem* m_pEventSystem;
};

////////////////////////////////////////////////////////////////////
//////////////// direct flash invoke emu panel /////////////////////
////////////////////////////////////////////////////////////////////
class CUIDirectInvokePanel : public CUIEmulatePanel
{
public:
	CUIDirectInvokePanel( bool bRemovable = true );

	virtual void Refresh();
	virtual void* GetData() const { return NULL; }

protected:
	virtual void CallEvent( const SUIArguments& args );
	virtual void InitVars();
	virtual void OnVarChange( IVariable *pVar );

private:
	void UpdateElementEnum();

private:
	_smart_ptr< CVarEnumList<CString> > m_pEnumElements;

	CSmartVariableEnum<CString> mv_Element;
	CSmartVariable<int> mv_InstanceId;
	CSmartVariable<CString> mv_Function;
	CSmartVariable<int> mv_ArgCount;
};


////////////////////////////////////////////////////////////////////
///////////////////// IUIAction emu panel //////////////////////////
////////////////////////////////////////////////////////////////////
class CUIEmulateUIActionEventPanel : public CUIEmulatePanel
{
public:
	CUIEmulateUIActionEventPanel( IUIAction* pAction );

	virtual void Refresh();
	virtual void* GetData() const { return m_pAction; }

protected:
	virtual void CallEvent( const SUIArguments& args );
	virtual void InitVars();
	virtual void OnVarChange( IVariable *pVar );

private:
	IUIAction* m_pAction;
	CSmartVariable<int> mv_ArgCount;
};


#endif //__UIEmulateEventPanels__H_