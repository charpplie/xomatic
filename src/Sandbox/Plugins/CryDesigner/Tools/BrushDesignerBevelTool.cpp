#include "StdAfx.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerEditTool.h"
#include "BrushDesignerBevelTool.h"

int GetDifferentVerticesCount( const std::vector<BrushVec3>& vList )
{
	std::set<BrushVec3> vSet;
	for( int i = 0, iSize(vList.size()); i < iSize; ++i )
		vSet.insert(vList[i]);
	return vSet.size();
}

void CBrushDesignerBevelTool::Enter()
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( pSelected->IsEmpty() )
	{
		GetEditTool()->GoToPrevDesignerMode();
		return;
	}

	m_BevelMode = eBevelMode_Nothing;
	m_nMousePrevY = 0;
	m_fDelta = 0;

	if( m_BevelMode == eBevelMode_Nothing )
	{
		GetIEditor()->BeginUndo();
		GetDesigner()->RecordUndo("Designer : Bevel",GetBaseObject());
		PP0_Initialize();
	}
}

void CBrushDesignerBevelTool::Leave()
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->MoveShelf(1,0);
	GetDesigner()->SetShelf(1);
	GetDesigner()->Clear();
	GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
	m_BevelMode = eBevelMode_Nothing;
	GetIEditor()->AcceptUndo("Designer : Bevel");
	UpdateBrush();
}

bool CBrushDesignerBevelTool::PP0_Initialize( bool bSpreadEdge )
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( pSelected->IsEmpty() )
		return false;

	if( !m_pOriginalDesigner )
		m_pOriginalDesigner = new CBrushDesigner;
	(*m_pOriginalDesigner) = *GetDesigner();

	CBrushDesignerDB::QueryResult queryResult;
	OrganizedQueryResults organizedQueryResult;

	m_OriginalRegions.clear();
	m_OriginalSelectedElements.Clear();

	for( int i = 0, iElementSize(pSelected->GetSize()); i < iElementSize; ++i )
	{		
		if( (*pSelected)[i].IsEdge() )
		{
			if( m_OriginalSelectedElements.Has((*pSelected)[i]) )
				continue;
			m_OriginalSelectedElements.Add((*pSelected)[i]);
			for( int k = 0, iVertexSize((*pSelected)[i].m_Vertices.size()); k < iVertexSize; ++k )
				GetDesigner()->GetDB()->QueryAsVertex((*pSelected)[i].m_Vertices[k],queryResult);
		}
		else if( (*pSelected)[i].IsFace() )
		{
			for( int k = 0, iEdgeSize((*pSelected)[i].m_pRegion->GetEdgeSize()); k < iEdgeSize; ++k )
			{
				BrushEdge3D edge = (*pSelected)[i].m_pRegion->GetEdge(k);
				SDesignerElement elementInfo;
				elementInfo.m_Vertices.push_back(edge.m_v[0]);
				elementInfo.m_Vertices.push_back(edge.m_v[1]);
				elementInfo.m_pObject = (*pSelected)[i].m_pObject;
				elementInfo.m_pRegion = NULL;
				if( m_OriginalSelectedElements.Has(elementInfo) )
					continue;
				m_OriginalSelectedElements.Add(elementInfo);
				GetDesigner()->GetDB()->QueryAsVertex(edge.m_v[0],queryResult);
				GetDesigner()->GetDB()->QueryAsVertex(edge.m_v[1],queryResult);
			}
		}
	}

	if( m_OriginalSelectedElements.IsEmpty() )
		return false;

	pSelected->Clear();

	GetDesigner()->SetShelf(0);
	organizedQueryResult = CBrushDesignerSelectTool::CreateOrganizedResultsAroundRegionFromQueryResults(queryResult);

	std::vector<CBrushRegion::RegionPtr> removedRegions;
	OrganizedQueryResults::iterator ii = organizedQueryResult.begin();
	for( ; ii != organizedQueryResult.end(); ++ii )
	{
		CBrushRegion::RegionPtr pRegion = ii->first;
		removedRegions.push_back(pRegion);
		m_OriginalRegions.push_back(pRegion);
	}

	for( int i = 0, iRemovedRegionSize(removedRegions.size()); i < iRemovedRegionSize; ++i )
	{
		GetDesigner()->RemoveRegion(removedRegions[i]);
		RemoveMirroredRegion(GetDesigner(), removedRegions[i]);
	}

	PP0_SpreadEdges(0,bSpreadEdge);
	UpdateBrush();

	m_BevelMode = eBevelMode_Spread;

	return true;
}

void CBrushDesignerBevelTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	m_nMousePrevY = point.y;

	if( m_BevelMode == eBevelMode_Spread )
	{
		DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
		GetDesigner()->SetShelf(1);

		for( int i = 0, iRegionSize(m_ResultForSecondPhase.middlePhaseEdgeRegions.size()); i < iRegionSize; ++i )
			GetDesigner()->RemoveRegion(m_ResultForSecondPhase.middlePhaseEdgeRegions[i]);

		for( int i = 0, iRegionSize(m_ResultForSecondPhase.middlePhaseSideRegions.size()); i < iRegionSize; ++i )
			GetDesigner()->RemoveRegion(m_ResultForSecondPhase.middlePhaseSideRegions[i]);

		for( int i = 0, iRegionSize(m_ResultForSecondPhase.middlePhaseBottomRegions.size()); i < iRegionSize; ++i )
			GetDesigner()->RemoveRegion(m_ResultForSecondPhase.middlePhaseBottomRegions[i]);

		GetDesigner()->MoveShelf(1,0);

		for( int i = 0, iRegionSize(m_ResultForSecondPhase.middlePhaseEdgeRegions.size()); i < iRegionSize; ++i )
			GetDesigner()->AddRegion(m_ResultForSecondPhase.middlePhaseEdgeRegions[i],CBrushDesigner::eOpType_Add);

		for( int i = 0, iRegionSize(m_ResultForSecondPhase.middlePhaseSideRegions.size()); i < iRegionSize; ++i )
		{
			if( GetDesigner()->QueryEquivalentRegion(m_ResultForSecondPhase.middlePhaseSideRegions[i]) )
				continue;
			GetDesigner()->AddRegion(m_ResultForSecondPhase.middlePhaseSideRegions[i],CBrushDesigner::eOpType_Add);
		}

		for( int i = 0, iRegionSize(m_ResultForSecondPhase.middlePhaseBottomRegions.size()); i < iRegionSize; ++i )
			GetDesigner()->AddRegion(m_ResultForSecondPhase.middlePhaseBottomRegions[i],CBrushDesigner::eOpType_Add);

		UpdateBrush();

		m_nDividedNumber = 0;

		m_BevelMode = eBevelMode_Divide;
	}
	else if( m_BevelMode == eBevelMode_Divide )
	{
		GetDesigner()->MoveShelf(1,0);
		GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
		m_BevelMode = eBevelMode_Nothing;
		GetIEditor()->AcceptUndo("Designer : Bevel");
		UpdateBrush();
		Sync();
		UpdateGameResource(GetBaseObject());
		m_fDelta = 0;
	}
}

