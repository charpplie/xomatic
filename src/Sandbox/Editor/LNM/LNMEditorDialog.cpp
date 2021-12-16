/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:  This is the implementation file for the module LNMEditorDialog. 
The LNMEditorDialog is a framewindow containing all the other windows 
related to useful shortcuts about LNM. It also serves as a hub and manager
for this windows.
-------------------------------------------------------------------------
History:
- 18:01:2010   10:51 : Created by Ricardo Pillosu

*************************************************************************/
#include "stdafx.h"

#include "LNMEditorDialog.h"

// CTextureBrowserDialog dialog
IMPLEMENT_DYNCREATE(CLNMEditorDialog,CBaseFrameWnd)

BEGIN_MESSAGE_MAP(CLNMEditorDialog, CBaseFrameWnd)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CLNMEditorDialog::CLNMEditorDialog() : 
m_poDockingPane(NULL)
{
	CRect rc(0,0,0,0);
	Create( WS_CHILD|WS_VISIBLE,rc,AfxGetMainWnd() );
}
//////////////////////////////////////////////////////////////////////////
CLNMEditorDialog::~CLNMEditorDialog()
{

}
//////////////////////////////////////////////////////////////////////////
void CLNMEditorDialog::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CLNMEditorClass());
}
//////////////////////////////////////////////////////////////////////////
//void CLNMEditorDialog::OnNotifySelectionChange(TDLNMPrototypes& rcLNMPrototypes)
//{
//  m_poLNMEditorPropertiesPanel->OnNotifySelectionChange(rcLNMPrototypes);
//}
//////////////////////////////////////////////////////////////////////////
void CLNMEditorDialog::DoDataExchange(CDataExchange* pDX)
{
	CBaseFrameWnd::DoDataExchange(pDX);
}
//////////////////////////////////////////////////////////////////////////
LRESULT CLNMEditorDialog::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == XTP_DPN_SHOWWINDOW)
	{
		return TRUE;
	}
	else if (wParam == XTP_DPN_CLOSEPANE)
	{
	}
	return FALSE;
}
//////////////////////////////////////////////////////////////////////////
BOOL CLNMEditorDialog::OnInitDialog()
{
	// Create the docking pane
//Diesel cut	
//m_poDockingPane = GetDockingPaneManager()->CreatePane( IDC_OPERATIONS_DOCKING_PANE, CRect(0,0,220,200), dockLeftOf );
//Diesel new
	m_poDockingPane = GetDockingPaneManager()->CreatePane( IDC_OPERATIONS_DOCKING_PANE, CRect(0,0,220,200), xtpPaneDockLeft );


	// We MUST use the ID AFX_IDW_STATUS_BAR if we are speaking about the status bar.
	UINT indicators[] =
	{
		AFX_IDW_STATUS_BAR           
	};

	m_oWindowStatusBar.Create( this, WS_CHILD|WS_VISIBLE|CBRS_BOTTOM);
	m_oWindowStatusBar.SetIndicators(indicators,1);
	SetStatusBarText("LNM ready");

	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
void CLNMEditorDialog::OnDestroy()
{

}
//////////////////////////////////////////////////////////////////////////
void CLNMEditorDialog::OnClose()
{
	DestroyWindow();
}
//////////////////////////////////////////////////////////////////////////
void CLNMEditorDialog::SetStatusBarText(const char* pText)
{
	assert(pText != NULL);
	m_oWindowStatusBar.SetPaneText(0, pText);
}