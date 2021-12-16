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

//#pragma optimize("", off)

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

const char * NPostEffects::CRainDrops::GetName() const
{
	return "RainDrops";
}

bool CRainDrops::Preprocess()
{  
  bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality( eRQ_Medium, eSQ_Medium );
  if( !bQualityCheck )
    return false;

  static float fLastSpawnTime = 0.0f;

  if( m_pAmount->GetParam() > 0.09f || m_nAliveDrops)
  {
    fLastSpawnTime = 0.0f;
    return true;
  }

  if( fLastSpawnTime == 0.0f)
  {
    fLastSpawnTime = GetUtils().m_pTimer->GetCurrTime();
  }

  if( fabs(GetUtils().m_pTimer->GetCurrTime() - fLastSpawnTime) < 1.0f ) 
  {
    return true;
  }  

  m_pPrevView = GetUtils().m_pView;    

  SAFE_DELETE( m_pRainDrops );

  return false;
}

void CRainDrops::SpawnParticle( SRainDrop *&pParticle )
{
  static SRainDrop pNewDrop;

  static float fLastSpawnTime = 0.0f;

  float fUserSize = m_pSize->GetParam();
  float fUserSizeVar = m_pSizeVar->GetParam();

  if( Random() > 0.5f && fabs(GetUtils().m_pTimer->GetCurrTime() - fLastSpawnTime) > m_pSpawnTimeDistance->GetParam() )
  {
    pParticle->m_pPos.x = Random();
    pParticle->m_pPos.y = Random();

    pParticle->m_fLifeTime = (pNewDrop.m_fLifeTime + pNewDrop.m_fLifeTimeVar *(Random()*2.0f-1.0f) ) ;
    //pParticle->m_fSize = (pNewDrop.m_fSize + pNewDrop.m_fSizeVar * (Random()*2.0f-1.0f)) * 2.5f;    
    //pNewDrop.m_fSize + pNewDrop.m_fSizeVar
    pParticle->m_fSize = 1.0f /( 10.0f *(fUserSize  + 0.5f * (fUserSizeVar) * (Random()*2.0f-1.0f)) ); 

    pParticle->m_pPos.x -= pParticle->m_fSize  / (float)m_pRainDrops->m_pTexture->GetWidth();
    pParticle->m_pPos.y -= pParticle->m_fSize  / (float)m_pRainDrops->m_pTexture->GetHeight();

    pParticle->m_fSpawnTime = GetUtils().m_pTimer->GetCurrTime();  
    pParticle->m_fWeight = 0.0f; // default weight to force rain drop to be stopped for a while
    
    fLastSpawnTime = GetUtils().m_pTimer->GetCurrTime();
  }
  else
  {
    pParticle->m_fSize = 0.0f;
  }

}

void CRainDrops::UpdateParticles()
{
  SRainDropsItor pItor, pItorEnd = m_pDropsLst.end(); 

  // Store camera parameters
  Vec3 vz = gcpRendD3D->GetRCamera().Z;    // front vec
  float fDot = vz.Dot(Vec3(0, 0, -1.0f));
  float fGravity = 1.0f - fabs( fDot );

  float fCurrFrameTime = 10.0f * gEnv->pTimer->GetFrameTime();

  bool bAllowSpawn( m_pAmount->GetParam() > 0.005f );

  m_nAliveDrops = 0;

  for(pItor=m_pDropsLst.begin(); pItor!=pItorEnd; ++pItor )
  {
    SRainDrop *pCurr = (*pItor);

    float fCurrLifeTime = (GetUtils().m_pTimer->GetCurrTime() - pCurr->m_fSpawnTime) / pCurr->m_fLifeTime;

    //if( fDot < - 0.5 )
    {
      //pCurr->m_fLifeTime = -1.0f;
      //continue;
    }

    // particle died, spawn new 
    if( fabs(fCurrLifeTime) > 1.0f || pCurr->m_fSize < 0.01f)
    {
      if( bAllowSpawn )
      {
        SpawnParticle( pCurr );
      }
      else
      {
        pCurr->m_fSize = 0.0f; 
        continue;
      }
    }

    m_nAliveDrops++;

    // update position, etc

    // add gravity
    pCurr->m_pPos.y += m_pVelocityProj.y * ((Random() *2.0f - 1.0f)*0.6f+0.4f); 
    pCurr->m_pPos.y += fCurrFrameTime * fGravity * min( pCurr->m_fWeight, 0.5f * pCurr->m_fSize);
    // random horizontal movement and size + camera horizontal velocity    
    pCurr->m_pPos.x += m_pVelocityProj.x * ((Random() *2.0f - 1.0f)*0.6f+0.4f); 
    pCurr->m_pPos.x += fCurrFrameTime * (min(pCurr->m_fWeight, 0.25f * pCurr->m_fSize) * fGravity * ( (Random() *2.0f - 1.0f)));  

    // Increase/decrease weight randomly 
    pCurr->m_fWeight = clamp_tpl<float>(pCurr->m_fWeight + fCurrFrameTime * pCurr->m_fWeightVar * (Random()*2.0f-1.0f)*4.0f, 0.0f, 1.0f );                    
  }
}

