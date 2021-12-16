////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TVTrackEventKeyDialog.cpp
//  Version:     v1.00
//  Created:     4/4/2008 by Kevin.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "TVTrackEventKeyDialog.h"
#include "CryEditDoc.h"
#include "Objects\EntityScript.h"
#include "mission.h"
#include "missionscript.h"

#include "IMovieSystem.h"
#include "IEntitySystem.h"
#include <I3DEngine.h>

#include "TVEventsDialog.h"
#include "TrackViewKeyPropertiesDlg.h"

// CTVEntityKeyDialog dialog

IMPLEMENT_DYNAMIC(CTVTrackEventKeyDialog, CDialog)
CTVTrackEventKeyDialog::CTVTrackEventKeyDialog(CWnd* pParent /*=NULL*/)
	: IKeyDlg(CTVTrackEventKeyDialog::IDD, pParent)
{
	m_node = 0;
	m_track = 0;
	m_key = 0;
}

CTVTrackEventKeyDialog::~CTVTrackEventKeyDialog()
{
}

void CTVTrackEventKeyDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EVENT, m_event);
	DDX_Control(pDX, IDC_EVENT_VALUE, m_eventValue);
	//DDX_Control(pDX, IDC_HIDE, m_hide);
}


BEGIN_MESSAGE_MAP(CTVTrackEventKeyDialog, CDialog)
	//ON_BN_CLICKED(IDC_HIDE, ControlsToKey)
	ON_CBN_SELENDOK(IDC_EVENT, ControlsToKey)
	ON_CBN_SELENDOK(IDC_ANIMATION, ControlsToKey)
	ON_EN_KILLFOCUS(IDC_EVENT_VALUE, ControlsToKey)
	ON_BN_CLICKED(IDC_EVENT_EDIT, &CTVTrackEventKeyDialog::OnBnClickedEventEdit)
END_MESSAGE_MAP()


// CTVEntityKeyDialog message handlers

void CTVTrackEventKeyDialog::SetKey( IAnimNode *node,IAnimTrack *track,int nkey )
{
	m_node = node;
	m_track = track;
	m_key = nkey;

	if (m_key < 0 || m_key >= m_track->GetNumKeys())
		return;

	if (track->GetType() != ATRACK_TRACKEVENT)
		return;

	m_event.ResetContent();
	m_eventValue.Clear();

	// Add track events
	if (CAnimationContext *pContext = GetIEditor()->GetAnimation())
	{
		if (IAnimSequence *pSequence = pContext->GetSequence())
		{
			const int iCount = pSequence->GetTrackEventsCount();
			for (int i = 0; i < iCount; ++i)
			{
				m_event.AddString(pSequence->GetTrackEvent(i));
			}
		}
	}
	
	IEventKey key;
	m_track->GetKey( m_key,&key );

	m_event.SelectString( -1,key.event );
	m_eventValue.SetWindowText( key.eventValue );
	//m_hide.SetCheck( (key.hidden)?BST_CHECKED:BST_UNCHECKED );
}

//////////////////////////////////////////////////////////////////////////
BOOL CTVTrackEventKeyDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////
void CTVTrackEventKeyDialog::ControlsToKey()
{
	if (!m_track || m_key < 0 || m_key >= m_track->GetNumKeys())
		return;

	//bool hidden = m_hide.GetCheck() == BST_CHECKED;

	CString event,value,animation;
	m_event.GetWindowText(event);
	m_eventValue.GetWindowText(value);

	IEventKey key;
	m_track->GetKey( m_key,&key );
	
	//key.hidden = hidden;
	strncpy( key.event,event,sizeof(key.event) );
	strncpy( key.eventValue,value,sizeof(key.eventValue) );
	strncpy( key.animation,animation,sizeof(key.animation) );

	key.duration = 0;

	m_track->SetKey( m_key,&key );

	RefreshTrackView();
}

void CTVTrackEventKeyDialog::OnBnClickedEventEdit()
{
	if (CAnimationContext *pContext = GetIEditor()->GetAnimation())
	{
		if (IAnimSequence *pSequence = pContext->GetSequence())
		{
			// Create dialog
			CTVEventsDialog dlg(pSequence);
			if (dlg.DoModal() == IDOK)
			{
				m_event.ResetContent();
				pSequence->ClearTrackEvents();

				// Copy over track events and repopulate list
				TrackEvents::iterator itEvent = dlg.m_events.begin();
				TrackEvents::iterator itEventEnd = dlg.m_events.end();
				for (; itEvent != itEventEnd; ++itEvent)
				{
					const char* szEvent = itEvent->c_str();
					pSequence->AddTrackEvent(szEvent);
					m_event.AddString(szEvent);
				}

				// Reselect event (if it still exists)
				IEventKey key;
				m_track->GetKey(m_key, &key);
				m_event.SelectString(-1, key.event);
			}
		}
	}
}
