#include "StdAfx.h"
#include "BrushPrimitive.h"
#include "SurfaceInfoPicker.h"
#include "ViewManager.h"
#include "Grid.h"
#include "BrushCommonInterface.h"
#include "BrushDesignerPolygonDecomposer.h"

void CBrushPrimitive::CreateBox( const BrushVec3 &mins, const BrushVec3& maxs, std::vector<CBrushRegion::RegionPtr>* pOutRegionList ) const
{
	for(int i=0 ; i<3 ; ++i)
	{
		if (maxs[i] < mins[i])
			CLogFile::WriteLine ("Error: Failed to Create a box.");
	}

	BrushVec3 vlist[] = {
		BrushVec3(mins[0], mins[1], mins[2]), 
		BrushVec3(mins[0], maxs[1], mins[2]),
		BrushVec3(maxs[0], maxs[1], mins[2]),
		BrushVec3(maxs[0], mins[1], mins[2]),
		BrushVec3(mins[0], mins[1], maxs[2]),
		BrushVec3(mins[0], maxs[1], maxs[2]),
		BrushVec3(maxs[0], maxs[1], maxs[2]),
		BrushVec3(maxs[0], mins[1], maxs[2])	};

	int indexlist[][4] = {
		{0,4,5,1},
		{1,5,6,2},
		{2,6,7,3},
		{3,7,4,0},
		{6,5,4,7},
		{0,1,2,3} };

	std::vector<CBrushRegion::RegionPtr> regionList;

	int indexnum = sizeof(indexlist)/sizeof(*indexlist);
	for( int i=0; i<indexnum; ++i )
	{
		std::vector<BrushVec3> vList;
		vList.reserve(4);
		for( int k=0; k<4; ++k )
			vList.push_back(vlist[indexlist[i][k]]);
		BrushPlane plane;
		if( BUtil::ComputePlane( vList, plane ) )
		{
			CBrushRegion* pRegion = new CBrushRegion(vList);
			if( pRegion->IsValid() && !pRegion->IsOpen() )
				regionList.push_back(pRegion);
		}
	}

	if( pOutRegionList )
		*pOutRegionList = regionList;
	else
		m_pBrushCreator->Reset(regionList);
}


void CBrushPrimitive::CreateSphere( const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<CBrushRegion::RegionPtr>* pOutRegionList ) const
{
	BrushFloat radius = (Vec2(maxs.x,mins.y)-Vec2(mins.x,mins.y)).GetLength()*(BrushFloat)0.5;
	BrushVec3 vOffset = (maxs+mins)*(BrushFloat)0.5 + BrushVec3(0,0,radius);
	CreateSphere( vOffset, radius, numSides, pOutRegionList );
}

