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

void CVolumetricScattering::Render()
{
  PROFILE_SHADER_START

    // quickie-prototype
    //    - some ideas: add several types (cloudy/sparky/yadayada)

    // Get current viewport
    int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  gcpRendD3D->Set2DMode(false, 1, 1);       

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Render god-rays into low-res render target for less fillrate hit

  gcpRendD3D->FX_PushRenderTarget(0,  CTexture::s_ptexBackBufferScaled[1],  GetUtils().m_pCurDepthSurface); 
  gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexBackBufferScaled[1]->GetWidth(), CTexture::s_ptexBackBufferScaled[1]->GetHeight());        

  ColorF clearColor(0, 0, 0, 0);
  gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clearColor);


  float fAmount = m_pAmount->GetParam();
  float fTilling = m_pTilling->GetParam();
  float fSpeed = m_pSpeed->GetParam();
  Vec4 pColor = m_pColor->GetParamVec4();

  static CCryName pTechName("VolumetricScattering");
  uint32 nPasses;
  CShaderMan::m_shPostEffects->FXSetTechnique(pTechName);
  CShaderMan::m_shPostEffects->FXBegin(&nPasses, FEF_DONTSETSTATES);

  gcpRendD3D->SetCullMode( R_CULL_NONE );
  gcpRendD3D->EF_SetState(GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCCOL | GS_NODEPTHTEST);    

  int nSlicesCount = 10;   

  Vec4 pParams;
  pParams= Vec4(fTilling, fSpeed, fTilling, fSpeed);

  static CCryName pParam0Name("VolumetricScattering");
  static CCryName pParam1Name("VolumetricScatteringColor");

  static CCryName pParam2Name("PI_volScatterParamsVS");
  static CCryName pParam3Name("PI_volScatterParamsPS");

  for( int r(0); r < nSlicesCount; ++r)   
  {
    // !force updating constants per-pass! (dx10..)
    CShaderMan::m_shPostEffects->FXBeginPass(0);

    // Set PS default params  
    Vec4 pParamsPI = Vec4(1.0f, fAmount, r, 1.0f / (float) nSlicesCount);
    CShaderMan::m_shPostEffects->FXSetVSFloat(pParam0Name, &pParams, 1);
    CShaderMan::m_shPostEffects->FXSetPSFloat(pParam1Name, &pColor, 1);    
    CShaderMan::m_shPostEffects->FXSetVSFloat(pParam2Name, &pParamsPI, 1);
    CShaderMan::m_shPostEffects->FXSetPSFloat(pParam3Name, &pParamsPI, 1);

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBufferScaled[1]->GetWidth(), CTexture::s_ptexBackBufferScaled[1]->GetHeight()); 

    CShaderMan::m_shPostEffects->FXEndPass();
  }


  CShaderMan::m_shPostEffects->FXEnd(); 

  gcpRendD3D->Set2DMode(true, 1, 1);       

  // Restore previous viewport
  gcpRendD3D->FX_PopRenderTarget(0);
  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);     

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Display volumetric scattering effect

  CCryName pTechName0("VolumetricScatteringFinal");

  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName0, FEF_DONTSETSTATES);   
  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 
  GetUtils().ShEndPass(); 

  gcpRendD3D->FX_Flush(); 
  PROFILE_SHADER_END  
}

