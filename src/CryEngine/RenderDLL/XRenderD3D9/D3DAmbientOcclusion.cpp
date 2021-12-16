/*=============================================================================
D3DAmbientOcclusion.cpp : implementation of ambient occlusion related features.
Copyright 2001 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Vladimir Kajalin

=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "D3DPostProcess.h"

using namespace NPostEffects;

#undef min
#undef max

#if defined(DIRECT3D10) && !defined(PS3)
// there is no support for 16-bits RTs on DX10/11
# define FMT_SSAO_TARGET eTF_A8R8G8B8
#else
# define FMT_SSAO_TARGET eTF_R5G6B5
#endif

void CD3D9Renderer::CreateDeferredUnitBox(t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff)
{
  SVF_P3F_C4B_T2F vert;
  Vec3 vNDC;

  indBuff.clear();
  indBuff.reserve(36);

  vertBuff.clear();
  vertBuff.reserve(8);

  //Create frustum
  for (int i=0; i<8; i++ )
  {
    //Generate screen space frustum (CCW faces)
    vNDC = Vec3((i==0 || i==1 || i==4 || i==5) ? 0.0f : 1.0f,
      (i==0 || i==3 || i==4 || i==7) ? 0.0f : 1.0f,
      (i==0 || i==1 || i==2 || i==3) ? 0.0f : 1.0f
      );
    vert.xyz = vNDC;
    vert.st = Vec2(0.0f, 0.0f);
    vert.color.dcolor = -1;
    vertBuff.push_back(vert);
  }

  //CCW faces
  uint16 nFaces[6][4] = {{0,1,2,3},
  {4,7,6,5},
  {0,3,7,4},
  {1,5,6,2},
  {0,4,5,1},
  {3,2,6,7}
  };

  //init indices for triangles drawing
  for(int i=0; i < 6; i++)
  {
    indBuff.push_back( (uint16)  nFaces[i][0] );
    indBuff.push_back( (uint16)  nFaces[i][1] );
    indBuff.push_back( (uint16)  nFaces[i][2] );

    indBuff.push_back( (uint16)  nFaces[i][0] );
    indBuff.push_back( (uint16)  nFaces[i][2] );
    indBuff.push_back( (uint16)  nFaces[i][3] );
  }
}

void CD3D9Renderer::SetDepthBoundTest(float fMin, float fMax, bool bEnable)
{
  if(!m_bDeviceSupports_NVDBT)
    return;

  if (bEnable)
  {
#if defined (DIRECT3D9) || defined(OPENGL)
#if !defined(XENON) && !defined(PS3)
    m_pd3dDevice->SetRenderState(D3DRS_ADAPTIVETESS_X,MAKEFOURCC('N','V','D','B')); 
    m_pd3dDevice->SetRenderState(D3DRS_ADAPTIVETESS_Z,*(DWORD*)&fMin); 
    m_pd3dDevice->SetRenderState(D3DRS_ADAPTIVETESS_W,*(DWORD*)&fMax);
#endif


#elif defined (DIRECT3D10) //transparent execution without NVDB
    assert(0); 
#endif
  }
  else // disable depth bound test
  {
#if defined (DIRECT3D9) || defined(OPENGL)
  #if !defined(XENON) && !defined(PS3)
      m_pd3dDevice->SetRenderState(D3DRS_ADAPTIVETESS_X,0);     
  #endif


#elif defined (DIRECT3D10)
    assert(0);  //transparent execution without NVDB
#endif
  }
}

void CD3D9Renderer::FX_DeferredShadowPassAO( CCryName & TechName, SSectorTextureSet * pSector, Matrix44 * pMatComposite, int nIndexTexSlot, Vec4 * vOffset, bool bEnableStencil, bool bNoColorWrite )
{
  int nOffs;
  SVF_P3F_T2F_T3F *vQuad( (SVF_P3F_T2F_T3F *) GetVBPtr( 4, nOffs, POOL_P3F_TEX2F_TEX3F ) );

  if(!vQuad)
    return;

  Vec3 vBoundRectMin(0.0f, 0.0f, 0.0f), vBoundRectMax(1.0f, 1.0f, 1.0f);

  if(pSector && (CV_r_TerrainAO&4))
  {
    assert(pMatComposite); 

    CRenderCamera& rc = m_RP.m_TI[m_RP.m_nProcessThreadID].m_rcam;

    if(!pSector->stencilBox.IsContainPoint(rc.Orig))
      CalcAABBScreenRect(pSector->stencilBox, rc, *pMatComposite, &vBoundRectMin,  &vBoundRectMax);

    if(CV_r_TerrainAO&8)
    {
      gRenDev->Draw2dLabel( vBoundRectMin.x*GetWidth(), GetHeight() - vBoundRectMin.y*GetHeight(), 2, NULL, true, "MIN");
      gRenDev->Draw2dLabel( vBoundRectMax.x*GetWidth(), GetHeight() - vBoundRectMax.y*GetHeight(), 2, NULL, true, "MAX");
    }
  }

  float offsetX( 0 );
  float offsetY( 0 );

#if HALF_PIXEL_SHIFT_NEEDED
  int maskRTWidth = CTexture::s_ptexAOTarget->GetWidth();
  int maskRTHeight = CTexture::s_ptexAOTarget->GetHeight();
  if(maskRTWidth)
  {
	  offsetX = ( 0.5f / (float) maskRTWidth );
	  offsetY = ( - 0.5f / (float) maskRTHeight );
  }
#endif

  Vec3 vCoords[8];

  if (m_RenderTileInfo.nGridSizeX > 1.f && m_RenderTileInfo.nGridSizeY > 1.f)
    gcpRendD3D->GetRCamera().CalcTileVerts( vCoords,  
    m_RenderTileInfo.nGridSizeX-1-m_RenderTileInfo.nPosX, 
    m_RenderTileInfo.nPosY, 
    m_RenderTileInfo.nGridSizeX,
    m_RenderTileInfo.nGridSizeY);
  else
    gcpRendD3D->GetRCamera().CalcVerts( vCoords );

  Vec3 vRT = vCoords[4] - vCoords[0];
  Vec3 vLT = vCoords[5] - vCoords[1];
  Vec3 vLB = vCoords[6] - vCoords[2];
  Vec3 vRB = vCoords[7] - vCoords[3];

  GetRCamera().CalcRegionVerts(vCoords, Vec2(vBoundRectMin), Vec2(vBoundRectMax));
  vRT = vCoords[4] - vCoords[0];
  vLT = vCoords[5] - vCoords[1];
  vLB = vCoords[6] - vCoords[2];
  vRB = vCoords[7] - vCoords[3];

  float fVertDepth = 0.f;

  vQuad[0].p.x = vBoundRectMin.x - offsetX;
  vQuad[0].p.y = vBoundRectMin.y - offsetY;
  vQuad[0].p.z = fVertDepth;
  vQuad[0].st0[0] = vBoundRectMin.x;
  vQuad[0].st0[1] = 1 - vBoundRectMin.y;
  vQuad[0].st1 = vLB;

  vQuad[1].p.x = vBoundRectMax.x - offsetX;
  vQuad[1].p.y = vBoundRectMin.y - offsetY;
  vQuad[1].p.z = fVertDepth;
  vQuad[1].st0[0] = vBoundRectMax.x;
  vQuad[1].st0[1] = 1 - vBoundRectMin.y;
  vQuad[1].st1 = vRB;

  vQuad[3].p.x = vBoundRectMax.x - offsetX;
  vQuad[3].p.y = vBoundRectMax.y - offsetY;
  vQuad[3].p.z = fVertDepth;
  vQuad[3].st0[0] = vBoundRectMax.x;
  vQuad[3].st0[1] = 1-vBoundRectMax.y;
  vQuad[3].st1 = vRT;

  vQuad[2].p.x = vBoundRectMin.x - offsetX;
  vQuad[2].p.y = vBoundRectMax.y - offsetY;
  vQuad[2].p.z = fVertDepth;
  vQuad[2].st0[0] = vBoundRectMin.x;
  vQuad[2].st0[1] = 1-vBoundRectMax.y;
  vQuad[2].st1 = vLT;

  UnlockVB( POOL_P3F_TEX2F_TEX3F );

  FX_SetVStream( 0, m_pVB[ POOL_P3F_TEX2F_TEX3F ], 0, sizeof( SVF_P3F_T2F_T3F ) );

  // set shader
  CShader *pSH( CShaderMan::m_ShaderAmbientOcclusion );

  uint32 nPasses = 0;         

  pSH->FXSetTechnique(TechName);
  pSH->FXBegin( &nPasses, (pSector||(nIndexTexSlot>=0)) ? (FEF_DONTSETSTATES | FEF_DONTSETTEXTURES) : FEF_DONTSETSTATES);

  FX_DisableATOC();

  D3DSetCull(eCULL_Back);

  // Set shader quality
  switch (m_RP.m_nShaderQuality = CV_r_SSAO_quality)
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

  int newState = 0;

  newState |= GS_NODEPTHTEST;

  pSH->FXBeginPass( 0 );

  Vec4 vConst;

  if(pSector)
  { // terrain
    STexState TexStatePoint( FILTER_LINEAR, true );

    CTexture::s_ptexZTarget->Apply( 0, CTexture::GetTexState(TexStatePoint) );

    CTexture * pTerrTex0 = CTexture::GetByID(pSector->nTex0);
    pTerrTex0->Apply( 1, CTexture::GetTexState(TexStatePoint) );

    if(pSector->nTex1>0)
    {
      CTexture * pTerrTex1 = CTexture::GetByID(pSector->nTex1);
      pTerrTex1->Apply( 2, CTexture::GetTexState(TexStatePoint) );
    }
    else
      CTexture::s_ptexWhite->Apply( 2, CTexture::GetTexState(TexStatePoint) );

    float fSkyBr = gEnv->p3DEngine->GetSkyBrightness();
		static CCryName TerrainAOInfoName("TerrainAOInfo");
		Vec4 vTerrainAOInfo = Vec4(1.f - clamp_tpl(fSkyBr, 0.f, 1.f), 1.f/std::max((float)CV_r_TerrainAO_FadeDist,0.1f), 
															 m_RP.m_TI[m_RP.m_nProcessThreadID].m_rcam.Far / 1024.f, pSector->fTexScale);
		pSH->FXSetPSFloat(TerrainAOInfoName, &vTerrainAOInfo, 1);

		static CCryName AOSectorRangeName("AOSectorRange");
		Vec4 vAOSectorRange = Vec4(pSector->nodeBox.min.x, pSector->nodeBox.min.y, pSector->fTerrainMinZ, pSector->fTerrainMaxZ);
		// pre-multiplication for shader
		vAOSectorRange.x *= vTerrainAOInfo.w;
		vAOSectorRange.y *= vTerrainAOInfo.w;
		vAOSectorRange.w -= vAOSectorRange.z;
		pSH->FXSetPSFloat(AOSectorRangeName, &vAOSectorRange, 1);

  }
  else
  {
#if !defined(XENON) && !defined(PS3) && !defined(DIRECT3D10)
    if(nIndexTexSlot>=0)
    {
      int i=0;

      {
        STexState texState( FILTER_POINT, true );

        // bind index
        if(nIndexTexSlot>100)
          CTexture::GetByID(nIndexTexSlot-100)->Apply( 0, CTexture::GetTexState(texState) );
        else
          CTexture::GetByID(m_arrVoxTerrainDebugSrcTexId[nIndexTexSlot])->Apply( 0, CTexture::GetTexState(texState) );

        // bind data
        CTexture::GetByID(m_arrVoxTerrainDebugSrcTexId[ 8])->Apply( 1, CTexture::GetTexState(texState) );

        if(m_arrVoxTerrainDebugSrcTexId.Count() > 9)
        {
          CTexture::GetByID(m_arrVoxTerrainDebugSrcTexId[ 9])->Apply( 2, CTexture::GetTexState(texState) );
          CTexture::GetByID(m_arrVoxTerrainDebugSrcTexId[10])->Apply( 3, CTexture::GetTexState(texState) );

          for(int n=0; n<8; n++)
            CTexture::GetByID(m_arrVoxTerrainDebugSrcTexId[11+n])->Apply( 4+n, CTexture::GetTexState(texState) );
        }

        if(vOffset)
        {
          static CCryName paramName("VoxTerrainDebug_Offset");
          pSH->FXSetPSFloat(paramName, vOffset, 1);
        }
      }

      if(nIndexTexSlot<100)
      {
        newState |= (GS_BLSRC_ONE | GS_BLDST_ONE);		// additive
#if !defined(EXCLUDE_SCALEFORM_SDK)
        SF_SetBlendOp(SSF_GlobalDrawParams::Max);
#else
#pragma message("CD3D9Renderer::FX_DeferredShadowPassAO() - SF_SetBlendOp() not available in this config!")
#endif
      }
    }
#endif
  }

  if (!FAILED(FX_SetVertexDeclaration( 0, eVF_P3F_T2F_T3F )))
  {
    if(bNoColorWrite)
      newState |= GS_COLMASK_NONE;
    else if(nIndexTexSlot>=0)
    {
    }
    else if(pSector)
		{
			// multiplicative blending
			newState |= GS_BLDST_SRCCOL | GS_BLSRC_ZERO;
			newState &= ~(GS_NOCOLMASK_R | GS_NOCOLMASK_G | GS_NOCOLMASK_B | GS_NOCOLMASK_A);
		}

    if(bEnableStencil)
      newState |= GS_STENCIL;

    if(pSector && bEnableStencil)
      EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_EQUAL) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
      STENCOP_PASS(FSS_STENCOP_KEEP),
      m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFFFFFF);

    if(fVertDepth > 0)
      newState |= GS_DEPTHFUNC_GREAT;

    EF_SetState( newState );
    FX_Commit();

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

#if !defined(EXCLUDE_SCALEFORM_SDK)
  if(vOffset) // restore default blend operation
    SF_SetBlendOp(SSF_GlobalDrawParams::RevSubstract, true);
#else
#pragma message("CD3D9Renderer::FX_DeferredShadowPassAO() - SF_SetBlendOp() not available in this config!")
#endif

  // workaround: otherwise GS_DEPTHFUNC stays random
  EF_SetState( 0 );
  FX_Commit();
}

void StretchRectAO(CTexture *pSrc, CTexture *pDst, uint32 method);

void StretchRect_VoxTerrain(SDynTexture ** pSrc, int nSrcNum, SDynTexture * pDst, CCryName & strTechName);

void CD3D9Renderer::ProcessVoxTerrainDebugTarget(int * _pSrcTexIds, int nMipsNum, bool bOnlySetSrcTexture)
{
  if(bOnlySetSrcTexture)
  { // only store textures id
    m_arrVoxTerrainDebugSrcTexId.Clear();
    for(int i=0; i<nMipsNum; i++)
      m_arrVoxTerrainDebugSrcTexId.Add(_pSrcTexIds[i]);
    return;
  }

  PROFILE_FRAME(ProcessVoxTerrainDebugTarget);





  int nWidth = GetWidth();
  int nHeight = GetHeight();

  const int nTempRTNum = 8;
  static SDynTexture * arrTempRT[nTempRTNum] = {0,0,0,0,0,0,0,0};

  for(int i=0; i<nTempRTNum; i++)
  {
    if(!arrTempRT[i])
    {
      char szName[256];
      sprintf(szName, "VoxTerrainTempRT_%d", i);
      arrTempRT[i] = new SDynTexture(nWidth, nHeight, eTF_A16B16G16R16F, eTT_2D,  FT_STATE_CLAMP, szName, 95);
    }
    
    arrTempRT[i]->Update(nWidth, nHeight);

    FX_PushRenderTarget(0, arrTempRT[i]->m_pTexture, &m_DepthBufferOrig);

    ColorF clClear(0,0,0,0);
    EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_STENCIL|FRT_CLEAR_IMMEDIATE, &clClear, 1);

    {
      PROFILE_SHADER_START;
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Push();
      mathMatrixOrthoOffCenterLH( m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop(), 0, 1, 0, 1, -1, 1 );
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Push();
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->LoadIdentity();

      bool bUseStencil = true;

      if(bUseStencil)
      { // stencil set
        EF_SetStencilState(
          STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
          STENCOP_FAIL(FSS_STENCOP_KEEP) |
          STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
          STENCOP_PASS(FSS_STENCOP_REPLACE),
          100, 0xFFFFFFFF, 0xFFFFFFFF);

        static CCryName TechName = "VoxTerrainDebugClear_Pass";
        Vec4 vOffset(0,0,(float)gEnv->p3DEngine->GetTerrainSize(),gEnv->pSystem->GetViewCamera().GetFarPlane());
        FX_DeferredShadowPassAO( TechName, NULL, NULL, i, &vOffset, true, true );

        // test
        EF_SetStencilState(
          STENC_FUNC(FSS_STENCFUNC_EQUAL) |
          STENCOP_FAIL(FSS_STENCOP_KEEP) |
          STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
          STENCOP_PASS(FSS_STENCOP_KEEP),
          100, 0xFFFFFFFF, 0xFFFFFFFF
          );
      }

      {
        static CCryName TechName = "VoxTerrainDebug_Pass";
        Vec4 vOffset(0,0,(float)gEnv->p3DEngine->GetTerrainSize(),gEnv->pSystem->GetViewCamera().GetFarPlane());
        FX_DeferredShadowPassAO( TechName, NULL, NULL, i, &vOffset, bUseStencil, false );
      }

      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Pop();
      PROFILE_SHADER_END;
    }

    FX_PopRenderTarget(0);
  }

  // combine result
  static SDynTexture * pTempResultRT0 = 0;
  {
    if(!pTempResultRT0)
      pTempResultRT0 = new SDynTexture(nWidth, nHeight, eTF_A16B16G16R16F, eTT_2D,  FT_STATE_CLAMP, "VoxTerrainRT1", 95);
    static CCryName strTechName = "CombineVoxDebugTargets";
    Set2DMode(true, 1,1);
    pTempResultRT0->Update(nWidth, nHeight);
    StretchRect_VoxTerrain(&arrTempRT[0], nTempRTNum, pTempResultRT0, strTechName);
    Set2DMode(false, 1,1);
  }

  // compute normals
  static SDynTexture * pTempResultRT1 = 0;
  {
    if(!pTempResultRT1)
      pTempResultRT1 = new SDynTexture(nWidth, nHeight, eTF_A16B16G16R16F, eTT_2D,  FT_STATE_CLAMP, "VoxTerrainRT0", 95);
    pTempResultRT1->Update(nWidth, nHeight);

    FX_PushRenderTarget(0, pTempResultRT1->m_pTexture, &m_DepthBufferOrig);

    ColorF clClear(0,0,0,0);
    EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_STENCIL, &clClear, 1);

    PROFILE_SHADER_START;
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Push();
    mathMatrixOrthoOffCenterLH( m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop(), 0, 1, 0, 1, -1, 1 );
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Push();
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->LoadIdentity();

    Vec4 vOffset(0,0,(float)gEnv->p3DEngine->GetTerrainSize(),gEnv->pSystem->GetViewCamera().GetFarPlane());
    static CCryName TechName = "VoxTerrainDebugNormals_Pass";
    FX_DeferredShadowPassAO( TechName, NULL, NULL, pTempResultRT0->GetTexture()->GetTextureID()+100, &vOffset, false, false );

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Pop();
    PROFILE_SHADER_END;

    FX_PopRenderTarget(0);
  }




}


void CD3D9Renderer::GenerateAO(CTexture *pSSAORT, CCryName const& TechName)
{
	int nOffs;
	SVF_P3F_T2F_T3F *vQuad( (SVF_P3F_T2F_T3F *) GetVBPtr( 4, nOffs, POOL_P3F_TEX2F_TEX3F ) );

	if(!vQuad)
		return;

	// Prepare the matrices
	m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Push();
	mathMatrixOrthoOffCenterLH( m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop(), 0, 1, 0, 1, -1, 1 );
	m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Push();
	m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->LoadIdentity();

	int maskRTWidth = pSSAORT->GetWidth();
	int maskRTHeight = pSSAORT->GetHeight();

#if HALF_PIXEL_SHIFT_NEEDED
	Vec3  offset( -1.f/maskRTWidth, 1.f/maskRTHeight, 0 );
#else
	Vec3  offset( 0, 0, 0 );
#endif

	vQuad[0].p = Vec3(-1, 1, 1) + offset;
	vQuad[0].st0[0] = 0;
	vQuad[0].st0[1] = 0;

	vQuad[2].p = Vec3( 1, 1, 1) + offset;
	vQuad[2].st0[0] = 1;
	vQuad[2].st0[1] = 0;

	vQuad[1].p = Vec3(-1,-1, 1) + offset;
	vQuad[1].st0[0] = 0;
	vQuad[1].st0[1] = 1;

	vQuad[3].p = Vec3( 1,-1, 1) + offset;
	vQuad[3].st0[0] = 1;
	vQuad[3].st0[1] = 1;

	UnlockVB( POOL_P3F_TEX2F_TEX3F );

	FX_SetVStream( 0, m_pVB[ POOL_P3F_TEX2F_TEX3F ], 0, sizeof( SVF_P3F_T2F_T3F ) );

	// set shader
	CShader *pSH( CShaderMan::m_ShaderAmbientOcclusion );

	uint32 nPasses = 0;         

	pSH->FXSetTechnique(TechName);
	pSH->FXBegin( &nPasses, FEF_DONTSETSTATES);

	FX_DisableATOC();

	D3DSetCull(eCULL_Back);

	// Set shader quality
	switch (m_RP.m_nShaderQuality = CV_r_SSAO_quality)
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

	int newState = 0;

	newState |= GS_NODEPTHTEST;

	// Save the old flags because we might modify them later
	uint64 shaderFalgs = m_RP.m_FlagsShader_RT;

	if (CV_r_SSAO_normals)
	{
		// SSAO with normals

		const uint64 fullMask = (g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1]);
		uint64 mask = 0;

		if (CV_r_SSAO_downscale)
			mask |= g_HWSR_MaskBit[HWSR_SAMPLE0];

		if (CV_r_SSAO_downscale_ztarget)
			mask |= g_HWSR_MaskBit[HWSR_SAMPLE1];

		m_RP.m_FlagsShader_RT = (m_RP.m_FlagsShader_RT & ~fullMask) | mask;
	}

	pSH->FXBeginPass( 0 );

	static CCryName sSSAOParamName("SSAO_params");

	if (CV_r_SSAO_normals)
	{
		Vec4 vConst( CV_r_SSAO_amount*gEnv->p3DEngine->GetSSAOAmount(), 0, 0, 0);
		pSH->FXSetPSFloat(sSSAOParamName, &vConst, 1);

		int destWidth = maskRTWidth, distHeight = maskRTHeight,
				srcWidth  = maskRTWidth, srcHeight = maskRTHeight;

		Matrix44A matView;
		matView = m_RP.m_TI[m_RP.m_nProcessThreadID].m_cam.GetViewMatrix();

		// Adjust the camera matrix so that the camera space will be:
		// +y = down, +z - towards, +x - right
		Vec3 zAxis = matView.GetRow(1);

		matView.SetRow(1, -matView.GetRow(2));
		matView.SetRow(2, zAxis);

		float z = matView.m13;
		matView.m13 = -matView.m23;
		matView.m23 = z;
	
		static CCryName paramName2("SSAO_CameraMatrix");
		pSH->FXSetPSFloat(paramName2, (Vec4*)matView.GetData(), 3);

		static CCryName paramName3("SSAO_filter");

		// Setup the offsets for normal filtering.
		float dx = .5f / GetWidth(), dy = .5f / GetHeight();
		vConst = Vec4(-dx, -dy, dx, dy);

		pSH->FXSetPSFloat(paramName3, &vConst, 1);
	}
	else
	{
		float fRadiusXY = CV_r_SSAO_radius;
		if(m_RenderTileInfo.nGridSizeX > 1.f)
			fRadiusXY *= 0.5f*(m_RenderTileInfo.nGridSizeX+m_RenderTileInfo.nGridSizeY);

		Vec4 vConst(CV_r_SSAO_amount*gEnv->p3DEngine->GetSSAOAmount(), CV_r_SSAO_darkening, fRadiusXY, CV_r_SSAO_radius);
		pSH->FXSetPSFloat(sSSAOParamName, &vConst, 1);
	}

	if (!FAILED(FX_SetVertexDeclaration( 0, eVF_P3F_T2F_T3F )))
	{
#if !defined(XENON) && !defined(PS3)
		newState |= (GS_NOCOLMASK_G | GS_NOCOLMASK_B | GS_NOCOLMASK_A);



#endif

		EF_SetState( newState );

		FX_Commit();

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

	// workaround: otherwise GS_DEPTHFUNC stays random
	EF_SetState( 0 );
	FX_Commit();

	// Restore the old flags
	m_RP.m_FlagsShader_RT = shaderFalgs;

	// Restore the matrices
	m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();
	m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Pop();
}

void StretchRect(CTexture *pSrc, CTexture *pDst);

bool CD3D9Renderer::FX_ProcessAOTarget()
{
  PROFILE_FRAME(FX_ProcessAOTarget);
	PROFILE_LABEL_PUSH( "SSAO" );

  // generate terrain sector texture
  BuildTerrainTexture();
  
  if(m_arrVoxTerrainDebugSrcTexId.Count())
    ProcessVoxTerrainDebugTarget(0, 0, false);

  if (!CTexture::IsTextureExist(CTexture::s_ptexZTarget))
    return true; // z-target not ready

  bool bSSAO_ON = CV_r_SSAO && (CV_r_SSAO_amount*gEnv->p3DEngine->GetSSAOAmount() > 0.1f);

  // AO target is used for SSAO and also for TerrainAO
  // In order to disable AO completely you have to set to zero r_SSAO and r_TerrainAO
  if(!bSSAO_ON && !(CV_r_TerrainAO && m_RP.m_TerrainAONodes[m_RP.m_nProcessThreadID].Count()))
    return true;





  m_RP.m_nAOMaskUpdateLastFrameId = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID;

  int nWidth = GetWidth();
  int nHeight = GetHeight();

	SAFE_DELETE(CDeferredShading::Instance().m_pSSAORT);

  // allocate temp RT if needed
  if(bSSAO_ON)
  {
		CV_r_SSAO_downscale = clamp_tpl(CV_r_SSAO_downscale, 0, 1);
#if !defined(XENON) && !defined(PS3)
		// reuse downsampled z buffer for SSAO (blue channel)
		CDeferredShading::Instance().m_pSSAORT = new SDynTexture(nWidth>>CV_r_SSAO_downscale, nHeight>>CV_r_SSAO_downscale, FMT_SSAO_TARGET, eTT_2D,  FT_STATE_CLAMP, "TempAORT", 95);
    CDeferredShading::Instance().m_pSSAORT->Update(nWidth>>CV_r_SSAO_downscale, nHeight>>CV_r_SSAO_downscale);
#endif
  }

	SetDepthBoundTest(0.f, 1.f, false);

  // generate downscaled z-target
  if(CV_r_SSAO_downscale_ztarget && bSSAO_ON)
  {
    Set2DMode(true, 1,1);
#if !defined(XENON) && !defined(PS3)
		ETEX_Format fmtZScaled = eTF_G16R16F;



#endif

		CTexture::s_ptexZTargetScaled->Invalidate(GetWidth()>>1, GetHeight()>>1, fmtZScaled);





    StretchRectAO(CTexture::s_ptexZTarget, CTexture::s_ptexZTargetScaled, 0);
    Set2DMode(false, 1,1);
	}
  else
	{
		CTexture::s_ptexZTargetScaled->ReleaseDeviceTexture(false);
	}

	SDynTexture *pSSAODynRT = CDeferredShading::Instance().m_pSSAORT;

	if( !CV_r_SSAO_downscale || !CV_r_deferredshading )
	{
		int nDownScaleResult = pSSAODynRT ? CV_r_SSAO_downscale_result_mask : 0;
		CTexture::s_ptexAOTarget->Invalidate(nWidth>>nDownScaleResult, nHeight>>nDownScaleResult, FMT_SSAO_TARGET);
	}
	else
		CTexture::s_ptexAOTarget->ReleaseDeviceTexture(false);	// Release device texture when not needed at all

#if !defined(XENON) && !defined(PS3)
	CTexture * pSSAORT = pSSAODynRT ? pSSAODynRT->m_pTexture : CTexture::s_ptexAOTarget;



#endif
	FX_PushRenderTarget(0, pSSAORT, CV_r_SSAO_downscale ? NULL : &m_DepthBufferOrig, false, -1, CV_r_SSAO_downscale==0);

  // render SSAO pass into Red channel
  if (bSSAO_ON)
  {
    PROFILE_FRAME(FX_ProcessAOTarget_SSAO);

    PROFILE_SHADER_START;

		static CCryName pTechNames[4] =
		{
			"Deferred_SSAO_Pass",
			"Deferred_SSAO_WithNormals_Fast",
			"Deferred_SSAO_WithNormals_Quality",
			"Deferred_SSAO_WithNormals_Old"
		};







		uint32 nTechID = (CV_r_SSAO_normals && CV_r_deferredshading)? min(CV_r_SSAO, 3) : 0;
		GenerateAO( pSSAORT, pTechNames[nTechID] );

    PROFILE_SHADER_END;
  }
  else 
  { // workaround: otherwise terrain AO does not work
		EF_SetState( 0 );
    FX_Commit();
  }

  FX_PopRenderTarget(0);





	PROFILE_LABEL_POP( "SSAO" );

  return true;
}

void StretchRect(CTexture *pSrc, CTexture *pDst);

IDynTexture * CD3D9Renderer::RequestTerrainTexture(SBuildTerrainTextureParams & params) 
{ 
  if(params.pDstTex_Diff)
  {
    for(int i=0; i<m_buildTerrainTextureTasks.Count(); i++)
    {
      assert(m_buildTerrainTextureTasks[i].pDstTex_Diff);
      if(m_buildTerrainTextureTasks[i].pDstTex_Diff == params.pDstTex_Diff)
      {
        m_buildTerrainTextureTasks.Delete(i);
        i--;
      }
    }
  }

  m_buildTerrainTextureTasks.Add(params);

  SBuildTerrainTextureParams & rLast = m_buildTerrainTextureTasks.Last(); 

  if(!rLast.pDstTex_Diff)
  {
    rLast.pDstTex_Diff = (IDynTexture*) new SDynTexture2(params.nDstTexDimX, params.nDstTexDimY,  FT_STATE_CLAMP, "BTT_Diff", eTP_VoxTerrain);

    rLast.pDstTex_Bump = (IDynTexture*) new SDynTexture2(params.nDstTexDimX/params.nBumpDownScale, params.nDstTexDimY/params.nBumpDownScale,  FT_STATE_CLAMP, "BTT_Bump", eTP_VoxTerrain);
  }

  params = rLast;

  BuildTerrainTexture();

  return NULL;
}

void StretchRect_BTT(CTexture *pSrc, IDynTexture * pDst, CCryName & strTechName);

// terrain texture generation on GPU
void CD3D9Renderer::BuildTerrainTexture()
{
  FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

  //PS3workaround -> remove




  if(!m_buildTerrainTextureTasks.Count())
    return;

	// Avoid using ResetToDefault - it's reseting everything, including gpr's states - we leave this comment out for now
	// in case any problems
	//ResetToDefault();

  FX_ResetPipe();

  int TempX, TempY, TempWidth, TempHeight;
  GetViewport(&TempX, &TempY, &TempWidth, &TempHeight);

  while(m_buildTerrainTextureTasks.Count())
  {
    SBuildTerrainTextureParams rTask = m_buildTerrainTextureTasks[0];
    m_buildTerrainTextureTasks.Delete(0);

    if(!rTask.pRM || !rTask.nDstTexDimX)
      continue;

    int nPrevStr = CV_r_texturesstreamingsync;
    int nPrevAsync = CV_r_shadersasynccompiling;

    if(rTask.nSyncTextures)
      CV_r_texturesstreamingsync = 1;
    CV_r_shadersasynccompiling = 0;
    
    CRenderMesh2 * pRM = (CRenderMesh2 *)rTask.pRM;
    pRM->CheckUpdate(eVF_P3S_N4B_C4B_T2S, 0);

    if(m_RP.m_pCurObject)
      m_RP.m_pCurObject->m_fDistance = rTask.fLodDistance;
    assert(m_RP.m_pCurObject);

    // use tmp targets
    static SDynTexture * pRT_Diff = new SDynTexture(rTask.nDstTexDimX, rTask.nDstTexDimY, 
      eTF_A8R8G8B8, eTT_2D,  FT_STATE_CLAMP, "BTT_RT_Diff", 95);      
    static SDynTexture * pRT_Bump = new SDynTexture(rTask.nDstTexDimX, rTask.nDstTexDimY, 
      eTF_A8R8G8B8, eTT_2D,  FT_STATE_CLAMP, "BTT_RT_Bump", 95);      

    pRT_Diff->Update(rTask.nDstTexDimX, rTask.nDstTexDimY);
    pRT_Bump->Update(rTask.nDstTexDimX, rTask.nDstTexDimY);

    FX_PushRenderTarget(0, pRT_Diff->m_pTexture, NULL);
    FX_PushRenderTarget(1, pRT_Bump->m_pTexture, NULL);

    RT_SetViewport(0, 0, rTask.nDstTexDimX, rTask.nDstTexDimY);

    // clear
    float clearVal = 0.1f;

//#define CUSTOM_BLEND 1

#if defined (DIRECT3D9) && defined (CUSTOM_BLEND)
    ColorF clClear(clearVal,clearVal,clearVal,0.f);
#else
    ColorF clClear(clearVal,clearVal,clearVal,1.f);
#endif

    EF_ClearBuffers(FRT_CLEAR_COLOR|FRT_CLEAR_IMMEDIATE, &clClear, 1);

    PROFILE_SHADER_START;

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Push();
    mathMatrixOrthoOffCenterLH( m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop(), 0, 1, 0, 1, -1, 1 );
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Push();
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->LoadIdentity();

#if defined (DIRECT3D9) && defined (CUSTOM_BLEND)
    m_pd3dDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
#endif

    int nTexStateIdRepeat = CTexture::GetTexState(STexState( FILTER_TRILINEAR, false ));

    // apply roads, decals and terrain layers sorted by sort priority
    for(int d=0; d<rTask.pDecalsAndRoadsAndTerrainLayers->Count(); d++)
    {
      SBuildTerrainTextureParams::SBuildItem & buildItem = rTask.pDecalsAndRoadsAndTerrainLayers->GetAt(d);

      if(buildItem.pRenderNode && buildItem.pRenderNode->GetRenderNodeType() == eERType_Road)
      {
        IRoadRenderNode * pRoadNode = (IRoadRenderNode*)buildItem.pRenderNode;

        Matrix44 matRot; matRot.SetIdentity(); //pRoadNode->GetMatrix());
        matRot.SetTranslation(Vec3(0,0,0));

        float arrTexCoordInfo[4];
        pRoadNode->GetTexCoordInfo(&arrTexCoordInfo[0]);

        if(IMaterial * pMaterial = pRoadNode->GetMaterial())
        {
          SShaderItem & sItem = pMaterial->GetShaderItem(0);

          ITexture * pITex_Bump = CTexture::s_ptexFlatBump;
          if(SEfResTexture * pTex_Bump = sItem.m_pShaderResources->GetTexture(EFTT_BUMP))
            if(pTex_Bump->m_Sampler.m_pITex)
              pITex_Bump = pTex_Bump->m_Sampler.m_pITex;

          ITexture * pITex_Spec = CTexture::s_ptexWhite;
          if(SEfResTexture * pTex_Spec = sItem.m_pShaderResources->GetTexture(EFTT_GLOSS))
            if(pTex_Spec->m_Sampler.m_pITex)
              pITex_Spec = pTex_Spec->m_Sampler.m_pITex;

          if(SEfResTexture * pTex_Diff = sItem.m_pShaderResources->GetTexture(EFTT_DIFFUSE))
          {
            if(ITexture * pITex_Diff = pTex_Diff->m_Sampler.m_pITex)
            {
              // set shader
              CShader *pSH( CShaderMan::m_ShaderAmbientOcclusion );
              static CCryName TechName = "ProjectRoadSegment";
              pSH->FXSetTechnique(TechName);
              uint32 nPasses = 0;         
              pSH->FXBegin( &nPasses, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES);

              // set states
              FX_DisableATOC();
              D3DSetCull(eCULL_None);
              int newState = GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;

#if defined (DIRECT3D9) && defined (CUSTOM_BLEND)
              m_pd3dDevice->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_ZERO); 
              m_pd3dDevice->SetRenderState(D3DRS_DESTBLENDALPHA,D3DBLEND_ONE); 
#endif

              pSH->FXBeginPass( 0 );

              // set road textures

              CTexture * pTerrTex0 = (CTexture *)pITex_Diff;
              pTerrTex0->Apply( 0, nTexStateIdRepeat );

              CTexture * pTerrTex1 = (CTexture *)pITex_Bump;
              pTerrTex1->Apply( 1, nTexStateIdRepeat );

              CTexture * pTerrTex2 = (CTexture *)pITex_Spec;
              pTerrTex2->Apply( 2, nTexStateIdRepeat );

              Plane arrPlanes[8];
              ZeroStruct(arrPlanes);

              pRoadNode->GetClipPlanes(&arrPlanes[0], 6);

              // set params
              {
                static CCryName paramName("ProjectRoadSegmentPlanes0123_PS");
                pSH->FXSetPSFloat(paramName, (Vec4*)&arrPlanes[0], 4);
              }

              {
                static CCryName paramName("ProjectRoadSegmentPlanes4567_PS");
                pSH->FXSetPSFloat(paramName, (Vec4*)&arrPlanes[4], 4);
              }

              {
                static CCryName paramName("ProjectRoadSegmentTexCoordRange_PS");
                pSH->FXSetPSFloat(paramName, (Vec4*)&arrTexCoordInfo[0], 1);
              }

              {
                static CCryName paramName("ProjectRoadSegmentMatrixRot_PS");
                pSH->FXSetPSFloat(paramName, (Vec4*)matRot.GetData(), 4);
              }

              {
                Vec4 vDiffuseColor;
                vDiffuseColor.x = sItem.m_pShaderResources->GetDiffuseColor().r;
                vDiffuseColor.y = sItem.m_pShaderResources->GetDiffuseColor().g;
                vDiffuseColor.z = sItem.m_pShaderResources->GetDiffuseColor().b;
                vDiffuseColor.w = sItem.m_pShaderResources->GetOpacity();
                static CCryName paramName("ProjectRoadSegmentMatColor_PS");
                pSH->FXSetPSFloat(paramName, (Vec4*)&vDiffuseColor, 1);
              }

              {
                Vec4 vSpec(0,0,0,0);
                vSpec.x = sItem.m_pShaderResources->GetSpecularColor().Luminance();
                vSpec.y = sItem.m_pShaderResources->GetSpecularShininess();
                if(!(sItem.m_pShader->GetFlags2() & EF2_VERTEXCOLORS))
                  vSpec.z = 1.f;
                static CCryName paramName("ProjectRoadSegmentMatSpec_PS");
                pSH->FXSetPSFloat(paramName, (Vec4*)&vSpec, 1);
              }

              {
                static CCryName paramName("BuildTerrainTextureParams_VS");
                Vec4 vConst(rTask.vMeshWSPos.x, rTask.vMeshWSPos.y, rTask.vMeshWSPos.z, (float)buildItem.nTerrainLayerId);
                pSH->FXSetVSFloat(paramName, (Vec4*) &vConst, 1);       
              }

              {
                static CCryName paramName("BuildTerrainTextureParams4_PS");
                Vec4 vConst(rTask.fTexRangeScale, 0.f, 0.f, 0.f);
                pSH->FXSetPSFloat(paramName, &vConst, 1);
              }

              EF_SetState( newState );
              FX_Commit();

              // render mesh
              pRM->DrawImmediately();

              // end
              pSH->FXEndPass();
              pSH->FXEnd();
              EF_SetState( 0 );
              FX_Commit();
            }
          }
        }
      }
      else if(buildItem.pRenderNode && buildItem.pRenderNode->GetRenderNodeType() == eERType_Decal)
      {
        IDecalRenderNode * pDecalNode = (IDecalRenderNode*)buildItem.pRenderNode;

        Matrix44 matDecalProjInv(pDecalNode->GetMatrix().GetInverted());
        Matrix44 matDecalProjRot(pDecalNode->GetMatrix());
        matDecalProjRot.SetTranslation(Vec3(0,0,0));

        if(IMaterial * pMaterial = pDecalNode->GetMaterial())
        {
          SShaderItem & sItem = pMaterial->GetShaderItem(0);

          ITexture * pITex_Bump = CTexture::s_ptexFlatBump;
          if(SEfResTexture * pTex_Bump = sItem.m_pShaderResources->GetTexture(EFTT_BUMP))
            if(pTex_Bump->m_Sampler.m_pITex)
              pITex_Bump = pTex_Bump->m_Sampler.m_pITex;

          ITexture * pITex_Spec = CTexture::s_ptexWhite;
          if(SEfResTexture * pTex_Spec = sItem.m_pShaderResources->GetTexture(EFTT_GLOSS))
            if(pTex_Spec->m_Sampler.m_pITex)
              pITex_Spec = pTex_Spec->m_Sampler.m_pITex;

          if(SEfResTexture * pTex_Diff = sItem.m_pShaderResources->GetTexture(EFTT_DIFFUSE))
          {
            if(ITexture * pITex_Diff = pTex_Diff->m_Sampler.m_pITex)
            {
              // set shader
              CShader *pSH( CShaderMan::m_ShaderAmbientOcclusion );
              static CCryName TechName = "ProjectTerrainDecal";
              pSH->FXSetTechnique(TechName);
              uint32 nPasses = 0;         
              pSH->FXBegin( &nPasses, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES);

              // set states
              FX_DisableATOC();
              D3DSetCull(eCULL_None);
              int newState = GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;

#if defined (DIRECT3D9) && defined (CUSTOM_BLEND)
              m_pd3dDevice->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_ZERO); 
              m_pd3dDevice->SetRenderState(D3DRS_DESTBLENDALPHA,D3DBLEND_ONE); 
#endif

              pSH->FXBeginPass( 0 );

              // set decal textures

              STexState texState( FILTER_TRILINEAR, true );
              texState.SetClampMode(
                (*pTex_Diff).m_bUTile ? TADDR_WRAP : TADDR_CLAMP,
                (*pTex_Diff).m_bVTile ? TADDR_WRAP : TADDR_CLAMP, 
                (*pTex_Diff).m_bUTile ? TADDR_WRAP : TADDR_CLAMP);

              int nTexState = CTexture::GetTexState(texState);

              CTexture * pTerrTex0 = (CTexture *)pITex_Diff;
              pTerrTex0->Apply( 0, nTexState );

              CTexture * pTerrTex1 = (CTexture *)pITex_Bump;
              pTerrTex1->Apply( 1, nTexState );

              CTexture * pTerrTex2 = (CTexture *)pITex_Spec;
              pTerrTex2->Apply( 2, nTexState );

              // set params
              {
                static CCryName paramName("ProjectTerrainDecalMatrixInv_PS");
                pSH->FXSetPSFloat(paramName, (Vec4*)matDecalProjInv.GetData(), 4);
              }

              {
                static CCryName paramName("ProjectTerrainDecalMatrixRot_PS");
                pSH->FXSetPSFloat(paramName, (Vec4*)matDecalProjRot.GetData(), 4);
              }

              {
                static CCryName paramName("ProjectTerrainDecalTextureAtlasInfo_PS");
                Vec4 vTextureAtlasInfo;
                vTextureAtlasInfo.x = (*pTex_Diff).m_TexModificator.m_Offs[0];
                vTextureAtlasInfo.y = (*pTex_Diff).m_TexModificator.m_Offs[1];
                vTextureAtlasInfo.z = (*pTex_Diff).m_TexModificator.m_Tiling[0];
                vTextureAtlasInfo.w = (*pTex_Diff).m_TexModificator.m_Tiling[1];
                pSH->FXSetPSFloat(paramName, &vTextureAtlasInfo, 1);
              }

              {
                Vec4 vDiffuseColor;
                vDiffuseColor.x = sItem.m_pShaderResources->GetDiffuseColor().r;
                vDiffuseColor.y = sItem.m_pShaderResources->GetDiffuseColor().g;
                vDiffuseColor.z = sItem.m_pShaderResources->GetDiffuseColor().b;
                vDiffuseColor.w = sItem.m_pShaderResources->GetOpacity();
                static CCryName paramName("ProjectTerrainDecalMatColor_PS");
                pSH->FXSetPSFloat(paramName, (Vec4*)&vDiffuseColor, 1);
              }

              {
                Vec4 vSpec(0,0,0,0);
                vSpec.x = sItem.m_pShaderResources->GetSpecularColor().Luminance();
                vSpec.y = sItem.m_pShaderResources->GetSpecularShininess();
                vSpec.z = float(pDecalNode->GetDecalProperties()->m_projectionType == SDecalProperties::ePlanar);
                static CCryName paramName("ProjectTerrainDecalMatSpec_PS");
                pSH->FXSetPSFloat(paramName, (Vec4*)&vSpec, 1);
              }

              {
                static CCryName paramName("BuildTerrainTextureParams_VS");
                Vec4 vConst(rTask.vMeshWSPos.x, rTask.vMeshWSPos.y, rTask.vMeshWSPos.z, (float)buildItem.nTerrainLayerId);
                pSH->FXSetVSFloat(paramName, (Vec4*) &vConst, 1);       
              }

              {
                static CCryName paramName("BuildTerrainTextureParams4_PS");
                Vec4 vConst(rTask.fTexRangeScale, 0.f, 0.f, 0.f);
                pSH->FXSetPSFloat(paramName, &vConst, 1);
              }

              EF_SetState( newState );
              FX_Commit();

              // render mesh
              pRM->DrawImmediately();

              // end
              pSH->FXEndPass();
              pSH->FXEnd();
              EF_SetState( 0 );
              FX_Commit();
            }
          }
        }
      }
      else if(!buildItem.pRenderNode && buildItem.nTerrainLayerId>=0)
      { // terrain layer
        SBuildTerrainTextureInfo & rInfo = rTask.arrSrcTextures[buildItem.nTerrainLayerId];
        assert(rInfo.nTexId_Diff);

        // set shader
        CShader *pSH( CShaderMan::m_ShaderAmbientOcclusion );
        static CCryName TechName0 = "BuildTerrainTexture_DetailLayerON";
        static CCryName TechName1 = "BuildTerrainTexture_DetailLayerOFF";
        static CCryName TechName2 = "BuildTerrainTexture_DetailLayerON_MixMask";
        static CCryName TechName3 = "BuildTerrainTexture_DetailLayerOFF_MixMask";
        static CCryName TechName4 = "BuildTerrainTexture_DetailLayerON_HM";
        static CCryName TechName5 = "BuildTerrainTexture_DetailLayerOFF_HM";
        static CCryName TechName6 = "BuildTerrainTexture_DetailLayerON_MixMask_HM";
        static CCryName TechName7 = "BuildTerrainTexture_DetailLayerOFF_MixMask_HM";

        pSH->FXSetTechnique(rTask.bHeightMapMode ?
          (rInfo.pMatTerrainLayer ? (rTask.bMixMask ? TechName6 : TechName4) : (rTask.bMixMask ? TechName7 : TechName5)) :
          (rInfo.pMatTerrainLayer ? (rTask.bMixMask ? TechName2 : TechName0) : (rTask.bMixMask ? TechName3 : TechName1))
          );

        uint32 nPasses = 0;         
        pSH->FXBegin( &nPasses, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES);

        // set states
        FX_DisableATOC();
        D3DSetCull(eCULL_None);

#if defined (DIRECT3D9) && defined (CUSTOM_BLEND)
        int newState = GS_NODEPTHTEST | GS_BLSRC_ONEMINUSDSTALPHA | GS_BLDST_DSTALPHA;
        m_pd3dDevice->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_ONE); 
        m_pd3dDevice->SetRenderState(D3DRS_DESTBLENDALPHA,D3DBLEND_ONE); 
#else
        int newState = GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
#endif

        pSH->FXBeginPass( 0 );

        { // set base textures
          CTexture * pTerrTex0 = rInfo.nTexId_Diff ? CTexture::GetByID(rInfo.nTexId_Diff) : CTexture::s_ptexGray;
          CTexture * pTerrTex1 = rInfo.nTexId_Bump ? CTexture::GetByID(rInfo.nTexId_Bump) : CTexture::s_ptexFlatBump;
          CTexture * pTerrTex2 = rInfo.nTexId_Spec ? CTexture::GetByID(rInfo.nTexId_Spec) : CTexture::s_ptexWhite;
          pTerrTex0->Apply( 0, nTexStateIdRepeat );
          pTerrTex1->Apply( 1, nTexStateIdRepeat );
          pTerrTex2->Apply( 2, nTexStateIdRepeat );
        }

        { // set detail layer textures
          CTexture * pITex_Diff = CTexture::s_ptexGray;
          SEfResTexture * pTex_Diff = 0;

          CTexture * pITex_Bump = CTexture::s_ptexFlatBump;
          CTexture * pITex_Spec = CTexture::s_ptexWhite;
          CTexture * pITex_Detail = CTexture::s_ptexGray;

          SShaderItem * pShaderItem = 0;

          if(IMaterial * pMaterial = rInfo.pMatTerrainLayer)
          {
            pShaderItem = &pMaterial->GetShaderItem(0);

            if(SEfResTexture * pTex_Bump = pShaderItem->m_pShaderResources->GetTexture(EFTT_BUMP))
              if(pTex_Bump->m_Sampler.m_pITex)
                pITex_Bump = (CTexture*)pTex_Bump->m_Sampler.m_pITex;

            if(SEfResTexture * pTex_Spec = pShaderItem->m_pShaderResources->GetTexture(EFTT_GLOSS))
              if(pTex_Spec->m_Sampler.m_pITex)
                pITex_Spec = (CTexture*)pTex_Spec->m_Sampler.m_pITex;

            if(SEfResTexture * pTex_Detail = pShaderItem->m_pShaderResources->GetTexture(EFTT_DETAIL_OVERLAY))
              if(pTex_Detail->m_Sampler.m_pITex)
                pITex_Detail = (CTexture*)pTex_Detail->m_Sampler.m_pITex;

            if(pTex_Diff = pShaderItem->m_pShaderResources->GetTexture(EFTT_DIFFUSE))
              if(pTex_Diff->m_Sampler.m_pITex)
                pITex_Diff = (CTexture*)pTex_Diff->m_Sampler.m_pITex;
          }

          pITex_Diff->Apply( 3, nTexStateIdRepeat );
          pITex_Bump->Apply( 4, nTexStateIdRepeat );
          pITex_Spec->Apply( 5, nTexStateIdRepeat );
          pITex_Detail->Apply( 6, nTexStateIdRepeat );

          int nTexStateCopy = CTexture::GetTexState(STexState( FILTER_POINT, true ));
          int nTexStateBilinear = CTexture::GetTexState(STexState( FILTER_LINEAR, true ));
          if(buildItem.nRemeshTexId[0])
          { // set base textures
            for(int t=0; t<VOX_TEX_NUM_SRC; t++)
            {
              CTexture * pTerrTex0 = CTexture::GetByID(buildItem.nRemeshTexId[t]);
              pTerrTex0->Apply( 7+t, (t==2 && rTask.bHeightMapMode) ? nTexStateBilinear : nTexStateCopy );
            }
          }
          else
          {
            for(int t=0; t<VOX_TEX_NUM_SRC; t++)
            {
              CTexture::s_ptexBlack->Apply( 7+t, nTexStateCopy );
            }
          }

/*          {
            static CCryName paramName("BuildTerrainTextureParams2_PS");
            Vec4 vTextureAtlasInfo(0,0,0,0);
            if(pTex_Diff)
            {
              vTextureAtlasInfo.x = pTex_Diff->m_TexModificator.m_Offs[0];
              vTextureAtlasInfo.y = pTex_Diff->m_TexModificator.m_Offs[1];
              vTextureAtlasInfo.z = pTex_Diff->m_TexModificator.m_Tiling[0];
              vTextureAtlasInfo.w = pTex_Diff->m_TexModificator.m_Tiling[1];
            }
            pSH->FXSetPSFloat(paramName, &vTextureAtlasInfo, 1);
          }*/

          /*{
            Vec4 vDiffuseColor(0,0,0,0);
            if(pShaderItem && pShaderItem->m_pShaderResources)
            {
              vDiffuseColor.x = pShaderItem->m_pShaderResources->GetDiffuseColor().r;
              vDiffuseColor.y = pShaderItem->m_pShaderResources->GetDiffuseColor().g;
              vDiffuseColor.z = pShaderItem->m_pShaderResources->GetDiffuseColor().b;
              vDiffuseColor.w = pShaderItem->m_pShaderResources->GetOpacity();
            }
            static CCryName paramName("BuildTerrainTextureParams3_PS");
            pSH->FXSetPSFloat(paramName, (Vec4*)&vDiffuseColor, 1);
          }*/

