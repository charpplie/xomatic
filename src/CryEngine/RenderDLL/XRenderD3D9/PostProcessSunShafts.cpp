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
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CSunShafts::Preprocess()
{  
  bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality( eRQ_Medium, eSQ_Medium );

  // We need to check every case and compare against merged post effects case

  m_bShaftsEnabled = true;

  if( !bQualityCheck )
    m_bShaftsEnabled &= false;

  if( gEnv->p3DEngine->GetSunColor().len2() < 0.01f)
    m_bShaftsEnabled &= false;

  if( m_pShaftsAmount->GetParam() < 0.01f && m_pRaysAmount->GetParam() < 0.01f)
    m_bShaftsEnabled &= false;

  // sun behind camera, can skip post process	
  const float fSunVisThreshold = 0.45f;
  float fLdotV = gEnv->p3DEngine->GetSunDirNormalized().dot( gRenDev->GetRCamera().Z );	
  if( fLdotV > fSunVisThreshold )
    m_bShaftsEnabled &= false;

  if( CRenderer::CV_r_sunshafts && IsActive() ) 
  {
    // Disable for interiors
    uint32 nCamVisAreaFlags = gRenDev->m_p3DEngineCommon.m_pCamVisAreaInfo.nFlags;
    if( (nCamVisAreaFlags& S3DEngineCommon::VAF_EXISTS_FOR_POSITION) && !(nCamVisAreaFlags& (S3DEngineCommon::VAF_CONNECTED_TO_OUTDOOR | S3DEngineCommon::VAF_AFFECTED_BY_OUT_LIGHTS)) )
      m_bShaftsEnabled &= false;
    else
      m_bShaftsEnabled &= true;
  }
  else
    m_bShaftsEnabled &= false;
  
  // Check if shafts occluded - if so skip them - todo: fade in/out shafts amount
  if( CRenderer::CV_r_sunshafts > 1 && m_bShaftsEnabled)
  {
    if( !m_pOcclQuery )
      Create();

    //bool bReady = m_pOcclQuery->IsReady();   
    bool bSunShaftsVisible =  ((int)m_nVisSampleCount > (CTexture::s_ptexBackBuffer->GetWidth() * CTexture::s_ptexBackBuffer->GetHeight()  / 100) );// || !bReady;  
    m_bShaftsEnabled &= bSunShaftsVisible; 

    m_nVisSampleCount = m_pOcclQuery->GetVisibleSamples( CRenderer::CV_r_sunshafts== 2 );   
    if( !m_pOcclQuery->GetDrawFrame() || m_pOcclQuery->IsReady() )  
    {
			PROFILE_LABEL_PUSH("SUNSHAFTS OCCLUSION");

      static CCryName pTechName("OcclCheckTechnique");

      GetUtils().ShBeginPass(CShaderMan::m_shPostSunShafts, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                    





      gcpRendD3D->SetCullMode( R_CULL_NONE );
      gcpRendD3D->EF_SetState( GS_DEPTHFUNC_LEQUAL | GS_COLMASK_NONE );   

      m_pOcclQuery->BeginQuery();
      SD3DPostEffectsUtils::DrawScreenQuadWPOS(0, 0, CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(),
        1.0f, 
        CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

      m_pOcclQuery->EndQuery();

      GetUtils().ShEndPass();                          

			PROFILE_LABEL_POP("SUNSHAFTS OCCLUSION");
    }      
  }

    
 // Check EdgeAA and Color grading cases - if any enabled enable merged sun shafts/edgeaa/colorgrading technique
  if( !m_bShaftsEnabled && CRenderer::CV_r_use_merged_posteffects && ( CRenderer::CV_r_useedgeaa==1 || CRenderer::CV_r_colorgrading && CRenderer::CV_r_colorgrading_charts ))		
    return true;

  return m_bShaftsEnabled;
}

///////////////////////////////////////////////////////////////////////////////////////////////////// 
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CSunShafts::Render() 
{ 
  PROFILE_SHADER_START
  
  if( CRenderer::CV_r_use_merged_posteffects )
	{
    PROFILE_LABEL_PUSH( "MERGED_SUNSHAFTS_EDGEAA_COLORCORRECTION" );
	}
  else
	{
    PROFILE_LABEL_PUSH( "SUNSHAFTS" );
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Update color grading

	if( CRenderer::CV_r_colorgrading && CRenderer::CV_r_colorgrading_charts )
	{
		CColorGrading *pColorGrad = 0;
		if( !PostEffectMgr().GetEffects().empty() )
			pColorGrad =  static_cast<CColorGrading*>( PostEffectMgr().GetEffect(ePFX_ColorGrading) );

		SColorGradingMergeParams pMergeParams;
		pColorGrad->UpdateParams( pMergeParams );
	}

  uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
  gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);

  float fShaftsType = m_pShaftsType->GetParam(); 

  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  // Use higher or lower texture resolution. The higher the more details, and more fillrate hit
  CTexture *pSunShafts = CTexture::s_ptexBackBufferScaled[1];

#if !defined(XENON) && !defined(PS3)
  if( gRenDev->m_RP.m_eQuality >= eRQ_High)
    pSunShafts = CTexture::s_ptexBackBufferScaled[0];
#endif

  Vec4 pShaftParams(0,0,0,0);
  static CCryName pParam2Name("PI_sunShaftsParams");
  if( m_bShaftsEnabled )
  {
    /////////////////////////////////////////////
    // Create shafts mask texture

    gcpRendD3D->FX_PushRenderTarget(0, pSunShafts, NULL);  
    gcpRendD3D->RT_SetViewport(0, 0, pSunShafts->GetWidth(), pSunShafts->GetHeight());        

    // Optimized mask gen requires ssao since it does resize ZTarget
    //		1. unoptimized pass does texture resampling to depth and scene target
    //		2. optimized version does texture resampling to scene target and modulates it with resampled ztarget for sunshafts masking
    const bool bUseOptMaskGen = CRenderer::CV_r_SSAO != 0;

    static CCryName pTech0Name("SunShaftsMaskGen");

    if( bUseOptMaskGen )
      gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

    GetUtils().ShBeginPass(CShaderMan::m_shPostSunShafts, pTech0Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

    gcpRendD3D->EF_SetState(GS_NODEPTHTEST);

    // Get sample size ratio (based on empirical "best look" approach)
    float fSampleSize = ((float)CTexture::s_ptexBackBuffer->GetWidth()/(float)pSunShafts->GetWidth()) * 0.5f;

    // Set samples position
    float s1 = fSampleSize / (float) CTexture::s_ptexBackBuffer->GetWidth();  // 2.0 better results on lower res images resizing        
    float t1 = fSampleSize / (float) CTexture::s_ptexBackBuffer->GetHeight();       
    // Use rotated grid
    Vec4 pParams0=Vec4(s1*0.95f, t1*0.25f, -s1*0.25f, t1*0.96f); 
    Vec4 pParams1=Vec4(-s1*0.96f, -t1*0.25f, s1*0.25f, -t1*0.96f);  

    static CCryName pParam3Name("texToTexParams0");
    static CCryName pParam4Name("texToTexParams1");

    CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam3Name, &pParams0, 1);        
    CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam4Name, &pParams1, 1); 

    if( !bUseOptMaskGen )
    {
      GetUtils().SetTexture(CTexture::s_ptexZTarget, 0, FILTER_POINT);
      GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 1, FILTER_LINEAR);
    }
    else
    {
      GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_LINEAR);
      GetUtils().SetTexture(CTexture::s_ptexZTargetScaled, 1, FILTER_POINT);		
    }

    gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   
    GetUtils().DrawFullScreenQuad(pSunShafts->GetWidth(), pSunShafts->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);  

    GetUtils().ShEndPass();   

    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;    

    // Restore previous viewport  
    gcpRendD3D->FX_PopRenderTarget(0);
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);        

    /////////////////////////////////////////////
    // Apply local radial blur to shafts mask

    gcpRendD3D->RT_SetViewport(0, 0, pSunShafts->GetWidth(), pSunShafts->GetHeight());        
    //gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

    static CCryName pTech1Name("SunShaftsGen");
    GetUtils().ShBeginPass(CShaderMan::m_shPostSunShafts, pTech1Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);     

    gcpRendD3D->EF_SetState(GS_NODEPTHTEST);

    static CCryName pParam0Name("SunShafts_ViewProj");
    CShaderMan::m_shPostSunShafts->FXSetVSFloat(pParam0Name, (Vec4 *) GetUtils().m_pViewProj.GetData(), 4);   
    Vec3 pSunPos = gEnv->p3DEngine->GetSunDir()*1000.0f;

    Vec4 pParams= Vec4(pSunPos, 1.0f);       
    static CCryName pParam1Name("SunShafts_SunPos");
    CShaderMan::m_shPostSunShafts->FXSetVSFloat(pParam1Name, &pParams, 1);

    // no need to waste gpu to compute sun screen pos
    Vec4 pSunPosScreen = GetUtils().m_pViewProj * pParams;
    pSunPosScreen.x = ( ( pSunPosScreen.x + pSunPosScreen.w) * 0.5f ) /pSunPosScreen.w;   
    pSunPosScreen.y = ( (-pSunPosScreen.y + pSunPosScreen.w) * 0.5f ) /pSunPosScreen.w; 
    pSunPosScreen.w = gEnv->p3DEngine->GetSunDirNormalized().dot( GetUtils().m_pViewProj.GetRow(2));

    // big radius, project until end of screen
    pShaftParams.x = 0.1f;    
    pShaftParams.y = clamp_tpl<float>(m_pRaysAttenuation->GetParam(), 0.0f, 10.0f);  

    CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam2Name, &pShaftParams, 1);     
    CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam1Name, &pSunPosScreen, 1);
    GetUtils().SetTexture(pSunShafts, 0, FILTER_LINEAR);     
    GetUtils().DrawFullScreenQuad(pSunShafts->GetWidth(), pSunShafts->GetHeight());  
    GetUtils().CopyScreenToTexture(pSunShafts);    // 8 samples

		GetUtils().ShEndPass();   

    //if( CRenderer::CV_r_sunshafts == 1 )
    {
			GetUtils().ShBeginPass(CShaderMan::m_shPostSunShafts, pTech1Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);     

      // interpolate between projections  
      pShaftParams.x = 0.025f;
      CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam2Name, &pShaftParams, 1);
      CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam1Name, &pSunPosScreen, 1);
      GetUtils().SetTexture(pSunShafts, 0, FILTER_LINEAR);    
      GetUtils().DrawFullScreenQuad(pSunShafts->GetWidth(), pSunShafts->GetHeight());  
      GetUtils().CopyScreenToTexture(pSunShafts);    // 64 samples  

			GetUtils().ShEndPass();   

			GetUtils().ShBeginPass(CShaderMan::m_shPostSunShafts, pTech1Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);     
      // smooth out final result
      pShaftParams.x = 0.01f;   
      CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam2Name, &pShaftParams, 1); 
      CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam1Name, &pSunPosScreen, 1);
      GetUtils().SetTexture(pSunShafts, 0, FILTER_LINEAR);    
      GetUtils().DrawFullScreenQuad(pSunShafts->GetWidth(), pSunShafts->GetHeight());        
      GetUtils().CopyScreenToTexture(pSunShafts);    // 512 samples  

      GetUtils().ShEndPass();   
    }

    // sun shafts by default
    gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
  }

  /////////////////////////////////////////////
  // Display sun shafts

  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);        

  if( CRenderer::CV_r_use_merged_posteffects && CRenderer::CV_r_useedgeaa )
    gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
  if( CRenderer::CV_r_use_merged_posteffects && CRenderer::CV_r_colorgrading && CRenderer::CV_r_colorgrading_charts)
    gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
	if( SPostEffectsUtils::tempRainTexture != 0 )
		gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];

  static CCryName pTech2Name("SunShaftsDisplay");
  GetUtils().ShBeginPass(CShaderMan::m_shPostSunShafts, pTech2Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);

  const float fSunVisThreshold = 0.45f;
  float fLdotV = clamp_tpl<float>( -(gEnv->p3DEngine->GetSunDirNormalized().dot( gRenDev->GetRCamera().Z )-fSunVisThreshold) * 4.0f , 0.0f, 1.0f); 
  pShaftParams.x = clamp_tpl<float>( m_pShaftsAmount->GetParam() * fLdotV, 0.0f, 1.0f);
  pShaftParams.y = clamp_tpl<float>(m_pRaysAmount->GetParam(), 0.0f, 10.0f);
  CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam2Name, &pShaftParams, 1);

  static CCryName pParam5Name("SunShafts_SunCol");
  Vec4 pRaysCustomCol = m_pRaysCustomCol->GetParamVec4();
  Vec3 pSunColor = gEnv->p3DEngine->GetSunColor();
  pSunColor.Normalize();
  pSunColor.SetLerp( Vec3(pRaysCustomCol.x, pRaysCustomCol.y, pRaysCustomCol.z), pSunColor, m_pRaysSunColInfluence->GetParam() ); 

  Vec4 pShaftsSunCol( pSunColor, 1);
  CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam5Name, &pShaftsSunCol, 1);

	GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, (gRenDev->m_RP.m_FlagsShader_RT&g_HWSR_MaskBit[HWSR_SAMPLE1])?FILTER_LINEAR:FILTER_POINT); 
  GetUtils().SetTexture(pSunShafts, 1, FILTER_LINEAR); 
  GetUtils().SetTexture(CTexture::s_ptexZTarget, 2, FILTER_POINT);        

	CColorGradingControllerD3D* pCtrl = gcpRendD3D->m_pColorGradingControllerD3D; 
	CTexture *pTexColorChar = pCtrl? pCtrl->GetColorChart() : 0;
	if( pTexColorChar )
		GetUtils().SetTexture(pTexColorChar, 3, FILTER_LINEAR);

	if (SPostEffectsUtils::tempRainTexture != 0)
		GetUtils().SetTexture(SPostEffectsUtils::tempRainTexture->m_pTexture, 4, FILTER_LINEAR);

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());  

  GetUtils().ShEndPass();   	

  gcpRendD3D->FX_Flush();

	PROFILE_SHADER_END    

  gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;

  if( CRenderer::CV_r_use_merged_posteffects )
  {
    PROFILE_LABEL_POP( "MERGED_SUNSHAFTS_EDGEAA_COLORCORRECTION" );
  }
  else
  {
    PROFILE_LABEL_POP( "SUNSHAFTS" );
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CPointLightShafts::Render()
{
  TArray<CDLight> &pLights = PostEffectMgr().GetLights();

  
  ColorF clearColor(0, 0, 0, 0);
  gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clearColor);

  uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
  gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);

  CTexture *pTexBuffers[2];
  pTexBuffers[0] = CTexture::s_ptexBackBufferScaled[0];
  pTexBuffers[1] = CTexture::s_ptexGlow;//TempBuffer4x;

  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
  gcpRendD3D->Set2DMode(false, 1, 1);     

