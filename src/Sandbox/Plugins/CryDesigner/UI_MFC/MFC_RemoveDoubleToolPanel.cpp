#include "StdAfx.h"
#include "MFC_RemoveDoubleToolPanel.h"
#include "Tools/BrushDesignerRemoveDoubles.h"

IMPLEMENT_DYNAMIC(MFC_RemoveDoubleToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_RemoveDoubleToolPanel, CXTResizeDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

namespace
{
	MFC_RemoveDoubleToolPanel* s_pRemoveDoublesTool = NULL;
	int s_nRemoveDoublePanelId = 0;
}

IRemoveDoubleToolPanel* CreateRemoveDoubleToolPanel( CBrushDesignerRemoveDoublesTool* pRemoveDoubleTool, void* pData )
{
	if( !s_nRemoveDoublePanelId )
	{
		s_pRemoveDoublesTool = new MFC_RemoveDoubleToolPanel(pRemoveDoubleTool);
		s_nRemoveDoublePanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Remove Doubles Attributes",s_pRemoveDoublesTool,false,(int)pData);
	}
	return s_pRemoveDoublesTool;
}

void MFC_RemoveDoubleToolPanel::DestroyPanel()
{
	if( s_nRemoveDoublePanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,s_nRemoveDoublePanelId);
		s_pRemoveDoublesTool = NULL;
		s_nRemoveDoublePanelId = 0;
	}
}

MFC_RemoveDoubleToolPanel::MFC_RemoveDoubleToolPanel( CBrushDesignerRemoveDoublesTool* pRemoveDoubleTool, CWnd* pParent ) : m_pRemoveDoubleTool(pRemoveDoubleTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create(IDD,pParent);
}

BOOL MFC_RemoveDoubleToolPanel::OnInitDialog()
{
	__super::OnInitDialog();

	InitEmptyPanel();

	SetCallBack(functor(*this,&MFC_RemoveDoubleToolPanel::OnInternalVariableChange));

	CVarBlock* pBlock = new CVarBlock;
	float fDistance = (float)(AfxGetApp()->GetProfileInt("DesignerSetting", "RemoveDoubles_Distance",10)/1000.0f);
	m_Distance = new CVariable<float>;
	m_Distance->Set(fDistance);
	m_Distance->SetFlags(m_Distance->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	m_Distance->SetLimits(0.0001f, 100.0f, 0.001f);
	AddVariable(m_Distance);
	pBlock->AddVariable(m_Distance,"Distance");

	m_PropertyCtrl.AddVarBlock(pBlock);

	return TRUE;
}

void MFC_RemoveDoubleToolPanel::OnDestroy()
{
	int nDistance = GetDistance()*1000.0f;
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "RemoveDoubles_Distance", nDistance );
	DestroyVariables();
}

BrushFloat MFC_RemoveDoubleToolPanel::GetDistance() const
{
	float fDistance = 0;
	m_Distance->Get(fDistance);
	return (BrushFloat)fDistance;
}

void MFC_RemoveDoubleToolPanel::OnInternalVariableChange( IVariable* pVar )
{
	if( !m_bAllowChangeVariable )
		return;
}