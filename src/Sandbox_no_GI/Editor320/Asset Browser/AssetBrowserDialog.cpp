#include "stdafx.h"
#include "AssetBrowserDialog.h"
#include "Include/IAssetDisplayDatabase.h"
#include "Visual Budget System/VisualBudgetSystem.h"
#include "Include/IAssetDisplay.h"

#define IDC_DOCKING_PANE 1000
#define IDC_BUDGET_PANE 1001
#define IDW_ASSET_LIST_VIEW ( AFX_IDW_CONTROLBAR_FIRST + 100 )
#define IDW_ASSET_VIEW_THUMBS ( AFX_IDW_CONTROLBAR_FIRST + 101 )
#define IDW_ASSET_BROWSER_SEARCH_PANE ( AFX_IDW_CONTROLBAR_FIRST + 102 )
#define ID_TIMER_REFRESH_OPERATION_INFO 1

//! the maximum cached asset count pool when browsing through the list view assets
//TODO: put this in UI and in gSettings ?
const int kAssetBrowser_MaxRecentListViewAssetPoolSize = 1000;
const COLORREF kAssetBrowser_ListViewOddRowColor = RGB( 215, 215, 215 );
const COLORREF kAssetBrowser_ListViewEvenRowColor = RGB( 235, 235, 235);
const COLORREF kAssetBrowser_ListViewTextColor = RGB( 0, 0, 0);

CAssetListReportHeader::CAssetListReportHeader(CAssetBrowserDialog* pAssetBrowserDlg, CXTPReportControl* pCtrl, CXTPReportColumns* pCols ) : CXTPReportHeader( pCtrl, pCols )
{
	m_pAssetBrowserDlg = pAssetBrowserDlg;
}

void CAssetListReportHeader::OnLButtonUp(UINT nFlags, CPoint ptClick)
{
	__super::OnLButtonUp( nFlags, ptClick );
	CString str;

	CXTPReportColumn* pColumn = HitTest( ptClick );

	if( pColumn && m_pAssetBrowserDlg )
	{
		m_pAssetBrowserDlg->SortAssets( pColumn->GetInternalName(), pColumn->IsSortedDecreasing() );
	}
}

//---

#define ID_ASSET_BROWSER_REPORT_CONTROL_TIMER 1

const int kAssetBrowserReportCtrl_CacheTimerDelay = 50;

BEGIN_MESSAGE_MAP(CAssetBrowserReportControl, CXTPReportControl)
	ON_WM_CREATE()
	ON_WM_TIMER()
END_MESSAGE_MAP()

CAssetBrowserReportControl::CAssetBrowserReportControl() : CXTPReportControl()
{
	m_pAssetBrowserDlg = NULL;
}

void CAssetBrowserReportControl::OnSelectionChanged()
{
	assert( m_pAssetBrowserDlg );
	UINT index = 0;

	if( !m_pAssetBrowserDlg )
		return;

	if( GetSelectedRows() )
	{
		POSITION pos = GetSelectedRows()->GetFirstSelectedRowPosition();
		index = (UINT) pos;
		index --;
		index = CLAMP( index, 0, GetRows()->GetCount() );
	}

	m_pAssetBrowserDlg->m_oAssetViewer.EnsureAssetVisible( index );
}

int CAssetBrowserReportControl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if(CXTPReportControl::OnCreate(lpCreateStruct) == -1) 
		return -1;

	SetTimer( ID_ASSET_BROWSER_REPORT_CONTROL_TIMER, kAssetBrowserReportCtrl_CacheTimerDelay, NULL );
	return 0;
}

void CAssetBrowserReportControl::OnTimer( UINT_PTR nIDEvent )
{
	if( nIDEvent == ID_ASSET_BROWSER_REPORT_CONTROL_TIMER )
	{
		if ( CacheFieldsInfoForVisibleAsset() )
			RedrawControl();
		else
			KillTimer( ID_ASSET_BROWSER_REPORT_CONTROL_TIMER );
	}
}

bool CAssetBrowserReportControl::CacheFieldsInfoForVisibleAsset()
{
	for( size_t nCurrentItem = 0, nTotalItems = m_cVisibleItems.size(); nCurrentItem < nTotalItems; ++nCurrentItem )
	{
		IAssetDisplay* const pItem = m_cVisibleItems[nCurrentItem];

		if(	pItem->IsFlagSet( IAssetDisplay::eAssetFlags_CachedFieldsInfo ) ||
			pItem->IsFlagSet( IAssetDisplay::eAssetFlags_Invalid ) )
		{
			continue;
		}

		pItem->CacheFieldsInfo();

		return true;
	}

	return false;
}

void CAssetBrowserReportControl::ClearVisibleItems()
{
	m_cVisibleItems.clear();
	SetTimer( ID_ASSET_BROWSER_REPORT_CONTROL_TIMER, kAssetBrowserReportCtrl_CacheTimerDelay, NULL );
}

