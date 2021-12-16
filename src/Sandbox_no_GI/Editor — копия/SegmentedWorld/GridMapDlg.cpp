#include "stdafx.h"
#include "GridMapDlg.h"
#include "Controls/MemDC.h"
#include "SegmentedWorldManager.h"
#include "SegmentedWorld/ViewStates.h"

using namespace sw;

#define IDW_GRID_MAP_PANE AFX_IDW_CONTROLBAR_FIRST+10

CGridMapDlg *CGridMapDlg::m_pInstance = 0;
CryCriticalSection CGridMapDlg::s_cs;

//////////////////////////////////////////////////////////////////////////
class CGridMapEditorViewClass : public TRefCountBase<IViewPaneClass>
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
	virtual const char* ClassName() { return _T("Grid Map Editor"); };
	virtual const char* Category() { return _T("Segmented World"); };
	//////////////////////////////////////////////////////////////////////////
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CGridMapEditor); };
	virtual const char* GetPaneTitle() { return _T("Grid Map Editor"); };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(0,0,700,400); };
	virtual CSize GetMinSize() { return CSize(800,600); }
	virtual bool SinglePane() { return true; };
	virtual bool WantIdleUpdate() { return true; };
};
#ifdef SEG_WORLD
REGISTER_CLASS_DESC(CGridMapEditorViewClass);
#endif

//////////////////////////////////////////////////////////////////////////
// CGDListCtrl dialog
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(CGDListCtrl, CListCtrl)

BEGIN_MESSAGE_MAP(CGDListCtrl, CWnd)
  ON_WM_PAINT()
END_MESSAGE_MAP()
//---------------------------------------
void CGDListCtrl::Init()
{
	DeleteAllItems();
	InsertColumn(0,"GDType",LVCFMT_LEFT,150);
	InsertColumn(1,"Status",LVCFMT_LEFT,200);
	for(int i = 0; i < WDB_COUNT; i++)
	{
		InsertItem(i, CSWManager::GetWDBlockName((EWDBType)i));
		m_GDStates[i].SetState(sw::WDBState_Unknown, 0, "");
		SetItemText(i, 1, "Unknown");
	}
}
//---------------------------------------
void CGDListCtrl::SetGDState( int nWDBType, int nWDBState, int nLockedBy, const char* pchLockedBy )
{
	if (nWDBType < 0 || nWDBType >= WDB_COUNT)
	{
		assert(0);
		return;
	}
	if (m_GDStates[nWDBType].m_nWDBState == nWDBState && m_GDStates[nWDBType].m_nLockedBy == nLockedBy)
		return;

	m_GDStates[nWDBType].SetState(nWDBState, nLockedBy, pchLockedBy);
	
	Invalidate();
}
void CGDListCtrl::GetGDStatusText(int nWDBType, string &strText)
{

	switch (m_GDStates[nWDBType].m_nWDBState)
	{
	case sw::WDBState_Default:
		strText.Format("Not Locked");
		break;
	case sw::WDBState_Missing:
		strText.Format("Missing");
		break;
	case sw::WDBState_LockedByMe:
		strText.Format("Locked By me %s(%d)", m_GDStates[nWDBType].m_strLockedBy.c_str(), m_GDStates[nWDBType].m_nLockedBy);
		break;
	case sw::WDBState_LockedByOther:
		strText.Format("Locked By %s(%d)", m_GDStates[nWDBType].m_strLockedBy.c_str(), m_GDStates[nWDBType].m_nLockedBy);
		break;
	case sw::WDBState_Modified:
		strText.Format("Modified [Lock: %s(%d)]", m_GDStates[nWDBType].m_strLockedBy.c_str(), m_GDStates[nWDBType].m_nLockedBy);
		break;
	case sw::WDBState_OutOfDate:
		strText.Format("OUT OF DATE [Lock: %s(%d)]", m_GDStates[nWDBType].m_strLockedBy.c_str(), m_GDStates[nWDBType].m_nLockedBy);
		break;
	case sw::WDBState_Conflicted:
		strText.Format("CONFLICTED [Lock: %s(%d)]", m_GDStates[nWDBType].m_strLockedBy.c_str(), m_GDStates[nWDBType].m_nLockedBy);
		break;
	case sw::WDBState_Unknown:
	default:
		strText.Format("Unknown");
		break;

	}
}

void CGDListCtrl::OnPaint()
{
	AUTO_LOCK(CGridMapDlg::s_cs);
	if (GetItemCount() != WDB_COUNT)
	{
		assert(0);
		Init();
	}
	string strText;
	for (int i = 0; i < WDB_COUNT; i++)
	{
		GetGDStatusText(i, strText);
		SetItemText(i, 1, strText.c_str());
	}
	CListCtrl::OnPaint();
}

//////////////////////////////////////////////////////////////////////////
// CGridMapDlg dialog
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CGridMapDlg, CXTResizeDialog)