//  gcpRendD3D->FX_PushRenderTarget(0, pTexBuffers[0], NULL);  
  gcpRendD3D->RT_SetViewport(0, 0, pTexBuffers[0]->GetWidth(), pTexBuffers[0]->GetHeight());         
 // gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clearColor);

  float fRatioWidth = (float)pTexBuffers[0]->GetWidth() / (float)CTexture::s_ptexSceneTarget->GetWidth();
  float fRatioHeight = (float)pTexBuffers[0]->GetHeight() / (float)CTexture::s_ptexSceneTarget->GetHeight();
  float fOffsetW = 1.0f;// / (float)pTexBuffers[0]->GetWidth();
  float fOffsetH = 1.0f;// / (float)pTexBuffers[0]->GetHeight();
    gcpRendD3D->RT_SetViewport(0, 0, pTexBuffers[0]->GetWidth(), pTexBuffers[0]->GetHeight());         

  uint32 nCurrBuffer=0;
  for(uint32 n = 0; n < pLights.Num(); ++n)
  {  
    Vec3 pLightPos = pLights[n].m_Origin;
    float fInvRadius = 1.0f / pLights[n].m_fRadius;
    Vec4 pLightCol = Vec4(pLights[n].m_Color.r, pLights[n].m_Color.g, pLights[n].m_Color.b, 1);     

    Vec4 pLightRect = Vec4(pLights[n].m_sX * fRatioWidth +0.5f , pLights[n].m_sY * fRatioHeight+0.5f, pLights[n].m_sWidth * fRatioWidth+fOffsetW, pLights[n].m_sHeight * fRatioHeight+fOffsetH);        


    // create mask/light volume
    {      
      gcpRendD3D->FX_PushRenderTarget(0, pTexBuffers[0], NULL);  
      gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clearColor);
      gcpRendD3D->EF_Scissor(true, (int)pLightRect.x, (int)pLightRect.y, (int)pLightRect.z, (int)pLightRect.w);           

      static CCryName pTech1Name("OmniLightShafts");
      GetUtils().ShBeginPass(CShaderMan::m_shPostSunShafts, pTech1Name, FEF_DONTSETSTATES);     

      static CCryName pParam0Name("mViewProj");
      CShaderMan::m_shPostSunShafts->FXSetVSFloat(pParam0Name, (Vec4 *) GetUtils().m_pViewProj.GetData(), 4);   

      Vec4 pParams= Vec4(pLightPos, fInvRadius);       
      static CCryName pParam1Name("LightPos");
      CShaderMan::m_shPostSunShafts->FXSetVSFloat(pParam1Name, &pParams, 1);
      CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam1Name, &pParams, 1);

      CCryName pParam5Name("LightCol");
      CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam5Name, &pLightCol, 1); 
      
      gcpRendD3D->EF_SetState(GS_NODEPTHTEST);

      GetUtils().DrawScreenQuadWPOS(0, 0, CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(),
                                    0.0f, 
                                    CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());
      
      GetUtils().ShEndPass();   	
      
