#include "stdafx.h"
#include "LiveCreateDialog.h"

#include "../Objects/Entity.h"

#include "../Dialogs/Generic/StringInputDialog.h"

/*

  CLiveCreateTreeConnection

*/

CLiveCreateTreeConnection::CLiveCreateTreeConnection()
{
}

CLiveCreateTreeConnection::~CLiveCreateTreeConnection()
{
}

//

CLiveCreateConnection* CLiveCreateTreeConnection::IsConnection(HTREEITEM hItem)
{
	if (GetParentItem(hItem))
		return NULL;

	return (CLiveCreateConnection*)GetItemData(hItem);
}

CLiveCreateActor* CLiveCreateTreeConnection::IsActor(HTREEITEM hItem)
{
	if (!GetParentItem(hItem))
		return NULL;

	return (CLiveCreateActor*)GetItemData(hItem);
}

HTREEITEM CLiveCreateTreeConnection::FindConnectionItem(const CLiveCreateConnection* pConnection)
{
	if (!pConnection)
		return NULL;

	HTREEITEM hItem = GetFirstVisibleItem();
	while (hItem)
	{
		HTREEITEM hNext = GetNextItem(hItem, TVGN_NEXT);
		if (pConnection == IsConnection(hItem))
			return hItem;
		hItem = hNext;
	}

	return hItem;
}

HTREEITEM CLiveCreateTreeConnection::FindConnectionItem(const CLiveCreateScene* pScene)
{
	if (!pScene)
		return NULL;

	HTREEITEM hItem = GetFirstVisibleItem();
	while (hItem)
	{
		HTREEITEM hNext = GetNextItem(hItem, TVGN_NEXT);
		if (CLiveCreateConnection* pConnection = IsConnection(hItem))
		{
			if (pScene == &pConnection->GetScene())
				return hItem;
		}
		hItem = hNext;
	}

	return hItem;
}

CLiveCreateConnection* CLiveCreateTreeConnection::GetSelectedConnection()
{
	HTREEITEM hItem = GetSelectedItem();
	if (!hItem)
		return NULL;

	return IsConnection(hItem);
}

CLiveCreateActor* CLiveCreateTreeConnection::GetSelectedActor()
{
	HTREEITEM hItem = GetSelectedItem();
	if (!hItem)
		return NULL;

	return IsActor(hItem);
}

IEntity* CLiveCreateTreeConnection::GetEditorSelectedEntity()
{
	CBaseObject* pObject = ::GetIEditor()->GetSelectedObject();
	if (!pObject)
		return NULL;

	if (!pObject->IsKindOf(RUNTIME_CLASS(CEntity)))
		return NULL;

	return ((CEntity*)pObject)->GetIEntity();
}

void CLiveCreateTreeConnection::RefreshConnection(HTREEITEM hConnection, bool bForceExpand)
{
	CLiveCreateConnection* pConnection = IsConnection(hConnection);
	if (!pConnection)
		return;

	HTREEITEM hChild = GetChildItem(hConnection);
	while (hChild)
	{
		HTREEITEM hNext = GetNextItem(hChild, TVGN_NEXT);
		DeleteItem(hChild);
		hChild = hNext;
	}

	const CLiveCreateScene& scene = pConnection->GetScene();
	uint32 actorCount = scene.GetActorCount();
	for (uint32 i=0; i<actorCount; ++i)
	{
		const CLiveCreateActor* pActor = scene.GetActor(i);
		hChild = InsertItem(pActor->GetName(), hConnection, TVI_LAST);
		SetItemData(hChild, (DWORD_PTR)pActor);
	}

	if (bForceExpand)
		Expand(hConnection, TVE_EXPAND);
}

void CLiveCreateTreeConnection::RefreshConnections()
{
	CLiveCreate& liveCreate = CLiveCreate::Instance();

	DeleteAllItems();
	size_t connectionCount = liveCreate.GetConnectionCount();
	for (size_t i=0; i<connectionCount; ++i)
	{
		CLiveCreateConnection* pConnection = liveCreate.GetConnection(i);
		HTREEITEM hConnection = InsertItem(pConnection->GetName(), TVI_ROOT, TVI_LAST);
		SetItemState(hConnection, TVIS_EXPANDPARTIAL, TVIS_EXPANDPARTIAL);
		SetItemData(hConnection, (DWORD_PTR)pConnection);

		RefreshConnection(hConnection, false);
	}
}

