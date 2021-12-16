////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEmulatePanel.cpp
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "UIEmulatePanel.h"
#include "UIManager.h"
#include "UIEditor.h"

const int IDD_BTN_INVOKE = 6000;
const int IDD_BTN_REMOVE = 6001;

BEGIN_MESSAGE_MAP(CUIEmulatePanel, CPropertiesPanel)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDD_BTN_INVOKE, OnBtnInvoke)
	ON_BN_CLICKED(IDD_BTN_REMOVE, OnBtnRemove)
END_MESSAGE_MAP()


////////////////////////////////////////////////////////////////////
CUIEmulatePanel::CUIEmulatePanel(const char* invokeBtnCaption, bool bRemoveable)
: m_bInit(false)
, m_bInvalidated(true)
, m_bRemoveable(bRemoveable)
{
	m_pVarBlock = new CVarBlock;

	GetIEditor()->RegisterNotifyListener(this);

	m_InvokeButton.Create(invokeBtnCaption, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(0,0,100,24), this, IDD_BTN_INVOKE);
	m_InvokeButton.ShowWindow(TRUE);

	m_RemoveButton.Create("Remove", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(0,0,100,24), this, IDD_BTN_REMOVE);
	m_RemoveButton.ShowWindow(TRUE);

	if (!bRemoveable)
	{
		m_RemoveButton.EnableWindow(FALSE);
	}
}

////////////////////////////////////////////////////////////////////
CUIEmulatePanel::~CUIEmulatePanel()
{
	GetIEditor()->UnregisterNotifyListener(this);
}

////////////////////////////////////////////////////////////////////
void CUIEmulatePanel::ResizeToFitProperties()
{
	if (!m_bInvalidated) return;

	CRect rc;
	GetClientRect( rc );
	int h = m_pWndProps->GetVisibleHeight() + 40;
	SetWindowPos( NULL,0,0,rc.right + 50,h+4*2+4,SWP_NOMOVE );

	if (GetVarBlock()->IsEmpty())
		m_pWndProps->ShowWindow(FALSE);
	else
		m_pWndProps->ShowWindow(TRUE);

	m_pWndProps->SetWindowPos( NULL, 0, 0, rc.right, h - 32 ,SWP_NOMOVE );

	m_InvokeButton.SetWindowPos( NULL, 6, h - 20, 0, 0,SWP_NOSIZE );
	m_RemoveButton.SetWindowPos( NULL, rc.right - 6 - 100, h - 20, 0, 0,SWP_NOSIZE );

	m_bInvalidated = false;
}

////////////////////////////////////////////////////////////////////
void CUIEmulatePanel::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if (event == eNotify_OnIdleUpdate)
	{
		Init();
		ResizeToFitProperties();
	}
}

////////////////////////////////////////////////////////////////////
void CUIEmulatePanel::RemoveThis()
{
	if (GetIEditor()->GetUIManager()->GetEditor())
		GetIEditor()->GetUIManager()->GetEditor()->RemoveEventEmuPanel( this );
}

////////////////////////////////////////////////////////////////////
void CUIEmulatePanel::SetArgVarsCount( int count )
{
	// assign each variable manually with new CSmartVariable since resize() will assign all elements "the one" default SmartVar wich points to the same CVariable!
	m_ArgVars.resize(count);
	for (int i = 0; i < count; ++i)
	{
		m_ArgVars[i].first = CSmartVariable<CString>();
		m_ArgVars[i].second = false;
	}
}


////////////////////////////////////////////////////////////////////
void CUIEmulatePanel::AddVariables()
{
	SetVarBlock( m_pVarBlock, functor(*this, &CUIEmulatePanel::OnVarChangeInt) );
}

////////////////////////////////////////////////////////////////////
void CUIEmulatePanel::OnVarChangeInt( IVariable *pVar )
{
	OnVarChange(pVar);
}

////////////////////////////////////////////////////////////////////
void CUIEmulatePanel::OnSize(UINT nType, int cx, int cy) 
{
	CPropertiesPanel::OnSize(nType, cx, cy);
	m_bInvalidated = true;
}

////////////////////////////////////////////////////////////////////
void CUIEmulatePanel::OnBtnInvoke()
{
	SUIArguments args;
	for (int i = 0; i < m_ArgVars.size(); ++i)
	{
		CString arg = m_ArgVars[i].first;
		if (m_ArgVars[i].second) // dynamic arg, parse the string and add all values
		{
			SUIArguments arr;
			arr.SetDelimiter(",");
			arr.SetArguments(arg.GetString());
			for (int k = 0; k < arr.GetArgCount(); ++k)
			{
				string arr_arg;
				arr.GetArg(k, arr_arg);
				args.AddArgument( arr_arg, eUIDT_Any );
			}
		}
		else
			args.AddArgument( string(arg.GetString()), eUIDT_Any );
	}

	CallEvent( args );
}

////////////////////////////////////////////////////////////////////
void CUIEmulatePanel::OnBtnRemove()
{
	RemoveThis();
}

void CUIEmulatePanel::Init()
{
	if (m_bInit) return;

	InitVars();
	AddVariables();

	m_bInit = true;
}