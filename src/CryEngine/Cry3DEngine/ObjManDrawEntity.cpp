////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   statobjmandraw.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Render all entities in the sector together with shadows
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <ICryAnimation.h>

#include "terrain.h"
#include "StatObj.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "terrain_sector.h"
#include "3dEngine.h"
#include "CullBuffer.h"
#include "3dEngine.h"
#include "CryParticleSpawnInfo.h"
#include "LightEntity.h"
#include "DecalManager.h"
#include "ObjectsTree.h"
#include "Brush.h"

uint8 BoxSides[0x40*8] = {
	0,0,0,0, 0,0,0,0, //00
	0,4,6,2, 0,0,0,4, //01
	7,5,1,3, 0,0,0,4, //02
	0,0,0,0, 0,0,0,0, //03
	0,1,5,4, 0,0,0,4, //04
	0,1,5,4, 6,2,0,6, //05
	7,5,4,0, 1,3,0,6, //06
	0,0,0,0, 0,0,0,0, //07
	7,3,2,6, 0,0,0,4, //08
	0,4,6,7, 3,2,0,6, //09
	7,5,1,3, 2,6,0,6, //0a
	0,0,0,0, 0,0,0,0, //0b
	0,0,0,0, 0,0,0,0, //0c
	0,0,0,0, 0,0,0,0, //0d
	0,0,0,0, 0,0,0,0, //0e
	0,0,0,0, 0,0,0,0, //0f
	0,2,3,1, 0,0,0,4, //10
	0,4,6,2, 3,1,0,6, //11
	7,5,1,0, 2,3,0,6, //12
	0,0,0,0, 0,0,0,0, //13
	0,2,3,1, 5,4,0,6, //14
	1,5,4,6, 2,3,0,6, //15
	7,5,4,0, 2,3,0,6, //16
	0,0,0,0, 0,0,0,0, //17
	0,2,6,7, 3,1,0,6, //18
	0,4,6,7, 3,1,0,6, //19
	7,5,1,0, 2,6,0,6, //1a
	0,0,0,0, 0,0,0,0, //1b
	0,0,0,0, 0,0,0,0, //1c
	0,0,0,0, 0,0,0,0, //1d
	0,0,0,0, 0,0,0,0, //1e
	0,0,0,0, 0,0,0,0, //1f
	7,6,4,5, 0,0,0,4, //20
	0,4,5,7, 6,2,0,6, //21
	7,6,4,5, 1,3,0,6, //22
	0,0,0,0, 0,0,0,0, //23
	7,6,4,0, 1,5,0,6, //24
	0,1,5,7, 6,2,0,6, //25
	7,6,4,0, 1,3,0,6, //26
	0,0,0,0, 0,0,0,0, //27
	7,3,2,6, 4,5,0,6, //28
	0,4,5,7, 3,2,0,6, //29
	6,4,5,1, 3,2,0,6, //2a
	0,0,0,0, 0,0,0,0, //2b
	0,0,0,0, 0,0,0,0, //2c
	0,0,0,0, 0,0,0,0, //2d
	0,0,0,0, 0,0,0,0, //2e
	0,0,0,0, 0,0,0,0, //2f
	0,0,0,0, 0,0,0,0, //30
	0,0,0,0, 0,0,0,0, //31
	0,0,0,0, 0,0,0,0, //32
	0,0,0,0, 0,0,0,0, //33
	0,0,0,0, 0,0,0,0, //34
	0,0,0,0, 0,0,0,0, //35
	0,0,0,0, 0,0,0,0, //36
	0,0,0,0, 0,0,0,0, //37
	0,0,0,0, 0,0,0,0, //38
	0,0,0,0, 0,0,0,0, //39
	0,0,0,0, 0,0,0,0, //3a
	0,0,0,0, 0,0,0,0, //3b
	0,0,0,0, 0,0,0,0, //3c
	0,0,0,0, 0,0,0,0, //3d
	0,0,0,0, 0,0,0,0, //3e
	0,0,0,0, 0,0,0,0, //3f
};

bool CObjManager::IsAfterWater(const Vec3 & vPos, const Vec3 & vCamPos, float fUserWaterLevel)
{
	float fWaterLevel = fUserWaterLevel==WATER_LEVEL_UNKNOWN && GetTerrain() ? GetTerrain()->GetWaterLevel() : fUserWaterLevel;
	return (0.5f-m_nRenderStackLevel)*(vCamPos.z-fWaterLevel)*(vPos.z-fWaterLevel) > 0;
}

