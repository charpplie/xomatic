#include "StdAfx.h"
#include "BrushDesignerCubeEditor.h"
#include "Core/BrushBSPTree3D.h"
#include "ViewManager.h"
#include "Core/BrushPrimitive.h"
#include "Material/MaterialManager.h"
#include "Core/BrushDesignerAdjustHeightHelper.h"
#include "IBaseToolPanel.h"

namespace 
{
	ICubeEditorPanel* s_pCubeEditorPanel = NULL;
}

void CBrushDesignerCubeEditor::Enter()
{
	__super::Enter();
	m_BrushAABB = AABB(Vec3(0,0,0),Vec3(0,0,0));
	m_CurMousePos = CPoint(-1,-1);
	GetIEditor()->GetMaterialManager()->AddListener(this);
}

void CBrushDesignerCubeEditor::Leave()
{
	__super::Leave();
	GetIEditor()->GetMaterialManager()->RemoveListener(this);
}

void CBrushDesignerCubeEditor::BeginEditParams()
{
	if( !s_pCubeEditorPanel )
		s_pCubeEditorPanel = CreateCubeEditorPanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerCubeEditor::EndEditParams()
{
	if( s_pCubeEditorPanel )
	{
		s_pCubeEditorPanel->DestroyPanel();
		s_pCubeEditorPanel = NULL;
	}
}

BrushVec2 ConvertTwoPositionsToDirInViewport( CViewport *view, const Vec3& v0, const Vec3& v1 )
{
	POINT p0 = view->WorldToView(v0);
	POINT p1 = view->WorldToView(v1);
	BrushVec2 vDir = BrushVec2(BrushFloat(p1.x-p0.x),BrushFloat(p1.y-p0.y));
	if( vDir.x != 0 || vDir.y != 0 )
		vDir.Normalize();
	return vDir;
}

void CBrushDesignerCubeEditor::OnLButtonDown( CViewport *view, UINT nFlags, CPoint point )
{
	__super::OnLButtonDown(view,nFlags,point);
	if( nFlags & MK_SHIFT )
	{
		m_DS.m_bPressingShift = true;
		BrushVec3 vPickedPos, vNormal;
		GetBrushPos(view,point,m_DS.m_StartingPos,vPickedPos,&m_DS.m_StartingNormal);
		m_BrushAABB = GetBrushBox(view,point);
	}
	else
	{
		m_DS.m_bPressingShift = false;
	}
}

void CBrushDesignerCubeEditor::OnLButtonUp( CViewport *view, UINT nFlags, CPoint point )
{
	if( !m_DS.m_bPressingShift )
	{
		m_BrushAABB = GetBrushBox(view,point);
		AddBrush(m_BrushAABB);
	}

	m_DS.m_bPressingShift = false;
	m_CurMousePos = point;

	if( m_BrushAABBs.empty() || GetBaseObject() == NULL )
		return;

	CUndo undo("Designer : CubeEditor");
	GetDesigner()->RecordUndo("Designer : CubeEditor",GetBaseObject());

	bool bEmptyDesigner = GetDesigner()->IsEmpty();

	for( int i = 0, iBrushAABBCount(m_BrushAABBs.size()); i < iBrushAABBCount; ++i )
	{
		if( GetEditMode() == eEditorMode_Add )
			AddCube(m_BrushAABBs[i]);
		else if( GetEditMode() == eEditorMode_Remove )
			RemoveCube(m_BrushAABBs[i]);
		else if( GetEditMode() == eEditorMode_Paint )
			PaintCube(m_BrushAABBs[i]);
	}

	if( bEmptyDesigner )
	{
		AABB aabb = GetDesigner()->GetBoundBox();
		GetBrush()->PivotToPos(GetBaseObject(),GetDesigner(),aabb.min);
	}

	GetDesigner()->ResetDB(BUtil::eDBRF_Vertex);
	UpdateBrush();
	m_BrushAABBs.clear();
}

BrushVec3 CBrushDesignerCubeEditor::Snap( const BrushVec3& vPos ) const
{
	BrushVec3 vCorrectedPos(BUtil::CorrectVec3(vPos));
	BrushFloat fCubeSize = s_pCubeEditorPanel->GetCubeSize();

	std::vector<BrushFloat> fCutUnits;
	if( std::abs(fCubeSize-(BrushFloat)0.125) < kDesignerEpsilon )
	{
		fCutUnits.push_back((BrushFloat)0.125);
		fCutUnits.push_back((BrushFloat)0.25);
		fCutUnits.push_back((BrushFloat)0.375);
		fCutUnits.push_back((BrushFloat)0.5);
		fCutUnits.push_back((BrushFloat)0.625);
		fCutUnits.push_back((BrushFloat)0.75);
		fCutUnits.push_back((BrushFloat)0.875);
	}
	else if( std::abs(fCubeSize-(BrushFloat)0.25) < kDesignerEpsilon )
	{
		fCutUnits.push_back((BrushFloat)0.25);
		fCutUnits.push_back((BrushFloat)0.5);
		fCutUnits.push_back((BrushFloat)0.75);
	}
	else if( std::abs(fCubeSize-(BrushFloat)0.5) < kDesignerEpsilon )
	{
		fCutUnits.push_back((BrushFloat)0.5);
	}

	for( int i = 0; i < 3; ++i )
	{
		for( int k = 0, iCutUnitCount(fCutUnits.size()); k < iCutUnitCount; ++k )
		{
			BrushFloat fGreatedLessThan = std::floor(vCorrectedPos[i]) + fCutUnits[k];
			if( std::abs(vCorrectedPos[i]-fGreatedLessThan) < kDesignerEpsilon )
			{
				vCorrectedPos[i] = fGreatedLessThan;
				break;
			}
		}
	}

	BrushFloat fSnapSize = s_pCubeEditorPanel->GetCubeSize();
	BrushVec3 snapped;
	snapped.x = std::floor(vCorrectedPos.x/fSnapSize)*fSnapSize;
	snapped.y = std::floor(vCorrectedPos.y/fSnapSize)*fSnapSize;
	snapped.z = std::floor(vCorrectedPos.z/fSnapSize)*fSnapSize;

	return snapped;
}

bool CBrushDesignerCubeEditor::GetBrushPos( CViewport *view, CPoint point, BrushVec3& outPos, BrushVec3& outPickedPos, BrushVec3* pOutNormal )
{
	BrushVec3 localRaySrc, localRayDir;
	BUtil::GetLocalViewRay( GetWorldTM(), view, point, localRaySrc, localRayDir );	
	BrushPlane plane;
	if( GetDesigner()->QueryPosition(localRaySrc, localRayDir, outPickedPos, &plane) )
	{
		if( pOutNormal )
			*pOutNormal = plane.Normal();
		outPos = Snap(outPickedPos);
		return true;
	}

	if( !s_pCubeEditorPanel->IsAddButtonChecked() )
		return false; 

	Vec3 vPickedPosInWorld;
	if( BUtil::PickPosFromWorld(view, point, vPickedPosInWorld) )
	{
		BrushVec3 vLocalPickedPos = GetWorldTM().GetInverted().TransformPoint(ToBrushVec3(vPickedPosInWorld));
		if( pOutNormal )
			*pOutNormal = BrushVec3(0,0,1);
		outPickedPos = ToVec3(vPickedPosInWorld);
		outPos = Snap(vLocalPickedPos);
		return true;
	}

	return false;
}

void CBrushDesignerCubeEditor::OnMouseMove( CViewport *view, UINT nFlags, CPoint point )
{
	if( !m_DS.m_bPressingShift )
		m_BrushAABB = GetBrushBox(view,point);

	if( nFlags & MK_LBUTTON )
	{
		if( m_DS.m_bPressingShift )
		{
			BrushVec3 vWorldPivot = GetWorldTM().TransformPoint(m_DS.m_StartingPos);
			BrushVec2 vRightDirInViewport = ConvertTwoPositionsToDirInViewport(view, vWorldPivot, vWorldPivot+GetBaseObject()->GetWorldTM().GetColumn0());
			BrushVec2 vForwardDirInViewport = ConvertTwoPositionsToDirInViewport(view, vWorldPivot, vWorldPivot+GetBaseObject()->GetWorldTM().GetColumn1());
			BrushVec2 vUpDirInViewport = ConvertTwoPositionsToDirInViewport(view, vWorldPivot, vWorldPivot+GetBaseObject()->GetWorldTM().GetColumn2());

			CPoint startPosInViewport = view->WorldToView(vWorldPivot);

			BrushVec2 dir = BrushVec2(point.x-startPosInViewport.x,point.y-startPosInViewport.y).GetNormalized();

			BrushFloat angleToUp = dir.Dot(vUpDirInViewport);
			BrushFloat angleToRight = dir.Dot(vRightDirInViewport);
			BrushFloat angleToForward = dir.Dot(vForwardDirInViewport);

			BrushFloat fInvertUp = 1, fInvertRight = 1, fInvertForward = 1;
			if( angleToUp < 0 )
			{
				angleToUp = dir.Dot(-vUpDirInViewport);
				fInvertUp = -1;
			}
			if( angleToRight < 0 )
			{
				angleToRight = dir.Dot(-vRightDirInViewport);
				fInvertRight = -1;
			}
			if( angleToForward < 0 )
			{
				angleToForward = dir.Dot(-vForwardDirInViewport);
				fInvertForward = -1;
			}

			if( angleToUp > angleToRight && angleToUp > angleToForward )
				m_DS.m_StraightDir = GetWorldTM().GetColumn2()*fInvertUp;
			else if( angleToRight > angleToUp && angleToRight > angleToForward )
				m_DS.m_StraightDir = GetWorldTM().GetColumn0()*fInvertRight;
			else
				m_DS.m_StraightDir = GetWorldTM().GetColumn1()*fInvertForward;

			CBrushDesignerAdjustHeightHelper adjustHeightHelper;
			adjustHeightHelper.Init(BrushPlane(m_DS.m_StraightDir,-m_DS.m_StraightDir.Dot(m_DS.m_StartingPos)),m_DS.m_StartingPos);
			BrushFloat fLength = adjustHeightHelper.UpdateHeight(GetWorldTM(),view,point);
			BrushFloat fCubeSize = s_pCubeEditorPanel->GetCubeSize();

			int nNumber = fLength/fCubeSize;
			m_BrushAABBs.clear();
			for( int i = 0; i <= nNumber; ++i )
			{
				BrushVec3 vPickedPos = m_DS.m_StartingPos+m_DS.m_StraightDir*fCubeSize*i;
				AddBrush(GetBrushBox(Snap(vPickedPos),vPickedPos,m_DS.m_StartingNormal));
			}

			m_DS.m_StraightDir *= fLength;
		}
		AddBrush(m_BrushAABB);
	}
	m_CurMousePos = point;
}

void CBrushDesignerCubeEditor::OnMouseWheel( CViewport *view,UINT nFlags,CPoint point )
{
	short zDelta = (short)nFlags;

	if( zDelta > 0 )
		s_pCubeEditorPanel->SelectPrevBrush();
	else if( zDelta < 0 )
		s_pCubeEditorPanel->SelectNextBrush();

	if( m_CurMousePos.x == -1 )
		m_CurMousePos = point;

	m_BrushAABB = GetBrushBox( view, m_CurMousePos );
}

void CBrushDesignerCubeEditor::Display( DisplayContext &dc )
{
	__super::Display(dc);
	DisplayBrush(dc);

#ifdef DEBUG
	if( m_DS.m_bPressingShift )
		dc.DrawLine(m_DS.m_StartingPos,m_DS.m_StartingPos+m_DS.m_StraightDir,ColorF(1,0,0,1),ColorF(1,0,0,1));
#endif
}

void CBrushDesignerCubeEditor::DisplayBrush( DisplayContext& dc )
{
	for( int i = 0, iBrushCount(m_BrushAABBs.size()); i < iBrushCount; ++i )
	{
		if( m_BrushAABBs[i].IsReset() )
			continue;
		dc.SetColor(0.43f,0.43f,0,0.43f);
		dc.DrawSolidBox( ToVec3(m_BrushAABBs[i].min), ToVec3(m_BrushAABBs[i].max) );
	}

	if( !m_BrushAABB.IsReset() )
	{
		dc.SetColor(0.8392f,0.58f,0,0.43f);
		dc.DrawSolidBox( ToVec3(m_BrushAABB.min), ToVec3(m_BrushAABB.max) );
	}
}

AABB CBrushDesignerCubeEditor::GetBrushBox(  CViewport *view, CPoint point  )
{
	BrushVec3 vBrushPos,vPickedPos,vNormal;
	if( !GetBrushPos(view,point,vBrushPos,vPickedPos,&vNormal) )
	{
		AABB brushAABB;
		brushAABB.Reset();
		return brushAABB;
	}
	return GetBrushBox(vBrushPos,vPickedPos,vNormal);
}

AABB CBrushDesignerCubeEditor::GetBrushBox( const BrushVec3& vSnappedPos, const BrushVec3& vPickedPos, const BrushVec3& vNormal )
{
	AABB brushAABB;
	brushAABB.Reset();

	int nElement = -1;
	if( std::abs(vNormal.x) > (BrushFloat)0.999 )
		nElement = 0;
	else if( std::abs(vNormal.y) > (BrushFloat)0.999 )
		nElement = 1;
	else if( std::abs(vNormal.z) > (BrushFloat)0.999 )
		nElement = 2;

	const float fCubeSize = ToFloat(s_pCubeEditorPanel->GetCubeSize());
	Vec3 vSize(fCubeSize,fCubeSize,fCubeSize);

	AABB aabb;
	aabb.Reset();
	aabb.Add(ToVec3(vSnappedPos));
	aabb.Add(ToVec3(vSnappedPos+vSize));
	aabb.Expand(Vec3(-kDesignerEpsilon,-kDesignerEpsilon,-kDesignerEpsilon));
	if( aabb.IsContainPoint(ToVec3(vPickedPos)) )
		nElement = -1;

	if( nElement != -1 )
	{
		int nSign = vNormal[nElement] > 0 ? 1 : -1;
		if( GetEditMode() != eEditorMode_Add )
			nSign = -nSign;		
		vSize[nElement] = vSize[nElement] * nSign;
	}

	brushAABB.Add(ToVec3(vSnappedPos));
	brushAABB.Add(ToVec3(vSnappedPos+vSize));

	return brushAABB;
}

std::vector<CBrushRegion::RegionPtr> CBrushDesignerCubeEditor::GetBrushRegions( const AABB& aabb ) const
{
	std::vector<CBrushRegion::RegionPtr> brushRegions;
	if( !aabb.IsReset() )
	{
		CBrushPrimitive bp(NULL);	
		bp.CreateBox(aabb.min, aabb.max, &brushRegions);

		int nMatID = s_pCubeEditorPanel->GetSubMatID();
		for( int i = 0, iRegionCount(brushRegions.size()); i < iRegionCount; ++i )
			brushRegions[i]->SetMaterialID(nMatID);
	}
	return brushRegions;
}

void CBrushDesignerCubeEditor::AddCube( const AABB& brushAABB )
{
	std::vector<CBrushRegion::RegionPtr> brushRegions = GetBrushRegions(brushAABB);
	if( brushRegions.empty() )
		return;

	AABB reducedBrushAABB = brushAABB;
	AABB enlargedBrushAABB = brushAABB;
	const float kOffset = (float)kDesignerEpsilon;
	reducedBrushAABB.Expand(Vec3(-kOffset,-kOffset,-kOffset));
	enlargedBrushAABB.Expand(Vec3(kOffset,kOffset,kOffset));
	std::vector<CBrushRegion::RegionPtr> intersectedRegions;
	if( GetDesigner()->QueryIntersectedRegionsByAABB(reducedBrushAABB,intersectedRegions) )
	{
		std::vector<CBrushRegion::RegionPtr> originalIntersectedRegions(intersectedRegions);

		std::vector<CBrushRegion::RegionPtr>::iterator ii = intersectedRegions.begin();
		for( ; ii != intersectedRegions.end(); )
		{
			if( brushAABB.ContainsBox((*ii)->GetBoundBox()) )
			{
				GetDesigner()->RemoveRegion(*ii);
				ii = intersectedRegions.erase(ii);
			}
			else
			{
				++ii;
			}
		}

		if( !intersectedRegions.empty() )
		{
			_smart_ptr<CBrushBSPTree3D> pBSPTreeForBrush = new CBrushBSPTree3D(brushRegions);
			for( ii = intersectedRegions.begin(); ii != intersectedRegions.end(); ++ii )
			{
				CBrushBSPTree3D::SOutputRegions output;
				pBSPTreeForBrush->GetPartitions(*ii,output);
				if( !output.negList.empty() )
				{
					GetDesigner()->RemoveRegion(*ii);
					for( int i = 0, iPosRegionCount(output.posList.size()); i < iPosRegionCount; ++i )
						GetDesigner()->AddRegion(output.posList[i],CBrushDesigner::eOpType_Union);
				}
			}

			CBrushDesigner brushDesigner(brushRegions);
			_smart_ptr<CBrushBSPTree3D> pBSPTree = new CBrushBSPTree3D(originalIntersectedRegions);
			for( int i = 0, iRegionCount(brushRegions.size()); i < iRegionCount; ++i )
			{
				CBrushBSPTree3D::SOutputRegions output;
				pBSPTree->GetPartitions(brushRegions[i],output);
				if( !output.negList.empty() )
				{	
					brushDesigner.RemoveRegion(brushRegions[i]);
					for( int k = 0, iPosRegionCount(output.posList.size()); k < iPosRegionCount; ++k )
						brushDesigner.AddRegion(output.posList[k],CBrushDesigner::eOpType_Union);
				}
			}
			brushRegions.clear();
			brushDesigner.GetRegionList(brushRegions);
		}
	}	

	for( int i = 0, iRegionCount(brushRegions.size()); i < iRegionCount; ++i )
	{
		if( brushRegions[i] == NULL )
			continue;
		const BrushPlane& plane = brushRegions[i]->GetPlane();
		std::vector<CBrushRegion::RegionPtr> candidateRegions;
		GetDesigner()->QueryRegions(plane,candidateRegions);
		if( !candidateRegions.empty() )
		{
			CBrushRegion::RegionPtr pRegion = brushRegions[i];
			std::vector<CBrushRegion::RegionPtr> touchedRegions;
			GetDesigner()->QueryIntersectionByRegion(pRegion,touchedRegions);
			if( !touchedRegions.empty() )
			{
				if( s_pCubeEditorPanel->IsSidesMerged() )
					GetDesigner()->AddRegion( pRegion, CBrushDesigner::eOpType_Union );
				else
					GetDesigner()->AddRegion( pRegion, CBrushDesigner::eOpType_Split );
				continue;
			}
		}

		BrushPlane invPlane = plane.GetInverted();
		candidateRegions.clear();
		GetDesigner()->QueryRegions(invPlane,candidateRegions);
		if( !candidateRegions.empty() )
		{
			CBrushRegion::RegionPtr pInvertedRegion = brushRegions[i]->Clone()->Flip();
			std::vector<CBrushRegion::RegionPtr> touchedRegions;
			GetDesigner()->QueryIntersectionByRegion(pInvertedRegion,touchedRegions);
			if( !touchedRegions.empty() )
			{
				for( int k = 0, iTouchedRegionCount(touchedRegions.size()); k < iTouchedRegionCount; ++k )
				{
					if( enlargedBrushAABB.ContainsBox(touchedRegions[k]->GetBoundBox()) )
					{						
						GetDesigner()->RemoveRegion(touchedRegions[k]);
						std::vector<CBrushRegion::RegionPtr> outsideRegions;
						if( touchedRegions[k]->GetSeparatedRegions(outsideRegions,CBrushRegion::eSR_OuterHull) )
						{
							for( int a = 0, iOutsideRegionCount(outsideRegions.size()); a < iOutsideRegionCount; ++a )
								pInvertedRegion->Subtract(outsideRegions[a]);
						}
					}
					else if( CBrushRegion::HasIntersection(touchedRegions[k], pInvertedRegion) == BUtil::eIT_Intersection )
					{
						std::vector<CBrushRegion::RegionPtr> outsideRegions;
						touchedRegions[k]->GetSeparatedRegions(outsideRegions,CBrushRegion::eSR_OuterHull);

						touchedRegions[k]->Subtract(pInvertedRegion);

						for( int a = 0, iOutsideRegionCount(outsideRegions.size()); a < iOutsideRegionCount; ++a )
							pInvertedRegion->Subtract(outsideRegions[a]);

						if( touchedRegions[k]->IsValid() )
							GetDesigner()->AddRegionSeparately(touchedRegions[k],true);
						else
							GetDesigner()->RemoveRegion(touchedRegions[k]);
					}					
				}
				if( pInvertedRegion->IsValid() && !pInvertedRegion->IsOpen() )
					GetDesigner()->AddRegionSeparately(pInvertedRegion->Flip());
				continue;
			}
		}

		if( s_pCubeEditorPanel->IsSidesMerged() )
			GetDesigner()->AddRegion(brushRegions[i],CBrushDesigner::eOpType_Union);
		else
			GetDesigner()->AddRegion(brushRegions[i],CBrushDesigner::eOpType_Add);
	}
}

void CBrushDesignerCubeEditor::RemoveCube( const AABB& brushAABB )
{
	std::vector<CBrushRegion::RegionPtr> brushRegions = GetBrushRegions(brushAABB);
	if( brushRegions.empty() )
		return;

	std::vector<CBrushRegion::RegionPtr> intersectedRegions;
	if( !GetDesigner()->QueryIntersectedRegionsByAABB(brushAABB,intersectedRegions) )
		return;

	if( intersectedRegions.empty() )
		return;

	std::vector<CBrushRegion::RegionPtr> brushRegionsForBSPTree;
	CBrushRegion::CopyRegions(brushRegions,brushRegionsForBSPTree);
	_smart_ptr<CBrushBSPTree3D> pBrushBSP3D = new CBrushBSPTree3D(brushRegionsForBSPTree);

	std::vector<CBrushRegion::RegionPtr> boundaryRegions;
	CBrushRegion::CopyRegions(intersectedRegions,boundaryRegions);

	std::vector<CBrushRegion::RegionPtr>::iterator ii = boundaryRegions.begin();
	for( ; ii != boundaryRegions.end(); )
	{
		bool bBoundaryRegion = true;
		for( int k = 0, iBrushRegionCount(brushRegions.size()); k < iBrushRegionCount; ++k )
		{
			if( brushRegions[k]->GetPlane().GetInverted().IsEquivalent((*ii)->GetPlane(),kDesignerEpsilon) )
			{				
				bBoundaryRegion = false;
				break;
			}
		}
		if( !bBoundaryRegion )
			ii = boundaryRegions.erase(ii);
		else
			++ii;
	}

	if( boundaryRegions.empty() )
		return;

	_smart_ptr<CBrushBSPTree3D> pBoundaryBSPTree = new CBrushBSPTree3D(boundaryRegions);

	CBrushDesigner designer;
	std::vector<CBrushRegion::RegionPtr>::iterator iBrush = brushRegions.begin();
	for( ;iBrush != brushRegions.end(); )
	{
		bool bSubtracted = false;
		std::vector<CBrushRegion::RegionPtr>::iterator ii = intersectedRegions.begin();
		for( ; ii != intersectedRegions.end(); )
		{
			if( !(*iBrush)->IsPlaneEquivalent(*ii) || CBrushRegion::HasIntersection(*iBrush,*ii) != BUtil::eIT_Intersection )
			{
				++ii;
				continue;
			}
			bSubtracted = true;
			CBrushRegion::RegionPtr pClone = (*ii)->Clone();
			(*ii)->Subtract(*iBrush);
			(*iBrush)->Subtract(pClone);
			GetDesigner()->RemoveRegion(*ii);
			if( !(*ii)->IsValid() )
			{	
				ii = intersectedRegions.erase(ii);
			}
			else
			{
				designer.AddRegion(*ii,CBrushDesigner::eOpType_Union);
				++ii;
			}
		}

		if( bSubtracted && !(*iBrush)->IsValid() )
			iBrush = brushRegions.erase(iBrush);
		else
			++iBrush;
	}

	for( std::vector<CBrushRegion::RegionPtr>::iterator ii = intersectedRegions.begin(); ii != intersectedRegions.end(); )
	{
		if( brushAABB.ContainsBox((*ii)->GetBoundBox()) )
		{
			bool bTouched = false;
			for( int i = 0, iBrushRegionCount(brushRegions.size()); i < iBrushRegionCount; ++i )
			{
				BrushPlane invertedBrushPlane = brushRegions[i]->GetPlane().GetInverted();
				if( invertedBrushPlane.IsEquivalent((*ii)->GetPlane(),kDesignerEpsilon) )
				{
					bTouched = true;
					break;
				}
			}
			if( !bTouched )
			{
				GetDesigner()->RemoveRegion(*ii);
				ii = intersectedRegions.erase(ii);
			}
			else
			{
				++ii;
			}
		}
		else if( brushAABB.IsIntersectBox((*ii)->GetBoundBox()) )
		{
			CBrushBSPTree3D::SOutputRegions output;
			pBrushBSP3D->GetPartitions(*ii,output);
			if( !output.negList.empty() )
			{
				GetDesigner()->RemoveRegion(*ii);
				for( int i = 0, iPosCount(output.posList.size());  i < iPosCount; ++i )
					designer.AddRegion(output.posList[i],CBrushDesigner::eOpType_Union);
				ii = intersectedRegions.erase(ii);
			}
			else
			{
				++ii;
			}
		}
		else
		{
			++ii;
		}
	}

	for( int i = 0, iBrushRegionCount(brushRegions.size()); i < iBrushRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pInvertedRegion = brushRegions[i]->Clone()->Flip();

		CBrushBSPTree3D::SOutputRegions output;
		pBoundaryBSPTree->GetPartitions(pInvertedRegion,output);
		if( output.negList.empty() )
			continue;

		if( !output.posList.empty() || !output.coSameList.empty() || !output.coDiffList.empty() )
		{
			for( int k = 0, negListCount(output.negList.size()); k < negListCount; ++k )
				designer.AddRegion(output.negList[k],CBrushDesigner::eOpType_Union);
		}
		else
		{
			designer.AddRegion(pInvertedRegion,CBrushDesigner::eOpType_Union);
		}
	}

	for( int i = 0, iRegionCount(designer.GetRegionSize()); i < iRegionCount; ++i )
	{
		if( s_pCubeEditorPanel->IsSidesMerged() || !pBoundaryBSPTree->IsValidTree() )
			GetDesigner()->AddRegion(designer.GetRegion(i),CBrushDesigner::eOpType_Union);
		else
			GetDesigner()->AddRegionSeparately(designer.GetRegion(i));
	}
}

void CBrushDesignerCubeEditor::PaintCube( const AABB& brushAABB )
{
	std::vector<CBrushRegion::RegionPtr> brushRegions = GetBrushRegions(brushAABB);
	if( brushRegions.empty() )
		return;

	std::vector<CBrushRegion::RegionPtr> candidateRegions;
	GetDesigner()->QueryIntersectedRegionsByAABB(brushAABB,candidateRegions);
	if( candidateRegions.empty() )
		return;

	std::vector<CBrushRegion::RegionPtr>::iterator ii = candidateRegions.begin();
	for( ; ii != candidateRegions.end();  )
	{
		if( brushAABB.ContainsBox((*ii)->GetBoundBox()) )
		{
			(*ii)->SetMaterialID(s_pCubeEditorPanel->GetSubMatID());
			ii = candidateRegions.erase(ii);
		}
		else
		{
			++ii;
		}
	}

	if( candidateRegions.empty() )
		return;

	_smart_ptr<CBrushBSPTree3D> pBrushTree = new CBrushBSPTree3D(brushRegions);

	CBrushDesigner designer[2];
	for( int i = 0, iCandidateRegionCount(candidateRegions.size()); i < iCandidateRegionCount; ++i )
	{
		CBrushBSPTree3D::SOutputRegions output;
		pBrushTree->GetPartitions(candidateRegions[i],output);

		std::vector<CBrushRegion::RegionPtr> regionsInside;
		regionsInside.insert(regionsInside.end(),output.negList.begin(),output.negList.end());
		regionsInside.insert(regionsInside.end(),output.coSameList.begin(),output.coSameList.end());
		regionsInside.insert(regionsInside.end(),output.coDiffList.begin(),output.coDiffList.end());

		if( regionsInside.empty() )
			continue;

		GetDesigner()->RemoveRegion(candidateRegions[i]);
		for( int k = 0, iPosRegionCount(output.posList.size()); k < iPosRegionCount; ++k )
			designer[0].AddRegion(output.posList[k],CBrushDesigner::eOpType_Union);

		for( int k = 0, iRegionCountInside(regionsInside.size()); k < iRegionCountInside; ++k )
		{
			regionsInside[k]->SetMaterialID(s_pCubeEditorPanel->GetSubMatID());
			designer[1].AddRegion(regionsInside[k],CBrushDesigner::eOpType_Union);
		}
	}

	for( int i = 0; i < 2; ++i )
	{
		for( int k = 0, iRegionCount(designer[i].GetRegionSize()); k < iRegionCount; ++k )
			GetDesigner()->AddRegionSeparately(designer[i].GetRegion(k));
	}
}

CBrushDesignerCubeEditor::EEditorMode CBrushDesignerCubeEditor::GetEditMode() const
{
	if( s_pCubeEditorPanel->IsAddButtonChecked() )
		return eEditorMode_Add;
	else if( s_pCubeEditorPanel->IsRemoveButtonChecked() )
		return eEditorMode_Remove;
	else if( s_pCubeEditorPanel->IsPaintButtonChecked() )
		return eEditorMode_Paint;

	return eEditorMode_Invalid;
}

void CBrushDesignerCubeEditor::MaterialChanged()
{
	s_pCubeEditorPanel->UpdateSubMaterialComboBox();
}

void CBrushDesignerCubeEditor::SetSubMatID( int nSubMatID, CBrushDesigner* pDesigner )
{
	s_pCubeEditorPanel->SetSubMatID(nSubMatID+1);
}

void CBrushDesignerCubeEditor::AddBrush( const AABB& aabb )
{
	if( aabb.IsReset() )
		return;
	bool bHaveSame = false;
	for( int i = 0, iCount(m_BrushAABBs.size()); i < iCount; ++i )
	{
		if( m_BrushAABBs[i].min.IsEquivalent(aabb.min,(float)kDesignerEpsilon) && m_BrushAABBs[i].max.IsEquivalent(aabb.max,(float)kDesignerEpsilon) )
		{
			bHaveSame = true;
			break;
		}
	}
	if( !bHaveSame )
		m_BrushAABBs.push_back(aabb);
}

void CBrushDesignerCubeEditor::OnDataBaseItemEvent( IDataBaseItem *pItem,EDataBaseItemEvent event )
{
	if( pItem == NULL )
		return;

	if( event == EDB_ITEM_EVENT_SELECTED )
	{	
		s_pCubeEditorPanel->SetMaterial((CMaterial*)pItem);		
	}
}
