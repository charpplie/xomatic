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

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Game/Hud specific post-processing
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CD3D9Renderer::FX_CustomRenderScene( bool bEnable )
{
  if(bEnable)
  {
    GetUtils().Log(" +++ Begin custom render scene +++ \n"); 
		if( CRenderer::CV_r_customvisions == 1)
		{
			FX_PushRenderTarget(0, CTexture::s_ptexBackBuffer, &gcpRendD3D->m_DepthBufferOrig);
			RT_SetViewport(0, 0, GetWidth(), GetHeight());                        
			ColorF clearColor(0, 0, 0, 0);
			EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clearColor); // //FRT_CLEAR_DEPTH|
		}

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_CUSTOM_RENDER_PASS;
  }
  else  
  { 
		if( CRenderer::CV_r_customvisions == 1)
		  FX_PopRenderTarget(0);    

    FX_ResetPipe();     

    GetUtils().Log(" +++ End custom render scene +++ \n"); 

    RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_CUSTOM_RENDER_PASS;
  }

  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CCryVision::Render()
{
  //todo: use lower res rt for low spec
  bool bLowSpec = gRenDev->m_RP.m_eQuality == eRQ_Low;
  float fType = m_pType->GetParam();

  gcpRendD3D->Set2DMode(false, 1, 1);






  // render to texture all masks
  gcpRendD3D->FX_ProcessPostRenderLists(FB_CUSTOM_RENDER);

	gcpRendD3D->Set2DMode(true, 1, 1);

	if( CRenderer::CV_r_customvisions == 2 )
		return;

  gcpRendD3D->RT_SetViewport(GetUtils().m_pScreenRect.left, GetUtils().m_pScreenRect.top, GetUtils().m_pScreenRect.right, GetUtils().m_pScreenRect.bottom); 
  GetUtils().m_pCurDepthSurface = &gcpRendD3D->m_DepthBufferOrig;

  CTexture *pScreen = CTexture::s_ptexBackBuffer;
  CTexture *pMask = CTexture::s_ptexBlack;
  CTexture *pMaskBlurred = CTexture::s_ptexBlack;

  // skip processing, nothing was added to mask
  if( ( SRendItem::BatchFlags(EFSLIST_GENERAL, gRenDev->m_RP.m_nProcessThreadID, gRenDev->m_RP.m_pRLD) | SRendItem::BatchFlags(EFSLIST_TRANSP, gRenDev->m_RP.m_nProcessThreadID, gRenDev->m_RP.m_pRLD) ) & FB_CUSTOM_RENDER)
  {
    // hi specs only, else render directly to frame buffer
    {
      // store silhouettes/signature temporary render target, so that we can post process this after wards
      if( !bLowSpec )
      {
        gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexSceneTarget,GetUtils().GetDepthSurface( CTexture::s_ptexSceneTarget ));
        gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight());
      }

      static CCryName pTech1Name("BinocularView");
      GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTech1Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

      gRenDev->EF_SetState(GS_NODEPTHTEST |((!bLowSpec)? 0 : GS_BLSRC_ONE | GS_BLDST_ONE), 0); 

      // Set PS default params
      Vec4 pParams= Vec4(0, 0, 0, (!fType)? 1.0f : 0.0f);
      CCryName pParamName("psParams");
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1); 

      GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);   
      GetUtils().SetTexture(CTexture::s_ptexZTarget, 1, FILTER_POINT);   
      GetUtils().DrawFullScreenQuad(CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight());

      GetUtils().ShEndPass();
      if( !bLowSpec )
      {
        gcpRendD3D->FX_PopRenderTarget(0);    
        pMask = CTexture::s_ptexSceneTarget;
        pMaskBlurred = CTexture::s_ptexBackBufferScaled[0];
      }
    }

    // hi specs only add mask into glow render target
    if( !bLowSpec )
    {
      gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexBackBufferScaled[0], &gcpRendD3D->m_DepthBufferOrig);
      gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexBackBufferScaled[0]->GetWidth(), CTexture::s_ptexBackBufferScaled[0]->GetHeight());

      static CCryName pTech0Name("BinocularViewGlow");
      GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTech0Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
      gRenDev->EF_SetState(GS_NODEPTHTEST, 0); 

      GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_LINEAR);    
      GetUtils().SetTexture(CTexture::s_ptexSceneTarget, 1, FILTER_LINEAR);   
      GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBufferScaled[0]->GetWidth(), CTexture::s_ptexBackBufferScaled[0]->GetHeight());

      GetUtils().ShEndPass();

      gcpRendD3D->FX_PopRenderTarget(0);    

      gcpRendD3D->RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());

      // blur - for glow
      GetUtils().TexBlurGaussian(CTexture::s_ptexBackBufferScaled[0], 1, 1.25f, 1.5f, false);                  
    }
  }

  if( !bLowSpec )
  {
    float fBlendParam = clamp_tpl<float>( m_pAmount->GetParam(), 0.0f, 1.0f) ;





    // update backbuffer
    GetUtils().CopyScreenToTexture( CTexture::s_ptexBackBuffer); 


    static CCryName pTech2Name("BinocularViewFinal");
    static CCryName pTech3Name("BinocularViewFinalNoTinting");

    GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, (!fType)? pTech2Name : pTech3Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    gRenDev->EF_SetState(GS_NODEPTHTEST, 0); 

    GetUtils().SetTexture(pScreen, 0, FILTER_POINT);   
    GetUtils().SetTexture(pMask, 1, FILTER_POINT);   
    GetUtils().SetTexture(pMaskBlurred, 2);   

    // Set PS default params
    Vec4 pParams= Vec4(0, 0, 0, fBlendParam);
    static CCryName pParamName("psParams");
    CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

    GetUtils().ShEndPass();

    gcpRendD3D->RT_SetViewport(GetUtils().m_pScreenRect.left, GetUtils().m_pScreenRect.top, GetUtils().m_pScreenRect.right, GetUtils().m_pScreenRect.bottom); 
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CAlienInterference::Render()
{
  float fAmount = m_pAmount->GetParam();

  static CCryName pTechName("AlienInterference");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

  Vec4 vParams=Vec4(1, 1, (float)GetUtils().m_iFrameCounter, fAmount);    
  static CCryName pParamName("psParams");
  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

  GetUtils().ShEndPass();   

}


/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CBloodSplats::Preprocess()
{
  bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality( eRQ_High, eSQ_High );
  if( !bQualityCheck )
    return false;

  float fAmount = m_pAmount->GetParam();
  float fSpawn = m_pSpawn->GetParam();

  if( fAmount > 0.005f && !(fAmount<=0.02f || (m_fSpawnTime == 0.0f && fSpawn == 0)) )
  {
    return true;
  }

  ColorF c = ColorF(1, 1, 1, 1);
  CTexture::s_ptexEffectsAccum->Fill(c);
  m_nAccumCount = 0;

  return false;
}

void CBloodSplats::Render()
{
  float fAmount = m_pAmount->GetParam();

  float fSpawn = m_pSpawn->GetParam();
  float fScale = m_pScale->GetParam();

  fScale *= 2.0f;
  fScale = min(fScale, 1.0f);

  static uint32 nCount = 0;

  GetUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[1]);   
  //GetUtils().TexBlurGaussian(CTexture::s_ptexBackBufferScaled[1], 1, 1.0f, 5.0f );              

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Generate blood splats displacement texture

  // Get current viewport
  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  if(fSpawn) 
  {
    // spawn particles into effects accumulation buffer
    gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexEffectsAccum, GetUtils().m_pCurDepthSurface); 
    gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexEffectsAccum->GetWidth(), CTexture::s_ptexEffectsAccum->GetHeight());        

    if( !m_nAccumCount ) // clear first time using
    {
      ColorF clearColor(1, 1, 1, 1);
      gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clearColor);
      m_nAccumCount = 1;
    }

    static CCryName pTechName("BloodSplatsGen");
    GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);   

    gcpRendD3D->EF_SetState(GS_BLSRC_DSTCOL | GS_BLDST_ZERO | GS_NODEPTHTEST);      

    // place random sized 'blood' quads in screen
    int w = CTexture::s_ptexBackBuffer->GetWidth(), h = CTexture::s_ptexBackBuffer->GetHeight();
    for(int s(0); s<20; ++s)        
    {
      float x0 = (cry_rand()%(w)) / (float) w;  
      float y0 = (cry_rand()%(h)) / (float) h;

      float x1 = x0 + fScale * ((cry_rand()%300) / (float) w);
      float y1 = y0 + fScale * ((cry_rand()%300) / (float) h);          

      GetUtils().DrawScreenQuad(w, h, x0, y0, x1, y1);
    }

    GetUtils().ShEndPass();   

    // Restore previous view port
    gcpRendD3D->FX_PopRenderTarget(0);
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

    // save spawn time and reset spawning
    m_fSpawnTime = GetUtils().m_pTimer->GetCurrTime();
    m_pSpawn->SetParam(0.0f);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Apply flow
  float fTimeDif = GetUtils().m_pTimer->GetCurrTime() - m_fSpawnTime;
  Vec4 vParams=Vec4(1, 1, min(fTimeDif, 2.0f), (fTimeDif < 0.5f)?1 : 0 );               

  static CCryName pParamName("bloodSplatsParams");

  if(fTimeDif<2.0f) 
  {
    gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexEffectsAccum->GetWidth(), CTexture::s_ptexEffectsAccum->GetHeight());        

    static CCryName pTechName("BloodSplatsFlow");
    GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);   

    gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

    CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexEffectsAccum->GetWidth(), CTexture::s_ptexEffectsAccum->GetHeight());

    GetUtils().ShEndPass();   

    GetUtils().CopyScreenToTexture(CTexture::s_ptexEffectsAccum);     
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);
  }


  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Display splats

  float fType = m_pType->GetParam();

  if(fType == 0)
  {
    // display human blood
    static CCryName pTechName("BloodSplatsFinal");
    GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);   

    gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

    vParams=Vec4(1, 1, 1, fAmount);    
    CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

    GetUtils().ShEndPass();   
  }
  else
  {
    // display alien blood

    gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexBackBufferScaled[0]->GetWidth(), CTexture::s_ptexBackBufferScaled[0]->GetHeight());        

    // Generate blood texture
    static CCryName pTech0Name("AlienBloodSplatsFinal");
    GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTech0Name, FEF_DONTSETSTATES);   

    gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

    vParams=Vec4(1, 1, 1, fAmount);    
    CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

    GetUtils().ShEndPass();   

    // Copy current blood result into a temporary texture
    GetUtils().CopyScreenToTexture(CTexture::s_ptexBackBufferScaled[0]);  
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

    // Blur result
    GetUtils().StretchRect(CTexture::s_ptexBackBufferScaled[0], CTexture::s_ptexBackBufferScaled[2]);                  
    GetUtils().TexBlurGaussian(CTexture::s_ptexBackBufferScaled[2], 1, 1.25f,5.0f , false);                         

    // Display alien blood texture and add glowing
    static CCryName pTech1Name("AlienBloodSplatsGlow");
    GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTech1Name, FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);   

    gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

    vParams=Vec4(1, 1, 1, fAmount);    
    CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

    GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);   
    GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[0], 1);   
    GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[2], 2);    

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

    GetUtils().ShEndPass();       
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CScreenFrost::Render()
{
  float fAmount = m_pAmount->GetParam();

  if(fAmount<=0.02f) 
  {
    m_fRandOffset = ((float) cry_rand() / (float) RAND_MAX); 
    return;
  }

  float fCenterAmount = m_pCenterAmount->GetParam();

  GetUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[1]);    

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // display frost
  static CCryName pTechName("ScreenFrost");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

  static CCryName pParam0Name("screenFrostParamsVS");
  static CCryName pParam1Name("screenFrostParamsPS");

  GetUtils().ShSetParamVS(pParam0Name, Vec4(1, 1, 1, m_fRandOffset));
  GetUtils().ShSetParamPS(pParam1Name, Vec4(1, 1, fCenterAmount, fAmount));

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

  GetUtils().ShEndPass();   
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CNightVision::Preprocess()
{
  m_bWasActive = IsActive();
  if ( (IsActive() || m_fActiveTime )&& m_pGradient && m_pNoise && gRenDev->CV_r_nightvision )
    return true;


  return false;
}

