#include "StdAfx.h"
#include "BaseBrush.h"
#include "BrushDesigner.h"
#include "BrushDesignerUndo.h"
#include "IBaseToolPanel.h"
#include "BrushDesignerEdgesSharpnessManager.h"
#include "BrushDesignerSmoothingGroupManager.h"
#include "BrushDesignerHalfEdgeMesh.h"
#include "BrushDesignerDB.h"

const BrushFloat CBrushDesigner::kDrillOffsetSize = -0.1f;

void CBrushDesigner::Init()
{
	m_nModeFlag = 0;
	m_ShelfID = 0;
	m_MirrorPlane = BrushPlane(BrushVec3(0,0,0),0);
	m_SubdivisionLevel = 0;
	m_nTessFactor = 0;
	m_pDB = new CBrushDesignerDB;
	m_pDB->AddRef();
	m_pSmoothingGroupMgr = new CBrushDesignerSmoothingGroupManager;
	m_pSmoothingGroupMgr->AddRef();
	m_pEdgeSharpnessMgr = new CBrushDesignerEdgeSharpnessManager;
	m_pEdgeSharpnessMgr->AddRef();
	m_pSubdividionResult = NULL;
}

CBrushDesigner::CBrushDesigner()
{
	Init();
}

CBrushDesigner::~CBrushDesigner()
{
	if( m_pDB )
		m_pDB->Release();
	if( m_pSmoothingGroupMgr )
		m_pSmoothingGroupMgr->Release();
	if( m_pEdgeSharpnessMgr )
		m_pEdgeSharpnessMgr->Release();
	if( m_pSubdividionResult )
		m_pSubdividionResult->Release();
}

CBrushDesigner::CBrushDesigner( const std::vector<CBrushRegion::RegionPtr>& regionList )
{
	Init();
	m_Regions[0] = regionList;
}

CBrushDesigner::CBrushDesigner( const CBrushDesigner& designer ) :
CRefCountBase()
{
	Init();
	operator =(designer);
}

CBrushDesigner& CBrushDesigner::operator = ( const CBrushDesigner& designer )
{
	for( BUtil::ShelfID shelfID = 0; shelfID < BUtil::kMaxShelfCount; ++shelfID )
	{
		SetShelf(shelfID);
		Clear();
	}

	for( BUtil::ShelfID shelfID = 0; shelfID < BUtil::kMaxShelfCount; ++shelfID )
	{
		int nRegionSize(designer.m_Regions[shelfID].size());
		m_Regions[shelfID].reserve(nRegionSize);
		for( int i = 0; i < nRegionSize; ++i )
			m_Regions[shelfID].push_back(designer.m_Regions[shelfID][i]->Clone());
	}

	m_MirrorPlane = designer.m_MirrorPlane;
	m_nModeFlag = designer.m_nModeFlag;
	m_SubdivisionLevel = designer.m_SubdivisionLevel;
	m_nTessFactor = designer.m_nTessFactor;
	
	*m_pDB = *designer.m_pDB;
	m_pSmoothingGroupMgr->CopyFromDesigner(this,&designer);
	m_pEdgeSharpnessMgr->CopyFromDesigner(this,&designer);

	m_ShelfID = 0;

	return *this;
}

bool CBrushDesigner::QueryPosition( const BrushPlane& plane, const BrushVec3& localRayOrigin, const BrushVec3& localRayDir, BrushVec3& outPosition, BrushFloat* outDist, CBrushRegion::RegionPtr* outRegion ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	BrushFloat dist;
	if( plane.HitTest( localRayOrigin, localRayOrigin+localRayDir, kDesignerEpsilon, &dist, &outPosition ) == false )
		return false;
	int nRegion(-1);
	if( outRegion && QueryRegion( plane, localRayOrigin, localRayDir, nRegion ) )
		*outRegion = m_Regions[m_ShelfID][nRegion];
	if( outDist )
		*outDist = dist;
	return true;
}

bool CBrushDesigner::QueryPosition( const BrushVec3& localRayOrigin, const BrushVec3& localRayDir, BrushVec3& outPosition, BrushPlane* outPlane, BrushFloat* outDist, CBrushRegion::RegionPtr* outRegion ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	int nRegion(-1);
	if( QueryRegion( localRayOrigin, localRayDir, nRegion ) == false )
		return false;

	CBrushRegion::RegionPtr pRegion(m_Regions[m_ShelfID][nRegion]);
	BrushFloat dist;
	if( pRegion->GetPlane().HitTest( localRayOrigin, localRayOrigin+localRayDir, kDesignerEpsilon, &dist, &outPosition ) == false )
		return false;

	if(outRegion)
		*outRegion = pRegion;
	if( outPlane )
		*outPlane = pRegion->GetPlane();
	if( outDist )
		*outDist = dist;

	return true;
}

bool CBrushDesigner::QueryEdgesHavingVertex( const BrushVec3& vertex, std::vector<BrushEdge3D>& outEdges ) const
{
	bool bFound = false;

	for( int i = 0, iRegionCount(GetRegionSize()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = GetRegion(i);

		std::vector<CBrushRegion::RegionPtr> regionsOutside;
		if( !pRegion->IsOpen() )
		{
			if( !pRegion->GetSeparatedRegions(regionsOutside,CBrushRegion::eSR_OuterHull) || regionsOutside.empty() )
				continue;
		}
		else
		{
			regionsOutside.push_back(pRegion);
		}

		for( int k = 0, iRegionsOutsideCount(regionsOutside.size()); k < iRegionsOutsideCount; ++k )
		{
			pRegion = regionsOutside[k];
			std::vector<int> edgeIndices;
			if( !pRegion->QueryEdgesHavingVertex(vertex,edgeIndices) )
				continue;
			bFound = true;
			for( int k = 0, iEdgeCount(edgeIndices.size()); k < iEdgeCount; ++k )
				outEdges.push_back(pRegion->GetEdge(edgeIndices[k]));
		}
	}

	return bFound;
}

void CBrushDesigner::QueryOpenRegions( const BrushVec3& raySrc, const BrushVec3& rayDir, std::vector<CBrushRegion::RegionPtr>& outRegions ) const
{
	for( int i = 0, iRegionCount(m_Regions[m_ShelfID].size()); i < iRegionCount; ++i )
	{
		if( !m_Regions[m_ShelfID][i]->IsOpen() )
			continue;
		Vec3 vHitPos;
		AABB aabb = m_Regions[m_ShelfID][i]->GetBoundBox();
		aabb.Expand(Vec3(0.01f));
		if( Intersect::Ray_AABB(ToVec3(raySrc),ToVec3(rayDir),aabb,vHitPos) )
			outRegions.push_back(m_Regions[m_ShelfID][i]);
	}
}

bool CBrushDesigner::AddRegion( CBrushRegion::RegionPtr pRegion, EOperationType opType )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion )
		return false;

	bool bSuccess(false);
	if( opType == eOpType_Add || pRegion->IsOpen() )
	{
		bSuccess = true; 
		AddRegion(pRegion);
	}
	else if( opType == eOpType_Union )
		bSuccess = UnionRegion(pRegion);
	else if( opType == eOpType_SubtractAB )
		bSuccess = SubtractRegionAB(pRegion);
	else if( opType == eOpType_SubtractBA )
		bSuccess = SubtractRegionBA(pRegion);
	else if( opType == eOpType_Intersection )
		bSuccess = IntersectRegion(pRegion);
	else if( opType == eOpType_Split)
		bSuccess = SplitRegion(pRegion);
	else if( opType == eOpType_ExclusiveOR )
		bSuccess = ExclusiveORRegion(pRegion);

	return bSuccess;
}

void CBrushDesigner::AddRegionUnconditionally( CBrushRegion::RegionPtr pRegion )
{
	m_Regions[m_ShelfID].push_back(pRegion);
}

bool CBrushDesigner::AddOpenRegion( CBrushRegion::RegionPtr pRegion, bool bOnlyAdd )
{
	DESIGNER_ASSERT( pRegion->IsOpen() );

	if( !pRegion || !pRegion->IsOpen() )
		return false;

	if( bOnlyAdd )
	{
		AddRegion(pRegion->Clone());
		return true;
	}

	return SplitRegionsByOpenRegion(pRegion);
}

bool CBrushDesigner::UnionRegion( CBrushRegion::RegionPtr pRegion )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion )
		return false;

	if( pRegion->IsOpen() )
		return true;

	AABB aabb = pRegion->GetBoundBox();
	aabb.Expand(Vec3(0.01f,0.01f,0.01f));

	CBrushRegion::RegionPtr pNewRegion = pRegion->Clone();
	std::set<CBrushRegion::RegionPtr> deletedRegions;
	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		if( m_Regions[m_ShelfID][i]->IsOpen() )
			continue;

		if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) != m_Regions[m_ShelfID][i]->CheckFlags(CBrushRegion::eRF_Mirrored) )
			continue;

		if( !aabb.IsIntersectBox(m_Regions[m_ShelfID][i]->GetBoundBox()) )
			continue;

		if( CBrushRegion::HasIntersection(m_Regions[m_ShelfID][i],pRegion) != BUtil::eIT_None )
		{	
			CBrushRegion::RegionPtr pBackupRegion = pNewRegion->Clone();

			if( pNewRegion->Union(m_Regions[m_ShelfID][i]) )
			{
#ifdef ENABLE_OUTPUT_DEBUGINFO
				DESIGNER_ASSERT(pNewRegion->IsValid() && !pNewRegion->IsOpen());
				if( !pNewRegion->IsValid() || pNewRegion->IsOpen() )
				{
					pBackupRegion->OutputDebugData();
					m_Regions[m_ShelfID][i]->OutputDebugData();
					CBrushRegion::OutputTestCode(pBackupRegion,m_Regions[m_ShelfID][i],"Union",kDesignerEpsilon);

					IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
					dlg->AddRegion(pBackupRegion.get(),"pBackupRegion"); 
					dlg->AddRegion(m_Regions[m_ShelfID][i].get(),"m_Regions[m_ShelfID][i]");
					dlg->AddRegion(pNewRegion.get(),"pNewRegion");
					dlg->Open();
				}
#endif
				deletedRegions.insert(m_Regions[m_ShelfID][i]);
			}
			else
			{
#ifdef ENABLE_OUTPUT_DEBUGINFO
				pBackupRegion->OutputDebugData();
				m_Regions[m_ShelfID][i]->OutputDebugData();
				CBrushRegion::OutputTestCode(pBackupRegion,m_Regions[m_ShelfID][i],"Union",kDesignerEpsilon);

				IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
				dlg->AddRegion(pBackupRegion.get(),"pBackupRegion"); 
				dlg->AddRegion(m_Regions[m_ShelfID][i].get(),"m_Regions[m_ShelfID][i]");
				dlg->AddRegion(pNewRegion.get(),"pNewRegion");
				dlg->Open();
#endif
				return false;
			}
		}
	}

	if( !pNewRegion->IsValid() || pNewRegion->IsOpen() )
	{
		if( !deletedRegions.empty() )
		{
#ifdef ENABLE_OUTPUT_DEBUGINFO
			pNewRegion->OutputDebugData();
			CBrushRegion::OutputTestCode( pRegion, *deletedRegions.begin(), "Union", kDesignerEpsilon );
#endif
		}
		return false;
	}

	DeleteRegions(deletedRegions);
	AddRegionSeparately(pNewRegion);
	InvalidateAABB(m_ShelfID);

	return true;
}

