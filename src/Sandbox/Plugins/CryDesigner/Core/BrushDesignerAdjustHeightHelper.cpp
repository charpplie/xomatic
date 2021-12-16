#include "StdAfx.h"
#include "BrushDesignerAdjustHeightHelper.h"
#include "BrushDesignerExtrudeSnappingHelper.h"
#include "Viewport.h"

namespace BrushDesigner
{
	CBrushDesignerAdjustHeightHelper s_AdjustHeightHelper;
}

BrushFloat CBrushDesignerAdjustHeightHelper::UpdateHeight( const BrushMatrix34& worldTM, CViewport *view, const CPoint& point )
{
	const CCamera& camera = GetIEditor()->GetRenderer()->GetCamera();
	BrushMatrix34 invWorldTM = worldTM.GetInverted();
	BrushVec3 vThirdDirs[] = { invWorldTM.TransformVector(camera.GetMatrix().GetColumn0()),
		invWorldTM.TransformVector(camera.GetMatrix().GetColumn2()) };
	int nThirdIdx = 0;

	BrushFloat fDotNearestZero = (BrushFloat)1.0f;
	for( int i = 0,iCount(sizeof(vThirdDirs)/sizeof(*vThirdDirs)); i < iCount; ++i )
	{
		BrushFloat fDot = m_FloorPlane.Normal().Dot(vThirdDirs[i]);
		if( std::abs(fDot) < fDotNearestZero )
		{
			fDotNearestZero = fDot;
			nThirdIdx = i;
		}
	}

	BrushVec3 v0 = m_vPivot;
	BrushVec3 v1 = m_vPivot+m_FloorPlane.Normal();
	BrushVec3 v2 = m_vPivot+vThirdDirs[nThirdIdx];
	m_HelperPlane = BrushPlane(v0,v1,v2,kDesignerEpsilon);

	BrushVec3 vLocalSrc;
	BrushVec3 vLocalDir;
	BUtil::GetLocalViewRay( worldTM, view, point, vLocalSrc, vLocalDir );
	BrushVec3 vHit;
	BrushFloat t;
	if( !m_HelperPlane.HitTest( vLocalSrc, vLocalSrc+vLocalDir, kDesignerEpsilon, &t, &vHit ) || t < 0 )
		return 0;

	m_bDisplayable = true;

	return BUtil::SnapGrid(m_FloorPlane.Distance(vHit));
}

void CBrushDesignerAdjustHeightHelper::Display( DisplayContext &dc )
{
#ifdef ENABLE_DRAWDEBUGHELPER
	if( m_bDisplayable )
		BUtil::DrawPlane(dc, m_vPivot, m_HelperPlane,1000.0f);
#endif
}