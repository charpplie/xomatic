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

void CDepthOfField::Render()
{
  PROFILE_LABEL_PUSH( "DEPTH OF FIELD" );

  uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
  gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);

  float fBlurAmount = m_pBlurAmount->GetParam();

  Vec4 vParamsFocus;
  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  PROFILE_SHADER_START

  float fPrevDist = 0, fPrevMaxCoC = 0, fPrevFocusRange = 0, fPrevBlurAmount = 0, fPrevFocusMin = 0, fPrevFocusMax = 0;
  float fPrevFocusLimit = 0, fPrevUseMask = 0;

  float fFocusRange = m_pFocusRange->GetParam();
  float fMaxCoC = m_pMaxCoC->GetParam();  
  fBlurAmount = m_pBlurAmount->GetParam();

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  {
    // Copy depth into backbuffer alpha channel    

    bool bGameDof = IsActive();
    float fUserFocusRange = m_pUserFocusRange->GetParam();
    float fUserFocusDistance = m_pUserFocusDistance->GetParam();
    float fUserBlurAmount = m_pUserBlurAmount->GetParam();
    float fFrameTime = clamp_tpl<float>(gEnv->pTimer->GetFrameTime()*3.0f, 0.0f, 1.0f); 

    if( bGameDof )
      fUserFocusRange = fUserFocusDistance = fUserBlurAmount = 0.0f;

    m_fUserFocusRangeCurr += (fUserFocusRange - m_fUserFocusRangeCurr) * fFrameTime;
    m_fUserFocusDistanceCurr += (fUserFocusDistance - m_fUserFocusDistanceCurr) * fFrameTime;
    m_fUserBlurAmountCurr += ( fUserBlurAmount - m_fUserBlurAmountCurr) * fFrameTime;

    if( CRenderer::CV_r_dof == 3)
      gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1]; 
     
    float fUseMask = m_pUseMask->GetParam();
    if(fFocusRange<0.0f && bGameDof )
    { 
      // Special case for gameplay
      if( fUseMask )        
      {
        static CCryName TechName("CopyDepthToAlphaBiased");
        GetUtils().ShBeginPass(CShaderMan::m_shPostDepthOfField, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
      }
      else
      {
        static CCryName TechName("CopyDepthToAlphaBiasedNoMask");
        GetUtils().ShBeginPass(CShaderMan::m_shPostDepthOfField, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);            
      }
    }
    else
    {
      // For cinematics (simplified interface, using only focus distance/range)
      static CCryName TechName("CopyDepthToAlphaNoMask");
      GetUtils().ShBeginPass(CShaderMan::m_shPostDepthOfField, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                                   
    }

    gcpRendD3D->EF_SetState( GS_NODEPTHTEST|GS_COLMASK_A ); 
    if( bGameDof )
    {
      if(fFocusRange<0.0f)
      {
        float fFocusMin = m_pFocusMin->GetParam();
        float fFocusMax = m_pFocusMax->GetParam();
        float fFocusLimit = m_pFocusLimit->GetParam();

        // near blur plane distance, far blur plane distance, focus plane distance, blur amount
        vParamsFocus=Vec4(fFocusMin, fFocusMax, fFocusLimit-fFocusMax, fBlurAmount);   
      }
      else
      {
        float fFocusRange_ = m_pFocusRange->GetParam();
        float fFocusDistance = m_pFocusDistance->GetParam();

        // near blur plane distance, far blur plane distance, focus plane distance, blur amount
        vParamsFocus=Vec4(-fFocusRange_*0.5f, fFocusRange_*0.5f, fFocusDistance, fBlurAmount);    
      }
    }
    else
    {
      // near blur plane distance, far blur plane distance, focus plane distance, blur amount
      vParamsFocus=Vec4(-m_fUserFocusRangeCurr*0.5f, m_fUserFocusRangeCurr*0.5f, m_fUserFocusDistanceCurr, m_fUserBlurAmountCurr);   
    }

    static CCryName ParamName("dofParamsFocus");
    GetUtils().ShSetParamPS(ParamName, vParamsFocus);

    GetUtils().SetTexture(CTexture::s_ptexZTarget, 0, FILTER_POINT);  

    CTexture *pMaskTex = const_cast<CTexture *> (static_cast<CParamTexture*>(m_pMaskTex)->GetParamTexture());

    if(pMaskTex && fUseMask)
    {
      float fMaskW = pMaskTex->GetWidth();
      float fMaskH = pMaskTex->GetHeight();

      Vec4 pParamTexScale = Vec4(0, 0, fMaskW, fMaskH); 

      GetUtils().SetTexture(pMaskTex, 1, FILTER_LINEAR);  
    }

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);

    GetUtils().ShEndPass();                    

    // XENON-srgb notes: since render directly to framebuffer then resolve to non-srgb surface, to get gamma correction done in hw - also saves 1 redundant resolve		
    GetUtils().CopyScreenToTexture(CTexture::s_ptexBackBuffer);

    // Restore previous viewport
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);     

    // Update back-buffer
    GetUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[0]);
    GetUtils().StretchRect(CTexture::s_ptexBackBufferScaled[0], CTexture::s_ptexBackBufferScaled[1]);
  }

  Vec4 vParamsBlur = Vec4(fMaxCoC*0.5f, fMaxCoC, fBlurAmount, vParamsFocus.w);
  if( CRenderer::CV_r_dof == 3)
    LayeredDofRender( vParamsFocus, vParamsBlur );
  else
    PoissonDofRender( vParamsFocus, vParamsBlur );

  gcpRendD3D->FX_Flush();

  PROFILE_SHADER_END    
  PROFILE_LABEL_POP( "DEPTH OF FIELD" );

  gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
} 

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CDepthOfField::PoissonDofRender( const Vec4 &pFocusParams, const Vec4 &pBlurParams )
{
  GetUtils().TexBlurGaussian(CTexture::s_ptexBackBufferScaled[1], 1, 1, 1, false);

  // Stencil-pre pass
  if( CRenderer::CV_r_dof_stencil_prepass )
  {
    // Draw in focus
    CCryName TechInFocusName("DofOutOfFocus");
    GetUtils().ShBeginPass(CShaderMan::m_shPostDepthOfField, TechInFocusName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                    

    gcpRendD3D->EF_SetState(GS_NODEPTHTEST);          
    GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);   
    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);
    GetUtils().ShEndPass();         		

    /////////////////////////////////////////////////////////////////////////////////////////////////

    static CCryName TechDOFStencilPrepassName("DofStencilPrepass");
    ////////////////////////
    // In focus stencil pre-pass

    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);
    gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

    GetUtils().ShBeginPass(CShaderMan::m_shPostDepthOfField, TechDOFStencilPrepassName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                    

    GetUtils().BeginStencilPrePass( /*true*/ );

    GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);  

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);

    GetUtils().ShEndPass();              

    GetUtils().EndStencilPrePass( );

    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////

  //if( CRenderer::CV_r_dof_stencil_prepass )
  //	GetUtils().XE_InvertHiStencil();

  static CCryName TechDOFName("DepthOfField");
  GetUtils().ShBeginPass(CShaderMan::m_shPostDepthOfField, TechDOFName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                    

  uint32 nRenderStates = GS_NODEPTHTEST;
  if( CRenderer::CV_r_dof_stencil_prepass )
  {
    nRenderStates |= GS_STENCIL;
    //GetUtils().SetupStencilStates(FSS_STENCFUNC_NOTEQUAL); 
    GetUtils().SetupStencilStates(FSS_STENCFUNC_EQUAL); 
  }

  gcpRendD3D->EF_SetState( nRenderStates );          

  static CCryName Param1Name("dofParamsFocus");
  GetUtils().ShSetParamPS(Param1Name, pFocusParams);

  //Vec4 vParamsBlur=Vec4(fMaxCoC*0.5f, fMaxCoC, fBlurAmount, fBlurAmount);   
  static CCryName Param2Name("dofParamsBlur");
  GetUtils().ShSetParamPS(Param2Name, pBlurParams);    

  Vec4 vPixelSizes=Vec4(1.0f/(float)CTexture::s_ptexBackBuffer->GetWidth(),
    1.0f/(float)CTexture::s_ptexBackBuffer->GetHeight(),
    1.0f/(float)CTexture::s_ptexBackBufferScaled[1]->GetWidth(),
    1.0f/(float)CTexture::s_ptexBackBufferScaled[1]->GetHeight()); 
  static CCryName Param3Name("pixelSizes");
  GetUtils().ShSetParamPS(Param3Name, vPixelSizes);    

  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);
  GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[1], 1);  

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY); 

  GetUtils().ShEndPass();                

  if( CRenderer::CV_r_dof_stencil_prepass )
    GetUtils().SetupStencilStates(-1);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CDepthOfField::BokehBlur( CTexture *pTex, float fAmount, float fOffsetsScale, bool bNearBlur )
{
  uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
  gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);

  if( bNearBlur )
    gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
  gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());     

  static CCryName TechBokehBlurName("BokehBlur");
  static CCryName Param1Name("psWeights");
  static CCryName Param2Name("bokehParams");
  CCryName Param3Name("psBokehOffsets");

  GetUtils().ShBeginPass(CShaderMan::m_shPostDepthOfField, TechBokehBlurName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                    
  gcpRendD3D->EF_SetState( GS_NODEPTHTEST );          

  Vec4 vParams = Vec4(1.0f, 1.0f, 1.0f, fOffsetsScale);   
  GetUtils().ShSetParamPS(Param2Name, vParams);    

  float fTexSizeW = fOffsetsScale * 0.5f / pTex->GetWidth();
  float fTexSizeH = fOffsetsScale * 0.5f / pTex->GetHeight();

  const float fMinAttenuation = 0.1f;
  Vec4 vWeights = Vec4(fTexSizeW, fTexSizeH, fMinAttenuation, fAmount );

  const int nBokehTypes = 5;
  Vec4 vBokehOffsets[nBokehTypes][15];

  float fDegToRad = 3.1415926f / 180.0f;
  float fSamplesRot =  fAmount * 10.0f;//0.0f;//100.0f*gEnv->pTimer->GetCurrTime();//0.0f; 

  const int nSamplesCount = 15;
  //Vec4 vBokehOffsets[nSamplesCount];

  // circular bokeh
  vBokehOffsets[0][0] = Vec4(cosf(DEG2RAD(0.0f + fSamplesRot) ), sinf(DEG2RAD(0.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[0][1] = Vec4(cosf(DEG2RAD(45.0f + fSamplesRot) ), sinf(DEG2RAD(45.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[0][2] = Vec4(cosf(DEG2RAD(90.0f + fSamplesRot) ), sinf(DEG2RAD(90.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[0][3] = Vec4(cosf(DEG2RAD(0.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(0.0f + 180.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[0][4] = Vec4(cosf(DEG2RAD(-45.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(-45.0f + 180.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[0][5] = Vec4(cosf(DEG2RAD(45.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(45.0f + 180.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[0][6] = Vec4(cosf(DEG2RAD(90.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(90.0f + 180.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[0][7] = Vec4(cosf(DEG2RAD(45.0f + 180.0f + 90.0f + fSamplesRot) ), sinf(DEG2RAD(45.0f + 180.0f + 90.0f + fSamplesRot) ), 0, 0);
  // compute intermediate samples
  vBokehOffsets[0][8]  = vBokehOffsets[0][0] * 0.5f;
  vBokehOffsets[0][9] = vBokehOffsets[0][1] * 0.5f;
  vBokehOffsets[0][10] = vBokehOffsets[0][3] * 0.5f;
  vBokehOffsets[0][11] = vBokehOffsets[0][4] * 0.5f;
  vBokehOffsets[0][12] = vBokehOffsets[0][5] * 0.5f;
  vBokehOffsets[0][13] = vBokehOffsets[0][7] * 0.5f;
  vBokehOffsets[0][14] =Vec4(0,0,0,0);

  // Store center distance - for sample attenuation
  const float fAttenuationScale = 1.0f / 1.0f;//.25f;
  for(int s= 0; s < nSamplesCount; ++s)
  {
    vBokehOffsets[0][s].w = clamp( fMinAttenuation + fAttenuationScale * sqrtf( vBokehOffsets[0][s].x * vBokehOffsets[0][s].x + vBokehOffsets[0][s].y * vBokehOffsets[0][s].y ), 0.0f, 1.0f);
  }

  // pentagonal bokeh
  vBokehOffsets[1][0] = Vec4(cosf(DEG2RAD(90.0f + fSamplesRot) ), sinf(DEG2RAD(90.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[1][1] = Vec4(cosf(DEG2RAD(30.0f + fSamplesRot) ), sinf(DEG2RAD(30.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[1][2] = Vec4(cosf(DEG2RAD(30.0f + 180.0f - fSamplesRot) ), sinf(DEG2RAD(30.0f - fSamplesRot) ), 0, 0);
  vBokehOffsets[1][3] = Vec4(cosf(DEG2RAD(60.0f - fSamplesRot) ), sinf(DEG2RAD(-60.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[1][4] = Vec4(cosf(DEG2RAD(60.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(-60.0f - fSamplesRot) ), 0, 0);
  vBokehOffsets[1][5] = Vec4(cosf(DEG2RAD(90.0f - fSamplesRot) ), sinf(DEG2RAD(-90.0f - fSamplesRot) ) * sinf( DEG2RAD(60.0f) ), 0, 0);
  // compute intermediate samples
  vBokehOffsets[1][6] = (vBokehOffsets[1][0] + vBokehOffsets[1][1]) * 0.5f;
  vBokehOffsets[1][7] = (vBokehOffsets[1][0] + vBokehOffsets[1][2]) * 0.5f;
  vBokehOffsets[1][8] = (vBokehOffsets[1][1] + vBokehOffsets[1][3]) * 0.5f;
  vBokehOffsets[1][9] = (vBokehOffsets[1][2] + vBokehOffsets[1][4]) * 0.5f;
  vBokehOffsets[1][10] = (vBokehOffsets[1][1]) * 0.5f;
  vBokehOffsets[1][11] = (vBokehOffsets[1][2]) * 0.5f;
  vBokehOffsets[1][12] = (vBokehOffsets[1][3]) * 0.5f;
  vBokehOffsets[1][13] = (vBokehOffsets[1][4]) * 0.5f;  
  vBokehOffsets[1][14] = (vBokehOffsets[1][0]) * 0.5f;

  // Store center distance - for sample attenuation
  //const float fAttenuationScale = 1.0f / 1.0f;//.25f;
  for(int s= 0; s < nSamplesCount; ++s)
  {
    vBokehOffsets[1][s].w = clamp( fMinAttenuation + fAttenuationScale * sqrtf( vBokehOffsets[1][s].x * vBokehOffsets[1][s].x + vBokehOffsets[1][s].y * vBokehOffsets[1][s].y ), 0.0f, 1.0f);
  }

  // square bokeh
  vBokehOffsets[2][0] = Vec4(cosf(DEG2RAD(45.0f + fSamplesRot) ), sinf(DEG2RAD(45.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[2][1] = Vec4(cosf(DEG2RAD(-45.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(-45.0f + 180.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[2][2] = Vec4(cosf(DEG2RAD(45.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(45.0f + 180.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[2][3] = Vec4(cosf(DEG2RAD(45.0f + 180.0f + 90.0f + fSamplesRot) ), sinf(DEG2RAD(45.0f + 180.0f + 90.0f + fSamplesRot) ), 0, 0);

  // compute intermediate samples
  vBokehOffsets[2][4] = (vBokehOffsets[2][0] + vBokehOffsets[2][1]) * 0.5f;
  vBokehOffsets[2][5] = (vBokehOffsets[2][0] + vBokehOffsets[2][3]) * 0.5f;
  vBokehOffsets[2][6] = (vBokehOffsets[2][1] + vBokehOffsets[2][2]) * 0.5f;
  vBokehOffsets[2][7] = (vBokehOffsets[2][2] + vBokehOffsets[2][3]) * 0.5f;

  vBokehOffsets[2][8] = (vBokehOffsets[2][4]) * 0.5f;
  vBokehOffsets[2][9] = (vBokehOffsets[2][7]) * 0.5f;

  vBokehOffsets[2][10] = (vBokehOffsets[2][0]) * 0.5f;
  vBokehOffsets[2][11] = (vBokehOffsets[2][1]) * 0.5f;
  vBokehOffsets[2][12] = (vBokehOffsets[2][2]) * 0.5f;
  vBokehOffsets[2][13] = (vBokehOffsets[2][3]) * 0.5f;  
  vBokehOffsets[2][14] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);

  // Store center distance - for sample attenuation
  //const float fAttenuationScale = 1.0f / 1.0f;//.25f;
  for(int s= 0; s < nSamplesCount; ++s)
  {
    vBokehOffsets[2][s].w = clamp( fMinAttenuation + fAttenuationScale * sqrtf( vBokehOffsets[2][s].x * vBokehOffsets[2][s].x + vBokehOffsets[2][s].y * vBokehOffsets[2][s].y ), 0.0f, 1.0f);
  }

  // hexagonal bokeh
  vBokehOffsets[3][0] = Vec4(cosf(DEG2RAD(45.0f + fSamplesRot) ), sinf(DEG2RAD(45.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[3][1] = Vec4(cosf(DEG2RAD(-45.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(-45.0f + 180.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[3][2] = Vec4(cosf(DEG2RAD(45.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(45.0f + 180.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[3][3] = Vec4(cosf(DEG2RAD(45.0f + 180.0f + 90.0f + fSamplesRot) ), sinf(DEG2RAD(45.0f + 180.0f + 90.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[3][4] = Vec4(cosf(DEG2RAD(0.0f+ fSamplesRot) ), sinf(DEG2RAD(0.0f + fSamplesRot) ), 0, 0);
  vBokehOffsets[3][5] = Vec4(cosf(DEG2RAD(0.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(0.0f + 180.0f + fSamplesRot) ), 0, 0);

  // compute intermediate samples
  vBokehOffsets[3][6] = (vBokehOffsets[3][0]) * 0.5f;
  vBokehOffsets[3][7] = (vBokehOffsets[3][1]) * 0.5f;
  vBokehOffsets[3][8] = (vBokehOffsets[3][2]) * 0.5f;
  vBokehOffsets[3][9] = (vBokehOffsets[3][3]) * 0.5f;
  vBokehOffsets[3][10] = (vBokehOffsets[3][4]) * 0.5f;
  vBokehOffsets[3][11] = (vBokehOffsets[3][5]) * 0.5f;

  vBokehOffsets[3][12] = (vBokehOffsets[3][0] + vBokehOffsets[3][1]) * 0.5f;
  vBokehOffsets[3][13] = (vBokehOffsets[3][2] + vBokehOffsets[3][3]) * 0.5f;
  vBokehOffsets[3][14] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);

  // Store center distance - for sample attenuation
  //const float fAttenuationScale = 1.0f / 1.0f;//.25f;
  for(int s= 0; s < nSamplesCount; ++s)
  {
    vBokehOffsets[3][s].w = clamp( fMinAttenuation + fAttenuationScale * sqrtf( vBokehOffsets[3][s].x * vBokehOffsets[3][s].x + vBokehOffsets[3][s].y * vBokehOffsets[3][s].y ), 0.0f, 1.0f);
  }

  // "love" bokeh

  fSamplesRot = 0;
  // Store center distance - for sample attenuation
  vBokehOffsets[4][0] = Vec4(cosf(DEG2RAD(15.0f + fSamplesRot) ), sinf(DEG2RAD(15.0f + fSamplesRot) ), 0, 1.0f);
  vBokehOffsets[4][1] = Vec4(cosf(DEG2RAD(50.0f + fSamplesRot) ), sinf(DEG2RAD(50.0f + fSamplesRot) ), 0, 1.0f);
  vBokehOffsets[4][2] = Vec4(cosf(DEG2RAD(70.0f + fSamplesRot) ), sinf(DEG2RAD(70.0f + fSamplesRot) ), 0, 1.0f) * 0.5f;
  vBokehOffsets[4][3] = Vec4(cosf(DEG2RAD(-15.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(-15.0f + 180.0f + fSamplesRot) ), 0, 1.0f);
  vBokehOffsets[4][4] = Vec4(cosf(DEG2RAD(-50.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(-50.0f + 180.0f + fSamplesRot) ), 0, 1.0f);
  vBokehOffsets[4][5] = Vec4(cosf(DEG2RAD(-70.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(-70.0f + 180.0f + fSamplesRot) ), 0, 1.0f)*0.5f;

  
  vBokehOffsets[4][6] = Vec4(cosf(DEG2RAD(40.0f + 180.0f + fSamplesRot) ), sinf(DEG2RAD(40.0f + 180.0f +fSamplesRot) ), 0, 1.0f)*0.75f;
  vBokehOffsets[4][7] = Vec4(cosf(DEG2RAD(0.0f + 180.0f + 90.0f + fSamplesRot) ), sinf(DEG2RAD(0.0f + 180.0f + 90.0f + fSamplesRot) ), 0, 1.0f);
  vBokehOffsets[4][8] = Vec4(cosf(DEG2RAD(50.0f + 180.0f + 90.0f + fSamplesRot) ), sinf(DEG2RAD(50.0f + 180.0f + 90.0f + fSamplesRot) ), 0, 1.0f)*0.75f;

  // compute intermediate samples
  vBokehOffsets[4][9] = (vBokehOffsets[4][0]) * 0.5f;
  vBokehOffsets[4][10] = (vBokehOffsets[4][3]) * 0.5f;
  vBokehOffsets[4][11] = (vBokehOffsets[4][2]) * 0.5f;
  vBokehOffsets[4][12] = (vBokehOffsets[4][5]) * 0.5f;
  vBokehOffsets[4][13] = (vBokehOffsets[4][6]) * 0.5f;
  vBokehOffsets[4][14] = (vBokehOffsets[4][8]) * 0.5f;

  for(int s= 0; s < nSamplesCount; ++s)
  {
    vBokehOffsets[4][s].w = clamp( fMinAttenuation + fAttenuationScale * sqrtf( vBokehOffsets[4][s].x * vBokehOffsets[4][s].x + vBokehOffsets[4][s].y * vBokehOffsets[4][s].y ), 0.0f, 1.0f);
  }

  static Vec4 vMorphedBokeh[15]=
  {
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f),
    Vec4(0.0f, 0.0f, 0.0f, 0.0f)
  };

  float fBlend = gEnv->pTimer->GetFrameTime() *5.0f;
  int nBokehType = max(min(CRenderer::CV_r_dof_bokeh, 4), 0);
  vMorphedBokeh[0] += (vBokehOffsets[nBokehType][0] - vMorphedBokeh[0]) * fBlend;
  vMorphedBokeh[1] += (vBokehOffsets[nBokehType][1] - vMorphedBokeh[1]) * fBlend;
  vMorphedBokeh[2] += (vBokehOffsets[nBokehType][2] - vMorphedBokeh[2]) * fBlend;
  vMorphedBokeh[3] += (vBokehOffsets[nBokehType][3] - vMorphedBokeh[3]) * fBlend;
  vMorphedBokeh[4] += (vBokehOffsets[nBokehType][4] - vMorphedBokeh[4]) * fBlend;
  vMorphedBokeh[5] += (vBokehOffsets[nBokehType][5] - vMorphedBokeh[5]) * fBlend;
  vMorphedBokeh[6] += (vBokehOffsets[nBokehType][6] - vMorphedBokeh[6]) * fBlend;
  vMorphedBokeh[7] += (vBokehOffsets[nBokehType][7] - vMorphedBokeh[7]) * fBlend;
  vMorphedBokeh[8] += (vBokehOffsets[nBokehType][8] - vMorphedBokeh[8]) * fBlend;
  vMorphedBokeh[9] += (vBokehOffsets[nBokehType][9] - vMorphedBokeh[9]) * fBlend;
  vMorphedBokeh[10] += (vBokehOffsets[nBokehType][10] - vMorphedBokeh[10]) * fBlend;
  vMorphedBokeh[11] += (vBokehOffsets[nBokehType][11] - vMorphedBokeh[11]) * fBlend;
  vMorphedBokeh[12] += (vBokehOffsets[nBokehType][12] - vMorphedBokeh[12]) * fBlend;
  vMorphedBokeh[13] += (vBokehOffsets[nBokehType][13] - vMorphedBokeh[13]) * fBlend;
  vMorphedBokeh[14] += (vBokehOffsets[nBokehType][14] - vMorphedBokeh[14]) * fBlend;

  CShaderMan::m_shPostDepthOfField->FXSetPSFloat(Param1Name, &vWeights, 1);   
  CShaderMan::m_shPostDepthOfField->FXSetPSFloat(Param3Name, vMorphedBokeh, 15);  

  GetUtils().SetTexture( pTex, 0, FILTER_LINEAR ); 
  GetUtils().DrawFullScreenQuad(pTex->GetWidth(), pTex->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);  

  GetUtils().ShEndPass();
  
  GetUtils().CopyScreenToTexture( pTex );

  // Restore previous viewport
  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);     
  
  gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}

void CDepthOfField::LayeredDofRender( const Vec4 &pFocusParams, const Vec4 &pBlurParams )
{
  float fBlurAmount = min( pBlurParams.w, 2.0f);   // clamp blur to acceptable range

   //BokehBlur( CTexture::s_ptexBackBufferScaled[1] , 0.025f, pBlurParams.w );    
  BokehBlur( CTexture::s_ptexBackBufferScaled[1] , pFocusParams.z, fBlurAmount * 2.0f);   

  // 1. Far out of focus render

  // In focus render
  static CCryName TechInFocusName("DofInFocus");

  GetUtils().ShBeginPass(CShaderMan::m_shPostDepthOfField, TechInFocusName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                    
  gcpRendD3D->EF_SetState( GS_NODEPTHTEST);          

  CCryName ParamName("bokehParams");
  float fBlurBlendAmount = powf(fBlurAmount, 0.1f);

  Vec4 vParams = Vec4(1.0f, 1.0f, 1.0f, min(1.0f, fBlurBlendAmount));   
  GetUtils().ShSetParamPS(ParamName, vParams);    

  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);
  GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[1], 1, FILTER_LINEAR);
   
  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY); 

  GetUtils().ShEndPass();

  if( pFocusParams.z > 0.01f )
  {
    // 2. Near out of focus render
    GetUtils().CopyScreenToTexture( CTexture::s_ptexBackBuffer );
    //GetUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[0]); 
    GetUtils().StretchRect(CTexture::s_ptexBackBufferScaled[0], CTexture::s_ptexBackBufferScaled[1]);
    

    
    BokehBlur( CTexture::s_ptexBackBufferScaled[1] , pFocusParams.z, fBlurAmount* 2.0f, true ); 
    //GetUtils().StretchRect(CTexture::s_ptexBackBufferScaled[1], CTexture::s_ptexBackBufferScaled[2]);
    //BokehBlur( CTexture::s_ptexBackBufferScaled[2] , 0.025f, fBlurAmount* 1.0f, true ); 
    
    
    //BokehBlur( CTexture::s_ptexBackBufferScaled[0] , -1.0f, fBlurAmount , true );    
    
    //
    
      //GetUtils().TexBlurGaussian(CTexture::s_ptexBackBufferScaled[1], 1, 1.0f, LERP(0.0f, 0.5f, min(1.0, fBlurAmount)), false);             

    // Out focus near render
    static CCryName TechNearOutFocusName("NearOutFocus");

    GetUtils().ShBeginPass(CShaderMan::m_shPostDepthOfField, TechNearOutFocusName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                    
    gcpRendD3D->EF_SetState( GS_NODEPTHTEST);          

    GetUtils().ShSetParamPS(ParamName, vParams);    

    GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT); 
    GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[1], 1, FILTER_LINEAR);
    //GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[2], 2, FILTER_LINEAR);
    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY); 

    GetUtils().ShEndPass();
  }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CDepthOfField::RenderHDR()
{
  if ( !CRenderer::CV_r_usezpass || !CRenderer::CV_r_dof || CRenderer::CV_r_dof != 2)  
    return;

  float fTimeOfDayBlurAmount = m_pTimeOfDayBlurAmount->GetParam();
  if( (m_pBlurAmount->GetParam() <= 0.1f && m_pUserBlurAmount->GetParam() <= 0.1f && fTimeOfDayBlurAmount <= 0.1f))      
    return;

  if( !IsActive() && !m_pUserActive->GetParam() && (!fTimeOfDayBlurAmount || (fTimeOfDayBlurAmount && (!CRenderer::CV_r_colorgrading || !CRenderer::CV_r_colorgrading_dof))) )
    return;

  float fBlurAmount = m_pBlurAmount->GetParam();     

  Vec4 vParamsFocus;
  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
  // Enable corresponding shader variation
  gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);

  //if( CRenderer::CV_r_colorgrading_levels && (fMinInput || fGammaInput || fMaxInput || fMinOutput ||fMaxOutput) )
  gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0]; // enable hdr version

  float fPrevDist = 0, fPrevMaxCoC = 0, fPrevFocusRange = 0, fPrevBlurAmount = 0, fPrevFocusMin = 0, fPrevFocusMax = 0;
  float fPrevFocusLimit = 0, fPrevUseMask = 0;

  float fTodFocusRange = m_pTimeOfDayFocusRange->GetParam();
  float fTodBlurAmount = m_pTimeOfDayBlurAmount->GetParam();

  float fFocusRange = m_pFocusRange->GetParam();
  float fMaxCoC = m_pMaxCoC->GetParam();  
  fBlurAmount = m_pBlurAmount->GetParam();

  //if( fBlurAmount < 0.01f)
  //return;

  PROFILE_SHADER_START

  CTexture *pSceneRT = CTexture::s_ptexSceneTarget;
  CTexture *pSceneScaledTempRT = CTexture::s_ptexHDRTargetScaled[0];
  CTexture *pSceneScaledRT = CTexture::s_ptexHDRTargetScaled[1];

  bool bGameDof = IsActive();
  float fUserBlurAmount = m_pUserBlurAmount->GetParam();

  // update scene target
  //gcpRendD3D->FX_ScreenStretchRect(pSceneRT);

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  //if( 0 ) 
  {  
    gcpRendD3D->FX_PushRenderTarget(0,  pSceneRT,  &gcpRendD3D->m_DepthBufferOrigFSAA); 
    gcpRendD3D->RT_SetViewport(0, 0, pSceneRT->GetWidth(), pSceneRT->GetHeight());        

    // Copy depth into backbuffer alpha channel

    float fUserFocusRange = m_pUserFocusRange->GetParam();
    float fUserFocusDistance = m_pUserFocusDistance->GetParam();

    float fFrameTime = clamp_tpl<float>(gEnv->pTimer->GetFrameTime()*3.0f, 0.0f, 1.0f); 

    if( bGameDof )
      fUserFocusRange = fUserFocusDistance = fUserBlurAmount = 0.0f;

    m_fUserFocusRangeCurr += (fUserFocusRange - m_fUserFocusRangeCurr) * fFrameTime;
    m_fUserFocusDistanceCurr += (fUserFocusDistance - m_fUserFocusDistanceCurr) * fFrameTime;
    m_fUserBlurAmountCurr += ( fUserBlurAmount - m_fUserBlurAmountCurr) * fFrameTime;

    float fUseMask = m_pUseMask->GetParam();

    if(fFocusRange<0.0f && bGameDof)
    { 
      // Special case for gameplay
      if( fUseMask )        
      {
        static CCryName TechName("CopyDepthToAlphaBiased");
        GetUtils().ShBeginPass(CShaderMan::m_shPostDepthOfField, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
      }
      else
      {
        static CCryName TechName("CopyDepthToAlphaBiasedNoMask");
        GetUtils().ShBeginPass(CShaderMan::m_shPostDepthOfField, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);            
      }
    }
    else
    {
      // For cinematics (simplified interface, using only focus distance/range)
      static CCryName TechName("CopyDepthToAlphaNoMask");
      GetUtils().ShBeginPass(CShaderMan::m_shPostDepthOfField, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                                   
    }

    gcpRendD3D->EF_SetState(GS_NODEPTHTEST);  //|GS_COLMASK_A

    if( bGameDof )
    {
      if(fFocusRange<0.0f)
      {
        float fFocusMin = m_pFocusMin->GetParam();
        float fFocusMax = m_pFocusMax->GetParam();
        float fFocusLimit = m_pFocusLimit->GetParam();

        // near blur plane distance, far blur plane distance, focus plane distance, blur amount
        vParamsFocus=Vec4(fFocusMin, fFocusMax, fFocusLimit-fFocusMax, fBlurAmount);   
      }
      else
      {
        float fFocusRange_ = m_pFocusRange->GetParam();
        float fFocusDistance = m_pFocusDistance->GetParam();

        // near blur plane distance, far blur plane distance, focus plane distance, blur amount
        vParamsFocus=Vec4(-fFocusRange_*0.5f, fFocusRange_*0.5f, fFocusDistance, fBlurAmount);    
      }
    }
    else
    {
      // near blur plane distance, far blur plane distance, focus plane distance, blur amount
      static float s_fTodFocusRange = 0.0f;
      static float s_fTodBlurAmount = 0.0f;
      bool bUseGameSettings = (m_pUserActive->GetParam() )? true: false;

      if( bUseGameSettings )
      {
        s_fTodFocusRange += (m_fUserFocusRangeCurr - s_fTodFocusRange) * fFrameTime;
        s_fTodBlurAmount += (m_fUserBlurAmountCurr - s_fTodBlurAmount) * fFrameTime;

        vParamsFocus=Vec4(-m_fUserFocusRangeCurr*0.5f, m_fUserFocusRangeCurr*0.5f, m_fUserFocusDistanceCurr, m_fUserBlurAmountCurr);   
      }
      else
      {
        s_fTodFocusRange += (fTodFocusRange*2.0f - s_fTodFocusRange) * fFrameTime;
        s_fTodBlurAmount += (fTodBlurAmount - s_fTodBlurAmount) * fFrameTime;

        vParamsFocus=Vec4(-s_fTodFocusRange*0.5f, s_fTodFocusRange*0.5f, 0, s_fTodBlurAmount);   
      }
    }

    static CCryName ParamName("dofParamsFocus");
    GetUtils().ShSetParamPS(ParamName, vParamsFocus);

    //vParamsFocus=Vec4(0, 0, fTodFocusRange, fTodBlurAmount);   
    //static CCryName Param1Name("dofParamsFocus2");
    //CShaderMan::m_shPostEffects->FXSetPSFloat(Param1Name, &vParamsFocus, 1);

    GetUtils().SetTexture(CTexture::s_ptexZTarget, 0, FILTER_POINT);  

    CTexture *pMaskTex = const_cast<CTexture *> (static_cast<CParamTexture*>(m_pMaskTex)->GetParamTexture());

    if(pMaskTex && fUseMask && bGameDof)
    {
      float fMaskW = pMaskTex->GetWidth();
      float fMaskH = pMaskTex->GetHeight();

      Vec4 pParamTexScale = Vec4(0, 0, fMaskW, fMaskH); 

      GetUtils().SetTexture(pMaskTex, 1, FILTER_LINEAR);  
      GetUtils().SetTexture(CTexture::s_ptexHDRTarget, 2, FILTER_POINT);
    }
    else
      GetUtils().SetTexture(CTexture::s_ptexHDRTarget, 1, FILTER_POINT);

    GetUtils().DrawFullScreenQuad(pSceneRT->GetWidth(), pSceneRT->GetHeight());

    GetUtils().ShEndPass();                    

    // Restore previous viewport
    gcpRendD3D->FX_PopRenderTarget(0);
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);     

    // Update back-buffer
    GetUtils().StretchRect(pSceneRT, pSceneScaledTempRT);
    //GetUtils().StretchRect(pSceneScaledTempRT, pSceneScaledRT);
  }

  //GetUtils().TexBlurGaussian(pSceneScaledRT, 1, 1, 0.5, false);  
  GetUtils().TexBlurGaussian(pSceneScaledTempRT, 1, 1, 1, false);  

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////

  static CCryName TechDOFName("DepthOfFieldHDR");
  GetUtils().ShBeginPass(CShaderMan::m_shPostDepthOfField, TechDOFName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                    

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);          

  static CCryName Param1Name("dofParamsFocus");
  GetUtils().ShSetParamPS(Param1Name, vParamsFocus);

  Vec4 vParamsBlur=Vec4(fMaxCoC*0.5f, fMaxCoC, fBlurAmount, fBlurAmount);   
  static CCryName Param2Name("dofParamsBlur");
  GetUtils().ShSetParamPS(Param2Name, vParamsBlur);    

  Vec4 vPixelSizes=Vec4(1.0f/(float)pSceneRT->GetWidth(),
    1.0f/(float)pSceneRT->GetHeight(),
    1.0f/(float)pSceneScaledTempRT->GetWidth(),
    1.0f/(float)pSceneScaledTempRT->GetHeight()); 
  static CCryName Param3Name("pixelSizes");
  GetUtils().ShSetParamPS(Param3Name, vPixelSizes);    

  GetUtils().SetTexture(pSceneRT, 0, FILTER_POINT);
  GetUtils().SetTexture(pSceneScaledTempRT, 1);  
  GetUtils().SetTexture(m_pNoise, 2, FILTER_POINT, 0);  


  GetUtils().DrawFullScreenQuad(pSceneRT->GetWidth(), pSceneRT->GetHeight());

  GetUtils().ShEndPass();                    

  gcpRendD3D->FX_Flush();
  PROFILE_SHADER_END    

    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}
