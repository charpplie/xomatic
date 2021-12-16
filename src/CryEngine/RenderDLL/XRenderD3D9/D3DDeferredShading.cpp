
#include "StdAfx.h"
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "D3DPostProcess.h"
#include "D3DIrradianceVolume.h"

#pragma warning(disable: 4244)

using namespace NPostEffects;

CDeferredShading CDeferredShading::m_pInstance;

#define RT_LIGHTSMASK g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]|g_HWSR_MaskBit[HWSR_SAMPLE3]|g_HWSR_MaskBit[HWSR_LIGHT_TEX_PROJ]|g_HWSR_MaskBit[HWSR_CUBEMAP0]
#define RT_DEBUGMASK g_HWSR_MaskBit[HWSR_DEBUG0]|g_HWSR_MaskBit[HWSR_DEBUG1]|g_HWSR_MaskBit[HWSR_DEBUG2]|g_HWSR_MaskBit[HWSR_DEBUG3]
#define RT_SPOTLIGHT g_HWSR_MaskBit[HWSR_SAMPLE0]
#define RT_GLOBAL_CUBEMAP g_HWSR_MaskBit[HWSR_SAMPLE0]
#define RT_SPECULAR_CUBEMAP g_HWSR_MaskBit[HWSR_SAMPLE1]
#define RT_NEGATIVE_LIGHT g_HWSR_MaskBit[HWSR_SAMPLE2]
#define RT_CLIPEDSPOTLIGHT g_HWSR_MaskBit[HWSR_SAMPLE1]
#define RT_HEIGHTBASEDAMBIENT g_HWSR_MaskBit[HWSR_SAMPLE4]
#define RT_NO_SPECULAR g_HWSR_MaskBit[HWSR_SAMPLE3]
#define RT_SUNLIGHT g_HWSR_MaskBit[HWSR_SAMPLE2]
#define RT_AMBIENT_INDEXED g_HWSR_MaskBit[HWSR_SAMPLE3]
#define RT_OVERDRAW_DEBUG g_HWSR_MaskBit[HWSR_DEBUG0]

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CDeferredShading::AddLight( const CDLight &pDL, float fMult )
{
	const uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;
	const int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID] - 1;

	eDeferredLightType LightType = eDLT_DeferredLight;
	if(pDL.m_Flags & DLF_NEGATIVE)
	{
		LightType = eDLT_DeferredNegative;
	}
	else if(pDL.m_Flags & DLF_DEFERRED_CUBEMAPS)
	{
		LightType = eDLT_DeferredCubemap;
		if(pDL.m_Flags & DLF_AMBIENT_LIGHT)
			LightType = eDLT_DeferredAmbientCubemap;
	}
	else if(pDL.m_Flags & DLF_AMBIENT_LIGHT)
			LightType = eDLT_DeferredAmbientLight;

  uint32 nId = m_pLights[LightType][nThreadID][nRecurseLevel].size() - 1 ;
  TArray<CDLight> & rArray = m_pLights[LightType][nThreadID][nRecurseLevel];
  rArray.AddElem(pDL);
  rArray[rArray.Num()-1].m_Color *= fMult;
  rArray[rArray.Num()-1].m_SpecMult *= fMult;

	gcpRendD3D->EF_CheckLightMaterial( const_cast<CDLight*>(&pDL) );

	return nId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

