#include "Stdafx.h"
#include "BaseBrush.h"
#include "BrushDesigner.h"
#include "Objects/DesignerBrushObject.h"
#include "Tools/BrushDesignerBaseTool.h"
#include "BrushDesignerSmoothingGroupManager.h"
#include "BrushSubdivisionModifier.h"
#include "Material/Material.h"
#include "BrushTriangles.h"

CBaseBrush::CBaseBrush( int nBaseBrushFlag )
{
	for( BUtil::ShelfID shelfID = 0; shelfID < BUtil::kMaxShelfCount; ++shelfID )
	{
		m_pStatObj[shelfID] = NULL;
		m_pRenderNode[shelfID] = NULL;
	}

	m_nBrushFlag = nBaseBrushFlag;
	m_RenderFlags = CheckFlags(eBaseBrushFlag_CastShadow) ? ERF_CASTSHADOWMAPS|ERF_HAS_CASTSHADOWMAPS|ERF_BAKEDSHADOW : 0;
	m_viewDistRatio = 100;
}

CBaseBrush::CBaseBrush( const CBaseBrush& brush )
{
	for( BUtil::ShelfID shelfID = 0; shelfID < BUtil::kMaxShelfCount; ++shelfID )
	{
		m_pStatObj[shelfID] = NULL;
		m_pRenderNode[shelfID] = NULL;
	}
	m_RenderFlags = brush.m_RenderFlags;
	m_viewDistRatio = brush.m_viewDistRatio;
	m_nBrushFlag = brush.m_nBrushFlag;
}

CBaseBrush::~CBaseBrush()
{
	RemoveStatObj();
	DeleteRenderAllNodes();
}

IMaterial* CBaseBrush::GetMaterialFromBaseObj( CBaseObject* pObj )
{
	IMaterial* pMaterial = NULL;
	if( pObj )
	{
		CString materialName(pObj->GetMaterialName());
		CMaterial* pObjMaterial = pObj->GetMaterial();
		if( pObjMaterial )
			pMaterial = pObjMaterial->GetMatInfo();
	}
	return pMaterial;
}

void CBaseBrush::RemoveStatObj()
{
	for( BUtil::ShelfID shelfID = 0; shelfID < BUtil::kMaxShelfCount; ++shelfID )
	{
		if( m_pStatObj[shelfID] )
		{
			m_pStatObj[shelfID]->Release();
			m_pStatObj[shelfID] = NULL;
		}
	}
}

void CBaseBrush::CreateStatObj( int nShelfID ) const
{
	if( !m_pStatObj[nShelfID] )
	{
		m_pStatObj[nShelfID] = GetIEditor()->Get3DEngine()->CreateStatObj();
		m_pStatObj[nShelfID]->AddRef();
	}
}

void CBaseBrush::InvalidateStatObj( IStatObj* pStatObj, bool bPhysics )
{
	if( !pStatObj )
		return;
	IIndexedMesh* pIndexedMesh = pStatObj->GetIndexedMesh();
	if( pIndexedMesh )
	{
		CMesh* pMesh = pIndexedMesh->GetMesh();
		if( !pMesh->m_pIndices )
			bPhysics = false;
	}
	pStatObj->Invalidate(bPhysics);
	int nSubObjCount = pStatObj->GetSubObjectCount();
	for( int i = 0; i < nSubObjCount; ++i )
	{
		IStatObj::SSubObject* pSubObject = pStatObj->GetSubObject(i);
		if( pSubObject && pSubObject->pStatObj )
			pSubObject->pStatObj->Invalidate(bPhysics);
	}
}

void CBaseBrush::Update( CBaseObject* pBaseObject, CBrushDesigner* pDesigner, BUtil::ShelfID shelfID, bool bUpdateOnlyRenderNode )
{
	DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);

	int nSubdivisionLevel = pDesigner->GetSubdivisionLevel();
	if( nSubdivisionLevel == 0 )
	{
		if( shelfID == -1 )
		{
			for( BUtil::ShelfID id = 0; id < BUtil::kMaxShelfCount; ++id )
			{
				pDesigner->SetShelf(id);
				if( !bUpdateOnlyRenderNode )
					UpdateMesh(pBaseObject,pDesigner);
				UpdateRenderNode(pBaseObject,pDesigner);
			}
		}
		else
		{
			pDesigner->SetShelf(shelfID);
			if( !bUpdateOnlyRenderNode )
				UpdateMesh(pBaseObject,pDesigner);
			UpdateRenderNode(pBaseObject,pDesigner);
		}
		pDesigner->SetSubdivisionResult(NULL);
	}
	else
	{
		if( !bUpdateOnlyRenderNode )
		{
			RemoveStatObj();

			if(!m_pStatObj[0] )
				CreateStatObj(0);

			int nTessFactor = pDesigner->GetTessFactor();

			CBrushSubdivisionModifier subdivisionModifier;
			BUtil::SSubdivisionContext sc = subdivisionModifier.CreateSubdividedMesh(pDesigner,nSubdivisionLevel,nTessFactor);

			bool bDisplayBackface = pDesigner->GetModeFlag()&CBrushDesigner::eDesignerMode_DisplayBackFace;

			IMaterial* pMaterial = GetMaterialFromBaseObj(pBaseObject);			

			std::vector<BUtil::SMeshInfo> meshes;
			sc.fullPatches->CreateMeshFaces(meshes,bDisplayBackface);

			pDesigner->SetSubdivisionResult(sc.fullPatches);

 			if( meshes.size() == 1 )
 			{
 				IIndexedMesh* pMesh = m_pStatObj[0]->GetIndexedMesh();
 				BUtil::FillMesh(meshes[0],pMesh);
				m_pStatObj[0]->SetMaterial(pMaterial);
 				OptimizeMesh(pMesh);
 				InvalidateStatObj(m_pStatObj[0],CheckFlags(eBaseBrushFlag_Physicalize));
 			}
			else
			{
				for( int i = 0, iSubObjCount(meshes.size()); i < iSubObjCount; ++i )
					m_pStatObj[0]->AddSubObject(GetIEditor()->Get3DEngine()->CreateStatObj());

				for( int i = 0, iSubObjCount(m_pStatObj[0]->GetSubObjectCount()); i < iSubObjCount; ++i )
				{
					IStatObj* pSubObj = m_pStatObj[0]->GetSubObject(i)->pStatObj;
					pSubObj->SetMaterial(pMaterial);
					IIndexedMesh* pMesh = pSubObj->GetIndexedMesh();
					BUtil::FillMesh(meshes[i],pMesh);
					OptimizeMesh(pMesh);
					InvalidateStatObj(pSubObj,CheckFlags(eBaseBrushFlag_Physicalize));
					pSubObj->m_eStreamingStatus = ecss_Ready;
				}

				AABB aabb = pDesigner->GetBoundBox();
				m_pStatObj[0]->SetBBoxMin(aabb.min);
				m_pStatObj[0]->SetBBoxMax(aabb.max);
			}
		}
		UpdateRenderNode(pBaseObject,pDesigner);
	}
}

