#include "StdAfx.h"
#include "BrushDesignerElementManager.h"
#include "BrushDesigner.h"
#include "Viewport.h"

SDesignerElement SDesignerElement::GetMirroredElement( const BrushPlane& mirrorPlane ) const
{
	DESIGNER_ASSERT( !m_Vertices.empty() );

	SDesignerElement mirroredElement(*this);

	for( int i = 0, iSize(m_Vertices.size()); i < iSize; ++i )
		mirroredElement.m_Vertices[i] = mirrorPlane.MirrorVertex(m_Vertices[i]);

	return mirroredElement;
}

bool SDesignerElement::operator == ( const SDesignerElement& info )
{
	if( m_Vertices.size() != info.m_Vertices.size() )
		return false;

	if( IsEdge() )
	{
		DESIGNER_ASSERT( m_Vertices.size() == 2 && info.m_Vertices.size() == 2 );
		BrushEdge3D edge(m_Vertices[0],m_Vertices[1]);
		BrushEdge3D infoEdge(info.m_Vertices[0],info.m_Vertices[1]);
		if( edge.IsEquivalent(infoEdge,kDesignerEpsilon) || edge.IsEquivalent(infoEdge.GetInverted(),kDesignerEpsilon) )
			return true;
		else
			return false;
	}

	if( IsFace() || IsVertex() )
	{
		if( m_Vertices.size() != info.m_Vertices.size() || m_pRegion != info.m_pRegion )
			return false;
		for( int i = 0, iSize(m_Vertices.size()); i < iSize; ++i )
		{
			bool bSameExist = false;
			for( int k = 0; k < iSize; ++k )
			{
				int nIndex = (i+k)%iSize;
				if( m_Vertices[nIndex].IsEquivalent(info.m_Vertices[nIndex],kDesignerEpsilon) )
				{
					bSameExist = true;
					break;
				}
			}
			if( !bSameExist )
				return false;
		}
		return true;
	}

	return false;
}

void SDesignerElement::Invalidate()
{
	m_Vertices.clear();
	m_pRegion = NULL;
	m_pObject = NULL;
}

bool SDesignerElement::IsEquivalent( const SDesignerElement& elementInfo ) const
{
	if( m_Vertices.size() != elementInfo.m_Vertices.size() )
		return false;

	if( IsFace() )
	{
		if( m_pRegion != NULL && m_pRegion == elementInfo.m_pRegion )
			return true;
		return false;
	}

	for( int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i )
	{
		bool bFoundEquivalent = false;
		for( int k = 0; k < iVertexSize; ++k )
		{
			if( m_Vertices[i].IsEquivalent(elementInfo.m_Vertices[(k+i)%iVertexSize],kDesignerEpsilon) )
			{
				bFoundEquivalent = true;
				break;
			}
		}
		if( !bFoundEquivalent )
			return false;
	}

	return true;
}

bool CBrushDesignerElementManager::Add( CBrushDesignerElementManager& elements )
{
	if( elements.IsEmpty() )
		return false;
	for( int i = 0, iElementSize(elements.GetSize()); i < iElementSize; ++i )
	{
		bool bEquivalentExist = false;
		for( int k = 0, iSelectedElementSize(m_Elements.size()); k < iSelectedElementSize; ++k )
		{
			if( m_Elements[k].IsEquivalent(elements[i]) )
			{
				bEquivalentExist = true;
				break;
			}
		}
		if( !bEquivalentExist )
			m_Elements.push_back(elements[i]);
	}
	return true;
}

bool CBrushDesignerElementManager::Add( const SDesignerElement& element )
{
	for( int k = 0, iSelectedElementSize(m_Elements.size()); k < iSelectedElementSize; ++k )
	{
		if( m_Elements[k].IsEquivalent(element) )
			return false;
	}
	m_Elements.push_back(element);
	return true;
}

