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

bool CAlphaTestAA::Preprocess()
{
  bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality( eRQ_High, eSQ_High );
  if( !bQualityCheck || gRenDev->m_RP.m_FSAAData.Type || CRenderer::CV_r_use_merged_posteffects && CRenderer::CV_r_useedgeaa == 1)
    return false;

  return ( IsActive() && CRenderer::CV_r_useedgeaa != 0 );
}

void CAlphaTestAA::Render()
{
  if(!CRenderer::CV_r_usezpass) 
  {
    return;
  }	

  PROFILE_LABEL_PUSH( "EDGEAA" );

  PROFILE_SHADER_START

  uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
  gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);

  //bool bEnableColorCorrection = false;
  //if( !CRenderer::CV_r_HDRRendering )
  //	bEnableColorCorrection = CColorCorrection::CheckColorChanges();
  //
  //if( bEnableColorCorrection )
  //	gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

  if(CRenderer::CV_r_useedgeaa == 2)
  {
    static CCryName TechName("EdgeAA");
    GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                    
  }
  else 
  {
    static CCryName TechName("EdgeBlurOpt");
    GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                    
  }

  gcpRendD3D->SetCullMode( R_CULL_NONE );
  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

  //if( bEnableColorCorrection )
  //{
  //	// Compose final color matrix and set fragment program constants
  //	Matrix44 &pColorMat = GetUtils().GetColorMatrix();

  //	Vec4 pColorMatrix[4]=
  //	{
  //		Vec4(pColorMat.m00, pColorMat.m01, pColorMat.m02, pColorMat.m03),
  //		Vec4(pColorMat.m10, pColorMat.m11, pColorMat.m12, pColorMat.m13), 
  //		Vec4(pColorMat.m20, pColorMat.m21, pColorMat.m22, pColorMat.m23),
  //		Vec4(pColorMat.m30, pColorMat.m31, pColorMat.m32, pColorMat.m33)
  //	};  

  //	static CCryName pParamName("mColorMatrix");
  //	CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, pColorMatrix, 4);
  //}

  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_LINEAR);   
  GetUtils().SetTexture(CTexture::s_ptexZTarget, 1, FILTER_POINT);        
  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);


  GetUtils().ShEndPass();                    

  gcpRendD3D->FX_Flush();

  gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;

  PROFILE_SHADER_END

    PROFILE_LABEL_POP( "EDGEAA" );
}

///////////////////////////////////////////////////////////////////////////////////////////////////// 
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CColorCorrection::Render() 
{
  PROFILE_LABEL_PUSH( "COLORCORRECTION" );

  static CCryName pTechName("ColorCorrection");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

  // Compose final color matrix and set fragment program constants
  Matrix44 &pColorMat = GetUtils().GetColorMatrix();

  Vec4 pColorMatrix[4]=
  {
    Vec4(pColorMat.m00, pColorMat.m01, pColorMat.m02, pColorMat.m03),
    Vec4(pColorMat.m10, pColorMat.m11, pColorMat.m12, pColorMat.m13), 
    Vec4(pColorMat.m20, pColorMat.m21, pColorMat.m22, pColorMat.m23),
    Vec4(pColorMat.m30, pColorMat.m31, pColorMat.m32, pColorMat.m33)
  };  

  static CCryName pParamName("mColorMatrix");
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, pColorMatrix, 4);

  //  // Set PS default params
  //  Vec4 pParams= Vec4(CRenderer::CV_r_pp_grainamount, CRenderer::CV_r_pp_grainthreshold, 
  //                     fGlobalSharpening, (CRenderer::CV_r_pp_gamma>0)?1.0f/CRenderer::CV_r_pp_gamma: 0.0f);       
  //  CShaderMan::m_shPostEffects->FXSetPSFloat("renderModeParamsPS", &pParams, 1);      
  //  // Set VS default params 
  //  pParams= Vec4(0.0f, 0.0f, CRenderer::CV_r_pp_grainscale, (float)GetUtils().m_iFrameCounter);       
  //  CShaderMan::m_shPostEffects->FXSetVSFloat("renderModeParamsVS", &pParams, 1);

  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);   
  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 

  GetUtils().ShEndPass();   

  PROFILE_LABEL_POP( "COLORCORRECTION" );
} 



////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// todo: handle diferent sharpening filters

