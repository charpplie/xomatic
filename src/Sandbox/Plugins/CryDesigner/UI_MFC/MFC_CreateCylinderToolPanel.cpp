#include "StdAfx.h"
#include "MFC_CreateCylinderToolPanel.h"
#include "Tools/BrushDesignerCreateCylinderTool.h"

IMPLEMENT_DYNAMIC(MFC_CreateCylinderToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_CreateCylinderToolPanel, CXTResizeDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

namespace 
{
	MFC_CreateCylinderToolPanel* g_pCylinderPanel = NULL;
	int g_nCylinderPanelId = 0;
}

ICreateCylinderConeToolPanel* CreateCylinderPanel( CBrushDesignerCreateCylinderTool* pConeCreateTool, void* pData )
{
	if( !g_pCylinderPanel )
	{
		g_pCylinderPanel = new MFC_CreateCylinderToolPanel(pConeCreateTool);
		g_nCylinderPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Cylinder Attributes",g_pCylinderPanel,false,(int)pData);
	}
	return g_pCylinderPanel;
}

MFC_CreateCylinderToolPanel::MFC_CreateCylinderToolPanel( CBrushDesignerCreateCylinderTool* pCylinderCreateTool, CWnd* pParent ) : m_pCylinderTool(pCylinderCreateTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create( IDD,pParent );
}

void MFC_CreateCylinderToolPanel::DestroyPanel()
{
	if( g_nCylinderPanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,g_nCylinderPanelId);
		g_pCylinderPanel = NULL;
		g_nCylinderPanelId = 0;
	}
}

BOOL MFC_CreateCylinderToolPanel::OnInitDialog()
{
	__super::OnInitDialog();

	InitEmptyPanel();

	SetCallBack(functor(*this,&MFC_CreateCylinderToolPanel::OnInternalVariableChange));

	CVarBlock* pBlock = new CVarBlock;

	int numOfSubdivision = AfxGetApp()->GetProfileInt( "DesignerSetting", "CylinderSubdivisionNum", BUtil::kDefaultSubdivisionNum );
	m_pNumOfSubdivision = new CVariable<int>;
	m_pNumOfSubdivision->Set(numOfSubdivision);
	m_pNumOfSubdivision->SetFlags(m_pNumOfSubdivision->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_pNumOfSubdivision->SetLimits(3,128,0.01f);
	AddVariable(m_pNumOfSubdivision);
	pBlock->AddVariable(m_pNumOfSubdivision,"Subdivision");

	m_pHeight = new CVariable<float>;
	m_pHeight->Set(1);
	m_pHeight->SetFlags(m_pHeight->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_pHeight->SetLimits(0.01f,10000.0f,0.01f);
	AddVariable(m_pHeight);
	pBlock->AddVariable(m_pHeight,"Height");

	m_pRadius = new CVariable<float>;
	m_pRadius->Set(1);
	m_pRadius->SetFlags(m_pRadius->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_pRadius->SetLimits(0.01f,10000.0f,0.01f);
	AddVariable(m_pRadius);
	pBlock->AddVariable(m_pRadius,"Radius");

	m_PropertyCtrl.AddVarBlock(pBlock);

	return TRUE;
}

void MFC_CreateCylinderToolPanel::OnDestroy()
{
	int numOfSubdivision = BUtil::kDefaultSubdivisionNum;
	m_pNumOfSubdivision->Get(numOfSubdivision);
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "CylinderSubdivisionNum", numOfSubdivision );

	DestroyVariables();
}

void MFC_CreateCylinderToolPanel::Update( float fRadius, float fHeight )
{
	m_bAllowChangeVariable = false;
	m_pRadius->Set(fRadius);
	m_pHeight->Set(fHeight);
	m_bAllowChangeVariable = true;
}

int MFC_CreateCylinderToolPanel::GetSubdivisionNum() const
{
	int nNum = BUtil::kDefaultCurveEdgeNum;
	m_pNumOfSubdivision->Get(nNum);
	return nNum;
}

float MFC_CreateCylinderToolPanel::GetRadius() const
{
	float fRadius = 1.0f;
	m_pRadius->Get(fRadius);
	return fRadius;
}

void MFC_CreateCylinderToolPanel::OnInternalVariableChange( IVariable* pVar )
{
	if( !m_bAllowChangeVariable )
		return;

	int numSide(4);
	m_pNumOfSubdivision->Get(numSide);

	float fRadius(1);
	m_pRadius->Get(fRadius);

	float fHeight(1);
	m_pHeight->Get(fHeight);

	m_pCylinderTool->UpdateAll(fRadius,fHeight,numSide);
}
