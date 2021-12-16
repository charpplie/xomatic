////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetBrowserFilterPresetsDlg.cpp
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	Implementation of AssetBrowserFilterPresetsDlg.h
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "stdafx.h"
#include "AssetBrowserFilterPresetsDlg.h"
#include "AssetBrowserDialog.h"
#include "../Dialogs/Generic/StringInputDialog.h"

namespace AssetBrowser
{
	const char*			kAllDatabasesText = "(All databases)";
	const char*			kFieldFiltersPresetsFilename = "Editor/AssetBrowserFilterPresets.xml";
	const char*			kCommonFieldsTreeItemText = "Common fields";
	const COLORREF	kUsedFieldInFilterTreeItemTextColor = RGB( 200, 180, 180 );
	const COLORREF	kFieldTreeItemTextColor = RGB( 0, 200, 0 );
	const COLORREF	kChangedFieldFilterValueColor = RGB( 0, 0, 255 );
	const COLORREF	kDisabledFieldItemColor = RGB( 100, 100, 100 );
	const char*			kStrOnlyForInsideRange = "<not used, range only>";
};

typedef std::vector<std::pair<CString,SAssetField::EAssetFilterCondition>>		TSearchFilterConditions;
typedef std::set<SAssetField::EAssetFilterCondition>												TFieldFilterConditionSet;
typedef std::map<SAssetField::EAssetFieldType,TFieldFilterConditionSet>			TTypeSpecificConditionsMap;

static TSearchFilterConditions												s_cFieldFilterConditions;
static TTypeSpecificConditionsMap											s_mapTypeSpecificConditions;
static CAssetBrowserFilterPresetsDlg*									s_pFilterPresetsDlg = NULL;
static std::map<SAssetField::EAssetFieldType, CString>	s_mapFieldTypeNames;

class AutoFillStaticVariables
{
	public:

		AutoFillStaticVariables()
		{
			s_cFieldFilterConditions.push_back( std::make_pair( "any", SAssetField::eAssertFilterCondition_Any ) );
			s_cFieldFilterConditions.push_back( std::make_pair( "contains", SAssetField::eAssertFilterCondition_Contains ) );
			s_cFieldFilterConditions.push_back( std::make_pair( "contains one of the words", SAssetField::eAssertFilterCondition_ContainsOneOfTheWords ) );
			s_cFieldFilterConditions.push_back( std::make_pair( "starts with", SAssetField::eAssertFilterCondition_StartsWith ) );
			s_cFieldFilterConditions.push_back( std::make_pair( "ends with", SAssetField::eAssertFilterCondition_EndsWith ) );
			s_cFieldFilterConditions.push_back( std::make_pair( "equal to", SAssetField::eAssertFilterCondition_Equal ) );
			s_cFieldFilterConditions.push_back( std::make_pair( "greater than", SAssetField::eAssertFilterCondition_Greater ) );
			s_cFieldFilterConditions.push_back( std::make_pair( "less than", SAssetField::eAssertFilterCondition_Less ) );
			s_cFieldFilterConditions.push_back( std::make_pair( "greater or equal to", SAssetField::eAssertFilterCondition_GreaterOrEqual ) );
			s_cFieldFilterConditions.push_back( std::make_pair( "less or equal to", SAssetField::eAssertFilterCondition_LessOrEqual ) );
			s_cFieldFilterConditions.push_back( std::make_pair( "not equal to", SAssetField::eAssertFilterCondition_Not ) );
			s_cFieldFilterConditions.push_back( std::make_pair( "inside range", SAssetField::eAssertFilterCondition_InsideRange ) );

			s_mapFieldTypeNames[SAssetField::eAssetFieldType_None] = "None";
			s_mapFieldTypeNames[SAssetField::eAssetFieldType_Bool] = "Boolean";
			s_mapFieldTypeNames[SAssetField::eAssetFieldType_Int8] = "Int8";
			s_mapFieldTypeNames[SAssetField::eAssetFieldType_Int16] = "Int16";
			s_mapFieldTypeNames[SAssetField::eAssetFieldType_Int32] = "Int32";
			s_mapFieldTypeNames[SAssetField::eAssetFieldType_Int64] = "Int64";
			s_mapFieldTypeNames[SAssetField::eAssetFieldType_Float] = "Real32";
			s_mapFieldTypeNames[SAssetField::eAssetFieldType_Double] = "Real64";
			s_mapFieldTypeNames[SAssetField::eAssetFieldType_String] = "Text";

			// define the specific condition types available only for the strings/text fields
			{
				TFieldFilterConditionSet& condSet = s_mapTypeSpecificConditions[SAssetField::eAssetFieldType_String];

				condSet.insert( SAssetField::eAssertFilterCondition_Contains );
				condSet.insert( SAssetField::eAssertFilterCondition_ContainsOneOfTheWords );
				condSet.insert( SAssetField::eAssertFilterCondition_Equal );
				condSet.insert( SAssetField::eAssertFilterCondition_StartsWith );
				condSet.insert( SAssetField::eAssertFilterCondition_EndsWith );
				condSet.insert( SAssetField::eAssertFilterCondition_Less );
				condSet.insert( SAssetField::eAssertFilterCondition_Greater );
				condSet.insert( SAssetField::eAssertFilterCondition_InsideRange );
				condSet.insert( SAssetField::eAssertFilterCondition_Not );
			}

			// define the specific conditions for boolean
			{
				TFieldFilterConditionSet& condSet = s_mapTypeSpecificConditions[SAssetField::eAssetFieldType_Bool];

				condSet.insert( SAssetField::eAssertFilterCondition_Equal );
				condSet.insert( SAssetField::eAssertFilterCondition_Not );
			}

			// define the specific conditions for numbers
			{
				TFieldFilterConditionSet condSet;

				condSet.insert( SAssetField::eAssertFilterCondition_Any );
				condSet.insert( SAssetField::eAssertFilterCondition_Equal );
				condSet.insert( SAssetField::eAssertFilterCondition_Greater );
				condSet.insert( SAssetField::eAssertFilterCondition_Less );
				condSet.insert( SAssetField::eAssertFilterCondition_GreaterOrEqual );
				condSet.insert( SAssetField::eAssertFilterCondition_LessOrEqual );
				condSet.insert( SAssetField::eAssertFilterCondition_Not );
				condSet.insert( SAssetField::eAssertFilterCondition_InsideRange );

				s_mapTypeSpecificConditions[SAssetField::eAssetFieldType_Int8] = condSet;
				s_mapTypeSpecificConditions[SAssetField::eAssetFieldType_Int16] = condSet;
				s_mapTypeSpecificConditions[SAssetField::eAssetFieldType_Int32] = condSet;
				s_mapTypeSpecificConditions[SAssetField::eAssetFieldType_Int64] = condSet;
				s_mapTypeSpecificConditions[SAssetField::eAssetFieldType_Float] = condSet;
				s_mapTypeSpecificConditions[SAssetField::eAssetFieldType_Double] = condSet;
			}
		}
} g_autoFillStaticVariables;