BEGIN_MESSAGE_MAP(CGridMapDlg, CXTResizeDialog)
  ON_WM_LBUTTONDOWN()
  ON_BN_CLICKED(IDC_GO, &CGridMapDlg::OnBnClickedGo)
  ON_BN_CLICKED(IDC_LOAD, &CGridMapDlg::OnBnClickedLoad)
  ON_BN_CLICKED(IDC_CANCEL, &CGridMapDlg::OnBnClickedCancel)
  ON_CBN_SELCHANGE(IDC_LOCATIONS, &CGridMapDlg::OnLocationShow)
  ON_CBN_EDITCHANGE(IDC_LOCATIONS, &CGridMapDlg::OnLocationUpdate)
  ON_BN_CLICKED(IDC_LOC_ADD, &CGridMapDlg::OnLocationAdd)
  ON_BN_CLICKED(IDC_LOC_DEL, &CGridMapDlg::OnLocationDel)
  ON_WM_SIZE()
  ON_BN_CLICKED(IDC_CHECKING_MODE, &CGridMapDlg::OnBnClickedCheckingMode)
  ON_BN_CLICKED(IDC_LOCK_SEGMENTS, &CGridMapDlg::OnBnClickedLockSegments)
  ON_BN_CLICKED(IDC_UNLOCK_SEGMENTS, &CGridMapDlg::OnBnClickedUnlockSegments)
  ON_BN_CLICKED(IDC_RESOLVE_SEGMENTS, &CGridMapDlg::OnBnClickedResolveSegments)
  ON_BN_CLICKED(IDC_UNSELECT_ALL_SEGMENTS, &CGridMapDlg::OnBnClickedUnselectAllSegments)
  ON_BN_CLICKED(IDC_REVERT_SEGMENTS, &CGridMapDlg::OnBnClickedRevertSegments)
  ON_BN_CLICKED(IDC_ROLLBACK_SEGMENTS, &CGridMapDlg::OnBnClickedRollbackSegments)
	ON_BN_CLICKED(IDC_GENERATE_SEG_MAP, &CGridMapDlg::OnBnClickedGenerateSegMap)
	ON_BN_CLICKED(IDC_LOCK_GDATA, &CGridMapDlg::OnBnClickedLockGData)
  ON_BN_CLICKED(IDC_COMMIT_GDATA, &CGridMapDlg::OnBnClickedCommitGData)
  ON_BN_CLICKED(IDC_RELOAD_GDATA, &CGridMapDlg::OnBnClickedReloadGData)
  ON_BN_CLICKED(IDC_REVERT_GDATA, &CGridMapDlg::OnBnClickedRevertGData)
  ON_BN_CLICKED(IDC_SAVE_GDATA, &CGridMapDlg::OnBnClickedSaveGData)
	ON_BN_CLICKED(IDC_AUTO_LEVEL, &CGridMapDlg::OnBnClickedAutoLevel)
	ON_BN_CLICKED(IDC_AUTO_LEVEL_RESET, &CGridMapDlg::OnBnClickedAutoLevelReset)
	ON_BN_CLICKED(IDC_BTN_SW_MODE, &CGridMapDlg::OnBnClickedBtnSwMode)
	ON_MESSAGE(WM_USER_ON_DBL_CLICK, &CGridMapDlg::OnDoubleClickedInMapWnd)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CGridMapDlg::DoDataExchange(CDataExchange* pDX)
{
  CXTResizeDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_GRID_X, m_EditX);
  //DDX_Control(pDX, IDC_GDINFO_EDIT, m_EditGDStatus);
  DDX_Control(pDX, IDC_GRID_MAP, m_MapWnd);
	DDX_Control(pDX, IDC_LSTCTRL_SW_GD, m_GDListCtrl);
	DDX_Control(pDX, IDC_BTN_SW_MODE, m_btnSWMode);
}

CGridMapDlg::CGridMapDlg(CWnd *pParent)
: CXTResizeDialog(CGridMapDlg::IDD, pParent)
{
	m_pInstance = this;

	Create( IDD, pParent );
}

CGridMapDlg::~CGridMapDlg()
{
	SaveLocations(m_strWorldName);

	if(CSWMiniMapUpdater::Get())
  {
    CSWMiniMapUpdater::Get()->SetSWMiniMapUpdaterCallback(NULL);
		CSWMiniMapUpdater::Get()->SetSWMiniMapLockStatusUpdaterCallback(NULL);
		CSWMiniMapUpdater::Get()->SetSWGDLockStatusUpdaterCallBack(NULL);
  }

  m_pInstance = 0;
}

CGridMapDlg *CGridMapDlg::Get()
{
	return m_pInstance;
}

void CGridMapDlg::OnCancel()
{
  AUTO_LOCK(CGridMapDlg::s_cs);
  CXTResizeDialog::OnCancel();
  delete this;
}

void CGridMapDlg::OnOK()
{
  AUTO_LOCK(CGridMapDlg::s_cs);
  CXTResizeDialog::OnOK();
  delete this;
}

