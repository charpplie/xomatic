#include "StdAfx.h"
#include "BrushDesignerMagnetTool.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerEditTool.h"
#include "Viewport.h"

void CBrushDesignerMagnetTool::AddVertexToList( const BrushVec3& vertex, ColorB color, std::vector<SSourceVertex>& vertices )
{
	bool bHasSame = false;
	for( int i = 0, iVertexCount(vertices.size()); i < iVertexCount; ++i )
	{
		if( vertices[i].position.IsEquivalent(vertex,kDesignerEpsilon) )
		{
			bHasSame = true;
			break;
		}
	}
	if( !bHasSame )
		vertices.push_back(SSourceVertex(vertex,color));
}

void CBrushDesignerMagnetTool::Enter()
{
	__super::Enter();

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( pSelected->GetSize() != 1 )
	{
		AfxMessageBox("only one face should be selected to use this tool",MB_OK);
		GetEditTool()->GoToPrevDesignerMode();
		return;
	}

	CBrushDesignerElementManager copiedSelected(*pSelected);	
	for( int i = 0, iCount(pSelected->GetSize()); i < iCount; ++i )
	{
		if( !(*pSelected)[i].IsFace() || (*pSelected)[i].m_pRegion == NULL )
			copiedSelected.Erase((*pSelected)[i]);
	}

	pSelected->Set(copiedSelected);
	PrepareChooseFirstPointStep();
	m_PickedPos = BrushVec3(0,0,0);
	m_vTargetUpDir = BrushVec3(0,0,1);
}

void CBrushDesignerMagnetTool::Leave()
{
	if( m_Phase == eMTP_ChooseMoveToTargetPoint )
	{
		GetIEditor()->AcceptUndo("Designer : Magnet Tool");
		GetDesigner()->MoveShelf(1,0);
		UpdateBrush();
	}

	m_Phase = eMTP_ChooseFirstPoint;
	m_SourceVertices.clear();
	m_nSelectedSourceVertex = -1;
	m_nSelectedUpVertex = -1;
	m_pInitRegion = NULL;

	__super::Leave();
}

void CBrushDesignerMagnetTool::PrepareChooseFirstPointStep()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	
	if( pSelected->IsEmpty() || (*pSelected)[0].m_pRegion == NULL )
		return;

	m_SourceVertices.clear();

	const BrushPlane& plane = (*pSelected)[0].m_pRegion->GetPlane();
	for( int k = 0, iSelectedCount(pSelected->GetSize()); k < iSelectedCount; ++k )
	{
		const AABB& aabb((*pSelected)[k].m_pRegion->GetBoundBox());		

		AABB rectangleAABB;
		rectangleAABB.Reset();
		
		BrushVec2 v0 = plane.W2P(ToBrushVec3(aabb.min));
		BrushVec2 v1 = plane.W2P(ToBrushVec3(aabb.max));

		rectangleAABB.Add(Vec3(v0.x,0,v0.y));
		rectangleAABB.Add(Vec3(v1.x,0,v1.y));

		BrushVec3 step = ToBrushVec3((rectangleAABB.max-rectangleAABB.min)*0.5f);

		for( int i = 0; i <= 2; ++i )
			for( int j = 0; j <= 2; ++j )
				for( int k = 0; k <= 2; ++k )
				{
					BrushVec3 v = ToBrushVec3(rectangleAABB.min)+BrushVec3(i*step.x,j*step.y,k*step.z);
					v = plane.P2W(BrushVec2(v.x,v.z));
					AddVertexToList(v,ColorB(0xFF40FF40),m_SourceVertices);
				}

		(*pSelected)[k].m_bIsolated = true;
		for( int i = 0, iVertexCount((*pSelected)[k].m_pRegion->GetVertexListSize()); i < iVertexCount; ++i )
			AddVertexToList((*pSelected)[k].m_pRegion->GetVertex(i),BUtil::kElementBoxColor,m_SourceVertices);

		for( int i = 0, iEdgeCount((*pSelected)[k].m_pRegion->GetEdgeSize()); i < iEdgeCount; ++i )
		{
			BrushEdge3D e = (*pSelected)[k].m_pRegion->GetEdge(i);
			AddVertexToList(e.GetCenter(),BUtil::kElementBoxColor,m_SourceVertices);
		}

		AddVertexToList((*pSelected)[k].m_pRegion->GetRepresentativePosition(),BUtil::kElementBoxColor,m_SourceVertices);		
	}

	m_nSelectedSourceVertex = -1;
	m_nSelectedUpVertex = -1;
	m_Phase = eMTP_ChooseFirstPoint;
}

void CBrushDesignerMagnetTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_Phase == eMTP_ChooseFirstPoint && m_nSelectedSourceVertex != -1 )
	{
		m_nSelectedUpVertex = -1;
		m_Phase = eMTP_ChooseUpPoint;
		m_PickedPos = m_SourceVertices[m_nSelectedSourceVertex].position;
	}
	else if( m_Phase == eMTP_ChooseUpPoint )
	{
		CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
		GetIEditor()->BeginUndo();
		GetDesigner()->RecordUndo("Designer : Magnet Tool",GetBaseObject());
		m_pInitRegion = NULL;
		m_Phase = eMTP_ChooseMoveToTargetPoint;
		DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
		m_pInitRegion = (*pSelected)[0].m_pRegion->Clone();
		GetDesigner()->SetShelf(0);
		GetDesigner()->RemoveRegion((*pSelected)[0].m_pRegion);
		GetDesigner()->SetShelf(1);
		GetDesigner()->AddRegion((*pSelected)[0].m_pRegion,CBrushDesigner::eOpType_Add);
		UpdateBrush();
		m_bPickedTargetPos = false;
		m_bSwitchedSides = false;
	}
	else if( m_Phase == eMTP_ChooseMoveToTargetPoint )
	{
		GetEditTool()->GoToPrevDesignerMode();
	}
}

void CBrushDesignerMagnetTool::SwitchSides()
{
	DESIGNER_ASSERT(m_pInitRegion);
	if( !m_pInitRegion )
		return;
 	BrushVec3 vPivot = m_SourceVertices[m_nSelectedSourceVertex].position;
 	BrushVec3 vUpPos = m_SourceVertices[m_nSelectedUpVertex].position;
	BrushPlane mirrorPlane(vPivot, vUpPos, m_pInitRegion->GetPlane().Normal() + vPivot, kDesignerEpsilon);
	m_pInitRegion->Mirror(mirrorPlane);
	InitializeSelectedRegionBeforeTransform();

	if( m_bPickedTargetPos && !CheckVirtualKey(VK_SHIFT) )
	{
		AlignSelectedRegion();
	}
	else
	{
		UpdateSelectionMeshFromSelectedElementList(GetMainContext());
		UpdateShelf(1);
	}

	m_bSwitchedSides = !m_bSwitchedSides;
}

void CBrushDesignerMagnetTool::AlignSelectedRegion()
{
	std::vector<BrushEdge3D> edges;
	bool bEdgeExist = false;
	BrushEdge3D dirEdge;
	if( GetDesigner()->QueryEdgesHavingVertex(m_TargetPos,edges) )
	{
		int iEdgeCount(edges.size());
		for( int i = 0; i < iEdgeCount; ++i )
		{
			if( edges[i].m_v[0].IsEquivalent(m_TargetPos,kDesignerEpsilon) )
			{
				dirEdge = edges[i];
				bEdgeExist = true;
				break;
			}
			else if( edges[i].m_v[1].IsEquivalent(m_TargetPos,kDesignerEpsilon) )
			{
				dirEdge = edges[i].GetInverted();
				bEdgeExist = true;
				break;
			}
		}
	}
	if( bEdgeExist )
	{
		InitializeSelectedRegionBeforeTransform();

		DESIGNER_ASSERT(m_pInitRegion);
		if( !m_pInitRegion )
			return;

		BrushVec3 vSelectionNormal = m_pInitRegion->GetPlane().Normal();
		BrushVec3 vEdgeDir = (dirEdge.m_v[1]-dirEdge.m_v[0]).GetNormalized();
		BrushMatrix34 tmRot1 = ToBrushMatrix33(Matrix33::CreateRotationV0V1(vSelectionNormal,-vEdgeDir));
		BrushMatrix34 tmRot2 = BrushMatrix34::CreateIdentity();
		if( m_nSelectedUpVertex != -1 )
		{
			BrushVec3 vUpDir = (m_SourceVertices[m_nSelectedUpVertex].position-m_SourceVertices[m_nSelectedSourceVertex].position).GetNormalized();
			vUpDir = tmRot1.TransformVector(vUpDir);
			BrushVec3 vIntermediateDir = vEdgeDir.Cross(vUpDir);
			tmRot2 = ToBrushMatrix33(Matrix33::CreateRotationV0V1(vIntermediateDir,m_vTargetUpDir)*Matrix33::CreateRotationV0V1(vUpDir,vIntermediateDir));
		}
		BrushMatrix34 tmRot = tmRot2 * tmRot1;
		CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
		(*pSelected)[0].m_pRegion->Move(-m_TargetPos);
		(*pSelected)[0].m_pRegion->Transform(tmRot);
		(*pSelected)[0].m_pRegion->Move(m_TargetPos);

		UpdateSelectionMeshFromSelectedElementList(GetMainContext());
		UpdateShelf(1);
	}
}

bool CBrushDesignerMagnetTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if( nChar == VK_ESCAPE )
	{
		if( m_Phase == eMTP_ChooseMoveToTargetPoint || m_Phase == eMTP_ChooseUpPoint )
		{
			CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();			
			if( m_bSwitchedSides )
				SwitchSides();

			if( m_pInitRegion )
				*((*pSelected)[0].m_pRegion) = *m_pInitRegion;

			GetDesigner()->MoveShelf(1,0);
			UpdateBrush();
			GetIEditor()->CancelUndo();
			PrepareChooseFirstPointStep();
			UpdateSelectionMeshFromSelectedElementList(GetMainContext());
		}
		else if( m_Phase == eMTP_ChooseFirstPoint )
		{
			GetEditTool()->GoToPrevDesignerMode();
		}
	}
	else if( nChar == VK_CONTROL && m_Phase == eMTP_ChooseMoveToTargetPoint )
	{
		SwitchSides();
	}
	return true;
}

void CBrushDesignerMagnetTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	BrushVec3 localRaySrc, localRayDir;
	BUtil::GetLocalViewRay( GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir );	

	Vec3 raySrc, rayDir;
	view->ViewToWorldRay(point,raySrc,rayDir);

	if( m_Phase == eMTP_ChooseFirstPoint || m_Phase == eMTP_ChooseUpPoint )
	{
		BrushFloat fLeastDist = (BrushFloat)3e10;
		
		int nSelectedVertex = -1;		
		for( int i = 0, iVertexCount(m_SourceVertices.size()); i < iVertexCount; ++i )
		{
			const BrushVec3& v(m_SourceVertices[i].position);
			BrushFloat dist;
			BrushVec3 vWorldPos = GetWorldTM().TransformPoint(v);
			BrushVec3 vBoxSize = BUtil::GetElementBoxSize(view, view->GetType()!=ET_ViewportCamera, vWorldPos);
			if( !BUtil::GetIntersectionOfRayAndAABB(ToBrushVec3(raySrc), ToBrushVec3(rayDir), AABB(vWorldPos-vBoxSize,vWorldPos+vBoxSize), &dist) )
				continue;

			if( dist > 0 && dist < fLeastDist )
			{
				fLeastDist = dist;
				nSelectedVertex = i;
			}
		}

		if( nSelectedVertex != -1 )
		{
			if( m_Phase == eMTP_ChooseFirstPoint )
				m_nSelectedSourceVertex = nSelectedVertex;
			else
			{
				if( nSelectedVertex != m_nSelectedSourceVertex )
					m_nSelectedUpVertex = nSelectedVertex;
				else
					m_nSelectedUpVertex = -1;
				m_PickedPos = m_SourceVertices[nSelectedVertex].position;
			}
		}
		else
		{
			GetDesigner()->QueryPosition( localRaySrc, localRayDir, m_PickedPos );
		}
	}
	else
	{
		BrushVec3 vPickedPos;
		m_bPickedTargetPos = !(nFlags & MK_SHIFT) ? m_bPickedTargetPos : false;
		CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
		if( pSelected->QueryNearestVertex( GetBaseObject(), GetDesigner(), view, point, localRaySrc, localRayDir, vPickedPos, &m_vTargetUpDir ) )
		{
			m_TargetPos = vPickedPos;
			InitializeSelectedRegionBeforeTransform();
			UpdateSelectionMeshFromSelectedElementList(GetMainContext());
			UpdateShelf(1);

			m_bPickedTargetPos = true;
		}
		if( m_bPickedTargetPos && !(nFlags & MK_SHIFT) )
			AlignSelectedRegion();
	}
}

