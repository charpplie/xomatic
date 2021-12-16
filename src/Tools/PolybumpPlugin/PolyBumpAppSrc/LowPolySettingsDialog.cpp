// LowPolySettingsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "PolyBumpApplication.h"
#include "LowPolySettingsDialog.h"
#include "../SimpleIndexedMesh.h"						// CSimpleIndexedMesh
#include ".\lowpolysettingsdialog.h"


// CLowPolySettingsDialog dialog

IMPLEMENT_DYNAMIC(CLowPolySettingsDialog, CDialog)
CLowPolySettingsDialog::CLowPolySettingsDialog( CSimpleIndexedMesh &rMesh, CWnd* pParent /*=NULL*/)
	: CDialog(CLowPolySettingsDialog::IDD, pParent), m_rMesh(rMesh)
{
}

CLowPolySettingsDialog::~CLowPolySettingsDialog()
{
}

BOOL CLowPolySettingsDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	{
		m_LowPolyMaterial.AddString("<All>");

		for(uint32 dwM=0;dwM<m_rMesh.m_MaterialCount;++dwM)
			m_LowPolyMaterial.AddString(m_rMesh.m_pMaterials[dwM].m_sMaterialName.c_str());

		m_LowPolyMaterial.SelectString(0,"<All>");
	}

	return TRUE;
}

CString CLowPolySettingsDialog::GetMaterial() const
{
	return m_sMaterial;
}

void CLowPolySettingsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOWPOLYMATERIAL, m_LowPolyMaterial);
}


BEGIN_MESSAGE_MAP(CLowPolySettingsDialog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CLowPolySettingsDialog message handlers

void CLowPolySettingsDialog::OnBnClickedOk()
{
	int iSel = m_LowPolyMaterial.GetCurSel();

	if(iSel!=-1)
		m_LowPolyMaterial.GetLBText(iSel,m_sMaterial);

	OnOK();
}
