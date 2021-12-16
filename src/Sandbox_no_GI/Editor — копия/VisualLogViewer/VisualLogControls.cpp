// VisualLogControls.cpp : implementation file
//

#include "stdafx.h"
#include "Resource.h"
#include "VisualLogControls.h"

#define IDC_BTN_COLORPICK		0x100
#define ID_PLAYBACK_TIMER		1



// VisualLogDialog
IMPLEMENT_DYNAMIC(CVisualLogDialog, CXTResizeDialog)

BEGIN_MESSAGE_MAP(CVisualLogDialog, CXTResizeDialog)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_HSCROLL()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_VLB_FOLDER, &CVisualLogDialog::OnBnFolderClicked)
	ON_BN_CLICKED(IDC_VLCHK_KEEPASPECTR, &CVisualLogDialog::OnBnClickedVLchkKeepaspectr)
	ON_BN_CLICKED(IDC_VLCHK_UPDATETEXT, &CVisualLogDialog::OnBnClickedVLchkUpdatetext)
	ON_BN_CLICKED(IDC_VLCHK_UPDATEIMAGES, &CVisualLogDialog::OnBnClickedVLchkUpdateimages)
	ON_CPN_XT_SELENDOK(IDC_BTN_COLORPICK, &CVisualLogDialog::OnBnClickedColorPicker)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()



// CVisualLogDialog construction & destruction
CVisualLogDialog::CVisualLogDialog(CWnd *pParent /* = NULL */):
	CXTResizeDialog(CVisualLogDialog::IDD, pParent)
	, m_strFolder(_T(""))
	, m_bUpdateText(FALSE)
	, m_bKeepAspectR(FALSE)
	, m_bUpdateImages(FALSE)
{
	m_bmPlayerBtns.LoadBitmap(IDB_VL_PLAYER);
	m_pCD = 0;
	m_rcPlayer.SetRect(10, 10, 166, 36);
	m_nEnabled			= eVLPB_None;
	m_eDown					= eVLPB_None;
	m_nPlayerClick	= 0;

	CRect rcTarget(11, 11, 35, 35);
	for (int i = 0; i < 6; ++i)
	{
		m_arrButtons[i] = rcTarget;
		m_arrButtons[i].OffsetRect(26*i, 0);
	}
}

CVisualLogDialog::~CVisualLogDialog()
{
}



// CVisualLogDialog operations
void CVisualLogDialog::EnablePlayerButtons()
{
	if (m_pCD->eState == eVLPS_Uninitialized)
		m_nEnabled = eVLPB_None;
	else if (m_pCD->eState == eVLPS_Playing)
		m_nEnabled = eVLPB_Stop;
	else
	{
		m_nEnabled = eVLPB_Play;
		if (m_pCD->nCurFrame > 0)
			m_nEnabled |= eVLPB_First | eVLPB_Prev;
		if (m_pCD->nCurFrame < m_pCD->nLastFrame)
			m_nEnabled |= eVLPB_Next | eVLPB_Last;
	}

	m_eDown = eVLPB_None;
	DrawPlayerButtons(eVLPB_All);
}