bool CBrushDesignerElementManager::Pick( CBaseObject* pObject, CBrushDesigner* pDesigner, CViewport* viewport, CPoint point, int nFlag, bool bOnlyIncludeCube, BrushVec3* pOutPickedPos )
{
	BrushVec3 localRaySrc, localRayDir;
	BUtil::GetLocalViewRay( pObject->GetWorldTM(), viewport, point, localRaySrc, localRayDir );

	BrushVec3 outNearestPos;
	if( nFlag & BUtil::ePF_Vertex )
	{		
		if( QueryNearestVertex( pObject, pDesigner, viewport, point, localRaySrc, localRayDir, outNearestPos ) )
		{
			Clear();
			SDesignerElement element;
			element.m_Vertices.push_back(outNearestPos);
			element.m_pObject = pObject;
			element.m_pRegion = NULL;
			if( pOutPickedPos )
				*pOutPickedPos = outNearestPos;
			Add(element);
			return true;
		}
	}

	std::vector<CBrushDesigner::SQueryEdgeResult> queryResults;
	if( nFlag & BUtil::ePF_Edge )
	{
		std::vector< std::pair<BrushEdge3D,BrushVec3> > edges;		
		CRect selectionRect(CPoint(point.x-5,point.y-5),CPoint(point.x+5,point.y+5));
		CBrushRegion::RegionPtr pRectRegion = CBrushRegion::MakeRegionFromRectangle(selectionRect);
		pDesigner->QueryIntersectionEdgesWith2DRect(viewport,pObject->GetWorldTM(),pRectRegion,false,edges);
		if( !edges.empty() )
		{
			Clear();
			std::vector<BrushEdge3D> nearestEdges = BUtil::FindNearestEdges(viewport,pObject->GetWorldTM(),edges);
			int nShortEdgeIndex = BUtil::FindShortestEdge(nearestEdges);
			BrushEdge3D shortestEdge = nearestEdges[nShortEdgeIndex];
			SDesignerElement element;
			element.m_Vertices.push_back(shortestEdge.m_v[0]);
			element.m_Vertices.push_back(shortestEdge.m_v[1]);
			element.m_pObject = pObject;
			element.m_pRegion = NULL;
			if( pOutPickedPos )
				*pOutPickedPos = shortestEdge.GetCenter();
			Add(element);
			return true;
		}
	}

	if( nFlag & BUtil::ePF_Face )
	{
		int nPickedRegion(-1);
		BrushVec3 hitPos;
		if( !bOnlyIncludeCube && pDesigner->QueryRegion(localRaySrc, localRayDir, nPickedRegion) )
		{
			pDesigner->GetRegion(nPickedRegion)->GetPlane().HitTest( localRaySrc, localRaySrc+localRayDir, kDesignerEpsilon, NULL, &hitPos );
			if( pOutPickedPos )
				*pOutPickedPos = hitPos;
		}

		BrushVec3 vPickedPosFromBox;
		CBrushRegion::RegionPtr pPickedRegionFromBox = PickRegionFromRepresentativeBox( pObject, pDesigner, viewport, point, localRaySrc, localRayDir, vPickedPosFromBox );

		if( nPickedRegion != -1 )
		{
			if( pPickedRegionFromBox )
			{
				if( pPickedRegionFromBox != pDesigner->GetRegion(nPickedRegion) )
				{
					BrushFloat distToBox = vPickedPosFromBox.GetDistance(localRaySrc);
					BrushFloat distToFace = hitPos.GetDistance(localRaySrc);
					if( distToFace < distToBox )
					{
						pPickedRegionFromBox = pDesigner->GetRegion(nPickedRegion);
					}
					else
					{
						if( pOutPickedPos )
							*pOutPickedPos = vPickedPosFromBox;
					}
				}
			}
			else
			{
				pPickedRegionFromBox = pDesigner->GetRegion(nPickedRegion);
			}
		}

		if( pPickedRegionFromBox )
		{
			Clear();
			SDesignerElement element;
			element.SetFace(pObject,pPickedRegionFromBox);
			Add(element);
			return true;
		}
	}

	return false;
}

void CBrushDesignerElementManager::Display( CBaseObject* pObject, DisplayContext &dc ) const
{
	int nOldLineWidth = dc.GetLineWidth();
	dc.SetColor(BUtil::kSelectedColor);	
	dc.SetLineWidth(BUtil::kChosenLineThickness);	

	dc.PopMatrix();
	for( int i = 0, iElementCount(m_Elements.size()); i < iElementCount; ++i )
	{
		if( m_Elements[i].IsVertex() )
		{
			BrushVec3 worldVertexPos = pObject->GetWorldTM().TransformPoint(m_Elements[i].m_Vertices[0]);
			BrushVec3 vBoxSize = BUtil::GetElementBoxSize(dc.view,dc.flags&DISPLAY_2D,worldVertexPos);
			dc.DrawSolidBox(ToVec3(worldVertexPos-vBoxSize), ToVec3(worldVertexPos+vBoxSize));
		}
	}	
	dc.PushMatrix(pObject->GetWorldTM());

	for( int i = 0, iElementCount(m_Elements.size()); i < iElementCount; ++i )
	{
		if( m_Elements[i].IsEdge() )
		{
			dc.DrawLine(m_Elements[i].m_Vertices[0],m_Elements[i].m_Vertices[1]);
		}
		else if( m_Elements[i].IsFace() && m_Elements[i].m_pRegion->IsOpen() )
		{
			for( int k = 0, iEdgeCount(m_Elements[i].m_pRegion->GetEdgeSize()); k < iEdgeCount; ++k )
			{
				BrushEdge3D e = m_Elements[i].m_pRegion->GetEdge(k);
				dc.DrawLine(e.m_v[0],e.m_v[1]);
			}
		}
	}

	dc.SetLineWidth(nOldLineWidth);
}

