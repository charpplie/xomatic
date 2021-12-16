#include "StdAfx.h"
#include <InitGuid.h>
#include "BrushDesignerEditTool.h"
#include "ViewManager.h"
#include "Grid.h"
#include "Objects/DesignerBrushObject.h"
#include "Core/BaseBrush.h"
#include "IBaseToolPanel.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerBaseTool.h"
#include "BrushDesignerMoveTool.h"
#include "BrushDesignerExtrudeTool.h"
#include "BrushDesignerTextureMappingTool.h"
#include "BrushDesignerStairTool.h"
#include "BrushDesignerStairProfileTool.h"
#include "BrushDesignerRemoveTool.h"
#include "BrushDesignerFlipTool.h"
#include "BrushDesignerOffsetTool.h"
#include "BrushDesignerFillSpaceTool.h"
#include "BrushDesignerDrawLineTool.h"
#include "BrushDesignerDrawRectangleTool.h"
#include "BrushDesignerDrawCurveTool.h"
#include "BrushDesignerDrawDiscTool.h"
#include "BrushDesignerCreateSphereTool.h"
#include "BrushDesignerCloneTool.h"
#include "BrushDesignerSelectTool.h"
#include "BrushDesignerSliceTool.h"
#include "BrushDesignerBevelTool.h"
#include "BrushDesignerMirrorTool.h"
#include "BrushDesignerDebuggerTool.h"
#include "BrushDesignerCreateBoxTool.h"
#include "BrushDesignerCreateConeTool.h"
#include "BrushDesignerCreateCylinderTool.h"
#include "BrushDesignerDrawRectangleTool.h"
#include "BrushDesignerObjectModeTool.h"
#include "BrushDesignerPivotTool.h"
#include "BrushDesignerPivot2BottomTool.h"
#include "BrushDesignerResetXFormTool.h"
#include "BrushDesignerBooleanTool.h"
#include "BrushDesignerMergeTool.h"
#include "BrushDesignerExportTool.h"
#include "BrushDesignerSeparateTool.h"
#include "BrushDesignerCopyTool.h"
#include "BrushDesignerSelectAllNoneTool.h"
#include "BrushDesignerSelectConnectedTool.h"
#include "BrushDesignerSelectGrowTool.h"
#include "BrushDesignerInvertSelectionTool.h"
#include "BrushDesignerLoopSelectionTool.h"
#include "BrushDesignerRingSelectionTool.h"
#include "BrushDesignerLatheTool.h"
#include "BrushDesignerWeldTool.h"
#include "BrushDesignerSnapToGrid.h"
#include "BrushDesignerMagnetTool.h"
#include "BrushDesignerSmoothingGroupTool.h"
#include "BrushDesignerRemoveDoubles.h"
#include "BrushDesignerHideFace.h"
#include "BrushDesignerCubeEditor.h"
#include "Core/BrushDesignerUndo.h"
#include "BrushDesignerSubdivisionTool.h"
#include "Core/BrushCommonInterface.h"
#include "DisplaySettings.h"
#include "Objects/ObjectLayer.h"
#include "Objects/ObjectLayerManager.h"
#include "Core/BrushDesignerElementManager.h"

namespace
{
	std::map<int,int> s_DesignerMode2EditButtonIdMap;
}

BUtil::EDesignerMode CBrushDesignerEditTool::m_DesignerMode = BUtil::eDesigner_Max;
BUtil::EDesignerMode CBrushDesignerEditTool::m_PreviousDesignerMode = BUtil::eDesigner_Max;
BUtil::EDesignerMode CBrushDesignerEditTool::m_PreviousSelectMode = BUtil::eDesigner_Max;
SDesignerEnvironmentInfo CBrushDesignerEditTool::m_DesignerGlobalSetting;

IMPLEMENT_DYNCREATE(CBrushDesignerEditTool,CEditTool)

CBrushDesignerEditTool::CBrushDesignerEditTool() : m_pSelectedElements(new CBrushDesignerElementManager)
{
	m_pBaseObject = NULL;
	m_PreviousSelectMode = BUtil::eDesigner_Select_Face;
}

CBrushDesignerEditTool::~CBrushDesignerEditTool()
{
	ReleaseEventHandlers();
}

void CBrushDesignerEditTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	CBrushDesignerElementManager* pSelected = GetSelectedElements();
	switch(event)
	{
	case eNotify_OnBeginGameMode:
		m_DesignerGlobalSetting.EnableExclusiveMode(false);
		pSelected->Clear();
		break;
	case eNotify_OnEndUndoRedo:
		if( !BUtil::IsSelectElementMode(m_DesignerMode) )
		{
			pSelected->Clear();
			if( GetCurrentTool() )
				GetCurrentTool()->ClearRegionSelections();
		}
		break;
	}

	if( GetCurrentTool() )
		GetCurrentTool()->OnEditorNotifyEvent(event);

	if( GetDesignerToolPanel() )
		GetDesignerToolPanel()->OnEditorNotifyEvent(event);
}

void CBrushDesignerEditTool::InitializeEventHandlers()
{
	if( !m_ToolMap.empty() )
		return;

	m_ToolMap[BUtil::eDesigner_Select_Vertex] = new CBrushDesignerMoveTool(BUtil::ePF_Vertex);
	m_ToolMap[BUtil::eDesigner_Select_Edge] = new CBrushDesignerMoveTool(BUtil::ePF_Edge);
	m_ToolMap[BUtil::eDesigner_Select_Face] = new CBrushDesignerMoveTool(BUtil::ePF_Face);
	m_ToolMap[BUtil::eDesigner_Select_VertexEdge] = new CBrushDesignerMoveTool(BUtil::ePF_Vertex|BUtil::ePF_Edge);
	m_ToolMap[BUtil::eDesigner_Select_VertexFace] = new CBrushDesignerMoveTool(BUtil::ePF_Vertex|BUtil::ePF_Face);
	m_ToolMap[BUtil::eDesigner_Select_EdgeFace] = new CBrushDesignerMoveTool(BUtil::ePF_Edge|BUtil::ePF_Face);
	m_ToolMap[BUtil::eDesigner_Select_VertexEdgeFace] = new CBrushDesignerMoveTool(BUtil::ePF_All);
	m_ToolMap[BUtil::eDesigner_Magnet] = new CBrushDesignerMagnetTool;
	m_ToolMap[BUtil::eDesigner_Select_AllNone] = new CBrushDesignerSelectAllNoneTool;
	m_ToolMap[BUtil::eDesigner_Select_Connected] = new CBrushDesignerSelectConnectedTool;
	m_ToolMap[BUtil::eDesigner_Select_Grow] = new CBrushDesignerSelectGrowTool;
	m_ToolMap[BUtil::eDesigner_Select_Loop] = new CBrushDesignerLoopSelectionTool;
	m_ToolMap[BUtil::eDesigner_Select_Ring] = new CBrushDesignerRingSelectionTool;
	m_ToolMap[BUtil::eDesigner_Select_Invert] = new CBrushDesignerInvertSelectionTool;
	m_ToolMap[BUtil::eDesigner_Box] = new CBrushDesignerCreateBoxTool;
	m_ToolMap[BUtil::eDesigner_Sphere] = new CBrushDesignerCreateSphereTool;
	m_ToolMap[BUtil::eDesigner_Cylinder] = new CBrushDesignerCreateCylinderTool;
	m_ToolMap[BUtil::eDesigner_Cone] = new CBrushDesignerCreateConeTool;
	m_ToolMap[BUtil::eDesigner_Rectangle] = new CBrushDesignerDrawRectangleTool;
	m_ToolMap[BUtil::eDesigner_Disc] = new CBrushDesignerDrawDiscTool;
	m_ToolMap[BUtil::eDesigner_Line] = new CBrushDesignerDrawLineTool;
	m_ToolMap[BUtil::eDesigner_Curve] = new CBrushDesignerDrawCurveTool;
	m_ToolMap[BUtil::eDesigner_Weld] = new CBrushDesignerWeldTool;
	m_ToolMap[BUtil::eDesigner_Slice] = new CBrushDesignerSliceTool;
	m_ToolMap[BUtil::eDesigner_Remove] = new CBrushDesignerRemoveTool;
	m_ToolMap[BUtil::eDesigner_Fill] = new CBrushDesignerFillSpaceTool;
	m_ToolMap[BUtil::eDesigner_Extrude] = new CBrushDesignerExtrudeTool;
	m_ToolMap[BUtil::eDesigner_Offset] = new CBrushDesignerOffsetTool;
	m_ToolMap[BUtil::eDesigner_Seprate] = new CBrushDesignerSeparateTool;
	m_ToolMap[BUtil::eDesigner_Merge] = new CBrushDesignerMergeTool;
	m_ToolMap[BUtil::eDesigner_Copy] = new CBrushDesignerCopyTool;
	m_ToolMap[BUtil::eDesigner_Flip] = new CBrushDesignerFlipTool;
	m_ToolMap[BUtil::eDesigner_Bevel] = new CBrushDesignerBevelTool;
	m_ToolMap[BUtil::eDesigner_Stair] = new CBrushDesignerStairTool;
	m_ToolMap[BUtil::eDesigner_StairProfile] = new CBrushDesignerStairProfileTool;
	m_ToolMap[BUtil::eDesigner_Array] = new CBrushDesignerCloneTool(BUtil::eArrangeType_Array);
	m_ToolMap[BUtil::eDesigner_Clone] = new CBrushDesignerCloneTool(BUtil::eArrangeType_Circle);
	m_ToolMap[BUtil::eDesigner_Mirror] = new CBrushDesignerMirrorTool;
	m_ToolMap[BUtil::eDesigner_Lathe] = new CBrushDesignerLatheTool;
	m_ToolMap[BUtil::eDesigner_Mapping] = new CBrushDesignerTextureMappingTool;
	m_ToolMap[BUtil::eDesigner_Debugger] = new CBrushDesignerDebuggerTool;
	m_ToolMap[BUtil::eDesigner_ResetXForm] = new CBrushDesignerResetXFormTool;
	m_ToolMap[BUtil::eDesigner_Export] = new CBrushDesignerExportTool;
	m_ToolMap[BUtil::eDesigner_Pivot] = new CBrushDesignerPivotTool;
	m_ToolMap[BUtil::eDesigner_Pivot2Bottom] = new CBrushDesignerPivot2BottomTool;
	m_ToolMap[BUtil::eDesigner_Boolean] = new CBrushDesignerBooleanTool;
	m_ToolMap[BUtil::eDesigner_ObjectMode] = new CBrushDesignerObjectModeTool;
	m_ToolMap[BUtil::eDesigner_SnapToGrid] = new CBrushDesignerSnapToGridTool;
	m_ToolMap[BUtil::eDesigner_SmoothingGroup] = new CBrushDesignerSmoothingGroupTool;
	m_ToolMap[BUtil::eDesigner_HideFace] = new CBrushDesignerHideFaceTool;
	m_ToolMap[BUtil::eDesigner_RemoveDoubles] = new CBrushDesignerRemoveDoublesTool;
	m_ToolMap[BUtil::eDesigner_CubeEditor] = new CBrushDesignerCubeEditor;
	m_ToolMap[BUtil::eDesigner_Subdivision] = new CBrushDesignerSubdivisionTool;
}