bool CBaseBrush::UpdateMesh( CBaseObject* pBaseObject, CBrushDesigner* pDesigner )
{
	if( pDesigner == NULL )
		return false;

	BUtil::ShelfID shelfID = pDesigner->GetShelf();
	DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);

	pDesigner->SetShelf(shelfID);
	if( !pDesigner->HasClosedRegion(shelfID) )
	{
		DeleteRenderNode(shelfID);
		if( m_pStatObj[shelfID] )
		{
			m_pStatObj[shelfID]->Release();
			m_pStatObj[shelfID] = NULL;
		}
		return true;
	}

	RemoveStatObj();

	if(!m_pStatObj[shelfID] )
		CreateStatObj(shelfID);

	IMaterial* pMaterial = GetMaterialFromBaseObj(pBaseObject);	

	int prevSubObjCount = m_pStatObj[shelfID]->GetSubObjectCount();
	bool bDisplayBackface = pDesigner->GetModeFlag()&CBrushDesigner::eDesignerMode_DisplayBackFace;

	std::vector<IStatObj*> statObjList;
	std::vector<bool> generateBackFaces;
	if( bDisplayBackface )
	{
		if( prevSubObjCount == 0 )
		{
			m_pStatObj[shelfID]->AddSubObject(GetIEditor()->Get3DEngine()->CreateStatObj());
			m_pStatObj[shelfID]->AddSubObject(GetIEditor()->Get3DEngine()->CreateStatObj());
		}
		statObjList.push_back(m_pStatObj[shelfID]->GetSubObject(0)->pStatObj);
		generateBackFaces.push_back(false);		
		statObjList.push_back(m_pStatObj[shelfID]->GetSubObject(1)->pStatObj);
		generateBackFaces.push_back(true);
	}
	else
	{
		if( prevSubObjCount > 0 )
		{
			m_pStatObj[shelfID]->GetIndexedMesh()->SetSubSetCount(0);
			m_pStatObj[shelfID]->SetSubObjectCount(0);
		}
		statObjList.push_back(m_pStatObj[shelfID]);
		generateBackFaces.push_back(false);
	}

	for( int i = 0, iSize(statObjList.size()); i < iSize; ++i )
	{
		IIndexedMesh *pMesh = statObjList[i]->GetIndexedMesh();
		assert(pMesh);

		statObjList[i]->SetMaterial(pMaterial);
		
		bool bSuccessGenerateMesh = GenerateIndexMesh(pDesigner,pMesh,generateBackFaces[i]);
		if( !bSuccessGenerateMesh )
			continue;

		OptimizeMesh(pMesh);
		InvalidateStatObj(statObjList[i],CheckFlags(eBaseBrushFlag_Physicalize));
		statObjList[i]->m_eStreamingStatus = ecss_Ready;
	}

	int nStatObjFlag = m_pStatObj[shelfID]->GetFlags();
	if( m_pStatObj[shelfID]->GetSubObjectCount() > 0 )
	{
		AABB aabb = pDesigner->GetBoundBox();
		m_pStatObj[shelfID]->SetBBoxMin(aabb.min);
		m_pStatObj[shelfID]->SetBBoxMax(aabb.max);
		m_pStatObj[shelfID]->SetFlags(nStatObjFlag|STATIC_OBJECT_COMPOUND);
		m_pStatObj[shelfID]->m_eStreamingStatus = ecss_Ready;
	}
	else
	{
		m_pStatObj[shelfID]->SetFlags(nStatObjFlag&(~STATIC_OBJECT_COMPOUND));
	}

	return true;
}