void CVisualLogDialog::DrawPlayerButtons(EVLogPlayerButtons eb)
{
	InvalidateRect(m_rcPlayer);

	static COLORREF clrBR = RGB(200,200,200), clrTL = RGB(60,60,60), clrDis = RGB(175,175,175);
	static CBitmap bmNull;
	static CRect rcTarget(1, 1, 25, 25);

	if (eb & eVLPB_First)
	{
		static POINT arr[] =
		{
			{23, 3},
			{3, 3},
			{23, 23},
		};

		if (m_nEnabled & eVLPB_First)	// enabled
		{
			m_dcControlBoard.PlgBlt(arr, &m_dcPlayerBtns, 120, 0, 20, 20, bmNull, 0, 0);
			if (m_eDown == eVLPB_First)
				m_dcControlBoard.Draw3dRect(rcTarget, clrTL, clrBR);	// down
			else
				m_dcControlBoard.Draw3dRect(rcTarget, clrBR, clrTL);	// up
		}
		else	// disabled
		{
			m_dcControlBoard.PlgBlt(arr, &m_dcPlayerBtns, 140, 0, 20, 20, bmNull, 0, 0);
			m_dcControlBoard.Draw3dRect(rcTarget, clrDis, clrDis);
		}
	}
	if (eb & eVLPB_Prev)
	{
		static POINT arr[] =
		{
			{49, 3},
			{29, 3},
			{49, 23},
		};
		CRect rcTmp(rcTarget);
		rcTmp.OffsetRect(26, 0);

		if (m_nEnabled & eVLPB_Prev)	// enabled
		{
			m_dcControlBoard.PlgBlt(arr, &m_dcPlayerBtns, 80, 0, 20, 20, bmNull, 0, 0);
			if (m_eDown == eVLPB_Prev)
				m_dcControlBoard.Draw3dRect(rcTmp, clrTL, clrBR);	// down
			else
				m_dcControlBoard.Draw3dRect(rcTmp, clrBR, clrTL);	// up
		}
		else	// disabled
		{
			m_dcControlBoard.PlgBlt(arr, &m_dcPlayerBtns, 100, 0, 20, 20, bmNull, 0, 0);
			m_dcControlBoard.Draw3dRect(rcTmp, clrDis, clrDis);
		}
	}
	if (eb & eVLPB_Play)
	{
		CRect rcTmp(rcTarget);
		rcTmp.OffsetRect(2*26, 0);

		if (m_nEnabled & eVLPB_Play)	// enabled
		{
			m_dcControlBoard.BitBlt(2*26+3, 3, 20, 20, &m_dcPlayerBtns, 0, 0, SRCCOPY);
			if (m_eDown == eVLPB_Play)
				m_dcControlBoard.Draw3dRect(rcTmp, clrTL, clrBR);	// down
			else
				m_dcControlBoard.Draw3dRect(rcTmp, clrBR, clrTL);	// up
		}
		else	// disabled
		{
			m_dcControlBoard.BitBlt(2*26+3, 3, 20, 20, &m_dcPlayerBtns, 20, 0, SRCCOPY);
			m_dcControlBoard.Draw3dRect(rcTmp, clrDis, clrDis);
		}
	}
	if (eb & eVLPB_Stop)
	{
		CRect rcTmp(rcTarget);
		rcTmp.OffsetRect(3*26, 0);

		if (m_nEnabled & eVLPB_Stop)	// enabled
		{
			m_dcControlBoard.BitBlt(3*26+3, 3, 20, 20, &m_dcPlayerBtns, 40, 0, SRCCOPY);
			if (m_eDown == eVLPB_Stop)
				m_dcControlBoard.Draw3dRect(rcTmp, clrTL, clrBR);	// down
			else
				m_dcControlBoard.Draw3dRect(rcTmp, clrBR, clrTL);	// up
		}
		else	// disabled
		{
			m_dcControlBoard.BitBlt(3*26+3, 3, 20, 20, &m_dcPlayerBtns, 60, 0, SRCCOPY);
			m_dcControlBoard.Draw3dRect(rcTmp, clrDis, clrDis);
		}
	}
	if (eb & eVLPB_Next)
	{
		CRect rcTmp(rcTarget);
		rcTmp.OffsetRect(4*26, 0);

		if (m_nEnabled & eVLPB_Next)	// enabled
		{
			m_dcControlBoard.BitBlt(4*26+3, 3, 20, 20, &m_dcPlayerBtns, 80, 0, SRCCOPY);
			if (m_eDown == eVLPB_Next)
				m_dcControlBoard.Draw3dRect(rcTmp, clrTL, clrBR);	// down
			else
				m_dcControlBoard.Draw3dRect(rcTmp, clrBR, clrTL);	// up
		}
		else	// disabled
		{
			m_dcControlBoard.BitBlt(4*26+3, 3, 20, 20, &m_dcPlayerBtns, 100, 0, SRCCOPY);
			m_dcControlBoard.Draw3dRect(rcTmp, clrDis, clrDis);
		}
	}
	if (eb & eVLPB_Last)
	{
		CRect rcTmp(rcTarget);
		rcTmp.OffsetRect(5*26, 0);

		if (m_nEnabled & eVLPB_Last)	// enabled
		{
			m_dcControlBoard.BitBlt(5*26+3, 3, 20, 20, &m_dcPlayerBtns, 120, 0, SRCCOPY);
			if (m_eDown == eVLPB_Last)
				m_dcControlBoard.Draw3dRect(rcTmp, clrTL, clrBR);	// down
			else
				m_dcControlBoard.Draw3dRect(rcTmp, clrBR, clrTL);	// up
		}
		else	// disabled
		{
			m_dcControlBoard.BitBlt(5*26+3, 3, 20, 20, &m_dcPlayerBtns, 140, 0, SRCCOPY);
			m_dcControlBoard.Draw3dRect(rcTmp, clrDis, clrDis);
		}
	}
}