void CBrushDesignerEditTool::ReleaseEventHandlers()
{
	m_ToolMap.clear();
}

bool CBrushDesignerEditTool::Activate( CEditTool *pPreviousTool )
{
	// Remember selection here.
	CSelectionGroup *pSel = GetIEditor()->GetSelection();
	if (pSel->IsEmpty())
		return false;

	bool bConsistOfOnlyDesigner = true;
	for( int i = 0, iCount(pSel->GetCount()); i < iCount; ++i )
	{
		if( !pSel->GetObject(i)->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
		{
			bConsistOfOnlyDesigner = false;
			break;
		}
	}
	if( !bConsistOfOnlyDesigner )
		GetIEditor()->GetObjectManager()->EndEditParams();

	InitializeEventHandlers();

	return true;
}

void CBrushDesignerEditTool::BeginEditParams( IEditor *ie,int flags )
{
	CBrushDesignerElementManager* pSelected = GetSelectedElements();
	pSelected->Clear();
	CreateMappingTableBetweenToolIdAndButtonId();

	if( !CDesignerBrushObject::GetMenuPanel() )
	{
		CDesignerBrushObject::CreateDesignerPanels((CDesignerBrushObject*)GetBaseObject(),false);
		CDesignerBrushObject::GetMenuPanel()->SetEditTool(this);
	}
}

void CBrushDesignerEditTool::EndEditParams()
{
	if( GetCurrentTool() )
		GetCurrentTool()->Leave();

	ReleaseEventHandlers();
	if( s_pDesignerObj )
		GetIEditor()->GetObjectManager()->EndEditParams();

	if( CDesignerBrushObject::GetMenuPanel() )
		CDesignerBrushObject::DestroyDesignerPanels();

	GetIEditor()->ShowTransformManipulator(false);
}

void CBrushDesignerEditTool::SetBaseObject( CBaseObject* pBaseObject )
{
	m_pBaseObject = pBaseObject;
// 	if( s_pDesignerObj && s_pDesignerObj->GetMenuPanel() )
// 		s_pDesignerObj->GetMenuPanel()->UpdateCloneArrayButtons();
}

void CBrushDesignerEditTool::CreateMappingTableBetweenToolIdAndButtonId()
{
	if( !s_DesignerMode2EditButtonIdMap.empty() )
		return;

	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Select_Vertex] = IDC_DESIGNER_SELECT_VERTEX;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Select_Edge] = IDC_DESIGNER_SELECT_EDGE;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Select_Face] = IDC_DESIGNER_SELECT_FACE;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Select_AllNone] = IDC_DESIGNER_SELECT_ALLNONE;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Select_Connected] = IDC_DESIGNER_SELECT_CONNECTED;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Select_Grow] = IDC_DESIGNER_SELECT_GROW;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Select_Loop] = IDC_DESIGNER_SELECT_LOOP;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Select_Ring] = IDC_DESIGNER_SELECT_RING;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Select_Invert] = IDC_DESIGNER_SELECT_INVERT;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Magnet] = IDC_DESIGNER_MAGNETTOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Box] = IDC_DESIGNER_PRIMITIVE_BOX;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Sphere] = IDC_DESIGNER_PRIMITIVE_SPHERE;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Cylinder] = IDC_DESIGNER_PRIMITIVE_CYLINDER;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Cone] = IDC_DESIGNER_PRIMITIVE_CONE;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Rectangle] = IDC_DESIGNER_PRIMITIVE_RECTANGLE;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Disc] = IDC_DESIGNER_PRIMITIVE_DISC;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Line] = IDC_DESIGNER_DRAWING_LINE;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Curve] = IDC_DESIGNER_DRAWING_CURVE;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Weld] = IDC_DESIGNER_WELDTOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Slice] = IDC_DESIGNER_SLICETOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Remove] = IDC_DESIGNER_REMOVETOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Fill] = IDC_DESIGNER_FILLTOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Extrude] = IDC_DESIGNER_EXTRUDETOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Offset] = IDC_DESIGNER_OFFSETTOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Seprate] = IDC_DESIGNER_SEPARATETOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Merge] = IDC_DESIGNER_MERGETOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Copy] = IDC_DESIGNER_COPYTOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Flip] = IDC_DESIGNER_FLIPTOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Bevel] = IDC_DESIGNER_BEVELTOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Stair] = IDC_DESIGNER_STAIRTOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_StairProfile] = IDC_DESIGNER_STAIRPROFILETOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Array] = IDC_DESIGNER_ARRAYCLONETOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Clone] = IDC_DESIGNER_CLONETOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Mirror] = IDC_DESIGNER_MIRRORTOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Lathe] = IDC_DESIGNER_LATHETOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Mapping] = IDC_DESIGNER_MAPPINGTOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Debugger] = IDC_DESIGNER_DEBUGGERTOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_ResetXForm] = IDC_DESIGNER_RESETXFORM;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Export] = IDC_DESIGNER_EXPORTTOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Pivot] = IDC_DESIGNER_SELECT_PIVOT;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Pivot2Bottom] = IDC_DESIGNER_PIVOT2BOTTOM;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Boolean] = IDC_DESIGNER_BOOLEANTOOL;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_ObjectMode] = IDC_DESIGNER_OBJECTMODE;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_SnapToGrid] = IDC_DESIGNER_SNAPTOGRID;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_SmoothingGroup] = IDC_DESIGNER_SMOOTHINGGROUP;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_HideFace] = IDC_DESIGNER_HIDEFACE;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_RemoveDoubles] = IDC_DESIGNER_REMOVEDOUBLES;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_CubeEditor] = IDC_DESIGNER_CUBEEDITOR;
	s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Subdivision] = IDC_DESIGNER_SUBDIVISION;
}

