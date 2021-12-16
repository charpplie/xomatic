#include "stdafx.h"
#include "Utilities.h"
#include "GridMapRenderWnd.h"
#include "GridMapTool.h"
#include "GridMapDlg.h"
#include "WorldDataStatusPanel.h"
#include "SegmentDataStatusPanel.h"
#include "SegmentSelectionPanel.h"
#include "SegmentedWorldManager.h"
#include "SegmentedWorld/ViewStates.h"
#include "GameEngine.h"
#include "UserMessageDefines.h"

using namespace sw;

#define IDW_GRID_MAP_PANE AFX_IDW_CONTROLBAR_FIRST+10

CryCriticalSection CGridMapDlg::s_cs;

//////////////////////////////////////////////////////////////////////////
class CGridMapDlgViewClass : public TRefCountBase<IViewPaneClass>
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		// {6EC1997E-5147-ab1b-F4B6-946D37032A2D}
		static const GUID guid = { 0x6ec1997e, 0x5147, 0xab1b, { 0xf4, 0xb6, 0x94, 0x6d, 0x37, 0x03, 0x2a, 0x2d } };
		return guid;
	}
	virtual const char* ClassName() { return _T("Segmented Level Editor"); };
	virtual const char* Category() { return _T("Segmented World"); };
	//////////////////////////////////////////////////////////////////////////
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CGridMapDlg); };
	virtual const char* GetPaneTitle() { return _T("Segmented Level Editor"); };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(100,100,1000,800); };
	virtual CSize GetMinSize() { return CSize(800,600); }
	virtual bool SinglePane() { return false; };
	virtual bool WantIdleUpdate() { return true; };
};

void CGridMapDlg::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass(new CGridMapDlgViewClass);
}

//////////////////////////////////////////////////////////////////////////
// CGridMapDlg dialog
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CGridMapDlg, CBaseFrameWnd)

BEGIN_MESSAGE_MAP(CGridMapDlg, CBaseFrameWnd)
	ON_COMMAND(ID_SW_OPEN, OnOpenWorld)
	ON_COMMAND(ID_SW_MOVE_TO_SELECTED, OnMoveToSelected)
	ON_COMMAND(ID_SW_MERGE, OnMergeLevels)
	ON_COMMAND(ID_SW_DEL_SELECTED, OnDeleteSelected)
	ON_COMMAND(ID_SW_GENERATEMAP, OnBnClickedGenerateSegMap)
	//////////////////////////////////////////////////////////////////////////
	ON_COMMAND(ID_TAG_LOC1, OnTagLocation1)
	ON_COMMAND(ID_TAG_LOC2, OnTagLocation2)
	ON_COMMAND(ID_TAG_LOC3, OnTagLocation3)
	ON_COMMAND(ID_TAG_LOC4, OnTagLocation4)
	ON_COMMAND(ID_TAG_LOC5, OnTagLocation5)
	ON_COMMAND(ID_TAG_LOC6, OnTagLocation6)
	ON_COMMAND(ID_TAG_LOC7, OnTagLocation7)
	ON_COMMAND(ID_TAG_LOC8, OnTagLocation8)
	ON_COMMAND(ID_TAG_LOC9, OnTagLocation9)
	ON_COMMAND(ID_TAG_LOC10, OnTagLocation10)
	ON_COMMAND(ID_TAG_LOC11, OnTagLocation11)
	ON_COMMAND(ID_TAG_LOC12, OnTagLocation12)
	//////////////////////////////////////////////////////////////////////////
	ON_COMMAND(ID_GOTO_LOC1, OnGotoLocation1)
	ON_COMMAND(ID_GOTO_LOC2, OnGotoLocation2)
	ON_COMMAND(ID_GOTO_LOC3, OnGotoLocation3)
	ON_COMMAND(ID_GOTO_LOC4, OnGotoLocation4)
	ON_COMMAND(ID_GOTO_LOC5, OnGotoLocation5)
	ON_COMMAND(ID_GOTO_LOC6, OnGotoLocation6)
	ON_COMMAND(ID_GOTO_LOC7, OnGotoLocation7)
	ON_COMMAND(ID_GOTO_LOC8, OnGotoLocation8)
	ON_COMMAND(ID_GOTO_LOC9, OnGotoLocation9)
	ON_COMMAND(ID_GOTO_LOC10, OnGotoLocation10)
	ON_COMMAND(ID_GOTO_LOC11, OnGotoLocation11)
	ON_COMMAND(ID_GOTO_LOC12, OnGotoLocation12)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_LOCK_SEGMENTS, &CGridMapDlg::OnBnClickedLockSegments)
	ON_BN_CLICKED(IDC_UNLOCK_SEGMENTS, &CGridMapDlg::OnBnClickedUnlockSegments)
	ON_BN_CLICKED(IDC_RESOLVE_SEGMENTS, &CGridMapDlg::OnBnClickedResolveSegments)
	ON_BN_CLICKED(IDC_REVERT_SEGMENTS, &CGridMapDlg::OnBnClickedRevertSegments)
	ON_BN_CLICKED(IDC_ROLLBACK_SEGMENTS, &CGridMapDlg::OnBnClickedRollbackSegments)
	ON_BN_CLICKED(IDC_LOCK_GDATA, &CGridMapDlg::OnBnClickedLockGData)
	ON_BN_CLICKED(IDC_COMMIT_GDATA, &CGridMapDlg::OnBnClickedCommitGData)
	ON_BN_CLICKED(IDC_RELOAD_GDATA, &CGridMapDlg::OnBnClickedReloadGData)
	ON_BN_CLICKED(IDC_REVERT_GDATA, &CGridMapDlg::OnBnClickedRevertGData)
	ON_BN_CLICKED(IDC_SAVE_GDATA, &CGridMapDlg::OnBnClickedSaveGData)