//

void CLiveCreateTreeConnection::CreateMenuConnection(CLiveCreateConnection& connection, CMenu& menu)
{
	menu.CreatePopupMenu();
	if (connection.IsConnected())
		menu.AppendMenu(NULL, eID_Connection_Disconnect, "Disconnect");
	else
		menu.AppendMenu(NULL, eID_Connection_Connect, "Connect");
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(NULL, eID_Connection_SetEntity, "SetEntity");
}

void CLiveCreateTreeConnection::Create(CWnd* pParent, UINT nID)
{
	CTreeCtrl::Create(WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_LINESATROOT, CRect(0, 0, 0, 0), pParent, nID);

	m_menuConnection.CreatePopupMenu();
	m_menuConnection.AppendMenu(NULL, eID_Connection_Connect, "Connect");
	m_menuConnection.AppendMenu(MF_SEPARATOR);
	m_menuConnection.AppendMenu(NULL, eID_Connection_SetEntity, "SetEntity");

	m_menuActor.CreatePopupMenu();
	m_menuActor.AppendMenu(MF_STRING, eID_Actor_SetEntity, "SetEntity");
	m_menuActor.AppendMenu(MF_SEPARATOR);

	m_menuMappingGraph.CreatePopupMenu();
	m_menuMappingGraph.AppendMenu(MF_STRING, eID_Actor_MappingGraph_New, "New");
	m_menuMappingGraph.AppendMenu(MF_STRING, eID_Actor_MappingGraph_Open, "Open");
	m_menuActor.AppendMenu(MF_POPUP, (UINT_PTR)m_menuMappingGraph.GetSafeHmenu(), "Mapping Graph");
}

//

BEGIN_MESSAGE_MAP(CLiveCreateTreeConnection, CTreeCtrl)
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()

	ON_BN_CLICKED(eID_Connection_Connect, &OnConnection_Connect)
	ON_BN_CLICKED(eID_Connection_Disconnect, &OnConnection_Disconnect)
	ON_BN_CLICKED(eID_Connection_SetEntity, &OnConnection_SetEntity)

	ON_BN_CLICKED(eID_Actor_SetEntity, &OnActor_SetEntity)
	ON_BN_CLICKED(eID_Actor_MappingGraph_New, &OnActor_MappingGraph_New)
	ON_BN_CLICKED(eID_Actor_MappingGraph_Open, &OnActor_MappingGraph_Open)
END_MESSAGE_MAP()

afx_msg void CLiveCreateTreeConnection::OnRButtonDown(UINT nFlags, CPoint point)
{
	HTREEITEM hItem = HitTest(point);
	if (!hItem)
		return;

	SelectItem(hItem);
}

afx_msg void CLiveCreateTreeConnection::OnRButtonUp(UINT nFlags, CPoint point)
{
	HTREEITEM hItem = GetSelectedItem();
	if (!hItem)
		return;

	ClientToScreen(&point);

	if (CLiveCreateConnection* pConnection = IsConnection(hItem))
	{
//		m_menuConnection.TrackPopupMenu(NULL, point.x, point.y, this);
		CMenu menu;
		CreateMenuConnection(*pConnection, menu);
		menu.TrackPopupMenu(NULL, point.x, point.y, this);
	}
	else if (IsActor(hItem))
	{
		m_menuActor.TrackPopupMenu(NULL, point.x, point.y, this);
	}
}

afx_msg void CLiveCreateTreeConnection::OnConnection_Connect()
{
	CLiveCreateConnection* pConnection = GetSelectedConnection();
	if (!pConnection)
		return;

	CStringInputDialog address;
	address.SetTitle("Connection Address");
	address.SetText("192.168.6.81");
	if (address.DoModal() != 1)
		return;

	if (!pConnection->Connect(address.GetResultingText()))
	{
		::AfxMessageBox("Was unable to connect!");
		return;
	}

	pConnection->GetScene().AddListener(this);

	if (HTREEITEM hItem = GetSelectedItem())
		Expand(hItem, TVE_EXPAND);
}

