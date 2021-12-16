////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   BrushDesignerEditToolPanel.cpp
//  Version:     v1.00
//  Created:     8/12/2011 by Jaesik.
//  Compilers:   Visual Studio 2008
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "MFC_BrushDesignerEditToolPanel.h"
#include "Tools/BrushDesignerEditTool.h"
#include "ViewManager.h"
#include "Tools/BrushDesignerDrawTool.h"
#include "Objects/AreaSolidObject.h"
#include "Core/BrushDesignerGlobalSettings.h"
#include "Core/BrushDesignerSmoothingGroupManager.h"
#include "IBaseToolPanel.h"

namespace 
{
	MFC_BrushDesignerEditToolPanel* s_pMenuPanel = NULL;
	int s_nMenuPanelIndex = 0;
	int s_nMenuPanelId = 0;	
}

IBrushDesignerEditToolPanel* CreateDesignerEditToolPanel()
{
	if( !s_nMenuPanelId )
	{
		s_pMenuPanel = new MFC_BrushDesignerEditToolPanel;
		s_nMenuPanelId = GetIEditor()->AddRollUpPage( ROLLUP_OBJECTS, _T("Designer Menu"), s_pMenuPanel, false );
		s_nMenuPanelIndex = GetIEditor()->GetRollUpPageCount(ROLLUP_OBJECTS);
	}
	return s_pMenuPanel;
}

void MFC_BrushDesignerEditToolPanel::DestroyPanel()
{
	if(s_pMenuPanel)
	{
		GetIEditor()->RemoveRollUpPage( ROLLUP_OBJECTS,s_nMenuPanelId );
		s_pMenuPanel = 0;
		s_nMenuPanelId = 0;
	}
}

IMPLEMENT_DYNAMIC(MFC_BrushDesignerEditToolPanel, DesignerPanelDialog)
MFC_BrushDesignerEditToolPanel::MFC_BrushDesignerEditToolPanel( CWnd* pParent )
: DesignerPanelDialog(MFC_BrushDesignerEditToolPanel::IDD, pParent)
{
	m_pEditTool = NULL;
	m_bExclusiveModeBeforeSave = false;
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create(IDD,pParent);
}

//////////////////////////////////////////////////////////////////////////
MFC_BrushDesignerEditToolPanel::~MFC_BrushDesignerEditToolPanel ()
{
}

int MFC_BrushDesignerEditToolPanel::GetPanelIndex()
{
	return s_nMenuPanelIndex;
}

