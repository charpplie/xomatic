#include "StdAfx.h"
#include "MFC_PivotToolPanel.h"
#include "Tools/BrushDesignerPivotTool.h"

IMPLEMENT_DYNAMIC(MFC_PivotToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_PivotToolPanel, CXTResizeDialog)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_DESIGNER_PIVOT_BOUNDBOX, OnBnClickedDesignerPivotBoundbox)
	ON_BN_CLICKED(IDC_DESIGNER_PIVOT_MESH, OnBnClickedDesignerPivotMesh)
END_MESSAGE_MAP()

namespace 
{
	MFC_PivotToolPanel* s_pPivotToolPanel = NULL;
	int s_nPivotPanelId = 0;
}

IBaseToolPanel* CreatePivotToolPanel( CBrushDesignerPivotTool* pPivotTool, void* pData )
{
	if( !s_nPivotPanelId )
	{
		s_pPivotToolPanel = new MFC_PivotToolPanel(pPivotTool);
		s_nPivotPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Pivot Tool",s_pPivotToolPanel,false,(int)pData);
	}
	return s_pPivotToolPanel;
}

void MFC_PivotToolPanel::DestroyPanel()
{
	if( s_nPivotPanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,s_nPivotPanelId);
		s_pPivotToolPanel = NULL;
		s_nPivotPanelId = 0;
	}
}

MFC_PivotToolPanel::MFC_PivotToolPanel( CBrushDesignerPivotTool* pTool, CWnd* pParent ) : m_pTool(pTool)
{	
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create(IDD,pParent);
}

BOOL MFC_PivotToolPanel::OnInitDialog()
{
	CButton* pPivotBoundboxButton = (CButton*)GetDlgItem(IDC_DESIGNER_PIVOT_BOUNDBOX);
	if( pPivotBoundboxButton )
		pPivotBoundboxButton->SetCheck(BST_UNCHECKED);

	CButton* pPivotMeshButton = (CButton*)GetDlgItem(IDC_DESIGNER_PIVOT_MESH);
	if( pPivotMeshButton )
		pPivotMeshButton->SetCheck(BST_UNCHECKED);

	if( AfxGetApp()->GetProfileInt( "DesignerSetting", "PIVOTSELECTIONTYPE", 0 ) == 0 )
	{
		if( pPivotBoundboxButton )
			pPivotBoundboxButton->SetCheck(BST_CHECKED);
		m_pTool->SetSelectionType(CBrushDesignerPivotTool::ePST_BoundBox,true);
	}
	else
	{
		if( pPivotMeshButton )
			pPivotMeshButton->SetCheck(BST_CHECKED);
		m_pTool->SetSelectionType(CBrushDesignerPivotTool::ePST_Designer,true);
	}

	return TRUE;
}

void MFC_PivotToolPanel::OnDestroy()
{
	CButton* pPivotBoundboxButton = (CButton*)GetDlgItem(IDC_DESIGNER_PIVOT_BOUNDBOX);
	if( pPivotBoundboxButton && pPivotBoundboxButton->GetCheck() == BST_CHECKED )
		AfxGetApp()->WriteProfileInt( "DesignerSetting", "PIVOTSELECTIONTYPE", 0 );
	else
		AfxGetApp()->WriteProfileInt( "DesignerSetting", "PIVOTSELECTIONTYPE", 1 );
}

void MFC_PivotToolPanel::OnBnClickedDesignerPivotBoundbox()
{
	CButton* pPivotMeshButton = (CButton*)GetDlgItem(IDC_DESIGNER_PIVOT_MESH);
	if( pPivotMeshButton )
		pPivotMeshButton->SetCheck(BST_UNCHECKED);
	m_pTool->SetSelectionType(CBrushDesignerPivotTool::ePST_BoundBox);
}

void MFC_PivotToolPanel::OnBnClickedDesignerPivotMesh()
{
	CButton* pPivotBoundboxButton = (CButton*)GetDlgItem(IDC_DESIGNER_PIVOT_BOUNDBOX);
	if( pPivotBoundboxButton )
		pPivotBoundboxButton->SetCheck(BST_UNCHECKED);
	m_pTool->SetSelectionType(CBrushDesignerPivotTool::ePST_Designer);
}