////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "TextureAssetDbFilterDlg.h"
#include "IAssetViewer.h"

// CTextureAssetDbFilterDlg dialog

IMPLEMENT_DYNAMIC(CTextureAssetDbFilterDlg, CDialog)

CTextureAssetDbFilterDlg::CTextureAssetDbFilterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTextureAssetDbFilterDlg::IDD, pParent)
{
}

CTextureAssetDbFilterDlg::~CTextureAssetDbFilterDlg()
{
}

void CTextureAssetDbFilterDlg::UpdateFilterUI()
{
	auto filters = m_pAssetViewer->GetCurrentFilters();

	{
		SAssetField& field = filters["width"];

		m_cbMinWidth.SelectString(-1, field.m_filterValue);
		m_cbMaxWidth.SelectString(-1, field.m_maxFilterValue);
	}

	{
		SAssetField& field = filters["height"];

		m_cbMinHeight.SelectString(-1, field.m_filterValue);
		m_cbMaxHeight.SelectString(-1, field.m_maxFilterValue);
	}

	{
		SAssetField& field = filters["mips"];

		m_cbMinMips.SelectString(-1, field.m_filterValue);
		m_cbMaxMips.SelectString(-1, field.m_maxFilterValue);
	}

	{
		SAssetField& field = filters["type"];

		m_cbType.SelectString(-1, field.m_filterValue);
	}

	{
		SAssetField& field = filters["texture_usage"];

		m_cbUsage.SelectString(-1, field.m_filterValue);
	}
}

void CTextureAssetDbFilterDlg::ApplyFilter()
{
	auto filters = m_pAssetViewer->GetCurrentFilters();
	CString str;

	{
		SAssetField& field = filters["width"];
		
		field.m_fieldName = "width";
		field.m_filterCondition = SAssetField::eCondition_InsideRange;
		m_cbMinWidth.GetLBText(m_cbMinWidth.GetCurSel(), str);
		field.m_filterValue = str;
		m_cbMaxWidth.GetLBText(m_cbMaxWidth.GetCurSel(), str);
		field.m_maxFilterValue = str;
		field.m_fieldType = SAssetField::eType_Int32;
	}

	{
		SAssetField& field = filters["height"];

		field.m_fieldName = "height";
		field.m_filterCondition = SAssetField::eCondition_InsideRange;
		m_cbMinHeight.GetLBText(m_cbMinHeight.GetCurSel(), str);
		field.m_filterValue = str;
		m_cbMaxHeight.GetLBText(m_cbMaxHeight.GetCurSel(), str);
		field.m_maxFilterValue = str;
		field.m_fieldType = SAssetField::eType_Int32;
	}

	{
		SAssetField& field = filters["mips"];

		field.m_fieldName = "mips";
		field.m_filterCondition = SAssetField::eCondition_InsideRange;
		m_cbMinMips.GetLBText(m_cbMinMips.GetCurSel(), str);
		field.m_filterValue = str;
		m_cbMaxMips.GetLBText(m_cbMaxMips.GetCurSel(), str);
		field.m_maxFilterValue = str;
		field.m_fieldType = SAssetField::eType_Int32;
	}

	{
		m_cbType.GetLBText(m_cbType.GetCurSel(), str);

		if (str != "Any")
		{
			SAssetField& field = filters["type"];
	
			field.m_fieldName = "type";
			field.m_filterCondition = SAssetField::eCondition_Equal;
			field.m_filterValue = str;
			field.m_fieldType = SAssetField::eType_String;
		}
		else
		{
			auto iter = filters.find("type");

			if (iter != filters.end())
			{
				filters.erase(iter);
			}
		}
	}

	{
		m_cbUsage.GetLBText(m_cbUsage.GetCurSel(), str);

		if (str != "Any")
		{
			SAssetField& field = filters["texture_usage"];

			field.m_fieldName = "texture_usage";
			field.m_filterCondition = SAssetField::eCondition_Equal;
			field.m_filterValue = str;
			field.m_fieldType = SAssetField::eType_String;
		}
		else
		{
			auto iter = filters.find("texture_usage");

			if (iter != filters.end())
			{
				filters.erase(iter);
			}
		}
	}

	m_pAssetViewer->ApplyFilters(filters);
}

void CTextureAssetDbFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_MINIMUM_WIDTH, m_cbMinWidth);
	DDX_Control(pDX, IDC_COMBO_MAXIMUM_WIDTH, m_cbMaxWidth);
	DDX_Control(pDX, IDC_COMBO_MINIMUM_HEIGHT, m_cbMinHeight);
	DDX_Control(pDX, IDC_COMBO_MAXIMUM_HEIGHT, m_cbMaxHeight);
	DDX_Control(pDX, IDC_COMBO_MINIMUM_MIPS, m_cbMinMips);
	DDX_Control(pDX, IDC_COMBO_MAXIMUM_MIPS, m_cbMaxMips);
	DDX_Control(pDX, IDC_COMBO_TEXTURE_TYPE, m_cbType);
	DDX_Control(pDX, IDC_COMBO_TEXTURE_USAGE, m_cbUsage);
}

