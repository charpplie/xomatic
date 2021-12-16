////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   3denginelight.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Light sources manager
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "3dEngine.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "AABBSV.h"
#include "terrain.h"
#include "VoxTerrain.h"
#include "LightEntity.h"
#include "ObjectsTree.h"
#include "Brush.h"
#include "IrradianceVolumeRenderNode.h"

#ifndef PI
#define PI 3.14159f
#endif

void C3DEngine::RegisterLightSourceInSectors(CDLight * pDynLight, int nSID)
{
	// AntonK: this workaround for colored shadow maps is temporary, since we will render it another way in the future
	if(pDynLight->m_Flags & DLF_SUN || pDynLight->m_Flags & DLF_REFLECTIVE_SHADOWMAP || !m_pTerrain || !pDynLight->m_pOwner)
		return; 

  if(GetRenderer()->EF_IsFakeDLight(pDynLight))
    return;

  // pass to outdoor only outdoor lights and some indoor projectors 
  IVisArea * pLightArea = pDynLight->m_pOwner->GetEntityVisArea();
  if( !pLightArea || ((pDynLight->m_Flags & DLF_PROJECT) && pLightArea->IsConnectedToOutdoor()))
  {
    if(m_bShowTerrainSurface)
      m_pTerrain->RegisterLightMaskInSectors(pDynLight, nSID);

    if(m_pVoxTerrain)
      m_pVoxTerrain->RegisterLightSource(pDynLight);

    if(m_pObjectsTree[nSID])
      m_pObjectsTree[nSID]->AddLightSource(pDynLight);
  }

	if(m_pVisAreaManager)
		m_pVisAreaManager->AddLightSource(pDynLight);
}

ILightSource * C3DEngine::CreateLightSource()
{
	// construct new object
	CLightEntity * pLightEntity = new CLightEntity( );

	m_lstStaticLights.Add(pLightEntity);

	return pLightEntity;
}

void C3DEngine::DeleteLightSource(ILightSource * pLightSource)
{
	if(m_lstStaticLights.Delete((CLightEntity*)pLightSource) || pLightSource == m_pSun)
	{
		delete pLightSource;
		if(pLightSource == m_pSun)
			m_pSun = NULL;
	}
	else 
		assert(!"Light object not found");
}

void CLightEntity::Release(bool)
{ 
	Get3DEngine()->DeleteLightSource(this);
}

void CLightEntity::SetLightProperties(const CDLight & light) 
{ 
	if( m_light.m_fRadius < light.m_fRadius )
		m_bCoverageBufferDirty = true;

//	if( !m_light.m_Origin.IsEquivalent(light.m_Origin, 0.01f) )
	m_bCoverageBufferDirty = true;

	m_light = light; 
  m_light.m_fLightFrustumAngle = CLAMP(m_light.m_fLightFrustumAngle, 0.f, 60.f);

	if(!(m_light.m_Flags & DLF_PROJECT))
		m_light.m_fLightFrustumAngle = 90.f/2.f;

	m_light.m_pOwner = this;

  Get3DEngine()->GetLightEntities()->Delete((ILightSource*)this);



  if(light.m_Flags&DLF_DEFERRED_LIGHT)
    Get3DEngine()->GetLightEntities()->Add((ILightSource*)this);
  else
    Get3DEngine()->GetLightEntities()->InsertBefore((ILightSource*)this, 0);
}

const PodArray<CDLight*> * C3DEngine::GetStaticLightSources()
{
	// tmp solution since .h files are checked out
	static PodArray<CDLight*> lstLights;
	lstLights.Reset();

	for(int i=0; i<m_lstStaticLights.Count(); i++)
	{
		CDLight & light = m_lstStaticLights[i]->GetLightProperties();
		lstLights.Add( &light );
	}

	return &lstLights;
}

void C3DEngine::FindPotentialLightSources()
{
	FUNCTION_PROFILER_3DENGINE;

	static ICVar* pCV_r_wireframe = GetConsole()->GetCVar("r_wireframe");
	if (pCV_r_wireframe && pCV_r_wireframe->GetIVal() == R_WIREFRAME_MODE)
		return;

	static ICVar* pCV_e_sketch_mode = gEnv->pConsole->GetCVar("e_SketchMode");
	if (pCV_e_sketch_mode && pCV_e_sketch_mode->GetIVal() == 4) // 4 is set by setting CV_r_TexelsPerMeter ration to != 0
		return;

	const Vec3 vCamPos = GetCamera().GetPosition();

	for(int i=0; i<m_lstStaticLights.Count(); i++)
	{
		CLightEntity * pLightEntity = (CLightEntity *)m_lstStaticLights[i];		
		CDLight * pLight = &pLightEntity->m_light;

    if(pLight->m_Flags & DLF_DEFERRED_LIGHT)
      break; // process deferred lights in CLightEntity::Render(), deferred lights are stored in the end of this array

    int nRenderNodeMinSpec = (pLightEntity->m_dwRndFlags&ERF_SPEC_BITS_MASK) >> ERF_SPEC_BITS_SHIFT;
    if(!CheckMinSpec(nRenderNodeMinSpec))
      continue;

		float fEntDistance = sqrt(Distance::Point_AABBSq(vCamPos, pLightEntity->GetBBox()))*m_fZoomFactor;
		if(fEntDistance > pLightEntity->m_fWSMaxViewDist)
			continue;

    IMaterial * pMat = pLightEntity->GetMaterial();
    if(pMat)
      pLight->m_Shader = pMat->GetShaderItem();

    if(GetCamera().IsSphereVisible_F( Sphere(pLight->m_Origin,pLight->m_fRadius) ))
    {
      if((pLight->m_Flags & DLF_PROJECT) && (pLight->m_fLightFrustumAngle<90.f) && pLight->m_pLightImage)
      { 
        CCamera lightCam = GetCamera();
        lightCam.SetPosition(pLight->m_Origin);
        Matrix34 entMat = ((ILightSource*)(pLight->m_pOwner))->GetMatrix();
        Matrix33 matRot = Matrix33::CreateRotationVDir( entMat.GetColumn(0) );
        lightCam.SetMatrix(Matrix34(matRot,pLight->m_Origin));
        lightCam.SetFrustum(1, 1, (pLight->m_fLightFrustumAngle*2)/180.0f*gf_PI, 0.1f,pLight->m_fRadius);
        if( !CLightEntity::FrustumIntersection(GetCamera(), lightCam) )
          continue;
      }

  		AddDynamicLightSource(*pLight,pLight->m_pOwner,1,pLightEntity->m_fWSMaxViewDist > 0.f ? fEntDistance / pLightEntity->m_fWSMaxViewDist : 0.f);
    }
	}
}

void C3DEngine::ResetCasterCombinationsCache()
{
	for(int nSunInUse=0; nSunInUse<2; nSunInUse++)
	{
		// clear all allocated lists
		for(ShadowFrustumListsCache::iterator it = m_FrustumsCache[nSunInUse].begin(); it != m_FrustumsCache[nSunInUse].end(); ++it)
			(it->second)->Clear();

		// clear user counters
		for(ShadowFrustumListsCacheUsers::iterator it = m_FrustumsCacheUsers[nSunInUse].begin(); it != m_FrustumsCacheUsers[nSunInUse].end(); ++it)
			it->second = 0;
	}
}

void C3DEngine::DeleteAllStaticLightSources()
{
	for(int i=0; i<m_lstStaticLights.Count(); i++)
		delete m_lstStaticLights[i];
	m_lstStaticLights.Reset();

	m_pSun = NULL;
}

Ang3 ConvertProjAngles(const Ang3& vIn)
{
	Ang3 vOut;
	vOut.x = vIn.x;
	vOut.y = vIn.y;
	vOut.z = vIn.z;
	return vOut;
}

