/*=============================================================================
  D3DShadows.cpp : shadows support.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include <IEntityRenderState.h>
#include "../Common/Shadow_Renderer.h"
#include "D3DIrradianceVolume.h"

#include "I3DEngine.h"

namespace
{
  CryCriticalSection g_cDynTexLock;
}

void CD3D9Renderer::EF_PrepareShadowGenRenderList()
{
  if (CV_r_UseShadowsPool)
    return;

  int nThreadID  = m_RP.m_nFillThreadID;
  int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID]-1;

  int NumDynLights = m_RP.m_DLights[nThreadID][nCurRecLevel].Num();

  //TFIX nCurRecLevel+1 is incorrect
  TArray<CDLight>&  arrDeferLights = CDeferredShading::Instance().GetLights( nThreadID, nCurRecLevel);

  if (NumDynLights<=0 && arrDeferLights.Num()<=0)
    return;

  for (int nLightID=0; nLightID < NumDynLights; nLightID++)
  {
    CDLight* pLight =  &m_RP.m_DLights[nThreadID][nCurRecLevel][nLightID];
    EF_PrepareShadowGenForLight(pLight, nLightID);
  }

  for(uint32 nDeferLightID=0; nDeferLightID < arrDeferLights.Num(); nDeferLightID++)
  {
    CDLight* pLight =  &arrDeferLights[nDeferLightID];
    EF_PrepareShadowGenForLight(pLight, (NumDynLights+nDeferLightID) );
  }

}

void CD3D9Renderer::EF_PrepareShadowGenForLight(CDLight* pLight, int nLightID)
{
	assert((unsigned int) nLightID < (MAX_REND_LIGHTS+MAX_DEFERRED_LIGHTS));
  int nThreadID  = m_RP.m_nFillThreadID;
  int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID]-1;

  if (!(pLight->m_Flags & DLF_CASTSHADOW_MAPS))
    return;

  int nLightFrustumID = nLightID * MAX_SHADOWMAP_LOD;

  //fix
  //if (!(pLight->m_Flags & DLF_DIRECTIONAL))
  //{
  //  return;
  //}

#if defined (DIRECT3D10) && !defined(PS3)
  if (CV_r_ShadowGenGS!=0 && pLight->m_Flags & DLF_DIRECTIONAL)
  {
    assert(0);
    PrepareDepthMap( pLight );
    return;
  }
#endif

  ShadowMapFrustum** ppSMFrustumList = pLight->m_pShadowMapFrustums;
  //assert( ppSMFrustumList != 0); fail with differed lights
  if (!ppSMFrustumList)
    return;

  ShadowMapFrustum* pCopyFrustum = NULL;

  int32 nCurLOD = 0;//max(nFrustumLOD, nCaster);

  //render all valid shadow frustums
  SRendItem::m_StartFrust[nThreadID][nLightID] = m_RP.m_SMFrustums[nThreadID][nCurRecLevel].Num();
  for(int nCaster = 0; *ppSMFrustumList && nCaster!=MAX_GSM_LODS_NUM; ++ppSMFrustumList, ++nCaster )
  {       
    //TODO: check does light cast shadow here or in PrepareDepthMap
	//NICK: please fix that, it lead to a crash on PS3
    //assert( (*ppSMFrustumList)->nDLightId == nLightID );
	  if( (*ppSMFrustumList)->nDLightId != nLightID ) 
      continue;

    int nShadowRecur = nLightFrustumID + nCurLOD;

    //start new shadowgen recursion level
    if (!((*ppSMFrustumList)->bOmniDirectionalShadow))
    {
      SRendItem::m_ShadowsStartRI[nThreadID][nShadowRecur] = SRendItem::RendItems(nThreadID,SG_SORT_GROUP,EFSLIST_SHADOW_GEN).Num();
    }

    bool bShadowPass = PrepareShadowGenForFrustum(*ppSMFrustumList, pLight , nLightFrustumID);

    //copy current frustum (should be called after PrepareShadowGenForFrustum)
    if (bShadowPass)
    {
      //AUTO_LOCK(g_cDynTexLock);
      pCopyFrustum = m_RP.m_SMFrustums[nThreadID][nCurRecLevel].AddIndex(1);
      memcpy(pCopyFrustum, *ppSMFrustumList, sizeof(ShadowMapFrustum));
      //ToFix
      //pCopyFrustum->ppDepthTex = &((*ppSMFrustumList)->pDepthTex);
      nCurLOD++;
    }

    //should be num()
    if (bShadowPass)
    {
      if (!((*ppSMFrustumList)->bOmniDirectionalShadow))
      {
        SRendItem::m_ShadowsEndRI[nThreadID][nShadowRecur] = SRendItem::RendItems(nThreadID,SG_SORT_GROUP,EFSLIST_SHADOW_GEN).Num();
      }
    }
  }
  SRendItem::m_EndFrust[nThreadID][nLightID] = m_RP.m_SMFrustums[nThreadID][nCurRecLevel].Num();
 
}

bool CD3D9Renderer::PrepareShadowGenForFrustum(ShadowMapFrustum* pCurFrustum, CDLight* pLight, int nLightFrustumID)
{

  int nThreadID  = m_RP.m_nFillThreadID;
  assert(SRendItem::m_RecurseLevel[nThreadID] == 1);

  PROFILE_FRAME(PrepareShadowGenForFrustum);

  //validate shadow frustum
  assert(pCurFrustum);
  if(!pCurFrustum)
    return false;
  if (!(pCurFrustum->pCastersList))
    return false;
  if (pCurFrustum->pCastersList->Count() <= 0)
    return false;
//////////////////////////////////////////////////////////////////////////

  if(pCurFrustum->bOmniDirectionalShadow == true)
  {
    int nnn = 0;
  }

  //regenerate on reset device
  if (pCurFrustum->nResetID != m_nFrameReset)
  {
    pCurFrustum->nResetID = m_nFrameReset;
    pCurFrustum->RequestUpdate();
  }

  int nShadowGenGPU = 0;

  if(IsMultiGPUModeActive() && CV_r_ShadowGenMode==1)
  {
    //TOFIx: make m_nFrameSwapID - double buffered
    nShadowGenGPU = gRenDev->m_nFrameSwapID % gRenDev->m_nGPUs;

    pCurFrustum->nOmniFrustumMask = 0x3F;
    //in case there was switch on the fly - regenerate all faces
    if(pCurFrustum->nInvalidatedFrustMask[nShadowGenGPU] > 0) 
      pCurFrustum->nInvalidatedFrustMask[nShadowGenGPU] = 0x3F;
  }

  SDynTexture_Shadow *pTX = NULL;
  if(pCurFrustum->pDepthTex)
  {
    pTX = pCurFrustum->pDepthTex;

    bool bNotNeedUpdate = false;
    if (pCurFrustum->bOmniDirectionalShadow)
      bNotNeedUpdate = !(pCurFrustum->nInvalidatedFrustMask[nShadowGenGPU] & pCurFrustum->nOmniFrustumMask);
    else
      bNotNeedUpdate = !pCurFrustum->isUpdateRequested(nShadowGenGPU);

    if ( pTX && pTX->m_pTexture && pTX==pCurFrustum->pDepthTex && pTX->pLightOwner==pCurFrustum->pLightOwner &&
      bNotNeedUpdate )
      return true;            

    //force full shadow map rebuild
    if ( !pTX || !(pTX->m_pTexture) || pTX!=pCurFrustum->pDepthTex || pTX->pLightOwner!=pCurFrustum->pLightOwner)
      pCurFrustum->RequestUpdate();

    if (!CV_r_ShadowGen)
      return true;
  }

  //////////////////////////////////////////////////////////////////////////
  //  update is requested - we should generate new shadow map
  //////////////////////////////////////////////////////////////////////////
  //force unwrap frustum
  if (CV_r_ShadowsDeferredMode && pCurFrustum->bOmniDirectionalShadow)
    pCurFrustum->bUnwrapedOmniDirectional = true;
  else 
    pCurFrustum->bUnwrapedOmniDirectional = false;

  pCurFrustum->bUseHWShadowMap = false;

  ETEX_Type eTT = (pCurFrustum->bOmniDirectionalShadow && !pCurFrustum->bUnwrapedOmniDirectional) ? eTT_Cube : eTT_2D;

  //////////////////////////////////////////////////////////////////////////
  //recalculate LOF rendering params
  //////////////////////////////////////////////////////////////////////////

  //calc texture resolution and frustum resolution
  pCurFrustum->nTexSize = max(pCurFrustum->nTexSize, 32);
  pCurFrustum->nTextureWidth = pCurFrustum->nTexSize;
  pCurFrustum->nTextureHeight = pCurFrustum->nTexSize;
  pCurFrustum->nShadowMapSize = pCurFrustum->nTexSize;

  if (pCurFrustum->bUnwrapedOmniDirectional)
  {
    pCurFrustum->nTextureWidth = pCurFrustum->nTexSize * 3;
    pCurFrustum->nTextureHeight = pCurFrustum->nTexSize * 2;
  }

  //////////////////////////////////////////////////////////////////////////
  //Select shadow buffers format
  //////////////////////////////////////////////////////////////////////////
  //fallback formats


#if defined (DIRECT3D10)
  //dx10 default 
  ETEX_Format eTF = eTF_D32F; //eTF_D24S8;



#else
  //dx9 default fallback
  ETEX_Format eTF = eTF_R32F;
#endif

#if defined (DIRECT3D10)
    if ( pCurFrustum->bForSubSurfScattering || pCurFrustum->bUseAdditiveBlending /*|| eTT != eTT_2D*/)
    {
      //Force eTF_G16R16 usage



      eTF = eTF_G16R16F;

      pCurFrustum->bHWPCFCompare = false;
      pCurFrustum->bUseFilter = true;
      pCurFrustum->bUseHWShadowMap = false;
    }
    else 
    {
#if !defined(PS3)
      if (CV_r_shadowtexformat==4 )
      {
        eTF = eTF_D32F;//eTF_D24S8;
      }
      else
      {
        eTF = eTF_D24S8;
      }
#endif
      pCurFrustum->bUseHWShadowMap = true;
      if (pCurFrustum->bOmniDirectionalShadow && !(pCurFrustum->bUnwrapedOmniDirectional))
      {
        pCurFrustum->bHWPCFCompare = false;
      }
      else
      {
        pCurFrustum->bHWPCFCompare = true;
      }
    }














#else
    //TODO: make per frustum PCF enabling
    pCurFrustum->bHWPCFCompare = false; 

    //use hw shadow buffers for 2d shadow maps for now
    if (CV_r_shadowtexformat==3 && eTT == eTT_2D && m_FormatDF24.IsValid())
    {
      //Force eTF_DF24 usage
      eTF = eTF_DF24;
      pCurFrustum->bUseHWShadowMap = true;
      pCurFrustum->bHWPCFCompare = false;
    }

    if (CV_r_shadowtexformat==4 && eTT == eTT_2D && m_FormatD24S8.IsValid())
    { //use hw shadow buffers for 2d shadow maps for now
      eTF = eTF_D24S8;
      pCurFrustum->bUseHWShadowMap = true;
      pCurFrustum->bHWPCFCompare = true;
    }

    if (CV_r_shadowtexformat==5 && eTT == eTT_2D && m_FormatD16.IsValid())
    { //use hw shadow buffers for 2d shadow maps for now
      eTF = eTF_D16;
      pCurFrustum->bUseHWShadowMap = true;
      pCurFrustum->bHWPCFCompare = true;
    }

    if (CV_r_shadowtexformat==2 && eTT == eTT_2D && m_FormatR32F.IsValid())
    {
      //Force eTF_R32F usage
      eTF = eTF_R32F;
    }

    if(CV_r_shadowtexformat==1 && m_bDeviceSupportsR16FRendertarget)
    {
      eTF = eTF_R16F;
    }

    //Custom depth buffer frustums' format handling
    //scattering only (for ATI cards)
    //add few functions to the PixFormat like support filtering, support srgbread etc
    //scattering only (for NVIDIA cards)
    if ( (pCurFrustum->bForSubSurfScattering || pCurFrustum->bUseAdditiveBlending ) && eTT == eTT_2D)
    {
      if (m_FormatG16R16F.IsValid() && m_bDeviceSupportsFP16Filter)
      {
        eTF = eTF_G16R16F;
        pCurFrustum->bHWPCFCompare = true;
        pCurFrustum->bUseFilter = true;
        pCurFrustum->bUseHWShadowMap = false;
      }
      else //TOFIX: disable bilinear filtering for ATI G16R16 for now; it produces filtering artifacts
        if (m_FormatG16R16.IsValid() && m_bDeviceSupportsG16R16Filter)
        {
          eTF = eTF_G16R16;
          //TOFIX: bilinear filtering for ATI G16R16 is enabled for now; it produces filtering artifacts;
          pCurFrustum->bHWPCFCompare = true;
          pCurFrustum->bUseFilter = true;
          pCurFrustum->bUseHWShadowMap = false;
        }
        else
          if ( m_FormatG16R16F.IsValid() )
          {
            eTF = eTF_G16R16F;
            pCurFrustum->bHWPCFCompare = false;
            pCurFrustum->bUseFilter = false;
            pCurFrustum->bUseHWShadowMap = false;
          }
          else 
          {
            assert(0);
          }
    }
#endif

  //depth shift for precision increasing
  if ( (pCurFrustum->bForSubSurfScattering || pCurFrustum->bUseAdditiveBlending) && eTF == eTF_G16R16 ) //special case
  {
    pCurFrustum->bNormalizedDepth = true;
  } 
  else
  if ( !pCurFrustum->bUseHWShadowMap && pCurFrustum->fNearDist > 1000.f ) //check for sun
  {
    pCurFrustum->bNormalizedDepth = false;
  }
  else 
  {
    pCurFrustum->bNormalizedDepth = true;
  }

//////////////////////////////////////////////////////////////////////////

  //assign requested texture format
  pCurFrustum->m_eReqTF = eTF;
  pCurFrustum->m_eReqTT = eTT;
/*
  if(pTX)
  {
    if( pTX->m_eTF!=pCurFrustum->m_eReqTF || pTX->m_eTT!=pCurFrustum->m_eReqTT || pTX->pLightOwner!=pCurFrustum->pLightOwner ||
      pTX->m_nReqWidth!=pCurFrustum->nTextureWidth ||  pTX->m_nReqHeight!=pCurFrustum->nTextureHeight)
    {
      SAFE_DELETE(pTX);

      //force all cubemap faces update
      pCurFrustum->RequestUpdate();
    }
  }

  //check after freeing texture
  if(!pTX)
  {
    int nRTFlags = pCurFrustum->bUseHWShadowMap?FT_USAGE_DEPTHSTENCIL:0;
    nRTFlags |= FT_STATE_CLAMP; 

    pTX = new SDynTexture_Shadow(pCurFrustum->nTextureWidth, pCurFrustum->nTextureHeight, pCurFrustum->m_eReqTF, pCurFrustum->m_eReqTT, nRTFlags, "ShadowRT");
    assert(pCurFrustum->nTextureWidth == pTX->m_nWidth && pCurFrustum->nTextureHeight == pTX->m_nHeight);
  }

  pCurFrustum->pDepthTex = pTX;
*/

  //assign owners
  pCurFrustum->pFrustumOwner = pCurFrustum;
  //pTX->pLightOwner = pCurFrustum->pLightOwner;
