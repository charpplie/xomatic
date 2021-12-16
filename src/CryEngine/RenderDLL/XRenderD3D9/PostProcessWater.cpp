/*=============================================================================
D3DPostProcess : Direct3D specific post processing special effects
Copyright (c) 2001 Crytek Studios. All Rights Reserved.

Revision history:
* 23/02/2005: Re-factored/Converted to CryEngine 2.0 by Tiago Sousa
* Created by Tiago Sousa

=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "D3DPostProcess.h"

#pragma warning(disable: 4244)

using namespace NPostEffects;







////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CUnderwaterGodRays::Render()
{
  PROFILE_LABEL_PUSH( "GODRAYS" );

  PROFILE_SHADER_START

    // Get current viewport
    int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  gcpRendD3D->Set2DMode(false, 1, 1);       

  float fAmount = m_pAmount->GetParam();
  float fWatLevel = SPostEffectsUtils::m_fWaterLevel;

  static CCryName pTechName("UnderwaterGodRays");
  static CCryName pParam0Name("PI_GodRaysParamsVS");
  static CCryName pParam1Name("PI_GodRaysParamsPS");
  static CCryName pParam2Name("PI_GodRaysSunDirVS");

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Render god-rays into low-res render target for less fillrate hit

  gcpRendD3D->FX_PushRenderTarget(0,  CTexture::s_ptexBackBufferScaled[1], GetUtils().m_pCurDepthSurface); 
  gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexBackBufferScaled[1]->GetWidth(), CTexture::s_ptexBackBufferScaled[1]->GetHeight());        

  ColorF clearColor(0, 0, 0, 0);
  gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clearColor);

  //  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETSTATES);   
  uint32 nPasses;
  CShaderMan::m_shPostEffects->FXSetTechnique(pTechName);
  CShaderMan::m_shPostEffects->FXBegin(&nPasses, FEF_DONTSETSTATES);

  int nSlicesCount = 10;   
  bool bInVisarea = (gRenDev->m_p3DEngineCommon.m_pCamVisAreaInfo.nFlags & S3DEngineCommon::VAF_EXISTS_FOR_POSITION);
  Vec3 vSunDir = bInVisarea? Vec3(0.5f,0.5f,1.0f) :  -gEnv->p3DEngine->GetSunDirNormalized();

  for( int r(0); r < nSlicesCount; ++r)   
  {
    // !force updating constants per-pass!
    CShaderMan::m_shPostEffects->FXBeginPass(0);

    // Set per instance params  
    Vec4 pParams= Vec4(fWatLevel, fAmount, r, 1.0f / (float) nSlicesCount);
    CShaderMan::m_shPostEffects->FXSetVSFloat(pParam0Name, &pParams, 1);
    Vec4 pParams2 = Vec4(vSunDir.x, vSunDir.y, vSunDir.z, 1.0f);
    CShaderMan::m_shPostEffects->FXSetVSFloat(pParam2Name, &pParams2, 1);

    CShaderMan::m_shPostEffects->FXSetPSFloat(pParam1Name, &pParams, 1);

    gcpRendD3D->SetCullMode( R_CULL_NONE );
    gcpRendD3D->EF_SetState(GS_BLSRC_ONE | GS_BLDST_ONE | GS_NODEPTHTEST);

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 

    CShaderMan::m_shPostEffects->FXEndPass();
  }


  CShaderMan::m_shPostEffects->FXEnd(); 

  //  GetUtils().ShEndPass(); 

  gcpRendD3D->Set2DMode(true, 1, 1);       

  // Restore previous viewport
  gcpRendD3D->FX_PopRenderTarget(0);
  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);        

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Display god-rays

  CCryName pTechName0("UnderwaterGodRaysFinal");

  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName0, FEF_DONTSETSTATES);   
  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 
  GetUtils().ShEndPass(); 

  gcpRendD3D->FX_Flush();
  PROFILE_SHADER_END  

    PROFILE_LABEL_POP( "GODRAYS" );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CWaterDroplets::Render()
{
  //  float fDropletsAmount = m_pAmount->GetParam();

  static CCryName pTechName("WaterDroplets");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

  float fUserAmount = m_pAmount->GetParam();

  float fAtten = clamp_tpl<float>(fabs( gRenDev->GetRCamera().Orig.z - SPostEffectsUtils::m_fWaterLevel ), 0.0f, 1.0f);
  Vec4 vParams=Vec4(1, 1, 1, min(fUserAmount + (1.0f - clamp_tpl<float>(m_fCurrLifeTime, 0.0f, 1.0f)), 1.0f) * fAtten);    
  static CCryName pParamName("waterDropletsParams");
  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

  GetUtils().ShEndPass();   
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CWaterFlow::Render()
{
  float fAmount = m_pAmount->GetParam();

  static CCryName pTechName("WaterFlow");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

  Vec4 vParams=Vec4(1, 1, 1, fAmount);    
  static CCryName pParamName("waterFlowParams");
  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

  GetUtils().ShEndPass();   
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CWaterPuddles::Render()
{
  m_nCurrPuddleID++;
  m_nCurrPuddleID = m_nCurrPuddleID &1;
  CTexture *pPrevPuddle = CTexture::s_ptexWaterPuddles[m_nCurrPuddleID^1];
  CTexture *pCurrPuddle = CTexture::s_ptexWaterPuddles[m_nCurrPuddleID];

  // Get current viewport
  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  float fCurrentTime = gEnv->pTimer->GetCurrTime();
  float fTimeDif = fCurrentTime - m_fLastSpawnTime;
  Vec4 vParams = Vec4(0.0f, 0.0f, 0.0f, 0.0f);    
  if( fTimeDif > 0.125f )
  {
    m_fLastSpawnTime = fCurrentTime;
    vParams=Vec4(GetUtils().randf(), GetUtils().randf(), GetUtils().randf(), 1);//GetUtils().randf()>0.25f);    
  }

  {
    // spawn particles into effects accumulation buffer
    gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexWaterPuddlesDDN, GetUtils().m_pCurDepthSurface); 
    gcpRendD3D->RT_SetViewport(0, 0, pCurrPuddle->GetWidth(), pCurrPuddle->GetHeight()); 

    ColorF clearColor(0, 0, 0, 0);
    //gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clearColor);

    // compute wave propagation
    static CCryName pTechName("WaterPuddlesGen");
    GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETSTATES|FEF_DONTSETTEXTURES);   
    gcpRendD3D->EF_SetState(GS_NODEPTHTEST);      

    static CCryName pParamName("waterPuddlesParams");
    CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

    GetUtils().SetTexture(pPrevPuddle, 0, FILTER_POINT, 0);   
    GetUtils().SetTexture(pCurrPuddle, 1, FILTER_POINT, 0);   
    GetUtils().DrawFullScreenQuad(pCurrPuddle->GetWidth(), pCurrPuddle->GetHeight());

    GetUtils().ShEndPass(); 

    //GetUtils().CopyScreenToTexture(pCurrPuddle);     

    gcpRendD3D->FX_PopRenderTarget(0);    

    // Update current puddle
    GetUtils().StretchRect(CTexture::s_ptexWaterPuddlesDDN, pCurrPuddle);     

    gcpRendD3D->RT_SetViewport(0, 0, iWidth, iHeight);        
  }

  // make final normal map
  gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexWaterPuddlesDDN, GetUtils().m_pCurDepthSurface); 
  gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexWaterPuddlesDDN->GetWidth(), CTexture::s_ptexWaterPuddlesDDN->GetHeight()); 

  static CCryName pTechName("WaterPuddlesDisplay");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETSTATES|FEF_DONTSETTEXTURES);   
  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

  static CCryName pParamName("waterPuddlesParams");
  vParams.w = 256.0f;
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &vParams, 1);

  GetUtils().SetTexture(pCurrPuddle, 0, FILTER_LINEAR, 0);   
  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

  GetUtils().ShEndPass(); 

  gcpRendD3D->FX_PopRenderTarget(0);    
  gcpRendD3D->RT_SetViewport(0, 0, iWidth, iHeight);      

  // disable processing
  m_pAmount->SetParam(0.0f);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

namespace WaterVolumeStaticData
{
	CWater *pWaterSim = 0;
	void GetMemoryUsage( ICrySizer *pSizer )
	{
		if( pWaterSim )
			pWaterSim->GetMemoryUsage(pSizer);
	}
}

void CWaterVolume::Render()
{
  {
    static int nFrameID = 0;
    static bool bInitialize = true;
    static CWater pWaterSim;

		// remember ptr of WaterSim to access it with CrySizer
		WaterVolumeStaticData::pWaterSim = &pWaterSim;

    const int nGridSize = 64;

    int nCurFrameID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_nFrameID;
    if( nFrameID != nCurFrameID )
    {
      static Vec4 pParams0(0, 0, 0, 0), pParams1(0, 0, 0, 0);
      Vec4 pCurrParams0, pCurrParams1;
      gEnv->p3DEngine->GetOceanAnimationParams(pCurrParams0, pCurrParams1);

      // Update sim settings
      if( bInitialize || pCurrParams0.x != pParams0.x || pCurrParams0.y != pParams0.y ||
        pCurrParams0.z != pParams0.z || pCurrParams0.w != pParams0.w || pCurrParams1.x != pParams1.x || 
        pCurrParams1.y != pParams1.y || pCurrParams1.z != pParams1.z || pCurrParams1.w != pParams1.w )
      {
        pParams0 = pCurrParams0;
        pParams1 = pCurrParams1;
        pWaterSim.Create( 1.0, pParams0.x, pParams0.z, 1.0f, 1.0f);
        bInitialize = false;
      }

      // Create texture if required
      if (!CTexture::IsTextureExist(CTexture::s_ptexWaterVolumeTemp))
      {
        CTexture::s_ptexWaterVolumeTemp->Create2DTexture(64, 64, 1, 
          FT_DONT_RELEASE | FT_NOMIPS |  FT_USAGE_DYNAMIC, 
          0, eTF_A32B32G32R32F, eTF_A32B32G32R32F);
        CTexture::s_ptexWaterVolumeTemp->Fill(ColorF(0, 0, 0, 0));
      }

			CTexture *pTexture = CTexture::s_ptexWaterVolumeTemp;

      // Copy data..
      if (CTexture::IsTextureExist(pTexture))
      {
				const float fUpdateTime = 2.f*0.125f*gEnv->pTimer->GetCurrTime();













        pWaterSim.Update( nCurFrameID, fUpdateTime, true );

        Vec4 *pDispGrid = pWaterSim.GetDisplaceGrid();

        uint32 pitch = 4 * sizeof( float )*nGridSize; 
        uint32 width = nGridSize; 
        uint32 height = nGridSize;

















        STALL_PROFILER("update subresource")

        CDeviceTexture* pDevTex = pTexture->GetDevTexture();
        assert(pDevTex);
        STexLock rect;
        if (SUCCEEDED(pDevTex->LockRect(0, rect, LF_DISCARD))) //
        {
          cryMemcpy(rect.pData, pDispGrid, 4 * width * height* sizeof(f32) );
          pDevTex->UnlockRect(0);
        }




      }
      nFrameID = nCurFrameID;
    }
  }
  // Get current viewport
  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);





  // make final normal map
  gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexWaterVolumeDDN, GetUtils().m_pCurDepthSurface); 
  gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexWaterVolumeDDN->GetWidth(), CTexture::s_ptexWaterVolumeDDN->GetHeight()); 

  static CCryName pTechName("WaterVolumesNormalGen");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETSTATES|FEF_DONTSETTEXTURES);   
  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

  static CCryName pParamName("waterVolumesParams");
  Vec4 vParams = Vec4(64.0f, 64.0f, 64.0f, 64.0f);    
  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

  int32 nFilter = FILTER_LINEAR;



  
  GetUtils().SetTexture(CTexture::s_ptexWaterVolumeTemp, 0, nFilter, 0);   
  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

  GetUtils().ShEndPass(); 

  gcpRendD3D->FX_PopRenderTarget(0);    
  gcpRendD3D->RT_SetViewport(0, 0, iWidth, iHeight);      





  // disable processing
  m_pAmount->SetParam(0.0f);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CWaterRipples::Render()
{

  // for this frame, fetch all water volumes hits

  // render each hit in screen space

  // 1st test, we need 1 heightmap texture with following channel mapping:
  //		R= camera pos (current frame), G= camera pos( previous frame)
  //		B= next camera pos (current frame), A= next camera pos( previous frame)

  // after heightmap processed, we need to compute normal and store it in another texture
  // if only one camera position used, we could store normal in heigh.ba channels

}