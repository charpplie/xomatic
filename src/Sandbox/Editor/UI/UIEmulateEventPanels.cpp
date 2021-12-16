////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEmulateEventPanel.cpp
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "UIEmulateEventPanels.h"
#include <limits>

#define MAX_INSTANCE_ID  (std::numeric_limits<int>::max())

////////////////////////////////////////////////////////////////////
//////// Base emu panel for events that have SUIEventDesc //////////
////////////////////////////////////////////////////////////////////
CUIEmulateUIEventPanel::CUIEmulateUIEventPanel( const SUIEventDesc& event )
: CUIEmulatePanel("Call")
, m_Event(event)
, m_EventName(event.sDisplayName)
{
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIEventPanel::InitVars()
{
	const int count = m_Event.InputParams.Params.size(); 
	SetArgVarsCount(count + (m_Event.InputParams.IsDynamic ? 1 : 0));
	for (int i = 0; i < count; ++i)
	{
		GetVarBlock()->AddVariable(GetArgVar(i), m_Event.InputParams.Params[i].sDisplayName );
	}
	if (m_Event.InputParams.IsDynamic)
	{
		SetArgVarDyn(count);
		GetVarBlock()->AddVariable(GetArgVar(count), m_Event.InputParams.sDynamicName );
	}
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIEventPanel::OnVarChange( IVariable *pVar )
{
}


////////////////////////////////////////////////////////////////////
//////////////// IUIElement function emu panel /////////////////////
////////////////////////////////////////////////////////////////////
CUIEmulateUIElementEventPanel::CUIEmulateUIElementEventPanel( IUIElement* pElement, const SUIEventDesc& event )
: CUIEmulateUIEventPanel(event)
, m_pElement(pElement)
{
	mv_InstanceId = 0;
	mv_InstanceId->SetLimits(0, MAX_INSTANCE_ID);
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIElementEventPanel::Refresh()
{
	if (m_pElement->IsValid())
	{
		for (int i = 0; i < m_pElement->GetFunctionCount(); ++i)
		{
			const SUIEventDesc* pEvt = m_pElement->GetFunctionDesc(i);
			if (strcmp(pEvt->sDisplayName, GetEventName()) == 0)
			{
				SetEventDesc(*pEvt);
				Invalidate();
				return;
			}
		}
	}

	RemoveThis();
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIElementEventPanel::CallEvent( const SUIArguments& args )
{
	if (m_pElement)
	{
		IUIElement* pInstance = m_pElement->GetInstance(mv_InstanceId);
		pInstance->CallFunction( &GetEventDesc(), args );
	}
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIElementEventPanel::InitVars()
{
	GetVarBlock()->AddVariable( mv_InstanceId, "InstanceId" );
	CUIEmulateUIEventPanel::InitVars();
}


////////////////////////////////////////////////////////////////////
///////////////// IUIEventSystem event emu panel ///////////////////
////////////////////////////////////////////////////////////////////
CUIEmulateUIEventSystemEventPanel::CUIEmulateUIEventSystemEventPanel( IUIEventSystem* pEventSystem, const SUIEventDesc& event )
: CUIEmulateUIEventPanel(event)
, m_pEventSystem(pEventSystem)
{
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIEventSystemEventPanel::Refresh()
{
	for (int i = 0; i < m_pEventSystem->GetEventCount(); ++i)
	{
		const SUIEventDesc* pEvt = m_pEventSystem->GetEventDesc(i);
		if (strcmp(pEvt->sDisplayName, GetEventName()) == 0)
		{
			SetEventDesc(*pEvt);
			Invalidate();
			return;
		}
	}

	RemoveThis();
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIEventSystemEventPanel::CallEvent( const SUIArguments& args )
{
	if (m_pEventSystem)
	{
		SUIEvent evt( m_pEventSystem->GetEventId( GetEventName() ), args );
 		m_pEventSystem->SendEvent(evt);
	}
}

////////////////////////////////////////////////////////////////////
//////////////// direct flash invoke emu panel /////////////////////
////////////////////////////////////////////////////////////////////
CUIDirectInvokePanel::CUIDirectInvokePanel( bool bRemovable /*= true*/ )
: CUIEmulatePanel("Invoke", bRemovable)
{
	mv_Element = "";
	mv_Function = "";
	mv_InstanceId = 0;
	mv_ArgCount = 0;

	mv_ArgCount->SetLimits(0,100);
	mv_InstanceId->SetLimits(0, MAX_INSTANCE_ID);
}

////////////////////////////////////////////////////////////////////
void CUIDirectInvokePanel::Refresh()
{
	Invalidate();
}

////////////////////////////////////////////////////////////////////
void CUIDirectInvokePanel::CallEvent( const SUIArguments& args )
{
	CString element = mv_Element;
	int instanceid = mv_InstanceId;
	CString fct = mv_Function;
	IUIElement* pElement = gEnv->pFlashUI->GetUIElement( element.GetString() );
	if (pElement && pElement->IsValid() && !fct.IsEmpty())
	{
		IUIElement* pInstance = pElement->GetInstance(instanceid);
		IFlashPlayer* pPlayer = pInstance ? pInstance->GetFlashPlayer() : NULL;
		if (pPlayer)
		{
			pPlayer->Invoke( fct.GetString(), args.GetAsList(), args.GetArgCount() );
		}
	}
}

////////////////////////////////////////////////////////////////////
void CUIDirectInvokePanel::InitVars()
{
	UpdateElementEnum();

	GetVarBlock()->AddVariable(mv_Element,    "UIElement");
	GetVarBlock()->AddVariable(mv_InstanceId, "Instance ID");
	GetVarBlock()->AddVariable(mv_Function,   "AS Function");
	GetVarBlock()->AddVariable(mv_ArgCount,   "ArgCount");

	const int argCount = mv_ArgCount;
	SetArgVarsCount(argCount);
	for (int i = 0; i < argCount; ++i)
	{
		GetVarBlock()->AddVariable(GetArgVar(i), string().Format("Arg %i", i+1).c_str());
	}

}

////////////////////////////////////////////////////////////////////
void CUIDirectInvokePanel::OnVarChange( IVariable *pVar )
{
	if (GetArgVarsCount() != mv_ArgCount)
	{
		Invalidate();
	}
}

////////////////////////////////////////////////////////////////////
void CUIDirectInvokePanel::UpdateElementEnum()
{
	mv_Element->SetEnumList( NULL );
	m_pEnumElements = new CVarEnumList<CString>;

	if (gEnv->pFlashUI)
	{
		const int count = gEnv->pFlashUI->GetUIElementCount();
		for (int i = 0; i < count; ++i)
		{
			IUIElement* pElement = gEnv->pFlashUI->GetUIElement(i);
			CString name = pElement->GetName();
			if (!pElement->IsValid())
				name += " [ERROR]";
			m_pEnumElements->AddItem( pElement->GetName(),name );
		}
	}
	mv_Element->SetEnumList( m_pEnumElements );
}


////////////////////////////////////////////////////////////////////
///////////////////// IUIAction emu panel //////////////////////////
////////////////////////////////////////////////////////////////////
CUIEmulateUIActionEventPanel::CUIEmulateUIActionEventPanel( IUIAction* pAction )
: CUIEmulatePanel("Start")
, m_pAction(pAction)
{
	mv_ArgCount = 0;
	mv_ArgCount->SetLimits(0,100);
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIActionEventPanel::Refresh()
{
	if (!m_pAction->IsValid())
		RemoveThis();
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIActionEventPanel::CallEvent( const SUIArguments& args )
{
	gEnv->pFlashUI->GetUIActionManager()->StartAction(m_pAction, args);
}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIActionEventPanel::InitVars()
{
	GetVarBlock()->AddVariable(mv_ArgCount,   "ArgCount");

	const int argCount = mv_ArgCount;
	SetArgVarsCount(argCount);
	for (int i = 0; i < argCount; ++i)
	{
		GetVarBlock()->AddVariable(GetArgVar(i), string().Format("Arg %i", i+1).c_str());
	}

}

////////////////////////////////////////////////////////////////////
void CUIEmulateUIActionEventPanel::OnVarChange( IVariable *pVar )
{
	if (GetArgVarsCount() != mv_ArgCount)
	{
		Invalidate();
	}
}