TArray<CDLight>& CDeferredShading::GetLights(int nThreadID, int nCurRecLevel, const eDeferredLightType eType/* = eDLT_DeferredLight*/)
{
  return m_pLights[eType][nThreadID][nCurRecLevel];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CDeferredShading::GetLightsNum(const eDeferredLightType eType)
{
  uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;
  int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID] - 1;
  return m_pLights[eType][nThreadID][nRecurseLevel].size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::ResetLights( )
{
	uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;
	int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID] - 1;

	for(uint32 iLightType = 0;iLightType<eDLT_NumLightTypes;++iLightType)
  m_pLights[iLightType][nThreadID][nRecurseLevel].SetUse(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CDeferredShading::AddVisArea( const IVisArea *pVisArea )
{
  uint32 nThreadID = gRenDev->m_RP.m_nFillThreadID;
  // Note: vis area code is processed before EF_StartEf() in 3DEngine side - so recurse level still at 0 at beginning
  int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID] /*- 1*/;

	VisAreaIDMapItor it = m_pVisAreas[nThreadID][nRecurseLevel].find( pVisArea );
  if( it != m_pVisAreas[nThreadID][nRecurseLevel].end() )
    return it->second;

  m_nVisAreasCount[nThreadID][nRecurseLevel]++;
  m_pVisAreas[nThreadID][nRecurseLevel].insert( VisAreaIDMapItor::value_type( pVisArea, m_nVisAreasCount[nThreadID][nRecurseLevel] ) );

  return m_nVisAreasCount[nThreadID][nRecurseLevel];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CDeferredShading::GetVisAreaID( uint32 nThreadID, const IVisArea *pVisArea ) 
{
  int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID] - 1;

  VisAreaIDMapItor it = m_pVisAreas[nThreadID][nRecurseLevel].find( pVisArea );
  if( it != m_pVisAreas[nThreadID][nRecurseLevel].end() )
    return it->second;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::ResetVisAreas()
{
  uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;
	// Note: vis area code is processed before EF_StartEf() in 3DEngine side - so recurse level still at 0 at beginning
  int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID] /*- 1*/;

  m_pVisAreas[nThreadID][nRecurseLevel].clear();
  m_nVisAreasCount[nThreadID][nRecurseLevel] = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::SpecularAccEnableMRT( bool bEnable )
{
#if !defined(XENON_FORCE_720P)

	assert( m_pSpecularAccRT );

  CD3D9Renderer *const __restrict rd = gcpRendD3D;
  if( bEnable )
    rd->FX_PushRenderTarget(1, m_pSpecularAccRT, NULL, 0, -1, false, 1);
  else
	{
		m_pSpecularAccRT->SetResolved( true );
    rd->FX_PopRenderTarget(1);
	}

#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::SetupPasses()
{
  CreateDeferredMaps();

  gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(RT_LIGHTSMASK | RT_DEBUGMASK);

  m_pDiffuseAccRT = CTexture::s_ptexCurrentSceneDiffuseAccMap;
	m_pSpecularAccRT = CTexture::s_ptexSceneSpecularAccMap;
  m_pNormalsRT = CTexture::s_ptexSceneNormalsMap;
  m_pDepthRT = CTexture::s_ptexZTarget;
	m_pSceneTexturesRT = CTexture::s_ptexSceneTexturesMap;

  CRenderCamera *pCam = &gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_rcam;  
  m_pCamFront = pCam->Z;
  m_pCamFront.Normalize();
  m_pCamPos = pCam->Orig;

  m_fCamFar = pCam->Far;
  m_fCamNear = pCam->Near;

  m_fRatioWidth = (float)m_pDiffuseAccRT->GetWidth() / (float)CTexture::s_ptexSceneTarget->GetWidth();
  m_fRatioHeight = (float)m_pDiffuseAccRT->GetHeight() / (float)CTexture::s_ptexSceneTarget->GetHeight();

  m_pView = gcpRendD3D->m_CameraMatrix;
  m_pView.Transpose();

  m_pViewProj = gcpRendD3D->m_CameraProjMatrix;
  m_pViewProj.Transpose();

  m_pViewProjI = gcpRendD3D->m_CameraProjMatrix;
  m_pViewProjI.Invert();  

  m_pShader = CShaderMan::m_shDeferredShading;

  gcpRendD3D->SetCullMode(R_CULL_BACK);

  if( CRenderer::CV_r_deferredshadingdepthboundstest <= 1 )
    m_nRenderState |= GS_NODEPTHTEST;
  else 
    m_nRenderState &= ~GS_NODEPTHTEST;

  if( CRenderer::CV_r_deferredshadingdebug == 3)
  {
    m_pDiffuseAccRT = m_pDebugRT;
    gcpRendD3D->m_RP.m_FlagsShader_RT |= RT_OVERDRAW_DEBUG;
  }

  SetupGlobalConsts();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeferredShading::SetupGlobalConsts()
{
  CD3D9Renderer *const __restrict rd = gcpRendD3D;

  //set world basis
  float maskRTWidth = float(m_pDiffuseAccRT->GetWidth());
  float maskRTHeight = float(m_pDiffuseAccRT->GetHeight());
  Vec4r vWBasisX, vWBasisY, vWBasisZ, vCamPos;
  bool bVPosSM30 = true; //(GetFeatures() & (RFT_HW_PS30|RFT_HW_PS40))!=0;
  Vec4 vParamValue, vMag;
  CShadowUtils::ProjectScreenToWorldExpansionBasis(rd->m_IdentityMatrix , rd->GetCamera(), maskRTWidth, maskRTHeight, vWBasisX, vWBasisY, vWBasisZ, vCamPos, bVPosSM30, NULL);

  //normalize
  /*
  vMag.x = vWBasisX.GetLength();
  vMag.y = vWBasisY.GetLength();
  vMag.z = vWBasisZ.GetLength();
  vMag.w = 1.0f;
  vWBasisX /= vWBasisX.GetLength();
  vWBasisY /= vWBasisY.GetLength();
  vWBasisZ /= vWBasisZ.GetLength();
  */

  vWorldBasisX = vWBasisX;
  vWorldBasisY = vWBasisY;
  vWorldBasisZ = vWBasisZ;

}
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeferredShading::DrawLightVolume(const CDLight *pDL)
{

  CD3D9Renderer *const __restrict rd = gcpRendD3D;

  float maskRTWidth = float(m_pDiffuseAccRT->GetWidth());
  float maskRTHeight = float(m_pDiffuseAccRT->GetHeight());

#if defined(DIRECT3D10) && !defined(PS3)
  Vec4 vScreenScale(1.0f/maskRTWidth, 1.0f/maskRTHeight,
    0.0f, 0.0f);
#else
  Vec4 vScreenScale(1.0f/maskRTWidth, 1.0f/maskRTHeight,
    0.5f/maskRTWidth, 0.5f/maskRTHeight);
#endif

  {
    static CCryName paramName("g_ScreenScale");
    m_pShader->FXSetPSFloat(paramName, &vScreenScale, 1);
  }

  {
    static CCryName paramName("vWBasisX");
    m_pShader->FXSetPSFloat(paramName, &vWorldBasisX, 1);
  }

  {
    static CCryName paramName("vWBasisY");
    m_pShader->FXSetPSFloat(paramName, &vWorldBasisY, 1);
  }

  {
    static CCryName paramName("vWBasisZ");
    m_pShader->FXSetPSFloat(paramName, &vWorldBasisZ, 1);
  } 

  /*{
  static CCryName paramName("vBasisMagnitudes");
  vParamValue = vMag;
  m_pShader->FXSetPSFloat(paramName, &vParamValue, 1);
  }*/

  /*
  {
  static CCryName paramName("vCamWorldPos");
  //vParamValue = vCamPos;
  vParamValue.x = vCamPos.x;
  vParamValue.y = vCamPos.y;
  vParamValue.z = vCamPos.z;
  vParamValue.w = vCamPos.w;
  m_pShader->FXSetPSFloat(paramName, &vParamValue, 1);
  }
  */

  //////////////// light sphere processing /////////////////////////////////
  float fExpensionRadius = pDL->m_fRadius*1.08f;
  Vec3 vScale(fExpensionRadius, fExpensionRadius, fExpensionRadius);
  Matrix34 mLocal;
  mLocal.SetIdentity();
  mLocal.SetScale(vScale, pDL->m_Origin);
  Matrix44 mLocalTransposed = GetTransposed44(Matrix44(mLocal));

  Matrix44 mViewProjMatrix =  mLocalTransposed * rd->m_ViewMatrix  ;
  mViewProjMatrix.Multiply(mViewProjMatrix, rd->m_ProjMatrix);
  //mViewProjMatrix.Multiply(Matrix44(mLocalTransposed), m_pViewProj);

  {
    static CCryName paramName("g_mViewProj");
    m_pShader->FXSetVSFloat(paramName, (Vec4*) mViewProjMatrix.GetData(), 4);       
  }

  rd->FX_SetVStream( 0, rd->m_pUnitSphereVB, 0, sizeof( SVF_P3F_C4B_T2F ) );
  rd->FX_SetIStream(rd->m_pUnitSphereIB);

  rd->D3DSetCull(eCULL_Back);
  if (!FAILED(rd->FX_SetVertexDeclaration( 0, eVF_P3F_C4B_T2F )))
  {
    rd->FX_Commit(false);
#if defined (DIRECT3D9) || defined(OPENGL)
    rd->m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, rd->m_UnitSphereVBSize, 0, rd->m_UnitSphereIBSize/3);
#elif defined (DIRECT3D10)
    rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    rd->m_pd3dDeviceContext->DrawIndexed(rd->m_UnitSphereIBSize, 0, 0);
#endif
    rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += rd->m_UnitSphereIBSize/3;
    rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;
  }

}


//////////////////////////////////////////////////////////////////////////
void CDeferredShading::DeferredDecalPass( SDeferrredDecal& rDecal)
{
  PROFILE_FRAME(CDeferredShading_LightPass);
  PROFILE_SHADER_START;

  gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(RT_LIGHTSMASK);

  CD3D9Renderer *const __restrict rd = gcpRendD3D;

  bool bProj2D = true;
  bool bStencilMask = false;//(CRenderer::CV_r_deferredshadingstencilprepass && bProj2D );

  bool bUseLightVolumes = false;
  /*if( CRenderer::CV_r_deferredshadingLightVolumes)
  {
    float fSqLightRadius = pDL->m_fRadius * 1.22f;	// the light mesh tessellation and near clipping plane require some bias when testing if inside sphere
    fSqLightRadius *= fSqLightRadius;
    if (fSqLightRadius < pDL->m_Origin.GetSquaredDistance(m_pCamPos))
      bUseLightVolumes = true;
    else
      bStencilMask = true;
  } */

  //reset stencil mask
  /*if (pDL->m_Flags & DLF_CASTSHADOW_MAPS)
  {
    bStencilMask = false;
  } */

  //bool bSunLight = (pDL->m_Flags & DLF_SUN)!=0;
  //float fInvRadius = (!bSunLight)? (pDL->m_fRadius <= 0) ? 1.0f : 1.0f / pDL->m_fRadius : 0.0f;
  //Vec4 pLightPosCS = (!bSunLight)? Vec4(pDL->m_Origin - m_pCamPos, fInvRadius) : Vec4((pDL->m_Origin - m_pCamPos).normalized(), 0.0f);  
  //Vec4 pLightRect = (!bSunLight)? Vec4(pDL->m_sX * m_fRatioWidth, pDL->m_sY * m_fRatioHeight, pDL->m_sWidth * m_fRatioWidth, pDL->m_sHeight * m_fRatioHeight): Vec4(0.0f, 0.0f, 0.0f, 0.0f);        
  //Vec4 pDepthBounds = (!bSunLight)? GetLightDepthBounds( pDL ) : Vec4(0.0f, 0.0f, 0.0f, 0.0f);

  //if( CRenderer::CV_r_deferredshadinglightlodratio && !bSunLight)
  //{
  //  //const float fMinDistThreshold = 64.0f;
  //  //pLightDiffuse.w *= 1.0f - min(1.0f, pDepthBounds.y / fMinDistThreshold);

  //  float fLightArea = pLightRect.z * pLightRect.w;
  //  float fScreenArea = (float) CTexture::s_ptexSceneTarget->GetWidth() * CTexture::s_ptexSceneTarget->GetHeight();
  //  float fLightRatio = fLightArea / fScreenArea;    
  //  const float fMinScreenAreaRatioThreshold = 0.01f ;  // 1% of screen by default
  //  //pLightDiffuse.w *= ( clamp_tpl<float>(32.0f * (fLightRatio* CRenderer::CV_r_deferredshadinglightlodratio - fMinScreenAreaRatioThreshold), 0.0f, 1.0f ) );     

  //  float fDrawVolumeThres = 0.005f;
  //  if ( (fLightRatio* CRenderer::CV_r_deferredshadinglightlodratio)<fDrawVolumeThres )
  //  {
  //    bUseLightVolumes = false;
  //  }
  //}

  //const float fMinSpecThreshold = 0.025f;


//  if( CRenderer::CV_r_deferredshadingscissor )
//    rd->EF_Scissor(true, (int)pLightRect.x, (int)pLightRect.y, (int)pLightRect.z, (int)pLightRect.w);
//
//  if( bStencilMask )
//  {
//    SpecularAccEnableMRT( false );
//
//    rd->SetDepthBoundTest(0.0f, 1.0f, false); // stencil pre-passes are rop bound, using depth bounds increases even more rop cost
//    rd->FX_StencilFrustumCull(-1, pDL, NULL, 0);
//  }
//  else
//    if( rd->m_bDeviceSupports_NVDBT && CRenderer::CV_r_deferredshadingdepthboundstest == 1)
//      rd->SetDepthBoundTest( pDepthBounds.x, pDepthBounds.z, true);
//
//  // Render..
//  if( bStencilMask )
//  {
//#if !defined(XENON)
//    rd->FX_StencilRefresh(STENC_FUNC(FSS_STENCFUNC_EQUAL), rd->m_nStencilMaskRef, 0xFFFFFFFF);
//#endif
//
//    SpecularAccEnableMRT( true );
//  }
//

  rd->EF_Scissor(false, 0, 0, 1, 1);
  rd->SetDepthBoundTest(0.0f, 1.0f, false); // stencil pre-passes are rop bound, using depth bounds increases even more rop cost

  if (bUseLightVolumes)
  {
    //enable light volumes rendering
    rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pDeferredDecalTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    //m_nRenderState &= GS_WIREFRAME;
    //m_nRenderState |= GS_DEPTHFUNC_GREAT;
  }
  else
  {
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pDeferredDecalTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    //m_nRenderState &= ~GS_NODEPTHTEST;
    //m_nRenderState &= ~GS_DEPTHFUNC_MASK;
  }

  int nStates = m_nRenderState;

  nStates  &= ~GS_BLEND_MASK;

  //t                                                                                                                                                     
  //nStates |=GS_BLDST_ONEMINUSSRCCOL | GS_BLSRC_ONE;
  //d
  //nStates |=GS_BLDST_ONEMINUSSRCCOL | GS_BLSRC_ZERO;

  //darken
  nStates |=GS_BLSRC_DSTCOL | GS_BLDST_ONEMINUSSRCALPHA;

  rd->EF_SetState( nStates);
  if( bStencilMask )
  {
    rd->FX_StencilTestCurRef(true);
  }

  //////////////////////////////////////////////////////////////////////////
  IMaterial* pDecalMaterial = rDecal.pMaterial;
  if (pDecalMaterial==NULL)
  {
    assert(0);
    return;
  }

  SShaderItem & sItem = pDecalMaterial->GetShaderItem(0);

  ITexture* pDiffuseTex = NULL;
  if (sItem.m_pShaderResources==NULL)
  {
    assert(0);
    return;
  }

  if(SEfResTexture* pDiffuseRes= sItem.m_pShaderResources->GetTexture(EFTT_DIFFUSE))
  {
    if(pDiffuseRes->m_Sampler.m_pITex)
      pDiffuseTex = pDiffuseRes->m_Sampler.m_pITex;
  }

  assert(pDiffuseTex!=NULL);
  //////////////////////////////////////////////////////////////////////////

  SD3DPostEffectsUtils::SetTexture( (CTexture *)pDiffuseTex, 2, FILTER_LINEAR, 0);//, 2, FILTER_LINEAR); 

  //camera matrix
  m_pShader->FXSetPSFloat(m_pParamCameraMatrix, (Vec4*) m_pView.GetData(), 4);        

  //projection matrix
  {
    //Matrix44 ProjMatrixT;
    

    Matrix44 mLightProj;

//////////////////////////////////////////////////////////////////////////
    float fOffsetX = 0.5f;
    float fOffsetY = 0.5f;
#if defined (DIRECT3D9) || defined(PS3)
    //fOffsetX += (0.5f / max(pDiffuseTex->GetWidth(), 1.00001f) );

    //fOffsetY += (0.5f / max(pDiffuseTex->GetHeight(),1.00001f) );

#endif
    Matrix44A mTexScaleBiasMat = Matrix44A( 0.5f,     0.0f,     0.0f,    0.0f,  
      0.0f,    -0.5f,     0.0f,    0.0f,
      0.0f,     0.0f,     1.0f,    0.0f,
      fOffsetX, fOffsetY, 0.0f,    1.0f );


    Vec3 zaxis, yaxis, xaxis;
    //coord systems conversion(from orientation to shader matrix)
    zaxis =rDecal.projMatrix.GetColumn1().GetNormalized();
    xaxis =rDecal.projMatrix.GetColumn0().GetNormalized();
    yaxis =-rDecal.projMatrix.GetColumn2().GetNormalized();

    Vec3 vEyePt = rDecal.projMatrix.GetTranslation();
    //RH
    mLightProj(0,0) = xaxis.x;		mLightProj(0,1) = zaxis.x;	mLightProj(0,2) = yaxis.x;	mLightProj(0,3) = 0; 
    mLightProj(1,0) = xaxis.y;		mLightProj(1,1) = zaxis.y;	mLightProj(1,2) = yaxis.y;	mLightProj(1,3) = 0; 
    mLightProj(2,0) = xaxis.z;		mLightProj(2,1) = zaxis.z;	mLightProj(2,2) = yaxis.z;	mLightProj(2,3) = 0; 
    mLightProj(3,0) = -xaxis.Dot(vEyePt);	mLightProj(3,1) = -zaxis.Dot(vEyePt);	mLightProj(3,2) = -yaxis.Dot(vEyePt);	mLightProj(3,3) = 1;
//////////////////////////////////////////////////////////////////////////

    Matrix44A mProj;
    //mathMatrixPerspectiveFov( &mProj, (float)DEG2RAD_R(90.0f), 1.f, 0.025f, 5.0f);

    f32 fScale = rDecal.projMatrix.GetColumn0().GetLength();
    f32 yScale = fScale*2.0f;
    f32 xScale = fScale*2.0f;
    mathMatrixOrtho(&mProj, yScale, xScale, 0.0f, fScale);

    mLightProj = mLightProj * mProj;
    mLightProj = mLightProj * mTexScaleBiasMat;

    mLightProj.Transpose();

    m_pShader->FXSetPSFloat(m_pParamLightProjMatrix, (Vec4*) mLightProj.GetData(), 4);            
  }


  SD3DPostEffectsUtils::SetTexture( m_pDepthRT, 0, FILTER_POINT, 0);
  //SD3DPostEffectsUtils::SetTexture( m_pNormalsRT, 1, FILTER_POINT, 0); 
  //diffuse decal






  if( CRenderer::CV_r_deferredshadingdebug == 1)
    return;

  if (bUseLightVolumes)
  {
    //DrawLightVolume(pDL);
  }
  else
  {
    SD3DPostEffectsUtils::DrawScreenQuadWPOS(0, 0, m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight(),
      //CRenderer::CV_r_deferredshadingdepthboundstest > 1 ? pDepthBounds.x : 0.0f, 
      0.0f,
      m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight());
  }

  SD3DPostEffectsUtils::ShEndPass();


  if( bStencilMask )
  {
    rd->FX_StencilTestCurRef(false);
  }

  PROFILE_SHADER_END
}

//////////////////////////////////////////////////////////////////////////
void CDeferredShading::LightPass( const CDLight *pDL, bool bForceStencilDisable)
{
  PROFILE_FRAME(CDeferredShading_LightPass);
	PROFILE_SHADER_START;

  gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(RT_LIGHTSMASK);
   
  CD3D9Renderer *const __restrict rd = gcpRendD3D;

  bool bProj2D = (pDL->m_Flags & DLF_PROJECT) && pDL->m_pLightImage && !(pDL->m_pLightImage->GetFlags()&FT_REPLICATE_TO_ALL_SIDES);
  bool bStencilMask = (CRenderer::CV_r_deferredshadingstencilprepass && bProj2D ) || CRenderer::CV_r_DebugLightVolumes || (pDL->m_fProjectorNearPlane<0) || (pDL->m_Flags & DLF_HASCLIPBOUND);

  bool bUseLightVolumes = false;
  if( CRenderer::CV_r_deferredshadingLightVolumes)
  {
    float fSqLightRadius = pDL->m_fRadius * 1.22f;	// the light mesh tessellation and near clipping plane require some bias when testing if inside sphere
    fSqLightRadius *= fSqLightRadius;
    if (fSqLightRadius < pDL->m_Origin.GetSquaredDistance(m_pCamPos))
      bUseLightVolumes = true;
    else
      bStencilMask = true;
  }

  //reset stencil mask
  if (bForceStencilDisable)
  {
    bStencilMask = false;
  }

  // Store light properties (color/radius, position relative to camera, rect, zbounds)
  Vec4 pLightDiffuse = Vec4(pDL->m_Color.r, pDL->m_Color.g, pDL->m_Color.b, pDL->m_SpecMult);  
  pLightDiffuse.w *= min(1.0f, (pLightDiffuse.x + pLightDiffuse.y + pLightDiffuse.z)*0.333f );  

  bool bSunLight = (pDL->m_Flags & DLF_SUN)!=0;
  float fInvRadius = (!bSunLight)? (pDL->m_fRadius <= 0) ? 1.0f : 1.0f / pDL->m_fRadius : 0.0f;
	Vec4 pLightPosCS = (!bSunLight)? Vec4(pDL->m_Origin - m_pCamPos, fInvRadius) : Vec4((pDL->m_Origin - m_pCamPos).normalized(), 0.0f);  
  Vec4 pLightRect = (!bSunLight)? Vec4(pDL->m_sX * m_fRatioWidth, pDL->m_sY * m_fRatioHeight, pDL->m_sWidth * m_fRatioWidth, pDL->m_sHeight * m_fRatioHeight): Vec4(0.0f, 0.0f, 0.0f, 0.0f);        
  Vec4 pDepthBounds = (!bSunLight)? GetLightDepthBounds( pDL ) : Vec4(0.0f, 0.0f, 0.0f, 0.0f);

  if( CRenderer::CV_r_deferredshadinglightlodratio && !bSunLight)
  {
    //const float fMinDistThreshold = 64.0f;
    //pLightDiffuse.w *= 1.0f - min(1.0f, pDepthBounds.y / fMinDistThreshold);

    float fLightArea = pLightRect.z * pLightRect.w;
    float fScreenArea = (float) CTexture::s_ptexSceneTarget->GetWidth() * CTexture::s_ptexSceneTarget->GetHeight();
    float fLightRatio = fLightArea / fScreenArea;    
    const float fMinScreenAreaRatioThreshold = 0.01f ;  // 1% of screen by default
    //pLightDiffuse.w *= ( clamp_tpl<float>(32.0f * (fLightRatio* CRenderer::CV_r_deferredshadinglightlodratio - fMinScreenAreaRatioThreshold), 0.0f, 1.0f ) );     

    float fDrawVolumeThres = 0.005f;
    if ( (fLightRatio* CRenderer::CV_r_deferredshadinglightlodratio)<fDrawVolumeThres )
    {
      bUseLightVolumes = false;
    }
  }

  const float fMinSpecThreshold = 0.025f;
  if( pLightDiffuse.w <= fMinSpecThreshold )
    rd->m_RP.m_FlagsShader_RT |= RT_NO_SPECULAR;

	// Apply LBuffers range rescale
	pLightDiffuse.x *= rd->m_fAdaptedSceneScaleLBuffer;
	pLightDiffuse.y *= rd->m_fAdaptedSceneScaleLBuffer;
	pLightDiffuse.z *= rd->m_fAdaptedSceneScaleLBuffer;

  // Enable light pass flags
  if( (pDL->m_Flags & DLF_PROJECT) )
  {
    assert(!(pDL->GetDiffuseCubemap() && pDL->GetSpecularCubemap()));
    rd->m_RP.m_FlagsShader_RT |= RT_SPOTLIGHT;
    if( pDL->m_fProjectorNearPlane > 0.01f ) 
      rd->m_RP.m_FlagsShader_RT |= RT_CLIPEDSPOTLIGHT;
    if (bProj2D)
    {
      rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_LIGHT_TEX_PROJ];
    }
  }

  if( !(pDL->m_Flags & DLF_SUN) )  
  {
    if( CRenderer::CV_r_deferredshadingscissor )
      rd->EF_Scissor(true, (int)pLightRect.x, (int)pLightRect.y, (int)pLightRect.z, (int)pLightRect.w);

    if( bStencilMask )
    {
      SpecularAccEnableMRT( false );

      rd->SetDepthBoundTest(0.0f, 1.0f, false); // stencil pre-passes are rop bound, using depth bounds increases even more rop cost
      rd->FX_StencilFrustumCull(-1, pDL, NULL, 0);
    }
    else
    if( rd->m_bDeviceSupports_NVDBT && CRenderer::CV_r_deferredshadingdepthboundstest == 1)
      rd->SetDepthBoundTest( pDepthBounds.x, pDepthBounds.z, true);
  }
  else
  {
    // sun case needs optimization - should render only in areas affected by sun
    rd->EF_Scissor(false, 0, 0, 0, 0);
    rd->SetDepthBoundTest(0.0f, 1.0f, false);
    rd->m_RP.m_FlagsShader_RT |= RT_SUNLIGHT; 
  }

  // Render..
  if( bStencilMask )
  {
#if !defined(XENON)
    rd->FX_StencilRefresh(STENC_FUNC(FSS_STENCFUNC_EQUAL), rd->m_nStencilMaskRef, 0xFFFFFFFF);
#endif

    SpecularAccEnableMRT( true );
  }

  if (bUseLightVolumes)
  {
    //enable light volumes rendering
    rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pLightVolumeTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    //m_nRenderState &= GS_WIREFRAME;
    //m_nRenderState |= GS_DEPTHFUNC_GREAT;
  }
  else
  {
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    //m_nRenderState &= ~GS_NODEPTHTEST;
    //m_nRenderState &= ~GS_DEPTHFUNC_MASK;
  }

	int nStates = m_nRenderState;
	nStates &= ~GS_BLEND_MASK;
#ifndef PS3
	// use custom blending for PS3
	nStates |= (GS_BLSRC_ONE | GS_BLDST_ONE);
#endif
  rd->EF_SetState( nStates);
  if( bStencilMask )
  {
    rd->FX_StencilTestCurRef(true);
  }











  m_pShader->FXSetPSFloat(m_pParamCameraMatrix, (Vec4*) m_pView.GetData(), 4);        
  if( (pDL->m_Flags & DLF_PROJECT) )
  {
    Matrix44 ProjMatrixT;

    if (bProj2D)
    {
      CShadowUtils::GetProjectiveTexGen(pDL, 0, &ProjMatrixT);
    }
    else
    {
      ProjMatrixT = pDL->m_ProjMatrix;
    }

    ProjMatrixT.Transpose();
    m_pShader->FXSetPSFloat(m_pParamLightProjMatrix, (Vec4*) ProjMatrixT.GetData(), 4);            

    if( rd->m_RP.m_FlagsShader_RT & RT_CLIPEDSPOTLIGHT )
    {
      Vec4 pProjParams = Vec4( 1, 1, 1, max(pDL->m_fProjectorNearPlane, 0.0f));
      m_pShader->FXSetPSFloat(m_pParamLightProjParams, &pProjParams, 1);         
    }
  }
  m_pShader->FXSetPSFloat(m_pParamLightPos, &pLightPosCS, 1);         
  m_pShader->FXSetPSFloat(m_pParamLightDiffuse, &pLightDiffuse, 1);      

  m_pShader->FXSetVSFloat(m_pParamViewProjI, (Vec4*) m_pViewProjI.GetData(), 4);       

  SD3DPostEffectsUtils::SetTexture( m_pDepthRT, 0, FILTER_POINT, 0);    
  SD3DPostEffectsUtils::SetTexture( m_pNormalsRT, 1, FILTER_POINT, 0); 

  if( (pDL->m_Flags & DLF_PROJECT) && pDL->m_pLightImage)
    SD3DPostEffectsUtils::SetTexture( (CTexture *)pDL->m_pLightImage, 2, FILTER_LINEAR, 0);//, 2, FILTER_LINEAR); 






  if( CRenderer::CV_r_deferredshadingdebug == 1)
    return;

  if (bUseLightVolumes)
  {
    DrawLightVolume(pDL);
  }
  else
  {
    SD3DPostEffectsUtils::DrawScreenQuadWPOS(0, 0, m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight(),
      CRenderer::CV_r_deferredshadingdepthboundstest > 1 ? pDepthBounds.x : 0.0f, 
      m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight());
  }

  SD3DPostEffectsUtils::ShEndPass();






  if( bStencilMask )
  {
    rd->FX_StencilTestCurRef(false);
  }

  PROFILE_SHADER_END
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CDeferredShading::DrawAmbientIndexed()
{
	bool bAmbientRendered = false;






























































































































  return bAmbientRendered;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


bool CDeferredShading::AmbientPass(const CDLight *pGlobalCubemap)
{
  PROFILE_SHADER_START

  CD3D9Renderer *const __restrict rd = gcpRendD3D;

  uint32 nThreadID = gcpRendD3D->m_RP.m_nProcessThreadID;
  int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID] - 1;

  bool bAmbientRendered = false;
  
  PROFILE_FRAME(CDeferredShading_AmbientPass);
  PROFILE_LABEL_PUSH( "AMBIENT_PASS" );

  // Optimization/simplification todo (for single pass): 
  //  - create 32x1 rt (ps3 will have to be 64x64, minimum size there), each pixel contains area ambient color
  //  - acess directly stencil values, fetch ambient texture by stencil value

  static CCryName pszAmbientConst("g_cDeferredAmbient");
	static CCryName pszAmbGroundColConst("g_cAmbGround");
	static CCryName pszAmbHeigthParamsConst("g_vAmbHeightParams");

  const uint64 nFlagsShaderRT = gcpRendD3D->m_RP.m_FlagsShader_RT;
  gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]|g_HWSR_MaskBit[HWSR_SAMPLE3]|g_HWSR_MaskBit[HWSR_SAMPLE4]);

  gcpRendD3D->Set2DMode(true, 1, 1);   

  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexSceneNormalsMap->GetWidth(), CTexture::s_ptexSceneNormalsMap->GetHeight());        

  // Render vis areas ambient
  static const int32 nStencilState = STENC_FUNC(FSS_STENCFUNC_EQUAL) |STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP) | STENCOP_PASS(FSS_STENCOP_KEEP);
  bool bOutdoorVisible = m_nVisAreasCount[nThreadID][nRecurseLevel]? false : true; 
	const bool bVisAreasVisible = m_nVisAreasCount[nThreadID][nRecurseLevel] != 0;
  if(  bVisAreasVisible && !CRenderer::CV_r_ambientIndexedDraw)
  {
    VisAreaIDMapItor pItor = m_pVisAreas[nThreadID][nRecurseLevel].begin();
    VisAreaIDMapItor pEnd = m_pVisAreas[nThreadID][nRecurseLevel].end();
    for(  ; pItor != pEnd; ++pItor )
    {
      IVisArea *pVisArea = (IVisArea*)pItor->first;
      bOutdoorVisible |= pVisArea->IsConnectedToOutdoor();
      // Additional scissor + depth bounds test would be good

      SpecularAccEnableMRT( false );

      // Check stencil reference against vis area reference
      uint32 nVisAreaID = pItor->second;
      rd->m_nStencilMaskRef = nVisAreaID;
			
			if (CRenderer::CV_r_ambientPartialStencilRefresh)
			{
				// AABB geometry
				const Vec3 &bbMin = pVisArea->GetAABBox()->min;
				const Vec3 &bbMax = pVisArea->GetAABBox()->max;
				const Vec3 verts[8] = {
					Vec3(bbMin.x, bbMin.y, bbMin.z),
					Vec3(bbMin.x, bbMax.y, bbMin.z),
					Vec3(bbMax.x, bbMax.y, bbMin.z),
					Vec3(bbMax.x, bbMin.y, bbMin.z),
					Vec3(bbMin.x, bbMin.y, bbMax.z),
					Vec3(bbMin.x, bbMax.y, bbMax.z),
					Vec3(bbMax.x, bbMax.y, bbMax.z),
					Vec3(bbMax.x, bbMin.y, bbMax.z)
				};
				const uint16 indices[36] = {
					0, 1, 2, 0, 2, 3,
					7, 6, 5, 7, 5, 4,
					3, 2, 6, 3, 6, 7,
					4, 5, 1, 4, 1, 0,
					1, 5, 6, 1, 6, 2,
					4, 0, 3, 4, 3, 7
				};

				gcpRendD3D->Set2DMode(false, 1, 1);
				rd->FX_StencilRefreshPartial(STENC_FUNC(FSS_STENCFUNC_EQUAL), rd->m_nStencilMaskRef, 0xFFFFFFFF,
				                             (Vec3 *)verts, 8, (uint16 *)indices, 36);
				gcpRendD3D->Set2DMode(true, 1, 1);
			}
			else
			{
				rd->FX_StencilRefresh(STENC_FUNC(FSS_STENCFUNC_EQUAL), rd->m_nStencilMaskRef, 0xFFFFFFFF);
			}
			
      // Re-enable back mrt 
      SpecularAccEnableMRT( true ); 

      SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pAmbientTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

      rd->EF_SetState( GS_NODEPTHTEST );
      rd->FX_StencilTestCurRef( true );
			gcpRendD3D->D3DSetCull(eCULL_None);

      Vec4 cAmbientConst(pVisArea->GetFinalAmbientColor() * rd->m_fAdaptedSceneScaleLBuffer, 0);
      SD3DPostEffectsUtils::ShSetParamPS( pszAmbientConst, cAmbientConst );
      SD3DPostEffectsUtils::SetTexture( m_pNormalsRT, 0, FILTER_POINT); 
			SD3DPostEffectsUtils::DrawScreenQuadWPOS(0, 0, m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight(),
																								0, m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight());
      SD3DPostEffectsUtils::ShEndPass();

      rd->FX_StencilTestCurRef( false );
      
      bAmbientRendered = true; 
    }
  }
  else if ( bVisAreasVisible && CRenderer::CV_r_ambientIndexedDraw)
  {
		bAmbientRendered = DrawAmbientIndexed();
  }
 

	gcpRendD3D->Set2DMode(false, 1, 1);   

	gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(RT_LIGHTSMASK);
	
	if(!CRenderer::CV_r_deferredshadingcubemaps)
		pGlobalCubemap = NULL;

	// Store global cubemap color
	Vec4 pLightDiffuse;
	if(pGlobalCubemap)
	{
		pLightDiffuse = Vec4(pGlobalCubemap->m_Color.r, pGlobalCubemap->m_Color.g, pGlobalCubemap->m_Color.b, pGlobalCubemap->m_SpecMult);  
		pLightDiffuse.w *= min(1.0f, (pLightDiffuse.x + pLightDiffuse.y + pLightDiffuse.z)*0.333f );  

		const float fLuminance = pGlobalCubemap->m_Color.Luminance();

		// too dull => fill light
		if( fLuminance > 0.001f )
		{
			rd->m_RP.m_FlagsShader_RT |= RT_GLOBAL_CUBEMAP;
			// ignore specular if it's too dull
			if( fLuminance * pLightDiffuse.w >= 0.005f )
				rd->m_RP.m_FlagsShader_RT |= RT_SPECULAR_CUBEMAP;
		}
		else
			pGlobalCubemap = NULL;

		pLightDiffuse.x *= rd->m_fAdaptedSceneScaleLBuffer;
		pLightDiffuse.y *= rd->m_fAdaptedSceneScaleLBuffer;
		pLightDiffuse.z *= rd->m_fAdaptedSceneScaleLBuffer;
	}

	if( CRenderer::CV_r_deferredshadingheightbasedambient )
		rd->m_RP.m_FlagsShader_RT |= RT_HEIGHTBASEDAMBIENT;

  // Render outdoor ambient lighting
  //if( bOutdoorVisible )
  {
    Vec4 cOutdoorAmbient = Vec4(gEnv->p3DEngine->GetSkyColor(), 0);
    const float fMinAmbThreshold = 1.0f/255.0f;
		float fAmbientIntensity = fabsf( cOutdoorAmbient.x + cOutdoorAmbient.y + cOutdoorAmbient.z );
		if(pGlobalCubemap)
			fAmbientIntensity += pGlobalCubemap->m_Color.Luminance();
    if( fAmbientIntensity >= fMinAmbThreshold )
    {
			cOutdoorAmbient *= rd->m_fAdaptedSceneScaleLBuffer;

      // Check stencil state against outdoors reference
      if( CRenderer::CV_r_deferredshadingstencilprepass && bVisAreasVisible )
      {
        SpecularAccEnableMRT( false ); 

        rd->m_nStencilMaskRef = 0;
        rd->FX_StencilRefresh(STENC_FUNC(FSS_STENCFUNC_EQUAL), rd->m_nStencilMaskRef, 0xFFFFFFFF);

        SpecularAccEnableMRT( true ); 
      }

      SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pAmbientTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

      rd->EF_SetState( GS_NODEPTHTEST );
      if( CRenderer::CV_r_deferredshadingstencilprepass && bVisAreasVisible )
        rd->FX_StencilTestCurRef(true);

			Vec3 vE3DParam;
			gEnv->p3DEngine->GetGlobalParameter( E3DPARAM_AMBIENT_GROUND_COLOR, vE3DParam);

			Vec4 cAmbGroundColor = Vec4( vE3DParam *  rd->m_fAdaptedSceneScaleLBuffer, 0 );						
			Vec4 cAmbHeightParams = Vec4( gEnv->p3DEngine->GetGlobalParameter( E3DPARAM_AMBIENT_MIN_HEIGHT ), gEnv->p3DEngine->GetGlobalParameter( E3DPARAM_AMBIENT_MAX_HEIGHT ), 0, 0 );
			cAmbHeightParams.z = 1.0 / max( 0.0001f, cAmbHeightParams.y );
    
			SD3DPostEffectsUtils::ShSetParamPS( pszAmbientConst, cOutdoorAmbient );
			SD3DPostEffectsUtils::ShSetParamPS( pszAmbGroundColConst, cAmbGroundColor );
			SD3DPostEffectsUtils::ShSetParamPS( pszAmbHeigthParamsConst, cAmbHeightParams );

      SD3DPostEffectsUtils::SetTexture( m_pNormalsRT, 0, FILTER_POINT); 

			if(pGlobalCubemap)
			{
				SD3DPostEffectsUtils::ShSetParamPS(m_pParamLightDiffuse, pLightDiffuse);      
				SD3DPostEffectsUtils::SetTexture( (CTexture *)pGlobalCubemap->GetDiffuseCubemap(), 1, FILTER_BILINEAR);
				SD3DPostEffectsUtils::SetTexture( (CTexture *)pGlobalCubemap->GetSpecularCubemap(), 2, FILTER_TRILINEAR);
			}
			if( CRenderer::CV_r_deferredshadingheightbasedambient )
				SD3DPostEffectsUtils::SetTexture( m_pDepthRT, 3, FILTER_POINT); 

			SD3DPostEffectsUtils::DrawScreenQuadWPOS(0, 0, m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight(),
																								0, m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight());
      SD3DPostEffectsUtils::ShEndPass();

      bAmbientRendered = true;
    }
  }
  
	

  if( CRenderer::CV_r_deferredshadingstencilprepass && bVisAreasVisible )
  {
    rd->m_nStencilMaskRef = m_nVisAreasCount[nThreadID][nRecurseLevel] + 1;
    rd->FX_StencilTestCurRef(false);
  }

  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);    
