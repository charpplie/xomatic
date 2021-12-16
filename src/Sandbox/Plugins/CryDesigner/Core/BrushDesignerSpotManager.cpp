#include "StdAfx.h"
#include "Viewport.h"
#include "BrushDesignerSpotManager.h"
#include "Tools/BrushDesignerBaseTool.h"
#include "BrushDesigner.h"
#include "ViewManager.h"
#include "SurfaceInfoPicker.h"

void CBrushDesignerSpotManager::DrawCurrentSpot( DisplayContext& dc, const BrushMatrix34& worldTM ) const
{
	static const ColorB edgeCenterColor(100,255,100,255);
	static const ColorB regionCenterColor(100,255,100,255);
	static const ColorB eitherPointColor(255,100,255,255);
	static const ColorB normalColor(100,100,100,255);
	static const ColorB edgeColor(100,100,255,255);
	static const ColorB startPointColor(255,255,100,255);

	if( m_CurrentSpot.m_PosState == eSpotPosState_Edge || m_CurrentSpot.m_PosState == eSpotPosState_OnVirtualLine )
		BUtil::DrawSpot( dc, worldTM, m_CurrentSpot.m_Pos, edgeColor );
	else if( m_CurrentSpot.m_PosState == eSpotPosState_CenterOfEdge )
		BUtil::DrawSpot( dc, worldTM, m_CurrentSpot.m_Pos, edgeCenterColor );
	else if( m_CurrentSpot.m_PosState == eSpotPosState_CenterOfRegion )
		BUtil::DrawSpot( dc, worldTM, m_CurrentSpot.m_Pos, regionCenterColor );
	else if( m_CurrentSpot.m_PosState == eSpotPosState_EitherPointOfEdge || m_CurrentSpot.IsAtEndPoint() )
		BUtil::DrawSpot( dc, worldTM, m_CurrentSpot.m_Pos, eitherPointColor );
	else if( m_CurrentSpot.m_PosState == eSpotPosState_AtFirstSpot )
		BUtil::DrawSpot( dc, worldTM, m_CurrentSpot.m_Pos, startPointColor );
	else
		BUtil::DrawSpot( dc, worldTM, m_CurrentSpot.m_Pos, normalColor );
}

void CBrushDesignerSpotManager::DrawPolyline( DisplayContext& dc ) const
{
	if( m_SpotList.empty() )
		return;

	for( int i = 0, iSpotSize(m_SpotList.size()-1); i < iSpotSize; ++i )
	{
		if( m_SpotList[i].m_bProcessed )
			continue;
		const BrushVec3& currPos(m_SpotList[i].m_Pos);
		const BrushVec3& nextPos(m_SpotList[i+1].m_Pos);
		dc.DrawLine( currPos, nextPos );
	}
}

