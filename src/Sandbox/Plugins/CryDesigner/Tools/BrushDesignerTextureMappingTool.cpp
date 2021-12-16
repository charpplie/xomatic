#include "StdAfx.h"
#include "BrushDesignerTextureMappingTool.h"
#include "BrushDesignerEditTool.h"
#include "Core/BrushDesigner.h"
#include "Viewport.h"
#include "Core/BrushDesignerUndo.h"
#include "Objects/DesignerBrushObject.h"
#include "IBaseToolPanel.h"

namespace
{
	ITextureMappingToolPanel* g_pTextureMappingToolPanel = NULL;
}

CBrushDesignerTextureMappingTool::CBrushDesignerTextureMappingTool()
{
	m_nPickFlag = BUtil::ePF_Face;
}

CBrushDesignerTextureMappingTool::~CBrushDesignerTextureMappingTool()
{
}

void CBrushDesignerTextureMappingTool::Enter()
{
	__super::Enter();
	MoveSelectedElements();

	if( GetIEditor()->GetEditMode() == eEditModeRotate )
		GetIEditor()->SetEditMode(eEditModeRotateCircle);

	int nCurrentEditMode = GetIEditor()->GetEditMode();

	GetIEditor()->SetEditMode(eEditModeMove);
	GetIEditor()->SetReferenceCoordSys(COORDS_WORLD);
	GetIEditor()->SetEditMode(eEditModeScale);
	GetIEditor()->SetReferenceCoordSys(COORDS_WORLD);
	GetIEditor()->SetEditMode(nCurrentEditMode);

	if( GetIEditor()->GetEditMode() == eEditModeRotateCircle )
		GetIEditor()->SetReferenceCoordSys(COORDS_LOCAL);

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	UpdateTMManipulatorBasedOnElements(pSelected);
}

void CBrushDesignerTextureMappingTool::Leave()
{
	if( GetDesigner() )
	{
		GetDesigner()->MoveShelf(1,0);
		GetDesigner()->SetShelf(0);
		UpdateBrush();
		Sync();
	}
	__super::Leave();
}

void CBrushDesignerTextureMappingTool::BeginEditParams()
{
	if( !g_pTextureMappingToolPanel )
		g_pTextureMappingToolPanel = CreateTextureMappingPanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerTextureMappingTool::EndEditParams()
{
	if( g_pTextureMappingToolPanel )
	{
		g_pTextureMappingToolPanel->DestroyPanel();
		g_pTextureMappingToolPanel = NULL;
	}
}

bool CBrushDesignerTextureMappingTool::IsPicking() const
{
	if( g_pTextureMappingToolPanel == NULL )
		return false;
	return g_pTextureMappingToolPanel->IsPicking();
}

void CBrushDesignerTextureMappingTool::OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )
{
	if( IsPicking() )
	{
		if( !g_pTextureMappingToolPanel )
			return;

		bool bOnlyIncludeCube = GetKeyState(VK_SPACE) & (1<<15);
		CBrushDesignerElementManager pickedElements;
		if( !pickedElements.Pick(GetBaseObject(), GetDesigner(), view, point, m_nPickFlag, bOnlyIncludeCube, NULL) )
			GetIEditor()->ShowTransformManipulator(false);

		if( !pickedElements.IsEmpty() && pickedElements[0].IsFace() && pickedElements[0].m_pRegion )
		{
			g_pTextureMappingToolPanel->SetTexInfo(pickedElements[0].m_pRegion->GetTexInfo());
			g_pTextureMappingToolPanel->SetMatID(pickedElements[0].m_pRegion->GetMaterialID());
			ApplyTextureInfo(pickedElements[0].m_pRegion->GetTexInfo(),false);
			UpdateBrush();
		}
	}
	else
	{
		__super::OnLButtonDown(view,nFlags,point);
	}
}

void CBrushDesignerTextureMappingTool::OnLButtonUp( CViewport *view,UINT nFlags,CPoint point )
{
	__super::OnLButtonUp(view,nFlags,point);
	MoveSelectedElements();
}