void CBrushDesignerElementManager::DisplayHighlightElements( CBaseObject* pObject, CBrushDesigner* pDesigner, DisplayContext& dc, int nPickFlag ) const
{
	if( nPickFlag & BUtil::ePF_Vertex )
		DisplayVertexElements(pObject,pDesigner,dc);
	if( nPickFlag & BUtil::ePF_Face )
		DisplayFaceElements(pObject,pDesigner,dc);
}

void CBrushDesignerElementManager::DisplayVertexElements( CBaseObject* pObject, CBrushDesigner* pDesigner, DisplayContext& dc, int nShelf, std::vector<BrushVec3>* pExcludedVertices ) const
{
	dc.PopMatrix();

	DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);
	for( int k = 0; k < 2; ++k )
	{
		if( nShelf != -1 && nShelf != k )
			continue;
		pDesigner->SetShelf(k);
		for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
		{
			CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
			if( pRegion->CheckFlags(CBrushRegion::eRF_Hidden|CBrushRegion::eRF_Mirrored) )
				continue;
			for( int a = 0, iVertexCount(pRegion->GetVertexListSize()); a < iVertexCount; ++a )
			{
				const BrushVec3& v = pRegion->GetVertex(a);
				if( HasVertex(v) )
					continue;
				if( pExcludedVertices )
				{
					bool bFound = false;
					for( int b = 0, iExcludedVertexCount(pExcludedVertices->size()); b < iExcludedVertexCount; ++b )
					{
						if( (*pExcludedVertices)[b].IsEquivalent(v,kDesignerEpsilon) )
						{
							bFound = true;
							break;
						}
					}
					if( bFound )
						continue;
				}
				BrushVec3 vWorldVertexPos = pObject->GetWorldTM().TransformPoint(v);
				BrushVec3 vBoxSize = BUtil::GetElementBoxSize(dc.view,dc.flags&DISPLAY_2D,vWorldVertexPos);
				dc.SetColor(BUtil::kElementBoxColor);
				if( pRegion->IsOpen() )
				{
					std::vector<int> edgeIndices;
					if( pRegion->QueryEdgesHavingVertex(v,edgeIndices) && edgeIndices.size() == 1 )
					{
						BrushEdge3D e = pRegion->GetEdge(edgeIndices[0]);						
						if( e.m_v[0].IsEquivalent(v,kDesignerEpsilon) )
							dc.SetColor(ColorB(0xFFFFAAFF));
					}
				}
				dc.DrawSolidBox(ToVec3(vWorldVertexPos-vBoxSize), ToVec3(vWorldVertexPos+vBoxSize));
			}
		}
	}

	dc.PushMatrix(pObject->GetWorldTM());
}

void CBrushDesignerElementManager::DisplayFaceElements( CBaseObject* pObject, CBrushDesigner* pDesigner, DisplayContext& dc ) const
{
	dc.PopMatrix();

	DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);
	for( int k = 0; k < 2; ++k )
	{
		pDesigner->SetShelf(k);
		for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
		{
			CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
			if( !pRegion->IsValid() || pRegion->CheckFlags(CBrushRegion::eRF_Hidden|CBrushRegion::eRF_Mirrored) )
				continue;
			if( HasRegionSelected(pRegion) )
				dc.SetColor(BUtil::kSelectedColor);
			else
				dc.SetColor(BUtil::kElementBoxColor);
			BrushVec3 pos = pObject->GetWorldTM().TransformPoint(pRegion->GetRepresentativePosition());
			BrushVec3 vBoxSize = BUtil::GetElementBoxSize(dc.view,dc.flags&DISPLAY_2D,pos);
			dc.DrawSolidBox(ToVec3(pos-vBoxSize),ToVec3(pos+vBoxSize));
		}
	}

	dc.PushMatrix(pObject->GetWorldTM());
}

