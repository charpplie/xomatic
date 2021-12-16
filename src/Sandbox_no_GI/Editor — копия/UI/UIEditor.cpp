////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEditor.cpp
//  Version:     v1.00
//  Created:     06/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UIEditor.h"

#include "UIManager.h"
#include "UIViewport.h"

#include "UITools.h"
#include "UIEmulateTools.h"

#include "UISettingsPanels.h"

#include "UIEmulatePanel.h"
#include "UIEmulateEventPanels.h"
#include "UIEmulateElementPanels.h"

#include "UIEventLogView.h"

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
namespace {
	struct SResolutionInfo
	{
		SResolutionInfo(int w, int h, const char* display) : W(w), H(h), Display(display) {}

		int W;
		int H;
		const char* Display;
	};

	struct SResolutionMan
	{
		static const SResolutionMan* Get()	{ static SResolutionMan inst;	return &inst; }
		int GetCount() const { return resList.size(); }
		const SResolutionInfo& GetRes(int i) const { if (i >= 0 && i < GetCount()) return resList[i]; return defRes; }

	private:
		SResolutionMan()
			: defRes(-1, -1, "Custom" )
		{
			resList.push_back( SResolutionInfo(-1, -1, "Custom" ) );
			resList.push_back( SResolutionInfo(800, 600, "800 x 600" ) );
			resList.push_back( SResolutionInfo(1024,768, "1024 x 768" ) );
			resList.push_back( SResolutionInfo(1151,720, "1151 x 720 (Console)" ) );
			resList.push_back( SResolutionInfo(1280,720, "1280 x 720" ) );
			resList.push_back( SResolutionInfo(1280,1050,"1280 x 1050" ) );
			resList.push_back( SResolutionInfo(1680,1050,"1680 x 1050" ) );
			resList.push_back( SResolutionInfo(1920,1080,"1920 x 1080" ) );

		}

		std::vector< SResolutionInfo > resList;;
		SResolutionInfo defRes;
	};
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
CryCriticalSection CUIEditor::m_sLock;

const char* UI_EDITOR_NAME     = "UI Emulator";
const char* UI_EDITOR_CATEGORY = "UIEditor";

IMPLEMENT_DYNCREATE( CUIEditor, CBaseFrameWnd )

BEGIN_MESSAGE_MAP( CUIEditor, CBaseFrameWnd )
	ON_WM_DESTROY()

	ON_COMMAND(ID_UI_MODE_FGUPDATE, OnToggleFGUpdate)
	ON_COMMAND(ID_UI_RELOAD_ALL, OnReloadAll)
	ON_COMMAND(ID_UI_RESET_STATE, OnResetState)
	ON_UPDATE_COMMAND_UI(ID_UI_RESOLUTIONS, OnUpdateResolutions)
	ON_UPDATE_COMMAND_UI(ID_UI_PLATFORM, OnUpdatePlatform)
	ON_COMMAND(ID_UI_CLEARLOG, OnClearLog)