bool CBrushDesignerSpotManager::AddRegionToDesignerFromSpotList( CBrushDesigner* pDesigner, const SpotList& spotList )
{
	if( pDesigner == NULL || spotList.empty() )
		return false;

	SpotList copiedSpotList(spotList);

	SSpot firstSpot = copiedSpotList[0];
	SSpot lastSpot = copiedSpotList[copiedSpotList.size()-1];

	if( firstSpot.IsEquivalentPos(lastSpot) )
	{
		SpotList spotListWithoutBeginning(copiedSpotList);
		spotListWithoutBeginning.erase(spotListWithoutBeginning.begin());
		CreateRegionFromSpots(true,spotListWithoutBeginning);
	}
	else if( firstSpot.IsAtEndPoint() && lastSpot.IsAtEndPoint() )
	{
		CBrushRegion::RegionPtr pRegion0 = firstSpot.m_pRegion;
		CBrushRegion::RegionPtr pRegion1 = lastSpot.m_pRegion;
		if( pRegion0 )
		{
			for( int i = 0, nSpotSize(copiedSpotList.size()-1); i < nSpotSize; ++i )
			{
				if( firstSpot.m_PosState == eSpotPosState_FirstPointOfRegion )
					pRegion0->AddEdge(BrushEdge3D(copiedSpotList[i+1].m_Pos,copiedSpotList[i].m_Pos));
				else if( firstSpot.m_PosState == eSpotPosState_LastPointOfRegion )
					pRegion0->AddEdge(BrushEdge3D(copiedSpotList[i].m_Pos,copiedSpotList[i+1].m_Pos));
			}
			if( pRegion1 && pRegion0 != pRegion1 )
			{
				if( pRegion0->Concatenate(pRegion1) )
					pDesigner->RemoveRegion(pRegion1);
			}

			BrushVec3 firstVertex;
			BrushVec3 lastVertex;
			pRegion0->GetFirstVertex(firstVertex);
			pRegion0->GetLastVertex(lastVertex);

			if( !pRegion0->IsOpen() )
			{
				pDesigner->RemoveRegion(pRegion0);
				pRegion0->ModifyOrientation();
				pDesigner->AddRegion(pRegion0, CBrushDesigner::eOpType_Split);
			}
			else if( pDesigner->IsVertexOnEdge(pRegion0->GetPlane(),firstVertex,pRegion0) && pDesigner->IsVertexOnEdge(pRegion0->GetPlane(),lastVertex,pRegion0) )
			{
				pDesigner->RemoveRegion(pRegion0);
				pDesigner->AddOpenRegion( pRegion0, false );
			}
		}
	}
	else if( firstSpot.IsAtEndPoint() || lastSpot.IsAtEndPoint() )
	{
		if( !firstSpot.IsAtEndPoint() )
		{
			std::swap(firstSpot,lastSpot);
			int iSize(copiedSpotList.size());
			for( int i = 0, iHalfSize(iSize/2); i < iHalfSize; ++i )
				std::swap(copiedSpotList[i],copiedSpotList[iSize-i-1]);
		}

		CBrushRegion::RegionPtr pRegion0 = firstSpot.m_pRegion;
		if( !pRegion0 )
			return true;

		pDesigner->RemoveRegion(pRegion0);

		for( int i = 0, nSpotSize(copiedSpotList.size()-1); i < nSpotSize; ++i )
		{
			if( firstSpot.m_PosState == eSpotPosState_FirstPointOfRegion )
				pRegion0->AddEdge(BrushEdge3D(copiedSpotList[i+1].m_Pos,copiedSpotList[i].m_Pos));
			else if( firstSpot.m_PosState == eSpotPosState_LastPointOfRegion )
				pRegion0->AddEdge(BrushEdge3D(copiedSpotList[i].m_Pos,copiedSpotList[i+1].m_Pos));
		}

		if( !pRegion0->IsOpen() )
		{
			pDesigner->AddRegion(pRegion0, CBrushDesigner::eOpType_Split);
		}
		else
		{
			BrushVec3 firstVertex;
			BrushVec3 lastVertex;
			bool bOnlyAdd = false;

			bool bFirstOnEdge = pRegion0->GetFirstVertex(firstVertex) && pDesigner->IsVertexOnEdge(pRegion0->GetPlane(),firstVertex,pRegion0);
			bool bLastOnEdge = pRegion0->GetLastVertex(lastVertex) && pDesigner->IsVertexOnEdge(pRegion0->GetPlane(),lastVertex,pRegion0);

			if( bFirstOnEdge && bLastOnEdge )
				bOnlyAdd = false;
			else
				bOnlyAdd = true;

			pDesigner->AddOpenRegion(pRegion0, bOnlyAdd);
		}
	} 
	else if( firstSpot.IsOnEdge() && lastSpot.IsOnEdge() )
	{
		BrushPlane plane;
		if( FindBestPlane(pDesigner,firstSpot,lastSpot,plane) )
		{
			std::vector<BrushVec3> vList;
			GenerateVertexListFromSpotList( copiedSpotList, vList );
			CBrushRegion::RegionPtr pOpenRegion = new CBrushRegion( vList, plane, 0, NULL, false );			
			pDesigner->AddOpenRegion(pOpenRegion, false);
		}
	}
	else if( firstSpot.IsInRegion() || lastSpot.IsInRegion() )
	{
		BrushPlane plane;
		if( FindBestPlane(pDesigner,firstSpot,lastSpot,plane) )
		{
			std::vector<BrushVec3> vList;
			GenerateVertexListFromSpotList( copiedSpotList, vList );
			CBrushRegion::RegionPtr pRegion = new CBrushRegion( vList, plane, 0, NULL, false );
			pDesigner->AddOpenRegion(pRegion, true);
		}
	}
	else if( firstSpot.m_PosState == eSpotPosState_OutsideDesigner && lastSpot.m_PosState == eSpotPosState_OutsideDesigner )
	{
		if( firstSpot.m_Plane.IsEquivalent(lastSpot.m_Plane,kDesignerEpsilon) )
		{
			std::vector<BrushVec3> vList;
			GenerateVertexListFromSpotList( copiedSpotList, vList );
			CBrushRegion::RegionPtr pRegion = new CBrushRegion( vList, firstSpot.m_Plane, 0, NULL, false );
			pDesigner->AddOpenRegion(pRegion, true);
		}
	}
	else
	{
		ResetAllSpots();
		return false;
	}

	if( pDesigner )
		pDesigner->ResetDB(BUtil::eDBRF_ALL);

	return true;
}

