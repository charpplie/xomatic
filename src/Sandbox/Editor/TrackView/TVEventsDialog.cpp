////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TVEventsDialog.h
//  Version:     v1.00
//  Created:     3/4/2008 by Kevin.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TVEventsDialog.h"
#include "..\StringDlg.h"


// CTVEventsDialog dialog

IMPLEMENT_DYNAMIC(CTVEventsDialog, CDialog)

CTVEventsDialog::CTVEventsDialog(IAnimSequence *pTrack, CWnd* pParent /*=NULL*/)
	: CDialog(CTVEventsDialog::IDD, pParent),
	m_pTrack(pTrack)
{

}

CTVEventsDialog::~CTVEventsDialog()
{
}

void CTVEventsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EVENTS_LIST, m_List);
}


BEGIN_MESSAGE_MAP(CTVEventsDialog, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_ADDEVENT, &CTVEventsDialog::OnBnClickedButtonAddevent)
	ON_BN_CLICKED(IDC_BUTTON_REMOVEEVENT, &CTVEventsDialog::OnBnClickedButtonRemoveevent)
END_MESSAGE_MAP()


// CTVEventsDialog message handlers

void CTVEventsDialog::OnBnClickedButtonAddevent()
{
	CStringDlg dlg( _T("Track Event Name") );
	if (dlg.DoModal() == IDOK && !dlg.GetString().IsEmpty())
	{
		CString add = dlg.GetString();

		// Make sure it doesn't already exist
		LVFINDINFO find;
		find.flags = LVFI_STRING|LVFI_WRAP;
		find.psz = add;
		if (-1 == m_List.FindItem(&find))
		{
			m_List.InsertItem(m_List.GetItemCount(), add);
		}
	}
}

void CTVEventsDialog::OnBnClickedButtonRemoveevent()
{
	while (true)
	{
		POSITION pos = m_List.GetFirstSelectedItemPosition();
		if (!pos) break;
		m_List.DeleteItem(m_List.GetNextSelectedItem(pos));
	}
}

BOOL CTVEventsDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_events.resize(0);

	// Push existing items into list
	assert(m_pTrack);
	const int iCount = m_pTrack->GetTrackEventsCount();
	for (int i = 0; i < iCount; ++i)
	{
		m_List.InsertItem(i, m_pTrack->GetTrackEvent(i));
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CTVEventsDialog::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class
	int count = m_List.GetItemCount();
	for (int i = 0; i < count; ++i)
	{
		m_events.push_back(m_List.GetItemText(i,0).GetString());
	}

	CDialog::OnOK();
}
