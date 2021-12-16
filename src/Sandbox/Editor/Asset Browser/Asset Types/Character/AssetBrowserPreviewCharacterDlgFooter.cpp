////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AssetBrowserPreviewCharacterDlgFooter.h"
#include "AssetCharacterItem.h"

IMPLEMENT_DYNAMIC(CAssetBrowserPreviewCharacterDlgFooter, CDialog)

CAssetBrowserPreviewCharacterDlgFooter::CAssetBrowserPreviewCharacterDlgFooter(CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserPreviewCharacterDlgFooter::IDD, pParent)
{
	m_minLodDefault = 0;
}

CAssetBrowserPreviewCharacterDlgFooter::~CAssetBrowserPreviewCharacterDlgFooter()
{
}

void CAssetBrowserPreviewCharacterDlgFooter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MODEL_PREVIEW_LODLEVEL_COMBO, m_CBLodLevel);
	DDX_Control(pDX, IDC_ASSETVIEWPORT_AMBIENT_SLIDER, m_sliderAmbient);
}

BEGIN_MESSAGE_MAP(CAssetBrowserPreviewCharacterDlgFooter, CDialog)
	ON_CBN_SELCHANGE(IDC_MODEL_PREVIEW_LODLEVEL_COMBO, OnLodLevelChanged)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

// CAssetBrowserPreviewCharacterDlgFooter message handlers
BOOL CAssetBrowserPreviewCharacterDlgFooter::OnInitDialog()
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
	m_sliderAmbient.SetPos((int)AssetBrowser::kCharacterAssetAmbienceMultiplier);

	CString text;

	text.Format("%d", (int)AssetBrowser::kCharacterAssetAmbienceMultiplier);
	SetDlgItemText(IDC_ASSET_BROWSER_AMBIENT_VALUE, text);
	UpdateData(FALSE);

	return TRUE;
}

void CAssetBrowserPreviewCharacterDlgFooter::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CAssetCharacterItem::s_fAmbience = (float)m_sliderAmbient.GetPos();
	CString text;
	text.Format("%d", (int)CAssetCharacterItem::s_fAmbience);
	SetDlgItemText(IDC_ASSET_BROWSER_AMBIENT_VALUE, text);
	GetParent()->RedrawWindow();
}

void CAssetBrowserPreviewCharacterDlgFooter::Init()
{
}

void CAssetBrowserPreviewCharacterDlgFooter::Reset()
{
	gEnv->pConsole->GetCVar("e_LodMin")->Set(0);
	m_CBLodLevel.SetCurSel(0);
	OnLodLevelChanged();
}

void CAssetBrowserPreviewCharacterDlgFooter::OnLodLevelChanged()
{
	gEnv->pConsole->GetCVar("e_LodMin")->Set(m_CBLodLevel.GetCurSel());
}