END_MESSAGE_MAP()

void CGridMapDlg::DoDataExchange(CDataExchange* pDX)
{
  CBaseFrameWnd::DoDataExchange(pDX);
  //DDX_Control(pDX, IDC_GDINFO_EDIT, m_EditGDStatus);
  //DDX_Control(pDX, IDC_GRID_MAP, m_MapWnd);
	DDX_Control(pDX, IDC_BTN_SW_MODE, m_btnSWMode);
}

CGridMapDlg::CGridMapDlg()
: m_pWDStatusPanel(NULL)
, m_pSDStatusPanel(NULL)
, m_pSelectionPanel(NULL)
{
	GetIEditor()->RegisterNotifyListener(this);
	
	CRect rc(0,0,0,0);
	Create(WS_CHILD|WS_VISIBLE, rc, AfxGetMainWnd());
}

CGridMapDlg::~CGridMapDlg()
{
	SaveLocations();

	if(CSWMiniMapUpdater::Get())
	{
		CSWMiniMapUpdater::Get()->SetSWMiniMapUpdaterCallback(NULL);
		CSWMiniMapUpdater::Get()->SetSWMiniMapLockStatusUpdaterCallback(NULL);
		CSWMiniMapUpdater::Get()->SetSWGDLockStatusUpdaterCallBack(NULL);
	}

	GetIEditor()->SetEditTool(0);
	m_pSegmentTool = NULL;

	GetIEditor()->UnregisterNotifyListener( this );
}