void CBrushDesignerEditTool::Display( DisplayContext &dc )
{
	if( !GetDesigner() || !GetBaseObject() )
		return;

	dc.SetDrawInFrontMode(true);

	dc.PushMatrix(GetBaseObject()->GetWorldTM());

	if( gSettings.bDesignerHighlightElements && BUtil::IsEdgeSelectMode(m_DesignerMode) )
		GetDesigner()->Display(dc,BUtil::kElementEdgeThickness,BUtil::kElementBoxColor);
	else
		GetDesigner()->Display(dc);

	if( m_DesignerMode < BUtil::eDesigner_Max && m_ToolMap[m_DesignerMode] )
		m_ToolMap[m_DesignerMode]->Display(dc);

	if( gSettings.bDesignerDisplayTriangulation )
		GetBrush()->DisplayTriangulation(GetBaseObject(),GetDesigner(),dc);

	dc.PopMatrix();

	dc.SetDrawInFrontMode(false);
}

std::set<CDesignerBrushObject*> CBrushDesignerEditTool::GetSelectedDesignerObjects() const
{
	std::set<CDesignerBrushObject*> designerObjectSet;
	CSelectionGroup* pSelection = GetIEditor()->GetSelection();
	for( int i = 0, iSelectionCount(pSelection->GetCount()); i < iSelectionCount; ++i )
	{
		CBaseObject* pObj = pSelection->GetObject(i);
		if( !pObj->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
			continue;
		designerObjectSet.insert((CDesignerBrushObject*)pObj);
	}
	return designerObjectSet;
}

bool CBrushDesignerEditTool::MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags )
{
	if( m_DesignerMode >= BUtil::eDesigner_Max )
		return false;

	CBrushDesignerBaseTool* pHandler = m_ToolMap[m_DesignerMode];
	if( pHandler == NULL )
		return false;

	if( GetDesigner() && GetCurrentTool() )
	{
		if( BUtil::IsCreationTool(m_DesignerMode) && GetDesigner()->IsEmpty() && GetCurrentTool()->IsPhaseFirstStepOnPrimitiveCreation() )
		{
			GetIEditor()->SuspendUndo();
			GetBaseObject()->SetWorldPos(view->MapViewToCP(point));
			GetIEditor()->ResumeUndo();
		}
		UpdateStatusText();
	}

	if( m_DesignerMode != BUtil::eDesigner_ObjectMode && !GetDesigner() )
		return true;

	if (event == eMouseLDown)
	{
		view->SetCapture();
		pHandler->OnLButtonDown( view,flags,point );
	}
	else if (event == eMouseLUp)
	{
		pHandler->OnLButtonUp( view,flags,point );
		ReleaseCapture();
	}
	else if (event == eMouseMove)
	{
		CBrushDesignerElementManager* pSelected = GetSelectedElements();
		std::set<CDesignerBrushObject*> selectedDesignerObjects = GetSelectedDesignerObjects();		
		if( flags == 0 && pHandler->EnabledSeamlessSelection() && (gSettings.bDesignerSeamlessSelection || selectedDesignerObjects.size() > 1) )
		{
			if( pSelected->IsEmpty() )
				pHandler->SelectDesignerObject(point);
		}
		pHandler->OnMouseMove( view,flags,point );
	}
	else if (event == eMouseLDblClick)
	{
		pHandler->OnLButtonDblClk( view,flags,point );
	}
	else if( event == eMouseRDown )
	{
		pHandler->OnRButtonDown(view,flags,point);
	}
	else if( event == eMouseRUp )
	{
		pHandler->OnRButtonUp(view,flags,point);
	}
	else if( event == eMouseMDown )
	{
		pHandler->OnMButtonDown(view,flags,point);
	}
	else if( event == eMouseWheel )
	{
		pHandler->OnMouseWheel(view,flags,point);
	}
	return true;
}

