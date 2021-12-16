/*=============================================================================
D3DFXPipeline.cpp : Direct3D specific FX shaders rendering pipeline.
Copyright (c) 2001 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include <I3DEngine.h>
#include <IEntityRenderState.h>

//====================================================================================

HRESULT CD3D9Renderer::FX_SetVStream(int nID, void *pB, uint32 nOffs, uint32 nStride, uint32 nFreq)
{
  D3DVertexBuffer *pVB = (D3DVertexBuffer *)pB;
  HRESULT h = S_OK;
  if (m_RP.m_VertexStreams[nID].pStream != pVB || m_RP.m_VertexStreams[nID].nOffset != nOffs)
  {
    m_RP.m_VertexStreams[nID].pStream = pVB;
    m_RP.m_VertexStreams[nID].nOffset = nOffs;
#if defined (DIRECT3D9) || defined(OPENGL)
    h = m_pd3dDevice->SetStreamSource(nID, pVB, nOffs, nStride);
#elif defined (DIRECT3D10)
    m_pd3dDeviceContext->IASetVertexBuffers(nID, 1, &pVB, &nStride, &nOffs);
#endif
  }
#if defined (DIRECT3D9) || defined(OPENGL)
  if (nFreq != -1 && m_RP.m_VertexStreams[nID].nFreq != nFreq)
  {
    m_RP.m_VertexStreams[nID].nFreq = nFreq;
#if !defined(XENON) && !defined(PS3)
    h = m_pd3dDevice->SetStreamSourceFreq(nID, nFreq);
#endif
  }
#endif
  assert(h == S_OK);
  return h;
}
HRESULT CD3D9Renderer::FX_SetIStream(void* pB)
{
  D3DIndexBuffer *pIB = (D3DIndexBuffer *)pB;
  HRESULT h = S_OK;
  if (m_RP.m_pIndexStream != pIB)
  {
    m_RP.m_pIndexStream = pIB;
#if defined (DIRECT3D9) || defined(OPENGL)
    h = m_pd3dDevice->SetIndices(pIB);
#elif defined (DIRECT3D10)
    m_pd3dDeviceContext->IASetIndexBuffer(pIB, DXGI_FORMAT_R16_UINT, 0);
#endif
    assert(h == S_OK);
  }
  return h;
}

#if defined (DIRECT3D9) || defined(OPENGL)
namespace D3D9R2VBHelper
{
	enum eR2VBConstants
	{
		R2VB_GLB_ENA_CMD = 0x0,
		R2VB_VS2SM_CMD = 0x1,
		// R2VB Command Token
		R2VB_TOK_CMD_SHFT = 24,
		R2VB_TOK_CMD_MSK = 0x0F000000,
		R2VB_TOK_CMD_MAG = 0x70FF0000,
		R2VB_TOK_CMD_MAT = 0xFFFF0000,
		R2VB_TOK_PLD_MSK = 0x0000FFFF,
		R2VB_GLB_ENA_MSK = 0x1,
		R2VB_VS2SM_STRM_MSK = 0xF,
		R2VB_VS2SM_SMP_SHFT = 0x4,
		R2VB_VS2SM_SMP_MSK = 0x7,
		// R2VB enums
		R2VB_VSMP_OVR_DMAP = 0, // override stream with dmap sampler
		R2VB_VSMP_OVR_VTX0 = 1, // override stream with vertex texture 0 sampler
		R2VB_VSMP_OVR_VTX1 = 2, // override stream with vertex texture 1 sampler
		R2VB_VSMP_OVR_VTX2 = 3, // override stream with vertex texture 2 sampler
		R2VB_VSMP_OVR_VTX3 = 4, // override stream with vertex texture 3 sampler
		R2VB_VSMP_OVR_DIS = 5, // disable stream override
		R2VB_VSMP_OVR_NUM = 6, //
		R2VB_VSMP_NUM = 5, // 5 available texture samplers
	};

	DWORD r2vbToken_Set(DWORD cmd, DWORD payload)
	{
		DWORD cmd_token = (cmd << R2VB_TOK_CMD_SHFT) & R2VB_TOK_CMD_MSK;
		DWORD pld_data = payload & R2VB_TOK_PLD_MSK;
		return (R2VB_TOK_CMD_MAG | cmd_token | pld_data);
	}
	DWORD r2vbGlbEnable_Set(BOOL ena)
	{
		DWORD payload = ena & R2VB_GLB_ENA_MSK;
		DWORD dw = r2vbToken_Set(R2VB_GLB_ENA_CMD, payload);
		return dw;
	}
	DWORD r2vbVStrm2SmpMap_Set(DWORD str, DWORD smp)
	{
		DWORD sampler = (smp & R2VB_VS2SM_SMP_MSK) << R2VB_VS2SM_SMP_SHFT;
		DWORD stream = (str & R2VB_VS2SM_STRM_MSK);
		DWORD payload = sampler | stream;
		DWORD dw = r2vbToken_Set(R2VB_VS2SM_CMD, payload);
		return dw;
	}
}
#endif 

HRESULT CD3D9Renderer::FX_SetTextureAsVStream( int nID, CTexture *pVBTexture, uint32 nStride )
{
	if(!(m_Features & RFT_HW_R2VB) || (pVBTexture && (!(pVBTexture->GetFlags() & FT_USAGE_VERTEX_BUFFER) || pVBTexture->GetDevTexture() == NULL))	|| nID > 3)
	{
		assert(0);
		return E_FAIL;
	}

	const BOOL bSetVB = pVBTexture != NULL;
















#if defined (DIRECT3D9) || defined(OPENGL)
	// Enable render to vertex buffer extension
	HRESULT hr = m_pd3dDevice->SetRenderState(D3DRS_POINTSIZE, D3D9R2VBHelper::r2vbGlbEnable_Set(bSetVB));
	assert(SUCCEEDED(hr));
	if(bSetVB)
	{
		// Setup stream nID R2VB data
		hr = m_pd3dDevice->SetTexture(D3DVERTEXTEXTURESAMPLER0 + nID, pVBTexture->GetDevTexture()->Get2DTexture());
		assert(SUCCEEDED(hr));
		// Tell the driver that stream nID is to be fetched from the DMAP texture
		hr = m_pd3dDevice->SetRenderState(D3DRS_POINTSIZE, D3D9R2VBHelper::r2vbVStrm2SmpMap_Set(nID, D3D9R2VBHelper::R2VB_VSMP_OVR_VTX0 + nID));
		assert(SUCCEEDED(hr));
	}
	else
	{
		hr = m_pd3dDevice->SetTexture(D3DVERTEXTEXTURESAMPLER0 + nID, NULL);
		assert(SUCCEEDED(hr));
		// Stream 1 restored to regular vertex buffer mode
		hr = m_pd3dDevice->SetRenderState(D3DRS_POINTSIZE, D3D9R2VBHelper::r2vbVStrm2SmpMap_Set(1, D3D9R2VBHelper::R2VB_VSMP_OVR_DIS));
		assert(SUCCEEDED(hr));
	}
#elif defined (DIRECT3D10)
	// the feature is not supported
#endif

	return S_OK;
}

HRESULT CD3D9Renderer::FX_SetVertexDeclaration(int StreamMask, EVertexFormat eVF)
{
  HRESULT hr;

  assert (eVF>=0 && eVF<eVF_Max);

	bool bMorph = (StreamMask&VSM_MORPHBUDDY)!=0;

	SOnDemandD3DVertexDeclarationCache *pDeclCache = &m_RP.m_D3DVertexDeclarationCache[(StreamMask&0xff)>>1][eVF][bMorph?1:0];

	if (!pDeclCache->m_pDeclaration)
  {
		SOnDemandD3DVertexDeclaration Decl;

		EF_OnDemandVertexDeclaration(Decl,(StreamMask&0xff)>>1, eVF, bMorph);

#if defined (DIRECT3D9) || defined(OPENGL)
    if(FAILED(hr = m_pd3dDevice->CreateVertexDeclaration(&Decl.m_Declaration[0], &pDeclCache->m_pDeclaration)))
      return hr;
#elif defined (DIRECT3D10)
		if(!CHWShader_D3D::m_pCurInstVS || !CHWShader_D3D::m_pCurInstVS->m_pShaderData || CHWShader_D3D::m_pCurInstVS->m_bFallback)
			return -1;
    assert(CHWShader_D3D::m_pCurInstVS->m_pShaderData);
    if(FAILED(hr = m_pd3dDevice->CreateInputLayout(&Decl.m_Declaration[0], Decl.m_Declaration.Num(), CHWShader_D3D::m_pCurInstVS->m_pShaderData, 
      CHWShader_D3D::m_pCurInstVS->m_nShaderByteCodeSize, &pDeclCache->m_pDeclaration)))
    {
      assert(SUCCEEDED(hr));
      return hr;
    }
#endif
  }
  D3DVertexDeclaration *pD3DDecl = pDeclCache->m_pDeclaration;










  if (m_pLastVDeclaration != pD3DDecl)
  {
#if defined (DIRECT3D9) || defined(OPENGL)
    m_pLastVDeclaration = pD3DDecl;
    return m_pd3dDevice->SetVertexDeclaration(pD3DDecl);
#elif defined (DIRECT3D10) 
    // Don't set input layout on fallback shader (crashes in DX11 NV driver)
    if (!CHWShader_D3D::m_pCurInstVS || CHWShader_D3D::m_pCurInstVS->m_bFallback)
       return -1;
    m_pLastVDeclaration = pD3DDecl;
    m_pd3dDeviceContext->IASetInputLayout(pD3DDecl);
#endif
  }
#if defined (DIRECT3D10)
  // Don't render fallback in DX11
  if (!CHWShader_D3D::m_pCurInstVS || !CHWShader_D3D::m_pCurInstPS || CHWShader_D3D::m_pCurInstVS->m_bFallback || CHWShader_D3D::m_pCurInstPS->m_bFallback)
    return -1;
#endif
  return S_OK;
}

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
void CD3D9Renderer::FX_TagVStreamAsDirty(int nID)
{
	m_RP.m_VertexStreams[nID].pStream = NULL;
	m_RP.m_VertexStreams[nID].nOffset = -1;
	m_RP.m_VertexStreams[nID].nFreq = -1;
}

void CD3D9Renderer::FX_TagIStreamAsDirty()
{
	m_RP.m_pIndexStream = NULL;
}

void CD3D9Renderer::FX_TagVertexDeclarationAsDirty()
{
	m_pLastVDeclaration = NULL;
}
#endif

// Clear buffers (color, depth/stencil)
void CD3D9Renderer::EF_ClearBuffers(uint32 nFlags, const ColorF *Colors, float fDepth)
{
  if (nFlags & FRT_CLEAR_FOGCOLOR)
    m_pNewTarget[0]->m_ReqColor = m_cClearColor;
  else
  if (nFlags & FRT_CLEAR_COLOR)
  {
    if (Colors)
      m_pNewTarget[0]->m_ReqColor = *Colors;
    else
    if(m_wireframe_mode>R_SOLID_MODE)
      m_pNewTarget[0]->m_ReqColor = ColorF(0.25f,0.5f,1.0f,0);
    else
      m_pNewTarget[0]->m_ReqColor = m_cClearColor;
  }

  if (nFlags & FRT_CLEAR_DEPTH)
    m_pNewTarget[0]->m_fReqDepth = fDepth;

	if (!(nFlags & FRT_CLEAR_IMMEDIATE))
    m_pNewTarget[0]->m_ClearFlags = 0;
  if (nFlags & FRT_CLEAR_DEPTH)
    m_pNewTarget[0]->m_ClearFlags |= D3DCLEAR_ZBUFFER;
  if (nFlags & FRT_CLEAR_COLOR)
    m_pNewTarget[0]->m_ClearFlags |= D3DCLEAR_TARGET;
  if (m_sbpp && (nFlags & FRT_CLEAR_STENCIL))
    m_pNewTarget[0]->m_ClearFlags |= D3DCLEAR_STENCIL;

	if (nFlags & FRT_CLEAR_IMMEDIATE)
		FX_SetActiveRenderTargets(true);
}

void CD3D9Renderer::FX_ClearRegion()
{
#if defined (DIRECT3D10)
  CRenderObject *pObj = m_RP.m_pCurObject;
  CShader *pSHSave = m_RP.m_pShader;
  SShaderTechnique *pSHT = m_RP.m_pCurTechnique;
  SShaderPass *pPass = m_RP.m_pCurPass;
  SRenderShaderResources *pShRes = m_RP.m_pShaderResources;

  m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags |= RBPF_IN_CLEAR;
  CShader *pSH = CShaderMan::m_ShaderCommon;
  uint32 nPasses = 0;
  pSH->FXSetTechnique("Clear");
  pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
  pSH->FXBeginPass(0);
  int nState = GS_NODEPTHTEST;
  if (m_pNewTarget[0]->m_ClearFlags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL))
  {
    nState = GS_DEPTHFUNC_GREAT;
    nState &= ~GS_NODEPTHTEST;
    nState |= GS_DEPTHWRITE ;
  }

  EF_SetState(nState, -1);
  D3DSetCull(eCULL_None);
  float fX = (float)m_CurViewport.nWidth;
  float fY = (float)m_CurViewport.nHeight;
  DrawQuad(-0.5f, -0.5f, fX-0.5f, fY-0.5f, m_pNewTarget[0]->m_ReqColor, 1.0f/*m_pNewTarget[0]->m_fReqDepth*/, fX, fY, fX, fY);
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_IN_CLEAR;

  m_RP.m_pCurObject = pObj;
  m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
  m_RP.m_pShader = pSHSave;
  m_RP.m_pCurTechnique = pSHT;
  m_RP.m_pCurPass = pPass;
	m_RP.m_pShaderResources = pShRes;
#endif
}

void CD3D9Renderer::FX_SetActiveRenderTargets(bool bAllowDIP)
{
  if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_IN_CLEAR)
    return;
  HRESULT hr = S_OK;
  bool bDirty = false;
  if (m_nMaxRT2Commit >= 0)
  {
    for (int i=0; i<=m_nMaxRT2Commit; i++)
    {
      if (!m_pNewTarget[i]->m_bWasSetRT)
      {
        m_pNewTarget[i]->m_bWasSetRT = true;
        if (m_pNewTarget[i]->m_pTex)
          m_pNewTarget[i]->m_pTex->SetResolved(false);
        m_pCurTarget[i] = m_pNewTarget[i]->m_pTex;
        bDirty = true;
        if (m_LogFile)
        {
          Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ Set RT");
          if (m_pNewTarget[i]->m_pTex)
          {
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " '%s'", m_pNewTarget[i]->m_pTex->GetName());
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " Format:%s", CTexture::NameForTextureFormat( m_pNewTarget[i]->m_pTex->m_eTFDst));
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " Type:%s", CTexture::NameForTextureType( m_pNewTarget[i]->m_pTex->m_eTT));
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " W/H:%d:%d\n",  m_pNewTarget[i]->m_pTex->GetWidth(), m_pNewTarget[i]->m_pTex->GetHeight());

          }
          else
          {
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " 'Unknown'\n");
          }
        }


#if defined (DIRECT3D9) || defined(OPENGL)
				{
					PROFILE_FRAME(SetRenderTarget);
					hr = m_pd3dDevice->SetRenderTarget(i, m_pNewTarget[i]->m_pTarget);
#if !defined(XENON)
					if(i==0)
					{
						bool bSrgbTarget = (m_pNewTarget[0]->m_pTarget == m_pBackBuffer || m_pNewTarget[0]->m_pTex == CTexture::s_ptexSceneTarget);
            bool bSRGBWriteEnable = (gRenDev->IsLinearSpaceShadingEnabled() && bSrgbTarget) && !(gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags2&RBPF2_NO_SRGBWRITES);//  && !gRenDev->IsHDRModeEnabled();
            m_pd3dDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, bSRGBWriteEnable);
					}
#endif
				}
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRTChanges++;
        CTexture *pRT = m_pNewTarget[i]->m_pTex;
        if (CV_r_stats == 11)
          EF_AddRTStat(pRT);
        if (pRT)
        {
          if (pRT->m_nRTSetFrameID != m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID)
          {
            pRT->m_nRTSetFrameID = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID;
            m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRTs++;
            m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTSize += pRT->GetDeviceDataSize();
            if (CV_r_stats == 12)
              EF_AddRTStat(pRT);
          }
        }
#endif
      }
    }
    if (!m_pNewTarget[0]->m_bWasSetD)
    {
      m_pNewTarget[0]->m_bWasSetD = true;
      bDirty = true;
#if defined (DIRECT3D9) || defined(OPENGL)
			{
				PROFILE_FRAME(SetDepthStencilSurface);
				hr = m_pd3dDevice->SetDepthStencilSurface(m_pNewTarget[0]->m_pDepth);
			}














#endif
    }
    //m_nMaxRT2Commit = -1;
  }
#if defined (DIRECT3D10)
  if (bDirty)
  {
    #if !defined (PS3) // For PS3 we allow textures currently being used as RenderTargets
      if (m_pNewTarget[0]->m_pTex)
      {
        // Reset all texture slots which are used as RT currently
        ID3D11ShaderResourceView *pRes = NULL;
        for (int i=0; i<MAX_TMU; i++)
        {
          if (CTexture::s_TexStages[i].m_Texture == m_pNewTarget[0]->m_pTex)
          {
            m_pd3dDeviceContext->PSSetShaderResources(i, 1, &pRes);
            CTexture::s_TexStages[i].m_Texture = NULL;
          }
        }
      }
    #endif
    
      //m_pd3dDeviceContext->OMSetRenderTargets(m_pNewTarget[0]->m_pTarget==NULL?0:1, &m_pNewTarget[0]->m_pTarget, m_pNewTarget[0]->m_pDepth);
    const uint32 nMaxRenderTargetViews = 4;
    ID3D11RenderTargetView *pRTV[ nMaxRenderTargetViews ]; 

    uint32 nNumViews = max(m_nMaxRT2Commit + 1, 0);
    for( uint32 r = 0 ; r < nNumViews; ++r)
    {        
      if( m_pNewTarget[r] )
        pRTV[r] = (ID3D11RenderTargetView *)m_pNewTarget[r]->m_pTarget;
    }






    m_pd3dDeviceContext->OMSetRenderTargets(m_pNewTarget[0]->m_pTarget==NULL?0:nNumViews, pRTV, m_pNewTarget[0]->m_pDepth);


    
    /*D3D11_RENDER_TARGET_VIEW_DESC RTV;
    if (m_pNewTarget[0]->m_pTarget)
    {
      m_pNewTarget[0]->m_pTarget->GetDesc(&RTV);
      if (m_pNewTarget[0]->m_pTex)
      {
        D3DTexture *pT = (D3DTexture *)m_pNewTarget[0]->m_pTex->m_pDevTexture;
        if (pT)
        {
          D3D11_RESOURCE_DIMENSION Type;
          pT->GetType(&Type);
          D3D11_TEXTURE2D_DESC TX;
          pT->GetDesc(&TX);
        }
      }
    }*/
  }
#endif

  if( m_nMaxRT2Commit >= 0)
    m_nMaxRT2Commit = -1;

  // Set current viewport
  if (m_bViewportDirty)
  {
    m_bViewportDirty = false;
    if (m_CurViewport != m_NewViewport)
    {
      m_CurViewport = m_NewViewport;
      D3DViewPort Port;
#if defined (DIRECT3D9) || defined(OPENGL)
			Port.Width = m_CurViewport.nWidth;
			Port.Height = m_CurViewport.nHeight;
      Port.X = m_CurViewport.nX;
      Port.Y = m_CurViewport.nY;
#if defined(INVERT_DEPTH_RANGE)
      Port.MinZ = m_CurViewport.fMaxZ;
      Port.MaxZ = m_CurViewport.fMinZ;
#else
      Port.MinZ = m_CurViewport.fMinZ;
      Port.MaxZ = m_CurViewport.fMaxZ;
#endif
      hr = m_pd3dDevice->SetViewport(&Port);
#elif defined (DIRECT3D10)






			Port.Width = (FLOAT)m_CurViewport.nWidth;
			Port.Height = (FLOAT)m_CurViewport.nHeight;
      Port.TopLeftX = (FLOAT)m_CurViewport.nX;
      Port.TopLeftY = (FLOAT)m_CurViewport.nY;

      Port.MinDepth = m_CurViewport.fMinZ;
      Port.MaxDepth = m_CurViewport.fMaxZ;
      m_pd3dDeviceContext->RSSetViewports(1, &Port);
#endif
    }
  }












  if (m_pNewTarget[0]->m_ClearFlags)
  {
    //m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    DWORD cColor = D3DRGBA(m_pNewTarget[0]->m_ReqColor[0], m_pNewTarget[0]->m_ReqColor[1], m_pNewTarget[0]->m_ReqColor[2], m_pNewTarget[0]->m_ReqColor[3]);
#if defined (DIRECT3D9) || defined(OPENGL)








		// AntonK: need to remove these checks after we have fixed issues with recursive rendering
		if(m_pNewTarget[0]->m_pTarget == NULL)
		{
			assert((m_pNewTarget[0]->m_ClearFlags & FRT_CLEAR_COLOR) == 0);
			m_pNewTarget[0]->m_ClearFlags &= ~FRT_CLEAR_COLOR;
		}

#if defined(INVERT_DEPTH_RANGE)
    hr = m_pd3dDevice->Clear(0, NULL, m_pNewTarget[0]->m_ClearFlags, cColor, 1.0f-m_pNewTarget[0]->m_fReqDepth, 0);
    assert(SUCCEEDED(hr));
#else
    hr = m_pd3dDevice->Clear(0, NULL, m_pNewTarget[0]->m_ClearFlags, cColor, m_pNewTarget[0]->m_fReqDepth, 0);
    assert(SUCCEEDED(hr));
#endif







#elif defined (DIRECT3D10)
    bool bEntireClear = true;
   if  (m_pNewTarget[0]->m_pTex)
    {
      int nWidth = m_pNewTarget[0]->m_pTex->GetWidth();
      int nHeight = m_pNewTarget[0]->m_pTex->GetHeight();
      if (bAllowDIP && (m_CurViewport.nX || m_CurViewport.nY || m_CurViewport.nWidth!=nWidth || m_CurViewport.nHeight!=nHeight))
      {
        // Clear region
        FX_ClearRegion();
        bEntireClear = false;
      }
    }
    if (bEntireClear)
    {
      if ( (m_pNewTarget[0]->m_pTarget!=NULL) && m_pNewTarget[0]->m_ClearFlags & D3DCLEAR_TARGET)
			{
#ifndef PS3
				for(int i=0;i<SIZEOF_ARRAY(m_pNewTarget);++i)
					if(m_pNewTarget[i]->m_pTarget)
						m_pd3dDeviceContext->ClearRenderTargetView(m_pNewTarget[i]->m_pTarget, &m_pNewTarget[0]->m_ReqColor[0]);







#endif
			}
      int nFlags = m_pNewTarget[0]->m_ClearFlags & ~D3DCLEAR_TARGET;
      if (nFlags == (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL))
      {
        m_pd3dDeviceContext->ClearDepthStencilView(m_pNewTarget[0]->m_pDepth, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, m_pNewTarget[0]->m_fReqDepth, 0);
      }
      else
      if (nFlags == D3DCLEAR_ZBUFFER)
        m_pd3dDeviceContext->ClearDepthStencilView(m_pNewTarget[0]->m_pDepth, D3D11_CLEAR_DEPTH, m_pNewTarget[0]->m_fReqDepth, 0);
      else
      if (nFlags == D3DCLEAR_STENCIL)
        m_pd3dDeviceContext->ClearDepthStencilView(m_pNewTarget[0]->m_pDepth, D3D11_CLEAR_STENCIL, m_pNewTarget[0]->m_fReqDepth, 0);
      else
      if (nFlags)
      {
        assert(0);
      }
    }
#endif
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTCleared++;
    CTexture *pRT = m_pNewTarget[0]->m_pTex;
    if (CV_r_stats == 13)
      EF_AddRTStat(pRT, m_pNewTarget[0]->m_ClearFlags, m_CurViewport.nWidth, m_CurViewport.nHeight);
    if (pRT)
    {
      if (m_pNewTarget[0]->m_ClearFlags & D3DCLEAR_TARGET)
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTClearedSize += pRT->GetDeviceDataSize();
      if (m_pNewTarget[0]->m_ClearFlags & D3DCLEAR_ZBUFFER)
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTClearedSize += pRT->GetWidth() * pRT->GetHeight() * 4;
    }
    else
    {
      if (m_pNewTarget[0]->m_ClearFlags & D3DCLEAR_TARGET)
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTClearedSize += m_width * m_height * 4;
      if (m_pNewTarget[0]->m_ClearFlags & D3DCLEAR_ZBUFFER)
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTClearedSize += m_width * m_height * 4;
      else
      if (m_pNewTarget[0]->m_ClearFlags & D3DCLEAR_STENCIL)
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTClearedSize += m_width * m_height;
    }

    m_pNewTarget[0]->m_ClearFlags = 0;
  }
}

void CD3D9Renderer::FX_Commit(bool bAllowDIP)
{
  // Commit all changed shader parameters
  CHWShader_D3D::mfCommitParams(true);

  // Commit all changed RT's
  FX_SetActiveRenderTargets(bAllowDIP);




}

// Set current geometry culling modes
void CD3D9Renderer::D3DSetCull(ECull eCull)
{ 
  if (eCull != eCULL_None)
  {
    if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MIRRORCULL)
      eCull = (eCull == eCULL_Back) ? eCULL_Front : eCULL_Back;
  }

  if (eCull == m_RP.m_eCull)
    return;

#if defined (DIRECT3D9) || defined(OPENGL)
  if (eCull == eCULL_None)
    m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  else
  {
    if (eCull == eCULL_Back)
      m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
    else
      m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
  }
#elif defined (DIRECT3D10)
  SStateRaster RS = m_StatesRS[m_nCurStateRS];

  RS.Desc.FrontCounterClockwise = true;

  if (eCull == eCULL_None)
    RS.Desc.CullMode = D3D11_CULL_NONE;
  else
  {
    if (eCull == eCULL_Back)
    {
      RS.Desc.CullMode = D3D11_CULL_BACK;
    }
    else
    {
      RS.Desc.CullMode = D3D11_CULL_FRONT;
    }
  }
  SetRasterState(&RS);
#endif
  m_RP.m_eCull = eCull;
}

void CRenderer::EF_SetStencilState(int st, uint32 nStencRef, uint32 nStencMask, uint32 nStencWriteMask)
{
#if defined (DIRECT3D9) || defined(OPENGL)
  LPDIRECT3DDEVICE9 dv = gcpRendD3D->GetD3DDevice();
  if (nStencRef != m_RP.m_CurStencRef)
  {
    m_RP.m_CurStencRef = nStencRef;
    dv->SetRenderState(D3DRS_STENCILREF, nStencRef);
  }
  if (nStencMask != m_RP.m_CurStencMask)
  {
    m_RP.m_CurStencMask = nStencMask;
    dv->SetRenderState(D3DRS_STENCILMASK, nStencMask);
  }
  if (nStencWriteMask != m_RP.m_CurStencWriteMask)
  {
    m_RP.m_CurStencWriteMask = nStencWriteMask;
    dv->SetRenderState(D3DRS_STENCILWRITEMASK, nStencWriteMask);
  }

  int Changed = st ^ m_RP.m_CurStencilState;
  if (!Changed)
    return;
  if (Changed & FSS_STENCIL_TWOSIDED)
  {
    if (st & FSS_STENCIL_TWOSIDED)
      dv->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, TRUE);
    else
      dv->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE);
  }
  if (Changed & FSS_STENCFUNC_MASK)
  {
    int nCurFunc = st & FSS_STENCFUNC_MASK;
    switch(nCurFunc)
    {
    case FSS_STENCFUNC_ALWAYS:
      dv->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
      break;
    case FSS_STENCFUNC_NEVER:
      dv->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_NEVER);
      break;
    case FSS_STENCFUNC_LESS:
      dv->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_LESS);
      break;
    case FSS_STENCFUNC_LEQUAL:
      dv->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_LESSEQUAL);
      break;
    case FSS_STENCFUNC_GREATER:
      dv->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_GREATER);
      break;
    case FSS_STENCFUNC_GEQUAL:
      dv->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_GREATEREQUAL);
      break;
    case FSS_STENCFUNC_EQUAL:
      dv->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
      break;
    case FSS_STENCFUNC_NOTEQUAL:
      dv->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_NOTEQUAL);
      break;
    default:
      assert(false);
    }
  }
  if (Changed & FSS_STENCFAIL_MASK)
  {
    int nCurOp = (st & FSS_STENCFAIL_MASK);
    switch(nCurOp >> FSS_STENCFAIL_SHIFT)
    {
    case FSS_STENCOP_KEEP:
      dv->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
      break;
    case FSS_STENCOP_REPLACE:
      dv->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_REPLACE);
      break;
    case FSS_STENCOP_INCR:
      dv->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_INCRSAT);
      break;
    case FSS_STENCOP_DECR:
      dv->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_DECRSAT);
      break;
    case FSS_STENCOP_INCR_WRAP:
      dv->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_INCR);
      break;
    case FSS_STENCOP_DECR_WRAP:
      dv->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_DECR);
      break;
    case FSS_STENCOP_ZERO:
      dv->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_ZERO);
      break;
    default:
      assert(false);
    }
  }
  if (Changed & FSS_STENCZFAIL_MASK)
  {
    int nCurOp = (st & FSS_STENCZFAIL_MASK);
    switch(nCurOp >> FSS_STENCZFAIL_SHIFT)
    {
    case FSS_STENCOP_KEEP:
      dv->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
      break;
    case FSS_STENCOP_REPLACE:
      dv->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_REPLACE);
      break;
    case FSS_STENCOP_INCR:
      dv->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_INCRSAT);
      break;
    case FSS_STENCOP_DECR:
      dv->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_DECRSAT);
      break;
    case FSS_STENCOP_INCR_WRAP:
      dv->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_INCR);
      break;
    case FSS_STENCOP_DECR_WRAP:
      dv->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_DECR);
      break;
    case FSS_STENCOP_ZERO:
      dv->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_ZERO);
      break;
    default:
      assert(false);
    }
  }
  if (Changed & FSS_STENCPASS_MASK)
  {
    int nCurOp = (st & FSS_STENCPASS_MASK);
    switch(nCurOp >> FSS_STENCPASS_SHIFT)
    {
    case FSS_STENCOP_KEEP:
      dv->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
      break;
    case FSS_STENCOP_REPLACE:
      dv->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
      break;
    case FSS_STENCOP_INCR:
      dv->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT);
      break;
    case FSS_STENCOP_DECR:
      dv->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_DECRSAT);
      break;
    case FSS_STENCOP_INCR_WRAP:
      dv->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCR);
      break;
    case FSS_STENCOP_DECR_WRAP:
      dv->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_DECR);
      break;
    case FSS_STENCOP_ZERO:
      dv->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_ZERO);
      break;
    default:
      assert(false);
    }
  }

  if (Changed & (FSS_STENCFUNC_MASK << FSS_CCW_SHIFT))
  {
    int nCurFunc = (st & (FSS_STENCFUNC_MASK << FSS_CCW_SHIFT));
    switch(nCurFunc >> FSS_CCW_SHIFT)
    {
    case FSS_STENCFUNC_ALWAYS:
      dv->SetRenderState(D3DRS_CCW_STENCILFUNC, D3DCMP_ALWAYS);
      break;
    case FSS_STENCFUNC_NEVER:
      dv->SetRenderState(D3DRS_CCW_STENCILFUNC, D3DCMP_NEVER);
      break;
    case FSS_STENCFUNC_LESS:
      dv->SetRenderState(D3DRS_CCW_STENCILFUNC, D3DCMP_LESS);
      break;
    case FSS_STENCFUNC_LEQUAL:
      dv->SetRenderState(D3DRS_CCW_STENCILFUNC, D3DCMP_LESSEQUAL);
      break;
    case FSS_STENCFUNC_GREATER:
      dv->SetRenderState(D3DRS_CCW_STENCILFUNC, D3DCMP_GREATER);
      break;
    case FSS_STENCFUNC_GEQUAL:
      dv->SetRenderState(D3DRS_CCW_STENCILFUNC, D3DCMP_GREATEREQUAL);
      break;
    case FSS_STENCFUNC_EQUAL:
      dv->SetRenderState(D3DRS_CCW_STENCILFUNC, D3DCMP_EQUAL);
      break;
    case FSS_STENCFUNC_NOTEQUAL:
      dv->SetRenderState(D3DRS_CCW_STENCILFUNC, D3DCMP_NOTEQUAL);
      break;
    default:
      assert(false);
    }
  }
  if (Changed & (FSS_STENCFAIL_MASK << FSS_CCW_SHIFT))
  {
    int nCurOp = (st & (FSS_STENCFAIL_MASK << FSS_CCW_SHIFT));
    switch(nCurOp >> (FSS_STENCFAIL_SHIFT+FSS_CCW_SHIFT))
    {
    case FSS_STENCOP_KEEP:
      dv->SetRenderState(D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_KEEP);
      break;
    case FSS_STENCOP_REPLACE:
      dv->SetRenderState(D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_REPLACE);
      break;
    case FSS_STENCOP_INCR:
      dv->SetRenderState(D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_INCRSAT);
      break;
    case FSS_STENCOP_DECR:
      dv->SetRenderState(D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_DECRSAT);
      break;
    case FSS_STENCOP_INCR_WRAP:
      dv->SetRenderState(D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_INCR);
      break;
    case FSS_STENCOP_DECR_WRAP:
      dv->SetRenderState(D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_DECR);
      break;
    case FSS_STENCOP_ZERO:
      dv->SetRenderState(D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_ZERO);
      break;
    default:
      assert(false);
    }
  }
  if (Changed & (FSS_STENCZFAIL_MASK << FSS_CCW_SHIFT))
  {
    int nCurOp = (st & (FSS_STENCZFAIL_MASK << FSS_CCW_SHIFT));
    switch(nCurOp >> (FSS_STENCZFAIL_SHIFT+FSS_CCW_SHIFT))
    {
    case FSS_STENCOP_KEEP:
      dv->SetRenderState(D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_KEEP);
      break;
    case FSS_STENCOP_REPLACE:
      dv->SetRenderState(D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_REPLACE);
      break;
    case FSS_STENCOP_INCR:
      dv->SetRenderState(D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_INCRSAT);
      break;
    case FSS_STENCOP_DECR:
      dv->SetRenderState(D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_DECRSAT);
      break;
    case FSS_STENCOP_INCR_WRAP:
      dv->SetRenderState(D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_INCR);
      break;
    case FSS_STENCOP_DECR_WRAP:
      dv->SetRenderState(D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_DECR);
      break;
    case FSS_STENCOP_ZERO:
      dv->SetRenderState(D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_ZERO);
      break;
    default:
      assert(false);
    }
  }
  if (Changed & (FSS_STENCPASS_MASK << FSS_CCW_SHIFT))
  {
    int nCurOp = (st & (FSS_STENCPASS_MASK << FSS_CCW_SHIFT));
    switch(nCurOp >> (FSS_STENCPASS_SHIFT+FSS_CCW_SHIFT))
    {
    case FSS_STENCOP_KEEP:
      dv->SetRenderState(D3DRS_CCW_STENCILPASS, D3DSTENCILOP_KEEP);
      break;
    case FSS_STENCOP_REPLACE:
      dv->SetRenderState(D3DRS_CCW_STENCILPASS, D3DSTENCILOP_REPLACE);
      break;
    case FSS_STENCOP_INCR:
      dv->SetRenderState(D3DRS_CCW_STENCILPASS, D3DSTENCILOP_INCRSAT);
      break;
    case FSS_STENCOP_DECR:
      dv->SetRenderState(D3DRS_CCW_STENCILPASS, D3DSTENCILOP_DECRSAT);
      break;
    case FSS_STENCOP_INCR_WRAP:
      dv->SetRenderState(D3DRS_CCW_STENCILPASS, D3DSTENCILOP_INCR);
      break;
    case FSS_STENCOP_DECR_WRAP:
      dv->SetRenderState(D3DRS_CCW_STENCILPASS, D3DSTENCILOP_DECR);
      break;
    case FSS_STENCOP_ZERO:
      dv->SetRenderState(D3DRS_CCW_STENCILPASS, D3DSTENCILOP_ZERO);
      break;
    default:
      assert(false);
    }
  }