void CAssetBrowserReportControl::AddVisibleItem(IAssetDisplay *pAsset)
{
	stl::push_back_unique(m_cVisibleItems, pAsset);
}


//---

CAssetBrowserDialog::CAssetListVirtualRecord::CAssetListVirtualRecord( CAssetBrowserDialog* pDlg, std::vector<IAssetDisplay*>* pcAssets )
{
	m_pAssetBrowserDlg = pDlg;
	m_pAssets = pcAssets;
	m_prevTopRowIndex = -1;
}

void CAssetBrowserDialog::CAssetListVirtualRecord::GetItemMetrics( XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics )
{
	CXTPReportColumnOrder* pSortOrder = pDrawArgs->pControl->GetColumns()->GetSortOrder();
	BOOL bDecreasing = pSortOrder->GetCount() > 0 && pSortOrder->GetAt(0)->IsSortedDecreasing();

	CString strColumn = pDrawArgs->pColumn->GetCaption();
	int nIndexCol = pDrawArgs->pColumn->GetItemIndex();
	int nIndexRow = pDrawArgs->pRow->GetIndex();
	int nCount = pDrawArgs->pControl->GetRows()->GetCount();

	if( m_pAssets )
	{
		if ( m_prevTopRowIndex != m_pAssetBrowserDlg->m_oAssetListView.GetTopRowIndex() )
		{
			// The report view has changed since last time. So we should rebuild the list of visible items.
			m_pAssetBrowserDlg->m_oAssetListView.ClearVisibleItems();
			m_prevTopRowIndex = m_pAssetBrowserDlg->m_oAssetListView.GetTopRowIndex();

			int rowIndex = std::min(m_pAssetBrowserDlg->m_oAssetListView.GetRows()->GetCount(), 
															m_prevTopRowIndex 
																+ m_pAssetBrowserDlg->m_oAssetListView.GetReportAreaRows(m_prevTopRowIndex, TRUE) + 2);
			for ( int i=m_prevTopRowIndex; i < rowIndex; ++i)
			{
				if( !(*m_pAssets)[i]->IsFlagSet( IAssetDisplay::eAssetFlags_CachedFieldsInfo ) )
				{
					m_pAssetBrowserDlg->m_oAssetListView.AddVisibleItem((*m_pAssets)[i]);
				}
			}
		}

		pItemMetrics->strText = CAssetViewer::GetAssetFieldDisplayValue( (*m_pAssets)[nIndexRow], pDrawArgs->pColumn->GetInternalName() );
		pItemMetrics->nVirtRowLevel = 0;
		pItemMetrics->nVirtRowFlags |= xtpVirtRowLastChild;
	}

	if( nIndexRow % 2 )
	{
		pItemMetrics->clrBackground = kAssetBrowser_ListViewOddRowColor;
		pItemMetrics->clrForeground = kAssetBrowser_ListViewTextColor;
	}
	else
	{
		pItemMetrics->clrBackground = kAssetBrowser_ListViewEvenRowColor;
		pItemMetrics->clrForeground = kAssetBrowser_ListViewTextColor;
	}
}

// CAssetBrowserDialog dialog
IMPLEMENT_DYNCREATE(CAssetBrowserDialog,CBaseFrameWnd)

BEGIN_MESSAGE_MAP(CAssetBrowserDialog, CBaseFrameWnd)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
END_MESSAGE_MAP()

CAssetBrowserDialog* CAssetBrowserDialog::s_poCurrentInstance = NULL;

CAssetBrowserDialog::CAssetBrowserDialog()
{
	CRect rc( 0, 0, 0, 0 );

	s_poCurrentInstance = this;
	m_pAssetVirtualRecord = NULL;
	m_pReportHeader = NULL;
	GetIEditor()->RegisterNotifyListener( this );
	Create( WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, rc, AfxGetMainWnd() );
	m_oAssetViewer.SetStatusDisplay( this );
}

CAssetBrowserDialog::~CAssetBrowserDialog()
{
	m_oAssetViewer.SetStatusDisplay( NULL );
	GetIEditor()->UnregisterNotifyListener( this );
	CVisualBudgetSystem::GetVisualBudgetSystem().SetAssetBrowserDialog( 0 );
	s_poCurrentInstance = NULL;
}

void CAssetBrowserDialog::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CAssetBrowserViewClass );
}