void CFilterSharpening::Render()
{ 
  PROFILE_LABEL_PUSH( "SHARPENING" );

  GetUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[0]);    

  static CCryName pTechName("BlurInterpolation");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);     
  float fType = m_pType->GetParam();
  float fAmount = m_pAmount->GetParam();

  // Set PS default params
  Vec4 pParams= Vec4(0, 0, 0, fAmount);
  static CCryName pParamName("psParams");
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);   
  GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[0], 1);    
  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 

  GetUtils().ShEndPass();   

  PROFILE_LABEL_POP( "SHARPENING" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// todo: handle diferent blurring filters, add wavelength based blur

void CFilterBlurring::Render()
{  
  PROFILE_LABEL_PUSH( "BLURRING" );

  float fType = m_pType->GetParam();
  float fAmount = m_pAmount->GetParam();
  fAmount = clamp_tpl<float>(fAmount, 0.0f, 1.0f);

  // maximum blur amount to have nice results
  const float fMaxBlurAmount = 5.0f;

  // this is uber expensive - and barely no need for this - adjusting gaussian distribution already looks quite good
  //GetUtils().TexBlurGaussian(CTexture::s_ptexBackBuffer, 1, 1.0f, LERP(0.0f, fMaxBlurAmount, sqrtf(fAmount) ), false);             

  GetUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[0]);    
  GetUtils().TexBlurGaussian(CTexture::s_ptexBackBufferScaled[0], 1, 1.0f, LERP(0.0f, fMaxBlurAmount, fAmount), false);             

  static CCryName pTechName("BlurInterpolation");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);     

  // Set PS default params  
  Vec4 pParams= Vec4(0, 0, 0, fAmount * fAmount);
  static CCryName pParamName("psParams");
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

  GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[0], 0);    
  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 1, FILTER_POINT);     
  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 

  GetUtils().ShEndPass();   

  PROFILE_LABEL_POP( "BLURRING" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CFilterMaskedBlurring::Render()
{  
  PROFILE_LABEL_PUSH( "MASKED_BLURRING" );

  if( m_pAmount->GetParam() <= 0.005f )
    return;

  float fAmount = m_pAmount->GetParam();
  fAmount = clamp_tpl<float>(fAmount, 0.0f, 1.0f);

  CTexture *pMaskTex = const_cast<CTexture *> (static_cast<CParamTexture*>(m_pMaskTex)->GetParamTexture());
  if( !pMaskTex )
    pMaskTex = CTexture::s_ptexWhite;

  // maximum blur amount to have nice results
  const float fMaxBlurAmount = 5.0f;

  CTexture *pScreen = CTexture::s_ptexBackBuffer;
  CTexture *pScreenScaled = CTexture::s_ptexBackBufferScaled[0];

  // do it in hdr mode instead
  if( gRenDev->m_RP.m_eQuality >= eRQ_High && CRenderer::CV_r_HDRRendering)
  {
    pScreen = CTexture::s_ptexSceneTarget;
    pScreenScaled = CTexture::s_ptexHDRTargetScaled[0];
    // need to update target
    //gcpRendD3D->FX_ScreenStretchRect(pScreen);

    // draw hdr render target into scene target - saves 1 resolve (improves performance under FSAA)
    GetUtils().StretchRect(CTexture::s_ptexHDRTarget, pScreen);    
  }

  GetUtils().TexBlurGaussian(pScreen, 1, 1.0f, LERP(0.0f, fMaxBlurAmount, sqrtf(fAmount) ), false, pMaskTex);             
  GetUtils().StretchRect(pScreen, pScreenScaled);    
  GetUtils().TexBlurGaussian(pScreenScaled, 1, 1.0f, LERP(0.0f, fMaxBlurAmount, fAmount), false, pMaskTex);             

  static CCryName pTechName("BlurInterpolation");
  CCryName pTechName2("MaskedBlurInterpolation");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName2, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);     

  // Set PS default params  

  static CCryName pParamName("psParams");
  static CCryName pParam1Name("TexSizeInfo");
  Vec4 pParams= Vec4(0, 0, 0, fAmount * fAmount);
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

  int w = pMaskTex->GetWidth();
  int h = pMaskTex->GetHeight();

  pParams= Vec4(w, h, 1.0f / (float) w, 1.0f/ (float) h);
  CShaderMan::m_shPostEffects->FXSetVSFloat(pParam1Name, &pParams, 1);

  GetUtils().SetTexture(pScreenScaled, 0);    
  GetUtils().SetTexture(pScreen, 1, FILTER_POINT);     
  GetUtils().SetTexture(pMaskTex, 2);     

  GetUtils().DrawFullScreenQuad(pScreen->GetWidth(), pScreen->GetHeight()); 

  GetUtils().ShEndPass();   

  PROFILE_LABEL_POP( "MASKED_BLURRING" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// todo: add wavelength based blur

void CFilterRadialBlurring::Render()
{  
  PROFILE_LABEL_PUSH( "RADIAL_BLURRING" );

  // Get current viewport
  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  float fAmount = m_pAmount->GetParam();
  fAmount = clamp_tpl<float>(fAmount, 0.0f, 1.0f);

  float fScreenPosX = m_pScreenPosX->GetParam();
  fScreenPosX = clamp_tpl<float>(fScreenPosX, 0.0f, 1.0f);

  float fScreenPosY = m_pScreenPosY->GetParam();
  fScreenPosY = clamp_tpl<float>(fScreenPosY, 0.0f, 1.0f);

  float fRadius = m_pRadius->GetParam();
  fRadius = 1.0f / clamp_tpl<float>(fRadius*2.0f, 0.001f, 2.0f); 

  // Back-buffer radial blurring
  static CCryName pTech0Name("RadialBlurring");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTech0Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);     

  // Set PS default params  
  Vec4 pParams= Vec4(fScreenPosX, fScreenPosY, fRadius, 0.01f * fAmount);
  static CCryName pParam0Name("vRadialBlurParams");
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParam0Name, &pParams, 1);

  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);     
  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 

  GetUtils().ShEndPass();   

  // Update back-buffer texture
  GetUtils().CopyScreenToTexture(CTexture::s_ptexBackBuffer);    
  GetUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[0]);    

  gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexBackBufferScaled[0]->GetWidth(), CTexture::s_ptexBackBufferScaled[0]->GetHeight());        

  // Low-res back-buffer radial blurring
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTech0Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);     

  pParams= Vec4(fScreenPosX, fScreenPosY, fRadius, 0.02f * fAmount); // use 2x bigger blurring radius
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParam0Name, &pParams, 1);

  GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[0], 0);     
  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 

  GetUtils().CopyScreenToTexture(CTexture::s_ptexBackBufferScaled[0]);    

  GetUtils().ShEndPass();   

  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);        

  // Blurring amount blending
  static CCryName pTech1Name("BlurInterpolation");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTech1Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);     

  // Set PS default params  
  pParams= Vec4(0, 0, 0, fAmount * fAmount);
  static CCryName pParam1Name("psParams");
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParam1Name, &pParams, 1);

  GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[0], 0);    
  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 1, FILTER_POINT);     
  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 

  GetUtils().ShEndPass();   

  PROFILE_LABEL_POP( "RADIAL_BLURRING" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CFilterDepthEnhancement::Render()
{
  float fAmount = m_pAmount->GetParam();
  fAmount = clamp_tpl<float>(fAmount, 0.0f, 1.0f);

  static CCryName pTechName("DepthEnhancement");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);     

  // Set PS default params  
  Vec4 pParams= Vec4(0, 0, 0, 1.0f);
  static CCryName pParamName("psParams");
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);     
  GetUtils().SetTexture(CTexture::s_ptexZTarget, 1, FILTER_POINT);     
  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 

  GetUtils().ShEndPass();   
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CFilterChromaShift::Render()
{
  PROFILE_LABEL_PUSH( "CHROMA_SHIFT" );

  float fAmount = m_pAmount->GetParam();
  fAmount = max(fAmount, 0.0f);

  float fUserAmount = m_pUserAmount->GetParam();
  fUserAmount = max(fUserAmount, 0.0f);

  static CCryName pTechName("ChromaShift");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);     

  // Set PS default params  
  Vec4 pParams= Vec4(1.0f, 0.5f, 0.1f, 1) * (fAmount + fUserAmount) * 0.25f;
  static CCryName pParamName("psParams");
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0);       
  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 

  GetUtils().ShEndPass();   
  
  PROFILE_LABEL_POP( "CHROMA_SHIFT" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CFilterGrain::Render()
{
  float fAmount = m_pAmount->GetParam();
  fAmount = max(fAmount, 0.0f);

  static CCryName pTechName("GrainFilter");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);     

  // Set PS default params     
  Vec4 pParams= Vec4(cry_rand()%1024, cry_rand()%1024, fAmount, fAmount);
  static CCryName pParamName("psParams");
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 

  GetUtils().ShEndPass();   
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CFilterStylized::Render()
{
  float fAmount = m_pAmount->GetParam();
  fAmount = max(fAmount, 0.0f);

  GetUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[0]);    
  GetUtils().TexBlurGaussian(CTexture::s_ptexBackBufferScaled[0], 1, 1.0f, 1, false);             

  static CCryName pTechName("StylizedFilter");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);     

  // Set PS default params     
  Vec4 pParams= Vec4(cry_rand()%1024, cry_rand()%1024, fAmount, fAmount);
  static CCryName pParamName("psParams");
  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName, &pParams, 1);

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 

  GetUtils().ShEndPass();   
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CColorGrading::UpdateParams( SColorGradingMergeParams &pMergeParams )
{
	float fSharpenAmount = max(m_pSharpenAmount->GetParam(), 0.0f);

	// add cvar color_grading/color_grading_levels/color_grading_selectivecolor/color_grading_filters

	// Clamp to same Photoshop min/max values
	float fMinInput = clamp_tpl<float>(m_pMinInput->GetParam(), 0.0f, 255.0f);
	float fGammaInput = clamp_tpl<float>(m_pGammaInput->GetParam(), 0.0f, 10.0f);;
	float fMaxInput = clamp_tpl<float>(m_pMaxInput->GetParam(), 0.0f, 255.0f);
	float fMinOutput = clamp_tpl<float>(m_pMinOutput->GetParam(), 0.0f, 255.0f);

	float fMaxOutput = clamp_tpl<float>(m_pMaxOutput->GetParam(), 0.0f, 255.0f);

	float fBrightness = m_pBrightness->GetParam();
	float fContrast = m_pContrast->GetParam();
	float fSaturation = m_pSaturation->GetParam() + m_pSaturationOffset->GetParam();
	Vec4  pFilterColor = m_pPhotoFilterColor->GetParamVec4() + m_pPhotoFilterColorOffset->GetParamVec4();
	float fFilterColorDensity = clamp_tpl<float>(m_pPhotoFilterColorDensity->GetParam() + m_pPhotoFilterColorDensityOffset->GetParam(), 0.0f, 1.0f);
	float fGrain = min(m_pGrainAmount->GetParam()+m_pGrainAmountOffset->GetParam(), 1.0f);

	Vec4 pSelectiveColor = m_pSelectiveColor->GetParamVec4();
	float fSelectiveColorCyans = clamp_tpl<float>(m_pSelectiveColorCyans->GetParam()*0.01f, -1.0f, 1.0f);
	float fSelectiveColorMagentas = clamp_tpl<float>(m_pSelectiveColorMagentas->GetParam()*0.01f, -1.0f, 1.0f);
	float fSelectiveColorYellows = clamp_tpl<float>(m_pSelectiveColorYellows->GetParam()*0.01f, -1.0f, 1.0f);
	float fSelectiveColorBlacks = clamp_tpl<float>(m_pSelectiveColorBlacks->GetParam()*0.01f, -1.0f, 1.0f);

	// Saturation matrix
	Matrix44 pSaturationMat;      
	{
		float y=0.3086f, u=0.6094f, v=0.0820f, s=clamp_tpl<float>(fSaturation, -1.0f, 100.0f);  

		float a = (1.0f-s)*y + s;
		float b = (1.0f-s)*y;
		float c = (1.0f-s)*y;
		float d = (1.0f-s)*u;
		float e = (1.0f-s)*u + s;
		float f = (1.0f-s)*u;
		float g = (1.0f-s)*v;
		float h = (1.0f-s)*v;
		float i = (1.0f-s)*v + s;

		pSaturationMat.SetIdentity();
		pSaturationMat.SetRow(0, Vec3(a, d, g));  
		pSaturationMat.SetRow(1, Vec3(b, e, h));
		pSaturationMat.SetRow(2, Vec3(c, f, i));                                     
	}

	//  Brightness matrix
	Matrix44 pBrightMat;
	fBrightness=clamp_tpl<float>(fBrightness, 0.0f, 100.0f);    
	pBrightMat.SetIdentity();
	pBrightMat.SetRow(0, Vec3(fBrightness, 0, 0)); 
	pBrightMat.SetRow(1, Vec3(0, fBrightness, 0));
	pBrightMat.SetRow(2, Vec3(0, 0, fBrightness));            

	// Create Contrast matrix
	Matrix44 pContrastMat;
	{
		float c=clamp_tpl<float>(fContrast, -1.0f, 100.0f);  
		pContrastMat.SetIdentity();
		pContrastMat.SetRow(0, Vec3(c, 0, 0));  
		pContrastMat.SetRow(1, Vec3(0, c, 0));
		pContrastMat.SetRow(2, Vec3(0, 0, c));              
		pContrastMat.SetColumn(3, 0.5f*Vec3(1.0f-c, 1.0f-c, 1.0f-c));  
	}

	// Compose final color matrix and set fragment program constants
	Matrix44 pColorMat = pSaturationMat * (pBrightMat * pContrastMat);      

	Vec4 pParams0 = Vec4(fMinInput, fGammaInput, fMaxInput, fMinOutput);
	Vec4 pParams1 = Vec4(fMaxOutput, fGrain, cry_rand()%1024, cry_rand()%1024);
	Vec4 pParams2 = Vec4(pFilterColor.x, pFilterColor.y, pFilterColor.z, fFilterColorDensity);
	Vec4 pParams3 = Vec4(pSelectiveColor.x, pSelectiveColor.y, pSelectiveColor.z, fSharpenAmount + 1.0f);
	Vec4 pParams4 = Vec4(fSelectiveColorCyans, fSelectiveColorMagentas, fSelectiveColorYellows, fSelectiveColorBlacks);    

	// Enable corresponding shader variation
	pMergeParams.nFlagsShaderRT = gRenDev->m_RP.m_FlagsShader_RT;
	pMergeParams.nFlagsShaderRT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]|g_HWSR_MaskBit[HWSR_SAMPLE3]|g_HWSR_MaskBit[HWSR_SAMPLE4]);

