#include "StdAfx.h"
#include "BrushDesignerHideFace.h"
#include "BrushDesignerSelectTool.h"
#include "BrushDesignerEditTool.h"

class CBrushDesignerHideFacePanel : public CXTResizeDialog
{
	DECLARE_DYNAMIC(CBrushDesignerHideFacePanel)

public:
	CBrushDesignerHideFacePanel(CBrushDesignerHideFaceTool* pHideFaceTool) : CXTResizeDialog(CBrushDesignerHideFacePanel::IDD, NULL),
		m_pDesignerHideFaceTool(pHideFaceTool)
	{
		RESOURCEHANDLER_RECONSTRUCTOR;
		Create(IDD,NULL);
	}
	virtual ~CBrushDesignerHideFacePanel(){}

	enum { IDD = IDD_PANEL_BRUSHDESIGNER_HIDEFACE };

protected:
	void OnOK() {};
	void OnCancel() {};
	void DoDataExchange(CDataExchange* pDX)
	{
		CXTResizeDialog::DoDataExchange(pDX);
	}

	BOOL OnInitDialog()
	{
		CXTResizeDialog::OnInitDialog();
		return TRUE;
	}

	void PostNcDestroy(){ delete this; }

	CBrushDesignerHideFaceTool* m_pDesignerHideFaceTool;

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedDesignerUnhideall()
	{
		m_pDesignerHideFaceTool->UnhideAll();
	}
};

IMPLEMENT_DYNAMIC(CBrushDesignerHideFacePanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(CBrushDesignerHideFacePanel, CXTResizeDialog)
	ON_BN_CLICKED(IDC_DESIGNER_UNHIDEALL, OnBnClickedDesignerUnhideall)
END_MESSAGE_MAP()

namespace 
{
	CBrushDesignerHideFacePanel* s_pHideFacePanel = NULL;
	int s_nHideFacePanelId = 0;
}

void CBrushDesignerHideFaceTool::BeginEditParams()
{
	if( !s_nHideFacePanelId )
	{
		s_pHideFacePanel = new CBrushDesignerHideFacePanel(this);
		s_nHideFacePanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"HideFce Tool",s_pHideFacePanel,false,GetPanelIndex());
	}
}

void CBrushDesignerHideFaceTool::EndEditParams()
{
	if( s_nHideFacePanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,s_nHideFacePanelId);
		s_pHideFacePanel = NULL;
		s_nHideFacePanelId = 0;
	}
}

void CBrushDesignerHideFaceTool::Enter()
{
	__super::Enter();

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	std::vector<CBrushRegion::RegionPtr> selectedRegions;

	for( int i = 0, iCount(pSelected->GetSize()); i < iCount; ++i )
	{
		if( !(*pSelected)[i].IsFace() || (*pSelected)[i].m_pRegion == NULL )
			continue;
		selectedRegions.push_back((*pSelected)[i].m_pRegion);
	}
	
	if( !selectedRegions.empty() )
	{
		CUndo undo("Designer : Hide Face(s)");
		GetDesigner()->RecordUndo("Designer : Hide Face(s)",GetBaseObject());
		for( int i = 0, iRegionCount(selectedRegions.size()); i < iRegionCount; ++i )
			selectedRegions[i]->AddFlags(CBrushRegion::eRF_Hidden);
		pSelected->Clear();
		GetEditTool()->SetDesignerMode(BUtil::eDesigner_Select_Face);
		CreateMirroredRegions(GetDesigner());
		Sync();
		UpdateBrush();
	}
}

void CBrushDesignerHideFaceTool::UnhideAll()
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(0);

	CBrushDesignerElementManager hiddenElements;
	for( int i = 0, iCount(GetDesigner()->GetRegionSize()); i < iCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(i);
		if( pRegion->CheckFlags(CBrushRegion::eRF_Hidden) && !pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) )	
			hiddenElements.Add(SDesignerElement(GetBaseObject(),pRegion));
	}
	
	if( !hiddenElements.IsEmpty() )
	{
		CUndo undo("Designer : Unhide Face(s)");
		GetDesigner()->RecordUndo("Designer : Unhide Face(s)",GetBaseObject());

		for( int i = 0, iElementCount(hiddenElements.GetSize()); i < iElementCount; ++i )
			hiddenElements[i].m_pRegion->RemoveFlags(CBrushRegion::eRF_Hidden);
		
		CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
		pSelected->Set(hiddenElements);
		CreateMirroredRegions(GetDesigner());

		UpdateBrush();
		GetEditTool()->SetDesignerMode(BUtil::eDesigner_Select_Face);
	}
	else
	{
		GetEditTool()->GoToPrevDesignerMode();
	}
}