void CVisualLogDialog::OnPlayerFirst()
{
	m_pCD->nCurFrame = 0;
	EnablePlayerButtons();
	GetOwner()->SendMessage(UWM_BUTTON_CLICKED, 0, eVLDML_FrameChanged);	// Navigation

	m_sldFrame.SetPos(m_pCD->nCurFrame);
	CString strTmp;
	strTmp.Format("%d/%d", m_pCD->nCurFrame, m_pCD->nLastFrame);
	SetDlgItemText(IDC_VLSTATIC_FRAME, strTmp);
}

void CVisualLogDialog::OnPlayerPrev()
{
	m_pCD->nCurFrame--;
	EnablePlayerButtons();
	GetOwner()->SendMessage(UWM_BUTTON_CLICKED, 0, eVLDML_FrameChanged);	// Navigation

	m_sldFrame.SetPos(m_pCD->nCurFrame);
	CString strTmp;
	strTmp.Format("%d/%d", m_pCD->nCurFrame, m_pCD->nLastFrame);
	SetDlgItemText(IDC_VLSTATIC_FRAME, strTmp);
}

void CVisualLogDialog::OnPlayerPlay()
{
	SetTimer(ID_PLAYBACK_TIMER, 10, 0);

	m_pCD->eState = eVLPS_Playing;
	EnablePlayerButtons();
	m_nTimerVar = 1;
}

void CVisualLogDialog::OnPlayerStop()
{
	KillTimer(ID_PLAYBACK_TIMER);
	m_pCD->eState = eVLPS_Active;
	EnablePlayerButtons();
}

void CVisualLogDialog::OnPlayerNext()
{
	m_pCD->nCurFrame++;
	EnablePlayerButtons();
	GetOwner()->SendMessage(UWM_BUTTON_CLICKED, 0, eVLDML_FrameChanged);	// Navigation

	m_sldFrame.SetPos(m_pCD->nCurFrame);
	CString strTmp;
	strTmp.Format("%d/%d", m_pCD->nCurFrame, m_pCD->nLastFrame);
	SetDlgItemText(IDC_VLSTATIC_FRAME, strTmp);
}

void CVisualLogDialog::OnPlayerLast()
{
	m_pCD->nCurFrame = m_pCD->nLastFrame;
	EnablePlayerButtons();
	GetOwner()->SendMessage(UWM_BUTTON_CLICKED, 0, eVLDML_FrameChanged);	// Navigation

	m_sldFrame.SetPos(m_pCD->nCurFrame);
	CString strTmp;
	strTmp.Format("%d/%d", m_pCD->nCurFrame, m_pCD->nLastFrame);
	SetDlgItemText(IDC_VLSTATIC_FRAME, strTmp);
}



// CSnapPreviewDialog virtual overrides
void CVisualLogDialog::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_VLEDIT_FOLDERPATH, m_strFolder);
	DDX_Control(pDX, IDC_VLSLIDER_SPEED, m_sldSpeed);
	DDX_Control(pDX, IDC_VLSLIDER_CURFRAME, m_sldFrame);
	DDX_Check(pDX, IDC_VLCHK_UPDATETEXT, m_bUpdateText);
	DDX_Check(pDX, IDC_VLCHK_KEEPASPECTR, m_bKeepAspectR);
	DDX_Check(pDX, IDC_VLCHK_UPDATEIMAGES, m_bUpdateImages);
	DDX_Control(pDX, IDC_BTN_COLORPICK, m_colorPickBtn);
}

