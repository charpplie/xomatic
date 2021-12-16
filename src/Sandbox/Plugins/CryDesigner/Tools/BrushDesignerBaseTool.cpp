#include "StdAfx.h"
#include "BrushDesignerBaseTool.h"
#include "BrushDesignerEditTool.h"
#include "ViewManager.h"
#include "Core/BrushDesigner.h"
#include "ITransformManipulator.h"
#include "Objects/DesignerBrushObject.h"
#include "SurfaceInfoPicker.h"
#include "Core/BaseBrush.h"
#include "Objects/PrefabObject.h"
#include "Objects/AreaSolidObject.h"
#include "Objects/ClipVolumeObject.h"
#include "BrushDesignerSelectTool.h"
#include "BrushDesignerSeparateTool.h"
#include "BrushDesignerResetXFormTool.h"
#include "Core/BrushCommonInterface.h"
#include "Core/BrushDesignerSmoothingGroupManager.h"
#include "IGizmoManager.h"
#include "IBaseToolPanel.h"

_smart_ptr<CBrushRegionMesh> CBrushDesignerBaseTool::m_pSelectionMesh = NULL;

bool CBrushDesignerBaseTool::BinarySearchForScale( BrushFloat fValidScale, BrushFloat fInvalidScale, int nCount, CBrushRegion& region, BrushFloat& fOutScale )
{
	static const int kMaxCount(10);
	static const int kMaxCountToAvoidInfiniteLoop(100);

	BrushFloat fMiddleScale = (fValidScale+fInvalidScale)*0.5f;

	CBrushRegion tempRegion(region);
	if( tempRegion.Scale(fMiddleScale,true) )
	{
		if( nCount >= kMaxCount || std::abs(fValidScale-fMiddleScale) <= (BrushFloat)0.001 )
		{
			fOutScale = fMiddleScale;
			return true;
		}
		return BinarySearchForScale( fMiddleScale, fInvalidScale, nCount+1, region, fOutScale );
	}

	if( nCount >= kMaxCountToAvoidInfiniteLoop )
		return false;

	return BinarySearchForScale( fValidScale, fMiddleScale, nCount+1, region, fOutScale );
}

void CBrushDesignerBaseTool::UpdateSelectionMesh( CBrushRegion::RegionPtr pRegion, CBaseBrush* pBrush, CBaseObject* pObj, bool bForce )
{
	if( m_pSelectionMesh == NULL )
		m_pSelectionMesh = new CBrushRegionMesh;

	int renderFlag = pBrush->GetRenderFlags();
	int viewDist = pBrush->GetViewDistRatio();
	uint32 minSpec = pObj->GetMinSpec();
	uint32 materialLayerMask = pObj->GetMaterialLayersMask();
	BrushMatrix34 worldTM = pObj->GetWorldTM();

	if( pRegion )
	{
		BrushVec3 vDir = worldTM.TransformVector(pRegion->GetPlane().Normal()).GetNormalized();
		worldTM.SetTranslation( worldTM.GetTranslation() + vDir*0.01f );
	}

	m_pSelectionMesh->SetRegion(pRegion, bForce, worldTM, renderFlag, viewDist, minSpec, materialLayerMask );
}

BrushMatrix34 CBrushDesignerBaseTool::GetWorldTM() const
{
	CBaseObject* pBaseObj = GetBaseObject();
	DESIGNER_ASSERT(pBaseObj);
	return pBaseObj->GetWorldTM();
}

void CBrushDesignerBaseTool::Enter()
{
	if( m_pSelectionMesh )
		m_pSelectionMesh->SetRegion(NULL,false);
	ReleaseObjectGizmo();
	BeginEditParams();
	if( GetBaseObject() )
		GetBaseObject()->SetHighlight(false);
}

void CBrushDesignerBaseTool::Leave()
{	
	if( GetDesigner() )
		GetDesigner()->ClearExcludedEdgesInDrawing();
	ReleaseSelectionMesh();
	EndEditParams();
}

