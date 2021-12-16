#include "StdAfx.h"
#include "MFC_SmoothingGroupToolPanel.h"
#include "Tools/BrushDesignerSmoothingGroupTool.h"
#include "Core/BrushDesigner.h"

IMPLEMENT_DYNAMIC(MFC_SmoothingGroupToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_SmoothingGroupToolPanel, CXTResizeDialog)
	ON_WM_DESTROY()
	ON_CONTROL_RANGE(BN_CLICKED, IDC_SMOOTHINGGROUP_NUM_0, IDC_SMOOTHINGGROUP_NUM_31, OnBnClickedSmoothingGroupNumber)
	ON_BN_CLICKED(IDC_SMOOTHINGGROUP_SELECTBYSG, OnBnClickedSmoothinggroupSelectbysg)
	ON_BN_CLICKED(IDC_SMOOTHINGGROUP_REMOVESMOOTHINGGROUPS, OnBnClickedSmoothinggroupRemoveSmoothingGroup)
	ON_BN_CLICKED(IDC_SMOOTHINGGROUP_AUTOSMOOTH, OnBnClickedSmoothinggroupAutosmooth)
END_MESSAGE_MAP()

namespace
{
	MFC_SmoothingGroupToolPanel* g_pDesignerSmoothingGroupToolPanel = NULL;
	int g_nSmoothingGroupToolPanelID = 0;
}

ISmoothingGroupToolPanel* CreateSmoothingGroupToolPanel( CBrushDesignerSmoothingGroupTool* pSmoothingGroupTool, void* pData )
{
	if( g_nSmoothingGroupToolPanelID == 0 )
	{
		g_pDesignerSmoothingGroupToolPanel = new MFC_SmoothingGroupToolPanel(pSmoothingGroupTool);
		g_nSmoothingGroupToolPanelID = GetIEditor()->AddRollUpPage( ROLLUP_OBJECTS, _T("Smoothing Group Tool"), g_pDesignerSmoothingGroupToolPanel, false, (int)pData );
	}
	return g_pDesignerSmoothingGroupToolPanel;
}

void MFC_SmoothingGroupToolPanel::DestroyPanel()
{
	if( g_nSmoothingGroupToolPanelID != 0 )
	{
		GetIEditor()->RemoveRollUpPage( ROLLUP_OBJECTS, g_nSmoothingGroupToolPanelID );
		g_pDesignerSmoothingGroupToolPanel = NULL;
		g_nSmoothingGroupToolPanelID = 0;
	}
}

MFC_SmoothingGroupToolPanel::MFC_SmoothingGroupToolPanel( CBrushDesignerSmoothingGroupTool* pSmoothingGroupTool, CWnd* pParent ) : m_pSmoothingGroupTool(pSmoothingGroupTool)
{
	DESIGNER_ASSERT(m_pSmoothingGroupTool);
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create(IDD,pParent);
}

MFC_SmoothingGroupToolPanel::~MFC_SmoothingGroupToolPanel()
{
}

void MFC_SmoothingGroupToolPanel::ClearAllSelectionsOfNumbers( int nExcludedID )
{
	std::map<int,int>::iterator ii = m_nSmoothingGroupButtonIDs.begin();
	for( ;ii != m_nSmoothingGroupButtonIDs.end(); ++ii )
	{
		if( nExcludedID == ii->first )
			continue;
		CButton* pButton = (CButton*)GetDlgItem(ii->first);
		pButton->SetCheck(BST_UNCHECKED);
	}
}

void MFC_SmoothingGroupToolPanel::ShowAllNumbers()
{
	std::map<int,int>::iterator ii = m_nSmoothingGroupButtonIDs.begin();
	for(; ii != m_nSmoothingGroupButtonIDs.end(); ++ii )
	{			
		CButton* pButton = (CButton*)GetDlgItem(ii->first);
		CString number_str;
		number_str.Format("%d",ii->second);
		pButton->SetWindowText(number_str);
	}
}

void MFC_SmoothingGroupToolPanel::HideNumber( int nNumber )
{
	std::map<int,int>::iterator ii = m_nSmoothingGroupButtonIDs.begin();
	for(; ii != m_nSmoothingGroupButtonIDs.end(); ++ii )
	{
		CButton* pButton = (CButton*)GetDlgItem(ii->first);
		if( nNumber == ii->second )
		{
			pButton->SetWindowText("");
			continue;
		}
	}
}

