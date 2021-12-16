#include "StdAfx.h"
#include "BrushDesignerPivotTool.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerEditTool.h"
#include "ViewManager.h"
#include "IBaseToolPanel.h"
#include "Core/BrushDesignerDB.h"

namespace
{
	IBaseToolPanel* g_pPivotToolPanel = NULL;
}

void CBrushDesignerPivotTool::Enter()
{
	m_nSelectedCandidate = -1;
	m_nPivotIndex = -1;	
	GetDesigner()->ResetDB(BUtil::eDBRF_Vertex);

	__super::Enter();

	m_StartingDragManipulatorPos = m_PivotPos = BrushVec3(0,0,0);
	GetIEditor()->SetEditMode(eEditModeMove);
	UpdateTMManipulator(m_PivotPos,BrushVec3(0,0,1));
}

void CBrushDesignerPivotTool::Leave()
{
	__super::Leave();
	GetIEditor()->ShowTransformManipulator(false);
	if( m_nPivotIndex != -1 )
	{
		CUndo undo("Designer : Pivot Tool"); 
		GetDesigner()->RecordUndo("Designer : Pivot",GetBaseObject());
		GetBrush()->PivotToPos(GetBaseObject(),GetDesigner(),m_PivotPos);
		UpdateBrush();
		UpdateGameResource(GetBaseObject());
	}
}

void CBrushDesignerPivotTool::BeginEditParams()
{
	if( !g_pPivotToolPanel )
		g_pPivotToolPanel = CreatePivotToolPanel(this,(void*)GetPanelIndex());	
}

void CBrushDesignerPivotTool::EndEditParams()
{
	if( g_pPivotToolPanel )
	{
		g_pPivotToolPanel->DestroyPanel();
		g_pPivotToolPanel = NULL;
	}
}

void CBrushDesignerPivotTool::SetSelectionType( EPivotSelectionType selectionType, bool bForce )
{
	if( m_nSelectedCandidate == selectionType && !bForce )
		return;

	m_nPivotIndex = -1;
	m_CandidateVertices.clear();

	if( selectionType == ePST_BoundBox )
	{
		AABB aabb;
		GetBaseObject()->GetLocalBounds(aabb);
		BrushVec3 step = ToBrushVec3((aabb.max-aabb.min)*0.5f);
		for( int i = 0; i <= 2; ++i )
			for( int j = 0; j <= 2; ++j )
				for( int k = 0; k <= 2; ++k )
					m_CandidateVertices.push_back(aabb.min+BrushVec3(i*step.x,j*step.y,k*step.z));
	}
	else if( selectionType == ePST_Designer )
	{
		CBrushDesignerDB* pDB = GetDesigner()->GetDB();
		pDB->GetVertexList(m_CandidateVertices);
	}
}

void CBrushDesignerPivotTool::Display( DisplayContext &dc )
{	
	dc.SetColor(0xAAAAAAFF);
	dc.DepthTestOff();
	dc.DepthWriteOff();

	dc.PopMatrix();

	for( int i = 0, iCount(m_CandidateVertices.size()); i < iCount; ++i )
	{
		if( m_nSelectedCandidate == i )
			continue;
		BrushVec3 vWorldVertexPos = GetWorldTM().TransformPoint(m_CandidateVertices[i]);
		BrushVec3 vBoxSize = BUtil::GetElementBoxSize(dc.view,dc.flags&DISPLAY_2D,vWorldVertexPos);
		dc.DrawSolidBox(ToVec3(vWorldVertexPos-vBoxSize), ToVec3(vWorldVertexPos+vBoxSize));
	}
	if( m_nSelectedCandidate != -1 )
	{
		dc.SetColor(RGB(100,100,255));
		BrushVec3 vWorldVertexPos = GetWorldTM().TransformPoint(m_CandidateVertices[m_nSelectedCandidate]);
		BrushVec3 vBoxSize = BUtil::GetElementBoxSize(dc.view,dc.flags&DISPLAY_2D,vWorldVertexPos);
		dc.DrawSolidBox(ToVec3(vWorldVertexPos-vBoxSize), ToVec3(vWorldVertexPos+vBoxSize));
	}

	dc.PushMatrix(GetWorldTM());

	dc.DepthWriteOn();
	dc.DepthTestOn();
}

void CBrushDesignerPivotTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_nSelectedCandidate != -1 )
	{
		UpdateTMManipulator(m_CandidateVertices[m_nSelectedCandidate],BrushVec3(0,0,1));
		m_nPivotIndex = m_nSelectedCandidate;
		m_PivotPos = m_CandidateVertices[m_nSelectedCandidate];
	}
}

void CBrushDesignerPivotTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	Vec3 raySrc, rayDir;
	view->ViewToWorldRay(point,raySrc,rayDir);

	BrushFloat leastT = (BrushFloat)3e10;
	m_nSelectedCandidate = -1;

	for( int i = 0, iCount(m_CandidateVertices.size()); i < iCount; ++i )
	{
		BrushFloat t = 0;
		BrushVec3 vWorldPos = GetWorldTM().TransformPoint(m_CandidateVertices[i]);
		BrushVec3 vBoxSize = BUtil::GetElementBoxSize(view,view->GetType()!=ET_ViewportCamera,vWorldPos);
		if( !BUtil::GetIntersectionOfRayAndAABB( raySrc, rayDir, AABB(vWorldPos-vBoxSize,vWorldPos+vBoxSize), &t ) )
			continue;
		if( t < leastT && t > 0 )
		{
			m_nSelectedCandidate = i;
			leastT = t;
		}
	}
}

bool CBrushDesignerPivotTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if( nChar == VK_ESCAPE )
		GetEditTool()->SetDesignerMode(BUtil::eDesigner_ObjectMode);
	return true;
}

void CBrushDesignerPivotTool::OnManipulatorDrag( CViewport *pView,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const BrushVec3 &value )
{
	BrushMatrix34 moveTM = GetOffsetTM(pManipulator,value);
	m_PivotPos = moveTM.TransformPoint(m_StartingDragManipulatorPos);
	UpdateTMManipulator(m_PivotPos,BrushVec3(0,0,1));
}

void CBrushDesignerPivotTool::OnManipulatorMouseEvent( CViewport *pView, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo )
{
	if( flags == eMouseLDown )
	{
		m_StartingDragManipulatorPos = m_PivotPos;
	}
}