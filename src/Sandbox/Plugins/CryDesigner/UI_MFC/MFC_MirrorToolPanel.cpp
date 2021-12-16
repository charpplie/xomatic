#include "StdAfx.h"
#include "MFC_MirrorToolPanel.h"
#include "Core/BrushDesigner.h"
#include "Tools/BrushDesignerMirrorTool.h"

IMPLEMENT_DYNAMIC(MFC_MirrorToolPanel, CXTResizeDialog)

BEGIN_MESSAGE_MAP(MFC_MirrorToolPanel, CXTResizeDialog)
	ON_BN_CLICKED(IDC_ALIGN_X, OnBnClickedAlignX)
	ON_BN_CLICKED(IDC_ALIGN_Y, OnBnClickedAlignY)
	ON_BN_CLICKED(IDC_ALIGN_Z, OnBnClickedAlignZ)
	ON_BN_CLICKED(IDC_MIRROR_APPLYBTN, OnBnClickedMirrorApply)
	ON_BN_CLICKED(IDC_FREEZE_DESIGNER, OnBnClickedFreezeDesigner)
	ON_BN_CLICKED(IDC_INVERTPLANE, OnBnClickedMirrorInvert)
	ON_BN_CLICKED(IDC_MIRROR_CENTERPIVOT, OnBnClickedCenterPivot)
END_MESSAGE_MAP()

namespace
{
	MFC_MirrorToolPanel* g_pDesignerMirrorToolPanel = NULL;
	int g_nMirrorToolPanelID = 0;
}

IMirrorToolPanel* CreateMirrorToolPanel( CBrushDesignerMirrorTool* pMirroTool, void* pData )
{
	if( g_nMirrorToolPanelID == 0 )
	{
		g_pDesignerMirrorToolPanel = new MFC_MirrorToolPanel(pMirroTool);
		g_nMirrorToolPanelID = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,_T("Mirror Tool"),g_pDesignerMirrorToolPanel,false,(int)pData);
	}
	return g_pDesignerMirrorToolPanel;
}

void MFC_MirrorToolPanel::DestroyPanel()
{
	if( g_nMirrorToolPanelID != 0 )
	{
		GetIEditor()->RemoveRollUpPage( ROLLUP_OBJECTS, g_nMirrorToolPanelID );
		g_pDesignerMirrorToolPanel = NULL;
		g_nMirrorToolPanelID = 0;
	}
}

MFC_MirrorToolPanel::MFC_MirrorToolPanel( CBrushDesignerMirrorTool* pMirrorTool, CWnd* pParent ) : m_pMirrorTool(pMirrorTool)
{	
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create(IDD,pParent);
}

void MFC_MirrorToolPanel::ToggleWndEnableDisable()
{
	UINT nIDs[] = { IDC_ALIGN_X, IDC_ALIGN_Y, IDC_ALIGN_Z, IDC_MIRROR_APPLYBTN, IDC_INVERTPLANE, IDC_MIRROR_CENTERPIVOT };
	BOOL bMirrorType = m_pMirrorTool->GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror);

	for( int i = 0; i < sizeof(nIDs)/sizeof(*nIDs); ++i )
	{
		CWnd* pWnd = GetDlgItem(nIDs[i]);
		DESIGNER_ASSERT(pWnd);
		if( pWnd )
			pWnd->EnableWindow(!bMirrorType);
	}

	CWnd* pWnd = GetDlgItem(IDC_FREEZE_DESIGNER);
	DESIGNER_ASSERT(pWnd);
	if( pWnd )
		pWnd->EnableWindow(bMirrorType);
}

void MFC_MirrorToolPanel::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
}

BOOL MFC_MirrorToolPanel::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();
	ToggleWndEnableDisable();
	return TRUE;
}

void MFC_MirrorToolPanel::OnBnClickedAlignX()
{
	m_pMirrorTool->AlignSlicePlane(BrushVec3(1,0,0));
}

void MFC_MirrorToolPanel::OnBnClickedAlignY()
{
	m_pMirrorTool->AlignSlicePlane(BrushVec3(0,1,0));
}

void MFC_MirrorToolPanel::OnBnClickedAlignZ()
{
	m_pMirrorTool->AlignSlicePlane(BrushVec3(0,0,1));
}

void MFC_MirrorToolPanel::OnBnClickedMirrorApply()
{
	m_pMirrorTool->ApplyMirror();
}

void MFC_MirrorToolPanel::OnBnClickedFreezeDesigner()
{
	m_pMirrorTool->FreezeDesigner();
}

void MFC_MirrorToolPanel::OnBnClickedMirrorInvert()
{
	m_pMirrorTool->InvertSlicePlane();
}

void MFC_MirrorToolPanel::OnBnClickedCenterPivot()
{
	m_pMirrorTool->CenterPivot();
}