void C3DEngine::AddDynamicLightSource(const class CDLight & LSource, ILightSource *pEnt, int nEntityLightId, float fFadeout)
{
	assert(pEnt && _finite(LSource.m_Origin.x) && _finite(LSource.m_Origin.y) && _finite(LSource.m_fRadius));
	assert(LSource.IsOk());

  if((LSource.m_Flags & DLF_DISABLED) || (LSource.m_Flags & DLF_LOCAL) || (!GetCVars()->e_DynamicLights))
    return;

	if ((LSource.m_Flags & DLF_ONLY_FOR_HIGHSPEC) && (m_LightConfigSpec < CONFIG_HIGH_SPEC))
		return; // check spec settings
  
  if ((LSource.m_Flags & DLF_DEFERRED_LIGHT) && !m_pDeferredShading->GetIVal())
    return; 
  
  if(m_lstDynLights.Count()== GetCVars()->e_DynamicLightsMaxCount)
    Warning("C3DEngine::AddDynamicLightSource: more than %d dynamic light sources created", GetCVars()->e_DynamicLightsMaxCount);
  else if(m_lstDynLights.Count()>GetCVars()->e_DynamicLightsMaxCount)
    return;

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Detect sun case
	////////////////////////////////////////////////////////////////////////////////////////////////

	if((LSource.m_Flags & DLF_SUN && !(GetCVars()->e_CoverageBuffer==2)) || LSource.m_Flags & DLF_REFLECTIVE_SHADOWMAP)
	{ // sun
		if(LSource.m_Color.Max() < 0.01f || !GetCVars()->e_Sun)
			return; // sun disabled
	}
	else	if(GetCVars()->e_DynamicLightsFrameIdVisTest>1 && (GetCVars()->e_CoverageBuffer != 2))
	{
    if(GetCVars()->e_DynamicLightsConsistentSortOrder)
		  if(GetFrameID() - pEnt->GetDrawFrame(0) > MAX_FRAME_ID_STEP_PER_FRAME)
			  return;
	}

	if(GetCVars()->e_DynamicLightsFrameIdVisTest)
	{
		int nMaxReqursion = (LSource.m_Flags & DLF_THIS_AREA_ONLY) ? 2 : 3;
		if(!m_pObjManager || !m_pVisAreaManager || !m_pVisAreaManager->IsEntityVisAreaVisible(pEnt,nMaxReqursion, &LSource) )
		{
			if(LSource.m_Flags & (DLF_SUN|DLF_REFLECTIVE_SHADOWMAP) && m_pVisAreaManager && m_pVisAreaManager->m_bSunIsNeeded)
			{ // sun may be used in indoor even if outdoor is not visible
			}
			else
				return;			
		}
	}

  // distance fading
  const float fFadingRange = 0.25f;
	fFadeout -= (1.f - fFadingRange);
	fFadeout = fFadeout < 0.f ? 0.f : fFadeout;
	fFadeout = 1.f - fFadeout/fFadingRange;
  assert(fFadeout>0); // should be culled earlier

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Try to update present lsource
	////////////////////////////////////////////////////////////////////////////////////////////////

	for (int i=0; i<m_lstDynLights.Count(); i++)
	{
		if( m_lstDynLights[i]->m_pOwner == pEnt )
		{
      // copy lsource (keep old CRenderObject)
      CRenderObject *pObj[MAX_RECURSION_LEVELS];
      memcpy(&pObj[0], &(m_lstDynLights[i]->m_pObject[0]), sizeof(pObj));
      *m_lstDynLights[i] = LSource;
      //reset DLF_CASTSHADOW_MAPS 
      if (!GetCVars()->e_Shadows)
        m_lstDynLights[i]->m_Flags &=~DLF_CASTSHADOW_MAPS;
      memcpy(&m_lstDynLights[i]->m_pObject[0], &pObj[0], sizeof(pObj));

      // !workaround: Needs to decrement reference counter of shader because m_lstDynLights never release light sources
      if (LSource.m_Shader.m_pShader)
        LSource.m_Shader.m_pShader->Release();

			m_lstDynLights[i]->m_pOwner = pEnt;

			// re-init start time
			m_lstDynLights[i]->m_fStartTime = GetTimer()->GetCurrTime();
			m_lstDynLights[i]->m_fLifeTime = LSource.m_fLifeTime;

			m_lstDynLights[i]->m_ProjAngles = ConvertProjAngles(LSource.m_ProjAngles);


			// set base params
			//m_lstDynLights[i]->m_BaseOrigin = m_lstDynLights[i]->m_Origin;

			if ((m_lstDynLights[i]->m_Flags & DLF_SPECULAR_ONLY_FOR_HIGHSPEC) && (m_LightConfigSpec < CONFIG_HIGH_SPEC))
			{
				m_lstDynLights[i]->m_SpecMult = 0;
			}

			m_lstDynLights[i]->SetLightColor(ColorF(LSource.m_Color.r*fFadeout,LSource.m_Color.g*fFadeout,LSource.m_Color.b*fFadeout,LSource.m_Color.a));

			return;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Add new lsource into list and set some parameters
	////////////////////////////////////////////////////////////////////////////////////////////////

	m_lstDynLights.Add(new CDLight);
  *m_lstDynLights.Last() = LSource;

	// add ref to avoid shader deleting
	if (m_lstDynLights.Last()->m_Shader.m_pShader)
		m_lstDynLights.Last()->m_Shader.m_pShader->AddRef();

	m_lstDynLights.Last()->m_fStartTime = GetTimer()->GetCurrTime();
	m_lstDynLights.Last()->m_fLifeTime = LSource.m_fLifeTime;

	//if (!LSource.m_fEndRadius)
	//	m_lstDynLights.Last()->m_fStartRadius = LSource.m_fRadius;

	m_lstDynLights.Last()->m_pOwner = pEnt;

	m_lstDynLights.Last()->m_ProjAngles = ConvertProjAngles(LSource.m_ProjAngles);

	// set base params
	//m_lstDynLights.Last()->m_BaseOrigin = m_lstDynLights.Last()->m_Origin; 

	if ((m_lstDynLights.Last()->m_Flags & DLF_SPECULAR_ONLY_FOR_HIGHSPEC) && (m_LightConfigSpec < CONFIG_HIGH_SPEC))
	{
		m_lstDynLights.Last()->m_SpecMult = 0;
	}


  // Temporary rotation animation solution for GDC09
  /*Vec3 &pAnimRotation = m_lstDynLights.Last()->m_AnimRotation;
  if( pAnimRotation.x + pAnimRotation.y + pAnimRotation.z )
  {
    Matrix33 pAnimRot = Matrix33::CreateRotationAA( pAnimRotation*m_lstDynLights.Last()->m_fStartTime );
    m_lstDynLights.Last()->m_ObjMatrix.SetRotation33( Matrix33(m_lstDynLights.Last()->m_ObjMatrix) * Matrix33::CreateRotationAA( pAnimRotation*m_lstDynLights.Last()->m_fStartTime ) * pAnimRot );
    m_lstDynLights.Last()->m_ProjMatrix = m_lstDynLights.Last()->m_ProjMatrix * pAnimRot;
  }*/
  


	m_lstDynLights.Last()->SetLightColor(ColorF(LSource.m_Color.r*fFadeout,LSource.m_Color.g*fFadeout,LSource.m_Color.b*fFadeout,LSource.m_Color.a));
}

void C3DEngine::PrepareLightSourcesForRendering_0()
{
  FUNCTION_PROFILER_3DENGINE;

  // reset lists of lsource pointers in sectors
  if(m_pTerrain)
  {
    bool bSunFound = m_lstDynLights.Count() && (m_lstDynLights.GetAt(0)->m_Flags & DLF_SUN);
    m_pTerrain->SetSunLightMask(bSunFound ? 1 : 0);
    //ResetDLightMaskInSectors(bSunFound ? 1 : 0);
  }

  //	GetRenderer()->EF_ClearLightsList();
  m_lstDynLightsNoLight.Clear();

  bool bWarningPrinted = false;

  // update lmasks in terrain sectors
  if(m_nRenderStackLevel)

  { // do not delete lsources during recursion, becasue hmap lpasses are shared between levels
    for (int i=0; i<m_nRealLightsNum/*m_lstDynLights.Count()*/; i++)    
    {
      m_lstDynLights[i]->m_Id = -1;
  //    GetRenderer()->EF_ADDDlight(&m_lstDynLights[i]);
    //  assert(m_lstDynLights[i]->m_Id == i);
      //if(m_lstDynLights[i]->m_Id != -1)
        //RegisterLightSourceInSectors(&m_lstDynLights[i]);
    }
  }
  else
  {
    IVisArea * pCameraVisArea = GetVisAreaFromPos(GetCamera().GetPosition());

    for (int i=0; i<m_lstDynLights.Count(); i++)    
    {
      m_lstDynLights[i]->m_Id = -1;

      if( m_lstDynLights[i]->m_pOwner && m_lstDynLights[i]->m_pOwner->GetEntityVisArea())
      { // vis area lsource

        if(!GetCamera().IsSphereVisible_F( Sphere(m_lstDynLights[i]->m_Origin,m_lstDynLights[i]->m_fRadius) ))
        {
          FreeLightSourceComponents(m_lstDynLights[i]);
          m_lstDynLights.Delete(i); i--;
          continue; // invisible
        }

        // check if light is visible thru light area portal cameras
        if(CVisArea * pArea = (CVisArea *)m_lstDynLights[i]->m_pOwner->GetEntityVisArea())
          if(pArea->m_nRndFrameId == GetFrameID() && pArea != (CVisArea *)pCameraVisArea)
        {
          int nCam=0;
          for(; nCam<pArea->m_lstCurCameras.Count(); nCam++)
            if(pArea->m_lstCurCameras[nCam].IsSphereVisible_F( Sphere(m_lstDynLights[i]->m_Origin,m_lstDynLights[i]->m_fRadius) ))
              break;

          if(nCam==pArea->m_lstCurCameras.Count())
          { 
            FreeLightSourceComponents(m_lstDynLights[i]);
            m_lstDynLights.Delete(i); i--;
            continue; // invisible
          }
        }

        // check if lsource is in visible area
        ILightSource * pEnt = m_lstDynLights[i]->m_pOwner;
        if(!pEnt->IsLightAreasVisible() && pCameraVisArea != pEnt->GetEntityVisArea())
        {
          if(m_lstDynLights[i]->m_Flags & DLF_THIS_AREA_ONLY)
          {
            if(pEnt->GetEntityVisArea())
            {
              int nRndFrameId = pEnt->GetEntityVisArea()->GetVisFrameId();
              if(GetFrameID() - pEnt->GetDrawFrame(0) > MAX_FRAME_ID_STEP_PER_FRAME)
              {
                FreeLightSourceComponents(m_lstDynLights[i]);
                m_lstDynLights.Delete(i); i--;
                continue; // area invisible
              }
            }
            else
            {
              FreeLightSourceComponents(m_lstDynLights[i]);
              m_lstDynLights.Delete(i); i--;
              continue; // area invisible
            }
          }
        }
      }
      else
      { // outdoor lsource
        if( !(m_lstDynLights[i]->m_Flags & DLF_DIRECTIONAL) && !m_lstDynLights[i]->m_pOwner->IsLightAreasVisible())
        {
          FreeLightSourceComponents(m_lstDynLights[i]);
          m_lstDynLights.Delete(i); i--;
          continue; // outdoor invisible
        }
      }

      
      if( m_lstDynLights[i]->m_nPostEffect )
        m_pREPostProcess->AddLight( *m_lstDynLights[i] );

      if((m_lstDynLights[i]->m_Flags&DLF_DEFERRED_LIGHT || 
        (!(m_lstDynLights[i]->m_Flags&DLF_DIRECTIONAL)) && m_pDeferredShading->GetIVal() == 3)
				&& !(m_lstDynLights[i]->m_Flags&DLF_REFLECTIVE_SHADOWMAP))	// ignore RSM lights processing
      {
        if( m_pDeferredShading->GetIVal())
        {
					bool bAdded = false;
					if(m_lstDynLights[i]->m_Flags&DLF_IRRAD_VOLUMES)
						bAdded |= CIrradianceVolumeRenderNode::TryInsertLightIntoVolumes(*m_lstDynLights[i]);

					if(!bAdded)
					{
            CDLight * pLight = m_lstDynLights[i];
            CLightEntity * pLightEntity = (CLightEntity *)pLight->m_pOwner;		

            if(GetCVars()->e_Shadows && (pLight->m_Flags & DLF_CASTSHADOW_MAPS) && pLight->m_Id >= 0)
            {
              pLightEntity->UpdateGSMLightSourceShadowFrustum();

              if(pLightEntity->m_pShadowMapInfo)
              {
                pLight->m_pShadowMapFrustums = pLightEntity->m_pShadowMapInfo->pGSM;
                for(int nLod=0; nLod<MAX_GSM_LODS_NUM && pLight->m_pShadowMapFrustums[nLod]; nLod++)
                  pLight->m_pShadowMapFrustums[nLod]->nDLightId = pLight->m_Id;
              }
            }

            if(GetCVars()->e_DynamicLights)
            {
						  GetRenderer()->EF_AddDeferredLight(*m_lstDynLights[i], 1.f);
						  m_nDeferredLightsNum++;
            }
					}
        }

        FreeLightSourceComponents(m_lstDynLights[i]);
        m_lstDynLights.Delete(i); i--;
        continue;
      }

      if(GetRenderer()->EF_IsFakeDLight(m_lstDynLights[i]))
      { // ignored by renderer
        m_lstDynLightsNoLight.Add(m_lstDynLights[i]);
        m_lstDynLights.Delete(i); i--;
        continue; 
      }

      const int32 nMaxLightsNum = max(GetCVars()->e_DynamicLightsMaxCount, MAX_LIGHTS_NUM);
      if(i >= nMaxLightsNum)
      { // ignored by renderer

        assert(i >= nMaxLightsNum );

        if(i >= nMaxLightsNum && !bWarningPrinted && (GetMainFrameID()&7)==7)
        { // no more sources can be accepted by renderer
          Warning( "C3DEngine::PrepareLightSourcesForRendering: No more than %d real lsources allowed on the screen", (int)nMaxLightsNum);
          bWarningPrinted = true;
        }

        FreeLightSourceComponents(m_lstDynLights[i]);
        m_lstDynLights.Delete(i); i--;
        continue; 
      }

      /*if (m_lstDynLights[i]->m_Shader.m_pShader!=0 && (m_lstDynLights[i]->m_Shader.m_pShader->GetLFlags() & LMF_DISABLE))
      { // fake
        assert(0); // should not be called, but no problem
        FreeLightSourceComponents(m_lstDynLights[i]);
        m_lstDynLights.Delete(i); i--;
        continue; 
      }*/

      //if(i != m_lstDynLights[i]->m_Id)
      //  Warning( "C3DEngine::PrepareLightSourcesForRendering: Invalid light source id");

      if(m_lstDynLights[i]->m_Flags & DLF_PROJECT 
        && m_lstDynLights[i]->m_fLightFrustumAngle<90 // actual fov is twice bigger
        && m_lstDynLights[i]->m_pLightImage
        /*&& (m_lstDynLights[i]->m_pLightImage->GetFlags() & FT_FORCE_CUBEMAP)*/)
      { // prepare projector camera for frustum test
        m_arrLightProjFrustums.PreAllocate(i+1,i+1);
        CCamera & cam = m_arrLightProjFrustums[i];
        cam.SetPosition(m_lstDynLights[i]->m_Origin);
        //				Vec3 Angles(m_lstDynLights[i]->m_ProjAngles[1], 0, m_lstDynLights[i]->m_ProjAngles[2]+90.0f);
        //			cam.SetAngle(Angles);
        Matrix34 entMat = ((ILightSource*)(m_lstDynLights[i]->m_pOwner))->GetMatrix();
        Matrix33 matRot = Matrix33::CreateRotationVDir( entMat.GetColumn(0) );
        cam.SetMatrix(Matrix34(matRot,m_lstDynLights[i]->m_Origin));
        cam.SetFrustum(1, 1, (m_lstDynLights[i]->m_fLightFrustumAngle*2)/180.0f*PI, 0.1f,m_lstDynLights[i]->m_fRadius);

      }

    }

    m_nRealLightsNum = m_lstDynLights.Count();
    m_lstDynLights.AddList(m_lstDynLightsNoLight);
/*
    for(int i=m_nRealLightsNum; i<m_lstDynLights.Count(); i++)
    { // ignored by renderer
      m_lstDynLights[i]->m_Id = -1;
      GetRenderer()->EF_ADDDlight(m_lstDynLights[i]);
      assert(m_lstDynLights[i]->m_Id == -1);
    }
*/
    m_lstDynLightsNoLight.Clear();
  }

  if(GetCVars()->e_DynamicLights==2)
    for (int i=0; i<m_lstDynLights.Count(); i++)
    {
      float fSize = 0.05f*(sinf(GetCurTimeSec()*10.f)+2.0f);
      DrawSphere(m_lstDynLights[i]->m_Origin, fSize);
    }
}

int __cdecl C3DEngine__Cmp_LightPos(const void* v1, const void* v2)
{
  CDLight * p1 = ((CDLight*)v1);
  CDLight * p2 = ((CDLight*)v2);

  //from now the sun not need to have a shadowmap, but some part of the engine check only the first light to decide is there sun or not
  if((p1->m_Origin.x) > (p2->m_Origin.x))
    return -1;
  else if((p1->m_Origin.x) < (p2->m_Origin.x))
    return 1;

  return 0;
}

void C3DEngine::PrepareLightSourcesForRendering_1()
{
	FUNCTION_PROFILER_3DENGINE;

  //qsort(m_lstDynLights.GetElements(), m_nRealLightsNum, sizeof(CDLight), C3DEngine__Cmp_CastShadowFlag);

	if(!m_nRenderStackLevel)
  {
   // SortForShadowMask();
  }

	// reset lists of lsource pointers in sectors
  if(m_nRenderStackLevel)
  { // do not delete lsources during recursion, becasue hmap lpasses are shared between levels
    for (int i=0; i<m_nRealLightsNum/*m_lstDynLights.Count()*/; i++)    
    {
      m_lstDynLights[i]->m_Id = -1;
      GetRenderer()->EF_ADDDlight(m_lstDynLights[i]);
      assert(m_lstDynLights[i]->m_Id == i);
      if(m_lstDynLights[i]->m_Id != -1)
      {
        for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
          RegisterLightSourceInSectors(m_lstDynLights[i], nSID);
      }
    }
  }
  else
	{
		for (int i=0; i<m_nRealLightsNum && i<m_lstDynLights.Count(); i++)    
		{
			m_lstDynLights[i]->m_Id = -1;
      m_lstDynLights[i]->m_n3DEngineLightId = i;

      if(m_lstDynLights[i]->m_Flags&DLF_SUN || GetCVars()->e_DynamicLightsConsistentSortOrder)
        CheckAddLight(m_lstDynLights[i]);

      if(m_lstDynLights[i]->m_fRadius >= 0.5f)
      {
        assert(m_lstDynLights[i]->m_fRadius >= 0.5f && !(m_lstDynLights[i]->m_Flags & DLF_FAKE));
        for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
          RegisterLightSourceInSectors(m_lstDynLights[i], nSID);
      }
    }


		for(int i=m_nRealLightsNum; i<m_lstDynLights.Count(); i++)
		{ // ignored by renderer
			m_lstDynLights[i]->m_Id = -1;
			GetRenderer()->EF_ADDDlight(m_lstDynLights[i]);
			assert(m_lstDynLights[i]->m_Id == -1);
		}
	}
}

void C3DEngine::InitShadowFrustums()
{
	FUNCTION_PROFILER_3DENGINE;

  //reset shadow for e_ShadowsMasksLimit
  if (GetCVars()->e_ShadowsMasksLimit>0)
  {
    int nValidCasters = min( (GetCVars()->e_ShadowsMasksLimit*4), m_nRealLightsNum );

    for (int i=nValidCasters; i<m_nRealLightsNum; i++)
    {
		  CDLight * pCurLight = m_lstDynLights[i];
      pCurLight->m_Flags &= ~DLF_CASTSHADOW_MAPS;
    }
  }

	for (int i=0; i<m_nRealLightsNum; i++)
	{
		CDLight * pLight = m_lstDynLights[i];
		CLightEntity * pLightEntity = (CLightEntity *)pLight->m_pOwner;		

		if(GetCVars()->e_Shadows && (pLight->m_Flags & DLF_CASTSHADOW_MAPS) && pLight->m_Id >= 0)
		{
			pLightEntity->UpdateGSMLightSourceShadowFrustum();

			if(pLightEntity->m_pShadowMapInfo)
			{
				pLight->m_pShadowMapFrustums = pLightEntity->m_pShadowMapInfo->pGSM;
				for(int nLod=0; nLod<MAX_GSM_LODS_NUM && pLight->m_pShadowMapFrustums[nLod]; nLod++)
					pLight->m_pShadowMapFrustums[nLod]->nDLightId = pLight->m_Id;
			}
		}

		IMaterial * pMat = pLightEntity->GetMaterial();
		if(pMat)
			pLight->m_Shader = pMat->GetShaderItem();

		if((GetCVars()->e_CoverageBuffer==2) && !(pLight->m_Flags&DLF_SUN))
		{
			if(pLight->m_Flags&DLF_HAS_CBUFFER)
				pLightEntity->CheckUpdateCoverageMask();


			if( GetCVars()->e_CoverageBufferLightsDebugSide>=0 )//&& pLightEntity->GetRndFlags()&ERF_SELECTED )
				pLightEntity->DebugCoverageMask();
		}

    // update copy of light ion the renderer
    if(pLight->m_Id >= 0)
    {
      CDLight * pRndLight = (CDLight*)GetRenderer()->EF_Query(EFQ_LightSource, pLight->m_Id);
      assert(pLight->m_Id == pRndLight->m_Id);
      pRndLight->m_pShadowMapFrustums = pLight->m_pShadowMapFrustums;
      pRndLight->m_Shader = pLight->m_Shader;
      pRndLight->m_Flags = pLight->m_Flags;
    }
	}

	//shadows frustums intersection test
	if (GetCVars()->e_ShadowsDebug == 4)
	{
		for (int i=0; i<m_nRealLightsNum; i++)
		{
			for (int j=(m_nRealLightsNum-1); j>=(i+1); j--)
			{

				CDLight * pLight = m_lstDynLights[i];
				CLightEntity * pLightEntity1 = (CLightEntity *)pLight->m_pOwner;		

				pLight = m_lstDynLights[j];
				CLightEntity * pLightEntity2 = (CLightEntity *)pLight->m_pOwner;		

				pLightEntity1->CheckFrustumsIntersect(pLightEntity2);
			}
		}
	}

	if(GetCVars()->e_Shadows)
		ResetCasterCombinationsCache();
}

void C3DEngine::FreeLightSourceComponents(CDLight *pLight, bool bDeleteLight)
{
	FUNCTION_PROFILER_3DENGINE;

	for (int i=0; i<MAX_RECURSION_LEVELS; i++)
	{
		if (pLight->m_pObject[i])
			GetRenderer()->EF_ObjRemovePermanent(pLight->m_pObject[i]);
		pLight->m_pObject[i]=0;
	}

		//  delete pLight->m_pProjCamera;
		//pLight->m_pProjCamera=0;

		//if(pLight->m_pShader)
		//		SAFE_RELEASE(pLight->m_pShader);

  if(bDeleteLight)
    delete pLight;
}

int __cdecl C3DEngine__Cmp_CastShadowFlag(const void* v1, const void* v2)
{
	CDLight * p1 = *((CDLight**)v1);
	CDLight * p2 = *((CDLight**)v2);

	// move sun first
	if((p1->m_Flags&DLF_SUN) > (p2->m_Flags&DLF_SUN))
		return -1;
	else if((p1->m_Flags&DLF_SUN) < (p2->m_Flags&DLF_SUN))
		return 1;

  // move shadow casters first
	if((p1->m_Flags&DLF_CASTSHADOW_MAPS) > (p2->m_Flags&DLF_CASTSHADOW_MAPS))
		return -1;
	else if((p1->m_Flags&DLF_CASTSHADOW_MAPS) < (p2->m_Flags&DLF_CASTSHADOW_MAPS))
		return 1;
  
  // get some sorting consistency for shadow casters
  if(p1->m_pOwner > p2->m_pOwner)
    return -1;
  else if(p1->m_pOwner < p2->m_pOwner)
    return 1;

	return 0;
}

typedef std::multimap<int, class CDLight*, std::greater<int> > t_mmapLSIntersect;
typedef std::pair<int, class CDLight*> t_pairLSIntersect;

//typedef std::multimap<uint32, class CDLight> t_mmapLightGroups;
//typedef std::pair<uint32, class CDLight> t_pairLightGroups;

//typedef std::pair<t_mmapGeomMeshes::iterator, t_mmapGeomMeshes::iterator> t_mmapGeomMeshRange;

void C3DEngine::SortForShadowMask()
{
/*
  t_mmapLSIntersect mmapLSIntersect;

  mmapLSIntersect.clear();
  //mmapLSIntersect.reserve( m_lstDynLights.Count() );

  //sort lights by number of intersections
  for(int i=0; i<m_lstDynLights.Count(); i++)
  {
    CDLight* pLight1 = m_lstDynLights.GetAt(i);

    //skip sun and fake lights
    if ( (pLight1->m_Flags&DLF_SUN) || (pLight1->m_Flags&DLF_FAKE) || !(pLight1->m_Flags&DLF_CASTSHADOW_MAPS))
      continue;

    int numIntersect = 0;

    for(int j=0; j<m_lstDynLights.Count(); j++)
    {
      if(i==j)
        continue;

      CDLight* pLight2 = m_lstDynLights.GetAt(j);

      //skip sun and fake lights
      if ( (pLight2->m_Flags&DLF_SUN) || (pLight2->m_Flags&DLF_FAKE) || !(pLight2->m_Flags&DLF_CASTSHADOW_MAPS))
        continue;

      f32 fDistLS = (pLight2->m_Origin - pLight1->m_Origin).GetLength(); 

      if (fDistLS < (pLight2->m_fRadius + pLight1->m_fRadius))
        numIntersect++;
    }

    mmapLSIntersect.insert(t_pairLSIntersect(numIntersect,pLight1));
  }

  //FIX: reactivate multimap for lightgroups
  //t_mmapLightGroups LightGroups;
  //LightGroups.clear();

  

  //pack all valid light sources to the non-overlapped lightgroups

  TArray<CDLight*> LightGroups[32];
  for (int i=0; i<32; i++)
  {
    LightGroups[i].SetUse(0);
  }

  int nCurrentLG = -1;

  //add sun to separate channel 
  for(int i=0; i<m_lstDynLights.Count(); i++)
  {
    CDLight* pLight = m_lstDynLights.GetAt(i);

    if (pLight->m_Flags&DLF_SUN)
    {
      nCurrentLG++;
      assert(nCurrentLG<32);
      LightGroups[nCurrentLG].AddElem(pLight);
    }
  }


  //create light groups (share one shadow mask channel per light group) 
	for(t_mmapLSIntersect::const_iterator LSItor = mmapLSIntersect.begin(); LSItor!=mmapLSIntersect.end(); LSItor++ )
  {
    CDLight* pCurrLight = LSItor->second;
    //all created group
    bool bWasAdded = false;
    for(int nLG=0; nLG<=nCurrentLG; nLG++)
    {
      bool bWasIntersected = false;
      for(int i=0; i<LightGroups[nLG].Num(); i++)
      {
        CDLight* pLight = LightGroups[nLG][i];

        //separate intersection test for sun
        if (pLight->m_Flags&DLF_SUN)
        {
          bWasIntersected = true;
          break;
        }

        f32 fDistLS = (pLight->m_Origin - pCurrLight->m_Origin).GetLength(); 

        if (fDistLS < (pLight->m_fRadius + pCurrLight->m_fRadius))
        {
          bWasIntersected = true;
          break;
        }
      }

      if (!bWasIntersected)
      {
        bWasAdded = true;
        LightGroups[nCurrentLG].AddElem(pCurrLight);
        break;
      }
    }

    //start new light group
    if(!bWasAdded)
    {
      nCurrentLG++;
      assert(nCurrentLG<32);

      LightGroups[nCurrentLG].AddElem(pCurrLight);
    }

  }

  //////////////////////////////////////////////////////////////////////////

  //adjust lightgroups for the limitation of continious four-light light groups
  int nLGroup=0;
  while(nLGroup<8)
  {
    int i,j;
    
    int iFirstChan = nLGroup*4;
    int iLastChan = iFirstChan+3;
    int iLightsNum = 0;
    for (i=iFirstChan; i<=iLastChan; i++)
    {
      iLightsNum += LightGroups[i].Num();
    }
    //number of lights which need to be separated
    int nSepNum = (iLightsNum>4)?(iLightsNum%4):0;


    if (nSepNum>0)
    {
      //move lights to other light groups 
      for (i=iLastChan; i>=iFirstChan; i--)
      {
        int nMaxChanLights =  LightGroups[i].Num();
        int nMoveLight = min(nSepNum, nMaxChanLights);

        nCurrentLG++;
        assert(nCurrentLG<32);
        for(j=0; j<nMoveLight; j++)
        {
          LightGroups[nCurrentLG].AddElem( LightGroups[i][0] );
          LightGroups[i].Remove(0);
        }

        nSepNum -= nMoveLight;
        if (nSepNum<=0)
          break;
      }

      //if there was rearrangement then
      //start check for all light groups from the beginning
      nLGroup = 0;
      continue;
    }

    nLGroup++;

  }

  //generate actual final lights list
  PodArray<CDLight> m_lstNewDynLights;

  m_lstNewDynLights.clear();

  for (int i=0; i<32; i++)
  {
    for (int j=0; j<LightGroups[i].Num(); j++)
    {
      CDLight* pLight = LightGroups[i][j];
      pLight->m_nShadowMaskId = i/4;
      pLight->m_nShadowMaskChan = i%4;
      m_lstNewDynLights.Add(*pLight);
    }
  }


  //add all other light sources
  for(int i=0; i<m_lstDynLights.Count(); i++)
  {
    CDLight* pLight1 = m_lstDynLights.GetAt(i);

    if ((pLight1->m_Flags&DLF_FAKE) || !(pLight1->m_Flags&DLF_CASTSHADOW_MAPS))
    m_lstNewDynLights.Add(*pLight1);
  }


  //copy all to actual light list
  m_lstDynLights.Clear();
  m_lstDynLights.AddList(m_lstNewDynLights);

//  if(CV_e_)
  {
    float fTextPosX = 20;
    float fTextPosY = 20;
    float fTextStepY = 17;


	  float color[] = {1,1,1,1};

    for (int i=0; i<32; i++)
    {
      GetRenderer()->Draw2dLabel( fTextPosX, fTextPosY+=fTextStepY, 2, color, false, "Group #%8d | Lights #%8d", i, LightGroups[i].Num() );
    }
  }



*/
};
//t_mmapLightGroups::interator LGitor =  LightGroups.equal_range(nLG);
/*
for(t_mmapLSIntersect::const_iterator LSItor2 = mmapLSIntersect->begin(); LSItor2!=mmapLSIntersect->end(); LSItor2++ )
{
  t_mmapLightGroups::interator LightGroups.find();

}
*/


//#define GI_TEST

void C3DEngine::UpdateLightSources()
{
	FUNCTION_PROFILER_3DENGINE;

  m_nRenderLightsNum = 0;
	GetRenderer()->EF_ClearDeferredLightsList();

	// sort by cast shadow flag
	if(!m_nRenderStackLevel)
		qsort(m_lstDynLights.GetElements(), m_lstDynLights.Count(), sizeof(CDLight*), C3DEngine__Cmp_CastShadowFlag);

	// process lsources
	float fCurrTime = GetTimer()->GetCurrTime();
	for(int i=0; i<m_lstDynLights.Count(); i++)
	{
		CDLight *pLight = m_lstDynLights[i];

		if((pLight->m_fStartTime + pLight->m_fLifeTime+0.001f < fCurrTime) || (pLight->m_fStartTime > (fCurrTime+0.1f)))
		{
			// time is come to delete this lsource
			FreeLightSourceComponents(pLight);
			m_lstDynLights.Delete(i); i--;
			continue;
		}

		bool bDeleteNow = GetRenderer()->EF_UpdateDLight(pLight);
		//pLight->m_Color.Clamp();
		//pLight->m_SpecColor.Clamp();

		/*if(pLight->m_fLifeTime && !pLight->m_Shader.m_pShader)
		{ // evaluate radius
			float fAtten = 1.f - (fCurrTime - pLight->m_fStartTime) / pLight->m_fLifeTime;
			if(fAtten<0)
			{
				fAtten = 0;
				bDeleteNow = true;
			}
			else if(fAtten>1.f)
				fAtten = 1.f;
			pLight->m_fRadius = pLight->m_fBaseRadius*fAtten;
		}*/

		if(bDeleteNow)
		{
			// time is come to delete this lsource
			FreeLightSourceComponents(pLight);
			m_lstDynLights.Delete(i); i--;
			continue;
		}

    if(!m_nRenderStackLevel)
    {
		  SetupLightScissors( m_lstDynLights[i] );
      
#ifdef GI_TEST
      //// testing/research..
      if(m_lstDynLights[i]->m_Flags&DLF_DEFERRED_LIGHT && m_pDeferredShading->GetIVal() == 33)
        SetupIndirectLights( m_lstDynLights[i] );
#endif			

    }
	}
}

//////////////////////////////////////////////////////////////////////////
// testing/research..
#ifdef GI_TEST

const int g_nDirs=256; // rays x face
Vec3	g_vDirs[g_nDirs][6];
Vec3	g_testPos[g_nDirs];
bool	g_bFirst=true;
float	g_vCubeDir[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
float	g_vCubeDir2[6][3]={{0,0,1},{0,0,-1},{1,0,0},{-1,0,0},{0,1,0},{0,-1,0}};

void C3DEngine::SetupIndirectLights(CDLight *pLight)
{
	
	// test one-Bounce GI
		
	const uint32 nEntQueryFlags( rwi_any_hit | rwi_stop_at_pierceable );  
	const int32 nObjectTypes( ent_all );

	float fRadius=pLight->m_fRadius;
	Vec3 v0 = pLight->m_Origin;  
	ray_hit pHit;

	int nSize=(int)sqrt((float)g_nDirs);

	if (g_bFirst)
	{						
		// construct cube rays 
		// (needs to be done only once and it is the same for all light sources)
		for (int k=0;k<6;k++)
		{			
			int nPos=0;
			for (int y=0;y<nSize;y++)
			{		
				for (int x=0;x<nSize;x++,nPos++)
				{		
					Vec3 vDir(g_vCubeDir[k][0],g_vCubeDir[k][1],g_vCubeDir[k][2]); //Vec3(-1,0,0);					
					Vec3 vDir2(g_vCubeDir2[k][0],g_vCubeDir2[k][1],g_vCubeDir2[k][2]);
					Vec3 vDir3=vDir.cross(vDir2);

					float fX=(nSize/2-x)/(float)(nSize/2.0f);
					float fY=(nSize/2-y)/(float)(nSize/2.0f);

					vDir2*=fX;
					vDir3*=fY;

					vDir+=vDir2;
					vDir+=vDir3;
					vDir.Normalize();
					g_vDirs[nPos][k]=vDir*fRadius;
				} //x
			} //y				
		} //k
		g_bFirst=false;
	}
 
	for (int j=0;j<6;j++)
	{
		// raycast in all directions

		for (int k=0;k<g_nDirs;k++)
		{
			if(GetPhysicalWorld()->RayWorldIntersection(v0, g_vDirs[k][j], nObjectTypes, nEntQueryFlags, &pHit, 1))  
			{			
				// store the intersection for later comparison
				// move the light source a bit away from the surface
				g_testPos[k]=pHit.pt + pHit.n * 0.1f;
			}	
			else
				g_testPos[k].Set(0,0,0);
		} //k

		// check if light sources are too close
		int nPos=0; 
		int dirs[2][2]={{1,0},{0,1}};
		for (int y=0;y<nSize;y++)
		{				
			for (int x=0;x<nSize;x++,nPos++)
			{	
				Vec3 vCurrPos=g_testPos[nPos];
				if (vCurrPos.len2()<0.001f)
					continue; // light was removed because too close, don't add
				
				for (int k=0;k<2;k++)
				{		
					int x1=x+dirs[k][0];
					int y1=y+dirs[k][1];
					if (x1>=nSize || y1>=nSize)
						continue;
					if (g_testPos[y1*nSize+x1].len2()<0.001f)
						continue;
					float fDist2=vCurrPos.GetSquaredDistance(g_testPos[y1*nSize+x1]);
					if (fDist2<0.5f)
					{
						// too close, remove this light
						g_testPos[y1*nSize+x1].Set(0,0,0); 
					}
				} //k

				CDLight lBounce = *pLight;
				lBounce.m_Flags |= DLF_DEFERRED_INDIRECT_LIGHT;
				lBounce.m_Origin = vCurrPos;			
								
				// looks better with constant radius...
				//lBounce.m_fRadius=v0.GetDistance(vCurrPos)*0.25f;				

				lBounce.m_fRadius=1.0f; 				

        if(GetCVars()->e_DynamicLights)
        {
				  GetRenderer()->EF_AddDeferredLight( lBounce );
				  m_nDeferredLightsNum++;
        }
			} //x
		} //y
	} //j

	/*
	return;
	//////////////////////////////////////////////////////////////////////////
	
  int nHits = 0;
  ray_hit pBounceHit;

  float fRange = pLight->m_fRadius;

  ColorF pDiffuseAcc = ColorF(0.0f, 0.0f, 0.0f, 0.0f);

  float fBoxRad = cry_sqrtf( pLight->m_fRadius * pLight->m_fRadius * 2.0f );
  //float fBoxRad = pLight->m_fRadius *1.5f;

  //const int nSamples = 512;
  const int nSamplesCount = 8;//m_pDeferredShadingILSamples->GetIVal();
  static Vec3 pOffsets[nSamplesCount] = 
  {
    Vec3(0.333f,0.333f,0.333f),
    Vec3(-0.333f,-0.333f,-0.333f),
    Vec3(-0.333f,0.333f,-0.333f),
    Vec3(0.333f,-0.333f,0.333f),
    Vec3(0.333f,-0.333f,-0.333f),
    Vec3(-0.333f,0.333f,0.333f),
    Vec3(-0.333f,-0.333f,0.333f),
    Vec3(0.333f,0.333f,-0.333f)
  };
  //if( !bInitialized )   
  //{
  //  for(int s = 0; s < nSamples; ++s)
  //  { 
  //    pOffsets[s] = Vec3(Random() *  2.0f - 1.0f, Random() *  2.0f - 1.0f, Random() *  2.0f - 1.0f).normalized();
  //  };
  //  bInitialized = true;
  //}

  //const uint32 nEntQueryFlags( rwi_any_hit | rwi_stop_at_pierceable );  
  //const int32 nObjectTypes( ent_terrain|ent_static|ent_rigid|ent_sleeping_rigid );

  Vec3 pPos = pLight->m_Origin;  
  for(int s = 0; s < nSamplesCount; ++s )
  {
    if(GetPhysicalWorld()->RayWorldIntersection(pPos, fBoxRad * pOffsets[s], nObjectTypes, nEntQueryFlags, &pHit, 1))  
    {
      if( pHit.bTerrain )
        continue;

      int nHitMatID = pHit.idmatOrg;
      if (pHit.pCollider)
      {
        int type = pHit.pCollider->GetiForeignData();
        if( type == PHYS_FOREIGN_ID_STATIC ) //|| type == PHYS_FOREIGN_ID_ENTITY )
        {
          IRenderNode *pRenderNode= (IRenderNode*)pHit.pCollider->GetForeignData(PHYS_FOREIGN_ID_STATIC);
          if (pRenderNode)
          {
            Matrix34A worldTM;
            IStatObj * pStatObj = pRenderNode->GetEntityStatObj( 0, 0, &worldTM );
            //IMaterial *pHitMat = pStatObj->GetMaterial();

            Vec3 arrHitPosAndSurfaceNormal[2];
            arrHitPosAndSurfaceNormal[0] = pHit.pt;
            arrHitPosAndSurfaceNormal[1] = pHit.n;
            
            IMaterial *pHitMat = pRenderNode->GetMaterial(arrHitPosAndSurfaceNormal);
            if( pHitMat )
            {
              SShaderItem &pSH = pHitMat->GetShaderItem();
              if( pSH.m_pShaderResources )
              {
                pDiffuseAcc = 0.0f;
                pBounceHit = pHit; 

//              if( pBounceHit.bTerrain )
//                  continue;
                CDLight pIndirLight = *pLight;
                
                float fAtten = max(0.0f, 1.0f -(pHit.pt - pLight->m_Origin).GetLength() / fBoxRad);
                pIndirLight.m_Color = fAtten * pIndirLight.m_Color * (pSH.m_pShaderResources->GetDiffuseColor()) * (1.0f / (float) nSamplesCount );
                pIndirLight.m_Origin = pHit.pt + pHit.n * pIndirLight.m_fRadius * 0.1f;

                if( fAtten > 0.01f)
                {
                  SetupLightScissors( &pIndirLight );

                  pIndirLight.m_Flags |= DLF_DEFERRED_INDIRECT_LIGHT;
                  GetRenderer()->EF_AddDeferredLight( pIndirLight );

                  m_nDeferredLightsNum++;
                }

                nHits++;
              }
            }
          }
        }
      }
    }
  }
	*/
}
#endif

//////////////////////////////////////////////////////////////////////////
void C3DEngine::SetupLightScissors(CDLight * pLight)
{
  Vec3 vViewVec = pLight->m_Origin - GetCamera().GetPosition(); 
  float fDistToLS =  vViewVec.GetLength();

  bool bProjectiveLight = (pLight->m_Flags & DLF_PROJECT) && pLight->m_pLightImage && !(pLight->m_pLightImage->GetFlags()&FT_REPLICATE_TO_ALL_SIDES);
  bool bInsideLightVolume = fDistToLS<=pLight->m_fRadius ;
  if (bInsideLightVolume && !bProjectiveLight)
  {
    //optimization when we are inside light frustum
    pLight->m_sX = 0;
    pLight->m_sY = 0;
    pLight->m_sWidth  = GetRenderer()->GetWidth();  
    pLight->m_sHeight = GetRenderer()->GetHeight();

    return;
  }

  Matrix44 mProj, mView;
  GetRenderer()->GetProjectionMatrix(mProj.GetData());
  GetRenderer()->GetModelViewMatrix(mView.GetData());

  Vec3 vCenter = pLight->m_Origin;    
  float fRadius = pLight->m_fRadius;  

  const int nMaxVertsToProject = 10;
  int nVertsToProject = 4;
  Vec3 pBRectVertices[nMaxVertsToProject];  

  Vec4 vCenterVS = Vec4(vCenter, 1) * mView;

  if( !bInsideLightVolume )
  {
    // Compute tangent planes
    float r = fRadius;
    float sq_r = r * r;

    Vec3 vLPosVS = Vec3(vCenterVS.x, vCenterVS.y, vCenterVS.z);
    float lx = vLPosVS.x;
    float ly = vLPosVS.y;
    float lz = vLPosVS.z;
    float sq_lx = lx * lx;
    float sq_ly = ly * ly;
    float sq_lz = lz * lz;        

    // Compute left and right tangent planes to light sphere
    float sqrt_d = cry_sqrtf( max( sq_r * sq_lx  - (sq_lx + sq_lz) * ( sq_r - sq_lz), 0.0f) );
    float nx = (r * lx + sqrt_d) / (sq_lx + sq_lz);
    float nz = iszero(lz)? 1.0f : (r - nx * lx) / lz;

    Vec3 vTanLeft = Vec3(nx, 0, nz).normalized();

    nx = (r * lx - sqrt_d) / (sq_lx + sq_lz);
    nz = iszero(lz)? 1.0f : (r - nx * lx) / lz;
    Vec3 vTanRight = Vec3(nx, 0, nz).normalized();

    pBRectVertices[0] = vLPosVS - r * vTanLeft;
    pBRectVertices[1] = vLPosVS - r * vTanRight;

    // Compute top and bottom tangent planes to light sphere
    sqrt_d = cry_sqrtf( max(sq_r * sq_ly  - (sq_ly + sq_lz) * ( sq_r - sq_lz), 0.0f) );
    float ny = (r * ly - sqrt_d) / ( sq_ly + sq_lz);
    nz = iszero(lz)? 1.0f : (r - ny * ly) / lz;
    Vec3 vTanBottom = Vec3(0, ny, nz).normalized();

    ny = (r * ly + sqrt_d) / ( sq_ly + sq_lz );
    nz = iszero(lz)? 1.0f : (r - ny * ly) / lz; 
    Vec3 vTanTop = Vec3(0, ny, nz).normalized();

    pBRectVertices[2] = vLPosVS - r * vTanTop;
    pBRectVertices[3] = vLPosVS - r * vTanBottom;
  }

  if( bProjectiveLight )
  {    

    // todo: improve/simplify projective case

    Vec3 vRight  = pLight->m_ObjMatrix.GetColumn2();
    Vec3 vUp	  = -pLight->m_ObjMatrix.GetColumn1();
    Vec3 pDirFrontN = pLight->m_ObjMatrix.GetColumn0();
    Vec3 pDirFront = pDirFrontN;
    Vec3 pDirBack = -pDirFront;        

    // Cone radius
    float fConeAngleThreshold = 0.0f;
    float fConeRadiusScale = /*min(1.0f,*/ cry_tanf( (pLight->m_fLightFrustumAngle + fConeAngleThreshold) * (PI/180.0f) ) ; //);
    float fConeRadius = fRadius * fConeRadiusScale;

    Vec3 pDiagA = (vUp + vRight);
    float fDiagLen = 1.0f / pDiagA.GetLengthFast();    
    pDiagA *= fDiagLen;

    Vec3 pDiagB = (vUp - vRight);
    pDiagB *= fDiagLen;

    float fPyramidBase =  cry_sqrtf(fConeRadius * fConeRadius * 2.0f);             
    pDirFront *= fRadius;

    Vec3 pEdgeA  = (pDirFront + pDiagA* fPyramidBase);
    Vec3 pEdgeA2 = (pDirFront - pDiagA* fPyramidBase);
    Vec3 pEdgeB  = (pDirFront + pDiagB* fPyramidBase) ;    
    Vec3 pEdgeB2 = (pDirFront - pDiagB* fPyramidBase) ;    

    uint32 nOffset = bInsideLightVolume? 0 : 4;
    // Put all pyramid vertices in view space
    Vec4 pPosVS = Vec4(pLight->m_Origin, 1) * mView;
    pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);
    pPosVS = Vec4(pLight->m_Origin + pEdgeA, 1) * mView;
    pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);
    pPosVS = Vec4(pLight->m_Origin + pEdgeB, 1) * mView;
    pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);
    pPosVS = Vec4(pLight->m_Origin + pEdgeA2, 1) * mView;
    pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);
    pPosVS = Vec4(pLight->m_Origin + pEdgeB2, 1) * mView;
    pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);

    nVertsToProject = nOffset;  
  }

  Vec2 vPMin = Vec2(1,1);
  Vec2 vPMax = Vec2(0,0);
  Vec2 vMin = Vec2(1,1);
  Vec2 vMax = Vec2(0,0);

  if (GetCVars()->e_ScissorDebug)
    mView.Invert();

  // Project all vertices
  for (int i=0; i<nVertsToProject; i++)
  {
    if (GetCVars()->e_ScissorDebug)
    {      
      if (GetRenderer()->GetIRenderAuxGeom()!=NULL)
      {
        Vec4 pVertWS = Vec4( pBRectVertices[i], 1) * mView;
        Vec3 v = Vec3(pVertWS.x, pVertWS.y, pVertWS.z);
        GetRenderer()->GetIRenderAuxGeom()->DrawPoint( v ,RGBA8(0xff,0xff,0xff,0xff),10);

        int32 nPrevVert = (i-1)<0?3:(i-1);
        pVertWS = Vec4( pBRectVertices[nPrevVert], 1) * mView;
        Vec3 v2 = Vec3(pVertWS.x, pVertWS.y, pVertWS.z); 
        GetRenderer()->GetIRenderAuxGeom()->DrawLine( v, RGBA8(0xff,0xff,0x0,0xff), v2, RGBA8(0xff,0xff,0x0,0xff), 3.0f);
      }
    }

    Vec4 vScreenPoint = Vec4(pBRectVertices[i], 1.0) * mProj;

    //projection space clamping
    vScreenPoint.w = max(vScreenPoint.w, 0.00000000000001f);
    vScreenPoint.x = max(vScreenPoint.x, -(vScreenPoint.w));
    vScreenPoint.x = min(vScreenPoint.x, vScreenPoint.w);
    vScreenPoint.y = max(vScreenPoint.y, -(vScreenPoint.w));
    vScreenPoint.y = min(vScreenPoint.y, vScreenPoint.w);

    //NDC
    vScreenPoint /= vScreenPoint.w;

    //output coords
    //generate viewport (x=0,y=0,height=1,width=1)
    Vec2 vWin;
    vWin.x = (1.0f + vScreenPoint.x) *  0.5f;
    vWin.y = (1.0f + vScreenPoint.y) *  0.5f;  //flip coords for y axis

    assert(vWin.x>=0.0f && vWin.x<=1.0f);
    assert(vWin.y>=0.0f && vWin.y<=1.0f);

    if( !bInsideLightVolume && bProjectiveLight && i >= 4)
    {
      // Get light pyramid screen bounds
      vPMin.x = min(vPMin.x, vWin.x);
      vPMin.y = min(vPMin.y, vWin.y);
      vPMax.x = max(vPMax.x, vWin.x);
      vPMax.y = max(vPMax.y, vWin.y);
    }
    else
    {
      // Get light sphere screen bounds
      vMin.x = min(vMin.x, vWin.x);
      vMin.y = min(vMin.y, vWin.y);
      vMax.x = max(vMax.x, vWin.x);
      vMax.y = max(vMax.y, vWin.y);
    }
  }

  float fWidth = (float)GetRenderer()->GetWidth();
  float fHeight = (float)GetRenderer()->GetHeight();

  if( bProjectiveLight && !bInsideLightVolume)
  {
    // Clamp light pyramid bounds to light sphere screen bounds - when not inside light volume
    vMin.x = clamp_tpl<float>( vPMin.x, vMin.x, vMax.x);
    vMin.y = clamp_tpl<float>( vPMin.y, vMin.y, vMax.y);    
    vMax.x = clamp_tpl<float>( vPMax.x, vMin.x, vMax.x);
    vMax.y = clamp_tpl<float>( vPMax.y, vMin.y, vMax.y);
  }

  pLight->m_sX = (short)(vMin.x * fWidth);
  pLight->m_sY = (short)((1-vMax.y) * fHeight);
  pLight->m_sWidth = (short)((vMax.x-vMin.x) * fWidth);
  pLight->m_sHeight = (short)((vMax.y-vMin.y) * fHeight);
}