BOOL CVisualLogDialog::OnInitDialog()
{
	// Create the color pick button (before calling base::OnInitDialog())
	CRect rcColor, rcBtn;
	GetDlgItem(IDC_VLCHK_UPDATETEXT)->GetWindowRect(rcColor);
	ScreenToClient(rcColor);
	rcBtn.SetRect(rcColor.right + 25, rcColor.top, rcColor.right + 120, rcColor.top + 35);
	m_colorPickBtn.Create("Background", BS_TEXT | BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE, rcBtn, this, IDC_BTN_COLORPICK);
	m_colorPickBtn.ShowText(TRUE);
	m_colorPickBtn.SetColor(RGB(0,0,0));
	m_colorPickBtn.SetDefaultColor(RGB(0,0,0));
	m_colorPickBtn.SetOwner(this);

	CXTResizeDialog::OnInitDialog();

	// Initialize dialog controls
	m_strFolder = "not set";
	SetResize(IDC_VLEDIT_FOLDERPATH, SZ_TOP_LEFT, SZ_TOP_RIGHT);
	SetResize(IDC_VLSLIDER_CURFRAME, SZ_TOP_LEFT, SZ_TOP_RIGHT);

	m_sldSpeed.EnableWindow(FALSE);
	m_sldSpeed.SetRange(-100, 100);
	m_sldSpeed.SetPos(100);
	m_sldFrame.EnableWindow(FALSE);

	SetDlgItemText(IDC_VLSTATIC_SPEED, "0.00");

	m_bUpdateText		= TRUE;
	m_bKeepAspectR	= TRUE;
	m_bUpdateImages	= TRUE;
	UpdateData(FALSE);

	GetDlgItem(IDC_VLCHK_UPDATETEXT)->EnableWindow(FALSE);
	GetDlgItem(IDC_VLCHK_UPDATEIMAGES)->EnableWindow(FALSE);
	GetDlgItem(IDC_VLCHK_KEEPASPECTR)->EnableWindow(FALSE);

	// Create necessary DCs and bitmaps
	CClientDC dc(this);
	m_dcControlBoard.CreateCompatibleDC(&dc);
	m_dcPlayerBtns.CreateCompatibleDC(&dc);
	m_bmControlBoard.CreateCompatibleBitmap(&dc, 156, 26);
	m_pBmCB = m_dcControlBoard.SelectObject(&m_bmControlBoard);
	m_pBmPB = m_dcPlayerBtns.SelectObject(&m_bmPlayerBtns);

	m_dcControlBoard.SelectStockObject(DC_PEN);
	m_dcControlBoard.SelectStockObject(DC_BRUSH);
	m_dcControlBoard.SetDCPenColor(RGB(0,0,0));
	m_dcControlBoard.SetDCBrushColor(RGB(150,150,150));
	m_dcControlBoard.Rectangle(0,0,156,26);
	m_dcControlBoard.SelectStockObject(NULL_BRUSH);

	DrawPlayerButtons(eVLPB_All);

	m_brSlider.CreateSolidBrush(RGB(200,200,205));

	// Create the tool tips
	m_ctlTT.CreateEx(this, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, WS_EX_TOPMOST);
	m_ctlTT.SetDelayTime(500);
	static LPCTSTR szToolTips[] = { "First Frame", "Previous Frame", "Play", "Stop", "Next Frame", "Last Frame" };
	for (int i = 0; i < 6; ++i)
		m_ctlTT.AddRectTool(this, szToolTips[i], m_arrButtons[i], 1);

	return TRUE;
}



// CSnapPreviewDialog message handlers
void CVisualLogDialog::OnPaint()
{
	CPaintDC dc(this);

	dc.BitBlt(10, 10, 156, 26, &m_dcControlBoard, 0, 0, SRCCOPY);
}

void CVisualLogDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nEnabled & eVLPB_First) && m_arrButtons[0].PtInRect(point))	// kSVPB_FIRST
	{
		SetCapture();
		m_nPlayerClick = 1;
		m_eDown = eVLPB_First;
		DrawPlayerButtons(eVLPB_First);
	}
	else if ((m_nEnabled & eVLPB_Prev) && m_arrButtons[1].PtInRect(point))	// kSVPB_PREV
	{
		SetCapture();
		m_nPlayerClick = 2;
		m_eDown = eVLPB_Prev;
		DrawPlayerButtons(eVLPB_Prev);
	}
	else if ((m_nEnabled & eVLPB_Play) && m_arrButtons[2].PtInRect(point))	// kSVPB_PLAY
	{
		SetCapture();
		m_nPlayerClick = 3;
		m_eDown = eVLPB_Play;
		DrawPlayerButtons(eVLPB_Play);
	}
	else if ((m_nEnabled & eVLPB_Stop) && m_arrButtons[3].PtInRect(point))	// kSVPB_STOP
	{
		SetCapture();
		m_nPlayerClick = 4;
		m_eDown = eVLPB_Stop;
		DrawPlayerButtons(eVLPB_Stop);
	}
	else if ((m_nEnabled & eVLPB_Next) && m_arrButtons[4].PtInRect(point))	// kSVPB_NEXT
	{
		SetCapture();
		m_nPlayerClick = 5;
		m_eDown = eVLPB_Next;
		DrawPlayerButtons(eVLPB_Next);
	}
	else if ((m_nEnabled & eVLPB_Last) && m_arrButtons[5].PtInRect(point))	// kSVPB_LAST
	{
		SetCapture();
		m_nPlayerClick = 6;
		m_eDown = eVLPB_Last;
		DrawPlayerButtons(eVLPB_Last);
	}

	CXTResizeDialog::OnLButtonDown(nFlags, point);
}

void CVisualLogDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_nPlayerClick)
	{
		if (m_arrButtons[m_nPlayerClick-1].PtInRect(point) && (m_eDown == eVLPB_None))
		{
			switch (m_nPlayerClick)
			{
			case 1:	m_eDown = eVLPB_First;	break;
			case 2:	m_eDown = eVLPB_Prev;	break;
			case 3:	m_eDown = eVLPB_Play;	break;
			case 4:	m_eDown = eVLPB_Stop;	break;
			case 5:	m_eDown = eVLPB_Next;	break;
			case 6:	m_eDown = eVLPB_Last;	break;
			}
			DrawPlayerButtons(m_eDown);
		}
		else if (!m_arrButtons[m_nPlayerClick-1].PtInRect(point) && (m_eDown != eVLPB_None))
		{
			EVLogPlayerButtons eTmp = m_eDown;
			m_eDown = eVLPB_None;
			DrawPlayerButtons(eTmp);
		}
	}

	CXTResizeDialog::OnMouseMove(nFlags, point);
}

void CVisualLogDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_nPlayerClick)
	{
		EVLogPlayerButtons eTmp = m_eDown;
		m_eDown = eVLPB_None;
		DrawPlayerButtons(eTmp);
		::ReleaseCapture();

		switch (m_nPlayerClick)
		{
		case 1:
			if (m_arrButtons[m_nPlayerClick-1].PtInRect(point))
				OnPlayerFirst();
			break;
		case 2:
			if (m_arrButtons[m_nPlayerClick-1].PtInRect(point))
				OnPlayerPrev();
			break;
		case 3:
			if (m_arrButtons[m_nPlayerClick-1].PtInRect(point))
				OnPlayerPlay();
			break;
		case 4:
			if (m_arrButtons[m_nPlayerClick-1].PtInRect(point))
				OnPlayerStop();
			break;
		case 5:
			if (m_arrButtons[m_nPlayerClick-1].PtInRect(point))
				OnPlayerNext();
			break;
		case 6:
			if (m_arrButtons[m_nPlayerClick-1].PtInRect(point))
				OnPlayerLast();
			break;
		}

		m_nPlayerClick = 0;
	}

	CXTResizeDialog::OnLButtonUp(nFlags, point);
}