void CBrushPrimitive::CreateSphere( const BrushVec3& vCenter, float radius, int numSides, std::vector<CBrushRegion::RegionPtr>* pOutRegionList ) const
{
	DESIGNER_ASSERT( numSides >= 3 );
	if( numSides < 3 )
		return;	

	BrushFloat costheta(radius*std::cos((BrushFloat)0));
	BrushFloat sintheta(radius*std::sin((BrushFloat)0));
	BrushFloat cosnexttheta(costheta);
	BrushFloat sinnexttheta(sintheta);

	std::vector<CBrushRegion::RegionPtr> regionList;	

	for( int i = 1; i < numSides+1; ++i )
	{
		BrushFloat nexttheta = BUtil::PI * (BrushFloat)i/(BrushFloat)numSides;

		cosnexttheta = radius * std::cos(nexttheta);
		sinnexttheta = radius * std::sin(nexttheta);

		BrushFloat cosphi(std::cos((BrushFloat)0));
		BrushFloat sinphi(std::sin((BrushFloat)0));
		BrushFloat cosnextphi(cosphi);
		BrushFloat sinnextphi(sinphi);

		for( int j = 1; j < numSides+1; ++j )
		{
			BrushFloat nextphi = 2 * BUtil::PI * (BrushFloat)j/(BrushFloat)numSides;

			cosnextphi = std::cos(nextphi);
			sinnextphi = std::sin(nextphi);

			std::vector<BrushVec3> vList;

			if( i == 1 || i == numSides )
			{
				vList.reserve(3);
				vList.push_back(BrushVec3(sintheta*cosphi, sintheta*sinphi,	costheta)+vCenter);

				if( i == 1 )
				{
					vList.push_back(BrushVec3(sinnexttheta*cosphi, sinnexttheta*sinphi, cosnexttheta)+vCenter);
					vList.push_back(BrushVec3(sinnexttheta*cosnextphi, sinnexttheta*sinnextphi, cosnexttheta)+vCenter);
				}
				else if( i == numSides )
				{
					vList.push_back(BrushVec3(sinnexttheta*cosnextphi, sinnexttheta*sinnextphi, cosnexttheta)+vCenter);
					vList.push_back(BrushVec3(sintheta*cosnextphi, sintheta*sinnextphi, costheta)+vCenter);
				}

				regionList.push_back(new CBrushRegion(vList));
			}
			else
			{
				vList.reserve(4);
				vList.push_back(BrushVec3(sinnexttheta*cosphi, sinnexttheta*sinphi, cosnexttheta)+vCenter);
				vList.push_back(BrushVec3(sinnexttheta*cosnextphi, sinnexttheta*sinnextphi, cosnexttheta)+vCenter);
				vList.push_back(BrushVec3(sintheta*cosnextphi, sintheta*sinnextphi,costheta)+vCenter);
				vList.push_back(BrushVec3(sintheta*cosphi, sintheta*sinphi, costheta)+vCenter);
				regionList.push_back(new CBrushRegion(vList));
			}

			cosphi = cosnextphi;
			sinphi = sinnextphi;
		}

		costheta = cosnexttheta;
		sintheta = sinnexttheta;
	}

	if( pOutRegionList )
		*pOutRegionList = regionList;
	else
		m_pBrushCreator->Reset(regionList);
}

void CBrushPrimitive::CreateCylinder( const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<CBrushRegion::RegionPtr>* pOutRegionList ) const
{
	DESIGNER_ASSERT( numSides >= 3 );
	if( numSides < 3 )
		return;

	BrushVec3 bottomMax(maxs.x,maxs.y,0);
	BrushVec3 bottomMin(mins.x,mins.y,0);
	BrushFloat radius = (bottomMax-bottomMin).GetLength()*(BrushFloat)0.5;
	BrushVec3 bottomCenter = (bottomMin + bottomMax)*(BrushFloat)0.5;

	BrushFloat costheta(cosf(0));
	BrushFloat sintheta(sinf(0));

	std::vector<CBrushRegion::RegionPtr> regionList;

	std::vector<BrushVec3> bottomFace;
	bottomFace.resize(numSides);
	for( int i = 0; i < numSides; ++i )
	{
		BrushFloat theta = 2 * BUtil::PI * (BrushFloat)i/(BrushFloat)numSides;
		costheta = cosf(theta);
		sintheta = sinf(theta);
		BrushFloat x = radius * costheta;
		BrushFloat y = radius * sintheta;
		bottomFace[numSides-i-1] = BrushVec3(x,y,mins.z)+bottomCenter;
	}

	std::vector<BrushVec3> topFace;
	topFace.resize(numSides);
	for( int i = 0; i < numSides; ++i )
	{
		const BrushVec3& v = bottomFace[i];
		topFace[numSides-i-1] = BrushVec3(v.x,v.y,maxs.z);
	}

	CBrushRegion* pBottomRegion = new CBrushRegion(bottomFace);
	CBrushRegion* pTopRegion = new CBrushRegion(topFace);

	regionList.push_back(pBottomRegion);
	regionList.push_back(pTopRegion);

	for( int i = 0; i < numSides; ++i )
	{
		std::vector<BrushVec3> vSideList;
		vSideList.reserve(4);

		int nexti = (i+1)%numSides;

		vSideList.push_back(topFace[numSides-i-1]);
		vSideList.push_back(topFace[numSides-nexti-1]);
		vSideList.push_back(bottomFace[nexti]);
		vSideList.push_back(bottomFace[i]);

		CBrushRegion::RegionPtr pRegion = new CBrushRegion(vSideList);
		if( pRegion->IsValid() && !pRegion->IsOpen() )
			regionList.push_back(new CBrushRegion(vSideList));
	}

	if( pOutRegionList )
		*pOutRegionList = regionList;
	else
		m_pBrushCreator->Reset(regionList);
}