void CNightVision::Render()
{ 
  m_fActiveTime += ( (m_bWasActive)? 4.0f : -4.0f ) * gEnv->pTimer->GetFrameTime();
  m_fActiveTime = clamp_tpl<float>(m_fActiveTime, 0.0f, 1.0f);

  Vec4 vParamsPS = Vec4(1, 1, 0, m_fActiveTime);                
  Vec4 vParamsVS = Vec4(cry_rand()%1024, cry_rand()%1024, cry_rand()%1024, cry_rand()%1024 );                

  uint64 nFlagsShader_RT = gcpRendD3D->m_RP.m_FlagsShader_RT; 

  GetUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[1]);    
  GetUtils().TexBlurGaussian(CTexture::s_ptexBackBufferScaled[1], 1, 1.25f, 1.5f, false);                  

  if( CRenderer::CV_r_HDRRendering )    
    gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_HDR_MODE];

  static CCryName pTechName("NightVision"); 
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

  static CCryName pParamNameVS("nightVisionParamsVS");
  static CCryName pParamNamePS("nightVisionParamsPS");
  CShaderMan::m_shPostEffectsGame->FXSetVSFloat(pParamNameVS, &vParamsVS, 1);
  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamNamePS, &vParamsPS, 1);

  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);     
  GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[1], 1, FILTER_LINEAR);   
  GetUtils().SetTexture( m_pNoise, 2, FILTER_POINT, 0);   
  GetUtils().SetTexture( m_pGradient, 3, FILTER_LINEAR);    
  if( CRenderer::CV_r_HDRRendering )    
    GetUtils().SetTexture(CTexture::s_ptexCurLumTexture, 4, FILTER_POINT);   

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());
  GetUtils().ShEndPass();   

  gcpRendD3D->m_RP.m_FlagsShader_RT = nFlagsShader_RT;

  m_bWasActive = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CFlashBang::Preprocess()
{

  float fActive = m_pActive->GetParam();
  if( fActive || m_fSpawnTime)
  {
    if( fActive )
      m_fSpawnTime = 0.0f;

    m_pActive->SetParam( 0.0f );

    return true;
  }


  return false;
}