//////////////////////////////////////////////////////////////////////////
void MFC_BrushDesignerEditToolPanel::DoDataExchange(CDataExchange* pDX)
{
	DesignerPanelDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(MFC_BrushDesignerEditToolPanel , DesignerPanelDialog)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_DESIGNER_PRIMITIVE_BOX, OnBnClickedDesignerPrimitiveBox)
	ON_BN_CLICKED(IDC_DESIGNER_PRIMITIVE_SPHERE, OnBnClickedDesignerPrimitiveSphere)
	ON_BN_CLICKED(IDC_DESIGNER_PRIMITIVE_CYLINDER, OnBnClickedDesignerPrimitiveCylinder)
	ON_BN_CLICKED(IDC_DESIGNER_PRIMITIVE_CONE, OnBnClickedDesignerPrimitiveCone)
	ON_BN_CLICKED(IDC_DESIGNER_PRIMITIVE_RECTANGLE, OnBnClickedDesignerPrimitiveRectangle)
	ON_BN_CLICKED(IDC_DESIGNER_PRIMITIVE_DISC, OnBnClickedDesignerPrimitiveDisc)
	ON_BN_CLICKED(IDC_DESIGNER_DRAWING_LINE, OnBnClickedDesignerDrawingLine)
	ON_BN_CLICKED(IDC_DESIGNER_DRAWING_CURVE, OnBnClickedDesignerDrawingCurve)
	ON_BN_CLICKED(IDC_DESIGNER_WELDTOOL, OnBnClickedDesignerWeldTool)
	ON_BN_CLICKED(IDC_DESIGNER_SLICETOOL, OnBnClickedDesignerSliceTool)
	ON_BN_CLICKED(IDC_DESIGNER_REMOVETOOL, OnBnClickedDesignerRemoveTool)
	ON_BN_CLICKED(IDC_DESIGNER_FILLTOOL, OnBnClickedDesignerFillTool)
	ON_BN_CLICKED(IDC_DESIGNER_EXTRUDETOOL, OnBnClickedDesignerExtrudeTool)
	ON_BN_CLICKED(IDC_DESIGNER_OFFSETTOOL, OnBnClickedDesignerOffsetTool)
	ON_BN_CLICKED(IDC_DESIGNER_SEPARATETOOL, OnBnClickedDesignerSeparateTool)
	ON_BN_CLICKED(IDC_DESIGNER_MERGETOOL, OnBnClickedDesignerMergeTool)
	ON_BN_CLICKED(IDC_DESIGNER_COPYTOOL, OnBnClickedDesignerCopyTool)
	ON_BN_CLICKED(IDC_DESIGNER_FLIPTOOL, OnBnClickedDesignerFlipTool)
	ON_BN_CLICKED(IDC_DESIGNER_BEVELTOOL, OnBnClickedDesignerBevelTool)
	ON_BN_CLICKED(IDC_DESIGNER_STAIRTOOL, OnBnClickedDesignerStairTool)
	ON_BN_CLICKED(IDC_DESIGNER_STAIRPROFILETOOL, OnBnClickedDesignerStairProfileTool)
	ON_BN_CLICKED(IDC_DESIGNER_CLONETOOL, OnBnClickedDesignerCloneTool)
	ON_BN_CLICKED(IDC_DESIGNER_ARRAYCLONETOOL, OnBnClickedDesignerArraycloneTool)
	ON_BN_CLICKED(IDC_DESIGNER_CIRCLECLONETOOL, OnBnClickedDesignerCirclecloneTool)
	ON_BN_CLICKED(IDC_DESIGNER_MIRRORTOOL, OnBnClickedDesignerMirrorTool)
	ON_BN_CLICKED(IDC_DESIGNER_LATHETOOL, OnBnClickedDesignerLatheTool)
	ON_BN_CLICKED(IDC_DESIGNER_MAPPINGTOOL, OnBnClickedDesignerMappingTool)
	ON_BN_CLICKED(IDC_DESIGNER_DEBUGGERTOOL, OnBnClickedDesignerDebuggerTool)
	ON_BN_CLICKED(IDC_DESIGNER_RESETXFORM, OnBnClickedDesignerResetXFormTool)
	ON_BN_CLICKED(IDC_DESIGNER_EXPORTTOOL, OnBnClickedDesignerExportTool)
	ON_BN_CLICKED(IDC_DESIGNER_SELECT_PIVOT, OnBnClickedDesignerSetuppivotTool)
	ON_BN_CLICKED(IDC_DESIGNER_DISPLAY_BACKFACES, OnBnClickedDesignerDisplayBackfaces)
	ON_BN_CLICKED(IDC_DESIGNER_EXCLUSIVEMODE, OnBnClickedDesignerExclusivemode)
	ON_BN_CLICKED(IDC_DESIGNER_OBJECTMODE, OnBnClickedDesignerObjectmode)
	ON_BN_CLICKED(IDC_DESIGNER_BOOLEANTOOL, OnBnClickedDesignerBooleantool)
	ON_BN_CLICKED(IDC_DESIGNER_SELECT_VERTEX, OnBnClickedDesignerSelectVertex)
	ON_BN_CLICKED(IDC_DESIGNER_SELECT_EDGE, OnBnClickedDesignerSelectEdge)
	ON_BN_CLICKED(IDC_DESIGNER_SELECT_FACE, OnBnClickedDesignerSelectFace)
	ON_BN_CLICKED(IDC_DESIGNER_PIVOT2BOTTOM, OnBnClickedDesignerPivot2Bottom)
	ON_BN_CLICKED(IDC_DESIGNER_SELECT_ALLNONE, OnBnClickedDesignerSelectAllnone)
	ON_BN_CLICKED(IDC_DESIGNER_SELECT_CONNECTED, OnBnClickedDesignerSelectConnected)
	ON_BN_CLICKED(IDC_DESIGNER_SELECT_GROW, OnBnClickedDesignerSelectGrow)
	ON_BN_CLICKED(IDC_DESIGNER_SELECT_LOOP, OnBnClickedDesignerSelectLoop)
	ON_BN_CLICKED(IDC_DESIGNER_SELECT_RING, OnBnClickedDesignerSelectionRing)
	ON_BN_CLICKED(IDC_DESIGNER_SELECT_INVERT, OnBnClickedDesignerSelectInvert)
	ON_BN_CLICKED(IDC_DESIGNER_SNAPTOGRID, OnBnClickedDesignerSnapToGrid)
	ON_BN_CLICKED(IDC_DESIGNER_SMOOTHINGGROUP, OnBnClickedDesignerSmoothinggroup)
	ON_BN_CLICKED(IDC_DESIGNER_MAGNETTOOL,OnBnClickedDesignerMagnetTool)
	ON_BN_CLICKED(IDC_DESIGNER_REMOVEDOUBLES, OnBnClickedDesignerRemoveDoubles)
	ON_BN_CLICKED(IDC_DESIGNER_HIDEFACE, OnBnClickedDesignerHideFace)
	ON_BN_CLICKED(IDC_DESIGNER_CUBEEDITOR, OnBnClickedDesignerCubeeditor)
	ON_BN_CLICKED(IDC_DESIGNER_SUBDIVISION, OnBnClickedDesignerSubdivisionTool)
	ON_BN_CLICKED(IDC_DESIGNER_SEAMLESSEDIT, OnBnClickedDesignerSeamlessedit)
