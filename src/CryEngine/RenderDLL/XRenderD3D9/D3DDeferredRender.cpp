#include "StdAfx.h"
#include "DriverD3D.h"
#include "I3DEngine.h"

#pragma warning(disable: 4244)

bool CD3D9Renderer::FX_DeferredShadowPassSetup(const Matrix44& mShadowTexGen, float maskRTWidth, float maskRTHeight)
{

  //float fFragXSize;
  //CShadowUtils::CalcDifferentials(GetCamera(), maskRTWidth, maskRTHeight, fFragXSize);

	//set ScreenToWorld Expansion Basis
	Vec4r vWBasisX, vWBasisY, vWBasisZ, vCamPos;
	bool bVPosSM30 = (GetFeatures() & (RFT_HW_PS30|RFT_HW_PS40))!=0;






	if(m_RenderTileInfo.nGridSizeX > 1.f || m_RenderTileInfo.nGridSizeY > 1.f)
	{
		CShadowUtils::ProjectScreenToWorldExpansionBasis(mShadowTexGen, GetCamera(), maskRTWidth, maskRTHeight, vWBasisX, vWBasisY, vWBasisZ, vCamPos, bVPosSM30, &m_RenderTileInfo);
	}
	else
	{
		CShadowUtils::ProjectScreenToWorldExpansionBasis(mShadowTexGen, GetCamera(), maskRTWidth, maskRTHeight, vWBasisX, vWBasisY, vWBasisZ, vCamPos, bVPosSM30, NULL);
	}


	//TOFIX: create PB components for these params
	//creating common projection matrix for depth reconstruction

	//save magnitudes separately to inrease precision
	m_cEF.m_TempVecs[14].x = vWBasisX.GetLength();
	m_cEF.m_TempVecs[14].y = vWBasisY.GetLength();
	m_cEF.m_TempVecs[14].z = vWBasisZ.GetLength();
	m_cEF.m_TempVecs[14].w = 1.0f;

	//Vec4r normalization in doubles
  vWBasisX /= vWBasisX.GetLength();
  vWBasisY /= vWBasisY.GetLength();
  vWBasisZ /= vWBasisZ.GetLength();

	m_cEF.m_TempVecs[10].x = vWBasisX.x;
	m_cEF.m_TempVecs[10].y = vWBasisX.y;
	m_cEF.m_TempVecs[10].z = vWBasisX.z;
	m_cEF.m_TempVecs[10].w = vWBasisX.w;

	m_cEF.m_TempVecs[11].x = vWBasisY.x;
	m_cEF.m_TempVecs[11].y = vWBasisY.y;
	m_cEF.m_TempVecs[11].z = vWBasisY.z;
	m_cEF.m_TempVecs[11].w = vWBasisY.w;

	m_cEF.m_TempVecs[12].x = vWBasisZ.x;
	m_cEF.m_TempVecs[12].y = vWBasisZ.y;
	m_cEF.m_TempVecs[12].z = vWBasisZ.z;
	m_cEF.m_TempVecs[12].w = vWBasisZ.w;

	m_cEF.m_TempVecs[0].x =  vCamPos.x;
	m_cEF.m_TempVecs[0].y =  vCamPos.y;
	m_cEF.m_TempVecs[0].z =  vCamPos.z;
	m_cEF.m_TempVecs[0].w =  vCamPos.w;
  
	return true;
}


HRESULT GetSampleOffsetsGaussBlur5x5Bilinear(DWORD dwD3DTexWidth, DWORD dwD3DTexHeight, Vec4* avTexCoordOffset, Vec4* avSampleWeight, FLOAT fMultiplier)
{                 
  float tu = 1.0f / (float)dwD3DTexWidth ;
  float tv = 1.0f / (float)dwD3DTexHeight ;
  float totalWeight = 0.0f;
  Vec4 vWhite( 1.f, 1.f, 1.f, 1.f );
  float fWeights[5];

  int index = 0;
  for (int x=-2; x<=2; x++, index++)
  {
    fWeights[index] = GaussianDistribution((float)x, 0.f, 4);
  }

  //  compute weights for the 2x2 taps.  only 9 bilinear taps are required to sample the entire area.
  index = 0;
  for (int y=-2; y<=2; y+=2)
  {
    float tScale = (y==2)?fWeights[4] : (fWeights[y+2] + fWeights[y+3]);
    float tFrac  = fWeights[y+2] / tScale;
    float tOfs   = ((float)y + (1.f-tFrac)) * tv;
    for (int x=-2; x<=2; x+=2, index++)
    {
      float sScale = (x==2)?fWeights[4] : (fWeights[x+2] + fWeights[x+3]);
      float sFrac  = fWeights[x+2] / sScale;
      float sOfs   = ((float)x + (1.f-sFrac)) * tu;
      avTexCoordOffset[index] = Vec4(sOfs, tOfs, 0, 1);
      avSampleWeight[index]   = vWhite * sScale * tScale;
      totalWeight += sScale * tScale;
    }
  }

  for (int i=0; i<index; i++)
  {
    avSampleWeight[i] *= (fMultiplier / totalWeight);
  }

  return S_OK;
}

void CRenderer::FX_ApplyShadowQuality()
{
  SShaderProfile *pSP = &m_cEF.m_ShaderProfiles[eST_Shadow];
  m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_QUALITY] | g_HWSR_MaskBit[HWSR_QUALITY1]);
  int nQuality = (int)pSP->GetShaderQuality();
  m_RP.m_nShaderQuality = nQuality;
  switch (nQuality)
  {
  case eSQ_Medium:
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_QUALITY];
    break;
  case eSQ_High:
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_QUALITY1];
    break;
  case eSQ_VeryHigh:
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_QUALITY];
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_QUALITY1];
    break;
  }
}

void CD3D9Renderer::FX_StateRestore(int prevState)
{
#if defined (DIRECT3D9)
  //works only for for this state for now
  switch (prevState & GS_DEPTHFUNC_MASK)
  {
#if defined(INVERT_DEPTH_RANGE)
    case GS_DEPTHFUNC_EQUAL:
      m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
      break;
    case GS_DEPTHFUNC_LEQUAL:
      m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
      break;
    case GS_DEPTHFUNC_GREAT:
      m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
      break;
    case GS_DEPTHFUNC_LESS:
      m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATER);
      break;
    case GS_DEPTHFUNC_NOTEQUAL:
      m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_NOTEQUAL);
      break;
    case GS_DEPTHFUNC_GEQUAL:
      m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
      break;
#else 
    case GS_DEPTHFUNC_EQUAL:
      m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
      break;
    case GS_DEPTHFUNC_LEQUAL:
      m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
      break;
    case GS_DEPTHFUNC_GREAT:
      m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATER);
      break;
    case GS_DEPTHFUNC_LESS:
      m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
      break;
    case GS_DEPTHFUNC_NOTEQUAL:
      m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_NOTEQUAL);
      break;
    case GS_DEPTHFUNC_GEQUAL:
      m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
      break;
#endif

  }
#endif
}

void CD3D9Renderer::FX_ShadowBlur(float fShadowBluriness, SDynTexture *tpSrcTemp, CTexture *tpDst, int iShadowMode, bool bScreenVP, CTexture *tpDst2)
{
  if (m_LogFile)
    Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "   Blur shadow map...\n");

  uint32 nP;

  m_RP.m_FlagsStreams_Decl = 0;
  m_RP.m_FlagsStreams_Stream = 0;
  m_RP.m_FlagsPerFlush = 0;
  m_RP.m_pCurObject = m_RP.m_Objects[0];
  m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;

  m_RP.m_pPrevObject = NULL;
  m_RP.m_FrameObject++;
  EF_Scissor(false, 0, 0, 0, 0);
  D3DSetCull(eCULL_None);
  int nSizeX = tpDst->GetWidth();
  int nSizeY = tpDst->GetHeight();
  bool bCreateBlured = true;
  uint64 nMaskRT = m_RP.m_FlagsShader_RT;
                                                                                                                                                                                              
  STexState sTexState = STexState(FILTER_LINEAR, true);

  if (tpDst && tpSrcTemp)
	  fShadowBluriness *= (tpDst->GetWidth() / tpSrcTemp->GetWidth());

  float fVertDepth = 0.f;
  if(iShadowMode == 4)
  {
		// Use far clip plane for near objects to avoid SSAO computation on the weapon
    fVertDepth = CV_r_DrawNearZRange;
    Set2DMode(true, 1, 1, 0, 1);
  }
  else
    Set2DMode(true, 1, 1);

  // setup screen aligned quad
  SVF_P3F_C4B_T2F pScreenBlur[] =  
  {
		{ Vec3(0, 0, fVertDepth), {{~0}}, Vec2(0, 0) },
    { Vec3(0, 1, fVertDepth), {{~0}}, Vec2(0, 1) },
    { Vec3(1, 0, fVertDepth), {{~0}}, Vec2(1, 0) },
    { Vec3(1, 1, fVertDepth), {{~0}}, Vec2(1, 1) },
  };     

  CShader *pSH = m_cEF.m_ShaderShadowBlur;
  if (!pSH)
  {
    Set2DMode(false, 1, 1);
    return;
  }

	if(iShadowMode<0)
	{
		iShadowMode = CV_r_shadowblur;

		if(CV_r_shadowblur==3)
		{
			if (m_RP.m_nPassGroupID == EFSLIST_TRANSP || !CTexture::s_ptexZTarget)
				iShadowMode=2;  // blur transparent object in the standard way
		}
	}

//////////////////////////////////////////////////////////////////////////
  if (iShadowMode == 9)
  {


























































































































  } else if (iShadowMode == 8)
  {
    uint32 nPasses = 0;
    static CCryName TechName("ShadowBlurGen");
    pSH->FXSetTechnique(TechName);
    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    tpSrcTemp->SetRT(0, true, &m_DepthBufferOrig, bScreenVP);
    bool bSwap = false;

    int m_GenerateSATRDSamples = 4;
    for (int iPass = 1; iPass < nSizeX; iPass*= m_GenerateSATRDSamples)
    {
      int nDone =  iPass / m_GenerateSATRDSamples;

      EF_Scissor(true, nDone, 0, nSizeX, nSizeY);

      STexState sPointFilterState = STexState(FILTER_POINT, true);
      if (bSwap)
      {
        FX_SetRenderTarget(0, tpDst, &m_DepthBufferOrig, false, false, -1, bScreenVP);
        tpSrcTemp->m_pTexture->Apply(0, CTexture::GetTexState(sPointFilterState)); 
      }
      else
      {
        FX_SetRenderTarget(0, tpSrcTemp->m_pTexture, &m_DepthBufferOrig, false, false, -1, bScreenVP);
        tpDst->Apply(0, CTexture::GetTexState(sPointFilterState)); 
      }
      bSwap = !bSwap;

      EF_SetState(GS_NODEPTHTEST);

      pSH->FXBeginPass(0);
      Vec4 v;
#if defined (DIRECT3D10)
      v[0] = 0;
      v[1] = 0;
#else
      v[0] = 1.0f / (float)nSizeX;
      v[1] = 1.0f / (float)nSizeY;
#endif
      v[2] = 0;
      v[3] = 0;
      static CCryName Param1Name("PixelOffset");
      pSH->FXSetVSFloat(Param1Name, &v, 1);

      //setup pass offset
      Vec4 PassOffset;

      PassOffset[0] = float(iPass)/float(nSizeX);
      PassOffset[1] = 0;
      PassOffset[2] = 0;
      PassOffset[3] = 0;

      static CCryName ParamName("BlurOffset");
      pSH->FXSetPSFloat(ParamName, &PassOffset, 1);

      // Draw a fullscreen quad to sample the RT
      {
        CVertexBuffer pVertexBuffer(pScreenBlur,eVF_P3F_C4B_T2F);
        //EF_Commit() is called here
        DrawPrimitives(&pVertexBuffer, 4);  
      }
      pSH->FXEndPass();

    }
    EF_Scissor(false, 0, 0, 0, 0);
    SetTexture(0);
    pSH->FXEnd();
    FX_PopRenderTarget(0);
    FX_Commit();
  }
