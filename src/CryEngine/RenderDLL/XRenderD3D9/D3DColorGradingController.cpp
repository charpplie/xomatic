#include "StdAfx.h"
#include "D3DColorGradingController.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include <StringUtils.h>


const int COLORCHART_WIDTH = 16;
const int COLORCHART_HEIGHT = 16 * 16;
const uint32 COLORCHART_TEXFLAGS = FT_NOMIPS |  FT_DONT_STREAM | FT_DONT_RESIZE | FT_STATE_CLAMP;
const char* COLORCHART_DEF_TEX = "textures/defaults/default_cch.tif";


CColorGradingControllerD3D::CColorGradingControllerD3D(CD3D9Renderer* pRenderer)
: m_layers()
, m_pRenderer(pRenderer)
, m_pQuadVB(0)
, m_pQuadData(0)
, m_pChartIdentity(0)
, m_pChartStatic(0)
, m_pChartToUse(0)
{
	assert(m_pRenderer);
	m_pMergeLayers[0] = m_pMergeLayers[1] = 0;
}


CColorGradingControllerD3D::~CColorGradingControllerD3D()
{
	SAFE_RELEASE(m_pChartIdentity);
	SAFE_RELEASE(m_pChartStatic);
	SAFE_RELEASE(m_pMergeLayers[0]);
	SAFE_RELEASE(m_pMergeLayers[1]);
	SAFE_DELETE(m_pQuadVB);
	SAFE_DELETE(m_pQuadData);
}


bool CColorGradingControllerD3D::ValidateColorChart(const CTexture* pChart) const
{
  if (!CTexture::IsTextureExist(pChart))
		return false;

	if (pChart->IsNoTexture())
		return false;

	if (pChart->GetWidth() != COLORCHART_WIDTH || pChart->GetHeight() != COLORCHART_HEIGHT)
		return false;

	return true;
}


CTexture* CColorGradingControllerD3D::LoadColorChartInt(const char* pChartFilePath) const
{
	if (!pChartFilePath || !pChartFilePath[0])
		return 0;

	CTexture* pChart = (CTexture*) m_pRenderer->EF_LoadTexture(pChartFilePath, COLORCHART_TEXFLAGS, eTT_2D);
	if (!ValidateColorChart(pChart))
	{
		SAFE_RELEASE(pChart);
		return 0;
	}
	return pChart;
}


int CColorGradingControllerD3D::LoadColorChart(const char* pChartFilePath) const
{
	CTexture* pChart = LoadColorChartInt(pChartFilePath);
	return pChart ? pChart->GetID() : -1;
}


int CColorGradingControllerD3D::LoadDefaultColorChart() const
{
	CTexture* pChartIdentity = LoadColorChartInt(COLORCHART_DEF_TEX);
	return pChartIdentity ? pChartIdentity->GetID() : -1;
}


void CColorGradingControllerD3D::UnloadColorChart(int texID) const
{
	CTexture* pChart = CTexture::GetByID(texID);
	SAFE_RELEASE(pChart);
}


void CColorGradingControllerD3D::SetLayers(const SColorChartLayer* pLayers, uint32 numLayers)
{
	gRenDev->m_pRT->RC_CGCSetLayers(this, pLayers, numLayers);
}


void CColorGradingControllerD3D::RT_SetLayers(const SColorChartLayer* pLayerInfo, uint32 numLayers)
{
	m_layers.reserve(numLayers);
	m_layers.resize(0);

	if (numLayers)
	{
		float blendSum = 0;
		for (size_t i=0; i<numLayers; ++i)
		{
			const SColorChartLayer& l = pLayerInfo[i];
			if (l.m_texID > 0 && l.m_blendAmount > 0)
			{
				const CTexture* pChart = CTexture::GetByID(l.m_texID);
				if (ValidateColorChart(pChart))
				{
					m_layers.push_back(l);
					blendSum += l.m_blendAmount;
				}
			}
		}

		const size_t numActualLayers = m_layers.size();
		if (numActualLayers)
		{
			if (numActualLayers > 1)
			{
				float normalizeBlendAmount = (float) (1.0 / (double) blendSum);
				for (size_t i=0; i<numActualLayers; ++i)
					m_layers[i].m_blendAmount *= normalizeBlendAmount;
			}
			else
				m_layers[0].m_blendAmount = 1;
		}
	}
}