int __cdecl C3DEngine__Cmp_DLightAmount(const void* v1, const void* v2)
{
	DLightAmount * p1 = ((DLightAmount*)v1);
	DLightAmount * p2 = ((DLightAmount*)v2);

	if(p1->fAmount > p2->fAmount)
		return -1;
	else if(p1->fAmount < p2->fAmount)
		return 1;

	return 0;
}

uint32 C3DEngine::GetFullLightMask()
{
	uint32 nRes=0;
	for(int n=0; n<min(m_nRealLightsNum,m_lstDynLights.Count()); n++)
	{
		const int nId = m_lstDynLights[n]->m_Id;
		assert(nId>=0);
		nRes |= (1<<nId);
	}

	return nRes;
}

float C3DEngine::GetLightAmount(CDLight * pLight, const AABB & objBox)
{
	// find amount of light
  float fDist = cry_sqrtf(Distance::Point_AABBSq(pLight->m_Origin, objBox));
	float fLightAttenuation = (pLight->m_Flags & DLF_DIRECTIONAL) ? 1.f : 1.f - (fDist) / (pLight->m_fRadius);
	if(fLightAttenuation<0)
		fLightAttenuation=0;

	float fLightAmount = 
		(pLight->m_Color.r+pLight->m_Color.g+pLight->m_Color.b)*0.233f + 
		(pLight->m_SpecMult)*0.1f;

	return fLightAmount*fLightAttenuation;
}