void CObjManager::RenderOccluderIntoZBuffer(IRenderNode * pEnt, float fEntDistance, CCullBuffer & rCB, bool bCompletellyInFrustum)
{
	FUNCTION_PROFILER_3DENGINE;

#ifdef _DEBUG
	const char * szName = pEnt->GetName();
	const char * szClassName = pEnt->GetEntityClassName();
#endif // _DEBUG

	// do not draw if marked to be not drawn
	unsigned int nRndFlags = pEnt->GetRndFlags();
	if(nRndFlags&ERF_HIDDEN)
		return;
	
	// Mark this object as good occluder if not yet marked.
	pEnt->SetRndFlags( ERF_GOOD_OCCLUDER, true );

	EERType eERType = pEnt->GetRenderNodeType();
/*
	if(GetCVars()->e_back_objects_culling &&	eERType == eERType_Brush)
	{
		if(CullBackObject(pEnt, rCB.GetCamera().GetPosition(), pEnt->m_WSBBox.GetCenter()))
		{
			pEnt->m_pRNTmpData->userData.m_OcclState.nLastOccludedMainFrameID = GetMainFrameID();
			return;
		}
	}
*/
  // check cvars
  switch(eERType)
  {
  case eERType_Brush:
    if(!GetCVars()->e_Brushes) return;
    break;
  case eERType_Vegetation:
    if(!GetCVars()->e_Vegetation) return;
    break;
  case eERType_ParticleEmitter:
    if(!GetCVars()->e_Particles) return;
    break;
  case eERType_Decal:
    if(!GetCVars()->e_Decals) return;
    break;
  case eERType_VoxelObject:
    if(!GetCVars()->e_Voxel) return;
    break;
  case eERType_WaterWave:
    if(!GetCVars()->e_WaterWaves) return;
    break;
	case eERType_Road:
		if(!GetCVars()->e_Roads) return;
		break;
  default:
    if(!GetCVars()->e_Entities) return;
    break;
  }

  // detect bad objects
  float fEntLengthSquared = pEnt->GetBBox().GetSize().GetLengthSquared();
  if(eERType != eERType_Light || !_finite(fEntLengthSquared))
  {
    if(fEntLengthSquared > MAX_VALID_OBJECT_VOLUME || !_finite(fEntLengthSquared) || fEntLengthSquared<=0)
    {
      static float fLastPrintTime = 0;

      if(fLastPrintTime < GetCurTimeSec()-1.f)
      {
        Warning("CObjManager::RenderObject: Object has invalid bbox: %s,%s, Radius = %.2f, Center = (%.1f,%.1f,%.1f)",
          pEnt->GetName(), pEnt->GetEntityClassName(), cry_sqrtf(fEntLengthSquared)*0.5f, 
          pEnt->GetBBox().GetCenter().x, pEnt->GetBBox().GetCenter().y, pEnt->GetBBox().GetCenter().z);
        
        fLastPrintTime = GetCurTimeSec();
      }

      return; // skip invalid objects - usually only objects with invalid very big scale will reach this point
    }
  }

  // allocate RNTmpData for potentially visible objects
  Get3DEngine()->CheckCreateRNTmpData(&pEnt->m_pRNTmpData, pEnt);

  { // avoid double processing
    int nFrameId = GetFrameID();
    if(pEnt->m_pRNTmpData->userData.m_nOccluderFrameId == nFrameId)
      return;
    pEnt->m_pRNTmpData->userData.m_nOccluderFrameId = nFrameId;
  }

	// check all possible occlusions for outdoor objects
	if(fEntLengthSquared && GetCVars()->e_Portals!=3)// && eERType == eERType_Brush)
	{
		// test occlusion of outdoor objects by mountains
		CStatObj * pStatObj = (CStatObj *)pEnt->GetEntityStatObj(0);
		if(pStatObj && m_fZoomFactor)
		{
			if(GetCVars()->e_CoverageBufferOccludersTestMinTrisNum < pStatObj->GetRenderTrisCount())
			{
				float fZoomedDistance = fEntDistance/m_fZoomFactor;

				//	uint32 nOcclMask = 0;
				//			if(GetCVars()->e_CoverageBuffer_per_object_test)
				//	nOcclMask |= OCCL_TEST_CBUFFER;

				if(IsBoxOccluded(pEnt->GetBBox(), fZoomedDistance, &pEnt->m_pRNTmpData->userData.m_OcclState, pEnt->GetEntityVisArea()!=NULL, eoot_OCCLUDER))
					return;
				/*			
				bool bRes = !rCB.IsBoxVisible_OCCLUDER(pEnt->m_WSBBox);

				if(&rCB == Get3DEngine()->GetCoverageBuffer())
				{
				pEnt->m_OcclState.bOccluded = bRes;
				if(pEnt->m_OcclState.bOccluded)
				pEnt->m_OcclState.nLastOccludedMainFrameID = GetMainFrameID();
				else
				pEnt->m_OcclState.nLastVisibleMainFrameID = GetMainFrameID();
				}

				if(bRes)
				return;*/
			}
		}
	}

	// mark as rendered in this frame
	//	pEnt->SetDrawFrame( GetFrameID(), m_nRenderStackLevel );

	// update scissor
	SRendParams DrawParams;  

	DrawParams.fDistance = fEntDistance;

	DrawParams.pRenderNode = pEnt;

	// ignore ambient color set by artist
	DrawParams.dwFObjFlags = 0;

	if(nRndFlags&ERF_SELECTED)
		DrawParams.dwFObjFlags |= FOB_SELECTED;

	DrawParams.pShadowMapCasters = NULL;

	DrawParams.dwFObjFlags |= FOB_TRANS_MASK;

	if(GetCVars()->e_HwOcclusionCullingObjects)
	{
		GetRenderer()->EF_StartEf();  

    pEnt->Render(DrawParams);

		GetRenderer()->EF_EndEf3D(SHDF_ZPASS|SHDF_ZPASS_ONLY, -1);
	}

	//	if(GetCVars()->e_HwOcclusionCullingObjects)
	//	pEnt->m_nOccluderFrameId = GetFrameID();

	// add occluders into c-buffer
	if(	!GetCVars()->e_CoverageBuffer )
		return;

	bool bMeshFound = false;

	for(int nEntSlot = 0; nEntSlot<8; nEntSlot++)
	{
		Matrix34A matParent;
		if(CStatObj * pMainStatObj = (CStatObj *)pEnt->GetEntityStatObj(nEntSlot, 0, &matParent, true))
			bMeshFound |= RenderStatObjIntoZBuffer(pMainStatObj, matParent, nRndFlags, bCompletellyInFrustum, rCB, pEnt->GetMaterial());
		else if(ICharacterInstance * pChar = (ICharacterInstance *)pEnt->GetEntityCharacter(nEntSlot, &matParent, true))
		{
			ISkeletonPose * pSkeletonPose = pChar->GetISkeletonPose();
			uint32 numJoints = pSkeletonPose->GetJointCount();

			// go thru list of bones
			for(uint32 i=0; i<numJoints; i++)
			{
				IStatObj * pStatObj = pSkeletonPose->GetStatObjOnJoint(i);
				if(!pStatObj || pStatObj->GetFlags()&STATIC_OBJECT_HIDDEN)
					continue;

				Matrix34A tm34 = matParent*Matrix34(pSkeletonPose->GetAbsJointByID(i));

				if(pStatObj->GetRenderMesh())
					bMeshFound |= RenderStatObjIntoZBuffer(pStatObj, tm34, nRndFlags, bCompletellyInFrustum, rCB, pEnt->GetMaterial());
				else
				{
					// render sub-objects of bone object
					if(int nCount = pStatObj->GetSubObjectCount())
					{
						for (int nSubObjectId=0; nSubObjectId<nCount; nSubObjectId++)
							if(IStatObj * pSubObj = pStatObj->GetSubObject(nSubObjectId)->pStatObj)
								bMeshFound |= RenderStatObjIntoZBuffer(pSubObj, tm34, nRndFlags, bCompletellyInFrustum, rCB, pEnt->GetMaterial());
					}
				}
			}
		}
	}

	if(!bMeshFound)
	{ // voxel objects
		Matrix34A matParent;
    matParent.SetIdentity();
		pEnt->GetEntityStatObj(0, 0, &matParent);

    int nLod = CObjManager::GetObjectLOD(DrawParams.fDistance, pEnt->GetLodRatioNormalized(), 
      pEnt->GetBBox().GetRadius() * 0.5f * GetCVars()->e_CoverageBufferOccludersLodRatio);
    
#define VOX_MAX_LODS_NUM 3

    if(nLod >= min(1+GetCVars()->e_VoxelLodsNum,VOX_MAX_LODS_NUM))
      nLod = min(1+GetCVars()->e_VoxelLodsNum,VOX_MAX_LODS_NUM) - 1;
    
    if(nLod<0)
      nLod=0;

    while(nLod && !pEnt->GetRenderMesh(nLod))
      nLod--;

		if(pEnt->GetRenderMesh(nLod))
			rCB.AddRenderMesh(pEnt->GetRenderMesh(nLod), &matParent, pEnt->GetMaterial(), (nRndFlags&ERF_OUTDOORONLY)!=0, bCompletellyInFrustum,false);
	}

	if(GetCVars()->e_CoverageBufferDebug == 16)
		DrawBBox(pEnt->GetBBox(), Col_Orange);
}