#if !defined(XENON) && !defined(PS3)
	if( CRenderer::CV_r_colorgrading_levels && (fMinInput || fGammaInput || fMaxInput || fMinOutput ||fMaxOutput) )
		pMergeParams.nFlagsShaderRT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
#endif

	if( CRenderer::CV_r_colorgrading_filters && (fFilterColorDensity || fGrain || fSharpenAmount))
	{
		if( fFilterColorDensity )
			pMergeParams.nFlagsShaderRT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
		if( fGrain || fSharpenAmount )
			pMergeParams.nFlagsShaderRT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
	}

	if( CRenderer::CV_r_colorgrading_selectivecolor && (fSelectiveColorCyans || fSelectiveColorMagentas || fSelectiveColorYellows || fSelectiveColorBlacks))
		pMergeParams.nFlagsShaderRT |= g_HWSR_MaskBit[HWSR_SAMPLE2];


	if (CRenderer::CV_r_colorgrading_charts && CRenderer::CV_r_colorgrading == 2) 
		pMergeParams.nFlagsShaderRT |= g_HWSR_MaskBit[HWSR_SAMPLE3]; 


	Matrix44 pColorMatFromUserAndTOD = GetUtils().GetColorMatrix();
	pColorMat = pColorMat  * pColorMatFromUserAndTOD;
	
	Vec4 pColorMatrix[3]=
	{
		Vec4(pColorMat.m00, pColorMat.m01, pColorMat.m02, pColorMat.m03),
		Vec4(pColorMat.m10, pColorMat.m11, pColorMat.m12, pColorMat.m13), 
		Vec4(pColorMat.m20, pColorMat.m21, pColorMat.m22, pColorMat.m23),
	};

	pMergeParams.pColorMatrix[0] = pColorMatrix[0];
	pMergeParams.pColorMatrix[1] = pColorMatrix[1];
	pMergeParams.pColorMatrix[2] = pColorMatrix[2];
	pMergeParams.pLevels[0] = pParams0;
	pMergeParams.pLevels[1] = pParams1;
	pMergeParams.pFilterColor = pParams2;
	pMergeParams.pSelectiveColor[0] = pParams3;
	pMergeParams.pSelectiveColor[1] = pParams4;

	if (gcpRendD3D->m_pColorGradingControllerD3D)
	{  

		if( CRenderer::CV_r_colorgrading == 2)
		{
			gcpRendD3D->m_pColorGradingControllerD3D->Update(&pMergeParams);
		}
		else

			gcpRendD3D->m_pColorGradingControllerD3D->Update();    
	}

	// If using merged color grading with color chart disable regular color transformations in display - only need to use color chart


	if( CRenderer::CV_r_colorgrading == 2)
		pMergeParams.nFlagsShaderRT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE2]|g_HWSR_MaskBit[HWSR_SAMPLE4]);


}