uint32 C3DEngine::BuildLightMask( const AABB & objBox )
{
  CVisArea * pArea = (CVisArea *)GetVisAreaFromPos(objBox.GetCenter());

  if(pArea)
  {
    COctreeNode * pObjectsTree = pArea->m_pObjectsTree;

    if(!pObjectsTree)
      return 0;

    pObjectsTree = pObjectsTree->FindNodeContainingBox(objBox);

    if(!pObjectsTree)
      return 0;

    return BuildLightMask( objBox, pObjectsTree->GetAffectingLights(), pArea, false );
  }

  for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
  {
    if(COctreeNode * pObjectsTree = m_pObjectsTree[nSID])
    {
      pObjectsTree = pObjectsTree->FindNodeContainingBox(objBox);

      if(!pObjectsTree)
        continue;

      return BuildLightMask( objBox, pObjectsTree->GetAffectingLights(), pArea, false );
    }
  }

  return 0;
}

uint32 C3DEngine::BuildLightMask( const AABB & objBox, PodArray<CDLight*> * pAffectingLights, CVisArea * pObjArea, bool bObjOutdoorOnly, SRestLightingInfo * pRestLightingInfo)
{
	FUNCTION_PROFILER_3DENGINE;

	assert(m_nRenderStackLevel>=0 && "C3DEngine::BuildLightMask call is allowed only during C3DEngine::Render call");

  static PodArray<DLightAmount> lstLightInfos; lstLightInfos.Clear();

  uint32 nDLightMask = 0;

	// make list of affecting light sources
  for(int i=0; i<pAffectingLights->Count(); i++)
  {
    CDLight * pLight = pAffectingLights->GetAt(i);
    ILightSource * pLightOwner = pLight->m_pOwner;
    CVisArea * pLightArea = (CVisArea*)pLightOwner->GetEntityVisArea();
    bool bThisAreaOnlyLight = (pLight->m_Flags & DLF_THIS_AREA_ONLY) != 0;

    assert(pLight->m_Id>=0 || !GetCVars()->e_DynamicLightsConsistentSortOrder);
    assert(pLightOwner);

		if(pLightArea && bThisAreaOnlyLight && pObjArea != pLightArea)
			if(!pObjArea || !pObjArea->IsPortal())
				continue; // different areas

		if(pLight->m_Flags&DLF_INDOOR_ONLY && bObjOutdoorOnly)
      continue; // indoor-only light is not affecting outdoor-only objects

		// calculate amount of light for object
		if( float fLightAmount = GetLightAmount(pLight, objBox) )
		{
      // check projector frustum
      if(  pLight->m_Flags & DLF_PROJECT && pLight->m_fLightFrustumAngle<90 // actual fov is twice bigger
        && pLight->m_pLightImage!=0 /*&& (pLight->m_pLightImage->GetFlags() & FT_FORCE_CUBEMAP)*/)
      { 
        if (!m_arrLightProjFrustums[pLight->m_n3DEngineLightId].IsAABBVisible_E( objBox ))
          continue;
      }

      // check sphere/bbox intersection
      if(!Overlap::Sphere_AABB(Sphere(pLight->m_Origin, pLight->m_fRadius), objBox))
        continue;

      // check object to light visibility
      if(pObjArea || pLightArea)
        if(BuildLightMask_CheckPortals( pObjArea, pLightArea, pLight, objBox ))
          continue;

      Get3DEngine()->CheckAddLight(pLight);

      DLightAmount & la = lstLightInfos.AddNew();
			la.pLight = pLight;
			la.fAmount = fLightAmount;
		}
	}

	if(!lstLightInfos.Count())
		return 0; // no light sources found

	// sort by light amount
	qsort(&lstLightInfos[0], lstLightInfos.Count(), sizeof(lstLightInfos[0]), C3DEngine__Cmp_DLightAmount);

  // cull lights by coverage buffer
  /*if(GetCVars()->e_CoverageBuffer==2 && pObj->GetRenderNodeType() != eERType_Light)
  {
    const char * szName = pObj->GetName();

    if(!pObj->m_pAffectingLights)
      pObj->m_pAffectingLights = Get3DEngine()->GetAffectingLights(pObj->m_WSBBox, true);

    // limit number of effective light sources
    int nRealCount = 0;
    for(int n=0; nRealCount<GetCVars()->e_DynamicLightsMaxEntityLights && n<lstLightInfos.Count(); n++)
    {
      CDLight * pLight = lstLightInfos[n].pLight;

      if(!(pLight->m_Flags&DLF_SUN))
        if(pObj->m_pAffectingLights->Find(pLightOwner)<0)
          continue;

      nDLightMask |= (1<<lstLightInfos[n].pLight->m_Id);
      nRealCount++;
    }
  }
  else*/
  {
    // limit number of effective light sources
    int nMaxNum = min(GetCVars()->e_DynamicLightsMaxEntityLights, lstLightInfos.Count());
    for(int n=0; n<nMaxNum; n++)
      nDLightMask |= (1<<lstLightInfos[n].pLight->m_Id);

    // accumulate information about rest of the lights
    if(pRestLightingInfo)
    {
      for(int n=nMaxNum; n<lstLightInfos.Count(); n++)
      {
        DLightAmount & la = lstLightInfos[n];

        float fDist = Distance::Point_Point(la.pLight->m_Origin, pRestLightingInfo->refPoint);
        float fLightAttenuation = (la.pLight->m_Flags & DLF_DIRECTIONAL) ? 1.f : 1.f - (fDist) / (la.pLight->m_fRadius);
        if(fLightAttenuation>0)
        {
          pRestLightingInfo->averCol += la.pLight->m_Color * fLightAttenuation;
          pRestLightingInfo->averDir += (la.pLight->m_Origin - pRestLightingInfo->refPoint).GetNormalized() * fLightAttenuation;
        }
      }
    }
  }

  return nDLightMask;
}