//      GetUtils().CopyScreenToTexture(pTexBuffers[0]);    // 8 samples
      gcpRendD3D->FX_PopRenderTarget(0);
    }

    // Do radial blur..
    if( 1 )
    { 
      for(int p = 0; p < 2; ++p)
      {
        gcpRendD3D->RT_SetViewport(0, 0, pTexBuffers[0]->GetWidth(), pTexBuffers[0]->GetHeight());         

        static CCryName pTech1Name("ShaftsBlurPS");
        CCryName pTech2Name("ShaftsBlur");
        GetUtils().ShBeginPass(CShaderMan::m_shPostSunShafts, pTech2Name , FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);     


        static CCryName pParam0Name("mViewProj");
        CShaderMan::m_shPostSunShafts->FXSetVSFloat(pParam0Name, (Vec4 *) GetUtils().m_pViewProj.GetData(), 4);   


        Vec4 pParams= Vec4(pLightPos, max(0.0f, ((3.0f - (float)p) / 3.0f)));       
        //if( p==0 )
        //  pParams.w=0.1f;
        //if( p==1 )
        //  pParams.w=0.025f;
        //if( p==2 )
        //  pParams.w=0.0125f;

        //Vec4 pParams= Vec4(pLightPos, 1.0f );       
        static CCryName pParam1Name("LightPos");
        CShaderMan::m_shPostSunShafts->FXSetVSFloat(pParam1Name, &pParams, 1);
        CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam1Name, &pParams, 1);

        static CCryName pParam5Name("LightCol");      
        CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam5Name, &pLightCol, 1); 

        GetUtils().SetTexture(pTexBuffers[0], 0, FILTER_LINEAR);
        GetUtils().SetTexture(CTexture::s_ptexScreenNoiseMap, 1, FILTER_POINT, 0);     	

        gcpRendD3D->EF_SetState(GS_NODEPTHTEST);      

        gcpRendD3D->FX_Commit(); // workaround - fx_commit changes viewport which invalidates scissor rects - commit before scissor test set
        gcpRendD3D->EF_Scissor(true, (int)pLightRect.x, (int)pLightRect.y, (int)pLightRect.z, (int)pLightRect.w);          

        GetUtils().DrawScreenQuadWPOS(0, 0, CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(),
          0.0f, 
         CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

        GetUtils().ShEndPass();   	

        GetUtils().CopyScreenToTexture(pTexBuffers[0]);
      }
    }        
    
    if( 1 )
    { 
      if( n )
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
         
      // add to accumulation
      gcpRendD3D->RT_SetViewport(0, 0, pTexBuffers[0]->GetWidth(), pTexBuffers[0]->GetHeight());         

      CCryName pTech1Name("ShaftsAdd");
      GetUtils().ShBeginPass(CShaderMan::m_shPostSunShafts, pTech1Name , FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);     

      static CCryName pParam0Name("mViewProj");
      CShaderMan::m_shPostSunShafts->FXSetVSFloat(pParam0Name, (Vec4 *) GetUtils().m_pViewProj.GetData(), 4);   

      Vec4 pParams= Vec4(pLightPos, fInvRadius);       
      static CCryName pParam1Name("LightPos");
      CShaderMan::m_shPostSunShafts->FXSetVSFloat(pParam1Name, &pParams, 1);
      CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam1Name, &pParams, 1);

      static CCryName pParam5Name("LightCol");      
      CShaderMan::m_shPostSunShafts->FXSetPSFloat(pParam5Name, &pLightCol, 1); 

      GetUtils().SetTexture(pTexBuffers[0], 0, FILTER_POINT);
      if( n )
        GetUtils().SetTexture(pTexBuffers[1], 1, FILTER_POINT);

      // only alpha writes
      //gcpRendD3D->EF_SetState(GS_NODEPTHTEST/*|GS_COLMASK_A*/);
      gcpRendD3D->EF_SetState(GS_NODEPTHTEST);      

      gcpRendD3D->FX_Commit(); // workaround - fx_commit changes viewport which invalidates scissor rects - commit before scissor test set
      gcpRendD3D->EF_Scissor(true, (int)pLightRect.x, (int)pLightRect.y, (int)pLightRect.z, (int)pLightRect.w);          

      GetUtils().DrawScreenQuadWPOS(0, 0, CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(),
        0.0f, 
        CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

      GetUtils().ShEndPass();   	

      GetUtils().CopyScreenToTexture(pTexBuffers[1]);
    }        

    gcpRendD3D->EF_Scissor(false, 0, 0, 0, 0);
  }
  // Restore previous viewport  
  //gcpRendD3D->FX_PopRenderTarget(0);
  gcpRendD3D->Set2DMode(true, 1, 1);   

  gcpRendD3D->EF_Scissor(false, 0, 0, 0, 0);

  GetUtils().TexBlurIterative(pTexBuffers[1], 1);

  // Blend shafts
  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);        

  static CCryName pTechName("ShaftsBlend");
  GetUtils().ShBeginPass(CShaderMan::m_shPostSunShafts, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

  //gcpRendD3D->EF_SetState(GS_NODEPTHTEST|GS_BLSRC_ONEMINUSDSTCOL|GS_BLDST_ONE); 
  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);//|GS_BLSRC_ONEMINUSDSTCOL|GS_BLDST_ONE); 

  

  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT); 
  //pTexBuffers[0] = CTexture::s_ptexBackBufferScaled[0];
  GetUtils().SetTexture(pTexBuffers[1], 1, FILTER_LINEAR); 
  
  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());  

  GetUtils().ShEndPass();   	
  
  gRenDev->m_RP.m_FlagsShader_RT =nSaveFlagsShader_RT ; 
  m_nRenderFlags|=PSP_UPDATE_BACKBUFFER;
}