#elif defined (DIRECT3D10)
  SStateDepth DS = gcpRendD3D->m_StatesDP[gcpRendD3D->m_nCurStateDP];
  DS.Desc.StencilReadMask = nStencMask;
  DS.Desc.StencilWriteMask = nStencWriteMask;

  int nCurFunc = st & FSS_STENCFUNC_MASK;
  switch(nCurFunc)
  {
  case FSS_STENCFUNC_ALWAYS:
    DS.Desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    break;
  case FSS_STENCFUNC_NEVER:
    DS.Desc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;
    break;
  case FSS_STENCFUNC_LESS:
    DS.Desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS;
    break;
  case FSS_STENCFUNC_LEQUAL:
    DS.Desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;
    break;
  case FSS_STENCFUNC_GREATER:
    DS.Desc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER;
    break;
  case FSS_STENCFUNC_GEQUAL:
    DS.Desc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;
    break;
  case FSS_STENCFUNC_EQUAL:
    DS.Desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
    break;
  case FSS_STENCFUNC_NOTEQUAL:
    DS.Desc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
    break;
  default:
    assert(false);
  }

  int nCurOp = (st & FSS_STENCFAIL_MASK);
  switch(nCurOp >> FSS_STENCFAIL_SHIFT)
  {
  case FSS_STENCOP_KEEP:
    DS.Desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    break;
  case FSS_STENCOP_REPLACE:
    DS.Desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
    break;
  case FSS_STENCOP_INCR:
    DS.Desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_INCR_SAT;
    break;
  case FSS_STENCOP_DECR:
    DS.Desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_DECR_SAT;
    break;
  case FSS_STENCOP_INCR_WRAP:
    DS.Desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_INCR;
    break;
  case FSS_STENCOP_DECR_WRAP:
    DS.Desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_DECR;
    break;
  case FSS_STENCOP_ZERO:
    DS.Desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_ZERO;
    break;
  default:
    assert(false);
  }

  nCurOp = (st & FSS_STENCZFAIL_MASK);
  switch(nCurOp >> FSS_STENCZFAIL_SHIFT)
  {
  case FSS_STENCOP_KEEP:
    DS.Desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    break;
  case FSS_STENCOP_REPLACE:
    DS.Desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
    break;
  case FSS_STENCOP_INCR:
    DS.Desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR_SAT;
    break;
  case FSS_STENCOP_DECR:
    DS.Desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR_SAT;
    break;
  case FSS_STENCOP_INCR_WRAP:
    DS.Desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    break;
  case FSS_STENCOP_DECR_WRAP:
    DS.Desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    break;
  case FSS_STENCOP_ZERO:
    DS.Desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;
    break;
  default:
    assert(false);
  }

  nCurOp = (st & FSS_STENCPASS_MASK);
  switch(nCurOp >> FSS_STENCPASS_SHIFT)
  {
  case FSS_STENCOP_KEEP:
    DS.Desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    break;
  case FSS_STENCOP_REPLACE:
    DS.Desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    break;
  case FSS_STENCOP_INCR:
    DS.Desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR_SAT;
    break;
  case FSS_STENCOP_DECR:
    DS.Desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR_SAT;
    break;
  case FSS_STENCOP_INCR_WRAP:
    DS.Desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
    break;
  case FSS_STENCOP_DECR_WRAP:
    DS.Desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
    break;
  case FSS_STENCOP_ZERO:
    DS.Desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
    break;
  default:
    assert(false);
  }

  nCurFunc = (st & (FSS_STENCFUNC_MASK << FSS_CCW_SHIFT));
  switch(nCurFunc >> FSS_CCW_SHIFT)
  {
  case FSS_STENCFUNC_ALWAYS:
    DS.Desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    break;
  case FSS_STENCFUNC_NEVER:
    DS.Desc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
    break;
  case FSS_STENCFUNC_LESS:
    DS.Desc.BackFace.StencilFunc = D3D11_COMPARISON_LESS;
    break;
  case FSS_STENCFUNC_LEQUAL:
    DS.Desc.BackFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;
    break;
  case FSS_STENCFUNC_GREATER:
    DS.Desc.BackFace.StencilFunc = D3D11_COMPARISON_GREATER;
    break;
  case FSS_STENCFUNC_GEQUAL:
    DS.Desc.BackFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;
    break;
  case FSS_STENCFUNC_EQUAL:
    DS.Desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
    break;
  case FSS_STENCFUNC_NOTEQUAL:
    DS.Desc.BackFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
    break;
  default:
    assert(false);
  }

  nCurOp = (st & (FSS_STENCFAIL_MASK << FSS_CCW_SHIFT));
  switch(nCurOp >> (FSS_STENCFAIL_SHIFT+FSS_CCW_SHIFT))
  {
  case FSS_STENCOP_KEEP:
    DS.Desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    break;
  case FSS_STENCOP_REPLACE:
    DS.Desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
    break;
  case FSS_STENCOP_INCR:
    DS.Desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_INCR_SAT;
    break;
  case FSS_STENCOP_DECR:
    DS.Desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_DECR_SAT;
    break;
  case FSS_STENCOP_INCR_WRAP:
    DS.Desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_INCR;
    break;
  case FSS_STENCOP_DECR_WRAP:
    DS.Desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_DECR;
    break;
  case FSS_STENCOP_ZERO:
    DS.Desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_ZERO;
    break;
  default:
    assert(false);
  }

  nCurOp = (st & (FSS_STENCZFAIL_MASK << FSS_CCW_SHIFT));
  switch(nCurOp >> (FSS_STENCZFAIL_SHIFT+FSS_CCW_SHIFT))
  {
  case FSS_STENCOP_KEEP:
    DS.Desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    break;
  case FSS_STENCOP_REPLACE:
    DS.Desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
    break;
  case FSS_STENCOP_INCR:
    DS.Desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR_SAT;
    break;
  case FSS_STENCOP_DECR:
    DS.Desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR_SAT;
    break;
  case FSS_STENCOP_INCR_WRAP:
    DS.Desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    break;
  case FSS_STENCOP_DECR_WRAP:
    DS.Desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    break;
  case FSS_STENCOP_ZERO:
    DS.Desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;
    break;
  default:
    assert(false);
  }

  nCurOp = (st & (FSS_STENCPASS_MASK << FSS_CCW_SHIFT));
  switch(nCurOp >> (FSS_STENCPASS_SHIFT+FSS_CCW_SHIFT))
  {
  case FSS_STENCOP_KEEP:
    DS.Desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    break;
  case FSS_STENCOP_REPLACE:
    DS.Desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    break;
  case FSS_STENCOP_INCR:
    DS.Desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR_SAT;
    break;
  case FSS_STENCOP_DECR:
    DS.Desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_DECR_SAT;
    break;
  case FSS_STENCOP_INCR_WRAP:
    DS.Desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
    break;
  case FSS_STENCOP_DECR_WRAP:
    DS.Desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
    break;
  case FSS_STENCOP_ZERO:
    DS.Desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
    break;
  default:
    assert(false);
  }

  m_RP.m_CurStencRef = nStencRef;
  m_RP.m_CurStencMask = nStencMask;
  m_RP.m_CurStencWriteMask = nStencWriteMask;

  gcpRendD3D->SetDepthState(&DS, nStencRef);
#endif





  m_RP.m_CurStencilState = st;
}

void CD3D9Renderer::EF_Scissor(bool bEnable, int sX, int sY, int sWdt, int sHgt)
{
  if (!CV_r_scissor || (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN))
    return;
#if defined (DIRECT3D9) || defined (OPENGL)
  RECT scRect;
  if (bEnable)
  {
    if (sX != m_sPrevX || sY != m_sPrevY || sWdt != m_sPrevWdt || sHgt != m_sPrevHgt)
    {
      m_sPrevX = sX;
      m_sPrevY = sY;
      m_sPrevWdt = sWdt;
      m_sPrevHgt = sHgt;
      scRect.left = sX;
      scRect.right = sX + sWdt;
      scRect.top = sY;
      scRect.bottom = sY + sHgt;
      m_pd3dDevice->SetScissorRect(&scRect);
    }
    if (bEnable != m_bsPrev)
    {
      m_bsPrev = bEnable;
      m_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    }
  }
  else
  {
    if (bEnable != m_bsPrev)
    {
      m_bsPrev = bEnable;
      m_sPrevWdt = 0;
      m_sPrevHgt = 0;
      m_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    }
  }
#elif defined (DIRECT3D10)
  D3D11_RECT scRect;
  if (bEnable)
  {
    if (sX != m_sPrevX || sY != m_sPrevY || sWdt != m_sPrevWdt || sHgt != m_sPrevHgt)
    {
      m_sPrevX = sX;
      m_sPrevY = sY;
      m_sPrevWdt = sWdt;
      m_sPrevHgt = sHgt;
      scRect.left = sX;
      scRect.top = sY;

// ps3 handles scissors with width/height only




      scRect.right = sX + sWdt;      
      scRect.bottom = sY + sHgt;


      m_pd3dDeviceContext->RSSetScissorRects(1, &scRect);
    }
    if (bEnable != m_bsPrev)
    {
      m_bsPrev = bEnable;
      SStateRaster RS = m_StatesRS[m_nCurStateRS];
      RS.Desc.ScissorEnable = bEnable;
      SetRasterState(&RS);
    }
  }
  else
  {
    if (bEnable != m_bsPrev)
    {
      m_bsPrev = bEnable;
      m_sPrevWdt = 0;
      m_sPrevHgt = 0;
      SStateRaster RS = m_StatesRS[m_nCurStateRS];
      RS.Desc.ScissorEnable = bEnable;
      SetRasterState(&RS);
    }








  }
#endif
}

uint32 CD3D9Renderer::EF_FogCorrection()
{
  static ColorF pColBlack = Col_Black;
  static ColorF pColWhite = Col_White;
  static ColorF pColGrey = ColorF(0.5f, 0.5f, 0.5f, 1.0f);

  switch ( m_RP.m_CurState & GS_BLEND_MASK )
  {
  case GS_BLSRC_ONE | GS_BLDST_ONE:
    EF_SetFogColor(pColBlack);
    return 1;
    break;
  case GS_BLSRC_DSTALPHA | GS_BLDST_ONE:
    EF_SetFogColor(pColBlack);
    return 1;
    break;
  case GS_BLSRC_DSTCOL | GS_BLDST_SRCCOL:
    EF_SetFogColor(pColGrey);
    return 1;
    break;
  case GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA:
    EF_SetFogColor(pColBlack);
    return 1;
    break;
  case GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCCOL:
    EF_SetFogColor(pColBlack);
    return 1;
    break;
  case GS_BLSRC_ZERO | GS_BLDST_ONEMINUSSRCCOL:
    EF_SetFogColor(pColBlack);
    return 1;
    break;
  case GS_BLSRC_SRCALPHA | GS_BLDST_ONE:
    EF_SetFogColor(pColBlack);
    return 1;
    break;
  case GS_BLSRC_ZERO | GS_BLDST_ONE:
    EF_SetFogColor(pColBlack);
    return 1;
    break;
  case GS_BLSRC_DSTCOL | GS_BLDST_ZERO:
    EF_SetFogColor(pColWhite);
    return 1;
    break;
  }
  return 0;
}

void CD3D9Renderer::EF_FogRestore(uint32 nFogOverrided)
{
  if (nFogOverrided)
    EF_SetFogColor(m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_FogColor);
}


void CD3D9Renderer::EF_SetHiZState(int newHiZState, int curState, int curStencilState)
{




















































































  return;
}

// Set current render states 
void CD3D9Renderer::EF_SetState(int st, int AlphaRef, int RestoreState)
{
  int Changed;

  if (m_RP.m_Flags & RBF_SHOWLINES)
    st |= GS_NODEPTHTEST;
  if (m_pNewTarget[0] && m_pNewTarget[0]->m_bDontDraw)
    st |= GS_COLMASK_NONE;

  if (m_wireframe_mode == R_POINT_MODE)
    st |= GS_POINTRENDERING;
  else if (m_wireframe_mode == R_WIREFRAME_MODE)
    st |= GS_WIREFRAME;

  if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_DISABLECOLORWRITES)
  {
    st |= GS_COLMASK_NONE;
  }
  
  Changed = st ^ m_RP.m_CurState;

  Changed |= RestoreState;




  if (!Changed && (AlphaRef==-1 || AlphaRef==m_RP.m_CurAlphaRef))  

    return;

  //PROFILE_FRAME(State_RStates);

#if defined (DIRECT3D9) || defined(OPENGL)

  int src, dst;
  LPDIRECT3DDEVICE9 dv = gcpRendD3D->GetD3DDevice();
  m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumStateChanges++;

  if (Changed & GS_DEPTHFUNC_MASK)
  {
    switch (st & GS_DEPTHFUNC_MASK)
    {
#if defined(INVERT_DEPTH_RANGE)
    case GS_DEPTHFUNC_EQUAL:
      dv->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
      break;
    case GS_DEPTHFUNC_LEQUAL:
      dv->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
      break;
    case GS_DEPTHFUNC_GREAT:
      dv->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
      break;
    case GS_DEPTHFUNC_LESS:
      dv->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATER);
      break;
    case GS_DEPTHFUNC_NOTEQUAL:
      dv->SetRenderState(D3DRS_ZFUNC, D3DCMP_NOTEQUAL);
      break;
    case GS_DEPTHFUNC_GEQUAL:
      dv->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
      break;
#else 
    case GS_DEPTHFUNC_EQUAL:
      dv->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
      break;
    case GS_DEPTHFUNC_LEQUAL:
      dv->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
      break;
    case GS_DEPTHFUNC_GREAT:
      dv->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATER);
      break;
    case GS_DEPTHFUNC_LESS:
      dv->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
      break;
    case GS_DEPTHFUNC_NOTEQUAL:
      dv->SetRenderState(D3DRS_ZFUNC, D3DCMP_NOTEQUAL);
      break;
    case GS_DEPTHFUNC_GEQUAL:
      dv->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
      break;
#endif

    }
    



  }

  if (Changed & (GS_WIREFRAME|GS_POINTRENDERING))
  {
    if (st & GS_POINTRENDERING)
    {
      dv->SetRenderState(D3DRS_FILLMODE, D3DFILL_POINT);
    }
    else if (st & GS_WIREFRAME)
    {
      dv->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    }
    else
    {
      dv->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    }
  }

	if (Changed & GS_COLMASK_MASK )
  {
		uint32 nMask = 0xfffffff0 | ((st & GS_COLMASK_MASK) >> GS_COLMASK_SHIFT);
		nMask = (~nMask) & 0xf;
		dv->SetRenderState(D3DRS_COLORWRITEENABLE, nMask);
		dv->SetRenderState(D3DRS_COLORWRITEENABLE1, nMask);
		dv->SetRenderState(D3DRS_COLORWRITEENABLE2, nMask);
		dv->SetRenderState(D3DRS_COLORWRITEENABLE3, nMask);
	}

	bool bHiPrecisionBlend = false;
  
  if (Changed & GS_BLEND_MASK)
  {
    //reset for current state if necessary
    if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_NOALPHABLEND)
    {
      st &= ~GS_BLEND_MASK;
    }

		// Need to disable color write to MRTs for shadow map alpha blending (not supported by all hw)
		if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN)
		{
			if(st & GS_BLEND_MASK)
			{
				dv->SetRenderState(D3DRS_COLORWRITEENABLE1, 0);
				dv->SetRenderState(D3DRS_COLORWRITEENABLE2, 0);
				dv->SetRenderState(D3DRS_COLORWRITEENABLE3, 0);
			}
			else
			{
				uint32 nMask = 0xfffffff0 | ((st & GS_COLMASK_MASK) >> GS_COLMASK_SHIFT);
				nMask = (~nMask) & 0xf;
				dv->SetRenderState(D3DRS_COLORWRITEENABLE1, nMask);
				dv->SetRenderState(D3DRS_COLORWRITEENABLE2, nMask);
				dv->SetRenderState(D3DRS_COLORWRITEENABLE3, nMask);
			}
		}

    if (st & GS_BLEND_MASK)
    {
			D3DBLENDOP blendOperation = D3DBLENDOP_ADD;
      if (CV_r_measureoverdraw && (m_RP.m_nRendFlags & SHDF_ALLOWHDR))
      {
        st = (st & ~GS_BLEND_MASK) | (GS_BLSRC_ONE | GS_BLDST_ONE);
        st &= ~GS_ALPHATEST_MASK;
      }
      
      // Source factor
      switch (st & GS_BLSRC_MASK)
      {
      case GS_BLSRC_ZERO:
        src = D3DBLEND_ZERO;
        break;
      case GS_BLSRC_ONE:
        src = D3DBLEND_ONE;
        break;
      case GS_BLSRC_DSTCOL:
        src = D3DBLEND_DESTCOLOR;
        break;
      case GS_BLSRC_ONEMINUSDSTCOL:
        src = D3DBLEND_INVDESTCOLOR;
        break;
      case GS_BLSRC_SRCALPHA:
        src = D3DBLEND_SRCALPHA;
        break;
      case GS_BLSRC_ONEMINUSSRCALPHA:
        src = D3DBLEND_INVSRCALPHA;
        break;
      case GS_BLSRC_DSTALPHA:
        src = D3DBLEND_DESTALPHA;
        break;
      case GS_BLSRC_ONEMINUSDSTALPHA:
        src = D3DBLEND_INVDESTALPHA;
        break;
      case GS_BLSRC_ALPHASATURATE:
        src = D3DBLEND_SRCALPHASAT;
        break;
      default:
        iLog->Log("CD3D9Renderer::SetState: invalid src blend state bits '%d'", st & GS_BLSRC_MASK);
        break;
      }

      //Destination factor
      switch (st & GS_BLDST_MASK)
      {
      case GS_BLDST_ZERO:
        dst = D3DBLEND_ZERO;
        break;
      case GS_BLDST_ONE:
        dst = D3DBLEND_ONE;
        break;
      case GS_BLDST_SRCCOL:
        dst = D3DBLEND_SRCCOLOR;
        break;
      case GS_BLDST_ONEMINUSSRCCOL:
        dst = D3DBLEND_INVSRCCOLOR;
        if (m_nHDRType == 1 && (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_HDR))
            dst = D3DBLEND_ONE;
        break;
      case GS_BLDST_SRCALPHA:
        dst = D3DBLEND_SRCALPHA;
        break;
      case GS_BLDST_ONEMINUSSRCALPHA:
        dst = D3DBLEND_INVSRCALPHA;
        break;
      case GS_BLDST_DSTALPHA:
        dst = D3DBLEND_DESTALPHA;
        break;
      case GS_BLDST_ONEMINUSDSTALPHA:
        dst = D3DBLEND_INVDESTALPHA;
        break;
      default:
        iLog->Log("CD3D9Renderer::SetState: invalid dst blend state bits '%d'", st & GS_BLDST_MASK);
        break;
      }
			//Blending operation
			if (Changed & GS_BLEND_OP_MASK)
			{
				switch (st & GS_BLEND_OP_MASK)
				{
				case GS_BLOP_MAX:
					blendOperation = D3DBLENDOP_MAX;
					break;
				default:
					blendOperation = D3DBLENDOP_ADD;
					break;
				}
				dv->SetRenderState(D3DRS_BLENDOP,  blendOperation);
			}

      if ( !(m_RP.m_CurState & GS_BLEND_MASK) )
        dv->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

			dv->SetRenderState(D3DRS_SRCBLEND,  src);
      dv->SetRenderState(D3DRS_DESTBLEND, dst);
      




		}
		else
		{
			dv->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			
		}
	}































  if (Changed & GS_DEPTHWRITE)
  {
    if (st & GS_DEPTHWRITE)
      dv->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    else
      dv->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);



  }

  if (Changed & GS_NODEPTHTEST)
  {
    if (st & GS_NODEPTHTEST)
    {
      dv->SetRenderState(D3DRS_ZENABLE, FALSE);
    }
    else
    {
      dv->SetRenderState(D3DRS_ZENABLE, TRUE);
    }



  }

  if (Changed & GS_STENCIL && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_LOCKSTENCIL))
  {
    if (st & GS_STENCIL)
      dv->SetRenderState(D3DRS_STENCILENABLE, TRUE);
    else
      dv->SetRenderState(D3DRS_STENCILENABLE, FALSE);



  }

  if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_NOALPHATEST) || (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_ATOC))
  {
#if !defined(XENON) && !defined(PS3)
    if (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] == 1 && m_RP.m_nPassGroupID == EFSLIST_GENERAL && !(m_RP.m_nBatchFilter & (FB_ZPREPASS|FB_Z|FB_GLOW)) && CV_r_usezpass)
    {
      if (m_RP.m_CurState & GS_ALPHATEST_MASK)
      {
        dv->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
      }  
      st &= ~GS_ALPHATEST_MASK;
    }
    else
#endif
    {
      if ((st & GS_ALPHATEST_MASK) && m_RP.m_CurAlphaRef != AlphaRef)
      {
        //assert(AlphaRef>=0 && AlphaRef<255);
        m_RP.m_CurAlphaRef = AlphaRef;
        dv->SetRenderState(D3DRS_ALPHAREF, AlphaRef);
      }
      if ((st ^ m_RP.m_CurState) & GS_ALPHATEST_MASK)
      {
        if (st & GS_ALPHATEST_MASK)
        {
          if (!(m_RP.m_CurState & GS_ALPHATEST_MASK))
            dv->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
          switch (st & GS_ALPHATEST_MASK)
          {
          case GS_ALPHATEST_GREATER:
            dv->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
            break;
          case GS_ALPHATEST_LESS:
            dv->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESS);
            break;
          case GS_ALPHATEST_GEQUAL:
            dv->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
            break;
          case GS_ALPHATEST_LEQUAL:
            dv->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
            break;
          }
        }
        else
          dv->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
      }
    }
  }
#elif defined (DIRECT3D10)
  m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumStateChanges++;
  SStateDepth DS = m_StatesDP[m_nCurStateDP];
  SStateBlend BS = m_StatesBL[m_nCurStateBL];
  SStateRaster RS = m_StatesRS[m_nCurStateRS];
  bool bDirtyDS = false;
  bool bDirtyBS = false;
  bool bDirtyRS = false;

  if (Changed & GS_DEPTHFUNC_MASK)
  {
    bDirtyDS = true;
    switch (st & GS_DEPTHFUNC_MASK)
    {
    case GS_DEPTHFUNC_EQUAL:
      DS.Desc.DepthFunc = D3D11_COMPARISON_EQUAL;
      break;
    case GS_DEPTHFUNC_LEQUAL:
      DS.Desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
      break;
    case GS_DEPTHFUNC_GREAT:
      DS.Desc.DepthFunc = D3D11_COMPARISON_GREATER;
      break;
    case GS_DEPTHFUNC_LESS:
      DS.Desc.DepthFunc = D3D11_COMPARISON_LESS;
      break;
    case GS_DEPTHFUNC_NOTEQUAL:
      DS.Desc.DepthFunc = D3D11_COMPARISON_NOT_EQUAL;
      break;
    case GS_DEPTHFUNC_GEQUAL:
      DS.Desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
      break;
    }
  }

	if (Changed & (GS_WIREFRAME|GS_POINTRENDERING))
	{
		bDirtyRS = true;
		if (st & GS_WIREFRAME)
			RS.Desc.FillMode = D3D11_FILL_WIREFRAME;





		else      
			RS.Desc.FillMode = D3D11_FILL_SOLID;
	}

  if (Changed & GS_COLMASK_MASK)
  {
    bDirtyBS = true;
    uint32 nMask = 0xfffffff0 | ((st & GS_COLMASK_MASK) >> GS_COLMASK_SHIFT);
		nMask = (~nMask) & 0xf;
		BS.Desc.RenderTarget[0].RenderTargetWriteMask = nMask;
		BS.Desc.RenderTarget[1].RenderTargetWriteMask = nMask;
		BS.Desc.RenderTarget[2].RenderTargetWriteMask = nMask;
		BS.Desc.RenderTarget[3].RenderTargetWriteMask = nMask;
  }

  if (Changed & GS_BLEND_MASK) 
  {
    if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_NOALPHABLEND)
      st &= ~GS_BLEND_MASK;

		bDirtyBS = true;
		if (st & GS_BLEND_MASK)
		{
			// todo: add separate alpha blend support for mrt
			for(size_t i=0;i<8;++i)
				BS.Desc.RenderTarget[i].BlendEnable = TRUE;

			// Source factor
			switch (st & GS_BLSRC_MASK)
			{
			case GS_BLSRC_ZERO:
				BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
				BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
				break;
			case GS_BLSRC_ONE:
				BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
				BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
				break;
			case GS_BLSRC_DSTCOL:
				BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
				BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA;
				break;
			case GS_BLSRC_ONEMINUSDSTCOL:
				BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;
				BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
				break;
			case GS_BLSRC_SRCALPHA:
				BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
				break;
			case GS_BLSRC_ONEMINUSSRCALPHA:
				BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_SRC_ALPHA;
				BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				break;
			case GS_BLSRC_DSTALPHA:
				BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_ALPHA;
				BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA;
				break;
			case GS_BLSRC_ONEMINUSDSTALPHA:
				BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_ALPHA;
				BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
				break;
			case GS_BLSRC_ALPHASATURATE:
				BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA_SAT;
				BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA_SAT;
				break;
			default:
				iLog->Log("CD3D9Renderer::SetState: invalid src blend state bits '%d'", st & GS_BLSRC_MASK);
				break;
			}

			//Destination factor
			switch (st & GS_BLDST_MASK)
			{
			case GS_BLDST_ZERO:
				BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
				BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
				break;
			case GS_BLDST_ONE:
				BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
				BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				break;
			case GS_BLDST_SRCCOL:
				BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
				BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
				break;
			case GS_BLDST_ONEMINUSSRCCOL:
				if (m_nHDRType == 1 && (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_HDR))
				{
					BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
					BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				}
				else
				{
					BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
					BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				}
				break;
			case GS_BLDST_SRCALPHA:
				BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA;
				BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
				break;
			case GS_BLDST_ONEMINUSSRCALPHA:
				BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				break;
			case GS_BLDST_DSTALPHA:
				BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_ALPHA;
				BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
				break;
			case GS_BLDST_ONEMINUSDSTALPHA:
				BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_DEST_ALPHA;
				BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
				break;
			default:
				iLog->Log("CD3D9Renderer::SetState: invalid dst blend state bits '%d'", st & GS_BLDST_MASK);
				break;
			}

			//Blending operation
			D3D11_BLEND_OP blendOperation = D3D11_BLEND_OP_ADD;
			switch (st & GS_BLEND_OP_MASK)
			{
			case GS_BLOP_MAX:
				blendOperation = D3D11_BLEND_OP_MAX;
				break;
			}

			// todo: add separate alpha blend support for mrt
			for(size_t i=0;i<8;++i)
			{
				BS.Desc.RenderTarget[i].BlendOp = blendOperation;
				BS.Desc.RenderTarget[i].BlendOpAlpha = blendOperation;
			}
    }
    else
    {
      // todo: add separate alpha blend support for mrt
			for(size_t i=0;i<8;++i)
				BS.Desc.RenderTarget[i].BlendEnable = FALSE;
    }

		// Need to disable color write to MRTs for shadow map alpha blending (not supported by all hw)
		if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN)
		{
			bDirtyBS = true;
			uint32 nMask = 0xfffffff0 | ((st & GS_COLMASK_MASK) >> GS_COLMASK_SHIFT);
			nMask = (~nMask) & 0xf;
			BS.Desc.RenderTarget[0].RenderTargetWriteMask = nMask;
			if(st & GS_BLEND_MASK)
			{
				BS.Desc.IndependentBlendEnable = TRUE;
				for(size_t i=1;i<8;++i)
				{
					BS.Desc.RenderTarget[i].RenderTargetWriteMask = 0;
					BS.Desc.RenderTarget[i].BlendEnable = FALSE;
				}
			}
			else
			{
				BS.Desc.IndependentBlendEnable = FALSE;
				for(size_t i=1;i<8;++i)
				{
					BS.Desc.RenderTarget[i].RenderTargetWriteMask = nMask;
					BS.Desc.RenderTarget[i].BlendEnable = TRUE;
				}
			}
		}
  }

  if (Changed & GS_DEPTHWRITE)
  {
    bDirtyDS = true;
    if (st & GS_DEPTHWRITE)
      DS.Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    else
      DS.Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
  }

  if (Changed & GS_NODEPTHTEST)
  {
    bDirtyDS = true;
    if (st & GS_NODEPTHTEST)
      DS.Desc.DepthEnable = FALSE;
    else
      DS.Desc.DepthEnable = TRUE;
  }

  if (Changed & GS_STENCIL)
  {
    bDirtyDS = true;
    if (st & GS_STENCIL)
      DS.Desc.StencilEnable = TRUE;
    else
      DS.Desc.StencilEnable = FALSE;
  }

  if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_NOALPHATEST) || (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_ATOC))
  {
    // Alpha test must be handled in shader in D3D10 API
    if ( ((st ^ m_RP.m_CurState) & GS_ALPHATEST_MASK)
			|| ((st & GS_ALPHATEST_MASK) != 0 && (m_RP.m_CurAlphaRef != AlphaRef && AlphaRef != -1)))
    {
			if (st & GS_ALPHATEST_MASK)
				m_RP.m_CurAlphaRef = AlphaRef;






















    }
  }

	bool bCurATOC = BS.Desc.AlphaToCoverageEnable != 0;
	bool bNewATOC = ((st & GS_ALPHATEST_MASK) != 0) && ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_ATOC) != 0);
	bDirtyBS |= bNewATOC ^ bCurATOC;
	BS.Desc.AlphaToCoverageEnable = bNewATOC;

	if (bDirtyDS)
    SetDepthState(&DS, m_nCurStencRef);
  if (bDirtyRS)
    SetRasterState(&RS);
  if (bDirtyBS)
    SetBlendState(&BS);
#endif

  m_RP.m_CurState = st;
}

void CD3D9Renderer::FX_ZState(int& nState)
{
  // We cannot use z-prepass results with predicated tiling on Xenon





  assert(m_RP.m_pRootTechnique);		// cannot be 0 here
  if (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] == 1 && (m_RP.m_nBatchFilter & (FB_GENERAL|FB_MULTILAYERS|FB_RAIN)) && (m_RP.m_nRendFlags & (SHDF_ALLOWHDR | SHDF_ALLOWPOSTPROCESS)) && m_RP.m_nPassGroupID==EFSLIST_GENERAL && CV_r_usezpass && (m_RP.m_pRootTechnique->m_Flags & (FHF_WASZWRITE | FHF_POSITION_INVARIANT)))
  {
    if (!(m_RP.m_pRootTechnique->m_Flags & FHF_POSITION_INVARIANT))
      nState |= GS_DEPTHFUNC_EQUAL;
    nState &= ~(GS_DEPTHWRITE | GS_ALPHATEST_MASK);
  }

  if (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] == 1 && (m_RP.m_nBatchFilter & FB_SCATTER) && m_RP.m_nPassGroupID==EFSLIST_GENERAL && CV_r_usezpass && (m_RP.m_pRootTechnique->m_Flags & (FHF_WASZWRITE | FHF_POSITION_INVARIANT)))
  {
    if (!(m_RP.m_pRootTechnique->m_Flags & FHF_POSITION_INVARIANT))
      nState |= GS_NODEPTHTEST;
    nState &= ~(GS_DEPTHWRITE | GS_ALPHATEST_MASK);
  }
}

void CD3D9Renderer::FX_CommitStates(SShaderTechnique *pTech, SShaderPass *pPass, bool bUseMaterialState)
{
  int State = 0;
  int AlphaRef = pPass->m_AlphaRef == 0xff ? -1 : pPass->m_AlphaRef;
  
  if (m_RP.m_pCurObject->m_RState)
  {
    switch (m_RP.m_pCurObject->m_RState & 7)
    {
      case OS_ALPHA_BLEND:
        State = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
        break;
      case OS_COLOR_BLEND:
        State = GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCCOL;
        break;
      case OS_ADD_BLEND:
        State = GS_BLSRC_ONE | GS_BLDST_ONE;
        break;
    }
    if (m_RP.m_pCurObject->m_RState & OS_NODEPTH_TEST)
      State |= GS_NODEPTHTEST;
    if (m_RP.m_pCurObject->m_RState & OS_ALPHATEST_GREATER)
      State |= GS_ALPHATEST_GREATER;
    AlphaRef = 0;
  }
  else
    State = pPass->m_RenderState;    

  if ( m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_LIGHTSTENCILCULL /*&& !(m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_AMBIENT] ) */)  
    State |= GS_STENCIL;  
  else      
    State &=~GS_STENCIL;  //reset stencil  

  if (bUseMaterialState && m_RP.m_MaterialState != 0)
  {
    if (m_RP.m_MaterialState & GS_ALPHATEST_MASK)
      AlphaRef = m_RP.m_MaterialAlphaRef;

    // Reminder for Andrey: this will not work if zpass off
    if (m_RP.m_MaterialState & GS_BLEND_MASK)
      State = (State & ~(GS_BLEND_MASK | GS_DEPTHWRITE | GS_DEPTHFUNC_EQUAL)) | (m_RP.m_MaterialState & GS_BLEND_MASK);
    State = (State & ~GS_ALPHATEST_MASK) | (m_RP.m_MaterialState & GS_ALPHATEST_MASK);
  
    if (!(State & GS_ALPHATEST_MASK)) 
      State &= ~GS_DEPTHWRITE;
  }

  if (!(pTech->m_Flags & FHF_POSITION_INVARIANT) && !(pPass->m_PassFlags & SHPF_FORCEZFUNC))
    FX_ZState(State);

  if ((m_RP.m_pShader->m_Flags & EF_DECAL) && !(m_RP.m_FlagsShader_MDV & MDV_DEPTH_OFFSET))
    State = (State & ~GS_DEPTHFUNC_MASK)|GS_DEPTHFUNC_EQUAL;

  if ( bUseMaterialState && (m_RP.m_pCurObject->m_fAlpha < 1.0f) && !m_RP.m_bIgnoreObjectAlpha)
    State = (State & ~(GS_BLEND_MASK | GS_DEPTHWRITE))|(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA); 

  // Specific condition for cloak transition
  if( (m_RP.m_pCurObject->m_nMaterialLayers&MTL_LAYER_BLEND_CLOAK) && !m_RP.m_bIgnoreObjectAlpha)
  {
    uint32 nResourcesNoDrawFlags = m_RP.m_pShaderResources->GetMtlLayerNoDrawFlags();
    if( !(nResourcesNoDrawFlags&MTL_LAYER_CLOAK) )
    {
      const float fCloakMinThreshold = 0.85f;
      if( (((m_RP.m_pCurObject->m_nMaterialLayers&MTL_LAYER_BLEND_CLOAK)>> 8) / 255.0f) > fCloakMinThreshold )
      {
        State &= ~(GS_BLEND_MASK|GS_DEPTHWRITE); 
        State |= (GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
      }
    }
  }

  //after the first pass we need to change the srcalpha-oneminusscralpha to scralpha-one
  if (m_RP.m_bNotFirstPass)
  {
    if ((State & GS_BLEND_MASK) == (GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA))
      State = (State & ~GS_BLEND_MASK) | GS_BLSRC_SRCALPHA | GS_BLDST_ONE;
    else
      State = (State & ~GS_BLEND_MASK) | GS_BLSRC_ONE | GS_BLDST_ONE;
  }


  if ((m_RP.m_pShader->m_Flags2 & EF2_HAIR) && (m_RP.m_nPassGroupID==EFSLIST_GENERAL || m_RP.m_nPassGroupID==EFSLIST_TRANSP) && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN|RBPF_ZPASS)))
  {
    // force per object fog      
    m_RP.m_FlagsShader_RT |=(g_HWSR_MaskBit[HWSR_FOG]| g_HWSR_MaskBit[HWSR_ALPHABLEND]);
        
    if( (pPass->m_RenderState & GS_DEPTHFUNC_MASK) == GS_DEPTHFUNC_LESS )
    {      
      if( m_RP.m_bNotFirstPass )
      {
        State = (State & ~(GS_BLEND_MASK|GS_DEPTHFUNC_MASK|GS_DEPTHWRITE));
        if( pPass->m_RenderState & GS_DEPTHWRITE )
          State |=GS_BLSRC_SRCALPHA| GS_BLDST_ONE | GS_DEPTHFUNC_EQUAL;
        else
          State |= GS_BLSRC_SRCALPHA | GS_BLDST_ONE | GS_DEPTHFUNC_LEQUAL; //GS_BLSRC_ONESRCALPHA
      }
      else
      {
        State = (State & ~(GS_BLEND_MASK|GS_DEPTHFUNC_MASK));
        State |= GS_DEPTHFUNC_LESS | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA; 
        
        if( pPass->m_RenderState & GS_DEPTHWRITE )
          State |= GS_DEPTHWRITE;
        else
          State &= ~GS_DEPTHWRITE;
      }
    }
    else
    {
      if( m_RP.m_bNotFirstPass ) 
      {
        State = (State & ~(GS_BLEND_MASK|GS_DEPTHFUNC_MASK|GS_DEPTHWRITE));
        State |= GS_BLSRC_ONE| GS_BLDST_ONE | GS_DEPTHFUNC_EQUAL; 
      }
      else
      {
        State = (State & ~(GS_BLEND_MASK|GS_DEPTHFUNC_MASK));
        State |= GS_DEPTHFUNC_EQUAL/*| GS_DEPTHWRITE*/;
      }
    }
  }

  
  if ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_ALLOW_DEFERREDSHADING) && (m_RP.m_pShader->m_Flags & EF_SUPPORTSDEFERREDSHADING))
  {
    // Set correct states for deferred rendering normals
		m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEFERRED_SHADING];

    if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_ZPASS)
    {    
      if ((m_RP.m_pShader->m_Flags & EF_DECAL) || m_RP.m_nPassGroupID == EFSLIST_TERRAINLAYER)
      {
        State = (State & ~(GS_BLEND_MASK|GS_DEPTHWRITE|GS_DEPTHFUNC_MASK));
        State |= GS_DEPTHFUNC_LEQUAL | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;  
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ALPHABLEND]; 
      }

      // Disable alpha writes - for alpha blend case we use default alpha value as a default power factor
      if (State & GS_BLEND_MASK)
        State |= GS_COLMASK_RGB;
    }
		else 
		//if ( State&GS_BLEND_MASK && (m_RP.m_nPassGroupID != EFSLIST_TRANSP) && !( (m_RP.m_pShader->m_Flags & EF_DECAL) || (m_RP.m_nPassGroupID  == EFSLIST_TERRAINLAYER) )  )
		if (m_RP.m_nPassGroupID == EFSLIST_TRANSP)
			m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_DEFERRED_SHADING];
  }

  //force no depth test for scattering passes
  if (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] == 1 && (m_RP.m_nBatchFilter & FB_SCATTER))
  {
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SCATTERSHADE ];
    if ((m_RP.m_FlagsPerFlush & RBSI_NOCULL) && (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_ZPASS)) //scattering z-only pass for proper skeleton visibility
    {
      State &= ~GS_BLEND_MASK;
      State |= GS_DEPTHWRITE;
      State &= ~GS_NODEPTHTEST;
      State |= GS_DEPTHFUNC_LEQUAL;

      State &= ~GS_ALPHATEST_MASK;

      State |= GS_COLMASK_NONE;
      //State |= GS_COLMASK_RGB;
    }
    else 
    if ((m_RP.m_FlagsPerFlush & RBSI_NOCULL) && !(m_RP.m_pShaderResources->m_ResFlags & MTL_FLAG_2SIDED)) //detect depth scattering case  
    {
      //depth estimation
      State |= GS_NODEPTHTEST;
      //State |= GS_NODEPTHTEST;
      State &= ~(GS_DEPTHWRITE | GS_ALPHATEST_MASK);
      //State &= ~GS_BLEND_MASK;
      State |= (GS_BLSRC_ONE | GS_BLDST_ONE);






      State |= GS_COLMASK_A;

      //State |= GS_COLMASK_RGB; //for silhouette
    }
    else 
    {

      //internal RGB skeleton and all occluders rendering
      //note: should be drawn before all the transparent parts
      m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[ HWSR_DEFERRED_SHADING ];
      State &= ~GS_BLEND_MASK;
      if (m_RP.m_bNotFirstPass)
      {
          State |= GS_BLSRC_ONE | GS_BLDST_ONE;
      }
      State |= GS_DEPTHWRITE;
      //enable depth test
      State &= ~GS_NODEPTHTEST;
      //State |= GS_NODEPTHTEST;
      State |= GS_DEPTHFUNC_LEQUAL;

      //TD:: skeleton front faces depth should be rendered and RGB skeleton should be rendered during next pass
      //for proper depth estimation 
      //State |= GS_COLMASK_A;
      //D3DSetCull(eCULL_Front);
      //m_RP.m_FlagsPerFlush |= RBSI_NOCULL;

      //State |= GS_COLMASK_NONE;
      //for RGB occluders rendering




      State |= GS_COLMASK_RGB; 

    }
  }

	if( (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_CUSTOM_RENDER_PASS) )
	{
		gcpRendD3D->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];
		if( CRenderer::CV_r_customvisions == 2 )
		{
			gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
			State |= GS_BLSRC_ONE | GS_BLDST_ONE;
		}
	}

  if (m_NewViewport.fMaxZ <= 0.01f)
    State &= ~GS_DEPTHWRITE;

  if ((m_RP.m_MaterialState|State) & GS_ALPHATEST_MASK)
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ALPHATEST];


























  if(CV_r_VegetationAlphaTestOnly == 2)
  {
    State &= ~GS_ALPHATEST_MASK;
    m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_ALPHATEST];
  }

	// Intermediate solution to disable depth testing in 3D HUD
	if (m_RP.m_pCurObject->m_ObjFlags & FOB_RENDER_AFTER_POSTPROCESSING )
	{
	  State &= ~GS_DEPTHFUNC_MASK;
		State |= GS_NODEPTHTEST;
	}


  EF_SetState(State, AlphaRef);
  
  int nBlend;
  if (nBlend=(m_RP.m_CurState & GS_BLEND_MASK))
  {
    if (nBlend == (GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA) || nBlend == (GS_BLSRC_SRCALPHA | GS_BLDST_ONE) || nBlend == (GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA))
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ALPHABLEND];
  }
}

