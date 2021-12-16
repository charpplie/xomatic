// ReconstructionOptionsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "PhotoBump10.h"
#include "ReconstructionOptionsDialog.h"
#include ".\reconstructionoptionsdialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CReconstructionOptionsDialog dialog

IMPLEMENT_DYNAMIC(CReconstructionOptionsDialog, CDialog)
CReconstructionOptionsDialog::CReconstructionOptionsDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CReconstructionOptionsDialog::IDD, pParent)
{
	m_fNormalMapScale=1.0;
	m_fDetailScale=1.0;
	m_bShowFeaturesOnly=0;
	m_bUseSingleStereoPhoto=0;
	m_bUseDoubleStereoPhoto=0;
}

CReconstructionOptionsDialog::~CReconstructionOptionsDialog()
{
}

void CReconstructionOptionsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//DDX_Radio(pDX,IDC_RADIO7,m_nGeomType);	
	//DDX_Check(pDX,IDC_CHECK2,m_nGenerateHeighMapFromHiRes);
	//DDX_Check(pDX,IDC_CHECK3,m_nGenerateLowRes);
	//DDX_Check(pDX,IDC_CHECK4,m_nExportHiResObj);

	DDX_Text(pDX, IDC_EDIT1, m_fNormalMapScale);
  DDV_MinMaxDouble(pDX, m_fNormalMapScale, 0.0, 100.0); 
	DDX_Text(pDX, IDC_EDIT2, m_fDetailScale);
	DDV_MinMaxDouble(pDX, m_fDetailScale, 0.0, 100.0); 
	DDX_Check(pDX,IDC_CHECK1,m_bShowFeaturesOnly);

	DDX_Check(pDX,IDC_CHECK2,m_bUseSingleStereoPhoto);
	DDX_Check(pDX,IDC_CHECK3,m_bUseDoubleStereoPhoto);
}

BEGIN_MESSAGE_MAP(CReconstructionOptionsDialog, CDialog)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButton3)
END_MESSAGE_MAP()


// CReconstructionOptionsDialog message handlers

void CReconstructionOptionsDialog::OnBnClickedButton2()
{
	// TODO: Add your control notification handler code here
	CDialog::OnOK();
}

void CReconstructionOptionsDialog::OnBnClickedButton3()
{
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}