void CBrushDesignerElementManager::PickAdjacentCurvedEdges( CBaseObject* pObject, CBrushRegion::RegionPtr pRegion, const BrushEdge3D& edge, std::vector<SDesignerElement>* outPickInfo ) const
{
	int nInitialIndex = pRegion->GetEdgeIndex(edge);
	int nEdgeIndex(nInitialIndex);
	if( nEdgeIndex != -1 )
	{
		while(1)
		{
			int nPrevEdgeIndex = -1;
			if( !pRegion->GetAdjacentEdgesByEdgeIndex( nEdgeIndex, &nPrevEdgeIndex, NULL ) || nPrevEdgeIndex == -1 || nInitialIndex == nPrevEdgeIndex )
				break;

			BrushEdge3D currentEdge = pRegion->GetEdge(nEdgeIndex);
			BrushEdge3D prevEdge = pRegion->GetEdge(nPrevEdgeIndex);
			if( currentEdge.GetDirection().Dot(-prevEdge.GetDirection()) >= 0 )
				break;

			SDesignerElement element;
			element.m_Vertices.push_back(prevEdge.m_v[0]);
			element.m_Vertices.push_back(prevEdge.m_v[1]);
			element.m_pObject = pObject;
			outPickInfo->push_back(element);
			nEdgeIndex = nPrevEdgeIndex;
		}

		nEdgeIndex = nInitialIndex;

		while(1)
		{
			int nNextEdgeIndex = -1;
			if( !pRegion->GetAdjacentEdgesByEdgeIndex( nEdgeIndex, NULL, &nNextEdgeIndex ) || nNextEdgeIndex == -1 || nInitialIndex == nNextEdgeIndex )
				break;

			BrushEdge3D currentEdge = pRegion->GetEdge(nEdgeIndex);
			BrushEdge3D nextEdge = pRegion->GetEdge(nNextEdgeIndex);
			if( nextEdge.GetDirection().Dot(-currentEdge.GetDirection()) >= 0 )
				break;

			SDesignerElement element;
			element.m_Vertices.push_back(nextEdge.m_v[0]);
			element.m_Vertices.push_back(nextEdge.m_v[1]);
			element.m_pObject = pObject;
			outPickInfo->push_back(element);
			nEdgeIndex = nNextEdgeIndex;
		}
	}
}

BrushVec3 CBrushDesignerElementManager::GetNormal( CBrushDesigner* pDesigner ) const
{
	if( m_Elements.empty() )
		return BrushVec3(0,0,1);

	BrushVec3 normal(0,0,0);
	for( int i = 0, iSize(m_Elements.size()); i < iSize; ++i )
	{
		if( m_Elements[i].IsFace() )
			normal += m_Elements[i].m_pRegion->GetPlane().Normal();
	}

	if( !normal.IsZero() )
		return normal.GetNormalized();

	CBrushDesignerDB::QueryResult queryResult = QueryFromElements(pDesigner);
	for( int i = 0, iQuerySize(queryResult.size()); i < iQuerySize; ++i )
	{
		for( int k = 0, iMarkListSize(queryResult[i].m_MarkList.size()); k < iMarkListSize; ++k )
		{
			CBrushRegion::RegionPtr pRegion = queryResult[i].m_MarkList[k].m_pRegion;
			if( pRegion == NULL )
				continue;
			normal += pRegion->GetPlane().Normal();
		}
	}

	return normal.GetNormalized();
}

void CBrushDesignerElementManager::Erase( int nElementFlags )
{
	std::vector<SDesignerElement>::iterator ii = m_Elements.begin();
	for( ; ii != m_Elements.end(); )
	{
		if( (nElementFlags&BUtil::ePF_Vertex) && (*ii).IsVertex() || (nElementFlags&BUtil::ePF_Edge) && (*ii).IsEdge() || (nElementFlags&BUtil::ePF_Face) && (*ii).IsFace() )
			ii = m_Elements.erase(ii);
		else
			++ii;
	}
}

bool CBrushDesignerElementManager::Erase( CBrushDesignerElementManager& elements )
{
	std::vector<SDesignerElement>::iterator ii = m_Elements.begin();
	bool bErasedAtLeastOne = false;
	for( ; ii != m_Elements.end() ; )
	{
		bool bErased = false;
		for( int i = 0, iElementCount(elements.GetSize()); i < iElementCount; ++i )
		{
			if( (*ii) == elements[i] )
			{
				ii = m_Elements.erase(ii);
				bErasedAtLeastOne = bErased = true;
				break;
			}
		}
		if( !bErased )
			++ii;
	}
	return bErasedAtLeastOne;
}