/*          {
            Vec4 vSpec(0,0,0,0);
            if(pShaderItem && pShaderItem->m_pShaderResources)
            {
              vSpec.x = pShaderItem->m_pShaderResources->GetSpecularColor().Luminance();
              vSpec.y = pShaderItem->m_pShaderResources->GetSpecularShininess();
            }
            static CCryName paramName("BuildTerrainTextureParams4_PS");
            pSH->FXSetPSFloat(paramName, (Vec4*)&vSpec, 1);
          }*/

          // set params
          {
            static CCryName paramName("BuildTerrainTextureParams0_PS");
            Vec4 vConst(rInfo.filterColor.r, rInfo.filterColor.g, rInfo.filterColor.b, rInfo.fTiling);
            pSH->FXSetPSFloat(paramName, &vConst, 1);
          }

          {
            static CCryName paramName("BuildTerrainTextureParams1_PS");
            Vec4 vConst(rInfo.fSpecularAmount, rTask.fMeshNodeSize, buildItem.nRemeshTexId[0] ? 1.f : 0.f, 0);
            if(pTex_Diff)
              vConst.w = pTex_Diff->m_TexModificator.m_Tiling[0];
            pSH->FXSetPSFloat(paramName, &vConst, 1);
          }

          {
            static CCryName paramName("BuildTerrainTextureParams2_PS");
            Vec4 vConst(0, (float)buildItem.nTerrainLayerId, (float)rTask.nSrcTexDimX, (float)rTask.nSrcTexDimY);
            if(pShaderItem && pShaderItem->m_pShader && pShaderItem->m_pShader->GetFlags2() & EF2_DETAILBUMPMAPPING)
              vConst.x = 1.f;
            pSH->FXSetPSFloat(paramName, &vConst, 1);
          }

          {
            static CCryName paramName("BuildTerrainTextureParams3_PS");

            Vec3 vDiffuseColor(1,1,1);
            if(pShaderItem)
            {
              vDiffuseColor.x = pShaderItem->m_pShaderResources->GetDiffuseColor().r;
              vDiffuseColor.y = pShaderItem->m_pShaderResources->GetDiffuseColor().g;
              vDiffuseColor.z = pShaderItem->m_pShaderResources->GetDiffuseColor().b;
            }

            Vec4 vConst(vDiffuseColor.x, vDiffuseColor.y, vDiffuseColor.z, rTask.bOverlayBlend ? 1.f : 0.f);
            pSH->FXSetPSFloat(paramName, &vConst, 1);
          }
          
          {
            static CCryName paramName("BuildTerrainTextureParams_VS");
            Vec4 vConst(rTask.vMeshWSPos.x, rTask.vMeshWSPos.y, rTask.vMeshWSPos.z, (float)buildItem.nTerrainLayerId);
            pSH->FXSetVSFloat(paramName, (Vec4*) &vConst, 1);       
          }

          {
            static CCryName paramName("BuildTerrainTextureParams1_VS");
            Vec4 vConst(rTask.vBoxMin.x, rTask.vBoxMin.y, rTask.vBoxMin.z, 0);
            if(pShaderItem)
              SShaderParam::GetValue("DetailTextureStrength", &pShaderItem->m_pShaderResources->GetParameters(), &vConst.x, 3);
            pSH->FXSetVSFloat(paramName, (Vec4*) &vConst, 1);       
          }

          {
            static CCryName paramName("BuildTerrainTextureParams4_PS");
            Vec4 vConst(rTask.fTexRangeScale, 0.f, 0.f, 0.f);
            pSH->FXSetPSFloat(paramName, &vConst, 1);
          }
        }

        EF_SetState( newState );
        FX_Commit();

        // render mesh
        pRM->DrawImmediately();

        // end
        pSH->FXEndPass();
        pSH->FXEnd();
        EF_SetState( 0 );
        FX_Commit();
      }
      else
        assert(!"Undefined item");
    }