bool AreAllCorrespondingEdgesInSameDirection( CBrushRegion::RegionPtr pRegion0, CBrushRegion::RegionPtr pRegion1 )
{
	int nEdgeCount0 = pRegion0->GetVertexListSize();
	int nEdgeCount1 = pRegion1->GetVertexListSize();

	if( nEdgeCount0 != nEdgeCount1 )
		return false;

	for( int i = 0; i < nEdgeCount0; ++i )
	{
		BrushEdge3D e0 = pRegion0->GetEdge(i);
		BrushEdge3D e1 = pRegion1->GetEdge(i);

		BrushVec3 vDir0 = (e0.m_v[1]-e0.m_v[0]).GetNormalized();
		BrushVec3 vDir1 = (e1.m_v[1]-e1.m_v[0]).GetNormalized();

		if( vDir0.Dot(vDir1) < 0 )
			return false;
	}

	return true;
}

bool HasCrossEdges( CBrushRegion::RegionPtr pRegion )
{
	if( pRegion->IsOpen() )
		return true;

	std::vector<BrushVec3> vList;
	pRegion->GetLinkedVertices(vList);

	std::vector<BrushEdge> eList;
	for( int i = 0, iVertexCount(vList.size()); i < iVertexCount; ++i )
		eList.push_back(BrushEdge(pRegion->GetPlane().W2P(vList[i]),pRegion->GetPlane().W2P(vList[(i+1)%iVertexCount])));

	for( int i = 0, iEdgeCount(eList.size()); i < iEdgeCount; ++i )
	{
		const BrushEdge& e0 = eList[i];
		for( int k = 0; k < iEdgeCount; ++k )
		{
			if( i == k || i == (k+1)%iEdgeCount || (i+1)%iEdgeCount == k )
				continue;

			const BrushEdge& e1 = eList[k];

			if( e0.IsIntersect(e1,kDesignerEpsilon) )
				return true;
		}
	}

	return false;
}

bool IsRegionValid( CBrushRegion::RegionPtr pRegion, CBrushRegion::RegionPtr pOriginalRegion )
{
	if( pOriginalRegion->HasHoles() )
	{
		std::vector<CBrushRegion::RegionPtr> innerRegions;
		pRegion->GetSeparatedRegions(innerRegions,CBrushRegion::eSR_InnerHull);

		std::vector<CBrushRegion::RegionPtr> outerRegions;
		pRegion->GetSeparatedRegions(outerRegions,CBrushRegion::eSR_OuterHull);

		if( outerRegions.size() != 1 )
			return false;

		for( int k = 0, iInnterRegionCount(innerRegions.size()); k < iInnterRegionCount; ++k )
		{	
			innerRegions[k]->ReverseEdges();
			if( !outerRegions[0]->Include(innerRegions[k]) )
				return false;
		}

		if( !outerRegions[0]->IsCCW() )
			return false;
	}
	else
	{
		for( int k = 0, iEdgeCount(pRegion->GetEdgeSize()); k < iEdgeCount; ++k )
		{
			BrushEdge3D e = pRegion->GetEdge(k);
			if( e.GetLength() < kDesignerLooseEpsilon )
				return false;
		}
		if( !pRegion->IsCCW() || !AreAllCorrespondingEdgesInSameDirection(pRegion,pOriginalRegion) )
			return false;
	}

	return true;
}

