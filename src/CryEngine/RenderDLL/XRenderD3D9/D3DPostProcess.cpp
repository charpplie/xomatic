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

SD3DPostEffectsUtils SD3DPostEffectsUtils::m_pInstance;

SDynTexture* SPostEffectsUtils::tempRainTexture = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

SD3DSurface *SD3DPostEffectsUtils::GetDepthSurface( CTexture *pTex )
{
  if( pTex->GetFlags() & FT_USAGE_FSAA && gRenDev->m_RP.m_FSAAData.Type)
    return &gcpRendD3D->m_DepthBufferOrigFSAA;

  return &gcpRendD3D->m_DepthBufferOrig;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::ResolveRT( CTexture *&pDst )
{    
  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

#if defined (DIRECT3D9) || defined (OPENGL)
  D3DSurface *pBackSurface= gcpRendD3D->m_pNewTarget[0]->m_pTarget;
  D3DSurface *pTexSurf= pDst->GetSurface(0, 0);



























  RECT pSrcRect={0, 0, iWidth, iHeight };
  RECT pDstRect={0, 0, pDst->GetWidth(), pDst->GetHeight() };

  gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_RTCopied++; 
  gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_RTCopiedSize += pDst->GetDeviceDataSize();
  gcpRendD3D->m_pd3dDevice->StretchRect(pBackSurface, &pSrcRect, pTexSurf, &pDstRect, D3DTEXF_NONE); 



  SAFE_RELEASE(pTexSurf);

#elif defined (DIRECT3D10)

  CDeviceTexture *pDstResource = pDst->GetDevTexture();        
  ID3D11RenderTargetView* pOrigRT = gcpRendD3D->m_pNewTarget[0]->m_pTarget;
  if (pOrigRT && pDstResource)
  {
    D3D11_BOX box;
    ZeroStruct(box);
    box.right = pDst->GetWidth(); //iWidth;
    box.bottom = pDst->GetHeight(); //iHeight;
    box.back = 1;





    ID3D11Resource *pSrcResource;
    pOrigRT->GetResource( &pSrcResource );


    HRESULT hr = 0;
    gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_RTCopied++; 
    gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_RTCopiedSize += pDst->GetDeviceDataSize();

    gcpRendD3D->m_pd3dDeviceContext->CopySubresourceRegion(pDstResource->Get2DTexture(), 0, 0, 0, 0, pSrcResource, 0, &box);      
    SAFE_RELEASE(pSrcResource);
  }

#endif    
}

void SD3DPostEffectsUtils::CopyScreenToTexture(CTexture *&pDst)
{
  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  ResolveRT( pDst );
}

























////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::StretchRect(CTexture *pSrc, CTexture *&pDst) 
{
  if(!pSrc || !pDst) 
  {
    return;
  }

  PROFILE_LABEL_PUSH( "STRETCHRECT_EMU" );

  PROFILE_SHADER_START

  uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
  gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);

  // Get current viewport
  int iTempX, iTempY, iWidth, iHeight;
  gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
  bool bResample=0;

  if(pSrc->GetWidth()!=pDst->GetWidth() && pSrc->GetHeight()!=pDst->GetHeight())
  {
    bResample = 1;
  }

  gcpRendD3D->FX_PushRenderTarget(0, pDst, NULL); 
  gcpRendD3D->RT_SetViewport(0, 0, pDst->GetWidth(), pDst->GetHeight());     







  if(!bResample)
  {
    static CCryName pTechName("TextureToTexture");                 
    ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
  }
  else
  { 
    static CCryName pTechName("TextureToTextureResampled");     
    ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
  }

  gRenDev->EF_SetState(GS_NODEPTHTEST);   

  // Get sample size ratio (based on empirical "best look" approach)
  float fSampleSize = ((float)pSrc->GetWidth()/(float)pDst->GetWidth()) * 0.5f;

  // Set samples position
  float s1 = fSampleSize / (float) pSrc->GetWidth();  // 2.0 better results on lower res images resizing        
  float t1 = fSampleSize / (float) pSrc->GetHeight();       

  //	float s1 = 0.5f / (float) pSrc->GetWidth();  // 2.0 better results on lower res images resizing        
  //	float t1 = 0.5f / (float) pSrc->GetHeight();       

  // Use box filtering
  //Vec4 pParams0=Vec4(-s1, -t1, s1, -t1); 
  //Vec4 pParams1=Vec4(s1, t1, -s1, t1);    

  // Use rotated grid
  Vec4 pParams0=Vec4(s1*0.95f, t1*0.25f, -s1*0.25f, t1*0.96f); 
  Vec4 pParams1=Vec4(-s1*0.96f, -t1*0.25f, s1*0.25f, -t1*0.96f);  

  static CCryName pParam0Name("texToTexParams0");
  static CCryName pParam1Name("texToTexParams1");

  CShaderMan::m_shPostEffects->FXSetPSFloat(pParam0Name, &pParams0, 1);        
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParam1Name, &pParams1, 1); 


  SetTexture(pSrc, 0, bResample?FILTER_LINEAR: FILTER_POINT, 1);//, (pSrc->GetFlags()&FT_USAGE_ALLOWREADSRGB) && !(pDst->GetFlags()&FT_USAGE_ALLOWREADSRGB));

  DrawFullScreenQuad(pDst->GetWidth(), pDst->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);

  ShEndPass();

  // Restore previous viewport
  gcpRendD3D->FX_PopRenderTarget(0);
  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);        

  gcpRendD3D->FX_Flush();
  PROFILE_SHADER_END    

    PROFILE_LABEL_POP( "STRETCHRECT_EMU" );

  gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::TexBlurIterative(CTexture *pTex, int nIterationsMul)
{
  if(!pTex)
    return;

  SDynTexture *tpBlurTemp = new SDynTexture(pTex->GetWidth(), pTex->GetHeight(), pTex->GetDstFormat(), eTT_2D, FT_STATE_CLAMP | FT_USAGE_RENDERTARGET, "TempBlurRT");
  tpBlurTemp->Update( pTex->GetWidth(), pTex->GetHeight() );

  if( !tpBlurTemp->m_pTexture)
  {
    SAFE_DELETE(tpBlurTemp);
    return;
  }

  PROFILE_LABEL_PUSH( "TEXBLUR_16TAPS" );

  PROFILE_SHADER_START

	// Set same EDRAM offsets
	tpBlurTemp->m_pTexture->SetRenderTargetTile( pTex->GetRenderTargetTile() );

  // Get current viewport
  int iTempX, iTempY, iWidth, iHeight;
  gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

	// Iterative blur (aka Kawase): 4 taps, 16 taps, 64 taps, 256 taps, etc
	for(int i= 1; i <= nIterationsMul; ++i)
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 1st iteration (4 taps)

		gcpRendD3D->FX_PushRenderTarget(0, tpBlurTemp->m_pTexture, NULL);
		gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight()); 

		// only regular gaussian blur supporting masking
		static CCryName pTechName("Blur4Taps");

		uint32 nPasses;
		CShaderMan::m_shPostEffects->FXSetTechnique( pTechName );
		CShaderMan::m_shPostEffects->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

		gRenDev->EF_SetState(GS_NODEPTHTEST);   

		// setup texture offsets, for texture sampling
		// Get sample size ratio (based on empirical "best look" approach)
		float fSampleSize = 0.5f * ((float) i);//((float)pTex->GetWidth()/(float)pTex->GetWidth()) * 0.5f;

		// Set samples position
		float s1 = fSampleSize / (float) pTex->GetWidth();  // 2.0 better results on lower res images resizing        
		float t1 = fSampleSize / (float) pTex->GetHeight();       

		// Use rotated grid
		Vec4 pParams0=Vec4(-s1, -t1, s1, -t1); 
		Vec4 pParams1=Vec4(s1, t1, -s1, t1);    
		//Vec4 pParams0=Vec4(s1*0.95f, t1*0.25f, -s1*0.25f, t1*0.96f); 
		//Vec4 pParams1=Vec4(-s1*0.96f, -t1*0.25f, s1*0.25f, -t1*0.96f);  
		Vec4 pParams2=Vec4(0.0f, 0.0f, 0.0f, 0.2f);  

		static CCryName pParam0Name("texToTexParams0");
		static CCryName pParam1Name("texToTexParams1");
		CCryName pParam2Name("blurParams0");

		CShaderMan::m_shPostEffects->FXSetPSFloat(pParam0Name, &pParams0, 1);        
		CShaderMan::m_shPostEffects->FXSetPSFloat(pParam1Name, &pParams1, 1); 
		CShaderMan::m_shPostEffects->FXSetPSFloat(pParam2Name, &pParams2, 1); 

		CShaderMan::m_shPostEffects->FXBeginPass(0);

		SetTexture(pTex, 0, FILTER_LINEAR);

		DrawFullScreenQuad(pTex->GetWidth(), pTex->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);

		CShaderMan::m_shPostEffects->FXEndPass();
		CShaderMan::m_shPostEffects->FXEnd(); 

		gcpRendD3D->FX_PopRenderTarget(0);

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 2nd iteration (4 x 4 taps)

		gcpRendD3D->FX_PushRenderTarget(0, pTex, NULL);
		gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight()); 

		CShaderMan::m_shPostEffects->FXSetTechnique( pTechName );
		CShaderMan::m_shPostEffects->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

		gRenDev->EF_SetState(GS_NODEPTHTEST);   
		// increase kernel size for second iteration
		fSampleSize = 0.5f * 2.0f * ((float) i);
		// Set samples position
		s1 = fSampleSize / (float) pTex->GetWidth();  // 2.0 better results on lower res images resizing        
		t1 = fSampleSize / (float) pTex->GetHeight();       

		// Use rotated grid
		pParams0=Vec4(-s1, -t1, s1, -t1); 
		pParams1=Vec4(s1, t1, -s1, t1);    

		//pParams0=Vec4(s1*0.95f, t1*0.25f, -s1*0.25f, t1*0.96f); 
		//pParams1=Vec4(-s1*0.96f, -t1*0.25f, s1*0.25f, -t1*0.96f);  
		pParams2=Vec4(0.0f, 0.0f, 0.0f, 0.2f);  

		CShaderMan::m_shPostEffects->FXSetPSFloat(pParam0Name, &pParams0, 1);        
		CShaderMan::m_shPostEffects->FXSetPSFloat(pParam1Name, &pParams1, 1); 
		CShaderMan::m_shPostEffects->FXSetPSFloat(pParam2Name, &pParams2, 1); 

		CShaderMan::m_shPostEffects->FXBeginPass(0);

		SetTexture(tpBlurTemp->m_pTexture, 0, FILTER_LINEAR);

		DrawFullScreenQuad(pTex->GetWidth(), pTex->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);

		CShaderMan::m_shPostEffects->FXEndPass();
		CShaderMan::m_shPostEffects->FXEnd(); 

		gcpRendD3D->FX_PopRenderTarget(0);
	}

  // Restore previous viewport
  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

	tpBlurTemp->m_pTexture->SetRenderTargetTile( 0 );

  gcpRendD3D->FX_Flush();
  PROFILE_SHADER_END      

  PROFILE_LABEL_POP( "TEXBLUR_16TAPS" );

  SAFE_DELETE(tpBlurTemp);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::TexBlurGaussian(CTexture *pTex, int nAmount, float fScale, float fDistribution, bool bAlphaOnly, CTexture *pMask, bool bSRGB )
{
  if(!pTex)
    return;

  uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
  gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);

  PROFILE_LABEL_PUSH( "TEXBLUR_GAUSSIAN" );

	if( bSRGB )
		gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

  SDynTexture *tpBlurTemp = new SDynTexture(pTex->GetWidth(), pTex->GetHeight(), pTex->GetDstFormat(), eTT_2D,  FT_STATE_CLAMP | FT_USAGE_RENDERTARGET, "TempBlurRT");
  tpBlurTemp->Update( pTex->GetWidth(), pTex->GetHeight() );

  if( !tpBlurTemp->m_pTexture)
  {
    SAFE_DELETE(tpBlurTemp);
    return;
  }