//  gcpRendD3D->Set2DMode(false, 1, 1);   

  rd->m_RP.m_FlagsShader_RT = nFlagsShaderRT;

  PROFILE_LABEL_POP( "AMBIENT_PASS" );

  PROFILE_SHADER_END

  return bAmbientRendered;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CDeferredShading::DeferredCubemaps( const TArray<CDLight>& rCubemaps, const uint32 nStartIndex/* = 0 */ )
{
	if( nStartIndex < rCubemaps.Num() && CRenderer::CV_r_deferredshadingcubemaps )
	{
		// apply deferred cubemaps first
		PROFILE_LABEL_PUSH( "DEFERRED_CUBEMAPS" );		
		for(uint32 nCurrentCubemap = nStartIndex;nCurrentCubemap < rCubemaps.Num() ; ++nCurrentCubemap )
		{
			const CDLight &pDL = rCubemaps[nCurrentCubemap];
			assert(pDL.GetDiffuseCubemap());
			DeferredCubemapPass( &pDL );
			m_nLightsProcessedCount++;
		}
		PROFILE_LABEL_POP( "DEFERRED_CUBEMAPS" );





	}
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CDeferredShading::DeferredCubemapPass( const CDLight *pDL )
{
  PROFILE_SHADER_START

  PROFILE_FRAME(CDeferredShading_CubemapPass);

  CD3D9Renderer *const __restrict rd = gcpRendD3D;

	bool bStencilMask = (CRenderer::CV_r_deferredshadingstencilprepass) || CRenderer::CV_r_DebugLightVolumes;
	
	bool bHasSpecular = false;

  const uint64 nOldFlags = rd->m_RP.m_FlagsShader_RT;

  rd->m_RP.m_FlagsShader_RT &= ~(RT_LIGHTSMASK|RT_HEIGHTBASEDAMBIENT|RT_GLOBAL_CUBEMAP|RT_SPECULAR_CUBEMAP);

	// todo: add check - if inside vis area, skip this step
	if( CRenderer::CV_r_deferredshadingheightbasedambient )
		rd->m_RP.m_FlagsShader_RT |= RT_HEIGHTBASEDAMBIENT;

  // Store light properties (color/radius, position relative to camera, rect, z bounds)
  Vec4 pLightDiffuse = Vec4(pDL->m_Color.r, pDL->m_Color.g, pDL->m_Color.b, pDL->m_SpecMult);  
	pLightDiffuse.w *= min(1.0f, (pLightDiffuse.x + pLightDiffuse.y + pLightDiffuse.z)*0.333f );  

  float fInvRadius = (pDL->m_fRadius <= 0) ? 1.0f : 1.0f / pDL->m_fRadius;
  Vec4 pLightPosCS = Vec4(pDL->m_Origin - m_pCamPos, fInvRadius);  

  Vec4 pLightRect = Vec4(pDL->m_sX * m_fRatioWidth, pDL->m_sY * m_fRatioHeight, pDL->m_sWidth * m_fRatioWidth, pDL->m_sHeight * m_fRatioHeight);        
  Vec4 pDepthBounds = GetLightDepthBounds( pDL );

  assert(!(pDL->m_Flags & DLF_PROJECT));

  if( CRenderer::CV_r_deferredshadinglightlodratio)
  {
    //const float fMinDistThreshold = 64.0f;
    //pLightDiffuse.w *= 1.0f - min(1.0f, pDepthBounds.y / fMinDistThreshold);

    float fLightArea = pLightRect.z * pLightRect.w;
    float fScreenArea = (float) CTexture::s_ptexSceneTarget->GetWidth() * CTexture::s_ptexSceneTarget->GetHeight();
    float fLightRatio = fLightArea / fScreenArea;    
    const float fMinScreenAreaRatioThreshold = 0.01f ;  // 1% of screen by default
    //pLightDiffuse.w *= ( clamp_tpl<float>(32.0f * (fLightRatio* CRenderer::CV_r_deferredshadinglightlodratio - fMinScreenAreaRatioThreshold), 0.0f, 1.0f ) );     

    float fDrawVolumeThres = 0.01f;
    if ( (fLightRatio* CRenderer::CV_r_deferredshadinglightlodratio)<fDrawVolumeThres )
    {
      //scissor + depthbound test only
      bStencilMask = false;
    }
  }

  if( CRenderer::CV_r_deferredshadingscissor )
    rd->EF_Scissor(true, (int)pLightRect.x, (int)pLightRect.y, (int)pLightRect.z, (int)pLightRect.w);

	SpecularAccEnableMRT( false );
    
  if( bStencilMask )
  {
    rd->SetDepthBoundTest(0.0f, 1.0f, false);
    rd->FX_StencilFrustumCull(-1, pDL, NULL, 0);

#if !defined(XENON)
    // check stencil reference for regular cubes - and against vis area reference for global cube
    rd->FX_StencilRefresh(STENC_FUNC(FSS_STENCFUNC_EQUAL), rd->m_nStencilMaskRef, 0xFFFFFFFF);
#endif
    rd->FX_StencilTestCurRef(true);

  }
  else
  if( rd->m_bDeviceSupports_NVDBT && CRenderer::CV_r_deferredshadingdepthboundstest == 1)
    rd->SetDepthBoundTest( pDepthBounds.x, pDepthBounds.z, true);

	const float fLuminance = pDL->m_Color.Luminance();

	// too dull => fill light
	if( fLuminance <= 0.03f )
	{
		rd->m_RP.m_FlagsShader_RT |= RT_NEGATIVE_LIGHT;
		pLightDiffuse.w = 1.f;		// replace specular term with the negativeness
	}
	else	// ignore specular if it's too dull
	{
		if( fLuminance * pLightDiffuse.w >= 0.03f )
		{
			rd->m_RP.m_FlagsShader_RT |= RT_SPECULAR_CUBEMAP;
			bHasSpecular = true;
			SpecularAccEnableMRT( bHasSpecular );
		}
	}	

	// Apply LBuffers range rescale
	pLightDiffuse.x *= rd->m_fAdaptedSceneScaleLBuffer;
	pLightDiffuse.y *= rd->m_fAdaptedSceneScaleLBuffer;
	pLightDiffuse.z *= rd->m_fAdaptedSceneScaleLBuffer;
	
  // Render..
  SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pCubemapsTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

  int MultplyState = m_nRenderState;
  MultplyState &= ~GS_BLEND_MASK;

#ifndef PS3
	MultplyState |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
#endif

	if( CRenderer::CV_r_deferredshadingdebug == 3)
	{
		// Debug mode
		MultplyState &= ~GS_BLEND_MASK;
		MultplyState |= (GS_BLSRC_ONE | GS_BLDST_ONE) ;
	}

	if( bStencilMask )
		MultplyState |= GS_STENCIL;

	
  rd->EF_SetState(MultplyState);
  
  m_pShader->FXSetPSFloat(m_pParamCameraMatrix, (Vec4*) m_pView.GetData(), 4);
  m_pShader->FXSetPSFloat(m_pParamLightPos, &pLightPosCS, 1);         
  m_pShader->FXSetPSFloat(m_pParamLightDiffuse, &pLightDiffuse, 1);      

  m_pShader->FXSetVSFloat(m_pParamViewProjI, (Vec4*) m_pViewProjI.GetData(), 4);       

  SD3DPostEffectsUtils::SetTexture( m_pDepthRT, 0, FILTER_POINT, 0);    
  SD3DPostEffectsUtils::SetTexture( m_pNormalsRT, 1, FILTER_POINT, 0); 

  SD3DPostEffectsUtils::SetTexture( (CTexture *)pDL->GetDiffuseCubemap(), 2, FILTER_BILINEAR);
  SD3DPostEffectsUtils::SetTexture( (CTexture *)pDL->GetSpecularCubemap(), 3, FILTER_TRILINEAR);






  if( CRenderer::CV_r_deferredshadingdebug != 1)
  {
    SD3DPostEffectsUtils::DrawScreenQuadWPOS(0, 0, m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight(),
      0.0f, m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight());
  }

  SD3DPostEffectsUtils::ShEndPass();
  
  if(!bHasSpecular)
  {
		SpecularAccEnableMRT(true); 
  }

  if( bStencilMask )
    rd->FX_StencilTestCurRef(false);
	else if( rd->m_bDeviceSupports_NVDBT && CRenderer::CV_r_deferredshadingdepthboundstest == 1)
		rd->SetDepthBoundTest( 0.f, 1.f, false);

	if( CRenderer::CV_r_deferredshadingscissor )
		rd->EF_Scissor(false, 0, 0, 0, 0);

  rd->m_RP.m_FlagsShader_RT = nOldFlags;

  PROFILE_SHADER_END
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CDeferredShading::DeferredLights( const TArray<CDLight>& rLights )
{
  if(rLights.Num())
  {
    PROFILE_LABEL_PUSH( "DEFERRED_LIGHTS" );
    for(uint32 nCurrentLight = 0; nCurrentLight < rLights.Num() ; ++nCurrentLight )
    {
      const CDLight &pDL = rLights[nCurrentLight];
      assert(pDL.GetSpecularCubemap() == NULL);
      if ( !(pDL.m_Flags & DLF_CASTSHADOW_MAPS) )
        LightPass( &pDL );
      else
        if ( (pDL.m_Flags & DLF_CASTSHADOW_MAPS) )
        {
          if (pDL.m_Flags & DLF_DIRECTIONAL )
            SunLightPasses(pDL, pDL.m_Id);
          else
          {
            if (!ShadowLightPasses(pDL, nCurrentLight))
              LightPass( &pDL, false);
          }
        }

        if( CRenderer::CV_r_deferredshadingdebug == 2)
          break;

        m_nLightsProcessedCount++;
    }
    PROFILE_LABEL_POP( "DEFERRED_LIGHTS" );
  }
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CDeferredShading::NegativeLightPass( const CDLight *pDL )
{
	PROFILE_SHADER_START

	PROFILE_FRAME(CDeferredShading_NegativeLightPass);

	CD3D9Renderer *const __restrict rd = gcpRendD3D;

	bool bStencilMask = (CRenderer::CV_r_deferredshadingstencilprepass) || CRenderer::CV_r_DebugLightVolumes;

	const uint64 nOldFlags = rd->m_RP.m_FlagsShader_RT;

	rd->m_RP.m_FlagsShader_RT &= ~(RT_LIGHTSMASK|RT_HEIGHTBASEDAMBIENT|RT_GLOBAL_CUBEMAP|RT_SPECULAR_CUBEMAP);

	// Store light properties (color/radius, position relative to camera, rect, z bounds)
	const float fInvRadius = (pDL->m_fRadius <= 0) ? 1.0f : 1.0f / pDL->m_fRadius;
	const Vec4 pLightPosCS = Vec4(pDL->m_Origin - m_pCamPos, fInvRadius);  

	const Vec4 pLightRect = Vec4(pDL->m_sX * m_fRatioWidth, pDL->m_sY * m_fRatioHeight, pDL->m_sWidth * m_fRatioWidth, pDL->m_sHeight * m_fRatioHeight);        
	const Vec4 pDepthBounds = GetLightDepthBounds( pDL );

	assert(!(pDL->m_Flags & DLF_PROJECT));

	if( CRenderer::CV_r_deferredshadinglightlodratio)
	{
		const float fLightArea = pLightRect.z * pLightRect.w;
		const float fScreenArea = (float) CTexture::s_ptexSceneTarget->GetWidth() * CTexture::s_ptexSceneTarget->GetHeight();
		const float fLightRatio = fLightArea / fScreenArea;    
		const float fMinScreenAreaRatioThreshold = 0.01f ;  // 1% of screen by default

		float fDrawVolumeThres = 0.01f;
		if ( (fLightRatio * CRenderer::CV_r_deferredshadinglightlodratio) < fDrawVolumeThres )
		{
			//scissor + depthbound test only
			bStencilMask = false;
		}
	}

	if( CRenderer::CV_r_deferredshadingscissor )
		rd->EF_Scissor(true, (int)pLightRect.x, (int)pLightRect.y, (int)pLightRect.z, (int)pLightRect.w);

	if( bStencilMask )
	{
		rd->SetDepthBoundTest(0.0f, 1.0f, false);
		rd->FX_StencilFrustumCull(-1, pDL, NULL, 0);

#if !defined(XENON)
		// check stencil reference for regular cubes - and against vis area reference for global cube
		rd->FX_StencilRefresh(STENC_FUNC(FSS_STENCFUNC_EQUAL), rd->m_nStencilMaskRef, 0xFFFFFFFF);
#endif
		rd->FX_StencilTestCurRef(true);
	}
	else
		if( rd->m_bDeviceSupports_NVDBT && CRenderer::CV_r_deferredshadingdepthboundstest == 1)
			rd->SetDepthBoundTest( pDepthBounds.x, pDepthBounds.z, true);

	rd->m_RP.m_FlagsShader_RT |= RT_NEGATIVE_LIGHT;

	// Render..
	SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pCubemapsTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

	int MultplyState = m_nRenderState;
	MultplyState &= ~GS_BLEND_MASK;

#ifndef PS3
	MultplyState |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
#endif

	if( CRenderer::CV_r_deferredshadingdebug == 3)
	{
		// Debug mode
		MultplyState &= ~GS_BLEND_MASK;
		MultplyState |= (GS_BLSRC_ONE | GS_BLDST_ONE) ;
	}

	if( bStencilMask )
		MultplyState |= GS_STENCIL;

	rd->EF_SetState(MultplyState);

	// Store light properties (color/radius, position relative to camera, rect, z bounds)
	Vec4 vLightDiffuse = Vec4(pDL->m_Color.r, pDL->m_Color.g, pDL->m_Color.b, pDL->m_SpecMult);  
	vLightDiffuse.w = pDL->m_Color.Luminance();  

	// Apply LBuffers range rescale
	vLightDiffuse.x *= rd->m_fAdaptedSceneScaleLBuffer;
	vLightDiffuse.y *= rd->m_fAdaptedSceneScaleLBuffer;
	vLightDiffuse.z *= rd->m_fAdaptedSceneScaleLBuffer;

	m_pShader->FXSetPSFloat(m_pParamLightDiffuse, &vLightDiffuse, 1);      

	m_pShader->FXSetPSFloat(m_pParamCameraMatrix, (Vec4*) m_pView.GetData(), 4);
	m_pShader->FXSetPSFloat(m_pParamLightPos, &pLightPosCS, 1);         

	m_pShader->FXSetVSFloat(m_pParamViewProjI, (Vec4*) m_pViewProjI.GetData(), 4);       

	SD3DPostEffectsUtils::SetTexture( m_pDepthRT, 0, FILTER_POINT, 0);    
	SD3DPostEffectsUtils::SetTexture( m_pNormalsRT, 1, FILTER_POINT, 0); 






	if( CRenderer::CV_r_deferredshadingdebug != 1)
	{
		SD3DPostEffectsUtils::DrawScreenQuadWPOS(0, 0, m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight(),
			0.0f, m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight());
	}

	SD3DPostEffectsUtils::ShEndPass();

	if( bStencilMask )
		rd->FX_StencilTestCurRef(false);
	else if( rd->m_bDeviceSupports_NVDBT && CRenderer::CV_r_deferredshadingdepthboundstest == 1)
		rd->SetDepthBoundTest( 0.f, 1.f, false);

	if( CRenderer::CV_r_deferredshadingscissor )
		rd->EF_Scissor(false, 0, 0, 0, 0);

	rd->m_RP.m_FlagsShader_RT = nOldFlags;

	PROFILE_SHADER_END
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CDeferredShading::SunLightPasses(const CDLight& light, const int nLightID)
{
  PROFILE_SHADER_START

  CD3D9Renderer *const __restrict rd = gcpRendD3D;

  assert (light.m_Flags & DLF_DIRECTIONAL);
  int nThreadID = rd->m_RP.m_nProcessThreadID;
  int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID]-1;
  int nDLights = rd->m_RP.m_DLights[nThreadID][nCurRecLevel].Num();

  //////////////////////////////////////////////////////////////////////////
  int nStartIdx = SRendItem::m_StartFrust[nThreadID][nLightID + nDLights];
  int nEndIdx = SRendItem::m_EndFrust[nThreadID][nLightID + nDLights];
  assert((nEndIdx-nStartIdx)<=MAX_GSM_LODS_NUM);

  int nCasters = nEndIdx - nStartIdx;

  int nOffs;
  rd->FX_CreateDeferredQuad(nOffs, &light, m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight(), NULL, NULL);
  rd->FX_SetVStream( 0, rd->m_pVB[ POOL_P3F_TEX2F_TEX3F ], 0, sizeof( SVF_P3F_T2F_T3F ) ); 

  // ortho projection matrix
  Matrix44A mQuadProj;
  //init matrix for deferred quads rendering
  mathMatrixOrthoOffCenterLH( &mQuadProj , 0, 1, 0, 1, -1, 1 );
  rd->m_RP.m_TI[nThreadID].m_matProj->Push();
  rd->m_RP.m_TI[nThreadID].m_matProj->LoadMatrix(&mQuadProj);  
  rd->m_RP.m_TI[nThreadID].m_matView->Push();
  rd->m_RP.m_TI[nThreadID].m_matView->LoadIdentity();
  rd->EF_DirtyMatrix();

	//// TODO: optimize that. Stencul buffer gets dirty after deferred cubemaps?
	//rd->EF_ClearBuffers(FRT_CLEAR_STENCIL|FRT_CLEAR_IMMEDIATE, NULL);

	//if( CRenderer::CV_r_deferredshadingscissor )
	//	rd->EF_Scissor(false, 0, 0, 0, 0);

	//if( CRenderer::CV_r_deferredshadingdepthboundstest )
	//	rd->SetDepthBoundTest(0.0f, 1.0f, false);

  //SpecularAccEnableMRT( false );

  //////////////////////////////////////////////////////////////////////////
  int nFrustIdx;
  for(nFrustIdx=(nEndIdx-1); nFrustIdx >= nStartIdx; --nFrustIdx)
  {       
    ShadowMapFrustum& shadowFrustum = rd->m_RP.m_SMFrustums[nThreadID][nCurRecLevel][nFrustIdx];

    if( shadowFrustum.bForSubSurfScattering||
      shadowFrustum.bUseAdditiveBlending
      ) 
      continue;

    rd->FX_DeferredShadowPass(&light, -1, &shadowFrustum, /*fFinalRange[nCaster]*/0, nOffs, false, true, nFrustIdx-nStartIdx+1);
  }

  rd->m_RP.m_TI[nThreadID].m_matProj->Pop();
  rd->m_RP.m_TI[nThreadID].m_matView->Pop();
  rd->EF_DirtyMatrix();


  rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];

  for(nFrustIdx=nStartIdx; nFrustIdx < nEndIdx; ++nFrustIdx)
  {       
    ShadowMapFrustum& shadowFrustum = rd->m_RP.m_SMFrustums[nThreadID][nCurRecLevel][nFrustIdx];


    SetupPasses();

    m_nRenderState |= GS_STENCIL;

    //m_nRenderState  &= ~GS_BLEND_MASK;

    //enable hw-pcf per frustum
    if (shadowFrustum.bHWPCFCompare)
    {
      rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];
    }

    rd->ConfigShadowTexgen( 0, &shadowFrustum, -1, -1, &light);

    //SpecularAccEnableMRT( true );

    SD3DPostEffectsUtils::SetTexture( shadowFrustum.pDepthTex->m_pTexture, 3, FILTER_LINEAR, 0 );
    //////////////////////////////////////////////////////////////////////////                         e

    rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VARIANCE_SM];

    rd->EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_EQUAL) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
      STENCOP_PASS(FSS_STENCOP_KEEP),
      (nFrustIdx-nStartIdx+1)|0x80, 0xFF, 0xFF);
    LightPass( &light, true);

    rd->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_VARIANCE_SM];
    rd->EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_EQUAL) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
      STENCOP_PASS(FSS_STENCOP_KEEP),
      (nFrustIdx-nStartIdx+1), 0xFF, 0xFF);
    LightPass( &light, true);

  }


  rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE4]|g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ]);

  m_nRenderState &= ~GS_STENCIL;
  rd->EF_SetState(m_nRenderState);

  PROFILE_SHADER_END
}