bool C3DEngine::BuildLightMask_CheckPortals( CVisArea * pObjArea, CVisArea * pLightArea, CDLight * pLight, const AABB & objBox )
{
  bool bThisAreaOnlyLight = (pLight->m_Flags & DLF_THIS_AREA_ONLY) != 0;

  if(pObjArea) // entity is indoor
  { 
    if( pObjArea != pLightArea )
    {	// try also neighbor areas
      if(pLightArea)
      { // check areas connectivity
        int nSearchDepth;
        if(pLight->m_Flags & DLF_PROJECT && !bThisAreaOnlyLight)
          nSearchDepth = 5 + int(pLightArea->IsPortal()); // allow projector to go depther
        else
          nSearchDepth = 2 + int(bThisAreaOnlyLight==false) + int(pLightArea->IsPortal());

        bool bNearFound = pObjArea->FindVisArea(pLightArea, nSearchDepth, true);
        if(!bNearFound)
          return true; // areas do not much
      }
      else // outdoor light
      { // check connection to outdoor
        if(!pObjArea->IsConnectedToOutdoor() || !pObjArea->m_bAffectedByOutLights)
          return true;
      }

      // construct frustum from light pos and portal, check that object is visible from light position
      if(!(bThisAreaOnlyLight && pLightArea) && pObjArea && !pObjArea->IsPortal() && 
        GetCVars()->e_Shadows && pLight->m_Flags & DLF_CASTSHADOW_MAPS)
      {
        Shadowvolume sv;

        int p = 0;
        for(; p<pObjArea->m_lstConnections.Count(); p++)
        {
          CVisArea * pPortal = pObjArea->m_lstConnections[p];

          assert( pPortal->IsPortal() );

          if(pPortal == pLightArea)
            break; // light is inside of near portal - frustum construction is not possible

          if(int nConnNum = pPortal->m_lstConnections.Count())
          {
            if( (nConnNum==1 && !pLightArea) || 
              pPortal->m_lstConnections[0] == pLightArea || (nConnNum>1 && pPortal->m_lstConnections[1] == pLightArea))
            {
              NAABB_SV::AABB_ShadowVolume(pLight->m_Origin, pPortal->m_boxArea, sv, pLight->m_fRadius);
              if(NAABB_SV::Is_AABB_In_ShadowVolume(sv, objBox))
                break;
            }
          }
        }
        if(p == pObjArea->m_lstConnections.Count())
          return true; // object is not visible from light position
      }
    }
  }
  else if(pLightArea) // entity is outside, light is inside
  {
    if(!bThisAreaOnlyLight && pLightArea->IsConnectedToOutdoor() && pLight->m_Flags & DLF_PROJECT)
    {
      if(!pLightArea->IsPortal())
      {
        Shadowvolume sv;

        int p = 0;
        for(; p<pLightArea->m_lstConnections.Count(); p++)
        {
          CVisArea * pPortal = pLightArea->m_lstConnections[p];

          if(pPortal->m_lstConnections.Count() == 1)
          { // portal to outdoor found
            NAABB_SV::AABB_ShadowVolume(pLight->m_Origin, pPortal->m_boxArea, sv, pLight->m_fRadius);
            if(NAABB_SV::Is_AABB_In_ShadowVolume(sv, objBox))
              break;
          }
        }

        if(p == pLightArea->m_lstConnections.Count())
          return true; // object is not visible from light position
      }
    }
    else 
      return true; // by default indoor lsource should not affect outdoor entity
  }

  return false;
}

