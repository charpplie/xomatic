/*=============================================================================
  D3DRenderThrread.cpp : D3D specific thread functions.
  Copyright 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"

//=======================================================================

bool CD3D9Renderer::RT_CreateDevice()
{
  return SetRes();
}

HRESULT CD3D9Renderer::RT_CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, UINT Pool, void** ppVertexBuffer, HANDLE* pSharedHandle)
{
#if defined (DIRECT3D9) || defined (OPENGL)
  return m_pd3dDevice->CreateVertexBuffer(Length, Usage, FVF, (D3DPOOL)Pool, (D3DVertexBuffer**)ppVertexBuffer, pSharedHandle);
#elif defined (DIRECT3D10)
  assert(0);
#endif
  return S_OK;
}
HRESULT CD3D9Renderer::RT_CreateIndexBuffer(UINT Length, DWORD Usage, DWORD Format, UINT Pool, void** ppIndexBuffer, HANDLE* pSharedHandle)
{
#if defined (DIRECT3D9) || defined (OPENGL)
  return m_pd3dDevice->CreateIndexBuffer(Length, Usage, (D3DFORMAT)Format, (D3DPOOL)Pool, (D3DIndexBuffer**)ppIndexBuffer, pSharedHandle);
#elif defined (DIRECT3D10)
  assert(0);
#endif
  return S_OK;
}

void CD3D9Renderer::RT_ReleaseVBStream(void *pVB, int nStream)
{
  D3DVertexBuffer *pBuf = (D3DVertexBuffer *)pVB;
#if defined (DIRECT3D9)
  if (pBuf == m_RP.m_VertexStreams[nStream].pStream)
  {
    m_RP.m_VertexStreams[nStream].pStream = NULL;
    m_pd3dDevice->SetStreamSource(nStream, NULL, 0, 0);
  }
#endif
  SAFE_RELEASE(pBuf);
}
void CD3D9Renderer::RT_ReleaseCB(void *pVCB)
{
#if defined (DIRECT3D10)
  ID3D11Buffer* pCB = (ID3D11Buffer *)pVCB;
  SAFE_RELEASE(pCB);
#endif
}


HRESULT CD3D9Renderer::RT_CreateVertexShader(DWORD *pBuf, void **pShader, void *pI)
{
#if defined (DIRECT3D9) || defined (OPENGL)
  HRESULT hr = S_OK;
  hr = m_pd3dDevice->CreateVertexShader(pBuf, (D3DVertexShader **)pShader);
  if (FAILED(hr))
    return hr;















































#elif defined (DIRECT3D10)
  assert(0);
#endif
  return S_OK;
}
HRESULT CD3D9Renderer::RT_CreatePixelShader(DWORD *pBuf, void **pShader)
{
#if defined (DIRECT3D9) || defined (OPENGL)
  return m_pd3dDevice->CreatePixelShader(pBuf, (D3DPixelShader **)pShader);
#elif defined (DIRECT3D10)
  assert(0);
#endif
  return S_OK;
}

void CD3D9Renderer::RT_DrawDynVB(int nOffs, int Pool, int nVerts)
{
}


void CD3D9Renderer::RT_DrawDynVB(SVF_P3F_C4B_T2F *pBuf, uint16 *pInds, int nVerts, int nInds, int nPrimType)
{
#if defined (DIRECT3D9) || defined (OPENGL)
  if (FAILED(m_pd3dDevice->TestCooperativeLevel()))
    return;
#endif

  HRESULT h;

  int nOffs, nIOffs;
  SVF_P3F_C4B_T2F *pDst = (SVF_P3F_C4B_T2F *)GetVBPtr(nVerts, nOffs);
  assert(pDst);
  if (!pDst)
    return;

  uint16 *pDstInds = NULL;
  if (pInds)
    pDstInds = GetIBPtr(nInds, nIOffs);

  memcpy(pDst, pBuf, nVerts*sizeof(SVF_P3F_C4B_T2F));
  if (pDstInds)
  {
    memcpy(pDstInds, pInds, nInds*sizeof(short));
  }

  UnlockVB(0);
  if (pDstInds)
    UnlockIB();
  FX_SetFPMode();

  // Bind our vertex as the first data stream of our device
  h = FX_SetVStream(0, m_pVB[0], 0, sizeof(SVF_P3F_C4B_T2F));
  // Render triangles from the data stream

  int primType;
  int vertexCount = (pDstInds ? nInds : nVerts);

#if defined (DIRECT3D9) || defined (OPENGL)
  int primCount;
  switch(nPrimType)
  {
  case R_PRIMV_LINES:
    primType = D3DPT_LINELIST;
    primCount = vertexCount/2;
    break;
  case R_PRIMV_LINESTRIP:
    primType = D3DPT_LINESTRIP;
    primCount = vertexCount-1;
    break;
  case R_PRIMV_TRIANGLE_STRIP:
    primType = D3DPT_TRIANGLESTRIP;
    primCount = vertexCount-2;
    break;
  case R_PRIMV_TRIANGLE_FAN:
    primType = D3DPT_TRIANGLEFAN;
    primCount = vertexCount-2;
    break;
  default:
    primType = D3DPT_TRIANGLELIST;
    primCount = vertexCount/3;
    break;
  }
#elif defined (DIRECT3D10)
  switch(nPrimType)
  {
  case R_PRIMV_LINES:
    primType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
    break;
  case R_PRIMV_LINESTRIP:
    primType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
    break;
  case R_PRIMV_TRIANGLE_STRIP:
    primType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    break;
  case R_PRIMV_TRIANGLE_FAN:
    assert(0);
    break;
  default:
    primType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    break;
  }
#endif

  if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
  {
    if (pDstInds)
    {
      h = FX_SetIStream(m_pIB);
#if defined (DIRECT3D9) || defined (OPENGL)
      h = m_pd3dDevice->DrawIndexedPrimitive((D3DPRIMITIVETYPE)primType, nOffs, 0, nVerts, nIOffs, primCount);
#elif defined (DIRECT3D10)
      SetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)primType);

      m_pd3dDeviceContext->DrawIndexed(nInds, nIOffs, nOffs);
#endif
    }
    else
    {
#if defined (DIRECT3D9) || defined (OPENGL)
      h = m_pd3dDevice->DrawPrimitive((D3DPRIMITIVETYPE)primType, nOffs, primCount);
#elif defined (DIRECT3D10)
      SetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)primType);

      m_pd3dDeviceContext->Draw(vertexCount, nOffs);
#endif
    }
  }
}

void CD3D9Renderer::RT_Draw2dImage(float xpos,float ypos,float w,float h,int textureid,float s0,float t0,float s1,float t1,float angle,DWORD col,float z)
{
  bool bSaveZTest = ((m_RP.m_CurState & GS_NODEPTHTEST) == 0);
  SetCullMode(R_CULL_DISABLE);

  HRESULT hReturn = S_OK;

  xpos = (float)ScaleCoordX(xpos); w = (float)ScaleCoordX(w);
  ypos = (float)ScaleCoordY(ypos); h = (float)ScaleCoordY(h);
	//no half pixel offset on D3D10 (including PS3)

#if HALF_PIXEL_SHIFT_NEEDED
	float fx = xpos-0.5f;
	float fy = ypos-0.5f;
#else
	float fx = xpos;
	float fy = ypos;
#endif

  float fw = w;
  float fh = h;

  EF_SelectTMU(0);

  if(textureid>0)
  {
    SetTexture(textureid);
    EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
  }
  else
  {
    EnableTMU(false);
    EF_SetColorOp(eCO_REPLACE, eCO_REPLACE, (eCA_Diffuse|(eCA_Diffuse<<3)), (eCA_Diffuse|(eCA_Diffuse<<3)));
  }

  //CTexture::m_Text_Fog->Apply(0);


  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Push();
  Matrix44A *m = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop();
  mathMatrixOrthoOffCenterLH(m, 0.0f, (float)m_width, (float)m_height, 0.0f, 0.0f, 1.0f);
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Push();
  m = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->GetTop();
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->LoadIdentity();

  // Lock the entire buffer and obtain a pointer to the location where we have to
  // write the vertex data. Note that we specify zero here to lock the entire
  // buffer.

  int nOffs;
  SVF_P3F_C4B_T2F *vQuad = (SVF_P3F_C4B_T2F *)GetVBPtr(4, nOffs);
  if (!vQuad)
  {
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Pop();
    return;
  }

  // Now that we have write access to the buffer, we can copy our vertices
  // into it

  if (angle!=0)
  {
    float xsub=(float)(xpos+w/2.0f);
    float ysub=(float)(ypos+h/2.0f);

    float x,y,x1,y1;
    float mcos=cry_cosf(DEG2RAD(angle));
    float msin=cry_sinf(DEG2RAD(angle));

    x=xpos-xsub;
    y=ypos-ysub;
    x1=x*mcos-y*msin;
    y1=x*msin+y*mcos;
    x1+=xsub;y1+=ysub;

    // Define the quad
    vQuad[0].xyz.x = x1;
    vQuad[0].xyz.y = y1;

    x=xpos+w-xsub;
    y=ypos-ysub;
    x1=x*mcos-y*msin;
    y1=x*msin+y*mcos;
    x1+=xsub;y1+=ysub;

    vQuad[1].xyz.x = x1;//fx + fw;
    vQuad[1].xyz.y = y1;// fy;

    x=xpos+w-xsub;
    y=ypos+h-ysub;
    x1=x*mcos-y*msin;
    y1=x*msin+y*mcos;
    x1+=xsub;y1+=ysub;

    vQuad[3].xyz.x = x1;//fx + fw;
    vQuad[3].xyz.y = y1;//fy + fh;

    x=xpos-xsub;
    y=ypos+h-ysub;
    x1=x*mcos-y*msin;
    y1=x*msin+y*mcos;
    x1+=xsub;y1+=ysub;

    vQuad[2].xyz.x = x1;//fx;
    vQuad[2].xyz.y = y1;//fy + fh;
  }
  else
  {
    // Define the quad
    vQuad[0].xyz.x = fx;
    vQuad[0].xyz.y = fy;

    vQuad[1].xyz.x = fx + fw;
    vQuad[1].xyz.y = fy;

    vQuad[2].xyz.x = fx;
    vQuad[2].xyz.y = fy + fh;

    vQuad[3].xyz.x = fx + fw;
    vQuad[3].xyz.y = fy + fh;
  }

  vQuad[0].st = Vec2(s0, 1.0f-t0);
  vQuad[1].st = Vec2(s1, 1.0f-t0);
  vQuad[2].st = Vec2(s0, 1.0f-t1);
  vQuad[3].st = Vec2(s1, 1.0f-t1);

  // set data
  for(int i=0;i<4;i++)
  {
    vQuad[i].color.dcolor = col;
    vQuad[i].xyz.z = z;
  }

  // We are finished with accessing the vertex buffer
  UnlockVB();

  FX_SetFPMode();
  // Bind our vertex as the first data stream of our device
  FX_SetVStream(0, m_pVB[POOL_P3F_COL4UB_TEX2F], 0, sizeof(SVF_P3F_C4B_T2F));
  if (FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
  {
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Pop();
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();
    return;
  }

  // Render the two triangles from the data stream
#if defined (DIRECT3D9) || defined (OPENGL)
  hReturn = m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, nOffs, 2);
#elif defined (DIRECT3D10)
  SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  m_pd3dDeviceContext->Draw(4, nOffs);
#endif

  EF_PopMatrix();
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();
  if (FAILED(hReturn))
  {
    assert(hReturn);
    return;
  }
}

void CD3D9Renderer::RT_PushRenderTarget(int nTarget, CTexture *pTex, SD3DSurface *pDepth, int nS)
{
  FX_PushRenderTarget(nTarget, pTex, pDepth, false, nS);
}

void CD3D9Renderer::RT_PopRenderTarget(int nTarget)
{
  FX_PopRenderTarget(nTarget);
}

void CD3D9Renderer::RT_Init()
{
  EF_Init();
  m_cEF.mfInit();
}

void CD3D9Renderer::RT_CreateResource(SResourceAsync* pRes)
{
  if (pRes->eClassName == eRCN_Texture)
  {
    CTexture * pTex = NULL;

    if(pRes->nTexId)
    { // only create device texture
      pTex = CTexture::GetByID(pRes->nTexId);
      byte * arrData[6] = {pRes->pData,0,0,0,0,0};
      pTex->CreateDeviceTexture(arrData);
    }
    else
    { // create full texture
      char *pName = pRes->Name;
      char szName[128];
      if (!pName)
      {
        sprintf(szName, "$AutoDownloadAsync_%d", m_TexGenID++);
        pName = szName;
      }
      pTex = CTexture::Create2DTexture(pName, pRes->nWidth, pRes->nHeight, pRes->nMips, pRes->nTexFlags, pRes->pData, (ETEX_Format)pRes->nFormat, (ETEX_Format)pRes->nFormat);
    }

    SAFE_DELETE(pRes->pData);
    pRes->pResource = pTex;
    pRes->nReady = (CTexture::IsTextureExist(pTex));
  }
  else
  {
    assert(0);
  }

  delete pRes;
}
void CD3D9Renderer::RT_ReleaseResource(SResourceAsync* pRes)
{
  if (pRes->eClassName == eRCN_Texture)
  {
    CTexture *pTex = (CTexture *)pRes->pResource;
    pTex->Release();
  }
  else
  {
    assert(0);
  }

  delete pRes;
}