//=====================================================================================























void CD3D9Renderer::FX_GetRTDimensions(bool bRTPredicated, int& nWidth, int& nHeight, bool bForcePredicated)
{




































}

bool CD3D9Renderer::FX_GetTargetSurfaces(CTexture *pTarget, D3DSurface*& pTargSurf, SRTStack *pCur, int nCMSide, int nTarget, bool bFP16, uint32 nTileCount)
{
  if (pTarget)
  {
    if (!CTexture::IsTextureExist(pTarget) && !pTarget->m_bNoDevTexture)
      pTarget->CreateRenderTarget(eTF_Unknown);














































































    
		if (!CTexture::IsTextureExist(pTarget))
      return false;
#if defined (DIRECT3D9) || defined (OPENGL)
    pTargSurf = (D3DSurface *)pTarget->GetDeviceRT();
    if (pTargSurf)
      pTargSurf->AddRef();
    else
      pTargSurf = pTarget->GetSurface(nCMSide, 0);
#elif defined (DIRECT3D10)
    pTargSurf = pTarget->GetSurface(nCMSide, 0);
#endif


  }
  else
    pTargSurf = NULL;
  return true;
}

bool CD3D9Renderer::FX_SetRenderTarget(int nTarget, D3DSurface *pTargetSurf, SD3DSurface *pDepthTarget, bool bClearOnResolve, uint32 nTileCount)
{
  if (m_nRTStackLevel[nTarget] >= MAX_RT_STACK)
    return false;
  HRESULT hr = 0;
  SRTStack *pCur = &m_RTStack[nTarget][m_nRTStackLevel[nTarget]];
  pCur->m_pTarget = pTargetSurf;
  pCur->m_pSurfDepth = pDepthTarget;
  pCur->m_pDepth = pDepthTarget ? (D3DDepthSurface *)pDepthTarget->pSurf : NULL;
  pCur->m_pTex = NULL;

#ifdef _DEBUG
  if (m_nRTStackLevel[nTarget] == 0 && nTarget == 0)
  {
    assert(pCur->m_pTarget == m_pBackBuffer && pCur->m_pDepth == m_pZBuffer);
  }
#endif

  pCur->m_bNeedReleaseRT = false;
  pCur->m_bWasSetRT = false;
  pCur->m_bWasSetD = false;
  pCur->m_bClearOnResolve = bClearOnResolve;
  pCur->m_ClearFlags = 0;
  m_pNewTarget[nTarget] = pCur;
  m_nMaxRT2Commit = max(m_nMaxRT2Commit, nTarget);







  return (hr == S_OK);
}
bool CD3D9Renderer::FX_PushRenderTarget(int nTarget, D3DSurface *pTargetSurf, SD3DSurface *pDepthTarget, bool bClearOnResolve, uint32 nTileCount)
{
  assert(m_pRT->IsRenderThread());
  if (m_nRTStackLevel[nTarget] >= MAX_RT_STACK)
    return false;
  m_nRTStackLevel[nTarget]++;
  return FX_SetRenderTarget(nTarget, pTargetSurf, pDepthTarget, bClearOnResolve, nTileCount);
}

bool CD3D9Renderer::FX_SetRenderTarget(int nTarget, CTexture *pTarget, SD3DSurface *pDepthTarget, bool bPush, bool bClearOnResolve, int nCMSide, bool bScreenVP, uint32 nTileCount)
{
  assert(!nTarget || !pDepthTarget);
	assert((unsigned int) nTarget < 4);

  if (pTarget && pDepthTarget)
  {
#ifndef XENON
    if (pTarget->GetWidth() > pDepthTarget->nWidth || pTarget->GetHeight() > pDepthTarget->nHeight)
    {
      iLog->LogError("Error: RenderTarget '%s' size:%i x %i DepthSurface size:%i x %i \n", pTarget->GetName(), pTarget->GetWidth(), pTarget->GetHeight(), pDepthTarget->nWidth, pDepthTarget->nHeight);
    }
    assert(pTarget->GetWidth() <= pDepthTarget->nWidth);
    assert(pTarget->GetHeight() <= pDepthTarget->nHeight);
#endif
  }

  if (m_nRTStackLevel[nTarget] >= MAX_RT_STACK)
    return false;

  SRTStack *pCur = &m_RTStack[nTarget][m_nRTStackLevel[nTarget]];
  D3DSurface* pTargSurf;
  bool bFP16 = false;
  if (pCur->m_pTex)
  {
    bFP16 = pCur->m_pTex->IsHighQualityRT();
    if (pCur->m_bNeedReleaseRT)
    {
      pCur->m_bNeedReleaseRT = false;




    }
    if (pCur->m_pTarget && pCur->m_pTarget == m_pNewTarget[0]->m_pTarget)
    {



    }
    m_pNewTarget[0]->m_bWasSetRT = false;
#if defined (DIRECT3D9) || defined (OPENGL)
    SAFE_RELEASE(pCur->m_pTarget);
#endif
    m_pNewTarget[0]->m_pTarget = NULL;

    pCur->m_pTex->Unlock();
  }

#if defined (DIRECT3D10)
  if (!pTarget)
    pTargSurf = NULL;
  else
  {
    if (!FX_GetTargetSurfaces(pTarget, pTargSurf, pCur, nCMSide, nTarget, bFP16, nTileCount))
      return false;
  }
#else
  if (!pTarget)
    return false;
  if (!FX_GetTargetSurfaces(pTarget, pTargSurf, pCur, nCMSide, nTarget, bFP16, nTileCount))
    return false;
#endif

  if (pTarget)
  {
    int nFrameID = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID;
    if (pTarget && pTarget->m_nUpdateFrameID != nFrameID)
    {
      pTarget->m_nUpdateFrameID = nFrameID;
    }
  }

  if (!bPush && pDepthTarget && pDepthTarget->pSurf != pCur->m_pDepth)
  {
    //assert(pCur->m_pDepth == m_pCurDepth);
    //assert(pCur->m_pDepth != m_pZBuffer);   // Attempt to override default Z-buffer surface
    if (pCur->m_pSurfDepth)
      pCur->m_pSurfDepth->bBusy = false;
  }
  pCur->m_pDepth = pDepthTarget ? (D3DDepthSurface *)pDepthTarget->pSurf : NULL;
  pCur->m_ClearFlags = 0;
  pCur->m_pTarget = pTargSurf;
  pCur->m_bNeedReleaseRT = true;
  pCur->m_bWasSetRT = false;
  pCur->m_bWasSetD = false;
  pCur->m_bScreenVP = bScreenVP;
  pCur->m_bClearOnResolve = bClearOnResolve;



  if (pTarget)
    pTarget->Lock();
  if (pDepthTarget)
  {
    pDepthTarget->bBusy = true;
    pDepthTarget->nFrameAccess = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID;
  }

  if (pTarget)
    pCur->m_pTex = pTarget;
  else if(pDepthTarget)
		pCur->m_pTex = (CTexture*)pDepthTarget->pTex;
	else
		pCur->m_pTex = NULL;

  pCur->m_pSurfDepth = pDepthTarget;

  if (pTarget)
  {
    pCur->m_Width = pTarget->GetWidth();
    pCur->m_Height = pTarget->GetHeight();
  }
  else 
  if (pDepthTarget)
  {
    pCur->m_Width = pDepthTarget->nWidth;
    pCur->m_Height = pDepthTarget->nHeight;
  }
  if (!nTarget)
  {
#if defined (DIRECT3D9) || defined (OPENGL)
    m_CurViewport.nWidth = pCur->m_Width;
    m_CurViewport.nHeight = pCur->m_Height;
#endif
    if (bScreenVP)
      RT_SetViewport(m_MainViewport.nX, m_MainViewport.nY, m_MainViewport.nWidth, m_MainViewport.nHeight);
    else
      RT_SetViewport(0, 0, pCur->m_Width, pCur->m_Height);
  }
  m_pNewTarget[nTarget] = pCur;
  m_nMaxRT2Commit = max(m_nMaxRT2Commit, nTarget);    

  return true;
}
bool CD3D9Renderer::FX_PushRenderTarget(int nTarget, CTexture *pTarget, SD3DSurface *pDepthTarget, bool bClearOnResolve, int nCMSide, bool bScreenVP, uint32 nTileCount)
{
  assert(m_pRT->IsRenderThread());

  if (m_nRTStackLevel[nTarget] == MAX_RT_STACK)
  {
    assert(0);
    return false;
  }
  m_nRTStackLevel[nTarget]++;
  return FX_SetRenderTarget(nTarget, pTarget, pDepthTarget, true, bClearOnResolve, nCMSide, bScreenVP, nTileCount);
}

bool CD3D9Renderer::FX_RestoreRenderTarget(int nTarget)
{
  if (m_nRTStackLevel[nTarget] < 0)
    return false;

  SRTStack *pCur = &m_RTStack[nTarget][m_nRTStackLevel[nTarget]];
#ifdef _DEBUG
  if (m_nRTStackLevel[nTarget] == 0 && nTarget == 0)
  {
    assert(pCur->m_pTarget == m_pBackBuffer && pCur->m_pDepth == m_pZBuffer);
  }
#endif

  SRTStack *pPrev = &m_RTStack[nTarget][m_nRTStackLevel[nTarget]+1];
  if (pPrev->m_bNeedReleaseRT)
  {
    pPrev->m_bNeedReleaseRT = false;




    if (pPrev->m_pTarget && pPrev->m_pTarget == m_pNewTarget[nTarget]->m_pTarget)
    {
      m_pNewTarget[nTarget]->m_bWasSetRT = false;


#if defined(DIRECT3D9)
      if( nTarget )
        m_pd3dDevice->SetRenderTarget(nTarget, NULL);
#endif

#if defined (DIRECT3D9) || defined (OPENGL)
      pPrev->m_pTarget->Release();
#endif
      pPrev->m_pTarget = NULL;
      m_pNewTarget[nTarget]->m_pTarget = NULL;
    }
  }

  if (nTarget == 0)
  {
    if (pPrev->m_pSurfDepth)
    {
      pPrev->m_pSurfDepth->bBusy = false;
      pPrev->m_pSurfDepth = NULL;
    }
  }
  if (pPrev->m_pTex)
  {
    pPrev->m_pTex->Unlock();
    pPrev->m_pTex = NULL;
  }
  if (!nTarget)
  {
#if defined (DIRECT3D9) || defined (OPENGL)
    m_CurViewport.nWidth = pCur->m_Width;
    m_CurViewport.nHeight = pCur->m_Height;
#endif
    if (pCur->m_bScreenVP)
      RT_SetViewport(m_MainViewport.nX, m_MainViewport.nY, m_MainViewport.nWidth, m_MainViewport.nHeight);
    else
    if (!m_nRTStackLevel[nTarget])
      RT_SetViewport(0, 0, GetWidth(), GetHeight());
    else
      RT_SetViewport(0, 0, pCur->m_Width, pCur->m_Height);
  }
  pCur->m_bWasSetD = false;
  pCur->m_bWasSetRT = false;
  m_pNewTarget[nTarget] = pCur;
  m_nMaxRT2Commit = max(m_nMaxRT2Commit, nTarget );
  



  return true;
}
bool CD3D9Renderer::FX_PopRenderTarget(int nTarget)
{
  assert(m_pRT->IsRenderThread());
  if (m_nRTStackLevel[nTarget] <= 0)
  {
    assert(0);
    return false;
  }
  m_nRTStackLevel[nTarget]--;
  return FX_RestoreRenderTarget(nTarget);
}

//XENON version


















































































     
SD3DSurface *CD3D9Renderer::FX_GetDepthSurface(int nWidth, int nHeight, bool bAA)
{
  assert(m_pRT->IsRenderThread());

  SD3DSurface *pSrf = NULL;
  uint32 i;
  int nBestX = -1;
  int nBestY = -1;
  for (i=0; i<m_TempDepths.Num(); i++)
  {
    pSrf = m_TempDepths[i];
    if (!pSrf->bBusy)
    {
      if (pSrf->nWidth == nWidth && pSrf->nHeight == nHeight)
      {
        nBestX = i;
        break;
      }
#if !defined(OPENGL)
      // Need an exact match for OpenGL.
      if (nBestX < 0 && pSrf->nWidth == nWidth && pSrf->nHeight >= nHeight)
        nBestX = i;
      else
        if (nBestY < 0 && pSrf->nWidth >= nWidth && pSrf->nHeight == nHeight)
          nBestY = i;
#endif
    }
  }
  if (nBestX >= 0)
    return m_TempDepths[nBestX];
  if (nBestY >= 0)
    return m_TempDepths[nBestY];

#if !defined(PS3)
  for (i=0; i<m_TempDepths.Num(); i++)
  {
    pSrf = m_TempDepths[i];
    if (pSrf->nWidth >= nWidth && pSrf->nHeight >= nHeight && !pSrf->bBusy)
      break;
  }


#endif
  if (i == m_TempDepths.Num())
  {
    pSrf = new SD3DSurface;
    pSrf->nWidth = nWidth;
    pSrf->nHeight = nHeight;
    pSrf->nFrameAccess = -1;
    pSrf->bBusy = false;




























#if defined (DIRECT3D9) || defined(OPENGL)
    m_pd3dDevice->CreateDepthStencilSurface(nWidth, nHeight, m_ZFormat, D3DMULTISAMPLE_NONE, 0, FALSE, (LPDIRECT3DSURFACE9 *)(&pSrf->pSurf), NULL);
#elif defined (DIRECT3D10)
    HRESULT hr;
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroStruct(descDepth);
    descDepth.Width = nWidth;
    descDepth.Height = nHeight;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = m_ZFormat;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = m_pd3dDevice->CreateTexture2D(&descDepth,       // Texture desc
      NULL,                  // Initial data
      (ID3D11Texture2D **)(&pSrf->pTex)); // [out] Texture
    assert(hr == S_OK);

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroStruct(descDSV);
    descDSV.Format = m_ZFormat;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;

    // Create the depth stencil view
    hr = m_pd3dDevice->CreateDepthStencilView((ID3D11Texture2D *)pSrf->pTex, // Depth stencil texture
      &descDSV, // Depth stencil desc
      (ID3D11DepthStencilView **)(&pSrf->pSurf));  // [out] Depth stencil view
#endif
    m_TempDepths.AddElem(pSrf);
  }

  return pSrf;
}



SD3DSurface *CD3D9Renderer::FX_GetScreenDepthSurface(bool bAA)
{
  SD3DSurface *pSurf = FX_GetDepthSurface(m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height, bAA);
  assert(pSurf);
  return pSurf;
}


//============================================================================================
_inline void sCopyInds8(uint32 *dinds, uint32 *inds, int nInds8, int n)
{
  if (!nInds8)
    return;
#ifdef DO_ASM
  _asm
  {
    push       ebx
    mov        edi, dinds
    mov        esi, inds
    mov        ecx, nInds8
    mov        eax, n
align 4
_Loop:
    prefetchT0  [esi+10h]
    mov        edx, [esi]
    add        edx, eax
    mov        [edi], edx
    mov        ebx, [esi+4]
    add        edi, 16
    add        ebx, eax
    mov        [edi+4-16], ebx
    mov        edx, [esi+8]
    add        edx, eax
    add        esi, 16
    mov        [edi+8-16], edx
    mov        ebx, [esi+12-16]
    add        ebx, eax
    dec        ecx
    mov        [edi+12-16], ebx
    jne        _Loop
    pop        ebx
  }
#else
  for (int i=0; i<nInds8; i++, dinds+=4, inds+=4)
  {
    dinds[0] = inds[0] + n;
    dinds[1] = inds[1] + n;
    dinds[2] = inds[2] + n;
    dinds[3] = inds[3] + n;
  }
#endif
}

void sCopyTransf_P_C_T(byte *dst, Matrix44 *mat, int nNumVerts, byte *OffsP)
{
#ifdef DO_ASM
  _asm
  {
    mov         eax, mat
    mov         ecx, nNumVerts;
    movaps      xmm2,xmmword ptr [eax]
    mov         esi, OffsP
    movaps      xmm4,xmmword ptr [eax+10h]
    movaps      xmm6,xmmword ptr [eax+20h]
    mov         edi, dst
    movaps      xmm5,xmmword ptr [eax+30h]
align 16
_Loop1:
    prefetchT0  [esi+24]
    movlps      xmm1,qword ptr [esi]
    movss       xmm0,dword ptr [esi+8]
    shufps      xmm0,xmm0,0
    add         edi, 24
    movaps      xmm3,xmm1
    mulps       xmm0,xmm6
    mov         eax, [esi+12]
    shufps      xmm3,xmm1,55h
    mulps       xmm3,xmm4
    mov         [edi+12-24], eax
    shufps      xmm1,xmm1,0
    mulps       xmm1,xmm2
    mov         eax, [esi+16]
    addps       xmm3,xmm1
    mov         [edi+16-24], eax
    addps       xmm3,xmm0
    mov         eax, [esi+20]
    addps       xmm3,xmm5
    add         esi, 24
    movhlps     xmm1,xmm3
    movlps      qword ptr [edi-24],xmm3
    dec         ecx
    mov         [edi+20-24], eax
    movss       dword ptr [edi+8-24],xmm1
    jne         _Loop1
  }
#else
  SVF_P3F_C4B_T2F *pDst = (SVF_P3F_C4B_T2F *)dst;
  for (int i=0; i<nNumVerts; i++, OffsP+=sizeof(SVF_P3F_C4B_T2F))
  {
    //pDst->xyz = mat->TransformPoint(*(Vec3 *)OffsP);
    TransformPosition(pDst->xyz, *(Vec3 *)OffsP, *mat);
    pDst->color.dcolor = *(DWORD *)&OffsP[12];
    pDst->st.x = *(float *)&OffsP[16];
    pDst->st.y = *(float *)&OffsP[20];
    pDst++;
  }
#endif
}

float f3 = 32767.0f;
float fi3 = 1.0f/32767.0f;
//DEFINE_ALIGNED_DATA(int, val[4], 16);
void sCopyTransf_TN(byte *dst, Matrix33 *mat, int nNumVerts, byte *OffsP)
{
#ifdef DO_ASM
  _asm
  {
    mov ecx,    nNumVerts;
    mov eax,    mat
    mov esi,    OffsP
    mov edi,    dst
    movaps      xmm2,xmmword ptr [eax]
    movaps      xmm4,xmmword ptr [eax+10h]
    movaps      xmm6,xmmword ptr [eax+20h]
align 16
_Loop:
    movsx       eax, word ptr [esi+4]
    cvtsi2ss    xmm0, eax
    shufps      xmm0,xmm0,0
    prefetcht0  [esi+10h] 
    mulps       xmm0,xmm6
    movsx       eax, word ptr [esi+2]
    cvtsi2ss    xmm3, eax
    mulps       xmm3,xmm4
    movsx       eax, word ptr [esi+0]
    cvtsi2ss    xmm1, eax
    mulps       xmm1,xmm2
    addps       xmm3,xmm1
    addps       xmm3,xmm0
    mov         ax, word ptr [esi+6]
    movaps      xmm1,xmm3     // r1 = vx, vy, vz, X
    mulps		    xmm1,xmm3			// r1 = vx * vx, vy * vy, vz * vz, X
    movhlps		  xmm5,xmm1			// r5 = vz * vz, X, X, X
    movaps		  xmm0,xmm1			// r0 = r1
    mov         word ptr [edi+6], ax
    shufps	  	xmm0,xmm0, 1	// r0 = vy * vy, X, X, X
    addss	      xmm1,xmm0			// r0 = (vx * vx) + (vy * vy), X, X, X
    addss	      xmm1,xmm5			// r1 = (vx * vx) + (vy * vy) + (vz * vz), X, X, X
    sqrtss	    xmm1,xmm1			// r1 = sqrt((vx * vx) + (vy * vy) + (vz * vz)), X, X, X
    rcpss		    xmm1,xmm1			// r1 = 1/radius, X, X, X
    shufps		  xmm1,xmm1, 0	// r1 = 1/radius, 1/radius, 1/radius, X
    mulps		    xmm3,xmm1			// r3 = vx * 1/radius, vy * 1/radius, vz * 1/radius, X
    movhlps     xmm5,xmm3
    cvtss2si    eax, xmm3
    mov         word ptr [edi+0], ax
    cvtss2si    eax, xmm5
    mov         word ptr [edi+4], ax
    shufps	  	xmm3,xmm3, 1
    cvtss2si    eax, xmm3
    mov         word ptr [edi+2], ax

    movsx       eax, word ptr [esi+12]
    cvtsi2ss    xmm0, eax
    shufps      xmm0,xmm0,0
    mulps       xmm0,xmm6
    movsx       eax, word ptr [esi+10]
    cvtsi2ss    xmm3, eax
    mulps       xmm3,xmm4
    movsx       eax, word ptr [esi+8]
    cvtsi2ss    xmm1, eax
    mulps       xmm1,xmm2
    addps       xmm3,xmm1
    addps       xmm3,xmm0
    mov         ax, word ptr [esi+14]
    movaps      xmm1,xmm3     // r1 = vx, vy, vz, X
    mulps		    xmm1,xmm3			// r1 = vx * vx, vy * vy, vz * vz, X
    movhlps		  xmm5,xmm1			// r5 = vz * vz, X, X, X
    movaps		  xmm0,xmm1			// r0 = r1
    mov         word ptr [edi+14], ax
    shufps	  	xmm0,xmm0, 1	// r0 = vy * vy, X, X, X
    addss	      xmm1,xmm0			// r0 = (vx * vx) + (vy * vy), X, X, X
    addss	      xmm1,xmm5			// r1 = (vx * vx) + (vy * vy) + (vz * vz), X, X, X
    sqrtss	    xmm1,xmm1			// r1 = sqrt((vx * vx) + (vy * vy) + (vz * vz)), X, X, X
    rcpss		    xmm1,xmm1			// r1 = 1/radius, X, X, X
    shufps		  xmm1,xmm1, 0	// r1 = 1/radius, 1/radius, 1/radius, X
    mulps		    xmm3,xmm1			// r3 = vx * 1/radius, vy * 1/radius, vz * 1/radius, X
    movhlps     xmm5,xmm3
    cvtss2si    eax, xmm3
    mov         word ptr [edi+8], ax
    cvtss2si    eax, xmm5
    mov         word ptr [edi+12], ax
    shufps	  	xmm3,xmm3, 1
    cvtss2si    eax, xmm3
    mov         word ptr [edi+10], ax

    add         esi, 16
    add         edi, 16
    dec         ecx
    jne         _Loop
  }
#else
  SPipTangents *pDst = (SPipTangents *)dst;
  SPipTangents *pSrc = (SPipTangents *)OffsP;
  for (int i=0; i<nNumVerts; i++, pSrc++, pDst++)
  {
    Vec3 v;
    v.x = tPackB2F(pSrc->Binormal.x);
    v.y = tPackB2F(pSrc->Binormal.y);
    v.y = tPackB2F(pSrc->Binormal.z);
    pDst->Binormal.x = tPackF2B(v.Dot(mat->GetColumn0()));
    pDst->Binormal.y = tPackF2B(v.Dot(mat->GetColumn1()));
    pDst->Binormal.z = tPackF2B(v.Dot(mat->GetColumn2()));
    pDst->Binormal.w = pSrc->Binormal.w;

    v.x = tPackB2F(pSrc->Tangent.x);
    v.y = tPackB2F(pSrc->Tangent.y);
    v.y = tPackB2F(pSrc->Tangent.z);
    pDst->Tangent.x = tPackF2B(v.Dot(mat->GetColumn0()));
    pDst->Tangent.y = tPackF2B(v.Dot(mat->GetColumn1()));
    pDst->Tangent.z = tPackF2B(v.Dot(mat->GetColumn2()));
    pDst->Tangent.w = pDst->Tangent.w;
  }
#endif
}

// Commit changed states to the hardware before drawing
bool CD3D9Renderer::FX_CommitStreams(SShaderPass *sl, bool bSetVertexDecl)
{
  bool bRet = true;

  //PROFILE_FRAME(Draw_Predraw);
  SRenderPipeline& RESTRICT_REFERENCE rp(m_RP);

  HRESULT hr;	
  if (bSetVertexDecl)
  {
    hr = FX_SetVertexDeclaration(rp.m_FlagsStreams_Decl, rp.m_CurVFormat);
    if (FAILED(hr))
      return false;
  }

  if (!rp.m_pRE && rp.m_RendNumVerts)
  {
    if (!(rp.m_FlagsPerFlush & RBSI_VERTSMERGED))
    {
			uint32 nStart;
			uint32 nSize = rp.m_Stride*rp.m_RendNumVerts;
      rp.m_FlagsPerFlush |= RBSI_VERTSMERGED;
      void *pVB = FX_LockVB(nSize, nStart);
#ifndef XENON
      memcpy(pVB, rp.m_Ptr.Ptr, nSize);


#endif
      FX_UnlockVB();
      rp.m_FirstVertex = 0;
      rp.m_MergedStreams[0] = rp.m_VBs[rp.m_CurVB];
      rp.m_nStreamOffset[0] = nStart;
      rp.m_PS[rp.m_nProcessThreadID].m_DynMeshUpdateBytes += nSize;

      uint16 *pIB = rp.m_IndexBuf->Lock(rp.m_RendNumIndices, nStart);
#ifndef XENON
      memcpy(pIB, rp.m_SysRendIndices, rp.m_RendNumIndices * sizeof(short));


#endif
      rp.m_IndexBuf->Unlock();
      rp.m_FirstIndex = nStart;
      rp.m_PS[rp.m_nProcessThreadID].m_DynMeshUpdateBytes += rp.m_RendNumIndices*sizeof(short);
    }
    rp.m_IndexBuf->Bind();

    if (rp.m_FlagsStreams_Stream & VSM_TANGENTS)
    {
      if (!(rp.m_FlagsPerFlush & RBSI_TANGSMERGED))
      {
				uint32 nStart;
        uint32 nSize = sizeof(SPipTangents)*rp.m_RendNumVerts;
        rp.m_FlagsPerFlush |= RBSI_TANGSMERGED;
        void *pVB = FX_LockVB(nSize, nStart);
        memcpy(pVB, rp.m_PtrTang.Ptr, nSize);
        FX_UnlockVB();
        rp.m_PS[rp.m_nProcessThreadID].m_DynMeshUpdateBytes += nSize;
        rp.m_MergedStreams[VSF_TANGENTS] = rp.m_VBs[rp.m_CurVB];
        rp.m_nStreamOffset[VSF_TANGENTS] = nStart;
      }
      rp.m_MergedStreams[VSF_TANGENTS].VBPtr_0->Bind(VSF_TANGENTS, rp.m_nStreamOffset[VSF_TANGENTS], sizeof(SPipTangents));
      rp.m_TI[rp.m_nProcessThreadID].m_PersFlags |= RBPF_USESTREAM<<VSF_TANGENTS;
    }
    else
    if (rp.m_TI[rp.m_nProcessThreadID].m_PersFlags & (RBPF_USESTREAM<<(VSF_TANGENTS | VSF_QTANGENTS)))
    {
      rp.m_TI[rp.m_nProcessThreadID].m_PersFlags &= ~(RBPF_USESTREAM<<(VSF_TANGENTS | VSF_QTANGENTS));
      FX_SetVStream(1, NULL, 0, 0);
    }
		rp.m_MergedStreams[0].VBPtr_0->Bind(0, rp.m_nStreamOffset[0], rp.m_Stride);
  }
  else
  if (rp.m_pRE)
    bRet = rp.m_pRE->mfPreDraw(sl);

  return bRet;
}



bool CD3D9Renderer::FX_DebugCheckConsistency(int FirstVertex, int FirstIndex, int RendNumVerts, int RendNumIndices)
{
  if (CV_r_validateDraw != 2)
    return true;
  HRESULT hr = S_OK;
  assert(m_RP.m_VertexStreams[0].pStream);
  //assert(m_RP.m_VertexStreams[0].nFreq == 1);
  D3DVertexBuffer *pVB = (D3DVertexBuffer *)m_RP.m_VertexStreams[0].pStream;
  D3DIndexBuffer *pIB = (D3DIndexBuffer *)m_RP.m_pIndexStream;
  assert(pIB && pVB);
  if (!pIB || !pVB)
    return false;
  int i;
  int nVBOffset = m_RP.m_VertexStreams[0].nOffset;

  uint16 *pIBData = (uint16 *)m_DevBufMan.LockIB(pIB, FirstIndex, FSL_READ);
  byte *pVBData = (byte *)m_DevBufMan.LockVB(pVB, nVBOffset, FSL_READ);
  EVertexFormat eVBFormat = m_RP.m_CurVFormat;
  int nVBStride = CRenderMesh2::m_cSizeVF[eVBFormat];
  SOnDemandD3DVertexDeclarationCache *pDecl = &m_RP.m_D3DVertexDeclarationCache[(m_RP.m_FlagsStreams_Decl&0xff)>>1][eVBFormat][0];
  assert(pDecl->m_pDeclaration);

  Vec3 vMax, vMin;
  vMin = Vec3(100000.0f, 100000.0f, 100000.0f);
  vMax = Vec3(-100000.0f, -100000.0f, -100000.0f);

  for (i=0; i<RendNumIndices; i++)
  {
    int nInd = pIBData[i];
    assert(nInd>=FirstVertex && nInd<FirstVertex+RendNumVerts);
    byte *pV = &pVBData[nInd*nVBStride];
    Vec3 VV = ((Vec3f16 *)pV)->ToVec3();
    vMin.CheckMin(VV);
    vMax.CheckMax(VV);
    Vec3 vAbs = VV.abs();
    assert(vAbs.x < 10000.0f && vAbs.y < 10000.0f && vAbs.z < 10000.0f);
    if(vAbs.x > 10000.0f || vAbs.y > 10000.0f || vAbs.z > 10000.0f || !_finite(vAbs.x) || !_finite(vAbs.y) || !_finite(vAbs.z))
      hr = S_FALSE;
  }
  Vec3 vDif = vMax - vMin;
  assert(vDif.x < 10000.0f && vDif.y < 10000.0f && vDif.z < 10000.0f);
  if (vDif.x >= 10000.0f || vDif.y > 10000.0f || vDif.z > 10000.0f)
    hr = S_FALSE;

  m_DevBufMan.UnlockIB(pIB);
  m_DevBufMan.UnlockVB(pVB);
  if (hr != S_OK)
  {
    iLog->LogError("ERROR: CD3D9Renderer::FX_DebugCheckConsistency: Validation failed for DIP (Shader: '%s')\n", m_RP.m_pShader->GetName());
  }
  return (hr==S_OK);
}

// Draw current indexed mesh
void CD3D9Renderer::FX_DrawIndexedMesh (int nPrimType)
{
  HRESULT h = 0;

  FX_Commit();

  if (CV_r_nodrawshaders)
    return;

  int nFaces;

  PROFILE_FRAME(Draw_DrawCall);

#if defined (DIRECT3D9) || defined(OPENGL)
  D3DPRIMITIVETYPE nType;
  switch (nPrimType)
  {
  case R_PRIMV_LINES:
    nType = D3DPT_LINELIST;
    nFaces = m_RP.m_RendNumIndices/2;
    break;

  case R_PRIMV_TRIANGLES:
    nType = D3DPT_TRIANGLELIST;
    nFaces = m_RP.m_RendNumIndices/3;
    break;

  case R_PRIMV_TRIANGLE_STRIP:
    nType = D3DPT_TRIANGLESTRIP;
    nFaces = m_RP.m_RendNumIndices-2;
    break;

  case R_PRIMV_TRIANGLE_FAN:
    nType = D3DPT_TRIANGLEFAN;
    nFaces = m_RP.m_RendNumIndices-2;
    break;

  case R_PRIMV_MULTI_STRIPS:
    {
      PodArray<CRenderChunk> *mats = m_RP.m_pRE->mfGetMatInfoList();
      if (mats)
      {
        CRenderChunk *m = mats->Get(0);
        for (int i=0; i<mats->Count(); i++, m++)
        {
          FX_DebugCheckConsistency(m->nFirstVertId, m->nFirstIndexId+m_RP.m_IndexOffset, m->nNumVerts, m->nNumIndices);
          if (FAILED(h=m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, m->nFirstVertId, m->nNumVerts, m->nFirstIndexId+m_RP.m_IndexOffset, m->nNumIndices - 2)))
          {
            Error("CD3D9Renderer::FX_DrawIndexedMesh: DrawIndexedPrimitive error", h);
            return;
          }
          m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += (m->nNumIndices - 2);
          m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
        }
      }
      return;
    }
    break;

  case R_PRIMV_HWSKIN_GROUPS:
    {
      CRenderChunk *pChunk = m_RP.m_pRE->mfGetMatInfo();
      if (pChunk)
      {
        // SHWSkinBatch *pBatch = pChunk->m_pHWSkinBatch;
        FX_DebugCheckConsistency(m_RP.m_FirstVertex, pChunk->nFirstIndexId+m_RP.m_IndexOffset, pChunk->nNumVerts, pChunk->nNumIndices);
        if (FAILED(h=m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, m_RP.m_FirstVertex, pChunk->nNumVerts, pChunk->nFirstIndexId+m_RP.m_IndexOffset, pChunk->nNumIndices / 3)))
        {
          Error("CD3D9Renderer::FX_DrawIndexedMesh: DrawIndexedPrimitive error", h);
          return;
        }
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += (pChunk->nNumIndices / 3);
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
      }
      return;
    }
    break;

  default:
    assert(0);
  }

  if (nFaces)
  {
    FX_DebugCheckConsistency(m_RP.m_FirstVertex, m_RP.m_FirstIndex+m_RP.m_IndexOffset, m_RP.m_RendNumVerts, m_RP.m_RendNumIndices);

#ifdef ENABLE_FRAME_PROFILER_LABELS
		if(m_RP.m_pCurObject->m_pRenderNode && m_RP.m_pCurObject->m_pRenderNode == m_pDebugRenderNode)
		{
			PROFILE_LABEL_PUSH("Debug Node");
		}
#endif

    if (FAILED(h=m_pd3dDevice->DrawIndexedPrimitive(nType, 0, m_RP.m_FirstVertex, m_RP.m_RendNumVerts, m_RP.m_FirstIndex+m_RP.m_IndexOffset, nFaces)))
    {
      Error("CD3D9Renderer::FX_DrawIndexedMesh: DrawIndexedPrimitive error", h);
      return;
    }
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += nFaces;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
		
#ifdef ENABLE_FRAME_PROFILER_LABELS
		if(m_RP.m_pCurObject->m_pRenderNode && m_RP.m_pCurObject->m_pRenderNode == m_pDebugRenderNode)
		{
			PROFILE_LABEL_POP("Debug Node");
		}
#endif
  }
#elif defined (DIRECT3D10)
  // Don't render fallback in DX11
  if (!CHWShader_D3D::m_pCurInstVS || !CHWShader_D3D::m_pCurInstPS || CHWShader_D3D::m_pCurInstVS->m_bFallback || CHWShader_D3D::m_pCurInstPS->m_bFallback)
    return;
  if (CHWShader_D3D::m_pCurInstGS && CHWShader_D3D::m_pCurInstGS->m_bFallback)
    return;

  D3D11_PRIMITIVE_TOPOLOGY nType;
  switch (nPrimType)
  {
  case R_PRIMV_LINES:
    nType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
    nFaces = m_RP.m_RendNumIndices/2;
    break;

  case R_PRIMV_TRIANGLES:
    nType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    nFaces = m_RP.m_RendNumIndices/3;
    break;

  case R_PRIMV_TRIANGLE_STRIP:
    nType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    nFaces = m_RP.m_RendNumIndices-2;
    break;

  case R_PRIMV_TRIANGLE_FAN:
    assert(0);
    break;

  case R_PRIMV_MULTI_STRIPS:
    {
      PodArray<CRenderChunk> *mats = m_RP.m_pRE->mfGetMatInfoList();
      if (mats)
      {
				SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        CRenderChunk *m = mats->Get(0);
        for (int i=0; i<mats->Count(); i++, m++)
        {
          m_pd3dDeviceContext->DrawIndexed(m->nNumIndices, m->nFirstIndexId+m_RP.m_IndexOffset, m->nFirstVertId);
          m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += (m->nNumIndices - 2);
          m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
        }
      }
      return;
    }
    break;

  case R_PRIMV_HWSKIN_GROUPS:
    {
      CRenderChunk *pChunk = m_RP.m_pRE->mfGetMatInfo();
      if (pChunk)
      {
        FX_DebugCheckConsistency(m_RP.m_FirstVertex, m_RP.m_FirstIndex+m_RP.m_IndexOffset, m_RP.m_RendNumVerts, m_RP.m_RendNumIndices);

        // SHWSkinBatch *pBatch = pChunk->m_pHWSkinBatch;        
				SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_pd3dDeviceContext->DrawIndexed(pChunk->nNumIndices, pChunk->nFirstIndexId+m_RP.m_IndexOffset, 0);
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += (pChunk->nNumIndices / 3);
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
      }
      return;
    }
    break;

  default:
    assert(0);
  }

  if (nFaces)
  {
		SetPrimitiveTopology(nType);
    m_pd3dDeviceContext->DrawIndexed(m_RP.m_RendNumIndices, m_RP.m_FirstIndex+m_RP.m_IndexOffset, 0);
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += nFaces;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
  }