bool CBrushDesigner::SubtractRegionAB( CBrushRegion::RegionPtr pRegion )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion )
		return false;

	if( pRegion->IsOpen() )
		return true;

	AABB aabb = pRegion->GetBoundBox();
	aabb.Expand(Vec3(0.01f,0.01f,0.01f));

	CBrushRegion::RegionPtr pNewRegion = pRegion->Clone();
	bool bIntersetion(false);

	std::vector<CBrushRegion::RegionPtr> copiedRegions(m_Regions[m_ShelfID]);
	std::vector<CBrushRegion::RegionPtr>::iterator ii = copiedRegions.begin();

	for( ; ii != copiedRegions.end(); ++ii )
	{
		if( (*ii)->IsOpen() )
			continue;
		if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) != (*ii)->CheckFlags(CBrushRegion::eRF_Mirrored) )
			continue;
		if( !aabb.IsIntersectBox((*ii)->GetBoundBox()) )
			continue;		
		if( (*ii)->Subtract(pNewRegion) )
		{
			bIntersetion = true;
			std::vector<CBrushRegion::RegionPtr>::iterator ioriginal = m_Regions[m_ShelfID].begin();
			for( ; ioriginal != m_Regions[m_ShelfID].end(); ++ioriginal )
			{
				if( *ioriginal == *ii )
				{
					ioriginal = RemoveRegion(ioriginal);
					break;
				}
			}
			if( (*ii)->IsValid() )
				AddRegionSeparately(*ii);
		}
	}

	if( bIntersetion == false )
		AddRegionSeparately(pNewRegion);

	InvalidateAABB(m_ShelfID);

	return true;
}

bool CBrushDesigner::SubtractRegionBA( CBrushRegion::RegionPtr pRegion )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion )
		return false;

	if( pRegion->IsOpen() )
		return true;

	AABB aabb = pRegion->GetBoundBox();
	aabb.Expand(Vec3(0.01f,0.01f,0.01f));

	CBrushRegion::RegionPtr pNewRegion = pRegion->Clone();
	std::vector<CBrushRegion::RegionPtr>::iterator ii = m_Regions[m_ShelfID].begin();

	for( ; ii !=  m_Regions[m_ShelfID].end(); ++ii )
	{
		if( pRegion->IsOpen() )
			continue;
		if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) != (*ii)->CheckFlags(CBrushRegion::eRF_Mirrored) )
			continue;
		if( !aabb.IsIntersectBox((*ii)->GetBoundBox()) )
			continue;
		pNewRegion->Subtract(*ii);
	}

	if( pNewRegion->IsValid() )
		AddRegionSeparately(pNewRegion);	

	InvalidateAABB(m_ShelfID);

	return true;
}

bool CBrushDesigner::IntersectRegion( CBrushRegion::RegionPtr pRegion )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion )
		return false;

	if( pRegion->IsOpen() )
		return true;

	AABB aabb = pRegion->GetBoundBox();
	aabb.Expand(Vec3(0.01f,0.01f,0.01f));

	CBrushRegion::RegionPtr pNewRegion = pRegion->Clone();
	std::vector<CBrushRegion::RegionPtr> intersectedRegions;
	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{		
		if( m_Regions[m_ShelfID][i]->IsOpen() )
			continue;
		if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) != m_Regions[m_ShelfID][i]->CheckFlags(CBrushRegion::eRF_Mirrored) )
			continue;
		if( !aabb.IsIntersectBox(m_Regions[m_ShelfID][i]->GetBoundBox()) )
			continue;		
		if( !m_Regions[m_ShelfID][i]->Intersect(pNewRegion,CBrushRegion::eICEII_IncludeCoSame) )
			return false;
		intersectedRegions.push_back(m_Regions[m_ShelfID][i]);
	}

	if( intersectedRegions.empty() )
	{
		AddRegion(pNewRegion);
	}
	else
	{
		for( int i = 0, iRegionCount(intersectedRegions.size()); i < iRegionCount; ++i )
		{
			if( !intersectedRegions[i]->IsValid() )
				RemoveRegion(intersectedRegions[i]);
			else
				AddRegionSeparately(intersectedRegions[i],true);
		}
	}

	InvalidateAABB(m_ShelfID);

	return true;
}

bool CBrushDesigner::ExclusiveORRegion( CBrushRegion::RegionPtr pRegion )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion )
		return false;

	if( pRegion->IsOpen() )
		return true;

	AABB aabb = pRegion->GetBoundBox();
	aabb.Expand(Vec3(0.01f,0.01f,0.01f));

	std::vector<CBrushRegion::RegionPtr> overlappedRegions;
	std::vector<CBrushRegion::RegionPtr> overlappedReplicatedRegions;	
	std::vector<CBrushRegion::RegionPtr> touchedRegions;

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		if( m_Regions[m_ShelfID][i]->IsOpen() )
			continue;
		if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) != m_Regions[m_ShelfID][i]->CheckFlags(CBrushRegion::eRF_Mirrored) )
			continue;
		if( !aabb.IsIntersectBox(m_Regions[m_ShelfID][i]->GetBoundBox()) )
			continue;
		if( !pRegion->GetPlane().IsEquivalent(m_Regions[m_ShelfID][i]->GetPlane(),kDesignerEpsilon) )
			continue;

		BUtil::EIntersectionType it = CBrushRegion::HasIntersection(m_Regions[m_ShelfID][i],pRegion);
		if( it == BUtil::eIT_Intersection || it == BUtil::eIT_JustTouch && m_Regions[m_ShelfID][i]->HasBridgeEdges() )
		{
			overlappedRegions.push_back(m_Regions[m_ShelfID][i]);
			overlappedReplicatedRegions.push_back(m_Regions[m_ShelfID][i]->Clone());
		}
		else if( it == BUtil::eIT_JustTouch )
		{
			touchedRegions.push_back(m_Regions[m_ShelfID][i]);
		}
	}

	int iOverlappedRegionSize = overlappedRegions.size();
	if( iOverlappedRegionSize == 0 )
	{
		if( !touchedRegions.empty() )
		{
			touchedRegions[0]->Union(pRegion);
			for( int i = 1, iTouchedRegionCount(touchedRegions.size()); i < iTouchedRegionCount; ++i )				
			{
				touchedRegions[0]->Union(touchedRegions[i]);
				RemoveRegion(touchedRegions[i]);
			}
		}
		else
		{
			AddRegion(pRegion->Clone());
		}
		InvalidateAABB(m_ShelfID);
		return true;
	}
	else if( iOverlappedRegionSize == 1 && pRegion->IncludeAllEdges(overlappedRegions[0]) )
	{
		CBrushRegion::RegionPtr pInputRegion = pRegion->Clone();
		pInputRegion->Subtract( overlappedRegions[0] );
		if( pInputRegion->IsValid() )
		{
			pInputRegion->Flip();
			*overlappedRegions[0] = *pInputRegion;
			AddRegionSeparately(overlappedRegions[0],true);
		}
		else
		{
			std::vector<CBrushRegion::RegionPtr>::iterator ii = m_Regions[m_ShelfID].begin();
			for( ; ii != m_Regions[m_ShelfID].end(); ++ii )
			{
				if( *ii == overlappedRegions[0] )
				{
					RemoveRegion(ii);
					break;
				}
			}
		}
		InvalidateAABB(m_ShelfID);
		return true;
	}
	else
	{
		for( int i = 0; i < iOverlappedRegionSize; ++i )
		{
			if( pRegion->IncludeAllEdges(overlappedRegions[i]) )
			{
				std::vector<CBrushRegion::RegionPtr>::iterator ii = m_Regions[m_ShelfID].begin();
				for( ; ii != m_Regions[m_ShelfID].end(); ++ii )
				{
					if( *ii == overlappedRegions[i] )
					{
						RemoveRegion(ii);
						InvalidateAABB(m_ShelfID);
						break;
					}
				}
			}
			else
			{
				CBrushRegion::RegionPtr pInputRegion(pRegion->Clone());
				for( int k = 0; k < iOverlappedRegionSize; ++k )
				{
					if( i == k )
						continue;
					pInputRegion->Subtract(overlappedReplicatedRegions[k]);
				}
					
				overlappedRegions[i]->Subtract(pInputRegion);
				AddRegionSeparately(overlappedRegions[i],true);

				pInputRegion->Subtract(overlappedReplicatedRegions[i]);
				if( pInputRegion->IsValid() )
				{
					pInputRegion->Flip();
					AddRegionSeparately(pInputRegion);
				}

				InvalidateAABB(m_ShelfID);
			}
		}
	}

	return true;
}

void CBrushDesigner::Replace( int nIndex, CBrushRegion::RegionPtr pRegion )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion )
		return;
	if( nIndex < 0 || nIndex >= m_Regions[m_ShelfID].size() )
		return;
	*(m_Regions[m_ShelfID][nIndex]) = *pRegion;
	InvalidateAABB(m_ShelfID);
}

bool CBrushDesigner::SplitRegion( CBrushRegion::RegionPtr pRegion )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion )
		return false;	

	if( pRegion->IsOpen() )
		return false;

	AABB aabb = pRegion->GetBoundBox();
	aabb.Expand(Vec3(0.01f,0.01f,0.01f));

	CBrushRegion::RegionPtr pEnteredRegion = pRegion->Clone();

	std::set<CBrushRegion::RegionPtr> deletedRegions;
	std::vector<CBrushRegion::RegionPtr> spannedRegions;
	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		if( m_Regions[m_ShelfID][i]->IsOpen() )
			continue;
		if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) != m_Regions[m_ShelfID][i]->CheckFlags(CBrushRegion::eRF_Mirrored) )
			continue;
		if( !aabb.IsIntersectBox(m_Regions[m_ShelfID][i]->GetBoundBox()) )
			continue;
		if( !pRegion->GetPlane().IsEquivalent(m_Regions[m_ShelfID][i]->GetPlane(),kDesignerEpsilon) )
			continue;
		if( CBrushRegion::HasIntersection(m_Regions[m_ShelfID][i],pEnteredRegion) == BUtil::eIT_Intersection )
		{
			spannedRegions.push_back(m_Regions[m_ShelfID][i]->Clone());
			deletedRegions.insert(m_Regions[m_ShelfID][i]);
		}
	}

	if( spannedRegions.empty() )
	{
		if( pEnteredRegion )
			AddRegion(pEnteredRegion);
		return true;
	}

	for( int i = 0, iSize(spannedRegions.size()); i < iSize; ++i )
	{
		if( !pEnteredRegion->Subtract(spannedRegions[i]) )
			break;
	}

	std::vector<CBrushRegion::RegionPtr> intersectedRegions;
	for( int i = 0, iSize(spannedRegions.size()); i < iSize; ++i )
	{
		CBrushRegion::RegionPtr intersectedRegion = spannedRegions[i]->Clone();
		if( intersectedRegion->Intersect(pRegion,CBrushRegion::eICEII_IncludeCoSame) )
			intersectedRegions.push_back(intersectedRegion);
	}

	std::vector<CBrushRegion::RegionPtr> subtractedRegions(spannedRegions);
	for( int i = 0, iSize(subtractedRegions.size()); i < iSize; ++i )
	{
		if( !subtractedRegions[i]->Subtract(pRegion) )
			return false;
	}

	DeleteRegions(deletedRegions);

	for( int i = 0, iSize(subtractedRegions.size()); i < iSize; ++i )
		AddRegionSeparately(subtractedRegions[i]);

	for( int i = 0, iSize(intersectedRegions.size()); i < iSize; ++i )
		AddRegionSeparately(intersectedRegions[i]);

	if( pEnteredRegion && pEnteredRegion->IsValid() )
		AddRegionSeparately(pEnteredRegion);

	return true;
}