//////////////////////////////////////////////////////////////////////////


  //enable view space shadow maps
  static ICVar * p_e_gsm_view_dependent = iConsole->GetCVar("e_GsmViewSpace");
  bool bViewSpace = (p_e_gsm_view_dependent->GetIVal() > 0) && pCurFrustum->bAllowViewDependency;

  //actual view camera position 
  Vec3 vCamOrigin = iSystem->GetViewCamera().GetPosition();

  CCamera tmpCamera;

  int nSides = 1;
  if (pCurFrustum->bOmniDirectionalShadow)
    nSides = 6;


  Matrix44 m;

  m_RP.m_TI[m_RP.m_nFillThreadID].m_PersFlags |= RBPF_SHADOWGEN;
  iSystem->GetI3DEngine()->SetRenderIntoShadowmap(true);

  for (int nS=0; nS<nSides; nS++)
  {
    //update check for shadow frustums
    if (pCurFrustum->bOmniDirectionalShadow)
    {
      if (!( (pCurFrustum->nInvalidatedFrustMask[nShadowGenGPU] & pCurFrustum->nOmniFrustumMask) & (1 << nS) ))
      {
        continue;
      }
      else
      {
        pCurFrustum->nInvalidatedFrustMask[nShadowGenGPU] &= ~(1 << nS);
      }
    }
    else
    {
      pCurFrustum->nInvalidatedFrustMask[nShadowGenGPU] = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    // Calc frustum CCamera for current frustum
    //////////////////////////////////////////////////////////////////////////
    if (!pCurFrustum->bOmniDirectionalShadow)
    {
      if (pCurFrustum->m_Flags & DLF_PROJECT)
      {
        CShadowUtils::GetCubemapFrustumForLight(pLight, 0, 2*pLight->m_fLightFrustumAngle, &(pCurFrustum->mLightProjMatrix), &(pCurFrustum->mLightViewMatrix), false);
        //TF enable linear shadow space and disable this back faces for projectors
        //m_RP.m_TI.m_PersFlags |= RBPF_MIRRORCULL;
      }
      else
      {
        CShadowUtils::GetShadowMatrixOrtho( pCurFrustum->mLightProjMatrix,  
          pCurFrustum->mLightViewMatrix,  
          m_CameraMatrix, pCurFrustum, bViewSpace);
      }
      //Pre-multiply matrices
      Matrix44r mViewProj =  Matrix44r(pCurFrustum->mLightViewMatrix) * Matrix44r(pCurFrustum->mLightProjMatrix);
      pCurFrustum->mLightViewMatrix = mViewProj;

      //recalc projection matrix with different aspect and slope 
      /*if (lof->m_Flags & DLF_PROJECT)
      {
      CShadowUtils::GetCubemapFrustumForLight(pLight, 0, 2*pLight->m_fLightFrustumAngle, &(lof->mLightProjMatrix), &(lof->mLightViewMatrix), false); 
      //TF enable linear shadow space and disable this back faces for projectors
      Matrix44r mViewProj =  Matrix44r(lof->mLightViewMatrix) * Matrix44r(lof->mLightProjMatrix);
      lof->mLightViewMatrix = mViewProj;//lof->mLightViewMatrix * lof->mLightProjMatrix;
      }*/
    }
    else
    {
      Vec3 vPos = pCurFrustum->vLightSrcRelPos + pCurFrustum->vProjTranslation;

      float sCubeVector[6][7] = 
      {
        {1,0,0,  0,0,1, -90}, //posx
        {-1,0,0, 0,0,1,  90}, //negx
        {0,1,0,  0,0,-1, 0},  //posy
        {0,-1,0, 0,0,1,  0},  //negy
        {0,0,1,  0,1,0,  0},  //posz
        {0,0,-1, 0,1,0,  0},  //negz 
      };
      Vec3 vForward = Vec3(sCubeVector[nS][0], sCubeVector[nS][1], sCubeVector[nS][2]);
      Vec3 vUp      = Vec3(sCubeVector[nS][3], sCubeVector[nS][4], sCubeVector[nS][5]);
      Matrix33 matRot = Matrix33::CreateOrientation(vForward, vUp, DEG2RAD(sCubeVector[nS][6]));

      float fMinDist = pCurFrustum->fNearDist;
      float fMaxDist = pCurFrustum->fFarDist;
      tmpCamera.SetMatrix(Matrix34(matRot, vPos));
      if (!(pCurFrustum->bUnwrapedOmniDirectional))
      {
        tmpCamera.SetFrustum(pCurFrustum->nShadowMapSize, pCurFrustum->nShadowMapSize, (float)DEG2RAD_R(90.0f), fMinDist, fMaxDist);
      }
      else 
      {
        tmpCamera.SetFrustum(pCurFrustum->nShadowMapSize, pCurFrustum->nShadowMapSize, (float)DEG2RAD_R(CShadowUtils::g_fOmniShadowFov), fMinDist, fMaxDist);
      }
    }

  //////////////////////////////////////////////////////////////////////////
/*    {
      int nThreadID  = m_RP.m_nProcessThreadID;

      //end previous recursion
      int nPrevShadowRecur = SRendItem::m_ShadowGenRecurLevel[nThreadID];
      SRendItem::m_ShadowsEndRI[nThreadID][nPrevShadowRecur] = SRendItem::RendItems(nThreadID,SG_SORT_GROUP,EFSLIST_SHADOW_GEN).Num();

      int32 nCurLOD = max(nFrustumLOD, nS);
      int nLightID = 0;
      int nShadowRecur = nLightFrustumID + nCurLOD;

      //start new shadowgen recursion level
      SRendItem::m_ShadowsStartRI[nThreadID][nShadowRecur] = SRendItem::RendItems(nThreadID,SG_SORT_GROUP,EFSLIST_SHADOW_GEN).Num();
      SRendItem::m_ShadowGenRecurLevel[nThreadID] = nShadowRecur;
    }*/
//////////////////////////////////////////////////////////////////////////
    int nShadowRecur = 0;
    if (pCurFrustum->bOmniDirectionalShadow)
    {
      nShadowRecur = nLightFrustumID + nS;
      SRendItem::m_ShadowsStartRI[nThreadID][nShadowRecur] = SRendItem::RendItems(nThreadID,SG_SORT_GROUP,EFSLIST_SHADOW_GEN).Num();
    }

    for(int i = 0; i<pCurFrustum->pCastersList->Count(); i++)
    { 
      IShadowCaster* pEnt  = (*pCurFrustum->pCastersList)[i];
      AABB aabb = pEnt->GetBBoxVirtual();

      //TOFIX reactivate OmniDirectionalShadow
      if (pCurFrustum->bOmniDirectionalShadow)
      {
        //!!! Reactivate proper camera
        bool bVis = true;//tmpCamera.IsAABBVisible_F( aabb );
        if (!bVis)
            continue;
      }

      if( (pCurFrustum->m_Flags & DLF_DIFFUSEOCCLUSION) && pEnt->HasOcclusionmap( 0, pCurFrustum->pLightOwner) )
        continue;

      //SRenderObjectModifier rParms;
      //rParms.nTechniqueID = TTYPE_SHADOWGEN; 
      //rParms.fDistance = sqrtf(Distance::Point_AABBSq(vCamOrigin,aabb));
      //rParms.nStatesInUse = true;

      gEnv->p3DEngine->RenderRenderNode(pEnt, NULL);
    }

		// Add decals and roads for RSMs
		if(pCurFrustum->bReflectiveShadowMap)
			EF_AddClientPolys();

    if (pCurFrustum->bOmniDirectionalShadow)
    {
      SRendItem::m_ShadowsEndRI[nThreadID][nShadowRecur] = SRendItem::RendItems(nThreadID,SG_SORT_GROUP,EFSLIST_SHADOW_GEN).Num();
    }
  }//nSides

  m_RP.m_TI[m_RP.m_nFillThreadID].m_PersFlags &= ~RBPF_SHADOWGEN;
  iSystem->GetI3DEngine()->SetRenderIntoShadowmap(false);

  return true;
}

void CD3D9Renderer::BlurShadow(ShadowMapFrustum *pFr)
{
  SDynTexture_Shadow *tpSrc = pFr->pDepthTex;
  if (!tpSrc || !tpSrc->m_pTexture || tpSrc->m_eTT != eTT_2D)
    return;
  int nSizeX = tpSrc->m_nWidth;
  int nSizeY = tpSrc->m_nHeight;
  if (m_LogFile)
    Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "   Blur shadow map %d\n", tpSrc->m_nUniqueID);
  ETEX_Format eTF = tpSrc->m_eTF;

	SDynTexture_Shadow *tpSrcTemp = new SDynTexture_Shadow(nSizeX, nSizeY, eTF, eTT_2D,  FT_STATE_CLAMP, "BluredShadowRT");

  FX_ResetPipe();

  STexState sTexState = (eTF == eTF_G16R16 || eTF == eTF_G16R16F || eTF == eTF_R32F) ? STexState(FILTER_POINT, true) : STexState(FILTER_LINEAR, true);
  tpSrc->Apply(0, CTexture::GetTexState(sTexState)); 
  tpSrcTemp->SetRT(0, true, FX_GetDepthSurface(nSizeX, nSizeY, false));

  CShader *pSH = CShaderMan::m_ShaderShadowBlur;
  HRESULT hr;
  Vec4 avSampleOffsets[16];
  Vec4 avSampleWeights[16];
  int TempX, TempY, TempWidth, TempHeight;
  GetViewport(&TempX, &TempY, &TempWidth, &TempHeight);
  RT_SetViewport(0, 0, nSizeX, nSizeY);

  //FIX: temp crash fix
  //texture should not be pushed out from pool because it was used on this frame
  //check pool behavior
  if (!tpSrc || !tpSrc->m_pTexture || tpSrc->m_eTT != eTT_2D)
    return;

  RECT rectSrc;
  GetTextureRect(tpSrc->m_pTexture, &rectSrc);
  InflateRect(&rectSrc, -1, -1);

  RECT rectDest;
  GetTextureRect(tpSrcTemp->m_pTexture, &rectDest);
  InflateRect(&rectDest, -1, -1);

  CoordRect coords;
  GetTextureCoords(tpSrc->m_pTexture, &rectSrc, tpSrcTemp->m_pTexture, &rectDest, &coords);

  uint32 nPasses;
  static CCryName TechName = "ShadowGaussBlur5x5";
  pSH->FXSetTechnique(TechName);
  pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

  pSH->FXBeginPass(0);

  float fBluriness = CLAMP(CV_r_VarianceShadowMapBlurAmount, 0.01f, 16.0f);
	
	// use only 9 samples
  hr = GetSampleOffsets_GaussBlur5x5((int)(nSizeX/fBluriness), (int)(nSizeY/fBluriness), avSampleOffsets, avSampleWeights, 1.f/0.728325f);
  static CCryName Param1Name = "SampleOffsets";
  static CCryName Param2Name = "SampleWeights";
  pSH->FXSetPSFloat(Param1Name, avSampleOffsets, 9);
  pSH->FXSetPSFloat(Param2Name, avSampleWeights, 9);

  Vec4 v;
  v[0] = 1.0f / (float)nSizeX;
  v[1] = 1.0f / (float)nSizeY;
  v[2] = 0;
  v[3] = 0;
  static CCryName Param3Name = "PixelOffset";
  pSH->FXSetVSFloat(Param3Name, &v, 1);

  EF_Scissor(true, rectDest.left, rectDest.top, rectDest.right-rectDest.left, rectDest.bottom-rectDest.top);

  // Draw a fullscreen quad to sample the RT
  ::DrawFullScreenQuad(coords, false);

  tpSrcTemp->RestoreRT(0, true);

  //copy from source texture
  tpSrcTemp->pLightOwner = tpSrc->pLightOwner;
  tpSrcTemp->nObjectsRenderedCount = tpSrc->nObjectsRenderedCount + 1;

  pFr->pDepthTex = tpSrcTemp;

  CTexture::s_ptexWhite->Apply(0);
  SAFE_DELETE(tpSrc);

  EF_Scissor(false, rectDest.left, rectDest.top, rectDest.right-rectDest.left, rectDest.bottom-rectDest.top);
  RT_SetViewport(TempX, TempY, TempWidth, TempHeight);

  pSH->FXEndPass();
  pSH->FXEnd();
}

void CD3D9Renderer::OnEntityDeleted(IRenderNode * pRenderNode)
{
  m_pRT->RC_EntityDelete(pRenderNode);
}

void _DrawText(ISystem * pSystem, int x, int y, const float fScale, const char * format, ...)
	PRINTF_PARAMS(5, 6);

void _DrawText(ISystem * pSystem, int x, int y, const float fScale, const char * format, ...)
{
  char buffer[512];
  va_list args;
  va_start(args, format);
  vsprintf_s(buffer, format, args);
  va_end(args);

	float color[4] = {0,1,0,1};
	gEnv->pRenderer->Draw2dLabel( (float)x,(float)y,fScale,color,false,buffer );
}

