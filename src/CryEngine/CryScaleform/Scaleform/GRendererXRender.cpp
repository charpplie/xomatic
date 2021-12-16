#include "StdAfx.h"


#ifndef EXCLUDE_SCALEFORM_SDK

#include "GRendererXRender.h"
#include "GTextureXRender.h"
#include "SharedStates.h"

#include <ISystem.h>
#include <IRenderer.h>
#include <IConsole.h>


ICVar* GRendererXRender::CV_sys_flash_debugdraw(0);
int GRendererXRender::ms_sys_flash_debugdraw(0);

ICVar* GRendererXRender::CV_sys_flash_newstencilclear(0);
int GRendererXRender::ms_sys_flash_newstencilclear(1);


GRendererXRender::GRendererXRender( IObserverLifeTime_GRenderer* pObserver )
: m_pXRender(0)
, m_pObserver(pObserver)
, m_stencilAvail(false)
, m_renderMasked(false)
, m_stencilCounter(0)
, m_scissorEnabled(false)
, m_applyHalfPixelOffset(false)
, m_maxVertices(0)
, m_maxIndices(0)
, m_matViewport()
, m_matUncached()
, m_matCached2D()
, m_matCached3D()
, m_matCachedWVP()
, m_mat()
, m_pMatWorld3D(0)
, m_matView3D()
, m_matProj3D()
, m_matCached2DDirty(false)
, m_matCached3DDirty(false)
, m_matCachedWVPDirty(false)
, m_cxform()
, m_glyphVB()
, m_blendOpStack()
, m_curBlendMode(Blend_None)
, m_renderStats()
, m_pDrawParams(0)
, m_canvasRect(0, 0, 0, 0)
, m_compDepth(0)
, m_maxParallax(0)
, m_screenDepth(1)
, m_planeDepth(1)
{
	m_pXRender = gEnv->pRenderer;
	assert(m_pXRender);

	m_stencilAvail = m_pXRender->GetStencilBpp() != 0;

	ERenderType rndType = m_pXRender->GetRenderType();
	m_applyHalfPixelOffset = rndType == eRT_DX9 || rndType == eRT_Xbox360;

	m_blendOpStack.reserve(16);
	m_renderStats.Clear();
	m_pDrawParams = new SSF_GlobalDrawParams;
	
	m_pXRender->SF_GetMeshMaxSize(m_maxVertices, m_maxIndices);

	if (!CV_sys_flash_debugdraw)
		CV_sys_flash_debugdraw = REGISTER_CVAR2("sys_flash_debugdraw", &ms_sys_flash_debugdraw, 0, VF_CHEAT, "Enables/disables debug drawing of flash files.\n");

	if (!CV_sys_flash_newstencilclear)
		CV_sys_flash_newstencilclear = REGISTER_CVAR2("sys_flash_newstencilclear", &ms_sys_flash_newstencilclear, 1, VF_CHEAT,"");
}


GRendererXRender::~GRendererXRender()
{
	delete m_pDrawParams;

	if (m_pObserver)
		m_pObserver->OnDestroy(this);
}


bool GRendererXRender::GetRenderCaps( RenderCaps* pCaps )
{
	pCaps->CapBits = Cap_Index16 | Cap_FillGouraud | Cap_FillGouraudTex | Cap_CxformAdd | Cap_NestedMasks; // should we allow 32 bit indices if supported?
	pCaps->BlendModes	= (1 << Blend_None) | (1 << Blend_Normal) | (1 << Blend_Multiply) | (1 << Blend_Lighten) | (1 << Blend_Darken) | (1 << Blend_Add) | (1 << Blend_Subtract);
	pCaps->VertexFormats = (1 << Vertex_None) | (1 << Vertex_XY16i) | (1 << Vertex_XY16iC32) | (1 << Vertex_XY16iCF32);
	pCaps->MaxTextureSize = m_pXRender->GetMaxTextureSize();
	return true;
}


GTexture* GRendererXRender::CreateTexture()
{
	return new GTextureXRender(this);
}


GTexture* GRendererXRender::CreateTextureYUV()
{
	assert(0 && "GRendererXRender::CreateTextureYUV() -- Not implemented!");
	return 0;
}


GRenderTarget* GRendererXRender::CreateRenderTarget()
{
	assert(0 && "GRendererXRender::CreateRenderTarget() -- Not implemented!");
	return 0;
}


void GRendererXRender::SetDisplayRenderTarget(GRenderTarget* pRT, bool setState)
{
	assert(0 && "GRendererXRender::SetDisplayRenderTarget() -- Not implemented!");
}


void GRendererXRender::PushRenderTarget(const GRectF& frameRect, GRenderTarget* pRT)
{
	assert(0 && "GRendererXRender::PushRenderTarget() -- Not implemented!");
}