bool CBrushDesignerEditTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if( m_DesignerMode >= BUtil::eDesigner_Max )
		return false;

	if( m_DesignerMode == BUtil::eDesigner_ObjectMode && nChar == VK_DELETE )
	{
		GetIEditor()->SetEditTool(NULL);
		return false;
	}

	CBrushDesignerBaseTool* pHandler = m_ToolMap[m_DesignerMode];
	if( pHandler == NULL )
		return false;

	bool bContinueEdit = pHandler->OnKeyDown(view,nChar,nRepCnt,nFlags);
	if( !bContinueEdit )
	{
		if( GetCurrentTool() )
			GetCurrentTool()->ReleaseObjectGizmo();
	}

	return bContinueEdit;
}

void CBrushDesignerEditTool::SetCheckButton( BUtil::EDesignerMode designerMode, int nCheck )
{
	IBrushDesignerEditToolPanel* pMenuPanel = GetDesignerToolPanel();
	if( !pMenuPanel )
		return;

	if( BUtil::IsSelectElementMode(designerMode) )
	{		
		int nDesignerMode = (int)designerMode;
		if( nDesignerMode & BUtil::eDesigner_Select_Vertex )
			pMenuPanel->SetButtonCheck(s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Select_Vertex],nCheck);
		if( nDesignerMode & BUtil::eDesigner_Select_Edge )
			pMenuPanel->SetButtonCheck(s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Select_Edge],nCheck);			
		if( nDesignerMode & BUtil::eDesigner_Select_Face )
			pMenuPanel->SetButtonCheck(s_DesignerMode2EditButtonIdMap[BUtil::eDesigner_Select_Face],nCheck);			
	}
	else if( s_DesignerMode2EditButtonIdMap.find(designerMode) != s_DesignerMode2EditButtonIdMap.end() )
	{
		pMenuPanel->SetButtonCheck(s_DesignerMode2EditButtonIdMap[designerMode],nCheck);
	}
}