void CColorGrading::Render()
{
  PROFILE_LABEL_PUSH( "COLOR_GRADING" );

  float fSharpenAmount = max(m_pSharpenAmount->GetParam(), 0.0f);

	// Enable corresponding shader variation
	uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
	gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]|g_HWSR_MaskBit[HWSR_SAMPLE3]|g_HWSR_MaskBit[HWSR_SAMPLE4]);

	SColorGradingMergeParams pMergeParams;
	UpdateParams( pMergeParams );

	gRenDev->m_RP.m_FlagsShader_RT |= pMergeParams.nFlagsShaderRT;

	Vec4 pColorMatrix[3];
	pColorMatrix[0] = pMergeParams.pColorMatrix[0];
	pColorMatrix[1] = pMergeParams.pColorMatrix[1];
	pColorMatrix[2] = pMergeParams.pColorMatrix[2];

	Vec4 pParams0 = pMergeParams.pLevels[0];
	Vec4 pParams1 = pMergeParams.pLevels[1];
	Vec4 pParams2 = pMergeParams.pFilterColor;
	Vec4 pParams3 = pMergeParams.pSelectiveColor[0];
	Vec4 pParams4 = pMergeParams.pSelectiveColor[1];

#if !defined(XENON) && !defined(PS3)  
  if( fSharpenAmount && CRenderer::CV_r_colorgrading_filters )
    GetUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[0]); 