#if defined (DIRECT3D9) && defined (CUSTOM_BLEND)
    m_pd3dDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
#endif

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Pop();
    PROFILE_SHADER_END;

    RT_SetViewport(TempX, TempY, TempWidth, TempHeight);

    FX_PopRenderTarget(0);
    FX_PopRenderTarget(1);

    // Dilate
    static CCryName strTechName = "DilateTerrainTexture";
    Set2DMode(true, 1,1);

    SDynTexture2 * pDstRT_Diff = (SDynTexture2 *)rTask.pDstTex_Diff;
    SDynTexture2 * pDstRT_Bump = (SDynTexture2 *)rTask.pDstTex_Bump;

    pDstRT_Diff->Update(rTask.nDstTexDimX, rTask.nDstTexDimY);
    pDstRT_Bump->Update(rTask.nDstTexDimX/rTask.nBumpDownScale, rTask.nDstTexDimY/rTask.nBumpDownScale);






























    {
      StretchRect_BTT(pRT_Diff->m_pTexture, pDstRT_Diff, strTechName);
      StretchRect_BTT(pRT_Bump->m_pTexture, pDstRT_Bump, strTechName);
    }

    Set2DMode(false, 1,1);

    CV_r_texturesstreamingsync = nPrevStr;
    CV_r_shadersasynccompiling = nPrevAsync;
  }

  RT_SetViewport(TempX, TempY, TempWidth, TempHeight);
  
	//ResetToDefault();
	FX_ResetPipe();
}