bool CDeferredShading::ShadowLightPasses(const CDLight& light, const int nLightID)
{
  PROFILE_SHADER_START

  CD3D9Renderer *const __restrict rd = gcpRendD3D;

  int nThreadID = rd->m_RP.m_nProcessThreadID;
  int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID]-1;
  int nDLights = rd->m_RP.m_DLights[nThreadID][nCurRecLevel].Num();

  //////////////////////////////////////////////////////////////////////////
  int nFrustumIdx = nLightID + nDLights;
  int nStartIdx = SRendItem::m_StartFrust[nThreadID][nFrustumIdx];
  int nEndIdx = SRendItem::m_EndFrust[nThreadID][nFrustumIdx];

  //no single frustum was allocated for this light
  if(nEndIdx<=nStartIdx)
    return false;
  
  if(nCurRecLevel<0 || nCurRecLevel>=rd->m_RP.m_SMFrustums[nThreadID]->Num())
    return false;

  ShadowMapFrustum &firstFrustum = rd->m_RP.m_SMFrustums[nThreadID][nCurRecLevel][nStartIdx];
  int nSides = 1;
  if (firstFrustum.bOmniDirectionalShadow)
    nSides = 6;

  //enable shadow mapping
  rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];

  //enable hw-pcf per frustum
  if (firstFrustum.bHWPCFCompare)
  {
    rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];
  }

  rd->m_nStencilMaskRef+= (nSides+2);
  if (rd->m_nStencilMaskRef>255)
  {
    rd->EF_ClearBuffers(FRT_CLEAR_STENCIL|FRT_CLEAR_IMMEDIATE, NULL);
    rd->m_nStencilMaskRef= nSides+1;
  }

  for (int nSi=-1; nSi<nSides; nSi++)
  {
    //TI
    int nS;
    if (nSi<0)
      nS = 0;
    else
      nS = nSi;

    //FIX: temp solution for projector's camera
    //TF enable linear shadow space and disable this back faces for projectors
    if (light.m_Flags & DLF_PROJECT)
    {
      rd->m_RP.m_TI[nThreadID].m_PersFlags &= ~RBPF_MIRRORCULL;
    }
    else
    {
      rd->m_RP.m_TI[nThreadID].m_PersFlags |= RBPF_MIRRORCULL;
    }

    SpecularAccEnableMRT( false );

    //use current WorldProj matrix
    rd->FX_StencilFrustumCull(-2, &light, &firstFrustum, nS);
    rd->m_RP.m_TI[nThreadID].m_PersFlags &= ~RBPF_MIRRORCULL;

    rd->EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_EQUAL) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
      STENCOP_PASS(FSS_STENCOP_KEEP),
      rd->m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFFFFFF);

    SetupPasses();

    m_nRenderState |= GS_STENCIL;




    rd->ConfigShadowTexgen( 0, &firstFrustum, -1, nS, &light);

    SpecularAccEnableMRT( true );

    SD3DPostEffectsUtils::SetTexture( firstFrustum.pDepthTex->m_pTexture, 3, FILTER_LINEAR, 0 ); 
    //////////////////////////////////////////////////////////////////////////

    LightPass( &light, true);

  }

  //assign range
  rd->m_nStencilMaskRef+=nSides;

  //////////////////////////////////////////////////////////////////////////

  rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE4]|g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ]);

  m_nRenderState &= ~GS_STENCIL;
  rd->EF_SetState(m_nRenderState);

  /*rd->EF_SetStencilState(
  STENCOP_FAIL(FSS_STENCOP_KEEP) |
  STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
  STENCOP_PASS(FSS_STENCOP_KEEP) |
  STENC_FUNC(FSS_STENCFUNC_EQUAL),
  0x0, 0xFFFFFFFF, 0xFFFFFFFF
  );*/
  //////////////////////////////////////////////////////////////////////////

  return true;

  PROFILE_SHADER_END
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::CreateDeferredMaps()
{
	int nWidth = gcpRendD3D->GetWidth(); 
	int nHeight = gcpRendD3D->GetHeight();

	ETEX_Format nNormalsFormat = eTF_A8R8G8B8;	




	SD3DPostEffectsUtils::CreateRenderTarget( "$SceneNormalsMap", CTexture::s_ptexSceneNormalsMap, nWidth, nHeight, true, false, nNormalsFormat, TO_SCENE_NORMALMAP);	
//	SD3DPostEffectsUtils::CreateRenderTarget( "$SceneTextures", CTexture::s_ptexSceneTexturesMap, nWidth, nHeight, true, false, eTF_A8R8G8B8, TO_SCENE_TEXTURES);





	ETEX_Format nTexFormat = eTF_A16B16G16R16F;	
	int nFTFlags = 0;












	SD3DPostEffectsUtils::CreateRenderTarget( "$SceneDiffuseAcc", CTexture::s_ptexSceneDiffuseAccMap, nWidth, nHeight, true, false, nTexFormat, TO_SCENE_DIFFUSE_ACC, nFTFlags);
	CTexture::s_ptexCurrentSceneDiffuseAccMap = CTexture::s_ptexSceneDiffuseAccMap;
	SD3DPostEffectsUtils::CreateRenderTarget( "$SceneSpecularAcc", CTexture::s_ptexSceneSpecularAccMap, nWidth, nHeight, true, false, nTexFormat, TO_SCENE_SPECULAR_ACC, nFTFlags);














	if( CRenderer::CV_r_deferredshadingdebug == 3)
		SD3DPostEffectsUtils::CreateRenderTarget( "$DeferredDebug", m_pDebugRT, nWidth, nHeight, true);    
	else
		SAFE_RELEASE(m_pDebugRT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::DestroyDeferredMaps()
{
	SAFE_RELEASE(CTexture::s_ptexSceneNormalsMap);
	SAFE_RELEASE(CTexture::s_ptexSceneDiffuseAccMap);
	SAFE_RELEASE(CTexture::s_ptexSceneSpecularAccMap);




	SAFE_RELEASE(m_pDebugRT);
	SAFE_DELETE(m_pSSAORT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::DeferredScenePass()
{
	PROFILE_LABEL_PUSH( "DEFERRED_SCENE_PASS" );

	CD3D9Renderer *const __restrict rd = gcpRendD3D;
	const uint64 nFlagsShaderRT = gcpRendD3D->m_RP.m_FlagsShader_RT;
	gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]|g_HWSR_MaskBit[HWSR_SAMPLE3]);

	gcpRendD3D->Set2DMode(true, 1, 1);   

	int iTempX, iTempY, iWidth, iHeight;
	gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

	gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexSceneNormalsMap->GetWidth(), CTexture::s_ptexSceneNormalsMap->GetHeight());        

	// Render..
	SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pDeferredSceneTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

	rd->EF_SetState( GS_NODEPTHTEST );
	gcpRendD3D->D3DSetCull(eCULL_Back);

	SD3DPostEffectsUtils::SetTexture( m_pSceneTexturesRT, 0, FILTER_POINT); 
	SD3DPostEffectsUtils::SetTexture( m_pDiffuseAccRT, 1, FILTER_POINT); 
	SD3DPostEffectsUtils::SetTexture( m_pSpecularAccRT, 2, FILTER_POINT); 
  SD3DPostEffectsUtils::SetTexture( m_pNormalsRT, 3, FILTER_POINT); 

	SD3DPostEffectsUtils::DrawFullScreenQuad(m_pDiffuseAccRT->GetWidth(), m_pDiffuseAccRT->GetHeight());  

	SD3DPostEffectsUtils::ShEndPass();

	gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);    

	gcpRendD3D->Set2DMode(false, 1, 1);   

	rd->m_RP.m_FlagsShader_RT = nFlagsShaderRT;

	PROFILE_LABEL_POP( "DEFERRED_SCENE_PASS" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

struct CubemapsCompare
{
	bool operator()(const CDLight& l0, const CDLight& l1) const
	{
		return l0.m_fRadius > l1.m_fRadius;
	}
};

struct LightsCompare
{
  bool operator()(const CDLight& l0, const CDLight& l1) const
  {
    if ( !(l0.m_Flags&DLF_CASTSHADOW_MAPS) && (l1.m_Flags&DLF_CASTSHADOW_MAPS) )
    {
      return true;
    }
    else
      return false;
  }
};


void CDeferredShading::Render()
{  
  m_nLightsProcessedCount = 0;

  CD3D9Renderer *const __restrict rd = gcpRendD3D;

	uint32 nThreadID = gcpRendD3D->m_RP.m_nProcessThreadID;
	int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID] - 1;

	TArray<CDLight>& rDeferredLights						= m_pLights[eDLT_DeferredLight][nThreadID][nRecurseLevel];
	TArray<CDLight>& rDeferredSSAOLights				= m_pLights[eDLT_DeferredAmbientLight][nThreadID][nRecurseLevel];
	TArray<CDLight>& rDeferredCubemaps					= m_pLights[eDLT_DeferredCubemap][nThreadID][nRecurseLevel];
	TArray<CDLight>& rDeferredSSAOCubemaps			= m_pLights[eDLT_DeferredAmbientCubemap][nThreadID][nRecurseLevel];
	TArray<CDLight>& rDeferredNegativeLights		= m_pLights[eDLT_DeferredNegative][nThreadID][nRecurseLevel];

  const bool bRenderIrradianceVolumes = IrrVolumes.IsRenderable();

	// Avoid using ResetToDefault - it's reseting everything, including gpr's states - we leave this comment out for now
	// in case any problems
	//rd->ResetToDefault();

  rd->FX_ResetPipe();





  uint64 nFlagsShaderRT = gcpRendD3D->m_RP.m_FlagsShader_RT;

  SetupPasses();

  if( CRenderer::CV_r_deferredshadingscissor )
    rd->EF_Scissor(false, 0, 0, 0, 0);

	if( CRenderer::CV_r_deferredshadingdepthboundstest )
		rd->SetDepthBoundTest(0.f, 0.99999f, true); // skip sky  for ambient and deferred cubemaps

  int iTempX, iTempY, iWidth, iHeight;
  rd->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  static const ColorF BlackColor(0.0f, 0.0f, 0.0f, 0.0f);

  // Process deferred direct lighting accumulation
  rd->FX_PushRenderTarget(0, m_pDiffuseAccRT, &gcpRendD3D->m_DepthBufferOrigFSAA, 0, -1, false, 1);
#if !defined(XENON_FORCE_720P)
	rd->FX_PushRenderTarget(1, m_pSpecularAccRT, NULL, 0, -1, false, 1);
#endif

	// sort lights
	if(rDeferredSSAOCubemaps.Num())
	{
		std::sort(rDeferredSSAOCubemaps.begin(), rDeferredSSAOCubemaps.end(), CubemapsCompare());
	}

	uint32 nCurrentDeferredCubemap = 0;

	// ambient prepass
	if(CRenderer::CV_r_deferredshadingdebug < 3)
	{
		// determine if we have a global cubemap in the scene
		const CDLight* pGlobalCubemap = NULL;
		if(rDeferredSSAOCubemaps.Num() && CRenderer::CV_r_deferredshadingcubemaps)
		{
			const CDLight& pFirstLight = rDeferredSSAOCubemaps[0];

			if(pFirstLight.GetDiffuseCubemap() && pFirstLight.m_fRadius >= 100000.f)
			{
				pGlobalCubemap = &pFirstLight;
				++nCurrentDeferredCubemap;
			}
		}
		if( AmbientPass(pGlobalCubemap) )
			m_nLightsProcessedCount++;
	}

	// Clear L-buffers if required
	if( !m_nLightsProcessedCount || gRenDev->GetWireframeMode() )
		rd->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &BlackColor); 

	// apply deferred SSAO-affected cubemaps first
	DeferredCubemaps(rDeferredSSAOCubemaps);

	// apply negative lights after deferred cubemaps
	if( rDeferredNegativeLights.Num() )
	{
		PROFILE_LABEL_PUSH( "NEGATIVE_LIGHTS" );		
		SpecularAccEnableMRT( false );
		for(uint32 nCurrentNegLight = 0;nCurrentNegLight < rDeferredNegativeLights.Num() ; ++nCurrentNegLight )
		{
			const CDLight &pDL = rDeferredNegativeLights[nCurrentNegLight];
			assert(pDL.GetDiffuseCubemap() == NULL);
			NegativeLightPass( &pDL );
			m_nLightsProcessedCount++;
		}
		SpecularAccEnableMRT( true );
		PROFILE_LABEL_POP( "NEGATIVE_LIGHTS" );
	}

	rd->EF_Scissor(false, 0, 0, 0, 0);

	if( CRenderer::CV_r_deferredshadingdepthboundstest )
		rd->SetDepthBoundTest(CRenderer::CV_r_DrawNearZRange, 0.99999f, true); // skip sky and near objects for SSAO/irradiance volumes

	// apply deferred SSAO-affected light sources
	DeferredLights(rDeferredSSAOLights);

	SpecularAccEnableMRT( false );

	// apply GI
	if(IrrVolumes.IsGIRenderable() && (CRenderer::CV_r_irradiancevolumes == 1 || CRenderer::CV_r_deferredshading != 3))
	{
		IrrVolumes.RenderGI(m_pDepthRT, m_pNormalsRT);
		m_nLightsProcessedCount++;
	}

	if( CRenderer::CV_r_SSAO && CRenderer::CV_r_deferredshadingdebug != 3)
	{
		// multiply by SSAO directly into L-Buffer
		PROFILE_SHADER_START;
		const uint64 nSSAOFlagsShaderRT = rd->m_RP.m_FlagsShader_RT & ( ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]) ) ;
		rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];	// means we need multiply
		if (CRenderer::CV_r_SSAO_downscale)
			rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
		if (CRenderer::CV_r_SSAO_downscale_ztarget)
			rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

		rd->FX_ShadowBlur(1.f, m_pSSAORT, m_pDiffuseAccRT, 4);
		rd->m_RP.m_FlagsShader_RT = nSSAOFlagsShaderRT;

		SAFE_DELETE(m_pSSAORT);
		PROFILE_SHADER_END
	}

	// Terrain AO
	const bool bTerrainAO = CRenderer::CV_r_TerrainAO && rd->m_RP.m_TerrainAONodes[nThreadID].Count(); 
	if ( bTerrainAO )
	{
		PROFILE_FRAME(FX_ProcessTerrainAO);

		// make box for stencil passes
		static t_arrDeferredMeshIndBuff arrDeferredInds;
		static t_arrDeferredMeshVertBuff arrDeferredVerts;
		arrDeferredInds.resize(0);
		arrDeferredVerts.resize(0);
		rd->CreateDeferredUnitBox(arrDeferredInds, arrDeferredVerts);

		// render terrain AO
		rd->FX_PrepareTerrainAOTarget(arrDeferredInds, arrDeferredVerts);
	}

  // apply lighting irradiance volumes
  if(bRenderIrradianceVolumes && (CRenderer::CV_r_irradiancevolumes == 1 || CRenderer::CV_r_deferredshading != 3))
  {
    IrrVolumes.Render(m_pDepthRT, m_pNormalsRT);
    m_nLightsProcessedCount++;
  }

	SpecularAccEnableMRT( true );

	if( CRenderer::CV_r_deferredshadingdepthboundstest )
		rd->SetDepthBoundTest(0.0f, 1.0f, false);

	// apply deferred reflective cubemaps 
	DeferredCubemaps(rDeferredCubemaps);

  // apply deferred light sources
  //std::sort(rDeferredLights.begin(), rDeferredLights.end(), LightsCompare());
	DeferredLights(rDeferredLights);

  //TF - multi-threading
  //apply deferred decals
  //if(SRendItem::m_RecurseLevel[rd->m_RP.m_nFillThreadID]>0)
  TArray<SDeferrredDecal>& deferredDecals = rd->m_RP.m_DeferrredDecals[nThreadID][nRecurseLevel];
  for( uint nCurDecal = 0; nCurDecal < deferredDecals.size(); nCurDecal++)
  {
    DeferredDecalPass(deferredDecals[nCurDecal]);
  }
  deferredDecals.SetNum(0);

	if( CRenderer::CV_r_deferredshadingscissor )
		rd->EF_Scissor(false, 0, 0, 0, 0);

	if( CRenderer::CV_r_deferredshadingdepthboundstest )
		rd->SetDepthBoundTest(0.0f, 1.0f, false);

  rd->m_RP.m_FlagsShader_RT = nFlagsShaderRT;

	// Commit any potential render target changes
	rd->FX_SetActiveRenderTargets(false);




  rd->FX_PopRenderTarget(0);