void GRendererXRender::PopRenderTarget()
{
	assert(0 && "GRendererXRender::PopRenderTarget() -- Not implemented!");
}


GTexture* GRendererXRender::PushTempRenderTarget(const GRectF& frameRect, UInt targetW, UInt targetH)
{
	assert(0 && "GRendererXRender::PushTempRenderTarget() -- Not implemented!");
	return 0;
}


static inline void matrixOrthoOffCenterLH(Matrix44& mat, f32 l, f32 r, f32 b, f32 t)
{
	mat.m00 = 2.0f/(r-l); mat.m01 = 0;          mat.m02 = 0;    mat.m03 = (l+r)/(l-r);
	mat.m10 = 0;          mat.m11 = 2.0f/(t-b); mat.m12 = 0;    mat.m13 = (t+b)/(b-t);
	mat.m20 = 0;          mat.m21 = 0;          mat.m22 = 1.0f; mat.m23 = 0.0f;
	mat.m30 = 0;          mat.m31 = 0;          mat.m32 = 0;    mat.m33 = 1.0f;
}


void GRendererXRender::BeginDisplay( GColor backgroundColor, const GViewport& viewport, Float x0, Float x1, Float y0, Float y1 )
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SCALEFORMGFX, g_bProfilerEnabled);

	assert((x1 - x0) != 0 && (y1 - y0) != 0);

	float invWidth(1.0f / max((x1 - x0), 1.0f));
	float invHeight(1.0f / max((y1 - y0), 1.0f));

	int rtX0(0), rtY0(0), rtWidth(0), rtHeight(0);
	m_pXRender->GetViewport(&rtX0, &rtY0, &rtWidth, &rtHeight);
	rtWidth = max(rtWidth, 1); rtHeight = max(rtHeight, 1);

	Matrix44 matScale = Matrix44(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
	matScale.m00 = (float) viewport.Width * invWidth;
	matScale.m11 = (float) viewport.Height * invHeight;
	matScale.m03 = -x0 * invWidth + (float) viewport.Left;
	matScale.m13 = -y0 * invHeight + (float) viewport.Top;

	Matrix44 matRescale;
	matrixOrthoOffCenterLH(matRescale, (f32) rtX0, (f32) (rtX0 + rtWidth), (f32) (rtY0 + rtHeight), (f32) rtY0);
	if (m_applyHalfPixelOffset)
	{
		matRescale.m03 -= 1.0f / (float) rtWidth;
		matRescale.m13 += 1.0f / (float) rtHeight;
	}

	matRescale.m03 += m_maxParallax * (1.0f - m_screenDepth / m_planeDepth);

	m_matViewport = matRescale * matScale;

	m_scissorEnabled = (viewport.Flags & GViewport::View_UseScissorRect) != 0;
	if (m_scissorEnabled)
	{
		int scX0(rtX0), scY0(rtY0), scX1(rtX0 + rtWidth), scY1(rtY0 + rtHeight);
		if (scX0 < viewport.ScissorLeft)
			scX0 = viewport.ScissorLeft;
		if (scY0 < viewport.ScissorTop)
			scY0 = viewport.ScissorTop;
		if (scX1 > viewport.ScissorLeft + viewport.ScissorWidth)
			scX1 = viewport.ScissorLeft + viewport.ScissorWidth;
		if (scY1 > viewport.ScissorTop + viewport.ScissorHeight)
			scY1 = viewport.ScissorTop + viewport.ScissorHeight;

		m_pXRender->SetScissor(scX0, scY0, scX1 - scX0, scY1 - scY0);
	}

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.blendModeStates = 0;
	params.renderMaskedStates = 0;
	m_stencilCounter = 0;

	m_blendOpStack.resize(0);
	ApplyBlendMode(Blend_None);

	Clear(backgroundColor, x0, y0, x1, y1);

	m_canvasRect = SRect(x0, y0, x1, y1);
}


void GRendererXRender::EndDisplay()
{
	assert(!m_stencilCounter);

	if (m_scissorEnabled)
		m_pXRender->SetScissor();
}


void GRendererXRender::SetMatrix( const GMatrix2D& m )
{
	m_mat = m;
	m_matCached2DDirty = true;
	m_matCached3DDirty = true;
}


void GRendererXRender::SetUserMatrix( const GMatrix2D& m )
{
	assert(0 && "GRendererXRender::SetUserMatrix() -- Not implemented!");
}


void GRendererXRender::SetCxform( const Cxform& cx )
{
	m_cxform = cx;
}


