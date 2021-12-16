#include "StdAfx.h"
#include "MFC_CreateRectangleToolPanel.h"
#include "Tools/BrushDesignerDrawRectangleTool.h"

IMPLEMENT_DYNAMIC(MFC_CreateRectangleToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_CreateRectangleToolPanel, CXTResizeDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

namespace 
{
	MFC_CreateRectangleToolPanel* s_pRectanglePanel = NULL;
	int s_nRectanglePanelId = 0;
}

ICreateRectangleToolPanel* CreateRectanglePanel( CBrushDesignerDrawRectangleTool* pRectTool, void* pData )
{
	if( !s_pRectanglePanel )
	{
		s_pRectanglePanel = new MFC_CreateRectangleToolPanel(pRectTool);
		s_nRectanglePanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Rectangle Attributes",s_pRectanglePanel,false,(int)pData);
	}
	return s_pRectanglePanel;
}

MFC_CreateRectangleToolPanel::MFC_CreateRectangleToolPanel( CBrushDesignerDrawRectangleTool* pRectangleTool, CWnd* pParent ) : m_pRectangleTool(pRectangleTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create( IDD,pParent );
}

void MFC_CreateRectangleToolPanel::DestroyPanel()
{
	if( s_nRectanglePanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,s_nRectanglePanelId);
		s_pRectanglePanel = NULL;
		s_nRectanglePanelId = 0;
	}
}

BOOL MFC_CreateRectangleToolPanel::OnInitDialog()
{
	__super::OnInitDialog();

	InitEmptyPanel();
	SetCallBack(functor(*this,&MFC_CreateRectangleToolPanel::OnInternalVariableChange));

	CVarBlock* pBlock = new CVarBlock;

	m_Width = new CVariable<float>;
	m_Width->Set(1);
	m_Width->SetFlags(m_Width->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_Width->SetLimits(0.01f,10000.0f,0.01f);
	AddVariable(m_Width);
	pBlock->AddVariable(m_Width,"Width");

	m_Depth = new CVariable<float>;
	m_Depth->Set(1);
	m_Depth->SetFlags(m_Depth->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_Depth->SetLimits(0.01f,10000.0f,0.01f);
	AddVariable(m_Depth);
	pBlock->AddVariable(m_Depth,"Depth");

	m_PropertyCtrl.AddVarBlock(pBlock);

	return TRUE;
}

void MFC_CreateRectangleToolPanel::OnDestroy()
{
	DestroyVariables();
}

void MFC_CreateRectangleToolPanel::Update( float fWidth, float fDepth )
{
	m_bAllowChangeVariable = false;

	m_Width->Set(fWidth);
	m_Depth->Set(fDepth);

	m_bAllowChangeVariable = true;
}

void MFC_CreateRectangleToolPanel::OnInternalVariableChange( IVariable* pVar )
{
	if( !m_bAllowChangeVariable )
		return;

	BrushVec3* v = m_pRectangleTool->GetRectangleTwoVertices();

	float fWidth = 0;
	float fDepth = 0;

	m_Width->Get(fWidth);
	m_Depth->Get(fDepth);

	BrushVec2 p0 = m_pRectangleTool->GetPlane().W2P(v[0]);
	BrushVec2 p1 = m_pRectangleTool->GetPlane().W2P(v[1]);
	BrushVec2 center = (p0+p1)*0.5f;

	p0.x = center.x - fWidth*0.5f;
	p1.x = center.x + fWidth*0.5f;

	p0.y = center.y - fDepth*0.5f;
	p1.y = center.y + fDepth*0.5f;

	m_pRectangleTool->UpdateRectangle(m_pRectangleTool->GetPlane().P2W(p0),m_pRectangleTool->GetPlane().P2W(p1),true);
}