BOOL CGridMapDlg::OnInitDialog()
{
  CXTResizeDialog::OnInitDialog();
  
	m_GDListCtrl.SetExtendedStyle(m_GDListCtrl.GetStyle()|LVS_EX_CHECKBOXES);
	m_GDListCtrl.Init();

	m_MapWnd.Init();

	CSWManager* pSWMgr = GetIEditor()->GetSegmentedWorldManager();
	OnSWModeChange(pSWMgr->GetMode());

  SetWindowText(m_strWorldName);
  m_pnodeLocations = NULL;

	CRect rcMap;
  m_MapWnd.GetWindowRect(&rcMap);
  ScreenToClient(&rcMap);

  CRect rcRect;
  GetWindowRect(&rcRect);
  m_rcOrgRect = rcRect;
  m_nMapWndDH = rcRect.Height() - rcMap.Height();
  m_nMapWndDW = rcRect.Width() - rcMap.Width();

  SetResize(IDC_GRID_MAP, SZ_RESIZE(1));

  SetResize(IDC_LOCK_SEGMENTS, SZ_HORREPOS(1));
  SetResize(IDC_UNLOCK_SEGMENTS, SZ_HORREPOS(1));
  SetResize(IDC_ROLLBACK_SEGMENTS, SZ_HORREPOS(1));
  SetResize(IDC_REVERT_SEGMENTS, SZ_HORREPOS(1));
  SetResize(IDC_RESOLVE_SEGMENTS, SZ_HORREPOS(1));

  SetResize(IDC_LSTCTRL_SW_GD, SZ_HORREPOS(1));
  SetResize(IDC_RELOAD_GDATA, SZ_HORREPOS(1));
  SetResize(IDC_LOCK_GDATA, SZ_HORREPOS(1));
  SetResize(IDC_SAVE_GDATA, SZ_HORREPOS(1));
  SetResize(IDC_COMMIT_GDATA, SZ_HORREPOS(1));
  SetResize(IDC_REVERT_GDATA, SZ_HORREPOS(1));

  SetResize(IDC_STATIC_POS, SZ_HORREPOS(1));
  SetResize(IDC_GRID_X, SZ_HORREPOS(1));
  SetResize(IDC_GO, SZ_HORREPOS(1));

  SetResize(IDC_STATIC_LOC, SZ_HORREPOS(1));
  SetResize(IDC_LOCATIONS, SZ_HORREPOS(1));
  SetResize(IDC_LOC_ADD, SZ_HORREPOS(1));
  SetResize(IDC_LOC_DEL, SZ_HORREPOS(1));

  SetResize(IDC_CHECKING_MODE, SZ_HORREPOS(1));

  SetResize(IDC_UNSELECT_ALL_SEGMENTS, SZ_HORREPOS(1));
  SetResize(IDC_BTN_SW_MODE, SZ_HORREPOS(1));
	
  UpdatePosition();

	//Create the ToolTip control
	if( !m_ToolTip.Create(this))
	{
		TRACE0("Unable to create the ToolTip!");
	}
	else
	{
		// Add tool tips to the controls, either by hard coded string 
		// or using the string table resource
		//m_ToolTip.AddTool( &m_GDListCtrl, _T("tooltip #1"));

		m_ToolTip.SetMaxTipWidth(0); //this is hack for creating multiline tooltip with predefined rows
		m_ToolTip.AddTool( GetDlgItem(IDC_RESOLVE_SEGMENTS), _T("Resolve conflicts (preffer not use)"));
		m_ToolTip.AddTool( GetDlgItem(IDC_LOCK_GDATA), _T("Lock selected Global data types"));
		m_ToolTip.AddTool( GetDlgItem(IDC_REVERT_SEGMENTS), _T("Revert segments and objects to last save"));
		//m_ToolTip.AddTool( GetDlgItem(IDC_GLOBAL_DATA_LABEL), _T("tooltip #4"));
		//m_ToolTip.AddTool( GetDlgItem(IDC_GDINFO_EDIT), _T("tooltip #5"));
		m_ToolTip.AddTool( GetDlgItem(IDC_COMMIT_GDATA), _T("Commit selected Global data types"));
		m_ToolTip.AddTool( GetDlgItem(IDC_RELOAD_GDATA), _T("Reload selected Global data types"));
		m_ToolTip.AddTool( GetDlgItem(IDC_REVERT_GDATA), _T("Rollback selected Global\r\n data types to official version\r\n (NOTE: Some Global data types\r\n need to rollback all existing\r\n global data types)"));
		m_ToolTip.AddTool( GetDlgItem(IDC_SAVE_GDATA), _T("Save selected Global data types"));
		m_ToolTip.AddTool( GetDlgItem(IDC_STATIC_POS), _T("tooltip #12"));
		m_ToolTip.AddTool( GetDlgItem(IDC_GRID_X), _T("tooltip #13"));
		m_ToolTip.AddTool( GetDlgItem(IDC_GO), _T("tooltip #14"));
		//m_ToolTip.AddTool( GetDlgItem(IDC_STATIC_SEL), _T("tooltip #16"));
		m_ToolTip.AddTool( GetDlgItem(IDC_STATIC_LOC), _T("tooltip #20"));
		m_ToolTip.AddTool( GetDlgItem(IDC_LOCATIONS), _T("tooltip #21"));
		m_ToolTip.AddTool( GetDlgItem(IDC_LOC_ADD), _T("tooltip #22"));
		m_ToolTip.AddTool( GetDlgItem(IDC_LOC_DEL), _T("tooltip #23"));
		m_ToolTip.AddTool( GetDlgItem(IDC_LOCK_SEGMENTS), _T("Lock segments in loaded area"));
		m_ToolTip.AddTool( GetDlgItem(IDC_UNLOCK_SEGMENTS), _T("Save and Commit modified segments and objects"));
		m_ToolTip.AddTool( GetDlgItem(IDC_ROLLBACK_SEGMENTS), _T("Rolback to official version"));
		m_ToolTip.AddTool( GetDlgItem(IDC_LSTCTRL_SW_GD), _T("Different Global Data types status and selections"));
		m_ToolTip.AddTool( GetDlgItem(IDC_CHECKING_MODE), _T("Switch between \"select mode\" and \"area mode\"\n(preffer use only for minimap generation\n and segment locking)"));
		m_ToolTip.AddTool( GetDlgItem(IDC_UNSELECT_ALL_SEGMENTS), _T("Clear selection (for select mode)"));
	
		m_ToolTip.Activate(TRUE);
	}

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
		m_statusBar.SetPaneWidth(i, 200);
	}
	
  return TRUE;
}

void CGridMapDlg::UpdatePosition()
{
  CHAR chBuf[255];
  Vec2 pt = m_MapWnd.GetPos();
  sprintf(chBuf, "%.0f,%.0f", pt.x, pt.y);
  m_EditX.SetWindowText(chBuf);
}

void CGridMapDlg::UpdateWorld()
{
  m_MapWnd.UpdateWorld();
}

void CGridMapDlg::SendImage(int nSegmentID, CImage* pImage)
{
	AUTO_LOCK(CSWMiniMapUpdater::g_cs);
  CRect rect;
  CSWMiniMapUpdater::SegIdToWorldRect(nSegmentID, rect);
  m_MapWnd.SetLayerImage(1, nSegmentID, pImage, &rect, false);
  //m_MapWnd.UpdateWorld();

  // auto level by default
  OnBnClickedAutoLevel();
}

void CGridMapDlg::SetSegInfoUpdateSuspended(bool bSuspend)
{
	m_MapWnd.m_bSegInfoUpdateSuspended = bSuspend;
}

