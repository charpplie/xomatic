#include "StdAfx.h"
#include "BrushDesignerStairTool.h"
#include "Core/BrushDesigner.h"
#include "Viewport.h"
#include "Objects/DesignerBrushObject.h"
#include "Core/BrushDesignerAdjustHeightHelper.h"
#include "Core/BrushDesignerExtrudeSnappingHelper.h"
#include "BrushDesignerEditTool.h"
#include "IBaseToolPanel.h"

namespace
{
	ICreateStairToolPanel* g_pStairToolPanel = NULL;
	BrushFloat s_MinimumSize = (BrushFloat)0.001;
}

CBrushDesignerStairTool::CBrushDesignerStairTool() 
{
	m_StairMode = eStairMode_PlaceFirstPoint;
	m_fBoxWidth = m_fBoxDepth = 0;
	m_bIsOverOpposite = false;
}

CBrushDesignerStairTool::~CBrushDesignerStairTool()
{
}

void CBrushDesignerStairTool::Enter()
{
	__super::Enter();
	m_StairMode = eStairMode_PlaceFirstPoint;
}

void CBrushDesignerStairTool::Leave()
{
	if( m_StairMode == eStairMode_Done )
	{
		FreezeDesigner();
		AcceptUndo();
	}
	else
	{
		if( GetDesigner() )
		{
			DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
			GetDesigner()->SetShelf(1);
			GetDesigner()->Clear();
			UpdateShelf(1);
		}
	}

	m_pUndoDesigner = NULL;

	__super::Leave();
}

void CBrushDesignerStairTool::BeginEditParams()
{
	if( !g_pStairToolPanel )
		g_pStairToolPanel = CreateStairPanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerStairTool::EndEditParams()
{
	if( g_pStairToolPanel )
	{
		g_pStairToolPanel->DestroyPanel();
		g_pStairToolPanel = NULL;
	}
}

void CBrushDesignerStairTool::AcceptUndo()
{
	if( m_pUndoDesigner == NULL )
		return;
	GetIEditor()->BeginUndo();	
	m_pUndoDesigner->RecordUndo("Designer : Create a Stair",GetBaseObject());
	GetIEditor()->AcceptUndo("Designer : Create a Stair");
	m_pUndoDesigner = NULL;
}

void CBrushDesignerStairTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_StairMode == eStairMode_Done )
	{
		FreezeDesigner();
		AcceptUndo();
		m_StairMode = eStairMode_PlaceFirstPoint;
	}

	if( m_StairMode == eStairMode_PlaceFirstPoint )
	{
		if( !UpdateCurrentSpotPosition(view,nFlags,point,false,false) )
			return;

		m_StairMode = eStairMode_CreateRectangle;
		m_pUndoDesigner = new CBrushDesigner(*GetDesigner());
		m_fBoxWidth = m_fBoxDepth = 0;

		SetPlane(GetCurrentSpot().m_Plane);
		m_vStartPos = m_vEndPos = GetCurrentSpotPos();
		SetTempRegion(GetCurrentSpot().m_pRegion);
		StoreSeparateStatus();

 		g_pStairToolPanel->SetMirrored(false);
 		g_pStairToolPanel->SetRotateBy90Degree(false);
		s_SnappingHelper.Init(GetDesigner());
	}
	else if( m_StairMode == eStairMode_CreateBox )
	{
		if( m_fBoxHeight >= s_MinimumSize )
		{
			m_StairMode = eStairMode_Done;
			PlaceFirstPoint(view,nFlags,point);
		}
	}
}

void CBrushDesignerStairTool::OnLButtonUp( CViewport *view,UINT nFlags,CPoint point )
{
	if( m_StairMode == eStairMode_CreateRectangle )
	{
		if( std::abs(m_vStartPos.x-m_vEndPos.x) < s_MinimumSize || std::abs(m_vStartPos.y-m_vEndPos.y) < s_MinimumSize )
			return;
			
		m_StairMode = eStairMode_CreateBox;

		s_AdjustHeightHelper.Init(GetPlane(),m_vStartPos);

		BrushVec2 startSpotPos = GetPlane().W2P(m_vStartPos);
		BrushVec2 currentSpotPos = GetPlane().W2P(m_vEndPos);

		if( startSpotPos.x > currentSpotPos.x )
			std::swap(startSpotPos.x,currentSpotPos.x);
		if( startSpotPos.y > currentSpotPos.y )
			std::swap(startSpotPos.y,currentSpotPos.y);

		std::vector<BrushVec3> vList;
		vList.push_back(GetPlane().P2W(startSpotPos));
		vList.push_back(GetPlane().P2W(BrushVec2(startSpotPos.x,currentSpotPos.y)));
		vList.push_back(GetPlane().P2W(currentSpotPos));
		vList.push_back(GetPlane().P2W(BrushVec2(currentSpotPos.x,startSpotPos.y)));
		
		m_pCapRegion = new CBrushRegion(vList);
		s_SnappingHelper.SearchForOppositeRegions(m_pCapRegion);
		
		m_FloorPlane = GetPlane();
		CreateBox(view,nFlags,point);
	}
}

void CBrushDesignerStairTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
 	if( m_StairMode == eStairMode_PlaceFirstPoint || m_StairMode == eStairMode_Done )
 		PlaceFirstPoint(view,nFlags,point);
 	else if( m_StairMode == eStairMode_CreateRectangle )
		CreateRectangle(view,nFlags,point);
	else if( m_StairMode == eStairMode_CreateBox )
	{
		CreateBox(view,nFlags,point);
		UpdateStair();
	}
}

bool CBrushDesignerStairTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if( nChar == VK_ESCAPE )
	{
		if( m_StairMode == eStairMode_PlaceFirstPoint )
		{
			GetEditTool()->GoToSelectDesignerMode();
		}
		else if( m_StairMode == eStairMode_Done )
		{
			FreezeDesigner();
			AcceptUndo();
			m_StairMode = eStairMode_PlaceFirstPoint;
			return true;
		}
		else if( m_StairMode == eStairMode_CreateRectangle )
		{
			m_StairMode = eStairMode_PlaceFirstPoint;
		}
		else if( m_StairMode == eStairMode_CreateBox )
		{
			DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
			GetDesigner()->SetShelf(1);
			GetDesigner()->Clear();
			UpdateShelf(1);
			if( m_StairMode == eStairMode_CreateBox )
				m_StairMode = eStairMode_PlaceFirstPoint;
			else
				m_StairMode = eStairMode_CreateBox;
		}
	}
	return true;
}

void CBrushDesignerStairTool::GetRectangleVertices( BrushVec3& outV0, BrushVec3& outV1, BrushVec3& outV2, BrushVec3& outV3 )
{
	BrushVec2 p0 = GetPlane().W2P(m_vStartPos);
	BrushVec2 p2 = GetPlane().W2P(m_vEndPos);
	BrushVec2 p1(p2.x,p0.y);
	BrushVec2 p3(p0.x,p2.y);

	outV0 = GetPlane().P2W(p0);
	outV1 = GetPlane().P2W(p1);
	outV2 = GetPlane().P2W(p2);
	outV3 = GetPlane().P2W(p3);
}

void CBrushDesignerStairTool::Display( DisplayContext &dc )
{
	if( m_StairMode == eStairMode_CreateRectangle || m_StairMode == eStairMode_PlaceFirstPoint || m_StairMode == eStairMode_Done )
		DrawCurrentSpot(dc,GetWorldTM());

	if( m_StairMode == eStairMode_CreateRectangle )
	{
		dc.SetColor(ColorB(0,0,0,255));
		BrushVec3 v[4];
		GetRectangleVertices( v[0], v[1], v[2], v[3] );
		for( int i = 0; i < 4; ++i )
			dc.DrawLine( v[i], v[(i+1)%4] );
	}
	else if( m_StairMode == eStairMode_CreateBox )
	{
		dc.SetColor(ColorB(0,0,0,255));
		for( int i = 0; i < 4; ++i )
		{
			dc.DrawLine(m_BottomVertices[i],m_BottomVertices[(i+1)%4]);
			dc.DrawLine(m_TopVertices[i],m_TopVertices[(i+1)%4]);
			dc.DrawLine(m_BottomVertices[i],m_TopVertices[i]);
		}
	}

	DisplayDimensionHelper(dc,1);
	s_AdjustHeightHelper.Display(dc);
}

void CBrushDesignerStairTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch(event)
	{
	case eNotify_OnBeginUndoRedo:
	case eNotify_OnBeginSceneSave:
		if( m_StairMode == eStairMode_Done )
		{
			AcceptUndo();
			FreezeDesigner();
		}
		else
		{
			GetIEditor()->CancelUndo();
			CancelDesigner();
		}
		m_StairMode = eStairMode_PlaceFirstPoint;
		break;
	}
}

void CBrushDesignerStairTool::PlaceFirstPoint( CViewport *view,UINT nFlags,CPoint point )
{
	if( !CBrushDesignerDrawTool::UpdateCurrentSpotPosition(view,nFlags,point,false,true) )
	{
		UpdateSelectionMesh(NULL,GetBrush(),GetBaseObject());
		return;
	}
	if( m_StairMode == eStairMode_PlaceFirstPoint )
		SetPlane(GetCurrentSpot().m_Plane);	
}

void CBrushDesignerStairTool::CreateRectangle( CViewport *view, UINT nFlags, CPoint point )
{
	if( !CBrushDesignerDrawTool::UpdateCurrentSpotPosition(view,nFlags,point,true) )
	{
		UpdateSelectionMesh(NULL,GetBrush(),GetBaseObject());
		return;
	}

	m_vStartPos = GetCurrentSpotPos();

	BrushVec2 startSpotPos = GetPlane().W2P(m_vStartPos);
	BrushVec2 endSpotPos = GetPlane().W2P(m_vEndPos);

	m_fBoxWidth = endSpotPos.x-startSpotPos.x;
	m_fBoxDepth = endSpotPos.y-startSpotPos.y;

	m_bXDirection = std::abs(m_fBoxWidth) >= std::abs(m_fBoxDepth);

	g_pStairToolPanel->Update( std::abs(m_fBoxWidth), 0, std::abs(m_fBoxDepth) );
}

void CBrushDesignerStairTool::CreateBox( CViewport *view, UINT nFlags, CPoint point )
{
	m_fBoxHeight = s_AdjustHeightHelper.UpdateHeight(GetWorldTM(), view, point);	

	CBrushRegion::RegionPtr pRegion = m_pCapRegion->Clone();
	pRegion->UpdatePlane(BrushPlane(pRegion->GetPlane().Normal(),m_pCapRegion->GetPlane().Distance()-m_fBoxHeight));

	m_bIsOverOpposite = false;

	if( nFlags & MK_SHIFT )
	{
		CBrushRegion::RegionPtr pAlignedRegion = s_SnappingHelper.FindAlignedRegion(m_pCapRegion,GetWorldTM(),view,point);
		if( pAlignedRegion )
			m_fBoxHeight = GetPlane().Distance() - pAlignedRegion->GetPlane().Distance();
		pRegion->UpdatePlane(BrushPlane(pRegion->GetPlane().Normal(),m_pCapRegion->GetPlane().Distance()-m_fBoxHeight));
	}
	else
	{
		m_bIsOverOpposite = s_SnappingHelper.IsOverOppositeRegion(pRegion,BUtil::ePP_Pull);
		if( pRegion && m_bIsOverOpposite )
			m_fBoxHeight = s_SnappingHelper.GetNearestDistanceToOpposite(BUtil::ePP_Pull);
	}

	BrushVec3 vNormal = m_fBoxHeight*GetPlane().Normal();
	GetRectangleVertices(m_BottomVertices[0],m_BottomVertices[1],m_BottomVertices[2],m_BottomVertices[3]);

	g_pStairToolPanel->Update( std::abs(m_fBoxWidth), m_fBoxHeight, std::abs(m_fBoxDepth) );

	for(int i = 0; i < 4; ++i )
		m_TopVertices[i] = m_BottomVertices[i] + vNormal;
}

CBrushRegion::RegionPtr CBrushDesignerStairTool::CreateRegion( const std::vector<BrushVec3>& vList, bool bFlip, CBrushRegion::RegionPtr pBaseRegion )
{
	if( vList.size() < 3 )
		return NULL;
	CBrushRegion::RegionPtr pRegion = new CBrushRegion(vList);
	if( pRegion->IsOpen() )
		return NULL;
	if( bFlip )
		pRegion->Flip();
	if( pBaseRegion )
	{
		pRegion->SetTexInfo(pBaseRegion->GetTexInfo());
		pRegion->SetMaterialID(pBaseRegion->GetMaterialID());
	}
	return pRegion;
}

