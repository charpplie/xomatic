////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "GeneralAssetDbFilterDlg.h"
#include "Util/EditorUtils.h"
#include "AssetBrowserDialog.h"
#include "AssetBrowserManager.h"

namespace AssetBrowser
{
	const char* kFilterPresetsFilename = "Editor/AssetBrowserFilterPresets.xml";
};

// CGeneralAssetDbFilterDlg dialog

IMPLEMENT_DYNAMIC(CGeneralAssetDbFilterDlg, CDialog)

CGeneralAssetDbFilterDlg::CGeneralAssetDbFilterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGeneralAssetDbFilterDlg::IDD, pParent)
{
}

CGeneralAssetDbFilterDlg::~CGeneralAssetDbFilterDlg()
{
}

void CGeneralAssetDbFilterDlg::UpdateFilterUI()
{
	auto filters = m_pAssetViewer->GetCurrentFilters();

	{
		SAssetField& field = filters["filesize"];
		CString str;

		str.Format("%d", atoi(field.m_filterValue) / 1024);
		m_cbMinFilesize.SelectString(-1, str);
		str.Format("%d", atoi(field.m_maxFilterValue) / 1024);
		m_cbMaxFilesize.SelectString(-1, str);
	}

	{
		SAssetField& field = filters["usedinlevel"];

		CheckDlgButton(IDC_CHECK_USED_IN_LEVEL, (field.m_filterValue == "Yes"));
	}
}

void CGeneralAssetDbFilterDlg::ApplyFilter()
{
	auto filters = m_pAssetViewer->GetCurrentFilters();
	CString str;

	{
		SAssetField& field = filters["filesize"];

		field.m_fieldName = "filesize";
		field.m_filterCondition = SAssetField::eCondition_InsideRange;
		m_cbMinFilesize.GetLBText(m_cbMinFilesize.GetCurSel(), str);
		int filesize = atoi(str);
		str.Format("%d", filesize * 1024);
		field.m_filterValue = str;
		m_cbMaxFilesize.GetLBText(m_cbMaxFilesize.GetCurSel(), str);
		filesize = atoi(str);
		str.Format("%d", filesize * 1024);
		field.m_maxFilterValue = str;
		field.m_fieldType = SAssetField::eType_Int32;
	}

	{
		bool bChecked = IsDlgButtonChecked(IDC_CHECK_USED_IN_LEVEL);

		if (bChecked)
		{
			SAssetField& field = filters["usedinlevel"];

			field.m_fieldName = "usedinlevel";
			field.m_filterCondition = SAssetField::eCondition_Equal;
			field.m_filterValue = "Yes";
			field.m_fieldType = SAssetField::eType_Bool;
		}
		else
		{
			auto iter = filters.find("usedinlevel");

			if (iter != filters.end())
			{
				filters.erase(iter);
			}
		}
	}

	m_pAssetViewer->ApplyFilters(filters);
}

void CGeneralAssetDbFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_PRESET, m_cbPresets);
	DDX_Control(pDX, IDC_COMBO_MINIMUM_FILESIZE, m_cbMinFilesize);
	DDX_Control(pDX, IDC_COMBO_MAXIMUM_FILESIZE, m_cbMaxFilesize);
	DDX_Control(pDX, IDC_LIST_DATABASES, m_lstDatabases);
}

BEGIN_MESSAGE_MAP(CGeneralAssetDbFilterDlg, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_PRESET, &CGeneralAssetDbFilterDlg::OnCbnSelchangeComboPreset)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_PRESET, &CGeneralAssetDbFilterDlg::OnBnClickedButtonSavePreset)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_PRESET, &CGeneralAssetDbFilterDlg::OnBnClickedButtonRemovePreset)
	ON_BN_CLICKED(IDC_CHECK_USED_IN_LEVEL, &CGeneralAssetDbFilterDlg::OnBnClickedCheckUsedInLevel)
	ON_CBN_SELCHANGE(IDC_COMBO_MINIMUM_FILESIZE, &CGeneralAssetDbFilterDlg::OnCbnSelchangeComboMinimumFilesize)
	ON_CBN_SELCHANGE(IDC_COMBO_MAXIMUM_FILESIZE, &CGeneralAssetDbFilterDlg::OnCbnSelchangeComboMaximumFilesize)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_DATABASES, &CGeneralAssetDbFilterDlg::OnLvnItemchangedListDatabases)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE_USED_IN_LEVEL, &CGeneralAssetDbFilterDlg::OnBnClickedButtonUpdateUsedInLevel)