void CGridMapDlg::SetStatus(std::map<int, DWORD> &stateMap, std::map<int,TSegmentState> &mapSegStates)
{
	AUTO_LOCK(CSWMiniMapUpdater::g_cs);
	for (std::map<int, TSegmentState>::iterator itx = mapSegStates.begin(); itx != mapSegStates.end(); ++itx)
		m_MapWnd.m_mapSegmentStates[itx->first] = itx->second;


	for (std::map<int, TSegmentState>::iterator itx = m_MapWnd.m_mapSegmentStates.begin(); itx != m_MapWnd.m_mapSegmentStates.end();)
	{
		std::map<int, TSegmentState>::iterator itCur = itx; ++itx;
		
		if (mapSegStates.find(itCur->first) == mapSegStates.end())
			m_MapWnd.m_mapSegmentStates.erase(itCur);
	}

	bool bChanged = false;
	for (CMapWnd::TSegmentMap::iterator it1 = m_MapWnd.m_SegmentsStatus.begin(); it1 != m_MapWnd.m_SegmentsStatus.end(); ++it1)
		stateMap[it1->first];
	
	for (CMapWnd::TSegmentMap::iterator it2 = stateMap.begin(); it2 != stateMap.end(); ++it2) 
	{
		CPoint ptPos;
		SegIDToWorldCoordPoint(ptPos, it2->first);
		if (m_MapWnd.SetSegmentState(m_MapWnd.m_SegmentsStatus, ptPos, it2->second))
			bChanged = true;
	}

	if (bChanged)
		m_MapWnd.Invalidate();

}
void CGridMapDlg::SetGDStatus(int nWDBType, int nWDBState, int nLockedBy, const char* pchLockedBy)
{
	m_GDListCtrl.SetGDState(nWDBType, nWDBState, nLockedBy, pchLockedBy);
}
void CGridMapDlg::SetWorld(const char* pchWorldName)
{
	CSWMiniMapUpdater::s_bSuspend = true;
	AUTO_LOCK(CSWMiniMapUpdater::g_cs);
	if (strcmpi(m_strWorldName, pchWorldName) != 0)
	{
		m_MapWnd.DelLayer(1);
		m_strWorldName.Format("%s", pchWorldName);
		//m_MapWnd.Init();
		CRect rect = m_MapWnd.GetRectMt();
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

BOOL CGridMapDlg::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
  if (nID == 1) {
    CWnd *pw = GetFocus();
    if (pw && pw->m_hWnd == m_EditX.m_hWnd) {
      OnBnClickedGo();
      this->SetFocus();
    }
    return true;
  }

  return CXTResizeDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CGridMapDlg::OnBnClickedGo()
{
  CString sText;
  m_EditX.GetWindowText(sText);
  float x,y;
  sscanf(sText, "%f,%f", &x, &y);
  m_MapWnd.SetPos(Vec2(x, y));
  m_MapWnd.SetFocus();
}

void CGridMapDlg::OnBnClickedLoad()
{
  OnOK();
}

void CGridMapDlg::OnBnClickedCancel()
{
  OnCancel();
}

void CGridMapDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
  m_MapWnd.SetFocus();
}

void CGridMapDlg::OnSize(UINT nType, int cx, int cy)
{
	CXTResizeDialog::OnSize(nType, cx, cy);

	if(m_MapWnd.m_hWnd)
	{
		CRect rcMap;
		m_MapWnd.GetWindowRect(&rcMap);
		ScreenToClient(&rcMap);
		m_MapWnd.m_offscreenBitmap.DeleteObject();
		m_MapWnd.m_offscreenLayer.DeleteObject();
		m_MapWnd.m_selectionBitmap.DeleteObject();
		int nHeight = cy - m_nMapWndDH;
		int nWidth = cx - m_nMapWndDW;

		Vec2 posmt = m_MapWnd.GetPos(&CPoint(0,0));
		m_MapWnd.MoveWindow(rcMap.left, rcMap.top, nWidth, nHeight);
		m_MapWnd.SetPos(posmt, &CPoint(0,0));

		m_MapWnd.UpdateSelection(m_MapWnd.m_ptMousePos);

		Invalidate();
	}
	
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST,AFX_IDW_CONTROLBAR_LAST,0);
}

void CGridMapDlg::OnLocationShow()
{
  CComboBox *pCombo = (CComboBox*)GetDlgItem(IDC_LOCATIONS);
  if (!pCombo)
    return;

  int idx = pCombo->GetCurSel();
  if (idx < 0)
    return;

  if (!m_pnodeLocations)
    return;

  XmlNodeRef location = m_pnodeLocations->getChild(idx);
  if (! location)
    return;

  Vec2 pos; 
  location->getAttr("pos", pos);

  m_MapWnd.SetPos(pos);

  // remember the last selected location index
  pCombo->SetItemData(0, idx);

  m_MapWnd.SetFocus();
}

void CGridMapDlg::OnLocationUpdate()
{
  CComboBox *pCombo = (CComboBox*)GetDlgItem(IDC_LOCATIONS);
  if (!pCombo)
    return;

  // restore the last selected location index
  int idx = pCombo->GetItemData(0);
  if (idx < 0) 
    return;

  if (!m_pnodeLocations)
    return;

  XmlNodeRef location = m_pnodeLocations->getChild(idx);
  if (!location)
    return;

  CString sNewName;
  pCombo->GetWindowText(sNewName);

  location->setAttr("name", sNewName);
  pCombo->DeleteString(idx);
  pCombo->InsertString(idx, sNewName);
}

void CGridMapDlg::OnLocationAdd()
{
  CComboBox *pCombo = (CComboBox*)GetDlgItem(IDC_LOCATIONS);
  if (!pCombo)
    return;

  if (!m_pnodeLocations)
    m_pnodeLocations = GetISystem()->CreateXmlNode("WorldMapLocations");

  int cnt = m_pnodeLocations->getChildCount();
  char name[255]; 
  sprintf(name, "location_%d", cnt);
  XmlNodeRef newloc = m_pnodeLocations->newChild(name);
  newloc->setAttr("name", name);
  newloc->setAttr("pos", m_MapWnd.m_ptDrawWorldPosMt);

  pCombo->InsertString(cnt, name);
  pCombo->SetCurSel(cnt);
  OnLocationShow();
}