bool CD3D9Renderer::FX_PrepareTerrainAOTarget(t_arrDeferredMeshIndBuff& indBuff, t_arrDeferredMeshVertBuff& vertBuff)
{
	PROFILE_LABEL_PUSH( "TERRAIN_AO" );

	Matrix44 mCurComposite = *(m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->GetTop()) * *(m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop());

  for(int i=0; i<m_RP.m_TerrainAONodes[m_RP.m_nProcessThreadID].Count(); i++)
  {
		bool bUseStencil = false;
		// stencil pass
		if(CV_r_TerrainAO&1)
		{
			const AABB& stencilBox = m_RP.m_TerrainAONodes[m_RP.m_nProcessThreadID][i].stencilBox;

			Vec3 vBoundRectMin(0.0f, 0.0f, 0.0f), vBoundRectMax(1.0f, 1.0f, 1.0f);

			CRenderCamera& rc = m_RP.m_TI[m_RP.m_nProcessThreadID].m_rcam;

			//if(!stencilBox.IsContainPoint(rc.Orig))
			{
				PROFILE_SHADER_START;
				CShader *pSH( CShaderMan::m_ShaderShadowMaskGen );
				uint32 nPasses = 0;         
				static CCryName TechName0 = "DeferredShadowPass";
				pSH->FXSetTechnique(TechName0);
				pSH->FXBegin( &nPasses, FEF_DONTSETSTATES );

				m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Push();
				Matrix34 mLocal;
				mLocal.SetIdentity();
				mLocal.SetScale(stencilBox.max-stencilBox.min,stencilBox.min);
				Matrix44 mLocalTransposed = GetTransposed44(Matrix44(mLocal));
				m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->MultMatrixLocal(&mLocalTransposed);

				FX_StencilCull(-1, indBuff, vertBuff, pSH);

				m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Pop();
				pSH->FXEnd();

				CalcAABBScreenRect(stencilBox, rc, mCurComposite, &vBoundRectMin,  &vBoundRectMax);

				// if more than half a screen is filled with the sector, refresh HiS
				if(vBoundRectMin.GetSquaredDistance2D(vBoundRectMax) > 0.7f * 0.7f)
				{
					// Refresh HiStencil
					FX_StencilRefresh(STENC_FUNC(FSS_STENCFUNC_EQUAL), m_nStencilMaskRef, 0xFFFFFFFF);
					FX_StencilTestCurRef(true);
				}

				bUseStencil = true;

				PROFILE_SHADER_END;
			}
    }

    if(CV_r_TerrainAO&2)
    { // terrain AO pass
      PROFILE_SHADER_START;

      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Push();
      mathMatrixOrthoOffCenterLH( m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop(), 0, 1, 0, 1, -1, 1 );
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Push();
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->LoadIdentity();

      static CCryName TechName1 = "Deferred_TerrainAO_Pass";
      FX_DeferredShadowPassAO( TechName1, &m_RP.m_TerrainAONodes[m_RP.m_nProcessThreadID][i], &mCurComposite, -1, NULL, bUseStencil );

      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();
      m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Pop();

			if(bUseStencil)
				FX_StencilTestCurRef(false);

			PROFILE_SHADER_END;
    }
  }

  m_RP.m_TerrainAONodes[m_RP.m_nProcessThreadID].Clear();

	PROFILE_LABEL_POP( "TERRAIN_AO" );

  return true;
}

