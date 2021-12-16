#include "StdAfx.h"
#include "BrushDesignerLatheTool.h"
#include "BrushDesignerEditTool.h"
#include "BrushDesignerSelectTool.h"
#include "Viewport.h"

void CBrushDesignerLatheTool::Enter()
{
	__super::Enter();
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( pSelected->IsEmpty() )
		GetEditTool()->GoToPrevDesignerMode();
}

void CBrushDesignerLatheTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	BrushVec3 localRaySrc, localRayDir;
	BUtil::GetLocalViewRay( GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir );
	int nPickedRegion(-1);
	BrushVec3 outPos;
	if( !GetDesigner()->QueryRegion(localRaySrc, localRayDir, nPickedRegion) )
		return;

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( pSelected->GetSize() == 1 && (*pSelected)[0].m_pRegion )
		m_pPathRegion = (*pSelected)[0].m_pRegion;
	else
		m_pPathRegion = NULL;

	ELatheErrorCode errorCode = CreateShapeAlongPath(GetDesigner()->GetRegion(nPickedRegion));
	if( errorCode == eLEC_Success )
		return;

	switch(errorCode)
	{
	case eLEC_NoPath:
		MessageBox( NULL, "Edges or a face have to be selected to be used as a path.", "Warning", MB_OK );
		break;
	case eLEC_InappropriateProfileShape:
		MessageBox( NULL, "The profile face is not appropriate.", "Warning", MB_OK );
		break;
	case eLEC_ProfileShapeTooBig:
		MessageBox( NULL, "The profile face is too big or located at a wrong position. You should reduce the width of the profile face or move it.", "Warning", MB_OK );
		break;
	}

	GetEditTool()->GoToPrevDesignerMode();
}

std::vector<BrushVec3> CBrushDesignerLatheTool::ExtractPathFromSelectedElements( bool& bOutClosed )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();	
	std::vector<BrushVec3> vPath;
	bOutClosed = true;
	if( pSelected->GetSize() != 1 || !(*pSelected)[0].IsFace() )
	{
		int iSelectedElementCount(pSelected->GetSize());
		for( int i = 0; i < iSelectedElementCount; ++i )
		{
			if( !(*pSelected)[i].IsEdge() )
				continue;
			BrushEdge3D edge = (*pSelected)[i].GetEdge();
			vPath.push_back(edge.m_v[0]);
		}

		BrushEdge3D firstEdge = (*pSelected)[0].GetEdge();
		BrushEdge3D lastEdge = (*pSelected)[iSelectedElementCount-1].GetEdge();
		bOutClosed = iSelectedElementCount >= 3 && (firstEdge.m_v[0].IsEquivalent(lastEdge.m_v[1],kDesignerEpsilon) || firstEdge.m_v[1].IsEquivalent(lastEdge.m_v[0],kDesignerEpsilon));
		if( !bOutClosed )
			vPath.push_back(lastEdge.m_v[1]);
	}
	else
	{
		if( (*pSelected)[0].m_pRegion->IsOpen() )
		{
			(*pSelected)[0].m_pRegion->GetLinkedVertices(vPath);
			bOutClosed = false;
		}
		else
		{
			std::vector<CBrushRegion::RegionPtr> outSeparatedRegions;
			(*pSelected)[0].m_pRegion->GetSeparatedRegions(outSeparatedRegions,CBrushRegion::eSR_OuterHull);
			if( outSeparatedRegions.size() == 1 )
				outSeparatedRegions[0]->GetLinkedVertices(vPath);
		}		
	}

	return vPath;
}

std::vector<BrushPlane> CBrushDesignerLatheTool::CreatePlanesAtEachPointOfPath( const std::vector<BrushVec3>& vPath, bool bPathClosed )
{
	std::vector<BrushPlane> planeAtEveryIntersection;
	int iPathCount = vPath.size();
	planeAtEveryIntersection.resize(iPathCount);

	int nPathCount = bPathClosed ? iPathCount : iPathCount-2;
	for( int i = 0; i < nPathCount; ++i )
	{
		const BrushVec3& vCommon = vPath[(i+1)%iPathCount];
		BrushVec3 vDir = (vCommon-vPath[i]).GetNormalized();
		BrushVec3 vDirNext = (vPath[(i+2)%iPathCount]-vCommon).GetNormalized();
		planeAtEveryIntersection[(i+1)%iPathCount] = BrushPlane(vCommon, vCommon-vDir.Cross(vDirNext), vCommon-vDir+vDirNext, kDesignerEpsilon);
	}

	if( !bPathClosed )
	{
		BrushVec3 vDir = (vPath[0]-vPath[1]).GetNormalized();
		planeAtEveryIntersection[0] = BrushPlane(vDir,-vDir.Dot(vPath[0]));
		vDir = (vPath[iPathCount-1]-vPath[iPathCount-2]).GetNormalized();
		planeAtEveryIntersection[iPathCount-1] = BrushPlane(vDir,-vDir.Dot(vPath[iPathCount-1]));
	}

	return planeAtEveryIntersection;
}