int CObjManager::GetObjectLOD(float fDistance, float fLodRatioNorm, float fRadius)
{
	int nLodLevel = (int)(fDistance*(fLodRatioNorm*fLodRatioNorm)/(max(GetCVars()->e_LodRatio*min(fRadius, GetCVars()->e_LodCompMaxSize),0.001f)));
	return nLodLevel;
}

void CObjManager::FillTerrainTexInfo(IOctreeNode * pOcNode, float fEntDistance, struct SSectorTextureSet * & pTerrainTexInfo, const AABB & objBox)
{
  IVisArea * pVisArea = pOcNode->m_pVisArea;
  CTerrainNode * pTerrainNode = pOcNode->m_pTerrainNode;

  if((!pVisArea || pVisArea->IsAffectedByOutLights()) && pTerrainNode)
  { // provide terrain texture info
    AABB boxCenter; boxCenter.min = boxCenter.max = objBox.GetCenter();
    if(CTerrainNode * pTerNode = pTerrainNode)
      if(pTerNode = pTerNode->FindMinNodeContainingBox(boxCenter))
      {
        while((!pTerNode->m_nNodeTexSet.nTex0 || 
          fEntDistance*2.f*8.f > (pTerNode->m_boxHeigtmapLocal.max.x-pTerNode->m_boxHeigtmapLocal.min.x))
          && pTerNode->m_pParent )
          pTerNode = pTerNode->m_pParent;

        pTerrainTexInfo = &(pTerNode->m_nNodeTexSet);
      }
  }
}

void CObjManager::RenderBrush( CBrush * pEnt, PodArray<CDLight*> * pAffectingLights,
                               const AABB & objBox, 
                               float fEntDistance, 
                               bool bSunOnly, int8 nThreadId, CVisArea * pVisArea, bool nCheckOcclusion)
{

  //	FUNCTION_PROFILER_3DENGINE;

#ifdef _DEBUG
  const char * szName = pEnt->GetName();
  const char * szClassName = pEnt->GetEntityClassName();
#endif // _DEBUG

  if(pEnt->m_dwRndFlags&ERF_HIDDEN)
    return;

  // check cvars
  assert(GetCVars()->e_Brushes);

  // check-allocate RNTmpData for visible objects
  Get3DEngine()->CheckCreateRNTmpData(&pEnt->m_pRNTmpData, pEnt);

  if(pEnt->GetDrawFrame(m_nRenderStackLevel) == GetFrameID())
    return;

  pEnt->SetDrawFrame( GetFrameID(), m_nRenderStackLevel );

  if(nCheckOcclusion && pEnt->m_pOcNode)
    if(GetObjManager()->IsBoxOccluded(pEnt->m_WSBBox, fEntDistance/m_fZoomFactor, &pEnt->m_pRNTmpData->userData.m_OcclState, 
      pEnt->m_pOcNode->m_pVisArea != NULL, eoot_OBJECT))
        return;

  CRenderObject * pObj = pEnt->m_pRNTmpData->userData.pRenderObject;

  pObj->m_fDistance = fEntDistance;

  // set lights bit mask
  if(bSunOnly)
    pObj->m_DynLMMask[m_nRenderThreadListID] = 1;
  else if(pAffectingLights)
    pObj->m_DynLMMask[m_nRenderThreadListID] = m_p3DEngine->BuildLightMask( objBox, pAffectingLights, pVisArea, (pEnt->m_dwRndFlags&ERF_OUTDOORONLY)!=0);

  pEnt->m_pRNTmpData->userData.pRenderObject = GetRenderer()->EF_GetObject(false, pEnt->m_pRNTmpData->userData.pRenderObject->m_Id, false);

  pEnt->Render(NULL, nThreadId);
}