void GRendererXRender::PushBlendMode( BlendType mode )
{
	m_blendOpStack.push_back(m_curBlendMode);

	if ((mode > Blend_Layer) && (m_curBlendMode != mode))
	{
		ApplyBlendMode(mode);
	}
}


void GRendererXRender::PopBlendMode()
{
	assert(m_blendOpStack.size() && "GRendererXRender::PopBlendMode() -- Blend mode stack is empty!");

	if (m_blendOpStack.size())
	{
		BlendType	newBlendMode(Blend_None);

		for (int i(m_blendOpStack.size() - 1); i >= 0;  --i)
		{
			if (m_blendOpStack[i] > Blend_Layer)
			{
				newBlendMode = m_blendOpStack[i];
				break;
			}
		}

		m_blendOpStack.pop_back();

		if (newBlendMode != m_curBlendMode)
		{
			ApplyBlendMode(newBlendMode);
		}
	}
}


void GRendererXRender::SetPerspective3D(const GMatrix3D& projMatIn)
{
	m_matProj3D = projMatIn;
	assert(m_matProj3D.M_[2][3] == -1.0f && "GRendererXRender::SetPerspective3D() -- projMatIn is not right handed");
	m_matProj3D.M_[2][0] = -m_maxParallax; // negative here as m_matProj3D is RH
	m_matProj3D.M_[3][0] = -m_maxParallax * m_screenDepth;
	m_matCachedWVPDirty = true;
	m_matCached3DDirty = true;
}


void GRendererXRender::SetView3D(const GMatrix3D& viewMatIn)
{
	m_matView3D = viewMatIn;
	m_matCachedWVPDirty = true;
	m_matCached3DDirty = true;
}


void GRendererXRender::SetWorld3D(const GMatrix3D* pWorldMatIn)
{
	m_pMatWorld3D = pWorldMatIn;
	m_matCachedWVPDirty = pWorldMatIn != 0;
	m_matCached3DDirty = pWorldMatIn != 0;
}


void GRendererXRender::ApplyBlendMode( BlendType blendMode )
{
	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;

	if (ms_sys_flash_debugdraw)
	{
		m_curBlendMode = Blend_None;
		params.blendModeStates = 0;
		return;
	}

	m_curBlendMode = blendMode;
	params.blendModeStates = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;

	switch (blendMode)
	{
	case Blend_None:
	case Blend_Normal:
	case Blend_Layer:
	case Blend_Screen:
	case Blend_Difference:
	case Blend_Invert:
	case Blend_Overlay:
	case Blend_HardLight:
		params.blendModeStates = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
		break;
	case Blend_Multiply:
		params.blendModeStates = GS_BLSRC_DSTCOL | GS_BLDST_ZERO;
		break;
	case Blend_Add:
	case Blend_Subtract:
	case Blend_Lighten:
	case Blend_Darken:
		params.blendModeStates = GS_BLSRC_SRCALPHA | GS_BLDST_ONE;
		break;
	case Blend_Alpha:
	case Blend_Erase:
		params.blendModeStates = GS_BLSRC_ZERO | GS_BLDST_ONE;
		break;
	default:
		assert(!"GRendererXRender::ApplyBlendMode() -- Unsupported blend type!");
		break;
	}

	switch (blendMode)
	{
	case Blend_Subtract:
		params.blendOp = SSF_GlobalDrawParams::RevSubstract;
		break;
	case Blend_Lighten:
		params.blendOp = SSF_GlobalDrawParams::Max;
		break;
	case Blend_Darken:
		params.blendOp = SSF_GlobalDrawParams::Min;
		break;
	default: 
		params.blendOp = SSF_GlobalDrawParams::Add;
		break;
	}
	params.isMultiplyDarkBlendMode = m_curBlendMode == Blend_Multiply || m_curBlendMode == Blend_Darken;
}


void GRendererXRender::ApplyColor( const GColor& src )
{
	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	
	ColorF& dst(params.colTransform1st);
	const float scale(1.0f / 255.0f);
	dst.r = src.GetRed() * scale;
	dst.g = src.GetGreen() * scale;
	dst.b = src.GetBlue() * scale;
	dst.a = src.GetAlpha() * scale;
	if (m_curBlendMode == Blend_Multiply || m_curBlendMode == Blend_Darken)
	{
		float a(dst.a);
		dst.r = 1.0f + a * (dst.r - 1.0f);
		dst.g = 1.0f + a * (dst.g - 1.0f);
		dst.b = 1.0f + a * (dst.b - 1.0f);
	}

	params.colTransform2nd = ColorF(0, 0, 0, 0);
}


