#include "StdAfx.h"

#include "../Common/VideoPlayerInstance.h"
#include "DriverD3D.h"


#ifndef EXCLUDE_CRI_SDK


void CVideoPlayer::ClearTexture(CTexture* pTexture, unsigned char clearValue)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);

	if (CTexture::IsTextureExist(pTexture))
	{



		assert(pTexture->GetDstFormat() == eTF_A8);

    STALL_PROFILER("update texture")
    CDeviceTexture* pTex = pTexture->GetDevTexture();
    assert(pTex);
    STexLock lockedRect;
    if (SUCCEEDED(pTex->LockRect(0, lockedRect, LF_DISCARD)))
		{
      uint32 nWidth = pTexture->GetWidth();
      uint32 nHeight = pTexture->GetHeight();
			for (uint32 y(0); y < nHeight; ++y)
				memset((void*) ((size_t)lockedRect.pData + y * lockedRect.Pitch), clearValue, nWidth);
			pTex->UnlockRect(0);
		}
	}
}


void CVideoPlayer::UploadTextureData(CTexture* pTexture, void* pSrcData, int srcWidth)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);

  if (CTexture::IsTextureExist(pTexture))
	{



		assert(pTexture->GetDstFormat() == eTF_A8);

    STALL_PROFILER("update texture")
    CDeviceTexture* pTex = pTexture->GetDevTexture();
    assert(pTex);
    STexLock lockedRect;
    if (SUCCEEDED(pTex->LockRect(0, lockedRect, LF_DISCARD)))
    {
      int nWidth = pTexture->GetWidth();
      int nHeight = pTexture->GetHeight();
			assert(srcWidth >= nWidth);
			for (int y=0; y < nHeight; ++y)
#	if !defined(XENON)
				memcpy((void*) ((size_t)lockedRect.pData+ y * lockedRect.Pitch), (void*) ((size_t)pSrcData + y * srcWidth), nWidth);


#	endif
			pTex->UnlockRect(0);
		}
	}
}


void CVideoPlayer::Display()
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
	CD3D9Renderer* rd(gcpRendD3D);
	assert(m_pCriMwShader);

	bool isSRGB = rd->IsLinearSpaceShadingEnabled();
#if defined(DIRECT3D9)
#	if !defined(XENON)
	if (isSRGB)
		rd->m_pd3dDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);



#	endif
#elif defined(DIRECT3D10)





#endif

	bool isFullAlphaMovie(IsFullAlphaMovie());

	// setup matrices
	rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matView->Push();
	rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matView->LoadIdentity();

	int viewportX0, viewportY0, viewportWidth, viewportHeight;
	rd->GetViewport(&viewportX0, &viewportY0, &viewportWidth, &viewportHeight);

	rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->Push();
	Matrix44A* m(rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->GetTop());
	mathMatrixOrthoOffCenterLH(m, (float)viewportX0, (float)(viewportX0 + viewportWidth), (float)(viewportY0 + viewportHeight), (float)viewportY0, -1.f, 1.f);

	bool texResourcesOk(!DynTexDataLost());
	if (texResourcesOk)
	{
		CTexture* pTex = m_pCurFrameY;
		texResourcesOk &= CTexture::IsTextureExist(pTex);

		pTex = m_pCurFrameCb;
		texResourcesOk &= CTexture::IsTextureExist(pTex);

		pTex = m_pCurFrameCr;
		texResourcesOk &= CTexture::IsTextureExist(pTex);

		if (isFullAlphaMovie)
		{
			pTex = m_pCurFrameA;
			texResourcesOk &= CTexture::IsTextureExist(pTex);
		}
	}

	if (texResourcesOk)
	{
		if (isFullAlphaMovie)
			m_pCriMwShader->FXSetTechnique("CCIR601_ColAlpha");
		else
			m_pCriMwShader->FXSetTechnique("CCIR601");
	}
	else
		m_pCriMwShader->FXSetTechnique("Fallback");

	uint32 passes(0);
	m_pCriMwShader->FXBegin(&passes, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
	m_pCriMwShader->FXBeginPass(0);
	
	if (texResourcesOk)
	{
		((CTexture*) m_pCurFrameY)->Apply(0);
		((CTexture*) m_pCurFrameCb)->Apply(1);
		((CTexture*) m_pCurFrameCr)->Apply(2);
		if (isFullAlphaMovie)
			((CTexture*) m_pCurFrameA)->Apply(3);
	}

	int nOffs;
	SVF_P3F_C4B_T2F* pQuad((SVF_P3F_C4B_T2F*) rd->GetVBPtr(4, nOffs, POOL_P3F_COL4UB_TEX2F));
  if (!pQuad)
  {
    rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matView->Pop();
    rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->Pop();
    return;
  }

#if defined(DIRECT3D9)
	const float halfPixelOffset(0.5f);
#else
	const float halfPixelOffset(0.0f);
#endif

	pQuad[0].xyz = Vec3(m_viewportX0 + m_viewportWidth - halfPixelOffset, m_viewportY0 - halfPixelOffset, 0);
	pQuad[0].st = Vec2(1, 0);

	pQuad[1].xyz = Vec3(m_viewportX0 - halfPixelOffset, m_viewportY0 - halfPixelOffset, 0);
	pQuad[1].st = Vec2(0, 0);

	pQuad[2].xyz = Vec3(m_viewportX0 + m_viewportWidth - halfPixelOffset, m_viewportY0 + m_viewportHeight - halfPixelOffset, 0);
	pQuad[2].st = Vec2(1, 1);

	pQuad[3].xyz = Vec3(m_viewportX0 - halfPixelOffset, m_viewportY0 + m_viewportHeight - halfPixelOffset, 0);
	pQuad[3].st = Vec2(0, 1);

	rd->UnlockVB(POOL_P3F_COL4UB_TEX2F);

	int state(GS_NODEPTHTEST);
	if (isFullAlphaMovie)
		state |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
	if (rd->CV_r_measureoverdraw)
	{
		state = (state & ~GS_BLEND_MASK) | (GS_BLSRC_ONE | GS_BLDST_ONE);
		state &= ~GS_ALPHATEST_MASK;
	}
	rd->EF_SetState(state);

	rd->D3DSetCull(eCULL_None);

	rd->FX_Commit();
	rd->FX_SetVStream(0, rd->m_pVB[POOL_P3F_COL4UB_TEX2F], 0, sizeof(SVF_P3F_C4B_T2F));
	if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
  {
  #if defined (DIRECT3D9) || defined (OPENGL)
	  rd->m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, nOffs, 2);
  #elif defined (DIRECT3D10)
	  rd->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	  rd->m_pd3dDeviceContext->Draw(4, nOffs);
  #endif
  }

	m_pCriMwShader->FXEndPass();
	m_pCriMwShader->FXEnd();

	rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nPolygons[rd->m_RP.m_nPassGroupDIP] += 2;
	rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_nDIPs[rd->m_RP.m_nPassGroupDIP]++;

	rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matView->Pop();
	rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_matProj->Pop();
	rd->EF_DirtyMatrix();

#if defined(DIRECT3D9)
#	if !defined(XENON)
	if (isSRGB)
		rd->m_pd3dDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);



#	endif
#elif defined(DIRECT3D10)





#endif
}


#endif // #ifndef EXCLUDE_CRI_SDK