END_MESSAGE_MAP()


// CGeneralAssetDbFilterDlg message handlers

bool CGeneralAssetDbFilterDlg::LoadFilterPresets()
{
	return LoadFilterPresetsTo(m_filterPresets);
}

bool CGeneralAssetDbFilterDlg::SaveFilterPresets()
{
	return SaveFilterPresetsFrom(m_filterPresets);
}

void CGeneralAssetDbFilterDlg::SaveCurrentPreset()
{
	UpdateFilterPreset(m_currentPresetName);
	SaveFilterPresetsFrom(m_filterPresets);
}

void CGeneralAssetDbFilterDlg::FillPresetList()
{
	m_cbPresets.ResetContent();

	int sel = m_cbPresets.GetCurSel();

	m_cbPresets.AddString("<None>");

	for (auto iter = m_filterPresets.begin(), iterEnd = m_filterPresets.end(); iter != iterEnd; ++iter )
	{
		m_cbPresets.AddString(iter->first);
	}

	m_cbPresets.SetCurSel(sel);
	m_cbPresets.SetWindowText(m_currentPresetName);
}

void CGeneralAssetDbFilterDlg::FillDatabases()
{
	TAssetDatabases dbs = CAssetBrowserManager::Instance()->GetAssetDatabases();

	m_lstDatabases.DeleteAllItems();

	for (size_t i = 0; i < dbs.size(); ++i)
	{
		m_lstDatabases.InsertItem(i, dbs[i]->GetDatabaseName());
	}
}

void CGeneralAssetDbFilterDlg::OnCbnSelchangeComboPreset()
{
	CString presetName;

	IAssetItemDatabase::TAssetFields fields;

	if (m_cbPresets.GetCount() && m_cbPresets.GetCurSel() > 0)
	{
		m_cbPresets.GetLBText(m_cbPresets.GetCurSel(), presetName);
		m_currentPresetName = presetName;
		fields = m_filterPresets[m_currentPresetName].fields;

		auto preset = m_filterPresets[m_currentPresetName];

		for (size_t i = 0; i < m_lstDatabases.GetItemCount(); ++i)
		{
			auto iter = std::find(preset.checkedDatabaseNames.begin(), preset.checkedDatabaseNames.end(), m_lstDatabases.GetItemText(i, 0));

			if (iter != preset.checkedDatabaseNames.end())
			{
				m_lstDatabases.SetCheck(i);
			}
			else
			{
				m_lstDatabases.SetCheck(i, FALSE);
			}
		}

		CheckDlgButton(IDC_CHECK_USED_IN_LEVEL, preset.bUsedInLevel);
	}
	else
	{
		m_currentPresetName = "";
	}

	IAssetItemDatabase::TAssetFieldFiltersMap filters;

	for (size_t i = 0; i < fields.size(); ++i)
	{
		filters[fields[i].m_fieldName] = fields[i];
	}

	CAssetBrowserDialog::Instance()->GetAssetViewer().SetFilters(filters);
	CAssetBrowserDialog::Instance()->ApplyAllFiltering();
	UpdateFilterUI();
	CAssetBrowserDialog::Instance()->GetAssetFiltersDlg().UpdateAllFiltersUI();
}