BOOL CGridMapDlg::OnInitDialog()
{
	LoadAccelTable( MAKEINTRESOURCE(IDR_GAMEACCELERATOR) );

	// Create and setup the heightmap edit viewport and the toolbars
	GetCommandBars()->GetCommandBarsOptions()->bShowExpandButtonAlways = FALSE;
	GetCommandBars()->EnableCustomization(FALSE);

	CXTPToolBar *pToolBar1 = GetCommandBars()->Add( _T("ToolBar1"), xtpBarTop);
	pToolBar1->EnableCustomization(FALSE);
	VERIFY(pToolBar1->LoadToolBar( IDR_GRID_MAP_BAR ));

	CXTPDockingPane* pDockPane_Rollup = GetDockingPaneManager()->CreatePane( IDW_GRID_MAP_PANE, CRect(0,0,300,100), xtpPaneDockRight );
	pDockPane_Rollup->SetTitle( "Rollup" );

	m_pViewport = new CGridMapRenderWnd;
	m_pViewport->SetDlgCtrlID(AFX_IDW_PANE_FIRST);
	m_pViewport->MoveWindow( CRect(20,50,500,500) );
	m_pViewport->ModifyStyle( WS_POPUP,WS_CHILD,0 );
	m_pViewport->SetParent( this );
	m_pViewport->SetOwner( this );
	m_pViewport->ShowWindow( SW_SHOW );
	m_pViewport->SetShowViewMarker( false );
	
	m_pSegmentTool = new CSegmentSelectTool;
	int nSegmentSizeMt = GetIEditor()->GetSegmentedWorldDoc().GetSegmentSizeInMeters();
	m_pSegmentTool->SetPerSegmentSize(nSegmentSizeMt);
	m_pViewport->SetEditTool(m_pSegmentTool, true);

	m_rollupCtrl.Create(WS_CHILD|WS_VISIBLE, CRect(0,0,100,100), this, 2);

	m_pSelectionPanel = new CSegmentSelectionPanel(this, m_pSegmentTool);
	m_rollupCtrl.InsertPage("Selection", m_pSelectionPanel);

	m_pWDStatusPanel = new CWorldDataStatusPanel(this);
	m_rollupCtrl.InsertPage("World Data", m_pWDStatusPanel);
	
	m_pSDStatusPanel = new CSegmentDataStatusPanel(this);
	m_rollupCtrl.InsertPage("Segment Data", m_pSDStatusPanel);
	m_pSegmentTool->SetExnernalUIPanel(m_pSDStatusPanel);

	UINT indicators[] =
	{
		AFX_IDW_STATUS_BAR,
		AFX_IDW_STATUS_BAR,
	};
	int indicatorCount = sizeof(indicators) / sizeof(UINT);
	m_statusBar.Create(this, WS_CHILD|WS_VISIBLE|CBRS_BOTTOM);
	m_statusBar.SetIndicators(indicators, indicatorCount);
	for(int i = 0; i < indicatorCount; i++)
	{
		m_statusBar.SetPaneText(i, "");
		m_statusBar.SetPaneWidth(i, 500);
	}

	OnLevelChanged();
	
	return TRUE;
}

BOOL CGridMapDlg::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	BOOL res = FALSE;
	if (m_pSDStatusPanel->m_hWnd)
	{
		res = m_pSDStatusPanel->OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
		if (TRUE == res)
			return res;
	}

	return CBaseFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CGridMapDlg::SendImage(int nSegmentID, CImageEx* pImage)
{
	AUTO_LOCK(CSWMiniMapUpdater::g_cs);

	Recti rect;
	SegIdToWorldRect(nSegmentID, rect, m_pSegmentTool->GetPerSegmentSize());
	m_pViewport->SetLayerImage(nSegmentID, pImage, rect);

  // auto level by default
  OnBnClickedAutoLevel();
}

void CGridMapDlg::SetSegInfoUpdateSuspended(bool bSuspend)
{
#ifdef USE_GDIWND
	m_pMapWnd->m_bSegInfoUpdateSuspended = bSuspend;
#endif
}