bool CBrushDesignerBevelTool::PP1_PushEdgesAndVerticesOut( SResultForNextPhase& outResultForNextPhase, SMappingInfo& outMappingInfo )
{
	std::vector<int> vertices;
	std::vector<int> selectedElements;

	std::vector<CBrushRegion::RegionPtr> updatedRegions;

	for( int i = 0, iRegionSize(m_OriginalRegions.size()); i < iRegionSize; ++i )
	{
		CBrushRegion::RegionPtr pOriginalRegion = m_OriginalRegions[i];
		CBrushRegion::RegionPtr pRegion = pOriginalRegion->Clone();
		updatedRegions.push_back(pRegion);

		std::set<int> edgeSetMatchingSelectedElement;
		std::map<int,int> mapBetweenEdgeIndexAndElement;

		for( int k = 0, iElementSize(m_OriginalSelectedElements.GetSize()); k < iElementSize; ++k )
		{
			BrushEdge3D edge(m_OriginalSelectedElements[k].m_Vertices[0],m_OriginalSelectedElements[k].m_Vertices[1]);
			int nEdgeIndex = -1;
			if( !pRegion->Exist(edge,true,&nEdgeIndex) )
				continue;
			edgeSetMatchingSelectedElement.insert(nEdgeIndex);
			mapBetweenEdgeIndexAndElement[nEdgeIndex] = k;
		}

		if( !edgeSetMatchingSelectedElement.empty() )
		{
			std::set<int>::iterator iEdgeMatchingSelectedElement;
			for( iEdgeMatchingSelectedElement = edgeSetMatchingSelectedElement.begin(); iEdgeMatchingSelectedElement != edgeSetMatchingSelectedElement.end(); ++iEdgeMatchingSelectedElement )
			{
				int nPrevEdgeIndex = -1;
				int nNextEdgeIndex = -1;
				if( m_OriginalRegions[i]->GetAdjacentEdgesByEdgeIndex(*iEdgeMatchingSelectedElement,&nPrevEdgeIndex,&nNextEdgeIndex) )
				{
					BrushEdge3D originalEdge = m_OriginalRegions[i]->GetEdge(*iEdgeMatchingSelectedElement);
					const BUtil::SEdge& originalEdgeIndexPair = m_OriginalRegions[i]->GetEdgeIndexPair(*iEdgeMatchingSelectedElement);

					BrushVec3 vUpdatedPos[2] = { originalEdge.m_v[0], originalEdge.m_v[1] };
					BrushVec3 vOriginalEdgeDir = (originalEdge.m_v[1]-originalEdge.m_v[0]).GetNormalized();

					BrushEdge3D prevEdge = m_OriginalRegions[i]->GetEdge(nPrevEdgeIndex);
					BrushVec3 vPrevEdgeDir = (prevEdge.m_v[0]-prevEdge.m_v[1]).GetNormalized();

					if( edgeSetMatchingSelectedElement.find(nPrevEdgeIndex) == edgeSetMatchingSelectedElement.end() )
					{
						if( GetEdgeCountHavingVertexInElementList(prevEdge.m_v[1],m_OriginalSelectedElements) == 1 )
						{
							BrushFloat fTheta = std::acos(vOriginalEdgeDir.Dot(-vPrevEdgeDir));
							vUpdatedPos[0] = prevEdge.m_v[1] + vPrevEdgeDir*(m_fDelta/std::sin(fTheta));
						}
						else
						{
							vUpdatedPos[0] = prevEdge.m_v[1] + vPrevEdgeDir*m_fDelta;
						}

						pRegion->SetVertex(originalEdgeIndexPair.m_i[0], vUpdatedPos[0]);
					}
					else
					{
						BrushVec3 vInclinedDir = (vOriginalEdgeDir+vPrevEdgeDir).GetNormalized();
						BrushFloat fTheta = std::acos(vOriginalEdgeDir.Dot(vInclinedDir));
						if( vOriginalEdgeDir.Cross(vPrevEdgeDir).Dot(pRegion->GetPlane().Normal()) < 0 )
							vInclinedDir = -vInclinedDir;
						vUpdatedPos[0] = prevEdge.m_v[1] + vInclinedDir*(m_fDelta/std::sin(fTheta));
						pRegion->SetVertex(originalEdgeIndexPair.m_i[0], vUpdatedPos[0]);
					}

					BrushEdge3D nextEdge = m_OriginalRegions[i]->GetEdge(nNextEdgeIndex);
					if( edgeSetMatchingSelectedElement.find(nNextEdgeIndex) == edgeSetMatchingSelectedElement.end() )
					{
						BrushVec3 vNextEdgeDir = (nextEdge.m_v[1]-nextEdge.m_v[0]).GetNormalized();
						if( GetEdgeCountHavingVertexInElementList(nextEdge.m_v[0],m_OriginalSelectedElements) == 1 )
						{
							BrushFloat fTheta = std::acos(vNextEdgeDir.Dot(vOriginalEdgeDir));
							vUpdatedPos[1] = nextEdge.m_v[0]+vNextEdgeDir*(m_fDelta/std::sin(fTheta));
						}
						else
						{
							vUpdatedPos[1] = nextEdge.m_v[0]+vNextEdgeDir*m_fDelta;
						}
						pRegion->SetVertex(originalEdgeIndexPair.m_i[1], vUpdatedPos[1]);
					}

					for( int k = 0, iElementSize(m_OriginalSelectedElements.GetSize()); k < iElementSize; ++k )
					{
						if( originalEdge.m_v[0].IsEquivalent(m_OriginalSelectedElements[k].m_Vertices[0],kDesignerEpsilon) )
							outMappingInfo.mapSpreadedVertex2Apex.push_back(std::pair<BrushVec3,BrushVec3>(vUpdatedPos[0],m_OriginalSelectedElements[k].m_Vertices[0]));
						else if( originalEdge.m_v[1].IsEquivalent(m_OriginalSelectedElements[k].m_Vertices[0],kDesignerEpsilon) )
							outMappingInfo.mapSpreadedVertex2Apex.push_back(std::pair<BrushVec3,BrushVec3>(vUpdatedPos[1],m_OriginalSelectedElements[k].m_Vertices[0]));

						if( originalEdge.m_v[1].IsEquivalent(m_OriginalSelectedElements[k].m_Vertices[1],kDesignerEpsilon) )
							outMappingInfo.mapSpreadedVertex2Apex.push_back(std::pair<BrushVec3,BrushVec3>(vUpdatedPos[1],m_OriginalSelectedElements[k].m_Vertices[1]));
						else if( originalEdge.m_v[0].IsEquivalent(m_OriginalSelectedElements[k].m_Vertices[1],kDesignerEpsilon) )
							outMappingInfo.mapSpreadedVertex2Apex.push_back(std::pair<BrushVec3,BrushVec3>(vUpdatedPos[0],m_OriginalSelectedElements[k].m_Vertices[1]));
					}
				}
			}

			for( iEdgeMatchingSelectedElement = edgeSetMatchingSelectedElement.begin(); iEdgeMatchingSelectedElement != edgeSetMatchingSelectedElement.end(); ++iEdgeMatchingSelectedElement )
			{
				BrushEdge3D edgeMatchingSelectedElement = pRegion->GetEdge(*iEdgeMatchingSelectedElement);
				outMappingInfo.mapElementIdx2Edges[mapBetweenEdgeIndexAndElement[*iEdgeMatchingSelectedElement]].push_back(edgeMatchingSelectedElement.GetInverted());
				outMappingInfo.mapElementIdx2OriginalRegion[mapBetweenEdgeIndexAndElement[*iEdgeMatchingSelectedElement]] = m_OriginalRegions[i];
			}

			pRegion->Optimize();

			if( !IsRegionValid(pRegion,pOriginalRegion) )
				return false;
		}

		for( int k = 0, iElementSize(m_OriginalSelectedElements.GetSize()); k < iElementSize; ++k )
		{
			BrushEdge3D edge(m_OriginalSelectedElements[k].m_Vertices[0],m_OriginalSelectedElements[k].m_Vertices[1]);
			int nVertexIndex = -1;
			bool bSuccess0 = pRegion->Exist(edge.m_v[0],kDesignerEpsilon,&nVertexIndex);
			bool bSuccess1 = pRegion->Exist(edge.m_v[1],kDesignerEpsilon,&nVertexIndex);
			if( bSuccess0 == bSuccess1 )
				continue;
			vertices.push_back(nVertexIndex);
			selectedElements.push_back(k);
			outResultForNextPhase.middlePhaseSideRegions.push_back(pRegion);
		}		
	}

	for( int i = 0, iSize(outResultForNextPhase.middlePhaseSideRegions.size()); i < iSize; ++i )
	{
		CBrushRegion::RegionPtr pRegion = outResultForNextPhase.middlePhaseSideRegions[i];

		bool bTouched = false;
		for( int k = 0; k < iSize; ++k )
		{
			if( i == k || pRegion == outResultForNextPhase.middlePhaseSideRegions[k] || selectedElements[i] != selectedElements[k] )
				continue;

			if( pRegion->HasOverlappedEdges(outResultForNextPhase.middlePhaseSideRegions[k]) )
			{
				bTouched = true;
				break;
			}
		}

		if( !bTouched )
		{
			if( GetEdgeCountHavingVertexInElementList(pRegion->GetVertex(vertices[i]), m_OriginalSelectedElements) == 1 )
			{
				BrushEdge3D baseEdge(m_OriginalSelectedElements[selectedElements[i]].m_Vertices[0],m_OriginalSelectedElements[selectedElements[i]].m_Vertices[1]);
				pRegion->BroadenVertex(m_fDelta,vertices[i],&baseEdge);
			}
			else
			{
				pRegion->BroadenVertex(m_fDelta,vertices[i],NULL);
			}
		}
		else
		{
			outMappingInfo.vertexSetToMakeRegion.insert(pRegion->GetVertex(vertices[i]));
		}

		if( HasCrossEdges(pRegion) )
			return false;
	}

	for( int i = 0, iUpdatedRegionCount(updatedRegions.size()); i < iUpdatedRegionCount; ++i )
		GetDesigner()->AddRegion(updatedRegions[i],CBrushDesigner::eOpType_Add);

	return true;
}

