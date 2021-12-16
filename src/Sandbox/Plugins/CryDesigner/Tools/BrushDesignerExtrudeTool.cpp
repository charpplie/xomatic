#include "StdAfx.h"
#include "BrushDesignerExtrudeTool.h"
#include "ViewManager.h"
#include "Core/BrushDesigner.h"
#include "Controls/PropertyCtrl.h"
#include "Core/BrushDesignerAdjustHeightHelper.h"
#include "Core/BrushDesignerExtrudeSnappingHelper.h"
#include "Core/BrushDesignerGlobalSettings.h"
#include "BrushDesignerEditTool.h"

void CBrushDesignerExtrudeTool::Enter()
{
	__super::Enter();
	m_ec.pushPull = BUtil::ePP_None;
	m_LButtonInfo = SLButtonInfo();
	GetIEditor()->ShowTransformManipulator(false);
}

void CBrushDesignerExtrudeTool::Leave()
{	
	m_pScaledRegion->Init();
	__super::Leave();
}

bool CBrushDesignerExtrudeTool::StartPushPull(CViewport *view,UINT nFlags,CPoint point)
{
	m_ec.pArgumentBrush = NULL;
	m_ec.pushPull = m_ec.initPushPull = BUtil::ePP_None;

	if( GetPickedRegion() )
	{
		GetIEditor()->BeginUndo();
		GetDesigner()->RecordUndo("Designer : Extrusion",GetBaseObject());

		BrushVec3 localRaySrc;
		BrushVec3 localRayDir;
		BUtil::GetLocalViewRay( GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir );
		BrushVec3 vPivot;
		if( GetDesigner()->QueryPosition( localRaySrc, localRayDir, vPivot ) )
			s_AdjustHeightHelper.Init( GetPlane(), vPivot );
		else
			s_AdjustHeightHelper.Init( GetPlane(), GetPickedRegion()->GetCenterPosition() );

		m_ec.pBrush = GetBrush();
		m_ec.pObject = GetBaseObject();
		m_ec.pDesigner = GetDesigner();
		m_ec.pRegion = GetPickedRegion();

		return PrepareExtrusion(m_ec);
	}
	return false;
}

bool CBrushDesignerExtrudeTool::PrepareExtrusion( SExtrusionContext& ec )
{
	DESIGNER_ASSERT(ec.pRegion);
	if( !ec.pRegion )
		return false;

	ec.backupRegions.clear();
	ec.pDesigner->QueryPerpendicularRegions(ec.pRegion,ec.backupRegions);
	RemoveRegionWithSpecificFlagsFromList(ec.pDesigner,ec.backupRegions,CBrushRegion::eRF_Mirrored);
	if( ec.pDesigner->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
	{
		ec.mirroredBackupRegions.clear();
		ec.pDesigner->QueryPerpendicularRegions(ec.pRegion->Clone()->Mirror(ec.pDesigner->GetMirrorPlane()),ec.mirroredBackupRegions);
		RemoveRegionWithoutSpecificFlagsFromList(ec.pDesigner,ec.mirroredBackupRegions,CBrushRegion::eRF_Mirrored);
	}

	MakeArgumentBrush(ec);
	if( ec.pArgumentBrush )
	{
		s_SnappingHelper.SearchForOppositeRegions(ec.pArgumentBrush->GetCapRegion());
		DESIGNER_SHELF_RECONSTRUCTOR(ec.pDesigner);
		ec.pDesigner->SetShelf(0);
		ec.pDesigner->DrillRegion(ec.pRegion);
		DrillMirroredRegion(ec.pDesigner,ec.pRegion);
		ec.bFirstUpdate = true;
		return true;
	}

	return false;
}

void CBrushDesignerExtrudeTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	if( !IsOverDoubleClickTime(m_LButtonInfo.m_DownTimeStamp) )
		return;

	s_SnappingHelper.Init(GetDesigner());
	m_ec.pArgumentBrush = NULL;
	m_LButtonInfo.m_LastAction = eMouseAction_LButtonDown;
	m_LButtonInfo.m_DownTimeStamp = GetTickCount();
	m_LButtonInfo.m_DownPos = point;
	m_ec.bTouchedMirrorPlane = false;
}

CBrushDesignerExtrudeTool::~CBrushDesignerExtrudeTool()
{
}