static CString getFieldFilterConditionName( SAssetField::EAssetFilterCondition aCond )
{
	for( size_t i = 0, iCount = s_cFieldFilterConditions.size(); i < iCount; ++i )
	{
		if( s_cFieldFilterConditions[i].second == aCond )
		{
			return s_cFieldFilterConditions[i].first;
		}
	}

	return "";
}

//////////////////////////////////////////////////////////////////////

IMPLEMENT_SERIAL(CFilterFieldRecord, CXTPReportRecord, VERSIONABLE_SCHEMA | _XTP_SCHEMA_CURRENT)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFilterFieldRecord::CFilterFieldRecord()
{
	m_pField = NULL;
	CreateItems();
}

CFilterFieldRecord::CFilterFieldRecord( SAssetField* pField )
{
	m_pField = pField;
	AddItem( new CFilterFieldCheckNameItem( pField ) );
	AddItem( new CFilterFieldConditionItem( pField ) );
	AddItem( new CFilterFieldValueItem( pField ) );
	AddItem( new CFilterFieldMaxValueItem( pField ) );
	AddItem( new CXTPReportRecordItemText( ( pField->m_parentDatabaseName != "" ? pField->m_parentDatabaseName : "<common field>" ) ) );
	AddItem( new CXTPReportRecordItemText( s_mapFieldTypeNames[pField->m_fieldType] ) );
}

CFilterFieldRecord::CFilterFieldRecord(CFilterFieldRecord* pRecord)
{
	for( size_t i = 0; i < GetItemCount(); ++i )
	{
		AddItem( new CXTPReportRecordItemText( ((CXTPReportRecordItemText*)pRecord->GetItem( i ))->GetValue() ) );
	}
}

CFilterFieldRecord& CFilterFieldRecord::operator= (const CFilterFieldRecord& rOther)
{
	for( size_t i = 0; i < GetItemCount(); ++i )
	{
		((CXTPReportRecordItemText*)GetItem( i ))->SetValue( ((CXTPReportRecordItemText*)rOther.GetItem( i ) )->GetValue() );
	}

	return *this;
}

void CFilterFieldRecord::CreateItems()
{
	// Initialize record items with empty values
	AddItem(new CXTPReportRecordItemText(_T("")));
	AddItem(new CXTPReportRecordItemText(_T("")));
	AddItem(new CXTPReportRecordItemText(_T("")));
	AddItem(new CXTPReportRecordItemText(_T("")));
	AddItem(new CXTPReportRecordItemText(_T("")));
	AddItem(new CXTPReportRecordItemText(_T("")));
}

CFilterFieldRecord::~CFilterFieldRecord()
{
}

void CFilterFieldRecord::GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
{
	CXTPReportRecord::GetItemMetrics(pDrawArgs, pItemMetrics);
}

//---

IMPLEMENT_SERIAL(CFilterFieldCheckNameItem, CXTPReportRecordItem, VERSIONABLE_SCHEMA | _XTP_SCHEMA_CURRENT)

CFilterFieldCheckNameItem::CFilterFieldCheckNameItem()
{
}

CFilterFieldCheckNameItem::CFilterFieldCheckNameItem( SAssetField* pField )
{
	m_pField = pField;
	SetCaption( pField->m_displayName );
	SetBold();
}

BOOL CFilterFieldCheckNameItem::OnRequestEdit(XTP_REPORTRECORDITEM_ARGS* pItemArgs)
{
	return TRUE;
}

void CFilterFieldCheckNameItem::OnInplaceButtonDown(CXTPReportInplaceButton* pButton)
{
}

//---

IMPLEMENT_SERIAL(CFilterFieldConditionItem, CXTPReportRecordItem, VERSIONABLE_SCHEMA | _XTP_SCHEMA_CURRENT)

CFilterFieldConditionItem::CFilterFieldConditionItem()
{
	m_pField = NULL;
}

CFilterFieldConditionItem::CFilterFieldConditionItem( SAssetField* pField )
{
	m_pField = pField;
	SetCaption( getFieldFilterConditionName( pField->m_filterCondition ) );
}

void CFilterFieldConditionItem::OnEditChanged(XTP_REPORTRECORDITEM_ARGS* pItemArgs, LPCTSTR szText)
{
	SetCaption( szText );
	SetTextColor( AssetBrowser::kChangedFieldFilterValueColor );
	SetBold();
	CAssetBrowserFilterPresetsDlg::AutoSaveCurrentFilterPreset();
}

BOOL CFilterFieldConditionItem::OnRequestEdit(XTP_REPORTRECORDITEM_ARGS* pItemArgs)
{
	return TRUE;
}

void CFilterFieldConditionItem::OnInplaceButtonDown( CXTPReportInplaceButton* pButton )
{
	CRect rc, rcBtn;
	CMenu menu;

	menu.CreatePopupMenu();

	//
	// add asset field conditions to the menu
	//
	for( size_t i = 0, iCount = s_cFieldFilterConditions.size(); i < iCount; ++i )
	{
		// if this field's type has specific conditions assigned, then we will show only those conditions in the combo box
		if( s_mapTypeSpecificConditions.end() != s_mapTypeSpecificConditions.find( m_pField->m_fieldType ) )
		{
			// if we cannot find the condition type in the specific condition for this field type, then we should not show it in the conditions combo list
			if( s_mapTypeSpecificConditions[m_pField->m_fieldType].end() == s_mapTypeSpecificConditions[m_pField->m_fieldType].find( s_cFieldFilterConditions[i].second ) )
			{
				continue;
			}
		}

		// condition allowed, add it
		menu.AppendMenu( MF_STRING | MF_BYPOSITION, i + 1/*for TrackPopupMenu returns 0 if no item was chosen*/, s_cFieldFilterConditions[i].first );
	}

	rc = this->GetColumn()->GetRect();
	pButton->GetWindowRect( &rcBtn );
	this->GetColumn()->GetControl()->ClientToScreen( &rc );
	rc.top = rcBtn.bottom;
		
	UINT theChosenOne = menu.TrackPopupMenu( TPM_LEFTALIGN|TPM_RETURNCMD, rc.left, rc.top, this->GetColumn()->GetControl() );
	
	// correct index, because TrackPopupMenu returns zero if no item is chosen
	theChosenOne--;
	
	if( theChosenOne >= 0 && theChosenOne < s_cFieldFilterConditions.size() )
	{
		SetCaption( s_cFieldFilterConditions[theChosenOne].first );
		SetTextColor( AssetBrowser::kChangedFieldFilterValueColor );
		SetBold();
		m_pField->m_filterCondition = s_cFieldFilterConditions[theChosenOne].second;

		if( SAssetField::eAssertFilterCondition_InsideRange == m_pField->m_filterCondition )
		{
			this->GetRecord()->GetItem( 3 )->SetCaption( m_pField->m_maxFilterValue );
			this->GetRecord()->GetItem( 3 )->SetTextColor( 0 );
		}
		else
		{
			this->GetRecord()->GetItem( 3 )->SetCaption( AssetBrowser::kStrOnlyForInsideRange );
			this->GetRecord()->GetItem( 3 )->SetTextColor( AssetBrowser::kDisabledFieldItemColor );
		}

		CAssetBrowserFilterPresetsDlg::AutoSaveCurrentFilterPreset();
		this->GetColumn()->GetControl()->RedrawControl();
	}

	menu.DestroyMenu();
}