void CBrushDesignerMagnetTool::InitializeSelectedRegionBeforeTransform()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	const BrushVec3& vInitPivot = m_SourceVertices[m_nSelectedSourceVertex].position;
	BrushMatrix34 tmDelta = BrushMatrix34::CreateIdentity();
	tmDelta.SetTranslation(m_TargetPos-vInitPivot);

	DESIGNER_ASSERT(m_pInitRegion);
	if( !m_pInitRegion )
		return;

	*((*pSelected)[0].m_pRegion) = *m_pInitRegion;
	(*pSelected)[0].m_pRegion->Transform(tmDelta);
}

void CBrushDesignerMagnetTool::Display( DisplayContext &dc )
{
	dc.PopMatrix();
	if( m_Phase == eMTP_ChooseFirstPoint || m_Phase == eMTP_ChooseUpPoint )
	{		
		for( int i = 0, iVertexCount(m_SourceVertices.size()); i < iVertexCount; ++i )
		{
			dc.SetColor(m_SourceVertices[i].color);
			const BrushVec3& v = m_SourceVertices[i].position;
			if( m_nSelectedSourceVertex == i || m_nSelectedUpVertex == i )
				continue;
			BrushVec3 vWorldVertexPos = GetWorldTM().TransformPoint(v);
			BrushVec3 vVertexBoxSize = BUtil::GetElementBoxSize(dc.view,dc.flags&DISPLAY_2D,vWorldVertexPos);
			dc.DrawSolidBox(ToVec3(vWorldVertexPos-vVertexBoxSize), ToVec3(vWorldVertexPos+vVertexBoxSize));
		}		

		if( m_nSelectedSourceVertex != -1 )
		{
			BrushVec3 vWorldVertexPos = GetWorldTM().TransformPoint(m_SourceVertices[m_nSelectedSourceVertex].position);
			BrushVec3 vBoxSize = BUtil::GetElementBoxSize(dc.view,dc.flags&DISPLAY_2D,vWorldVertexPos);
			dc.SetColor(BUtil::kSelectedColor);
			dc.DrawSolidBox(ToVec3(vWorldVertexPos-vBoxSize), ToVec3(vWorldVertexPos+vBoxSize));
			
			if( m_Phase == eMTP_ChooseUpPoint )
			{
				dc.SetColor(ColorB(0,150,214,255));
				dc.SetLineWidth(4);
				dc.DrawLine(GetWorldTM().TransformPoint(m_SourceVertices[m_nSelectedSourceVertex].position),GetWorldTM().TransformPoint(m_PickedPos));
			}
		}
		if( m_nSelectedUpVertex != -1 )
		{
			BrushVec3 vWorldVertexPos = GetWorldTM().TransformPoint(m_SourceVertices[m_nSelectedUpVertex].position);
			BrushVec3 vBoxSize = BUtil::GetElementBoxSize(dc.view,dc.flags&DISPLAY_2D,vWorldVertexPos);
			dc.SetColor(ColorB(0,150,214,255));
			dc.DrawSolidBox(ToVec3(vWorldVertexPos-vBoxSize), ToVec3(vWorldVertexPos+vBoxSize));
		}
	}
	else if( m_Phase == eMTP_ChooseMoveToTargetPoint )
	{
		std::vector<BrushVec3> excludedVertices;
		if( m_bPickedTargetPos )
		{
			excludedVertices.push_back(m_TargetPos);
			dc.SetColor(BUtil::kSelectedColor);
			BrushVec3 vWorldPos = GetWorldTM().TransformPoint(m_TargetPos);
			BrushVec3 vBoxSize = BUtil::GetElementBoxSize(dc.view,dc.flags&DISPLAY_2D,vWorldPos);
			dc.DrawSolidBox(ToVec3(vWorldPos-vBoxSize), ToVec3(vWorldPos+vBoxSize));
		}

		dc.PushMatrix(GetWorldTM());
		CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
		pSelected->DisplayVertexElements(GetBaseObject(),GetDesigner(),dc,0,&excludedVertices);
		dc.PopMatrix();
	}
	dc.PushMatrix(GetWorldTM());
}