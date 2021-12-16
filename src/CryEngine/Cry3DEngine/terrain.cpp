////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: check vis
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "terrain.h"
#include "terrain_sector.h"
#include "StatObj.h"
#include "ObjMan.h"
#include "CullBuffer.h"
#include "terrain_water.h"
#include "VisAreas.h"
#include "Vegetation.h"

void CTerrain::AddVisSector(CTerrainNode * newsec)
{
	assert(newsec->m_cNewGeomMML<m_nUnitsToSectorBitShift);
  m_lstVisSectors.Add((CTerrainNode * )newsec);
}

void CTerrain::CheckVis()
{
  FUNCTION_PROFILER_3DENGINE;

  if(m_nRenderStackLevel==0)
    m_fDistanceToSectorWithWater = OCEAN_IS_VERY_FAR_AWAY;

  m_lstVisSectors.Clear();

  for(int nSID=0; nSID<m_pParentNodes.Count(); nSID++)
    if(GetParentNode(nSID))
    {
      // reopen texture file if needed, texture pack may be randomly closed by editor so automatic reopening used
      if(!m_arrBaseTexInfos[nSID].m_nDiffTexIndexTableSize)
        OpenTerrainTextureFile(m_arrBaseTexInfos[nSID].m_hdrDiffTexHdr, m_arrBaseTexInfos[nSID].m_hdrDiffTexInfo, 
        COMPILED_TERRAIN_TEXTURE_FILE_NAME, m_arrBaseTexInfos[nSID].m_ucpDiffTexTmpBuffer, m_arrBaseTexInfos[nSID].m_nDiffTexIndexTableSize, nSID);

      GetParentNode(nSID)->CheckVis(false, (GetCVars()->e_CoverageBufferTerrain!=0) && (GetCVars()->e_CoverageBuffer!=0), m_arrSegmentOrigns[nSID]);
    }

  if(m_nRenderStackLevel==0)
  {
    m_bOceanIsVisible = (int)((m_fDistanceToSectorWithWater != OCEAN_IS_VERY_FAR_AWAY) || !m_lstVisSectors.Count());

    if(m_fDistanceToSectorWithWater<0)
      m_fDistanceToSectorWithWater=0;

    if(!m_lstVisSectors.Count())
      m_fDistanceToSectorWithWater=0;

    m_fDistanceToSectorWithWater = max(m_fDistanceToSectorWithWater, (GetCamera().GetPosition().z-m_fOceanWaterLevel));
  }
}

int __cdecl CmpTerrainNodesImportance(const void* v1, const void* v2)
{
	CTerrainNode *p1 = *(CTerrainNode**)v1;
	CTerrainNode *p2 = *(CTerrainNode**)v2;

	// process textures in progress first
	bool bInProgress1 = p1->m_eTexStreamingStatus == ecss_InProgress;
	bool bInProgress2 = p2->m_eTexStreamingStatus == ecss_InProgress;
	if(bInProgress1 > bInProgress2)
		return -1;
	else if(bInProgress1 < bInProgress2)
		return 1;

	// process recently requested textures first
	if(p1->m_nNodeTextureLastUsedSec > p2->m_nNodeTextureLastUsedSec)
		return -1;
	else if(p1->m_nNodeTextureLastUsedSec < p2->m_nNodeTextureLastUsedSec)
		return 1;

	// move parents first
	float f1 = (float)p1->m_nTreeLevel;
	float f2 = (float)p2->m_nTreeLevel;
	if(f1 > f2)
		return -1;
	else if(f1 < f2)
		return 1;

	// move closest first
	f1 = (p1->m_arrfDistance[0]);
	f2 = (p2->m_arrfDistance[0]);
	if(f1 > f2)
		return 1;
	else if(f1 < f2)
		return -1;

	return 0;
}

int __cdecl CmpTerrainNodesDistance(const void* v1, const void* v2)
{
	CTerrainNode *p1 = *(CTerrainNode**)v1;
	CTerrainNode *p2 = *(CTerrainNode**)v2;

	float f1 = (p1->m_arrfDistance[0]);
	float f2 = (p2->m_arrfDistance[0]);
	if(f1 > f2)
		return 1;
	else if(f1 < f2)
		return -1;

	return 0;
}