void CAssetBrowserDialog::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if( event == eNotify_OnIdleUpdate )
	{
		return;
	}

	switch( event )
	{
		case eNotify_OnCloseScene: // Send when the document is about to close.
		case eNotify_OnBeginSceneOpen: // Sent when document is about to be opened.
		case eNotify_OnBeginNewScene: // Sent when the document is begin to be cleared.
		{
			break;
		}

		//case eNotify_OnMissionChange: // Send when the current mission changes. I am not sure if this is necessary.
		case eNotify_OnEndSceneOpen: // Sent after document have been opened.
		case eNotify_OnEndNewScene: // Sent after the document have been cleared.
		{
			m_oAssetListView.ClearVisibleItems();
			m_oAssetViewer.RefreshDatabases();
			break;
		}

		// Sent when document is about to be saved.
		case eNotify_OnBeginSceneSave:          
		{
			m_oAssetViewer.LockAll();
			break;
		}

		// Sent after document have been saved.
		case eNotify_OnEndSceneSave:
		{
			m_oAssetViewer.UnlockAll();
			break;
		}
	}
}

const CAssetBrowserDialog* CAssetBrowserDialog::GetCurrentInstance()
{
	static CAssetBrowserDialog* poCurrentInstance = NULL;

	return poCurrentInstance;
}

void CAssetBrowserDialog::GetSelectedItems( CAssetViewer::TAssetItems& rcpoSelectedItemArray )
{
	m_oAssetViewer.GetSelectedItems( rcpoSelectedItemArray );
}

void CAssetBrowserDialog::SetAssetBrowserDoubleClickCallback( CAssetViewer::TDoubleClickCallback pfnDoubleClickCallback )
{
	if( m_oAssetViewer )
	{
		m_oAssetViewer.SetDoubleClickCallback( pfnDoubleClickCallback );
	}
}

void CAssetBrowserDialog::DoDataExchange( CDataExchange* pDX )
{
	CBaseFrameWnd::DoDataExchange( pDX );
}

BOOL CAssetBrowserDialog::OnInitDialog()
{
	CreateAssetViewer();
	CVisualBudgetSystem::GetVisualBudgetSystem().SetAssetBrowserDialog(this);

	// We MUST use the ID AFX_IDW_STATUS_BAR if we are speaking about the status bar.
	// We also are reserving 2 slots into the status bar, for currently selected file path
	// AND current dataset size.
	UINT indicators[] =
	{
		AFX_IDW_STATUS_BAR,
		AFX_IDW_STATUS_BAR,
		AFX_IDW_STATUS_BAR,
	};

	m_oWindowStatusBar.Create( this, WS_CHILD|WS_VISIBLE|CBRS_BOTTOM);
	m_oWindowStatusBar.SetIndicators(indicators,3);

	char szCaption[512];

	sprintf(szCaption, "No selection"); //Placeholder info.
	m_oWindowStatusBar.SetPaneText( 0, szCaption );//Placeholder info.

	sprintf(szCaption, "Ready"); //Placeholder info.
	m_oWindowStatusBar.SetPaneText( 1, szCaption );//Placeholder info.

	sprintf(szCaption, "No operation"); //Placeholder info.
	m_oWindowStatusBar.SetPaneText( 2, szCaption );//Placeholder info.
	m_oWindowStatusBar.SetPaneInfo( 2, AFX_IDW_STATUS_BAR, SBPS_STRETCH, 200 );

	m_oVisualBudgetSystem.Create( CVisualBudgetSystemSettings::IDD, this );
	
//Diesel cut 
	//CXTPDockingPane * pPane = CreateDockingPane( "Visual Budget", &m_oVisualBudgetSystem, IDC_BUDGET_PANE, CRect(0,0,220,200), dockRightOf);
//Diesel new
	CXTPDockingPane * pPane = CreateDockingPane( "Visual Budget", &m_oVisualBudgetSystem, IDC_BUDGET_PANE, CRect(0,0,220,200), xtpPaneDockRight);

	if( pPane )
	{
		pPane->Hide();
	}

	//CreateRibbonBar();
	SetTimer( ID_TIMER_REFRESH_OPERATION_INFO, 500, NULL );

	return TRUE;
}

