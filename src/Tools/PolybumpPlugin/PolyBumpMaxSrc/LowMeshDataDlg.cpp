// LowMeshDataDlg.cpp : implementation file
//

#include "stdafx.h"
//#include "polybumpplugin.h"
#include "LowMeshDataDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLowMeshDataDlg dialog


CLowMeshDataDlg::CLowMeshDataDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLowMeshDataDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLowMeshDataDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
} 


void CLowMeshDataDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLowMeshDataDlg)
	DDX_Control(pDX, IDC_CHANNELNO, m_ChannelNo);
	DDX_Control(pDX, IDC_MATERIALID, m_MaterialId);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLowMeshDataDlg, CDialog)
	//{{AFX_MSG_MAP(CLowMeshDataDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLowMeshDataDlg message handlers

BOOL CLowMeshDataDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	for(int i=1;i<100;i++)
	{
		char str[80];

		sprintf(str,"%.2d",i);
		m_ChannelNo.AddString(str);
	}

	m_MaterialId.AddString("All");
	
	for(int i=1;i<100;i++)
	{
		char str[80];

		sprintf(str,"%.2d",i);
		m_MaterialId.AddString(str);
	}

	m_ChannelNo.SetCurSel(0);			// default: channel no 1
	m_MaterialId.SetCurSel(0);		// default: all Material IDs

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLowMeshDataDlg::OnOK()
{
	// TODO: Add extra validation here
	UpdateData();

	m_iChannelNo=m_ChannelNo.GetCurSel()+1;			// channel 0 is not used
	m_iMaterialId=m_MaterialId.GetCurSel()-1;		// internally in MAX MaterialID 0 means 1 in User Interface
	
	CDialog::OnOK();
}
