#include "stdafx.h"
#include "EngineSettingsManager.h"
#include "RCDialog.h"


IMPLEMENT_DYNAMIC(CRCDialog, CDialog)


//////////////////////////////////////////////////////////////////////////
CRCDialog::CRCDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CRCDialog::IDD, pParent)
{
}


//////////////////////////////////////////////////////////////////////////
void CRCDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SHOW_WINDOW_CB, m_bShowWindow);
	DDX_Control(pDX, IDC_HIDE_CUSTOM_CB, m_bHideCustom);
	DDX_Control(pDX, IDC_PARAMS_EDIT, m_eParams);
}


//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CRCDialog, CDialog)
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
void CRCDialog::Create(UINT temp, CWnd* parent, CEngineSettingsManager* settingsManager)
{
	m_settingsManager = settingsManager;
	CDialog::Create(temp, parent);
}


//////////////////////////////////////////////////////////////////////////
BOOL CRCDialog::OnInitDialog()
{
	CDialog::OnInitDialog();	
	m_bShowWindow.SetCheck(m_settingsManager->GetValue<string>("RC_ShowWindow")=="true" ? BST_CHECKED : BST_UNCHECKED);
	m_bHideCustom.SetCheck(m_settingsManager->GetValue<string>("RC_HideCustom")=="true" ? BST_CHECKED : BST_UNCHECKED);
	m_eParams.SetWindowText(m_settingsManager->GetValue<string>("RC_Parameters").c_str());
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
bool CRCDialog::IsShowWindow()
{
	return m_bShowWindow.GetCheck() == BST_CHECKED;
}


//////////////////////////////////////////////////////////////////////////
bool CRCDialog::IsHideCustom()
{
	return m_bHideCustom.GetCheck() == BST_CHECKED;
}


//////////////////////////////////////////////////////////////////////////
CString CRCDialog::GetParams()
{
	CString s;
	m_eParams.GetWindowText(s);
	return s;
}