void CBrushDesignerSpotManager::GenerateVertexListFromSpotList( const SpotList& spotList, std::vector<BrushVec3>& outVList )
{
	int iSpotSize(spotList.size());
	outVList.reserve(iSpotSize);
	for( int i = 0; i < iSpotSize; ++i )
		outVList.push_back(spotList[i].m_Pos);
}

void CBrushDesignerSpotManager::RegisterSpotList( CBrushDesigner* pDesigner, const SpotList& spotList )
{
	if( pDesigner == NULL )
		return;

	if( spotList.size() <= 1 )
	{
		ResetAllSpots();
		return;
	}

	std::vector<SpotList> splittedSpotList;
	SplitSpotList( pDesigner, spotList, splittedSpotList );

	if( !splittedSpotList.empty() )
	{
		for( int i = 0, spotListCount(splittedSpotList.size()); i < spotListCount; ++i )
			AddRegionToDesignerFromSpotList(pDesigner,splittedSpotList[i]);
	}
}

void CBrushDesignerSpotManager::SplitSpotList( CBrushDesigner* pDesigner, const SpotList& spotList, std::vector<SpotList>& outSpotLists )
{
	if( pDesigner == NULL )
		return;

	SpotList partSpotList;

	for( int k = 0, iSpotListCount(spotList.size()); k < iSpotListCount-1; ++k )
	{
		const SSpot& currentSpot = spotList[k];
		const SSpot& nextSpot = spotList[k+1];

		SpotPairList splittedSpotPairs;
		SplitSpot( pDesigner, SSpotPair(currentSpot,nextSpot), splittedSpotPairs );

		if( splittedSpotPairs.empty() )
			continue;

		std::map<BrushFloat,SSpotPair> sortedSpotPairs;
		for( int i = 0, iSpotPairCount(splittedSpotPairs.size()); i < iSpotPairCount; ++i )
		{
			BrushFloat fDistance = currentSpot.m_Pos.GetDistance(splittedSpotPairs[i].m_Spot[0].m_Pos);
			sortedSpotPairs[fDistance] = splittedSpotPairs[i];
		}

		std::map<BrushFloat,SSpotPair>::iterator ii = sortedSpotPairs.begin();
		for( ; ii != sortedSpotPairs.end(); ++ii )
		{
			const SSpotPair& spotPair = ii->second;

			if( partSpotList.empty() )
				partSpotList.push_back(spotPair.m_Spot[0]);

			partSpotList.push_back(spotPair.m_Spot[1]);
			if( !spotPair.m_Spot[1].IsEquivalentPos(nextSpot) )
			{
				outSpotLists.push_back(partSpotList);
				partSpotList.clear();
			}
		}
	}
	if( !partSpotList.empty() )
		outSpotLists.push_back(partSpotList);
}