//#if defined(XENON)
//  pTex->SetRenderTargetTile(1);
//  tpBlurTemp->m_pTexture->SetRenderTargetTile(1);
//#endif

  PROFILE_SHADER_START

    // Get current viewport
    int iTempX, iTempY, iWidth, iHeight;
  gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
  gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());        

  Vec4 vWhite( 1.0f, 1.0f, 1.0f, 1.0f );

  // TODO: Make test with Martin's idea about the horizontal blur pass with vertical offset.

  // only regular gaussian blur supporting masking
  static CCryName pTechName("GaussBlurBilinear");
  static CCryName pTechNameMasked("MaskedGaussBlurBilinear");
  static CCryName pTechName1("GaussAlphaBlur");

  //ShBeginPass(CShaderMan::m_shPostEffects, , FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
  uint32 nPasses;
  CShaderMan::m_shPostEffects->FXSetTechnique((!bAlphaOnly)? ((pMask)?pTechNameMasked : pTechName) :pTechName1);
  CShaderMan::m_shPostEffects->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

  gRenDev->EF_SetState(GS_NODEPTHTEST);   

  // setup texture offsets, for texture sampling
  float s1 = 1.0f/(float) pTex->GetWidth();     
  float t1 = 1.0f/(float) pTex->GetHeight();    

  // Horizontal/Vertical pass params
  const int nSamples = 16;
  int nHalfSamples = (nSamples>>1);

  Vec4 pHParams[32], pVParams[32], pWeightsPS[32];
  float pWeights[32], fWeightSum = 0;

  memset( pWeights,0,sizeof(pWeights) );

  int s;
  for(s = 0; s<nSamples; ++s)
  {
    if(fDistribution != 0.0f)
      pWeights[s] = GaussianDistribution1D(s - nHalfSamples, fDistribution);      
    else
      pWeights[s] = 0.0f;
    fWeightSum += pWeights[s];
  }

  // normalize weights
  for(s = 0; s < nSamples; ++s)
  {
    pWeights[s] /= fWeightSum;  
  }

  // set bilinear offsets
  for(s = 0; s < nHalfSamples; ++s)
  {
    float off_a = pWeights[s*2];
    float off_b = ( (s*2+1) <= nSamples-1 )? pWeights[s*2+1] : 0;   
    float a_plus_b = (off_a + off_b);
    if (a_plus_b == 0)
      a_plus_b = 1.0f;
    float offset = off_b / a_plus_b;

    pWeights[s] = off_a + off_b;
    pWeights[s] *= fScale ;
    pWeightsPS[s] = vWhite * pWeights[s];

    float fCurrOffset = (float) s*2 + offset - nHalfSamples;
    pHParams[s] = Vec4(s1 * fCurrOffset , 0, 0, 0);  
    pVParams[s] = Vec4(0, t1 * fCurrOffset , 0, 0);       
  }


  STexState sTexState = STexState(FILTER_LINEAR, true);
  static CCryName pParam0Name("psWeights");
  static CCryName pParam1Name("PI_psOffsets");

  //SetTexture(pTex, 0, FILTER_LINEAR); 
  CShaderMan::m_shPostEffects->FXSetPSFloat(pParam0Name, pWeightsPS, nHalfSamples);  

  //for(int p(1); p<= nAmount; ++p)   
  {
    //Horizontal


    gcpRendD3D->FX_PushRenderTarget(0, tpBlurTemp->m_pTexture, NULL);
    gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());        

    // !force updating constants per-pass! (dx10..)
    CShaderMan::m_shPostEffects->FXBeginPass(0);

    pTex->Apply(0, CTexture::GetTexState(sTexState)); 
    if( pMask )
      pMask->Apply(1, CTexture::GetTexState(sTexState));
    CShaderMan::m_shPostEffects->FXSetVSFloat(pParam1Name, pHParams, nHalfSamples);                
    DrawFullScreenQuad(pTex->GetWidth(), pTex->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);

    CShaderMan::m_shPostEffects->FXEndPass();

    gcpRendD3D->FX_PopRenderTarget(0);

    //Vertical
    gcpRendD3D->FX_PushRenderTarget(0, pTex, NULL);
    gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());         

    // !force updating constants per-pass! (dx10..)
    CShaderMan::m_shPostEffects->FXBeginPass(0);

    CShaderMan::m_shPostEffects->FXSetVSFloat(pParam1Name, pVParams, nHalfSamples);
    tpBlurTemp->m_pTexture->Apply(0, CTexture::GetTexState(sTexState)); 
    if( pMask )
      pMask->Apply(1, CTexture::GetTexState(sTexState));
    DrawFullScreenQuad(pTex->GetWidth(), pTex->GetHeight(), CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);      

    CShaderMan::m_shPostEffects->FXEndPass();

    gcpRendD3D->FX_PopRenderTarget(0);
  }             

  CShaderMan::m_shPostEffects->FXEnd(); 

  //    ShEndPass( );

  // Restore previous viewport
  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