void CBrushDesignerTextureMappingTool::MoveSelectedElements()
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->MoveShelf(1,0);
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	for( int i = 0, iCount(pSelected->GetSize()); i < iCount; ++i )
	{
		GetDesigner()->SetShelf(1);
		GetDesigner()->AddRegion((*pSelected)[i].m_pRegion,CBrushDesigner::eOpType_Add);
		GetDesigner()->SetShelf(0);
		GetDesigner()->RemoveRegion((*pSelected)[i].m_pRegion);
	}
	UpdateBrush();
}

bool CBrushDesignerTextureMappingTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if( nChar == VK_ESCAPE )
		GetEditTool()->GoToSelectDesignerMode();
	return true;
}

bool CBrushDesignerTextureMappingTool::QueryRegion( const BrushVec3& localRaySrc, const BrushVec3& localRayDir, int& nOutRegionIndex, bool& bOutNew ) const
{
	bOutNew = false;
	GetDesigner()->SetShelf(1);
	if( !GetDesigner()->QueryRegion( localRaySrc, localRayDir, nOutRegionIndex ) )
	{
		GetDesigner()->SetShelf(0);
		if( !GetDesigner()->QueryRegion( localRaySrc, localRayDir, nOutRegionIndex ) )
			return false;
		bOutNew = true;
	}
	return true;
}

void CBrushDesignerTextureMappingTool::ApplyTextureInfo( const BUtil::STexInfo& texInfo, bool bAdd )
{
	CUndo undo("Change Texture Info of a designer");

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	for( int i = 0, iCount(pSelected->GetSize()); i < iCount; ++i )
	{
		if( !(*pSelected)[i].m_pRegion )
			continue;
		if( bAdd )
		{
			const BUtil::STexInfo& regionTexInfo = (*pSelected)[i].m_pRegion->GetTexInfo();
			BUtil::STexInfo addedTexInfo;
			addedTexInfo.shift[0] = regionTexInfo.shift[0] + texInfo.shift[0];
			addedTexInfo.shift[1] = regionTexInfo.shift[1] + texInfo.shift[1];
			addedTexInfo.scale[0] = regionTexInfo.scale[0] + texInfo.scale[0];
			addedTexInfo.scale[1] = regionTexInfo.scale[1] + texInfo.scale[1];
			addedTexInfo.rotate = regionTexInfo.rotate + texInfo.rotate;
			SetTexInfoToRegion( (*pSelected)[i].m_pRegion, addedTexInfo );
		}
		else
		{
			SetTexInfoToRegion( (*pSelected)[i].m_pRegion, texInfo );
		}
	}
}

void CBrushDesignerTextureMappingTool::FitTexture( float fTileU, float fTileV )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	for( int i = 0, iCount(pSelected->GetSize()); i < iCount; ++i )
	{
		if( !(*pSelected)[i].m_pRegion )
			continue;	

		const AABB& regionAABB = (*pSelected)[i].m_pRegion->GetBoundBox();
		BUtil::STexInfo texInfo;
		BUtil::FitTexture( ToFloatPlane((*pSelected)[i].m_pRegion->GetPlane()), regionAABB.min, regionAABB.max, fTileU, fTileV, texInfo );
		SetTexInfoToRegion( (*pSelected)[i].m_pRegion, texInfo );

		if( i == 0 && g_pTextureMappingToolPanel )
			g_pTextureMappingToolPanel->SetTexInfo((*pSelected)[i].m_pRegion->GetTexInfo());
	}
}

void CBrushDesignerTextureMappingTool::SetSubMatID( int nSubMatID, CBrushDesigner* pDesigner )
{	
	if( g_pTextureMappingToolPanel )
		g_pTextureMappingToolPanel->SetMatID(nSubMatID+1);
}

