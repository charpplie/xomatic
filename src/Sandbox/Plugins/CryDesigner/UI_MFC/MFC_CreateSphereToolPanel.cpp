#include "StdAfx.h"
#include "MFC_CreateSphereToolPanel.h"
#include "Tools/BrushDesignerCreateSphereTool.h"

IMPLEMENT_DYNAMIC(MFC_CreateSphereToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_CreateSphereToolPanel, CXTResizeDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

namespace 
{
	MFC_CreateSphereToolPanel* s_pSpherePrimitivePanel = NULL;
	int s_nSpherePrimitivePanelId = 0;
}

ICreateSphereDiscCurveToolPanel* CreateSpherePanel( CBrushDesignerCreateSphereTool* pSphereCreateTool, void* pData )
{
	if( !s_pSpherePrimitivePanel )
	{
		s_pSpherePrimitivePanel = new MFC_CreateSphereToolPanel(pSphereCreateTool);
		s_nSpherePrimitivePanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Sphere Attributes",s_pSpherePrimitivePanel,false,(int)pData);
	}
	return s_pSpherePrimitivePanel;
}

MFC_CreateSphereToolPanel::MFC_CreateSphereToolPanel( CBrushDesignerCreateSphereTool* pSphereTool, CWnd* pParent ) : m_pSphereTool(pSphereTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create( IDD,pParent );
}

void MFC_CreateSphereToolPanel::DestroyPanel()
{
	if( s_nSpherePrimitivePanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,s_nSpherePrimitivePanelId);
		s_pSpherePrimitivePanel = NULL;
		s_nSpherePrimitivePanelId = 0;
	}
}

BOOL MFC_CreateSphereToolPanel::OnInitDialog()
{
	__super::OnInitDialog();

	InitEmptyPanel();

	SetCallBack(functor(*this,&MFC_CreateSphereToolPanel::OnInternalVariableChange));

	CVarBlock* pBlock = new CVarBlock;

	int numOfSubdivision = AfxGetApp()->GetProfileInt( "DesignerSetting", "SphereSubdivisionNum", BUtil::kDefaultSubdivisionNum );
	if( numOfSubdivision > 32 )
		numOfSubdivision = 32;
	m_pNumOfSubdivision = new CVariable<int>;
	m_pNumOfSubdivision->Set(numOfSubdivision);
	m_pNumOfSubdivision->SetFlags(m_pNumOfSubdivision->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_pNumOfSubdivision->SetLimits(3,32,0.01f);
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

void MFC_CreateSphereToolPanel::OnDestroy()
{
	int numOfSubdivision = BUtil::kDefaultSubdivisionNum;
	m_pNumOfSubdivision->Get(numOfSubdivision);
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "SphereSubdivisionNum", numOfSubdivision );

	m_pRadius->RemoveOnSetCallback(m_ValueCallBack);
	m_pNumOfSubdivision->RemoveOnSetCallback(m_ValueCallBack);
}

void MFC_CreateSphereToolPanel::Update( float fRadius )
{
	m_bAllowChangeVariable = false;
	m_pRadius->Set(fRadius);
	m_bAllowChangeVariable = true;
}

void MFC_CreateSphereToolPanel::OnInternalVariableChange( IVariable* pVar )
{
	if( !m_bAllowChangeVariable )
		return;

	int numSide(4);
	m_pNumOfSubdivision->Get(numSide);

	float fRadius(0);
	m_pRadius->Get(fRadius);

	if( pVar == m_pNumOfSubdivision )
		m_pSphereTool->UpdateSphere(fRadius,numSide);
	else if( pVar == m_pRadius )
		m_pSphereTool->UpdateSphere(fRadius,numSide);
}

int MFC_CreateSphereToolPanel::GetSubdivisionNum() const
{
	int nNum = BUtil::kDefaultCurveEdgeNum;
	m_pNumOfSubdivision->Get(nNum);
	return nNum;
}