void CBrushDesignerExtrudeTool::RaiseHeight( const CPoint& point, CViewport* view, int nFlags )
{
	BrushFloat fHeight = s_AdjustHeightHelper.UpdateHeight( GetWorldTM(), view, point );

	m_ec.pArgumentBrush->SetHeight(fHeight);
	m_ec.pArgumentBrush->Update(CBrushArgument::eBAU_None);
	m_ec.bTouchedMirrorPlane = false;

	if( !GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		return;

	CBrushRegion::RegionPtr pCapRegion = m_ec.pArgumentBrush->GetCapRegion();
	for( int i = 0, iVertexCount(pCapRegion->GetVertexListSize()); i < iVertexCount; ++i )
	{
		const BrushVec3& v = pCapRegion->GetVertex(i);
		BrushFloat distFromVertexToMirrorPlane = GetDesigner()->GetMirrorPlane().Distance(v);
		if( distFromVertexToMirrorPlane <= -kDesignerEpsilon )
			continue;
		BrushFloat distFromVertexMirrorPlaneInCapNormalDir = 0;
		GetDesigner()->GetMirrorPlane().HitTest(v,v+pCapRegion->GetPlane().Normal(),kDesignerEpsilon,&distFromVertexMirrorPlaneInCapNormalDir);
		m_ec.pArgumentBrush->SetHeight(fHeight+distFromVertexMirrorPlaneInCapNormalDir);
		m_ec.pArgumentBrush->Update(CBrushArgument::eBAU_None);
		m_ec.bTouchedMirrorPlane = true;
		break;
	}
}

void CBrushDesignerExtrudeTool::RaiseLowerRegion( CViewport *view, UINT nFlags, CPoint point )
{
	if( !(nFlags&MK_LBUTTON) )
		return;

	BrushFloat fPrevHeight = m_ec.pArgumentBrush->GetHeight();

	// When the mouse cursor is in the selected region, push/pull should be done.
	// And the cursor is outside the selected region and it is located on another region, the height of the selected region should be same as the height of the another region.
	BrushVec3 localRaySrc;
	BrushVec3 localRayDir;
	BUtil::GetLocalViewRay( GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir );
	BrushFloat outT(0);
	if( !(nFlags&MK_SHIFT) || m_ec.pArgumentBrush->GetCapRegion()->IsPassed(localRaySrc,localRayDir,outT) )
	{
		RaiseHeight( point, view, nFlags );
	}
	else
	{
		if( !AlignHeight(m_ec.pArgumentBrush->GetCapRegion(),view,point) )
			RaiseHeight( point, view, nFlags );
	}

	BrushFloat fHeight = m_ec.pArgumentBrush->GetHeight();
	BrushFloat fHeightDifference(fHeight-fPrevHeight);
	if( fHeightDifference > 0 )
		m_ec.pushPull = BUtil::ePP_Pull;
	else if( fHeightDifference < 0 )
		m_ec.pushPull = BUtil::ePP_Push;

	if( m_ec.initPushPull == BUtil::ePP_None )
		m_ec.initPushPull = m_ec.pushPull;

	CheckBoundary(m_ec);
	UpdateDesigner(m_ec);
}

void CBrushDesignerExtrudeTool::ResizeRegion( CViewport *view, UINT nFlags, CPoint point )
{
	if( !GetPickedRegion() )
		return;

	if( m_ResizeStatus == eRS_None )
	{
		m_ResizeStartScreenPos = BrushVec2(point.x,point.y);
		m_ResizeStatus = eRS_Resizing;
		m_ec.fScale = 0;
	}

	BrushVec3 localRaySrc,localRayDir;
	BUtil::GetLocalViewRay( GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir );

	BrushVec3 crossPoint;
	if( !GetPickedRegion()->GetPlane().HitTest(localRaySrc, localRaySrc+localRayDir, kDesignerEpsilon, NULL, &crossPoint) )
		return;

	BrushFloat fPrevDifference(m_ec.fScale);

	m_ec.fScale = (m_ResizeStartScreenPos.y-(BrushFloat)point.y)*0.01f;
	if( std::abs(m_ec.fScale) < 0.1f )
		m_ec.fScale = 0;

	CBrushRegion region = *GetPickedRegion();
	if( region.Scale(m_ec.fScale,true) )
	{
		*m_pScaledRegion = region;
	}
	else
	{
		BrushFloat fAdjustedScale(m_ec.fScale);
		BinarySearchForScale( fPrevDifference, m_ec.fScale, 0, region, fAdjustedScale);
		if( region.Scale(fAdjustedScale,true) )
		{
			if( fAdjustedScale > 0 )
				m_ec.fScale = fAdjustedScale-kDesignerEpsilon*100.0f;
			else
				m_ec.fScale = fAdjustedScale+kDesignerEpsilon*100.0f;
			*m_pScaledRegion = region;
		}
		else
		{
			DESIGNER_ASSERT(0);
		}
	}
}

void CBrushDesignerExtrudeTool::SelectRegion( CViewport *view, UINT nFlags, CPoint point )
{
	if( GetBaseObject() == NULL )
		return;

	m_ResizeStatus = eRS_None;

	BrushVec3 localRaySrc, localRayDir;
	BUtil::GetLocalViewRay( GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir );

	int nRegionIndex(0);
	if( GetDesigner()->QueryRegion( localRaySrc, localRayDir, nRegionIndex ) )
	{
		CBrushRegion::RegionPtr pCandidateRegion = GetDesigner()->GetRegion(nRegionIndex);
		if( pCandidateRegion != GetPickedRegion() && !pCandidateRegion->CheckFlags(CBrushRegion::eRF_Mirrored) && !pCandidateRegion->IsOpen() )
		{
			SetPickedRegion(pCandidateRegion);
			SetPlane(pCandidateRegion->GetPlane());
			*m_pScaledRegion = *GetPickedRegion();
			m_ec.fScale = 0;
			UpdateSelectionMesh(GetPickedRegion(),GetBrush(),GetBaseObject());
		}
	}
	else
	{
		SetPickedRegion(NULL);
		UpdateSelectionMesh(NULL,GetBrush(),GetBaseObject());
	}
}

void CBrushDesignerExtrudeTool::OnMouseMove( CViewport *view, UINT nFlags, CPoint point )
{
	if( nFlags & MK_LBUTTON )
	{
		if( m_LButtonInfo.m_LastAction == eMouseAction_LButtonDown && (IsOverDoubleClickTime(m_LButtonInfo.m_DownTimeStamp) || !IsTwoPointEquivalent(m_LButtonInfo.m_DownPos,point)) )
		{
			if( m_ec.pArgumentBrush )
			{
				RaiseLowerRegion( view, nFlags, point );
			}
			else if( m_LButtonInfo.m_LastAction == eMouseAction_LButtonDown )
			{
				if( StartPushPull(view,nFlags,point) )
					UpdateSelectionMesh(m_pScaledRegion,GetBrush(),GetBaseObject());
			}
		}
	}
	else
	{
		if( nFlags & MK_SHIFT )
			ResizeRegion( view, nFlags, point );
		else
			SelectRegion( view, nFlags, point );
	}
}

void CBrushDesignerExtrudeTool::Display( DisplayContext &dc )
{
	__super::Display(dc);

	if( m_ResizeStatus == eRS_Resizing )
	{
		dc.SetColor(BUtil::kResizedRegionColor);
		for( int i = 0, iEdgeSize(m_pScaledRegion->GetEdgeSize()); i < iEdgeSize; ++i )
		{
			BrushEdge3D edge = m_pScaledRegion->GetEdge(i);
			dc.DrawLine( ToVec3(edge.m_v[0]), ToVec3(edge.m_v[1]) );
		}
	}

	if( m_ec.pArgumentBrush )
	{
		SDesignerEnvironmentInfo& globalInfo = CBrushDesignerEditTool::GetGlobalEnvironmentInfo();
		if( gSettings.bDesignerDisplayDimensionHelper )
		{
			Matrix34 poppedTM = dc.GetMatrix();
			dc.PopMatrix();
			GetBaseObject()->DrawDimensionsImpl(dc, m_ec.pArgumentBrush->GetBoundBox());
			dc.PushMatrix(poppedTM);
		}
	}

	s_AdjustHeightHelper.Display(dc);
}

void CBrushDesignerExtrudeTool::CheckBoundary( SExtrusionContext& ec )
{
	if( ec.pushPull == BUtil::ePP_None || ec.pArgumentBrush == NULL )
		return;

	ec.bIsLocatedAtOpposite = false;

	if( s_SnappingHelper.IsOverOppositeRegion(ec.pArgumentBrush->GetCapRegion(),ec.pushPull) )
	{
		ec.pArgumentBrush->SetHeight(s_SnappingHelper.GetNearestDistanceToOpposite(ec.pushPull));
		ec.pArgumentBrush->Update(CBrushArgument::eBAU_None);
		ec.bIsLocatedAtOpposite = true;
	}
}

void CBrushDesignerExtrudeTool::MakeArgumentBrush( SExtrusionContext& ec )
{
	if( ec.pRegion == NULL )
		return;

	ec.pArgumentBrush = new CBrushArgumentEx( ec.pRegion, ec.fScale, ec.pObject, &ec.backupRegions, ec.pDesigner->GetDB() );
	ec.pArgumentBrush->SetHeight(0);
	ec.pArgumentBrush->Update(CBrushArgument::eBAU_None);
}

void CBrushDesignerExtrudeTool::FinishPushPull( SExtrusionContext& ec )
{
	if( !ec.pArgumentBrush )
		return;

	if( !ec.bIsLocatedAtOpposite )
		ec.pDesigner->MoveShelf(1,0);
	else
	{
		s_SnappingHelper.ApplyOppositeRegions(ec.pArgumentBrush->GetCapRegion(),ec.pushPull);
		ec.pDesigner->MoveShelf(1,0);
		UpdateSelectionMesh(NULL,ec.pBrush,ec.pObject,true);
		UpdateMirroredPartWithPlane( ec.pDesigner, ec.pArgumentBrush->GetCapRegion()->GetPlane() );
		UpdateMirroredPartWithPlane( ec.pDesigner, ec.pArgumentBrush->GetCapRegion()->GetPlane().GetInverted() );
		ec.bIsLocatedAtOpposite = false;
	}
	ec.pDesigner->Optimize();
	if( gSettings.bDesignerKeepCenterPivot )
		ec.pBrush->PivotToCenter(ec.pObject,ec.pDesigner);

	if( ec.bUpdateBrush )
	{
		ec.pDesigner->ResetDB(BUtil::eDBRF_ALL);
		ec.pBrush->Update(ec.pObject,ec.pDesigner);
	}

	UpdateGameResource(ec.pObject);

	ec.pArgumentBrush = NULL;
}

bool CBrushDesignerExtrudeTool::AlignHeight( CBrushRegion::RegionPtr pCapRegion, CViewport *view, const CPoint& point )
{
	CBrushRegion::RegionPtr pAlignedRegion = s_SnappingHelper.FindAlignedRegion(pCapRegion,GetWorldTM(),view,point);
	if( !pAlignedRegion )
		return false;

	BrushFloat updatedHeight = m_ec.pArgumentBrush->GetBasePlane().Distance() - pAlignedRegion->GetPlane().Distance();
	if( updatedHeight != m_ec.pArgumentBrush->GetHeight() )
	{
		m_ec.pArgumentBrush->SetHeight(updatedHeight);
		m_ec.pArgumentBrush->Update(CBrushArgument::eBAU_None);
		m_ec.pArgumentBrush->GetCapRegion()->UpdatePlane(m_ec.pArgumentBrush->GetCurrentCapPlane());
	}

	return true;
}

void CBrushDesignerExtrudeTool::OnLButtonUp( CViewport *view, UINT nFlags, CPoint point )
{
	if( m_LButtonInfo.m_LastAction == eMouseAction_LButtonDoubleClick )
		return;

	m_LButtonInfo.m_LastAction = eMouseAction_LButtonUp;

	if( !IsOverDoubleClickTime(m_LButtonInfo.m_DownTimeStamp) && IsTwoPointEquivalent(m_LButtonInfo.m_DownPos,point) )
		return;

	if( m_ec.pArgumentBrush )
	{
		if( m_ec.pushPull != BUtil::ePP_None )
		{
			m_PrevAction.m_Type = m_ec.pushPull;
			m_PrevAction.m_Distance = m_ec.pArgumentBrush ? m_ec.pArgumentBrush->GetHeight() : 0;
		}

		CheckBoundary(m_ec);
		if( m_ec.pArgumentBrush )
		{
			FinishPushPull(m_ec);
			GetIEditor()->AcceptUndo("Designer : Extrude");
		}
		else
		{
			GetIEditor()->CancelUndo();
		}
		Sync();
	}
}

void CBrushDesignerExtrudeTool::OnLButtonDblClk( CViewport *view,UINT nFlags,CPoint point )
{
	m_LButtonInfo.m_LastAction = eMouseAction_LButtonDoubleClick;
	if( m_ec.pArgumentBrush )
	{
		m_ec.pArgumentBrush = NULL;
		return;
	}

	if( !StartPushPull(view,nFlags,point) )
		return;

	if( std::abs(m_PrevAction.m_Distance) > kDesignerEpsilon && m_PrevAction.m_Type != BUtil::ePP_None )
	{
		m_ec.pushPull = m_PrevAction.m_Type;
		m_ec.pArgumentBrush->SetHeight(m_PrevAction.m_Distance);
		m_ec.pArgumentBrush->Update(CBrushArgument::eBAU_None);
	}

	m_ec.pArgumentBrush->Update(CBrushArgument::eBAU_None);
	CheckBoundary(m_ec);
	UpdateDesigner(m_ec);
	if( m_ec.pArgumentBrush )
	{
		FinishPushPull(m_ec);
		GetIEditor()->AcceptUndo("Designer : Extrude");
	}
	else
	{
		GetIEditor()->CancelUndo();
	}
	Sync();

	m_ec.pArgumentBrush = NULL;
}

void CBrushDesignerExtrudeTool::Extrude( BUtil::SMainContext& mc, CBrushRegion::RegionPtr pRegion, float fHeight, float fScale )
{
	SExtrusionContext ec;
	ec.pObject = mc.pObject;
	ec.pBrush = mc.pBrush;
	ec.pDesigner = mc.pDesigner;
	ec.pRegion = pRegion;
	ec.pushPull = ec.initPushPull = fHeight > 0 ? BUtil::ePP_Pull : BUtil::ePP_Push;
	ec.fScale = fScale;
	ec.bUpdateBrush = false;
	s_SnappingHelper.Init(ec.pDesigner);
	PrepareExtrusion(ec);
	ec.pArgumentBrush->SetHeight(fHeight);
	ec.pArgumentBrush->Update(CBrushArgument::eBAU_None);
	CheckBoundary(ec);
	UpdateDesigner(ec);
	FinishPushPull(ec);
}

void CBrushDesignerExtrudeTool::UpdateDesigner( SExtrusionContext& ec )
{
	DESIGNER_SHELF_RECONSTRUCTOR(ec.pDesigner);

	if( !ec.pArgumentBrush )
		return;

	if( ec.bFirstUpdate )
	{
		ec.pDesigner->SetShelf(0);
		for( int i = 0, iRegionCount(ec.backupRegions.size()); i < iRegionCount; ++i )
			ec.pDesigner->RemoveRegion(ec.backupRegions[i]);
		for( int i = 0, iRegionCount(ec.mirroredBackupRegions.size()); i < iRegionCount; ++i )
			ec.pDesigner->RemoveRegion(ec.mirroredBackupRegions[i]);
	}

	ec.pDesigner->SetShelf(1);
	ec.pDesigner->Clear();
	for( int i = 0, iRegionCount(ec.backupRegions.size()); i < iRegionCount; ++i )
		ec.pDesigner->AddRegion(ec.backupRegions[i]->Clone(),CBrushDesigner::eOpType_Add);

	const BrushPlane& mirrorPlane = ec.pDesigner->GetMirrorPlane();
	BrushPlane invertedMirrorPlane = mirrorPlane.GetInverted();

	std::vector<CBrushRegion::RegionPtr> sideRegions;
	if( ec.pArgumentBrush->GetSideRegionList(sideRegions) )
	{
		for( int i = 0, iSideRegionSize(sideRegions.size()); i < iSideRegionSize; ++i )
		{
			if( mirrorPlane.IsEquivalent(sideRegions[i]->GetPlane(),kDesignerEpsilon) || invertedMirrorPlane.IsEquivalent(sideRegions[i]->GetPlane(),kDesignerEpsilon) )
				continue;

			bool bOperated = false;
			CBrushRegion::RegionPtr pSideRegion = sideRegions[i];
			CBrushRegion::RegionPtr pSideFlipedRegion = sideRegions[i]->Clone()->Flip();

			std::set<int> removedBackUpRegions;

			for( int k = 0, iBackupRegionSize(ec.backupRegions.size()); k < iBackupRegionSize; ++k )
			{
				bool bIncluded = pSideRegion->IncludeAllEdges(ec.backupRegions[k]);
				bool bFlippedIncluded = pSideFlipedRegion->IncludeAllEdges(ec.backupRegions[k]);
				if( !bIncluded && !bFlippedIncluded )
					continue;
				ec.pDesigner->RemoveRegion(ec.pDesigner->QueryEquivalentRegion(ec.backupRegions[k]));
				if( bIncluded )
				{
					pSideRegion->Subtract(ec.backupRegions[k]);
					pSideFlipedRegion->Subtract(ec.backupRegions[k]->Clone()->Flip());
				}
				if( bFlippedIncluded )
				{
					pSideFlipedRegion->Subtract(ec.backupRegions[k]);
					pSideRegion->Subtract(ec.backupRegions[k]->Clone()->Flip());
				}
				removedBackUpRegions.insert(k);
			}

			if( !pSideRegion->IsValid() || !pSideFlipedRegion->IsValid() )
				continue;

			for( int k = 0, iBackupRegionSize(ec.backupRegions.size()); k < iBackupRegionSize; ++k )
			{
				if( removedBackUpRegions.find(k) != removedBackUpRegions.end() )
					continue;

				BUtil::EIntersectionType it = CBrushRegion::HasIntersection(ec.backupRegions[k],pSideRegion);
				BUtil::EIntersectionType itFliped = CBrushRegion::HasIntersection(ec.backupRegions[k],pSideFlipedRegion);

				if( it == BUtil::eIT_None && itFliped == BUtil::eIT_None )
					continue;

				CBrushRegion::RegionPtr pThisSideRegion = pSideRegion;
				if( itFliped == BUtil::eIT_Intersection || itFliped == BUtil::eIT_JustTouch && ec.pArgumentBrush->GetHeight() < 0 )
				{	
					it = itFliped;
					pThisSideRegion = pSideFlipedRegion;
				}
				
				if( it == BUtil::eIT_Intersection || it == BUtil::eIT_JustTouch && ec.backupRegions[k]->HasOverlappedEdges(pThisSideRegion) )
				{
					if( ec.pArgumentBrush->GetHeight() > 0 && CheckVirtualKey(VK_CONTROL) )
						ec.pDesigner->AddRegion( pThisSideRegion->Clone(), CBrushDesigner::eOpType_Add );
					else
						ec.pDesigner->AddRegion( pThisSideRegion, CBrushDesigner::eOpType_ExclusiveOR );
					bOperated = true;
					break;
				}
			}
			if( !bOperated )
			{
				if( ec.pArgumentBrush->GetHeight() < 0 )
				{
					CBrushRegion::RegionPtr pFlipRegion = sideRegions[i]->Clone()->Flip();
					ec.pDesigner->AddRegion(pFlipRegion, CBrushDesigner::eOpType_Add);
				}
				else
				{
					ec.pDesigner->AddRegion(sideRegions[i]->Clone(), CBrushDesigner::eOpType_Add);
				}
			}
		}
	}

	CBrushRegion::RegionPtr pCapRegion(ec.pArgumentBrush->GetCapRegion());
	if( pCapRegion )
	{
		if( !ec.bTouchedMirrorPlane || !ec.pDesigner->GetMirrorPlane().Normal().IsEquivalent(pCapRegion->GetPlane().Normal(),kDesignerEpsilon) )
			ec.pDesigner->AddRegion( pCapRegion->Clone(), CBrushDesigner::eOpType_Add );
	}

	if( GetSelectionMesh() )
	{
		Matrix34 worldTM = ec.pObject->GetWorldTM();
		worldTM.SetTranslation( worldTM.GetTranslation() + worldTM.TransformVector(ec.pRegion->GetPlane().Normal()*(ec.pArgumentBrush->GetHeight()+0.01f)) );
		GetSelectionMesh()->SetWorldTM(worldTM);
	}

	CreateMirroredRegions(ec.pDesigner);

	if( ec.bUpdateBrush )
	{
		if( ec.bFirstUpdate )
		{
			ec.pBrush->Update(ec.pObject,ec.pDesigner);
			ec.bFirstUpdate = false;
		}
		else
		{
			ec.pBrush->Update(ec.pObject,ec.pDesigner,1);
		}
	}
}

void CBrushDesignerExtrudeTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	__super::OnEditorNotifyEvent(event);
	switch(event)
	{
	case eNotify_OnEndUndoRedo:
		UpdateSelectionMesh(NULL,GetBrush(),GetBaseObject(),true);
		break;
	}
}