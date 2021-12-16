// DimensionsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "DimensionsDialog.h"

/////////////////////////////////////////////////////////////////////////////
// CDimensionsDialog dialog


CDimensionsDialog::CDimensionsDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CDimensionsDialog::IDD, pParent)
	, m_bDebugTexture(FALSE)
{
	m_bQuality = FALSE;
	m_bUpdateIL = FALSE;
	m_pText=0;
	//{{AFX_DATA_INIT(CDimensionsDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDimensionsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDimensionsDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Radio(pDX, IDC_512, m_iSelection);
	DDX_Check(pDX, IDC_CHECK1, m_bQuality);
	DDX_Check(pDX, IDC_UPDATE_IL, m_bUpdateIL);
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_DEBUGTEXTURE, m_bDebugTexture);
}


BEGIN_MESSAGE_MAP(CDimensionsDialog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	//{{AFX_MSG_MAP(CDimensionsDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDimensionsDialog message handlers


//////////////////////////////////////////////////////////////////////////
BOOL CDimensionsDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	if(m_pText)
		SetDlgItemText(IDC_STATIC_TEXT, m_pText);

	m_fBrMultiplier.Create( this, IDC_COLOR_MULT );
	//m_fBrMultiplier.SetRange( 1, 16);
	m_fBrMultiplier.SetRange( 1, 32);
	m_fBrMultiplier.SetValue( gSettings.fBrMultiplier );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


//////////////////////////////////////////////////////////////////////////
void CDimensionsDialog::SetDimensions(UINT iWidth)
{
	////////////////////////////////////////////////////////////////////////
	// Select a dimension option button in the dialog
	////////////////////////////////////////////////////////////////////////

	switch (iWidth)
	{
	case 512:
		m_iSelection = 0;
		break;

	case 1024:
		m_iSelection = 1;
		break;

	case 2048:
		m_iSelection = 2;
		break;

	case 4096:
		m_iSelection = 3;
		break;

	case 8192:
		m_iSelection = 4;
		break;
//Diesel cut
	//case 16384:
	//	m_iSelection = 5;
//Diesel new
	case 32768:
		m_iSelection = 6;

		break;

	default:
		ASSERT(FALSE);
		break;
	}

	// Update the controls with the new selection
	if (m_hWnd)
		UpdateData(FALSE);
}

UINT CDimensionsDialog::GetDimensions()
{
	////////////////////////////////////////////////////////////////////////
	// Get the currently selected dimension option button in the dialog
	////////////////////////////////////////////////////////////////////////

	// Update the member variables with the current 
	// selection from the controls
	if (m_hWnd)
		UpdateData(TRUE);

	// Identify the currently activated radio button
	switch (m_iSelection)
	{
	case 0:
		return 512;

	case 1:
		return 1024;

	case 2:
		return 2048;

	case 3:
		return 4096;

	case 4:
		return 8192;
//Diesel cut
	//case 5:
	//	return 16384;
//Diesel new
	case 6:
		return 32768;

	default:
		ASSERT(FALSE);
		break;
	}

	return 0;
}


void CDimensionsDialog::SetText(const char * pText)
{
	m_pText = pText;
}


//////////////////////////////////////////////////////////////////////////
void CDimensionsDialog::OnBnClickedOk()
{
	gSettings.fBrMultiplier = m_fBrMultiplier.GetValue();
	gSettings.Save();
	OnOK();
}


//////////////////////////////////////////////////////////////////////////
float CDimensionsDialog::GetBrMultiplier()
{
	return gSettings.fBrMultiplier;
}