void CBrushDesigner::AddRegionSeparately( CBrushRegion::RegionPtr pRegion, bool bAddedOnlyAsSeparated )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion->IsValid() )
		return;
	std::vector<CBrushRegion::RegionPtr> separatedRegions;
	if( pRegion->GetSeparatedRegions(separatedRegions) && separatedRegions.size() >= 2 )
	{
		for( int k = 0, iSeparatedRegionSize(separatedRegions.size()); k < iSeparatedRegionSize; ++k )
		{
			DESIGNER_ASSERT(separatedRegions[k]->IsValid());
			AddRegion(separatedRegions[k]);
			RemoveRegion(pRegion);
		}
	}
	else if( !bAddedOnlyAsSeparated )
	{
		AddRegion(pRegion);
	}
}

bool CBrushDesigner::DrillRegion( int nRegionIndex, bool bRemainFrame )
{
	CBrushRegion::RegionPtr pRegion(GetRegionPtr(nRegionIndex));
	if( pRegion == NULL )
		return false;

	bool bDrillAfterOffsetMode = m_nModeFlag&eDesignerMode_DrillAfterOffset;
	if( bRemainFrame && bDrillAfterOffsetMode )
	{
		ESurroundType surroundType = QuerySurroundType(nRegionIndex);
		bool bSurrounding = surroundType == eST_None || surroundType == eST_Surrounding || surroundType == eST_Partly;
		if( bSurrounding )
		{
			CBrushRegion::RegionPtr pScaledRegion = pRegion->Clone();
			pScaledRegion->RemoveInside();
			pScaledRegion->Scale(-CBrushDesigner::kDrillOffsetSize);
			AddRegion(pScaledRegion,eOpType_SubtractAB);
			return true;
		}
	}

	RemoveRegion(nRegionIndex);

	return true;
}

bool CBrushDesigner::DrillRegion( CBrushRegion::RegionPtr pRegion, bool bRemainFrame )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	for( int i = 0, iRegionCount(m_Regions[m_ShelfID].size()); i < iRegionCount; ++i )
	{
		if( m_Regions[m_ShelfID][i] == pRegion )
			return DrillRegion(i,bRemainFrame);
	}
	return false;
}

bool CBrushDesigner::DrillRegion( CBrushDesigner* pDesigner )
{
	for( int i = 0, iRegionCount(GetRegionSize()); i < iRegionCount; ++i )
	{
		std::vector<CBrushRegion::RegionPtr> pRegions;
		GetRegion(i)->GetSeparatedRegions(pRegions,CBrushRegion::eSR_OuterHull);
		if( pRegions.size() != 1 )
			continue;
		std::vector<CBrushRegion::RegionPtr> intersections = pDesigner->GetIntersectedParts(pRegions[0]);
		if( intersections.empty() )
			continue;

		for( int k = 0, iIntersectionCount(intersections.size()); k < iIntersectionCount; ++k )
			AddRegion(intersections[k],eOpType_SubtractAB);
	}

	return true;
}

void CBrushDesigner::RecordUndo( const char *sUndoDescription, CBaseObject* pObject ) const
{
	if( pObject == NULL )
		return;
	if( CUndo::IsRecording() )
		CUndo::Record( new CUndoDesigner(pObject, this, sUndoDescription) );
}

bool CBrushDesigner::EraseEdge( const BrushEdge3D& edge )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	std::vector<CBrushRegion::RegionPtr> neighbourRegions;

	QueryNeighbourRegionsByEdge( edge, neighbourRegions );
	if( neighbourRegions.empty() )
		return false;

#ifdef DEBUG
	if( neighbourRegions.size() > 2 )
	{
		for( int i = 0; i < neighbourRegions.size(); ++i )
		{
			for( int k = i+1; k < neighbourRegions.size(); ++k )
			{
				if( neighbourRegions[i]->IsEquivalent(neighbourRegions[k]) )
					DESIGNER_ASSERT(0);
			}
		}
	}
#endif

	if( neighbourRegions.size() == 1 && neighbourRegions[0]->IsOpen() )
	{
		int edgeIndex;
		if( !neighbourRegions[0]->IsEdgeOnCrust(edge,&edgeIndex) )
			return false;

		std::vector<CBrushRegion::RegionPtr> splittedRegions;
		neighbourRegions[0]->ClipByEdge( edgeIndex, splittedRegions );
		RemoveRegion(neighbourRegions[0]);
		if( splittedRegions.empty() )
			return true;
		for( int i = 0, iRegionSize(splittedRegions.size()); i < iRegionSize; ++i )
			AddOpenRegion( splittedRegions[i], true );
	}
	else if( neighbourRegions.size() == 1 && neighbourRegions[0]->HasEdge(edge,true) && neighbourRegions[0]->HasEdge(edge.GetInverted(),true) )
	{
		neighbourRegions[0]->RemoveEdge(edge);
		neighbourRegions[0]->RemoveEdge(edge.GetInverted());
	}
	else if( neighbourRegions.size() == 2 )
	{
		if( neighbourRegions[0]->IncludeAllEdges(neighbourRegions[1]) )
		{
			neighbourRegions[0]->Union(neighbourRegions[1]);
			neighbourRegions[1]->RemoveEdge(edge);
		}
		else if( neighbourRegions[1]->IncludeAllEdges(neighbourRegions[0]) )
		{
			neighbourRegions[1]->Union(neighbourRegions[0]);
			neighbourRegions[0]->RemoveEdge(edge);
		}
		else
		{
			CBrushRegion::RegionPtr pClone0 = neighbourRegions[0]->Clone();
			neighbourRegions[0]->Union(neighbourRegions[1]);

			RemoveRegion(neighbourRegions[1]);

			neighbourRegions[1]->Intersect(pClone0,CBrushRegion::eICEII_IncludeCoDiff);

			std::vector<CBrushRegion::RegionPtr> unconnectedRegions;
			neighbourRegions[1]->GetUnconnectedRegions(unconnectedRegions);

			for( int k = 0, iUnconnectedSize(unconnectedRegions.size()); k < iUnconnectedSize; ++k )
			{
				int edgeIndex;

				if( unconnectedRegions[k]->IsEdgeOnCrust(edge,&edgeIndex) )
				{
					std::vector<CBrushRegion::RegionPtr> splittedRegions;
					unconnectedRegions[k]->ClipByEdge(edgeIndex,splittedRegions);
					for( int i = 0, iSize(splittedRegions.size()); i < iSize; ++i )
						AddOpenRegion( splittedRegions[i], true );
				}
				else
				{
					AddOpenRegion( unconnectedRegions[k], true );
				}
			}
		}
	}
	else
	{
		return false;
	}

	return true;
}

void CBrushDesigner::QueryNeighbourRegionsByEdge( const BrushEdge3D& edge, RegionList& neighbourRegions ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	BrushVec3 edgeDir = (edge.m_v[1]-edge.m_v[0]).GetNormalized();

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		const CBrushRegion::RegionPtr& pRegion(m_Regions[m_ShelfID][i]);
		if( pRegion == NULL )
			continue;
		if( std::abs(pRegion->GetPlane().Normal().Dot(edgeDir)) > kDesignerLooseEpsilon  )
			continue;
		if( !pRegion->IsEdgeOnCrust(edge) )
			continue;
		neighbourRegions.push_back(pRegion);
	}
}

bool CBrushDesigner::HasIntersection( CBrushRegion::RegionPtr pRegion, bool bStrongCheck ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion )
		return false;

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )	
	{
		if( pRegion == m_Regions[m_ShelfID][i] )
			continue;
		if( bStrongCheck )
		{
			if( CBrushRegion::HasIntersection(m_Regions[m_ShelfID][i],pRegion) == BUtil::eIT_Intersection )
				return true;
		}
		else
		{
			if( CBrushRegion::HasIntersection(m_Regions[m_ShelfID][i],pRegion) != BUtil::eIT_None )
				return true;
		}
	}
	return false;
}

bool CBrushDesigner::HasTouched( CBrushRegion::RegionPtr pRegion ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion )
		return false;

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )	
	{
		if( pRegion == m_Regions[m_ShelfID][i] )
			continue;
		if( CBrushRegion::HasIntersection(m_Regions[m_ShelfID][i],pRegion) == BUtil::eIT_JustTouch )
			return true;
	}
	return false;
}

bool CBrushDesigner::RemoveRegion( int nRegionIndex )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( nRegionIndex < 0 || nRegionIndex >= m_Regions[m_ShelfID].size() )
		return false;
	
	RemoveRegion(m_Regions[m_ShelfID].begin() + nRegionIndex);
	InvalidateAABB(m_ShelfID);

	return true;
}

bool CBrushDesigner::RemoveRegion( CBrushRegion::RegionPtr pRegion )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		if( m_Regions[m_ShelfID][i] == pRegion )
			return RemoveRegion(i);
	}

	return false;
}

CBrushDesigner::RegionList::iterator CBrushDesigner::RemoveRegion( const RegionList::iterator& iRegion )
{
	if( *iRegion )
		GetSmoothingGroupMgr()->RemoveRegion(*iRegion);
	std::vector<CBrushRegion::RegionPtr>::iterator iNext = m_Regions[m_ShelfID].erase(iRegion);
	return iNext;
}

void CBrushDesigner::RemoveRegionsWithSpecificFlagsPlane( int nFlags, const BrushPlane* pPlane )
{
	std::vector<CBrushRegion::RegionPtr>::iterator ii = m_Regions[m_ShelfID].begin();
	for( ; ii != m_Regions[m_ShelfID].end() ; )
	{
		if( pPlane && !pPlane->IsEquivalent((*ii)->GetPlane(),kDesignerEpsilon) )
		{
			++ii;
			continue;
		}

		if( (*ii)->CheckFlags(nFlags) )
			ii = RemoveRegion(ii);
		else
			++ii;
	}
	InvalidateAABB(m_ShelfID);
}

