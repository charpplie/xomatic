#include "StdAfx.h"
#include "BrushDesignerExtrudeSnappingHelper.h"
#include "ViewManager.h"

namespace BrushDesigner
{
	CBrushDesignerExtrudeSnappingHelper s_SnappingHelper;
}

void CBrushDesignerExtrudeSnappingHelper::SearchForOppositeRegions( CBrushRegion::RegionPtr pCapRegion )
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(0);

	for( int i = 0; i < 2; ++i )
	{
		// i == 0 --> Query push direction region
		// i == 1 --> Query pull direction region
		m_Opposites[i].Init();

		SOpposite opposite;
		SOpposite oppositeAdjancent;

		CBrushDesigner::EFindOppositeFlag nFlag = i == 0 ? CBrushDesigner::eFOF_PushDirection : CBrushDesigner::eFOF_PullDirection;
		BrushFloat fScale = kDesignerEpsilon*10.0f;

		opposite.bTouchAdjacent = false;
		opposite.regionRelation = GetDesigner()->QueryOppositeRegion(
			pCapRegion,
			nFlag,
			0,
			opposite.region,
			opposite.nearestDistanceToRegion);

		oppositeAdjancent.bTouchAdjacent = true;
		oppositeAdjancent.regionRelation = GetDesigner()->QueryOppositeRegion(
			pCapRegion,
			nFlag,
			fScale,
			oppositeAdjancent.region,
			oppositeAdjancent.nearestDistanceToRegion);

		if( opposite.regionRelation != CBrushDesigner::eER_None && oppositeAdjancent.regionRelation != CBrushDesigner::eER_None )
		{
			if( opposite.nearestDistanceToRegion-oppositeAdjancent.nearestDistanceToRegion <= kDesignerEpsilon )
				m_Opposites[i] = opposite;
			else
				m_Opposites[i] = oppositeAdjancent;
		}
		else if( opposite.regionRelation != CBrushDesigner::eER_None && oppositeAdjancent.regionRelation == CBrushDesigner::eER_None )
			m_Opposites[i] = opposite;
		else if( opposite.regionRelation == CBrushDesigner::eER_None && oppositeAdjancent.regionRelation != CBrushDesigner::eER_None )
			m_Opposites[i] = oppositeAdjancent;
	}
}

bool CBrushDesignerExtrudeSnappingHelper::IsOverOppositeRegion( const CBrushRegion::RegionPtr& pRegion, BUtil::EPushPull pushpull )
{
	if( m_Opposites[pushpull].region )
	{
		BrushVec3 direction = -pRegion->GetPlane().Normal();
		BrushFloat fNearestDistance = pRegion->GetNearestDistance(m_Opposites[pushpull].region,direction);

		if( pushpull == BUtil::ePP_Push )
			return fNearestDistance > -kDesignerEpsilon;
		else
			return fNearestDistance < kDesignerEpsilon;
	}

	return false;
}

void CBrushDesignerExtrudeSnappingHelper::ApplyOppositeRegions( CBrushRegion::RegionPtr pCapRegion, BUtil::EPushPull pushpull, bool bHandleTouch )
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	if( m_Opposites[pushpull].region && m_Opposites[pushpull].regionRelation == CBrushDesigner::eER_Intersection )
	{
		if( m_Opposites[pushpull].bTouchAdjacent && bHandleTouch )
		{
			GetDesigner()->SetShelf(0);
			GetDesigner()->AddRegion(pCapRegion,CBrushDesigner::eOpType_Union);
			GetDesigner()->SetShelf(1);
			GetDesigner()->RemoveRegion(pCapRegion);
		}
		else if( !m_Opposites[pushpull].bTouchAdjacent )
		{
			pCapRegion->Flip();
			GetDesigner()->SetShelf(0);

			std::vector<CBrushRegion::RegionPtr> intersetionRegions;
			GetDesigner()->QueryIntersectionByRegion(pCapRegion,intersetionRegions);

			CBrushRegion::RegionPtr ABInterectRegion = pCapRegion->Clone();
			CBrushRegion::RegionPtr ABSubtractRegion = pCapRegion->Clone();
			CBrushRegion::RegionPtr pUnionRegion = new CBrushRegion;

			for( int i = 0, iIntersectionSize(intersetionRegions.size()); i < iIntersectionSize; ++i )
				pUnionRegion->Union(intersetionRegions[i]);

			ABInterectRegion->Intersect(pUnionRegion,CBrushRegion::eICEII_IncludeCoSame);
			ABSubtractRegion->Subtract(pUnionRegion);

			if( ABInterectRegion->IsValid() && !ABInterectRegion->IsOpen() ) 
				GetDesigner()->AddRegion(ABInterectRegion,CBrushDesigner::eOpType_SubtractAB);

			if( ABSubtractRegion->IsValid() && !ABSubtractRegion->IsOpen() )
				GetDesigner()->AddRegion(ABSubtractRegion->Flip(),CBrushDesigner::eOpType_Add);

			GetDesigner()->SetShelf(1);
			pCapRegion->Flip();
			std::vector<CBrushRegion::RegionPtr> regionList;
			GetDesigner()->QueryRegions(pCapRegion->GetPlane(),regionList);
			for( int i = 0, iRegionCount(regionList.size()); i < iRegionCount; ++i )
				GetDesigner()->DrillRegion(regionList[i]);
		}
	}

	m_Opposites[0].region = NULL;
	m_Opposites[1].region = NULL;
}

BrushFloat CBrushDesignerExtrudeSnappingHelper::GetNearestDistanceToOpposite( BUtil::EPushPull pushpull ) const
{
	BrushFloat nearestDistance = m_Opposites[pushpull].nearestDistanceToRegion;
	if( pushpull == BUtil::ePP_Push )
		nearestDistance = -nearestDistance;
	return nearestDistance;
}

CBrushRegion::RegionPtr CBrushDesignerExtrudeSnappingHelper::FindAlignedRegion( CBrushRegion::RegionPtr pCapRegion, const BrushMatrix34& worldTM, CViewport *view, const CPoint& point ) const
{
	if( pCapRegion == NULL )
		return NULL;
	BrushVec3 localRaySrc;
	BrushVec3 localRayDir;
	BUtil::GetLocalViewRay( worldTM, view, point, localRaySrc, localRayDir );
	int nIndex(-1);
	if( !GetDesigner()->QueryRegion( localRaySrc, localRayDir, nIndex ) )
		return NULL;

	const CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(nIndex);
	if( pRegion->GetPlane().Normal().IsEquivalent( pCapRegion->GetPlane().Normal() ) )
		return pRegion;

	return NULL;
}