bool CGeneralAssetDbFilterDlg::LoadFilterPresetsTo(TPresetNamePresetMap& rOutFilters, bool bClearMap)
{
	if (bClearMap)
	{
		rOutFilters.clear();
	}

	if (!CFileUtil::FileExists(AssetBrowser::kFilterPresetsFilename))
	{
		return false;
	}

	XmlNodeRef xmlRoot = GetISystem()->LoadXmlFromFile(AssetBrowser::kFilterPresetsFilename);
	XmlString xmlStr;
	std::vector<CString> strings;
	CString strTemp;

	if (!xmlRoot)
		return false;

	for (size_t i = 0, iCount = xmlRoot->getChildCount(); i < iCount; ++i)
	{
		SFieldFiltersPreset preset;

		xmlRoot->getChild( i )->getAttr( "name", xmlStr );
		preset.presetName = xmlStr;
		xmlRoot->getChild(i)->getAttr("bUsedInLevel", preset.bUsedInLevel);
		xmlRoot->getChild(i)->getAttr("checkedDbs", strTemp);
		SplitString(strTemp, preset.checkedDatabaseNames);

		for( size_t j = 0, jCount = xmlRoot->getChild( i )->getChildCount(); j < jCount; ++j )
		{
			SAssetField field;
			XmlNodeRef xmlFilter = xmlRoot->getChild( i )->getChild( j );

			xmlFilter->getAttr( "displayName", xmlStr );
			field.m_displayName = xmlStr;
			xmlFilter->getAttr( "fieldType", xmlStr );
			field.m_fieldType = (SAssetField::EAssetFieldType)atoi( xmlStr.c_str() );
			xmlFilter->getAttr( "fieldName", xmlStr );
			field.m_fieldName = xmlStr;
			xmlFilter->getAttr( "filterCondition", (int&)field.m_filterCondition );
			xmlFilter->getAttr( "filterValue", xmlStr );
			field.m_filterValue = xmlStr;
			xmlFilter->getAttr( "maxFilterValue", xmlStr );
			field.m_maxFilterValue = xmlStr;
			xmlFilter->getAttr( "parentDB", xmlStr );
			field.m_parentDatabaseName = xmlStr;
			xmlFilter->getAttr( "enumValues", xmlStr );
			SplitString( CString( xmlStr.c_str() ), field.m_enumValues );
			xmlFilter->getAttr( "useEnumValues", (int&)field.m_bUseEnumValues );

			preset.fields.push_back( field );
		}

		rOutFilters[preset.presetName] = preset;
	}

	return true;
}

bool CGeneralAssetDbFilterDlg::SaveFilterPresetsFrom(TPresetNamePresetMap& rFilters)
{
	CString			strTemp;
	XmlNodeRef	xmlRoot = XmlHelpers::CreateXmlNode( "filterPresets" );
	XmlString		xmlStr;

	if( !xmlRoot )
		return false;

	for( TPresetNamePresetMap::iterator iter = rFilters.begin(), iterEnd = rFilters.end(); iter != iterEnd; ++iter )
	{
		XmlNodeRef xmlFilterPreset = XmlHelpers::CreateXmlNode("filterPreset");
		xmlFilterPreset->setAttr("name", iter->first);
		xmlFilterPreset->setAttr("bUsedInLevel", iter->second.bUsedInLevel);
		JoinStrings(iter->second.checkedDatabaseNames, strTemp);
		xmlFilterPreset->setAttr("checkedDbs", strTemp);

		for( size_t i = 0, iCount = iter->second.fields.size(); i < iCount; ++i )
		{
			XmlNodeRef xmlFilter = XmlHelpers::CreateXmlNode( "fieldFilter" );
			xmlFilter->setAttr( "displayName", iter->second.fields[i].m_displayName );
			xmlFilter->setAttr( "fieldName", iter->second.fields[i].m_fieldName );
			xmlFilter->setAttr( "fieldType", (int)iter->second.fields[i].m_fieldType );
			xmlFilter->setAttr( "filterCondition", (int)iter->second.fields[i].m_filterCondition );
			xmlFilter->setAttr( "filterValue", iter->second.fields[i].m_filterValue );
			xmlFilter->setAttr( "maxFilterValue", iter->second.fields[i].m_maxFilterValue );
			xmlFilter->setAttr( "parentDB", iter->second.fields[i].m_parentDatabaseName );
			strTemp = "";
			JoinStrings( iter->second.fields[i].m_enumValues, strTemp );
			xmlFilter->setAttr( "enumValues", strTemp );
			xmlFilter->setAttr( "useEnumValues", (int)iter->second.fields[i].m_bUseEnumValues );

			xmlFilterPreset->addChild( xmlFilter );
		}

		xmlRoot->addChild( xmlFilterPreset );
	}

	bool bResult = xmlRoot->saveToFile( AssetBrowser::kFilterPresetsFilename ); 

	return bResult;
}

