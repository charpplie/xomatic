#include "StdAfx.h"
#include "MFC_ExportToolPanel.h"
#include "Tools/BrushDesignerExportTool.h"

IMPLEMENT_DYNAMIC(MFC_ExportToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_ExportToolPanel, CXTResizeDialog)
	ON_BN_CLICKED(IDC_DESIGNER_EXPORT_CGF, OnBnClickedDesignerExportCgf)
	ON_BN_CLICKED(IDC_DESIGNER_EXPORT_GRP, OnBnClickedDesignerExportGrp)
	ON_BN_CLICKED(IDC_DESIGNER_EXPORT_OBJ, OnBnClickedDesignerExportObj)
END_MESSAGE_MAP()

namespace
{
	MFC_ExportToolPanel* s_pExportPanel = NULL;
	int s_nExportPanelId = 0;
}

IBaseToolPanel* CreateExportToolPanel( CBrushDesignerExportTool* pExportTool, void* pData )
{	
	if( !s_nExportPanelId )
	{
		s_pExportPanel = new MFC_ExportToolPanel(pExportTool);
		s_nExportPanelId = GetIEditor()->AddRollUpPage( ROLLUP_OBJECTS, _T("Export Tool"), s_pExportPanel, false );
	}
	return s_pExportPanel;
}

void MFC_ExportToolPanel::DestroyPanel()
{
	if( s_nExportPanelId )
	{
		GetIEditor()->RemoveRollUpPage( ROLLUP_OBJECTS, s_nExportPanelId );
		s_pExportPanel = NULL;
		s_nExportPanelId = 0;
	}
}

MFC_ExportToolPanel::MFC_ExportToolPanel(CBrushDesignerExportTool* pExportTool) : CXTResizeDialog(MFC_ExportToolPanel::IDD, NULL), m_pExportTool(pExportTool)
{	
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create(IDD,NULL);
}

void MFC_ExportToolPanel::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
}

BOOL MFC_ExportToolPanel::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();
	return TRUE;
}

void MFC_ExportToolPanel::OnBnClickedDesignerExportCgf()
{
	m_pExportTool->ExportToCgf();
}

void MFC_ExportToolPanel::OnBnClickedDesignerExportGrp()
{
	m_pExportTool->ExportToGrp();
}

void MFC_ExportToolPanel::OnBnClickedDesignerExportObj()
{
	m_pExportTool->ExportToObj();
}