bool CColorGradingControllerD3D::InitResources()
{
	if (!m_pChartIdentity)
	{
		m_pChartIdentity = LoadColorChartInt(COLORCHART_DEF_TEX);
		if (!m_pChartIdentity)
			return false;
	}

#if !defined(PS3)
	const uint32 RTWidth = COLORCHART_WIDTH;


#endif

	if (!m_pMergeLayers[0])
	{
		m_pMergeLayers[0] = CTexture::CreateRenderTarget("ColorGradingMergeLayer0", RTWidth, COLORCHART_HEIGHT, eTT_2D, COLORCHART_TEXFLAGS, eTF_A8R8G8B8);
    if (!CTexture::IsTextureExist(m_pMergeLayers[0]))
			return false;
	}

	if (!m_pMergeLayers[1])
	{
		m_pMergeLayers[1] = CTexture::CreateRenderTarget("ColorGradingMergeLayer1", RTWidth, COLORCHART_HEIGHT, eTT_2D, COLORCHART_TEXFLAGS, eTF_A8R8G8B8);
    if (!CTexture::IsTextureExist(m_pMergeLayers[1]))
			return false;
	}

	if (!m_pQuadVB)
	{
		assert(!m_pQuadData);
		m_pQuadData = new SVF_P3F_C4B_T2F[4];
		m_pQuadData[0].xyz = Vec3(-1, -1, 0);
		m_pQuadData[1].xyz = Vec3(1, -1, 0);
		m_pQuadData[2].xyz = Vec3(-1, 1, 0);
		m_pQuadData[3].xyz = Vec3(1, 1, 0);
		m_pQuadVB = new CVertexBuffer(m_pQuadData, eVF_P3F_C4B_T2F, 4);
	}

	return true;
}


