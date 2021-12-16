////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   TelemetryViewDialog.cpp
//  Version:     v1.00
//  Created:     13/05/11 by Steve Humphreys
//  Description: Dialog class for the whole telemetry viewer
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TelemetryViewDialog.h"

#include "GameEngine.h"
#include <IEditorGame.h>

#include "Resource.h"
#include "ViewPane.h"
#include "StringDlg.h"
#include "NumberDlg.h"
#include "ViewManager.h"

#include "IViewPane.h"
#include "PluginManager.h"

#include "Telemetry\XmlDataSource.h"
#include "Telemetry\FileDataSource.h"

#include "Controls\TreeCtrlReport.h"

//////////////////////////////////////////////////////////////////////////

#define TELEMETRYVIEW_LAYOUT_SECTION _T("TelemetryViewLayout")
#define TELEMETRYVIEW_SECTION _T("DockingPaneLayouts\\TelemetryView")
#define SPLITTER_ENTRY _T("Splitter")
#define VERSION_ENTRY _T("TelemetryViewLayoutVersion")

#define TELEMETRYVIEW_SETTINGS_SECTION _T("TelemetryView")
#define SNAPPING_MODE_ENTRY _T("SnappingMode")
#define FRAME_SNAPPING_FPS_ENTRY _T("FrameSnappingFPS")
#define TICK_DISPLAY_MODE_ENTRY _T("TickDisplayMode")


namespace
{
	const int TELEMETRYVIEW_LAYOUT_VERSION = 0x0001; // bump this up on every substantial pane layout change
}

//////////////////////////////////////////////////////////////////////////
class CTelemetryViewPaneClass : public TRefCountBase<IViewPaneClass>
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		// {EB4D0786-8912-476b-B7AC-AFCDD56E92FE}
		static const GUID guid = 
		{ 0xeb4d0786, 0x8912, 0x476b, { 0xb7, 0xac, 0xaf, 0xcd, 0xd5, 0x6e, 0x92, 0xfe } };
		return guid;
	}
	virtual const char* ClassName() { return "Telemetry Viewer"; };
	virtual const char* Category() { return "Editor"; };
	//////////////////////////////////////////////////////////////////////////
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CTelemetryViewDialog); };
	virtual const char* GetPaneTitle() { return _T("Telemetry Viewer"); };
	virtual EDockingDirection GetDockingDirection() { return DOCK_BOTTOM; };
	virtual CRect GetPaneRect() { return CRect(0,0,500,300); };
	virtual bool SinglePane() { return true; };
	virtual bool WantIdleUpdate() { return true; };
};

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CTelemetryViewPaneClass );
	GetIEditor()->GetSettingsManager()->AddToolName(TELEMETRYVIEW_LAYOUT_SECTION,"Telemetry Viewer");
}

IMPLEMENT_DYNCREATE(CTelemetryViewDialog, CBaseFrameWnd)


//////////////////////////////////////////////////////////////////////////
CTelemetryViewDialog *CTelemetryViewDialog::s_pTelemetryViewDialog = NULL;

//////////////////////////////////////////////////////////////////////////
CTelemetryViewDialog::CTelemetryViewDialog(CWnd* pParent /*=NULL*/)
: m_pDataSource(NULL)
{
	s_pTelemetryViewDialog = this;
	m_bPause = false;
	m_bPlay = false;
	m_fLastTime=-1.0f;
	GetIEditor()->RegisterNotifyListener(this);

	m_lazyInitDone = false;

	CRect rc(0,0,0,0);
	Create( WS_CHILD|WS_VISIBLE,rc,AfxGetMainWnd() );
}

CTelemetryViewDialog::~CTelemetryViewDialog()
{
	s_pTelemetryViewDialog = 0;
	GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::PostNcDestroy()
{
	delete this;
}

void CTelemetryViewDialog::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTelemetryViewDialog, CBaseFrameWnd)
//	ON_WM_CREATE()
	ON_COMMAND( ID_TEL_OPEN_XML, OnOpenLegacyXmlFile )
	ON_COMMAND( ID_TEL_OPEN_FILE, OnOpenDataFile )
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_DESTROY()
//	ON_WM_PAINT()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnToolbarDropDown(NMHDR *pnhdr, LRESULT *plr)
{
	CRect rc;
	CPoint pos;
	GetCursorPos( &pos );
	NMTOOLBAR* pnmtb = (NMTOOLBAR*)pnhdr;
	m_cDlgToolBar.GetToolBarCtrl().GetRect( pnmtb->iItem,rc );
	ClientToScreen( rc );
	pos.x = rc.left;
	pos.y = rc.bottom;

	// Switch on button command id's.
// 	switch (pnmtb->iItem)
// 	{
// 		case ID_TV_PLAY:
// 			OnPlayMenu(pos);
// 			break;
// 		case ID_TV_STOP:
// 			OnStopMenu(pos);
// 			break;
// 		case ID_TV_RECORD_AUTO:
// 			OnRecordMenu(pos);
// 			break;
// 		default:
// 			return;
// 	}
	*plr = TBDDRET_DEFAULT;
}