//////////////////////////////////////////////////////////////////////////
  else if (iShadowMode == 4 && CTexture::s_ptexZTarget)   // used for SSAO, with depth lookup to avoid shadow leaking
  {
    uint32 nPasses = 0;

		PROFILE_LABEL_PUSH( "SSAO_BLUR" );





    static CCryName TechName("SSAO_Blur");
    pSH->FXSetTechnique(TechName);
    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

		CTexture *pDepthTex = CTexture::s_ptexZTarget;
		CTexture *pDepthScaledTex = CTexture::s_ptexZTargetScaled;

		CTexture *tpSrc = tpSrcTemp ? tpSrcTemp->m_pTexture : CTexture::s_ptexZTargetScaled;

		STexState sPointState;
		sPointState.SetFilterMode(FILTER_POINT);        
		sPointState.SetClampMode(TADDR_CLAMP, TADDR_CLAMP, TADDR_CLAMP);
		sPointState.m_bSRGBLookup = 0;
		int nPointState = CTexture::GetTexState(sPointState);

		assert(tpSrc);
		tpSrc->Apply(0, CTexture::GetTexState(sTexState));
		pDepthTex->Apply(1, nPointState); 
		if( CRenderer::CV_r_SSAO_downscale_ztarget )
			pDepthScaledTex->Apply(2, nPointState); 






    D3DSetCull(eCULL_Back);

		int nStates = 0;

#ifndef PS3
		nStates |= GS_BLDST_SRCALPHA | GS_BLSRC_ZERO;
#endif

		EF_SetState(nStates);

		const uint64 fullMask = g_HWSR_MaskBit[HWSR_SAMPLE0];
		uint64 mask = 0;

		if (CV_r_SSAO_downscale_ztarget)
			mask |= g_HWSR_MaskBit[HWSR_SAMPLE0];

		m_RP.m_FlagsShader_RT = (m_RP.m_FlagsShader_RT & ~fullMask) | mask;

    for (nP=0; nP<nPasses; nP++)
    {
      pSH->FXBeginPass(nP);

			int nSrcSizeX = tpSrc->GetWidth();
			int nSrcSizeY = tpSrc->GetHeight();

      Vec4 v;

			#if HALF_PIXEL_SHIFT_NEEDED
				v[0] = 0.5f / nSizeX;
				v[1] = 0.5f / nSizeY;
			#else
				v[0] = 0;
				v[1] = 0;
			#endif
      v[2] = nSrcSizeX;
      v[3] = nSrcSizeY;

      static CCryName Param1Name("PixelOffset");
      pSH->FXSetVSFloat(Param1Name, &v, 1);

      // X Blur
      v[0] = 1.0f / (float)nSrcSizeX;
      v[1] = 1.0f / (float)nSrcSizeY;
			v[2] = (float)nSrcSizeX;
			v[3] = (float)nSrcSizeY;
      static CCryName Param2Name("BlurOffset");
      pSH->FXSetPSFloat(Param2Name, &v, 1);
			
			v[0] = 2.f / nSrcSizeX;
			v[1] = 0;
			v[2] = 2.f / nSrcSizeY;
			v[3] = 0;

			static CCryName Param3Name("SSAO_BlurKernel");
			pSH->FXSetPSFloat(Param3Name, &v, 1);

      // Draw a fullscreen quad to sample the RT
      CVertexBuffer pVertexBuffer(pScreenBlur,eVF_P3F_C4B_T2F);
      DrawPrimitives(&pVertexBuffer, 4);  

      pSH->FXEndPass();
    }
    EF_SetState(0);
    SetTexture(0);
    pSH->FXEnd();
		if(bScreenVP)
			FX_PopRenderTarget(0);



		PROFILE_LABEL_POP( "SSAO_BLUR" );
  }
  else if (iShadowMode == 3 && CTexture::s_ptexZTarget)   // with depth lookup to avoid shadow leaking - s_ptexZTarget might be 0 in wireframe mode
  {
    CTexture *tpDepthSrc = CTexture::s_ptexZTarget;

    tpDepthSrc->Apply(1, CTexture::GetTexState(sTexState)); 

    uint32 nPasses = 0;

    EF_SetState(GS_NODEPTHTEST);

    FX_PushRenderTarget(0, tpDst, &m_DepthBufferOrig, false, -1, bScreenVP);
    tpSrcTemp->Apply(0, CTexture::GetTexState(sTexState));

    static CCryName TechName("ShadowBlurScreenOpaque");
    pSH->FXSetTechnique(TechName);
    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    for (nP=0; nP<nPasses; nP++)
    {
      pSH->FXBeginPass(nP);

      float sW[9] = {0.2813f, 0.2137f, 0.1185f, 0.0821f, 0.0461f, 0.0262f, 0.0162f, 0.0102f, 0.0052f};

      Vec4 v;
    #if defined (DIRECT3D10)
      v[0] = 0;
      v[1] = 0;
    #else
      v[0] = 1.0f / (float)nSizeX;
      v[1] = 1.0f / (float)nSizeY;
    #endif
      v[2] = 0;
      v[3] = 0;
      static CCryName Param1Name("PixelOffset");
      pSH->FXSetVSFloat(Param1Name, &v, 1);

      // X Blur
      v[0] = 1.0f / (float)nSizeX * fShadowBluriness;
      v[1] = 1.0f / (float)nSizeY * fShadowBluriness;
      static CCryName Param2Name("BlurOffset");
      pSH->FXSetPSFloat(Param2Name, &v, 1);

      Vec4 vWeight[9];
      for (uint32 i=0; i<9; i++)
      {
        vWeight[i].x = sW[i];
        vWeight[i].y = sW[i];
        vWeight[i].z = sW[i];
        vWeight[i].w = sW[i];
      }
      //static CCryName Param3Name("SampleWeights");
      //pSH->FXSetPSFloat(Param3Name, vWeight, 9);

      // Draw a fullscreen quad to sample the RT
      CVertexBuffer pVertexBuffer(pScreenBlur,eVF_P3F_C4B_T2F);
      DrawPrimitives(&pVertexBuffer, 4);  

      pSH->FXEndPass();
    }
    SetTexture(0);
    pSH->FXEnd();
    FX_PopRenderTarget(0);
  }
  else if (iShadowMode == 1)
  {
    tpDst->Apply(0, CTexture::GetTexState(sTexState)); 
    tpSrcTemp->SetRT(0, true, &m_DepthBufferOrig, bScreenVP);
    uint32 nPasses = 0;
    static CCryName TechName("ShadowBlurScreen");
    pSH->FXSetTechnique(TechName);
    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    EF_SetState(GS_NODEPTHTEST);

    for (nP=0; nP<nPasses; nP++)
    {
      pSH->FXBeginPass(nP);

      float sW[9] = {0.2813f, 0.2137f, 0.1185f, 0.0821f, 0.0461f, 0.0262f, 0.0162f, 0.0102f, 0.0052f};

      Vec4 v;
    #if defined (DIRECT3D10)
      v[0] = 0;
      v[1] = 0;
    #else
      v[0] = 1.0f / (float)nSizeX;
      v[1] = 1.0f / (float)nSizeY;
    #endif
      v[2] = 0;
      v[3] = 0;
      static CCryName Param1Name("PixelOffset");
      pSH->FXSetVSFloat(Param1Name, &v, 1);

      // X Blur
      v[0] = 1.0f / (float)nSizeX * fShadowBluriness * 2.f;
      v[1] = 0;
      static CCryName Param2Name("BlurOffset");
      pSH->FXSetPSFloat(Param2Name, &v, 1);

      Vec4 vWeight[9];
			float fSumm = 0;
			for (uint32 i=0; i<9; i++)
				fSumm += sW[i];

      for (uint32 i=0; i<9; i++)
      {
        vWeight[i].x = sW[i]/fSumm;
        vWeight[i].y = sW[i]/fSumm;
        vWeight[i].z = sW[i]/fSumm;
        vWeight[i].w = sW[i]/fSumm;
      }
      static CCryName Param3Name("SampleWeights");
      pSH->FXSetPSFloat(Param3Name, vWeight, 9);

      // Draw a fullscreen quad to sample the RT
      {
        CVertexBuffer pVertexBuffer(pScreenBlur,eVF_P3F_C4B_T2F);
        DrawPrimitives(&pVertexBuffer, 4);  
      }

      FX_SetRenderTarget(0, tpDst, &m_DepthBufferOrig, false, false, -1, bScreenVP);

      // Y Blur
      v[0] = 0;
      v[1] = 1.0f / (float)nSizeY * fShadowBluriness * 2.f;
      pSH->FXSetPSFloat(Param2Name, &v, 1);

      tpSrcTemp->m_pTexture->Apply(0, CTexture::GetTexState(sTexState)); 

      // Draw a fullscreen quad to sample the RT
      {
        CVertexBuffer pVertexBuffer(pScreenBlur,eVF_P3F_C4B_T2F);
        DrawPrimitives(&pVertexBuffer, 4);  
      }
      pSH->FXEndPass();
    }
    SetTexture(0);
    pSH->FXEnd();
    FX_PopRenderTarget(0);
    FX_Commit();
  }
  else if (iShadowMode == 0 || iShadowMode == 2 )
  {

    FX_PushRenderTarget(0, tpDst, NULL, false, -1, bScreenVP);
    tpSrcTemp->Apply(0, CTexture::GetTexState(sTexState));

    uint32 nPasses = 0;
    static CCryName TechName("ShadowGaussBlur5x5");
    pSH->FXSetTechnique(TechName);
    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    Vec4 avSampleOffsets[10];
    Vec4 avSampleWeights[10];

    EF_SetState(GS_NODEPTHTEST);

    RECT rectSrc;
    GetTextureRect(tpSrcTemp->m_pTexture, &rectSrc);
    InflateRect(&rectSrc, -1, -1);

    RECT rectDest;
    GetTextureRect(tpDst, &rectDest);
    InflateRect(&rectDest, -1, -1);

    CoordRect coords;
    GetTextureCoords(tpSrcTemp->m_pTexture, &rectSrc, tpDst, &rectDest, &coords);

    for (nP=0; nP<nPasses; nP++)
    {
      pSH->FXBeginPass(nP);

      Vec4 v;
    #if defined (DIRECT3D10)
      v[0] = 0;
      v[1] = 0;
    #else
      v[0] = 1.0f / (float)nSizeX;
      v[1] = 1.0f / (float)nSizeY;
    #endif
      v[2] = 0;
      v[3] = 0;
      static CCryName Param1Name("PixelOffset");
      pSH->FXSetVSFloat(Param1Name, &v, 1);

      uint32 n = 9;
      float fBluriness = CLAMP(fShadowBluriness, 0.01f, 16.0f);
      HRESULT hr = GetSampleOffsetsGaussBlur5x5Bilinear((int)(nSizeX/fBluriness), (int)(nSizeY/fBluriness), avSampleOffsets, avSampleWeights, 1.0f);
      static CCryName Param2Name("SampleOffsets");
      static CCryName Param3Name("SampleWeights");
      pSH->FXSetPSFloat(Param2Name, avSampleOffsets, n);
      pSH->FXSetPSFloat(Param3Name, avSampleWeights, n);

      // Draw a fullscreen quad to sample the RT
      ::DrawFullScreenQuad(coords);

      pSH->FXEndPass();
    }
    pSH->FXEnd();
    FX_PopRenderTarget(0);
  }
  Set2DMode(false, 1, 1);
  m_RP.m_FlagsShader_RT = nMaskRT;

  if (m_LogFile)
    Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "   End bluring of shadow map...\n");
}

void CD3D9Renderer::FX_StencilTestCurRef(bool bEnable)
{
  if (bEnable)
  {
    //conservative stencil state








    EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_EQUAL) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
      STENCOP_PASS(FSS_STENCOP_KEEP),
      m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFFFFFF);


    EF_SetState( m_RP.m_CurState | GS_STENCIL );




  }
  else 
  {



  }

  
}

void CD3D9Renderer::FX_HiStencilUpdate(int nVertexOffset, int nNumVers, int nIndOffs, int nNumInds, int stencilFunc)
{

  //state to save depth func before D3DCMP_ALWAYS
  int prevState = m_RP.m_CurState;

  int newState = 0;
  newState |= GS_COLMASK_NONE;
  newState |= GS_NODEPTHTEST;
  newState &= ~GS_DEPTHWRITE;
  newState |= GS_STENCIL;

  FX_Commit();
  EF_SetState( newState );

  //platform specific






#if defined (DIRECT3D9) 
  m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);





#endif










#if defined (DIRECT3D9) || defined(OPENGL)
  if (nNumVers>0)
  {
    m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, nVertexOffset, 0, nNumVers, nIndOffs, nNumInds/3);
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += nNumInds/3;
  }
  else
  {
    //draw quad
    m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, nVertexOffset, 2);
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += 2;
  }
#elif defined (DIRECT3D10)
  if (nNumVers>0)
  {
    SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pd3dDeviceContext->DrawIndexed(nNumInds, nIndOffs, nVertexOffset);
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += nNumInds/3;
  }
  else
  {
    //draw quad
    SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_pd3dDeviceContext->Draw(4, nVertexOffset);
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += 2;
  }
#endif

  m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;

//disable
//platform specific








  //restore previous depthtest state because current is D3DCMP_ALWAYS
  FX_StateRestore(prevState);

}

enum EDefShadows_Passes
{
  DS_STENCIL_PASS,
  DS_SHADOW_PASS,
  DS_SHADOW_CULL_PASS,
  DS_SHADOW_FRUSTUM_CULL_PASS,
  DS_STENCIL_VOLUME_CLIP,
  DS_PASS_MAX
};

void CD3D9Renderer::FX_DeferredShadowPass(const CDLight* pLight, int nLightInGroup, ShadowMapFrustum *pShadowFrustum, float fFinalRange, int nVertexOffset, bool bShadowPass, bool bStencilPrepass, int nLod, int nFrustNum)
{
  uint32 nPassCount = 0;
  CShader*  pShader = CShaderMan::m_ShaderShadowMaskGen;
  static CCryName DeferredShadowTechName = "DeferredShadowPass";

  FX_DisableATOC();

  D3DSetCull(eCULL_Back);

  //if(pShadowFrustum->nAffectsReceiversFrameId != GetFrameID(false))
  //  return;
  if (!CV_r_UseShadowsPool && (pShadowFrustum->pDepthTex==NULL && pShadowFrustum->pDepthTexArray==NULL ))
    return;


  //used in light pass only
  if (pShadowFrustum->bForSubSurfScattering)
    return;

  if (pShadowFrustum->pCastersList == NULL)
  {   
    return;
  }

  FX_ApplyShadowQuality();

  //////////////////////////////////////////////////////////////////////////
  // set global shader RT flags
  //////////////////////////////////////////////////////////////////////////

  // set pass dependent RT flags
  m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[ HWSR_SAMPLE0 ] | g_HWSR_MaskBit[ HWSR_SAMPLE1 ] | g_HWSR_MaskBit[ HWSR_SAMPLE2 ] | g_HWSR_MaskBit[ HWSR_SAMPLE3 ] | g_HWSR_MaskBit[ HWSR_SAMPLE3 ] |
                             g_HWSR_MaskBit[ HWSR_SAMPLE4 ] |
                             g_HWSR_MaskBit[HWSR_CUBEMAP0] | g_HWSR_MaskBit[HWSR_CUBEMAP1] | g_HWSR_MaskBit[HWSR_CUBEMAP2] | g_HWSR_MaskBit[HWSR_CUBEMAP3] |
                             g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ]  | g_HWSR_MaskBit[ HWSR_POINT_LIGHT ] |
                             g_HWSR_MaskBit[ HWSR_SHADOW_MIXED_MAP_G16R16 ] | g_HWSR_MaskBit[ HWSR_SHADOW_FILTER ] |
                             g_HWSR_MaskBit[HWSR_FSAA] | g_HWSR_MaskBit[HWSR_SHADOW_JITTERING] | g_HWSR_MaskBit[HWSR_FSAA_QUALITY]);

  //enable multi-sample rendering
  if (!(pShadowFrustum->bUseVarianceSM))
  {
    if (m_RP.m_FSAAData.Type==8)
    {
      m_RP.m_FlagsShader_RT |= (g_HWSR_MaskBit[HWSR_FSAA] | g_HWSR_MaskBit[HWSR_FSAA_QUALITY]);
    }
    else if (m_RP.m_FSAAData.Type==4)
    {
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_FSAA];
    }
    else if (m_RP.m_FSAAData.Type==2)
    {
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_FSAA_QUALITY];
    }
  }

//Diesel cut
  //if( CV_r_shadow_jittering > 0.0f )
  //  m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SHADOW_JITTERING ];


  //depthMapSampler0 is used
  m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SAMPLE0 ];

  if( pShadowFrustum->bOmniDirectionalShadow && !pShadowFrustum->bUnwrapedOmniDirectional)
  {
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_CUBEMAP0];

    //FIX:: force using G16R16 for cubemaps for now
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SHADOW_MIXED_MAP_G16R16 ];
  }

  if(pShadowFrustum->bUseFilter)
  {
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SHADOW_FILTER ];
  }

  //enable depth precision shift for sun's FP shadow RTs
  if(!(pShadowFrustum->bNormalizedDepth) && !(pShadowFrustum->bHWPCFCompare))
  {
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SHADOW_MIXED_MAP_G16R16 ];
  }

  //FIX: workaround to process TEXTURE ARRAYS properly
  if(pShadowFrustum->pDepthTexArray && pShadowFrustum->pDepthTexArray->m_eTF == eTF_G16R16)
  {
    m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[ HWSR_SHADOW_MIXED_MAP_G16R16 ];
  }

  if (!(pShadowFrustum->m_Flags & DLF_DIRECTIONAL))
  {
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_POINT_LIGHT ];  

    //one-side projector
    if (pShadowFrustum->m_Flags & DLF_PROJECT)
    {
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SAMPLE4 ];
    }
  }

  //enable hw-pcf per frustum
  if (pShadowFrustum->bHWPCFCompare)
  {
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];
  }

	if(pShadowFrustum->bUseVarianceSM)
		m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_VARIANCE_SM ];
	else
		m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[ HWSR_VARIANCE_SM ];

#if defined (DIRECT3D10)
  if(CV_r_ShadowGenGS!=0 && (pLight->m_Flags & DLF_DIRECTIONAL))
  {
    //enable sampling from texture array
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_TEX_ARR_SAMPLE ];

    //-1 to disable custom resource view
    ConfigShadowTexgen( 0, pShadowFrustum, nLod - 1 ); 
  }
  else if ( pShadowFrustum->bOmniDirectionalShadow && (nFrustNum > -1) )
  {
    //enable unwraped shadow maps for omni lights
    ConfigShadowTexgen( 0, pShadowFrustum, -1, nFrustNum, pLight);
  }
  else
  {
    ConfigShadowTexgen( 0, pShadowFrustum);
  }
#else
  if ( pShadowFrustum->bOmniDirectionalShadow && (nFrustNum > -1) )
  {
    //enable unwraped shadow maps for omni lights
    ConfigShadowTexgen( 0, pShadowFrustum, -1, nFrustNum, pLight);
  }
  else
  {
    ConfigShadowTexgen( 0, pShadowFrustum);
  }