void CFlashBang::Render()
{
  float fTimeDuration = m_pTime->GetParam();
  float fDifractionAmount = m_pDifractionAmount->GetParam();
  float fBlindTime = m_pBlindAmount->GetParam();

  if( !m_fSpawnTime )
  {
    m_fSpawnTime = GetUtils().m_pTimer->GetCurrTime();

    // Create temporary ghost image and capture screen
    SAFE_DELETE( m_pGhostImage );

    m_pGhostImage = new SDynTexture(CTexture::s_ptexBackBuffer->GetWidth()>>1, CTexture::s_ptexBackBuffer->GetHeight()>>1, eTF_A8R8G8B8, eTT_2D,  FT_STATE_CLAMP, "GhostImageTempRT");  
    m_pGhostImage->Update( CTexture::s_ptexBackBuffer->GetWidth()>>1, CTexture::s_ptexBackBuffer->GetHeight()>>1 );

    if( m_pGhostImage && m_pGhostImage->m_pTexture )
    {
      GetUtils().StretchRect(CTexture::s_ptexBackBuffer, m_pGhostImage->m_pTexture);        
    }
  } 

  // Update current time
  float fCurrTime = (GetUtils().m_pTimer->GetCurrTime() - m_fSpawnTime) / fTimeDuration;  

  // Effect finished
  if( fCurrTime > 1.0f )
  {
    m_fSpawnTime = 0.0f;
    m_pActive->SetParam(0.0f);

    SAFE_DELETE( m_pGhostImage );

    return;
  }  

  // make sure to update dynamic texture if required
  if( m_pGhostImage && !m_pGhostImage->m_pTexture )
  {
    m_pGhostImage->Update( CTexture::s_ptexBackBuffer->GetWidth()>>1, CTexture::s_ptexBackBuffer->GetHeight()>>1 );
  }

  if( !m_pGhostImage || !m_pGhostImage->m_pTexture )
  {        
    return;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////    
  static CCryName pTechName("FlashBang");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETSTATES|FEF_DONTSETTEXTURES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

  float fLuminance = 1.0f  - fCurrTime; //GetUtils().InterpolateCubic(0.0f, 1.0f, 0.0f, 1.0f, fCurrTime);

  // opt: some pre-computed constants
  Vec4 vParams = Vec4( fLuminance, fLuminance * fDifractionAmount, 3.0f * fLuminance * fBlindTime, fLuminance );                
  static CCryName pParamName("vFlashBangParams");
  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);     
  GetUtils().SetTexture(m_pGhostImage->m_pTexture, 1, FILTER_LINEAR);     

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

  GetUtils().ShEndPass();   
}
