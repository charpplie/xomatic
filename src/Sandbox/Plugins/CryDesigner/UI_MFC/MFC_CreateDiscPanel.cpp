#include "StdAfx.h"
#include "MFC_CreateDiscPanel.h"
#include "Tools/BrushDesignerDrawDiscTool.h"

IMPLEMENT_DYNAMIC(MFC_CreateDiscPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_CreateDiscPanel, CXTResizeDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

namespace
{
	MFC_CreateDiscPanel* g_pDiscPanel = NULL;
	int g_nDiscPanelId = 0;
}

ICreateSphereDiscCurveToolPanel* CreateDiscPanel( CBrushDesignerDrawDiscTool* pDiscCreateTool, void* pData )
{
	if( !g_pDiscPanel )
	{
		g_pDiscPanel = new MFC_CreateDiscPanel(pDiscCreateTool);
		g_nDiscPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Disc Attributes",g_pDiscPanel,false,(int)pData);
	}
	return g_pDiscPanel;
}

MFC_CreateDiscPanel::MFC_CreateDiscPanel( CBrushDesignerDrawDiscTool* pDiscTool, CWnd* pParent ) : m_pDiscTool(pDiscTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create( IDD,pParent );
}

void MFC_CreateDiscPanel::DestroyPanel()
{
	if( g_nDiscPanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,g_nDiscPanelId);
		g_pDiscPanel = NULL;
		g_nDiscPanelId = 0;
	}
}

BOOL MFC_CreateDiscPanel::OnInitDialog()
{
	__super::OnInitDialog();

	InitEmptyPanel();

	SetCallBack(functor(*this,&MFC_CreateDiscPanel::OnInternalVariableChange));

	CVarBlock* pBlock = new CVarBlock;

	int numOfSubdivision = AfxGetApp()->GetProfileInt( "DesignerSetting", "DiscSubdivisionNum", BUtil::kDefaultSubdivisionNum );
	m_pNumOfSubdivision = new CVariable<int>;
	m_pNumOfSubdivision->Set(numOfSubdivision);
	m_pNumOfSubdivision->SetFlags(m_pNumOfSubdivision->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_pNumOfSubdivision->SetLimits(3,128,0.01f);
	AddVariable(m_pNumOfSubdivision);
	pBlock->AddVariable(m_pNumOfSubdivision,"Subdivision");

	m_pRadius = new CVariable<float>;
	m_pRadius->Set(1);
	m_pRadius->SetFlags(m_pRadius->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_pRadius->SetLimits(0.01f,10000.0f,0.01f);
	AddVariable(m_pRadius);
	pBlock->AddVariable(m_pRadius,"Radius");

	m_PropertyCtrl.AddVarBlock(pBlock);

	return TRUE;
}

void MFC_CreateDiscPanel::OnDestroy()
{
	int numOfSubdivision = BUtil::kDefaultSubdivisionNum;
	m_pNumOfSubdivision->Get(numOfSubdivision);
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "DiscSubdivisionNum", numOfSubdivision );

	DestroyVariables();
}

void MFC_CreateDiscPanel::Update( float fRadius )
{
	m_bAllowChangeVariable = false;
	m_pRadius->Set(fRadius);
	m_bAllowChangeVariable = true;
}

void MFC_CreateDiscPanel::OnInternalVariableChange( IVariable* pVar )
{
	if( !m_bAllowChangeVariable )
		return;

	int numSide(4);
	m_pNumOfSubdivision->Get(numSide);

	float fRadius(0);
	m_pRadius->Get(fRadius);

	m_pDiscTool->UpdateDisc(fRadius,numSide);
}

int MFC_CreateDiscPanel::GetSubdivisionNum() const
{
	int nNum = BUtil::kDefaultCurveEdgeNum;
	m_pNumOfSubdivision->Get(nNum);
	return nNum;
}