void CObjManager::RenderDecalAndRoad(IRenderNode * pEnt, PodArray<CDLight*> * pAffectingLights,
                               const Vec3 & vAmbColor, const AABB & objBox, 
                               float fEntDistance, const CCamera & rCam,
                               bool bSunOnly, int8 nThreadId)
{
  //	FUNCTION_PROFILER_3DENGINE;

#ifdef _DEBUG
  const char * szName = pEnt->GetName();
  const char * szClassName = pEnt->GetEntityClassName();
#endif // _DEBUG

  // do not draw if marked to be not drawn or already drawn in this frame
  unsigned int nRndFlags = pEnt->GetRndFlags();

  if(nRndFlags&ERF_HIDDEN)
    return;

  EERType eERType = pEnt->GetRenderNodeType();

  // detect bad objects
  float fEntLengthSquared = objBox.GetSize().GetLengthSquared();
  if(eERType != eERType_Light || !_finite(fEntLengthSquared))
  {
    if(fEntLengthSquared > MAX_VALID_OBJECT_VOLUME || !_finite(fEntLengthSquared) || fEntLengthSquared<=0)
    {
      Warning("CObjManager::RenderObject: Object has invalid bbox: %s,%s, Radius = %.2f, Center = (%.1f,%.1f,%.1f)",
        pEnt->GetName(), pEnt->GetEntityClassName(), cry_sqrtf(fEntLengthSquared)*0.5f, 
        pEnt->GetBBox().GetCenter().x, pEnt->GetBBox().GetCenter().y, pEnt->GetBBox().GetCenter().z);
      return; // skip invalid objects - usually only objects with invalid very big scale will reach this point
    }
  }
  else
    pAffectingLights = NULL;

  // allocate RNTmpData for potentially visible objects
  Get3DEngine()->CheckCreateRNTmpData(&pEnt->m_pRNTmpData, pEnt);

  // skip "outdoor only" objects if outdoor is not visible
  if(GetCVars()->e_CoverageBuffer==2 && nRndFlags&ERF_OUTDOORONLY && !Get3DEngine()->GetCoverageBuffer()->IsOutdooVisible())
    return;

  int nFrameId = GetFrameID();
  if(pEnt->GetDrawFrame(m_nRenderStackLevel) == nFrameId)
    return;
  pEnt->SetDrawFrame( nFrameId, m_nRenderStackLevel );

  CVisArea * pVisArea = (CVisArea *)pEnt->GetEntityVisArea();

  // test only near/big occluders - others will be tested on tree nodes level
  if(!objBox.IsContainPoint(rCam.GetPosition()))
    if(eERType == eERType_Light || fEntDistance < pEnt->m_fWSMaxViewDist*GetCVars()->e_OcclusionCullingViewDistRatio)
      if(IsBoxOccluded(objBox, fEntDistance/m_fZoomFactor, &pEnt->m_pRNTmpData->userData.m_OcclState, pVisArea!=NULL, eoot_OBJECT))
        return;

  SRendParams DrawParams;  
  DrawParams.pTerrainTexInfo = NULL;
  DrawParams.dwFObjFlags = 0;
  DrawParams.fDistance = fEntDistance;
  DrawParams.AmbientColor = vAmbColor;
  DrawParams.pRenderNode = pEnt;
  DrawParams.nThreadId = nThreadId;

  // set lights bit mask
  if(bSunOnly)
  {
    DrawParams.nDLightMask = 1;
  }
  else if(pAffectingLights)
  {
    DrawParams.restLightInfo.refPoint = m_p3DEngine->GetEntityRegisterPoint( pEnt );
    DrawParams.nDLightMask = m_p3DEngine->BuildLightMask( objBox, pAffectingLights, pVisArea, (nRndFlags&ERF_OUTDOORONLY)!=0, &DrawParams.restLightInfo);
  }

  DrawParams.nAfterWater = IsAfterWater(objBox.GetCenter(), rCam.GetPosition()) ? 1 : 0;

  // draw bbox
  if (GetCVars()->e_BBoxes)// && eERType != eERType_Light)
  {
    RenderObjectDebugInfo(pEnt, fEntDistance, DrawParams);
  }

  DrawParams.dwFObjFlags |= FOB_TRANS_MASK;

  DrawParams.m_pVisArea	=	pVisArea;

  if(GetCVars()->e_DebugLights && (nRndFlags&ERF_SELECTED) && eERType != eERType_Light)
  {
    int nLightCount = 0;
    for(int i=0; i<32; i++)
      if(DrawParams.nDLightMask & (1<<i))
        nLightCount++;

    if(nLightCount>=GetCVars()->e_DebugLights)
      GetRenderer()->DrawLabel(objBox.GetCenter(), 2, "%d lights", nLightCount);
  }

  DrawParams.nMaterialLayers = pEnt->GetMaterialLayers();

  pEnt->Render(DrawParams);
}

void CObjManager::RenderVegetation(CVegetation * pEnt, PodArray<CDLight*> * pAffectingLights,
                               /*const Vec3 & vAmbColor, */const AABB & objBox, 
                               float fEntDistance, const CCamera & rCam,
                               bool bSunOnly, int8 nThreadId, SSectorTextureSet * pTerrainTexInfo)
{
  //	FUNCTION_PROFILER_3DENGINE;

#ifdef _DEBUG
  const char * szName = pEnt->GetName();
  const char * szClassName = pEnt->GetEntityClassName();
#endif // _DEBUG

  // check cvars
  assert(GetCVars()->e_Vegetation);

  // check-allocate RNTmpData for visible objects
  Get3DEngine()->CheckCreateRNTmpData(&pEnt->m_pRNTmpData, pEnt);

  assert(pEnt->GetDrawFrame(m_nRenderStackLevel) != GetFrameID());
  pEnt->SetDrawFrame( GetFrameID(), m_nRenderStackLevel );

  CRenderObject * pObj = pEnt->m_pRNTmpData->userData.pRenderObject;

  pObj->m_fDistance = fEntDistance;
  pObj->m_nSort = fastround_positive(fEntDistance * 2.0f);

  // set lights bit mask
  if (bSunOnly)
    pObj->m_DynLMMask[m_nRenderThreadListID] = 1;
  else
  if(pAffectingLights)
    pObj->m_DynLMMask[m_nRenderThreadListID] = m_p3DEngine->BuildLightMask( objBox, pAffectingLights, NULL, true);

  if(pTerrainTexInfo)
  {
    if(pObj->m_ObjFlags & (FOB_BLEND_WITH_TERRAIN_COLOR | FOB_AMBIENT_OCCLUSION))
    {
      pObj->m_nTextureID = pTerrainTexInfo->nTex0;
      //pObj->m_nTextureID1 = pTerrainTexInfo->nTex1;
      SRenderObjData *pOD = GetRenderer()->EF_GetObjData(pObj, true);
      pOD->m_fTempVars[0] = pTerrainTexInfo->fTexOffsetX;
      pOD->m_fTempVars[1] = pTerrainTexInfo->fTexOffsetY;
      pOD->m_fTempVars[2] = pTerrainTexInfo->fTexScale;
      pOD->m_fTempVars[3] = pTerrainTexInfo->fTerrainMinZ - GetCamera().GetPosition().z;
      pOD->m_fTempVars[4] = pTerrainTexInfo->fTerrainMaxZ - GetCamera().GetPosition().z;
    }
  }

  // Update wind
  //  if( (nRndFlags&ERF_RECVWIND) && GetCVars()->e_Wind )
  //  SetupWindBending( pEnt, DrawParams, objBox );

  pEnt->m_pRNTmpData->userData.pRenderObject = GetRenderer()->EF_GetObject(false, pEnt->m_pRNTmpData->userData.pRenderObject->m_Id, false);

  pEnt->Render(NULL, nThreadId, pTerrainTexInfo);
}

