#include "StdAfx.h"
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "D3DPostProcess.h"

#pragma warning(disable: 4244)

using namespace NPostEffects;

//#pragma optimize("", off)

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

float Saturate(float x)
{
	// Fran: do we have a saturate for floats?
	if (x < 0.0f) return 0.0f;
	if (x > 1.0f) return 1.0f;
	return x;
}

Vec4 SaturateXYZ(Vec4 v)
{
	v.x = Saturate(v.x);
	v.y = Saturate(v.y);
	v.z = Saturate(v.z);
	return v;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CD3D9Renderer::FX_DeferredCaustics( ) 
{ 
	if( !CRenderer::CV_r_watercaustics || !CRenderer::CV_r_watercausticsdeferred || !CRenderer::CV_r_deferredshading || !CTexture::s_ptexBackBuffer || !CTexture::s_ptexSceneTarget )
		return false;

	S3DEngineCommon::SOceanInfo &OceanInfo= gRenDev->m_p3DEngineCommon.m_OceanInfo;
	bool bOceanVolumeVisible = (OceanInfo.m_nOceanRenderFlags & OCR_OCEANVOLUME_VISIBLE) != 0;
	if( !bOceanVolumeVisible )
		return false;

	uint64 nFlagsShaderRTSave = gcpRendD3D->m_RP.m_FlagsShader_RT;
	
	if (m_LogFile)
		Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ Deferred caustics pass begin +++ \n");

	PROFILE_FRAME(DrawShader_DeferredCausticsPass);

	// Stencil pre-pass
	float fWatLevel = OceanInfo.m_fWaterLevel;
	Vec4 pCausticsParams = OceanInfo.m_vCausticsParams;
	float fDist = pCausticsParams.y;

	if(  CRenderer::CV_r_watercausticsdeferred == 2 ) 
	{
		// stencil pre-pass
		CShader *pSH( CShaderMan::m_ShaderShadowMaskGen );
		//EF_ClearBuffers(FRT_CLEAR_STENCIL|FRT_CLEAR_IMMEDIATE, NULL, 1); 

		// make box for stencil passes
		t_arrDeferredMeshIndBuff arrDeferredInds;
		t_arrDeferredMeshVertBuff arrDeferredVerts;
		CreateDeferredUnitBox(arrDeferredInds, arrDeferredVerts);

		Vec3 vCamPos = gRenDev->GetRCamera().Orig;
		float fWaterPlaneSize = gRenDev->GetCamera().GetFarPlane();

		m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Push();
		Matrix34 mLocal;
		mLocal.SetIdentity();

		// todo: adjust box size based on attenuation range

		mLocal.SetScale(Vec3(fDist*2, fDist*2, fWatLevel + 3.0f));//,boxOcean.max);
		mLocal.SetTranslation( Vec3(vCamPos.x-fDist, vCamPos.y-fDist, -2) );
		Matrix44 mLocalTransposed = GetTransposed44(Matrix44(mLocal));
		m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->MultMatrixLocal(&mLocalTransposed);

		uint32 nPasses = 0;         
		static CCryName TechName0 = "DeferredShadowPass";
		pSH->FXSetTechnique(TechName0);
		pSH->FXBegin( &nPasses, FEF_DONTSETSTATES );
		pSH->FXBeginPass( 2 );

		int nVertOffs, nIndOffs;

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

		if (!FAILED(FX_SetVertexDeclaration( 0, eVF_P3F_C4B_T2F )))
			FX_StencilCullPass(-1, nVertOffs, arrDeferredVerts.size(), nIndOffs, arrDeferredInds.size());

		pSH->FXEndPass();
		pSH->FXEnd();

		m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Pop();

		FX_StencilRefresh(STENC_FUNC(FSS_STENCFUNC_EQUAL), m_nStencilMaskRef, 0xFFFFFFFF);
		FX_StencilTestCurRef(true);

		//EF_SetStencilState(
		//									 STENC_FUNC(FSS_STENCFUNC_EQUAL) |
		//									 STENCOP_FAIL(FSS_STENCOP_KEEP) |
		//									 STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
		//									 STENCOP_PASS(FSS_STENCOP_KEEP),
		//									 m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFFFFFF);

		//m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |=RBPF2_LIGHTSTENCILCULL;
	}

	// Deferred caustic pass

	gcpRendD3D->EF_Scissor(false, 0, 0, 0, 0);
	//gcpRendD3D->Set2DMode(true, 1, 1);   







	CShader *pShader = CShaderMan::m_ShaderDeferredCaustics;
	gcpRendD3D->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]|g_HWSR_MaskBit[HWSR_SAMPLE3];

	static CCryName pTechName = "General";
	SD3DPostEffectsUtils::ShBeginPass(pShader, pTechName, FEF_DONTSETSTATES);

  int32 nRState = GS_NODEPTHTEST |(( CRenderer::CV_r_watercausticsdeferred == 2)?GS_STENCIL:0)|(GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA);





	gcpRendD3D->EF_SetState(nRState);      

	SD3DPostEffectsUtils::DrawScreenQuadWPOS(0, 0, CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight(),
																					 0.0f, 
																					 CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight());

	SD3DPostEffectsUtils::ShEndPass();


	//gcpRendD3D->Set2DMode(false, 1, 1);   

	if(  CRenderer::CV_r_watercausticsdeferred == 2 ) 
		FX_StencilTestCurRef(false);

	if (m_LogFile)
		Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ Deferred caustics pass end +++ \n");

	gcpRendD3D->m_RP.m_FlagsShader_RT = nFlagsShaderRTSave;
	//m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 = nPersFlags2Save;

	FX_ResetPipe();
	return true;
}

