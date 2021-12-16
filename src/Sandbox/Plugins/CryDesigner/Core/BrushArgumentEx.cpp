#include "Stdafx.h"
#include "BrushArgumentEx.h"
#include "BrushDesigner.h"
#include "BrushRegion.h"
#include "BrushDesignerDB.h"

CBrushArgumentEx::CBrushArgumentEx(
	CBrushRegion* pRegion,
	const BrushFloat& fScale,
	CBaseObject* pObject,
	std::vector<CBrushRegion::RegionPtr>* perpendicularRegions,
	CBrushDesignerDB* pDB ) :
	CBrushArgument(pRegion,pObject,perpendicularRegions,pDB),
	m_fScale(fScale)
{
}

void CBrushArgumentEx::Update(EBrushArgumentUpdate updateOp)
{
	if( std::abs(m_fScale) < kDesignerEpsilon )
	{
		CBrushArgument::Update(updateOp);
		return;
	}

	m_pDesigner->Clear();

	m_CapPlane = BrushPlane(m_BasePlane.Normal(),m_BasePlane.Distance()-m_fHeight);
	AddCapRegions();

	for( int i = 0, iEdgeSize(m_pInitialRegion->GetEdgeSize()); i < iEdgeSize; ++i )
	{
		BrushEdge3D initEdge = m_pInitialRegion->GetEdge(i);
		BrushEdge3D capEdge = m_pRegion->GetEdge(i);

		std::vector<BrushVec3> v;
		v.resize(4);

		v[0] = capEdge.m_v[1];
		v[1] = capEdge.m_v[0];
		v[2] = initEdge.m_v[0];
		v[3] = initEdge.m_v[1];

		if( m_fHeight < 0.0f )
		{
			std::swap(v[0],v[3]);
			std::swap(v[1],v[2]);
		}

		BrushPlane sidePlane( v[0], v[1], v[2], kDesignerEpsilon );

		if( m_pDB )
			m_pDB->FindPlane(sidePlane,sidePlane);

		CBrushRegion::RegionPtr pSideRegion = new CBrushRegion(v, sidePlane, m_pInitialRegion->GetMaterialID(), &m_pInitialRegion->GetTexInfo(), true);
		pSideRegion->SetFlag(m_pInitialRegion->GetFlag());
		if( m_pDB )
			m_pDB->UpdateRegionVertices(pSideRegion);

		m_pDesigner->AddRegion( pSideRegion->Clone(), CBrushDesigner::eOpType_Add );
	}

	if( updateOp == eBAU_UpdateBrush )
		UpdateBrush();
}

void CBrushArgumentEx::AddCapRegions()
{
	UpdateRegionVertices2Plane(m_CapPlane);
	m_pRegion->Scale(m_fScale);
	m_pDesigner->AddRegion( m_pRegion->Clone(), CBrushDesigner::eOpType_Add );
}