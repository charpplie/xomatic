// AssetBrowserSearchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "AssetBrowserSearchDlg.h"
#include "AssetBrowserDialog.h"
#include "AssetBrowserFilterEditDlg.h"

const UINT	kAssetBrowser_SearchPanelOpenedHeight = 318;
const UINT	kAssetBrowser_SearchPanelClosedHeight = 53;
const UINT	kAssetBrowser_SearchFileFontSize = 120;
const char* kAssetBrowser_AllDatabasesText = "(All databases)";

// CAssetBrowserSearchDlg dialog

IMPLEMENT_DYNAMIC(CAssetBrowserSearchDlg, CDialog)

CAssetBrowserSearchDlg::CAssetBrowserSearchDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserSearchDlg::IDD, pParent)
{

}

CAssetBrowserSearchDlg::~CAssetBrowserSearchDlg()
{
	DestroyFilterDialogs();
	m_fntSearchText.DeleteObject();
}

void CAssetBrowserSearchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_SEARCH_TEXT, m_edSearchText);
	DDX_Control(pDX, IDC_COMBO_FILENAME_CONDITION, m_cbFilenameFilterCondition);
	DDX_Control(pDX, IDC_COMBO_DATABASE, m_cbDatabase);
	DDX_Control(pDX, IDC_COMBO_THUMB_SIZE, m_cbThumbSize);
}

BEGIN_MESSAGE_MAP(CAssetBrowserSearchDlg, CDialog)
	ON_BN_CLICKED(IDC_CHECK_SHOW_FILTERS, &CAssetBrowserSearchDlg::OnBnClickedCheckShowFilters)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, &CAssetBrowserSearchDlg::OnBnClickedButtonClear)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_FILTERS, &CAssetBrowserSearchDlg::OnBnClickedCheckEnableFilters)
	ON_WM_SIZE()
	ON_EN_CHANGE(IDC_EDIT_SEARCH_TEXT, &CAssetBrowserSearchDlg::OnEnChangeEditSearchText)
	ON_WM_TIMER()
	ON_CBN_SELCHANGE(IDC_COMBO_FILENAME_CONDITION, &CAssetBrowserSearchDlg::OnCbnSelchangeComboFilenameCondition)
	ON_CBN_SELCHANGE(IDC_COMBO_DATABASE, &CAssetBrowserSearchDlg::OnCbnSelchangeComboDatabase)
	ON_CBN_SELCHANGE(IDC_COMBO_THUMB_SIZE, &CAssetBrowserSearchDlg::OnCbnSelchangeComboThumbSize)
END_MESSAGE_MAP()

// CAssetBrowserSearchDlg message handlers

void CAssetBrowserSearchDlg::CreateFilterDialogs()
{
	DestroyFilterDialogs();

	CRect rc;
	UINT crtY = 0;

	for( IAssetDisplayDatabase::TAssetFields::iterator iter = m_fields.begin(), iterEnd = m_fields.end(); iter != iterEnd; ++iter )
	{
		// skip filename field, we do it in the main edit of search dialog
		if( iter->m_fieldName == "filename" )
		{
			continue;
		}

		CAssetBrowserFilterEditDlg* pDlg = new CAssetBrowserFilterEditDlg();

		pDlg->m_pFilterField = &(*iter);
		pDlg->m_pSearchDlg = this;
		pDlg->Create( CAssetBrowserFilterEditDlg::IDD, &m_fieldEditorsContainer );
		pDlg->GetClientRect( &rc );
		pDlg->SetWindowPos( 0, 0, crtY, 0, 0, SWP_NOSIZE );
		pDlg->m_chkFieldname.SetWindowText( iter->m_displayName.c_str() );
		
		if( iter->m_bUseEnumValues )
		{
			for( size_t j = 0, jCount = iter->m_enumValues.size(); j < jCount; ++j )
			{
				pDlg->m_cbValue.AddString( iter->m_enumValues[j].first.c_str() );
				pDlg->m_cbMaxValue.AddString( iter->m_enumValues[j].first.c_str() );
			}
		}
		
		pDlg->ShowWindow( SW_SHOW );
		crtY += rc.Height();
		m_filterDlgs.push_back( pDlg );
	}

	m_clientSize.SetSize( rc.Width(), crtY );
	m_fieldEditorsContainer.SetClientSize( m_clientSize.cx, m_clientSize.cy );

	Invalidate();
}