void CBrushDesignerElementManager::Erase( const SDesignerElement& element )
{
	std::vector<SDesignerElement>::iterator ii = m_Elements.begin();
	for( ;ii != m_Elements.end(); )
	{
		if( (*ii) == element )
			ii = m_Elements.erase(ii);
		else
			++ii;
	}
}

bool CBrushDesignerElementManager::Has( const SDesignerElement& elementInfo ) const
{
	if( m_Elements.empty() )
		return false;

	for( int i = 0, iElementSize(m_Elements.size()); i < iElementSize; ++i )
	{
		if( m_Elements[i].IsEquivalent(elementInfo) )
			return true;
	}

	return false;
}

bool CBrushDesignerElementManager::HasVertex( const BrushVec3& vertex ) const
{
	for( int i = 0, iElementCount(m_Elements.size()); i < iElementCount; ++i )
	{
		if( !m_Elements[i].IsVertex() )
			continue;

		if( m_Elements[i].GetVertex().IsEquivalent(vertex,kDesignerEpsilon) )
			return true;
	}

	return false;
}

CString CBrushDesignerElementManager::GetElementsInfoText()
{
	int nVertexNum = 0;
	int nEdgeNum = 0;
	int nFaceNum = 0;	
	for( int i = 0, iCount(m_Elements.size()); i < iCount; ++i )
	{
		if( m_Elements[i].IsVertex() )
			++nVertexNum;
		else if( m_Elements[i].IsEdge() ) 
			++nEdgeNum;
		else if( m_Elements[i].IsFace() ) 
			++nFaceNum;
	}

	CString str;
	if( nVertexNum > 0 )
	{
		CString substr;
		substr.Format("%d Vertex(s)",nVertexNum);
		str += substr;
	}
	if( nEdgeNum > 0 )
	{
		if(!str.IsEmpty())
			str += ",";
		CString substr;
		substr.Format("%d Edge(s)",nEdgeNum);
		str += substr;
	}
	if( nFaceNum > 0 )
	{
		if(!str.IsEmpty())
			str += ",";
		CString substr;
		substr.Format("%d Face(s)",nFaceNum);
		str += substr;
	}

	return str;
}

void CBrushDesignerElementManager::RemoveInvalidElements()
{
	std::vector<SDesignerElement>::iterator ii = m_Elements.begin();
	for( ; ii != m_Elements.end(); )
	{
		if( (*ii).IsFace() && ((*ii).m_pRegion == NULL || !(*ii).m_pRegion->IsValid()) )
			ii = m_Elements.erase(ii);
		else
			++ii;
	}
}

CBrushDesignerDB::QueryResult CBrushDesignerElementManager::QueryFromElements( CBrushDesigner* pDesigner ) const
{
	DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);
	CBrushDesignerDB::QueryResult queryResult;	

	for( int i = 0, iSelectedSize(m_Elements.size()); i < iSelectedSize; ++i )
	{	
		int iVertexSize(m_Elements[i].m_Vertices.size());
		for( int k = 0; k < iVertexSize; ++k )
			pDesigner->GetDB()->QueryAsVertex(m_Elements[i].m_Vertices[k],queryResult);
	}

	if( !pDesigner->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		return queryResult;

	CBrushDesignerDB::QueryResult::iterator iQuery = queryResult.begin();
	for( ;iQuery != queryResult.end(); )
	{
		CBrushDesignerDB::MarkList::iterator iMark= (*iQuery).m_MarkList.begin();
		for( ;iMark != (*iQuery).m_MarkList.end(); )
		{
			CBrushRegion::RegionPtr pRegion = iMark->m_pRegion;
			if( pRegion == NULL || pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) )
				iMark = (*iQuery).m_MarkList.erase(iMark);
			else
				++iMark;
		}

		if( (*iQuery).m_MarkList.empty() )
			iQuery = queryResult.erase(iQuery);
		else
			++iQuery;
	}

	return queryResult;
}