CBrushRegion::RegionPtr CBrushDesigner::QueryRegion( REFGUID guid ) const
{
	for( int i = 0; i < BUtil::kMaxShelfCount; ++i )
	{
		for( int k = 0, iRegionCount(m_Regions[i].size()); k < iRegionCount; ++k )
		{
			if( m_Regions[i][k]->GetGUID() == guid )
				return m_Regions[i][k];
		}
	}
	return NULL;
}

CBrushRegion::RegionPtr CBrushDesigner::GetRegionPtr( int nIndex ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( nIndex >= m_Regions[m_ShelfID].size() || nIndex < 0 )
		return NULL;
	return m_Regions[m_ShelfID][nIndex];
}

bool CBrushDesigner::QueryRegion( const BrushPlane& plane, const BrushVec3& raySrc, const BrushVec3& rayDir, int& nOutIndex ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	BrushPlane invPlane = plane.GetInverted();

	BrushFloat fShortestDist(1000000.0f);
	nOutIndex = -1;
	for( int i = 0, regionSize(m_Regions[m_ShelfID].size()); i < regionSize; ++i )
	{
		CBrushRegion::RegionPtr region = m_Regions[m_ShelfID][i];
		BrushFloat t = 0;
		if( !region->GetPlane().IsEquivalent(plane,kDesignerEpsilon) && !region->GetPlane().IsEquivalent(invPlane,kDesignerEpsilon) )
			continue;
		if( region->CheckFlags(CBrushRegion::eRF_Hidden) )
			continue;
		if( region->IsPassed( raySrc, rayDir, t ) )
		{
			if( t < fShortestDist || std::abs(t-fShortestDist) < kDesignerEpsilon && region->IsOpen() )
			{
				fShortestDist = t;
				nOutIndex = i;
			}
		}
	}
	return nOutIndex != -1;
}

bool CBrushDesigner::QueryRegions( const BrushPlane& plane, std::vector<CBrushRegion::RegionPtr>& outRegions ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );
	bool bAdded = false;
	for( int i = 0, iRegionCount(m_Regions[m_ShelfID].size()); i < iRegionCount; ++i )
	{
		if( m_Regions[m_ShelfID][i]->GetPlane().IsEquivalent(plane,kDesignerEpsilon) )
		{
			bAdded = true;
			outRegions.push_back(m_Regions[m_ShelfID][i]);
		}
	}
	return bAdded;
}

bool CBrushDesigner::QueryIntersectedRegionsByAABB( const AABB& aabb, RegionList& outRegions ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );
	bool bAdded = false;
	for( int i = 0, iRegionCount(m_Regions[m_ShelfID].size()); i < iRegionCount; ++i )
	{
		if( m_Regions[m_ShelfID][i]->IntersectedBetweenAABBs(aabb) )
		{
			bAdded = true;
			outRegions.push_back(m_Regions[m_ShelfID][i]);
		}
	}
	return bAdded;
}

bool CBrushDesigner::QueryRegion( const BrushVec3& raySrc, const BrushVec3& rayDir, int& nOutIndex ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	BrushFloat fShortestDist(1000000.0f);
	nOutIndex = -1;
	for( int i = 0, regionSize(m_Regions[m_ShelfID].size()); i < regionSize; ++i )
	{
		CBrushRegion::RegionPtr pRegion = m_Regions[m_ShelfID][i];

		if( !pRegion->IsValid() || pRegion->CheckFlags(CBrushRegion::eRF_Hidden) )
			continue;

		BrushFloat t = 0;
		if( pRegion->IsPassed( raySrc, rayDir, t ) )
		{
			BrushFloat fDistanceFromPlane = pRegion->GetPlane().Distance(raySrc);
			BrushFloat fAbsShortestDistance = std::abs(fShortestDist);
			if( std::abs(t-fAbsShortestDistance) < kDesignerEpsilon && fDistanceFromPlane >= 0 || t < fAbsShortestDistance )
			{			
				fShortestDist = t;
				if( fDistanceFromPlane < 0 )
					fShortestDist = -fShortestDist;
				nOutIndex = i;
			}
		}
	}
	return nOutIndex != -1;
}

bool CBrushDesigner::QueryCenterOfRegion( const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outCenterOfPos ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	int nRegionIndex(0);	
	if( !QueryRegion( raySrc,rayDir, nRegionIndex ) )
		return false;

	outCenterOfPos = m_Regions[m_ShelfID][nRegionIndex]->GetBoundBox().GetCenter();
	return true;
}

bool CBrushDesigner::QueryAdjacentRegionsByEdge( const BrushEdge3D& edge, std::vector<CBrushRegion::RegionPtr>& outRegions ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	bool bAdded = false;
	for( int i = 0, iRegionCount(m_Regions[m_ShelfID].size()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = m_Regions[m_ShelfID][i];
		if( pRegion->HasEdge(edge) )
		{
			bAdded = true;
			outRegions.push_back(pRegion);
		}
	}

	return bAdded;
}

bool CBrushDesigner::QueryNearestPosFromBoundary( const BrushVec3& pos, BrushVec3& outNearestPos ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	BrushFloat fNearestDistance(BUtil::kEnoughBigNumber);
	BrushVec3 NearestPos;
	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		CBrushRegion::RegionPtr region(m_Regions[m_ShelfID][i]);
		BrushVec3 NearestPosOnWorld;
		if( !m_Regions[m_ShelfID][i]->QueryNearestPosFromBoundary(pos,NearestPosOnWorld) )
			continue;
		BrushFloat fSqDistance((NearestPosOnWorld-pos).GetLengthSquared());
		if( fSqDistance < fNearestDistance )
		{
			fNearestDistance = fSqDistance;
			outNearestPos = NearestPosOnWorld;
		}
	}
	return fNearestDistance < BUtil::kEnoughBigNumber;
}

CBrushDesigner::ESurroundType CBrushDesigner::QuerySurroundType( int nRegionIndex ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( nRegionIndex < 0 || nRegionIndex >= m_Regions[m_ShelfID].size() )
		return eST_WrongInput;

	CBrushRegion::RegionPtr pUnionRegion = new CBrushRegion;

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		if( i == nRegionIndex )
			continue;
		if( !m_Regions[m_ShelfID][nRegionIndex]->IsPlaneEquivalent(m_Regions[m_ShelfID][i]) )
			continue;
		if( CBrushRegion::HasIntersection(m_Regions[m_ShelfID][nRegionIndex],m_Regions[m_ShelfID][i]) != BUtil::eIT_JustTouch )
			continue;
		pUnionRegion->Union(m_Regions[m_ShelfID][i]);
	}

	if( !pUnionRegion->IsValid() )
		return eST_None;

	pUnionRegion->RemoveInside();

	if( pUnionRegion->IncludeAllEdges(m_Regions[m_ShelfID][nRegionIndex]) )
		return eST_Surrounded;

	CBrushRegion::RegionPtr pInputRegion = m_Regions[m_ShelfID][nRegionIndex]->Clone();
	pInputRegion->RemoveInside();
	if( pInputRegion->IncludeAllEdges(pUnionRegion) )
		return eST_Surrounding;

	return eST_Partly;
}

bool CBrushDesigner::QueryNearestEdges( const BrushPlane& plane, const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outPos, BrushVec3& outPosOnEdge, std::vector<SQueryEdgeResult>& outEdges ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	float fShorestDist = 3e10f;
	BrushVec3 posOnEdge;

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		const CBrushRegion::RegionPtr pRegion(m_Regions[m_ShelfID][i]);
		if( pRegion == NULL )
			continue;
		if( !pRegion->GetPlane().IsEquivalent(plane,kDesignerEpsilon) )
			continue;
		BrushPlane p;
		BrushEdge3D edge;
		if( !QueryNearestEdge( i, raySrc, rayDir, outPos, posOnEdge, p, edge ) )
			continue;
		BrushFloat fDistance = outPos.GetDistance(posOnEdge);
		if( std::abs(fDistance-fShorestDist) < kDesignerEpsilon )
		{
			outEdges.push_back(SQueryEdgeResult(pRegion,edge));
		}
		else if( fDistance < fShorestDist )
		{
			fShorestDist = fDistance;
			outPosOnEdge = posOnEdge;
			outEdges.clear();
			outEdges.push_back(SQueryEdgeResult(pRegion,edge));
		}
	}
	return fShorestDist < 3e9f;
}

bool CBrushDesigner::QueryNearestEdges( const BrushPlane& plane, const BrushVec3& position, BrushVec3& outPosOnEdge, std::vector<SQueryEdgeResult>& outEdges ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	float fShorestDist = 3e10f;
	BrushVec3 posOnEdge;

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		const CBrushRegion::RegionPtr pRegion(m_Regions[m_ShelfID][i]);
		if( pRegion == NULL )
			continue;
		if( !pRegion->GetPlane().IsEquivalent(plane,kDesignerEpsilon) )
			continue;
		BrushPlane p;
		BrushEdge3D edge;
		if( !QueryNearestEdge( i, position, posOnEdge, p, edge ) )
			continue;
		BrushFloat fDistance = position.GetDistance(posOnEdge);
		if( std::abs(fDistance-fShorestDist) < kDesignerEpsilon )
		{
			outEdges.push_back(SQueryEdgeResult(pRegion,edge));
		}
		else if( fDistance < fShorestDist )
		{
			fShorestDist = fDistance;
			outPosOnEdge = posOnEdge;
			outEdges.clear();
			outEdges.push_back(SQueryEdgeResult(pRegion,edge));
		}
	}
	return fShorestDist < 3e9f;
}

bool CBrushDesigner::QueryNearestEdges( const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outPos, BrushVec3& outPosOnEdge, BrushPlane& outPlane, std::vector<SQueryEdgeResult>& outEdges ) const
{
	int nRegionIndex(-1);
	if( QueryRegion(raySrc,rayDir,nRegionIndex) == false )
		return false;
	outPlane = GetRegion(nRegionIndex)->GetPlane();
	return QueryNearestEdges(outPlane, raySrc, rayDir, outPos, outPosOnEdge, outEdges);
}

bool CBrushDesigner::QueryNearestEdge( int nRegionIndex, const BrushVec3& raySrc, const BrushVec3& rayDir, BrushVec3& outPos, BrushVec3& outPosOnEdge, BrushPlane& outPlane, BrushEdge3D& outEdge ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	CBrushRegion::RegionPtr pRegion(m_Regions[m_ShelfID][nRegionIndex]);
	outPlane = pRegion->GetPlane();

	if( !outPlane.HitTest( raySrc, raySrc+rayDir, kDesignerEpsilon, NULL, &outPos ) )
		return false;

	BrushVec3 posOnEdge;
	BrushEdge3D edge;
	if( pRegion->QueryNearestEdge( outPos, edge, posOnEdge ) == false )
		return false;

	outEdge = edge;
	outPosOnEdge = posOnEdge;

	return true;
}

bool CBrushDesigner::QueryNearestEdge( int nRegionIndex, const BrushVec3& position, BrushVec3& outPosOnEdge, BrushPlane& outPlane, BrushEdge3D& outEdge ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	CBrushRegion::RegionPtr pRegion(m_Regions[m_ShelfID][nRegionIndex]);
	outPlane = pRegion->GetPlane();

	BrushVec3 posOnEdge;
	BrushEdge3D edge;
	if( pRegion->QueryNearestEdge( position, edge, posOnEdge ) == false )
		return false;

	outEdge = edge;
	outPosOnEdge = posOnEdge;

	return true;
}

