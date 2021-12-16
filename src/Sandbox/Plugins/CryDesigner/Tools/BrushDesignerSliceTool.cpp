#include "StdAfx.h"
#include "BrushDesignerSliceTool.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerEditTool.h"
#include "ViewManager.h"
#include "ITransformManipulator.h"
#include "Objects/DesignerBrushObject.h"
#include "Objects/AreaSolidObject.h"
#include "IBaseToolPanel.h"

int CBrushDesignerSliceTool::m_NumberSlicePlane = 0;

namespace
{
	IBaseToolPanel* g_pSliceToolPanel = NULL;
}

CBrushDesignerSliceTool::CBrushDesignerSliceTool()
{
	m_PrevGizmoPos = m_GizmoPos = m_CursorPos = BrushVec3(0,0,0);
}

void CBrushDesignerSliceTool::Enter()
{
	AABB aabb;
	GetBaseObject()->GetLocalBounds(aabb);
	m_SlicePlane = BrushPlane(BrushVec3(0,0,1),-aabb.GetCenter().z);
	GenerateLoop(m_SlicePlane,m_MainTraverseLines);	
	UpdateGizmo();

	__super::Enter();
}

void CBrushDesignerSliceTool::Leave()
{
	__super::Leave();
	GetIEditor()->ShowTransformManipulator(false);
}

void CBrushDesignerSliceTool::BeginEditParams()
{
	if( !g_pSliceToolPanel )
		g_pSliceToolPanel = CreateSliceToolPanel(this,(void*)GetPanelIndex());
	CenterPivot();
}

void CBrushDesignerSliceTool::EndEditParams()
{
	if( g_pSliceToolPanel )
	{
		g_pSliceToolPanel->DestroyPanel();
		g_pSliceToolPanel = NULL;
	}
}

void CBrushDesignerSliceTool::Display( DisplayContext &dc )
{
	DrawOutlines(dc);
}

void CBrushDesignerSliceTool::DrawOutlines( DisplayContext& dc )
{
	dc.SetDrawInFrontMode(true);

	float oldLineWidth = dc.GetLineWidth();
	dc.SetLineWidth(3);

	dc.SetColor(ColorB(99,99,99,255));
	for( int i = 0, iSize(m_RestTraverseLineSet.size()); i < iSize; ++i )
		DrawOutline(dc,m_RestTraverseLineSet[i]);

	dc.SetColor(ColorB(50,200,50,255));
	DrawOutline(dc,m_MainTraverseLines);

	dc.SetLineWidth(oldLineWidth);

	dc.DepthTestOff();
	dc.DrawArrow( m_CursorPos, m_CursorPos+m_SlicePlane.Normal()*4.0f, 2.0f );
	dc.DepthTestOn();

	dc.SetDrawInFrontMode(false);
}

void CBrushDesignerSliceTool::DrawOutline( DisplayContext& dc, TraverseLineList& lineList )
{
	for( int i = 0, iSize(lineList.size()); i < iSize; ++i )
		dc.DrawLine(lineList[i].m_Edge.m_v[0],lineList[i].m_Edge.m_v[1]);
}

void CBrushDesignerSliceTool::GenerateLoop( const BrushPlane& SlicePlane, TraverseLineList& outLineList ) const
{
	outLineList.clear();

	for( int i = 0, iRegionSize(GetDesigner()->GetRegionSize()); i < iRegionSize; ++i )
	{
		CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(i);
		BrushLine3D intersectedLine;
		if( !SlicePlane.IntersectionLine(pRegion->GetPlane(),intersectedLine) )
			continue;

		std::vector<BrushEdge3D> sortedIntersectedEdges;
		if( !pRegion->QueryIntersections(SlicePlane,intersectedLine,sortedIntersectedEdges) )
			continue;

		for( int i = 0, iEdgeSize(sortedIntersectedEdges.size()); i < iEdgeSize; ++i )
			outLineList.push_back(ETraverseLineInfo(pRegion,sortedIntersectedEdges[i],SlicePlane));
	}
}

BrushVec3 CBrushDesignerSliceTool::GetLoopPivotPoint() const
{
	AABB aabb;
	aabb.Reset();
	for( int i = 0, iSize(m_MainTraverseLines.size()); i < iSize; ++i )
	{
		aabb.Add(m_MainTraverseLines[i].m_Edge.m_v[0]);
		aabb.Add(m_MainTraverseLines[i].m_Edge.m_v[1]);
	}
	return aabb.GetCenter();
}

void CBrushDesignerSliceTool::SliceFrontPart()
{
	_smart_ptr<CBrushDesigner> frontPart;
	_smart_ptr<CBrushDesigner> backPart;

	CUndo undo("Designer : Slice Front");

	if( GetDesigner()->Clip( m_SlicePlane, frontPart, backPart, true ) == CBrushDesigner::eCRR_CLIPFAILED )
		return;

	GetDesigner()->RecordUndo("Designer : Slice Front",GetBaseObject());

	if( backPart )
	{
		(*GetDesigner()) = *backPart;
		UpdateRestTraverseLineSet();
		GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
		UpdateBrush();
		Sync();
	}
}

