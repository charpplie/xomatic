////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   GroupPanel.cpp
//  Version:     v1.00
//  Created:     23/08/2009 by Sergiy Shaykin.
//  Compilers:   Visual C++.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Resource.h"
#include "GroupPanel.h"
#include "CryEdit.h"

#include "Objects\\Group.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// CGroupPanel dialog

IMPLEMENT_DYNAMIC(CGroupPanel, CDialog)

//////////////////////////////////////////////////////////////////////////
CGroupPanel::CGroupPanel( CGroup * obj, CWnd* pParent /* = NULL */)
	: CDialog(CGroupPanel::IDD, pParent), m_obj(obj)
{
	Create( IDD,AfxGetMainWnd() );

	GetIEditor()->RegisterNotifyListener( this );
}

CGroupPanel::~CGroupPanel()
{
	GetIEditor()->UnregisterNotifyListener( this );
}

void CGroupPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_GROUP, m_GroupBn);
	DDX_Control(pDX, IDC_OPEN, m_OpenBn);
	DDX_Control(pDX, IDC_ATTACH, m_AttachBn);
	DDX_Control(pDX, IDC_UNGROUP, m_UngroupBn);
	DDX_Control(pDX, IDC_CLOSE, m_CloseBn);
	DDX_Control(pDX, IDC_DETACH, m_DetachBn);
}


BEGIN_MESSAGE_MAP(CGroupPanel, CDialog)
	ON_BN_CLICKED(IDC_GROUP, OnGroup)
	ON_BN_CLICKED(IDC_OPEN, OnOpen)
	ON_BN_CLICKED(IDC_ATTACH, OnAttach)
	ON_BN_CLICKED(IDC_UNGROUP, OnUngroup)
	ON_BN_CLICKED(IDC_CLOSE, OnCloseGroup)
	ON_BN_CLICKED(IDC_DETACH, OnDetach)

END_MESSAGE_MAP()


// CGroupPanel message handlers

BOOL CGroupPanel::OnInitDialog()
{
	__super::OnInitDialog();

	UpdateButtons();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnGroup()
{
	((CCryEditApp*)AfxGetApp())->OnGroupMake();
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnOpen()
{
	((CCryEditApp*)AfxGetApp())->OnGroupOpen();
	UpdateButtons();
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnAttach()
{
	((CCryEditApp*)AfxGetApp())->OnGroupAttach();
	//UpdateButtons();
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnUngroup()
{
	((CCryEditApp*)AfxGetApp())->OnGroupUngroup();
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnCloseGroup()
{
	((CCryEditApp*)AfxGetApp())->OnGroupClose();
	UpdateButtons();
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnDetach()
{
	((CCryEditApp*)AfxGetApp())->OnGroupDetach();
	UpdateButtons();
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::UpdateButtons()
{
	if(!m_obj)
		return;

	GetDlgItem(IDC_OPEN)->EnableWindow( m_obj->IsOpen() ? FALSE : TRUE );
	GetDlgItem(IDC_CLOSE)->EnableWindow( m_obj->IsOpen() ? TRUE : FALSE );
	GetDlgItem(IDC_DETACH)->EnableWindow( m_obj->GetParent() ? TRUE : FALSE );
}

//////////////////////////////////////////////////////////////////////////
void CGroupPanel::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnEditToolChange:
		UpdateButtons();
		break;
	}
}