void CBrushDesignerSpotManager::SplitSpot( CBrushDesigner* pDesigner, const SSpotPair& spotPair, SpotPairList& outSpotPairs )
{
	if( pDesigner == NULL )
		return;

	bool bSubtracted = false;
	BrushEdge3D inputEdge(spotPair.m_Spot[0].m_Pos, spotPair.m_Spot[1].m_Pos);
	BrushEdge3D invInputEdge(spotPair.m_Spot[1].m_Pos, spotPair.m_Spot[0].m_Pos);

	BrushPlane plane;
	if( !FindBestPlane(pDesigner,spotPair.m_Spot[0],spotPair.m_Spot[1],plane) )
		return;

	for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
		if( pRegion == NULL )
			continue;

		if( !pRegion->GetPlane().IsEquivalent(plane,kDesignerEpsilon) )
			continue;

		std::vector<BrushEdge3D> subtractedEdges;
		bSubtracted = pRegion->SubtractEdge( inputEdge, subtractedEdges );

		if( !subtractedEdges.empty() )
		{
			std::vector<BrushEdge3D>::iterator ii = subtractedEdges.begin();
			for( ; ii != subtractedEdges.end(); )
			{
				if( (*ii).IsEquivalent(inputEdge,kDesignerEpsilon) || (*ii).IsEquivalent(invInputEdge,kDesignerEpsilon) )
					ii = subtractedEdges.erase(ii);
				else
					++ii;
			}
			if( subtractedEdges.empty() )
				bSubtracted = false;
		}

		if( subtractedEdges.empty() )
		{
			if( bSubtracted )
				break;
			continue;
		}

		if( subtractedEdges.size() == 1 )
		{
			std::vector<BrushEdge3D>::iterator ii = subtractedEdges.begin();
			if( (*ii).m_v[0].IsEquivalent((*ii).m_v[1],kDesignerEpsilon) )
				continue;
		}

		int iSubtractedEdgeCount(subtractedEdges.size());

		for( int k = 0; k < iSubtractedEdgeCount; ++k )
		{
			BrushEdge3D subtractedEdge = subtractedEdges[k];
			SSpotPair subtractedSpotPair;

			for( int a = 0; a < 2; ++a )
			{
				subtractedSpotPair.m_Spot[a].m_Pos = subtractedEdge.m_v[a];

				if( pRegion->HasVertex(subtractedEdge.m_v[a]) )
				{
					subtractedSpotPair.m_Spot[a].m_PosState = eSpotPosState_EitherPointOfEdge;
					subtractedSpotPair.m_Spot[a].m_pRegion = pRegion;
				}
				else if( pRegion->IsVertexOnCrust(subtractedEdge.m_v[a]) )
				{
					subtractedSpotPair.m_Spot[a].m_PosState = eSpotPosState_Edge;
					subtractedSpotPair.m_Spot[a].m_pRegion = pRegion;
				}
			}

			SplitSpot( pDesigner, subtractedSpotPair, outSpotPairs );
		}
		break;
	}

	if( !bSubtracted )
		outSpotPairs.push_back(spotPair);
}

bool CBrushDesignerSpotManager::FindSpotNearAxisAlignedLine( IDisplayViewport* pViewport, CBrushRegion::RegionPtr pRegion, const BrushMatrix34& worldTM, SSpot& outSpot )
{
	if( m_CurrentSpot.IsOnEdge() || !pRegion->Include(m_CurrentSpot.m_Pos) )
		return false;

	std::vector<BrushLine> axisAlignedLines;
	if( !pRegion->QueryAxisAlignedLines(axisAlignedLines) )
		return false;

	BrushVec2 vCurrentPos2D = pRegion->GetPlane().W2P(m_CurrentSpot.m_Pos);
	for( int i = 0, iLineCount(axisAlignedLines.size()); i < iLineCount; ++i )
	{
		BrushVec2 vHitPos;
		if( !axisAlignedLines[i].HitTest(vCurrentPos2D, vCurrentPos2D+axisAlignedLines[i].m_Normal,kDesignerEpsilon,0,&vHitPos) )
			continue;

		BrushFloat fLength(0);
		if( BUtil::AreTwoPositionsNear(pRegion->GetPlane().P2W(vHitPos),m_CurrentSpot.m_Pos,worldTM,pViewport,kLimitForMagnetic,&fLength) )
		{
			outSpot.Reset();
			outSpot.m_pRegion = pRegion;
			outSpot.m_PosState = eSpotPosState_OnVirtualLine;
			outSpot.m_Plane = pRegion->GetPlane();
			outSpot.m_Pos = outSpot.m_Plane.P2W(vHitPos);
			return true;
		}
	}

	return false;
}

bool CBrushDesignerSpotManager::FindNicestSpot( IDisplayViewport* pViewport, const std::vector<SCandidateInfo>& candidates, const CBrushDesigner* pDesigner, const BrushMatrix34& worldTM, const BrushVec3& pickedPos, CBrushRegion::RegionPtr pPickedRegion, const BrushPlane& plane, SSpot& outSpot ) const
{
	if( !pDesigner )
		return false;

	BrushFloat fMinimumLength(3e10f);

	for( int i = 0; i < candidates.size(); ++i )
	{
		BrushFloat fLength(0);
		if( !BUtil::AreTwoPositionsNear(pickedPos,candidates[i].m_Pos,worldTM,pViewport,kLimitForMagnetic,&fLength) )
			continue;

		if( fLength >= fMinimumLength )
			continue;

		fMinimumLength = fLength;

		outSpot.m_Pos = candidates[i].m_Pos;
		outSpot.m_PosState = candidates[i].m_SpotPosState;
		if( pPickedRegion )
		{
			outSpot.m_pRegion = pPickedRegion;
			outSpot.m_Plane = pPickedRegion->GetPlane();
		}
		else
		{
			outSpot.m_pRegion = NULL;
			outSpot.m_Plane = plane;
		}

		if( outSpot.m_PosState != eSpotPosState_EitherPointOfEdge )
			continue;

		bool bFirstPoint = false;
		if( !pPickedRegion || !pPickedRegion->IsEndPoint(outSpot.m_Pos,&bFirstPoint) )
			continue;

		if( bFirstPoint )
			outSpot.m_PosState = eSpotPosState_FirstPointOfRegion;
		else
			outSpot.m_PosState = eSpotPosState_LastPointOfRegion;
	}

	return true;
}

