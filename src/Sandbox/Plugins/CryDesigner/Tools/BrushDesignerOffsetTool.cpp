#include "StdAfx.h"
#include "BrushDesignerOffsetTool.h"
#include "Viewport.h"
#include "ViewManager.h"
#include "Core/BrushDesigner.h"

BrushFloat CBrushDesignerOffsetTool::m_fPrevScale = 0;

void CBrushDesignerOffsetTool::Leave()
{
	m_pOffsetedRegion = NULL;
	m_pSelectedRegion = NULL;
	__super::Leave();
}

void CBrushDesignerOffsetTool::OnLButtonDown( CViewport *view, UINT nFlags, CPoint point )
{
	m_fScale = 0;
	m_LButtonInfo.m_DownTimeStamp = GetTickCount();
	m_LButtonInfo.m_DownPos = point;
	m_LButtonInfo.m_LastAction = eMouseAction_LButtonDown;
	m_pSelectedRegion = QueryOffsetRegion(view,point);
	if( m_pSelectedRegion )
	{
		m_pOffsetedRegion = m_pSelectedRegion->Clone();
		m_pOffsetedRegion->RemoveInside();
		UpdateSelectionMesh(NULL,GetBrush(),GetBaseObject());
		m_PrevPos = point;
	}	
}

CBrushRegion::RegionPtr CBrushDesignerOffsetTool::QueryOffsetRegion( CViewport *view, CPoint point ) const
{
	BrushVec3 localRaySrc, localRayDir;
	BUtil::GetLocalViewRay( GetWorldTM(), view, point, localRaySrc, localRayDir );
	int nRegionIndex = 0;
	if( GetDesigner()->QueryRegion( localRaySrc, localRayDir, nRegionIndex ) )
	{
		CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(nRegionIndex);
		if( !pRegion->IsOpen() )
			return pRegion;
	}
	return NULL;
}

void CBrushDesignerOffsetTool::OnLButtonUp( CViewport *view,UINT nFlags,CPoint point )
{
	m_LButtonInfo.m_LastAction = eMouseAction_LButtonUp;

	if( !IsOverDoubleClickTime(m_LButtonInfo.m_DownTimeStamp) && IsTwoPointEquivalent(point,m_LButtonInfo.m_DownPos) )
	{
		m_pOffsetedRegion = NULL;
	}
	else if( m_pOffsetedRegion )
	{
		AddScaledRegion();
		m_fPrevScale = m_fScale;
	}
}

void CBrushDesignerOffsetTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_pOffsetedRegion == NULL || !(nFlags&MK_LBUTTON) )
	{
		BrushVec3 localRaySrc, localRayDir;
		BUtil::GetLocalViewRay( GetWorldTM(), view, point, localRaySrc, localRayDir );
		int nRegionIndex(-1);
		if( GetDesigner()->QueryRegion( localRaySrc, localRayDir, nRegionIndex ) )
		{
			CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(nRegionIndex);
			if( !pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) && !pRegion->IsOpen() )
			{
				UpdateSelectionMesh(pRegion,GetBrush(),GetBaseObject());
				SetPlane(pRegion->GetPlane());
			}
			else
			{
				UpdateSelectionMesh(NULL,GetBrush(),GetBaseObject());
			}
		}
		else
		{
			UpdateSelectionMesh(NULL,GetBrush(),GetBaseObject());
		}
	}
	else if( IsOverDoubleClickTime(m_LButtonInfo.m_DownTimeStamp) || !IsTwoPointEquivalent(point,m_LButtonInfo.m_DownPos) )
	{
		BrushFloat fScale((BrushFloat)(m_PrevPos.y-point.y)/100.0f);
		if( GetIEditor()->GetViewManager()->GetGrid()->IsEnabled() )
			fScale = BUtil::Snap(fScale);
		if( std::abs(fScale) >= kDesignerEpsilon )
		{
			m_pOffsetedRegion = m_pSelectedRegion->Clone();
			m_pOffsetedRegion->RemoveInside();
			m_fScale += fScale;
			m_fScale = ApplyScaleToSelectedRegion(m_pOffsetedRegion,m_fScale);
			m_PrevPos = point;
		}
	}
}