afx_msg void CLiveCreateTreeConnection::OnConnection_Disconnect()
{
	CLiveCreateConnection* pConnection = GetSelectedConnection();
	if (!pConnection)
		return;

	pConnection->Disconnect();
	pConnection->GetScene().DeleteActors();
	pConnection->Reset();

	HTREEITEM hItem = FindConnectionItem(pConnection);
	if (!hItem)
		return;

	RefreshConnection(hItem, false);
}

afx_msg void CLiveCreateTreeConnection::OnConnection_SetEntity()
{
	CLiveCreateConnection* pConnection = GetSelectedConnection();
	if (!pConnection)
		return;

	IEntity* pEntity = GetEditorSelectedEntity();
	if (!pEntity)
		return;

	pConnection->GetScene().SetEntity(pEntity);
}

afx_msg void CLiveCreateTreeConnection::OnActor_SetEntity()
{
	CLiveCreateActor* pActor = GetSelectedActor();
	if (!pActor)
		return;

	IEntity* pEntity = GetEditorSelectedEntity();
	if (!pEntity)
		return;

	pActor->SetEntity(pEntity);
}

afx_msg void CLiveCreateTreeConnection::OnActor_MappingGraph_New()
{
	CLiveCreateActor* pActor = GetSelectedActor();
	if (!pActor)
		return;

	Skeleton::CMapperGraph* pGraph = pActor->CreateSkeletonMapperGraph();
	m_pGraphView->SetGraph(pGraph);
}

afx_msg void CLiveCreateTreeConnection::OnActor_MappingGraph_Open()
{
	CLiveCreateActor* pActor = GetSelectedActor();
	if (!pActor)
		return;

	CFileDialog fileDialog(true);
	if (fileDialog.DoModal() == IDCANCEL)
		return;

	Skeleton::CMapperGraph* pGraph =
		Skeleton::CMapperGraphManager::Instance().Load(fileDialog.GetPathName());
	pActor->SetSkeletonMapperGraph(pGraph);

	m_pGraphView->SetGraph(pGraph);
}

// ILiveCreateSceneListener
void CLiveCreateTreeConnection::OnCreateActor(const CLiveCreateScene& scene, const CLiveCreateActor& actor)
{
	HTREEITEM hConnection = FindConnectionItem(&scene);
	if (!hConnection)
		return;

	RefreshConnection(hConnection);
}

/*

  CLiveCreateDialog

*/

void CLiveCreateDialog::RegisterViewClass()
{
	class CViewClass :
		public TRefCountBase<IViewPaneClass>
	{
		virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
		virtual REFGUID ClassID()
		{
			// {CD9DEEBE-C417-482b-8A6E-2D4E01EA67B3}
			static const GUID guid = { 0xcd9deebe, 0xc417, 0x482b, { 0x8a, 0x6e, 0x2d, 0x4e, 0x1, 0xea, 0x67, 0xb3 } };
			return guid;
		}
		virtual const char* ClassName() { return "LiveMocap"; };
		virtual const char* Category() { return "LiveMocap"; };
		virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CLiveCreateDialog); };
		virtual const char* GetPaneTitle() { return _T("LiveMocap"); };
		virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
		virtual CRect GetPaneRect() { return CRect(100, 100, 1000, 800); };
		virtual bool SinglePane() { return false; };
		virtual bool WantIdleUpdate() { return true; };
	};

	::GetIEditor()->GetClassFactory()->RegisterClass(new CViewClass());
}

//

IMPLEMENT_DYNCREATE(CLiveCreateDialog, CBaseFrameWnd)

BEGIN_MESSAGE_MAP(CLiveCreateDialog, CBaseFrameWnd)
	ON_NOTIFY(TVN_SELCHANGED, AFX_IDW_PANE_FIRST, OnTreeSelection) // TEMP FIX!
END_MESSAGE_MAP()

//

CLiveCreateDialog::CLiveCreateDialog()
{
	Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), ::AfxGetMainWnd());
}

CLiveCreateDialog::~CLiveCreateDialog()
{
}

//

void CLiveCreateDialog::DoDataExchange(CDataExchange* pDX)
{
	CBaseFrameWnd::DoDataExchange(pDX);
}