//---

IMPLEMENT_SERIAL(CFilterFieldValueItem, CXTPReportRecordItem, VERSIONABLE_SCHEMA | _XTP_SCHEMA_CURRENT)

CFilterFieldValueItem::CFilterFieldValueItem()
{
	m_pField = NULL;
}

CFilterFieldValueItem::CFilterFieldValueItem( SAssetField* pField )
{
	m_pField = pField;
	SetCaption( pField->m_filterValue );
}

void CFilterFieldValueItem::OnEditChanged(XTP_REPORTRECORDITEM_ARGS* pItemArgs, LPCTSTR szText)
{
	m_pField->m_filterValue = szText;
	SetCaption( szText );
	SetTextColor( AssetBrowser::kChangedFieldFilterValueColor );
	SetBold();
	CAssetBrowserFilterPresetsDlg::AutoSaveCurrentFilterPreset();
}

void CFilterFieldValueItem::OnInplaceButtonDown( CXTPReportInplaceButton* pButton )
{
	CRect rc, rcBtn;
	CMenu menu;

	if( !m_pField->m_bUseEnumValues )
		return;

	menu.CreatePopupMenu();

	//
	// add asset field enum items to the menu
	//
	for( size_t i = 0, iCount = m_pField->m_enumValues.size(); i < iCount; ++i )
	{
		// condition allowed, add it
		// we increment the position of the menu item by 1, so we can have 1-based indices, because TrackPopupMenu returns 0 if no item was chosen
		menu.AppendMenu( MF_STRING | MF_BYPOSITION, i + 1, m_pField->m_enumValues[i] );
	}

	rc = this->GetColumn()->GetRect();
	pButton->GetWindowRect( &rcBtn );
	this->GetColumn()->GetControl()->ClientToScreen( &rc );
	rc.top = rcBtn.bottom;

	UINT theChosenOne = menu.TrackPopupMenu( TPM_LEFTALIGN|TPM_RETURNCMD, rc.left, rc.top, this->GetColumn()->GetControl() );

	// decrement result, from 1-based index, so we can get our index into the enums vector 
	theChosenOne--;

	if( theChosenOne >= 0 && theChosenOne < m_pField->m_enumValues.size() )
	{
		m_pField->m_filterValue = m_pField->m_enumValues[theChosenOne];
		SetCaption( m_pField->m_filterValue );
		SetTextColor( AssetBrowser::kChangedFieldFilterValueColor );
		SetBold();
		CAssetBrowserFilterPresetsDlg::AutoSaveCurrentFilterPreset();
		// set the new value to the current edit control
		this->GetColumn()->GetControl()->GetInplaceEdit()->SetWindowText( m_pField->m_filterValue );
	}

	menu.DestroyMenu();
}

//---

IMPLEMENT_SERIAL(CFilterFieldMaxValueItem, CXTPReportRecordItem, VERSIONABLE_SCHEMA | _XTP_SCHEMA_CURRENT)

CFilterFieldMaxValueItem::CFilterFieldMaxValueItem()
{
	m_pField = NULL;
}

CFilterFieldMaxValueItem::CFilterFieldMaxValueItem( SAssetField* pField )
{
	m_pField = pField;
	
	if( m_pField->m_filterCondition == SAssetField::eAssertFilterCondition_InsideRange )
	{
		SetCaption( pField->m_maxFilterValue );
	}
	else
	{
		SetCaption( AssetBrowser::kStrOnlyForInsideRange );
		SetTextColor( AssetBrowser::kDisabledFieldItemColor );
	}
}

BOOL CFilterFieldMaxValueItem::OnRequestEdit(XTP_REPORTRECORDITEM_ARGS* pItemArgs)
{
	return ( m_pField->m_filterCondition == SAssetField::eAssertFilterCondition_InsideRange );
}

void CFilterFieldMaxValueItem::OnEditChanged(XTP_REPORTRECORDITEM_ARGS* pItemArgs, LPCTSTR szText)
{
	if( m_pField->m_filterCondition != SAssetField::eAssertFilterCondition_InsideRange )
	{
		return;
	}

	m_pField->m_maxFilterValue = szText;
	SetCaption( szText );
	SetTextColor( AssetBrowser::kChangedFieldFilterValueColor );
	SetBold();
	CAssetBrowserFilterPresetsDlg::AutoSaveCurrentFilterPreset();
}

void CFilterFieldMaxValueItem::OnInplaceButtonDown( CXTPReportInplaceButton* pButton )
{
	CRect rc, rcBtn;
	CMenu menu;

	if( !m_pField->m_bUseEnumValues )
		return;

	menu.CreatePopupMenu();

	//
	// add asset field enum items to the menu
	//
	for( size_t i = 0, iCount = m_pField->m_enumValues.size(); i < iCount; ++i )
	{
		// condition allowed, add it
		// we increment the position of the menu item by 1, so we can have 1-based indices, because TrackPopupMenu returns 0 if no item was chosen
		menu.AppendMenu( MF_STRING | MF_BYPOSITION, i + 1, m_pField->m_enumValues[i] );
	}

	rc = this->GetColumn()->GetRect();
	pButton->GetWindowRect( &rcBtn );
	this->GetColumn()->GetControl()->ClientToScreen( &rc );
	rc.top = rcBtn.bottom;

	UINT theChosenOne = menu.TrackPopupMenu( TPM_LEFTALIGN|TPM_RETURNCMD, rc.left, rc.top, this->GetColumn()->GetControl() );

	// decrement result, from 1-based index, so we can get our index into the enums vector 
	theChosenOne--;

	if( theChosenOne >= 0 && theChosenOne < m_pField->m_enumValues.size() )
	{
		m_pField->m_maxFilterValue = m_pField->m_enumValues[theChosenOne];
		SetCaption( m_pField->m_filterValue );
		SetTextColor( AssetBrowser::kChangedFieldFilterValueColor );
		SetBold();
		CAssetBrowserFilterPresetsDlg::AutoSaveCurrentFilterPreset();
		// set the new value to the current edit control
		this->GetColumn()->GetControl()->GetInplaceEdit()->SetWindowText( m_pField->m_maxFilterValue );
	}

	menu.DestroyMenu();
}

// CAssetBrowserFilterPresetsDlg dialog