void CD3D9Renderer::DrawAllShadowsOnTheScreen()
{
	float width=800;
	float height=600;
	Set2DMode(true, (int)width, (int)height);
	EF_SelectTMU(0);
	EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);

  //overwrite 2D-mode viewport
  //int TempX, TempY, TempWidth, TempHeight;
  //GetViewport(&TempX, &TempY, &TempWidth, &TempHeight);

  //RT_SetViewport(0,0,width,height);


	int nMaxCount = 16;

	float fArrDim = max(4.f, sqrtf((float)nMaxCount));
	float fPicDimX = width/fArrDim;
	float fPicDimY = height/fArrDim;
	int nShadowId=0;
	SDynTexture_Shadow *pTX = SDynTexture_Shadow::s_RootShadow.m_NextShadow;
	for(float x=0; nShadowId<nMaxCount && x<width-10;  x+=fPicDimX)
	{
		for(float y=0; nShadowId<nMaxCount && y<height-10; y+=fPicDimY)
		{
			static ICVar *pVar = iConsole->GetCVar("e_ShadowsDebug");
			if(pVar && pVar->GetIVal()==1)
			{
				while(pTX->m_pTexture && (pTX->m_pTexture->m_nAccessFrameID+2) < GetFrameID(false) && pTX != &SDynTexture_Shadow::s_RootShadow)
					pTX = pTX->m_NextShadow;
			}

			if (pTX == &SDynTexture_Shadow::s_RootShadow)
				break;

			if(pTX->m_pTexture && pTX->pLightOwner)
			{

				CTexture *tp = pTX->m_pTexture;
				if (tp)
				{
					int nSavedAccessFrameID = pTX->m_pTexture->m_nAccessFrameID;



					SetState( /*GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | */GS_NODEPTHTEST );
          if (tp->GetTextureType() == eTT_2D)
          {
					  //Draw2dImage(x, y, fPicDimX-1, fPicDimY-1, tp->GetID(), 0,1,1,0,180);

            // set shader
            CShader *pSH( CShaderMan::m_ShaderShadowMaskGen );

            uint32 nPasses = 0;
            static CCryName TechName = "DebugShadowMap";
            pSH->FXSetTechnique(TechName);
            pSH->FXBegin( &nPasses, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES );
            pSH->FXBeginPass( 0 );

            m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Push();
            EF_PushMatrix();
            Matrix44A *m = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop();
            mathMatrixOrthoOffCenterLH(m, 0.0f, (float)width, (float)height, 0.0f, -1e30f, 1e30f);
            m = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->GetTop();
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->LoadIdentity();

            SetState( GS_NODEPTHTEST );
            STexState ts(FILTER_LINEAR, false);
            ts.m_nAnisotropy = 1;
            tp->Apply(0, CTexture::GetTexState(ts));
            D3DSetCull(eCULL_None);


            int nOffs;
            SVF_P3F_T3F *vQuad = (SVF_P3F_T3F *)GetVBPtr(4, nOffs, POOL_P3F_TEX3F);
            //SVF_P3F_C4B_T2F *vQuad = (SVF_P3F_C4B_T2F *)GetVBPtr(4, nOffs, POOL_P3F_COL4UB_TEX2F);
            if (!vQuad)
              return;
            vQuad[0].p = Vec3(x, y, 1);
            vQuad[0].st = Vec3(0, 1, 1);
            //vQuad[0].color.dcolor = (uint32)-1;
            vQuad[1].p = Vec3(x+fPicDimX-1, y, 1);
            vQuad[1].st = Vec3(1, 1, 1);
            //vQuad[1].color.dcolor = (uint32)-1;
            vQuad[3].p = Vec3(x+fPicDimX-1, y+fPicDimY-1, 1);
            vQuad[3].st = Vec3(1, 0, 1);
            //vQuad[3].color.dcolor = (uint32)-1;
            vQuad[2].p = Vec3(x, y+fPicDimY-1, 1);
            vQuad[2].st = Vec3(0, 0, 1);
            //vQuad[2].color.dcolor = (uint32)-1;

            // We are finished with accessing the vertex buffer
            UnlockVB(POOL_P3F_TEX3F);

            if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_T3F)))
            {
              // Bind our vertex as the first data stream of our device
              FX_SetVStream(0, m_pVB[POOL_P3F_TEX3F], 0, sizeof(SVF_P3F_T3F));
              FX_Commit();
              // Render the two triangles from the data stream
  #if defined (DIRECT3D9) || defined(OPENGL)
              HRESULT hReturn = m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, nOffs, 2);
  #elif defined (DIRECT3D10)
              SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
              m_pd3dDeviceContext->Draw(4, nOffs);
  #endif
            }

            pSH->FXEndPass();
            pSH->FXEnd();

            EF_PopMatrix();
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();

          }
          else
          {
            // set shader
            CShader *pSH( CShaderMan::m_ShaderShadowMaskGen );

            uint32 nPasses = 0;
            static CCryName TechName = "DebugCubeMap";
            pSH->FXSetTechnique(TechName);
            pSH->FXBegin( &nPasses, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES );
            pSH->FXBeginPass( 0 );

            float fSizeX = fPicDimX / 3;
            float fSizeY = fPicDimY / 2;
            float fx = ScaleCoordX(x); fSizeX = ScaleCoordX(fSizeX);
            float fy = ScaleCoordY(y); fSizeY = ScaleCoordY(fSizeY);
            float fOffsX[] = {fx, fx+fSizeX, fx+fSizeX*2, fx, fx+fSizeX, fx+fSizeX*2};
            float fOffsY[] = {fy, fy, fy, fy+fSizeY, fy+fSizeY, fy+fSizeY};
            Vec3 vTC0[] = {Vec3(1,1,1), Vec3(-1,1,-1), Vec3(-1,1,-1), Vec3(-1,-1,1), Vec3(-1,1,1), Vec3(1,1,-1)};
            Vec3 vTC1[] = {Vec3(1,1,-1), Vec3(-1,1,1), Vec3(1,1,-1), Vec3(1,-1,1), Vec3(1,1,1), Vec3(-1,1,-1)};
            Vec3 vTC2[] = {Vec3(1,-1,-1), Vec3(-1,-1,1), Vec3(1,1,1), Vec3(1,-1,-1), Vec3(1,-1,1), Vec3(-1,-1,-1)};
            Vec3 vTC3[] = {Vec3(1,-1,1), Vec3(-1,-1,-1), Vec3(-1,1,1), Vec3(-1,-1,-1), Vec3(-1,-1,1), Vec3(1,-1,-1)};

            m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Push();
            EF_PushMatrix();
            Matrix44A *m = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop();
            mathMatrixOrthoOffCenterLH(m, 0.0f, (float)m_width, (float)m_height, 0.0f, -1e30f, 1e30f);
            m = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->GetTop();
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->LoadIdentity();

            //EF_SetColorOp(eCO_REPLACE, eCO_REPLACE, DEF_TEXARG0, DEF_TEXARG0);
					  SetState( GS_NODEPTHTEST );
            STexState ts(FILTER_LINEAR, false);
            ts.m_nAnisotropy = 1;
            tp->Apply(0, CTexture::GetTexState(ts));

            D3DSetCull(eCULL_None);
            
            //overwrite 2D-mode viewport
            /*int TempX, TempY, TempWidth, TempHeight;
            GetViewport(&TempX, &TempY, &TempWidth, &TempHeight);

            RT_SetViewport(0,0,(int)width,(int)height);*/
 
            for (int i=0; i<6; i++)
            {
              int nOffs;
              SVF_P3F_T3F *vQuad = (SVF_P3F_T3F *)GetVBPtr(4, nOffs, POOL_P3F_TEX3F);
              //SVF_P3F_C4B_T2F *vQuad = (SVF_P3F_C4B_T2F *)GetVBPtr(4, nOffs, POOL_P3F_COL4UB_TEX2F);
              if (!vQuad)
                return;
              vQuad[0].p = Vec3(fOffsX[i], fOffsY[i], 1);
              vQuad[0].st = vTC0[i];
              //vQuad[0].color.dcolor = (uint32)-1;
              vQuad[1].p = Vec3(fOffsX[i]+fSizeX-1, fOffsY[i], 1);
              vQuad[1].st = vTC1[i];
              //vQuad[1].color.dcolor = (uint32)-1;
              vQuad[3].p = Vec3(fOffsX[i]+fSizeX-1, fOffsY[i]+fSizeY-1, 1);
              vQuad[3].st = vTC2[i];
              //vQuad[3].color.dcolor = (uint32)-1;
              vQuad[2].p = Vec3(fOffsX[i], fOffsY[i]+fSizeY-1, 1);
              vQuad[2].st = vTC3[i];
              //vQuad[2].color.dcolor = (uint32)-1;

              // We are finished with accessing the vertex buffer
              UnlockVB(POOL_P3F_TEX3F);

              //FX_SetFPMode();

              if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_T3F)))
              {
                // Bind our vertex as the first data stream of our device
                FX_SetVStream(0, m_pVB[POOL_P3F_TEX3F], 0, sizeof(SVF_P3F_T3F));

                FX_Commit();



                // Render the two triangles from the data stream
  #if defined (DIRECT3D9) || defined(OPENGL)
                HRESULT hReturn = m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, nOffs, 2);
  #elif defined (DIRECT3D10)
                SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                m_pd3dDeviceContext->Draw(4, nOffs);
  #endif
              }
            }

            //RT_SetViewport(TempX,TempY,TempWidth,TempHeight);

            pSH->FXEndPass();
            pSH->FXEnd();

            EF_PopMatrix();
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();
          }

	
					pTX->m_pTexture->m_nAccessFrameID = nSavedAccessFrameID;

          ILightSource *pLS = (ILightSource *)pTX->pLightOwner;

					_DrawText(iSystem, (int)(x/width*800.f), (int)(y/height*600.f), 1.0f, "%s \n %d %d-%d %d x%d", 
						pTX->m_pTexture->GetSourceName(),
            pTX->m_nUniqueID, 
						pTX->m_pTexture ? pTX->m_pTexture->m_nUpdateFrameID : 0, 
						pTX->m_pTexture ? pTX->m_pTexture->m_nAccessFrameID : 0, 
						pTX->nObjectsRenderedCount,
						pTX->m_nWidth);

					if(pLS->GetLightProperties().m_sName)
						_DrawText(iSystem, (int)(x/width*800.f), (int)(y/height*600.f)+32, 1.0f, "%s", pLS->GetLightProperties().m_sName);


				}
			}
			pTX = pTX->m_NextShadow;
		}
	}
  //RT_SetViewport(TempX,TempY,TempWidth,TempHeight);

	Set2DMode(false, m_width, m_height);
}

Vec3 UnProject(ShadowMapFrustum * pFr, Vec3 vPoint)
{
	const int shadowViewport[4] = {0,0,1,1};

	Vec3 vRes(0,0,0);
	gRenDev->UnProject(vPoint.x,vPoint.y,vPoint.z,
		&vRes.x,&vRes.y,&vRes.z,
		(float*)&pFr->mLightViewMatrix,
		(float*)&pFr->mLightProjMatrix,
		shadowViewport);

//	gRenDev->DrawLabel(vRes,1,"%d-%d-%d", (int)vPoint.x,(int)vPoint.y,(int)vPoint.z);

	return vRes;
}

//////////////////////////////////////////////////////////////////////////
#if defined (DIRECT3D10) && !defined(PS3)

