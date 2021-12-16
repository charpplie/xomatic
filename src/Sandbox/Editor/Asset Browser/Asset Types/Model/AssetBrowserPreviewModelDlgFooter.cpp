////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetBrowserPreviewModelDlg.cpp
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//	Description: Implementation of AssetBrowserPreviewModelDlg.h
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AssetBrowserPreviewModelDlgFooter.h"
#include "AssetModelItem.h"

IMPLEMENT_DYNAMIC(CAssetBrowserPreviewModelDlgFooter, CDialog)

CAssetBrowserPreviewModelDlgFooter::CAssetBrowserPreviewModelDlgFooter(CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserPreviewModelDlgFooter::IDD, pParent)
{
	m_minLodDefault = 0;
}

CAssetBrowserPreviewModelDlgFooter::~CAssetBrowserPreviewModelDlgFooter()
{
}

void CAssetBrowserPreviewModelDlgFooter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MODEL_PREVIEW_LODLEVEL_COMBO, m_CBLodLevel);
	DDX_Control(pDX, IDC_ASSETVIEWPORT_AMBIENT_SLIDER, m_sliderAmbient);
}

BEGIN_MESSAGE_MAP(CAssetBrowserPreviewModelDlgFooter, CDialog)
	ON_CBN_SELCHANGE(IDC_MODEL_PREVIEW_LODLEVEL_COMBO, OnLodLevelChanged)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

// CAssetBrowserPreviewModelDlgFooter message handlers
BOOL CAssetBrowserPreviewModelDlgFooter::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_CBLodLevel.Clear();

	int lodmax = -1;

	ICVar* pCvar = gEnv->pConsole->GetCVar("e_LodMax");

	if (pCvar)
	{
		lodmax = pCvar->GetIVal();
	}

	CString str;
	
	for (int idx = 0; idx <= lodmax; idx++)
	{
		str.Format("%d", idx);
		m_CBLodLevel.InsertString(idx, str);
	}

	m_minLodDefault = gEnv->pConsole->GetCVar("e_LodMin")->GetIVal();
	m_CBLodLevel.SetCurSel(m_minLodDefault);
	m_sliderAmbient.SetRange(0, 100);
	m_sliderAmbient.SetPos((int)AssetBrowser::kModelAssetAmbienceMultiplyer);

	CString text;

	text.Format("%d", (int)AssetBrowser::kModelAssetAmbienceMultiplyer);
	SetDlgItemText(IDC_ASSET_BROWSER_AMBIENT_VALUE, text);
	UpdateData(FALSE);

	return TRUE;
}

void CAssetBrowserPreviewModelDlgFooter::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CAssetModelItem::s_fAmbience = (float)m_sliderAmbient.GetPos();
	CString text;
	text.Format("%d", (int)CAssetModelItem::s_fAmbience);
	SetDlgItemText(IDC_ASSET_BROWSER_AMBIENT_VALUE, text);
	GetParent()->RedrawWindow();
}

void CAssetBrowserPreviewModelDlgFooter::Init()
{
}

void CAssetBrowserPreviewModelDlgFooter::Reset()
{
	gEnv->pConsole->GetCVar("e_LodMin")->Set(0);
	m_CBLodLevel.SetCurSel(0);
	OnLodLevelChanged();
}

void CAssetBrowserPreviewModelDlgFooter::OnLodLevelChanged()
{
	gEnv->pConsole->GetCVar("e_LodMin")->Set(m_CBLodLevel.GetCurSel());
}