void CRainDrops::RainDropsMapGen()
{  
  // Generate rain drops displacement texture

  // Store camera parameters
  Vec3 vx = gcpRendD3D->GetRCamera().X;    // up vec
  Vec3 vy = gcpRendD3D->GetRCamera().Y;    // right vec
  Vec3 vz = gcpRendD3D->GetRCamera().Z;    // front vec

  // Get current viewport
  float fScreenW = m_pRainDrops->m_pTexture->GetWidth();
  float fScreenH = m_pRainDrops->m_pTexture->GetHeight();

  float fInvScreenW = 1.0f / fScreenW;
  float fInvScreenH = 1.0f / fScreenH;

  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  // Store camera parameters
  float fDot = vz.Dot(Vec3(0, 0, -1.0f));
  float fGravity = (1.0f - fabs( fDot ));  

  gcpRendD3D->Set2DMode(false, 1, 1);       
  gcpRendD3D->Set2DMode(true, (int)fScreenW, (int)fScreenH);       

  static CCryName pParam0Name("vRainParams");

  //if( fDot >= - 0.25)
  {

    // render particles into effects rain map
    gcpRendD3D->FX_PushRenderTarget(0, m_pRainDrops->m_pTexture, GetUtils().m_pCurDepthSurface); 
    gcpRendD3D->RT_SetViewport(0, 0, (int)fScreenW, (int)fScreenH);        

    static CCryName pTech0Name("RainDropsGen");

    GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTech0Name, FEF_DONTSETSTATES);   

    gcpRendD3D->EF_SetState(GS_BLSRC_ONE | GS_BLDST_ONE |  GS_NODEPTHTEST); //

    // override blend operation
#if defined (DIRECT3D9) || defined (OPENGL)
    gcpRendD3D->m_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MAX);
#elif defined (DIRECT3D10)
    SStateBlend bl = gcpRendD3D->m_StatesBL[gcpRendD3D->m_nCurStateBL];
    bl.Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MAX;
    bl.Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
    gcpRendD3D->SetBlendState(&bl);
#endif

    SRainDropsItor pItor, pItorEnd = m_pDropsLst.end(); 
    for(pItor=m_pDropsLst.begin(); pItor!=pItorEnd; ++pItor )
    {
      SRainDrop *pCurr = (*pItor);

      if( pCurr->m_fSize < 0.01f) 
      {
        continue;
      }

      // render a sprite    
      float x0 = pCurr->m_pPos.x*fScreenW;
      float y0 = pCurr->m_pPos.y*fScreenH;  

      float x1 = (pCurr->m_pPos.x + pCurr->m_fSize * (fScreenH/fScreenW)) * fScreenW ;
      float y1 = (pCurr->m_pPos.y + pCurr->m_fSize) * fScreenH ;          

      float fCurrLifeTime = (GetUtils().m_pTimer->GetCurrTime() - pCurr->m_fSpawnTime) / pCurr->m_fLifeTime;
      Vec4 vRainParams = Vec4(1.0f, 1.0f, 1.0f, 1.0f - fCurrLifeTime);                  
      CShaderMan::m_shPostEffectsGame->FXSetPSFloat(pParam0Name, &vRainParams, 1);  
      GetUtils().DrawScreenQuad(256, 256, x0, y0, x1, y1);
    }

    // restore previous blend operation
#if defined (DIRECT3D9) || defined (OPENGL)
    gcpRendD3D->m_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
#elif defined (DIRECT3D10)
    bl.Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bl.Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    gcpRendD3D->SetBlendState(&bl);