void CBrushDesignerBevelTool::PP1_MakeEdgeRegions( const SMappingInfo& mappingInfo, SResultForNextPhase& outResultForNextPhase )
{
	MapBetweenElementIndexAndEdges::const_iterator ii = mappingInfo.mapElementIdx2Edges.begin();
	for( ; ii != mappingInfo.mapElementIdx2Edges.end(); ++ii )
	{
		std::vector<BrushEdge3D> edges = ii->second;
		if( edges.size() == 2 )
		{
			edges.push_back(BrushEdge3D(edges[0].m_v[1],edges[1].m_v[0]));
			edges.push_back(BrushEdge3D(edges[1].m_v[1],edges[0].m_v[0]));
		}

		if( edges.size() != 4 ) 
			continue;

		std::set<int> usedEdges;
		std::vector<BrushVec3> vList;

		int nIndex = 0;
		while(vList.size()<4)
		{
			bool bFound = false;
			for( int i = 0; i < 4; ++i )
			{
				if( i == nIndex )
					continue;
				if( edges[nIndex].m_v[1].IsEquivalent(edges[i].m_v[0],kDesignerEpsilon) )
				{
					nIndex = i;
					vList.push_back(edges[i].m_v[0]);
					bFound = true;
					break;
				}
			}
			if( !bFound )
			{
				DESIGNER_ASSERT(0);
				vList.clear();
				break;
			}
		}
		if( vList.size() == 4 )
		{
			if( GetDifferentVerticesCount(vList) == 4 )
			{
				BrushPlane plane(vList[0],vList[1],vList[2],kDesignerEpsilon);
				CBrushRegion::RegionPtr pEdgeRegion = new CBrushRegion(vList,plane,0,NULL,true);
				MapBetweenElementIndexAndOrignialRegion::const_iterator iOriginalRegion = mappingInfo.mapElementIdx2OriginalRegion.find(ii->first);
				if( iOriginalRegion != mappingInfo.mapElementIdx2OriginalRegion.end() )
				{
					pEdgeRegion->SetTexInfo(iOriginalRegion->second->GetTexInfo());
					pEdgeRegion->SetMaterialID(iOriginalRegion->second->GetMaterialID());
				}
				outResultForNextPhase.middlePhaseEdgeRegions.push_back(pEdgeRegion);
				GetDesigner()->AddRegion(pEdgeRegion, CBrushDesigner::eOpType_Add);

				PP2_MapBetweenEdgeIdToApexPos(mappingInfo,pEdgeRegion,edges[2],edges[3],outResultForNextPhase);
			}
		}
	}
}