void MFC_SmoothingGroupToolPanel::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
}

BOOL MFC_SmoothingGroupToolPanel::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();

	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_SMOOTHINGGROUP_ANGLE);
	CString threshold_str;
	threshold_str.Format("%d",AfxGetApp()->GetProfileInt("DesignerSetting", "SmoothingGroup_Threshold", 45));
	pEdit->SetWindowText(threshold_str);

	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_0] = 1;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_1] = 2;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_2] = 3;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_3] = 4;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_4] = 5;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_5] = 6;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_6] = 7;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_7] = 8;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_8] = 9;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_9] = 10;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_10] = 11;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_11] = 12;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_12] = 13;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_13] = 14;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_14] = 15;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_15] = 16;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_16] = 17;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_17] = 18;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_18] = 19;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_19] = 20;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_20] = 21;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_21] = 22;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_22] = 23;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_23] = 24;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_24] = 25;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_25] = 26;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_26] = 27;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_27] = 28;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_28] = 29;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_29] = 30;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_30] = 31;
	m_nSmoothingGroupButtonIDs[IDC_SMOOTHINGGROUP_NUM_31] = 32;

	return TRUE;
}

void MFC_SmoothingGroupToolPanel::OnDestroy()
{		
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_SMOOTHINGGROUP_ANGLE);
	CString threshold_str;
	pEdit->GetWindowText(threshold_str);
	AfxGetApp()->WriteProfileInt("DesignerSetting","SmoothingGroup_Threshold",atoi(threshold_str));
}

void MFC_SmoothingGroupToolPanel::OnBnClickedSmoothingGroupNumber(UINT nCtrlID)
{
	CButton* pButton = (CButton*)GetDlgItem(nCtrlID);
	if( pButton == NULL )
		return;

	CUndo undo("Designer : Smoothing Group");
	m_pSmoothingGroupTool->GetDesigner()->RecordUndo("Designer : Smoothing Group",m_pSmoothingGroupTool->GetBaseObject());

	bool bControlPressed = CheckVirtualKey(VK_CONTROL);
	if( !bControlPressed )
		ClearAllSelectionsOfNumbers(nCtrlID);

	CString number_str;
	number_str.Format("%d",m_nSmoothingGroupButtonIDs[nCtrlID]);
	pButton->SetWindowText(number_str);

	if( !bControlPressed )
	{
		int nSmoothingGroupID = m_nSmoothingGroupButtonIDs[nCtrlID];
		m_pSmoothingGroupTool->SetSmoothingGroup(nSmoothingGroupID);
	}
}

void MFC_SmoothingGroupToolPanel::OnBnClickedSmoothinggroupSelectbysg()
{
	m_pSmoothingGroupTool->ClearSelectedElements();
	std::map<int,int>::iterator iter = m_nSmoothingGroupButtonIDs.begin();
	for( ; iter != m_nSmoothingGroupButtonIDs.end(); ++iter )
	{
		CButton* pButton = (CButton*)GetDlgItem(iter->first);
		if( pButton->GetCheck() == BST_CHECKED )
			m_pSmoothingGroupTool->SelectRegionsInSmoothingGroup(iter->second);
	}
}

void MFC_SmoothingGroupToolPanel::OnBnClickedSmoothinggroupRemoveSmoothingGroup()
{
	CUndo undo("Designer : Smoothing Group");
	m_pSmoothingGroupTool->GetDesigner()->RecordUndo("Designer : Smoothing Group",m_pSmoothingGroupTool->GetBaseObject());
	m_pSmoothingGroupTool->RemoveRegionsFromSmoothingGroups();
}

void MFC_SmoothingGroupToolPanel::OnBnClickedSmoothinggroupAutosmooth()
{
	CUndo undo("Designer : Smoothing Group");
	m_pSmoothingGroupTool->GetDesigner()->RecordUndo("Designer : Smoothing Group",m_pSmoothingGroupTool->GetBaseObject());
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_SMOOTHINGGROUP_ANGLE);
	CString threshold_str;
	pEdit->GetWindowText(threshold_str);
	m_pSmoothingGroupTool->ApplyAutoSmooth(atoi(threshold_str));
}