void CBrushDesignerStairTool::CreateStair(	const BrushVec3& vStartPos,
											const BrushVec3& vEndPos, 
											BrushFloat fBoxWidth, 
											BrushFloat fBoxDepth, 
											BrushFloat fBoxHeight, 
											const BrushPlane& floorPlane, 
											float fStepRise, 
											bool bXDirection, 
											bool bMirrored, 
											bool bRotationBy90Degree, 
											CBrushRegion::RegionPtr pBaseRegion,
											SOutputParameterForStairCreation& out )
{	
	if( bRotationBy90Degree )
		bXDirection = !bXDirection;

	int nCompleteStepNum = (int)(fBoxHeight/fStepRise);
	BrushFloat fRestStepRise = fBoxHeight - fStepRise*nCompleteStepNum;
	BrushFloat fRatioRestStepLengthToFullLength = fRestStepRise/fBoxHeight;

	BrushVec2 startSpotPos = floorPlane.W2P(vStartPos);
	BrushVec2 endSpotPos = floorPlane.W2P(vEndPos);

	endSpotPos.x = startSpotPos.x + fBoxWidth;
	endSpotPos.y = startSpotPos.y + fBoxDepth;

	if( bMirrored )
	{
		std::swap(startSpotPos,endSpotPos);
		fBoxDepth = -fBoxDepth;
		fBoxWidth = -fBoxWidth;
	}

	BrushFloat fStairSize = bXDirection ? fBoxWidth : fBoxDepth;

	BrushFloat fStepTread = nCompleteStepNum == 0 ? 0 : ((1-fRatioRestStepLengthToFullLength)*std::abs(fStairSize))/nCompleteStepNum;
	BrushFloat fRestStepTread = std::abs(fStairSize)-fStepTread*nCompleteStepNum;

	int nWidthSign = fBoxWidth > 0 ? 1 : -1;
	int nDepthSign = fBoxDepth > 0 ? 1 : -1;

	std::vector<BrushVec3> vSideList[2];
	std::vector<BrushVec3> vBackList;
	std::vector<BrushVec3> vBottomList;

	bool bFlip = bXDirection ? nWidthSign*nDepthSign == -1 : nWidthSign*nDepthSign == 1;

	for( int i = 0; i <= nCompleteStepNum; ++i )
	{
		BrushFloat x = startSpotPos.x + i*fStepTread*nWidthSign;
		BrushFloat next_x = i < nCompleteStepNum ? startSpotPos.x + (i+1)*fStepTread*nWidthSign : x+fRestStepTread*nWidthSign;

		BrushFloat z = startSpotPos.y + i*fStepTread*nDepthSign;
		BrushFloat next_z = i < nCompleteStepNum ? startSpotPos.y + (i+1)*fStepTread*nDepthSign : z+fRestStepTread*nDepthSign;

		BrushFloat y = i*fStepRise;
		BrushFloat next_y = i < nCompleteStepNum ? (i+1)*fStepRise : y+fRestStepRise;

		std::vector<BrushVec3> vStepRiseList(4);
		if( bXDirection )
		{
			vStepRiseList[0] = floorPlane.P2W(BrushVec2(x,startSpotPos.y));
			vStepRiseList[1] = floorPlane.P2W(BrushVec2(x,endSpotPos.y));
			vStepRiseList[0] += floorPlane.Normal() * y;
			vStepRiseList[1] += floorPlane.Normal() * y;

			vStepRiseList[2] = floorPlane.P2W(BrushVec2(x,endSpotPos.y));
			vStepRiseList[3] = floorPlane.P2W(BrushVec2(x,startSpotPos.y));
			vStepRiseList[2] += floorPlane.Normal() * next_y;
			vStepRiseList[3] += floorPlane.Normal() * next_y;
		}
		else
		{
			vStepRiseList[0] = floorPlane.P2W(BrushVec2(startSpotPos.x,z));
			vStepRiseList[1] = floorPlane.P2W(BrushVec2(endSpotPos.x,z));
			vStepRiseList[0] += floorPlane.Normal() * y;
			vStepRiseList[1] += floorPlane.Normal() * y;

			vStepRiseList[2] = floorPlane.P2W(BrushVec2(endSpotPos.x,z));
			vStepRiseList[3] = floorPlane.P2W(BrushVec2(startSpotPos.x,z));
			vStepRiseList[2] += floorPlane.Normal() * next_y;
			vStepRiseList[3] += floorPlane.Normal() * next_y;
		}
		CBrushRegion::RegionPtr pStepRiseRegion = CreateRegion(vStepRiseList,bFlip,pBaseRegion);
		if( pStepRiseRegion )
			out.regions.push_back(pStepRiseRegion);

		if( i == 0 )
		{
			vBottomList.push_back(vStepRiseList[1]);
			vBottomList.push_back(vStepRiseList[0]);

			if( pStepRiseRegion )
				out.regionsNeedPostProcess.push_back(pStepRiseRegion);
		}

		vSideList[0].push_back(vStepRiseList[0]);
		vSideList[0].push_back(vStepRiseList[3]);

		vSideList[1].push_back(vStepRiseList[1]);
		vSideList[1].push_back(vStepRiseList[2]);

		std::vector<BrushVec3> vStepTreadList(4);
		if( bXDirection )
		{
			vStepTreadList[0] = floorPlane.P2W(BrushVec2(x,startSpotPos.y));
			vStepTreadList[1] = floorPlane.P2W(BrushVec2(x,endSpotPos.y));
			vStepTreadList[2] = floorPlane.P2W(BrushVec2(next_x,endSpotPos.y));
			vStepTreadList[3] = floorPlane.P2W(BrushVec2(next_x,startSpotPos.y));

			vStepTreadList[0] += floorPlane.Normal() * next_y;
			vStepTreadList[1] += floorPlane.Normal() * next_y;
			vStepTreadList[2] += floorPlane.Normal() * next_y;
			vStepTreadList[3] += floorPlane.Normal() * next_y;
		}
		else
		{
			vStepTreadList[0] = floorPlane.P2W(BrushVec2(startSpotPos.x,z));
			vStepTreadList[1] = floorPlane.P2W(BrushVec2(endSpotPos.x,z));
			vStepTreadList[2] = floorPlane.P2W(BrushVec2(endSpotPos.x,next_z));
			vStepTreadList[3] = floorPlane.P2W(BrushVec2(startSpotPos.x,next_z));

			vStepTreadList[0] += floorPlane.Normal() * next_y;
			vStepTreadList[1] += floorPlane.Normal() * next_y;
			vStepTreadList[2] += floorPlane.Normal() * next_y;
			vStepTreadList[3] += floorPlane.Normal() * next_y;
		}
		CBrushRegion::RegionPtr pStepTreadRegion = CreateRegion(vStepTreadList,bFlip,pBaseRegion);
		if( pStepTreadRegion )
			out.regions.push_back(pStepTreadRegion);

		if( i >= nCompleteStepNum-1 )
		{
			if( pStepTreadRegion )
				out.pCapRegion = pStepTreadRegion;
		}

		if( i == nCompleteStepNum )
		{
			vSideList[0].push_back(vStepTreadList[3]);
			if( bXDirection )
				vSideList[0].push_back(floorPlane.P2W(BrushVec2(next_x,startSpotPos.y)));
			else
				vSideList[0].push_back(floorPlane.P2W(BrushVec2(startSpotPos.x,next_z)));
			CBrushRegion::RegionPtr pSideRegion0 = CreateRegion(vSideList[0],bFlip,pBaseRegion);			
			if( pSideRegion0 )
			{
				out.regions.push_back(pSideRegion0);
				out.regionsNeedPostProcess.push_back(pSideRegion0);
			}

			vSideList[1].push_back(vStepTreadList[2]);
			if( bXDirection )
				vSideList[1].push_back(floorPlane.P2W(BrushVec2(next_x,endSpotPos.y)));
			else
				vSideList[1].push_back(floorPlane.P2W(BrushVec2(endSpotPos.x,next_z)));
			CBrushRegion::RegionPtr pSideRegion1 = CreateRegion(vSideList[1],!bFlip,pBaseRegion);
			if( pSideRegion1 )
			{
				out.regions.push_back(pSideRegion1);
				out.regionsNeedPostProcess.push_back(pSideRegion1);
			}

			vBackList.push_back(vSideList[0][vSideList[0].size()-1]);
			vBackList.push_back(vSideList[0][vSideList[0].size()-2]);
			vBackList.push_back(vSideList[1][vSideList[1].size()-2]);
			vBackList.push_back(vSideList[1][vSideList[1].size()-1]);
			CBrushRegion::RegionPtr pBackRegion = CreateRegion(vBackList,bFlip,pBaseRegion);
			if( pBackRegion )
			{
				out.regions.push_back(pBackRegion);
				out.regionsNeedPostProcess.push_back(pBackRegion);
			}

			vBottomList.push_back(vBackList[0]);
			vBottomList.push_back(vBackList[3]);
			out.regions.push_back(CreateRegion(vBottomList,bFlip,pBaseRegion));
		}
	}
}

