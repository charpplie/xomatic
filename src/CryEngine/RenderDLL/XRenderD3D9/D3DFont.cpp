#include "StdAfx.h"
#include "DriverD3D.h"

//=========================================================================================

#include "../CryFont/FBitmap.h"

bool CD3D9Renderer::FontUpdateTexture(int nTexId, int nX, int nY, int USize, int VSize, byte *pData)
{
  CTexture *tp = CTexture::GetByID(nTexId);
  assert (tp);

  if (tp)
  {
    tp->UpdateTextureRegion(pData, nX, nY, USize, VSize, eTF_A8);

		return true;
  }
  return false;
}

bool CD3D9Renderer::FontUploadTexture(class CFBitmap* pBmp, ETEX_Format eTF)
{
  if(!pBmp)
	{
		return false;
	}

	unsigned int *pData = new unsigned int[pBmp->GetWidth() * pBmp->GetHeight()];

	if (!pData)
	{
		return false;
	}

	pBmp->Get32Bpp(&pData);

	char szName[128];
	sprintf(szName, "$AutoFont_%d", m_TexGenID++);

	int iFlags = FT_TEX_FONT | FT_DONT_STREAM | FT_DONT_RESIZE;
  CTexture *tp = CTexture::Create2DTexture(szName, pBmp->GetWidth(), pBmp->GetHeight(), 1, iFlags, (unsigned char *)pData, eTF, eTF);

	SAFE_DELETE_ARRAY(pData);

	pBmp->SetRenderData((void *)tp);
	
	return true;
}

int CD3D9Renderer::FontCreateTexture(int Width, int Height, byte *pData, ETEX_Format eTF, bool genMips)
{
  if (!pData)
    return -1;

  char szName[128];
  sprintf(szName, "$AutoFont_%d", m_TexGenID++);

  int iFlags = FT_TEX_FONT | FT_DONT_STREAM | FT_DONT_RESIZE;
	if( genMips )
		iFlags |= FT_FORCE_MIPS;
	CTexture *tp = CTexture::Create2DTexture(szName, Width, Height, 1, iFlags, (unsigned char *)pData, eTF, eTF);

  return tp->GetID();
}

void CD3D9Renderer::FontReleaseTexture(class CFBitmap *pBmp)
{
	if(!pBmp)
	{
		return;
	}
  
	CTexture *tp = (CTexture *)pBmp->GetRenderData();

  SAFE_RELEASE(tp);
}

void CD3D9Renderer::FontSetTexture(class CFBitmap* pBmp, int nFilterMode)
{
	if (pBmp)
	{
		CTexture *tp = (CTexture *)pBmp->GetRenderData();
    if (tp)
    {
			EF_SelectTMU(0);
      tp->SetFilterMode(nFilterMode);
	  	tp->Apply();
    }
  }
}

void CD3D9Renderer::FontSetTexture(int nTexId, int nFilterMode)
{
  if (nTexId <= 0)
    return;
  CTexture *tp = CTexture::GetByID(nTexId);
  assert (tp);
  if (!tp)
    return;

#ifdef OPENGL
  nFilterMode = FILTER_LINEAR;
#endif
	EF_SelectTMU(0);
  tp->SetFilterMode(nFilterMode);
  tp->Apply();
  //CTexture::m_Text_NoTexture->Apply(0);
}

int s_InFontState = 0;

void CD3D9Renderer::FontSetRenderingState(unsigned int nVPWidth, unsigned int nVPHeight)
{
  assert(!s_InFontState);
  assert(m_pRT->IsRenderThread());

  // setup various d3d things that we need
  FontSetState(false);

  s_InFontState++;

  D3DXMATRIXA16 *m;
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Push();
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->LoadIdentity();
  m = (D3DXMATRIXA16*)m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop();
  
	if (m_NewViewport.nWidth != 0 && m_NewViewport.nHeight != 0)
		mathMatrixOrthoOffCenter((Matrix44A*)m, 0.0f, (float)m_NewViewport.nWidth, (float)m_NewViewport.nHeight, 0.0f, -1.0f, 1.0f);
  //m_pd3dDevice->SetTransform(D3DTS_PROJECTION, m);

  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Push();
  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->LoadIdentity();
  //m_pd3dDevice->SetTransform(D3DTS_VIEW, (D3DXMATRIX*)m_RP.m_TI[nThreadID].m_matView->GetTop());
}

void CD3D9Renderer::FontRestoreRenderingState()
{
  D3DXMATRIXA16 *m;

  assert(m_pRT->IsRenderThread());
  assert(s_InFontState == 1);
  s_InFontState--;

  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->Pop();
  m = (D3DXMATRIXA16*)m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj->GetTop();
  //m_pd3dDevice->SetTransform(D3DTS_PROJECTION, m);

  m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView->Pop();

  FontSetState(true);
}


// match table with the blending modes
static int nBlendMatchTable[] =
{
	D3DBLEND_ZERO,
	D3DBLEND_ONE,
	D3DBLEND_SRCCOLOR,
	D3DBLEND_INVSRCCOLOR,
	D3DBLEND_SRCALPHA,
	D3DBLEND_INVSRCALPHA,
	D3DBLEND_DESTALPHA,
	D3DBLEND_INVDESTALPHA,
	D3DBLEND_DESTCOLOR,
	D3DBLEND_INVDESTCOLOR,
};