void CGridMapDlg::OnLocationDel()
{
  CComboBox *pCombo = (CComboBox*)GetDlgItem(IDC_LOCATIONS);
  if (!pCombo)
    return;

  int idx = pCombo->GetCurSel();
  if (idx < 0)
    return;

  if (!m_pnodeLocations)
    return;

  pCombo->DeleteString(idx);
  pCombo->SetItemData(0, -1);

  XmlNodeRef location = m_pnodeLocations->getChild(idx);
  if (!location)
    return;

  m_pnodeLocations->removeChild(location);
}

void CGridMapDlg::OnBnClickedCheckingMode()
{
  CButton *pBtn = (CButton*)GetDlgItem(IDC_CHECKING_MODE);
  if (!pBtn)
    return;

  if (pBtn->GetCheck()) {
    m_MapWnd.SetEditMode(EGEM_CHECK);
    m_MapWnd.SetSelecionSize(m_MapWnd.m_nCheckSize);
	//m_statusBar.SetPaneText(0, "Check Mode");
  } else {
    CButton *pBtn = (CButton*)GetDlgItem(IDC_CHECKING_MODE);
    if (pBtn)
      pBtn->SetCheck(0);
    m_MapWnd.SetEditMode(EGEM_EDIT);
    m_MapWnd.SetSelecionSize(m_MapWnd.m_nEditSize);
	//m_statusBar.SetPaneText(0, "Edit Mode");
  }

  m_MapWnd.SetFocus();

  m_MapWnd.Invalidate();
}

void CGridMapDlg::OnBnClickedLockSegments()
{
  GetISystem()->GetIConsole()->ExecuteString("sw lock");
  m_MapWnd.m_SegmentsCheck.clear();
  m_MapWnd.SetFocus();
  m_MapWnd.Invalidate();
}

void CGridMapDlg::OnBnClickedUnlockSegments()
{
  GetISystem()->GetIConsole()->ExecuteString("sw commit");
  m_MapWnd.m_SegmentsCheck.clear();
  m_MapWnd.SetFocus();
  m_MapWnd.Invalidate();
}

void CGridMapDlg::OnBnClickedResolveSegments()
{
  GetISystem()->GetIConsole()->ExecuteString("sw resolve");
  m_MapWnd.m_SegmentsCheck.clear();
  m_MapWnd.SetFocus();
  m_MapWnd.Invalidate();
}

void CGridMapDlg::OnBnClickedUnselectAllSegments()
{
  m_MapWnd.m_SegmentsCheck.clear();
  m_MapWnd.SetFocus();
  m_MapWnd.Invalidate();
}

void CGridMapDlg::OnBnClickedRevertSegments()
{
  GetISystem()->GetIConsole()->ExecuteString("sw revert");
  m_MapWnd.m_SegmentsCheck.clear();
  m_MapWnd.SetFocus();
  m_MapWnd.Invalidate();
}

void CGridMapDlg::OnBnClickedRollbackSegments()
{
  GetISystem()->GetIConsole()->ExecuteString("sw revert ov");
  m_MapWnd.m_SegmentsCheck.clear();
  m_MapWnd.SetFocus();
  m_MapWnd.Invalidate();
}
void CGridMapDlg::OnBnClickedGenerateSegMap()
{
	GetISystem()->GetIConsole()->ExecuteString("sw map gen");
	m_MapWnd.m_SegmentsCheck.clear();
	m_MapWnd.SetFocus();
	m_MapWnd.Invalidate();
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
	AUTO_LOCK(s_cs);
	arrWDBTypes.clear();
	for(int nItem =0 ; nItem <  m_GDListCtrl.GetItemCount(); nItem++)
	{
		BOOL bChecked = m_GDListCtrl.GetCheck(nItem);
		if( bChecked )
		{
			arrWDBTypes.push_back(nItem);
		}
	}
}

void CGridMapDlg::UnSelectedGDs()
{
	AUTO_LOCK(s_cs);
	for(int nItem =0 ; nItem <  m_GDListCtrl.GetItemCount(); nItem++)
	{
		m_GDListCtrl.SetCheck(nItem, FALSE);
	}
}

void CGridMapDlg::OnBnClickedLockGData()
{
	AUTO_LOCK(s_cs);
	std::vector<int> arrWDBTypes;
	GetSelectedGDs(arrWDBTypes);
	for (int i = 0; i < arrWDBTypes.size(); i++)
	{
		string strWDBCC;
		strWDBCC.Format("sw wdb %d %d", arrWDBTypes[i], (int)sw::WDBCC_Lock);
		GetISystem()->GetIConsole()->ExecuteString(strWDBCC.c_str());
	}
  
}

void CGridMapDlg::OnBnClickedCommitGData()
{
	AUTO_LOCK(s_cs);
	std::vector<int> arrWDBTypes;
	GetSelectedGDs(arrWDBTypes);
	//if (arrWDBTypes.size() > 0 && !stl::find(arrWDBTypes, WDB_LEVELPAK))
	//{
	//	arrWDBTypes.push_back(WDB_LEVELPAK);
	//}
	for (int i = 0; i < arrWDBTypes.size(); i++)
	{
		string strWDBCC;
		strWDBCC.Format("sw wdb %d %d", arrWDBTypes[i], (int)sw::WDBCC_Commit);
		GetISystem()->GetIConsole()->ExecuteString(strWDBCC.c_str());
	}
	UnSelectedGDs();
}

