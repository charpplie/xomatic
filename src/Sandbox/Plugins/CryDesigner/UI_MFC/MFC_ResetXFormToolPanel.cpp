#include "StdAfx.h"
#include "MFC_ResetXFormToolPanel.h"
#include "Tools/BrushDesignerResetXFormTool.h"
#include "Tools/BrushDesignerEditTool.h"

IMPLEMENT_DYNAMIC(MFC_ResetXFormToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_ResetXFormToolPanel, CXTResizeDialog)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_DESIGNER_RESETXFORM_FREEZE, OnBnClickedDesignerResetxformFreeze)
END_MESSAGE_MAP()

namespace 
{
	MFC_ResetXFormToolPanel* s_pResetXFormPanel = NULL;
	int s_nResetXFormPanelId = 0;
}

IBaseToolPanel* CreateResetXFormToolPanel( CBrushDesignerResetXFormTool* pResetXFormTool, void* pData )
{
	if( !s_nResetXFormPanelId )
	{
		s_pResetXFormPanel = new MFC_ResetXFormToolPanel(pResetXFormTool);
		s_nResetXFormPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"ResetXForm Tool",s_pResetXFormPanel,false);
	}
	return s_pResetXFormPanel;
}

void MFC_ResetXFormToolPanel::DestroyPanel()
{
	if( s_nResetXFormPanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,s_nResetXFormPanelId);
		s_pResetXFormPanel = NULL;
		s_nResetXFormPanelId = 0;
	}
}

MFC_ResetXFormToolPanel::MFC_ResetXFormToolPanel(CBrushDesignerResetXFormTool* pResetXFormTool) : CXTResizeDialog(MFC_ResetXFormToolPanel::IDD, NULL),
	m_pDesignerResetXFormTool(pResetXFormTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create(IDD,NULL);
}

void MFC_ResetXFormToolPanel::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
}

BOOL MFC_ResetXFormToolPanel::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();

	CButton* pRotationButton = (CButton*)GetDlgItem(IDC_DESIGNER_RESETXFORM_ROTATION);
	CButton* pScaleButton = (CButton*)GetDlgItem(IDC_DESIGNER_RESETXFORM_SCALE);
	CButton* pPositionButton = (CButton*)GetDlgItem(IDC_DESIGNER_RESETXFORM_POSITION);

	if( AfxGetApp()->GetProfileInt( "DesignerSetting", "ResetXForm_Rotation", 1 ) )
		pRotationButton->SetCheck( BST_CHECKED );
	else
		pRotationButton->SetCheck( BST_UNCHECKED );

	if( AfxGetApp()->GetProfileInt( "DesignerSetting", "ResetXForm_Scale", 1 ) )
		pScaleButton->SetCheck( BST_CHECKED );
	else
		pScaleButton->SetCheck( BST_UNCHECKED );

	if( AfxGetApp()->GetProfileInt( "DesignerSetting", "ResetXForm_Position", 1 ) )
		pPositionButton->SetCheck( BST_CHECKED );
	else
		pPositionButton->SetCheck( BST_UNCHECKED );

	return TRUE;
}

void MFC_ResetXFormToolPanel::OnDestroy()
{
	CButton* pRotationButton = (CButton*)GetDlgItem(IDC_DESIGNER_RESETXFORM_ROTATION);
	CButton* pScaleButton = (CButton*)GetDlgItem(IDC_DESIGNER_RESETXFORM_SCALE);
	CButton* pPositionButton = (CButton*)GetDlgItem(IDC_DESIGNER_RESETXFORM_POSITION);

	AfxGetApp()->WriteProfileInt( "DesignerSetting", "ResetXForm_Rotation", pRotationButton->GetCheck() == BST_CHECKED ? 1 : 0 );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "ResetXForm_Scale", pScaleButton->GetCheck() == BST_CHECKED ? 1 : 0 );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "ResetXForm_Position", pPositionButton->GetCheck() == BST_CHECKED ? 1 : 0 );
}

void MFC_ResetXFormToolPanel::OnBnClickedDesignerResetxformFreeze()
{
	CButton* pRotationButton = (CButton*)GetDlgItem(IDC_DESIGNER_RESETXFORM_ROTATION);
	CButton* pScaleButton = (CButton*)GetDlgItem(IDC_DESIGNER_RESETXFORM_SCALE);
	CButton* pPositionButton = (CButton*)GetDlgItem(IDC_DESIGNER_RESETXFORM_POSITION);

	int nResetFlag = 0;

	if( pRotationButton->GetCheck() == BST_CHECKED )
		nResetFlag |= BUtil::eResetXForm_Rotation;

	if( pScaleButton->GetCheck() == BST_CHECKED )
		nResetFlag |= BUtil::eResetXForm_Scale;

	if( pPositionButton->GetCheck() == BST_CHECKED )
		nResetFlag |= BUtil::eResetXForm_Position;

	m_pDesignerResetXFormTool->FreezeXForm(nResetFlag);
	m_pDesignerResetXFormTool->GetEditTool()->GoToSelectDesignerMode();
}