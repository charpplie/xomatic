////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushBspTree3D.h
//  Created:     Oct/25/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BrushBSPTree3D.h"
#include "BrushRegion.h"
#include "BrushDesignerPolygonDecomposer.h"

class CBrushBSPTree3DNode
{
public:
	CBrushBSPTree3DNode() : m_PosChild(NULL), m_NegChild(NULL)
	{
	}
	~CBrushBSPTree3DNode()
	{
		if( m_PosChild )
			delete m_PosChild;
		if( m_NegChild )
			delete m_NegChild;
	}

	void AddCoplnarRegion( CBrushRegion::RegionPtr pRegion )
	{
		bool bHaveIntersection = false;
		std::set<CBrushRegion::RegionPtr> removedRegions;

		for( int i = 0, iRegionCount(m_RegionsOnCoplanar.size()); i < iRegionCount; ++i )
		{
			if( BUtil::eIT_None != CBrushRegion::HasIntersection( m_RegionsOnCoplanar[i], pRegion ) )
			{
				bHaveIntersection = true;
				m_RegionsOnCoplanar[i]->Union(pRegion);
				removedRegions.insert(pRegion);
				pRegion = m_RegionsOnCoplanar[i];
			}
		}

		if( !bHaveIntersection )
		{
			m_RegionsOnCoplanar.push_back(pRegion->Clone());
		}
		else
		{
			std::vector<CBrushRegion::RegionPtr>::iterator ii = m_RegionsOnCoplanar.begin();
			for( ; ii != m_RegionsOnCoplanar.end(); )
			{
				if( removedRegions.find(*ii) != removedRegions.end() )
					ii = m_RegionsOnCoplanar.erase(ii);
				else
					++ii;
			}
		}
	}

	void SetPositiveChild( CBrushBSPTree3DNode* pTree ){	m_PosChild = pTree;	}
	void SetNegativeChild( CBrushBSPTree3DNode* pTree ){	m_NegChild = pTree;	}
	CBrushBSPTree3DNode* GetPositiveChild() const{	return m_PosChild;	}
	CBrushBSPTree3DNode* GetNegativeChild() const{	return m_NegChild;	}

	void GetPartitions( CBrushRegion::RegionPtr& pRegion, CBrushBSPTree3D::SOutputRegions& outRegions ) const
	{
		DESIGNER_ASSERT( !m_RegionsOnCoplanar.empty() );

		if( m_RegionsOnCoplanar.empty() )
			return;

		const BrushPlane& plane = m_RegionsOnCoplanar[0]->GetPlane();

		bool bOnCoSamePlane = plane.IsEquivalent(pRegion->GetPlane(),kDesignerEpsilon);
		bool bOnCoDiffPlane = plane.IsEquivalent(pRegion->GetPlane().GetInverted(),kDesignerEpsilon);

		if( bOnCoSamePlane || bOnCoDiffPlane )
		{
			CBrushRegion::RegionPtr pIntersected = pRegion->Clone();
			for( int i = 0, iRegionSize(m_RegionsOnCoplanar.size()); i < iRegionSize; ++i )
			{
				if( bOnCoSamePlane )
					pIntersected->Intersect(m_RegionsOnCoplanar[i]);
				else
					pIntersected->Intersect(m_RegionsOnCoplanar[i]->Clone()->Flip());
			}

			if( pIntersected->IsValid() && !pIntersected->IsOpen() )
			{
				if( bOnCoSamePlane )
					outRegions.coSameList.push_back(pIntersected);
				else
					outRegions.coDiffList.push_back(pIntersected);
			}

			CBrushRegion::RegionPtr pSubtracted = pRegion->Clone();
			pSubtracted->Subtract(pIntersected);
			if( pSubtracted->IsValid() && !pSubtracted->IsOpen() )
				GetPositivePartitions(pSubtracted, outRegions);
		}
		else
		{
			std::vector<CBrushRegion::RegionPtr> pFrontRegions;
			std::vector<CBrushRegion::RegionPtr> pBackRegions;
			if( !pRegion->ClipByPlane( plane, pFrontRegions, pBackRegions ) )
				return;

			if( !pFrontRegions.empty() )
			{
				for( int i = 0, iRegionCount(pFrontRegions.size()); i < iRegionCount; ++i )
					GetPositivePartitions( pFrontRegions[i], outRegions );
			}

			if( !pBackRegions.empty() )
			{
				for( int i = 0, iRegionCount(pBackRegions.size()); i < iRegionCount; ++i )
					GetNegativePartitions( pBackRegions[i], outRegions );
			}
		}
	}