void C3DEngine::RemoveEntityLightSources(IRenderNode * pEntity)
{
	for (int i=0; i<m_lstDynLights.Count(); i++)    
	{
		if(m_lstDynLights[i]->m_pOwner == pEntity)
		{
			FreeLightSourceComponents(m_lstDynLights[i]);
			m_lstDynLights.Delete(i);
			i--;
		}
	}
	
	for (int i=0; i<m_lstStaticLights.Count(); i++)    
	{
		if(m_lstStaticLights[i] == pEntity)
		{
			CLightEntity * pLightEntity = (CLightEntity*)m_lstStaticLights[i];
			m_lstStaticLights.Delete(i);
			if(pEntity == m_pSun)
				m_pSun = NULL;
			i--;
		}
	}
}

float C3DEngine::GetLightAmountInRange(const Vec3 &pPos, float fRange, bool bAccurate)
{ 
  static Vec3 pPrevPos(0, 0, 0);  
  static float fPrevLightAmount = 0.0f, fPrevRange = 0.0f;

  CVisAreaManager *pVisAreaMan( GetVisAreaManager() );
  if( !pVisAreaMan )
  {
    return 0.0f;
  }

  // Return cached result instead ( should also check if lights position/range changes - but how todo efficiently.. ? )
  if( fPrevLightAmount != 0.0f && fPrevRange == fRange && pPrevPos.GetSquaredDistance( pPos ) < 0.25f )
  {
    return fPrevLightAmount;
  }
  
  CVisArea *pCurrVisArea( static_cast<CVisArea *>( Get3DEngine()->GetVisAreaFromPos( pPos ) ) );

  Vec3 pAmb;
  if( pCurrVisArea )
    pAmb = pCurrVisArea->GetFinalAmbientColor();
  else
    pAmb = Get3DEngine()->GetSkyColor();
  
  // Take into account ambient lighting first
  float fAmbLength( pAmb.len2() );
  if( fAmbLength > 1.0f )
  {
    // Normalize color (for consistently working with HDR/LDR)
    pAmb /= cry_sqrtf( fAmbLength );
  }

  float fLightAmount( ( pAmb.x + pAmb.y + pAmb.z ) / 3.0f );     

  for(int l(0); l < m_lstStaticLights.Count(); ++l)    
  {     
    CDLight &pLight = m_lstStaticLights[l]->GetLightProperties();
    
    if( (pLight.m_Flags & DLF_FAKE) )
    {
      // Fake light source not needed for light amount
      continue;
    }

    CVisArea *pLightVisArea( static_cast<CVisArea *>( m_lstStaticLights[l]->GetEntityVisArea() ) );

    if( pCurrVisArea != pLightVisArea && (pLight.m_Flags & DLF_THIS_AREA_ONLY) )
    {
      // Not same vis area, skip
      continue;
    }           

    float fAtten = 1.0f;
    float fDist = 0.0f;
    
    // Check if light comes from sun (only directional light source..), if true then no attenuation is required
    if( !(pLight.m_Flags & DLF_DIRECTIONAL) )
    {
      fDist = pLight.m_Origin.GetSquaredDistance(pPos);      
      fAtten = (1.0f - (fDist - fRange * fRange) / (pLight.m_fRadius * pLight.m_fRadius)) ;

      // No need to proceed if attenuation coefficient is too small
      if( fAtten < 0.01f ) 
      {
       continue;
      }

      fAtten = clamp_tpl<float>(fAtten, 0.0f, 1.0f);         
    }

    Vec3 pLightColor( pLight.m_Color.r, pLight.m_Color.g, pLight.m_Color.b );
    float fLightLength( pLightColor.len2() );
    if( fLightLength > 1.0f )
    {
      // Normalize color (for consistently working with HDR/LDR)
      pLightColor /= cry_sqrtf(fLightLength);
    }

    float fCurrAmount( ((pLightColor.x + pLightColor.y + pLightColor.z)/3.0f) * fAtten );         
    if(fCurrAmount < 0.01f)
    {
      // no need to proceed if current lighting amount is too small
      continue;
    }  

    if(bAccurate && (pLight.m_Flags & DLF_CASTSHADOW_MAPS)) 
    {      
      // Ray-cast to each of the 6 'edges' of a sphere, get an average of visibility and use it
      // to attenuate current light amount
      
      float fHitAtten( 0 );
      ray_hit pHit;

      const int nSamples = 6;
      Vec3 pOffsets[nSamples]=
      { 
        Vec3( fRange, 0, 0 ),
        Vec3( -fRange, 0, 0 ),
        Vec3( 0, fRange, 0 ),
        Vec3( 0, -fRange, 0 ),
        Vec3( 0, 0, fRange ),
        Vec3( 0, 0, -fRange ),
      };

      const uint32 nEntQueryFlags( rwi_any_hit | rwi_stop_at_pierceable );
      const int32 nObjectTypes( ent_terrain|ent_static|ent_rigid|ent_sleeping_rigid );

      for(int s(0); s < nSamples; ++s )
      {
        Vec3 pCurrPos( pPos - pOffsets[s] );
        Vec3 pDir(pLight.m_Origin - pCurrPos);           

        if(GetPhysicalWorld()->RayWorldIntersection(pPos, pDir, nObjectTypes, nEntQueryFlags, &pHit, 1))  
        {
          fHitAtten += 1.0f;
        }
      }

      // Average the n edges hits and attenuate light amount factor
      fHitAtten = 1.0f - (fHitAtten / (float) nSamples );  
      fCurrAmount *= fHitAtten; 
    }    

     // Sum up results  
    fLightAmount += fCurrAmount;
  }

  fLightAmount = clamp_tpl<float>(fLightAmount, 0.0f, 1.0f);

  // Cache results
  pPrevPos = pPos;
  fPrevLightAmount = fLightAmount;
  fPrevRange = fRange;

	return fLightAmount;
}
  