void CBrushDesigner::QueryAdjacentPerpendicularRegions( CBrushRegion::RegionPtr pRegion, std::vector<CBrushRegion::RegionPtr>& outRegions ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion )
		return;

	for( int i = 0, iRegionCount(m_Regions[m_ShelfID].size()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pCandidateRegion = m_Regions[m_ShelfID][i];
		if( std::abs(pRegion->GetPlane().Normal().Dot(pCandidateRegion->GetPlane().Normal())) > kDesignerEpsilon )
			continue;
		for( int a = 0, iCandidateEdgeCount(pCandidateRegion->GetEdgeSize()); a < iCandidateEdgeCount; ++a )
		{
			BrushEdge3D candidateEdge3D = pCandidateRegion->GetEdge(a);
			if( pRegion->IsEdgeOnCrust(candidateEdge3D) )
			{
				outRegions.push_back(pCandidateRegion);
				break;
			}
		}
	}
}

void CBrushDesigner::QueryPerpendicularRegions( CBrushRegion::RegionPtr pRegion, std::vector<CBrushRegion::RegionPtr>& outRegions ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion )
		return;

	AABB aabb = pRegion->GetBoundBox();
	aabb.Expand(Vec3(0.01f,0.01f,0.01f));

	for( int i = 0, iRegionCount(m_Regions[m_ShelfID].size()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pCandidateRegion = m_Regions[m_ShelfID][i];
		if( pCandidateRegion == pRegion || pCandidateRegion->IsOpen() )
			continue;

		BrushFloat normalDot = std::abs(pRegion->GetPlane().Normal().Dot(pCandidateRegion->GetPlane().Normal()));
		if( normalDot > kDesignerEpsilon )
			continue;

		for( int a = 0, iVertexCount(pRegion->GetVertexListSize()); a < iVertexCount; ++a )
		{
			const BrushVec3& v = pRegion->GetVertex(a);
			BrushFloat fAbsDist = std::abs(pCandidateRegion->GetPlane().Distance(v));
			if( fAbsDist < kDistanceLimitation )
			{
				outRegions.push_back(pCandidateRegion);
				break;
			}
		}
	}
}

void CBrushDesigner::Clear()
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	RegionList::iterator ii = m_Regions[m_ShelfID].begin();
	for( ;ii != m_Regions[m_ShelfID].end(); )
		ii = RemoveRegion(ii);

	InvalidateAABB();
}

void CBrushDesigner::Display( DisplayContext& dc, const int nLineThickness, const ColorB& lineColor )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );
	int numOfRegions(m_Regions[m_ShelfID].size());
	DisplayRegions(dc,nLineThickness,lineColor);
	DisplaySubdividedMesh(dc);
}

void CBrushDesigner::DisplayRegions( DisplayContext& dc, const int nLineThickness, const ColorB& lineColor )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );
	DESIGNER_SHELF_RECONSTRUCTOR(this);

	int oldThickness = dc.GetLineWidth();
	dc.SetLineWidth(nLineThickness);
	dc.SetColor( lineColor );

	for( BUtil::ShelfID shelfID = 0; shelfID < BUtil::kMaxShelfCount; ++shelfID )
	{
		SetShelf(shelfID);

		int iRegionSize(m_Regions[shelfID].size());

		for( int i = 0; i < iRegionSize; ++i )
		{
			CBrushRegion::RegionPtr pRegion = m_Regions[m_ShelfID][i];

			if( !pRegion->IsValid() || pRegion->CheckFlags(CBrushRegion::eRF_Hidden|CBrushRegion::eRF_Mirrored) )
				continue;

			for( int k = 0, iEdgeSize(pRegion->GetEdgeSize()); k < iEdgeSize; ++k )
			{
				const BUtil::SEdge& rEdge = pRegion->GetEdgeIndexPair(k);
				BrushEdge3D edge(pRegion->GetVertex(rEdge.m_i[0]), pRegion->GetVertex(rEdge.m_i[1]));
				std::vector<BrushEdge3D> visibleParts;
				if( !GetVisibleEdge(edge,pRegion->GetPlane(),visibleParts) )
					continue;
				if( !visibleParts.empty() )
				{
					for( int i = 0, iVisibleSize(visibleParts.size()); i < iVisibleSize; ++i )
						dc.DrawLine(visibleParts[i].m_v[0], visibleParts[i].m_v[1]);
				}
				else
				{
					dc.DrawLine(edge.m_v[0], edge.m_v[1]);
				}
			}
		}
	}
	dc.SetLineWidth(oldThickness);
}

void CBrushDesigner::DisplaySubdividedMesh( DisplayContext& dc )
{
	if( !m_pSubdividionResult )
		return;

	dc.SetColor(ColorB(150,150,150));
	dc.SetLineWidth(2);

	for( int i = 0, iFaceCount(m_pSubdividionResult->GetFaceCount()); i < iFaceCount; ++i )
	{
		const HE_Face& f = m_pSubdividionResult->GetFace(i);
		std::vector<BrushVec3> vertices;
		m_pSubdividionResult->GetFaceVertices(f,vertices);
		for( int k = 0, iVertexCount(vertices.size()); k < iVertexCount; ++k )
		{
			const BrushVec3& v0 = vertices[k];
			const BrushVec3& v1 = vertices[(k+1)%iVertexCount];

			dc.DrawLine(ToVec3(v0),ToVec3(v1));
		}
	}
}

bool CBrushDesigner::GetVisibleEdge( const BrushEdge3D& edge, const BrushPlane& plane, std::vector<BrushEdge3D>& outVisibleEdges ) const
{
	BrushLine line( plane.W2P(edge.m_v[0]), plane.W2P(edge.m_v[1]) );
	BrushLine invLine(line.GetInverted());

	std::vector<int> indicesOnCoLine;

	for( int i = 0, iSize(m_ExcludedEdgesInDrawing.size()); i < iSize; ++i )
	{
		BrushLine lineFromRejected( plane.W2P(m_ExcludedEdgesInDrawing[i].m_v[0]), plane.W2P(m_ExcludedEdgesInDrawing[i].m_v[1]) );
		if( !line.IsEquivalent(lineFromRejected,kDesignerEpsilon) && !invLine.IsEquivalent(lineFromRejected,kDesignerEpsilon) )
			continue;

		bool bEquivalent = edge.IsEquivalent(m_ExcludedEdgesInDrawing[i],kDesignerEpsilon);
		bool bInverseEquivalent = !bEquivalent ? edge.m_v[0].IsEquivalent(m_ExcludedEdgesInDrawing[i].m_v[1],kDesignerEpsilon) && edge.m_v[1].IsEquivalent(m_ExcludedEdgesInDrawing[i].m_v[0],kDesignerEpsilon) : true;
		bool bInside = !bInverseEquivalent ? m_ExcludedEdgesInDrawing[i].ContainVertex(edge.m_v[0],kDesignerEpsilon) && m_ExcludedEdgesInDrawing[i].ContainVertex(edge.m_v[1],kDesignerEpsilon) : true;
		if( bEquivalent || bInverseEquivalent || bInside )
			return false;

		indicesOnCoLine.push_back(i);
	}

	for( int i = 0, iSize(indicesOnCoLine.size()); i < iSize; ++i )
	{
		if( edge.GetSubtractedEdges(m_ExcludedEdgesInDrawing[indicesOnCoLine[i]],outVisibleEdges,kDesignerEpsilon) )
			break;
	}

	return true;
}

void CBrushDesigner::DeleteRegions( std::set<CBrushRegion::RegionPtr>& deletedRegions )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !deletedRegions.empty() )
	{
		std::vector<CBrushRegion::RegionPtr>::iterator ii = m_Regions[m_ShelfID].begin();
		for( ; ii != m_Regions[m_ShelfID].end(); )
		{
			if( deletedRegions.find(*ii) != deletedRegions.end() )
				ii = RemoveRegion(ii);
			else
				++ii;
		}
		InvalidateAABB(m_ShelfID);
	}
}

bool CBrushDesigner::GetRegionPlane( int nRegionIndex, BrushPlane& outPlane ) const
{
	CBrushRegion::RegionPtr pRegion(GetRegionPtr(nRegionIndex));
	if( pRegion == NULL )
		return false;

	outPlane = pRegion->GetPlane();
	return true;
}

void CBrushDesigner::Serialize( XmlNodeRef &xmlNode, bool bLoading, bool bUndo )
{
	if( bLoading )
	{
		int childNum = xmlNode->getChildCount();

		m_Regions[0].clear();
		m_Regions[1].clear();
		m_Regions[0].reserve(childNum);

		for( int i = 0; i < childNum; ++i )
		{
			XmlNodeRef pChildNode = xmlNode->getChild(i);
			DESIGNER_ASSERT(pChildNode);
			if( !pChildNode )
				continue;
			if( !strcmp( pChildNode->getTag(), "Region") )
			{				
				unsigned int nRegionFlag = 0;
				pChildNode->getAttr("Flags", nRegionFlag);
				static const int kBackFaceFlag = BIT(2);
				if( !(nRegionFlag&kBackFaceFlag) )
				{
					CBrushRegion::RegionPtr pRegion = new CBrushRegion;
					pRegion->Serialize(pChildNode, bLoading, bUndo);
					AddRegion(0,pRegion);
				}
			}
			else if( !strcmp(pChildNode->getTag(), "SmoothingGroups") )
			{
				GetSmoothingGroupMgr()->Serialize(pChildNode,bLoading,bUndo,this);
			}
			else if( !strcmp(pChildNode->getTag(), "SemiSharpCrease"))
			{
				GetEdgeSharpnessMgr()->Serialize(pChildNode,bLoading,bUndo,this);
			}
		}

		xmlNode->getAttr("DesignerModeFlags",m_nModeFlag);
		xmlNode->getAttr("SubdivisionLevel",m_SubdivisionLevel);
		xmlNode->getAttr("TessFactor",m_nTessFactor);

		BrushVec3 mirrorPlaneNormal;
		BrushFloat mirrorPlaneDistance;
		if( xmlNode->getAttr("MirrorPlaneNormal", mirrorPlaneNormal) && xmlNode->getAttr("MirrorPlaneDistance", mirrorPlaneDistance) )
			m_MirrorPlane = BrushPlane(mirrorPlaneNormal,mirrorPlaneDistance);

		InvalidateAABB();
	}
	else
	{
		for( int i = 0, iSize(m_Regions[0].size()); i < iSize; ++i )
		{
			if( m_Regions[0][i] )
			{
				XmlNodeRef regionNode(xmlNode->newChild("Region"));
				m_Regions[0][i]->Serialize(regionNode, bLoading, bUndo);
			}
			else
			{
				DESIGNER_ASSERT(0);
			}
		}

		XmlNodeRef smoothingGroupsNode(xmlNode->newChild("SmoothingGroups"));
		GetSmoothingGroupMgr()->Serialize(smoothingGroupsNode,bLoading,bUndo,this);

		XmlNodeRef semiSharpCreaseNode(xmlNode->newChild("SemiSharpCrease"));
		GetEdgeSharpnessMgr()->Serialize(semiSharpCreaseNode,bLoading,bUndo,this);

		xmlNode->setAttr("DesignerModeFlags",m_nModeFlag);
		xmlNode->setAttr("SubdivisionLevel",m_SubdivisionLevel);
		xmlNode->setAttr("TessFactor",m_nTessFactor);
		xmlNode->setAttr("MirrorPlaneNormal", m_MirrorPlane.Normal());
		xmlNode->setAttr("MirrorPlaneDistance", m_MirrorPlane.Distance());
	}
}