void CD3D9Renderer::PrepareDepthMap(CDLight* pLightSource)
{
  // This function mist executed in main thread
  assert(!m_pRT->IsRenderThread());
  int nThreadList = m_RP.m_nProcessThreadID;
  ShadowMapFrustum** ppSMFrustumList = pLightSource->m_pShadowMapFrustums;

  assert(ppSMFrustumList);
  if(!ppSMFrustumList)
    return;

  //assign first frustum for test
  ShadowMapFrustum* lof= *ppSMFrustumList;
  //do all the father checks for first frustum only
  //////////////////////////////////////////////////////////////////////////

  assert(lof);
  if(!lof)
    return;

  assert(SRendItem::m_RecurseLevel[nThreadList] == 1);


  //TOFIX: temp check
  assert(lof->bOmniDirectionalShadow!=true);
  if(lof->bOmniDirectionalShadow)
    return;

  //if(lof->nAffectsReceiversFrameId != GetFrameID(false))  
  //	return;

  //check for this (should not be happened in the 3dengine)
  if (!(lof->pCastersList))
    return;

  if (lof->pCastersList->Count() <= 0)
    return;


  int nShadowGenGPU = 0;
  //lof->RequestUpdate();

  //reset device processing
  //FIX:should it be valid for d3d10 device ?
  if (lof->nResetID != m_nFrameReset)
  {
    lof->nResetID = m_nFrameReset;
    lof->RequestUpdate();
  }

  //check for update requested otherwise return
  //all these checks should be in separate function in ShadowMapFrustum class
  SDynTextureArray *pTX = NULL;

  if(lof->pDepthTexArray)
  {
    pTX = lof->pDepthTexArray;

    bool bNotNeedUpdate = false;
    if (lof->bOmniDirectionalShadow)
      bNotNeedUpdate = !(lof->nInvalidatedFrustMask[nShadowGenGPU] & lof->nOmniFrustumMask);
    else
      bNotNeedUpdate = !lof->isUpdateRequested(nShadowGenGPU);

    if ( pTX && pTX->m_pTexture && pTX==lof->pDepthTexArray && pTX->pLightOwner==lof->pLightOwner &&
      bNotNeedUpdate )
      return;
    if (!CV_r_ShadowGen)
      return;
  }
  else
  {
    int nnn = 0;
  }

  //////////////////////////////////////////////////////////////////////////
  //  update is requested - we should generate new shadow map
  //////////////////////////////////////////////////////////////////////////

  PROFILE_FRAME(Prep_PrepareDepthMap);

  SD3DSurface D3dDepthSurface;
  lof->bUseHWShadowMap = false;

  ETEX_Type eTT = lof->bOmniDirectionalShadow ? eTT_Cube : eTT_2D;

  lof->nTexSize = max(lof->nTexSize, 32);
  int nTextureSize = lof->nTexSize;

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Render objects into frame and Z buffers
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////

  EF_PushMatrix();
  m_RP.m_TI[nThreadList].m_matProj->Push();

  //fix problem with log file for scattering SM
  /*if (m_LogFile)
  {
  int nR = SRendItem::m_RecurseLevel;
  CDLight *pDL = m_RP.m_DLights[m_RP.m_nProcessThreadID][nR][lof->nDLightId];
  Logv(SRendItem::m_RecurseLevel, "   Really generating %dx%d shadow map for %s, [%s] \n", 
  nTextureSize, nTextureSize, lof->pLightOwner->GetName(), pDL->m_sName);
  }*/

  // setup matrices
  int vX, vY, vWidth, vHeight;
  GetViewport(&vX, &vY, &vWidth, &vHeight);

  Matrix44 camMatr = m_CameraMatrix;

  //////////////////////////////////////////////////////////////////////////
  //Select shadow buffers format
  //////////////////////////////////////////////////////////////////////////
  ETEX_Format eTF = eTF_D24S8;//eTF_D32F;


  if ( lof->bForSubSurfScattering || lof->bUseAdditiveBlending /*|| eTT != eTT_2D*/)
  {
    //Force eTF_G16R16 usage
    eTF = eTF_G16R16F;
    lof->bHWPCFCompare = false;
    lof->bUseHWShadowMap = false;
  }
  else 
  {
    eTF = eTF_D24S8;//eTF_D32F;
    lof->bUseHWShadowMap = true;

    if (lof->bOmniDirectionalShadow)
    {
      lof->bHWPCFCompare = false;
    }
    else
    {
      lof->bHWPCFCompare = true;
    }
  }

  //enable view space shadow maps
  static ICVar * p_e_gsm_view_dependent = iConsole->GetCVar("e_GsmViewSpace");
  bool bViewSpace = (p_e_gsm_view_dependent->GetIVal() > 0) && lof->bAllowViewDependency;


  Matrix44A* m = m_RP.m_TI[nThreadList].m_matProj->GetTop();
  m->SetIdentity();
  lof->mLightProjMatrix.SetIdentity();
  m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_FP_MATRIXDIRTY;

  m = m_RP.m_TI[nThreadList].m_matView->GetTop();
  m->SetIdentity();


  if(!pTX)
  {

    int nRTFlags = lof->bUseHWShadowMap?FT_USAGE_DEPTHSTENCIL:0;
    nRTFlags |= FT_STATE_CLAMP; 
    nRTFlags |= FT_USAGE_RENDERTARGET; 

    pTX = new SDynTextureArray(nTextureSize, nTextureSize, 4, eTF, /*eTT, */nRTFlags, "ShadowRT_TexArray");

    nTextureSize = pTX->m_nWidth;
    lof->nTexSize = nTextureSize;

  }

  lof->pDepthTexArray = pTX;
  pTX->pLightOwner = lof->pLightOwner;

  pTX->Update(nTextureSize, nTextureSize);

  //////////////////////////////////////////////////////////////////////////
  CTexture *pTexColorBuff = NULL;
  bool bDepthMapCreated = false;


  //dx10 RTs selection and check for device texture
  //FIX: is it correct?
  {
    pTexColorBuff = pTX->m_pTexture;

    if (CTexture::IsTextureExist(pTexColorBuff))
      bDepthMapCreated = true;
  }

  if (bDepthMapCreated)
  {
    //get DepthStencil surface
    SD3DSurface *pCurrDepthSurf;
    // Set render target

    //Dx9 DepthStencilSurf selection for all faces only once 
    if (!lof->bUseHWShadowMap)
    {
      //use depth texture from pool
      pCurrDepthSurf = FX_GetDepthSurface(nTextureSize, nTextureSize, false);
    }

    int nSides = 1;
    if (lof->bOmniDirectionalShadow)
      nSides = 6;

    int nS;
    int nOldScissor = CV_r_scissor;
    int old_CV_r_nodrawnear = CV_r_nodrawnear;
    int ClipPlanes = m_RP.m_ClipPlaneEnabled;
    SPlane clP = m_RP.m_CurClipPlane;
    int nPersFlags = m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags;
    int nPersFlags2 = m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2;
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_HDR;
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_SHADOWGEN;
    iSystem->GetI3DEngine()->SetRenderIntoShadowmap(true);

    //workaround remove texkill for eTF_DF24 and eTF_D24S8
    //and fix alphablend for dx10 depth stencil
    if (eTF == eTF_R32F || eTF == eTF_G16R16F || eTF == eTF_A16B16G16R16F || eTF == eTF_DF24 || eTF == eTF_DF16 || eTF == eTF_D16 || eTF == eTF_D24S8)
    {
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_NOALPHABLEND;
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_NOALPHATEST;
    }


    CCamera saveCam = GetCamera();

    //used for cubemap
    Vec3 vPos = lof->vLightSrcRelPos + lof->vProjTranslation;

    pTX->nObjectsRenderedCount = 0;

    for (nS=0; nS<nSides; nS++)
    {

      //update check for shadow frustums
      /*if (lof->bOmniDirectionalShadow)
      {
        if (!( (lof->nInvalidatedFrustMask & lof->nOmniFrustumMask) & (1 << nS) ))
        {
          continue;
        }
        else
        {
          lof->nInvalidatedFrustMask &= ~(1 << nS);
        }
      }
      else*/
      {
        lof->nInvalidatedFrustMask[nShadowGenGPU] = 0;
      }

      //Dx10 DSSurf selection for every CM face
      if (lof->bUseHWShadowMap)
      {
        if (lof->bOmniDirectionalShadow)
        {
          pTexColorBuff = NULL;
          //obtain ID3D11DepthStencilView
          //////////////////////////////////////////////////////////////////////////
          D3dDepthSurface.nWidth = nTextureSize;
          D3dDepthSurface.nHeight = nTextureSize;
          D3dDepthSurface.nFrameAccess = -1;
          D3dDepthSurface.bBusy = false;
          D3dDepthSurface.pSurf = pTX->m_pTexture->GetDeviceDepthStencilCMSurf(nS);
          D3dDepthSurface.pTex = pTX->m_pTexture;
          pCurrDepthSurf = &D3dDepthSurface;
          //////////////////////////////////////////////////////////////////////////
        }
        else
        {
          pTexColorBuff = NULL;
          //obtain ID3D11DepthStencilView
          //////////////////////////////////////////////////////////////////////////
          D3dDepthSurface.nWidth = nTextureSize;
          D3dDepthSurface.nHeight = nTextureSize;
          D3dDepthSurface.nFrameAccess = -1;
          D3dDepthSurface.bBusy = false;
          D3dDepthSurface.pSurf = pTX->m_pTexture->GetDeviceDepthStencilSurf();
          D3dDepthSurface.pTex = pTX->m_pTexture;
          pCurrDepthSurf = &D3dDepthSurface;
          //////////////////////////////////////////////////////////////////////////
        }
      }

      CCamera tmpCamera;
      if (eTT == eTT_2D)
      {
        //FIX: replace by pre-multiplied matrix
        //use different matrix in ShadowFrustum structure
        CShadowUtils::GetShadowMatrixOrtho( lof->mLightProjMatrix, lof->mLightViewMatrix, m_CameraMatrix, lof, bViewSpace);

        //Pre-multiply matrices
        lof->mLightViewMatrix = lof->mLightViewMatrix * lof->mLightProjMatrix;
        *m =  lof->mLightViewMatrix; //*(m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop())
      }
      else
      {
        float sCubeVector[6][7] = 
        {
          {1,0,0,  0,0,1, -90}, //posx
          {-1,0,0, 0,0,1,  90}, //negx
          {0,1,0,  0,0,-1, 0},  //posy
          {0,-1,0, 0,0,1,  0},  //negy
          {0,0,1,  0,1,0,  0},  //posz
          {0,0,-1, 0,1,0,  0},  //negz 
        };
        Vec3 vForward = Vec3(sCubeVector[nS][0], sCubeVector[nS][1], sCubeVector[nS][2]);
        Vec3 vUp      = Vec3(sCubeVector[nS][3], sCubeVector[nS][4], sCubeVector[nS][5]);
        Matrix33 matRot = Matrix33::CreateOrientation(vForward, vUp, DEG2RAD(sCubeVector[nS][6]));
        /*Vec3 Eye = vPos;
        Vec3 At = Eye + vForward;
        mathMatrixLookAt(m, Eye, At, vUp);*/
        float fMinDist = lof->fNearDist;
        float fMaxDist = lof->fFarDist;
        tmpCamera.SetMatrix(Matrix34(matRot, vPos));
        tmpCamera.SetFrustum(nTextureSize, nTextureSize, (float)DEG2RAD_R(90.0f), fMinDist, fMaxDist);
        m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_MIRRORCAMERA | RBPF_MIRRORCULL | RBPF_DRAWTOTEXTURE;
        m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_DRAWTOCUBE;
        SetCamera(tmpCamera);

        static ICVar *p_e_debug_draw = iConsole->GetCVar("e_DebugDraw");		assert(p_e_debug_draw);

        if(p_e_debug_draw->GetIVal()==8)
          gEnv->p3DEngine->DebugDraw_PushFrustrum("ShadowSide",tmpCamera,ColorB(0xff,0x1f,0x1f,0x0a));
      }
      m_RP.m_TI[nThreadList].m_vFrustumInfo.x = lof->fNearDist;
      m_RP.m_TI[nThreadList].m_vFrustumInfo.y = lof->fFarDist;

      //depth shift for precision increasing
      if ( (lof->bForSubSurfScattering || lof->bUseAdditiveBlending) && eTF == eTF_G16R16 ) //special case
      {
        m_RP.m_TI[nThreadList].m_vFrustumInfo.z = 0.0f;
        lof->bNormalizedDepth = true;
      } 
      else
      if ( !lof->bUseHWShadowMap && lof->fNearDist > 1000.f ) //FIX: strange check for sun
      {
        m_RP.m_TI[nThreadList].m_vFrustumInfo.z = 0.5f;
        lof->bNormalizedDepth = false;

      }
      else 
      {
        m_RP.m_TI[nThreadList].m_vFrustumInfo.z = 0.0f;
        lof->bNormalizedDepth = true;
      }

      //shadowgen bias
      m_RP.m_TI[nThreadList].m_vFrustumInfo.w = lof->fDepthTestBias;  


#if defined (DIRECT3D10) && !defined(PS3)
      //set all valid shadow frustums for GS
      for(int nCaster = 0; *ppSMFrustumList && nCaster!=4; ++ppSMFrustumList, ++nCaster )
      {
        SetupShadowGenPass(nCaster, *ppSMFrustumList);
      }
#else
      EF_SetCameraInfo();
#endif
      //EF_SetCameraInfo();


      FX_PushRenderTarget(0, pTexColorBuff, pCurrDepthSurf, false, eTT==eTT_Cube ? nS : -1);		// calls RT_SetViewport() implicitly
      if (lof->bUseHWShadowMap)
      {
        pTX->m_pTexture->m_nUpdateFrameID = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID;
        //FIX: check if hw-pcf texture has m_nAccessFrameID updated properly
        //pTX->m_pTexture->m_nAccessFrameID = pCurrDepthSurf->nFrameAccess;
      }

      //FIX:: is in necessary here because FX_PushRenderTarget already called it
      RT_SetViewport(0, 0, nTextureSize, nTextureSize);

      //////////////////////////////////////////////////////////////////////////
      // clear frame buffer after RT push
      //////////////////////////////////////////////////////////////////////////
      ColorF col(0,0,0);
      if (!lof->bUseHWShadowMap)
      { 
        if (pTexColorBuff->GetDstFormat() == eTF_A8R8G8B8 && !lof->bReflectiveShadowMap)
          col = ColorF(1,1,1,0);
				else if(lof->bReflectiveShadowMap)
					col = ColorF(0,0,0,0);
				else
          col = ColorF(1,0,0,0);

        EF_ClearBuffers(FRT_CLEAR, &col);

      }
      else
      {
        EF_ClearBuffers(/*FRT_CLEAR_DEPTH|FRT_CLEAR_STENCIL*/FRT_CLEAR, &col, 1.0f);
      }
      //////////////////////////////////////////////////////////////////////////

      //disable color writes
      if(lof->bUseHWShadowMap && !lof->bReflectiveShadowMap)
      {
        EF_SetState(GS_COLMASK_NONE);
        m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |=	RBPF2_DISABLECOLORWRITES;
      }
      //////////////////////////////////////////////////////////////////////////

      /*Vec4 vZRange; 
      vZRange.x = -lof->fNearDist / (lof->fFarDist - lof->fNearDist);
      vZRange.y = 1.0f / (lof->fFarDist - lof->fNearDist);
      vZRange.z = 1;
      vZRange.w = 0;*/
      FX_Commit();

      int nBorder = max(4,nTextureSize/64);
      //if (eTT == eTT_2D)
      //  EF_Scissor(true,nBorder,nBorder,nTextureSize-nBorder*2,nTextureSize-nBorder*2);
      //FIX !!! why CULL_NONE is setting here
      D3DSetCull(eCULL_None);
      // disable scissor modifications during objects rendering
      CV_r_scissor = 0;

      Vec3 vCamPos = iSystem->GetViewCamera().GetPosition();

      // workaround: avoid casing shadow from 1ps weapon
      CV_r_nodrawnear = true;
      if (lof->pCastersList)
      {
        // draw entities
        EF_StartEf();

        //fast check: is there any receiver on that side of the cubemap ?
        //			float fMaxDist = lof->fMaxDistanceOfTheRecivers[ nS ];
        //				if( fMaxDist > 0 )
        for(int m = 0; m<lof->pCastersList->Count(); m++)
        { 
          IShadowCaster * pEnt  = (*lof->pCastersList)[m];

          AABB aabb = pEnt->GetBBoxVirtual();

          //farer than the last receiver.. not needed to include
          //Vec3 vCenter( (aabb.min+aabb.max)*0.5 );
          //							float fDistance = (vPos - vCenter).GetLength() - (aabb.max-vCenter).GetLength();
          //							if( fDistance > fMaxDist )
          //							continue;

          if (eTT == eTT_Cube)
          {
            bool bVis = tmpCamera.IsAABBVisible_F( aabb );
            if (!bVis)
              continue;
          }

          if( (lof->m_Flags & DLF_DIFFUSEOCCLUSION) && pEnt->HasOcclusionmap( 0, lof->pLightOwner) )
            continue;
          //          const char *name = pEnt->GetName();

          //SRenderObjectModifier rParms;
          //rParms.nTechniqueID = TTYPE_SHADOWGEN; 
          //rParms.fDistance = sqrtf(Distance::Point_AABBSq(vCamPos,aabb));              
          //rParms.nStatesInUse = true;
          gEnv->p3DEngine->RenderRenderNode(pEnt, NULL);

          pTX->nObjectsRenderedCount ++;
        }

        //bind current shadow frustum to the pipeline
        m_RP.m_pCurShadowFrustum = lof;
        EF_EndEf3D(0, -1);
      }

      FX_PopRenderTarget(0);

      //enable color writes
      if(lof->bUseHWShadowMap)
      {
        m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &=	~RBPF2_DISABLECOLORWRITES;
      }
    }

    SetCamera(saveCam);
    if (eTT == eTT_Cube)
    {
      //FIX: replace this by light space matrix computation and move out here
      lof->mLightViewMatrix.SetIdentity();
      lof->mLightViewMatrix.SetTranslation(vPos);
      lof->mLightViewMatrix.Transpose();
    }

    CV_r_nodrawnear = old_CV_r_nodrawnear;

    // restore scissor
    CV_r_scissor = nOldScissor;

    if (ClipPlanes)
      FX_SetClipPlane(true, &clP.m_Normal.x, false);

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags = nPersFlags;
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 = nPersFlags2;

    //Copy shadow pass params to other frustums
    //for dx10 tex arrays
    ppSMFrustumList = pLightSource->m_pShadowMapFrustums;
    for(int nCaster = 0; *ppSMFrustumList && nCaster!=4; ++ppSMFrustumList, ++nCaster )
    {
      (*ppSMFrustumList)->nTexSize = lof->nTexSize;
      (*ppSMFrustumList)->pDepthTexArray = lof->pDepthTexArray;
      (*ppSMFrustumList)->pDepthTex = lof->pDepthTex;
      (*ppSMFrustumList)->bHWPCFCompare = lof->bHWPCFCompare;
      (*ppSMFrustumList)->bNormalizedDepth = lof->bNormalizedDepth;
    }

    //FIX Scessor is not used
    //EF_Scissor(false,0,0,0,0);
  }
  else
  {
    iLog->Log("Error: cannot create depth texture '%s' (%s) (skipping)", pTexColorBuff->GetName(), pTexColorBuff->GetTextureType() == eTT_2D ? "Texture" : "Cubemap");
  }

  EF_PopMatrix();
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();
  m_CameraMatrix = camMatr;
  EF_SetCameraInfo();
  iSystem->GetI3DEngine()->SetRenderIntoShadowmap(false);

  if(CV_r_VarianceShadowMapBlurAmount>0 && lof->bUseVarianceSM && lof->pCastersList->Count() > 0)
  {
    BlurShadow(lof);
    //lof->bHWPCFCompare = true;
  }

  if(lof->bForSubSurfScattering && lof->pCastersList->Count() > 0)
  {
    BlurShadow(lof);
  }

  RT_SetViewport(vX, vY, vWidth, vHeight);

  CHWShader_D3D::mfSetGlobalParams();
}