#include "CryThread.h"

void CObjManager::RenderObject(IRenderNode * pEnt, PodArray<CDLight*> * pAffectingLights,
                               const Vec3 & vAmbColor, const AABB & objBox, 
                               float fEntDistance, const CCamera & rCam,
                               bool bSunOnly, int8 nThreadId)
{
  FUNCTION_PROFILER_3DENGINE;

#ifdef _DEBUG
  const char * szName = pEnt->GetName();
  const char * szClassName = pEnt->GetEntityClassName();
#endif // _DEBUG

  // do not draw if marked to be not drawn or already drawn in this frame
  unsigned int nRndFlags = pEnt->GetRndFlags();

  if(nRndFlags&ERF_HIDDEN)
    return;

  EERType eERType = pEnt->GetRenderNodeType();

//  if(GetCVars()->e_SceneMerging && GetCVars()->e_scene_merging_show_onlymerged && !(nRndFlags & ERF_MERGE_RESULT))
 //   return;

  SSectorTextureSet * pTerrainTexInfo = NULL;
  int dwFObjFlags = 0;

  // check cvars
  switch(eERType)
  {
  case eERType_Brush:
    if(!GetCVars()->e_Brushes) return;
    break;
  case eERType_Vegetation:
    assert(0);
    break;
  case eERType_ParticleEmitter:
    if(!GetCVars()->e_Particles) return;
    break;
  case eERType_Decal:
    if(!GetCVars()->e_Decals) return;
    break;
  case eERType_VoxelObject:
    if(!GetCVars()->e_Voxel) return;
    break;
  case eERType_WaterWave:
    if(!GetCVars()->e_WaterWaves) return;
    break;
  case eERType_Light:
    {
      if(!GetCVars()->e_DynamicLights || !GetCVars()->e_Entities) return;
      CLightEntity * pLightEnt = (CLightEntity*)pEnt;
      if(!rCam.IsSphereVisible_F( Sphere(pLightEnt->m_light.m_Origin, pLightEnt->m_light.m_fRadius) ))
        return;
    }
    break;
  case eERType_Road:
    if(!GetCVars()->e_Roads) return;
    break;
  default:
    if(!GetCVars()->e_Entities) return;
    break;
  }

  // detect bad objects
  float fEntLengthSquared = objBox.GetSize().GetLengthSquared();
  if(eERType != eERType_Light || !_finite(fEntLengthSquared))
  {
    if(fEntLengthSquared > MAX_VALID_OBJECT_VOLUME || !_finite(fEntLengthSquared) || fEntLengthSquared<=0)
    {
      Warning("CObjManager::RenderObject: Object has invalid bbox: %s, %s, Radius = %.2f, Center = (%.1f,%.1f,%.1f)",
        pEnt->GetName(), pEnt->GetEntityClassName(), cry_sqrtf(fEntLengthSquared)*0.5f, 
        pEnt->GetBBox().GetCenter().x, pEnt->GetBBox().GetCenter().y, pEnt->GetBBox().GetCenter().z);
      return; // skip invalid objects - usually only objects with invalid very big scale will reach this point
    }
  }
  else
    pAffectingLights = NULL;

  // allocate RNTmpData for potentially visible objects
  Get3DEngine()->CheckCreateRNTmpData(&pEnt->m_pRNTmpData, pEnt);

  // detect already culled occluder
  if ((nRndFlags & ERF_GOOD_OCCLUDER))
  {
    if (pEnt->m_pRNTmpData->userData.m_OcclState.nLastOccludedMainFrameID == GetMainFrameID())
      return;

    if(GetCVars()->e_CoverageBufferDrawOccluders)
    {
      return;
    }
  }

  // skip "outdoor only" objects if outdoor is not visible
  if(GetCVars()->e_CoverageBuffer==2 && nRndFlags&ERF_OUTDOORONLY && !Get3DEngine()->GetCoverageBuffer()->IsOutdooVisible())
    return;

  if(eERType != eERType_Light)
  {
    int nFrameId = GetFrameID();
    if(pEnt->GetDrawFrame(m_nRenderStackLevel) == nFrameId)
      return;
    pEnt->SetDrawFrame( nFrameId, m_nRenderStackLevel );
  }

  CVisArea * pVisArea = (CVisArea *)pEnt->GetEntityVisArea();

  // test only near/big occluders - others will be tested on tree nodes level
  if(!(nRndFlags&ERF_RENDER_ALWAYS) && !objBox.IsContainPoint(rCam.GetPosition()))
    if(eERType == eERType_Light || fEntDistance < pEnt->m_fWSMaxViewDist*GetCVars()->e_OcclusionCullingViewDistRatio)
      if(IsBoxOccluded(objBox, fEntDistance/m_fZoomFactor, &pEnt->m_pRNTmpData->userData.m_OcclState, pVisArea!=NULL, eoot_OBJECT))
        return;

  if(eERType == eERType_Light)
  {
    int nFrameId = GetFrameID();
    if(pEnt->GetDrawFrame(m_nRenderStackLevel) == nFrameId)
      return;
    pEnt->SetDrawFrame( nFrameId, m_nRenderStackLevel );
  }


  SRendParams DrawParams;  
  DrawParams.pTerrainTexInfo = pTerrainTexInfo;
  DrawParams.dwFObjFlags = dwFObjFlags;
  DrawParams.fDistance = fEntDistance;
  DrawParams.AmbientColor = vAmbColor;
  DrawParams.pRenderNode = pEnt;
  DrawParams.nThreadId = nThreadId;

  // set lights bit mask
  if(bSunOnly)
  {
    DrawParams.nDLightMask = 1;
  }
  else if(pAffectingLights)
  {
    DrawParams.restLightInfo.refPoint = m_p3DEngine->GetEntityRegisterPoint( pEnt );
    DrawParams.nDLightMask = m_p3DEngine->BuildLightMask( objBox, pAffectingLights, pVisArea, (nRndFlags&ERF_OUTDOORONLY)!=0, &DrawParams.restLightInfo);

    // test lights per triangle for big objects, TODO: move this test before CheckAddLight() calls
    if( DrawParams.nDLightMask && eERType != eERType_Unknown && pEnt->m_pRNTmpData && GetCVars()->e_CullLightsPerTriangleMinObjRadius && pEnt->m_pRNTmpData->userData.fRadius && pEnt->m_pRNTmpData->userData.fRadius > GetCVars()->e_CullLightsPerTriangleMinObjRadius )
    {
      if(CStatObj * pStatObj = (CStatObj *)pEnt->GetEntityStatObj(0, 0, NULL))
      {
        if(IRenderMesh * pMesh = pStatObj->GetRenderMesh())
          CObjManager::CullLightsPerTriangle(pMesh, DrawParams.nDLightMask, pEnt->m_pRNTmpData);
      }
      else if(eERType == eERType_VoxelObject || eERType == eERType_Road)
      {
        if(IRenderMesh * pMesh = pEnt->GetRenderMesh(0))
          CObjManager::CullLightsPerTriangle(pMesh, DrawParams.nDLightMask, pEnt->m_pRNTmpData);
      }
    }
  }

  if(GetCVars()->e_Dissolve && !(nRndFlags&ERF_MERGE_RESULT) && !m_nRenderStackLevel)
  { 
    DrawParams.nDissolveRef = GetDissolveRef(fEntDistance - pEnt->m_fWSMaxViewDist, &pEnt->m_pRNTmpData->userData.dissolveTransitionState);
    if(DrawParams.nDissolveRef)
    {
      DrawParams.dwFObjFlags |= FOB_DISSOLVE;
      if(DrawParams.nDissolveRef == 255)
        return;
    }
  }

  DrawParams.nAfterWater = IsAfterWater(objBox.GetCenter(), rCam.GetPosition()) ? 1 : 0;

  if(nRndFlags&ERF_SELECTED)
    DrawParams.dwFObjFlags |= FOB_SELECTED;

  // get list of shadow map frustums affecting this entity
  if( (pEnt->m_nInternalFlags & IRenderNode::PER_OBJECT_SHADOW_PASS_NEEDED) && 
    (GetCVars()->e_ShadowsOnAlphaBlend || (nRndFlags & ERF_SUBSURFSCATTER)) && GetCVars()->e_Shadows && !m_nRenderStackLevel)
    DrawParams.pShadowMapCasters = GetShadowFrustumsList(pAffectingLights, objBox, fEntDistance, 
    DrawParams.nDLightMask, fEntDistance < m_fGSMMaxDistance);

  // draw bbox
  if (GetCVars()->e_BBoxes)// && eERType != eERType_Light)
  {
/*      if(pInstInfo)
      DrawBBox(objBox);
    else*/
      RenderObjectDebugInfo(pEnt, fEntDistance, DrawParams);
  }

//  if(!DrawParams.pTerrainTexInfo && eERType != eERType_Light && (GetCVars()->e_TerrainAo || GetCVars()->e_VegetationUseTerrainColor))
  //  FillTerrainTexInfo(pEnt->m_pOcNode, fEntDistance, DrawParams.pTerrainTexInfo, /*DrawParams.dwFObjFlags, */objBox);

  if(DrawParams.pTerrainTexInfo && GetTerrain()->IsAmbientOcclusionEnabled())
    dwFObjFlags |= FOB_AMBIENT_OCCLUSION;

  DrawParams.dwFObjFlags |= FOB_TRANS_MASK;

  DrawParams.m_pVisArea	=	pVisArea;

  if(GetCVars()->e_DebugLights && (nRndFlags&ERF_SELECTED) && eERType != eERType_Light)
  {
    int nLightCount = 0;
    for(int i=0; i<32; i++)
      if(DrawParams.nDLightMask & (1<<i))
        nLightCount++;

    if(nLightCount>=GetCVars()->e_DebugLights)
      GetRenderer()->DrawLabel(objBox.GetCenter(), 2, "%d lights", nLightCount);
  }

  // Update wind
  if( (nRndFlags&ERF_RECVWIND) && GetCVars()->e_Wind )
    SetupWindBending( pEnt, DrawParams, objBox );

  DrawParams.nMaterialLayers = pEnt->GetMaterialLayers();
	DrawParams.nLod = pEnt->m_pRNTmpData->userData.nLod;

  {
    static CryCriticalSection s_cResLock;
    AUTO_LOCK(s_cResLock); // Not thread safe without this
    pEnt->Render(DrawParams);
  }
}

