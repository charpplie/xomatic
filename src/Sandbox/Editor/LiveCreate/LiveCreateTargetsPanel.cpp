#include "StdAfx.h"
#include "LiveCreateTargetsPanel.h"
#include "LiveCreate/EditorLiveCreate.h"
#include "LiveCreate/LiveCreateAddTargetsDlg.h"
#include "LiveCreate/LiveCreateEditConnectionDlg.h"
#include "Controls/MemDC.h"
#include "IIconManager.h"

#ifndef NO_LIVECREATE

BEGIN_MESSAGE_MAP(CLiveCreateTargetsList, CListBox)
	ON_WM_DRAWITEM_REFLECT()
	ON_WM_MEASUREITEM_REFLECT()
	ON_WM_COMPAREITEM_REFLECT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_CREATE()
	ON_WM_SETCURSOR()
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	ON_WM_CONTEXTMENU()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CLiveCreateTargetsList, CListBox)

CLiveCreateTargetsList::Bitmaps::Bitmaps(const char* szBaseFileName)
{
	bool alphaFlag;
	m_pNormal = GetIEditor()->GetIconManager()->GetIconBitmap(szBaseFileName, alphaFlag, 0);
	m_pDisabled = GetIEditor()->GetIconManager()->GetIconBitmap(szBaseFileName, alphaFlag, eIconEffect_Dim);
	m_pConnected = GetIEditor()->GetIconManager()->GetIconBitmap(szBaseFileName, alphaFlag, eIconEffect_TintGreen);
}

CLiveCreateTargetsList::Bitmaps::~Bitmaps()
{
	delete m_pNormal;
	delete m_pDisabled;
	delete m_pConnected;
}

CLiveCreateTargetsList::CLiveCreateTargetsList()
	: m_bIsInitialized(false)
	, m_pNormalFont(NULL)
	, m_pBoldFont(NULL)
	, m_pItalicFont(NULL)
{
}

CLiveCreateTargetsList::~CLiveCreateTargetsList()
{
}

