#include "StdAfx.h"
#include "BrushDesignerFillSpaceTool.h"
#include "BrushDesignerSelectTool.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerEditTool.h"
#include "Viewport.h"

void CBrushDesignerFillSpaceTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	BrushVec3 localRaySrc, localRayDir;
	BUtil::GetLocalViewRay( GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir );

	int nQueriedIndex = -1;

	if( m_pHoleContainer->QueryRegion( localRaySrc, localRayDir, nQueriedIndex ) )
	{
		m_PickedHoleRegion = m_pHoleContainer->GetRegion(nQueriedIndex);
		UpdateSelectionMesh(m_PickedHoleRegion,GetBrush(),GetBaseObject());
	}
	else
	{
		m_PickedHoleRegion = NULL;
		UpdateSelectionMesh(NULL,GetBrush(),GetBaseObject());
	}
}

void CBrushDesignerFillSpaceTool::OnLButtonDown( CViewport *view, UINT nFlags, CPoint point )
{
	if( m_PickedHoleRegion )
	{
		CUndo undo("Designer : Fill Space");

		GetDesigner()->RecordUndo("Fill Space",GetBaseObject());

		std::vector<CBrushRegion::RegionPtr> perpendicularRegions;
		GetDesigner()->QueryAdjacentPerpendicularRegions( m_PickedHoleRegion, perpendicularRegions );

		if( perpendicularRegions.size() == m_PickedHoleRegion->GetEdgeSize()*2 )
		{
			CBrushRegion::RegionPtr pOppositeRegion;
			BrushFloat fDistance = 0;
			if( CBrushDesigner::eER_Intersection == m_pHoleContainer->QueryOppositeRegion( m_PickedHoleRegion, CBrushDesigner::eFOF_PushDirection, 0.0f, pOppositeRegion, fDistance ) )
			{
				std::vector<CBrushRegion::RegionPtr> perpendicularRegionsFromOpposite;
				GetDesigner()->QueryAdjacentPerpendicularRegions( pOppositeRegion, perpendicularRegionsFromOpposite );

				if( perpendicularRegions.size() == perpendicularRegionsFromOpposite.size() )
				{
					bool bShareTunnel = true;
					int iRegionSize(perpendicularRegions.size());
					for( int i = 0; i < iRegionSize; ++i )
					{
						if( !ContainRegion( perpendicularRegions[i], perpendicularRegionsFromOpposite ) )
						{
							bShareTunnel = false;
							break;
						}
					}

					if( bShareTunnel )
					{
						for( int i = 0; i < iRegionSize; ++i )
						{
							GetDesigner()->RemoveRegion(perpendicularRegions[i]);
							RemoveMirroredRegion(GetDesigner(),perpendicularRegions[i]);
						}

						GetDesigner()->AddRegion(pOppositeRegion->Clone(),CBrushDesigner::eOpType_Add);
						UpdateMirroredPartWithPlane(GetDesigner(),pOppositeRegion->GetPlane());
						m_pHoleContainer->AddRegion(pOppositeRegion,CBrushDesigner::eOpType_SubtractAB);
					}
				}
			}
		}

		GetDesigner()->AddRegion( m_PickedHoleRegion->Clone(), CBrushDesigner::eOpType_Add );
		UpdateMirroredPartWithPlane( GetDesigner(), m_PickedHoleRegion->GetPlane() );
		UpdateBrush();
		Sync();

		m_pHoleContainer->DrillRegion(m_PickedHoleRegion);
	}
}

bool CBrushDesignerFillSpaceTool::ContainRegion( CBrushRegion::RegionPtr pRegion, std::vector<CBrushRegion::RegionPtr>& regionList )
{
	for( int i = 0, iRegionSize(regionList.size()); i < iRegionSize; ++i )
	{
		if( pRegion == regionList[i] )
			return true;
	}
	return false;
}

void CBrushDesignerFillSpaceTool::Enter()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( !pSelected->IsEmpty() )
	{
		if( FillHoleBasedOnSelectedElements() )
		{
			pSelected->Clear();
			GetDesigner()->ClearExcludedEdgesInDrawing();
			GetEditTool()->GoToSelectDesignerMode();
			return;
		}
	}

	CompileHoles();
}

void CBrushDesignerFillSpaceTool::Leave()
{
	m_pHoleContainer = NULL;
}

