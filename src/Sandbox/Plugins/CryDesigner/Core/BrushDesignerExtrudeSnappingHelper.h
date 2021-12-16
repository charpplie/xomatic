#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerExtrudeSnappingHelper.h
//  Created:     20/Feb/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushRegion.h"
#include "BrushDesigner.h"

class CViewport;

class CBrushDesignerExtrudeSnappingHelper
{
public:

	void Init( CBrushDesigner* pDesigner )
	{
		m_pDesigner = pDesigner;
	}

	void SearchForOppositeRegions( CBrushRegion::RegionPtr pCapRegion );
	bool IsOverOppositeRegion( const CBrushRegion::RegionPtr& pCapRegion, BUtil::EPushPull pushpull );
	void ApplyOppositeRegions( CBrushRegion::RegionPtr pCapRegion, BUtil::EPushPull pushpull, bool bHandleTouch = false );
	BrushFloat GetNearestDistanceToOpposite( BUtil::EPushPull pushpull ) const;
	CBrushRegion::RegionPtr FindAlignedRegion( CBrushRegion::RegionPtr pCapRegion, const BrushMatrix34& worldTM, CViewport *view, const CPoint& point ) const;

	struct SOpposite
	{
		SOpposite()
		{
			Init();
		}
		void Init()
		{
			region = NULL;
			regionRelation = CBrushDesigner::eER_None;
			nearestDistanceToRegion = 0;
			bTouchAdjacent = false;
		}
		CBrushRegion::RegionPtr region;
		CBrushDesigner::ERegionRelation regionRelation;
		BrushFloat nearestDistanceToRegion;
		bool bTouchAdjacent;
	};

private:

	CBrushDesigner* GetDesigner() const { return m_pDesigner; }

	_smart_ptr<CBrushDesigner> m_pDesigner;
	SOpposite m_Opposites[2];
};

namespace BrushDesigner
{
	extern CBrushDesignerExtrudeSnappingHelper s_SnappingHelper;
}
using BrushDesigner::s_SnappingHelper;
