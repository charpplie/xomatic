#include "StdAfx.h"
#include "ErrorsDlg.h"

IMPLEMENT_DYNAMIC(CErrorsDlg, CDialog)

CErrorsDlg::CErrorsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CErrorsDlg::IDD, pParent)
{
	m_bFirstMessage = true;

	Create(CErrorsDlg::IDD, this);
}

CErrorsDlg::~CErrorsDlg()
{
	DestroyWindow();
}

void CErrorsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ERROR_ICON, m_errorIconCtrl);
	DDX_Control(pDX, IDC_ERRORS, m_richEdit);
}

BEGIN_MESSAGE_MAP(CErrorsDlg, CDialog)
END_MESSAGE_MAP()

BOOL CErrorsDlg::OnInitDialog()
{
	__super::OnInitDialog();
	
	HICON hIconInfo = LoadIcon( NULL, IDI_HAND);
	m_errorIconCtrl.SetIcon(hIconInfo);

	return TRUE;
}

void CErrorsDlg::AddMessage(const CString& text, const CString& caption)
{
	// At the load time this dialog is frozen, cause there is no message loop in progress.
	// We need to dispatch messages before showing a window if it was closed by user.
	ProcessingMessages();

	if (!IsWindowVisible())
	{
		ShowWindow(SW_SHOW);
	}

	long textLength = m_richEdit.GetTextLength();
	m_richEdit.SetSel(textLength-1,textLength-1);

	if (m_bFirstMessage)
	{
		m_bFirstMessage = false;
	}
	else
	{
		m_richEdit.ReplaceSel("\n\n");
	}

	CHARFORMAT cf;
	cf.dwMask = CFM_BOLD;
	cf.dwEffects = CFE_BOLD;
	m_richEdit.SetSelectionCharFormat(cf);
	m_richEdit.ReplaceSel(caption + "\n");
	cf.dwEffects = 0;
	m_richEdit.SetSelectionCharFormat(cf);
	m_richEdit.ReplaceSel(text);
	// Show message in a dialog
	ProcessingMessages();
}

void CErrorsDlg::ProcessingMessages()
{
	MSG msg;

	while (FALSE != ::PeekMessage( &msg, 0, 0, 0, PM_REMOVE))
	{ 
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
}

void CErrorsDlg::OnCancel()
{
	ShowWindow(SW_HIDE);
}