void CLiveCreateTargetsList::DrawItem(LPDRAWITEMSTRUCT lpdis)
{
	LiveCreate::CEditorHostInfo* pHost = (LiveCreate::CEditorHostInfo*)GetItemDataPtr(lpdis->itemID);
	if(NULL == pHost)
		return;

	if (lpdis->itemAction == ODA_SELECT || lpdis->itemAction == ODA_DRAWENTIRE)
	{
		CRect rc = lpdis->rcItem;

		CDC dc;
		dc.Attach(lpdis->hDC);

		const RECT rect = lpdis->rcItem;

		// Background
		if (lpdis->itemState & ODS_SELECTED)
		{
			dc.FillSolidRect(&rect,GetSysColor(COLOR_HIGHLIGHT));
		}
		else
		{
			dc.FillSolidRect(&rect,GetSysColor(COLOR_3DFACE));
		}

		// Draw platform icon		
		TPlatformBitmaps::const_iterator it = m_platformBitmaps.find(pHost->GetPlatform()->GetPlatformName());
		if (it != m_platformBitmaps.end())
		{
			Bitmaps* pBitmaps = (*it).second;

			CBitmap* pBitmap = pBitmaps->m_pNormal;
			if (!pHost->IsEnabled())
			{
				pBitmap = pBitmaps->m_pDisabled;
			}
			else if (pHost->IsReady())
			{
				pBitmap = pBitmaps->m_pConnected;
			}

			if (NULL != pBitmap)
			{
				CDC mdc;
				mdc.CreateCompatibleDC(&dc);
				mdc.SelectObject(*pBitmap);

				BLENDFUNCTION bf;
				bf.BlendOp = AC_SRC_OVER;
				bf.BlendFlags = 0;
				bf.SourceConstantAlpha = 255;
				bf.AlphaFormat = AC_SRC_ALPHA;
				dc.AlphaBlend(rect.left + kItemMargin, rect.top + kItemMargin, 
					kItemIconSize, kItemIconSize, &mdc, 
					0, 0, kItemIconSize, kItemIconSize, bf);
			}
		}

		const uint leftX = rect.left + kItemMargin*2 + kItemIconSize;
		const uint textTopX = rect.top + kItemMargin - 3;

		// Platform name
		{
			RECT textRect;
			textRect.left = leftX;
			textRect.top = textTopX;
			textRect.right = textRect.left+1;
			textRect.bottom = textRect.top+1;
			dc.SelectObject(m_pBoldFont);
			dc.DrawText(pHost->GetTargetName(), &textRect, DT_NOCLIP);
		}

		// Address
		{
			RECT textRect;
			textRect.left = leftX;
			textRect.top = textTopX + kLineHeight;
			textRect.right = textRect.left+1;
			textRect.bottom = textRect.top+1;

			if (pHost->GetAddres().IsEmpty())
			{
				dc.SelectObject(m_pItalicFont);
				dc.SetTextColor(GetSysColor(COLOR_GRAYTEXT));
				dc.DrawText("Unknown IP", &textRect, DT_NOCLIP);
			}
			else
			{
				dc.SelectObject(m_pNormalFont);
				dc.SetTextColor(GetSysColor(COLOR_BTNTEXT));
				dc.DrawText(pHost->GetAddres(), &textRect, DT_NOCLIP);
			}
		}

		// Status text
		{
			RECT textRect;
			textRect.left = leftX;
			textRect.top = textTopX + kLineHeight*2;
			textRect.right = textRect.left+1;
			textRect.bottom = textRect.top+1;

			COLORREF textColor = GetSysColor(COLOR_GRAYTEXT);
			CString text = "UNKNONW";
		
			const LiveCreate::EHostStatus status = pHost->EvaluateStatus();
			switch (status)
			{
				case LiveCreate::eHostStatus_Dead: text = "DEAD"; break;
				case LiveCreate::eHostStatus_Offline: text = "OFFLINE"; break;
				case LiveCreate::eHostStatus_Online: text = "DISCONNECTED"; break;
				case LiveCreate::eHostStatus_Connected: text = "CONNECTED"; break;
				case LiveCreate::eHostStatus_Ready: text = "READY"; break;
			}

			dc.SelectObject(m_pNormalFont);
			dc.SetTextColor(textColor);
			dc.DrawText(text, &textRect, DT_NOCLIP);
		}

		// Bottom line
		{
			CPen linePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
			dc.SelectObject(&linePen);
			dc.MoveTo(rect.left,rect.bottom-1);
			dc.LineTo(rect.right,rect.bottom-1);
		}
	

		dc.Detach();
	}
}

void CLiveCreateTargetsList::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	lpMeasureItemStruct->itemHeight = kItemHeight;
}

int CLiveCreateTargetsList::CompareItem( LPCOMPAREITEMSTRUCT lpCompareItemStruct )
{
	return 0;
}

int CLiveCreateTargetsList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListBox::OnCreate(lpCreateStruct) == -1)
		return -1;

	Init();
		
	return 0;
}

BOOL CLiveCreateTargetsList::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	return CListBox::OnSetCursor(pWnd, nHitTest, message);
}

void CLiveCreateTargetsList::OnMouseMove(UINT nFlags, CPoint point)
{
	return CListBox::OnMouseMove(nFlags, point);
}

void CLiveCreateTargetsList::OnLButtonDown(UINT nFlags, CPoint point)
{
	CListBox::OnLButtonDown(nFlags, point);
}

void CLiveCreateTargetsList::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	const int selectedObject = GetCurSel();
	if (selectedObject != -1)
	{
		LiveCreate::CEditorHostInfo* pHost = (LiveCreate::CEditorHostInfo*)GetItemDataPtr(selectedObject);
		if (pHost != NULL)
		{
			CLiveCreateEditConnectionDlg dlg(this);

			CLiveCreateEditConnectionDlg::Parameters params;
			params.bAdding = false;
			params.targetName = pHost->GetTargetName();
			params.address = pHost->GetAddres();
			params.bIsEnabled = pHost->IsEnabled();
			params.buildDirectory = pHost->GetBuildDirectory();
			params.buildExecutable = pHost->GetBuildExecutable();
			params.platformName = pHost->GetPlatform()->GetPlatformName();
			params.pPlatform = pHost->GetPlatform();

			dlg.SetParameters(params);
			if (dlg.DoModal() == IDOK)
			{
				params = dlg.GetParameters();

				if (!params.address.IsEmpty())
				{
					pHost->SetAddress(params.address);
				}

				pHost->SetBuild(params.buildDirectory, params.buildExecutable);
			}
		}
	}
}