void CBrushDesignerBaseTool::ReleaseSelectionMesh()
{
	if( m_pSelectionMesh )
	{
		m_pSelectionMesh->ReleaseResources();
		m_pSelectionMesh = NULL;
	}
}

void CBrushDesignerBaseTool::OnMouseMove( CViewport *view,UINT nFlags,CPoint point )
{
	BrushVec3 localRaySrc, localRayDir;
	BUtil::GetLocalViewRay( GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir );

	int nRegionIndex(0);
	if( GetDesigner()->QueryRegion( localRaySrc, localRayDir, nRegionIndex ) )
	{
		CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(nRegionIndex);
		UpdateSelectionMesh(pRegion,GetBrush(),GetBaseObject());
		m_Plane = pRegion->GetPlane();
	}
	else
	{	
		UpdateSelectionMesh(NULL,GetBrush(),GetBaseObject());
	}
}

void CBrushDesignerBaseTool::Sync()
{
	if( GetDesigner() == NULL )
		return;

	CBaseObject* pObject = GetBaseObject();
	if( pObject == NULL )
		return;

	pObject->UpdateGroup();

	CBaseObject* pSelected = GetIEditor()->GetSelectedObject();
	if( pSelected && pSelected != pObject && pSelected->GetParent() )
	{
		DESIGNER_ASSERT( pSelected->GetParent()->IsKindOf(RUNTIME_CLASS(CPrefabObject)) );
		CPrefabObject* pParent = ((CPrefabObject*)pSelected->GetParent());
		pParent->SetPrefab(pParent->GetPrefab(),true);
	}
}

CBrushDesignerEditTool* CBrushDesignerBaseTool::GetEditTool()
{
	CEditTool* pEditTool = GetIEditor()->GetEditTool();
	if( pEditTool && pEditTool->IsKindOf(RUNTIME_CLASS(CBrushDesignerEditTool)) )
		return (CBrushDesignerEditTool*)pEditTool;
	return NULL;
}