void GRendererXRender::ApplyTextureInfo(unsigned int texSlot, const FillTexture* pFill)
{
	assert(texSlot < 2);
	if (texSlot >= 2)
		return;

	SSF_GlobalDrawParams::STextureInfo& texInfo(m_pDrawParams->texture[texSlot]);
	if (pFill && pFill->pTexture)
	{
		const GTextureXRender* pTex(static_cast<const GTextureXRender*>(pFill->pTexture));

		const GMatrix2D& m(pFill->TextureMatrix);

		texInfo.texID = pTex->GetID();

		texInfo.texGenMat.m00 = m.M_[0][0];
		texInfo.texGenMat.m01 = m.M_[0][1];
		texInfo.texGenMat.m02 = 0; 
		texInfo.texGenMat.m03 = m.M_[0][2];

		texInfo.texGenMat.m10 = m.M_[1][0];
		texInfo.texGenMat.m11 = m.M_[1][1];
		texInfo.texGenMat.m12 = 0; 
		texInfo.texGenMat.m13 = m.M_[1][2];

		texInfo.texGenMat.m20 = 0;
		texInfo.texGenMat.m21 = 0;
		texInfo.texGenMat.m22 = 0;
		texInfo.texGenMat.m23 = 1;

		texInfo.texState = (pFill->WrapMode == Wrap_Clamp) ? SSF_GlobalDrawParams::TS_Clamp : 0;
		texInfo.texState |= (pFill->SampleMode == Sample_Linear) ? SSF_GlobalDrawParams::TS_FilterLin : 0;
	}
	else
	{
		texInfo.texID = 0;
		texInfo.texGenMat.SetIdentity();
		texInfo.texState = 0;
	}
}


void GRendererXRender::ApplyCxform()
{
	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.colTransform1st = ColorF(m_cxform.M_[0][0], m_cxform.M_[1][0], m_cxform.M_[2][0], m_cxform.M_[3][0]);
	params.colTransform2nd = ColorF(m_cxform.M_[0][1] / 255.0f, m_cxform.M_[1][1] / 255.0f, m_cxform.M_[2][1] / 255.0f, m_cxform.M_[3][1] / 255.0f);
}


void GRendererXRender::CalcTransMat2D(const GMatrix2D* pSrc, Matrix44* __restrict pDst) const
{
	assert(pSrc && pDst);

	Matrix44 mat;
	mat.m00 = pSrc->M_[0][0]; mat.m01 = pSrc->M_[0][1]; mat.m02 = 0; mat.m03 = pSrc->M_[0][2];
	mat.m10 = pSrc->M_[1][0]; mat.m11 = pSrc->M_[1][1]; mat.m12 = 0; mat.m13 = pSrc->M_[1][2];
	mat.m20 = 0;							mat.m21 = 0;							mat.m22 = 0; mat.m23 = m_compDepth;
	mat.m30 = 0;							mat.m31 = 0;							mat.m32 = 0; mat.m33 = 1;

	*pDst = m_matViewport * mat;
}


void GRendererXRender::CalcTransMat3D(const GMatrix2D* pSrc, Matrix44* __restrict pDst)
{
	assert(pSrc && pDst);
	assert(m_pMatWorld3D);

	if (m_matCachedWVPDirty)
	{
		GMatrix3D mat = *m_pMatWorld3D;
		mat.Append(m_matView3D);
		mat.Append(m_matProj3D);

		m_matCachedWVP.m00 = mat.M_[0][0]; m_matCachedWVP.m01 = mat.M_[1][0]; m_matCachedWVP.m02 = mat.M_[2][0]; m_matCachedWVP.m03 = mat.M_[3][0];
		m_matCachedWVP.m10 = mat.M_[0][1]; m_matCachedWVP.m11 = mat.M_[1][1]; m_matCachedWVP.m12 = mat.M_[2][1]; m_matCachedWVP.m13 = mat.M_[3][1];
		// TODO: !!! Hack to force depth test to always pass (clip z/w = 0). Remove after E3!!!
		//m_matCachedWVP.m20 = mat.M_[0][2]; m_matCachedWVP.m21 = mat.M_[1][2]; m_matCachedWVP.m22 = mat.M_[2][2]; m_matCachedWVP.m23 = mat.M_[3][2];
		m_matCachedWVP.m20 = 0; m_matCachedWVP.m21 = 0; m_matCachedWVP.m22 = 0; m_matCachedWVP.m23 = 0;
		m_matCachedWVP.m30 = mat.M_[0][3]; m_matCachedWVP.m31 = mat.M_[1][3]; m_matCachedWVP.m32 = mat.M_[2][3]; m_matCachedWVP.m33 = mat.M_[3][3];

		m_matCachedWVPDirty = false;
	}

	Matrix44 mat;
	mat.m00 = pSrc->M_[0][0]; mat.m01 = pSrc->M_[0][1]; mat.m02 = 0; mat.m03 = pSrc->M_[0][2];
	mat.m10 = pSrc->M_[1][0]; mat.m11 = pSrc->M_[1][1]; mat.m12 = 0; mat.m13 = pSrc->M_[1][2];
	mat.m20 = 0;							mat.m21 = 0;							mat.m22 = 1; mat.m23 = 0;
	mat.m30 = 0;							mat.m31 = 0;							mat.m32 = 0; mat.m33 = 1;

	*pDst = m_matCachedWVP * mat;
}