void CBrushDesignerLatheTool::AddRegionToDesigner( CBrushDesigner* pDesigner, const std::vector<BrushVec3>& vList, CBrushRegion::RegionPtr pInitRegion, bool bFlip )
{
	CBrushRegion::RegionPtr pRegion = pInitRegion->Clone();

	for( int i = 0, iVertexCount(vList.size()); i < iVertexCount; ++i )
		pRegion->SetVertex(i,vList[i]);

	BrushPlane plane;
	if( pRegion->GetComputedPlane(plane) )
	{
		pRegion->SetPlane(plane);
		if( bFlip )
			pRegion->Flip();
		GetDesigner()->AddRegionUnconditionally(pRegion);
	}
}

bool CBrushDesignerLatheTool::GlueRegions( const std::vector<CBrushRegion::RegionPtr>& regions )
{
	for( int i = 0, iRegionCount(regions.size()); i < iRegionCount; ++i )
	{
		BrushPlane invertedPlane = regions[i]->GetPlane().GetInverted();

		std::vector<CBrushRegion::RegionPtr> candidatedRegions;
		if( !GetDesigner()->QueryRegions(invertedPlane,candidatedRegions) || candidatedRegions.empty() )
			continue;

		CBrushRegion::RegionPtr pFlipedRegion = regions[i]->Clone()->Flip();
		bool bSubtracted = false;
		std::vector<CBrushRegion::RegionPtr> intersectedRegions;
		for( int k = 0, iCandidateCount(candidatedRegions.size()); k < iCandidateCount; ++k )
		{
			if( CBrushRegion::HasIntersection(candidatedRegions[k],pFlipedRegion) == BUtil::eIT_Intersection )
				intersectedRegions.push_back(candidatedRegions[k]);
		}

		if( intersectedRegions.empty() )
			continue;

		for( int k = 0, iIntersectCount(intersectedRegions.size()); k < iIntersectCount; ++k )
		{
			if( m_pPathRegion == intersectedRegions[k] )
				continue;
			bSubtracted = true;
			intersectedRegions[k]->Subtract(pFlipedRegion);
		}

		if( bSubtracted )
		{
			GetDesigner()->RemoveRegion(regions[i]);
			return true;
		}
	}
	return false;
}