void CBrushPrimitive::CreateCylinder( CBrushRegion::RegionPtr pBaseDiscRegion, float fHeight, std::vector<CBrushRegion::RegionPtr>* pOutRegionList ) const
{
	AABB boundbox = pBaseDiscRegion->GetBoundBox();
	BrushVec3 vCenter = boundbox.GetCenter();

	std::vector<CBrushRegion::RegionPtr> regionList;

	CBrushRegion::RegionPtr pTopRegion = pBaseDiscRegion->Clone()->Flip();
	BrushMatrix34 matToTop = BrushMatrix34::CreateIdentity();
	matToTop.SetTranslation(pBaseDiscRegion->GetPlane().Normal()*(-fHeight));
	pTopRegion->Transform(matToTop);

	regionList.push_back(pTopRegion);

	for( int i = 0, iEdgeCount(pBaseDiscRegion->GetEdgeSize()); i < iEdgeCount; ++i )
	{
		BrushEdge3D baseEdge = pBaseDiscRegion->GetEdge(i);
		BrushEdge3D topEdge = pTopRegion->GetEdge(iEdgeCount-i-1);

		std::vector<BrushVec3> vSideList;
		vSideList.reserve(4);
		vSideList.push_back(topEdge.m_v[1]);
		vSideList.push_back(topEdge.m_v[0]);
		vSideList.push_back(baseEdge.m_v[1]);
		vSideList.push_back(baseEdge.m_v[0]);

		CBrushRegion::RegionPtr pRegion = new CBrushRegion(vSideList);
		if( pRegion->IsValid() && !pRegion->IsOpen() )
			regionList.push_back(pRegion);
	}

	regionList.push_back(pBaseDiscRegion->Clone());

	if( pOutRegionList )
		*pOutRegionList = regionList;
	else
		m_pBrushCreator->Reset(regionList);
}

void CBrushPrimitive::CreateCone( const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<CBrushRegion::RegionPtr>* pOutRegionList ) const
{
	DESIGNER_ASSERT( numSides >= 3 );
	if( numSides < 3 )
		return;

	std::vector<CBrushRegion::RegionPtr> regionList;
	std::vector<BrushVec3> vBottomList;

	CreateCircle( mins, maxs, numSides, vBottomList );
	regionList.push_back(new CBrushRegion(vBottomList));

	BrushVec3 bottomCenter((BrushVec3(maxs.x,maxs.y,0)+BrushVec3(mins.x,mins.y,0))*(BrushFloat)0.5);
	BrushVec3 peak(bottomCenter.x,bottomCenter.y,maxs.z);

	for( int i = 0; i < numSides; ++i )
	{
		std::vector<BrushVec3> vSideList;
		vSideList.reserve(3);
		vSideList.push_back(peak);
		vSideList.push_back(vBottomList[(i+1)%numSides]);
		vSideList.push_back(vBottomList[i]);
		CBrushRegion::RegionPtr pRegion = new CBrushRegion(vSideList);
		if( pRegion->IsValid() && !pRegion->IsOpen() )
			regionList.push_back(pRegion);
	}

	if( pOutRegionList )
		*pOutRegionList = regionList;
	else
		m_pBrushCreator->Reset(regionList);
}