void CBrushDesigner::Save( CArchive& ar )
{
	DESIGNER_SHELF_RECONSTRUCTOR(this);
	SetShelf(0);
	ar.Write(&m_nModeFlag,sizeof(int));
	BrushVec3 mirrorPlaneNormal = m_MirrorPlane.Normal();
	BrushFloat mirrorPlaneDistance = m_MirrorPlane.Distance();
	ar.Write( &mirrorPlaneNormal, sizeof(BrushVec3) );
	ar.Write( &mirrorPlaneDistance, sizeof(BrushFloat) );
	int nRegionCount = GetRegionSize();
	ar.Write(&nRegionCount,sizeof(int));
	for( int i = 0; i < nRegionCount; ++i )
		GetRegion(i)->SaveBinary(ar);
}

void CBrushDesigner::Load( CArchive& ar )
{
	DESIGNER_SHELF_RECONSTRUCTOR(this);
	SetShelf(0);
	Clear();
	ar.Read(&m_nModeFlag,sizeof(int));
	BrushVec3 mirrorPlaneNormal;
	BrushFloat mirrorPlaneDistance;
	ar.Read( &mirrorPlaneNormal, sizeof(BrushVec3) );
	ar.Read( &mirrorPlaneDistance, sizeof(BrushFloat) );
	m_MirrorPlane = BrushPlane(mirrorPlaneNormal,mirrorPlaneDistance);
	int nRegionCount = GetRegionSize();
	ar.Read(&nRegionCount,sizeof(int));
	for( int i = 0; i < nRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = new CBrushRegion;
		pRegion->LoadBinary(ar);
		AddRegion(m_ShelfID,pRegion);
	}
	InvalidateAABB();
}

CBrushRegion::RegionPtr CBrushDesigner::GetRegion( int nRegionIndex ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( nRegionIndex < 0 || nRegionIndex >= m_Regions[m_ShelfID].size() )
		return NULL;
	return m_Regions[m_ShelfID][nRegionIndex];
}

int CBrushDesigner::GetRegionIndex( CBrushRegion::RegionPtr pRegion ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	for( int i = 0, iRegionCount(m_Regions[m_ShelfID].size()); i < iRegionCount; ++i )
	{
		if( m_Regions[m_ShelfID][i] == pRegion )
			return i;
	}

	return -1;
}

void CBrushDesigner::GetRegions( const BrushPlane& plane, std::vector<int>& outRegionIndices ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	for( int i = 0, nRegionSize(m_Regions[m_ShelfID].size()); i < nRegionSize; ++i )
	{
		if( plane.IsEquivalent(m_Regions[m_ShelfID][i]->GetPlane(),kDesignerEpsilon) )
			outRegionIndices.push_back(i);
	}
}

CBrushDesigner::ERegionRelation CBrushDesigner::QueryOppositeRegion( CBrushRegion::RegionPtr pRegion, EFindOppositeFlag nFlag, BrushFloat fScale, CBrushRegion::RegionPtr& outRegion, BrushFloat& outDistance ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	if( !pRegion )
		return eER_None;

	BrushPlane plane(pRegion->GetPlane()); 
	BrushPlane invertedPlane(plane);
	invertedPlane.Invert();

	BrushVec3 centerOfRegion(pRegion->GetCenterPosition());

	BrushFloat nearestDistance(3e10f);
	CBrushRegion::RegionPtr pNearestRegion;

	for( int i = 0, nRegionSize(m_Regions[m_ShelfID].size()); i < nRegionSize; ++i )
	{
		const BrushPlane& oppositePlane(m_Regions[m_ShelfID][i]->GetPlane());

		if( oppositePlane.IsEquivalent(plane,kDesignerEpsilon) || oppositePlane.IsEquivalent(invertedPlane,kDesignerEpsilon) )
			continue;

		if( !m_Regions[m_ShelfID][i]->IsValid() || m_Regions[m_ShelfID][i]->IsOpen() )
			continue;

		BrushVec3 v(m_Regions[m_ShelfID][i]->GetVertex(0));
		BrushFloat dist(0);
		if( !plane.HitTest( v, v+oppositePlane.Normal(),kDesignerEpsilon, &dist ) )
			continue;

		if( std::abs(fScale) < kDesignerEpsilon )
		{
			if( oppositePlane.Normal().Dot(plane.Normal()) > -kDesignerEpsilon )
				continue;

			if( nFlag == eFOF_PushDirection )
			{
				if( dist > 0 )
					continue;
			}
			else
			{
				if( dist < 0 )
					continue;
			}
		}
		else
		{
			if( oppositePlane.Normal().Dot(plane.Normal()) < 1-kDesignerEpsilon )
				continue;

			if( nFlag == eFOF_PushDirection )
			{
				if( dist < 0 )
					continue;
			}
			else
			{
				if( dist > 0 )
					continue;
			}
		}

		CBrushRegion::RegionPtr pIntersectionRegion = m_Regions[m_ShelfID][i]->Clone();

		if( !pIntersectionRegion->UpdatePlane(pRegion->GetPlane()) )
			continue;

		if( std::abs(fScale) > kDesignerEpsilon )
		{
			if( !pIntersectionRegion->Scale(-fScale) )
			{
#ifdef DEBUG
				m_Regions[m_ShelfID][i]->OutputDebugData();
				pIntersectionRegion->OutputDebugData();
#endif
				continue;
			}
		}

		pIntersectionRegion->Intersect(pRegion,CBrushRegion::eICEII_IncludeCoSame);
		if( pIntersectionRegion->IsValid() == false )
			continue;

		BrushVec3 direction = (nFlag == eFOF_PushDirection) ? pRegion->GetPlane().Normal() : -pRegion->GetPlane().Normal();
		BrushFloat distance(0);
		if( !pIntersectionRegion->UpdatePlane(m_Regions[m_ShelfID][i]->GetPlane(),direction) )
			continue;
		distance = pRegion->GetNearestDistance(pIntersectionRegion,direction);

		if( distance >= 0 && distance < nearestDistance )
		{
			nearestDistance = distance;
			pNearestRegion = pIntersectionRegion;
		}
	}

	if( nearestDistance == 3e10f )
	{
		outRegion = NULL;
		return eER_None;
	}

	outDistance = nearestDistance;
	outRegion = pNearestRegion;

	if( std::abs(outDistance) < kDesignerEpsilon*10.0f )
		return eER_ZeroDistance;

	if( std::abs(plane.Normal().Dot(outRegion->GetPlane().Normal())) < 1-kDesignerEpsilon )
		outDistance = nearestDistance-0.01f;

	return eER_Intersection;
}

void CBrushDesigner::QueryIntersectionByRegion( CBrushRegion::RegionPtr pRegion, std::vector<CBrushRegion::RegionPtr>& outIntersetionRegions ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );
	if( !pRegion )
		return;

	AABB bb = pRegion->GetBoundBox();
	bb.Expand(Vec3(0.01f,0.01f,0.01f));

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		if( m_Regions[m_ShelfID][i]->CheckFlags(CBrushRegion::eRF_Mirrored) )
			continue;
		if( !m_Regions[m_ShelfID][i]->GetBoundBox().IsIntersectBox(bb) )
			continue;
		if( !m_Regions[m_ShelfID][i]->GetPlane().IsEquivalent(pRegion->GetPlane(),kDesignerEpsilon) )
			continue;
		if( CBrushRegion::HasIntersection(m_Regions[m_ShelfID][i],pRegion) == BUtil::eIT_Intersection )
			outIntersetionRegions.push_back(m_Regions[m_ShelfID][i]);
	}
}

void CBrushDesigner::QueryIntersectionRegionsWith2DRect( IDisplayViewport* pView, const BrushMatrix34& worldTM, CBrushRegion::RegionPtr pRectRegion, bool bExcludeBackFace, RegionList& outIntersectionRegions ) const
{
	for( int i = 0, iRegionCount(m_Regions[m_ShelfID].size()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = m_Regions[m_ShelfID][i];
		if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored|CBrushRegion::eRF_Hidden) )
			continue;
		if( pRegion->InRectangle(pView,worldTM,pRectRegion,bExcludeBackFace) )
			outIntersectionRegions.push_back(pRegion);
	}
}

void CBrushDesigner::QueryIntersectionEdgesWith2DRect( IDisplayViewport* pView, const BrushMatrix34& worldTM, CBrushRegion::RegionPtr pRectRegion, bool bExcludeBackFace, BUtil::EdgeQueryResult& outIntersectionEdges ) const
{
	for( int i = 0, iRegionCount(m_Regions[m_ShelfID].size()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = m_Regions[m_ShelfID][i];
		if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored|CBrushRegion::eRF_Hidden) )
			continue;
		pRegion->QueryIntersectionEdgesWith2DRect(pView,worldTM,pRectRegion,bExcludeBackFace,outIntersectionEdges);
	}
}

void CBrushDesigner::QueryIntersectionByEdge( const BrushEdge3D& edge, std::vector<IntersectionPair>& outIntersections ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );
	std::map<BrushFloat,IntersectionPair> sortedIntersections;

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		if( m_Regions[m_ShelfID][i]->CheckFlags(CBrushRegion::eRF_Mirrored) )
			continue;
		std::map<BrushFloat,BrushVec3> regionIntersections;
		m_Regions[m_ShelfID][i]->QueryIntersections(edge,regionIntersections);

		std::map<BrushFloat,BrushVec3>::iterator ii = regionIntersections.begin();
		for( ; ii != regionIntersections.end(); ++ii )
		{
			std::map<BrushFloat,IntersectionPair>::iterator iSorted = sortedIntersections.begin();
			bool bIdenticalExist = false;
			for( ;iSorted != sortedIntersections.end(); ++iSorted )
			{
				const IntersectionPair& intersection = iSorted->second;
				if( intersection.second.IsEquivalent(ii->second,kDesignerEpsilon) )
				{
					bIdenticalExist = true;
					break;
				}
			}
			if( !bIdenticalExist )
				sortedIntersections[ii->first] = IntersectionPair(m_Regions[m_ShelfID][i],ii->second);
		}
	}

	std::map<BrushFloat,IntersectionPair>::iterator iter = sortedIntersections.begin();
	for( ; iter != sortedIntersections.end(); ++iter )
		outIntersections.push_back(iter->second);
}

