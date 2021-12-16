/*=============================================================================
D3DPostProcess : Direct3D specific post processing special effects
Copyright (c) 2001 Crytek Studios. All Rights Reserved.

Revision history:
* 23/02/2005: Re-factored/Converted to CryEngine 2.0 by Tiago Sousa
* Created by Tiago Sousa

Todo:
* Cleanup code - big mess
* When we have a proper static branching support use it instead of shader switches inside code

=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "D3DPostProcess.h"

#pragma warning(disable: 4244)

using namespace NPostEffects;

bool CD3D9Renderer::FX_GlowScene(bool bEnable)
{
  
  CTexture *pSceneTarget =  CTexture::s_ptexSceneTarget; //(CRenderer::CV_r_debug_extra_scenetarget_fsaa && gcpRendD3D->m_RP.m_FSAAData.Type)?CTexture::m_Text_SceneTargetFSAA : CTexture::s_ptexSceneTarget;    
  SD3DSurface *pCurrDepthBuffer = GetUtils().GetDepthSurface( pSceneTarget );
	
	bool bHDRModeEnabled = gcpRendD3D->IsHDRModeEnabled();
	bool bLinearSpaceShadingEnabled = gcpRendD3D->IsLinearSpaceShadingEnabled();
	bool bMergeWithHDR = bHDRModeEnabled;

	if(bEnable)
	{
		PROFILE_LABEL_PUSH( "GLOWGEN" );
		GetUtils().Log(" +++ Begin Glow scene +++ \n"); 
	}
	else
	{
		GetUtils().Log(" +++ End Glow scene +++ \n"); 
		PROFILE_LABEL_POP( "GLOWGEN" );
	}


	// r_Glow 2 mode is fastest mode: renders glow directly to hdr scene target - will be handled as usual by tone mapping/eye adaptation
	if( /*CRenderer::CV_r_glow == 2 && */bMergeWithHDR )
		return true;

  if(bEnable)
  {
		gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_NO_SRGBWRITES;

#if defined( XENON )
		// Disable gamma conversion during resolve
		if( bLinearSpaceShadingEnabled && !bHDRModeEnabled )
			gcpRendD3D->XE_SRGBWriteEnable( false );

		GetUtils().PushEDRAM();
#endif

		// For x360 we do this steps a bit differently to minimize amount of resolves and overall cost
		//	- Resolve only current scene target
		//	- Draw geometry
		//	- Resolve directly from 4x msaa target (no downsample step)
		//	- Restore scene target using additive blending (one less resolve and less fetch in shader)

#if !defined( XENON  )
    FX_PushRenderTarget(0, pSceneTarget, pCurrDepthBuffer);
#endif
    RT_SetViewport(0, 0, GetWidth(), GetHeight());                        
    ColorF clearColor(0, 0, 0, 0);











    {
      EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clearColor);
    }

  }
  else  
  { 
#if !defined( XENON  )
    FX_PopRenderTarget(0);    
#endif
        
    FX_ResetPipe();     
    
    RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());

		gcpRendD3D->Set2DMode(true, 1, 1);           
    GetUtils().m_pCurDepthSurface = &gcpRendD3D->m_DepthBufferOrig;

    CTexture *pSrc = pSceneTarget;    

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Copy glow from framebuffer into glow texture

		int32 nRenderState = GS_NODEPTHTEST;      

#if defined( XENON  )
		gcpRendD3D->XE_HalfScaleMode(true, 4, true);
		CTexture::s_ptexGlow->SetResolved(false);
		CTexture::s_ptexGlow->Resolve();
		gcpRendD3D->XE_HalfScaleMode(false, 4, true);
#else

    // Todo: add custom renderstate support for stretchrect
//  if( ( m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_LIGHTSHAFTS ) && !CRenderer::CV_r_HDRRendering)
//    nRenderState |= GS_BLSRC_ONE | GS_BLDST_ONE;

    GetUtils().StretchRect(pSrc, CTexture::s_ptexGlow);

