////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AssetBrowserCachingOptionsDlg.h"
#include "IAssetItemDatabase.h"
#include "AssetBrowserDialog.h"

// CAssetBrowserCachingOptionsDlg dialog

IMPLEMENT_DYNAMIC(CAssetBrowserCachingOptionsDlg, CDialog)

CAssetBrowserCachingOptionsDlg::CAssetBrowserCachingOptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserCachingOptionsDlg::IDD, pParent)
{

}

CAssetBrowserCachingOptionsDlg::~CAssetBrowserCachingOptionsDlg()
{
}

void CAssetBrowserCachingOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_ASSET_TYPES, m_lstDatabases);
	DDX_Control(pDX, IDC_COMBO_THUMBNAIL_BITMAP_SIZE, m_cbThumbSize);
}


BEGIN_MESSAGE_MAP(CAssetBrowserCachingOptionsDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CAssetBrowserCachingOptionsDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CAssetBrowserCachingOptionsDlg message handlers

BOOL CAssetBrowserCachingOptionsDlg::OnInitDialog()
{
	__super::OnInitDialog();

	m_bForceCache = false;

	std::vector<IClassDesc*> assetDatabasePlugins;
	IAssetItemDatabase* pAssetDB = NULL;
	IEditorClassFactory* pClassFactory = GetIEditor()->GetClassFactory();

	pClassFactory->GetClassesByCategory("Asset Item DB", assetDatabasePlugins);

	for (size_t i = 0; i < assetDatabasePlugins.size(); ++i)
	{
		if (assetDatabasePlugins[i]->QueryInterface(__uuidof(IAssetItemDatabase), (void**)&pAssetDB) == S_OK)
		{
			m_lstDatabases.AddString(pAssetDB->GetDatabaseName());
			m_databases.push_back(pAssetDB);
		}
	}
	
	m_lstDatabases.SelItemRange(TRUE, 0, m_databases.size());
	CString str;

	str.Format("%d", CAssetBrowserDialog::Instance()->GetAssetViewer().GetAssetThumbSize());
	m_cbThumbSize.SelectString(-1, str);

	return TRUE;
}

TAssetDatabases CAssetBrowserCachingOptionsDlg::GetSelectedDatabases()
{
	return m_selectedDBs;
}

bool CAssetBrowserCachingOptionsDlg::IsForceCache()
{
	return m_bForceCache;
}

UINT CAssetBrowserCachingOptionsDlg::GetThumbSize()
{
	return m_thumbSize;
}

void CAssetBrowserCachingOptionsDlg::OnBnClickedOk()
{
	for (size_t i = 0; i < m_databases.size(); ++i)
	{
		if (m_lstDatabases.GetSel(i))
		{
			m_selectedDBs.push_back(m_databases[i]);
		}
	}

	m_bForceCache = IsDlgButtonChecked(IDC_CHECK_FORCE_CACHE);
	CString str;
	m_cbThumbSize.GetLBText(m_cbThumbSize.GetCurSel(), str);
	m_thumbSize = atoi(str);

	CDialog::OnOK();
}