void CBrushDesignerSliceTool::SliceBackPart()
{
	_smart_ptr<CBrushDesigner> frontPart;
	_smart_ptr<CBrushDesigner> backPart;

	CUndo undo("Designer : Slice Back");

	if( GetDesigner()->Clip( m_SlicePlane, frontPart, backPart, true ) == CBrushDesigner::eCRR_CLIPFAILED  )
		return;

	GetDesigner()->RecordUndo("Designer : Slice Back",GetBaseObject());

	if( frontPart )
	{
		(*GetDesigner()) = *frontPart;
		UpdateRestTraverseLineSet();
		GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
		UpdateBrush();
		Sync();
	}
}

void CBrushDesignerSliceTool::Divide()
{
	TraverseLineLists traverseLineListSet;
	if( m_RestTraverseLineSet.empty() )
		traverseLineListSet.push_back(m_MainTraverseLines);
	else
		traverseLineListSet = m_RestTraverseLineSet;

	CUndo undo("Designer : Divide");
	GetDesigner()->RecordUndo("Designer : Divide",GetBaseObject());

	for( int i = 0, iSize(traverseLineListSet.size()); i < iSize; ++i )
	{
		const TraverseLineList& lineList = traverseLineListSet[i];
		for( int k = 0, iLineListSize(lineList.size()); k < iLineListSize; ++ k )
		{
			CBrushRegion::RegionPtr pSliceRegion = new CBrushRegion;
			pSliceRegion->SetPlane(lineList[k].m_pRegion->GetPlane());
			pSliceRegion->AddEdge(lineList[k].m_Edge);
			GetDesigner()->AddOpenRegion( pSliceRegion, false );
			GetDesigner()->RemoveRegion(lineList[k].m_pRegion);
		}
	}
	GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
	UpdateBrush();
	Sync();

	UpdateGameResource(GetBaseObject());
}

