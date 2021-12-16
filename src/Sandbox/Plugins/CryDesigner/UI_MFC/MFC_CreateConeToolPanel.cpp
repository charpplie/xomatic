#include "StdAfx.h"
#include "MFC_CreateConeToolPanel.h"
#include "Tools/BrushDesignerCreateConeTool.h"

IMPLEMENT_DYNAMIC(MFC_CreateConeToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_CreateConeToolPanel, CXTResizeDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

namespace 
{
	MFC_CreateConeToolPanel* g_pConePanel = NULL;
	int g_nConePanelId = 0;
}

ICreateCylinderConeToolPanel* CreateConePanel( CBrushDesignerCreateConeTool* pConeCreateTool, void* pData )
{
	if( !g_pConePanel )
	{
		g_pConePanel = new MFC_CreateConeToolPanel(pConeCreateTool);
		g_nConePanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Cone Attributes",g_pConePanel,false,(int)pData);
	}
	return g_pConePanel;
}

MFC_CreateConeToolPanel::MFC_CreateConeToolPanel( CBrushDesignerCreateConeTool* pConeCreateTool, CWnd* pParent ) : m_pConeTool(pConeCreateTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create( IDD,pParent );
}

void MFC_CreateConeToolPanel::DestroyPanel()
{
	if( g_nConePanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,g_nConePanelId);
		g_pConePanel = NULL;
		g_nConePanelId = 0;
	}
}

BOOL MFC_CreateConeToolPanel::OnInitDialog()
{
	__super::OnInitDialog();

	InitEmptyPanel();

	SetCallBack(functor(*this,&MFC_CreateConeToolPanel::OnInternalVariableChange));

	CVarBlock* pBlock = new CVarBlock;

	int numOfSubdivision = AfxGetApp()->GetProfileInt( "DesignerSetting", "ConeSubdivisionNum", BUtil::kDefaultSubdivisionNum );
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

void MFC_CreateConeToolPanel::OnDestroy()
{
	int numOfSubdivision = BUtil::kDefaultSubdivisionNum;
	m_pNumOfSubdivision->Get(numOfSubdivision);
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "ConeSubdivisionNum", numOfSubdivision );

	DestroyVariables();
}

void MFC_CreateConeToolPanel::Update( float fRadius, float fHeight )
{
	m_bAllowChangeVariable = false;
	m_pRadius->Set(fRadius);
	m_pHeight->Set(fHeight);
	m_bAllowChangeVariable = true;
}

int MFC_CreateConeToolPanel::GetSubdivisionNum() const
{
	int nNum = BUtil::kDefaultCurveEdgeNum;
	m_pNumOfSubdivision->Get(nNum);
	return nNum;
}

float MFC_CreateConeToolPanel::GetRadius() const
{
	float fRadius = 1.0f;
	m_pRadius->Get(fRadius);
	return fRadius;
}

void MFC_CreateConeToolPanel::OnInternalVariableChange( IVariable* pVar )
{
	if( !m_bAllowChangeVariable )
		return;

	int numSide(4);
	m_pNumOfSubdivision->Get(numSide);

	float fRadius(1);
	m_pRadius->Get(fRadius);

	float fHeight(1);
	m_pHeight->Get(fHeight);

	m_pConeTool->UpdateAll(fRadius,fHeight,numSide);
}