void CBrushDesignerOffsetTool::ApplyOffset( CBrushDesigner* pDesigner, CBrushRegion::RegionPtr pScaledRegion, CBrushRegion::RegionPtr pOriginalRegion, bool bCreateBridgeEdges )
{
	if( pScaledRegion->IncludeAllEdges(pOriginalRegion) )
	{	
		for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
		{
			CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);

			if( pScaledRegion->GetPlane().IsEquivalent(pRegion->GetPlane(),kDesignerEpsilon) )
			{
				if( pScaledRegion->IncludeAllEdges(pRegion) )
				{
					std::vector<CBrushRegion::RegionPtr> outsideRegions;
					pRegion->GetSeparatedRegions(outsideRegions,CBrushRegion::eSR_OuterHull);
					if( outsideRegions.size() == 1 )
						pScaledRegion->Subtract(outsideRegions[0]);
				}
			}
		}
		if( pScaledRegion->IsValid() && !pScaledRegion->IsOpen() )
		{
			pDesigner->AddRegion(pScaledRegion->Clone(),CBrushDesigner::eOpType_Add);
			pScaledRegion->Flip();
			pDesigner->AddRegion(pScaledRegion,CBrushDesigner::eOpType_Add);
		}
	}
	else if( pOriginalRegion->IncludeAllEdges(pScaledRegion) )
	{
		pDesigner->AddRegion(pScaledRegion,CBrushDesigner::eOpType_Split);
	}

	if( bCreateBridgeEdges )
	{
		std::map<int,BrushLine> correspondingLineMap;
		CBrushRegion::RegionPtr pResizedRegion = pOriginalRegion->Clone();
		pResizedRegion->RemoveInside();
		pResizedRegion->Scale((BrushFloat)0.01);

		for( int i = 0, iCount(pResizedRegion->GetVertexListSize()); i < iCount; ++i )
		{
			int nCorrespondingIndex = 0;
			if( !pOriginalRegion->GetNearestVertexIndex(pResizedRegion->GetVertex(i),nCorrespondingIndex) )
			{
				DESIGNER_ASSERT(0);
				correspondingLineMap.clear();
				break;
			}
			BrushVec2 vFromSource = pOriginalRegion->GetPlane().W2P(pOriginalRegion->GetVertex(nCorrespondingIndex));
			BrushVec2 vToResized = pOriginalRegion->GetPlane().W2P(pResizedRegion->GetVertex(i));
			correspondingLineMap[nCorrespondingIndex] = BrushLine(vFromSource,vToResized);
		}		

		for( int i = 0, iVertexCount(pScaledRegion->GetVertexListSize()); i < iVertexCount; ++i )
		{
			BrushVec3 vInOffsetRegion = pScaledRegion->GetVertex(i);

			std::map<int,BrushLine>::iterator ii = correspondingLineMap.begin();
			std::map<BrushFloat,BrushVec3> candidatedVertices;

			for( ; ii != correspondingLineMap.end(); ++ii )
			{
				int nVertexIndexInSelectedRegion = ii->first;
				const BrushLine& correspondingLine = ii->second;

				if( std::abs(correspondingLine.Distance(pOriginalRegion->GetPlane().W2P(vInOffsetRegion))) < kDesignerLooseEpsilon )
				{
					BrushVec3 vInSelectedRegion = pOriginalRegion->GetVertex(nVertexIndexInSelectedRegion);
					candidatedVertices[vInSelectedRegion.GetDistance(vInOffsetRegion)] = vInSelectedRegion;					
				}
			}

			if( !candidatedVertices.empty() )
			{
				std::vector<BrushVec3> vList;
				vList.push_back(candidatedVertices.begin()->second); 
				vList.push_back(vInOffsetRegion);
				CBrushRegion::RegionPtr pRegion = new CBrushRegion(vList,pOriginalRegion->GetPlane(),pOriginalRegion->GetMaterialID(),&(pOriginalRegion->GetTexInfo()),false);
				if( pRegion->IsValid() )
					pDesigner->AddOpenRegion(pRegion,false);
			}
		}
	}
}

void CBrushDesignerOffsetTool::AddScaledRegion()
{
	CUndo undo("Designer : Offset");
	GetDesigner()->RecordUndo("Offset",GetBaseObject());
	ApplyOffset( GetDesigner(), m_pOffsetedRegion, m_pSelectedRegion, CheckVirtualKey(VK_CONTROL) );
	UpdateMirroredPartWithPlane(GetDesigner(),m_pOffsetedRegion->GetPlane());
	m_pOffsetedRegion = NULL;
	GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
	UpdateBrush();
	Sync();
	UpdateGameResource(GetBaseObject());
}

void CBrushDesignerOffsetTool::OnLButtonDblClk( CViewport *view,UINT nFlags,CPoint point )
{
	m_LButtonInfo.m_LastAction = eMouseAction_LButtonDoubleClick;

	if( !IsTwoPointEquivalent(point,m_LButtonInfo.m_DownPos) )
		return;
	m_pOffsetedRegion = QueryOffsetRegion(view,point);
	if( !m_pOffsetedRegion || fabs(m_fPrevScale) < kDesignerEpsilon )
		return;
	m_pOffsetedRegion = m_pOffsetedRegion->Clone();
	ApplyScaleToSelectedRegion(m_pOffsetedRegion,m_fPrevScale);
	AddScaledRegion();
}

BrushFloat CBrushDesignerOffsetTool::ApplyScaleToSelectedRegion( CBrushRegion::RegionPtr pRegion, BrushFloat fScale )
{
	if( !pRegion )
		return fScale;	

	if( !pRegion->Scale(fScale,true) )
	{
		BrushFloat fChangedScale = fScale;
		if( BinarySearchForScale(0, fScale, 0, *pRegion, fChangedScale) )
		{
			if( pRegion->Scale(fChangedScale,true) )
				return fChangedScale;
		}
	}

	return fScale;
}

void CBrushDesignerOffsetTool::Display( DisplayContext &dc )
{
	__super::Display(dc);
	dc.SetFillMode(e_FillModeSolid);
	if( m_pOffsetedRegion )
	{
		int oldThickness = dc.GetLineWidth();
		dc.SetLineWidth(BUtil::kLineThickness);
		dc.SetColor(BUtil::kResizedRegionColor);
		m_pOffsetedRegion->Display(dc);
		dc.SetLineWidth(oldThickness);
	}
}