void CBrushDesignerSliceTool::Clip()
{
	_smart_ptr<CBrushDesigner> frontPart;
	_smart_ptr<CBrushDesigner> backPart;

	CUndo undo("Designer : Slip");

	if( GetDesigner()->Clip( m_SlicePlane, frontPart, backPart, true ) == CBrushDesigner::eCRR_CLIPFAILED )
		return;

	GetDesigner()->RecordUndo("Designer : Slip",GetBaseObject());

	if( !frontPart || !backPart )
	{
		if( frontPart )
			(*GetDesigner()) = *frontPart;
		else if( backPart )
			(*GetDesigner()) = *backPart;
	}
	else
	{
		(*GetDesigner()) = *frontPart;

		CBaseObject* pClonedObject = GetIEditor()->GetObjectManager()->CloneObject(GetBaseObject());
		if( pClonedObject->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
		{
			CDesignerBrushObject* pBackObj = ((CDesignerBrushObject*)pClonedObject);
			pBackObj->SetDesigner(backPart);
			pBackObj->GetBrush()->Update( pBackObj, pBackObj->GetDesigner() );
		}
		else if( pClonedObject->IsKindOf(RUNTIME_CLASS(CAreaSolid)) )
		{
			CAreaSolid* pBackObj = ((CAreaSolid*)pClonedObject);
			pBackObj->SetDesigner(backPart);
			pBackObj->GetBrush()->Update( pBackObj, pBackObj->GetDesigner() );
		}
		else
		{
			DESIGNER_ASSERT(0);
		}
	}

	UpdateRestTraverseLineSet();
	GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
	UpdateBrush();
	Sync();

	UpdateGameResource(GetBaseObject());
}

void CBrushDesignerSliceTool::UpdateSlicePlane()
{
	GenerateLoop(m_SlicePlane,m_MainTraverseLines);
	UpdateRestTraverseLineSet();
}

void CBrushDesignerSliceTool::SetNumberSlicePlane( int numberSlicePlane )
{
	if( numberSlicePlane < 0 || numberSlicePlane > 100 )
		return;

	m_NumberSlicePlane = numberSlicePlane;
	UpdateRestTraverseLineSet();
}

void CBrushDesignerSliceTool::UpdateRestTraverseLineSet()
{
	m_RestTraverseLineSet.clear();

	if( m_NumberSlicePlane <= 0 )
		return;

	AABB localBoundbox;
	GetBaseObject()->GetLocalBounds(localBoundbox);

	Vec3 apexes[] = {
		Vec3( localBoundbox.min.x, localBoundbox.min.y, localBoundbox.min.z ), 
		Vec3( localBoundbox.max.x, localBoundbox.min.y, localBoundbox.min.z ), 
		Vec3( localBoundbox.min.x, localBoundbox.max.y, localBoundbox.min.z ), 
		Vec3( localBoundbox.max.x, localBoundbox.max.y, localBoundbox.min.z ), 
		Vec3( localBoundbox.min.x, localBoundbox.min.y, localBoundbox.max.z ), 
		Vec3( localBoundbox.max.x, localBoundbox.min.y, localBoundbox.max.z ), 
		Vec3( localBoundbox.min.x, localBoundbox.max.y, localBoundbox.max.z ), 
		Vec3( localBoundbox.max.x, localBoundbox.max.y, localBoundbox.max.z ) };

	int nFarthestFrontApex = -1;
	float fFarthestFrontDistance = -1;
	int nFarthestBackApex = -1;
	float fFarthestBackDistance = -1;

	BrushPlane planeWithZeroDistance( m_SlicePlane.Normal(), 0 );
	for( int i = 0, iSize(sizeof(apexes)/sizeof(apexes[0])); i < iSize; ++i )
	{
		float fDistance = planeWithZeroDistance.Distance(apexes[i]);
		if( fDistance >= 0 )
		{
			if( fDistance > fFarthestFrontDistance )
			{
				fFarthestFrontDistance = fDistance;
				nFarthestFrontApex = i;
			}
		}
		else if( fDistance < 0 )
		{
			if( -fDistance > -fFarthestBackDistance )
			{
				fFarthestBackDistance = fDistance;
				nFarthestBackApex = i;
			}
		}
	}

	if( nFarthestFrontApex == -1 || nFarthestBackApex == -1 )
		return;

	int nRealNumSlicePlane = m_NumberSlicePlane+2;

	m_RestTraverseLineSet.resize(nRealNumSlicePlane-2);
	float fGap = (fFarthestFrontDistance-fFarthestBackDistance)/(nRealNumSlicePlane-1);

	for( int i = 1; i < nRealNumSlicePlane-1; ++i )
	{
		BrushPlane plane( m_SlicePlane.Normal(), fFarthestBackDistance+fGap*i );
		GenerateLoop(plane,m_RestTraverseLineSet[i-1]);
	}
}

void CBrushDesignerSliceTool::AlignSlicePlane( const BrushVec3& normal )
{
	BrushPlane plane(normal,0);
	m_SlicePlane = BrushPlane(normal,-plane.Distance(m_CursorPos));
	GenerateLoop(m_SlicePlane,m_MainTraverseLines);
	UpdateRestTraverseLineSet();
}

void CBrushDesignerSliceTool::InvertSlicePlane()
{
	m_SlicePlane.Invert();
	GenerateLoop(m_SlicePlane,m_MainTraverseLines);
	UpdateRestTraverseLineSet();
}

void CBrushDesignerSliceTool::OnManipulatorDrag( CViewport *pView,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const BrushVec3 &value )
{
	if( GetIEditor()->GetEditMode() == eEditModeScale )
		return;

	BrushVec3 vDelta = value-m_PrevGizmoPos;
	if( vDelta.IsEquivalent(BrushVec3(0,0,0),kDesignerEpsilon) )
		return;

	BrushMatrix34 offsetTM = GetOffsetTM(pManipulator,vDelta);
	bool bUpdatedManipulator = false;
	bool bDesignerMirrorMode = GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror);

	if( bDesignerMirrorMode )
		bUpdatedManipulator = UpdateManipulatorInMirrorMode(offsetTM);

	if( !bUpdatedManipulator )
	{
		if( GetIEditor()->GetEditMode() == eEditModeMove )
		{
			m_GizmoPos += vDelta;
			m_CursorPos = m_GizmoPos;
			AlignSlicePlane(m_SlicePlane.Normal());
		}
		else if( !bDesignerMirrorMode && GetIEditor()->GetEditMode() == eEditModeRotate )
		{
			AlignSlicePlane(offsetTM.TransformVector(m_SlicePlane.Normal()));
		}
	}

	UpdateTMManipulator(m_GizmoPos,BrushVec3(0,0,1));
	m_PrevGizmoPos = value;
}

void CBrushDesignerSliceTool::OnManipulatorMouseEvent( CViewport *pView, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo )
{
	if( event == eMouseLDown )
		m_PrevGizmoPos = BrushVec3(0,0,0);
}

void CBrushDesignerSliceTool::UpdateGizmo()
{
	m_GizmoPos = m_CursorPos;
	UpdateTMManipulator(m_GizmoPos,BrushVec3(0,0,1));
}

void CBrushDesignerSliceTool::CenterPivot()
{
	AABB bbox;
	if( !GetBaseObject() )
		return;
	GetBaseObject()->GetLocalBounds(bbox);
	BrushVec3 vCenter = bbox.GetCenter();
	m_CursorPos = vCenter;
	AlignSlicePlane(m_SlicePlane.Normal());
	UpdateGizmo();
}