#endif
}


//====================================================================================

struct SShadowLight
{
  PodArray<ShadowMapFrustum*> *pSmLI;
  CDLight *pDL;
};

static bool sbHasDot3LM;
static _inline int Compare(SShadowLight &a, SShadowLight &b)
{
  if (a.pSmLI->Count() > b.pSmLI->Count())
    return -1;
  if (a.pSmLI->Count() < b.pSmLI->Count())
    return 1;
  if (sbHasDot3LM)
  {
    if ((a.pDL->m_Flags & DLF_LM) < (b.pDL->m_Flags & DLF_LM))
      return -1;
    if ((a.pDL->m_Flags & DLF_LM) > (b.pDL->m_Flags & DLF_LM))
      return -1;
  }
  return 0;
}

static void sFillLP(SLightPass *pLP, uint32& i, CDLight** pLight, int nLights, const int nMaxLights, const int nMaxAmbLights, bool& bHasAmb)
{
  if (!nLights || !nMaxLights || (!nMaxAmbLights && bHasAmb))
    return;
  int j, n, m;
  int nActualMaxLights = bHasAmb ? nMaxAmbLights : nMaxLights;
  bHasAmb = false;
  m = 0;

  for (j=0; j<32; j++)
  {
    nActualMaxLights = (nLights < nActualMaxLights) ? nLights : nActualMaxLights;
    if (!nActualMaxLights)
      break;
#ifndef XENON
    if (CRenderer::CV_r_shadersdynamicbranching)
    {
      // With dynamic branching we cannot access lights form different groups in single pass
      int nGroup = pLight[m]->m_Id>>2;
      for (n=0; n<nActualMaxLights; n++)
      {
        if ((pLight[m]->m_Id>>2) != nGroup)
          break;
        pLP[i].pLights[n] = pLight[m++];
      }
    }
    else
#endif
    for (n=0; n<nActualMaxLights; n++)
    {
      pLP[i].pLights[n] = pLight[m++];
    }
    pLP[i].nLights = n;
    i++;
    nLights -= n;
    nActualMaxLights = nMaxLights;
  }
}

void CD3D9Renderer::FX_StencilLights(SLightPass *pLP)
{
  if (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] > 1)
    return;
  if (!pLP->nLights || (m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_AMBIENT]))
  {
    EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
      STENCOP_PASS(FSS_STENCOP_KEEP),
      0x0, 0xF/*pLP->nStencLTMask*/, 0xFFFFFFFF
      );
  }
  else
  {

    //FX_StencilRefresh(STENC_FUNC(FSS_STENCFUNC_NOTEQUAL), 0x0, pLP->nStencLTMask);
    //uint32 nStencilMask = 1<<(nLod - 1);
    EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_NOTEQUAL) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
      STENCOP_PASS(FSS_STENCOP_KEEP),
      0x0, 0xF/*pLP->nStencLTMask*/, 0xFFFFFFFF
    );
  }
  return;
}

int CD3D9Renderer::FX_SetupLightPasses(SShaderTechnique *pTech)
{
  uint32 i;
  if (!pTech->m_Passes.Num())
    return -1;
  if (pTech->m_Flags & FHF_NOLIGHTS)
  {
    if (m_RP.m_pSunLight && (m_RP.m_DynLMask & 1))
    {
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_AMBIENT];
      if (m_RP.m_ObjFlags & FOB_AMBIENT_OCCLUSION)
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_AMBIENT_OCCLUSION];
    }
    m_RP.m_LPasses[0].nLights = 0;
    m_RP.m_LPasses[0].nLTMask = 0;
    m_RP.m_nCurLightPasses = 1;
    m_RP.m_PrevLMask = 0;
    return 1;
  }
  int nLightGroup = (m_RP.m_nBatchFilter & (FB_ZPREPASS|FB_Z)) ? 0 : m_RP.m_nCurLightGroup;
  uint32 nAffectMask = nLightGroup < 0 ? -1 : (0xf << (nLightGroup*4));
  uint32 nMask = m_RP.m_DynLMask & nAffectMask;
  uint32 nStencGroupLTMask = (nMask >> (nLightGroup*4)) & 0xf;

  bool bHasAmb = false;

  bHasAmb = !m_RP.m_bNotFirstPass;
  if (m_RP.m_nNumRendPasses > 0)
    bHasAmb = false;
  if (bHasAmb)
  {
    bHasAmb = !m_RP.m_bNotFirstPass;
    if (m_RP.m_nNumRendPasses > 0)
      bHasAmb = false;
    if (bHasAmb)
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_AMBIENT];
  }

  if (nMask == m_RP.m_PrevLMask && CV_r_optimisedlightsetup)
    return m_RP.m_nCurLightPasses;

  m_RP.m_PrevLMask = nMask;
  m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumLightSetups++;
  uint32 nFirst = nLightGroup < 0 ? 0 : nLightGroup*4;
  SShaderPass *slw = &pTech->m_Passes[0];
  int nR = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]-1;

  uint32 nLight;
  CRenderObject *pObj = m_RP.m_pCurObject;

  CDLight *DirLights[4];
  int nDirLights = 0;
  CDLight *OmniLights[32];
  int nOmniLights = 0;
  CDLight *ProjLights[32];
  int nProjLights = 0;
  CDLight *dl;
 
  for (nLight=nFirst; nLight<m_RP.m_DLights[m_RP.m_nProcessThreadID][nR].Num(); nLight++)
  {
    if (nMask & (1<<nLight))
    {
      dl = &m_RP.m_DLights[m_RP.m_nProcessThreadID][nR][nLight];
      if ((dl->m_Flags & DLF_DIRECTIONAL) && (CV_r_optimisedlightsetup==2 || CV_r_optimisedlightsetup==3))
        DirLights[nDirLights++] = dl;
      else
      if (dl->m_Flags & DLF_PROJECT)
        ProjLights[nProjLights++] = dl;
      else
        OmniLights[nOmniLights++] = dl;
      if (nLightGroup >= 0)
      {
        dl->m_ShadowChanMask = Vec4(0,0,0,0);
        assert(nLight-nFirst <= 3);
        dl->m_ShadowChanMask[nLight-nFirst] = 1;
      }
    }
  }
  if (!bHasAmb && !nDirLights && !nOmniLights && !nProjLights)
  {
    m_RP.m_nCurLightPasses = -1;
    return -1;
  }
  uint32 nMaxLights = m_RP.m_nMaxLightsPerPass;
  uint32 nMaxAmbLights = nMaxLights;
  if (CV_r_lightssinglepass || (m_RP.m_pShader && (m_RP.m_pShader->m_Flags2 & EF2_SINGLELIGHTPASS) ))
  {
    nMaxLights = 1;
    nMaxAmbLights = 1;
  }
  i = 0;
  sFillLP(&m_RP.m_LPasses[0], i, DirLights,  nDirLights,  nMaxLights, nMaxAmbLights, bHasAmb);
  sFillLP(&m_RP.m_LPasses[0], i, OmniLights, nOmniLights, nMaxLights, nMaxAmbLights, bHasAmb);
  sFillLP(&m_RP.m_LPasses[0], i, ProjLights, nProjLights, min(nMaxLights,1U), min(nMaxAmbLights,1U), bHasAmb);

  //no lightpass for ambient light, we need to create one
  if (bHasAmb)
  {
    m_RP.m_LPasses[0].nLights = 0;
    m_RP.m_LPasses[0].nLTMask = 0;
    m_RP.m_nCurLightPasses = 1;
    return 1;
  }

  int nPasses = i;
  int nPass;
  for (nPass=0; nPass<nPasses; nPass++)
  {
    int Types[4];
    SLightPass *lp = &m_RP.m_LPasses[nPass];
    lp->bRect = false;
    lp->nStencLTMask = 0;//nStencGroupLTMask;
    lp->nLTMask = lp->nLights;
    for (i=0; i<lp->nLights; i++)
    {
      dl = lp->pLights[i];

//////////////////////////////////////////////////////////////////////////
      //TOFIX: don't rely on conseq light groups
      int nSLMask = 1<<dl->m_Id;
      lp->nStencLTMask |= nSLMask >> (nLightGroup*4);
//////////////////////////////////////////////////////////////////////////
      if (dl->m_Flags & DLF_POINT)
        Types[i] = SLMF_POINT;
      else
      if (dl->m_Flags & DLF_PROJECT)
      {
        Types[i] = SLMF_PROJECTED;
        assert(i == 0);
      }
      else
        Types[i] = SLMF_DIRECT;
    }
    switch(lp->nLights)
    {
    case 2:
      if (Types[0] > Types[1])
      {
        Exchange(Types[0], Types[1]);
        Exchange(lp->pLights[0], lp->pLights[1]);
      }
      break;
    case 3:
      if (Types[0] > Types[1])
      {
        Exchange(Types[0], Types[1]);
        Exchange(lp->pLights[0], lp->pLights[1]);
      }
      if (Types[0] > Types[2])
      {
        Exchange(Types[0], Types[2]);
        Exchange(lp->pLights[0], lp->pLights[2]);
      }
      if (Types[1] > Types[2])
      {
        Exchange(Types[1], Types[2]);
        Exchange(lp->pLights[1], lp->pLights[2]);
      }
      break;
    case 4:
      {
        for (int k=0; k<4; k++)
        {
          for (int j=i; j<4; j++)
          {
            if (Types[i] > Types[j])
            {
              Exchange(Types[i], Types[j]);
              Exchange(lp->pLights[i], lp->pLights[j]);
            }
          }
        }
      }
      break;
    }

    for (i=0; i<lp->nLights; i++)
    {
      lp->nLTMask |= Types[i] << (SLMF_LTYPE_SHIFT + i*SLMF_LTYPE_BITS);
    }
  }
  m_RP.m_nCurLightPasses = nPasses;
  if (m_RP.m_nShaderQuality != eSQ_Low)
  {
    if (nPasses == 1)
      CHWShader_D3D::mfSetLightParams(0);
  }
  return nPasses;
}

void CD3D9Renderer::FX_SetLightsScissor(SLightPass *pLP, const RectI* pDef)
{
  if (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] > 1)
    return;
  if (!pLP->nLights || (m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_AMBIENT]))
  {
		if (pDef)
			EF_Scissor(true, pDef->x, pDef->y, pDef->w, pDef->h);
		else
			EF_Scissor(false, 0, 0, 0, 0);
    return;
  }
  if (!pLP->bRect)
  {
    pLP->bRect = true;
    CDLight *pDL = pLP->pLights[0];
    pLP->rc = RectI(pDL->m_sX, pDL->m_sY, pDL->m_sWidth, pDL->m_sHeight);
    for (uint32 i=1; i<pLP->nLights; i++)
    {
      pDL = pLP->pLights[i];
      pLP->rc.Add(pDL->m_sX, pDL->m_sY, pDL->m_sWidth, pDL->m_sHeight);
    }
  }
  if (pLP->rc.w == m_width && pLP->rc.h == m_height)
	{
		if (pDef)
			EF_Scissor(true, pDef->x, pDef->y, pDef->w, pDef->h);
		else
			EF_Scissor(false, 0, 0, 0, 0);
	}
	else
	{
		if (pDef)
		{
			int x1 = max(pDef->x, pLP->rc.x);
			int x2 = min(pDef->x + pDef->w, pLP->rc.x + pLP->rc.w);
			int y1 = max(pDef->y, pLP->rc.y);
			int y2 = min(pDef->y + pDef->h, pLP->rc.y + pLP->rc.h);
			EF_Scissor(true, x1, y1, max(x2-x1, 0), max(y2-y1, 0));
		}
		else
			EF_Scissor(true, pLP->rc.x, pLP->rc.y, pLP->rc.w, pLP->rc.h);
	}
}

void CD3D9Renderer::FX_DrawMultiLightPasses(CShader *ef, SShaderTechnique *pTech, int nShadowChans)
{
  SShaderPass *slw;
  int32 i;

  PROFILE_FRAME(DrawShader_MultiLight_Passes);

  int nPasses = FX_SetupLightPasses(pTech);
  if (nPasses < 0)
    return;

  SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
  SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];
  CRenderObject *pObj = rRP.m_pCurObject;

  if ((rRP.m_ObjFlags & FOB_BLEND_WITH_TERRAIN_COLOR) && rRP.m_pCurObject->m_nTextureID>0)
    rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_BLEND_WITH_TERRAIN_COLOR];

  if (rRP.m_ObjFlags & FOB_AMBIENT_OCCLUSION)
    rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_AMBIENT_OCCLUSION];

  
  //////////////////////////////////////////////////////////////////////////

  PodArray<ShadowMapFrustum*> * lsources = pObj->m_pShadowCasters;
  if ((ef->m_Flags2 & EF2_DEPTHMAP_SUBSURFSCATTER) && lsources)
  {
    if (SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID]!= 1)
    {
      assert(0);
    }
    //set separate scattering depth buffer
    for (i=0; i<lsources->Count(); i++)
    {
      if (lsources->GetAt(i)->bForSubSurfScattering)
      {
        ShadowMapFrustum* pScatterDepthBuffer = lsources->GetAt(i);

        //configure first depth buffer  
        SetupShadowOnlyPass(0, pScatterDepthBuffer, NULL);

        if (pScatterDepthBuffer->bHWPCFCompare)
          m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];

        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SHADOW_MIXED_MAP_G16R16];
        m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3]);
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

        //if (UseSkyLightBasedFog())
        //  m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SKYLIGHT_BASED_FOG];

        //break;
      }
    }
  } 

  //////////////////////////////////////////////////////////////////////////

  int nPass;
  rTI.m_PersFlags |= RBPF_MULTILIGHTS;  
  for (nPass=0; nPass<nPasses; nPass++)
  {
    rRP.m_nCurLightPass = nPass;
    if (nPass && (rRP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_AMBIENT]))
      rRP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_AMBIENT] | g_HWSR_MaskBit[HWSR_AMBIENT_OCCLUSION]);

    if (nPasses > 1 || rRP.m_nShaderQuality == eSQ_Low)
      CHWShader_D3D::mfSetLightParams(nPass);
    SLightPass *lp = &rRP.m_LPasses[nPass];

		if (CV_r_optimisedlightsetup == 2 || CV_r_optimisedlightsetup == 3)
		{
			SRenderObjData* pOD = pObj->GetObjData(rRP.m_nProcessThreadID);
			if (pOD && pOD->m_scissorWidth && pOD->m_scissorHeight)
			{
				const RectI scissorRect(pOD->m_scissorX, pOD->m_scissorY, pOD->m_scissorWidth, pOD->m_scissorHeight);
				FX_SetLightsScissor(lp, &scissorRect);
			}
			else
				FX_SetLightsScissor(lp, 0);
		}
		
    rRP.m_FlagsShader_LT = lp->nLTMask;
#ifdef DO_RENDERLOG
    if (CRenderer::CV_r_log >= 3)
    {
      if (lp->nLights > 0)
      {
        if (nPass)
          Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "\n--- Lights: ");
        else
          Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "--- Lights: ");
        for (uint32 k=0; k<lp->nLights; k++)
        {
          Logv(0, "%s ", lp->pLights[k]->m_sName);
        }
        Logv(0, "\n");
      }
    }
#endif
    
    slw = &pTech->m_Passes[0];
    const int nTechPasses = pTech->m_Passes.Num();
    for (i=0; i<nTechPasses; i++, slw++)
    {
      m_RP.m_pCurPass = slw;

      // Set all textures and HW TexGen modes for the current pass
      //assert (slw->m_VShader && slw->m_PShader);
      if (!slw->m_VShader || !slw->m_PShader)
        continue;
      CHWShader_D3D *curVS = (CHWShader_D3D *)slw->m_VShader;
      CHWShader_D3D *curPS = (CHWShader_D3D *)slw->m_PShader;

      
      FX_CommitStates(pTech, slw, (slw->m_PassFlags & SHPF_NOMATSTATE) == 0);

//#if !defined(XENON) && !defined(PS3)
      if (rRP.m_FlagsPerFlush & RBSI_INSTANCED)
      {
        // Using geometry instancing approach
        FX_DrawShader_InstancedHW(ef, slw);
      }
      else
//#endif
      {
        // Set Pixel shader and all associated textures
        bool bRes = curPS->mfSet(HWSF_SETTEXTURES);
        if (bRes)
          FX_DrawBatches(ef, slw, curVS, curPS);
      }      
    }

		rRP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_DEFERRED_SHADING];

    // Should only be enabled per light pass (else if shader has more than 1 pass in technique will blend incorrect)
    rRP.m_bNotFirstPass = true;
  }
  rRP.m_bNotFirstPass = false;
  rRP.m_FlagsShader_LT = 0;
  rRP.m_nCurLightPass = 0;
  rRP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_MULTILIGHTS;
  rRP.m_FrameObject++;
}

//=================================================================================

static TArray<CRenderObject *> sTempObjects[2];
static TArray<SRendItem *> sTempRIs;

//#if !defined(XENON) && !defined(PS3)

#define INST_PARAM_SIZE sizeof(Vec4)

// Actual drawing of instances
void CD3D9Renderer::FX_DrawInstances(CShader *ef, SShaderPass *slw, uint32 nStartInst, uint32 nLastInst, uint32 nUsedAttr, int nInstAttrMask, byte Attributes[], uint32 nConstBasedInstancing)
{
  uint32 i;

  CHWShader_D3D *vp = (CHWShader_D3D *)slw->m_VShader;

#if defined (DIRECT3D10)
  // Don't render fallback in DX11
  if (!CHWShader_D3D::m_pCurInstVS || !CHWShader_D3D::m_pCurInstPS || CHWShader_D3D::m_pCurInstVS->m_bFallback || CHWShader_D3D::m_pCurInstPS->m_bFallback)
    return;
#endif

  // Set culling mode
  if (!(m_RP.m_FlagsPerFlush & RBSI_NOCULL))
  {
    if (slw->m_eCull != -1)
      D3DSetCull((ECull)slw->m_eCull);
  }
  HRESULT hr;

  if (nConstBasedInstancing)
  {
    nInstAttrMask = 0;
    if (!nStartInst && nConstBasedInstancing == 2)
      FX_CommitStreams(slw, true);
  }
  if (!nStartInst && nConstBasedInstancing != 2)
  {
    // Set the stream 3 to be per instance data and iterate once per instance
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags &= ~(RBPF_USESTREAM<<3);
    int nCompared = 0;
    FX_CommitStreams(slw, false);
    int StreamMask = m_RP.m_FlagsStreams_Decl >> 1;
    SVertexDeclaration *vd = 0;
    for (i=0; i<m_RP.m_CustomVD.Num(); i++)
    {
      vd = m_RP.m_CustomVD[i];
      if (vd->StreamMask == StreamMask && vd->VertFormat == m_RP.m_CurVFormat && vd->InstAttrMask == nInstAttrMask)
        break;
    }
    if (i == m_RP.m_CustomVD.Num())
    {
      vd = new SVertexDeclaration;
      m_RP.m_CustomVD.AddElem(vd);
      vd->StreamMask = StreamMask;
      vd->VertFormat = m_RP.m_CurVFormat;
      vd->InstAttrMask = nInstAttrMask;
      vd->m_pDeclaration = NULL;

			SOnDemandD3DVertexDeclaration Decl;
			EF_OnDemandVertexDeclaration(Decl,StreamMask,m_RP.m_CurVFormat,false);

      int nElementsToCopy = Decl.m_Declaration.Num()-1;
#if defined (DIRECT3D10)
      nElementsToCopy++;
#endif
      for (i=0; i<(uint32)nElementsToCopy; i++)
      {
        vd->m_Declaration.AddElem(Decl.m_Declaration[i]);
      }
      int nInstOffs = 1;
#if defined (DIRECT3D9) || defined (OPENGL)
      D3DVERTEXELEMENT9 ve;
      ve.Stream = 3;
      ve.Method = D3DDECLMETHOD_DEFAULT;
      ve.Usage = D3DDECLUSAGE_TEXCOORD;
      if (nConstBasedInstancing == 1)
      {
        ve.Type = D3DDECLTYPE_FLOAT1;
        ve.UsageIndex = (BYTE)vp->m_pCurInst->m_nInstIndex;
        ve.Offset = 0;
        vd->m_Declaration.AddElem(ve);
      }
      else
      {
        ve.Type = D3DDECLTYPE_FLOAT4;
        for (i=0; i<nUsedAttr; i++)
        {
          ve.Offset = i*INST_PARAM_SIZE;
          ve.UsageIndex = Attributes[i]+nInstOffs;
          vd->m_Declaration.AddElem(ve);
        }








      }
      ve.Stream = 0xff;
      ve.Type = D3DDECLTYPE_UNUSED;
      ve.Usage = 0;
      ve.UsageIndex = 0;
      ve.Offset = 0;
      vd->m_Declaration.AddElem(ve);
#elif defined (DIRECT3D10)
      D3D11_INPUT_ELEMENT_DESC elemTC = {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1};      // texture
      if (nConstBasedInstancing == 1)
      {
        assert(0);
      }
      else
      {
        for (i=0; i<nUsedAttr; i++)
        {
          elemTC.AlignedByteOffset = i*INST_PARAM_SIZE;
          elemTC.SemanticIndex = Attributes[i]+nInstOffs;
          vd->m_Declaration.AddElem(elemTC);
        }
      }
#endif
    }
#if defined (DIRECT3D9) || defined (OPENGL)
		assert(vd);
    if (!vd->m_pDeclaration)
    {
      hr = m_pd3dDevice->CreateVertexDeclaration(&vd->m_Declaration[0], &vd->m_pDeclaration);
      assert (hr == S_OK);
    }
    if (m_pLastVDeclaration != vd->m_pDeclaration)
    {
      m_pLastVDeclaration = vd->m_pDeclaration;
      hr = m_pd3dDevice->SetVertexDeclaration(vd->m_pDeclaration);
      assert (hr == S_OK);
    }
#elif defined (DIRECT3D10)
    if (!vd->m_pDeclaration)
    {
      assert(CHWShader_D3D::m_pCurInstVS && CHWShader_D3D::m_pCurInstVS->m_pShaderData);
      if (FAILED(hr = m_pd3dDevice->CreateInputLayout(&vd->m_Declaration[0], vd->m_Declaration.Num(), CHWShader_D3D::m_pCurInstVS->m_pShaderData, CHWShader_D3D::m_pCurInstVS->m_nShaderByteCodeSize, &vd->m_pDeclaration)))
      {
        assert(SUCCEEDED(hr));
        return;
      }
    }
    if (m_pLastVDeclaration != vd->m_pDeclaration)
    {
      m_pLastVDeclaration = vd->m_pDeclaration;
      m_pd3dDeviceContext->IASetInputLayout(vd->m_pDeclaration);
    }
#endif
  }

  int nInsts = nLastInst-nStartInst+1;
  {
    //PROFILE_FRAME(Draw_ShaderIndexMesh);
    int nPolys = m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP];
    int nSaveInds, nSaveVerts;
    int nPolysPerInst = m_RP.m_RendNumIndices / 3;
    if (nConstBasedInstancing == 2)
    {
      nSaveInds = m_RP.m_RendNumIndices;
      nSaveVerts = m_RP.m_RendNumVerts;

      m_RP.m_RendNumIndices *= nInsts;
      m_RP.m_RendNumVerts *= nInsts;
    }
#if defined (DIRECT3D9) || defined (OPENGL)
 #ifndef XENON
    if (m_RP.m_pRE)
      m_RP.m_pRE->mfDraw(ef, slw);
    else
      FX_DrawIndexedMesh(R_PRIMV_TRIANGLES);

































 #endif
#else
    assert (m_RP.m_pRE && m_RP.m_pRE->mfGetType() == eDATA_Mesh);
    FX_Commit();
    SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pd3dDeviceContext->DrawIndexedInstanced(m_RP.m_RendNumIndices, nInsts, m_RP.m_FirstIndex+m_RP.m_IndexOffset, 0, 0);
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += m_RP.m_RendNumIndices/3;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nDIPs[m_RP.m_nPassGroupDIP]++;
#endif
    if (nConstBasedInstancing == 2)
    {
      m_RP.m_RendNumIndices = nSaveInds;
      m_RP.m_RendNumVerts = nSaveVerts;
    }
    int nPolysAll = nPolysPerInst*nInsts;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] -= m_RP.m_RendNumIndices/3;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygons[m_RP.m_nPassGroupDIP] += nPolysAll;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_RendHWInstancesPolysOne += nPolysPerInst;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_RendHWInstancesPolysAll += nPolysAll;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumRendHWInstances += nInsts;
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_RendHWInstancesDIPs++;
  }
}

#define MAX_HWINST_PARAMS_CONST (240 - VSCONST_INSTDATA)

// Draw geometry instances in single DIP using HW geom. instancing (StreamSourceFreq)
void CD3D9Renderer::FX_DrawShader_InstancedHW(CShader *ef, SShaderPass *slw)
{
  PROFILE_FRAME(DrawShader_Instanced);

  SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
  SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

  // Set culling mode
  if (!(rRP.m_FlagsPerFlush & RBSI_NOCULL))
  {
    if (slw->m_eCull != -1)
      D3DSetCull((ECull)slw->m_eCull);
  }
  
  uint32 i, j, n;
  CHWShader_D3D *vp = (CHWShader_D3D *)slw->m_VShader;
  CHWShader_D3D *ps = (CHWShader_D3D *)slw->m_PShader;
  uint32 nOffs;
  Matrix44 m;
  SCGBind bind;
  byte Attributes[32];
  sTempObjects[0].SetUse(0);
  sTempObjects[1].SetUse(0);

  rRP.m_FlagsPerFlush |= RBSI_INSTANCED;

  CRenderObject *pObj;
  uint32 nO;
  uint32 nInsts[2] = {0,0};
  uint32 nCurInst;
  byte *data = NULL;
#ifdef DO_RENDERLOG
  if (CRenderer::CV_r_log >= 3)
  {
    for (nO=0; nO<rRP.m_RIs.Num(); nO++)
    {
      pObj = rRP.m_RIs[nO]->pObj;
      //DynArray16<SInstanceInfo> *pII = pObj->GetInstanceInfo(m_RP.m_nProcessThreadID);
      bool bRotate = (CV_r_geominstancing == 2 || (pObj->m_ObjFlags & FOB_TRANS_ROTATE));

      if (!bRotate)
      {
        Vec3 vPos = pObj->GetTranslation();
        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "+++ Instance NonRotated %d (Obj: %d [%.3f, %.3f, %.3f]) * %d\n", nInsts[0], pObj->m_Id, vPos[0], vPos[1], vPos[2], /*pII ? pII->size() :*/ 1);
        sTempObjects[0].AddElem(pObj);
        //if (pII)
        //  nInsts[0] += pII->size();
        //else
          nInsts[0]++;
      }
      else
      {
        Vec3 vPos = pObj->GetTranslation();
        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "+++ Instance Rotated %d (Obj: %d [%.3f, %.3f, %.3f]) * %d\n", nInsts[1], pObj->m_Id, vPos[0], vPos[1], vPos[2], /*pII ? pII->size() :*/ 1);
        sTempObjects[1].AddElem(pObj);
        //if (pII)
        //  nInsts[1] += pII->size();
        //else
          nInsts[1]++;
      }
    }
  }
  else
#endif
  {
    int nInstType = CV_r_geominstancing;
    for (nO=0; nO<rRP.m_RIs.Num(); nO++)
    {
      //DynArray16<SInstanceInfo> *pII = pObj->GetInstanceInfo(m_RP.m_nProcessThreadID);
      pObj = rRP.m_RIs[nO]->pObj;
      int nInd = (nInstType == 2 || (pObj->m_ObjFlags & FOB_TRANS_ROTATE));

      nInsts[nInd]++;
      sTempObjects[nInd].AddElem(pObj);
    }
  }

  uint32 nConstBasedInstancing = 0;
  if (CV_r_geominstancing == 3)
    nConstBasedInstancing = 2;
//#ifdef XENON
//  bConstBasedInstancing = true; 
//#endif
  static CCryName nmFXInst = "_InstancingParams";

  if (nConstBasedInstancing)
    rRP.m_FlagsShader_RT |= (g_HWSR_MaskBit[HWSR_INSTANCING_CONST] | g_HWSR_MaskBit[HWSR_INSTANCING_ATTR]);
  else
    rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_INSTANCING_ATTR];
  rRP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_INSTANCING_ROT];

  // Non-rotated instances
  if (nInsts[0])
  {
    int nUsedAttr = 1;

    // Set Pixel shader and all associated textures
    if (!ps->mfSet(HWSF_SETTEXTURES))
      return;

    // Set Vertex shader
    if (!vp->mfSet(HWSF_INSTANCED))
      return;

#if defined(DIRECT3D10)
		CHWShader_D3D *gs = (CHWShader_D3D *)slw->m_GShader;
		if (gs)
			gs->mfSet(0);
		else
			CHWShader_D3D::mfBindGS(0, 0);
#endif

    // Create/Update video mesh (VB management)
    if (!rRP.m_pRE->mfCheckUpdate(rRP.m_CurVFormat, rRP.m_FlagsStreams_Stream))
      return;

    CHWShader_D3D::SHWSInstance *pVPInst = vp->m_pCurInst;
    if (!pVPInst->m_bFallback)
    {
      // Starting from TC1 for instanced parameters
      Attributes[0] = (byte)pVPInst->m_nInstMatrixID;
      int nInstAttrMask = 1 << Attributes[0];

      if (pVPInst->m_nParams_Inst >= 0)
      {
        SCGParamsGroup& Group = CGParamManager::m_Groups[pVPInst->m_nParams_Inst];
        const uint32 nSize = Group.nParams;
        for (j=0; j<nSize; j++)
        {
          SCGParam *pr = &Group.pParams[j];
          for (uint32 na=0; na < (uint32)pr->m_nParameters; na++)
          {
            Attributes[nUsedAttr+na] = pr->m_dwBind+na;
            nInstAttrMask |= 1<<Attributes[nUsedAttr+na];
          }
          nUsedAttr += pr->m_nParameters;
        }
      }
      if (pVPInst->m_nParams[1] >= 0)
      {
        SCGParamsGroup& Group = CGParamManager::m_Groups[pVPInst->m_nParams[1]];
        const uint32 nSize = Group.nParams;
        for (i=1; i<nSize; i++)
        {
          SCGParam *pr = &Group.pParams[i];
          if (pr->m_Flags & PF_INSTANCE)
            iLog->LogWarning("WARNING: Instance depend constant '%s' used in the vertex shader %s during instancing", pr->m_Name.c_str(), vp->GetName());
        }
      }




      if (nConstBasedInstancing)
      {
        Vec4 vInst = Vec4((float)rRP.m_RendNumIndices, 0, 0, (float)nUsedAttr);
        ef->FXSetVSFloat(nmFXInst, &vInst, 1);
      }


      // Detects ability of using attributes based instancing
      // If number of used attributes exceed 16 we can't use attributes based instancing (switch to constant based)
      int nStreamMask = rRP.m_FlagsStreams_Decl >> 1;
      int nVFormat = rRP.m_CurVFormat;
      int nCO = 0;
      int nCI = 0;
      nCurInst = 0;
			uint32 dwDeclarationSize = 0;	// todo: later m_RP.m_D3DVertexDeclaration[nStreamMask][nVFormat].m_Declaration.Num();
      if (!nConstBasedInstancing && dwDeclarationSize+nUsedAttr-1 > 16)
        iLog->LogWarning("WARNING: Attributes based instancing cannot exceed 16 attributes (%s uses %d attr. + %d vertex decl.attr.)[VF: %d, SM: 0x%x]", vp->GetName(), nUsedAttr, dwDeclarationSize-1, nVFormat, nStreamMask);
      else
      while ((int)nCurInst < nInsts[0])
      {
        uint32 nLastInst = nInsts[0] - 1;
        if (nConstBasedInstancing == 2)
        {
          if (nLastInst-nCurInst+1 >= RM_INSTANCES)
            nLastInst = nCurInst+RM_INSTANCES-1;
          //if (nLastInst-nCurInst+1 >= 1)
          //  nLastInst = nCurInst+1-1;
        }
        else
        {
          uint32 nParamsPerDIPAllowed = nConstBasedInstancing==1 ? MAX_HWINST_PARAMS_CONST : MAX_HWINST_PARAMS;
          if ((nLastInst-nCurInst+1)*nUsedAttr >= nParamsPerDIPAllowed)
            nLastInst = nCurInst+(nParamsPerDIPAllowed/nUsedAttr)-1;
        }
        byte *inddata = NULL;
        if (nConstBasedInstancing)
        {
  #if defined (DIRECT3D9) || defined (OPENGL)
          data = (byte *)&CHWShader_D3D::m_CurVSParams[VSCONST_INSTDATA].x;
          if (nConstBasedInstancing == 1)
            inddata = (byte *)rRP.m_VB_Inst->Lock((nLastInst-nCurInst+1)*sizeof(float), nOffs);
  #else
          assert(0);
  #endif
        }
        else
          data = (byte *)rRP.m_VB_Inst->Lock((nLastInst-nCurInst+1)*nUsedAttr*INST_PARAM_SIZE, nOffs);
        CRenderObject *curObj = rRP.m_pCurObject;
        n = 0;
        // Fill the stream 3 for per-instance data
        CRenderObject *pRO;
        SInstanceInfo *pI;
        for (i=nCurInst; i<=nLastInst; i++, n++)
        {
          float *fParm = (float *)&data[n*nUsedAttr*INST_PARAM_SIZE];
          {
            pRO = sTempObjects[0][nCO++];






            rRP.m_pCurObject = pRO;
            rRP.m_FrameObject++;
            pI = &pRO->m_II;
          }
          rRP.m_pCurInstanceInfo = pI;
          float *fSrc = pI->m_Matrix.GetData();
          fParm[0] = fSrc[3]; fParm[1] = fSrc[7]; fParm[2] = fSrc[11]; fParm[3] = fSrc[0];
          fParm += 4;

          if (pVPInst->m_nParams_Inst >= 0)
          {
            SCGParamsGroup& Group = CGParamManager::m_Groups[pVPInst->m_nParams_Inst];
  #if defined (DIRECT3D9) || defined (OPENGL)
            fParm = vp->mfSetParametersPI(Group.pParams, Group.nParams, fParm, eHWSC_Vertex);
  #else
            fParm = vp->mfSetParametersPI(Group.pParams, Group.nParams, fParm, eHWSC_Vertex, 40);
  #endif
          }
          if (nConstBasedInstancing == 1)
          {
            float *fInd = (float *)&inddata[n*sizeof(float)];
            *fInd = (float)n*nUsedAttr;
          }
        }
        rRP.m_pCurObject = curObj;
        rRP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
        rRP.m_VB_Inst->Unlock();
        if (nConstBasedInstancing)
        {
  //#if defined(OPENGL)
  //#if defined(PS3_ACTIVATE_CONSTANT_ARRAYS)
  //        CHWShader_D3D::sGlobalConsts[VSCONST_INSTDATA_OPENGL].SetConstantIntoShader
  //          (gcpRendD3D->GetD3DDevice(), &CHWShader_D3D::m_CurVSParams[VSCONST_INSTDATA].x, n*nUsedAttr);
  //#endif
  //#else
  #if defined (DIRECT3D9) || defined (OPENGL)
          vp->mfParameterReg_NoCheck(VSCONST_INSTDATA, &CHWShader_D3D::m_CurVSParams[VSCONST_INSTDATA].x, n*nUsedAttr, eHWSC_Vertex);
  #endif
  //#endif
        }

        // Set the first stream to be the indexed data and render N instances
        if (nConstBasedInstancing != 2)
        {
  #ifndef XENON
          rRP.m_ReqStreamFrequence[0] = n | D3DSTREAMSOURCE_INDEXEDDATA;
          if (nConstBasedInstancing == 1)
            rRP.m_VB_Inst->Bind(3, nOffs, sizeof(float), 1 | D3DSTREAMSOURCE_INSTANCEDATA);
          else
            rRP.m_VB_Inst->Bind(3, nOffs, nUsedAttr*INST_PARAM_SIZE, 1 | D3DSTREAMSOURCE_INSTANCEDATA);


  #endif
        }

        FX_DrawInstances(ef, slw, nCurInst, nLastInst, nUsedAttr, nInstAttrMask, Attributes, nConstBasedInstancing);
        nCurInst = nLastInst+1;
      }
    }
  }

  // Rotated instances
  if (nInsts[1])
  {
    rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_INSTANCING_ROT];

    // Set Pixel shader and all associated textures
    if (!ps->mfSet(HWSF_SETTEXTURES))
      return;

    // Set Vertex shader
    if (!vp->mfSet(HWSF_INSTANCED))
      return;

#if defined(DIRECT3D10)
		CHWShader_D3D *gs = (CHWShader_D3D *)slw->m_GShader;
		if (gs)
			gs->mfSet(0);
		else
			CHWShader_D3D::mfBindGS(0, 0);
