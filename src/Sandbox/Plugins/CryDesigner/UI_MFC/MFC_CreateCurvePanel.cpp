#include "StdAfx.h"
#include "MFC_CreateCurvePanel.h"
#include "Tools/BrushDesignerDrawCurveTool.h"

IMPLEMENT_DYNAMIC(MFC_CreateCurvePanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_CreateCurvePanel, CXTResizeDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

namespace
{
	MFC_CreateCurvePanel* g_pCurvePanel = NULL;
	int g_nCurvePanelId = 0;
}

ICreateSphereDiscCurveToolPanel* CreateCurvePanel( CBrushDesignerDrawCurveTool* pCurveCreateTool, void* pData )
{
	if( !g_pCurvePanel )
	{
		g_pCurvePanel = new MFC_CreateCurvePanel(pCurveCreateTool);
		g_nCurvePanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Curve Attributes",g_pCurvePanel,false,(int)pData);
	}
	return g_pCurvePanel;
}

void MFC_CreateCurvePanel::DestroyPanel()
{
	if( g_nCurvePanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,g_nCurvePanelId);
		g_pCurvePanel = NULL;
		g_nCurvePanelId = 0;
	}
}

MFC_CreateCurvePanel::MFC_CreateCurvePanel( CBrushDesignerDrawCurveTool* pDrawCurveTool, CWnd* pParent ) : m_pCurveTool(pDrawCurveTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create(IDD,pParent);
}

void MFC_CreateCurvePanel::OnDestroy()
{
	int nNumOfSubdivision = BUtil::kDefaultCurveEdgeNum;
	if( m_pNumOfSubdivision )
		m_pNumOfSubdivision->Get(nNumOfSubdivision);

	AfxGetApp()->WriteProfileInt("DesignerSetting", "CurveSubdivisionNum", nNumOfSubdivision);
}

BOOL MFC_CreateCurvePanel::OnInitDialog()
{
	__super::OnInitDialog();

	InitEmptyPanel();

	SetCallBack(functor(*this,&MFC_CreateCurvePanel::OnInternalVariableChange));

	CVarBlock* pBlock = new CVarBlock;
	int numOfSubdivision = AfxGetApp()->GetProfileInt( "DesignerSetting", "CurveSubdivisionNum", BUtil::kDefaultCurveEdgeNum );
	m_pNumOfSubdivision = new CVariable<int>;
	m_pNumOfSubdivision->Set(numOfSubdivision);
	m_pNumOfSubdivision->SetLimits(1,32,1);
	pBlock->AddVariable(m_pNumOfSubdivision,"Subdivision");

	m_PropertyCtrl.AddVarBlock(pBlock);

	return TRUE;
}

int MFC_CreateCurvePanel::GetSubdivisionNum() const
{
	int nSubdivision = BUtil::kDefaultCurveEdgeNum;
	if( m_pNumOfSubdivision )
		m_pNumOfSubdivision->Get(nSubdivision);
	return nSubdivision;
}

void MFC_CreateCurvePanel::OnInternalVariableChange( IVariable* pVar )
{
}