// return number of vertices to add
int ClipEdge(const Vec3 & v1, const Vec3 & v2, const Plane & ClipPlane, Vec3 & vRes1, Vec3 & vRes2)
{
  float d1 = -ClipPlane.DistFromPlane(v1);
  float d2 = -ClipPlane.DistFromPlane(v2);
  if(d1<0 && d2<0)
    return 0; // all clipped = do not add any vertices

  if(d1>=0 && d2>=0)
  {
    vRes1 = v2;
    return 1; // both not clipped - add second vertex
  }

  // calculate new vertex
  Vec3 vIntersectionPoint = v1 + (v2-v1)*(Ffabs(d1)/(Ffabs(d2)+Ffabs(d1)));

#ifdef _DEBUG
  float fNewDist = -ClipPlane.DistFromPlane(vIntersectionPoint);
  assert(Ffabs(fNewDist)<0.01f);
#endif

  if(d1>=0 && d2<0)
  { // from vis to no vis
    vRes1 = vIntersectionPoint;
    return 1;
  }
  else if(d1<0 && d2>=0)
  { // from not vis to vis
    vRes1 = vIntersectionPoint;
    vRes2 = v2;
    return 2;
  }

  assert(0);
  return 0;
}

void ClipPolygon(PodArray<Vec3> * pPolygon, const Plane & ClipPlane)
{
  static PodArray<Vec3> PolygonOut; // Keep this list static to not perform reallocation every time.
  PolygonOut.Clear();
  // clip edges, make list of new vertices
  for(int i=0; i<pPolygon->Count(); i++)
  {
    Vec3 vNewVert1(0,0,0), vNewVert2(0,0,0);
    if(int nNewVertNum = ClipEdge(pPolygon->GetAt(i), pPolygon->GetAt((i+1)%pPolygon->Count()), ClipPlane, vNewVert1, vNewVert2))
    {
      PolygonOut.Add(vNewVert1);
      if(nNewVertNum>1)
        PolygonOut.Add(vNewVert2);
    }
  }

  // check result
  for(int i=0; i<PolygonOut.Count(); i++)
  {
    float d1 = -ClipPlane.DistFromPlane(PolygonOut.GetAt(i));
    assert(d1>=-0.01f);
  }

  assert(PolygonOut.Count()==0 || PolygonOut.Count() >= 3);

  pPolygon->Clear();
  pPolygon->AddList( PolygonOut );
}