#endif

    // Create/Update video mesh (VB management)
    if (!rRP.m_pRE->mfCheckUpdate(rRP.m_CurVFormat, rRP.m_FlagsStreams_Stream))
      return;

    CHWShader_D3D::SHWSInstance *pVPInst = vp->m_pCurInst;
    if (!pVPInst->m_bFallback)
    {
      int nUsedAttr = 3;
      Attributes[0] = (byte)pVPInst->m_nInstMatrixID;
      Attributes[1] = Attributes[0]+1;
      Attributes[2] = Attributes[0]+2;
      int nInstAttrMask = 0x7 << pVPInst->m_nInstMatrixID;
      if (pVPInst->m_nParams_Inst >= 0)
      {
        SCGParamsGroup& Group = CGParamManager::m_Groups[pVPInst->m_nParams_Inst];
        uint32 nSize = Group.nParams;
        for (j=0; j<nSize; j++)
        {
          SCGParam *pr = &Group.pParams[j];
          for (uint32 na=0; na<(uint32)pr->m_nParameters; na++)
          {
            Attributes[nUsedAttr+na] = pr->m_dwBind+na;
            nInstAttrMask |= 1<<Attributes[nUsedAttr+na];
          }
          nUsedAttr += pr->m_nParameters;
        }
      }
      nCurInst = 0;





      if (nConstBasedInstancing)
      {
        Vec4 vInst = Vec4((float)rRP.m_RendNumIndices, 0, 0, (float)nUsedAttr);
        ef->FXSetVSFloat(nmFXInst, &vInst, 1);
      }


      // Detects possibility of using attributes based instancing
      // If number of used attributes exceed 16 we can't use attributes based instancing (switch to constant based)
      int nStreamMask = rRP.m_FlagsStreams_Stream >> 1;
      int nVFormat = rRP.m_CurVFormat;
      int nCO = 0;
      int nCI = 0;
			uint32 dwDeclarationSize = 0;	// todo: later m_RP.m_D3DVertexDeclaration[nStreamMask][nVFormat].m_Declaration.Num();
      if (!nConstBasedInstancing && dwDeclarationSize+nUsedAttr-1 > 16)
        iLog->LogWarning("WARNING: Attributes based instancing cannot exceed 16 attributes (%s uses %d attr. + %d vertex decl.attr.)[VF: %d, SM: 0x%x]", vp->GetName(), nUsedAttr, dwDeclarationSize-1, nVFormat, nStreamMask);
      else
      while ((int)nCurInst < nInsts[1])
      {
        uint32 nLastInst = nInsts[1] - 1;
        if (nConstBasedInstancing == 2)
        {
          if (nLastInst-nCurInst+1 >= RM_INSTANCES)
            nLastInst = nCurInst+RM_INSTANCES-1;
        }
        else
        {
          uint32 nParamsPerInstAllowed = nConstBasedInstancing ? MAX_HWINST_PARAMS_CONST : MAX_HWINST_PARAMS;
          if ((nLastInst-nCurInst+1)*nUsedAttr >= nParamsPerInstAllowed)
            nLastInst = nCurInst+(nParamsPerInstAllowed/nUsedAttr)-1;
        }
        byte *inddata = NULL;
        if (nConstBasedInstancing)
        {
  #if defined (DIRECT3D9) || defined (OPENGL)
          data = (byte *)&CHWShader_D3D::m_CurVSParams[VSCONST_INSTDATA].x;
          if (CV_r_geominstancing != 3)
            inddata = (byte *)rRP.m_VB_Inst->Lock((nLastInst-nCurInst+1)*nUsedAttr*sizeof(float), nOffs);
  #else
          assert(0);
  #endif
        }
        else
          data = (byte *)rRP.m_VB_Inst->Lock((nLastInst-nCurInst+1)*nUsedAttr*INST_PARAM_SIZE, nOffs);
        CRenderObject *curObj = rRP.m_pCurObject;
        n = 0;

        // Fill the stream 3 for per-instance data
        CRenderObject *pRO;
        SInstanceInfo *pI;
        for (i=nCurInst; i<=nLastInst; i++, n++)
        {
          float *fParm = (float *)&data[n*nUsedAttr*INST_PARAM_SIZE];
          float *fSrc;
          {
            pRO = sTempObjects[1][nCO++];






            rRP.m_pCurObject = pRO;
            rRP.m_FrameObject++;
            pI = &pRO->m_II;
          }
          rRP.m_pCurInstanceInfo = pI;
          fSrc = pI->m_Matrix.GetData();

          fParm[0] = fSrc[0];  fParm[1] = fSrc[1];  fParm[2] = fSrc[2];  fParm[3] = fSrc[3]; 
          fParm[4] = fSrc[4];  fParm[5] = fSrc[5];  fParm[6] = fSrc[6];  fParm[7] = fSrc[7]; 
          fParm[8] = fSrc[8];  fParm[9] = fSrc[9];  fParm[10] = fSrc[10];  fParm[11] = fSrc[11]; 
          fParm += 3*4;

          if (pVPInst->m_nParams_Inst >= 0)
          {
            SCGParamsGroup& Group = CGParamManager::m_Groups[pVPInst->m_nParams_Inst];
  #if defined (DIRECT3D9) || defined (OPENGL)
            fParm = vp->mfSetParametersPI(Group.pParams, Group.nParams, fParm, eHWSC_Vertex);
  #else
            fParm = vp->mfSetParametersPI(Group.pParams, Group.nParams, fParm, eHWSC_Vertex, 40);
  #endif
          }
          if (nConstBasedInstancing == 1)
          {
            float *fInd = (float *)&inddata[n*sizeof(float)];
            *fInd = (float)n*nUsedAttr;
          }
        }
        rRP.m_pCurObject = curObj;
        rRP.m_VB_Inst->Unlock();

        if (nConstBasedInstancing)
        {
  //#if defined(OPENGL)
  //#if defined(PS3_ACTIVATE_CONSTANT_ARRAYS)
  //        CHWShader_D3D::sGlobalConsts[VSCONST_INSTDATA_OPENGL].SetConstantIntoShader
  //          (gcpRendD3D->GetD3DDevice(), &CHWShader_D3D::m_CurVSParams[VSCONST_INSTDATA].x, n*nUsedAttr);
  //#endif
  //#else
  #if defined (DIRECT3D9) || defined (OPENGL)
          vp->mfParameterReg_NoCheck(VSCONST_INSTDATA, &CHWShader_D3D::m_CurVSParams[VSCONST_INSTDATA].x, n*nUsedAttr, eHWSC_Vertex);
  #endif
  //#endif
        }

        // Set the first stream to be the indexed data and render N instances
        if (nConstBasedInstancing != 2)
        {
#ifndef XENON
          rRP.m_ReqStreamFrequence[0] = n | D3DSTREAMSOURCE_INDEXEDDATA;
          if (nConstBasedInstancing == 1)
            rRP.m_VB_Inst->Bind(3, nOffs, sizeof(float), 1 | D3DSTREAMSOURCE_INSTANCEDATA);
          else
            rRP.m_VB_Inst->Bind(3, nOffs, nUsedAttr*INST_PARAM_SIZE, 1 | D3DSTREAMSOURCE_INSTANCEDATA);


#endif
        }

        FX_DrawInstances(ef, slw, nCurInst, nLastInst, nUsedAttr, nInstAttrMask, Attributes, nConstBasedInstancing);
        nCurInst = nLastInst+1;
      }
    }
  }

  rTI.m_PersFlags |= RBPF_USESTREAM<<3;
  rRP.m_ReqStreamFrequence[0] = 1;
  rRP.m_ReqStreamFrequence[3] = 1;
}
//#endif

//====================================================================================

void CD3D9Renderer::FX_FlushSkinVSParams(CHWShader_D3D *pVS, int nFirst, int nBones, int nOffsVS, int numBonesPerChunk, int nSlot, QuatTS *pSkinQuats, QuatTS *pMBSkinQuats)
{
#if defined (DIRECT3D9) || defined (OPENGL)
  pVS->mfSetVSConst(VSCONST_SKINMATRIX + (nOffsVS<<1), &pSkinQuats[nFirst].q.v.x, nBones<<1);
  if ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_MOTIONBLURPASS) && (numBonesPerChunk < NUM_MAX_BONES_PER_GROUP_WITH_MB) )
  { 
    // if in motion blur pass, and bones count is less than NUM_MAX_BONES_PER_GROUP_WITH_MB, allow previous frame skinning
    pVS->mfSetVSConst(VSCONST_SKINMATRIX + ((nOffsVS + NUM_MAX_BONES_PER_GROUP_WITH_MB)<<1), &pMBSkinQuats[nFirst].q.v.x, nBones<<1);
  }                      
#elif defined (DIRECT3D10)
  if (nSlot>=0)
  {
    int nVecs = VSCONST_SKINMATRIX+numBonesPerChunk * 2;
    if ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_MOTIONBLURPASS) && (numBonesPerChunk < NUM_MAX_BONES_PER_GROUP_WITH_MB) )
      nVecs += NUM_MAX_BONES_PER_GROUP_WITH_MB * 2;
    pVS->mfParameterReg(VSCONST_SKINMATRIX + (nOffsVS<<1), nSlot, eHWSC_Vertex, &pSkinQuats[nFirst].q.v.x, (nBones<<1), nVecs);
    if ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_MOTIONBLURPASS) && (numBonesPerChunk < NUM_MAX_BONES_PER_GROUP_WITH_MB) )
    { 
      // if in motion blur pass, and bones count is less than NUM_MAX_BONES_PER_GROUP_WITH_MB, allow previous frame skinning
      pVS->mfParameterReg(VSCONST_SKINMATRIX + ((nOffsVS + NUM_MAX_BONES_PER_GROUP_WITH_MB)<<1), nSlot, eHWSC_Vertex, &pMBSkinQuats[nFirst].q.v.x, (nBones<<1), nVecs);
    }                      
  }
#endif
}

//for PS3 it is inlined as it is very simple and requires some loads when it is not inlined
#ifndef PS3
byte CD3D9Renderer::FX_StartQuery(SRendItem *pRI)
{
#if defined(DIRECT3D9)
  if (CV_r_ConditionalRendering)
	{
		if (m_RP.m_nBatchFilter & FB_Z)
		{
			if (m_OcclQueries.Num() >= MAX_OCCL_QUERIES)
				return 0;

			assert(pRI->nOcclQuery > MAX_OCCL_QUERIES);
			uint32 nQuery = m_OcclQueries.Num();
			m_OcclQueries.AddIndex(1);
			COcclusionQuery *pQ = &m_OcclQueries[nQuery];
			pQ->BeginQuery();
			pRI->nOcclQuery = nQuery;
			m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumQIssued++;
			return 1;
		}
		else
		{
			if (pRI->nOcclQuery > MAX_OCCL_QUERIES)
				return 0;

			COcclusionQuery *pQ = &m_OcclQueries[pRI->nOcclQuery];
			uint32 nPixels = pQ->GetVisibleSamples(CV_r_ConditionalRendering==2 ? false : true);
			bool bReady = pQ->IsReady();
			if (!bReady)
			{
				m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumQNotReady++;
				return 0;
			}
			if (nPixels == 0)
			{
				m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumQOccluded++;
				return 2;
			}
			return 0;
		}
	}
#endif
  return 0;
}
#endif//PS3

void CD3D9Renderer::FX_EndQuery(SRendItem *pRI, byte bStartQ)
{






#if defined(DIRECT3D9)
  if (!bStartQ)
    return;

  COcclusionQuery *pQ = &m_OcclQueries[pRI->nOcclQuery];
  pQ->EndQuery();
#endif
}

void CD3D9Renderer::FX_DrawBatchSkinned(CShader *pSh, SShaderPass *pPass, CHWShader_D3D *curVS, CHWShader_D3D *curPS)
{
  PROFILE_FRAME(DrawShader_BatchSkinned);

  SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
  SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

  int nThreadID = m_RP.m_nProcessThreadID;
  m_RP.m_PS[nThreadID].m_NumRendSkinnedObjects++;

  QuatTS *pGlobalSkinQuatsL = NULL;
  QuatTS *pGlobalSkinQuatsS = NULL;
  QuatTS *pMBGlobalSkinQuatsL = NULL;
  QuatTS *pMBGlobalSkinQuatsS = NULL;
  Vec4 HWShapeDeformationData[8];
  CRenderObject *pObj = m_RP.m_pCurObject;
  uint32 HWSkinningFlags = 0;
  CREMesh *pRE = (CREMesh *)m_RP.m_pRE;





  uint32 nBones = 0;
  uint32 j;

  uint8 * pRemapTable = 0;
  /*if (pObj->m_ObjFlags & FOB_VEGETATION)
  {
    int nnn = 0;
  }*/

  //if (pObj->m_pCharInstance && !CV_r_character_nodeform)
  {
    PROFILE_FRAME(Objects_GetSkinningData);
    int nFlags = 0;
    {
      HWSkinningFlags |= (m_RP.m_TI[nThreadID].m_PersFlags2 & RBPF2_MOTIONBLURPASS)? eHWS_MotionBlured: 0;
      HWSkinningFlags |= eHWS_MorphTarget;
      SRenderObjData *pOD = pObj->GetObjData(nThreadID);
      assert(pOD);
      if (pOD)
        nBones = pOD->m_pCharInstance->GetSkeletonPose(pObj->m_nLod, nThreadID, pObj->m_II.m_Matrix, pGlobalSkinQuatsL, pGlobalSkinQuatsS, pMBGlobalSkinQuatsL, pMBGlobalSkinQuatsS, HWShapeDeformationData, HWSkinningFlags, pRemapTable);
    }
  }
  CRenderChunk *pChunk = pRE->m_pChunk;



  if (nBones < pChunk->m_arrChunkBoneIDs.size())
  {
    Warning ("Warning: Skinned geometry number of bones mismatch (Mesh: %d, Character instance: %d)", pChunk->m_arrChunkBoneIDs.size(), nBones);
  }
  else
  {
    if (HWSkinningFlags & eHWS_MorphTarget)
    {
      m_RP.m_FlagsStreams_Decl |= VSM_HWSKIN_MORPHTARGET;
      m_RP.m_FlagsStreams_Stream |= VSM_HWSKIN_MORPHTARGET;
    }
    if (HWSkinningFlags & eHWS_ShapeDeform)
    {
      m_RP.m_FlagsStreams_Decl |= VSM_HWSKIN_SHAPEDEFORM;
      m_RP.m_FlagsStreams_Stream |= VSM_HWSKIN_SHAPEDEFORM;
    }

    m_RP.m_nNumRendPasses++;

    m_RP.m_RendNumGroup = 0;
    m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SPHERICAL] | g_HWSR_MaskBit[HWSR_SKELETON_SSD]);

    //always use 4 bone per vertex
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SKELETON_SSD];

    //choose linear- or spherical-skinning
    static ICVar* pCVar_SphericalSkinning = gEnv->pConsole->GetCVar( "ca_SphericalSkinning" );		assert(pCVar_SphericalSkinning);
    static ICVar* pCVar_VegetationSphericalSkinning = gEnv->pConsole->GetCVar( "e_VegetationSphericalSkinning" );		assert(pCVar_VegetationSphericalSkinning);

    int iSphericalSkinning = pCVar_SphericalSkinning->GetIVal();
    int iVegSphericalSkinning = (pCVar_VegetationSphericalSkinning->GetIVal() && (m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_VEGETATION]));

    if (iSphericalSkinning || iVegSphericalSkinning)
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SPHERICAL];
    m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_MORPHTARGET] | g_HWSR_MaskBit[HWSR_SHAPEDEFORM]);
    if (HWSkinningFlags & eHWS_MorphTarget)
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_MORPHTARGET];

    if (HWSkinningFlags & eHWS_ShapeDeform)
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SHAPEDEFORM];

#if !defined (DIRECT3D10)
    // Disable perlin-noise based deformations on skinned meshes
    int nMDF = m_RP.m_FlagsShader_MDV & 0xf;
    if (nMDF == eDT_Perlin3D || nMDF == eDT_Perlin2D)
      m_RP.m_FlagsShader_MDV &= ~0xf;
#endif

    if (pRemapTable) 
    {
      // Set Vertex shader
      bool bRes = curVS->mfSetVS(0);

      if (bRes)
      {
#if defined (DIRECT3D10)
        if (CHWShader_D3D::m_pCurInstVS && CHWShader_D3D::m_pCurInstVS->m_bFallback)
          return;
#endif
        // Create/Update video mesh (VB management)
        if (!pRE->mfCheckUpdate(m_RP.m_CurVFormat, m_RP.m_FlagsStreams_Stream))
          return;

        uint32 numBonesPerChunk=pChunk->m_arrChunkBoneIDs.size();
        assert( numBonesPerChunk <= NUM_MAX_BONES_PER_GROUP );

        if ((iSphericalSkinning || iVegSphericalSkinning) && pGlobalSkinQuatsS) 
        {
          int nSlot = -1;
#if defined (DIRECT3D10)
					static const CCryName Name_g_SkinQuat("_g_SkinQuat");
          SCGBind *pBind = curVS->mfGetParameterBind(Name_g_SkinQuat);
          if (pBind)
            nSlot = pBind->m_dwCBufSlot;
#endif
          int nFirst = pRemapTable[pChunk->m_arrChunkBoneIDs[0]];
          int nLast = nFirst;
          int nOffs = 0;
          for (j=1; j<numBonesPerChunk; j++)
          {
            uint32 BoneID=pRemapTable[pChunk->m_arrChunkBoneIDs[j]];
            if (BoneID != nLast+1)
            {
              FX_FlushSkinVSParams(curVS, nFirst, j-nOffs, nOffs, numBonesPerChunk, nSlot, pGlobalSkinQuatsS, pMBGlobalSkinQuatsS);
              nFirst = BoneID;
              nOffs = j;
            }
            nLast = BoneID;
          }
          FX_FlushSkinVSParams(curVS, nFirst, j-nOffs, nOffs, numBonesPerChunk, nSlot, pGlobalSkinQuatsS, pMBGlobalSkinQuatsS);
        }
        else
        {
          int nSlot = -1;
#if defined (DIRECT3D10)
					static const CCryName Name_g_SkinQuat("_g_SkinQuat");
          SCGBind *pBind = curVS->mfGetParameterBind(Name_g_SkinQuat);
          assert(pBind);
          if (pBind)
            nSlot = pBind->m_dwCBufSlot;
#endif
          int nFirst = pRemapTable[pChunk->m_arrChunkBoneIDs[0]];
          int nLast = nFirst;
          int nOffs = 0;
          for (j=1; j<numBonesPerChunk; j++)
          {
            uint32 BoneID=pRemapTable[pChunk->m_arrChunkBoneIDs[j]];
            if (BoneID != nLast+1)
            {
              FX_FlushSkinVSParams(curVS, nFirst, j-nOffs, nOffs, numBonesPerChunk, nSlot, pGlobalSkinQuatsL, pMBGlobalSkinQuatsS);
              nFirst = BoneID;
              nOffs = j;
            }
            nLast = BoneID;
          }
          FX_FlushSkinVSParams(curVS, nFirst, j-nOffs, nOffs, numBonesPerChunk, nSlot, pGlobalSkinQuatsL, pMBGlobalSkinQuatsS);
        }

        if (HWSkinningFlags & eHWS_ShapeDeform)
        {
#if defined (DIRECT3D9) || defined (OPENGL)
          curVS->mfSetVSConst(VSCONST_SHAPEDEFORMATION, &HWShapeDeformationData[0].x, 8);
#elif defined (DIRECT3D10)
					static const CCryName Name_g_ShapeDeformationData("_g_ShapeDeformationData");
          SCGBind *pBind = curVS->mfGetParameterBind(Name_g_ShapeDeformationData);
	#ifndef PS3
          assert(pBind);
	#endif
          if (pBind)
            curVS->mfParameterReg(VSCONST_SHAPEDEFORMATION, pBind->m_dwCBufSlot, eHWSC_Vertex, &HWShapeDeformationData[0].x, 8, VSCONST_SHAPEDEFORMATION+8);
#endif
        }
        int bFogOverrided = EF_FogCorrection();

        // Unlock all VB (if needed) and set current streams
        if (FX_CommitStreams(pPass))
        {
          uint32 nObj = 0;
          CRenderObject *pSaveObj = m_RP.m_pCurObject;
          CRenderObject *pObject;
          for (nObj=0; nObj<m_RP.m_RIs.Num(); nObj++)
          {
            pObject = m_RP.m_RIs[nObj]->pObj;
            m_RP.m_pCurObject = pObject;
            m_RP.m_pCurInstanceInfo = &pObject->m_II;
            m_RP.m_FrameObject++;

#ifdef DO_RENDERSTATS
            if( (CV_r_stats==6 || m_pDebugRenderNode) && !(rTI.m_PersFlags & RBPF_MAKESPRITE) ) 
              FX_TrackStats(pObject);
#endif
            if ((rRP.m_nBatchFilter & FB_Z) && !(rTI.m_PersFlags & RBPF_SHADOWGEN))
              FX_SetRenderObjVisAreaStencilRef( pObject );

#ifdef DO_RENDERLOG
            if (CRenderer::CV_r_log >= 3)
            {
              Vec3 vPos = pObject->GetTranslation();
              Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "+++ HWSkin Group Pass %d (Obj: %d [%.3f, %.3f, %.3f])\n", m_RP.m_nNumRendPasses, pObject->m_Id, vPos[0], vPos[1], vPos[2]);
            }
#endif




            curVS->mfSetParametersPI(pObject, pSh);
            curPS->mfSetParametersPI(NULL, NULL);

#if defined(DIRECT3D10) && !defined(PS3)
            CHWShader_D3D *curGS = (CHWShader_D3D *)pPass->m_GShader;
            if (curGS)
              curGS->mfSetParametersPI(NULL, NULL);
            else
              CHWShader_D3D::mfBindGS(NULL, NULL);
#endif

            {
              //PROFILE_FRAME(Draw_ShaderIndexMesh);
              if (m_RP.m_pRE)
                m_RP.m_pRE->mfDraw(pSh, pPass);
              else
                FX_DrawIndexedMesh(R_PRIMV_TRIANGLES);
            }
          }
          EF_FogRestore(bFogOverrided);
          m_RP.m_FlagsShader_MD &= ~(HWMD_TCM | HWMD_TCG);
          if (pSaveObj != m_RP.m_pCurObject)
          {
            m_RP.m_pCurObject = pSaveObj;
            m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
            m_RP.m_FrameObject++;
          }
        }
      }

    }
    else
    {
      // Set Vertex shader
      bool bRes = curVS->mfSetVS(0);

      if (bRes)
      {
#if defined (DIRECT3D10)
        if (CHWShader_D3D::m_pCurInstVS && CHWShader_D3D::m_pCurInstVS->m_bFallback)
          return;
#endif
        // Create/Update video mesh (VB management)
        if (!pRE->mfCheckUpdate(m_RP.m_CurVFormat, m_RP.m_FlagsStreams_Stream))
          return;

        uint32 numBonesPerChunk=pChunk->m_arrChunkBoneIDs.size();
        assert( numBonesPerChunk <= NUM_MAX_BONES_PER_GROUP );
        numBonesPerChunk = min( numBonesPerChunk, (uint32)NUM_MAX_BONES_PER_GROUP );

        if ((iSphericalSkinning || iVegSphericalSkinning) && pGlobalSkinQuatsS) 
        {
          int nSlot = -1;
#if defined (DIRECT3D10)
					static const CCryName Name_g_SkinQuat("_g_SkinQuat");
          SCGBind *pBind = curVS->mfGetParameterBind(Name_g_SkinQuat);
					assert(pBind);
          if (pBind)
            nSlot = pBind->m_dwCBufSlot;
#endif
          int nFirst = pChunk->m_arrChunkBoneIDs[0];
          int nLast = nFirst;
          int nOffs = 0;
          for (j=1; j<numBonesPerChunk; j++)
          {
            uint32 BoneID=pChunk->m_arrChunkBoneIDs[j];
            if (BoneID != nLast+1)
            {
              FX_FlushSkinVSParams(curVS, nFirst, j-nOffs, nOffs, numBonesPerChunk, nSlot, pGlobalSkinQuatsS, pMBGlobalSkinQuatsS);
              nFirst = BoneID;
              nOffs = j;
            }
            nLast = BoneID;
          }
          FX_FlushSkinVSParams(curVS, nFirst, j-nOffs, nOffs, numBonesPerChunk, nSlot, pGlobalSkinQuatsS, pMBGlobalSkinQuatsS);
        }
        else
        {
          int nSlot = -1;
#if defined (DIRECT3D10)
					static const CCryName Name_g_SkinQuat("_g_SkinQuat");
          SCGBind *pBind = curVS->mfGetParameterBind(Name_g_SkinQuat);
          assert(pBind);
          if (pBind)
            nSlot = pBind->m_dwCBufSlot;
#endif
          int nFirst = pChunk->m_arrChunkBoneIDs[0];
          int nLast = nFirst;
          int nOffs = 0;
          for (j=1; j<numBonesPerChunk; j++)
          {
            uint32 BoneID=pChunk->m_arrChunkBoneIDs[j];
            assert(BoneID < 0x100);
            if (BoneID != nLast+1)
            {
              FX_FlushSkinVSParams(curVS, nFirst, j-nOffs, nOffs, numBonesPerChunk, nSlot, pGlobalSkinQuatsL, pMBGlobalSkinQuatsL);
              nFirst = BoneID;
              nOffs = j;
            }
            nLast = BoneID;
          }
          FX_FlushSkinVSParams(curVS, nFirst, j-nOffs, nOffs, numBonesPerChunk, nSlot, pGlobalSkinQuatsL, pMBGlobalSkinQuatsL);
        }

        if (HWSkinningFlags & eHWS_ShapeDeform)
        {
#if defined (DIRECT3D9) || defined (OPENGL)
          curVS->mfParameterReg(VSCONST_SHAPEDEFORMATION, &HWShapeDeformationData[0].x, 8, eHWSC_Vertex);
#elif defined (DIRECT3D10)
					static const CCryName Name__g_ShapeDeformationData("_g_ShapeDeformationData");
          SCGBind *pBind = curVS->mfGetParameterBind(Name__g_ShapeDeformationData);
	#ifndef PS3
          assert(pBind);
	#endif
          if (pBind)
            curVS->mfParameterReg(VSCONST_SHAPEDEFORMATION, pBind->m_dwCBufSlot, eHWSC_Vertex, &HWShapeDeformationData[0].x, 8, VSCONST_SHAPEDEFORMATION+8);
#endif
        }
        int bFogOverrided = EF_FogCorrection();

        // Unlock all VB (if needed) and set current streams
        if (FX_CommitStreams(pPass))
        {
          uint32 nObj;
          CRenderObject *pSaveObj = m_RP.m_pCurObject;
          CRenderObject *pObject = pSaveObj;
          for (nObj=0; nObj<m_RP.m_RIs.Num(); nObj++)
          {       
            pObject = m_RP.m_RIs[nObj]->pObj;
            m_RP.m_pCurObject = pObject;
            m_RP.m_pCurInstanceInfo = &pObject->m_II;
            m_RP.m_FrameObject++;

#ifdef DO_RENDERLOG
            if (CRenderer::CV_r_log >= 3)
            {
              Vec3 vPos = pObject->GetTranslation();
              Logv(SRendItem::m_RecurseLevel[nThreadID], "+++ HWSkin Group Pass %d (Obj: %d [%.3f, %.3f, %.3f])\n", m_RP.m_nNumRendPasses, pObject->m_Id, vPos[0], vPos[1], vPos[2]);
            }
#endif




            curVS->mfSetParametersPI(pObject, pSh);
            curPS->mfSetParametersPI(NULL, NULL);

#if defined(DIRECT3D10) && !defined(PS3)
            CHWShader_D3D *curGS = (CHWShader_D3D *)pPass->m_GShader;
            if (curGS)
              curGS->mfSetParametersPI(NULL, NULL);
            else
              CHWShader_D3D::mfBindGS(NULL, NULL);
#endif

            {
              //PROFILE_FRAME(Draw_ShaderIndexMesh);
              if (m_RP.m_pRE)
                m_RP.m_pRE->mfDraw(pSh, pPass);
              else
                FX_DrawIndexedMesh(R_PRIMV_TRIANGLES);
            }
          }
          EF_FogRestore(bFogOverrided);
          m_RP.m_FlagsShader_MD &= ~(HWMD_TCM | HWMD_TCG);
          if (pSaveObj != m_RP.m_pCurObject)
          {
            m_RP.m_pCurObject = pSaveObj;
            m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
            m_RP.m_FrameObject++;
          }
        }
      }
    }
  }
  m_RP.m_RendNumGroup = -1;
}

void CD3D9Renderer::FX_TrackStats( CRenderObject *pObj )
{
  SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;

  if( pObj )
  {
    if((IRenderNode*)pObj->m_pRenderNode  )
    {
      SRenderPipeline::RNDrawcallsMapItor pItor = rRP.m_pRNDrawCallsInfo.find( ((IRenderNode*)pObj->m_pRenderNode) );
      if( pItor != rRP.m_pRNDrawCallsInfo.end() )
      {
        SDrawCallCountInfo &pInfoDP = pItor->second;
        pInfoDP.Update( pObj );
      }
      else
      {
        SDrawCallCountInfo pInfoDP;
        pInfoDP.Update( pObj );
        rRP.m_pRNDrawCallsInfo.insert( SRenderPipeline::RNDrawcallsMapItor::value_type( ((IRenderNode*)pObj->m_pRenderNode), pInfoDP ) );
      }
    }
  }
}

void CD3D9Renderer::FX_DrawBatches(CShader *pSh, SShaderPass *pPass, CHWShader_D3D *curVS, CHWShader_D3D *curPS)
{
  SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
  SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];
  CRendElementBase *const __restrict pRE = rRP.m_pRE;
  
#if defined (DIRECT3D10) && !defined(PS3)
  CHWShader_D3D *curGS = (CHWShader_D3D *)pPass->m_GShader;
#endif

  // Set culling mode
  if (!(rRP.m_FlagsPerFlush & RBSI_NOCULL))
  {
    if (pPass->m_eCull != -1)
      D3DSetCull((ECull)pPass->m_eCull);
  }
  bool bHWSkinning = FX_SetStreamFlags(pPass);
  if (bHWSkinning && (rRP.m_pCurObject->m_ObjFlags & FOB_CHARACTER) && !CV_r_character_nodeform)
    FX_DrawBatchSkinned(pSh, pPass, curVS, curPS);
  else
  {
    PROFILE_FRAME(DrawShader_BatchStatic);








    if (bHWSkinning)
    {
      SRenderObjData *pOD = rRP.m_pCurObject->GetObjData(rRP.m_nProcessThreadID);
      //if (!pOD || !pOD->m_pCharInstance) //Diesel cut
      //  Warning ("Warning: Skinned geometry used without character instance"); //Diesel cut
    }


    // Set Vertex shader
    bool bRes = curVS->mfSetVS(0);

#if defined (DIRECT3D10) && !defined(PS3)
    if (curGS)
      bRes = curGS->mfSetGS(0);
    else
      CHWShader_D3D::mfBindGS(NULL, NULL);
#endif

    if (bRes)
    {
      // Create/Update video mesh (VB management)
      if (pRE)
      {
        if (!pRE->mfCheckUpdate(rRP.m_CurVFormat, rRP.m_FlagsStreams_Stream))
          return;
      }

      rRP.m_nNumRendPasses++;

      int bFogOverrided = EF_FogCorrection();

      // Unlock all VBs (if needed) and bind current streams
      if (FX_CommitStreams(pPass))
      {
        uint32 nO;
        const uint32 nNumRI = m_RP.m_RIs.Num();
        CRenderObject *pSaveObj = rRP.m_pCurObject;
        CRenderObject *pObj = NULL;
        SInstanceInfo *pI;

#ifdef DO_RENDERSTATS
        if( (CV_r_stats==6 || m_pDebugRenderNode) && !(rTI.m_PersFlags & RBPF_MAKESPRITE) ) 
        {
          for (nO=0; nO<nNumRI; nO++)
          {
            pObj = rRP.m_RIs[nO]->pObj;
            FX_TrackStats(pObj);
          }
        }
#endif
        for (nO=0; nO<nNumRI; ++nO)
        {
          pObj = rRP.m_RIs[nO]->pObj;
          rRP.m_pCurObject = pObj;
          rRP.m_FrameObject++;
          pI = &pObj->m_II;
          rRP.m_pCurInstanceInfo = pI;

          byte bStartQ = FX_StartQuery(rRP.m_RIs[nO]);
          if (bStartQ == 2)
            continue;

          if( (rRP.m_nBatchFilter & FB_Z) && !(rTI.m_PersFlags & RBPF_SHADOWGEN) )
            FX_SetRenderObjVisAreaStencilRef( pObj );

  #ifdef DO_RENDERLOG
          if (CRenderer::CV_r_log >= 3)
          {
            Vec3 vPos = pObj->GetTranslation();
            Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "+++ General Pass %d (Obj: %d [%.3f, %.3f, %.3f], %.3f)\n", m_RP.m_nNumRendPasses, pObj->m_Id, vPos[0], vPos[1], vPos[2], pObj->m_fDistance);
          }
  #endif





          curVS->mfSetParametersPI(pObj, pSh);
          curPS->mfSetParametersPI(NULL, NULL);

  #if defined (DIRECT3D10) && !defined(PS3)
          if (curGS)
            curGS->mfSetParametersPI(NULL, NULL);
          else
            CHWShader_D3D::mfBindGS(NULL, NULL);
  #endif

          {
            if (pRE)
              pRE->mfDraw(pSh, pPass);
            else
              FX_DrawIndexedMesh(R_PRIMV_TRIANGLES);
          }
          FX_EndQuery(rRP.m_RIs[nO], bStartQ);
        }
        EF_FogRestore(bFogOverrided);

        rRP.m_FlagsShader_MD &= ~(HWMD_TCM | HWMD_TCG);
        if (pSaveObj != rRP.m_pCurObject)
        {
          rRP.m_pCurObject = pSaveObj;
          rRP.m_pCurInstanceInfo = &pSaveObj->m_II;
          rRP.m_FrameObject++;
        }
      }
    }
  }
}

//============================================================================================

void CD3D9Renderer::FX_DrawShader_General(CShader *ef, SShaderTechnique *pTech, bool bUseZState, bool bUseMaterialState)
{
  SShaderPass *slw;
  int32 i;

  PROFILE_FRAME(DrawShader_Generic);

  SThreadInfo& RESTRICT_REFERENCE rTI = m_RP.m_TI[m_RP.m_nProcessThreadID];

  EF_Scissor(false, 0,0,0,0);

  if (pTech->m_Passes.Num())
  {
    slw = &pTech->m_Passes[0];
    const int nCount = pTech->m_Passes.Num();
    for (i=0; i<nCount; i++, slw++)
    {    
      m_RP.m_pCurPass = slw;

      // Set all textures and HW TexGen modes for the current pass (ShadeLayer)
      assert (slw->m_VShader && slw->m_PShader);
      if (!slw->m_VShader || !slw->m_PShader)
        continue;
      CHWShader_D3D *curVS = (CHWShader_D3D *)slw->m_VShader;
      CHWShader_D3D *curPS = (CHWShader_D3D *)slw->m_PShader;

      if (rTI.m_PersFlags & RBPF_SHADOWGEN)
      {
        if (slw->m_eCull == eCULL_None)
          m_cEF.m_TempVecs[1][0] = rTI.m_vFrustumInfo.w;
      }

      FX_CommitStates(pTech, slw, (slw->m_PassFlags & SHPF_NOMATSTATE) == 0);

//#if !defined(XENON) && !defined(PS3)
      if (m_RP.m_FlagsPerFlush & RBSI_INSTANCED)
      {
        // Using HW geometry instancing approach
        FX_DrawShader_InstancedHW(ef, slw);
      }
      else
//#endif
      {
        // Set Pixel shader and all associated textures
        bool bRes = curPS->mfSetPS(HWSF_SETTEXTURES);
        if (bRes)
          FX_DrawBatches(ef, slw, curVS, curPS);
      }
    }
  }
}

// Draw terrain pass(es)
void CD3D9Renderer::FX_DrawShader_Terrain(CShader *ef, SShaderTechnique *pTech)
{
  // Light terrain layers
  FX_DrawMultiLightPasses(ef, pTech, 0);
}

// Draw detail textures passes (used in programmable pipeline shaders)
void CD3D9Renderer::FX_DrawDetailOverlayPasses()
{    
  if (!m_RP.m_pRootTechnique || m_RP.m_pRootTechnique->m_nTechnique[TTYPE_DETAIL] < 0)
    return;

  bool bDetailDecal = (m_RP.m_pShaderResources->m_ResFlags & MTL_FLAG_DETAIL_DECAL)!=0;
  CShader *sh = m_RP.m_pShader;
  SShaderTechnique *pTech = m_RP.m_pShader->m_HWTechniques[m_RP.m_pRootTechnique->m_nTechnique[TTYPE_DETAIL]];

  PROFILE_FRAME(DrawShader_DetailPasses);

  sTempRIs.SetUse(0);

  float fDistToCam = 500.0f;
  float fDist = CV_r_detaildistance;
  bool bReuse = false;
  if (m_RP.m_pRE)
  {
    CRenderObject *pObj = m_RP.m_pCurObject;
    uint32 nObj;

    if( !bDetailDecal )
    {
      for (nObj=0; nObj<m_RP.m_RIs.Num(); nObj++)
      {
        pObj = m_RP.m_RIs[nObj]->pObj;
        float fDistObj = pObj->m_fDistance;
        if (fDistObj <= fDist+4.0f && !((pObj->m_nMaterialLayers&MTL_LAYER_BLEND_CLOAK)>>8)  ) // special case: skip if using  cloak layer
          sTempRIs.AddElem(m_RP.m_RIs[nObj]);
      }
    }
    else
    {
      bReuse = true;
    }
  } //
  else
    return;

  if (bReuse)
  {
    if (!m_RP.m_RIs.Num())
      return;
  }
  else
  {
    if (!sTempRIs.Num())
      return;
  }

  PROFILE_LABEL_PUSH( "DETAIL_TEXTURE_PASS" );

  uint64 nSaveRT = m_RP.m_FlagsShader_RT;
  uint32 nSaveMD = m_RP.m_FlagsShader_MD;
  uint32 nSavePersFlags2 = m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2;
  bool bHDRMode = IsHDRModeEnabled();
  bool bLinearSpaceShading = IsLinearSpaceShadingEnabled();






  TArray<SRendItem *> saveArr;
  if (!bReuse)
  {
    saveArr.Assign(m_RP.m_RIs);
    m_RP.m_RIs.Assign(sTempRIs);
  }
  CRenderObject *pSaveObject = m_RP.m_pCurObject;
  m_RP.m_pCurObject = m_RP.m_RIs[0]->pObj;
  m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
  m_RP.m_FlagsShader_MD &= ~(HWMD_TCM | HWMD_TCG);

  Vec4 data[4];
  void *pCustomData = m_RP.m_pRE->m_CustomData;
  m_RP.m_pRE->m_CustomData = &data[0].x;

  if( !bDetailDecal )
  {
    SEfResTexture *rt = m_RP.m_pShaderResources->m_Textures[EFTT_DETAIL_OVERLAY];    
    float fUScale = rt->m_TexModificator.m_Tiling[0];
    float fVScale = rt->m_TexModificator.m_Tiling[1];
    if (!fUScale)
      fUScale = CV_r_detailscale;
    if (!fVScale)
      fVScale = CV_r_detailscale;
    data[0].x = fUScale; data[0].y = fVScale;
    data[0].z = 0;
    data[0].w = CV_r_detaildistance;
    uint32 n = CLAMP(CV_r_detailnumlayers, 1, 3);
    m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3]);
    if (n >= 1)
    {
      //m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
      if (n >= 2)
      {
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
        if (n >= 3)
          m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];
      }
    }
  }
  else
  {
    m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3]);
    SDetailDecalInfo *pDetailDecalInfo = m_RP.m_pShaderResources->m_pDetailDecalInfo;    
   
    data[0] = Vec4(pDetailDecalInfo->vTileOffs[0].x, pDetailDecalInfo->vTileOffs[0].y, pDetailDecalInfo->vTileOffs[1].x, pDetailDecalInfo->vTileOffs[1].y);
    data[1] = Vec4(pDetailDecalInfo->vTileOffs[0].z, pDetailDecalInfo->vTileOffs[0].w, pDetailDecalInfo->vTileOffs[1].z, pDetailDecalInfo->vTileOffs[1].w);

    const float fThresholdRatio = 10.0f / 255.0f;
    data[2] = Vec4(Word2Degr(pDetailDecalInfo->nRotation[0]), Word2Degr(pDetailDecalInfo->nRotation[1]), pDetailDecalInfo->nThreshold[0] * fThresholdRatio, pDetailDecalInfo->nThreshold[1] * fThresholdRatio);

    const float fNormRatio = 1.0f / 255.0f;
    data[3] = Vec4( (float)pDetailDecalInfo->nDeformation[0], (float)pDetailDecalInfo->nDeformation[1], (float)pDetailDecalInfo->nSSAOAmount, (float)pDetailDecalInfo->nBlending) * fNormRatio;
    
    const float fSSAOThreshold = 0.01f;    
    
    // Disable ssao if not used
    if( data[3].z > fSSAOThreshold )
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];

    // save 2 instruction in ps
    data[3].z = 1.0f - data[3].z; // pass inverted ssao amount 
    data[3].w *= 100.0f; // set maximum range for blending 

    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
  }







  FX_DrawTechnique(sh, pTech, true, false);







  if (!bReuse)
  {
    m_RP.m_RIs.Assign(saveArr);
    saveArr.ClearArr();
  }

  m_RP.m_pCurObject = pSaveObject;
  m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
  m_RP.m_pPrevObject = NULL;
  m_RP.m_FrameObject++;

  m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 = nSavePersFlags2;

  m_RP.m_FlagsShader_RT = nSaveRT;
  m_RP.m_FlagsShader_MD = nSaveMD;
  m_RP.m_pRE->m_CustomData = pCustomData;

  PROFILE_LABEL_POP( "DETAIL_TEXTURE_PASS" );
}