bool CBaseBrush::GenerateIndexMesh(CBrushDesigner* pDesigner, IIndexedMesh *pMesh, bool bGenerateBackFaces)
{
	if( !pDesigner )
		return false;

	BUtil::ShelfID shelfID = pDesigner->GetShelf();
	DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);

	CBrushDesignerSmoothingGroupManager* pSmoothingGroupMgr = pDesigner->GetSmoothingGroupMgr();

	pDesigner->SetShelf(shelfID);
	std::vector<CBrushRegion::RegionPtr> regionList;
	for( int i = 0, iRegionCount(pDesigner->GetRegionSize()); i < iRegionCount; ++i )
	{
		CBrushRegion::RegionPtr pRegion = pDesigner->GetRegion(i);
		if( pSmoothingGroupMgr->GetSmoothingGroupID(pRegion) == -1 )
			regionList.push_back(pRegion);
	}

	BUtil::SMeshInfo mesh;
	mesh.Reserve(1000);

	for( int i = 0, iSize(regionList.size()); i < iSize; ++i )
	{
		if( regionList[i]->CheckFlags(CBrushRegion::eRF_Hidden) )
			continue;
		BUtil::JoinTwoMeshes(mesh, regionList[i]->GetTriangles(bGenerateBackFaces)->GetMesh());
	}

	std::vector<DesignerSmoothingGroupPtr> smoothingGroupList = pSmoothingGroupMgr->GetSmoothingGroupList();
	for( int i = 0, iSmoothingGroupCount(smoothingGroupList.size()); i < iSmoothingGroupCount; ++i )
	{
		DesignerSmoothingGroupPtr pSmoothingGroup = smoothingGroupList[i];
		BUtil::JoinTwoMeshes( mesh, pSmoothingGroup->GetMeshInfo(bGenerateBackFaces) );
	}

	if( mesh.IsValid() )
		BUtil::FillMesh( mesh, pMesh );
	else
	{
		pMesh->FreeStreams();
		return false;
	}	

	return true;
}

void CBaseBrush::OutputMeshInfo( IIndexedMesh* pMesh )
{
	IIndexedMesh::SMeshDescription desc;
	pMesh->GetMeshDescription(desc);

	CString buffer;
	buffer.Format("Vertex Count : %d\n", desc.m_nVertCount);
	OutputDebugString(buffer);

	buffer.Format("Face Count : %d\n", desc.m_nFaceCount);
	OutputDebugString(buffer);

	buffer.Format("Index Count : %d\n", desc.m_nIndexCount);
	OutputDebugString(buffer);

	buffer.Format("TexCoord Count : %d\n", desc.m_nCoorCount);
	OutputDebugString(buffer);

	buffer.Format("Subset Count %d\n", pMesh->GetSubSetCount() );
	OutputDebugString(buffer);

	OutputDebugString("-Face List-\n");
	for( int i = 0; i < desc.m_nFaceCount; ++i )
	{
		buffer.Format("%d Face : (%d,%d,%d),(%d)\n", i, desc.m_pFaces[i].v[0], desc.m_pFaces[i].v[1], desc.m_pFaces[i].v[2], desc.m_pFaces[i].nSubset );
		OutputDebugString(buffer);
	}

	OutputDebugString("-Vertex List-\n");
	for( int i = 0; i < desc.m_nVertCount; ++i )
	{
		buffer.Format("%d Vertex : (%f,%f,%f)\n", i, desc.m_pVerts[i].x, desc.m_pVerts[i].y, desc.m_pVerts[i].z );
		OutputDebugString(buffer);
	}

	OutputDebugString("-Index List-\n");
	if( desc.m_pIndices )
	{
		for( int i = 0; i < desc.m_nIndexCount; i += 3 )
		{
			buffer.Format("%d Index : (%d,%d,%d)\n", i, desc.m_pIndices[i], desc.m_pIndices[i+1], desc.m_pIndices[i+2] );
			OutputDebugString(buffer);
		}
	}

	OutputDebugString("-Tex List-\n");
	for( int i = 0; i < desc.m_nCoorCount; ++i )
	{
		buffer.Format("%d TexCoord : (%f,%f)\n", i, desc.m_pTexCoord[i].s, desc.m_pTexCoord[i].t );
		OutputDebugString(buffer);
	}

	OutputDebugString("-Normal List-\n");
	for( int i = 0; i < desc.m_nVertCount; ++i )
	{
		buffer.Format("%d Normal : (%f,%f,%f)\n", i, desc.m_pNorms[i].x, desc.m_pNorms[i].y, desc.m_pNorms[i].z );
		OutputDebugString(buffer);
	}

	OutputDebugString("-Subset List-\n");
	for( int i = 0; i < pMesh->GetSubSetCount(); ++i )
	{
		const SMeshSubset& subset = pMesh->GetSubSet(i);

		buffer.Format("%d vCenter : %f,%f,%f\n", i, subset.vCenter.x, subset.vCenter.y, subset.vCenter.z );
		OutputDebugString(buffer);

		buffer.Format("%d fRadius : %f\n",i, subset.fRadius);
		OutputDebugString(buffer);

		buffer.Format("%d fTexelDensity : %f\n", i, subset.fTexelDensity);
		OutputDebugString(buffer);

		buffer.Format("%d nFirstIndexId,nNumIndices : %d,%d\n", i, subset.nFirstIndexId, subset.nNumIndices );
		OutputDebugString(buffer);

		buffer.Format("%d nFirstVertId, nNumVerts : %d,%d\n", i, subset.nFirstVertId, subset.nNumVerts );
		OutputDebugString(buffer);

		buffer.Format("%d nMatID : %d\n",i, subset.nMatID);
		OutputDebugString(buffer);

		buffer.Format("%d nMatFlag : %x\n", i, subset.nMatFlags);
		OutputDebugString(buffer);

		buffer.Format("%d nPhysicalizeType : %d\n", i, subset.nPhysicalizeType);
		OutputDebugString(buffer);
	}
}

void CBaseBrush::OptimizeMesh(IIndexedMesh* pMesh)
{
	DESIGNER_ASSERT(pMesh);
	pMesh->Optimize(); 
}

