#include "StdAfx.h"
#include "Tools/BrushDesignerStairTool.h"
#include "MFC_CreateStairToolPanel.h"

IMPLEMENT_DYNAMIC(MFC_CreateStairToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_CreateStairToolPanel, CXTResizeDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

namespace 
{
	MFC_CreateStairToolPanel* g_pStairPanel = NULL;
	int g_nStairPanelId = 0;
}

ICreateStairToolPanel* CreateStairPanel( CBrushDesignerStairTool* pStairCreateTool, void* pData )
{
	if( !g_pStairPanel )
	{
		g_pStairPanel = new MFC_CreateStairToolPanel(pStairCreateTool);
		g_nStairPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Stair Attributes",g_pStairPanel,false,(int)pData);
	}
	return g_pStairPanel;
}

MFC_CreateStairToolPanel::MFC_CreateStairToolPanel( CBrushDesignerStairTool* pStairCreateTool, CWnd* pParent ) : 
	m_pStairTool(pStairCreateTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create( IDD,pParent );
}

void MFC_CreateStairToolPanel::DestroyPanel()
{
	if( g_nStairPanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,g_nStairPanelId);
		g_pStairPanel = NULL;
		g_nStairPanelId = 0;
	}
}

BOOL MFC_CreateStairToolPanel::OnInitDialog()
{
	__super::OnInitDialog();

	InitEmptyPanel();

	SetCallBack(functor(*this,&MFC_CreateStairToolPanel::OnInternalVariableChange));

	CVarBlock* pBlock = new CVarBlock;

	float fStepRise = (AfxGetApp()->GetProfileInt("DesignerSetting", "StepRise", BUtil::kDefaultStepRise*1000))/1000.0f;
	m_StepRise = new CVariable<float>;
	m_StepRise->Set(fStepRise);
	m_StepRise->SetFlags(m_StepRise->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_StepRise->SetLimits(0,5,0.01f);
	AddVariable(m_StepRise);
	pBlock->AddVariable(m_StepRise,"Step Rise");

	m_bMirror = new CVariable<bool>;
	m_bMirror->Set(false);
	m_bMirror->SetFlags(m_bMirror->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	AddVariable(m_bMirror);
	pBlock->AddVariable(m_bMirror,"Mirror");

	m_bRotation90Degree = new CVariable<bool>;
	m_bRotation90Degree->Set(false);
	m_bRotation90Degree->SetFlags(m_bRotation90Degree->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	AddVariable(m_bRotation90Degree);
	pBlock->AddVariable(m_bRotation90Degree,"90 Degree");

	m_Width = new CVariable<float>;
	m_Width->Set(1);
	m_Width->SetFlags(m_Width->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_Width->SetLimits(0,10000.0f,0.01f);
	AddVariable(m_Width);
	pBlock->AddVariable(m_Width,"Width");

	m_Height = new CVariable<float>;
	m_Height->Set(1);
	m_Height->SetFlags(m_Height->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_Height->SetLimits(0,10000.0f,0.01f);
	AddVariable(m_Height);
	pBlock->AddVariable(m_Height,"Height");

	m_Depth = new CVariable<float>;
	m_Depth->Set(1);
	m_Depth->SetFlags(m_Depth->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_Depth->SetLimits(0,10000.0f,0.01f);
	AddVariable(m_Depth);
	pBlock->AddVariable(m_Depth,"Depth");

	m_PropertyCtrl.AddVarBlock(pBlock);

	return TRUE;
}

void MFC_CreateStairToolPanel::OnDestroy()
{
	int nStepRise = GetStepRise()*1000.0f;
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "StepRise", nStepRise );
	DestroyVariables();
}

BrushFloat MFC_CreateStairToolPanel::GetStepRise() const
{
	float fStepRise = 0;
	m_StepRise->Get(fStepRise);
	return (BrushFloat)fStepRise;
}

bool MFC_CreateStairToolPanel::IsMirrored() const
{
	bool bMirrored = false;
	m_bMirror->Get(bMirrored);
	return bMirrored;
}

void MFC_CreateStairToolPanel::SetMirrored( bool bMirrored )
{
	m_bAllowChangeVariable = false;
	m_bMirror->Set(bMirrored);
	m_bAllowChangeVariable = true;
}

bool MFC_CreateStairToolPanel::IsRotateBy90Degree() const
{
	bool bRotate90Degree = false;
	m_bRotation90Degree->Get(bRotate90Degree);
	return bRotate90Degree;
}

void MFC_CreateStairToolPanel::SetRotateBy90Degree( bool bRotateBy90Degree )
{
	m_bAllowChangeVariable = false;
	m_bRotation90Degree->Set(bRotateBy90Degree);
	m_bAllowChangeVariable = true;
}

void MFC_CreateStairToolPanel::Update( BrushFloat fWidth, BrushFloat fHeight, BrushFloat fDepth )
{
	m_bAllowChangeVariable = false;
	m_Width->Set((float)fWidth);
	m_Height->Set((float)fHeight);
	m_Depth->Set((float)fDepth);
	m_bAllowChangeVariable = true;
}

void MFC_CreateStairToolPanel::OnInternalVariableChange( IVariable* pVar )
{
	if( !m_bAllowChangeVariable )
		return;

	if( pVar == m_StepRise )
	{
		m_pStairTool->UpdateStair();
		return;
	}

	float fWidth = 0;
	float fHeight = 0;
	float fDepth = 0;

	m_Width->Get(fWidth);
	m_Height->Get(fHeight);
	m_Depth->Get(fDepth);

	m_pStairTool->UpdateStair((BrushFloat)fWidth,(BrushFloat)fHeight,(BrushFloat)fDepth);
}