ILightSource * C3DEngine::GetSunEntity() 
{ 
	return m_pSun; 
}

PodArray<struct ILightSource*> * C3DEngine::GetAffectingLights(const AABB & bbox, bool bAllowSun)
{
	static PodArray<struct ILightSource*> lstAffectingLights; lstAffectingLights.Clear();

	// fill temporary list
	for(int i=0; i<m_lstStaticLights.Count(); i++)
	{
		CLightEntity * pLightEntity = (CLightEntity *)m_lstStaticLights[i];		
		CDLight * pLight = &pLightEntity->m_light;
		if(pLight->m_Flags&DLF_SUN)
			continue;

		bool bHasCBuffer = (pLight->m_Flags&DLF_HAS_CBUFFER) != 0;
	
		// check occlusion
		if(bHasCBuffer)
			pLightEntity->CheckUpdateCoverageMask();

		if(!bHasCBuffer || pLightEntity->IsBoxAffected(bbox))
		{
			ILightSource * p = (ILightSource*)pLightEntity;
			lstAffectingLights.Add(p);
		}
	}

	// search for same cached list
	for(int nListId=0; nListId<m_lstAffectingLightsCombinations.Count(); nListId++)
	{
		PodArray<struct ILightSource*> * pCachedList = m_lstAffectingLightsCombinations[nListId];
		if(pCachedList->Count() == lstAffectingLights.Count() &&
			!memcmp(pCachedList->GetElements(),lstAffectingLights.GetElements(), lstAffectingLights.Count()*sizeof(ILightSource*)))
			return pCachedList;
	}

	// if not found in cache - create new
	PodArray<struct ILightSource*> * pNewList = new PodArray<struct ILightSource*>;
	pNewList->AddList(lstAffectingLights);
	m_lstAffectingLightsCombinations.Add(pNewList);

	return pNewList;
}

void C3DEngine::UregisterLightFromAccessabilityCache(ILightSource * pLight)
{
	// search for same cached list
	for(int nListId=0; nListId<m_lstAffectingLightsCombinations.Count(); nListId++)
	{
		PodArray<struct ILightSource*> * pCachedList = m_lstAffectingLightsCombinations[nListId];
		pCachedList->Delete(pLight);
	}
}

void C3DEngine::CheckAddLight(CDLight*pLight)
{
  if(pLight->m_Id<0)
  {
    GetRenderer()->EF_ADDDlight(pLight);
    GetRenderLightsNum()++;
    assert(pLight->m_Id>=0);
  }
}

void C3DEngine::OnCasterDeleted(IShadowCaster*pCaster)
{
	{ // make sure pointer to object will not be used somewhere in the renderer
		PodArray<CDLight*> * pLigts = GetDynamicLightSources();
		for(int i=0; i<pLigts->Count(); i++)
		{
			CLightEntity * pLigt = (CLightEntity *)(pLigts->GetAt(i)->m_pOwner);
			pLigt->OnCasterDeleted(pCaster);
		}

		if(m_pSun)
			m_pSun->OnCasterDeleted(pCaster);
	}
}