IMPLEMENT_DYNAMIC(CAssetBrowserFilterPresetsDlg, CDialog)

CAssetBrowserFilterPresetsDlg::CAssetBrowserFilterPresetsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserFilterPresetsDlg::IDD, pParent)
{
	m_pAssetBrowserDlg = NULL;
	m_bDraggingField = false;
	m_hCommonFieldsTreeItem = 0;
	m_hDraggedTreeItem = 0;
	s_pFilterPresetsDlg = this;
}

CAssetBrowserFilterPresetsDlg::~CAssetBrowserFilterPresetsDlg()
{
}

void CAssetBrowserFilterPresetsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_PRESETS, m_cbPresets);
	DDX_Control(pDX, IDC_TREE_FIELDS, m_treeFields);
}

BEGIN_MESSAGE_MAP(CAssetBrowserFilterPresetsDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_PRESET_NEW, &CAssetBrowserFilterPresetsDlg::OnBnClickedButtonPresetNew)
	ON_BN_CLICKED(IDC_BUTTON_PRESET_UPDATE, &CAssetBrowserFilterPresetsDlg::OnBnClickedButtonPresetUpdate)
	ON_BN_CLICKED(IDC_BUTTON_PRESET_DELETE, &CAssetBrowserFilterPresetsDlg::OnBnClickedButtonPresetDelete)
	ON_BN_CLICKED(IDC_BUTTON_APPLY_FILTER_PRESET, &CAssetBrowserFilterPresetsDlg::OnBnClickedButtonApplyPresetNow)
	ON_BN_CLICKED(IDC_CHECK_AUTOSAVE_PRESET, &CAssetBrowserFilterPresetsDlg::OnBnClickedButtonAutoSavePreset)
	ON_CBN_SELCHANGE(IDC_COMBO_PRESETS, &CAssetBrowserFilterPresetsDlg::OnCbnSelchangeComboPresets)
	ON_BN_CLICKED(IDC_BUTTON_PRESET_RENAME, &CAssetBrowserFilterPresetsDlg::OnBnClickedButtonRenamePreset)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON_ADD_FIELDS, &CAssetBrowserFilterPresetsDlg::OnBnClickedButtonAddFields)
	ON_BN_CLICKED(IDC_BUTTON_ADD_ALL_FIELDS, &CAssetBrowserFilterPresetsDlg::OnBnClickedButtonAddAllFields)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_FIELDS, &CAssetBrowserFilterPresetsDlg::OnBnClickedButtonRemoveFields)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_ALL_FIELDS, &CAssetBrowserFilterPresetsDlg::OnBnClickedButtonRemoveAllFields)
	ON_BN_CLICKED(IDC_BUTTON_EXPAND_ALL, &CAssetBrowserFilterPresetsDlg::OnBnClickedButtonExpandAll)
	ON_BN_CLICKED(IDC_BUTTON_COLLAPSE_ALL, &CAssetBrowserFilterPresetsDlg::OnBnClickedButtonCollapseAll)
	ON_NOTIFY(TVN_BEGINDRAG, IDC_TREE_FIELDS, &CAssetBrowserFilterPresetsDlg::OnTvnBegindragTreeFields)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_FIELDS, &CAssetBrowserFilterPresetsDlg::OnNMDblclkTreeFields)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// CAssetBrowserFilterPresetsDlg message handlers
void CAssetBrowserFilterPresetsDlg::FillFieldsList()
{
	CRect	rc;
	UINT	crtY = 0;

	m_rptFields.ResetContent();

	for( size_t i = 0, iCount = m_fields.size(); i < iCount; ++i )
	{
		CXTPReportRecord* pRec = (CXTPReportRecord*) new CFilterFieldRecord( &m_fields[i] );

		// name
		CXTPReportRecordItem* pRecItem = pRec->GetItem( 0 );
		pRecItem->SetEditable( FALSE );

		// condition
		pRecItem = pRec->GetItem( 1 );
		pRecItem->SetEditable( TRUE );

		// value
		pRecItem = pRec->GetItem( 2 );
		pRecItem->SetEditable( TRUE );

		// max value
		pRecItem = pRec->GetItem( 3 );
		pRecItem->SetEditable( TRUE );

		// database name
		pRecItem = pRec->GetItem( 4 );
		pRecItem->SetEditable( FALSE );

		// field type
		pRecItem = pRec->GetItem( 5 );
		pRecItem->SetEditable( FALSE );

		m_rptFields.AddRecord( pRec );
	};

	m_rptFields.Populate();
}

void CAssetBrowserFilterPresetsDlg::FillPresets()
{
	m_cbPresets.ResetContent();

	for( TPresetNameAndPresetMap::iterator iter = m_fieldFiltersPresets.begin(), iterEnd = m_fieldFiltersPresets.end(); iter != iterEnd; ++iter )
	{
		m_cbPresets.AddString( iter->first );
	}
}

void CAssetBrowserFilterPresetsDlg::FillDatabaseFieldsTree()
{
	std::set<CString> currentFieldNames;

	m_treeFields.DeleteAllItems();
	m_specificFieldsForEachDatabase.clear();

	for( size_t i = 0, iCount = m_fields.size(); i < iCount; ++i )
	{
		currentFieldNames.insert( m_fields[i].m_displayName );
	}

	//
	// fill up the common fields
	//
	HTREEITEM hParent = m_treeFields.InsertItem( AssetBrowser::kCommonFieldsTreeItemText );
	m_treeFields.SetItemBold( hParent );
	m_treeFields.SetItemData( hParent, 0 );
	m_hCommonFieldsTreeItem = hParent;

	for( size_t i = 0, iCount = m_commonFieldNames.size(); i < iCount; ++i )
	{
		HTREEITEM hItem = m_treeFields.InsertItem( m_commonFields[i].m_displayName, hParent );
		m_treeFields.SetItemData( hItem, (DWORD)&m_commonFields[i] );

		// if is not in the current list, its available for dragging
		if( currentFieldNames.end() == currentFieldNames.find( m_commonFields[i].m_displayName ) )
		{
			m_treeFields.SetItemColor( hItem, AssetBrowser::kFieldTreeItemTextColor );
			m_treeFields.SetItemBold( hItem );
		}
		else
		{
			m_treeFields.SetItemColor( hItem, AssetBrowser::kUsedFieldInFilterTreeItemTextColor );
		}
	}

	//
	// for each database fill up the specific fields
	//
	for( size_t i = 0, iCount = m_pAssetBrowserDlg->m_cAssetDatabases.size(); i < iCount; ++i )
	{
		IAssetDisplayDatabase::TAssetFields& rFields = m_pAssetBrowserDlg->m_cAssetDatabases[i]->GetAssetFields();

		HTREEITEM hParent = m_treeFields.InsertItem( m_pAssetBrowserDlg->m_cAssetDatabases[i]->GetDatabaseName() );
		m_treeFields.SetItemBold( hParent );
		
		IAssetDisplayDatabase::TAssetFields specificFields;

		for( size_t j = 0, jCount = rFields.size(); j < jCount; ++j )
		{
			if( m_commonFieldNames.end() == std::find( m_commonFieldNames.begin(), m_commonFieldNames.end(), rFields[j].m_fieldName ) )
			{
				HTREEITEM hItem = m_treeFields.InsertItem( rFields[j].m_displayName, hParent );
				specificFields.push_back( rFields[j] );
				m_treeFields.SetItemData( hItem, (DWORD)( specificFields.size() - 1 ) );
				m_treeFields.SetItemColor( hItem, AssetBrowser::kFieldTreeItemTextColor );

				// if is not in the current list, its available for dragging
				if( currentFieldNames.end() == currentFieldNames.find( rFields[j].m_displayName ) )
				{
					m_treeFields.SetItemColor( hItem, AssetBrowser::kFieldTreeItemTextColor );
					m_treeFields.SetItemBold( hItem );
				}
				else
				{
					m_treeFields.SetItemColor( hItem, AssetBrowser::kUsedFieldInFilterTreeItemTextColor );
					m_treeFields.SetItemBold( hItem, FALSE );
				}

				specificFields[specificFields.size() - 1].m_parentDatabaseName = m_pAssetBrowserDlg->m_cAssetDatabases[i]->GetDatabaseName();
			}
		}

		m_specificFieldsForEachDatabase[CString( m_pAssetBrowserDlg->m_cAssetDatabases[i]->GetDatabaseName())] = specificFields;
	}
}

BOOL CAssetBrowserFilterPresetsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect rc;
	assert( m_pAssetBrowserDlg );

	if( !m_rptFields.SubclassDlgItem( IDC_FIELDS_LIST_REPORT, this ) )
	{
		AfxMessageBox( "Could not find report control in filter editing dialog" );

		return FALSE;
	}

	CheckDlgButton( IDC_CHECK_AUTOSAVE_PRESET, gSettings.sAssetBrowserSettings.bAutoSaveFilterPreset );

	m_rptFields.AddColumn( new CXTPReportColumn( 0, "Field name", 130 ) );
	m_rptFields.AddColumn( new CXTPReportColumn( 1, "Condition", 130 ) );
	m_rptFields.AddColumn( new CXTPReportColumn( 2, "Value", 130 ) );
	m_rptFields.AddColumn( new CXTPReportColumn( 3, "Max value", 130 ) );
	m_rptFields.AddColumn( new CXTPReportColumn( 4, "Database", 90 ) );
	m_rptFields.AddColumn( new CXTPReportColumn( 5, "Type", 60 ) );
	m_rptFields.AllowEdit( TRUE );

	m_rptFields.GetColumns()->GetAt( 1 )->GetEditOptions()->m_bConstraintEdit = TRUE;
	m_rptFields.GetColumns()->GetAt( 1 )->GetEditOptions()->m_bAllowEdit = FALSE;
	m_rptFields.GetColumns()->GetAt( 1 )->GetEditOptions()->AddComboButton( TRUE );

	m_rptFields.GetColumns()->GetAt( 2 )->GetEditOptions()->m_bAllowEdit = TRUE;
	m_rptFields.GetColumns()->GetAt( 2 )->GetEditOptions()->AddComboButton( TRUE );

	m_rptFields.GetColumns()->GetAt( 3 )->GetEditOptions()->m_bAllowEdit = TRUE;
	m_rptFields.GetColumns()->GetAt( 3 )->GetEditOptions()->AddComboButton( TRUE );
	
	m_rptFields.GetPaintManager()->SetGridStyle( TRUE, xtpReportGridSolid );
	m_rptFields.GetReportHeader()->AllowColumnRemove( FALSE );
	//m_rptFields.EnableDragDrop( _T("ReportDialog"), xtpReportAllowDrag | xtpReportAllowDrop );

	// load presets
	CAssetBrowserFilterPresetsDlg::LoadFieldFiltersPresets( m_fieldFiltersPresets );

	GatherCommonFieldsFromDatabases();

	// fill the presets combo list
	FillPresets();
	FillDatabaseFieldsTree();

	// select the first preset, if any
	if( !m_fieldFiltersPresets.empty() )
	{
		m_cbPresets.SetCurSel( 0 );
		OnCbnSelchangeComboPresets();
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

bool CAssetBrowserFilterPresetsDlg::LoadFieldFiltersPresets( TPresetNameAndPresetMap& rOutFilters, bool bClearMap )
{
	if( bClearMap )
	{
		rOutFilters.clear();
	}

	XmlNodeRef xmlRoot = GetISystem()->LoadXmlFromFile( AssetBrowser::kFieldFiltersPresetsFilename );
	XmlString xmlStr;
	std::vector<CString> strings;

	if( !xmlRoot )
		return false;

	for( size_t i = 0, iCount = xmlRoot->getChildCount(); i < iCount; ++i )
	{
		CFieldFiltersPreset preset;
		
		xmlRoot->getChild( i )->getAttr( "name", xmlStr );
		preset.m_presetName = xmlStr;

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

			preset.m_fields.push_back( field );
		}

		rOutFilters[preset.m_presetName] = preset;
	}

	return true;
}

bool CAssetBrowserFilterPresetsDlg::SaveFieldFiltersPresets( TPresetNameAndPresetMap& rFilters )
{
	CString			strTemp;
	XmlNodeRef	xmlRoot = XmlHelpers::CreateXmlNode( "filterPresets" );
	XmlString		xmlStr;

	if( !xmlRoot )
		return false;

	for( TPresetNameAndPresetMap::iterator iter = rFilters.begin(), iterEnd = rFilters.end(); iter != iterEnd; ++iter )
	{
		XmlNodeRef xmlFilterPreset = XmlHelpers::CreateXmlNode( "filterPreset" );
		xmlFilterPreset->setAttr( "name", iter->first );

		for( size_t i = 0, iCount = iter->second.m_fields.size(); i < iCount; ++i )
		{
			XmlNodeRef xmlFilter = XmlHelpers::CreateXmlNode( "fieldFilter" );
			xmlFilter->setAttr( "displayName", iter->second.m_fields[i].m_displayName );
			xmlFilter->setAttr( "fieldName", iter->second.m_fields[i].m_fieldName );
			xmlFilter->setAttr( "fieldType", (int)iter->second.m_fields[i].m_fieldType );
			xmlFilter->setAttr( "filterCondition", (int)iter->second.m_fields[i].m_filterCondition );
			xmlFilter->setAttr( "filterValue", iter->second.m_fields[i].m_filterValue );
			xmlFilter->setAttr( "maxFilterValue", iter->second.m_fields[i].m_maxFilterValue );
			xmlFilter->setAttr( "parentDB", iter->second.m_fields[i].m_parentDatabaseName );
			strTemp = "";
			JoinStrings( iter->second.m_fields[i].m_enumValues, strTemp );
			xmlFilter->setAttr( "enumValues", strTemp );
			xmlFilter->setAttr( "useEnumValues", (int)iter->second.m_fields[i].m_bUseEnumValues );

			xmlFilterPreset->addChild( xmlFilter );
		}

		xmlRoot->addChild( xmlFilterPreset );
	}

	bool bResult = xmlRoot->saveToFile( AssetBrowser::kFieldFiltersPresetsFilename ); 

	return bResult;
}

