////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:  MotionAdaptorDlg.cpp
//  Version:    v1.00
//  Created:    16/4/2009 by Xiaomao Wu.
//  Compiler:   Visual Studio 2005 Professional
//  Description:

// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MotionAdaptorDlg.h"
#include "..\CharacterEditor\ModelViewportCE.h"

#define IDC_TASKPANEL 1

#define PANE_EDIT_ITEM_ID_START 50
#define PANE_EDIT_ITEM_ID_END   52

#define ID_MOTIONADAPTOR_LOAD_V_FILES 100
#define ID_MOTIONADAPTOR_CONVERT_SELECTED 101
#define ID_MOTIONADAPTOR_SAVE_CONVERTED 102
#define ID_MOTIONADAPTOR_PLAY_PAUSE 103
#define ID_MOTIONADAPTOR_ANIM_PROGRESS_EDIT 104
#define ID_MOTIONADAPTOR_ANIM_PROGRESS_SLIDER 105
#define ID_MOTIONADAPTOR_FIX_INVERSE_BENDING 106
#define ID_MOTIONADAPTOR_MOTION_RETARGETING 107
#define ID_MOTIONADAPTOR_SHOW_VICON_MARKERS 109
#define ID_MOTIONADAPTOR_SHOW_VICON_SKELETON 110
#define ID_MOTIONADAPTOR_ADDITIVE_MOTION 112
#define ID_MOTIONADAPTOR_ADJUSTFOOT 113
#define ID_MOTIONADAPTOR_BATCH_CONVERT 114
#define ID_MOTIONADAPTOR_REPORT_CONTROL 1

// CMotionAdaptorDlg dialog

class VFileRecord : public CXTPReportRecord
{
public:
	VFileRecord(const string& name,const char* toolTips = NULL, float fLength=0.0f)
		:m_name(name)
	{
		CXTPReportRecordItem* pNewItem = new CXTPReportRecordItemText(name );
		pNewItem->SetTooltip(toolTips);
		AddItem(pNewItem);
	}
	const string& GetName() const { return m_name; }
protected:
	string m_name;
};


IMPLEMENT_DYNAMIC(CMotionAdaptorDlg, CToolbarDialog)
CMotionAdaptorDlg::CMotionAdaptorDlg(CWnd* pParent /*=NULL*/)
{
	m_pParentPane = NULL;
	m_MotionName = 0;
	m_ViconFrameRate = 0;
	m_BipedFrameRate = 0;
	m_BipedSpeed = 0;
	m_PlayPause = 0;

	Create( CMotionAdaptorDlg::IDD,pParent );
	m_wndTaskPanel.SetOwner(this);

}

CMotionAdaptorDlg::~CMotionAdaptorDlg()
{
}

void CMotionAdaptorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CMotionAdaptorDlg::CXTPTaskPanelSpecific::OnCommand(WPARAM wParam,LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case ID_MOTIONADAPTOR_FIX_INVERSE_BENDING :
		{
			m_poMyOwner->OnBnClickedFixInverseBending();
			return TRUE;
		}
	case ID_MOTIONADAPTOR_MOTION_RETARGETING:
		{
			m_poMyOwner->OnBnClickedMotionRetargeting();
			return TRUE;
		}
	case ID_MOTIONADAPTOR_ADDITIVE_MOTION:
		{
			m_poMyOwner->OnBnClickedAdditiveMotion();
			return TRUE;
		}
	case ID_MOTIONADAPTOR_ADJUSTFOOT:
		{
			m_poMyOwner->OnBnClickedAdjustFoot();
			return TRUE;
		}
	case ID_MOTIONADAPTOR_SHOW_VICON_MARKERS:
		{
			m_poMyOwner->OnBnClickedShowViconMarkers();
			return TRUE;
		}
	case ID_MOTIONADAPTOR_SHOW_VICON_SKELETON:
		{
			m_poMyOwner->onBnClickedShowViconSkeleton();
			return TRUE;
		}
	default:
		break;
	}

	return __super::OnCommand(wParam,lParam);
}