#endif

//////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::PrepareDepthMap(ShadowMapFrustum* lof, CDLight* pLight, int nLightFrustumID)
{

  int nThreadList = m_RP.m_nProcessThreadID;
  assert(lof);
  if(!lof)
    return;

  //select shadowgen gpu
  int nShadowGenGPU = 0;
  if(IsMultiGPUModeActive() && CV_r_ShadowGenMode==1)
  {
    //TOFIx: make m_nFrameSwapID - double buffered
    nShadowGenGPU = gRenDev->m_nFrameSwapID % gRenDev->m_nGPUs;
  }

  //////////////////////////////////////////////////////////////////////////
  // Set matrices
  //////////////////////////////////////////////////////////////////////////

	//enable view space shadow maps
	static ICVar * p_e_gsm_view_dependent = iConsole->GetCVar("e_GsmViewSpace");
	bool bViewSpace = (p_e_gsm_view_dependent->GetIVal() > 0) && lof->bAllowViewDependency;

    // Save previous camera
  int vX, vY, vWidth, vHeight;
  GetViewport(&vX, &vY, &vWidth, &vHeight);
  Matrix44 camMatr = m_CameraMatrix;

  // Setup matrices
  EF_PushMatrix(); //push view matrix
  m_RP.m_TI[nThreadList].m_matProj->Push();

  Matrix44A* m = m_RP.m_TI[nThreadList].m_matProj->GetTop();
	m->SetIdentity();
  m = m_RP.m_TI[nThreadList].m_matView->GetTop();
	m->SetIdentity();

	m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_FP_MATRIXDIRTY;

  lof->mLightProjMatrix.SetIdentity();

  //////////////////////////////////////////////////////////////////////////
  //  Assign RTs
  //////////////////////////////////////////////////////////////////////////
  SD3DSurface D3dDepthSurface;
  CTexture *pTexColorBuff = NULL;
  CTexture *pTexDepthBuff = NULL;
  bool bDepthMapCreated = false;

  //before was bound to the Frustum
  //SDynTexture_Shadow *pTX = lof->pDepthTex;
  //search for Frustum owner
  SDynTexture_Shadow *pTX, *pNext;
  lof->pDepthTex = NULL;
  bool bFoundMapping = false;
  for (pTX=SDynTexture_Shadow::s_RootShadow.m_NextShadow; pTX != &SDynTexture_Shadow::s_RootShadow; pTX=pNext)
  {
    if(pTX->m_pFrustumOwner == lof->pFrustumOwner)
    {
      bFoundMapping = true;
      lof->pDepthTex = pTX;
      break;
    }
    pNext = pTX->m_NextShadow;
  }
  pTX = lof->pDepthTex;

  //assert(bFoundMapping == true);
  //if (!bFoundMapping)
  //{
  //  //lof->pDepthTex = NULL;
  //  return;
  //}


  if (CV_r_UseShadowsPool && lof->bUseHWShadowMap)
  {
    //FIX: optimize static texture creation with invalidation
    // create 1x1 texture for everything
    CTexture::s_ptexRT_NULL->Invalidate( lof->nTextureWidth, lof->nTextureHeight, m_FormatNULL.IsValid()?eTF_NULL:eTF_A8R8G8B8 );
    //current eTF should be stored in the shadow frustum
    ETEX_Format ePoolTF = eTF_D24S8;
    CTexture::s_ptexRT_ShadowPool->Invalidate( /*1024.0f, 1024.0f*/lof->nTextureWidth, lof->nTextureHeight, ePoolTF );

    if (!CTexture::IsTextureExist(CTexture::s_ptexRT_ShadowPool))
      CTexture::s_ptexRT_ShadowPool->CreateRenderTarget(eTF_Unknown);

    pTexColorBuff = CTexture::s_ptexRT_NULL;
    pTexDepthBuff = CTexture::s_ptexRT_ShadowPool;
    bDepthMapCreated = true;
  }
  else
  {
    //////////////////////////////////////////////////////////////////////////
    //  assign texture from shadow pool
    //////////////////////////////////////////////////////////////////////////
    //AUTO_LOCK(g_cDynTexLock); 

    if(pTX)
    {
      if( pTX->m_eTF!=lof->m_eReqTF || pTX->m_eTT!=lof->m_eReqTT || pTX->pLightOwner!=lof->pLightOwner ||
        pTX->m_nReqWidth!=lof->nTextureWidth ||  pTX->m_nReqHeight!=lof->nTextureHeight)
      {
        SAFE_DELETE(pTX);

        //force all cubemap faces update
        lof->RequestUpdate();
      }
    }

    //check after freeing texture
    if(!pTX)
    {
      int nRTFlags = lof->bUseHWShadowMap?FT_USAGE_DEPTHSTENCIL:0;
      nRTFlags |= FT_STATE_CLAMP; 

      pTX = new SDynTexture_Shadow(lof->nTextureWidth, lof->nTextureHeight, lof->m_eReqTF, lof->m_eReqTT, nRTFlags, "ShadowRT");
      assert(lof->nTextureWidth == pTX->m_nWidth && lof->nTextureHeight == pTX->m_nHeight);
    }

    lof->pDepthTex = pTX;

    //assign owners
    pTX->pLightOwner = lof->pLightOwner;
    pTX->m_pFrustumOwner = lof->pFrustumOwner;

    //propagate texture for MT sync
    //assert(lof->ppDepthTex != NULL);
    //*(lof->ppDepthTex) = pTX;
    //assert(pTX!=NULL);

    //create device texture
    pTX->RT_Update(lof->nTextureWidth, lof->nTextureHeight);


    //////////////////////////////////////////////////////////////////////////
    #if defined (DIRECT3D10)
      {
        //TOFIX: should not be used as native HW shadow maps are used all the time
        pTexColorBuff = pTX->m_pTexture;
        if (CTexture::IsTextureExist(pTexColorBuff))
          bDepthMapCreated = true;
      }
    #else
      //dx9 RTs selection
      if (!lof->bUseHWShadowMap)
      {
        pTexColorBuff = pTX->m_pTexture;
        if (CTexture::IsTextureExist(pTexColorBuff))
          bDepthMapCreated = true;
      }
      else
      {
        //FIX: optimize static texture creation with invalidation







        if (!CV_d3d9_debugruntime)
        {
          if (m_FormatNULL.IsValid())
          {
            // create 1x1 texture for everything
            CTexture::s_ptexRT_NULL->Invalidate( 1, 1, eTF_NULL);
          }
          else
          {
            CTexture::s_ptexRT_NULL->Invalidate( 1, 1, eTF_A8R8G8B8);
          }
        }
        else 
        {
          //fall back for debug runtime validation
          CTexture::s_ptexRT_NULL->Invalidate( lof->nTextureWidth, lof->nTextureHeight, eTF_A8R8G8B8 );
        }



        pTexColorBuff = CTexture::s_ptexRT_NULL;
        pTexDepthBuff = pTX->m_pTexture;


        //should be created later
        bDepthMapCreated = true;
      }
        //////////////////////////////////////////////////////////////////////////
    #endif

  }

  if (bDepthMapCreated)
  {
		//get DepthStencil surface
		SD3DSurface *pCurrDepthSurf = NULL;
    // Set render target

    //Dx9 DSSurf selection for all faces only once 
    if (!lof->bUseHWShadowMap)
    {
      //TOFIX: disable this call for DX11

      //use depth texture from pool
      pCurrDepthSurf = FX_GetDepthSurface(lof->nTextureWidth, lof->nTextureHeight, false);
    }
    else 
    if (pTexDepthBuff)
    {


    #if defined (DIRECT3D9)
      //////////////////////////////////////////////////////////////////////////
      D3dDepthSurface.nWidth = lof->nTextureWidth;
      D3dDepthSurface.nHeight = lof->nTextureHeight;
      D3dDepthSurface.nFrameAccess = -1;
      D3dDepthSurface.bBusy = false;
      D3dDepthSurface.pSurf = pTexDepthBuff->GetSurface(0, 0);
      pCurrDepthSurf = &D3dDepthSurface;
      //////////////////////////////////////////////////////////////////////////
    #endif
    }
    else
    {
      #if defined (DIRECT3D9)
        assert(0);
      #endif
    }

    int nSides = 1;
    if (lof->bOmniDirectionalShadow)
      nSides = 6;

    int nS;
    int nFirstShadowGenRI, nLastShadowGenRI;

    int nOldScissor = CV_r_scissor;
    int old_CV_r_nodrawnear = CV_r_nodrawnear;
    int ClipPlanes = m_RP.m_ClipPlaneEnabled;
    SPlane clP = m_RP.m_CurClipPlane;
    int nPersFlags = m_RP.m_TI[nThreadList].m_PersFlags;
    int nPersFlags2 = m_RP.m_TI[nThreadList].m_PersFlags2;
    m_RP.m_TI[nThreadList].m_PersFlags &= ~RBPF_HDR;
    m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_SHADOWGEN;

    if (lof->bUseAdditiveBlending)
    {
      m_RP.m_TI[nThreadList].m_PersFlags2 |= RBPF2_VSM;
    }

    if (!(lof->m_Flags & DLF_DIRECTIONAL))
    {
      m_RP.m_TI[nThreadList].m_PersFlags2 |= RBPF2_DRAWTOCUBE;
    }

    //workaround remove texkill for eTF_DF24 and eTF_D24S8
    if (pTX->m_eTF == eTF_R32F || pTX->m_eTF == eTF_G16R16F || pTX->m_eTF == eTF_R16F || 
        pTX->m_eTF == eTF_A16B16G16R16F || pTX->m_eTF == eTF_DF24 || pTX->m_eTF == eTF_DF16 || 
        pTX->m_eTF == eTF_D16 || pTX->m_eTF == eTF_D24S8 || pTX->m_eTF == eTF_D32F || pTX->m_eTF == eTF_DEPTH24



				)
      m_RP.m_TI[nThreadList].m_PersFlags2 |= RBPF2_NOALPHABLEND;

#ifndef PS3
		// enable alpha blending for RSMs
		if(lof->bReflectiveShadowMap)
			m_RP.m_TI[nThreadList].m_PersFlags2 &= ~RBPF2_NOALPHABLEND;
#endif

    if (pTX->m_eTF == eTF_R32F || pTX->m_eTF == eTF_G16R16F || pTX->m_eTF == eTF_R16F || pTX->m_eTF == eTF_A16B16G16R16F



			)
      m_RP.m_TI[nThreadList].m_PersFlags2 |= RBPF2_NOALPHATEST;

    CCamera saveCam = GetCamera();
    Vec3 vPos = lof->vLightSrcRelPos + lof->vProjTranslation;

    if (pTX)
		  pTX->nObjectsRenderedCount = 0;

    for (nS=0; nS<nSides; nS++)
    {

      //////////////////////////////////////////////////////////////////////////
      //compute shadow recursive level
      //////////////////////////////////////////////////////////////////////////
      int nThreadID  = m_RP.m_nProcessThreadID;
      int nShadowRecur = nLightFrustumID;
      if (lof->bOmniDirectionalShadow)
      {
        nShadowRecur += nS;
      }
      nFirstShadowGenRI = SRendItem::m_ShadowsStartRI[nThreadID][nShadowRecur];
      nLastShadowGenRI = SRendItem::m_ShadowsEndRI[nThreadID][nShadowRecur];
      if (nLastShadowGenRI-nFirstShadowGenRI<1)
        continue;

      //sort
      SRendItem::mfSortByLight(&SRendItem::RendItems(nThreadID,SG_SORT_GROUP,EFSLIST_SHADOW_GEN)[nFirstShadowGenRI], 
                               nLastShadowGenRI-nFirstShadowGenRI, true, false, lof->bReflectiveShadowMap);
         
      //Dx10 DSSurf selection for every CM face
#if defined (DIRECT3D10)
      if (lof->bUseHWShadowMap)
      {
        if (lof->bOmniDirectionalShadow && !(lof->bUnwrapedOmniDirectional))
        {
          pTexColorBuff = NULL;
          //obtain ID3D11DepthStencilView
          //////////////////////////////////////////////////////////////////////////
          D3dDepthSurface.nWidth = lof->nTextureWidth;
          D3dDepthSurface.nHeight = lof->nTextureHeight;
          D3dDepthSurface.nFrameAccess = -1;
          D3dDepthSurface.bBusy = false;
          D3dDepthSurface.pSurf = pTX->m_pTexture->GetDeviceDepthStencilCMSurf(nS);
          D3dDepthSurface.pTex = pTX->m_pTexture;
          pCurrDepthSurf = &D3dDepthSurface;
          //////////////////////////////////////////////////////////////////////////
        }
        else
        {
          pTexColorBuff = NULL;
          //obtain ID3D11DepthStencilView
          //////////////////////////////////////////////////////////////////////////
          D3dDepthSurface.nWidth = lof->nTextureWidth;
          D3dDepthSurface.nHeight = lof->nTextureHeight;
          D3dDepthSurface.nFrameAccess = -1;
          D3dDepthSurface.bBusy = false;
          D3dDepthSurface.pSurf = pTX->m_pTexture->GetDeviceDepthStencilSurf();
          D3dDepthSurface.pTex = pTX->m_pTexture;
          pCurrDepthSurf = &D3dDepthSurface;
          //////////////////////////////////////////////////////////////////////////
        }
      }
#if defined(CRY_DXPS_RASTERTHREAD)
			gcpRendD3D->m_pd3dDevice->SoftRast().DepthBuffer(reinterpret_cast<CCryDXPSDepthStencilView*>(D3dDepthSurface.pSurf));
#endif
#endif

      CCamera tmpCamera;
      if (!lof->bOmniDirectionalShadow)
      {
          *m =  lof->mLightViewMatrix;
      }
      else
			{
				 float sCubeVector[6][7] = 
					{
						{1,0,0,  0,0,1, -90}, //posx
						{-1,0,0, 0,0,1,  90}, //negx
						{0,1,0,  0,0,-1, 0},  //posy
						{0,-1,0, 0,0,1,  0},  //negy
						{0,0,1,  0,1,0,  0},  //posz
						{0,0,-1, 0,1,0,  0},  //negz 
					};
          Vec3 vForward = Vec3(sCubeVector[nS][0], sCubeVector[nS][1], sCubeVector[nS][2]);
          Vec3 vUp      = Vec3(sCubeVector[nS][3], sCubeVector[nS][4], sCubeVector[nS][5]);
          Matrix33 matRot = Matrix33::CreateOrientation(vForward, vUp, DEG2RAD(sCubeVector[nS][6]));
          /*Vec3 Eye = vPos;
			    Vec3 At = Eye + vForward;
					mathMatrixLookAt(m, Eye, At, vUp);*/
					float fMinDist = lof->fNearDist;
          float fMaxDist = lof->fFarDist;
          tmpCamera.SetMatrix(Matrix34(matRot, vPos));
          if (!(lof->bUnwrapedOmniDirectional))
          {
            tmpCamera.SetFrustum(lof->nShadowMapSize, lof->nShadowMapSize, (float)DEG2RAD_R(90.0f), fMinDist, fMaxDist);
            //m_RP.m_TI.m_PersFlags2 |= RBPF2_DRAWTOCUBE;
          }
          else 
          {
            tmpCamera.SetFrustum(lof->nShadowMapSize, lof->nShadowMapSize, (float)DEG2RAD_R(CShadowUtils::g_fOmniShadowFov), fMinDist, fMaxDist);
          }
					//m_RP.m_TI.m_PersFlags |= RBPF_MIRRORCAMERA | RBPF_MIRRORCULL | RBPF_DRAWTOTEXTURE;
          //SetCamera(tmpCamera);

//////////////////////////////////////////////////////////////////////////
          const Matrix34& m34 = tmpCamera.GetMatrix();
          CRenderCamera c;
          c.Perspective(tmpCamera.GetFov(), tmpCamera.GetProjRatio(), tmpCamera.GetNearPlane(), tmpCamera.GetFarPlane());
          Vec3 vEyeC = tmpCamera.GetPosition();
          Vec3 vAtC = vEyeC + Vec3(m34(0,1), m34(1,1), m34(2,1));
          Vec3 vUpC = Vec3(m34(0,2), m34(1,2), m34(2,2));
          c.LookAt(vEyeC, vAtC, vUpC);
          SetRCamera(c);

          /*float fFrustFov;
          if (!(lof->bUnwrapedOmniDirectional))
          {
            fFrustFov = 90.0;
            //m_RP.m_TI.m_PersFlags2 |= RBPF2_DRAWTOCUBE;
						m_RP.m_TI.m_PersFlags |= RBPF_DRAWTOTEXTURE;
						//m_RP.m_TI.m_PersFlags |= RBPF_MIRRORCULL;
          }
          else 
          {
            fFrustFov = CShadowUtils::g_fOmniShadowFov;
            //m_RP.m_TI.m_PersFlags2 |= RBPF2_DRAWTOCUBE;
					  m_RP.m_TI.m_PersFlags |= RBPF_DRAWTOTEXTURE;
						//m_RP.m_TI.m_PersFlags |= RBPF_MIRRORCULL;
          }*/

          //get matrices
          /*if (pLight)
          {
            Matrix33 mRot = ( Matrix33(pLight->m_ObjMatrix) );//.GetTransposed();
            lof->GetCubemapFrustum(nS, fFrustFov, m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop(), m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->GetTop(), &mRot);
          }
          else*/
          {
            CShadowUtils::GetCubemapFrustum(FTYP_SHADOWOMNIPROJECTION, lof, nS, m_RP.m_TI[nThreadList].m_matProj->GetTop(), m_RP.m_TI[nThreadList].m_matView->GetTop(), false);
          }

          //enable back facing for omni lights for now
          m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_MIRRORCULL;

          //EF_DirtyMatrix();
          //EF_SetCameraInfo();
//////////////////////////////////////////////////////////////////////////



					static ICVar *p_e_debug_draw = iConsole->GetCVar("e_DebugDraw");		assert(p_e_debug_draw);

					if(p_e_debug_draw->GetIVal()==8)
          {
						gEnv->p3DEngine->DebugDraw_PushFrustrum("ShadowSide",tmpCamera,ColorB(0xff,0x1f,0x1f,0x0a));
          }
      }
      m_RP.m_TI[nThreadList].m_vFrustumInfo.x = lof->fNearDist;
      m_RP.m_TI[nThreadList].m_vFrustumInfo.y = lof->fFarDist;

      //depth shift for precision increasing
      if ( (lof->bForSubSurfScattering || lof->bUseAdditiveBlending) && pTX->m_eTF == eTF_G16R16 ) //special case
      {
        m_RP.m_TI[nThreadList].m_vFrustumInfo.z = 0.0f;
        lof->bNormalizedDepth = true;
      } 
      else
      if ( !lof->bUseHWShadowMap && lof->fNearDist > 1000.f ) //FIX: strange check for sun
      {
			  m_RP.m_TI[nThreadList].m_vFrustumInfo.z = 0.5f;
        lof->bNormalizedDepth = false;
       
      }
      else 
      {
        m_RP.m_TI[nThreadList].m_vFrustumInfo.z = 0.0f;
        lof->bNormalizedDepth = true;
      }
      
      //shadowgen bias
      m_RP.m_TI[nThreadList].m_vFrustumInfo.w = lof->fDepthTestBias;
 
#if defined (DIRECT3D10)
      //set all valid shadow frustums for GS
      /*for(int nCaster = 0; *ppSMFrustumList && nCaster!=4; ++ppSMFrustumList, ++nCaster )
      {
        SetupShadowGenPass(nCaster, *ppSMFrustumList);
      }*/
#else
      //EF_SetCameraInfo();
#endif
      EF_SetCameraInfo();

      int arrViewport[4];

			if(!lof->bReflectiveShadowMap)
				FX_PushRenderTarget(0, pTexColorBuff, pCurrDepthSurf, false, pTX->m_eTT==eTT_Cube ? nS : -1);		// calls RT_SetViewport() implicitly
			else
			{
				if(!IrrVolumes.IsEnabled())
					continue;
				PROFILE_LABEL_PUSH( "REFLECTIVE_SHADOWMAP" );
				IrrVolumes.UpdateReflectiveShadowmapSize(IrrVolumes.m_RSM, lof->nTextureWidth, lof->nTextureHeight);
				FX_PushRenderTarget(0, (CTexture*)IrrVolumes.m_RSM.pColorRT, pCurrDepthSurf, false, -1);
				FX_PushRenderTarget(1, (CTexture*)IrrVolumes.m_RSM.pDepthRT, NULL);
				FX_PushRenderTarget(2, (CTexture*)IrrVolumes.m_RSM.pNormalRT, NULL);
			}

      if (lof->bUseHWShadowMap)
      {
       if (pTX && pTX->m_pTexture)
         pTX->m_pTexture->m_nUpdateFrameID = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID;

       //FIX: check if hw-pcf texture has m_nAccessFrameID updated properly
       //pTX->m_pTexture->m_nAccessFrameID = pCurrDepthSurf->nFrameAccess;
      }

      //SDW-GEN_REND_PATH
      //////////////////////////////////////////////////////////////////////////
      // clear frame buffer after RT push
      //////////////////////////////////////////////////////////////////////////
      ColorF col(0,0,0);
      float fClearDepth = 1.0f;
      if (!lof->bUseHWShadowMap)
      { 
        if (pTexColorBuff->GetDstFormat() == eTF_A8R8G8B8 && !lof->bReflectiveShadowMap)
          col = ColorF(1,1,1,0);
				else if(lof->bReflectiveShadowMap)
					col = ColorF(0,0,0,0);
        else
          col = ColorF(1,0,0,0);
      }


#if defined(CRY_DXPS_RASTERTHREAD)
			if(UsePS3SoftwareRastizer())
			{
				gcpRendD3D->m_pd3dDevice->SoftRast().ViewPort(0,0,lof->nTextureWidth,lof->nTextureWidth);
				gcpRendD3D->m_pd3dDevice->SoftRast().Clear(DXPSRDEPTH_MAX);
			}
			else
#endif
      if (lof->bUnwrapedOmniDirectional && lof->nOmniFrustumMask == 0x3F && CV_r_ShadowGenMode==1)
      {
        //force entire clear
        if (nS == 0)
        {
			    RT_SetViewport(0, 0, lof->nTextureWidth , lof->nTextureHeight);
          EF_ClearBuffers(FRT_CLEAR|FRT_CLEAR_IMMEDIATE, &col, fClearDepth);
        }
      }
      else
      {
        //clear will happen after actual RT_SetViewport call
        EF_ClearBuffers(FRT_CLEAR/*FRT_CLEAR_DEPTH|FRT_CLEAR_STENCIL*/, &col, fClearDepth);
      }
      //////////////////////////////////////////////////////////////////////////

			//set proper side-viewport
      if (lof->bUnwrapedOmniDirectional)
      {
        lof->GetSideViewport(nS, arrViewport);
        RT_SetViewport(arrViewport[0], arrViewport[1], arrViewport[2], arrViewport[3]);
      }
			else
				RT_SetViewport(0, 0, lof->nShadowMapSize, lof->nShadowMapSize);


			//////////////////////////////////////////////////////////////////////////

      /*Vec4 vZRange;
      vZRange.x = -lof->fNearDist / (lof->fFarDist - lof->fNearDist);
      vZRange.y = 1.0f / (lof->fFarDist - lof->fNearDist);
      vZRange.z = 1;
      vZRange.w = 0;*/

      FX_Commit(true);

			//disable color writes
			if(lof->bUseHWShadowMap)
			{
				if(!lof->bReflectiveShadowMap)
				{
					EF_SetState(GS_COLMASK_NONE, -1);
					m_RP.m_TI[nThreadList].m_PersFlags2 |=	RBPF2_DISABLECOLORWRITES;
				}
        float fShadowsBias = CV_r_ShadowsBias;
        float fShadowsSlopeScaleBias = lof->fDepthSlopeBias;
#if defined (DIRECT3D9)
        //m_pd3dDevice->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&fShadowsBias);
        if (fShadowsSlopeScaleBias > 0.0f && (lof->m_Flags & DLF_DIRECTIONAL))
        {
          m_pd3dDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&fShadowsSlopeScaleBias);
        }
#elif defined (DIRECT3D10)
        if (lof->fDepthSlopeBias > 0.0f && (lof->m_Flags & DLF_DIRECTIONAL))
        {
          SStateRaster CurRS = m_StatesRS[m_nCurStateRS];

          CurRS.Desc.DepthBias = 0;//1;
          CurRS.Desc.DepthBiasClamp = fShadowsBias * 20;
          CurRS.Desc.SlopeScaledDepthBias = lof->fDepthSlopeBias;
          //TD: force disabling multisampling for shadowgen
          //CurRS.Desc.MultisampleEnable = false;
          //CurRS.Desc.AntialiasedLineEnable = false;

          SetRasterState(&CurRS);
        }
#endif
			}

	    int nBorder = max(4,lof->nShadowMapSize/64);
      //if (eTT == eTT_2D)
	    //  EF_Scissor(true,nBorder,nBorder,nTextureSize-nBorder*2,nTextureSize-nBorder*2);
      //FIX !!! why CULL_NONE is setting here
      D3DSetCull(eCULL_None);

			Vec3 vCamOrigin = iSystem->GetViewCamera().GetPosition();
	    // workaround: avoid casing shadow from 1ps weapon
      int nRendFlags = SHDF_NO_SCISSOR | SHDF_NO_DRAWNEAR;
			if (lof->pCastersList)
			{
        m_RP.m_pCurShadowFrustum = lof;

        FX_ProcessRenderList(nFirstShadowGenRI,
                             nLastShadowGenRI, 
                             EFSLIST_SHADOW_GEN, 0, FX_FlushShader_ShadowGen, false);
      

        //TOFIX proper update
        //if (pTX)
        //  pTX->nObjectsRenderedCount ++;

				// draw entities
				/*EF_StartEf();

						for(int m = 0; m<lof->pCastersList->Count(); m++)
						{ 
							IShadowCaster* pEnt  = (*lof->pCastersList)[m];

							AABB aabb = pEnt->GetBBoxVirtual();

							if (lof->bOmniDirectionalShadow)
							{
								bool bVis = tmpCamera.IsAABBVisible_F( aabb );
								if (!bVis)
									continue;
							}

							if( (lof->m_Flags & DLF_DIFFUSEOCCLUSION) && pEnt->HasOcclusionmap( 0, lof->pLightOwner) )
								continue;
		//          const char *name = pEnt->GetName();

							SRenderObjectModifier rParms;
							rParms.nTechniqueID = TTYPE_SHADOWGEN; 
							rParms.fDistance = sqrt(Distance::Point_AABBSq(vCamOrigin,aabb));              
              rParms.nStatesInUse = true;
              gEnv->p3DEngine->RenderRenderNode(pEnt, &rParms);

              if (pTX)
							  pTX->nObjectsRenderedCount ++;
						}
      
        //bind current shadow frustum to the pipeline
        EF_EndEf3D(nRendFlags);*/
      }











      if(lof->bReflectiveShadowMap)
      {
        PROFILE_LABEL_POP( "REFLECTIVE_SHADOWMAP" );
        FX_PopRenderTarget(2);
        FX_PopRenderTarget(1);
      }
      FX_PopRenderTarget(0);
#if defined(CRY_DXPS_RASTERTHREAD)
			if(UsePS3SoftwareRastizer())
				gcpRendD3D->m_pd3dDevice->SoftRast().CopyBuffer();
#endif




      //enable color writes
      if(lof->bUseHWShadowMap)
      {
        m_RP.m_TI[nThreadList].m_PersFlags2 &=	~RBPF2_DISABLECOLORWRITES;
#if defined (DIRECT3D9) || defined(OPENGL)
        //m_pd3dDevice->SetRenderState(D3DRS_DEPTHBIAS, 0);
        m_pd3dDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0);
#elif defined (DIRECT3D10)
        if (CV_r_ShadowsBias>0.0f)
        {
          //D3D11_RASTERIZER_DESC RSShadowGen;

          SStateRaster CurRS = m_StatesRS[m_nCurStateRS];
          CurRS.Desc.DepthBias = 0;
          CurRS.Desc.DepthBiasClamp = 0;
          CurRS.Desc.SlopeScaledDepthBias = 0;

          //CurRS.Desc.MultisampleEnable = false;
          //CurRS.Desc.AntialiasedLineEnable = false;

          SetRasterState(&CurRS);
        }
#endif
			}

    }


    SetCamera(saveCam);
    if (pTX->m_eTT == eTT_Cube)
    {
			//FIX: replace this by light space matrix computation and move out here
			lof->mLightViewMatrix.SetIdentity();
			lof->mLightViewMatrix.SetTranslation(vPos);
			lof->mLightViewMatrix.Transpose();
    }

    if (lof->bUseAdditiveBlending)
    {
      m_RP.m_TI[nThreadList].m_PersFlags2 &= ~RBPF2_VSM;
    }

    if (!(lof->m_Flags & DLF_DIRECTIONAL))
    {
      m_RP.m_TI[nThreadList].m_PersFlags2 &= ~RBPF2_DRAWTOCUBE;
    }

    m_RP.m_TI[nThreadList].m_PersFlags &= ~RBPF_MIRRORCULL;

	  CV_r_nodrawnear = old_CV_r_nodrawnear;

    // restore scissor
    CV_r_scissor = nOldScissor;

    if (ClipPlanes)
      FX_SetClipPlane(true, &clP.m_Normal.x, false);

    m_RP.m_TI[nThreadList].m_PersFlags = nPersFlags;
    m_RP.m_TI[nThreadList].m_PersFlags2 = nPersFlags2;

    EF_Scissor(false,0,0,0,0);

    if(CV_r_multithreaded == 0 && CV_r_VarianceShadowMapBlurAmount>0 && lof->bUseVarianceSM)
    {
      //BlurShadow(lof);
      //lof->bHWPCFCompare = true;
    }

    if(CV_r_multithreaded == 0 && lof->bForSubSurfScattering)
    {
      //BlurShadow(lof);
    }

  }
  else if (pTexColorBuff)
  {
    iLog->Log("Error: cannot create depth texture '%s' (%s) (skipping)", pTexColorBuff->GetName(), pTexColorBuff->GetTextureType() == eTT_2D ? "Texture" : "Cubemap");
  }

  EF_PopMatrix();
  m_RP.m_TI[nThreadList].m_matProj->Pop();
  m_CameraMatrix = camMatr;
  EF_SetCameraInfo();