#if !defined(XENON_FORCE_720P)
	rd->FX_PopRenderTarget(1);
#endif

  rd->RT_SetViewport(iTempX, iTempY, iWidth, iHeight); 

	if(CRenderer::CV_r_SSGI != 0)	// apply SSGI directly
	{
		ComputeSSGI(m_pDiffuseAccRT, IrrVolumes.IsGIRenderable());
	}

  if( CRenderer::CV_r_deferredshadingdebug == 3 )
  {
    rd->Set2DMode(true, 1, 1);   
    if( CRenderer::CV_r_deferredshading != 0)
      GetUtils().StretchRect(m_pDiffuseAccRT, m_pDebugRT);
    rd->Set2DMode(false, 1, 1);  
  }

	if( CRenderer::CV_r_deferredshading == 3)
	{
		//DeferredScenePass();

		// apply irradiance volumes in debug
		if(CRenderer::CV_r_irradiancevolumes > 1)
		{
			if(IrrVolumes.IsGIRenderable())
			{
				IrrVolumes.RenderGI(m_pDepthRT, m_pNormalsRT);
				m_nLightsProcessedCount++;
			}
			if(bRenderIrradianceVolumes)
			{
				IrrVolumes.Render(m_pDepthRT, m_pNormalsRT);
				m_nLightsProcessedCount++;
			}
		}
	}

	rd->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);





  rd->m_RP.m_FlagsShader_RT = nFlagsShaderRT;

  rd->FX_ResetPipe(); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::Release()
{
  DestroyDeferredMaps();   
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::Debug()
{
  uint64 nFlagsShaderRT = gcpRendD3D->m_RP.m_FlagsShader_RT;

  if( CRenderer::CV_r_deferredshadingdebug == 3)
  {
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pDebugTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    gcpRendD3D->EF_SetState( GS_NODEPTHTEST );   
    SD3DPostEffectsUtils::SetTexture( m_pDebugRT, 0, FILTER_POINT, 0);    
    SD3DPostEffectsUtils::SetTexture( CTexture::s_ptexPaletteDebug, 1, FILTER_LINEAR, 1);        
    SD3DPostEffectsUtils::DrawScreenQuadWPOS(0, 0, m_pDebugRT->GetWidth(), m_pDebugRT->GetHeight(), 0,                                          
      m_pDebugRT->GetWidth(), m_pDebugRT->GetHeight());
    SD3DPostEffectsUtils::ShEndPass();
  }

  gcpRendD3D->m_RP.m_FlagsShader_RT = nFlagsShaderRT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CRenderer::EF_GetDeferredLightsNum(const eDeferredLightType eLightType /*= eDLT_DeferredLight*/)
{
  return CDeferredShading::Instance().GetLightsNum(eLightType);
}

TArray<CDLight>* CRenderer::EF_GetDeferredLights(const eDeferredLightType eLightType /*= eDLT_DeferredLight*/)
{
  uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;
  int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID];

  return &CDeferredShading::Instance().GetLights(nThreadID,nRecurseLevel, eLightType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

struct SSGI
{
	static const int numLevels = 3;

	void EnableFadeIn(float a, float b);

	void Work(CTexture * pTarget);

			SSGI();

private:
	void InitData();
	void ReleaseData();

	void ProcessData();
	void ComputeSSGI();

	void SetupContext();
	void RestoreContext();

private:

// Shader names
	static CCryName
			s_ssgiDownsampleOffset0, s_ssgiDownsampleOffset1, s_ssgiWorldToView
			, s_ssgiDownsampleNormal, s_ssgiDownsampleIrradiance, s_ssgiDownsampleDepth
			, s_ssgiCompute, s_ssgiParams, s_ssgiFadeIn;

// Global information
	int m_fullWidth, m_fullHeight;

// Input textures:
	CTexture *m_pIllumFull, *m_pAlbedoFull, *m_pNormalsFull, *m_pDepthFull;

// Output textures:
	CTexture *m_pSSGIOutput, *m_pSSGIRaw;

// Intermediate Data:
	CTexture *m_ppNormal[numLevels], *m_ppIrradiance[numLevels], *m_ppDepth[numLevels];

// Working resolution textures
	CTexture *m_pDepthWork, *m_pNormalWork;

	// Dynamic textures
	static const int numDynTextures = numLevels * 3 + 1;
	SDynTexture	*m_ppDynTex[numDynTextures];

	// Render context
	int	m_prevViewport[4];
	int m_prevFlagsShader_RT;

	bool fadeInEnabled;
	float fadeInCoef[2];
};

CCryName SSGI::s_ssgiDownsampleOffset0("SSGI_DownsampleOffset0");
CCryName SSGI::s_ssgiDownsampleOffset1("SSGI_DownsampleOffset1");
CCryName SSGI::s_ssgiWorldToView			("SSGI_WorldToView");
CCryName SSGI::s_ssgiDownsampleNormal	("SSGI_DownsampleNormal");
CCryName SSGI::s_ssgiDownsampleIrradiance("SSGI_DownsampleIrradiance");
CCryName SSGI::s_ssgiDownsampleDepth	("SSGI_DownsampleDepth");
CCryName SSGI::s_ssgiCompute					("SSGI_Compute");
CCryName SSGI::s_ssgiParams						("SSGI_params");
CCryName SSGI::s_ssgiFadeIn						("SSGI_fadeIn");

SSGI::SSGI()
{
	fadeInEnabled = false;
}

void SSGI::EnableFadeIn(float a, float b)
{
	fadeInEnabled = true;
	fadeInCoef[0] = a; fadeInCoef[1] = b;
}

void SSGI::InitData()
{
  m_pIllumFull = CTexture::s_ptexCurrentSceneDiffuseAccMap;
	m_pAlbedoFull		= CTexture::s_ptexSceneTexturesMap;
	m_pNormalsFull	= CTexture::s_ptexSceneNormalsMap;
	m_pDepthFull		= CTexture::s_ptexZTarget;
 
	m_fullWidth		= m_pIllumFull->GetWidth();
	m_fullHeight	= m_pIllumFull->GetHeight();

	// Generate secondary textures
	int idx = 0;
	int width, height;

	// Normals
	ETEX_Format normalFmt = eTF_A8R8G8B8;

	width = m_fullWidth; height = m_fullHeight;
	for (int i = 0; i < numLevels; ++i)
	{
		width >>= 1; height >>= 1;
		m_ppDynTex[idx] = new SDynTexture(width, height, normalFmt, eTT_2D, FT_STATE_CLAMP, "ssgiNormal", 95);
		m_ppDynTex[idx]->Update(width, height);
		m_ppNormal[i]		= m_ppDynTex[idx++]->m_pTexture;
	}

	// Irradiance
	ETEX_Format irradFmt = eTF_A16B16G16R16F;

	width = m_fullWidth; height = m_fullHeight;
	for (int i = 0; i < numLevels; ++i)
	{
		width >>= 1; height >>= 1;
		m_ppDynTex[idx] = new SDynTexture(width, height, irradFmt, eTT_2D, FT_STATE_CLAMP, "ssgiIrrad", 95);
		m_ppDynTex[idx]->Update(width, height);
		m_ppIrradiance[i] = m_ppDynTex[idx++]->m_pTexture;
	}

	// Depth
	ETEX_Format depthFmt = eTF_R16F;

	width = m_fullWidth; height = m_fullHeight;
	for (int i = 0; i < numLevels; ++i)
	{
		width >>= 1; height >>= 1;
		m_ppDynTex[idx] = new SDynTexture(width, height, depthFmt, eTT_2D, FT_STATE_CLAMP, "ssgiDepth", 95);
		m_ppDynTex[idx]->Update(width, height);
		m_ppDepth[i]		= m_ppDynTex[idx++]->m_pTexture;
	}

	int halfWidth = m_fullWidth >> 1, halfHeight = m_fullHeight >> 1;
	int quadWidth = halfWidth >> 1,   quadHeight = halfHeight >> 1;

	// Raw output
	ETEX_Format ssgiFmt = m_pSSGIOutput->GetDstFormat();
	m_ppDynTex[idx] = new SDynTexture(halfWidth, halfHeight, ssgiFmt, eTT_2D, FT_STATE_CLAMP, "ssgiRaw", 95);
	m_ppDynTex[idx]->Update(halfWidth, halfHeight);
	m_pSSGIRaw			= m_ppDynTex[idx++]->m_pTexture;

	assert(numDynTextures == idx);

	// Debugging
	{
		CTexture::s_ptexSSGITarget->Invalidate(halfWidth, halfHeight, ssgiFmt);
		m_pSSGIRaw = CTexture::s_ptexSSGITarget;
	}

	// Setup the working resolution textures
	m_pDepthWork = m_ppDepth[0];
	m_pNormalWork = m_ppNormal[0];
}

void SSGI::ReleaseData()
{
	for (int i = 0; i < numDynTextures; ++i)
		SAFE_RELEASE(m_ppDynTex[i]);
}

void SSGI::ProcessData()
{
	CShader * pSH = CShaderMan::m_ShaderScreenSpaceGI;
	SRenderPipeline & RP = gcpRendD3D->m_RP;

	// Downsample (illum + albedo) into sub levels
	// Output:	irradiance[2] 
	PROFILE_LABEL_PUSH( "DOWNSAMPLE_ILLUM_ALBEDO" );
	{
		int destWidth, destHeight;

		int srcWidth, srcHeight;
		srcWidth = m_fullWidth; srcHeight = m_fullHeight;

		// Downsample light accumulation + albedo -> m_ppIrradiance[0] (half res)
		destWidth = m_ppIrradiance[0]->GetWidth(); destHeight = m_ppIrradiance[0]->GetHeight();

		gcpRendD3D->FX_PushRenderTarget(0, m_ppIrradiance[0], NULL);
		gcpRendD3D->RT_SetViewport(0, 0, destWidth, destHeight);

		SD3DPostEffectsUtils::ShBeginPass(pSH, s_ssgiDownsampleIrradiance, FEF_DONTSETSTATES);
		SD3DPostEffectsUtils::SetTexture(m_pAlbedoFull, 0, FILTER_LINEAR); 
		SD3DPostEffectsUtils::SetTexture(m_pIllumFull,  1, FILTER_LINEAR); 

		float s1 = 0.5f / srcWidth;
		float t1 = 0.5f / srcHeight;       

		Vec4 pParams0(s1*0.95f, t1*0.25f, -s1*0.25f, t1*0.96f); 
		Vec4 pParams1(-s1*0.96f, -t1*0.25f, s1*0.25f, -t1*0.96f);  

		pSH->FXSetPSFloat(s_ssgiDownsampleOffset0, &pParams0, 1);        
		pSH->FXSetPSFloat(s_ssgiDownsampleOffset1, &pParams1, 1); 

		// Draw call
		SD3DPostEffectsUtils::DrawFullScreenQuad(destWidth, destHeight);
		SD3DPostEffectsUtils::ShEndPass();

		gcpRendD3D->FX_PopRenderTarget(0);

		// Downsample the rest of the textures
		for (int i = 0; i < numLevels - 1; ++i)
			GetUtils().StretchRect(m_ppIrradiance[i], m_ppIrradiance[i+1]);
	}
	PROFILE_LABEL_POP( "DOWNSAMPLE_ILLUM_ALBEDO" );

	// Downsample normals and transform to screen space (opt. SH)
	// Req:			World to Screen matrix, Dynamic textures
	// Output:	normal[2]
	PROFILE_LABEL_PUSH( "DOWNSAMPLE_NORMAL" );
	{
		int destWidth, destHeight;

		int srcWidth, srcHeight;
		srcWidth = m_fullWidth; srcHeight = m_fullHeight;

		// Downsample normal and transform to screen space -> m_ppNormal[0] (half res)
		destWidth = m_ppNormal[0]->GetWidth(); destHeight = m_ppNormal[0]->GetHeight();

		gcpRendD3D->FX_PushRenderTarget(0, m_ppNormal[0], NULL);
		gcpRendD3D->RT_SetViewport(0, 0, destWidth, destHeight);

		SD3DPostEffectsUtils::ShBeginPass(pSH, s_ssgiDownsampleNormal, FEF_DONTSETSTATES);
		SD3DPostEffectsUtils::SetTexture(m_pNormalsFull, 0, FILTER_LINEAR); 

		float s1 = 0.5f / srcWidth;
		float t1 = 0.5f / srcHeight;       

		Vec4 pParams0(s1*0.95f, t1*0.25f, -s1*0.25f, t1*0.96f); 
		Vec4 pParams1(-s1*0.96f, -t1*0.25f, s1*0.25f, -t1*0.96f);  

		pSH->FXSetPSFloat(s_ssgiDownsampleOffset0, &pParams0, 1);        
		pSH->FXSetPSFloat(s_ssgiDownsampleOffset1, &pParams1, 1); 

		// Setup world to camera space matrix
		Matrix44A matView;
		matView = RP.m_TI[RP.m_nProcessThreadID].m_cam.GetViewMatrix();

		// Adjust the camera matrix so that the camera space will be:
		// +y = down, +z - towards, +x - right
		Vec3 zAxis = matView.GetRow(1);
		matView.SetRow(1, -matView.GetRow(2));
		matView.SetRow(2, zAxis);

		float z = matView.m13;
		matView.m13 = -matView.m23;
		matView.m23 = z;

		pSH->FXSetPSFloat(s_ssgiWorldToView, (Vec4*)matView.GetData(), 4);

		// Draw call
		SD3DPostEffectsUtils::DrawFullScreenQuad(destWidth, destHeight);
		SD3DPostEffectsUtils::ShEndPass();

		gcpRendD3D->FX_PopRenderTarget(0);

		// Downsample the rest of the textures
		for (int i = 0; i < numLevels - 1; ++i)
			GetUtils().StretchRect(m_ppNormal[i], m_ppNormal[i+1]);
	}
	PROFILE_LABEL_POP( "DOWNSAMPLE_NORMAL" );

	// Downsample depth
	// Output:	depth[2]
	PROFILE_LABEL_PUSH( "DOWNSAMPLE_DEPTH" );
	{
		GetUtils().StretchRect(m_pDepthFull, m_ppDepth[0]);

		// Downsample the rest of the textures
		for (int i = 0; i < numLevels - 1; ++i)
			GetUtils().StretchRect(m_ppDepth[i], m_ppDepth[i+1]);
	}
	PROFILE_LABEL_POP( "DOWNSAMPLE_DEPTH" );
}

void SSGI::ComputeSSGI()
{
	CShader * pSH = CShaderMan::m_ShaderScreenSpaceGI;

	// Compute SSGI (which resolution?)
	// Input:		irradiance, normal, depth
	// Output:	pTempSSGI
	PROFILE_LABEL_PUSH( "COMPUTE_SSGI" );
	{
		int width, height;
		width = m_pSSGIRaw->GetWidth(); height = m_pSSGIRaw->GetHeight();

		gcpRendD3D->FX_PushRenderTarget(0, m_pSSGIRaw, NULL);
		gcpRendD3D->RT_SetViewport(0, 0, width, height);

		// Clear the destination
		ColorF clearColor(0, 0, 0, 0);
		gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clearColor);

		// The results will be iteratively blended
		gcpRendD3D->SetState(GS_BLDST_ONE | GS_BLSRC_ONE);

		if (fadeInEnabled)
			gcpRendD3D->m_RP.m_FlagsShader_RT |=  g_HWSR_MaskBit[HWSR_SAMPLE0];
		else
			gcpRendD3D->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];

		// Start the technique
		SD3DPostEffectsUtils::ShBeginPass(pSH, s_ssgiCompute, FEF_DONTSETSTATES|FEF_DONTSETTEXTURES);

		static int startPass = 0;
		static int endPass = numLevels - 1;
		for (int i = startPass; i <= endPass; ++i)
		{
			// Apply textures
			SD3DPostEffectsUtils::SetTexture(m_ppNormal[i],			0, FILTER_LINEAR, 1);
			SD3DPostEffectsUtils::SetTexture(m_ppIrradiance[i], 1, FILTER_LINEAR, TADDR_BORDER);
			SD3DPostEffectsUtils::SetTexture(m_ppDepth[i],			2, FILTER_LINEAR, 1);

			SD3DPostEffectsUtils::SetTexture(m_pDepthWork,			3, FILTER_POINT);
			SD3DPostEffectsUtils::SetTexture(m_pNormalWork,			4, FILTER_POINT);

			if (fadeInEnabled)
			{
				Vec4 fadeInVec(fadeInCoef[0], fadeInCoef[1], 0, 0);
				SD3DPostEffectsUtils::ShSetParamPS(s_ssgiFadeIn, fadeInVec);
				}

			// Apply constants
			Vec4 ssgiParams(CRenderer::CV_r_SSGI_amount, CRenderer::CV_r_SSGI_radius, float(i), 0);
			SD3DPostEffectsUtils::ShSetParamPS(s_ssgiParams, ssgiParams);

			// Render
			SD3DPostEffectsUtils::DrawFullScreenQuad(width, height);
		}
		SD3DPostEffectsUtils::ShEndPass();

		gcpRendD3D->FX_PopRenderTarget(0);
	}
	PROFILE_LABEL_POP( "COMPUTE_SSGI" );
	
	// Filter (+upsample) to pSSGITarget
	// m_pSSGIRaw -> m_pSSGIOutput
	{
		// Add the results to the destination buffer
		gcpRendD3D->SetState(GS_BLDST_ONE | GS_BLSRC_ONE);

		// Blur the target
		GetUtils().BilateralBlurring(m_pSSGIRaw, m_pSSGIOutput);
	}
}

void SSGI::SetupContext()
{
	gcpRendD3D->Set2DMode(true, 1, 1);   

	// Clear the unused RTs
	gcpRendD3D->FX_PushRenderTarget(1, NULL, NULL, false, -1);
	gcpRendD3D->FX_PushRenderTarget(2, NULL, NULL, false, -1);
	gcpRendD3D->FX_PushRenderTarget(3, NULL, NULL, false, -1);

	// Store the current view port
	gcpRendD3D->GetViewport(&m_prevViewport[0], &m_prevViewport[1], &m_prevViewport[2], &m_prevViewport[3]);

	m_prevFlagsShader_RT = gcpRendD3D->m_RP.m_FlagsShader_RT;

	// Make sure that the flags are not set
	uint64 mask = g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2];
	gcpRendD3D->m_RP.m_FlagsShader_RT = gcpRendD3D->m_RP.m_FlagsShader_RT & ~mask;

	gcpRendD3D->D3DSetCull(eCULL_None);
	gcpRendD3D->SetState(GS_NODEPTHTEST);
}