LRESULT CMotionAdaptorDlg::CXTPTaskPanelSpecific::WindowProc(UINT message,WPARAM wParam,LPARAM lParam)
{
	//switch(message)
	//{
	//case WM_HSCROLL:
	//	{
	//		CScrollBar	oScrollBar,*poScrollBarPointer(NULL);
	//		if (lParam!=NULL)
	//		{
	//			oScrollBar.FromHandle((HWND)lParam);
	//			poScrollBarPointer=&oScrollBar;
	//		}					
	//		//__super::OnHScroll(HIWORD(wParam),LOWORD(wParam),poScrollBarPointer);
	//		m_poMyOwner->OnHScroll(HIWORD(wParam),LOWORD(wParam),poScrollBarPointer);
	//		return 0;
	//	}
	//	break;
	//}
	return __super::WindowProc(message,wParam,lParam);
}

BOOL CMotionAdaptorDlg::OnInitDialog()
{
	BOOL res = __super::OnInitDialog();
	if (!res)
		return res;

	CRect rc;
	GetClientRect(rc);

	//------------------------------------------------------------------------------
	// Remove old items
	m_aPartEditPanes.resize(0);
	m_aPartEditPanes.resize(PANE_EDIT_ITEM_ID_END - PANE_EDIT_ITEM_ID_START + 1, NULL);
	CXTPDockingPaneManager* paneManager = ((CMainFrame*)AfxGetMainWnd())->GetDockingPaneManager();
	if(!paneManager)
		return FALSE;

	for (int paneID = PANE_EDIT_ITEM_ID_START; paneID <= PANE_EDIT_ITEM_ID_END; ++paneID)
	{
		paneManager->DestroyPane(paneID);
	}

	//-------------------------------------------------------------------------
	// Tasks panel
	m_wndTaskPanel.Create(WS_CHILD|WS_BORDER|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN, rc, this, IDC_TASKPANEL);
	m_wndTaskPanel.SetBehaviour(xtpTaskPanelBehaviourExplorer);
	m_wndTaskPanel.SetTheme(xtpTaskPanelThemeNativeWinXP);
	m_wndTaskPanel.SetSelectItemOnFocus(TRUE);
	m_wndTaskPanel.AllowDrag(FALSE);
	m_wndReport.SetMultipleSelection(TRUE);


	// V-files
	int32 groupID = 1;
	CXTPTaskPanelGroup* pGroup = NULL;
	CXTPTaskPanelGroupItem *pItem = NULL;
	{
		pGroup = m_wndTaskPanel.AddGroup(groupID);
		++groupID;
		pGroup->SetCaption(_T("V-Files"));

		CRect rc(0,0,100,170);

		m_wndReport.Create(WS_CHILD|WS_VISIBLE, rc, &m_wndTaskPanel, ID_MOTIONADAPTOR_REPORT_CONTROL);
		m_wndReport.ModifyStyle(0, WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_TABSTOP);
		m_wndReport.GetReportHeader()->AllowColumnRemove(FALSE);
		m_wndReport.Populate();
		m_wndReport.GetPaintManager()->SetGridStyle(TRUE, xtpReportGridSolid);
	

		pItem = pGroup->AddControlItem(m_wndReport);
		m_wndReport.BeginUpdate();
		CXTPReportColumn* pColumn;
		pColumn = m_wndReport.AddColumn(new CXTPReportColumn(0, _T("V-file Names"), 10));
		pColumn->SetAllowDrag(FALSE);
		pColumn->SetSortable(TRUE);
		m_wndReport.GetColumns()->SetSortColumn(pColumn, true);
		
		m_wndReport.EndUpdate();
		m_wndReport.Populate();

		class CMotionAdaptorReportCtrlPaintManager : public CXTPReportPaintManager
		{
		public:
			virtual int GetRowHeight(CDC* pDC, CXTPReportRow* pRow)
			{
				return __super::GetRowHeight(pDC,pRow)-1;
			}
		};

		CXTPReportPaintManager* pPMgr = new CXTPReportPaintManager();
		pPMgr->m_nTreeIndent = 0x0a;
		pPMgr->m_bShadeSortColumn = false;
		pPMgr->m_strNoItems = _T("No V-Files");
		pPMgr->SetGridStyle(FALSE, xtpGridNoLines);
		pPMgr->SetGridStyle(TRUE, xtpGridNoLines);
		m_wndReport.SetPaintManager( pPMgr );

		m_CurrBatchConvertName = pGroup->AddTextItem(_T("Batch info:"));	
	}
	
	// Operations
	{
		pGroup = m_wndTaskPanel.AddGroup(groupID);
		++groupID;
		pGroup->SetCaption(_T("Operations"));

		pItem = pGroup->AddLinkItem(ID_MOTIONADAPTOR_LOAD_V_FILES); pItem->SetType(xtpTaskItemTypeLink);
		pItem->SetCaption(_T("Load V-Files"));

		pItem = pGroup->AddLinkItem(ID_MOTIONADAPTOR_CONVERT_SELECTED); pItem->SetType(xtpTaskItemTypeLink);
		pItem->SetCaption(_T("Convert Selected Motions"));

		pItem = pGroup->AddLinkItem(ID_MOTIONADAPTOR_BATCH_CONVERT); pItem->SetType(xtpTaskItemTypeLink);
		pItem->SetCaption(_T("Batch conversion"));

		pItem = pGroup->AddLinkItem(ID_MOTIONADAPTOR_SAVE_CONVERTED); pItem->SetType(xtpTaskItemTypeLink);
		pItem->SetCaption(_T("Save Converted Motion"));
	}

	// Play Control
	{
		pGroup = m_wndTaskPanel.AddGroup(groupID); ++groupID;
		pGroup->SetCaption(_T("Play Control"));

		pItem = pGroup->AddTextItem(_T("Progress:"));

		m_AnimProgressEdit.Create(WS_CHILD|WS_VISIBLE|ES_CENTER, CRect(0, 0, 20, 16), this, ID_MOTIONADAPTOR_ANIM_PROGRESS_EDIT);
		m_AnimProgressEdit.SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
		m_AnimProgressEdit.SetParent(&m_wndTaskPanel);
		m_AnimProgressEdit.SetOwner(this);

		pItem = pGroup->AddControlItem(m_AnimProgressEdit);

		pItem = pGroup->AddTextItem(_T(""));

		m_AnimProgressSlider.Create(TBS_HORZ |TBS_NOTICKS, CRect(0, 0, 20, 16), this, ID_MOTIONADAPTOR_ANIM_PROGRESS_SLIDER);
		m_AnimProgressSlider.SetParent(&m_wndTaskPanel);
		m_AnimProgressSlider.SetOwner(this);
		m_AnimProgressSlider.SetRange(0, CProgressSlider::NUM_INCREMENTS);
		pGroup->AddControlItem(m_AnimProgressSlider);

		m_PlayPause = pGroup->AddLinkItem(ID_MOTIONADAPTOR_PLAY_PAUSE); 
		m_PlayPause->SetType(xtpTaskItemTypeLink);
		m_PlayPause->SetCaption(_T("PAUSE"));
	}

	// Options
	{
		pGroup = m_wndTaskPanel.AddGroup(groupID);
		++groupID;
		pGroup->SetCaption(_T("Options"));

		m_FixInverseBending.Create("Fix Inverse Bending", BS_AUTOCHECKBOX|BS_TEXT|BS_LEFT, CRect(0, 0, 20, 16), this, ID_MOTIONADAPTOR_FIX_INVERSE_BENDING);
		m_FixInverseBending.SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
		
		m_FixInverseBending.SetCheck(BST_CHECKED);
		pItem = pGroup->AddControlItem(m_FixInverseBending);
		m_FixInverseBending.SetParent(&m_wndTaskPanel);		m_FixInverseBending.SetOwner(this);

		m_MotionRetargeting.Create("Motion Retargeting", BS_AUTOCHECKBOX|BS_TEXT|BS_LEFT, CRect(0, 0, 20, 16), this, ID_MOTIONADAPTOR_MOTION_RETARGETING);
		m_MotionRetargeting.SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
		m_MotionRetargeting.SetParent(&m_wndTaskPanel);
		m_MotionRetargeting.SetOwner(this);
		m_MotionRetargeting.SetCheck(BST_CHECKED);
		pItem = pGroup->AddControlItem(m_MotionRetargeting);

		m_AdjustFoot.Create("Adjust Feet", BS_AUTOCHECKBOX|BS_TEXT|BS_LEFT, CRect(0, 0, 20, 16), this, ID_MOTIONADAPTOR_ADJUSTFOOT);
		m_AdjustFoot.SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
		m_AdjustFoot.SetParent(&m_wndTaskPanel);
		m_AdjustFoot.SetOwner(this);
		m_AdjustFoot.SetCheck(m_dlgMain.GetAdjustFoot());
		pGroup->AddControlItem(m_AdjustFoot);

		m_AdditiveMotion.Create("Additive Motion", BS_AUTOCHECKBOX|BS_TEXT|BS_LEFT, CRect(0, 0, 20, 16), this, ID_MOTIONADAPTOR_ADDITIVE_MOTION);
		m_AdditiveMotion.SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
		m_AdditiveMotion.SetParent(&m_wndTaskPanel);
		m_AdditiveMotion.SetOwner(this);
		m_AdditiveMotion.SetCheck(BST_CHECKED);
		pGroup->AddControlItem(m_AdditiveMotion);


		m_ShowViconMarkers.Create("Show Vicon Markers", BS_AUTOCHECKBOX|BS_TEXT|BS_LEFT, CRect(0, 0, 20, 16), this, ID_MOTIONADAPTOR_SHOW_VICON_MARKERS);
		m_ShowViconMarkers.SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
		//m_FixInverseBending.SetXButtonStyle(BS_XT_XPFLAT);
		m_ShowViconMarkers.SetParent(&m_wndTaskPanel);
		m_ShowViconMarkers.SetOwner(this);
		m_ShowViconMarkers.SetCheck(BST_CHECKED);
		pGroup->AddControlItem(m_ShowViconMarkers);

		m_ShowViconSkeleton.Create("Show Vicon Skeleton", BS_AUTOCHECKBOX|BS_TEXT|BS_LEFT, CRect(0, 0, 20, 16), this, ID_MOTIONADAPTOR_SHOW_VICON_SKELETON);
		m_ShowViconSkeleton.SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
		m_ShowViconSkeleton.SetParent(&m_wndTaskPanel);
		m_ShowViconSkeleton.SetOwner(this);
		m_ShowViconSkeleton.SetCheck(BST_CHECKED);
		pGroup->AddControlItem(m_ShowViconSkeleton);
	}

	// Info
	{
		pGroup = m_wndTaskPanel.AddGroup(groupID);
		++groupID;
		pGroup->SetCaption(_T("Info"));

		m_MotionName = pGroup->AddTextItem(_T("Motion Name:"));
		m_ViconFrameRate = pGroup->AddTextItem(_T("Vicon Frame Rate:"));
		m_BipedFrameRate = pGroup->AddTextItem(_T("Biped Frame Rate::"));
		m_BipedSpeed = pGroup->AddTextItem(_T("Speed (m/s):"));
	}

	m_wndTaskPanel.Reposition(FALSE);

	return res;
}