	bool IsInside( const BrushVec3& vPos ) const
	{
		DESIGNER_ASSERT( !m_RegionsOnCoplanar.empty() );
		if( m_RegionsOnCoplanar.empty() )
			return false;

		BrushFloat distance(m_RegionsOnCoplanar[0]->GetPlane().Distance(vPos));

		if( distance > kDesignerEpsilon )
		{
			if( m_PosChild )
				return m_PosChild->IsInside(vPos);
			else
				return false;
		}
		else if( distance < -kDesignerEpsilon )
		{
			if( m_NegChild )
				return m_NegChild->IsInside(vPos);
			else
				return true;
		}
		else if( m_PosChild || m_NegChild )
		{
			for( int i = 0, iCoRegionCount(m_RegionsOnCoplanar.size()); i < iCoRegionCount; ++i )
			{
				if( m_RegionsOnCoplanar[i]->Include(vPos) )
					return true;
			}
		}
		return false;
	}

private:

	void GetPositivePartitions( CBrushRegion::RegionPtr& pRegion, CBrushBSPTree3D::SOutputRegions& outRegions ) const
	{
		if( m_PosChild )
			m_PosChild->GetPartitions(pRegion, outRegions);
		else
			outRegions.posList.push_back(pRegion);
	}

	void GetNegativePartitions( CBrushRegion::RegionPtr& pRegion, CBrushBSPTree3D::SOutputRegions& outRegions ) const
	{
		if( m_NegChild )
			m_NegChild->GetPartitions(pRegion, outRegions);
		else
			outRegions.negList.push_back(pRegion);
	}

private:
	std::vector<CBrushRegion::RegionPtr> m_RegionsOnCoplanar;
	CBrushBSPTree3DNode* m_PosChild;
	CBrushBSPTree3DNode* m_NegChild;
};

CBrushBSPTree3D::CBrushBSPTree3D( std::vector<CBrushRegion::RegionPtr>& regionList )
{
	m_bValidTree = true;
	m_pRootNode = BuildBSP(regionList);
}

CBrushBSPTree3D::~CBrushBSPTree3D()
{
	if( m_pRootNode )
		delete m_pRootNode;
}

void CBrushBSPTree3D::GetPartitions( CBrushRegion::RegionPtr& pRegion, SOutputRegions& outRegions ) const
{
	m_pRootNode->GetPartitions( pRegion, outRegions );
}

bool CBrushBSPTree3D::IsInside( const BrushVec3& vPos ) const
{
	return m_pRootNode->IsInside(vPos);
}

CBrushBSPTree3DNode* CBrushBSPTree3D::BuildBSP( std::vector<CBrushRegion::RegionPtr>& regionList )
{
	CBrushBSPTree3DNode* pNode = new CBrushBSPTree3DNode;

	CBrushRegion::RegionPtr pNodeRegion = regionList[0];
	pNode->AddCoplnarRegion(pNodeRegion);

	std::vector<CBrushRegion::RegionPtr> posList, negList;

	for( int i = 1, iRegionSize(regionList.size()); i < iRegionSize; ++i )
	{
		if( pNodeRegion->GetPlane().IsEquivalent(regionList[i]->GetPlane(),kDesignerEpsilon) )
		{
			pNode->AddCoplnarRegion(regionList[i]);
			continue;
		}

		std::vector<CBrushRegion::RegionPtr> pFrontRegions;
		std::vector<CBrushRegion::RegionPtr> pBackRegions;
		if( regionList[i]->ClipByPlane( pNodeRegion->GetPlane(), pFrontRegions, pBackRegions, NULL ) )
		{
			for( int k = 0; k < pFrontRegions.size(); ++k )
				posList.push_back(pFrontRegions[k]);
			for( int k = 0; k < pBackRegions.size(); ++k )
				negList.push_back(pBackRegions[k]);
		}
		else
		{
			m_bValidTree = false;
		}

		DESIGNER_ASSERT(!pFrontRegions.empty() || !pBackRegions.empty());
		if( pFrontRegions.empty() && pBackRegions.empty() )
			m_bValidTree = false;
	}

	if( !posList.empty() )
		pNode->SetPositiveChild(BuildBSP(posList));

	if( !negList.empty() )
		pNode->SetNegativeChild(BuildBSP(negList));

	return pNode;
}