void CGridMapDlg::OnBnClickedReloadGData()
{
	AUTO_LOCK(s_cs);
	std::vector<int> arrWDBTypes;
	GetSelectedGDs(arrWDBTypes);
	
	for (int i = 0; i < arrWDBTypes.size(); i++)
	{
		string strWDBCC;
		strWDBCC.Format("sw wdb %d %d", arrWDBTypes[i], (int)sw::WDBCC_Revert);
		GetISystem()->GetIConsole()->ExecuteString(strWDBCC.c_str());
		if (arrWDBTypes[i] == WDB_LEVELGENERAL || arrWDBTypes[i] == WDB_TIMEOFDAY)
			return;
	}
}

void CGridMapDlg::OnBnClickedRevertGData()
{
	AUTO_LOCK(s_cs);
	std::vector<int> arrWDBTypes;
	GetSelectedGDs(arrWDBTypes);
	for (int i = 0; i < arrWDBTypes.size(); i++)
	{
		string strWDBCC;
		strWDBCC.Format("sw wdb %d %d", arrWDBTypes[i], (int)sw::WDBCC_Rollback);
		GetISystem()->GetIConsole()->ExecuteString(strWDBCC.c_str());
		if (arrWDBTypes[i] == WDB_LEVELGENERAL || arrWDBTypes[i] == WDB_TIMEOFDAY)
			break;
	}
	UnSelectedGDs();
}

void CGridMapDlg::OnBnClickedSaveGData()
{
	AUTO_LOCK(s_cs);
	std::vector<int> arrWDBTypes;
	GetSelectedGDs(arrWDBTypes);
	for (int i = 0; i < arrWDBTypes.size(); i++)
	{
		string strWDBCC;
		strWDBCC.Format("sw wdb %d %d", arrWDBTypes[i], (int)sw::WDBCC_Save);
		GetISystem()->GetIConsole()->ExecuteString(strWDBCC.c_str());
	}
}

//------------------------------------------------------
BOOL CGridMapDlg::PreTranslateMessage(MSG* pMsg)
{
	if(m_ToolTip.GetSafeHwnd() != 0)
		m_ToolTip.RelayEvent(pMsg);

	return CXTResizeDialog::PreTranslateMessage(pMsg);
}
void CGridMapDlg::OnBnClickedAutoLevel()
{
	for (CMapWnd::TLayersMap::iterator it = m_MapWnd.m_Layers.begin(); it != m_MapWnd.m_Layers.end(); ++it)
	{
		DWORD dwLayerId = it->first;
		CMapWnd::TLayer& layer = it->second;
		layer.GetHistogramMinMax(&layer.autoLevelMin, &layer.autoLevelMax);
		for (CMapWnd::TImgagesMap::iterator it2 = layer.mImages.begin(); it2 != layer.mImages.end(); ++it2)
		{
			DWORD dwBitmapId = it2->first;
			m_MapWnd.UpdateLayerBitmap(dwLayerId, dwBitmapId);
		}
	}

	m_MapWnd.SetFocus();
  m_MapWnd.Invalidate();
}

void CGridMapDlg::OnBnClickedAutoLevelReset()
{
	for (CMapWnd::TLayersMap::iterator it = m_MapWnd.m_Layers.begin(); it != m_MapWnd.m_Layers.end(); ++it)
	{
		DWORD dwLayerId = it->first;
		CMapWnd::TLayer& layer = it->second;
		layer.autoLevelMin = 0;
		layer.autoLevelMax = 255;
		for (CMapWnd::TImgagesMap::iterator it2 = layer.mImages.begin(); it2 != layer.mImages.end(); ++it2)
		{
			DWORD dwBitmapId = it2->first;
			m_MapWnd.UpdateLayerBitmap(dwLayerId, dwBitmapId);
		}
	}

	m_MapWnd.SetFocus();
  m_MapWnd.Invalidate();
}

void CGridMapDlg::OnBnClickedBtnSwMode()
{
	if (gEnv->pConsole->GetCVar("doc_segmented_world_mode")->GetIVal() == 0)
		gEnv->pConsole->GetCVar("doc_segmented_world_mode")->Set(1);
	else
		gEnv->pConsole->GetCVar("doc_segmented_world_mode")->Set(0);
}

void CGridMapDlg::OnBnClickedDelete()
{
	CSWManager* pSWMgr = GetIEditor()->GetSegmentedWorldManager();

	std::vector<CPoint> selectedPoints;
	m_MapWnd.PrepareSegmentsForDelete(selectedPoints);
	pSWMgr->RemoveSegments(selectedPoints, m_strWorldName);

	UpdateWorldBounds();
}

// fully update WorldMap when level changed
void CGridMapDlg::OnLevelChanged()
{
  ISegmentedWorldDoc& swdoc = GetIEditor()->GetSegmentedWorldManager()->GetDoc();
  if (swdoc.IsOk())
  {
	  // Save location file for previous level
	  SaveLocations(m_strWorldName);

	  // Apply new world name
	  SetWorld(swdoc.GetWorldName());
	  SetWindowText(m_strWorldName);

	  // Load location file from new level
	  LoadLocations(m_strWorldName);

	  sw::TViewStates& viewStates = swdoc.GetViewStates();

	  m_MapWnd.m_ptDrawWorldPosMt = viewStates.m_ptWorldMapOfs;
	  //pDlg->m_MapWnd.m_nZoomLevel = m_nWorldMapZoom;
	  m_MapWnd.SetZoomLevel(viewStates.m_nWorldMapZoom);
	  UpdatePosition();

	  uint32 nWidth,nHeight;
	  int32 iOffX,iOffY;
	  swdoc.GetSizeInSegments(nWidth,nHeight);
	  swdoc.GetOffsetInSegments(iOffX,iOffY);
	  m_MapWnd.SetEditSeg(iOffX, iOffY, nWidth, nHeight);

  }
}

LRESULT CGridMapDlg::OnDoubleClickedInMapWnd(WPARAM wParam, LPARAM lParam)
{
	switch(m_MapWnd.GetEditMode())
	{
	case EGEM_EDIT:
		MoveToSelected();
		break;

	case EGEM_MERGE:
		MergeToSelected();
		m_MapWnd.RestoreEditMode();
		break;

	default:
		break;
	}
	return TRUE;
}