#endif

  int newState = m_RP.m_CurState;
  newState |= GS_NODEPTHTEST;
  newState &= ~GS_DEPTHWRITE;
  if(pShadowFrustum->bUseAdditiveBlending)
  {
    newState |= GS_BLSRC_ONE | GS_BLDST_ONE;

    static ICVar * p_e_shadows_clouds = iConsole->GetCVar("e_ShadowsClouds");

    if (p_e_shadows_clouds->GetIVal() && m_nCloudShadowTexId > 0)
    {
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SAMPLE2 ]; //enable modulation by clouds shadow 
    }

  }
  else
    newState &= ~(GS_BLSRC_ONE | GS_BLDST_ONE);

  //////////////////////////////////////////////////////////////////////////
  //half and quarter shadow mask resolutions can not be used together with DBT due to RT - Depth Buffer mismatch
  if (CV_r_ShadowsDepthBoundNV && m_bDeviceSupports_NVDBT && CV_r_ShadowsMaskResolution==0 && !m_RP.m_FSAAData.Type)
  {
    if (!(pShadowFrustum->bOmniDirectionalShadow))
    {
      //bounds calculation code in 3dengine
      //zMax = ppSMFrustumList[ nCaster ]->fMaxFrustumBound;
      SetDepthBoundTest(0.0f, fFinalRange, true);
    }
    else
    {
      SetDepthBoundTest(0.0f, 0.0f, false);    
    }
  }

  pShader->FXSetTechnique(DeferredShadowTechName);
  pShader->FXBegin(&nPassCount, FEF_DONTSETSTATES) ;


  //////////////////////////////////////////////////////////////////////////
  //Stencil cull pre-pass for GSM
  //////////////////////////////////////////////////////////////////////////
  if ( bStencilPrepass && !(pShadowFrustum->bUseAdditiveBlending) )
  {
    newState |= GS_STENCIL;
    //Disable color writes
    newState |= GS_COLMASK_NONE;

    EF_SetState(newState);
    //////////////////////////////////////////////////////////////////////////
    if (!CV_r_ShadowsUseClipVolume)
    {
      EF_SetStencilState(
        STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
        STENCOP_FAIL(FSS_STENCOP_KEEP) |
        STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
        STENCOP_PASS(FSS_STENCOP_REPLACE),
        nLod, 0x7F, 0x7F
        );
      pShader->FXBeginPass( DS_STENCIL_PASS );
      if (!FAILED(FX_SetVertexDeclaration( 0, eVF_P3F_T2F_T3F )))
      {
        FX_Commit();
        //FX_ZState( newState );
#if defined (DIRECT3D9) || defined(OPENGL)
        m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, nVertexOffset, 2 );
#elif defined (DIRECT3D10)
        SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        m_pd3dDeviceContext->Draw(4, nVertexOffset);
#endif                                                                   
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += 2;
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
      }
    }
    else
    { //render clip volume
      Matrix44 mViewProj = pShadowFrustum->mLightViewMatrix;
      Matrix44 mViewProjInv = mViewProj.GetInverted();
      gRenDev->m_TempMatrices[0][0].Transpose(mViewProjInv);

      pShader->FXBeginPass( DS_STENCIL_VOLUME_CLIP );
      if (!FAILED(FX_SetVertexDeclaration( 0, eVF_P3F_C4B_T2F )))
      {
        FX_SetVStream( 0, m_pUnitFrustumVB[SHAPE_SIMPLE_PROJECTOR], 0, sizeof( SVF_P3F_C4B_T2F ) );
        FX_SetIStream(m_pUnitFrustumIB[SHAPE_SIMPLE_PROJECTOR]);
        FX_StencilCullPass(nLod, 0, m_UnitFrustVBSize[SHAPE_SIMPLE_PROJECTOR], 0, m_UnitFrustIBSize[SHAPE_SIMPLE_PROJECTOR]);
      }
    }

    pShader->FXEndPass();
  }
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Shadow Pass
//////////////////////////////////////////////////////////////////////////

  if (bShadowPass)
  {
    pShader->FXBeginPass( DS_SHADOW_PASS );

    if (!FAILED(FX_SetVertexDeclaration( 0, eVF_P3F_T2F_T3F )))
    {
  #if defined (DIRECT3D9) || defined(PS3)

    //Hi-Stencil Refresh



      //for nvidia only
      bool bHiStencilRefresh = m_bDeviceSupports_NVDBT;

      if (true/*nLod !=0 && bHiStencilRefresh*/)
      {
        if (pShadowFrustum->bOmniDirectionalShadow || pLight->m_Flags & DLF_PROJECT)
        {
          if (nLod>=0)
          {
            uint32 nStencilMask = 1<<(nLod - 1);
            EF_SetStencilState(
              STENC_FUNC(FSS_STENCFUNC_EQUAL) |
              STENCOP_FAIL(FSS_STENCOP_KEEP) |
              STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
              STENCOP_PASS(FSS_STENCOP_KEEP),
              0xFFFF, nStencilMask, 0xFFFFFFFF
            );
            //TD - fix stencil refresh for point lights for xenon&ps3 (bitwise-deferred mode)
          }
          else
          {
            EF_SetStencilState(
              STENC_FUNC(FSS_STENCFUNC_EQUAL) |
              STENCOP_FAIL(FSS_STENCOP_KEEP) |
              STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
              STENCOP_PASS(FSS_STENCOP_KEEP),
              m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFFFFFF);
          }

        }
        else
        {
          EF_SetStencilState(
            STENC_FUNC(FSS_STENCFUNC_EQUAL) |
            STENCOP_FAIL(FSS_STENCOP_KEEP) |
            STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
            STENCOP_PASS(FSS_STENCOP_KEEP),
            nLod, 0xFFFFFFFF, 0xFFFFFFFF
          );
        }

        FX_HiStencilUpdate(nVertexOffset);
      }
  #endif
  //////////////////////////////////////////////////////////////////////////

      //was stencil pre-pass
      if (nLod !=0)
      {

        //Shadow pass states
        newState |= GS_STENCIL;
        if (pShadowFrustum->bOmniDirectionalShadow  || pLight->m_Flags & DLF_PROJECT)
        {
          //TODO:generalize stencil cull pass for GSM, omni-lights and projectors
          if (nLod>=0)
          {
            uint32 nStencilMask = 1<<(nLod - 1);
            EF_SetStencilState(
              STENC_FUNC(FSS_STENCFUNC_EQUAL) |
              STENCOP_FAIL(FSS_STENCOP_KEEP) |
              STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
              STENCOP_PASS(FSS_STENCOP_KEEP),
              0xFFFF, nStencilMask, 0xFFFFFFFF
              );
          }
          else
          {
            EF_SetStencilState(
              STENC_FUNC(FSS_STENCFUNC_EQUAL) |
              STENCOP_FAIL(FSS_STENCOP_KEEP) |
              STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
              STENCOP_PASS(FSS_STENCOP_KEEP),
              m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFFFFFF);
          }
        }
        else
        {
          EF_SetStencilState(
            STENC_FUNC(FSS_STENCFUNC_EQUAL) |
            STENCOP_FAIL(FSS_STENCOP_KEEP) |
            STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
            STENCOP_PASS(FSS_STENCOP_KEEP),
            nLod, 0xFFFFFFFF, 0xFFFFFFFF
            );
          // newState |= GS_DEPTHFUNC_EQUAL;
        }




      }
      else
      {
        newState &= ~GS_STENCIL;
      }

      //Set LS colormask
      newState &= ~GS_COLMASK_NONE;
      newState |= ( ( ~( 1 << nLightInGroup ) ) << GS_COLMASK_SHIFT ) & GS_COLMASK_MASK;
      //newState |= ( ( ~( 1 << /*nCaster */(nLod-1)) ) << GS_COLMASK_SHIFT ) & GS_COLMASK_MASK;


      //FX_ZState( newState );
      EF_SetState( newState );
      FX_Commit();

      #if defined (DIRECT3D9) || defined(OPENGL)
        m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, nVertexOffset, 2 );
      #elif defined (DIRECT3D10)
        SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        m_pd3dDeviceContext->Draw(4, nVertexOffset);
      #endif





      m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += 2;
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
      pShader->FXEndPass();
    }
  }
  pShader->FXEnd();

}


bool CD3D9Renderer::CreateAuxiliaryMeshes()
{
  //projector frustum mesh
  t_arrDeferredMeshIndBuff arrDeferredInds;
  t_arrDeferredMeshVertBuff arrDeferredVerts;
  CDeferredRenderUtils::CreateUnitFrustumMesh(10, 10, arrDeferredInds, arrDeferredVerts);
  SAFE_RELEASE(m_pUnitFrustumVB[SHAPE_PROJECTOR]);
  SAFE_RELEASE(m_pUnitFrustumIB[SHAPE_PROJECTOR]);
  CreateUnitVolumeMesh(arrDeferredInds, arrDeferredVerts, m_pUnitFrustumIB[SHAPE_PROJECTOR], m_pUnitFrustumVB[SHAPE_PROJECTOR]);
  m_UnitFrustVBSize[SHAPE_PROJECTOR] = arrDeferredVerts.size();
  m_UnitFrustIBSize[SHAPE_PROJECTOR] = arrDeferredInds.size();

  //clip-projector frustum mesh
  CDeferredRenderUtils::CreateUnitFrustumMesh(30, 30, arrDeferredInds, arrDeferredVerts);
  SAFE_RELEASE(m_pUnitFrustumVB[SHAPE_CLIP_PROJECTOR]);
  SAFE_RELEASE(m_pUnitFrustumIB[SHAPE_CLIP_PROJECTOR]);
  CreateUnitVolumeMesh(arrDeferredInds, arrDeferredVerts, m_pUnitFrustumIB[SHAPE_CLIP_PROJECTOR], m_pUnitFrustumVB[SHAPE_CLIP_PROJECTOR]);
  m_UnitFrustVBSize[SHAPE_CLIP_PROJECTOR] = arrDeferredVerts.size();
  m_UnitFrustIBSize[SHAPE_CLIP_PROJECTOR] = arrDeferredInds.size();

  //omni-light mesh
  CDeferredRenderUtils::CreateUnitSphere(1, arrDeferredInds, arrDeferredVerts);
  SAFE_RELEASE(m_pUnitSphereVB);
  SAFE_RELEASE(m_pUnitSphereIB);
  CreateUnitVolumeMesh(arrDeferredInds, arrDeferredVerts, m_pUnitSphereIB, m_pUnitSphereVB);
  m_UnitSphereVBSize = arrDeferredVerts.size();
  m_UnitSphereIBSize = arrDeferredInds.size();

  //unit box
  CDeferredRenderUtils::CreateUnitBox(arrDeferredInds, arrDeferredVerts);
  SAFE_RELEASE(m_pUnitBoxVB);
  SAFE_RELEASE(m_pUnitBoxIB);
  CreateUnitVolumeMesh(arrDeferredInds, arrDeferredVerts, m_pUnitBoxIB, m_pUnitBoxVB);
  m_UnitBoxVBSize = arrDeferredVerts.size();
  m_UnitBoxIBSize = arrDeferredInds.size();

  //frustum approximated with 8 vertices
  CDeferredRenderUtils::CreateSimpleLightFrustumMesh(arrDeferredInds, arrDeferredVerts);
  SAFE_RELEASE(m_pUnitFrustumVB[SHAPE_SIMPLE_PROJECTOR]);
  SAFE_RELEASE(m_pUnitFrustumIB[SHAPE_SIMPLE_PROJECTOR]);
  CreateUnitVolumeMesh(arrDeferredInds, arrDeferredVerts, m_pUnitFrustumIB[SHAPE_SIMPLE_PROJECTOR], m_pUnitFrustumVB[SHAPE_SIMPLE_PROJECTOR]);
  m_UnitFrustVBSize[SHAPE_SIMPLE_PROJECTOR] = arrDeferredVerts.size();
  m_UnitFrustIBSize[SHAPE_SIMPLE_PROJECTOR] = arrDeferredInds.size();


  return true;
}

bool CD3D9Renderer::ReleaseAuxiliaryMeshes()
{
  
  for (int i=0;i<SHAPE_MAX;i++)
  {
    SAFE_RELEASE(m_pUnitFrustumVB[i]);
    SAFE_RELEASE(m_pUnitFrustumIB[i]);
  }

  SAFE_RELEASE(m_pUnitSphereVB);
  SAFE_RELEASE(m_pUnitSphereIB);

  SAFE_RELEASE(m_pUnitBoxVB);
  SAFE_RELEASE(m_pUnitBoxIB);

  return true;
}

bool CD3D9Renderer::CreateUnitVolumeMesh(t_arrDeferredMeshIndBuff& arrDeferredInds, t_arrDeferredMeshVertBuff& arrDeferredVerts, D3DIndexBuffer*& pUnitFrustumIB, D3DVertexBuffer*& pUnitFrustumVB)
{
  /*CDLight unitLight;

  unitLight.m_fRadius = 1.0f;
  unitLight.m_Origin = Vec3(0.0f, 0.0f, 0.0f);
  unitLight.m_ObjMatrix.SetIdentity();
  unitLight.m_fLightFrustumAngle = 90.0f;*/

  //t_arrDeferredMeshIndBuff arrDeferredInds;
  //t_arrDeferredMeshVertBuff arrDeferredVerts;
  //CDeferredRenderUtils::CreateUnitFrustumMesh(10, 10, arrDeferredInds, arrDeferredVerts);

  //SAFE_RELEASE(m_pUnitFrustumVB[i]);
  //SAFE_RELEASE(m_pUnitFrustumIB[i]);

  HRESULT hr = S_OK;

  //FIX: try default pools
#if defined (DIRECT3D9) || defined(OPENGL)
  hr = m_pd3dDevice->CreateVertexBuffer( arrDeferredVerts.size() * sizeof( SDeferMeshVert ), D3DUSAGE_WRITEONLY,
    0, D3DPOOL_DEFAULT, &pUnitFrustumVB, NULL );
  assert(SUCCEEDED(hr));
  if(FAILED(hr))
  {
    return false;
  }

  hr = m_pd3dDevice->CreateIndexBuffer( arrDeferredInds.size() * sizeof( uint16 ), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
    D3DPOOL_DEFAULT, &pUnitFrustumIB, NULL );
  assert(SUCCEEDED(hr));
  if(FAILED(hr))
  {
    return false;
  }

  SDeferMeshVert* pVerts = NULL;
  uint16* pInds = NULL;

  //allocate vertices
  hr = pUnitFrustumVB->Lock(0, arrDeferredVerts.size() * sizeof( SDeferMeshVert ), (void **) &pVerts, 0);
  assert(SUCCEEDED(hr));

  memcpy( pVerts, &arrDeferredVerts[0], arrDeferredVerts.size()*sizeof(SDeferMeshVert ) );

  hr = pUnitFrustumVB->Unlock();
  assert(SUCCEEDED(hr));

  //allocate indices
  hr = pUnitFrustumIB->Lock(0, arrDeferredInds.size() * sizeof( uint16 ), (void **) &pInds, 0);
  assert(SUCCEEDED(hr));

  memcpy( pInds, &arrDeferredInds[0], sizeof(uint16)*arrDeferredInds.size() );

  hr = pUnitFrustumIB->Unlock();
  assert(SUCCEEDED(hr));

#elif defined (DIRECT3D10)

  D3D11_BUFFER_DESC BufDesc;
  ZeroStruct(BufDesc);
  BufDesc.ByteWidth = arrDeferredVerts.size() * sizeof( SDeferMeshVert );
  BufDesc.Usage = D3D11_USAGE_IMMUTABLE;
  BufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  BufDesc.CPUAccessFlags = 0;
  BufDesc.MiscFlags = 0;

  D3D11_SUBRESOURCE_DATA SubResData;
  ZeroStruct(SubResData);
  SubResData.pSysMem = &arrDeferredVerts[0];
  SubResData.SysMemPitch = 0;
  SubResData.SysMemSlicePitch = 0;

  hr = m_pd3dDevice->CreateBuffer(&BufDesc, &SubResData, (ID3D11Buffer **)&pUnitFrustumVB);
  assert(SUCCEEDED(hr));

  ZeroStruct(BufDesc);
  BufDesc.ByteWidth = arrDeferredInds.size() * sizeof( uint16 );
  BufDesc.Usage = D3D11_USAGE_IMMUTABLE;
  BufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  BufDesc.CPUAccessFlags = 0;
  BufDesc.MiscFlags = 0;

  ZeroStruct(SubResData);
  SubResData.pSysMem = &arrDeferredInds[0];
  SubResData.SysMemPitch = 0;
  SubResData.SysMemSlicePitch = 0;

  hr = m_pd3dDevice->CreateBuffer(&BufDesc, &SubResData, &pUnitFrustumIB);
  assert(SUCCEEDED(hr));
#endif

  return SUCCEEDED(hr);
}



bool CD3D9Renderer::CreateUnitLightMeshes()
{

  t_arrDeferredMeshIndBuff arrDeferredInds;
  t_arrDeferredMeshVertBuff arrDeferredVerts;

  CDeferredRenderUtils::CreateUnitSphere(1, arrDeferredInds, arrDeferredVerts);

  SAFE_RELEASE(m_pUnitSphereVB);
  SAFE_RELEASE(m_pUnitSphereIB);

  HRESULT hr = S_OK;

  //FIX: try default pools
#if defined (DIRECT3D9) || defined(OPENGL)
  hr = m_pd3dDevice->CreateVertexBuffer( arrDeferredVerts.size() * sizeof( SDeferMeshVert ), D3DUSAGE_WRITEONLY,
    0, D3DPOOL_DEFAULT, &m_pUnitSphereVB, NULL );
  assert(SUCCEEDED(hr));
  if(FAILED(hr))
  {
    return false;
  }

  hr = m_pd3dDevice->CreateIndexBuffer( arrDeferredInds.size() * sizeof( uint16 ), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
    D3DPOOL_DEFAULT, &m_pUnitSphereIB, NULL );
  assert(SUCCEEDED(hr));
  if(FAILED(hr))
  {
    return false;
  }
#elif defined (DIRECT3D10)
  assert(0);
  /*D3D11_BUFFER_DESC BufDesc;
  ZeroStruct(BufDesc);
  BufDesc.ByteWidth = VBsize;
  BufDesc.Usage = D3D11_USAGE_DEFAULT;
  BufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  BufDesc.CPUAccessFlags = 0;
  BufDesc.MiscFlags = 0; //D3D11_RESOURCE_MISC_COPY_DESTINATION;
  HRESULT hReturn = m_pd3dDevice->CreateBuffer(&BufDesc, NULL, (ID3D11Buffer **)&m_pUnitFrustumVB);
  assert(SUCCEEDED(hReturn));
  */
#endif

  SDeferMeshVert* pVerts = NULL;
  uint16* pInds = NULL;

#if defined (DIRECT3D9) || defined(OPENGL)
  //allocate vertices
  hr = m_pUnitSphereVB->Lock(0, arrDeferredVerts.size() * sizeof( SDeferMeshVert ), (void **) &pVerts, 0);
  assert(SUCCEEDED(hr));

  memcpy( pVerts, &arrDeferredVerts[0], arrDeferredVerts.size()*sizeof(SDeferMeshVert ) );

  hr = m_pUnitSphereVB->Unlock();
  assert(SUCCEEDED(hr));

  //allocate indices
  hr = m_pUnitSphereIB->Lock(0, arrDeferredInds.size() * sizeof( uint16 ), (void **) &pInds, 0);
  assert(SUCCEEDED(hr));

  memcpy( pInds, &arrDeferredInds[0], sizeof(uint16)*arrDeferredInds.size() );

  hr = m_pUnitSphereIB->Unlock();
  assert(SUCCEEDED(hr));

#elif defined (DIRECT3D10)
  assert(0);
  /*byte *pData = (byte*) 0x12345678;
  hr = m_pVBTemp[m_nCurStagedVB]->Map(D3D11_MAP_WRITE, 0, (void **) &pData);
  m_StagedStream[nType] = m_nCurStagedVB++;
  if (m_nCurStagedVB > CV_d3d10_NumStagingBuffers-1)
  m_nCurStagedVB = 0;

  pVertices = &pData[0];*/
#endif

  m_UnitSphereVBSize = arrDeferredVerts.size();
  m_UnitSphereIBSize = arrDeferredInds.size();

  return true;

}