BEGIN_MESSAGE_MAP(CTextureAssetDbFilterDlg, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_MINIMUM_WIDTH, &CTextureAssetDbFilterDlg::OnCbnSelchangeComboMinimumWidth)
	ON_CBN_SELCHANGE(IDC_COMBO_MAXIMUM_WIDTH, &CTextureAssetDbFilterDlg::OnCbnSelchangeComboMaximumWidth)
	ON_CBN_SELCHANGE(IDC_COMBO_MINIMUM_HEIGHT, &CTextureAssetDbFilterDlg::OnCbnSelchangeComboMinimumHeight)
	ON_CBN_SELCHANGE(IDC_COMBO_MAXIMUM_HEIGHT, &CTextureAssetDbFilterDlg::OnCbnSelchangeComboMaximumHeight)
	ON_CBN_SELCHANGE(IDC_COMBO_MINIMUM_MIPS, &CTextureAssetDbFilterDlg::OnCbnSelchangeComboMinimumMips)
	ON_CBN_SELCHANGE(IDC_COMBO_MAXIMUM_MIPS, &CTextureAssetDbFilterDlg::OnCbnSelchangeComboMaximumMips)
	ON_CBN_SELCHANGE(IDC_COMBO_TEXTURE_TYPE, &CTextureAssetDbFilterDlg::OnCbnSelchangeComboTextureType)
	ON_CBN_SELCHANGE(IDC_COMBO_TEXTURE_USAGE, &CTextureAssetDbFilterDlg::OnCbnSelchangeComboTextureUsage)
END_MESSAGE_MAP()

// CTextureAssetDbFilterDlg message handlers

void CTextureAssetDbFilterDlg::OnBnClickedOk()
{
	// do nothing here
}

void CTextureAssetDbFilterDlg::OnBnClickedCancel()
{
	// do nothing here
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboMinimumWidth()
{
	if (m_cbMinWidth.GetCurSel() > m_cbMaxWidth.GetCurSel())
	{
		m_cbMinWidth.SetCurSel(m_cbMaxWidth.GetCurSel());
		return;
	}

	ApplyFilter();
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboMaximumWidth()
{
	if (m_cbMaxWidth.GetCurSel() < m_cbMinWidth.GetCurSel())
	{
		m_cbMaxWidth.SetCurSel(m_cbMinWidth.GetCurSel());
		return;
	}

	ApplyFilter();
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboMinimumHeight()
{
	if (m_cbMinHeight.GetCurSel() > m_cbMaxHeight.GetCurSel())
	{
		m_cbMinHeight.SetCurSel(m_cbMaxHeight.GetCurSel());
		return;
	}

	ApplyFilter();
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboMaximumHeight()
{
	if (m_cbMaxHeight.GetCurSel() < m_cbMinHeight.GetCurSel())
	{
		m_cbMaxHeight.SetCurSel(m_cbMinHeight.GetCurSel());
		return;
	}

	ApplyFilter();
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboMinimumMips()
{
	if (m_cbMinMips.GetCurSel() > m_cbMaxMips.GetCurSel())
	{
		m_cbMinMips.SetCurSel(m_cbMaxMips.GetCurSel());
		return;
	}

	ApplyFilter();
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboMaximumMips()
{
	if (m_cbMaxMips.GetCurSel() < m_cbMinMips.GetCurSel())
	{
		m_cbMaxMips.SetCurSel(m_cbMinMips.GetCurSel());
		return;
	}

	ApplyFilter();
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboTextureType()
{
	ApplyFilter();
}


BOOL CTextureAssetDbFilterDlg::OnInitDialog()
{
	__super::OnInitDialog();

	m_cbMinWidth.SetCurSel(0);
	m_cbMaxWidth.SetCurSel(m_cbMaxWidth.GetCount() - 1);
	m_cbMinHeight.SetCurSel(0);
	m_cbMaxHeight.SetCurSel(m_cbMaxHeight.GetCount() - 1);
	m_cbMinMips.SetCurSel(0);
	m_cbMaxMips.SetCurSel(m_cbMaxMips.GetCount() - 1);
	m_cbType.SetCurSel(0);
	m_cbUsage.SetCurSel(0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CTextureAssetDbFilterDlg::OnCbnSelchangeComboTextureUsage()
{
	ApplyFilter();
}
