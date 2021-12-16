#include "StdAfx.h"
#include "BrushDesignerMoveTool.h"
#include "ViewManager.h"
#include "Core/BrushDesigner.h"
#include "Core/BrushRegion.h"
#include "ITransformManipulator.h"
#include "Core/BrushDesignerTestCodeSet.h"
#include "IBaseToolPanel.h"
#include "BrushDesignerEditTool.h"
#include "BrushDesignerMovePipeline.h"

static const BrushFloat kQueryEpsilon = 0.005f;

CBrushDesignerMoveTool::CBrushDesignerMoveTool( int pickFlag ) : CBrushDesignerSelectTool(pickFlag), m_Pipeline(new CBrushDesignerMovePipeline)
{
	m_bManipulatingGizmo = false;	
}

CBrushDesignerMoveTool::~CBrushDesignerMoveTool()
{
}

void CBrushDesignerMoveTool::Enter()
{
	__super::Enter();
	UpdateTMManipulatorBasedOnElements(GetEditTool()->GetSelectedElements());
	ResetDesignerRejectedEdgeList(GetMainContext());
	m_bManipulatingGizmo = false;
}

void CBrushDesignerMoveTool::Leave()
{
	__super::Leave();
}

BrushMatrix34 CBrushDesignerMoveTool::GetOffsetTMOnAlignedPlane( CViewport *pView, const BrushPlane& planeAlighedWithView, CPoint prevPos, CPoint currentPos )
{
	BrushMatrix34 offsetTM;
	offsetTM.SetIdentity();

	BrushVec3 localPrevRaySrc, localPrevRayDir;
	BUtil::GetLocalViewRay( GetBaseObject()->GetWorldTM(), pView, prevPos, localPrevRaySrc, localPrevRayDir );

	BrushVec3 localCurrentRaySrc, localCurrentRayDir;
	BUtil::GetLocalViewRay( GetBaseObject()->GetWorldTM(), pView, currentPos, localCurrentRaySrc, localCurrentRayDir );

	BrushVec3 prevHitPos;
	m_PlaneAlignedWithView.HitTest( localPrevRaySrc, localPrevRaySrc+localPrevRayDir, kDesignerEpsilon, NULL, &prevHitPos );

	BrushVec3 currentHitPos;
	m_PlaneAlignedWithView.HitTest( localCurrentRaySrc, localCurrentRaySrc+localCurrentRayDir, kDesignerEpsilon, NULL, &currentHitPos );

	offsetTM.SetTranslation(currentHitPos-prevHitPos);

	return offsetTM;
}

void CBrushDesignerMoveTool::InitializeMovementOnViewport( CViewport* pView, UINT nMouseFlags )
{
	m_PlaneAlignedWithView = BrushPlane(m_PickedPosAsLMBDown,
		m_PickedPosAsLMBDown+pView->GetViewTM().GetColumn0(),
		m_PickedPosAsLMBDown+pView->GetViewTM().GetColumn2(), kDesignerEpsilon );
	StartTransformation(nMouseFlags&MK_SHIFT);
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(1);
	CreateMirroredRegions(GetDesigner());
}

void CBrushDesignerMoveTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_bManipulatingGizmo )
		return;

#ifdef ENABLE_OUTPUT_DEBUGINFO
	if( nFlags & MK_SHIFT )
		CBrushDesignerTestCodeSet::RunAllTestCodes();
#endif

	__super::OnLButtonDown( view, nFlags, point );
	UpdateTMManipulatorBasedOnElements(GetEditTool()->GetSelectedElements());
}

void CBrushDesignerMoveTool::OnLButtonUp( CViewport *pView,UINT nFlags,CPoint point )
{
	if( m_SelectionType == eST_MoveSelection )
		EndTransformation();

	__super::OnLButtonUp(pView,nFlags,point);
}

void CBrushDesignerMoveTool::OnMouseMove( CViewport *pView,UINT nFlags,CPoint point )
{
	if( m_bManipulatingGizmo )
		return;

	if( m_SelectionType == eST_NormalSelection && !(nFlags & MK_CONTROL) )
	{		
		m_SelectionType = eST_LikelyToMoveSelection;
		m_MouseDownPos = point;
	}

	if( m_SelectionType == eST_LikelyToMoveSelection && (nFlags&MK_LBUTTON) )
	{
		if( std::abs(m_MouseDownPos.x-point.x) > 5 || std::abs(m_MouseDownPos.y-point.y) > 5 )
		{
			InitializeMovementOnViewport( pView, nFlags );
			m_SelectionType = eST_MoveSelection;
		}
	}

	if( m_SelectionType == eST_MoveSelection )
		TransformSelections(GetOffsetTMOnAlignedPlane(pView,m_PlaneAlignedWithView,m_MouseDownPos,point) );	
	else
		__super::OnMouseMove(pView,nFlags,point);

	if( (nFlags & MK_CONTROL) || m_SelectionType == eST_MoveSelection )
		UpdateTMManipulatorBasedOnElements(GetEditTool()->GetSelectedElements());
}