void CGridMapDlg::SetStatus(std::map<int, DWORD> &stateMap, std::map<int,TSegmentState> &mapSegStates)
{
#ifdef USE_GDIWND
	AUTO_LOCK(CSWMiniMapUpdater::g_cs);
	for (std::map<int, TSegmentState>::iterator itx = mapSegStates.begin(); itx != mapSegStates.end(); ++itx)
		m_pMapWnd->m_mapSegmentStates[itx->first] = itx->second;


	for (std::map<int, TSegmentState>::iterator itx = m_pMapWnd->m_mapSegmentStates.begin(); itx != m_pMapWnd->m_mapSegmentStates.end();)
	{
		std::map<int, TSegmentState>::iterator itCur = itx; ++itx;
		
		if (mapSegStates.find(itCur->first) == mapSegStates.end())
			m_pMapWnd->m_mapSegmentStates.erase(itCur);
	}

	bool bChanged = false;
	for (CMapWnd::TSegmentMap::iterator it1 = m_pMapWnd->m_SegmentsStatus.begin(); it1 != m_pMapWnd->m_SegmentsStatus.end(); ++it1)
		stateMap[it1->first];
	
	for (CMapWnd::TSegmentMap::iterator it2 = stateMap.begin(); it2 != stateMap.end(); ++it2) 
	{
		CPoint ptPos;
		SegIDToWorldCoordPoint(ptPos, it2->first);
		if (m_pMapWnd->SetSegmentState(m_pMapWnd->m_SegmentsStatus, ptPos, it2->second))
			bChanged = true;
	}

	if (bChanged)
		m_pMapWnd->Invalidate();
#endif
}
void CGridMapDlg::SetGDStatus(int nWDBType, int nWDBState, int nLockedBy, const char* pchLockedBy)
{
	//m_GDListCtrl.SetGDState(nWDBType, nWDBState, nLockedBy, pchLockedBy);
}
void CGridMapDlg::SetWorld(const char* pchWorldName)
{
	CSWMiniMapUpdater::s_bSuspend = true;
	AUTO_LOCK(CSWMiniMapUpdater::g_cs);
	if (strcmpi(m_strWorldName, pchWorldName) != 0)
	{
		m_strWorldName.Format("%s", pchWorldName);

		m_pViewport->DeleteLayer();
		Recti rect = m_pSegmentTool->GetWorldBoundary();
		m_pSegmentTool->SetWorldName(m_strWorldName);

		CSWMiniMapUpdater::Init(m_strWorldName, &rect);
	}

	if (CSWMiniMapUpdater::Get())
	{
		CSWMiniMapUpdater::Get()->SetSWMiniMapUpdaterCallback(this);
		CSWMiniMapUpdater::Get()->SetSWMiniMapLockStatusUpdaterCallback(this);
		CSWMiniMapUpdater::Get()->SetSWGDLockStatusUpdaterCallBack(this);
	}

	CSWMiniMapUpdater::s_bSuspend = false;
	
	if (CSWMiniMapUpdater::Get())
		CSWMiniMapUpdater::Get()->TriggerManuallyUpdate();
}

void CGridMapDlg::OnSize(UINT nType, int cx, int cy)
{
	CBaseFrameWnd::OnSize(nType, cx, cy);
	
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST,AFX_IDW_CONTROLBAR_LAST,0);
}

void CGridMapDlg::TagLocation(int index)
{
	assert(index > 0);

	Vec3 vPos = m_pViewport->GetOrigin2D();

	m_tagLocations[index - 1] = Vec2(vPos.x, vPos.y);

	CString sTagConsoleText("");
	sTagConsoleText.Format("Segmented Level Tag Point %d set to the position: x=%.2f, y=%.2f ",index,vPos.x,vPos.y);
	GetIEditor()->WriteToConsole(sTagConsoleText);

	if(gSettings.bAutoSaveTagPoints)
		SaveLocations();
}

void CGridMapDlg::GotoTagLocation(int index)
{
	m_pViewport->SetOrigin2D(Vec3(m_tagLocations[index - 1].x, m_tagLocations[index - 1].y, 0));
}

void CGridMapDlg::SaveLocations()
{
	char filename[_MAX_PATH];
	strcpy( filename,GetIEditor()->GetGameEngine()->GetLevelPath() );
	strcat( filename, "\\Locations.txt" );
	SetFileAttributes( filename,FILE_ATTRIBUTE_NORMAL );

	FILE *f = fopen( filename, "wt" );
	if (f)
	{
		for (int i = 0; i < 12; i++)
		{
			fprintf( f, "%f,%f\n", m_tagLocations[i].x,m_tagLocations[i].y);
		}
		fclose(f);
	}
}

void CGridMapDlg::LoadLocations()
{
	char filename[_MAX_PATH];
	strcpy( filename,GetIEditor()->GetGameEngine()->GetLevelPath() );
	strcat( filename, "\\Locations.txt" );
	SetFileAttributes( filename,FILE_ATTRIBUTE_NORMAL );

	ZeroStruct(m_tagLocations);

	FILE *f = fopen( filename, "rt" );
	if (f)
	{
		for (int i = 0; i < 12; i++)
		{
			float x=0,y=0;
			fscanf( f, "%f,%f\n",&x,&y );
			m_tagLocations[i] = Vec2(x,y);
		}
		fclose(f);
	}
}

void CGridMapDlg::OnBnClickedLockSegments()
{
#ifdef USE_GDIWND
  GetISystem()->GetIConsole()->ExecuteString("sw lock");
  m_pMapWnd->m_SegmentsCheck.clear();
  m_pMapWnd->SetFocus();
  m_pMapWnd->Invalidate();
#endif
}