void CD3D9Renderer::FX_StencilCullPass(int nStencilID, int nVertOffs, int nNumVers, int nIndOffs, int nNumInds)
{
  int newState = m_RP.m_CurState;






  //Set LS colormask
  //debug states
  if (CV_r_DebugLightVolumes /*&& m_RP.m_TI.m_PersFlags2 & RBPF2_LIGHTSTENCILCULL*/)
  {
    newState &= ~GS_COLMASK_NONE;
    newState &= ~GS_NODEPTHTEST;
    //newState |= GS_NODEPTHTEST;
    newState |= GS_DEPTHWRITE;
    newState |= ( ( ~( 0xF) ) << GS_COLMASK_SHIFT ) & GS_COLMASK_MASK;
    if (CV_r_DebugLightVolumes>1)
    {
      newState |= GS_WIREFRAME;
    }
  }
  else
  {
    //Disable color writes
    newState |= GS_COLMASK_NONE;

    //setup depth test  
    newState &= ~GS_NODEPTHTEST;
    newState &= ~GS_DEPTHWRITE;
    newState |= GS_DEPTHFUNC_LEQUAL;
    newState |= GS_STENCIL;





  }

  //////////////////////////////////////////////////////////////////////////
  //draw back faces - inc when depth fail
  //////////////////////////////////////////////////////////////////////////
  if (nStencilID >= 0)
  {
    D3DSetCull(eCULL_Front);
#if defined (DIRECT3D10)
    EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_ALWAYS) | STENC_CCW_FUNC(FSS_STENCFUNC_ALWAYS) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_CCW_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_REPLACE) | STENCOP_CCW_ZFAIL(FSS_STENCOP_REPLACE) |
      STENCOP_PASS(FSS_STENCOP_KEEP) | STENCOP_CCW_PASS(FSS_STENCOP_KEEP),
      nStencilID, 0xFFFFFFFF, 0xFFFF
      );
#else
    EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
      STENCOP_PASS(FSS_STENCOP_KEEP),
      nStencilID, 0xFFFFFFFF, 0xFFFF
      );
#endif
    //    uint32 nStencilWriteMask = 1 << nStencilID; //0..7
    //#if defined (DIRECT3D10)
    //    EF_SetStencilState(
    //      STENC_FUNC(FSS_STENCFUNC_ALWAYS) | STENC_CCW_FUNC(FSS_STENCFUNC_ALWAYS) |
    //      STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_CCW_FAIL(FSS_STENCOP_KEEP) |
    //      STENCOP_ZFAIL(FSS_STENCOP_REPLACE) | STENCOP_CCW_ZFAIL(FSS_STENCOP_REPLACE) |
    //      STENCOP_PASS(FSS_STENCOP_KEEP) | STENCOP_CCW_PASS(FSS_STENCOP_KEEP),
    //      0xFF, 0xFFFFFFFF, nStencilWriteMask
    //      );
    //#else
    //    EF_SetStencilState(
    //      STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
    //      STENCOP_FAIL(FSS_STENCOP_KEEP) |
    //      STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
    //      STENCOP_PASS(FSS_STENCOP_KEEP),
    //      0xFF, 0xFFFFFFFF, nStencilWriteMask
    //      );
    //#endif
  }
  else
  {
    if (nStencilID<-1)
    {
      m_nStencilMaskRef--;
      assert(m_nStencilMaskRef>0 && m_nStencilMaskRef<=255);
    }
    else
    {
      m_nStencilMaskRef++;
      //m_nStencilMaskRef = m_nStencilMaskRef%255 + m_nStencilMaskRef/255;
      if (m_nStencilMaskRef>255)
      {
        EF_ClearBuffers(FRT_CLEAR_STENCIL|FRT_CLEAR_IMMEDIATE, NULL);
        m_nStencilMaskRef= 1;
      }
    }

    //TD: Fill stencil by values=1 for drawn volumes in order to avoid overdraw
    uint32 nCurrRef = m_nStencilMaskRef;
    D3DSetCull(eCULL_Front);
#if defined (DIRECT3D10)
    EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_GEQUAL) | STENC_CCW_FUNC(FSS_STENCFUNC_GEQUAL) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_CCW_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_REPLACE) | STENCOP_CCW_ZFAIL(FSS_STENCOP_REPLACE) |
      STENCOP_PASS(FSS_STENCOP_KEEP) | STENCOP_CCW_PASS(FSS_STENCOP_KEEP),
      nCurrRef, 0xFFFFFFFF, 0xFFFF
      );
#else
    EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_GEQUAL) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
      STENCOP_PASS(FSS_STENCOP_KEEP),
      nCurrRef, 0xFFFFFFFF, 0xFFFF
      );

#endif
  }

  EF_SetState( newState );


  FX_Commit();
#if defined (DIRECT3D9) || defined(OPENGL)
  m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, nVertOffs, 0, nNumVers, nIndOffs, nNumInds/3);
#elif defined (DIRECT3D10)
  SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_pd3dDeviceContext->DrawIndexed(nNumInds, nIndOffs, nVertOffs);
#endif

  m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += nNumInds/3;
  m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;

  //////////////////////////////////////////////////////////////////////////
  //draw front faces - decr when depth fail
  //////////////////////////////////////////////////////////////////////////
  if (nStencilID >= 0)
  {
//    uint32 nStencilWriteMask = 1 << nStencilID; //0..7
//    D3DSetCull(eCULL_Back);
//#if defined (DIRECT3D10)
//    EF_SetStencilState(
//      STENC_FUNC(FSS_STENCFUNC_ALWAYS) | STENC_CCW_FUNC(FSS_STENCFUNC_ALWAYS) | 
//      STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_CCW_FAIL(FSS_STENCOP_KEEP) |
//      STENCOP_ZFAIL(FSS_STENCOP_REPLACE) | STENCOP_CCW_ZFAIL(FSS_STENCOP_REPLACE) |
//      STENCOP_PASS(FSS_STENCOP_KEEP) | STENCOP_CCW_PASS(FSS_STENCOP_KEEP),
//      0x0, 0xFFFFFFFF, nStencilWriteMask
//    );
//#else
//    EF_SetStencilState(
//      STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
//      STENCOP_FAIL(FSS_STENCOP_KEEP) |
//      STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
//      STENCOP_PASS(FSS_STENCOP_KEEP),
//      0x0, 0xFFFFFFFF, nStencilWriteMask
//      );
//#endif
  }
  else
  {
    D3DSetCull(eCULL_Back);
//TD: deferred meshed should have proper front facing on dx10
#if defined (DIRECT3D10)
    EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_GEQUAL) | STENC_CCW_FUNC(FSS_STENCFUNC_GEQUAL) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_CCW_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_ZERO) | STENCOP_CCW_ZFAIL(FSS_STENCOP_ZERO) |
      STENCOP_PASS(FSS_STENCOP_KEEP) | STENCOP_CCW_PASS(FSS_STENCOP_KEEP),
      m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFF
      );
#else
    EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_GEQUAL) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_ZERO) |
      STENCOP_PASS(FSS_STENCOP_KEEP),
      m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFF
      );
#endif
  }

  //skip front faces when nStencilID is specified
  if (nStencilID <= 0)
  {
    EF_SetState( newState );
    FX_Commit();
  #if defined (DIRECT3D9) || defined(OPENGL)
    m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, nVertOffs, 0, nNumVers, nIndOffs, nNumInds/3);
  #elif defined (DIRECT3D10)
    SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pd3dDeviceContext->DrawIndexed(nNumInds, nIndOffs, nVertOffs);
  #endif
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += nNumInds/3;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
  }





  return;
}

void CD3D9Renderer::FX_StencilFrustumCull(int nStencilID, const CDLight* pLight, ShadowMapFrustum* pFrustum, int nAxis)
{
  EShapeMeshType nPrimitiveID = SHAPE_PROJECTOR;
  uint32 nPassCount = 0;
  CShader*  pShader = CShaderMan::m_ShaderShadowMaskGen;
  static CCryName StencilCullTechName = "DeferredShadowPass";

  Matrix44A mProjection;
  Matrix44A mView;

  Vec3 vOffsetDir(0,0,0);

  assert(pLight!=NULL);

  //un-projection matrix calc
  if(pFrustum == NULL)
  {

    if (pLight->m_Flags & DLF_HASCLIPBOUND)
    {



      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Push();

			Matrix44 mLocalTransposed;
			mLocalTransposed.Transpose(pLight->m_ClipBox);
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->MultMatrixLocal(&mLocalTransposed);

      pShader->FXSetTechnique(StencilCullTechName);
      pShader->FXBegin( &nPassCount, FEF_DONTSETSTATES );

      FX_SetVStream( 0, m_pUnitBoxVB, 0, sizeof( SVF_P3F_C4B_T2F ) );
      FX_SetIStream(m_pUnitBoxIB);

      pShader->FXBeginPass( DS_SHADOW_CULL_PASS );

      if (!FAILED(FX_SetVertexDeclaration( 0, eVF_P3F_C4B_T2F )))
        FX_StencilCullPass(-1, 0, m_UnitBoxVBSize, 0, m_UnitBoxIBSize);

      pShader->FXEndPass();

      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Pop();

      pShader->FXEnd();




      return;
    }
    else
    if (pLight->m_fProjectorNearPlane<0)
    {
      CDLight instLight = *pLight;
      vOffsetDir = (-pLight->m_fProjectorNearPlane) * (pLight->m_ObjMatrix.GetColumn0().GetNormalized());
      instLight.m_Origin -= vOffsetDir;
      instLight.m_fRadius -= pLight->m_fProjectorNearPlane;
      CShadowUtils::GetCubemapFrustumForLight(&instLight, nAxis, 160.0f,&mProjection,&mView, false); // 3.0f -  offset to make sure that frustums are intersected
    }
    else
    if( (pLight->m_Flags & DLF_PROJECT) &&  pLight->m_pLightImage && !(pLight->m_pLightImage->GetFlags()&FT_REPLICATE_TO_ALL_SIDES))
    {
      //projective light
      //refactor projective light

      //for light source
      CShadowUtils::GetCubemapFrustumForLight(pLight, nAxis, pLight->m_fLightFrustumAngle*2.f/*CShadowUtils::g_fOmniLightFov+3.0f*/,&mProjection,&mView, false); // 3.0f -  offset to make sure that frustums are intersected
    }
    else //omni light
    {
      //////////////// light sphere processing /////////////////////////////////



      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Push();

      float fExpensionRadius = pLight->m_fRadius*1.08f;

      Vec3 vScale(fExpensionRadius, fExpensionRadius, fExpensionRadius);
      Matrix34 mLocal;
      mLocal.SetIdentity();
      mLocal.SetScale( vScale, pLight->m_Origin );
      Matrix44 mLocalTransposed = GetTransposed44(Matrix44(mLocal));
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->MultMatrixLocal(&mLocalTransposed);

      pShader->FXSetTechnique(StencilCullTechName);
      pShader->FXBegin( &nPassCount, FEF_DONTSETSTATES );

      FX_SetVStream( 0, m_pUnitSphereVB, 0, sizeof( SVF_P3F_C4B_T2F ) );
      FX_SetIStream(m_pUnitSphereIB);

      //  shader pass
      pShader->FXBeginPass( DS_SHADOW_CULL_PASS );

      if (!FAILED(FX_SetVertexDeclaration( 0, eVF_P3F_C4B_T2F )))
        FX_StencilCullPass(-1, 0, m_UnitSphereVBSize, 0, m_UnitSphereIBSize);

      pShader->FXEndPass();

      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Pop();

      pShader->FXEnd();




      return;
      //////////////////////////////////////////////////////////////////////////
    }
  }
  else
  {
    if(!pFrustum->bOmniDirectionalShadow)
    {
      //temporarily disabled since mLightProjMatrix contains pre-multiplied matrix already
      //pmProjection = &(pFrustum->mLightProjMatrix);
      mProjection = gRenDev->m_IdentityMatrix;
      mView = pFrustum->mLightViewMatrix;
    }
    else
    {
      //calc one of cubemap's frustums
      Matrix33 mRot = ( Matrix33(pLight->m_ObjMatrix) );
      //rotation for shadow frustums is disabled
      CShadowUtils::GetCubemapFrustum(FTYP_OMNILIGHTVOLUME, pFrustum, nAxis, &mProjection,&mView/*, &mRot*/);
    }
  }

  //matrix concanation and inversion should be computed in doubles otherwise we have precision problems with big coords on big levels
  //which results to the incident frustum's discontinues for omni-lights
  Matrix44r mViewProj =  Matrix44r(mView) * Matrix44r(mProjection);
  Matrix44A mViewProjInv = mViewProj.GetInverted();

  gRenDev->m_TempMatrices[0][0].Transpose(mViewProjInv);

  //setup light source pos/radius
  m_cEF.m_TempVecs[5] = Vec4(pLight->m_Origin, pLight->m_fRadius * 1.1f); //increase radius slightly
  if (pLight->m_fProjectorNearPlane<0)
  {
    m_cEF.m_TempVecs[5].x -= vOffsetDir.x;
    m_cEF.m_TempVecs[5].y -= vOffsetDir.y;
    m_cEF.m_TempVecs[5].z -= vOffsetDir.z;
    nPrimitiveID = SHAPE_CLIP_PROJECTOR;
  }

  //if (m_pUnitFrustumVB==NULL || m_pUnitFrustumIB==NULL)
  //  CreateUnitFrustumMesh();

  FX_SetVStream( 0, m_pUnitFrustumVB[nPrimitiveID], 0, sizeof( SVF_P3F_C4B_T2F ) );
  FX_SetIStream(m_pUnitFrustumIB[nPrimitiveID]);

  pShader->FXSetTechnique(StencilCullTechName);
  pShader->FXBegin(&nPassCount, FEF_DONTSETSTATES) ;

  //  shader pass
  pShader->FXBeginPass( DS_SHADOW_FRUSTUM_CULL_PASS );

  if (pFrustum == NULL)
  {



  }

  if (!FAILED(FX_SetVertexDeclaration( 0, eVF_P3F_C4B_T2F )))
    FX_StencilCullPass(nStencilID, 0, m_UnitFrustVBSize[nPrimitiveID], 0, m_UnitFrustIBSize[nPrimitiveID]);

  if (pFrustum == NULL)
  {



  }

  pShader->FXEndPass();
  pShader->FXEnd();

  return;
}

void CD3D9Renderer::FX_StencilCull(int nStencilID, t_arrDeferredMeshIndBuff& arrDeferredInds, t_arrDeferredMeshVertBuff& arrDeferredVerts, CShader *pShader)
{
  PROFILE_FRAME(FX_StencilCull);

  int nVertOffs, nIndOffs;

  //CreateSimpleLightFrustumMesh(pShadowFrustum, nSide, arrDeferredInds, arrDeferredVerts);
  //CreateUnitFrustumMeshTransformed(pLight, pShadowFrustum, nSide, 10, 10, arrDeferredInds, arrDeferredVerts);

  //allocate vertices
  SVF_P3F_C4B_T2F  *pVerts( (SVF_P3F_C4B_T2F *) GetVBPtr( arrDeferredVerts.size(), nVertOffs, POOL_P3F_COL4UB_TEX2F) );
  memcpy( pVerts, &arrDeferredVerts[0], arrDeferredVerts.size()*sizeof(SVF_P3F_C4B_T2F ) );
  UnlockVB( POOL_P3F_COL4UB_TEX2F );

  //allocate indices
  uint16 *pInds = GetIBPtr(arrDeferredInds.size(), nIndOffs);
  memcpy( pInds, &arrDeferredInds[0], sizeof(uint16)*arrDeferredInds.size() );
  UnlockIB();

  FX_SetVStream( 0, m_pVB[ POOL_P3F_COL4UB_TEX2F ], 0, sizeof( SVF_P3F_C4B_T2F ) );
  FX_SetIStream(m_pIB);

  uint32 nPasses = 0;         
  //  shader pass
  pShader->FXBeginPass( DS_SHADOW_CULL_PASS );

  if (!FAILED(FX_SetVertexDeclaration( 0, eVF_P3F_C4B_T2F )))
    FX_StencilCullPass(nStencilID, nVertOffs, arrDeferredVerts.size(), nIndOffs, arrDeferredInds.size());

  pShader->FXEndPass();

}

