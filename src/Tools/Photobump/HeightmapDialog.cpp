// HeightmapDialog.cpp : implementation file
//

#include "stdafx.h"
#include "PhotoBump10.h"
#include "HeightmapDialog.h"
#include ".\heightmapdialog.h"


// CHeightmapDialog dialog

IMPLEMENT_DYNAMIC(CHeightmapDialog, CDialog)
CHeightmapDialog::CHeightmapDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CHeightmapDialog::IDD, pParent)
{
	m_nIterations=1000;
}

CHeightmapDialog::~CHeightmapDialog()
{
}

void CHeightmapDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_EDIT1, m_nIterations);
	DDV_MinMaxInt(pDX, m_nIterations, 1, 100000); 
}


BEGIN_MESSAGE_MAP(CHeightmapDialog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// CHeightmapDialog message handlers

void CHeightmapDialog::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	OnOK();
}

void CHeightmapDialog::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}