void CGridMapDlg::OnBnClickedUnlockSegments()
{
#ifdef USE_GDIWND
  GetISystem()->GetIConsole()->ExecuteString("sw commit");
  m_pMapWnd->m_SegmentsCheck.clear();
  m_pMapWnd->SetFocus();
  m_pMapWnd->Invalidate();
#endif
}

void CGridMapDlg::OnBnClickedResolveSegments()
{
#ifdef USE_GDIWND
  GetISystem()->GetIConsole()->ExecuteString("sw resolve");
  m_pMapWnd->m_SegmentsCheck.clear();
  m_pMapWnd->SetFocus();
  m_pMapWnd->Invalidate();
#endif
}

void CGridMapDlg::OnBnClickedRevertSegments()
{
#ifdef USE_GDIWND
  GetISystem()->GetIConsole()->ExecuteString("sw revert");
  m_pMapWnd->m_SegmentsCheck.clear();
  m_pMapWnd->SetFocus();
  m_pMapWnd->Invalidate();
#endif
}

void CGridMapDlg::OnBnClickedRollbackSegments()
{
#ifdef USE_GDIWND
  GetISystem()->GetIConsole()->ExecuteString("sw revert ov");
  m_pMapWnd->m_SegmentsCheck.clear();
  m_pMapWnd->SetFocus();
  m_pMapWnd->Invalidate();
#endif
}
void CGridMapDlg::OnBnClickedGenerateSegMap()
{
	GetISystem()->GetIConsole()->ExecuteString("sw map gen");
#ifdef USE_GDIWND
	m_pMapWnd->m_SegmentsCheck.clear();
	m_pMapWnd->SetFocus();
	m_pMapWnd->Invalidate();
#endif
}

void CGridMapDlg::OnSWModeChange(int nMode)
{
	if (nMode == 0)
		m_btnSWMode.SetWindowText("Go Online");
	else
		m_btnSWMode.SetWindowText("Go Offline");
}

void CGridMapDlg::GetSelectedGDs(std::vector<int> &arrWDBTypes)
{
	//AUTO_LOCK(s_cs);
	//arrWDBTypes.clear();
	//for(int nItem =0 ; nItem <  m_GDListCtrl.GetItemCount(); nItem++)
	//{
	//	BOOL bChecked = m_GDListCtrl.GetCheck(nItem);
	//	if( bChecked )
	//	{
	//		arrWDBTypes.push_back(nItem);
	//	}
	//}
}

void CGridMapDlg::UnSelectedGDs()
{
	//AUTO_LOCK(s_cs);
	//for(int nItem =0 ; nItem <  m_GDListCtrl.GetItemCount(); nItem++)
	//{
	//	m_GDListCtrl.SetCheck(nItem, FALSE);
	//}
}

void CGridMapDlg::OnBnClickedLockGData()
{
	//AUTO_LOCK(s_cs);
	//std::vector<int> arrWDBTypes;
	//GetSelectedGDs(arrWDBTypes);
	//for (int i = 0; i < arrWDBTypes.size(); i++)
	//{
	//	string strWDBCC;
	//	strWDBCC.Format("sw wdb %d %d", arrWDBTypes[i], (int)sw::WDBCC_Lock);
	//	GetISystem()->GetIConsole()->ExecuteString(strWDBCC.c_str());
	//}
  
}

void CGridMapDlg::OnBnClickedCommitGData()
{
	//AUTO_LOCK(s_cs);
	//std::vector<int> arrWDBTypes;
	//GetSelectedGDs(arrWDBTypes);
	////if (arrWDBTypes.size() > 0 && !stl::find(arrWDBTypes, WDB_LEVELPAK))
	////{
	////	arrWDBTypes.push_back(WDB_LEVELPAK);
	////}
	//for (int i = 0; i < arrWDBTypes.size(); i++)
	//{
	//	string strWDBCC;
	//	strWDBCC.Format("sw wdb %d %d", arrWDBTypes[i], (int)sw::WDBCC_Commit);
	//	GetISystem()->GetIConsole()->ExecuteString(strWDBCC.c_str());
	//}
	//UnSelectedGDs();
}