void CTerrain::ActivateNodeTexture( CTerrainNode * pNode )
{
	if(pNode->m_nNodeTextureOffset < 0 || m_nRenderStackLevel)
		return;

	pNode->m_nNodeTextureLastUsedSec = (int)GetCurTimeSec();

	if(m_lstActiveTextureNodes.Find(pNode)<0)	
	{
		if(	!pNode->m_nNodeTexSet.nTex1 || !pNode->m_nNodeTexSet.nTex0 )
		{
			m_lstActiveTextureNodes.Add(pNode);
		}
	}
}

void CTerrain::ActivateNodeProcObj( CTerrainNode * pNode )
{
	if(m_lstActiveProcObjNodes.Find(pNode)<0)	
	{
		m_lstActiveProcObjNodes.Add(pNode);
	}
}

int CTerrain::GetNotReadyTextureNodesCount() 
{ 
	int nRes=0;
	for(int i=0; i<m_lstActiveTextureNodes.Count(); i++)
		if(m_lstActiveTextureNodes[i]->m_eTexStreamingStatus != ecss_Ready)
			nRes++;
	return nRes; 
}

void CTerrain::CheckNodesGeomUnload(int nSID)
{
  FUNCTION_PROFILER_3DENGINE;

  if(!Get3DEngine()->m_bShowTerrainSurface || !GetParentNode(nSID))
    return;

  for(int n=0; n<32; n++)
  {
    static uint32 nOldSectorsX=~0, nOldSectorsY=~0, nTreeLevel=~0;

    if(nTreeLevel >(uint32) GetParentNode(nSID)->m_nTreeLevel)
      nTreeLevel = GetParentNode(nSID)->m_nTreeLevel;

    uint32 nTableSize = CTerrain::GetSectorsTableSize()>>nTreeLevel;
    assert(nTableSize);

    // x/y cycle
    nOldSectorsY++;
    if(nOldSectorsY >= nTableSize)
    {
      nOldSectorsY = 0;
      nOldSectorsX++;
    }

    if(nOldSectorsX >= nTableSize)
    {
      nOldSectorsX = 0;
      nTreeLevel++;
    }

    if(nTreeLevel > (uint32)GetParentNode(nSID)->m_nTreeLevel)
      nTreeLevel = 0;

    if(CTerrainNode * pNode = m_arrSecInfoPyramid[nSID][nTreeLevel][nOldSectorsX][nOldSectorsY])
      pNode->CheckNodeGeomUnload();
  }
}