void GRendererXRender::ApplyMatrix(const GMatrix2D* pMatIn)
{
	assert(pMatIn);

	bool is3D = m_pMatWorld3D != 0;
	bool useCaching = pMatIn == &m_mat;

	Matrix44* __restrict pMatOut = 0;

	if (!is3D)
	{
		pMatOut = useCaching ? &m_matCached2D : &m_matUncached;
		if (!useCaching || m_matCached2DDirty)
		{
			CalcTransMat2D(pMatIn, pMatOut);
			if (useCaching)
				m_matCached2DDirty = false;
		}
	}
	else
	{
		pMatOut = useCaching ? &m_matCached3D : &m_matUncached;
		if (!useCaching || m_matCached3DDirty)
		{
			CalcTransMat3D(pMatIn, pMatOut);
			if (useCaching)
				m_matCached3DDirty = false;
		}
	}

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.pTransMat = pMatOut;
}


void GRendererXRender::SetVertexData( const void* pVertices, int numVertices, VertexFormat vf, CacheProvider* /*pCache*/ )
{
	assert((!pVertices || vf == Vertex_XY16i || vf == Vertex_XY16iC32 || vf == Vertex_XY16iCF32) && "GRendererXRender::SetVertexData() -- Unsupported source vertex format!");
	if (pVertices && numVertices > 0 && numVertices <= m_maxVertices && (vf == Vertex_XY16i || vf == Vertex_XY16iC32 || vf == Vertex_XY16iCF32))
	{
		SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
		switch (vf)
		{
		case Vertex_XY16i:
			params.vertexFmt = SSF_GlobalDrawParams::Vertex_XY16i;
			break;
		case Vertex_XY16iC32:
			params.vertexFmt = SSF_GlobalDrawParams::Vertex_XY16iC32;
			break;
		case Vertex_XY16iCF32:
			params.vertexFmt = SSF_GlobalDrawParams::Vertex_XY16iCF32;
			break;
		}
		params.pVertexPtr = pVertices;
		params.numVertices = numVertices;
	}
	else
	{
		SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
		params.vertexFmt = SSF_GlobalDrawParams::Vertex_None;
		params.pVertexPtr = 0;
		params.numVertices = 0;

		if (numVertices > m_maxVertices)
			CryGFxLog::GetAccess().LogWarning("Trying to send too many vertices at once (amount = %d, limit = %d)!", numVertices, m_maxVertices);
	}
}


void GRendererXRender::SetIndexData( const void* pIndices, int numIndices, IndexFormat idxf, CacheProvider* /*pCache*/ )
{
	assert((!pIndices || idxf == Index_16) && "GRendererXRender::SetIndexData() -- Unsupported source index format!");
	if (pIndices && numIndices > 0 && numIndices <= m_maxIndices && idxf == Index_16)
	{
		SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
		params.indexFmt = SSF_GlobalDrawParams::Index_16;
		params.pIndexPtr = pIndices;
		params.numIndices = numIndices;
	}
	else
	{
		SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
		params.indexFmt = SSF_GlobalDrawParams::Index_None;
		params.pIndexPtr = 0;
		params.numIndices = 0;

		if (numIndices > m_maxIndices)
			CryGFxLog::GetAccess().LogWarning("Trying to send too many indices at once (amount = %d, limit = %d)!", numIndices, m_maxIndices);
	}
}


void GRendererXRender::ReleaseCachedData( CachedData* pData, CachedDataType type )
{
	// currently we're not keeping track of cached buffers, might be implemented later
}


void GRendererXRender::DrawIndexedTriList( int baseVertexIndex, int minVertexIndex, int numVertices, int startIndex, int triangleCount )
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SCALEFORMGFX, g_bProfilerEnabled);
	
	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	if ((m_renderMasked && !m_stencilAvail) || params.vertexFmt == SSF_GlobalDrawParams::Vertex_None || params.indexFmt == SSF_GlobalDrawParams::Index_None)
		return;

	assert(params.vertexFmt != SSF_GlobalDrawParams::Vertex_None);
	assert(params.indexFmt == SSF_GlobalDrawParams::Index_16);

	// setup render parameters
	ApplyMatrix(&m_mat);

	// draw triangle list
	m_pXRender->SF_DrawIndexedTriList(baseVertexIndex, minVertexIndex, numVertices, startIndex, triangleCount, params);

	params.pVertexPtr = 0;
	params.numVertices = 0;
	params.pIndexPtr = 0;
	params.numIndices = 0;

	// update stats
	m_renderStats.Triangles += triangleCount;
	++m_renderStats.Primitives;
}


