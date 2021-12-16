#include "StdAfx.h"
#include "MFC_SubdivisionToolPanel.h"
#include "Tools/BrushDesignerSubdivisionTool.h"
#include "Core/BrushDesigner.h"
#include "Core/BrushDesignerEdgesSharpnessManager.h"

namespace 
{
	MFC_SubdivisionToolPanel* s_pSubdivisionToolPanel = NULL;
	int s_nSubdivisionToolPanelId = 0;
}

IMPLEMENT_DYNAMIC(MFC_SubdivisionToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_SubdivisionToolPanel, CXTResizeDialog)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_ADDSHARPNESS_BUTTON, OnBnClickedAddSharpnessButton)
	ON_BN_CLICKED(IDC_DELETESHARPNESS_BUTTON, OnBnClickedDeleteSharpnessButton)
	ON_NOTIFY(NM_DBLCLK, IDC_EDGESHARPNESS_LISTCTRL, OnNMDblclkEdgesharpnessListctrl)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_EDGESHARPNESS_LISTCTRL, OnLvnEndlabeleditEdgesharpnessListctrl)
	ON_EN_CHANGE(IDC_SUBDIVISION_TESSFACTOR, OnEnChangeSubdivisionTessfactor)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_EDGESHARPNESS_LISTCTRL, OnLvnItemchangedEdgesharpnessListctrl)
END_MESSAGE_MAP()

IBaseToolPanel* CreateSubdivisionToolPanel( CBrushDesignerSubdivisionTool* pSubdivisionTool, void* pData )
{
	if( !s_pSubdivisionToolPanel )
	{
		s_pSubdivisionToolPanel = new MFC_SubdivisionToolPanel(pSubdivisionTool);
		s_nSubdivisionToolPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Subdivision Attributes",s_pSubdivisionToolPanel,false,(int)pData);
	}
	return s_pSubdivisionToolPanel;
}

void MFC_SubdivisionToolPanel::DestroyPanel()
{
	if( s_nSubdivisionToolPanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,s_nSubdivisionToolPanelId);
		s_pSubdivisionToolPanel = NULL;
		s_nSubdivisionToolPanelId = 0;
	}
}

MFC_SubdivisionToolPanel::MFC_SubdivisionToolPanel( CBrushDesignerSubdivisionTool* pTool, CWnd* pParent ) : m_pEditTool(pTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create(IDD,pParent);	
}

void MFC_SubdivisionToolPanel::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SUBDIVISION_SLIDER, m_SubdivisionLevelSliderCtrl);
	DDX_Control(pDX, IDC_EDGESHARPNESS_LISTCTRL, m_EdgeSharpnessListCtrl);
}

BOOL MFC_SubdivisionToolPanel::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();

	m_SubdivisionLevelSliderCtrl.SetRange(0,BUtil::kMaximumSubdivisionLevel-1);
	m_SubdivisionLevelSliderCtrl.SetPos(m_pEditTool->GetDesigner()->GetSubdivisionLevel());
	m_SubdivisionLevelSliderCtrl.SetTicFreq(1);

	CRect rc;
	GetClientRect(rc);
	int w = rc.Width()/2+20;
	m_EdgeSharpnessListCtrl.SetExtendedStyle(LVS_EX_FLATSB|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	m_EdgeSharpnessListCtrl.InsertColumn(0,"Edges",LVCFMT_LEFT,w,0);
	m_EdgeSharpnessListCtrl.InsertColumn(1,"Sharpness",LVCFMT_LEFT,w,1);

	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_SUBDIVISION_TESSFACTOR);
	if( pEdit )
	{
		CString tessFactorStr;
		tessFactorStr.Format("%d",m_pEditTool->GetDesigner()->GetTessFactor());		
		pEdit->SetWindowText(tessFactorStr);
	}

	m_nEditedSubItem = -1;

	UpdateEdgeGroupList();

	return TRUE;
}

void MFC_SubdivisionToolPanel::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if( m_pEditTool == NULL )
		return;
	if( nSBCode == SB_ENDSCROLL )		
		CBrushDesignerSubdivisionTool::Subdivide(m_SubdivisionLevelSliderCtrl.GetPos(),true);
}

void MFC_SubdivisionToolPanel::OnBnClickedAddSharpnessButton()
{
	m_pEditTool->AddNewEdgeTag();
	UpdateEdgeGroupList();	
}

void MFC_SubdivisionToolPanel::OnBnClickedDeleteSharpnessButton()
{
	POSITION itemPos = m_EdgeSharpnessListCtrl.GetFirstSelectedItemPosition();
	if(itemPos)
	{
		int iItem = m_EdgeSharpnessListCtrl.GetNextSelectedItem(itemPos);
		CString itemName = m_EdgeSharpnessListCtrl.GetItemText(iItem,0);
		m_EdgeSharpnessListCtrl.DeleteItem(iItem);
		m_pEditTool->DeleteEdgeTag(itemName);
	}
}