void CTerrain::UpdateNodesIncrementaly()
{
	FUNCTION_PROFILER_3DENGINE;

	// process textures
	if(m_arrBaseTexInfos[0].m_nDiffTexIndexTableSize && m_lstActiveTextureNodes.Count() && m_texCache[0].Update())
	{
		m_texCache[1].Update();

		for(int i=0; i<m_lstActiveTextureNodes.Count(); i++)
			m_lstActiveTextureNodes[i]->UpdateDistance();

		// sort by importance
		qsort(m_lstActiveTextureNodes.GetElements(), m_lstActiveTextureNodes.Count(), 
			sizeof(m_lstActiveTextureNodes[0]), CmpTerrainNodesImportance);

		// release unimportant textures and make sure at least one texture is free for possible loading
		while(m_lstActiveTextureNodes.Count()>GetCVars()->e_TerrainTextureStreamingPoolItemsNum-1)
		{
			m_lstActiveTextureNodes.Last()->UnloadNodeTexture(false);
			m_lstActiveTextureNodes.DeleteLast();
		}

		// load one sector if needed
		assert(m_texCache[0].m_FreeTextures.Count());
		for(int i=0; i<m_lstActiveTextureNodes.Count(); i++)
			if(!m_lstActiveTextureNodes[i]->CheckUpdateDiffuseMap())
        if(!Get3DEngine()->IsTerrainSyncLoad())
				  break;

		if(GetCVars()->e_TerrainTextureStreamingDebug>=2)
		{
			for(int i=0; i<m_lstActiveTextureNodes.Count(); i++)
			{
				CTerrainNode * pNode = m_lstActiveTextureNodes[i];
				if(pNode->m_nTreeLevel <= GetCVars()->e_TerrainTextureStreamingDebug-2)
					switch(pNode->m_eTexStreamingStatus)
				{
					case ecss_NotLoaded:
						DrawBBox(pNode->GetBBoxVirtual(), Col_Red);
						break;
					case ecss_InProgress:
						DrawBBox(pNode->GetBBoxVirtual(), Col_Green);
						break;
					case ecss_Ready:
						DrawBBox(pNode->GetBBoxVirtual(), Col_Blue);
						break;
				}
			}
		}
	}

	// process procedural objects
	if(m_lstActiveProcObjNodes.Count())
	{
    if(!CTerrainNode::GetProcObjChunkPool())
    {
      CTerrainNode::SetProcObjChunkPool(new SProcObjChunkPool(MAX_PROC_OBJ_CHUNKS_NUM));
      CTerrainNode::SetProcObjPoolMan(new CProcVegetPoolMan(GetCVars()->e_ProcVegetationMaxSectorsInCache));
    }

		// make sure distances are correct
		for(int i=0; i<m_lstActiveProcObjNodes.Count(); i++)
			m_lstActiveProcObjNodes[i]->UpdateDistance();

		// sort by distance
		qsort(m_lstActiveProcObjNodes.GetElements(), m_lstActiveProcObjNodes.Count(), 
			sizeof(m_lstActiveProcObjNodes[0]), CmpTerrainNodesDistance);

		// release unimportant sectors
		static int nMaxSectors = GetCVars()->e_ProcVegetationMaxSectorsInCache;
		while(m_lstActiveProcObjNodes.Count() > (GetCVars()->e_ProcVegetation ? nMaxSectors : 0))
		{
			m_lstActiveProcObjNodes.Last()->RemoveProcObjects(false);
			m_lstActiveProcObjNodes.DeleteLast();
		}

    while(1)
    {
		  // release even more if we are running out of memory
		  while(m_lstActiveProcObjNodes.Count())
		  {
			  int nAll=0;
			  int nUsed = CTerrainNode::GetProcObjChunkPool()->GetUsedInstancesCount(nAll);
			  if(nAll - nUsed > (MAX_PROC_OBJ_CHUNKS_NUM/(GetCVars()->e_ProcVegetationMaxSectorsInCache))) // make sure at least X chunks are free and ready to be used in this frame
				  break;

			  m_lstActiveProcObjNodes.Last()->RemoveProcObjects(false);
			  m_lstActiveProcObjNodes.DeleteLast();

			  nMaxSectors = min(nMaxSectors, m_lstActiveProcObjNodes.Count());
		  }

		  // build most important not ready sector
      bool bAllDone = true;
		  for(int i=0; i<m_lstActiveProcObjNodes.Count(); i++)
      {
			  if(m_lstActiveProcObjNodes[i]->CheckUpdateProcObjects())
        {
          bAllDone = false;
          break;
        }
      }

      if(!Get3DEngine()->IsTerrainSyncLoad() || bAllDone)
        break;
    }

//    if(m_lstActiveProcObjNodes.Count())
  //    m_lstActiveProcObjNodes[0]->RemoveProcObjects(false);

		if(GetCVars()->e_ProcVegetation==2)
			for(int i=0; i<m_lstActiveProcObjNodes.Count(); i++)
				DrawBBox(m_lstActiveProcObjNodes[i]->GetBBoxVirtual(),
				m_lstActiveProcObjNodes[i]->IsProcObjectsReady() ? Col_Green : Col_Red);
	}
}

int CTerrain::RenderTerrainWater()
{
  FUNCTION_PROFILER_3DENGINE;

  if(!m_pOcean || !GetCVars()->e_WaterOcean)
    return 0;

  m_pOcean->Render(m_nRenderStackLevel);

  m_pOcean->SetLastFov(GetCamera().GetFov());

  return 0;
}

void CTerrain::ApplyForceToEnvironment(Vec3 vPos, float fRadius, float fAmountOfForce)
{
  if(fRadius<=0)
    return;

  if(!_finite(vPos.x) || !_finite(vPos.y) || !_finite(vPos.z))
  {
    PrintMessage("Error: 3DEngine::ApplyForceToEnvironment: Undefined position passed to the function");
    return;
  }

  if (fRadius > 50.f)
    fRadius = 50.f;

  if(fAmountOfForce>1.f)
    fAmountOfForce = 1.f;

  if( (vPos.GetDistance(GetCamera().GetPosition()) > 50.f+fRadius*2.f ) || // too far
    vPos.z < (GetZApr(vPos.x, vPos.y, false, GetDefSID())-1.f)) // under ground
    return;

	PodArray<SRNInfo> lstObjects;
	Get3DEngine()->MoveObjectsIntoListGlobal(&lstObjects, NULL);

	// TODO: support in octree

  // affect objects around
  for(int i=0; i<lstObjects.Count(); i++)
  if(lstObjects[i].pNode->GetRenderNodeType() == eERType_Vegetation)
  {
    CVegetation * pVegetation =  (CVegetation*)lstObjects[i].pNode;
		Vec3 vDist = pVegetation->GetPos() - vPos;
    float fDist = vDist.GetLength();
    if(fDist<fRadius && fDist>0.f)
    {
      float fMag = (1.f - fDist/fRadius) * fAmountOfForce;
      pVegetation->AddBending(vDist * (fMag/fDist));
    }
  }
}

