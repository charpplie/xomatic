#include "StdAfx.h"
#include "BrushDesignerCreateSphereTool.h"
#include "Core/BrushDesigner.h"
#include "Core/BrushPrimitive.h"
#include "BrushDesignerEditTool.h"
#include "IBaseToolPanel.h"

namespace 
{
	ICreateSphereDiscCurveToolPanel* g_pSpherePanel = NULL;
}

void CBrushDesignerCreateSphereTool::Enter()
{
	__super::Enter();
	SetEditMode(eEditMode_Beginning);
}

void CBrushDesignerCreateSphereTool::Leave()
{
	if( GetEditMode() == eEditMode_Done )
	{
		GetIEditor()->AcceptUndo("Designer : Create a Sphere");
		FreezeDesigner();
	}
	else
	{
		DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
		GetDesigner()->SetShelf(1);
		GetDesigner()->Clear();
		UpdateShelf(1);
		GetIEditor()->CancelUndo();
	}
	__super::Leave();
}

void CBrushDesignerCreateSphereTool::BeginEditParams()
{
	if( !g_pSpherePanel )
		g_pSpherePanel = CreateSpherePanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerCreateSphereTool::EndEditParams()
{
	if( g_pSpherePanel )
	{
		g_pSpherePanel->DestroyPanel();	
		g_pSpherePanel = NULL;
	}
}

void CBrushDesignerCreateSphereTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	int bKeepInitPlane = GetEditMode() == eEditMode_Editing ? true : false;
	int bSearchAllShelves = GetEditMode() == eEditMode_Done ? true : false;
	if( !CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, bKeepInitPlane, bSearchAllShelves ) )
		return;

	if( GetPickedRegion() )
		SetPlane(GetPickedRegion()->GetPlane());

	if( GetEditMode() == eEditMode_Editing )
	{
		BrushVec2 vSpotPos2D = GetPlane().W2P(GetCurrentSpotPos());

		BrushFloat fRadius = (vSpotPos2D-m_vCenterOnPlane).GetLength();
		const BrushFloat kSmallestRadius = 0.05f;
		if( fRadius < kSmallestRadius )
			fRadius = kSmallestRadius;

		m_fAngle = BUtil::ComputeAnglePointedByPos( m_vCenterOnPlane, vSpotPos2D );

		UpdateSphere(fRadius,g_pSpherePanel->GetSubdivisionNum());
		g_pSpherePanel->Update(fRadius);
	}
}

void CBrushDesignerCreateSphereTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	if( GetEditMode() == eEditMode_Done )
	{
		SetEditMode(eEditMode_Beginning);
		if( GetIntermediateRegion()->IsValid() )
			FreezeDesigner();
		SetIntermediateRegion(NULL);
		GetIEditor()->AcceptUndo("Designer : Create a Sphere");
	}
	if( GetEditMode() == eEditMode_Beginning )
	{
		if( CBrushDesignerDrawTool::UpdateCurrentSpotPosition( view, nFlags, point, false, true ) )
		{
			SetStartSpot(GetCurrentSpot());
			SetEditMode(eEditMode_Editing);
			SetPlane(GetCurrentSpot().m_Plane);
			SetTempRegion(GetCurrentSpot().m_pRegion);
			m_vCenterOnPlane = GetPlane().W2P(GetCurrentSpotPos());
			GetIEditor()->BeginUndo();
			GetDesigner()->RecordUndo("Designer : Create a Sphere",GetBaseObject());
			StoreSeparateStatus();
			m_MatTo001 = ToBrushMatrix33(Matrix33::CreateRotationV0V1(GetPlane().Normal(),Vec3(0,0,1)));
			m_MatTo001.SetTranslation(-GetCurrentSpotPos());
		}
	}
	else if( GetEditMode() == eEditMode_Editing )
	{
		SetEditMode(eEditMode_Done);
	}
}

bool CBrushDesignerCreateSphereTool::OnKeyDown( CViewport *view, uint32 nChar, uint32 nRepCnt, uint32 nFlags )
{
	if (nChar == VK_ESCAPE )
	{
		if( GetEditMode() == eEditMode_Editing )
		{
			CancelDesigner();
			SetEditMode(eEditMode_Beginning);
			return true;
		}
		else if( GetEditMode() == eEditMode_Done )
		{
			SetPlane(BrushPlane(BrushVec3(0,0,0),0));
			FreezeDesigner();
			SetEditMode(eEditMode_Beginning);
			GetIEditor()->AcceptUndo("Designer : Create a Sphere");
			return true;
		}
		GetEditTool()->GoToSelectDesignerMode();
	}
	return true;
}