bool CBrushDesignerElementManager::QueryNearestVertex( CBaseObject* pObject, CBrushDesigner* pDesigner, CViewport* pView, CPoint point, const BrushVec3& rayLocalSrc, const BrushVec3& rayLocalDir, BrushVec3& outPos, BrushVec3* pOutNormal ) const
{
	Vec3 raySrc, rayDir;
	pView->ViewToWorldRay(point,raySrc,rayDir);

	BrushFloat fLeastDist = (BrushFloat)3e10;
	bool bFound = false;

	for( int a = 0, iRegionCount(pDesigner->GetRegionSize()); a < iRegionCount; ++a )
	{
		CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(a);
		if( pRegion->CheckFlags(CBrushRegion::eRF_Hidden) )
			continue;
		for( int i = 0, iVertexCount(pRegion->GetVertexListSize()); i < iVertexCount; ++i )
		{
			const BrushVec3& v = pRegion->GetVertex(i);
			BrushFloat t = 0;
			BrushVec3 vWorldPos = pObject->GetWorldTM().TransformPoint(v);
			BrushVec3 vBoxSize = BUtil::GetElementBoxSize( pView, pView->GetType()!=ET_ViewportCamera, vWorldPos );
			if( !BUtil::GetIntersectionOfRayAndAABB(ToBrushVec3(raySrc),ToBrushVec3(rayDir),AABB(ToVec3(vWorldPos-vBoxSize),ToVec3(vWorldPos+vBoxSize)),&t) )
				continue;
			if( t > 0 && t < fLeastDist )
			{
				fLeastDist = t;
				outPos = v;
				if( pOutNormal )
				{
					if( pRegion->IsOpen() )
					{
						int nRegionIndex = -1;
						if( pDesigner->QueryRegion(rayLocalSrc, rayLocalDir, nRegionIndex) )
						{
							CBrushRegion::RegionPtr pClosedRegion = pDesigner->GetRegion(nRegionIndex);
							*pOutNormal = pClosedRegion->GetPlane().Normal();
						}
						else
						{
							*pOutNormal = BrushVec3(0,0,1);
						}
					}
					else
					{
						*pOutNormal = pRegion->GetPlane().Normal();
					}
				}
				bFound = true;
			}
		}
	}

	return bFound;
}

bool CBrushDesignerElementManager::HasRegionSelected( CBrushRegion::RegionPtr pRegion ) const
{
	for( int i = 0, iCount(m_Elements.size()); i < iCount; ++i )
	{
		if( m_Elements[i].IsFace() && m_Elements[i].m_pRegion == pRegion )
			return true;
	}
	return false;
}

CBrushRegion::RegionPtr CBrushDesignerElementManager::PickRegionFromRepresentativeBox( CBaseObject* pObject, CBrushDesigner* pDesigner, CViewport* pView, CPoint point, const BrushVec3& rayLocalSrc, const BrushVec3& rayLocalDir, BrushVec3& outPickedPos ) const
{
	if( !gSettings.bDesignerHighlightElements )
		return NULL;

	Vec3 raySrc, rayDir;
	pView->ViewToWorldRay(point,raySrc,rayDir);

	DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);

	BrushFloat fLeastDist = (BrushFloat)3e10;
	CBrushRegion::RegionPtr pPickedRegion = NULL;	

	BrushMatrix34 matInvWorld = pObject->GetWorldTM().GetInverted();

	for( int i = 0; i < BUtil::kMaxShelfCount; ++i )
	{
		pDesigner->SetShelf(i);
		for( int k = 0, iRegionCount(pDesigner->GetRegionSize()); k < iRegionCount; ++k )
		{
			CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(k);
			if( !pRegion->IsValid() || pRegion->CheckFlags(CBrushRegion::eRF_Hidden) )
				continue;
			BrushVec3 v = pRegion->GetRepresentativePosition();
			BrushFloat t = 0;
			BrushVec3 vWorldPos = pObject->GetWorldTM().TransformPoint(v);
			BrushVec3 vBoxSize = BUtil::GetElementBoxSize(pView,pView->GetType()!=ET_ViewportCamera,vWorldPos);
			if( BUtil::GetIntersectionOfRayAndAABB( ToBrushVec3(raySrc), ToBrushVec3(rayDir), AABB(ToVec3(vWorldPos-vBoxSize),ToVec3(vWorldPos+vBoxSize)), &t ) )
			{
				if( t > 0 && t < fLeastDist )
				{
					fLeastDist = t;
					outPickedPos = matInvWorld.TransformPoint(raySrc + rayDir * t);
					pPickedRegion = pRegion;
				}
			}
		}
	}

	return pPickedRegion;
}