Vec3 CTerrain::GetTerrainSurfaceNormal(Vec3 vPos, float fRange, bool bIncludeVoxels)
{ 
  FUNCTION_PROFILER_3DENGINE;

	fRange += 0.05f;
  Vec3 v1 = Vec3(vPos.x-fRange, vPos.y-fRange,  GetZApr(vPos.x-fRange , vPos.y-fRange , bIncludeVoxels, GetDefSID()));
  Vec3 v2 = Vec3(vPos.x-fRange,	vPos.y+fRange,  GetZApr(vPos.x        , vPos.y+fRange , bIncludeVoxels, GetDefSID()));
  Vec3 v3 = Vec3(vPos.x+fRange, vPos.y-fRange,	GetZApr(vPos.x+fRange , vPos.y        , bIncludeVoxels, GetDefSID()));
  Vec3 v4 = Vec3(vPos.x+fRange, vPos.y+fRange,  GetZApr(vPos.x+fRange, vPos.y+fRange  , bIncludeVoxels, GetDefSID()));
  return (v3-v2).Cross(v4-v1).GetNormalized(); 
}

void CTerrain::GetMemoryUsage( class ICrySizer*pSizer)
{
  for(int nSID=0; nSID<m_arrSecInfoPyramid.Count(); nSID++)
    if(m_pParentNodes[nSID])
  {
    SIZER_COMPONENT_NAME(pSizer, "SecInfoTable");
		for(int nTreeLevel = 0; nTreeLevel<TERRAIN_NODE_TREE_DEPTH && m_arrSecInfoPyramid[nSID][nTreeLevel].m_nSize; nTreeLevel++)
			m_arrSecInfoPyramid[nSID][nTreeLevel].GetMemoryUsage(pSizer);
  }

  if(CTerrainNode::GetProcObjPoolMan())
	{
		SIZER_COMPONENT_NAME(pSizer, "ProcVeget");
		CTerrainNode::GetProcObjPoolMan()->GetMemoryUsage(pSizer);
	}

  int nSize = 0;

  for(int nSID=0; nSID<m_arrSecInfoPyramid.Count(); nSID++)
    if(m_pParentNodes[nSID])
	{
		SIZER_COMPONENT_NAME(pSizer, "TerrainNode");
		for(int nTreeLevel = 0; nTreeLevel<TERRAIN_NODE_TREE_DEPTH && m_arrSecInfoPyramid[nSID][nTreeLevel].m_nSize; nTreeLevel++)
			for(int x=0; x<CTerrain::GetSectorsTableSize()>>nTreeLevel; x++)
				for(int y=0; y<CTerrain::GetSectorsTableSize()>>nTreeLevel; y++)
					if (m_arrSecInfoPyramid[nSID][nTreeLevel][x][y])
						m_arrSecInfoPyramid[nSID][nTreeLevel][x][y]->GetMemoryUsage(pSizer);

    nSize += m_arrBaseTexInfos[nSID].m_TerrainTextureLayer[0].nSectorSizeBytes;
	}
  
  nSize += sizeofVector(m_lstVisSectors);

  if(m_pOcean)
    nSize += sizeof(*m_pOcean) + m_pOcean->GetMemoryUsage();

  pSizer->AddObject(this, nSize + sizeof(*this));
}

bool CTerrain::PreloadResources()
{
  FUNCTION_PROFILER_3DENGINE;

  return 0;
}