#endif

    GetUtils().ShEndPass();   

    // Restore previous view port
    gcpRendD3D->FX_PopRenderTarget(0);

    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);
  }

  gcpRendD3D->Set2DMode(false, iWidth, iHeight);       
  gcpRendD3D->Set2DMode(true, 1, 1);       

  gcpRendD3D->RT_SetViewport(0, 0, m_pRainDrops->m_pTexture->GetWidth(), m_pRainDrops->m_pTexture->GetHeight());

  // apply extinction
  static CCryName pTech1Name("RainDropsExtinction");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTech1Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);    

  float fFrameScale = 4.0f * gEnv->pTimer->GetFrameTime();
  Vec4 vRainNormalMapParams = Vec4(m_pVelocityProj.x * ((float) iWidth), 0, fFrameScale * fGravity, fFrameScale*1.0f + m_pVelocityProj.y * ((float) iHeight)) * 0.25f;

  static CCryName pParam1Name("vRainNormalMapParams");
  GetUtils().ShSetParamVS(pParam1Name, vRainNormalMapParams);

  vRainNormalMapParams.w = fFrameScale;
  GetUtils().ShSetParamPS(pParam0Name, vRainNormalMapParams);

  GetUtils().SetTexture(m_pRainDrops->m_pTexture, 0, FILTER_LINEAR);     

  GetUtils().DrawFullScreenQuad(m_pRainDrops->m_pTexture->GetWidth(), m_pRainDrops->m_pTexture->GetHeight());

  GetUtils().ShEndPass();   

  // update rain drops texture
  GetUtils().CopyScreenToTexture(m_pRainDrops->m_pTexture);   

  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);
}