void CVisualLogDialog::OnBnFolderClicked()
{
	CXTBrowseDialog dlg(this);

	if (m_pCD->eState == eVLPS_Playing)
		OnPlayerStop();

	dlg.SetOptions(BIF_NONEWFOLDERBUTTON);
	if (IDOK == dlg.DoModal())
	{
		// Working folder
		m_pCD->strFolder = dlg.GetSelPath();
		if ((m_pCD->strFolder[m_pCD->strFolder.GetLength() - 1] != '\\') &&
			(m_pCD->strFolder[m_pCD->strFolder.GetLength() - 1] != '/'))
		{
			m_pCD->strFolder += "\\";
		}

		if (GetOwner()->SendMessage(UWM_BUTTON_CLICKED, 0, eVLDML_CheckValidFolder))	// If folder is OK
		{
			// Enable some controls
			m_sldFrame.EnableWindow();
			m_sldFrame.SetRange(0, m_pCD->nLastFrame);
			m_sldFrame.SetPos(0);
			m_sldSpeed.EnableWindow();

			GetDlgItem(IDC_VLCHK_UPDATETEXT)->EnableWindow(m_pCD->bHasText  ?  TRUE : FALSE);
			GetDlgItem(IDC_VLCHK_UPDATEIMAGES)->EnableWindow(m_pCD->bHasImages  ?  TRUE : FALSE);
			GetDlgItem(IDC_VLCHK_KEEPASPECTR)->EnableWindow(TRUE);

			CString strTmp;
			strTmp.Format("0/%d", m_pCD->nLastFrame);
			SetDlgItemText(IDC_VLSTATIC_FRAME, strTmp);

			int nPos = m_sldSpeed.GetPos();
			strTmp.Format("%0.2f", 1000.f/((101 - abs(nPos))*10.f));
			if (nPos < 0)
				strTmp = "-" + strTmp;
			SetDlgItemText(IDC_VLSTATIC_SPEED, strTmp);

			m_strFolder = m_pCD->strFolder;
			UpdateData(FALSE);

			EnablePlayerButtons();
		}
		else
		{
			AfxMessageBox("Not a valid folder!", MB_OK | MB_ICONEXCLAMATION);

			GetDlgItem(IDC_VLCHK_UPDATETEXT)->EnableWindow(FALSE);
			GetDlgItem(IDC_VLCHK_UPDATEIMAGES)->EnableWindow(FALSE);
			GetDlgItem(IDC_VLCHK_KEEPASPECTR)->EnableWindow(FALSE);

			m_pCD->eState = eVLPS_Uninitialized;
			m_pCD->vecFiles.clear();

			m_sldFrame.EnableWindow(FALSE);
			m_sldSpeed.EnableWindow(FALSE);

			SetDlgItemText(IDC_VLSTATIC_FRAME, "");
			SetDlgItemText(IDC_VLSTATIC_SPEED, "0.00");

			m_strFolder = "not set";
			UpdateData(FALSE);

			EnablePlayerButtons();
		}
	}
}

void CVisualLogDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar == reinterpret_cast<CScrollBar *>(&m_sldFrame))
	{
		if (m_pCD->eState == eVLPS_Playing)
			OnPlayerStop();
		if (m_pCD->nCurFrame != m_sldFrame.GetPos())
		{
			m_pCD->nCurFrame = m_sldFrame.GetPos();

			CString strTmp;
			strTmp.Format("%d/%d", m_pCD->nCurFrame, m_pCD->nLastFrame);
			SetDlgItemText(IDC_VLSTATIC_FRAME, strTmp);

			GetOwner()->SendMessage(UWM_BUTTON_CLICKED, 0, eVLDML_FrameChanged);
			EnablePlayerButtons();
		}
	}
	else if (pScrollBar == reinterpret_cast<CScrollBar *>(&m_sldSpeed))
	{
		int nPos = m_sldSpeed.GetPos();

		if (nPos == 0)
			SetDlgItemText(IDC_VLSTATIC_SPEED, "0.00");
		else
		{
			CString strTmp;
			strTmp.Format("%0.2f", 1000.f/((101 - abs(nPos))*10.f));
			if (nPos < 0)
				strTmp = "-" + strTmp;
			SetDlgItemText(IDC_VLSTATIC_SPEED, strTmp);
		}
	}
}