bool CBrushDesignerSpotManager::FindSnappedSpot( const BrushMatrix34& worldTM, CBrushRegion::RegionPtr pPickedRegion, const BrushVec3& pickedPos, SSpot& outSpot ) const
{
	bool bEnableSnap = IsSnapEnabled();
	if( !bEnableSnap )
		return false;

	if( !pPickedRegion || pPickedRegion->IsOpen() )
		return false;

	BrushPlane pickedPlane(pPickedRegion->GetPlane());
	BrushVec3 snappedPlanePos = Snap(pickedPos);
	outSpot.m_Pos = pickedPlane.P2W(pickedPlane.W2P(snappedPlanePos));
	outSpot.m_pRegion = pPickedRegion;
	outSpot.m_Plane = pPickedRegion->GetPlane();
	return true;
}

bool CBrushDesignerSpotManager::UpdateCurrentSpotPosition( 
	CBrushDesigner* pDesigner, 
	const BrushMatrix34& worldTM, 
	const BrushPlane& plane, 
	IDisplayViewport *view, 
	CPoint point, 
	bool bKeepInitialPlane, 
	bool bSearchAllShelves )
{
	if( pDesigner == NULL )
		return false;
	 
	BrushVec3 localRaySrc, localRayDir;	
	BrushPlane pickedPlane(plane);
	BrushVec3 pickedPos(m_CurrentSpot.m_Pos);
	bool bSuccessQuery(false);
	CBrushRegion::RegionPtr pPickedRegion;
	int nShelf = -1;

	ResetCurrentSpotWeakly();

	BUtil::GetLocalViewRay( worldTM, view, point, localRaySrc, localRayDir );

	if( !bSearchAllShelves )
	{
		if( bKeepInitialPlane )
			bSuccessQuery = pDesigner->QueryPosition( pickedPlane, localRaySrc, localRayDir, pickedPos, NULL, &pPickedRegion );
		else
			bSuccessQuery = pDesigner->QueryPosition( localRaySrc, localRayDir, pickedPos, &pickedPlane, NULL, &pPickedRegion );
	}
	else
	{
		CBrushRegion::RegionPtr pRegions[2] = { NULL, NULL };
		bool bSuccessShelfQuery[2] = { false, false };
		DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);
		for( int i = 0; i < 2; ++i )
		{
			pDesigner->SetShelf(i);
			if( bKeepInitialPlane )
				bSuccessShelfQuery[i] = pDesigner->QueryPosition( pickedPlane, localRaySrc, localRayDir, pickedPos, NULL, &(pRegions[i]) );
			else
				bSuccessShelfQuery[i] = pDesigner->QueryPosition( localRaySrc, localRayDir, pickedPos, &pickedPlane, NULL, &(pRegions[i]) );
		}
		
		BrushFloat ts[2] = { 3e10, 3e10 };
		for( int i = 0; i < 2; ++i )
		{
			if( pRegions[i] )
				pRegions[i]->IsPassed(localRaySrc, localRayDir, ts[i] );
		}

		if( ts[0] < ts[1] )
		{
			pPickedRegion = pRegions[0];
			bSuccessQuery = bSuccessShelfQuery[0];
			nShelf = 0;
		}
		else
		{
			pPickedRegion = pRegions[1];
			bSuccessQuery = bSuccessShelfQuery[1];
			nShelf = 1;
		}
	}

	if( pPickedRegion && pPickedRegion->CheckFlags(CBrushRegion::eRF_Mirrored|CBrushRegion::eRF_Hidden) )
	{
		m_CurrentSpot.m_pRegion = NULL;
		return false;
	}

	if( !bSuccessQuery || bKeepInitialPlane && !pPickedRegion )
	{
		if( !bSuccessQuery )
		{
			if( !GetPosAndPlaneBasedOnWorld(view,point,worldTM,pickedPos,pickedPlane) )
				return false;
		}
		m_CurrentSpot.m_pRegion = NULL;
		m_CurrentSpot.m_Pos = pickedPos;
		m_CurrentSpot.m_PosState = eSpotPosState_OutsideDesigner;
		m_CurrentSpot.m_Plane = pickedPlane;

		SSpot niceSpot(m_CurrentSpot);
		std::vector<SCandidateInfo> candidates;
		if( GetSpotListCount() > 0 )
			candidates.push_back(SCandidateInfo(GetSpot(0).m_Pos,eSpotPosState_AtFirstSpot));
		
		std::vector<CBrushRegion::RegionPtr> penetratedOpenRegions;
		pDesigner->QueryOpenRegions(localRaySrc,localRayDir,penetratedOpenRegions);
		CBrushRegion::RegionPtr pConnectedRegion = NULL;
		for( int i = 0, iCount(penetratedOpenRegions.size()); i < iCount; ++i )
		{
			if( GetSpotListCount() > 0 && !GetSpot(0).m_Plane.IsEquivalent(penetratedOpenRegions[i]->GetPlane(),kDesignerEpsilon) )
				continue;
			std::vector<BrushVec3> vList;
			penetratedOpenRegions[i]->GetLinkedVertices(vList);
			pConnectedRegion = penetratedOpenRegions[i];
			for( int k = 0, iVCount(vList.size()); k < iVCount; ++k )
			{
				if( k == 0 )
					candidates.push_back(SCandidateInfo(vList[k],eSpotPosState_FirstPointOfRegion));
				else if( k == iVCount-1 )
					candidates.push_back(SCandidateInfo(vList[k],eSpotPosState_LastPointOfRegion));
				else
					candidates.push_back(SCandidateInfo(vList[k],eSpotPosState_EitherPointOfEdge));
				if( k < iVCount-1 )
				{
					candidates.push_back(SCandidateInfo((vList[k]+vList[k+1])*0.5f,eSpotPosState_CenterOfEdge));
					BrushEdge3D edge(vList[k],vList[k+1]);
					BrushVec3 posOnEdge;
					bool bInEdge = false;
					if( edge.GetNearestVertex(pickedPos,posOnEdge,bInEdge) && bInEdge && BUtil::AreTwoPositionsNear(pickedPos,posOnEdge,worldTM,view,kLimitForMagnetic) )
					{
						if( !posOnEdge.IsEquivalent(vList[k],kDesignerEpsilon) && !posOnEdge.IsEquivalent(vList[k+1],kDesignerEpsilon) )
							candidates.push_back(SCandidateInfo(posOnEdge,eSpotPosState_Edge));
					}
				}
			}
		}
		if( !candidates.empty() && FindNicestSpot( view, candidates, pDesigner, worldTM, pickedPos, pConnectedRegion, plane, niceSpot ) )
			m_CurrentSpot = niceSpot;
		
		if( !m_CurrentSpot.m_pRegion )
			m_CurrentSpot.m_Pos = Snap(m_CurrentSpot.m_Pos);
		return true;
	}
	
	m_CurrentSpot.m_pRegion = pPickedRegion;

	bool bEnableSnap = IsSnapEnabled();
	if( !m_bEnableMagnetic && !bEnableSnap )
	{
		if( bSuccessQuery )
		{
			m_CurrentSpot.m_Pos = pickedPos;
			m_CurrentSpot.m_Plane = pickedPlane;
		}
		return true;
	}
	
	if( bEnableSnap && pPickedRegion == NULL )
	{
		m_CurrentSpot.m_Plane = pickedPlane;
		m_CurrentSpot.m_Pos = Snap(pickedPos);
		m_CurrentSpot.m_pRegion = NULL;
		return true;
	}

	BrushVec3 nearestEdge[2];
	BrushVec3 posOnEdge;
	std::vector<CBrushDesigner::SQueryEdgeResult> queryResults;
	std::vector<BrushEdge3D> queryEdges;

	{
		DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);
		if( nShelf != -1 )
			pDesigner->SetShelf(nShelf);
		bSuccessQuery = pDesigner->QueryNearestEdges( pickedPlane, localRaySrc, localRayDir, pickedPos, posOnEdge, queryResults );
	}

	if( bKeepInitialPlane && !bSuccessQuery )
	{
		if( !bEnableSnap )
			m_CurrentSpot.m_Pos = pickedPos;
		m_CurrentSpot.m_Plane = pickedPlane;
		return true;
	}

	if( !bSuccessQuery )
		return false;

	for( int i = 0, iQueryResultsCount(queryResults.size()); i < iQueryResultsCount; ++i )
		queryEdges.push_back(queryResults[i].m_Edge);

	if( m_bEnableMagnetic && BUtil::AreTwoPositionsNear(pickedPos,posOnEdge,worldTM,view,kLimitForMagnetic) )
	{
		m_CurrentSpot.m_Pos = posOnEdge;
		m_CurrentSpot.m_PosState = eSpotPosState_Edge;
	}
	else if( !bEnableSnap )
	{
		m_CurrentSpot.m_Pos = pickedPos;
	}
	m_CurrentSpot.m_Plane = pickedPlane;

	int nShortestIndex = BUtil::FindShortestEdge(queryEdges);
	pPickedRegion = queryResults[nShortestIndex].m_pRegion;
	const BrushEdge3D& edge = queryEdges[nShortestIndex];

	SSpot niceSpot(m_CurrentSpot);
	std::vector<SCandidateInfo> candidates;
	candidates.push_back(SCandidateInfo(edge.m_v[0],eSpotPosState_EitherPointOfEdge));
	candidates.push_back(SCandidateInfo(edge.m_v[1],eSpotPosState_EitherPointOfEdge));
	candidates.push_back(SCandidateInfo((edge.m_v[0]+edge.m_v[1])*0.5f,eSpotPosState_CenterOfEdge));
	if( GetSpotListCount() > 0 )
		candidates.push_back(SCandidateInfo(GetSpot(0).m_Pos,eSpotPosState_AtFirstSpot));
	BrushVec3 centerRegionPos(m_CurrentSpot.m_Pos);
	{
		DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);
		if( nShelf != -1 )
			pDesigner->SetShelf(nShelf);
		if( pDesigner->QueryCenterOfRegion(localRaySrc,localRayDir,centerRegionPos) )
			candidates.push_back(SCandidateInfo(centerRegionPos,eSpotPosState_CenterOfRegion));
	}
	if( FindNicestSpot( view, candidates, pDesigner, worldTM, pickedPos, pPickedRegion, plane, niceSpot ) )
		m_CurrentSpot = niceSpot;

	SSpot snappedSpot;
	if( FindSnappedSpot(worldTM,m_CurrentSpot.m_pRegion,pickedPos,snappedSpot) )	
	{
		m_CurrentSpot = snappedSpot;
		queryResults.clear();
		DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);
		pDesigner->SetShelf(0);
		pDesigner->QueryNearestEdges( pickedPlane, m_CurrentSpot.m_Pos, posOnEdge, queryResults );
		if( posOnEdge.IsEquivalent(m_CurrentSpot.m_Pos,kDesignerEpsilon) )
			m_CurrentSpot.m_PosState = eSpotPosState_Edge;
		else if( bSearchAllShelves )
		{
			pDesigner->SetShelf(1);
			pDesigner->QueryNearestEdges( pickedPlane, m_CurrentSpot.m_Pos, posOnEdge, queryResults );
			if( posOnEdge.IsEquivalent(m_CurrentSpot.m_Pos,kDesignerEpsilon) )
				m_CurrentSpot.m_PosState = eSpotPosState_Edge;
		}
		return true;
	}

	SSpot nearSpot(m_CurrentSpot);
	if( FindSpotNearAxisAlignedLine( view, pPickedRegion, worldTM, nearSpot ) )
		m_CurrentSpot = nearSpot;

	return bSuccessQuery;
}