bool CD3D9Renderer::FX_DeferredRainLayer()
{
  if( CRenderer::CV_r_rain <2 || !CRenderer::CV_r_deferredshading || !CRenderer::CV_r_PostProcess || !CTexture::s_ptexBackBuffer || !CTexture::s_ptexSceneTarget )
    return false;

	static bool disable = false;
	if (disable)
		return true;

	PROFILE_LABEL_PUSH("DEFERRED_RAIN");

  // Big workaround - todo: make specific Rain Volume entity
  TArray<CDLight> &pLights = PostEffectMgr().GetLights();
  bool bRainVolumes = false;
  for(uint32 n = 0; n < pLights.Num(); ++n)
  {
    if( pLights[n].m_nPostEffect == 2)
      bRainVolumes = true;
  }

  CEffectParam *pParam = PostEffectMgr().GetByName("SceneRain_Amount"); 
  pParam->SetParam(0);

  if( !bRainVolumes ) 
    return false;

  FX_ScreenStretchRect(CTexture::s_ptexSceneTarget);

  CTexture *pTexScreenSpaceRefl = (!CRenderer::CV_r_HDRRendering)? CTexture::s_ptexBackBufferScaled[1] : CTexture::s_ptexHDRTargetScaled[0];
	pTexScreenSpaceRefl->SetRenderTargetTile(1);

	PROFILE_LABEL_PUSH("VERTICAL_BLUR");
  // Blur screen vertically for faking screen space reflections
  {
    SDynTexture *tpBlurTemp = new SDynTexture( pTexScreenSpaceRefl->GetWidth(),  pTexScreenSpaceRefl->GetHeight(),  pTexScreenSpaceRefl->GetDstFormat(), eTT_2D,  FT_STATE_CLAMP, "TempSSReflRT");
		
    tpBlurTemp->Update(  pTexScreenSpaceRefl->GetWidth(), pTexScreenSpaceRefl->GetHeight() );

    if( !tpBlurTemp->m_pTexture )
    {
      SAFE_DELETE(tpBlurTemp);
      return false;
    }

		tpBlurTemp->m_pTexture->SetRenderTargetTile(1);

    gcpRendD3D->Set2DMode(true, 1, 1);     

		GetUtils().StretchRect(CTexture::s_ptexSceneTarget, pTexScreenSpaceRefl);

    uint64 nFlagsShaderRT = gcpRendD3D->m_RP.m_FlagsShader_RT;
    gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

    float fScale = -32.0f;

    // Get current viewport
    int iTempX, iTempY, iWidth, iHeight;
    gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
    gcpRendD3D->RT_SetViewport(0, 0,  pTexScreenSpaceRefl->GetWidth(), pTexScreenSpaceRefl->GetHeight());        

    Vec4 vWhite( 1.0f, 1.0f, 1.0f, 1.0f );

		PROFILE_LABEL_PUSH("ANISOTROPIC_VERTICAL");

    CCryName pTechName("AnisotropicVertical");
    CShader *m_pCurrShader = CShaderMan::m_shPostEffects;

    uint32 nPasses;

    // setup texture offsets, for texture sampling
    float s1 = 1.0f/(float) pTexScreenSpaceRefl->GetWidth();     
    float t1 = 1.0f/(float) pTexScreenSpaceRefl->GetHeight();    

    Vec4 pWeightsPS;
    pWeightsPS.x = (1.0f / 8.0f)* t1;
    pWeightsPS.y = 0.5f * t1;
    pWeightsPS.z = 0.75f * t1;
    pWeightsPS.w = 1.0f * t1;
    
    pWeightsPS *= fScale;

    STexState sTexState = STexState(FILTER_LINEAR, true);
    sTexState.m_nAddressU = TADDR_BORDER;
    sTexState.m_nAddressV = TADDR_BORDER;
    sTexState.SetBorderColor(0);

    CCryName pParam0Name("blurParams0");

    m_pCurrShader->FXSetTechnique(pTechName);
    m_pCurrShader->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    m_pCurrShader->FXBeginPass(0);

    gRenDev->EF_SetState(GS_NODEPTHTEST);   

    CShaderMan::m_shPostEffects->FXSetVSFloat(pParam0Name, &pWeightsPS, 1);  
    gcpRendD3D->FX_PushRenderTarget(0, tpBlurTemp->m_pTexture, &gcpRendD3D->m_DepthBufferOrig);
    gcpRendD3D->RT_SetViewport(0, 0, pTexScreenSpaceRefl->GetWidth(), pTexScreenSpaceRefl->GetHeight());        

    pTexScreenSpaceRefl->Apply(0, CTexture::GetTexState(sTexState)); 
    GetUtils().DrawFullScreenQuad(pTexScreenSpaceRefl->GetWidth(), pTexScreenSpaceRefl->GetHeight());

    gcpRendD3D->FX_PopRenderTarget(0);
    m_pCurrShader->FXEndPass();
    m_pCurrShader->FXEnd(); 

    pWeightsPS*=0.5f;

    m_pCurrShader->FXSetTechnique(pTechName);
    m_pCurrShader->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    m_pCurrShader->FXBeginPass(0);

    gcpRendD3D->FX_PushRenderTarget(0, pTexScreenSpaceRefl, &gcpRendD3D->m_DepthBufferOrig);
    gcpRendD3D->RT_SetViewport(0, 0, pTexScreenSpaceRefl->GetWidth(), pTexScreenSpaceRefl->GetHeight());         

    CShaderMan::m_shPostEffects->FXSetVSFloat(pParam0Name, &pWeightsPS, 1);  
    tpBlurTemp->m_pTexture->Apply(0, CTexture::GetTexState(sTexState)); 

    GetUtils().DrawFullScreenQuad( pTexScreenSpaceRefl->GetWidth(), pTexScreenSpaceRefl->GetHeight());      

    gcpRendD3D->FX_PopRenderTarget(0);

    m_pCurrShader->FXEndPass();
    m_pCurrShader->FXEnd(); 

		PROFILE_LABEL_POP("BLUR");

    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

    SAFE_DELETE(tpBlurTemp);

    m_RP.m_FlagsShader_RT = nFlagsShaderRT;

    gcpRendD3D->Set2DMode(false, 1, 1);     
  }

	PROFILE_LABEL_POP("VERTICAL_BLUR");
  
  CRenderCamera *pCam = &gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_rcam;  
  Vec3 pCamPos = pCam->Orig;

  uint64 nFlagsShaderRTSave = gcpRendD3D->m_RP.m_FlagsShader_RT;
  if (m_LogFile)
    Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ Deferred rain pass begin +++ \n");

  // Used for water puddle texture translation animation
  CCryName paramPuddleName("g_RainPuddleParams");
  float fTime = m_RP.m_TI[m_RP.m_nProcessThreadID].m_RealTime * 0.333f;
  Vec4 pPuddleParams = Vec4(sinf(fTime), cosf(fTime), sinf(fTime * 2.0f), cosf(fTime * 2.0f));

	PROFILE_LABEL_PUSH("COMPOSITE_PASS");

  for(uint32 n = 0; n < pLights.Num(); ++n)
  {
    CDLight *pDL = &pLights[n];
    if( pDL->m_nPostEffect == 2)
    {
      // Store light properties (color/radius, position relative to camera, rect, zbounds)
      Vec4 pLightDiffuse = Vec4(pDL->m_Color.r, pDL->m_Color.g, pDL->m_Color.b, pDL->m_SpecMult);  
      float fInvRadius = 1.0f / pDL->m_fRadius; 
      Vec4 pLightPosCS = Vec4(pDL->m_Origin - pCamPos, fInvRadius) ;  
      Vec4 pDepthBounds = CDeferredShading::Instance().GetLightDepthBounds( (const CDLight*)pDL );

      CShader *pShader = CShaderMan::m_ShaderDeferredRain;
      gcpRendD3D->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]|g_HWSR_MaskBit[HWSR_SAMPLE3];
			if( IsHDRModeEnabled() )
				gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_HDR_MODE];
			
			static CCryName pTechName = "General";
      SD3DPostEffectsUtils::ShBeginPass(pShader, pTechName, FEF_DONTSETSTATES);
      gcpRendD3D->EF_SetState(GS_NODEPTHTEST);      

      static CCryName paramName("g_RainVolumeParams");
      pShader->FXSetPSFloat(paramName, &pLightPosCS, 1);       

      static CCryName param1Name("g_RainVolumeMultipliers"); 
			Vec4 rainVolumeMultipliers;
			rainVolumeMultipliers.x = 0.8f * Saturate(pLightDiffuse.x);
			rainVolumeMultipliers.y = 0.8f * Saturate(pLightDiffuse.y);
			rainVolumeMultipliers.z = 0.8f * Saturate(pLightDiffuse.z);
			rainVolumeMultipliers.w = pLightDiffuse.w;
			
      pShader->FXSetPSFloat(param1Name, &pLightDiffuse, 1);
      
      static CCryName param2Name("g_WorldViewPos");
      Vec4 pCamPosParam = Vec4( pCamPos, 1.0f) ;  
      pShader->FXSetPSFloat(param2Name, &pCamPosParam, 1);       

      pShader->FXSetPSFloat(paramPuddleName, &pPuddleParams, 1);        

      gcpRendD3D->FX_Commit();

      gcpRendD3D->EF_Scissor(true, pDL->m_sX , pDL->m_sY, pDL->m_sWidth, pDL->m_sHeight);
      if( gcpRendD3D->m_bDeviceSupports_NVDBT )
        gcpRendD3D->SetDepthBoundTest( pDepthBounds.x, pDepthBounds.z, true);

			gcpRendD3D->EF_SetState(GS_NODEPTHTEST|GS_BLSRC_SRCALPHA|GS_BLDST_ONE);
//			gcpRendD3D->EF_SetState(GS_NODEPTHTEST|GS_BLSRC_ONE|GS_BLDST_ONE);

      SD3DPostEffectsUtils::DrawScreenQuadWPOS(0, 0, CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight(),
        0.0f, 
        CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight());

      SD3DPostEffectsUtils::ShEndPass();

      // Enable rain postprocessing - todo: should check which emitter is nearest and use that one
      if( CRenderer::CV_r_rain == 3)
      {
        float fDistToCenter = pLightPosCS.GetLength();
        Vec3 vL = Vec3(pLightPosCS.x, pLightPosCS.y, pLightPosCS.z) * pLightPosCS.w;
        float fAtten = 1.0f - vL.dot(vL);
        if( fAtten > 0.01f )
          pParam->SetParam(min(1.0f, 10000.0f * fAtten) );   
      }
    }
  }

	PROFILE_LABEL_POP("COMPOSITE_PASS");

  gcpRendD3D->EF_Scissor(false, 0,0,0,0);
  gcpRendD3D->SetDepthBoundTest(0.0f, 1.0f, false);

  gcpRendD3D->m_RP.m_FlagsShader_RT = nFlagsShaderRTSave;

  FX_ResetPipe();

//  CEffectParam *pParam = PostEffectMgr().GetByName("SceneRain_Amount"); 
  //pParam->SetParam(1.0f);   

  m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_OCEANPARTICLES;

  if (m_LogFile)
    Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ Deferred rain pass end +++ \n");

	PROFILE_LABEL_POP("DEFERRED_RAIN");

  return true;

}