void CTerrain::GetObjects(PodArray<SRNInfo> * pLstObjects)
{
  // take out all objects in terrain 
/*	for(int nTreeLevel = 0; nTreeLevel<TERRAIN_NODE_TREE_DEPTH && m_arrSecInfoPyramid[nSID][nTreeLevel].m_nSize; nTreeLevel++)
	for(int x=0; x<CTerrain::GetSectorsTableSize()>>nTreeLevel; x++)
	for(int y=0; y<CTerrain::GetSectorsTableSize()>>nTreeLevel; y++)
  {
    CBasicArea * pSecInfo = m_arrSecInfoPyramid[nSID][nTreeLevel][x][y];  
    for(int nStatic=0; nStatic<2; nStatic++)
    {
      if(pLstObjects)
        pLstObjects->AddList(pSecInfo->m_lstEntities[nStatic]);
      pSecInfo->m_lstEntities[nStatic].Reset();        
    }
  }*/
}

int CTerrain::GetTerrainNodesAmount() 
{
//	((N pow l)-1)/(N-1)
#if defined(__GNUC__)
	uint64 amount = (uint64)0xaaaaaaaaaaaaaaaaULL;
#else
	uint64 amount = (uint64)0xaaaaaaaaaaaaaaaa;
#endif
	amount >>= (65-(GetParentNode(0)->m_nTreeLevel+1)*2);
	return (int)amount;
}

void CTerrain::GetVisibleSectorsInAABB(PodArray<struct CTerrainNode*> & lstBoxSectors, const AABB & boxBox)
{
	lstBoxSectors.Clear();
	for(int i=0; i<m_lstVisSectors.Count(); i++)
	{
		CTerrainNode * pNode = m_lstVisSectors[i];
		if(pNode->GetBBox().IsIntersectBox(boxBox))
			lstBoxSectors.Add(pNode);
	}
}

void CTerrain::RegisterLightMaskInSectors(CDLight * pLight, int nSID)
{
  if(!GetParentNode(nSID))
    return;

	FUNCTION_PROFILER_3DENGINE;

	assert(pLight->m_Id >= 0 || 1);
	//assert(!pLight->m_Shader.m_pShader || !(pLight->m_Shader.m_pShader->GetLFlags() & LMF_DISABLE));

	// get intersected outdoor sectors
	static PodArray<CTerrainNode*> lstSecotors; lstSecotors.Clear();
	AABB aabbBox(pLight->m_Origin - Vec3(pLight->m_fRadius,pLight->m_fRadius,pLight->m_fRadius), 
		pLight->m_Origin + Vec3(pLight->m_fRadius,pLight->m_fRadius,pLight->m_fRadius));
	GetParentNode(nSID)->IntersectTerrainAABB(aabbBox, lstSecotors);

	// set lmask in all affected sectors
	for( int s=0; s<lstSecotors.Count(); s++)
		lstSecotors[s]->AddLightSource(pLight);
}

void CTerrain::IntersectWithShadowFrustum(PodArray<CTerrainNode*> * plstResult, ShadowMapFrustum * pFrustum, int nSID)
{
	if(GetParentNode(nSID))
	{
		float fHalfGSMBoxSize = 0.5f / (pFrustum->fFrustrumSize * GetCVars()->e_GsmRange);

		GetParentNode(nSID)->IntersectWithShadowFrustum(false, plstResult, pFrustum, fHalfGSMBoxSize);
	}
}

void CTerrain::IntersectWithBox(const AABB & aabbBox, PodArray<CTerrainNode*> * plstResult, int nSID)
{
	if(GetParentNode(nSID))
		GetParentNode(nSID)->IntersectWithBox(aabbBox, plstResult);
}

void CTerrain::MarkAllSectorsAsUncompiled(int nSID)
{
	if(GetParentNode(nSID))
		GetParentNode(nSID)->RemoveProcObjects(true);

	if(nSID < Get3DEngine()->m_pObjectsTree.Count() && Get3DEngine()->m_pObjectsTree[nSID])
    if(Get3DEngine()->m_pObjectsTree[nSID])
  		Get3DEngine()->m_pObjectsTree[nSID]->MarkAsUncompiled();
}

void CTerrain::SetHeightMapMaxHeight(float fMaxHeight) 
{
  for(int nSID=0; nSID<m_pParentNodes.Count(); nSID++)
    if(m_pParentNodes[nSID])
      InitHeightfieldPhysics(nSID);
}
/*
void CTerrain::RenaderImposterContent(class CREImposter * pImposter, const CCamera & cam)
{
	pImposter->GetTerrainNode()->RenderImposterContent(pImposter, cam);
}
*/

void SetTerrain( CTerrain &rTerrain );


void CTerrain::SerializeTerrainState( TSerialize ser )
{
	ser.BeginGroup("TerrainState");

	m_StoredModifications.SerializeTerrainState(ser);

	ser.EndGroup();
}