#if !defined(XENON)
  //don't need to restore viewport (restored in FX_RestoreRenderTarget)
  RT_SetViewport(vX, vY, vWidth, vHeight);
#endif

}


void CD3D9Renderer::SetupShadowGenPass(int Num, ShadowMapFrustum * pFr)
{
  Matrix44 shadowMat, mLightView, mLightProj, mLightViewProj;

  //check for successful PrepareDepthMap
  /*if ( !pFr->pDepthTex ) 
  {
    return;
  }
  else 
  {
    if (!pFr->pDepthTex->m_pTexture)
      return;
  }*/

  //FIX: temporal computation for matrix here
  CShadowUtils::GetShadowMatrixOrtho( pFr->mLightProjMatrix,
                            pFr->mLightViewMatrix,
                            m_CameraMatrix, pFr, false);
  //Pre-multiply matrices
  pFr->mLightViewMatrix = pFr->mLightViewMatrix * pFr->mLightProjMatrix;

 
  //FIX:remove temporarly matrices
  mLightView = pFr->mLightViewMatrix;
  mLightProj = pFr->mLightProjMatrix;

  //currently mLightProj has already pre-multiplied LightProj matrix
  mLightViewProj = mLightView ; //* mLightProj

  shadowMat = mLightViewProj; //*mInvertedView * 

  //set shadow matrix
  gRenDev->m_TempMatrices[Num][0].Transpose(shadowMat);

  //set light space transformation
  m_cEF.m_TempVecs[5] = Vec4(mLightViewProj.m30, mLightViewProj.m31, mLightViewProj.m32, 1);
  
  if (Num >= 0)
  {
      /*SDynTexture_Shadow *pTX = pFr->pDepthTex;
      int nID = pTX->m_pTexture ? pTX->m_pTexture->GetID() : 0;
      int nIDBlured = 0;
      m_RP.m_ShadowCustomTexBind[Num*2+0] = nID;
      m_RP.m_ShadowCustomTexBind[Num*2+1] = nIDBlured;*/

      assert(Num<4);
      //per frustum parameters
      m_cEF.m_TempVecs[1][Num] = pFr->fDepthTestBias;
      m_cEF.m_TempVecs[2][Num] = 1.f / (pFr->fFarDist);

      //TOFIX:replace by mask based function to set these parameters
      //depth shift to extend range precision
      if ( (pFr->bForSubSurfScattering || pFr->bUseAdditiveBlending) && pFr->pDepthTex->m_eTF == eTF_G16R16  )
      {
        m_cEF.m_TempVecs[3][Num] = 0.0f;
      } 
       else
        if ( pFr->fNearDist > 1000.f)
        {
          m_cEF.m_TempVecs[3][Num] = 0.5f;
        }
        else 
        {
          m_cEF.m_TempVecs[3][Num] = 0.0f;
        }
  }
}
 


