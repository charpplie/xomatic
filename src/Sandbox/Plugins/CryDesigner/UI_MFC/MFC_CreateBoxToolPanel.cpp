#include "StdAfx.h"
#include "MFC_CreateBoxToolPanel.h"
#include "Tools/BrushDesignerCreateBoxTool.h"

namespace 
{
	MFC_CreateBoxToolPanel* s_pBoxPrimitivePanel = NULL;
	int s_nBoxPrimitivePanelId = 0;
}

ICreateBoxToolPanel* CreateBoxPanel( CBrushDesignerCreateBoxTool* pBoxCreateTool, void* pData )
{
	if( !s_pBoxPrimitivePanel )
	{
		s_pBoxPrimitivePanel = new MFC_CreateBoxToolPanel(pBoxCreateTool);
		s_nBoxPrimitivePanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Box Attributes",s_pBoxPrimitivePanel,false,(int)pData);
	}
	return s_pBoxPrimitivePanel;
}

IMPLEMENT_DYNAMIC(MFC_CreateBoxToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_CreateBoxToolPanel, CXTResizeDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

MFC_CreateBoxToolPanel::MFC_CreateBoxToolPanel(  CBrushDesignerCreateBoxTool* pBoxCreateTool, CWnd* pParent ) : m_pBoxCreateTool(pBoxCreateTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create( IDD,pParent );
}

void MFC_CreateBoxToolPanel::DestroyPanel()
{
	if( s_nBoxPrimitivePanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,s_nBoxPrimitivePanelId);
		s_pBoxPrimitivePanel = NULL;
		s_nBoxPrimitivePanelId = 0;
	}
}

void MFC_CreateBoxToolPanel::Update( const BrushVec2& p0, const BrushVec2& p1, BrushFloat fHeight )
{
	m_bAllowChangeVariable = false;

	float fWidth = std::abs(p0.x-p1.x);
	float fDepth = std::abs(p0.y-p1.y);

	m_Width->Set(fWidth);
	m_Height->Set((float)fHeight);
	m_Depth->Set(fDepth);

	m_bAllowChangeVariable = true;
}

BOOL MFC_CreateBoxToolPanel::OnInitDialog()
{
	__super::OnInitDialog();

	InitEmptyPanel();

	SetCallBack(functor(*this,&MFC_CreateBoxToolPanel::OnInternalVariableChange));

	CVarBlock* pBlock = new CVarBlock;

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

void MFC_CreateBoxToolPanel::OnDestroy()
{
	DestroyVariables();
}

void MFC_CreateBoxToolPanel::OnInternalVariableChange( IVariable* pVar )
{
	if( !m_bAllowChangeVariable )
		return;

	BrushVec3* v = m_pBoxCreateTool->GetBottomRectangleTwoVertices();

	BrushVec2 p0 = m_pBoxCreateTool->GetPlane().W2P(v[0]);
	BrushVec2 p1 = m_pBoxCreateTool->GetPlane().W2P(v[1]);
	BrushVec2 center = (p0+p1)*0.5f;

	float fWidth = 0;
	float fHeight = 0;
	float fDepth = 0;

	m_Width->Get(fWidth);
	m_Height->Get(fHeight);
	m_Depth->Get(fDepth);

	p0.x = center.x - fWidth*0.5f;
	p1.x = center.x + fWidth*0.5f;
	p0.y = center.y - fDepth*0.5f;
	p1.y = center.y + fDepth*0.5f;

	m_pBoxCreateTool->UpdateBoxWithBoundaryCheck(m_pBoxCreateTool->GetPlane().P2W(p0),m_pBoxCreateTool->GetPlane().P2W(p1),fHeight);
}