void CBrushDesignerSpotManager::AddSpotToSpotList( const SSpot& spot )
{
	bool bExistSameSpot(false);
	for( int i = 0, iSpotSize(GetSpotListCount()); i < iSpotSize; ++i )
	{
		if( GetSpot(i).IsEquivalentPos(spot) && !GetSpot(i).m_bProcessed )
		{
			bExistSameSpot = true;
			break;
		}
	}

	if( !bExistSameSpot )
		m_SpotList.push_back(spot);
}

void CBrushDesignerSpotManager::ReplaceSpotList( const SpotList& spotList )
{
	m_SpotList = spotList;
}

CBrushDesignerSpotManager::SSpot CBrushDesignerSpotManager::Convert2Spot( CBrushDesigner* pDesigner, const BrushVec3& pos ) const
{
	SSpot spot(pos);

	for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
		if( !pRegion )
			continue;

		if( pRegion->HasVertex(pos) )
		{
			spot.m_PosState = eSpotPosState_EitherPointOfEdge;
			spot.m_pRegion = pRegion;
		}
		else if( pRegion->IsVertexOnCrust(pos) )
		{
			spot.m_PosState = eSpotPosState_Edge;
			spot.m_pRegion = pRegion;
		}
	}

	return spot;
}

bool CBrushDesignerSpotManager::GetPlaneBeginEndPoints( const BrushPlane& plane, BrushVec2& outProjectedStartPt, BrushVec2& outProjectedEndPt ) const
{
	outProjectedStartPt = plane.W2P(GetStartSpotPos());
	outProjectedEndPt = plane.W2P(GetCurrentSpotPos());

	const BrushFloat SmallestRegionSize(0.01f);
	if( std::abs(outProjectedEndPt.x-outProjectedStartPt.x) < SmallestRegionSize || std::abs(outProjectedEndPt.y-outProjectedStartPt.y) < SmallestRegionSize )
		return false;

	const BrushVec2 vecMinimum( -1000.0f, -1000.0f );
	const BrushVec2 vecMaximum( 1000.0f, 1000.0f );

	outProjectedStartPt.x = std::max(outProjectedStartPt.x, vecMinimum.x);
	outProjectedStartPt.y = std::max(outProjectedStartPt.y, vecMinimum.y);
	outProjectedEndPt.x = std::min(outProjectedEndPt.x, vecMaximum.x);
	outProjectedEndPt.y = std::min(outProjectedEndPt.y, vecMaximum.y);

	return true;
}