//#if defined(XENON)
//  pTex->SetRenderTargetTile(0);
//  tpBlurTemp->m_pTexture->SetRenderTargetTile(0);
//#endif

  //release dyntexture
  SAFE_DELETE(tpBlurTemp);

  gcpRendD3D->FX_Flush();
  PROFILE_SHADER_END      

    PROFILE_LABEL_POP( "TEXBLUR_GAUSSIAN" );

  gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}


void SD3DPostEffectsUtils::BilateralBlurring(CTexture *pSrc, CTexture *pDest)
{
	PROFILE_LABEL_PUSH( "BILATERAL_BLURRING" );

	uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;

	// Clear the sample flags
	gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);

	PROFILE_SHADER_START

	int destWidth = pDest->GetWidth(), 
			destHeight = pDest->GetHeight();

	// Get current viewport
	int iTempX, iTempY, iWidth, iHeight;
	gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

	gcpRendD3D->FX_PushRenderTarget(0, pDest, NULL);
	gcpRendD3D->RT_SetViewport(0, 0, destWidth, destHeight); 

	static CCryName techName("BilateralBlurring");
	uint32 nPasses;

	CShaderMan::m_shPostEffects->FXSetTechnique( techName );
	CShaderMan::m_shPostEffects->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

	gRenDev->EF_SetState(gRenDev->m_RP.m_CurState | GS_NODEPTHTEST);   

	// Setup shader constants
	int srcWidth = pSrc->GetWidth();
	int srcHeight = pSrc->GetHeight();

	bool resample = (destWidth > srcWidth || destHeight > srcHeight);

	static CCryName paramBlurOffset("BlurOffset");
	static CCryName paramBlurKernel("BlurKernel");
	Vec4 v;