void CBrushDesignerBevelTool::PP2_MapBetweenEdgeIdToApexPos( const SMappingInfo& mappingInfo,
	CBrushRegion::RegionPtr pEdgeRegion, 
	const BrushEdge3D& sideEdge0, 
	const BrushEdge3D& sideEdge1, 
	SResultForNextPhase& outResultForNextPhase )
{
	for( int a = 0, iEdgeSize(pEdgeRegion->GetEdgeSize()); a < iEdgeSize; ++a )
	{
		BrushEdge3D edge = pEdgeRegion->GetEdge(a);
		if( !sideEdge0.IsEquivalent(edge,kDesignerEpsilon) && !sideEdge1.IsEquivalent(edge,kDesignerEpsilon) )
			continue;

		const std::pair<BrushVec3,BrushVec3>* iApex = NULL;
		for( int b = 0 ,iVertex2ApexCount(mappingInfo.mapSpreadedVertex2Apex.size()); b < iVertex2ApexCount; ++b )
		{
			if( mappingInfo.mapSpreadedVertex2Apex[b].first.IsEquivalent(edge.m_v[0],kDesignerEpsilon) )
			{
				iApex = &mappingInfo.mapSpreadedVertex2Apex[b];
				break;
			}
		}

		if( iApex == NULL )
		{
			DESIGNER_ASSERT(0 && "iApex == NULL");
			continue;
		}

		BrushVec3 apexPos = iApex->second;
		BrushFloat fDistance = 0;

		int nCount = GetEdgeCountHavingVertexInElementList(apexPos,m_OriginalSelectedElements);
		if( nCount < 3 )
		{
			outResultForNextPhase.mapBetweenEdgeIdToApex[EdgeIdentifier(pEdgeRegion,a)] = apexPos;
			if( mappingInfo.vertexSetToMakeRegion.find(apexPos) != mappingInfo.vertexSetToMakeRegion.end() )
			{
				std::vector<BrushVec3> vList;
				vList.push_back(apexPos);
				vList.push_back(edge.m_v[1]);
				vList.push_back(edge.m_v[0]);
				BrushPlane plane(vList[0],vList[1],vList[2],kDesignerEpsilon);
				CBrushRegion::RegionPtr pApexRegion = new CBrushRegion(vList,plane,pEdgeRegion->GetMaterialID(),&(pEdgeRegion->GetTexInfo()),true);
				GetDesigner()->AddRegion(pApexRegion, CBrushDesigner::eOpType_Add);
				outResultForNextPhase.middlePhaseBottomRegions.push_back(pApexRegion);

				outResultForNextPhase.mapBetweenEdgeIdToVertex[EdgeIdentifier(pEdgeRegion,a)] = apexPos;
			}
		}
		else if( pEdgeRegion->GetPlane().HitTest(
			apexPos,
			apexPos-pEdgeRegion->GetPlane().Normal(),
			kDesignerEpsilon,&fDistance, NULL) )
		{
			outResultForNextPhase.mapBetweenEdgeIdToApex[EdgeIdentifier(pEdgeRegion,a)] = edge.GetCenter() + pEdgeRegion->GetPlane().Normal()*fDistance;
		}
	}
}

void CBrushDesignerBevelTool::PP1_MakeApexRegions( const SMappingInfo& mappingInfo, SResultForNextPhase& outResultForNextPhase )
{
	std::vector<BrushEdge3D> apexEdges;
	MapBetweenElementIndexAndEdges::const_iterator ii = mappingInfo.mapElementIdx2Edges.begin();
	std::vector<CBrushRegion::RegionPtr> corrspondingRegions;
	for( ; ii != mappingInfo.mapElementIdx2Edges.end(); ++ii )
	{
		std::vector<BrushEdge3D> edges = ii->second;
		if( edges.size() == 2 )
		{
			apexEdges.push_back(BrushEdge3D(edges[0].m_v[0],edges[1].m_v[1]));
			apexEdges.push_back(BrushEdge3D(edges[1].m_v[0],edges[0].m_v[1]));
			MapBetweenElementIndexAndOrignialRegion::const_iterator iOriginalRegion = mappingInfo.mapElementIdx2OriginalRegion.find(ii->first);
			if( iOriginalRegion != mappingInfo.mapElementIdx2OriginalRegion.end() )
			{
				corrspondingRegions.push_back(iOriginalRegion->second);
				corrspondingRegions.push_back(iOriginalRegion->second);
			}
		}
	}

	if( apexEdges.size() < 3 )
		return;

	std::set<int> usedEdges;
	int nIndex = -1;
	std::vector<BrushVec3> vList;
	bool bFoundNextEdge = false;
	bool bFoundApexRegion = false;
	int iApexEdgeSize(apexEdges.size());

	while( usedEdges.size() < iApexEdgeSize || usedEdges.size() == iApexEdgeSize && bFoundApexRegion )
	{
		if( bFoundApexRegion )
		{
			DESIGNER_ASSERT( vList.size() >= 3 );
			if( GetDifferentVerticesCount(vList) >= 3 )
			{
				BrushPlane plane(vList[0],vList[1],vList[2],kDesignerEpsilon);
				CBrushRegion::RegionPtr pApexRegion = new CBrushRegion(vList,plane,0,NULL,true);
				if( nIndex != -1 )
				{
					pApexRegion->SetTexInfo(corrspondingRegions[nIndex]->GetTexInfo());
					pApexRegion->SetMaterialID(corrspondingRegions[nIndex]->GetMaterialID());
				}
				outResultForNextPhase.middlePhaseApexRegions.push_back(pApexRegion);
				GetDesigner()->AddRegion(pApexRegion, CBrushDesigner::eOpType_Add);
				vList.clear();
				nIndex = -1;
				if( usedEdges.size() == iApexEdgeSize )
					break;
			}
			else
			{
				break;
			}
		}
		else if( !bFoundNextEdge )
		{
			vList.clear();
			if( nIndex != -1 )
				usedEdges.insert(nIndex);
			nIndex = -1;
		}

		if( nIndex == -1 )
		{
			for( int i = 0; i < iApexEdgeSize; ++i )
			{
				if( usedEdges.find(i) == usedEdges.end() )
				{
					nIndex = i;
					break;
				}
			}
		}

		if( nIndex == -1 )
			break;

		vList.push_back(apexEdges[nIndex].m_v[0]);

		bFoundNextEdge = false;
		bFoundApexRegion = false;

		for( int i = 0, iApexEdgeSize(apexEdges.size()); i < iApexEdgeSize; ++i )
		{
			if( i == nIndex )
				continue;

			bool bEdgeConnected = apexEdges[nIndex].m_v[1].IsEquivalent(apexEdges[i].m_v[0],kDesignerEpsilon);
			if( !bEdgeConnected )
				continue;

			if( vList.size() >= 3 && vList[0].IsEquivalent(apexEdges[i].m_v[0],kDesignerEpsilon) )
			{
				usedEdges.insert(nIndex);
				bFoundApexRegion = true;
				break;
			}

			if( usedEdges.find(i) != usedEdges.end() )
				continue;

			usedEdges.insert(nIndex);
			nIndex = i;
			bFoundNextEdge = true;
			break;
		}
	}
}

