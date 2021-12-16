////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "SoundAssetDbFilterDlg.h"
#include "IAssetViewer.h"

// CSoundAssetDbFilterDlg dialog

IMPLEMENT_DYNAMIC(CSoundAssetDbFilterDlg, CDialog)

CSoundAssetDbFilterDlg::CSoundAssetDbFilterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSoundAssetDbFilterDlg::IDD, pParent)
{
}

CSoundAssetDbFilterDlg::~CSoundAssetDbFilterDlg()
{
}

void CSoundAssetDbFilterDlg::UpdateFilterUI()
{
	auto filters = m_pAssetViewer->GetCurrentFilters();

	{
		SAssetField& field = filters["length"];

		m_cbMinLen.SelectString(-1, field.m_filterValue);
		m_cbMaxLen.SelectString(-1, field.m_maxFilterValue);
	}

	{
		SAssetField& field = filters["loopsound"];

		CheckDlgButton(IDC_CHECK_LOOPING_SOUNDS, (field.m_filterValue == "Yes"));
	}
}

void CSoundAssetDbFilterDlg::ApplyFilter()
{
	auto filters = m_pAssetViewer->GetCurrentFilters();
	CString str;

	{
		SAssetField& field = filters["length"];

		field.m_fieldName = "length";
		field.m_filterCondition = SAssetField::eCondition_InsideRange;
		m_cbMinLen.GetLBText(m_cbMinLen.GetCurSel(), str);
		field.m_filterValue = str;
		m_cbMaxLen.GetLBText(m_cbMaxLen.GetCurSel(), str);
		field.m_maxFilterValue = str;
		field.m_fieldType = SAssetField::eType_Int32;
	}

	if (IsDlgButtonChecked(IDC_CHECK_LOOPING_SOUNDS))
	{
		SAssetField& field = filters["loopsound"];
		
		field.m_fieldName = "loopsound";
		field.m_filterCondition = SAssetField::eCondition_Equal;
		field.m_filterValue = "Yes";
		field.m_fieldType = SAssetField::eType_Bool;
	}
	else
	{
		auto iter = filters.find("loopsound");

		if (iter != filters.end())
		{
			filters.erase(iter);
		}
	}

	m_pAssetViewer->ApplyFilters(filters);
}

void CSoundAssetDbFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_MINIMUM_LENGTH, m_cbMinLen);
	DDX_Control(pDX, IDC_COMBO_MAXIMUM_LENGTH, m_cbMaxLen);
}


BEGIN_MESSAGE_MAP(CSoundAssetDbFilterDlg, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_MINIMUM_LENGTH, &CSoundAssetDbFilterDlg::OnCbnSelchangeComboMinimumLength)
	ON_CBN_SELCHANGE(IDC_COMBO_MAXIMUM_LENGTH, &CSoundAssetDbFilterDlg::OnCbnSelchangeComboMaximumLength)
	ON_BN_CLICKED(IDC_CHECK_LOOPING_SOUNDS, &CSoundAssetDbFilterDlg::OnBnClickedCheckLoopingSounds)
END_MESSAGE_MAP()


// CSoundAssetDbFilterDlg message handlers

void CSoundAssetDbFilterDlg::OnCbnSelchangeComboMinimumLength()
{
	if (m_cbMinLen.GetCurSel() > m_cbMaxLen.GetCurSel())
	{
		m_cbMinLen.SetCurSel(m_cbMaxLen.GetCurSel());
		return;
	}

	ApplyFilter();
}

void CSoundAssetDbFilterDlg::OnCbnSelchangeComboMaximumLength()
{
	if (m_cbMaxLen.GetCurSel() < m_cbMinLen.GetCurSel())
	{
		m_cbMaxLen.SetCurSel(m_cbMinLen.GetCurSel());
		return;
	}

	ApplyFilter();
}

void CSoundAssetDbFilterDlg::OnBnClickedCheckLoopingSounds()
{
	ApplyFilter();
}

BOOL CSoundAssetDbFilterDlg::OnInitDialog()
{
	__super::OnInitDialog();

	m_cbMinLen.SetCurSel(0);
	m_cbMaxLen.SetCurSel(m_cbMaxLen.GetCount() - 1);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
