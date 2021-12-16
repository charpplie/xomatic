////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   statobjmandraw.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Draw static objects (vegetations)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "terrain.h"
#include "StatObj.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "terrain_sector.h"
#include "3dEngine.h"
#include "CullBuffer.h"

bool CObjManager::IsBoxOccluded_HeightMap 
(
 	const AABB & objBox, 
	float fDistance, 
	EOcclusionObjectType eOcclusionObjectType,
	OcclusionTestClient * pOcclTestVars
)
{ 
#ifdef SUPP_HMAP_OCCL
	// test occlusion by heightmap
  FUNCTION_PROFILER_3DENGINE;

	assert(pOcclTestVars);

	const Vec3 vTopMax(objBox.max);
	const Vec3 vTopMin(objBox.min.x, objBox.min.y, vTopMax.z);

	const int cMainID = GetMainFrameID();

	//unlikely
	const bool cBoxTooLarge = ((vTopMax.x - vTopMin.x)>10000) | ((vTopMax.y - vTopMin.y)>10000);
	IF(cBoxTooLarge, false)
	{
		pOcclTestVars->nLastOccludedMainFrameID = cMainID;
		pOcclTestVars->nTerrainOccLastFrame = 1;
		return true;
	}

	const Vec3 & vCamPos = GetCamera().GetPosition();

	const bool cCamInsideBox = (vCamPos.x<=vTopMax.x) & (vCamPos.x>=vTopMin.x) & (vCamPos.y<=vTopMax.y) & (vCamPos.y>=vTopMin.y);
	IF(cCamInsideBox, false)
	{
		if((eOcclusionObjectType != eoot_OCCLUDER && eOcclusionObjectType != eoot_OCCELL_OCCLUDER))
			pOcclTestVars->nLastVisibleMainFrameID = cMainID;
		pOcclTestVars->nTerrainOccLastFrame = 0;
		return false; // camera inside of box
	}

	int nMaxTestsToScip = (GetVisAreaManager()->m_pCurPortal) ? 3 : 10000;

	// precision in meters for this object
	float fMaxStep = fDistance*GetCVars()->e_TerrainOcclusionCullingPrecision;

	CTerrain * const pTerrain = GetTerrain();

	const float cTerrainOccPrecRatio = GetCVars()->e_TerrainOcclusionCullingPrecisionDistRatio;
	if( ( fMaxStep < (vTopMax.x - vTopMin.x) * cTerrainOccPrecRatio || 
        fMaxStep < (vTopMax.y - vTopMin.y) * cTerrainOccPrecRatio ) && 
			objBox.min.x != objBox.max.x && objBox.min.y != objBox.max.y )
	{
		float dx = (vTopMax.x - vTopMin.x);
		while(dx>fMaxStep)
			dx*=0.5f;

		float dy = (vTopMax.y - vTopMin.y);
		while(dy>fMaxStep)
			dy*=0.5f;

		dy = max(dy,0.001f);
		dx = max(dx,0.001f);

		bool bCameraAbove = vCamPos.z>vTopMax.z;
		
		if(bCameraAbove && eOcclusionObjectType != eoot_TERRAIN_NODE)
		{
			for(float y=vTopMin.y; y<=vTopMax.y; y+=dy)
				for(float x=vTopMin.x; x<=vTopMax.x; x+=dx)
					if(!pTerrain->IntersectWithHeightMap(vCamPos, Vec3(x, y, vTopMax.z), fDistance, nMaxTestsToScip, GetDefSID()))
					{
						//handle post setup of call to IsBoxOccluded_HeightMap:
						if((eOcclusionObjectType != eoot_OCCLUDER && eOcclusionObjectType != eoot_OCCELL_OCCLUDER))
							pOcclTestVars->nLastVisibleMainFrameID = cMainID;
						pOcclTestVars->nTerrainOccLastFrame = 0;
						return false;
					}
		}
		else
		{
			// test only needed edges, note: there are duplicated checks on the corners

			if( (vCamPos.x>vTopMin.x) == bCameraAbove ) // test min x side
				for(float y=vTopMin.y; y<=vTopMax.y; y+=dy)
					if(!pTerrain->IntersectWithHeightMap(vCamPos, Vec3(vTopMin.x, y, vTopMax.z), fDistance, nMaxTestsToScip, GetDefSID()))
					{
						//handle post setup of call to IsBoxOccluded_HeightMap:
						if((eOcclusionObjectType != eoot_OCCLUDER && eOcclusionObjectType != eoot_OCCELL_OCCLUDER))
							pOcclTestVars->nLastVisibleMainFrameID = cMainID;
						pOcclTestVars->nTerrainOccLastFrame = 0;
						return false;
					}

			if( (vCamPos.x<vTopMax.x) == bCameraAbove ) // test max x side
				for(float y=vTopMax.y; y>=vTopMin.y; y-=dy)
					if(!pTerrain->IntersectWithHeightMap(vCamPos, Vec3(vTopMax.x, y, vTopMax.z), fDistance, nMaxTestsToScip, GetDefSID()))
					{
						//handle post setup of call to IsBoxOccluded_HeightMap:
						if((eOcclusionObjectType != eoot_OCCLUDER && eOcclusionObjectType != eoot_OCCELL_OCCLUDER))
							pOcclTestVars->nLastVisibleMainFrameID = cMainID;
						pOcclTestVars->nTerrainOccLastFrame = 0;
						return false;
					}

			if( (vCamPos.y>vTopMin.y) == bCameraAbove ) // test min y side
				for(float x=vTopMax.x; x>=vTopMin.x; x-=dx)
					if(!pTerrain->IntersectWithHeightMap(vCamPos, Vec3(x, vTopMin.y, vTopMax.z), fDistance, nMaxTestsToScip, GetDefSID()))
					{
						//handle post setup of call to IsBoxOccluded_HeightMap:
						if((eOcclusionObjectType != eoot_OCCLUDER && eOcclusionObjectType != eoot_OCCELL_OCCLUDER))
							pOcclTestVars->nLastVisibleMainFrameID = cMainID;
						pOcclTestVars->nTerrainOccLastFrame = 0;
						return false;
					}

			if( (vCamPos.y<vTopMax.y) == bCameraAbove ) // test max y side
				for(float x=vTopMin.x; x<=vTopMax.x; x+=dx)
					if(!pTerrain->IntersectWithHeightMap(vCamPos, Vec3(x, vTopMax.y, vTopMax.z), fDistance, nMaxTestsToScip, GetDefSID()))
					{
						//handle post setup of call to IsBoxOccluded_HeightMap:
						if((eOcclusionObjectType != eoot_OCCLUDER && eOcclusionObjectType != eoot_OCCELL_OCCLUDER))
							pOcclTestVars->nLastVisibleMainFrameID = cMainID;
						pOcclTestVars->nTerrainOccLastFrame = 0;
						return false;
					}
		}
		pOcclTestVars->nLastOccludedMainFrameID = cMainID;
		pOcclTestVars->nTerrainOccLastFrame = 1;
		return true;
	}
	else
	{
		Vec3 vTopMid = (vTopMin+vTopMax)*0.5f; 
		if( pTerrain->IntersectWithHeightMap(vCamPos, vTopMid, fDistance, nMaxTestsToScip, GetDefSID()))
		{
			pOcclTestVars->nLastOccludedMainFrameID = cMainID;
			pOcclTestVars->nTerrainOccLastFrame = 1;
			return true;
		}
	}
	//handle post setup of call to IsBoxOccluded_HeightMap:
	if((eOcclusionObjectType != eoot_OCCLUDER && eOcclusionObjectType != eoot_OCCELL_OCCLUDER))
		pOcclTestVars->nLastVisibleMainFrameID = cMainID;
	pOcclTestVars->nTerrainOccLastFrame = 0;
#endif
	return false;
}