void CBrushDesignerBevelTool::PP0_SpreadEdges( int offset, bool bSpreadEdge )
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	BrushFloat fPrevDelta = m_fDelta;
	if( bSpreadEdge )
	{
		m_fDelta -= (float)offset * 0.01f;
		if( m_fDelta <= 0 )
			m_fDelta = 0;
	}
	else
	{
		m_fDelta = 0;
	}

	GetDesigner()->SetShelf(1);
	GetDesigner()->Clear();

	m_ResultForSecondPhase.Reset();
	SMappingInfo mappingInfo;
	int nCount = 0;
	while( !PP1_PushEdgesAndVerticesOut(m_ResultForSecondPhase,mappingInfo) && ++nCount < 100 )
	{
		m_fDelta = (m_fDelta+fPrevDelta) * (BrushFloat)0.5;
		mappingInfo.Reset();
		m_ResultForSecondPhase.Reset();
	}

	DESIGNER_ASSERT(nCount<100);

	PP1_MakeEdgeRegions(mappingInfo, m_ResultForSecondPhase);
	PP1_MakeApexRegions(mappingInfo, m_ResultForSecondPhase);

	CreateMirroredRegions(GetDesigner());
	UpdateShelf(1);
}

void CBrushDesignerBevelTool::PP0_SubdivideSpreadedEdge( int nSubdivideNum )
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	GetDesigner()->SetShelf(1);
	GetDesigner()->Clear();

	SResultForNextPhase& rp = m_ResultForSecondPhase;

	if( nSubdivideNum == 1 )
	{
		for( int i = 0, iSize(rp.middlePhaseEdgeRegions.size()); i < iSize; ++i )
			GetDesigner()->AddRegion(rp.middlePhaseEdgeRegions[i],CBrushDesigner::eOpType_Add);

		for( int i = 0, iSideRegionSize(rp.middlePhaseSideRegions.size()); i < iSideRegionSize; ++i )
		{
			if( GetDesigner()->QueryEquivalentRegion(rp.middlePhaseSideRegions[i]) )
				continue;
			GetDesigner()->AddRegion(rp.middlePhaseSideRegions[i],CBrushDesigner::eOpType_Add);
		}

		for( int i = 0, iFlatRegionSize(rp.middlePhaseBottomRegions.size()); i < iFlatRegionSize; ++i )
			GetDesigner()->AddRegion(rp.middlePhaseBottomRegions[i],CBrushDesigner::eOpType_Add);

		CreateMirroredRegions(GetDesigner());
		UpdateShelf(1);
		return;
	}

	std::vector<SInfoForSubdivingApexRegion> infoForSubdividingApexRegionList;

	std::vector<CBrushRegion::RegionPtr> middleSideRegions;
	for( int i = 0, iMiddleSideRegionSize(rp.middlePhaseSideRegions.size()); i < iMiddleSideRegionSize; ++i )
	{
		if( GetDesigner()->QueryEquivalentRegion(rp.middlePhaseSideRegions[i]) )
			continue;
		CBrushRegion::RegionPtr pMiddleSideRegion = rp.middlePhaseSideRegions[i]->Clone();
		middleSideRegions.push_back(pMiddleSideRegion);
		GetDesigner()->AddRegion(pMiddleSideRegion,CBrushDesigner::eOpType_Add);
	}

	for( int k = 0, iRegionSize(rp.middlePhaseEdgeRegions.size()); k < iRegionSize; ++k )
	{
		std::vector<BrushVec2> sidePoints[2];
		std::vector<BrushVec3> sideVertices[2];
		int nCount = 0;
		CBrushRegion::RegionPtr pEdgeRegion = rp.middlePhaseEdgeRegions[k];
		for( int i = 0; i < 4; ++i )
		{
			EdgeIdentifier edgeId(pEdgeRegion,i);
			if( rp.mapBetweenEdgeIdToApex.find(edgeId) == rp.mapBetweenEdgeIdToApex.end() )
				continue;

			DESIGNER_ASSERT(nCount < 2);

			int nEdgeIndex = -1;
			BrushEdge3D sideEdge = pEdgeRegion->GetEdge(i);

			BrushVec3 vApexPos = rp.mapBetweenEdgeIdToApex[edgeId];
			BrushPlane sidePlane(sideEdge.m_v[0], sideEdge.m_v[1], vApexPos, kDesignerEpsilon);

			if( nCount == 1 )
				sideEdge.Invert();

			BrushVec3 vDir = (vApexPos-sideEdge.GetCenter()).GetNormalized();
			CBrushRegion::RegionPtr pSideRegion;
			for( int a = 0, iSideRegionSize(middleSideRegions.size()); a < iSideRegionSize; ++a )
			{
				if( middleSideRegions[a]->GetPlane().IsEquivalent(sidePlane,kDesignerEpsilon) || middleSideRegions[a]->GetPlane().IsEquivalent(sidePlane.GetInverted(),kDesignerEpsilon) )
				{
					pSideRegion = middleSideRegions[a];
					break;
				}
			}

			vApexPos = sideEdge.GetCenter() + 0.35f*(vApexPos-sideEdge.GetCenter());

			BrushVec2 vOutsideVtx = sidePlane.W2P(vApexPos);
			BrushVec2 vBaseVtx0 = sidePlane.W2P(sideEdge.m_v[0]);
			BrushVec2 vBaseVtx1 = sidePlane.W2P(sideEdge.m_v[1]);
			if( MakeListConsistingOfArc(vOutsideVtx,vBaseVtx0,vBaseVtx1,nSubdivideNum+1,sidePoints[nCount]) )
			{
				sideVertices[nCount].push_back(sideEdge.m_v[1]);
				for( int a = 1, iSize(sidePoints[nCount].size()); a < iSize; ++a )
				{
					BrushVec3 vertex = sidePlane.P2W(sidePoints[nCount][a]);
					sideVertices[nCount].push_back(vertex);
					if( pSideRegion )
						pSideRegion->AddVertex(vertex);
				}
				sideVertices[nCount][sideVertices[nCount].size()-1] = sideEdge.m_v[0];

				if( rp.mapBetweenEdgeIdToVertex.find(edgeId) != rp.mapBetweenEdgeIdToVertex.end() )
				{
					std::vector<BrushVec3> vList;
					vList.push_back(rp.mapBetweenEdgeIdToVertex[edgeId]);
					for( int a = 0, iSideVertexCount(sideVertices[nCount].size()); a < iSideVertexCount; ++a )
					{
						if( nCount == 0 )
							vList.push_back(sideVertices[nCount][a]);
						else
							vList.push_back(sideVertices[nCount][iSideVertexCount-a-1]);
					}
					if( vList.size() >= 3 )
					{
						BrushPlane plane(vList[0],vList[1],vList[2],kDesignerEpsilon);
						CBrushRegion::RegionPtr pRegion = new CBrushRegion(vList,plane,pEdgeRegion->GetMaterialID(),&(pEdgeRegion->GetTexInfo()),true);
						GetDesigner()->AddRegion(pRegion,CBrushDesigner::eOpType_Add);
					}
				}
			}
			++nCount;
		}

		DESIGNER_ASSERT( nCount == 2 );
		DESIGNER_ASSERT( sideVertices[0].size() == sideVertices[1].size() );

		if( nCount == 2 && sideVertices[0].size() == sideVertices[1].size() )
		{
			SInfoForSubdivingApexRegion isar0;
			SInfoForSubdivingApexRegion isar1;

			isar0.edge = BrushEdge3D(sideVertices[0][0],sideVertices[0][sideVertices[0].size()-1]);
			isar1.edge = BrushEdge3D(sideVertices[1][sideVertices[1].size()-1],sideVertices[1][0]);

			int nValidDivideNum = sideVertices[0].size();
			for( int i = 0; i < nValidDivideNum-1; ++i )
			{
				std::vector<BrushVec3> vList;
				vList.push_back(sideVertices[0][i]);
				vList.push_back(sideVertices[1][i]);
				vList.push_back(sideVertices[1][i+1]);
				vList.push_back(sideVertices[0][i+1]);

				BrushPlane plane(sideVertices[0][i],sideVertices[1][i],sideVertices[1][i+1],kDesignerEpsilon);

				isar0.vIntermediate.push_back(std::pair<BrushVec3,BrushVec3>(sideVertices[0][i],plane.Normal()));
				isar1.vIntermediate.push_back(std::pair<BrushVec3,BrushVec3>(sideVertices[1][nValidDivideNum-i-1],plane.Normal()));

				CBrushRegion::RegionPtr pDividedRegion = new CBrushRegion(vList,plane,pEdgeRegion->GetMaterialID(),&(pEdgeRegion->GetTexInfo()),true);
				GetDesigner()->AddRegion(pDividedRegion,CBrushDesigner::eOpType_Add);
			}

			isar0.vIntermediate.push_back(std::pair<BrushVec3,BrushVec3>(sideVertices[0][nValidDivideNum-1],isar0.vIntermediate[isar0.vIntermediate.size()-1].second));
			isar1.vIntermediate.push_back(std::pair<BrushVec3,BrushVec3>(sideVertices[1][0],isar1.vIntermediate[isar1.vIntermediate.size()-1].second));

			infoForSubdividingApexRegionList.push_back(isar0);
			infoForSubdividingApexRegionList.push_back(isar1);
		}
	}

	PP1_SubdivideApexRegion(nSubdivideNum,infoForSubdividingApexRegionList);

	CreateMirroredRegions(GetDesigner());
	UpdateShelf(1);
}