BOOL CLiveCreateDialog::OnInitDialog()
{
	if (!__super::OnInitDialog())
		return false;

	m_splitters[0].CreateStatic(this, 1, 2, WS_CHILD | WS_VISIBLE);
	m_splitters[0].SetSplitterStyle(XT_SPLIT_NOFULLDRAG | XT_SPLIT_NOBORDER | XT_SPLIT_DOTTRACKER);
	m_splitters[0].SetColumnInfo(0, 160, 0);

	m_splitters[1].CreateStatic(&m_splitters[0], 3, 1, WS_CHILD | WS_VISIBLE);
	m_splitters[1].SetSplitterStyle(XT_SPLIT_NOFULLDRAG | XT_SPLIT_NOBORDER | XT_SPLIT_DOTTRACKER);
	m_splitters[1].SetRowInfo(0, 160, 0);
	m_splitters[1].SetRowInfo(1, 160, 0);
	m_splitters[1].SetDlgCtrlID(m_splitters[0].IdFromRowCol(0, 0));

	m_tree.Create(&m_splitters[1], eID_Tree);
	m_tree.SetDlgCtrlID(m_splitters[1].IdFromRowCol(0, 0));

	m_properties.SetParent(&m_splitters[1]);
	m_properties.SetDlgCtrlID(m_splitters[1].IdFromRowCol(1, 0));
	m_properties.ShowWindow(SW_SHOWDEFAULT);

	m_graphProperties.SetParent(&m_splitters[1]);
	m_graphProperties.SetDlgCtrlID(m_splitters[1].IdFromRowCol(2, 0));
	m_graphProperties.ShowWindow(SW_SHOWDEFAULT);

	m_graphView.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), &m_splitters[0], NULL);
	m_graphView.SetDlgCtrlID(m_splitters[0].IdFromRowCol(0, 1));
	m_graphView.AddListener(this);

	m_tree.m_pGraphView = &m_graphView;
	m_tree.RefreshConnections();

	return true;
}

//

void CLiveCreateDialog::DisplayConnectioProperties(CLiveCreateConnection* pConnection)
{
}

void CLiveCreateDialog::DisplayActorProperties(CLiveCreateActor* pActor)
{
	m_splitters[1].HideRow(1);

	CVariableArray* pLocations = new CVariableArray();
	pLocations->SetHumanName("Locations");
	pLocations->AddChildVar(pActor->m_locationsShow);
	pLocations->AddChildVar(pActor->m_locationsShowName);
	pLocations->AddChildVar(pActor->m_locationsShowHierarchy);

	CVariableArray* pEntity = new CVariableArray();
	pEntity->AddChildVar(pActor->m_entityUpdate);
	pEntity->AddChildVar(pActor->m_entityShowHierarchy);
	pEntity->AddChildVar(pActor->m_entityScale);

	CVarBlockPtr varBlock = new CVarBlock();
	varBlock->AddVariable(pLocations);
	varBlock->AddVariable(pEntity);
	m_properties.AddVars(varBlock);

	m_splitters[1].ShowRow();
}

void CLiveCreateDialog::OnTreeSelection(NMHDR*, LRESULT*)
{
	m_properties.DeleteVars();

	if (CLiveCreateConnection* pConnection = m_tree.GetSelectedConnection())
	{
		DisplayConnectioProperties(pConnection);
		return;
	}

	if (CLiveCreateActor* pActor = m_tree.GetSelectedActor())
	{
		DisplayActorProperties(pActor);
		m_graphView.SetGraph(pActor->GetSkeletonMapperGraph());
		return;
	}
}

// Skeleton::IMapperGraphViewListener

void CLiveCreateDialog::OnSelection(const std::vector<CHyperNode*>& previous, const std::vector<CHyperNode*>& current)
{
	m_graphProperties.DeleteVars();
	if (current.empty())
		return;

	if (::stricmp(current[0]->GetClassName(), "Node") == 0)
		return;

	Skeleton::CMapperOperator* pOperator =
		((Skeleton::CMapperGraphManager::COperator*)current[0])->GetOperator();
	if (!pOperator)
		return;

	uint32 parameterCount = pOperator->GetParameterCount();
	if (!parameterCount)
		return;

	CVarBlockPtr varBlock = new CVarBlock();
	for (uint32 i=0; i<parameterCount; ++i)
		varBlock->AddVariable(pOperator->GetParameter(i));
	m_graphProperties.AddVars(varBlock);
}