void GRendererXRender::DrawLineStrip( int baseVertexIndex, int lineCount )
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SCALEFORMGFX, g_bProfilerEnabled);

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	if ((m_renderMasked && !m_stencilAvail) || params.vertexFmt != SSF_GlobalDrawParams::Vertex_XY16i)
		return;

	// setup render parameters
	ApplyMatrix(&m_mat);

	// draw triangle list
	if (params.numVertices)
		m_pXRender->SF_DrawLineStrip(baseVertexIndex, lineCount, params);

	params.pVertexPtr = 0;
	params.numVertices = 0;

	// update stats
	m_renderStats.Lines += lineCount;
	++m_renderStats.Primitives;
}


void GRendererXRender::LineStyleDisable()
{
}


void GRendererXRender::LineStyleColor( GColor color )
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SCALEFORMGFX, g_bProfilerEnabled);

	ApplyTextureInfo(0);
	ApplyTextureInfo(1);
	ApplyColor(m_cxform.Transform(color));

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.fillType = SSF_GlobalDrawParams::SolidColor; 
}


void GRendererXRender::FillStyleDisable()
{
}


void GRendererXRender::FillStyleColor( GColor color )
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SCALEFORMGFX, g_bProfilerEnabled);

	ApplyTextureInfo(0);
	ApplyTextureInfo(1);
	ApplyColor(m_cxform.Transform(color));

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.fillType = SSF_GlobalDrawParams::SolidColor; 
}


void GRendererXRender::FillStyleBitmap( const FillTexture* pFill )
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SCALEFORMGFX, g_bProfilerEnabled);

	ApplyTextureInfo(0, pFill);
	ApplyTextureInfo(1);
	ApplyCxform();

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.fillType = SSF_GlobalDrawParams::Texture; 
}


void GRendererXRender::FillStyleGouraud( GouraudFillType fillType, const FillTexture* pTexture0, const FillTexture* pTexture1, const FillTexture* pTexture2 )
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SCALEFORMGFX, g_bProfilerEnabled);

	const FillTexture* t0(0);
	const FillTexture* t1(0);

	if (pTexture0 || pTexture1 || pTexture2)
	{
		t0 = pTexture0;
		if (!t0)
			t0 = pTexture1;
		if (pTexture1)
			t1 = pTexture1;
	}

	ApplyTextureInfo(0, t0);
	ApplyTextureInfo(1, t1);
	ApplyCxform();

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	switch( fillType )
	{
	case GFill_Color:
		params.fillType = SSF_GlobalDrawParams::GColor;
		break;
	case GFill_1Texture:
		params.fillType = SSF_GlobalDrawParams::G1Texture;
		break;
	case GFill_1TextureColor:
		params.fillType = SSF_GlobalDrawParams::G1TextureColor;
		break;
	case GFill_2Texture:
		params.fillType = SSF_GlobalDrawParams::G2Texture;
		break;
	case GFill_2TextureColor:
		params.fillType = SSF_GlobalDrawParams::G2TextureColor;
		break;
	case GFill_3Texture:
		params.fillType = SSF_GlobalDrawParams::G3Texture;
		break;
	default:
		assert(0);
		break;
	}
}