bool CGeneralAssetDbFilterDlg::AddFilterPreset(const char* pPresetName)
{
	SFieldFiltersPreset newPreset;

	if (!strcmp(pPresetName, ""))
	{
		return false;
	}

	if (!GetFilterPresetByName(pPresetName))
	{
		// insert a new one
		newPreset.presetName = pPresetName;
		m_filterPresets[pPresetName] = newPreset;
		m_currentPresetName = pPresetName;
		SaveCurrentPreset();

		return true;
	}

	SaveFilterPresets();

	return false;
}

bool CGeneralAssetDbFilterDlg::UpdateFilterPreset( const char* pPresetName )
{
	if( !strcmp( pPresetName, "" ) )
	{
		return false;
	}

	if( m_filterPresets.empty() )
	{
		return false;
	}

	SFieldFiltersPreset* pPreset = GetFilterPresetByName(pPresetName);

	if (!pPreset)
	{
		// no existing preset with that name
		return false;
	}

	IAssetItemDatabase::TAssetFieldFiltersMap filters = m_pAssetViewer->GetCurrentFilters();
	IAssetItemDatabase::TAssetFields fields;

	for (auto iter = filters.begin(); iter != filters.end(); ++iter)
	{
		fields.push_back(iter->second);
	}

	pPreset->fields = fields;
	pPreset->checkedDatabaseNames.clear();

	for (size_t i = 0; i < m_lstDatabases.GetItemCount(); ++i)
	{
		if (m_lstDatabases.GetCheck(i))
		{
			pPreset->checkedDatabaseNames.push_back(m_lstDatabases.GetItemText(i, 0));
		}
	}

	pPreset->bUsedInLevel = IsDlgButtonChecked(IDC_CHECK_USED_IN_LEVEL);
	SaveFilterPresets();

	return true;
}

bool CGeneralAssetDbFilterDlg::DeleteFilterPreset(const char* pPresetName)
{
	m_filterPresets.erase(m_filterPresets.find(pPresetName));
	SaveFilterPresets();

	return true;
}

SFieldFiltersPreset* CGeneralAssetDbFilterDlg::GetFilterPresetByName( const char* pPresetName )
{
	if( m_filterPresets.end() == m_filterPresets.find( pPresetName ) )
		return NULL;

	return &m_filterPresets[pPresetName];
}

void CGeneralAssetDbFilterDlg::SelectPresetByName(const char* pName)
{
	m_cbPresets.SelectString(-1, pName);
}

void CGeneralAssetDbFilterDlg::OnBnClickedButtonSavePreset()
{
	CString presetName;
		
	m_cbPresets.GetWindowText(presetName);

	if (!m_cbPresets.GetCurSel())
	{
		AfxMessageBox("Please type in a valid preset name in the preset combobox edit.");
		return;
	}
	
	SFieldFiltersPreset* pPreset = GetFilterPresetByName(presetName);

	if (!pPreset)
	{
		if (AddFilterPreset(presetName))
		{
			FillPresetList();
			SelectPresetByName(presetName);
			OnCbnSelchangeComboPreset();
		}
	}

	m_currentPresetName = presetName;
	SaveCurrentPreset();
	SaveFilterPresets();
	FillPresetList();
}

void CGeneralAssetDbFilterDlg::OnBnClickedButtonRemovePreset()
{
	CString presetName;

	if (!m_cbPresets.GetCurSel())
	{
		AfxMessageBox("No preset to delete.");
		return;
	}

	m_cbPresets.GetWindowText(presetName);

	if (presetName != "")
	{
		CString strMsg;

		strMsg.Format("Delete filter preset: '%s' ?", presetName.GetBuffer());

		if (AfxMessageBox(strMsg, MB_YESNO) == IDYES)
		{
			if (DeleteFilterPreset(presetName))
			{
				FillPresetList();

				if (m_cbPresets.GetCount())
				{
					m_cbPresets.SetCurSel(0);
				}

				OnCbnSelchangeComboPreset();
				SaveFilterPresets();
			}
		}
	}
}


