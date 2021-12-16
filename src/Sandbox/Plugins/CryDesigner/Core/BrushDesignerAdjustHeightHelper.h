#pragma once

#include "BrushPlane.h"

class CBrushDesignerAdjustHeightHelper
{
public:

	CBrushDesignerAdjustHeightHelper() : m_bDisplayable(false) {}
	void Init( const BrushPlane& floorPlane, const BrushVec3& vPivot )
	{
		m_FloorPlane = floorPlane;
		m_vPivot = vPivot;
		m_bDisplayable = false;
	}
	BrushFloat UpdateHeight( const BrushMatrix34& worldTM, CViewport *view, const CPoint& point );
	void Display( DisplayContext &dc );

private:

	BrushVec3 m_vPivot;
	BrushPlane m_FloorPlane;
	BrushPlane m_HelperPlane;
	bool m_bDisplayable;
};

namespace BrushDesigner
{
	extern CBrushDesignerAdjustHeightHelper s_AdjustHeightHelper;
}
using BrushDesigner::s_AdjustHeightHelper;