void CBrushDesignerTextureMappingTool::SelectRegionsByMatID( int matID )
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	GetDesigner()->MoveShelf(1,0);
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->Clear();

	GetDesigner()->SetShelf(0);
	for( int i = 0, iRegionCount(GetDesigner()->GetRegionSize()); i < iRegionCount; ++i )
	{
		GetDesigner()->SetShelf(0);
		CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(i);
		if( pRegion == NULL )
			continue;
		if( pRegion->GetMaterialID() == matID )
		{
			GetDesigner()->SetShelf(1);
			GetDesigner()->AddRegion(pRegion,CBrushDesigner::eOpType_Add);
			SDesignerElement element;
			element.SetFace(GetBaseObject(),pRegion);
			pSelected->Add(element);
		}
	}

	GetDesigner()->SetShelf(0);
	for( int i = 0, iSelectionCount(pSelected->GetSize()); i < iSelectionCount; ++i )
		GetDesigner()->RemoveRegion((*pSelected)[i].m_pRegion);

	UpdateSelectionMeshFromSelectedElementList(GetMainContext());
}

bool CBrushDesignerTextureMappingTool::GetTexInfoOfSelectedRegion( BUtil::STexInfo& outTexInfo ) const
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( pSelected->IsEmpty() )
		return false;
	if( !(*pSelected)[0].m_pRegion )
		return false;
	outTexInfo = (*pSelected)[0].m_pRegion->GetTexInfo();
	return true;
}

void CBrushDesignerTextureMappingTool::SetTexInfoToRegion( CBrushRegion::RegionPtr pRegion, const BUtil::STexInfo& texInfo )
{
	if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) )
	{
		BUtil::STexInfo mirroredTexInfo(texInfo);
		mirroredTexInfo.rotate = -mirroredTexInfo.rotate;
		pRegion->SetTexInfo(mirroredTexInfo);
	}
	else
	{
		pRegion->SetTexInfo(texInfo);
	}
}

void CBrushDesignerTextureMappingTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch( event )
	{
	case eNotify_OnBeginSceneSave:
		GetDesigner()->MoveShelf(1,0);
		UpdateBrush();
		break;
	case eNotify_OnEndSceneSave:
		MoveSelectedElements();
		break;
	}
}

void CBrushDesignerTextureMappingTool::OnManipulatorDrag( CViewport *pView,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const BrushVec3 &value )
{
	if( m_MouseDownContext.m_TexInfos.empty() )
		return;

	BrushMatrix34 offsetTM;
	offsetTM = GetOffsetTM(pManipulator,value);

	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	int editMode = GetIEditor()->GetEditMode();
	if( editMode == eEditModeMove )
	{
		Vec3 vTranslation = offsetTM.GetTranslation();
		BrushVec3 vNormal(0,0,0);

		for( int i = 0, iCount(m_MouseDownContext.m_TexInfos.size()); i < iCount; ++i )
		{
			CBrushRegion::RegionPtr pRegion(m_MouseDownContext.m_TexInfos[i].first);
			BUtil::STexInfo texInfo(m_MouseDownContext.m_TexInfos[i].second);

			SBrushPlane<float> p(ToVec3(pRegion->GetPlane().Normal()),ToFloat(pRegion->GetPlane().Distance()));
			Vec3 basis_u, basis_v;
			BUtil::CalcTextureBasis(p, texInfo, basis_u, basis_v);

			BrushFloat tu = (BrushFloat)basis_u.Dot(-vTranslation);
			BrushFloat tv = (BrushFloat)basis_v.Dot(-vTranslation);
			
			texInfo.shift[0] = texInfo.shift[0] + tu;
			texInfo.shift[1] = texInfo.shift[1] + tv;

			pRegion->SetTexInfo(texInfo);
			g_pTextureMappingToolPanel->SetTexInfo(texInfo);

			vNormal += pRegion->GetPlane().Normal();
		}

		UpdateShelf(1);
		UpdateTMManipulator(m_MouseDownContext.m_MouseDownPos+vTranslation, vNormal.GetNormalized());
	}
	else if( editMode == eEditModeRotateCircle )
	{
		for( int i = 0, iCount(m_MouseDownContext.m_TexInfos.size()); i < iCount; ++i )
		{
			CBrushRegion::RegionPtr pRegion(m_MouseDownContext.m_TexInfos[i].first);
			BUtil::STexInfo texInfo(m_MouseDownContext.m_TexInfos[i].second);

			BrushVec3 tn = pRegion->GetPlane().Normal();
			if( std::abs(tn.x) > std::abs(tn.y) && std::abs(tn.x) > std::abs(tn.z) )
				tn.y = tn.z = 0;
			else if( std::abs(tn.y) > std::abs(tn.x) && std::abs(tn.y) > std::abs(tn.z) )
				tn.x = tn.z = 0;
			else 
				tn.x = tn.y = 0;

			float fDeltaRotation = (180.0F/BUtil::PI)*value.x;
 			if( tn.y < 0 || tn.x > 0 || tn.z > 0 )
 				fDeltaRotation = -fDeltaRotation;

			texInfo.rotate -= fDeltaRotation;
			pRegion->SetTexInfo(texInfo);
			g_pTextureMappingToolPanel->SetTexInfo(texInfo);
		}
		UpdateShelf(1);
	}
	else if( editMode == eEditModeScale )
	{
		Vec3 vScale(offsetTM.m00,offsetTM.m11,offsetTM.m22);
		for( int i = 0, iCount(m_MouseDownContext.m_TexInfos.size()); i < iCount; ++i )
		{
			CBrushRegion::RegionPtr pRegion(m_MouseDownContext.m_TexInfos[i].first);
			BUtil::STexInfo texInfo(m_MouseDownContext.m_TexInfos[i].second);

			SBrushPlane<float> p(ToVec3(pRegion->GetPlane().Normal()),ToFloat(pRegion->GetPlane().Distance()));
			Vec3 basis_u, basis_v;
			float fBackupRotate = texInfo.rotate;
			texInfo.rotate = 0;
			BUtil::CalcTextureBasis(p, texInfo, basis_u, basis_v);
			texInfo.rotate = fBackupRotate;

			BrushFloat tu = std::abs((BrushFloat)basis_u.Dot(vScale));
			BrushFloat tv = std::abs((BrushFloat)basis_v.Dot(vScale));

			texInfo.scale[0] += tu-1;
			texInfo.scale[1] += tv-1;

			if( texInfo.scale[0] < 0.01f ) texInfo.scale[0] = 0.01f;
			if( texInfo.scale[1] < 0.01f ) texInfo.scale[1] = 0.01f;

			pRegion->SetTexInfo(texInfo);
			g_pTextureMappingToolPanel->SetTexInfo(texInfo);
		}
		UpdateShelf(1);
	}
}