bool CAssetBrowserFilterPresetsDlg::AddFieldFilterPreset( const char* pPresetName )
{
	CFieldFiltersPreset newPreset;

	if( !strcmp( pPresetName, "" ) )
	{
		return false;
	}

	if( !GetFieldFilterPresetByName( pPresetName ) )
	{
		// insert a new one
		newPreset.m_presetName = pPresetName;
		newPreset.m_fields = m_fields;
		m_fieldFiltersPresets[pPresetName] = newPreset;

		return true;
	}

	return false;
}

bool CAssetBrowserFilterPresetsDlg::UpdateFieldFilterPreset( const char* pPresetName )
{
	if( !strcmp( pPresetName, "" ) )
	{
		return false;
	}

	if( m_fieldFiltersPresets.empty() )
	{
		return false;
	}

	if( !GetFieldFilterPresetByName( pPresetName ) )
	{
		// no existing preset with that name
		return false;
	}

	m_fieldFiltersPresets[pPresetName].m_fields = m_fields;

	return true;
}

bool CAssetBrowserFilterPresetsDlg::DeleteFieldFilterPreset( const char* pPresetName )
{
	m_fieldFiltersPresets.erase( m_fieldFiltersPresets.find( pPresetName ) );

	return true;
}

void CAssetBrowserFilterPresetsDlg::GatherCommonFieldsFromDatabases()
{
	m_commonFields.clear();
	m_commonFieldNames.clear();

	//
	// find the common fields to all selected databases
	//
	TAssetDatabases& dbs = m_pAssetBrowserDlg->m_cAssetDatabases;

	if( dbs.empty() )
	{
		return;
	}

	IAssetDisplayDatabase::TAssetFields& firstDbFields = dbs[0]->GetAssetFields();

	for( IAssetDisplayDatabase::TAssetFields::iterator iter = firstDbFields.begin(), iterEnd = firstDbFields.end(); iter != iterEnd; ++iter )
	{
		bool bAddField = true;

		for( size_t i = 1, iCount = dbs.size(); i < iCount; ++i )
		{
			IAssetDisplayDatabase::TAssetFields& fields = dbs[i]->GetAssetFields();

			// if field was not found, then do not add it, and exit from the loop
			if( !dbs[i]->GetAssetFieldByName( iter->m_fieldName ) )
			{
				bAddField = false;
				break;
			}
		}

		if( bAddField )
		{
			m_commonFields.push_back( *iter );
			// this field is a common field, so parent is -nothing-
			m_commonFields[m_commonFields.size() - 1].m_parentDatabaseName = "";
			m_commonFieldNames.push_back( iter->m_fieldName );
		}
	}
}

CFieldFiltersPreset* CAssetBrowserFilterPresetsDlg::GetFieldFilterPresetByName( const char* pPresetName )
{
	if( m_fieldFiltersPresets.end() == m_fieldFiltersPresets.find( pPresetName ) )
		return NULL;

	return &m_fieldFiltersPresets[pPresetName];
}

void CAssetBrowserFilterPresetsDlg::OnBnClickedButtonPresetDelete()
{
	CString presetName;

	m_cbPresets.GetWindowText( presetName );

	if( presetName != "" )
	{
		CString strMsg;

		strMsg.Format( "Delete the following filter preset: '%s' ?", presetName.GetBuffer() );

		if( AfxMessageBox( strMsg, MB_YESNO ) == IDYES )
		{
			if( DeleteFieldFilterPreset( presetName ) )
			{
				FillPresets();

				if( m_cbPresets.GetCount() )
				{
					m_cbPresets.SetCurSel( 0 );
				}

				OnCbnSelchangeComboPresets();
				CAssetBrowserFilterPresetsDlg::SaveFieldFiltersPresets( m_fieldFiltersPresets );
			}
		}
	}
}

void CAssetBrowserFilterPresetsDlg::OnBnClickedButtonPresetUpdate()
{
	if( !m_cbPresets.GetCount() )
	{
		IAssetDisplayDatabase::TAssetFields saveFields = m_fields;
		OnBnClickedButtonPresetNew();
		m_fields = saveFields;
	}

	SaveCurrentPreset();
	FillFieldsList();
}

void CAssetBrowserFilterPresetsDlg::SaveCurrentPreset()
{
	UpdateFieldFilterPreset( m_currentPresetName );
	CAssetBrowserFilterPresetsDlg::SaveFieldFiltersPresets( m_fieldFiltersPresets );
}

void CAssetBrowserFilterPresetsDlg::OnCbnSelchangeComboPresets()
{
	CString presetName;

	if( m_cbPresets.GetCount() )
	{
		m_cbPresets.GetLBText( m_cbPresets.GetCurSel(), presetName );
		m_currentPresetName = presetName;
		m_fields = m_fieldFiltersPresets[m_currentPresetName].m_fields;
	}
	else
	{
		m_currentPresetName = "";
		m_fields.clear();
	}

	FillFieldsList();
	FillDatabaseFieldsTree();
	OnBnClickedButtonExpandAll();
}

void CAssetBrowserFilterPresetsDlg::OnBnClickedButtonPresetNew()
{
	static int nPresetCounter = 1;
	CString presetName;
	CStringInputDialog dlgText;

	presetName.Format( "FilterPreset%02d", nPresetCounter++ );
	dlgText.SetText( presetName );
	dlgText.SetTitle( "Input preset name" );

	if( IDOK == dlgText.DoModal() )
	{
		presetName = dlgText.GetResultingText();

		if( AddFieldFilterPreset( presetName ) )
		{
			FillPresets();
			m_cbPresets.SetCurSel( m_cbPresets.GetCount() - 1 );
			OnCbnSelchangeComboPresets();
		}
	}

	CAssetBrowserFilterPresetsDlg::SaveFieldFiltersPresets( m_fieldFiltersPresets );
}

