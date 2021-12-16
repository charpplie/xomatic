////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   trackviewdialog.cpp
//  Version:     v1.00
//  Created:     24/4/2002 by Timur.
//  Compilers:   Visual C++ 7.0
//  Description: CTrackViewDialog Implementation file.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TrackViewDialog.h"

#include "..\ViewPane.h"
#include "..\StringDlg.h"
#include "..\NumberDlg.h"
#include "TVSequenceProps.h"
#include "..\ViewManager.h"
#include "..\AnimationContext.h"
#include "..\AnimationSerializer.h"
#include "TrackViewFindDlg.h"
#include "TrackViewUndo.h"

#include "..\Objects\ObjectManager.h"
#include "..\Objects\Entity.h"
#include "..\Objects\CameraObject.h"
#include "..\Objects\SequenceObject.h"
#include "IViewPane.h"
#include "..\PluginManager.h"
#include "..\Util\3DConnexionDriver.h"

#include <IMovieSystem.h>
#include <IEntitySystem.h>
// CTrackViewDialog dialog

#include "..\Controls\TreeCtrlReport.h"

//////////////////////////////////////////////////////////////////////////
#define IDW_KEYS_PANE         AFX_IDW_CONTROLBAR_FIRST+10
#define IDW_TRACK_PROPS_PANE  AFX_IDW_CONTROLBAR_FIRST+11
#define IDW_TRACK_GRAPH_PANE  AFX_IDW_CONTROLBAR_FIRST+12
#define IDW_TRACK_SEQUENCES_PANE  AFX_IDW_CONTROLBAR_FIRST+13

//////////////////////////////////////////////////////////////////////////
class CTrackViewPaneClass : public TRefCountBase<IViewPaneClass>
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		// {D21C9FE5-22D3-41e3-B84B-A377AFA0A05C}
		static const GUID guid = 
		{ 0xd21c9fe5, 0x22d3, 0x41e3, { 0xb8, 0x4b, 0xa3, 0x77, 0xaf, 0xa0, 0xa0, 0x5c } };
		return guid;
	}
	virtual const char* ClassName() { return "Track View"; };
	virtual const char* Category() { return "Track View"; };
	//////////////////////////////////////////////////////////////////////////
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CTrackViewDialog); };
	virtual const char* GetPaneTitle() { return _T("Track View"); };
	virtual EDockingDirection GetDockingDirection() { return DOCK_BOTTOM; };
	virtual CRect GetPaneRect() { return CRect(0,0,500,300); };
	virtual bool SinglePane() { return true; };
	virtual bool WantIdleUpdate() { return true; };
};

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CTrackViewPaneClass );
}

IMPLEMENT_DYNCREATE(CTrackViewDialog, CBaseFrameWnd)


//////////////////////////////////////////////////////////////////////////
CString CTrackViewDialog::m_currSequenceName;
CTrackViewDialog *g_pTrackViewDialog = NULL;

//////////////////////////////////////////////////////////////////////////
CTrackViewDialog::CTrackViewDialog(CWnd* pParent /*=NULL*/)
{
	g_pTrackViewDialog = this;
	m_currSequence = 0;
	m_bRecord = false;
	m_bAutoRecord = false;
	m_bPause = false;
	m_bPlay = false;
	m_bReloading=false;
	m_fLastTime=-1.0f;
	ZeroStruct(m_defaulCameraObjectId);
	m_movieSystem = 0;
	m_fAutoRecordStep = 0.5f;
	GetIEditor()->RegisterNotifyListener(this);
	m_bNeedReloadSequence = false;
	m_bInLevelLoad = false;

	m_findDlg = 0;

	m_lazyInitDone = false;

	CRect rc(0,0,0,0);
	Create( WS_CHILD|WS_VISIBLE,rc,AfxGetMainWnd() );
}