	ON_XTP_EXECUTE(ID_UI_RESOLUTIONS, OnSelectionResolutionsChanged)
	ON_XTP_EXECUTE(ID_UI_PLATFORM, OnSelectionPlatformChanged)
	ON_XTP_CREATECONTROL()

END_MESSAGE_MAP()

const int ID_UI_EDITOR_PANEL = IDD_DATABASE;
const int ID_UI_RESOLUTIOPNS_BEGIN = 6000;

enum
{
	ID_UI_EDITOR_BASE = 60000,
	ID_UI_VIEWPORT,
	ID_UI_ELEMENTSROLLUP,
	ID_UI_EMULATEROLLUP,
	ID_UI_EVENTLOG,
};

enum EUIMainRollupControlIds
{
	UI_ROLLUP_ELEMENTS = 0,
	UI_ROLLUP_SETTINGS,
};

enum EUIDebugRollupControlIds
{
	UI_ROLLUP_EMULATE = 0,
	UI_ROLLUP_WATCHES,
};


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class CUIEditorViewClass 
	: public IViewPaneClass
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; }
	virtual REFGUID ClassID()
	{
		// {5648708B-5D54-4577-B4DF-ED28CDCF4542}
		
		static const GUID guid = { 0x5648708B, 0x5D54, 0x4577, { 0xB4, 0xDF, 0xED, 0x28, 0xCD, 0xCF, 0x45, 0x42 } };
		return guid;
	}
	virtual const char* ClassName() { return UI_EDITOR_NAME; }
	virtual const char* Category() { return UI_EDITOR_CATEGORY; }
	//////////////////////////////////////////////////////////////////////////

	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS( CUIEditor ); }
	virtual const char* GetPaneTitle() { return UI_EDITOR_NAME; }
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; }
	virtual CRect GetPaneRect() { return CRect( 200, 200, 800, 700 ); }
	virtual bool SinglePane() { return false; }
	virtual bool WantIdleUpdate() { return true; }
};

REGISTER_CLASS_DESC( CUIEditorViewClass )

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
CUIEditor::CUIEditor()
: m_pViewport(NULL)
, m_pMainToolButtons(NULL)
, m_pMainTool(NULL)
, m_pEventLog(NULL)
, m_LastChange(0)
, m_CurrPlattform(IFlashUI::ePUI_PC)
{
	CRect rc( 0, 0, 0, 0 );
	Create( WS_CHILD | WS_VISIBLE, rc, AfxGetMainWnd() );
}