void CD3D9Renderer::SetupShadowOnlyPass(int Num, ShadowMapFrustum * pFr, Matrix34A * pObjMat)
{
	assert(pFr);

  ConfigShadowTexgen(Num, pFr);
}

//TODO:make two independent functions for dx10 and dx9 without processing texture array and resource views
// setup projection texgen
void CD3D9Renderer::ConfigShadowTexgen(int Num, ShadowMapFrustum * pFr, int nResView, int nFrustNum, const CDLight* pLight)
{
	Matrix44A shadowMat, mTexScaleBiasMat, mLightView, mLightProj, mLightViewProj;


	//check for successful PrepareDepthMap
	if ( !pFr->pDepthTex || !pFr->pDepthTex->m_pTexture ) 
	{
    if (!pFr->pDepthTexArray || !pFr->pDepthTexArray->m_pTexture)
    {
      if (!CV_r_UseShadowsPool)
        return;
    }
	}

	//calc view inversion matrix
	//Matrix44 *pCurrViewMatrix = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->GetTop();
	//Matrix44 mInvertedView = pCurrViewMatrix->GetInverted();

	//shadow frustum adjustment matrix
  //half texel offset
  //TOFIX: should take in account real shadow atlas resolution
	float fOffsetX = 0.5f;
	float fOffsetY = 0.5f;

#if !defined (DIRECT3D10)
  fOffsetX += (0.5f / pFr->nTexSize);
  fOffsetY += (0.5f / pFr->nTexSize);
#endif


  	mTexScaleBiasMat = Matrix44( 0.5f,     0.0f,     0.0f,    0.0f,
															0.0f,    -0.5f,     0.0f,    0.0f,  
															0.0f,     0.0f,     1.0f,    0.0f,
															fOffsetX, fOffsetY, 0.0f,    1.0f );


	//FIX:remove temporarly matrices
  if (pFr->bOmniDirectionalShadow && nFrustNum>-1)
  {

    //rotate frustums if light is specified
    /*if (pLight)
    {
      Matrix33 mRot = ( Matrix33(pLight->m_ObjMatrix) );//.GetTransposed();
      pFr->GetCubemapFrustum(nFrustNum,g_fOmniShadowFovf,&mLightProj,&mLightView, &mRot);
    }
    else*/
    {
     CShadowUtils::GetCubemapFrustum(FTYP_SHADOWOMNIPROJECTION, pFr, nFrustNum,  &mLightProj, &mLightView);
    }

    pFr->GetTexOffset(nFrustNum, &m_cEF.m_TempVecs[6][0]);

    //calculate crop matrix for  frustum
    //TD: investigate proper half-texel offset with mCropView
    float fScaleX=1.0f/3.0f;
    float fScaleY=1.0f/2.0f;

    float fOffsX = m_cEF.m_TempVecs[6].x;
    float fOffsY = m_cEF.m_TempVecs[6].y;

    Matrix44 mCropView(  fScaleX,     0.0f,  0.0f,   0.0f,
                         0.0f,  fScaleY,  0.0f,   0.0f,
                         0.0f,     0.0f,  1.0f,   0.0f,
                         fOffsX, fOffsY,  0.0f,   1.0f );

    // multiply the projection matrix with it
    mTexScaleBiasMat = mTexScaleBiasMat * mCropView;

  }
  else
  {
	  mLightView = pFr->mLightViewMatrix;
    //temporarily disabled since mLightProjMatrix contains pre-multiplied matrix already
	  mLightProj.SetIdentity(); //pFr->mLightProjMatrix;
  }

	//currently mLightProj has already pre-multiplied LightProj matrix
 	mLightViewProj = mLightView * mLightProj; 

	shadowMat = /*mInvertedView * */mLightViewProj * mTexScaleBiasMat;

	//set shadow matrix
	gRenDev->m_TempMatrices[Num][0] = GetTransposed44(shadowMat); //TOFIX: construct projection and view matrices with Column Convention  
	//set light space transformation
	m_cEF.m_TempVecs[5] = Vec4(mLightViewProj.m30, mLightViewProj.m31, mLightViewProj.m32, 1);

	//////////////////////////////////////////////////////////////////////////
	// Deferred shadow pass setup
	//////////////////////////////////////////////////////////////////////////
	int vpX, vpY, vpWidth, vpHeight;
	GetViewport( &vpX, &vpY, &vpWidth, &vpHeight );
	FX_DeferredShadowPassSetup(/*shadowMat*/gRenDev->m_TempMatrices[Num][0], (float)vpWidth, (float)vpHeight);
	//////////////////////////////////////////////////////////////////////////

	//rotation matrix for far terrain projection
	//Fix: Generate separate matrix for frustum
	Matrix33 mRotMatrix(mLightView);
	mRotMatrix.OrthonormalizeFast();
	gRenDev->m_TempMatrices[0][1] =  GetTransposed44(Matrix44(mRotMatrix));
  
	// enable combined shadow maps
	static ICVar * p_e_gsm_combined = 0;//iConsole->GetCVar("e_GsmCombined");		assert(p_e_gsm_combined);

	// move and scale projection matrix to access the subareas in the combined texture
	if(p_e_gsm_combined && p_e_gsm_combined->GetIVal())
	{
		float fOffsX=0, fOffsY=0;

		switch(pFr->nShadowMapLod)
		{
			case 0:	fOffsX=0.0f;fOffsY=0.0f;break;
			case 1: fOffsX=0.5f;fOffsY=0.0f;break;
			case 2: fOffsX=0.0f;fOffsY=0.5f;break;
			case 3: fOffsX=0.5f;fOffsY=0.5f;break;
			default: assert(0);
		}

		switch(Num)
		{
			case 0: m_cEF.m_TempVecs[6][0] = fOffsX;m_cEF.m_TempVecs[6][1] = fOffsY; break;
			case 1: m_cEF.m_TempVecs[6][2] = fOffsX;m_cEF.m_TempVecs[6][3] = fOffsY; break;
			case 2: m_cEF.m_TempVecs[7][0] = fOffsX;m_cEF.m_TempVecs[7][1] = fOffsY; break;
			case 3: m_cEF.m_TempVecs[7][2] = fOffsX;m_cEF.m_TempVecs[7][3] = fOffsY; break;
			default: assert(0);
		}
	}


  if (Num >= 0)
  {
    if(!pFr->pDepthTex && !pFr->pDepthTexArray && !CV_r_UseShadowsPool)
      Warning("Warning: CD3D9Renderer::ConfigShadowTexgen: pFr->depth_tex_id not set");
    else
    {
      int nID = 0;
      int nIDBlured = 0;
      if (CV_r_UseShadowsPool)
      {
        nID = CTexture::s_ptexRT_ShadowPool->GetID();
      }
      else
      if(pFr->pDepthTex!=NULL)
      {
        SDynTexture_Shadow *pTX = pFr->pDepthTex;
        nID = pTX->m_pTexture ? pTX->m_pTexture->GetID() : 0;
      }
      else 
        if (pFr->pDepthTexArray!=NULL)
      {
        SDynTextureArray *pTX = pFr->pDepthTexArray;
        nID = pTX->m_pTexture ? pTX->m_pTexture->GetID() : 0;
      }

      m_RP.m_ShadowCustomTexBind[Num*2+0] = nID;
      m_RP.m_ShadowCustomResViewID[Num*2+0] = nResView;

      //to fix: what was that ?
      m_RP.m_ShadowCustomTexBind[Num*2+1] = nIDBlured;
//      m_cEF.m_TempVecs[0][Num] = pFr->fAlpha;

      //SDW-CFG_PRM
      //GSM shadow fading param
      m_cEF.m_TempVecs[8][0] = pFr->fShadowFadingDist;

      assert(Num<4);
      //per frustum parameters
      //fDepthTestBias param
      if (pFr->bHWPCFCompare)
      {
        if (pFr->m_Flags & DLF_DIRECTIONAL)
        {
          //linear case + constant offset
          m_cEF.m_TempVecs[1][Num] = pFr->fDepthConstBias; 
        }
        else
        if (pFr->m_Flags & DLF_PROJECT)
        {
          //non-linear case
          m_cEF.m_TempVecs[1][Num] = pFr->fDepthConstBias /** pFr->fFarDist*/; 
        }
        else
        {

          m_cEF.m_TempVecs[1][Num] = pFr->fDepthConstBias /** pFr->fFarDist*/; 
        }

      }
      else
      {
        //linear case
        m_cEF.m_TempVecs[1][Num] = pFr->fDepthTestBias; 
      }

      //fOneDivFarDist param
      if ((pFr->m_Flags & DLF_DIRECTIONAL) || (!(pFr->bUseHWShadowMap) && !(pFr->bHWPCFCompare)))
      {
        //linearized shadows are used for any kind of directional lights and for non-hw point lights
        m_cEF.m_TempVecs[2][Num] = 1.f / (pFr->fFarDist);
      }
      else
      {
        //hw point lights sources have non-linear depth for now
        m_cEF.m_TempVecs[2][Num] = 1.f/ (pFr->fFarDist);
      }


      //vInvShadowMapWH param
      m_cEF.m_TempVecs[9][0] = 1.f / pFr->nTexSize;
      m_cEF.m_TempVecs[9][1] = 1.f / pFr->nTexSize;

      //TOFIX:replace by mask based function to set these parameters
      //depth shift to extend range precision
      if (pFr->bNormalizedDepth)
      {
        m_cEF.m_TempVecs[3][Num] = 0.0f;
      }
      else
      {
        m_cEF.m_TempVecs[3][Num] = 0.5f;
      }


			float fShadowJitter = CV_r_shadow_jittering;

      //TOFIX: currently force console variable using
      //should not set it like this but 
      //all the time retation samples should be recompiled to reduce pixel shader
      if (pFr->m_Flags & DLF_DIRECTIONAL)
      {
        //FIME: insert calculation for Filtered Area
        float fFilteredArea = fShadowJitter * (pFr->fWidthS + pFr->fBlurS);
        m_cEF.m_TempVecs[4].x = fFilteredArea;
        m_cEF.m_TempVecs[4].y = fFilteredArea;
      }
      else
      {
        fShadowJitter = 2.0; //constant penumbra for now
        //USE frustums in sGetIrregKernel() directly but not through the temp vector
        m_cEF.m_TempVecs[4].x = fShadowJitter; /** pFr->fFrustrumSize*/;
        m_cEF.m_TempVecs[4].y = fShadowJitter;

        if (pFr->bOmniDirectionalShadow)
        {
          m_cEF.m_TempVecs[4].x *= 1.0f/3.0f;
          m_cEF.m_TempVecs[4].y *= 1.0f/2.0f;
        }
      }
      

      //calc clouds shadow texgen
      //todo move to the 3dengine
      if (pFr->bUseAdditiveBlending)
      {
        I3DEngine* p3DEngine( gEnv->p3DEngine );

        // terrain size
        int heightMapSize( p3DEngine->GetTerrainSize() );
        Vec3 vCloudShadowOffset = m_vCloudShadowSpeed*gEnv->pTimer->GetCurrTime();
        vCloudShadowOffset.x = vCloudShadowOffset.x - int(vCloudShadowOffset.x);
        vCloudShadowOffset.y = vCloudShadowOffset.y - int(vCloudShadowOffset.y);
        Vec4 invTerrainSize( 1.0f / heightMapSize, 1.0f / heightMapSize, vCloudShadowOffset.x, vCloudShadowOffset.y );

        m_cEF.m_TempVecs[9] = invTerrainSize;
      }
    }
  }
}

