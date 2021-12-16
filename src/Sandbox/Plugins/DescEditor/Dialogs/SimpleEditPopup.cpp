// SimpleEditPopup.cpp : implementation file
//

#include "pch.h"
#include "SimpleEditPopup.h"
#include "afxdialogex.h"


using namespace CryGame;


// CSimpleEditPopup dialog

IMPLEMENT_DYNAMIC(CSimpleEditPopup, CXTResizeDialog)

CSimpleEditPopup::CSimpleEditPopup(CWnd* pParent /*=NULL*/)
	: CXTResizeDialog(CSimpleEditPopup::IDD, pParent)
	, m_title(NULL)
	, m_label(NULL)
	, m_editString(NULL)
	, m_nEditWidth(0)
	, m_nEditHeight(0)
{

}

CSimpleEditPopup::~CSimpleEditPopup()
{
}

void CSimpleEditPopup::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSimpleEditPopup, CXTResizeDialog)
END_MESSAGE_MAP()

// CSimpleEditPopup message handlers
BOOL CSimpleEditPopup::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();

	SetWindowTextA(m_title);
	SetDlgItemText(IDC_SIMPLE_LABEL, m_label);

	// Set initial text to that of the return value
	SetDlgItemText(IDC_SIMPLE_EDITBOX, m_editString->c_str());

	// Setup anchoring
	SetResize(IDC_SIMPLE_LABEL, SZ_HORRESIZE(1));
	SetResize(IDC_SIMPLE_EDITBOX, SZ_RESIZE(1));
	SetResize(IDOK, SZ_REPOS(1));
	SetResize(IDCANCEL, SZ_REPOS(1));
	SetResize(IDC_STATIC, CXTResizeRect(0,1,1,1));

	// Adjust window size
	CRect rc;
	GetWindowRect(rc);
	SetWindowPos(&CWnd::wndTopMost, rc.left, rc.top, rc.Width() + m_nEditWidth, rc.Height() + m_nEditHeight, 0);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSimpleEditPopup::OnOK()
{
	if (m_editString)
	{
		CString text;
		GetDlgItemText(IDC_SIMPLE_EDITBOX, text);
		*m_editString = text;
	}

	CXTResizeDialog::OnOK();
}