void CVisualLogDialog::OnBnClickedVLchkUpdatetext()
{
	UpdateData(TRUE);

	m_pCD->bUpdateTxt = (m_bUpdateText)  ?  true : false;
	GetOwner()->SendMessage(UWM_BUTTON_CLICKED, eVLDMW_Text, eVLDML_ForceInvalidate);
}

void CVisualLogDialog::OnBnClickedVLchkUpdateimages()
{
	UpdateData(TRUE);

	m_pCD->bUpdateImages = (m_bUpdateImages)  ?  true : false;
	GetOwner()->SendMessage(UWM_BUTTON_CLICKED, eVLDMW_View, eVLDML_ForceInvalidate);
}

void CVisualLogDialog::OnBnClickedVLchkKeepaspectr()
{
	UpdateData(TRUE);

	m_pCD->bKeepAspect = (m_bKeepAspectR)  ?  true : false;
	GetOwner()->SendMessage(UWM_BUTTON_CLICKED, eVLDMW_View, eVLDML_ForceInvalidate);
}

void CVisualLogDialog::OnBnClickedColorPicker()
{
	m_pCD->clrBack = m_colorPickBtn.GetColor();
	GetOwner()->SendMessage(UWM_BUTTON_CLICKED, eVLDMW_View, eVLDML_ForceInvalidate);
}

void CVisualLogDialog::OnDestroy()
{
	CXTResizeDialog::OnDestroy();

	m_dcPlayerBtns.SelectObject(m_pBmPB);
	m_dcControlBoard.SelectObject(m_pBmCB);
	if (m_pCD->eState == eVLPS_Playing)
		KillTimer(ID_PLAYBACK_TIMER);
}

void CVisualLogDialog::OnTimer(UINT_PTR nIDEvent)
{
	int nPos = m_sldSpeed.GetPos();	// -100 ... 100

	if (nPos == 0)	// Stopped state
	{
		CXTResizeDialog::OnTimer(nIDEvent);
		return;
	}

	int nSkipVal = 101 - abs(nPos);					// 100 ... 1
	if	(m_nTimerVar % nSkipVal == 0)
	{
		m_pCD->nCurFrame += (m_sldSpeed.GetPos() < 0)  ?  -1 : 1;
		if (m_pCD->nCurFrame < 0) m_pCD->nCurFrame = m_pCD->nLastFrame;
		else if (m_pCD->nCurFrame > m_pCD->nLastFrame) m_pCD->nCurFrame = 0;
		GetOwner()->SendMessage(UWM_BUTTON_CLICKED, 0, eVLDML_Invalidate);

		m_sldFrame.SetPos(m_pCD->nCurFrame);
		CString strTmp;
		strTmp.Format("%d/%d", m_pCD->nCurFrame, m_pCD->nLastFrame);
		SetDlgItemText(IDC_VLSTATIC_FRAME, strTmp);
	}

	m_nTimerVar++;
	if (m_nTimerVar > 100)
		m_nTimerVar = 1;

	CXTResizeDialog::OnTimer(nIDEvent);
}

HBRUSH CVisualLogDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CXTResizeDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	if ((pWnd->GetDlgCtrlID() == IDC_VLSLIDER_CURFRAME) && (pWnd->IsWindowEnabled()))
		return HBRUSH(m_brSlider.m_hObject);

	return hbr;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VisualLogControls docking frame
IMPLEMENT_DYNAMIC(CVisualLogControls, CToolbarDialog)

BEGIN_MESSAGE_MAP(CVisualLogControls, CToolbarDialog)
END_MESSAGE_MAP()



// SnapPreviewControls construction & destruction
CVisualLogControls::CVisualLogControls()
{
}

CVisualLogControls::~CVisualLogControls()
{
}



// SnapPreviewControls virtual overrides



// SnapPreviewControls message handlers