END_MESSAGE_MAP()

void MFC_BrushDesignerEditToolPanel::SetEditTool( CBrushDesignerEditTool* pTool, BUtil::EDesignerMode designerMode )
{
	m_pEditTool = pTool;
	DESIGNER_ASSERT(m_pEditTool && m_pEditTool->GetDesigner());
	if( m_pEditTool && m_pEditTool->GetDesigner() )
	{
		DESIGNER_SHELF_RECONSTRUCTOR(m_pEditTool->GetDesigner());
		m_pEditTool->GetDesigner()->SetShelf(0);
		if( designerMode == BUtil::eDesigner_Max || designerMode == BUtil::eDesigner_Merge || designerMode == BUtil::eDesigner_Boolean )
		{
			if( m_pEditTool->GetDesigner()->GetRegionSize() > 0 )
				m_pEditTool->SetDesignerMode(BUtil::eDesigner_ObjectMode,true);
			else
				m_pEditTool->SetDesignerMode(BUtil::eDesigner_Box,true);
		}
		else
		{
			m_pEditTool->SetDesignerMode(designerMode,true);
		}

		CButton* pCheckBox = (CButton*)GetDlgItem(IDC_DESIGNER_DISPLAY_BACKFACES);
		if( pCheckBox )
		{
			if( m_pEditTool->GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_DisplayBackFace) )
				pCheckBox->SetCheck(BST_CHECKED);
		}

		UpdateCloneArrayButtons();
	}
}

void MFC_BrushDesignerEditToolPanel::SetButtonCheck( int nButtonID, int nCheckID )
{
	CButton* pButton = (CButton*)GetDlgItem(nButtonID);
	if( pButton )
		pButton->SetCheck(nCheckID);;
}

