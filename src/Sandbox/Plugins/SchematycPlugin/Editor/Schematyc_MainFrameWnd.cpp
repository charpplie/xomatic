/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc main frame window.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_MainFrameWnd.h"

#include <QBoxLayout>
#include <Include/IPropertyTree.h>
#include <Include/IViewPane.h>
#include <Schematyc/Schematyc_IDocManager.h>
#include <Schematyc/Schematyc_ILib.h>

class CTrackViewAnimNode;

#include "../Editor/Util/XmlArchive.h"
#include "../Editor/Objects/EntityObject.h"
#include "../EditorCommon/QParentWndWidget.h"
#include "../EditorCommon/QViewport.h"
#include "../EditorCommon/QViewportSettings.h"

#include "Resource.h"
#include "Schematyc_BrowserIcons.h"
#include "Schematyc_PluginUtils.h"
#include "Schematyc_PropertyTreeDlg.h"

// TODO : Find a less hacky way to set/get c_vars?

//////////////////////////////////////////////////
/*
#include <IResourceSelectorHost.h>

#include "Schematyc_QuickSearchDlg.h"

typedef std::vector<string> TStringVector;

dll_string SearchStringListSelector(const SResourceSelectorContext& context, const char* previousValue, Serialization::StringListValue* pStringListValue)
{
	class CQuickSearchOptions : public Schematyc::IQuickSearchOptions
	{
	public:

		// IQuickSearchOptions

		virtual size_t GetCount() const
		{
			return m_options.size();
		}

		virtual const char* GetName(size_t iOption) const
		{
			return iOption < m_options.size() ? m_options[iOption].c_str() : "";
		}

		// ~IQuickSearchOptions

		void AddOption(const char* option)
		{
			CRY_ASSERT(option != NULL);
			if(option != NULL)
			{
				m_options.push_back(option);
			}
		}

	private:

		TStringVector	m_options;
	};

	CPoint	cursorPos;
	GetCursorPos(&cursorPos);
	CQuickSearchOptions	quickSearchOptions;
	quickSearchOptions.AddOption("One");
	quickSearchOptions.AddOption("Two");
	quickSearchOptions.AddOption("Three");
	quickSearchOptions.AddOption("Ext::Four");
	quickSearchOptions.AddOption("Ext::Five");
	quickSearchOptions.AddOption("Ext::Six");
	SET_LOCAL_RESOURCE_SCOPE
	Schematyc::CQuickSearchDialog	quickSearchDialog(CWnd::FromHandle(context.parentWindow), CPoint(cursorPos.x - 100, cursorPos.y - 100), quickSearchOptions);
	if(quickSearchDialog.DoModal() == IDOK)
	{
		return quickSearchOptions.GetName(quickSearchDialog.GetSelectedOption());
	}
	else
	{
		return "";
	}
}

REGISTER_RESOURCE_SELECTOR("SearchStringList", SearchStringListSelector, "Editor/Icons/GameDatabase/Show_In_Explorer.png")
*/
//////////////////////////////////////////////////

namespace
{
	//////////////////////////////////////////////////////////////////////////
	// TODO : Move to Schematyc_PluginUtils.h?
	EntityId GetSelectedEntityId()
	{
		CBaseObject*	pSelectedObject = ::GetIEditor()->GetSelectedObject();
		if(pSelectedObject != NULL)
		{
			if(pSelectedObject->IsKindOf(RUNTIME_CLASS(CEntityObject)) == TRUE)
			{
				IEntity*	pSelectedEntity = static_cast<CEntityObject*>(pSelectedObject)->GetIEntity();
				if(pSelectedEntity != NULL)
				{
					return pSelectedEntity->GetId();
				}
			}
		}
		return 0;
	}
}

namespace Schematyc
{
	namespace
	{
		static const UINT		IDW_SCHEMATYC_BROWSER_PANE					= AFX_IDW_PANE_FIRST + 1;
		static const UINT		IDW_SCHEMATYC_ENV_BROWSER_PANE			= AFX_IDW_PANE_FIRST + 2;
		static const UINT		IDW_SCHEMATYC_DETAIL_PANE						= AFX_IDW_PANE_FIRST + 3;
		static const UINT		IDW_SCHEMATYC_GRAPH_PANE						= AFX_IDW_PANE_FIRST + 4;
		static const UINT		IDW_SCHEMATYC_LOG_OUTPUT_PANE_A			= AFX_IDW_PANE_FIRST + 5;
		static const UINT		IDW_SCHEMATYC_LOG_OUTPUT_PANE_B			= AFX_IDW_PANE_FIRST + 6;
		static const UINT		IDW_SCHEMATYC_LOG_OUTPUT_PANE_C			= AFX_IDW_PANE_FIRST + 7;
		static const UINT		IDW_SCHEMATYC_LOG_OUTPUT_PANE_D			= AFX_IDW_PANE_FIRST + 8;
		static const UINT		IDW_SCHEMATYC_COMPILER_OUTPUT_PANE	= AFX_IDW_PANE_FIRST + 9;
		static const UINT		IDW_SCHEMATYC_VIEWPORT_PANE					= AFX_IDW_PANE_FIRST + 10;

		static const UINT		IDC_SCHEMATYC_BROWSER								= 1;
		static const UINT		IDC_SCHEMATYC_ENV_BROWSER						= 2;
		static const UINT		IDC_SCHEMATYC_DETAIL								= 3;
		static const UINT		IDC_SCHEMATYC_GRAPH									= 4;
		static const UINT		IDC_SCHEMATYC_LOG_OUTPUT_A					= 5;
		static const UINT		IDC_SCHEMATYC_LOG_OUTPUT_B					= 6;
		static const UINT		IDC_SCHEMATYC_LOG_OUTPUT_C					= 7;
		static const UINT		IDC_SCHEMATYC_LOG_OUTPUT_D					= 8;
		static const UINT		IDC_SCHEMATYC_COMPILER_OUTPUT				= 9;
		static const UINT		IDC_SCHEMATYC_VIEWPORT							= 10;

		static const char*	MAIN_WND_CLASS_NAME									= "Schematyc";
		static const char*	LAYOUT_FOLDER_NAME									= "Config";
		static const char*	LAYOUT_FILE_NAME										= "SchematycPlugin.xml";
		static const char*	LAYOUT_SECTION											= "Layout";
	}

	//////////////////////////////////////////////////////////////////////////
	// TODO : Ideally this class would derive from TRef		Counted base rather
	//        than implementing it's own reference counting, but that causes
	//        link errors.
	class CMainViewClass : public IViewPaneClass
	{
	public:

		inline CMainViewClass()
			: m_refCount(0)
		{}

		inline ULONG AddRef()
		{
			++ m_refCount;
			return m_refCount;
		};

		inline ULONG Release()
		{
			const int32	refCount = -- m_refCount;
			if(refCount <= 0)
			{
				delete this;
			}
			return refCount;
		}

		virtual ESystemClassID SystemClassID()
		{
			return ESYSTEM_CLASS_VIEWPANE;
		};

		virtual REFGUID ClassID()
		{
			static const GUID guid = { 0x530840ee, 0x6f1c, 0x4319, { 0x8a, 0xa2, 0xd9, 0xb5, 0x4e, 0xde, 0x43, 0x8a } };
			return guid;
		}

		virtual const char* ClassName()
		{
			return MAIN_WND_CLASS_NAME;
		};

		virtual const char* Category()
		{
			return "Schematyc";
		};

		virtual CRuntimeClass* GetRuntimeClass()
		{
			return RUNTIME_CLASS(CMainFrameWnd);
		};

		virtual const char* GetPaneTitle()
		{
			return _T("Schematyc");
		};

		virtual EDockingDirection GetDockingDirection()
		{
			return DOCK_FLOAT;
		};

		virtual CRect GetPaneRect()
		{
			return CRect(200, 200, 600, 500);
		};

		virtual bool SinglePane()
		{
			return false;
		};

		virtual bool WantIdleUpdate()
		{
			return true;
		};

	private:

		int32	m_refCount;
	};

	//////////////////////////////////////////////////////////////////////////
	CViewportMessageHandler::CViewportMessageHandler()
		: m_pViewportWnd(NULL)
	{}

	//////////////////////////////////////////////////////////////////////////
	void CViewportMessageHandler::Connect(CViewportWnd* pViewportWnd, QViewport* pViewport)
	{
		m_pViewportWnd = pViewportWnd;
		connect(pViewport, SIGNAL(SignalRender(const SRenderContext&)), SLOT(OnRender(const SRenderContext&)));
	}