//////////////////////////////////////////////////////////////////////////
BOOL CTelemetryViewDialog::OnInitDialog()
{
	__super::OnInitDialog();

	CMFCUtils::LoadShortcuts(GetCommandBars(), IDR_TELEMETRY, "TelemetryView");

	//////////////////////////////////////////////////////////////////////////
	// Create and setup the heightmap edit viewport and the toolbars
	GetCommandBars()->GetCommandBarsOptions()->bShowExpandButtonAlways = FALSE;
	GetCommandBars()->EnableCustomization(FALSE);

	CRect rcClient;
	GetClientRect(rcClient);

	CXTPCommandBar* pMenuBar = GetCommandBars()->SetMenu( _T("Menu Bar"),IDR_TELEMETRY);
	pMenuBar->SetFlags(xtpFlagStretched);
	pMenuBar->EnableCustomization(TRUE);
	//////////////////////////////////////////////////////////////////////////

	CRect rc;
	InitToolbar();

	// Resize the toolbar
	GetClientRect(rc);
	m_cDlgToolBar.SetWindowPos(NULL, 0, 0, rc.right, 70, SWP_NOZORDER);
	
	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	m_cDlgToolBar.SetBarStyle(m_cDlgToolBar.GetBarStyle()|CBRS_TOOLTIPS|CBRS_FLYBY);

	// Create splitter and track view controls.
	GetClientRect(rc);
	rc.right /= 2;
	
	m_timelinesCtrl.Create(WS_CHILD|WS_TABSTOP,CRect(0,0,200,300),this,1 );
	
	// Create key tracks dialog.
	m_eventsCtrl.Create( NULL,NULL,WS_VISIBLE|WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_HSCROLL,rc,this,2 );
	m_eventsCtrl.SetTimeRange( 0,20 );
	m_eventsCtrl.SetTimeScale( 100,0 );
	m_eventsCtrl.SetCurrTime( 1 );

	m_timelinesCtrl.SetEventsCtrl( &m_eventsCtrl );


	GetClientRect(rc);
	m_wndSplitter.CreateStatic( this,1,2,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS );
	m_wndSplitter.SetPane( 0,0,&m_timelinesCtrl, CSize(150, 100) );
	m_wndSplitter.SetPane( 0,1,&m_eventsCtrl, CSize(300, 100) );

	m_wndSplitter.MoveWindow(&rc, TRUE);

	m_eventsCtrl.SetOwner(this);

	m_timelinesCtrl.SetOwner(this);
	m_timelinesCtrl.SetTelemetryViewDialog(this);
	m_timelinesCtrl.ShowWindow(SW_SHOW);


	RecalcLayout();

	m_lazyInitDone = false;

	ReadLayouts();
	ReadMiscSettings();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::InitToolbar()
{
	// Create the toolbar
	m_cDlgToolBar.CreateEx(this, TBSTYLE_FLAT|TBSTYLE_WRAPABLE,
		WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC);
	m_cDlgToolBar.LoadToolBar24(IDR_TELEMETRY);
	m_cDlgToolBar.GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);
// 	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_ADDKEY),TBBS_CHECKGROUP );
// 	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_MOVEKEY),TBBS_CHECKGROUP );
// 	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_SLIDEKEY),TBBS_CHECKGROUP );
// 	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_SCALEKEY),TBBS_CHECKGROUP );
// 	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_RECORD),TBBS_CHECKBOX );
// 	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_RECORD_AUTO),TBBS_CHECKBOX );
// 	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_PLAY),TBBS_CHECKBOX );
// 	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_PAUSE),TBBS_CHECKBOX );
// 	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_SNAP_NONE),TBBS_CHECKGROUP );
// 	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_SNAP_MAGNET),TBBS_CHECKGROUP );
// 	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_SNAP_FRAME),TBBS_CHECKGROUP );
// 	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_SNAP_TICK),TBBS_CHECKGROUP );
// 	int nIndex=m_cDlgToolBar.CommandToIndex(ID_TV_PLAY);
// 	m_cDlgToolBar.SetButtonStyle(nIndex, m_cDlgToolBar.GetButtonStyle(nIndex) | TBSTYLE_DROPDOWN);
// 	nIndex=m_cDlgToolBar.CommandToIndex(ID_TV_RECORD_AUTO);
// 	m_cDlgToolBar.SetButtonStyle(nIndex, m_cDlgToolBar.GetButtonStyle(nIndex) | TBSTYLE_DROPDOWN);
// 	nIndex=m_cDlgToolBar.CommandToIndex(ID_TV_STOP);
// 	m_cDlgToolBar.SetButtonStyle(nIndex, m_cDlgToolBar.GetButtonStyle(nIndex) | TBSTYLE_DROPDOWN);
	m_cDlgToolBar.CalcFixedLayout(0,TRUE);

	m_bPause = false;
	m_bPlay = false;

	RecalcLayout();

// 	CRect rc(0,0,0,0);
// 	int index;
// 	index = m_cDlgToolBar.CommandToIndex(ID_TV_SEQUENCE);
// 	m_cDlgToolBar.SetButtonInfo(index, ID_TV_SEQUENCE, TBBS_SEPARATOR, 200);
// 	m_cDlgToolBar.GetItemRect(index, &rc);
// 	rc.top++;
// 	rc.bottom += 400;
// 	m_sequences.Create( WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|CBS_DROPDOWNLIST|CBS_SORT,rc,this,IDC_SEQUENCES );
// 	m_sequences.SetFont( CFont::FromHandle((HFONT)gSettings.gui.hSystemFont) );
// 	m_sequences.SetParent( &m_cDlgToolBar );
// 
// 	rc.SetRect(0,0,0,0);
// 	index = m_cDlgToolBar.CommandToIndex(ID_TV_CURSORPOS);
// 	m_cDlgToolBar.SetButtonInfo(index, ID_TV_CURSORPOS, TBBS_SEPARATOR, 60);
// 	m_cDlgToolBar.GetItemRect(index, &rc);
// 	++rc.top;
// 
// 	m_CursorPos.Create("0.000", WS_CHILD|WS_VISIBLE|SS_CENTER|SS_CENTERIMAGE|SS_SUNKEN,rc,this,IDC_STATIC);
// 
// 	m_CursorPos.SetFont( CFont::FromHandle((HFONT)gSettings.gui.hSystemFont) );
// 	m_CursorPos.SetParent( &m_cDlgToolBar );
// 
// 	rc.SetRect(0,0,0,0);
// 	index = m_cDlgToolBar.CommandToIndex(ID_TV_ACTIVE_CAMERA);
// 	m_cDlgToolBar.SetButtonInfo(index, ID_TV_ACTIVE_CAMERA, TBBS_SEPARATOR, 100);
// 	m_cDlgToolBar.GetItemRect(index, &rc);
// 	++rc.top;
}