void CAssetBrowserDialog::CreateAssetViewer()
{
	LOGFONT lf;

	XTAuxData().fontBold.GetLogFont( &lf );
	lf.lfHeight = 18;
	m_fontCaption.CreateFontIndirect( &lf );

	m_oThumbsSplitter.CreateStatic( this, 2, 1, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN );
	m_oThumbsSplitter.SetSplitterStyle( XT_SPLIT_NOFULLDRAG | XT_SPLIT_NOSIZE | XT_SPLIT_NOBORDER );
	m_oThumbsSplitter.SetRowInfo( 0, 22, 0 );
	m_oThumbsSplitter.SetRowInfo( 1, 22, 0 );
	m_oThumbsCaption.Create( &m_oThumbsSplitter, _T("Asset thumbs browser"), NULL, WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, CRect( 0, 0, 0, 0 ), m_oThumbsSplitter.IdFromRowCol( 0, 0 ) );
	m_oThumbsCaption.SetCaptionColors( GetXtremeColor( COLOR_3DFACE ), GetXtremeColor( COLOR_3DSHADOW ), GetXtremeColor( COLOR_WINDOW ) );
	m_oThumbsCaption.SetOffice2003Colors( true );
	m_oThumbsCaption.ModifyCaptionStyle( 0, &m_fontCaption, NULL, NULL );

	m_oAssetViewer.Create( 0, 0, 100, 100, &m_oThumbsSplitter );
	m_oAssetViewer.SetDlgCtrlID( m_oThumbsSplitter.IdFromRowCol( 1, 0 ) );
	m_oAssetViewer.SetOwner( this );
	m_oAssetViewer.SetDoubleClickCallback( functor( *this, &CAssetBrowserDialog::DefaultDoubleClickCallback ) );

	m_searchDlg.Create( CAssetBrowserSearchDlg::IDD, this );
	m_searchDlg.m_pAssetBrowserDlg = this;
	
//Diesel cut
	//m_pDockPaneSearch = CreateDockingPane( "Search",&m_searchDlg, IDW_ASSET_BROWSER_SEARCH_PANE, CRect(0,0,479,75), dockBottomOf );
//Diesel new	
	m_pDockPaneSearch = CreateDockingPane( "Search",&m_searchDlg, IDW_ASSET_BROWSER_SEARCH_PANE, CRect(0,0,479,75), xtpPaneDockBottom );

	
	m_pDockPaneSearch->SetOptions(xtpPaneNoCloseable);

	m_oAssetListView.Create( WS_CHILD|WS_VISIBLE, CRect( 0, 0, 500, 500 ), this, 1211 );
	
//Diesel cut	
	//CXTPDockingPane *pDockPaneList = CreateDockingPane( "Asset list",&m_oAssetListView,IDW_ASSET_LIST_VIEW,CRect(0,0,550,300),dockLeftOf );
//Diesel new	
	CXTPDockingPane *pDockPaneList = CreateDockingPane( "Asset list",&m_oAssetListView,IDW_ASSET_LIST_VIEW,CRect(0,0,550,300),xtpPaneDockLeft );
	
	
	pDockPaneList->SetOptions(xtpPaneNoCloseable);

	GetDockingPaneManager()->RecalcFramesLayout();

	// We have to do this because this way the owner doesn't need to implement the information
	// display specifics...doing so will allow us to use this control in more places.
	//m_oAssetViewer.SetStatusDisplay(this);

	CreateDatabases();
}

void CAssetBrowserDialog::CreateDatabases()
{
	size_t										nTotalDatabases = 0, nCurrentDatabase = 0;
	std::vector<IClassDesc*>	cAssetDatabasePlugins;
	IEditorClassFactory*			piClassFactory = NULL;
	IAssetDisplayDatabase*		piCurrentDatabaseInterface = NULL;

	piClassFactory = GetIEditor()->GetClassFactory();
	piClassFactory->GetClassesByCategory( "Asset Display", cAssetDatabasePlugins );
	nTotalDatabases = cAssetDatabasePlugins.size();

	for( nCurrentDatabase = 0; nCurrentDatabase < nTotalDatabases; ++nCurrentDatabase )
	{
		if( cAssetDatabasePlugins[nCurrentDatabase]->QueryInterface( __uuidof(IAssetDisplayDatabase), (void**)&piCurrentDatabaseInterface ) == S_OK )
		{
			piCurrentDatabaseInterface->SetAssociatedViewer( &m_oAssetViewer );
			m_cAssetDatabases.push_back( piCurrentDatabaseInterface );
		}
	}

	m_oAssetListView.ClearVisibleItems();
	m_oAssetViewer.SetDatabases( m_cAssetDatabases );
	m_oAssetViewer.RefreshDatabases();
	ResetAssetListView();
	m_searchDlg.m_fields = m_commonFields;
	m_searchDlg.CreateFilterDialogs();
	m_oAssetViewer.RestartThread();

	m_searchDlg.FillDatabases();

	// used to match size to index of combo list thumb size item
	std::map<int,int> sizeToIndex;
	sizeToIndex[32] = 0;
	sizeToIndex[64] = 1;
	sizeToIndex[128] = 2;
	sizeToIndex[256] = 3;
	sizeToIndex[512] = 4;
	sizeToIndex[1024] = 5;
	
	m_searchDlg.m_cbThumbSize.SetCurSel( sizeToIndex[m_oAssetViewer.GetAssetThumbSize()] );
}

void CAssetBrowserDialog::FreeDatabases()
{
	size_t nCurrentDatabase = 0, nTotalDatabases = 0;
	m_oAssetViewer.WaitThreadAndFreeData();

	if( m_pAssetVirtualRecord )
	{
		m_pAssetVirtualRecord->m_pAssets = NULL;
		m_pAssetVirtualRecord->m_recentAssetPool.clear();
	}

	nTotalDatabases = m_cAssetDatabases.size();

	for( nCurrentDatabase = 0; nCurrentDatabase < nTotalDatabases; ++nCurrentDatabase )
	{
		m_cAssetDatabases[nCurrentDatabase]->FreeData();
	}

	m_cAssetDatabases.clear();
}