void MFC_BrushDesignerEditToolPanel::UpdateCloneArrayButtons()
{
	if( !m_pEditTool )
		return;

	CBaseObject* pObj = m_pEditTool->GetBaseObject();
	if( !pObj )
		return;

	if( pObj->GetParent() && pObj->GetParent() )
	{
		GetDlgItem(IDC_DESIGNER_CLONETOOL)->EnableWindow(false);
		GetDlgItem(IDC_DESIGNER_ARRAYCLONETOOL)->EnableWindow(false);
	}
	else if( pObj->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
	{
		GetDlgItem(IDC_DESIGNER_CLONETOOL)->EnableWindow(true);
		GetDlgItem(IDC_DESIGNER_ARRAYCLONETOOL)->EnableWindow(true);
	}
}

BOOL MFC_BrushDesignerEditToolPanel::OnInitDialog()
{
	DesignerPanelDialog::OnInitDialog();

	SDesignerEnvironmentInfo& envInfo = CBrushDesignerEditTool::GetGlobalEnvironmentInfo();
	envInfo.Load();

	CButton* pCheckBox = (CButton*)GetDlgItem(IDC_DESIGNER_EXCLUSIVEMODE);
	if( pCheckBox )
		pCheckBox->SetCheck(envInfo.IsEnableExclusiveMode() ? BST_CHECKED : BST_UNCHECKED);

	if( pCheckBox = (CButton*)GetDlgItem(IDC_DESIGNER_SEAMLESSEDIT) )
		pCheckBox->SetCheck( gSettings.bDesignerSeamlessSelection ? BST_CHECKED : BST_UNCHECKED);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void MFC_BrushDesignerEditToolPanel::OnDestroy()
{
	SDesignerEnvironmentInfo& envInfo = CBrushDesignerEditTool::GetGlobalEnvironmentInfo();
	envInfo.Save();
	gSettings.Save();
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSelectVertex()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Select_Vertex);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSelectEdge()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Select_Edge);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSelectFace()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Select_Face);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerPrimitiveBox()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Box);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerPrimitiveSphere()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Sphere);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerPrimitiveCylinder()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Cylinder);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerPrimitiveCone()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Cone);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerPrimitiveRectangle()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Rectangle);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerPrimitiveDisc()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Disc);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerDrawingLine()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Line);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerDrawingCurve()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Curve);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerWeldTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Weld);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSliceTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Slice);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerRemoveTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Remove);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerFillTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Fill);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerExtrudeTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Extrude);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerOffsetTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Offset);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSeparateTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Seprate);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerMergeTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Merge);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerCopyTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Copy);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerFlipTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Flip);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerBevelTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Bevel);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerStairTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Stair);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerStairProfileTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_StairProfile);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerCloneTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Clone);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerArraycloneTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Array);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerCirclecloneTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Clone);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerMirrorTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Mirror);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerLatheTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Lathe);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerMappingTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Mapping);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerDebuggerTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Debugger);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerResetXFormTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_ResetXForm);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerExportTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Export);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSetuppivotTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Pivot);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerObjectmode()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_ObjectMode);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerBooleantool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Boolean);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerPivot2Bottom()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Pivot2Bottom);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSelectAllnone()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Select_AllNone);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSelectConnected()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Select_Connected);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSelectGrow()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Select_Grow);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSelectLoop()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Select_Loop);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSelectionRing()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Select_Ring);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSelectInvert()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Select_Invert);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSnapToGrid()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_SnapToGrid);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSmoothinggroup()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_SmoothingGroup);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerMagnetTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Magnet);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerRemoveDoubles()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_RemoveDoubles);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerHideFace()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_HideFace);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerCubeeditor()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_CubeEditor);
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSubdivisionTool()
{
	if( m_pEditTool )
		m_pEditTool->SetDesignerMode(BUtil::eDesigner_Subdivision);
}

void MFC_BrushDesignerEditToolPanel::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
	SDesignerEnvironmentInfo& envInfo = CBrushDesignerEditTool::GetGlobalEnvironmentInfo();

	switch(event)
	{
	case eNotify_OnSelectionChange:
		UpdateBackFaceCheckBoxFromContext();
		break;

	case eNotify_OnChangeDesignerGlobalSettings:
		{
			CButton* pButton = (CButton*)GetDlgItem(IDC_DESIGNER_EXCLUSIVEMODE);
			if( pButton )
			{
				if( envInfo.IsEnableExclusiveMode() )
					pButton->SetCheck(BST_CHECKED);
				else
					pButton->SetCheck(BST_UNCHECKED);
			}
		}
		break;

	case eNotify_OnBeginSceneSave:
	case eNotify_OnBeginSceneOpen:
	case eNotify_OnBeginLoad:
	case eNotify_OnBeginNewScene:
		m_bExclusiveModeBeforeSave = envInfo.IsEnableExclusiveMode();
		if( envInfo.IsEnableExclusiveMode() )
			envInfo.EnableExclusiveMode(false);
		break;

	case eNotify_OnEndSceneSave:
		if( m_bExclusiveModeBeforeSave )
		{
			envInfo.EnableExclusiveMode(true);
			m_bExclusiveModeBeforeSave = false;
		}
		break;
	}
}