int CBrushDesignerBevelTool::FindCorrespondingEdge( const BrushEdge3D& e, const std::vector<SInfoForSubdivingApexRegion>& infoForSubdividingApexRegionList ) const
{
	for( int i = 0, iCount(infoForSubdividingApexRegionList.size()); i < iCount; ++i )
	{
		if( infoForSubdividingApexRegionList[i].edge.IsEquivalent(e,kDesignerEpsilon) )
			return i;
	}
	return -1;
}

void CBrushDesignerBevelTool::PP1_SubdivideApexRegion( int nSubdivideNum, const std::vector<SInfoForSubdivingApexRegion>& infoForSubdividingApexRegionList )
{
	SResultForNextPhase& rp = m_ResultForSecondPhase;

	for( int i = 0, iCount(rp.middlePhaseApexRegions.size()); i < iCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = rp.middlePhaseApexRegions[i];

		std::vector<BrushVec3> vList;
		pRegion->GetLinkedVertices(vList);

		std::vector<const SInfoForSubdivingApexRegion*> sortedInfos;

		for( int k = 0, iVListCount(vList.size()); k < iVListCount; ++k )
		{
			BrushEdge3D e(vList[k],vList[(k+1)%iVListCount]);
			int nCorrespondingEdgeIndex = FindCorrespondingEdge(e, infoForSubdividingApexRegionList);
			DESIGNER_ASSERT(nCorrespondingEdgeIndex != -1);
			if( nCorrespondingEdgeIndex == -1 )
				continue;
			sortedInfos.push_back(&infoForSubdividingApexRegionList[nCorrespondingEdgeIndex]);
		}

		std::vector<CBrushRegion::RegionPtr> initialRegions;
		if( (nSubdivideNum%2) == 0 )
			initialRegions = CreateFirstOddSubdividedApexRegions(sortedInfos);
		else
			initialRegions = CreateFirstEvenSubdividedApexRegions(sortedInfos);

		if( !initialRegions.empty() )
		{
			GetDesigner()->RemoveRegion(pRegion);
			for( int k = 0, iInitialRegionCount(initialRegions.size()); k < iInitialRegionCount; ++k )
				GetDesigner()->AddRegion(initialRegions[k],CBrushDesigner::eOpType_Add);
		}
	}
}

std::vector<CBrushRegion::RegionPtr> CBrushDesignerBevelTool::CreateFirstOddSubdividedApexRegions( const std::vector<const SInfoForSubdivingApexRegion*>& subdividedEdges )
{
	std::vector<CBrushRegion::RegionPtr> subdividedRegions;

	int nSubdivisionCount = subdividedEdges[0]->vIntermediate.size();
	int nMiddleVertexIndex = nSubdivisionCount/2;

	BrushVec3 vCenter(0,0,0);
	int nSubdividedEdgeCount(subdividedEdges.size());
	for( int i = 0; i < nSubdividedEdgeCount; ++i )
		vCenter += subdividedEdges[i]->vIntermediate[nMiddleVertexIndex].first;
	vCenter /= nSubdividedEdgeCount;

	for( int i = 0; i < nSubdividedEdgeCount; ++i )
	{
		std::vector<BrushVec3> vList(4);
		vList[0] = subdividedEdges[i]->vIntermediate[nMiddleVertexIndex].first;
		vList[1] = subdividedEdges[i]->edge.m_v[1];
		vList[2] = subdividedEdges[(i+1)%nSubdividedEdgeCount]->vIntermediate[nMiddleVertexIndex].first;
		if( i == 0 )
		{
			BrushPlane p(vList[0],vList[1],vList[2],kDesignerEpsilon);
			p.HitTest(vCenter,vCenter+p.Normal(),kDesignerEpsilon,NULL,&vCenter);
		}
		vList[3] = vCenter;
		subdividedRegions.push_back(new CBrushRegion(vList));
	}

	return subdividedRegions;
}