bool CColorGradingControllerD3D::Update(const SColorGradingMergeParams *pMergeParams)
{
	m_pChartToUse = 0;

	if (!m_pRenderer->CV_r_colorgrading_charts)
		return true;

	if (m_pChartStatic)
	{
		m_pChartToUse = m_pChartStatic;
		return true;
	}

	if (!InitResources())
	{
		m_pChartToUse = m_pChartIdentity;
		return m_pChartToUse != 0;
	}

	static const int texStatePntID = CTexture::GetTexState(STexState(FILTER_POINT, true));
	static const int texStateLinID = CTexture::GetTexState(STexState(FILTER_LINEAR, true));

	// merge layers
	const size_t numLayers = m_layers.size();
	if (!numLayers)
	{
		m_pChartToUse = m_pChartIdentity;
	}
	else if (numLayers == 1)
	{
		m_pChartToUse = CTexture::GetByID(m_layers[0].m_texID);
	}
	else
	{
		CTexture* pNewMergeResult = m_pMergeLayers[0];
		m_pRenderer->FX_PushRenderTarget(0, pNewMergeResult, 0);



		//m_pRenderer->EF_ClearBuffers(FRT_CLEAR_COLOR | FRT_CLEAR_IMMEDIATE, &ColorF(0, 0, 0, 0));

		int numMergePasses = 0;
		for (size_t curLayer=0; curLayer<numLayers; )
		{
			size_t mergeLayerIdx[4] = {-1, -1, -1, -1};
			int numLayersPerPass = 0;

			for (; curLayer<numLayers && numLayersPerPass<4; ++curLayer)
			{
				if (m_layers[curLayer].m_blendAmount > 0.001)
					mergeLayerIdx[numLayersPerPass++] = curLayer;
			}

			if (numLayersPerPass)
			{
				m_pRenderer->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]|g_HWSR_MaskBit[HWSR_SAMPLE1]|g_HWSR_MaskBit[HWSR_SAMPLE2]);
				if ((numLayersPerPass-1) & 1)
					gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
				if ((numLayersPerPass-1) & 2)
					gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];

				CShader* pSh = CShaderMan::m_shPostEffectsGame;

				static CCryName techName("MergeColorCharts");
				NPostEffects::SD3DPostEffectsUtils::ShBeginPass(pSh, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

				Vec4 layerBlendAmount(0, 0, 0, 0);
				for (int i=0; i<numLayersPerPass; ++i)
				{
					const SColorChartLayer& l = m_layers[mergeLayerIdx[i]];
					CTexture* pChart = CTexture::GetByID(l.m_texID);
					pChart->Apply(i, texStatePntID);
					layerBlendAmount[i] = l.m_blendAmount;
				}

				static CCryName semLayerBlendAmount("LayerBlendAmount");
				NPostEffects::SD3DPostEffectsUtils::ShSetParamPS(semLayerBlendAmount, layerBlendAmount);

				m_pRenderer->EF_SetState(GS_NODEPTHTEST | (numMergePasses ? GS_BLSRC_ONE | GS_BLDST_ONE : 0));
				m_pRenderer->SetCullMode(R_CULL_NONE);
				m_pRenderer->DrawPrimitives(m_pQuadVB, 4, R_PRIMV_TRIANGLE_STRIP);

				NPostEffects::SD3DPostEffectsUtils::ShEndPass();
				++numMergePasses;
			}
		}

		m_pRenderer->FX_PopRenderTarget(0);
		m_pChartToUse = numMergePasses ? pNewMergeResult : m_pChartIdentity;
	}

	// combine merged layers with color grading stuff
	if (m_pChartToUse && pMergeParams)
	{
		uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
		gRenDev->m_RP.m_FlagsShader_RT = pMergeParams->nFlagsShaderRT;

		CTexture* pNewMergeResult = m_pMergeLayers[1];
		m_pRenderer->FX_PushRenderTarget(0, pNewMergeResult, 0); 




		CShader* pSh = CShaderMan::m_shPostEffectsGame;

		static CCryName techName("CombineColorGradingWithColorChart");
		NPostEffects::SD3DPostEffectsUtils::ShBeginPass(pSh, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

		m_pChartIdentity->Apply(0, texStatePntID);
		m_pChartToUse->Apply(1, texStateLinID);

		static CCryName pParamName0("ColorGradingParams0");
		static CCryName pParamName1("ColorGradingParams1");
		static CCryName pParamName2("ColorGradingParams2");
		static CCryName pParamName3("ColorGradingParams3");
		static CCryName pParamName4("ColorGradingParams4");
		static CCryName pParamMatrix("mColorGradingMatrix");

		pSh->FXSetPSFloat(pParamName0, &pMergeParams->pLevels[0], 1);
		pSh->FXSetPSFloat(pParamName1, &pMergeParams->pLevels[1], 1);
		pSh->FXSetPSFloat(pParamName2, &pMergeParams->pFilterColor, 1);
		pSh->FXSetPSFloat(pParamName3, &pMergeParams->pSelectiveColor[0], 1);
		pSh->FXSetPSFloat(pParamName4, &pMergeParams->pSelectiveColor[1], 1);
		pSh->FXSetPSFloat(pParamMatrix, &pMergeParams->pColorMatrix[0], 3);

		m_pRenderer->EF_SetState(GS_NODEPTHTEST);
		m_pRenderer->SetCullMode(R_CULL_NONE);
		m_pRenderer->DrawPrimitives(m_pQuadVB, 4, R_PRIMV_TRIANGLE_STRIP);

		NPostEffects::SD3DPostEffectsUtils::ShEndPass();

		m_pRenderer->FX_PopRenderTarget(0);
		m_pChartToUse = pNewMergeResult;

		gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
	}

	return m_pChartToUse != 0;
}


CTexture* CColorGradingControllerD3D::GetColorChart() const
{
	return m_pChartToUse;
}