bool CBrushDesignerFillSpaceTool::FillHoleBasedOnSelectedElements()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	std::vector<BrushEdge3D> validEdgeList;
	for( int i = 0, iElementSize(pSelected->GetSize()); i < iElementSize; ++i )
	{
		if( !(*pSelected)[i].IsEdge() )
			continue;
		validEdgeList.push_back((*pSelected)[i].GetEdge());
	}

	if( validEdgeList.size() == 2 )
	{
		if( !validEdgeList[0].m_v[0].IsEquivalent(validEdgeList[1].m_v[0],kDesignerEpsilon) && !validEdgeList[0].m_v[0].IsEquivalent(validEdgeList[1].m_v[1],kDesignerEpsilon) && 
			!validEdgeList[0].m_v[1].IsEquivalent(validEdgeList[1].m_v[0],kDesignerEpsilon) && !validEdgeList[0].m_v[1].IsEquivalent(validEdgeList[1].m_v[1],kDesignerEpsilon) )
		{
			BrushVec3 vEdge0Dir = (validEdgeList[0].m_v[1]-validEdgeList[0].m_v[0]).GetNormalized();
			BrushVec3 vEdge0V0ToEdge1V0 = (validEdgeList[1].m_v[0]-validEdgeList[0].m_v[0]).GetNormalized();
			BrushVec3 vEdge0V0ToEdge1V1 = (validEdgeList[1].m_v[1]-validEdgeList[0].m_v[0]).GetNormalized();
			if( vEdge0Dir.Dot(vEdge0V0ToEdge1V0) < vEdge0Dir.Dot(vEdge0V0ToEdge1V1) )
				validEdgeList[1].Invert();

			validEdgeList.push_back(BrushEdge3D(validEdgeList[1].m_v[1],validEdgeList[0].m_v[0]));
			validEdgeList.push_back(BrushEdge3D(validEdgeList[0].m_v[1],validEdgeList[1].m_v[0]));
		}
	}

	if( validEdgeList.empty() )
	{
		std::vector<BrushVec3> validVertexList;
		for( int i = 0, iElementSize(pSelected->GetSize()); i < iElementSize; ++i )
		{
			if( !(*pSelected)[i].IsVertex() )
				continue;
			validVertexList.push_back((*pSelected)[i].m_Vertices[0]);
		}
		if( validVertexList.size() < 2 )
			return false;
		for( int i = 0, iVertexSize(validVertexList.size()); i < iVertexSize; ++i )
			validEdgeList.push_back(BrushEdge3D(validVertexList[i],validVertexList[(i+1)%iVertexSize]));
	}

	if( validEdgeList.empty() )
		false;
	std::vector<BrushEdge3D> linkedEdgeList;
	std::set<int> usedEdgeSet;

	linkedEdgeList.push_back(validEdgeList[0]);
	usedEdgeSet.insert(0);
	const int nValidEdgeSize(validEdgeList.size());
	bool bFinishLoop = false;

	while( linkedEdgeList.size() < nValidEdgeSize )
	{
		bool bFoundNext = false;
		std::set<int>::iterator iEdgeIndex(usedEdgeSet.begin());
		for( ; iEdgeIndex != usedEdgeSet.end(); ++iEdgeIndex )
		{
			int nCurrentIndex = *iEdgeIndex;
			int k = 0;
			for( ; k < nValidEdgeSize; ++k )
			{
				if( nCurrentIndex == k || usedEdgeSet.find(k) != usedEdgeSet.end() )
					continue;

				if( validEdgeList[nCurrentIndex].m_v[0].IsEquivalent(validEdgeList[k].m_v[0],kDesignerEpsilon) || validEdgeList[nCurrentIndex].m_v[1].IsEquivalent(validEdgeList[k].m_v[1],kDesignerEpsilon) )
				{
					usedEdgeSet.insert(k);
					linkedEdgeList.insert(linkedEdgeList.begin(),validEdgeList[k].GetInverted());
					bFoundNext = true;
					break;
				}
				else if( validEdgeList[nCurrentIndex].m_v[0].IsEquivalent(validEdgeList[k].m_v[1],kDesignerEpsilon) || validEdgeList[nCurrentIndex].m_v[1].IsEquivalent(validEdgeList[k].m_v[0],kDesignerEpsilon) )
				{
					usedEdgeSet.insert(k);
					linkedEdgeList.push_back(validEdgeList[k]);
					bFoundNext = true;
					break;
				}
			}
			if( bFoundNext )
				break;
		}
		if( !bFoundNext )
			break;
	}

	if( linkedEdgeList.size() < 2 )
		return false;

	CBrushRegion::RegionPtr pFilledRegion = new CBrushRegion;
	for( int i = 0, iEdgeSize(linkedEdgeList.size()); i < iEdgeSize; ++i )
		pFilledRegion->AddEdge(linkedEdgeList[i]);

	if( pFilledRegion->GetEdgeSize() < 2 )
		return false;

	CUndo undo("Designer : Fill a hole");
	GetDesigner()->RecordUndo("Designer : Fill a hole",GetBaseObject());

	std::vector<BrushVec3> linkedVertices;
	pFilledRegion->GetLinkedVertices(linkedVertices);	
	BrushPlane plane;
	if( !BUtil::ComputePlane(linkedVertices,plane) )
		plane = BrushPlane(linkedVertices[0],linkedVertices[1],linkedVertices[2],kDesignerEpsilon);
	pFilledRegion->SetPlane(plane);

	if( pFilledRegion->IsOpen() )
	{
		pFilledRegion->AddEdge(BrushEdge3D(linkedVertices[linkedVertices.size()-1],linkedVertices[0]));
		if( pFilledRegion->IsOpen() )
		{
			undo.Cancel();
			return false;
		}
	}

	Matrix34 invWorldTM = GetBaseObject()->GetWorldTM().GetInverted();

	const CCamera& camera = GetIEditor()->GetRenderer()->GetCamera();
	BrushVec3 vLocalCameraNormal = ToBrushVec3(invWorldTM.TransformVector(camera.GetViewdir()));
	if( vLocalCameraNormal.Dot(plane.Normal()) > 0 )
		pFilledRegion->Flip();

	pFilledRegion->Optimize();

	BrushVec3 vLocalCameraPos = ToBrushVec3(invWorldTM.TransformPoint(camera.GetPosition()));
	BrushVec3 vLocalHitPos;

	pFilledRegion->GetPlane().HitTest( vLocalCameraPos, vLocalCameraPos+vLocalCameraNormal, kDesignerEpsilon, NULL, &vLocalHitPos );

	GetDesigner()->AddRegion(pFilledRegion, CBrushDesigner::eOpType_Add);
	CreateMirroredRegions(GetDesigner());

	UpdateBrush();

	return true;
}

