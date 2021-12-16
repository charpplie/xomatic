// SegmentedWorldCommitDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SegmentedWorldCommitDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CSWCommitDlg dialog

CSWCommitDlg::CSWCommitDlg(CWnd* pParent, const char *title)
	: CDialog(CSWCommitDlg::IDD, pParent)
{
	if (title)
		m_title = title;
	
}

CSWCommitDlg::~CSWCommitDlg()
{	
}

void CSWCommitDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control( pDX, IDC_EDIT_SW_COMMIT_DESCR, m_EditDescription );
	DDX_Control( pDX, IDC_CHECK_SW_COMMIT, m_cbEnableDescr );
}


BEGIN_MESSAGE_MAP(CSWCommitDlg, CDialog)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDOK, &CSWCommitDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSWCommitDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_CHECK_SW_COMMIT, &CSWCommitDlg::OnBnClickedCheckSwCommit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSWHistorySelectWorldDlg message handlers

BOOL CSWCommitDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if (!m_title.IsEmpty())
		SetWindowText( m_title );

	m_cbEnableDescr.SetCheck(FALSE);
	m_EditDescription.EnableWindow(FALSE);
	
	return TRUE;
}

void CSWCommitDlg::OnBnClickedOk()
{
	if (m_cbEnableDescr.GetCheck())
	{
		CString strDescr;
		m_EditDescription.GetWindowText( strDescr );
		m_strDescription = strDescr;
		if (m_strDescription.length() > 1024)
		{
			m_strDescription = m_strDescription.substr(0, 1024);
		}
		
		if (m_strDescription.length() == 0)
			m_strDescription = "<none>";

	}
	else
		m_strDescription = "<none>";
	// TODO: Add your control notification handler code here
	OnOK();
}

void CSWCommitDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

const char* CSWCommitDlg::GetDescription()
{
	return m_strDescription.c_str();
}
void CSWCommitDlg::OnBnClickedCheckSwCommit()
{
	m_EditDescription.EnableWindow(m_cbEnableDescr.GetCheck());
}