void CBrushPrimitive::CreateCone( CBrushRegion::RegionPtr pBaseDiscRegion, float fHeight, std::vector<CBrushRegion::RegionPtr>* pOutRegionList ) const
{
	AABB boundbox = pBaseDiscRegion->GetBoundBox();
	BrushVec3 vCenter = boundbox.GetCenter();
	BrushVec3 peak = vCenter + pBaseDiscRegion->GetPlane().Normal() * (-fHeight);

	std::vector<CBrushRegion::RegionPtr> regionList;

	for( int i = 0, iEdgeCount(pBaseDiscRegion->GetEdgeSize()); i < iEdgeCount; ++i )
	{
		BrushEdge3D edge = pBaseDiscRegion->GetEdge(i);
		std::vector<BrushVec3> vSideList;
		vSideList.reserve(3);
		vSideList.push_back(peak);
		vSideList.push_back(edge.m_v[1]);
		vSideList.push_back(edge.m_v[0]);
		CBrushRegion::RegionPtr pRegion = new CBrushRegion(vSideList);
		if( pRegion->IsValid() && !pRegion->IsOpen() )
			regionList.push_back(pRegion);
	}

	regionList.push_back(pBaseDiscRegion->Clone());

	if( pOutRegionList )
		*pOutRegionList = regionList;
	else
		m_pBrushCreator->Reset(regionList);
}

void CBrushPrimitive::CreateRectangle( const BrushVec3& mins, const BrushVec3& maxs, std::vector<CBrushRegion::RegionPtr>* pOutRegionList ) const
{
	if (maxs[0] < mins[0] || maxs[1] < mins[1])
		CLogFile::WriteLine ("Error: Failed to Creat a Rectangle");

	std::vector<BrushVec3> vList;
	vList.reserve(4);
	vList.push_back(BrushVec3(mins[0],mins[1],mins[2]));
	vList.push_back(BrushVec3(maxs[0],mins[1],mins[2]));
	vList.push_back(BrushVec3(maxs[0],maxs[1],mins[2]));
	vList.push_back(BrushVec3(mins[0],maxs[1],mins[2]));

	BrushPlane plane;
	if( !BUtil::ComputePlane(vList,plane) )
		return;

	std::vector<CBrushRegion::RegionPtr> regionList;
	regionList.push_back(new CBrushRegion(vList));

	if( pOutRegionList )
		*pOutRegionList = regionList;
	else
		m_pBrushCreator->Reset(regionList);
}

void CBrushPrimitive::CreateDisc( const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<CBrushRegion::RegionPtr>* pOutRegionList ) const
{
	DESIGNER_ASSERT( numSides >= 3 );

	std::vector<BrushVec3> vBottomList;
	CreateCircle( mins, maxs, numSides, vBottomList );

	std::vector<BrushVec3> vBottomReverseList;

	vBottomReverseList.insert( vBottomReverseList.end(), vBottomList.rbegin(), vBottomList.rend() );

	std::vector<CBrushRegion::RegionPtr> regionList;
	regionList.push_back(new CBrushRegion(vBottomReverseList));

	if( pOutRegionList )
		*pOutRegionList = regionList;
	else
		m_pBrushCreator->Reset(regionList);
}

void CBrushPrimitive::CreateCircle( const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<BrushVec3>& outVertexList ) const
{
	BrushVec3 bottomMax(maxs.x,maxs.y,0);
	BrushVec3 bottomMin(mins.x,mins.y,0);
	BrushFloat radius = (bottomMax-bottomMin).GetLength()*(BrushFloat)0.5;
	BrushVec3 bottomCenter((bottomMax+bottomMin)*(BrushFloat)0.5);

	BrushFloat costheta(std::cos((BrushFloat)0));
	BrushFloat sintheta(std::sinf((BrushFloat)0));

	std::vector<CBrushRegion::RegionPtr> regionList;

	outVertexList.clear();
	outVertexList.resize(numSides);
	for( int i = 0; i < numSides; ++i )
	{
		BrushFloat theta = 2 * BUtil::PI * (BrushFloat)i/(BrushFloat)numSides;

		costheta = cosf(theta);
		sintheta = sinf(theta);

		BrushFloat x = radius * costheta;
		BrushFloat y = radius * sintheta;

		outVertexList[numSides-i-1] = BrushVec3(x,y,mins.z)+bottomCenter;
	}
}