void CMotionAdaptorDlg::SetProgressEditText(const char* txt)
{
	m_AnimProgressEdit.SetWindowText(txt);

}

void CMotionAdaptorDlg::SetProgressSliderPos(const int32 pos)
{
	m_AnimProgressSlider.SetPos(pos);
}

// CMotionAdaptorDlg message handlers

BEGIN_MESSAGE_MAP(CMotionAdaptorDlg, CToolbarDialog)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_MESSAGE(XTPWM_TASKPANEL_NOTIFY, &CMotionAdaptorDlg::OnTaskPanelNotify)
	ON_NOTIFY(XTP_NM_REPORT_SELCHANGED, ID_MOTIONADAPTOR_REPORT_CONTROL, &CMotionAdaptorDlg::OnReportSelChange)
END_MESSAGE_MAP()

void CMotionAdaptorDlg::OnBnClickedFixInverseBending()
{
	m_dlgMain.SetFixInverseBending(m_FixInverseBending.GetCheck());
	m_dlgMain.RedoConversion();
	
}
void CMotionAdaptorDlg::OnBnClickedMotionRetargeting()
{
	m_dlgMain.SetMotionRetargeting(m_MotionRetargeting.GetCheck());
	m_dlgMain.RedoConversion();

}
void CMotionAdaptorDlg::OnBnClickedAdditiveMotion()
{
	m_dlgMain.SetAdditiveMotion(m_AdditiveMotion.GetCheck());
	m_dlgMain.RedoConversion();

}
void CMotionAdaptorDlg::OnBnClickedAdjustFoot()
{
	m_dlgMain.SetAdjustFoot(m_AdjustFoot.GetCheck());
	m_dlgMain.RedoConversion();

}
void CMotionAdaptorDlg::OnBnClickedShowViconMarkers()
{
	m_dlgMain.SetDispViconMarkers(m_ShowViconMarkers.GetCheck());
}
void CMotionAdaptorDlg::onBnClickedShowViconSkeleton()
{
	m_dlgMain.SetDispViconMarkers(m_ShowViconSkeleton.GetCheck());
}
void CMotionAdaptorDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollbar)
{
	if( (CSliderCtrl*) (pScrollbar) == &m_AnimProgressSlider){
		m_dlgMain.OnProgressSliderDrag( m_AnimProgressSlider.GetPos());
	}
}