#ifdef SUPP_HWOBJ_OCCL
bool CObjManager::IsBoxOccluded_HWOcclQuery( const AABB & objBox, float fDistance, OcclusionTestClient * pOcclTestVars )
{
	// construct RE if needed
	if(!pOcclTestVars->pREOcclusionQuery)
	{
		pOcclTestVars->pREOcclusionQuery = (CREOcclusionQuery *)GetRenderer()->EF_CreateRE(eDATA_OcclusionQuery);
		pOcclTestVars->pREOcclusionQuery->m_pRMBox = (CRenderMesh2*)m_pRMBox;
	}

	// define bbox
	float fBorder = 0.01f;
	pOcclTestVars->pREOcclusionQuery->m_vBoxMin = objBox.min-Vec3(fBorder,fBorder,fBorder);
	pOcclTestVars->pREOcclusionQuery->m_vBoxMax = objBox.max+Vec3(fBorder,fBorder,fBorder);

	// if some new object enters into view - test visibility right now
//	bool bForceCheckNow = false;
	//if(pOcclTestVars->nLastVisibleMainFrameID < GetMainFrameID()-1 && pOcclTestVars->nLastOccludedMainFrameID < GetMainFrameID()-1)
		//bForceCheckNow = true;

	bool bReadResultWasReadImmediately = false;

/*	if((pOcclTestVars->bOccluded && (GetCVars()->e_HwOcclusionCullingObjects<3 || pOcclTestVars->nInstantTestRequested)) || bForceCheckNow || pOcclTestVars->nInstantTestRequested)
	{ // if was not visible - make test immediately
		FRAME_PROFILER( "CObjManager::IsBoxOccluded_HWOcclQuery::_NOW_", GetSystem(), PROFILE_3DENGINE );
		bReadResultWasReadImmediately = true;
		pOcclTestVars->pREOcclusionQuery->mfDraw(NULL, NULL);
		pOcclTestVars->pREOcclusionQuery->mfReadResult_Now();
		pOcclTestVars->bOccluded = pOcclTestVars->pREOcclusionQuery->m_nVisSamples < 120;

		pOcclTestVars->nInstantTestRequested = false;
	}
	else*/
	{ // if was visible start next lazy test once previous one is ready
//		pOcclTestVars->nInstantTestRequested = false;

//		if((GetMainFrameID()&15)==((((uint64)(UINT_PTR)pOcclTestVars->pREOcclusionQuery)>>7)&15))
		if(!pOcclTestVars->pREOcclusionQuery->m_nDrawFrame || pOcclTestVars->pREOcclusionQuery->mfReadResult_Try())
		{
			bool bNewOccluded = (pOcclTestVars->pREOcclusionQuery->m_nVisSamples < 120);
      pOcclTestVars->bOccluded = bNewOccluded;
			/*if(bNewOccluded && !pOcclTestVars->bOccluded)
			{
				pOcclTestVars->nInstantTestRequested = true;
			}
			else*/
			{
				pOcclTestVars->bOccluded = pOcclTestVars->pREOcclusionQuery->m_nVisSamples < 120;
				SShaderItem shIt(m_pShaderOcclusionQuery);
				GetRenderer()->EF_AddEf(pOcclTestVars->pREOcclusionQuery,shIt,GetIdentityCRenderObject(),EFSLIST_DECAL);
			}
		}
	}

	if(GetCVars()->e_HwOcclusionCullingObjects == 2)
	{
		float fVis = pOcclTestVars->bOccluded ? 0.f : 1.f;
		float fCol[] = {fVis,!fVis,1,1};
		char szText[32];
		strcpy(szText, fVis ? "V" : "N");
		if(bReadResultWasReadImmediately)
			strcat(szText, "_now");

		GetRenderer()->DrawLabelEx(objBox.GetCenter(), 2, fCol, true, true, "%s", szText);

		DrawBBox(pOcclTestVars->pREOcclusionQuery->m_vBoxMin,pOcclTestVars->pREOcclusionQuery->m_vBoxMax);
	}

	return pOcclTestVars->bOccluded!=0;
}
#endif