void CRainDrops::Render()
{
	PROFILE_LABEL_PUSH("RAIN_DROPS");

  int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  Matrix44 pCurrView( GetUtils().m_pView ), pCurrProj( GetUtils().m_pProj );    

  int nRainMapSizeW = CTexture::s_ptexBackBuffer->GetWidth()>>1;
  int nRainMapSizeH = CTexture::s_ptexBackBuffer->GetHeight()>>1;
  if( !m_pRainDrops || (m_pRainDrops->m_pTexture && (m_pRainDrops->m_pTexture->GetWidth() != nRainMapSizeW || m_pRainDrops->m_pTexture->GetHeight() != nRainMapSizeH) ) )
  {    
    SAFE_DELETE( m_pRainDrops );
    m_pRainDrops = new SDynTexture(nRainMapSizeW, nRainMapSizeW, eTF_A8R8G8B8, eTT_2D,  FT_STATE_CLAMP, "RainDropsTempRT");  
    m_pRainDrops->Update( nRainMapSizeW, nRainMapSizeH );
    ColorF c = ColorF(0.0, 0.0, 0.0, 0.0);
    m_pRainDrops->m_pTexture->Fill(c);
  }

  if( !m_pRainDrops || !m_pRainDrops->m_pTexture )
  {        
    return;
  }

  Matrix33 pLerpedView;
  pLerpedView.SetIdentity(); 

  float fCurrFrameTime = gEnv->pTimer->GetFrameTime();
  // scale down speed a bit
  float fAlpha = iszero (fCurrFrameTime) ? 0.0f : .0005f/( fCurrFrameTime);            

  // Interpolate matrixes and position
  pLerpedView = Matrix33(pCurrView)*(1-fAlpha) + Matrix33(m_pPrevView)*fAlpha;   

  Vec3 pLerpedPos = Vec3::CreateLerp(pCurrView.GetRow(3), m_pPrevView.GetRow(3), fAlpha);     

  // Compose final 'previous' viewProjection matrix
  Matrix44 pLView = pLerpedView;
  pLView.m30 = pLerpedPos.x;   
  pLView.m31 = pLerpedPos.y;
  pLView.m32 = pLerpedPos.z; 

  m_pViewProjPrev = pLView * pCurrProj;      
  m_pViewProjPrev.Transpose();

  // Compute camera velocity vector
  Vec3 vz = gcpRendD3D->GetRCamera().Z;    // front vec
  Vec3 vMoveDir = gcpRendD3D->GetRCamera().Orig - vz;
  Vec4 vCurrPos = Vec4(vMoveDir.x, vMoveDir.y, vMoveDir.z, 1.0f);

  Matrix44 pViewProjCurr ( GetUtils().m_pViewProj );

  Vec4 pProjCurrPos = pViewProjCurr * vCurrPos;

  pProjCurrPos.x = ( (pProjCurrPos.x + pProjCurrPos.w) * 0.5f + (1.0f / (float) iWidth) * pProjCurrPos.w) / pProjCurrPos.w;
  pProjCurrPos.y = ( (pProjCurrPos.w - pProjCurrPos.y) * 0.5f + (1.0f / (float) iHeight) * pProjCurrPos.w) / pProjCurrPos.w;

  Vec4 pProjPrevPos = m_pViewProjPrev * vCurrPos;

  pProjPrevPos.x = ( (pProjPrevPos.x + pProjPrevPos.w) * 0.5f + (1.0f / (float) iWidth) * pProjPrevPos.w) / pProjPrevPos.w;
  pProjPrevPos.y = ( (pProjPrevPos.w - pProjPrevPos.y) * 0.5f + (1.0f / (float) iHeight) * pProjPrevPos.w) / pProjPrevPos.w;

  m_pVelocityProj = Vec3(pProjCurrPos.x - pProjPrevPos.x, pProjCurrPos.y - pProjPrevPos.y, 0);

  UpdateParticles();
  RainDropsMapGen();

  // make sure to update dynamic texture if required
  if( m_pRainDrops && !m_pRainDrops->m_pTexture )
  {
    m_pRainDrops->Update( nRainMapSizeW, nRainMapSizeH );  
  }

  // display rain
  static CCryName pTechName("RainDropsFinal");
  GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);   

  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);    

  static CCryName pParam0Name("vRainNormalMapParams");
  GetUtils().ShSetParamVS(pParam0Name, Vec4(1.0f, 1.0f, 1.0f, -1.0f));
  static CCryName pParam1Name("vRainParams");
  GetUtils().ShSetParamPS(pParam1Name, Vec4(1.0f, 1.0f, 1.0f, 1.0f));

  GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_LINEAR);     
  GetUtils().SetTexture(m_pRainDrops->m_pTexture, 1, FILTER_LINEAR);     

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

  GetUtils().ShEndPass();   

  // store previous frame data
  m_pPrevView =  pCurrView;

	PROFILE_LABEL_POP("RAIN_DROPS");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDistantRain::Render()
{
  PROFILE_SHADER_START
	PROFILE_LABEL_PUSH("DISTANT_RAIN");

    // Get current viewport
    int iTempX, iTempY, iWidth, iHeight;
  gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

  gcpRendD3D->Set2DMode(false, 1, 1);       

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Render distant-rain into low-res render target for less fillrate hit

  gcpRendD3D->FX_PushRenderTarget(0,  CTexture::s_ptexBackBufferScaled[0],  GetUtils().m_pCurDepthSurface); 
  gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexBackBufferScaled[0]->GetWidth(), CTexture::s_ptexBackBufferScaled[0]->GetHeight());        

  ColorF clearColor(0, 0, 0, 0);
  gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clearColor);

  float fAmount = m_pAmount->GetParam();
  float fSpeed = m_pSpeed->GetParam();
  float fDistance = m_pDistanceScale->GetParam();

  static CCryName pTechName("DistantRain");
  static CCryName pParam0Name("PI_RainParamsVS");
  static CCryName pParam1Name("PI_RainParamsPS");
  CCryName pParam2Name("cRainColor");

  Vec4 pColor = m_pColor->GetParamVec4();

  //  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName, FEF_DONTSETSTATES);   
  uint32 nPasses;
  CShaderMan::m_shPostEffects->FXSetTechnique(pTechName);
  CShaderMan::m_shPostEffects->FXBegin(&nPasses, FEF_DONTSETSTATES);

  gcpRendD3D->SetCullMode( R_CULL_NONE );
  gcpRendD3D->EF_SetState(GS_BLSRC_ONE | GS_BLDST_ONE |GS_NODEPTHTEST);     

  int nSlicesCount = 12;   

  // Get sample size ratio (based on empirical "best look" approach)
  float fSampleSize = ((float)CTexture::s_ptexBackBuffer->GetWidth()/(float)CTexture::s_ptexBackBufferScaled[0]->GetWidth()) * 0.5f;

  // Set samples position
  float s1 = fSampleSize / (float) CTexture::s_ptexBackBuffer->GetWidth();  // 2.0 better results on lower res images resizing        
  float t1 = fSampleSize / (float) CTexture::s_ptexBackBuffer->GetHeight();       
  // Use rotated grid
  Vec4 pParams0=Vec4(s1*0.95f, t1*0.25f, -s1*0.25f, t1*0.96f); 
  Vec4 pParams1=Vec4(-s1*0.96f, -t1*0.25f, s1*0.25f, -t1*0.96f);  

  CCryName pParam3Name("texToTexParams0");
  CCryName pParam4Name("texToTexParams1");

  for( int r(0); r < nSlicesCount; ++r)   
  {

    // !force updating constants per-pass! (dx10..)
    CShaderMan::m_shPostEffects->FXBeginPass(0);

    CShaderMan::m_shPostEffects->FXSetPSFloat(pParam3Name, &pParams0, 1);        
    CShaderMan::m_shPostEffects->FXSetPSFloat(pParam4Name, &pParams1, 1); 
    CShaderMan::m_shPostEffects->FXSetPSFloat(pParam2Name, &pColor, 1); 

    // Set PS default params  
    Vec4 pParams= Vec4(fSpeed, fAmount, r, 1.0f / (float) nSlicesCount);
    pParams.y = fDistance;
    CShaderMan::m_shPostEffects->FXSetVSFloat(pParam0Name, &pParams, 1);
    pParams.y = fAmount;
    CShaderMan::m_shPostEffects->FXSetPSFloat(pParam1Name, &pParams, 1);

    GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 
    CShaderMan::m_shPostEffects->FXEndPass();
  }

  CShaderMan::m_shPostEffects->FXEnd(); 
  //  GetUtils().ShEndPass(); 

  // Restore previous viewport
  gcpRendD3D->FX_PopRenderTarget(0);
  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);     

  gcpRendD3D->Set2DMode(true, 1, 1);       
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Display distant rain effect

  CCryName pTechName0("DistantRainFinal");

  GetUtils().ShBeginPass(CShaderMan::m_shPostEffects, pTechName0, FEF_DONTSETSTATES);   
  gcpRendD3D->EF_SetState(GS_NODEPTHTEST);

  GetUtils().DrawFullScreenQuad(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight()); 
  GetUtils().ShEndPass(); 

  gcpRendD3D->FX_Flush(); 

	PROFILE_LABEL_POP("DISTANT_RAIN");

  PROFILE_SHADER_END
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CSceneRain::CreateBuffers( uint16 nVerts, void *&pINVB, SVF_P3F_C4B_T2F *pVtxList, uint16 nInds, void *&pINIB, uint16 *pIndsList)
{
  D3DVertexBuffer *pVB = 0;
  D3DIndexBuffer *pIB = 0;

  HRESULT hr = S_OK;
  D3DDevice *dv = gcpRendD3D->GetDevice();

  // Create VB/IB

#if defined (DIRECT3D9) || defined(OPENGL)
  hr = dv->CreateVertexBuffer( nVerts * sizeof( SVF_P3F_C4B_T2F ), D3DUSAGE_WRITEONLY,
    0, D3DPOOL_DEFAULT, &pVB, NULL );
  assert(SUCCEEDED(hr));
  if(FAILED(hr))
    return;

  hr = dv->CreateIndexBuffer( nInds * sizeof( uint16 ), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
    D3DPOOL_DEFAULT, &pIB, NULL );
  assert(SUCCEEDED(hr));
  if(FAILED(hr))
    return;

  SVF_P3F_C4B_T2F* pVerts = NULL;
  uint16* pInds = NULL;

  //allocate vertices
  hr = pVB->Lock(0, nVerts * sizeof( SVF_P3F_C4B_T2F ), (void **) &pVerts, 0);
  assert(SUCCEEDED(hr));

  memcpy( pVerts, pVtxList, nVerts*sizeof(SVF_P3F_C4B_T2F ) );

  hr = pVB->Unlock();
  assert(SUCCEEDED(hr));

  //allocate indices
  hr = pIB->Lock(0, nInds * sizeof( uint16 ), (void **) &pInds, 0);
  assert(SUCCEEDED(hr));

  memcpy( pInds, pIndsList, sizeof(uint16)*nInds );

  hr = pIB->Unlock();
  assert(SUCCEEDED(hr));

#elif defined (DIRECT3D10)
  D3D11_BUFFER_DESC BufDesc;
  ZeroStruct(BufDesc);
  BufDesc.ByteWidth = nVerts * sizeof( SVF_P3F_C4B_T2F );
  BufDesc.Usage = D3D11_USAGE_IMMUTABLE;
  BufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  BufDesc.CPUAccessFlags = 0;
  BufDesc.MiscFlags = 0;

  D3D11_SUBRESOURCE_DATA SubResData;
  ZeroStruct(SubResData);
  SubResData.pSysMem = pVtxList;
  SubResData.SysMemPitch = 0;
  SubResData.SysMemSlicePitch = 0;

  hr = dv->CreateBuffer(&BufDesc, &SubResData, (ID3D11Buffer **)&pVB);
  assert(SUCCEEDED(hr));

  ZeroStruct(BufDesc);
  BufDesc.ByteWidth = nInds * sizeof( uint16 );
  BufDesc.Usage = D3D11_USAGE_IMMUTABLE;
  BufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  BufDesc.CPUAccessFlags = 0;
  BufDesc.MiscFlags = 0;

  ZeroStruct(SubResData);
  SubResData.pSysMem = pIndsList;
  SubResData.SysMemPitch = 0;
  SubResData.SysMemSlicePitch = 0;

  hr = dv->CreateBuffer(&BufDesc, &SubResData, &pIB);
  assert(SUCCEEDED(hr));
#endif

  pINVB = pVB;
  pINIB = pIB;
}