bool CBrushDesignerEditTool::SetDesignerModeToSelectElements( BUtil::EDesignerMode designerMode )
{
	if( !BUtil::IsSelectElementMode(designerMode) )
		return false;

	int nCombinationDesignerMode = m_DesignerMode;

	bool bPressedCtrl = CheckVirtualKey(VK_CONTROL);
	if( bPressedCtrl )
	{
		CBrushDesignerSelectTool* pTool = (CBrushDesignerSelectTool*)GetCurrentTool();
		if( pTool && pTool->CheckPickFlag(BUtil::ePF_Vertex) && designerMode == BUtil::eDesigner_Select_Vertex )
		{
			SetCheckButton(BUtil::eDesigner_Select_Vertex,BST_UNCHECKED);
			nCombinationDesignerMode &= ~(BUtil::ePF_Vertex);
		}
		else if( pTool && pTool->CheckPickFlag(BUtil::ePF_Edge) && designerMode == BUtil::eDesigner_Select_Edge )
		{
			SetCheckButton(BUtil::eDesigner_Select_Edge,BST_UNCHECKED);
			nCombinationDesignerMode &= ~(BUtil::ePF_Edge);
		}
		else if( pTool && pTool->CheckPickFlag(BUtil::ePF_Face) && designerMode == BUtil::eDesigner_Select_Face )
		{
			SetCheckButton(BUtil::eDesigner_Select_Face,BST_UNCHECKED);
			nCombinationDesignerMode &= ~(BUtil::ePF_Face);
		}
		else if( BUtil::IsSelectElementMode(m_DesignerMode) )
		{
			nCombinationDesignerMode |= designerMode;
		}
		else
		{
			nCombinationDesignerMode = designerMode;
		}
	}
	else
	{
		nCombinationDesignerMode = designerMode;
	}

	if( !BUtil::IsSelectElementMode((BUtil::EDesignerMode)nCombinationDesignerMode) )
		return false;

	TOOLDESIGNER_MAP::iterator iTool = m_ToolMap.find((BUtil::EDesignerMode)nCombinationDesignerMode);
	if( iTool == m_ToolMap.end() )
		return false;

	CBrushDesignerSelectTool* pNextTool = (CBrushDesignerSelectTool*)&*(iTool->second);

	if( !bPressedCtrl )
	{
		SetCheckButton(BUtil::eDesigner_Select_Vertex,BST_UNCHECKED);
		SetCheckButton(BUtil::eDesigner_Select_Edge,BST_UNCHECKED);
		SetCheckButton(BUtil::eDesigner_Select_Face,BST_UNCHECKED);
	}

	SetCheckButton(m_DesignerMode,BST_UNCHECKED);
	if( GetCurrentTool() )
		GetCurrentTool()->Leave();
	m_PreviousDesignerMode = BUtil::IsSelectElementMode(m_DesignerMode) ? m_PreviousDesignerMode : m_DesignerMode;
	m_DesignerMode = (BUtil::EDesignerMode)nCombinationDesignerMode;
	pNextTool->Enter();

	SetCheckButton((BUtil::EDesignerMode)nCombinationDesignerMode,BST_CHECKED);
	
	GetIEditor()->GetActiveView()->SetFocus();
	GetIEditor()->GetActiveView()->Invalidate();

	m_PreviousSelectMode = m_DesignerMode;

	return true;
}

void CBrushDesignerEditTool::SetDesignerMode( BUtil::EDesignerMode designerMode, bool bForceChange )
{
	if( m_pBaseObject == NULL )
		return;

	if( SetDesignerModeToSelectElements(designerMode) )
		return;

	if( m_DesignerMode == designerMode && !bForceChange )
	{
		SetCheckButton(designerMode,BST_CHECKED);
		return;
	}

	if( GetCurrentTool() )
		GetCurrentTool()->Leave();

	if( BUtil::IsSelectElementMode(m_DesignerMode) )
	{
		SetCheckButton(BUtil::eDesigner_Select_Vertex,BST_UNCHECKED);
		SetCheckButton(BUtil::eDesigner_Select_Edge,BST_UNCHECKED);
		SetCheckButton(BUtil::eDesigner_Select_Face,BST_UNCHECKED);
	}

	SetCheckButton(m_DesignerMode,BST_UNCHECKED);
	SetCheckButton(designerMode,BST_CHECKED);

	m_PreviousDesignerMode = m_DesignerMode;
	m_DesignerMode = designerMode;

	CBrushDesignerBaseTool* pHandler = GetCurrentTool();
	if( pHandler == NULL )
		return;

	pHandler->Enter();

	GetIEditor()->GetActiveView()->SetFocus();
	GetIEditor()->GetActiveView()->Invalidate();
}