bool CD3D9Renderer::FX_DeferredProjLights(int nGroup, bool bCheckValidMask)
{
  if (bCheckValidMask && SRendItem::m_ShadowsValidMask[SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][nGroup] )
    return false;

  // reset render element and current render object in pipeline
  m_RP.m_pRE = 0;
  m_RP.m_pCurObject = m_RP.m_Objects[0];
  m_RP.m_ObjFlags = 0;
  m_RP.m_FrameObject++;


  bool bWasDrawn = false;

  Matrix44A mCurComposite = *(m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->GetTop()) * *(m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop());
  Matrix44A mCurView = *(m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->GetTop()); 
  Matrix44A mCurProj = *(m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop());

  Matrix44A mQuadProj;
  //init matrix for deferred quads rendering
  mathMatrixOrthoOffCenterLH( &mQuadProj , 0, 1, 0, 1, -1, 1 );

  int TempX, TempY, TempWidth, TempHeight;
  GetViewport(&TempX, &TempY, &TempWidth, &TempHeight);

  CTexture *tpTarget = NULL;
  SDynTexture *pTempBlur = NULL;

  //FX_PushRenderTarget(0, tpTarget, &m_DepthBufferOrig);
  FX_SetShadowMaskRT(nGroup, 0, tpTarget, pTempBlur);

  int maskRTWidth = tpTarget->GetWidth();
  int maskRTHeight = tpTarget->GetHeight();


  if (bCheckValidMask)
  {
    ColorF clClear(0,0,0,0);
    EF_ClearBuffers(FRT_CLEAR_COLOR | FRT_CLEAR_STENCIL, &clClear);
  }


  // loop over all lights in this light group
  for( int i = 0; i < 4; ++i )
  {
    //note: bCheckValidMask is for sharing SRendItem::m_ShadowsValidMask info with shadows
    if (bCheckValidMask && SRendItem::m_ShadowsValidMask[SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][nGroup] & (1<<i))
      continue;

    uint32 nLightIndex =  i + nGroup * 4 ;

    //Disabled for now since temporal blur texture can be changed for next render list
    //So we can produce unnecessary shadow generating for some cases

    //select valid light sources
    //if ( !(pGr->m_GroupLightMask & (1<<nLightIndex)) )
    //  continue;

    //FIX this unnecessary check!
    if (nLightIndex>=m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1].Num())
      continue;

    // get list of shadow casters for nLightID
    CDLight* pLight = &m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][nLightIndex];

    //check for valid light to process
    if (!(pLight->m_Flags & DLF_PROJECT) || !(pLight->m_pLightImage) )
      continue;

#if defined (DIRECT3D9) || defined(OPENGL) //draw first LOD with stencil-fill enabled
    //TOFIX: add shadows stencil test variable
    //TOFIX: workaround
    //m_pd3dDevice->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 1.0f, 0);
    //EF_ClearBuffers(FRT_CLEAR_STENCIL, NULL, 1);
#endif //draw first LOD with stencil-fill enabled

    //fill light pass for projector setup
    m_RP.m_LPasses[0].nLights = 1;
    m_RP.m_LPasses[0].pLights[0] = pLight;

    //FIX: temp solution for projectors
    CDLight FakeBlackLight;
    FakeBlackLight.m_pLightImage = CTexture::s_ptexBlack;
    FakeBlackLight.m_Flags |= DLF_PROJECT;

    // set shader
    CShader *pSH( CShaderMan::m_ShaderShadowMaskGen );

    int nOffs;
    uint32 nPasses = 0;         
    static CCryName StencilCullTechName = "DeferredShadowPass";
    static CCryName LightTechName = "DeferredLightProj";


    PROFILE_SHADER_START

    int nSides = 1;
    //TOFIX: add case for omni-lights
    //if (ppSMFrustumList[0]->bOmniDirectionalShadow)
      nSides = 6;
    m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[ HWSR_SAMPLE0 ] | g_HWSR_MaskBit[ HWSR_SAMPLE1 ] | g_HWSR_MaskBit[ HWSR_SAMPLE2 ] | g_HWSR_MaskBit[ HWSR_SAMPLE3 ] | g_HWSR_MaskBit[HWSR_CUBEMAP0] );

    EF_ClearBuffers(FRT_CLEAR_STENCIL | FRT_CLEAR_IMMEDIATE, NULL, 1);
#if defined (DIRECT3D9)
    //m_pd3dDevice->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 1.0f, 0);
#else
    //assert(0);
#endif 

    pSH->FXSetTechnique(StencilCullTechName);
    pSH->FXBegin( &nPasses, FEF_DONTSETSTATES );

    for (int nS=0; nS<nSides; nS++)
    {
      //TODO: use light volumes IDs to avoid constant clearing
      assert(pLight!= NULL);

      t_arrDeferredMeshIndBuff arrDeferredInds;
      t_arrDeferredMeshVertBuff arrDeferredVerts;
      CDeferredRenderUtils::CreateUnitFrustumMeshTransformed(pLight, NULL, nS, 10, 10, arrDeferredInds, arrDeferredVerts);
      //use current WorldProj matrix
      FX_StencilCull(nS, arrDeferredInds, arrDeferredVerts, pSH);

    }
    pSH->FXEnd();

    // ortho projection matrix
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Push();
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->LoadMatrix(&mQuadProj);  
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Push();
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->LoadIdentity();
    EF_DirtyMatrix();

    FX_CreateDeferredQuad(nOffs, pLight, maskRTWidth, maskRTHeight, &mCurView, &mCurComposite);
    FX_SetVStream( 0, m_pVB[ POOL_P3F_TEX2F_TEX3F ], 0, sizeof( SVF_P3F_T2F_T3F ) ); 

    for (int nS=0; nS<nSides; nS++)
    {
      uint32 nStencilMask = 1<<nS;
      //FX_StencilRefresh(STENC_FUNC(FSS_STENCFUNC_EQUAL), 0xFFFF, nStencilMask);

      //FIX: temp workaround - fill all other sides by black texture
      if(nS>0) 
      {
        m_RP.m_LPasses[0].pLights[0] = &FakeBlackLight;
      }


      pSH->FXSetTechnique(LightTechName);
      pSH->FXBegin( &nPasses, FEF_DONTSETSTATES );

      //////////////////////////////////////////////////////////////////////////
      // set matrices
      //////////////////////////////////////////////////////////////////////////
      Matrix44 mProjector;
      CShadowUtils::GetProjectiveTexGen(pLight, nS, &mProjector);

      //same parameters for DeferredShadowVS
      gRenDev->m_TempMatrices[0][0] = GetTransposed44(mProjector);
      gRenDev->m_TempMatrices[0][1] = gRenDev->m_TempMatrices[0][0];
      m_cEF.m_TempVecs[1][0] = 0.0f; //disable bias
      m_cEF.m_TempVecs[2][0] = 1.f / (pLight->m_fRadius);
      //////////////////////////////////////////////////////////////////////////


      pSH->FXBeginPass(0);
      if (!FAILED(FX_SetVertexDeclaration( 0, eVF_P3F_T2F_T3F )))
      {
        int prevState = m_RP.m_CurState;
        int newState = prevState;
        //////////////////////////////////////////////////////////////////////////
        //trick to update zcull with this ref value for multi-lod shadows
  #if defined (DIRECT3D9)
        newState |= GS_COLMASK_NONE;
        newState |= GS_NODEPTHTEST;
        newState &= ~GS_DEPTHWRITE;
        newState |= GS_STENCIL;

        EF_SetStencilState(
          STENC_FUNC(FSS_STENCFUNC_EQUAL) |
          STENCOP_FAIL(FSS_STENCOP_KEEP) |
          STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
          STENCOP_PASS(FSS_STENCOP_KEEP),
          0xFFFF, nStencilMask, 0xFFFFFFFF
        );
        EF_SetState( newState );
        FX_Commit();

        m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);

  #if defined (DIRECT3D9) || defined(OPENGL)
        m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, nOffs, 2);
  #elif defined (DIRECT3D10)
        SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        m_pd3dDeviceContext->Draw(4, nOffs);
  #endif
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += 2;
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;

        //restore depth value
        EF_SetState(prevState, -1, -1);
        newState |= GS_DEPTHFUNC_LEQUAL;
#endif
        //////////////////////////////////////////////////////////////////////////


        newState |= GS_NODEPTHTEST;
        newState &= ~GS_DEPTHWRITE;
        newState |= GS_BLSRC_ONE | GS_BLDST_ONE;
        newState |= GS_STENCIL;

        //Set LS colormask
        newState &= ~GS_COLMASK_NONE;
        newState |= ( ( ~( 1 << i ) ) << GS_COLMASK_SHIFT ) & GS_COLMASK_MASK;

        EF_SetState( newState );
        FX_Commit();

        EF_SetStencilState(
          STENC_FUNC(FSS_STENCFUNC_EQUAL) |
          STENCOP_FAIL(FSS_STENCOP_KEEP) |
          STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
          STENCOP_PASS(FSS_STENCOP_KEEP),
          0xFFFF, nStencilMask, 0xFFFFFFFF
          );

  #if defined (DIRECT3D9) || defined(OPENGL)
        m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, nOffs, 2 );
  #elif defined (DIRECT3D10)
        SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        m_pd3dDeviceContext->Draw(4, nOffs);
  #endif

        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += 2;
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
      }
      pSH->FXEndPass();


      pSH->FXEnd();

    }
 
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Pop();

    EF_DirtyMatrix();

  //EF_SetState(~GS_STENCIL);
    EF_SetStencilState(
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
      STENCOP_PASS(FSS_STENCOP_KEEP) |
      STENC_FUNC(FSS_STENCFUNC_EQUAL),
      0x0, 0xFFFFFFFF, 0xFFFFFFFF
    );

    //shadow mask is valid flag 
    // overwrite
    SRendItem::m_ShadowsValidMask[SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][nGroup] |= (1 << i);
    bWasDrawn = true;

    PROFILE_SHADER_END
  }

  FX_PopRenderTarget(0);

  RT_SetViewport(TempX, TempY, TempWidth, TempHeight);

  //reset lightpass setup
  m_RP.m_LPasses[0].nLights = 0;
  m_RP.m_LPasses[0].pLights[0] = NULL;


  m_RP.m_FrameObject++;
  return bWasDrawn;
}

void CD3D9Renderer::FX_ResolveDepthTarget(CTexture* pSrcZTarget, SD3DSurface* pDstDepthBuffer)
{
#if defined (DIRECT3D10)
    int prevState = m_RP.m_CurState;

    CTexture *pTexColorBuff = NULL;
    CTexture::s_ptexCurrentScreenShadowMap[0]->Invalidate(pDstDepthBuffer->nWidth, pDstDepthBuffer->nHeight, eTF_A8R8G8B8);
    FX_PushRenderTarget(0, CTexture::s_ptexCurrentScreenShadowMap[0], pDstDepthBuffer);		// calls SetViewport() implicitly
    RT_SetViewport(0, 0, pDstDepthBuffer->nWidth, pDstDepthBuffer->nHeight);

    //calculate linear-device depth convertion ratios
    //first
    //float fDevDepth = (zf-(zn/SceneDepth))/(zf-zn);
    //second
    //float c1 = zf/(zf-zn);
    //float c2 = zn/(zn-zf);
    //float fDevDepth = c1 + c2/vProjRatios.y;

    float zn = GetRCamera().Near;
    float zf = GetRCamera().Far; 
    float c1 = zf/(zf-zn);
    float c2 = zn/(zn-zf);
    m_cEF.m_TempVecs[0].x = c1;
    m_cEF.m_TempVecs[0].y = c2;

    CShader *pSH( CShaderMan::m_ShaderShadowMaskGen );

    uint32 nPasses = 0;         
    static CCryName TechName = "ResolveDepthTarget";
    pSH->FXSetTechnique(TechName);
    pSH->FXBegin( &nPasses, FEF_DONTSETSTATES /*| FEF_DONTSETTEXTURES */);

    pSH->FXBeginPass(0);

    //////////////////////////////////////////////////////////////////////////
    float fWidth = (float)m_NewViewport.nWidth;
    float fHeight = (float)m_NewViewport.nHeight;

    int nVertexOffset;
    SVF_P3F_C4B_T2F *Verts = (SVF_P3F_C4B_T2F *)GetVBPtr(4, nVertexOffset, POOL_P3F_COL4UB_TEX2F);

    Vec2 vBRMin(0.0f, 0.0f), vBRMax(1.0f, 1.0f);

    assert(Verts!=NULL) ;
    if( Verts )
    {
      Verts->xyz = Vec3(0.0f, 0.0f, 0.0f);
      Verts->st = Vec2(vBRMin.x, 1 - vBRMin.y);
      Verts++;

      Verts->xyz = Vec3(fWidth, 0.0f, 0.0f);
      Verts->st = Vec2(vBRMax.x, 1 - vBRMin.y);
      Verts++;

      Verts->xyz = Vec3(0.0f, fHeight, 0.0f);
      Verts->st = Vec2(vBRMin.x, 1-vBRMax.y);
      Verts++;

      Verts->xyz = Vec3(fWidth, fHeight, 0.0f);
      Verts->st = Vec2(vBRMax.x, 1-vBRMax.y);
    }
    UnlockVB(POOL_P3F_COL4UB_TEX2F);

    const ColorF col(0,0,0,0);
    EF_ClearBuffers(FRT_CLEAR, &col);

    //FX_Commit();

    int newState = m_RP.m_CurState;

    newState = GS_DEPTHFUNC_LESS;

    //debug depth
    //newState |= GS_COLMASK_RGB;
    newState |= GS_COLMASK_NONE;
    newState &= ~GS_NODEPTHTEST;
    newState |= GS_DEPTHWRITE;
    //newState |= GS_WIREFRAME;
    EF_SetState( newState );

    D3DSetCull(eCULL_None);

    FX_Commit();


    if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
      FX_SetVStream(0, m_pVB[POOL_P3F_COL4UB_TEX2F], 0, sizeof(SVF_P3F_C4B_T2F));

      SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      m_pd3dDeviceContext->Draw(4, nVertexOffset);

      m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += 2;
      m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
    }
    pSH->FXEndPass();
    pSH->FXEnd();

    //restore previous state
    EF_SetState(prevState);
    FX_PopRenderTarget(0);
#endif
}


void CD3D9Renderer::FX_StencilRefresh(int StencilFunc, uint32 nStencRef, uint32 nStencMask)
{



  //for nvidia only
  bool bHiStencilRefresh = m_bDeviceSupports_NVDBT;


  //check for nvidia device
  if (bHiStencilRefresh)
  {
		int nVertexOffset;
		SVF_P3F_C4B_T2F *Verts = (SVF_P3F_C4B_T2F *)GetVBPtr(4, nVertexOffset, POOL_P3F_COL4UB_TEX2F);
		if( !Verts )
		{
			assert( Verts );
			return;
		}

    CShader *pSH( CShaderMan::m_ShaderShadowMaskGen );

    uint32 nPasses = 0;         
    static CCryName TechName = "DeferredSimpleQuad";
    pSH->FXSetTechnique(TechName);
    pSH->FXBegin( &nPasses, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES );

    pSH->FXBeginPass(0);

//////////////////////////////////////////////////////////////////////////
    float fWidth5 = (float)m_NewViewport.nWidth-0.5f;
    float fHeight5 = (float)m_NewViewport.nHeight-0.5f;

    Verts->xyz = Vec3(-0.5f, -0.5f, 0.0f);
    Verts->st = Vec2(0.0f, 0.0f);
    Verts++;

    Verts->xyz = Vec3(fWidth5, -0.5f, 0.0f);
    Verts->st = Vec2(0.0f, 0.0f);
    Verts++;

    Verts->xyz = Vec3(-0.5f, fHeight5, 0.0f);
    Verts->st = Vec2(0.0f, 0.0f);
    Verts++;

    Verts->xyz = Vec3(fWidth5, fHeight5, 0.0f);
    Verts->st = Vec2(0.0f, 0.0f);

		UnlockVB(POOL_P3F_COL4UB_TEX2F);

    EF_SetStencilState(
      StencilFunc |
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
      STENCOP_PASS(FSS_STENCOP_KEEP),
      nStencRef, nStencMask, 0xFFFFFFFF
      );
    D3DSetCull(eCULL_None);

    FX_Commit();

    if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
      FX_SetVStream(0, m_pVB[POOL_P3F_COL4UB_TEX2F], 0, sizeof(SVF_P3F_C4B_T2F));

      FX_HiStencilUpdate(nVertexOffset, -1, -1, -1, StencilFunc);
    }
    pSH->FXEndPass();
    pSH->FXEnd();
  }
}

