#include "StdAfx.h"
#include "MFC_SliceToolPanel.h"
#include "Tools/BrushDesignerSliceTool.h"

IMPLEMENT_DYNAMIC(MFC_SliceToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_SliceToolPanel, CXTResizeDialog)
	ON_BN_CLICKED(IDC_SLICE_FRONT, OnBnClickedSliceFront)
	ON_BN_CLICKED(IDC_SLICE_BACK, OnBnClickedSliceBack)
	ON_BN_CLICKED(IDC_SLICE_CLIP, OnBnClickedSliceClip)
	ON_EN_CHANGE(IDC_NUMBER_SLICEPLANES, OnEnChangeNumberSliceplanes)
	ON_BN_CLICKED(IDC_SLICE_DIVIDE, OnBnClickedSliceDivide)
	ON_BN_CLICKED(IDC_ALIGN_X, OnBnClickedAlignX)
	ON_BN_CLICKED(IDC_ALIGN_Y, OnBnClickedAlignY)
	ON_BN_CLICKED(IDC_ALIGN_Z, OnBnClickedAlignZ)
	ON_BN_CLICKED(IDC_INVERTPLANE, OnBnClickedMirrorInvert)
END_MESSAGE_MAP()

namespace
{
	MFC_SliceToolPanel* g_pDesignerSliceToolPanel = NULL;
	int g_nSliceToolPanelID = 0;
}

IBaseToolPanel* CreateSliceToolPanel( CBrushDesignerSliceTool* pSliceTool, void* pData )
{	
	if( g_nSliceToolPanelID == 0 )
	{
		g_pDesignerSliceToolPanel = new MFC_SliceToolPanel(pSliceTool);
		g_nSliceToolPanelID = GetIEditor()->AddRollUpPage( ROLLUP_OBJECTS, _T("Slice Tool"), g_pDesignerSliceToolPanel, false, (int)pData );
	}
	return g_pDesignerSliceToolPanel;
}

void MFC_SliceToolPanel::DestroyPanel()
{
	if( g_nSliceToolPanelID != 0 )
	{
		GetIEditor()->RemoveRollUpPage( ROLLUP_OBJECTS, g_nSliceToolPanelID );
		g_pDesignerSliceToolPanel = NULL;
		g_nSliceToolPanelID = 0;
	}
}

MFC_SliceToolPanel::MFC_SliceToolPanel( CBrushDesignerSliceTool* pSliceTool, CWnd* pParent )
	: m_nCutRadioButtons(0), m_pSliceTool(pSliceTool)
{	
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create(IDD,pParent);
}

MFC_SliceToolPanel::~MFC_SliceToolPanel()
{
}

void MFC_SliceToolPanel::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
}

BOOL MFC_SliceToolPanel::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();

	CString buffer;
	CEdit* pSliceNumberEdit = (CEdit*)GetDlgItem(IDC_NUMBER_SLICEPLANES);
	if( pSliceNumberEdit )
	{
		buffer.Format("%d",m_pSliceTool->GetNumberSlicePlane());
		pSliceNumberEdit->SetWindowText(buffer);
	}

	return TRUE;
}

void MFC_SliceToolPanel::OnBnClickedSliceFront()
{	
	m_pSliceTool->SliceFrontPart();
}

void MFC_SliceToolPanel::OnBnClickedSliceBack()
{
	m_pSliceTool->SliceBackPart();
}

void MFC_SliceToolPanel::OnBnClickedSliceClip()
{
	m_pSliceTool->Clip();
}

void MFC_SliceToolPanel::OnBnClickedSliceDivide()
{
	m_pSliceTool->Divide();
}

void MFC_SliceToolPanel::OnBnClickedMirrorInvert()
{
	m_pSliceTool->InvertSlicePlane();
}

void MFC_SliceToolPanel::OnBnClickedAlignX()
{
	m_pSliceTool->AlignSlicePlane(BrushVec3(1,0,0));
}

void MFC_SliceToolPanel::OnBnClickedAlignY()
{
	m_pSliceTool->AlignSlicePlane(BrushVec3(0,1,0));
}

void MFC_SliceToolPanel::OnBnClickedAlignZ()
{
	m_pSliceTool->AlignSlicePlane(BrushVec3(0,0,1));
}

void MFC_SliceToolPanel::OnEnChangeNumberSliceplanes()
{
	CEdit* pNumberSlicePlane = (CEdit*)GetDlgItem(IDC_NUMBER_SLICEPLANES);
	if( pNumberSlicePlane == NULL )
		return;
	CString numberSlicePlaneText;
	pNumberSlicePlane->GetWindowText(numberSlicePlaneText);
	m_pSliceTool->SetNumberSlicePlane(atoi(numberSlicePlaneText));
}