void CBrushDesignerStairTool::UpdateStair()
{
	if( m_fBoxHeight < 0 || m_StairMode == eStairMode_PlaceFirstPoint )
		return;

	BrushFloat fStepRise = g_pStairToolPanel->GetStepRise();
	bool bXDirection = m_bXDirection;
	bool bMirrored = g_pStairToolPanel->IsMirrored();
	bool bRotationBy90Degree = g_pStairToolPanel->IsRotateBy90Degree();

	SOutputParameterForStairCreation output;
	CreateStair( m_vStartPos, m_vEndPos, m_fBoxWidth, m_fBoxDepth, m_fBoxHeight, m_FloorPlane, fStepRise, m_bXDirection, bMirrored, bRotationBy90Degree, GetTempRegion(), output );

	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(1);
	GetDesigner()->Clear();

	m_RegionsNeedPostProcess = output.regionsNeedPostProcess;
	for( int i = 0, iRegionCount(output.regions.size()); i < iRegionCount; ++i )
		GetDesigner()->AddRegion(output.regions[i],CBrushDesigner::eOpType_Add);
	m_pCapRegion = output.pCapRegion;

	CBrushDesignerBaseTool::CreateMirroredRegions(GetDesigner());
	UpdateShelf(1);
}

void CBrushDesignerStairTool::UpdateStair( BrushFloat fWidth, BrushFloat fHeight, BrushFloat fDepth )
{
	m_fBoxHeight = fHeight;
	m_fBoxWidth = m_fBoxWidth >= 0 ? fWidth : -fWidth;
	m_fBoxDepth = m_fBoxDepth >= 0 ? fDepth : -fDepth;
	UpdateStair();
}