bool CGridMapDlg::IsSelectMode() const
{
	return m_MapWnd.IsEditMode(EGEM_CHECK);
}

bool CGridMapDlg::IsOpenForEditMode() const
{
	return m_MapWnd.IsEditMode(EGEM_EDIT);
}

bool CGridMapDlg::IsSegmentSelected( CPoint const& pt ) const
{
	return m_MapWnd.GetSegmentState(m_MapWnd.m_SegmentsCheck, pt);
}

void CGridMapDlg::SetEditArea( int32 x0,int32 y0, int32 w, int32 h )
{
	m_MapWnd.SetEditSeg(x0,y0,w,h);
}

int CGridMapDlg::GetEditSize()
{
	return m_MapWnd.GetSelectionSize();
}

void CGridMapDlg::SetEditSize(int size)
{
	m_MapWnd.SetSelecionSize(size);
}

void CGridMapDlg::OnDestroy()
{
	CXTResizeDialog::OnDestroy();
}

void CGridMapDlg::PostNcDestroy()
{
	CXTResizeDialog::PostNcDestroy();
	delete this;
	m_pInstance = 0;
}

void CGridMapDlg::LoadLocations(const CString &worldName)
{
	if(worldName.IsEmpty())
		return;

	CComboBox *pComboBox = (CComboBox*)GetDlgItem(IDC_LOCATIONS);
	if (!pComboBox)
		return;

	pComboBox->ResetContent();

	CString locFile;
	locFile.Format("Levels/%s/%s_editor/Locations.xml", worldName, worldName);
	
	m_pnodeLocations = GetISystem()->LoadXmlFromFile(locFile);
	if (!m_pnodeLocations)
		return;
	
	int cnt = m_pnodeLocations->getChildCount();

	for(int i = 0; i < cnt; i++)
	{
		CString name = "";
		m_pnodeLocations->getChild(i)->getAttr("name", name);
		pComboBox->InsertString(i, name);
	}
}

void CGridMapDlg::SaveLocations(const CString &worldName)
{
	if(worldName.IsEmpty() || !m_pnodeLocations)
		return;

	CString locFile;
	locFile.Format("Levels/%s/%s_editor/Locations.xml", worldName, worldName);

	m_pnodeLocations->saveToFile(locFile);
}

bool CGridMapDlg::OpenWorldInteranl(CString &outWorldName)
{
	CString docFilename;
	CString currentLevelsPath = Path::GetExecutableParentDirectory() + "\\" + Path::GetGameFolder() + "\\Levels";
	if(!CFileUtil::SelectFile("sw.xml", currentLevelsPath, docFilename))
		return false;

	// Levels outside of Game/Levels won't be opened
	if (0 != docFilename.Left(currentLevelsPath.GetLength()).CompareNoCase(currentLevelsPath))
		return false;

	CString strRelative = docFilename.Mid(currentLevelsPath.GetLength()+1);
	outWorldName = strRelative.Left(strRelative.FindOneOf("/\\"));

	return true;
}

bool CGridMapDlg::UpdateWorldBounds()
{
	CRect rcBound;
	CString levelpath = Path::GetGameFolder() + "\\Levels\\" + m_strWorldName + "\\";
	if(!CSWManager::GetWorldBounds(levelpath, rcBound))
	{
		m_statusBar.SetPaneText(0, "Failed to get world bounds from current level");
		return false;
	}
	rcBound.right += 1;
	rcBound.bottom += 1;

	m_MapWnd.UpdateWorldBound(rcBound, levelpath);

	return true;
}

void CGridMapDlg::OnOpenWorld()
{
	CString strWorldName;
	if(!OpenWorldInteranl(strWorldName))
		return;

	SetWorld(strWorldName);
	
	UpdateWorldBounds();

	m_MapWnd.SetFocus();
}