void CMotionAdaptorDlg::OnSize(UINT nType, int cx, int cy)
{
	if(m_wndTaskPanel)
	{	
		CRect rc;
		GetClientRect(rc);
		m_wndTaskPanel.MoveWindow(rc);
	}
	__super::OnSize(nType, cx, cy);
}

void CMotionAdaptorDlg::UpdateVFileList()
{
	m_wndReport.BeginUpdate();
	m_wndReport.GetRecords()->RemoveAll();

	int32 nameSize = m_VFileNames.size();
	for (int32 i = 0; i < nameSize; ++i)
	{
		VFileRecord* pRec  = new VFileRecord(m_VFileNames[i]);
		m_wndReport.AddRecord(pRec);
	}	

	m_wndReport.EndUpdate();
	m_wndReport.Populate();

}

LRESULT CMotionAdaptorDlg::OnTaskPanelNotify( WPARAM wParam, LPARAM lParam )
{
	switch(wParam) 
	{
	case XTP_TPN_CLICK:
		{
			CXTPTaskPanelGroupItem* pItem = (CXTPTaskPanelGroupItem*)lParam;
			UINT nCmdID = pItem->GetID();
			switch (nCmdID)
			{
			case ID_MOTIONADAPTOR_LOAD_V_FILES:
				{
					m_dlgMain.OnLoadVFiles(m_VFileNames);
					UpdateVFileList();

				}
				break;
			case ID_MOTIONADAPTOR_CONVERT_SELECTED:
				{
					std::vector<string> vFilePaths;
					GetSelectedVFiles(vFilePaths);
					m_dlgMain.OnConvertMotions(vFilePaths);
					break;
				}
			case ID_MOTIONADAPTOR_BATCH_CONVERT:
				{
					std::vector<string> vFilePaths;
					GetSelectedVFiles(vFilePaths);
					m_dlgMain.OnBatchConvertMotions(vFilePaths);
					break;

				}
			case ID_MOTIONADAPTOR_SAVE_CONVERTED:
				{
					m_dlgMain.OnSaveMotion();
					break;
				}
			case ID_MOTIONADAPTOR_PLAY_PAUSE:
				{
					m_dlgMain.SetProgressSliderPos(m_AnimProgressSlider.GetPos());
					m_dlgMain.OnPlayPauseMotions();
					break;
				}
			default:
				break;		
			}
		}
	default:
		break;
	}
	return 0;
}