#ifdef USE_CULL_QUEUE
//use a simplified version relying on occlusion buffer
bool CObjManager::IsBoxOccluded( const AABB & objBox, 
																float fDistance, 
																OcclusionTestClient * const __restrict pOcclTestVars, 
																bool/* bIndoorOccludersOnly*/, 
																EOcclusionObjectType eOcclusionObjectType )
{
	// if object was visible during last frames
	const uint32 mainFrameID = GetMainFrameID();
  if(pOcclTestVars->nLastVisibleMainFrameID > mainFrameID - 16)
  {
    // prevent checking all objects in same frame
    int nId = (int)(uint64(pOcclTestVars)/256);
    if((nId&7) != (mainFrameID&7))
      return false;
  }
	
	if(GetCVars()->e_CoverageBuffer)
	{
		CullQueue().AddItem(objBox, fDistance, pOcclTestVars, mainFrameID);
		return pOcclTestVars->nLastOccludedMainFrameID == mainFrameID-1;
	}
	pOcclTestVars->nLastVisibleMainFrameID = mainFrameID;
	return false;
}

#else 

bool CObjManager::IsBoxOccluded( const AABB & objBox, 
																float fDistance, 
																OcclusionTestClient * const __restrict pOcclTestVars, 
																bool bIndoorOccludersOnly, 
																EOcclusionObjectType eOcclusionObjectType )
{
	assert(pOcclTestVars);
	assert(fDistance>=0);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Check if we can return cached results
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// if object was visible during last frames
	const uint32 mainFrameID = GetMainFrameID();
  if(/*eOcclusionObjectType == eoot_OBJECT && */pOcclTestVars->nLastVisibleMainFrameID > mainFrameID - 16)
  {
    // prevent checking all objects in same frame
    int nId = (int)(uint64(pOcclTestVars)/256);
    if((nId&7) != (mainFrameID&7))
      return false;
  }
  FUNCTION_PROFILER_3DENGINE;
  if(GetCVars()->e_TerrainOcclusionCulling==2)
  {
    bool bOccl = pOcclTestVars->nLastOccludedMainFrameID == mainFrameID-1;
    DrawBBox(objBox, bOccl ? Col_Black : Col_White);
  }

	// in recursion return result of base level test if result is not too old
	if(m_nRenderStackLevel)
	{

		if(GetCVars()->e_RecursionOcclusionCulling && pOcclTestVars->nLastOccludedMainFrameID > mainFrameID-32)
			return true;
		else
			return false;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Check if camera is inside of object bbox
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if(objBox.IsContainSphere(GetCamera().GetPosition(), -0.05f))
	{
		pOcclTestVars->nLastVisibleMainFrameID = mainFrameID;
		return false;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Anti-portals
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if(GetVisAreaManager()->IsOccludedByOcclVolumes(objBox, bIndoorOccludersOnly))
	{
		pOcclTestVars->nLastOccludedMainFrameID = mainFrameID;
		return true;
	}
#ifdef SUPP_HWOBJ_OCCL
  if( GetCVars()->e_HwOcclusionCullingObjects && IsBoxOccluded_HWOcclQuery( objBox, fDistance, pOcclTestVars ))
  {

    pOcclTestVars->nLastOccludedMainFrameID = mainFrameID;
    return true;
  }
#endif
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// C-Buffer
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//this turns for PS3 into an async query writing mainFrameID to pOcclTestVars->nLastOccludedMainFrameID if occluded
  if(	GetCVars()->e_CoverageBuffer && 
			fDistance > Get3DEngine()->GetCoverageBuffer()->GetZNearInMeters() &&
			!Get3DEngine()->GetCoverageBuffer()->IsObjectVisible(objBox, eOcclusionObjectType, fDistance, &pOcclTestVars->nLastOccludedMainFrameID))
	{
		pOcclTestVars->nLastOccludedMainFrameID = mainFrameID;
		return true;
	}

  if((GetCVars()->e_VoxTer && GetCVars()->e_Voxel))
    return false;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// HM
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if(GetCVars()->e_TerrainOcclusionCullingVersion)
  {
    if(!bIndoorOccludersOnly && GetCVars()->e_TerrainOcclusionCulling && (fDistance>32.f))
			return GetTerrain()->IsBoxOccluded( objBox, fDistance, (eOcclusionObjectType == eoot_TERRAIN_NODE), pOcclTestVars, GetDefSID() );
		if((eOcclusionObjectType != eoot_OCCLUDER && eOcclusionObjectType != eoot_OCCELL_OCCLUDER))
			pOcclTestVars->nLastVisibleMainFrameID = mainFrameID;
  }
  else
  {
		if(!bIndoorOccludersOnly && GetCVars()->e_TerrainOcclusionCulling && (fDistance>32.f))
		{
			if(IsBoxOccluded_HeightMap( objBox, fDistance, eOcclusionObjectType, pOcclTestVars))
				return true;
		}
		//else handled by IsBoxOccluded_HeightMap
		else	
		if((eOcclusionObjectType != eoot_OCCLUDER && eOcclusionObjectType != eoot_OCCELL_OCCLUDER))
			pOcclTestVars->nLastVisibleMainFrameID = mainFrameID;
	}
	return false;
}
#endif//USE_CULL_QUEUE

void CObjManager::PrefechObjects()
{
	for (LoadedObjects::iterator it = m_lstLoadedObjects.begin(); it != m_lstLoadedObjects.end(); ++it)
	{
		CStatObj * pStatObj = (*it);
		SRendParams params;
		params.nDLightMask = 1;
		GetRenderer()->EF_StartEf();
		pStatObj->Render(params);
		GetRenderer()->EF_EndEf3D(0, -1);
	}
}
