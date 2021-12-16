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
: m_bRemoveable(bRemoveable)
, m_bInvalidated(true)
, m_invokeBtnCaption(invokeBtnCaption)
, m_pInvokeButton(NULL)
, m_pRemoveButton(NULL)
{
	m_pVarBlock = new CVarBlock;

	GetIEditor()->RegisterNotifyListener(this);
}

////////////////////////////////////////////////////////////////////
CUIEmulatePanel::~CUIEmulatePanel()
{
	SAFE_DELETE(m_pInvokeButton);
	SAFE_DELETE(m_pRemoveButton);
	GetIEditor()->UnregisterNotifyListener(this);
}

////////////////////////////////////////////////////////////////////
void CUIEmulatePanel::ResizeToFitProperties()
{
	CRect rc;
	GetClientRect( rc );
	int h = m_wndProps.GetVisibleHeight() + 40;
	SetWindowPos( NULL,0,0,rc.right,h+4*2+4,SWP_NOMOVE );

	if (!m_pInvokeButton)
	{
		m_pInvokeButton = new CXTPButton;
		m_pInvokeButton->Create(m_invokeBtnCaption, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(0,0,100,24), this, IDD_BTN_INVOKE);
		m_pInvokeButton->ShowWindow(TRUE);
	}

	if (!m_pRemoveButton && m_bRemoveable)
	{
		m_pRemoveButton = new CXTPButton;
		m_pRemoveButton->Create("Remove", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(0,0,100,24), this, IDD_BTN_REMOVE);
		m_pRemoveButton->ShowWindow(TRUE);
	}

	if (GetVarBlock()->IsEmpty())
		m_wndProps.ShowWindow(FALSE);
	else
		m_wndProps.ShowWindow(TRUE);

	m_wndProps.SetWindowPos( NULL, 0, 0, rc.right, h - 32 ,SWP_NOMOVE );
	m_pInvokeButton->SetWindowPos( NULL, 6, h - 20, 0, 0,SWP_NOSIZE );
	if (m_pRemoveButton)
		m_pRemoveButton->SetWindowPos( NULL, rc.right - 6 - 100, h - 20, 0, 0,SWP_NOSIZE );
}

////////////////////////////////////////////////////////////////////
void CUIEmulatePanel::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if (event == eNotify_OnIdleUpdate)
	{
		if (m_bInvalidated)
		{
			DeleteVars();
			GetVarBlock()->Clear();
			InitVars();
			AddVariables();
			ResizeToFitProperties();
			m_bInvalidated = false;
		}
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
	ResizeToFitProperties();
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