void CAssetBrowserSearchDlg::DestroyFilterDialogs()
{
	for( size_t i = 0, iCount = m_filterDlgs.size(); i < iCount; ++i )
	{
		m_filterDlgs[i]->DestroyWindow();
		delete m_filterDlgs[i];
	}

	m_filterDlgs.resize( 0 );
}

void CAssetBrowserSearchDlg::OnBnClickedCheckShowFilters()
{
	CRect rc;

	GetClientRect( &rc );

	if( IsDlgButtonChecked( IDC_CHECK_SHOW_FILTERS ) )
	{
		SetWindowPos( 0, 0, 0, rc.Width(), kAssetBrowser_SearchPanelOpenedHeight, SWP_NOMOVE );
		m_pAssetBrowserDlg->m_pDockPaneSearch->SetMinTrackSize( CSize( 1, kAssetBrowser_SearchPanelOpenedHeight ) );
		m_pAssetBrowserDlg->m_pDockPaneSearch->SetMaxTrackSize( CSize( 10000, kAssetBrowser_SearchPanelOpenedHeight ) );
		m_pAssetBrowserDlg->GetDockingPaneManager()->RecalcFramesLayout();
		m_pAssetBrowserDlg->m_pDockPaneSearch->SetMinTrackSize( CSize( 1, 1 ) );
		m_pAssetBrowserDlg->m_pDockPaneSearch->SetMaxTrackSize( CSize( 10000, 10000 ) );
	}
	else
	{
		SetWindowPos( 0, 0, 0, rc.Width(), kAssetBrowser_SearchPanelClosedHeight, SWP_NOMOVE );
		m_pAssetBrowserDlg->m_pDockPaneSearch->SetMinTrackSize( CSize( 1, kAssetBrowser_SearchPanelClosedHeight ) );
		m_pAssetBrowserDlg->m_pDockPaneSearch->SetMaxTrackSize( CSize( 10000, kAssetBrowser_SearchPanelClosedHeight ) );
		m_pAssetBrowserDlg->GetDockingPaneManager()->RecalcFramesLayout();
		m_pAssetBrowserDlg->m_pDockPaneSearch->SetMinTrackSize( CSize( 1, 1 ) );
		m_pAssetBrowserDlg->m_pDockPaneSearch->SetMaxTrackSize( CSize( 10000, 10000 ) );
	}
}

void CAssetBrowserSearchDlg::OnBnClickedButtonClear()
{
	m_edSearchText.SetWindowText( "" );
	m_edSearchText.SetFocus();
	m_cbFilenameFilterCondition.SetCurSel( 1 );
	SetTimer( ID_TIMER_SEARCH_BY_FILENAME, kAssetBrowser_TimerSearchByFilenameDelay, NULL );
}

void CAssetBrowserSearchDlg::OnBnClickedCheckEnableFilters()
{
	SetTimer( ID_TIMER_SEARCH_BY_FILENAME, kAssetBrowser_TimerSearchByFilenameDelay, NULL );
}

BOOL CAssetBrowserSearchDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_fntSearchText.CreatePointFont( kAssetBrowser_SearchFileFontSize, "Arial Bold" );
	m_edSearchText.SetFont( &m_fntSearchText );

	CRect rc;

	CWnd* pFitBox = GetDlgItem( IDC_STATIC_FIELD_LIST_PLACEHOLDER );
	CWnd* pParent = GetDlgItem( IDC_STATIC_FILTERS );
	assert( pFitBox );
	assert( pParent );

	if( pFitBox && pParent )
	{
		pFitBox->GetWindowRect( &rc );
		pParent->ScreenToClient( &rc );
		m_fieldEditorsContainer.Create( NULL, "AssetFieldEditorsScrollWindow", WS_CHILD | WS_VISIBLE, rc, pParent, 0 );
		m_fieldEditorsContainer.SetAutoScrollWindowFlag( TRUE );
	}

	m_cbFilenameFilterCondition.SetCurSel( 1 );
	CheckDlgButton( IDC_CHECK_ENABLE_FILTERS, BST_CHECKED );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAssetBrowserSearchDlg::FillDatabases()
{
	assert( m_pAssetBrowserDlg );

	if( !m_pAssetBrowserDlg )
		return;

	m_cbDatabase.ResetContent();
	m_cbDatabase.AddString( kAssetBrowser_AllDatabasesText );
	m_cbDatabase.SetCurSel( 0 );

	// fill database names
	for( size_t i = 0, iCount = m_pAssetBrowserDlg->m_cAssetDatabases.size(); i < iCount; ++i )
	{
		m_cbDatabase.AddString( m_pAssetBrowserDlg->m_cAssetDatabases[i]->GetDatabaseName() );
	}
}

