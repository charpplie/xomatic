////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   TelemetryDialog.cpp
//  Version:     v1.00
//  Created:     17/12/2009 by Sergey Mikhtonyuk
//  Description: Implementaiton of telemetry dialog
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "Resource.h"
#include "TelemetryDialog.h"
#include "TelemetryViewClass.h"

//////////////////////////////////////////////////////////////////////////

#define TELEM_DIALOGFRAME_CLASSNAME "TelemetryDialog"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CTelemetryDialog, CWnd)

BEGIN_MESSAGE_MAP(CTelemetryDialog, CWnd)
	ON_WM_SIZE()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CTelemetryDialog::CTelemetryDialog()
: m_connectionPanel(m_repository)
{
	Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 1, 1), AfxGetMainWnd(), 0);
	
	m_rollupCtrl.Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 1, 1), this, NULL);

	m_connectionPanel.Create(CConnectionPanel::IDD, &m_rollupCtrl);
	m_rollupCtrl.InsertPage("StatsTool", &m_connectionPanel);
}

//////////////////////////////////////////////////////////////////////////

CTelemetryDialog::~CTelemetryDialog()
{
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryDialog::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	RECT rcRollUp;
	GetClientRect(&rcRollUp);

	if (m_rollupCtrl.m_hWnd)
	{
		m_rollupCtrl.MoveWindow(rcRollUp.left, rcRollUp.top, rcRollUp.right, rcRollUp.bottom, FALSE);
	}
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryDialog::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CTelemetryViewClass );
}

//////////////////////////////////////////////////////////////////////////


