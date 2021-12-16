#include "StdAfx.h"
#include "MFC_CreateStairProfileToolPanel.h"

IMPLEMENT_DYNAMIC(MFC_CreateStairProfileToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_CreateStairProfileToolPanel, CXTResizeDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

namespace
{
	MFC_CreateStairProfileToolPanel* s_pStairProfilePanel = NULL;
	int s_nStairProfilePanelId = 0;
}

ICreateStairProfileToolPanel* CreateStairProfilePanel( CBrushDesignerStairProfileTool* pStairProfileCreateTool, void* pData )
{
	if( !s_pStairProfilePanel )
	{
		s_pStairProfilePanel = new MFC_CreateStairProfileToolPanel(pStairProfileCreateTool);
		s_nStairProfilePanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Stair Profile Attributes",s_pStairProfilePanel,false,(int)pData);
	}
	return s_pStairProfilePanel;
}

void MFC_CreateStairProfileToolPanel::DestroyPanel()
{
	if( s_nStairProfilePanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,s_nStairProfilePanelId);
		s_pStairProfilePanel = NULL;
		s_nStairProfilePanelId = 0;
	}
}

MFC_CreateStairProfileToolPanel::MFC_CreateStairProfileToolPanel( CBrushDesignerStairProfileTool* pStairTool, CWnd* pParent ) : 
	m_pStairTool(pStairTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create( IDD,pParent );
}

BOOL MFC_CreateStairProfileToolPanel::OnInitDialog()
{
	__super::OnInitDialog();

	InitEmptyPanel();

	SetCallBack(functor(*this,&MFC_CreateStairProfileToolPanel::OnInternalVariableChange));

	CVarBlock* pBlock = new CVarBlock;

	float fStepRise = (AfxGetApp()->GetProfileInt("DesignerSetting", "StepRise", BUtil::kDefaultStepRise*1000))/1000.0f;
	m_StepRise = new CVariable<float>;
	m_StepRise->Set(fStepRise);
	m_StepRise->SetFlags(m_StepRise->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_StepRise->SetLimits(0,5,0.01f);
	AddVariable(m_StepRise);
	pBlock->AddVariable(m_StepRise,"Step Rise");

	m_PropertyCtrl.AddVarBlock(pBlock);

	return TRUE;
}

void MFC_CreateStairProfileToolPanel::OnDestroy()
{
	int nStepRise = GetStepRise()*1000.0f;
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "StepRise", nStepRise );
	DestroyVariables();
}

BrushFloat MFC_CreateStairProfileToolPanel::GetStepRise() const
{
	float fStepRise = 0;
	m_StepRise->Get(fStepRise);
	return (BrushFloat)fStepRise;
}

void MFC_CreateStairProfileToolPanel::OnInternalVariableChange( IVariable* pVar )
{
	if( !m_bAllowChangeVariable )
		return;
}