void CBrushDesignerStairTool::FreezeDesigner()
{
	if( m_bIsOverOpposite )
		s_SnappingHelper.ApplyOppositeRegions(m_pCapRegion,BUtil::ePP_Pull,true);

	std::vector<CBrushRegion::RegionPtr> candidateRegions;
	for( int k = 0, iCount(m_RegionsNeedPostProcess.size()); k < iCount; ++k )
	{
		DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

		CBrushRegion::RegionPtr pPostRegion = m_RegionsNeedPostProcess[k];		
		CBrushRegion::RegionPtr pFlipedPostRegion = pPostRegion->Clone()->Flip();

		GetDesigner()->SetShelf(0);
		std::vector<CBrushRegion::RegionPtr> intersectedRegions;

		for( int i = 0, iRegionCount(GetDesigner()->GetRegionSize()); i < iRegionCount; ++i )
		{
			CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(i);
			if( CBrushRegion::HasIntersection(pRegion,pFlipedPostRegion) )
			{
				intersectedRegions.push_back(pRegion);
				candidateRegions.push_back(pRegion);
			}
		}

		if( !intersectedRegions.empty() )
		{
			GetDesigner()->SetShelf(1);
			GetDesigner()->RemoveRegion(pPostRegion);
			for( int i = 0, iRegionCount(intersectedRegions.size()); i < iRegionCount; ++i )
			{
				CBrushRegion::RegionPtr pCopiedFlipedPostRegion = pFlipedPostRegion->Clone();
				pFlipedPostRegion->Subtract(intersectedRegions[i]);
				intersectedRegions[i]->Subtract(pCopiedFlipedPostRegion);
			}
			if( pFlipedPostRegion->IsValid() && !pFlipedPostRegion->IsOpen() )
				GetDesigner()->AddRegion(pFlipedPostRegion->Flip(),CBrushDesigner::eOpType_Add);
		}
	}

	GetDesigner()->SetShelf(0);
	for( int i = 0, iCount(candidateRegions.size()); i < iCount; ++i )
		GetDesigner()->SeparateRegions(candidateRegions[i]->GetPlane());

	CBrushDesignerBaseTool::FreezeDesigner();
}