void GRendererXRender::DrawBitmaps( BitmapDesc* pBitmapList, int /*listSize*/, int startIndex, int count, const GTexture* pTi, const GMatrix2D& m, CacheProvider* /*pCache*/ )
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SCALEFORMGFX, g_bProfilerEnabled);

	if (!pBitmapList || !pTi)
		return;

	// resize buffer
	uint32 numVertices(4 * count + ( count - 1 ) * 2);
	m_glyphVB.resize(numVertices);

	// merge multiple bitmaps into one draw call via stitching the triangle strip
	for( int bitmapIdx( 0 ), vertOffs( 0 ); bitmapIdx < count; ++bitmapIdx )
	{
		const BitmapDesc& bitmapDesc(pBitmapList[bitmapIdx + startIndex]);
		const Rect& coords(bitmapDesc.Coords);
		const Rect& uvCoords(bitmapDesc.TextureCoords);

		const GColor& srcCol(bitmapDesc.Color);
		unsigned int color((srcCol.GetAlpha() << 24) | (srcCol.GetBlue() << 16) | (srcCol.GetGreen() << 8) | srcCol.GetRed());
		SGlyphVertex& v0(m_glyphVB[vertOffs]);
		v0.x = coords.Left; v0.y = coords.Top; v0.u = uvCoords.Left; v0.v = uvCoords.Top; v0.col = color;
		++vertOffs;

		if (bitmapIdx > 0)
		{
			m_glyphVB[vertOffs] = m_glyphVB[vertOffs-1];
			++vertOffs;
		}

		SGlyphVertex& v1(m_glyphVB[vertOffs]);
		v1.x = coords.Right; v1.y = coords.Top; v1.u = uvCoords.Right; v1.v = uvCoords.Top; v1.col = color;
		++vertOffs;

		SGlyphVertex& v2(m_glyphVB[vertOffs]);
		v2.x = coords.Left; v2.y = coords.Bottom; v2.u = uvCoords.Left; v2.v = uvCoords.Bottom; v2.col = color;
		++vertOffs;

		SGlyphVertex& v3(m_glyphVB[vertOffs]);
		v3.x = coords.Right; v3.y = coords.Bottom; v3.u = uvCoords.Right; v3.v = uvCoords.Bottom; v3.col = color;
		++vertOffs;

		if (bitmapIdx < count - 1)
		{
			m_glyphVB[vertOffs] = m_glyphVB[vertOffs-1];
			++vertOffs;
		}
	}

	// setup render parameters
	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;

	ApplyMatrix(&m);

	const GTextureXRender* pTex(static_cast<const GTextureXRender*>(pTi));
	params.texture[0].texID = pTex ? pTex->GetID() : 0;
	params.texture[0].texGenMat.SetIdentity();
	params.texture[0].texState = SSF_GlobalDrawParams::TS_FilterTriLin | SSF_GlobalDrawParams::TS_Clamp;

	ApplyTextureInfo(1);
	ApplyCxform();

	params.fillType = pTex->GetFmt() != eTF_A8 ? SSF_GlobalDrawParams::GlyphTexture : SSF_GlobalDrawParams::GlyphAlphaTexture;
	params.vertexFmt = SSF_GlobalDrawParams::Vertex_Glyph;
	params.pVertexPtr = &m_glyphVB[0];
	params.numVertices = numVertices;

	// draw glyphs
	m_pXRender->SF_DrawGlyphClear(params);

	params.pVertexPtr = 0;
	params.numVertices = 0;

	// update stats
	m_renderStats.Triangles += numVertices - 2;
	++m_renderStats.Primitives;
}


void GRendererXRender::BeginSubmitMask( SubmitMaskMode maskMode )
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SCALEFORMGFX, g_bProfilerEnabled);

	m_renderMasked = true;
	if (!m_stencilAvail)
		return;

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.renderMaskedStates = GS_STENCIL | GS_COLMASK_NONE;

	switch (maskMode)
	{
	case Mask_Clear:
		{
			if (!ms_sys_flash_newstencilclear)
				m_pXRender->ClearBuffer(FRT_CLEAR_STENCIL, 0);
			else
			{
				m_pXRender->SF_ConfigMask(IRenderer::BeginSubmitMask_Clear, 0);
				Clear(GColor(0, 0, 0, 255), m_canvasRect.m_x0, m_canvasRect.m_y0, m_canvasRect.m_x1, m_canvasRect.m_y1);
			}
			m_pXRender->SF_ConfigMask(IRenderer::BeginSubmitMask_Clear, 1);
			m_stencilCounter = 1;
			break;
		}
	case Mask_Increment:
		{
			m_pXRender->SF_ConfigMask(IRenderer::BeginSubmitMask_Inc, m_stencilCounter);
			++m_stencilCounter;
			break;
		}
	case Mask_Decrement:
		{
			m_pXRender->SF_ConfigMask(IRenderer::BeginSubmitMask_Dec, m_stencilCounter);
			--m_stencilCounter;
			break;
		}
	}
	++m_renderStats.Masks;
}


void GRendererXRender::EndSubmitMask()
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SCALEFORMGFX, g_bProfilerEnabled);

	m_renderMasked = true;
	if (!m_stencilAvail)
		return;

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.renderMaskedStates = GS_STENCIL;

	m_pXRender->SF_ConfigMask(IRenderer::EndSubmitMask, m_stencilCounter);
}


void GRendererXRender::DisableMask()
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SCALEFORMGFX, g_bProfilerEnabled);

	m_renderMasked = false;
	if (!m_stencilAvail)
		return;

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;
	params.renderMaskedStates = 0;

	m_pXRender->SF_ConfigMask(IRenderer::DisableMask, 0);
	m_stencilCounter = 0;
}