#endif

    RT_SetViewport(0, 0, GetWidth(), GetHeight());

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Add glow/decode into frame-buffer
		uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;  
		gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);

		if ( bHDRModeEnabled )
			gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

		static CCryName pTech1Name("GlowScene");
    GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTech1Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

		nRenderState = GS_NODEPTHTEST| GS_BLSRC_ONE | GS_BLDST_ONE; 

#if defined( XENON )
		GetUtils().SetTexture((!CRenderer::CV_r_HDRRendering)? CTexture::s_ptexBackBuffer:CTexture::s_ptexHDRTarget, 0, FILTER_POINT);    
#else
		GetUtils().SetTexture(pSrc, 0, FILTER_POINT);    
    nRenderState |= GS_ALPHATEST_GREATER;    
#endif
    
    gRenDev->EF_SetState(nRenderState, 0);     
    GetUtils().DrawFullScreenQuad(pSrc->GetWidth(), pSrc->GetHeight());

    GetUtils().ShEndPass();

    gcpRendD3D->Set2DMode(false, 1, 1);     
    gcpRendD3D->RT_SetViewport(GetUtils().m_pScreenRect.left, GetUtils().m_pScreenRect.top, GetUtils().m_pScreenRect.right, GetUtils().m_pScreenRect.bottom); 

    // Enable glow postprocessing
    CEffectParam *pParam = PostEffectMgr().GetByName("Glow_Active"); 
    assert(pParam && "Parameter doesn't exist");
    pParam->SetParam(1.0f);   

#if defined(XENON  )
		// Re-enable gamma conversion
		if( bLinearSpaceShadingEnabled && !bHDRModeEnabled )
			gcpRendD3D->XE_SRGBWriteEnable( true );

		if ( bHDRModeEnabled )
			CTexture::s_ptexHDRTarget->SetResolved(false);