void CGeneralAssetDbFilterDlg::UpdateVisibleDatabases()
{
	for (size_t i = 0; i < m_lstDatabases.GetItemCount(); ++i)
	{
		CString dbName = m_lstDatabases.GetItemText(i, 0);
		IAssetItemDatabase* pDB = CAssetBrowserManager::Instance()->GetDatabaseByName(dbName);

		if (!pDB)
		{
			return;
		}

		bool bChecked = m_lstDatabases.GetCheck(i);

		bChecked 
			? CAssetBrowserDialog::Instance()->GetAssetViewer().AddDatabase(pDB)
			: CAssetBrowserDialog::Instance()->GetAssetViewer().RemoveDatabase(pDB);
	}

	m_pAssetViewer->ApplyFilters(m_pAssetViewer->GetCurrentFilters());
	CAssetBrowserDialog::Instance()->GetAssetFiltersDlg().RefreshVisibleAssetDbsRollups();
}

void CGeneralAssetDbFilterDlg::SelectDatabase(const char* pDbName)
{
	for (size_t i = 0; i < m_lstDatabases.GetItemCount(); ++i)
	{
		bool bCheck = (m_lstDatabases.GetItemText(i, 0) == pDbName);
		m_lstDatabases.SetCheck(i, bCheck);
	}

	CAssetBrowserDialog::Instance()->GetAssetViewer().ClearDatabases();

	IAssetItemDatabase* pDB = CAssetBrowserManager::Instance()->GetDatabaseByName(pDbName);
	
	if (!pDB)
	{
		return;
	}

	TAssetDatabases dbs;

	dbs.push_back(pDB);
	CAssetBrowserDialog::Instance()->GetAssetViewer().SetDatabases(dbs);
	m_pAssetViewer->ApplyFilters(m_pAssetViewer->GetCurrentFilters());
}

void CGeneralAssetDbFilterDlg::OnBnClickedCheckUsedInLevel()
{
	ApplyFilter();
}

void CGeneralAssetDbFilterDlg::OnCbnSelchangeComboMinimumFilesize()
{
	if (m_cbMinFilesize.GetCurSel() > m_cbMaxFilesize.GetCurSel())
	{
		m_cbMinFilesize.SetCurSel(m_cbMaxFilesize.GetCurSel());
		AfxMessageBox("Please choose a smaller value than the one specified in 'Maximum filesize'");
		return;
	}

	ApplyFilter();
}

void CGeneralAssetDbFilterDlg::OnCbnSelchangeComboMaximumFilesize()
{
	if (m_cbMaxFilesize.GetCurSel() < m_cbMinFilesize.GetCurSel())
	{
		m_cbMaxFilesize.SetCurSel(m_cbMinFilesize.GetCurSel());
		AfxMessageBox("Please choose a greater value than the one specified in 'Minimum filesize'");
		return;
	}

	ApplyFilter();
}

BOOL CGeneralAssetDbFilterDlg::OnInitDialog()
{
	__super::OnInitDialog();

	m_cbMinFilesize.SetCurSel(0);
	m_cbMaxFilesize.SetCurSel(m_cbMaxFilesize.GetCount() - 1);
	m_lstDatabases.SetExtendedStyle(LVS_EX_CHECKBOXES);
	LoadFilterPresets();
	FillPresetList();
	FillDatabases();

	for (size_t i = 0; i < m_lstDatabases.GetItemCount(); ++i)
	{
		m_lstDatabases.SetCheck(i);
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CGeneralAssetDbFilterDlg::OnLvnItemchangedListDatabases(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	UpdateVisibleDatabases();
	*pResult = 0;
}

void CGeneralAssetDbFilterDlg::OnBnClickedButtonUpdateUsedInLevel()
{
	CAssetBrowserManager::Instance()->MarkUsedInLevelAssets();
	ApplyFilter();
}