CTrackViewDialog::~CTrackViewDialog()
{
	if(m_findDlg)
	{
		m_findDlg->DestroyWindow();
		delete m_findDlg;
		m_findDlg = 0;
	}
	g_pTrackViewDialog = 0;
	GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::PostNcDestroy()
{
	delete this;
}

void CTrackViewDialog::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTrackViewDialog, CBaseFrameWnd)
//	ON_WM_CREATE()
	ON_CBN_SELENDOK( IDC_SEQUENCES,OnChangeSequence )
	ON_COMMAND( ID_TV_PREVKEY,OnGoToPrevKey )
	ON_COMMAND( ID_TV_NEXTKEY,OnGoToNextKey )
	ON_COMMAND( ID_TV_ADDKEY,OnAddKey )
	ON_COMMAND( ID_TV_DELKEY,OnDelKey )
	ON_COMMAND( ID_TV_MOVEKEY,OnMoveKey )
	ON_COMMAND( ID_TV_SLIDEKEY,OnSlideKey )
	ON_COMMAND( ID_TV_SCALEKEY,OnScaleKey )
	ON_COMMAND( ID_TV_ADD_SEQUENCE,OnAddSequence )
	ON_COMMAND( ID_TV_DEL_SEQUENCE,OnDelSequence )
	ON_COMMAND( ID_TV_EDIT_SEQUENCE,OnEditSequence )
	ON_COMMAND( ID_TV_RECORD,OnRecord )
	ON_COMMAND( ID_TV_RECORD_AUTO,OnAutoRecord )
	ON_COMMAND( ID_TV_JUMPSTART,OnGoToStart)
	ON_COMMAND( ID_TV_JUMPEND,OnGoToEnd)
	ON_COMMAND( ID_TV_PLAY,OnPlay )
	ON_COMMAND( ID_TV_STOP,OnStop )
	ON_COMMAND( ID_TV_PAUSE,OnPause )
	ON_COMMAND(	ID_PLAY_LOOP, OnLoop)
	ON_COMMAND( ID_TV_COPYPASTEKEYS,OnCopyPasteKeys)
	ON_COMMAND( ID_ADDNODE,OnAddSelectedNode )
	ON_COMMAND( ID_ADDSCENETRACK,OnAddSceneTrack )
	ON_COMMAND( ID_FIND,OnFindNode )
	ON_COMMAND( ID_TV_SEQUENCE_NEW,OnAddSequence )
	ON_COMMAND( ID_TV_MODE_CURVEEDITOR,OnModeCurveEditor )
	ON_COMMAND( ID_TV_MODE_DOPESHEET,OnModeDopeSheet )
	ON_COMMAND( ID_TV_MODE_OPENCURVEEDITOR,OnOpenCurveEditor )
	ON_COMMAND( ID_TV_SNAP_NONE,OnSnapNone )
	ON_COMMAND( ID_TV_SNAP_MAGNET,OnSnapMagnet )
	ON_COMMAND( ID_TV_SNAP_FRAME,OnSnapFrame )
	ON_COMMAND( ID_TV_SNAP_TICK,OnSnapTick )
	ON_COMMAND( ID_TV_SNAP_FPS,OnSnapFPS )
	ON_UPDATE_COMMAND_UI(ID_TV_RECORD, OnUpdateRecord)
	ON_UPDATE_COMMAND_UI(ID_TV_RECORD_AUTO, OnUpdateRecordAuto)
	ON_UPDATE_COMMAND_UI(ID_TV_PLAY, OnUpdatePlay)
	ON_UPDATE_COMMAND_UI(ID_TV_PAUSE, OnUpdatePause)
	ON_UPDATE_COMMAND_UI(ID_PLAY_LOOP, OnUpdateLoop)
	ON_UPDATE_COMMAND_UI(ID_TV_COPYPASTEKEYS, OnUpdateCopyPasteKeys)
	ON_UPDATE_COMMAND_UI(ID_ADDNODE, OnAddSelectedNodeUpdate )
	ON_NOTIFY(TBN_DROPDOWN, AFX_IDW_TOOLBAR, OnToolbarDropDown)
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_DESTROY()
//	ON_WM_PAINT()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnToolbarDropDown(NMHDR *pnhdr, LRESULT *plr)
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
	switch (pnmtb->iItem)
	{
		case ID_TV_PLAY:
			OnPlayMenu(pos);
			break;
		case ID_TV_RECORD_AUTO:
			OnRecordMenu(pos);
			break;
		default:
			return;
	}
	*plr = TBDDRET_DEFAULT;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnRecordMenu( CPoint pos )
{
	struct SScales
	{
		const char *pszText;
		float fScale;
	};
	SScales Scales[]={
		{" 1 sec ", 1.0f},
		{"1/2 sec", 1.0f/2},
		{"1/5 sec", 1.0f/5},
		{"1/10 sec", 1.0f/10},
		{"1/25 sec", 1.0f/25},
		{"1/50 sec", 1.0f/50},
		{"1/100 sec", 1.0f/100},
	};
	int nScales=sizeof(Scales)/sizeof(SScales);
	CMenu menu;
	menu.CreatePopupMenu();
	for (int i=0;i<nScales;i++)
	{
		int flags = 0;
		if (m_fAutoRecordStep == Scales[i].fScale)
			flags |= MF_CHECKED;
		menu.AppendMenu(MF_STRING|flags, i+1, Scales[i].pszText);
	}
	int cmd = CXTPCommandBars::TrackPopupMenu(&menu, 
							TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, pos.x,pos.y, this, NULL);
	if (cmd >= 1 && cmd <= nScales)
		m_fAutoRecordStep = Scales[cmd-1].fScale;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnPlayMenu( CPoint pos )
{
	struct SScales
	{
		const char *pszText;
		float fScale;
	};
	SScales Scales[]={
		{" 2 ", 2.0f},
		{" 1 ", 1.0f},
		{"1/2", 0.5f},
		{"1/4", 0.25f},
		{"1/8", 0.125f},
	};
	int nScales=sizeof(Scales)/sizeof(SScales);
	CMenu menu;
	menu.CreatePopupMenu();
	for (int i=0;i<nScales;i++)
	{
		int flags = 0;
		if (GetIEditor()->GetAnimation()->GetTimeScale() == Scales[i].fScale)
			flags |= MF_CHECKED;
		menu.AppendMenu(MF_STRING|flags, i+1, Scales[i].pszText);
	}
	//////////////////////////////////////////////////////////////////////////
	menu.AppendMenu(MF_SEPARATOR);
	//////////////////////////////////////////////////////////////////////////
	int flags = 0;
	if (GetIEditor()->GetAnimation()->IsPlayingToAVI())
		flags |= MF_CHECKED;
	menu.AppendMenu(MF_STRING|flags, 100, _T("Encode to AVI") );
	if (!m_aviFile.IsEmpty())
	{
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING|MF_GRAYED, 102, m_aviFile );
	}

	menu.AppendMenu(MF_SEPARATOR);


	bool bSequenceCamera=false;

	CViewport* pRendView = GetIEditor()->GetViewManager()->GetViewport(ET_ViewportCamera);
	if(pRendView)
		bSequenceCamera=pRendView->GetCameraObjectState();

	menu.AppendMenu(MF_STRING|((bSequenceCamera)?MF_CHECKED:MF_UNCHECKED), 200, _T("Sequence Camera") );

	int cmd = CXTPCommandBars::TrackPopupMenu(&menu, 
							TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, pos.x,pos.y, this, NULL);
	if (cmd >= 1 && cmd <= nScales && cmd < 100)
		GetIEditor()->GetAnimation()->SetTimeScale(Scales[cmd-1].fScale);

	switch (cmd)
	{
	case 100:
		{
			if (!GetIEditor()->GetAnimation()->IsPlayingToAVI())
			{
				if (CFileUtil::SelectSaveFile( "AVI Files (*.avi)|*.avi","avi","",m_aviFile ))
				{
					GetIEditor()->GetAnimation()->PlayToAVI( true,m_aviFile );
				}
			}
			else
			{
				GetIEditor()->GetAnimation()->PlayToAVI( false );
			}
		}
		break;
	case 101:
		{
			
		}
		break;
	case 200:
		{
			if(pRendView)
				pRendView->ToggleCameraObject();
		}
		break;

	}
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewDialog::OnInitDialog()
{
	__super::OnInitDialog();

	LoadAccelTable( MAKEINTRESOURCE(IDR_TRACKVIEW) );

	//////////////////////////////////////////////////////////////////////////
	// Create and setup the heightmap edit viewport and the toolbars
	GetCommandBars()->GetCommandBarsOptions()->bShowExpandButtonAlways = FALSE;
	GetCommandBars()->EnableCustomization(FALSE);

	CRect rcClient;
	GetClientRect(rcClient);

	CXTPCommandBar* pMenuBar = GetCommandBars()->SetMenu( _T("Menu Bar"),IDR_TRACKVIEW );
	pMenuBar->SetFlags(xtpFlagStretched);
	pMenuBar->EnableCustomization(FALSE);
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
	//m_wndNodes.Create( WS_VISIBLE|WS_CHILD| TVS_FULLROWSELECT|TVS_HASBUTTONS|TVS_HASLINES|TVS_SHOWSELALWAYS|TVS_LINESATROOT,rc,this,1 );


	m_wndNodesCtrl.Create(WS_CHILD|WS_TABSTOP,CRect(0,0,200,300),this,1 );
	//CreateDockingPane( &m_wndNodesCtrl,IDW_KEYS_PANE,CRect(0,0,200,300),dockLeftOf )->SetTitle( _T("Nodes") );

	m_wndTrackProps.Create( CTVTrackPropsDialog::IDD,this );
	
	// Create key tracks dialog.
	m_wndKeys.Create( NULL,NULL,WS_VISIBLE|WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_HSCROLL,rc,this,2 );
	m_wndKeys.SetTimeRange( 0,20 );
	m_wndKeys.SetTimeScale( 100,0 );
	m_wndKeys.SetCurrTime( 1 );
	m_wndKeys.SetTrackDialog( &m_wndTrackProps );

	m_wndSpline.Create( NULL,NULL,WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|LBS_NOINTEGRALHEIGHT
											|LBS_OWNERDRAWFIXED|LBS_WANTKEYBOARDINPUT|LBS_DISABLENOSCROLL|WS_HSCROLL,rc,this,3 );
	m_wndSpline.SetTimeRange( 0,20 );
	m_wndSpline.SetTimeScale( 100 );
	m_wndSpline.SetCurrentTime( 1 );

	//m_wndNodes.SetKeyListCtrl( &m_wndGraph );
	//m_wndNodes.SetKeyListCtrl( &m_wndKeys );
	m_wndNodesCtrl.SetKeyListCtrl( &m_wndKeys );


	//m_wndNodes.SetItemHeight( 16 );
	
	GetClientRect(rc);
	m_wndSplitter.CreateStatic( this,1,2,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS );
	m_wndSplitter.SetPane( 0,0,&m_wndNodesCtrl, CSize(150, 100) );
	m_wndSplitter.SetPane( 0,1,&m_wndKeys, CSize(300, 100) );
	//m_wndSplitter.SetPane( 0,1,&m_wndGraph, CSize(300, 100) );
	m_wndSplitter.MoveWindow(&rc, TRUE);

	m_wndKeys.SetOwner(this);
	m_wndNodesCtrl.SetOwner(this);

	m_wndNodesCtrl.ShowWindow(SW_SHOW);

	//////////////////////////////////////////////////////////////////////////
	// Create Key properties docking pane.
	CXTPDockingPane *pDockPane1,*pDockPane2;
	{
		m_wndKeyProperties.Create( CTrackViewKeyPropertiesDlg::IDD,this );
		m_wndKeyProperties.PopulateVariables();
		m_wndKeyProperties.SetKeysCtrl(&m_wndKeys);
		
//Diesel cut		
		//CXTPDockingPane *pDockPane = CreateDockingPane( "Key",&m_wndKeyProperties,IDW_KEYS_PANE,CRect(0,0,270,300),dockRightOf );
//Diesel new		
		CXTPDockingPane *pDockPane = CreateDockingPane( "Key",&m_wndKeyProperties,IDW_KEYS_PANE,CRect(0,0,270,300),xtpPaneDockRight );
		
		
		pDockPane->SetOptions(xtpPaneNoCloseable);
		pDockPane1 = pDockPane;
	}

	{
		m_wndTrackProps.ShowWindow(SW_HIDE);
		
//Diesel cut		
		//CXTPDockingPane *pDockPane = CreateDockingPane( "Old Key",&m_wndTrackProps,IDW_TRACK_PROPS_PANE,CRect(0,0,270,300),dockRightOf );
//Diesel new		
		CXTPDockingPane *pDockPane = CreateDockingPane( "Old Key",&m_wndTrackProps,IDW_TRACK_PROPS_PANE,CRect(0,0,270,300),xtpPaneDockRight );
		
		
		pDockPane->SetOptions(xtpPaneNoCloseable);
		GetDockingPaneManager()->HidePane( pDockPane );
		pDockPane2 = pDockPane;
	}
	//GetDockingPaneManager()->DockPane( pDockPane2,xtpPaneDockBottom,pDockPane1 );


	m_wndGraph.Create( CTrackViewGraph::IDD,this );

	{
		m_wndSequences.Create(WS_CHILD|WS_TABSTOP,CRect(0,0,200,300),this,1 );
		m_wndSequences.SetTrackViewDialog(this);
		
//Diesel cut		
		//CXTPDockingPane *pDockPane = CreateDockingPane( "Sequences",&m_wndSequences,IDW_TRACK_SEQUENCES_PANE,CRect(0,0,270,300),dockRightOf );
//Diesel new		
		CXTPDockingPane *pDockPane = CreateDockingPane( "Sequences",&m_wndSequences,IDW_TRACK_SEQUENCES_PANE,CRect(0,0,270,300),xtpPaneDockRight );
		
		
		pDockPane->SetOptions(xtpPaneNoCloseable);
		GetDockingPaneManager()->HidePane(pDockPane);
	}


	//CreateDockingPane( &m_wndSplitter,IDW_KEYS_PANE,CRect(0,0,300,500),dockTopOf );

	RecalcLayout();
	InitSequences();

	m_lazyInitDone = false;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

IMovieSystem* CTrackViewDialog::GetMovieSystem()
{
	if (m_movieSystem)
	{
		return m_movieSystem;
	}
	m_movieSystem = GetIEditor()->GetMovieSystem();

	return m_movieSystem;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::InitToolbar()
{
	// No duplicate toolbars!
	//// Create Library toolbar.
	//CXTPToolBar *pToolBar1 = GetCommandBars()->Add( _T("PlayToolbar"),xtpBarTop );
	//pToolBar1->EnableCustomization(FALSE);
	//VERIFY(pToolBar1->LoadToolBar( IDR_TRACKVIEW_PLAY_BAR ));

	//// Create Library toolbar.
	//CXTPToolBar *pToolBar2 = GetCommandBars()->Add( _T("KeysToolbar"),xtpBarTop );
	//pToolBar2->EnableCustomization(FALSE);
	//VERIFY(pToolBar2->LoadToolBar( IDR_TRACKVIEW_KEYS_BAR ));
	////GetCommandBars()->DockCommandBar( pToolBar2,)

	//DockRightOf( pToolBar2,pToolBar1 );
	


	// Create the toolbar
	m_cDlgToolBar.CreateEx(this, TBSTYLE_FLAT|TBSTYLE_WRAPABLE,
		WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC);
	m_cDlgToolBar.LoadToolBar24(IDR_TRACKVIEW);
	m_cDlgToolBar.GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);
	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_ADDKEY),TBBS_CHECKGROUP );
	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_MOVEKEY),TBBS_CHECKGROUP );
	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_SLIDEKEY),TBBS_CHECKGROUP );
	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_SCALEKEY),TBBS_CHECKGROUP );
	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_RECORD),TBBS_CHECKBOX );
	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_RECORD_AUTO),TBBS_CHECKBOX );
	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_PLAY),TBBS_CHECKBOX );
	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_PAUSE),TBBS_CHECKBOX );
	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_SNAP_NONE),TBBS_CHECKGROUP );
	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_SNAP_MAGNET),TBBS_CHECKGROUP );
	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_SNAP_FRAME),TBBS_CHECKGROUP );
	m_cDlgToolBar.SetButtonStyle( m_cDlgToolBar.CommandToIndex(ID_TV_SNAP_TICK),TBBS_CHECKGROUP );
	int nIndex=m_cDlgToolBar.CommandToIndex(ID_TV_PLAY);
	m_cDlgToolBar.SetButtonStyle(nIndex, m_cDlgToolBar.GetButtonStyle(nIndex) | TBSTYLE_DROPDOWN);
	nIndex=m_cDlgToolBar.CommandToIndex(ID_TV_RECORD_AUTO);
	m_cDlgToolBar.SetButtonStyle(nIndex, m_cDlgToolBar.GetButtonStyle(nIndex) | TBSTYLE_DROPDOWN);
	m_cDlgToolBar.CalcFixedLayout(0,TRUE);

	// Initially move operation is enabled.
	m_cDlgToolBar.GetToolBarCtrl().CheckButton( ID_TV_MOVEKEY,TRUE );

	// No snapping, by default
	m_cDlgToolBar.GetToolBarCtrl().CheckButton( ID_TV_SNAP_NONE,TRUE );

	m_bRecord = false;
	m_bPause = false;
	m_bPlay = false;



	RecalcLayout();

	CRect rc(0,0,0,0);
	int index;
	index = m_cDlgToolBar.CommandToIndex(ID_TV_SEQUENCE);
	m_cDlgToolBar.SetButtonInfo(index, ID_TV_SEQUENCE, TBBS_SEPARATOR, 200);
	m_cDlgToolBar.GetItemRect(index, &rc);
	rc.top++;
	rc.bottom += 400;
	m_sequences.Create( WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|CBS_DROPDOWNLIST|CBS_SORT,rc,this,IDC_SEQUENCES );
	m_sequences.SetFont( CFont::FromHandle((HFONT)gSettings.gui.hSystemFont) );
	m_sequences.SetParent( &m_cDlgToolBar );

	rc.SetRect(0,0,0,0);
	index = m_cDlgToolBar.CommandToIndex(ID_TV_CURSORPOS);
	m_cDlgToolBar.SetButtonInfo(index, ID_TV_CURSORPOS, TBBS_SEPARATOR, 60);
	m_cDlgToolBar.GetItemRect(index, &rc);
	++rc.top;

	m_CursorPos.Create("0.000", WS_CHILD|WS_VISIBLE|SS_CENTER|SS_CENTERIMAGE|SS_SUNKEN,rc,this,IDC_STATIC);

	m_CursorPos.SetFont( CFont::FromHandle((HFONT)gSettings.gui.hSystemFont) );
	m_CursorPos.SetParent( &m_cDlgToolBar );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::InitSequences()
{
	CRect rc;
	int index;
	index = m_cDlgToolBar.CommandToIndex(ID_TV_SEQUENCE);
	m_cDlgToolBar.SetButtonInfo(index, ID_TV_SEQUENCE, TBBS_SEPARATOR, 200);
	m_cDlgToolBar.GetItemRect(index, &rc);
	// Expand the rectangle
	rc.left += 2;
	rc.bottom += 400;
	m_sequences.MoveWindow( rc );

	ReloadSequences();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::InvalidateSequence()
{
	m_bNeedReloadSequence = true;
}

void CTrackViewDialog::InvalidateTrackList()
{
	if (m_wndTrackProps.m_hWnd)
	{
		m_wndTrackProps.ReloadKey();
	}
	if (m_wndKeys.m_hWnd)
		m_wndKeys.Invalidate();
}

//////////////////////////////////////////////////////////////////////////
static void AddTrack( IAnimNode *node )
{
	ITcbKey key;
	ZeroStruct(key);

	IAnimTrack *track = node->GetTrackForParameter(APARAM_POS);
	if (!track)
		return;

	track->SetNumKeys( 10 );
	for (int i = 0; i < track->GetNumKeys(); i++)
	{
		key.time = (frand()+1)/2.0f * 20;
		key.SetFloat( (frand()) * 100 );
		track->SetKey(i,&key);
	}

	track->SortKeys();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::Update()
{
	if (m_bNeedReloadSequence)
	{
		m_bNeedReloadSequence = false;
		IAnimSequence *seq = GetMovieSystem()->FindSequence(m_currSequenceName);
		SetCurrentSequence( seq );
	}
	CAnimationContext *ac = GetIEditor()->GetAnimation();
	float fTime=ac->GetTime();
	m_wndKeys.SetCurrTime(fTime);
	m_wndGraph.SetCurrTime(fTime);
	if (fTime!=m_fLastTime)
	{
		m_fLastTime=fTime;
		char sText[16];
		sprintf(sText, "%1.3f", fTime);
		m_CursorPos.SetWindowText(sText);
	}

	// Check movie camera.
	//GetIEditor()->GetMovieSystem()->GetCameraParams(
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnGoToPrevKey()
{
	CTrackViewNodesCtrl::SItemInfo *pItemInfo=m_wndNodesCtrl.GetSelectedNode();
	if ((!pItemInfo) || (!pItemInfo->node))
		return;
	std::vector<IAnimTrack*> vecTracks;
	if (!pItemInfo->track)
	{
		for (int i=0;i<pItemInfo->node->GetTrackCount();i++)
		{
			vecTracks.push_back(pItemInfo->node->GetTrackByIndex(i));
		}
	}else
	{
		vecTracks.push_back(pItemInfo->track);
	}
	float fClosestKeyTime=-1.0f;
	float fClosestDist=1E8;
	float fCurrTime=GetIEditor()->GetAnimation()->GetTime();
	for (std::vector<IAnimTrack*>::iterator It=vecTracks.begin();It!=vecTracks.end();++It)
	{
		IAnimTrack *pTrack=(*It);
		for (int i=0;i<pTrack->GetNumKeys();i++)
		{
			float fKeyTime=pTrack->GetKeyTime(i);
			float fKeyDist=fCurrTime-fKeyTime;
			if ((fKeyDist>0.0f) && (fKeyDist<fClosestDist))
			{
				fClosestDist=fKeyDist;
				fClosestKeyTime=pTrack->GetKeyTime(i);
			}
		}
	}
	if (fClosestKeyTime>=0.0f)
		GetIEditor()->GetAnimation()->SetTime(fClosestKeyTime);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnGoToNextKey()
{
	CTrackViewNodesCtrl::SItemInfo *pItemInfo=m_wndNodesCtrl.GetSelectedNode();
	if ((!pItemInfo) || (!pItemInfo->node))
		return;
	std::vector<IAnimTrack*> vecTracks;
	if (!pItemInfo->track)
	{
		for (int i=0;i<pItemInfo->node->GetTrackCount();i++)
		{
			vecTracks.push_back(pItemInfo->node->GetTrackByIndex(i));
		}
	}else
	{
		vecTracks.push_back(pItemInfo->track);
	}
	float fClosestKeyTime=-1.0f;
	float fClosestDist=1E8;
	float fCurrTime=GetIEditor()->GetAnimation()->GetTime();
	for (std::vector<IAnimTrack*>::iterator It=vecTracks.begin();It!=vecTracks.end();++It)
	{
		IAnimTrack *pTrack=(*It);
		for (int i=0;i<pTrack->GetNumKeys();i++)
		{
			float fKeyTime=pTrack->GetKeyTime(i);
			float fKeyDist=fKeyTime-fCurrTime;
			if ((fKeyDist>0.0f) && (fKeyDist<fClosestDist))
			{
				fClosestDist=fKeyDist;
				fClosestKeyTime=pTrack->GetKeyTime(i);
			}
		}
	}
	if (fClosestKeyTime>=0.0f)
		GetIEditor()->GetAnimation()->SetTime(fClosestKeyTime);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnAddKey()
{
	m_wndKeys.SetMouseActionMode( TVMODE_ADDKEY );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnDelKey()
{
	m_wndKeys.DelSelectedKeys();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnMoveKey()
{
	m_wndKeys.SetMouseActionMode( TVMODE_MOVEKEY );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSlideKey()
{
	m_wndKeys.SetMouseActionMode( TVMODE_SLIDEKEY );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnScaleKey()
{
	m_wndKeys.SetMouseActionMode( TVMODE_SCALEKEY );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnAddSequence()
{
	CStringDlg dlg( "New Sequence" );
	if (dlg.DoModal() == IDOK)
	{
		CUndo undo("Add Sequence");
		CString seq = dlg.GetString();
		if (!seq.IsEmpty() && !GetIEditor()->GetMovieSystem()->FindSequence(seq))
		{
			IAnimSequence *sequence = GetIEditor()->GetMovieSystem()->CreateSequence(seq);
			GetIEditor()->GetObjectManager()->NewObject("SequenceObject", 0, seq);
			//m_sequences.AddString( seq );
			SetCurrentSequence( sequence );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::ReloadSequences()
{
	if (!GetMovieSystem())
		return;

	CAnimationContext *ac = GetIEditor()->GetAnimation();
	if (ac->IsPlaying())
		ac->SetPlaying( false );
	if (ac->IsRecordMode())
		ac->SetRecording(false);

	if (m_wndTrackProps.m_hWnd)
	{
		//m_wndTrackProps.DestroyWindow();
	}

	m_wndSequences.Reload();

	m_bReloading=true;
	GetIEditor()->GetAnimation()->SetRecording( false );
	ISequenceIt *It= GetMovieSystem()->GetSequences();
	m_sequences.ResetContent();
	IAnimSequence *seq=It->first();
	while (seq)
	{
		m_sequences.AddString( seq->GetName() );
		seq=It->next();
	}
	It->Release();

	// Select current sequence.
	m_currSequence = 0;
	if (!m_currSequenceName.IsEmpty())
	{
		IAnimSequence *seq = GetMovieSystem()->FindSequence(m_currSequenceName);
		SetCurrentSequence( seq );
	}
	if (!m_currSequence && m_sequences.GetCount() > 0)
	{
		m_sequences.SetCurSel(0);
		m_sequences.GetLBText(0,m_currSequenceName);
		IAnimSequence *seq = GetMovieSystem()->FindSequence(m_currSequenceName);
		SetCurrentSequence( seq );
	}

	m_bReloading=false;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::DeleteSequence( const CString &sSequenceName )
{
	assert(sSequenceName);
	CUndo undo("Delete Sequence");
	CBaseObject * pObj = GetIEditor()->GetObjectManager()->FindObject( sSequenceName );
	if(pObj && pObj->IsKindOf(RUNTIME_CLASS(CSequenceObject)))
	{
		GetIEditor()->GetObjectManager()->DeleteObject(pObj);
	}
	IAnimSequence *pSeq = GetMovieSystem()->FindSequence(sSequenceName);
	if (pSeq)
		GetMovieSystem()->RemoveSequence(pSeq);
	ReloadSequences();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnDelSequence()
{
	//CString str;
	//str.Format( 
	if (AfxMessageBox( "Delete current sequence?",MB_YESNO|MB_ICONQUESTION ) == IDYES)
	{
		int sel = m_sequences.GetCurSel();
		CString seq;
		m_sequences.GetLBText(sel,seq);
		
		m_sequences.DeleteString(sel);
		
		if (m_sequences.GetCount() > 0)
		{
			m_sequences.SetCurSel(0);
		}
		OnChangeSequence();
		DeleteSequence(seq);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnEditSequence()
{
	// Edit current sequence.
	if (!m_currSequence)
		return;

	CTVSequenceProps dlg( m_currSequence,this );
	if (dlg.DoModal() == IDOK)
	{
		// Sequence updated.
		ReloadSequences();
	}
	m_wndKeys.Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnChangeSequence()
{
	int sel = m_sequences.GetCurSel();
	if (sel == LB_ERR)
	{
		SetCurrentSequence( 0 );
		return;
	}
	CString name;
	m_sequences.GetLBText( sel,name );
	// Display current sequence.
	IAnimSequence *seq = GetMovieSystem()->FindSequence(name);
	if (seq != m_currSequence)
	{
		SetCurrentSequence( seq );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::SetCurrentSequence( IAnimSequence *curr )
{
	bool bChanged = m_currSequence != curr;
	if (curr)
	{
		m_currSequence = curr;
		GetIEditor()->GetAnimation()->SetSequence( m_currSequence );
		m_currSequenceName = curr->GetName();

		//m_wndNodes.SetSequence( m_currSequence );
		m_wndKeyProperties.SetSequence( m_currSequence );
		m_wndNodesCtrl.SetSequence( m_currSequence );
		m_wndKeys.SetSequence( m_currSequence );
		Range timeRange = m_currSequence->GetTimeRange();
		m_wndKeys.SetTimeRange( timeRange.start,timeRange.end );
		m_wndKeys.SetStartMarker( timeRange.start );
		m_wndKeys.SetEndMarker( timeRange.end );
		m_wndKeys.SetTimeScale( m_wndKeys.GetTimeScale(),0 );
		m_wndGraph.SetSequence( m_currSequence );
		
		m_sequences.SelectString(-1,m_currSequenceName);
	}
	else
	{
		m_currSequence = curr;
		GetIEditor()->GetAnimation()->SetSequence( 0 );
		m_currSequenceName = "";
		m_wndNodesCtrl.SetSequence(0);
		//m_wndNodes.SetSequence( 0 );
		m_wndKeys.SetSequence( 0 );
		m_wndKeyProperties.SetSequence( 0 );
		m_wndGraph.SetSequence( 0 );
	}
	if (bChanged)
	{
		// Notify all objects to update thier track gizmos.
		GetIEditor()->GetObjectManager()->SendEvent( EVENT_UPDATE_TRACKGIZMO );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnRecord()
{
	CAnimationContext *ac = GetIEditor()->GetAnimation();
	ac->SetRecording( !ac->IsRecording() );
	m_wndKeys.Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnAutoRecord()
{
	CAnimationContext *ac = GetIEditor()->GetAnimation();
	ac->SetAutoRecording( !ac->IsRecording(),m_fAutoRecordStep );
	m_wndKeys.Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnGoToStart()
{
	CAnimationContext *ac = GetIEditor()->GetAnimation();
	ac->SetTime(ac->GetMarkers().start);
	ac->SetPlaying( false );
	ac->SetRecording( false );

	if (m_currSequence != NULL)
	{
		// Reset sequence to the beginning.
		m_currSequence->Reset(true);
	}

	// Restore default camera in view.
	//GetIEditor()->GetViewManager()->SetCameraObjectId( m_defaulCameraObjectId );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnGoToEnd()
{
	CAnimationContext *ac = GetIEditor()->GetAnimation();
	ac->SetTime(ac->GetMarkers().end);
	ac->SetPlaying( false );
	ac->SetRecording( false );

	if (m_currSequence != NULL)
	{
		// Reset sequence to the beginning.
		m_currSequence->Reset(true);
	}

	// Restore default camera in view.
	//GetIEditor()->GetViewManager()->SetCameraObjectId( m_defaulCameraObjectId );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnPlay()
{
	m_defaulCameraObjectId = GetIEditor()->GetViewManager()->GetCameraObjectId();

	CAnimationContext *ac = GetIEditor()->GetAnimation();
	if (!ac->IsPlaying())
		ac->SetPlaying( true );
	else
		ac->SetPlaying( false );

	// Set current camera params.
	//OnSetCamera( GetMovieSystem()->GetCameraParams() );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnStop()
{
	CAnimationContext *ac = GetIEditor()->GetAnimation();

	bool bWasPlaying = ac->IsPlaying() || ac->IsRecordMode();
	if (ac->IsPlaying())
		ac->SetPlaying( false );
	else
		OnGoToStart();
	ac->SetRecording( false );

	if (m_currSequence != NULL && !bWasPlaying)
	{
		// Reset sequence to the beginning.
		m_currSequence->Reset(true);
	}

	// Restore default camera in view.
	//GetIEditor()->GetViewManager()->SetCameraObjectId( m_defaulCameraObjectId );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnPause()
{
	CAnimationContext *ac = GetIEditor()->GetAnimation();
	if (ac->IsPaused())
		ac->Resume();
	else
		ac->Pause();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnLoop()
{
	CAnimationContext *ac = GetIEditor()->GetAnimation();
	ac->SetLoopMode(!ac->IsLoopMode());
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnCopyPasteKeys()
{
	if (!m_wndKeys.CopyPasteKeys())
		AfxMessageBox("Copying of keys failed.", MB_ICONEXCLAMATION | MB_OK);
}

void CTrackViewDialog::OnSetFocus(CWnd* pOldWnd)
{
	//__super::OnSetFocus(pOldWnd);

	//if (AfxGetMainWnd())
		//AfxGetMainWnd()->SetFocus();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnUpdateRecord( CCmdUI* pCmdUI )
{
	if (GetIEditor()->GetAnimation()->IsRecordMode())
		pCmdUI->SetCheck(TRUE);
	else
		pCmdUI->SetCheck(FALSE);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnUpdateRecordAuto( CCmdUI* pCmdUI )
{
	if (GetIEditor()->GetAnimation()->IsAutoRecording())
		pCmdUI->SetCheck(TRUE);
	else
		pCmdUI->SetCheck(FALSE);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnUpdatePlay( CCmdUI* pCmdUI )
{
	if (GetIEditor()->GetAnimation()->IsPlayMode())
		pCmdUI->SetCheck(TRUE);
	else
		pCmdUI->SetCheck(FALSE);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnUpdatePause( CCmdUI* pCmdUI )
{
	if (GetIEditor()->GetAnimation()->IsPaused())
		pCmdUI->SetCheck(TRUE);
	else
		pCmdUI->SetCheck(FALSE);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnUpdateLoop( CCmdUI* pCmdUI )
{
	if (GetIEditor()->GetAnimation()->IsLoopMode())
		pCmdUI->SetCheck(TRUE);
	else
		pCmdUI->SetCheck(FALSE);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnUpdateCopyPasteKeys( CCmdUI* pCmdUI )
{
	// deprecated. This button will be removed.
	pCmdUI->Enable(false);//m_wndKeys.CanCopyPasteKeys());
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnBeginNewScene:
	case eNotify_OnCloseScene:
		SetCurrentSequence(NULL);
		break;
	case eNotify_OnBeginSceneOpen:
		m_bInLevelLoad = true;
		m_wndTrackProps.Reset();
		break;
	case eNotify_OnEndSceneOpen:
		m_bInLevelLoad = false;
		ReloadSequences();
		break;
	case eNotify_OnMissionChange:
		if (!m_bInLevelLoad)
			ReloadSequences();
		break;
	case eNotify_OnUpdateTrackView:
		if (!m_bInLevelLoad)
			ReloadSequences();
		break;
	case eNotify_OnIdleUpdate:
		Update();
		break;
	case eNotify_OnUpdateTrackViewKeys:
		if (!m_bInLevelLoad)
		{
			InvalidateTrackList();
			GetIEditor()->GetAnimation()->ForceAnimation();
		}
		break;
	case eNotify_OnUpdateTVKeySelection:
		if (m_currSequence && !m_bInLevelLoad)
			m_wndKeyProperties.OnKeySelectionChange();
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnDestroy()
{
	__super::OnDestroy();

	if (GetMovieSystem())
		GetMovieSystem()->SetCallback(NULL);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnAddSelectedNode()
{
	if (!m_currSequence)
		return;

	//m_wndNodes.AddSelectedNodes();
	AddSelectedNodes(0);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnAddSceneTrack()
{
	if (!m_currSequence)
		return;

	//m_wndNodes.AddSceneNodes();
	CreateAnimNode( ANODE_SCENE,"Scene",0 );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnFindNode()
{
	//CTrackViewFindDlg dlg;
	//dlg.DoModal();
	if(!m_findDlg)
	{
		m_findDlg = new CTrackViewFindDlg;
		if(m_findDlg)
		{
			m_findDlg->Create(CTrackViewFindDlg::IDD, 0);
			m_findDlg->Init(this);
		}
	}

	if(m_findDlg)
	{
		m_findDlg->FillData();
		m_findDlg->ShowWindow(SW_SHOW);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnAddSelectedNodeUpdate( CCmdUI* pCmdUI )
{
	if (GetIEditor()->GetSelection()->IsEmpty() || !m_currSequence)
		pCmdUI->Enable(FALSE);
	else
		pCmdUI->Enable(TRUE);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::SelectNode(const char * sName)
{
	m_wndNodesCtrl.SelectNode(sName);
}

//////////////////////////////////////////////////////////////////////////
BOOL CTrackViewDialog::PreTranslateMessage( MSG* pMsg )
{
	if (pMsg->message == WM_INPUT)
	{
		static C3DConnexionDriver* p3DConnexionDriver = 0;

		if (!p3DConnexionDriver)
		{
			p3DConnexionDriver = (C3DConnexionDriver*)GetIEditor()->GetPluginManager()->GetPluginByGUID( "{AD109901-9128-4ffd-8E67-137CB2B1C41B}" );
		}
		if (p3DConnexionDriver)
		{
			S3DConnexionMessage msg;
			if (p3DConnexionDriver->GetInputMessageData( pMsg->lParam,msg ))
			{
				if (msg.bGotRotation)
				{
					//float fTime = m_wndKeys.GetCurrTime();
					float fTime = GetIEditor()->GetAnimation()->GetTime();
					//float fDelta = msg.vRotate[1] * msg.vRotate[1] * 0.1f;
					float fDelta2 = msg.vRotate[2] * 0.1f;
					//if (msg.vRotate[1] < 0)
						//fTime += fDelta + fDelta2;
					//else
						//fTime -= fDelta + fDelta2;

					fTime += fDelta2;

					//m_wndKeys.SetCurrTime(fTime);

					GetIEditor()->GetAnimation()->SetTime( fTime );
				}
			}
		}
	}
	return __super::PreTranslateMessage(pMsg);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::AddSelectedNodes( IAnimNode *pGroupNode )
{
	if (!m_currSequence)
		return;

	CUndo undo("Add Anim Node(s)");
	CUndo::Record( new CUndoAnimSequenceObject(m_currSequence) );

	// Add selected nodes.
	CSelectionGroup *sel = GetIEditor()->GetSelection();
	for (int i = 0; i < sel->GetCount(); i++)
	{
		CBaseObject *obj = sel->GetObject(i);
		if (!obj)
			continue;

		// Check if object already assigned to some AnimNode.
		if (obj->GetAnimNode())
			continue;

		EAnimNodeType nodeType = ANODE_ENTITY;
		if (obj->IsKindOf( RUNTIME_CLASS(CCameraObject) ))
		{
			nodeType = ANODE_CAMERA;
		}
		else if (obj->IsKindOf( RUNTIME_CLASS(CEntity) ))
		{
			nodeType = ANODE_ENTITY;
		}
		else
		{
			// Not supported
			continue;
		}

		IAnimNode *pAnimNode = m_currSequence->CreateNode( nodeType );
		if (!pAnimNode)
			continue;

		pAnimNode->SetName( obj->GetName() );
		pAnimNode->SetNodeOwner( (CEntity*)obj );
		pAnimNode->SetEntityGuid( ToEntityGuid(obj->GetId()) );

		if(obj->GetLookAt() && obj->GetLookAt()->IsKindOf(RUNTIME_CLASS(CEntity)))
		{
			CEntity *target = (CEntity*)obj->GetLookAt();
			pAnimNode->SetEntityGuidTarget( ToEntityGuid( target->GetId() ));
		}
		if(obj->GetLookAtSource() && obj->GetLookAtSource()->IsKindOf(RUNTIME_CLASS(CEntity)))
		{
			CEntity *source = (CEntity*)obj->GetLookAtSource();
			pAnimNode->SetEntityGuidSource( ToEntityGuid( source->GetId() ));
		}

		pAnimNode->CreateDefaultTracks();

		// Force default parameters.
		pAnimNode->SetPos(0,obj->GetPos());
		pAnimNode->SetRotate(0,obj->GetRotation());
		pAnimNode->SetScale(0,obj->GetScale());
		pAnimNode->SetFlags( pAnimNode->GetFlags()|ANODE_FLAG_SELECTED|ANODE_FLAG_EXPANDED );

		if (obj->IsKindOf( RUNTIME_CLASS(CCameraObject) ))
		{
			nodeType = ANODE_CAMERA;

			float fov = ((CCameraObject*)obj)->GetFOV();
			pAnimNode->SetParamValue( 0,APARAM_FOV,RAD2DEG(fov) );
		}

		// Update&Create track gizmo
		obj->OnEvent( EVENT_UPDATE_TRACKGIZMO );

		pAnimNode->SetParent(pGroupNode);
	}
	GetIEditor()->Notify( eNotify_OnUpdateTrackView );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::CreateAnimNode( EAnimNodeType nodeType,const char *sName,IAnimNode *pGroupNode )
{
	if (!m_currSequence)
		return;

	CUndo undo("Add Scene Node");
	CUndo::Record( new CUndoAnimSequenceObject(m_currSequence) );

	IAnimNode *pAnimNode = m_currSequence->CreateNode( nodeType );
	if (pAnimNode)
	{
		pAnimNode->SetName( sName );
		pAnimNode->CreateDefaultTracks();
		pAnimNode->SetFlags( pAnimNode->GetFlags()|ANODE_FLAG_SELECTED|ANODE_FLAG_EXPANDED );
		pAnimNode->SetParent(pGroupNode);
	}
	GetIEditor()->Notify( eNotify_OnUpdateTrackView );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnModeDopeSheet()
{
	if (GetDockingPaneManager()->FindPane(IDW_TRACK_GRAPH_PANE))
		GetDockingPaneManager()->DestroyPane(IDW_TRACK_GRAPH_PANE);
	m_wndGraph.ShowWindow(SW_HIDE);
	m_wndGraph.SetDlgCtrlID(1);
	m_wndSplitter.SetPane( 0,1,&m_wndKeys, CSize(300, 100) );
	m_wndKeys.ShowWindow(SW_SHOW);
	
	m_wndSplitter.RecalcLayout();
	m_wndGraph.SetSequence(0);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnModeCurveEditor()
{
	if (GetDockingPaneManager()->FindPane(IDW_TRACK_GRAPH_PANE))
		GetDockingPaneManager()->DestroyPane(IDW_TRACK_GRAPH_PANE);
	m_wndKeys.ShowWindow(SW_HIDE);
	m_wndKeys.SetDlgCtrlID(1);
	m_wndSplitter.SetPane( 0,1,&m_wndGraph, CSize(300, 100) );
	m_wndGraph.ShowWindow(SW_SHOW);
	m_wndSplitter.RecalcLayout();
	m_wndGraph.SetSequence(m_currSequence);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnOpenCurveEditor()
{
	OnModeDopeSheet();
	if (!GetDockingPaneManager()->FindPane(IDW_TRACK_GRAPH_PANE))
	{
//Diesel cut		
		//CXTPDockingPane *pDockPane = CreateDockingPane( "Graph",&m_wndGraph,IDW_TRACK_GRAPH_PANE,CRect(0,0,270,300),dockBottomOf );
//Diesel new		
		CXTPDockingPane *pDockPane = CreateDockingPane( "Graph",&m_wndGraph,IDW_TRACK_GRAPH_PANE,CRect(0,0,270,300),xtpPaneDockBottom );
		
		
		pDockPane->SetOptions(xtpPaneNoCloseable);
	}
	m_wndGraph.SetSequence(m_currSequence);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSnapNone()
{
	m_wndKeys.SetSnappingMode(TVKEY_SNAP_NONE);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSnapMagnet()
{
	m_wndKeys.SetSnappingMode(TVKEY_SNAP_MAGNET);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSnapFrame()
{
	m_wndKeys.SetSnappingMode(TVKEY_SNAP_FRAME);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSnapTick()
{
	m_wndKeys.SetSnappingMode(TVKEY_SNAP_TICK);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSnapFPS()
{
	float fps = 30.0f;
	CNumberDlg cDialog( this,fps,"Frame rate for frame snapping" );
	cDialog.SetInteger(true);
	cDialog.SetRange(1.0f, 120.0f);

	if (cDialog.DoModal() == IDOK)
	{
		fps = cDialog.GetValue();
		m_wndKeys.SetSnapFPS(FloatToIntRet(fps));
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDialog::OnSize( UINT nType, int cx, int cy )
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
		m_sequences.MoveWindow( rc );

		rc.SetRect(0,0,0,0);
		index = m_cDlgToolBar.CommandToIndex(ID_TV_CURSORPOS);
		m_cDlgToolBar.SetButtonInfo(index, ID_TV_CURSORPOS, TBBS_SEPARATOR, 60);
		m_cDlgToolBar.GetItemRect(index, &rc);
		++rc.top;
		m_CursorPos.MoveWindow( rc );

		m_wndNodesCtrl.SyncKeyCtrl();

		m_lazyInitDone = true;
	}
}