// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "MainFrm.h"

#include "CryEdit.h"
#include "CryEditDoc.h"
#include "MainTools.h"
#include "TerrainPanel.h"
#include "PanelDisplayHide.h"
#include "PanelDisplayRender.h"
#include "PanelDisplayLayer.h"
#include "Mission.h"
#include "ViewManager.h"
#include "LayoutWnd.h"
#include "ExternalTools.h"
#include "Settings.h"
#include "GridSettingsDialog.h"
#include "UndoDropDown.h"
#include "CharacterEditor\CharacterEditor.h"
#include "SelectObjectDlg.h"

#include "PropertiesPanel.h"
#include "MusicInfoDlg.h"
#include "Objects\ObjectManager.h"

#include "TrackView\TrackViewDialog.h"
#include "DataBaseDialog.h"
#include "ErrorReportDialog.h"
#include "Material\MaterialDialog.h"
#include "Material\MaterialManager.h"
#include "LightmapCompiler\LightmapCompilerDialog.h"

#include "Vehicles\VehicleEditorDialog.h"
#include "SmartObjects\SmartObjectsEditorDialog.h"
#include "AI\AIDebugger.h"

#include "HyperGraph\HyperGraphDialog.h"
#include "HyperGraph\AnimationGraphDialog.h"
#include "HyperGraph\AnimationGraph2\AnimationGraphDialog_2.h"

#include "FacialEditor\FacialEditorDialog.h"

#include "DialogEditor\DialogEditorDialog.h"
#include "Modelling\ModellingToolsPanel.h"

#include "TimeOfDayDialog.h"
#include "VisualLogViewer/VisualLogWnd.h"

#include "Texture Browser\TextureBrowserDialog.h"
#include "Asset Browser/AssetBrowserDialog.h"
#include "LNM\LNMEditorDialog.h"

#include <IBudgetingSystem.h>
#include <ISound.h>
#include <IAudioDevice.h> // Update on IAudioDevice for network audition
//#include <ICryAnimation.h>
#include "SoundBrowserDialog.h"

#include "ProcessInfo.h"

#include "StringDlg.h"
#include "Util/StringNoCasePredicate.h"

#include "ControlMRU.h"
#include "Controls/CrytekTheme.h"

#define  IDW_VIEW_EDITMODE_BAR		AFX_IDW_CONTROLBAR_FIRST+10
#define  IDW_VIEW_OBJECT_BAR			AFX_IDW_CONTROLBAR_FIRST+11
#define  IDW_VIEW_MISSION_BAR			AFX_IDW_CONTROLBAR_FIRST+12
#define  IDW_VIEW_TERRAIN_BAR			AFX_IDW_CONTROLBAR_FIRST+13
#define  IDW_VIEW_AVI_RECORD_BAR	AFX_IDW_CONTROLBAR_FIRST+14

#define  IDW_VIEW_ROLLUP_BAR			AFX_IDW_CONTROLBAR_FIRST+20
#define  IDW_VIEW_CONSOLE_BAR			AFX_IDW_CONTROLBAR_FIRST+21
#define  IDW_VIEW_INFO_BAR				AFX_IDW_CONTROLBAR_FIRST+22
#define  IDW_VIEW_TRACKVIEW_BAR		AFX_IDW_CONTROLBAR_FIRST+23
#define  IDW_VIEW_DIALOGTOOL_BAR	AFX_IDW_CONTROLBAR_FIRST+24
#define  IDW_VIEW_DATABASE_BAR		AFX_IDW_CONTROLBAR_FIRST+25

#define BAR_SECTION _T("Bars")

#define AUTOSAVE_TIMER_EVENT 200
#define AUTOREMIND_TIMER_EVENT 201
#define NETWORK_AUDITION_UPDATE_TIMER_EVENT 202

#define STYLES_PATH "Editor\\Styles"

#define LAYOUTS_PATH "Editor\\Layouts\\"
#define LAYOUTS_EXTENSION ".layout"
#define LAYOUTS_WILDCARD "*.layout"
#define DUMMY_LAYOUT_NAME "Dummy_Layout"






/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CXTPFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CXTPFrameWnd)
	ON_WM_CREATE()

	ON_COMMAND_EX(ID_VIEW_MENUBAR, OnBarCheck )
	ON_UPDATE_COMMAND_UI(ID_VIEW_MENUBAR, OnUpdateControlBarMenu)

	ON_COMMAND_EX(ID_VIEW_STATUS_BAR, OnBarCheck )
	ON_UPDATE_COMMAND_UI(ID_VIEW_STATUS_BAR, OnUpdateControlBarMenu)

	ON_COMMAND_EX(IDW_VIEW_ROLLUP_BAR, OnToggleBar )
	ON_UPDATE_COMMAND_UI(IDW_VIEW_ROLLUP_BAR, OnUpdateControlBar )

	ON_COMMAND_EX(IDW_VIEW_CONSOLE_BAR, OnToggleBar )
	ON_UPDATE_COMMAND_UI(IDW_VIEW_CONSOLE_BAR, OnUpdateControlBar )

	ON_COMMAND_EX(IDW_VIEW_DIALOGTOOL_BAR, OnToggleBar )
	ON_UPDATE_COMMAND_UI(IDW_VIEW_DIALOGTOOL_BAR, OnUpdateControlBar )

	ON_COMMAND(ID_SOUND_SHOWMUSICINFO, OnMusicInfo)
	ON_COMMAND(ID_SOUND_OPENSOUNDBROWSER, OnOpenSoundBrowser)
	ON_COMMAND(ID_SOUND_OPENDIALOGBROWSER, OnOpenDialogBrowser)
	

	ON_UPDATE_COMMAND_UI(ID_SNAP_TO_GRID, OnUpdateSnapToGrid)
	ON_UPDATE_COMMAND_UI(ID_LAYER_SELECT, OnUpdateCurrentLayer)

	ON_COMMAND_EX(ID_THEME_OFFICE2000, OnSwitchTheme)
	ON_COMMAND_EX(ID_THEME_OFFICEXP, OnSwitchTheme)
	ON_COMMAND_EX(ID_THEME_OFFICE2003, OnSwitchTheme)
	ON_COMMAND_EX(ID_THEME_OFFICE2007, OnSwitchTheme)
	ON_COMMAND_EX(ID_THEME_WINDOWSXP, OnSwitchTheme)
	ON_COMMAND_EX(ID_THEME_EXPLORER, OnSwitchTheme)
	ON_COMMAND_EX(ID_THEME_VISIO, OnSwitchTheme)
	ON_COMMAND_EX(ID_THEME_VISUALSTUDIO2005, OnSwitchTheme)
	ON_COMMAND_EX(ID_THEME_GRIPPERED, OnSwitchTheme)
	//ON_COMMAND_EX(ID_THEME_SIMPLE, OnSwitchTheme)

	ON_UPDATE_COMMAND_UI(ID_THEME_OFFICE2000, OnUpdateTheme)
	ON_UPDATE_COMMAND_UI(ID_THEME_OFFICEXP, OnUpdateTheme)
	ON_UPDATE_COMMAND_UI(ID_THEME_OFFICE2003, OnUpdateTheme)
	ON_UPDATE_COMMAND_UI(ID_THEME_OFFICE2007, OnUpdateTheme)
	ON_UPDATE_COMMAND_UI(ID_THEME_WINDOWSXP, OnUpdateTheme)
	ON_UPDATE_COMMAND_UI(ID_THEME_EXPLORER, OnUpdateTheme)
	ON_UPDATE_COMMAND_UI(ID_THEME_VISIO, OnUpdateTheme)
	ON_UPDATE_COMMAND_UI(ID_THEME_VISUALSTUDIO2005, OnUpdateTheme)
	ON_UPDATE_COMMAND_UI(ID_THEME_GRIPPERED, OnUpdateTheme)
	//ON_UPDATE_COMMAND_UI(ID_THEME_SIMPLE, OnUpdateTheme)

	ON_COMMAND(ID_VIEW_DOCKINGHELPERS, OnDockingHelpers)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DOCKINGHELPERS, OnUpdateDockingHelpers)
	ON_COMMAND(ID_VIEW_SKINING, OnSkining)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SKINING, OnUpdateSkining)
	ON_COMMAND_EX(ID_SKINS_CRYDARK, OnSwitchSkin)
	ON_COMMAND_EX(ID_SKINS_CRYLIGHT, OnSwitchSkin)
	ON_UPDATE_COMMAND_UI(ID_SKINS_CRYDARK, OnUpdateSkin)
	ON_UPDATE_COMMAND_UI(ID_SKINS_CRYLIGHT, OnUpdateSkin)

	ON_UPDATE_COMMAND_UI(ID_INDICATOR_MEMORY,OnUpdateMemoryStatus)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_CONSOLE_MEMORY,OnUpdateConsoleMemoryStatus)

	ON_COMMAND_RANGE(ID_TOOL1,ID_TOOL_LAST,OnExecuteTool)
	ON_COMMAND_RANGE(ID_VIEW_OPENPANE0,ID_VIEW_OPENPANE_LAST,OnOpenViewPane)

	ON_COMMAND( ID_VIEW_SAVELAYOUT, OnViewSavePaneLayout )
	ON_COMMAND( ID_VIEW_LAYOUT_LOAD_DEFAULT, OnViewRestoreDefaultLayout )
	ON_COMMAND_RANGE( ID_VIEW_LAYOUT_FIRST, ID_VIEW_LAYOUT_LAST, OnViewLoadPaneLayout )
	ON_COMMAND_RANGE( ID_VIEW_LAYOUT_LOAD_FIRST, ID_VIEW_LAYOUT_LOAD_LAST, OnViewLoadPaneLayout )
	ON_COMMAND_RANGE( ID_VIEW_LAYOUT_RENAME_FIRST, ID_VIEW_LAYOUT_RENAME_LAST, OnViewRenamePaneLayout )
	ON_COMMAND_RANGE( ID_VIEW_LAYOUT_DELETE_FIRST, ID_VIEW_LAYOUT_DELETE_LAST, OnViewDeletePaneLayout )
	ON_COMMAND_RANGE( ID_VIEW_LAYOUT_SAVE_FIRST, ID_VIEW_LAYOUT_SAVE_LAST, OnViewSavePaneLayout )

	//ON_COMMAND_RANGE( ID_FILE_MRU_REMOVE_FIRST, ID_FILE_MRU_REMOVE_LAST, OnFileMRURemove )

	ON_BN_CLICKED(ID_PROGRESSBAR_CANCEL,OnProgressCancelClicked)

	ON_WM_SIZE()
	ON_WM_COPYDATA()
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_COMMAND(ID_EDIT_NEXTSELECTIONMASK, OnEditNextSelectionMask)

	//////////////////////////////////////////////////////////////////////////
	// XT Commands.
	ON_MESSAGE(XTPWM_DOCKINGPANE_NOTIFY, OnDockingPaneNotify)
	
	ON_XTP_EXECUTE(IDC_SELECTION, OnSelectionChanged)
	ON_COMMAND(ID_SELECTION_DELETE, OnDeleteSelection)

	ON_XTP_EXECUTE(IDC_SELECTION_MASK, OnSelectionMaskChanged)
	ON_XTP_EXECUTE(ID_REF_COORDS_SYS, OnRefCoordSysChange)
	ON_XTP_EXECUTE(IDC_MISSION, OnMissionChanged)
	ON_UPDATE_COMMAND_UI(IDC_SELECTION_MASK, OnUpdateSelectionMask)
	ON_UPDATE_COMMAND_UI(ID_REF_COORDS_SYS, OnUpdateRefCoordSys)

	ON_COMMAND(XTP_ID_CUSTOMIZE, OnCustomize)
	ON_XTP_CREATECOMMANDBAR()
	ON_XTP_CREATECONTROL()
	ON_XTP_INITCOMMANDSPOPUP()

	ON_MESSAGE(WM_MATEDITSEND, OnMatEditSend)

END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_GAME_INFO,
	ID_XBOX360_CONNECTION,
	ID_PS3_CONNECTION,
	ID_INDICATOR_CONSOLE_MEMORY,
	ID_INDICATOR_MEMORY,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

namespace
{
	XTPPaintTheme s_themePaint = xtpThemeOffice2003;
	XTPDockingPanePaintTheme s_themePane = xtpPaneThemeOffice2003;
	BOOL s_bDockingHelpers = TRUE;
//	CString s_lpszSkinPath = "Editor\\Styles\\CryDark.cjstyles"; //Diesel cut
}

//////////////////////////////////////////////////////////////////////////
class CControlDropDownCmd : public CXTPControlPopup
{
public:
	DECLARE_XTP_CONTROL(CControlDropDownCmd)
	CControlDropDownCmd() { m_controlType = xtpControlSplitButtonPopup; };
	// Input:	bPopup - TRUE to set popup.
	// Returns: TRUE if successful; otherwise returns FALSE
	// Summary: This method is called to popup the control.
	virtual BOOL OnSetPopup(BOOL bPopup)
	{
		if (bPopup)
		{
			CRect rc = GetRect();
			GetParent()->ClientToScreen(rc);
			((CMainFrame*)AfxGetMainWnd())->OnToolbarDropDown(GetID(),rc);
		}
		return TRUE;
	}
	/*
	virtual BOOL IsContainPopup(CXTPControlPopup* pControlPopup)
	{
		return TRUE;
	}
	*/
};
IMPLEMENT_XTP_CONTROL(CControlDropDownCmd,CXTPControlPopup)

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

//HANDLE hLowMemSignal = 0;

CMainFrame::CMainFrame()
{
	m_autoSaveTimer = 0;
	m_autoRemindTimer = 0;
	m_networkAuditionTimer = 0;

	//////////////////////////////////////////////////////////////////////////
	m_currentMission = 0;
	m_currentLayer = 0;

	m_terrainPanel = 0;
	m_mainTools = 0;
	
	m_gridSize = -1;
	m_coordSys = (RefCoordSys)-1;
	m_objectSelectionMask = 0;

	m_bAcceleratorsEnabled = true;

	m_pDefaultLayout = NULL;

	m_pEditToolBar=NULL;
	m_poConsoleToolbar=NULL;

	m_layoutWnd = 0;

	CXTRegistryManager regMgr;
	s_themePaint = (XTPPaintTheme)regMgr.GetProfileInt(_T("Settings"), _T("PaintTheme"), s_themePaint );
	s_themePane = (XTPDockingPanePaintTheme)regMgr.GetProfileInt(_T("Settings"), _T("PaneTheme"), s_themePane);
	// The Skining is forced to 1 because we are only supporting the internal skin now.
	gSettings.gui.bSkining = 1;//regMgr.GetProfileInt(_T("Settings"), _T("Skining"),1); 

//Diesel cut
	//s_lpszSkinPath = regMgr.GetProfileString(_T("Settings"), _T("SkinPath"), s_lpszSkinPath);;
 
	s_bDockingHelpers = regMgr.GetProfileInt(_T("Settings"), _T("DockingHelpers"), s_bDockingHelpers );

	GetIEditor()->GetExternalToolsManager()->Load();

	// Enable/Disable Menu Shadows
	//xtAfxData.bMenuShadows = FALSE;

	RegisterStdViewClasses();

	GetIEditor()->RegisterNotifyListener(this);

	//////////////////////////////////////////////////////////////////////////
//	hLowMemSignal = CreateMemoryResourceNotification( LowMemoryResourceNotification );
	//////////////////////////////////////////////////////////////////////////
}