void CAssetBrowserDialog::SortAssets( const char* pFieldname, bool bDescending )
{
	m_oAssetViewer.SortAssets( pFieldname, bDescending );
}

void CAssetBrowserDialog::ResetAssetListView()
{
	m_oAssetListView.m_pAssetBrowserDlg = this;
	m_pReportHeader = new CAssetListReportHeader( this, &m_oAssetListView, m_oAssetListView.GetColumns() );
	m_oAssetListView.SetReportHeader( m_pReportHeader );

	// clear old columns
	if( m_oAssetListView.GetColumns() )
		m_oAssetListView.GetColumns()->Clear();

	m_oAssetViewer.LockAll();

	CAssetViewer::TAssetDatabases 	visibleAssetDatabases = m_oAssetViewer.GetDatabases();

	if( !visibleAssetDatabases.size() )
		return;

	//
	// find the common fields to all selected databases
	//
	IAssetDisplayDatabase::TAssetFields& firstDbFields = visibleAssetDatabases[0]->GetAssetFields();

	m_commonFields.clear();

	for( IAssetDisplayDatabase::TAssetFields::iterator iter = firstDbFields.begin(), iterEnd = firstDbFields.end(); iter != iterEnd; ++iter )
	{
		bool bAddField = true;

		for( size_t i = 1, iCount = visibleAssetDatabases.size(); i < iCount; ++i )
		{
			IAssetDisplayDatabase::TAssetFields& fields = visibleAssetDatabases[i]->GetAssetFields();

			// if field was not found, then do not add it, and exit from the loop
			if( !visibleAssetDatabases[i]->GetAssetFieldByName( iter->m_fieldName.c_str() ) )
			{
				bAddField = false;
				break;
			}
		}

		if( bAddField )
		{
			m_commonFields.push_back( *iter );
		}
	}

	//
	// create columns from fields
	//
	size_t index = 0;

	for( IAssetDisplayDatabase::TAssetFields::iterator iter = m_commonFields.begin(), iterEnd = m_commonFields.end(); iter != iterEnd; ++iter )
	{
		m_oAssetListView.AddColumn( new CXTPReportColumn( index++, iter->m_displayName.c_str(), iter->m_fieldName.c_str(), iter->m_listColumnWidth, TRUE, XTP_REPORT_NOICON, TRUE, TRUE ) );
	}

	//
	// set virtual mode for the list
	//

	m_pAssetVirtualRecord = new CAssetListVirtualRecord( this, &m_oAssetViewer.GetAssetItems() );
	// workaround for XTP deleting the pointer
	m_oAssetListView.SetVirtualMode( new CXTPReportRecord(), 0 );
	// set our virtual record
	m_oAssetListView.SetVirtualMode( m_pAssetVirtualRecord, m_oAssetViewer.GetAssetItems().size(), m_commonFields.size() );
	m_oAssetListView.GetReportHeader()->AllowColumnSort( TRUE );
	m_oAssetListView.Populate();
	
	m_oAssetViewer.UnlockAll();
}

void CAssetBrowserDialog::ShowDatabase( const char* pName )
{
	m_oAssetViewer.WaitThreadAndFreeData();

	if( m_pAssetVirtualRecord )
	{
		m_pAssetVirtualRecord->m_pAssets = NULL;
		m_pAssetVirtualRecord->m_recentAssetPool.clear();
	}

	CAssetViewer::TAssetDatabases dbs;

	for( size_t i = 0, iCount = m_cAssetDatabases.size(); i < iCount; ++i )
	{
		if( !strcmp( pName, "" ) || !stricmp( m_cAssetDatabases[i]->GetDatabaseName(), pName ) )
		{
			dbs.push_back( m_cAssetDatabases[i] );
		}
	}
			
	m_oAssetListView.ClearVisibleItems();
	m_oAssetViewer.SetDatabases( dbs );
	m_oAssetViewer.RefreshDatabases();
	ResetAssetListView();
	m_searchDlg.m_fields = m_commonFields;
	m_searchDlg.CreateFilterDialogs();
	m_oAssetViewer.RestartThread();
}

void CAssetBrowserDialog::ApplyFilters( IAssetDisplayDatabase::TAssetFieldFiltersMap& rFilterFields )
{
	m_oAssetViewer.ApplyFilters( rFilterFields );

	m_oAssetViewer.LockAll();
	m_pAssetVirtualRecord = new CAssetListVirtualRecord( this, &m_oAssetViewer.GetAssetItems() );
	// workaround for XTP deleting the pointer
	m_oAssetListView.SetVirtualMode( new CXTPReportRecord(), 0 );
	// set our virtual record
	m_oAssetListView.SetVirtualMode( m_pAssetVirtualRecord, m_oAssetViewer.GetAssetItems().size(), m_commonFields.size() );
	m_oAssetListView.GetReportHeader()->AllowColumnSort( TRUE );
	m_oAssetListView.Populate();
	m_oAssetListView.ClearVisibleItems();
	m_oAssetViewer.UnlockAll();
}