void CAssetBrowserFilterPresetsDlg::OnBnClickedButtonRenamePreset()
{
	CString presetName, oldName;
	CStringInputDialog dlgText;
	UINT selectedItem = 0;

	if( !m_cbPresets.GetCount() )
	{
		AfxMessageBox( "No preset to rename, please create one first, by pressing the 'New' button." );
		return;
	}

	m_cbPresets.GetLBText( m_cbPresets.GetCurSel(), oldName );
	selectedItem = m_cbPresets.GetCurSel();

	if( oldName.IsEmpty() )
		return;

	dlgText.SetText( oldName );
	dlgText.SetTitle( "Input new preset name" );

	if( IDOK == dlgText.DoModal() )
	{
		presetName = dlgText.GetResultingText();

		TPresetNameAndPresetMap::iterator iter = m_fieldFiltersPresets.find( oldName.GetBuffer() );

		if( iter != m_fieldFiltersPresets.end() )
		{
			CFieldFiltersPreset preset = iter->second;

			m_fieldFiltersPresets.erase( iter );
			preset.m_presetName = presetName;
			m_fieldFiltersPresets[presetName.GetBuffer()] = preset;
		}
		
		FillPresets();
		m_cbPresets.SetCurSel( selectedItem );
		OnCbnSelchangeComboPresets();
	}

	CAssetBrowserFilterPresetsDlg::SaveFieldFiltersPresets( m_fieldFiltersPresets );
}
void CAssetBrowserFilterPresetsDlg::OnBnClickedButtonAddFields()
{
	HTREEITEM hItem = m_treeFields.GetSelectedItem();

	AddFieldsFromTreeItem( hItem );
}

void CAssetBrowserFilterPresetsDlg::RemoveFieldFromList( const char* pFieldDisplayName )
{
	//
	// search this field in current fields
	//
	for( IAssetDisplayDatabase::TAssetFields::iterator iter = m_fields.begin(); iter != m_fields.end(); ++iter )
	{
		// if this is the field we are searching
		if( iter->m_displayName == pFieldDisplayName )
		{
			// this is a common field
			if( iter->m_parentDatabaseName == "" )
			{
				// if we have the common fields item as parent, lets add the field from the list
				if( m_hCommonFieldsTreeItem )
				{
					HTREEITEM hItem = m_treeFields.FindItem( iter->m_displayName, FALSE, TRUE, TRUE, m_hCommonFieldsTreeItem );
					COLORREF color = m_treeFields.GetItemColor( hItem );

					if( color == AssetBrowser::kUsedFieldInFilterTreeItemTextColor )
					{
						m_treeFields.SetItemColor( hItem, AssetBrowser::kFieldTreeItemTextColor );
						m_treeFields.SetItemBold( hItem );
						iter = m_fields.erase( iter );
						break;
					}
				}
			}
			else
				// this must be a field specific to a database
			{
				HTREEITEM hDbItem = m_treeFields.FindItem( iter->m_parentDatabaseName, FALSE, TRUE, TRUE, 0 );

				if( hDbItem )
				{
					HTREEITEM hItem = m_treeFields.FindItem( iter->m_displayName, FALSE, TRUE, TRUE, hDbItem );
					COLORREF color = m_treeFields.GetItemColor( hItem );

					if( color == AssetBrowser::kUsedFieldInFilterTreeItemTextColor )
					{
						m_treeFields.SetItemColor( hItem, AssetBrowser::kFieldTreeItemTextColor );
						m_treeFields.SetItemBold( hItem );
						iter = m_fields.erase( iter );
						break;
					}
				}
			}
		}
	}
}

void CAssetBrowserFilterPresetsDlg::OnBnClickedButtonRemoveFields()
{
	CString str;

	for( size_t i = 0, iCount = m_rptFields.GetRows()->GetCount(); i < iCount; ++i )
	{
		if( m_rptFields.GetRows()->GetAt( i )->IsSelected() )
		{
			str = m_rptFields.GetRows()->GetAt( i )->GetRecord()->GetItem( 0 )->GetCaption( m_rptFields.GetColumns()->GetAt( 0 ) );
			RemoveFieldFromList( str.GetBuffer() );
		}
	}

	AutoSaveCurrentFilterPreset();
	FillFieldsList();
}

void CAssetBrowserFilterPresetsDlg::OnBnClickedButtonRemoveAllFields()
{
	m_rptFields.ResetContent();
	m_fields.clear();
	FillDatabaseFieldsTree();
	OnBnClickedButtonExpandAll();
	AutoSaveCurrentFilterPreset();
}

void CAssetBrowserFilterPresetsDlg::OnTvnBegindragTreeFields(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	
	m_bDraggingField = true;
	m_bDraggedInsideList = false;
	m_hDraggedTreeItem = pNMTreeView->itemNew.hItem;
	SetCapture();

	*pResult = 0;
}

void CAssetBrowserFilterPresetsDlg::OnNMDblclkTreeFields(NMHDR *pNMHDR, LRESULT *pResult)
{
	HTREEITEM hItem = m_treeFields.GetSelectedItem();

	// only add single fields on double click, because the databases can expand/collapse on dobule click
	if( m_treeFields.GetParentItem( hItem ) )
	{
		AddFieldsFromTreeItem( hItem );
		AutoSaveCurrentFilterPreset();
	}
	
	*pResult = 0;
}

void CAssetBrowserFilterPresetsDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if( m_bDraggingField )
	{
		CRect rc;

		ClientToScreen( &point );
		m_rptFields.GetWindowRect( &rc );

		if( rc.PtInRect( point ) )
		{
			m_bDraggedInsideList = true;
			::SetCursor( ::LoadCursor( AfxGetApp()->m_hInstance, MAKEINTRESOURCE( IDC_POINTER_DRAG_ITEM ) ) );
		}
		else
		{
			m_bDraggedInsideList = false;
			::SetCursor( ::LoadCursor( NULL, IDC_NO ) );
		}
	}

	CDialog::OnMouseMove(nFlags, point);
}

