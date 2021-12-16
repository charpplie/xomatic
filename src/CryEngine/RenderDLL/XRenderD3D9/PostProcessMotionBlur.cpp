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

bool CD3D9Renderer::FX_MotionBlurScene(bool bEnable)
{
  bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality( eRQ_VeryHigh, eSQ_High );
  if( !bQualityCheck || !CRenderer::CV_r_HDRRendering )
    return false;

  static uint64 nPrevFlagsShader_RT = 0;
  if( CRenderer::s_MotionBlurMode < 3 || (iSystem->IsEditorMode() && CRenderer::s_MotionBlurMode !=5 ))
    return false;

  CTexture *pSceneTarget = CTexture::s_ptexSceneTarget; //(CRenderer::CV_r_debug_extra_scenetarget_fsaa && gcpRendD3D->m_RP.m_FSAAData.Type)?CTexture::m_Text_SceneTargetFSAA : CTexture::s_ptexSceneTarget;    
  SD3DSurface *pCurrDepthBuffer = GetUtils().GetDepthSurface( pSceneTarget );

  if(bEnable)
  {




    nPrevFlagsShader_RT = m_RP.m_FlagsShader_RT;

    GetUtils().Log(" +++ Begin object motion blur scene +++ \n"); 

    FX_PushRenderTarget(0, pSceneTarget, pCurrDepthBuffer);

    // Re-use scene target rendertarget for velocity buffer

    RT_SetViewport(0, 0, CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight());                        
    ColorF clearColor(0, 0, 0, 0);
    EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clearColor);

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_MOTIONBLURPASS;
  }
  else  
  {     
    FX_PopRenderTarget(0);    

    FX_ResetPipe();     
    gcpRendD3D->RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());

    // compute motion blur
    gcpRendD3D->Set2DMode(true, 1, 1);            

    GetUtils().m_pCurDepthSurface = &gcpRendD3D->m_DepthBufferOrig;

    uint64 nFlagsShaderRT = m_RP.m_FlagsShader_RT;
    m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);

    CTexture *pOffsetMap = CTexture::s_ptexBackBufferScaled[1];
    CTexture *pDst = CTexture::s_ptexBackBufferScaled[0];
    CTexture *pDstTemp = CTexture::s_ptexHDRTargetScaled[0]; 

    // Velocity buffer rescaling
    {
      static CCryName pTechName0("OMB_VelocityIDRescale");

      // velocity dilation: iteration 1
      gcpRendD3D->FX_PushRenderTarget(0,  pDst,  GetUtils().GetDepthSurface( pDst )) ;
      gRenDev->RT_SetViewport(0, 0, pDst->GetWidth(), pDst->GetHeight());        

      GetUtils().ShBeginPass(CShaderMan::m_shPostMotionBlur, pTechName0, FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);
      gcpRendD3D->EF_SetState(GS_NODEPTHTEST);    

      CCryName pParamName("PI_motionBlurParams");
      Vec4 vParams=Vec4(0,0, 1.0f / pSceneTarget->GetWidth(),  1.0f / pSceneTarget->GetHeight()) ;       
      CShaderMan::m_shPostMotionBlur->FXSetPSFloat(pParamName, &vParams, 1);

      GetUtils().SetTexture(pSceneTarget, 0, FILTER_POINT);  

      GetUtils().DrawFullScreenQuad(pDst->GetWidth(), pDst->GetHeight());      
      GetUtils().ShEndPass();

      gcpRendD3D->FX_PopRenderTarget(0);
    }

    // Generate offset map
    {
      static CCryName pTechName0("OMB_OffsetMap");

      // velocity dilation: iteration 1
      gcpRendD3D->FX_PushRenderTarget(0,  pOffsetMap,  GetUtils().GetDepthSurface( pOffsetMap )) ;
      gcpRendD3D->RT_SetViewport(0, 0, pOffsetMap->GetWidth(), pOffsetMap->GetHeight());        

      GetUtils().ShBeginPass(CShaderMan::m_shPostMotionBlur, pTechName0, FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);
      gcpRendD3D->EF_SetState(GS_NODEPTHTEST);    

      GetUtils().SetTexture(pDst, 0, FILTER_POINT);  

      GetUtils().DrawFullScreenQuad(pOffsetMap->GetWidth(), pOffsetMap->GetHeight());      
      GetUtils().ShEndPass();

      gcpRendD3D->FX_PopRenderTarget(0);

      GetUtils().TexBlurGaussian(pOffsetMap, 2, 1.0f, 5.0f, false);                  
      GetUtils().TexBlurGaussian(pOffsetMap, 2, 1.0f, 5.0f, false);                  
    }

    // Velocity buffer dilation
    {
      static CCryName pTechName0("OMB_VelocityDilation");
      CCryName pTechName("OMB_VelocityDilation");

      static CCryName pParamName("PI_motionBlurParams");
      Vec4 vParams=Vec4(1.0f/pSceneTarget->GetWidth(), 1.0f/pSceneTarget->GetHeight(), 1.0f / pDst->GetWidth(),  1.0f / pDst->GetHeight()) ;       

      int nIterationCount = 2;

      for(int n= 0; n < nIterationCount; ++n)
      {
        // enable horizontal pass
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];

        // velocity dilation: iteration 1n
        gcpRendD3D->FX_PushRenderTarget(0,  pDstTemp,  GetUtils().GetDepthSurface( pDst )) ;
        gcpRendD3D->RT_SetViewport(0, 0, pDstTemp->GetWidth(), pDstTemp->GetHeight());        

        GetUtils().ShBeginPass(CShaderMan::m_shPostMotionBlur, pTechName0, FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);
        gcpRendD3D->EF_SetState(GS_NODEPTHTEST);    

        CShaderMan::m_shPostMotionBlur->FXSetPSFloat(pParamName, &vParams, 1);

        GetUtils().SetTexture(pDst, 0, FILTER_POINT);      
        GetUtils().SetTexture(CTexture::s_ptexZTarget, 1, FILTER_POINT);
        GetUtils().SetTexture(pOffsetMap, 2, FILTER_LINEAR);  
        GetUtils().SetTexture(pSceneTarget, 3, FILTER_POINT);  


        GetUtils().DrawFullScreenQuad(pDstTemp->GetWidth(), pDstTemp->GetHeight());      
        GetUtils().ShEndPass();

        gcpRendD3D->FX_PopRenderTarget(0);

        // enable vertical pass
        m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE1];

        // velocity dilation: iteration 2n
        gcpRendD3D->FX_PushRenderTarget(0,  pDst,  GetUtils().GetDepthSurface( pDst )) ;
        gcpRendD3D->RT_SetViewport(0, 0, pDst->GetWidth(), pDst->GetHeight());        

        GetUtils().ShBeginPass(CShaderMan::m_shPostMotionBlur, pTechName0, FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);
        gcpRendD3D->EF_SetState(GS_NODEPTHTEST);    

        CShaderMan::m_shPostMotionBlur->FXSetPSFloat(pParamName, &vParams, 1);
        GetUtils().SetTexture(pDstTemp, 0, FILTER_POINT);      
        GetUtils().SetTexture(CTexture::s_ptexZTarget, 1, FILTER_POINT);
        GetUtils().SetTexture(pOffsetMap, 2, FILTER_POINT);  
        GetUtils().SetTexture(pSceneTarget, 3, FILTER_POINT);  

        GetUtils().DrawFullScreenQuad(pDst->GetWidth(), pDst->GetHeight());      
        GetUtils().ShEndPass();

        gcpRendD3D->FX_PopRenderTarget(0);
      }

      gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight());        
    }







    // Put objects ID into alpha channel
    {
      CCryName pTechName0("OMB_CopyAlphaID");

#ifndef XENON
      // velocity dilation: iteration 1
      gcpRendD3D->FX_PushRenderTarget(0,  CTexture::s_ptexHDRTarget,  GetUtils().GetDepthSurface( CTexture::s_ptexHDRTarget )) ;
      gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight());        
