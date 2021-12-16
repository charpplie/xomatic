#include "StdAfx.h"
#include "SegmentSelectionPanel.h"
#include "GridMapTool.h"

#define MAX_EDIT_SIZE 4096

BEGIN_MESSAGE_MAP(CSegmentSelectionPanel, CXTResizeDialog)
	ON_WM_HSCROLL()
	ON_COMMAND(IDC_SW_SEL_SIZE_SLIDER, OnSetSelectionSize)
	ON_COMMAND(IDC_SW_SELECTION, OnChangeSelectionMode)
	ON_COMMAND(IDC_SW_SEL_CLEAR, OnClearSelection)
END_MESSAGE_MAP()

CSegmentSelectionPanel::CSegmentSelectionPanel(CWnd *pParent, CSegmentSelectTool *pTool)
: CXTResizeDialog(CSegmentSelectionPanel::IDD, pParent)
, m_pTool(pTool)
, m_bSelectionMode(FALSE)
{
	Create(IDD, pParent);
}

void CSegmentSelectionPanel::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SW_SELECTION, m_cSelectionMode);
	DDX_Control(pDX, IDC_SW_SEL_CLEAR, m_cSelectionClear);
	DDX_Control(pDX, IDC_SW_SEL_SIZE_SLIDER, m_cSelectionSizeSlider);
}

BOOL CSegmentSelectionPanel::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();

	int nCurSizeMt = m_pTool->GetPerSegmentSize();
	int nMax = MAX_EDIT_SIZE / nCurSizeMt - 1;

	m_cSelectionSizeSlider.SetRange(0, nMax);
	m_cSelectionSizeSlider.SetTicFreq(1);
	m_cSelectionSizeSlider.SetPos(m_pTool->GetEditSize() - 1);
	
	m_cSelectionSize.Create(this, IDC_SW_SEL_SIZE);
	m_cSelectionSize.SetInteger(true);
	m_cSelectionSize.EnableWindow(FALSE);

	OnUpdateNumbers();

	return TRUE;
}

void CSegmentSelectionPanel::OnUpdateNumbers()
{
	m_cSelectionSize.SetValue(m_pTool->GetEditSize());
}

void CSegmentSelectionPanel::OnSetSelectionSize()
{
	m_cSelectionSizeSlider.SetPos(m_pTool->GetEditSize());
	m_cSelectionSizeSlider.Invalidate();
	OnUpdateNumbers();
}

void CSegmentSelectionPanel::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl *pSliderCtrl = (CSliderCtrl*)pScrollBar;
	if(pSliderCtrl == &m_cSelectionSizeSlider)
	{
		m_pTool->SetEditSize(m_cSelectionSizeSlider.GetPos() + 1);
	}
	OnUpdateNumbers();
}

void CSegmentSelectionPanel::OnChangeSelectionMode()
{
	m_bSelectionMode = !m_bSelectionMode;
	m_cSelectionMode.SetChecked(m_bSelectionMode);

#ifdef USE_GDIWND
	m_pMapWnd->SetEditMode(m_bSelectionMode ? EGEM_CHECK : EGEM_EDIT);
	m_pMapWnd->Invalidate();
#endif
}

void CSegmentSelectionPanel::OnClearSelection()
{
#ifdef USE_GDIWND
	m_pMapWnd->m_SegmentsCheck.clear();
	m_pMapWnd->Invalidate();
#endif
}