void CLiveCreateTargetsList::OnLButtonUp(UINT nFlags, CPoint point)
{
	return CListBox::OnLButtonUp(nFlags, point);
}

void CLiveCreateTargetsList::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_DELETE)
	{
		const int selectedObject = GetCurSel();
		if (selectedObject == -1)
		{
			return;
		}

		LiveCreate::CEditorHostInfo* pHost = (LiveCreate::CEditorHostInfo*)GetItemDataPtr(selectedObject);
		if (pHost == NULL)
		{
			return;
		}

		if (GetIEditor()->GetLiveCreate()->RemoveEntry(pHost))
		{
			DeleteString(selectedObject);
			pHost->Release();
			Invalidate(TRUE);
		}
	}
}

void CLiveCreateTargetsList::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// auto select clicked item
	{
		CPoint clientPoint = point;
		ScreenToClient(&clientPoint);

		const int count = GetCount();
		for (int i=0; i<count; ++i)
		{
			CRect rect;
			CListBox::GetItemRect(i, rect);
			if (rect.PtInRect(clientPoint))
			{
				SetCurSel(i);
				Invalidate(TRUE);
				UpdateWindow();
				break;
			}
		}
	}

	const int selectedObject = GetCurSel();
	if (selectedObject == -1)
	{
		return;
	}

	LiveCreate::CEditorHostInfo* pHost = (LiveCreate::CEditorHostInfo*)GetItemDataPtr(selectedObject);
	if (pHost == NULL)
	{
		return;
	}

	enum EOption
	{
		eOption_Remove=1,
		eOption_Reconnect,
		eOption_Disable,
		eOption_Enable,
	};

	CMenu menu;
	menu.CreatePopupMenu();

	// connection option
	if (pHost->IsConnected() || !pHost->GetAddres().IsEmpty())
	{
		menu.AppendMenu(MF_STRING, eOption_Reconnect, (const char*)"Reconnect");
		menu.AppendMenu(MF_SEPARATOR, 0, "");
	}

	if (pHost->IsEnabled())
	{
		menu.AppendMenu(MF_STRING, eOption_Disable, (const char*)"Disable");
	}
	else
	{
		menu.AppendMenu(MF_STRING, eOption_Enable, (const char*)"Enable");
	}

	menu.AppendMenu(MF_STRING, eOption_Remove, (const char*)"Remove");

	const UINT menuIndex = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, pWnd);
	if (menuIndex > 0)
	{
		if (menuIndex == eOption_Remove)
		{
			if (GetIEditor()->GetLiveCreate()->RemoveEntry(pHost))
			{
				DeleteString(selectedObject);
				pHost->Release();
				Invalidate(TRUE);
			}
		}
		else if (menuIndex == eOption_Reconnect)
		{
			if (NULL != pHost->GetHostInfo())
			{
				pHost->GetHostInfo()->Disconnect();
				pHost->GetHostInfo()->Connect();
				Invalidate(TRUE);
			}
			else
			{
				MessageBox("Target data is not valid.", "Reconnected", MB_ICONERROR);
			}
		}
		else if (menuIndex == eOption_Disable)
		{
			pHost->Enable(false);
			Invalidate(TRUE);
		}
		else if (menuIndex == eOption_Enable)
		{
			pHost->Enable(true);
			Invalidate(TRUE);
		}
	}

	menu.DestroyMenu();
}

void CLiveCreateTargetsList::OnPaint()
{
	CPaintDC dc(this);
	CRect rc;
	GetClientRect(rc);
	CMemoryDC memDC(dc, rc);

	CRect rcClip;
	memDC->GetClipBox(rcClip);
	memDC->FillSolidRect(rcClip, GetSysColor(COLOR_3DFACE));

	DefWindowProc(WM_PAINT, (WPARAM)memDC->m_hDC, (LPARAM)0);
}