bool CBaseBrush::HitTest( CBaseObject* pBaseObject, CBrushDesigner* pDesigner, HitContext &hit ) const
{
	if( !pDesigner || !pBaseObject )
		return false;

	DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);
	pDesigner->SetShelf(0);

	BrushVec3 outPosition;
	BrushVec3 localRaySrc, localRayDir;
	const Matrix34& worldTM(pBaseObject->GetWorldTM());
	BUtil::GetLocalViewRay( worldTM, hit.view, hit.point2d, localRaySrc, localRayDir );
	if( pDesigner->QueryPosition( localRaySrc, localRayDir, outPosition ) )
	{
		BrushVec3 worldRaySrc = worldTM.TransformPoint(localRaySrc);
		BrushVec3 worldHitPos = worldTM.TransformPoint(outPosition);
		hit.dist = (float)worldRaySrc.GetDistance(worldHitPos);
		return true;
	}
	return false;
}

void CBaseBrush::SetRenderFlags( int nRenderFlag )
{
	for( BUtil::ShelfID shelfID = 0; shelfID < BUtil::kMaxShelfCount; ++shelfID )
	{
		m_RenderFlags = nRenderFlag;
		if( m_pRenderNode[shelfID] )
			m_pRenderNode[shelfID]->SetRndFlags(m_RenderFlags);
	}
}

void CBaseBrush::SetStaticObjFlags( int nStaticObjFlag )
{
	if( !m_pStatObj[0] )
		CreateStatObj(0);
	m_pStatObj[0]->SetFlags(nStaticObjFlag);
}

int CBaseBrush::GetStaticObjFlags() const
{
	if( !m_pStatObj[0] )
		CreateStatObj(0);
	return m_pStatObj[0]->GetFlags();
}

void CBaseBrush::DisplayTriangulation( CBaseObject* pBaseObject, CBrushDesigner* pDesigner, DisplayContext& dc )
{
	DESIGNER_SHELF_RECONSTRUCTOR(pDesigner);

	for( BUtil::ShelfID shelfID = 0; shelfID < BUtil::kMaxShelfCount; ++shelfID )
	{
		pDesigner->SetShelf(shelfID);

		if( !pDesigner->HasClosedRegion(shelfID) )
			continue;

		if( !m_pStatObj[shelfID] )
		{
			if( !UpdateMesh(pBaseObject,pDesigner) )
				continue; 
		}		

		IIndexedMesh* pMesh;
		if( m_pStatObj[shelfID]->GetSubObjectCount() == 2 )
			pMesh = m_pStatObj[shelfID]->GetSubObject(0)->pStatObj->GetIndexedMesh();
		else
			pMesh = m_pStatObj[shelfID]->GetIndexedMesh();

		if( pMesh == NULL )
			return;

		IIndexedMesh::SMeshDescription meshDesc;
		pMesh->GetMeshDescription(meshDesc);

		dc.SetColor(Vec3(0.5f,0.5f,0.5f));
		float oldLineWidth = dc.GetLineWidth();
		dc.SetLineWidth(1.0f);

		if( meshDesc.m_pFaces )
		{
			for( int i = 0, iFaceCount(meshDesc.m_nFaceCount); i < iFaceCount; ++i )
			{
				Vec3 v[3] = { meshDesc.m_pVerts[meshDesc.m_pFaces[i].v[0]],
					meshDesc.m_pVerts[meshDesc.m_pFaces[i].v[1]],
					meshDesc.m_pVerts[meshDesc.m_pFaces[i].v[2]] };
				dc.DrawPolyLine(v, 3);
			}
		}
		else if( meshDesc.m_pIndices )
		{
			for( int i = 0, iIndexCount(meshDesc.m_nIndexCount); i < iIndexCount; i+=3 )
			{
				Vec3 v[3] = { meshDesc.m_pVerts[meshDesc.m_pIndices[i]],
					meshDesc.m_pVerts[meshDesc.m_pIndices[i+1]],
					meshDesc.m_pVerts[meshDesc.m_pIndices[i+2]] };
				dc.DrawPolyLine(v, 3);
			}
		}

		dc.SetLineWidth(oldLineWidth);
	}
}

void CBaseBrush::DeleteRenderAllNodes()
{
	for( BUtil::ShelfID shelfID = 0; shelfID < BUtil::kMaxShelfCount; ++shelfID )	
		DeleteRenderNode(shelfID);
}

void CBaseBrush::DeleteRenderNode( BUtil::ShelfID shelfID )
{
	if( m_pRenderNode[shelfID] )
	{
		GetIEditor()->Get3DEngine()->DeleteRenderNode(m_pRenderNode[shelfID]);
		m_pRenderNode[shelfID] = NULL;
	}
}