////////////////////////////////////////////////////////////////////
CUIEditor::~CUIEditor()
{
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
BOOL CUIEditor::OnInitDialog()
{
	CryAutoCriticalSection lock(m_sLock);

	GetDockingPaneManager()->HideClient( TRUE );
	GetDockingPaneManager()->SetTheme( CMainFrame::GetDockingPaneTheme() );

	// create components
	CreateToolBars();
	CreateViewPort();
	CreateEventLog();
	CreateRollupBarObjects();
	CreateRollupBarEmulate();

	// add default Invoke panel
	AddEventEmuPanelInvoke( false );

	// load layout
	AutoLoadFrameLayout( UI_EDITOR_NAME );

	// register
	GetIEditor()->GetUIManager()->SetEditor(this);
	GetIEditor()->RegisterNotifyListener(this);

	// reset UISystem
	OnResetState();

	// register as log receiver
	gEnv->pFlashUI->SetEditorUILogEventCallback(functor(*this,&CUIEditor::OnUILogEvent));
	// register plattform callback
	gEnv->pFlashUI->SetEditorPlatformCallback(functor(*this,&CUIEditor::OnGetCurrentPlattform));

	m_pFileChangeMonitor = new CFileChangeMonitor;
	CString masterCD = Path::AddBackslash( CString(GetIEditor()->GetMasterCDFolder()) );
	m_pFileChangeMonitor->Subscribe(this);
	m_pFileChangeMonitor->MonitorItem( masterCD + Path::GetGameFolder() + "\\Libs\\UI\\" );

	return TRUE;
}

////////////////////////////////////////////////////////////////////
void CUIEditor::OnDestroy()
{
	if (m_pMainTool && GetIEditor()->GetEditTool() == m_pMainTool)
		GetIEditor()->SetEditTool(NULL);

	GetIEditor()->UnregisterNotifyListener(this);

	ClearEventEmuPanels( true );

	gEnv->pFlashUI->RemoveEditorUILogEventCallback();
	gEnv->pFlashUI->RemoveEditorPlatformCallback();

	if (GetIEditor()->GetUIManager()->GetEditor() == this)
		GetIEditor()->GetUIManager()->SetEditor(NULL);

	if (m_pFileChangeMonitor)
	{
		m_pFileChangeMonitor->StopMonitor();
		SAFE_DELETE( m_pFileChangeMonitor );
	}

	GetIEditor()->GetUIManager()->PromptChanges();
	gEnv->pFlashUI->Reload();
	gEnv->pFlashUI->EnableEventStack(true);

	CBaseFrameWnd::OnDestroy();
}

////////////////////////////////////////////////////////////////////
void CUIEditor::CreateToolBars()
{
	CXTPCommandBars* pCommandBars = GetCommandBars();
	if(pCommandBars == NULL)
	{
		TRACE0("Failed to create command bars object.\n");
		assert(false);
		return;
	}

	CXTPToolBar *pMainToolBar = pCommandBars->Add( _T("Main ToolBar"),xtpBarTop );
	pMainToolBar->SetVisible(true);
	pMainToolBar->EnableCustomization(FALSE);
	VERIFY(pMainToolBar->LoadToolBar(IDR_UIEDITOR_MAIN));
}

////////////////////////////////////////////////////////////////////
void CUIEditor::CreateViewPort()
{
	m_pViewport = new CUIViewport();
	m_pViewport->SetDlgCtrlID( ID_UI_VIEWPORT );
	m_pViewport->ModifyStyle( WS_POPUP,WS_CHILD,0 );
	m_pViewport->SetType( ET_ViewportUI );
	m_pViewport->SetParent( this );
	m_pViewport->SetOwner( this );
	m_pViewport->ShowWindow(SW_SHOW);

	AttachDockingWnd( ID_UI_VIEWPORT, m_pViewport, "UI Preview" );
}

////////////////////////////////////////////////////////////////////
void CUIEditor::CreateEventLog()
{
	m_pEventLog = new CUIEventLogView();
	m_pEventLog->Create(MAKEINTRESOURCE(CUIEventLogView::IDD),this);
	m_pEventLog->SetParent( this );
	m_pEventLog->SetOwner( this );

	AttachDockingWnd( ID_UI_EVENTLOG,m_pEventLog, "EventLog", xtpPaneDockBottom, GetDockingPaneManager()->FindPane(ID_UI_VIEWPORT) );
}

////////////////////////////////////////////////////////////////////
void CUIEditor::CreateRollupBarObjects()
{
	AttachDockingWnd( ID_UI_ELEMENTSROLLUP, m_rollupObjects.Create(this), "Main", xtpPaneDockLeft, GetDockingPaneManager()->FindPane(ID_UI_VIEWPORT) );

	CUIRollupControl* pageElements = m_rollupObjects.CreateControl(UI_ROLLUP_ELEMENTS, "Elements");
	m_pMainToolButtons = new CUIToolButtons();
	m_pMainToolButtons->Create(MAKEINTRESOURCE(CUIToolButtons::IDD),this);
	pageElements->AddPage("Objects", m_pMainToolButtons);

	m_SettingPanels.clear();
	CUISettingsPropertiesPanel* pSettingsDebug = new CUISettingsPropertiesPanelDebug();
	CUISettingsPropertiesPanel* pSettingsFlash = new CUISettingsPropertiesPanelFlashInfo();
	CUISettingsPropertiesPanel* pSettingsRender = new CUISettingsPropertiesPanelFlashRenderInfo();
	m_SettingPanels.push_back(pSettingsDebug);
	m_SettingPanels.push_back(pSettingsFlash);
	m_SettingPanels.push_back(pSettingsRender);

	CUIRollupControl* pageSettings = m_rollupObjects.CreateControl(UI_ROLLUP_SETTINGS, "Settings");
	pageSettings->AddPage("Display Settings", pSettingsDebug);
	pageSettings->AddPage("Flash Settings",   pSettingsFlash);
	pageSettings->AddPage("Render Settings",  pSettingsRender);

	m_rollupObjects.SelectRollUpBar( UI_ROLLUP_ELEMENTS );
}

////////////////////////////////////////////////////////////////////
void CUIEditor::CreateRollupBarEmulate()
{
	AttachDockingWnd( ID_UI_EMULATEROLLUP, m_rollupEmulate.Create(this), "Emulation", xtpPaneDockRight, GetDockingPaneManager()->FindPane(ID_UI_VIEWPORT) );

	CUIRollupControl* pageEmulate = m_rollupEmulate.CreateControl(UI_ROLLUP_EMULATE, "Emulate");
	CUIEmulateToolButtons* pEmulateTools = new CUIEmulateToolButtons();
	pEmulateTools->Create(MAKEINTRESOURCE(CUIEmulateToolButtons::IDD),this);
	pageEmulate->AddPage("Manage", pEmulateTools);

	// TODO: create watch panel!
// 	CUIRollupControl* pageDebug = m_rollupEmulate.CreateControl(UI_ROLLUP_WATCHES, "Watches");

	m_rollupEmulate.SelectRollUpBar( UI_ROLLUP_EMULATE );
}

////////////////////////////////////////////////////////////////////
void CUIEditor::AttachDockingWnd( UINT wndId, CWnd* pWnd, const CString& dockingPaneTitle, XTPDockingPaneDirection direction, CXTPDockingPaneBase* pNeighbour )
{
	assert( pWnd != NULL );
	if ( pWnd != NULL )
	{
		CRect rc( 0, 0, 400, 400 );

		CXTPDockingPane* pDockPane = CreateDockingPane(dockingPaneTitle, pWnd, wndId, rc, direction, pNeighbour );// pDockingPaneManager->CreatePane( wndId, rc, direction, pNeighbour );
		pDockPane->SetOptions( xtpPaneNoCloseable );

		pWnd->SetOwner( this );
	}
}

////////////////////////////////////////////////////////////////////
CUIRollupControl* CUIEditor::GetElementRollup()
{
	return m_rollupObjects.GetControl(UI_ROLLUP_ELEMENTS);
}

////////////////////////////////////////////////////////////////////
int CUIEditor::OnCreateControl(LPCREATECONTROLSTRUCT lpCreateControl)
{
	if (lpCreateControl->bToolBar)
	{
		if (lpCreateControl->nID == ID_UI_RESOLUTIONS)
		{
			CXTPControlComboBox* pComboBox = (CXTPControlComboBox*)CXTPControlComboBox::CreateObject();

			pComboBox->SetDropDownListStyle(TRUE);

			int id;
			for (int i = 0; i < SResolutionMan::Get()->GetCount(); ++i)
			{
				id = pComboBox->AddString(SResolutionMan::Get()->GetRes(i).Display);
				pComboBox->SetItemData( id, i);
			}

			pComboBox->SetCurSel(0);

			pComboBox->SetWidth(160);
			pComboBox->SetStyle(xtpComboNormal);
			lpCreateControl->pControl = pComboBox;
			return TRUE;
		}
		if (lpCreateControl->nID == ID_UI_PLATFORM)
		{
			CXTPControlComboBox* pComboBox = (CXTPControlComboBox*)CXTPControlComboBox::CreateObject();

			pComboBox->SetDropDownListStyle(TRUE);

			pComboBox->SetItemData( pComboBox->AddString("PC"), (int) IFlashUI::ePUI_PC);
			pComboBox->SetItemData( pComboBox->AddString("X360"), (int) IFlashUI::ePUI_X360);
			pComboBox->SetItemData( pComboBox->AddString("PS3"), (int) IFlashUI::ePUI_PS3);
#if !defined(IS_CRYDEV) && !defined(CRYTEK_SDK_EVALUATION)
			pComboBox->SetItemData( pComboBox->AddString("gringo"), (int) IFlashUI::ePUI_gringo);
#endif

			pComboBox->SetCurSel(0);

			pComboBox->SetWidth(60);
			pComboBox->SetStyle(xtpComboNormal);
			lpCreateControl->pControl = pComboBox;
			return TRUE;
		}
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void CUIEditor::OnToggleFGUpdate()
{
	if (m_pViewport)
	{
		m_pViewport->SetRegularUpdateFg( !m_pViewport->IsRegularUpdateFg() );
		CXTPControlButton* pControl = (CXTPControlButton*)GetCommandBars()->FindControl(xtpControlButton,ID_UI_MODE_FGUPDATE,TRUE,TRUE);
		if (pControl)
		{
			pControl->SetChecked( m_pViewport->IsRegularUpdateFg() );
		}
	}
}

////////////////////////////////////////////////////////////////////
void CUIEditor::OnReloadAll()
{
	CWaitCursor wait;

	if (m_pMainTool)
		m_pMainTool->CloseTools();
	if (m_pMainToolButtons) 
		m_pMainToolButtons->UncheckAll();

	gEnv->pConsole->ExecuteString("gfx_reload_all");

	for (TEventEmulatePanels::iterator it = m_EventPanels.begin(); it != m_EventPanels.end(); ++it)
		it->first->Refresh();

	OnResetState();
}

////////////////////////////////////////////////////////////////////
void CUIEditor::OnResetState()
{
	m_LastChange = 0;

	gEnv->pFlashUI->Reload();
	if (m_pViewport && m_pViewport->IsRegularUpdateFg()) gEnv->pFlashUI->UpdateFG(); // flush UIFlowgraphs if enabled

	const int count = gEnv->pFlashUI->GetUIElementCount();
	for (int i = 0; i < count; ++i)
	{
		IUIElement* pElement = gEnv->pFlashUI->GetUIElement(i);
		pElement->Unload(true);
	}

	if (m_pEventLog)
		m_pEventLog->ClearLog();
}

////////////////////////////////////////////////////////////////////
void CUIEditor::OnUpdateResolutions(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
	CRect rc;
	m_pViewport->GetWindowRect(&rc);
	for (int i = 0; i < SResolutionMan::Get()->GetCount(); i++)
	{
		const SResolutionInfo& info = SResolutionMan::Get()->GetRes(i);
		if (info.W == rc.Width() && info.H == rc.Height())
		{
			pCmdUI->SetText( info.Display );
			return;
		}
	}
	pCmdUI->SetText( SResolutionMan::Get()->GetRes(0).Display );
}

////////////////////////////////////////////////////////////////////
void CUIEditor::OnSelectionResolutionsChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	CXTPControlComboBox* pControl = (CXTPControlComboBox*)GetCommandBars()->FindControl(xtpControlComboBox,ID_UI_RESOLUTIONS,TRUE,TRUE);
	if (!pControl)
		return;

	int sel = pControl->GetCurSel();

	if (sel != LB_ERR)
	{
		CXTPDockingPane* pPane = GetDockingPaneManager()->FindPane(ID_UI_VIEWPORT);
		if (m_pViewport && pPane)
		{
			const SResolutionInfo& info = SResolutionMan::Get()->GetRes(sel);
		if (info.W > 0 && info.H > 0)
			{
				pPane->SetMinTrackSize(CSize(info.W, info.H));
				pPane->SetMaxTrackSize(CSize(info.W, info.H));
				CRect rc;
				m_pViewport->GetWindowRect(&rc);
				m_pViewport->SetWindowPos( NULL, 0, 0, (long)info.W, (long)info.H, SWP_NOMOVE );
				pPane->InvalidatePane(TRUE);
				pPane->RedrawPane();
			}
			else
			{
				pPane->SetMinTrackSize(CSize(100, 100));
				pPane->SetMaxTrackSize(CSize(9999, 9999));
			}
		}
	}

}

////////////////////////////////////////////////////////////////////
void CUIEditor::OnUpdatePlatform(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

////////////////////////////////////////////////////////////////////
void CUIEditor::OnSelectionPlatformChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	CXTPControlComboBox* pControl = (CXTPControlComboBox*)GetCommandBars()->FindControl(xtpControlComboBox,ID_UI_PLATFORM,TRUE,TRUE);
	if (!pControl)
		return;

	int sel = pControl->GetCurSel();

	if (sel != LB_ERR)
	{
		m_CurrPlattform = (IFlashUI::EPlatformUI)sel;
	}

	OnResetState();
}

////////////////////////////////////////////////////////////////////
void CUIEditor::OnClearLog()
{
	if (m_pEventLog)
		m_pEventLog->ClearLog();
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void CUIEditor::OnFileMonitorChange()
{
	while (m_pFileChangeMonitor->HaveModifiedFiles())
	{
		SFileChangeInfo change;
		char ext[_MAX_EXT];

		if(m_pFileChangeMonitor->PopNextFileChange(change))
		{
			_splitpath( change.filename,NULL,NULL,NULL, ext );
			if (CUIManager::CV_gfx_FlashReloadEnabled && (strcmpi(ext,".gfx") == 0 || strcmpi(ext,".swf") == 0) )
			{
 				m_LastChange = gEnv->pTimer->GetAsyncCurTime();
			}
		}
	}
}

////////////////////////////////////////////////////////////////////
void CUIEditor::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if (event == eNotify_OnIdleUpdate)
	{
		float currTime = gEnv->pTimer->GetAsyncCurTime();
		if (m_LastChange != 0 && m_LastChange + CUIManager::CV_gfx_FlashReloadTime < currTime)
		{
			gEnv->pFlashUI->Reload();
			if (m_pViewport)
			{
				m_pViewport->Update();
			}
			m_LastChange = 0;
		}

		if (m_pMainTool)
			m_pMainTool->UpdateTools();
		for (TSettingPanels::iterator it = m_SettingPanels.begin(); it != m_SettingPanels.end(); ++it)
			(*it)->UpdateChanges();
	}

}

////////////////////////////////////////////////////////////////////
BOOL CUIEditor::PreTranslateMessage(MSG* pMsg)
{
	float currTime = gEnv->pTimer->GetAsyncCurTime();
	if (m_LastChange != 0 && m_LastChange + CUIManager::CV_gfx_FlashReloadTime < currTime)
	{
		gEnv->pFlashUI->Reload();
		if (m_pViewport)
		{
			m_pViewport->Update();
		}
		m_LastChange = 0;
	}
	return __super::PreTranslateMessage(pMsg);
}

////////////////////////////////////////////////////////////////////
void CUIEditor::OnUILogEvent(const IFlashUI::SUILogEvent& evt)
{
	if (m_pEventLog)
		m_pEventLog->AddLogEvent(evt);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void CUIEditor::AddEventEmuPanelInvoke( bool bRemoveable /*= true*/ )
{
	CUIDirectInvokePanel* pNewPanel = new CUIDirectInvokePanel(bRemoveable);
	AddEventEmuPanel( pNewPanel, "Direct Invoke" );
}

////////////////////////////////////////////////////////////////////
void CUIEditor::AddEventEmuPanelElement( IUIElement* pElement, const SUIEventDesc& evt )
{
 	CUIEmulateUIElementEventPanel* pNewPanel = new CUIEmulateUIElementEventPanel(pElement, evt);
	AddEventEmuPanel( pNewPanel, string().Format("%s : %s", pElement->GetName(), evt.sDisplayName).c_str() );
}

////////////////////////////////////////////////////////////////////
void CUIEditor::AddEventEmuPanelAction( IUIAction* pAction )
{
	CUIEmulateUIActionEventPanel* pNewPanel = new CUIEmulateUIActionEventPanel(pAction);
	AddEventEmuPanel( pNewPanel, pAction->GetName() );
}

////////////////////////////////////////////////////////////////////
void CUIEditor::AddEventEmuPanelEventSystem( IUIEventSystem* pEventSystem, const SUIEventDesc& evt )
{
	CUIEmulateUIEventSystemEventPanel* pNewPanel = new CUIEmulateUIEventSystemEventPanel(pEventSystem, evt);
	AddEventEmuPanel( pNewPanel, string().Format("%s : %s", pEventSystem->GetName(), evt.sDisplayName).c_str() );
}

////////////////////////////////////////////////////////////////////
void CUIEditor::AddEventEmuPanelElementVar( IUIElement* pElement, const SUIParameterDesc& desc )
{
	CUIEmulateUIElementVarPanel* pNewPanel = new CUIEmulateUIElementVarPanel(pElement, desc);
	AddEventEmuPanel( pNewPanel, string().Format("%s : %s", pElement->GetName(), desc.sDisplayName).c_str() );
}

////////////////////////////////////////////////////////////////////
void CUIEditor::AddEventEmuPanelElementArray( IUIElement* pElement, const SUIParameterDesc& desc )
{
	CUIEmulateUIElementArrayPanel* pNewPanel = new CUIEmulateUIElementArrayPanel(pElement, desc);
	AddEventEmuPanel( pNewPanel, string().Format("%s : %s", pElement->GetName(), desc.sDisplayName).c_str() );
}

////////////////////////////////////////////////////////////////////
void CUIEditor::AddEventEmuPanelElementMovieClip( IUIElement* pElement, const SUIParameterDesc& desc )
{
// 	CUIEmulateUIElementMCPanel* pNewPanel = new CUIEmulateUIElementMCPanel(pElement, desc);
// 	AddEventEmuPanel( pNewPanel, string().Format("%s : %s", pElement->GetName(), desc.sDisplayName).c_str() );
}

////////////////////////////////////////////////////////////////////
void CUIEditor::AddEventEmuPanelsForElement( IUIElement* pElement )
{
	for (int i = 0; i < pElement->GetFunctionCount(); ++i)
	{
		AddEventEmuPanelElement(pElement, *pElement->GetFunctionDesc(i));
	}
}

////////////////////////////////////////////////////////////////////
void CUIEditor::AddEventEmuPanelsForEventSystem( IUIEventSystem* pEventSystem )
{
	for (int i = 0; i < pEventSystem->GetEventCount(); ++i)
	{
		AddEventEmuPanelEventSystem(pEventSystem, *pEventSystem->GetEventDesc(i));
	}
}

////////////////////////////////////////////////////////////////////
void CUIEditor::RemoveEventEmuPanel( CUIEmulatePanel* pPanel )
{
	TEventEmulatePanels::iterator it = m_EventPanels.find(pPanel); 
	if (it != m_EventPanels.end())
	{
		CUIRollupControl* pageEmulate = m_rollupEmulate.GetControl(UI_ROLLUP_EMULATE);
		assert(pageEmulate);
		pageEmulate->RemovePage(it->second);
		m_EventPanels.erase(it);
	}
}

////////////////////////////////////////////////////////////////////
void CUIEditor::RemoveEventEmuPanelsForGroup( void* pGroup )
{
	TEventEmulatePanels panels = m_EventPanels;
	for (TEventEmulatePanels::iterator it = panels.begin(); it != panels.end(); ++it)
	{
		if (it->first->IsRemoveAble() && it->first->GetData() == pGroup)
			RemoveEventEmuPanel( it->first );
	}
}

////////////////////////////////////////////////////////////////////
void CUIEditor::ClearEventEmuPanels( bool force /*= false*/ )
{
	TEventEmulatePanels panels = m_EventPanels;
	for (TEventEmulatePanels::iterator it = panels.begin(); it != panels.end(); ++it)
	{
		if (it->first->IsRemoveAble() || force)
			RemoveEventEmuPanel( it->first );
	}
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void CUIEditor::AddEventEmuPanel( CUIEmulatePanel* pPanel, const char* name )
{
	CUIRollupControl* pageEmulate = m_rollupEmulate.GetControl(UI_ROLLUP_EMULATE);
	assert(pageEmulate);
	m_EventPanels[pPanel] = pageEmulate->AddPage(name, pPanel);
	m_rollupEmulate.SelectRollUpBar( UI_ROLLUP_EMULATE );
}

