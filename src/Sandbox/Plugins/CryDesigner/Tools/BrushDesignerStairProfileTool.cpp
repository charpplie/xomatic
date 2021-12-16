#include "StdAfx.h"
#include "BrushDesignerStairProfileTool.h"
#include "Core/BrushDesigner.h"
#include "Viewport.h"
#include "BrushDesignerEditTool.h"
#include "IBaseToolPanel.h"

namespace
{
	ICreateStairProfileToolPanel* g_pStairProfilePanel = NULL;
}

void CBrushDesignerStairProfileTool::BeginEditParams()
{
	if( !g_pStairProfilePanel )
		g_pStairProfilePanel = CreateStairProfilePanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerStairProfileTool::EndEditParams()
{
	if( g_pStairProfilePanel )
	{
		g_pStairProfilePanel->DestroyPanel();
		g_pStairProfilePanel = NULL;
	}
}

CBrushDesignerStairProfileTool::CBrushDesignerStairProfileTool()
{
	m_SideStairMode = eSideStairMode_PlaceFirstPoint;
	m_nSelectedCandidate = 0;
}

void CBrushDesignerStairProfileTool::Enter()
{
	__super::Enter();
	m_SideStairMode = eSideStairMode_PlaceFirstPoint;
	m_bOnDesignerObject = false;
}

void CBrushDesignerStairProfileTool::Leave()
{
	__super::Leave();
}

CBrushDesignerStairProfileTool::~CBrushDesignerStairProfileTool()
{
}

void CBrushDesignerStairProfileTool::Display( DisplayContext &dc )
{
	if( m_SideStairMode == eSideStairMode_PlaceFirstPoint || m_SideStairMode == eSideStairMode_DrawDiagonal )
	{
		DrawCurrentSpot(dc,GetWorldTM());
		if( m_SideStairMode == eSideStairMode_DrawDiagonal )
		{
			dc.SetColor(BUtil::RegionLineColor);
			dc.DrawLine(GetCurrentSpotPos(), GetStartSpotPos());
		}
	}
	else if( m_SideStairMode == eSideStairMode_SelectDirection )
	{
		for( int i = 0; i < 2; ++i )
		{
			if( m_nSelectedCandidate == i )
			{
				dc.SetLineWidth(BUtil::kChosenLineThickness);
				DrawCandidateStair(dc,i,BUtil::RegionLineColor);
			}
			else
			{
				dc.SetLineWidth(BUtil::kLineThickness);
				DrawCandidateStair(dc,i,ColorB(100,100,100,255));
			}
		}
	}
}

void CBrushDesignerStairProfileTool::DrawCandidateStair( DisplayContext &dc, int nIndex, const ColorB& color )
{
	if( m_CandidateStairs[nIndex].empty() )
		return;

	dc.SetColor(color);

	int nSize = m_CandidateStairs[nIndex].size();
	if( nSize == 0 )
		return;
	std::vector<Vec3> vList;
	for( int i = 0; i < nSize; ++i )
		vList.push_back(m_CandidateStairs[nIndex][i].m_Pos);
	if( vList.size() >= 2 )
		dc.DrawPolyLine( &vList[0], vList.size(), false );
}

bool CBrushDesignerStairProfileTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if (nChar == VK_ESCAPE )
	{
		if( m_SideStairMode == eSideStairMode_SelectDirection )
		{
			SetCurrentSpot(GetStartSpot());
			m_SideStairMode = eSideStairMode_DrawDiagonal;
		}
		else if( m_SideStairMode == eSideStairMode_DrawDiagonal )
		{
			m_SideStairMode = eSideStairMode_PlaceFirstPoint;
			ResetAllSpots();
		}
		else if( m_SideStairMode == eSideStairMode_PlaceFirstPoint )
		{
			GetEditTool()->GoToSelectDesignerMode();
		}
	}

	return true;
}

void CBrushDesignerStairProfileTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_SideStairMode == eSideStairMode_PlaceFirstPoint )
	{
		if( m_bOnDesignerObject )
		{
			SetStartSpot(GetCurrentSpot());
			m_SideStairMode = eSideStairMode_DrawDiagonal;
		}
	}
	else if( m_SideStairMode == eSideStairMode_DrawDiagonal )
	{
		CreateCandidates();
		m_SideStairMode = eSideStairMode_SelectDirection;
	}
	else if( m_SideStairMode == eSideStairMode_SelectDirection )
	{
		CUndo undo("Designer : Add Stair");
		GetDesigner()->RecordUndo("Add Stair",GetBaseObject());

		std::vector<SSpot>& selectedStairs = m_CandidateStairs[m_nSelectedCandidate];
		int nStairSize = selectedStairs.size();

		DESIGNER_ASSERT( nStairSize >= 3 );

		if( nStairSize >= 3 )
		{
			if( !(nFlags&MK_SHIFT) )
			{
				BrushVec2 v0 = GetPlane().W2P(selectedStairs[0].m_Pos);
				BrushVec2 v1 = GetPlane().W2P(selectedStairs[1].m_Pos);
				BrushVec2 v2 = GetPlane().W2P(selectedStairs[2].m_Pos);

				BrushVec2 v3 = GetPlane().W2P(selectedStairs[nStairSize-3].m_Pos);
				BrushVec2 v4 = GetPlane().W2P(selectedStairs[nStairSize-2].m_Pos);
				BrushVec2 v5 = GetPlane().W2P(selectedStairs[nStairSize-1].m_Pos);

				BrushLine l0(v0, v0+(v1-v2));
				BrushLine l1(v5, v5+(v4-v3));

				BrushVec2 intersection;
				if( l0.Intersect(l1,intersection,kDesignerEpsilon) )
				{
					selectedStairs.push_back(SSpot(GetPlane().P2W(intersection)));
					std::vector<BrushVec3> vList;
					GenerateVertexListFromSpotList(selectedStairs,vList);
					CBrushRegion::RegionPtr pRegion = new CBrushRegion( vList, GetPlane(), GetMatID(), &GetTexInfo(), true );
					pRegion->ModifyOrientation();
					GetDesigner()->AddRegion( pRegion, CBrushDesigner::eOpType_Split );
				}
			}
			else
			{
				RegisterSpotList(GetDesigner(),selectedStairs);
			}
		}

		UpdateMirroredPartWithPlane(GetDesigner(),GetPlane());
		ResetAllSpots();
		UpdateBrush();
		m_SideStairMode = eSideStairMode_PlaceFirstPoint;
		Sync();
		UpdateGameResource(GetBaseObject());
	}
}

void CBrushDesignerStairProfileTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_SideStairMode == eSideStairMode_SelectDirection )
	{
		BrushVec3 localRaySrc, localRayDir;
		BUtil::GetLocalViewRay( GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir );

		BrushVec3 vPosition;
		if( !GetDesigner()->QueryPosition( GetPlane(), localRaySrc, localRayDir, vPosition ) )
			return;

		DESIGNER_ASSERT( !m_CandidateStairs[0].empty() );
		if( !m_CandidateStairs[0].empty() )
		{
			BrushFloat dist2Pos = m_BorderLine.Distance(GetPlane().W2P(vPosition));
			BrushFloat dist2Candidate0 = m_BorderLine.Distance(GetPlane().W2P(m_CandidateStairs[0][1].m_Pos));

			if( dist2Pos * dist2Candidate0 > 0 )
				m_nSelectedCandidate = 0;
			else
				m_nSelectedCandidate = 1;
		}
	}
	else
	{
		m_bOnDesignerObject = false;
		if( !CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, m_SideStairMode==eSideStairMode_DrawDiagonal ) )
			return;
		if( GetPickedRegion() )
		{
			m_bOnDesignerObject = true;
			SetPlane(GetPickedRegion()->GetPlane());
		}
	}
}

