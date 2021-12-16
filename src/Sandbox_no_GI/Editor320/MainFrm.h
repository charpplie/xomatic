// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__88B37B80_D04F_46F1_8FEF_A09696002A81__INCLUDED_)
#define AFX_MAINFRM_H__88B37B80_D04F_46F1_8FEF_A09696002A81__INCLUDED_


//#define INCLUDE_LIVE_MOCAP


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <XTToolkitPro.h>

#include "Controls\ConsoleSCB.h"

#include "Controls\RollupBar.h"
#include "Controls\RollupCtrl.h"

#include "Controls\HiColorToolBar.h"
#include "Controls\ToolbarTab.h"
#include "Controls\EditModeToolbar.h"
#include "Include\IViewPane.h"
#include "ToolBoxBar.h"

// forward declaration.
class CMission;
class CLayoutWnd;

class SANDBOX_API CMainFrame : public CXTPFrameWnd,public CXTPOffice2007FrameHook, public IEditorNotifyListener
{
	
public: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

	static XTPDockingPanePaintTheme GetDockingPaneTheme();
	static BOOL GetDockingHelpers();
	static XTPPaintTheme GetPaintTheme();

// Attributes
public:

	//! Show window and restore saved state.
	void ShowWindowEx(int nCmdShow);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL DestroyWindow();
	virtual void ActivateFrame(int nCmdShow);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext);

	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Access the status bar to display toolbar tooltips and status messages
	void SetStatusText( LPCTSTR pszText );
	void SetStatusText(CString strText) { SetStatusText((const char*)strText); };

	// Parameter nItemIdentifier can be found in MainFrm.cpp, in structure "indicators"
	void SetItemStatusText(int nItemIdentifier,LPCTSTR pszText );
	void SetItemStatusText(int nItemIdentifier,CString strText ){SetItemStatusText(nItemIdentifier,(const char*)strText);};

	int SelectRollUpBar( int rollupBarId );
	CRollupCtrl* GetRollUpControl( int rollupBarId=ROLLUP_OBJECTS );

	CLayoutWnd*	GetLayout() { return m_layoutWnd; };
	CString GetSelectionName();
	void SetSelectionName( const CString &name );

	void IdleUpdate();
	void ResetUI();

	//! Enable/Disable keyboard accelerator.
	void EnableAccelerator( bool bEnable );
	//! Edit keyboard shortcuts.
	void EditAccelerator();

	// Check if dock state is valid with this window.
	BOOL VerifyBarState( CDockState &state );

	//! Save current window configuration.
	void SaveConfig();
	void LoadConfig();

	// ! Disables the main menu.
	void EnableMenu( bool bEnable );

	//! Put external tools to menu.
	void UpdateToolsMenu();
	void UpdateExternalsTools();
	void UpdateViewPaneMenu();

	//! Create the Views > Layouts menu.
	void UpdateViewLayoutsPaneMenu( CXTPControl* pLayoutMenuControl );
	//! Create a Layout sub menu with commands that can be done on a layout ( eg. Load, Delete... )
	void UpdateViewLayoutsPaneSubMenu( CXTPControl* pLayoutSubMenuControl, int layoutId );

	//! Returnns pointer to data base dialog.
	void ShowDataBaseDialog( bool bShow );

	// Check if some window is child of ouw docking windows.
	bool IsDockedWindowChild( CWnd *pWnd );

	// Reset timers used for auto saving.
	void ResetAutoSaveTimers();

	//////////////////////////////////////////////////////////////////////////
	// IEditorNotifyListener
	//////////////////////////////////////////////////////////////////////////
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );

	//////////////////////////////////////////////////////////////////////////
	// Panes Manager.
	//////////////////////////////////////////////////////////////////////////
	CXTPDockingPaneManager* GetDockingPaneManager() { return &m_paneManager; }
	CWnd* OpenPage( const char *sPaneClassName,bool bReuseOpened=true );
	CWnd* FindPage( const char *sPaneClassName );
	bool  ClosePage(const char *sPaneClassName);
	bool	IsPageDocked(const char *sPaneClassName);

	void OnToolbarDropDown( int idCommand,CRect &rcControl );