void CBrushDesigner::Optimize()
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	std::vector<CBrushRegion::RegionPtr>::iterator ii = m_Regions[m_ShelfID].begin();
	for( ; ii != m_Regions[m_ShelfID].end(); )
	{
		if( !(*ii)->IsValid() )
			ii = RemoveRegion(ii);
		else
			++ii;
	}

	std::vector<CBrushRegion::RegionPtr> regions = m_Regions[m_ShelfID];
	for( int i = 0, iRegionSize(regions.size()); i < iRegionSize; ++i )
	{
		if( regions[i]->IsOpen() )
			continue;
		AddRegionSeparately(regions[i],true);
	}
}

void CBrushDesigner::SeparateRegions( const BrushPlane& plane )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	std::vector<CBrushRegion::RegionPtr> regions = m_Regions[m_ShelfID];
	for( int i = 0, iRegionSize(regions.size()); i < iRegionSize; ++i )
	{
		if( regions[i]->IsOpen() )
			continue;
		if( !plane.IsEquivalent(regions[i]->GetPlane(), kDesignerEpsilon) )
			continue;
		AddRegionSeparately(regions[i],true);
	}
}

void CBrushDesigner::Move( const BrushVec3& offset )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		m_Regions[m_ShelfID][i]->Move(offset);
		GetSmoothingGroupMgr()->InvalidateSmoothingGroup(m_Regions[m_ShelfID][i]);
	}

	InvalidateAABB(m_ShelfID);
}

void CBrushDesigner::Transform( const BrushMatrix34& tm )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		m_Regions[m_ShelfID][i]->Transform(tm);
		GetSmoothingGroupMgr()->InvalidateSmoothingGroup(m_Regions[m_ShelfID][i]);
	}

	InvalidateAABB(m_ShelfID);
}

bool CBrushDesigner::IsVertexOnEdge( const BrushPlane& plane, const BrushVec3& vertex, CBrushRegion::RegionPtr pExcludedRegion ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		CBrushRegion::RegionPtr pRegion = m_Regions[m_ShelfID][i];
		if( pRegion == NULL )
			continue;
		if( pExcludedRegion && pExcludedRegion == pRegion )
			continue;
		const BrushPlane& regionPlane = pRegion->GetPlane();
		if( !plane.IsEquivalent(regionPlane,kDesignerEpsilon) )
			continue; 
		BrushEdge3D nearestEdge;
		BrushVec3 hitPos;
		if( !pRegion->QueryNearestEdge(vertex, nearestEdge, hitPos) )
			continue;
		if( (vertex-hitPos).GetLength() < kDesignerEpsilon )
			return true;
	}
	return false;
}

void CBrushDesigner::GetRegionList( std::vector<CBrushRegion::RegionPtr>& outExportedRegionList ) const
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );
	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		if( m_Regions[m_ShelfID][i]->IsOpen() )
			continue;
		outExportedRegionList.push_back(m_Regions[m_ShelfID][i]);
	}
}

void CBrushDesigner::MoveShelf( BUtil::ShelfID sourceShelfID, BUtil::ShelfID destShelfID )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	DESIGNER_SHELF_RECONSTRUCTOR(this);

	for( int i = 0, iRegionCount(m_Regions[sourceShelfID].size()); i < iRegionCount; ++i )
		AddRegion(destShelfID,m_Regions[sourceShelfID][i]);

	SetShelf(sourceShelfID);
	Clear();
	InvalidateAABB();
}

bool CBrushDesigner::SplitRegionsByOpenRegion( CBrushRegion::RegionPtr pOpenRegion )
{
	DESIGNER_ASSERT( m_ShelfID >= 0 && m_ShelfID < BUtil::kMaxShelfCount );

	std::set<CBrushRegion::RegionPtr> spannedRegions;
	for( int i = 0, iSize(m_Regions[m_ShelfID].size()); i < iSize; ++i )
	{
		if( m_Regions[m_ShelfID][i]->IsOpen() )
			continue;
		BUtil::EIntersectionType intersectionType = CBrushRegion::HasIntersection(m_Regions[m_ShelfID][i],pOpenRegion);
		if( intersectionType == BUtil::eIT_Intersection )
			spannedRegions.insert(m_Regions[m_ShelfID][i]);
	}

	if( spannedRegions.empty() )
	{
		AddRegion(pOpenRegion->Clone());
		return true;
	}

	std::vector<CBrushRegion::RegionPtr>::iterator ii;
	for(ii = m_Regions[m_ShelfID].begin(); ii!=m_Regions[m_ShelfID].end(); )
	{
		if( spannedRegions.find(*ii) != spannedRegions.end() )
			ii = RemoveRegion(ii);
		else
			++ii;
	}

	CBrushRegion::RegionPtr pRearrangedOpenRegion(pOpenRegion->Clone());
	pRearrangedOpenRegion->Rearrange();

	std::set<CBrushRegion::RegionPtr>::iterator iRegion = spannedRegions.begin();
	for( ; iRegion != spannedRegions.end(); ++iRegion )
	{
		CBrushRegion::RegionPtr pSpannedRegion = (*iRegion);
		CBrushRegion::RegionPtr pSpannedRegionWithoutBridgeEdges = (*iRegion)->Clone();
		pSpannedRegionWithoutBridgeEdges->RemoveBridgeEdges();

		bool hHasHoles = pSpannedRegionWithoutBridgeEdges->HasHoles();

		CBrushRegion::RegionPtr pCulledOpenRegion(pRearrangedOpenRegion->Clone());
		pCulledOpenRegion->ClipOutside(pSpannedRegion);

		if( !pCulledOpenRegion->IsValid() )
			continue;

		std::vector<CBrushRegion::RegionPtr> unconnectedRegions;
		pCulledOpenRegion->GetUnconnectedRegions(unconnectedRegions);

		for( int i = 0, iUnconnectedRegionSize(unconnectedRegions.size()); i < iUnconnectedRegionSize; ++i )
		{
			unconnectedRegions[i]->Rearrange();

			const BrushVec3& vFirstVertex = unconnectedRegions[i]->GetVertex(0);
			const BrushVec3& vLastVertex = unconnectedRegions[i]->GetVertex(unconnectedRegions[i]->GetVertexListSize()-1);

			DESIGNER_ASSERT( pSpannedRegion->IsVertexOnCrust(vFirstVertex) );
			DESIGNER_ASSERT( pSpannedRegion->IsVertexOnCrust(vLastVertex) );

			int nNewVtxIdx[2] = { -1, -1 };
			int nNewEdgeIdx[2] = { -1, -1 };

			std::set<int> newEdgeIndexCandidates[2];
			pSpannedRegion->AddVertex(vFirstVertex, &nNewVtxIdx[0], &newEdgeIndexCandidates[0]);
			pSpannedRegion->AddVertex(vLastVertex, &nNewVtxIdx[1], &newEdgeIndexCandidates[1]);

			for( int k = 0; k < 2; ++k )
			{
				if( newEdgeIndexCandidates[k].size() > 1 )
				{
					BUtil::EdgeIndexSet secondIndices;
					std::set<int>::iterator iter = newEdgeIndexCandidates[k].begin();
					for( ; iter != newEdgeIndexCandidates[k].end(); ++iter )
						secondIndices.insert(pSpannedRegion->GetEdgeIndexPair(*iter).m_i[1]);
					nNewEdgeIdx[k] = pSpannedRegion->ChooseNextEdge( BUtil::SEdge(nNewVtxIdx[1-k],nNewVtxIdx[k]), secondIndices );
				}
				else if( newEdgeIndexCandidates[k].size() == 1 )
				{
					nNewEdgeIdx[k] = *newEdgeIndexCandidates[k].begin();
				}
				else
				{
					DESIGNER_ASSERT(0);
					return false;
				}
			}

			int iVertexSize = unconnectedRegions[i]->GetVertexListSize();

			std::vector<BrushVec3> vList;
			CBrushRegion::EResultExtract result = pSpannedRegion->ExtractVertexList( nNewEdgeIdx[0], nNewEdgeIdx[1], vList );
			if( result == CBrushRegion::eRE_EndAtEndVtx )
			{
				for( int k = iVertexSize-2; k >= 1; --k )
					vList.push_back(unconnectedRegions[i]->GetVertex(k));

				CBrushRegion::RegionPtr pNewRegion = new CBrushRegion(vList,pSpannedRegion->GetPlane(),pSpannedRegion->GetMaterialID(),&pSpannedRegion->GetTexInfo(),true);
				pNewRegion->SetFlag(pSpannedRegion->GetFlag());

				if( pNewRegion->IsValid() )
				{
					if( hHasHoles )
						pNewRegion->Intersect(pSpannedRegionWithoutBridgeEdges);
					if( pNewRegion->IsValid() && !pNewRegion->IsOpen() )
						AddRegion( pNewRegion, eOpType_Add);
				}

				vList.clear();
				result = pSpannedRegion->ExtractVertexList( nNewEdgeIdx[1], nNewEdgeIdx[0], vList );
				DESIGNER_ASSERT( result == CBrushRegion::eRE_EndAtEndVtx );
				for( int k = 1; k < iVertexSize-1; ++k )
					vList.push_back(unconnectedRegions[i]->GetVertex(k));

				pNewRegion = new CBrushRegion(vList,pSpannedRegion->GetPlane(),pSpannedRegion->GetMaterialID(),&(pSpannedRegion->GetTexInfo()),true);
				pNewRegion->SetFlag(pSpannedRegion->GetFlag());

				if( pNewRegion->IsValid() )
				{
					if( hHasHoles )
						pNewRegion->Intersect(pSpannedRegionWithoutBridgeEdges);
					if( pNewRegion->IsValid() && !pNewRegion->IsOpen() )
						AddRegion( pNewRegion, eOpType_Add);
				}
			}
			else if( result == CBrushRegion::eRE_EndAtStartVtx )
			{
				for( int k = 0; k < iVertexSize-1; ++k )
				{
					const BrushVec3& vertex = unconnectedRegions[i]->GetVertex(k);
					const BrushVec3& nextVertex = unconnectedRegions[i]->GetVertex(k+1);
					pSpannedRegion->AddEdge(BrushEdge3D(vertex,nextVertex));
					pSpannedRegion->AddEdge(BrushEdge3D(nextVertex,vertex));
				}
				if( i == iUnconnectedRegionSize-1 )
					AddRegion(pSpannedRegion->Clone(),eOpType_Add);
			}
		}
	}

	InvalidateAABB();
	return true;
}

CBrushRegion::RegionPtr CBrushDesigner::QueryEquivalentRegion( CBrushRegion::RegionPtr pRegion, int* pOutRegionIndex ) const
{
	return QueryEquivalentRegion(m_ShelfID,pRegion,pOutRegionIndex);
}

CBrushRegion::RegionPtr CBrushDesigner::QueryEquivalentRegion( int nShelfID, CBrushRegion::RegionPtr pRegion, int* pOutRegionIndex ) const
{
	int iRegionSize(m_Regions[nShelfID].size());
	for( int i = 0; i < iRegionSize; ++i )
	{
		if( m_Regions[nShelfID][i] && m_Regions[nShelfID][i] == pRegion )
		{
			if( pOutRegionIndex )
				*pOutRegionIndex = i;
			return pRegion;
		}
	}

	for( int i = 0; i < iRegionSize; ++i )
	{
		if( m_Regions[nShelfID][i] && m_Regions[nShelfID][i]->IsEquivalent(pRegion) )
		{
			if( pOutRegionIndex )
				*pOutRegionIndex = i;
			return m_Regions[nShelfID][i];
		}
	}
	return NULL;

}