int CSceneRain::Create()
{
  Release();

  HRESULT hr = S_OK;
  D3DDevice *dv = gcpRendD3D->GetD3DDevice(); 

  std::vector<SVF_P3F_C4B_T2F> pVB;
  std::vector<uint16> pIB;
  SVF_P3F_C4B_T2F pVertex;

  // Generate sphere vertices
  const int nSlicesH = 9 ;
  const int nSlicesV = 5;

  float nSliceVStep( DEG2RAD( 180.0f / (float) nSlicesV ) );
  float nSliceHStep( DEG2RAD( 360.0f / (float) nSlicesH ) );

  // center pos
  pVertex.xyz = Vec3(0,0,1.0);
  pVertex.color.dcolor = ~0;
  pVertex.st = Vec2(0.0f, 0.0f);
  pVB.push_back( pVertex );

  // Other alternative instead of creating cone base: create a grid, normalize yz -> this way we can wrap around sphere volume - tigher = more optimal

  // Generate base vertices
  pVertex.xyz = Vec3(0.0f,0,0.0f);
  pVB.push_back( pVertex );

  for(int h = 0; h < nSlicesH; ++h)
  {
    Vec3 pPos = Vec3( cosf( ((float)h) * nSliceHStep ),
                      sinf( ((float)h) * nSliceHStep ), 
                      0.0f);

    pVertex.xyz = pPos;
    pVB.push_back(pVertex);
  }

  // Generate base indices
  for(uint16 h = 0; h <= nSlicesH ; ++h)
  {
    pIB.push_back( 0 /* exclude center index */ + 1);
    pIB.push_back( h /* exclude center index */ + 1);

    if( h == nSlicesH  )
    {
      // last triangle
      pIB.push_back( 0/* exclude top/bottom cap index */ + 1);
      pIB.push_back( 1/* exclude top/bottom cap index */ + 1);
    }
  }

  // close previous strip
  pIB.push_back( 2 );
  pIB.push_back( 0 );

  m_nConeVBSize = pVB.size();
  m_nConeIBSize = pIB.size(); 

  CreateBuffers( m_nConeVBSize,m_pConeVB, &pVB[0], m_nConeIBSize, m_pConeIB, &pIB[0]);

  return 1;

}