#endif

      GetUtils().ShBeginPass(CShaderMan::m_shPostMotionBlur, pTechName0, FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);

      // only write to alpha
      gcpRendD3D->EF_SetState(GS_NODEPTHTEST|GS_COLMASK_A);     

      //GetUtils().SetTexture(pDst, 0, FILTER_LINEAR);  
      GetUtils().SetTexture(pSceneTarget, 0, FILTER_POINT);  

      GetUtils().DrawFullScreenQuad(CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight());      
      GetUtils().ShEndPass();

#ifndef XENON
      gcpRendD3D->FX_PopRenderTarget(0);
#endif

      gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight());        
    }





    gcpRendD3D->FX_PushRenderTarget(0,  CTexture::s_ptexSceneTarget,  GetUtils().GetDepthSurface( CTexture::s_ptexSceneTarget )) ; //&gcpRendD3D->m_DepthBufferOrig); 
    gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight());        


    // Render motion blurred scene
    static CCryName pTechName0("OMB_UsingVelocityDilation");

    m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE1];

    GetUtils().ShBeginPass(CShaderMan::m_shPostMotionBlur, pTechName0, FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);
    gcpRendD3D->EF_SetState(GS_NODEPTHTEST);    

    static CCryName pParamName("motionBlurParams");
    CCryName pParamName2("PI_motionBlurParams");






    GetUtils().SetTexture(CTexture::s_ptexHDRTarget, 0, FILTER_LINEAR);       
    GetUtils().SetTexture(pDstTemp, 1, FILTER_LINEAR); 
    GetUtils().SetTexture(pOffsetMap, 2, FILTER_LINEAR); 


    Vec4 vParams=Vec4(1, 1, 1,  1) ;       
    vParams.x =0.5f;  
    CShaderMan::m_shPostMotionBlur->FXSetPSFloat(pParamName2, &vParams, 1);

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());      
    GetUtils().ShEndPass();

