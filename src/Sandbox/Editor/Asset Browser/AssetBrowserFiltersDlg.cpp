////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
// -------------------------------------------------------------------------
//  Created:	25/11/2012 by Nicusor Nedelcu
//
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AssetBrowserFiltersDlg.h"
#include "AssetBrowserManager.h"
#include "AssetBrowserDialog.h"
#include "GeneralAssetDbFilterDlg.h"
#include "IAssetItemDatabase.h"

IMPLEMENT_DYNAMIC(CAssetBrowserFiltersDlg, CDialog)

CAssetBrowserFiltersDlg::CAssetBrowserFiltersDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserFiltersDlg::IDD, pParent)
	, m_pGeneralDlg(0)
{
}

CAssetBrowserFiltersDlg::~CAssetBrowserFiltersDlg()
{
}

void CAssetBrowserFiltersDlg::CreateFilterGroupsRollup()
{
	auto pAssetBrowserMgr = CAssetBrowserManager::Instance();
	
	m_rollupCtrl.RemoveAllPages();

	CGeneralAssetDbFilterDlg* pDlgGeneral = new CGeneralAssetDbFilterDlg();

	pDlgGeneral->SetAssetViewer(&CAssetBrowserDialog::Instance()->GetAssetViewer());
	pDlgGeneral->Create(IDD_ASSET_BROWSER_GENERAL_DB_FILTER, &m_rollupCtrl);
	pDlgGeneral->ShowWindow(SW_SHOW);
	m_pGeneralDlg = pDlgGeneral;
	m_rollupCtrl.InsertPage("General", pDlgGeneral, FALSE);
	m_panelIds.clear();

	for (size_t i = 0; i < pAssetBrowserMgr->GetAssetDatabases().size(); ++i)
	{
		auto pAssetDb = pAssetBrowserMgr->GetAssetDatabases()[i];
		CDialog* pDlg = pAssetDb->CreateDbFilterDialog(&m_rollupCtrl, &CAssetBrowserDialog::Instance()->GetAssetViewer());

		if (!pDlg)
		{
			continue;
		}

		m_filterDlgs[pAssetDb] = pDlg;
		int id = m_rollupCtrl.InsertPage(pAssetDb->GetDatabaseName(), pDlg, FALSE);
		m_panelIds.push_back(id);
	}
}

void CAssetBrowserFiltersDlg::DestroyFilterGroupsRollup()
{
	m_filterDlgs.clear();
	m_rollupCtrl.RemoveAllPages();
}

void CAssetBrowserFiltersDlg::RefreshVisibleAssetDbsRollups()
{
	for (size_t i = 0; i < m_panelIds.size(); ++i)
	{
		m_rollupCtrl.RemovePage(m_panelIds[i]);
	}

	m_panelIds.clear();

	TAssetDatabases visibleDbs = CAssetBrowserDialog::Instance()->GetAssetViewer().GetDatabases();
	for (size_t i = 0; i < visibleDbs.size(); ++i)
	{
		auto pAssetDb = visibleDbs[i];
		int id = m_rollupCtrl.InsertPage(pAssetDb->GetDatabaseName(), m_filterDlgs[pAssetDb], FALSE);
		m_panelIds.push_back(id);
	}
}

void CAssetBrowserFiltersDlg::UpdateAllFiltersUI()
{
	for (auto iter = m_filterDlgs.begin(); iter != m_filterDlgs.end(); ++iter)
	{
		if (!iter->first)
			continue;

		iter->first->UpdateDbFilterDialogUI(iter->second);
	}
}

void CAssetBrowserFiltersDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAssetBrowserFiltersDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CAssetBrowserFiltersDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CAssetBrowserFiltersDlg::OnBnClickedCancel)
	ON_WM_SIZE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CAssetBrowserFiltersDlg::OnBnClickedOk()
{
	// nothing here
}

void CAssetBrowserFiltersDlg::OnBnClickedCancel()
{
	// nothing here
}

void CAssetBrowserFiltersDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (m_rollupCtrl.GetSafeHwnd())
	{
		m_rollupCtrl.SetWindowPos(0, 0, 0, cx, cy, 0);
	}
}

BOOL CAssetBrowserFiltersDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect rc;

	GetClientRect(rc);
	m_rollupCtrl.Create(WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS, rc, this, 0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAssetBrowserFiltersDlg::OnDestroy()
{
	CDialog::OnDestroy();
	auto iter = m_filterDlgs.begin();

	while (iter != m_filterDlgs.end())
	{
		iter->second->DestroyWindow();
		++iter;
	}

	m_rollupCtrl.RemoveAllPages();
}