//=============================================================================================================

//set light source based shadows
void CD3D9Renderer::FX_SetDeferredShadows()
{
  CRenderObject *pObj = m_RP.m_pCurObject;

  PodArray<ShadowMapFrustum*> * pShadowMapFrustums = pObj->m_pShadowCasters;

  if (!pShadowMapFrustums)
    return;

  for (int i=0; i<pShadowMapFrustums->Count(); i++)
  {
    if (i == 4)
      break;
    ShadowMapFrustum* pShadowFrustum = pShadowMapFrustums->GetAt(i);
    SetupShadowOnlyPass(i, pShadowFrustum, (m_RP.m_ObjFlags & FOB_TRANS_MASK) ? &m_RP.m_pCurObject->m_II.m_Matrix : NULL);
    if (pShadowFrustum->bOmniDirectionalShadow)
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0+i];
  }

  int32 nCasters = pShadowMapFrustums->Count();

  m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SHADOW_MIXED_MAP_G16R16];

  m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3]);

  if (nCasters >= 1)
  {
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    if (nCasters >= 2)
    {
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
      if (nCasters >= 3)
      {
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
        if (nCasters >= 4)
          m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];
      }
    }
  }

}

void CD3D9Renderer::FX_DrawShadowPasses(CShader *ef, SShaderTechnique *pTech, int nChanNum )
{
  SShaderPass *slw;
  uint32 i, j;

  PodArray<ShadowMapFrustum*> * lsources = m_RP.m_pCurObject->m_pShadowCasters;
  bool bHasShadow = (lsources && lsources->Count());

  m_RP.m_FlagsPerFlush |= RBSI_SHADOWPASS;
  m_RP.m_nNumRendPasses = 0;

  PROFILE_FRAME(DrawShader_MultiShadow_Passes);

  uint32 nCaster = 0;
  uint32 nCasters;
  if( bHasShadow ) 
    nCasters = lsources->Count();
  else
    nCasters = 0;

  uint32 nDeltaCasters = 1;

  static PodArray<ShadowMapFrustum*> SmLI;
  SmLI.Clear();
  int nFirstLightID = m_RP.m_nCurLightGroup*4;
  int nLightID = nFirstLightID+nChanNum;
  BYTE nShadowCasterMask = 0;

  if( bHasShadow )
  {
#if !defined (DIRECT3D10)
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SHADOW_MIXED_MAP_G16R16];
#else
    m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SHADOW_MIXED_MAP_G16R16];
#endif

    bHasShadow = false;
    for (; nCaster<nCasters; nCaster++)
    {
      ShadowMapFrustum* pFr = lsources->GetAt(nCaster);
      //TOFIX: skip frustum for subsurf scattering on early stage
      if (pFr->nDLightId == nLightID && !(pFr->bUseAdditiveBlending) && !(pFr->bForSubSurfScattering)) // TOFIX: transparent objects do not receive shadows currently
      {
        SmLI.Add(pFr);
        bHasShadow = true;
      }

      //generate shadowmask for this group
      int nLocalLightID = pFr->nDLightId - nFirstLightID;
      if( nLocalLightID >= 0 && nLocalLightID < 4 )
        nShadowCasterMask |= 1 << nLocalLightID;
    }

//Diesel cut
    //if(CV_r_shadow_jittering>0.0f)
    //  m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SHADOW_JITTERING];

    // set global shader RT flags

    // enable combined shadow maps
    {
      static ICVar * p_e_gsm_combined = 0;//iConsole->GetCVar("e_GsmCombined");		assert(p_e_gsm_combined);

      if(p_e_gsm_combined && p_e_gsm_combined->GetIVal())
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GSM_COMBINED];
      else 
        assert((m_RP.m_FlagsShader_RT&g_HWSR_MaskBit[HWSR_GSM_COMBINED])==0);
    }
  }
  int nPass = 0;
  if(bHasShadow)
    nCasters = SmLI.Count();
  else
    nCasters = 0;

  for (nCaster=0; nCaster<nCasters; nCaster+=nDeltaCasters)
  {
    slw = &pTech->m_Passes[0];
    for (i=0; i<pTech->m_Passes.Num(); i++, slw++)
    {
      bool bUseLight = false;
      nDeltaCasters = min(nCasters-nCaster, 4U);
      m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_CUBEMAP0] |
                                 g_HWSR_MaskBit[HWSR_POINT_LIGHT] | g_HWSR_MaskBit[HWSR_HW_PCF_COMPARE]);
      if( bHasShadow )
      {
        if (nDeltaCasters >= 1 )
        {
          m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
          if (nDeltaCasters >= 2)
          {
            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
            if (nDeltaCasters >= 3)
            {
              m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
              if (nDeltaCasters >= 4)
                m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];
            }
          }
        }
        for (j=0; j<nDeltaCasters; j++)
        {
          // setup projection
          SetupShadowOnlyPass(j, SmLI[j + nCaster], (m_RP.m_ObjFlags & FOB_TRANS_MASK) ? &m_RP.m_pCurObject->m_II.m_Matrix : NULL);
          /*if (SmLI[j+nCaster]->bOmniDirectionalShadow)
          {
            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0+j];
          }*/

          if (!(SmLI[j+nCaster]->m_Flags & DLF_DIRECTIONAL))
          {
            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_POINT_LIGHT ];
          }

          //TODO:implement per shadow frustum HW_PCF_COMPARE to combine HW_PCF with non-HW_PCF shadow maps per pass
          if (SmLI[j+nCaster]->bHWPCFCompare)
          {
            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];
          }

        }
      }
      m_RP.m_pCurPass = slw;

      // Set all textures and HW TexGen modes for the current pass (ShadeLayer)
      assert (slw->m_VShader && slw->m_PShader);
      if (!slw->m_VShader || !slw->m_PShader)
        continue;
      CHWShader_D3D *curVS = (CHWShader_D3D *)slw->m_VShader;
      CHWShader_D3D *curPS = (CHWShader_D3D *)slw->m_PShader;

      uint64 nPrevFlagsShaderRT = m_RP.m_FlagsShader_RT;
      int nPrevMaterialState = m_RP.m_MaterialState;
      int State = slw->m_RenderState;
      int AlphaRef = slw->m_AlphaRef;
      if (m_RP.m_MaterialState != 0 || m_RP.m_pCurObject->m_fAlpha != 1.0f)
      {
        if (m_RP.m_pCurObject->m_fAlpha != 1.0f)
        {
          m_RP.m_MaterialState &= ~GS_BLEND_MASK;
          m_RP.m_MaterialState |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
        }
        if (!m_RP.m_nNumRendPasses && m_RP.m_nCurLightGroup <= 0) 
        {
          State &= ~GS_BLEND_MASK;
          State |= (m_RP.m_MaterialState & GS_BLEND_MASK);
          if (!(State & GS_ALPHATEST_MASK))
            State &= ~GS_DEPTHWRITE;
        }
        else
          if (int nBlend=(m_RP.m_CurState & GS_BLEND_MASK))
          {
            if (nBlend == (GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA))
              State = (State & ~GS_BLEND_MASK) | (GS_BLSRC_SRCALPHA | GS_BLDST_ONE);
          }
      }

      //masking the different shadow-map - occlusion map cases
      int nColorMask = bHasShadow ? ((~(1<<nChanNum))<<GS_COLMASK_SHIFT) & GS_COLMASK_MASK : GS_COLMASK_NONE;

      State |= nColorMask;

      if (m_RP.m_MaterialState & GS_BLEND_MASK)
        State &= ~GS_DEPTHWRITE;
      FX_ZState(State);
      EF_SetState(State);

      int nBlend;
      if (nBlend=(m_RP.m_CurState & GS_BLEND_MASK))
      {
        if (nBlend == (GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA))
          m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ALPHABLEND];
      }

#if !defined(XENON) && !defined(PS3)
      if (m_RP.m_FlagsPerFlush & RBSI_INSTANCED)
      {
        // Using geometry instancing approach
        FX_DrawShader_InstancedHW(ef, slw);
      }
      else
#endif
      {
        // Set Pixel shader and all associated textures
        bool bRes = curPS->mfSet(HWSF_SETTEXTURES);
        if (bRes)
          FX_DrawBatches(ef, slw, curVS, curPS);
      }

      // Make sure to restore flags/states after changing them explicitly
      m_RP.m_MaterialState = nPrevMaterialState;
      m_RP.m_FlagsShader_RT = nPrevFlagsShaderRT;

      SRendItem::m_ShadowsValidMask[SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][m_RP.m_nCurLightGroup] |= (1 << m_RP.m_nCurLightChan);
    }
  }
}


void CD3D9Renderer::FX_PrepareDepthMapsForLight(CDLight* pLight, int nLightID)
{
  int nThreadID = m_RP.m_nProcessThreadID;
  int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID]-1;

  /*ShadowMapFrustum** ppSMFrustumList = pLight->m_pShadowMapFrustums;
  assert( ppSMFrustumList != 0);
  if (!ppSMFrustumList)
  continue;
  */

  //render all valid shadow frustums
  /*for(int nCaster = 0; *ppSMFrustumList && nCaster!=MAX_GSM_LODS_NUM; ++ppSMFrustumList, ++nCaster )
  {       
  //TODO: check does light cast shadow here or in PrepareDepthMap
  assert( (*ppSMFrustumList)->nDLightId == nLightID );

  PrepareDepthMap( *ppSMFrustumList, pLight, nCaster);
  }*/


  //render all valid shadow frustums
  int nStartIdx = SRendItem::m_StartFrust[nThreadID][nLightID];
  int nEndIdx = SRendItem::m_EndFrust[nThreadID][nLightID];
  assert((nEndIdx-nStartIdx)<=MAX_GSM_LODS_NUM);
  for(int nFrustIdx=nStartIdx; nFrustIdx < nEndIdx; ++nFrustIdx )
  {       
    ShadowMapFrustum* pCurFrustum = &m_RP.m_SMFrustums[nThreadID][nCurRecLevel][nFrustIdx];
    //temp disable for deferred lights
    //assert( pCurFrustum->nDLightId == nLightID );

    int nLightFrustumID = nLightID * MAX_SHADOWMAP_LOD + (nFrustIdx-nStartIdx);
    PrepareDepthMap( pCurFrustum, pLight, nLightFrustumID);
  }
}

void CD3D9Renderer::EF_PrepareAllDepthMaps()
{
  if (CV_r_UseShadowsPool)
    return;

  // get list of shadow casters for nLightID;
  //int nLightID = 0 + 0 * 4;
  int nThreadID = m_RP.m_nProcessThreadID;
  int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID]-1;

  int NumDynLights = m_RP.m_DLights[nThreadID][nCurRecLevel].Num();
  //assert(m_RP.m_DLights[ SRendItem::m_RecurseLevel ].Num()>0);
  TArray<CDLight>&  arrDeferLights = CDeferredShading::Instance().GetLights( nThreadID, nCurRecLevel);

  if (NumDynLights<=0 && arrDeferLights.Num()<=0)
    return;

  for (int nLightID=0; nLightID < NumDynLights; nLightID++)
  {
    CDLight* pLight =  &m_RP.m_DLights[nThreadID][SRendItem::m_RecurseLevel[nThreadID]-1][nLightID];

    if (!(pLight->m_Flags & DLF_CASTSHADOW_MAPS))
      continue;
    //if (!(pLight->m_Flags & DLF_DIRECTIONAL))
    //return;
#if defined (DIRECT3D10) && !defined(PS3)
    if (CV_r_ShadowGenGS!=0 && pLight->m_Flags & DLF_DIRECTIONAL)
    {
      PrepareDepthMap( pLight );
      continue;
    }
#endif
    FX_PrepareDepthMapsForLight(pLight, nLightID);

		// reflective shadowmaps injection
		if(pLight->m_Flags & DLF_REFLECTIVE_SHADOWMAP)
		{
			int nStartIdx = SRendItem::m_StartFrust[nThreadID][nLightID];
			int nEndIdx = SRendItem::m_EndFrust[nThreadID][nLightID];
			if(nStartIdx < nEndIdx)
			{
				ShadowMapFrustum* pCurFrustum = &m_RP.m_SMFrustums[nThreadID][nCurRecLevel][nStartIdx];
				CREIrradianceVolume* pVol = IrrVolumes.GetGIVolumeByLight(pLight->m_pOwner);
				if(pCurFrustum && pVol)
				{
					IrrVolumes.m_RSM.mxLightViewProj = pCurFrustum->mLightViewMatrix;
					pVol->RenderReflectiveShadowMap(IrrVolumes.m_RSM);
				}
			}
		}
  }

  //////////////////////////////////////////////////////////////////////////
  for(uint32 nDeferLightID=0; nDeferLightID < arrDeferLights.Num(); nDeferLightID++)
  {
    CDLight* pLight =  &arrDeferLights[nDeferLightID];
    if (!(pLight->m_Flags & DLF_CASTSHADOW_MAPS))
      continue;

    int nDeferLightIdx = nDeferLightID + NumDynLights;
    FX_PrepareDepthMapsForLight(pLight, nDeferLightIdx);
  }
}