void CColorGradingControllerD3D::DrawLayer(float x, float y, float w, float h, CTexture* pChart, float blendAmount, const char* pLayerName) const
{
	CShader* pSh = CShaderMan::m_shPostEffectsGame;

	static CCryName techName("DisplayColorCharts");
	NPostEffects::SD3DPostEffectsUtils::ShBeginPass(pSh, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

	static const int texStateID = CTexture::GetTexState(STexState(FILTER_POINT, true));
	pChart->Apply(0, texStateID);

	int offs;
	SVF_P3F_C4B_T2F* pVerts = (SVF_P3F_C4B_T2F*) m_pRenderer->GetVBPtr(4, offs, POOL_P3F_COL4UB_TEX2F);





	pVerts[0].xyz = Vec3(x, y, 0);
	pVerts[0].st = Vec2(1, 0);

	pVerts[1].xyz = Vec3(x+w, y, 0);
	pVerts[1].st = Vec2(1, 1);

	pVerts[2].xyz = Vec3(x, y+h, 0);
	pVerts[2].st = Vec2(0, 0);

	pVerts[3].xyz = Vec3(x+w, y+h, 0);
	pVerts[3].st = Vec2(0, 1);



	m_pRenderer->UnlockVB(POOL_P3F_COL4UB_TEX2F);

	m_pRenderer->FX_Commit();
	m_pRenderer->EF_SetState(GS_NODEPTHTEST);
	m_pRenderer->SetCullMode(R_CULL_NONE);

	m_pRenderer->FX_SetVStream(0, m_pRenderer->m_pVB[POOL_P3F_COL4UB_TEX2F], 0, sizeof(SVF_P3F_C4B_T2F));
	if (!FAILED(m_pRenderer->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
	{
#if defined (DIRECT3D9) || defined (OPENGL)
		m_pRenderer->m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, offs, 2);
#elif defined (DIRECT3D10)
		m_pRenderer->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		m_pRenderer->m_pd3dDeviceContext->Draw(4, offs);
#endif

		m_pRenderer->m_RP.m_PS[m_pRenderer->m_RP.m_nProcessThreadID].m_nPolygons[EFSLIST_GENERAL] += 2;
		m_pRenderer->m_RP.m_PS[m_pRenderer->m_RP.m_nProcessThreadID].m_nDIPs[EFSLIST_GENERAL]++;
	}

	NPostEffects::SD3DPostEffectsUtils::ShEndPass();

	float color[4] = {1, 1, 1, 1};
	m_pRenderer->Draw2dLabel(x + w + 10.0f, y, 1.35f, color, false, "%2.1f%%", blendAmount * 100.0f);
	m_pRenderer->Draw2dLabel(x + w + 55.0f, y, 1.35f, color, false, "%s", pLayerName);
}


void CColorGradingControllerD3D::DrawDebugInfo() const
{
	if (m_pRenderer->CV_r_colorgrading_charts < 2) 
		return;

	const float w = (float) COLORCHART_HEIGHT;
	const float h = (float) COLORCHART_WIDTH;

	float x = 16.0f;
	float y = 16.0f;

	if (!m_pChartStatic)
	{
		for (size_t i=0, numLayers=m_layers.size(); i<numLayers; ++i)
		{
			const SColorChartLayer& l = m_layers[i];
			CTexture* pChart = CTexture::GetByID(l.m_texID);
			DrawLayer(x, y, w, h, pChart, l.m_blendAmount, CryStringUtils::FindFileNameInPath(pChart->GetName()));
			y += h + 4;
		}
		// TODO: display text to indicate "combine color matrix with color chart" if used
		if (m_pChartToUse)
			DrawLayer(x, y, w, h, m_pChartToUse, 1, "FinalChart");
	}
	else
		DrawLayer(x, y, w, h, m_pChartStatic, 1, CryStringUtils::FindFileNameInPath(m_pChartStatic->GetName()));

	m_pRenderer->RT_FlushTextMessages();
}


bool CColorGradingControllerD3D::LoadStaticColorChart(const char* pChartFilePath)
{
	SAFE_RELEASE(m_pChartStatic);
	if (pChartFilePath && pChartFilePath[0] != '\0')
	{
		m_pChartStatic = LoadColorChartInt(pChartFilePath);
		return m_pChartStatic != 0;
	}
	return true;
}


const CTexture* CColorGradingControllerD3D::GetStaticColorChart() const
{
	return m_pChartStatic;
}