void CObjManager::SetupWindBending( IRenderNode *pEnt, SRendParams &pDrawParams, const AABB & objBox )
{
  Vec3 pCurrWind( Get3DEngine()->GetWind( objBox, pDrawParams.m_pVisArea != 0 ) );

  // verify if any wind, else no need to proceed

  Vec2 pCurrBending = pEnt->GetWindBending();      

  const float fMaxBending = 5.0f;     
  const float fStiffness = 2.0f;     
  const float fWindResponse = fStiffness * fMaxBending / 10.0f;
  
  // apply current wind    
  pCurrBending = pCurrBending + Vec2(pCurrWind) * (GetTimer()->GetFrameTime()*fWindResponse);

  // apply restoration
  pCurrBending *= max(1.f - GetTimer()->GetFrameTime()*fStiffness, 0.f);

  pEnt->SetWindBending( pCurrBending );   

  pDrawParams.vBending = pCurrBending * fMaxBending / (pCurrBending.GetLength() + fMaxBending);  
}

void CObjManager::RenderObjectDebugInfo(IRenderNode * pEnt, float fEntDistance, const SRendParams & DrawParams)
{
  if(IsRenderIntoShadowmap() || m_nRenderStackLevel)
    return;

	if (GetCVars()->e_BBoxes > 0)
	{
		ColorF color(1,1,1,1);

		if (GetCVars()->e_BBoxes == 2 && pEnt->GetRndFlags()&ERF_SELECTED)
		{
			color.a *= clamp_tpl(pEnt->GetImportance(), 0.5f, 1.f);
			float fFontSize = max(2.f - fEntDistance * 0.01f, 1.f);

			string sLabel = pEnt->GetDebugString();
			if (sLabel.empty())
			{
				sLabel.Format("%s/%s %d/%d", 
					pEnt->GetName(), pEnt->GetEntityClassName(), 
					DrawParams.nDLightMask, DrawParams.pShadowMapCasters ? DrawParams.pShadowMapCasters->Count() : 0);
			}
			GetRenderer()->DrawLabelEx(pEnt->GetBBox().GetCenter(), fFontSize, (float*)&color, true, true, "%s", sLabel.c_str());
		}

		IRenderAuxGeom* pRenAux = GetRenderer()->GetIRenderAuxGeom();
		pRenAux->SetRenderFlags(SAuxGeomRenderFlags());
		pRenAux->DrawAABB(pEnt->GetBBox(), false, color, eBBD_Faceted);
	}
}