void CSolidCustomPrimitive::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	_smart_ptr<CBaseBrushCreator> pCreator = GetBrushCreator();
	if( pCreator == NULL )
		return;
	if( m_State == eSCS_RaiseHeight )
	{
		std::vector<CBrushRegion::RegionPtr> regions;
		if( m_ArgumentBrush->GetRegionList(regions) )
			pCreator->Attach(regions,true);
		m_State = eSCS_End;
	}
	else if( m_State == eSCS_DrawBase )
	{
		ApplyInitialPosToBrush(point);
		if( GetSpotListCount() )
			UpdateBaseFace( view, nFlags, point );
	}
}

void CSolidCustomPrimitive::OnLButtonUp( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_State == eSCS_DrawBase )	
	{
		ApplyInitialPosToBrush(point);

		m_mouseDownPos = point;

		BrushVec3 vHitPos;
		if( !GetHitPosition(point,vHitPos) )
			return;

		view->SetConstructionOrigin(vHitPos);
		return CBrushDesignerDrawLineTool::OnLButtonUp( view, nFlags, point );
	}
}

bool CSolidCustomPrimitive::GetHitPosition( const CPoint& point, BrushVec3& outPos )
{
	CSurfaceInfoPicker picker;
	SRayHitInfo hitInfo;
	CSurfaceInfoPicker::CExcludedObjects excludedObjects;
	excludedObjects.Add(m_pBrushCreator->GetBaseObject());
	if( !picker.Pick( point, hitInfo, &excludedObjects ) )
		return false;
	outPos = GetIEditor()->GetViewManager()->GetGrid()->Snap(hitInfo.vHitPos);
	return true;
}

void CSolidCustomPrimitive::ApplyInitialPosToBrush( const CPoint& point )
{
	_smart_ptr<CBaseBrushCreator> pBrush = GetBrushCreator();
	if( pBrush == NULL )
		return;
	if( GetSpotListCount() == 0 )
		return;
	BrushVec3 initPos;
	if( GetHitPosition(point,initPos) )
		pBrush->SetPos(initPos);
} 

void CSolidCustomPrimitive::CreateRegionFromSpots( bool bClosedRegion, const SpotList& spotList )
{
	_smart_ptr<CBaseBrushCreator> pBrush = GetBrushCreator();
	if( pBrush == NULL )
		return;

	AABB boundbox;
	boundbox.Reset();
	int iSpotSize(GetSpotListCount());
	for( int i = 0; i < iSpotSize; ++i )
		boundbox.Add(GetSpotPos(i));
	pBrush->SetPos(boundbox.GetCenter());

	std::vector<BrushVec3> vList;
	GenerateVertexListFromSpotList(spotList,vList);
	for( int i = 0, iVListCount(vList.size()); i < iVListCount; ++i )
		vList[i] -= boundbox.GetCenter();

	SetIntermediateRegion( new CBrushRegion( vList, BrushPlane(BrushVec3(0,0,1),0), 0, NULL, true ) );
	if( GetIntermediateRegion() )
		GetIntermediateRegion()->ModifyOrientation();
	m_State = eSCS_RaiseHeight;

	m_ArgumentBrush = new CBrushArgument( GetIntermediateRegion(), m_pObject, NULL, NULL );
}

void CSolidCustomPrimitive::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_State == eSCS_DrawBase )
	{
		ApplyInitialPosToBrush(point);
		UpdateBaseFace( view, nFlags, point );
	}
	else if( m_State == eSCS_RaiseHeight )
	{
		RaiseHeight( view, nFlags, point );
	}
}