void CBaseBrush::UpdateRenderNode( CBaseObject* pBaseObject, CBrushDesigner* pDesigner )
{
	BUtil::ShelfID shelfID = pDesigner->GetShelf();

	if( m_pStatObj[shelfID] == NULL )
	{
		DeleteRenderNode(shelfID);
		return;
	}

	if( !pDesigner->GetRegionSize() )
		return;	

	if( !m_pRenderNode[shelfID] )
	{
		m_pRenderNode[shelfID] = GetIEditor()->Get3DEngine()->CreateRenderNode( eERType_Brush );
		if( pBaseObject )
			m_pRenderNode[shelfID]->SetEditorObjectId( pBaseObject->GetId().Data1 );
	}	

	if( gSettings.viewports.bHighlightSelectedGeometry)
		m_RenderFlags |= ERF_SELECTED;
	else
		m_RenderFlags &= ~ERF_SELECTED;

	m_pRenderNode[shelfID]->SetRndFlags( m_RenderFlags );
	m_pRenderNode[shelfID]->SetViewDistRatio( m_viewDistRatio );
	m_pRenderNode[shelfID]->SetMinSpec( pBaseObject->GetMinSpec() );
	m_pRenderNode[shelfID]->SetMaterialLayers( pBaseObject->GetMaterialLayersMask() );	

	if( pBaseObject && pBaseObject->IsKindOf( RUNTIME_CLASS(CDesignerBrushObject) ) )
		m_pStatObj[shelfID]->SetFilePath( ((CDesignerBrushObject*)pBaseObject)->GenerateGameFilename() );

	Matrix34A mtx = pBaseObject->GetWorldTM();

	m_pStatObj[shelfID]->SetMaterial(GetMaterialFromBaseObj(pBaseObject));
	m_pRenderNode[shelfID]->SetEntityStatObj( 0,m_pStatObj[shelfID],&mtx );
	m_pRenderNode[shelfID]->SetMaterial(m_pStatObj[shelfID]->GetMaterial());

	m_RenderFlags = m_pRenderNode[shelfID]->GetRndFlags();

	if( m_pRenderNode[shelfID] )
	{
		if( CheckFlags(eBaseBrushFlag_Physicalize) )
			m_pRenderNode[shelfID]->Physicalize();
		else
			m_pRenderNode[shelfID]->Dephysicalize();
	}
}

bool CBaseBrush::GetIStatObj( _smart_ptr<IStatObj>* pOutStatObj )
{
	if( !m_pStatObj[0] )
		return false;

	if( pOutStatObj )
		*pOutStatObj = m_pStatObj[0];

	return IsValid();
}

void CBaseBrush::PivotToCenter( CBaseObject* pObject, CBrushDesigner* pDesigner )
{
	AABB boundbox;
	pObject->GetBoundBox(boundbox);
	BrushVec3 vBottomCenter = BrushVec3(boundbox.GetCenter().x,boundbox.GetCenter().y,boundbox.min.z);
	BrushVec3 vPos = pObject->GetWorldPos();
	BrushVec3 vDiff = pObject->GetWorldTM().GetInverted().TransformVector(vPos-vBottomCenter);

	pDesigner->Move(vDiff);

	pObject->SetWorldPos(vPos-pObject->GetWorldTM().TransformVector(vDiff));
}

void CBaseBrush::PivotToPos( CBaseObject* pObject, CBrushDesigner* pDesigner, const BrushVec3& vPivot )
{
	BrushVec3 vNewPivot = pObject->GetWorldTM().TransformPoint(vPivot);
	BrushVec3 vPos = pObject->GetWorldPos();
	BrushVec3 vDiff = pObject->GetWorldTM().GetInverted().TransformVector(vPos-vNewPivot);
	pDesigner->Move(vDiff);
	pObject->SetWorldPos(vPos-pObject->GetWorldTM().TransformVector(vDiff));
}

void CBaseBrush::SaveToCgf( const char* filename )
{
	_smart_ptr<IStatObj> pObj;
	if( GetIStatObj(&pObj) )
		pObj->SaveToCGF( filename, NULL, true );
}

void CBaseBrush::ResetXForm( CBaseObject* pBaseObject, CBrushDesigner* pDesigner, int nResetFlag )
{
	if( pDesigner == NULL || pBaseObject == NULL )
		return;	

	Matrix34 worldTM;
	worldTM.SetIdentity();

	if( (nResetFlag & BUtil::eResetXForm_Rotation) && (nResetFlag & BUtil::eResetXForm_Scale) )
	{
		Matrix34 brushTM = pBaseObject->GetWorldTM();
		brushTM.SetTranslation(BrushVec3(0,0,0));
		pDesigner->Transform(ToBrushMatrix34(brushTM));
	}
	else if( nResetFlag & BUtil::eResetXForm_Rotation )
	{
		Matrix34 rotationTM = Matrix34(pBaseObject->GetRotation());
		worldTM = Matrix34::CreateScale(pBaseObject->GetScale());
		pDesigner->Transform(ToBrushMatrix34(rotationTM));
	}
	else if( nResetFlag & BUtil::eResetXForm_Scale )
	{
		Matrix34 scaleTM = Matrix34::CreateScale(pBaseObject->GetScale());
		worldTM = Matrix34(pBaseObject->GetRotation());
		pDesigner->Transform(ToBrushMatrix34(scaleTM));
	}

	worldTM.SetTranslation(pBaseObject->GetWorldPos());
	pBaseObject->SetWorldTM(worldTM);

	if( nResetFlag & BUtil::eResetXForm_Position )
		PivotToCenter(pBaseObject, pDesigner);
}

