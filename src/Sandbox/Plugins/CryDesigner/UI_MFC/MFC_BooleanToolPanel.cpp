#include "StdAfx.h"
#include "MFC_BooleanToolPanel.h"
#include "Tools/BrushDesignerBooleanTool.h"

IMPLEMENT_DYNAMIC(MFC_BooleanToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_BooleanToolPanel, CXTResizeDialog)
	ON_BN_CLICKED(IDC_DESIGNER_BOOLEAN_UNION, OnBnClickedDesignerBooleanUnion)
	ON_BN_CLICKED(IDC_DESIGNER_BOOLEAN_DIFFERENCE, OnBnClickedDesignerBooleanDifference)
	ON_BN_CLICKED(IDC_DESIGNER_BOOLEAN_INTERSECTION, OnBnClickedDesignerBooleanIntersection)
END_MESSAGE_MAP()

namespace 
{
	MFC_BooleanToolPanel* s_pBooleanPanel = NULL;
	int s_nBooleanPanelId = 0;
}

IBaseToolPanel* CreateBoolaenToolPanel( CBrushDesignerBooleanTool* pBooleanTool, void* pData )
{
	if( !s_nBooleanPanelId )
	{
		s_pBooleanPanel = new MFC_BooleanToolPanel(pBooleanTool);
		s_nBooleanPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Boolean Tool",s_pBooleanPanel,false,(int)pData);
	}
	return s_pBooleanPanel;
}

void MFC_BooleanToolPanel::DestroyPanel()
{
	if( s_nBooleanPanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,s_nBooleanPanelId);
		s_pBooleanPanel = NULL;
		s_nBooleanPanelId = 0;
	}
}

MFC_BooleanToolPanel::MFC_BooleanToolPanel(CBrushDesignerBooleanTool* pBooleanTool) : CXTResizeDialog(MFC_BooleanToolPanel::IDD, NULL),
	m_pDesignerBooleanTool(pBooleanTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create(IDD,NULL);
}

void MFC_BooleanToolPanel::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
}

BOOL MFC_BooleanToolPanel::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();
	return TRUE;
}

void MFC_BooleanToolPanel::OnBnClickedDesignerBooleanUnion()
{
	m_pDesignerBooleanTool->BooleanOperation(BUtil::eBOE_Union);
}

void MFC_BooleanToolPanel::OnBnClickedDesignerBooleanDifference()
{
	m_pDesignerBooleanTool->BooleanOperation(BUtil::eBOE_Difference);
}

void MFC_BooleanToolPanel::OnBnClickedDesignerBooleanIntersection()
{
	m_pDesignerBooleanTool->BooleanOperation(BUtil::eBOE_Intersection);
}