void CD3D9Renderer::FX_DrawEffectLayerPasses()
{
	if (!m_RP.m_pRootTechnique || m_RP.m_pRootTechnique->m_nTechnique[TTYPE_EFFECTLAYER] < 0)
		return;

	CShader *sh = m_RP.m_pShader;
	SShaderTechnique *pTech = m_RP.m_pShader->m_HWTechniques[m_RP.m_pRootTechnique->m_nTechnique[TTYPE_EFFECTLAYER]];

	PROFILE_FRAME(DrawShader_EffectLayerPasses);

	sTempRIs.SetUse(0);

	float fDistToCam = 500.0f;
	float fDist = CV_r_detaildistance;
	bool bReuse = false;
	if (m_RP.m_pRE)
	{
		CRenderObject *pObj = m_RP.m_pCurObject;
		uint32 nObj;

		for (nObj=0; nObj<m_RP.m_RIs.Num(); nObj++)
		{
			pObj = m_RP.m_RIs[nObj]->pObj;
			float fDistObj = pObj->m_fDistance;
			if ( fDistObj <= fDist+4.0f ) 
				sTempRIs.AddElem(m_RP.m_RIs[nObj]);
		}
	} //
	else
		return;

	if (bReuse)
	{
		if (!m_RP.m_RIs.Num())
			return;
	}
	else
	{
		if (!sTempRIs.Num())
			return;
	}

	PROFILE_LABEL_PUSH( "EFFECT_LAYER_PASS" );

	uint64 nSaveRT = m_RP.m_FlagsShader_RT;
	uint32 nSaveMD = m_RP.m_FlagsShader_MD;
	uint32 nSavePersFlags2 = m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2;
	bool bHDRMode = IsHDRModeEnabled();
	bool bLinearSpaceShading = IsLinearSpaceShadingEnabled();






	TArray<SRendItem *> saveArr;
	if (!bReuse)
	{
		saveArr.Assign(m_RP.m_RIs);
		m_RP.m_RIs.Assign(sTempRIs);
	}
	CRenderObject *pSaveObject = m_RP.m_pCurObject;
	m_RP.m_pCurObject = m_RP.m_RIs[0]->pObj;
	m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
	m_RP.m_FlagsShader_MD &= ~(HWMD_TCM | HWMD_TCG);

	Vec4 data[4];
	void *pCustomData = m_RP.m_pRE->m_CustomData;
	m_RP.m_pRE->m_CustomData = &data[0].x;

	SEfResTexture *rt = m_RP.m_pShaderResources->m_Textures[EFTT_CUSTOM];    

	const float fDefaultUVTilling = 20;
	float fUScale = rt->m_TexModificator.m_Tiling[0];
	float fVScale = rt->m_TexModificator.m_Tiling[1];
	if (!fUScale)
		fUScale = fDefaultUVTilling;
	if (!fVScale)
		fVScale = fDefaultUVTilling;

	data[0].x = fUScale; data[0].y = fVScale;
	data[0].z = rt->m_TexModificator.m_UOscRate;
	if( !data[0].z )
		data[0].z = 1.0f;

	// Flicker timming for sparks/plasma
	float fTime = m_RP.m_TI[m_RP.m_nProcessThreadID].m_RealTime;
	data[0].w = (fTime * 20.0f + fTime * 4.0f);
	data[0].w -= floorf( data[0].w );
	data[0].w = fabs( data[0].w *2.0f - 1.0f );
	data[0].w *= data[0].w;

	int32 nMaterialStatePrev = m_RP.m_MaterialState;
	m_RP.m_MaterialState &= ~GS_BLEND_MASK;
	m_RP.m_MaterialState |= GS_BLSRC_ONE | GS_BLDST_ONE;

	m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3]|g_HWSR_MaskBit[HWSR_SAMPLE4]);





	FX_DrawTechnique(sh, pTech, true, false);

	if (!bReuse)
	{
		m_RP.m_RIs.Assign(saveArr);
		saveArr.ClearArr();
	}

	m_RP.m_pCurObject = pSaveObject;
	m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
	m_RP.m_pPrevObject = NULL;
	m_RP.m_FrameObject++;

	m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 = nSavePersFlags2;

	m_RP.m_MaterialState = nMaterialStatePrev;
	m_RP.m_FlagsShader_RT = nSaveRT;
	m_RP.m_FlagsShader_MD = nSaveMD;
	m_RP.m_pRE->m_CustomData = pCustomData;

	PROFILE_LABEL_POP( "EFFECT_LAYER_PASS" );
}

void CD3D9Renderer::FX_DrawCausticsPasses( )
{
  // todo: test if stencil pre-pass is worth when above water
  if( (!(m_RP.m_nRendFlags & SHDF_ALLOW_WATER) && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2&RBPF_MIRRORCAMERA)) || !m_RP.m_pRootTechnique || m_RP.m_pRootTechnique->m_nTechnique[TTYPE_CAUSTICS] < 0 )
    return;
  
  if( !m_RP.m_pCurObject || !m_RP.m_pShader )
    return;

  if( (m_RP.m_pCurObject->m_ObjFlags & FOB_DECAL) || (m_RP.m_pShader->m_Flags & EF_DECAL) )
    return;
  
  static int nFrameID = 0;
  static bool bOceanVolumeVisible = false;
  static ICVar *pVar = iConsole->GetCVar("e_WaterOcean");
  static bool bOceanEnabled = 0;
  static float fWatLevel = 0;
  static Vec4 pCausticsParams;

  int nCurrFrameID = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameID;

  // Only get 3dengine data once..
  if( nFrameID != nCurrFrameID )
  {
    nFrameID = nCurrFrameID;
    bOceanVolumeVisible = (gEnv->p3DEngine->GetOceanRenderFlags() & OCR_OCEANVOLUME_VISIBLE) != 0;
    pCausticsParams = gEnv->p3DEngine->GetCausticsParams();
    fWatLevel = gEnv->p3DEngine->GetWaterLevel();
  }

  if( !bOceanVolumeVisible )
    return;

  float fDistToSectorWithWater = gEnv->p3DEngine->GetDistanceToSectorWithWater();
  if( fDistToSectorWithWater > CRenderer::CV_r_watercausticsdistance * 0.5f )
    return;

  CShader *sh = m_RP.m_pShader;
  SShaderTechnique *pTech = m_RP.m_pShader->m_HWTechniques[m_RP.m_pRootTechnique->m_nTechnique[TTYPE_CAUSTICS]];

  PROFILE_FRAME(DrawShader_CausticsPasses);

  sTempRIs.SetUse(0);

  Vec3 pMinStart;
  float fDistToCam = 500.0f;
  float fDist = CRenderer::CV_r_watercausticsdistance;//pCausticsParams.y;
  if (m_RP.m_pRE)
  {
    CRenderObject *pObj = NULL;
    uint32 nObj;
 
    AABB bb;
    m_RP.m_pRE->mfGetBBox(bb.min, bb.max);        
    
    Vec3 pMin, pMinOS;        
    pMinStart = bb.min;

    for (nObj=0; nObj<m_RP.m_RIs.Num(); nObj++)
    {
      pObj = m_RP.m_RIs[nObj]->pObj;
      float fDistObj = pObj->m_fDistance;
                  
      if (fDistObj <= fDist+4.0f && !(pObj->m_nMaterialLayers&MTL_LAYER_BLEND_CLOAK) ) // special case: disable caustics when cloak active
      {

        AABB bbObj = bb.CreateTransformedAABB(pObj->GetMatrix(), bb);  
        float fBBRadius = bbObj.GetRadius();

        if( m_RP.m_nPassGroupID != EFSLIST_TERRAINLAYER )
          pMin = bbObj.GetCenter();
        else  
          pMin = bbObj.min;
        
        if (pMin.z < fWatLevel )
          sTempRIs.AddElem(m_RP.m_RIs[nObj]);
      }
    }
  }
  else
    return;

  if (!sTempRIs.Num())
    return;

  PROFILE_LABEL_PUSH( "CAUSTICS_PASS" );

  uint64 nSaveRT = m_RP.m_FlagsShader_RT;
//  m_RP.m_FlagsShader_RT = 0;

  Vec4 data = Vec4(pMinStart.x, pMinStart.y, pMinStart.z, 1.0f);
  void *pCustomData = m_RP.m_pRE->m_CustomData;
  if( m_RP.m_nPassGroupID != EFSLIST_TERRAINLAYER )
    m_RP.m_pRE->m_CustomData = &data.x;

  TArray<SRendItem *> saveArr;
  saveArr.Assign(m_RP.m_RIs);
  CRenderObject *pSaveObject = m_RP.m_pCurObject;

  m_RP.m_RIs.Assign(sTempRIs);
  m_RP.m_pCurObject = m_RP.m_RIs[0]->pObj;
  m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;

  int nPersFlags2Save = m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2;

  static int nStencilFrameID = 0;
  if( CRenderer::CV_r_watercaustics == 2 )
  { 
    if( nStencilFrameID != GetFrameID())
    {
      nStencilFrameID = GetFrameID();

      FX_ResetPipe();     
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
    }

    EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_EQUAL) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
      STENCOP_PASS(FSS_STENCOP_KEEP),
      m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFFFFFF);

    //m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |=RBPF2_LIGHTSTENCILCULL;
  }


  FX_DrawTechnique(sh, pTech, true, false);

  m_RP.m_RIs.Assign(saveArr);
  saveArr.ClearArr();

  m_RP.m_pCurObject = pSaveObject;
  m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
  m_RP.m_pPrevObject = NULL;
  m_RP.m_FrameObject++;

  m_RP.m_FlagsShader_RT = nSaveRT;  
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 = nPersFlags2Save;

  if( m_RP.m_nPassGroupID != EFSLIST_TERRAINLAYER )
    m_RP.m_pRE->m_CustomData = pCustomData;

  PROFILE_LABEL_POP( "CAUSTICS_PASS" );
}

void CD3D9Renderer::FX_DrawDebugPasses()
{
	if (!m_RP.m_pRootTechnique || m_RP.m_pRootTechnique->m_nTechnique[TTYPE_DEBUG] < 0)
		return;

	CShader *sh = m_RP.m_pShader;
	SShaderTechnique *pTech = m_RP.m_pShader->m_HWTechniques[m_RP.m_pRootTechnique->m_nTechnique[TTYPE_DEBUG]];

	PROFILE_FRAME(DrawShader_DebugPasses);

	sTempRIs.SetUse(0);

	if (!m_RP.m_pRE)
		return;

	for (uint32 i=0; i<m_RP.m_RIs.Num(); i++)
		sTempRIs.AddElem(m_RP.m_RIs[i]);

	if (!sTempRIs.Num())
		return;

	PROFILE_LABEL_PUSH("DEBUG_PASS");

	uint32 nSaveMD = m_RP.m_FlagsShader_MD;

	TArray<SRendItem *> saveArr;
	saveArr.Assign(m_RP.m_RIs);
	m_RP.m_RIs.Assign(sTempRIs);

	CRenderObject *pSaveObject = m_RP.m_pCurObject;
	m_RP.m_pCurObject = m_RP.m_RIs[0]->pObj;
	m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
	m_RP.m_FlagsShader_MD &= ~(HWMD_TCM | HWMD_TCG);
	int32 nMaterialStatePrev = m_RP.m_MaterialState;
	m_RP.m_MaterialState &= ~GS_BLEND_MASK;
	m_RP.m_MaterialState |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;

	FX_DrawTechnique(sh, pTech, true, false);

	m_RP.m_RIs.Assign(saveArr);
	saveArr.ClearArr();

	m_RP.m_pCurObject = pSaveObject;
	m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
	m_RP.m_pPrevObject = NULL;
	m_RP.m_FlagsShader_MD = nSaveMD;
	m_RP.m_MaterialState = nMaterialStatePrev;

	m_RP.m_FrameObject++;

	PROFILE_LABEL_POP("DEBUG_PASS");
}

void CD3D9Renderer::FX_SetupMultiLayers( bool bEnable )
{
  CREMesh *pRE = (CREMesh *)m_RP.m_pRE;
  CRenderObject *pObj =  m_RP.m_pCurObject;

  if (!(pObj->m_nMaterialLayers&MTL_LAYER_BLEND_FROZEN) || !pRE)
    return; 

  if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN)
    return;

  if( (SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] > 1 && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE)) )
    return;

  IMaterial *pObjMat = pObj->m_pCurrMaterial;  
  if (!m_RP.m_pShaderResources || !pObjMat)
    return;

  uint32 nResourcesNoDrawFlags = m_RP.m_pShaderResources->GetMtlLayerNoDrawFlags();
  if (nResourcesNoDrawFlags & MTL_LAYER_FROZEN)
    return;

  if (pObj->m_ObjFlags & FOB_DECAL || (m_RP.m_pShader->m_Flags & EF_DECAL) )
    return;

  PROFILE_FRAME(SetupMultiLayers);

  // Verify if current mesh has valid data for layers  
  static IMaterial *pDefaultMtl = gEnv->p3DEngine->GetMaterialManager()->GetDefaultLayersMaterial();
  static SShaderItem pLayerShaderItem;

  static SRenderShaderResources *pPrevShaderResources = 0; 
  static SEfResTexture **pPrevResourceTexs;
  static SEfResTexture *pPrevLayerResourceTexs[EFTT_MAX];

  bool bDefaultLayer = false;

  if( bEnable )
  {    
    
    m_RP.m_pReplacementShader = 0;

    CRenderChunk *pChunk = pRE->m_pChunk;
    if( !pChunk )
      return;

    // Check if chunk material has layers at all    
    IMaterial *pCurrMtl = pObjMat->GetSubMtlCount()? pObjMat->GetSubMtl( pChunk->m_nMatID ) : pObjMat; 
    if( !pCurrMtl || !pDefaultMtl || (pCurrMtl->GetFlags() & MTL_FLAG_NODRAW ) )
      return;

    // Atm only frozen layer supports replacing of general pass
    uint8 nMaterialLayers = ((pObj->m_nMaterialLayers&MTL_LAYER_BLEND_FROZEN)>>24)? MTL_LAYER_FROZEN : 0;

    IMaterialLayer *pDefaultLayer = const_cast< IMaterialLayer* >( pDefaultMtl->GetLayer( nMaterialLayers, 0 ) ); 
    IMaterialLayer *pLayer = const_cast< IMaterialLayer* >( pCurrMtl->GetLayer( nMaterialLayers, 0 ) );      
    if( !pLayer )
    {
      bDefaultLayer = true;
      pLayer = pDefaultLayer;      

      if( !pLayer )
        return;
    }    

    if( !pLayer->IsEnabled() )
      return;

    pLayerShaderItem = pLayer->GetShaderItem();

    // Check for valid shader
    CShader *pShader = static_cast< CShader * >(pLayerShaderItem.m_pShader);
    if( !pShader || !pShader->m_HWTechniques.Num() || !(pShader->m_Flags2&EF2_SUPPORTS_REPLACEBASEPASS) || !pLayerShaderItem.m_pShaderResources )
      return;

    // Custom textures replacement    
    pPrevResourceTexs = m_RP.m_pShaderResources->m_Textures;
    pPrevShaderResources = m_RP.m_pShaderResources; 

    // Keep layer resources and replace with resources from base shader
    for(int t = 0; t < EFTT_MAX; ++t) 
    {
      pPrevLayerResourceTexs[t] = ((SRenderShaderResources *)pLayerShaderItem.m_pShaderResources)->m_Textures[t];
      ((SRenderShaderResources *)pLayerShaderItem.m_pShaderResources)->m_Textures[t] = pPrevResourceTexs[t];
    }

    if( bDefaultLayer )
    {
      // Get opacity/alpha test values (only required for default layer, which has no information)
      ((SRenderShaderResources *)pLayerShaderItem.m_pShaderResources)->m_AlphaRef = pPrevShaderResources->m_AlphaRef;
      
      if( pPrevShaderResources->m_ResFlags & MTL_FLAG_2SIDED )
        ((SRenderShaderResources *)pLayerShaderItem.m_pShaderResources)->m_ResFlags |= MTL_FLAG_2SIDED;
      else
        ((SRenderShaderResources *)pLayerShaderItem.m_pShaderResources)->m_ResFlags &= ~MTL_FLAG_2SIDED;

      ((SRenderShaderResources *)pLayerShaderItem.m_pShaderResources)->m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][3] = pPrevShaderResources->m_Constants[eHWSC_Pixel][PS_DIFFUSE_COL][3];
    }

    // Replace shader and resources
    m_RP.m_pShader = pShader;       
    m_RP.m_pReplacementShader = m_RP.m_pShader;
    m_RP.m_pShaderResources = (SRenderShaderResources *)pLayerShaderItem.m_pShaderResources;
  }
  else
  {
    if( pPrevShaderResources )
    {      
      // Restore custom resources
      m_RP.m_pReplacementShader = 0;

      for(int t = 0; t < EFTT_MAX; ++t)
      {
        ((SRenderShaderResources *)pLayerShaderItem.m_pShaderResources)->m_Textures[t] = pPrevLayerResourceTexs[t];
        pPrevLayerResourceTexs[t] = 0;
      }

      m_RP.m_pShaderResources = pPrevShaderResources;   
      pPrevShaderResources = 0;         

    }
  } 
}

void CD3D9Renderer::FX_DrawMultiLayers()
{
  // Verify if current mesh has valid data for layers
  CREMesh *pRE = (CREMesh *)m_RP.m_pRE;  
  if (!m_RP.m_pShader || !m_RP.m_pShaderResources || !m_RP.m_pCurObject->m_nMaterialLayers)
    return;

  IMaterial *pObjMat = m_RP.m_pCurObject->m_pCurrMaterial;   
  if ((SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] > 1 && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE)) || !m_RP.m_pShaderResources || !pObjMat)
    return;

  CRenderChunk *pChunk = pRE->m_pChunk;
  if (!pChunk)
  {
    assert(pChunk);
    return;
  }
  
  // Check if chunk material has layers at all
  IMaterial *pDefaultMtl = gEnv->p3DEngine->GetMaterialManager()->GetDefaultLayersMaterial();
  IMaterial *pCurrMtl = pObjMat->GetSubMtlCount()? pObjMat->GetSubMtl( pChunk->m_nMatID ) : pObjMat;  
  if (!pCurrMtl || !pDefaultMtl || (pCurrMtl->GetFlags() & MTL_FLAG_NODRAW))
    return;

  uint32 nLayerCount = pDefaultMtl->GetLayerCount();
  if (!nLayerCount)
    return;

  // Start multi-layers processing
  PROFILE_FRAME(DrawShader_MultiLayers);

  if (m_LogFile)
    Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Start Multilayers processing ***\n");

  // Render all layers
  for(uint32 nCurrLayer(0); nCurrLayer < nLayerCount ; ++nCurrLayer) 
  { 
    IMaterialLayer *pLayer = const_cast< IMaterialLayer* >(pCurrMtl->GetLayer(nCurrLayer));
    IMaterialLayer *pDefaultLayer =  const_cast< IMaterialLayer* >(pDefaultMtl->GetLayer(nCurrLayer));
    bool bDefaultLayer = false;
    if(!pLayer)
    {
      // Replace with default layer
      pLayer =  pDefaultLayer;
      bDefaultLayer = true;

      if( !pLayer )
        continue;
    }

    if( !pLayer->IsEnabled() )
      continue;

    // Set/verify layer shader technique 
    SShaderItem &pCurrShaderItem = pLayer->GetShaderItem();      
    CShader *pSH = static_cast<CShader*>(pCurrShaderItem.m_pShader);      
    if( !pSH || pSH->m_HWTechniques.empty())
      continue;

    SShaderTechnique *pTech = pSH->m_HWTechniques[0];
    if(!pTech) 
    {
      continue;
    }

    // Re-create render object list, based on layer properties
    {
      sTempRIs.SetUse(0);

      float fDistToCam = 500.0f;
      float fDist = CV_r_detaildistance;
      CRenderObject *pObj = m_RP.m_pCurObject;
      uint32 nObj = 0;

      for (nObj=0; nObj<m_RP.m_RIs.Num(); nObj++)
      {
        pObj = m_RP.m_RIs[nObj]->pObj;
        uint8 nMaterialLayers = 0;
        nMaterialLayers |= ((pObj->m_nMaterialLayers&MTL_LAYER_BLEND_DYNAMICFROZEN))? MTL_LAYER_FROZEN : 0;
        nMaterialLayers |= ((pObj->m_nMaterialLayers&MTL_LAYER_BLEND_CLOAK)>>8)? MTL_LAYER_CLOAK : 0;

        if ( nMaterialLayers & (1<<nCurrLayer) )   
          sTempRIs.AddElem(m_RP.m_RIs[nObj]); 
      }

      // nothing in render list
      if( !sTempRIs.Num() )
        continue;
    }

    SShaderItem &pMtlShaderItem = pCurrMtl->GetShaderItem();    

    SEfResTexture **pResourceTexs = ((SRenderShaderResources *)pCurrShaderItem.m_pShaderResources)->m_Textures;
    SEfResTexture *pPrevLayerResourceTexs[EFTT_MAX];

    if( bDefaultLayer )
    {
      // Keep layer resources and replace with resources from base shader
      for(int t = 0; t < EFTT_MAX; ++t) 
      {
        pPrevLayerResourceTexs[t] = ((SRenderShaderResources *)pCurrShaderItem.m_pShaderResources)->m_Textures[t];
        ((SRenderShaderResources *)pCurrShaderItem.m_pShaderResources)->m_Textures[t] = m_RP.m_pShaderResources->m_Textures[t];
      }
    }

    // Store current rendering data
    TArray<SRendItem *> pPrevRenderObjLst;
    pPrevRenderObjLst.Assign( m_RP.m_RIs );
    CRenderObject *pPrevObject = m_RP.m_pCurObject;
    SRenderShaderResources *pPrevShaderResources = m_RP.m_pShaderResources;      
    CShader *pPrevSH = m_RP.m_pShader;
    uint32 nPrevNumRendPasses = m_RP.m_nNumRendPasses;
    uint64 nFlagsShaderRTprev = m_RP.m_FlagsShader_RT;

    SShaderTechnique *pPrevRootTech = m_RP.m_pRootTechnique;
    m_RP.m_pRootTechnique = pTech;

    bool pNotFirstPassprev = m_RP.m_bNotFirstPass;
    int nMaterialStatePrev = m_RP.m_MaterialState;
    uint32 nFlagsShaderLTprev = m_RP.m_FlagsShader_LT;
    int nCurLightPass = m_RP.m_nCurLightPass;

    int nPersFlagsPrev = m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags;
    int nPersFlags2Prev = m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2;
    int nMaterialAlphaRefPrev = m_RP.m_MaterialAlphaRef;
    short nLightGroupPrev = m_RP.m_nCurLightGroup;
    bool bIgnoreObjectAlpha = m_RP.m_bIgnoreObjectAlpha;
    m_RP.m_bIgnoreObjectAlpha = true;

    // Reset light passes (need ambient)
    m_RP.m_nNumRendPasses = 0;
    m_RP.m_nCurLightGroup = 0;
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 |= RBPF2_MATERIALLAYERPASS;    

    m_RP.m_pShader = pSH;
    m_RP.m_RIs.Assign(sTempRIs);
    m_RP.m_pCurObject = m_RP.m_RIs[0]->pObj;    
    m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
    m_RP.m_pPrevObject = NULL;
    m_RP.m_pShaderResources = (SRenderShaderResources *)pCurrShaderItem.m_pShaderResources; 

    if( (1<<nCurrLayer) & MTL_LAYER_FROZEN )
    {
       m_RP.m_MaterialState = (m_RP.m_MaterialState & ~(GS_BLEND_MASK|GS_ALPHATEST_MASK))  | (GS_BLSRC_ONE | GS_BLDST_ONE);
       m_RP.m_MaterialAlphaRef = 0xff;
    }

    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];

		FX_DrawTechnique(pSH, pTech, false, true);       

    // Restore previous rendering data
    m_RP.m_RIs.Assign( pPrevRenderObjLst );
    pPrevRenderObjLst.ClearArr();
    m_RP.m_pShader = pPrevSH;
    m_RP.m_pShaderResources = pPrevShaderResources;
    m_RP.m_pCurObject = pPrevObject;
    m_RP.m_pCurInstanceInfo = &m_RP.m_pCurObject->m_II;
    m_RP.m_pPrevObject = NULL;
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 = nPersFlags2Prev;    

    m_RP.m_nNumRendPasses = nPrevNumRendPasses;

    m_RP.m_bNotFirstPass = pNotFirstPassprev;
    m_RP.m_FlagsShader_LT = nFlagsShaderLTprev;
    m_RP.m_nCurLightPass = nCurLightPass;
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags = nPersFlagsPrev;
    m_RP.m_FlagsShader_RT = nFlagsShaderRTprev;

    m_RP.m_nNumRendPasses = 0;
    m_RP.m_nCurLightGroup = nLightGroupPrev;

    m_RP.m_pRootTechnique = pPrevRootTech;
    m_RP.m_bIgnoreObjectAlpha = bIgnoreObjectAlpha;
    m_RP.m_MaterialState = nMaterialStatePrev;
    m_RP.m_MaterialAlphaRef = nMaterialAlphaRefPrev;

    if( bDefaultLayer )
    {
      for(int t = 0; t < EFTT_MAX; ++t)
      {
        ((SRenderShaderResources *)pCurrShaderItem.m_pShaderResources)->m_Textures[t] = pPrevLayerResourceTexs[t];
        pPrevLayerResourceTexs[t] = 0;
      }
    }

    m_RP.m_FrameObject++;    

    //break; // only 1 layer allowed
  } 

  if (m_LogFile)
    Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** End Multilayers processing ***\n");
}

void CD3D9Renderer::FX_SelectTechnique(CShader *pShader, SShaderTechnique *pTech)
{
  SShaderTechniqueStat Stat;
  Stat.pTech = pTech;
  Stat.pShader = pShader;
  if (pTech->m_Passes.Num())
  {
    SShaderPass *pPass = &pTech->m_Passes[0];
    if (pPass->m_PShader && pPass->m_VShader)
    {
      Stat.pVS = (CHWShader_D3D *)pPass->m_VShader;
      Stat.pPS = (CHWShader_D3D *)pPass->m_PShader;
      Stat.pVSInst = Stat.pVS->m_pCurInst;
      Stat.pPSInst = Stat.pPS->m_pCurInst;
      g_SelectedTechs.push_back(Stat);
    }
  }
}

void CD3D9Renderer::FX_DrawTechnique(CShader *ef, SShaderTechnique *pTech, bool bGeneral, bool bUseMaterialState)
{
  switch(ef->m_eSHDType)
  {
  case eSHDT_General:
    FX_DrawShader_General(ef, pTech, true, bUseMaterialState);
    break;
  case eSHDT_Light:
    if (bGeneral)
      FX_DrawShader_General(ef, pTech, false, bUseMaterialState);
    else
      FX_DrawMultiLightPasses(ef, pTech, 0);
    break;
  case eSHDT_Terrain:
    if (bGeneral)  
      FX_DrawShader_General(ef, pTech, false, bUseMaterialState);
    else
      FX_DrawShader_Terrain(ef, pTech);
    break;
  case eSHDT_Fur:
    break;
  case eSHDT_CustomDraw:
  case eSHDT_Sky:
    if (m_RP.m_pRE) 
    {
      EF_Scissor(false, 0, 0, 0, 0);
      if (pTech && pTech->m_Passes.Num())
        m_RP.m_pRE->mfDraw(ef, &pTech->m_Passes[0]);
      else
        m_RP.m_pRE->mfDraw(ef, NULL);
    }
    break;
  default:
    assert(0);
  }
  if (m_RP.m_ObjFlags & FOB_SELECTED)
    FX_SelectTechnique(ef, pTech);
}

void sDetectInstancing(CShader *pShader, CRenderObject *pObj)
{
	assert(CRenderer::m_iGeomInstancingThreshold>=1);			// call ChangeGeomInstancingThreshold();

  // Hardware instancing works only if:
  // 1. no projected light
  // 2. number of instances exceeds m_iGeomInstancingThreshold
  // 3. shader and mesh supports instancing
  CRenderer *rd = gRenDev;
  if (CRenderer::CV_r_geominstancing == 3 && rd->m_RP.m_RIs.Num() > CRenderer::m_iGeomInstancingThreshold && (pShader->m_Flags & EF_SUPPORTSINSTANCING))
  {  // use mesh duplicating approach
    if (rd->m_RP.m_pRE && rd->m_RP.m_pRE->mfGetType() == eDATA_Mesh)
    {
      CREMesh *pRE = (CREMesh *)rd->m_RP.m_pRE;
      CRenderMesh2 *pMesh = pRE->m_pRenderMesh;
      if (pMesh->_GetFlags() & FRM_INSTANCED)
        rd->m_RP.m_FlagsPerFlush |= RBSI_INSTANCED;
    }
    else
      rd->m_RP.m_FlagsPerFlush &= ~RBSI_INSTANCED;
  }
  else
	if (CRenderer::CV_r_geominstancing && ((int)rd->m_RP.m_RIs.Num()>CRenderer::m_iGeomInstancingThreshold || (rd->m_RP.m_FlagsPerFlush & RBSI_INSTANCED)) && (pShader->m_Flags & EF_SUPPORTSINSTANCING) && !CRenderer::CV_r_measureoverdraw)
    rd->m_RP.m_FlagsPerFlush |= RBSI_INSTANCED;
  else
    rd->m_RP.m_FlagsPerFlush &= ~RBSI_INSTANCED;
}


// Set/Restore shader resources overrided states
bool CD3D9Renderer::FX_SetResourcesState()
{
  if (!m_RP.m_pShader)
    return false;
  m_RP.m_MaterialState = 0;
  if (m_RP.m_pShader->m_Flags2 & EF2_IGNORERESOURCESTATES)
    return true;
  if (!m_RP.m_pShaderResources)
    return true;
  if (m_RP.m_pShaderResources->m_ResFlags & MTL_FLAG_NOTINSTANCED)
    m_RP.m_FlagsPerFlush &= ~RBSI_INSTANCED;

  bool bRes = true;
  if (m_RP.m_pShaderResources->m_ResFlags & MTL_FLAG_2SIDED)
  {
    D3DSetCull(eCULL_None);
    m_RP.m_FlagsPerFlush |= RBSI_NOCULL;    
  }
  m_RP.m_ShaderTexResources[EFTT_DECAL_OVERLAY] = NULL;

  // Only enable for resources not using zpass
  if( !(m_RP.m_pRLD->m_nBatchFlags[m_RP.m_nSortGroupID][m_RP.m_nPassGroupID] & FB_Z) || (CRenderer::CV_r_deferredshading && (m_RP.m_pShader->m_Flags & EF_DECAL)) )
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_NOZPASS ];                                           

	if( CV_r_ZPrePass && m_RP.m_nBatchFilter == FB_ZPREPASS )
		m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_ZPREPASS ];                                           
	//else
	//	m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[ HWSR_ZPREPASS ];

  if (m_RP.m_pShaderResources->m_AlphaRef)
  {
    if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_NOALPHATEST) || (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_ATOC))
    {
      int nAlphaRef = (int)(m_RP.m_pShaderResources->m_AlphaRef*255.0f);
      m_RP.m_MaterialAlphaRef = nAlphaRef;
      m_RP.m_MaterialState = GS_ALPHATEST_GEQUAL | GS_DEPTHWRITE;
    }
    else
      m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ALPHATEST];
  }
  float fOpacity;
  if ((fOpacity=m_RP.m_pShaderResources->Opacity()) != 1.0f)
  {
    if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_NOALPHABLEND))
    {
      if (m_RP.m_MaterialState != 0)
        m_RP.m_MaterialState &= ~GS_DEPTHWRITE;
      if (m_RP.m_pShaderResources->m_ResFlags & MTL_FLAG_ADDITIVE)
      {
        m_RP.m_MaterialState |= GS_BLSRC_ONE | GS_BLDST_ONE;
        m_RP.m_CurGlobalColor[0] = fOpacity;
        m_RP.m_CurGlobalColor[1] = fOpacity;
        m_RP.m_CurGlobalColor[2] = fOpacity;
      }
      else
      {
        m_RP.m_MaterialState |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
        m_RP.m_CurGlobalColor[3] = fOpacity;
      }
      m_RP.m_fCurOpacity = fOpacity;
      
      // This is incorrect - and specially visible in non-hdr
      //if (m_RP.m_nMaxPasses)
      //  m_RP.m_fCurOpacity = fOpacity / (float)(m_RP.m_nMaxPasses);
    }
  }
  if (!(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE))
  {
    if (m_RP.m_pShaderResources->m_pDeformInfo)
      m_RP.m_FlagsShader_MDV |= m_RP.m_pShaderResources->m_pDeformInfo->m_eType;
    m_RP.m_FlagsShader_MDV |= m_RP.m_pCurObject->m_nMDV | m_RP.m_pShader->m_nMDV;
    if (m_RP.m_ObjFlags & FOB_OWNER_GEOMETRY)
      m_RP.m_FlagsShader_MDV &= ~MDV_DEPTH_OFFSET;
  }

  if( (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags2 & RBPF2_OCEANPARTICLES) )
    gcpRendD3D->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_OCEAN_PARTICLE];






  return true;
}

//===================================================================================================

// Flush current render item
void CD3D9Renderer::FX_FlushShader_General()
{
  CD3D9Renderer *const __restrict rd = gcpRendD3D;
	SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
  if (!rRP.m_pRE && !rRP.m_RendNumVerts)
    return;

	CShader *ef = rRP.m_pShader;
  //bool bSupportsFullDeferred = (CV_r_deferredshading==3 && !(rRP.m_nBatchFilter&FB_TRANSPARENT) && (ef->m_Flags &EF_SUPPORTSDEFERREDSHADING_FULL) && (!rRP.m_pShaderResources || rRP.m_nPassGroupID == EFSLIST_DECAL || (rRP.m_pShaderResources && !rRP.m_pShaderResources->GetAlphaRef())) );
	if (!ef ) 
		return;

  // Shader overriding with layer
  if ((rRP.m_nBatchFilter & FB_GENERAL) && CV_r_usemateriallayers)
    rd->FX_SetupMultiLayers(true);


  SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];
  assert(!(rTI.m_PersFlags & RBPF_SHADOWGEN));
  assert(!(rRP.m_nBatchFilter & FB_Z));
	//leave out for ps3 since it costs too much
#ifndef PS3
  if (!rRP.m_sExcludeShader.empty())
  {
		char nm[1024];
    strcpy(nm, ef->GetName());
    strlwr(nm);
    if (strstr(rRP.m_sExcludeShader.c_str(), nm))
      return;
  }

  if (!rRP.m_sShowOnlyShader.empty())
  {
		char nm[1024];
    strcpy(nm, ef->GetName());
    strlwr(nm);
    if (!strstr(rRP.m_sShowOnlyShader.c_str(), nm))
      return;
  }
#endif//PS3
#ifdef DO_RENDERLOG
  if (rd->m_LogFile && CV_r_log == 3)
    rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "\n\n.. Start %s flush: '%s' ..\n", "General", ef->GetName());
#endif

	SPipeStat& rPS = rRP.m_PS[rRP.m_nProcessThreadID];
  ++rPS.m_NumRendBatches;
	const uint32 objInstNum = rRP.m_RIs.Num();
	const uint32 numRendInstances = rPS.m_NumRendInstances;
	//implement branch-free using masks:
	//	rPS.m_NumRendInstances = objInstNum?(numRendInstancesIncr+objInstNum) : (numRendInstances+1);
	const uint32 numRendInstancesMask = (uint32)(((-(int)objInstNum)) >> 31);
	rPS.m_NumRendInstances = (numRendInstances+objInstNum) & ~numRendInstancesMask | (numRendInstances+1) & numRendInstancesMask;

  CRenderObject *pObj = rRP.m_pCurObject;

  PROFILE_SHADER_START

#ifdef DO_RENDERLOG
  if (rd->m_LogFile && CV_r_log >= 3)
    rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "\n");
#endif

//#if !defined(PS3) && !defined(XENON)
  sDetectInstancing(ef, pObj);