void CBaseBrush::SaveMesh( CArchive& ar, CBaseObject* pObj, CBrushDesigner* pDesigner )
{
	if( !m_pStatObj[0] )
		return;

	int nSubObjectCount = m_pStatObj[0]->GetSubObjectCount();

	std::vector<IStatObj*> pStatObjs;
	if( nSubObjectCount == 0 )
	{
		pStatObjs.push_back(m_pStatObj[0]);
	}
	else if( nSubObjectCount > 0 )
	{
		for( int i = 0; i < nSubObjectCount; ++i )
			pStatObjs.push_back(m_pStatObj[0]->GetSubObject(i)->pStatObj);
	}

	ar.Write( &nSubObjectCount, sizeof(nSubObjectCount) );

	for( int k = 0, iStatObjCount(pStatObjs.size()); k < iStatObjCount; ++k )
	{
		IIndexedMesh *pMesh = pStatObjs[k]->GetIndexedMesh();
		if( !pMesh )
			return;

		int nPositionCount = pMesh->GetVertexCount();
		Vec3* const positions = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::POSITIONS);
		Vec3* const normals = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::NORMALS);

		int nTexCoordCount = pMesh->GetTexCoordCount();
		SMeshTexCoord* const texcoords = pMesh->GetMesh()->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);

		int nFaceCount = pMesh->GetFaceCount();
		SMeshFace* faces = pMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);
		if( nFaceCount == 0 || faces == NULL )
		{
			pMesh->RestoreFacesFromIndices();
			faces = pMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);
			nFaceCount = pMesh->GetFaceCount();
		}

		int nIndexCount = pMesh->GetIndexCount();
		vtx_idx* const indices = pMesh->GetMesh()->GetStreamPtr<vtx_idx>(CMesh::INDICES);

		int nSubsetCount = pMesh->GetSubSetCount();

		ar.Write( &nPositionCount, sizeof(int) );
		ar.Write( &nTexCoordCount, sizeof(int) );
		ar.Write( &nFaceCount, sizeof(int) );
		ar.Write( &nSubsetCount, sizeof(int) );

		ar.Write( positions, sizeof(Vec3)*nPositionCount );
		ar.Write( normals, sizeof(Vec3)*nPositionCount );
		ar.Write( texcoords, sizeof(SMeshTexCoord)*nTexCoordCount );

		for( int i = 0; i < nSubsetCount; ++i )
		{
			const SMeshSubset& subset = pMesh->GetSubSet(i);
			for( int k = 0; k < subset.nNumIndices/3; ++k )
				faces[(subset.nFirstIndexId/3)+k].nSubset = i;
		}
		ar.Write( faces, sizeof(SMeshFace)*nFaceCount );

		for( int i = 0; i < nSubsetCount; ++i )
		{
			const SMeshSubset& subset = pMesh->GetSubSet(i);
			ar.Write( &subset.vCenter, sizeof(Vec3) );
			ar.Write( &subset.fRadius, sizeof(float) );
			ar.Write( &subset.fTexelDensity, sizeof(float) );
			ar.Write( &subset.nFirstIndexId, sizeof(int) );
			ar.Write( &subset.nNumIndices, sizeof(int) );
			ar.Write( &subset.nFirstVertId, sizeof(int) );
			ar.Write( &subset.nNumVerts, sizeof(int) );
			ar.Write( &subset.nMatID, sizeof(int) );
			ar.Write( &subset.nMatFlags, sizeof(int) );
			ar.Write( &subset.nPhysicalizeType, sizeof(int) );
		}
	}
}

bool CBaseBrush::LoadMesh( CArchive& ar, CBaseObject* pObj, CBrushDesigner* pDesigner )
{
	int nSubObjectCount = 0;
	ar.Read( &nSubObjectCount, sizeof(nSubObjectCount) );

	if(!m_pStatObj[0] )
	{
		m_pStatObj[0] = GetIEditor()->Get3DEngine()->CreateStatObj();
		m_pStatObj[0]->AddRef();
	}

	std::vector<IStatObj*> statObjList;
	if( nSubObjectCount == 2 )
	{
		m_pStatObj[0]->AddSubObject(GetIEditor()->Get3DEngine()->CreateStatObj());
		m_pStatObj[0]->AddSubObject(GetIEditor()->Get3DEngine()->CreateStatObj());
		m_pStatObj[0]->GetIndexedMesh()->FreeStreams();
		statObjList.push_back(m_pStatObj[0]->GetSubObject(0)->pStatObj);
		statObjList.push_back(m_pStatObj[0]->GetSubObject(1)->pStatObj);
	}
	else
	{
		statObjList.push_back(m_pStatObj[0]);
	}

	IMaterial* pMaterial = GetMaterialFromBaseObj(pObj);
	m_pStatObj[0]->SetMaterial(pMaterial);

	for( int k = 0, iCount(statObjList.size()); k < iCount; ++k )
	{
		int nPositionCount = 0;
		int nTexCoordCount = 0;
		int nFaceCount = 0;
		ar.Read( &nPositionCount, sizeof(int) );
		ar.Read( &nTexCoordCount, sizeof(int) );
		ar.Read( &nFaceCount, sizeof(int) );
		if( nPositionCount <= 0 || nTexCoordCount <= 0 || nFaceCount <= 0 )
		{
			assert(0);
			return false;
		}

		int nSubsetCount = 0;
		ar.Read( &nSubsetCount, sizeof(int) );

		IIndexedMesh *pMesh = statObjList[k]->GetIndexedMesh();
		if( !pMesh )
		{
			assert(0);
			return false;
		}

		pMesh->FreeStreams();
		pMesh->SetVertexCount(nPositionCount);
		pMesh->SetFaceCount(nFaceCount);
		pMesh->SetIndexCount(0);
		pMesh->SetTexCoordCount(nTexCoordCount);

		Vec3* const positions = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::POSITIONS);
		Vec3* const normals = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::NORMALS);
		SMeshTexCoord* const texcoords = pMesh->GetMesh()->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);
		SMeshFace* const faces = pMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);

		ar.Read( positions, sizeof(Vec3)*nPositionCount );
		ar.Read( normals, sizeof(Vec3)*nPositionCount );
		ar.Read( texcoords, sizeof(SMeshTexCoord)*nTexCoordCount );
		ar.Read( faces, sizeof(SMeshFace)*nFaceCount );

		pMesh->SetSubSetCount(nSubsetCount);
		for( int i = 0; i < nSubsetCount; ++i )
		{
			SMeshSubset subset;
			ar.Read( &subset.vCenter, sizeof(Vec3) );
			ar.Read( &subset.fRadius, sizeof(float) );
			ar.Read( &subset.fTexelDensity, sizeof(float) );
			ar.Read( &subset.nFirstIndexId, sizeof(int) );
			ar.Read( &subset.nNumIndices, sizeof(int) );
			ar.Read( &subset.nFirstVertId, sizeof(int) );
			ar.Read( &subset.nNumVerts, sizeof(int) );
			ar.Read( &subset.nMatID, sizeof(int) );
			ar.Read( &subset.nMatFlags, sizeof(int) );
			ar.Read( &subset.nPhysicalizeType, sizeof(int) );

			pMesh->SetSubsetBounds( i, subset.vCenter, subset.fRadius );
			pMesh->SetSubsetIndexVertexRanges( i, subset.nFirstIndexId, subset.nNumIndices, subset.nFirstVertId, subset.nNumVerts );
			pMesh->SetSubsetMaterialId( i, subset.nMatID );
			pMesh->SetSubsetMaterialProperties( i, subset.nMatFlags, subset.nPhysicalizeType );
		}

		OptimizeMesh(pMesh);

		statObjList[k]->SetMaterial(pMaterial);
		InvalidateStatObj(statObjList[k],CheckFlags(eBaseBrushFlag_Physicalize));
	}

	AABB aabb = pDesigner->GetBoundBox(0);
	m_pStatObj[0]->SetBBoxMin(aabb.min);
	m_pStatObj[0]->SetBBoxMax(aabb.max);

	UpdateRenderNode(pObj,pDesigner);

	return true;
}