#endif

  static CCryName pTechName("ColorGrading");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);     

  // Set PS default params
  static CCryName pParamName0("ColorGradingParams0");
  static CCryName pParamName1("ColorGradingParams1");
  static CCryName pParamName2("ColorGradingParams2");
  static CCryName pParamName3("ColorGradingParams3");
  static CCryName pParamName4("ColorGradingParams4");
  static CCryName pParamName5("ColorGradingParams5");
  static CCryName pParamMatrix("mColorGradingMatrix");

  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName0, &pParams0, 1);
  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName1, &pParams1, 1);
  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName2, &pParams2, 1);
  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName3, &pParams3, 1);
  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName4, &pParams4, 1);
  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamMatrix, pColorMatrix, 3);

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 

  GetUtils().ShEndPass();

  gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;

  PROFILE_LABEL_POP( "COLOR_GRADING" );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

//void CSSDisplacement::Render()
//{
//  gcpRendD3D->Set2DMode(false, 1, 1);           
//  
//  Matrix44  pView = gcpRendD3D->m_CameraMatrix;
//  pView.Transpose();
//
//  Matrix44 pViewProj = gcpRendD3D->m_CameraProjMatrix;
//  pViewProj.Transpose();
//
//  Matrix44 pViewProjI = gcpRendD3D->m_CameraProjMatrix;
//  pViewProjI.Invert();  
//  pViewProjI.Transpose();  
//
//  gcpRendD3D->Set2DMode(true, 1, 1);           
//
//  float fAmount = m_pAmount->GetParam();
//  fAmount = max(fAmount, 0.0f);
//
//  static CCryName pTechName("SSDM");
//  GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETTEXTURES|FEF_DONTSETSTATES);   
//
//  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);     
//
//  Vec4 pParams= Vec4(fAmount, fAmount, fAmount, fAmount);
//  static CCryName pParamName("psParams");
//  CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParamName, &pParams, 1);
//
//  CCryName pParamViewProjI = "g_mViewProjI";
//  CShaderMan::m_shDeferredShading->FXSetPSFloat(pParamViewProjI, (Vec4*) pViewProjI.GetData(), 4);       
//
//  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0);       
//  GetUtils().SetTexture(CTexture::s_ptexSceneNormalsMap, 1);       
//  GetUtils().SetTexture(CTexture::s_ptexZTarget, 2, FILTER_POINT);       
//
//  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 
//
//  GetUtils().ShEndPass();   
//
//}