void CBrushDesignerMoveTool::TransformSelections( const BrushMatrix34& offsetTM )
{
	BUtil::SMainContext mc(GetMainContext());

	m_Pipeline->TransformSelections(mc,offsetTM);
	mc.pDesigner->ClearExcludedEdgesInDrawing();

	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(1);

	CreateMirroredRegions(GetDesigner());

	GetDesigner()->ResetDB(BUtil::eDBRF_ALL,1);

	m_Pipeline->SetQueryResultsFromSelectedElements(*mc.pSelected);

	if( GetIEditor()->GetEditMode() == eEditModeMove )
		UpdateTMManipulatorBasedOnElements(mc.pSelected);

	ResetDesignerRejectedEdgeList(GetMainContext());

	UpdateBrush();
	UpdateSelectionMeshFromSelectedElementList(GetMainContext());
}

void CBrushDesignerMoveTool::OnManipulatorDrag( CViewport *pView,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const BrushVec3 &value )
{
	if( !m_bManipulatingGizmo )
		return;

	TransformSelections(GetOffsetTM(pManipulator,value));
}

void CBrushDesignerMoveTool::StartTransformation( bool bSeparate )
{
	GetIEditor()->BeginUndo();
	GetEditTool()->StoreSelectionUndo();
	GetDesigner()->RecordUndo("Move",GetBaseObject());

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	m_SelectedElementNormal = pSelected->GetNormal(GetDesigner());
	m_Pipeline->SetDesigner(GetDesigner());
	if( bSeparate )
		m_Pipeline->InitializeIndependently(*pSelected);
	else
		m_Pipeline->Initialize(*pSelected);
}

void CBrushDesignerMoveTool::EndTransformation()
{
	GetIEditor()->AcceptUndo("Designer Move");
	if( gSettings.bDesignerKeepCenterPivot )
		GetBrush()->PivotToCenter(GetBaseObject(),GetDesigner());
	m_Pipeline->End();
	UpdateBrush();
	Sync();
}

void CBrushDesignerMoveTool::OnManipulatorMouseEvent( CViewport *pView, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( pSelected->IsEmpty() )
		return;

	m_bHitGizmo = bHitGizmo;
	UpdateCursor(pView,bHitGizmo);

	if( event == eMouseLDown )
	{
		if( !m_bManipulatingGizmo )
		{
			StartTransformation(flags & MK_SHIFT);
			m_bManipulatingGizmo = true;
		}
	}
	else if( event == eMouseLUp )
	{
		if( m_bManipulatingGizmo )
		{
			EndTransformation();
			m_bManipulatingGizmo = false;
		}
	}
}

void CBrushDesignerMoveTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if( GetBaseObject() == NULL )
		return;
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( pSelected->QueryFromElements(GetDesigner()).empty() )
		return;
	__super::OnEditorNotifyEvent(event);
	switch(event)
	{
	case eNotify_OnEndUndoRedo:
		{
			UpdateTMManipulatorBasedOnElements(pSelected);
			m_Pipeline->SetDesigner(GetDesigner());
			m_Pipeline->SetQueryResultsFromSelectedElements(*pSelected);
		}
		break;
	}
}

void CBrushDesignerMoveTool::Transform( BUtil::SMainContext& mc, const BrushMatrix34& tm, bool bMoveTogether )
{	
	CBrushDesignerMovePipeline pipeline;

	pipeline.SetDesigner(mc.pDesigner);

	if( bMoveTogether )
		pipeline.Initialize(*mc.pSelected);
	else
		pipeline.InitializeIndependently(*mc.pSelected);

	pipeline.TransformSelections(mc,tm);

	DESIGNER_SHELF_RECONSTRUCTOR(mc.pDesigner);
	mc.pDesigner->SetShelf(1);
	mc.pDesigner->ResetDB(BUtil::eDBRF_ALL,1);

	pipeline.End();
}