bool CBaseBrush::SaveMesh( int nVersion, std::vector<char>& buffer, CBaseObject* pObj, CBrushDesigner* pDesigner )
{
	if( !m_pStatObj[0] )
		return false;

	int32 nSubObjectCount = (int32)m_pStatObj[0]->GetSubObjectCount();

	std::vector<IStatObj*> pStatObjs;
	if( nSubObjectCount == 0 )
	{
		pStatObjs.push_back(m_pStatObj[0]);
	}
	else if( nSubObjectCount > 0 )
	{
		for( int i = 0; i < nSubObjectCount; ++i )
			pStatObjs.push_back(m_pStatObj[0]->GetSubObject(i)->pStatObj);
	}

	BUtil::Write2Buffer(buffer, &nSubObjectCount, sizeof(int32) );

	if( nVersion == 2 )
	{
		int32 nStaticObjFlags = (int32)m_pStatObj[0]->GetFlags();
		BUtil::Write2Buffer(buffer, &nStaticObjFlags, sizeof(int32) );
	}

	for( int k = 0, iStatObjCount(pStatObjs.size()); k < iStatObjCount; ++k )
	{
		IIndexedMesh *pMesh = pStatObjs[k]->GetIndexedMesh();
		if( !pMesh )
			return false;
		int nIndexCount = pMesh->GetIndexCount();
		vtx_idx* const indices = pMesh->GetMesh()->GetStreamPtr<vtx_idx>(CMesh::INDICES);
		if( nIndexCount == 0 || indices == 0 )
		{
			pMesh->Optimize();
			break;
		}
	}

	for( int k = 0, iStatObjCount(pStatObjs.size()); k < iStatObjCount; ++k )
	{
		IIndexedMesh *pMesh = pStatObjs[k]->GetIndexedMesh();
		if( !pMesh )
			return false;

		int32 nPositionCount = (int32)pMesh->GetVertexCount();
		int32 nTexCoordCount = (int32)pMesh->GetTexCoordCount();
		int32 nFaceCount = (int32)pMesh->GetFaceCount();
		int32 nTangentCount = (int32)pMesh->GetTangentCount();
		int32 nSubsetCount = (int32)pMesh->GetSubSetCount();
		int32 nIndexCount = (int32)pMesh->GetIndexCount();

		BUtil::Write2Buffer(buffer, &nPositionCount, sizeof(int32) );
		BUtil::Write2Buffer(buffer, &nTexCoordCount, sizeof(int32) );
		if( nVersion == 0 )
		{
			BUtil::Write2Buffer(buffer, &nFaceCount, sizeof(int32) );
		}
		else if( nVersion >= 1 )
		{
			if( sizeof(vtx_idx) == sizeof(uint16) && nIndexCount >= 0xFFFFFFFF )			
				return false;
			BUtil::Write2Buffer(buffer, &nIndexCount, sizeof(int32) );
			BUtil::Write2Buffer(buffer, &nTangentCount, sizeof(int32) );
		}
		BUtil::Write2Buffer(buffer, &nSubsetCount, sizeof(int32) );

		Vec3* const positions = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::POSITIONS);
		Vec3* const normals = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::NORMALS);		
		SMeshTexCoord* const texcoords = pMesh->GetMesh()->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);		
		SMeshFace* faces = pMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);	
		if( nVersion == 0 && (nFaceCount == 0 || faces == NULL) )
		{
			pMesh->RestoreFacesFromIndices();
			faces = pMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);
			nFaceCount = pMesh->GetFaceCount();
		}		
		vtx_idx* const indices = pMesh->GetMesh()->GetStreamPtr<vtx_idx>(CMesh::INDICES);
		uint16* const indices16 = nVersion >= 1 ? (sizeof(vtx_idx) == sizeof(uint16) ? (uint16*)indices : new uint16[nIndexCount]) : NULL;
		if( nVersion >= 1 && (uint16*)indices != indices16 )
		{
			for( int i = 0; i < nIndexCount; ++i )
				indices16[i] = (uint16)indices[i];
		}

		BUtil::Write2Buffer(buffer, positions, sizeof(Vec3)*nPositionCount );
		BUtil::Write2Buffer(buffer, normals, sizeof(Vec3)*nPositionCount );
		BUtil::Write2Buffer(buffer, texcoords, sizeof(SMeshTexCoord)*nTexCoordCount );
		if( nVersion == 0 )
		{
			for( int i = 0; i < nSubsetCount; ++i )
			{
				const SMeshSubset& subset = pMesh->GetSubSet(i);
				for( int k = 0; k < subset.nNumIndices/3; ++k )
					faces[(subset.nFirstIndexId/3)+k].nSubset = i;
			}
			BUtil::Write2Buffer(buffer, faces, sizeof(SMeshFace)*nFaceCount );
		}
		else if( nVersion >= 1 )
		{
			BUtil::Write2Buffer(buffer, indices16, sizeof(uint16)*nIndexCount );
			if( nTangentCount > 0 )
			{
				SMeshTangents* tangents = pMesh->GetMesh()->GetStreamPtr<SMeshTangents>(CMesh::TANGENTS);
				BUtil::Write2Buffer(buffer, tangents, sizeof(SMeshTangents)*nTangentCount);
			}
		}

		for( int i = 0; i < nSubsetCount; ++i )
		{
			const SMeshSubset& subset = pMesh->GetSubSet(i);
			BUtil::Write2Buffer(buffer, &subset.vCenter, sizeof(Vec3) );
			BUtil::Write2Buffer(buffer, &subset.fRadius, sizeof(float) );
			BUtil::Write2Buffer(buffer, &subset.fTexelDensity, sizeof(float) );

			int32 nFirstIndexId = (int32)subset.nFirstIndexId;
			int32 nNumIndices = (int32)subset.nNumIndices;
			int32 nFirstVertId = (int32)subset.nFirstVertId;
			int32 nNumVerts = (int32)subset.nNumVerts;
			int32 nMatID = (int32)subset.nMatID;
			int32 nMatFlags = (int32)subset.nMatFlags;
			int32 nPhysicalizeType = (int32)subset.nPhysicalizeType;

			BUtil::Write2Buffer(buffer, &nFirstIndexId, sizeof(int32) );
			BUtil::Write2Buffer(buffer, &nNumIndices, sizeof(int32) );
			BUtil::Write2Buffer(buffer, &nFirstVertId, sizeof(int32) );
			BUtil::Write2Buffer(buffer, &nNumVerts, sizeof(int32) );
			BUtil::Write2Buffer(buffer, &nMatID, sizeof(int32) );
			BUtil::Write2Buffer(buffer, &nMatFlags, sizeof(int32) );
			BUtil::Write2Buffer(buffer, &nPhysicalizeType, sizeof(int32) );
		}

		if( indices16 && (uint16*)indices != indices16 )
			delete [] indices16;
	}

	return true;
}