void CD3D9Renderer::FontSetBlending(int blendSrc, int blendDest)
{
  assert(m_pRT->IsRenderThread());
	m_fontBlendMode = blendSrc|blendDest;
	
	if(eTF_G16R16F == CTexture::s_eTFZ || CTexture::s_eTFZ == eTF_R32F)
		EF_SetState(m_fontBlendMode);// | GS_NODEPTHTEST);
	else
		EF_SetState(m_fontBlendMode /*| GS_NODEPTHTEST*/ | GS_ALPHATEST_GREATER, 0);
}

void CD3D9Renderer::FontSetState(bool bRestore)
{
  static DWORD wireframeMode;
  static D3DCOLORVALUE color;
  static bool bMatColor;
  static int State;
  
  assert(m_pRT->IsRenderThread());

  // grab the modes that we might need to change
  if(!bRestore)
  {
    D3DSetCull(eCULL_None);

    State = m_RP.m_CurState;
    wireframeMode = m_wireframe_mode;

    EF_SetVertColor();
    
    if(wireframeMode > R_SOLID_MODE)
    {
#if defined (DIRECT3D9) || defined(OPENGL)
      m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
#elif defined (DIRECT3D10)
			SStateRaster RS = gcpRendD3D->m_StatesRS[gcpRendD3D->m_nCurStateRS];
			RS.Desc.FillMode = D3D11_FILL_SOLID;
			gcpRendD3D->SetRasterState(&RS);
#endif
    }

    m_RP.m_FlagsPerFlush = 0;
		if (eTF_G16R16F == CTexture::s_eTFZ || CTexture::s_eTFZ == eTF_R32F)
			EF_SetState(m_fontBlendMode);// | GS_NODEPTHTEST);
		else
			EF_SetState(m_fontBlendMode /*| GS_NODEPTHTEST*/ | GS_ALPHATEST_GREATER, 0);

    EF_SetColorOp(eCO_REPLACE, eCO_MODULATE, eCA_Diffuse | (eCA_Diffuse<<3), DEF_TEXARG0);
		//m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		//m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  }
  else
  {
    if(wireframeMode == R_WIREFRAME_MODE)
    {
#if defined (DIRECT3D9) || defined(OPENGL)
      m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
#elif defined (DIRECT3D10)
			SStateRaster RS = gcpRendD3D->m_StatesRS[gcpRendD3D->m_nCurStateRS];
			RS.Desc.FillMode = D3D11_FILL_WIREFRAME;
			gcpRendD3D->SetRasterState(&RS);
#endif
    }
    if(wireframeMode == R_POINT_MODE)
    {
#if defined (DIRECT3D9) || defined(OPENGL)
      m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_POINT);




#endif
    }
		
    //EF_SetState(State);
  }
}

void CD3D9Renderer::DrawDynVB(int nOffs, int Pool, int nVerts)
{
  PROFILE_FRAME(Draw_IndexMesh_Dyn);

  //if (CV_d3d9_forcesoftware)
  //  return;
  if (!nVerts)
    return;
  if (m_bDeviceLost)
    return;

  HRESULT h = S_OK;

  SVF_P3F_C4B_T2F *pDst = (SVF_P3F_C4B_T2F *)GetVBPtr(nVerts, nOffs);
  //assert(pDst);
  if (!pDst)
    return;

  assert(m_pRT->IsRenderThread());

  if (m_pTempDynVB)
  {
#ifndef XENON
    memcpy(pDst, m_pTempDynVB, nVerts*sizeof(SVF_P3F_C4B_T2F));


#endif
    SAFE_DELETE_ARRAY(m_pTempDynVB);
  }
  else
  {
#ifndef XENON
    memcpy(pDst, m_DynVB, nVerts*sizeof(SVF_P3F_C4B_T2F));


#endif
  }

  UnlockVB(POOL_P3F_COL4UB_TEX2F);
  FX_SetFPMode();

  // Bind our vertex as the first data stream of our device
  FX_SetVStream(0, m_pVB[POOL_P3F_COL4UB_TEX2F], 0, sizeof(SVF_P3F_C4B_T2F));
  if (FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    return;

  // Render the two triangles from the data stream
#if defined (DIRECT3D9) || defined (OPENGL)
  h = m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, nOffs, nVerts/3);
#elif defined (DIRECT3D10)
  SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_pd3dDeviceContext->Draw(nVerts, nOffs);
#endif
}

void CD3D9Renderer::DrawDynVB(SVF_P3F_C4B_T2F *pBuf, uint16 *pInds, int nVerts, int nInds, int nPrimType)
{
  PROFILE_FRAME(Draw_IndexMesh_Dyn);

  if (!pBuf)
    return;

  //if (CV_d3d9_forcesoftware)
  //  return;
  if (!nVerts || (pInds && !nInds))
    return;

  m_pRT->RC_DrawDynVB(pBuf, pInds, nVerts, nInds, nPrimType);
}

//===============================================================================

void *CD3D9Renderer::GetDynVBPtr(int nVerts, int &nOffs, int Pool)
{
  void *vBuf = NULL;
  nOffs = 0;
  switch(Pool)
  {
  case 0:
  default:
    if (nVerts <= 2048)
    {
      SAFE_DELETE_ARRAY(m_pTempDynVB);
      return m_DynVB;
    }
    else
    {
      m_pTempDynVB = new SVF_P3F_C4B_T2F [nVerts];
      return m_pTempDynVB;
    }
    break;

  case 1:
  case 2:
    vBuf = GetVBPtr(nVerts, nOffs, Pool);
    break;
  }
  return vBuf;
}