protected:
	void RegisterStdViewClasses();
	void InvalidateControls();
	void CreateRollupBar();
	void DockControlBarLeftOf(CControlBar *Bar, CControlBar *LeftOf);
	void DockControlBarNextTo(CControlBar* pBar,CControlBar* pTargetBar);
	bool IsPreview() const;
	void OnSnapMenu( CPoint pos );
	void SwitchTheme( XTPPaintTheme paintTheme,XTPDockingPanePaintTheme paneTheme );

	bool FindMenuPos(CMenu *pBaseMenu, UINT myID, CMenu * & pMenu, int & mpos);
	bool AttachToPane( CXTPDockingPane *pDockPane );

	struct SViewPaneDesc
	{
		int m_paneId;
		CString m_category;
		CWnd *m_pViewWnd;
		CRuntimeClass *m_pRuntimeClass;
		IViewPaneClass *m_pViewClass;
		bool m_bWantsIdleUpdate;
	};
	SViewPaneDesc* FindPaneByCategory( const char *sPaneCategory );
	SViewPaneDesc* FindPaneByClass( IViewPaneClass *pClass );

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void OnClose();
	afx_msg void OnMusicInfo();
	afx_msg void OnOpenSoundBrowser();
	afx_msg void OnOpenDialogBrowser();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnEditNextSelectionMask();
	afx_msg LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTrackView();
	afx_msg void OnDataBaseView();
	afx_msg void OnCustomize();
	afx_msg int OnCreateControl(LPCREATECONTROLSTRUCT lpCreateControl);
	afx_msg int OnCreateCommandBar(LPCREATEBARSTRUCT lpCreatePopup);
	afx_msg void OnInitCommandsPopup(CXTPPopupBar* pPopupBar);
	
	afx_msg void OnDockingHelpers();
	afx_msg void OnUpdateDockingHelpers(CCmdUI* pCmdUI);
	afx_msg void OnSkining();
	afx_msg void OnUpdateSkining(CCmdUI* pCmdUI);
	afx_msg void OnSkinChanged();
	afx_msg void LoadCrySkin();

	afx_msg void OnUpdateSnapToGrid(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCurrentLayer(CCmdUI* pCmdUI);
	afx_msg void OnSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteSelection();

	afx_msg void OnSelectionMaskChanged(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg void OnRefCoordSysChange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateRefCoordSys(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSelectionMask(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMissions(CCmdUI* pCmdUI);
	afx_msg void OnMissionChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnExecuteTool( UINT nID );
	afx_msg void OnOpenViewPane( UINT nID );
	afx_msg void OnUpdateControlBar(CCmdUI* pCmdUI);
	afx_msg BOOL OnToggleBar(UINT nID);
	afx_msg BOOL OnSwitchTheme(UINT nID);
	afx_msg void OnUpdateTheme(CCmdUI* pCmdUI);
	afx_msg BOOL OnSwitchSkin(UINT nID);
	afx_msg void OnUpdateSkin(CCmdUI* pCmdUI);
	afx_msg void OnProgressCancelClicked();
	afx_msg LRESULT OnMatEditSend(WPARAM, LPARAM);
	afx_msg void OnUpdateMemoryStatus( CCmdUI *pCmdUI );
	afx_msg void OnUpdateConsoleMemoryStatus( CCmdUI *pCmdUI );

	afx_msg void OnViewSavePaneLayout();
	afx_msg void OnViewRestoreDefaultLayout();
	afx_msg void OnViewLoadPaneLayout( UINT nID );
	afx_msg void OnViewSavePaneLayout( UINT nID );
	afx_msg void OnViewDeletePaneLayout( UINT nID );
	afx_msg void OnViewRenamePaneLayout( UINT nID );

	//afx_msg void OnFileMRURemove( UINT nID );
	
	DECLARE_MESSAGE_MAP()


	static void Command_Open_MaterialEditor();
	static void Command_Open_CharacterEditor();
	static void Command_Open_DataBaseView();
	static void Command_Open_TrackView();
	//static void Command_Open_LightmapCompiler();
	static void Command_Open_SelectObjects();
	static void Command_Open_FlowGraph();
	static void Command_Open_TimeOfDay();
	static void Command_Open_FacialEditor();
	static void Command_Open_SmartObjectsEditor();
	static void Command_Open_DialogSystemEditor();
	static void Command_Open_VisualLogViewer();
	static void Command_Open_TerrainEditor();
	static void Command_Open_TerrainTextureLayers();



	static void Command_Open_TextureBrowser();
	static void Command_Open_AssetBrowser();
	static void Command_Open_LNMEditor();

	static void Command_Close_MaterialEditor();
	static void Command_Close_CharacterEditor();
	static void Command_Close_DataBaseView();
	static void Command_Close_TrackView();
	static void Command_Close_LightmapCompiler();
	static void Command_Close_SelectObjects();
	static void Command_Close_FlowGraph();
	static void Command_Close_TimeOfDay();
	static void Command_Close_FacialEditor();
	static void Command_Close_SmartObjectsEditor();
	static void Command_Close_DialogSystemEditor();
	static void Command_Close_VisualLogViewer();
	static void Command_Close_TerrainEditor();
	static void Command_Close_TerrainTextureLayers();



	static void Command_Close_TextureBrowser();
	static void Command_Close_AssetBrowser();
	static void Command_Close_AnimActionEditor();
	static void Command_Close_LNMEditor();

protected:
	CXTPStatusBar	m_wndStatusBar;
	//CXTPStatusBarIconPane m_wndStatusIconPane;

	// Added this variable for quick access to this particular toolbar.
	CXTPToolBar *m_pEditToolBar;

	CReBar			m_wndReBar;
	CEditModeToolBar m_editModeBar;
	CXTPToolBar*		m_poConsoleToolbar;
	CToolBar m_wndToolBar;
	CToolBar m_objectModifyBar;
	CToolBar m_missionToolBar;
	CToolBar m_wndTerrainToolBar;
	CToolBar m_wndAvoToolBar;

	
	//////////////////////////////////////////////////////////////////////////

	CLayoutWnd *m_layoutWnd;
		
	//! Console dialog
	CConsoleSCB m_cConsole;

	// ToolBox
	CToolBoxDialog m_toolBoxDialog;

	// Rollup sizing bar
	CRollupBar m_wndRollUp;
	CRollupCtrl m_objectRollupCtrl;
	CRollupCtrl m_terrainRollupCtrl;
	CRollupCtrl m_modellingRollupCtrl;
	CRollupCtrl m_displayRollupCtrl;
	CRollupCtrl m_layersRollupCtrl;

	//CToolBar m_terrain;
	CXTFlatComboBox m_missions;
	//CComboBox m_missions;

	class CTerrainPanel* m_terrainPanel;
	class CMainTools* m_mainTools;

	CString		m_selectionName;
	//CDateTimeCtrl m_missionTime;
	CMission*	m_currentMission;
	class CObjectLayer *m_currentLayer;

	float m_gridSize;
	RefCoordSys m_coordSys;
	int m_objectSelectionMask;

	//! Saves mainframe position.
	CXTWindowPos m_wndPosition;

	int m_autoSaveTimer;
	int m_autoRemindTimer;
	int m_networkAuditionTimer;
	bool m_bAcceleratorsEnabled;

	CXTPDockingPaneManager m_paneManager;
	CXTPDockingPaneLayout* m_pDefaultLayout;

	std::vector<IViewPaneClass*> m_viewPaneClasses;

	//////////////////////////////////////////////////////////////////////////
	// ViewPane manager.
	//////////////////////////////////////////////////////////////////////////
	typedef std::map<int,SViewPaneDesc> PanesMap;
	PanesMap m_panesMap;
	
	struct SPaneHistory
	{
		CRect rect; // Last known size of this panel.
		XTPDockingPaneDirection dockDir; // Last known dock style.
	};
	typedef std::map<CString,SPaneHistory> PanesHistoryMap;
	PanesHistoryMap m_panesHistoryMap;

	// Names of available pane layout filenames in the Layouts folder.
	std::vector< CString > m_layoutFilenames;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__88B37B80_D04F_46F1_8FEF_A09696002A81__INCLUDED_)