void CGridMapDlg::OnMergeLevels()
{
	if(m_strWorldName.IsEmpty())
	{
		MessageBox("Please open a segmented world level before merge!", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	
	if(!OpenWorldInteranl(m_strMergeWorldName))
		return;

	if(!strcmpi(m_strMergeWorldName, m_strWorldName))
	{
		MessageBox("Cannot merge to self!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	CRect rc;
	CString levelpath = Path::GetGameFolder() + "\\Levels\\" + m_strMergeWorldName + "\\";
	if(!CSWManager::GetWorldBounds(levelpath, rc))
	{
		MessageBox("Failed to get world bounds from segmentinfo.xml!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	m_MapWnd.m_nMergeSize = Vec2i(rc.right - rc.left + 1, rc.bottom - rc.top + 1);
	m_MapWnd.SetEditMode(EGEM_MERGE);
	m_statusBar.SetPaneText(0, "Merge Mode");
}

bool CGridMapDlg::MoveToSelected()
{
	m_MapWnd.SetFocus();

	if (m_MapWnd.m_iEditSegW <= 0 || m_MapWnd.m_iEditSegH <= 0)
		return false;

	CSWManager* pSWMgr = GetIEditor()->GetSegmentedWorldManager();

	bool result = false;
	{
		CRect rcWorld(CPoint(m_MapWnd.m_iEditSegX,m_MapWnd.m_iEditSegY),CSize(m_MapWnd.m_iEditSegW,m_MapWnd.m_iEditSegH));
		TWorldName wn;
		wn.Name = m_strWorldName;
		wn.isSegmented = true;

		if(wn.Name.empty())
			return false;

		if (!pSWMgr->IsSWDoc() || 0 != strcmpi(wn.Name.c_str(), pSWMgr->GetDoc().GetWorldName()) )
		{
			EOpenResult nOpenRet = pSWMgr->OpenWorld( wn, false, &rcWorld );
			assert(nOpenRet == OpenResult_NoError);
		}
		result = pSWMgr->GetDoc().MoveTo(rcWorld.left, rcWorld.top, rcWorld.Width(), rcWorld.Height());
	}

	{
		if(result)
		{
			m_statusBar.SetPaneText(0, "Open selected area successfully");
		}
		else
		{
			m_statusBar.SetPaneText(0, "Failed to open selected area");
		}
		m_MapWnd.Invalidate();
	}
	return true;
}

bool CGridMapDlg::MergeToSelected()
{
	CRect rc;
	m_MapWnd.PrepareSegmentsForMerge(rc);

	CSWManager* pSWMgr = GetIEditor()->GetSegmentedWorldManager();
	if(!pSWMgr->MergeLevel(m_strWorldName, m_strMergeWorldName,
		Vec2i(rc.left, rc.top),
		Vec2i(0, 0),
		m_MapWnd.m_nMergeSize)
		)
	{
		m_statusBar.SetPaneText(0, "Merge failed");
		return false;
	}

	UpdateWorldBounds();

	return true;
}

//////////////////////////////////////////////////////////////////////////
// CGridMap Editor
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CGridMapEditor, CBaseFrameWnd)

BEGIN_MESSAGE_MAP(CGridMapEditor, CBaseFrameWnd)
  ON_COMMAND(ID_SW_OPEN, OnOpenWorld)
  ON_COMMAND(ID_SW_MOVE_TO_SELECTED, OnMoveToSelected)
  ON_COMMAND(ID_SW_GENERATEMAP, OnGenerateMap)
  ON_COMMAND(ID_SW_DEL_SELECTED, OnDelSelected)
  ON_COMMAND(ID_SW_MERGE, OnMergeSelected)
  ON_COMMAND(ID_SW_AUTOCOLOR, OnAutoColor)
  ON_COMMAND(ID_SW_RESETCOLOR, OnResetColor)
  ON_COMMAND_RANGE(ID_SW_GRID_1K, ID_SW_GRID_16K, OnSetMapSize)
  ON_UPDATE_COMMAND_UI_RANGE(ID_SW_GRID_1K, ID_SW_GRID_16K, OnUpdateMapSizeUI)
END_MESSAGE_MAP()

CGridMapEditor::CGridMapEditor()
: m_pGridMapDlg(NULL)
{
	CRect rc(0,0,0,0);
	Create(WS_CHILD|WS_VISIBLE, rc, AfxGetMainWnd());
}

CGridMapEditor::~CGridMapEditor()
{
}

BOOL CGridMapEditor::OnInitDialog()
{
	CMFCUtils::LoadShortcuts(GetCommandBars(), IDR_GRID_MAP, "Grid Map Editor");

	// Create and setup the heightmap edit viewport and the toolbars
	GetCommandBars()->GetCommandBarsOptions()->bShowExpandButtonAlways = FALSE;
	GetCommandBars()->EnableCustomization(FALSE);

	CXTPCommandBar* pMenuBar = GetCommandBars()->SetMenu(_T("Menu Bar"), IDR_GRID_MAP);
	pMenuBar->SetFlags(xtpFlagStretched);
	pMenuBar->EnableCustomization(TRUE);

	CXTPToolBar *pToolBar1 = GetCommandBars()->Add( _T("ToolBar1"), xtpBarTop);
	pToolBar1->EnableCustomization(FALSE);
	VERIFY(pToolBar1->LoadToolBar( IDR_GRID_MAP_BAR ));

	CXTPToolBar *pToolBar2 = GetCommandBars()->Add(_T("ToolBar2"), xtpBarTop);
	pToolBar2->EnableCustomization(FALSE);
	VERIFY(pToolBar2->LoadToolBar( IDR_GRID_SIZE_BAR ));
	
	DockRightOf(pToolBar2,pToolBar1);

	CXTPDockingPane *pDockPane = GetDockingPaneManager()->CreatePane(IDW_GRID_MAP_PANE, CRect(0, 0, 1200, 600), dockLeftOf);
	pDockPane->SetOptions(xtpPaneNoCloseable|xtpPaneNoCaption|xtpPaneNoHideable|xtpPaneNoFloatable);

	m_pGridMapDlg = new CGridMapDlg(this);
	assert(m_pGridMapDlg);

	return TRUE;
}

LRESULT CGridMapEditor::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
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
				pwndDockWindow->Attach(m_pGridMapDlg);
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

void CGridMapEditor::OnOpenWorld()
{
	m_pGridMapDlg->OnOpenWorld();
}

void CGridMapEditor::OnMoveToSelected()
{
	m_pGridMapDlg->MoveToSelected();
}

void CGridMapEditor::OnGenerateMap()
{
	m_pGridMapDlg->OnBnClickedGenerateSegMap();
}

void CGridMapEditor::OnDelSelected()
{
	m_pGridMapDlg->OnBnClickedDelete();
}

void CGridMapEditor::OnMergeSelected()
{
	m_pGridMapDlg->OnMergeLevels();
}

void CGridMapEditor::OnAutoColor()
{
	m_pGridMapDlg->OnBnClickedAutoLevel();
}

void CGridMapEditor::OnResetColor()
{
	m_pGridMapDlg->OnBnClickedAutoLevelReset();
}

void CGridMapEditor::OnSetMapSize(UINT id)
{
	UINT index = id - ID_SW_GRID_1K;
	
	if( index <= ID_SW_GRID_16K )
	{
		m_pGridMapDlg->SetEditSize(index);
	}
}

void CGridMapEditor::OnUpdateMapSizeUI(CCmdUI *pCmdUI)
{
	UINT index = pCmdUI->m_nID - ID_SW_GRID_1K;
	
	if( index <= ID_SW_GRID_16K )
	{
		int nEdit = m_pGridMapDlg->GetEditSize();
		pCmdUI->SetCheck( nEdit == index ? TRUE : FALSE );
	}
}