BOOL CLiveCreateTargetsList::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

void CLiveCreateTargetsList::PreSubclassWindow()
{
	CListBox::PreSubclassWindow();

	Init();
}

void CLiveCreateTargetsList::InitializeFonts(CFont* pBaseFont)
{
	LOGFONT fontInfo;
	memset(&fontInfo, 0, sizeof(fontInfo));
	pBaseFont->GetLogFont(&fontInfo);

	// normal font
	m_pNormalFont = new CFont();
	m_pNormalFont->CreateFontIndirect(&fontInfo);

	// italic font
	fontInfo.lfItalic = true;
	m_pItalicFont = new CFont();
	m_pItalicFont->CreateFontIndirect(&fontInfo);

	// bold font
	fontInfo.lfItalic = false;
	fontInfo.lfWeight = 900;
	m_pBoldFont = new CFont();
	m_pBoldFont->CreateFontIndirect(&fontInfo);
}

void CLiveCreateTargetsList::Init()
{
	if (m_bIsInitialized)
		return;

	m_bIsInitialized = true;

	SetItemHeight(0,kItemHeight);

	m_platformBitmaps["PC"] = new Bitmaps("Editor/Icons/lc_pc.png");
	m_platformBitmaps["GamePC"] = new Bitmaps("Editor/Icons/lc_pc.png");
	m_platformBitmaps["X360"] = new Bitmaps("Editor/Icons/lc_x360.png");
	m_platformBitmaps["PS3"] = new Bitmaps("Editor/Icons/lc_ps3.png");
	m_platformBitmaps["PS4"] = new Bitmaps("Editor/Icons/lc_ps4.png");
	m_platformBitmaps["XboxONE"] = new Bitmaps("Editor/Icons/lc_xboxone.png");
	m_platformBitmaps["WiiU"] = new Bitmaps("Editor/Icons/lc_wiiu.png");
	m_platformBitmaps["Any"] = new Bitmaps("Editor/Icons/lc_any.png");

	CFont baseFont;
	baseFont.Attach(GetStockObject(DEFAULT_GUI_FONT));
	InitializeFonts(&baseFont);
}

//-----------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CLiveCreateTargetsPanel, CDialog)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_DISCOVER_PEERS, OnAddTargets)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_PEERS, OnRemoveZomibes)
END_MESSAGE_MAP();

CLiveCreateTargetsPanel::CLiveCreateTargetsPanel(CWnd* pParent /*= NULL*/)
{
	Create(IDD_IDD_LIVECREATE_SETTINGS_PANEL, pParent);

	GetIEditor()->RegisterNotifyListener(this);

	gEnv->pLiveCreateManager->RegisterListener(this);
}

CLiveCreateTargetsPanel::~CLiveCreateTargetsPanel()
{
	gEnv->pLiveCreateManager->UnregisterListener(this);

	GetIEditor()->UnregisterNotifyListener(this);
}

void CLiveCreateTargetsPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_DISCOVER_PEERS, m_btnAddHosts);
	DDX_Control(pDX, IDC_BUTTON_REMOVE_PEERS, m_btnCleanHosts);
}

void CLiveCreateTargetsPanel::OnDestroy()
{
	CDialog::OnDestroy();
	ClearList();
}

BOOL CLiveCreateTargetsPanel::OnInitDialog()
{
	RECT client;
	GetClientRect(&client);

	RECT placement;
	placement.left = client.left + kListMargin;
	placement.top = client.top + kListTopMargin;
	placement.right = client.right - kListMargin;
	placement.bottom = client.bottom - kListMargin;

	m_targets.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_OWNERDRAWFIXED, placement, this, IDC_LIST_PEERS);
	m_targets.ModifyStyleEx(0, WS_EX_CLIENTEDGE);
	m_targets.SetBkColor(GetSysColor(COLOR_BTNFACE));

	// create target items
	FillList();

	return TRUE;
}