void CTelemetryViewDialog::InvalidateTrackList()
{
	if (m_eventsCtrl.m_hWnd)
		m_eventsCtrl.Invalidate();

	m_timelinesCtrl.OnFillItems();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::Update()
{
	if(m_pDataSource)
	{
		bool done = m_pDataSource->Update();
		if(done)
		{
			m_pDataSource->Close();
			delete m_pDataSource;
			m_pDataSource = NULL;

			InvalidateTrackList();

			IEditorGame* pGame = GetIEditor()->GetGameEngine()->GetIEditorGame();
			if(pGame)
			{
				pGame->RegisterTelemetryTimelineRenderers(&m_repository);
			}
		}
	}

	// for each timeline, get the checkbox state to determine if we render it or not
	m_timelinesCtrl.UpdateRenderStatus();

	// todo: better place to call this?
	m_repository.Render();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnOpenLegacyXmlFile()
{
	assert(!m_pDataSource);

	m_pDataSource = new Telemetry::CXmlDataSource(m_repository);
	m_pDataSource->Open();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnOpenDataFile()
{
	assert(!m_pDataSource);

	m_pDataSource = new Telemetry::CFileDataSource(m_repository);
	m_pDataSource->Open();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnGoToStart()
{
// 	if (m_currSequence != NULL)
// 	{
// 		// Reset sequence to the beginning.
// 		m_currSequence->Reset(true);
// 	}

	// Restore default camera in view.
	//GetIEditor()->GetViewManager()->SetCameraObjectId( m_defaulCameraObjectId );
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnGoToEnd()
{
// 	if (m_currSequence != NULL)
// 	{
// 		// Reset sequence to the beginning.
// 		m_currSequence->Reset(true);
// 	}

	// Restore default camera in view.
	//GetIEditor()->GetViewManager()->SetCameraObjectId( m_defaulCameraObjectId );
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnPlay()
{


	// Set current camera params.
	//OnSetCamera( GetMovieSystem()->GetCameraParams() );
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnStop()
{
	// Restore default camera in view.
	//GetIEditor()->GetViewManager()->SetCameraObjectId( m_defaulCameraObjectId );
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnPause()
{
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnLoop()
{
}

void CTelemetryViewDialog::OnSetFocus(CWnd* pOldWnd)
{
	//__super::OnSetFocus(pOldWnd);

	//if (AfxGetMainWnd())
		//AfxGetMainWnd()->SetFocus();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnUpdatePlay( CCmdUI* pCmdUI )
{
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnUpdatePause( CCmdUI* pCmdUI )
{
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnUpdateLoop( CCmdUI* pCmdUI )
{
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnBeginNewScene:
	case eNotify_OnCloseScene:
		//SetCurrentSequence(NULL);
		break;
	case eNotify_OnBeginSceneOpen:
		/*m_wndTrackProps.Reset();*/
		break;
	case eNotify_OnEndSceneOpen:
		// trigger reload data here?
		break;
	case eNotify_OnMissionChange:
// 		if (!m_bInLevelLoad)
// 			ReloadSequences();
		break;
	case eNotify_OnUpdateTrackView:
// 		if (!m_bInLevelLoad)
// 			ReloadSequences();
		break;
	case eNotify_OnIdleUpdate:
		Update();
		break;
	case eNotify_OnUpdateTrackViewKeys:
// 		if (!m_bInLevelLoad)
// 		{
// 			InvalidateTrackList();
// 			GetIEditor()->GetAnimation()->ForceAnimation();
// 		}
		break;
	case eNotify_OnUpdateTVKeySelection:
// 		if (m_currSequence && !m_bInLevelLoad)
// 			m_wndKeyProperties.OnKeySelectionChange();
		break;
	case eNotify_OnQuit:
		SaveLayouts();
		SaveMiscSettings();
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnDestroy()
{
	SaveLayouts();
	SaveMiscSettings();

	__super::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
BOOL CTelemetryViewDialog::PreTranslateMessage( MSG* pMsg )
{
	return __super::PreTranslateMessage(pMsg);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnModeDopeSheet()
{
	m_wndSplitter.SetPane( 0,1,&m_eventsCtrl, CSize(300, 100) );
	m_eventsCtrl.ShowWindow(SW_SHOW);
	
	m_wndSplitter.RecalcLayout();
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnSize( UINT nType, int cx, int cy )
{
	__super::OnSize( nType,cx,cy );

	if (cx > 0 && cy > 0
	&& m_lazyInitDone == false)
	{
		CRect rc;
		int index;
		index = m_cDlgToolBar.CommandToIndex(ID_TV_SEQUENCE);
		m_cDlgToolBar.SetButtonInfo(index, ID_TV_SEQUENCE, TBBS_SEPARATOR, 200);
		m_cDlgToolBar.GetItemRect(index, &rc);
		// Expand the rectangle
		rc.left += 2;
		rc.bottom += 400;
		//m_sequences.MoveWindow( rc );

		rc.SetRect(0,0,0,0);
		index = m_cDlgToolBar.CommandToIndex(ID_TV_CURSORPOS);
		m_cDlgToolBar.SetButtonInfo(index, ID_TV_CURSORPOS, TBBS_SEPARATOR, 60);
		m_cDlgToolBar.GetItemRect(index, &rc);
		++rc.top;
		//m_CursorPos.MoveWindow( rc );

		rc.SetRect(0,0,0,0);
		index = m_cDlgToolBar.CommandToIndex(ID_TV_ACTIVE_CAMERA);
		m_cDlgToolBar.SetButtonInfo(index, ID_TV_ACTIVE_CAMERA, TBBS_SEPARATOR, 100);
		m_cDlgToolBar.GetItemRect(index, &rc);
		++rc.top;
/*		m_activeCamStatic.MoveWindow( rc );*/

		m_timelinesCtrl.SyncEventsCtrl();

		m_lazyInitDone = true;
	}
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnViewTickInSeconds()
{
	m_eventsCtrl.SetTickDisplayMode(TeVTICK_INSECONDS);
	/*m_wndGraph.SetTickDisplayMode(TeVTICK_INSECONDS);*/
	//SetCursorPosText(GetIEditor()->GetAnimation()->GetTime());
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnViewTickInFrames()
{
 	m_eventsCtrl.SetTickDisplayMode(TeVTICK_INFRAMES);
// 	m_wndGraph.SetTickDisplayMode(TeVTICK_INFRAMES);
	//SetCursorPosText(GetIEditor()->GetAnimation()->GetTime());
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnUpdateViewTickInSeconds( CCmdUI* pCmdUI )
{
	if (m_eventsCtrl.GetTickDisplayMode() == TeVTICK_INSECONDS)
		pCmdUI->SetCheck(TRUE);
	else
		pCmdUI->SetCheck(FALSE);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnUpdateViewTickInFrames( CCmdUI* pCmdUI )
{
	if (m_eventsCtrl.GetTickDisplayMode() == TeVTICK_INFRAMES)
		pCmdUI->SetCheck(TRUE);
	else
		pCmdUI->SetCheck(FALSE);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::SaveMiscSettings() const
{
	//float fps = m_wndGraph.GetFPS();
	//AfxGetApp()->WriteProfileBinary(TELEMETRYVIEW_SETTINGS_SECTION, FRAME_SNAPPING_FPS_ENTRY, (LPBYTE)&fps, sizeof(float));
	AfxGetApp()->WriteProfileInt(TELEMETRYVIEW_SETTINGS_SECTION, TICK_DISPLAY_MODE_ENTRY, (int)m_eventsCtrl.GetTickDisplayMode());
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::ReadMiscSettings()
{
	LPBYTE pData=NULL; UINT bytes=0;
	if(AfxGetApp()->GetProfileBinary(TELEMETRYVIEW_SETTINGS_SECTION, FRAME_SNAPPING_FPS_ENTRY, &pData, &bytes))
	{
		float fps = *(float*)pData;
		m_eventsCtrl.SetSnapFPS(FloatToIntRet(fps));
		//m_wndGraph.SetFPS(fps);
	}
	
	ETeVTickMode tickMode = (ETeVTickMode)(AfxGetApp()->GetProfileInt(TELEMETRYVIEW_SETTINGS_SECTION, TICK_DISPLAY_MODE_ENTRY, TeVTICK_INSECONDS));
	m_eventsCtrl.SetTickDisplayMode(tickMode);
	//m_wndGraph.SetTickDisplayMode(tickMode);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::SaveLayouts()
{
	CXTPDockingPaneLayout layout(GetDockingPaneManager());
	GetDockingPaneManager()->GetLayout( &layout );
	layout.Save(TELEMETRYVIEW_LAYOUT_SECTION);

	int colWidth, dummy;
	m_wndSplitter.GetColumnInfo(0, colWidth, dummy);
	AfxGetApp()->WriteProfileInt(TELEMETRYVIEW_SECTION, SPLITTER_ENTRY, colWidth);
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::ReadLayouts()
{
	CXTRegistryManager regMgr;
	int paneLayoutVersion = regMgr.GetProfileInt(TELEMETRYVIEW_SECTION, VERSION_ENTRY, 0);
	if (paneLayoutVersion == TELEMETRYVIEW_LAYOUT_VERSION)
	{
		CXTPDockingPaneLayout layout(GetDockingPaneManager());
		if (layout.Load(TELEMETRYVIEW_LAYOUT_SECTION)) 
		{
			if (layout.GetPaneList().GetCount() > 0)
				GetDockingPaneManager()->SetLayout(&layout);	
		}

		int colWidth = regMgr.GetProfileInt(TELEMETRYVIEW_SECTION, SPLITTER_ENTRY, 150);
		m_wndSplitter.SetPane(0, 0, &m_timelinesCtrl, CSize(colWidth, 100));
	}
	else
	{
		regMgr.WriteProfileInt(TELEMETRYVIEW_SECTION, VERSION_ENTRY, TELEMETRYVIEW_LAYOUT_VERSION);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::SetCursorPosText( float fTime )
{
// 	CString sText;
// 	if(m_eventsCtrl.GetTickDisplayMode() == TeVTICK_INSECONDS)
// 	{
// 		sText.Format("%1.3f", fTime);
// 		m_CursorPos.SetWindowText(sText);
// 	}
// 	else if(m_eventsCtrl.GetTickDisplayMode() == TeVTICK_INFRAMES)
// 	{
// 		//sText.Format("%d", FloatToIntRet(fTime * m_wndGraph.GetFPS()));
// 		m_CursorPos.SetWindowText(sText);
// 	}
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnCustomize()
{
	CMFCUtils::ShowShortcutsCustomizeDlg(GetCommandBars(), IDR_TELEMETRY, "TelemetryView");
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnExportShortcuts()
{
	CMFCUtils::ExportShortcuts(GetCommandBars()->GetShortcutManager());
}

//////////////////////////////////////////////////////////////////////////
void CTelemetryViewDialog::OnImportShortcuts()
{
	CMFCUtils::ImportShortcuts(GetCommandBars()->GetShortcutManager(), "Telemetry");
}