void CGridMapDlg::OnBnClickedReloadGData()
{
	//AUTO_LOCK(s_cs);
	//std::vector<int> arrWDBTypes;
	//GetSelectedGDs(arrWDBTypes);
	//
	//for (int i = 0; i < arrWDBTypes.size(); i++)
	//{
	//	string strWDBCC;
	//	strWDBCC.Format("sw wdb %d %d", arrWDBTypes[i], (int)sw::WDBCC_Revert);
	//	GetISystem()->GetIConsole()->ExecuteString(strWDBCC.c_str());
	//	if (arrWDBTypes[i] == WDB_LEVELGENERAL || arrWDBTypes[i] == WDB_TIMEOFDAY)
	//		return;
	//}
}

void CGridMapDlg::OnBnClickedRevertGData()
{
	//AUTO_LOCK(s_cs);
	//std::vector<int> arrWDBTypes;
	//GetSelectedGDs(arrWDBTypes);
	//for (int i = 0; i < arrWDBTypes.size(); i++)
	//{
	//	string strWDBCC;
	//	strWDBCC.Format("sw wdb %d %d", arrWDBTypes[i], (int)sw::WDBCC_Rollback);
	//	GetISystem()->GetIConsole()->ExecuteString(strWDBCC.c_str());
	//	if (arrWDBTypes[i] == WDB_LEVELGENERAL || arrWDBTypes[i] == WDB_TIMEOFDAY)
	//		break;
	//}
	//UnSelectedGDs();
}

void CGridMapDlg::OnBnClickedSaveGData()
{
	//AUTO_LOCK(s_cs);
	//std::vector<int> arrWDBTypes;
	//GetSelectedGDs(arrWDBTypes);
	//for (int i = 0; i < arrWDBTypes.size(); i++)
	//{
	//	string strWDBCC;
	//	strWDBCC.Format("sw wdb %d %d", arrWDBTypes[i], (int)sw::WDBCC_Save);
	//	GetISystem()->GetIConsole()->ExecuteString(strWDBCC.c_str());
	//}
}

LRESULT CGridMapDlg::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == XTP_DPN_SHOWWINDOW)
	{
		// get a pointer to the docking pane being shown.
		CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;    
		if (!pwndDockWindow->IsValid())
		{
			switch (pwndDockWindow->GetID())
			{
			case IDW_GRID_MAP_PANE:
				pwndDockWindow->SetOptions(xtpPaneNoCloseable);
				pwndDockWindow->Attach(&m_rollupCtrl);
				break;
			default:
				return FALSE;
			}
		}
		return TRUE;
	}
	else if (wParam == XTP_DPN_CLOSEPANE)
	{
		// get a pointer to the docking pane being closed.
		CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;
		if (pwndDockWindow->IsValid())
		{
		}
	}

	return FALSE;
}

void CGridMapDlg::OnBnClickedAutoLevel()
{
#ifdef USE_GDIWND
	for (CMapWnd::TLayersMap::iterator it = m_pMapWnd->m_Layers.begin(); it != m_pMapWnd->m_Layers.end(); ++it)
	{
		DWORD dwLayerId = it->first;
		CMapWnd::TLayer& layer = it->second;
		layer.GetHistogramMinMax(&layer.autoLevelMin, &layer.autoLevelMax);
		for (CMapWnd::TImgagesMap::iterator it2 = layer.mImages.begin(); it2 != layer.mImages.end(); ++it2)
		{
			DWORD dwBitmapId = it2->first;
			m_pMapWnd->UpdateLayerBitmap(dwLayerId, dwBitmapId);
		}
	}

  m_pMapWnd->Invalidate();
#endif
}

void CGridMapDlg::OnBnClickedAutoLevelReset()
{
#ifdef USE_GDIWND
	for (CMapWnd::TLayersMap::iterator it = m_pMapWnd->m_Layers.begin(); it != m_pMapWnd->m_Layers.end(); ++it)
	{
		DWORD dwLayerId = it->first;
		CMapWnd::TLayer& layer = it->second;
		layer.autoLevelMin = 0;
		layer.autoLevelMax = 255;
		for (CMapWnd::TImgagesMap::iterator it2 = layer.mImages.begin(); it2 != layer.mImages.end(); ++it2)
		{
			DWORD dwBitmapId = it2->first;
			m_pMapWnd->UpdateLayerBitmap(dwLayerId, dwBitmapId);
		}
	}

  m_pMapWnd->Invalidate();
#endif
}