void MFC_BrushDesignerEditToolPanel::UpdateBackFaceFlag( CBaseObject* pBaseObject, CBaseBrush* pBrush, CBrushDesigner* pDesigner )
{
	CButton* pBackFaceCheckButton = (CButton*)GetDlgItem(IDC_DESIGNER_DISPLAY_BACKFACES);
	if( !pBackFaceCheckButton )
		return;
	if( pBackFaceCheckButton->GetCheck() == BST_CHECKED )
		pDesigner->SetModeFlag(pDesigner->GetModeFlag()|CBrushDesigner::eDesignerMode_DisplayBackFace);
	else
		pDesigner->SetModeFlag(pDesigner->GetModeFlag()&(~CBrushDesigner::eDesignerMode_DisplayBackFace));

	CBrushDesignerSmoothingGroupManager* pSmoothingGroupMgr = pDesigner->GetSmoothingGroupMgr();
	pSmoothingGroupMgr->InvalidateAll();

	pBrush->Update(pBaseObject,pDesigner);
	pBaseObject->UpdateGroup();
}

void MFC_BrushDesignerEditToolPanel::UpdateBackFaceCheckBox( CBrushDesigner* pDesigner )
{
	CButton* pBackFaceCheckButton = (CButton*)GetDlgItem(IDC_DESIGNER_DISPLAY_BACKFACES);
	if( !pBackFaceCheckButton )
		return;
	pBackFaceCheckButton->SetCheck(BST_UNCHECKED);
	if( pDesigner == NULL )
		return;
	if( pDesigner->CheckModeFlag(CBrushDesigner::eDesignerMode_DisplayBackFace) )
		pBackFaceCheckButton->SetCheck(BST_CHECKED);
}

void MFC_BrushDesignerEditToolPanel::UpdateBackFaceCheckBoxFromContext()
{
	if( m_pEditTool )
		UpdateBackFaceCheckBox(m_pEditTool->GetDesigner());
	else
	{
		std::vector<BUtil::SSelectedInfo> selection;
		m_pEditTool->GetSelectedObjectList(selection);
		if( selection.size() > 0 )
			UpdateBackFaceCheckBox(selection[0].m_pDesigner);
	}
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerDisplayBackfaces()
{
	if( m_pEditTool && m_pEditTool->GetDesigner() )
	{
		UpdateBackFaceFlag(m_pEditTool->GetBaseObject(), m_pEditTool->GetBrush(), m_pEditTool->GetDesigner());
	}
	else
	{
		std::vector<BUtil::SSelectedInfo> selections;
		m_pEditTool->GetSelectedObjectList(selections);
		for( int i = 0, iCount(selections.size()); i < iCount; ++i )
		{
			BUtil::SSelectedInfo& selection = selections[i];
			if( selection.m_pDesigner == NULL || selection.m_pBrush == NULL || selection.m_pObj == NULL )
				continue;
			UpdateBackFaceFlag(selection.m_pObj, selection.m_pBrush, selection.m_pDesigner);
		}
	}
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerExclusivemode()
{
	CButton* pButton = (CButton*)GetDlgItem(IDC_DESIGNER_EXCLUSIVEMODE);
	if( !pButton )
		return;

	SDesignerEnvironmentInfo& envInfo = CBrushDesignerEditTool::GetGlobalEnvironmentInfo();
	envInfo.EnableExclusiveMode(pButton->GetCheck());
}

void MFC_BrushDesignerEditToolPanel::OnBnClickedDesignerSeamlessedit()
{
	CButton* pButton = (CButton*)GetDlgItem(IDC_DESIGNER_SEAMLESSEDIT);
	if( !pButton )
		return;

	gSettings.bDesignerSeamlessSelection = pButton->GetCheck() == BST_CHECKED;
}

void MFC_BrushDesignerEditToolPanel::OnSize( UINT nType, int cx, int cy )
{
}

void MFC_BrushDesignerEditToolPanel::DisableButton( int nButtonID )
{
	CButton* pButton = (CButton*)GetDlgItem(nButtonID);
	if( !pButton )
		return;
	pButton->EnableWindow(FALSE);
}
