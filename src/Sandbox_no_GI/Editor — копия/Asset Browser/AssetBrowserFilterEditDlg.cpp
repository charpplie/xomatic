// AssetBrowserFilterEditDlg.cpp : implementation file
//

#include "stdafx.h"
#include "AssetBrowserFilterEditDlg.h"
#include "AssetBrowserSearchDlg.h"

// CAssetBrowserFilterEditDlg dialog

IMPLEMENT_DYNAMIC(CAssetBrowserFilterEditDlg, CDialog)

CAssetBrowserFilterEditDlg::TSearchFilterConditions		CAssetBrowserFilterEditDlg::m_scConditions;
CAssetBrowserFilterEditDlg::TStringOnlyConditionsSet	CAssetBrowserFilterEditDlg::m_ssStringConditions;

CAssetBrowserFilterEditDlg::CAssetBrowserFilterEditDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserFilterEditDlg::IDD, pParent)
{
	if( m_scConditions.empty() )
	{
		m_scConditions.push_back( std::make_pair( "any", SAssetField::eAssertFilterCondition_Any ) );
		m_scConditions.push_back( std::make_pair( "contains", SAssetField::eAssertFilterCondition_Contains ) );
		m_scConditions.push_back( std::make_pair( "match", SAssetField::eAssertFilterCondition_Match ) );
		m_scConditions.push_back( std::make_pair( "starts with", SAssetField::eAssertFilterCondition_StartsWith ) );
		m_scConditions.push_back( std::make_pair( "ends with", SAssetField::eAssertFilterCondition_EndsWith ) );
		m_scConditions.push_back( std::make_pair( "equal to", SAssetField::eAssertFilterCondition_Equal ) );
		m_scConditions.push_back( std::make_pair( "greater than", SAssetField::eAssertFilterCondition_Greater ) );
		m_scConditions.push_back( std::make_pair( "less than", SAssetField::eAssertFilterCondition_Less ) );
		m_scConditions.push_back( std::make_pair( "greater or equal than", SAssetField::eAssertFilterCondition_GreaterOrEqual ) );
		m_scConditions.push_back( std::make_pair( "less or equal than", SAssetField::eAssertFilterCondition_LessOrEqual ) );
		m_scConditions.push_back( std::make_pair( "not equal to", SAssetField::eAssertFilterCondition_Not ) );
		m_scConditions.push_back( std::make_pair( "inside range", SAssetField::eAssertFilterCondition_InsideRange ) );

		// add the condition types available only for the strings/text fields
		m_ssStringConditions.insert( SAssetField::eAssertFilterCondition_Contains );
		m_ssStringConditions.insert( SAssetField::eAssertFilterCondition_Match );
		m_ssStringConditions.insert( SAssetField::eAssertFilterCondition_StartsWith );
		m_ssStringConditions.insert( SAssetField::eAssertFilterCondition_EndsWith );
	}
}

CAssetBrowserFilterEditDlg::~CAssetBrowserFilterEditDlg()
{
}

void CAssetBrowserFilterEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_FIELD_LABEL, m_chkFieldname);
	DDX_Control(pDX, IDC_COMBO_CONDITION, m_cbCondition);
	DDX_Control(pDX, IDC_COMBO_FILTER_VALUE, m_cbValue);
	DDX_Control(pDX, IDC_COMBO_FILTER_MAXVALUE, m_cbMaxValue);
}


BEGIN_MESSAGE_MAP(CAssetBrowserFilterEditDlg, CDialog)
	ON_BN_CLICKED(IDC_CHECK_FIELD_LABEL, &CAssetBrowserFilterEditDlg::OnBnClickedCheckFieldLabel)
	ON_CBN_SELCHANGE(IDC_COMBO_CONDITION, &CAssetBrowserFilterEditDlg::OnCbnSelchangeComboCondition)
	ON_CBN_EDITCHANGE(IDC_COMBO_FILTER_VALUE, &CAssetBrowserFilterEditDlg::OnCbnEditchangeComboFilterValue)
	ON_CBN_EDITCHANGE(IDC_COMBO_FILTER_MAXVALUE, &CAssetBrowserFilterEditDlg::OnCbnEditchangeComboFilterMaxvalue)
	ON_CBN_SELCHANGE(IDC_COMBO_FILTER_VALUE, &CAssetBrowserFilterEditDlg::OnCbnSelchangeComboFilterValue)
	ON_CBN_SELCHANGE(IDC_COMBO_FILTER_MAXVALUE, &CAssetBrowserFilterEditDlg::OnCbnSelchangeComboFilterMaxvalue)