void CBrushDesignerCreateSphereTool::Display( DisplayContext &dc )
{
	DisplayCurrentSpot(dc);
	if( GetEditMode() == eEditMode_Editing || GetEditMode() == eEditMode_Done )
		DrawIntermediateRegion(dc);

	DisplayDimensionHelper(dc,1);
	if( GetStartSpot().m_pRegion )
		DisplayDimensionHelper(dc);
}

void CBrushDesignerCreateSphereTool::UpdateSphere( float fRadius, int nSubdivisionNum )
{
	UpdateHelperDisc(fRadius,nSubdivisionNum);

	AABB aabb;
	aabb.Reset();
	for( int i = 0, iVertexCount(GetIntermediateRegion()->GetVertexListSize()); i < iVertexCount; ++i )
		aabb.Add(m_MatTo001.TransformPoint(GetIntermediateRegion()->GetVertex(i)));

	aabb.max.z = aabb.max.z + fRadius;
	aabb.min.z = aabb.min.z - fRadius;

	if( aabb.IsReset() )
		return;

	m_SphereRegions.clear();
	CBrushPrimitive bp(NULL);
	bp.CreateSphere( aabb.min, aabb.max, g_pSpherePanel->GetSubdivisionNum(), &m_SphereRegions );

	if( GetTempRegion() )
	{
		for( int i = 0, iRegionCount(m_SphereRegions.size()); i < iRegionCount; ++i )
		{
			m_SphereRegions[i]->SetMaterialID(GetTempRegion()->GetMaterialID());
			m_SphereRegions[i]->SetTexInfo(GetTempRegion()->GetTexInfo());
		}
	}

	UpdateDesignerBasedOnSphereRegions(BrushMatrix34::CreateIdentity());
}

void CBrushDesignerCreateSphereTool::UpdateHelperDisc( float fRadius, int nSubdivisionNum )
{
	std::vector<BrushVec2> vertices2D;
	BUtil::MakeSectorOfCircle( fRadius, m_vCenterOnPlane, m_fAngle, BUtil::PI2, nSubdivisionNum+1, vertices2D );
	vertices2D.erase(vertices2D.begin());
	BUtil::STexInfo texInfo = GetTexInfo();
	CBrushRegion::RegionPtr pRegion = GetIntermediateRegion();
	if( !pRegion )
	{
		pRegion = new CBrushRegion( vertices2D, GetPlane(), GetMatID(), &texInfo, true );
		SetIntermediateRegion(pRegion);
	}
	else
	{
		*pRegion = CBrushRegion( vertices2D, pRegion->GetPlane(), GetMatID(), &texInfo, true );
	}
}

void CBrushDesignerCreateSphereTool::UpdateDesignerBasedOnSphereRegions( const BrushMatrix34& tm )
{
	BrushMatrix34 matFrom001 = m_MatTo001.GetInverted();

	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(1);
	GetDesigner()->Clear();
	for( int i = 0, iRegionCount(m_SphereRegions.size()); i < iRegionCount; ++i )
	{
		m_SphereRegions[i]->Transform(matFrom001);
		GetDesigner()->AddRegion(m_SphereRegions[i],CBrushDesigner::eOpType_Add);
	}
	CBrushDesignerBaseTool::CreateMirroredRegions(GetDesigner());
	UpdateShelf(1);
}

void CBrushDesignerCreateSphereTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch(event)
	{
	case eNotify_OnBeginUndoRedo:
	case eNotify_OnBeginSceneSave:
		if( GetEditMode() == eEditMode_Done )
		{
			GetIEditor()->AcceptUndo("Designer : Create a Sphere");
			FreezeDesigner();
		}
		else
		{
			GetIEditor()->CancelUndo();
			CancelDesigner();
		}
		SetEditMode(eEditMode_Beginning);
	}
}

void CBrushDesignerCreateSphereTool::FreezeDesigner()
{
	SetPlane(BrushPlane(BrushVec3(0,0,0),0));
	CBrushDesignerBaseTool::FreezeDesigner();
}