void CSolidCustomPrimitive::UpdateBaseFace( CViewport *view,UINT nFlags,CPoint point )
{
	_smart_ptr<CBaseBrushCreator> pBrush = GetBrushCreator();
	if( pBrush == NULL )
		return;

	if( GetSpotListCount() == 0 )
	{
		BrushVec3 vHitPos;
		if( GetHitPosition(point,vHitPos) )
		{
			SetCurrentSpotPos(vHitPos);
		}
		else
		{
			SetCurrentSpotPos(ToVec3(view->MapViewToCP(point)));
		}
		SetPlane( BrushPlane(BrushVec3(0,0,1),-GetCurrentSpotPos().z) );
	}
	else
	{
		BrushVec3 srcRay;
		BrushVec3 dirRay;

		Vec3 srcRayf32;
		Vec3 dirRayf32;
		view->ViewToWorldRay(point,srcRayf32,dirRayf32);
		srcRay = BrushVec3( BrushFloat(srcRayf32.x), BrushFloat(srcRayf32.y), BrushFloat(srcRayf32.z) );
		dirRay = BrushVec3( BrushFloat(dirRayf32.x), BrushFloat(dirRayf32.y), BrushFloat(dirRayf32.z) );

		BrushVec3 outPos;
		GetPlane().HitTest( srcRay, srcRay+dirRay, kDesignerEpsilon, NULL, &outPos);
		SetCurrentSpotPos(outPos);
	}

	if( GetSpotListCount() == 0 )
		pBrush->SetPos(GetCurrentSpotPos());

	const BrushFloat fLimitForMagnetic(9.0f);
	SetCurrentSpotPosState(eSpotPosState_InRegion);

	if( BUtil::AreTwoPositionsNear(GetCurrentSpotPos(),GetStartSpotPos(),GetWorldTM(),view,fLimitForMagnetic) )
		SetCurrentSpot(GetStartSpot());

	if( GetSpotListCount() > 0 )
	{
		if( IntersectExisintingLines( GetSpotPos(GetSpotListCount()-1), GetCurrentSpotPos() ) )
			SetLineState(eLineState_Cross);
		else
			SetLineState(eLineState_Diagonal);

		SetCurrentSpotPos(BrushVec3(GetCurrentSpotPos().x,GetCurrentSpotPos().y,GetStartSpotPos().z));
	}	
}

void CSolidCustomPrimitive::RaiseHeight( CViewport *view,UINT nFlags,CPoint point )
{
	BrushVec3 src = view->MapViewToCP(m_mouseDownPos,AXIS_Z);
	BrushVec3 trg = view->MapViewToCP(point,AXIS_Z);
	BrushVec3 dir = view->GetCPVector(src,trg,AXIS_Z);
	m_ArgumentBrush->SetHeight(dir.z);
	m_ArgumentBrush->Update(CBrushArgument::eBAU_UpdateBrush);
}

bool CSolidCustomPrimitive::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if (nChar == VK_ESCAPE )
		m_State = eSCE_Cancel;
	return true;
}

CBaseBrushCreator* CSolidCustomPrimitive::GetBrushCreator() const
{
	if( !m_pBrushCreator )
	{
		CBaseBrushCreator* pBrush = NULL;
		if( CBrushCommonInterface::GetBrushCreator(m_pObject,pBrush) == false )
			return NULL;
		m_pBrushCreator = pBrush;
	}
	return m_pBrushCreator;
}

void CSolidCustomPrimitive::Display( DisplayContext &dc )
{
	if( m_State == eSCS_DrawBase )
	{
		CBrushDesignerDrawLineTool::Display(dc);
	}
	else if( m_ArgumentBrush && m_pObject )
	{
		m_pObject->DrawDimensionsImpl(dc,m_ArgumentBrush->GetBoundBox());
	}
}