void CLiveCreateTargetsPanel::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (m_targets.GetSafeHwnd() != NULL)
	{
		RECT client;
		GetClientRect(&client);

		RECT placement;
		placement.left = client.left + kListMargin;
		placement.top = client.top + kListTopMargin;
		placement.right = client.right - kListMargin;
		placement.bottom = client.bottom - kListMargin;

		m_targets.SetWindowPos(NULL, placement.left, placement.top, placement.right - placement.left, placement.bottom - placement.top,
			SWP_NOACTIVATE | SWP_NOMOVE);
	}
}

void CLiveCreateTargetsPanel::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
}

void CLiveCreateTargetsPanel::OnHostConnected(LiveCreate::IHostInfo* pHostInfo)
{
	m_targets.Invalidate();
}

void CLiveCreateTargetsPanel::OnHostDisconnected(LiveCreate::IHostInfo* pHostInfo)
{
	m_targets.Invalidate();
}

void CLiveCreateTargetsPanel::OnHostReady(LiveCreate::IHostInfo* pHostInfo)
{
	m_targets.Invalidate();
}

void CLiveCreateTargetsPanel::OnHostBusy(LiveCreate::IHostInfo* pHostInfo)
{
	m_targets.Invalidate();
}

void CLiveCreateTargetsPanel::OnAddTargets()
{
	CLiveCreateAddTargetsDlg dlg(this);
	if (dlg.DoModal() == IDOK)
	{
		bool bTargetsAdded = false;
		const CLiveCreateAddTargetsDlg::TOutputTargetList& targets = dlg.GetTargets();
		for (CLiveCreateAddTargetsDlg::TOutputTargetList::const_iterator it = targets.begin();
			it != targets.end(); ++it)
		{
			const CLiveCreateAddTargetsDlg::TargetInformation& info = *it;
			LiveCreate::CEditorHostInfo* pHost = GetIEditor()->GetLiveCreate()->AddHostEntry(info.m_platformName, info.m_targetName, info.m_validAddress);
			if (NULL != pHost)
			{
				// update build settings
				if (!info.m_buildExecutable.IsEmpty())
				{
					pHost->SetBuild(info.m_buildDirectory, info.m_buildExecutable);
				}

				bTargetsAdded = true;
			}
		}

		if (bTargetsAdded)
		{
			GetIEditor()->GetLiveCreate()->SaveSettings();
			FillList();
		}		
	}
}

void CLiveCreateTargetsPanel::ClearList()
{
	// Clean the list
	const uint32 size = m_targets.GetCount();
	for (uint32 i=0; i<size; ++i)
	{
		LiveCreate::CEditorHostInfo* pHost = (LiveCreate::CEditorHostInfo*)m_targets.GetItemDataPtr(i);
		SAFE_RELEASE(pHost);
	}
	m_targets.ResetContent();
}

void CLiveCreateTargetsPanel::FillList()
{
	ClearList();

	// Remove the zombie hosts
	std::vector<LiveCreate::CEditorHostInfo*> hosts;
	GetIEditor()->GetLiveCreate()->GetHosts(hosts);
	for (uint32 i=0; i<hosts.size(); ++i)
	{
		LiveCreate::CEditorHostInfo* pHost = hosts[i];
		int index = m_targets.AddString(pHost->GetTargetName());
		m_targets.SetItemDataPtr(index, pHost); // list keeps the reference
	}

	m_targets.Invalidate(TRUE);
}

void CLiveCreateTargetsPanel::OnRemoveZomibes()
{
	ClearList();

	// Remove the zombie hosts
	std::vector<LiveCreate::CEditorHostInfo*> hosts;
	GetIEditor()->GetLiveCreate()->GetHosts(hosts);
	for (uint32 i=0; i<hosts.size(); ++i)
	{
		LiveCreate::CEditorHostInfo* pHost = hosts[i];
		if (pHost->EvaluateStatus() == LiveCreate::eHostStatus_Dead)
		{
			GetIEditor()->GetLiveCreate()->RemoveEntry(pHost);
		}
		pHost->Release();
	}

	GetIEditor()->GetLiveCreate()->SaveSettings();
	FillList();
}


#endif