void CBrushDesigner::AddRegion( CBrushRegion::RegionPtr pRegion )
{
	AddRegion(m_ShelfID,pRegion);
}

void CBrushDesigner::AddRegion( int nShelf, CBrushRegion::RegionPtr pRegion )
{
	bool bEquivalentExists = QueryEquivalentRegion(nShelf,pRegion) ? true : false;
	DESIGNER_ASSERT(!bEquivalentExists);
	DESIGNER_ASSERT(pRegion->IsValid());
	if( !bEquivalentExists && pRegion->IsValid())
	{
		m_Regions[nShelf].push_back(pRegion);
		InvalidateAABB();
	}
}

CBrushDesigner::EClipRegionResult CBrushDesigner::Clip( const BrushPlane& clipPlane, _smart_ptr<CBrushDesigner>& pOutFrontPart, _smart_ptr<CBrushDesigner>& pOutBackPart, bool bFillFacet ) const
{
	EClipRegionResult clipResult = eCRR_SUCCESSED;

	_smart_ptr<CBrushDesigner> pFrontPart = new CBrushDesigner;
	_smart_ptr<CBrushDesigner> pBackPart = new CBrushDesigner;

	pFrontPart->m_nModeFlag = pBackPart->m_nModeFlag = m_nModeFlag;
	std::vector<BrushEdge3D> facetEdges;

	for( int i = 0, iRegionSize(m_Regions[m_ShelfID].size()); i < iRegionSize; ++i )
	{
		CBrushRegion::RegionPtr pRegionWithoutBridges;
		if( m_Regions[m_ShelfID][i]->HasBridgeEdges() )
		{
			pRegionWithoutBridges = m_Regions[m_ShelfID][i]->Clone();
			pRegionWithoutBridges->RemoveBridgeEdges();
		}
		else
		{
			pRegionWithoutBridges = m_Regions[m_ShelfID][i];
		}

		std::vector<CBrushRegion::RegionPtr> pFrontRegions;
		std::vector<CBrushRegion::RegionPtr> pBackRegions;
		if( !m_Regions[m_ShelfID][i]->ClipByPlane(clipPlane, pFrontRegions, pBackRegions, &facetEdges) )
			continue;

		for( int k = 0; k < pFrontRegions.size(); ++k )
			pFrontPart->AddRegion(pFrontRegions[k]->Clone(),eOpType_Add);

		for( int k = 0; k < pBackRegions.size(); ++k )
			pBackPart->AddRegion(pBackRegions[k]->Clone(),eOpType_Add);
	}

	if( bFillFacet )
	{
		std::vector<CBrushRegion::RegionPtr> facetRegions;
		if( GenerateRegionsFromEdgeList( facetEdges, clipPlane, facetRegions ) )
		{
			for( int i = 0, iRegionSize(facetRegions.size()); i < iRegionSize; ++i )
			{
				CBrushRegion::RegionPtr pFacetRegion = facetRegions[i];

				DESIGNER_ASSERT(!pFacetRegion->IsOpen());
				if( pFacetRegion->IsOpen() )
					continue;

				pBackPart->AddRegion(pFacetRegion->Clone(),eOpType_Add);
				pFrontPart->AddRegion(pFacetRegion->Clone()->Flip(),eOpType_Add);
			}
		}
		else
		{
			clipResult = eCRR_CLIPSUCCESSEDBUTFILLFAILED;
		}
	}

	pOutFrontPart = NULL;
	pOutBackPart = NULL;

	if( pFrontPart->GetRegionSize() > 0 )
		pOutFrontPart = pFrontPart;

	if( pBackPart->GetRegionSize() > 0 )
		pOutBackPart = pBackPart;

	return pOutFrontPart || pOutBackPart ? clipResult : eCRR_CLIPFAILED;
}

bool CBrushDesigner::GenerateRegionsFromEdgeList( std::vector<BrushEdge3D>& edgeList, const BrushPlane& plane, std::vector<CBrushRegion::RegionPtr>& outRegions )
{
	if( edgeList.empty() )
		return false;

	std::set<int> usedIndices;
	int nEdgeCount(edgeList.size());
	BrushEdge3D currentEdge(edgeList[0]);

	std::vector<CBrushRegion::RegionPtr> facetRegions;
	CBrushRegion::RegionPtr pFacetRegion = new CBrushRegion;
	pFacetRegion->AddEdge(currentEdge);
	usedIndices.insert(0);
	facetRegions.push_back(pFacetRegion);

	while( usedIndices.size() < nEdgeCount )
	{
		bool bFoundNextEdge = false;
		int k = 0;

		for( ; k < nEdgeCount; ++k )
		{
			if( usedIndices.find(k) != usedIndices.end() )
				continue;
			if( currentEdge.m_v[1].IsEquivalent(edgeList[k].m_v[0],kDesignerEpsilon) )
			{
				currentEdge = edgeList[k];
				bFoundNextEdge = true;
				break;
			}
			else if( currentEdge.m_v[1].IsEquivalent(edgeList[k].m_v[1],kDesignerEpsilon) )
			{
				currentEdge = edgeList[k].GetInverted();
				bFoundNextEdge = true;
				break;
			}
		}

		if( bFoundNextEdge )
		{
			pFacetRegion->AddEdge(currentEdge);
			usedIndices.insert(k);
		}
		else if( usedIndices.size() < nEdgeCount )
		{
			DESIGNER_ASSERT( !pFacetRegion->IsOpen() );
			if( pFacetRegion->IsOpen() )
				return false;

			pFacetRegion = new CBrushRegion;
			facetRegions.push_back(pFacetRegion);
			for( int i = 0; i < nEdgeCount; ++i )
			{
				if( usedIndices.find(i) == usedIndices.end() )
				{
					currentEdge = edgeList[i];
					pFacetRegion->AddEdge(currentEdge);
					usedIndices.insert(i);
					break;
				}
			}
		}
		else
		{
			DESIGNER_ASSERT(0);
			break;
		}
	}

	int ifacetRegionsize(facetRegions.size());

	for( int i = 0; i < ifacetRegionsize; ++i )
	{
		pFacetRegion = facetRegions[i];

		std::vector<BrushVec3> polygon;
		pFacetRegion->GetLinkedVertices(polygon);

		BrushFloat fSmallestY = 3e10f;
		int nSmallestIndex = 0;
		int iPtSize(polygon.size());
		for( int k = 0; k < iPtSize; ++k )
		{
			BrushVec2 pos2D = plane.W2P(polygon[k]);
			if( fSmallestY > pos2D.y )
			{
				nSmallestIndex = k;
				fSmallestY = pos2D.y;
			}
		}

		pFacetRegion->SetPlane(BrushPlane(
			polygon[((nSmallestIndex-1)+iPtSize)%iPtSize],
			polygon[nSmallestIndex],
			polygon[(nSmallestIndex+1)%iPtSize],kDesignerEpsilon));

		if( !pFacetRegion->GetPlane().IsSameFacing(plane) )
			pFacetRegion->Flip();

		pFacetRegion->SetPlane(plane);
	}

	usedIndices.clear();
	for( int i = 0; i < ifacetRegionsize; ++i )
	{
		if( usedIndices.find(i) != usedIndices.end() )
			continue;
		bool bInclude = false;
		for( int k = 0; k < ifacetRegionsize; ++k )
		{
			if( i == k )
				continue;
			if( usedIndices.find(k) != usedIndices.end() )
				continue;
			if( facetRegions[i]->IncludeAllEdges(facetRegions[k]) )
			{
				usedIndices.insert(k);
				facetRegions[i]->Subtract(facetRegions[k]);
				facetRegions[k] = NULL;
				bInclude = true;
			}
		}
		if( bInclude )
		{
			usedIndices.insert(i);
		}
	}

	for( int i = 0; i < ifacetRegionsize; ++i )
	{
		if( facetRegions[i] == NULL )
			continue;
		outRegions.push_back(facetRegions[i]);
	}

	return true;
}

void CBrushDesigner::ResetFromList( const std::vector<CBrushRegion::RegionPtr>& regionList )
{
	Clear();

	for( int i = 0, iRegionCount(regionList.size()); i < iRegionCount; ++i )
		AddRegion(regionList[i],CBrushDesigner::eOpType_Union);
}

AABB CBrushDesigner::GetBoundBox( int nShelf )
{	
	DESIGNER_SHELF_RECONSTRUCTOR(this);

	for( int i = 0; i < BUtil::kMaxShelfCount; ++i )
	{
		if( i == nShelf || nShelf == -1 )
		{
			if( m_BoundBox[i].bValid )
			{
				if( i == nShelf )
					return m_BoundBox[i].aabb;
				continue;
			}

			m_BoundBox[i].bValid = true;
			m_BoundBox[i].aabb.Reset();

			SetShelf(i);
			for( int k = 0, iRegionCount(GetRegionSize()); k < iRegionCount; ++k )
				m_BoundBox[i].aabb.Add(GetRegion(k)->GetBoundBox());

			if( i == nShelf )
				return m_BoundBox[i].aabb;
		}
	}

	AABB aabb;
	aabb.Reset();
	if( nShelf == -1 )
	{
		aabb.Add(m_BoundBox[0].aabb);
		aabb.Add(m_BoundBox[1].aabb);
		return aabb;
	}

	DESIGNER_ASSERT(0 && "Invalid calling CBrushDesigner::GetBoundBox()");
	return aabb;
}

bool CBrushDesigner::IsEmpty( int nShelf ) const
{
	if( nShelf == -1 )
		return m_Regions[0].empty() && m_Regions[1].empty();

	DESIGNER_ASSERT( nShelf == 0 || nShelf == 1 );
	if( nShelf != 0 && nShelf != 1 )
		return false;

	return m_Regions[nShelf].empty();
}

bool CBrushDesigner::HasClosedRegion( int nShelf ) const
{
	for( int i = 0; i < 2; ++i )
	{
		if( i != nShelf && nShelf != -1 )
			continue;
		for( int k = 0, iRegionCount(m_Regions[i].size()); k < iRegionCount; ++k )
		{
			if( !m_Regions[i][k]->IsOpen() )
				return true;
		}
	}
	return false;
}

void CBrushDesigner::ResetDB( int nFlag, int nValidShelfID )
{
	m_pDB->Reset(this,nFlag,nValidShelfID);
}

void CBrushDesigner::SetSubdivisionResult( CBrushDesignerHalfEdgeMesh* pSubdividedHalfMesh )
{
	if( m_pSubdividionResult )
	{
		m_pSubdividionResult->Release();
		m_pSubdividionResult = NULL;
	}

	m_pSubdividionResult = pSubdividedHalfMesh;
	if( m_pSubdividionResult )
		m_pSubdividionResult->AddRef();
}