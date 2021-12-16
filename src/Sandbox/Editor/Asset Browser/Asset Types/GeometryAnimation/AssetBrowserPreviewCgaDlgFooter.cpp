////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AssetBrowserPreviewCgaDlgFooter.h"
#include "AssetCgaItem.h"

IMPLEMENT_DYNAMIC(CAssetBrowserPreviewCgaDlgFooter, CDialog)

CAssetBrowserPreviewCgaDlgFooter::CAssetBrowserPreviewCgaDlgFooter(CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserPreviewCgaDlgFooter::IDD, pParent)
{
	m_minLodDefault = 0;
}

CAssetBrowserPreviewCgaDlgFooter::~CAssetBrowserPreviewCgaDlgFooter()
{
}

void CAssetBrowserPreviewCgaDlgFooter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MODEL_PREVIEW_LODLEVEL_COMBO, m_CBLodLevel);
	DDX_Control(pDX, IDC_ASSETVIEWPORT_AMBIENT_SLIDER, m_sliderAmbient);
}

BEGIN_MESSAGE_MAP(CAssetBrowserPreviewCgaDlgFooter, CDialog)
	ON_CBN_SELCHANGE(IDC_MODEL_PREVIEW_LODLEVEL_COMBO, OnLodLevelChanged)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

// CAssetBrowserPreviewModelDlgFooter message handlers
BOOL CAssetBrowserPreviewCgaDlgFooter::OnInitDialog()
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
	m_sliderAmbient.SetPos((int)AssetBrowser::kCgaAssetAmbienceMultiplier);

	CString text;

	text.Format("%d", (int)AssetBrowser::kCgaAssetAmbienceMultiplier);
	SetDlgItemText(IDC_ASSET_BROWSER_AMBIENT_VALUE, text);
	UpdateData(FALSE);

	return TRUE;
}

void CAssetBrowserPreviewCgaDlgFooter::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CAssetCgaItem::s_fAmbience = (float)m_sliderAmbient.GetPos();
	CString text;
	text.Format("%d", (int)CAssetCgaItem::s_fAmbience);
	SetDlgItemText(IDC_ASSET_BROWSER_AMBIENT_VALUE, text);
	GetParent()->RedrawWindow();
}

void CAssetBrowserPreviewCgaDlgFooter::Init()
{
}

void CAssetBrowserPreviewCgaDlgFooter::Reset()
{
	gEnv->pConsole->GetCVar("e_LodMin")->Set(0);
	m_CBLodLevel.SetCurSel(0);
	OnLodLevelChanged();
}

void CAssetBrowserPreviewCgaDlgFooter::OnLodLevelChanged()
{
	gEnv->pConsole->GetCVar("e_LodMin")->Set(m_CBLodLevel.GetCurSel());
}