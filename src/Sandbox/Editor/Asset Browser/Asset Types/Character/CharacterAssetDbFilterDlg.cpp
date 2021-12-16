////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CharacterAssetDbFilterDlg.h"
#include "IAssetViewer.h"

// CCharacterAssetDbFilterDlg dialog

IMPLEMENT_DYNAMIC(CCharacterAssetDbFilterDlg, CDialog)

CCharacterAssetDbFilterDlg::CCharacterAssetDbFilterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCharacterAssetDbFilterDlg::IDD, pParent)
{

}

CCharacterAssetDbFilterDlg::~CCharacterAssetDbFilterDlg()
{
}

void CCharacterAssetDbFilterDlg::UpdateFilterUI()
{
	auto filters = m_pAssetViewer->GetCurrentFilters();
	CString str;

	{
		SAssetField& field = filters["trianglecount"];

		m_cbMinTris.SelectString(-1, field.m_filterValue);
		m_cbMaxTris.SelectString(-1, field.m_maxFilterValue);
	}

	{
		SAssetField& field = filters["mtlcount"];

		m_cbMinMtls.SelectString(-1, field.m_filterValue);
		m_cbMaxMtls.SelectString(-1, field.m_maxFilterValue);
	}

	{
		SAssetField& field = filters["islod"];

		CheckDlgButton(IDC_CHECK_HIDE_LODS, field.m_filterValue == "No");
	}
}

void CCharacterAssetDbFilterDlg::ApplyFilter()
{
	auto filters = m_pAssetViewer->GetCurrentFilters();
	CString str;

	{
		SAssetField& field = filters["trianglecount"];

		field.m_fieldName = "trianglecount";
		field.m_filterCondition = SAssetField::eCondition_InsideRange;
		m_cbMinTris.GetLBText(m_cbMinTris.GetCurSel(), str);
		field.m_filterValue = str;
		m_cbMaxTris.GetLBText(m_cbMaxTris.GetCurSel(), str);
		field.m_maxFilterValue = str;
		field.m_fieldType = SAssetField::eType_Int32;
	}

	{
		SAssetField& field = filters["mtlcount"];

		field.m_fieldName = "mtlcount";
		field.m_filterCondition = SAssetField::eCondition_InsideRange;
		m_cbMinMtls.GetLBText(m_cbMinMtls.GetCurSel(), str);
		field.m_filterValue = str;
		m_cbMaxMtls.GetLBText(m_cbMaxMtls.GetCurSel(), str);
		field.m_maxFilterValue = str;
		field.m_fieldType = SAssetField::eType_Int32;
	}

	{
		SAssetField& field = filters["islod"];

		if (IsDlgButtonChecked(IDC_CHECK_HIDE_LODS))
		{
			field.m_fieldName = "islod";
			field.m_filterCondition = SAssetField::eCondition_Equal;
			field.m_filterValue =  "No";
			field.m_fieldType = SAssetField::eType_Bool;
		}
		else
		{
			auto iter = filters.find("islod");
			
			if (iter != filters.end())
			{
				filters.erase(iter);
			}
		}
	}

	m_pAssetViewer->ApplyFilters(filters);
}

void CCharacterAssetDbFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_MIN_TRIANGLES, m_cbMinTris);
	DDX_Control(pDX, IDC_COMBO_MAX_TRIANGLES, m_cbMaxTris);
	DDX_Control(pDX, IDC_COMBO_MIN_MATERIALS, m_cbMinMtls);
	DDX_Control(pDX, IDC_COMBO_MAX_MATERIALS, m_cbMaxMtls);
}

BEGIN_MESSAGE_MAP(CCharacterAssetDbFilterDlg, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_MIN_TRIANGLES, &OnCbnSelchangeComboMinTriangles)
	ON_CBN_SELCHANGE(IDC_COMBO_MAX_TRIANGLES, &OnCbnSelchangeComboMaxTriangles)
	ON_CBN_SELCHANGE(IDC_COMBO_MIN_MATERIALS, &OnCbnSelchangeComboMinMaterials)
	ON_CBN_SELCHANGE(IDC_COMBO_MAX_MATERIALS, &OnCbnSelchangeComboMaxMaterials)
	ON_BN_CLICKED(IDC_CHECK_HIDE_LODS, &OnBnClickedCheckHideLods)
END_MESSAGE_MAP()

// CCharacterAssetDbFilterDlg message handlers

void CCharacterAssetDbFilterDlg::OnCbnSelchangeComboMinTriangles()
{
	if (m_cbMinTris.GetCurSel() > m_cbMaxTris.GetCurSel())
	{
		m_cbMinTris.SetCurSel(m_cbMaxTris.GetCurSel());
		return;
	}

	ApplyFilter();
}

void CCharacterAssetDbFilterDlg::OnCbnSelchangeComboMaxTriangles()
{
	if (m_cbMaxTris.GetCurSel() < m_cbMinTris.GetCurSel())
	{
		m_cbMaxTris.SetCurSel(m_cbMinTris.GetCurSel());
		return;
	}

	ApplyFilter();
}

void CCharacterAssetDbFilterDlg::OnCbnSelchangeComboMinMaterials()
{
	if (m_cbMinMtls.GetCurSel() > m_cbMaxMtls.GetCurSel())
	{
		m_cbMinMtls.SetCurSel(m_cbMaxMtls.GetCurSel());
		return;
	}

	ApplyFilter();
}

void CCharacterAssetDbFilterDlg::OnCbnSelchangeComboMaxMaterials()
{
	if (m_cbMaxMtls.GetCurSel() < m_cbMinMtls.GetCurSel())
	{
		m_cbMaxMtls.SetCurSel(m_cbMinMtls.GetCurSel());
		return;
	}

	ApplyFilter();
}

void CCharacterAssetDbFilterDlg::OnBnClickedCheckHideLods()
{
	ApplyFilter();
}

BOOL CCharacterAssetDbFilterDlg::OnInitDialog()
{
	__super::OnInitDialog();

	m_cbMinTris.SetCurSel(0);
	m_cbMaxTris.SetCurSel(m_cbMaxTris.GetCount() - 1);
	m_cbMinMtls.SetCurSel(0);
	m_cbMaxMtls.SetCurSel(m_cbMaxMtls.GetCount() - 1);
	CheckDlgButton(IDC_CHECK_HIDE_LODS, 1);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