END_MESSAGE_MAP()


// CAssetBrowserFilterEditDlg message handlers

void CAssetBrowserFilterEditDlg::OnCancel()
{
	//CDialog::OnCancel();
}

void CAssetBrowserFilterEditDlg::OnOK()
{
	//CDialog::OnOK();
}

void CAssetBrowserFilterEditDlg::OnBnClickedCheckFieldLabel()
{
	BOOL bEnable = ( BST_CHECKED == IsDlgButtonChecked( IDC_CHECK_FIELD_LABEL ) );

	m_cbCondition.EnableWindow( bEnable );
	m_cbValue.EnableWindow( bEnable );
	m_cbMaxValue.EnableWindow( bEnable );
	m_pFilterField->m_bEnableFilter = bEnable;
	m_pSearchDlg->SetTimer( ID_TIMER_SEARCH_BY_FILENAME, kAssetBrowser_TimerSearchByFilenameDelay, NULL );
}

void CAssetBrowserFilterEditDlg::OnCbnSelchangeComboCondition()
{
	CString str;

	m_cbCondition.GetLBText( m_cbCondition.GetCurSel(), str );

	for( size_t i = 0, iCount = m_scConditions.size(); i < iCount; ++i )
	{
		if( m_scConditions[i].first == (const char*)str )
		{
			m_pFilterField->m_filterCondition = m_scConditions[i].second;
			break;
		}
	}

	if( SAssetField::eAssertFilterCondition_InsideRange == m_pFilterField->m_filterCondition )
	{
		CString str;

		// put the min value in max value, if is max is empty
		m_cbMaxValue.GetWindowText( str );
		
		if( str.IsEmpty() )
		{
			m_cbMaxValue.SetWindowText( str );
		}

		m_cbMaxValue.ShowWindow( SW_SHOW );
	}
	else
	{
		m_cbMaxValue.ShowWindow( SW_HIDE );
	}
	
	if( SAssetField::eAssertFilterCondition_Any == m_pFilterField->m_filterCondition )
	{
		m_cbValue.ShowWindow( SW_HIDE );
		m_cbMaxValue.ShowWindow( SW_HIDE );
	}
	else
	{
		m_cbValue.ShowWindow( SW_SHOW );
	}

	m_pSearchDlg->SetTimer( ID_TIMER_SEARCH_BY_FILENAME, kAssetBrowser_TimerSearchByFilenameDelay, NULL );
}

void CAssetBrowserFilterEditDlg::OnCbnEditchangeComboFilterValue()
{
	CString str;

	m_cbValue.GetWindowText( str );
	m_pFilterField->m_filterValue = str;
	m_pSearchDlg->SetTimer( ID_TIMER_SEARCH_BY_FILENAME, kAssetBrowser_TimerSearchByFilenameDelay, NULL );
}

void CAssetBrowserFilterEditDlg::OnCbnEditchangeComboFilterMaxvalue()
{
	CString str;

	m_cbMaxValue.GetWindowText( str );
	m_pFilterField->m_maxFilterValue = str;
	m_pSearchDlg->SetTimer( ID_TIMER_SEARCH_BY_FILENAME, kAssetBrowser_TimerSearchByFilenameDelay, NULL );
}

void CAssetBrowserFilterEditDlg::OnCbnSelchangeComboFilterValue()
{
	CString str;

	m_cbValue.GetLBText( m_cbValue.GetCurSel(), str );
	m_pFilterField->m_filterValue = str;
	m_pSearchDlg->SetTimer( ID_TIMER_SEARCH_BY_FILENAME, kAssetBrowser_TimerSearchByFilenameDelay, NULL );
}

void CAssetBrowserFilterEditDlg::OnCbnSelchangeComboFilterMaxvalue()
{
	CString str;

	m_cbMaxValue.GetLBText( m_cbMaxValue.GetCurSel(), str );
	m_cbMaxValue.SetWindowText( str );
	OnCbnEditchangeComboFilterMaxvalue();
}

BOOL CAssetBrowserFilterEditDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	ASSERT(m_pFilterField);

	if( m_pFilterField )
	{
		for( size_t i = 0, iCount = m_scConditions.size(); i < iCount; ++i )
		{
			// if this field has not the String type, and the current condition is string-related, then skip it 
			if( SAssetField::eAssetFieldType_String != m_pFilterField->m_fieldType &&
					m_ssStringConditions.end() != m_ssStringConditions.find( m_scConditions[i].second ) )
			{
				continue;
			}

			m_cbCondition.AddString( m_scConditions[i].first.c_str() );
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
