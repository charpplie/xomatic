#include "stdafx.h"														// precompiled headers

#ifdef STRICT																	// Already defined by stdafx,
#undef STRICT																	// so we avoid warning linking msg
#endif
#ifdef _MBCS																	// The same as above
#undef _MBCS
#endif

#include "Max.h"
#include "PolyBumpPlugin.h"
#include "About.h"														// CABout
#include "../KeyGen.h"												// Serial No processing 
#include "../CopyProtection.h"								// ChangeSerialYesNo(),GetSerialFromRegistry(),HowToGetSerialDialog()
#include "about.h"														// CAbout
#include "credits.h"													// CCredits
#include "ResourceCompilerHelper.h"						// CResourceCompilerHelper
#include "about.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif







/////////////////////////////////////////////////////////////////////////////
// CAbout dialog


CAbout::CAbout(CWnd* pParent /*=NULL*/)
	: CDialog(CAbout::IDD, pParent)
	, m_sSerialNo(_T("")) 
{
	//{{AFX_DATA_INIT(CAbout)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CAbout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAbout)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_SERIAL, m_sSerialNo);
}


BEGIN_MESSAGE_MAP(CAbout, CDialog)
	//{{AFX_MSG_MAP(CAbout)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_GETSERIALFROMCRYTEK, OnBnClickedGetserialfromcrytek)
	ON_BN_CLICKED(IDC_CREDITS, OnBnClickedCredits)
	ON_BN_CLICKED(IDC_SETUPRC, OnBnClickedSetuprc)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAbout message handlers








void CAbout::OnOK()
{
	UpdateData();

#ifdef USE_COPYPROTECTION
	ChangeSerialYesNo(m_sSerialNo.GetBuffer(256));
#endif

	EndDialog(1);
}



BOOL CAbout::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_sSerialNo=g_szSerialNo;

#ifndef USE_COPYPROTECTION
	CWnd *dlgitem1=GetDlgItem(IDC_GETSERIALFROMCRYTEK);
	dlgitem1->ShowWindow(SW_HIDE);
	CWnd *dlgitem2=GetDlgItem(IDC_SERIAL);
	dlgitem2->ShowWindow(SW_HIDE);
	CWnd *dlgitem3=GetDlgItem(IDC_SERIAL_TEXT);
	dlgitem3->ShowWindow(SW_HIDE);
//	CWnd *dlgitem4=GetDlgItem(IDOK);
//	dlgitem4->ShowWindow(SW_HIDE);
#else
//	CWnd *dlgitem=GetDlgItem(IDC_NONSERIALVERSION);				assert(dlgitem);
//	if(dlgitem)dlgitem->ShowWindow(SW_HIDE);
#endif

	UpdateData(false);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CAbout::OnBnClickedGetserialfromcrytek()
{
	HowToGetSerialDialog();
}

void CAbout::OnBnClickedCredits()
{
	CCredits dlg;
	dlg.DoModal();
}

void CAbout::OnBnClickedSetuprc()
{
	CResourceCompilerHelper rc;

	rc.ResourceCompilerUI(GetSafeHwnd());
}