bool CPostMSAA::Preprocess()
{
	if( !CRenderer::CV_r_PostMSAA )
	{
		m_pPrevView = GetUtils().m_pView;
		m_pPrevViewProj = GetUtils().m_pViewProj;
		return false;
	}

	return true;
}

void CPostMSAA::Render()
{
  PROFILE_LABEL_PUSH( "POST_MSAA" );

  PROFILE_SHADER_START

  gcpRendD3D->Set2DMode(false, 1, 1);           
	gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());        
  
  Matrix44  pView = gcpRendD3D->m_CameraMatrix;
  pView.Transpose();

  Matrix44 pViewProj = gcpRendD3D->m_CameraProjMatrix;
  pViewProj.Transpose();

  Matrix44 pViewProjI = gcpRendD3D->m_CameraProjMatrix;
  pViewProjI.Invert();  
  pViewProjI.Transpose();  
	gcpRendD3D->Set2DMode(true, 1, 1);           




  static CCryName TechName("PostMSAA");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                    

  gcpRendD3D->SetCullMode( R_CULL_NONE );
  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);//|GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);   

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
	float fAlpha = iszero(fCurrFrameTime) ? 0.0f : fExposureTime /fCurrFrameTime ;

  static CCryName pPostMsaaParams("PostMsaaParams");
	int nCurrID = gcpRendD3D->GetFrameID()%(CRenderer::CV_r_PostMSAA==1? 2 : 4); // select 2x msaa or 4x msaa
  Vec4 pParam = Vec4(fAlpha,1,nCurrID>0, CRenderer::CV_r_PostMSAA);
	
  CShaderMan::m_shPostEffects->FXSetPSFloat(pPostMsaaParams, &pParam, 1);

	static CCryName pParam0Name("mViewProj");
	CShaderMan::m_shPostEffects->FXSetPSFloat(pParam0Name, (Vec4 *)pViewProj.GetData(), 4);     

	static CCryName pParam1Name("mViewProjPrev");
	CShaderMan::m_shPostEffects->FXSetPSFloat(pParam1Name, (Vec4 *)m_pPrevViewProj.GetData(), 4);    
 
  GetUtils().SetTexture(CTexture::s_ptexPrevBackBuffer, 0, FILTER_LINEAR);        
	GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 1, FILTER_POINT);        
	GetUtils().SetTexture(CTexture::s_ptexZTarget, 2, FILTER_POINT);
	GetUtils().SetTexture(CTexture::s_ptexPrevZTarget, 3, FILTER_POINT);
	SD3DPostEffectsUtils::DrawScreenQuadWPOS(0, 0, CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(),
		0, 
		CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

  //GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);

  GetUtils().ShEndPass();                    

  GetUtils().CopyScreenToTexture(CTexture::s_ptexPrevBackBuffer);

#if !defined(XENON)
	GetUtils().StretchRect(CTexture::s_ptexZTarget, CTexture::s_ptexPrevZTarget);
#endif

  gcpRendD3D->FX_Flush();
	m_pPrevView = GetUtils().m_pView;
	m_pPrevViewProj = GetUtils().m_pViewProj;


 // gcpRendD3D->Set2DMode(true, 1, 1);           

  PROFILE_SHADER_END

  PROFILE_LABEL_POP( "POST_MSAA" );
}

void CPostStereo::Render()
{
	PROFILE_LABEL_PUSH( "POST_STEREO" );

	PROFILE_SHADER_START

 GetUtils().TexBlurGaussian(CTexture::s_ptexZTargetScaled, 1, 1.0f, 2, false);             

	static CCryName TechName("PostStereo");
	GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                    

	gcpRendD3D->SetCullMode( R_CULL_NONE );
	gcpRendD3D->EF_SetState(GS_NODEPTHTEST);   

	GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);        
	GetUtils().SetTexture(CTexture::s_ptexZTargetScaled, 1, FILTER_LINEAR);        
	GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);

	GetUtils().ShEndPass();                    

	gcpRendD3D->FX_Flush();
	PROFILE_SHADER_END

		PROFILE_LABEL_POP( "POST_STEREO" );
}