CTerrainNode * CTerrain::FindMinNodeContainingBox(const AABB & someBox, int nSID)
{
	FUNCTION_PROFILER_3DENGINE;

	CTerrainNode * pParentNode = GetParentNode(nSID);
	return pParentNode ? pParentNode->FindMinNodeContainingBox(someBox) : NULL;
/*
	static PodArray<CTerrainNode*> lstTerrainSectors; lstTerrainSectors.Clear();
	static PodArray<CTerrainNode*> lstTerrainSectorsResult; lstTerrainSectorsResult.Clear();
	IntersectWithBox(someBox,&lstTerrainSectors);

	for(int i=0; i<lstTerrainSectors.Count(); i++)
	{
		CTerrainNode * pNode = lstTerrainSectors[i];

		if(pNode->m_boxHeigtmap.max.x > someBox.max.x)
		if(pNode->m_boxHeigtmap.max.y > someBox.max.y)
		if(pNode->m_boxHeigtmap.min.x < someBox.min.x)
		if(pNode->m_boxHeigtmap.min.y < someBox.min.y)
		if(pNode->m_nTreeLevel >= GetTerrain()->m_nDiffTexTreeLevelOffset)
			lstTerrainSectorsResult.Add(pNode);
	}

	if(lstTerrainSectorsResult.Count())
		return lstTerrainSectorsResult[0];

	return NULL;*/
}

int CTerrain::GetTerrainLightmapTexId( Vec4 & vTexGenInfo, int nSID )
{
	AABB nearWorldBox;
	float fBoxSize = 512;
	nearWorldBox.min = GetCamera().GetPosition() - Vec3(fBoxSize,fBoxSize,fBoxSize);
	nearWorldBox.max = GetCamera().GetPosition() + Vec3(fBoxSize,fBoxSize,fBoxSize);
	
	CTerrainNode * pNode = GetParentNode(nSID)->FindMinNodeContainingBox(nearWorldBox);
	
	if(pNode)
		pNode = pNode->GetTexuringSourceNode(0, ett_LM);
	else
		pNode = GetParentNode(nSID);

	vTexGenInfo.x = pNode->m_nOriginX;
	vTexGenInfo.y = pNode->m_nOriginY;
	vTexGenInfo.z = (float)(CTerrain::GetSectorSize()<<pNode->m_nTreeLevel);
	vTexGenInfo.w = 512.0f;
	return (pNode && pNode->m_nNodeTexSet.nTex1>0) ? pNode->m_nNodeTexSet.nTex1 : 0;
}

bool CTerrain::IsAmbientOcclusionEnabled() 
{ 
  if(this)
  {
    for(int nSID=0;nSID<m_arrBaseTexInfos.Count(); nSID++)
      if((m_arrBaseTexInfos[nSID].m_hdrDiffTexInfo.dwFlags == TTFHF_AO_DATA_IS_VALID) && GetCVars()->e_TerrainAo)
        return true;
  }

  return 0;
}

int CTerrain::GenerateMeshFromHeightmap(  int nMinX, int nMinY, int nMaxX, int nMaxY, PodArray<Vec3> & arrVerts, PodArray<uint16> & arrIndices, int nSID )
{
  if(GetCVars()->e_VoxTer)
    return 0;

  int nUnitSize = Get3DEngine()->GetHeightMapUnitSize();

  for(int x = nMinX; x <= nMaxX; x += nUnitSize)
  {
    for(int y = nMinY; y <= nMaxY; y += nUnitSize)
    {
      Vec3 vTmp((float)x,(float)y,(float)Get3DEngine()->GetTerrainElevation((float)x,(float)y));
      arrVerts.Add(vTmp);
    }
  }

  // make indices
  int dx = (nMaxX-nMinX)/nUnitSize;
  int dy = (nMaxY-nMinY)/nUnitSize;

  for(int x = 0; x < dx; x ++)
  {
    for(int y = 0; y < dy; y ++)
    {
      int nIdx0 = (x*(dy+1) + y);
      int nIdx1 = (x*(dy+1) + y+(dy+1));
      int nIdx2 = (x*(dy+1) + y+1);
      int nIdx3 = (x*(dy+1) + y+1+(dy+1));

      assert(nIdx3 < arrVerts.Count());

      int arrQuadIndices[] = { nIdx0, nIdx1, nIdx2, nIdx3 };
      bool bHole = false;
      for(int i=0; i<4; i++)
      {
        Vec3 & vert = arrVerts[arrQuadIndices[i]];
        if(GetHole((int)vert.x, (int)vert.y, nSID))
        {
          bHole = true;
          break;
        }
      }

      if(!bHole)
      {
        arrIndices.Add(nIdx0);
        arrIndices.Add(nIdx1);
        arrIndices.Add(nIdx2);

        arrIndices.Add(nIdx1);
        arrIndices.Add(nIdx3);
        arrIndices.Add(nIdx2);
      }
    }
  }

  return arrIndices.Count()/3;
}