#endif
    gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_NO_SRGBWRITES;
		gcpRendD3D->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;  







  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////// 
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CGlow::Render() 
{    
  PROFILE_LABEL_PUSH( "GLOW" );

  float fGlowScale = m_pScale->GetParam();

  PROFILE_SHADER_START

  uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;  
  gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);

  // Get current viewport
  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  if( CRenderer::CV_r_glowanamorphicflares && CRenderer::CV_r_glow == 1 && m_pActive->GetParam()) //&& (gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_GLOW_FLARES) )
  {
    // "Anamorphic" blur
    //  - similar to a separated Gaussian blur, we use 9 samples, but blur along an arbitrary direction
    //  - in next steps we smooth input result, ping-ponging between rt's to avoid resolves on some platforms

    PROFILE_LABEL_PUSH( "ANAMORPHIC_FLARES" );

    static CCryName pTechName("AnamorphicBlur");
    static CCryName pParamName("glowParamsPS"); 
    static CCryName pParamOffsets("glowOffsetsPS"); 
    static CCryName pParamWeights("glowWeightsPS"); 

    CTexture *pFlareTex = CTexture::s_ptexBackBufferScaled[1];
    SDynTexture *tpBlurTemp[2];
    tpBlurTemp[0] = new SDynTexture(pFlareTex->GetWidth(), pFlareTex->GetHeight(), pFlareTex->GetDstFormat(), eTT_2D,  FT_STATE_CLAMP | FT_USAGE_RENDERTARGET, "FlareHorizontal");
    tpBlurTemp[0]->Update( pFlareTex->GetWidth(), pFlareTex->GetHeight() );
    tpBlurTemp[1] = new SDynTexture(pFlareTex->GetWidth(), pFlareTex->GetHeight(), pFlareTex->GetDstFormat(), eTT_2D,  FT_STATE_CLAMP | FT_USAGE_RENDERTARGET, "FlareRotated");
    tpBlurTemp[1]->Update( pFlareTex->GetWidth(), pFlareTex->GetHeight() );

    if( !tpBlurTemp[0]->m_pTexture || !tpBlurTemp[1]->m_pTexture)
    {
      SAFE_DELETE(tpBlurTemp[0]);
      SAFE_DELETE(tpBlurTemp[1]);
      return;
    }

    // downsample initial glow into half res
    gcpRendD3D->RT_SetViewport(0, 0, pFlareTex->GetWidth(), pFlareTex->GetHeight());
    GetUtils().StretchRect(CTexture::s_ptexGlow, pFlareTex);      

    // Set default streaks stretch amount
    float fStreakStretch = 15.0f * m_pStreaksStretch->GetParam();

    const float fSampleCount = 9.0f;
    const float fRecipSamples = 1.0f / fSampleCount;

//    fWeight =  ( 1.h *fRecipSamples )*2.h-1.h;
//    vDir = float2( fWeight, 0)* glowParamsPS.zw * glowParamsPS.x;
//#if !%_RT_SAMPLE0
//    vDir*=0.5;  
//    vDir = 0.707 * vDir + 0.707 * float2(-vDir.y, vDir.x);
//#endif
//    cCurr = half4( tex2D(_tex0, IN.baseTC.xy + vDir ).xyz, 1 );
//#if !%_RT_SAMPLE1
//    cCurr *= sqrt( saturate( 1-abs(fWeight) ) );
//#endif
//
//    cFlare += cCurr;


    Vec4 pWeights[3];
    pWeights[0] = Vec4(0.0f, 1.0f, 2.0f, 3.0f) * fRecipSamples * 2.0f - Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    pWeights[1] = Vec4(4.0f, 5.0f, 6.0f, 7.0f) * fRecipSamples * 2.0f - Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    pWeights[2] = Vec4(8.0f, 9.0f, 10.0f, 11.0f) * fRecipSamples * 2.0f - Vec4(1.0f, 1.0f, 1.0f, 1.0f);

    Vec4 pOffsets[5];
    pOffsets[0] = Vec4(pWeights[0].x , 0, pWeights[0].y , 0);
    pOffsets[1] = Vec4(pWeights[0].z , 0, pWeights[0].w , 0);
    pOffsets[2] = Vec4(pWeights[1].x , 0, pWeights[1].y , 0);
    pOffsets[3] = Vec4(pWeights[1].z , 0, pWeights[1].w , 0);
    pOffsets[4] = Vec4(pWeights[2].x , 0, pWeights[2].y , 0);

    // Set final weights
    pWeights[0] = Vec4(sqrtf(max(0.0f, 1.0f - fabs(pWeights[0].x))), sqrtf(max(0.0f, 1.0f - fabs(pWeights[0].y))), sqrtf(max(0.0f, 1.0f - fabs(pWeights[0].z))), sqrtf(max(0.0f, 1.0f - fabs(pWeights[0].w))));
    pWeights[1] = Vec4(sqrtf(max(0.0f, 1.0f - fabs(pWeights[1].x))), sqrtf(max(0.0f, 1.0f - fabs(pWeights[1].y))), sqrtf(max(0.0f, 1.0f - fabs(pWeights[1].z))), sqrtf(max(0.0f, 1.0f - fabs(pWeights[1].w))));
    pWeights[2] = Vec4(sqrtf(max(0.0f, 1.0f - fabs(pWeights[2].x))), sqrtf(max(0.0f, 1.0f - fabs(pWeights[2].y))), sqrtf(max(0.0f, 1.0f - fabs(pWeights[2].z))), sqrtf(max(0.0f, 1.0f - fabs(pWeights[2].w))));

    {
      // "Anamorphic" streak (45 degrees)
      gcpRendD3D->FX_PushRenderTarget(0, tpBlurTemp[0]->m_pTexture, GetUtils().m_pCurDepthSurface); 
      gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]);

      GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   
      gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

      // Set default params    
      Vec4 pParams= Vec4(4.0f * fStreakStretch, 1.0f, 1.0f / (float)pFlareTex->GetWidth(), 1.0f / (float) pFlareTex->GetHeight());     
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

      // Rotate samples 45 degrees
      Vec4 pFinalOffsets[5];
      pFinalOffsets[0] = pOffsets[0] ; // 0.5f => use half range for rotated samples (visual tweak)
      pFinalOffsets[0] += Vec4(-pOffsets[0].y, pOffsets[0].x, -pOffsets[0].w, pOffsets[0].z);
      pFinalOffsets[0] *= 0.5f * pParams.z * pParams.x * 0.707f;
      pFinalOffsets[1] = pOffsets[1]; 
      pFinalOffsets[1] += Vec4(-pOffsets[1].y, pOffsets[1].x, -pOffsets[1].w, pOffsets[1].z);
      pFinalOffsets[1] *= 0.5f * pParams.z * pParams.x* 0.707f;
      pFinalOffsets[2] = pOffsets[2] ; 
      pFinalOffsets[2] += Vec4(-pOffsets[2].y, pOffsets[2].x, -pOffsets[2].w, pOffsets[2].z);
      pFinalOffsets[2] *= 0.5f * pParams.z * pParams.x* 0.707f;
      pFinalOffsets[3] = pOffsets[3] ; 
      pFinalOffsets[3] += Vec4(-pOffsets[3].y, pOffsets[3].x, -pOffsets[3].w, pOffsets[3].z);
      pFinalOffsets[3] *= 0.5f * pParams.z * pParams.x* 0.707f;
      pFinalOffsets[4] = pOffsets[4] ; 
      pFinalOffsets[4] += Vec4(-pOffsets[4].y, pOffsets[4].x, -pOffsets[4].w, pOffsets[4].z);
      pFinalOffsets[4] *= 0.5f * pParams.z * pParams.x* 0.707f;
      
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamOffsets, pFinalOffsets, 5);
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamWeights, pWeights, 3);

      GetUtils().SetTexture(pFlareTex, 0, FILTER_LINEAR, TADDR_BORDER);    
      GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());  

      GetUtils().ShEndPass();   
      gcpRendD3D->FX_PopRenderTarget(0);

      // 2nd step: Begin smoothing steps - smooth out results, this gives us 64 samples