bool CBrushDesignerBaseTool::SelectDesignerObject( CPoint point )
{
	if( !GetDesigner() )
		return false;

	CBaseObject* pBaseObject = GetBaseObject();
	if( pBaseObject && pBaseObject->GetType() != OBJTYPE_SOLID )
		return false;

	std::set<CDesignerBrushObject*> selectedObjectSet;
	if( GetEditTool() )
		selectedObjectSet = GetEditTool()->GetSelectedDesignerObjects();

	CSurfaceInfoPicker picker;
	SRayHitInfo hitInfo;
	if( picker.Pick( point, hitInfo, NULL, CSurfaceInfoPicker::ePOG_DesignerObject ) )
	{
		CBaseObject* pPickedObj = picker.GetPickedObject();

		if( !pPickedObj || GetBaseObject() == pPickedObj )
			return false;

		if( !pPickedObj->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
			return false;

		if( !gSettings.bDesignerSeamlessSelection && selectedObjectSet.size() > 2 && selectedObjectSet.find((CDesignerBrushObject*)pPickedObj) == selectedObjectSet.end() ) 
			return false;

		CDesignerBrushObject* pNewDesignerObj = (CDesignerBrushObject*)pPickedObj;
		if( !pNewDesignerObj->GetBrush() || !pNewDesignerObj->GetDesigner() )
			return false;

		if( GetEditTool() )
		{
			GetEditTool()->SetBaseObject(pNewDesignerObj);
			CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
			pSelected->Clear();
			return true;
		}
	}

	return false;
}

CBaseBrush* CBrushDesignerBaseTool::GetBrush() const
{
	CBrushDesignerEditTool* pEditTool = GetEditTool();
	if( pEditTool == NULL )
		return NULL;
	return pEditTool->GetBrush();
}

CBaseObject* CBrushDesignerBaseTool::GetBaseObject() const
{
	CBrushDesignerEditTool* pEditTool = GetEditTool();
	if( pEditTool == NULL )
		return NULL;
	return pEditTool->GetBaseObject();
}

BUtil::SMainContext CBrushDesignerBaseTool::GetMainContext() const
{
	BUtil::SMainContext mc;
	mc.pObject = GetBaseObject();
	mc.pBrush = GetBrush();
	mc.pDesigner = GetDesigner();
	mc.pSelected = GetEditTool()->GetSelectedElements();
	return mc;

}

void CBrushDesignerBaseTool::UpdateBrush()
{
	if( !GetBrush() || !GetDesigner() )
		return;
	GetBrush()->Update( GetBaseObject(), GetDesigner() );
}

void CBrushDesignerBaseTool::UpdateShelf( int nShelf )
{
	if( !GetBrush() )
		return;
	GetBrush()->Update(GetBaseObject(), GetDesigner(), nShelf );
}

void CBrushDesignerBaseTool::UpdateTMManipulatorBasedOnElements( CBrushDesignerElementManager* elements )
{
	if( elements->IsEmpty() )
		return;
	BrushVec3 averagePos(0,0,0);
	int iResultSize(0);
	for( int i = 0, iListSize(elements->GetSize()); i < iListSize; ++i )
	{
		if( (*elements)[i].IsFace() && (*elements)[i].m_pRegion )
		{
			averagePos += (*elements)[i].m_pRegion->GetRepresentativePosition();
			++iResultSize;
		}
		else
		{
			for( int k = 0, iElementSize((*elements)[i].m_Vertices.size()); k < iElementSize; ++k )
			{
				averagePos += (*elements)[i].m_Vertices[k];
				++iResultSize;
			}
		}
	}
	averagePos /= iResultSize;
	UpdateTMManipulator(averagePos, elements->GetNormal(GetDesigner()));
}

void CBrushDesignerBaseTool::UpdateTMManipulator( const BrushVec3& localPos, const BrushVec3& localNormal )
{
	ITransformManipulator *pManipulator = GetIEditor()->ShowTransformManipulator(true);
	if( pManipulator == NULL )
		return;	

	BrushMatrix34 localOrthogonalTM = Matrix33::CreateOrthogonalBase(localNormal);

	BrushVec3 worldPos = GetWorldTM().TransformPoint(localPos);

	BrushMatrix34 userTM = GetIEditor()->GetViewManager()->GetGrid()->GetMatrix();
	userTM.SetTranslation(worldPos);

	BrushMatrix34 refFrame(GetWorldTM()*localOrthogonalTM);
	refFrame.SetTranslation(worldPos);
	pManipulator->SetTransformation( COORDS_LOCAL, refFrame );
	pManipulator->SetTransformation( COORDS_USERDEFINED, userTM ); 

	CBaseObject* pObj = GetBaseObject();
	if( pObj && pObj->GetParent() )
	{
		BrushMatrix34 parentTM = pObj->GetParent()->GetWorldTM();
		parentTM.SetTranslation(worldPos);
		pManipulator->SetTransformation( COORDS_PARENT, parentTM );
	}
}

BrushMatrix34 CBrushDesignerBaseTool::GetOffsetTM( ITransformManipulator *pManipulator, const BrushVec3& vOffset ) const
{
	RefCoordSys coordSys = GetIEditor()->GetReferenceCoordSys();
	int editMode = GetIEditor()->GetEditMode();

	BrushMatrix34 worldRefTM(ToBrushMatrix34(pManipulator->GetTransformation(coordSys)));
	BrushMatrix34 worldTM = GetWorldTM();
	BrushMatrix34 invWorldTM = worldTM.GetInverted();
	BrushMatrix34 modRefFrame = invWorldTM * worldRefTM;
	BrushMatrix34 modRefFrameInverse = worldRefTM.GetInverted() * worldTM;

	if( editMode == eEditModeMove )
		return modRefFrame * BrushMatrix34::CreateTranslationMat(worldRefTM.GetInverted().TransformVector(vOffset)) * modRefFrameInverse;	
	else if( editMode == eEditModeRotate )
		return modRefFrame * BrushMatrix34::CreateRotationXYZ(Ang3_tpl<BrushFloat>(-vOffset)) * modRefFrameInverse;
	else if( editMode == eEditModeScale )
		return modRefFrame * BrushMatrix34::CreateScale(vOffset) * modRefFrameInverse;

	return BrushMatrix34::CreateIdentity();
}

CBrushDesigner* CBrushDesignerBaseTool::GetDesigner() const
{
	CBrushDesignerEditTool* pEditTool = GetEditTool();
	if( pEditTool == NULL )
		return NULL;
	return pEditTool->GetDesigner();
}

void CBrushDesignerBaseTool::AddMirroredRegion( CBrushDesigner* pDesigner, CBrushRegion::RegionPtr pRegion, int opDesignerType )
{
	if( !pDesigner->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		return;

	CBrushDesigner::EOperationType opType = (CBrushDesigner::EOperationType)opDesignerType;
	CBrushRegion::RegionPtr pMirroredRegion = pRegion->Clone()->Mirror(pDesigner->GetMirrorPlane());
	pDesigner->AddRegion( pMirroredRegion, opType );
}

void CBrushDesignerBaseTool::AddMirroredOpenRegion( CBrushDesigner* pDesigner, CBrushRegion::RegionPtr pRegion, bool bOnlyAdd )
{
	if( !pDesigner->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		return;

	CBrushRegion::RegionPtr pMirroredRegion = pRegion->Clone()->Mirror(pDesigner->GetMirrorPlane());
	pDesigner->AddOpenRegion(pMirroredRegion,bOnlyAdd);
}

void CBrushDesignerBaseTool::RemoveMirroredRegion( CBrushDesigner* pDesigner, CBrushRegion::RegionPtr pRegion )
{
	if( !pDesigner->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		return;

	CBrushRegion::RegionPtr pMirroredRegion = pDesigner->QueryEquivalentRegion(pRegion->Clone()->Mirror(pDesigner->GetMirrorPlane()));

	if( pMirroredRegion == NULL )
		return;

	pDesigner->RemoveRegion(pMirroredRegion);
}

void CBrushDesignerBaseTool::DrillMirroredRegion( CBrushDesigner* pDesigner, CBrushRegion::RegionPtr pRegion, bool bRemainFrame )
{
	if( !pDesigner->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		return;

	CBrushRegion::RegionPtr pMirroredRegion = pRegion->Clone()->Mirror(pDesigner->GetMirrorPlane());
	pDesigner->DrillRegion( pDesigner->QueryEquivalentRegion(pMirroredRegion), bRemainFrame );
}

void CBrushDesignerBaseTool::RemoveRegionWithSpecificFlagsFromList( CBrushDesigner* pDesigner, std::vector<CBrushRegion::RegionPtr>& regionList, int nFlags )
{
	std::vector<CBrushRegion::RegionPtr>::iterator ii = regionList.begin();
	for( ; ii != regionList.end() ; )
	{
		if( (*ii)->CheckFlags(nFlags) )
			ii = regionList.erase(ii);
		else
			++ii;
	}
}

void CBrushDesignerBaseTool::RemoveRegionWithoutSpecificFlagsFromList( CBrushDesigner* pDesigner, std::vector<CBrushRegion::RegionPtr>& regionList, int nFlags )
{
	std::vector<CBrushRegion::RegionPtr>::iterator ii = regionList.begin();
	for( ; ii != regionList.end() ; )
	{
		if( !(*ii)->CheckFlags(nFlags) )
			ii = regionList.erase(ii);
		else
			++ii;
	}
}

void CBrushDesignerBaseTool::CreateMirroredRegions( CBrushDesigner* pDesigner )
{
	if( !pDesigner->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		return;

	pDesigner->RemoveRegionsWithSpecificFlagsPlane(CBrushRegion::eRF_Mirrored);

	for( int i = 0, iRegionSize(pDesigner->GetRegionSize()); i < iRegionSize; ++i )
	{
		CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
		if( pRegion->CheckFlags(CBrushRegion::eRF_Mirrored) )
			continue;
		AddMirroredRegion(pDesigner,pRegion,CBrushDesigner::eOpType_Add);
	}
}

void CBrushDesignerBaseTool::UpdateMirroredPartWithPlane( CBrushDesigner* pDesigner, const BrushPlane& plane )
{
	if( !pDesigner->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		return;

	const BrushPlane& mirroredPlane = pDesigner->GetMirrorPlane().MirrorPlane(plane);
	pDesigner->RemoveRegionsWithSpecificFlagsPlane(CBrushRegion::eRF_Mirrored,&mirroredPlane);

	std::vector<CBrushRegion::RegionPtr> regions;
	if( !pDesigner->QueryRegions(plane,regions) )
		return;

	RemoveRegionWithSpecificFlagsFromList(pDesigner,regions,CBrushRegion::eRF_Mirrored);

	for( int i = 0, iRegionSize(regions.size()); i < iRegionSize; ++i )
	{
		if( regions[i]->IsOpen() )
			AddMirroredOpenRegion(pDesigner,regions[i],true);
		else
			AddMirroredRegion(pDesigner,regions[i],CBrushDesigner::eOpType_Add);
	}
}

void CBrushDesignerBaseTool::EraseMirroredEdge( CBrushDesigner* pDesigner, const BrushEdge3D& edge )
{
	if( !pDesigner->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		return;

	const BrushPlane& mirrorPlane = pDesigner->GetMirrorPlane();
	BrushEdge3D mirroredEdge( mirrorPlane.MirrorVertex(edge.m_v[0]), mirrorPlane.MirrorVertex(edge.m_v[1]) );

	pDesigner->EraseEdge( mirroredEdge );
}

bool CBrushDesignerBaseTool::MakeListConsistingOfArc( const BrushVec2& vOutsideVertex, const BrushVec2& vBaseVertex0, const BrushVec2& vBaseVertex1, int nSegmentCount, std::vector<BrushVec2>& outVertexList )
{
	BrushFloat distToCrossPoint;
	BrushEdge::GetSquaredDistance( BrushEdge(vBaseVertex0,vBaseVertex1), vOutsideVertex, distToCrossPoint );
	if( distToCrossPoint == 0 )
	{
		DESIGNER_ASSERT(0);
		return false;
	}

	BrushFloat circumCircleRadius;
	BrushVec2 circumCenter;
	if( !ComputeCircumradiusAndCircumcenter( vBaseVertex0, vBaseVertex1, vOutsideVertex, &circumCircleRadius, &circumCenter ) )
	{
		DESIGNER_ASSERT(0);
		return false;
	}

	const BrushVec2 vXAxis(1.0f,0.0f);

	BrushVec2 circumCenterToFirstPoint = (vBaseVertex0-circumCenter).GetNormalized();
	BrushVec2 circumCenterToLastPoint = (vBaseVertex1-circumCenter).GetNormalized();

	BrushFloat startRadian = acosf(vXAxis.Dot(circumCenterToFirstPoint));
	BrushFloat endRadian = acosf(vXAxis.Dot(circumCenterToLastPoint));

	BrushLine vXAxisLine(circumCenter,circumCenter+vXAxis);

	bool bFirstPointBetween180To360 = vXAxisLine.Distance(vBaseVertex0) < 0;
	if( bFirstPointBetween180To360 )
		startRadian = -startRadian;

	bool bSecondPointBetween180To360 = vXAxisLine.Distance(vBaseVertex1) < 0;
	if( bSecondPointBetween180To360 )
		endRadian =  -endRadian;

	BrushFloat diffRadian = endRadian - startRadian;
	BrushFloat diffOppositeRadian = diffRadian > 0 ? -(BUtil::PI2-diffRadian) : -(-BUtil::PI2-diffRadian);

	BrushFloat diffTryRadian[2] = { diffRadian, diffOppositeRadian };
	std::vector<BrushVec2> arcTryVertexList[2];
	const int nTryEdgeCount = 3;
	BrushFloat fNearestDistance = 3e10f;
	int nNearerSide(0);

	for( int k = 0; k < 2; ++k )
	{
		BUtil::MakeSectorOfCircle( circumCircleRadius, circumCenter, startRadian, diffTryRadian[k], nTryEdgeCount, arcTryVertexList[k] );

		for( int i = 0, iArcVertexSize(arcTryVertexList[k].size()); i < iArcVertexSize; ++i )
		{
			const BrushVec2& v0 = arcTryVertexList[k][i];
			const BrushVec2& v1 = arcTryVertexList[k][(i+1)%iArcVertexSize];

			BrushFloat fDistance = 3e10f;
			BrushEdge::GetSquaredDistance( BrushEdge(v0,v1), vOutsideVertex, fDistance );
			fDistance = std::abs(fDistance);
			if( fDistance < fNearestDistance )
			{
				nNearerSide = k;
				fNearestDistance = fDistance;
			}
		}
	}

	BUtil::MakeSectorOfCircle( circumCircleRadius, circumCenter, startRadian, diffTryRadian[nNearerSide], nSegmentCount, outVertexList );

	for( int i = 0, iCount(outVertexList.size()); i < iCount; ++i )
	{
		if( !outVertexList[i].IsValid() )
		{
			DESIGNER_ASSERT(0);
			return false;
		}
	}

	return true;
}

template<class T>
bool CBrushDesignerBaseTool::ComputeCircumradiusAndCircumcenter( const T& v0, const T& v1, const T& v2, BrushFloat* outCircumradius, T* outCircumcenter )
{
	BrushFloat dca = (v2-v0).Dot(v1-v0);
	BrushFloat dba = (v2-v1).Dot(v0-v1);
	BrushFloat dcb = (v0-v2).Dot(v1-v2);

	BrushFloat n1 = dba*dcb;
	BrushFloat n2 = dcb*dca;
	BrushFloat n3 = dca*dba;

	BrushFloat n123 = n1+n2+n3;

	if( n123 == 0 )
		return false;

	if( outCircumradius )
		*outCircumradius = sqrt(((dca+dba)*(dba+dcb)*(dcb+dca))/n123)*0.5f;

	if( outCircumcenter )
		*outCircumcenter = (v0*(n2+n3) + v1*(n3+n1) + v2*(n1+n2)) / (2*n123);

	return true;
}

void CBrushDesignerBaseTool::Display( DisplayContext &dc )
{
	if( dc.flags & DISPLAY_2D )
		return;
	DisplayDimensionHelper(dc);
}

bool CBrushDesignerBaseTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if( nChar == VK_ESCAPE )
		GetEditTool()->GoToSelectDesignerMode();
	return true;
}

void CBrushDesignerBaseTool::Separate1stStep()
{
	if( !IsSeparateStatus() || GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		return;

	if( GetBaseObject()->GetType() != OBJTYPE_SOLID )
		return;

	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(1);

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->Clear();
	for( int i = 0, iRegionCount(GetDesigner()->GetRegionSize()) ; i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = GetDesigner()->GetRegion(i);
		SDesignerElement de;
		de.SetFace(GetBaseObject(),pRegion);
		pSelected->Add(de);
	}
}

void CBrushDesignerBaseTool::Separate2ndStep()
{
	if( !IsSeparateStatus() || GetDesigner()->CheckModeFlag(CBrushDesigner::eDesignerMode_Mirror) )
		return;

	if( GetBaseObject()->GetType() != OBJTYPE_SOLID )
		return;

	CDesignerBrushObject* pSparatedObj = CBrushDesignerSeparateTool::Separate(GetMainContext());
	if( pSparatedObj )
		GetEditTool()->SetBaseObject(pSparatedObj);
	CBrushDesignerResetXFormTool::FreezeXForm(GetDesigner(),GetBrush(),GetBaseObject(),BUtil::eResetXForm_All);
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	pSelected->Clear();
}

void CBrushDesignerBaseTool::FreezeDesigner()
{
	if( !GetDesigner() )
		return;

	Separate1stStep();

	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());

	GetDesigner()->SetShelf(0);
	bool bEmptyDesigner = GetDesigner()->GetRegionSize() == 0 ? true : false;

	GetDesigner()->SetShelf(1);
	GetDesigner()->RemoveRegionsWithSpecificFlagsPlane(CBrushRegion::eRF_Mirrored);

	bool bUniqueRegion = GetDesigner()->GetRegionSize() == 1 ? true : false;

	CBrushDesigner::RegionList regions;
	GetDesigner()->GetRegionList(regions);
	CBrushRegion::RegionPtr pTouchingRegion = NULL;

	BrushPlane floorPlane = GetTempRegion() ? GetTempRegion()->GetPlane() : GetPlane();
	BrushPlane invFloorPlane = floorPlane.GetInverted();

	for( int i = 0, iRegionCount(regions.size()); i < iRegionCount; ++i )
	{
		const BrushPlane& plane = regions[i]->GetPlane();
		if( !plane.IsEquivalent(floorPlane,kDesignerEpsilon) && !plane.IsEquivalent(invFloorPlane,kDesignerEpsilon) )
			continue;
		GetDesigner()->RemoveRegion(regions[i]);
		if( plane.IsEquivalent(invFloorPlane,kDesignerEpsilon) )
		{
			pTouchingRegion = regions[i]->Flip();
			break;
		}
	}

	GetDesigner()->MoveShelf(1,0);
	GetDesigner()->SetShelf(0);

	if( pTouchingRegion )
	{
		bool bAdded = false;
		if( !IsSeparateStatus() )
		{
			if( GetTempRegion() && pTouchingRegion->IncludeAllEdges(GetTempRegion()) )
			{
				GetDesigner()->RemoveRegion(GetDesigner()->QueryEquivalentRegion(GetTempRegion()));
				CBrushRegion::RegionPtr pClonedRegion = pTouchingRegion->Clone();
				pClonedRegion->Subtract(GetTempRegion());
				if( pClonedRegion->IsValid() )
					GetDesigner()->AddRegion(pClonedRegion->Flip(),CBrushDesigner::eOpType_Add);
				bAdded = true;
			}
			else if( GetDesigner()->HasIntersection(pTouchingRegion) )
			{
				GetDesigner()->AddRegion(pTouchingRegion,CBrushDesigner::eOpType_ExclusiveOR);
				GetDesigner()->SeparateRegions(pTouchingRegion->GetPlane());
				bAdded = true;
			}
		}
		if( !bAdded )
			GetDesigner()->AddRegion(pTouchingRegion->Flip(),CBrushDesigner::eOpType_Add);
	}

	BrushVec3 vNewPivot = GetDesigner()->GetBoundBox().min;

	CreateMirroredRegions(GetDesigner());

	GetDesigner()->ResetDB(BUtil::eDBRF_ALL);
	GetDesigner()->GetSmoothingGroupMgr()->InvalidateAll();	
	if( !IsSeparateStatus() )
		GetBaseObject()->UpdateGroup();

	if( bEmptyDesigner || (gSettings.bDesignerKeepCenterPivot && !IsSeparateStatus()) )
		GetBrush()->PivotToCenter(GetBaseObject(),GetDesigner());
	else if( IsSeparateStatus() )
		Separate2ndStep();

	if( !IsSeparateStatus() || bEmptyDesigner )
	{
		CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
		pSelected->Clear();
		UpdateBrush();
	}

	UpdateGameResource(GetBaseObject());
}

void CBrushDesignerBaseTool::CancelDesigner()
{
	DESIGNER_SHELF_RECONSTRUCTOR(GetDesigner());
	GetDesigner()->SetShelf(1);
	GetDesigner()->Clear();
	UpdateShelf(1);
}

void CBrushDesignerBaseTool::CreateObjectGizmo()
{
	if( GetIEditor()->GetTransformManipulator() )
		return;
	if( CAxisGizmo::GetGlobalAxisGizmoCount() < gSettings.gizmo.axisGizmoMaxCount)
	{
		m_pObjectGizmo = new CAxisGizmo(GetBaseObject());
		GetIEditor()->GetObjectManager()->GetGizmoManager()->AddGizmo(m_pObjectGizmo);
	}
}

void CBrushDesignerBaseTool::ReleaseObjectGizmo()
{
	GetIEditor()->ShowTransformManipulator(false);

	if( m_pObjectGizmo )
	{
		GetIEditor()->GetObjectManager()->GetGizmoManager()->RemoveGizmo(m_pObjectGizmo);
		m_pObjectGizmo = NULL;
	}

	std::vector<CGizmo*> gizmoList;
	IGizmoManager* pGizmoMgr = GetIEditor()->GetObjectManager()->GetGizmoManager();
	for( int i = 0, iCount(pGizmoMgr->GetGizmoCount()); i < iCount; ++i )
		gizmoList.push_back(pGizmoMgr->GetGizmoByIndex(i));

	for( int i = 0, iCount(gizmoList.size()); i < iCount; ++i )
	{
		if( !gizmoList[i] )
			continue;
		if( gizmoList[i]->GetBaseObject() == GetBaseObject() )
			pGizmoMgr->RemoveGizmo(gizmoList[i]);
	}
}

void CBrushDesignerBaseTool::DisplayDimensionHelper( DisplayContext &dc, const AABB& aabb )
{
	if( gSettings.bDesignerDisplayDimensionHelper )
	{
		Matrix34 poppedTM = dc.GetMatrix();
		dc.PopMatrix();
		GetBaseObject()->DrawDimensionsImpl(dc,aabb);
		dc.PushMatrix(poppedTM);
	}
}

void CBrushDesignerBaseTool::DisplayDimensionHelper( DisplayContext &dc, int nShelf )
{
	AABB aabb = GetDesigner()->GetBoundBox(nShelf);
	if( !aabb.IsReset() )
	{
		SDesignerEnvironmentInfo& globalInfo = CBrushDesignerEditTool::GetGlobalEnvironmentInfo();
		if( gSettings.bDesignerDisplayDimensionHelper )
			DisplayDimensionHelper(dc,aabb);
	}
}

void CBrushDesignerBaseTool::UpdateGameResource( CBaseObject* pObject )
{
	CBrushCommonInterface::UpdateGameResource(pObject);
}

bool CBrushDesignerBaseTool::IsFrameRemainInRemovingFace( CBaseObject* pObject )
{
	if( pObject == NULL )
		return false;

	return pObject->GetType() != OBJTYPE_SOLID ? true : false;
}

int CBrushDesignerBaseTool::GetPanelIndex() const
{
	if( GetEditTool() == NULL )
		return -1;
	return GetEditTool()->GetPanelIndex()-1;
}

bool CBrushDesignerBaseTool::IsDesignerEmpty() const
{
	return GetDesigner() && GetDesigner()->IsEmpty(0) ? true : false;
}

bool CBrushDesignerBaseTool::IsSeparateStatus() const 
{ 
	if( GetBaseObject()->GetType() != OBJTYPE_SOLID )
		return false;
	return m_bSeparatedNewShape; 
}

CString CBrushDesignerBaseTool::GetStatusText() const
{
	CString str;
	int nCount = GetIEditor()->GetSelection()->GetCount();
	if (nCount > 0)
		str.Format( "%d Object(s) Selected",nCount );
	else
		str.Format( "No Selection",nCount );
	return str;
}