//#endif

  // Techniques draw cycle...
  SShaderTechnique *__restrict pTech = ef->mfGetStartTechnique(rRP.m_nShaderTechnique);

  if (pTech)
  {
    if (rRP.m_pShaderResources && !(rRP.m_nBatchFilter & (FB_ZPREPASS | FB_Z | FB_GLOW | FB_CUSTOM_RENDER | FB_MOTIONBLUR | FB_SCATTER | FB_RAIN | FB_FUR)))
    {
      uint32 i;
      // Update render targets if necessary
      if (!(rTI.m_PersFlags & RBPF_DRAWTOTEXTURE))
      {
				uint32 targetNum = rRP.m_pShaderResources->m_RTargets.Num();
				const SRenderShaderResources *const __restrict pShaderResources = rRP.m_pShaderResources;
        for (i=0; i<targetNum; ++i)
        {
          SHRenderTarget *pTarg = pShaderResources->m_RTargets[i];
          if (pTarg->m_eOrder == eRO_PreDraw)
            rd->FX_DrawToRenderTarget(ef, rRP.m_pShaderResources, pObj, pTech, pTarg, 0, rRP.m_pRE);
        }
				targetNum = pTech->m_RTargets.Num();
        for (i=0; i<targetNum; ++i)
        {
          SHRenderTarget *pTarg = pTech->m_RTargets[i];
          if (pTarg->m_eOrder == eRO_PreDraw)
            rd->FX_DrawToRenderTarget(ef, rRP.m_pShaderResources, pObj, pTech, pTarg, 0, rRP.m_pRE);
        }
      }
    }
    rRP.m_pRootTechnique = pTech;

    // Skip z-pass if appropriate technique is not exist
    bool bGeneral = false;
    if (rRP.m_nBatchFilter & (FB_ZPREPASS | FB_Z | FB_GLOW | FB_MOTIONBLUR | FB_CUSTOM_RENDER |FB_RAIN|FB_FUR|FB_DEBUG/*|FB_DEFERRED_SKIN_DIFFUSION*/))
    {
      bGeneral = true;   
      if (rRP.m_nBatchFilter & (FB_ZPREPASS|FB_Z))
      {
        if (pTech->m_nTechnique[TTYPE_Z] > 0)
        {
          assert(pTech->m_nTechnique[TTYPE_Z] < (int)ef->m_HWTechniques.Num());
          pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_Z]];
        }
      }
      else
      if (rRP.m_nBatchFilter & FB_GLOW)
      {
        if (pTech->m_nTechnique[TTYPE_GLOWPASS] > 0)
        {
          assert(pTech->m_nTechnique[TTYPE_GLOWPASS] < (int)ef->m_HWTechniques.Num());
          pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_GLOWPASS]];
        }
      }
      else
      if (rRP.m_nBatchFilter & FB_MOTIONBLUR)
      {
        if (pTech->m_nTechnique[TTYPE_MOTIONBLURPASS] > 0)
        {
          assert(pTech->m_nTechnique[TTYPE_MOTIONBLURPASS] < (int)ef->m_HWTechniques.Num());
          pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_MOTIONBLURPASS]];
        }
      }
      else
      if (rRP.m_nBatchFilter & (FB_CUSTOM_RENDER))
      {
        if (pTech->m_nTechnique[TTYPE_CUSTOMRENDERPASS] > 0)
        {
          assert(pTech->m_nTechnique[TTYPE_CUSTOMRENDERPASS] < (int)ef->m_HWTechniques.Num());
          pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_CUSTOMRENDERPASS]];
        }
      }
      else
      if (rRP.m_nBatchFilter & FB_RAIN)
      {
        if (pTech->m_nTechnique[TTYPE_RAINPASS] > 0)
        {
          assert(pTech->m_nTechnique[TTYPE_RAINPASS] < (int)ef->m_HWTechniques.Num());
          pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_RAINPASS]];
        }
      }
      else
      if (rRP.m_nBatchFilter & FB_FUR)
      {
        if (pTech->m_nTechnique[TTYPE_FURPASS] > 0)
        {
          assert(pTech->m_nTechnique[TTYPE_FURPASS] < (int)ef->m_HWTechniques.Num());
          pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_FURPASS]];
        }
      }
      else
      if (rRP.m_nBatchFilter & FB_DEBUG)
      {
        if (pTech->m_nTechnique[TTYPE_DEBUG] > 0)
        {
          assert(pTech->m_nTechnique[TTYPE_DEBUG] < (int)ef->m_HWTechniques.Num());
          pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_DEBUG]];
        }
      }
			/*else
			if (rRP.m_nBatchFilter & FB_DEFERRED_SKIN_DIFFUSION)
			{
				if (pTech->m_nTechnique[TTYPE_SKINDIFFUSIONPASS] > 0)
				{
					assert(pTech->m_nTechnique[TTYPE_SKINDIFFUSIONPASS] < (int)ef->m_HWTechniques.Num());
					pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_SKINDIFFUSIONPASS]];
				}
			}*/
    }
		if (!(rRP.m_nBatchFilter & FB_Z) && CV_r_debugrendermode)
		{
			if (CV_r_debugrendermode & 1)
				rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG0];
			if (CV_r_debugrendermode & 2)
				rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG1];
			if (CV_r_debugrendermode & 4)
				rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG2];
			if (CV_r_debugrendermode & 8)
				rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG3];
		}

    if (!rd->FX_SetResourcesState())
    {
      if (rRP.m_pReplacementShader)
        rd->FX_SetupMultiLayers(false); 
      return;
    }

    if (rRP.m_nBatchFilter & FB_SCATTER)
    {
      if (pTech->m_nTechnique[TTYPE_SCATTERPASS] > 0)
      {
        assert(pTech->m_nTechnique[TTYPE_SCATTERPASS] < (int)ef->m_HWTechniques.Num());
        pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_SCATTERPASS]];

        bGeneral = true;

        //overwrite cull mode
        if (rTI.m_PersFlags2 & RBPF2_SCATTERPASS)
          rd->D3DSetCull(eCULL_Back);
        else
          rd->D3DSetCull(eCULL_Front);

        rRP.m_FlagsPerFlush |= RBSI_NOCULL;
      }
      else
      {
        //TOFIX: uncomment for having proper scattering accumulation
        //////////////////////////////////////////////////////////////////////////
        //test for occluders(skeleton) rendering
        //////////////////////////////////////////////////////////////////////////
        /*if (pTech->m_nTechnique[TTYPE_Z] > 0)
        {
          assert(pTech->m_nTechnique[TTYPE_Z] < (int)ef->m_HWTechniques.Num());
          pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_Z]];

          bGeneral = true;
        }*/
        //////////////////////////////////////////////////////////////////////////

        if (!(rTI.m_PersFlags2 & RBPF2_SCATTERPASS))
          return;
      }
    }
		const int objFlags = rRP.m_ObjFlags;
    if (objFlags & (FOB_BENDED | FOB_SOFT_PARTICLE | FOB_OCEAN_PARTICLE | FOB_DECAL_TEXGEN_2D | FOB_DECAL_TEXGEN_3D | FOB_VEGETATION | FOB_NEAREST|FOB_SHADERLOD0|FOB_CAMERA_SPACE))
    {
      if (!(rTI.m_PersFlags & RBPF_MAKESPRITE) && (objFlags & FOB_BENDED))
        rRP.m_FlagsShader_MDV |= MDV_BENDING;
      if (objFlags & FOB_VEGETATION)
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VEGETATION];                

      if (CV_r_usesoftparticles && SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID]==1)
      {
        if (objFlags & FOB_SOFT_PARTICLE)
          rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SOFT_PARTICLE];
        if (objFlags & FOB_OCEAN_PARTICLE)
          rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_OCEAN_PARTICLE];
      }

      assert( ( FOB_DECAL_TEXGEN_2D | FOB_DECAL_TEXGEN_3D ) != ( objFlags & ( FOB_DECAL_TEXGEN_2D | FOB_DECAL_TEXGEN_3D ) ) );
      if(objFlags & FOB_DECAL_TEXGEN_2D)
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_2D];
      if(objFlags & FOB_DECAL_TEXGEN_3D)
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_3D];
      if(objFlags & FOB_NEAREST)
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_NEAREST];
#ifdef ALLOW_CAMERA_SPACE
      if(objFlags & FOB_CAMERA_SPACE)
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CAMERA_SPACE];
#endif

      if (objFlags & (FOB_SHADERLOD0) && CV_r_ShaderLod )
      {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SHADER_LOD];
        rRP.m_FlagsShader_MDV &= ~MDV_BENDING; // do this in 3dengine instead
      }
    }    
    if (rRP.m_RIs.Num()<=1 && !(objFlags & FOB_TRANS_MASK))
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_OBJ_IDENTITY];
		if (!(rTI.m_PersFlags2 & RBPF2_NOSHADERFOG) && rTI.m_FS.m_bEnable && !(objFlags & FOB_NO_FOG) || !(rTI.m_PersFlags2&RBPF2_ALLOW_DEFERREDSHADING))
    {
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_FOG];

      if (rd->UseSkyLightBasedFog())
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SKYLIGHT_BASED_FOG];
    }

    rRP.m_pCurTechnique = pTech;




    if ((rRP.m_nBatchFilter & (FB_DETAIL | FB_CAUSTICS | FB_MULTILAYERS | FB_LAYER_EFFECT | FB_DEBUG)) && !rRP.m_pReplacementShader)
    {
      if (rRP.m_nBatchFilter & FB_DETAIL)
        rd->FX_DrawDetailOverlayPasses();

			if (rRP.m_nBatchFilter & FB_LAYER_EFFECT)
				rd->FX_DrawEffectLayerPasses();

      if (rRP.m_nBatchFilter & FB_MULTILAYERS)
        rd->FX_DrawMultiLayers();

      if (rRP.m_nBatchFilter & FB_CAUSTICS)
        rd->FX_DrawCausticsPasses();

      if (rRP.m_nBatchFilter & FB_DEBUG)
        rd->FX_DrawDebugPasses();
    }
    else
      rd->FX_DrawTechnique(ef, pTech, bGeneral, true);

//if( !bSupportsFullDeferred  )
//      rd->FX_DrawTechnique(ef, pTech, bGeneral, true);


//////////////////////////////////////////////////////////////////////////
    //depth pass for scattered objects
    if ((rRP.m_nBatchFilter & FB_SCATTER) && !(rTI.m_PersFlags2 & RBPF2_SCATTERPASS))
    {
      pTech = ef->mfGetStartTechnique(rRP.m_nShaderTechnique);
      if (pTech->m_nTechnique[TTYPE_SCATTERPASS] > 0)
      {
        //assert(pTech->m_nTechnique[TTYPE_SCATTERPASS] < (int)ef->m_HWTechniques.Num());
        pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_SCATTERPASS]];

        bGeneral = true;

        //FB_SCATTER should not be processed during z-pass
        //assert(rRP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_ZPASS);
        rTI.m_PersFlags |= RBPF_ZPASS;

        rd->D3DSetCull(eCULL_Front);

        rRP.m_FlagsPerFlush |= RBSI_NOCULL;

        rd->FX_DrawTechnique(ef, pTech, bGeneral, true);
        rTI.m_PersFlags &= ~RBPF_ZPASS;
      }
    }

//////////////////////////////////////////////////////////////////////////

  }//pTech
  else
  if (ef->m_eSHDType == eSHDT_CustomDraw)
    rd->FX_DrawTechnique(ef, 0, true, true);

  if (rRP.m_pReplacementShader)
  {
    rd->FX_SetupMultiLayers(false); 
  }

  if (ef->m_Flags & EF_LOCALCONSTANTS)
    rRP.m_PrevLMask = -1;

  PROFILE_SHADER_END

#ifdef DO_RENDERLOG
  if (rd->m_LogFile)
  {
    if (CV_r_log == 4 && rRP.m_DynLMask)
    {
      uint32 nAffectMask = rRP.m_nCurLightGroup < 0 ? -1 : (0xf << (rRP.m_nCurLightGroup*4));
      uint32 nMask = (rRP.m_DynLMask & nAffectMask);
      for (uint32 n=0; n<rRP.m_DLights[rRP.m_nProcessThreadID][SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID]-1].Num(); n++)
      {
        CDLight *dl = &rRP.m_DLights[rRP.m_nProcessThreadID][SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID]-1][n];
        if (nMask & (1<<n))
          rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "   Light %d (\"%s\")\n", n, dl->m_sName ? dl->m_sName : "<Unknown>");
      }
    }

    char *str = "Flush General";
    if (rRP.m_pCurTechnique)
			rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "%s: '%s.%s', Id:%d, ResId:%d, Obj:%d, Cp: %d, VF:%d\n", str, ef->GetName(), rRP.m_pCurTechnique?rRP.m_pCurTechnique->m_Name.c_str():"", ef->GetID(), rRP.m_pShaderResources ? rRP.m_pShaderResources->m_Id : -1, rRP.m_pCurObject->m_Id, rRP.m_ClipPlaneEnabled, ef->m_eVertexFormat);
    else
      rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "%s: '%s', Id:%d, ResId:%d, Obj:%d, Cp: %d, VF:%d\n", str, ef->GetName(), ef->GetID(), rRP.m_pShaderResources ? rRP.m_pShaderResources->m_Id : -1, rRP.m_pCurObject->m_Id, rRP.m_ClipPlaneEnabled, ef->m_eVertexFormat);
    if (rRP.m_ObjFlags & FOB_SELECTED)
    {
      if (rRP.m_MaterialState & GS_ALPHATEST_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), LM: %d, (AT) (Selected)\n", rRP.m_pCurObject->m_II.m_Matrix(0,3), rRP.m_pCurObject->m_II.m_Matrix(1,3), rRP.m_pCurObject->m_II.m_Matrix(2,3), rRP.m_pCurObject->m_ObjFlags, rRP.m_DynLMask);
      else
      if (rRP.m_MaterialState & GS_BLEND_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AB), LM: %d (Dist: %.3f) (Selected)\n", rRP.m_pCurObject->m_II.m_Matrix(0,3), rRP.m_pCurObject->m_II.m_Matrix(1,3), rRP.m_pCurObject->m_II.m_Matrix(2,3), rRP.m_pCurObject->m_ObjFlags, rRP.m_DynLMask, rRP.m_pCurObject->m_fDistance);
      else
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), RE: 0x%x, LM: 0x%x (Selected)\n", rRP.m_pCurObject->m_II.m_Matrix(0,3), rRP.m_pCurObject->m_II.m_Matrix(1,3), rRP.m_pCurObject->m_II.m_Matrix(2,3), rRP.m_pCurObject->m_ObjFlags, rRP.m_pRE, rRP.m_DynLMask);
    }
    else
    {
      if (rRP.m_MaterialState & GS_ALPHATEST_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AT), Inst: %d, RE: 0x%x, LM: %d (Dist: %.3f)\n", rRP.m_pCurObject->m_II.m_Matrix(0,3), rRP.m_pCurObject->m_II.m_Matrix(1,3), rRP.m_pCurObject->m_II.m_Matrix(2,3), rRP.m_pCurObject->m_ObjFlags, rRP.m_RIs.Num(), rRP.m_pRE, rRP.m_DynLMask, rRP.m_pCurObject->m_fDistance);
      else
      if (rRP.m_MaterialState & GS_BLEND_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AB), Inst: %d, RE: 0x%x, LM: %d (Dist: %.3f)\n", rRP.m_pCurObject->m_II.m_Matrix(0,3), rRP.m_pCurObject->m_II.m_Matrix(1,3), rRP.m_pCurObject->m_II.m_Matrix(2,3), rRP.m_pCurObject->m_ObjFlags, rRP.m_RIs.Num(), rRP.m_pRE, rRP.m_DynLMask, rRP.m_pCurObject->m_fDistance);
      else
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), Inst: %d, RE: 0x%x, LM: 0x%x\n", rRP.m_pCurObject->m_II.m_Matrix(0,3), rRP.m_pCurObject->m_II.m_Matrix(1,3), rRP.m_pCurObject->m_II.m_Matrix(2,3), rRP.m_pCurObject->m_ObjFlags, rRP.m_RIs.Num(), rRP.m_pRE, rRP.m_DynLMask);
    }
  }
#endif
}

void CD3D9Renderer::FX_FlushShader_ShadowGen()
{
  CD3D9Renderer *const __restrict rd = gcpRendD3D;
  SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
  if (!rRP.m_pRE && !rRP.m_RendNumVerts)
    return;

  CShader *ef = rRP.m_pShader;
  if (!ef)
    return;

  //leave out for ps3 since it costs too much
#ifndef PS3
  if (!rRP.m_sExcludeShader.empty())
  {
    char nm[1024];
    strcpy(nm, ef->GetName());
    strlwr(nm);
    if (strstr(rRP.m_sExcludeShader.c_str(), nm))
      return;
  }
#endif

  SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];
  assert(rTI.m_PersFlags & RBPF_SHADOWGEN);
  assert(!(rTI.m_PersFlags & RBPF_MAKESPRITE));

  //leave out for ps3 since it costs too much
#ifdef DO_RENDERLOG
  if (rd->m_LogFile)
  {
    if (CV_r_log == 3)
      rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "\n\n.. Start %s flush: '%s' ..\n", "ShadowGen", ef->GetName());
    if (CV_r_log >= 3)
      rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "\n");
  }
#endif

  SPipeStat& rPS = rRP.m_PS[rRP.m_nProcessThreadID];
  ++rPS.m_NumRendBatches;
  const uint32 objInstNum = rRP.m_RIs.Num();
  const uint32 numRendInstances = rPS.m_NumRendInstances;
  //implement branch-free using masks:
  //	rPS.m_NumRendInstances = objInstNum?(numRendInstancesIncr+objInstNum) : (numRendInstances+1);
  const uint32 numRendInstancesMask = (uint32)(((-(int)objInstNum)) >> 31);
  rPS.m_NumRendInstances = (numRendInstances+objInstNum) & ~numRendInstancesMask | (numRendInstances+1) & numRendInstancesMask;

  CRenderObject *pObj = rRP.m_pCurObject;

  PROFILE_SHADER_START

//#if !defined(PS3) && !defined(XENON)
  sDetectInstancing(ef, pObj);
//#endif

  // Techniques draw cycle...
  SShaderTechnique *__restrict pTech = ef->mfGetStartTechnique(rRP.m_nShaderTechnique);
  assert(pTech);
  if (!pTech || pTech->m_nTechnique[TTYPE_SHADOWGEN] < 0)
    return;

  rRP.m_pRootTechnique = pTech;

  pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_SHADOWGEN]];





  if (ef->m_eSHDType == eSHDT_Terrain)
  {
    if (rTI.m_PersFlags2 & RBPF2_VSM)
    {
      rd->D3DSetCull(eCULL_None); 
      rd->m_RP.m_FlagsPerFlush |= RBSI_NOCULL;
    }
    else
    if (rd->m_RP.m_pCurShadowFrustum->m_Flags & DLF_DIRECTIONAL)
    {
      rd->D3DSetCull(eCULL_None); 
      rd->m_RP.m_FlagsPerFlush |= RBSI_NOCULL;
    }
    else 
    {
      //Flipped matrix for point light sources
      //front faces culling by default for terrain
      rd->D3DSetCull(eCULL_Back); 
      rd->m_RP.m_FlagsPerFlush |= RBSI_NOCULL;
      //reset slope bias here as well
    }
  }

  // RSMs
  if (rd->m_RP.m_pCurShadowFrustum->m_Flags & DLF_REFLECTIVE_SHADOWMAP)
  {
    rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_SAMPLE4 ];
    rd->D3DSetCull(eCULL_Back); 
    rd->m_RP.m_FlagsPerFlush |= RBSI_NOCULL;

		const int objFlags = rRP.m_ObjFlags;
		if(objFlags & FOB_DECAL_TEXGEN_2D)
			rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_2D];
		if(objFlags & FOB_DECAL_TEXGEN_3D)
			rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_3D];
  }
	else if (rTI.m_PersFlags2 & (RBPF2_VSM | RBPF2_DRAWTOCUBE | RBPF2_DISABLECOLORWRITES))
  {
    if (rTI.m_PersFlags2 & RBPF2_DISABLECOLORWRITES)
      rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];
    if (rTI.m_PersFlags2 & RBPF2_VSM)
      rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VARIANCE_SM];
    if (rTI.m_PersFlags2 & RBPF2_DRAWTOCUBE)
      rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
  }

  //per-object bias for Shadow Generation
  rd->m_cEF.m_TempVecs[1][0] = 0.0f;
  if(!(rTI.m_PersFlags2 & RBPF2_VSM))
  {
    if (rd->m_RP.m_pShaderResources)
    {
      if (rd->m_RP.m_pShaderResources->m_ResFlags & MTL_FLAG_2SIDED)
      {
        //handle terrain self-shadowing and two-sided geom
        rd->m_cEF.m_TempVecs[1][0] = rTI.m_vFrustumInfo.w;
      }

    }
    //don't make per-object bias for global VSM
    //if (rRP.m_pShader->m_eSHDType == eSHDT_Terrain /*&& m_RP.m_vFrustumInfo.x > 100.0f*/) //check for sun && terrain shadows
    //{
    //  rd->m_cEF.m_TempVecs[1][0] = -(rRP.m_vFrustumInfo.w);
    //}
  }

  if (!rd->FX_SetResourcesState())
    return;

  //rd->EF_ApplyQuality();

  const int objFlags = rRP.m_ObjFlags;

  if (objFlags & (FOB_BENDED | FOB_VEGETATION | FOB_NEAREST|FOB_SHADERLOD0))
  {
		const bool bRSMs = rd->m_RP.m_pCurShadowFrustum && rd->m_RP.m_pCurShadowFrustum->bReflectiveShadowMap;

    if (objFlags & FOB_BENDED && !bRSMs)
      rRP.m_FlagsShader_MDV |= MDV_BENDING;
		if (objFlags & FOB_VEGETATION)
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VEGETATION];                
    if (objFlags & FOB_NEAREST)
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_NEAREST];
    if (rd->m_RP.m_ObjFlags  & (FOB_SHADERLOD0) && CV_r_ShaderLod )
    {
      rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SHADER_LOD];
      rRP.m_FlagsShader_MDV &= ~MDV_BENDING;
    }
  }    
  if (rRP.m_RIs.Num()<=1 && !(objFlags & FOB_TRANS_MASK))
    rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_OBJ_IDENTITY];


	if ((rRP.m_pCurObject->m_ObjFlags & FOB_SHADOW_DISSOLVE ) && (rRP.m_pCurObject->m_nMaterialLayers & MTL_LAYER_BLEND_CLOAK))
	{
		if( (rRP.m_pCurObject->m_nMaterialLayers & MTL_LAYER_BLEND_CLOAK) == MTL_LAYER_BLEND_CLOAK) //fully blended, dont render
			return;
			
		rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_DISSOLVE ];
	}


  rRP.m_pCurTechnique = pTech;
  rd->FX_DrawTechnique(ef, pTech, true, true);

  if (ef->m_Flags & EF_LOCALCONSTANTS)
    rRP.m_PrevLMask = -1;

  PROFILE_SHADER_END

#ifdef DO_RENDERLOG
  if (rd->m_LogFile)
  {
    char *str = "Flush ShadowGen";
    if (rRP.m_pCurTechnique)
			rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "%s: '%s.%s', Id:%d, ResId:%d, Obj:%d, Cp: %d, VF:%d\n", str, ef->GetName(), rRP.m_pCurTechnique?rRP.m_pCurTechnique->m_Name.c_str():"", ef->GetID(), rRP.m_pShaderResources ? rRP.m_pShaderResources->m_Id : -1, rRP.m_pCurObject->m_Id, rRP.m_ClipPlaneEnabled, ef->m_eVertexFormat);
    if (rRP.m_ObjFlags & FOB_SELECTED)
    {
      if (rRP.m_MaterialState & GS_ALPHATEST_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), LM: %d, (AT) (Selected)\n", rRP.m_pCurObject->m_II.m_Matrix(0,3), rRP.m_pCurObject->m_II.m_Matrix(1,3), rRP.m_pCurObject->m_II.m_Matrix(2,3), rRP.m_pCurObject->m_ObjFlags, rRP.m_DynLMask);
      else
      if (rRP.m_MaterialState & GS_BLEND_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AB), LM: %d (Dist: %.3f) (Selected)\n", rRP.m_pCurObject->m_II.m_Matrix(0,3), rRP.m_pCurObject->m_II.m_Matrix(1,3), rRP.m_pCurObject->m_II.m_Matrix(2,3), rRP.m_pCurObject->m_ObjFlags, rRP.m_DynLMask, rRP.m_pCurObject->m_fDistance);
      else
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), RE: 0x%x, LM: 0x%x (Selected)\n", rRP.m_pCurObject->m_II.m_Matrix(0,3), rRP.m_pCurObject->m_II.m_Matrix(1,3), rRP.m_pCurObject->m_II.m_Matrix(2,3), rRP.m_pCurObject->m_ObjFlags, rRP.m_pRE, rRP.m_DynLMask);
    }
    else
    {
      if (rRP.m_MaterialState & GS_ALPHATEST_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AT), Inst: %d, RE: 0x%x, LM: %d (Dist: %.3f)\n", rRP.m_pCurObject->m_II.m_Matrix(0,3), rRP.m_pCurObject->m_II.m_Matrix(1,3), rRP.m_pCurObject->m_II.m_Matrix(2,3), rRP.m_pCurObject->m_ObjFlags, rRP.m_RIs.Num(), rRP.m_pRE, rRP.m_DynLMask, rRP.m_pCurObject->m_fDistance);
      else
      if (rRP.m_MaterialState & GS_BLEND_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AB), Inst: %d, RE: 0x%x, LM: %d (Dist: %.3f)\n", rRP.m_pCurObject->m_II.m_Matrix(0,3), rRP.m_pCurObject->m_II.m_Matrix(1,3), rRP.m_pCurObject->m_II.m_Matrix(2,3), rRP.m_pCurObject->m_ObjFlags, rRP.m_RIs.Num(), rRP.m_pRE, rRP.m_DynLMask, rRP.m_pCurObject->m_fDistance);
      else
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), Inst: %d, RE: 0x%x, LM: 0x%x\n", rRP.m_pCurObject->m_II.m_Matrix(0,3), rRP.m_pCurObject->m_II.m_Matrix(1,3), rRP.m_pCurObject->m_II.m_Matrix(2,3), rRP.m_pCurObject->m_ObjFlags, rRP.m_RIs.Num(), rRP.m_pRE, rRP.m_DynLMask);
    }
  }
#endif
}

void CD3D9Renderer::FX_FlushShader_ZPass()
{
  CD3D9Renderer *const __restrict rd = gcpRendD3D;
  SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
  if (!rRP.m_pRE && !rRP.m_RendNumVerts)
    return;

  CShader *ef = rRP.m_pShader;
  if (!ef)
    return;

  //leave out for ps3 since it costs too much
#ifndef PS3
  if (!rRP.m_sExcludeShader.empty())  
  {
    char nm[1024];
    strcpy(nm, ef->GetName());
    strlwr(nm);
    if (strstr(rRP.m_sExcludeShader.c_str(), nm))
      return;
  }
#endif

  SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];
  assert(!(rTI.m_PersFlags & RBPF_SHADOWGEN));
  assert(rRP.m_nBatchFilter & (FB_ZPREPASS|FB_Z));
  assert(!(rTI.m_PersFlags & RBPF_MAKESPRITE));

#ifdef DO_RENDERLOG
  if (rd->m_LogFile)
  {
    if (CV_r_log == 3)
      rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "\n\n.. Start %s flush: '%s' ..\n", "ZPass", ef->GetName());
    else
    if (CV_r_log >= 3)
      rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "\n");
  }
#endif

  SPipeStat& rPS = rRP.m_PS[rRP.m_nProcessThreadID];
  ++rPS.m_NumRendBatches;
  const uint32 objInstNum = rRP.m_RIs.Num();
  const uint32 numRendInstances = rPS.m_NumRendInstances;
  //implement branch-free using masks:
  //	rPS.m_NumRendInstances = objInstNum?(numRendInstancesIncr+objInstNum) : (numRendInstances+1);
  const uint32 numRendInstancesMask = (uint32)(((-(int)objInstNum)) >> 31);
  rPS.m_NumRendInstances = (numRendInstances+objInstNum) & ~numRendInstancesMask | (numRendInstances+1) & numRendInstancesMask;

  PROFILE_SHADER_START

//#if !defined(PS3) && !defined(XENON)
  sDetectInstancing(ef, rRP.m_pCurObject);
//#endif

  // Techniques draw cycle...
  SShaderTechnique *__restrict pTech = ef->mfGetStartTechnique(rRP.m_nShaderTechnique);
  if (!pTech || pTech->m_nTechnique[TTYPE_Z] < 0)
    return;

  rRP.m_pRootTechnique = pTech;

  // Skip z-pass if appropriate technique is not exist
  assert(pTech->m_nTechnique[TTYPE_Z] < (int)ef->m_HWTechniques.Num());
  pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_Z]];

  if (!rd->FX_SetResourcesState())
    return;





  const int objFlags = rRP.m_ObjFlags;
  if (objFlags & (FOB_BENDED | FOB_VEGETATION | FOB_NEAREST | FOB_DECAL_TEXGEN_2D | FOB_DECAL_TEXGEN_3D | FOB_DISSOLVE | FOB_SHADERLOD0))
  {
    if (objFlags & FOB_BENDED)
      rRP.m_FlagsShader_MDV |= MDV_BENDING;
    if (objFlags & FOB_VEGETATION)
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VEGETATION];                

    assert( ( FOB_DECAL_TEXGEN_2D | FOB_DECAL_TEXGEN_3D ) != ( objFlags & ( FOB_DECAL_TEXGEN_2D | FOB_DECAL_TEXGEN_3D ) ) );
    if(objFlags & FOB_DECAL_TEXGEN_2D)
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_2D];
    if(objFlags & FOB_DECAL_TEXGEN_3D)
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_3D];

    if(objFlags & FOB_NEAREST)
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_NEAREST];
    if (objFlags & (FOB_DISSOLVE))
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DISSOLVE];

    if (objFlags & (FOB_SHADERLOD0) && CV_r_ShaderLod )
    {
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SHADER_LOD];
      rRP.m_FlagsShader_MDV &= ~MDV_BENDING; // do this in 3dengine instead
    }
  }

  rd->FX_SetRenderObjDefaultVisAreaStencilRef( rRP.m_pCurObject );

  if ((ef->m_Flags2 & EF2_DEFERBACKLIGHTING) && !(objFlags & FOB_INVISAREA) && !(rTI.m_PersFlags & RBPF_SHADOWGEN))
  {
    //commit stencil state
    rd->EF_SetStencilState(
      STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
      STENCOP_FAIL(FSS_STENCOP_KEEP) |
      STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
      STENCOP_PASS(FSS_STENCOP_REPLACE),
      0x80, 0xFF, 0xFF
      );
    rTI.m_PersFlags2 |=	RBPF2_LIGHTSTENCILCULL;
    //rRP.m_FlagsPerFlush |= RBSI_ENABLESTENCIL;
  }

  if (rRP.m_RIs.Num()<=1 && !(objFlags & FOB_TRANS_MASK))
    rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_OBJ_IDENTITY];
  if (!(rTI.m_PersFlags2 & RBPF2_NOSHADERFOG) && rTI.m_FS.m_bEnable && !(objFlags & FOB_NO_FOG))
  {
    rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_FOG];

    if (rd->UseSkyLightBasedFog())
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SKYLIGHT_BASED_FOG];
  }

  rRP.m_pCurTechnique = pTech;
  rd->FX_DrawTechnique(ef, pTech, true, true);

  rTI.m_PersFlags2 &=	~RBPF2_LIGHTSTENCILCULL;

  PROFILE_SHADER_END

#ifdef DO_RENDERLOG
  if (rd->m_LogFile && rRP.m_pCurObject)
  {
    CRenderObject *pObj = rRP.m_pCurObject;
    char *str = "Flush ZPass";
    rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "%s: '%s.%s', Id:%d, ResId:%d, Obj:%d, Cp: %d, VF:%d\n", str, ef->GetName(), pTech->m_Name.c_str(), ef->GetID(), rRP.m_pShaderResources ? rRP.m_pShaderResources->m_Id : -1, pObj->m_Id, rRP.m_ClipPlaneEnabled, ef->m_eVertexFormat);
    if (rRP.m_ObjFlags & FOB_SELECTED)
    {
      if (rRP.m_MaterialState & GS_ALPHATEST_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), LM: %d, (AT) (Selected)\n", pObj->m_II.m_Matrix(0,3), pObj->m_II.m_Matrix(1,3), pObj->m_II.m_Matrix(2,3), pObj->m_ObjFlags, rRP.m_DynLMask);
      else
      if (rRP.m_MaterialState & GS_BLEND_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AB), LM: %d (Dist: %.3f) (Selected)\n", pObj->m_II.m_Matrix(0,3), pObj->m_II.m_Matrix(1,3), pObj->m_II.m_Matrix(2,3), pObj->m_ObjFlags, rRP.m_DynLMask, pObj->m_fDistance);
      else
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), RE: 0x%x, LM: 0x%x (Selected)\n", pObj->m_II.m_Matrix(0,3), pObj->m_II.m_Matrix(1,3), pObj->m_II.m_Matrix(2,3), pObj->m_ObjFlags, rRP.m_pRE, rRP.m_DynLMask);
    }
    else
    {
      if (rRP.m_MaterialState & GS_ALPHATEST_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AT), Inst: %d, RE: 0x%x, LM: %d (Dist: %.3f)\n", pObj->m_II.m_Matrix(0,3), pObj->m_II.m_Matrix(1,3), pObj->m_II.m_Matrix(2,3), pObj->m_ObjFlags, rRP.m_RIs.Num(), rRP.m_pRE, rRP.m_DynLMask, pObj->m_fDistance);
      else
      if (rRP.m_MaterialState & GS_BLEND_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AB), Inst: %d, RE: 0x%x, LM: %d (Dist: %.3f)\n", pObj->m_II.m_Matrix(0,3), pObj->m_II.m_Matrix(1,3), pObj->m_II.m_Matrix(2,3), pObj->m_ObjFlags, rRP.m_RIs.Num(), rRP.m_pRE, rRP.m_DynLMask, pObj->m_fDistance);
      else
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), Inst: %d, RE: 0x%x, LM: 0x%x\n", pObj->m_II.m_Matrix(0,3), pObj->m_II.m_Matrix(1,3), pObj->m_II.m_Matrix(2,3), pObj->m_ObjFlags, rRP.m_RIs.Num(), rRP.m_pRE, rRP.m_DynLMask);
    }
    if (rRP.m_pRE && rRP.m_pRE->mfGetType() == eDATA_Mesh)
    {
      CREMesh *pRE = (CREMesh *)rRP.m_pRE;
      CRenderMesh2 *pRM = pRE->m_pRenderMesh;
      if (pRM && pRM->m_Chunks.size() && pRM->m_sSource)
      {
        int nChunk = -1;
        for (uint32 i=0; i<pRM->m_Chunks.size(); i++)
        {
          CRenderChunk *pCH = &pRM->m_Chunks[i];
          if (pCH->pRE == pRE)
          {
            nChunk = i;
            break;
          }
        }
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  Mesh: %s (Chunk: %d)\n", pRM->m_sSource, nChunk);
      }
    }
  }
#endif
}

void CD3D9Renderer::FX_FlushShader_ShadowPass()
{
  CD3D9Renderer *const __restrict rd = gcpRendD3D;
  SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
  if (!rRP.m_pRE && !rRP.m_RendNumVerts)
    return;

  CShader *pShader = rRP.m_pShader;

  if (!pShader || !pShader->m_HWTechniques.Num())
    return;
  //  if (rd->m_RP.m_nPassGroupID == EFSLIST_TRANSP_ID)
  {
    if (!rd->FX_SetResourcesState())
      return;
  }
  SShaderTechnique *pTech = pShader->mfGetStartTechnique(rd->m_RP.m_nShaderTechnique);
  if (!pTech || pTech->m_nTechnique[TTYPE_SHADOWPASS] < 0)
    return;

  rRP.m_pRootTechnique = pTech;
  assert(pTech->m_nTechnique[TTYPE_SHADOWPASS] < (int)pShader->m_HWTechniques.Num());
  pTech = pShader->m_HWTechniques[pTech->m_nTechnique[TTYPE_SHADOWPASS]];
  SShaderTechnique *pBaseTech = pTech;



  if (rRP.m_ObjFlags & FOB_SELECTED)
    FX_SelectTechnique(pShader, pTech);

  rd->FX_ApplyShadowQuality();

  if (rRP.m_TI[rRP.m_nProcessThreadID].m_PersFlags2 & (RBPF2_VSM | RBPF2_DRAWTOCUBE | RBPF2_DISABLECOLORWRITES))
  {
    if (rRP.m_TI[rRP.m_nProcessThreadID].m_PersFlags2 & RBPF2_DISABLECOLORWRITES)
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];
    if (rd->m_RP.m_TI[rRP.m_nProcessThreadID].m_PersFlags2 & RBPF2_VSM)
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VARIANCE_SM];
    if (rRP.m_TI[rRP.m_nProcessThreadID].m_PersFlags2 & RBPF2_DRAWTOCUBE)
      rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
  }

  if (rRP.m_ObjFlags & FOB_VEGETATION)
  {
    rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VEGETATION];
    if (!(rRP.m_TI[rRP.m_nProcessThreadID].m_PersFlags & RBPF_MAKESPRITE) && (rRP.m_ObjFlags & FOB_BENDED))
      rRP.m_FlagsShader_MDV |= MDV_BENDING;
  }
  if (pShader->m_eShaderType == eST_Particle)
    rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_PARTICLE];

  if (rRP.m_pShaderResources)
  {
    if (rRP.m_pShaderResources->m_ResFlags & MTL_FLAG_2SIDED)
    {
      rd->D3DSetCull(eCULL_None);
      rRP.m_FlagsPerFlush |= RBSI_NOCULL;      
    }
    else
    if (rRP.m_pShader->m_eCull != -1)
    {
      rd->D3DSetCull(rRP.m_pShader->m_eCull);
      rRP.m_FlagsPerFlush |= RBSI_NOCULL;
    }
  }

//#if !defined(PS3) && !defined(XENON)
  sDetectInstancing(pShader, rRP.m_pCurObject);
//#endif

  PROFILE_SHADER_START

#ifdef DO_RENDERLOG
  if (rd->m_LogFile && CV_r_log >= 3)
    rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "\n");
#endif

  rd->FX_DrawShadowPasses(pShader, pTech, rRP.m_nCurLightChan);

  PROFILE_SHADER_END