	//////////////////////////////////////////////////////////////////////////
	void CViewportMessageHandler::OnRender(const SRenderContext& context)
	{
		if(m_pViewportWnd != NULL)
		{
			m_pViewportWnd->OnRender(context);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	BEGIN_MESSAGE_MAP(CViewportWnd, CWnd)
		ON_WM_SIZE()
	END_MESSAGE_MAP()

	//////////////////////////////////////////////////////////////////////////
	CViewportWnd::CViewportWnd()
		: m_pParentWndWidget(NULL)
		, m_pViewport(NULL)
	{}

	//////////////////////////////////////////////////////////////////////////
	CViewportWnd::~CViewportWnd()
	{
		SetPreview(NULL);
		SAFE_DELETE(m_pParentWndWidget);
	}

	//////////////////////////////////////////////////////////////////////////
	void CViewportWnd::Init()
	{
		m_pParentWndWidget	= new QParentWndWidget(GetSafeHwnd());
		m_pViewport					= new QViewport(gEnv, m_pParentWndWidget);
		m_messageHandler.Connect(this, m_pViewport);
	}

	//////////////////////////////////////////////////////////////////////////
	void CViewportWnd::InitLayout()
	{
		// Creating layout to handle resizing of the m_pViewport.
		QBoxLayout*	pLayout = new QBoxLayout(QBoxLayout::TopToBottom);
		pLayout->setContentsMargins(0, 0, 0, 0);
		pLayout->addWidget(m_pViewport, 1);
		m_pParentWndWidget->setLayout(pLayout);
		m_pParentWndWidget->show();
		// Initialize viewport settings.
		SViewportSettings	viewportSettings = m_pViewport->GetSettings();
		viewportSettings.debug.fps				= false;
		viewportSettings.grid.showGrid		= true;
		viewportSettings.grid.spacing			= 1.0f;
		viewportSettings.camera.moveSpeed	= 0.2f;
		m_pViewport->SetSettings(viewportSettings);
		m_pViewport->SetSceneDimensions(Vec3(100.0f, 100.0f, 100.0f));
		Update();
	}

	//////////////////////////////////////////////////////////////////////////
	void CViewportWnd::Update()
	{
		if((m_pParentWndWidget != NULL) && (m_pViewport != NULL))
		{
			CRect	rect; 
			CWnd::GetClientRect(&rect);
			m_pParentWndWidget->setGeometry(0, 0, rect.Width(), rect.Height());
			m_pViewport->Update();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CViewportWnd::SetPreview(const IDocSchemaConstPtr& pSchema)
	{
		if(m_pFoundationPreviewExtension != NULL)
		{
			m_pFoundationPreviewExtension->EndPreview();
			m_schemaGUID									= SGUID();
			m_pFoundationPreviewExtension = NULL;
		}
		if(pSchema != NULL)
		{
			IFoundationConstPtr	pFoundation = GetSchematycFramework().GetEnvRegistry().GetFoundation(pSchema->GetFoundationGUID());
			CRY_ASSERT(pFoundation != NULL);
			if(pFoundation != NULL)
			{
				m_pFoundationPreviewExtension = pFoundation->QueryExtension<IFoundationPreviewExtension>();
				if(m_pFoundationPreviewExtension != NULL)
				{
					m_schemaGUID = pSchema->GetGUID();
					m_pFoundationPreviewExtension->BeginPreview(m_schemaGUID, GetSelectedEntityId());
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CViewportWnd::ResetPreview()
	{
		if(m_pFoundationPreviewExtension != NULL)
		{
			m_pFoundationPreviewExtension->EndPreview();
			m_pFoundationPreviewExtension->BeginPreview(m_schemaGUID, GetSelectedEntityId());
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CViewportWnd::OnRender(const SRenderContext& context)
	{
		if(m_pFoundationPreviewExtension != NULL)
		{
			m_pFoundationPreviewExtension->RenderPreview(*context.renderParams, *context.passInfo);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CViewportWnd::OnSize(UINT nType, int cx, int cy)
	{
		Update();
	}

	//////////////////////////////////////////////////////////////////////////
	IMPLEMENT_DYNCREATE(CMainFrameWnd, CXTPFrameWnd)

	//////////////////////////////////////////////////////////////////////////
	BEGIN_MESSAGE_MAP(CMainFrameWnd, CXTPFrameWnd)
		
		ON_COMMAND_EX(ID_SCHEMATYC_VIEW_BROWSER, OnViewMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_VIEW_ENV_BROWSER, OnViewMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_VIEW_DETAIL, OnViewMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_VIEW_GRAPH, OnViewMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_VIEW_LOG_OUTPUT_A, OnViewMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_VIEW_LOG_OUTPUT_B, OnViewMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_VIEW_LOG_OUTPUT_C, OnViewMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_VIEW_LOG_OUTPUT_D, OnViewMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_VIEW_COMPILER_OUTPUT, OnViewMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_VIEW_VIEWPORT, OnViewMenuCommand)

		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_VIEW_BROWSER, OnUpdateViewMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_VIEW_ENV_BROWSER, OnUpdateViewMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_VIEW_DETAIL, OnUpdateViewMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_VIEW_GRAPH, OnUpdateViewMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_VIEW_LOG_OUTPUT_A, OnUpdateViewMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_VIEW_LOG_OUTPUT_B, OnUpdateViewMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_VIEW_LOG_OUTPUT_C, OnUpdateViewMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_VIEW_LOG_OUTPUT_D, OnUpdateViewMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_VIEW_COMPILER_OUTPUT, OnUpdateViewMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_VIEW_VIEWPORT, OnUpdateViewMenuItem)

		ON_COMMAND(ID_SCHEMATYC_TOOLS_CONFIGURE_LOG_STREAMS, OnToolsConfigureLogStreams)
		
		ON_COMMAND_EX(ID_SCHEMATYC_TOOLS_CONFIGURE_LOG_OUTPUT_A, OnToolsConfigureLogOutput)
		ON_COMMAND_EX(ID_SCHEMATYC_TOOLS_CONFIGURE_LOG_OUTPUT_B, OnToolsConfigureLogOutput)
		ON_COMMAND_EX(ID_SCHEMATYC_TOOLS_CONFIGURE_LOG_OUTPUT_C, OnToolsConfigureLogOutput)
		ON_COMMAND_EX(ID_SCHEMATYC_TOOLS_CONFIGURE_LOG_OUTPUT_D, OnToolsConfigureLogOutput)

		ON_COMMAND_EX(ID_SCHEMATYC_WORLD_DEBUG_SHOW_STATES, OnWorldDebugMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_WORLD_DEBUG_SHOW_VARIABLES, OnWorldDebugMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_WORLD_DEBUG_SHOW_CONTAINERS, OnWorldDebugMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_WORLD_DEBUG_SHOW_TIMERS, OnWorldDebugMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_WORLD_DEBUG_SHOW_ACTIONS, OnWorldDebugMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_WORLD_DEBUG_SHOW_SIGNAL_HISTORY, OnWorldDebugMenuCommand)
		
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_WORLD_DEBUG_SHOW_STATES, OnUpdateWorldDebugMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_WORLD_DEBUG_SHOW_VARIABLES, OnUpdateWorldDebugMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_WORLD_DEBUG_SHOW_CONTAINERS, OnUpdateWorldDebugMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_WORLD_DEBUG_SHOW_TIMERS, OnUpdateWorldDebugMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_WORLD_DEBUG_SHOW_ACTIONS, OnUpdateWorldDebugMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_WORLD_DEBUG_SHOW_SIGNAL_HISTORY, OnUpdateWorldDebugMenuItem)

		ON_COMMAND(ID_SCHEMATYC_DEBUG_SELECTED_ENTITY, OnDebugSelectedEntity)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_DEBUG_SELECTED_ENTITY, OnUpdateDebugSelectedEntity)

		ON_COMMAND_EX(ID_SCHEMATYC_ENTITY_DEBUG_SHOW_STATES, OnEntityDebugMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_ENTITY_DEBUG_SHOW_VARIABLES, OnEntityDebugMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_ENTITY_DEBUG_SHOW_CONTAINERS, OnEntityDebugMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_ENTITY_DEBUG_SHOW_TIMERS, OnEntityDebugMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_ENTITY_DEBUG_SHOW_ACTIONS, OnEntityDebugMenuCommand)
		ON_COMMAND_EX(ID_SCHEMATYC_ENTITY_DEBUG_SHOW_SIGNAL_HISTORY, OnEntityDebugMenuCommand)

		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_ENTITY_DEBUG_SHOW_STATES, OnUpdateEntityDebugMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_ENTITY_DEBUG_SHOW_VARIABLES, OnUpdateEntityDebugMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_ENTITY_DEBUG_SHOW_CONTAINERS, OnUpdateEntityDebugMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_ENTITY_DEBUG_SHOW_TIMERS, OnUpdateEntityDebugMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_ENTITY_DEBUG_SHOW_ACTIONS, OnUpdateEntityDebugMenuItem)
		ON_UPDATE_COMMAND_UI(ID_SCHEMATYC_ENTITY_DEBUG_SHOW_SIGNAL_HISTORY, OnUpdateEntityDebugMenuItem)

		ON_COMMAND(ID_SCHEMATYC_SAVE, OnSave)
		ON_COMMAND(ID_SCHEMATYC_REFRESH_ENV, OnRefreshEnv)
		ON_MESSAGE(XTPWM_DOCKINGPANE_NOTIFY, OnDockingPaneNotify)

		ON_WM_DESTROY()

	END_MESSAGE_MAP()

	//////////////////////////////////////////////////////////////////////////
	CMainFrameWnd::CMainFrameWnd()
		: m_pGraphView(new CDocGraphView())
		, m_pDetailPropertyTree(NULL)
		, m_pDetailDoc(NULL)
		, m_pViewportWnd(NULL)
	{ 
		const HINSTANCE	hInstance = AfxGetInstanceHandle();
		WNDCLASS				wndClass;
		if(!GetClassInfo(hInstance, MAIN_WND_CLASS_NAME, &wndClass))
		{
			wndClass.style					= CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
			wndClass.lpfnWndProc		= ::DefWindowProc;
			wndClass.cbClsExtra			= 0;
			wndClass.cbWndExtra			= 0;
			wndClass.hInstance			= hInstance;
			wndClass.hIcon					= NULL;
			wndClass.hCursor				= AfxGetApp()->LoadStandardCursor(IDC_ARROW);
			wndClass.hbrBackground	= (HBRUSH)(COLOR_3DFACE + 1);
			wndClass.lpszMenuName		= NULL;
			wndClass.lpszClassName	= MAIN_WND_CLASS_NAME;
			if(!AfxRegisterClass(&wndClass))
			{
				AfxThrowResourceException();
			}
		}
		GetIEditor()->RegisterNotifyListener(this);
		if(Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), AfxGetMainWnd()))
		{
			OnInitDialog();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	CMainFrameWnd::~CMainFrameWnd()
	{
		GetIEditor()->UnregisterNotifyListener(this);
		SAFE_RELEASE(m_pDetailPropertyTree);
		SAFE_DELETE(m_pViewportWnd);
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CMainFrameWnd::PreCreateWindow(CREATESTRUCT& cs)
	{
		if(__super::PreCreateWindow(cs))
		{
			cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
			return TRUE;
		}
		return FALSE;
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CMainFrameWnd::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd)
	{
		return __super::Create(MAIN_WND_CLASS_NAME, "", dwStyle, rect, pParentWnd);
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnEditorNotifyEvent(EEditorNotifyEvent event)
	{
		switch(event)
		{
		case eNotify_OnIdleUpdate:
			{
				m_pViewportWnd->Update();
				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CMainFrameWnd::OnInitDialog()
	{
		SET_LOCAL_RESOURCE_SCOPE
		// Set window style.
		ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
		ModifyStyle(0, WS_CLIPCHILDREN);
		// Create menu and toolbar.
		if(!CreateMenuAndToolbar())
		{
			return FALSE;
		}
		// Create child windows.
		m_browserCtrl.Create(WS_CHILD, CRect(0, 0, 0, 0), this, IDC_SCHEMATYC_BROWSER);
		m_envBrowserCtrl.Create(WS_CHILD, CRect(0, 0, 0, 0), this, IDC_SCHEMATYC_ENV_BROWSER);
		m_pGraphView->Create(WS_CHILD, CRect(0, 0, 0, 0), this, IDC_SCHEMATYC_GRAPH);
		m_compilerOutputCtrl.Create(WS_CHILD | WS_VSCROLL | ES_MULTILINE, CRect(0, 0, 0, 0), this, IDC_SCHEMATYC_COMPILER_OUTPUT);
		m_logOutputCtrlA.Create(WS_CHILD | WS_VSCROLL | ES_MULTILINE, CRect(0, 0, 0, 0), this, IDC_SCHEMATYC_LOG_OUTPUT_A);
		m_logOutputCtrlB.Create(WS_CHILD | WS_VSCROLL | ES_MULTILINE, CRect(0, 0, 0, 0), this, IDC_SCHEMATYC_LOG_OUTPUT_B);
		m_logOutputCtrlC.Create(WS_CHILD | WS_VSCROLL | ES_MULTILINE, CRect(0, 0, 0, 0), this, IDC_SCHEMATYC_LOG_OUTPUT_C);
		m_logOutputCtrlD.Create(WS_CHILD | WS_VSCROLL | ES_MULTILINE, CRect(0, 0, 0, 0), this, IDC_SCHEMATYC_LOG_OUTPUT_D);
		m_pViewportWnd = new CViewportWnd();
		m_pViewportWnd->Create(NULL, "Viewport", WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_SCHEMATYC_VIEWPORT);
		m_pViewportWnd->Init();
		// Create panes.
		CreatePanes();
		// Connect signal receivers.
		m_browserCtrl.GetSelectionSignal().Connect(MAKE_MEMBER_DELEGATE(CMainFrameWnd::OnBrowserSelection, *this), m_signalScopes.browserSelection);
		m_browserCtrl.GetDocModifiedSignal().Connect(MAKE_MEMBER_DELEGATE(CMainFrameWnd::OnBrowserDocModified, *this), m_signalScopes.browserDocModified);
		m_browserCtrl.GetDocGraphRemovedSignal().Connect(MAKE_MEMBER_DELEGATE(CMainFrameWnd::OnBrowserDocGraphRemoved, *this), m_signalScopes.browserDocGraphRemoved);
		m_pGraphView->GetNodeSelectionSignal().Connect(MAKE_MEMBER_DELEGATE(CMainFrameWnd::OnGraphViewNodeSelection, *this), m_signalScopes.graphViewNodeSelection);
		m_pGraphView->GetLinkSelectionSignal().Connect(MAKE_MEMBER_DELEGATE(CMainFrameWnd::OnGraphViewLinkSelection, *this), m_signalScopes.graphViewLinkSelection);
		m_pGraphView->GetDocModifiedSignal().Connect(MAKE_MEMBER_DELEGATE(CMainFrameWnd::OnGraphViewDocModified, *this), m_signalScopes.graphViewDocModified);
		GetSchematycFramework().GetLog().GetMessageSignal().Connect(MAKE_MEMBER_DELEGATE(CMainFrameWnd::OnLogMessage, *this), m_signalScopes.logMessage);
		// Hide docking client to ensure there are no gaps between panes.
		m_dockingPaneManager.HideClient(TRUE);
		// Close log output panes B, C and D, and compiler output pane by default.
		m_dockingPaneManager.ClosePane(IDW_SCHEMATYC_LOG_OUTPUT_PANE_B);
		m_dockingPaneManager.ClosePane(IDW_SCHEMATYC_LOG_OUTPUT_PANE_C);
		m_dockingPaneManager.ClosePane(IDW_SCHEMATYC_LOG_OUTPUT_PANE_D);
		m_dockingPaneManager.ClosePane(IDW_SCHEMATYC_COMPILER_OUTPUT_PANE);
		// Load layout and initialize pane titles.
		LoadLayout();
		InitPaneTitles();
		return TRUE;
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::DoDataExchange(CDataExchange* pDataExchange)
	{
		__super::DoDataExchange(pDataExchange);
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::PostNcDestroy()
	{
		delete this;
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CMainFrameWnd::PreTranslateMessage(MSG* pMsg)
	{
		if(__super::PreTranslateMessage(pMsg))
		{
			return TRUE;
		}
		else if((pMsg->message >= WM_KEYFIRST) && (pMsg->message <= WM_KEYLAST))
		{
			::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);
			return TRUE;
		}
		return FALSE;
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CMainFrameWnd::OnViewMenuCommand(UINT nID)
	{
		UINT	paneId = 0;
		switch(nID)
		{
		case ID_SCHEMATYC_VIEW_BROWSER:
			{
				paneId = IDW_SCHEMATYC_BROWSER_PANE;
				break;
			}
		case ID_SCHEMATYC_VIEW_ENV_BROWSER:
			{
				paneId = IDW_SCHEMATYC_ENV_BROWSER_PANE;
				break;
			}
		case ID_SCHEMATYC_VIEW_DETAIL:
			{
				paneId = IDW_SCHEMATYC_DETAIL_PANE;
				break;
			}
		case ID_SCHEMATYC_VIEW_GRAPH:
			{
				paneId = IDW_SCHEMATYC_GRAPH_PANE;
				break;
			}
		case ID_SCHEMATYC_VIEW_LOG_OUTPUT_A:
			{
				paneId = IDW_SCHEMATYC_LOG_OUTPUT_PANE_A;
				break;
			}
		case ID_SCHEMATYC_VIEW_LOG_OUTPUT_B:
			{
				paneId = IDW_SCHEMATYC_LOG_OUTPUT_PANE_B;
				break;
			}
		case ID_SCHEMATYC_VIEW_LOG_OUTPUT_C:
			{
				paneId = IDW_SCHEMATYC_LOG_OUTPUT_PANE_C;
				break;
			}
		case ID_SCHEMATYC_VIEW_LOG_OUTPUT_D:
			{
				paneId = IDW_SCHEMATYC_LOG_OUTPUT_PANE_D;
				break;
			}
		case ID_SCHEMATYC_VIEW_COMPILER_OUTPUT:
			{
				paneId = IDW_SCHEMATYC_COMPILER_OUTPUT_PANE;
				break;
			}
		case ID_SCHEMATYC_VIEW_VIEWPORT:
			{
				paneId = IDW_SCHEMATYC_VIEWPORT_PANE;
				break;
			}
		default:
			{
				return FALSE;
			}
		}
		CXTPDockingPane	*pPane = m_dockingPaneManager.FindPane(paneId);
		CRY_ASSERT(pPane != NULL);
		if(pPane != NULL)
		{
			if(pPane->IsClosed() == TRUE)
			{
				m_dockingPaneManager.ShowPane(pPane);
			}
			else
			{
				m_dockingPaneManager.ClosePane(pPane);
			}
			return TRUE;
		}
		return FALSE;
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnUpdateViewMenuItem(CCmdUI* pCmdUI)
	{
		UINT	paneId = 0;
		switch(pCmdUI->m_nID)
		{
		case ID_SCHEMATYC_VIEW_BROWSER:
			{
				paneId = IDW_SCHEMATYC_BROWSER_PANE;
				break;
			}
		case ID_SCHEMATYC_VIEW_ENV_BROWSER:
			{
				paneId = IDW_SCHEMATYC_ENV_BROWSER_PANE;
				break;
			}
		case ID_SCHEMATYC_VIEW_DETAIL:
			{
				paneId = IDW_SCHEMATYC_DETAIL_PANE;
				break;
			}
		case ID_SCHEMATYC_VIEW_GRAPH:
			{
				paneId = IDW_SCHEMATYC_GRAPH_PANE;
				break;
			}
		case ID_SCHEMATYC_VIEW_LOG_OUTPUT_A:
			{
				paneId = IDW_SCHEMATYC_LOG_OUTPUT_PANE_A;
				break;
			}
		case ID_SCHEMATYC_VIEW_LOG_OUTPUT_B:
			{
				paneId = IDW_SCHEMATYC_LOG_OUTPUT_PANE_B;
				break;
			}
		case ID_SCHEMATYC_VIEW_LOG_OUTPUT_C:
			{
				paneId = IDW_SCHEMATYC_LOG_OUTPUT_PANE_C;
				break;
			}
		case ID_SCHEMATYC_VIEW_LOG_OUTPUT_D:
			{
				paneId = IDW_SCHEMATYC_LOG_OUTPUT_PANE_D;
				break;
			}
		case ID_SCHEMATYC_VIEW_COMPILER_OUTPUT:
			{
				paneId = IDW_SCHEMATYC_COMPILER_OUTPUT_PANE;
				break;
			}
		case ID_SCHEMATYC_VIEW_VIEWPORT:
			{
				paneId = IDW_SCHEMATYC_VIEWPORT_PANE;
				break;
			}
		default:
			{
				pCmdUI->SetCheck(0);
				return;
			}
		}
		CXTPDockingPane	*pPane = m_dockingPaneManager.FindPane(paneId);
		CRY_ASSERT(pPane != NULL);
		if(pPane != NULL)
		{
			pCmdUI->SetCheck(pPane->IsClosed() == TRUE ? 0 : 1);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnToolsConfigureLogStreams()
	{
		SET_LOCAL_RESOURCE_SCOPE
		CPoint	point;
		GetCursorPos(&point);
		ILog&						log = GetSchematycFramework().GetLog();
		SLogUserStreams	logUserStreams = log.GetUserStreams();
		CPropertyTreeDlg	propertyTreeDlg(this, point, "Configure Log Streams", Serialization::SStruct(logUserStreams));
		if(propertyTreeDlg.DoModal() == IDOK)
		{
			log.SetUserStreams(logUserStreams);
			log.SaveUserStreams();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CMainFrameWnd::OnToolsConfigureLogOutput(UINT nID)
	{
		SET_LOCAL_RESOURCE_SCOPE
		SLogOutputSettings*	pLogOutputSettings = NULL;
		switch(nID)
		{
		case ID_SCHEMATYC_TOOLS_CONFIGURE_LOG_OUTPUT_A:
			{
				pLogOutputSettings = &m_logOutputSettings[0];
				break;
			}
		case ID_SCHEMATYC_TOOLS_CONFIGURE_LOG_OUTPUT_B:
			{
				pLogOutputSettings = &m_logOutputSettings[1];
				break;
			}
		case ID_SCHEMATYC_TOOLS_CONFIGURE_LOG_OUTPUT_C:
			{
				pLogOutputSettings = &m_logOutputSettings[2];
				break;
			}
		case ID_SCHEMATYC_TOOLS_CONFIGURE_LOG_OUTPUT_D:
			{
				pLogOutputSettings = &m_logOutputSettings[3];
				break;
			}
		default:
			{
				return FALSE;
			}
		}
		CPoint	point;
		GetCursorPos(&point);
		SLogOutputSettings	tempLogOutputSettings = *pLogOutputSettings;
		CPropertyTreeDlg		propertyTreeDlg(this, point, "Configure Log Output", Serialization::SStruct(tempLogOutputSettings));
		if(propertyTreeDlg.DoModal() == IDOK)
		{
			*pLogOutputSettings = tempLogOutputSettings;
		}
		return TRUE;
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CMainFrameWnd::OnWorldDebugMenuCommand(UINT nID)
	{
		ICVar*	pCVar = gEnv->pConsole->GetCVar("sc_WorldDebugConfig");
		CRY_ASSERT(pCVar != NULL);
		if(pCVar != NULL)
		{
			stack_string	worldDebugConfig = pCVar->GetString();
			char					debugOption[2] = "";
			switch(nID)
			{
			case ID_SCHEMATYC_WORLD_DEBUG_SHOW_STATES:
				{
					debugOption[0] = 's';
					break;
				}
			case ID_SCHEMATYC_WORLD_DEBUG_SHOW_VARIABLES:
				{
					debugOption[0] = 'v';
					break;
				}
			case ID_SCHEMATYC_WORLD_DEBUG_SHOW_CONTAINERS:
				{
					debugOption[0] = 'c';
					break;
				}
			case ID_SCHEMATYC_WORLD_DEBUG_SHOW_TIMERS:
				{
					debugOption[0] = 't';
					break;
				}
			case ID_SCHEMATYC_WORLD_DEBUG_SHOW_ACTIONS:
				{
					debugOption[0] = 'a';
					break;
				}
			case ID_SCHEMATYC_WORLD_DEBUG_SHOW_SIGNAL_HISTORY:
				{
					debugOption[0] = 'h';
					break;
				}
			}
			stack_string::size_type	pos = worldDebugConfig.find(debugOption);
			if(pos != stack_string::npos)
			{
				worldDebugConfig.erase(pos, 1);
			}
			else
			{
				worldDebugConfig.append(debugOption);
			}
			pCVar->Set(worldDebugConfig.c_str());
			return TRUE;
		}
		return FALSE;
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnUpdateWorldDebugMenuItem(CCmdUI* pCmdUI)
	{
		ICVar*	pCVar = gEnv->pConsole->GetCVar("sc_WorldDebugConfig");
		CRY_ASSERT(pCVar != NULL);
		if(pCVar != NULL)
		{
			const stack_string	worldDebugConfig = pCVar->GetString();
			char								debugOption[2] = "";
			switch(pCmdUI->m_nID)
			{
			case ID_SCHEMATYC_WORLD_DEBUG_SHOW_STATES:
				{
					debugOption[0] = 's';
					break;
				}
			case ID_SCHEMATYC_WORLD_DEBUG_SHOW_VARIABLES:
				{
					debugOption[0] = 'v';
					break;
				}
			case ID_SCHEMATYC_WORLD_DEBUG_SHOW_CONTAINERS:
				{
					debugOption[0] = 'c';
					break;
				}
			case ID_SCHEMATYC_WORLD_DEBUG_SHOW_TIMERS:
				{
					debugOption[0] = 't';
					break;
				}
			case ID_SCHEMATYC_WORLD_DEBUG_SHOW_ACTIONS:
				{
					debugOption[0] = 'a';
					break;
				}
			case ID_SCHEMATYC_WORLD_DEBUG_SHOW_SIGNAL_HISTORY:
				{
					debugOption[0] = 'h';
					break;
				}
			}
			pCmdUI->SetCheck(worldDebugConfig.find(debugOption) != stack_string::npos ? 1 : 0);
		}
		else
		{
			pCmdUI->SetCheck(0);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnDebugSelectedEntity()
	{
		ICVar*	pCVar = gEnv->pConsole->GetCVar("sc_EntityDebugName");
		CRY_ASSERT(pCVar != NULL);
		if(pCVar != NULL)
		{
			IEntity*	pSelectedEntity = gEnv->pEntitySystem->GetEntity(GetSelectedEntityId());
			pCVar->Set(pSelectedEntity != NULL ? pSelectedEntity->GetName() : "");
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnUpdateDebugSelectedEntity(CCmdUI* pCmdUI)
	{
		stack_string	text = "Debug Selected Entity";
		ICVar*				pCVar = gEnv->pConsole->GetCVar("sc_EntityDebugName");
		const char*		selectedEntity = pCVar != NULL ? pCVar->GetString() : "";
		if(selectedEntity[0] != '\0')
		{
			text.append(" [");
			text.append(selectedEntity);
			text.append("]");
		}
		pCmdUI->SetText(text.c_str());
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CMainFrameWnd::OnEntityDebugMenuCommand(UINT nID)
	{
		ICVar*	pCVar = gEnv->pConsole->GetCVar("sc_EntityDebugConfig");
		CRY_ASSERT(pCVar != NULL);
		if(pCVar != NULL)
		{
			stack_string	entityDebugConfig = pCVar->GetString();
			char					debugOption[2] = "";
			switch(nID)
			{
			case ID_SCHEMATYC_ENTITY_DEBUG_SHOW_STATES:
				{
					debugOption[0] = 's';
					break;
				}
			case ID_SCHEMATYC_ENTITY_DEBUG_SHOW_VARIABLES:
				{
					debugOption[0] = 'v';
					break;
				}
			case ID_SCHEMATYC_ENTITY_DEBUG_SHOW_CONTAINERS:
				{
					debugOption[0] = 'c';
					break;
				}
			case ID_SCHEMATYC_ENTITY_DEBUG_SHOW_TIMERS:
				{
					debugOption[0] = 't';
					break;
				}
			case ID_SCHEMATYC_ENTITY_DEBUG_SHOW_ACTIONS:
				{
					debugOption[0] = 'a';
					break;
				}
			case ID_SCHEMATYC_ENTITY_DEBUG_SHOW_SIGNAL_HISTORY:
				{
					debugOption[0] = 'h';
					break;
				}
			}
			stack_string::size_type	pos = entityDebugConfig.find(debugOption);
			if(pos != stack_string::npos)
			{
				entityDebugConfig.erase(pos, 1);
			}
			else
			{
				entityDebugConfig.append(debugOption);
			}
			pCVar->Set(entityDebugConfig.c_str());
			return TRUE;
		}
		return FALSE;
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnUpdateEntityDebugMenuItem(CCmdUI* pCmdUI)
	{
		ICVar*	pCVar = gEnv->pConsole->GetCVar("sc_EntityDebugConfig");
		CRY_ASSERT(pCVar != NULL);
		if(pCVar != NULL)
		{
			const stack_string	entityDebugConfig = pCVar->GetString();
			char								debugOption[2] = "";
			switch(pCmdUI->m_nID)
			{
			case ID_SCHEMATYC_ENTITY_DEBUG_SHOW_STATES:
				{
					debugOption[0] = 's';
					break;
				}
			case ID_SCHEMATYC_ENTITY_DEBUG_SHOW_VARIABLES:
				{
					debugOption[0] = 'v';
					break;
				}
			case ID_SCHEMATYC_ENTITY_DEBUG_SHOW_CONTAINERS:
				{
					debugOption[0] = 'c';
					break;
				}
			case ID_SCHEMATYC_ENTITY_DEBUG_SHOW_TIMERS:
				{
					debugOption[0] = 't';
					break;
				}
			case ID_SCHEMATYC_ENTITY_DEBUG_SHOW_ACTIONS:
				{
					debugOption[0] = 'a';
					break;
				}
			case ID_SCHEMATYC_ENTITY_DEBUG_SHOW_SIGNAL_HISTORY:
				{
					debugOption[0] = 'h';
					break;
				}
			}
			pCmdUI->SetCheck(entityDebugConfig.find(debugOption) != stack_string::npos ? 1 : 0);
		}
		else
		{
			pCmdUI->SetCheck(0);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnSave()
	{
		// De-select current details to ensure properties are up to date.
		m_pDetailPropertyTree->Detach();
		m_pDetailDoc = NULL;
		CXTPDockingPane*	pDetailPane = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_DETAIL_PANE);
		CRY_ASSERT(pDetailPane != NULL);
		if(pDetailPane != NULL)
		{
			pDetailPane->SetTitle(_T("Detail"));
			pDetailPane->SetTabCaption(_T("Detail"));
		}
		// Save documents.
		GetSchematycFramework().GetDocManager().Save();
		// Save environment settings.
		GetSchematycFramework().GetEnvRegistry().SaveAllSettings();
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnRefreshEnv()
	{
		// Save.
		OnSave();
		// Send signal to refresh environment.
		GetSchematycFramework().RefreshEnv();
		// Compile all documents.
		GetSchematycFramework().GetCompiler().CompileAllDocs();
		// Refresh environment browser and pdate compiler output.
		m_envBrowserCtrl.Refresh();
		UpdateCompilerOutputCtrl(false);
	}

	//////////////////////////////////////////////////////////////////////////
	LRESULT CMainFrameWnd::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
	{
		if(wParam == XTP_DPN_SHOWWINDOW)
		{
			if(CXTPDockingPane*	pDockingPane = reinterpret_cast<CXTPDockingPane *>(lParam))
			{
				if(!pDockingPane->IsValid())
				{
					switch(pDockingPane->GetID())
					{
					case IDW_SCHEMATYC_BROWSER_PANE:
						{
							pDockingPane->Attach(&m_browserCtrl);
							m_browserCtrl.ShowWindow(SW_SHOW);
							break;
						}
					case IDW_SCHEMATYC_ENV_BROWSER_PANE:
						{
							pDockingPane->Attach(&m_envBrowserCtrl);
							m_envBrowserCtrl.ShowWindow(SW_SHOW);
							break;
						}
					case IDW_SCHEMATYC_DETAIL_PANE:
						{
							CRect	paneWindowRect = pDockingPane->GetPaneWindowRect();
							ScreenToClient(&paneWindowRect);
							m_pDetailPropertyTree = CreatePropertyTree(pDockingPane->GetDockingSite(), paneWindowRect);
							pDockingPane->Attach(m_pDetailPropertyTree);
							m_pDetailPropertyTree->ShowWindow(SW_SHOW);
							break;
						}
					case IDW_SCHEMATYC_GRAPH_PANE:
						{
							pDockingPane->Attach(m_pGraphView);
							m_pGraphView->ShowWindow(SW_SHOW);
							break;
						}
					case IDW_SCHEMATYC_LOG_OUTPUT_PANE_A:
						{
							pDockingPane->Attach(&m_logOutputCtrlA);
							m_logOutputCtrlA.ShowWindow(SW_SHOW);
							break;
						}
					case IDW_SCHEMATYC_LOG_OUTPUT_PANE_B:
						{
							pDockingPane->Attach(&m_logOutputCtrlB);
							m_logOutputCtrlB.ShowWindow(SW_SHOW);
							break;
						}
					case IDW_SCHEMATYC_LOG_OUTPUT_PANE_C:
						{
							pDockingPane->Attach(&m_logOutputCtrlC);
							m_logOutputCtrlC.ShowWindow(SW_SHOW);
							break;
						}
					case IDW_SCHEMATYC_LOG_OUTPUT_PANE_D:
						{
							pDockingPane->Attach(&m_logOutputCtrlD);
							m_logOutputCtrlD.ShowWindow(SW_SHOW);
							break;
						}
					case IDW_SCHEMATYC_COMPILER_OUTPUT_PANE:
						{
							pDockingPane->Attach(&m_compilerOutputCtrl);
							m_compilerOutputCtrl.ShowWindow(SW_SHOW);
							break;
						}
					case IDW_SCHEMATYC_VIEWPORT_PANE:
						{
							pDockingPane->Attach(m_pViewportWnd);
							m_pViewportWnd->ShowWindow(SW_SHOW);
							m_pViewportWnd->InitLayout();
							break;
						}
					}
				}
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnDestroy()
	{
		SaveLayout();
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::RegisterViewClass()
	{
		GetIEditor()->GetClassFactory()->RegisterClass(new CMainViewClass());
	}

	//////////////////////////////////////////////////////////////////////////
	CMainFrameWnd::SLogOutputSettings::SLogOutputSettings()
		: showComments(true)
		, showWarnings(true)
		, showErrors(true)
	{
		streams.push_back(GetSchematycFramework().GetLog().GetStreamName(LOG_STREAM_DEFAULT));
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::SLogOutputSettings::Serialize(Serialization::IArchive& archive)
	{
		archive(showComments, "showComments", "Show Comments");
		archive(showWarnings, "showWarnings", "Show Warnings");
		archive(showErrors, "showErrors", "Show Errors");
		archive(streams, "streams", "Streams");
	}

	//////////////////////////////////////////////////////////////////////////
	bool CMainFrameWnd::CreateMenuAndToolbar()
	{
		// Initialize command bars.
		if(!CXTPFrameWnd::InitCommandBars())
		{
			return false;
		}
		CXTPCommandBars*	pCommandBars = CXTPFrameWnd::GetCommandBars();
		CRY_ASSERT(pCommandBars);
		if(!pCommandBars)
		{
			return false;
		}
		// Load menu.
		pCommandBars->GetShortcutManager()->SetAccelerators(IDR_SCHEMATYC_MENU);
		pCommandBars->GetShortcutManager()->LoadShortcuts("Shortcuts\\Schematyc");
		CXTPCommandBar*	pMenu = pCommandBars->SetMenu( _T("Menu"), IDR_SCHEMATYC_MENU);
		CRY_ASSERT(pMenu != NULL);
		if(pMenu != NULL)
		{
			pMenu->SetFlags(xtpFlagStretched);
			pMenu->EnableCustomization(TRUE);
		}
		// Create toolbar.
		CXTPToolBar*	pToolBar = pCommandBars->Add(_T("ToolBar"), xtpBarTop);
		if(!pToolBar)
		{
			return false;
		}
		pToolBar->EnableCustomization(FALSE);
		pToolBar->SetCustomizeDialogPresent(FALSE);
		pToolBar->ShowExpandButton(FALSE);
		pToolBar->LoadToolBar(IDR_SCHEMATYC_TOOLBAR);
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::CreatePanes()
	{
		// Initialise docking pane manager
		m_dockingPaneManager.InstallDockingPanes(this);
		m_dockingPaneManager.SetTheme(xtpPaneThemeOffice2003);
		m_dockingPaneManager.SetThemedFloatingFrames(TRUE);
		m_dockingPaneManager.SetAlphaDockingContext(TRUE);
		m_dockingPaneManager.SetShowDockingContextStickers(TRUE);
		// Create panes.
		CXTPDockingPane*	pBrowserPane = m_dockingPaneManager.CreatePane(IDW_SCHEMATYC_BROWSER_PANE, CRect(0, 0, 200, 600), xtpPaneDockLeft);
		CXTPDockingPane*	pGraphPane = m_dockingPaneManager.CreatePane(IDW_SCHEMATYC_GRAPH_PANE, CRect(0, 0, 1000, 800), xtpPaneDockRight);
		CXTPDockingPane*	pViewportPane = m_dockingPaneManager.CreatePane(IDW_SCHEMATYC_VIEWPORT_PANE, CRect(0, 0, 1000, 800), xtpPaneDockRight, pBrowserPane);
		CXTPDockingPane*	pLogOutputPaneA = m_dockingPaneManager.CreatePane(IDW_SCHEMATYC_LOG_OUTPUT_PANE_A, CRect(0, 0, 1000, 200), xtpPaneDockBottom, pViewportPane);
		CXTPDockingPane*	pLogOutputPaneB = m_dockingPaneManager.CreatePane(IDW_SCHEMATYC_LOG_OUTPUT_PANE_B, CRect(0, 0, 1000, 200), xtpPaneDockBottom, pViewportPane);
		CXTPDockingPane*	pLogOutputPaneC = m_dockingPaneManager.CreatePane(IDW_SCHEMATYC_LOG_OUTPUT_PANE_C, CRect(0, 0, 1000, 200), xtpPaneDockBottom, pViewportPane);
		CXTPDockingPane*	pLogOutputPaneD = m_dockingPaneManager.CreatePane(IDW_SCHEMATYC_LOG_OUTPUT_PANE_D, CRect(0, 0, 1000, 200), xtpPaneDockBottom, pViewportPane);
		CXTPDockingPane*	pCompilerOutputPane = m_dockingPaneManager.CreatePane(IDW_SCHEMATYC_COMPILER_OUTPUT_PANE, CRect(0, 0, 500, 200), xtpPaneDockBottom, pViewportPane);
		CXTPDockingPane*	pDetailPane = m_dockingPaneManager.CreatePane(IDW_SCHEMATYC_DETAIL_PANE, CRect(0, 0, 200, 600), xtpPaneDockRight, pViewportPane);
		CXTPDockingPane*	pEnvBrowserPane = m_dockingPaneManager.CreatePane(IDW_SCHEMATYC_ENV_BROWSER_PANE, CRect(0, 0, 200, 400), xtpPaneDockBottom, pBrowserPane);
		// Arrange panes.
		m_dockingPaneManager.AttachPane(pGraphPane, pViewportPane);
		m_dockingPaneManager.ShowPane(pViewportPane);
		m_dockingPaneManager.AttachPane(pLogOutputPaneB, pLogOutputPaneA);
		m_dockingPaneManager.AttachPane(pLogOutputPaneC, pLogOutputPaneB);
		m_dockingPaneManager.AttachPane(pLogOutputPaneD, pLogOutputPaneC);
		m_dockingPaneManager.AttachPane(pCompilerOutputPane, pLogOutputPaneD);
		m_dockingPaneManager.ShowPane(pLogOutputPaneA);
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::LoadLayout()
	{
		stack_string	path = gEnv->pSystem->GetIPak()->GetGameFolder();
		path.append("/");
		path.append(LAYOUT_FOLDER_NAME);
		path.append("/");
		path.append(LAYOUT_FILE_NAME);
		CXTPDockingPaneLayout	dockingPaneLayout(&m_dockingPaneManager);
		if(dockingPaneLayout.LoadFromFile(path.c_str(), LAYOUT_SECTION))
		{
			m_dockingPaneManager.SetLayout(&dockingPaneLayout);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::SaveLayout()
	{
		ICryPak*			pPak = gEnv->pSystem->GetIPak();
		stack_string	path = pPak->GetGameFolder();
		path.append("/");
		path.append(LAYOUT_FOLDER_NAME);
		pPak->MakeDir(path.c_str());
		path.append("/");
		path.append(LAYOUT_FILE_NAME);
		CXTPDockingPaneLayout	dockingPaneLayout(&m_dockingPaneManager);
		m_dockingPaneManager.GetLayout(&dockingPaneLayout);
		dockingPaneLayout.SaveToFile(path.c_str(), LAYOUT_SECTION);
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::InitPaneTitles()
	{
		CXTPDockingPane*	pBrowserPane = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_BROWSER_PANE);
		CRY_ASSERT(pBrowserPane != NULL);
		if(pBrowserPane != NULL)
		{
			pBrowserPane->SetTitle(_T("Browser"));
			pBrowserPane->SetTabCaption(_T("Browser"));
		}
		CXTPDockingPane*	pEnvBrowserPane = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_ENV_BROWSER_PANE);
		CRY_ASSERT(pEnvBrowserPane != NULL);
		if(pEnvBrowserPane != NULL)
		{
			pEnvBrowserPane->SetTitle(_T("Environment Browser"));
			pEnvBrowserPane->SetTabCaption(_T("Environment Browser"));
		}
		CXTPDockingPane*	pDetailPane = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_DETAIL_PANE);
		CRY_ASSERT(pDetailPane != NULL);
		if(pDetailPane != NULL)
		{
			pDetailPane->SetTitle(_T("Detail"));
			pDetailPane->SetTabCaption(_T("Detail"));
		}
		CXTPDockingPane*	pGraphPane = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_GRAPH_PANE);
		CRY_ASSERT(pGraphPane != NULL);
		if(pGraphPane != NULL)
		{
			pGraphPane->SetTitle(_T("Graph"));
			pGraphPane->SetTabCaption(_T("Graph"));
		}
		CXTPDockingPane*	pLogOutputPaneA = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_LOG_OUTPUT_PANE_A);
		CRY_ASSERT(pLogOutputPaneA != NULL);
		if(pLogOutputPaneA != NULL)
		{
			pLogOutputPaneA->SetTitle(_T("Log Output A"));
			pLogOutputPaneA->SetTabCaption(_T("Log Output A"));
		}
		CXTPDockingPane*	pLogOutputPaneB = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_LOG_OUTPUT_PANE_B);
		CRY_ASSERT(pLogOutputPaneB != NULL);
		if(pLogOutputPaneB != NULL)
		{
			pLogOutputPaneB->SetTitle(_T("Log Output B"));
			pLogOutputPaneB->SetTabCaption(_T("Log Output B"));
		}
		CXTPDockingPane*	pLogOutputPaneC = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_LOG_OUTPUT_PANE_C);
		CRY_ASSERT(pLogOutputPaneC != NULL);
		if(pLogOutputPaneC != NULL)
		{
			pLogOutputPaneC->SetTitle(_T("Log Output C"));
			pLogOutputPaneC->SetTabCaption(_T("Log Output C"));
		}
		CXTPDockingPane*	pLogOutputPaneD = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_LOG_OUTPUT_PANE_D);
		CRY_ASSERT(pLogOutputPaneD != NULL);
		if(pLogOutputPaneD != NULL)
		{
			pLogOutputPaneD->SetTitle(_T("Log Output D"));
			pLogOutputPaneD->SetTabCaption(_T("Log Output D"));
		}
		CXTPDockingPane*	pCompilerOutputPane = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_COMPILER_OUTPUT_PANE);
		CRY_ASSERT(pCompilerOutputPane != NULL);
		if(pCompilerOutputPane != NULL)
		{
			pCompilerOutputPane->SetTitle(_T("Compiler Output"));
			pCompilerOutputPane->SetTabCaption(_T("Compiler Output"));
		}
		CXTPDockingPane*	pViewportPane = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_VIEWPORT_PANE);
		CRY_ASSERT(pViewportPane != NULL);
		if(pViewportPane != NULL)
		{
			pViewportPane->SetTitle(_T("Viewport"));
			pViewportPane->SetTabCaption(_T("Viewport"));
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::SelectGraph(IDoc& doc, IDocGraph& docGraph)
	{
		stack_string								graphTitle(docGraph.GetName());
		const DocGraphType::EValue	docGraphType = docGraph.GetType();
		switch(docGraphType)
		{
		case DocGraphType::ABSTRACT_INTERFACE_FUNCTION:
			{
				graphTitle.append(" - Abstract Interface Function Graph");
				break;
			}
		case DocGraphType::FUNCTION:
			{
				graphTitle.append(" - Function Graph");
				break;
			}
		case DocGraphType::CONDITION:
			{
				graphTitle.append(" - Condition Graph");
				break;
			}
		case DocGraphType::SIGNAL_RECEIVER:
			{
				graphTitle.append(" - Signal Receiver Graph");
				break;
			}
		case DocGraphType::CONSTRUCTOR:
			{
				graphTitle.append(" - Constructor Graph");
				break;
			}
		case DocGraphType::TRANSITION:
			{
				graphTitle.append(" - Transition Graph");
				break;
			}
		}
		m_pGraphView->Load(&doc, &docGraph);
		CXTPDockingPane*	pGraphPane = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_GRAPH_PANE);
		CRY_ASSERT(pGraphPane);
		if(pGraphPane)
		{
			pGraphPane->SetTitle(_T(graphTitle.c_str()));
			m_dockingPaneManager.ShowPane(pGraphPane);
		}
		// Update compiler output.
		UpdateCompilerOutputCtrl(false);
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::ClearGraph()
	{
		m_pGraphView->Load(NULL, NULL);
		CXTPDockingPane*	pGraphPane = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_GRAPH_PANE);
		CRY_ASSERT(pGraphPane);
		if(pGraphPane)
		{
			pGraphPane->SetTitle(_T("Graph"));
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::UpdateCompilerOutputCtrl(bool recompileGraph)
	{
		struct SStringStreamBuffer
		{
			inline SStringStreamBuffer()
				: pos(0)
			{
				data[0] = '\0';
			}

			inline void Write(const char* input)
			{
				if(input)
				{
					const size_t	length = std::min(strlen(input), sizeof(data) - pos - 1);
					if(length)
					{
						memcpy(data + pos, input, length);
						pos += length;
						data[pos] = '\0';
					}
				}
			}

			char		data[2048];
			size_t	pos;
		};

		SStringStreamBuffer	stringStreamBuffer;
		IDoc*								pDoc = m_pGraphView->GetDoc();
		IDocGraph*					pDocGraph = m_pGraphView->GetDocGraph();
		if(pDoc && pDocGraph)
		{
			// TODO : Should be able to retrieve compiled documents from registry by name/filename!
			ILibPtr	pLib = /*recompileGraph ? */CompileDoc(*pDoc)/* : gEnv->pSchematycSystem->GetRegistry().GetLib(pDoc->GetFileName())*/;
			if(pLib)
			{
				pLib->PreviewGraphFunctions(pDocGraph->GetGUID(), MAKE_MEMBER_DELEGATE(SStringStreamBuffer::Write, stringStreamBuffer));
			}
		}
		m_compilerOutputCtrl.SetText(stringStreamBuffer.data);
	}

	//////////////////////////////////////////////////////////////////////////
	ILibPtr CMainFrameWnd::CompileDoc(IDoc& doc)
	{
		if(ILibPtr pLib = GetSchematycFramework().GetCompiler().CompileDoc(doc, TCompilerReportCallback()))
		{
			GetSchematycFramework().GetLibRegistry().RegisterLib(pLib);
			return pLib;
		}
		return ILibPtr();
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnBrowserSelection(const CBrowserCtrlItemPtr& pBrowserItem)
	{
		// TODO : Can't we just add a Serialize() function to the IDocElement interface?
		if(pBrowserItem != NULL)
		{
			const SGUID		browserItemGUID = pBrowserItem->GetGUID();
			IDoc*					pDoc = m_browserCtrl.GetItemDoc(*pBrowserItem);
			stack_string	title = "Detail";
			m_pDetailPropertyTree->Detach();
			m_pDetailDoc = pDoc;
			m_pDetailContextList.reset(new Serialization::CContextList());
			m_pDetailContextList->Update<IDoc>(m_pDetailDoc);
			m_pDetailPropertyTree->SetArchiveContext(m_pDetailContextList->Tail());

			switch(pBrowserItem->GetIcon())
			{
			case BrowserIcon::USER_GROUP:
				{
					IDocGroupPtr	pDocGroup = m_browserCtrl.GetItemDocGroup(*pBrowserItem);
					CRY_ASSERT(pDocGroup != NULL);
					if(pDocGroup != NULL)
					{
						m_pDetailPropertyTree->Attach(Serialization::SStruct(*pDocGroup));	// TODO : We should be storing a shared pointer to avoid this resource being released!
						title.append(" - Group");
					}
					break;
				}
			case BrowserIcon::ENUMERATION:
				{
					IDocEnumerationConstPtr	pDocEnumeration = m_browserCtrl.GetItemDocEnumeration(*pBrowserItem);
					CRY_ASSERT(pDocEnumeration != NULL);
					if(pDocEnumeration != NULL)
					{
						m_pDetailPropertyTree->Attach(Serialization::SStruct(*pDocEnumeration));	// TODO : We should be storing a shared pointer to avoid this resource being released!
						title.append(" - Enumeration");
					}
					break;
				}
			case BrowserIcon::STRUCTURE:
				{
					IDocStructureConstPtr	pDocStructure = m_browserCtrl.GetItemDocStructure(*pBrowserItem);
					CRY_ASSERT(pDocStructure != NULL);
					if(pDocStructure != NULL)
					{
						m_pDetailPropertyTree->Attach(Serialization::SStruct(*pDocStructure));	// TODO : We should be storing a shared pointer to avoid this resource being released!
						title.append(" - Structure");
					}
					break;
				}
			case BrowserIcon::SIGNAL:
				{
					IDocSignalPtr	pDocSignal = m_browserCtrl.GetItemDocSignal(*pBrowserItem);
					CRY_ASSERT(pDocSignal != NULL);
					if(pDocSignal != NULL)
					{
						m_pDetailPropertyTree->Attach(Serialization::SStruct(*pDocSignal));	// TODO : We should be storing a shared pointer to avoid this resource being released!
						title.append(" - Signal");
					}
					break;
				}
			case BrowserIcon::SCHEMA:
				{
					IDocSchemaPtr	pDocSchema = m_browserCtrl.GetItemDocSchema(*pBrowserItem);
					CRY_ASSERT(pDocSchema != NULL);
					if(pDocSchema != NULL)
					{
						m_pDetailPropertyTree->Attach(Serialization::SStruct(*pDocSchema));	// TODO : We should be storing a shared pointer to avoid this resource being released!
						title.append(" - Schema");
						break;
					}
					break;
				}
			case BrowserIcon::VARIABLE:
				{
					IDocVariablePtr	pDocVariable = m_browserCtrl.GetItemDocVariable(*pBrowserItem);
					CRY_ASSERT(pDocVariable != NULL);
					if(pDocVariable != NULL)
					{
						m_pDetailPropertyTree->Attach(Serialization::SStruct(*pDocVariable));	// TODO : We should be storing a shared pointer to avoid this resource being released!
						title.append(" - Variable");
					}
					break;
				}
			case BrowserIcon::TIMER:
				{
					IDocTimerPtr	pDocTimer = m_browserCtrl.GetItemDocTimer(*pBrowserItem);
					CRY_ASSERT(pDocTimer != NULL);
					if(pDocTimer != NULL)
					{
						m_pDetailPropertyTree->Attach(Serialization::SStruct(*pDocTimer));	// TODO : We should be storing a shared pointer to avoid this resource being released!
						title.append(" - Timer");
					}
					break;
				}
			case BrowserIcon::ENV_ABSTRACT_INTERFACE:
				{
					IDocAbstractInterfaceInstancePtr	pDocAbstractInterfaceInstance = m_browserCtrl.GetItemDocAbstractInterfaceInstance(*pBrowserItem);
					CRY_ASSERT(pDocAbstractInterfaceInstance != NULL);
					if(pDocAbstractInterfaceInstance != NULL)
					{
						m_pDetailPropertyTree->Attach(Serialization::SStruct(*pDocAbstractInterfaceInstance));	// TODO : We should be storing a shared pointer to avoid this resource being released!
						title.append(" - Abstract Interface");
					}
					break;
				}
			case BrowserIcon::COMPONENT:
				{
					IDocComponentInstancePtr	pDocComponentInstance = m_browserCtrl.GetItemDocComponentInstance(*pBrowserItem);
					CRY_ASSERT(pDocComponentInstance != NULL);
					if(pDocComponentInstance != NULL)
					{
						m_pDetailPropertyTree->Attach(Serialization::SStruct(*pDocComponentInstance));	// TODO : We should be storing a shared pointer to avoid this resource being released!
						title.append(" - Component");
					}
					break;
				}
			case BrowserIcon::ACTION_INSTANCE:
				{
					IDocActionInstancePtr	pDocActionInstance = m_browserCtrl.GetItemDocActionInstance(*pBrowserItem);
					CRY_ASSERT(pDocActionInstance != NULL);
					if(pDocActionInstance != NULL)
					{
						m_pDetailPropertyTree->Attach(Serialization::SStruct(*pDocActionInstance));	// TODO : We should be storing a shared pointer to avoid this resource being released!
						title.append(" - Action");
					}
					break;
				}
			case BrowserIcon::STATE_MACHINE:
				{
					IDocStateMachinePtr	pDocStateMachine = m_browserCtrl.GetItemDocStateMachine(*pBrowserItem);
					CRY_ASSERT(pDocStateMachine != NULL);
					if(pDocStateMachine != NULL)
					{
						m_pDetailPropertyTree->Attach(Serialization::SStruct(*pDocStateMachine));	// TODO : We should be storing a shared pointer to avoid this resource being released!
						title.append(" - State Machine");
					}
					break;
				}
			case BrowserIcon::STATE:
				{
					IDocStatePtr	pDocState = m_browserCtrl.GetItemDocState(*pBrowserItem);
					CRY_ASSERT(pDocState != NULL);
					if(pDocState != NULL)
					{
						m_pDetailPropertyTree->Attach(Serialization::SStruct(*pDocState));	// TODO : We should be storing a shared pointer to avoid this resource being released!
						title.append(" - State");
					}
					break;
				}
			case BrowserIcon::SETTINGS_DOC:
				{
					IEnvSettingsPtr	pSettings = GetSchematycFramework().GetEnvRegistry().GetSettings(pBrowserItem->GetText());
					CRY_ASSERT(pSettings != NULL);
					if(pSettings != NULL)
					{
						m_pDetailPropertyTree->Attach(Serialization::SStruct(*pSettings));	// TODO : We should be storing a shared pointer to avoid this resource being released!
						title.append(" - Settings");
					}
					break;
				}
			}

			m_pDetailPropertyTree->SetExpandLevels(1);
			m_pDetailPropertyTree->SetPropertyChangeHandler(functor(*this, &CMainFrameWnd::OnDetailPropertyChange));
			CXTPDockingPane*	pDetailPane = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_DETAIL_PANE);
			CRY_ASSERT(pDetailPane);
			if(pDetailPane)
			{
				pDetailPane->SetTitle(title.c_str());
				pDetailPane->SetTabCaption(title.c_str());
			}

			IDocGraphPtr	pDocGraph = m_browserCtrl.GetItemDocGraph(*pBrowserItem);
			CRY_ASSERT(pDocGraph != NULL);
			if(pDocGraph != NULL)
			{
				m_pDetailPropertyTree->Attach(Serialization::SStruct(*pDocGraph));	// TODO : We should be storing a shared pointer to avoid this resource being released!
				title.append(" - Graph");
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					SelectGraph(*pDoc, *pDocGraph);
				}
			}
			else
			{
				m_pGraphView->Load(NULL, NULL);
				// Bring viewport pane to front.
				CXTPDockingPane*	pViewportPane = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_VIEWPORT_PANE);
				CRY_ASSERT(pViewportPane != NULL);
				if(pViewportPane != NULL)
				{
					m_dockingPaneManager.ShowPane(pViewportPane);
				}
			}

			IDocSchemaPtr	pDocSchema = m_browserCtrl.GetItemDocSchema(*pBrowserItem);
			if(pDocSchema != NULL)
			{
				m_pViewportWnd->SetPreview(pDocSchema);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnBrowserDocModified(IDoc& doc)
	{
		// Refresh details in case change affected currently selected object.
		m_pDetailPropertyTree->Revert();
		// Re-compile doc.
		CompileDoc(doc);
		// Update compiler output?
		if(&doc == m_pGraphView->GetDoc())
		{
			UpdateCompilerOutputCtrl(false);
		}
		// Refresh graph view.
		m_pGraphView->Refresh();
		// Reset viewport preview.
		m_pViewportWnd->ResetPreview();
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnBrowserDocGraphRemoved(IDoc& doc, IDocGraph& docGraph)
	{
		if(&docGraph == m_pGraphView->GetDocGraph())
		{
			ClearGraph();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnDetailPropertyChange()
	{
		// Re-compile doc?
		if(m_pDetailDoc != NULL)
		{
			CompileDoc(*m_pDetailDoc);
		}
		// Refresh graph view.
		m_pGraphView->Refresh();
		// Reset viewport preview.
		m_pViewportWnd->ResetPreview();
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnGraphViewNodeSelection(const CDocGraphViewNodePtr& pSelectedNode)
	{
		m_pDetailPropertyTree->Detach();
		m_pDetailDoc = m_pGraphView->GetDoc();
		if(pSelectedNode != NULL)
		{
			m_pDetailPropertyTree->Attach(Serialization::SStruct(*pSelectedNode));	// TODO : We should be storing a shared pointer to avoid this resource being released!
		}
		CXTPDockingPane*	pDetailPane = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_DETAIL_PANE);
		CRY_ASSERT(pDetailPane != NULL);
		if(pDetailPane != NULL)
		{
			pDetailPane->SetTitle(_T("Detail - Graph Node"));
			pDetailPane->SetTabCaption(_T("Detail - Graph Node"));
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnGraphViewLinkSelection(SGraphViewLink* pSelectedLink)
	{
		m_pDetailPropertyTree->Detach();
		m_pDetailDoc = NULL;
		CXTPDockingPane*	pDetailPane = m_dockingPaneManager.FindPane(IDW_SCHEMATYC_DETAIL_PANE);
		CRY_ASSERT(pDetailPane);
		if(pDetailPane)
		{
			pDetailPane->SetTitle(_T("Detail"));
			pDetailPane->SetTabCaption(_T("Detail"));
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnGraphViewDocModified(IDoc& doc)
	{
		// Update compiler output?
		if(&doc == m_pGraphView->GetDoc())
		{
			UpdateCompilerOutputCtrl(true);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::OnLogMessage(const TLogStreamId& streamId, LogMessageType::EValue messageType, const char* message)
	{
		UpdateLogOutputCtrl(m_logOutputSettings[0], m_logOutputCtrlA, streamId, messageType, message);
		UpdateLogOutputCtrl(m_logOutputSettings[1], m_logOutputCtrlB, streamId, messageType, message);
		UpdateLogOutputCtrl(m_logOutputSettings[2], m_logOutputCtrlC, streamId, messageType, message);
		UpdateLogOutputCtrl(m_logOutputSettings[3], m_logOutputCtrlD, streamId, messageType, message);
	}

	//////////////////////////////////////////////////////////////////////////
	void CMainFrameWnd::UpdateLogOutputCtrl(const SLogOutputSettings& logOutputSettings, CCustomRichEditCtrl& logOutputCtrl, const TLogStreamId& streamId, LogMessageType::EValue messageType, const char* message)
	{
		switch(messageType)
		{
		case LogMessageType::COMMENT_MESSAGE:
			{
				if(logOutputSettings.showComments == false)
				{
					return;
				}
				break;
			}
		case LogMessageType::WARNING_MESSAGE:
			{
				if(logOutputSettings.showWarnings == false)
				{
					return;
				}
				break;
			}
		case LogMessageType::ERROR_MESSAGE:
			{
				if(logOutputSettings.showErrors == false)
				{
					return;
				}
				break;
			}
		}
		ILog&	log = GetSchematycFramework().GetLog();
		for(TLogStreamNameVector::const_iterator iOutputStream = logOutputSettings.streams.begin(), iEndOutputStream = logOutputSettings.streams.end(); iOutputStream != iEndOutputStream; ++ iOutputStream)
		{
			if(log.GetStreamId(iOutputStream->value.c_str()) == streamId)
			{
				logOutputCtrl.AppendText(message);
				break;
			}
		}
	}
}

#include <Editor/moc_Schematyc_MainFrameWnd.cpp>