void CAssetBrowserSearchDlg::OnCancel()
{
	// empty to prevent dialog closing on Esc key press
}

void CAssetBrowserSearchDlg::OnOK()
{
	// empty to prevent dialog closing on Enter key press
}

void CAssetBrowserSearchDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	CRect rc;

	CWnd* pFiltersFrame = GetDlgItem( IDC_STATIC_FILTERS );

	if( !pFiltersFrame )
		return;

	pFiltersFrame->GetWindowRect( &rc );
	ScreenToClient( &rc );
	pFiltersFrame->SetWindowPos( 0, 0, 0, cx - rc.left*2, cy - rc.top - rc.left*2, SWP_NOMOVE );
	m_fieldEditorsContainer.GetWindowRect( &rc );
	ScreenToClient( &rc );
	m_fieldEditorsContainer.SetWindowPos( 0, 0, 0, rc.Width(), cy - rc.top - rc.left*2, SWP_NOMOVE );
}

void CAssetBrowserSearchDlg::OnEnChangeEditSearchText()
{
	SetTimer( ID_TIMER_SEARCH_BY_FILENAME, kAssetBrowser_TimerSearchByFilenameDelay, NULL );
}

void CAssetBrowserSearchDlg::OnTimer(UINT_PTR nIDEvent)
{
	if( nIDEvent == ID_TIMER_SEARCH_BY_FILENAME )
	{
		KillTimer( ID_TIMER_SEARCH_BY_FILENAME );
		assert( m_pAssetBrowserDlg );

		if( !m_pAssetBrowserDlg )
			return;

		IAssetDisplayDatabase::TAssetFieldFiltersMap fields;

		for( size_t i = 0, iCount = m_fields.size(); i < iCount; ++i )
		{
			fields[m_fields[i].m_fieldName] = m_fields[i];

			if( BST_UNCHECKED == IsDlgButtonChecked( IDC_CHECK_ENABLE_FILTERS ) )
			{
				fields[m_fields[i].m_fieldName].m_bEnableFilter = false;
			}
		}

		CString searchText;

		m_edSearchText.GetWindowText( searchText );
		fields["filename"].m_filterValue = searchText;
		fields["filename"].m_bEnableFilter = true;
		//TODO: maybe check a map with strings-conditionType instead index
		fields["filename"].m_filterCondition = (SAssetField::EAssetFilterCondition)m_cbFilenameFilterCondition.GetCurSel();
		m_pAssetBrowserDlg->ApplyFilters( fields );
	}

	CDialog::OnTimer(nIDEvent);
}

void CAssetBrowserSearchDlg::OnCbnSelchangeComboFilenameCondition()
{
	SetTimer( ID_TIMER_SEARCH_BY_FILENAME, kAssetBrowser_TimerSearchByFilenameDelay, NULL );
}

void CAssetBrowserSearchDlg::OnCbnSelchangeComboDatabase()
{
	CString dbName;

	if( LB_ERR != m_cbDatabase.GetCurSel() )
	{
		m_cbDatabase.GetLBText( m_cbDatabase.GetCurSel(), dbName );
		m_pAssetBrowserDlg->ShowDatabase( dbName == kAssetBrowser_AllDatabasesText ? "" : dbName );
		SetTimer( ID_TIMER_SEARCH_BY_FILENAME, kAssetBrowser_TimerSearchByFilenameDelay, NULL );
	}
}

void CAssetBrowserSearchDlg::OnCbnSelchangeComboThumbSize()
{
	CString str;

	m_cbThumbSize.GetLBText( m_cbThumbSize.GetCurSel(), str );
	m_pAssetBrowserDlg->m_oAssetViewer.SetAssetThumbSize( atoi( str ) );
}