bool CBrushDesignerSpotManager::GetPosAndPlaneBasedOnWorld( IDisplayViewport* view, const CPoint& point, const BrushMatrix34& worldTM, BrushVec3& outPos, BrushPlane& outPlane )
{
	Vec3 vPickedPos;
	if( !BUtil::PickPosFromWorld(view,point,vPickedPos) )
		return false;
	Matrix34 invTM = worldTM.GetInverted();
	Vec3 p0 = invTM.TransformPoint(vPickedPos);
	outPlane = BrushPlane(BrushVec3(0,0,1),-p0.Dot(BrushVec3(0,0,1)));
	outPos = ToBrushVec3(p0);
	return true;
}

bool CBrushDesignerSpotManager::FindBestPlane( CBrushDesigner* pDesigner, const SSpot& s0, const SSpot& s1, BrushPlane& outPlane )
{
	if( s0.m_pRegion && s0.m_pRegion == s1.m_pRegion )
	{
		outPlane = s0.m_pRegion->GetPlane();
		return true;
	}

	BrushPlane candidatePlanes[] = { s0.m_Plane, s1.m_Plane };
	for( int i = 0, iCandidatePlaneCount(sizeof(candidatePlanes)/sizeof(*candidatePlanes)); i < iCandidatePlaneCount; ++i )
	{
		if( candidatePlanes[i].Normal().IsZero(kDesignerEpsilon) )
			continue;
		BrushFloat d0 = candidatePlanes[i].Distance(s0.m_Pos);
		BrushFloat d1 = candidatePlanes[i].Distance(s1.m_Pos);
		if( std::abs(d0) < kDesignerEpsilon && std::abs(d1) < kDesignerEpsilon )
		{
			outPlane = candidatePlanes[i];
			return true;
		}
	}

	for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
	{
		const BrushPlane& plane = pDesigner->GetRegion(i)->GetPlane();
		BrushFloat d0 = plane.Distance(s0.m_Pos);
		BrushFloat d1 = plane.Distance(s1.m_Pos);
		if( std::abs(d0) < kDesignerEpsilon && std::abs(d1) < kDesignerEpsilon )
		{
			outPlane = plane;
			return true;
		}
	}

	DESIGNER_ASSERT(0);
	return false;
}

bool CBrushDesignerSpotManager::IsSnapEnabled() const
{
	if( !m_bBuiltInSnap )
		return GetIEditor()->GetViewManager()->GetGrid()->IsEnabled() | m_bBuiltInSnap;
	return true;
}

BrushVec3 CBrushDesignerSpotManager::Snap( const BrushVec3& vPos ) const
{
	if( !m_bBuiltInSnap )
		return GetIEditor()->GetViewManager()->GetGrid()->Snap(vPos);

	BrushVec3 snapped;
	snapped.x = std::floor((vPos.x/m_BuiltInSnapSize)+(BrushFloat)0.5)*m_BuiltInSnapSize;
	snapped.y = std::floor((vPos.y/m_BuiltInSnapSize)+(BrushFloat)0.5)*m_BuiltInSnapSize;
	snapped.z = std::floor((vPos.z/m_BuiltInSnapSize)+(BrushFloat)0.5)*m_BuiltInSnapSize;

	return snapped;
}