CMainFrame::~CMainFrame()
{
	//if (hLowMemSignal)
		//CloseHandle(hLowMemSignal);
	if (m_layoutWnd)
		delete m_layoutWnd;
	m_layoutWnd = 0;

	GetIEditor()->UnregisterNotifyListener(this);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	//{
	//	ICVar* pCVar = gEnv->pConsole->GetCVar("sys_game_folder"); assert(pCVar);
	//	m_strTitle += "   [GameFolder: " + ((CString)pCVar->GetString())+"]";
	//	pCVar = gEnv->pConsole->GetCVar("sys_dll_game");  assert(pCVar);
	//	m_strTitle += "[GameDLL: " + ((CString)pCVar->GetString()) + "] ";
	//}

#ifdef WIN64
	m_strTitle += " (x64 Edition)";
#endif //WIN64

	if (__super::OnCreate(lpCreateStruct) == -1)
		return -1;

	//////////////////////////////////////////////////////////////////////////
	// Start autosave timer.
	//////////////////////////////////////////////////////////////////////////
	ResetAutoSaveTimers();
	
	m_networkAuditionTimer = SetTimer( NETWORK_AUDITION_UPDATE_TIMER_EVENT, 1000, 0 );

	//////////////////////////////////////////////////////////////////////////
	// Initialize the command bars
	if (!InitCommandBars())
		return -1;

	// Get a pointer to the command bars object.
	CXTPCommandBars* pCommandBars = GetCommandBars();
	if(pCommandBars == NULL)
	{
		TRACE0("Failed to create command bars object.\n");
		return -1;      // fail to create
	}

	// Initialize defaul accelerators.
	pCommandBars->GetShortcutManager()->SetAccelerators(IDR_MAINFRAME);

	// Set pain theme and skin.
	SwitchTheme(s_themePaint,s_themePane);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Create the status bar
	//////////////////////////////////////////////////////////////////////////
	{
		VERIFY(m_wndStatusBar.Create(this) );
		VERIFY(m_wndStatusBar.SetIndicators( indicators,sizeof(indicators)/sizeof(UINT) ) );
	} 
	//////////////////////////////////////////////////////////////////////////

	if (!IsPreview())
	{
		// Add the menu bar
		CXTPCommandBar* pMenuBar = pCommandBars->SetMenu( _T("Menu Bar"),IDR_MAINFRAME );
		ASSERT(pMenuBar);


		//////////////////////////////////////////////////////////////////////////
		// Create toolbars.
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// Create standart toolbar.
		CXTPToolBar *pStdToolBar = pCommandBars->Add( _T("Standard ToolBar"),xtpBarTop );
		pStdToolBar->SetVisible(FALSE);
		VERIFY(pStdToolBar->LoadToolBar(IDR_MAINFRAME));
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// Create EditMode Toolbar.
		CXTPToolBar *pEditToolBar = pCommandBars->Add( _T("EditMode ToolBar"),xtpBarTop );
		pEditToolBar->EnableCustomization(FALSE);
		VERIFY(pEditToolBar->LoadToolBar(IDR_EDIT_MODE));
		m_pEditToolBar=pEditToolBar;
		//DockRightOf(pEditToolBar, pStdToolBar);
		//////////////////////////////////////////////////////////////////////////


		//////////////////////////////////////////////////////////////////////////
		// Object Modify Toolbar
		CXTPToolBar *pObjectToolBar = pCommandBars->Add( _T("Object ToolBar"),xtpBarTop );
		VERIFY(pObjectToolBar->LoadToolBar(IDR_OBJECT_MODIFY));
		DockRightOf(pObjectToolBar,pEditToolBar);
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		CXTPToolBar *pAviToolBar = pCommandBars->Add( _T("AVI Recorder ToolBar"),xtpBarTop );
		VERIFY(pAviToolBar->LoadToolBar(IDR_AVI_RECORDER_BAR));
		pAviToolBar->SetVisible(FALSE);
		//DockRightOf(pAviToolBar,pObjectToolBar);
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		CXTPToolBar *pMissionToolBar = pCommandBars->Add( _T("Mission ToolBar"),xtpBarTop );
		VERIFY(pMissionToolBar->LoadToolBar(IDR_MISSION_BAR));
		pMissionToolBar->EnableCustomization(FALSE);
		pMissionToolBar->SetVisible(FALSE);
		//DockRightOf(pMissionToolBar,pAviToolBar);
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		CXTPToolBar *pTerrainToolBar = pCommandBars->Add( _T("Terrain ToolBar"),xtpBarTop );
		VERIFY(pTerrainToolBar->LoadToolBar(IDR_TERRAIN_BAR));
		pTerrainToolBar->SetVisible(FALSE);
		//DockRightOf(pTerrainToolBar,pMissionToolBar);
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		CXTPToolBar *pDialogsToolBar = pCommandBars->Add( _T("Dialogs ToolBar"),xtpBarTop );
		VERIFY(pDialogsToolBar->LoadToolBar(IDR_STDVIEWS_BAR));
		DockRightOf(pDialogsToolBar,pObjectToolBar);
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// Create Console Toobar
		CXTPToolBar *pConsoleToolbar = pCommandBars->Add( _T("Console Toolbar"),xtpBarTop );
		pConsoleToolbar->EnableCustomization(FALSE);
		VERIFY(pConsoleToolbar->LoadToolBar(IDR_CONSOLE_TOOLBAR));
		m_poConsoleToolbar=pConsoleToolbar;
		DockRightOf(pConsoleToolbar,pDialogsToolBar);
		//////////////////////////////////////////////////////////////////////////
	}


  // Initialize the docking pane manager and set the
	// initial them for the docking panes.  Do this only after all
	// control bars objects have been created and docked.
	GetDockingPaneManager()->InstallDockingPanes(this);
	GetDockingPaneManager()->SetTheme(s_themePane);
	if (!gSettings.gui.bWindowsVista)
		GetDockingPaneManager()->SetThemedFloatingFrames(TRUE);
	if (s_bDockingHelpers)
	{
		GetDockingPaneManager()->SetAlphaDockingContext(TRUE);
		GetDockingPaneManager()->SetShowDockingContextStickers(TRUE);
	}

	//////////////////////////////////////////////////////////////////////////
	// Create the console window.
	// Create the workspace bar.
	
//Diesel cut
	//CXTPDockingPane* pDockConsole = GetDockingPaneManager()->CreatePane( IDW_VIEW_CONSOLE_BAR,CRect(0,0,500,70),dockBottomOf);
//Diesel new	
	CXTPDockingPane* pDockConsole = GetDockingPaneManager()->CreatePane( IDW_VIEW_CONSOLE_BAR,CRect(0,0,500,70),xtpPaneDockBottom);
	
	
	pDockConsole->SetTitle( _T("Console") );
	m_cConsole.Create( NULL,NULL,WS_CHILD|WS_VISIBLE,CRect(0,0,0,0),this,0 );

	//////////////////////////////////////////////////////////////////////////
	// Create the RollupBar.
	CreateRollupBar();
	//////////////////////////////////////////////////////////////////////////

	if (IsPreview())
	{
		// Hide all menus.
		//ShowControlBar( pMenuBar,FALSE,0 );
		//ShowControlBar( &m_wndReBar,FALSE,0 );
		//ShowControlBar( &m_wndTrackViewBar,FALSE,0 );
		//ShowControlBar( &m_wndDataBaseBar,FALSE,0 );
	}
	else
	{
		// Update tools menu,
		UpdateToolsMenu();

//Diesel cut		
		//CXTPDockingPane* pDockPane = GetDockingPaneManager()->CreatePane( IDW_VIEW_DIALOGTOOL_BAR,CRect( 100,200,200,600),dockLeftOf);
//Diesel new		
		CXTPDockingPane* pDockPane = GetDockingPaneManager()->CreatePane( IDW_VIEW_DIALOGTOOL_BAR,CRect( 100,200,200,600),xtpPaneDockLeft);
		
		
		pDockPane->SetTitle( _T("ToolBox") );
		m_toolBoxDialog.Create( CToolBoxDialog::IDD,this );
		GetDockingPaneManager()->ClosePane(pDockPane->GetID());
	}

	//
	// set the game info status text
	//
	{
		CString strGameInfo;
		ICVar* pCVar = gEnv->pConsole->GetCVar( "sys_game_folder" );
		if (pCVar)
		{
			strGameInfo = "GameFolder: '" + ((CString)pCVar->GetString()) + "'";
		}
		int nIndex;
		nIndex = m_wndStatusBar.CommandToIndex( ID_INDICATOR_GAME_INFO );

		if( ( nIndex < 0 ) || ( nIndex >= m_wndStatusBar.GetPaneCount() ) )
		{
			assert( "CMainFrame::set game info status text" && false );
		}
		else
		{
			m_wndStatusBar.SetPaneText( nIndex, strGameInfo );
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
int CMainFrame::OnCreateControl(LPCREATECONTROLSTRUCT lpCreateControl)
{
	//m_wndTerrainToolBar.SetButtonText( m_wndTerrainToolBar.CommandToIndex(ID_TERRAIN),_T("Terrain") );
	//m_wndTerrainToolBar.SetButtonText( m_wndTerrainToolBar.CommandToIndex(ID_GENERATORS_TEXTURE),_T("Texture") );
	//m_wndTerrainToolBar.SetButtonText( m_wndTerrainToolBar.CommandToIndex(ID_GENERATORS_LIGHTING),_T("Lighting") );
	
	if (lpCreateControl->bToolBar)
	{

		if (lpCreateControl->nID == ID_TERRAIN || lpCreateControl->nID == ID_GENERATORS_TEXTURE || lpCreateControl->nID == ID_GENERATORS_LIGHTING)
		{
			CXTPControlButton* pButton = (CXTPControlButton*)CXTPControlButton::CreateObject();		
			lpCreateControl->pControl = pButton;
			pButton->SetID(lpCreateControl->nID);
			if (lpCreateControl->nID == ID_TERRAIN)
				pButton->SetCaption(_T("Terrain"));
			if (lpCreateControl->nID == ID_GENERATORS_TEXTURE)
				pButton->SetCaption(_T("Texture"));
			if (lpCreateControl->nID == ID_GENERATORS_LIGHTING)
				pButton->SetCaption(_T("Lighting"));
			pButton->SetStyle(xtpButtonIconAndCaption);
			return TRUE;		
		}
		else if (lpCreateControl->nID == ID_LAYER_SELECT)
		{
			CXTPControlButton* pButton = (CXTPControlButton*)CXTPControlButton::CreateObject();		
			lpCreateControl->pControl = pButton;
			pButton->SetID(lpCreateControl->nID);
			pButton->SetStyle(xtpButtonIconAndCaption);
			return TRUE;
		}
		else if (lpCreateControl->nID == ID_SNAP_TO_GRID)
		{
			CControlDropDownCmd* pCmdPopup = new CControlDropDownCmd();
			lpCreateControl->pControl = pCmdPopup;
			pCmdPopup->SetID(lpCreateControl->nID);
			pCmdPopup->SetStyle(xtpButtonIconAndCaption);
			return TRUE;		
		}
		else if (lpCreateControl->nID == ID_UNDO)
		{
			CControlDropDownCmd* pCmdPopup = new CControlDropDownCmd();
			lpCreateControl->pControl = pCmdPopup;
			pCmdPopup->SetID(lpCreateControl->nID);
			pCmdPopup->SetStyle(xtpButtonAutomatic);
			return TRUE;		
		}
		else if (lpCreateControl->nID == ID_REDO)
		{
			CControlDropDownCmd* pCmdPopup = new CControlDropDownCmd();
			lpCreateControl->pControl = pCmdPopup;
			pCmdPopup->SetID(lpCreateControl->nID);
			pCmdPopup->SetStyle(xtpButtonAutomatic);
			return TRUE;		
		}
		else if (lpCreateControl->nID == IDC_SELECTION)
		{
			CXTPControlComboBox* pComboBox = (CXTPControlComboBox*)CXTPControlComboBox::CreateObject();
			pComboBox->SetDropDownListStyle();
			
			pComboBox->SetWidth(100);
			pComboBox->SetCaption(_T("Selection Set"));
			pComboBox->SetStyle(xtpComboNormal);
			pComboBox->SetFlags(xtpFlagManualUpdate);

			lpCreateControl->pControl = pComboBox;
			return TRUE;		
		}
		else if (lpCreateControl->nID == ID_REF_COORDS_SYS)
		{
			CXTPControlComboBox* pComboBox = (CXTPControlComboBox*)CXTPControlComboBox::CreateObject();
			pComboBox->SetDropDownListStyle(FALSE);

			pComboBox->AddString("View");
			pComboBox->AddString("Local");
			pComboBox->AddString("Parent");
			pComboBox->AddString("World");
			pComboBox->AddString("User Def.");

			pComboBox->SetWidth(70);
			pComboBox->SetStyle(xtpComboNormal);
			//pComboBox->SetFlags(xtpFlagManualUpdate);
			lpCreateControl->pControl = pComboBox;
			return TRUE;		
		}
		else if (lpCreateControl->nID == IDC_SELECTION_MASK)
		{
			CXTPControlComboBox* pComboBox = (CXTPControlComboBox*)CXTPControlComboBox::CreateObject();
			pComboBox->SetDropDownListStyle(FALSE);
			int id;
			id = pComboBox->AddString("Select All"); pComboBox->SetItemData( id,OBJTYPE_ANY );
			id = pComboBox->AddString("Brushes"); pComboBox->SetItemData( id,OBJTYPE_BRUSH );
			id = pComboBox->AddString("No Brushes"); pComboBox->SetItemData( id,(~OBJTYPE_BRUSH) );
			id = pComboBox->AddString("Entities"); pComboBox->SetItemData( id,OBJTYPE_ENTITY );
			id = pComboBox->AddString("Prefabs"); pComboBox->SetItemData( id,OBJTYPE_PREFAB );
			id = pComboBox->AddString("Areas, Shapes"); pComboBox->SetItemData( id,OBJTYPE_VOLUME | OBJTYPE_SHAPE );
			id = pComboBox->AddString("AI Points"); pComboBox->SetItemData( id,OBJTYPE_AIPOINT );
			id = pComboBox->AddString("Decals"); pComboBox->SetItemData( id,OBJTYPE_DECAL );
			id = pComboBox->AddString("Solids"); pComboBox->SetItemData( id,OBJTYPE_SOLID );
			id = pComboBox->AddString("No Solids"); pComboBox->SetItemData( id,(~OBJTYPE_SOLID) );
			pComboBox->SetWidth(80);
			pComboBox->SetStyle(xtpComboNormal);
			//pComboBox->SetFlags(xtpFlagManualUpdate);
			pComboBox->SetCurSel(0);

			lpCreateControl->pControl = pComboBox;
			return TRUE;
		}
		else if (lpCreateControl->nID == IDC_MISSION)
		{
			CXTPControlComboBox* pComboBox = (CXTPControlComboBox*)CXTPControlComboBox::CreateObject();
			pComboBox->SetDropDownListStyle(FALSE);
			pComboBox->SetWidth(100);
			pComboBox->SetStyle(xtpComboNormal);
			lpCreateControl->pControl = pComboBox;
			return TRUE;
		}
	}
	
	if (lpCreateControl->nID == ID_TOOLS_TOOL1)
	{
		CXTPControlPopup* pPopup = CXTPControlPopup::CreateControlPopup(xtpControlPopup);
		pPopup->SetFlags(xtpFlagManualUpdate);
		lpCreateControl->pControl = pPopup;
		CXTPPopupBar* pToolsMenuBar = CXTPPopupBar::CreatePopupBar( GetCommandBars() );
		pToolsMenuBar->SetTearOffPopup(_T("User Commands"), ID_TOOLS_TOOL1);
		pPopup->SetCommandBar(pToolsMenuBar);
		return TRUE;
	}
	if (lpCreateControl->nID == ID_VIEW_OPENVIEWPANE)
	{
		CXTPControlPopup* pPopup = CXTPControlPopup::CreateControlPopup(xtpControlPopup);
		pPopup->SetFlags(xtpFlagManualUpdate);
		lpCreateControl->pControl = pPopup;
		CXTPPopupBar* pToolsMenuBar = CXTPPopupBar::CreatePopupBar( GetCommandBars() );
		pPopup->SetCommandBar(pToolsMenuBar);
		return TRUE;
	}

	if ( lpCreateControl->nID == ID_VIEW_LAYOUTS )
	{
		CXTPControlPopup* pPopup = CXTPControlPopup::CreateControlPopup( xtpControlPopup );
		pPopup->SetFlags( xtpFlagManualUpdate );
		lpCreateControl->pControl = pPopup;
		CXTPPopupBar* pToolsMenuBar = CXTPPopupBar::CreatePopupBar( GetCommandBars() );
		pPopup->SetCommandBar( pToolsMenuBar );
		return TRUE;
	}

	if ( ID_VIEW_LAYOUT_FIRST <= lpCreateControl->nID && lpCreateControl->nID <= ID_VIEW_LAYOUT_LAST )
	{
		CXTPControlPopup* pPopup = CXTPControlPopup::CreateControlPopup( xtpControlPopup );
		pPopup->SetFlags( xtpFlagManualUpdate );
		lpCreateControl->pControl = pPopup;
		CXTPPopupBar* pToolsMenuBar = CXTPPopupBar::CreatePopupBar( GetCommandBars() );
		pPopup->SetCommandBar( pToolsMenuBar );
		return TRUE;
	}

	if (lpCreateControl->nID == ID_FILE_MRU_FILE1)
	{
		CXTPControl* pControl = (CXTPControl*)CControlMRU::CreateObject();
		if (pControl)
		{
			pControl->SetFlags(xtpFlagManualUpdate);
			lpCreateControl->pControl = pControl;
			return TRUE;
		}
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
int CMainFrame::OnCreateCommandBar(LPCREATEBARSTRUCT lpCreatePopup)
{
	return FALSE;
}


//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnInitCommandsPopup( CXTPPopupBar* pPopupBar )
{
	if ( pPopupBar == NULL )
	{
		return;
	}

	CXTPControlPopup* pControlPopup = pPopupBar->GetControlPopup();
	if ( pControlPopup == NULL )
	{
		return;
	}

	int controlId = pControlPopup->GetID();
	if ( controlId == ID_TOOLS_TOOL1 )
	{
		UpdateToolsMenu();
	}
	else if ( controlId == ID_VIEW_OPENVIEWPANE )
	{
		UpdateViewPaneMenu();
	}
	else if ( controlId == ID_VIEW_LAYOUTS )
	{
		UpdateViewLayoutsPaneMenu( pControlPopup );
	}
	else if ( ID_VIEW_LAYOUT_FIRST <= controlId && controlId <= ID_VIEW_LAYOUT_LAST )
	{
		int layoutId = controlId - ID_VIEW_LAYOUT_FIRST;
		UpdateViewLayoutsPaneSubMenu( pControlPopup, layoutId );
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnIdleUpdate:
		IdleUpdate();
		break;
	case eNotify_OnInvalidateControls:
		InvalidateControls();
	}
}

//////////////////////////////////////////////////////////////////////////
CWnd* CMainFrame::FindPage( const char *sPaneClassName )
{
	IClassDesc* pClassDesc = GetIEditor()->GetClassFactory()->FindClass(sPaneClassName);
	if (!pClassDesc)
		return 0;

	IViewPaneClass* pViewPaneClass = NULL;
	if (FAILED(pClassDesc->QueryInterface( __uuidof(IViewPaneClass),(void**)&pViewPaneClass )))
		return 0;

	SViewPaneDesc* pPaneDesc = FindPaneByClass( pViewPaneClass );
	if (!pPaneDesc)
		return 0;

	// Pane with this class is created.

	CXTPDockingPane* pDockPane = GetDockingPaneManager()->FindPane(pPaneDesc->m_paneId);
	if (!pDockPane)
		return 0;

	return pPaneDesc->m_pViewWnd;
}
//////////////////////////////////////////////////////////////////////////
bool  CMainFrame::ClosePage( const char *sPaneClassName )
{
	PanesMap::iterator		itCurrentPane;
	PanesMap::iterator		itPreviousPane;
	bool									boResult(false);

	for (itCurrentPane=m_panesMap.begin();itCurrentPane!=m_panesMap.end();)
	{
		SViewPaneDesc &pn = itCurrentPane->second;
		if (pn.m_category.Compare(sPaneClassName)==0)
		{
			SPaneHistory						paneHistory;
			CXTPDockingPaneManager	*pDockingPaneManager=GetDockingPaneManager();
			CXTPDockingPane					*pDockPane = pDockingPaneManager->FindPane(pn.m_paneId);

			paneHistory.rect = pDockPane->GetPaneWindowRect();
			paneHistory.dockDir = pDockingPaneManager->GetPaneDirection(pDockPane);
			m_panesHistoryMap[pDockPane->GetTitle()] = paneHistory;

			pn.m_pViewWnd->DestroyWindow();
			pn.m_pViewWnd = 0;
			pDockPane->Detach();

			pDockingPaneManager->DestroyPane(pn.m_paneId);

			itPreviousPane=itCurrentPane;
			itCurrentPane++;

			m_panesMap.erase(itPreviousPane);


			boResult=true;
			continue;
		}

		itCurrentPane++;
	}

	return boResult;
}
//////////////////////////////////////////////////////////////////////////
bool	CMainFrame::IsPageDocked(const char *sPaneClassName)
{
	PanesMap::iterator		itCurrentPane;

	for (itCurrentPane=m_panesMap.begin();itCurrentPane!=m_panesMap.end();itCurrentPane++)
	{
		SViewPaneDesc &pn = itCurrentPane->second;
		if (pn.m_category.Compare(sPaneClassName)==0)
		{
			CXTPDockingPaneManager	*pDockingPaneManager=GetDockingPaneManager();
			CXTPDockingPane					*pDockPane = pDockingPaneManager->FindPane(pn.m_paneId);

			return (!pDockPane->IsFloating());
		}
	}

	return false;
}
//////////////////////////////////////////////////////////////////////////
CWnd* CMainFrame::OpenPage( const char *sPaneClassName,bool bReuseOpened )
{
	IClassDesc *pClassDesc = GetIEditor()->GetClassFactory()->FindClass(sPaneClassName);
	if (!pClassDesc)
		return 0;

	IViewPaneClass *pViewPaneClass = NULL;
	if (FAILED(pClassDesc->QueryInterface( __uuidof(IViewPaneClass),(void**)&pViewPaneClass )))
		return 0;

	// Check if view view pane class support only 1 pane at a time.
	if (pViewPaneClass->SinglePane() || bReuseOpened)
	{
		SViewPaneDesc* pPaneDesc = FindPaneByClass( pViewPaneClass );
		if (pPaneDesc)
		{
			// Pane with this class already created.

			// Only set focus to it.
			CXTPDockingPane *pDockPane = GetDockingPaneManager()->FindPane(pPaneDesc->m_paneId);
			if (pDockPane)
			{
				/*
				CRect rc = pDockPane->GetWindowRect();
				if (rc.IsRectEmpty())
				{
					CRect paneRect = pViewPaneClass->GetPaneRect();
					//GetDockingPaneManager()->FloatPane( pDockPane,paneRect );
					GetDockingPaneManager()->ShowPane( pDockPane );
				}
				*/
				GetDockingPaneManager()->ShowPane( pDockPane );
				pDockPane->SetFocus();
				return pPaneDesc->m_pViewWnd;
			}
			else
			{
				for (PanesMap::iterator it = m_panesMap.begin(); it != m_panesMap.end(); ++it)
				{
					if (pViewPaneClass == it->second.m_pViewClass)
					{
						m_panesMap.erase(it);
						break;
					}
				}
			}
		}
	}

	CXTPDockingPane *pDockPane = 0;

	if (pViewPaneClass->SinglePane() || bReuseOpened)
	{
		CXTPDockingPaneInfoList &panes = GetDockingPaneManager()->GetPaneList();
		POSITION pos = panes.GetHeadPosition();
		while (pos)
		{
			XTP_DOCKINGPANE_INFO &paneInfo = panes.GetNext(pos);
			if (paneInfo.pPane)
			{
				if (strcmp(paneInfo.pPane->GetTitle(),sPaneClassName) == 0)
				{
					GetDockingPaneManager()->ShowPane( paneInfo.pPane ); // This will show window and attach pane if needed.
					paneInfo.pPane->SetFocus();
					SViewPaneDesc* pPaneDesc = FindPaneByClass( pViewPaneClass );
					if (pPaneDesc)
						return pPaneDesc->m_pViewWnd;
					return 0;
				}
			}
		}
	}
	

	SViewPaneDesc pd;
	pd.m_pViewWnd = NULL;
	pd.m_pViewClass = pViewPaneClass;
	pd.m_pRuntimeClass = pViewPaneClass->GetRuntimeClass();
	pd.m_category = pViewPaneClass->Category();
	pd.m_bWantsIdleUpdate = pViewPaneClass->WantIdleUpdate();

	// Find free pane id.
	pd.m_paneId = 1;
	while (GetDockingPaneManager()->FindPane(pd.m_paneId) != NULL)
		pd.m_paneId++;

	assert(pd.m_pRuntimeClass);
	assert( pd.m_pRuntimeClass->IsDerivedFrom(RUNTIME_CLASS(CWnd)) || pd.m_pRuntimeClass == RUNTIME_CLASS(CWnd));
	if (!pd.m_pRuntimeClass)
		return 0;
	if (!(pd.m_pRuntimeClass->IsDerivedFrom(RUNTIME_CLASS(CWnd)) || pd.m_pRuntimeClass == RUNTIME_CLASS(CWnd)))
		return 0;

	bool bFloat = false;

//Diesel cut
	//XTPDockingPaneDirection dockDir = dockTopOf;
//Diesel new	
	XTPDockingPaneDirection dockDir = xtpPaneDockTop;
	
	
	switch (pViewPaneClass->GetDockingDirection())
	{
	case IViewPaneClass::DOCK_TOP:
		
//Diesel cut		
		//dockDir = dockTopOf;
//Diesel new		
		dockDir = xtpPaneDockTop;
		
		
		break;
	case IViewPaneClass::DOCK_BOTTOM:
		
//Diesel cut		
		//dockDir = dockBottomOf;
//Diesel new		
		dockDir = xtpPaneDockBottom;
		
		
		break;
	case IViewPaneClass::DOCK_LEFT:
		
//Diesel cut		
		//dockDir = dockLeftOf;
//Diesel new		
		dockDir = xtpPaneDockLeft;
		
		break;
	case IViewPaneClass::DOCK_RIGHT:
		
//Diesel cut
		//dockDir = dockRightOf;
//Diesel new		
		dockDir = xtpPaneDockRight;
		
		
		break;
	case IViewPaneClass::DOCK_FLOAT:
		bFloat = true;
		break;
	}

	CXTPDockingPane *pNextToPane = 0;
	SViewPaneDesc* pSimilarPaneDesc = FindPaneByCategory(pd.m_category);
	if (pSimilarPaneDesc)
	{
		pNextToPane = GetDockingPaneManager()->FindPane(pSimilarPaneDesc->m_paneId);
	}

	CString paneTitle = pViewPaneClass->GetPaneTitle();
	CRect paneRect = pViewPaneClass->GetPaneRect();
	if (m_panesHistoryMap.find(paneTitle) != m_panesHistoryMap.end())
	{
		const SPaneHistory &paneHistory = m_panesHistoryMap.find(paneTitle)->second;
		paneRect = paneHistory.rect;
		dockDir = paneHistory.dockDir;
	}

	CRect maxRc;
	maxRc.left = GetSystemMetrics( SM_XVIRTUALSCREEN );
	maxRc.top = GetSystemMetrics( SM_YVIRTUALSCREEN );
	maxRc.right = maxRc.left + GetSystemMetrics( SM_CXVIRTUALSCREEN );
	maxRc.bottom = maxRc.top + GetSystemMetrics( SM_CYVIRTUALSCREEN );

	// Clip to virtual desktop.
	paneRect.IntersectRect( paneRect,maxRc );

	// Ensure it is at least 10x10
  if (paneRect.Width() < 10)
		paneRect.right = paneRect.left + 10;
	if (paneRect.Height() < 10)
		paneRect.bottom = paneRect.top + 10;

	// Add pane description to map (we do it before creating the window object to avoid recursive window creation error)
	m_panesMap[pd.m_paneId] = pd;

	//------------------------------------------------------------------------------
	// Bugfix: 1. "Error Report" and "Character Editor" have the same id, closing Error Report will
	//         close all panels in Character Editor.
	//         2. Panels do n't extend to the full page of "Character Editor" 
	pDockPane = GetDockingPaneManager()->CreatePane( pd.m_paneId,paneRect,dockDir,pNextToPane );

	pd.m_pViewWnd = (CWnd*)pd.m_pRuntimeClass->CreateObject();
	assert( pd.m_pViewWnd );

	// If the window is not created successfully, should delete the panel as well
	if(!pd.m_pViewWnd)
		GetDockingPaneManager()->DestroyPane(pd.m_paneId);

	// Now update pane description with correct view window pointer
	m_panesMap[pd.m_paneId].m_pViewWnd = pd.m_pViewWnd;

	pDockPane->RedrawPane();
	if (!pDockPane)
		return 0;
	XTPDockingPaneType type = pDockPane->GetType();
	if (bFloat)
		GetDockingPaneManager()->FloatPane( pDockPane,paneRect );

	pDockPane->SetTitle( pViewPaneClass->GetPaneTitle() );
	pDockPane->SetMinTrackSize( pViewPaneClass->GetMinSize() );
	pDockPane->SetPaneData( 2 ); 



	return pd.m_pViewWnd;
}

//////////////////////////////////////////////////////////////////////////
bool CMainFrame::AttachToPane( CXTPDockingPane *pDockPane )
{
	CString title = pDockPane->GetTitle();
	if (title.IsEmpty())
		return false;

	CString sPaneClassName = title;

	IClassDesc *pClassDesc = GetIEditor()->GetClassFactory()->FindClass(sPaneClassName);
	if (!pClassDesc)
		return false;

	IViewPaneClass *pViewPaneClass = NULL;
	if (FAILED(pClassDesc->QueryInterface( __uuidof(IViewPaneClass),(void**)&pViewPaneClass )))
		return false;

	// Check if view view pane class support only 1 pane at a time.
	if (pViewPaneClass->SinglePane())
	{
		SViewPaneDesc* pPaneDesc = FindPaneByClass( pViewPaneClass );
		if (pPaneDesc)
		{
			// Pane with this class already created.
			// Only set focus to it.
			CXTPDockingPane *pDockPane = GetDockingPaneManager()->FindPane(pPaneDesc->m_paneId);
			if (pDockPane)
				pDockPane->SetFocus();
			return true;
		}
	}

	SViewPaneDesc pd;
	pd.m_pViewWnd = NULL;
	pd.m_pViewClass = pViewPaneClass;
	pd.m_pRuntimeClass = pViewPaneClass->GetRuntimeClass();
	pd.m_category = pViewPaneClass->Category();
	pd.m_bWantsIdleUpdate = pViewPaneClass->WantIdleUpdate();
	pd.m_paneId = pDockPane->GetID();

	assert(pd.m_pRuntimeClass);
	assert( pd.m_pRuntimeClass->IsDerivedFrom(RUNTIME_CLASS(CWnd)) || pd.m_pRuntimeClass == RUNTIME_CLASS(CWnd));
	if (!pd.m_pRuntimeClass)
		return false;
	if (!(pd.m_pRuntimeClass->IsDerivedFrom(RUNTIME_CLASS(CWnd)) || pd.m_pRuntimeClass == RUNTIME_CLASS(CWnd)))
		return false;

	pDockPane->SetTitle( pViewPaneClass->GetPaneTitle() );
	pDockPane->SetMinTrackSize( pViewPaneClass->GetMinSize() );

	CWnd *pWnd = (CWnd*)pd.m_pRuntimeClass->CreateObject();
	assert(pWnd);
	//pWnd->ShowWindow(SW_SHOW);
	pd.m_pViewWnd = pWnd;
	pDockPane->Attach(pd.m_pViewWnd);

	// Add pane description to map.
	m_panesMap[pd.m_paneId] = pd;

	return true;
}

//////////////////////////////////////////////////////////////////////////
CMainFrame::SViewPaneDesc* CMainFrame::FindPaneByCategory( const char *sPaneCategory )
{
	for (PanesMap::iterator it = m_panesMap.begin(); it != m_panesMap.end(); ++it)
	{
		if (stricmp(sPaneCategory,it->second.m_category) == 0)
			return &(it->second);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
CMainFrame::SViewPaneDesc* CMainFrame::FindPaneByClass( IViewPaneClass *pClass )
{
	for (PanesMap::iterator it = m_panesMap.begin(); it != m_panesMap.end(); ++it)
	{
		if (pClass == it->second.m_pViewClass)
			return &(it->second);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::RegisterStdViewClasses()
{




	CTrackViewDialog::RegisterViewClass();
	CDataBaseDialog::RegisterViewClass();
	CMaterialDialog::RegisterViewClass();
	CCharacterEditor::RegisterViewClass();
	CHyperGraphDialog::RegisterViewClass();
	CAnimationGraphDialog::RegisterViewClass();
	CAnimationGraphDialog2::RegisterViewClass();
	CVehicleEditorDialog::RegisterViewClass();
	CSmartObjectsEditorDialog::RegisterViewClass();
	CAIDebugger::RegisterViewClass();
	//CLightmapCompilerDialog::RegisterViewClass();
	CSelectObjectDlg::RegisterViewClass();
	CTimeOfDayDialog::RegisterViewClass();
	CFacialEditorDialog::RegisterViewClass();
	CDialogEditorDialog::RegisterViewClass();
	CVisualLogWnd::RegisterViewClass();






	CTextureBrowserDialog::RegisterViewClass();
	CAssetBrowserDialog::RegisterViewClass();
	CLNMEditorDialog::RegisterViewClass();
	CErrorReportDialog::RegisterViewClass();

	//////////////////////////////////////////////////////////////////////////
	// Register commands.
	//////////////////////////////////////////////////////////////////////////
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_MaterialEditor",functor(Command_Open_MaterialEditor) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_CharacterEditor",functor(Command_Open_CharacterEditor) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_DataBaseView",functor(Command_Open_DataBaseView) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_TrackView",functor(Command_Open_TrackView) );
	//GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_LightmapCompiler",functor(Command_Open_LightmapCompiler) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_SelectObjects",functor(Command_Open_SelectObjects) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_FlowGraph",functor(Command_Open_FlowGraph) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_FacialEditor",functor(Command_Open_FacialEditor) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_TimOfDay",functor(Command_Open_TimeOfDay) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_SmartObjectEditor",functor(Command_Open_SmartObjectsEditor) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_DialogSystemEditor",functor(Command_Open_DialogSystemEditor) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_VisualLogViewer",functor(Command_Open_VisualLogViewer) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_TerrainEditor",functor(Command_Open_TerrainEditor) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_TerrainTextureLayers",functor(Command_Open_TerrainTextureLayers) );






	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_TextureBrowser",functor(Command_Open_TextureBrowser) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_AssetBrowser",functor(Command_Open_AssetBrowser) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Open_LNMEditor",functor(Command_Open_LNMEditor) );

	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_MaterialEditor",functor(Command_Close_MaterialEditor) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_CharacterEditor",functor(Command_Close_CharacterEditor) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_DataBaseView",functor(Command_Close_DataBaseView) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_TrackView",functor(Command_Close_TrackView) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_LightmapCompiler",functor(Command_Close_LightmapCompiler) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_SelectObjects",functor(Command_Close_SelectObjects) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_FlowGraph",functor(Command_Close_FlowGraph) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_FacialEditor",functor(Command_Close_FacialEditor) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_TimOfDay",functor(Command_Close_TimeOfDay) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_SmartObjectEditor",functor(Command_Close_SmartObjectsEditor) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_DialogSystemEditor",functor(Command_Close_DialogSystemEditor) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_VisualLogViewer",functor(Command_Close_VisualLogViewer) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_TerrainEditor",functor(Command_Close_TerrainEditor) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_TerrainTextureLayers",functor(Command_Close_TerrainTextureLayers) );






	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_TextureBrowser",functor(Command_Close_TextureBrowser) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_AssetBrowser",functor(Command_Close_AssetBrowser) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_AnimActionEditor",functor(Command_Close_AnimActionEditor) );
	GetIEditor()->GetCommandManager()->RegisterCommand( "Editor.Close_LNMEditor",functor(Command_Close_LNMEditor) );
}

//////////////////////////////////////////////////////////////////////////
// Open commands.
//////////////////////////////////////////////////////////////////////////
void CMainFrame::Command_Open_MaterialEditor()  { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Material Editor" ); }
void CMainFrame::Command_Open_CharacterEditor() { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Character Editor" ); }
void CMainFrame::Command_Open_DataBaseView()    { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "DataBase View" ); }
void CMainFrame::Command_Open_TrackView()       { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Track View" ); }
//void CMainFrame::Command_Open_LightmapCompiler(){ ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Lightmap Compiler" ); }
void CMainFrame::Command_Open_SelectObjects()   { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Select Objects" ); }
void CMainFrame::Command_Open_FlowGraph()				{ ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Flow Graph" ); }
void CMainFrame::Command_Open_FacialEditor()		{ ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Facial Editor" ); }
void CMainFrame::Command_Open_TimeOfDay()       { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Time Of Day" ); }
void CMainFrame::Command_Open_SmartObjectsEditor() { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Smart Objects Editor" ); }
void CMainFrame::Command_Open_DialogSystemEditor() { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "DialogSystem Editor" ); }
void CMainFrame::Command_Open_VisualLogViewer() { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Visual Log Viewer" ); }
void CMainFrame::Command_Open_TerrainEditor() { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Terrain Editor" ); }
void CMainFrame::Command_Open_TerrainTextureLayers() { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Terrain Texture Layers" ); }



void CMainFrame::Command_Open_TextureBrowser() { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Texture Browser" ); }
void CMainFrame::Command_Open_AssetBrowser() { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "Asset Browser" ); }
void CMainFrame::Command_Open_LNMEditor() { ((CMainFrame*)AfxGetMainWnd())->OpenPage( "LNM Editor" ); }
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// Close commands.
//////////////////////////////////////////////////////////////////////////
void CMainFrame::Command_Close_MaterialEditor()  { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Material Editor" ); }
void CMainFrame::Command_Close_CharacterEditor() { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Character Editor" ); }
void CMainFrame::Command_Close_DataBaseView()    { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "DataBase View" ); }
void CMainFrame::Command_Close_TrackView()       { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Track View" ); }
void CMainFrame::Command_Close_LightmapCompiler(){ ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Lightmap Compiler" ); }
void CMainFrame::Command_Close_SelectObjects()   { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Select Objects" ); }
void CMainFrame::Command_Close_FlowGraph()				{ ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Flow Graph" ); }
void CMainFrame::Command_Close_FacialEditor()		{ ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Facial Editor" ); }
void CMainFrame::Command_Close_TimeOfDay()       { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Time Of Day" ); }
void CMainFrame::Command_Close_SmartObjectsEditor() { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Smart Objects Editor" ); }
void CMainFrame::Command_Close_DialogSystemEditor() { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "DialogSystem Editor" ); }
void CMainFrame::Command_Close_VisualLogViewer() { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Visual Log Viewer" ); }
void CMainFrame::Command_Close_TerrainEditor() { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Terrain Editor" ); }
void CMainFrame::Command_Close_TerrainTextureLayers() { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Terrain Texture Layers" ); }



void CMainFrame::Command_Close_TextureBrowser() { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Texture Browser" ); }
void CMainFrame::Command_Close_AssetBrowser() { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "Asset Browser" ); }
void CMainFrame::Command_Close_AnimActionEditor() { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "AnimAction Editor" ); }
void CMainFrame::Command_Close_LNMEditor() { ((CMainFrame*)AfxGetMainWnd())->ClosePage( "LNM Editor" ); }
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnSnapMenu( CPoint pos )
{
	CMenu menu;
	menu.CreatePopupMenu();

	float startSize = 0.125;
	int steps = 10;

	double size = startSize;
	for (int i = 0; i < steps; i++)
	{
		CString str;
		str.Format( "%g",size );
		menu.AppendMenu( MF_STRING,1+i,str );
		size *= 2;
	}
	menu.AppendMenu( MF_SEPARATOR );
	menu.AppendMenu( MF_STRING,100,"Setup grid" );

	int cmd = menu.TrackPopupMenu( TPM_RETURNCMD|TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_NONOTIFY,pos.x,pos.y,this );
	if (cmd >= 1 && cmd < 100)
	{
		size = startSize;
		for (int i = 0; i < cmd-1; i++)
		{
			size *= 2;
		}
		// Set grid to size.
		GetIEditor()->GetViewManager()->GetGrid()->size = size;
	}
	if (cmd == 100)
	{
		// Setup grid dialog.
		CGridSettingsDialog dlg;
		dlg.DoModal();
	}
}
//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnSkinChanged()
{
	OnSysColorChange();
	RedrawWindow(0, 0, RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE|RDW_ALLCHILDREN);		

	XTPPaintManager()->RefreshMetrics();

	GetCommandBars()->GetPaintManager()->RefreshMetrics();

	GetCommandBars()->RedrawCommandBars();	
}

//////////////////////////////////////////////////////////////////////////
///* //Diesel cut
void CMainFrame::SwitchTheme( XTPPaintTheme paintTheme,XTPDockingPanePaintTheme paneTheme )
{
		//pCommandBars->GetPaintManager()->m_bEnableAnimation = FALSE;
	if (paintTheme == xtpThemeOffice2007 || paintTheme == xtpThemeRibbon)
	{
		paintTheme = xtpThemeOffice2007;
/* //Diesel cut
		if (XTPOffice2007Images()->SetHandle( Path::Make(STYLES_PATH,"Office2007Blue.dll") ))
		//if (XTPOffice2007Images()->SetHandle( Path::Make(STYLES_PATH,"Office2007Silver.dll") ))
		{
			CXTPPaintManager::SetTheme(paintTheme);
			EnableOffice2007Frame(GetCommandBars());
			GetCommandBars()->GetPaintManager()->m_bEnableAnimation = TRUE;

			//if (gSettings.gui.bSkining)
				//XTPSkinManager()->LoadSkin( Path::Make(STYLES_PATH,"Vista.cjstyles"),"NormalSilver.ini" );
			//if (gSettings.gui.bSkining)
				//XTPSkinManager()->LoadSkin( Path::Make(STYLES_PATH,"Office2007.cjstyles"),"NormalBlue.ini" );
		}
		else
*/ //Diesel cut
			paintTheme = xtpThemeNativeWinXP;
	}
	else
	{
		if (s_themePaint == xtpThemeRibbon)
		{
			EnableOffice2007Frame(0);
		}

		CXTPPaintManager::SetTheme(paintTheme);
	}


	if (gSettings.gui.bSkining)
	{
		//CXTPPaintManager::SetTheme(xtpThemeNativeWinXP);
		//XTPSkinManager()->LoadSkin( Path::Make(STYLES_PATH,"CryDark.cjstyles") );

		//// Apply skin, including metrics, coloring and frame
		//XTPSkinManager()->SetApplyOptions(xtpSkinApplyMetrics|xtpSkinApplyColors|xtpSkinApplyFrame|xtpSkinApplyMenus);

		//// Apply skin to all new windows and threads

		//XTPSkinManager()->SetAutoApplyNewWindows(TRUE);

		//XTPSkinManager()->SetAutoApplyNewThreads(TRUE);

		//// Apply skin to current thread and window

		//XTPSkinManager()->EnableCurrentThread();

		//XTPSkinManager()->ApplyWindow(this->GetSafeHwnd());
		//// Redraw everything

		//XTPSkinManager()->RedrawAllControls();

		LoadCrySkin();

	}
	else
	{
		XTPSkinManager()->LoadSkin( "","" );
		XTPSkinManager()->SetApplyOptions( 0 );
	}
	s_themePaint = paintTheme;
	s_themePane = paneTheme;

	XTP_COMMANDBARS_ICONSINFO* pIconsInfo = XTPPaintManager()->GetIconsInfo();
	//pIconsInfo->bUseDisabledIcons = TRUE;

	if (GetDockingPaneManager())
	{
		GetDockingPaneManager()->SetTheme(paneTheme);
	}

	XTPPaintManager()->RefreshMetrics();
	GetCommandBars()->GetPaintManager()->RefreshMetrics();

	RecalcLayout(FALSE);
	if (GetCommandBars())
		GetCommandBars()->RedrawCommandBars();
	
	RedrawWindow( NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE|RDW_ALLCHILDREN );
	// Redraw twice so that owner draw buttons can change switch state.
	RedrawWindow( NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE|RDW_ALLCHILDREN );

	GetDockingPaneManager()->RedrawPanes();

	if (m_layoutWnd)
	{
		CWnd* pWnd = CWnd::FromHandle(*m_layoutWnd)->GetWindow(GW_CHILD);
		while(pWnd)
		{
			pWnd->RedrawWindow( NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE|RDW_ALLCHILDREN );
			pWnd = pWnd->GetWindow(GW_HWNDNEXT);
		}
	}

	OnSkinChanged();
}
//*/  //Diesel cut
//Diesel new
/*
void CMainFrame::SwitchTheme( XTPPaintTheme paintTheme,XTPDockingPanePaintTheme paneTheme )
{
	paintTheme = xtpThemeOffice2003;
	paneTheme = xtpPaneThemeWhidbey; // Force the same always.
	s_themePaint = paintTheme;
	s_themePane = paneTheme;

	if (gSettings.gui.bSkining)
	{
		LoadCrySkin();
	}
	else
	{
		XTPSkinManager()->LoadSkin( "","" );
		XTPSkinManager()->SetApplyOptions( 0 );
	}

		
	
	CXTPPaintManager::SetTheme(paintTheme);

	if (GetCommandBars())
	{
		GetCommandBars()->GetPaintManager()->m_bEnableAnimation = TRUE;
	}

	XTP_COMMANDBARS_ICONSINFO* pIconsInfo = XTPPaintManager()->GetIconsInfo();

	if (GetDockingPaneManager())
	{
		GetDockingPaneManager()->SetTheme(paneTheme);
	}

	XTPPaintManager()->RefreshMetrics();
	GetCommandBars()->GetPaintManager()->RefreshMetrics();

	RecalcLayout(FALSE);
	if (GetCommandBars())
		GetCommandBars()->RedrawCommandBars();
	
	RedrawWindow( NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE|RDW_ALLCHILDREN );
	// Redraw twice so that owner draw buttons can change switch state.
	RedrawWindow( NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE|RDW_ALLCHILDREN );

	GetDockingPaneManager()->RedrawPanes();

	if (m_layoutWnd)
	{
		CWnd* pWnd = CWnd::FromHandle(*m_layoutWnd)->GetWindow(GW_CHILD);
		while(pWnd)
		{
			pWnd->RedrawWindow( NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE|RDW_ALLCHILDREN );
			pWnd = pWnd->GetWindow(GW_HWNDNEXT);
		}
	}

	OnSkinChanged();
}
*/
//Diesel new

//Diesel 2 new
//void CMainFrame::SwitchTheme( XTPPaintTheme paintTheme,XTPDockingPanePaintTheme paneTheme )
//{
//	LoadCrySkin();
//	OnSkinChanged();
//}



//////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::OnSwitchTheme(UINT nID)
{
	switch (nID)
	{
	case ID_THEME_OFFICE2000:
		SwitchTheme(xtpThemeOffice2000,xtpPaneThemeOffice);
		break;
	case ID_THEME_OFFICEXP:
		SwitchTheme(xtpThemeOfficeXP,xtpPaneThemeOffice);
		break;
	case ID_THEME_OFFICE2003:
		SwitchTheme(xtpThemeOffice2003,xtpPaneThemeOffice2003);
		break;
	case ID_THEME_OFFICE2007:
		SwitchTheme(xtpThemeRibbon,xtpPaneThemeOffice2007);
		break;
	case ID_THEME_WINDOWSXP:
		SwitchTheme(xtpThemeNativeWinXP,xtpPaneThemeNativeWinXP);
		break;
	case ID_THEME_EXPLORER:
		SwitchTheme(xtpThemeNativeWinXP,xtpPaneThemeExplorer);
		break;
	case ID_THEME_VISIO:
		SwitchTheme(xtpThemeNativeWinXP,xtpPaneThemeVisio);
		break;
	case ID_THEME_VISUALSTUDIO2005:
		SwitchTheme(xtpThemeWhidbey,xtpPaneThemeVisualStudio2005);
		break;
	case ID_THEME_GRIPPERED:
		SwitchTheme(xtpThemeNativeWinXP,xtpPaneThemeGrippered);
		break;
	//case ID_THEME_SIMPLE:
		//SwitchTheme(xtpThemeNativeWinXP,xtpPaneThemeNativeWinXP);
		break;
	}
	return TRUE;
}

void CMainFrame::OnUpdateSkin(CCmdUI* pCmdUI)
{
	switch (pCmdUI->m_nID)
	{
	case ID_SKINS_CRYDARK:
		//pCmdUI->SetCheck(s_lpszSkinPath == "Editor\\Styles\\CryDark.cjstyles"); //Diesel cut
		break;
	case ID_SKINS_CRYLIGHT:
		//pCmdUI->SetCheck(s_lpszSkinPath == "Editor\\Styles\\CryLight.cjstyles"); //Diesel cut
		break;
	}
}

BOOL CMainFrame::OnSwitchSkin(UINT nID)
{
	switch (nID)
	{
	case ID_SKINS_CRYDARK:
		//s_lpszSkinPath = "Editor\\Styles\\CryDark.cjstyles"; //Diesel cut
		break;
	case ID_SKINS_CRYLIGHT:
		//s_lpszSkinPath = "Editor\\Styles\\CryLight.cjstyles"; //Diesel cut
		break;
	}
	SwitchTheme(s_themePaint,s_themePane);
	return TRUE;
}

void CMainFrame::OnUpdateTheme(CCmdUI* pCmdUI)
{
	switch (pCmdUI->m_nID)
	{
	case ID_THEME_OFFICE2000:
		pCmdUI->SetCheck(XTPPaintManager()->GetCurrentTheme() == xtpThemeOffice2000);
		break;
	case ID_THEME_OFFICEXP:
		pCmdUI->SetCheck(XTPPaintManager()->GetCurrentTheme() == xtpThemeOfficeXP);
		break;
	case ID_THEME_OFFICE2003:
		pCmdUI->SetCheck(s_themePane == xtpPaneThemeOffice2003);
		break;
	case ID_THEME_OFFICE2007:
		pCmdUI->SetCheck(s_themePane == xtpPaneThemeOffice2007);
		break;
	case ID_THEME_WINDOWSXP:
		pCmdUI->SetCheck(s_themePane == xtpPaneThemeNativeWinXP);
		break;
	case ID_THEME_EXPLORER:
		pCmdUI->SetCheck(s_themePane == xtpPaneThemeExplorer);
		break;
	case ID_THEME_VISIO:
		pCmdUI->SetCheck(s_themePane == xtpPaneThemeVisio);
		break;
	case ID_THEME_VISUALSTUDIO2005:
		pCmdUI->SetCheck(s_themePane == xtpPaneThemeVisualStudio2005);
		break;
	case ID_THEME_GRIPPERED:
		pCmdUI->SetCheck(s_themePane == xtpPaneThemeGrippered);
		break;
	//case ID_THEME_SIMPLE:
		//pCmdUI->SetCheck(s_themePane == xtpPaneThemeNativeWinXP);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnDockingHelpers()
{
	s_bDockingHelpers = (s_bDockingHelpers) ? FALSE:TRUE;
	GetDockingPaneManager()->SetAlphaDockingContext(s_bDockingHelpers);
	GetDockingPaneManager()->SetShowDockingContextStickers(s_bDockingHelpers);
}
//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateDockingHelpers(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck( s_bDockingHelpers == TRUE );
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnSkining()
{
	if (gSettings.gui.bSkining)
	{
		//XTPSkinManager()->LoadSkin( "" );
	}
	gSettings.gui.bSkining = !gSettings.gui.bSkining;
	SwitchTheme(s_themePaint,s_themePane);
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::LoadCrySkin()
{

	CXTPPaintManager::SetCustomTheme(new CCrytekTheme());

//Diesel cut	
	//BOOL bLoaded = XTPSkinManager()->LoadSkin( s_lpszSkinPath );

	// Apply skin, including metrics, coloring and frame
	XTPSkinManager()->SetApplyOptions(xtpSkinApplyMetrics|xtpSkinApplyColors|xtpSkinApplyFrame|xtpSkinApplyMenus);

	// Apply skin to all new windows and threads

	XTPSkinManager()->SetAutoApplyNewWindows(TRUE);

	XTPSkinManager()->SetAutoApplyNewThreads(TRUE);

	// Apply skin to current thread and window

	XTPSkinManager()->EnableCurrentThread();

	XTPSkinManager()->ApplyWindow(this->GetSafeHwnd());

	bool test = XTPSkinManager()->IsColorFilterExists();
	// Redraw everything



	XTPSkinManager()->RedrawAllControls();

	OnSkinChanged();
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateSkining( CCmdUI* pCmdUI )
{
	if (gSettings.gui.bSkining)
		pCmdUI->SetCheck(TRUE);
	else
		pCmdUI->SetCheck(FALSE);
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnMusicInfo()
{
	CMusicInfoDlg *pDlg = new CMusicInfoDlg;
	pDlg->Create(CMusicInfoDlg::IDD, this);
	pDlg->ShowWindow(SW_SHOW);
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnOpenSoundBrowser()
{
	CString file;
	CString startPath = Path::GetGameFolder()+"/Languages/dialog/";
	CFileUtil::SelectSingleFile( EFILE_TYPE_SOUND, file, "",startPath );
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnOpenDialogBrowser()
{
	CString file;
	CString startPath = Path::GetGameFolder()+"/Languages/dialog/";
	CFileUtil::SelectSingleFile( EFILE_TYPE_DIALOG, file, "",startPath );
}

//////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	// Prevent accelerators active inside ComboBox in toolbar.
	if ((pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
		&& (pMsg->wParam != VK_RETURN && pMsg->wParam != VK_TAB && pMsg->wParam != VK_ESCAPE))
	{
		CWnd* pWnd = CWnd::GetFocus();
		if (pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CXTPEdit)))
			return FALSE;
	}

	if (pMsg->wParam == VK_ESCAPE)
	{
		CWnd* pWnd = CWnd::GetFocus();
		if (pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CViewport)))
		{
			((CCryEditApp*)AfxGetApp())->OnEditEscape();
			return TRUE;
		}
	}

	/*
	if (pMsg->message == WM_INPUT)
	{
		static int d = 0;

		CWnd *pFocus = GetFocus();
		if (pFocus->IsKindOf( RUNTIME_CLASS(CViewport) ))
		{
			// We are in viewport.
			static int d = 0;
			CryLog( "WM_INPUT in viewport %d",d++ );
		}
	}
	*/

	return __super::PreTranslateMessage(pMsg);
}

//////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::DestroyWindow()
{
	if (m_autoSaveTimer)
		KillTimer( m_autoSaveTimer );
	if (m_autoRemindTimer)
		KillTimer( m_autoRemindTimer );
	if (m_networkAuditionTimer)
		KillTimer( m_networkAuditionTimer );

	SaveConfig();

	return __super::DestroyWindow();
}

void CMainFrame::LoadConfig()
{
	static CXTPDockingPaneLayout s_defaultLayout( &m_paneManager );
	if ( m_pDefaultLayout == NULL )
	{
		m_pDefaultLayout = &s_defaultLayout;
		m_paneManager.GetLayout( m_pDefaultLayout );
	}

	if (!IsPreview())
	{
		// Load the previous state for docking panes.
		CXTPDockingPaneLayout layoutNormal(&m_paneManager);
		if (layoutNormal.Load(_T("NormalLayout"))) 
		{
			if (layoutNormal.GetPaneList().GetCount() > 0)
			{
				m_paneManager.SetLayout(&layoutNormal);	
			}
		}
		// Load the previous state for toolbars and menus.
		LoadCommandBars(_T("CommandBars"),TRUE);

		XTPShortcutManager()->LoadShortcuts( "Shortcuts" );
		if(!XTPShortcutManager()->GetDefaultAccelerator())
			XTPShortcutManager()->Reset();

		// Kill original accelerator table.
		if (m_hAccelTable)
			DestroyAcceleratorTable( m_hAccelTable );
		m_hAccelTable = NULL;
		///

		m_panesHistoryMap.clear();
		CXTRegistryManager regMgr;
		int nIndex = 0;
		while (true)
		{
			CString idstr;
			idstr.Format("%d",nIndex++);
			CString title = regMgr.GetProfileString( _T("DockingPanesHistory"),idstr+"_Title","" );
			if (title.IsEmpty())
				break;
			CString datastr = regMgr.GetProfileString( _T("DockingPanesHistory"),idstr+"_Position","" );
			
			int x1,y1,x2,y2,dockDir;
			sscanf( datastr,"%d,%d,%d,%d,%d",&x1,&y1,&x2,&y2,&dockDir );
			if (x1 < -1000 || x1 > 10000)
				continue;
			SPaneHistory paneHistory;
			paneHistory.rect = CRect(x1,y1,x2,y2);
			paneHistory.dockDir = (XTPDockingPaneDirection)dockDir;
			m_panesHistoryMap[title] = paneHistory;
		}

		/*
		{
			CXTPDockingPaneInfoList &panes = GetDockingPaneManager()->GetPaneList();
			POSITION pos = panes.GetHeadPosition();
			while (pos)
			{
				XTP_DOCKINGPANE_INFO &paneInfo = panes.GetNext(pos);
				if (paneInfo.pPane)
				{
					if (paneInfo.pLastHolder != 0)
					{
						CRect rc = paneInfo.pLastHolder->GetPaneWindowRect();
						if (rc.IsRectNull())
							AttachToPane( paneInfo.pPane );
					}
				}
			}
		}
		*/
	}
	else
	{
		// Load the previous state for docking panes.
		CXTPDockingPaneLayout layoutNormal(&m_paneManager);
		if (layoutNormal.Load(_T("PreviewLayout"))) 
		{
			if (layoutNormal.GetPaneList().GetCount() > 0)
			{
				m_paneManager.SetLayout(&layoutNormal);	
			}
		}
		// Load the previous state for toolbars and menus.
		//LoadCommandBars(_T("CommandBars"));

		m_panesHistoryMap.clear();
		CXTRegistryManager regMgr;
		int nIndex = 0;
		while (true)
		{
			CString idstr;
			idstr.Format("%d",nIndex++);
			CString title = regMgr.GetProfileString( _T("DockingPanesHistory"),idstr+"_Title","" );
			if (title.IsEmpty())
				break;
			CString datastr = regMgr.GetProfileString( _T("DockingPanesHistory"),idstr+"_Position","" );

			int x1,y1,x2,y2,dockDir;
			sscanf( datastr,"%d,%d,%d,%d,%d",&x1,&y1,&x2,&y2,&dockDir );
			if (x1 < -1000 || x1 > 10000)
				continue;
			SPaneHistory paneHistory;
			paneHistory.rect = CRect(x1,y1,x2,y2);
			paneHistory.dockDir = (XTPDockingPaneDirection)dockDir;
			m_panesHistoryMap[title] = paneHistory;
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void CMainFrame::EnableMenu( bool bEnable )
{
	CXTPMenuBar *pMenuBar = GetCommandBars()->GetMenuBar();

	pMenuBar->EnableWindow(bEnable?TRUE:FALSE);

}
//////////////////////////////////////////////////////////////////////////
void CMainFrame::SaveConfig()
{
	// Save global settings.
	gSettings.Save();

	// Restore accelerators.
	if (!m_bAcceleratorsEnabled)
		EnableAccelerator( true );

	// Save frame window size and position.
	m_wndPosition.SaveWindowPos(this);

	CXTRegistryManager regMgr;
	//regMgr.WriteProfileInt(_T("Settings"), _T("bXPMode"), xtAfxData.bXPMode);
	regMgr.WriteProfileInt(_T("Settings"), _T("PaintTheme"), s_themePaint);
	regMgr.WriteProfileInt(_T("Settings"), _T("PaneTheme"), s_themePane);
	regMgr.WriteProfileInt(_T("Settings"), _T("DockingHelpers"), s_bDockingHelpers );
	regMgr.WriteProfileInt(_T("Settings"), _T("Skining"), gSettings.gui.bSkining );

//Diesel cut
	//regMgr.WriteProfileString(_T("Settings"), _T("SkinPath"),s_lpszSkinPath);

	if (!IsPreview())
	{
		// Save control bar postion.
		SaveCommandBars(_T("CommandBars"));
		XTPShortcutManager()->SaveShortcuts( "Shortcuts" );
	}

	CXTPDockingPaneLayout layoutNormal(&m_paneManager);
	m_paneManager.GetLayout(&layoutNormal);	

	if (!IsPreview())
		layoutNormal.Save(_T("NormalLayout"));
	else
		layoutNormal.Save(_T("PreviewLayout"));

	int nIndex = 0;
	for (PanesHistoryMap::iterator it = m_panesHistoryMap.begin(); it != m_panesHistoryMap.end(); ++it)
	{
		const SPaneHistory &paneHistory = it->second;
		CString datastr;
		datastr.Format( "%d,%d,%d,%d,%d",paneHistory.rect.left,paneHistory.rect.top,paneHistory.rect.right,paneHistory.rect.bottom,paneHistory.dockDir );
		CString idstr;
		idstr.Format("%d",nIndex++);
		regMgr.WriteProfileString(_T("DockingPanesHistory"), idstr+"_Title", (const char*)it->first );
		regMgr.WriteProfileString(_T("DockingPanesHistory"), idstr+"_Position", datastr );
	}

	if (m_layoutWnd)
		m_layoutWnd->SaveConfig();

	GetIEditor()->GetExternalToolsManager()->Save();
}

//////////////////////////////////////////////////////////////////////////
// WARNING uses undocumented MFC code!!!
//////////////////////////////////////////////////////////////////////////
void CMainFrame::DockControlBarNextTo(CControlBar* pBar,
                                      CControlBar* pTargetBar)
{
    ASSERT(pBar != NULL);
    ASSERT(pTargetBar != NULL);
    ASSERT(pBar != pTargetBar);

    // the neighbour must be already docked
    CDockBar* pDockBar = pTargetBar->m_pDockBar;
    ASSERT(pDockBar != NULL);
    UINT nDockBarID = pTargetBar->m_pDockBar->GetDlgCtrlID();
    ASSERT(nDockBarID != AFX_IDW_DOCKBAR_FLOAT);

    bool bHorz = (nDockBarID == AFX_IDW_DOCKBAR_TOP ||
        nDockBarID == AFX_IDW_DOCKBAR_BOTTOM);

    // dock normally (inserts a new row)
    DockControlBar(pBar, nDockBarID);

    // delete the new row (the bar pointer and the row end mark)
    pDockBar->m_arrBars.RemoveAt(pDockBar->m_arrBars.GetSize() - 1);
    pDockBar->m_arrBars.RemoveAt(pDockBar->m_arrBars.GetSize() - 1);

    // find the target bar
    for (int i = 0; i < pDockBar->m_arrBars.GetSize(); i++)
    {
        void* p = pDockBar->m_arrBars[i];
        if (p == pTargetBar) // and insert the new bar after it
            pDockBar->m_arrBars.InsertAt(i + 1, pBar);
    }

    // move the new bar into position
    CRect rBar;
    pTargetBar->GetWindowRect(rBar);
    rBar.OffsetRect(bHorz ? 1 : 0, bHorz ? 0 : 1);
    pBar->MoveWindow(rBar);
}

//////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::VerifyBarState( CDockState &state )
{
	for (int i = 0; i < state.m_arrBarInfo.GetSize(); i++)
	{
		CControlBarInfo* pInfo = (CControlBarInfo*)state.m_arrBarInfo[i];
		ASSERT(pInfo != NULL);
		int nDockedCount = pInfo->m_arrBarID.GetSize();
		if (nDockedCount > 0)
		{
			// dockbar
			for (int j = 0; j < nDockedCount; j++)
			{
				UINT_PTR nID = (UINT_PTR) pInfo->m_arrBarID[j];
				if (nID == 0) continue; // row separator
				if (nID > 0xFFFF)
					nID &= 0xFFFF; // placeholder - get the ID
				if (GetControlBar(nID) == NULL)
					return FALSE;
			}
		}

		if (!pInfo->m_bFloating) // floating dockbars can be created later
			if (GetControlBar(pInfo->m_nBarID) == NULL)
				return FALSE; // invalid bar ID
	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::CreateRollupBar()
{
//Diesel cut	
	//CXTPDockingPane* pDockRollup = GetDockingPaneManager()->CreatePane( IDW_VIEW_ROLLUP_BAR,CRect(0,0,220,500),dockRightOf);
//Diesel new	
	CXTPDockingPane* pDockRollup = GetDockingPaneManager()->CreatePane( IDW_VIEW_ROLLUP_BAR,CRect(0,0,220,500),xtpPaneDockRight);
	

	pDockRollup->SetTitle( _T("RollupBar") );

	m_wndRollUp.Create( NULL,NULL,WS_CHILD|WS_VISIBLE,CRect(0,0,1,1),this,0 );

	//////////////////////////////////////////////////////////////////////////
	// Create our RollupCtrl into DialogBar and register it
	m_objectRollupCtrl.Create(WS_VISIBLE | WS_CHILD, CRect(4, 4, 187, 362), &m_wndRollUp, NULL);
	m_wndRollUp.SetRollUpCtrl( ROLLUP_OBJECTS,&m_objectRollupCtrl,"Create" );

	if (!IsPreview())
	{
		m_terrainRollupCtrl.Create(WS_VISIBLE | WS_CHILD, CRect(4, 4, 187, 362), &m_wndRollUp, NULL);
		m_wndRollUp.SetRollUpCtrl( ROLLUP_TERRAIN,&m_terrainRollupCtrl,"Terrain Editing" );

		//m_modellingRollupCtrl.Create(WS_VISIBLE | WS_CHILD, CRect(4, 4, 187, 362), &m_wndRollUp, NULL);
		//m_wndRollUp.SetRollUpCtrl( ROLLUP_MODELLING,&m_modellingRollupCtrl,"Modelling" );

		m_displayRollupCtrl.Create(WS_VISIBLE | WS_CHILD, CRect(4, 4, 187, 362), &m_wndRollUp, NULL);
		m_wndRollUp.SetRollUpCtrl( ROLLUP_DISPLAY,&m_displayRollupCtrl,"Display Settings" );

		m_layersRollupCtrl.Create(WS_VISIBLE | WS_CHILD, CRect(4, 4, 187, 362), &m_wndRollUp, NULL);
		m_wndRollUp.SetRollUpCtrl( ROLLUP_LAYERS,&m_layersRollupCtrl,"Layers" );

		//////////////////////////////////////////////////////////////////////////
		// Insert the main rollup
		m_mainTools = new CMainTools;
		m_mainTools->Create(MAKEINTRESOURCE(CMainTools::IDD),this);
		m_objectRollupCtrl.InsertPage("Objects",m_mainTools);

		//	m_objectRollupCtrl.InsertPage("Main Tools",MAKEINTRESOURCE(CMainTools::IDD),RUNTIME_CLASS(CMainTools) );

		// Modelling panel disabled
		//CModellingToolsPanel *pModellingToolsPanel = new CModellingToolsPanel;
		//pModellingToolsPanel->Create(MAKEINTRESOURCE(CModellingToolsPanel::IDD),this);
		//// Modelling panel disabled
		////m_modellingRollupCtrl.InsertPage("Modelling",pModellingToolsPanel);
		//pModellingToolsPanel->InitDefaultModellingPanels();

		m_terrainPanel = new CTerrainPanel(this);
		m_terrainRollupCtrl.InsertPage("Terrain",m_terrainPanel );

		CPanelDisplayHide *hidePanel = new CPanelDisplayHide(this);
		m_displayRollupCtrl.InsertPage("Hide by Category",hidePanel );

		CPanelDisplayRender *renderPanel = new CPanelDisplayRender(this);
		m_displayRollupCtrl.InsertPage("Render Settings",renderPanel );

		CPanelDisplayLayer *layerPanel = new CPanelDisplayLayer(this);
		m_layersRollupCtrl.InsertPage( "Layers Settings",layerPanel );

		m_objectRollupCtrl.ExpandAllPages( TRUE );
		m_terrainRollupCtrl.ExpandAllPages(TRUE);
		// Modelling panel disabled
		//m_modellingRollupCtrl.ExpandAllPages(TRUE);
		m_displayRollupCtrl.ExpandAllPages(TRUE);
		m_layersRollupCtrl.ExpandAllPages(TRUE);

		SelectRollUpBar( ROLLUP_OBJECTS );
	}
	else
	{
		// Hide all menus.
	}
	m_wndRollUp.Select(ROLLUP_OBJECTS);
}

//////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// Use our own window class (Needed to detect single application instance).
	cs.lpszClass = _T("CryEditorClass");

	//WNDCLASS wndcls; //Diesel cut
	WNDCLASSEX wndcls; //Diesel new

//Diesel cut
	//BOOL bResult = AfxCtxGetClassInfo( AfxGetInstanceHandle(), cs.lpszClass, &wndcls);
//Diesel new
	BOOL bResult = GetClassInfoEx( AfxGetInstanceHandle(), cs.lpszClass, &wndcls);

	// Init the window with the lowest possible resolution
	RECT rc;
	SystemParametersInfo( SPI_GETWORKAREA,0,&rc,0 );
	cs.x = rc.left;
	cs.y = rc.top;
	cs.cx = rc.right - rc.left;
	cs.cy = rc.bottom - rc.top;

	if( !__super::PreCreateWindow(cs) )
		return FALSE;

	cs.dwExStyle = 0;

	//cs.lpszClass = AfxRegisterWndClass( 0, NULL, NULL,AfxGetApp()->LoadIcon(IDR_MAINFRAME));
	//cs.style &= ~FWS_ADDTOTITLE;

	return TRUE;
}

void CMainFrame::IdleUpdate()
{
}

void CMainFrame::DockControlBarLeftOf(CControlBar *Bar, CControlBar *LeftOf)
{
	////////////////////////////////////////////////////////////////////////
	// Dock a control bar left of another
	////////////////////////////////////////////////////////////////////////

	CRect rect;
	DWORD dw;
	UINT n;

	// Get MFC to adjust the dimensions of all docked ToolBars
	// so that GetWindowRect will be accurate
	RecalcLayout(TRUE);

	LeftOf->GetWindowRect(&rect);
	rect.OffsetRect(1, 0);
	dw = LeftOf->GetBarStyle();
	n = 0;
	n = (dw & CBRS_ALIGN_TOP) ? AFX_IDW_DOCKBAR_TOP : n;
	n = (dw & CBRS_ALIGN_BOTTOM && n == 0) ? AFX_IDW_DOCKBAR_BOTTOM : n;
	n = (dw & CBRS_ALIGN_LEFT && n == 0) ? AFX_IDW_DOCKBAR_LEFT : n;
	n = (dw & CBRS_ALIGN_RIGHT && n == 0) ? AFX_IDW_DOCKBAR_RIGHT : n;

	// When we take the default parameters on rect, DockControlBar will dock
	// each Toolbar on a seperate line. By calculating a rectangle, we
	// are simulating a Toolbar being dragged to that location and docked.
	DockControlBar(Bar, n, &rect);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	__super::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	__super::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
}

//////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::OnToggleBar( UINT nID )
{
	CXTPDockingPane *pBar = GetDockingPaneManager()->FindPane(nID);
	if (pBar)
	{
		if (pBar->IsClosed())
			GetDockingPaneManager()->ShowPane(pBar);
		else
			GetDockingPaneManager()->ClosePane(pBar);
		return TRUE;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateControlBar(CCmdUI* pCmdUI)
{
	CXTPCommandBars* pCommandBars = GetCommandBars();
	if(pCommandBars != NULL)
	{
		CXTPToolBar *pBar = pCommandBars->GetToolBar(pCmdUI->m_nID);
		if (pBar)
		{
			pCmdUI->SetCheck( (pBar->IsVisible()) ? 1 : 0 );
			return;
		}
	}
	CXTPDockingPane *pBar = GetDockingPaneManager()->FindPane(pCmdUI->m_nID);
	if (pBar)
	{
		pCmdUI->SetCheck( (!pBar->IsClosed()) ? 1 : 0 );
		return;
	}
	pCmdUI->SetCheck(0);
}

//////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	// TODO: Add your specialized code here and/or call the base class
	//m_layoutWnd.Create( this,2,2,CSize(10,10),pContext );
	m_layoutWnd = new CLayoutWnd;
	CRect rc(0,0,1,1);
	m_layoutWnd->CreateEx( 0,NULL,NULL,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,rc,this,AFX_IDW_PANE_FIRST );
	if (IsPreview())
	{
		m_layoutWnd->CreateLayout( ET_Layout0,true,ET_ViewportModel );
	}
	else
	{
		if (!m_layoutWnd->LoadConfig())
			m_layoutWnd->CreateLayout( ET_Layout0 );
	}

	return TRUE;
	//return __super::OnCreateClient(lpcs, pContext);
}

//////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	// TODO: Add your message handler code here and/or call default
	if (pCopyDataStruct->dwData == 100 && pCopyDataStruct->lpData != NULL)
	{
		char str[1024];
		memcpy( str,pCopyDataStruct->lpData,pCopyDataStruct->cbData );
		str[pCopyDataStruct->cbData] = 0;

		// Load this file.
		((CCryEditApp*)AfxGetApp())->LoadFile( str );
	}

	return __super::OnCopyData(pWnd, pCopyDataStruct);
}

//////////////////////////////////////////////////////////////////////////
CString CMainFrame::GetSelectionName()
{
	return m_selectionName;
}
//////////////////////////////////////////////////////////////////////////
int CMainFrame::SelectRollUpBar( int rollupBarId )
{
	if (m_wndRollUp.m_hWnd)
		m_wndRollUp.Select( rollupBarId );
	return rollupBarId;
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnClose()
{
	if (!GetIEditor()->GetDocument()->CanCloseFrame(this))
		return;
	GetIEditor()->GetDocument()->SetModifiedFlag(FALSE);
	GetIEditor()->GetDocument()->SetModifiedModules(eModifiedNothing);

	// Close all edit panels.
	GetIEditor()->ClearSelection();
	GetIEditor()->SetEditTool(0);
	GetIEditor()->GetObjectManager()->EndEditParams();

	SaveConfig();

	//__super::OnClose();
	AfxPostQuitMessage(0);
}

//////////////////////////////////////////////////////////////////////////
CRollupCtrl* CMainFrame::GetRollUpControl( int rollupBarId )
{
	if (m_hWnd == 0)
		return 0;
	if (rollupBarId == ROLLUP_OBJECTS)
	{
		return &m_objectRollupCtrl;
	} else if (rollupBarId == ROLLUP_TERRAIN)
	{
		return &m_terrainRollupCtrl;
	} else if (rollupBarId == ROLLUP_MODELLING)
	{
		// Modelling panel disabled
		// return &m_modellingRollupCtrl;
	} else if (rollupBarId == ROLLUP_DISPLAY)
	{
		return &m_displayRollupCtrl;
	} else if (rollupBarId == ROLLUP_LAYERS)
	{
		return &m_layersRollupCtrl;
	}
	// Default.
	return &m_objectRollupCtrl;
}
/////////////////////////////////////////////////////////////////////////
void CMainFrame::SetStatusText( LPCTSTR pszText)
{
	if (m_wndStatusBar.m_hWnd)
		m_wndStatusBar.SetPaneText(0, pszText);
};
//////////////////////////////////////////////////////////////////////////
void CMainFrame::SetItemStatusText(int nItemIdentifier,LPCTSTR pszText )
{
	if (!m_wndStatusBar.m_hWnd)
	{
		return;
	}

	int nIndex(-1);
	nIndex = m_wndStatusBar.CommandToIndex(nItemIdentifier);
	if((nIndex<0)||(nIndex>=m_wndStatusBar.GetPaneCount()))
	{
		assert("CMainFrame::SetItemStatusText"&&false);
		return;
	}
	m_wndStatusBar.SetPaneText(nIndex,pszText);
}
//////////////////////////////////////////////////////////////////////////
void CMainFrame::ActivateFrame(int nCmdShow)
{
	__super::ActivateFrame(nCmdShow);
}

void CMainFrame::ShowWindowEx(int nCmdShow)
{
	// Restore frame window size and position.
 	if (!m_wndPosition.LoadWindowPos(this))
	{
		nCmdShow = m_wndPosition.showCmd;
	}
	LoadConfig();

	__super::ShowWindow(nCmdShow);
}

//////////////////////////////////////////////////////////////////////////
bool CMainFrame::IsPreview() const
{
	return GetIEditor()->IsInPreviewMode();
}

//////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext)
{
	if (!__super::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
		return FALSE;

	/*
	// Initialize accelerator key manager.
	//accelManager.Init(this, IDR_MAINFRAME, _T("Main Frame"), _T("MainFrameKeys"));
	// initialize accelerator manager.
	if (!InitAccelManager() )
	{
		return FALSE;
	}
	*/

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::EnableAccelerator( bool bEnable )
{
	m_bAcceleratorsEnabled = bEnable;
	if (bEnable)
	{
		if (m_hAccelTable)
			DestroyAcceleratorTable( m_hAccelTable );
		m_hAccelTable = NULL;

		XTPShortcutManager()->DisableShortcuts(FALSE);
		XTPShortcutManager()->LoadShortcuts( "Shortcuts" );
		//CXTAccelManager &accelManager = CXTAccelManager::Get();
		//LoadAccelTable( MAKEINTRESOURCE(IDR_MAINFRAME) );
		//accelManager.UpdateWindowAccelerator();
		CLogFile::WriteLine( "Enable Accelerators" );
	}
	else
	{
		XTPShortcutManager()->DisableShortcuts(TRUE);
		if (m_hAccelTable)
			DestroyAcceleratorTable( m_hAccelTable );
		m_hAccelTable = NULL;
		LoadAccelTable( MAKEINTRESOURCE(IDR_GAMEACCELERATOR) );
		CLogFile::WriteLine( "Disable Accelerators" );
	}

}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::EditAccelerator()
{
	// Open accelerator key manager dialog.
	//accelManager.EditKeyboardShortcuts(this);
	OnCustomize();
}

//////////////////////////////////////////////////////////////////////////
bool CMainFrame::FindMenuPos(CMenu *pBaseMenu, UINT myID, CMenu * & pMenu, int & mpos)
{
	// REMARK: pMenu is a pointer to a Cmenu-Pointer
	int myPos;
	if( pBaseMenu == NULL )
	{
		// Sorry, Wrong Number
		pMenu = NULL;
		mpos = -1;
		return false;
	}
	for( myPos = pBaseMenu->GetMenuItemCount() -1; myPos >= 0; myPos-- )
	{
		int Status = pBaseMenu->GetMenuState( myPos, MF_BYPOSITION);
		CMenu* mNewMenu;

		if( Status == 0xFFFFFFFF )
		{
			// That was not an legal Menu/Position-Combination
			pMenu = NULL;
			mpos = -1;
			return false;
		}
		// Is this the real one?
		if( pBaseMenu->GetMenuItemID(myPos) == myID )
		{
			// Yep!
			pMenu = pBaseMenu;
			mpos = myPos;
			return true;
		}
		// Maybe a subMenu?
		mNewMenu = pBaseMenu->GetSubMenu(myPos);
		// This function will return NULL if ther is NO SubMenu
		if( mNewMenu != NULL )
		{
			// recursive
			bool found = FindMenuPos( mNewMenu, myID, pMenu, mpos);
			if(found)
				return true;	// return this loop
		}
		// I have to check the next in my loop
	}
	return false; // iterate in the upper stackframe
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::UpdateToolsMenu()
{
	CXTPMenuBar *pMenuBar = GetCommandBars()->GetMenuBar();
	CXTPControl *pControl = pMenuBar->GetControls()->FindControl(xtpControlError,ID_TOOLS_TOOL1,FALSE,TRUE);
	if (pControl && pControl->GetType() == xtpControlPopup)
	{
		CMenu menu;
		menu.CreatePopupMenu();
		CExternalToolsManager *pTools = GetIEditor()->GetExternalToolsManager();
		for (int i = 0; i < pTools->GetToolsCount(); i++)
		{
			menu.AppendMenu( MF_BYPOSITION|MF_STRING, ID_TOOL1+i, pTools->GetTool(i)->m_title );
		}
		pControl->GetCommandBar()->LoadMenu( &menu );

	}
}

void CMainFrame::UpdateExternalsTools()
{
	m_toolBoxDialog.UpdateTools();
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnExecuteTool( UINT nID )
{
	GetIEditor()->GetExternalToolsManager()->ExecuteTool( nID-ID_TOOL1 );
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::UpdateViewPaneMenu()
{
	CXTPMenuBar *pMenuBar = GetCommandBars()->GetMenuBar();
	CXTPControl *pControl = pMenuBar->GetControls()->FindControl(xtpControlError,ID_VIEW_OPENVIEWPANE,FALSE,TRUE);
	if (pControl && pControl->GetType() == xtpControlPopup)
	{
		CMenu menu;
		menu.CreatePopupMenu();

		m_viewPaneClasses.clear();
	
		int nViews = 0;
		int i;
		std::vector<CViewportDesc*> vdesc;
		GetIEditor()->GetViewManager()->GetViewportDescriptions( vdesc );
		m_viewPaneClasses.reserve(vdesc.size());
		for (i = 0; i < vdesc.size(); i++)
		{
			IViewPaneClass *pViewClass = (IViewPaneClass*)vdesc[i]->pViewClass;
			if (!pViewClass)
				continue;
			int flags = MF_STRING;
			if (pViewClass->ClassName() == GetIEditor()->GetActiveView()->GetName())
				flags |= MF_CHECKED;
			m_viewPaneClasses.push_back(pViewClass);
			menu.AppendMenu( flags, ID_VIEW_OPENPANE0+(nViews++),vdesc[i]->name );
		}

		menu.AppendMenu( MF_SEPARATOR,0,"" );

		std::vector<IClassDesc*> classes;
		GetIEditor()->GetClassFactory()->GetClassesBySystemID( ESYSTEM_CLASS_VIEWPANE,classes );
		for (i = 0; i < classes.size(); i++)
		{
			IClassDesc *pClass = classes[i];
			IViewPaneClass *pViewClass = NULL;
			HRESULT hRes = pClass->QueryInterface( __uuidof(IViewPaneClass),(void**)&pViewClass );
			if (FAILED(hRes))
			{
				continue;
			}
			if (stl::find(m_viewPaneClasses,pViewClass))
				continue;
			int flags = MF_STRING;
			if (pViewClass->ClassName() == GetIEditor()->GetActiveView()->GetName())
				flags |= MF_CHECKED;
			m_viewPaneClasses.push_back(pViewClass);
			menu.AppendMenu( flags, ID_VIEW_OPENPANE0+(nViews++),pViewClass->GetPaneTitle() );
		}

		pControl->GetCommandBar()->LoadMenu( &menu );
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnOpenViewPane( UINT nID )
{
	int nIndex = nID-ID_VIEW_OPENPANE0;
	std::vector<IClassDesc*> classes;
	GetIEditor()->GetClassFactory()->GetClassesBySystemID( ESYSTEM_CLASS_VIEWPANE,classes );
	CXTPMenuBar *pMenuBar = GetCommandBars()->GetMenuBar();
	CXTPControl *pControl = pMenuBar->GetControls()->FindControl(xtpControlError,ID_VIEW_OPENVIEWPANE,FALSE,TRUE);
	for (int i = 0; i < classes.size(); i++)
	{
		IClassDesc *pClass = classes[i];
		IViewPaneClass *pViewClass = NULL;
		if (!FAILED(pClass->QueryInterface( __uuidof(IViewPaneClass),(void**)&pViewClass )))
		{
			if (pViewClass->ClassName() == m_viewPaneClasses[nIndex]->ClassName())
			{
				// NickH: Allow multiple anim graph windows. Nasty workaround, but this is the recommended approach for now :-(
				if(0==strncmp("Animation Graph",pViewClass->ClassName(),sizeof("Animation Graph")-1))
				{
					OpenPage( pViewClass->ClassName(),false );
				}
				else
					OpenPage( pViewClass->ClassName() );
				break;
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
void CMainFrame::UpdateViewLayoutsPaneMenu( CXTPControl* pLayoutMenuControl )
{
	if ( pLayoutMenuControl == NULL )
	{
		return;
	}

	if ( pLayoutMenuControl->GetType() != xtpControlPopup )
	{
		return;
	}

	CMenu menu;
	menu.CreatePopupMenu();

	m_layoutFilenames.clear();
	CFileEnum::ScanDirectory( LAYOUTS_PATH, LAYOUTS_WILDCARD, m_layoutFilenames, false );

	std::sort( m_layoutFilenames.begin(), m_layoutFilenames.end(), CStringNoCasePredicate::LessThan() );

	const int MAX_LAYOUTS = ID_VIEW_LAYOUT_LAST - ID_VIEW_LAYOUT_FIRST;

	for ( int i = 0; i < m_layoutFilenames.size() && i <= MAX_LAYOUTS; i++ )
	{
		CString layoutFilename = m_layoutFilenames[ i ];
		CString layoutName = Path::RemoveExtension( layoutFilename );

		menu.AppendMenu( MF_STRING, ID_VIEW_LAYOUT_FIRST + i, layoutName );
	}

	menu.AppendMenu( MF_SEPARATOR );
	menu.AppendMenu( MF_STRING, ID_VIEW_SAVELAYOUT, "Save Layout..." );
	menu.AppendMenu( MF_STRING, ID_VIEW_LAYOUT_LOAD_DEFAULT, "Restore Default Layout" );

	pLayoutMenuControl->GetCommandBar()->LoadMenu( &menu );

}

void CMainFrame::UpdateViewLayoutsPaneSubMenu( CXTPControl* pLayoutSubMenuControl, int layoutId )
{
	assert( pLayoutSubMenuControl != NULL );
	if ( pLayoutSubMenuControl == NULL )
	{
		return;
	}

	CMenu menu;
	menu.CreatePopupMenu();

	menu.AppendMenu( MF_STRING, ID_VIEW_LAYOUT_LOAD_FIRST + layoutId, "Load" );
	menu.AppendMenu( MF_STRING, ID_VIEW_LAYOUT_SAVE_FIRST + layoutId, "Save" );
	menu.AppendMenu( MF_STRING, ID_VIEW_LAYOUT_RENAME_FIRST + layoutId, "Rename..." );
	menu.AppendMenu( MF_STRING, ID_VIEW_LAYOUT_DELETE_FIRST + layoutId, "Delete" );

	
	CXTPCommandBar* pSubMenuCommandBar = pLayoutSubMenuControl->GetCommandBar();
	assert( pSubMenuCommandBar != NULL );
	if ( pSubMenuCommandBar == NULL )
	{
		return;
	}

	pSubMenuCommandBar->LoadMenu( &menu );
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnViewSavePaneLayout()
{
	static CString s_lastLayoutName = "";

	CStringDlg askLayoutNameDialog( "Layout Name" );
	askLayoutNameDialog.SetString( s_lastLayoutName );
	INT_PTR result = askLayoutNameDialog.DoModal();
	if ( result != IDOK )
	{
		return;
	}

	CreateDirectory( LAYOUTS_PATH, NULL );

	CString layoutName = askLayoutNameDialog.GetString();
	layoutName = Path::GetFile( layoutName ); // Remove any path data that the user might add.
	CString layoutFilename = layoutName + LAYOUTS_EXTENSION;
	CString layoutFullFilename = LAYOUTS_PATH + layoutFilename;

	s_lastLayoutName = layoutName;

	bool layoutNameExists = ( std::find_if( m_layoutFilenames.begin(), m_layoutFilenames.end(), CStringNoCasePredicate::Equal( layoutFilename ) ) != m_layoutFilenames.end() );
	if ( layoutNameExists )
	{	
		int overwriteMessageResult = MessageBox( _T( "The chosen layout name already exists. Do you want to overwrite it?" ), _T( "Overwrite Layout?" ), MB_YESNO );
		if ( overwriteMessageResult != IDYES )
		{
			OnViewSavePaneLayout();
			return;
		}
	}

	CXTPDockingPaneLayout currentLayout( GetDockingPaneManager() );
	GetDockingPaneManager()->GetLayout( &currentLayout );

	currentLayout.SaveToFile( layoutFullFilename, _T( DUMMY_LAYOUT_NAME ) );

}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnViewRestoreDefaultLayout()
{
	assert( m_pDefaultLayout != NULL );
	if ( m_pDefaultLayout == NULL )
	{
		return;
	}

	m_paneManager.SetLayout( m_pDefaultLayout );
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnViewLoadPaneLayout( UINT nID )
{
	int layoutId = nID - ID_VIEW_LAYOUT_LOAD_FIRST;

	bool layoutIdIsValid = ( 0 <= layoutId && layoutId < m_layoutFilenames.size() );
	if ( ! layoutIdIsValid )
	{
		return;
	}

	CString layoutFilename = m_layoutFilenames[ layoutId ];
	CString layoutFullFilename = LAYOUTS_PATH + layoutFilename;

	CXTPDockingPaneLayout layout( GetDockingPaneManager() );
	layout.LoadFromFile( layoutFullFilename, _T( DUMMY_LAYOUT_NAME ) );

	bool loadedLayoutIsValid = ( 0 < layout.GetPaneList().GetCount() );
	if ( loadedLayoutIsValid )
	{
		GetDockingPaneManager()->SetLayout( &layout );
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnViewSavePaneLayout( UINT nID )
{
	int layoutId = nID - ID_VIEW_LAYOUT_SAVE_FIRST;

	bool layoutIdIsValid = ( 0 <= layoutId && layoutId < m_layoutFilenames.size() );
	if ( ! layoutIdIsValid )
	{
		return;
	}

	CString layoutFilename = m_layoutFilenames[ layoutId ];
	CString layoutName = Path::RemoveExtension( layoutFilename );
	CString layoutFullFilename = LAYOUTS_PATH + layoutFilename;

	int saveConfirmationMessageResult = MessageBox( _T( "Do you want to overwrite the layout '" + layoutName + "' with the current one?" ), _T( "Overwrite Layout?" ), MB_YESNO );
	if ( saveConfirmationMessageResult != IDYES )
	{
		return;
	}

	CXTPDockingPaneLayout currentLayout( GetDockingPaneManager() );
	GetDockingPaneManager()->GetLayout( &currentLayout );

	currentLayout.SaveToFile( layoutFullFilename, _T( DUMMY_LAYOUT_NAME ) );
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnViewRenamePaneLayout( UINT nID )
{
	int layoutId = nID - ID_VIEW_LAYOUT_RENAME_FIRST;

	bool layoutIdIsValid = ( 0 <= layoutId && layoutId < m_layoutFilenames.size() );
	if ( ! layoutIdIsValid )
	{
		return;
	}

	CString layoutFilename = m_layoutFilenames[ layoutId ];
	CString layoutName = Path::RemoveExtension( layoutFilename );

	CStringDlg askLayoutNameDialog( "Rename layout '" + layoutName + "'" );
	askLayoutNameDialog.SetString( layoutName );
	INT_PTR result = askLayoutNameDialog.DoModal();
	if ( result != IDOK )
	{
		return;
	}

	CString newLayoutName = askLayoutNameDialog.GetString();
	newLayoutName = Path::GetFile( newLayoutName ); // Remove any path data that the user might add.
	CString newLayoutFilename = newLayoutName + LAYOUTS_EXTENSION;

	bool newLayoutNameExists = ( std::find_if( m_layoutFilenames.begin(), m_layoutFilenames.end(), CStringNoCasePredicate::Equal( newLayoutFilename ) ) != m_layoutFilenames.end() );

	if ( newLayoutNameExists )
	{
		MessageBox( _T( "The layout name '" + newLayoutName +"' already exists, please choose a different name" ), _T( "Layout name already exists" ), MB_OK );
		OnViewRenamePaneLayout( nID );
		return;
	}

	CString layoutFullFilename = LAYOUTS_PATH + layoutFilename;
	CString newLayoutFullFilename = LAYOUTS_PATH + newLayoutFilename;

	CFile::Rename( layoutFullFilename, newLayoutFullFilename );
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnViewDeletePaneLayout( UINT nID )
{
	int layoutId = nID - ID_VIEW_LAYOUT_DELETE_FIRST;

	bool layoutIdIsValid = ( 0 <= layoutId && layoutId < m_layoutFilenames.size() );
	if ( ! layoutIdIsValid )
	{
		return;
	}

	CString layoutFilename = m_layoutFilenames[ layoutId ];
	CString layoutFullFilename = LAYOUTS_PATH + layoutFilename;
	CString layoutName = Path::RemoveExtension( layoutFilename );

	int confirmationMessageResult = MessageBox( _T( "Are you sure you want to delete the layout '" + layoutName +"'?" ), _T( "Delete Layout?" ), MB_YESNO );
	if ( confirmationMessageResult != IDYES )
	{
		return;
	}

	CFile::Remove( layoutFullFilename );
}

////////////////////////////////////////////////////////////////////////////
////implementation of the callback for removing recently used files
//void CMainFrame::OnFileMRURemove( UINT nID )
//{
//	int nIndex = nID - ID_FILE_MRU_REMOVE_FIRST;
//
//	bool bIndexIsValid = ( 0 <= nIndex && nIndex < 16 );
//	if (!bIndexIsValid)
//		return;
//
//	CRecentFileList *pRecentFileList = ((CCryEditApp *) (AfxGetApp()))->GetRecentFileList();
//	if (!pRecentFileList)
//	{
//		pRecentFileList->Remove(nIndex);
//		pRecentFileList->WriteList();
//	}
//}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateSnapToGrid(CCmdUI* pCmdUI)
{
	bool bEnabled = gSettings.pGrid->IsEnabled();
	if (bEnabled)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);

	float gridSize = gSettings.pGrid->size;
	if (gridSize != m_gridSize)
	{
		m_gridSize = gridSize;
		CString str;
		str.Format( "%g",gridSize );
		pCmdUI->SetText( str );
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateCurrentLayer(CCmdUI* pCmdUI)
{
	if (GetIEditor()->GetObjectManager()->GetLayersManager()->GetCurrentLayer() != m_currentLayer)
	{
		m_currentLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
		if (m_currentLayer)
		{
			CString text = CString(" ")+m_currentLayer->GetName();
			if(m_currentLayer->IsModified())
				text+="*";
				pCmdUI->SetText( text );
		}
		else
			pCmdUI->SetText( CString("") );
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnToolbarDropDown( int idCommand,CRect &rcControl )
{
	CRect rc;
	CPoint pos;
	GetCursorPos( &pos );
	
	pos.x = rcControl.left;
	pos.y = rcControl.bottom;

/*
	NMTOOLBAR* pnmtb = (NMTOOLBAR*)pnhdr;

	//GetItemRect( pnmtb->iItem,rc );
	rc = pnmtb->rcButton;
	ClientToScreen( rc );
	pos.x = rc.left;
	pos.y = rc.bottom;
	*/

	// Switch on button command id's.
	switch (idCommand)
	{
	case ID_UNDO:
		{
			CUndoDropDown undoDialog( pos,true,AfxGetMainWnd() );
			undoDialog.DoModal();
		}
		break;
	case ID_REDO:
		{
			CUndoDropDown undoDialog( pos,false,AfxGetMainWnd() );
			undoDialog.DoModal();
		}
		break;
	case ID_SELECT_AXIS_TERRAIN:
		//OnAxisTerrainMenu(pos);
		break;
	case ID_SNAP_TO_GRID:
		{
			// Display drop down menu with snap values.
			OnSnapMenu(pos);
		}
		break;
	default:
		return;
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent)
	{
	case AUTOSAVE_TIMER_EVENT:
		{
			if( gSettings.autoBackupEnabled )
			{
				// Call autosave function of CryEditApp.
				((CCryEditApp*)AfxGetApp())->SaveAutoBackup();
			}
			break;
		}
	case AUTOREMIND_TIMER_EVENT:
		{
			if( gSettings.autoRemindTime > 0 )
			{
				// Remind to save.
				((CCryEditApp*)AfxGetApp())->SaveAutoRemind();
			}
			break;
		}
	case NETWORK_AUDITION_UPDATE_TIMER_EVENT:
		{
			if (GetISystem())
				if (gEnv->pSoundSystem->GetIAudioDevice())
					gEnv->pSoundSystem->GetIAudioDevice()->Update();
			break;
		}
	default:
		{
		}
	}

	__super::OnTimer(nIDEvent);
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnEditNextSelectionMask()
{
	m_editModeBar.NextSelectMask();
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::ShowDataBaseDialog( bool bShow )
{
	//ShowControlBar( &m_wndDataBaseBar,bShow,TRUE );
	//m_wndDataBaseBar.Invalidate();
	//m_wndDataBase.RedrawWindow( NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE|RDW_ALLCHILDREN );
}

//////////////////////////////////////////////////////////////////////////
bool CMainFrame::IsDockedWindowChild( CWnd *pWnd )
{
	/*
	if (!pWnd)
		return false;
	if (pWnd->IsChild(&m_wndDataBaseBar))
		return true;
	if (pWnd->IsChild(&m_wndRollUpBar))
		return true;
	if (pWnd->IsChild(&m_wndConsoleBar))
		return true;
	if (pWnd->IsChild(&m_wndTrackViewBar))
		return true;
*/
	return false;
}

//////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == XTP_DPN_ACTION)
	{
		XTP_DOCKINGPANE_ACTION* pAction = (XTP_DOCKINGPANE_ACTION*)lParam;
		//CryLogAlways("DockPaneAction: %i, %s", pAction->action,(const char*)pAction->pPane->GetTitle() );
		return TRUE;
	}
	if (wParam == XTP_DPN_SHOWWINDOW)
	{
		// get a pointer to the docking pane being shown.
		CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;
		if (!pwndDockWindow->IsValid())
		{
			switch (pwndDockWindow->GetID())
			{
			case IDW_VIEW_CONSOLE_BAR:
				pwndDockWindow->Attach(&m_cConsole);
				break;
			case IDW_VIEW_ROLLUP_BAR:
				pwndDockWindow->Attach(&m_wndRollUp);
				break;
			case IDW_VIEW_DIALOGTOOL_BAR:
				m_toolBoxDialog.UpdateTools();
				pwndDockWindow->Attach(&m_toolBoxDialog);
				m_toolBoxDialog.ShowWindow(SW_SHOW);
				break;
			default:
				{
					// [2007/05/21 MichaelS] It's possible that we can get recursive calls here, which can result in
					// trying to construct multiple instances of panes (such as the Facial Editor). This may happen
					// as a result of an assert dialog being created as a child of the main frame - for some reason
					// XT sends us a docking pane notify message that looks like a request to create the view a second
					// time. Therefore we add this guard code to detect and reject that request.
					// Note that this check as it stands could allow a situation where pane A opens pane B in its
					// constructor and then XT tries to create pane A again, but this behaviour doesn't seem to occur.
					static CXTPDockingPane* s_pPaneBeingConstructed = 0;
					if (s_pPaneBeingConstructed != pwndDockWindow)
					{
						s_pPaneBeingConstructed = pwndDockWindow;

						// Make a dynamic pane.
						PanesMap::iterator it = m_panesMap.find(pwndDockWindow->GetID());
						if (it != m_panesMap.end())
						{
							SViewPaneDesc &pn = it->second;
							if (!pn.m_pViewWnd)
							{
								CWnd *pWnd = (CWnd*)pn.m_pRuntimeClass->CreateObject();
								assert(pWnd);
								pn.m_pViewWnd = pWnd;
							}
							pn.m_pViewWnd->ShowWindow(SW_SHOW);
							pwndDockWindow->Attach(pn.m_pViewWnd);
						}
						else
						{
							// Try to attach to pane.
							AttachToPane(pwndDockWindow);
						}

						s_pPaneBeingConstructed = 0;
					}
				}
			}
		}
		return TRUE;
	}
	else if (wParam == XTP_DPN_CLOSEPANE)
	{
		// get a pointer to the docking pane being shown.
		CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;
		if (pwndDockWindow->IsValid())
		{
			switch (pwndDockWindow->GetID())
			{
			case IDW_VIEW_CONSOLE_BAR:
				//pwndDockWindow->Attach(&m_cConsole);
				break;
			case IDW_VIEW_ROLLUP_BAR:
				//pwndDockWindow->Attach(&m_wndRollUp);
				break;
			case IDW_VIEW_DIALOGTOOL_BAR:
				pwndDockWindow->Detach();
				m_toolBoxDialog.ShowWindow(SW_HIDE);
				break;
			default:
				{
					// Delete a dynamic pane.
					PanesMap::iterator it = m_panesMap.find(pwndDockWindow->GetID());
					if (it != m_panesMap.end())
					{
						SPaneHistory paneHistory;
						paneHistory.rect = pwndDockWindow->GetPaneWindowRect();
						paneHistory.dockDir = GetDockingPaneManager()->GetPaneDirection(pwndDockWindow);
						m_panesHistoryMap[pwndDockWindow->GetTitle()] = paneHistory;

						SViewPaneDesc &pn = it->second;
						pn.m_pViewWnd->DestroyWindow();
						pn.m_pViewWnd = 0;
						pwndDockWindow->Detach();
						m_panesMap.erase(it);
					}
				}
			}
		}

		return TRUE; // handled
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnCustomize()
{
	// Get a pointer to the command bars object.
	CXTPCommandBars* pCommandBars = GetCommandBars();
	if(pCommandBars != NULL)
	{
		// Instanciate the customize dialog object.
		CXTPCustomizeSheet dlg(pCommandBars);

		// Add the options page to the customize dialog.
		CXTPCustomizeOptionsPage pageOptions(&dlg);
		dlg.AddPage(&pageOptions);

		// Add the commands page to the customize dialog.
		CXTPCustomizeCommandsPage* pCommands = dlg.GetCommandsPage();
		pCommands->AddCategories(IDR_MAINFRAME);

		// Use the command bar manager to initialize the 
		// customize dialog.
		pCommands->InsertAllCommandsCategory();
		pCommands->InsertBuiltInMenus(IDR_MAINFRAME);
		pCommands->InsertNewMenuCategory();

		// Add the options page to the customize dialog.
		CXTPCustomizeKeyboardPage pageKeyboard(&dlg);
		dlg.AddPage(&pageKeyboard);
		pageKeyboard.AddCategories(IDR_MAINFRAME,TRUE);
		//pageKeyboard.InsertCategory(()


		// Dispaly the dialog.
		dlg.DoModal();
	}
	SaveConfig();
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMXTPCONTROL* tagNMCONTROL = (NMXTPCONTROL*)pNMHDR;
	CXTPControlComboBox* pControl = (CXTPControlComboBox*)tagNMCONTROL->pControl;
	if (pControl->GetType() == xtpControlComboBox)
	{
		CString selection = pControl->GetEditText();
		m_selectionName = selection;
		if (!selection.IsEmpty())
		{
			if (GetIEditor()->GetObjectManager()->GetSelection( selection ))
				GetIEditor()->GetObjectManager()->SetSelection( selection );
			else
			{
				GetIEditor()->GetObjectManager()->NameSelection( selection );
				if (pControl->FindStringExact(0,selection.GetBuffer())==CB_ERR)
				{
					pControl->AddString(selection.GetBuffer());
				}				
			}
		}
	}
	*pResult = 1;
}
//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnDeleteSelection()
{
		if (!m_selectionName.IsEmpty())
		{
			CUndo undo( "Del Selection Group" );
			int   nItemIndex(0);

			GetIEditor()->BeginUndo();
			GetIEditor()->GetObjectManager()->RemoveSelection(m_selectionName);
			GetIEditor()->SetModifiedFlag();
			GetIEditor()->SetModifiedModule(eModifiedBrushes);
			GetIEditor()->Notify( eNotify_OnInvalidateControls );

			CXTPControlComboBox*		poComboBox(NULL);
			CXTPControl*						poControl(NULL);

			int											nTotal(m_pEditToolBar->GetControlCount());
			int											nCurrent(0);

			// As there doesn't seem to be a better way to get an specific control...
			// we need to iterate through all the controls in this bar.
			for (nCurrent=0;nCurrent<nTotal;++nCurrent)
			{
				poControl=m_pEditToolBar->GetControl(nCurrent);
				if (poControl->GetCaption().CompareNoCase("Selection Set")==0)
				{
					poComboBox=dynamic_cast<CXTPControlComboBox*>(poControl);
				}
			}

			nItemIndex=poComboBox->FindStringExact(0,m_selectionName.GetBuffer());
			if (nItemIndex!=CB_ERR)
			{
				poComboBox->DeleteItem(nItemIndex);
				poComboBox->SetEditText("");
			}			

			m_selectionName="";
		}
}
//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnSelectionMaskChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMXTPCONTROL* tagNMCONTROL = (NMXTPCONTROL*)pNMHDR;
	CXTPControlComboBox* pControl = (CXTPControlComboBox*)tagNMCONTROL->pControl;
	if (pControl->GetType() == xtpControlComboBox)
	{
		int sel = pControl->GetCurSel();
		if (sel != CB_ERR)
		{
			gSettings.objectSelectMask = pControl->GetItemData(sel);
			m_objectSelectionMask = gSettings.objectSelectMask;
		}
	}
	*pResult = 1;
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnRefCoordSysChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMXTPCONTROL* tagNMCONTROL = (NMXTPCONTROL*)pNMHDR;
	CXTPControlComboBox* pControl = (CXTPControlComboBox*)tagNMCONTROL->pControl;
	if (pControl->GetType() == xtpControlComboBox)
	{
		int sel = pControl->GetCurSel();
		if (sel != CB_ERR)
		{
			switch (sel)
			{
			case 0:
				GetIEditor()->SetReferenceCoordSys( COORDS_VIEW );
				break;
			case 1:
				GetIEditor()->SetReferenceCoordSys( COORDS_LOCAL );
				break;
			case 2:
				GetIEditor()->SetReferenceCoordSys( COORDS_PARENT );
				break;
			case 3:
				GetIEditor()->SetReferenceCoordSys( COORDS_WORLD );
				break;
			case 4:
				GetIEditor()->SetReferenceCoordSys( COORDS_USERDEFINED );
				break;
			}
		}
	}
	*pResult = 1;
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateSelectionMask(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);
	if (m_objectSelectionMask != gSettings.objectSelectMask)
	{
		CXTPControlComboBox* pControl = (CXTPControlComboBox*)GetCommandBars()->FindControl(xtpControlComboBox,IDC_SELECTION_MASK,TRUE,TRUE);
		if (!pControl)
			return;
		m_objectSelectionMask = gSettings.objectSelectMask;
		int sel = LB_ERR;
		for (int i = 0; i < pControl->GetCount(); i++)
		{
			if (pControl->GetItemData(i) == gSettings.objectSelectMask)
			{
				sel = i;
				break;
			}
		}
		if (sel != CB_ERR && sel != pControl->GetCurSel())
		{
			pControl->SetCurSel(sel);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateRefCoordSys(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);
	if (m_coordSys != GetIEditor()->GetReferenceCoordSys())
	{
		CXTPControlComboBox* pControl = (CXTPControlComboBox*)GetCommandBars()->FindControl(xtpControlComboBox,ID_REF_COORDS_SYS,TRUE,TRUE);
		if (!pControl)
			return;
		int sel = CB_ERR;
		m_coordSys = GetIEditor()->GetReferenceCoordSys();
		switch (m_coordSys)
		{
		case COORDS_VIEW:
			sel = 0;
			break;
		case COORDS_LOCAL:
			sel = 1;
			break;
		case COORDS_PARENT:
			sel = 2;
			break;
		case COORDS_WORLD:
			sel = 3;
			break;
		case COORDS_USERDEFINED:
			sel = 4;
			break;
		};
		if (sel != CB_ERR && sel != pControl->GetCurSel())
		{
			pControl->SetCurSel(sel);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateMissions(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);
	CCryEditDoc *doc = GetIEditor()->GetDocument();
	if (doc->GetCurrentMission() != m_currentMission)
	{
		CXTPControlComboBox* pControl = (CXTPControlComboBox*)GetCommandBars()->FindControl(xtpControlComboBox,IDC_MISSION,FALSE,TRUE);
		if (!pControl)
			return;
		m_currentMission = GetIEditor()->GetDocument()->GetCurrentMission();
		int sel = CB_ERR;
		pControl->ResetContent();
		for (int i = 0; i < doc->GetMissionCount(); i++)
		{
			pControl->AddString( doc->GetMission(i)->GetName() );
			if (m_currentMission && doc->GetMission(i)->GetName() == m_currentMission->GetName())
				sel = i;
		}
		if (sel != CB_ERR)
		{
			pControl->SetCurSel(sel);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnMissionChanged( NMHDR* pNMHDR, LRESULT* pResult )
{
	if (IsPreview())
		return;

	NMXTPCONTROL* tagNMCONTROL = (NMXTPCONTROL*)pNMHDR;
	CXTPControlComboBox* pControl = (CXTPControlComboBox*)tagNMCONTROL->pControl;
	if (pControl->GetType() == xtpControlComboBox)
	{
		int sel = pControl->GetCurSel();
		if (sel != CB_ERR)
		{
			CString str;
			pControl->GetLBText(sel,str);
			CCryEditDoc *doc = (CCryEditDoc*)GetActiveDocument();
			CMission *mission = doc->FindMission(str);
			if (mission)
			{
				doc->SetCurrentMission( mission );
				m_currentMission = 0;
			}	
		}
	}
	*pResult = 1;
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::InvalidateControls()
{
	m_currentLayer = 0;
	m_currentMission = 0;
	m_gridSize = -1;
	m_coordSys = (RefCoordSys)-1;
	m_objectSelectionMask = -1;
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::ResetUI()
{
	m_panesHistoryMap.clear();
	GetCommandBars()->ResetUsageData();
}

LRESULT CMainFrame::OnMatEditSend(WPARAM w, LPARAM l)
{
	if (w == eMSM_Init)
		return 0;
	CBaseLibraryDialog *pLibDlg = GetIEditor()->OpenDataBaseLibrary( EDB_TYPE_MATERIAL,GetIEditor()->GetMaterialManager()->GetSelectedItem() );
	if(pLibDlg)
	{
		GetIEditor()->GetMaterialManager()->SyncMaterialEditor();
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnProgressCancelClicked()
{
	CWaitProgress::CancelCurrent();
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateMemoryStatus( CCmdUI *pCmdUI )
{
	static uint64 nPrevSize = 0;
	
	ProcessMemInfo mi;
	CProcessInfo::QueryMemInfo( mi );

	//MEMORYSTATUS ms;
	//ms.dwLength = sizeof(MEMORYSTATUS);
	//GlobalMemoryStatus(&ms);
	CString info;
	//ms.dwAvailVirtual/(1024*1024),ms.dwTotalVirtual/(1024*1024) 

	uint64 nSizeMb = (uint64)(mi.PagefileUsage/(1024*1024));
	info.Format( "%d Mb",(uint32)nSizeMb );
	pCmdUI->SetText( info );

	if (nPrevSize != nSizeMb)
	{
#ifndef WIN64
		nPrevSize = nSizeMb;
		if (nSizeMb > 1250)
		{
			//m_wndStatusIconPane.SetPaneIcon( IDI_STATUS_MEM_VERY_LOW );
			m_wndStatusBar.Invalidate();
		}
		else if (nSizeMb > 1000)
		{
			//m_wndStatusIconPane.SetPaneIcon( IDI_STATUS_MEM_LOW );
			m_wndStatusBar.Invalidate();
		}
		else
		{
			//m_wndStatusIconPane.SetPaneIcon( IDI_STATUS_MEM_OK );
		}
#else //WIN64
		//m_wndStatusIconPane.SetPaneIcon( IDI_STATUS_MEM_OK );
#endif
	}
	/*
	BOOL bMemStatus;
	QueryMemoryResourceNotification( hLowMemSignal,&bMemStatus );
	if (bMemStatus)
	{
		pCmdUI->SetText( CString("Low Memory!!!") + info );
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateConsoleMemoryStatus( CCmdUI *pCmdUI )
{
	if (GetISystem()->GetConfigSpec() == CONFIG_X360 && GetISystem()->GetIGame())
	{
		int systemMemInMB=0;
		int videoMemTextureInMB=0;
		int videoMemMeshInMB=0;
		GetISystem()->GetIBudgetingSystem()->GetXbox360MemoryEstimation( systemMemInMB,videoMemTextureInMB,videoMemMeshInMB );

		CString info;
		info.Format( "X360=%d Mb",(uint32)systemMemInMB+videoMemTextureInMB+videoMemMeshInMB );
		pCmdUI->SetText( info );
	}
	else if (GetISystem()->GetConfigSpec() == CONFIG_PS3 && GetISystem()->GetIGame())
	{
		int systemMemInMB=0;
		int videoMemTextureInMB=0;
		int videoMemMeshInMB=0;
		GetISystem()->GetIBudgetingSystem()->GetXbox360MemoryEstimation( systemMemInMB,videoMemTextureInMB,videoMemMeshInMB );

		CString info;
		info.Format( "PS3=%d Mb",(uint32)systemMemInMB+videoMemTextureInMB+videoMemMeshInMB );
		pCmdUI->SetText( info );
	}
	else
	{
		pCmdUI->SetText( "" );
	}
}


//////////////////////////////////////////////////////////////////////////
void CMainFrame::ResetAutoSaveTimers()
{
	if (m_autoSaveTimer)
		KillTimer( m_autoSaveTimer );
	if (m_autoRemindTimer)
		KillTimer( m_autoRemindTimer );
	m_autoSaveTimer = 0;
	m_autoRemindTimer = 0;

	if (gSettings.autoBackupTime > 0 && gSettings.autoBackupEnabled)
		m_autoSaveTimer = SetTimer( AUTOSAVE_TIMER_EVENT,gSettings.autoBackupTime*1000*60,0 );
	if (gSettings.autoRemindTime > 0)
		m_autoRemindTimer = SetTimer( AUTOREMIND_TIMER_EVENT,gSettings.autoRemindTime*1000*60,0 );
}

XTPDockingPanePaintTheme CMainFrame::GetDockingPaneTheme() { return s_themePane; }
BOOL CMainFrame::GetDockingHelpers() { return s_bDockingHelpers; }
XTPPaintTheme CMainFrame::GetPaintTheme() { return s_themePaint; }