#ifndef XENON
    // Restore previous viewport/rt
    gcpRendD3D->FX_PopRenderTarget(0); 
    gcpRendD3D->RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());        

    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];

    GetUtils().ShBeginPass(CShaderMan::m_shPostMotionBlur, pTechName0, FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);
    gcpRendD3D->EF_SetState(GS_NODEPTHTEST);    

    GetUtils().SetTexture(CTexture::s_ptexSceneTarget, 0, FILTER_LINEAR);       
    GetUtils().SetTexture(pDstTemp, 1, FILTER_LINEAR); 
    GetUtils().SetTexture(pOffsetMap, 2, FILTER_LINEAR);  


    //vParams.x = 0.25;  
    vParams.x = 1.0; 
    CShaderMan::m_shPostMotionBlur->FXSetPSFloat(pParamName2, &vParams, 1);

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());      
    GetUtils().ShEndPass();
#endif

    m_RP.m_FlagsShader_RT =  nFlagsShaderRT;
    gcpRendD3D->Set2DMode(false, 1, 1);        

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_MOTIONBLURPASS;
    m_RP.m_FlagsShader_RT = nPrevFlagsShader_RT;

    GetUtils().Log(" +++ End object motion blur scene +++ \n"); 
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CMotionBlur::Preprocess()
{
	const int nThreadID = gRenDev->m_RP.m_nProcessThreadID;

  bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality( eRQ_Medium, eSQ_Medium );
  if( !bQualityCheck )
    return false;
  bool bActive = (CRenderer::s_MotionBlurMode==1 || !CRenderer::CV_r_HDRRendering && (CRenderer::s_MotionBlurMode==3 || CRenderer::s_MotionBlurMode==4)) && !iSystem->IsEditorMode() || CRenderer::s_MotionBlurMode== 5;





	Matrix44 &pPrevView = m_pPrevView[ min(1, SRendItem::m_RenderView[nThreadID]) ];

  if( !bActive )
    pPrevView = GetUtils().m_pView;

  m_fRotationAcc *= max(1.f - CRenderer::CV_r_motionblurdynamicquality_rotationacc_stiffness * gEnv->pTimer->GetFrameTime(), 0.f);    
  if (CTexture::s_ptexSceneTarget == NULL || !bActive)
    return false;

  Matrix44 pCurrView( GetUtils().m_pView  ), pCurrProj( GetUtils().m_pProj );  
  
  m_nQualityInfo = 0;
  m_nSamplesInfo = 0;
  m_nRotSamplesEst = m_fRotationAcc;
  m_nTransSamplesEst = 0;

  const CCamera &currentCamera = gcpRendD3D->GetCamera();

  // If it's a new camera, disable motion blur for the first frame
  if (currentCamera.IsJustActivated())
  {
    pPrevView = pCurrView;
    return false;
  }

  // No movement, skip motion blur - why no IsEquivalent for 4x4...
  Vec3 pCurr0 = pCurrView.GetRow(0), pCurr1 = pCurrView.GetRow(1), pCurr2 = pCurrView.GetRow(2), pCurr3 = pCurrView.GetRow(3);
  Vec3 pPrev0 = pPrevView.GetRow(0), pPrev1 = pPrevView.GetRow(1), pPrev2 = pPrevView.GetRow(2), pPrev3 = pPrevView.GetRow(3);
  if( pCurr0.IsEquivalent( pPrev0, 0.025f) && pCurr1.IsEquivalent( pPrev1, 0.025f) && pCurr2.IsEquivalent( pPrev2, 0.025f) && pCurr3.IsEquivalent( pPrev3, 0.3f) ) 
  {
    pPrevView = pCurrView;
    return false;
  }

  return bActive;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CMotionBlur::MotionDetection( float fCurrFrameTime, const Matrix44 &pCurrView, int &nQuality)
{
	const int nThreadID = gRenDev->m_RP.m_nProcessThreadID;
  float fExposureTime = 0.2f * CRenderer::CV_r_motionblur_shutterspeed;
  float fRotationDistToPrev = 1.0f;
  float fCurrDistToPrevPos = 1.0f;
  const float fMinPosDiferenceThreshold = 0.02f * CRenderer::CV_r_motionblurdynamicquality_translationthreshold;
  const float fMedPosDiferenceThreshold = 0.04f * CRenderer::CV_r_motionblurdynamicquality_translationthreshold;
  const float fMaxPosDiferenceThreshold = 0.08f * CRenderer::CV_r_motionblurdynamicquality_translationthreshold;

	Matrix44 &pPrevView = m_pPrevView[ min(1, SRendItem::m_RenderView[nThreadID]) ];

  //static float fRotationAcc = 0.0f;
  Matrix44 pLView = pPrevView;
  if( CRenderer::CV_r_motionblurdynamicquality  )
  {
    nQuality = 0;

    float fAlpha = iszero(fCurrFrameTime) ? 0.0f : fExposureTime /fCurrFrameTime;
    Matrix33 pLerpedView = Matrix33(pCurrView)*(1-fAlpha) + Matrix33(pPrevView)*fAlpha;   
    Vec3 pLerpedPos = pCurrView.GetRow(3)*(1-fAlpha) + pPrevView.GetRow(3) * fAlpha;     

    // Compose final 'previous' viewProjection matrix
    pLView = pLerpedView;
    pLView.m30 = pLerpedPos.x;   
    pLView.m31 = pLerpedPos.y;
    pLView.m32 = pLerpedPos.z; 

    // Set motion blur quality/passes based on amount of camera movement
    Vec3 pCurrPos = pCurrView.GetInverted().GetRow(3);
    Vec3 pPrevPos = pLView.GetInverted().GetRow(3);
    fCurrDistToPrevPos = (pCurrPos - pPrevPos).len();    
    if( fCurrDistToPrevPos > fMaxPosDiferenceThreshold )
      nQuality = 2;
    else
      if( fCurrDistToPrevPos > fMedPosDiferenceThreshold )
        nQuality = 1;

    // ???? - use just camera view front vector and dot product
    Vec3 frontPrev = pCurrView.TransformVector( Vec3(0,0,1) );
    frontPrev.Normalize();
    Vec3 frontCurr = pLView.TransformVector( Vec3(0,0,1) );
    frontCurr.Normalize();

    m_fRotationAcc += fabs(frontCurr.dot(frontPrev));//.len();
    fRotationDistToPrev = m_fRotationAcc;

    const float fMedDotDiferenceThreshold = 0.045f* CRenderer::CV_r_motionblurdynamicquality_rotationthreshold; 
    const float fMaxDotDiferenceThreshold = 0.1f* CRenderer::CV_r_motionblurdynamicquality_rotationthreshold;
    if( m_fRotationAcc > fMaxDotDiferenceThreshold )
      nQuality = max( nQuality , 2);
    else
      if( m_fRotationAcc > fMedDotDiferenceThreshold ) 
        nQuality = max( nQuality , 1); 
  }

  // No movement, skip motion blur
  Vec3 pCurr0 = pCurrView.GetRow(0), pCurr1 = pCurrView.GetRow(1), pCurr2 = pCurrView.GetRow(2), pCurr3 = pCurrView.GetRow(3);
  Vec3 pPrev0 = pLView.GetRow(0), pPrev1 = pLView.GetRow(1), pPrev2 = pLView.GetRow(2), pPrev3 = pLView.GetRow(3);
  bool isEquivalentMatrix = pCurr0.IsEquivalent( pPrev0, 0.025f) && pCurr1.IsEquivalent( pPrev1, 0.025f) && pCurr2.IsEquivalent( pPrev2, 0.025f) && pCurr3.IsEquivalent( pPrev3, 0.3f);
  if( CRenderer::CV_r_motionblurdynamicquality && (isEquivalentMatrix && m_fRotationAcc < 10.0f)  || isEquivalentMatrix ) 
  {
    //OutputDebugString("not enough motion \n"); 
    return false; 
  }

  gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);
  if( CRenderer::CV_r_motionblurdynamicquality )
  {
    float fsamples_pos = ceilf(clamp_tpl<float>(fCurrDistToPrevPos * 10.0f, 0.0f, 1.0f) *8.0f);
    float fsamples_rot = clamp_tpl<float>(fRotationDistToPrev * 20.0f, 0.0f, 1.0f) *8.0f;

    m_nRotSamplesEst = fsamples_rot;
    m_nTransSamplesEst = fsamples_pos;

    // not enough samples count, skip motion blur
    if( fsamples_pos <= 1.0f && fsamples_rot <= 1.0f)
      return false;
    
    // slow movement - use 4 samples only
    m_nSamplesInfo = 8;
    if( fsamples_pos < 4.0f && fsamples_rot < 1.0f )
    {
      m_nSamplesInfo = 4;
      gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CMotionBlur::Render()
{
	const int nThreadID = gRenDev->m_RP.m_nProcessThreadID;
  // lower specs camera motion blur
  if ( CTexture::s_ptexSceneTarget == NULL)
    return;

  PROFILE_LABEL_PUSH( "MOTION BLUR" );

	Matrix44 &pPrevView = m_pPrevView[ min(1, SRendItem::m_RenderView[nThreadID]) ];
  Matrix44A pCurrView( GetUtils().m_pView  ), pCurrProj( GetUtils().m_pProj );  
  int nShaderQuality = gcpRendD3D->EF_GetShaderQuality(eST_PostProcess); 
  uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;

  PROFILE_SHADER_START

    Matrix33 pLerpedView;
  pLerpedView.SetIdentity(); 

  float fCurrFrameTime = gEnv->pTimer->GetFrameTime();
  // renormalize frametime to account for time scaling
  float fTimeScale = gEnv->pTimer->GetTimeScale();
  if (fTimeScale < 1.0f)
  {
    fTimeScale = max(0.0001f, fTimeScale);
    fCurrFrameTime /= fTimeScale;
  }

  // scale down shutter speed a bit. default shutter is 0.02, so final result is 0.004 (old default value)
  float fExposureTime = 0.2f * CRenderer::CV_r_motionblur_shutterspeed;

  float fAlpha = iszero(fCurrFrameTime) ? 0.0f : fExposureTime /fCurrFrameTime;
  if( CRenderer::CV_r_motionblurframetimescale )
  {
    float fAlphaScale = iszero(fCurrFrameTime) ? 1.0f : min(1.0f, (1.0f / fCurrFrameTime) / ( 32.0f)); // attenuate motion blur for lower frame rates
    fAlpha *= fAlphaScale;
  }

  // Interpolate matrixes and position
  pLerpedView = Matrix33(pCurrView)*(1-fAlpha) + Matrix33(pPrevView)*fAlpha;   

  Vec3 pLerpedPos = Vec3::CreateLerp(pCurrView.GetRow(3), pPrevView.GetRow(3), fAlpha);     

  // Compose final 'previous' viewProjection matrix
  Matrix44A pLView = pLerpedView;
  pLView.m30 = pLerpedPos.x;   
  pLView.m31 = pLerpedPos.y;
  pLView.m32 = pLerpedPos.z; 

  int nQuality = 1;
  if( !MotionDetection( fCurrFrameTime, pCurrView, nQuality) )
  {
    PROFILE_LABEL_POP( "MOTION BLUR" );
		pPrevView = GetUtils().m_pView;
    return;
  }

  
  if( nShaderQuality < eSQ_High )
    nQuality = 0;

  nQuality = min(nQuality, 1);






  float fFocusRange = m_pFocusRange->GetParam();

  // First generate motion blur mask 
  if( nShaderQuality == eSQ_High )
  {
    Vec3 pCurrDir = pCurrView.GetColumn(2);
    Vec3 pPrevDir = pPrevView.GetColumn(2);

    CCryName pTechName("MotionBlurMaskGen");
    GetUtils().ShBeginPass(CShaderMan::m_shPostMotionBlur, pTechName, FEF_DONTSETSTATES);
    gcpRendD3D->EF_SetState(GS_NODEPTHTEST|GS_COLMASK_A);    

    float fRotationAmount = 1.0f - fabs( pCurrDir.Dot(pPrevDir) );
    Vec4 vParams=Vec4(1, 1, fFocusRange,  fRotationAmount) ;       
    static CCryName pParamName("motionBlurParams");
    CShaderMan::m_shPostMotionBlur->FXSetPSFloat(pParamName, &vParams, 1);

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());      
    GetUtils().ShEndPass();
  }

  // update backbuffer
  GetUtils().CopyScreenToTexture(CTexture::s_ptexBackBuffer); 

  Matrix44A pViewProjPrev = pLView * pCurrProj;      
  pViewProjPrev.Transpose();

  CVertexBuffer vertexBuffer(&m_pCamSphereVerts[0],eVF_P3F_C4B_T2F);

  float fVectorsScale = m_pVectorsScale->GetParam(); 
  float fCamScale = m_pCameraSphereScale->GetParam();

  Vec4 vCamParams=Vec4(1, 1, 1, fCamScale ) ;
  float fCurrVectorsScale = fVectorsScale;
  Vec4 vParams = Vec4(0, 0, fFocusRange, 0) ;       

  Vec4 cBlurVec = PostEffectMgr().GetByNameVec4("Global_DirectionalBlur_Vec") * 0.01f;    
  fCurrVectorsScale = 1.5f;

  static CCryName pTechName("MotionBlurDispl");
  static CCryName pParam0Name("mViewProjPrev");
  static CCryName pParam1Name("motionBlurCamParams");
  static CCryName pParam3Name("vDirectionalBlur");
  static CCryName pParam4Name("PI_motionBlurParams");
  static CCryName pParam2Name("motionBlurParams");

  for( int nPasses = 0; nPasses <= nQuality; ++nPasses )  
  {	
    vParams.w = fCurrVectorsScale;       
    gcpRendD3D->Set2DMode(false, 1, 1);         

    if( !nPasses )
      gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    else
      gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];

    GetUtils().ShBeginPass(CShaderMan::m_shPostMotionBlur, pTechName, FEF_DONTSETSTATES);

    // use depth test to cull nearby fragments (weapons)
    gcpRendD3D->EF_SetState(GS_DEPTHFUNC_LEQUAL);    
    gcpRendD3D->SetCullMode( R_CULL_NONE );

    CShaderMan::m_shPostMotionBlur->FXSetVSFloat(pParam0Name, (Vec4 *)pViewProjPrev.GetData(), 4);    
    CShaderMan::m_shPostMotionBlur->FXSetVSFloat(pParam1Name, &vCamParams, 1);
    CShaderMan::m_shPostMotionBlur->FXSetPSFloat(pParam3Name, &cBlurVec, 1);
    CShaderMan::m_shPostMotionBlur->FXSetPSFloat(pParam4Name, &vParams, 1);

    gRenDev->DrawPrimitives(&vertexBuffer,m_pCamSphereVerts.Count());

    fCurrVectorsScale *= 0.5f;

    GetUtils().ShEndPass();
    gcpRendD3D->Set2DMode(true, 1, 1);         

    if( nPasses != nQuality )
      GetUtils().CopyScreenToTexture(CTexture::s_ptexBackBuffer); 
  }

  m_nQualityInfo = nQuality;


  // store previous frame data
  pPrevView = GetUtils().m_pView;// pCurrView;
  gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;

  gcpRendD3D->FX_Flush();
  PROFILE_SHADER_END     

    PROFILE_LABEL_POP( "MOTION BLUR" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CMotionBlur::RenderHDR()
{
	const int nThreadID = gRenDev->m_RP.m_nProcessThreadID;

  // hdr camera motion blur
  if( CRenderer::s_MotionBlurMode < 3 || (iSystem->IsEditorMode() && CRenderer::s_MotionBlurMode !=5))
    return;

	Matrix44 &pPrevView = m_pPrevView[ min(1, SRendItem::m_RenderView[nThreadID]) ];

  m_fRotationAcc *= max(1.f - CRenderer::CV_r_motionblurdynamicquality_rotationacc_stiffness  * gEnv->pTimer->GetFrameTime(), 0.f);    

  m_nQualityInfo = 0;
  m_nSamplesInfo = 0;
  m_nRotSamplesEst = m_fRotationAcc;
  m_nTransSamplesEst = 0;

  gcpRendD3D->Set2DMode(false, 1, 1);   
  gcpRendD3D->GetModelViewMatrix( GetUtils().m_pView.GetData() );
  gcpRendD3D->GetProjectionMatrix( GetUtils().m_pProj.GetData() );
  gcpRendD3D->Set2DMode(true, 1, 1);   

  // Store some commonly used per-frame data
  SPostEffectsUtils::m_pViewProj = GetUtils().m_pView * GetUtils().m_pProj;
  SPostEffectsUtils::m_pViewProj.Transpose();

  Matrix44 pCurrView( GetUtils().m_pView  ), pCurrProj( GetUtils().m_pProj );  

  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
  // Process camera motion blur
  int nShaderQuality = gcpRendD3D->EF_GetShaderQuality(eST_PostProcess); 

  PROFILE_SHADER_START

    // First generate motion blur mask 
    Vec3 pCurrDir = pCurrView.GetColumn(2);
  Vec3 pPrevDir = pPrevView.GetColumn(2);

  float fCurrFrameTime = gEnv->pTimer->GetFrameTime();
  // renormalize frametime to account for time scaling
  float fTimeScale = gEnv->pTimer->GetTimeScale();
  if (fTimeScale < 1.0f)
  {
    fTimeScale = max(0.0001f, fTimeScale);
    fCurrFrameTime /= fTimeScale;
  }

  // scale down shutter speed a b it. default shutter is 0.02, so final result is 0.004 (old default value)
  float fExposureTime = 0.2f * CRenderer::CV_r_motionblur_shutterspeed;
  float fAlpha = iszero(fCurrFrameTime) ? 0.0f : fExposureTime /fCurrFrameTime;
  if( CRenderer::CV_r_motionblurframetimescale )
  {
    float fAlphaScale = iszero(fCurrFrameTime) ? 1.0f : min(1.0f, (1.0f / fCurrFrameTime) / ( 32.0f)); // attenuate motion blur for lower frame rates
    fAlpha *= fAlphaScale;
  }

  // Interpolate matrixes and position
  Matrix33 pLerpedView = Matrix33(pCurrView)*(1-fAlpha) + Matrix33(pPrevView)*fAlpha;   
  Vec3 pLerpedPos = pCurrView.GetRow(3)*(1-fAlpha) + pPrevView.GetRow(3) * fAlpha;     

  // Compose final 'previous' viewProjection matrix
  Matrix44 pLView = pLerpedView;
  pLView.m30 = pLerpedPos.x;   
  pLView.m31 = pLerpedPos.y;
  pLView.m32 = pLerpedPos.z; 

  int nQuality = 2;
  if( !MotionDetection( fCurrFrameTime, pCurrView, nQuality) )
  {
    pPrevView = GetUtils().m_pView;
    return;
  }

  float fFocusRange = m_pFocusRange->GetParam();

  // Generate motion blur depth mask
  {
    gcpRendD3D->FX_PushRenderTarget(0,  CTexture::s_ptexSceneTarget,  GetUtils().GetDepthSurface( CTexture::s_ptexSceneTarget )) ; //&gcpRendD3D->m_DepthBufferOrig); 
    gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight());        

    CCryName pTechName("MotionBlurMaskGenHDR");
    GetUtils().ShBeginPass(CShaderMan::m_shPostMotionBlur, pTechName, FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);
    gcpRendD3D->EF_SetState(GS_NODEPTHTEST);    

    float fRotationAmount = 1.0f - fabs( pCurrDir.Dot(pPrevDir) );
    Vec4 vParams=Vec4(1, 1, fFocusRange,  fRotationAmount) ;       
    static CCryName pParamName("motionBlurParams");
    CShaderMan::m_shPostMotionBlur->FXSetPSFloat(pParamName, &vParams, 1);

    GetUtils().SetTexture(CTexture::s_ptexHDRTarget, 0, FILTER_POINT);  
    GetUtils().SetTexture(CTexture::s_ptexZTarget, 1, FILTER_POINT);  

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());      
    GetUtils().ShEndPass();

    // Restore previous viewport
    gcpRendD3D->FX_PopRenderTarget(0);
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);     
  }

  Matrix44 pViewProjPrev = pLView * pCurrProj;      
  pViewProjPrev.Transpose();

  CVertexBuffer vertexBuffer(&m_pCamSphereVerts[0],eVF_P3F_C4B_T2F);

  float fCamScale = m_pCameraSphereScale->GetParam();
  float fCurrVectorsScale = 1.5f;
  Vec4 vParams = Vec4(0, 0, fFocusRange, 0) ;       

  static CCryName pParam0Name("mViewProjPrev");
  static CCryName pParam1Name("motionBlurCamParams");
  static CCryName pParam3Name("vDirectionalBlur");
  static CCryName pParam4Name("PI_motionBlurParams");

  for( int nPasses = 0; nPasses <= nQuality; ++nPasses )  
  {	
    gcpRendD3D->Set2DMode(false, 1, 1);         
    static CCryName pTechName("MotionBlurDisplHDR");

    if( !nPasses )
      gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    else
      gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];

    GetUtils().ShBeginPass(CShaderMan::m_shPostMotionBlur, pTechName, FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);

    //gcpRendD3D->EF_SetState(GS_NODEPTHTEST);    
    // use depth test to cull nearby fragments (weapons)
    gcpRendD3D->EF_SetState(GS_DEPTHFUNC_LEQUAL);    
    gcpRendD3D->SetCullMode( R_CULL_NONE );

    CShaderMan::m_shPostMotionBlur->FXSetVSFloat(pParam0Name, (Vec4 *)pViewProjPrev.GetData(), 4);    
    Vec4 cBlurVec = PostEffectMgr().GetByNameVec4("Global_DirectionalBlur_Vec") * 0.01f;    

    CShaderMan::m_shPostMotionBlur->FXSetPSFloat(pParam3Name, &cBlurVec, 1);
    Vec4 vCamParams=Vec4(1, 1, 1, fCamScale ) ;       
    CShaderMan::m_shPostMotionBlur->FXSetVSFloat(pParam1Name, &vCamParams, 1);

    vParams.w = fCurrVectorsScale;       
    CShaderMan::m_shPostMotionBlur->FXSetPSFloat(pParam4Name, &vParams, 1);

    GetUtils().SetTexture(CTexture::s_ptexSceneTarget, 0, FILTER_LINEAR);  
    GetUtils().SetTexture(CTexture::s_ptexZTarget, 1, FILTER_POINT);  

    gRenDev->DrawPrimitives(&vertexBuffer,m_pCamSphereVerts.Count());

    fCurrVectorsScale *= 0.5f;

    GetUtils().ShEndPass();

    gcpRendD3D->Set2DMode(true, 1, 1);         
    // copy hdr render target into scene target
    if(nPasses != nQuality)
      GetUtils().StretchRect(CTexture::s_ptexHDRTarget, CTexture::s_ptexSceneTarget);    
  }

  m_nQualityInfo = nQuality;

  // store previous frame data
  //m_pPrevView =  pCurrView;
  pPrevView = GetUtils().m_pView;// pCurrView;
  m_fPrevFrameTime = fCurrFrameTime;                 
  gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;

  gcpRendD3D->FX_Flush();
  PROFILE_SHADER_END      
}