void CTerrain::GetResourceMemoryUsage(ICrySizer*	pSizer,const AABB&	crstAABB, int nSID)
{
	CTerrainNode*	poTerrainNode=FindMinNodeContainingBox(crstAABB, nSID);

	if (poTerrainNode)
	{
		poTerrainNode->GetResourceMemoryUsage(pSizer,crstAABB);
	}
}

int CTerrain::CreateSegment()
{
  int nSID;

  // try to find existing empty slot
  for(nSID=0; nSID<m_pParentNodes.Count(); nSID++)
  {
    if(!m_pParentNodes[nSID])
      break;
  }

  if(nSID >= m_pParentNodes.Count())
  {
    // terrain nodes tree
    m_pParentNodes.PreAllocate(nSID+1,nSID+1);

    // terrain nodes pyramid
    m_arrSecInfoPyramid.PreAllocate(nSID+1, nSID+1);

    // terrain surface types
    m_SSurfaceType.PreAllocate(nSID+1, nSID+1);

    // terrain texture file info
    m_arrBaseTexInfos.PreAllocate(nSID+1, nSID+1);

    // position in the world
    m_arrSegmentOrigns.PreAllocate(nSID+1,nSID+1);

    // objects tree
    Get3DEngine()->m_pObjectsTree.PreAllocate(nSID+1,nSID+1);
  }

  assert(!m_pParentNodes[nSID]);

  m_SSurfaceType[nSID].PreAllocate(MAX_SURFACE_TYPES_COUNT,MAX_SURFACE_TYPES_COUNT);

  m_arrSegmentOrigns[nSID] = Vec3(0,0,0);

  assert(!Get3DEngine()->m_pObjectsTree[nSID]);
  Get3DEngine()->m_pObjectsTree[nSID] = new COctreeNode(nSID, AABB(Vec3(0,0,0),Vec3((float)GetTerrainSize(),(float)GetTerrainSize(),(float)GetTerrainSize())), NULL);

  return nSID;
}

bool CTerrain::SetSegmentOrigin(int nSID, Vec3 vSegmentOrigin)
{
  if(nSID<0 || nSID>=m_arrSegmentOrigns.Count())
    return false;
  
  m_arrSegmentOrigns[nSID] = vSegmentOrigin;

  return true;
}

bool CTerrain::DeleteSegment(int nSID)
{
  if(nSID<0 || nSID >= m_pParentNodes.Count())
    return false;

  // terrain texture file info
  CloseTerrainTextureFile(nSID);
  SAFE_DELETE_ARRAY(m_arrBaseTexInfos[nSID].m_ucpDiffTexTmpBuffer);
  ZeroStruct(m_arrBaseTexInfos[nSID]);

  // surface types
  for(int i=0; i<m_SSurfaceType[nSID].Count(); i++)
  {
    m_SSurfaceType[nSID][i].lstnVegetationGroups.Reset();
    m_SSurfaceType[nSID][i].pLayerMat = NULL;
  }

  m_SSurfaceType[nSID].Reset();
  ZeroStruct(m_SSurfaceType[nSID]);

  // terrain nodes tree
  SAFE_DELETE(m_pParentNodes[nSID]);

  // terrain nodes pyramid
  for(int i=0; i<TERRAIN_NODE_TREE_DEPTH && i<m_arrSecInfoPyramid[nSID].Count(); i++)
    m_arrSecInfoPyramid[nSID][i].Reset();
  m_arrSecInfoPyramid[nSID].Reset();

  // position in the world
  m_arrSegmentOrigns[nSID].Set(0,0,0);

  // objects tree
  SAFE_DELETE(Get3DEngine()->m_pObjectsTree[nSID]);

  return true;
}

#include UNIQUE_VIRTUAL_WRAPPER(ITerrain)