void CObjManager::RenderObjectDebugInfo(IRenderNode * pEnt, const CRenderObject * pRendObj)
{
  if (GetCVars()->e_BBoxes > 0)
  {
    ColorF color(1,1,1,1);

    if (GetCVars()->e_BBoxes == 2 && pEnt->GetRndFlags()&ERF_SELECTED)
    {
      color.a *= clamp_tpl(pEnt->GetImportance(), 0.5f, 1.f);
      float fFontSize = max(2.f - pRendObj->m_fDistance * 0.01f, 1.f);

      string sLabel = pEnt->GetDebugString();
      if (sLabel.empty())
      {
        sLabel.Format("%s/%s %d/%d", 
          pEnt->GetName(), pEnt->GetEntityClassName(), 
          pRendObj->m_DynLMMask, pRendObj->m_pShadowCasters ? pRendObj->m_pShadowCasters->Count() : 0);
      }
      GetRenderer()->DrawLabelEx(pEnt->GetBBox().GetCenter(), fFontSize, (float*)&color, true, true, "%s", sLabel.c_str());
    }

    IRenderAuxGeom* pRenAux = GetRenderer()->GetIRenderAuxGeom();
    pRenAux->SetRenderFlags(SAuxGeomRenderFlags());
    pRenAux->DrawAABB(pEnt->GetBBox(), false, color, eBBD_Faceted);
  }
}

bool CObjManager::RenderStatObjIntoZBuffer(IStatObj * pStatObj, Matrix34A & objMatrix, 
																						unsigned int nRndFlags, bool bCompletellyInFrustum,
																						CCullBuffer & rCB, IMaterial * pMat)
{
	assert(pStatObj);

	CStatObj *pCStatObj = (CStatObj*)pStatObj;

	if(!pCStatObj || pCStatObj->m_nFlags&STATIC_OBJECT_HIDDEN)
		return false;

	IRenderMesh *pRenderMeshOcclusion = pCStatObj->m_pRenderMeshOcclusion;
	if (pCStatObj->m_pLod0)
		pRenderMeshOcclusion = pCStatObj->m_pLod0->m_pRenderMeshOcclusion;

  if(!pRenderMeshOcclusion && pCStatObj->GetRenderTrisCount() && pCStatObj->GetRenderTrisCount()==12)
    pRenderMeshOcclusion = pCStatObj->GetRenderMesh(); // solids

	if (pRenderMeshOcclusion)
	{
		rCB.AddRenderMesh(pRenderMeshOcclusion, &objMatrix, Get3DEngine()->GetMaterialManager()->GetDefaultMaterial(), 
      (nRndFlags&ERF_OUTDOORONLY)!=0, bCompletellyInFrustum,true);
	}
	else
	{
		IRenderMesh *pRenderMesh = 0;
		if (pCStatObj->m_nFlags & STATIC_OBJECT_COMPOUND)
		{
			// multi-sub-objects
			for(int s=0,num = pCStatObj->SubObjectCount(); s<num; s++)
			{
				IStatObj::SSubObject &subObj = pCStatObj->SubObject(s);
				if (subObj.pStatObj && !subObj.bHidden && subObj.nType == STATIC_SUB_OBJECT_MESH)
				{
					Matrix34A subObjMatrix = objMatrix * subObj.tm;

					CStatObj *PCSubStatObj = ((CStatObj*)subObj.pStatObj);
					pRenderMesh = PCSubStatObj->m_pRenderMeshOcclusion;
					if (PCSubStatObj->m_pLod0)
						pRenderMesh = PCSubStatObj->m_pLod0->m_pRenderMeshOcclusion;

					if (pRenderMesh)
						rCB.AddRenderMesh(pRenderMesh, &subObjMatrix, pMat, (nRndFlags&ERF_OUTDOORONLY)!=0, bCompletellyInFrustum,true);
				}
			}
		}
	}

	return true;
}

