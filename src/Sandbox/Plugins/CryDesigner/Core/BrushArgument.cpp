#include "Stdafx.h"
#include "BrushArgument.h"
#include "BrushRegion.h"
#include "BrushDesignerDB.h"

CBrushArgument::CBrushArgument( 
	CBrushRegion* pRegion,
	CBaseObject* pObject,
	std::vector<CBrushRegion::RegionPtr>* perpendicularRegions,
	CBrushDesignerDB* pDB ) : 
	m_fHeight(0)
{
	DESIGNER_ASSERT(pRegion);

	if( pRegion )
	{
		m_pRegion = pRegion->Clone();
		m_pInitialRegion = pRegion->Clone();
		m_BasePlane = pRegion->GetPlane();

		if( m_pInitialRegion->HasBridgeEdges() )
		{
			m_pInitialOutsideRegionWithoutBridgeEdges = m_pInitialRegion->Clone();
			m_pInitialOutsideRegionWithoutBridgeEdges->RemoveBridgeEdges();
		}
		else
		{
			m_pInitialOutsideRegionWithoutBridgeEdges = m_pInitialRegion;
		}

		std::vector<CBrushRegion::RegionPtr> oustsideRegions;
		m_pInitialOutsideRegionWithoutBridgeEdges->GetSeparatedRegions(oustsideRegions,CBrushRegion::eSR_OuterHull);
		m_pInitialOutsideRegionWithoutBridgeEdges->GetSeparatedRegions(m_InitialInsideRegionsWithoutBrideEdges,CBrushRegion::eSR_InnerHull);
		if( oustsideRegions.size() == 1 && !m_InitialInsideRegionsWithoutBrideEdges.empty() )
			m_pInitialOutsideRegionWithoutBridgeEdges = oustsideRegions[0];
	}

	if( perpendicularRegions )
	{
		for( int i = 0, iPerpendicularSize(perpendicularRegions->size()); i < iPerpendicularSize; ++i )
		{
			CBrushRegion::RegionPtr pPerpendicularRegion = (*perpendicularRegions)[i];
			if( pPerpendicularRegion->CheckFlags(CBrushRegion::eRF_Mirrored) )
				continue;
			for( int k = 0, iEdgeSize(pRegion->GetEdgeSize()); k < iEdgeSize; ++k )
			{
				BrushEdge3D edge = pRegion->GetEdge(k);
				if( pPerpendicularRegion->IsEdgeOnCrust(edge) )
					m_EdgePlanePairs.push_back( EdgeBrushPlanePair(edge,pPerpendicularRegion->GetPlane()) );
			}
		}
	}

	m_pBaseObject = pObject;
	m_pDesigner = new CBrushDesigner;
	m_pBrush = new CBaseBrush(CBaseBrush::eBaseBrushFlag_CastShadow);
	m_pDB = pDB;
	if( m_pDB )
		m_pDB->AddRef();
}

CBrushArgument::~CBrushArgument()
{
	if( m_pDB )
		m_pDB->Release();
}

bool CBrushArgument::FindPlaneWithEdge( const BrushEdge3D& edge, const BrushPlane& hintPlane, BrushPlane& outPlane )
{
	for( int i = 0, iSize(m_EdgePlanePairs.size()); i < iSize; ++i )
	{
		if( m_EdgePlanePairs[i].first.IsEquivalent( edge, kDesignerEpsilon ) )
		{
			if( m_EdgePlanePairs[i].second.IsSameFacing(hintPlane) )
			{
				outPlane = m_EdgePlanePairs[i].second;
				return true;
			}
		}
	}
	return false;
}

void CBrushArgument::UpdateBrush()
{
	m_pBrush->Update(m_pBaseObject,m_pDesigner);
}

void CBrushArgument::Update(EBrushArgumentUpdate updateOp)
{
	if( m_pDesigner == NULL )
		return;
	m_pDesigner->Clear();

	m_CapPlane = BrushPlane(m_BasePlane.Normal(),m_BasePlane.Distance()-m_fHeight);
	
	AddCapRegions();
	AddSideRegions();

	if( updateOp == eBAU_UpdateBrush )
		UpdateBrush();
}

void CBrushArgument::AddCapRegions()
{
	if( m_pDesigner == NULL )
		return;

	UpdateRegionVertices2Plane(m_CapPlane);

	if( m_fHeight < 0.0f )
	{
		m_pDesigner->AddRegion(m_pRegion->Clone(), CBrushDesigner::eOpType_Add);
	}
	else
	{
		CBrushRegion::RegionPtr pBottomRegion = m_pRegion->Clone();
		pBottomRegion->Flip();
		m_pDesigner->AddRegion(pBottomRegion->Clone(), CBrushDesigner::eOpType_Add);
	}
}

