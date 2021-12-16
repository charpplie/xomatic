#include "StdAfx.h"
#include "BrushRegionMesh.h"
#include "BrushDesignerPolygonDecomposer.h"
#include "Material/Material.h"
#include "Material/MaterialManager.h"

CBrushRegionMesh::CBrushRegionMesh()
{
	m_pStatObj = NULL;
	m_pRenderNode = NULL;
	m_MaterialName = "Editor/Materials/crydesigner_selection";
}

CBrushRegionMesh::~CBrushRegionMesh()
{
	ReleaseResources();
}

void CBrushRegionMesh::ReleaseResources()
{
	if( m_pStatObj )
	{
		m_pStatObj->Release();
		m_pStatObj = NULL;
	}

	ReleaseRenderNode();
}

void CBrushRegionMesh::ReleaseRenderNode()
{
	if( m_pRenderNode )
	{
		GetIEditor()->Get3DEngine()->DeleteRenderNode(m_pRenderNode);
		m_pRenderNode = NULL;
	}
}

void CBrushRegionMesh::CreateRenderNode()
{
	ReleaseRenderNode();
	m_pRenderNode = GetIEditor()->Get3DEngine()->CreateRenderNode(eERType_Brush);
}

void CBrushRegionMesh::SetRegion( CBrushRegion::RegionPtr pRegion, bool bForce, const Matrix34& worldTM, int dwRndFlags, int nViewDistRatio, int nMinSpec, uint8 materialLayerMask )
{
	if( m_pRegions.size() == 1 && m_pRegions[0] == pRegion && !bForce )
		return;

	std::vector<CBrushRegion::RegionPtr> regions;
	regions.push_back(pRegion);

	SetRegions(regions, bForce, worldTM, dwRndFlags, nViewDistRatio, nMinSpec, materialLayerMask );
}

void CBrushRegionMesh::SetRegions( const std::vector<CBrushRegion::RegionPtr>& regionList, bool bForce, const Matrix34& worldTM, int dwRndFlags, int nViewDistRatio, int nMinSpec, uint8 materialLayerMask )
{
	m_pRegions = regionList;
	ReleaseResources();

	BUtil::SMeshInfo mesh;

	for( int i = 0, iRegionSize(regionList.size()); i < iRegionSize; ++i )
	{
		CBrushRegion::RegionPtr pRegion = regionList[i];
		if( !pRegion || pRegion->IsOpen() )
			continue;

		int vertexOffset = mesh.vertexList.size();
		int faceOffset = mesh.faceList.size();

		CBrushDesignerPolygonDecomposer decomposer;
		if( !decomposer.TriangulateRegion(pRegion, mesh.vertexList, mesh.normalList, mesh.faceList, vertexOffset, faceOffset) )
			continue; 

		const BrushPlane& plane = pRegion->GetPlane();
		int vListCount = mesh.vertexList.size();
		for( int i = vertexOffset; i < vListCount; ++i )
		{
			SMeshTexCoord uv;
			BUtil::CalcTexCoords( SBrushPlane<float>(ToVec3(plane.Normal()),0), pRegion->GetTexInfo(), ToVec3(mesh.vertexList[i]), uv.s, uv.t );
			mesh.uvList.push_back(uv);
		}
	}

	if( mesh.IsValid() )
		UpdateStatObjAndRenderNode( mesh, worldTM, dwRndFlags, nViewDistRatio, nMinSpec, materialLayerMask );
}

void CBrushRegionMesh::UpdateStatObjAndRenderNode( const BUtil::SMeshInfo& mesh, const Matrix34& worldTM, int dwRndFlags, int nViewDistRatio, int nMinSpec, uint8 materialLayerMask )
{
	CreateRenderNode();

	if( !m_pStatObj )
	{
		m_pStatObj = GetIEditor()->Get3DEngine()->CreateStatObj();
		m_pStatObj->AddRef();
	}

	IIndexedMesh *pMesh = m_pStatObj->GetIndexedMesh();
	if( !pMesh )
		return;

	BUtil::STexInfo texInfo;
	BUtil::FillMesh( mesh, pMesh );

	m_pStatObj->SetBBoxMin(pMesh->GetBBox().min);
	m_pStatObj->SetBBoxMax(pMesh->GetBBox().max);

	pMesh->Optimize();
	pMesh->RestoreFacesFromIndices();

	Matrix34 identityTM = Matrix34::CreateIdentity();
	m_pRenderNode->SetEntityStatObj(0,m_pStatObj,&identityTM);	

	m_pStatObj->Invalidate(false);

	m_pRenderNode->SetMatrix(worldTM);
	m_pRenderNode->SetRndFlags(dwRndFlags|ERF_RENDER_ALWAYS);
	m_pRenderNode->SetViewDistRatio(nViewDistRatio);
	m_pRenderNode->SetMinSpec(nMinSpec);
	m_pRenderNode->SetMaterialLayers(materialLayerMask);

	ApplyMaterial();
}

void CBrushRegionMesh::SetWorldTM( const Matrix34& worldTM )
{
	SetRegions(m_pRegions, true, worldTM);
}

void CBrushRegionMesh::SetMaterialName( const CString& name )
{
	m_MaterialName = name;
	ApplyMaterial();
	if( m_pStatObj )
		m_pStatObj->Invalidate(true);
}

void CBrushRegionMesh::ApplyMaterial()
{
	if( !m_pStatObj || !m_pRenderNode )
		return;

	_smart_ptr<CMaterial> pMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(m_MaterialName);
	if( pMaterial == NULL )
		return;

	m_pStatObj->SetMaterial(pMaterial->GetMatInfo());
	m_pRenderNode->SetMaterial(pMaterial->GetMatInfo());
}