void CD3D9Renderer::FX_StencilRefreshPartial(int StencilFunc, uint32 nStencRef, uint32 nStencMask,
                                             Vec3 *pVerts, uint32 nNumVerts, uint16 *pInds, uint32 nNumInds)
{
	// Use the backfaces of a specified volume to refresh hi-stencil

#if !defined(XENON) && !defined(PS3)
	if (!m_bDeviceSupports_NVDBT) return;  // For NVidia only
#endif

	int nVertexOffset, nIndOffset;
	
	SVF_P3F_C4B_T2F *Verts = (SVF_P3F_C4B_T2F *)GetVBPtr(nNumVerts, nVertexOffset, POOL_P3F_COL4UB_TEX2F);
	if (!Verts)
	{
		assert(Verts);
		return;
	}
	for (uint32 i = 0; i < nNumVerts; ++i)
	{
		Verts->xyz = pVerts[i];
		++Verts;
	}
	UnlockVB(POOL_P3F_COL4UB_TEX2F);
	
	uint16 *Inds = GetIBPtr(nNumInds, nIndOffset);
	if (!Inds)
	{
		assert(Inds);
		return;
	}
	memcpy(Inds, pInds, nNumInds*2);
	UnlockIB();

	uint32 nPasses = 0;         
	CShader *pSH( CShaderMan::m_ShaderShadowMaskGen );
	static CCryName TechName = "StencilVolume";
	pSH->FXSetTechnique(TechName);
	pSH->FXBegin( &nPasses, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES );

	pSH->FXBeginPass(0);

	EF_SetStencilState(
		StencilFunc |
		STENCOP_FAIL(FSS_STENCOP_KEEP) |
		STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
		STENCOP_PASS(FSS_STENCOP_KEEP),
		nStencRef, nStencMask, 0xFFFFFFFF
		);
	D3DSetCull(eCULL_Front);

	FX_Commit();

	if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
	{
		FX_SetVStream(0, m_pVB[POOL_P3F_COL4UB_TEX2F], 0, sizeof(SVF_P3F_C4B_T2F));
		FX_SetIStream(m_pIB);

		FX_HiStencilUpdate(nVertexOffset, nNumVerts, nIndOffset, nNumInds);
	}
	D3DSetCull(eCULL_None);
	pSH->FXEndPass();
	pSH->FXEnd();
}

void CD3D9Renderer::FX_StencilCullPassForLightGroup(int nLightGroup)
{


  CShader *pSH( CShaderMan::m_ShaderShadowMaskGen );
  static CCryName TechName = "DeferredShadowPass";

  uint32 nPasses = 0;

  if (m_pUnitFrustumVB[0]==NULL || m_pUnitFrustumIB[0]==NULL)
  {
    //bool bCreated = CreateUnitFrustumMesh();
    //if (!bCreated)
    {
      assert(0);
      return;
    }
  }

  if (m_pUnitSphereVB==NULL || m_pUnitSphereIB==NULL)
  {
    //bool bCreated = CreateUnitLightMeshes();
    //if (!bCreated)
    {
      assert(0);
      return;
    }
  }


  //always force to disable current scissor test
  EF_Scissor(false, 0, 0, 0, 0);

  //TOFIX: prevents non 4 light aligned lightgroups processing
  for( int i = 0; i < 4; ++i )
  {
    uint32 nLightIndex =  i + nLightGroup * 4 ;

    //select valid light sources
    //if ( !(pGr->m_GroupLightMask & (1<<nLightIndex)) )
    //continue;

    if (nLightIndex>=m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1].Num())
      continue;

    CDLight* pLight = &m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][nLightIndex];

    //processing projective & omni light
    FX_StencilFrustumCull(i, pLight, NULL, 0);
  }


  return;
}

void CD3D9Renderer::FX_CreateDeferredQuad(int& nOffs, const CDLight* pLight, float maskRTWidth, float maskRTHeight, Matrix44A* pmCurView, Matrix44A* pmCurComposite)
{
  //////////////////////////////////////////////////////////////////////////
  // Create FS quad
  //////////////////////////////////////////////////////////////////////////
  SVF_P3F_T2F_T3F *vQuad( (SVF_P3F_T2F_T3F *) GetVBPtr( 4, nOffs, POOL_P3F_TEX2F_TEX3F ) );


  Vec2 vBoundRectMin(0.0f, 0.0f), vBoundRectMax(1.0f, 1.0f);

  Vec3 vCoords[8];

  Vec3 vRT, vLT, vLB, vRB;


  //calc screen quad
  if (!(pLight->m_Flags & DLF_DIRECTIONAL))
  {
    if(CV_r_ShowLightBounds)
      CShadowUtils::CalcLightBoundRect(pLight, GetRCamera(), *pmCurView, *pmCurComposite, &vBoundRectMin, &vBoundRectMax, gRenDev->GetIRenderAuxGeom());
    else
      CShadowUtils::CalcLightBoundRect(pLight, GetRCamera(), *pmCurView, *pmCurComposite, &vBoundRectMin, &vBoundRectMax, NULL);

    if (m_RenderTileInfo.nGridSizeX > 1.f || m_RenderTileInfo.nGridSizeY > 1.f)
    {

      //always render full-screen quad for hi-res screenshots
      gcpRendD3D->GetRCamera().CalcTileVerts( vCoords,  
        m_RenderTileInfo.nGridSizeX-1-m_RenderTileInfo.nPosX, 
        m_RenderTileInfo.nPosY, 
        m_RenderTileInfo.nGridSizeX,
        m_RenderTileInfo.nGridSizeY);
      vBoundRectMin = Vec2(0.0f, 0.0f);
      vBoundRectMax = Vec2(1.0f, 1.0f);

      vRT = vCoords[4] - vCoords[0];
      vLT = vCoords[5] - vCoords[1];
      vLB = vCoords[6] - vCoords[2];
      vRB = vCoords[7] - vCoords[3];

      //////////////////////////////////////////////////////////////////////////
      /*gcpRendD3D->GetRCamera().CalcTiledRegionVerts( vCoords, vBoundRectMin, vBoundRectMax,
        m_RenderTileInfo.nGridSizeX-1-m_RenderTileInfo.nPosX, 
        m_RenderTileInfo.nPosY, 
        m_RenderTileInfo.nGridSizeX,
        m_RenderTileInfo.nGridSizeY)*/;


      //////////////////////////////////////////////////////////////////////////
      /*Vec3 vFarPlaneVerts[4];
      UnProjectFromScreen( vBoundRectMax.x * m_width, vBoundRectMax.y * m_height, 1, &vFarPlaneVerts[0].x, &vFarPlaneVerts[0].y, &vFarPlaneVerts[0].z);
      UnProjectFromScreen( vBoundRectMin.x * m_width, vBoundRectMax.y	* m_height, 1, &vFarPlaneVerts[1].x, &vFarPlaneVerts[1].y, &vFarPlaneVerts[1].z);
      UnProjectFromScreen( vBoundRectMin.x * m_width, vBoundRectMin.y	* m_height, 1, &vFarPlaneVerts[2].x, &vFarPlaneVerts[2].y, &vFarPlaneVerts[2].z);
      UnProjectFromScreen( vBoundRectMax.x * m_width, vBoundRectMin.y	* m_height, 1, &vFarPlaneVerts[3].x, &vFarPlaneVerts[3].y, &vFarPlaneVerts[3].z);

      vRT = vFarPlaneVerts[0] - GetCamera().GetPosition();
      vLT = vFarPlaneVerts[1] - GetCamera().GetPosition();
      vLB = vFarPlaneVerts[2] - GetCamera().GetPosition();
      vRB = vFarPlaneVerts[3] - GetCamera().GetPosition();*/

    }
    else
    {
      GetRCamera().CalcRegionVerts(vCoords, vBoundRectMin, vBoundRectMax);
      vRT = vCoords[4] - vCoords[0];
      vLT = vCoords[5] - vCoords[1];
      vLB = vCoords[6] - vCoords[2];
      vRB = vCoords[7] - vCoords[3];
    }
  }
  else //full screen case for directional light
  {
    if (m_RenderTileInfo.nGridSizeX > 1.f || m_RenderTileInfo.nGridSizeY > 1.f)
	    gcpRendD3D->GetRCamera().CalcTileVerts( vCoords,  
				    m_RenderTileInfo.nGridSizeX-1-m_RenderTileInfo.nPosX, 
				    m_RenderTileInfo.nPosY, 
				    m_RenderTileInfo.nGridSizeX,
				    m_RenderTileInfo.nGridSizeY);
    else
	    gcpRendD3D->GetRCamera().CalcVerts( vCoords );

    vRT = vCoords[4] - vCoords[0];
    vLT = vCoords[5] - vCoords[1];
    vLB = vCoords[6] - vCoords[2];
    vRB = vCoords[7] - vCoords[3];

  }


#if defined (DIRECT3D10)
  float offsetX( 0 );
  float offsetY( 0 );
#else
  float offsetX( 0.5f / (float) maskRTWidth );
  float offsetY( -0.5f / (float) maskRTHeight );
#endif


  vQuad[0].p.x = vBoundRectMin.x - offsetX;
  vQuad[0].p.y = vBoundRectMin.y - offsetY;
  vQuad[0].p.z = 0;
  vQuad[0].st0[0] = vBoundRectMin.x;
  vQuad[0].st0[1] = 1 - vBoundRectMin.y;
  vQuad[0].st1 = vLB;

  vQuad[1].p.x = vBoundRectMax.x - offsetX;
  vQuad[1].p.y = vBoundRectMin.y - offsetY;
  vQuad[1].p.z = 0;
  vQuad[1].st0[0] = vBoundRectMax.x;
  vQuad[1].st0[1] = 1 - vBoundRectMin.y;
  vQuad[1].st1 = vRB;

  vQuad[3].p.x = vBoundRectMax.x - offsetX;
  vQuad[3].p.y = vBoundRectMax.y - offsetY;
  vQuad[3].p.z = 0;
  vQuad[3].st0[0] = vBoundRectMax.x;
  vQuad[3].st0[1] = 1-vBoundRectMax.y;
  vQuad[3].st1 = vRT;

  vQuad[2].p.x = vBoundRectMin.x - offsetX;
  vQuad[2].p.y = vBoundRectMax.y - offsetY;
  vQuad[2].p.z = 0;
  vQuad[2].st0[0] = vBoundRectMin.x;
  vQuad[2].st0[1] = 1-vBoundRectMax.y;
  vQuad[2].st1 = vLT;

  #if defined(OPENGL)
  // XXX Untested!
  vQuad[0].st1 = vLT;
  vQuad[1].st1 = vRT;
  vQuad[2].st1 = vRB;
  vQuad[3].st1 = vLB;
  #endif

  UnlockVB( POOL_P3F_TEX2F_TEX3F );
}


