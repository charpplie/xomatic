// SegmentedWorldNewWorldDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SegmentedWorldNewWorldDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CSWHistorySelectWorldDlg dialog

CSWNewWorldDlg::CSWNewWorldDlg(CWnd* pParent, const char *title)
	: CDialog(CSWNewWorldDlg::IDD, pParent)
{
	if (title)
		m_title = title;
	m_nLevelListType = sw::LevelList_SW;
	m_strSelectedWorld.clear();
}

CSWNewWorldDlg::~CSWNewWorldDlg()
{	
}

void CSWNewWorldDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control( pDX, IDC_EDIT_SW_NEW_NAME, m_worldNameEditBox );
}


BEGIN_MESSAGE_MAP(CSWNewWorldDlg, CDialog)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDOK, &CSWNewWorldDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSWNewWorldDlg::OnBnClickedCancel)
	ON_EN_UPDATE(IDC_EDIT_SW_NEW_NAME, &CSWNewWorldDlg::OnEnUpdateEditSwNewName)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSWHistorySelectWorldDlg message handlers

BOOL CSWNewWorldDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if (!m_title.IsEmpty())
		SetWindowText( m_title );

	CSWManager::ListWorlds(m_lstWorldsNames, m_nLevelListType);
	
	
	return TRUE;
}

void CSWNewWorldDlg::OnBnClickedOk()
{
	CString worldName;
	m_worldNameEditBox.GetWindowText( worldName );
	m_strSelectedWorld = worldName;
	if (m_strSelectedWorld.empty())
	{
		AfxMessageBox( _T("Must Write name for World"), MB_ICONEXCLAMATION|MB_OK );
		return;
	}
	
	if (!CheckName())
	{
		AfxMessageBox( _T("World Exists"), MB_ICONEXCLAMATION|MB_OK );
		return;
	}
	// TODO: Add your control notification handler code here
	OnOK();
}

void CSWNewWorldDlg::OnBnClickedCancel()
{
	m_strSelectedWorld.clear();
	// TODO: Add your control notification handler code here
	OnCancel();
}

bool CSWNewWorldDlg::CheckName()
{
	if (m_strSelectedWorld.empty())
		return false;
	for (int i = 0; i < m_lstWorldsNames.size(); i++)
	{
		if (!m_strSelectedWorld.compareNoCase(m_lstWorldsNames[i].Name.c_str()))
			return false;
	}
	return true;
}
void CSWNewWorldDlg::OnEnUpdateEditSwNewName()
{
	CString str;
	str = "qwertyuiopasdfghjklzxcvbnm1234567890_";
	CString worldName;
	CString resultName;
	m_worldNameEditBox.GetWindowText( worldName );
	resultName = worldName;
	bool bUpdated = false;
	for (int i = 0; i < worldName.GetLength(); i++)
	{
		if (str.Find(worldName.GetAt(i)) < 0)
		{
			resultName.Trim(worldName.GetAt(i));
			bUpdated = true;
		}
	}
	if (bUpdated)
	{
		m_worldNameEditBox.SetWindowText( resultName );
		m_worldNameEditBox.SetSel(0xFFFF,0xFFFF);
	}
	
}