void CSceneRain::Release()
{
  D3DVertexBuffer *pVB = (D3DVertexBuffer*)m_pConeVB;
  D3DIndexBuffer *pIB = (D3DIndexBuffer*)m_pConeIB;
  SAFE_RELEASE(pVB);
  SAFE_RELEASE(pIB);
	m_pConeVB = 0;
	m_pConeIB = 0;

	SAFE_DELETE( SPostEffectsUtils::tempRainTexture );
}


void CSceneRain::Render()
{
	if( !m_pConeVB || !m_pConeIB)
	{
		Create();
	}

	PROFILE_LABEL_PUSH("RAIN");

	int rainBufferWidth = gcpRendD3D->GetWidth() / 2;
	int rainBufferHeight = gcpRendD3D->GetWidth() / 2;

	if( !SPostEffectsUtils::tempRainTexture )
		SPostEffectsUtils::tempRainTexture = new SDynTexture( rainBufferWidth, rainBufferHeight, eTF_A8R8G8B8, eTT_2D,  FT_USAGE_RENDERTARGET | FT_STATE_CLAMP, "TempRain");

	SPostEffectsUtils::tempRainTexture->Update(rainBufferWidth, rainBufferHeight);
	SPostEffectsUtils::tempRainTexture->m_pTexture->SetRenderTargetTile(1);

  D3DVertexBuffer *pVB = (D3DVertexBuffer*)m_pConeVB;
  D3DIndexBuffer *pIB = (D3DIndexBuffer*)m_pConeIB;
  
	uint16 nVerts = m_nConeVBSize;
  uint16 nInds = m_nConeIBSize;

  gcpRendD3D->Set2DMode(false, 1, 1);    

	gcpRendD3D->FX_PushRenderTarget(0, SPostEffectsUtils::tempRainTexture->m_pTexture, NULL);
	gcpRendD3D->RT_SetViewport(0, 0, rainBufferWidth, rainBufferHeight);        

	ColorF clearColor(0, 0, 0, 0);
	gcpRendD3D->EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clearColor);
	
	CCryName pTechName("SceneRain");
  CCryName pParamName("sceneRainParams");

  // Get lightning color - will just use for overbright rain
  Vec3 v;
  gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_COLOR, v);

  //  Vec2 pMovDir;
  // pMovDir.x = gRenDev->GetRCamera().Orig.x - pPrevCamPos.x;
  // pMovDir.y = gRenDev->GetRCamera().Orig.y - pPrevCamPos.y;

  static Vec3 pPrevCamPos = gRenDev->GetRCamera().Orig;
	Vec4 pParams= Vec4(m_pAmount->GetParam() * CRenderer::CV_r_rainamount, 0, v.len(), 0.2f);
	float pParamsW[4] = {0.2f, 0.5f, 1.0f, 4.0f};

  for( int pass = 0; pass < 4; ++pass )
	{
		GetUtils().ShBeginPass(CShaderMan::m_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);   

		gcpRendD3D->EF_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE);
		gcpRendD3D->D3DSetCull(eCULL_None);

		pParams.w = pParamsW[pass];
		CShaderMan::m_shPostEffects->FXSetVSFloat(pParamName, &pParams, 1);
		CShaderMan::m_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

		gcpRendD3D->FX_SetVStream( 0, pVB, 0, sizeof( SVF_P3F_C4B_T2F ) );
		gcpRendD3D->FX_SetIStream( pIB );

		if (!FAILED(gcpRendD3D->FX_SetVertexDeclaration( 0, eVF_P3F_C4B_T2F )))
		{
			gcpRendD3D->FX_Commit();
	#if defined (DIRECT3D9) || defined(OPENGL)
			gcpRendD3D->m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP , 0, 0, nVerts, 0, nInds - 2);
	#elif defined (DIRECT3D10)
			gcpRendD3D->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			gcpRendD3D->m_pd3dDeviceContext->DrawIndexed(nInds, 0, 0);
	#endif
		}

		GetUtils().ShEndPass();
	}

	gcpRendD3D->FX_PopRenderTarget(0);
  gcpRendD3D->Set2DMode(true, 1, 1);       

	// workaround (re-set back-buffer): due to lazy RT updates/setting there's strong possibility we run into problems on x360 when we try to resolve from edram with no RT set
	gcpRendD3D->FX_SetActiveRenderTargets();

  pPrevCamPos += (gRenDev->GetRCamera().Orig - pPrevCamPos) * gEnv->pTimer->GetFrameTime() * 4.0f;

	PROFILE_LABEL_POP("RAIN");
}


bool CSceneRain::Preprocess()
{
	if( m_pAmount->GetParam() < 0.05f )
		return false;

	return true;
}


void CSceneRain::Reset()
{
	SAFE_DELETE( SPostEffectsUtils::tempRainTexture );
	m_pAmount->ResetParam(0.0f);  
}

void CSceneRain::OnLostDevice()
{
	Release();
}

const char * NPostEffects::CSceneRain::GetName() const
{
	return "SceneRain";
}