void CBrushDesignerTextureMappingTool::OnManipulatorMouseEvent( CViewport *pView, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo )
{
	m_bHitGizmo = bHitGizmo;
	UpdateCursor(pView,bHitGizmo);

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	std::vector<CBrushRegion::RegionPtr> regions;
	for( int i = 0, iSelectionCount(pSelected->GetSize()); i < iSelectionCount; ++i )
	{
		if( (*pSelected)[i].IsFace() && (*pSelected)[i].m_pRegion )
			regions.push_back((*pSelected)[i].m_pRegion);
	}
	if( regions.empty() )
		return;

	if( event == eMouseLDown )
	{
		m_MouseDownContext.Init();
		m_MouseDownContext.m_MouseDownPos = GetWorldTM().GetInverted().TransformPoint(ToBrushVec3(pManipulator->GetTransformation(COORDS_WORLD).GetTranslation()));
		for( int i = 0, iRegionCount(regions.size()); i < iRegionCount; ++i )
			m_MouseDownContext.m_TexInfos.push_back(std::pair<CBrushRegion::RegionPtr,BUtil::STexInfo>(regions[i],regions[i]->GetTexInfo()));
	}
	
	if( event == eMouseLUp )
	{
		m_MouseDownContext.Init();
	}
}

void CBrushDesignerTextureMappingTool::RecordTextureMappingUndo( const char *sUndoDescription ) const
{
	if( CUndo::IsRecording() && GetBaseObject() )
		CUndo::Record( new CUndoDesignerTextureMapping(GetBaseObject(), sUndoDescription) );
}

void CBrushDesignerTextureMappingTool::AssignMatID( int matID )
{
	CBrushDesignerElementManager* pSelect = GetEditTool()->GetSelectedElements();
	for( int i = 0, iCount(pSelect->GetSize()); i < iCount; ++i )
	{
		if( !(*pSelect)[i].m_pRegion )
			continue;
		(*pSelect)[i].m_pRegion->SetMaterialID(matID);
	}
}