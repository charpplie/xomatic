////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   shapepanel.cpp
//  Version:     v1.00
//  Created:     28/2/2002 by Timur.
//  Compilers:   Visual C++.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AIWavePanel.h"
#include "Objects/AIWave.h"


BEGIN_MESSAGE_MAP(CAIWavePanel, CEntityPanel)
	ON_BN_CLICKED(IDC_PROTOTYPE, OnSelectAssignedAIs)

	ON_BN_CLICKED(IDC_OPENFLOWGRAPH, OnBnClickedOpenFlowGraph)
	ON_BN_CLICKED(IDC_LIST_ENTITY_FLOWGRAPHS, OnBnClickedListFlowGraphs)
	ON_BN_CLICKED(IDC_REMOVEFLOWGRAPH, OnBnClickedRemoveFlowGraph)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CAIWavePanel, CEntityPanel)

//////////////////////////////////////////////////////////////////////////
CAIWavePanel::CAIWavePanel(CWnd* pParent /* = NULL */)
	: CEntityPanel(CAIWavePanel::IDD, pParent)
{
}

void CAIWavePanel::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_EDITSCRIPT,   m_editScriptButton);
	DDX_Control(pDX, IDC_RELOADSCRIPT, m_reloadScriptButton);

	DDX_Control(pDX, IDC_PROTOTYPE,    m_SelectAssignedAIsButton);
	
	DDX_Control(pDX, IDC_OPENFLOWGRAPH,          m_flowGraphOpenBtn);
	DDX_Control(pDX, IDC_REMOVEFLOWGRAPH,        m_flowGraphRemoveBtn);
	DDX_Control(pDX, IDC_LIST_ENTITY_FLOWGRAPHS, m_flowGraphListBtn);
}

BOOL CAIWavePanel::OnInitDialog() 
{
	CXTResizeDialog::OnInitDialog();

	SetResize(IDC_FRAME1,      SZ_HORRESIZE(1));
	SetResize(IDC_FRAME2,      SZ_HORRESIZE(1));
	SetResize(IDC_FRAME3,      SZ_HORRESIZE(1));
	SetResize(IDC_PROTOTYPE,   SZ_HORRESIZE(1));
	SetResize(IDC_SCRIPT_NAME, SZ_HORRESIZE(1));

	return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////
void CAIWavePanel::OnSelectAssignedAIs()
{
	GetIEditor()->GetObjectManager()->SelectAssignedEntities();
}

void CAIWavePanel::UpdateAssignedAIsPanel()
{
	size_t nAssignedAIs = GetIEditor()->GetObjectManager()->NumberOfAssignedEntities();
	
	char buffer[100];
	sprintf(buffer, "Assigned AIs: %u", nAssignedAIs);
	SetDlgItemText(IDC_FRAME2, buffer);
	
	m_SelectAssignedAIsButton.EnableWindow(nAssignedAIs);
}