bool CBaseBrush::LoadMesh( int nVersion, std::vector<char>& buffer, CBaseObject* pObj, CBrushDesigner* pDesigner )
{
	IStatObj* pStatObj = GetIEditor()->Get3DEngine()->LoadDesignerObject( nVersion, &buffer[0], buffer.size() );
	if( pStatObj )
	{
		RemoveStatObj();
		pStatObj->AddRef();
		m_pStatObj[0] = pStatObj;

		IMaterial* pMaterial = GetMaterialFromBaseObj(pObj);
		m_pStatObj[0]->SetMaterial(pMaterial);

		AABB aabb = pDesigner->GetBoundBox(0);
		m_pStatObj[0]->SetBBoxMin(aabb.min);
		m_pStatObj[0]->SetBBoxMax(aabb.max);
		m_pStatObj[0]->m_eStreamingStatus = ecss_Ready;
		UpdateRenderNode(pObj,pDesigner);
		return true;
	}
	return false;
}

bool CBaseBrush::IsValid() const
{
	if( !m_pStatObj[0] && !m_pStatObj[1] )
		return false;

	int nPositionCount = 0;
	for( int i = 0; i < 2; ++i )
	{
		if( m_pStatObj[i] )
		{
			int nSubObjectCount = m_pStatObj[i]->GetSubObjectCount();
			for( int k = 0; k < nSubObjectCount; ++k )
			{
				if( m_pStatObj[i]->GetSubObject(k) && m_pStatObj[i]->GetSubObject(k)->pStatObj )
				{
					IIndexedMesh *pMesh = m_pStatObj[i]->GetSubObject(k)->pStatObj->GetIndexedMesh();
					nPositionCount += pMesh->GetVertexCount();
				}
			}

			IIndexedMesh *pMesh = m_pStatObj[i]->GetIndexedMesh();
			if( !pMesh )
				continue;

			nPositionCount += pMesh->GetVertexCount();
		}
	}

	return nPositionCount > 0 ? true : false;
}

int CBaseBrush::GetPolygonCount() const
{
	if( !m_pStatObj[0] && !m_pStatObj[1] )
		return 0;

	int nPolygonCount = 0;
	for( int i = 0; i < 2; ++i )
	{
		if( m_pStatObj[i] )
		{
			int nSubObjectCount = m_pStatObj[i]->GetSubObjectCount();
			for( int k = 0; k < nSubObjectCount; ++k )
			{
				if( m_pStatObj[i]->GetSubObject(k) && m_pStatObj[i]->GetSubObject(k)->pStatObj )
				{
					IIndexedMesh *pMesh = m_pStatObj[i]->GetSubObject(k)->pStatObj->GetIndexedMesh();
					nPolygonCount += pMesh->GetFaceCount();
				}
			}

			IIndexedMesh *pMesh = m_pStatObj[i]->GetIndexedMesh();
			if( !pMesh )
				continue;

			nPolygonCount += pMesh->GetVertexCount();
		}
	}

	return nPolygonCount;
}