void CBrushDesignerEditTool::OnManipulatorDrag( CViewport *view,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const Vec3 &value )
{
	CBrushDesignerBaseTool* pHandler = GetCurrentTool();
	if( pHandler == NULL )
		return;

	pHandler->OnManipulatorDrag(view,pManipulator,p0,p0,value);
}

void CBrushDesignerEditTool::OnManipulatorMouseEvent( CViewport *view, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo )
{
	CBrushDesignerBaseTool* pHandler = GetCurrentTool();
	if( pHandler == NULL )
		return;

	pHandler->OnManipulatorMouseEvent(view,pManipulator,event,point,flags,bHitGizmo);
}

CBrushDesignerBaseTool* CBrushDesignerEditTool::GetCurrentTool() const
{
	if( m_DesignerMode >= BUtil::eDesigner_Max )
		return NULL;

	CBrushDesignerBaseTool* pHandler = GetTool(m_DesignerMode);
	if( pHandler == NULL )
		return NULL;

	return pHandler;
}

const GUID BRUSHDESIGNER_TOOL_GUID = { 0x982FC59C, 0xC4CF, 0x11E0, { 0x9C, 0xDE,  0x60,  0x73,  0x48,  0x24,  0x01,  0x9B } };

class CBrushDesignerEditTool_ClassDesc : public CRefCountClassDesc
{
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_EDITTOOL; }
	virtual REFGUID ClassID() { return BRUSHDESIGNER_TOOL_GUID; }
	virtual const char* ClassName() { return "EditTool.BrushDesignerTool"; };
	virtual const char* Category() { return "Brush"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CBrushDesignerEditTool); }
};

void CBrushDesignerEditTool::RegisterTool( CRegistrationContext &rc )
{
	rc.pClassFactory->RegisterClass( new CBrushDesignerEditTool_ClassDesc );
}

void CBrushDesignerEditTool::LeaveCurrentTool()
{
	if( GetCurrentTool() )
		GetCurrentTool()->Leave();
}

void CBrushDesignerEditTool::EnterCurrentTool()
{
	if( GetCurrentTool() )
		GetCurrentTool()->Enter();
}

void CBrushDesignerEditTool::SelectAllElements()
{
	if( BUtil::IsSelectElementMode(GetDesignerMode()) )
		((CBrushDesignerSelectTool*)GetTool(GetDesignerMode()))->SelectAllElements();
}

CBrushDesigner* CBrushDesignerEditTool::GetDesigner() const
{
	if( m_pBaseObject == NULL )
		return NULL;

	CBrushDesigner* pDesigner;
	if(CBrushCommonInterface::GetDesigner(m_pBaseObject, pDesigner))
		return pDesigner;

	DESIGNER_ASSERT(0);
	return NULL;
}

CBaseBrush* CBrushDesignerEditTool::GetBrush() const
{
	DESIGNER_ASSERT(m_pBaseObject);
	if( m_pBaseObject == NULL )
		return NULL;

	CBaseBrush* pBrush;
	if(CBrushCommonInterface::GetBrush(m_pBaseObject, pBrush))
		return pBrush;

	DESIGNER_ASSERT(0);
	return NULL;
}

void CBrushDesignerEditTool::SetSubMatID( int nSubMatID, CBrushDesigner* pDesigner )
{
	if( CUndo::IsRecording() && GetBaseObject() )
		CUndo::Record( new CUndoDesignerTextureMapping(GetBaseObject(), "Designer : SetSubMatID") );

	CBrushDesignerElementManager* pSelected = GetSelectedElements();
	if( pSelected->IsEmpty() )
	{
		for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
		{
			CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
			if( pRegion == NULL )
				continue;
			pRegion->SetMaterialID(nSubMatID);
		}
	}
	else
	{
		CBrushDesignerTextureMappingTool::AssignMatID(nSubMatID);
	}

	if( GetCurrentTool() )
		GetCurrentTool()->SetSubMatID(nSubMatID,pDesigner);
}

void CBrushDesignerEditTool::MaterialChanged()
{
	if( m_DesignerMode == BUtil::eDesigner_CubeEditor )
		m_ToolMap[BUtil::eDesigner_CubeEditor]->MaterialChanged();
}