bool CD3D9Renderer::FX_DeferredShadows( int nGroup, SRendLightGroup* pGr, int maskRTWidth, int maskRTHeight )
{
  // reset render element and current render object in pipeline
  m_RP.m_pRE = 0;
  m_RP.m_pCurObject = m_RP.m_Objects[0];
  m_RP.m_ObjFlags = 0;
  m_RP.m_FrameObject++;

  m_RP.m_FlagsShader_RT = 0;
  m_RP.m_FlagsShader_LT = 0;
  m_RP.m_FlagsShader_MD = 0;
  m_RP.m_FlagsShader_MDV = 0;

  int nThreadID = m_RP.m_nProcessThreadID;
  int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID]-1;

  bool bWasDrawn = false;

  //Temp reset
  bool bResetPipe = true;





  Matrix44A mCurComposite = *(m_RP.m_TI[nThreadID].m_matView->GetTop()) * *(m_RP.m_TI[nThreadID].m_matProj->GetTop());
  Matrix44A mCurView = *(m_RP.m_TI[nThreadID].m_matView->GetTop()); 
  Matrix44A mCurProj = *(m_RP.m_TI[nThreadID].m_matProj->GetTop());

  //set ScreenToWorld Expansion Basis
  Vec3 vWBasisX, vWBasisY, vWBasisZ;
  bool bVPosSM30 = (GetFeatures() & (RFT_HW_PS30|RFT_HW_PS40))!=0;
  CShadowUtils::CalcScreenToWorldExpansionBasis(GetCamera(), (float)maskRTWidth, (float)maskRTHeight, vWBasisX, vWBasisY, vWBasisZ, bVPosSM30);
  m_cEF.m_TempVecs[10] = Vec4(vWBasisX, 1.0f);
  m_cEF.m_TempVecs[11] = Vec4(vWBasisY, 1.0f);
  m_cEF.m_TempVecs[12] = Vec4(vWBasisZ, 1.0f);

  Matrix44A mQuadProj;
  //init matrix for deferred quads rendering
	mathMatrixOrthoOffCenterLH( &mQuadProj , 0, 1, 0, 1, -1, 1 );

  // loop over all lights in this light group
	for( int i = 0; i < 4; ++i )
  {
    if (SRendItem::m_ShadowsValidMask[nCurRecLevel][nGroup] & (1<<i))
      continue;

    uint32 nLightIndex =  i + nGroup * 4 ;


    //Disabled for now since temporal blur texture can be changed for next render list
    //So we can produce unnecessary shadow generating for some cases

    //select valid light sources
    //if ( !(pGr->m_GroupLightMask & (1<<nLightIndex)) )
    //  continue;

    //FIX this unnecessary check!
    if (nLightIndex>=m_RP.m_DLights[nThreadID][nCurRecLevel].Num())
      continue;
    
    // get list of shadow casters for nLightID
    CDLight* pLight = &m_RP.m_DLights[nThreadID][nCurRecLevel][nLightIndex];

    //check for valid light to process
    if (!(pLight->m_Flags & DLF_CASTSHADOW_MAPS /*|| pLight->m_Flags & DLF_PROJECT*/))
      continue;

    if (bResetPipe)
    {
      FX_ResetPipe();
      FX_Commit();
      bResetPipe = false;
    }

    ShadowMapFrustum* pSMFrustumList = NULL;
    ShadowMapFrustum** ppSMFrustumList = pLight->m_pShadowMapFrustums;

    //tmp workaround
    ShadowMapFrustum* TmpSMFrustumList[MAX_SHADOWMAP_LOD];

		if (!ppSMFrustumList)
			continue;

		assert( ppSMFrustumList != 0);

    // determine number of casters
    int nCasters = 0;


    float fFinalRange[MAX_GSM_LODS_NUM];
	
    //FIX: replace m_pShadowMapFrustums in CDLight by array or list 

		bool bTerrainShadows = false;
		bool bOmniShadows = false;

    ShadowMapFrustum* pLastValidGsmFrustum = NULL;


    //////////////////////////////////////////////////////////////////////////
    //check for valid gsm frustums
    //////////////////////////////////////////////////////////////////////////
    int nStartIdx = SRendItem::m_StartFrust[nThreadID][nLightIndex];
    int nEndIdx = SRendItem::m_EndFrust[nThreadID][nLightIndex];
    assert((nEndIdx-nStartIdx)<=MAX_GSM_LODS_NUM);
    for(int nFrustIdx=nStartIdx; nFrustIdx < nEndIdx; ++nFrustIdx, ++nCasters )
    {       
    //for(/*int nCasterMax = 0 */; *ppSMFrustumList && nCasters!=MAX_GSM_LODS_NUM; ++ppSMFrustumList, ++nCasters/*++nCasterMax*/ )
    //{       
      pSMFrustumList = &m_RP.m_SMFrustums[nThreadID][nCurRecLevel][nFrustIdx];
      ppSMFrustumList = &pSMFrustumList;

//////////////////////////////////////////////////////////////////////////
      /*int nShadowRecur = (pSMFrustumList->nDLightId*MAX_SHADOWMAP_LOD);
      if (!pSMFrustumList->bOmniDirectionalShadow)
      {
        nShadowRecur += nFrustIdx;
      }

      int nFirstShadowGenRI = SRendItem::m_ShadowsStartRI[nThreadID][nShadowRecur];
      int nLastShadowGenRI = SRendItem::m_ShadowsEndRI[nThreadID][nShadowRecur];
      if (nLastShadowGenRI-nFirstShadowGenRI<1)
        continue;*/
//////////////////////////////////////////////////////////////////////////

      TmpSMFrustumList[nCasters] = pSMFrustumList;

      assert( (*ppSMFrustumList)->nDLightId == nLightIndex );          

      //calc DBT's ranges
      if (CV_r_ShadowsDepthBoundNV && m_bDeviceSupports_NVDBT)
      {





        fFinalRange[nCasters] = 1.0f;//vClipSp.z/vClipSp.w;

      }

      //stop enumeration by any non-gsm frustums
      if( (*ppSMFrustumList)->bUseAdditiveBlending ) 
      {
        if (nCasters!=MAX_GSM_LODS_NUM) 
        {
          ++nCasters;
          bTerrainShadows = true;
        }
        
        break;
      }
      else if( (*ppSMFrustumList)->bForSubSurfScattering) 
      {
        break;
      }
      else if ((*ppSMFrustumList)->pCastersList != NULL)
      {
        //TD: change from pointer to index
        pLastValidGsmFrustum = (*ppSMFrustumList);
      }
    }

		if( nCasters == 0)
      continue;

    //temp workaround
    ppSMFrustumList = TmpSMFrustumList;


    //////////////////////////////////////////////////////////////////////////
    //check for valid point light frustums
    //////////////////////////////////////////////////////////////////////////
    if (nCasters==1 && (ppSMFrustumList[0]->bOmniDirectionalShadow || !(pLight->m_Flags & DLF_DIRECTIONAL)))
    {
      if (ppSMFrustumList[0]->pCastersList == NULL)
      {
        continue;
      }
    }


#if defined (DIRECT3D9) || defined(OPENGL) //draw first LOD with stencil-fill enabled
    //TOFIX: add shadows stencil test variable
    //TOFIX: workaround
    //m_pd3dDevice->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 1.0f, 0);
    //EF_ClearBuffers(FRT_CLEAR_STENCIL, NULL, 1);
#endif //draw first LOD with stencil-fill enabled

    // set shader
    int nOffs;

    CShader *pSH( CShaderMan::m_ShaderShadowMaskGen );
    uint32 nPasses = 0;         
    static CCryName TechName = "DeferredShadowPass";

    PROFILE_SHADER_START








    if (nCasters==1 && (ppSMFrustumList[0]->bOmniDirectionalShadow || !(pLight->m_Flags & DLF_DIRECTIONAL)))
    {

      if (CV_r_UseShadowsPool /*&& ppSMFrustumList[0].bUseHWShadowMap*/)
      {
        PrepareDepthMap(ppSMFrustumList[0], pLight , 0);
      }

      if (CV_r_ShadowsDeferredMode == 1)
      {
        int nSides = 1;
        if (ppSMFrustumList[0]->bOmniDirectionalShadow)
          nSides = 6;

        for (int nS=0; nS<nSides; nS++)
        {
          //TODO: use light volumes IDs to avoid constant clearing
          assert(ppSMFrustumList[0]!= NULL);
          assert(pLight!= NULL);

          //FIX: temp solution for projector's camera
          //TF enable linear shadow space and disable this back faces for projectors
          if (pLight->m_Flags & DLF_PROJECT)
          {
            m_RP.m_TI[nThreadID].m_PersFlags &= ~RBPF_MIRRORCULL;
          }
          else
          {
            m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_MIRRORCULL;
          }

          //use current WorldProj matrix
          FX_StencilFrustumCull(-1, pLight, ppSMFrustumList[0], nS);
          //FIX: temp solution for projector's camera
          m_RP.m_TI[nThreadID].m_PersFlags &= ~RBPF_MIRRORCULL;

          // ortho projection matrix
          m_RP.m_TI[nThreadID].m_matProj->Push();
          m_RP.m_TI[nThreadID].m_matProj->LoadMatrix(&mQuadProj);  
          m_RP.m_TI[nThreadID].m_matView->Push();
          m_RP.m_TI[nThreadID].m_matView->LoadIdentity();
          EF_DirtyMatrix();

          FX_CreateDeferredQuad(nOffs, pLight, maskRTWidth, maskRTHeight, &mCurView, &mCurComposite);
          FX_SetVStream( 0, m_pVB[ POOL_P3F_TEX2F_TEX3F ], 0, sizeof( SVF_P3F_T2F_T3F ) );

          //use full range for now
          FX_DeferredShadowPass( pLight, i, ppSMFrustumList[0], 1.0f, nOffs, true, false, -1, nS); //fFinalRange[0]

          m_RP.m_TI[nThreadID].m_matProj->Pop();
          m_RP.m_TI[nThreadID].m_matView->Pop();
          EF_DirtyMatrix();
        }

      }
      else if (CV_r_ShadowsDeferredMode == 2)
      {
        int nSides = 1;
        if (ppSMFrustumList[0]->bOmniDirectionalShadow)
          nSides = 6;

#if defined (DIRECT3D9)
        m_pd3dDevice->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 1.0f, 0);
#else
        EF_ClearBuffers(FRT_CLEAR_STENCIL | FRT_CLEAR_IMMEDIATE, NULL, 1);
        //assert(0);
#endif 

        //merged projector light passes
        //fill light pass for projector setup















        //FIX: temp solution for projector's camera
        //TF enable linear shadow space and disable this back faces for projectors
        if (pLight->m_Flags & DLF_PROJECT)
        {
          m_RP.m_TI[nThreadID].m_PersFlags &= ~RBPF_MIRRORCULL;
        }
        else
        {
          m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_MIRRORCULL;
        }

        for (int nS=0; nS<nSides; nS++)
        {
          //TODO: use light volumes IDs to avoid constant clearing
          assert(ppSMFrustumList[0]!= NULL);
          assert(pLight!= NULL);

          //t_arrDeferredMeshIndBuff arrDeferredInds;
          //t_arrDeferredMeshVertBuff arrDeferredVerts;
          //CreateUnitFrustumMeshTransformed(pLight, ppSMFrustumList[0], nS, 10, 10, arrDeferredInds, arrDeferredVerts);

          //use current WorldProj matrix
          FX_StencilFrustumCull(nS, pLight, ppSMFrustumList[0], nS);

        }

        //FIX: temp solution for projector's camera
        m_RP.m_TI[nThreadID].m_PersFlags &= ~RBPF_MIRRORCULL;

        // ortho projection matrix
        m_RP.m_TI[nThreadID].m_matProj->Push();
        m_RP.m_TI[nThreadID].m_matProj->LoadMatrix(&mQuadProj);  
        m_RP.m_TI[nThreadID].m_matView->Push();
        m_RP.m_TI[nThreadID].m_matView->LoadIdentity();
        EF_DirtyMatrix();

        FX_CreateDeferredQuad(nOffs, pLight, maskRTWidth, maskRTHeight, &mCurView, &mCurComposite);
        FX_SetVStream( 0, m_pVB[ POOL_P3F_TEX2F_TEX3F ], 0, sizeof( SVF_P3F_T2F_T3F ) );
        for (int nS=0; nS<nSides; nS++)
        {
          FX_DeferredShadowPass( pLight, i, ppSMFrustumList[0], fFinalRange[0], nOffs, true, false, (nS+1), nS);
        }

        m_RP.m_TI[nThreadID].m_matProj->Pop();
        m_RP.m_TI[nThreadID].m_matView->Pop();
        EF_DirtyMatrix();
      }
      else
      {
        // ortho projection matrix
        m_RP.m_TI[nThreadID].m_matProj->Push();
        m_RP.m_TI[nThreadID].m_matProj->LoadMatrix(&mQuadProj);
        m_RP.m_TI[nThreadID].m_matView->Push();
        m_RP.m_TI[nThreadID].m_matView->LoadIdentity();

        //single pass per-each omni lighsource without stencil cull pass for omni lights and spot lights
        FX_CreateDeferredQuad(nOffs, pLight, maskRTWidth, maskRTHeight, &mCurView, &mCurComposite);
        FX_SetVStream( 0, m_pVB[ POOL_P3F_TEX2F_TEX3F ], 0, sizeof( SVF_P3F_T2F_T3F ) );
        FX_DeferredShadowPass( pLight, i, ppSMFrustumList[0], fFinalRange[0], nOffs, true, false, 0 );

        m_RP.m_TI[nThreadID].m_matProj->Pop();
        m_RP.m_TI[nThreadID].m_matView->Pop();
      }

    }
    else //GSM shadows
    {
#if defined (DIRECT3D9)
      DWORD cColor = D3DRGBA(0.0f, 0.0f, 0.0f, 0.0f);
      m_pd3dDevice->Clear(0, NULL, D3DCLEAR_STENCIL, cColor, 1.0f, 0);
#else
      EF_ClearBuffers(FRT_CLEAR_STENCIL | FRT_CLEAR_IMMEDIATE, NULL, 1);
      //assert(0);  
#endif

      if (!CV_r_ShadowsUseClipVolume)
      {
        m_RP.m_TI[nThreadID].m_matProj->Push();
        m_RP.m_TI[nThreadID].m_matProj->LoadMatrix(&mQuadProj);
        m_RP.m_TI[nThreadID].m_matView->Push();
        m_RP.m_TI[nThreadID].m_matView->LoadIdentity();
        FX_CreateDeferredQuad(nOffs, pLight, maskRTWidth, maskRTHeight, &mCurView, &mCurComposite);
        FX_SetVStream( 0, m_pVB[ POOL_P3F_TEX2F_TEX3F ], 0, sizeof( SVF_P3F_T2F_T3F ) );
      }

      //temporal workaround for processing whithout terrain shadows
      if (!bTerrainShadows)
        nCasters++;

      // loop over all casters to generate shadow mask value for light i
      if (CV_r_ShadowsStencilPrePass)
      {
        //stencil pre-passes
        for( int nCaster=(nCasters-2); nCaster>=0; nCaster-- )
  		  {
          FX_DeferredShadowPass(pLight, i, ppSMFrustumList[ nCaster ], fFinalRange[nCaster], nOffs, false, true, (nCaster+1) );
		    }

        if (CV_r_ShadowsUseClipVolume)
        {
          m_RP.m_TI[nThreadID].m_matProj->Push();
          m_RP.m_TI[nThreadID].m_matProj->LoadMatrix(&mQuadProj);
          m_RP.m_TI[nThreadID].m_matView->Push();
          m_RP.m_TI[nThreadID].m_matView->LoadIdentity();

          FX_CreateDeferredQuad(nOffs, pLight, maskRTWidth, maskRTHeight, &mCurView, &mCurComposite);
          FX_SetVStream( 0, m_pVB[ POOL_P3F_TEX2F_TEX3F ], 0, sizeof( SVF_P3F_T2F_T3F ) );
        }

        //shadows passes
        for( int nCaster=0; nCaster<(nCasters-1); nCaster++/*, m_RP.m_PS[rd->m_RP.m_nProcessThreadID]. ++ */) // for non-conservative 
        {

          //SDW-CFG_PRM
          //overload fading distance for now since we do shadowgen area based fading
          if (ppSMFrustumList[ nCaster ] == pLastValidGsmFrustum)
          {
            ppSMFrustumList[ nCaster ]->fShadowFadingDist = 1.0f;
          }
          else
          {
            ppSMFrustumList[ nCaster ]->fShadowFadingDist = 0.0f;
          }
#ifdef DO_RENDERLOG
          if (CRenderer::CV_r_log >= 3)
          {
              Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " Set Shadow Fading dist = %.3f)\n", ppSMFrustumList[ nCaster ]->fShadowFadingDist);
          }
#endif

          if (CV_r_UseShadowsPool /*&& ppSMFrustumList[ nCaster ].bUseHWShadowMap*/)
          {

            PrepareDepthMap(ppSMFrustumList[ nCaster ], pLight);

            m_RP.m_FlagsShader_RT = 0;
            m_RP.m_FlagsShader_LT = 0;
            m_RP.m_FlagsShader_MD = 0;
            m_RP.m_FlagsShader_MDV = 0;


            m_RP.m_TI[nThreadID].m_matProj->Push();
            m_RP.m_TI[nThreadID].m_matProj->LoadMatrix(&mQuadProj);
            m_RP.m_TI[nThreadID].m_matView->Push();
            m_RP.m_TI[nThreadID].m_matView->LoadIdentity();

            FX_CreateDeferredQuad(nOffs, pLight, maskRTWidth, maskRTHeight, &mCurView, &mCurComposite);
            FX_SetVStream( 0, m_pVB[ POOL_P3F_TEX2F_TEX3F ], 0, sizeof( SVF_P3F_T2F_T3F ) );


          }

          FX_DeferredShadowPass(pLight, i, ppSMFrustumList[ nCaster ], fFinalRange[nCaster], nOffs, true, false, (nCaster+1) );

          if (CV_r_UseShadowsPool /*&& ppSMFrustumList[ nCaster ].bUseHWShadowMap*/)
          {
            m_RP.m_TI[nThreadID].m_matProj->Pop();
            m_RP.m_TI[nThreadID].m_matView->Pop();
          }


        }
        if (CV_r_ShadowsUseClipVolume)
        {
          m_RP.m_TI[nThreadID].m_matProj->Pop();
          m_RP.m_TI[nThreadID].m_matView->Pop();
        }

      }
      else
      {
        //DX11 path
        for( int nCaster=(nCasters-2); nCaster>=0; nCaster--)
        {
          FX_DeferredShadowPass(pLight, i, ppSMFrustumList[ nCaster ], fFinalRange[nCaster], nOffs, true, false, 0);
        }
      }

      //terrain shadows
      //TOFIX: we assume that there are shadows from mountains all the time as a last frustum
      // should not be processed for indoors
      if (bTerrainShadows)
      {
        if (CV_r_ShadowsUseClipVolume)
        {
          m_RP.m_TI[nThreadID].m_matProj->Push();
          m_RP.m_TI[nThreadID].m_matProj->LoadMatrix(&mQuadProj);
          m_RP.m_TI[nThreadID].m_matView->Push();
          m_RP.m_TI[nThreadID].m_matView->LoadIdentity();

          FX_CreateDeferredQuad(nOffs, pLight, maskRTWidth, maskRTHeight, &mCurView, &mCurComposite);
          FX_SetVStream( 0, m_pVB[ POOL_P3F_TEX2F_TEX3F ], 0, sizeof( SVF_P3F_T2F_T3F ) );
        }

        //enable fading for terrain GSM
        ppSMFrustumList[ nCasters-1 ]->fShadowFadingDist = 1.0f;
		    FX_DeferredShadowPass(pLight, i, ppSMFrustumList[ nCasters-1 ], fFinalRange[ nCasters-1 ], nOffs, true, false, 0);

        if (CV_r_ShadowsUseClipVolume)
        {
          m_RP.m_TI[nThreadID].m_matProj->Pop();
          m_RP.m_TI[nThreadID].m_matView->Pop();
        }
      }

      if (!CV_r_ShadowsUseClipVolume)
      {
        m_RP.m_TI[nThreadID].m_matProj->Pop();
        m_RP.m_TI[nThreadID].m_matView->Pop();
      }

      //reset stencil back
#if defined (DIRECT3D9)
      m_pd3dDevice->Clear(0, NULL, D3DCLEAR_STENCIL, cColor, 1.0f, 0);
#else
      EF_ClearBuffers(FRT_CLEAR_STENCIL | FRT_CLEAR_IMMEDIATE, NULL, 1);
      //assert(0);  
#endif
    }
    EF_DirtyMatrix();

    if (CV_r_ShadowsDepthBoundNV && m_bDeviceSupports_NVDBT)
    {
      SetDepthBoundTest(0.0f, 0.0f, false);
    }

    //stencil optimization
    //EF_SetState(~GS_STENCIL);
#if defined (DIRECT3D9) || defined(OPENGL)
    EF_SetStencilState(
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
      STENCOP_PASS(FSS_STENCOP_KEEP) |
      STENC_FUNC(FSS_STENCFUNC_EQUAL),
      0x0, 0xFFFFFFFF, 0xFFFFFFFF
      );
#elif defined (DIRECT3D10)
    //assert(0);  //transparent execution without NVDB
#endif








    //shadow mask is valid
    SRendItem::m_ShadowsValidMask[nCurRecLevel][nGroup] |= (1 << i);

    bWasDrawn = true;

    PROFILE_SHADER_END
  }





  m_RP.m_FrameObject++;
  return bWasDrawn;
}


  
//The D3DRS_ZFUNC render state is compatible with the current depth-stencil surface's HiZFunc value. 
//If using a fixed-point depth buffer, the D3DRS_ZFUNC render state can be D3DCMP_LESS, D3DCMP_LESSEQUAL, or D3DCMP_EQUAL. 
//If using a floating-point depth buffer, D3DRS_ZFUNC render state can be D3DCMP_GREATER, D3DCMP_GREATEREQUAL, or D3DCMP_EQUAL.

//The current pixel shader does not perform depth-export.
 