void CBrushDesignerFillSpaceTool::CompileHoles()
{
	std::vector<CBrushRegion::RegionPtr> regionList[2];

	int nSource = 0;
	int nTarget = 1;

	for( int i = 0, iRegionSize(GetDesigner()->GetRegionSize()); i < iRegionSize; ++i )
	{
		CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(i);
		if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored|CBrushRegion::eRF_Hidden) || pRegion->IsOpen() )
			continue;
		CBrushRegion::RegionPtr pClonedRegion = pRegion->Clone();
		if( pClonedRegion->HasBridgeEdges() )
			pClonedRegion->RemoveBridgeEdges();
		regionList[nSource].push_back(pClonedRegion);
	}

	bool bHasAnyChanges = true;

	while(bHasAnyChanges)
	{
		for( int i = 0, iRegionSize(regionList[nSource].size()); i < iRegionSize; ++i )
		{
			CBrushRegion::RegionPtr pRegion = regionList[nSource][i];
			CBrushRegion::RegionPtr pAdjacentRegion = QueryAdjacentRegion( pRegion, regionList[nTarget] );
			if( pAdjacentRegion == NULL )
				regionList[nTarget].push_back(pRegion);
			else
				pAdjacentRegion->Union(pRegion);
		}
		if( regionList[nSource].size() == regionList[nTarget].size() )
		{
			bHasAnyChanges = false;
		}
		else
		{
			nSource = 1-nSource;
			nTarget = 1-nTarget;
			regionList[nTarget].clear();
		}
	}

	if( !m_pHoleContainer )
	{
		m_pHoleContainer = new CBrushDesigner;
		m_pHoleContainer->SetModeFlag(0);
	}
	m_pHoleContainer->Clear();

	for( int i = 0, iRegionSize(regionList[nTarget].size()); i < iRegionSize; ++i )
	{
		std::vector<CBrushRegion::RegionPtr> innerRegions;
		regionList[nTarget][i]->GetSeparatedRegions(innerRegions,CBrushRegion::eSR_InnerHull);

		for( int k = 0, innerRegionSize(innerRegions.size()); k < innerRegionSize; ++ k )
		{
			innerRegions[k]->ReverseEdges();
			for( int a = 0; a < iRegionSize; ++a )
			{
				if( a == i )
					continue;
				if( regionList[nTarget][a]->GetPlane().IsEquivalent( innerRegions[k]->GetPlane(), kDesignerEpsilon ) )
					innerRegions[k]->Subtract(regionList[nTarget][a]);
			}

			if( innerRegions[k]->IsValid() && !innerRegions[k]->IsOpen() )
				m_pHoleContainer->AddRegion( innerRegions[k], CBrushDesigner::eOpType_Add );
		}
	}
}

CBrushRegion::RegionPtr CBrushDesignerFillSpaceTool::QueryAdjacentRegion( CBrushRegion::RegionPtr pRegion, std::vector<CBrushRegion::RegionPtr>& regionList )
{
	for( int i = 0, iRegionSize(regionList.size()); i < iRegionSize; ++i )
	{
		BUtil::EIntersectionType it = CBrushRegion::HasIntersection( regionList[i], pRegion );
		if( it == BUtil::eIT_JustTouch )
			return regionList[i];
	}
	return NULL;
}

void CBrushDesignerFillSpaceTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	__super::OnEditorNotifyEvent(event);
	switch(event)
	{
	case eNotify_OnEndUndoRedo:
		CompileHoles();
		break;
	}
}