bool CBrushDesignerEditTool::IsDisplayGrid()
{
	if( m_DesignerMode == BUtil::eDesigner_ObjectMode )
		return true;
	return false;
}

bool CBrushDesignerEditTool::IsUpdateUIPanel()
{
	if( m_PreviousDesignerMode == BUtil::eDesigner_Clone || m_PreviousDesignerMode == BUtil::eDesigner_Array || 
		m_PreviousDesignerMode == BUtil::eDesigner_Merge || m_PreviousDesignerMode == BUtil::eDesigner_Boolean )
	{
		return true;
	}
	return false;
}

bool CBrushDesignerEditTool::IsCircleTypeRotateGizmo()
{
	if( GetCurrentTool() )
		return GetCurrentTool()->IsCircleTypeRotateGizmo();
	return false;
}

void CBrushDesignerEditTool::SetUserData( const char *key,void *userData )
{
	Matrix34 objectTM;
	objectTM.SetIdentity();

	CBaseObject* pObject = GetIEditor()->NewObject("Designer");
	if(pObject)
	{
		pObject->SetLocalTM(objectTM);

		int hideMask = GetIEditor()->GetDisplaySettings()->GetObjectHideMask();
		hideMask = hideMask & ~(pObject->GetType());
		GetIEditor()->GetDisplaySettings()->SetObjectHideMask( hideMask );

		CObjectLayer *pLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
		pLayer->SetFrozen(false);
		pLayer->SetVisible(true);
		pLayer->SetModified();

		GetIEditor()->GetObjectManager()->BeginEditParams(pObject,OBJECT_CREATE);

		GetIEditor()->SetModifiedFlag();
		GetIEditor()->SetModifiedModule(eModifiedBrushes);

		SetBaseObject(pObject);
		CBrushDesignerEditTool::SetPanelOwner((CDesignerBrushObject*)pObject);

		InitializeEventHandlers();
	}
}

void CBrushDesignerEditTool::Finish()
{
	if( m_pBaseObject && m_pBaseObject->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
	{
		CBrushDesigner* pDesigner = ((CDesignerBrushObject*)&*m_pBaseObject)->GetDesigner();

		bool bEmptyShelf0 = pDesigner->IsEmpty(0);
		bool bEmptyShelf1 = pDesigner->IsEmpty(1);

		if( bEmptyShelf0 && bEmptyShelf1 )
		{
			GetIEditor()->SuspendUndo();
			GetIEditor()->DeleteObject(m_pBaseObject);
			GetIEditor()->ResumeUndo();
		}

		if( GetCurrentTool() )
		{
			GetCurrentTool()->Leave();
			DeleteObjectIfEmpty();
			m_DesignerMode = BUtil::eDesigner_Max;
		}
	}
}

int CBrushDesignerEditTool::GetPanelIndex() const
{
	if( !GetPanelOwner() )
		return -1;
	return GetPanelOwner()->GetMenuPanelIndex();
}

void CBrushDesignerEditTool::DeleteObjectIfEmpty()
{
	if( GetDesigner()->IsEmpty() )
	{
		GetIEditor()->SuspendUndo();
		GetIEditor()->GetObjectManager()->DeleteObject(GetBaseObject());
		GetIEditor()->ResumeUndo();
	}
}

void CBrushDesignerEditTool::UpdateStatusText()
{
	if( GetCurrentTool() == NULL )
		return;
	SetStatusText(GetCurrentTool()->GetStatusText());
}

void CBrushDesignerEditTool::StoreSelectionUndo()
{
	if( CUndo::IsRecording() )
		CUndo::Record( new CUndoDesigneSelection( *GetSelectedElements(), GetBaseObject() ) );
}

void CBrushDesignerEditTool::GetAffectedObjects( DynArray<CBaseObject*>& outAffectedObjects )
{
	if( GetBaseObject() )
		outAffectedObjects.push_back(GetBaseObject());
}

void CBrushDesignerEditTool::GetSelectedObjectList( std::vector<BUtil::SSelectedInfo>& selections ) const
{
	CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
	if( pSelection == NULL )
		return;	

	for( int i = 0, iCount(pSelection->GetCount()); i < iCount; ++i )
	{
		BUtil::SSelectedInfo selectedInfo;
		CBaseObject* pObj = pSelection->GetObject(i);
		if( pObj == NULL )
			continue;

		selectedInfo.m_pObj = pObj;

		CBrushCommonInterface::GetBrush(pObj,selectedInfo.m_pBrush);
		CBrushCommonInterface::GetDesigner(pObj,selectedInfo.m_pDesigner);

		selections.push_back(selectedInfo);
	}
}