//      gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];

      gcpRendD3D->FX_PushRenderTarget(0, tpBlurTemp[1]->m_pTexture, GetUtils().m_pCurDepthSurface); 

      GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   
      gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

      pParams.x= 1.0f * fStreakStretch;
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

      // Rotate samples 45 degrees
      pFinalOffsets[0] = pOffsets[0] ; // 0.5f => use half range for rotated samples (visual tweak)
      pFinalOffsets[0] += Vec4(-pOffsets[0].y, pOffsets[0].x, -pOffsets[0].w, pOffsets[0].z);
      pFinalOffsets[0] *= 0.5f * pParams.z * pParams.x * 0.707f;
      pFinalOffsets[1] = pOffsets[1]; 
      pFinalOffsets[1] += Vec4(-pOffsets[1].y, pOffsets[1].x, -pOffsets[1].w, pOffsets[1].z);
      pFinalOffsets[1] *= 0.5f * pParams.z * pParams.x* 0.707f;
      pFinalOffsets[2] = pOffsets[2] ; 
      pFinalOffsets[2] += Vec4(-pOffsets[2].y, pOffsets[2].x, -pOffsets[2].w, pOffsets[2].z);
      pFinalOffsets[2] *= 0.5f * pParams.z * pParams.x* 0.707f;
      pFinalOffsets[3] = pOffsets[3] ; 
      pFinalOffsets[3] += Vec4(-pOffsets[3].y, pOffsets[3].x, -pOffsets[3].w, pOffsets[3].z);
      pFinalOffsets[3] *= 0.5f * pParams.z * pParams.x* 0.707f;
      pFinalOffsets[4] = pOffsets[4] ; 
      pFinalOffsets[4] += Vec4(-pOffsets[4].y, pOffsets[4].x, -pOffsets[4].w, pOffsets[4].z);
      pFinalOffsets[4] *= 0.5f * pParams.z * pParams.x* 0.707f;

      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamOffsets, pFinalOffsets, 5);
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamWeights, pWeights, 3);

      GetUtils().SetTexture(tpBlurTemp[0]->m_pTexture, 0, FILTER_LINEAR, TADDR_BORDER);
      GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());  

      GetUtils().ShEndPass();   
      gcpRendD3D->FX_PopRenderTarget(0);
    }

    {
      // "Anamorphic" horizontal streak
      gcpRendD3D->RT_SetViewport(0, 0, pFlareTex->GetWidth(), pFlareTex->GetHeight());
      gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]);
      gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

      // 1st step: use 9 samples, blur along a direction
      gcpRendD3D->FX_PushRenderTarget(0, tpBlurTemp[0]->m_pTexture, GetUtils().m_pCurDepthSurface); 
      GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   
      gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

      // Set default params    
      Vec4 pParams= Vec4(4.0f * fStreakStretch, 1.0f, 1.0f / (float)pFlareTex->GetWidth(), 1.0f / (float) pFlareTex->GetHeight());     
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);
      
      Vec4 pFinalOffsets[5];
      pFinalOffsets[0] = pOffsets[0] * pParams.z * pParams.x;
      pFinalOffsets[1] = pOffsets[1] * pParams.z * pParams.x;
      pFinalOffsets[2] = pOffsets[2] * pParams.z * pParams.x;
      pFinalOffsets[3] = pOffsets[3] * pParams.z * pParams.x;
      pFinalOffsets[4] = pOffsets[4] * pParams.z * pParams.x;
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamOffsets, pFinalOffsets, 5);
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamWeights, pWeights, 3);

      GetUtils().SetTexture(pFlareTex, 0, FILTER_LINEAR, TADDR_BORDER);    
      GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());  
      
      GetUtils().ShEndPass();   
      gcpRendD3D->FX_PopRenderTarget(0); 

      // 2nd step: Begin smoothing steps - smooth out results, this gives us 64 samples
      gcpRendD3D->FX_PushRenderTarget(0, pFlareTex, GetUtils().m_pCurDepthSurface); 
      //gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];

      GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);    

      gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

      pParams.x = 1.0f * fStreakStretch; // set smooth scale
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);
      pFinalOffsets[0] = pOffsets[0] * pParams.z * pParams.x;
      pFinalOffsets[1] = pOffsets[1] * pParams.z * pParams.x;
      pFinalOffsets[2] = pOffsets[2] * pParams.z * pParams.x;
      pFinalOffsets[3] = pOffsets[3] * pParams.z * pParams.x;
      pFinalOffsets[4] = pOffsets[4] * pParams.z * pParams.x;
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamOffsets, pFinalOffsets, 5);
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamWeights, pWeights, 3);

      GetUtils().SetTexture(tpBlurTemp[0]->m_pTexture, 0, FILTER_LINEAR, TADDR_BORDER);    
      GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());  
      
      GetUtils().ShEndPass();   
      gcpRendD3D->FX_PopRenderTarget(0); 

      // 3rd step: further smooth out results, this gives us 512 samples - useful for huge step blurs
      gcpRendD3D->FX_PushRenderTarget(0, tpBlurTemp[0]->m_pTexture, GetUtils().m_pCurDepthSurface); 
      GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   
      gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

      pParams.x= 0.75f * fStreakStretch; // intermediate smooth scale
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);
      pFinalOffsets[0] = pOffsets[0] * pParams.z * pParams.x;
      pFinalOffsets[1] = pOffsets[1] * pParams.z * pParams.x;
      pFinalOffsets[2] = pOffsets[2] * pParams.z * pParams.x;
      pFinalOffsets[3] = pOffsets[3] * pParams.z * pParams.x;
      pFinalOffsets[4] = pOffsets[4] * pParams.z * pParams.x;
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamOffsets, pFinalOffsets, 5);
      CShaderMan::m_shPostEffects->FXSetPSFloat(pParamWeights, pWeights, 3);

      GetUtils().SetTexture(pFlareTex, 0, FILTER_LINEAR, TADDR_BORDER);    
      GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());  

      GetUtils().ShEndPass();   
      gcpRendD3D->FX_PopRenderTarget(0); 
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Add results into main glow

    gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexGlow, GetUtils().m_pCurDepthSurface); 
    gcpRendD3D->RT_SetViewport( 0, 0, CTexture::s_ptexGlow->GetWidth(), CTexture::s_ptexGlow->GetHeight() );

    CCryName pTechName1("MergeAnamorphicBlurWithGlow");
    GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName1, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    Vec4 pParams = m_pStreaksColor->GetParamVec4() * m_pStreaksMul->GetParam();
    CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);
    GetUtils().SetTexture(tpBlurTemp[0]->m_pTexture, 0, FILTER_LINEAR);
    GetUtils().SetTexture(tpBlurTemp[1]->m_pTexture, 1, FILTER_LINEAR);




    gcpRendD3D->EF_SetState(GS_NODEPTHTEST|GS_BLSRC_ONE|GS_BLDST_ONE);


    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

    GetUtils().ShEndPass(); 

    gcpRendD3D->FX_PopRenderTarget(0); 

    SAFE_DELETE(tpBlurTemp[0]);
    SAFE_DELETE(tpBlurTemp[1]);
    PROFILE_LABEL_POP( "ANAMORPHIC_FLARES" );
  }

  CTexture *pGlowTmp = CTexture::s_ptexGlow;
  CTexture *pMergedGlow = CTexture::s_ptexBackBufferScaled[0];

  //Main scene glow
  //Desc: using 3 textures (2, 4, 8 times smaller), first texture is slightly blurred (to keep some detail at distance),
  //the second/third texture are quite blurred so that we get a nice/smooth looking halo around main glow
  GetUtils().TexBlurGaussian(pGlowTmp, 1, 1.25f, 1.5f, false);                  
  GetUtils().StretchRect(pGlowTmp, CTexture::s_ptexBackBufferScaled[1]);      
  GetUtils().TexBlurGaussian(CTexture::s_ptexBackBufferScaled[1], 1, 1.25f, 5.0f, false);           
  GetUtils().StretchRect(CTexture::s_ptexBackBufferScaled[1], CTexture::s_ptexBackBufferScaled[2]);                 
  GetUtils().TexBlurGaussian(CTexture::s_ptexBackBufferScaled[2], 1, 1.25f, 5.0f, false);         
	//GetUtils().TexBlur4Taps(pGlowTmp, 1.0f);                  

  // Blend out lens-flares when no glow surfaces on screen (blend to avoid popping)
  const float fMinFlaresBlendThreshold = 0.025f;
  static float fFlaresBlend = 1.0f;
  fFlaresBlend += (m_pActive->GetParam() - fFlaresBlend) * gEnv->pTimer->GetFrameTime() * 4.0f;

  gcpRendD3D->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];
  if( CRenderer::CV_r_glowanamorphicflares && m_pFlaresMul->GetParam() > 0.01f && CRenderer::CV_r_glow == 1 && fFlaresBlend > fMinFlaresBlendThreshold)
    gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

  //////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Merge all glow steps (at main glow rt resolution - not fullscreen) + add lens flares if enabled

  gcpRendD3D->FX_PushRenderTarget(0, pMergedGlow, GetUtils().m_pCurDepthSurface); 
  gcpRendD3D->RT_SetViewport( 0, 0, pMergedGlow->GetWidth(), pMergedGlow->GetHeight() );

  static CCryName pTechNameMerge("GlowMerge");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechNameMerge, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   
  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

  // Set default params    
  Vec4 pParams= Vec4(1.0f, 1.0f, fFlaresBlend * m_pFlaresMul->GetParam(), fGlowScale);     

  if( CRenderer::CV_r_glowanamorphicflares )
    pParams.w *= 2.0f; // If streaks enabled - make glow stronger

  static CCryName pParamName("glowParamsPS");
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

  GetUtils().SetTexture(pGlowTmp, 0);    
  GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[1], 1);    
  GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[2], 2);    

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());  

  GetUtils().ShEndPass();   

  gcpRendD3D->FX_PopRenderTarget(0); 

  //////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////

  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

  static CCryName pTechName("GlowDisplay");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   
  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

  CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);    
  GetUtils().SetTexture(pMergedGlow, 1);     

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());  

  GetUtils().ShEndPass();   

  // Disable glow post processing again
  m_pActive->SetParam(0.0f);  
  
  gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;

  gcpRendD3D->FX_Flush();
  PROFILE_SHADER_END    

  PROFILE_LABEL_POP( "GLOW" );
} 