void CheckTriangle(Vec3 * pVerts3d, int i1, int i2, int i3, const Plane & nearPlane, Vec3 & vMin, Vec3 & vMax, const Matrix44 & mViewProj)
{
  // make polygon
  static PodArray<Vec3> arrTriangle;
  arrTriangle.Clear();
  arrTriangle.Add(pVerts3d[i1]);
  arrTriangle.Add(pVerts3d[i2]);
  arrTriangle.Add(pVerts3d[i3]);

  // clip by near plane
  ClipPolygon(&arrTriangle, nearPlane);

  // check 2d bounds
  for(int i=0; i<arrTriangle.Count(); i++)
  {
    Vec4 vScreenPoint = Vec4(arrTriangle[i], 1.0) * mViewProj;

    //projection space clamping
    vScreenPoint.w = max(vScreenPoint.w, 0.00000000000001f);
    vScreenPoint.x = max(vScreenPoint.x, -(vScreenPoint.w));
    vScreenPoint.x = min(vScreenPoint.x, vScreenPoint.w);
    vScreenPoint.y = max(vScreenPoint.y, -(vScreenPoint.w));
    vScreenPoint.y = min(vScreenPoint.y, vScreenPoint.w);

    //NDC
    vScreenPoint /= vScreenPoint.w;

    //output coords
    //generate viewport (x=0,y=0,height=1,width=1)
    Vec3 vWin;
    vWin.x = (1 + vScreenPoint.x)/ 2;
    vWin.y = (1 + vScreenPoint.y)/ 2;  //flip coords for y axis
    vWin.z = vScreenPoint.z;

    assert(vWin.x>=0.0f && vWin.x<=1.0f);
    assert(vWin.y>=0.0f && vWin.y<=1.0f);

    vMin.x = min( vMin.x,vWin.x );
    vMin.y = min( vMin.y,vWin.y );
    vMin.z = min( vMin.z,vWin.z );
    
    vMax.x = max( vMax.x,vWin.x );
    vMax.y = max( vMax.y,vWin.y );
    vMax.z = max( vMax.z,vWin.z );
  }
}

