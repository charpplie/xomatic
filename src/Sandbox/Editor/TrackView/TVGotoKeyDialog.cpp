////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   tvGotokeydialog.cpp
//  Version:     v1.00
//  Created:     12/6/2003 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TVGotoKeyDialog.h"
#include "..\CryEditDoc.h"
#include "..\Objects\EntityScript.h"
#include "..\mission.h"
#include "..\missionscript.h"

#include "IMovieSystem.h"
#include "IEntitySystem.h"
#include "TrackViewKeyPropertiesDlg.h"

// CTVEntityKeyDialog dialog

IMPLEMENT_DYNAMIC(CTVGotoKeyDialog, CDialog)
CTVGotoKeyDialog::CTVGotoKeyDialog(CWnd* pParent /*=NULL*/)
: IKeyDlg(CTVGotoKeyDialog::IDD, pParent)
{
	m_node = 0;
	m_track = 0;
	m_key = 0;
}

CTVGotoKeyDialog::~CTVGotoKeyDialog()
{
}

void CTVGotoKeyDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTVGotoKeyDialog, CDialog)
	//ON_BN_CLICKED(IDC_HIDE, ControlsToKey)
	ON_EN_CHANGE(IDC_EDIT1, ControlsToKey)
END_MESSAGE_MAP()


// CTVEntityKeyDialog message handlers
//////////////////////////////////////////////////////////////////////////
void CTVGotoKeyDialog::SetKey( IAnimNode *node,IAnimTrack *track,int nkey )
{
	m_node = node;
	m_track = track;
	m_key = nkey;

	if (m_key < 0 || m_key >= m_track->GetNumKeys())
		return;

	if (track->GetType() != ATRACK_GOTO)
		return;

	IDiscreteFloatKey key;
	m_track->GetKey( m_key,&key );

	m_fTime.SetValue( key.m_fValue);
}

//////////////////////////////////////////////////////////////////////////
BOOL CTVGotoKeyDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	// TODO:  Add extra initialization here

	m_fTime.Create(this, IDC_EDIT1);
	m_fTime.SetInteger(false);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
//////////////////////////////////////////////////////////////////////////
void CTVGotoKeyDialog::ControlsToKey()
{
	if (!m_track || m_key < 0 || m_key >= m_track->GetNumKeys())
		return;

	//bool hidden = m_hide.GetCheck() == BST_CHECKED;

	double dValue=m_fTime.GetValue();

	IDiscreteFloatKey key;
	m_track->GetKey( m_key,&key );

	//key.hidden = hidden;
	key.m_fValue=(float)dValue;	
	m_track->SetKey( m_key,&key );

	RefreshTrackView();
}