void CBrushArgument::AddSideRegions()
{
	if( std::abs(m_fHeight) <= kDesignerEpsilon )
		return;

	std::vector<CBrushRegion::RegionPtr> regions;
	regions.push_back(m_pInitialOutsideRegionWithoutBridgeEdges);
	if( !m_InitialInsideRegionsWithoutBrideEdges.empty() )
		regions.insert(regions.end(),m_InitialInsideRegionsWithoutBrideEdges.begin(),m_InitialInsideRegionsWithoutBrideEdges.end());

	for( int a = 0, iRegionCount(regions.size()); a < iRegionCount; ++a )
	{
		CBrushRegion::RegionPtr pRegion = regions[a];

		for( int i = 0, iEdgeSize(pRegion->GetEdgeSize()); i < iEdgeSize; ++i )
		{
			BrushEdge3D e = pRegion->GetEdge(i);
			std::vector<BrushVec3> vSideList(4);

			m_CapPlane.HitTest(e.m_v[1], e.m_v[1]-m_CapPlane.Normal(), kDesignerEpsilon, NULL, &vSideList[0]);
			m_CapPlane.HitTest(e.m_v[0], e.m_v[0]-m_CapPlane.Normal(), kDesignerEpsilon, NULL, &vSideList[1]);
			vSideList[2] = e.m_v[0];
			vSideList[3] = e.m_v[1];

			if( m_fHeight < 0.0f )
			{
				std::swap(vSideList[0],vSideList[3]);
				std::swap(vSideList[1],vSideList[2]);
			}

			BrushPlane sidePlane( vSideList[0], vSideList[1], vSideList[2], kDesignerEpsilon );
			if( !FindPlaneWithEdge(e,sidePlane,sidePlane) )
			{
				if( m_pDB )
					m_pDB->FindPlane(sidePlane,sidePlane);
			}

			CBrushRegion::RegionPtr pSideRegion = new CBrushRegion(vSideList,sidePlane,m_pInitialRegion->GetMaterialID(),&m_pInitialRegion->GetTexInfo(),true);
			if( !pSideRegion->IsOpen() )
			{
				if( m_pDB )
					m_pDB->UpdateRegionVertices(pSideRegion);
				m_pDesigner->AddRegion(pSideRegion, CBrushDesigner::eOpType_Add);
			}
		}
	}
}

void CBrushArgument::UpdateRegionVertices2Plane( const BrushPlane& targetPlane )
{
	m_pRegion = m_pInitialRegion->Clone();
	for( int i = 0, iVertexSize(m_pRegion->GetVertexListSize()); i < iVertexSize; ++i )
	{
		const BrushVec3& in_v = m_pRegion->GetVertex(i);
		BrushVec3 out_v;
		if( targetPlane.HitTest( in_v, in_v-targetPlane.Normal(), kDesignerEpsilon, NULL, &out_v ) )
			m_pRegion->SetVertex( i, out_v );
	}
	m_pRegion->SetPlane(targetPlane);
}

bool CBrushArgument::GetRegionList( std::vector<CBrushRegion::RegionPtr>& outRegions ) const
{
	if( m_pDesigner->GetRegionSize() < 2 )
		return false;

	outRegions.push_back(m_pDesigner->GetRegion(0));

	GetSideRegionList(outRegions);

	return true;
}

bool CBrushArgument::GetSideRegionList( std::vector<CBrushRegion::RegionPtr>& outRegions ) const
{
	int iRegionSize(m_pDesigner->GetRegionSize());

	for( int i = 1; i < iRegionSize; ++i )
		outRegions.push_back(m_pDesigner->GetRegion(i));

	return true;
}

CBrushRegion::RegionPtr CBrushArgument::GetCapRegion() const
{
	if( m_pDesigner->GetRegionSize() <= 0 )
		return NULL;
	return m_pDesigner->GetRegion(0);
}

void CBrushArgument::SetHeight( BrushFloat fHeight )
{
	if( std::abs(fHeight) < kDesignerEpsilon )
		m_fHeight = 0;
	else
		m_fHeight = fHeight;
}

AABB CBrushArgument::GetBoundBox() const
{
	return m_pDesigner->GetBoundBox();
}