void SSGI::RestoreContext()
{
	gcpRendD3D->FX_PopRenderTarget(1);
	gcpRendD3D->FX_PopRenderTarget(2);
	gcpRendD3D->FX_PopRenderTarget(3);

	gcpRendD3D->RT_SetViewport(m_prevViewport[0], m_prevViewport[1], m_prevViewport[2], m_prevViewport[3]);

	gcpRendD3D->m_RP.m_FlagsShader_RT = m_prevFlagsShader_RT;

	gcpRendD3D->Set2DMode(false, 1, 1);   
	gcpRendD3D->FX_ResetPipe(); 
}

void SSGI::Work(CTexture * pTarget)
{
	m_pSSGIOutput = pTarget;

	SetupContext();

	InitData();

	ProcessData();

	ComputeSSGI();

	ReleaseData();

	RestoreContext();
}

void CDeferredShading::ComputeSSGI(CTexture* pSSGITarget, bool bMergeWithGI)
{
	PROFILE_LABEL_PUSH( "COMPUTE_SSGI" );

	SSGI ssgi;

	if (bMergeWithGI)
	{
		static ICVar*	pGIMaxDistance = gEnv->pConsole->GetCVar("e_GIMaxDistance");
		assert(pGIMaxDistance);
		assert(pGIMaxDistance->GetFVal() > 0.1f);

		const float d = pGIMaxDistance->GetFVal() * .5f;
		const float offset = min(d * .5f, 20.f);
		// att(d) = kd+b;
		const float k = -1.f / offset;
		const float b = d / offset;

		ssgi.EnableFadeIn(-k * m_fCamFar, 1.f - b);
	}

	ssgi.Work(pSSGITarget);

	PROFILE_LABEL_POP( "COMPUTE_SSGI" );	

	return;

	const bool blurSSGI = (CRenderer::CV_r_SSGI_blur != 0);
	const ETEX_Format ssgiFmt = pSSGITarget->GetDstFormat();

	int width, height;
	width = m_pDiffuseAccRT->GetWidth();
	height = m_pDiffuseAccRT->GetHeight();

	// Make sure the render target is allocated
	if(pSSGITarget == CTexture::s_ptexSSGITarget)
		CTexture::s_ptexSSGITarget->Invalidate(width, height, ssgiFmt);

	SDynTexture * pDynSurfSSGI = 0;
	CTexture * pSurfSSGI = pSSGITarget;

	if (blurSSGI)
	{
		// Create temporary render target is blur is required
		pDynSurfSSGI = new SDynTexture(width, height, ssgiFmt, eTT_2D,  FT_STATE_CLAMP, "tempSSGI", 95);
		pDynSurfSSGI->Update(width, height);

		pSurfSSGI = pDynSurfSSGI->m_pTexture;
	}

	gcpRendD3D->Set2DMode(true, 1, 1);   

	// Store the current view port
	int iTempX, iTempY, iWidth, iHeight;
	gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

	// Bind render target
	gcpRendD3D->FX_PushRenderTarget(0, pSurfSSGI, NULL);
	gcpRendD3D->RT_SetViewport(0, 0, width, height);

	gcpRendD3D->D3DSetCull(eCULL_Back);

	/////////////////////////////////////
	// Setup shader flags

	uint64 nFlagsShaderRT = gcpRendD3D->m_RP.m_FlagsShader_RT;

	const uint64 fullMask = (g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1]);
	uint64 mask = 0;
	SRenderPipeline & RP = gcpRendD3D->m_RP;

	if (CRenderer::CV_r_SSAO_downscale)
		mask |= g_HWSR_MaskBit[HWSR_SAMPLE0];

	if (CRenderer::CV_r_SSAO_downscale_ztarget)
		mask |= g_HWSR_MaskBit[HWSR_SAMPLE1];

	if(bMergeWithGI)
		mask |= g_HWSR_MaskBit[HWSR_SAMPLE2];

	RP.m_FlagsShader_RT = (RP.m_FlagsShader_RT & ~fullMask) | mask;

	// Set shader quality
	switch (RP.m_nShaderQuality = CRenderer::CV_r_SSGI_quality)
	{
	case eSQ_Medium:
		RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_QUALITY];
		break;
	case eSQ_High:
		RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_QUALITY1];
		RP;
	case eSQ_VeryHigh:
		RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_QUALITY];
		RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_QUALITY1];
		break;
	}

	CShader * pSH = CShaderMan::m_ShaderAmbientOcclusion;

	static CCryName techName("Deferred_SSGI_Pass");
	SD3DPostEffectsUtils::ShBeginPass(pSH, techName, FEF_DONTSETSTATES);

	// States
	if(!blurSSGI)
		gcpRendD3D->SetState(GS_NODEPTHTEST | GS_BLDST_ONE | GS_BLSRC_ONE);
	else
		gcpRendD3D->SetState(GS_NODEPTHTEST);

	/////////////////////////////////////
	// Setup constants

	// SSAO_CameraMatrix
	static CCryName paramCameraMatrix("SSAO_CameraMatrix");

	Matrix44A matView;
	matView = RP.m_TI[RP.m_nProcessThreadID].m_cam.GetViewMatrix();

	// Adjust the camera matrix so that the camera space will be:
	// +y = down, +z - towards, +x - right
	Vec3 zAxis = matView.GetRow(1);
	matView.SetRow(1, -matView.GetRow(2));
	matView.SetRow(2, zAxis);
	
	float z = matView.m13;
	matView.m13 = -matView.m23;
	matView.m23 = z;

	pSH->FXSetPSFloat(paramCameraMatrix, (Vec4*)matView.GetData(), 4);

	// SSAO_params
	static CCryName paramParams("SSAO_params");
	Vec4 vConst( CRenderer::CV_r_SSGI_amount, CRenderer::CV_r_SSGI_radius, 0, 0);
	if(bMergeWithGI)
	{
		static ICVar*	pGIMaxDistance = gEnv->pConsole->GetCVar("e_GIMaxDistance");
		assert(pGIMaxDistance);
		assert(pGIMaxDistance->GetFVal() > 0.1f);

		const float d = pGIMaxDistance->GetFVal() * .5f;
		const float offset = min(d * .5f, 20.f);
		// att(d) = kd+b;
		const float k = -1.f / offset;
		const float b = d / offset;

		vConst.z = -k * m_fCamFar;
		vConst.w = 1.f - b;
	}
	pSH->FXSetPSFloat(paramParams, &vConst, 1);

	// Render 
	SD3DPostEffectsUtils::DrawFullScreenQuad(CTexture::s_ptexSceneNormalsMap->GetWidth(), CTexture::s_ptexSceneNormalsMap->GetHeight());  
	SD3DPostEffectsUtils::ShEndPass();

	// Restore state
	gcpRendD3D->FX_PopRenderTarget(0);	
	gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

	RP.m_FlagsShader_RT = nFlagsShaderRT;
	
	if (blurSSGI)
	{
		// States
		gcpRendD3D->SetState(GS_BLDST_ONE | GS_BLSRC_ONE);

		// Blur the target
		GetUtils().BilateralBlurring(pSurfSSGI, pSSGITarget);
		SAFE_DELETE(pDynSurfSSGI);
	}

	gcpRendD3D->Set2DMode(false, 1, 1);   
	gcpRendD3D->FX_ResetPipe(); 

	PROFILE_LABEL_POP( "COMPUTE_SSGI" );	
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