// fully update WorldMap when level changed
void CGridMapDlg::OnLevelChanged()
{
	ISegmentedWorldDoc& swdoc = GetIEditor()->GetSegmentedWorldDoc();
	if(!swdoc.IsOk())
		return;

	// Save location file for previous level
	SaveLocations();

	// Apply new world name
	SetWorld(swdoc.GetWorldName());
	SetWindowText(m_strWorldName);

	// Load location file from new level
	LoadLocations();

	m_pSegmentTool->UpdateWorldBound();
	m_pViewport->UpdateViewSettings(true);
	m_pViewport->UpdateView();
}

bool CGridMapDlg::IsSelectMode() const
{
#ifdef USE_GDIWND
	return m_pMapWnd->IsEditMode(EGEM_CHECK);
#else
	return false;
#endif
}

bool CGridMapDlg::IsOpenForEditMode() const
{
#ifdef USE_GDIWND
	return m_pMapWnd->IsEditMode(EGEM_EDIT);
#else
	return false;
#endif
}

bool CGridMapDlg::IsSegmentSelected( CPoint const& pt ) const
{
#ifdef USE_GDIWND
	return m_pMapWnd->GetSegmentState(m_pMapWnd->m_SegmentsCheck, pt);
#else
	return false;
#endif
}

void CGridMapDlg::OnOpenWorld()
{
	CString strWorldName;
	if(!m_pSegmentTool->OpenWorldInteranl(strWorldName))
		return;

	SetWorld(strWorldName);
	
	m_pSegmentTool->UpdateWorldBound();
}

void CGridMapDlg::OnMoveToSelected()
{
	assert(m_pSegmentTool);
	m_pSegmentTool->MoveToSelected();
}

void CGridMapDlg::OnMergeLevels()
{
	assert(m_pSegmentTool);
	m_pSegmentTool->PrepareForMerge();
}

void CGridMapDlg::OnDeleteSelected()
{
	assert(m_pSegmentTool);
	m_pSegmentTool->RemoveSelected();
	m_pSegmentTool->UpdateWorldBound();
}

void CGridMapDlg::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
	switch(event)
	{
	case eNotify_OnEndSWNewScene:
		OnLevelChanged();
		break;
	case eNotify_OnEndSWMoveTo:
		m_pViewport->UpdateViewSettings(true);
		break;
	case eNotify_OnIdleUpdate:
		{
			const char *pText = m_pSegmentTool->GetStatusText();
			m_statusBar.SetPaneText(0, pText);
		}
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CGridMapDlg::OnTagLocation1() { TagLocation(1);}
void CGridMapDlg::OnTagLocation2() { TagLocation(2);}
void CGridMapDlg::OnTagLocation3() { TagLocation(3);}
void CGridMapDlg::OnTagLocation4() { TagLocation(4);}
void CGridMapDlg::OnTagLocation5() { TagLocation(5);}
void CGridMapDlg::OnTagLocation6() { TagLocation(6);}
void CGridMapDlg::OnTagLocation7() { TagLocation(7);}
void CGridMapDlg::OnTagLocation8() { TagLocation(8);}
void CGridMapDlg::OnTagLocation9() { TagLocation(9);}
void CGridMapDlg::OnTagLocation10() { TagLocation(10);}
void CGridMapDlg::OnTagLocation11() { TagLocation(11);}
void CGridMapDlg::OnTagLocation12() { TagLocation(12);}

//////////////////////////////////////////////////////////////////////////
void CGridMapDlg::OnGotoLocation1() { GotoTagLocation(1);}
void CGridMapDlg::OnGotoLocation2() { GotoTagLocation(2);}
void CGridMapDlg::OnGotoLocation3() { GotoTagLocation(3);}
void CGridMapDlg::OnGotoLocation4() { GotoTagLocation(4);}
void CGridMapDlg::OnGotoLocation5() { GotoTagLocation(5);}
void CGridMapDlg::OnGotoLocation6() { GotoTagLocation(6);}
void CGridMapDlg::OnGotoLocation7() { GotoTagLocation(7);}
void CGridMapDlg::OnGotoLocation8() { GotoTagLocation(8);}
void CGridMapDlg::OnGotoLocation9() { GotoTagLocation(9);}
void CGridMapDlg::OnGotoLocation10() { GotoTagLocation(10);}
void CGridMapDlg::OnGotoLocation11() { GotoTagLocation(11);}
void CGridMapDlg::OnGotoLocation12() { GotoTagLocation(12);}