void CAssetBrowserFilterPresetsDlg::AddFieldsFromTreeItem( HTREEITEM hItem )
{
	if( !hItem )
		return;

	HTREEITEM hParent = m_treeFields.GetParentItem( hItem );
	DWORD itemData = m_treeFields.GetItemData( hItem );
	IAssetDisplayDatabase::TAssetFields fieldsToAdd;
	bool bDraggedOneItem = false;

	// add the common fields
	if( hItem == m_hCommonFieldsTreeItem )
	{
		fieldsToAdd = m_commonFields;
		HTREEITEM hChildItem  = m_treeFields.GetChildItem( hItem );

		// colorize child items as moved into the list
		while( hChildItem != NULL )
		{
			m_treeFields.SetItemColor( hChildItem, AssetBrowser::kUsedFieldInFilterTreeItemTextColor );
			m_treeFields.SetItemBold( hChildItem, FALSE );
			hChildItem = m_treeFields.GetNextItem( hChildItem, TVGN_NEXT );
		}
	}
	else
	// if its a database
	if( !hParent )
	{
		CString dbName = m_treeFields.GetItemText( hItem );
		fieldsToAdd = m_specificFieldsForEachDatabase[dbName.GetBuffer()];

		HTREEITEM hChildItem  = m_treeFields.GetChildItem( hItem );

		// colorize child items as moved into the list
		while( hChildItem != NULL )
		{
			m_treeFields.SetItemColor( hChildItem, AssetBrowser::kUsedFieldInFilterTreeItemTextColor );
			m_treeFields.SetItemBold( hChildItem, FALSE );
			hChildItem = m_treeFields.GetNextItem( hChildItem, TVGN_NEXT );
		}
	}
	// add a single field from a node
	else
	{
		if( hParent != m_hCommonFieldsTreeItem )
		{
			CString dbName = m_treeFields.GetItemText( hParent );
			fieldsToAdd.push_back( m_specificFieldsForEachDatabase[dbName.GetBuffer()][itemData] );
		}
		else
		if( hParent == m_hCommonFieldsTreeItem )
		{
			fieldsToAdd.push_back( *((SAssetField*)itemData) );
		}

		bDraggedOneItem = true;
	}

	if( !fieldsToAdd.empty() )
	{
		// add the dragged fields
		for( size_t i = 0, iCount = fieldsToAdd.size(); i < iCount; ++i )
		{
			bool bAdd = true;

			for( size_t j = 0, jCount = m_fields.size(); j < jCount; ++j )
			{
				if( m_fields[j].m_fieldName == fieldsToAdd[i].m_fieldName )
				{
					bAdd = false;
					break;
				}
			}

			if( bAdd )
			{
				m_fields.push_back( fieldsToAdd[i] );
			}
		}

		// change color of one item dragged. databases and common field roots do not change color
		if( bDraggedOneItem )
		{
			m_treeFields.SetItemColor( hItem, AssetBrowser::kUsedFieldInFilterTreeItemTextColor );
			m_treeFields.SetItemBold( hItem, FALSE );
		}

		FillFieldsList();
	}
}

void CAssetBrowserFilterPresetsDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if( m_hDraggedTreeItem && m_bDraggedInsideList )
	{
		AddFieldsFromTreeItem( m_hDraggedTreeItem );
		AutoSaveCurrentFilterPreset();
	}

	m_bDraggingField = false;
	m_hDraggedTreeItem = 0;
	ReleaseCapture();

	CDialog::OnLButtonUp(nFlags, point);
}

void CAssetBrowserFilterPresetsDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	if( m_hDraggedTreeItem && m_bDraggedInsideList )
	{
		AddFieldsFromTreeItem( m_hDraggedTreeItem );
		AutoSaveCurrentFilterPreset();
	}

	m_bDraggingField = false;
	m_hDraggedTreeItem = 0;
	ReleaseCapture();

	CDialog::OnRButtonUp(nFlags, point);
}


void CAssetBrowserFilterPresetsDlg::OnBnClickedButtonAddAllFields()
{
	m_rptFields.ResetContent();
 
	for( size_t i = 0, iCount = m_commonFields.size(); i < iCount; ++i )
	{
		bool bAlreadyAdded = false;

		for( size_t j = 0, jCount = m_fields.size(); j < jCount; ++j )
		{
			if( m_fields[j].m_fieldName == m_commonFields[i].m_fieldName )
			{
				bAlreadyAdded = true;
				break;
			}
		}

		if( bAlreadyAdded )
			continue;

		m_fields.push_back( m_commonFields[i] );
	}
	
	for( std::map<CString,IAssetDisplayDatabase::TAssetFields>::iterator iter = m_specificFieldsForEachDatabase.begin(); iter != m_specificFieldsForEachDatabase.end(); ++iter )
	{
		for( size_t i = 0, iCount = iter->second.size(); i < iCount; ++i )
		{
			bool bAlreadyAdded = false;

			for( size_t j = 0, jCount = m_fields.size(); j < jCount; ++j )
			{
				if( m_fields[j].m_fieldName == iter->second[i].m_fieldName )
				{
					bAlreadyAdded = true;
					break;
				}
			}

			if( bAlreadyAdded )
				continue;

			m_fields.push_back( iter->second[i] );
		}
	}

	FillFieldsList();
	FillDatabaseFieldsTree();
	OnBnClickedButtonExpandAll();
	AutoSaveCurrentFilterPreset();
}

void CAssetBrowserFilterPresetsDlg::OnBnClickedButtonExpandAll()
{
	HTREEITEM hItem;

	hItem = m_treeFields.GetFirstVisibleItem();

	while( hItem != NULL )
	{
		m_treeFields.Expand( hItem, TVE_EXPAND );
		hItem = m_treeFields.GetNextItem( hItem, TVGN_NEXT );
	}
}

void CAssetBrowserFilterPresetsDlg::OnBnClickedButtonCollapseAll()
{
	HTREEITEM hItem;

	hItem = m_treeFields.GetFirstVisibleItem();

	while( hItem != NULL )
	{
		m_treeFields.Expand( hItem, TVE_COLLAPSE );
		hItem = m_treeFields.GetNextItem( hItem, TVGN_NEXT );
	}
}

void CAssetBrowserFilterPresetsDlg::OnBnClickedButtonApplyPresetNow()
{
	AutoSaveCurrentFilterPreset();
	m_pAssetBrowserDlg->ActivateFilterPreset( m_currentPresetName );
}

void CAssetBrowserFilterPresetsDlg::OnBnClickedButtonAutoSavePreset()
{
	gSettings.sAssetBrowserSettings.bAutoSaveFilterPreset = IsDlgButtonChecked( IDC_CHECK_AUTOSAVE_PRESET ) == BST_CHECKED;
}

BOOL CAssetBrowserFilterPresetsDlg::PreTranslateMessage(MSG* pMsg)
{
	if( pMsg->message == WM_KEYDOWN )
	{
		UINT nChar = pMsg->wParam;
		
		if( nChar == VK_DELETE && m_rptFields.HasFocus() )
		{
			OnBnClickedButtonRemoveFields();
		}

		if( nChar == VK_INSERT && ( &m_treeFields == GetFocus() ) )
		{
			HTREEITEM hItem = m_treeFields.GetSelectedItem();

			if( m_treeFields.GetParentItem( hItem ) )
			{
				AddFieldsFromTreeItem( hItem );
				AutoSaveCurrentFilterPreset();
			}
		}

		if( nChar == VK_DELETE && ( &m_treeFields == GetFocus() ) )
		{
			HTREEITEM hItem = m_treeFields.GetSelectedItem();

			if( hItem && m_treeFields.GetParentItem( hItem ) )
			{
				CString strTmp = m_treeFields.GetItemText( hItem );
				RemoveFieldFromList( strTmp.GetBuffer() );
				FillFieldsList();
				AutoSaveCurrentFilterPreset();
			}
		}
	}

	if (pMsg->message == WM_CLOSE)
	{
		EndDialog(0);
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CAssetBrowserFilterPresetsDlg::AutoSaveCurrentFilterPreset()
{
	if( gSettings.sAssetBrowserSettings.bAutoSaveFilterPreset )
	{
		s_pFilterPresetsDlg->SaveCurrentPreset();
	}
}