void CAssetBrowserDialog::ClearFilters()
{
	m_oAssetViewer.ClearFilters();

	m_oAssetViewer.LockAll();
	m_pAssetVirtualRecord = new CAssetListVirtualRecord( this, &m_oAssetViewer.GetAssetItems() );
	// workaround for XTP deleting the pointer
	m_oAssetListView.SetVirtualMode( new CXTPReportRecord(), 0 );
	// set our virtual record
	m_oAssetListView.SetVirtualMode( m_pAssetVirtualRecord, m_oAssetViewer.GetAssetItems().size(), m_commonFields.size() );
	m_oAssetListView.GetReportHeader()->AllowColumnSort( TRUE );
	m_oAssetListView.Populate();
	m_oAssetListView.ClearVisibleItems();
	m_oAssetViewer.UnlockAll();
}

void CAssetBrowserDialog::UpdateAssetDatabaseForVisualBudgetSystem( const std::vector<AABB> & AABBs )
{
	CResourceCollector* pRC = CVisualBudgetSystem::GetVisualBudgetSystem().CalculateMapResourceMemoryUsage( AABBs );
	size_t nTotalDatabases = m_cAssetDatabases.size();

	for( size_t nCurrentDatabase = 0; nCurrentDatabase < nTotalDatabases; ++nCurrentDatabase )
	{
		m_cAssetDatabases[nCurrentDatabase]->SetAssociatedViewer( &m_oAssetViewer );
	}

	string name;
	size_t assetCount = pRC->GetNumberOfAssets();

	for( size_t i = 0; i < assetCount; ++i )
	{
		pRC->GetAssetName( i, name );
		CString fileExt = Path::GetExt( CString( name ) );

		for( size_t nCurrentDatabase = 0; nCurrentDatabase < nTotalDatabases; ++nCurrentDatabase )
		{
			// if this database supports our file type, then specify we want this asset to be shown
			if( !stricmp( m_cAssetDatabases[nCurrentDatabase]->GetDatabaseTypeExt(), fileExt ) )
			{
				size_t size = GetIEditor()->GetSystem()->GetIPak()->FGetSize( name );
				//TODO: m_cAssetDatabases[nCurrentDatabase]->AddItem(CString(name), size);
				break;
			}
		}
	}
}

void CAssetBrowserDialog::OnDestroy()
{
	FreeDatabases();
}

void CAssetBrowserDialog::OnClose()
{	
	DestroyWindow();
}

BOOL CAssetBrowserDialog::PreTranslateMessage( MSG* pMsg )
{
	if( pMsg->message == WM_MOUSEWHEEL )
	{
		CWnd* poChildWindow = WindowFromPoint( pMsg->pt );

		if( poChildWindow == &m_oAssetViewer )
		{
			::SendMessage( m_oAssetViewer.GetSafeHwnd(), pMsg->message, pMsg->wParam, pMsg->lParam );
			return TRUE;
		}		
	}

	return FALSE;
}

void CAssetBrowserDialog::DefaultDoubleClickCallback()
{
	//CMainFrame*	poMainFrame = (CMainFrame*)AfxGetMainWnd();

	//if( !poMainFrame->IsPageDocked( "Asset Browser" ) )
	//{
	//	poMainFrame->ClosePage( "Asset Browser" );
	//}	
}