void MFC_SubdivisionToolPanel::UpdateEdgeGroupList()
{
	m_EdgeSharpnessListCtrl.DeleteAllItems();

	CBrushDesignerEdgeSharpnessManager* pEdgeSharpnessMgr = m_pEditTool->GetDesigner()->GetEdgeSharpnessMgr();
	for( int i = 0, iCount(pEdgeSharpnessMgr->GetCount()); i < iCount; ++i )
	{
		const BUtil::SEdgeSharpness& sharpness = pEdgeSharpnessMgr->Get(i);
		m_EdgeSharpnessListCtrl.InsertItem(i,sharpness.name.c_str());

		CString buffer;
		buffer.Format("%f",sharpness.sharpness);
		m_EdgeSharpnessListCtrl.SetItem(i,1,LVIF_TEXT,buffer,0,0,0,NULL);
	}
}

void MFC_SubdivisionToolPanel::OnNMDblclkEdgesharpnessListctrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);	

	int nItem = pNMItemActivate->iItem;
	int nSubItem = pNMItemActivate->iSubItem;

	if( nItem == -1 )
	{
		*pResult = 0;
		return;
	}

	m_nEditedSubItem = nSubItem;

	if( nSubItem == 0 )	
	{
		CEdit* pEdit = m_EdgeSharpnessListCtrl.EditLabel(nItem);
		pEdit->SetFocus();
		pEdit->SetSel(0,-1);
	}
	else if( nSubItem == 1 )
	{
		RESOURCEHANDLER_RECONSTRUCTOR;
		CString itemText = m_EdgeSharpnessListCtrl.GetItemText(nItem,nSubItem);
		CRect rect;
		m_EdgeSharpnessListCtrl.GetSubItemRect(nItem,nSubItem,LVIR_BOUNDS,rect);
		CEdit* pEdit = m_EdgeSharpnessListCtrl.EditLabel(nItem);
		pEdit->SetFocus();		
		pEdit->SetWindowPos(&CWnd::wndTop, rect.left+3, rect.top+4, rect.right-rect.left-3,  rect.bottom-rect.top-1, SWP_SHOWWINDOW|SWP_DRAWFRAME|SWP_FRAMECHANGED);
		pEdit->SetWindowText(itemText);
		pEdit->SetSel(0,-1);
	}

	*pResult = 0;
}


void MFC_SubdivisionToolPanel::OnLvnEndlabeleditEdgesharpnessListctrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	if( pDispInfo->item.pszText == NULL || m_nEditedSubItem == -1 )
	{
		*pResult = 0;
		return;
	}

	CBrushDesignerEdgeSharpnessManager* pEdgeMgr = m_pEditTool->GetDesigner()->GetEdgeSharpnessMgr();

	int nItem = pDispInfo->item.iItem;
	int nSubItem = m_nEditedSubItem;

	if( nSubItem == 0 )
	{
		CString oldName = m_EdgeSharpnessListCtrl.GetItemText(nItem,nSubItem);
		string name = pEdgeMgr->GenerateValidName(pDispInfo->item.pszText);
		pEdgeMgr->Rename(oldName,name);
		m_EdgeSharpnessListCtrl.SetItemText(nItem,nSubItem,name);
	}
	else if( nSubItem == 1 )
	{
		float sharpness = atof(pDispInfo->item.pszText);
		CString name = m_EdgeSharpnessListCtrl.GetItemText(nItem,0);
		pEdgeMgr->SetSharpness(name,sharpness);
		m_EdgeSharpnessListCtrl.SetItemText(nItem,nSubItem,pDispInfo->item.pszText);
		m_pEditTool->UpdateBrush();
	}

	*pResult = 0;
}

void MFC_SubdivisionToolPanel::OnEnChangeSubdivisionTessfactor()
{
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_SUBDIVISION_TESSFACTOR);
	if( pEdit == NULL )
		return;
	CString tessFactorStr;
	pEdit->GetWindowText(tessFactorStr);
	int nTessFactor = atoi(tessFactorStr);
	if( nTessFactor < 0 )
		nTessFactor = 0;
	if( nTessFactor > 9 )
		nTessFactor = 9;
	int nExistingTessFactor = m_pEditTool->GetDesigner()->GetTessFactor();	
	if( nExistingTessFactor != nTessFactor )
	{
		m_pEditTool->GetDesigner()->SetTessFactor((unsigned char)nTessFactor);
		m_pEditTool->UpdateBrush();
	}
}

void MFC_SubdivisionToolPanel::OnLvnItemchangedEdgesharpnessListctrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	if( pNMLV->iItem == -1 || pNMLV->uNewState == 0 )
	{
		m_pEditTool->HighlightEdgeGroup(NULL);
		return;
	}

	CString itemName = m_EdgeSharpnessListCtrl.GetItemText(pNMLV->iItem,0);
	m_pEditTool->HighlightEdgeGroup(itemName);
}