CBrushDesignerLatheTool::ELatheErrorCode CBrushDesignerLatheTool::CreateShapeAlongPath( CBrushRegion::RegionPtr pInitProfileRegion )
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();	
	if( pSelected->IsEmpty() )
	{
		GetEditTool()->GoToPrevDesignerMode();
		return eLEC_NoPath;
	}

	bool bClosed = true;
	std::vector<BrushVec3> vPath = ExtractPathFromSelectedElements(bClosed);
	if( vPath.empty() )
	{
		GetEditTool()->GoToPrevDesignerMode();
		return eLEC_NoPath;
	}

	std::vector<BrushPlane> planeAtEveryIntersection = CreatePlanesAtEachPointOfPath(vPath,bClosed);

	int iPathCount = vPath.size();
	int nStartIndex = 0;
	BrushFloat fNearestDist = (BrushFloat)3e10;
	BrushVec3 vProfilePos = pInitProfileRegion->GetRepresentativePosition();
	for( int i = 0; i < iPathCount; ++i )
	{
		BrushFloat fDist = vProfilePos.GetDistance(vPath[i]);
		if( fDist < fNearestDist )
		{
			fNearestDist = fDist;
			nStartIndex = i;
		}
	}

	CUndo undo("Designer : Lathe Tool");
	GetDesigner()->RecordUndo("Designer : Lathe Tool",GetBaseObject());

	int nVertexCount = pInitProfileRegion->GetVertexListSize();
	int iEdgeCount = pInitProfileRegion->GetEdgeSize();

	std::vector<BrushVec3> prevVertices(nVertexCount);
	std::vector<BrushPlane> prevPlanes(iEdgeCount);

	BrushVec3 vEdgeDir = (vPath[(nStartIndex+1)%iPathCount]-vPath[nStartIndex]).GetNormalized();
	for( int k = 0; k < nVertexCount; ++k )
	{
		const BrushVec3& v = pInitProfileRegion->GetVertex(k);

		bool bHitTest0 = planeAtEveryIntersection[nStartIndex].HitTest(v, v+vEdgeDir, kDesignerEpsilon, NULL, &prevVertices[k]);
		DESIGNER_ASSERT(bHitTest0);
		if( !bHitTest0 )
			return eLEC_InappropriateProfileShape;
	}

	for( int k = 0; k < iEdgeCount; ++k )
		prevPlanes[k] = BrushPlane(BrushVec3(0,0,0),0);

	GetDesigner()->SetShelf(1);	

	if( !bClosed )
		AddRegionToDesigner( GetDesigner(), prevVertices, pInitProfileRegion, false );

	int nPathCount = bClosed ? iPathCount : iPathCount-1;
	for( int i = 0; i < nPathCount; ++i )
	{
		int nPathIndex = (nStartIndex+i)%iPathCount;
		int nNextPathIndex = (nPathIndex+1)%iPathCount;

		const BrushPlane& planeNext = planeAtEveryIntersection[nNextPathIndex];
		vEdgeDir = (vPath[nNextPathIndex]-vPath[nPathIndex]).GetNormalized();

		std::vector<BrushVec3> vertices(nVertexCount);
		for( int k = 0; k < nVertexCount; ++k )
		{
			if( planeNext.Distance(prevVertices[k]) > kDesignerEpsilon )
			{
				GetDesigner()->Clear();
				UpdateBrush();
				undo.Cancel();
				return eLEC_ProfileShapeTooBig;
			}

			bool bHitTest0 = planeNext.HitTest(prevVertices[k], prevVertices[k]+vEdgeDir, kDesignerEpsilon, NULL, &vertices[k]);
			DESIGNER_ASSERT(bHitTest0);
			if( !bHitTest0 )
				continue;
		}

		for( int k = 0; k < iEdgeCount; ++k )
		{
			const BUtil::SEdge& e = pInitProfileRegion->GetEdgeIndexPair(k);

			std::vector<BrushVec3> vList(4);

			vList[0] = prevVertices[e.m_i[1]];
			vList[1] = prevVertices[e.m_i[0]];
			vList[2] = vertices[e.m_i[0]];
			vList[3] = vertices[e.m_i[1]];

			CBrushRegion::RegionPtr pSideRegion = new CBrushRegion(vList);

			if( prevPlanes[k].Normal().IsZero() )
			{
				prevPlanes[k] = pSideRegion->GetPlane();
			}
			else
			{
				BrushFloat d0 = std::abs(prevPlanes[k].Distance(vPath[nPathIndex]));
				BrushFloat d1 = std::abs(prevPlanes[k].Distance(vPath[nNextPathIndex]));
				if( std::abs(d0-d1) < kDesignerEpsilon && pSideRegion->GetPlane().IsSameFacing(prevPlanes[k]) )
					pSideRegion->SetPlane(prevPlanes[k]);
			}

			GetDesigner()->AddRegion(pSideRegion,CBrushDesigner::eOpType_Union);
		}

		prevVertices = vertices;
	}

	if( !bClosed )
		AddRegionToDesigner( GetDesigner(), prevVertices, pInitProfileRegion, true );

	std::vector<CBrushRegion::RegionPtr> newRegions;
	for( int i = 0, iRegionCount(GetDesigner()->GetRegionSize()); i < iRegionCount; ++i )
		newRegions.push_back(GetDesigner()->GetRegion(i));

	GetDesigner()->SetShelf(0);
	GetDesigner()->RemoveRegion(pInitProfileRegion);

	GetDesigner()->MoveShelf(1,0);
	
	bool bSubtractedFloor = GlueRegions(newRegions);
	
	for( int i = 0, iSelectedElementCount(pSelected->GetSize()); i < iSelectedElementCount; ++i )
	{
		if( (*pSelected)[i].IsEdge() )
			GetDesigner()->EraseEdge((*pSelected)[i].GetEdge());
		else if( !bSubtractedFloor && (*pSelected)[i].IsFace() ) 
			GetDesigner()->RemoveRegion((*pSelected)[i].m_pRegion);
	}

	pSelected->Clear();	
	for( int i = 0, iCount(newRegions.size()); i < iCount; ++i )
		pSelected->Add(SDesignerElement(GetBaseObject(),newRegions[i]));

	UpdateBrush();
	GetEditTool()->SetDesignerMode(BUtil::eDesigner_Select_Face);

	return eLEC_Success;
}