void CAssetBrowserDialog::CreateRibbonBar()
{
	CDynamicRibbonBar bar;

	AfxEnableControlContainer();
	CXTPWinDwmWrapper().SetProcessDPIAware();

	CXTPCommandBars* pCommandBars = GetCommandBars();

	{
		// Tooltips
		CXTPToolTipContext* pToolTipContext = GetCommandBars()->GetToolTipContext();
		pToolTipContext->SetStyle(xtpToolTipOffice2007);
		pToolTipContext->ShowTitleAndDescription();
		pToolTipContext->SetMargin(CRect(2, 2, 2, 2));
		pToolTipContext->SetMaxTipWidth(180);
		pToolTipContext->SetFont(pCommandBars->GetPaintManager()->GetIconFont());

		pCommandBars->GetCommandBarsOptions()->ShowKeyboardCues(xtpKeyboardCuesShowWindowsDefault);
		pCommandBars->GetCommandBarsOptions()->bToolBarAccelTips = TRUE;
	}

	GetCommandBars()->GetPaintManager()->m_bEnableAnimation = TRUE;
	XTPPaintManager()->SetTheme(xtpThemeRibbon);

	CMenu menu;
	menu.LoadMenu(IDR_MAINFRAME);
	SetMenu(NULL);

	CXTPRibbonBar* pRibbonBar = (CXTPRibbonBar*)pCommandBars->Add(_T("The Asset Browser Ribbon"), xtpBarTop, RUNTIME_CLASS(CXTPRibbonBar));
	pRibbonBar->SetCommandBars( pCommandBars );

	pRibbonBar->EnableDocking(0);

	bar.Init( pRibbonBar );

	CXTPControlPopup* pControlFile = (CXTPControlPopup*)pRibbonBar->AddSystemButton(0);

	pRibbonBar->GetSystemButton()->SetHideFlag( xtpHideGeneric, true );

	bar.LoadFromFile( "Editor/UI/AssetBrowserRibbon.xml" );
	bar.LoadFromFile( "Editor/UI/AssetBrowserCustomRibbon.xml" );

	//pRibbonBar->EnableFrameTheme();

//	// Tooltips
//	CXTPToolTipContext* pToolTipContext = GetCommandBars()->GetToolTipContext();
//
//	pToolTipContext->SetStyle( xtpToolTipOffice2007 );
//	pToolTipContext->ShowTitleAndDescription();
//	pToolTipContext->SetMargin( CRect( 2, 2, 2, 2 ) );
//	pToolTipContext->SetMaxTipWidth( 180 );
//	pToolTipContext->SetFont( pCommandBars->GetPaintManager()->GetIconFont() );
//	pCommandBars->GetCommandBarsOptions()->ShowKeyboardCues( xtpKeyboardCuesShowWindowsDefault );
//	pCommandBars->GetCommandBarsOptions()->bToolBarAccelTips = TRUE;
//
//	GetCommandBars()->GetPaintManager()->m_bEnableAnimation = TRUE;
//	//XTPPaintManager()->SetTheme( xtpThemeRibbon );
//
//	m_poRibbonBar = (CXTPRibbonBar*)pCommandBars->Add( _T("AssetBrowserRibbon"), xtpBarTop, RUNTIME_CLASS(CXTPRibbonBar) );
//	m_poRibbonBar->SetCommandBars( pCommandBars );
//	m_poRibbonBar->EnableDocking( TRUE );
//	m_poRibbonBar->ShowQuickAccess( FALSE );
//	m_poRibbonBar->AllowQuickAccessCustomization( FALSE );
//	m_poRibbonBar->AddSystemButton();
//	m_poRibbonBar->GetSystemButton()->SetVisible( FALSE );
//	m_poRibbonBar->ShowCaptionAlways( FALSE );
//	m_poRibbonBar->SetCloseable( TRUE );
//	m_poRibbonBar->EnableFrameTheme();
//
//	CXTPControlPopup* pControlFile = (CXTPControlPopup*)m_poRibbonBar->AddSystemButton(0);
//	CXTPRibbonTab* pAssetsTab = m_poRibbonBar->AddTab("Assets");
//	CXTPRibbonTab* pVisualBudgetTab = m_poRibbonBar->AddTab("Visual Budget System");
//
//#define ID_ASSET_BROWSER_LAYOUT_MODE 1
//#define ID_ASSET_BROWSER_DATABASES 2
//#define ID_ASSET_BROWSER_REFRESH 3
//
//	{
//		CXTPRibbonGroup* pGroup = pAssetsTab->AddGroup("General");
//		CXTPControlPopup* pControl = (CXTPControlPopup*)pGroup->Add(xtpControlSplitButtonPopup, ID_ASSET_BROWSER_LAYOUT_MODE);
//		pControl->SetCaption( "Layout" );
//		pControl = (CXTPControlPopup*)pGroup->Add(xtpControlSplitButtonPopup, ID_ASSET_BROWSER_DATABASES);
//		pControl->SetCaption( "Databases" );
//		CXTPControlButton* pBtn = (CXTPControlButton*)pGroup->Add(xtpControlButton, ID_ASSET_BROWSER_REFRESH);
//		pBtn->SetCaption( "Reload" );
//	}
/*
	{
	CXTPRibbonGroup* pGroupFile = pTab->AddGroup("Undo");
	pGroupFile->Add(xtpControlButton, ID_UNDO);
	pGroupFile->Add(xtpControlButton, ID_REDO);
	}

	{
	CXTPRibbonGroup* pGroupFile = pTab->AddGroup("Mode");
	pGroupFile->Add(xtpControlButton, ID_EDITMODE_SELECT);
	pGroupFile->Add(xtpControlButton, ID_EDITMODE_ROTATE);
	pGroupFile->Add(xtpControlButton, ID_EDITMODE_MOVE);
	pGroupFile->Add(xtpControlButton, ID_EDITMODE_SCALE);
	pGroupFile->Add(xtpControlButton, ID_EDITMODE_SELECTAREA);
	}

	{
	CXTPRibbonGroup* pGroupFile = pTab->AddGroup("Axis Lock");
	pGroupFile->Add(xtpControlButton, ID_SELECT_AXIS_X);
	pGroupFile->Add(xtpControlButton, ID_SELECT_AXIS_Y);
	pGroupFile->Add(xtpControlButton, ID_SELECT_AXIS_Z);
	pGroupFile->Add(xtpControlButton, ID_SELECT_AXIS_XY);
	pGroupFile->Add(xtpControlButton, ID_SELECT_AXIS_TERRAIN);
	pGroupFile->Add(xtpControlButton, ID_SELECT_AXIS_SNAPTOALL);
	}

	{
	CXTPRibbonGroup* pGroupFile = pTab->AddGroup("Tools");
	pGroupFile->Add(xtpControlButton, ID_EDITTOOL_LINK);
	pGroupFile->Add(xtpControlButton, ID_EDITTOOL_UNLINK);
	}

	{
	CXTPRibbonGroup* pGroupFile = pTab->AddGroup("Snap");
	pGroupFile->Add(xtpControlButton, ID_SNAP_TO_GRID);
	pGroupFile->Add(xtpControlButton, ID_SNAPANGLE);
	}

	CXTPRibbonGroup* pGroupClipborad = pTab->AddGroup("Clipboard");
	pGroupClipborad->ShowOptionButton();
	CXTPControlPopup* pControlPaste = (CXTPControlPopup*)pGroupClipborad->Add(xtpControlSplitButtonPopup, ID_EDIT_PASTE);
	pControlPaste->GetCommandBar()->GetControls()->Add(xtpControlButton, ID_EDIT_PASTE);
	pControlPaste->GetCommandBar()->GetControls()->Add(xtpControlButton, ID_EDIT_PASTE_SPECIAL);
	pGroupClipborad->Add(xtpControlButton, ID_EDIT_CUT);
	pGroupClipborad->Add(xtpControlButton, ID_EDIT_COPY);	
	}
	CXTPRibbonTab* pTab2 = pRibbonBar->AddTab("View1");
	CXTPRibbonTab* pTab3 = pRibbonBar->AddTab("Test1");



	CXTPControlPopup* pControlOptions = (CXTPControlPopup*)pRibbonBar->GetControls()->Add(xtpControlPopup, -1);
	pControlOptions->SetFlags(xtpFlagRightAlign);
	CMenu mnuOptions;
	mnuOptions.LoadMenu(IDR_MAINFRAME);
	pControlOptions->SetCommandBar(mnuOptions.GetSubMenu(15));
	pControlOptions->SetCaption(_T("View"));

	{
	CXTPControlPopup* pControlTools = (CXTPControlPopup*)pRibbonBar->GetControls()->Add(xtpControlPopup, -1);
	pControlTools->SetFlags(xtpFlagRightAlign);
	CMenu mnuTools;
	mnuTools.LoadMenu(IDR_MAINFRAME);
	pControlTools->SetCommandBar(mnuTools.GetSubMenu(14));
	pControlTools->SetCaption(_T("Tools"));
	}

	CXTPControl* pControlAbout = pRibbonBar->GetControls()->Add(xtpControlButton, ID_APP_ABOUT);
	pControlAbout->SetFlags(xtpFlagRightAlign);
	*/

	//m_poRibbonBar->EnableFrameTheme();
}