#if HALF_PIXEL_SHIFT_NEEDED
	v[0] = 0.5f / destWidth; v[1] = 0.5f / destHeight;
#else
	v[0] = 0; v[1] = 0;
#endif
	v[2] = srcWidth; v[3] = srcHeight;
	CShaderMan::m_shPostEffects->FXSetVSFloat(paramBlurOffset, &v, 1);

	// X Blur
	v[0] = 1.0f / srcWidth; v[1] = 1.0f / srcHeight; v[2] = srcWidth; v[3] = srcHeight;
	CShaderMan::m_shPostEffects->FXSetPSFloat(paramBlurOffset, &v, 1);

	v[0] = 2.f / srcWidth; v[1] = 0; v[2] = 2.f / srcHeight; v[3] = 0;
	CShaderMan::m_shPostEffects->FXSetPSFloat(paramBlurKernel, &v, 1);

	if (resample)
	{
		// Size mismatch, the surface is probably upscaled
		if (CTexture::s_ptexZTargetScaled)
			gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
	}

	CShaderMan::m_shPostEffects->FXBeginPass(0);

	SetTexture(pSrc, 0, FILTER_LINEAR, 1, (pSrc->GetFlags() & FT_USAGE_ALLOWREADSRGB) != 0);
	SetTexture(CTexture::s_ptexZTarget, 1, FILTER_POINT);

	if (resample && CTexture::s_ptexZTargetScaled)
		SetTexture(CTexture::s_ptexZTargetScaled, 2, FILTER_POINT);

	DrawFullScreenQuad(destWidth, destHeight, CRenderer::CV_r_PostProcessScreenQuadTessX, CRenderer::CV_r_PostProcessScreenQuadTessY);

	CShaderMan::m_shPostEffects->FXEndPass();
	CShaderMan::m_shPostEffects->FXEnd(); 

	gcpRendD3D->FX_PopRenderTarget(0);	

	// Restore previous viewport
	gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

	gcpRendD3D->FX_Flush();
	PROFILE_SHADER_END      

	gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;

	PROFILE_LABEL_POP( "BILATERAL_BLURRING" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////












































////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::BeginStencilPrePass( bool bAddToStencil, bool bDebug )
{
  if( !bAddToStencil )
    gcpRendD3D->m_nStencilMaskRef++;		

  if (gcpRendD3D->m_nStencilMaskRef>255)
  {
    gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_STENCIL|FRT_CLEAR_IMMEDIATE, NULL);
    gcpRendD3D->m_nStencilMaskRef= 1;
  }

  gcpRendD3D->EF_SetStencilState(
    STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
    STENCOP_FAIL(FSS_STENCOP_REPLACE) |
    STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
    STENCOP_PASS(FSS_STENCOP_REPLACE),
    gcpRendD3D->m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFF
    );








  gcpRendD3D->EF_SetState(GS_STENCIL|GS_NODEPTHTEST|(!bDebug?GS_COLMASK_NONE:0));  
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::EndStencilPrePass()
{






}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::SetupStencilStates( int32 nStFunc )
{
  if( nStFunc < 0 )
  {



    return;
  }

  gcpRendD3D->EF_SetStencilState(
    STENC_FUNC(nStFunc) |
    STENCOP_FAIL(FSS_STENCOP_KEEP) |
    STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
    STENCOP_PASS(FSS_STENCOP_KEEP),
    gcpRendD3D->m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFFFFFF);







}

////////////////////////////////////////////////////////////////////////////////////////////////////





































/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CGammaReference::Render()
{  
  PROFILE_SHADER_START

    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
  gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);






  // Show just reference image 
  if( CRenderer::CV_r_showgammareference == 2 )   
  {
    gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    if( !m_pGammaReference )
    {
      m_pGammaReference = CTexture::ForName("Textures/Defaults/calibration.tif",  FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);      
      m_pGammaReference->SRGBRead( false );
    }
  }
  
  static CCryName TechName("GammaReference");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);                    

  uint32 nRenderState = GS_NODEPTHTEST;
  if( CRenderer::CV_r_showgammareference != 2 ) 
    nRenderState |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
  else
    GetUtils().SetTexture(m_pGammaReference, 0, FILTER_LINEAR, 1);

  gcpRendD3D->EF_SetState( nRenderState );

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

  GetUtils().ShEndPass();                    

  if( CRenderer::CV_r_showgammareference != 2 ) 
  {
    gcpRendD3D->Set2DMode(false, 1, 1);           
    SDrawTextInfo pDrawTexInfo;    

    pDrawTexInfo.color[0] = pDrawTexInfo.color[2] = 0.0f;
    pDrawTexInfo.color[1] = 1.0f;

    gcpRendD3D->Draw2dText(50, 10, " " , pDrawTexInfo); // workaround to avoid garbage - something broken with draw2Dtext
    gcpRendD3D->Draw2dText(50, 400, "Linear:", pDrawTexInfo);

    gcpRendD3D->Draw2dText(50, 520, "SRGB (2.2):", pDrawTexInfo);

    gcpRendD3D->Draw2dText(10, 10, "Please adjust your display brightness/contrast", pDrawTexInfo);  

    pDrawTexInfo.color[0] = 1;
    pDrawTexInfo.color[1] = pDrawTexInfo.color[2] = 0.0f;

    gcpRendD3D->Draw2dText(240, 100, "Middle square color should blend with background color", pDrawTexInfo);   
    gcpRendD3D->Draw2dText(260, 340, "Each individual shade should be clearly seen", pDrawTexInfo);

    gcpRendD3D->Set2DMode(true, 1, 1);           
  }

  gcpRendD3D->FX_Flush();
  PROFILE_SHADER_END

    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CD3D9Renderer::FX_PostProcessScene(bool bEnable)
{  
  if( !gcpRendD3D )
  {
    return false;
  }

  if(bEnable)
  {    
    //if(!CTexture::s_ptexBackBuffer)
    {      
      // Create all resources if required
      GetUtils().Create();
    }
  }
  else
  if (!CRenderer::CV_r_PostProcess && !CRenderer::CV_r_HDRRendering && CTexture::s_ptexBackBuffer)
  {
    GetUtils().Release();
  }
  
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// Temporary workaround for hdr post effects

void CD3D9Renderer::FX_PostProcessSceneHDR()
{
  if ( !gcpRendD3D || !CRenderer::CV_r_HDRRendering || !CTexture::s_ptexSceneTarget || !gRenDev->CV_r_PostProcess)
    return;

	//bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality( eRQ_VeryHigh, eSQ_High );
	//if( !bQualityCheck )
	//	return;

  if( PostEffectMgr().GetEffects().empty() || gcpRendD3D->GetWireframeMode() > R_SOLID_MODE) 
    return;

  if( !CShaderMan::m_shPostEffects || !CShaderMan::m_shPostEffectsGame )
    return;

  if( !CTexture::IsTextureExist(CTexture::s_ptexBackBuffer) )
    return;

  if( !CTexture::IsTextureExist(CTexture::s_ptexSceneTarget) ) 
    return;

  gcpRendD3D->Set2DMode(true, 1, 1);     

  GetUtils().m_pCurDepthSurface = &gcpRendD3D->m_DepthBufferOrig;

  // do camera motion blur
  CMotionBlur::m_pInstance.RenderHDR();

  // depth of field
  CDepthOfField::m_pInstance.RenderHDR();

  // masked blurring
  CFilterMaskedBlurring::m_pInstance.Render();

  gcpRendD3D->Set2DMode(false, 1, 1);     
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CREPostProcessData::Create() 
{  
  // Initialize all post process data
	Reset();
}

void CREPostProcessData::Release() 
{
	Reset();

  // Free all used resources
  GetUtils().Release();  
}

void CREPostProcessData:: Reset()
{  
  // Reset all post process data
  PostEffectMgr().Reset();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CPostEffectsMgr::Begin()
{   
  GetUtils().Log("### POST-PROCESSING BEGINS ### "); 
  GetUtils().m_pTimer=gEnv->pTimer;    

  static EShaderQuality nPrevShaderQuality = eSQ_Low;
  static ERenderQuality nPrevRenderQuality = eRQ_Low;

  EShaderQuality nShaderQuality = (EShaderQuality) gcpRendD3D->EF_GetShaderQuality(eST_PostProcess);
  ERenderQuality nRenderQuality = gRenDev->m_RP.m_eQuality;
  if( nPrevShaderQuality != nShaderQuality || nPrevRenderQuality != nRenderQuality )
  {
    CPostEffectsMgr::Reset();
    nPrevShaderQuality = nShaderQuality;
    nPrevRenderQuality = nRenderQuality;
  }

  gcpRendD3D->GetModelViewMatrix( GetUtils().m_pView.GetData() );
  gcpRendD3D->GetProjectionMatrix( GetUtils().m_pProj.GetData() );

  // Store some commonly used per-frame data

  SPostEffectsUtils::m_pViewProj = GetUtils().m_pView * GetUtils().m_pProj;
  SPostEffectsUtils::m_pViewProj.Transpose();
 
	// Avoid using ResetToDefault - it's reseting everything, including gpr's states - we leave this comment out for now
	// in case any problems
	//gcpRendD3D->ResetToDefault();

  gcpRendD3D->FX_ResetPipe();  

  gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_NO_SRGBWRITES;

  if( gRenDev->IsLinearSpaceShadingEnabled() /*&& !gRenDev->IsHDRModeEnabled()*/)
  {
#if !defined(XENON) && (defined (DIRECT3D9) || defined(OPENGL) || defined(PS3)) 
  #if defined( PS3 )
      gcpRendD3D->m_pd3dDeviceContext->RSSetState(0, 0);
  #else
      gcpRendD3D->m_pd3dDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
  #endif  


#endif
  }

  gcpRendD3D->Set2DMode(true, 1, 1);       
  SPostEffectsUtils::m_pCurDepthSurface = gcpRendD3D->FX_GetScreenDepthSurface();

  SPostEffectsUtils::m_iFrameCounter=(GetUtils().m_iFrameCounter+1)%1000;  

  gcpRendD3D->RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());

  SPostEffectsUtils::m_fWaterLevel = gRenDev->m_cEF.m_PF.vWaterLevel.y;
  
  if (gcpRendD3D->GetWireframeMode() > R_SOLID_MODE)
  {
#if defined (DIRECT3D9) || defined (OPENGL)
    gcpRendD3D->m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
#elif defined (DIRECT3D10)
    SStateRaster RS = gcpRendD3D->m_StatesRS[gcpRendD3D->m_nCurStateRS];
    RS.Desc.FillMode = D3D11_FILL_SOLID;
    gcpRendD3D->SetRasterState(&RS);
#endif
  }

#if defined (DIRECT3D10) && !defined(PS3)
  // Due to D3D10 srgb limitations on implementation (cannot create typeless swap chain to allow casting between srgb/non-srgb render target view) 
  //we'll need extra render target when using srgb 
  if( gRenDev->IsLinearSpaceShadingEnabled() )
  {
    GetUtils().CopyScreenToTexture(CTexture::s_ptexBackBufferTemp);
    gcpRendD3D->FX_PushRenderTarget(0,  CTexture::s_ptexBackBufferTemp,  NULL); 
  }
#endif
  
}

void CPostEffectsMgr::End()
{
#if defined (DIRECT3D10) && !defined(PS3)
  if( gRenDev->IsLinearSpaceShadingEnabled() /*&& !gRenDev->IsHDRModeEnabled()*/)
  {
    gcpRendD3D->FX_PopRenderTarget(0);

    static CCryName pTechName("FinalPassSRGBCorrection");                 
    GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    gRenDev->EF_SetState(GS_NODEPTHTEST);   
    GetUtils().SetTexture(CTexture::s_ptexBackBufferTemp, 0, FILTER_POINT, 1, true); 
    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());      
    GetUtils().ShEndPass();
  }
#endif

  gcpRendD3D->Set2DMode(false, 1, 1);

  gcpRendD3D->RT_SetViewport(GetUtils().m_pScreenRect.left, GetUtils().m_pScreenRect.top, GetUtils().m_pScreenRect.right, GetUtils().m_pScreenRect.bottom); 

	// Avoid using ResetToDefault - it's reseting everything, including gpr's states - we leave this comment out for now
	// in case any problems
	//gcpRendD3D->ResetToDefault();

  gcpRendD3D->FX_ResetPipe();  

  gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags2 &= ~RBPF2_NO_SRGBWRITES;  
  gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_POSTPROCESS;

  if( gRenDev->IsLinearSpaceShadingEnabled() /*&& !gRenDev->IsHDRModeEnabled()*/)
  {
#if !defined(XENON) && (defined (DIRECT3D9) || defined(OPENGL) || defined(PS3)) 
  #if defined( PS3 )
    gcpRendD3D->m_pd3dDeviceContext->RSSetState(0, 1);
  #else
    gcpRendD3D->m_pd3dDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE); 
  #endif  


#endif
  }


  if(gcpRendD3D->GetWireframeMode() == R_WIREFRAME_MODE)
  {
#if defined (DIRECT3D9) || defined (OPENGL)
    gcpRendD3D->m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
#elif defined (DIRECT3D10)
    SStateRaster RS = gcpRendD3D->m_StatesRS[gcpRendD3D->m_nCurStateRS];
    RS.Desc.FillMode = D3D11_FILL_WIREFRAME;
    gcpRendD3D->SetRasterState(&RS);
#endif
  }
	else
  if(gcpRendD3D->GetWireframeMode() == R_POINT_MODE)
  {
#if defined (DIRECT3D9) || defined (OPENGL)
    gcpRendD3D->m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_POINT);




#endif
  }

  PostEffectMgr().ResetLights();

  GetUtils().Log("### POST-PROCESSING ENDS ### ");   
}

 
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CREPostProcess:: mfDraw(CShader *ef, SShaderPass *sfm)
{ 
  if( !gcpRendD3D || !gRenDev->CV_r_PostProcess || PostEffectMgr().GetEffects().empty() || gcpRendD3D->GetWireframeMode() > R_SOLID_MODE) 
    return 0;

  if( !CShaderMan::m_shPostEffects || !CShaderMan::m_shPostEffectsGame )
    return 0;

  if( !CTexture::IsTextureExist(CTexture::s_ptexBackBuffer) )
    return 0;
     
  if( !CTexture::IsTextureExist(CTexture::s_ptexSceneTarget) ) 
    return 0;

  PROFILE_LABEL_PUSH( "POST EFFECTS" );





  PostEffectMgr().Begin();   

  gcpRendD3D->FX_ApplyShaderQuality(eST_PostProcess);

  for(CPostEffectItor pItor = PostEffectMgr().GetEffects().begin(), pItorEnd = PostEffectMgr().GetEffects().end(); pItor != pItorEnd; ++pItor)
  {
    CPostEffect *pCurrEffect = (*pItor);
    if( pCurrEffect->GetRenderFlags() & PSP_REQUIRES_UPDATE )
      pCurrEffect->Update();
  }

	CPostEffectVec& activeEffects = PostEffectMgr().GetActiveEffects(gcpRendD3D->m_RP.m_nProcessThreadID);
	activeEffects.clear();

  for(CPostEffectItor pItor = PostEffectMgr().GetEffects().begin(), pItorEnd = PostEffectMgr().GetEffects().end(); pItor != pItorEnd; ++pItor)
  {
    CPostEffect *pCurrEffect = (*pItor);
    if(pCurrEffect->Preprocess())  
    {
      if( pCurrEffect->GetRenderFlags() & PSP_UPDATE_BACKBUFFER )
        GetUtils().CopyScreenToTexture(CTexture::s_ptexBackBuffer); 

       activeEffects.push_back( pCurrEffect );

      pCurrEffect->Render();
    }
  }

  // Output active post effects
  if( gRenDev->CV_r_PostProcess >= 2 && !activeEffects.empty() )
  {               
    CPostEffectItor pNameItor;        
    CPostEffectItor pNameEndItor = activeEffects.end();        

    // current output line position
    int nPosY = 20;  
    gcpRendD3D->Set2DMode(false, 1, 1);           
    SDrawTextInfo pDrawTexInfo;    
    
    pDrawTexInfo.color[0] = pDrawTexInfo.color[2] = 0.0f;
    pDrawTexInfo.color[1] = 1.0f;

    gcpRendD3D->Draw2dText(30, nPosY, " " , pDrawTexInfo); // workaround to avoid garbage - someting broken with draw2Dtext
    gcpRendD3D->Draw2dText(30, nPosY, "Active post effects:", pDrawTexInfo);
    nPosY += 15;

    pDrawTexInfo.color[0] = pDrawTexInfo.color[1] = pDrawTexInfo.color[2] = 1.0f;

    for(pNameItor = activeEffects.begin(); pNameItor != pNameEndItor; ++pNameItor) 
    {
			CPostEffect* pEffect = *pNameItor;

      // Output active post effects
      gcpRendD3D->Draw2dText(30, nPosY, pEffect->GetName(), pDrawTexInfo);
      nPosY += 10;
    }

		if( CRenderer::CV_r_SSAO )
		{
			gcpRendD3D->Draw2dText(30, nPosY, "SSAO", pDrawTexInfo);
		}

    gcpRendD3D->Set2DMode(true, 1, 1);           
  }

  if( gRenDev->CV_r_PostProcess == 3  )
  {
    StringEffectMap *pEffectsParamsUpdated = PostEffectMgr().GetDebugParamsUsedInFrame();
    if( pEffectsParamsUpdated && !pEffectsParamsUpdated->empty())
    {
      StringEffectMapItor pItor = pEffectsParamsUpdated->begin();
      StringEffectMapItor pEnd = pEffectsParamsUpdated->end();

      // current output line position
      int nPosX = 250, nPosY = 5;  
      gcpRendD3D->Set2DMode(false, 1, 1);
      SDrawTextInfo pDrawTexInfo;    
      pDrawTexInfo.color[0] = 0.0f;
      pDrawTexInfo.color[2] = 0.0f;
      pDrawTexInfo.color[1] = 1.0f;
      gcpRendD3D->Draw2dText(nPosX, nPosY, " " , pDrawTexInfo); // workaround to avoid garbage - something broken with draw2Dtext
      gcpRendD3D->Draw2dText(nPosX, nPosY, "Frame parameters:", pDrawTexInfo);
      nPosY += 15;

      pDrawTexInfo.color[0] = 1.0f;
      pDrawTexInfo.color[1] = 1.0f;
      pDrawTexInfo.color[2] = 1.0f;
      for( ; pItor != pEnd; ++pItor )
      {
        const char *pName = (pItor->first).c_str();
        char pNameAndValue[128];
        sprintf(pNameAndValue, "%s: %.4f\n", (pItor->first).c_str(), (pItor->second!=0)?pItor->second->GetParam():0.0f );
        gcpRendD3D->Draw2dText(nPosX, nPosY, pNameAndValue, pDrawTexInfo);   
        //CryLogAlways(pNameAndValue);

        nPosY += 10;
      }

      gcpRendD3D->Set2DMode(true, 1, 1);           
      pEffectsParamsUpdated->clear();
    }
  }
  
  PostEffectMgr().End(); 




  
  PROFILE_LABEL_POP( "POST EFFECTS" );

  return 1;
}
 