bool CObjManager::RayRenderMeshIntersection(IRenderMesh * pRenderMesh, const Vec3 & vInPos, const Vec3 & vInDir, Vec3 & vOutPos, Vec3 & vOutNormal,bool bFastTest,IMaterial * pMat)
{
	FUNCTION_PROFILER_3DENGINE;

	// get position offset and stride
	int nPosStride = 0;
	byte * pPos  = pRenderMesh->GetPosPtr(nPosStride, FSL_READ);

	// get indices
	uint16 *pInds = pRenderMesh->GetIndexPtr(FSL_READ);
	int nInds = pRenderMesh->GetIndicesCount();
	assert(nInds%3 == 0);

	float fClosestHitDistance = -1;

	if(!pMat)
		pMat = pRenderMesh->GetMaterial();

	Lineseg l0(vInPos+vInDir, vInPos-vInDir);
	Lineseg l1(vInPos-vInDir, vInPos+vInDir);

	Vec3 vHitPoint(0,0,0);

//	bool b2DTest = fabs(vInDir.x)<0.001f && fabs(vInDir.y)<0.001f;

	// test tris
	PodArray<CRenderChunk>& Chunks = pRenderMesh->GetChunks();
	for (int nChunkId=0; nChunkId<Chunks.Count(); nChunkId++)
	{
		CRenderChunk * pChunk = &Chunks[nChunkId];
		if(pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
			continue;

		if(pMat)
		{
			const SShaderItem &shaderItem = pMat->GetShaderItem(pChunk->m_nMatID);
			if (!shaderItem.m_pShader || shaderItem.m_pShader->GetFlags2() & EF2_NODRAW)
				continue;
		}

		AABB triBox;

		int nLastIndexId = pChunk->nFirstIndexId + pChunk->nNumIndices;
		for(int i=pChunk->nFirstIndexId; i<nLastIndexId; i+=3)
		{
			assert(pInds[i+0]<pRenderMesh->GetVerticesCount());
			assert(pInds[i+1]<pRenderMesh->GetVerticesCount());
			assert(pInds[i+2]<pRenderMesh->GetVerticesCount());

			// get vertices
			const Vec3 v0 = (*(Vec3*)&pPos[nPosStride*pInds[i+0]]);
			const Vec3 v1 = (*(Vec3*)&pPos[nPosStride*pInds[i+1]]);
			const Vec3 v2 = (*(Vec3*)&pPos[nPosStride*pInds[i+2]]);
/*
			if(b2DTest)
			{
				triBox.min = triBox.max = v0;
				triBox.Add(v1);
				triBox.Add(v2);
				if(	vInPos.x < triBox.min.x || vInPos.x > triBox.max.x || vInPos.y < triBox.min.y || vInPos.y > triBox.max.y )
					continue;
			}
*/
			// make line triangle intersection
			if(	Intersect::Lineseg_Triangle(l0, v0, v1, v2, vHitPoint) ||
					Intersect::Lineseg_Triangle(l1, v0, v1, v2, vHitPoint) )
			{ 
				if (bFastTest)
					return (true);

				float fDist = vHitPoint.GetDistance(vInPos);
				if(fDist<fClosestHitDistance || fClosestHitDistance<0)
				{
					fClosestHitDistance = fDist;
					vOutPos = vHitPoint;
					vOutNormal = (v1-v0).Cross(v2-v0);
				}
			}
		}
	}

	if(fClosestHitDistance>=0)
	{ 
		vOutNormal.Normalize();
		return true;
	}

	return false;
}

bool CObjManager::RayStatObjIntersection(IStatObj * pStatObj, const Matrix34 & objMatrix, IMaterial * pMat,
																				 Vec3 vStart, Vec3 vEnd, Vec3 & vClosestHitPoint, float & fClosestHitDistance, bool bFastTest)
{
	assert(pStatObj);

	CStatObj *pCStatObj = (CStatObj*)pStatObj;

	if(!pCStatObj || pCStatObj->m_nFlags&STATIC_OBJECT_HIDDEN)
		return false;

	Matrix34 matInv = objMatrix.GetInverted();
	Vec3 vOSStart = matInv.TransformPoint(vStart);
	Vec3 vOSEnd = matInv.TransformPoint(vEnd);
	Vec3 vOSHitPoint(0,0,0), vOSHitNorm(0,0,0);

	Vec3 vBoxHitPoint;
	if(!Intersect::Ray_AABB(Ray(vOSStart, vOSEnd-vOSStart), pCStatObj->GetAABB(), vBoxHitPoint))
		return false;

	bool bHitDetected = false;

	if(IRenderMesh *pRenderMesh = pStatObj->GetRenderMesh())
	{
		if(CObjManager::RayRenderMeshIntersection(pRenderMesh, vOSStart, vOSEnd-vOSStart, vOSHitPoint, vOSHitNorm, bFastTest, pMat))
		{
			bHitDetected = true;
			Vec3 vHitPoint = objMatrix.TransformPoint(vOSHitPoint);
			float fDist = vHitPoint.GetDistance(vStart);
			if(fDist<fClosestHitDistance)
			{
				fClosestHitDistance = fDist;
				vClosestHitPoint = vHitPoint;
			}
		}
	}
	else
	{
		// multi-sub-objects
		for(int s=0,num = pCStatObj->SubObjectCount(); s<num; s++)
		{
			IStatObj::SSubObject &subObj = pCStatObj->SubObject(s);
			if (subObj.pStatObj && !subObj.bHidden && subObj.nType == STATIC_SUB_OBJECT_MESH)
			{
				Matrix34 subObjMatrix = objMatrix * subObj.tm;
				if(RayStatObjIntersection(subObj.pStatObj, subObjMatrix, pMat, vStart, vEnd, vClosestHitPoint, fClosestHitDistance, bFastTest))
					bHitDetected = true;
			}
		}
	}

	return bHitDetected;
}

uint8 CObjManager::GetDissolveRef(float fDistDif, SDissolveTransitionState * pState)
{
  if(GetCVars()->e_Dissolve == 1)
    return (uint8)SATURATEB((1.f + fDistDif)*255.f);

  const float fDistIn  = -0.4f;
  const float fDistOut = -0.3f;
  const float fDistMid = (fDistIn+fDistOut)*0.5f;

  if(!pState->fStartTime)
  { // object just appeared in the view - assign one of final states
    pState->fStartTime = GetCurTimeSec();
    pState->fCurrVal = pState->fStartVal = pState->fEndVal = (fDistDif > fDistMid) ? 1.f : 0.f;
  }
  else if((pState->fStartVal == pState->fEndVal) && (pState->fStartVal == 0.f) && (fDistDif > fDistOut))
  { // start fading out
    pState->fStartTime = GetCurTimeSec();
    pState->fCurrVal = pState->fStartVal = 0.f;
    pState->fEndVal = 1.f;
  }
  else if((pState->fStartVal == pState->fEndVal) && (pState->fStartVal == 1.f) && (fDistDif < fDistIn))
  { // start fading in
    pState->fStartTime = GetCurTimeSec();
    pState->fCurrVal = pState->fStartVal = 1.f;
    pState->fEndVal = 0.f;
  }
  
  if(pState->fStartVal != pState->fEndVal)
  { // progress
    float t = SATURATE((GetCurTimeSec() - pState->fStartTime) / 0.3333f);

    if(pState->fStartVal < pState->fEndVal)
    { // speed up fade out if object is reaching max view distance
      float t_min = SATURATE(1.f - fDistDif/fDistOut);
      t = max(t, t_min);
    }

    pState->fCurrVal = (1.f-t)*pState->fStartVal + (t)*pState->fEndVal;

    if(t==1.f)
    { // finish transition
      pState->fStartVal = pState->fEndVal = pState->fCurrVal;
    }
  }

  return (uint8)SATURATEB(pState->fCurrVal*255.f);
}