void CAssetBrowserDialog::OnTimer(UINT_PTR nIDEvent)
{
	if( nIDEvent == ID_TIMER_REFRESH_OPERATION_INFO )
	{
		if( m_oAssetViewer.IsAnyOperationInProgress() )
		{
			CString str, opStr;
			int progress;

			m_oAssetViewer.GetCurrentOperationTextAndProgress( opStr, progress );
			str.Format( _T( "%s %d%%" ), opStr.GetBuffer(), progress );
			m_oWindowStatusBar.SetPaneText( 2, str.GetBuffer() );
		}
		else
		{
			m_oWindowStatusBar.SetPaneText( 2, "Idle" );
		}
	}

	__super::OnTimer(nIDEvent);
}

void CAssetBrowserDialog::OnChangeStatusBarInfo( UINT nSelectedItems, UINT nVisibleItems, UINT nTotalItems )
{
	CString str;

	if( !nSelectedItems )
	{
		str.Format( "No selection" );
	}
	else
	{
		str.Format( "%d selected assets", nSelectedItems );
	}

	m_oWindowStatusBar.SetPaneText( 0, str );

	str.Format( "%d visible assets, %d total assets", nVisibleItems, nTotalItems );
	m_oWindowStatusBar.SetPaneText( 1, str );
}

void CAssetBrowserDialog::OnSelectionChanged()
{
	int index = m_oAssetViewer.GetFirstSelectedItemIndex();
	if( index > -1 )
		m_oAssetListView.GetRows()->GetAt(index)->EnsureVisible();

	// This is the only way that I can make the list view properly repainted.
	// I tried 'RedrawControl()', 'RedrawWindow()', 'Invalidate()', etc. to no avail.
	CWnd *pWnd = m_oAssetListView.SetFocus();
	if( pWnd )
	 pWnd->SetFocus();
}