int CRenderer::EF_AddDeferredLight( const CDLight &pLight, float fMult )
{
  int nLightID = -1;
  CDeferredShading &pDS = CDeferredShading::Instance();
  if(CV_r_deferredshading)
    nLightID = pDS.AddLight( pLight, fMult );

  return 
    nLightID;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CRenderer::EF_ClearDeferredLightsList()
{
	CDeferredShading::Instance().ResetLights();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CRenderer::EF_ClearDeferredVisAreasList()
{
	CDeferredShading::Instance().ResetVisAreas();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CRenderer::EF_AddDeferredVisArea( const IVisArea *pVisArea )
{
	if( !pVisArea || !CV_r_deferredshading || !CV_r_deferredshadingstencilprepass )
		return;

	CDeferredShading::Instance().AddVisArea( pVisArea );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CRenderer::CheckRenderObjVisArea( CRenderObject *pObj )
{
  if( !pObj || !CV_r_deferredshading || !CV_r_deferredshadingstencilprepass )
    return;
	
	uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;

  pObj->m_ObjFlags &= ~FOB_INVISAREA;
  if(pObj->m_pRenderNode)
  {
    IVisArea *pVisArea = ((IRenderNode*)pObj->m_pRenderNode)->GetEntityVisArea();
    if( pVisArea && CDeferredShading::Instance().GetVisAreaID(nThreadID, pVisArea) ) 
      pObj->m_ObjFlags |= FOB_INVISAREA;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CRenderer::FX_SetRenderObjDefaultVisAreaStencilRef( CRenderObject *pObj )
{
  SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
  SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

  // Some cases don't need this at all - skip
  if( rRP.m_nPassGroupID == EFSLIST_TERRAINLAYER || rRP.m_nPassGroupID == EFSLIST_DECAL )
    return;

  if( CV_r_deferredshading && CV_r_deferredshadingstencilprepass )
  {
    rTI.m_PersFlags2 |=	RBPF2_LIGHTSTENCILCULL;
    if( !(pObj->m_ObjFlags & FOB_INVISAREA) )
    {
      // Write default vis area id - this is always needed, as we don't have any depth information at all during this step
      const uint32 nVisAreaID = 0;
      static const int32 nStencilState = STENC_FUNC(FSS_STENCFUNC_ALWAYS) |STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP) | STENCOP_PASS(FSS_STENCOP_REPLACE);
      EF_SetStencilState( nStencilState, nVisAreaID, 0xFF, 0xFF);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CRenderer::FX_SetRenderObjVisAreaStencilRef( CRenderObject *pObj )
{
  SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
  SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

  // Some cases don't need this at all - skip
  if( rRP.m_nPassGroupID == EFSLIST_TERRAINLAYER || rRP.m_nPassGroupID == EFSLIST_DECAL )
    return;

	uint32 nThreadID = gcpRendD3D->m_RP.m_nProcessThreadID;

  if( (rRP.m_nBatchFilter & FB_Z) && !(rTI.m_PersFlags & RBPF_SHADOWGEN) && CV_r_deferredshading && CV_r_deferredshadingstencilprepass)
  {
    // For geometry inside vis areas, we need per-instance vis area ID. Todo: pass just vis area ID
    if( (pObj->m_ObjFlags & FOB_INVISAREA) )
    {
      IRenderNode *pRN = (IRenderNode *)pObj->m_pRenderNode;
      if( pRN )
      {
				if( IVisArea *pVisArea = pRN->GetEntityVisArea() )
				{
					if( uint32 nVisAreaID = CDeferredShading::Instance().GetVisAreaID( nThreadID, pVisArea ) )
					{
						// Write vis area stencil id
						static const int32 nStencilState = STENC_FUNC(FSS_STENCFUNC_ALWAYS) |STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP) | STENCOP_PASS(FSS_STENCOP_REPLACE);
						EF_SetStencilState( nStencilState, nVisAreaID, 0xFF, 0xFF);
					}
				}
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CD3D9Renderer::FX_DeferredRendering( bool bDebugPass, bool bUpdateRTOnly ) 
{
  CDeferredShading &pDS = CDeferredShading::Instance();

  if( !CRenderer::CV_r_deferredshading || !CTexture::s_ptexSceneTarget )
  {
    pDS.Release();
    return false;
  }

  if( bUpdateRTOnly )
  {
    pDS.CreateDeferredMaps();
#if defined(PS3) && defined( PS3_CLEAR_DEBUG )
    const ColorF Black(0.0f, 0.0f, 0.0f, 0.0f);
    gcpRendD3D->m_pd3dDevice->ClearTexture((CCryDXPSTexture2D *)CTexture::s_ptexSceneTarget->GetDeviceTexture(), 0);
    void *test = ((CCryDXPSTexture2D *)CTexture::s_ptexSceneTarget->GetDeviceTexture())->RawPointer();
#endif

    return true;
  }

  if( !bDebugPass)
    pDS.Render();
  else
    pDS.Debug();

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CREDeferredShading::mfDraw(CShader *ef, SShaderPass *sfm)
{
  gcpRendD3D->FX_DeferredRendering();
  return true;
}

