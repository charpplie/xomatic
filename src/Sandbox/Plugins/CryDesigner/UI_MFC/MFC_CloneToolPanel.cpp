#include "StdAfx.h"
#include "MFC_CloneToolPanel.h"
#include "Tools/BrushDesignerCloneTool.h"

IMPLEMENT_DYNAMIC(MFC_CloneToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_CloneToolPanel, CXTResizeDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

namespace
{
	MFC_CloneToolPanel* s_pDesignerCloneToolPanel = NULL;
	int s_nCloneToolPanelID = 0;
}

ICloneToolPanel* CreateCloneToolPanel( CBrushDesignerCloneTool* pCloneTool, void* pData )
{
	if( s_nCloneToolPanelID == 0 )
	{
		s_pDesignerCloneToolPanel = new MFC_CloneToolPanel(pCloneTool);
		s_nCloneToolPanelID = GetIEditor()->AddRollUpPage( ROLLUP_OBJECTS, _T("Clone Tool"), s_pDesignerCloneToolPanel,false,(int)pData );
	}
	return s_pDesignerCloneToolPanel;
}

void MFC_CloneToolPanel::DestroyPanel()
{
	if( s_nCloneToolPanelID != 0 )
	{
		GetIEditor()->RemoveRollUpPage( ROLLUP_OBJECTS, s_nCloneToolPanelID );
		s_pDesignerCloneToolPanel = NULL;
		s_nCloneToolPanelID = 0;
	}
}

MFC_CloneToolPanel::MFC_CloneToolPanel( CBrushDesignerCloneTool* pCloneTool, CWnd* pParent ) : m_pDesignerCloneTool(pCloneTool)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create( IDD,pParent );
}

int MFC_CloneToolPanel::GetNumOfClone()
{
	CString numberCloneString = GetNumberString(BUtil::kDefaultNumberOfClone);
	m_NumberOfClones->Get(numberCloneString);
	int numberOfClone = std::atoi(numberCloneString);
	if( numberOfClone == 0 )
	{
		numberOfClone = BUtil::kDefaultNumberOfClone;
		m_bAllowChangeVariable = false;
		m_NumberOfClones->Set(GetNumberString(numberOfClone));
		m_bAllowChangeVariable = true;
	}
	return numberOfClone;
}

BUtil::EPlacementType MFC_CloneToolPanel::GetPlacementType() const
{
	CString placement(BUtil::kDefaultPlacement);
	m_PlacementWayVar->Get(placement);
	if( placement == "Divide" )
		return BUtil::ePlacementType_Divide;
	return BUtil::ePlacementType_Multiply; 
}


BOOL MFC_CloneToolPanel::OnInitDialog()
{
	__super::OnInitDialog();

	InitEmptyPanel();
	SetCallBack(functor(*this,&MFC_CloneToolPanel::OnInternalVariableChange));

	m_bAllowChangeVariable = false;

	CVarBlock* pBlock = new CVarBlock;

	int numOfClone = AfxGetApp()->GetProfileInt( "DesignerSetting", "NumberOfClone", BUtil::kDefaultNumberOfClone );

	m_NumberOfClones = new CVariable<CString>;
	m_NumberOfClones->Set(GetNumberString(numOfClone));
	m_NumberOfClones->SetFlags(m_NumberOfClones->GetFlags()|IVariable::UI_EXPLICIT_STEP);
	AddVariable(m_NumberOfClones);
	pBlock->AddVariable(m_NumberOfClones,"Number");

	if( m_pDesignerCloneTool->GetArrangeType() == BUtil::eArrangeType_Array )
	{
		int placementType = AfxGetApp()->GetProfileInt( "DesignerSetting", "PlacementTypeOfClone", 0 );
		CVarEnumList<CString>* placementEnumList = new CVarEnumList<CString>;
		placementEnumList->AddItem("Divide","Divide");
		placementEnumList->AddItem("Multiply","Multiply");
		m_PlacementWayVar->SetEnumList(placementEnumList);
		m_PlacementWayVar->AddOnSetCallback(m_ValueCallBack);
		if( placementType == BUtil::ePlacementType_Divide )
			m_PlacementWayVar->Set("Divide");
		else
			m_PlacementWayVar->Set("Multiply");
		pBlock->AddVariable(m_PlacementWayVar,"Placement");
	}

	m_PropertyCtrl.AddVarBlock(pBlock);

	m_bAllowChangeVariable = true;

	return TRUE;
}

void MFC_CloneToolPanel::OnDestroy()
{
	CString cloneNumberStr = GetNumberString(BUtil::kDefaultNumberOfClone);
	m_NumberOfClones->Get(cloneNumberStr);
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "NumberOfClone", std::atoi(cloneNumberStr) );

	if( m_pDesignerCloneTool->GetArrangeType() == BUtil::eArrangeType_Array )
	{
		CString placement(BUtil::kDefaultPlacement);
		m_PlacementWayVar->Get(placement);
		if( placement == "Divide" )
			AfxGetApp()->WriteProfileInt( "DesignerSetting", "PlacementTypeOfClone", 0 );
		else
			AfxGetApp()->WriteProfileInt( "DesignerSetting", "PlacementTypeOfClone", 1 );
		m_PlacementWayVar->RemoveOnSetCallback(m_ValueCallBack);
	}

	DestroyVariables();
}

void MFC_CloneToolPanel::OnInternalVariableChange( IVariable* pVar )
{
	if( !m_bAllowChangeVariable )
		return;
	m_pDesignerCloneTool->Update();
}

CString MFC_CloneToolPanel::GetNumberString( int number ) const
{
	CString buffer;
	buffer.Format("%d",number);
	return buffer;
}