void CBrushDesignerStairProfileTool::CreateCandidates()
{
	int nStepNumber = 0;
	float fStepRise = g_pStairProfilePanel->GetStepRise();
	BUtil::EStairHeightCalculationWayMode heightMode = BUtil::eStairHeightCalculationWay_StepRise;

	BrushVec2 vStartPoint = GetPlane().W2P(GetStartSpotPos());
	BrushVec2 vEndPoint = GetPlane().W2P(GetCurrentSpotPos());
	if( vStartPoint.y > vEndPoint.y )
	{
		std::swap( vStartPoint, vEndPoint );
		SwapCurrentAndStartSpots();
	}
	BrushVec2 vShrinkedEndPoint = vEndPoint;

	if( heightMode == BUtil::eStairHeightCalculationWay_StepRise )
	{
		BrushVec3 vX3D = GetWorldTM().TransformVector(GetPlane().P2W(BrushVec2(1,0)));
		BrushVec3 vY3D = GetWorldTM().TransformVector(GetPlane().P2W(BrushVec2(0,1)));
		const BrushVec3 vZ(0,0,1);
		int nElement = std::abs(vZ.Dot(vX3D)) > std::abs(vZ.Dot(vY3D)) ? 0 : 1;

		BrushFloat fFullRise = std::abs(vEndPoint[nElement]-vStartPoint[nElement]);
		BrushFloat fFullLength = (vEndPoint-vStartPoint).GetLength();

		nStepNumber = fFullRise/fStepRise;
		vShrinkedEndPoint = vStartPoint+((vEndPoint-vStartPoint).GetNormalized()*((fStepRise*fFullLength)/fFullRise))*nStepNumber;
	}

	BrushVec2 vStart2End = vShrinkedEndPoint - vStartPoint;

	if( fabs(vStart2End.x) < kDesignerEpsilon || fabs(vStart2End.y) < kDesignerEpsilon )
		return;

	m_BorderLine = BrushLine(vStartPoint,vEndPoint);

	BrushVec2 vOneStep = vStart2End / nStepNumber;
	BrushVec2 vCurrentShotDir(0,1);
	BrushVec2 vNextShotDir(1,0);

	for( int k = 0; k < 2; ++k )
	{
		BrushVec2 vCurrentPoint = vStartPoint;
		BrushVec2 vNextPoint = vCurrentPoint;

		m_CandidateStairs[k].clear();
		m_CandidateStairs[k].push_back(Convert2Spot(GetDesigner(),GetPlane().P2W(vCurrentPoint)));

		for( int i = 0; i < nStepNumber+1; ++i )
		{
			vNextPoint += vOneStep;
			if( i == nStepNumber )
			{
				if( heightMode == BUtil::eStairHeightCalculationWay_StepNumber )
					break;
				vNextPoint = vEndPoint;
			}

			BrushLine currentLine(vCurrentPoint,vCurrentPoint+vCurrentShotDir);
			BrushLine nextLine(vNextPoint,vNextPoint+vNextShotDir);

			BrushVec2 vIntersection;
			if( !currentLine.Intersect( nextLine, vIntersection, kDesignerEpsilon ) )
			{
				GetIEditor()->CancelUndo();
				DESIGNER_ASSERT(0);
				return;
			}

			m_CandidateStairs[k].push_back(Convert2Spot(GetDesigner(),GetPlane().P2W(vIntersection)));
			m_CandidateStairs[k].push_back(Convert2Spot(GetDesigner(),GetPlane().P2W(vNextPoint)));

			vCurrentPoint = vNextPoint;
		}

		std::swap(vCurrentShotDir,vNextShotDir);

		m_CandidateStairs[k][0] = GetStartSpot();
		m_CandidateStairs[k][m_CandidateStairs[k].size()-1] = GetCurrentSpot();
	}
}