//Stencil testing is not enabled or stencil testing is enabled and 
//the D3DRS_STENCILFAIL render state and the D3DRS_STENCILZFAIL render state are set to D3DSTENCILOP_KEEP. 
//If the D3DRS_TWOSIDEDSTENCILMODE render state is enabled, 
//the D3DRS_CCW_STENCILFAIL render state and the D3DRS_CCW_STENCILZFAIL render state must also be set to D3DSTENCILOP_KEEP.
 
//If you are using a D3DFMT_D24S8 or D3DFMT_D24X8 surface, use a value such as D3DCMP_LESS, D3DCMP_LESSEQUAL, or D3DCMP_EQUAL. 
//If you're using a D3DFMT_D24FS8 you must use a value such as D3DCMP_GREATER, D3DCMP_GREATEREQUAL, or D3DCMP_EQUAL. 
 
//D3DFMT_D24S8 or D3DFMT_D24X8 surface, use a value such as D3DCMP_LESS, D3DCMP_LESSEQUAL, or D3DCMP_EQUAL.
//D3DFMT_D24FS8 you must use a value such as D3DCMP_GREATER, D3DCMP_GREATEREQUAL, or D3DCMP_EQUAL. 
   
//D3DFMT_D24S8 - D3DCMP_LESS, D3DCMP_LESSEQUAL, D3DCMP_EQUAL
//D3DFMT_D24FS8 - D3DCMP_GREATER, D3DCMP_GREATEREQUAL, D3DCMP_EQUAL
/*bool HiZCompatible(D3DCMPFUNC SurfHiZFunc)
{
  //D3DFMT_D24S8
  int curState = m_RP.m_CurState;
  int curStencilState = m_RP.m_CurStencilState;

}*/

bool CD3D9Renderer::FX_SetShadowMaskRT(int nGroup, int nBlurMode, CTexture *&tpSrc, SDynTexture *&pTempBlur)
{
  bool bBlur = false;
  int TempX, TempY, TempWidth, TempHeight;
  GetViewport(&TempX, &TempY, &TempWidth, &TempHeight);

  //half of current viewport
  if (CV_r_ShadowsMaskResolution == 1)
  {
    TempHeight /= 2;
  }
  else
  if (CV_r_ShadowsMaskResolution == 2)
  {
    TempWidth /= 2;
    TempHeight /= 2;
  }

  //try to reuse other RTs
	if (!CV_capture_misc_render_buffers)
	{
		if ( nGroup==0 && CTexture::s_ptexBackBuffer!=NULL && 
			CTexture::s_ptexBackBuffer->GetWidth() == TempWidth &&
			CTexture::s_ptexBackBuffer->GetHeight() == TempHeight)
		{
			CTexture::s_ptexCurrentScreenShadowMap[nGroup] =  CTexture::s_ptexBackBuffer ;
		}
		else
			if ( nGroup==0 && CTexture::s_ptexBackBufferScaled[0]!=NULL && 
				CTexture::s_ptexBackBufferScaled[0]->GetWidth() == TempWidth &&
				CTexture::s_ptexBackBufferScaled[0]->GetHeight() == TempHeight)
			{
				CTexture::s_ptexCurrentScreenShadowMap[nGroup] =  CTexture::s_ptexBackBufferScaled[0];
			}
			else
			{
				//allocate separate RT
				CTexture::s_ptexCurrentScreenShadowMap[nGroup] = CTexture::s_ptexScreenShadowMap[nGroup];
			}
	}
	else
		CTexture::s_ptexCurrentScreenShadowMap[nGroup] = CTexture::s_ptexScreenShadowMap[nGroup];


  //does not support RTs sharing
  int nShadowsMaskDownScale = (nGroup==0 && CTexture::s_ptexBackBuffer!=NULL) ? 0 : CV_r_ShadowsMaskDownScale;
  CTexture::s_ptexCurrentScreenShadowMap[nGroup]->Invalidate(TempWidth>>nShadowsMaskDownScale, TempHeight>>nShadowsMaskDownScale, eTF_Unknown);

  bBlur = true;
  if (!nBlurMode)
    bBlur = false;

  if (nBlurMode == 1 || !bBlur)
    tpSrc = CTexture::s_ptexCurrentScreenShadowMap[nGroup];
  else
  {
    pTempBlur = new SDynTexture(TempWidth, TempHeight, eTF_A8R8G8B8, eTT_2D,  FT_STATE_CLAMP, "TempShadowRT", 95);
    pTempBlur->Update(TempWidth, TempHeight);
    tpSrc = pTempBlur->m_pTexture;
  }
  FX_PushRenderTarget(0, tpSrc, &m_DepthBufferOrig);

  RT_SetViewport(0, 0, TempWidth, TempHeight);
  ColorF clClear(0,0,0,0);		// clear shadowmask black (not in shadow) so we can combine mutliple shadow masks
  int nClear = 0;

  //invalidate shadowmask for transparent pass always
  if(m_RP.m_nPassGroupID == EFSLIST_TRANSP)
  {
    SRendItem::m_ShadowsValidMask[SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][nGroup] &= ~(0xF << (nGroup*4));
  }

  //clear only once for lightgroup per frame
  //Check if Shadow Mask was cleared for this frame already
  bool bInvalidatedShadMask = ( !( SRendItem::m_ShadowsValidMask[SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][nGroup] ) && SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] == 1 /*&& m_RP.m_nPassGroupID == EFSLIST_GENERAL && CV_r_usezpass*/);

  
  if (bInvalidatedShadMask)
  {
    nClear = FRT_CLEAR_COLOR | FRT_CLEAR_STENCIL;
    EF_ClearBuffers(nClear, &clClear);
  }
  /*else
    nClear = FRT_CLEAR_STENCIL;*/

  //#ifdef _DEBUG
  /*#else
  if (!bZpass && m_RP.m_nPassGroupID != EFSLIST_TRANSP_ID)
  nClear = FRT_CLEAR_DEPTH | FRT_CLEAR_STENCIL;
  if (bBlur || (m_RP.m_TI.m_PersFlags2 & RBPF2_CLEAR_SHADOW_MASK))
  nClear |= FRT_CLEAR_COLOR;
  #endif*/

  //EF_ClearBuffers(nClear, &clClear);

  return bBlur;
}


bool CD3D9Renderer::FX_ProcessShadowsListsForLightGroup(int nGroup, int nOffsRI)
{
	uint32 i, j;

	if (CV_r_ShadowPass == 0)
		return false;

	if (m_wireframe_mode > R_SOLID_MODE || !CV_r_usezpass)
	{
		return false;
	}

	//Currently all deferred passes are disabled for other recursions by default 
	if (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]!=1)
		return false;







	PROFILE_FRAME(DrawShader_Shadows_Passes);
	PROFILE_LABEL_PUSH( "SHADOWMASK" );

  bool bDrawn = false;
  bool bSetRT = false;
	bool bDefSunLG = false;
  bool bOpaqueDrawn = false;
  int nR = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1;
  bool bBlur = false;
  int TempX, TempY, TempWidth, TempHeight;
  int nPrevGroup = m_RP.m_nCurLightGroup;
  //m_RP.m_TI.m_PersFlags2 |= RBPF2_DRAWSHADOWS;
  m_RP.m_nCurLightGroup = nGroup;

  void (*pSaveRenderFunc)();
  pSaveRenderFunc = m_RP.m_pRenderFunc;
  m_RP.m_pRenderFunc = FX_FlushShader_ShadowPass;
  FX_PreRender(0);

  int nPassGroup = m_RP.m_nPassGroupID;
  int nPassGroup2 = m_RP.m_nPassGroupDIP;
  int nAW = m_RP.m_nSortGroupID;

  static ICVar * p_e_shadows_clouds = iConsole->GetCVar("e_ShadowsClouds");

  CTexture *tpSrc = NULL;
  SDynTexture *pTempBlur = NULL;
  for (int n=0; n<SRendItem::m_nSortGroups; n++)
  {
    SRendLightGroup *pGr = &SRendItem::m_RenderLightGroups[n][nGroup];

    //Special case for transparent geometry 
    if (nPassGroup == EFSLIST_TRANSP && !pGr->m_GroupLightMask ) //check for transparent geometry pass only
      continue;

    //Test for sun light group
    uint32 nFirstLight = nGroup < 0 ? 0 : nGroup*4;
    CDLight *pGroupFirstLight = &m_RP.m_DLights[m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1][nFirstLight];

    //Special case for transparent geometry
    if (nPassGroup == EFSLIST_TRANSP)
    //if ( !( CRenderer::CV_r_ShadowPassFS && (pGr->m_GroupLightMask == 1) && (pGroupFirstLight->m_Flags & DLF_DIRECTIONAL) ) )
    {
      if (!pGr->RendItemsShadows[0].size() && !pGr->RendItemsShadows[1].size() && !pGr->RendItemsShadows[2].size() && !pGr->RendItemsShadows[3].size())
        continue;
    }

    m_RP.m_pShader = NULL;
    m_RP.m_pShaderResources = NULL;
    m_RP.m_pCurObject = m_RP.m_Objects[0];
    m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;

    m_RP.m_pPrevObject = NULL;

    if (m_LogFile)
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "\n   +++ Draw shadows for group %d\n", nGroup); 

    GetViewport(&TempX, &TempY, &TempWidth, &TempHeight);
#if !defined(XENON)
    if (!bSetRT)
    {
      #if defined(DIRECT3D9) 
        //disable shadow blur with nvidia filtered PCF for now
        if (CV_r_shadowtexformat==4 && m_FormatD24S8.IsValid())
        {
          bBlur = 0;
          FX_SetShadowMaskRT(nGroup, 0,tpSrc, pTempBlur);
        }
        else
        {
          bBlur = CV_r_shadowblur != 0;
          FX_SetShadowMaskRT(nGroup, CV_r_shadowblur,tpSrc, pTempBlur);
        }
      #else
        //disable shadow blur for dx10
        bBlur = 0;
        FX_SetShadowMaskRT(nGroup, 0,tpSrc, pTempBlur);
      #endif


      bSetRT = true;
    }




























#endif

    //FIX:: should be prepeared already
    //if (!CV_r_ShadowsForwardPass)
    //  FX_PrepareDepthMapsForLightGroup(nGroup, pGr, nOffsRI);

    m_RP.m_nCurLightGroup = nGroup;
    m_RP.m_nPassGroupID = nPassGroup;
    m_RP.m_nPassGroupDIP = EFSLIST_SHADOW_PASS;
    m_RP.m_pRenderFunc = FX_FlushShader_ShadowPass;


    if ( nPassGroup != EFSLIST_TRANSP )
    {
       //viewport for current render target
       int vpX, vpY, vpWidth, vpHeight;
       GetViewport( &vpX, &vpY, &vpWidth, &vpHeight );
       assert( vpX == 0 && vpY == 0 && vpWidth > 0 && vpHeight > 0 );
       bDrawn = FX_DeferredShadows( nGroup, pGr, vpWidth, vpHeight );
       //additional flag for sharing SRendItem::m_ShadowsValidMask for projector validation
       bOpaqueDrawn = bDrawn;
    }
    else
    { //draw all others shadows
      CShader *pShader, *pCurShader;
      SRenderShaderResources *pRes;
      CRenderObject *pObject, *pCurObject;
      int nTech;

			for (i=/*bDefSunLG?1:*/0; i<4; i++) //draw shadows for transparent for sun only
      {
        //note: moved to the actual draw in FX_DrawShadowPasses
        //shadow mask is valid
        //SRendItem::m_ShadowsValidMask[SRendItem::m_RecurseLevel][nGroup] |= (1 << i);

        if (!(pGr->m_GroupLightMask & (1<<(i+nGroup*4))))
          continue;
        //int nR = SRendItem::m_RecurseLevel;
        CDLight* pLight = &m_RP.m_DLights[m_RP.m_nProcessThreadID][nR][nGroup*4+i];

        //skip all except sun for now
        if ( !(pLight ->m_Flags & DLF_DIRECTIONAL || pLight->m_Flags & DLF_PROJECT) )
          continue;

        m_RP.m_nCurLightChan = i;
        uint32 oldVal = ~0;
        pCurObject = NULL;
        pCurShader = NULL;
        bool bIgnore = false;
        bool bChanged;

        for (j=0; (int)j<pGr->RendItemsShadows[i].size(); j++)
        {
          int nRI = pGr->RendItemsShadows[i][j];
          nRI = (nRI & 0xffffff) + nOffsRI;
          SRendItem *ri = &SRendItem::RendItems(m_RP.m_nProcessThreadID,nAW,m_RP.m_nPassGroupID)[nRI];
          CRendElementBase *pRE = ri->Item;
          if (oldVal != ri->SortVal)
          {
            SRendItem::mfGet(ri->SortVal, nTech, pShader, pRes);
            bChanged = true;
          }
          else
            bChanged = false;
          pObject = ri->pObj;
          oldVal = ri->SortVal;
          if (pObject != pCurObject)
          {
            if (!bChanged)
            {
              if (FX_TryToMerge(pObject, pCurObject, pRE))
              {
                m_RP.m_RIs.AddElem(ri);
                continue;
              }
            }
            if (pCurShader)
            {
              m_RP.m_pRenderFunc();
              pCurShader = NULL;
              bChanged = true;
            }
            if (!FX_ObjectChange(pShader, pRes, pObject, pRE))
            {
              bIgnore = true;
              continue;
            }
            bIgnore = false;
            pCurObject = pObject;
          }

          if (bChanged)
          {
            if (pCurShader)
              m_RP.m_pRenderFunc();
            FX_Start(pShader, nTech, pRes, pRE);
            pCurShader = pShader;
          }

          {
            //PROFILE_FRAME_TOTAL(Mesh_REPrepare);
            pRE->mfPrepare();
          }
          if (!m_RP.m_RIs.Num())
            m_RP.m_RIs.AddElem(ri);
        }
        if (pCurShader)
          m_RP.m_pRenderFunc();

        bDrawn = true;
      }
    }
  }
  //m_RP.m_TI.m_PersFlags2 &= ~RBPF2_DRAWSHADOWS;
  m_RP.m_nPassGroupDIP = nPassGroup2;
  FX_PostRender();

  CTexture *pTexMask = CTexture::s_ptexCurrentScreenShadowMap[nGroup];

  //these passes are drawn for SUN lightgroup only
  if ((nPassGroup == EFSLIST_GENERAL) && (nGroup==0) && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE) && SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] == 1)
  {
    //Draw sprites shadows if shadows there is no deferred shadow pass for this lightgroup and 
    // there are sprites to draw
    if (bDrawn && !bDefSunLG )
    {
      //DrawSpritesShadows(bBlur, bSetRT, tpSrc);
    }
  }
  else
  if (!bSetRT && nPassGroup==EFSLIST_TRANSP && nGroup==0 && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE) && SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] == 1 && p_e_shadows_clouds->GetIVal())
  {
    ColorF cBlack(0,0,0);
    if (CTexture::IsTextureExist(pTexMask))
      pTexMask->Fill(cBlack);
  }

  //restore target























  if (bSetRT)
  {
    FX_PopRenderTarget(0);
  }


  // Shadow Blur
  if (bSetRT && bDrawn)
  {
    if (bBlur)
    {
      if (CV_r_shadowblur == 1)
      {
        int nSizeX = tpSrc->GetWidth();
        int nSizeY = tpSrc->GetHeight();
        if (!pTempBlur)
          pTempBlur = new SDynTexture(nSizeX, nSizeY, eTF_A8R8G8B8, eTT_2D,  FT_STATE_CLAMP, "TempShadowRT", 95);
        FX_ShadowBlur(CV_r_shadowbluriness, pTempBlur, pTexMask);
      }
      else
      {
        assert(pTempBlur);
        FX_ShadowBlur(CV_r_shadowbluriness, pTempBlur, pTexMask);
      }    
		}
  }

  //restore viewport
  if(bSetRT)
  {
    RT_SetViewport(TempX, TempY, TempWidth, TempHeight);
    if (m_LogFile)
      Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "\n   +++ End shadow maps for group %d\n", nGroup); 
  }

  //projectors 
  if (CV_r_ShadowsDeferredMode>2)
  {
    for (int n=0; n<SRendItem::m_nSortGroups; n++)
    {
      SRendLightGroup *pGr = &SRendItem::m_RenderLightGroups[n][nGroup];

      //Special case for transparent geometry 
      if (!pGr->m_GroupLightMask)
        continue;

      FX_DeferredProjLights(nGroup, !bOpaqueDrawn);
    }
  }


  SAFE_DELETE(pTempBlur);

  m_RP.m_nNumRendPasses = 0;
  m_RP.m_nCurLightGroup = nPrevGroup;
  m_RP.m_pRenderFunc = pSaveRenderFunc;

	PROFILE_LABEL_POP( "SHADOWMASK" );

  return true;
}