#ifdef DO_RENDERLOG
  if (rd->m_LogFile)
  {
    if (CV_r_log == 4)
    {
      uint32 nAffectMask = rd->m_RP.m_nCurLightGroup < 0 ? -1 : (0xf << (rd->m_RP.m_nCurLightGroup*4));
      uint32 nMask = (nAffectMask);
      for (uint32 n=0; n<rd->m_RP.m_DLights[rd->m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID]-1].Num(); n++)
      {
        CDLight *dl = &rd->m_RP.m_DLights[rd->m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID]-1][n];
        if (nMask & (1<<n))
          rd->Logv(SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID], "   Light %d (\"%s\")\n", n, dl->m_sName ? dl->m_sName : "<Unknown>");
      }
    }

    char *str = "FlushHW";
    if (pBaseTech)
      rd->Logv(SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID], "%s: '%s.%s', Id:%d, ResId:%d, Obj:%d, Cp: %d, VF:%d\n", str, pShader->GetName(), pBaseTech->m_Name.c_str(), pShader->GetID(), rd->m_RP.m_pShaderResources ? rd->m_RP.m_pShaderResources->m_Id : -1, rd->m_RP.m_pCurObject->m_Id, rd->m_RP.m_ClipPlaneEnabled, pShader->m_eVertexFormat);
    else
      rd->Logv(SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID], "%s: '%s', Id:%d, ResId:%d, Obj:%d, Cp: %d, VF:%d\n", str, pShader->GetName(), pShader->GetID(), rd->m_RP.m_pShaderResources ? rd->m_RP.m_pShaderResources->m_Id : -1, rd->m_RP.m_pCurObject->m_Id, rd->m_RP.m_ClipPlaneEnabled, pShader->m_eVertexFormat);
    if (rd->m_RP.m_ObjFlags & FOB_SELECTED)
    {
      if (rd->m_RP.m_MaterialState & GS_ALPHATEST_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), LM: %d, (AT) (Selected)\n", rd->m_RP.m_pCurObject->m_II.m_Matrix(0,3), rd->m_RP.m_pCurObject->m_II.m_Matrix(1,3), rd->m_RP.m_pCurObject->m_II.m_Matrix(2,3), rd->m_RP.m_pCurObject->m_ObjFlags, rd->m_RP.m_DynLMask);
      else
      if (rd->m_RP.m_MaterialState & GS_BLEND_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AB), LM: %d (Dist: %.3f) (Selected)\n", rd->m_RP.m_pCurObject->m_II.m_Matrix(0,3), rd->m_RP.m_pCurObject->m_II.m_Matrix(1,3), rd->m_RP.m_pCurObject->m_II.m_Matrix(2,3), rd->m_RP.m_pCurObject->m_ObjFlags, rd->m_RP.m_DynLMask, rd->m_RP.m_pCurObject->m_fDistance);
      else
        rd->Logv(SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), RE: 0x%x, LM: 0x%x (Selected)\n", rd->m_RP.m_pCurObject->m_II.m_Matrix(0,3), rd->m_RP.m_pCurObject->m_II.m_Matrix(1,3), rd->m_RP.m_pCurObject->m_II.m_Matrix(2,3), rd->m_RP.m_pCurObject->m_ObjFlags, rd->m_RP.m_pRE, rd->m_RP.m_DynLMask);
    }
    else
    {
      if (rd->m_RP.m_MaterialState & GS_ALPHATEST_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AB), Inst: %d, LM: %d (Dist: %.3f)\n", rd->m_RP.m_pCurObject->m_II.m_Matrix(0,3), rd->m_RP.m_pCurObject->m_II.m_Matrix(1,3), rd->m_RP.m_pCurObject->m_II.m_Matrix(2,3), rd->m_RP.m_pCurObject->m_ObjFlags, rd->m_RP.m_RIs.Num(), rd->m_RP.m_DynLMask, rd->m_RP.m_pCurObject->m_fDistance);
      else
      if (rd->m_RP.m_MaterialState & GS_BLEND_MASK)
        rd->Logv(SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AB), Inst: %d, LM: %d (Dist: %.3f)\n", rd->m_RP.m_pCurObject->m_II.m_Matrix(0,3), rd->m_RP.m_pCurObject->m_II.m_Matrix(1,3), rd->m_RP.m_pCurObject->m_II.m_Matrix(2,3), rd->m_RP.m_pCurObject->m_ObjFlags, rd->m_RP.m_RIs.Num(), rd->m_RP.m_DynLMask, rd->m_RP.m_pCurObject->m_fDistance);
      else
        rd->Logv(SRendItem::m_RecurseLevel[rd->m_RP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), Inst: %d, RE: 0x%x, LM: 0x%x\n", rd->m_RP.m_pCurObject->m_II.m_Matrix(0,3), rd->m_RP.m_pCurObject->m_II.m_Matrix(1,3), rd->m_RP.m_pCurObject->m_II.m_Matrix(2,3), rd->m_RP.m_pCurObject->m_ObjFlags, rd->m_RP.m_RIs.Num(), rd->m_RP.m_pRE, rd->m_RP.m_DynLMask);
    }
  }
#endif
}


//===================================================================================================

int sLimitSizeByScreenRes(uint32 size)
{
  CD3D9Renderer *r = gcpRendD3D;
#if defined (DIRECT3D9) || defined (OPENGL)
  while(true)
  {
    if (size > (uint32) r->m_pd3dpp->BackBufferWidth || size > (uint32) r->m_pd3dpp->BackBufferHeight)
      size >>= 1;
    else
      break;
  }
#endif
  return size;
}

static int sTexLimitRes(uint32 nSrcsize, uint32 nDstSize)
{
  while(true)
  {
    if (nSrcsize > nDstSize)
      nSrcsize >>= 1;
    else
      break;
  }
  return nSrcsize;
}

ILINE Matrix34 CreateReflectionMat3 ( const Plane& p )
{
  Matrix34 m; 
  f32 vxy   = -2 * p.n.x * p.n.y;
  f32 vxz   = -2 * p.n.x * p.n.z;
  f32 vyz   = -2 * p.n.y * p.n.z;
  f32 pdotn = -2 * p.d;

  m.m00=1-2*p.n.x*p.n.x;  m.m01=vxy;              m.m02=vxz;              m.m03=pdotn*p.n.x;
  m.m10=vxy;              m.m11=1-2*p.n.y*p.n.y;  m.m12=vyz;              m.m13=pdotn*p.n.y;
  m.m20=vxz;              m.m21=vyz;              m.m22=1-2*p.n.z*p.n.z;  m.m23=pdotn*p.n.z;

  return m;
}


static float sScaleBiasMat[16] = 
{
  0.5f, 0,   0,   0,
  0,   -0.5f, 0,   0,
  0,   0,   0.5f, 0,
  0.5f, 0.5f, 0.5f, 1.0f
};

static Matrix34 sMatrixLookAt( const Vec3 &dir,const Vec3 &up,float rollAngle=0 )
{
  Matrix34 M;
  // LookAt transform.
  Vec3 xAxis,yAxis,zAxis;
  Vec3 upVector = up;

  yAxis = -dir.GetNormalized();

  //if (zAxis.x == 0.0 && zAxis.z == 0) up.Set( -zAxis.y,0,0 ); else up.Set( 0,1.0f,0 );

  xAxis = upVector.Cross(yAxis).GetNormalized();
  zAxis = xAxis.Cross(yAxis).GetNormalized();

  // OpenGL kind of matrix.
  M(0,0) = xAxis.x;
  M(0,1) = yAxis.x;
  M(0,2) = zAxis.x;
  M(0,3) = 0;

  M(1,0) = xAxis.y;
  M(1,1) = yAxis.y;
  M(1,2) = zAxis.y;
  M(1,3) = 0;

  M(2,0) = xAxis.z;
  M(2,1) = yAxis.z;
  M(2,2) = zAxis.z;
  M(2,3) = 0;

  if (rollAngle != 0)
  {
    Matrix34 RollMtx;
    RollMtx.SetIdentity();

    float cossin[2];
 //   cry_sincosf(rollAngle, cossin);
		sincos_tpl(rollAngle, &cossin[1],&cossin[0]);

    RollMtx(0,0) = cossin[0]; RollMtx(0,2) = -cossin[1];
    RollMtx(2,0) = cossin[1]; RollMtx(2,2) = cossin[0];

    // Matrix multiply.
    M = RollMtx * M;
  }

  return M;
}

// this is bad: we should re-factor post process stuff to be more general/shareable
static void DrawFullScreenQuad(int nTexWidth, int nTexHeight)
{    
  static SVF_P3F_C4B_T2F pScreenQuad[] =
  {
    { Vec3(0, 0, 0), {{0}}, Vec2(0, 0) },   
    { Vec3(0, 1, 0), {{0}}, Vec2(0, 1) },    
    { Vec3(1, 0, 0), {{0}}, Vec2(1, 0) },   
    { Vec3(1, 1, 0), {{0}}, Vec2(1, 1) },   
  };

  // No offsets required in d3d10
  float fOffsetU = 0.0f;
  float fOffsetV = 0.0f;

#if defined (DIRECT3D9)

  fOffsetU = 0.5f/(float)nTexWidth;
  fOffsetV = 0.5f/(float)nTexHeight;  

#endif

  pScreenQuad[0].xyz = Vec3(-fOffsetU, -fOffsetV, 0);
  pScreenQuad[1].xyz = Vec3(-fOffsetU, 1-fOffsetV, 0);
  pScreenQuad[2].xyz = Vec3(1-fOffsetU, -fOffsetV, 0);
  pScreenQuad[3].xyz = Vec3(1-fOffsetU, 1-fOffsetV, 0);

  CVertexBuffer strip(pScreenQuad, eVF_P3F_C4B_T2F);
  gRenDev->DrawPrimitives(&strip, 4);     
}


void TexBlurAnisotropicVertical(CTexture *pTex, int nAmount, float fScale, float fDistribution, bool bAlphaOnly)
{
  if(!pTex)
  {
    return;
  }
  
  SDynTexture *tpBlurTemp = new SDynTexture(pTex->GetWidth(), pTex->GetHeight(), pTex->GetDstFormat(), eTT_2D,  FT_STATE_CLAMP, "TempBlurAnisoVertRT");
  tpBlurTemp->Update( pTex->GetWidth(), pTex->GetHeight() );

  if( !tpBlurTemp->m_pTexture)
  {
    SAFE_DELETE(tpBlurTemp);
    return;
  }
  
  gcpRendD3D->Set2DMode(true, 1, 1);     


  PROFILE_SHADER_START

    // Get current viewport
    int iTempX, iTempY, iWidth, iHeight;
  gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
  gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());        

  Vec4 vWhite( 1.0f, 1.0f, 1.0f, 1.0f );

  static CCryName pTechName("AnisotropicVertical");
  CShader *m_pCurrShader = CShaderMan::m_shPostEffects;

  uint32 nPasses;
  m_pCurrShader->FXSetTechnique(pTechName);
  m_pCurrShader->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
  m_pCurrShader->FXBeginPass(0);

  gRenDev->EF_SetState(GS_NODEPTHTEST);   

  // setup texture offsets, for texture sampling
  float s1 = 1.0f/(float) pTex->GetWidth();     
  float t1 = 1.0f/(float) pTex->GetHeight();    

  Vec4 pWeightsPS;
  pWeightsPS.x = 0.25f * t1;
  pWeightsPS.y = 0.5f * t1;
  pWeightsPS.z = 0.75f * t1;
  pWeightsPS.w = 1.0f * t1;

  
  pWeightsPS *= -fScale;


  STexState sTexState = STexState(FILTER_LINEAR, true);
  static CCryName pParam0Name("blurParams0");

  //SetTexture(pTex, 0, FILTER_LINEAR); 
  
  for(int p(1); p<= nAmount; ++p)   
  {
    //Horizontal

    CShaderMan::m_shPostEffects->FXSetVSFloat(pParam0Name, &pWeightsPS, 1);  
    gcpRendD3D->FX_PushRenderTarget(0, tpBlurTemp->m_pTexture, &gcpRendD3D->m_DepthBufferOrig);
    gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());        

    pTex->Apply(0, CTexture::GetTexState(sTexState)); 
    DrawFullScreenQuad(pTex->GetWidth(), pTex->GetHeight());

    gcpRendD3D->FX_PopRenderTarget(0);

    //Vertical

    pWeightsPS *= 2.0f;

    gcpRendD3D->FX_PushRenderTarget(0, pTex, &gcpRendD3D->m_DepthBufferOrig);
    gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());         

    CShaderMan::m_shPostEffects->FXSetVSFloat(pParam0Name, &pWeightsPS, 1);  
    tpBlurTemp->m_pTexture->Apply(0, CTexture::GetTexState(sTexState)); 
    DrawFullScreenQuad(pTex->GetWidth(), pTex->GetHeight());      

    gcpRendD3D->FX_PopRenderTarget(0);
  }             

  m_pCurrShader->FXEndPass();
  m_pCurrShader->FXEnd(); 

  // Restore previous viewport
  gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

  //release dyntexture
  SAFE_DELETE(tpBlurTemp);

  gcpRendD3D->FX_Flush();
  PROFILE_SHADER_END      

  gcpRendD3D->Set2DMode(false, 1, 1);     
}


bool CD3D9Renderer::FX_DrawToRenderTarget(CShader *pShader, SRenderShaderResources* pRes, CRenderObject *pObj, SShaderTechnique *pTech, SHRenderTarget *pRT, int nPreprType, CRendElementBase *pRE)
{
   if (!pRT)   
    return false;
   
   int nThreadList = m_pRT->GetThreadList();

   uint32 nPrFlags = pRT->m_nFlags;
   if (nPrFlags & FRT_RENDTYPE_CURSCENE)
     return false;

  if (!pRT->m_pTarget[0] && !pRT->m_pTarget[1])
	{
		if (pRT->m_refSamplerID >=0 && pRT->m_refSamplerID < EFTT_MAX)
		{
			IDynTextureSource* pDynTexSrc(pRes->m_Textures[pRT->m_refSamplerID]->m_Sampler.m_pDynTexSource);
			assert(pDynTexSrc);
			if (pDynTexSrc)
				return m_pRT->RC_DynTexSourceUpdate(pDynTexSrc, pObj->m_fDistance);
		}
    return false;
	}
  
  CRenderObject *pPrevIgn = m_RP.m_TI[nThreadList].m_pIgnoreObject;
  CTexture *Tex = pRT->m_pTarget[0];
  SEnvTexture *pEnvTex = NULL;
  
  if (nPreprType == SPRID_SCANTEX)
  {
    nPrFlags |= FRT_CAMERA_REFLECTED_PLANE;
    pRT->m_nFlags = nPrFlags;
  }

  if( nPrFlags & FRT_RENDTYPE_CURSCENE )
    return false;
  
  uint32 nWidth = pRT->m_nWidth;
  uint32 nHeight = pRT->m_nHeight;

  if (pRT->m_nIDInPool >= 0)
  {
    assert((int)CTexture::s_CustomRT_2D.Num() > pRT->m_nIDInPool);
    pEnvTex = &CTexture::s_CustomRT_2D[pRT->m_nIDInPool];

    if (nWidth == -1)
      nWidth = GetWidth();
    if (nHeight == -1)
      nHeight = GetHeight();

    nWidth = sTexLimitRes(nWidth, uint32(GetWidth() * 0.5f));
    nHeight = sTexLimitRes(nHeight, uint32(GetHeight() * 0.5f));    

    ETEX_Format eTF = pRT->m_eTF;
    // $HDR
    if (eTF == eTF_A8R8G8B8 && IsHDRModeEnabled() && m_nHDRType <= 1)
      eTF = eTF_A16B16G16R16F;
    if (pEnvTex && (!pEnvTex->m_pTex || pEnvTex->m_pTex->GetFormat() != eTF))
    {
      char name[128];
      sprintf(name, "$RT_2D_%d", m_TexGenID++);
      int flags = FT_NOMIPS | FT_STATE_CLAMP | FT_DONT_STREAM | FT_DONT_RESIZE;
      pEnvTex->m_pTex = new SDynTexture(nWidth, nHeight, eTF, eTT_2D, flags, name);
    }
    assert(nWidth > 0 && nWidth <= m_d3dsdBackBuffer.Width);
    assert(nHeight > 0 && nHeight <= m_d3dsdBackBuffer.Height);
    Tex = pEnvTex->m_pTex->m_pTexture;
  }
  else
  if (Tex)
  {
    if (Tex->GetCustomID() == TO_RT_2D)
    {
      bool bReflect = false;
      if (nPrFlags & (FRT_CAMERA_REFLECTED_PLANE | FRT_CAMERA_REFLECTED_WATERPLANE))
        bReflect = true;
      Matrix33 orientation = Matrix33(GetCamera().GetMatrix());
      Ang3 Angs = CCamera::CreateAnglesYPR(orientation);
      Vec3 Pos = GetCamera().GetPosition();
      bool bNeedUpdate = false;
			pEnvTex = CTexture::FindSuitableEnvTex(Pos, Angs, false, -1, false, pShader, pRes, pObj, bReflect, pRE, &bNeedUpdate);

      if (!bNeedUpdate)
      {
        if (!pEnvTex)
          return false;
        if (pEnvTex->m_pTex && pEnvTex->m_pTex->m_pTexture)
          return true;
      }
      m_RP.m_TI[nThreadList].m_pIgnoreObject = pObj;
      switch (CRenderer::CV_r_envtexresolution)
      {
      case 0:
        nWidth = 64;
        break;
      case 1:
        nWidth = 128;
        break;
      case 2:
      default:
        nWidth = 256;
        break;
      case 3:
        nWidth = 512;
        break;
      }
      nWidth = sLimitSizeByScreenRes(nWidth);
      nHeight = nWidth;
      if (!pEnvTex->m_pTex->m_pTexture)
      {
        pEnvTex->m_pTex->Update(nWidth, nHeight);
      }
      Tex = pEnvTex->m_pTex->m_pTexture;
    }
    else
    if (Tex->GetCustomID() == TO_RT_CM)
    {
      Vec3 vPos = pObj->GetTranslation();
      float fDistToCam = (vPos-m_RP.m_TI[nThreadList].m_cam.GetPosition()).len();
      CRenderObject *pPrevIgnore = m_RP.m_TI[nThreadList].m_pIgnoreObject;
      m_RP.m_TI[nThreadList].m_pIgnoreObject = pObj;
      pEnvTex = CTexture::FindSuitableEnvCMap(vPos, false, pRes->m_nReflectionMask, fDistToCam);
      m_RP.m_TI[nThreadList].m_pIgnoreObject = pPrevIgnore;

      if (pEnvTex && pEnvTex->m_pTex->m_pTexture)
        return true;
      return false;
    }
  }
  if (Tex && Tex->IsLocked())
    return true;

  bool bMGPUAllowNextUpdate = (!(gRenDev->m_nFrameSwapID % gRenDev->m_nGPUs)) && (CRenderer::CV_r_waterreflections_mgpu );

  // always allow for non-mgpu
  if( gRenDev->m_nGPUs == 1 || !CRenderer::CV_r_waterreflections_mgpu )
    bMGPUAllowNextUpdate = true; 

  ETEX_Format eTF = pRT->m_eTF;
  // $HDR
  if (eTF == eTF_A8R8G8B8 && IsHDRModeEnabled() && m_nHDRType <= 1)
    eTF = eTF_A16B16G16R16F;
  if (pEnvTex && (!pEnvTex->m_pTex || pEnvTex->m_pTex->GetFormat() != eTF))
  {
    SAFE_DELETE(pEnvTex->m_pTex);
    char name[128];
    sprintf(name, "$RT_2D_%d", m_TexGenID++);
    int flags = FT_NOMIPS | FT_STATE_CLAMP | FT_DONT_STREAM | FT_DONT_RESIZE;
    pEnvTex->m_pTex = new SDynTexture(nWidth, nHeight, eTF, eTT_2D, flags, name);
    assert(nWidth > 0 && nWidth <= m_d3dsdBackBuffer.Width);
    assert(nHeight > 0 && nHeight <= m_d3dsdBackBuffer.Height);
    pEnvTex->m_pTex->Update(nWidth, nHeight);
  }

  bool bEnableAnisotropicBlur = true;
  switch (pRT->m_eUpdateType)
  {
  case eRTUpdate_WaterReflect:
    {
      if( !CRenderer::CV_r_waterreflections )
      {
        ColorF c = ColorF(0, 0, 0, 1);
        assert(pEnvTex!=NULL);
        if (pEnvTex->m_pTex->m_pTexture)
            pEnvTex->m_pTex->m_pTexture->Fill(c);
        return true;
      }

      if( m_RP.m_nLastWaterFrameID == GetFrameID() )
        // water reflection already created this frame, share it
        return true;

      I3DEngine *eng = (I3DEngine *)gEnv->p3DEngine;
      int nVisibleWaterPixelsCount = eng->GetOceanVisiblePixelsCount() / 2; // bug in occlusion query returns 2x more
      int nPixRatioThreshold = (int)(GetWidth() * GetHeight() * CRenderer::CV_r_waterreflections_min_visible_pixels_update);

      static int nVisWaterPixCountPrev = nVisibleWaterPixelsCount;
      if( CRenderer::CV_r_waterreflections_mgpu )
      {
        nVisWaterPixCountPrev = bMGPUAllowNextUpdate ? nVisibleWaterPixelsCount : nVisWaterPixCountPrev;
      }
      else
        nVisWaterPixCountPrev = nVisibleWaterPixelsCount;

      float fUpdateFactorMul = 1.0f;
      float fUpdateDistanceMul = 1.0f;
      if( nVisWaterPixCountPrev < nPixRatioThreshold /4) 
      {
        bEnableAnisotropicBlur = false;
        fUpdateFactorMul = CV_r_waterreflections_minvis_updatefactormul * 10.0f;
        fUpdateDistanceMul = CV_r_waterreflections_minvis_updatedistancemul * 5.0f;
      }
      else
      if( nVisWaterPixCountPrev < nPixRatioThreshold) 
      {
        fUpdateFactorMul = CV_r_waterreflections_minvis_updatefactormul;
        fUpdateDistanceMul = CV_r_waterreflections_minvis_updatedistancemul;
      }

      float fMGPUScale = CRenderer::CV_r_waterreflections_mgpu? (1.0f / (float) gRenDev->m_nGPUs) : 1.0f;
      float fWaterUpdateFactor = CV_r_waterupdateFactor * fUpdateFactorMul * fMGPUScale;
      float fWaterUpdateDistance = CV_r_waterupdateDistance * fUpdateDistanceMul * fMGPUScale;

      float fTimeUpd = min(0.3f, eng->GetDistanceToSectorWithWater());
      fTimeUpd *= fWaterUpdateFactor;
      //if (fTimeUpd > 1.0f)
      //fTimeUpd = 1.0f; 
      Vec3 camView = m_RP.m_TI[nThreadList].m_rcam.ViewDir();
      Vec3 camUp = m_RP.m_TI[nThreadList].m_rcam.Y;

      m_RP.m_nLastWaterFrameID = GetFrameID();

      Vec3 camPos = GetCamera().GetPosition();
      float fDistCam = (camPos - m_RP.m_LastWaterPosUpdate).GetLength();
      float fDotView = camView * m_RP.m_LastWaterViewdirUpdate;
      float fDotUp = camUp * m_RP.m_LastWaterUpdirUpdate;
      float fFOV = GetCamera().GetFov();
      if (m_RP.m_fLastWaterUpdate-1.0f > m_RP.m_TI[nThreadList].m_RealTime)
        m_RP.m_fLastWaterUpdate = m_RP.m_TI[nThreadList].m_RealTime;

      const float fMaxFovDiff = 0.1f;		// no exact test to prevent slowly changing fov causing per frame water reflection updates

      static bool bUpdateReflection = true;
      if( bMGPUAllowNextUpdate )
      {
        bUpdateReflection = m_RP.m_TI[nThreadList].m_RealTime-m_RP.m_fLastWaterUpdate >= fTimeUpd || fDistCam > fWaterUpdateDistance;
        bUpdateReflection = bUpdateReflection || fDotView<0.9f || fabs(fFOV-m_RP.m_fLastWaterFOVUpdate)>fMaxFovDiff;
      }
      
      if ( bUpdateReflection && bMGPUAllowNextUpdate)
      {
        m_RP.m_fLastWaterUpdate = m_RP.m_TI[nThreadList].m_RealTime;
        m_RP.m_LastWaterViewdirUpdate = camView;
        m_RP.m_LastWaterUpdirUpdate = camUp;
        m_RP.m_fLastWaterFOVUpdate = fFOV;
        m_RP.m_LastWaterPosUpdate = camPos;
        assert(pEnvTex!=NULL);
        pEnvTex->m_pTex->ResetUpdateMask();
      }
      else
      if ( !bUpdateReflection )
      {
        assert(pEnvTex!=NULL);
        if ( pEnvTex->m_pTex->IsValid() )
          return true;
      }

      pEnvTex->m_pTex->SetUpdateMask();
    }
    break;
  }

  // Just copy current BB to the render target and exit
  if (nPrFlags & FRT_RENDTYPE_COPYSCENE)
  {
    // Get current render target from the RT stack
    if( !CRenderer::CV_r_debugrefraction )
      FX_ScreenStretchRect( Tex ); // should encode hdr format
    else
    {
      assert(Tex!=NULL);
      ColorF c = ColorF(1, 0, 0, 1);
      Tex->Fill(c);
    }

    return true;
  }

  I3DEngine *eng = (I3DEngine *)gEnv->p3DEngine;
  Matrix44A matProj, matView;
 
  float plane[4];
  bool bUseClipPlane = false;
  bool bChangedCamera = false;

  int nPersFlags = m_RP.m_TI[nThreadList].m_PersFlags;
  int nPersFlags2 = m_RP.m_TI[nThreadList].m_PersFlags2;
  
  static CCamera tmp_cam_mgpu = GetCamera();
  CCamera tmp_cam = GetCamera();
  CCamera prevCamera = tmp_cam;
  bool bMirror = false;
  bool bOceanRefl = false;

  // Set the camera
  if (nPrFlags & FRT_CAMERA_REFLECTED_WATERPLANE)
  {
    bOceanRefl = true;

    m_RP.m_TI[nThreadList].m_pIgnoreObject = pObj;
    float fMinDist = min(SKY_BOX_SIZE*0.5f, eng->GetDistanceToSectorWithWater()); // 16 is half of skybox size
    float fMaxDist = eng->GetMaxViewDistance();

    Vec3 vPrevPos = tmp_cam.GetPosition();
    Vec4 pOceanParams0, pOceanParams1;
    eng->GetOceanAnimationParams(pOceanParams0, pOceanParams1);

    Plane Pl;
    Pl.n = Vec3(0,0,1);
    Pl.d = eng->GetOceanWaterLevel(vPrevPos); // + CRenderer::CV_r_waterreflections_offset;// - pOceanParams1.x;         
    if ((vPrevPos | Pl.n) - Pl.d < 0)
    {
      Pl.d = -Pl.d;
      Pl.n = -Pl.n;
    }

    plane[0] = Pl.n[0];
    plane[1] = Pl.n[1];
    plane[2] = Pl.n[2];
    plane[3] = -Pl.d ;

    Matrix44 camMat;
    GetModelViewMatrix(camMat.GetData());
    Vec3 vPrevDir = Vec3(-camMat(0,2), -camMat(1,2), -camMat(2,2));
    Vec3 vPrevUp = Vec3(camMat(0,1), camMat(1,1), camMat(2,1));
    Vec3 vNewDir = Pl.MirrorVector(vPrevDir);
    Vec3 vNewUp = Pl.MirrorVector(vPrevUp);
    float fDot = vPrevPos.Dot(Pl.n) - Pl.d;
    Vec3 vNewPos = vPrevPos - Pl.n * 2.0f*fDot;
    Matrix34 m = sMatrixLookAt( vNewDir, vNewUp, tmp_cam.GetAngles()[2] );
    m.SetTranslation(vNewPos);
    tmp_cam.SetMatrix(m);

    float fDistOffset = fMinDist;
    if( CV_r_waterreflections_use_min_offset )
    {
      fDistOffset = max( fMinDist, 2.0f * gEnv->p3DEngine->GetDistanceToSectorWithWater() );
      if ( fDistOffset  >= fMaxDist ) // engine returning bad value
        fDistOffset = fMinDist; 
    }

		assert(pEnvTex);
    tmp_cam.SetFrustum((int)(pEnvTex->m_pTex->GetWidth()*tmp_cam.GetProjRatio()), pEnvTex->m_pTex->GetHeight(), tmp_cam.GetFov(), fDistOffset, fMaxDist); //tmp_cam.GetFarPlane());

    // Allow camera update
    if( bMGPUAllowNextUpdate ) 
      tmp_cam_mgpu = tmp_cam;

    SetCamera( tmp_cam_mgpu );
    bChangedCamera = true;
    bUseClipPlane = true;
    bMirror = true;
    //m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_MIRRORCULL;
  }
  else
  if (nPrFlags & FRT_CAMERA_REFLECTED_PLANE)
  {
    m_RP.m_TI[nThreadList].m_pIgnoreObject = pObj;
    float fMinDist = 0.25f;
    float fMaxDist = eng->GetMaxViewDistance();

    Vec3 vPrevPos = tmp_cam.GetPosition();

    if (pRes && pRes->m_pCamera)
    {
      tmp_cam = *pRes->m_pCamera; // Portal case
      //tmp_cam.SetPosition(Vec3(310, 150, 30));
      //tmp_cam.SetAngles(Vec3(-90,0,0));
      //tmp_cam.SetFrustum((int)(Tex->GetWidth()*tmp_cam.GetProjRatio()), Tex->GetHeight(), tmp_cam.GetFov(), fMinDist, tmp_cam.GetFarPlane());

      SetCamera(tmp_cam);
      bUseClipPlane = false;
      bMirror = false;
    }
    else
    { // Mirror case
      Plane Pl;
      pRE->mfGetPlane(Pl);
      //Pl.d = -Pl.d;
      if (pObj)
      {
        m_RP.m_FrameObject++;
        Matrix44 mat = GetTransposed44(Matrix44(pObj->m_II.m_Matrix));
        Pl = TransformPlane(mat, Pl);
      }
      if ((vPrevPos | Pl.n) - Pl.d < 0)
      {
        Pl.d = -Pl.d;
        Pl.n = -Pl.n;
      }

      plane[0] = Pl.n[0];
      plane[1] = Pl.n[1];
      plane[2] = Pl.n[2];
      plane[3] = -Pl.d;

      //this is the new code to calculate the reflection matrix

      Matrix44A camMat;
      GetModelViewMatrix(camMat.GetData());
      Vec3 vPrevDir = Vec3(-camMat(0,2), -camMat(1,2), -camMat(2,2));
      Vec3 vPrevUp = Vec3(camMat(0,1), camMat(1,1), camMat(2,1));
      Vec3 vNewDir = Pl.MirrorVector(vPrevDir);
      Vec3 vNewUp = Pl.MirrorVector(vPrevUp);
      float fDot = vPrevPos.Dot(Pl.n) - Pl.d;
      Vec3 vNewPos = vPrevPos - Pl.n * 2.0f*fDot;
      Matrix34A m = sMatrixLookAt( vNewDir, vNewUp, tmp_cam.GetAngles()[2] );
      m.SetTranslation(vNewPos);
      tmp_cam.SetMatrix(m);

      //Matrix34 RefMatrix34 = CreateReflectionMat3(Pl);
      //Matrix34 matMir=RefMatrix34*tmp_cam.GetMatrix();
      //tmp_cam.SetMatrix(matMir);
			assert(Tex);
      tmp_cam.SetFrustum((int)(Tex->GetWidth()*tmp_cam.GetProjRatio()), Tex->GetHeight(), tmp_cam.GetFov(), fMinDist, fMaxDist); //tmp_cam.GetFarPlane());
      bMirror = true;
      bUseClipPlane = true;
    }
    SetCamera(tmp_cam);
    bChangedCamera = true;
    //m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_MIRRORCULL;
  }
  else
  if (((nPrFlags & FRT_CAMERA_CURRENT) || (nPrFlags & FRT_RENDTYPE_CURSCENE)) && pRT->m_eOrder == eRO_PreDraw && !(nPrFlags & FRT_RENDTYPE_CUROBJECT))
  {
    // Always restore stuff after explicitly changing...

    // get texture surface
    // Get current render target from the RT stack
    if( !CRenderer::CV_r_debugrefraction )
      FX_ScreenStretchRect( Tex ); // should encode hdr format
    else
    {
      ColorF c = ColorF(1, 0, 0, 1);
      Tex->Fill(c);
    }

    m_RP.m_TI[nThreadList].m_pIgnoreObject = pPrevIgn;    
    return true;
  }
  /*	if (pRT->m_nFlags & FRT_CAMERA_CURRENT)
  {
  //m_RP.m_pIgnoreObject = pObj;

  SetCamera(tmp_cam);
  bChangedCamera = true;
  bUseClipPlane = true;
  }*/

  bool bRes = true;

  m_pRT->RC_PushVP();
  m_pRT->RC_PushFog();
  m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_DRAWTOTEXTURE;
  m_RP.m_TI[nThreadList].m_PersFlags2 |= RBPF2_ENCODE_HDR;

  if (m_LogFile)
    Logv(SRendItem::m_RecurseLevel[nThreadList], "*** Set RT for Water reflections ***\n");

	assert(pEnvTex);
  m_pRT->RC_SetEnvTexRT(pEnvTex, pRT->m_bTempDepth ? pEnvTex->m_pTex->GetWidth() : -1, pRT->m_bTempDepth ? pEnvTex->m_pTex->GetHeight() : -1, true);
  m_pRT->RC_ClearRT(pRT->m_nFlags|FRT_CLEAR_IMMEDIATE, &pRT->m_ClearColor, pRT->m_fClearDepth);

  float fAnisoScale = 1.0f;
  if (pRT->m_nFlags & FRT_RENDTYPE_CUROBJECT)
  {
    CCryName nameTech = pTech->m_Name;
    char newTech[128];
    sprintf(newTech, "%s_RT", nameTech.c_str());
    SShaderTechnique *pT = pShader->mfFindTechnique(newTech);
    if (!pT)
      iLog->Log("Error: CD3D9Renderer::FX_DrawToRenderTarget: Couldn't find technique '%s' in shader '%s'\n", newTech, pShader->GetName());
    else
    {
      FX_ObjectChange(pShader, pRes, pObj, pRE);
      FX_Start(pShader, -1, pRes, pRE);
      pRE->mfPrepare();
      FX_DrawShader_General(pShader, pT, false, true);
    }
    m_RP.m_FrameObject++;
  }
  else
  {
    if (bMirror)
    {
      if( bOceanRefl )
        SetCamera(tmp_cam);

			m_pRT->RC_SetEnvTexMatrix(pEnvTex);

      if( bOceanRefl )
        SetCamera(tmp_cam_mgpu);
    }

		m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_OBLIQUE_FRUSTUM_CLIPPING;   
		//m_RP.m_TI[nThreadList].m_PersFlags2 |= RBPF_MIRRORCAMERA;// | RBPF_MIRRORCULL; ??

    Plane p;
    p.n[0] = plane[0];
    p.n[1] = plane[1];
    p.n[2] = plane[2];
    p.d = plane[3]; // +0.25f;    
    fAnisoScale = plane[3];
    fAnisoScale = fabs(fabs(fAnisoScale) - GetCamera().GetPosition().z); 
    m_RP.m_TI[nThreadList].m_bObliqueClipPlane = true;

    // put clipplane in clipspace..
		Matrix44A mView, mProj, mCamProj, mInvCamProj;
		GetModelViewMatrix(&mView(0,0));
		GetProjectionMatrix(&mProj(0,0));  
		mCamProj.Multiply(mView, mProj);
		mInvCamProj.Invert(mCamProj);
    m_RP.m_TI[nThreadList].m_pObliqueClipPlane = TransformPlane2(mInvCamProj, p);

    int RendFlags = (gRenDev->m_RP.m_eQuality)? DLD_TERRAIN : 0;    

    int nReflQuality = ( bOceanRefl )? CV_r_waterreflections_quality : CV_r_reflections_quality; 

    // set reflection quality setting
    switch( nReflQuality )
    {
      case 1: RendFlags |= DLD_ENTITIES;   break;
      case 2: RendFlags |= DLD_DETAIL_TEXTURES | DLD_ENTITIES ;   break;
      case 3: 
        RendFlags |= DLD_STATIC_OBJECTS | DLD_ENTITIES|DLD_DETAIL_TEXTURES;
          break;
      case 4: 
        RendFlags |= DLD_STATIC_OBJECTS | DLD_ENTITIES|DLD_DETAIL_TEXTURES|DLD_PARTICLES;
        break;
      case 5: 
        RendFlags = -1;
        break;
    }

    int nRFlags = SHDF_ALLOWHDR | SHDF_SORT | SHDF_NO_DRAWNEAR;
 
    // disable caustics if camera above water
    if( p.d < 0)
      nRFlags |= SHDF_NO_DRAWCAUSTICS;

    eng->RenderWorld(nRFlags, bOceanRefl ? &tmp_cam_mgpu : &tmp_cam, 1, __FUNCTION__, RendFlags, pRT->m_nFilterFlags);
    
    m_RP.m_TI[nThreadList].m_bObliqueClipPlane = false;
    m_RP.m_TI[nThreadList].m_PersFlags &= ~RBPF_OBLIQUE_FRUSTUM_CLIPPING;
  }
  m_pRT->RC_PopRT(0);

  // Very Hi specs get anisotropic reflections
  int nReflQuality = ( bOceanRefl )? CV_r_waterreflections_quality : CV_r_reflections_quality; 
  if( nReflQuality >= 4 && bEnableAnisotropicBlur && Tex && Tex->GetDevTexture())
    m_pRT->RC_TexBlurAnisotropicVertical(Tex, fAnisoScale);

  if (m_LogFile)
    Logv(SRendItem::m_RecurseLevel[nThreadList], "*** End RT for Water reflections ***\n");

  // todo: encode hdr format

  m_RP.m_TI[nThreadList].m_PersFlags = nPersFlags;
  m_RP.m_TI[nThreadList].m_PersFlags2 = nPersFlags2;

  if (bChangedCamera)
    SetCamera(prevCamera);

  m_pRT->RC_PopVP();
  m_pRT->RC_PopFog();

  // increase frame id to support multiple recursive draws
  m_RP.m_TI[nThreadList].m_pIgnoreObject = pPrevIgn;
  m_RP.m_TI[nThreadList].m_nFrameID++;

  return bRes;
}












































































































