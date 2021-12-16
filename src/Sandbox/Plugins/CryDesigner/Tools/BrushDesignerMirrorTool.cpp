#include "StdAfx.h"
#include "BrushDesignerMirrorTool.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerSelectTool.h"
#include "BrushDesignerEditTool.h"
#include "IBaseToolPanel.h"

namespace
{
	IMirrorToolPanel* g_pMirrorToolPanel = NULL;
}

void CBrushDesignerMirrorTool::BeginEditParams()
{
	if( !g_pMirrorToolPanel )
		g_pMirrorToolPanel = CreateMirrorToolPanel(this,(void*)GetPanelIndex());
	CenterPivot();
}

void CBrushDesignerMirrorTool::EndEditParams()
{
	if( g_pMirrorToolPanel )
	{
		g_pMirrorToolPanel->DestroyPanel();
		g_pMirrorToolPanel = NULL;
	}
}

void CBrushDesignerMirrorTool::ApplyMirror()
{
	CUndo undo("Designer : Apply Mirror");
	GetDesigner()->RecordUndo("Designer : Apply Mirror",GetBaseObject());

	_smart_ptr<CBrushDesigner> frontPart;
	_smart_ptr<CBrushDesigner> backPart;
	if( GetDesigner()->Clip( m_SlicePlane, frontPart, backPart, false ) == CBrushDesigner::eCRR_CLIPFAILED || backPart == NULL )
		return;

	(*GetDesigner()) = *backPart;

	for( int i = 0, iRegionSize(backPart->GetRegionSize()); i < iRegionSize; ++i )
		GetDesigner()->AddRegion( backPart->GetRegion(i)->Mirror(m_SlicePlane), CBrushDesigner::eOpType_Add );

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->Clear();

	GetDesigner()->SetMirrorPlane(m_SlicePlane);
	GetDesigner()->SetModeFlag(GetDesigner()->GetModeFlag()|CBrushDesigner::eDesignerMode_Mirror);

	UpdateBrush();

	g_pMirrorToolPanel->ToggleWndEnableDisable();
	UpdateGizmo();

	UpdateGameResource(GetBaseObject());

	Sync();
}

void CBrushDesignerMirrorTool::FreezeDesigner()
{
	CUndo undo("Designer : Release Mirror");
	GetDesigner()->RecordUndo("Designer : Freeze Designer",GetBaseObject());
	ReleaseMirrorMode(GetDesigner());
	RemoveEdgesOnMirrorPlane(GetDesigner());
	g_pMirrorToolPanel->ToggleWndEnableDisable();
	UpdateGizmo();
	UpdateBrush();
	Sync();
}

void CBrushDesignerMirrorTool::ReleaseMirrorMode( CBrushDesigner* pDesigner )
{
	pDesigner->SetModeFlag(pDesigner->GetModeFlag()&(~CBrushDesigner::eDesignerMode_Mirror));
	for( int i = 0, iRegionSize(pDesigner->GetRegionSize()); i < iRegionSize; ++i )
		pDesigner->GetRegion(i)->RemoveFlags(CBrushRegion::eRF_Mirrored);
}

void CBrushDesignerMirrorTool::RemoveEdgesOnMirrorPlane( CBrushDesigner* pDesigner )
{
	BrushPlane mirrorPlane = pDesigner->GetMirrorPlane();

	struct SEdgeAndPlaneStruct
	{
		SEdgeAndPlaneStruct( const BrushEdge3D& edge, const BrushPlane& plane ) : m_Edge(edge),m_Plane(plane){}
		BrushEdge3D m_Edge;
		BrushPlane m_Plane;
	};

	std::vector<SEdgeAndPlaneStruct> edgePlaneList;

	for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
		DESIGNER_ASSERT(pRegion);
		if( !pRegion )
			continue;

		for( int k = 0, iEdgeCount(pRegion->GetEdgeSize()); k < iEdgeCount; ++k )
		{
			BrushEdge3D edge = pRegion->GetEdge(k);
			if( std::abs(mirrorPlane.Distance(edge.m_v[0])) < kDesignerEpsilon && std::abs(mirrorPlane.Distance(edge.m_v[1])) < kDesignerEpsilon )
			{
				edgePlaneList.push_back(SEdgeAndPlaneStruct(edge,pRegion->GetPlane()));
				edgePlaneList.push_back(SEdgeAndPlaneStruct(edge.GetInverted(),pRegion->GetPlane().GetInverted()));
			}
		}
	}

	for( int i = 0, iEdgePlaneListCount(edgePlaneList.size()); i < iEdgePlaneListCount; ++i )
		pDesigner->EraseEdge(edgePlaneList[i].m_Edge);
}

void CBrushDesignerMirrorTool::Display( DisplayContext &dc )
{
	if( GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		return;
	__super::Display(dc);
}

void CBrushDesignerMirrorTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	__super::OnEditorNotifyEvent(event);
	switch(event)
	{
	case eNotify_OnEndUndoRedo:
		if( g_pMirrorToolPanel )
			g_pMirrorToolPanel->ToggleWndEnableDisable();
		break;
	}
}

void CBrushDesignerMirrorTool::OnManipulatorMouseEvent( CViewport *pView, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo )
{
	if( event == eMouseLDown )
	{
		if( GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		{
			GetIEditor()->BeginUndo();
			GetDesigner()->RecordUndo("Designer : Mirror.Move Pivot",GetBaseObject());
		}
		m_PrevGizmoPos = BrushVec3(0,0,0);
	}
	else if( event == eMouseLUp )
	{
		if( GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		{
			GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
			GetIEditor()->AcceptUndo("Designer : Mirrir.Move Pivot");
		}
	}
}

void CBrushDesignerMirrorTool::UpdateGizmo()
{
	if( GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
	{
		BrushVec3 vAveragePos(0,0,0);
		int nCount(0);
		for( int i = 0, iRegionSize(GetDesigner()->GetRegionSize()); i < iRegionSize; ++i )
		{
			CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(i);
			if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) )
				continue;
			for( int k = 0, iVertexSize(pRegion->GetVertexListSize()); k < iVertexSize; ++k )
			{
				vAveragePos += pRegion->GetVertex(k);
				++nCount;
			}
		}
		vAveragePos /= nCount;
		m_GizmoPos = vAveragePos;
	}
	else
	{
		m_GizmoPos = m_CursorPos;
	}
	UpdateTMManipulator(m_GizmoPos,BrushVec3(0,0,1));
}

bool CBrushDesignerMirrorTool::UpdateManipulatorInMirrorMode( const BrushMatrix34& offsetTM )
{
	for( int i = 0, iRegionSize(GetDesigner()->GetRegionSize()); i < iRegionSize; ++i )
	{
		CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(i);
		if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) )
			continue;
		pRegion->Transform(offsetTM);
	}

	CreateMirroredRegions(GetDesigner());
	UpdateBrush();

	return true;
}