// brute force clip and check all triangles of AABB, TODO: use edges
void ClipAABBByPlane(const AABB & objBox, const Plane & nearPlane, const Vec3 & vCamPos, Vec3 & vMin, Vec3 & vMax, const Matrix44 & mViewProj)
{
  Vec3 arrVerts3d[8] = 
  {
    Vec3(objBox.min.x,objBox.min.y,objBox.min.z),
    Vec3(objBox.min.x,objBox.max.y,objBox.min.z),
    Vec3(objBox.max.x,objBox.min.y,objBox.min.z),
    Vec3(objBox.max.x,objBox.max.y,objBox.min.z),
    Vec3(objBox.min.x,objBox.min.y,objBox.max.z),
    Vec3(objBox.min.x,objBox.max.y,objBox.max.z),
    Vec3(objBox.max.x,objBox.min.y,objBox.max.z),
    Vec3(objBox.max.x,objBox.max.y,objBox.max.z)
  };

  CheckTriangle(arrVerts3d, 2, 3, 6, nearPlane, vMin, vMax, mViewProj);
  CheckTriangle(arrVerts3d, 6, 3, 7, nearPlane, vMin, vMax, mViewProj);
  CheckTriangle(arrVerts3d, 1, 0, 4, nearPlane, vMin, vMax, mViewProj);
  CheckTriangle(arrVerts3d, 1, 4, 5, nearPlane, vMin, vMax, mViewProj);
  CheckTriangle(arrVerts3d, 3, 1, 7, nearPlane, vMin, vMax, mViewProj);
  CheckTriangle(arrVerts3d, 7, 1, 5, nearPlane, vMin, vMax, mViewProj);
  CheckTriangle(arrVerts3d, 0, 2, 4, nearPlane, vMin, vMax, mViewProj);
  CheckTriangle(arrVerts3d, 4, 2, 6, nearPlane, vMin, vMax, mViewProj);
  CheckTriangle(arrVerts3d, 5, 4, 6, nearPlane, vMin, vMax, mViewProj);
  CheckTriangle(arrVerts3d, 5, 6, 7, nearPlane, vMin, vMax, mViewProj);
  CheckTriangle(arrVerts3d, 0, 1, 2, nearPlane, vMin, vMax, mViewProj);
  CheckTriangle(arrVerts3d, 2, 1, 3, nearPlane, vMin, vMax, mViewProj);
}