UInt GRendererXRender::CheckFilterSupport(const BlurFilterParams& params)
{
	return FilterSupport_None; // filter support to be added later
}


void GRendererXRender::DrawBlurRect(GTexture* pSrcIn, const GRectF& inSrcRect, const GRectF& inDestRect, const BlurFilterParams& params)
{
	assert(0 && "GRendererXRender::DrawBlurRect() -- Not implemented!");
}


void GRendererXRender::DrawColorMatrixRect(GTexture* pSrcIn, const GRectF& inSrcRect, const GRectF& inDestRect, const Float* pMatrix)
{
	assert(0 && "GRendererXRender::DrawBlurRect() -- Not implemented!");
}


void GRendererXRender::GetRenderStats( Stats* pStats, bool resetStats )
{
	if (pStats)
		memcpy(pStats, &m_renderStats, sizeof(Stats));
	if (resetStats)
		m_renderStats.Clear();
}


void GRendererXRender::GetStats( GStatBag* pBag, bool reset )
{
	assert(0 && "GRendererXRender::GRendererXRender::GetStats() -- Not implemented!");
}


void GRendererXRender::ReleaseResources()
{
	// nothing to do here atm
}


IRenderer* GRendererXRender::GetXRender() const
{
	return m_pXRender;
}


void GRendererXRender::Clear(const GColor& backgroundColor, float x0, float y0, float x1, float y1)
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SCALEFORMGFX, g_bProfilerEnabled);

	if (backgroundColor.GetAlpha() == 0)
		return;

	SSF_GlobalDrawParams& params = *(SSF_GlobalDrawParams*) m_pDrawParams;

	// setup quad
	uint16 quad[4*2] =
	{
		(uint16)x0, (uint16)y0,
		(uint16)x1, (uint16)y0,
		(uint16)x0, (uint16)y1,
		(uint16)x1, (uint16)y1
	};

	// setup render parameters
	ApplyTextureInfo(0);
	ApplyTextureInfo(1);
	ApplyColor(backgroundColor);

	params.pTransMat = &m_matViewport;
	params.fillType = SSF_GlobalDrawParams::SolidColor;
	params.vertexFmt = SSF_GlobalDrawParams::Vertex_XY16i;
	params.pVertexPtr = &quad[0];
	params.numVertices = 4;

	int oldBendState(params.blendModeStates);
	params.blendModeStates = backgroundColor.GetAlpha() < 255 ? GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA : 0;
	int oldRenderMaskedStates(params.renderMaskedStates);
	params.renderMaskedStates = m_renderMasked ? params.renderMaskedStates : 0;

	// draw quad
	m_pXRender->SF_DrawGlyphClear(params);

	params.pVertexPtr = 0;
	params.numVertices = 0;
	params.blendModeStates = oldBendState;
	params.renderMaskedStates = oldRenderMaskedStates;

	// update stats
	m_renderStats.Triangles += 2;
	++m_renderStats.Primitives;
}


void GRendererXRender::SetCompositingDepth(float depth)
{
	m_compDepth = (depth >= 0 && depth <= 1) ? depth : 0;
}


void GRendererXRender::SetStereoMode(bool stereo, bool isLeft)
{
	//////////////////////////////////////////////////////////////////////////
	// TODO: dbg code, remove
	static float s_maxParallax = 0;
	{
		static ICVar* p = gEnv->pConsole->Register("sys_flash_stereo_maxparallax", &s_maxParallax, s_maxParallax);
	}
	//////////////////////////////////////////////////////////////////////////

	m_maxParallax = !stereo ? 0 : isLeft ? s_maxParallax : -s_maxParallax; // TODO: use system value
	//m_maxParallax = s_maxParallax;
}


void GRendererXRender::SetStereoPlaneDepth(float depth)
{
	//////////////////////////////////////////////////////////////////////////
	// TODO: dbg code, remove
	static float s_stereoPlaneDepth = 10.0f;
	{
		static ICVar* p = gEnv->pConsole->Register("sys_flash_stereo_planedepth", &s_stereoPlaneDepth, s_stereoPlaneDepth);
	}
	depth = s_stereoPlaneDepth;

	static float s_stereoScreenDepth = 1.0f;
	{
		static ICVar* p = gEnv->pConsole->Register("sys_flash_stereo_screendepth", &s_stereoScreenDepth, s_stereoScreenDepth);
	}
	m_screenDepth = max(s_stereoScreenDepth , 0.0f); // TODO: use system value, move to BeginDisplay
	//////////////////////////////////////////////////////////////////////////

	m_planeDepth = max(depth, 0.1f);
}


#endif // #ifndef EXCLUDE_SCALEFORM_SDK