// The full path
void CMotionAdaptorDlg::GetSelectedVFiles(std::vector<string>& vFiles)
{
	std::map<string, int32> nameMap; // If you selected the same item several times, report control will bring in the same name several times.
	                                 // Use map to remove duplicated entries.

	POSITION pos = m_wndReport.GetSelectedRows()->GetFirstSelectedRowPosition();

	if(!pos)
		return;

	while (pos)
	{
		VFileRecord *pRec = DYNAMIC_DOWNCAST(VFileRecord,m_wndReport.GetSelectedRows()->GetNextSelectedRow( pos )->GetRecord());

		if (pRec && pRec->GetName().size() >0)
		{
			nameMap.insert(std::make_pair(pRec->GetName(), -1) );
		}
	}

	if(nameMap.empty())
		return;

	vFiles.resize(0);
	std::map<string, int32>::iterator it = nameMap.begin();
	for(it = nameMap.begin(); it != nameMap.end(); ++it)
	{
		vFiles.push_back(it->first);
	}

	if(!vFiles.empty())
	{
		string txt = string("Motion Name: ") + vFiles[0];
		m_MotionName->SetCaption(txt.c_str());
	}	
}

void CMotionAdaptorDlg::OnReportSelChange( NMHDR * pNotifyStruct, LRESULT *result )
{
	std::vector<string> anims;

	anims.clear();

	POSITION pos = m_wndReport.GetSelectedRows()->GetFirstSelectedRowPosition();
	while (pos)
	{
		VFileRecord *pRec = DYNAMIC_DOWNCAST(VFileRecord,m_wndReport.GetSelectedRows()->GetNextSelectedRow( pos )->GetRecord());

		if (pRec && pRec->GetName().size() >0)
		{
			anims.push_back(pRec->GetName());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CMotionAdaptorDlg::OnReportItemDblClick()
{
	MessageBox("double clicked");
}