std::vector<CBrushRegion::RegionPtr> CBrushDesignerBevelTool::CreateFirstEvenSubdividedApexRegions( const std::vector<const SInfoForSubdivingApexRegion*>& subdividedEdges )
{
	std::vector<CBrushRegion::RegionPtr> subdividedRegions;

	BrushPlane p(subdividedEdges[0]->vIntermediate[1].first,subdividedEdges[1]->vIntermediate[1].first,subdividedEdges[2]->vIntermediate[1].first,kDesignerEpsilon);

	BrushFloat fFirstEdgeDistance = subdividedEdges[0]->vIntermediate[0].first.GetDistance(subdividedEdges[0]->vIntermediate[1].first);
	std::vector<BrushVec3> vList;
	for( int i = 0, iEdgeCount(subdividedEdges.size()); i < iEdgeCount; ++i )
	{
		int nPrevIndex = i == 0 ? iEdgeCount-1 : i-1;
		int nCurrIndex = i;		

		const BrushVec3& vPrev = subdividedEdges[nPrevIndex]->edge.m_v[0];
		BrushVec3 vCurr = subdividedEdges[nCurrIndex]->edge.m_v[0];
		const BrushVec3& vNext = subdividedEdges[nCurrIndex]->edge.m_v[1];

		BrushVec3 vDir = ((vPrev-vCurr).GetNormalized() + (vNext-vCurr).GetNormalized()).GetNormalized();

		vCurr += vDir*fFirstEdgeDistance;
		p.HitTest(vCurr,vCurr+p.Normal(),kDesignerEpsilon,NULL,&vCurr);
		vList.push_back(vCurr);
	}

	CBrushRegion::RegionPtr pRegion = new CBrushRegion(vList);
	for( int i = 0, iEdgeCount(subdividedEdges.size()); i < iEdgeCount; ++i )
	{
		BrushEdge3D e = pRegion->GetEdge(i);

		vList.clear();
		vList.push_back(subdividedEdges[i]->vIntermediate[1].first);
		vList.push_back(subdividedEdges[i]->vIntermediate[subdividedEdges[i]->vIntermediate.size()-2].first);
		vList.push_back(e.m_v[1]);
		vList.push_back(e.m_v[0]);
		subdividedRegions.push_back(new CBrushRegion(vList));

		int nPrevIndex = i == 0 ? iEdgeCount-1 : i-1;
		vList.clear();
		vList.push_back(subdividedEdges[i]->vIntermediate[0].first);
		vList.push_back(subdividedEdges[i]->vIntermediate[1].first);
		vList.push_back(e.m_v[0]);
		vList.push_back(subdividedEdges[nPrevIndex]->vIntermediate[subdividedEdges[nPrevIndex]->vIntermediate.size()-2].first);
		subdividedRegions.push_back(new CBrushRegion(vList));
	}

	subdividedRegions.push_back(pRegion);

	return subdividedRegions;
}

void CBrushDesignerBevelTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_BevelMode != eBevelMode_Nothing )
	{
		if( !(nFlags & MK_CONTROL) )
		{
			int offset = point.y-m_nMousePrevY;
			if( m_BevelMode == eBevelMode_Spread )
				PP0_SpreadEdges(offset);
			else if( m_BevelMode == eBevelMode_Divide )
			{
				if( offset > 0 ) ++m_nDividedNumber;
				if( offset < 0 ) --m_nDividedNumber;
				if( m_nDividedNumber >= 20 )
					m_nDividedNumber = 20;
				if( m_nDividedNumber <= 1 )
					m_nDividedNumber = 1;
				PP0_SubdivideSpreadedEdge(m_nDividedNumber);
			}
		}
		m_nMousePrevY = point.y;
	}
}

bool CBrushDesignerBevelTool::OnKeyDown( CViewport *view,uint32 nKeycode,uint32 nRepCnt,uint32 nFlags )
{
	if (nKeycode == VK_ESCAPE )
	{
		if( m_BevelMode == eBevelMode_Nothing )
		{
			GetEditTool()->GoToSelectDesignerMode();
		}
		else if( m_BevelMode == eBevelMode_Spread )
		{
			if( m_pOriginalDesigner )
			{
				(*GetDesigner()) = *m_pOriginalDesigner;
				UpdateBrush();
			}
			GetEditTool()->GoToSelectDesignerMode();
		}
		else if( m_BevelMode == eBevelMode_Divide )
		{
			m_BevelMode = eBevelMode_Spread;

			DESIGNER_ASSERT(m_pOriginalDesigner);

			if( m_pOriginalDesigner )
			{
				(*GetDesigner()) = *m_pOriginalDesigner;
				UpdateBrush();
			}

			CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
			*pSelected = m_OriginalSelectedElements;

			if( !PP0_Initialize(true) )
				return false;
		}
	}

	return true;
}

int CBrushDesignerBevelTool::GetEdgeCountHavingVertexInElementList( const BrushVec3& vertex, const CBrushDesignerElementManager& elementList ) const
{
	int nCount = 0;
	for( int i = 0, iElementCount(elementList.GetSize()); i < iElementCount; ++i )
	{
		const SDesignerElement& elementInfo = elementList.Get(i);
		for( int k = 0, iVertexCount(elementInfo.m_Vertices.size()); k < iVertexCount; ++k )
		{
			if( elementInfo.m_Vertices[k].IsEquivalent(vertex,kDesignerEpsilon) )
				++nCount;
		}
	}
	return nCount;
}

void CBrushDesignerBevelTool::Display( DisplayContext &dc )
{
	CBrushDesignerBaseTool::Display(dc);
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->Display(GetBaseObject(),dc);
}