void CD3D9Renderer::CalcAABBScreenRect(const AABB & aabb, const CRenderCamera& RCam, const Matrix44& mViewProj, Vec3* pvMin,  Vec3* pvMax)
{
  * pvMin = Vec3(1.f,1.f,1.f);
  * pvMax = Vec3(0.f,0.f,0.f);

  ClipAABBByPlane(aabb, 
    *GetCamera().GetFrustumPlane(FR_PLANE_NEAR), GetCamera().GetPosition(), 
    *pvMin, *pvMax, mViewProj);
}

byte * CD3D9Renderer::GetTextureSubImageData32(byte * pDataOut, int nDataOutSize, int nX, int nY, int nW, int nH, CTexture * pTex)
{
#if defined (DIRECT3D9)

  LPDIRECT3DSURFACE9 pTargSurf, pSrcSurf;

  m_pd3dDevice->CreateOffscreenPlainSurface(pTex->GetWidth(), pTex->GetHeight(), pTex->m_pPixelFormat->DeviceFormat, D3DPOOL_SYSTEMMEM, &pTargSurf, NULL);
  pSrcSurf = (LPDIRECT3DSURFACE9)pTex->GetSurface(0, 0);
  gcpRendD3D->m_pd3dDevice->GetRenderTargetData(pSrcSurf, pTargSurf);

  D3DLOCKED_RECT d3dlr;
  HRESULT hr = pTargSurf->LockRect(&d3dlr, NULL, D3DLOCK_READONLY);

  assert(nW*nH*4 == nDataOutSize);

  byte *pDataIn = (byte *)d3dlr.pBits;
  for (int y=0; y<nH; y++)
  {
    byte *pSrc = &pDataIn[d3dlr.Pitch*(y+nY)] + nX*4;
    byte *pDst = &pDataOut[nW*y*4];
    memcpy(pDst, pSrc, nW*4);
  }

  pTargSurf->UnlockRect();
  SAFE_RELEASE(pTargSurf);
  SAFE_RELEASE(pSrcSurf);

  return pDataOut;

#else

  assert(!"Not implemented for dx10");
  return NULL;

#endif
}

// Used by StretchRectAO
enum E_SR_AO
{
	E_SRAO_NORM				= 0,
	E_SRAO_FIRST_3X3	= 1,
	E_SRAO_SECOND_3X3 = 2,
	E_SRAO_MIN				= 3,
	E_SRAO_NUM		
};

// methods defined in E_SR_AO
void StretchRectAO(CTexture *pSrc, CTexture *pDst, uint32 method) 
{
	assert( method < E_SRAO_NUM );

	if(!pSrc || !pDst) 
	{
		return;
	}

	// Get current viewport
	int iTempX, iTempY, iWidth, iHeight;
	gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

	bool bResample = 0;
	bool b3x3 = (method == E_SRAO_FIRST_3X3) || (method == E_SRAO_SECOND_3X3);

	if(method != E_SRAO_MIN && pSrc->GetWidth()!=pDst->GetWidth() && pSrc->GetHeight()!=pDst->GetHeight())
	{
		bResample = 1;
	}

	gcpRendD3D->FX_PushRenderTarget(0, pDst, NULL); 

	if(!bResample)
	{
		static CCryName TechName("TextureToTexture");
		SPostEffectsUtils::ShBeginPass(CShaderMan::m_shPostEffects, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
	}
	else
		switch (method)
	{
		case E_SRAO_NORM :
			{
				static CCryName TechName("TextureToTextureResampledAO");
				SPostEffectsUtils::ShBeginPass(CShaderMan::m_shPostEffects, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
			} break;
		case E_SRAO_FIRST_3X3 :
			{
				static CCryName TechName("TextureToTextureResampledAO_3x3_First");
				SPostEffectsUtils::ShBeginPass(CShaderMan::m_shPostEffects, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
			} break;
		case E_SRAO_SECOND_3X3 :
			{
				static CCryName TechName("TextureToTextureResampledAO_3x3_Second");
				SPostEffectsUtils::ShBeginPass(CShaderMan::m_shPostEffects, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
			} break;
		case E_SRAO_MIN :
			{
				static CCryName TechName("TextureToTextureMin");
				SPostEffectsUtils::ShBeginPass(CShaderMan::m_shPostEffects, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
			} break;
	}

	gRenDev->EF_SetState(GS_NODEPTHTEST);   

	// Get sample size ratio (based on empirical "best look" approach)
	float fSampleSize = ((float)pSrc->GetWidth()/(float)pDst->GetWidth()) * 0.5f;

	// Set samples position
	float s1 = fSampleSize / (float) pSrc->GetWidth();  // 2.0 better results on lower res images resizing        
	float t1 = fSampleSize / (float) pSrc->GetHeight();       

	// Use rotated grid
	Vec4 pParams0=Vec4( s1*0.5f,  t1*0.5f,-s1*0.5f,  t1*0.5f); 
	Vec4 pParams1=Vec4(-s1*0.5f, -t1*0.5f, s1*0.5f, -t1*0.5f);  

	static CCryName Param1Name("texToTexParams0");
	static CCryName Param2Name("texToTexParams1");

	if((bResample && !b3x3) || method == E_SRAO_MIN)
	{
		CShaderMan::m_shPostEffects->FXSetPSFloat(Param1Name, &pParams0, 1);        
		CShaderMan::m_shPostEffects->FXSetPSFloat(Param2Name, &pParams1, 1); 
	}
	else
	{
		CShaderMan::m_shPostEffects->FXSetVSFloat(Param1Name, &pParams0, 1);        
		CShaderMan::m_shPostEffects->FXSetVSFloat(Param2Name, &pParams1, 1); 
	}

	SPostEffectsUtils::SetTexture(pSrc, 0, FILTER_POINT);    
	SPostEffectsUtils::SetTexture(CTexture::s_ptexSceneNormalsMap, 1, FILTER_POINT);    


	SPostEffectsUtils::DrawFullScreenQuad(pDst->GetWidth(), pDst->GetHeight());

	SPostEffectsUtils::ShEndPass();

	// Restore previous viewport
	gcpRendD3D->FX_PopRenderTarget(0);
	gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);        
}

// terrain texture generation on GPU
void StretchRect_BTT(CTexture *pSrc, IDynTexture * pDst, CCryName & strTechName) 
{
	if(!pSrc || !pDst) 
		return;

	// Get current viewport
	int iTempX, iTempY, iWidth, iHeight;
	gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

	uint32 nX1, nY1, nW1, nH1;
	pDst->GetImageRect(nX1, nY1, nW1, nH1);

	pDst->SetRT(0, true, gcpRendD3D->FX_GetDepthSurface(nW1, nH1, false));

	SPostEffectsUtils::ShBeginPass(CShaderMan::m_shPostEffects, strTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

	gRenDev->EF_SetState(GS_NODEPTHTEST);   

	float s = 1.f / (float) pSrc->GetWidth();
	float t = 1.f / (float) pSrc->GetHeight();

	Vec4 pParams0=Vec4( s,  t,  0,  0 );
	Vec4 pParams1=Vec4( 0,  0,  0,  0 );

	static CCryName Param1Name("texToTexParams0");
	static CCryName Param2Name("texToTexParams1");
	CShaderMan::m_shPostEffects->FXSetVSFloat(Param1Name, &pParams0, 1);        
	CShaderMan::m_shPostEffects->FXSetVSFloat(Param2Name, &pParams1, 1); 

	SPostEffectsUtils::SetTexture(pSrc, 0, FILTER_LINEAR);    

	SPostEffectsUtils::DrawFullScreenQuad(pSrc->GetWidth(), pSrc->GetHeight());

	SPostEffectsUtils::ShEndPass();

	gcpRendD3D->FX_PopRenderTarget(0);

	gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);        
}

void StretchRect_VoxTerrain(SDynTexture ** pSrc, int nSrcNum, SDynTexture * pDst, CCryName & strTechName)
{
	if(!pSrc || !pDst) 
		return;

	// Get current viewport
	int iTempX, iTempY, iWidth, iHeight;
	gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

	uint32 nX1, nY1, nW1, nH1;
	pDst->GetImageRect(nX1, nY1, nW1, nH1);

	pDst->SetRT(0, true, gcpRendD3D->FX_GetDepthSurface(nW1, nH1, false));

	SPostEffectsUtils::ShBeginPass(CShaderMan::m_shPostEffects, strTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

	gRenDev->EF_SetState(GS_NODEPTHTEST);   

	float s = 1.f / (float) pSrc[0]->GetWidth();
	float t = 1.f / (float) pSrc[0]->GetHeight();

	Vec4 pParams0=Vec4( s,  t,  0,  0 );
	Vec4 pParams1=Vec4( 0,  0,  0,  0 );

	static CCryName Param1Name("texToTexParams0");
	static CCryName Param2Name("texToTexParams1");
	CShaderMan::m_shPostEffects->FXSetVSFloat(Param1Name, &pParams0, 1);        
	CShaderMan::m_shPostEffects->FXSetVSFloat(Param2Name, &pParams1, 1); 

	for(int i=0; i<nSrcNum; i++)
		SPostEffectsUtils::SetTexture(pSrc[i]->m_pTexture, i, FILTER_POINT);    

	SPostEffectsUtils::DrawFullScreenQuad(pSrc[0]->GetWidth(), pSrc[0]->GetHeight());

	SPostEffectsUtils::ShEndPass();

	gcpRendD3D->FX_PopRenderTarget(0);

	gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);        
}
